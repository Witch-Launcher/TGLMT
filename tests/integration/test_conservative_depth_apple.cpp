// test_conservative_depth_apple.cpp — ARB_conservative_depth layout qualifiers.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_conservative_depth_apple FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position=vec4(p,0,1); }\n";
static const char* kFSGreater =
    "#version 460 core\n"
    "layout(depth_greater) out float gl_FragDepth;\n"
    "out vec4 c;\n"
    "void main(){ gl_FragDepth=0.25; c=vec4(1,0,0,1); }\n";
static const char* kFSLess =
    "#version 460 core\n"
    "layout(depth_less) out float gl_FragDepth;\n"
    "out vec4 c;\n"
    "void main(){ gl_FragDepth=0.50; c=vec4(0,1,0,1); }\n";
static const char* kFSUnchanged =
    "#version 460 core\n"
    "layout(depth_unchanged) out float gl_FragDepth;\n"
    "out vec4 c;\n"
    "void main(){ gl_FragDepth=0.75; c=vec4(0,0,1,1); }\n";
static GLuint Make(const char* fs) {
    GLuint v = glCreateShader(0x8B31); glShaderSource(v,1,&kVS,nullptr); glCompileShader(v);
    GLuint f = glCreateShader(0x8B30); glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    GLint okv=0, okf=0; glGetShaderiv(v,0x8B81,&okv); glGetShaderiv(f,0x8B81,&okf);
    Check(okv && okf, "shader compile (conservative depth)");
    GLuint p = glCreateProgram(); glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    GLint okp=0; glGetProgramiv(p,0x8B82,&okp); Check(okp, "program link (conservative depth)");
    return p;
}
int main() {
    Context ctx("apple"); Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_conservative_depth_apple SKIP: no MTL device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}

    GLint nExt = 0; glGetIntegerv(0x821D, &nExt);
    bool haveExt = false;
    for (GLint i = 0; i < nExt; ++i) {
        const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
        if (e && strcmp(e, "GL_ARB_conservative_depth") == 0) haveExt = true;
    }
    Check(haveExt, "extension advertised");

    GLuint pg = Make(kFSGreater), pl = Make(kFSLess), pu = Make(kFSUnchanged);
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glBindVertexArray(vao);
    float tri[6]={-1,-1, 3,-1, -1,3};
    glGenBuffers(1,&vbo); glBindBuffer(0x8892,vbo); glBufferData(0x8892,sizeof(tri),tri,0x88E4);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,0x1406,0,8,(void*)0);

    GLuint color=0, depth=0, fbo=0;
    glGenTextures(1,&color); glBindTexture(0x0DE1,color);
    glTexImage2D(0x0DE1,0,0x8058,32,32,0,0x1908,0x1401,nullptr);
    glGenTextures(1,&depth); glBindTexture(0x0DE1,depth);
    glTexImage2D(0x0DE1,0,0x81A5,32,32,0,0x1902,0x1406,nullptr);
    glGenFramebuffers(1,&fbo); glBindFramebuffer(0x8D40,fbo);
    glFramebufferTexture2D(0x8D40,0x8CE0,0x0DE1,color,0);
    glFramebufferTexture2D(0x8D40,0x8D00,0x0DE1,depth,0);
    Check(glCheckFramebufferStatus(0x8D40)==0x8CD5, "depth FBO complete");

    glViewport(0,0,32,32); glEnable(0x0B71); glDepthFunc(0x0201);
    glClearColor(0,0,0,1); glClearDepth(1.0);
    unsigned char px[4]={};
    struct Step { GLuint p; float r,g,b; const char* name; };
    Step steps[] = {
        {pg, 1,0,0, "depth_greater"},
        {pl, 0,1,0, "depth_less"},
        {pu, 0,0,1, "depth_unchanged"},
    };
    for (auto& st : steps) {
        glClear(0x4000|0x100);
        glUseProgram(st.p); glDrawArrays(0x0004,0,3);
        Check(glGetError()==GL_NO_ERROR, st.name);
        ctx.FlushPendingEncoder(); ctx.CommitAndWait();
        glReadPixels(16,16,1,1,0x1908,0x1401,px);
        Check(px[0] == (unsigned char)(st.r*255) &&
              px[1] == (unsigned char)(st.g*255) &&
              px[2] == (unsigned char)(st.b*255), st.name);
    }
    glDeleteProgram(pg); glDeleteProgram(pl); glDeleteProgram(pu);
    glDeleteFramebuffers(1,&fbo); glDeleteTextures(1,&depth); glDeleteTextures(1,&color);
    if (gFails==0) { printf("test_conservative_depth_apple PASS\n"); return 0; }
    return 1;
}
