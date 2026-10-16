// test_perf_draw_apple.cpp — Perf harness (không assert thời gian, chỉ đúng/sai
// pixels): mô phỏng game mix (đổi program/VAO/uniform/texture/FBO), in ns/draw
// theo pha (pipe/uniform/ubo/samp/idx) để tìm hot path. Chạy với
// TGLMT_DIAG=1 trên backend Metal để timers hoạt động.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 330 core\n"
    "layout(location=0) in vec3 a_Pos;\n"
    "layout(location=1) in vec2 a_UV;\n"
    "uniform vec4 u_Off;\n"
    "out vec2 v_UV;\n"
    "void main(){ gl_Position = vec4(a_Pos + u_Off.xyz, 1.0); v_UV = a_UV; }\n";
static const char* kFS =
    "#version 330 core\n"
    "in vec2 v_UV;\n"
    "uniform sampler2D u_T;\n"
    "uniform vec4 u_Mul;\n"
    "out vec4 o_C;\n"
    "void main(){ o_C = texture(u_T, v_UV) * u_Mul; }\n";
static const char* kFS2 =
    "#version 330 core\n"
    "uniform vec4 u_Mul;\n"
    "out vec4 o_C;\n"
    "void main(){ o_C = u_Mul; }\n";

static GLuint Prog(bool textured) {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    const char* pf = textured ? kFS : kFS2;
    glShaderSource(fs, 1, &pf, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint pr = glCreateProgram();
    glAttachShader(pr, vs);
    glAttachShader(pr, fs);
    glLinkProgram(pr);
    return pr;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_perf_draw_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target =
        ctx.device->makeRenderTargetWithDepth(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_perf_draw_apple FAIL: target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    GLuint pTex = Prog(true), pFlat = Prog(false);
    GLint ok = 0;
    glGetProgramiv(pTex, 0x8B82, &ok);
    if (!ok) { printf("test_perf_draw_apple FAIL: link tex\n"); return 1; }
    glGetProgramiv(pFlat, 0x8B82, &ok);
    if (!ok) { printf("test_perf_draw_apple FAIL: link flat\n"); return 1; }

    // Texture 4x4 trắng.
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    unsigned char img[4 * 4 * 4];
    memset(img, 255, sizeof(img));
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, img);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);

    // 2 VAO xen kẽ.
    GLuint vaos[2], vbos[2], ebo;
    float quad[4 * 5] = {-1, -1, 0, 0, 0, 1, -1, 0, 1, 0,
                         1, 1, 0, 1, 1, -1, 1, 0, 0, 1};
    for (int k = 0; k < 2; ++k) {
        glGenVertexArrays(1, &vaos[k]);
        glBindVertexArray(vaos[k]);
        glGenBuffers(1, &vbos[k]);
        glBindBuffer(0x8892, vbos[k]);
        glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, 0x1406, 0, 20, (void*)0);
        glVertexAttribPointer(1, 2, 0x1406, 0, 20, (void*)12);
    }
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    // EBO thuộc VAO hiện tại (vaos[1]); bind lại cho vaos[0].
    glBindVertexArray(vaos[0]);
    glBindBuffer(0x8893, ebo);
    glBindVertexArray(vaos[1]);
    glBindBuffer(0x8893, ebo);

    GLint lOffT = glGetUniformLocation(pTex, "u_Off");
    GLint lMulT = glGetUniformLocation(pTex, "u_Mul");
    GLint lSamp = glGetUniformLocation(pTex, "u_T");
    GLint lOffF = glGetUniformLocation(pFlat, "u_Off");
    GLint lMulF = glGetUniformLocation(pFlat, "u_Mul");

    glViewport(0, 0, 256, 256);
    glDisable(0x0BE2);
    glDisable(0x0B71);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);

    const int N = 2000;
    uint64_t d0 = ctx.appleStats.drawsEncoded;
    for (int i = 0; i < N; ++i) {
        bool textured = (i % 3 != 0);
        GLuint pr = textured ? pTex : pFlat;
        glUseProgram(pr);
        glBindVertexArray(vaos[i % 2]);
        float f = (float)(i % 64) / 64.0f;
        if (textured) {
            glUniform4f(lOffT, f * 0.1f, 0, 0, 0);
            glUniform4f(lMulT, 1, 1, 1, 1);
            glUniform1i(lSamp, 0);
        } else {
            glUniform4f(lOffF, 0, f * 0.1f, 0, 0);
            glUniform4f(lMulF, 1, 1, 1, 1);
        }
        glDrawElements(0x0004, 6, 0x1405, (void*)0);
    }
    if (glGetError() != 0) { printf("test_perf_draw_apple FAIL: gl error\n"); return 1; }
    uint64_t dn = ctx.appleStats.drawsEncoded - d0;
    if (dn != (uint64_t)N) {
        printf("test_perf_draw_apple FAIL: encoded %llu/%d\n", (unsigned long long)dn, N);
        return 1;
    }
    unsigned char px[256 * 256 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    unsigned char* mid = px + (128 * 256 + 128) * 4;
    printf("perf mid=(%u,%u,%u)\n", mid[0], mid[1], mid[2]);
    if (mid[0] < 200) { printf("test_perf_draw_apple FAIL: pixels\n"); return 1; }
    double n = (double)dn;
    printf("perf draws=%llu pipe=%.1fus uniform=%.1fus ubo=%.1fus samp=%.1fus idx=%.1fus draw=%.1fus\n",
           (unsigned long long)dn, ctx.appleStats.nsPipe / n / 1000.0,
           ctx.appleStats.nsUniform / n / 1000.0, ctx.appleStats.nsUbo / n / 1000.0,
           ctx.appleStats.nsSamp / n / 1000.0, ctx.appleStats.nsIdx / n / 1000.0,
           ctx.appleStats.nsDraw / n / 1000.0);
    printf("test_perf_draw_apple PASS\n");
    return 0;
}
