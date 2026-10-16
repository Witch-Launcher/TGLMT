// test_mip_grad_apple.cpp — PROBE (Metal thật): texture 1 level (không
// mipmap) sample bằng textureGrad với gradient lớn (mô phỏng Sodium
// sampleNearest ở xa, minified) và textureLod lod=3. Gần (grad~0) có đúng?
// Nếu xa ra trắng/rác trong khi gần đúng → giải thích "gần đúng, xa trắng".
#include "tglmt/MetalInterface.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt::metal;

static const char* kMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct VIn { float2 p [[attribute(0)]]; float grad [[attribute(1)]]; };
struct VOut { float4 p [[position]]; float2 uv; float grad; };
vertex VOut gradVS(VIn in [[stage_in]]) {
    VOut o;
    o.p = float4(in.p, 0.0, 1.0);
    o.uv = in.p * 0.5 + 0.5;
    o.grad = in.grad;
    return o;
}
// grad_mult>1: gia lap minification (uv thay doi nhanh tren man hinh).
fragment float4 gradFS(VOut in [[stage_in]], texture2d<float> t [[texture(0)]]) {
    constexpr sampler s(address::clamp_to_edge, filter::linear);
    float2 du = float2(in.grad, 0.0);
    float2 dv = float2(0.0, in.grad);
    return t.sample(s, in.uv, gradient2d(du, dv));
}
)MSL";

int main() {
    auto dev = CreateDevice("apple", nullptr);
    if (!dev || dev->isNull()) {
        printf("test_mip_grad_apple SKIP: no MTL device\n");
        return 0;
    }
    std::string err;
    auto lib = dev->compileLibrary(kMSL, err);
    if (!lib) { printf("test_mip_grad_apple FAIL: compile %s\n", err.c_str()); return 1; }
    // Texture 8x8: o vuong do (255,0,0), nen den. 1 level.
    unsigned char img[8 * 8 * 4];
    memset(img, 0, sizeof(img));
    for (int y = 2; y < 6; ++y)
        for (int x = 2; x < 6; ++x) {
            unsigned char* p = img + (y * 8 + x) * 4;
            p[0] = 255;
            p[3] = 255;
        }
    auto tex = dev->newTextureWithBytes(8, 8, PixelFormat::RGBA8Unorm, img, 8 * 4);
    if (!tex) { printf("test_mip_grad_apple FAIL: texture\n"); return 1; }
    CustomAttrib cas[2];
    cas[0].loc = 0;
    cas[0].size = 2;
    cas[0].type = 0x1406;
    cas[0].normalized = false;
    cas[0].offset = 0;
    cas[0].bufferIndex = 0;
    cas[0].stride = 12;
    cas[0].divisor = 0;
    cas[1].loc = 1;
    cas[1].size = 1;
    cas[1].type = 0x1406;
    cas[1].normalized = false;
    cas[1].offset = 8;
    cas[1].bufferIndex = 0;
    cas[1].stride = 12;
    cas[1].divisor = 0;
    PipelineOpts opts;
    opts.depth = false;
    opts.blend = false;
    opts.colorWriteMask = 0xF;
    auto pipe = dev->makeCustomPipeline(lib.get(), "gradVS", lib.get(), "gradFS",
                                        PixelFormat::RGBA8Unorm, cas, 2, 12, &opts);
    if (!pipe) { printf("test_mip_grad_apple FAIL: pipeline nil\n"); return 1; }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    // Quad fullscreen, grad attribute = lod gia lap: 0.0 (gan) vs 4.0 (xa).
    struct V { float x, y, g; };
    auto run = [&](float g) -> unsigned {
        V q[6] = {{-1, -1, g}, {1, -1, g}, {1, 1, g}, {-1, -1, g}, {1, 1, g}, {-1, 1, g}};
        auto vb = dev->newBufferWithBytes(q, sizeof(q), StorageMode::Shared);
        ClearColor black{0, 0, 0, 1};
        auto enc = dev->makeRenderEncoder(target.get(), pipe.get(), black);
        Viewport vp{0, 0, 64, 64, 0, 1};
        enc->setViewport(vp);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->setFragmentTexture(tex.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) return 9999;
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        if (!target->readback(px, 64 * 4)) return 9999;
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        return ((unsigned)mid[0] << 16) | mid[1];
    };
    unsigned near = run(0.0f);
    unsigned far = run(4.0f);
    printf("mipgrad near=0x%06x far=0x%06x (do gan dung: R cao)\n", near, far);
    int fails = 0;
    if ((near & 0xFF0000) < 0x800000) {
        printf("test_mip_grad_apple FAIL: gan cung sai\n");
        ++fails;
    }
    // Far tren texture 1-level: ghi nhan ket qua (trang/den/do) de quyet dinh fix.
    printf("test_mip_grad_apple RESULT far R=%u\n", (far >> 16) & 0xFF);
    if (fails == 0) printf("test_mip_grad_apple PASS (gan dung)\n");
    return fails ? 1 : 0;
}
