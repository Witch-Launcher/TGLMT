// test_tess_instance_id_apple.cpp — gl_InstanceID + gl_PrimitiveID trong TCS/TES.
// 2 instance, mỗi instance 1 patch 3 CP; TCS đổi màu/offset theo instance,
// TES kiểm tra primitive ID local (0) và instance ID (0/1).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "void main() { gl_Position = vec4(0.0); }\n";
static const char* kTCS =
    "#version 460 core\n"
    "layout(vertices = 3) out;\n"
    "patch out vec4 pColor;\n"
    "patch out vec2 pShift;\n"
    "void main() {\n"
    "  pShift = vec2(float(gl_InstanceID) * 0.9, 0.0);\n"
    "  bool ok = (gl_PatchVerticesIn == 3 && gl_PrimitiveID == 0);\n"
    "  pColor = !ok ? vec4(0.0,0.0,1.0,1.0)\n"
    "               : (gl_InstanceID == 0 ? vec4(1.0,0.0,0.0,1.0)\n"
    "                                     : vec4(0.0,1.0,0.0,1.0));\n"
    "}\n";
static const char* kTES =
    "#version 460 core\n"
    "layout(triangles) in;\n"
    "patch in vec4 pColor;\n"
    "patch in vec2 pShift;\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "  vec2 uv = gl_TessCoord.xy;\n"
    "  vec2 p = vec2(-0.8 + 1.6 * uv.x, -0.8 + 1.6 * uv.y) + pShift;\n"
    "  vec4 col = pColor;\n"
    "  if (gl_PatchVerticesIn != 3 || gl_PrimitiveID != 0 || gl_InstanceID > 1)\n"
    "    col = vec4(0.0,0.0,1.0,1.0);\n"
    "  else if (gl_InstanceID == 1)\n"
    "    col.b = 0.5;\n"
    "  vColor = col;\n"
    "  gl_Position = vec4(p, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec4 vColor;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vColor; }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src, const char* nm) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    tglmt::GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("FAIL %s compile: %s\n", nm, log);
        return 0;
    }
    return s;
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_instance_id_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    tglmt::GLuint vs = Compile(0x8B31, kVS, "vs");
    tglmt::GLuint tc = Compile(0x8E88, kTCS, "tcs");
    tglmt::GLuint te = Compile(0x8E87, kTES, "tes");
    tglmt::GLuint fs = Compile(0x8B30, kFS, "fs");
    if (!vs || !tc || !te || !fs) return 1;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL link: %s\n", log);
        return 1;
    }
    glUseProgram(p);
    glPatchParameteri(0x8E72 /*GL_PATCH_VERTICES*/, 3);
    float dOuter[4] = {2.0f, 2.0f, 2.0f, 2.0f};
    float dInner[2] = {2.0f, 2.0f};
    glPatchParameterfv(0x8E74, dOuter);
    glPatchParameterfv(0x8E73, dInner);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArraysInstanced(0x000E /*GL_PATCHES*/, 0, 3, 2);
    if (glGetError() != 0) { printf("FAIL gl error\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* left = px + (32 * 64 + 16) * 4;
    unsigned char* right = px + (32 * 64 + 48) * 4;
    printf("left=(%u,%u,%u,%u) right=(%u,%u,%u,%u)\n",
           left[0], left[1], left[2], left[3], right[0], right[1], right[2], right[3]);
    if (!(left[0] > 200 && left[1] < 50 && left[2] < 50)) {
        printf("FAIL instance 0 không đỏ (TCS gl_InstanceID sai)\n");
        return 1;
    }
    if (!(right[1] > 200 && right[0] < 50 && right[2] > 90 && right[2] < 180)) {
        printf("FAIL instance 1 không xanh+blue (TCS/TES gl_InstanceID sai)\n");
        return 1;
    }
    printf("test_tess_instance_id_apple PASS (TCS/TES gl_InstanceID + local gl_PrimitiveID)\n");
    return 0;
}
