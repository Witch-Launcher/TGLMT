// test_rop_apple.cpp — glLogicOp composite: compute-ROP 16 ops (glspec46 Table 17.3).
// Kernel đọc src+dst (RGBA8Uint), ghi kết quả ra BUFFER (đọc qua contents()).
// Kỳ vọng tính độc lập bằng C++ từ cùng bảng spec. FAIL nếu GPU sai bất kỳ op nào.
#include "tglmt/MetalInterface.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    std::string cands[] = {"shaders/rop.metal", "../shaders/rop.metal",
                           "../../shaders/rop.metal"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/rop.metal";
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}
// Kỳ vọng CPU độc lập từ Table 17.3 (s=src byte, d=dst byte).
static uint8_t Expect(uint32_t op, uint8_t s, uint8_t d) {
    switch (op) {
        case 0x1500: return 0;
        case 0x1501: return s & d;
        case 0x1502: return s & (uint8_t)~d;
        case 0x1503: return s;
        case 0x1504: return (uint8_t)~s & d;
        case 0x1505: return d;
        case 0x1506: return s ^ d;
        case 0x1507: return s | d;
        case 0x1508: return (uint8_t)~(s | d);
        case 0x1509: return (uint8_t)~(s ^ d);
        case 0x150A: return (uint8_t)~d;
        case 0x150B: return s | (uint8_t)~d;
        case 0x150C: return (uint8_t)~s;
        case 0x150D: return (uint8_t)~s | d;
        case 0x150E: return (uint8_t)~(s & d);
        case 0x150F: return 0xFF;
        default: return d;
    }
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_rop_apple SKIP: no MTL device\n"); return 0; }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_rop_apple SKIP: rop.metal not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) { printf("test_rop_apple FAIL: compile: %s\n", err.c_str()); return 1; }
    auto cps = dev->makeComputePipeline(lib.get(), "rop");
    if (!cps) { printf("test_rop_apple FAIL: compute pipeline\n"); return 1; }
    uint8_t src[16] = {0xF0, 0x0F, 0xAA, 0x55, 0x3C, 0xC3, 0x99, 0x66,
                       0x11, 0x22, 0x44, 0x88, 0xFF, 0x00, 0x5A, 0xA5};
    uint8_t dst0[16] = {0xCC, 0x33, 0x5A, 0xA5, 0x0F, 0xF0, 0x69, 0x96,
                        0x24, 0x42, 0x81, 0x18, 0x00, 0xFF, 0x3C, 0xC3};
    auto srcT = dev->newTextureWithBytes(2, 2, PixelFormat::RGBA8Uint, src, 8);
    auto dstT = dev->newTextureWithBytes(2, 2, PixelFormat::RGBA8Uint, dst0, 8);
    if (!srcT || !dstT) { printf("test_rop_apple FAIL: uint texture unsupported\n"); return 1; }
    const uint32_t ops[16] = {0x1500, 0x1501, 0x1502, 0x1503, 0x1504, 0x1505, 0x1506, 0x1507,
                              0x1508, 0x1509, 0x150A, 0x150B, 0x150C, 0x150D, 0x150E, 0x150F};
    for (int k = 0; k < 16; ++k) {
        uint32_t op = ops[k];
        auto opB = dev->newBufferWithBytes(&op, 4, StorageMode::Shared);
        auto outB = dev->newBuffer(16, StorageMode::Shared);
        auto enc = dev->makeComputeEncoder();
        if (!enc) { printf("test_rop_apple FAIL: encoder\n"); return 1; }
        enc->setPipeline(cps.get());
        enc->setTexture(srcT.get(), 0);
        enc->setTexture(dstT.get(), 1);
        enc->setBuffer(opB.get(), 0);
        enc->setBuffer(outB.get(), 1);
        enc->dispatch2D(2, 2);
        if (!enc->endAndCommit()) { printf("test_rop_apple FAIL: commit op %04X\n", op); return 1; }
        const uint8_t* out = (const uint8_t*)outB->contents();
        for (int i = 0; i < 16; ++i) {
            uint8_t e = Expect(op, src[i], dst0[i]);
            if (out[i] != e) {
                printf("test_rop_apple FAIL: op %04X byte %d: gpu=%02X expect=%02X\n",
                       op, i, out[i], e);
                return 1;
            }
        }
    }
    printf("test_rop_apple PASS (16/16 ROPs exact)\n");
    return 0;
}
