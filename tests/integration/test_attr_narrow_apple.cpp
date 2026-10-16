// test_attr_narrow_apple.cpp — PROBE (Metal thật): descriptor hẹp (UShort2/
// UChar4) + shader input rộng (uint2/uint4) — đúng combo Sodium 0.9.2
// (TEXTURE=USHORTx2-int, LIGHT=UBYTE×4-int) gặp shader uvec2/uvec4.
// GL thật zero-extend (ushort 0x1234 → uint 0x1234). Metal có làm vậy không?
// Pipeline nil hay pixels sai đều là bằng chứng để thiết kế fix.
#include "tglmt/MetalInterface.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt::metal;

static const char* kMSL = R"MSL(
#include <metal_stdlib>
using namespace metal;
struct VIn {
    uint2 a [[attribute(0)]];
    uint4 b [[attribute(1)]];
};
struct VOut { float4 p [[position]]; uint2 a; uint4 b; };
vertex VOut narrowVS(VIn in [[stage_in]]) {
    VOut o;
    o.p = float4(0.0, 0.0, 0.0, 1.0);
    o.a = in.a; o.b = in.b;
    return o;
}
fragment float4 narrowFS(VOut in [[stage_in]]) {
    float r = float(in.a.x & 0xFFu) / 255.0;
    float g = float((in.a.x >> 8) & 0xFFu) / 255.0;
    float bl = float(in.b.z) / 255.0;
    return float4(r, g, bl, 1.0);
}
)MSL";

int main() {
    auto dev = CreateDevice("apple", nullptr);
    if (!dev || dev->isNull()) {
        printf("test_attr_narrow_apple SKIP: no MTL device\n");
        return 0;
    }
    std::string err;
    auto lib = dev->compileLibrary(kMSL, err);
    if (!lib) { printf("test_attr_narrow_apple FAIL: compile %s\n", err.c_str()); return 1; }

    CustomAttrib cas[2];
    cas[0].loc = 0;
    cas[0].size = 2;
    cas[0].type = 0x1403; // UNSIGNED_SHORT
    cas[0].normalized = false;
    cas[0].offset = 0;
    cas[0].bufferIndex = 0;
    cas[0].stride = 8;
    cas[0].divisor = 0;
    cas[1].loc = 1;
    cas[1].size = 4;
    cas[1].type = 0x1401; // UNSIGNED_BYTE
    cas[1].normalized = false;
    cas[1].offset = 4;
    cas[1].bufferIndex = 0;
    cas[1].stride = 8;
    cas[1].divisor = 0;
    PipelineOpts opts;
    opts.depth = false;
    opts.blend = false;
    opts.colorWriteMask = 0xF;
    auto pipe = dev->makeCustomPipeline(lib.get(), "narrowVS", lib.get(), "narrowFS",
                                        PixelFormat::RGBA8Unorm, cas, 2, 8, &opts);
    if (!pipe) {
        printf("test_attr_narrow_apple RESULT: pipeline nil (Metal tu choi ushort2->uint2 / uchar4->uint4)\n");
        return 1;
    }
    // a=(0x1234,0xABCD) ushort, b=(1,2,3,4) ubyte → kỳ vọng GL: r=0x34, g=0x12, b=3.
    uint8_t vbuf[8] = {0x34, 0x12, 0xCD, 0xAB, 1, 2, 3, 4};
    auto vb = dev->newBufferWithBytes(vbuf, sizeof(vbuf), StorageMode::Shared);
    auto target = dev->makeRenderTarget(8, 8, PixelFormat::RGBA8Unorm);
    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pipe.get(), black);
    Viewport vp{0, 0, 8, 8, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->drawPrimitives(PrimitiveType::Point, 0, 1);
    if (!enc->endAndCommit()) { printf("test_attr_narrow_apple FAIL: commit\n"); return 1; }
    unsigned char px[8 * 8 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 8 * 4)) { printf("test_attr_narrow_apple FAIL: readback\n"); return 1; }
    // point size 1 tại (0,0,0,1) → pixel nào đó có màu; quét tìm pixel sáng nhất.
    int bi = 0;
    for (int i = 1; i < 64; ++i)
        if (px[i * 4] + px[i * 4 + 1] + px[i * 4 + 2] > px[bi * 4] + px[bi * 4 + 1] + px[bi * 4 + 2]) bi = i;
    printf("narrow brightest=(%u,%u,%u,%u) want=(52,18,3,255)\n", px[bi * 4], px[bi * 4 + 1],
           px[bi * 4 + 2], px[bi * 4 + 3]);
    if (std::abs((int)px[bi * 4] - 52) > 2 || std::abs((int)px[bi * 4 + 1] - 18) > 2 ||
        std::abs((int)px[bi * 4 + 2] - 3) > 2) {
        printf("test_attr_narrow_apple RESULT: pixels sai (khong zero-extend)\n");
        return 1;
    }
    printf("test_attr_narrow_apple PASS (Metal zero-extend nhu GL)\n");
    return 0;
}
