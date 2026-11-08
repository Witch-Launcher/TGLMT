// test_readpixels_region.cpp — glReadPixels phải đọc đúng vùng x/y từ FBO (cả
// shadow path), hỗ trợ RGB/RG/RED/UBYTE/USHORT/FLOAT và PACK_ROW_LENGTH/SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_readpixels_region FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    const int W = 4, H = 4;
    std::vector<uint8_t> src((size_t)W * H * 4);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            uint8_t* p = src.data() + ((size_t)y * W + x) * 4;
            p[0] = (uint8_t)(x * 40 + 10);
            p[1] = (uint8_t)(y * 50 + 20);
            p[2] = (uint8_t)(x * 7 + y * 11);
            p[3] = 255;
        }
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, src.data());
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    glReadBuffer(0x8CE0);
    glPixelStorei(0x0D05 /*PACK_ALIGNMENT*/, 1);
    Check(glGetError() == 0, "setup FBO");

    // Vùng 2x2 tại (1,1): không được trả pixel (0,0).
    uint8_t out[64];
    memset(out, 0xEE, sizeof(out));
    glReadPixels(1, 1, 2, 2, 0x1908, 0x1401, out);
    Check(glGetError() == 0, "ReadPixels vùng 2x2 no error");
    bool regionOk = true;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 4; ++c)
                regionOk = regionOk &&
                    out[((size_t)y * 2 + x) * 4 + c] ==
                        src[((size_t)(y + 1) * W + (x + 1)) * 4 + c];
    Check(regionOk, "ReadPixels trả đúng vùng x/y");

    // RGB (3B) và RED (1B) chuyển đúng component.
    uint8_t rgb[3] = {0, 0, 0};
    glReadPixels(2, 3, 1, 1, 0x1907, 0x1401, rgb);
    Check(glGetError() == 0, "ReadPixels RGB no error");
    Check(rgb[0] == src[((size_t)3 * W + 2) * 4] &&
          rgb[1] == src[((size_t)3 * W + 2) * 4 + 1] &&
          rgb[2] == src[((size_t)3 * W + 2) * 4 + 2], "ReadPixels RGB đúng");
    uint8_t red = 0;
    glReadPixels(3, 0, 1, 1, 0x1903, 0x1401, &red);
    Check(glGetError() == 0 && red == src[((size_t)0 * W + 3) * 4],
          "ReadPixels RED đúng");

    // FLOAT RGBA: chuẩn hóa 0..1.
    float rgbaF[4] = {0, 0, 0, 0};
    glReadPixels(0, 2, 1, 1, 0x1908, 0x1406, rgbaF);
    Check(glGetError() == 0, "ReadPixels FLOAT no error");
    Check(rgbaF[0] > 0.03f && rgbaF[0] < 0.05f && rgbaF[1] > 0.45f && rgbaF[1] < 0.49f,
          "ReadPixels FLOAT đúng");

    // PACK_ROW_LENGTH=6, SKIP_ROWS=1, SKIP_PIXELS=1: pixel (0,0) nằm ở offset 28.
    glPixelStorei(0x0D02 /*PACK_ROW_LENGTH*/, 6);
    glPixelStorei(0x0D03 /*PACK_SKIP_ROWS*/, 1);
    glPixelStorei(0x0D04 /*PACK_SKIP_PIXELS*/, 1);
    std::vector<uint8_t> packed(80, 0xEE);
    glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, packed.data());
    Check(glGetError() == 0, "ReadPixels PACK no error");
    const size_t row = 6 * 4;
    const size_t packOff = row + 4; // SKIP_ROWS=1, SKIP_PIXELS=1
    bool packOk = true;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            for (int c = 0; c < 4; ++c)
                packOk = packOk &&
                    packed[packOff + (size_t)y * row + (size_t)x * 4 + c] ==
                        src[((size_t)y * W + x) * 4 + c];
    Check(packOk, "PACK_ROW_LENGTH/SKIP đúng offset");
    Check(packed[row - 1] == 0xEE && packed[row * 2] == 0xEE,
          "ReadPixels không ghi đè vùng đệm ngoài PACK");

    // ReadnPixels phải tính cả PACK khi kiểm bufSize.
    glReadnPixels(0, 0, 2, 2, 0x1908, 0x1401, 32, packed.data());
    Check(glGetError() == 0x0502, "ReadnPixels thiếu buffer theo PACK → INVALID_OPERATION");
    glReadnPixels(0, 0, 2, 2, 0x1908, 0x1401, 60, packed.data());
    Check(glGetError() == 0, "ReadnPixels đủ buffer theo PACK no error");

    glPixelStorei(0x0D02, 0);
    glPixelStorei(0x0D03, 0);
    glPixelStorei(0x0D04, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    if (gFails) return 1;
    printf("test_readpixels_region PASS\n");
    return 0;
}
