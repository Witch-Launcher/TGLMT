// test_stencil_apple.cpp — stencil test/op on combined Depth24Stencil8.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_stencil_apple FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFSRed =
    "#version 460 core\n"
    "out vec4 color; void main(){ color = vec4(1,0,0,1); }\n";
static const char* kFSGreen =
    "#version 460 core\n"
    "out vec4 color; void main(){ color = vec4(0,1,0,1); }\n";
static GLuint Make(const char* fs) {
    GLuint v = glCreateShader(0x8B31);
    glShaderSource(v, 1, &kVS, nullptr); glCompileShader(v);
    GLuint f = glCreateShader(0x8B30);
    glShaderSource(f, 1, &fs, nullptr); glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    return p;
}
static void Pixel(Context& ctx, unsigned char* px) {
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, px);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_stencil_apple SKIP: no MTL device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint red = Make(kFSRed), green = Make(kFSGreen);
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    float tri[6] = {-1,-1, 3,-1, -1,3};
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);

    GLuint tex = 0, ds = 0, fbo = 0;
    glGenTextures(1, &tex); glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, nullptr);
    glGenTextures(1, &ds); glBindTexture(0x0DE1, ds);
    glTexImage2D(0x0DE1, 0, 0x88F0 /*DEPTH24_STENCIL8*/, 16, 16, 0,
                 0x84F9 /*DEPTH_STENCIL*/, 0x84FA /*UNSIGNED_INT_24_8*/, nullptr);
    glGenFramebuffers(1, &fbo); glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    glFramebufferTexture2D(0x8D40, 0x84F9, 0x0DE1, ds, 0);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");

    glViewport(0, 0, 16, 16);
    glDisable(0x0B71); // depth off
    glEnable(0x0B90);  // stencil on
    glClearColor(0, 0, 1, 1);
    glClearStencil(0);
    glClear(0x00004000 | 0x00000400);
    Check(glGetError() == GL_NO_ERROR, "clear color+stencil");

    glUseProgram(red);
    glStencilMask(0xFF);
    glStencilFunc(0x0207 /*ALWAYS*/, 1, 0xFF);
    glStencilOp(0x1E00 /*KEEP*/, 0x1E00, 0x1E01 /*REPLACE*/);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "stencil write draw");
    unsigned char px[4] = {};
    Pixel(ctx, px);
    Check(px[0] > 200 && px[1] < 50, "first draw red");

    glUseProgram(green);
    glStencilFunc(0x0202 /*EQUAL*/, 0, 0xFF);
    glStencilOp(0x1E00, 0x1E00, 0x1E00);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "stencil reject draw");
    Pixel(ctx, px);
    Check(px[0] > 200 && px[1] < 50, "EQUAL 0 rejected, still red");

    glStencilFunc(0x0202 /*EQUAL*/, 1, 0xFF);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "stencil pass draw");
    Pixel(ctx, px);
    Check(px[1] > 200 && px[0] < 50, "EQUAL 1 passes, green");

    glDeleteProgram(red); glDeleteProgram(green);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &ds); glDeleteTextures(1, &tex);
    if (gFails == 0) { printf("test_stencil_apple PASS\n"); return 0; }
    return 1;
}
