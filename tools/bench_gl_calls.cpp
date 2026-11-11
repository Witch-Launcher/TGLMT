// bench_gl_calls.cpp — đo CPU cost từng lệnh GL mà Minecraft vanilla gọi nhiều.
// CÔNG CỤ CHẨN ĐOÁN (không chạy trong CI). Build:
//   g++ -std=c++17 -O2 -I include -I build-apple/generated tools/bench_gl_calls.cpp \
//       -o /tmp/bench -L build-apple -ltglmt -Wl,-rpath,$PWD/build-apple && /tmp/bench
// Cần backend Metal (build-apple) — Null backend không encode → đo sai.
//
// Log thật trên máy: vanilla ~9274 GL call/frame, glMs=123ms/frame → ~13µs/call.
// Mục tiêu: biết lệnh nào đắt nhất trong TGLMT để tối ưu.
//
// Dùng backend Metal (build-apple) để đo đúng đường encode thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static const int kN = 20000;

template <class F>
static void Bench(const char* name, F fn) {
    // warmup
    for (int i = 0; i < 200; ++i) fn();
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kN; ++i) fn();
    auto t1 = std::chrono::steady_clock::now();
    double us = std::chrono::duration<double, std::micro>(t1 - t0).count() / kN;
    printf("%-34s %8.3f us/call\n", name, us);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("no metal device\n"); return 0; }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint tex = 0, buf = 0, prog = 0, vao = 0, fbo = 0, rbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenBuffers(1, &buf);
    glBindBuffer(0x8892, buf);
    glBufferData(0x8892, 4096, nullptr, 0x88E4);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(0x8D41, rbo);
    glRenderbufferStorage(0x8D41, 0x81A5, 64, 64);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    glFramebufferRenderbuffer(0x8D40, 0x8D00, 0x8D41, rbo);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5) { printf("FBO incomplete!\n"); return 1; }
    glBindFramebuffer(0x8D40, 0);
    glBindVertexArray(0);

    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec4 aPos;\n"
        "layout(std140, binding = 0) uniform Globals { mat4 mvp; vec4 tint; } g;\n"
        "out vec4 v;\n"
        "void main() { v = g.tint; gl_Position = aPos * g.mvp; }\n";
    const char* fs =
        "#version 460 core\n"
        "layout(std140, binding = 0) uniform Globals { mat4 mvp; vec4 tint; } g;\n"
        "in vec4 v;\n"
        "out vec4 o;\n"
        "void main() { o = v * g.tint; }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    if (!ok) { printf("link fail\n"); return 1; }
    glUseProgram(prog);

    // UBO đã map (như MC persistent map)
    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(0x8A11, ubo);
    glBufferStorage(0x8A11, 80, nullptr, 0x0043);
    glBindBufferBase(0x8A11, 0, ubo);
    void* mp = glMapBufferRange(0x8A11, 0, 80, 0x0001 | 0x0002 | 0x0040);
    if (mp) { float* m = (float*)mp; for (int i = 0; i < 20; ++i) m[i] = 1.0f; }

    GLuint sampler = 0;
    glGenSamplers(1, &sampler);
    glBindSampler(0, sampler);

    printf("--- lệnh MC gọi nhiều ---\n");
    Bench("glBindBuffer", [&] { glBindBuffer(0x8892, buf); });
    Bench("glBindBufferRange", [&] { glBindBufferRange(0x8A11, 0, ubo, 0, 80); });
    Bench("glBindFramebuffer", [&] { glBindFramebuffer(0x8D40, fbo); });
    Bench("glBindTexture", [&] { glBindTexture(0x0DE1, tex); });
    Bench("glTexParameteri", [&] { glTexParameteri(0x0DE1, 0x2801, 0x2601); });
    Bench("glUniform1i", [&] { glUniform1i(0, 0); });
    Bench("glEnableVertexAttribArray", [&] { glEnableVertexAttribArray(0); });
    Bench("glVertexAttribPointer", [&] { glVertexAttribPointer(0, 4, 0x1406, 0, 16, nullptr); });
    Bench("glGetError", [&] { (void)glGetError(); });
    Bench("glGetIntegerv", [&] { GLint x; glGetIntegerv(0x8B82, &x); });

    printf("--- draw ---\n");
    glBindVertexArray(vao);
    glBindBuffer(0x8893, buf);
    glBindFramebuffer(0x8D40, fbo);
    glViewport(0, 0, 64, 64);
    glClear(0x4000 | 0x0100);
    Bench("glDrawElementsInstancedBaseVertex",
          [&] { glDrawElementsInstancedBaseVertex(0x0004, 6, 0x1403, nullptr, 1, 0); });
    Bench("glDrawArraysInstanced", [&] { glDrawArraysInstanced(0x0004, 0, 6, 1); });
    Bench("glClear", [&] { glClear(0x4000 | 0x0100); });
    return 0;
}
