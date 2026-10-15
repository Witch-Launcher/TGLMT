// test_water_translucent_apple.cpp — Bug 1 nước vanilla đục phẳng.
// Tái hiện translucent pass: clear xanh, blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA,
// vẽ quad đỏ alpha=0.5 → center phải tím ~(127,0,127).
// Null backend: chỉ kiểm tra state convert (Blend()[0]), bỏ qua GPU encode/readback.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool c, const char* m) {
    if (!c) { printf("test_water_translucent_apple FAIL: %s\n", m); ++gFails; }
}

static const char* kVS =
    "#version 330 core\n"
    "layout(location=0) in vec3 a_Pos;\n"
    "void main(){ gl_Position = vec4(a_Pos, 1.0); }\n";
static const char* kFS =
    "#version 330 core\n"
    "out vec4 o_C;\n"
    "uniform vec4 u_C;\n"
    "void main(){ o_C = u_C; }\n";

static GLuint MakeProg() {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    Check(ok == 1, "vs compile");
    glGetShaderiv(fs, 0x8B81, &ok);
    Check(ok == 1, "fs compile");
    GLuint pr = glCreateProgram();
    glAttachShader(pr, vs);
    glAttachShader(pr, fs);
    glLinkProgram(pr);
    glGetProgramiv(pr, 0x8B82, &ok);
    Check(ok == 1, "link");
    return pr;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);

    // --- Luôn kiểm tra state convert (cả Null lẫn Metal) ---
    glEnable(0x0BE2); // BLEND
    glBlendFunc(0x0302, 0x0303); // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
    Check(ctx.state.Blend()[0].srcRGB == 0x0302, "srcRGB convert");
    Check(ctx.state.Blend()[0].dstRGB == 0x0303, "dstRGB convert");
    Check(ctx.state.Blend()[0].srcAlpha == 0x0302, "srcAlpha convert");
    Check(ctx.state.Blend()[0].dstAlpha == 0x0303, "dstAlpha convert");
    Check(ctx.state.IsEnabled(0x0BE2), "blend enabled");
    glBlendFuncSeparate(0x0302, 0x0303, 0x0001, 0x0303);
    Check(ctx.state.Blend()[0].srcAlpha == 0x0001, "separate srcAlpha=ONE");
    glBlendFunc(0x0302, 0x0303); // trả lại như translucent pass vanilla
    glColorMask(1, 0, 0, 1);
    Check(ctx.state.ColorMask(0) == (1 | 8), "colormask state R+A");
    glColorMask(1, 1, 1, 1);

    if (ctx.device->isNull()) {
        if (gFails == 0) printf("test_water_translucent_apple PASS (null: state convert OK)\n");
        else printf("test_water_translucent_apple: %d FAIL\n", gFails);
        return gFails ? 1 : 0;
    }

    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_water_translucent_apple FAIL: target nil\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    GLuint prog = MakeProg();
    if (gFails) { printf("test_water_translucent_apple: %d FAIL\n", gFails); return 1; }
    GLint locC = glGetUniformLocation(prog, "u_C");

    // Quad fullscreen: 2 tam giác, pos only.
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float quad[6 * 3] = {-1, -1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, 1, 1, 0, -1, 1, 0};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406, 0, 0, (void*)0);

    auto drawOnce = [&](float r, float g, float b, float a) {
        glUseProgram(prog);
        glUniform4f(locC, r, g, b, a);
        glDrawArrays(0x0004, 0, 6); // TRIANGLES
    };

    // Case 1: glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA), đỏ a=0.5 trên nền xanh.
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71); // DEPTH_TEST off như translucent pass GUI
    glEnable(0x0BE2);
    glBlendFunc(0x0302, 0x0303);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    drawOnce(1, 0, 0, 0.5f);
    Check(glGetError() == 0, "gl error case1");
    Check(ctx.appleStats.drawsEncoded > 0, "case1 not encoded");
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("case1 mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    // kỳ vọng đỏ*0.5 + xanh*0.5 = (127,0,127)
    Check(std::abs((int)mid[0] - 127) <= 8 && mid[1] <= 8 && std::abs((int)mid[2] - 127) <= 8,
          "case1 blend formula sai (nuoc duc?)");

    // Case 2: BlendSeparate như GUI/MC (RGB SRC_ALPHA/ONE_MINUS, A ONE/ONE_MINUS).
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glBlendFuncSeparate(0x0302, 0x0303, 0x0001, 0x0303);
    drawOnce(1, 0, 0, 0.5f);
    Check(glGetError() == 0, "gl error case2");
    Check(ctx.appleStats.drawsEncoded > encBefore, "case2 not encoded");
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    mid = px + (32 * 64 + 32) * 4;
    printf("case2 mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(std::abs((int)mid[0] - 127) <= 8 && mid[1] <= 8 && std::abs((int)mid[2] - 127) <= 8,
          "case2 separate blend sai");

    // Case 3 (đối chứng): tắt blend → quad đục đỏ (giống ảnh 5 ban đêm).
    glDisable(0x0BE2);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    drawOnce(1, 0, 0, 0.5f);
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    mid = px + (32 * 64 + 32) * 4;
    printf("case3 noblend mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[0] > 200 && mid[2] < 80, "case3 doi chung that bai");

    // Case 4: glColorMask (G,B tắt) + tắt blend, vẽ xanh lá trên nền đen
    // → kênh G bị chặn, pixel phải đen. Chứng minh writeMask có bake (limits.md cũ ghi no-op).
    glColorMask(1, 0, 0, 1);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    drawOnce(0, 1, 0, 1);
    Check(glGetError() == 0, "gl error case4");
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    mid = px + (32 * 64 + 32) * 4;
    printf("case4 colormask mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[0] <= 8 && mid[1] <= 8 && mid[2] <= 8, "case4 colormask khong bake");
    glColorMask(1, 1, 1, 1);

    glDeleteProgram(prog);
    if (gFails == 0) { printf("test_water_translucent_apple PASS\n"); return 0; }
    printf("test_water_translucent_apple: %d FAIL\n", gFails);
    return 1;
}
