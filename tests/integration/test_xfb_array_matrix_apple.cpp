// test_xfb_array_matrix_apple.cpp — XFB phải capture được array element và mat2.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <string>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "out float vArr[2];\n"
    "out mat2 vMat;\n"
    "out mat2x3 vMat23;\n"
    "void main() {\n"
    "  vArr[0] = 1.0; vArr[1] = 2.0;\n"
    "  vMat = mat2(3.0, 4.0, 5.0, 6.0);\n"
    "  vMat23 = mat2x3(7.0, 8.0, 9.0, 10.0, 11.0, 12.0);\n"
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

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_xfb_array_matrix_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    tglmt::GLuint vs = Compile(0x8B31, kVS);
    if (!vs) return 1;
    tglmt::GLuint p = glCreateProgram(); glAttachShader(p, vs);
    const char* vars[4] = {"vArr[0]", "vArr[1]", "vMat", "vMat23"};
    glTransformFeedbackVaryings(p, 4, vars, 0x8C8C);
    glLinkProgram(p);
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL link: %s\n", log); return 1; }
    glUseProgram(p);
    tglmt::GLint s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    tglmt::GLenum t0 = 0, t1 = 0, t2 = 0, t3 = 0;
    char n0[64] = {}, n1[64] = {}, n2[64] = {}, n3[64] = {};
    glGetTransformFeedbackVarying(p, 0, sizeof(n0), nullptr, &s0, &t0, n0);
    glGetTransformFeedbackVarying(p, 1, sizeof(n1), nullptr, &s1, &t1, n1);
    glGetTransformFeedbackVarying(p, 2, sizeof(n2), nullptr, &s2, &t2, n2);
    glGetTransformFeedbackVarying(p, 3, sizeof(n3), nullptr, &s3, &t3, n3);
    printf("varyings: %s type=0x%x size=%d | %s type=0x%x size=%d | %s type=0x%x size=%d | %s type=0x%x size=%d\n",
           n0, t0, s0, n1, t1, s1, n2, t2, s2, n3, t3, s3);
    if (t0 != 0x1406 || s0 != 1 || t1 != 0x1406 || s1 != 1 ||
        t2 != 0x8B5A || s2 != 1 || t3 != 0x8B65 || s3 != 1) {
        printf("FAIL varying reflection\n"); return 1;
    }
    if (std::string(n0) != "vArr[0]" || std::string(n1) != "vArr[1]" ||
        std::string(n2) != "vMat" || std::string(n3) != "vMat23") {
        printf("FAIL varying names: %s %s %s %s\n", n0, n1, n2, n3); return 1;
    }
    // vecN: size = 1, type = FLOAT_VECN (không phải số component).
    static const char* kVSVec =
        "#version 460 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "out vec4 vVec;\n"
        "void main() { vVec = vec4(1.0, 2.0, 3.0, 4.0); gl_Position = vec4(aPos, 0.0, 1.0); }\n";
    tglmt::GLuint vsv = Compile(0x8B31, kVSVec);
    if (!vsv) return 1;
    tglmt::GLuint pv = glCreateProgram(); glAttachShader(pv, vsv);
    const char* vv[1] = {"vVec"};
    glTransformFeedbackVaryings(pv, 1, vv, 0x8C8C);
    glLinkProgram(pv);
    glGetProgramiv(pv, 0x8B82, &ok);
    if (!ok) { char log[2048] = {}; glGetProgramInfoLog(pv, sizeof(log), nullptr, log);
               printf("FAIL vec4 link: %s\n", log); return 1; }
    tglmt::GLint vsz = 0; tglmt::GLenum vty = 0; char vn[64] = {};
    glGetTransformFeedbackVarying(pv, 0, sizeof(vn), nullptr, &vsz, &vty, vn);
    if (vty != 0x8B52 || vsz != 1 || std::string(vn) != "vVec") {
        printf("FAIL vec4 reflect %s type=0x%x size=%d\n", vn, vty, vsz); return 1;
    }

    tglmt::GLuint vao = 0, vbo = 0, xb = 0;
    const float verts[] = {-1, -1, 1, -1, 0, 1};
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glGenBuffers(1, &xb); glBindBuffer(0x8892, xb);
    glBufferData(0x8892, 3 * 48, nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    glEnable(0x8C89);
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    glEndTransformFeedback();
    glFinish();
    if (glGetError() != 0) { printf("FAIL draw error\n"); return 1; }
    unsigned char raw[3 * 48] = {};
    glBindBuffer(0x8892, xb); glGetBufferSubData(0x8892, 0, sizeof(raw), raw);
    const float want[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    for (int i = 0; i < 3; ++i) {
        const float* f = (const float*)(raw + i * 48);
        for (int k = 0; k < 12; ++k)
            if (std::fabs(f[k] - want[k]) > 0.001f) {
                printf("FAIL vtx%d f[%d]=%.3f want %.3f\n", i, k, f[k], want[k]); return 1;
            }
    }
    // Whole-array name: GL cho phép `vArr` (capture toàn mảng). Reflection size
    // phải = số phần tử; pack vẫn đúng thứ tự phần tử.
    const char* whole[3] = {"vArr", "vMat", "vMat23"};
    glTransformFeedbackVaryings(p, 3, whole, 0x8C8C);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL relink whole array: %s\n", log); return 1; }
    glUseProgram(p);
    tglmt::GLint ws = 0; tglmt::GLenum wt = 0; char wn[64] = {};
    glGetTransformFeedbackVarying(p, 0, sizeof(wn), nullptr, &ws, &wt, wn);
    if (wt != 0x1406 || ws != 2 || std::string(wn) != "vArr[0]") {
        printf("FAIL whole array reflect %s type=0x%x size=%d\n", wn, wt, ws); return 1;
    }
    glBindBuffer(0x8892, xb); glBufferData(0x8892, 3 * 48, nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0); glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    glBeginTransformFeedback(0x0004); glDrawArrays(0x0004, 0, 3); glEndTransformFeedback();
    glFinish();
    memset(raw, 0, sizeof(raw));
    glBindBuffer(0x8892, xb); glGetBufferSubData(0x8892, 0, sizeof(raw), raw);
    for (int i = 0; i < 3; ++i) {
        const float* f = (const float*)(raw + i * 48);
        for (int k = 0; k < 12; ++k)
            if (std::fabs(f[k] - want[k]) > 0.001f) {
                printf("FAIL whole-array vtx%d f[%d]=%.3f want %.3f\n", i, k, f[k], want[k]); return 1;
            }
    }
    printf("test_xfb_array_matrix_apple PASS\n");
    return 0;
}
