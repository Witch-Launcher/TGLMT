// test_shader_program.cpp — Unit: shader compile/link/uniform/shader.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0.0); }";
    const char* fs = "#version 460 core\nout vec4 c;\nvoid main(){ c = vec4(1.0,0.0,0.0,1.0); }";
    tglmt::GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v); glCompileShader(f);
    tglmt::GLint ok = 0;
    glGetShaderiv(v, 0x8B81, &ok); assert(ok == 1); // COMPILE_STATUS
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok); assert(ok == 1); // LINK_STATUS
    glUseProgram(p);
    tglmt::GLint loc = glGetUniformLocation(p, "uColor");
    assert(loc >= 0);
    glUniform4f(loc, 1.0f, 0.0f, 0.0f, 1.0f);
    float out[4] = {0};
    glGetUniformfv(p, loc, out);
    assert(out[0] == 1.0f && out[3] == 1.0f);
    // shader hỏng (thiếu main) phải fail trung thực
    tglmt::GLuint bad = glCreateShader(0x8B31);
    const char* badsrc = "#version 460 core\n// no main";
    glShaderSource(bad, 1, &badsrc, nullptr);
    glCompileShader(bad);
    glGetShaderiv(bad, 0x8B81, &ok); assert(ok == 0);
    printf("test_shader_program PASS\n");
    return 0;
}
