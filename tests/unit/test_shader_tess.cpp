// test_shader_tess.cpp — validate P-Tess T1: TCS → kernel MSL (MoltenVK pattern).
// Trên backend Metal thật: Convert OK + MSL compile thật bằng Metal.
// Trên backend Null (device isNull): chỉ check Convert OK (bỏ qua MSL compile).
// TES (0x8E87): T2 triangles → post-tessellation vertex MSL; GS ngoài phạm vi.
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt;

namespace {
std::shared_ptr<metal::IDevice> MakeDev() {
    auto d = metal::CreateDevice("apple", nullptr);
    if (!d || d->isNull()) return nullptr;
    return d;
}
#define TCS(x) "#version 460 core\n" x
} // namespace

int main() {
    auto dev = MakeDev();
    const uint32_t TC = 0x8E88, TE = 0x8E87;
    struct Case {
        const char* name;
        uint32_t stage;
        const char* glsl;
        bool expectOk;
        int expectVertices; // -1 = không check
    };
    std::vector<Case> cases = {
        // --- TCS core: passthrough same-name in/out (pattern Iris) + TessLevel tam giác ---
        {"tcs_min", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID] + vec4(float(gl_PrimitiveID));\n"
             "  gl_TessLevelOuter[0] = 4.0;\n"
             "  gl_TessLevelOuter[1] = 4.0;\n"
             "  gl_TessLevelOuter[2] = 4.0;\n"
             "  gl_TessLevelInner[0] = 4.0;\n"
             "}"), true, 3},
        // --- uniform + barrier + conditional TessLevel (invocation 0) ---
        {"tcs_uniform_barrier", TC,
         TCS("layout(vertices = 3) out;\n"
             "uniform float tessF;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  barrier();\n"
             "  if (gl_InvocationID == 0) {\n"
             "    gl_TessLevelOuter[0] = tessF;\n"
             "    gl_TessLevelOuter[1] = tessF;\n"
             "    gl_TessLevelOuter[2] = tessF;\n"
             "    gl_TessLevelInner[0] = tessF;\n"
             "  }\n"
             "}"), true, 3},
        // --- gl_in.gl_Position theo khoảng cách (pattern Iris distance-based tess) ---
        {"tcs_glin", TC,
         TCS("layout(vertices = 3) out;\n"
             "uniform vec3 camPos;\n"
             "in vec2 vUV[];\n"
             "out vec2 vUV[];\n"
             "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
             "void main(){\n"
             "  vUV[gl_InvocationID] = vUV[gl_InvocationID];\n"
             "  vec4 p0 = gl_in[0].gl_Position;\n"
             "  float d = distance(camPos, p0.xyz);\n"
             "  float lv = clamp(16.0 / d, 1.0, 64.0);\n"
             "  gl_TessLevelOuter[0] = lv;\n"
             "  gl_TessLevelOuter[1] = lv;\n"
             "  gl_TessLevelOuter[2] = lv;\n"
             "  gl_TessLevelInner[0] = lv;\n"
             "}"), true, 3},
        // --- patch out scalar ---
        {"tcs_patch", TC,
         TCS("layout(vertices = 3) out;\n"
             "patch out vec4 patchCol;\n"
             "in vec4 vC[];\n"
             "out vec4 vC[];\n"
             "void main(){\n"
             "  vC[gl_InvocationID] = vC[gl_InvocationID];\n"
             "  patchCol = vec4(1.0);\n"
             "  gl_TessLevelOuter[0] = 2.0;\n"
             "  gl_TessLevelOuter[1] = 2.0;\n"
             "  gl_TessLevelOuter[2] = 2.0;\n"
             "  gl_TessLevelInner[0] = 2.0;\n"
             "}"), true, 3},
        // --- helper nhận phần tử (scalar) — HỢP LỆ ---
        {"tcs_helper_elem", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "vec4 dbl(vec4 x) { return x * 2.0; }\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = dbl(vPos[gl_InvocationID]);\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), true, 3},
        // --- sampler trong TCS (lock texParams path) ---
        {"tcs_sampler", TC,
         TCS("layout(vertices = 3) out;\n"
             "uniform sampler2D t;\n"
             "in vec2 vUV[];\n"
             "out vec2 vUV[];\n"
             "void main(){\n"
             "  vUV[gl_InvocationID] = vUV[gl_InvocationID] + texelFetch(t, ivec2(0), 0).rg;\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), true, 3},
        // --- UBO trong TCS (lock blockParams path) ---
        {"tcs_ubo", TC,
         TCS("layout(vertices = 3) out;\n"
             "layout(std140, binding = 0) uniform Params { float tessF; };\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelOuter[0] = tessF;\n"
             "  gl_TessLevelOuter[1] = tessF;\n"
             "  gl_TessLevelOuter[2] = tessF;\n"
             "  gl_TessLevelInner[0] = tessF;\n"
             "}"), true, 3},
        // --- SSBO trong TCS (lock slot 0+ path) ---
        {"tcs_ssbo", TC,
         TCS("layout(vertices = 3) out;\n"
             "layout(std430, binding = 0) buffer B { float v[]; };\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  v[0] = 1.0;\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), true, 3},
        // --- gl_PatchVerticesIn → literal ---
        {"tcs_patchvertices", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  int n = gl_PatchVerticesIn;\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID] + vec4(float(n));\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), true, 3},
        // --- không ghi TessLevel (vẫn compile — factor param không emit) ---
        {"tcs_no_factors", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){ vPos[gl_InvocationID] = vPos[gl_InvocationID]; }"), true, 3},
        // --- negative: thiếu layout(vertices) ---
        {"tcs_no_layout_neg", TC,
         TCS("in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), false, -1},
        // --- negative: vertices = 0 ---
        {"tcs_bad_vertices_neg", TC,
         TCS("layout(vertices = 0) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){ vPos[gl_InvocationID] = vPos[gl_InvocationID]; }"), false, -1},
        // --- negative: Outer[3] quad-only ---
        {"tcs_quad_outer_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelOuter[3] = 2.0;\n"
             "}"), false, -1},
        // --- negative: Inner[1] quad-only ---
        {"tcs_quad_inner_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelInner[1] = 2.0;\n"
             "}"), false, -1},
        // --- negative: gl_Position trong TCS ---
        {"tcs_gl_position_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  gl_Position = vec4(1.0);\n"
             "}"), false, -1},
        // --- negative: gl_TessCoord là TES-only ---
        {"tcs_tesscoord_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  float t = gl_TessCoord.x;\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID] + vec4(t);\n"
             "}"), false, -1},
        // --- negative: in scalar (TCS bắt mảng) ---
        {"tcs_scalar_in_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 x;\n"
             "out vec4 y[];\n"
             "void main(){ y[gl_InvocationID] = x; }"), false, -1},
        // --- negative: patch in ở TCS ---
        {"tcs_patch_in_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "patch in vec4 x;\n"
             "void main(){ }"), false, -1},
        // --- negative: sized mismatch layout ---
        {"tcs_size_mismatch_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 y[5];\n"
             "void main(){ y[gl_InvocationID] = vPos[gl_InvocationID]; }"), false, -1},
        // --- negative: index động vào TessLevel ---
        {"tcs_dynamic_index_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelOuter[gl_InvocationID] = 2.0;\n"
             "}"), false, -1},
        // --- negative: truyền cả mảng vào helper ---
        {"tcs_helper_array_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "vec4 avg(vec4 a[3]) { return (a[0] + a[1] + a[2]) / 3.0; }\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = avg(vPos);\n"
             "  gl_TessLevelOuter[0] = 1.0;\n"
             "  gl_TessLevelOuter[1] = 1.0;\n"
             "  gl_TessLevelOuter[2] = 1.0;\n"
             "  gl_TessLevelInner[0] = 1.0;\n"
             "}"), false, -1},
        // --- negative: gl_in member lạ ---
        {"tcs_glin_member_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
             "void main(){\n"
             "  vec4 p = gl_in[0].gl_ClipDistance;\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID] + p;\n"
             "}"), false, -1},
        // --- negative: gl_in không khai báo ---
        {"tcs_glin_no_block_neg", TC,
         TCS("layout(vertices = 3) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vec4 p = gl_in[0].gl_Position;\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID] + p;\n"
             "}"), false, -1},
        // --- T2: TES triangle tối thiểu ---
        {"tes_min", TE,
         TCS("layout(triangles) in;\n"
             "void main(){ gl_Position = vec4(gl_TessCoord, 1.0); }"), true, 3},
        // --- T2: per-vertex in nội suy + patch in + gl_in ---
        {"tes_interp_patch_glin", TE,
         TCS("layout(triangles) in;\n"
             "in vec3 vColor[];\n"
             "patch in float pScale;\n"
             "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
             "out vec3 vColor;\n"
             "void main(){\n"
             "  float s = pScale + float(gl_PrimitiveID) + gl_in.length();\n"
             "  vColor = vColor[0] * s;\n"
             "  gl_Position = gl_in[0].gl_Position + vec4(gl_TessCoord, 0.0);\n"
             "}"), true, 3},
        // --- T2: TES quad (P-Tess T3) ---
        {"tes_quad", TE,
         TCS("layout(quads) in;\n"
             "void main(){ gl_Position = vec4(gl_TessCoord.xy, 0.0, 1.0); }"), true, -1},
        // --- T2: TES đọc gl_TessLevelOuter/Inner (quad) ---
        {"tes_levels_quad", TE,
         TCS("layout(quads) in;\n"
             "void main(){\n"
             "  float e = gl_TessLevelOuter[0] + gl_TessLevelInner[1];\n"
             "  gl_Position = vec4(gl_TessCoord.xy, e, 1.0);\n"
             "}"), true, -1},
        // --- negative: TES isolines vẫn ngoài T2 ---
        {"tes_isolines_neg", TE,
         TCS("layout(isolines) in;\n"
             "void main(){ gl_Position = vec4(1.0); }"), false, -1},
        // --- TES clip_distance[0]: Metal native [[clip_distance]] ---
        {"tes_clip0", TE,
         TCS("layout(triangles) in;\n"
             "void main(){\n"
             "  gl_ClipDistance[0] = gl_TessCoord.x;\n"
             "  gl_Position = vec4(gl_TessCoord, 1.0);\n"
             "}"), true, 3},
        // Metal chỉ có 1 clip distance; index khác phải fail rõ.
        {"tes_clip1_neg", TE,
         TCS("layout(triangles) in;\n"
             "void main(){\n"
             "  gl_ClipDistance[1] = gl_TessCoord.x;\n"
             "  gl_Position = vec4(gl_TessCoord, 1.0);\n"
             "}"), false, -1},
        // CullDistance không có tương đương Metal.
        {"tes_culldist_neg", TE,
         TCS("layout(triangles) in;\n"
             "void main(){\n"
             "  gl_CullDistance[0] = gl_TessCoord.x;\n"
             "  gl_Position = vec4(gl_TessCoord, 1.0);\n"
             "}"), false, -1},
        // --- TCS: quad TessLevel (vertices=4) ---
        {"tcs_quad", TC,
         TCS("layout(vertices = 4) out;\n"
             "in vec4 vPos[];\n"
             "out vec4 vPos[];\n"
             "void main(){\n"
             "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
             "  gl_TessLevelOuter[3] = 2.0;\n"
             "  gl_TessLevelInner[1] = 2.0;\n"
             "}"), true, 4},
    };

    int fail = 0;
    for (auto& cs : cases) {
        auto r = ConvertGLSLtoMSL(cs.glsl, cs.stage);
        if (!r.ok) {
            if (cs.expectOk) { printf("CONV-FAIL %-24s %s\n", cs.name, r.log.c_str()); ++fail; }
            continue;
        }
        if (!cs.expectOk) {
            printf("CONV-UNEXPECTED-OK %-24s (đáng lẽ fail)\n", cs.name);
            ++fail;
            continue;
        }
        if (cs.stage == TE) {
            const bool quad = std::string(cs.name).find("quad") != std::string::npos;
            const int wantPatch = quad ? 4 : 3;
            if (r.tessPatchType != wantPatch) {
                printf("PATCH-FAIL %-24s tessPatchType=%d expect %d\n",
                       cs.name, r.tessPatchType, wantPatch);
                ++fail; continue;
            }
            if (r.entryPoint != "TGLMT_tes") {
                printf("ENTRY-FAIL %-24s entry=%s\n", cs.name, r.entryPoint.c_str());
                ++fail; continue;
            }
        }
        if (cs.stage == TC) {
            if (r.tessVertices != cs.expectVertices) {
                printf("VERT-FAIL %-24s tessVertices=%d expect %d\n", cs.name, r.tessVertices, cs.expectVertices);
                ++fail; continue;
            }
            if (r.entryPoint != "TGLMT_tcs") {
                printf("ENTRY-FAIL %-24s entry=%s\n", cs.name, r.entryPoint.c_str());
                ++fail; continue;
            }
            bool usesFactors = (std::string(cs.glsl).find("gl_TessLevel") != std::string::npos);
            if (usesFactors && r.tessFactorSlot < 0) {
                printf("FACTOR-FAIL %-24s dùng TessLevel nhưng không có factor slot\n", cs.name);
                ++fail; continue;
            }
            if (!usesFactors && r.tessFactorSlot >= 0) {
                printf("FACTOR-FAIL %-24s không dùng TessLevel nhưng có factor slot\n", cs.name);
                ++fail; continue;
            }
        }
        if (dev) {
            std::string err;
            auto lib = dev->compileLibrary(r.msl, err);
            if (!lib) {
                printf("MSL-FAIL  %-24s %s\n", cs.name, err.c_str());
                printf("  --- MSL ---\n%.800s\n", r.msl.c_str());
                ++fail;
            }
        }
    }

    // T4b: VS capture compute — attribute fetch + gl_in/VS varying output buffers.
    {
        const char* vs =
            "#version 460 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 1) in vec2 aUV;\n"
            "out vec3 vColor;\n"
            "void main() {\n"
            "  if (aPos.x > 1000.0) { vColor = vec3(0.0); return; }\n"
            "  vColor = vec3(aUV, 1.0);\n"
            "  gl_Position = vec4(aPos, 1.0);\n"
            "}\n";
        auto r = ConvertGLSLtoMSL(vs, 0x8B31, true);
        auto capFail = [&](const char* why) {
            printf("CAPTURE-FAIL %s\n", why);
            ++fail;
        };
        if (!r.ok) { printf("CAPTURE-FAIL convert: %s\n", r.log.c_str()); ++fail; }
        else {
            if (r.entryPoint != "TGLMT_vs_capture") capFail("entryPoint");
            if (r.msl.find("kernel void TGLMT_vs_capture") == std::string::npos) capFail("kernel missing");
            if (r.msl.find("_in.aPos") == std::string::npos) capFail("attribute load missing");
            if (r.captureIndexSlot != 28 || r.captureDrawSlot != 29 || r.captureDescSlot != 30)
                capFail("capture aux slots");
            int vColorSlot = -1, posSlot = -1, psSlot = -1;
            for (auto& c : r.captureSlots) {
                if (c.name == "vColor" && !c.isGlIn) vColorSlot = c.slot;
                if (c.name == "gl_Position") posSlot = c.slot;
                if (c.name == "gl_PointSize") psSlot = c.slot;
            }
            if (vColorSlot < 17 || posSlot != vColorSlot + 1 || psSlot != posSlot + 1)
                capFail("capture varying/builtin slots");
            if (dev) {
                std::string err;
                auto lib = dev->compileLibrary(r.msl, err);
                if (!lib) {
                    printf("CAPTURE-FAIL MSL: %s\n--- MSL ---\n%.1200s\n", err.c_str(), r.msl.c_str());
                    ++fail;
                }
            }
        }
    }

    // Link-time domain hint: triangle TES + TCS layout(vertices=5) phải dùng
    // factor struct tam giác, và gl_in.length() của TCS là input CP runtime.
    {
        const char* tcs =
            "#version 460 core\n"
            "layout(vertices = 5) out;\n"
            "in vec2 vIn[];\n"
            "out vec2 vOut[];\n"
            "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
            "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
            "void main(){\n"
            "  int n = gl_in.length();\n"
            "  vOut[gl_InvocationID] = vIn[gl_InvocationID] + vec2(float(n));\n"
            "  gl_out[gl_InvocationID].gl_Position = vec4(vOut[gl_InvocationID], 0.0, 1.0);\n"
            "  gl_TessLevelOuter[0] = 1.0;\n"
            "}\n";
        auto r = ConvertGLSLtoMSL(tcs, 0x8E88, false, 3);
        if (!r.ok) {
            printf("TCS-DOMAIN-FAIL convert: %s\n", r.log.c_str());
            ++fail;
        } else {
            if (r.tessPatchType != 3) { printf("TCS-DOMAIN-FAIL patchType=%d\n", r.tessPatchType); ++fail; }
            if (r.msl.find("MTLTriangleTessellationFactorsHalf") == std::string::npos) {
                printf("TCS-DOMAIN-FAIL thiếu triangle factor struct\n"); ++fail;
            }
            if (r.msl.find("MTLQuadTessellationFactorsHalf") != std::string::npos) {
                printf("TCS-DOMAIN-FAIL lẫn quad factor struct\n"); ++fail;
            }
            if (r.msl.find("int(tglmt_patchInputCount)") == std::string::npos) {
                printf("TCS-DOMAIN-FAIL gl_in.length không dùng input count\n"); ++fail;
            }
            if (dev) {
                std::string err;
                auto lib = dev->compileLibrary(r.msl, err);
                if (!lib) { printf("TCS-DOMAIN-FAIL MSL: %s\n", err.c_str()); ++fail; }
            }
        }
    }

    // Tessellation spacing/vertex order: phải parse thật, không được bỏ qua
    // âm thầm rồi luôn dùng partition Integer.
    {
        const char* teEven =
            "#version 460 core\n"
            "layout(triangles, fractional_even, ccw) in;\n"
            "in vec2 vPos[];\n"
            "void main(){\n"
            "  vec2 p = vPos[0]*gl_TessCoord.x + vPos[1]*gl_TessCoord.y + vPos[2]*gl_TessCoord.z;\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        auto re = ConvertGLSLtoMSL(teEven, 0x8E87);
        if (!re.ok) { printf("SPACING-FAIL fractional_even convert: %s\n", re.log.c_str()); ++fail; }
        else if (re.tessSpacing != 1) { printf("SPACING-FAIL fractional_even=%d\n", re.tessSpacing); ++fail; }
        const char* teOdd =
            "#version 460 core\n"
            "layout(triangles, fractional_odd) in;\n"
            "in vec2 vPos[];\n"
            "void main(){\n"
            "  vec2 p = vPos[0]*gl_TessCoord.x + vPos[1]*gl_TessCoord.y + vPos[2]*gl_TessCoord.z;\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        auto ro = ConvertGLSLtoMSL(teOdd, 0x8E87);
        if (!ro.ok) { printf("SPACING-FAIL fractional_odd convert: %s\n", ro.log.c_str()); ++fail; }
        else if (ro.tessSpacing != 2) { printf("SPACING-FAIL fractional_odd=%d\n", ro.tessSpacing); ++fail; }
        const char* teCw =
            "#version 460 core\n"
            "layout(triangles, equal_spacing, cw) in;\n"
            "in vec2 vPos[];\n"
            "void main(){\n"
            "  vec2 p = vPos[0]*gl_TessCoord.x + vPos[1]*gl_TessCoord.y + vPos[2]*gl_TessCoord.z;\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        auto rc = ConvertGLSLtoMSL(teCw, 0x8E87);
        if (!rc.ok) { printf("SPACING-FAIL cw convert: %s\n", rc.log.c_str()); ++fail; }
        else if (rc.tessVertexOrder != 1) { printf("SPACING-FAIL cw order=%d\n", rc.tessVertexOrder); ++fail; }
        const char* tePoint =
            "#version 460 core\n"
            "layout(triangles, point_mode) in;\n"
            "in vec2 vPos[];\n"
            "void main(){\n"
            "  vec2 p = vPos[0]*gl_TessCoord.x + vPos[1]*gl_TessCoord.y + vPos[2]*gl_TessCoord.z;\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        auto rp = ConvertGLSLtoMSL(tePoint, 0x8E87);
        if (rp.ok) { printf("SPACING-FAIL point_mode bị bỏ qua\n"); ++fail; }
    }

    if (fail == 0) {
        printf("test_shader_tess: %zu cases + capture OK\n", cases.size());
        return 0;
    }
    printf("test_shader_tess: %d FAIL\n", fail);
    return 1;
}
