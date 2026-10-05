// test_present_flip_apple.cpp — chứng minh hướng lật dọc của đường present.
//
// Bối cảnh: target mặc định của TGLMT lưu pixel theo THỨ TỰ BỘ NHỚ GL
// (hàng 0 = đáy GL, xem GLConvert.h). CAMetalDrawable dùng thứ tự đảo ngược
// (hàng 0 = trên màn hình) nên present phải lật dọc. Bản cũ loop ~1242
// copyFromTexture mỗi frame (rất chậm trên A11); bản mới là 1 render pass với
// shader flip. Test này chạy shader flip thật trên GPU và kiểm tra 4 góc để bắt
// lỗi lật ngược (ngược lỗi này = màn hình上下 đảo, không phát hiện bằng log).
#include "tglmt/MetalInterface.h"
#include "tglmt/gl46.h"
#include <cstdio>
#include <vector>

using namespace tglmt;

int main() {
    auto dev = metal::CreateDevice("apple");
    if (!dev || dev->isNull()) {
        printf("SKIP present_flip: khong co Metal device\n");
        return 0;
    }
    const uint32_t W = 64, H = 8;
    // GL-order: hàng 0 (đáy) = xanh dương, hàng H-1 (trên) = đỏ.
    std::vector<uint8_t> px(W * H * 4);
    for (uint32_t y = 0; y < H; ++y)
        for (uint32_t x = 0; x < W; ++x) {
            uint8_t r = 0, g = 0, b = 0;
            if (y == 0) { b = 255; }              // đáy GL
            else if (y == H - 1) { r = 255; }    // trên GL
            else { g = 255; }                     // giữa: xanh lá (để bắt lẫn dọc)
            uint8_t* p = &px[(y * W + x) * 4];
            p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
        }
    auto src = dev->newTextureWithBytes(W, H, metal::PixelFormat::RGBA8Unorm, px.data(), W * 4);
    auto dst = dev->newTexture(W, H, metal::PixelFormat::RGBA8Unorm);
    if (!src || !dst) { printf("FAIL present_flip: tạo texture\n"); return 1; }
    if (!dev->flipCopy(src.get(), dst.get())) {
        printf("FAIL present_flip: flipCopy=false (shader flip bị Metal từ chối?)\n");
        return 1;
    }
    std::vector<uint8_t> out(W * H * 4, 0);
    auto t = dev->wrapAsTarget(dst.get(), nullptr);
    if (!t || !t->readback(out.data(), W * 4)) {
        printf("FAIL present_flip: readback\n");
        return 1;
    }
    auto at = [&](uint32_t x, uint32_t y, const char* who) {
        const uint8_t* p = &out[(y * W + x) * 4];
        printf("  %-18s (%2u,%2u) = %3u,%3u,%3u\n", who, x, y, p[0], p[1], p[2]);
    };
    printf("present_flip (src GL-order: dong 0 = xanh duong, dong %u = do)\n", H - 1);
    at(0, 0, "dst dong 0");
    at(0, H - 1, "dst dong H-1");
    int fail = 0;
    // Đích là drawable-order (hàng 0 = TRÊN màn hình). Nguồn là GL-order
    // (hàng 0 = đáy). Vì vậy flip đúng ⇒ hàng 0 của đích = hàng H-1 của nguồn
    // (đỏ), hàng H-1 của đích = hàng 0 của nguồn (xanh dương).
    const uint8_t* dstTop = &out[(0 * W + W / 2) * 4];
    const uint8_t* dstBot = &out[((H - 1) * W + W / 2) * 4];
    const uint8_t* mid = &out[((H / 2) * W + W / 2) * 4];
    if (!(dstTop[0] > 200 && dstTop[1] < 60)) { printf("FAIL: dong 0 cua dich phai DO (dang %u,%u)\n", dstTop[0], dstTop[1]); fail = 1; }
    if (!(dstBot[2] > 200 && dstBot[0] < 60)) { printf("FAIL: dong H-1 cua dich phai XANH DUONG (dang %u,%u)\n", dstBot[0], dstBot[2]); fail = 1; }
    // Chống hồi quy: nếu dst y đổi thì dst[y] phải bằng src[H-1-y].
    for (uint32_t y = 0; y < H; ++y) {
        const uint8_t* d = &out[(y * W + W / 2) * 4];
        const uint8_t* s = &px[((H - 1 - y) * W + W / 2) * 4];
        if (d[0] != s[0] || d[1] != s[1] || d[2] != s[2]) {
            printf("FAIL: dst y=%u khong doi chieu voi src y=%u (%u,%u,%u vs %u,%u,%u)\n",
                   y, H - 1 - y, d[0], d[1], d[2], s[0], s[1], s[2]);
            fail = 1;
            break;
        }
    }
    if (!(mid[1] > 200)) { printf("FAIL: dong giua phai XANH LA\n"); fail = 1; }
    printf(fail ? "FAIL present_flip\n" : "PASS present_flip\n");
    return fail;
}