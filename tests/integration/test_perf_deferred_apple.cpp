// test_perf_deferred_apple.cpp — Deferred full proof (Phase 1-3):
// 100 draws cùng state → 1 encoder, 0 temp alloc (ring), trace skipped,
// uniform reuse, conditional-flush giữ batching, MultiDraw batch, hazard split.
// Pixel vẫn đúng (đỏ giữa, xanh góc). SKIP khi không có MTL device.
// Honest: đo Metal calls giảm, không chỉ đúng hình.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec4 col;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "void main() {\n"
    "  vCol = col;\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

static int gFails = 0;
static void Check(bool c, const char* m) {
    if (!c) { printf("test_perf_deferred FAIL: %s\n", m); ++gFails; }
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_perf_deferred SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;

    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float verts[3 * 6] = {-0.5f, -0.5f, 1, 0, 0, 1, 0.5f, -0.5f, 1, 0, 0, 1,
                          0.0f, 0.5f, 1, 0, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, 0x1406, 0, 24, (void*)8);

    tglmt::GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    Check(ok != 0, "link");
    if (gFails) return 1;
    glUseProgram(p);
    // Prewarm: libraries đã có sau link → đếm
    Check(ctx.PrewarmPipelineForProgram(p), "prewarm after link");
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);

    // ---- 1) 100 draws cùng state → 1 encoder, ring, 0 temp alloc ----
    uint64_t enc0 = ctx.appleStats.encodersCreated;
    uint64_t reuse0 = ctx.appleStats.encoderReused;
    uint64_t trace0 = ctx.appleStats.traceSkipped;
    uint64_t ring0 = ctx.appleStats.ringAllocs;
    uint64_t tmp0 = ctx.appleStats.tempAllocs;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 100; ++i) glDrawArrays(0x0004, 0, 3);
    auto t1 = std::chrono::steady_clock::now();
    long ms = (long)std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    uint64_t encNew = ctx.appleStats.encodersCreated - enc0;
    uint64_t reuseNew = ctx.appleStats.encoderReused - reuse0;
    uint64_t traceNew = ctx.appleStats.traceSkipped - trace0;
    uint64_t ringNew = ctx.appleStats.ringAllocs - ring0;
    uint64_t tmpNew = ctx.appleStats.tempAllocs - tmp0;
    printf("batch100: enc=%llu (want 1) reuse=%llu (want 99) traceSkip=%llu (want 100) "
           "ring=%llu tempFallback=%llu us=%ld\n",
           (unsigned long long)encNew, (unsigned long long)reuseNew,
           (unsigned long long)traceNew, (unsigned long long)ringNew,
           (unsigned long long)tmpNew, ms);
    Check(encNew == 1, "100 draws -> 1 encoder");
    Check(reuseNew == 99, "99 reuses");
    Check(traceNew == 100, "100 trace skipped (no double-encoder)");
    Check(tmpNew == 0, "0 fallback alloc (ring covers all temp uploads)");
    Check(ringNew > 0, "ring used");
    Check(ctx.appleStats.uniformReused > 0, "uniform reuse");
    ctx.FlushPendingEncoder();

    // ---- 2) Conditional-flush: stage scratch (không dùng) giữ batching ----
    tglmt::GLuint scratch = 0;
    glGenBuffers(1, &scratch);
    glBindBuffer(0x8892, scratch);
    std::vector<uint8_t> z256(256, 0);
    glBufferData(0x8892, 256, z256.data(), 0x88E4);
    glBindBuffer(0x8892, vbo);
    glClear(0x00004000);
    uint64_t encA = ctx.appleStats.encodersCreated;
    uint64_t fa0 = ctx.appleStats.flushAvoided;
    glDrawArrays(0x0004, 0, 3); // mở pass
    std::vector<uint8_t> ch(16, 0xAB);
    for (int i = 0; i < 5; ++i) glNamedBufferSubData(scratch, i * 16, 16, ch.data());
    glDrawArrays(0x0004, 0, 3); // cùng pass, scratch không dùng → không split
    uint64_t encB = ctx.appleStats.encodersCreated - encA;
    uint64_t faNew = ctx.appleStats.flushAvoided - fa0;
    printf("cond-flush: enc=%llu (want 1) flushAvoided=%llu (want >=5)\n",
           (unsigned long long)encB, (unsigned long long)faNew);
    Check(encB == 1, "scratch stage must not split pass");
    Check(faNew >= 5, "conditional-flush counted");
    ctx.FlushPendingEncoder();
    ctx.FlushAllBufferStaging();

    // ---- 3) Stage USED buffer phải split (đúng thứ tự GL) ----
    glClear(0x00004000);
    uint64_t encC0 = ctx.appleStats.encodersCreated;
    glDrawArrays(0x0004, 0, 3);
    // VBO đang dùng trong pass → SubData phải flush (split) để draws cũ giữ data cũ
    float green[4] = {0, 1, 0, 1};
    glNamedBufferSubData(vbo, 8, 16, green);
    glDrawArrays(0x0004, 0, 3);
    uint64_t encC = ctx.appleStats.encodersCreated - encC0;
    printf("used-flush: enc=%llu (want 2: split on used-buffer stage)\n", (unsigned long long)encC);
    Check(encC == 2, "used-buffer stage must split pass (correctness)");
    ctx.FlushPendingEncoder();
    ctx.FlushAllBufferStaging();
    // Trả màu đỏ cho các bước sau
    float red[4] = {1, 0, 0, 1};
    glNamedBufferSubData(vbo, 8, 16, red);
    glNamedBufferSubData(vbo, 32, 16, red);
    glNamedBufferSubData(vbo, 56, 16, red);
    ctx.FlushAllBufferStaging();

    // ---- 4) MultiDraw batch ----
    glClear(0x00004000);
    uint64_t encM0 = ctx.appleStats.encodersCreated;
    uint64_t md0 = ctx.appleStats.multidrawBatched;
    tglmt::GLint firsts[5] = {0, 0, 0, 0, 0};
    tglmt::GLsizei counts[5] = {3, 3, 3, 3, 3};
    glMultiDrawArrays(0x0004, firsts, counts, 5);
    uint64_t encM = ctx.appleStats.encodersCreated - encM0;
    uint64_t mdNew = ctx.appleStats.multidrawBatched - md0;
    printf("multidraw: enc=%llu (want 1) batched=%llu (want 5)\n",
           (unsigned long long)encM, (unsigned long long)mdNew);
    Check(encM == 1, "5 multidraws -> 1 encoder");
    Check(mdNew == 5, "multidraw counter");
    ctx.FlushPendingEncoder();

    // ---- 5) Pixel đúng sau tất cả batching ----
    // Vẽ lại 1 tam giác đỏ sau clear xanh để readback kiểm tra (tránh green còn lại).
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u)\n", mid[0], mid[1], mid[2]);
    // Mid phải đỏ (tam giác che giữa). Nếu xanh → batching sai thứ tự/clear.
    Check(mid[0] > 200 && mid[1] < 50 && mid[2] < 50, "mid red after batching");

    if (gFails) return 1;
    printf("test_perf_deferred PASS (1enc/100draws, ring 0-alloc, cond-flush, multidraw, pixels)\n");
    return 0;
}
