// test_shadow_compare_apple.cpp — E2E GPU: depth2d + sampler compare
// (GL_TEXTURE_COMPARE_MODE=COMPARE_REF_TO_TEXTURE, GL_LEQUAL).
// Pass 1: vẽ quad depth=0.5 vào Depth32F. Pass 2: sample_compare ref 0.4/0.6.
// Kỳ vọng: ref 0.4 ≤ 0.5 → 1.0; ref 0.6 > 0.5 → 0.0. Nếu bridge bỏ
// compareFunction (sampler thường), sample_compare trả rác/validation error.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstring>

static const char* kMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct VOut { float4 pos [[position]]; float2 uv; };
static float2 corner(uint vid) {
    float2 p[6] = { float2(-1.0,-1.0), float2(1.0,-1.0), float2(1.0,1.0),
                    float2(-1.0,-1.0), float2(1.0,1.0), float2(-1.0,1.0) };
    return p[vid];
}
vertex VOut depthVS(uint vid [[vertex_id]]) {
    float2 p = corner(vid);
    VOut o; o.pos = float4(p, 0.5, 1.0); o.uv = p * 0.5 + 0.5; return o;
}
fragment float4 depthFS(VOut in [[stage_in]]) { return float4(0.0, 0.0, 0.0, 1.0); }
vertex VOut probeVS(uint vid [[vertex_id]]) {
    float2 p = corner(vid);
    VOut o; o.pos = float4(p, 0.0, 1.0); o.uv = p * 0.5 + 0.5; return o;
}
fragment float4 probeFS(VOut in [[stage_in]],
                        depth2d<float> dt [[texture(0)]],
                        sampler smp [[sampler(0)]],
                        sampler rawSmp [[sampler(1)]]) {
    float lo = dt.sample_compare(smp, in.uv, 0.4);
    float hi = dt.sample_compare(smp, in.uv, 0.6);
    float raw = dt.sample(rawSmp, in.uv);
    return float4(lo, hi, raw, 1.0);
}
)MSL";

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_shadow_compare_apple SKIP: no MTL device\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(kMSL, err);
    if (!lib) { printf("test_shadow_compare_apple FAIL: compile: %s\n", err.c_str()); return 1; }

    const uint32_t W = 32, H = 32;
    Viewport vp{0, 0, (double)W, (double)H, 0, 1};
    ClearColor black{0, 0, 0, 1};

    // Pass 1: depth 0.5. wrapAsTarget tự clear depth 1.0 lần dùng đầu.
    auto color1 = dev->newTexture(W, H, PixelFormat::RGBA8Unorm);
    auto depth = dev->newDepthTexture(W, H, PixelFormat::Depth32Float);
    if (!color1 || !depth) { printf("test_shadow_compare_apple FAIL: textures\n"); return 1; }
    auto rt1 = dev->wrapAsTarget(color1.get(), depth.get());
    auto pso1 = dev->makeDepthPipeline(lib.get(), "depthVS", lib.get(), "depthFS",
                                       PixelFormat::RGBA8Unorm, false);
    auto dss = dev->makeDepthStencilState(0x0203 /* GL_LEQUAL */, true);
    if (!rt1 || !pso1 || !dss) { printf("test_shadow_compare_apple FAIL: pass1 setup\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(rt1.get(), pso1.get(), black);
        if (!enc) { printf("test_shadow_compare_apple FAIL: enc1\n"); return 1; }
        enc->setViewport(vp);
        enc->setDepthStencilState(dss.get());
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_shadow_compare_apple FAIL: commit1\n"); return 1; }
    }

    // Pass 2: compare sampler (LEQUAL) đọc depth texture.
    auto color2 = dev->newTexture(W, H, PixelFormat::RGBA8Unorm);
    auto rt2 = dev->wrapAsTarget(color2.get(), nullptr);
    auto pso2 = dev->makeRenderPipeline(lib.get(), "probeVS", lib.get(), "probeFS",
                                        PixelFormat::RGBA8Unorm, false);
    SamplerDesc sd;
    sd.minFilter = 0x2600; sd.magFilter = 0x2600; // GL_NEAREST
    sd.sWrap = 0x812F; sd.tWrap = 0x812F;         // CLAMP_TO_EDGE
    sd.maxAniso = 1.0f; sd.noMip = true;
    sd.compareFunc = 0x0203;                       // GL_LEQUAL
    auto smp = dev->makeSampler(sd);
    SamplerDesc rawDesc = sd;
    rawDesc.compareFunc = 0;
    auto rawSmp = dev->makeSampler(rawDesc);
    if (!rt2 || !pso2 || !smp || !rawSmp) { printf("test_shadow_compare_apple FAIL: pass2 setup\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(rt2.get(), pso2.get(), black);
        if (!enc) { printf("test_shadow_compare_apple FAIL: enc2\n"); return 1; }
        enc->setViewport(vp);
        enc->setFragmentTexture(depth.get(), 0);
        enc->setFragmentSamplerState(smp.get(), 0);
        enc->setFragmentSamplerState(rawSmp.get(), 1);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_shadow_compare_apple FAIL: commit2\n"); return 1; }
    }

    unsigned char px[W * H * 4];
    memset(px, 0, sizeof(px));
    if (!rt2->readback(px, W * 4)) { printf("test_shadow_compare_apple FAIL: readback\n"); return 1; }
    // Mọi pixel đều nằm trong quad → giá trị đồng nhất.
    unsigned char* c = px + ((H / 2) * W + W / 2) * 4;
    printf("center lo=%u hi=%u raw=%.3f\n", c[0], c[1], c[2] / 255.0f);
    if (!(c[0] > 200 && c[1] < 50 && c[2] > 110 && c[2] < 145)) {
        printf("test_shadow_compare_apple FAIL: compare sai (lo phải 1, hi phải 0)\n");
        return 1;
    }
    printf("test_shadow_compare_apple PASS (hardware compare đúng)\n");
    return 0;
}
