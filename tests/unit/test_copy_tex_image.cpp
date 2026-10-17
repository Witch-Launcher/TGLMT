// test_copy_tex_image.cpp — P1 copy texture: glCopyTexImage2D (định nghĩa lại
// storage + copy) và depth path của glCopyImageSubData.
//
// Căn cứ Iris 26.1: RenderTargets.copyPreTranslucentDepth/copyPreHandDepth gọi
// glCopyTexImage2D(GL_TEXTURE_2D, 0, currentDepthFormat, 0,0,W,H,0) với read FB =
// depth source; DepthCopyStrategy.fastest gọi glCopyImageSubData depth→depth.
// Spec (4.6 §18.3.2 + ARB_depth_texture): internalformat depth → nguồn là DEPTH
// buffer của read FB (không phải color); không depth buffer → INVALID_OPERATION.
//
// Null backend: chạy validation + shadow copy. Apple: color copy GPU +
// depth copy GPU, verify bằng depth-test discrimination (green=nền nếu depth
// copy được giữ; red nếu depth bị mất/clear — bắt cả hazard first-clear).
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
        printf("test_copy_tex_image FAIL: %s\n", msg);
        ++gFails;
    }
}
static void ExpectErr(GLenum e, const char* what) {
    GLenum g = glGetError();
    if (g != e) {
        printf("test_copy_tex_image FAIL: %s: glerr 0x%04x != 0x%04x\n", what, (unsigned)g,
               (unsigned)e);
        ++gFails;
    }
}

// Program đổ 1 màu; đỉnh (x,y,z) → clip thẳng (z dùng cho depth test).
static GLuint MakeSolidProgram() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec3 a_pos;\n"
        "uniform float uz;\n"
        "void main() { gl_Position = vec4(a_pos.xy, uz, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) printf("test_copy_tex_image: shader link fail\n");
    return ok ? p : 0;
}

// Quad fullscreen phẳng; z (NDC, GL window depth = (z+1)/2) truyền qua uniform
// `uz` để tránh phụ thuộc đường re-upload VBO giữa các draw.
static void QuadXY(GLuint vbo) {
    float v[18] = {-1, -1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, 1, 1, 0, -1, 1, 0};
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(v), v, 0x88E4);
}

