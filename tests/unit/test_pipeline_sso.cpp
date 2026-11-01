// test_pipeline_sso.cpp — contract tests for GL_ARB_separate_shader_objects.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_pipeline_sso FAIL: %s\n", msg); ++gFails; }
}
static void ClearErrors() { while (glGetError() != GL_NO_ERROR) {} }

static GLuint Compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("compile error: %s\n", log);
    }
    return s;
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "out vec4 v;\n"
    "void main(){ v = vec4(p,0,1); gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec4 v;\n"
    "out vec4 c;\n"
    "void main(){ c = v; }\n";

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    ClearErrors();

    GLuint vs = Compile(GL_VERTEX_SHADER, kVS);
    GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS);
    Check(vs && fs, "shader compile");
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glProgramParameteri(prog, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glLinkProgram(prog);
    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    Check(linked == GL_TRUE, "separable program link");

    GLuint pipe = 0;
    glGenProgramPipelines(1, &pipe);
    Check(pipe != 0 && glIsProgramPipeline(pipe), "gen pipeline");
    glBindProgramPipeline(pipe);
    glUseProgramStages(pipe, GL_VERTEX_SHADER_BIT, prog);
    glUseProgramStages(pipe, GL_FRAGMENT_SHADER_BIT, prog);
    Check(glGetError() == GL_NO_ERROR, "use program stages");
    glUseProgramStages(pipe, 0xFFFFFFFFu /*ALL_SHADER_BITS*/, prog);
    Check(glGetError() == GL_NO_ERROR, "use all shader bits");
    GLint sep = 0;
    glGetProgramiv(prog, 0x8258, &sep);
    Check(sep == 1, "PROGRAM_SEPARABLE query");
    glActiveShaderProgram(pipe, prog);
    Check(glGetError() == GL_NO_ERROR, "active shader program");
    GLint v = -1;
    glGetProgramPipelineiv(pipe, GL_ACTIVE_PROGRAM, &v);
    Check(v == (GLint)prog, "pipeline active program query");
    glGetProgramPipelineiv(pipe, GL_VERTEX_SHADER, &v);
    Check(v == (GLint)prog, "pipeline VS query");
    glGetProgramPipelineiv(pipe, GL_FRAGMENT_SHADER, &v);
    Check(v == (GLint)prog, "pipeline FS query");
    glValidateProgramPipeline(pipe);
    GLint valid = 0;
    glGetProgramPipelineiv(pipe, GL_VALIDATE_STATUS, &valid);
    Check(valid == GL_TRUE, "pipeline validate status");
    Check(glGetError() == GL_NO_ERROR, "pipeline validate no error");

    // Error paths.
    ClearErrors();
    glUseProgramStages(pipe, 0x80000000u, prog);
    Check(glGetError() == GL_INVALID_VALUE, "invalid stage bit");
    ClearErrors();
    glActiveShaderProgram(pipe, 99999);
    Check(glGetError() == GL_INVALID_OPERATION, "invalid active program");
    ClearErrors();
    glBindProgramPipeline(99999);
    Check(glGetError() == GL_INVALID_OPERATION, "invalid pipeline binding");

    // A non-separable program must be rejected by glUseProgramStages.
    GLuint plain = glCreateProgram();
    glAttachShader(plain, vs);
    glAttachShader(plain, fs);
    glLinkProgram(plain);
    ClearErrors();
    glUseProgramStages(pipe, GL_VERTEX_SHADER_BIT, plain);
    Check(glGetError() == GL_INVALID_OPERATION, "non-separable stage program rejected");

    glDeleteProgram(plain);
    glDeleteProgramPipelines(1, &pipe);
    glDeleteProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (gFails == 0) { printf("test_pipeline_sso: OK\n"); return 0; }
    return 1;
}
