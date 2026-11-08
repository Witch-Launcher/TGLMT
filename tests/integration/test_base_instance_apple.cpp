// test_base_instance_apple.cpp — ARB_base_instance: cả 3 entry point phải
// chuyển baseInstance thật vào Metal draw (gl_BaseInstance trong VS).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "void main() {\n"
    "  float b = (gl_BaseInstance == 0u) ? -0.5 : 0.5;\n"
    "  gl_Position = vec4(aPos + vec2(b, 0.0), 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 c;\n"
    "void main() { c = vec4(0, 1, 0, 1); }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    tglmt::GLint ok = 0; glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
               printf("FAIL compile: %s\n", log); return 0; }
    return s;
}

static bool CheckRightOnly(tglmt::Context& ctx, const char* label) {
    using namespace tglmt::gl;
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();
    unsigned char l[4] = {}, r[4] = {};
    glReadPixels(16, 32, 1, 1, 0x1908, 0x1401, l);
    glReadPixels(48, 32, 1, 1, 0x1908, 0x1401, r);
    printf("%s left=(%u,%u,%u) right=(%u,%u,%u)\n", label, l[0], l[1], l[2], r[0], r[1], r[2]);
    if (!(l[0] < 50 && l[1] < 50) || !(r[0] < 50 && r[1] > 200)) {
        printf("FAIL %s: baseInstance không dịch draw sang phải\n", label);
        return false;
    }
    return true;
}

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_base_instance_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vs = Compile(0x8B31, kVS), fs = Compile(0x8B30, kFS);
    if (!vs || !fs) return 1;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs); glLinkProgram(p);
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL link: %s\n", log); return 1; }
    glUseProgram(p);

    const float verts[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};
    const unsigned short idx[] = {0, 1, 2};
    tglmt::GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glGenBuffers(1, &ebo); glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);

    glViewport(0, 0, 64, 64); glDisable(0x0B71); glDisable(0x0B44); glDisable(0x0BE2);
    glClearColor(0, 0, 0, 1);

    glClear(0x4000);
    glDrawArraysInstancedBaseInstance(0x0004, 0, 3, 1, 1);
    if (glGetError() != 0 || !CheckRightOnly(ctx, "DrawArraysInstancedBaseInstance")) return 1;

    glClear(0x4000);
    glDrawElementsInstancedBaseInstance(0x0004, 3, 0x1403, nullptr, 1, 1);
    if (glGetError() != 0 || !CheckRightOnly(ctx, "DrawElementsInstancedBaseInstance")) return 1;

    glClear(0x4000);
    glDrawElementsInstancedBaseVertexBaseInstance(0x0004, 3, 0x1403, nullptr, 1, 0, 1);
    if (glGetError() != 0 || !CheckRightOnly(ctx, "DrawElementsInstancedBaseVertexBaseInstance")) return 1;

    printf("test_base_instance_apple PASS\n");
    return 0;
}
