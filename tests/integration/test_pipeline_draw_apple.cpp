// test_pipeline_draw_apple.cpp — draw through a separable VS/FS program pipeline.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_pipeline_draw_apple FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 color;\n"
    "void main(){ color = vec4(1,0,0,1); }\n";

static GLuint StageProgram(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLuint p = glCreateProgram();
    glAttachShader(p, s);
    glProgramParameteri(p, 0x8258 /*PROGRAM_SEPARABLE*/, GL_TRUE);
    glLinkProgram(p);
    return p;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_pipeline_draw_apple SKIP: no MTL device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint vsProg = StageProgram(0x8B31, kVS);
    GLuint fsProg = StageProgram(0x8B30, kFS);
    GLint lv = 0, lf = 0;
    glGetProgramiv(vsProg, 0x8B82, &lv);
    glGetProgramiv(fsProg, 0x8B82, &lf);
    Check(lv != 0 && lf != 0, "separable stage programs link");

    GLuint pipe = 0;
    glGenProgramPipelines(1, &pipe);
    glBindProgramPipeline(pipe);
    glUseProgramStages(pipe, 0x1 /*VERTEX*/, vsProg);
    glUseProgramStages(pipe, 0x2 /*FRAGMENT*/, fsProg);
    glActiveShaderProgram(pipe, fsProg);
    Check(glGetError() == GL_NO_ERROR, "pipeline setup");

    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);

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
    Check(glGetError() == GL_NO_ERROR, "pipeline draw no error");
    Check(ctx.appleStats.noProgram == 0, "pipeline draw program resolved");

    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* q = px + (32 * 64 + 32) * 4;
    Check(glGetError() == GL_NO_ERROR, "pipeline readback no error");
    Check(q[0] > 200 && q[1] < 50 && q[2] < 50, "pipeline center is red");

    glDeleteProgram(vsProg); glDeleteProgram(fsProg);
    glDeleteProgramPipelines(1, &pipe);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    if (gFails == 0) { printf("test_pipeline_draw_apple PASS\n"); return 0; }
    return 1;
}
