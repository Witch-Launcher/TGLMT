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
//  8. NormalizeSwizzles (stpq→xyzw) không được đụng FIELD ACCESS: attribute
//     tên `p` (`vec4(p, 1)` → `_in.z` từng sai — test_shader_builtins basic_vs
//     MSL-FAIL), field struct tên `st`; swizzle thật (.st trên vec member)
//     vẫn phải đổi xyzw.
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

    // 8) Swizzle stpq→xyzw chỉ áp cho swizzle thật — field access giữ nguyên.
    {
        const char* s = "#version 460 core\n"
                        "struct S { vec2 st; };\n"
                        "uniform vec2 camPos;\n"
                        "layout(location = 0) in vec3 p;\n"
                        "void main() {\n"
                        "    S sv = S(camPos.st);\n"
                        "    vec2 uv = sv.st;\n"
                        "    gl_Position = vec4(p, 1) + vec4(uv.st, 0.0, 0.0);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B31);
        CHECK(r.ok, "swizzle/field convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "float4(_in.p, 1)"), "attribute `p` giữ nguyên (không thành swizzle)");
            CHECK(!Has(r.msl, "_in.z") && !Has(r.msl, "_in.xy"), "attribute p không bị NormalizeSwizzles");
            CHECK(Has(r.msl, "sv.st"), "field struct tên `st` giữ nguyên");
            CHECK(Has(r.msl, "camPos.xy"), "swizzle .st trên uniform member → xy");
            CHECK(Has(r.msl, "uv.xy"), "swizzle .st trên local vec2 → xy");
            MetalCheck("swizzlefield", r.msl, "MSL swizzle/field biên dịch được (metal -c)");
        }
    }

    // 9) Ctor floatN với arg int-family (implicit int→float GLSL) — ĐÚNG dạng
    // Iris sinh ra: `vec4(iris_UV2, 0.0, 1.0)` với `in ivec2 iris_UV2;`
    // (VanillaTransformer.java:101 / gl_MultiTexCoord1). MSL không có
    // float4(int2,float,float) → converter phải bọc float2(...). Gồm cả helper
    // (param `ivec2 iris_UV2` — ca thật: GetLightMapCoordinates của CR).
    {
        const char* s = "#version 460 core\n"
                        "layout(location = 0) in ivec2 iris_UV2;\n"
                        "layout(location = 1) in int mc_webId;\n"
                        "uniform mat4 iris_LightmapTextureMatrix;\n"
                        "out vec2 vLm;\n"
                        "vec2 helperLm(ivec2 iris_UV2) {\n"
                        "    return (iris_LightmapTextureMatrix * vec4(iris_UV2, 0.0, 1.0)).xy;\n"
                        "}\n"
                        "void main() {\n"
                        "    vec2 lm = helperLm(iris_UV2);\n"
                        "    vec4 big = vec4(mc_webId, iris_UV2.x, 0.0, 1.0);\n"
                        "    vLm = lm + big.xy;\n"
                        "    gl_Position = vec4(0.0);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B31);
        CHECK(r.ok, "int-arg float-ctor convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "float4(float2(iris_UV2), 0.0, 1.0)"),
                  "helper: vec4(ivec2,…) → float4(float2(iris_UV2),…)");
            CHECK(Has(r.msl, "float(_in.iris_UV2.x)"), "arg swizzle int → float(_in.iris_UV2.x)");
            CHECK(Has(r.msl, "float(_in.mc_webId)"), "arg int scalar → float(_in.mc_webId)");
            MetalCheck("intargctor", r.msl, "MSL arg int-family trong ctor biên dịch được");
        }
    }

    // 10) Ctor floatN với arg là UNIFORM/UBO-member int-family — dạng Iris
    // IE_COMPAT: `uniform ivec2 iris_LightUV;` + `vec4(iris_LightUV, 0.0, 1.0)`
    // (VanillaTransformer.java:92-96, nhánh parameters.inputs.isIE(); Iris load
    // mọi ShaderKey.values() nên pack nào cũng compile). Lúc rule chạy text đã
    // là `tglmt_u.iris_LightUV` / `ubo_<block>.<member>` → dim phải lấy từ
    // uniforms/blocks (trước đây `tglmt_u.` bị bỏ qua → game fail "no matching
    // constructor for 'float4'" @ IE_COMPAT/IE_COMPAT_SHADOW của CR).
    {
        const char* s = "#version 460 core\n"
                        "layout(std140) uniform TestBlock { ivec2 blkUV; };\n"
                        "uniform ivec2 iris_LightUV;\n"
                        "uniform mat4 iris_LightmapTextureMatrix;\n"
                        "layout(location = 0) in vec3 pos;\n"
                        "out vec2 vLm;\n"
                        "void main() {\n"
                        "    vLm = (iris_LightmapTextureMatrix * vec4(iris_LightUV, 0.0, 1.0)).xy;\n"
                        "    vLm += vec4(iris_LightUV.xy, 0.0, 1.0).xy;\n"
                        "    vLm += vec4(blkUV, 0.0, 1.0).xy;\n"
                        "    gl_Position = vec4(pos, 1.0);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B31);
        CHECK(r.ok, "int uniform/UBO arg trong float-ctor convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(Has(r.msl, "float4(float2(tglmt_u.iris_LightUV), 0.0, 1.0)"),
                  "uniform ivec2 → float4(float2(tglmt_u.iris_LightUV),…)");
            CHECK(Has(r.msl, "float4(float2(tglmt_u.iris_LightUV.xy), 0.0, 1.0)"),
                  "uniform ivec2 swizzle → float4(float2(tglmt_u.iris_LightUV.xy),…)");
            CHECK(Has(r.msl, "float4(float2(ubo_TestBlock.blkUV), 0.0, 1.0)"),
                  "UBO member ivec2 → float4(float2(ubo_TestBlock.blkUV),…)");
            MetalCheck("uniformargctor", r.msl, "MSL uniform/UBO arg int trong ctor biên dịch được");
        }
    }

    // 11) SHADOW_LINES: Iris isNewLines (VanillaTransformer.java:196-224) rename
    // main → irisMain + sinh iris_widen_lines/wrapper main; helper ghi
    // `gl_Position` TRỰC TIẾP (mpVary chỉ áp cho body) → MSL từng fail
    // "use of undeclared identifier 'gl_Position'" (game, build 105.56ed8c3).
    // Fix: rewrite gl_Position trong helper → `tglmt_position` + nhét vào
    // outTypes để tái dùng cơ chế "O:" (thread float4& / _out.).
    {
        const char* s = "#version 330 compatibility\n"
                        "layout(location = 0) in vec3 iris_Position;\n"
                        "layout(location = 1) in vec3 iris_Normal;\n"
                        "layout(location = 2) in float iris_LineWidth;\n"
                        "layout(location = 3) in vec2 iris_UV2;\n"
                        "layout(std140) uniform iris_Globals {\n"
                        "    ivec3 CameraBlockPos; vec3 CameraOffset; vec2 ScreenSize;\n"
                        "    float GlintAlpha; float GameTime; int MenuBlurRadius;\n"
                        "} iris_globalInfo;\n"
                        "uniform mat4 iris_ShadowMVP;\n"
                        "out vec2 lmCoord;\n"
                        "vec3 iris_vertex_offset = vec3(0.0);\n"
                        "void irisMain() {\n"
                        "    lmCoord = vec2(iris_UV2);\n"
                        "    gl_Position = iris_ShadowMVP * vec4(iris_Position + iris_vertex_offset, 1.0);\n"
                        "}\n"
                        "void iris_widen_lines(vec4 linePosStart, vec4 linePosEnd) {\n"
                        "    vec3 ndc1 = linePosStart.xyz / linePosStart.w;\n"
                        "    vec3 ndc2 = linePosEnd.xyz / linePosEnd.w;\n"
                        "    vec2 lineScreenDirection = normalize((ndc2.xy - ndc1.xy) * iris_globalInfo.ScreenSize);\n"
                        "    vec2 lineOffset = vec2(-lineScreenDirection.y, lineScreenDirection.x) * iris_LineWidth / iris_globalInfo.ScreenSize;\n"
                        "    if (lineOffset.x < 0.0) { lineOffset *= -1.0; }\n"
                        "    if (gl_VertexID % 2 == 0) {\n"
                        "        gl_Position = vec4((ndc1 + vec3(lineOffset, 0.0)) * linePosStart.w, linePosStart.w);\n"
                        "    } else {\n"
                        "        gl_Position = vec4((ndc1 - vec3(lineOffset, 0.0)) * linePosStart.w, linePosStart.w);\n"
                        "    }\n"
                        "}\n"
                        "void main() {\n"
                        "    iris_vertex_offset = iris_Normal;\n"
                        "    irisMain();\n"
                        "    vec4 linePosEnd = gl_Position;\n"
                        "    gl_Position = vec4(0.0);\n"
                        "    iris_vertex_offset = vec3(0.0);\n"
                        "    irisMain();\n"
                        "    vec4 linePosStart = gl_Position;\n"
                        "    iris_widen_lines(linePosStart, linePosEnd);\n"
                        "}\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(s, 0x8B31);
        CHECK(r.ok, "shadow-lines convert ok");
        if (!r.ok) printf("  log: %s\n", r.log.c_str());
        if (r.ok) {
            CHECK(!Has(r.msl, "gl_Position"), "không còn gl_Position trong MSL");
            CHECK(Has(r.msl, "thread float4& tglmt_position"),
                  "helper nhận thread float4& tglmt_position");
            size_t p1 = r.msl.find("thread float4& tglmt_position");
            CHECK(p1 != std::string::npos &&
                      r.msl.find("thread float4& tglmt_position", p1 + 1) != std::string::npos,
                  "cả irisMain lẫn iris_widen_lines đều nhận thread ref");
            CHECK(Has(r.msl, "_out.tglmt_position"), "call site body truyền _out.tglmt_position");
            MetalCheck("shadowlines", r.msl, "MSL gl_Position trong helper biên dịch được");
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
