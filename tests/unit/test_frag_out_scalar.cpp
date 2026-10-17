// test_frag_out_scalar.cpp — fragment out kiểu scalar/vecN ngoài vec4.
// Trước fix: converter chặn mọi out != vec4 → Iris centerDepth.fsh (`out float` + R32F)
// chết "fragment out phải vec4" khiến mọi shaderpack Iris fail ngay bước pipeline.
// CPU-only: convert + assert MSL; có toolchain thì `metal -c` biên dịch thật.
#include "tglmt/GLSLConverter.h"
#include <cstdio>
#include <cstdlib>
#include <string>

static int gFails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++gFails; } } while (0)
static bool Run(const std::string& cmd) { return system(cmd.c_str()) == 0; }
static bool Has(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }

int main() {
    using namespace tglmt;

    // 1) nguyên văn Iris centerDepth.fsh — out float + R32F
    const char* centerDepth = R"(#version 150 core
uniform sampler2D depth;
uniform sampler2D altDepth;
uniform float lastFrameTime;
uniform float decay;
out float iris_fragColor;
void main() {
    float currentDepth = texture(depth, vec2(0.5)).r;
    float decay2 = 1.0 - exp(-decay * lastFrameTime);
    float oldDepth = texture(altDepth, vec2(0.5)).r;
    if (isnan(oldDepth)) { oldDepth = currentDepth; }
    iris_fragColor = mix(oldDepth, currentDepth, decay2);
}
)";
    GLSLConvertResult r = ConvertGLSLtoMSL(centerDepth, 0x8B30);
    CHECK(r.ok, "centerDepth out float convert ok");
    if (!r.ok) printf("  log: %s\n", r.log.c_str());
    if (r.ok) {
        CHECK(Has(r.msl, "fragment float TGLMT_fs"), "entry point trả float");
        CHECK(Has(r.msl, "float tglmt_fragColor"), "decl float tglmt_fragColor");
        // Body ghi vào tên out gốc đã map sang tglmt_fragColor.
        CHECK(Has(r.msl, "tglmt_fragColor = "), "body ghi tglmt_fragColor");
        // Không còn ép float4.
        CHECK(!Has(r.msl, "fragment float4 TGLMT_fs"), "không còn hardcode float4");
    }

    // 2) int out (R32I) — zero literal đúng kiểu
    {
        const char* s = "#version 150 core\nout int v;\nvoid main() { v = 7; }\n";
        GLSLConvertResult q = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(q.ok, "out int convert ok");
        if (q.ok) {
            CHECK(Has(q.msl, "fragment int TGLMT_fs"), "entry trả int");
            CHECK(Has(q.msl, "int tglmt_fragColor = 0;"), "zero int đúng kiểu");
        }
    }

    // 3) vec4 single-out regression + vec2
    {
        const char* s = "#version 150 core\nout vec4 c;\nvoid main() { c = vec4(0.5); }\n";
        GLSLConvertResult q = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(q.ok, "out vec4 vẫn ok");
        if (q.ok) CHECK(Has(q.msl, "fragment float4 TGLMT_fs"), "vec4 → float4");
    }
    {
        const char* s = "#version 150 core\nout vec2 c;\nvoid main() { c = vec2(0.5); }\n";
        GLSLConvertResult q = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(q.ok, "out vec2 convert ok");
        if (q.ok) CHECK(Has(q.msl, "fragment float2 TGLMT_fs"), "vec2 → float2");
    }

    // 4) MRT: mỗi attachment kiểu riêng (vec4 + float) — Iris có pack trộn kiểu.
    {
        const char* s = "#version 150 core\nlayout(location = 0) out vec4 col;\n"
                        "layout(location = 1) out float depth;\n"
                        "void main() { col = vec4(1.0); depth = 0.25; }\n";
        GLSLConvertResult q = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(q.ok, "MRT vec4+float convert ok");
        if (q.ok) {
            CHECK(Has(q.msl, "float4 mrt0 [[color(0)]]"), "MRT slot0 float4");
            CHECK(Has(q.msl, "float mrt1 [[color(1)]]"), "MRT slot1 float");
        }
    }

    // 5) Biên dịch MSL thật nếu toolchain sẵn (probe như test_converter_aquarium).
    if (r.ok && Run("xcrun --sdk macosx --find metal >/dev/null 2>&1")) {
        { FILE* f = fopen("/tmp/tglmt-fragout-probe.msl", "w");
          if (f) { fputs("#include <metal_stdlib>\nusing namespace metal;\nkernel void probe(){}\n", f); fclose(f); } }
        if (Run("xcrun -sdk macosx metal -c /tmp/tglmt-fragout-probe.msl -o /tmp/tglmt-fragout-probe.air >/dev/null 2>&1")) {
            FILE* f = fopen("/tmp/tglmt-fragout-center.msl", "w");
            if (f) { fputs(r.msl.c_str(), f); fclose(f); }
            CHECK(Run("xcrun -sdk macosx metal -c /tmp/tglmt-fragout-center.msl -o /tmp/tglmt-fragout-center.air >/dev/null 2>&1"),
                  "MSL centerDepth biên dịch được (metal -c)");
        } else {
            printf("SKIP: metal toolchain thiếu component\n");
        }
    }

    if (gFails) { printf("test_frag_out_scalar FAIL (%d)\n", gFails); return 1; }
    printf("test_frag_out_scalar PASS\n");
    return 0;
}
