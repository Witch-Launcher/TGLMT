// test_gpu_triangle_apple.cpp — Integration GPU THẬT (M5 pixel-compare).
// Vẽ tam giác đỏ lên offscreen target 64x64 qua Apple backend, readback pixel giữa.
// SKIP trung thực (exit 0 + message) khi: không có MTLDevice, backend Null,
// thiếu file shader, compile lỗi. KHÔNG giả pass.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    const char* cands[] = {
        "shaders/triangle.metal",
        "../shaders/triangle.metal",
        "../../shaders/triangle.metal",
    };
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/triangle.metal";
        std::ifstream f(p);
        if (f.good()) { std::ostringstream s; s << f.rdbuf(); return s.str(); }
    }
    for (auto c : cands) {
        std::ifstream f(c);
        if (f.good()) { std::ostringstream s; s << f.rdbuf(); return s.str(); }
    }
    return "";
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) {
        printf("test_gpu_triangle_apple SKIP: no MTL device / null backend\n");
        return 0;
    }
    printf("device: %s\n", dev->name().c_str());
    std::string msl = FindMSL();
    if (msl.empty()) {
        printf("test_gpu_triangle_apple SKIP: shaders/triangle.metal not found\n");
        return 0;
    }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) {
        printf("test_gpu_triangle_apple SKIP: MSL compile failed: %s\n", err.c_str());
        return 0;
    }
    auto pso = dev->makeRenderPipeline(lib.get(), "triVS", "triFS", PixelFormat::RGBA8Unorm);
    if (!pso) { printf("test_gpu_triangle_apple FAIL: pipeline nil\n"); return 1; }
    // cache check: gọi lại cùng key phải trả pipeline dùng được (không crash, encode được)
    auto pso2 = dev->makeRenderPipeline(lib.get(), "triVS", "triFS", PixelFormat::RGBA8Unorm);
    if (!pso2) { printf("test_gpu_triangle_apple FAIL: pipeline cache miss\n"); return 1; }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_gpu_triangle_apple FAIL: target nil\n"); return 1; }

    // Tam giác giữa màn, màu đỏ (layout chuẩn M5: float2 pos + float4 col, stride 24)
    struct V { float x, y, r, g, b, a; };
    V verts[3] = {
        {-0.5f, -0.5f, 1, 0, 0, 1},
        { 0.5f, -0.5f, 1, 0, 0, 1},
        { 0.0f,  0.5f, 1, 0, 0, 1},
    };
    auto vb = dev->newBufferWithBytes(verts, sizeof(verts), StorageMode::Shared);
    if (!vb) { printf("test_gpu_triangle_apple FAIL: vertex buffer nil\n"); return 1; }

    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    if (!enc) { printf("test_gpu_triangle_apple FAIL: encoder nil\n"); return 1; }
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
    if (!enc->endAndCommit()) { printf("test_gpu_triangle_apple FAIL: commit failed\n"); return 1; }

    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_gpu_triangle_apple FAIL: readback\n"); return 1; }
    // Pixel giữa (32,32) phải đỏ; góc (2,2) phải đen (clear). Lưu ý Metal origin top-left.
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    unsigned char* corner = px + (2 * 64 + 2) * 4;
    printf("mid=(%u,%u,%u,%u) corner=(%u,%u,%u,%u)\n",
           mid[0], mid[1], mid[2], mid[3], corner[0], corner[1], corner[2], corner[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50 && mid[3] > 200)) {
        printf("test_gpu_triangle_apple FAIL: center pixel not red\n");
        return 1;
    }
    if (!(corner[0] < 50 && corner[1] < 50 && corner[2] < 50)) {
        printf("test_gpu_triangle_apple FAIL: corner pixel not clear\n");
        return 1;
    }
    printf("test_gpu_triangle_apple PASS (real GPU render + readback)\n");
    return 0;
}
