// test_frag_out_array.cpp — fragment `out` ARRAY (Blaze3D OIT transmittance:
// `layout(location = 0) out vec4 _frag_output_00[2];` → 2 attachment 0/1).
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

    // 1) nguyên văn decl MC 26.3 + body ghi index thường (0/1)
    {
        const char* s = R"(#version 150 core
layout(location = 0) out vec4 _frag_output_00[2];
uniform sampler2D tex;
void main() {
    vec4 c = texture(tex, vec2(0.5));
    _frag_output_00[0] = c * vec4(1.0, 0.8, 0.6, 1.0);
    _frag_output_00[1] = vec4(c.a, c.a, c.a, 1.0);
}
)";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "fragment out array (decl MC) convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "float4 _frag_output_00[2] = {"), "local array zero-init");
            CHECK(Has(r.msl, "_out.mrt0 = _frag_output_00[0];"), "copy elem0 → mrt0");
            CHECK(Has(r.msl, "_out.mrt1 = _frag_output_00[1];"), "copy elem1 → mrt1");
            CHECK(Has(r.msl, "float4 mrt0 [[color(0)]]"), "FOut field mrt0");
            CHECK(Has(r.msl, "float4 mrt1 [[color(1)]]"), "FOut field mrt1");
        }
    }

    // 2) index động trong loop vẫn được (body dùng int i)
    {
        const char* s = R"(#version 150 core
layout(location = 0) out vec4 outputs_0[2];
uniform vec4 u;
void main() {
    for (int i = 0; i < 2; ++i) {
        outputs_0[i] = u * float(i + 1);
    }
}
)";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "fragment out array (loop) convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "_out.mrt0 = outputs_0[0];"), "loop elem0 copy");
            CHECK(Has(r.msl, "_out.mrt1 = outputs_0[1];"), "loop elem1 copy");
        }
    }

    // 3) in array ở FS vẫn phải fail (không nới lỏng too wide)
    {
        const char* s = R"(#version 150 core
in vec4 ins_0[2];
out vec4 outS;
void main() { outS = ins_0[0]; }
)";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(!r.ok, "fragment in array vẫn fail (narrow pattern)");
    }

    // 4) toolchain: biên dịch thật nếu có metal -c
    if (Run("xcrun --sdk macosx --find metal >/dev/null 2>&1")) {
        { FILE* f = fopen("/tmp/tglmt-fragoutarr-probe.msl", "w");
          if (f) { fputs("#include <metal_stdlib>\nusing namespace metal;\nkernel void probe(){}\n", f); fclose(f); } }
        if (Run("xcrun -sdk macosx metal -c /tmp/tglmt-fragoutarr-probe.msl -o /tmp/tglmt-fragoutarr-probe.air >/dev/null 2>&1")) {
            const char* s = R"(#version 150 core
layout(location = 0) out vec4 _frag_output_00[2];
void main() {
    _frag_output_00[0] = vec4(1.0);
    _frag_output_00[1] = vec4(0.0, 1.0, 0.0, 1.0);
}
)";
            GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
            if (r.ok) {
                FILE* f = fopen("/tmp/tglmt-fragoutarr.msl", "w");
                if (f) { fputs(r.msl.c_str(), f); fclose(f); }
                CHECK(Run("xcrun -sdk macosx metal -c /tmp/tglmt-fragoutarr.msl -o /tmp/tglmt-fragoutarr.air >/dev/null 2>&1"),
                      "MSL fragment out array biên dịch được (metal -c)");
            }
        }
    }

    if (gFails) { printf("test_frag_out_array FAIL (%d)\n", gFails); return 1; }
    printf("test_frag_out_array PASS\n");
    return 0;
}
