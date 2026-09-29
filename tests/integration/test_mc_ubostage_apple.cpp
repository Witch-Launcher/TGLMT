// test_mc_ubostage_apple.cpp — Hồi quy đen màn hình menu-blur trên A11.
//
// Căn nguyên từ log máy thật (b3-copyblur1, iPhone 8 Plus): các draw post-blur
// (box_blur: vs dùng [SamplerInfo, RotScaleConfig], fs dùng
// [Globals, SamplerInfo, BlurConfig]) fault GPU hàng loạt → iOS ban submissions
// (status=5 "prior/excessive GPU errors") → đen + đứng hình, game vẫn chạy.
//
// Bug: converter gán [[buffer(17+bi)]] theo thứ tự khai báo TRONG TỪNG stage,
// nhưng AppleDrawGL bind theo thứ tự GỘP (vs trước) cho CẢ 2 stage → fs đọc
// nhầm buffer (BlurConfig/Radius rác → loop treo GPU).
//
// Test dựng đúng hình đó: vs blocks [UBO_A, UBO_B], fs blocks [UBO_C, UBO_A,
// UBO_D] (block chung A ở index khác nhau mỗi stage). FS chỉ xuất D.
// Đúng slot → xanh lá; lệch slot (code cũ) → đỏ. Thiếu GPU → SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(std140) uniform UBO_A { vec4 a; };\n"
    "layout(std140) uniform UBO_B { vec4 b; };\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec4 vA;\n"
    "void main() { vA = a + b * 0.0; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(std140) uniform UBO_C { vec4 c; };\n"
    "layout(std140) uniform UBO_A { vec4 a; };\n"
    "layout(std140) uniform UBO_D { vec4 d; };\n"
    "layout(location = 0) in vec4 vA;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vA * 0.0 + c * 0.0 + a * 0.0 + d; }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_ubostage_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_ubostage_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_ubostage_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

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

    // Mỗi block 16B (vec4), giá trị phân biệt: A=đỏ, B=alpha, C=đỏ, D=xanh lá.
    // FS xuất D → đúng slot mới xanh; code cũ (gộp) đọc C ở slot D → đỏ.
    float va[4] = {1, 0, 0, 0};
    float vb[4] = {0, 0, 0, 0};
    float vc[4] = {1, 0, 0, 0};
    float vd[4] = {0, 1, 0, 0};
    GLuint bA, bB, bC, bD;
    glGenBuffers(1, &bA);
    glGenBuffers(1, &bB);
    glGenBuffers(1, &bC);
    glGenBuffers(1, &bD);
    glBindBuffer(0x8A11, bA);
    glBufferData(0x8A11, 16, va, 0x88E4);
    glBindBuffer(0x8A11, bB);
    glBufferData(0x8A11, 16, vb, 0x88E4);
    glBindBuffer(0x8A11, bC);
    glBufferData(0x8A11, 16, vc, 0x88E4);
    glBindBuffer(0x8A11, bD);
    glBufferData(0x8A11, 16, vd, 0x88E4);
    GLuint iA = glGetUniformBlockIndex(prog, "UBO_A");
    GLuint iB = glGetUniformBlockIndex(prog, "UBO_B");
    GLuint iC = glGetUniformBlockIndex(prog, "UBO_C");
    GLuint iD = glGetUniformBlockIndex(prog, "UBO_D");
    Check(iA != 0xFFFFFFFFu && iB != 0xFFFFFFFFu && iC != 0xFFFFFFFFu &&
              iD != 0xFFFFFFFFu,
          "block index");
    if (gFails) return 1;
    glUniformBlockBinding(prog, iA, 10);
    glUniformBlockBinding(prog, iB, 11);
    glUniformBlockBinding(prog, iC, 12);
    glUniformBlockBinding(prog, iD, 13);
    glBindBufferBase(0x8A11, 10, bA);
    glBindBufferBase(0x8A11, 11, bB);
    glBindBufferBase(0x8A11, 12, bC);
    glBindBufferBase(0x8A11, 13, bD);

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
    glBindFramebuffer(0x8CA9, 0);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == 0, "draw gl error");
    Check(ctx.appleStats.drawsEncoded > encBefore, "draw encoded");
    if (gFails) return 1;

    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("[ubostage] mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[1] > 200 && mid[0] < 50 && mid[2] < 50, "per-stage UBO slot (fs D = green)");
    if (gFails) return 1;
    printf("test_mc_ubostage_apple PASS (UBO per-stage slots)\n");
    return 0;
}
