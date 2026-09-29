// test_mc_mipmap_apple.cpp — Texture 1 level + minfilter mipmap (logo,
// widgets, sprite, blur-src UI: min 0x2702/0x2703, lv=1) phải sample được
// base level, không fault/đen (A11 fetch LOD>0 trên texture 1 level).
// Fix: sampler tắt lọc mip khi texture 1 level (NotMipmapped).
// Trên Intel hành vi base vốn đúng nên test này khóa plumbing (không hồi
// quy im lặng); bằng chứng A11 thật là screenshot + status=5 trên máy.
// Minified quad (texture 64 co về vùng nhỏ → LOD>0) vẫn phải ra màu base.
// Thiếu GPU → SKIP. Sai pixel → FAIL thật.
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
    "layout(location = 1) in vec2 uv;\n"
    "layout(location = 0) out vec2 vUV;\n"
    "void main() { vUV = uv; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform sampler2D Sampler0;\n"
    "void main() { fragColor = texture(Sampler0, vUV); }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_mipmap_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_mipmap_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_mipmap_apple FAIL: target nil\n");
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
    glBindAttribLocation(prog, 1, "uv");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok != 0, "link");
    if (gFails) return 1;

    // Texture 64 đỏ, chỉ 1 level, minfilter MIPMAP như logo/widgets game.
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    std::vector<unsigned char> red(64 * 64 * 4);
    for (size_t i = 0; i < 64 * 64; ++i) {
        red[i * 4] = 255;
        red[i * 4 + 1] = 0;
        red[i * 4 + 2] = 0;
        red[i * 4 + 3] = 255;
    }
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, red.data());
    glTexParameteri(0x0DE1, 0x2801, 0x2703); // MIN LINEAR_MIPMAP_LINEAR (logo)
    glTexParameteri(0x0DE1, 0x2800, 0x2601);
    glUseProgram(prog);
    glBindFramebuffer(0x8CA9, 0);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    // Quad NHỎ giữa màn hình (minify 64px → ~16px: LOD>0 như logo co).
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float q[4 * 4] = {-0.25f, -0.25f, 0, 0, /**/ 0.25f, -0.25f, 1, 0, /**/ 0.25f, 0.25f,
                      1,      1,       /**/ -0.25f, 0.25f, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(q), q, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, 0x1406, 0, 16, (void*)0);
    glVertexAttribPointer(1, 2, 0x1406, 0, 16, (void*)8);
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    Check(glGetError() == 0, "draw gl error");
    Check(ctx.appleStats.drawsEncoded > encBefore, "draw encoded");
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("[mip] mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[0] > 200 && mid[1] < 50 && mid[2] < 50, "minified mip-filter quad do (base)");
    if (gFails) return 1;
    printf("test_mc_mipmap_apple PASS (1-level + mipmap filter = base)\n");
    return 0;
}
