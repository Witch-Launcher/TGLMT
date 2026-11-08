// test_xfb_capture_apple.cpp — XFB capture thật (VS-only, Metal compute prepass).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "uniform float uBias;\n"
    "out vec3 vColor;\n"
    "void main() {\n"
    "  vColor = vec3(aPos.x * 0.25 + 0.5, uBias, 0.25);\n"
    "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec3 vColor;\n"
    "layout(location = 0) out vec4 outc;\n"
    "void main() { outc = vec4(vColor, 1.0); }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src, const char* nm) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    tglmt::GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("FAIL %s compile: %s\n", nm, log);
        return 0;
    }
    return s;
}

struct Vtx { float color[3]; float pad; float pos[4]; };

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_xfb_capture_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vs = Compile(0x8B31, kVS, "vs");
    tglmt::GLuint fs = Compile(0x8B30, kFS, "fs");
    if (!vs || !fs) return 1;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    const char* vars[2] = {"vColor", "gl_Position"};
    glTransformFeedbackVaryings(p, 2, vars, 0x8C8C /*INTERLEAVED_ATTRIBS*/);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL link: %s\n", log);
        return 1;
    }
    glUseProgram(p);
    tglmt::GLint uBias = glGetUniformLocation(p, "uBias");
    if (uBias < 0) { printf("FAIL uBias\n"); return 1; }
    glUniform1f(uBias, 0.75f);

    tglmt::GLuint vao = 0, vbo = 0, xb = 0;
    const float verts[] = {-1, -1, 1, -1, 0, 1};
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &xb);
    glBindBuffer(0x8892, xb);
    glBufferData(0x8892, 3 * sizeof(Vtx), nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);

    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    tglmt::GLuint xq = 0;
    glGenQueries(1, &xq);
    glBeginQuery(0x8C88 /*TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN*/, xq);
    glBeginTransformFeedback(0x0004 /*TRIANGLES*/);
    glDrawArrays(0x0004, 0, 3);
    glEndTransformFeedback();
    glEndQuery(0x8C88);
    tglmt::GLuint primsWritten = 0;
    glGetQueryObjectuiv(xq, 0x8866, &primsWritten);
    glDeleteQueries(1, &xq);
    glFinish();
    if (primsWritten != 1) {
        printf("FAIL primitives written=%u want 1\n", primsWritten);
        return 1;
    }

    unsigned char raw[3 * 32] = {};
    glBindBuffer(0x8892, xb);
    glGetBufferSubData(0x8892, 0, 3 * 28, raw);
    const float wantColor[3][3] = {
        {0.25f, 0.75f, 0.25f}, {0.75f, 0.75f, 0.25f}, {0.5f, 0.75f, 0.25f},
    };
    const float wantPos[3][4] = {
        {-1, -1, 0, 1}, {1, -1, 0, 1}, {0, 1, 0, 1},
    };
    for (int i = 0; i < 3; ++i) {
        const float* c = (const float*)(raw + i * 28);
        const float* q = (const float*)(raw + i * 28 + 12);
        for (int k = 0; k < 3; ++k)
            if (std::fabs(c[k] - wantColor[i][k]) > 0.001f) {
                printf("FAIL vColor[%d][%d]=%.4f want %.4f\n", i, k, c[k], wantColor[i][k]);
                return 1;
            }
        for (int k = 0; k < 4; ++k)
            if (std::fabs(q[k] - wantPos[i][k]) > 0.001f) {
                printf("FAIL gl_Position[%d][%d]=%.4f want %.4f\n", i, k, q[k], wantPos[i][k]);
                return 1;
            }
    }
    tglmt::GLint written = 0;
    glGetTransformFeedbackiv(0, 0x8E24, &written); // cursor chưa dùng; kiểm tra object
    // DrawTransformFeedback dùng đúng số vertex đã capture.
    uint64_t drawsBefore = ctx.appleStats.drawsAttempted;
    glDrawTransformFeedback(0x0004, 0);
    if (ctx.appleStats.drawsAttempted != drawsBefore + 1 || glGetError() != 0) {
        printf("FAIL glDrawTransformFeedback count/path\n");
        return 1;
    }

    // Separate attribs: 2 buffer, mỗi buffer nhận 1 varying.
    tglmt::GLuint sa = 0, sb = 0;
    glGenBuffers(1, &sa);
    glGenBuffers(1, &sb);
    glBindBuffer(0x8892, sa);
    glBufferData(0x8892, 6 * sizeof(float) * 3, nullptr, 0x88E4);
    glBindBuffer(0x8892, sb);
    glBufferData(0x8892, 6 * sizeof(float) * 4, nullptr, 0x88E4);
    const char* seps[2] = {"vColor", "gl_Position"};
    glTransformFeedbackVaryings(p, 2, seps, 0x8C8D /*SEPARATE_ATTRIBS*/);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL relink separate: %s\n", log);
        return 1;
    }
    glUseProgram(p);
    uBias = glGetUniformLocation(p, "uBias");
    glUniform1f(uBias, 0.5f);
    glBindBufferBase(0x8C8E, 0, sa);
    glBindBufferBase(0x8C8E, 1, sb);
    glBindBuffer(0x8892, vbo);
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    glPauseTransformFeedback();
    glDrawArrays(0x0004, 0, 3); // pause: không capture
    glResumeTransformFeedback();
    glDrawArrays(0x0004, 0, 3); // resume: ghi tiếp từ vertex 3
    glEndTransformFeedback();
    glFinish();
    if (ctx.xfbs[0].capturedCount != 6) {
        printf("FAIL pause/resume count=%u want 6\n", ctx.xfbs[0].capturedCount);
        return 1;
    }
    float colors[18] = {}, positions[24] = {};
    glBindBuffer(0x8892, sa);
    glGetBufferSubData(0x8892, 0, sizeof(colors), colors);
    glBindBuffer(0x8892, sb);
    glGetBufferSubData(0x8892, 0, sizeof(positions), positions);
    if (std::fabs(colors[1] - 0.5f) > 0.001f || std::fabs(colors[13] - 0.5f) > 0.001f ||
        std::fabs(positions[3] - 1.0f) > 0.001f || std::fabs(positions[4] - 1.0f) > 0.001f ||
        std::fabs(positions[5] + 1.0f) > 0.001f ||
        std::fabs(positions[15] - 1.0f) > 0.001f || std::fabs(positions[16] - 1.0f) > 0.001f) {
        printf("FAIL separate capture: c1=%.3f c4=%.3f pos=%f,%f,%f / %f,%f,%f\n",
               colors[1], colors[13], positions[3], positions[4], positions[5],
               positions[15], positions[16], positions[17]);
        return 1;
    }

    // Instanced capture: 3 vertex × 2 instance → 6 record, instance-major.
    const char* inter[2] = {"vColor", "gl_Position"};
    glTransformFeedbackVaryings(p, 2, inter, 0x8C8C);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL relink instanced: %s\n", log);
        return 1;
    }
    glUseProgram(p);
    uBias = glGetUniformLocation(p, "uBias");
    glUniform1f(uBias, 0.25f);
    glBindBuffer(0x8892, xb);
    glBufferData(0x8892, 6 * sizeof(Vtx), nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    glBeginTransformFeedback(0x0004);
    glDrawArraysInstanced(0x0004, 0, 3, 2);
    glEndTransformFeedback();
    glFinish();
    if (ctx.xfbs[0].capturedCount != 6) {
        printf("FAIL instanced count=%u want 6\n", ctx.xfbs[0].capturedCount);
        return 1;
    }
    unsigned char instRaw[6 * 28] = {};
    glBindBuffer(0x8892, xb);
    glGetBufferSubData(0x8892, 0, sizeof(instRaw), instRaw);
    for (int i = 0; i < 6; ++i) {
        const float* q = (const float*)(instRaw + i * 28 + 12);
        const int base = (i % 3) * 2;
        const float wx = (base == 0) ? -1.0f : (base == 2 ? 1.0f : 0.0f);
        const float wy = (base == 4) ? 1.0f : -1.0f;
        if (std::fabs(q[0] - wx) > 0.001f || std::fabs(q[1] - wy) > 0.001f) {
            printf("FAIL instanced pos[%d]=%.3f,%.3f want %.3f,%.3f\n", i, q[0], q[1], wx, wy);
            return 1;
        }
    }
    // VS-only + XFB: hợp lệ khi rasterizer discard bật; không có FS.
    tglmt::GLuint p2 = glCreateProgram();
    glAttachShader(p2, vs);
    const char* vsOnlyVars[2] = {"vColor", "gl_Position"};
    glTransformFeedbackVaryings(p2, 2, vsOnlyVars, 0x8C8C);
    glLinkProgram(p2);
    glGetProgramiv(p2, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p2, sizeof(log), nullptr, log);
        printf("FAIL link VS-only XFB: %s\n", log);
        return 1;
    }
    glUseProgram(p2);
    uBias = glGetUniformLocation(p2, "uBias");
    if (uBias < 0) { printf("FAIL uBias VS-only\n"); return 1; }
    glUniform1f(uBias, 0.5f);
    glBindBuffer(0x8892, xb);
    glBufferData(0x8892, 3 * sizeof(Vtx), nullptr, 0x88E4);
    glBindTransformFeedback(0x8E22, 0);
    glBindBufferBase(0x8C8E, 0, xb);
    glBindBuffer(0x8892, vbo);
    // Không có FS và rasterizer discard tắt → draw phải lỗi, không capture.
    glDisable(0x8C89);
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0x0502 || ctx.xfbs[0].capturedCount != 0) {
        printf("FAIL VS-only draw without rasterizer discard\n");
        return 1;
    }
    glEndTransformFeedback();
    glEnable(0x8C89); // RASTERIZER_DISCARD
    glBeginTransformFeedback(0x0004);
    glDrawArrays(0x0004, 0, 3);
    glEndTransformFeedback();
    glDisable(0x8C89);
    glFinish();
    if (ctx.xfbs[0].capturedCount != 3) {
        printf("FAIL VS-only count=%u want 3\n", ctx.xfbs[0].capturedCount);
        return 1;
    }
    unsigned char vsOnlyRaw[3 * 28] = {};
    glBindBuffer(0x8892, xb);
    glGetBufferSubData(0x8892, 0, sizeof(vsOnlyRaw), vsOnlyRaw);
    for (int i = 0; i < 3; ++i) {
        const float* c = (const float*)(vsOnlyRaw + i * 28);
        if (std::fabs(c[1] - 0.5f) > 0.001f) {
            printf("FAIL VS-only vColor[%d][1]=%.4f want 0.5\n", i, c[1]);
            return 1;
        }
    }
    printf("test_xfb_capture_apple PASS\n");
    return 0;
}
