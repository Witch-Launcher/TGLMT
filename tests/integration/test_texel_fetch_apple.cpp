// test_texel_fetch_apple.cpp — E2E GPU: texelFetch (MSL read()) trên
// texture2d<float, access::sample> + depth2d<float>.
// CR dùng texelFetch cho colortex/depthtex (54 chỗ) — nếu read() trả 0/rác
// thì deferred/composite ra đen dù sampler .sample() vẫn đúng.
// Pass 1 ghi hằng số biết trước (RGBA8 + RGBA16F + depth 0.5); pass 2 đọc
// bằng read() VÀ sample() rồi so — sai lệch → đỏ, đúng → xanh.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static const char* kMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct VOut { float4 pos [[position]]; float2 uv; };
static float2 corner(uint vid) {
    float2 p[6] = { float2(-1.0,-1.0), float2(1.0,-1.0), float2(1.0,1.0),
                    float2(-1.0,-1.0), float2(1.0,1.0), float2(-1.0,1.0) };
    return p[vid];
}
vertex VOut quadVS(uint vid [[vertex_id]]) {
    float2 p = corner(vid);
    VOut o; o.pos = float4(p, 0.0, 1.0); o.uv = p * 0.5 + 0.5; return o;
}
vertex VOut depthVS(uint vid [[vertex_id]]) {
    float2 p = corner(vid);
    VOut o; o.pos = float4(p, 0.5, 1.0); o.uv = p * 0.5 + 0.5; return o;
}
fragment float4 fillFS(VOut in [[stage_in]]) {
    return float4(0.25, 0.5, 0.75, 1.0);
}
fragment float4 fillHDRFS(VOut in [[stage_in]]) {
    return float4(2.0, 1.0, 0.5, 1.0);
}
fragment float4 fillSnormFS(VOut in [[stage_in]]) {
    return float4(0.5, -0.5, 0.0, 1.0);
}
fragment float4 probeSnormFS(VOut in [[stage_in]], texture2d<float> tx [[texture(0)]]) {
    uint2 sz = uint2(tx.get_width(), tx.get_height());
    uint2 xy = min(uint2(float2(sz) * in.uv), sz - 1u);
    float4 v = tx.read(xy, 0);
    return float4(v.r, v.g * 0.5 + 0.5, v.b * 0.5 + 0.5, 1.0);
}
fragment float4 probeFS(VOut in [[stage_in]],
                        texture2d<float> tx [[texture(0)]],
                        sampler smp [[sampler(0)]]) {
    uint2 sz = uint2(tx.get_width(), tx.get_height());
    uint2 xy = min(uint2(float2(sz) * in.uv), sz - 1u);
    float4 rd = tx.read(xy, 0);
    float4 sm = tx.sample(smp, in.uv);
    // Xanh = read==sample (đúng); đỏ = lệch (read hỏng).
    float4 diff = abs(rd - sm);
    float bad = max(max(diff.r, diff.g), max(diff.b, diff.a));
    float4 ok = float4(0.0, 1.0, 0.0, 1.0);
    float4 ng = float4(1.0, 0.0, 0.0, 1.0);
    return bad > 0.02 ? ng : ok;
}
fragment float4 probeRGBA(VOut in [[stage_in]], texture2d<float> tx [[texture(0)]]) {
    uint2 sz = uint2(tx.get_width(), tx.get_height());
    uint2 xy = min(uint2(float2(sz) * in.uv), sz - 1u);
    return float4(tx.read(xy, 0).rgb, 1.0);
}
fragment float4 probeDepth(VOut in [[stage_in]], depth2d<float> dt [[texture(0)]],
                          sampler smp [[sampler(0)]]) {
    uint2 sz = uint2(dt.get_width(), dt.get_height());
    uint2 xy = min(uint2(float2(sz) * in.uv), sz - 1u);
    float d = dt.read(xy, 0);
    float s = dt.sample(smp, in.uv);
    // R=read, G=sample, B=read==sample (đúng=1). Debug trực tiếp giá trị.
    return float4(d, s, abs(d - s) < 0.01 ? 1.0 : 0.0, 1.0);
}
)MSL";

using namespace tglmt::metal;

static bool checkPx(const unsigned char* p, int r, int g, int b, int tol) {
    return std::abs((int)p[0] - r) <= tol && std::abs((int)p[1] - g) <= tol &&
           std::abs((int)p[2] - b) <= tol;
}

