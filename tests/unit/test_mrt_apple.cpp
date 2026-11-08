// test_mrt_apple.cpp — P2 MRT (8 attachment): định tuyến fragment output theo
// glDrawBuffers, per-attachment colorWriteMask (glColorMaski), per-attachment
// blend (glEnablei/glBlendFunci), glClearBufferfv(COLOR,d) chỉ clear 1 slot,
// glClear(COLOR) clear mọi slot, slot GL_NONE, glReadBuffer + glReadPixels theo
// READ_BUFFER (không phải luôn attachment 0).
//
// Căn cứ Iris 26.1: RenderTargets.createFramebufferWritingToMain(int[] drawBuffers)
// + ClearPassCreator (glClear theo nhóm draw buffer) + ImageClearPass
// (glClearNamedFramebufferfv COLOR slot 0). Spec: GL 4.6 §17.4 (fragment outputs
// → draw buffers), §18.2.4 (ClearBuffer*), §17.3 (per-buffer enable/mask/blend).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mrt_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

// FS 3 output: o0 = u0 (uniform), o1 = xanh lá, o2 = xanh dương.
static GLuint MakeMrtProgram() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "layout(location = 0) out vec4 o0;\n"
        "layout(location = 1) out vec4 o1;\n"
        "layout(location = 2) out vec4 o2;\n"
        "uniform vec4 u0;\n"
        "void main() { o0 = u0; o1 = vec4(0.0, 1.0, 0.0, 1.0);"
        " o2 = vec4(0.0, 0.0, 1.0, 1.0); }\n";
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
    if (!ok) printf("test_mrt_apple: shader link fail\n");
    return ok ? p : 0;
}

static GLuint MakeTex() {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    std::vector<uint8_t> px((size_t)64 * 64 * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, px.data());
    return t;
}

// Đọc pixel (x,y) của attachment slot theo GL_READ_BUFFER (spec §18.2).
static void ReadSlot(int slot, int x, int y, unsigned char out[4]) {
    out[0] = out[1] = out[2] = out[3] = 0;
    glReadBuffer((GLenum)(0x8CE0 + slot));
    glReadPixels(x, y, 1, 1, 0x1908, 0x1401, out);
}

static bool Near(int v, int want, int tol = 8) { return v >= want - tol && v <= want + tol; }

static void CheckSlot(int slot, int r, int g, int b, const char* what) {
    unsigned char p[4];
    ReadSlot(slot, 32, 32, p);
    char msg[192];
    snprintf(msg, sizeof(msg), "%s (slot %d = %d,%d,%d, want %d,%d,%d)", what, slot, p[0],
             p[1], p[2], r, g, b);
    Check(Near(p[0], r) && Near(p[1], g) && Near(p[2], b), msg);
}

