// test_mc_validate_apple.cpp — Validator chẩn đoán GPU fault A11.
// CPU-side check trong AppleDrawGL, chỉ LOG + đếm (không chặn draw):
//  A. Draw tốt (textured quad đủ buffer) → rangeWarn==0 && hazardWarn==0.
//  B. Feedback: render vào FBO đồng thời sample đúng texture đó →
//     hazardWarn==1, vẫn encode, không crash.
//  C. Non-indexed đọc vượt VBO (1 đỉnh nhưng draw 3) → rangeWarn==1.
//  D. Indexed trỏ vượt VBO (index 5 khi chỉ có 2 đỉnh) → rangeWarn==1.
// Thiếu GPU → SKIP. Sai counter → FAIL thật.
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
    "layout(location = 0) out vec2 vUV;\n"
    "void main() { vUV = pos * 0.5 + 0.5; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFSRed =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";
static const char* kFSTex =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform sampler2D Sampler0;\n"
    "void main() { fragColor = texture(Sampler0, vUV); }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_validate_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_validate_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_validate_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint fRed = glCreateShader(0x8B30);
    glShaderSource(fRed, 1, &kFSRed, nullptr);
    glCompileShader(fRed);
    GLuint fTex = glCreateShader(0x8B30);
    glShaderSource(fTex, 1, &kFSTex, nullptr);
    glCompileShader(fTex);
    GLuint progRed = glCreateProgram();
    glAttachShader(progRed, vs);
    glAttachShader(progRed, fRed);
    glBindAttribLocation(progRed, 0, "pos");
    glLinkProgram(progRed);
    GLuint progTex = glCreateProgram();
    glAttachShader(progTex, vs);
    glAttachShader(progTex, fTex);
    glBindAttribLocation(progTex, 0, "pos");
    glLinkProgram(progTex);
    GLint ok = 0;
    glGetProgramiv(progRed, 0x8B82, &ok);
    Check(ok != 0, "link red");
    glGetProgramiv(progTex, 0x8B82, &ok);
    Check(ok != 0, "link tex");
    if (gFails) return 1;

    // ---- A. draw tốt vào default ----
    {
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float tri[6] = {-1, -1, 3, -1, -1, 3};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
        glUseProgram(progRed);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "A gl error");
        Check(ctx.appleStats.rangeWarn == 0, "A rangeWarn==0 cho draw tot");
        Check(ctx.appleStats.hazardWarn == 0, "A hazardWarn==0 cho draw tot");
    }
    if (gFails) return 1;

    // ---- B. feedback: FBO 64 + texture 64, sample chính nó ----
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, tex);
        std::vector<unsigned char> zeros(64 * 64 * 4, 0);
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, zeros.data());
        glTexParameteri(0x0DE1, 0x2801, 0x2600);
        glTexParameteri(0x0DE1, 0x2800, 0x2600);
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(0x8CA9, fbo);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float tri[6] = {-1, -1, 3, -1, -1, 3};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
        glUseProgram(progTex);
        glViewport(0, 0, 64, 64);
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "B gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "B draw encoded");
        Check(ctx.appleStats.hazardWarn == 1, "B hazardWarn==1 cho feedback");
    }
    if (gFails) return 1;

    // ---- C. non-indexed vượt VBO ----
    {
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float one[2] = {-1, -1}; // chỉ 1 đỉnh (8B)
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(one), one, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
        glUseProgram(progRed);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "C gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "C draw encoded (khong chan)");
        Check(ctx.appleStats.rangeWarn == 1, "C rangeWarn==1 cho fetch vuot");
    }
    if (gFails) return 1;

    // ---- D. indexed trỏ vượt VBO ----
    {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float two[4] = {-1, -1, 3, -1}; // 2 đỉnh (16B)
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(two), two, 0x88E4);
        uint32_t idx[3] = {0, 1, 5}; // 5 vượt
        glGenBuffers(1, &ebo);
        glBindBuffer(0x8893, ebo);
        glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
        glUseProgram(progRed);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glDrawElements(0x0004, 3, 0x1405, (void*)0);
        Check(glGetError() == 0, "D gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "D draw encoded (khong chan)");
        Check(ctx.appleStats.rangeWarn >= 2, "D rangeWarn tang cho index vuot");
    }
    if (gFails) return 1;
    printf("test_mc_validate_apple PASS (oracle/hazard/range)\n");
    return 0;
}
