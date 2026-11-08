// test_xfb_matrix_array_apple.cpp — XFB capture array of matrices (VS-only).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "out mat2 vMat[2];\n"
    "void main() {\n"
    "  vMat[0] = mat2(1.0, 2.0, 3.0, 4.0);\n"
    "  vMat[1] = mat2(5.0, 6.0, 7.0, 8.0);\n"
    "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "}\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    tglmt::GLint ok = 0; glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
               printf("FAIL vs compile: %s\n", log); return 0; }
    return s;
}

static int LinkWith(tglmt::GLuint vs, const char** vars, int n, tglmt::GLuint* out) {
    using namespace tglmt::gl;
    tglmt::GLuint p = glCreateProgram(); glAttachShader(p, vs);
    glTransformFeedbackVaryings(p, n, vars, 0x8C8C);
    glLinkProgram(p);
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL link: %s\n", log); return 0; }
    *out = p;
    return 1;
}

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_xfb_matrix_array_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    tglmt::GLuint vs = Compile(0x8B31, kVS);
    if (!vs) return 1;

    tglmt::GLuint vao = 0, vbo = 0, xb = 0;
    const float verts[] = {-1, -1, 1, -1, 0, 1};
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glGenBuffers(1, &xb); glBindBuffer(0x8892, xb);

    // Element vMat[1]: one FLOAT_MAT2 = 4 floats, 3 vertices.
    tglmt::GLuint p = 0;
    const char* elem[1] = {"vMat[1]"};
    if (!LinkWith(vs, elem, 1, &p)) return 1;
    glUseProgram(p);
    tglmt::GLint sz = 0; tglmt::GLenum ty = 0; char nm[64] = {};
    glGetTransformFeedbackVarying(p, 0, sizeof(nm), nullptr, &sz, &ty, nm);
    if (ty != 0x8B5A || sz != 1 || std::string(nm) != "vMat[1]") {
        printf("FAIL element reflect %s type=0x%x size=%d\n", nm, ty, sz); return 1;
    }
    glBufferData(0x8892, 3 * 16, nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    glEnable(0x8C89);
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    glEndTransformFeedback(); glFinish();
    if (glGetError() != 0) { printf("FAIL draw error (element)\n"); return 1; }
    unsigned char raw[3 * 16] = {};
    glBindBuffer(0x8892, xb); glGetBufferSubData(0x8892, 0, sizeof(raw), raw);
    const float wantElem[4] = {5, 6, 7, 8};
    for (int i = 0; i < 3; ++i) {
        const float* f = (const float*)(raw + i * 16);
        for (int k = 0; k < 4; ++k)
            if (std::fabs(f[k] - wantElem[k]) > 0.001f) {
                printf("FAIL element vtx%d f[%d]=%.3f want %.3f\n", i, k, f[k], wantElem[k]); return 1;
            }
    }

    // Whole array vMat: size = 2 matrices, 8 floats/vertex.
    tglmt::GLuint pw = 0;
    const char* whole[1] = {"vMat"};
    if (!LinkWith(vs, whole, 1, &pw)) return 1;
    glUseProgram(pw);
    glGetTransformFeedbackVarying(pw, 0, sizeof(nm), nullptr, &sz, &ty, nm);
    if (ty != 0x8B5A || sz != 2 || std::string(nm) != "vMat[0]") {
        printf("FAIL whole reflect %s type=0x%x size=%d\n", nm, ty, sz); return 1;
    }
    glBindBuffer(0x8892, xb); glBufferData(0x8892, 3 * 32, nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0); glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    glBeginTransformFeedback(0x0004); glDrawArrays(0x0004, 0, 3); glEndTransformFeedback();
    glFinish();
    if (glGetError() != 0) { printf("FAIL draw error (whole)\n"); return 1; }
    unsigned char raww[3 * 32] = {};
    glBindBuffer(0x8892, xb); glGetBufferSubData(0x8892, 0, sizeof(raww), raww);
    const float want[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    for (int i = 0; i < 3; ++i) {
        const float* f = (const float*)(raww + i * 32);
        for (int k = 0; k < 8; ++k)
            if (std::fabs(f[k] - want[k]) > 0.001f) {
                printf("FAIL whole vtx%d f[%d]=%.3f want %.3f\n", i, k, f[k], want[k]); return 1;
            }
    }
    printf("test_xfb_matrix_array_apple PASS\n");
    return 0;
}