int main() {
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_texel_fetch_apple SKIP: no MTL device\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(kMSL, err);
    if (!lib) { printf("test_texel_fetch_apple FAIL: compile: %s\n", err.c_str()); return 1; }

    const uint32_t W = 16, H = 16, OW = 16, OH = 16;
    Viewport vp{0, 0, (double)W, (double)H, 0, 1};
    Viewport ovp{0, 0, (double)OW, (double)OH, 0, 1};
    ClearColor black{0, 0, 0, 1};

    SamplerDesc sd;
    sd.minFilter = 0x2600; sd.magFilter = 0x2600;
    sd.sWrap = 0x812F; sd.tWrap = 0x812F;
    sd.maxAniso = 1.0f; sd.noMip = true;
    auto smp = dev->makeSampler(sd);
    if (!smp) { printf("test_texel_fetch_apple FAIL: sampler\n"); return 1; }

    // ---------- Test 1: read() RGBA8 phải bằng sample() ----------
    auto fillPSO = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "fillFS",
                                           PixelFormat::RGBA8Unorm, false);
    auto src = dev->newTexture(W, H, PixelFormat::RGBA8Unorm);
    auto srcRT = dev->wrapAsTarget(src.get(), nullptr);
    if (!fillPSO || !src || !srcRT) { printf("test_texel_fetch_apple FAIL: setup1\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(srcRT.get(), fillPSO.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc1\n"); return 1; }
        enc->setViewport(vp);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit1\n"); return 1; }
    }
    auto probePSO = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "probeFS",
                                            PixelFormat::RGBA8Unorm, false);
    auto out1 = dev->newTexture(OW, OH, PixelFormat::RGBA8Unorm);
    auto out1RT = dev->wrapAsTarget(out1.get(), nullptr);
    if (!probePSO || !out1 || !out1RT) { printf("test_texel_fetch_apple FAIL: setup2\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(out1RT.get(), probePSO.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc2\n"); return 1; }
        enc->setViewport(ovp);
        enc->setFragmentTexture(src.get(), 0);
        enc->setFragmentSamplerState(smp.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit2\n"); return 1; }
    }
    unsigned char px[OW * OH * 4];
    if (!out1RT->readback(px, OW * 4)) { printf("test_texel_fetch_apple FAIL: readback1\n"); return 1; }
    const unsigned char* c = px + ((OH / 2) * OW + OW / 2) * 4;
    printf("test1 read-vs-sample: (%u,%u,%u)\n", c[0], c[1], c[2]);
    if (!checkPx(c, 0, 255, 0, 2)) {
        printf("test_texel_fetch_apple FAIL: read() != sample() trên RGBA8 (read hỏng)\n");
        return 1;
    }

    // ---------- Test 2: read() đúng GIÁ TRỊ trên RGBA8 ----------
    auto probeRGBA = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "probeRGBA",
                                             PixelFormat::RGBA8Unorm, false);
    auto out2 = dev->newTexture(OW, OH, PixelFormat::RGBA8Unorm);
    auto out2RT = dev->wrapAsTarget(out2.get(), nullptr);
    if (!probeRGBA || !out2RT) { printf("test_texel_fetch_apple FAIL: setup3\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(out2RT.get(), probeRGBA.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc3\n"); return 1; }
        enc->setViewport(ovp);
        enc->setFragmentTexture(src.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit3\n"); return 1; }
    }
    if (!out2RT->readback(px, OW * 4)) { printf("test_texel_fetch_apple FAIL: readback2\n"); return 1; }
    c = px + ((OH / 2) * OW + OW / 2) * 4;
    printf("test2 rgba value: (%u,%u,%u)\n", c[0], c[1], c[2]);
    if (!checkPx(c, 64, 128, 191, 2)) {
        printf("test_texel_fetch_apple FAIL: read() RGBA8 sai giá trị\n");
        return 1;
    }

    // ---------- Test 3: read() HDR RGBA16Float (2.0,1.0,0.5) ----------
    auto fillHDR = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "fillHDRFS",
                                            PixelFormat::RGBA16Float, false);
    auto hdr = dev->newTexture(W, H, PixelFormat::RGBA16Float);
    auto hdrRT = dev->wrapAsTarget(hdr.get(), nullptr);
    if (!fillHDR || !hdr || !hdrRT) { printf("test_texel_fetch_apple FAIL: setup4\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(hdrRT.get(), fillHDR.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc4\n"); return 1; }
        enc->setViewport(vp);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit4\n"); return 1; }
    }
    auto out3 = dev->newTexture(OW, OH, PixelFormat::RGBA8Unorm);
    auto out3RT = dev->wrapAsTarget(out3.get(), nullptr);
    auto probeHDR = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "probeFS",
                                             PixelFormat::RGBA8Unorm, false);
    if (!out3RT || !probeHDR) { printf("test_texel_fetch_apple FAIL: setup5\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(out3RT.get(), probeHDR.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc5\n"); return 1; }
        enc->setViewport(ovp);
        enc->setFragmentTexture(hdr.get(), 0);
        enc->setFragmentSamplerState(smp.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit5\n"); return 1; }
    }
    if (!out3RT->readback(px, OW * 4)) { printf("test_texel_fetch_apple FAIL: readback3\n"); return 1; }
    c = px + ((OH / 2) * OW + OW / 2) * 4;
    printf("test3 hdr read-vs-sample: (%u,%u,%u)\n", c[0], c[1], c[2]);
    if (!checkPx(c, 0, 255, 0, 2)) {
        printf("test_texel_fetch_apple FAIL: read() != sample() trên RGBA16F\n");
        return 1;
    }

    // ---------- Test 4: read() depth = 0.5 ----------
    auto depth = dev->newDepthTexture(W, H, PixelFormat::Depth32Float);
    auto dColor = dev->newTexture(W, H, PixelFormat::RGBA8Unorm);
    auto dRT = dev->wrapAsTarget(dColor.get(), depth.get());
    auto depthPSO = dev->makeDepthPipeline(lib.get(), "depthVS", lib.get(), "fillFS",
                                            PixelFormat::RGBA8Unorm, false);
    auto dss = dev->makeDepthStencilState(0x0203 /* LEQUAL */, true);
    if (!depth || !dRT || !depthPSO || !dss) { printf("test_texel_fetch_apple FAIL: setup6\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(dRT.get(), depthPSO.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc6\n"); return 1; }
        enc->setViewport(vp);
        enc->setDepthStencilState(dss.get());
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit6\n"); return 1; }
    }
    auto probeDepth = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "probeDepth",
                                              PixelFormat::RGBA8Unorm, false);
    auto out4 = dev->newTexture(OW, OH, PixelFormat::RGBA8Unorm);
    auto out4RT = dev->wrapAsTarget(out4.get(), nullptr);
    if (!probeDepth || !out4RT) { printf("test_texel_fetch_apple FAIL: setup7\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(out4RT.get(), probeDepth.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc7\n"); return 1; }
        enc->setViewport(ovp);
        enc->setFragmentTexture(depth.get(), 0);
        enc->setFragmentSamplerState(smp.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit7\n"); return 1; }
    }
    if (!out4RT->readback(px, OW * 4)) { printf("test_texel_fetch_apple FAIL: readback4\n"); return 1; }
    c = px + ((OH / 2) * OW + OW / 2) * 4;
    printf("test4 depth read: (%u,%u,%u) [R=read G=sample B=match]\n", c[0], c[1], c[2]);
    if (c[0] < 120) {
        printf("test_texel_fetch_apple FAIL: read() depth sai (depth 0.5 không đọc được)\n");
        return 1;
    }

    // ---------- Test 5: RGBA8Snorm render + read (colortex4 normals CR) ----------
    auto fillSnorm = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "fillSnormFS",
                                              PixelFormat::RGBA8Snorm, false);
    auto snm = dev->newTexture(W, H, PixelFormat::RGBA8Snorm);
    auto snmRT = dev->wrapAsTarget(snm.get(), nullptr);
    if (!fillSnorm || !snm || !snmRT) { printf("test_texel_fetch_apple FAIL: setup8\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(snmRT.get(), fillSnorm.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc8\n"); return 1; }
        enc->setViewport(vp);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit8\n"); return 1; }
    }
    auto probeSnorm = dev->makeRenderPipeline(lib.get(), "quadVS", lib.get(), "probeSnormFS",
                                               PixelFormat::RGBA8Unorm, false);
    auto out5 = dev->newTexture(OW, OH, PixelFormat::RGBA8Unorm);
    auto out5RT = dev->wrapAsTarget(out5.get(), nullptr);
    if (!probeSnorm || !out5RT) { printf("test_texel_fetch_apple FAIL: setup9\n"); return 1; }
    {
        auto enc = dev->makeRenderEncoder(out5RT.get(), probeSnorm.get(), black);
        if (!enc) { printf("test_texel_fetch_apple FAIL: enc9\n"); return 1; }
        enc->setViewport(ovp);
        enc->setFragmentTexture(snm.get(), 0);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 6);
        if (!enc->endAndCommit()) { printf("test_texel_fetch_apple FAIL: commit9\n"); return 1; }
    }
    if (!out5RT->readback(px, OW * 4)) { printf("test_texel_fetch_apple FAIL: readback5\n"); return 1; }
    c = px + ((OH / 2) * OW + OW / 2) * 4;
    printf("test5 snorm: (%u,%u,%u) [expect 128,64,128; unorm-sai=128,128,128]\n",
           c[0], c[1], c[2]);
    if (!checkPx(c, 128, 64, 128, 3)) {
        printf("test_texel_fetch_apple FAIL: RGBA8Snorm render/read sai (normals CR sẽ hỏng)\n");
        return 1;
    }

    printf("test_texel_fetch_apple PASS (read 2D/HDR/depth đều đúng)\n");
    return 0;
}
