// test_subroutine_api.cpp — GL subroutine reflection + glUniformSubroutinesuiv.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_subroutine_api FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "subroutine vec4 ft(vec4 v);\n"
    "subroutine(ft) vec4 f1(vec4 v){ return v; }\n"
    "subroutine(ft) vec4 f2(vec4 v){ return v*2.0; }\n"
    "subroutine uniform ft myF;\n"
    "out vec4 c;\n"
    "void main(){ c = myF(vec4(1)); }\n";

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kFS, nullptr); glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint linked = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &linked);
    Check(linked == GL_TRUE, "subroutine program link");

    GLuint i1 = glGetSubroutineIndex(p, GL_FRAGMENT_SHADER, "f1");
    GLuint i2 = glGetSubroutineIndex(p, GL_FRAGMENT_SHADER, "f2");
    Check(i1 != 0xFFFFFFFFu && i2 != 0xFFFFFFFFu && i1 != i2, "subroutine indices");
    Check(glGetSubroutineIndex(p, GL_FRAGMENT_SHADER, "missing") == 0xFFFFFFFFu,
          "unknown subroutine index");
    GLint loc = glGetSubroutineUniformLocation(p, GL_FRAGMENT_SHADER, "myF");
    Check(loc >= 0, "subroutine uniform location");
    GLint stageN = 0;
    glGetProgramStageiv(p, GL_FRAGMENT_SHADER, 0x8DE5 /*ACTIVE_SUBROUTINE_UNIFORMS*/, &stageN);
    Check(stageN == 1, "program stage active subroutine uniforms");
    glGetProgramStageiv(p, GL_FRAGMENT_SHADER, 0x8DE7 /*ACTIVE_SUBROUTINES*/, &stageN);
    Check(stageN == 2, "program stage active subroutines");

    GLint n = 0;
    glGetActiveSubroutineUniformiv(p, GL_FRAGMENT_SHADER, (GLuint)loc,
                                   0x8E4A /*NUM_COMPATIBLE_SUBROUTINES*/, &n);
    Check(n == 2, "compatible subroutine count");
    GLint compat[4] = {};
    glGetActiveSubroutineUniformiv(p, GL_FRAGMENT_SHADER, (GLuint)loc,
                                   0x8E4B /*COMPATIBLE_SUBROUTINES*/, compat);
    Check((compat[0] == (GLint)i1 && compat[1] == (GLint)i2) ||
          (compat[0] == (GLint)i2 && compat[1] == (GLint)i1), "compatible list");
    char name[64] = {};
    GLsizei len = 0;
    glGetActiveSubroutineUniformName(p, GL_FRAGMENT_SHADER, (GLuint)loc, sizeof(name), &len, name);
    Check(std::string(name) == "myF", "subroutine uniform name");

    glUseProgram(p);
    GLuint global = i2;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &global);
    Check(glGetError() == GL_NO_ERROR, "set subroutine uniform");
    GLuint back = 0;
    glGetUniformSubroutineuiv(GL_FRAGMENT_SHADER, loc, &back);
    Check(back == i2, "query selected subroutine");

    // Count must equal active subroutine uniforms; unknown index is INVALID_VALUE.
    while (glGetError() != GL_NO_ERROR) {}
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 0, nullptr);
    Check(glGetError() == GL_INVALID_VALUE, "subroutine count mismatch");
    GLuint bad = 9999u;
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &bad);
    Check(glGetError() == GL_INVALID_VALUE, "subroutine index out of range");

    glDeleteProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    if (gFails == 0) { printf("test_subroutine_api: OK\n"); return 0; }
    return 1;
}
