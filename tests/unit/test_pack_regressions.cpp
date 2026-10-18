// test_pack_regressions.cpp — regression từ sweep Complementary Reimagined thật
// (tools/pack_conformance.py, 60 PASS/0 FAIL × world0/world1/world-1):
//  1. fragment 0 out (chỉ discard) — gbuffers_clouds.fsh, trước fix converter
//     "fragment cần 1..8 out (hiện có 0)" + segfault outs[0].
//  2. `writeonly uniform image3D` (qualifier đứng TRƯỚC uniform) — shadowcomp.glsl.
//  3. `#version 430 compatibility` — world0/shadowcomp.csh.
//  4. Hoist global mutable + helper overload cùng tên: args thread bị chèn 2 lần
//     vào call site (mỗi def 1 lượt) → "helper GetLinearDepth sai arity (co 3)"
//     của deferred1.fsh.
//  5. Hoisted global đụng tên local CÙNG SCOPE main — composite.fsh `ivec2
//     texelCoord` (common.glsl) + local đầu main: MSL redefinition. Fix: rename
//     hoisted thành texelCoord_tglmt_g + call-arg thread theo tên mới.
//  6. Local trong block LỒNG NHAU shadow hoisted global là hợp lệ ở MSL (END
//     gbuffers_water `float2 wind`): KHÔNG được rename (từng làm call-arg ngoài
//     block "undeclared identifier 'wind'").
//  7. mix(x,y,lessThan(a,b)) → select (ExpandRelationalBuiltins chạy SAU
//     PromoteBoolVecMix — phải chạy lại); min(1, v.z) int-lit → float(1)
//     (MSL ambiguous); if (v == helper_vec(...)) → all() (HelperVecReturns).
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
// metal -c: extension phải `.metal` (`.msl` → exit 0 nhưng không tạo .air —
// probe từng vacuous vì lỗi này).
static bool MetalAvail() {
    static int cached = -1;
    if (cached < 0) {
        FILE* f = fopen("/tmp/tglmt-packreg-probe.metal", "w");
        if (f) { fputs("#include <metal_stdlib>\nusing namespace metal;\nkernel void probe(){}\n", f); fclose(f); }
        cached = Run("xcrun --sdk macosx --find metal >/dev/null 2>&1") &&
                 Run("xcrun -sdk macosx metal -c /tmp/tglmt-packreg-probe.metal -o /tmp/tglmt-packreg-probe.air >/dev/null 2>&1")
                     ? 1 : 0;
    }
    return cached == 1;
}
static void MetalCheck(const std::string& tag, const std::string& msl, const char* msg) {
    if (!MetalAvail()) return;
    std::string p = "/tmp/tglmt-packreg-" + tag + ".metal";
    std::string a = "/tmp/tglmt-packreg-" + tag + ".air";
    FILE* f = fopen(p.c_str(), "w");
    if (!f) return;
    fputs(msl.c_str(), f);
    fclose(f);
    CHECK(Run("xcrun -sdk macosx metal -c " + p + " -o " + a + " >/dev/null 2>&1"), msg);
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

    // 5) Hoisted global đụng tên local CÙNG SCOPE main → rename `_tglmt_g`.
    {
        const char* s = "#version 330\n"
                        "uniform vec2 view;\n"
                        "out vec4 c;\n"
                        "ivec2 texelCoord = ivec2(gl_FragCoord.xy);\n"
                        "void main() {\n"
                        "    ivec2 texelCoord = ivec2(view);\n"
                        "    c = vec4(vec2(texelCoord), 0.0, 1.0);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "hoisted global trùng tên local top-level convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "texelCoord_tglmt_g"), "hoisted global đổi tên _tglmt_g");
            CHECK(Has(r.msl, "int2 texelCoord = int2"), "local main giữ nguyên tên");
            MetalCheck("hoistcoll", r.msl, "MSL hoist-collision biên dịch được (metal -c)");
        }
    }

    // 6) Nested local shadow hoisted global → KHÔNG rename (shadow hợp lệ ở MSL).
    {
        const char* s = "#version 130\n"
                        "uniform vec2 view;\n"
                        "vec2 gwind = vec2(0.5);\n"
                        "float sum2(vec2 p) { return p.x + p.y + gwind.x; }\n"
                        "void main() {\n"
                        "    vec4 color = vec4(sum2(gwind));\n"
                        "    if (view.y > 0.0) {\n"
                        "        vec2 gwind = vec2(1.0);\n"
                        "        color += vec4(gwind, 0.0, 1.0);\n"
                        "    }\n"
                        "    color += vec4(sum2(gwind));\n"
                        "    gl_FragColor = color;\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "nested local shadow hoisted global convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(!Has(r.msl, "gwind_tglmt_g"), "nested shadow không rename");
            CHECK(Has(r.msl, "thread float2 &gwind"), "helper vẫn được thread global");
            MetalCheck("nestedshadow", r.msl, "MSL nested-shadow biên dịch được (metal -c)");
        }
    }

    // 7) mix(...,lessThan) → select; min(1, v.z) → float(1); if (v == helper) → all().
    {
        const char* s = "#version 130\n"
                        "vec3 lift(vec3 x) { return x * 2.0; }\n"
                        "void main() {\n"
                        "    vec3 color = vec3(0.5);\n"
                        "    color = mix(vec3(1.0) - color, 12.92 * color, lessThan(color, vec3(0.0031308)));\n"
                        "    float m = min(1, color.z);\n"
                        "    if (color == lift(color)) {\n"
                        "        color = vec3(m);\n"
                        "    }\n"
                        "    gl_FragColor = vec4(color, 1.0);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        CHECK(r.ok, "select/min/vec-cmp convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "select("), "mix bvec → select");
            CHECK(Has(r.msl, "min(float(1), color.z)"), "int-lit min → float(1)");
            CHECK(Has(r.msl, "all(color == lift(color))"), "vec == helper-vec → all()");
            MetalCheck("selectmincmp", r.msl, "MSL select/min/vec-cmp biên dịch được (metal -c)");
        }
    }

    // metal -c cho case 4 (toolchain trên máy này không đáng tin — chỉ chạy khi probe pass).
    if (MetalAvail()) {
        const char* s = "#version 130\n"
                        "float gvar = 1.0;\n"
                        "float f(float x) { return x + gvar; }\n"
                        "float f(float x, float y) { return x + y + gvar; }\n"
                        "void main() {\n"
                        "    gl_FragColor = vec4(f(1.0) + f(1.0, 2.0));\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B30);
        if (r.ok) MetalCheck("hoist", r.msl, "MSL hoist+overload biên dịch được (metal -c)");
    } else {
        printf("SKIP: metal toolchain thiếu component\n");
    }

    if (gFails) { printf("test_pack_regressions FAIL (%d)\n", gFails); return 1; }
    printf("test_pack_regressions PASS\n");
    return 0;
}
