// test_texture_apple.cpp — Texturing E2E GPU thật: upload 2x2 + sampler NEAREST.
// Quad fullscreen uv 0..1 → 4 góc đọc đúng texel (đỏ/xanh lá/xanh dương/trắng).
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    std::string cands[] = {"shaders/raster.metal", "../shaders/raster.metal",
                           "../../shaders/raster.metal"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/raster.metal";
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_texture_apple SKIP: no MTL device\n"); return 0; }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_texture_apple SKIP: shader not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) { printf("test_texture_apple FAIL: compile: %s\n", err.c_str()); return 1; }
    auto pso = dev->makeRenderPipeline(lib.get(), "texVS", lib.get(), "texFS",
                                       PixelFormat::RGBA8Unorm);
    if (!pso) { printf("test_texture_apple FAIL: pipeline\n"); return 1; }
    // Byte upload theo chuẩn GL (spec §8.4: byte đầu = hàng DƯỚI/trái):
    // hàng 0 (dưới) = đỏ,xanh lá; hàng 1 (trên) = xanh dương,trắng.
    // Metal uv v=0 đọc đúng byte đầu → khớp hành vi GL, không cần flip.
    unsigned char tex[2 * 2 * 4] = {
        255, 0, 0, 255, 0, 255, 0, 255,
        0, 0, 255, 255, 255, 255, 255, 255,
    };
    auto tx = dev->newTextureWithBytes(2, 2, PixelFormat::RGBA8Unorm, tex, 2 * 4);
    if (!tx) { printf("test_texture_apple FAIL: texture\n"); return 1; }
    // glSamplerParameter: MIN/MAG NEAREST, WRAP CLAMP_TO_EDGE (không aniso)
    SamplerDesc sd;
    sd.minFilter = 0x2600; sd.magFilter = 0x2600;
    sd.sWrap = 0x812F; sd.tWrap = 0x812F; sd.maxAniso = 1.0f;
    auto smp = dev->makeSampler(sd);
    if (!smp) { printf("test_texture_apple FAIL: sampler\n"); return 1; }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    // quad fullscreen: 2 tam giác, uv pad float4 (layout M5 attr1 float4, dùng .xy)
    struct V { float x, y, u, v, z, w; };
    V quad[6] = {
        {-1,-1, 0,1, 0,1}, {1,-1, 1,1, 0,1}, {1,1, 1,0, 0,1},
        {-1,-1, 0,1, 0,1}, {1,1, 1,0, 0,1}, {-1,1, 0,0, 0,1},
    };
    auto vb = dev->newBufferWithBytes(quad, sizeof(quad), StorageMode::Shared);
    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->setFragmentTexture(tx.get(), 0);
    enc->setFragmentSamplerState(smp.get(), 0);
    enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
    if (!enc->endAndCommit()) { printf("test_texture_apple FAIL: commit\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_texture_apple FAIL: readback\n"); return 1; }
    // Shader lật y cho vị trí nhưng giữ uv: màn-trên hiện uv v≈0 (byte đầu = hàng
    // dưới GL = đỏ/xanh lá), màn-dưới hiện uv v≈1 (hàng trên GL = xanh dương/trắng).
    // Đây chính là hành vi GL đúng (hàng dưới ảnh ở đáy màn hình).
    auto at = [&](int x, int y) -> unsigned char* { return px + (y * 64 + x) * 4; };
    unsigned char* tl = at(16, 16);   // uv≈(0.25,0.75) → xanh dương
    unsigned char* tr = at(48, 16);   // uv≈(0.75,0.75) → trắng
    unsigned char* bl = at(16, 48);   // uv≈(0.25,0.25) → đỏ
    unsigned char* br = at(48, 48);   // uv≈(0.75,0.25) → xanh lá
    printf("tl=(%u,%u,%u) tr=(%u,%u,%u) bl=(%u,%u,%u) br=(%u,%u,%u)\n",
           tl[0], tl[1], tl[2], tr[0], tr[1], tr[2], bl[0], bl[1], bl[2], br[0], br[1], br[2]);
    bool ok = bl[0] > 200 && bl[1] < 50 && bl[2] < 50 &&
              br[1] > 200 && br[0] < 50 && br[2] < 50 &&
              tl[2] > 200 && tl[0] < 50 && tl[1] < 50 &&
              tr[0] > 200 && tr[1] > 200 && tr[2] > 200;
    if (!ok) { printf("test_texture_apple FAIL: texels wrong\n"); return 1; }
    printf("test_texture_apple PASS (upload+sampler+sample exact)\n");
    return 0;
}
