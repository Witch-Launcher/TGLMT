// test_restart_xfb.cpp — Primitive restart, XFB state machine, FBO/EBO/unit fixes.
#include "tglmt/gl46.h"
#include "test_util.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    // --- primitive restart: strip [0,1,FFFF,2,3] → 2 draws ---
    tglmt::GLushort idx[] = {0, 1, 0xFFFF, 2, 3};
    glEnable(0x8F9D); // PRIMITIVE_RESTART
    glPrimitiveRestartIndex(0xFFFF);
    ctx.device->clearTrace();
    glDrawElements(0x0005, 5, 0x1403, idx); // TRIANGLE_STRIP, UNSIGNED_SHORT
    TGLMT_CHECK((ctx.device->drawTrace().size() == 2), "ctx.device->drawTrace().size() == 2");
    TGLMT_CHECK((ctx.device->drawTrace()[0].count == 2), "ctx.device->drawTrace()[0].count == 2");
    TGLMT_CHECK((ctx.device->drawTrace()[1].count == 2), "ctx.device->drawTrace()[1].count == 2");
    // fixed-index (không custom): 0xFFFFFFFF cho UINT
    glDisable(0x8F9D);
    glEnable(0x8D69); // PRIMITIVE_RESTART_FIXED_INDEX
    tglmt::GLuint idx32[] = {0, 1, 0xFFFFFFFFu, 2, 3, 4};
    ctx.device->clearTrace();
    glDrawElements(0x0005, 6, 0x1405, idx32);
    TGLMT_CHECK((ctx.device->drawTrace().size() == 2), "ctx.device->drawTrace().size() == 2");
    TGLMT_CHECK((ctx.device->drawTrace()[1].count == 3), "ctx.device->drawTrace()[1].count == 3");
    // tắt restart → 1 draw nguyên
    glDisable(0x8D69);
    ctx.device->clearTrace();
    glDrawElements(0x0005, 5, 0x1403, idx);
    TGLMT_CHECK((ctx.device->drawTrace().size() == 1), "ctx.device->drawTrace().size() == 1");
    TGLMT_CHECK((ctx.device->drawTrace()[0].count == 5), "ctx.device->drawTrace()[0].count == 5");
    TGLMT_CHECK((glGetError() == 0), "glGetError() == 0");

    // --- XFB state machine + capture count ---
    tglmt::GLuint x;
    glGenTransformFeedbacks(1, &x);
    glBindTransformFeedback(0x8E22, x);
    tglmt::GLuint q = 0;
    glGenQueries(1, &q);
    glBeginQuery(0x8C88 /*TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN*/, q);
    glBeginTransformFeedback(0x0004); // TRIANGLES
    tglmt::GLint av = 0;
    glGetTransformFeedbackiv(x, 0x8E24, &av); // ACTIVE
    TGLMT_CHECK((av == 1), "av == 1");
    ctx.device->clearTrace();
    glDrawArrays(0x0004, 0, 3);
    glDrawArrays(0x0004, 0, 4);
    glPauseTransformFeedback();
    glGetTransformFeedbackiv(x, 0x8E23, &av); // PAUSED
    TGLMT_CHECK((av == 1), "av == 1");
    glDrawArrays(0x0004, 0, 9); // paused → không đếm
    glResumeTransformFeedback();
    glEndTransformFeedback();
    glEndQuery(0x8C88);
    TGLMT_CHECK((ctx.xfbs[x].capturedCount == 7), "ctx.xfbs[x].capturedCount == 7");
    tglmt::GLuint primsWritten = 0;
    glGetQueryObjectuiv(q, 0x8866 /*QUERY_RESULT*/, &primsWritten);
    TGLMT_CHECK((primsWritten == 2), "primsWritten == 2");
    glDeleteQueries(1, &q);
    // Draw mode không tương thích với primitive mode của XFB → INVALID_OPERATION
    glBeginTransformFeedback(0x0001); // LINES
    ctx.device->clearTrace();
    glDrawArrays(0x0004, 0, 3); // TRIANGLES: không hợp lệ
    TGLMT_CHECK((glGetError() == 0x0502), "glGetError() == 0x0502");
    TGLMT_CHECK((ctx.device->drawTrace().empty()), "ctx.device->drawTrace().empty()");
    glDrawArrays(0x0001, 0, 4); // LINES: hợp lệ
    TGLMT_CHECK((ctx.device->drawTrace().size() == 1), "ctx.device->drawTrace().size() == 1");
    glEndTransformFeedback();
    TGLMT_CHECK((ctx.xfbs[x].capturedCount == 4), "ctx.xfbs[x].capturedCount == 4");
    ctx.device->clearTrace();
    glDrawTransformFeedback(0x0004, x);
    TGLMT_CHECK((ctx.device->drawTrace().size() == 1), "ctx.device->drawTrace().size() == 1");
    TGLMT_CHECK((ctx.device->drawTrace()[0].count == 4), "ctx.device->drawTrace()[0].count == 4");
    // varyings roundtrip
    tglmt::GLuint p = glCreateProgram();
    const char* vars[2] = {"gl_Position", "vColor"};
    glTransformFeedbackVaryings(p, 2, vars, 0x8C8C);
    char name[64] = {0}; tglmt::GLsizei len = 0;
    glGetTransformFeedbackVarying(p, 1, 64, &len, nullptr, nullptr, name);
    TGLMT_CHECK((strcmp(name, "vColor") == 0 && len == 6), "strcmp(name, \"vColor\") == 0 && len == 6");

    // --- FBO attach đúng bound object ---
    tglmt::GLuint fa, fb, t;
    glGenFramebuffers(1, &fa);
    glGenFramebuffers(1, &fb);
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    unsigned char px[16] = {0};
    glPixelStorei(0x0CF2, 1);
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px);
    glBindFramebuffer(0x8D40, fa);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, t, 0);
    TGLMT_CHECK((ctx.fbos[fa].colorTex[0] == t), "ctx.fbos[fa].colorTex[0] == t");
    TGLMT_CHECK((ctx.fbos[fb].colorTex.count(0) == 0), "ctx.fbos[fb].colorTex.count(0) == 0");
    TGLMT_CHECK((glCheckFramebufferStatus(0x8D40) == 0x8CD5), "glCheckFramebufferStatus(0x8D40) == 0x8CD5");
    glBindFramebuffer(0x8D40, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, t, 0);
    TGLMT_CHECK((glGetError() == 0x0502), "glGetError() == 0x0502"); // attach vào default FB → INVALID_OPERATION

    // --- EBO là state của VAO ---
    tglmt::GLuint vao, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(0x8893, ebo); // ELEMENT_ARRAY_BUFFER
    TGLMT_CHECK((ctx.vaos[vao].elementBuffer == ebo), "ctx.vaos[vao].elementBuffer == ebo");

    // --- Map/Unmap đồng bộ shadow ---
    tglmt::GLuint b;
    glGenBuffers(1, &b);
    glBindBuffer(0x8892, b);
    float v[2] = {1, 2};
    glBufferData(0x8892, sizeof(v), v, 0x88E4);
    float* m = (float*)glMapBuffer(0x8892, 0x88B9);
    TGLMT_CHECK((m != nullptr), "m != nullptr");
    m[0] = 7.0f;
    TGLMT_CHECK((glUnmapBuffer(0x8892) == 1), "glUnmapBuffer(0x8892) == 1");
    float back[2] = {0};
    glGetBufferSubData(0x8892, 0, sizeof(back), back);
    TGLMT_CHECK((back[0] == 7.0f && back[1] == 2.0f), "back[0] == 7.0f && back[1] == 2.0f");

    // --- texture unit isolation ---
    tglmt::GLuint t0, t1;
    glGenTextures(1, &t0);
    glGenTextures(1, &t1);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, t0);
    glActiveTexture(0x84C1);
    glBindTexture(0x0DE1, t1);
    unsigned char img[16];
    memset(img, 0xAB, 16);
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, img);
    TGLMT_CHECK((ctx.textures[t1].pixels[0] == 0xAB), "ctx.textures[t1].pixels[0] == 0xAB");
    TGLMT_CHECK((ctx.textures[t0].pixels.empty()), "ctx.textures[t0].pixels.empty()");
    printf("test_restart_xfb PASS\n");
    return TGLMT_RESULT;
}
