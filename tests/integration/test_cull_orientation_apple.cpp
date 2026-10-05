// test_cull_orientation_apple.cpp — Kiểm chứng frontFace flip của TGLMT.
// Quad CCW (GL front) fullscreen: cull BACK phải HIỆN, cull FRONT phải ẨN.
// Nếu đảo (do flip frontFace sai với viewport mirror) → bug "bóng ma"/mất icon.
// Thiếu GPU → SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_cull_orientation_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_cull_orientation_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_cull_orientation_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    {
        GLint ok = 0;
        glGetProgramiv(prog, 0x8B82, &ok);
        Check(ok != 0, "link");
    }
    if (gFails) return 1;

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // CCW fullscreen quad (GL front khi front=CCW): (-1,-1) -> (1,-1) -> (1,1) -> (-1,1)
    float q[8] = {-1, -1, 1, -1, 1, 1, -1, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(q), q, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);

    glUseProgram(prog);
    glBindFramebuffer(0x8D40, 0);
    glViewport(0, 0, 64, 64);
    glFrontFace(0x0901); // CCW
    glEnable(0x0B44);    // CULL_FACE
    auto readCenter = [&]() {
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        return px[(32 * 64 + 32) * 4]; // R
    };
    glCullFace(0x0405); // BACK: CCW quad phải HIỆN
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned back = readCenter();
    printf("[cull] BACK cull center R=%u (want 255)\n", back);
    glCullFace(0x0404); // FRONT: CCW quad phải ẨN (xanh nền)
    glClear(0x00004000);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned front = readCenter();
    printf("[cull] FRONT cull center R=%u (want 0)\n", front);
    Check(back > 200, "cull BACK shows CCW front face");
    Check(front < 50, "cull FRONT hides CCW front face");
    if (gFails) return 1;
    printf("test_cull_orientation_apple PASS\n");
    return 0;
}
