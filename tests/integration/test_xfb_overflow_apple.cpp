// test_xfb_overflow_apple.cpp — XFB buffer nhỏ phải set overflow query, không
// được làm hỏng draw/GL error.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "out vec3 vColor;\n"
    "void main() { vColor = vec3(aPos, 0.5); gl_Position = vec4(aPos, 0.0, 1.0); }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    tglmt::GLint ok = 0; glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
               printf("FAIL compile: %s\n", log); return 0; }
    return s;
}

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_xfb_overflow_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    tglmt::GLuint vs = Compile(0x8B31, kVS);
    if (!vs) return 1;
    tglmt::GLuint p = glCreateProgram(); glAttachShader(p, vs);
    const char* vars[2] = {"vColor", "gl_Position"};
    glTransformFeedbackVaryings(p, 2, vars, 0x8C8C);
    glLinkProgram(p);
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL link: %s\n", log); return 1; }
    glUseProgram(p);
    tglmt::GLuint vao = 0, vbo = 0, xb = 0;
    const float verts[] = {-1, -1, 1, -1, 0, 1};
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glGenBuffers(1, &xb); glBindBuffer(0x8892, xb);
    glBufferData(0x8892, 28, nullptr, 0x88E4); // chỉ đủ 1 record
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferRange(0x8C8E, 0, xb, 0, 28);
    glBindBuffer(0x8892, vbo);
    glEnable(0x8C89);
    tglmt::GLuint qOvf = 0, qStream = 0;
    glGenQueries(1, &qOvf); glGenQueries(1, &qStream);
    glBeginQuery(0x82EC /*TRANSFORM_FEEDBACK_OVERFLOW*/, qOvf);
    glBeginQueryIndexed(0x82ED /*TRANSFORM_FEEDBACK_STREAM_OVERFLOW*/, 0, qStream);
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    glEndTransformFeedback();
    glEndQuery(0x82EC);
    glEndQueryIndexed(0x82ED, 0);
    glFinish();
    if (glGetError() != 0) { printf("FAIL overflow must not raise GL error\n"); return 1; }
    tglmt::GLuint ovf = 0, stream = 0;
    glGetQueryObjectuiv(qOvf, 0x8866, &ovf);
    glGetQueryObjectuiv(qStream, 0x8866, &stream);
    printf("overflow=%u stream0=%u captured=%u\n", ovf, stream, ctx.xfbs[0].capturedCount);
    if (!ovf || !stream) { printf("FAIL overflow query not set\n"); return 1; }
    if (ctx.xfbs[0].capturedCount != 3) { printf("FAIL capturedCount=%u\n", ctx.xfbs[0].capturedCount); return 1; }
    glDeleteQueries(1, &qOvf); glDeleteQueries(1, &qStream);
    printf("test_xfb_overflow_apple PASS\n");
    return 0;
}
