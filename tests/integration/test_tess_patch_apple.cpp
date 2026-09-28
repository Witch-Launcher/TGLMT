// test_tess_patch_apple.cpp — Tessellation GPU thật (triangle patch, factors 1.0).
// Căn cứ: MSL spec §5.1.1.1/§5.2.3.2 + MTL API (iOS 10+): drawPatches +
// setTessellationFactorBuffer + pipeline tess descriptor. factors 1.0 =
// passthrough đúng hành vi GL tess level 1. SKIP trung thực khi thiếu GPU/shader.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    const char* cands[] = {"shaders/tess_triangle.metal", "../shaders/tess_triangle.metal",
                           "../../shaders/tess_triangle.metal"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/tess_triangle.metal";
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) {
        printf("test_tess_patch_apple SKIP: no MTL device / null backend\n");
        return 0;
    }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_tess_patch_apple SKIP: tess_triangle.metal not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) { printf("test_tess_patch_apple FAIL: MSL compile: %s\n", err.c_str()); return 1; }
    auto pso = dev->makeTessPipeline(lib.get(), "tessVS", lib.get(), "tessFS",
                                     PixelFormat::RGBA8Unorm, 3);
    if (!pso) { printf("test_tess_patch_apple FAIL: tess pipeline nil\n"); return 1; }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_tess_patch_apple FAIL: target nil\n"); return 1; }

    struct V { float x, y, r, g, b, a; };
    V cps[3] = {{-0.5f,-0.5f, 1,0,0,1}, {0.5f,-0.5f, 1,0,0,1}, {0.0f,0.5f, 1,0,0,1}};
    auto cpBuf = dev->newBufferWithBytes(cps, sizeof(cps), StorageMode::Shared);
    // MTLTriangleTessellationFactorsHalf: 3 edge + 1 inside, half 1.0 = 0x3C00
    uint16_t factors[4] = {0x3C00, 0x3C00, 0x3C00, 0x3C00};
    auto fBuf = dev->newBufferWithBytes(factors, sizeof(factors), StorageMode::Shared);

    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    if (!enc) { printf("test_tess_patch_apple FAIL: encoder nil\n"); return 1; }
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(cpBuf.get(), 0, 0);
    enc->setTessellationFactorBuffer(fBuf.get(), 0, sizeof(factors));
    enc->drawPatches(3, 0, 1);
    if (!enc->endAndCommit()) { printf("test_tess_patch_apple FAIL: commit\n"); return 1; }

    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_tess_patch_apple FAIL: readback\n"); return 1; }
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("test_tess_patch_apple FAIL: center pixel not red\n");
        return 1;
    }
    printf("test_tess_patch_apple PASS (real GPU tessellation)\n");
    return 0;
}