int main() {
    Context apple("apple");
    Context::MakeCurrent(&apple);
    if (apple.device->isNull()) {
        printf("test_mrt_apple SKIP: no metal device\n");
        return 0;
    }

    GLuint t0 = MakeTex(), t1 = MakeTex(), t2 = MakeTex();
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40 /*GL_FRAMEBUFFER*/, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, t0, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, t1, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE2, 0x0DE1, t2, 0);
    glDrawBuffers(3, (const GLenum[]){0x8CE0, 0x8CE1, 0x8CE2});
    Check(glGetError() == 0, "setup FBO 3 attachment no error");

    GLuint prog = MakeMrtProgram();
    if (!prog) return 1;
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    {
        float v[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(v), v, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    }
    glUseProgram(prog);
    GLint u0Loc = glGetUniformLocation(prog, "u0");
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71); // DEPTH_TEST
    glDisable(0x0C11); // SCISSOR_TEST
    glDisable(0x0BE2); // BLEND toàn cục
    glColorMask(1, 1, 1, 1);

    // (1) Định tuyến: 3 fragment output → đúng 3 attachment theo glDrawBuffers.
    glUniform4f(u0Loc, 1.0f, 0.25f, 0.0f, 1.0f);
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "MRT draw no error");
    CheckSlot(0, 255, 64, 0, "output0 → att0");
    CheckSlot(1, 0, 255, 0, "output1 → att1");
    CheckSlot(2, 0, 0, 255, "output2 → att2");

    // (2) glClearBufferfv(COLOR, 1, trắng) chỉ đụng slot 1 (spec §18.2.4).
    glClearBufferfv(0x1800 /*COLOR*/, 1, (const GLfloat[]){1, 1, 1, 1});
    Check(glGetError() == 0, "clearBufferfv slot1 no error");
    CheckSlot(1, 255, 255, 255, "clearBufferfv chỉ clear slot1");
    CheckSlot(0, 255, 64, 0, "slot0 giữ nguyên sau clear slot1");
    CheckSlot(2, 0, 0, 255, "slot2 giữ nguyên sau clear slot1");

    // (3) Per-attachment mask + blend trong CÙNG 1 draw:
    //   slot1: glColorMaski tắt ghi → giữ trắng (uniform u0 đổi xanh vẫn không ghi)
    //   slot2: glEnablei(BLEND) + glBlendFunci(ONE, ONE) → dst(0.5,0,0) + src(0,0,1)
    //   slot0: ghi bình thường → u0 mới.
    glClearBufferfv(0x1800, 2, (const GLfloat[]){0.5f, 0.0f, 0.0f, 1.0f});
    CheckSlot(2, 128, 0, 0, "setup slot2 = 0.5 đỏ");
    glColorMaski(1, 0, 0, 0, 0);
    glEnablei(0x0BE2 /*BLEND*/, 2);
    glBlendFunci(2, 1 /*ONE*/, 1 /*ONE*/);
    glUniform4f(u0Loc, 0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "MRT mask/blend draw no error");
    CheckSlot(0, 0, 0, 255, "slot0 ghi u0 mới (blend tắt, mask bật)");
    CheckSlot(1, 255, 255, 255, "slot1 mask tắt → giữ trắng");
    CheckSlot(2, 128, 0, 255, "slot2 blend ONE+ONE = 0.5 đỏ + xanh");
    glColorMaski(1, 1, 1, 1, 1);
    glDisablei(0x0BE2, 2);

    // (4) glClear(COLOR) clear MỌI slot bằng clearColor chung.
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
    glClear(0x00004000);
    Check(glGetError() == 0, "glClear(COLOR) MRT no error");
    CheckSlot(0, 51, 102, 153, "glClear phủ slot0");
    CheckSlot(1, 51, 102, 153, "glClear phủ slot1");
    CheckSlot(2, 51, 102, 153, "glClear phủ slot2");

    // (5) glDrawBuffers tắt slot 0/1 (GL_NONE) → draw chỉ ghi slot2; slot0/1 giữ.
    glDrawBuffers(3, (const GLenum[]){0x0000 /*NONE*/, 0x0000 /*NONE*/, 0x8CE2});
    glUniform4f(u0Loc, 1.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "drawBuffers subset draw no error");
    CheckSlot(2, 0, 0, 255, "slot2 ghi khi chỉ DRAW_BUFFER2 bật");
    CheckSlot(0, 51, 102, 153, "slot0 NONE → không ghi");
    CheckSlot(1, 51, 102, 153, "slot1 NONE → không ghi");
    glDrawBuffers(3, (const GLenum[]){0x8CE0, 0x8CE1, 0x8CE2});

    // (6) glReadBuffer(ATT1) + glReadPixels đọc đúng slot (không phải att0).
    glClearBufferfv(0x1800, 0, (const GLfloat[]){1, 0, 0, 1});
    glClearBufferfv(0x1800, 1, (const GLfloat[]){0, 1, 0, 1});
    glClearBufferfv(0x1800, 2, (const GLfloat[]){0, 0, 1, 1});
    {
        unsigned char p[4];
        ReadSlot(1, 32, 32, p);
        Check(Near(p[0], 0) && Near(p[1], 255) && Near(p[2], 0),
              "glReadBuffer(ATT1): glReadPixels đọc slot1");
        ReadSlot(3, 32, 32, p);
        Check(p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0,
              "READ_BUFFER slot chưa attach → không đọc nhầm");
    }

    if (gFails) {
        printf("test_mrt_apple RESULT: FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_mrt_apple RESULT: PASS\n");
    return 0;
}
