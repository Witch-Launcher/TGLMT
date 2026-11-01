// test_msaa_depth_apple.cpp — MSAA color + multisample depth attachment.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_msaa_depth_apple FAIL: %s\n", msg); ++gFails; }
}
static const char* kVSRed =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p, 0.25, 1); }\n";
static const char* kVSGreen =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p, 0.75, 1); }\n";
static const char* kFSRed =
    "#version 460 core\n" "out vec4 c; void main(){ c=vec4(1,0,0,1); }\n";
static const char* kFSGreen =
    "#version 460 core\n" "out vec4 c; void main(){ c=vec4(0,1,0,1); }\n";
static GLuint Make(const char* vs, const char* fs) {
    GLuint v = glCreateShader(0x8B31); glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
    GLuint f = glCreateShader(0x8B30); glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    GLuint p = glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    return p;
}
int main() {
    Context ctx("apple"); Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_msaa_depth_apple SKIP: no MTL device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}
    GLuint pr = Make(kVSRed, kFSRed), pg = Make(kVSGreen, kFSGreen);
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    float tri[6]={-1,-1, 3,-1, -1,3};
    glGenBuffers(1,&vbo); glBindBuffer(0x8892,vbo); glBufferData(0x8892,sizeof(tri),tri,0x88E4);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,0x1406,0,8,(void*)0);

    GLuint color=0, depth=0, fbo=0;
    glGenTextures(1,&color); glBindTexture(0x9100,color);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 32, 32, GL_TRUE);
    glGenTextures(1,&depth); glBindTexture(0x0DE1,depth);
    glTexImage2D(0x0DE1, 0, 0x81A5 /*DEPTH_COMPONENT16→Depth32F*/, 32,32,0,
                 0x1902, 0x1406, nullptr);
    glGenFramebuffers(1,&fbo); glBindFramebuffer(0x8D40,fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x9100, color, 0);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "MSAA depth FBO complete");

    glViewport(0,0,32,32);
    glEnable(0x0B71); glDepthFunc(0x0201 /*LESS*/);
    glClearColor(0,0,1,1); glClearDepth(1.0); glClear(0x4000|0x100);
    glUseProgram(pr); glDrawArrays(0x0004,0,3);
    glUseProgram(pg); glDrawArrays(0x0004,0,3);
    Check(glGetError() == GL_NO_ERROR, "MSAA depth draws");
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();
    unsigned char px[4]={}; glReadPixels(16,16,1,1,0x1908,0x1401,px);
    Check(px[0] > 200 && px[1] < 50, "near red survives farther green");

    glDeleteProgram(pr); glDeleteProgram(pg);
    glDeleteFramebuffers(1,&fbo); glDeleteTextures(1,&depth); glDeleteTextures(1,&color);
    if (gFails==0) { printf("test_msaa_depth_apple PASS\n"); return 0; }
    return 1;
}
