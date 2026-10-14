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
    printf("test_compute_dispatch: 64/64 SSBO values OK (0.5..63.5)\n");

    // --- Case 2: compute + uniform (uoff) → v[g]=uoff+g ---
    {
        const char* src2 = "#version 460 core\n"
            "layout(local_size_x=8) in;\nuniform float uoff;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=uoff+float(gl_GlobalInvocationID.x); }\n";
        GLuint sh2 = glCreateShader(0x91B9);
        glShaderSource(sh2, 1, &src2, nullptr); glCompileShader(sh2);
        GLuint pr2 = glCreateProgram(); glAttachShader(pr2, sh2); glLinkProgram(pr2);
        GLint ok2 = 0; glGetProgramiv(pr2, 0x8B82, &ok2);
        if (!ok2) { printf("test_compute_dispatch FAIL uniform link\n"); return 1; }
        glUseProgram(pr2);
        GLint loc = glGetUniformLocation(pr2, "uoff");
        glUniform1f(loc, 100.0f);
        float z2[64] = {0}; GLuint buf2; glGenBuffers(1, &buf2);
        glBindBuffer(0x90D2, buf2); glBufferData(0x90D2, sizeof(z2), z2, 0x88E4);
        glBindBufferBase(0x90D2, 0, buf2);
        glDispatchCompute(8, 1, 1); glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, buf2);
        const float* r2 = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z2), 0x0008 | 0x0010);
        if (!r2) { printf("test_compute_dispatch FAIL uniform map\n"); return 1; }
        int bad2 = 0;
        for (int i = 0; i < 64; ++i) if (r2[i] != 100.0f + (float)i) ++bad2;
        glUnmapBuffer(0x90D2);
        if (bad2) { printf("test_compute_dispatch FAIL uniform: %d/64 wrong\n", bad2); return 1; }
        printf("test_compute_dispatch: uniform OK (r0=100 r63=163)\n");
    }

    // --- Case 3: compute + texture sampler → v[g]=texel.r (white 1x1 → 1.0) ---
    {
        const char* src3 = "#version 460 core\n"
            "layout(local_size_x=8) in;\nuniform sampler2D t;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=texelFetch(t,ivec2(0),0).r; }\n";
        GLuint sh3 = glCreateShader(0x91B9);
        glShaderSource(sh3, 1, &src3, nullptr); glCompileShader(sh3);
        GLuint pr3 = glCreateProgram(); glAttachShader(pr3, sh3); glLinkProgram(pr3);
        GLint ok3 = 0; glGetProgramiv(pr3, 0x8B82, &ok3);
        if (!ok3) { printf("test_compute_dispatch FAIL texture link\n"); return 1; }
        GLuint tex; glGenTextures(1, &tex);
        glActiveTexture(0x84C0); glBindTexture(0x0DE1, tex);
        unsigned char white[4] = {255, 255, 255, 255};
        glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, white);
        glUseProgram(pr3);
        GLint uloc = glGetUniformLocation(pr3, "t"); glUniform1i(uloc, 0);
        float z3[64] = {0}; GLuint buf3; glGenBuffers(1, &buf3);
        glBindBuffer(0x90D2, buf3); glBufferData(0x90D2, sizeof(z3), z3, 0x88E4);
        glBindBufferBase(0x90D2, 0, buf3);
        glDispatchCompute(8, 1, 1); glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, buf3);
        const float* r3 = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z3), 0x0008 | 0x0010);
        if (!r3) { printf("test_compute_dispatch FAIL texture map\n"); return 1; }
        int bad3 = 0;
        for (int i = 0; i < 64; ++i) if (r3[i] < 0.99f) ++bad3;
        glUnmapBuffer(0x90D2);
        if (bad3) { printf("test_compute_dispatch FAIL texture: %d/64 wrong\n", bad3); return 1; }
        printf("test_compute_dispatch: texture OK (r0=1.0)\n");
    }

    printf("test_compute_dispatch: all cases OK (SSBO + uniform + texture)\n");

    // --- Case 4: P6g image — compute imageStore → texture readback ---
    {
        GLuint tex; glGenTextures(1, &tex);
        glActiveTexture(0x84C0); glBindTexture(0x0DE1, tex);
        unsigned char z4[8 * 8 * 4] = {0};
        glTexImage2D(0x0DE1, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, z4);
        glBindImageTexture(0, tex, 0, 0, 0, 0x88B9, 0x8058); // unit0 READ_WRITE RGBA8
        const char* src4 = "#version 460 core\n"
            "layout(local_size_x=8, local_size_y=8) in;\n"
            "layout(rgba8, binding=0) uniform writeonly image2D img;\n"
            "void main(){ ivec2 p=ivec2(gl_GlobalInvocationID.xy);"
            " imageStore(img, p, vec4(float(p.x)/7.0, 0, 0, 1)); }\n";
        GLuint sh4 = glCreateShader(0x91B9);
        glShaderSource(sh4, 1, &src4, nullptr); glCompileShader(sh4);
        GLuint pr4 = glCreateProgram(); glAttachShader(pr4, sh4); glLinkProgram(pr4);
        GLint ok4 = 0; glGetProgramiv(pr4, 0x8B82, &ok4);
        if (!ok4) { printf("test_compute_dispatch FAIL image link\n"); return 1; }
        glUseProgram(pr4);
        glDispatchCompute(1, 1, 1); glMemoryBarrier(0x00002000 | 0x00000020);
        unsigned char out4[8 * 8 * 4] = {0};
        glBindTexture(0x0DE1, tex); glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, out4);
        int bad4 = 0;
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x) {
            int i = (y * 8 + x) * 4;
            unsigned char expectR = (unsigned char)(float(x) / 7.0f * 255.0f + 0.5f);
            if (out4[i] < expectR - 2 || out4[i] > expectR + 2) ++bad4;
            if (out4[i + 3] != 255) ++bad4;
        }
        if (bad4) { printf("test_compute_dispatch FAIL image: %d pixels wrong\n", bad4); return 1; }
        printf("test_compute_dispatch: image OK (8x8 red gradient)\n");
    }

    printf("test_compute_dispatch: all cases OK (SSBO + uniform + texture + image)\n");
    return 0;
}
