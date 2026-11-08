// test_compute_dispatch.cpp — P6f e2e: compute shader → link → dispatch → readback.
// Trên backend Metal thật: dispatch 8 workgroup × local_size 8 = 64 thread, verify SSBO.
// Trên backend Null / không có MTLDevice: skip (return 0) — Null không dispatch được.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstdlib>
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
    // ARB_ES3_1_compatibility: barrier theo region; TGLMT hạ xuống full barrier.
    glMemoryBarrierByRegion(0x0020 | 0x0040); // SHADER_STORAGE | COMMAND
    if (glGetError() != 0) { printf("test_compute_dispatch FAIL: MemoryBarrierByRegion\n"); return 1; }

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

    // --- Case 5: P9 atomic — 64 threads increment, verify unique + counter=64 ---
    {
        const char* src5 = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(binding=0, offset=0) uniform atomic_uint ac;\n"
            "layout(std430,binding=0) buffer B{uint v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=atomicCounterIncrement(ac); }\n";
        GLuint sh5 = glCreateShader(0x91B9);
        glShaderSource(sh5, 1, &src5, nullptr); glCompileShader(sh5);
        GLuint pr5 = glCreateProgram(); glAttachShader(pr5, sh5); glLinkProgram(pr5);
        GLint ok5 = 0; glGetProgramiv(pr5, 0x8B82, &ok5);
        if (!ok5) { printf("test_compute_dispatch FAIL atomic link\n"); return 1; }
        GLuint abuf; glGenBuffers(1, &abuf);
        glBindBuffer(0x92C0, abuf); // ATOMIC_COUNTER_BUFFER
        unsigned int zero = 0;
        glBufferData(0x92C0, 4, &zero, 0x88E4);
        glBindBufferBase(0x92C0, 0, abuf);
        unsigned int z5[64] = {0}; GLuint sbuf; glGenBuffers(1, &sbuf);
        glBindBuffer(0x90D2, sbuf); glBufferData(0x90D2, sizeof(z5), z5, 0x88E4);
        glBindBufferBase(0x90D2, 0, sbuf);
        glUseProgram(pr5);
        glDispatchCompute(8, 1, 1); glMemoryBarrier(0x00000040 | 0x00002000);
        glBindBuffer(0x90D2, sbuf);
        const unsigned int* r5 = (const unsigned int*)glMapBufferRange(0x90D2, 0, sizeof(z5), 0x0008 | 0x0010);
        if (!r5) { printf("test_compute_dispatch FAIL atomic map\n"); return 1; }
        int seen[64] = {0};
        for (int i = 0; i < 64; ++i) if (r5[i] < 64) seen[r5[i]] = 1;
        int uniq = 0; for (int i = 0; i < 64; ++i) uniq += seen[i];
        glUnmapBuffer(0x90D2);
        glBindBuffer(0x92C0, abuf);
        const unsigned int* ac = (const unsigned int*)glMapBufferRange(0x92C0, 0, 4, 0x0008 | 0x0010);
        unsigned int cnt = ac ? ac[0] : 0;
        if (ac) glUnmapBuffer(0x92C0);
        if (uniq != 64 || cnt != 64) {
            printf("test_compute_dispatch FAIL atomic: uniq=%d cnt=%u\n", uniq, cnt); return 1;
        }
        printf("test_compute_dispatch: atomic OK (uniq=64 cnt=64)\n");
    }

    printf("test_compute_dispatch: all cases OK (SSBO + uniform + texture + image + atomic)\n");

    // --- Case 6: P1 COMPUTE_WORK_GROUP_SIZE + glDispatchComputeIndirect ---
    {
        // Shader: v[i] = i + 7 (local 8). Args CPU-write {8,1,1} qua DISPATCH_INDIRECT.
        const char* src6 = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=float(gl_GlobalInvocationID.x)+7.0; }\n";
        GLuint sh6 = glCreateShader(0x91B9);
        glShaderSource(sh6, 1, &src6, nullptr); glCompileShader(sh6);
        GLuint pr6 = glCreateProgram(); glAttachShader(pr6, sh6); glLinkProgram(pr6);
        GLint ok6 = 0; glGetProgramiv(pr6, 0x8B82, &ok6);
        if (!ok6) { printf("test_compute_dispatch FAIL indirect link\n"); return 1; }
        // Iris ComputeProgram đọc query này ngay sau link — phải khớp layout.
        GLint wsz[3] = {0, 0, 0};
        glGetProgramiv(pr6, 0x8267, wsz);
        if (wsz[0] != 8 || wsz[1] != 1 || wsz[2] != 1) {
            printf("test_compute_dispatch FAIL work group size {%d,%d,%d}\n",
                   wsz[0], wsz[1], wsz[2]);
            return 1;
        }
        float z6[64] = {0};
        GLuint buf6; glGenBuffers(1, &buf6);
        glBindBuffer(0x90D2, buf6); glBufferData(0x90D2, sizeof(z6), z6, 0x88E4);
        glBindBufferBase(0x90D2, 0, buf6);
        GLuint ib; glGenBuffers(1, &ib);
        glBindBuffer(0x90EE, ib); // DISPATCH_INDIRECT_BUFFER
        unsigned int args[3] = {8, 1, 1};
        glBufferData(0x90EE, sizeof(args), args, 0x88E4);
        glUseProgram(pr6);
        glDispatchComputeIndirect(0);
        glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, buf6);
        const float* r6 = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z6), 0x0008 | 0x0010);
        if (!r6) { printf("test_compute_dispatch FAIL indirect map\n"); return 1; }
        int bad6 = 0;
        for (int i = 0; i < 64; ++i) if (r6[i] != (float)i + 7.0f) ++bad6;
        glUnmapBuffer(0x90D2);
        if (bad6) { printf("test_compute_dispatch FAIL indirect: %d/64 wrong\n", bad6); return 1; }
        printf("test_compute_dispatch: indirect OK (wgsz {8,1,1}, 64/64)\n");
    }

    // --- Case 7: P1 chain 2 dispatch deferred — B đọc SSBO A ghi (barrier giữa) ---
    {
        const char* srcA = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430,binding=0) buffer A{uvec4 v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x].x=gl_GlobalInvocationID.x*3u+1u; }\n";
        const char* srcB = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430,binding=0) buffer A{uvec4 v[];};\n"
            "layout(std430,binding=1) buffer W{uvec4 w[];};\n"
            "void main(){ w[gl_GlobalInvocationID.x].x = v[gl_GlobalInvocationID.x].x * 2u; }\n";
        GLuint shA = glCreateShader(0x91B9); glShaderSource(shA, 1, &srcA, nullptr); glCompileShader(shA);
        GLuint shB = glCreateShader(0x91B9); glShaderSource(shB, 1, &srcB, nullptr); glCompileShader(shB);
        GLuint prA = glCreateProgram(); glAttachShader(prA, shA); glLinkProgram(prA);
        GLuint prB = glCreateProgram(); glAttachShader(prB, shB); glLinkProgram(prB);
        GLint okA = 0, okB = 0;
        glGetProgramiv(prA, 0x8B82, &okA); glGetProgramiv(prB, 0x8B82, &okB);
        if (!okA || !okB) { printf("test_compute_dispatch FAIL chain link (%d,%d)\n", okA, okB); return 1; }
        // 8 group × 8 thread = 64 phần tử uvec4 (stride 16B/lane đúng std430).
        unsigned int zA[64 * 4] = {0}, zB[64 * 4] = {0};
        GLuint bufA, bufB;
        glGenBuffers(1, &bufA); glGenBuffers(1, &bufB);
        glBindBuffer(0x90D2, bufA); glBufferData(0x90D2, sizeof(zA), zA, 0x88E4);
        glBindBuffer(0x90D2, bufB); glBufferData(0x90D2, sizeof(zB), zB, 0x88E4);
        glBindBufferBase(0x90D2, 0, bufA);
        glUseProgram(prA);
        glDispatchCompute(8, 1, 1);                    // CB1 (deferred)
        glMemoryBarrier(0x0020 | 0x0040);              // commit CB1
        glBindBufferBase(0x90D2, 0, bufA);
        glBindBufferBase(0x90D2, 1, bufB);
        glUseProgram(prB);
        glDispatchCompute(8, 1, 1);                    // CB2 (deferred), đọc A ghi B
        glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, bufB);
        const unsigned int* rB = (const unsigned int*)glMapBufferRange(0x90D2, 0, sizeof(zB), 0x0008 | 0x0010);
        if (!rB) { printf("test_compute_dispatch FAIL chain map\n"); return 1; }
        int badB = 0;
        for (int i = 0; i < 64; ++i) {
            unsigned int want = ((unsigned int)i * 3u + 1u) * 2u;
            if (rB[i * 4] != want) ++badB;
        }
        glUnmapBuffer(0x90D2);
        if (badB) { printf("test_compute_dispatch FAIL chain: %d/64 wrong\n", badB); return 1; }
        printf("test_compute_dispatch: chain OK (B đọc A qua 2 CB deferred)\n");
    }

    // --- Case 8: P1 glDispatchCompute(0,1,1) = no work (GL §7.12.1), không fault ---
    {
        const char* src8 = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=123.0; }\n";
        GLuint sh8 = glCreateShader(0x91B9);
        glShaderSource(sh8, 1, &src8, nullptr); glCompileShader(sh8);
        GLuint pr8 = glCreateProgram(); glAttachShader(pr8, sh8); glLinkProgram(pr8);
        float z8[64];
        for (int i = 0; i < 64; ++i) z8[i] = 5.0f;
        GLuint buf8; glGenBuffers(1, &buf8);
        glBindBuffer(0x90D2, buf8); glBufferData(0x90D2, sizeof(z8), z8, 0x88E4);
        glBindBufferBase(0x90D2, 0, buf8);
        glUseProgram(pr8);
        glDispatchCompute(0, 1, 1); // x=0 → KHÔNG thread nào chạy
        glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, buf8);
        const float* r8 = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z8), 0x0008 | 0x0010);
        if (!r8) { printf("test_compute_dispatch FAIL zero-group map\n"); return 1; }
        int bad8 = 0;
        for (int i = 0; i < 64; ++i) if (r8[i] != 5.0f) ++bad8;
        glUnmapBuffer(0x90D2);
        if (bad8) { printf("test_compute_dispatch FAIL zero-group: %d/64 changed\n", bad8); return 1; }
        printf("test_compute_dispatch: zero-group OK (không thread nào chạy)\n");
    }

    // --- Case 9: TGLMT_COMPUTE_SYNC=1 escape hatch (commit+wait mỗi dispatch) ---
    {
        setenv("TGLMT_COMPUTE_SYNC", "1", 1);
        Context sync("apple");
        if (!sync.device->isNull()) {
            Context::MakeCurrent(&sync);
            const char* src9 = "#version 460 core\n"
                "layout(local_size_x=8) in;\n"
                "layout(std430,binding=0) buffer B{float v[];};\n"
                "void main(){ v[gl_GlobalInvocationID.x]=float(gl_GlobalInvocationID.x); }\n";
            GLuint sh9 = glCreateShader(0x91B9);
            glShaderSource(sh9, 1, &src9, nullptr); glCompileShader(sh9);
            GLuint pr9 = glCreateProgram(); glAttachShader(pr9, sh9); glLinkProgram(pr9);
            GLint ok9 = 0; glGetProgramiv(pr9, 0x8B82, &ok9);
            if (!ok9) { printf("test_compute_dispatch FAIL sync link\n"); return 1; }
            float z9[64] = {0};
            GLuint buf9; glGenBuffers(1, &buf9);
            glBindBuffer(0x90D2, buf9); glBufferData(0x90D2, sizeof(z9), z9, 0x88E4);
            glBindBufferBase(0x90D2, 0, buf9);
            glUseProgram(pr9);
            glDispatchCompute(8, 1, 1); // sync mode: commit+wait ngay
            // KHÔNG barrier/map-sync — sync mode đã wait, contents phải sẵn sàng.
            const float* r9 = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z9), 0x0008 | 0x0010);
            if (!r9) { printf("test_compute_dispatch FAIL sync map\n"); return 1; }
            int bad9 = 0;
            for (int i = 0; i < 64; ++i) if (r9[i] != (float)i) ++bad9;
            glUnmapBuffer(0x90D2);
            if (bad9) { printf("test_compute_dispatch FAIL sync mode: %d/64 wrong\n", bad9); return 1; }
            printf("test_compute_dispatch: sync mode OK (TGLMT_COMPUTE_SYNC=1)\n");
        }
        unsetenv("TGLMT_COMPUTE_SYNC");
    }

    printf("test_compute_dispatch: all P1 cases OK (indirect + chain + zero + sync)\n");
    return 0;
}
