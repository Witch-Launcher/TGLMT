// test_fbo_completeness.cpp — GL 4.6 core FBO:
//   - attachment completeness: dims / layer targets / MISSING_ATTACHMENT
//   - GL 4.1+ (ARB_ES2_compatibility, core): draw/read buffer trỏ attachment
//     trống KHÔNG làm FBO incomplete — GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER /
//     READ_BUFFER không còn được sinh ra (đã gỡ clause §9.4.2)
//   - depth-only FBO (voxy DepthFramebuffer / HiZBuffer) → COMPLETE
//   - draw/read buffer state per-FBO, NAMED variants, validation errors
//   - glGetIntegerv(GL_DRAW_BUFFER / GL_READ_BUFFER / GL_DRAW_BUFFERi)
//   - glClearBuffer* COLOR/DEPTH/STENCIL/DEPTH_STENCIL
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_fbo_completeness FAIL: %s\n", msg); ++gFails; }
}
static GLenum DrainErr() { // đọc hết error queue, trả mã ĐẦU TIÊN
    GLenum first = 0;
    while (true) {
        GLenum e = glGetError();
        if (!e) break;
        if (!first) first = e;
    }
    return first;
}

static GLuint MakeTex2D(int w, int h, GLenum fmt = 0x8058) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    glTexImage2D(0x0DE1, 0, fmt, w, h, 0, 0x1908, 0x1401, nullptr);
    return t;
}
static GLuint MakeArr(int w, int h, int d) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(0x8C1A, t);
    glTexImage3D(0x8C1A, 0, 0x8058, w, h, d, 0, 0x1908, 0x1401, nullptr);
    return t;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint color = MakeTex2D(8, 8);
    GLuint depth = MakeTex2D(8, 8);
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD7, "empty FBO → MISSING_ATTACHMENT");

    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "cùng 8x8 → COMPLETE");

    GLuint small = MakeTex2D(4, 4);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, small, 0);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD9,
          "depth 4x4 vs color 8x8 → INCOMPLETE_DIMENSIONS");
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);

    // GL 4.1+ (ES2 compat): draw/read buffer trỏ attachment TRỐNG → vẫn COMPLETE.
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, 0, 0); // detach color (depth còn)
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
          "draw buffer trỏ attachment trống → COMPLETE (GL 4.1+)");
    glReadBuffer(0x8CE0); // ATT0 trống
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
          "read buffer trỏ attachment trống → COMPLETE (GL 4.1+)");
    glReadBuffer(0x0000 /*GL_NONE*/);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
          "read buffer NONE → COMPLETE (GL 4.1+)");
    glDrawBuffers(1, (const GLenum[]){0x8CE1 /*ATT1 trống*/});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
          "draw buffer ATT1 chưa attach → COMPLETE (GL 4.1+)");
    // reattach color cho phần sau
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    glReadBuffer(0x8CE0);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "reattach color → COMPLETE");
    glDeleteFramebuffers(1, &fbo);

    // --- Regression (Voxy DepthFramebuffer/HiZBuffer): depth-only FBO, KHÔNG
    // glDrawBuffer(GL_NONE) → vẫn COMPLETE trên GL 4.1+ (TGLMT nói 4.6).
    {
        GLuint fboD = 0;
        glGenFramebuffers(1, &fboD);
        glBindFramebuffer(0x8CA9 /*DRAW_FRAMEBUFFER*/, fboD);
        GLuint depthTex = MakeTex2D(8, 8, 0x81A5 /*DEPTH_COMPONENT24*/);
        glFramebufferTexture2D(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x0DE1, depthTex, 0);
        Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
              "depth-only FBO (draw buffer mặc định trống) → COMPLETE");
        Check(glCheckNamedFramebufferStatus(fboD, 0x8D40) == 0x8CD5,
              "named depth-only FBO → COMPLETE");
        glNamedFramebufferDrawBuffer(fboD, 0x0000 /*GL_NONE*/);
        Check(glGetError() == 0, "named draw buffer NONE → OK");
        Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "draw buffer NONE → COMPLETE");
        glBindFramebuffer(0x8CA9, 0);
        glDeleteFramebuffers(1, &fboD);
        glDeleteTextures(1, &depthTex);
    }

    // --- Layer targets (giữ nguyên)
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    GLuint arr = MakeArr(8, 8, 2);
    GLuint c1 = MakeTex2D(8, 8);
    glFramebufferTexture(0x8D40, 0x8CE0, arr, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, c1, 0);
    glDrawBuffers(2, (const GLenum[]){0x8CE0, 0x8CE1});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8DA8,
          "array layered + 2D → INCOMPLETE_LAYER_TARGETS");
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, 0, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "array 1 attachment → COMPLETE");
    glBindFramebuffer(0x8CA9, 0);
    glDeleteFramebuffers(1, &fbo);

    // --- glDrawBuffer/glDrawBuffers/glNamedFramebufferDrawBuffer* validation
    {
        GLuint fboV = 0;
        glGenFramebuffers(1, &fboV);
        glBindFramebuffer(0x8CA9, fboV);

        DrainErr();
        glBindFramebuffer(0x9999, 1);
        Check(glGetError() == 0x0500, "glBindFramebuffer target lạ → INVALID_ENUM");
        Check(glCheckFramebufferStatus(0x9999) == 0,
              "glCheckFramebufferStatus target lạ → 0 + INVALID_ENUM");
        Check(glGetError() == 0x0500, "glCheckFramebufferStatus target lạ → INVALID_ENUM");
        glDrawBuffer(0x0404 /*FRONT*/);
        Check(glGetError() == 0x0502, "FBO + GL_FRONT → INVALID_OPERATION");
        glDrawBuffer(0x9999);
        Check(glGetError() == 0x0500, "FBO + hằng số lạ → INVALID_ENUM");
        glDrawBuffer(0x8CE8 /*COLOR_ATTACHMENT8, m >= MAX*/);
        Check(glGetError() == 0x0502, "FBO + COLOR_ATTACHMENT8 → INVALID_OPERATION");
        glDrawBuffer(0x0000 /*GL_NONE*/);
        Check(glGetError() == 0, "FBO + GL_NONE → OK");
        glDrawBuffer(0x8CE0);
        Check(glGetError() == 0, "FBO + GL_COLOR_ATTACHMENT0 → OK");

        glDrawBuffers(2, (const GLenum[]){0x8CE0, 0x8CE0});
        Check(glGetError() == 0x0502, "glDrawBuffers trùng buffer → INVALID_OPERATION");
        glDrawBuffers(9, (const GLenum[]){0,0,0,0,0,0,0,0,0});
        Check(glGetError() == 0x0501, "glDrawBuffers(9) → INVALID_VALUE");
        glDrawBuffers(2, (const GLenum[]){0x0405 /*BACK*/, 0x0000});
        Check(glGetError() == 0x0502, "FBO + BACK trong DrawBuffers → INVALID_OPERATION");
        glDrawBuffers(2, (const GLenum[]){0x0000, 0x0000});
        Check(glGetError() == 0, "glDrawBuffers NONE NONE → OK (NONE lặp lại hợp lệ)");
        glDrawBuffers(0, nullptr);
        Check(glGetError() == 0, "glDrawBuffers(0) → OK (toàn NONE)");
        // atomic: lỗi giữa chừng KHÔNG đổi state — ATT0 vẫn là draw buffer
        glDrawBuffer(0x8CE0);
        glDrawBuffers(2, (const GLenum[]){0x8CE1, 0x8CE0});
        Check(glGetError() == 0, "glDrawBuffers ATT1,ATT0 → OK");
        GLint v = 0;
        glGetIntegerv(0x0C01 /*DRAW_BUFFER*/, &v);
        Check(v == 0x8CE1, "DRAW_BUFFER == DRAW_BUFFER0 == ATT1");
        glGetIntegerv(0x8826 /*DRAW_BUFFER1*/, &v);
        Check(v == 0x8CE0, "DRAW_BUFFER1 == ATT0");
        glGetIntegerv(0x8827 /*DRAW_BUFFER2*/, &v);
        Check(v == 0x0000, "DRAW_BUFFER2 → NONE (quá n)");

        glNamedFramebufferDrawBuffer(0xDEADBEEF, 0x0000);
        Check(glGetError() == 0x0502, "named draw buffer tên lạ → INVALID_OPERATION");
        glNamedFramebufferDrawBuffers(0xDEADBEEF, 1, (const GLenum[]){0x8CE0});
        Check(glGetError() == 0x0502, "named drawBuffers tên lạ → INVALID_OPERATION");

        glBindFramebuffer(0x8CA9, 0);
        glNamedFramebufferDrawBuffer(0, 0x0405 /*BACK*/); // zero → default FB
        Check(glGetError() == 0, "named draw buffer id 0 → default FB, OK");
        glNamedFramebufferDrawBuffers(0, 1, (const GLenum[]){0x0402 /*BACK_LEFT*/});
        Check(glGetError() == 0, "named drawBuffers id 0 → default FB, OK");
        glDrawBuffers(1, (const GLenum[]){0x0405 /*BACK*/});
        Check(glGetError() == 0x0502, "default FB + BACK trong DrawBuffers → INVALID_OPERATION");
        glDrawBuffers(1, (const GLenum[]){0x0402 /*BACK_LEFT*/});
        Check(glGetError() == 0, "default FB + BACK_LEFT trong DrawBuffers → OK");
        glDrawBuffer(0x0405 /*BACK*/); // restore default FB
        Check(glGetError() == 0, "default FB + GL_BACK → OK");
        glBindFramebuffer(0x8CA9, fboV);
        glDeleteFramebuffers(1, &fboV);
        glBindFramebuffer(0x8CA9, 0);
    }

    // --- read buffer state: per-FBO + named + default FB
    // Bind qua FRAMEBUFFER (0x8D40) = bind cả DRAW và READ (đúng spec).
    {
        GLuint fboR1 = 0, fboR2 = 0;
        GLuint names[2] = {0, 0};
        glGenFramebuffers(2, names);
        fboR1 = names[0]; fboR2 = names[1];

        glBindFramebuffer(0x8D40, fboR1);
        glReadBuffer(0x8CE2 /*ATT2*/);
        Check(glGetError() == 0, "FBO1 glReadBuffer ATT2 → OK");
        glBindFramebuffer(0x8D40, fboR2);
        glReadBuffer(0x0000 /*GL_NONE*/);
        Check(glGetError() == 0, "FBO2 glReadBuffer NONE → OK");
        GLint v = 0;
        glGetIntegerv(0x0C02 /*READ_BUFFER*/, &v);
        Check(v == 0x0000, "FBO2 READ_BUFFER = NONE");
        glBindFramebuffer(0x8D40, fboR1);
        glGetIntegerv(0x0C02, &v);
        Check(v == 0x8CE2, "FBO1 READ_BUFFER = ATT2 (không rò sang FBO)");

        glReadBuffer(0x0405 /*BACK*/); // hằng số default FB trên FBO
        Check(glGetError() == 0x0502, "FBO + BACK → INVALID_OPERATION");
        glNamedFramebufferReadBuffer(fboR2, 0x8CE0);
        Check(glGetError() == 0, "named read buffer ATT0 → OK");
        glBindFramebuffer(0x8D40, fboR2);
        glGetIntegerv(0x0C02, &v);
        Check(v == 0x8CE0, "FBO2 READ_BUFFER = ATT0 (named set)");
        glNamedFramebufferReadBuffer(0xDEADBEEF, 0x0000);
        Check(glGetError() == 0x0502, "named read buffer tên lạ → INVALID_OPERATION");
        glNamedFramebufferReadBuffer(fboR2, 0x0404 /*FRONT*/);
        Check(glGetError() == 0x0502, "FBO + FRONT (named) → INVALID_OPERATION");
        glNamedFramebufferReadBuffer(fboR1, 0x8CE3 /*ATT3*/);
        Check(glGetError() == 0, "named read buffer ATT3 → OK");
        glBindFramebuffer(0x8D40, fboR1);
        glGetIntegerv(0x0C02, &v);
        Check(v == 0x8CE3, "FBO1 READ_BUFFER = ATT3 (named set, không cần bind)");
        glNamedFramebufferReadBuffer(0, 0x0000); // zero → default FB
        Check(glGetError() == 0, "named read buffer id 0 → default FB, OK");
        glNamedFramebufferReadBuffer(0, 0x0405); // restore default FB = BACK
        Check(glGetError() == 0, "named read buffer id 0 BACK → OK");

        glBindFramebuffer(0x8D40, 0);
        glReadBuffer(0x0405 /*BACK*/); // default FB luôn restore được
        Check(glGetError() == 0, "default FB read buffer BACK → OK");
        glGetIntegerv(0x0C02, &v);
        Check(v == 0x0405, "default FB READ_BUFFER = BACK");
        glDeleteFramebuffers(1, &fboR1);
        glDeleteFramebuffers(1, &fboR2);
    }

    // --- glClearBuffer* validation + no-op
    {
        GLuint fboC = 0;
        glGenFramebuffers(1, &fboC);
        glBindFramebuffer(0x8CA9, fboC);
        DrainErr();
        glDrawBuffers(1, (const GLenum[]){0x0000 /*NONE*/});
        Check(glGetError() == 0, "DrawBuffers NONE → OK");
        glClearBufferfv(0x1800 /*COLOR*/, 0, (const GLfloat[]){1, 0, 0, 1});
        Check(glGetError() == 0, "clear draw buffer NONE → no-op, không lỗi");
        glClearBufferfv(0x1800, 8, (const GLfloat[]){1, 0, 0, 1});
        Check(glGetError() == 0x0501, "glClearBufferfv d=8 → INVALID_VALUE");
        glClearBufferfv(0x9999, 0, (const GLfloat[]){1, 0, 0, 1});
        Check(glGetError() == 0x0500, "glClearBufferfv buffer lạ → INVALID_ENUM");
        glClearBufferfv(0x1802 /*STENCIL*/, 0, (const GLfloat[]){1});
        Check(glGetError() == 0, "glClearBufferfv STENCIL → OK");
        glClearBufferfi(0x9999, 0, 1.0f, 1);
        Check(glGetError() == 0x0500, "glClearBufferfi buffer lạ → INVALID_ENUM");
        glClearBufferfi(0x1801 /*DEPTH*/, 0, 1.0f, 1);
        Check(glGetError() == 0, "glClearBufferfi DEPTH → OK");
        glClearBufferfi(0x1802 /*STENCIL*/, 0, 1.0f, 1);
        Check(glGetError() == 0, "glClearBufferfi STENCIL → OK");
        glClearBufferfi(0x84F9 /*DEPTH_STENCIL*/, 0, 1.0f, 1);
        Check(glGetError() == 0, "glClearBufferfi DEPTH_STENCIL → OK");
        glBindFramebuffer(0x8CA9, 0);
        glDeleteFramebuffers(1, &fboC);
    }

    // --- default FB: DRAW_BUFFER / READ_BUFFER / DRAW_BUFFERi queries
    {
        glBindFramebuffer(0x8D40, 0);
        glDrawBuffer(0x0405 /*BACK*/);       // đưa default FB về mặc định
        glReadBuffer(0x0405);
        GLint v0 = 0, v1 = 0, v2 = 0;
        glGetIntegerv(0x0C01 /*DRAW_BUFFER*/, &v0);
        Check(v0 == 0x0405, "default FB DRAW_BUFFER = BACK");
        glGetIntegerv(0x8825 /*DRAW_BUFFER0*/, &v0);
        glGetIntegerv(0x8826 /*DRAW_BUFFER1*/, &v1);
        Check(v0 == 0x0405 && v1 == 0x0000, "default FB DRAW_BUFFER0=BACK, DRAW_BUFFER1=NONE");
        glGetIntegerv(0x0C02 /*READ_BUFFER*/, &v2);
        Check(v2 == 0x0405, "default FB READ_BUFFER = BACK");
        glReadBuffer(0x0000 /*NONE*/);
        glGetIntegerv(0x0C02, &v2);
        Check(v2 == 0x0000, "default FB glReadBuffer(NONE) → query NONE");
        glReadBuffer(0x0405);                // restore
    }

    glDeleteTextures(1, &color);
    glDeleteTextures(1, &depth);
    glDeleteTextures(1, &small);
    glDeleteTextures(1, &c1);
    glDeleteTextures(1, &arr);

    if (gFails) { printf("test_fbo_completeness FAILED (%d)\n", gFails); return 1; }
    printf("test_fbo_completeness PASS\n");
    return 0;
}
