// test_shader_tess.cpp — validate P-Tess T1: TCS → kernel MSL (MoltenVK pattern).
// Trên backend Metal thật: Convert OK + MSL compile thật bằng Metal.
// Trên backend Null (device isNull): chỉ check Convert OK (bỏ qua MSL compile).
// TES (0x8E87) vẫn fail trung thực (T2); GS không thuộc phạm vi file này.
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
        // --- negative: TES vẫn gate (T2) ---
        {"tes_gated_neg", TE,
         TCS("layout(triangles) in;\n"
             "void main(){ gl_Position = vec4(1.0); }"), false, -1},
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

    if (fail == 0) {
        printf("test_shader_tess: %zu cases OK\n", cases.size());
        return 0;
    }
    printf("test_shader_tess: %d FAIL\n", fail);
    return 1;
}
