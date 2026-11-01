// test_frag_bind_apple.cpp — glBindFragDataLocation on a GPU draw: a single
// `out vec4` with no layout must be rewritten to the bound attachment.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_frag_bind_apple FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 color;\n"
    "void main(){ color = vec4(1,0,0,1); }\n";

static GLuint MakeTex(int w, int h) {
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    glTexImage2D(0x0DE1, 0, 0x8058, w, h, 0, 0x1908, 0x1401, nullptr);
    return t;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_frag_bind_apple SKIP: no MTL device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr); glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs); glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glBindFragDataLocation(prog, 1, "color");
    glLinkProgram(prog);
    GLint linked = 0, loc = -1;
    glGetProgramiv(prog, 0x8B82, &linked);
    loc = glGetFragDataLocation(prog, "color");
    Check(linked != 0, "program link");
    Check(loc == 1, "reflected location = 1");
    glUseProgram(prog);

    GLuint t0 = MakeTex(64, 64), t1 = MakeTex(64, 64);
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, t0, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, t1, 0);
    GLenum bufs[2] = {0x8CE0, 0x8CE1};
    glDrawBuffers(2, bufs);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    float tri[6] = {-1,-1, 3,-1, -1,3};
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "draw no GL error");
    Check(ctx.appleStats.noPipeline == 0, "pipeline built");

    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    glReadBuffer(0x8CE1);
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* q = px + (32 * 64 + 32) * 4;
    Check(glGetError() == GL_NO_ERROR, "readback no GL error");
    Check(q[0] > 200 && q[1] < 50 && q[2] < 50, "attachment 1 is red");

    glDeleteProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &t0); glDeleteTextures(1, &t1);
    if (gFails == 0) { printf("test_frag_bind_apple PASS\n"); return 0; }
    return 1;
}
