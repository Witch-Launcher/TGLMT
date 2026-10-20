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
    const char* fs = "#version 460 core\nout vec4 c;\nuniform vec4 uColor;\nvoid main(){ c = uColor; }";
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
    // Regression: attribute ACTIVE không được app bind (iris_Entity — EntityPatcher
    // inject, format POSITION_TEX của TEXTURED không có element Entity) phải nhận
    // index TRỐNG nhỏ nhất (2), không được đè index đã bind iris_Position@0 —
    // bug "attribute location aliasing: iris_Position and iris_Entity" (CR+DH).
    tglmt::GLuint v2 = glCreateShader(0x8B31);
    const char* vs2 = "#version 460 core\n"
                      "in vec3 iris_Position;\n"
                      "in vec2 iris_UV0;\n"
                      "in ivec3 iris_Entity;\n"
                      "out vec2 vUV;\n"
                      "void main(){ vUV = iris_UV0 + vec2(iris_Entity.xy); gl_Position = vec4(iris_Position, 1.0); }";
    glShaderSource(v2, 1, &vs2, nullptr);
    glCompileShader(v2);
    glGetShaderiv(v2, 0x8B81, &ok); assert(ok == 1);
    tglmt::GLuint f2 = glCreateShader(0x8B30);
    const char* fs2 = "#version 460 core\nin vec2 vUV;\nout vec4 c;\nvoid main(){ c = vec4(vUV, 0.0, 1.0); }";
    glShaderSource(f2, 1, &fs2, nullptr);
    glCompileShader(f2);
    tglmt::GLuint p2 = glCreateProgram();
    glAttachShader(p2, v2); glAttachShader(p2, f2);
    glBindAttribLocation(p2, 0, "iris_Position");
    glBindAttribLocation(p2, 1, "iris_UV0");
    glLinkProgram(p2);
    glGetProgramiv(p2, 0x8B82, &ok);
    assert(ok == 1);
    assert(glGetAttribLocation(p2, "iris_Position") == 0);
    assert(glGetAttribLocation(p2, "iris_UV0") == 1);
    assert(glGetAttribLocation(p2, "iris_Entity") == 2);
    // attribFill (kiểu attribute cho fill-zero AppleDrawGL): đủ 3, đúng
    // loc/comps/cls — float=0, int=1. Thứ tự = thứ tự khai báo VS.
    // iris_Entity: cls=2 (UINT) vì MSL shim khai uint4 cho format UShort4
    // (Metal GPU Apple từ chối UShort4→signed int) — fill phải cùng dấu.
    {
        auto& po = tglmt::Context::Current().programs[p2];
        assert(po.attribFill.size() == 3);
        assert(po.attribFill[0].loc == 0 && po.attribFill[0].comps == 3 && po.attribFill[0].cls == 0);
        assert(po.attribFill[1].loc == 1 && po.attribFill[1].comps == 2 && po.attribFill[1].cls == 0);
        assert(po.attribFill[2].loc == 2 && po.attribFill[2].comps == 3 && po.attribFill[2].cls == 2);
    }
    printf("test_shader_program PASS\n");
    return 0;
}
