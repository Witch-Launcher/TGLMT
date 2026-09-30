// test_ir_full_apple.cpp — Deferred State Translation đầy đủ:
// 1) Pipeline-key cache: 2 draws cùng inputs → 1 bridge lookup + 1 skip.
// 2) Buffer staging: 10× SubData kề nhau → 10 coalesced, 1 flush, pixels đúng.
// 3) Texture staging: 4× TexSubImage → 4 coalesced, 1 flush (bbox gộp).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
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

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_ir_full SKIP: no MTL device\n"); return 0; }
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
    if (!ok) { printf("FAIL link\n"); return 1; }
    glUseProgram(p);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);

    // ---- 1) Pipeline-key cache ----
    uint64_t lk0 = ctx.appleStats.pipelineLookups;
    uint64_t sk0 = ctx.appleStats.pipelineLookupSkipped;
    glDrawArrays(0x0004, 0, 3);
    glDrawArrays(0x0004, 0, 3);
    uint64_t lkNew = ctx.appleStats.pipelineLookups - lk0;
    uint64_t skNew = ctx.appleStats.pipelineLookupSkipped - sk0;
    printf("pipeline: lookups=%llu (want 1) skipped=%llu (want 1)\n",
           (unsigned long long)lkNew, (unsigned long long)skNew);
    if (lkNew != 1 || skNew != 1) {
        printf("FAIL pipeline cache\n");
        return 1;
    }
    // Flush để cách ly với bước buffer (kẻo SubData flush encoder làm nhiễu đếm).
    ctx.FlushPendingEncoder();
    ctx.FlushAllBufferStaging();
    ctx.FlushAllTextureStaging();

    // ---- 2) Buffer staging + coalesce ----
    // 2a) Coalesce thuần: scratch 256B, 10 updates kề nhau 16B → 10 staged, 1 flush.
    tglmt::GLuint scratch = 0;
    glGenBuffers(1, &scratch);
    glBindBuffer(0x8892, scratch);
    std::vector<uint8_t> zeros(256, 0);
    glBufferData(0x8892, 256, zeros.data(), 0x88E4);
    glBindBuffer(0x8892, vbo); // trả VBO về để draw sau không lệch
    uint64_t bc0 = ctx.appleStats.bufferCoalesced;
    uint64_t bf0 = ctx.appleStats.bufferFlushes;
    std::vector<uint8_t> chunk(16, 0xAB);
    for (int i = 0; i < 10; ++i) glNamedBufferSubData(scratch, i * 16, 16, chunk.data());
    uint64_t bcNew = ctx.appleStats.bufferCoalesced - bc0;
    printf("buffer staging: coalesced=%llu (want 10)\n", (unsigned long long)bcNew);
    if (bcNew != 10) { printf("FAIL buffer coalesced\n"); return 1; }
    ctx.FlushBufferStaging(scratch);
    uint64_t bfNew = ctx.appleStats.bufferFlushes - bf0;
    printf("buffer flush: flushes=%llu (want 1)\n", (unsigned long long)bfNew);
    if (bfNew != 1) { printf("FAIL buffer flushes (10 adjacents must merge to 1)\n"); return 1; }
    // Verify GPU đã nhận (shadow→gpu sau flush): đọc lại shadow (authoritative) + gpu length.
    {
        auto it = ctx.buffers.find(scratch);
        if (it == ctx.buffers.end() || !it->second.gpu) { printf("FAIL scratch gpu\n"); return 1; }
        const uint8_t* g = (const uint8_t*)it->second.gpu->contents();
        for (int i = 0; i < 160; ++i)
            if (g[i] != 0xAB) { printf("FAIL scratch gpu byte %d = %u\n", i, g[i]); return 1; }
    }
    // 2b) Draw integration: đổi màu cả 3 đỉnh sang xanh lá qua staged SubData rồi draw.
    // VBO 72B: col đỉnh i ở offset 8+i*24 (float4). Ghi (0,1,0,1) ×3 → mid xanh lá.
    // Không flush tay — draw phải tự flush staging, nếu không mid vẫn đỏ (stale).
    {
        float green[4] = {0, 1, 0, 1};
        glNamedBufferSubData(vbo, 8, 16, green);
        glNamedBufferSubData(vbo, 32, 16, green);
        glNamedBufferSubData(vbo, 56, 16, green);
    }
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) { printf("FAIL gl error after buffer draw\n"); return 1; }
    {
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        printf("buffer-draw mid=(%u,%u,%u) (want green)\n", mid[0], mid[1], mid[2]);
        if (!(mid[1] > 200 && mid[0] < 4 && mid[2] < 4)) {
            printf("FAIL buffer-draw pixels (staging flush before draw broken)\n");
            return 1;
        }
    }
    ctx.FlushPendingEncoder();
    ctx.FlushAllBufferStaging();
    ctx.FlushAllTextureStaging();

    // ---- 3) Texture staging + coalesce ----
    tglmt::GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    std::vector<uint8_t> white(32 * 32 * 4, 255);
    glTexImage2D(0x0DE1, 0, 0x8058, 32, 32, 0, 0x1908, 0x1401, white.data());
    if (glGetError() != 0) { printf("FAIL teximage\n"); return 1; }
    uint64_t tc0 = ctx.appleStats.texCoalesced;
    uint64_t tf0 = ctx.appleStats.texFlushes;
    std::vector<uint8_t> red(8 * 8 * 4, 0);
    for (int i = 0; i < 8 * 8; ++i) { red[i * 4] = 255; red[i * 4 + 3] = 255; }
    // 4 updates kề nhau theo hàng (0,0),(8,0),(16,0),(24,0) size 8x8 → 1 bbox 32x8.
    glTexSubImage2D(0x0DE1, 0, 0, 0, 8, 8, 0x1908, 0x1401, red.data());
    glTexSubImage2D(0x0DE1, 0, 8, 0, 8, 8, 0x1908, 0x1401, red.data());
    glTexSubImage2D(0x0DE1, 0, 16, 0, 8, 8, 0x1908, 0x1401, red.data());
    glTexSubImage2D(0x0DE1, 0, 24, 0, 8, 8, 0x1908, 0x1401, red.data());
    uint64_t tcNew = ctx.appleStats.texCoalesced - tc0;
    printf("texture staging: coalesced=%llu (want 4)\n", (unsigned long long)tcNew);
    if (tcNew != 4) { printf("FAIL tex coalesced\n"); return 1; }
    ctx.FlushTextureStaging(tex);
    uint64_t tfNew = ctx.appleStats.texFlushes - tf0;
    printf("texture flush: flushes=%llu (want 1)\n", (unsigned long long)tfNew);
    if (tfNew != 1) { printf("FAIL tex flushes (4 adjacents must merge to 1)\n"); return 1; }
    if (glGetError() != 0) { printf("FAIL gl error after tex flush\n"); return 1; }
    // Shadow phải đúng (hàng đầu đỏ): hàng 0 byte đầu = (255,0,0,255).
    {
        auto it = ctx.textures.find(tex);
        if (it == ctx.textures.end() || it->second.pixels.size() < 4) {
            printf("FAIL tex shadow\n");
            return 1;
        }
        auto& px = it->second.pixels;
        if (!(px[0] == 255 && px[1] == 0 && px[2] == 0 && px[3] == 255)) {
            printf("FAIL tex shadow pixels (%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
            return 1;
        }
    }

    printf("test_ir_full PASS (pipeline 1+1, buffer 10->1, tex 4->1)\n");
    return 0;
}
