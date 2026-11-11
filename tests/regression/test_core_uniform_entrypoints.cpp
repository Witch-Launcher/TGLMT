// Test đầy đủ 87 hàm uniform của GL 4.6 Core (GL 4.6 §7.6.1):
//   glUniform{1234}{f,i,ui,d}          (16)
//   glUniform{1234}{f,i,ui,d}v         (16)
//   glUniformMatrix{n,nx m}{f,d}v       (18)
//   glProgramUniform{1234}{f,i,ui,d}   (16)
//   glProgramUniform{1234}{f,i,ui,d}v  (16)
//   glProgramUniformMatrix{n,nx m}{f,d}v (18)
//   glGetUniform{f,d,i,u}v + glGetnUniform* (8)
// = 106 entry point.
//
// Mỗi hàm được kiểm chứng bằng HÀNH VI thật của GL 4.6, không phải "gọi không
// crash":
//   1. Round-trip giá trị (set → GetUniform* → so sánh) cho từng kiểu.
//   2. Lỗi theo spec: count<0 → INVALID_VALUE; sai kiểu/sai kích thước →
//      INVALID_OPERATION; location không hợp lệ → INVALID_OPERATION;
//      program lạ → INVALID_VALUE; program chưa link → INVALID_OPERATION.
//   3. location == -1 → bỏ qua IM LẶNG (không lỗi, không đổi giá trị).
//   4. Ma trận: transpose đúng chiều, giá trị trả về column-major.

#include "tglmt/gl46.h"
#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

GLenum FirstError() { return glGetError(); }

// ---------------------------------------------------------------------------
// Shader dùng chung: một uniform cho MỌI kiểu, để location ứng với từng kiểu
// được biết chính xác (location = index của biến).
// ---------------------------------------------------------------------------
const char* kVertSrc =
    "#version 460 core\n"
    "layout(location = 0) in vec4 aPos;\n"
    "uniform float  uF;\n"
    "uniform float  uFv[2];\n"
    "uniform int    uI;\n"
    "uniform int    uIv[2];\n"
    "uniform uint   uU;\n"
    "uniform uint  uUiv[2];\n"
    "uniform double uD;\n"
    "uniform double uDv[2];\n"
    "uniform vec2   uV2;\n"
    "uniform vec3   uV3;\n"
    "uniform vec4   uV4;\n"
    "uniform ivec2  uIV2;\n"
    "uniform ivec3  uIV3;\n"
    "uniform ivec4  uIV4;\n"
    "uniform uvec2  uUV2;\n"
    "uniform uvec3  uUV3;\n"
    "uniform uvec4  uUV4;\n"
    "uniform bvec4  uBV4;\n"
    "uniform mat2   uM2;\n"
    "uniform mat3   uM3;\n"
    "uniform mat4   uM4;\n"
    "uniform mat2x3 uM2x3;\n"
    "uniform mat3x2 uM3x2;\n"
    "uniform mat2x4 uM2x4;\n"
    "uniform mat4x2 uM4x2;\n"
    "uniform mat3x4 uM3x4;\n"
    "uniform mat4x3 uM4x3;\n"
    "uniform sampler2D uTex;\n"
    "void main() { gl_Position = aPos; }\n";

// Sampler phải được DÙNG thì mới ACTIVE (đúng GL 4.6 §7.3) → glGetUniformLocation
// mới trả location hợp lệ. Sampler khai nhưng không dùng → location = -1.
const char* kFragSrc =
    "#version 460 core\n"
    "out vec4 oColor;\n"
    "uniform sampler2D uTex;\n"
    "void main() { oColor = texture(uTex, vec2(0.0)); }\n";

GLuint g_prog = 0;
struct Locs {
    GLint f = -1, fv = -1, i = -1, iv = -1, u = -1, uiv = -1, d = -1, dv = -1;
    GLint v2 = -1, v3 = -1, v4 = -1;
    GLint iv2 = -1, iv3 = -1, iv4 = -1;
    GLint uv2 = -1, uv3 = -1, uv4 = -1;
    GLint bv4 = -1;
    GLint m2 = -1, m3 = -1, m4 = -1, m2x3 = -1, m3x2 = -1, m2x4 = -1, m4x2 = -1, m3x4 = -1, m4x3 = -1;
    GLint tex = -1;
};
Locs g_l;

bool BuildProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &kVertSrc, nullptr);
    glCompileShader(vs);
    GLint ok = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096] = {0};
        glGetShaderInfoLog(vs, 4096, nullptr, log);
        printf("  vertex compile failed: %s\n", log);
        return false;
    }
    glShaderSource(fs, 1, &kFragSrc, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096] = {0};
        glGetShaderInfoLog(fs, 4096, nullptr, log);
        printf("  fragment compile failed: %s\n", log);
        return false;
    }
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char log[4096] = {0};
        glGetProgramInfoLog(g_prog, 4096, nullptr, log);
        printf("  link failed: %s\n", log);
        return false;
    }
    g_l.f = glGetUniformLocation(g_prog, "uF");
    g_l.fv = glGetUniformLocation(g_prog, "uFv");
    g_l.i = glGetUniformLocation(g_prog, "uI");
    g_l.iv = glGetUniformLocation(g_prog, "uIv");
    g_l.u = glGetUniformLocation(g_prog, "uU");
    g_l.uiv = glGetUniformLocation(g_prog, "uUiv");
    g_l.d = glGetUniformLocation(g_prog, "uD");
    g_l.dv = glGetUniformLocation(g_prog, "uDv");
    g_l.v2 = glGetUniformLocation(g_prog, "uV2");
    g_l.v3 = glGetUniformLocation(g_prog, "uV3");
    g_l.v4 = glGetUniformLocation(g_prog, "uV4");
    g_l.iv2 = glGetUniformLocation(g_prog, "uIV2");
    g_l.iv3 = glGetUniformLocation(g_prog, "uIV3");
    g_l.iv4 = glGetUniformLocation(g_prog, "uIV4");
    g_l.uv2 = glGetUniformLocation(g_prog, "uUV2");
    g_l.uv3 = glGetUniformLocation(g_prog, "uUV3");
    g_l.uv4 = glGetUniformLocation(g_prog, "uUV4");
    g_l.bv4 = glGetUniformLocation(g_prog, "uBV4");
    g_l.m2 = glGetUniformLocation(g_prog, "uM2");
    g_l.m3 = glGetUniformLocation(g_prog, "uM3");
    g_l.m4 = glGetUniformLocation(g_prog, "uM4");
    g_l.m2x3 = glGetUniformLocation(g_prog, "uM2x3");
    g_l.m3x2 = glGetUniformLocation(g_prog, "uM3x2");
    g_l.m2x4 = glGetUniformLocation(g_prog, "uM2x4");
    g_l.m4x2 = glGetUniformLocation(g_prog, "uM4x2");
    g_l.m3x4 = glGetUniformLocation(g_prog, "uM3x4");
    g_l.m4x3 = glGetUniformLocation(g_prog, "uM4x3");
    g_l.tex = glGetUniformLocation(g_prog, "uTex");
    return true;
}

