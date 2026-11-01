// test_ubo_layout_binding.cpp — Unit: layout(binding=N) của UBO.
//
// Căn nguyên từ log máy thật (diag-90dceef): prog@10823 chỉ gọi
// glUniformBlockBinding cho idx=1(SamplerInfo)→point0 và idx=2(BlurConfig)→point1,
// idx=0(Globals) KHÔNG BAO GIỜ được gọi → binding=0 (sai) → đọc nhầm buffer
// SamplerInfo 16B trong khi struct Globals 56B → uboSmall zero fallback →
// ánh sáng/mây sai. GL thật: glUniformBlockBinding mặc định = layout(binding=N).
//
// Test: converter parse binding; link gán binding từ layout; không layout → 0;
// glUniformBlockBinding sau link vẫn override.
#include "tglmt/GLSLConverter.h"
#include "tglmt/Context.h"
#include "tglmt/gl46.h"

#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_ubo_layout_binding FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(std140, binding=4) uniform Globals { vec4 fog; vec4 light[4]; };\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec4 vFog;\n"
    "void main() { vFog = fog; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(std140) uniform SamplerInfo { vec4 info; };\n"
    "layout(location = 0) in vec4 vFog;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vFog * 0.0 + info * 0.0 + vec4(1.0); }\n";

int main() {
    // --- 1. Converter parse layout(binding=N) ---
    {
        const char* fs =
            "#version 460 core\n"
            "layout(std140, binding=2) uniform Globals { vec4 fog; };\n"
            "layout(std140, binding = 3) uniform BlurConfig { vec4 radius; };\n"
            "layout(std140) uniform Plain { vec4 p; };\n"
            "layout(location = 0) out vec4 c;\n"
            "void main(){ c = fog * 0.0 + radius * 0.0 + p * 0.0; }\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(fs, 0x8B30);
        if (!r.ok) printf("[conv log] %s\n", r.log.c_str());
        Check(r.ok, "convert ok");
        Check(r.blocks.size() == 3, "3 blocks");
        if (r.blocks.size() == 3) {
            Check(r.blocks[0].name == "Globals" && r.blocks[0].binding == 2,
                  "Globals binding=2");
            Check(r.blocks[1].name == "BlurConfig" && r.blocks[1].binding == 3,
                  "BlurConfig binding=3 (cách =)");
            Check(r.blocks[2].name == "Plain" && r.blocks[2].binding == -1,
                  "Plain không layout(binding) → -1");
        }
    }

    // --- 1b. std140 vec3 phải size 12 / align 16 (Voxy SceneUniform):
    // ivec3 @64 → frameId @76 (không phải 80), vec3 @80.
    {
        const char* fs =
            "#version 460 core\n"
            "layout(std140) uniform Scene { mat4 MVP; ivec3 baseSectionPos; uint frameId; vec3 cameraSubPos; float pad; vec3 extra[2]; };\n"
            "layout(location = 0) out vec4 c;\n"
            "void main(){ c = MVP * vec4(vec3(baseSectionPos) + cameraSubPos + extra[1], float(frameId)+pad); }\n";
        GLSLConvertResult r = ConvertGLSLtoMSL(fs, 0x8B30);
        if (!r.ok) printf("[conv log] %s\n", r.log.c_str());
        Check(r.ok, "SceneUniform convert ok");
        Check(r.blocks.size() == 1 && r.blocks[0].members.size() == 6, "SceneUniform members");
        if (r.ok && r.blocks.size() == 1 && r.blocks[0].members.size() == 6) {
            auto& m = r.blocks[0].members;
            Check(m[0].uniformOffset == 0 && m[0].uniformSize == 64, "MVP @0");
            Check(m[1].uniformOffset == 64 && m[1].uniformSize == 12, "baseSectionPos @64 size12");
            Check(m[2].uniformOffset == 76 && m[2].uniformSize == 4, "frameId @76");
            Check(m[3].uniformOffset == 80 && m[3].uniformSize == 12, "cameraSubPos @80 size12");
            Check(m[4].uniformOffset == 92 && m[4].uniformSize == 4, "pad @92");
            Check(m[5].uniformOffset == 96 && m[5].uniformSize == 32, "vec3[2] @96 stride16");
        }
        Check(r.msl.find("alignas(16) packed_int3 baseSectionPos") != std::string::npos,
              "ivec3 → packed_int3");
        Check(r.msl.find("alignas(16) packed_float3 cameraSubPos") != std::string::npos,
              "vec3 → packed_float3");
    }

    // --- 2. Link: binding lấy từ layout(binding=N); không layout → 0 ---
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    Check(ok != 0, "vs compile");
    glGetShaderiv(fs, 0x8B81, &ok);
    Check(ok != 0, "fs compile");
    if (gFails) return 1;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok != 0, "link");
    if (gFails) return 1;

    auto Find = [&](const char* name) -> ProgramObject::UniformBlock* {
        auto pit = ctx.programs.find(prog);
        if (pit == ctx.programs.end()) return nullptr;
        for (auto& b : pit->second.uniformBlocks)
            if (b.name == name) return &b;
        return nullptr;
    };
    auto* g = Find("Globals");
    auto* s = Find("SamplerInfo");
    Check(g != nullptr, "Globals block exists");
    Check(s != nullptr, "SamplerInfo block exists");
    if (g) {
        Check(g->layoutBinding == 4, "Globals layoutBinding=4");
        Check(g->binding == 4, "Globals binding=4 từ layout (không cần API call)");
    }
    if (s) {
        Check(s->layoutBinding == -1, "SamplerInfo không layout(binding)");
        Check(s->binding == 0, "SamplerInfo binding mặc định 0");
    }

    // --- 3. glUniformBlockBinding sau link override layout ---
    if (s && s->index != 0xFFFFFFFFu) {
        glUniformBlockBinding(prog, s->index, 7);
        Check(s->binding == 7, "glUniformBlockBinding override sau link");
    }

    // --- 4. relink: không mất binding app đã set (app vanilla set 1 lần) ---
    glLinkProgram(prog);
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok != 0, "relink");
    if (ok) {
        auto* s2 = Find("SamplerInfo");
        auto* g2 = Find("Globals");
        if (s2) Check(s2->binding == 7, "relink giữ binding đã set (không layout)");
        if (g2) Check(g2->binding == 4, "relink layout binding giữ 4");
    }

    if (gFails) return 1;
    printf("test_ubo_layout_binding PASS\n");
    return 0;
}
