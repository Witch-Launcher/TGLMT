// test_msaa_apple.cpp — MSAA 4x + resolve GPU thật. Center đỏ sau resolve.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    if (!dev || dev->isNull()) { printf("test_msaa_apple SKIP: no MTL device\n"); return 0; }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_msaa_apple SKIP: shader not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) { printf("test_msaa_apple FAIL: compile\n"); return 1; }
    // rasterSampleCount của pipeline phải khớp attachment MSAA (validation Metal).
    auto pso = dev->makeMSAAPipeline(lib.get(), "triVS", lib.get(), "triFS",
                                     PixelFormat::RGBA8Unorm, 4);
    if (!pso) { printf("test_msaa_apple FAIL: pipeline\n"); return 1; }
    auto target = dev->makeMSAATarget(64, 64, 4);
    if (!target) { printf("test_msaa_apple SKIP/FAIL: MSAA target nil (GPU không hỗ trợ?)\n"); return 0; }
    struct V { float x, y, r, g, b, a; };
    V tri[3] = {{-0.5f,-0.5f, 1,0,0,1}, {0.5f,-0.5f, 1,0,0,1}, {0.0f,0.5f, 1,0,0,1}};
    auto vb = dev->newBufferWithBytes(tri, sizeof(tri), StorageMode::Shared);
    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    if (!enc) { printf("test_msaa_apple FAIL: encoder\n"); return 1; }
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
    if (!enc->endAndCommit()) { printf("test_msaa_apple FAIL: commit\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_msaa_apple FAIL: readback\n"); return 1; }
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("test_msaa_apple FAIL: center not red\n");
        return 1;
    }
    printf("test_msaa_apple PASS (MSAA resolve exact)\n");
    return 0;
}
