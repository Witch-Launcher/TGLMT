// test_validate_program.cpp — Unit: glValidateProgram/GL_VALIDATE_STATUS truth.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_validate_program FAIL: %s\n", msg); ++gFails; }
}

static const char* kVS =
    "#version 460 core\n"
    "void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location=0) out vec4 color;\n"
    "void main() { color = vec4(1.0); }\n";

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint empty = glCreateProgram();
    glValidateProgram(empty);
    GLint status = -1;
    glGetProgramiv(empty, 0x8B83 /*VALIDATE_STATUS*/, &status);
    Check(status == 0, "unlinked program is not valid");
    glDeleteProgram(empty);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint linked = 0;
    glGetProgramiv(prog, 0x8B82 /*LINK_STATUS*/, &linked);
    Check(linked == 1, "program links");
    glValidateProgram(prog);
    glGetProgramiv(prog, 0x8B83, &status);
    Check(status == 1, "linked program validates");
    glDeleteProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint n = 0;
    glGetIntegerv(0x821D /*NUM_EXTENSIONS*/, &n);
    bool advertised = false;
    for (GLint i = 0; i < n; ++i) {
        const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
        if (e && std::string(e) == "GL_ARB_shader_objects") advertised = true;
    }
    Check(advertised, "ARB_shader_objects advertised after validate fix");

    return gFails ? 1 : 0;
}
