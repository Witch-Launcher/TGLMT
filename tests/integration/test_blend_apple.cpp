// test_blend_apple.cpp — Blending GPU thật (SRC_ALPHA/ONE_MINUS_SRC_ALPHA).
// Clear xanh, vẽ đỏ alpha 0.5 → center ≈ (127,0,127). Kiểm tra công thức blend đúng.
#include "tglmt/MetalInterface.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    std::string cands[] = {"shaders/triangle.metal", "../shaders/triangle.metal",
                           "../../shaders/triangle.metal"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/triangle.metal";
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_blend_apple SKIP: no MTL device\n"); return 0; }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_blend_apple SKIP: shader not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) { printf("test_blend_apple FAIL: compile\n"); return 1; }
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), glBlendEquation(FUNC_ADD)
    AttachmentBlend b;
    b.enabled = true;
    b.srcRGB = 0x0302; b.dstRGB = 0x0303; b.srcAlpha = 0x0302; b.dstAlpha = 0x0303;
    b.rgbOp = 0x8006; b.alphaOp = 0x8006;
    auto pso = dev->makeBlendPipeline(lib.get(), "triVS", lib.get(), "triFS",
                                      PixelFormat::RGBA8Unorm, &b, 1);
    if (!pso) { printf("test_blend_apple FAIL: pipeline\n"); return 1; }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    struct V { float x, y, r, g, b, a; };
    V tri[3] = {{-0.9f,-0.9f, 1,0,0,0.5f}, {0.9f,-0.9f, 1,0,0,0.5f}, {0.0f,0.9f, 1,0,0,0.5f}};
    auto vb = dev->newBufferWithBytes(tri, sizeof(tri), StorageMode::Shared);
    ClearColor blue{0, 0, 1, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), blue);
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
    if (!enc->endAndCommit()) { printf("test_blend_apple FAIL: commit\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_blend_apple FAIL: readback\n"); return 1; }
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    // kỳ vọng: 255*0.5=127.5 → 127/128, xanh 127/128
    if (std::abs((int)mid[0] - 127) > 2 || mid[1] > 2 || std::abs((int)mid[2] - 127) > 2) {
        printf("test_blend_apple FAIL: not blended\n");
        return 1;
    }
    printf("test_blend_apple PASS (blend formula exact)\n");
    return 0;
}
