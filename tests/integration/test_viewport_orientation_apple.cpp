// test_viewport_orientation_apple.cpp — Xác minh quy ước tọa độ Metal:
// viewport mirror (y=H,h=-H, kiểu TGLMT) + scissor map ra memory rows nào.
// VS tô NDC y=+1 ĐỎ, y=-1 XANH; đọc memory rows trực tiếp (không qua present).
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
        printf("test_viewport_orientation_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec2 vP;\n"
    "void main() { vP = pos; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vP;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = (vP.y > 0.0) ? vec4(1.0,0.0,0.0,1.0) : vec4(0.0,0.0,1.0,1.0); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_viewport_orientation_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_viewport_orientation_apple FAIL: target nil\n");
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
    glBindFramebuffer(0x8CA9, 0);
    glViewport(0, 0, 64, 64); // TGLMT convert → mirror (y=64,h=-64)
    glDisable(0x0C11);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    std::vector<unsigned char> mem(64 * 64 * 4, 0);
    // Vẽ lại vào texture FBO để đọc memory trực tiếp:
    GLuint colTex = 0, fbo = 0;
    glGenTextures(1, &colTex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, colTex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, colTex, 0);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    {
        auto it = ctx.textures.find(colTex);
        Check(it != ctx.textures.end(), "col exists");
        if (it != ctx.textures.end() && it->second.gpu) {
            auto w = ctx.device->wrapAsTarget(it->second.gpu.get(), nullptr);
            bool ok = w && w->readback(mem.data(), 64 * 4);
            Check(ok, "readback");
            if (ok) {
                auto px = [&](int x, int y) -> unsigned {
                    return mem[(y * 64 + x) * 4]; // R
                };
                auto pb = [&](int x, int y) -> unsigned {
                    return mem[(y * 64 + x) * 4 + 2]; // B
                };
                printf("[vp] mem row0 (first bytes): R=%u B=%u\n", px(32, 0), pb(32, 0));
                printf("[vp] mem row63 (last bytes): R=%u B=%u\n", px(32, 63), pb(32, 63));
                // NDC y=+1 (ĐỎ) ở memory row nào?
                bool redFirst = px(32, 0) > 200;
                bool redLast = px(32, 63) > 200;
                printf("[vp] RED(NDC+1) at first-bytes=%d last-bytes=%d\n", (int)redFirst, (int)redLast);
            }
        }
    }
    if (gFails) return 1;
    printf("test_viewport_orientation_apple PASS (xem log để kết luận orientation)\n");
    return 0;
}