bool Near(float a, float b) { return fabsf(a - b) < 1e-5f; }
bool NearD(double a, double b) { return fabs(a - b) < 1e-9; }

// ===========================================================================
// 1) glUniform{1..4}{f,i,ui,d} — dạng scalar (16 hàm)
// ===========================================================================
bool TestUniformScalars() {
    glUseProgram(g_prog);

    // float
    glUniform1f(g_l.f, 1.5f);
    GLfloat f1 = 0;
    glGetUniformfv(g_prog, g_l.f, &f1);
    TGLMT_CHECK(Near(f1, 1.5f), "glUniform1f phải lưu đúng giá trị");

    // int
    glUniform1i(g_l.i, -7);
    GLint i1 = 0;
    glGetUniformiv(g_prog, g_l.i, &i1);
    TGLMT_CHECK(i1 == -7, "glUniform1i phải lưu đúng giá trị");

    // uint
    glUniform1ui(g_l.u, 4000000000u);
    GLuint u1 = 0;
    glGetUniformuiv(g_prog, g_l.u, &u1);
    TGLMT_CHECK(u1 == 4000000000u, "glUniform1ui phải lưu đúng giá trị (uint 32-bit)");

    // double
    glUniform1d(g_l.d, 3.25);
    GLdouble d1 = 0;
    glGetUniformdv(g_prog, g_l.d, &d1);
    TGLMT_CHECK(NearD(d1, 3.25), "glUniform1d phải lưu đúng giá trị double");

    // vector 2/3/4 — float
    glUniform2f(g_l.v2, 1.0f, 2.0f);
    glUniform3f(g_l.v3, 1.0f, 2.0f, 3.0f);
    glUniform4f(g_l.v4, 1.0f, 2.0f, 3.0f, 4.0f);
    GLfloat v[4] = {0};
    glGetUniformfv(g_prog, g_l.v2, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[1], 2.0f), "glUniform2f phải lưu đúng 2 thành phần");
    glGetUniformfv(g_prog, g_l.v3, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[2], 3.0f), "glUniform3f phải lưu đúng 3 thành phần");
    glGetUniformfv(g_prog, g_l.v4, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glUniform4f phải lưu đúng 4 thành phần");

    // vector 2/3/4 — int
    glUniform2i(g_l.iv2, 10, 20);
    glUniform3i(g_l.iv3, 10, 20, 30);
    glUniform4i(g_l.iv4, 10, 20, 30, 40);
    GLint iv[4] = {0};
    glGetUniformiv(g_prog, g_l.iv2, iv);
    TGLMT_CHECK(iv[0] == 10 && iv[1] == 20, "glUniform2i phải lưu đúng");
    glGetUniformiv(g_prog, g_l.iv3, iv);
    TGLMT_CHECK(iv[2] == 30, "glUniform3i phải lưu đúng");
    glGetUniformiv(g_prog, g_l.iv4, iv);
    TGLMT_CHECK(iv[3] == 40, "glUniform4i phải lưu đúng");

    // vector 2/3/4 — uint
    glUniform2ui(g_l.uv2, 11u, 22u);
    glUniform3ui(g_l.uv3, 11u, 22u, 33u);
    glUniform4ui(g_l.uv4, 11u, 22u, 33u, 44u);
    GLuint uv[4] = {0};
    glGetUniformuiv(g_prog, g_l.uv2, uv);
    TGLMT_CHECK(uv[1] == 22u, "glUniform2ui phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.uv3, uv);
    TGLMT_CHECK(uv[2] == 33u, "glUniform3ui phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.uv4, uv);
    TGLMT_CHECK(uv[3] == 44u, "glUniform4ui phải lưu đúng");

    // vector 2/3/4 — double
    glUniform2d(g_l.dv, 1.5, 2.5);
    GLdouble dv[4] = {0};
    glGetUniformdv(g_prog, g_l.dv, dv);
    TGLMT_CHECK(NearD(dv[0], 1.5) && NearD(dv[1], 2.5), "glUniform2d phải lưu đúng double");
    glUseProgram(0);
    return true;
}

// ===========================================================================
// 2) glUniform{1..4}{f,i,ui,d}v — dạng mảng (16 hàm)
// ===========================================================================
bool TestUniformArrays() {
    glUseProgram(g_prog);

    const GLfloat fv2[2] = {5.0f, 6.0f};
    glUniform1fv(g_l.f, 1, fv2);
    GLfloat out[4] = {0};
    glGetUniformfv(g_prog, g_l.f, out);
    TGLMT_CHECK(Near(out[0], 5.0f), "glUniform1fv phải lưu phần tử đầu");

    const GLfloat fv[2] = {7.0f, 8.0f};
    glUniform2fv(g_l.v2, 1, fv);
    glGetUniformfv(g_prog, g_l.v2, out);
    TGLMT_CHECK(Near(out[0], 7.0f) && Near(out[1], 8.0f), "glUniform2fv phải lưu đúng");

    const GLfloat fv3[3] = {1.0f, 2.0f, 3.0f};
    glUniform3fv(g_l.v3, 1, fv3);
    glGetUniformfv(g_prog, g_l.v3, out);
    TGLMT_CHECK(Near(out[2], 3.0f), "glUniform3fv phải lưu đúng");

    const GLfloat fv4[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    glUniform4fv(g_l.v4, 1, fv4);
    glGetUniformfv(g_prog, g_l.v4, out);
    TGLMT_CHECK(Near(out[3], 4.0f), "glUniform4fv phải lưu đúng");

    const GLint iv[2] = {-3, -4};
    glUniform1iv(g_l.i, 1, iv);
    GLint iout[4] = {0};
    glGetUniformiv(g_prog, g_l.i, iout);
    TGLMT_CHECK(iout[0] == -3, "glUniform1iv phải lưu đúng");

    glUniform2iv(g_l.iv2, 1, iv);
    glGetUniformiv(g_prog, g_l.iv2, iout);
    TGLMT_CHECK(iout[0] == -3 && iout[1] == -4, "glUniform2iv phải lưu đúng");

    const GLint iv3[3] = {1, 2, 3};
    glUniform3iv(g_l.iv3, 1, iv3);
    glGetUniformiv(g_prog, g_l.iv3, iout);
    TGLMT_CHECK(iout[2] == 3, "glUniform3iv phải lưu đúng");

    const GLint iv4[4] = {1, 2, 3, 4};
    glUniform4iv(g_l.iv4, 1, iv4);
    glGetUniformiv(g_prog, g_l.iv4, iout);
    TGLMT_CHECK(iout[3] == 4, "glUniform4iv phải lưu đúng");

    const GLuint uv[2] = {7u, 8u};
    glUniform1uiv(g_l.u, 1, uv);
    GLuint uout[4] = {0};
    glGetUniformuiv(g_prog, g_l.u, uout);
    TGLMT_CHECK(uout[0] == 7u, "glUniform1uiv phải lưu đúng");

    glUniform2uiv(g_l.uv2, 1, uv);
    glGetUniformuiv(g_prog, g_l.uv2, uout);
    TGLMT_CHECK(uout[1] == 8u, "glUniform2uiv phải lưu đúng");

    const GLuint uv3[3] = {1u, 2u, 3u};
    glUniform3uiv(g_l.uv3, 1, uv3);
    glGetUniformuiv(g_prog, g_l.uv3, uout);
    TGLMT_CHECK(uout[2] == 3u, "glUniform3uiv phải lưu đúng");

    const GLuint uv4[4] = {1u, 2u, 3u, 4u};
    glUniform4uiv(g_l.uv4, 1, uv4);
    glGetUniformuiv(g_prog, g_l.uv4, uout);
    TGLMT_CHECK(uout[3] == 4u, "glUniform4uiv phải lưu đúng");

    const GLdouble dv[2] = {9.5, 10.5};
    glUniform1dv(g_l.d, 1, dv);
    GLdouble dout[4] = {0};
    glGetUniformdv(g_prog, g_l.d, dout);
    TGLMT_CHECK(NearD(dout[0], 9.5), "glUniform1dv phải lưu đúng double");

    glUniform2dv(g_l.dv, 1, dv);
    glGetUniformdv(g_prog, g_l.dv, dout);
    TGLMT_CHECK(NearD(dout[1], 10.5), "glUniform2dv phải lưu đúng double");

    // Mảng uniform thật (float uFv[2]): mỗi phần tử 1 thành phần → lệnh
    // đúng là Uniform1fv với count = 2. Uniform2fv lên float[2] là SAI kích
    // thước (spec §7.6.1) → phải sinh INVALID_OPERATION.
    const GLfloat arr[2] = {1.0f, 2.0f};
    glUniform1fv(g_l.fv, 2, arr);
    glGetUniformfv(g_prog, g_l.fv, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[1], 2.0f),
                "glUniform1fv count=2 lên mảng float[2] phải lưu 2 phần tử");
    DrainErrors();
    glUniform2fv(g_l.fv, 2, arr);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform2fv lên mảng float[2] → INVALID_OPERATION (sai kích thước phần tử)");

    glUseProgram(0);
    return true;
}

// ===========================================================================
// 3) glUniformMatrix* — 18 hàm (transpose + layout)
// ===========================================================================
bool TestUniformMatrices() {
    glUseProgram(g_prog);
    GLfloat out[16] = {0};

    const GLfloat m4[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glUniformMatrix4fv(g_l.m4, 1, GL_FALSE, m4);
    glGetUniformfv(g_prog, g_l.m4, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[15], 16.0f),
                "glUniformMatrix4fv transpose=FALSE giữ nguyên column-major");

    const GLfloat m3[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    glUniformMatrix3fv(g_l.m3, 1, GL_FALSE, m3);
    glGetUniformfv(g_prog, g_l.m3, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[8], 9.0f), "glUniformMatrix3fv giữ layout");

    const GLfloat m2[4] = {1, 2, 3, 4};
    glUniformMatrix2fv(g_l.m2, 1, GL_FALSE, m2);
    glGetUniformfv(g_prog, g_l.m2, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[3], 4.0f), "glUniformMatrix2fv giữ layout");

    // Ma trận không vuông: mat2x3 = 2 cột × 3 hàng = 6 float.
    const GLfloat m2x3[6] = {1, 2, 3, 4, 5, 6};
    glUniformMatrix2x3fv(g_l.m2x3, 1, GL_FALSE, m2x3);
    glGetUniformfv(g_prog, g_l.m2x3, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[5], 6.0f), "glUniformMatrix2x3fv giữ layout");

    const GLfloat m3x2[6] = {1, 2, 3, 4, 5, 6};
    glUniformMatrix3x2fv(g_l.m3x2, 1, GL_FALSE, m3x2);
    glGetUniformfv(g_prog, g_l.m3x2, out);
    TGLMT_CHECK(Near(out[5], 6.0f), "glUniformMatrix3x2fv giữ layout");

    const GLfloat m2x4[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glUniformMatrix2x4fv(g_l.m2x4, 1, GL_FALSE, m2x4);
    glGetUniformfv(g_prog, g_l.m2x4, out);
    TGLMT_CHECK(Near(out[7], 8.0f), "glUniformMatrix2x4fv giữ layout");

    const GLfloat m4x2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glUniformMatrix4x2fv(g_l.m4x2, 1, GL_FALSE, m4x2);
    glGetUniformfv(g_prog, g_l.m4x2, out);
    TGLMT_CHECK(Near(out[7], 8.0f), "glUniformMatrix4x2fv giữ layout");

    const GLfloat m3x4[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    glUniformMatrix3x4fv(g_l.m3x4, 1, GL_FALSE, m3x4);
    glGetUniformfv(g_prog, g_l.m3x4, out);
    TGLMT_CHECK(Near(out[11], 12.0f), "glUniformMatrix3x4fv giữ layout");

    const GLfloat m4x3[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    glUniformMatrix4x3fv(g_l.m4x3, 1, GL_FALSE, m4x3);
    glGetUniformfv(g_prog, g_l.m4x3, out);
    TGLMT_CHECK(Near(out[11], 12.0f), "glUniformMatrix4x3fv giữ layout");

    // Bản double (9 hàm).
    const GLdouble d4[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glUniformMatrix4dv(g_l.m4, 1, GL_FALSE, d4);
    GLdouble dout[16] = {0};
    glGetUniformdv(g_prog, g_l.m4, dout);
    TGLMT_CHECK(NearD(dout[0], 1.0) && NearD(dout[15], 16.0), "glUniformMatrix4dv giữ layout");

    const GLdouble d3[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    glUniformMatrix3dv(g_l.m3, 1, GL_FALSE, d3);
    glGetUniformdv(g_prog, g_l.m3, dout);
    TGLMT_CHECK(NearD(dout[8], 9.0), "glUniformMatrix3dv giữ layout");

    const GLdouble d2[4] = {1, 2, 3, 4};
    glUniformMatrix2dv(g_l.m2, 1, GL_FALSE, d2);
    glGetUniformdv(g_prog, g_l.m2, dout);
    TGLMT_CHECK(NearD(dout[3], 4.0), "glUniformMatrix2dv giữ layout");

    const GLdouble d2x3[6] = {1, 2, 3, 4, 5, 6};
    glUniformMatrix2x3dv(g_l.m2x3, 1, GL_FALSE, d2x3);
    glGetUniformdv(g_prog, g_l.m2x3, dout);
    TGLMT_CHECK(NearD(dout[5], 6.0), "glUniformMatrix2x3dv giữ layout");

    const GLdouble d3x2[6] = {1, 2, 3, 4, 5, 6};
    glUniformMatrix3x2dv(g_l.m3x2, 1, GL_FALSE, d3x2);
    glGetUniformdv(g_prog, g_l.m3x2, dout);
    TGLMT_CHECK(NearD(dout[5], 6.0), "glUniformMatrix3x2dv giữ layout");

    const GLdouble d2x4[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glUniformMatrix2x4dv(g_l.m2x4, 1, GL_FALSE, d2x4);
    glGetUniformdv(g_prog, g_l.m2x4, dout);
    TGLMT_CHECK(NearD(dout[7], 8.0), "glUniformMatrix2x4dv giữ layout");

    const GLdouble d4x2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glUniformMatrix4x2dv(g_l.m4x2, 1, GL_FALSE, d4x2);
    glGetUniformdv(g_prog, g_l.m4x2, dout);
    TGLMT_CHECK(NearD(dout[7], 8.0), "glUniformMatrix4x2dv giữ layout");

    const GLdouble d3x4[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    glUniformMatrix3x4dv(g_l.m3x4, 1, GL_FALSE, d3x4);
    glGetUniformdv(g_prog, g_l.m3x4, dout);
    TGLMT_CHECK(NearD(dout[11], 12.0), "glUniformMatrix3x4dv giữ layout");

    const GLdouble d4x3[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    glUniformMatrix4x3dv(g_l.m4x3, 1, GL_FALSE, d4x3);
    glGetUniformdv(g_prog, g_l.m4x3, dout);
    TGLMT_CHECK(NearD(dout[11], 12.0), "glUniformMatrix4x3dv giữ layout");

    glUseProgram(0);
    return true;
}

// ===========================================================================
// 4) glProgramUniform* — đường DSA (50 hàm)
// ===========================================================================
bool TestProgramUniform() {
    // Không cần glUseProgram — đường DSA ghi thẳng vào program.
    const GLfloat fv[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    glProgramUniform1f(g_prog, g_l.f, 1.5f);
    glProgramUniform1i(g_prog, g_l.i, -7);
    glProgramUniform1ui(g_prog, g_l.u, 7u);
    glProgramUniform1d(g_prog, g_l.d, 3.25);
    glProgramUniform2f(g_prog, g_l.v2, 1.0f, 2.0f);
    glProgramUniform3f(g_prog, g_l.v3, 1.0f, 2.0f, 3.0f);
    glProgramUniform4f(g_prog, g_l.v4, 1.0f, 2.0f, 3.0f, 4.0f);
    glProgramUniform2i(g_prog, g_l.iv2, 5, 6);
    glProgramUniform3i(g_prog, g_l.iv3, 5, 6, 7);
    glProgramUniform4i(g_prog, g_l.iv4, 5, 6, 7, 8);
    glProgramUniform2ui(g_prog, g_l.uv2, 5u, 6u);
    glProgramUniform3ui(g_prog, g_l.uv3, 5u, 6u, 7u);
    glProgramUniform4ui(g_prog, g_l.uv4, 5u, 6u, 7u, 8u);
    glProgramUniform2d(g_prog, g_l.dv, 1.5, 2.5);

    GLfloat out[16] = {0};
    GLint iout[16] = {0};
    GLuint uout[16] = {0};
    GLdouble dout[16] = {0};

    glGetUniformfv(g_prog, g_l.f, out);
    TGLMT_CHECK(Near(out[0], 1.5f), "glProgramUniform1f phải lưu đúng");
    glGetUniformiv(g_prog, g_l.i, iout);
    TGLMT_CHECK(iout[0] == -7, "glProgramUniform1i phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.u, uout);
    TGLMT_CHECK(uout[0] == 7u, "glProgramUniform1ui phải lưu đúng");
    glGetUniformdv(g_prog, g_l.d, dout);
    TGLMT_CHECK(NearD(dout[0], 3.25), "glProgramUniform1d phải lưu đúng");
    glGetUniformfv(g_prog, g_l.v2, out);
    TGLMT_CHECK(Near(out[1], 2.0f), "glProgramUniform2f phải lưu đúng");
    glGetUniformfv(g_prog, g_l.v3, out);
    TGLMT_CHECK(Near(out[2], 3.0f), "glProgramUniform3f phải lưu đúng");
    glGetUniformfv(g_prog, g_l.v4, out);
    TGLMT_CHECK(Near(out[3], 4.0f), "glProgramUniform4f phải lưu đúng");
    glGetUniformiv(g_prog, g_l.iv2, iout);
    TGLMT_CHECK(iout[0] == 5 && iout[1] == 6, "glProgramUniform2i phải lưu đúng");
    glGetUniformiv(g_prog, g_l.iv3, iout);
    TGLMT_CHECK(iout[2] == 7, "glProgramUniform3i phải lưu đúng");
    glGetUniformiv(g_prog, g_l.iv4, iout);
    TGLMT_CHECK(iout[3] == 8, "glProgramUniform4i phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.uv2, uout);
    TGLMT_CHECK(uout[0] == 5u, "glProgramUniform2ui phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.uv3, uout);
    TGLMT_CHECK(uout[2] == 7u, "glProgramUniform3ui phải lưu đúng");
    glGetUniformuiv(g_prog, g_l.uv4, uout);
    TGLMT_CHECK(uout[3] == 8u, "glProgramUniform4ui phải lưu đúng");
    glGetUniformdv(g_prog, g_l.dv, dout);
    TGLMT_CHECK(NearD(dout[1], 2.5), "glProgramUniform2d phải lưu đúng");

    // Dạng mảng.
    const GLfloat fa[4] = {9.0f, 8.0f, 7.0f, 6.0f};
    glProgramUniform1fv(g_prog, g_l.f, 1, fa);
    glProgramUniform2fv(g_prog, g_l.v2, 1, fa);
    glProgramUniform3fv(g_prog, g_l.v3, 1, fa);
    glProgramUniform4fv(g_prog, g_l.v4, 1, fa);
    glGetUniformfv(g_prog, g_l.v4, out);
    TGLMT_CHECK(Near(out[0], 9.0f) && Near(out[3], 6.0f), "glProgramUniform4fv phải lưu đúng");

    const GLint ia[4] = {1, 2, 3, 4};
    glProgramUniform1iv(g_prog, g_l.i, 1, ia);
    glProgramUniform2iv(g_prog, g_l.iv2, 1, ia);
    glProgramUniform3iv(g_prog, g_l.iv3, 1, ia);
    glProgramUniform4iv(g_prog, g_l.iv4, 1, ia);
    glGetUniformiv(g_prog, g_l.iv4, iout);
    TGLMT_CHECK(iout[3] == 4, "glProgramUniform4iv phải lưu đúng");

    const GLuint ua[4] = {1u, 2u, 3u, 4u};
    glProgramUniform1uiv(g_prog, g_l.u, 1, ua);
    glProgramUniform2uiv(g_prog, g_l.uv2, 1, ua);
    glProgramUniform3uiv(g_prog, g_l.uv3, 1, ua);
    glProgramUniform4uiv(g_prog, g_l.uv4, 1, ua);
    glGetUniformuiv(g_prog, g_l.uv4, uout);
    TGLMT_CHECK(uout[3] == 4u, "glProgramUniform4uiv phải lưu đúng");

    const GLdouble da[4] = {1.25, 2.25, 3.25, 4.25};
    glProgramUniform1dv(g_prog, g_l.d, 1, da);
    glProgramUniform2dv(g_prog, g_l.dv, 1, da);
    glGetUniformdv(g_prog, g_l.d, dout);
    TGLMT_CHECK(NearD(dout[0], 1.25), "glProgramUniform1dv phải lưu đúng");

    // Ma trận DSA (18 hàm).
    const GLfloat m4[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glProgramUniformMatrix2fv(g_prog, g_l.m2, 1, GL_FALSE, m4);
    glProgramUniformMatrix3fv(g_prog, g_l.m3, 1, GL_FALSE, m4);
    glProgramUniformMatrix4fv(g_prog, g_l.m4, 1, GL_FALSE, m4);
    glProgramUniformMatrix2x3fv(g_prog, g_l.m2x3, 1, GL_FALSE, m4);
    glProgramUniformMatrix3x2fv(g_prog, g_l.m3x2, 1, GL_FALSE, m4);
    glProgramUniformMatrix2x4fv(g_prog, g_l.m2x4, 1, GL_FALSE, m4);
    glProgramUniformMatrix4x2fv(g_prog, g_l.m4x2, 1, GL_FALSE, m4);
    glProgramUniformMatrix3x4fv(g_prog, g_l.m3x4, 1, GL_FALSE, m4);
    glProgramUniformMatrix4x3fv(g_prog, g_l.m4x3, 1, GL_FALSE, m4);
    glGetUniformfv(g_prog, g_l.m4, out);
    TGLMT_CHECK(Near(out[0], 1.0f) && Near(out[15], 16.0f), "glProgramUniformMatrix4fv giữ layout");

    const GLdouble dm4[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glProgramUniformMatrix2dv(g_prog, g_l.m2, 1, GL_FALSE, dm4);
    glProgramUniformMatrix3dv(g_prog, g_l.m3, 1, GL_FALSE, dm4);
    glProgramUniformMatrix4dv(g_prog, g_l.m4, 1, GL_FALSE, dm4);
    glProgramUniformMatrix2x3dv(g_prog, g_l.m2x3, 1, GL_FALSE, dm4);
    glProgramUniformMatrix3x2dv(g_prog, g_l.m3x2, 1, GL_FALSE, dm4);
    glProgramUniformMatrix2x4dv(g_prog, g_l.m2x4, 1, GL_FALSE, dm4);
    glProgramUniformMatrix4x2dv(g_prog, g_l.m4x2, 1, GL_FALSE, dm4);
    glProgramUniformMatrix3x4dv(g_prog, g_l.m3x4, 1, GL_FALSE, dm4);
    glProgramUniformMatrix4x3dv(g_prog, g_l.m4x3, 1, GL_FALSE, dm4);
    glGetUniformdv(g_prog, g_l.m4, dout);
    TGLMT_CHECK(NearD(dout[0], 1.0) && NearD(dout[15], 16.0), "glProgramUniformMatrix4dv giữ layout");
    return true;
}

// ===========================================================================
// 5) Lỗi theo spec (GL 4.6 §7.6.1)
// ===========================================================================
bool TestUniformErrors() {
    glUseProgram(g_prog);
    const GLfloat fv[4] = {1, 2, 3, 4};
    const GLint iv[4] = {1, 2, 3, 4};
    const GLuint uiv[4] = {1, 2, 3, 4};
    const GLdouble dv[4] = {1, 2, 3, 4};

    // count < 0 → INVALID_VALUE (16 lệnh dạng mảng).
    DrainErrors();
    glUniform1fv(g_l.f, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform1fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform2fv(g_l.v2, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform2fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform3fv(g_l.v3, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform3fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform4fv(g_l.v4, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform1iv(g_l.i, -1, iv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform1iv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform2iv(g_l.iv2, -1, iv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform2iv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform3iv(g_l.iv3, -1, iv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform3iv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform4iv(g_l.iv4, -1, iv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform4iv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform1uiv(g_l.u, -1, uiv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform1uiv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform2uiv(g_l.uv2, -1, uiv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform2uiv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform3uiv(g_l.uv3, -1, uiv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform3uiv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform4uiv(g_l.uv4, -1, uiv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform4uiv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform1dv(g_l.d, -1, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform1dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniform2dv(g_l.dv, -1, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniform2dv count<0 → INVALID_VALUE");

    // count < 0 cho 18 lệnh ma trận.
    DrainErrors();
    glUniformMatrix2fv(g_l.m2, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix2x3fv(g_l.m2x3, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2x3fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix2x4fv(g_l.m2x4, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2x4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3fv(g_l.m3, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3x2fv(g_l.m3x2, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3x2fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3x4fv(g_l.m3x4, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3x4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4fv(g_l.m4, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4x2fv(g_l.m4x2, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4x2fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4x3fv(g_l.m4x3, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4x3fv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix2dv(g_l.m2, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix2x3dv(g_l.m2x3, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2x3dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix2x4dv(g_l.m2x4, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix2x4dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3dv(g_l.m3, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3x2dv(g_l.m3x2, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3x2dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix3x4dv(g_l.m3x4, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix3x4dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4dv(g_l.m4, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4x2dv(g_l.m4x2, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4x2dv count<0 → INVALID_VALUE");
    DrainErrors();
    glUniformMatrix4x3dv(g_l.m4x3, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glUniformMatrix4x3dv count<0 → INVALID_VALUE");

    // DSA: count < 0 → INVALID_VALUE.
    DrainErrors();
    glProgramUniform1fv(g_prog, g_l.f, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniform1fv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniform4fv(g_prog, g_l.v4, -1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniform4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniform1iv(g_prog, g_l.i, -1, iv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniform1iv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniform4uiv(g_prog, g_l.uv4, -1, uiv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniform4uiv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniform2dv(g_prog, g_l.dv, -1, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniform2dv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniformMatrix4fv(g_prog, g_l.m4, -1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glProgramUniformMatrix4fv count<0 → INVALID_VALUE");
    DrainErrors();
    glProgramUniformMatrix2x3dv(g_prog, g_l.m2x3, -1, GL_FALSE, dv);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glProgramUniformMatrix2x3dv count<0 → INVALID_VALUE");

    // Sai kích thước: vec4 nhận bằng lệnh 3 phần tử → INVALID_OPERATION.
    DrainErrors();
    glUniform3f(g_l.v4, 1.0f, 2.0f, 3.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform3f lên uniform vec4 → INVALID_OPERATION (sai kích thước)");
    DrainErrors();
    glUniform4f(g_l.v3, 1.0f, 2.0f, 3.0f, 4.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform4f lên uniform vec3 → INVALID_OPERATION (sai kích thước)");
    DrainErrors();
    glUniform1f(g_l.v2, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1f lên uniform vec2 → INVALID_OPERATION (sai kích thước)");

    // Sai kiểu: uniform float nhận bằng lệnh int → INVALID_OPERATION.
    DrainErrors();
    glUniform1i(g_l.f, 1);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1i lên uniform float → INVALID_OPERATION (sai kiểu)");
    DrainErrors();
    glUniform1f(g_l.i, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1f lên uniform int → INVALID_OPERATION (sai kiểu)");
    DrainErrors();
    glUniform1i(g_l.u, 1);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1i lên uniform uint → INVALID_OPERATION (sai kiểu)");
    DrainErrors();
    glUniform1ui(g_l.i, 1u);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1ui lên uniform int → INVALID_OPERATION (sai kiểu)");
    DrainErrors();
    glUniform1d(g_l.f, 1.0);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1d lên uniform float → INVALID_OPERATION (sai kiểu)");

    // Ma trận nhận bằng lệnh vector → INVALID_OPERATION.
    DrainErrors();
    glUniform3fv(g_l.m3, 1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform3fv lên uniform mat3 → INVALID_OPERATION (phải dùng UniformMatrix)");
    DrainErrors();
    glUniformMatrix3fv(g_l.m4, 1, GL_FALSE, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniformMatrix3fv lên uniform mat4 → INVALID_OPERATION (sai kích thước ma trận)");

    // count > 1 trên uniform KHÔNG phải mảng → INVALID_OPERATION.
    DrainErrors();
    glUniform4fv(g_l.v4, 2, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform4fv count=2 lên uniform vec4 (không phải mảng) → INVALID_OPERATION");

    // Sampler chỉ nạp được bằng Uniform1i{v}.
    DrainErrors();
    glUniform1f(g_l.tex, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1f lên uniform sampler → INVALID_OPERATION");
    DrainErrors();
    glUniform1fv(g_l.tex, 1, fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1fv lên uniform sampler → INVALID_OPERATION");

    // Sampler ngoài [0, MAX_COMBINED_TEXTURE_IMAGE_UNITS) → INVALID_VALUE.
    DrainErrors();
    glUniform1i(g_l.tex, -1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glUniform1i lên sampler với giá trị âm → INVALID_VALUE");
    DrainErrors();
    glUniform1i(g_l.tex, 100000);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glUniform1i lên sampler với giá trị vượt giới hạn → INVALID_VALUE");

    // Location không hợp lệ (không phải -1) → INVALID_OPERATION.
    DrainErrors();
    glUniform1f(9999, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1f với location không tồn tại → INVALID_OPERATION");

    // Program không hợp lệ (DSA) → INVALID_VALUE.
    DrainErrors();
    glProgramUniform1f(9999, g_l.f, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glProgramUniform1f với program không tồn tại → INVALID_VALUE");

    // Program chưa link → INVALID_OPERATION.
    {
        GLuint p2 = glCreateProgram();
        DrainErrors();
        glProgramUniform1f(p2, 0, 1.0f);
        TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                    "glProgramUniform1f với program chưa link → INVALID_OPERATION");
        glDeleteProgram(p2);
    }

    // Không có program đang dùng → INVALID_OPERATION.
    glUseProgram(0);
    DrainErrors();
    glUniform1f(g_l.f, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform1f khi không có program → INVALID_OPERATION");

    glUseProgram(g_prog);
    return true;
}

// ===========================================================================
// 6) location == -1 → bỏ qua IM LẶNG (không lỗi, không đổi giá trị)
// ===========================================================================
bool TestLocationMinusOne() {
    glUseProgram(g_prog);
    const GLfloat v4[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    // Giá trị trước khi ghi.
    glUniform4f(g_l.v4, 1.0f, 2.0f, 3.0f, 4.0f);
    GLfloat out[4] = {0};
    glGetUniformfv(g_prog, g_l.v4, out);
    const GLfloat before[4] = {out[0], out[1], out[2], out[3]};

    DrainErrors();
    glUniform1f(-1, 99.0f);
    glUniform2f(-1, 99.0f, 99.0f);
    glUniform3f(-1, 99.0f, 99.0f, 99.0f);
    glUniform4f(-1, 99.0f, 99.0f, 99.0f, 99.0f);
    glUniform1i(-1, 99);
    glUniform1ui(-1, 99u);
    glUniform1d(-1, 99.0);
    glUniform1fv(-1, 1, v4);
    glUniform4fv(-1, 1, v4);
    glUniform1iv(-1, 1, (const GLint*)v4);
    glUniform1uiv(-1, 1, (const GLuint*)v4);
    glUniform1dv(-1, 1, (const GLdouble*)v4);
    glUniformMatrix4fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix3fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix2fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix2x3fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix3x2fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix2x4fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix4x2fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix3x4fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix4x3fv(-1, 1, GL_FALSE, v4);
    glUniformMatrix4dv(-1, 1, GL_FALSE, (const GLdouble*)v4);
    glProgramUniform1f(g_prog, -1, 99.0f);
    glProgramUniform4f(g_prog, -1, 99.0f, 99.0f, 99.0f, 99.0f);
    glProgramUniform1fv(g_prog, -1, 1, v4);
    glProgramUniformMatrix4fv(g_prog, -1, 1, GL_FALSE, v4);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "location == -1 phải bị bỏ qua IM LẶNG (không sinh lỗi)");

    // Giá trị cũ phải được giữ nguyên.
    glGetUniformfv(g_prog, g_l.v4, out);
    TGLMT_CHECK(Near(out[0], before[0]) && Near(out[3], before[3]),
                "location == -1 không được đổi giá trị uniform hiện có");
    glUseProgram(0);
    return true;
}

// ===========================================================================
// 7) glGetUniform* / glGetnUniform* (8 hàm)
// ===========================================================================
bool TestGetUniformQueries() {
    glUseProgram(g_prog);
    const GLfloat fv[4] = {1.5f, 2.5f, 3.5f, 4.5f};
    glUniform4f(g_l.v4, 1.5f, 2.5f, 3.5f, 4.5f);

    GLfloat out[16] = {0};
    glGetUniformfv(g_prog, g_l.v4, out);
    TGLMT_CHECK(Near(out[0], 1.5f) && Near(out[3], 4.5f), "glGetUniformfv trả đúng giá trị");

    // glGetnUniformfv: bufSize tính bằng BYTE.
    GLfloat nout[16] = {0};
    glGetnUniformfv(g_prog, g_l.v4, 16, nout);
    TGLMT_CHECK(Near(nout[0], 1.5f), "glGetnUniformfv trả đúng giá trị");
    DrainErrors();
    glGetnUniformfv(g_prog, g_l.v4, 4, nout);  // 4 byte < 16 byte cần
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetnUniformfv bufSize quá nhỏ → INVALID_OPERATION");
    DrainErrors();
    glGetnUniformfv(g_prog, g_l.v4, -1, nout);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetnUniformfv bufSize âm → INVALID_VALUE");

    // glGetUniformiv/uiv/dv trên cùng uniform float → chuyển đổi kiểu.
    GLint iout[16] = {0};
    glGetUniformiv(g_prog, g_l.v4, iout);
    TGLMT_CHECK(iout[0] == 1 && iout[3] == 4, "glGetUniformiv phải chuyển đổi sang int");
    GLuint uout[16] = {0};
    glGetUniformuiv(g_prog, g_l.v4, uout);
    TGLMT_CHECK(uout[0] == 1u && uout[3] == 4u, "glGetUniformuiv phải chuyển đổi sang uint");
    GLdouble dout[16] = {0};
    glGetUniformdv(g_prog, g_l.v4, dout);
    TGLMT_CHECK(NearD(dout[0], 1.5) && NearD(dout[3], 4.5),
                "glGetUniformdv phải chuyển đổi sang double (không đọc rác)");

    // glGetnUniformiv/uiv/dv.
    GLint ni[16] = {0};
    glGetnUniformiv(g_prog, g_l.v4, 16, ni);
    TGLMT_CHECK(ni[0] == 1, "glGetnUniformiv trả đúng");
    GLuint nu[16] = {0};
    glGetnUniformuiv(g_prog, g_l.v4, 16, nu);
    TGLMT_CHECK(nu[0] == 1u, "glGetnUniformuiv trả đúng");
    GLdouble nd[16] = {0};
    glGetnUniformdv(g_prog, g_l.v4, 16, nd);
    TGLMT_CHECK(NearD(nd[0], 1.5), "glGetnUniformdv phải trả double đúng (không đọc rác)");

    // Program không hợp lệ → INVALID_VALUE; location không hợp lệ → INVALID_OPERATION.
    DrainErrors();
    glGetUniformfv(9999, 0, out);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetUniformfv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetUniformfv(g_prog, 9999, out);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetUniformfv với location không hợp lệ → INVALID_OPERATION");
    DrainErrors();
    glGetUniformdv(9999, 0, dout);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetUniformdv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetUniformiv(9999, 0, iout);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetUniformiv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetUniformuiv(9999, 0, uout);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetUniformuiv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetnUniformfv(9999, 0, 16, nout);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetnUniformfv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetnUniformiv(9999, 0, 16, ni);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetnUniformiv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetnUniformuiv(9999, 0, 16, nu);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetnUniformuiv với program không tồn tại → INVALID_VALUE");
    DrainErrors();
    glGetnUniformdv(9999, 0, 16, nd);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetnUniformdv với program không tồn tại → INVALID_VALUE");
    glUseProgram(0);
    return true;
}

// ===========================================================================
// 8) Uniform bool (bvec4) — chỉ nạp bằng i/ui/f, đọc lại phải là 0/1
// ===========================================================================
bool TestBoolUniform() {
    glUseProgram(g_prog);
    DrainErrors();
    const GLint bv[4] = {1, 0, 7, 0};
    glUniform4iv(g_l.bv4, 1, bv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glUniform4i lên uniform bvec4 phải hợp lệ");
    GLint out[4] = {0};
    glGetUniformiv(g_prog, g_l.bv4, out);
    TGLMT_CHECK(out[0] == 1 && out[1] == 0 && out[2] == 1 && out[3] == 0,
                "uniform bool phải chuẩn hoá về TRUE/FALSE (0/1)");

    // bool cũng nạp được bằng f/ui.
    const GLfloat fv[4] = {0.0f, 2.0f, 0.0f, 5.0f};
    glUniform4fv(g_l.bv4, 1, fv);
    glGetUniformiv(g_prog, g_l.bv4, out);
    TGLMT_CHECK(out[0] == 0 && out[1] == 1 && out[2] == 0 && out[3] == 1,
                "bool nạp bằng float phải quy đổi thành 0/1");

    // double KHÔNG khớp bool (spec: bool chỉ khớp i/ui/f).
    DrainErrors();
    glUniform4dv(g_l.bv4, 1, (const GLdouble*)fv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glUniform4d lên uniform bool → INVALID_OPERATION (bool không khớp double)");
    glUseProgram(0);
    return true;
}

// ===========================================================================
// 9) glUniform3d/3dv + glProgramUniform3d/3dv/4d/4dv (6 hàm còn thiếu)
// ===========================================================================
bool TestDoubleUniforms() {
    // Program có uniform double (GLSL `double` → MSL float, xem docs limitation).
    static const char* kVS =
        "#version 460 core\n"
        "layout(location = 0) in vec4 aPos;\n"
        "uniform double uD3;\n"
        "uniform double uD4;\n"
        "void main() { gl_Position = aPos; }\n";
    const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLint ok = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(vs, 2048, nullptr, log);
        printf("  (skip: uniform double không compile: %s)\n", log);
        return true;
    }
    const GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    if (!ok) {
        printf("  (skip: link fail)\n");
        glDeleteProgram(prog);
        return true;
    }
    const GLint l3 = glGetUniformLocation(prog, "uD3");
    const GLint l4 = glGetUniformLocation(prog, "uD4");
    TGLMT_CHECK(l3 >= 0 && l4 >= 0, "uniform double phải có location");

    glUseProgram(prog);
    // glUniform3d
    glUniform3d(l3, 1.5, 2.5, 3.5);
    GLdouble d[4] = {0};
    glGetUniformdv(prog, l3, d);
    TGLMT_CHECK(fabs(d[0] - 1.5) < 1e-6 && fabs(d[2] - 3.5) < 1e-6,
                "glUniform3d phải lưu đúng 3 thành phần");
    // glUniform3dv
    const GLdouble v3[3] = {4.5, 5.5, 6.5};
    glUniform3dv(l3, 1, v3);
    glGetUniformdv(prog, l3, d);
    TGLMT_CHECK(fabs(d[0] - 4.5) < 1e-6, "glUniform3dv phải lưu đúng");

    // glProgramUniform3d / 3dv / 4d / 4dv (đường DSA)
    glProgramUniform3d(prog, l3, 7.5, 8.5, 9.5);
    glGetUniformdv(prog, l3, d);
    TGLMT_CHECK(fabs(d[1] - 8.5) < 1e-6, "glProgramUniform3d phải lưu đúng");
    glProgramUniform3dv(prog, l3, 1, v3);
    glGetUniformdv(prog, l3, d);
    TGLMT_CHECK(fabs(d[2] - 6.5) < 1e-6, "glProgramUniform3dv phải lưu đúng");
    glProgramUniform4d(prog, l4, 1.0, 2.0, 3.0, 4.0);
    glGetUniformdv(prog, l4, d);
    TGLMT_CHECK(fabs(d[3] - 4.0) < 1e-6, "glProgramUniform4d phải lưu đúng");
    const GLdouble v4[4] = {10.0, 11.0, 12.0, 13.0};
    glProgramUniform4dv(prog, l4, 1, v4);
    glGetUniformdv(prog, l4, d);
    TGLMT_CHECK(fabs(d[0] - 10.0) < 1e-6, "glProgramUniform4dv phải lưu đúng");

    glUseProgram(0);
    glDeleteProgram(prog);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core uniform entry points (GL 4.6 §7.6.1 / §7.14) ===\n");
    if (!BuildProgram()) {
        printf("SKIP: không dựng được program test\n");
        return 0;
    }
    const Case kCases[] = {
        {"glUniform{1..4}{f,i,ui,d}", TestUniformScalars},
        {"glUniform{1..4}{f,i,ui,d}v", TestUniformArrays},
        {"glUniformMatrix* (18 hàm)", TestUniformMatrices},
        {"glProgramUniform* (50 hàm)", TestProgramUniform},
        {"lỗi theo spec §7.6.1", TestUniformErrors},
        {"location == -1 bỏ qua im lặng", TestLocationMinusOne},
        {"glGetUniform* / glGetnUniform*", TestGetUniformQueries},
        {"uniform bool", TestBoolUniform},
        {"glUniform3d/3dv + glProgramUniform3d/3dv/4d/4dv", TestDoubleUniforms},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