int main() {
    // ---------- Phần 1: Null backend — validation + shadow copy ----------
    {
        Context ctx("null");
        Context::MakeCurrent(&ctx);

        GLuint src = 0, dst = 0, fbo = 0;
        glGenTextures(1, &src);
        glGenTextures(1, &dst);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, dst);
        glGetError(); // reset lỗi cũ (nếu có) trước khi test

        // target GL_TEXTURE_CUBE_MAP (không phải face) → INVALID_ENUM
        glCopyTexImage2D(0x8513, 0, 0x8058, 0, 0, 8, 8, 0);
        ExpectErr(0x0500, "target 0x8513");
        // border != 0 → INVALID_VALUE
        glCopyTexImage2D(0x0DE1, 0, 0x8058, 0, 0, 8, 8, 1);
        ExpectErr(0x0501, "border=1");
        // level < 0 → INVALID_VALUE
        glCopyTexImage2D(0x0DE1, -1, 0x8058, 0, 0, 8, 8, 0);
        ExpectErr(0x0501, "level=-1");
        // w > MAX_TEXTURE_SIZE (8192) → INVALID_VALUE
        glCopyTexImage2D(0x0DE1, 0, 0x8058, 0, 0, 8193, 8, 0);
        ExpectErr(0x0501, "w=8193");
        // depth internalformat + read FBO không có depth → INVALID_OPERATION
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo);
        glCopyTexImage2D(0x0DE1, 0, 0x8CAC, 0, 0, 8, 8, 0);
        ExpectErr(0x0502, "depth ifmt, no depth buffer");
        {
            auto it = ctx.textures.find(dst);
            Check(it != ctx.textures.end() && it->second.w == 0 && it->second.h == 0,
                  "storage không đổi sau lỗi validate");
        }

        // Color copy null (shadow): nguồn FBO color 64x64 pattern → đích
        // định nghĩa lại 32x32 (đích pre-define 8x8 để test realloc).
        glBindTexture(0x0DE1, src);
        std::vector<uint8_t> px((size_t)64 * 64 * 4, 0);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                uint8_t* p = px.data() + ((size_t)y * 64 + x) * 4;
                p[0] = (uint8_t)(y * 4); p[1] = (uint8_t)(x * 4); p[2] = 77; p[3] = 255;
            }
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, px.data());
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, src, 0);
        glBindTexture(0x0DE1, dst);
        std::vector<uint8_t> tiny((size_t)8 * 8 * 4, 9);
        glTexImage2D(0x0DE1, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, tiny.data());
        glCopyTexImage2D(0x0DE1, 0, 0x8058, 0, 0, 32, 32, 0);
        Check(glGetError() == 0, "color copy null no error");
        {
            auto it = ctx.textures.find(dst);
            Check(it != ctx.textures.end() && it->second.w == 32 && it->second.h == 32,
                  "realloc 8x8 → 32x32");
        }
        std::vector<uint8_t> got((size_t)32 * 32 * 4, 0);
        glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, got.data());
        Check(got[0] == 0 && got[1] == 0 && got[2] == 77 && got[3] == 255,
              "null copy row0 = nguồn row0");
        Check(got[((size_t)16 * 32 + 0) * 4 + 0] == 64 &&
                  got[((size_t)31 * 32 + 0) * 4 + 0] == 124,
              "null copy row16/31 đúng");
    }

    // ---------- Phần 2: Apple GPU — color copy + depth copy thật ----------
    Context apple("apple");
    Context::MakeCurrent(&apple);
    if (apple.device->isNull()) {
        printf("test_copy_tex_image: SKIP phần GPU (no metal device). Null checks: %s\n",
               gFails ? "FAIL" : "OK");
        return gFails ? 1 : 0;
    }

    // (a) Color copy GPU qua glCopyTexImage2D + đọc lại bằng glGetTexImage.
    {
        GLuint s = 0, d = 0, fbo = 0;
        glGenTextures(1, &s);
        glGenTextures(1, &d);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, s);
        std::vector<uint8_t> px((size_t)64 * 64 * 4, 0);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                uint8_t* p = px.data() + ((size_t)y * 64 + x) * 4;
                p[0] = (uint8_t)(y * 4); p[1] = (uint8_t)(x * 4); p[2] = 77; p[3] = 255;
            }
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, px.data());
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, s, 0);
        glBindTexture(0x0DE1, d);
        glCopyTexImage2D(0x0DE1, 0, 0x8058, 0, 0, 32, 32, 0);
        Check(glGetError() == 0, "color copy GPU no error");
        std::vector<uint8_t> got((size_t)32 * 32 * 4, 0);
        glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, got.data());
        Check(got[0] == 0 && got[1] == 0 && got[2] == 77,
              "GPU copy row0 = nguồn row0");
        Check(got[((size_t)31 * 32 + 0) * 4 + 0] == 124 &&
                  got[((size_t)16 * 32 + 0) * 4 + 0] == 64,
              "GPU copy row31/16 đúng (32x32 định nghĩa lại)");
    }

    // Setup chung cho depth: program + quad + VAO/VBO.
    GLuint prog = MakeSolidProgram();
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406, 0, 0, (void*)0);
    glUseProgram(prog);
    GLint uzLoc = glGetUniformLocation(prog, "uz");
    glViewport(0, 0, 64, 64);
    glEnable(0x0B71);       // DEPTH_TEST
    glDepthFunc(0x0201);    // LESS
    glDepthMask(1);

    // Depth test discrimination trên FBO2 (depth = 0.25 window, copy về):
    //  z_ndc=0.5 → window 0.75 > 0.25 → FAIL → giữ green.
    //  z_ndc=-0.9 → window 0.05 < 0.25 → PASS → red (control: depth test thật sự
    //  chạy trên bản copy; trong GL window depth = (z_ndc+1)/2).
    auto DepthDiscrimination = [&](GLuint depthTex, const char* what) {
        GLuint c2 = 0, fbo2 = 0;
        glGenTextures(1, &c2);
        glBindTexture(0x0DE1, c2);
        std::vector<uint8_t> green((size_t)64 * 64 * 4, 0);
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, green.data());
        glGenFramebuffers(1, &fbo2);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo2);
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, c2, 0);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depthTex, 0);
        Check(glGetError() == 0, "attach FBO2");
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(0x00004000); // color xanh; depth load = giữ bản copy
        QuadXY(vbo);
        glUniform1f(uzLoc, 0.5f);
        glDrawArrays(0x0004, 0, 6); // window 0.75 > 0.25 fail → không ghi gì
        unsigned char g[4] = {0, 0, 0, 0};
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, g);
        Check(g[1] > 200 && g[0] < 50, what); // còn green = depth copy được giữ
        glUniform1f(uzLoc, -0.9f);
        glDrawArrays(0x0004, 0, 6); // window 0.05 < 0.25 pass → red (control)
        unsigned char r[4] = {0, 0, 0, 0};
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, r);
        char msg[160];
        snprintf(msg, sizeof(msg), "%s control (z_ndc=-0.9 pass → red)", what);
        Check(r[0] > 200 && r[1] < 50, msg);
        glDeleteFramebuffers(1, &fbo2);
        glDeleteTextures(1, &c2);
    };

    // (b0) Sanity: glClear depth trên FBO depth-only thật sự ghi 0.25?
    // (không copy — cô lập: nếu fail thì vấn đề ở clear, không phải đường copy).
    {
        GLuint d0 = 0, f0 = 0;
        glGenTextures(1, &d0);
        glBindTexture(0x0DE1, d0);
        glTexImage2D(0x0DE1, 0, 0x8CAC, 64, 64, 0, 0x1902, 0x1406, nullptr);
        glGenFramebuffers(1, &f0);
        glBindFramebuffer(0x8D40, f0);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, d0, 0);
        glClearDepth(0.25);
        glClear(0x00000100);
        Check(glGetError() == 0, "depth clear sanity no error");
        DepthDiscrimination(d0, "depth clear 0.25 sanity");
        glDeleteFramebuffers(1, &f0);
        glDeleteTextures(1, &d0);
    }

    // (b) glCopyTexImage2D depth: D1 clear 0.25 → copy sang D2 → depth test.
    {
        GLuint d1 = 0, fbo1 = 0;
        glGenTextures(1, &d1);
        glBindTexture(0x0DE1, d1);
        glTexImage2D(0x0DE1, 0, 0x8CAC /*DEPTH_COMPONENT32F*/, 64, 64, 0, 0x1902, 0x1406,
                     nullptr);
        glGenFramebuffers(1, &fbo1);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo1);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, d1, 0);
        glClearDepth(0.25);
        glClear(0x00000100); // DEPTH_BUFFER_BIT
        GLuint d2 = 0;
        glGenTextures(1, &d2);
        glBindTexture(0x0DE1, d2);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo1); // read FB = nguồn depth
        glCopyTexImage2D(0x0DE1, 0, 0x8CAC, 0, 0, 64, 64, 0);
        Check(glGetError() == 0, "copyTexImage2D depth no error");
        {
            auto it = apple.textures.find(d2);
            Check(it != apple.textures.end() && it->second.w == 64 &&
                      it->second.internalFormat == 0x8CAC,
                  "D2 định nghĩa 64x64 DEPTH_COMPONENT32F");
        }
        DepthDiscrimination(d2, "copyTexImage2D depth giữ 0.25");
        glDeleteFramebuffers(1, &fbo1);
        glDeleteTextures(1, &d1);
        glDeleteTextures(1, &d2);
    }

    // (c) glCopyImageSubData depth: D1 → D3 (DepthCopyStrategy.fastest).
    {
        GLuint d1 = 0, d3 = 0, fbo1 = 0;
        glGenTextures(1, &d1);
        glBindTexture(0x0DE1, d1);
        glTexImage2D(0x0DE1, 0, 0x8CAC, 64, 64, 0, 0x1902, 0x1406, nullptr);
        glGenFramebuffers(1, &fbo1);
        glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER — set cả READ+DRAW*/, fbo1);
        glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, d1, 0);
        glClearDepth(0.25);
        glClear(0x00000100);
        glGenTextures(1, &d3);
        glBindTexture(0x0DE1, d3);
        glTexImage2D(0x0DE1, 0, 0x8CAC, 64, 64, 0, 0x1902, 0x1406, nullptr);
        glCopyImageSubData(d1, 0x0DE1, 0, 0, 0, 0, d3, 0x0DE1, 0, 0, 0, 0, 64, 64, 1);
        Check(glGetError() == 0, "copyImageSubData depth no error");
        DepthDiscrimination(d3, "copyImageSubData depth giữ 0.25");
        glDeleteFramebuffers(1, &fbo1);
        glDeleteTextures(1, &d1);
        glDeleteTextures(1, &d3);
    }

    if (gFails) {
        printf("test_copy_tex_image: %d FAIL\n", gFails);
        return 1;
    }
    printf("test_copy_tex_image PASS (validation + color/depth copy)\n");
    return 0;
}
