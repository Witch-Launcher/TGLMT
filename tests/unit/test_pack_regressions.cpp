// test_pack_regressions.cpp — regression từ sweep Complementary Reimagined thật
// (tools/pack_conformance.py, 72/73 program × 3 world):
//  1. fragment 0 out (chỉ discard) — gbuffers_clouds.fsh, trước fix converter
//     "fragment cần 1..8 out (hiện có 0)" + segfault outs[0].
//  2. `writeonly uniform image3D` (qualifier đứng TRƯỚC uniform) — shadowcomp.glsl.
//  3. `#version 430 compatibility` — world0/shadowcomp.csh.
//  4. Hoist global mutable + helper overload cùng tên: args thread bị chèn 2 lần
//     vào call site (mỗi def 1 lượt) → "helper GetLinearDepth sai arity (co 3)"
//     của deferred1.fsh.
// CPU-only: convert + assert MSL; metal -c probe nếu toolchain sẵn.
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

    // 1) Void fragment (nguyên văn dạng gbuffers_clouds.fsh sau preprocess).
    {
        const char* s = "#version 130\n"
                        "void main() {\n"
                        "    discard;\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "void fragment convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) CHECK(Has(r.msl, "fragment void TGLMT_fs"), "emit fragment void TGLMT_fs");
    }

    // 2) image3D qualifier trước `uniform` (shadowcomp).
    {
        const char* s = "#version 430\n"
                        "layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;\n"
                        "writeonly uniform image3D floodfill_img;\n"
                        "void main() {\n"
                        "    ivec3 p = ivec3(gl_GlobalInvocationID);\n"
                        "    imageStore(floodfill_img, p, vec4(1.0));\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x91B9);
        CHECK(r.ok, "writeonly uniform image3D convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "texture3d<float, access::write>"), "image3D → texture3d write-only");
            CHECK(Has(r.msl, "floodfill_img.write("), "imageStore rewrite");
        }
    }

    // 3) #version 430 compatibility (world0/shadowcomp.csh).
    {
        const char* s = "#version 430 compatibility\n"
                        "out vec4 c;\n"
                        "void main() { c = vec4(1.0); }\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "#version 430 compatibility convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
    }

    // 4) Hoist global + overload cùng tên: thread arg chỉ được chèn 1 lần/call site.
    {
        const char* s = "#version 130\n"
                        "float gvar = 1.0;\n"
                        "float f(float x) { return x + gvar; }\n"
                        "float f(float x, float y) { return x + y + gvar; }\n"
                        "void main() {\n"
                        "    gl_FragColor = vec4(f(1.0) + f(1.0, 2.0));\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "hoist + overload cùng tên convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "f(1.0, gvar)"), "call f/1 nhận gvar đúng 1 lần");
            CHECK(Has(r.msl, "f(1.0, 2.0, gvar)"), "call f/2 nhận gvar đúng 1 lần");
            CHECK(!Has(r.msl, "gvar, gvar"), "không chèn gvar lặp");
            CHECK(Count(r.msl, "thread float &gvar") == 2, "cả 2 def đều thread gvar");
        }
    }

    // metal -c probe (toolchain trên máy này không đáng tin — chỉ chạy khi probe pass).
    if (Run("xcrun --sdk macosx --find metal >/dev/null 2>&1")) {
        { FILE* f = fopen("/tmp/tglmt-packreg-probe.msl", "w");
          if (f) { fputs("#include <metal_stdlib>\nusing namespace metal;\nkernel void probe(){}\n", f); fclose(f); } }
        if (Run("xcrun -sdk macosx metal -c /tmp/tglmt-packreg-probe.msl -o /tmp/tglmt-packreg-probe.air >/dev/null 2>&1")) {
            const char* s = "#version 130\n"
                            "float gvar = 1.0;\n"
                            "float f(float x) { return x + gvar; }\n"
                            "float f(float x, float y) { return x + y + gvar; }\n"
                            "void main() {\n"
                            "    gl_FragColor = vec4(f(1.0) + f(1.0, 2.0));\n"
                            "}\n";
            GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
            if (r.ok) {
                FILE* f = fopen("/tmp/tglmt-packreg-hoist.msl", "w");
                if (f) { fputs(r.msl.c_str(), f); fclose(f); }
                CHECK(Run("xcrun -sdk macosx metal -c /tmp/tglmt-packreg-hoist.msl -o /tmp/tglmt-packreg-hoist.air >/dev/null 2>&1"),
                      "MSL hoist+overload biên dịch được (metal -c)");
            }
        } else {
            printf("SKIP: metal toolchain thiếu component\n");
        }
    }

    if (gFails) { printf("test_pack_regressions FAIL (%d)\n", gFails); return 1; }
    printf("test_pack_regressions PASS\n");
    return 0;
}
