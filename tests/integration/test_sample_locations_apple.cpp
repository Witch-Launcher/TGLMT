// test_sample_locations_apple.cpp — ARB_sample_locations contract + Metal path.
// Kiểm tra query mặc định của device, bảng programmable 32 entry, state FBO/default,
// và draw MSAA thật có setSamplePositions trên render pass (validation layer bắt lỗi).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

extern "C" void glFramebufferSampleLocationsfvARB(GLenum target, GLuint start,
                                                   GLsizei count, const GLfloat* v);
extern "C" void glEvaluateDepthValuesARB(void);

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_sample_locations_apple FAIL: %s\n", msg); ++gFails; }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 c;\n"
    "void main(){ c = vec4(1,0,0,1); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_sample_locations_apple SKIP: no MTL device\n");
        return 0;
    }
    while (glGetError() != GL_NO_ERROR) {}

    GLint subpixel = 0, gw = 0, gh = 0, tableSize = 0;
    glGetIntegerv(0x933D, &subpixel);
    glGetIntegerv(0x933E, &gw);
    glGetIntegerv(0x933F, &gh);
    glGetIntegerv(0x9340, &tableSize);
    Check(subpixel == 4, "SAMPLE_LOCATION_SUBPIXEL_BITS = 4");
    Check(gw == 1 && gh == 1, "pixel grid 1x1");
    Check(tableSize == 32, "PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE = 32");
    Check(ctx.device->supportsProgrammableSamplePositions(),
          "Metal reports programmable sample positions");

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x9100, tex);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 64, 64, GL_TRUE);
    Check(glGetError() == GL_NO_ERROR, "MSAA texture storage");

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x9100, tex, 0);
    Check(glCheckFramebufferStatus(0x8CA9) == 0x8CD5, "MSAA FBO complete");

    // Default sample positions: phải lấy từ Metal, không phải hằng 0.5.
    GLfloat def[8] = {};
    bool distinct = false;
    for (GLuint i = 0; i < 4; ++i) {
        glGetMultisamplefv(0x8E50, i, def + i * 2);
        Check(def[i * 2] >= 0.0f && def[i * 2] <= 1.0f &&
              def[i * 2 + 1] >= 0.0f && def[i * 2 + 1] <= 1.0f,
              "default sample location trong [0,1]");
        if (def[i * 2] != 0.5f || def[i * 2 + 1] != 0.5f) distinct = true;
    }
    Check(distinct, "default sample locations không phải toàn 0.5");

    // Programmable table: 4 vị trí tuỳ biến + enable.
    const GLfloat custom[8] = {0.1f, 0.2f, 0.3f, 0.4f, 0.6f, 0.7f, 0.8f, 0.9f};
    glFramebufferSampleLocationsfvARB(0x8D40, 0, 4, custom);
    Check(glGetError() == GL_NO_ERROR, "set sample locations");
    glFramebufferParameteri(0x8D40, 0x9342, 1);
    GLint enabled = 0;
    glGetFramebufferParameteriv(0x8D40, 0x9342, &enabled);
    Check(enabled == 1, "FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS = TRUE");
    for (GLuint i = 0; i < 4; ++i) {
        GLfloat got[2] = {};
        glGetMultisamplefv(0x9341, i, got);
        Check(got[0] == custom[i * 2] && got[1] == custom[i * 2 + 1],
              "PROGRAMMABLE_SAMPLE_LOCATION đọc lại đúng");
    }
    GLfloat unset[2] = {1, 1};
    glGetMultisamplefv(0x9341, 31, unset);
    Check(unset[0] == 0.5f && unset[1] == 0.5f, "entry chưa set mặc định 0.5");

    // Error contract.
    while (glGetError() != GL_NO_ERROR) {}
    glGetMultisamplefv(0x8E50, 4, unset);
    Check(glGetError() == 0x0501, "sample index >= samples → INVALID_VALUE");
    glGetMultisamplefv(0x1234, 0, unset);
    Check(glGetError() == 0x0500, "pname lạ → INVALID_ENUM");
    glFramebufferSampleLocationsfvARB(0x8D40, 31, 2, custom);
    Check(glGetError() == 0x0501, "table vượt 32 entry → INVALID_VALUE");

    // Draw MSAA thật qua encoder có custom sample positions.
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr); glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs); glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint linked = 0; glGetProgramiv(prog, 0x8B82, &linked);
    Check(linked != 0, "program link");
    glUseProgram(prog);

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    float tri[6] = {-1, -1, 3, -1, -1, 3};
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "MSAA draw với custom sample positions");
    glEvaluateDepthValuesARB();
    Check(glGetError() == GL_NO_ERROR, "glEvaluateDepthValuesARB");

    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    unsigned char px[64 * 64 * 4];
    std::memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    Check(glGetError() == GL_NO_ERROR, "readback no error");
    unsigned char* q = px + (32 * 64 + 32) * 4;
    Check(q[0] > 200 && q[1] < 50 && q[2] < 50, "resolved center is red");

    // Default framebuffer chỉ nhận 2 pname sample-location.
    glBindFramebuffer(0x8D40, 0);
    glFramebufferParameteri(0x8D40, 0x9342, 1);
    glGetFramebufferParameteriv(0x8D40, 0x9342, &enabled);
    Check(enabled == 1, "default FB programmable location = TRUE");
    glFramebufferSampleLocationsfvARB(0x8D40, 0, 1, custom);
    Check(glGetError() == GL_NO_ERROR, "default FB sample location table");
    GLfloat got[2] = {};
    glGetMultisamplefv(0x9341, 0, got);
    Check(got[0] == custom[0] && got[1] == custom[1], "default FB table đọc lại đúng");
    glFramebufferParameteri(0x8D40, 0x9342, 0);

    glDeleteProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    if (gFails) { printf("test_sample_locations_apple FAILED (%d)\n", gFails); return 1; }
    printf("test_sample_locations_apple PASS\n");
    return 0;
}
