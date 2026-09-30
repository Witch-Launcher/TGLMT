// test_mc_gettex_apple.cpp — Hồi quy crash khi vào world 26.1.2 trên máy thật.
//
// Vanilla gọi glGetTextureSubImage(mainColorTex, ...) trong auto-screenshot
// khi join world (copyTextureToBuffer). Code cũ chỉ copy shadow CPU (draws đi
// GPU-only → shadow toàn 0) và không hỗ trợ PBO/region → screenshot đen, và
// game crash 1282 ở copyTobuffer. Fix: làm tươi từ GPU + PBO + region + PACK.
//
// Test: vẽ đỏ vào FBO texture, đọc lại bằng glGetTextureSubImage → đỏ
// (cũ: đen). Bản PBO: bind PIXEL_PACK_BUFFER, đọc offset → buffer đỏ.
// Thiếu GPU → SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_gettex_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_gettex_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_gettex_apple FAIL: target nil\n");
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
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok != 0, "link");
    if (gFails) return 1;

    // FBO texture 64x64 (đúng dạng main framebuffer color của game).
    GLuint fboTex, fbo;
    glGenTextures(1, &fboTex);
    glBindTexture(0x0DE1, fboTex);
    std::vector<unsigned char> zeros(64 * 64 * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, zeros.data());
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fboTex, 0);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float tri[6] = {-1, -1, 3, -1, -1, 3};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);

    glUseProgram(prog);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == 0, "draw gl error");

    // 1. Đọc trực tiếp (đường auto-screenshot): phải đỏ tươi, không lỗi.
    std::vector<unsigned char> px(64 * 64 * 4, 0);
    glGetTextureSubImage(fboTex, 0, 0, 0, 0, 64, 64, 1, 0x1908, 0x1401,
                         (GLsizei)px.size(), px.data());
    Check(glGetError() == 0, "getSubImage gl error");
    printf("[gettex] px0=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
    Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "fresh GPU pixels (red)");
    if (gFails) return 1;

    // 2. Bản PBO (vanilla screenshot dùng PBO + offset).
    GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(0x88EB, pbo);
    std::vector<unsigned char> pboInit(64 * 64 * 4 + 64, 0x7F);
    glBufferData(0x88EB, (GLsizei)pboInit.size(), pboInit.data(), 0x88E4);
    glGetTextureSubImage(fboTex, 0, 0, 0, 0, 64, 64, 1, 0x1908, 0x1401,
                         (GLsizei)(64 * 64 * 4), (void*)32);
    Check(glGetError() == 0, "pbo getSubImage gl error");
    std::vector<unsigned char> back(64 * 64 * 4 + 64, 0);
    glGetBufferSubData(0x88EB, 0, (GLsizei)back.size(), back.data());
    printf("[gettex-pbo] off32=(%u,%u,%u,%u) pre00=%u\n", back[32], back[33], back[34],
           back[35], back[0]);
    Check(back[32] > 200 && back[33] < 50 && back[0] == 0x7F, "pbo offset write");
    glBindBuffer(0x88EB, 0);
    if (gFails) return 1;

    printf("test_mc_gettex_apple PASS (fresh GetTextureSubImage + PBO)\n");
    return 0;
}
