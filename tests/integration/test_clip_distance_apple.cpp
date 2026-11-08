// test_clip_distance_apple.cpp — VS gl_ClipDistance[0] phải clip thật trên
// pipeline vertex/fragment thường (không tess).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "void main() {\n"
    "  gl_ClipDistance[0] = aPos.x + 0.5;\n"
    "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 c;\n"
    "void main() { c = vec4(1, 0, 0, 1); }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    tglmt::GLint ok = 0; glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
               printf("FAIL compile: %s\n", log); return 0; }
    return s;
}

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_clip_distance_apple SKIP: no MTL device\n"); return 0; }
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
    tglmt::GLuint vao = 0, vbo = 0;
    const float verts[] = {-1, -1, 3, -1, -1, 3};
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glViewport(0, 0, 64, 64); glDisable(0x0B71); glDisable(0x0B44); glDisable(0x0BE2);
    glClearColor(0, 0, 1, 1); glClear(0x4000);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) { printf("FAIL draw\n"); return 1; }
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();
    unsigned char l[4] = {}, r[4] = {};
    glReadPixels(8, 32, 1, 1, 0x1908, 0x1401, l);
    glReadPixels(56, 32, 1, 1, 0x1908, 0x1401, r);
    printf("left=(%u,%u,%u) right=(%u,%u,%u)\n", l[0], l[1], l[2], r[0], r[1], r[2]);
    if (!(l[0] < 50 && l[2] > 200) || !(r[0] > 200 && r[2] < 50)) {
        printf("FAIL clip_distance (left blue, right red)\n"); return 1;
    }
    printf("test_clip_distance_apple PASS\n");
    return 0;
}
