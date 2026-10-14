// test_compute_dispatch.cpp — P6f e2e: compute shader → link → dispatch → readback.
// Trên backend Metal thật: dispatch 8 workgroup × local_size 8 = 64 thread, verify SSBO.
// Trên backend Null / không có MTLDevice: skip (return 0) — Null không dispatch được.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context apple("apple");
    if (apple.device->isNull()) {
        printf("test_compute_dispatch: SKIP (no metal device)\n");
        return 0;
    }
    Context::MakeCurrent(&apple);

    // Compute shader: write v[gid] = float(gid)+0.5 qua SSBO (std430 binding 0).
    const char* src = "#version 460 core\n"
        "layout(local_size_x=8) in;\n"
        "layout(std430,binding=0) buffer B{float v[];};\n"
        "void main(){ uint g=gl_GlobalInvocationID.x; v[g]=float(g)+0.5; }\n";
    GLuint sh = glCreateShader(0x91B9);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, 0x8B81, &ok); // COMPILE_STATUS
    if (!ok) { char l[512]; GLsizei n=0; glGetShaderInfoLog(sh,sizeof(l),&n,l);
        printf("test_compute_dispatch FAIL compile: %.200s\n", l); return 1; }

    GLuint pr = glCreateProgram();
    glAttachShader(pr, sh);
    glLinkProgram(pr);
    glGetProgramiv(pr, 0x8B82, &ok); // LINK_STATUS
    if (!ok) { char l[512]; GLsizei n=0; glGetProgramInfoLog(pr,sizeof(l),&n,l);
        printf("test_compute_dispatch FAIL link: %.200s\n", l); return 1; }

    // SSBO 64 floats
    float z[64] = {0};
    GLuint buf; glGenBuffers(1, &buf);
    glBindBuffer(0x90D2, buf); // SHADER_STORAGE_BUFFER
    glBufferData(0x90D2, sizeof(z), z, 0x88E4); // STATIC_DRAW
    glBindBufferBase(0x90D2, 0, buf); // binding 0

    glUseProgram(pr);
    glDispatchCompute(8, 1, 1); // 8 workgroup × 8 thread = 64
    glMemoryBarrier(0x0020 | 0x0040); // SHADER_STORAGE | COMMAND

    glBindBuffer(0x90D2, buf);
    const float* r = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z), 0x0008 | 0x0010);
    if (!r) { printf("test_compute_dispatch FAIL: map buffer\n"); return 1; }
    int bad = 0;
    for (int i = 0; i < 64; ++i)
        if (r[i] != (float)i + 0.5f) ++bad;
    float r0 = r[0], r63 = r[63];
    glUnmapBuffer(0x90D2);

    if (bad) {
        printf("test_compute_dispatch FAIL: %d/64 values wrong (r0=%f r63=%f)\n", bad, r0, r63);
        return 1;
    }
    printf("test_compute_dispatch: 64/64 values OK (0.5..63.5)\n");
    return 0;
}
