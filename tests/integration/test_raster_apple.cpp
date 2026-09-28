// test_raster_apple.cpp — Wireframe (fillMode), scissor, depth test trên GPU thật.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL(const char* name) {
    std::string cands[] = {std::string("shaders/") + name, std::string("../shaders/") + name,
                           std::string("../../shaders/") + name};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/" + name;
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}
static void Readback(tglmt::metal::IRenderTarget* t, unsigned char* px) {
    memset(px, 0, 64 * 64 * 4);
    if (!t->readback(px, 64 * 4)) { printf("READBACK FAIL\n"); exit(1); }
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_raster_apple SKIP: no MTL device\n"); return 0; }
    std::string msl = FindMSL("triangle.metal");
    std::string rmsl = FindMSL("raster.metal");
    if (msl.empty() || rmsl.empty()) { printf("test_raster_apple SKIP: shader not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    auto rlib = dev->compileLibrary(rmsl, err);
    if (!lib || !rlib) { printf("test_raster_apple FAIL: compile\n"); return 1; }
    auto fill = dev->makeRenderPipeline(lib.get(), "triVS", lib.get(), "triFS", PixelFormat::RGBA8Unorm);
    auto depthPso = dev->makeDepthPipeline(rlib.get(), "depthVS", lib.get(), "triFS", PixelFormat::RGBA8Unorm);
    if (!fill || !depthPso) { printf("test_raster_apple FAIL: pipeline\n"); return 1; }

    struct V { float x, y, r, g, b, a; };
    // Big triangle bao gần hết màn hình (center chắc chắn trong, biên gần mép)
    V big[3] = {{-0.9f,-0.9f, 1,0,0,1}, {0.9f,-0.9f, 1,0,0,1}, {0.0f,0.9f, 1,0,0,1}};
    auto vb = dev->newBufferWithBytes(big, sizeof(big), StorageMode::Shared);
    Viewport vp{0, 0, 64, 64, 0, 1};
    ClearColor black{0, 0, 0, 1};
    unsigned char px[64 * 64 * 4];

    // 1. Fill: center đỏ
    {
        auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
        auto enc = dev->makeRenderEncoder(target.get(), fill.get(), black);
        enc->setViewport(vp);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) { printf("FAIL fill commit\n"); return 1; }
        Readback(target.get(), px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        if (!(mid[0] > 200 && mid[1] < 50)) { printf("FAIL fill mid\n"); return 1; }
    }
    // 2. Wireframe (glPolygonMode LINE): center phải đen (chỉ biên), biên phải đỏ
    {
        auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
        auto enc = dev->makeRenderEncoder(target.get(), fill.get(), black);
        enc->setViewport(vp);
        enc->setTriangleFillModeLines(true);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) { printf("FAIL wire commit\n"); return 1; }
        Readback(target.get(), px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        if (!(mid[0] < 50 && mid[1] < 50 && mid[2] < 50)) {
            printf("FAIL wire mid not clear (%u,%u,%u)\n", mid[0], mid[1], mid[2]);
            return 1;
        }
        // biên dưới tam giác (y≈60) phải có pixel đỏ
        bool edge = false;
        for (int x = 4; x < 60 && !edge; ++x) {
            unsigned char* p = px + (60 * 64 + x) * 4;
            if (p[0] > 200 && p[1] < 50) edge = true;
        }
        if (!edge) { printf("FAIL wire edge missing\n"); return 1; }
    }
    // 3. Scissor loại center → center đen
    {
        auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
        auto enc = dev->makeRenderEncoder(target.get(), fill.get(), black);
        enc->setViewport(vp);
        ScissorRect sc{0, 0, 10, 10}; // góc top-left Metal, xa center
        enc->setScissorRect(sc);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) { printf("FAIL scissor commit\n"); return 1; }
        Readback(target.get(), px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        if (!(mid[0] < 50)) { printf("FAIL scissor mid\n"); return 1; }
    }
    // 4. Depth LESS: vẽ xanh-near(z=0.2) trước, đỏ-far(z=0.8) sau → center vẫn xanh-near
    {
        auto target = dev->makeRenderTargetWithDepth(64, 64, PixelFormat::RGBA8Unorm);
        if (!target) { printf("FAIL depth target\n"); return 1; }
        auto ds = dev->makeDepthStencilState(0x0201 /*GL_LESS*/, true);
        if (!ds) { printf("FAIL depth state\n"); return 1; }
        struct DV { float x, y, z, r, g, b; };
        DV near_[3] = {{-0.9f,-0.9f, 0.2f, 0,0,1}, {0.9f,-0.9f, 0.2f, 0,0,1}, {0.0f,0.9f, 0.2f, 0,0,1}};
        DV far[3] = {{-0.9f,-0.9f, 0.8f, 1,0,0}, {0.9f,-0.9f, 0.8f, 1,0,0}, {0.0f,0.9f, 0.8f, 1,0,0}};
        auto nb = dev->newBufferWithBytes(near_, sizeof(near_), StorageMode::Shared);
        auto fb = dev->newBufferWithBytes(far, sizeof(far), StorageMode::Shared);
        auto enc = dev->makeRenderEncoder(target.get(), depthPso.get(), black);
        enc->setViewport(vp);
        enc->setDepthStencilState(ds.get());
        enc->setVertexBuffer(nb.get(), 0, 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        enc->setVertexBuffer(fb.get(), 0, 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) { printf("FAIL depth commit\n"); return 1; }
        Readback(target.get(), px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        // near xanh thắng (depth), không phải đỏ vẽ sau
        if (!(mid[2] > 200 && mid[0] < 50)) {
            printf("FAIL depth mid=(%u,%u,%u)\n", mid[0], mid[1], mid[2]);
            return 1;
        }
    }
    printf("test_raster_apple PASS (fill/wire/scissor/depth)\n");
    return 0;
}
