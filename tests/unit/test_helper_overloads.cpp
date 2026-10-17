// test_helper_overloads.cpp — helper overload CÙNG arity khác kiểu tham số.
// Complementary Reimagined định nghĩa pow2/pow3/sqrt*/clamp01... cho
// int/float/vec2/vec3/vec4 (5 defs cùng arity 1). Converter cũ resolve theo
// arity → "helper pow2 gọi mơ hồ (trùng arity n=1 ...)" làm deferred1.vsh chết.
// Fix: nếu mọi candidate cùng shape gọi được (req + splitKind y hệt) → chọn 1,
// call site giữ tên và để MSL tự overload theo kiểu; khác req → vẫn báo mơ hồ.
// CPU-only: convert + assert MSL; có toolchain thì `metal -c` biên dịch thật.
#include "tglmt/GLSLConverter.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static int gFails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++gFails; } } while (0)
static bool Run(const std::string& cmd) { return system(cmd.c_str()) == 0; }
static bool Has(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }
static int Count(const std::string& s, const char* sub) {
    int n = 0; size_t p = 0;
    while ((p = s.find(sub, p)) != std::string::npos) { ++n; p += 1; }
    return n;
}

int main() {
    using namespace tglmt;

    // 1) nguyên văn khối pow2 của lib/util/commonFunctions.glsl (5 overload arity 1)
    const char* pow2block =
        "int pow2(int x) {\n    return x * x;\n}\n"
        "float pow2(float x) {\n    return x * x;\n}\n"
        "vec2 pow2(vec2 x) {\n    return x * x;\n}\n"
        "vec3 pow2(vec3 x) {\n    return x * x;\n}\n"
        "vec4 pow2(vec4 x) {\n    return x * x;\n}\n";

    std::string shader1 = std::string("#version 150 core\n") + pow2block +
        "out vec4 c;\n"
        "void main() {\n"
        "    int i = pow2(3);\n"
        "    float f = pow2(1.5);\n"
        "    vec2 v2 = pow2(vec2(1.0, 2.0));\n"
        "    vec3 v3 = pow2(vec3(1.0));\n"
        "    vec4 v4 = pow2(vec4(2.0));\n"
        "    c = vec4(f, v2.x, v3.y, v4.z) + vec4(float(i));\n"
        "}\n";
    GLSLConvertResult r1 = ConvertGLSLtoMSL(shader1, 0x8B30);
    CHECK(r1.ok, "pow2 5 overload convert ok");
    if (!r1.ok) printf("  log: %s\n", r1.log.c_str());
    if (r1.ok) {
        // 5 defs + 5 call sites đều giữ tên `pow2(`.
        CHECK(Count(r1.msl, "pow2(") == 10, "5 defs + 5 call giữ tên pow2");
        CHECK(Has(r1.msl, "float2 pow2(float2 x)"), "overload vec2 → float2");
        CHECK(Has(r1.msl, "float4 pow2(float4 x)"), "overload vec4 → float4");
    }

    // 2) overload gọi nhau: pow3(x) = pow2(x) * x — call trong body helper cũng
    //    phải resolve được (phase collect, không chỉ phase rewrite).
    std::string shader2 = std::string("#version 150 core\n") + pow2block +
        "float pow3(float x) {\n    return pow2(x) * x;\n}\n"
        "out vec4 c;\n"
        "void main() { c = vec4(pow3(2.0)); }\n";
    GLSLConvertResult r2 = ConvertGLSLtoMSL(shader2, 0x8B30);
    CHECK(r2.ok, "pow3 gọi pow2 mơ hồ nội bộ convert ok");
    if (!r2.ok) printf("  log: %s\n", r2.log.c_str());

    // 3) Mơ hồ THẬT (khác req: một body sample sampler) → vẫn phải báo lỗi rõ.
    {
        const char* s = "#version 150 core\n"
                        "uniform sampler2D uTex;\n"
                        "out vec4 c;\n"
                        "float amb(float x) { return x + 1.0; }\n"
                        "float amb(vec2 x) { return texture(uTex, x).r; }\n"
                        "void main() { c = vec4(amb(1.0)); }\n";
        GLSLConvertResult q = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(!q.ok, "khác req vẫn mơ hồ (fail honest)");
        if (!q.ok) CHECK(Has(q.log, "mơ hồ"), "log nói mơ hồ");
    }

    // 4) Biên dịch MSL thật nếu toolchain sẵn (probe như test_frag_out_scalar).
    if (r1.ok && Run("xcrun --sdk macosx --find metal >/dev/null 2>&1")) {
        { FILE* f = fopen("/tmp/tglmt-overload-probe.msl", "w");
          if (f) { fputs("#include <metal_stdlib>\nusing namespace metal;\nkernel void probe(){}\n", f); fclose(f); } }
        if (Run("xcrun -sdk macosx metal -c /tmp/tglmt-overload-probe.msl -o /tmp/tglmt-overload-probe.air >/dev/null 2>&1")) {
            FILE* f = fopen("/tmp/tglmt-overload-pow2.msl", "w");
            if (f) { fputs(r1.msl.c_str(), f); fclose(f); }
            CHECK(Run("xcrun -sdk macosx metal -c /tmp/tglmt-overload-pow2.msl -o /tmp/tglmt-overload-pow2.air >/dev/null 2>&1"),
                  "MSL pow2 overloads biên dịch được (metal -c)");
        } else {
            printf("SKIP: metal toolchain thiếu component\n");
        }
    }

    if (gFails) { printf("test_helper_overloads FAIL (%d)\n", gFails); return 1; }
    printf("test_helper_overloads PASS\n");
    return 0;
}
