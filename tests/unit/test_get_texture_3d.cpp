// test_get_texture_3d.cpp — glGetTexImage/glGetTextureSubImage phải trả đủ
// volume cho TEXTURE_3D và từng layer cho TEXTURE_2D_ARRAY (GL 4.6 §8.11).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_get_texture_3d FAIL: %s\n", msg); ++gFails; }
}

static unsigned char P(int x, int y, int z, int c) {
    return (unsigned char)(5 + x * 13 + y * 29 + z * 47 + c * 61);
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 3, H = 2, D = 2;
    std::vector<uint8_t> src((size_t)W * H * D * 4);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                for (int c = 0; c < 4; ++c)
                    src[((((size_t)z * H + y) * W + x) * 4) + c] = P(x, y, z, c);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x806F /*TEXTURE_3D*/, tex);
    glTexImage3D(0x806F, 0, 0x8058 /*RGBA8*/, W, H, D, 0, 0x1908, 0x1401,
                 src.data());
    Check(glGetError() == 0, "setup 3D");

    // glGetTexImage trả toàn volume, z-major.
    std::vector<uint8_t> out((size_t)W * H * D * 4, 0);
    glGetTexImage(0x806F, 0, 0x1908, 0x1401, out.data());
    Check(glGetError() == 0, "glGetTexImage 3D no error");
    Check(out == src, "glGetTexImage 3D trả đủ volume");

    // DSA glGetTextureImage cũng đủ volume.
    std::fill(out.begin(), out.end(), 0);
    glGetTextureImage(tex, 0, 0x1908, 0x1401, (GLsizei)out.size(), out.data());
    Check(glGetError() == 0, "glGetTextureImage 3D no error");
    Check(out == src, "glGetTextureImage 3D trả đủ volume");

    // SubImage chỉ lấy z=1, d=1, x=1..2, y=1.
    std::vector<uint8_t> sub((size_t)2 * 1 * 1 * 4, 0);
    glGetTextureSubImage(tex, 0, 1, 1, 1, 2, 1, 1, 0x1908, 0x1401,
                         (GLsizei)sub.size(), sub.data());
    Check(glGetError() == 0, "glGetTextureSubImage z=1 no error");
    bool subOk = true;
    for (int x = 0; x < 2; ++x)
        for (int c = 0; c < 4; ++c)
            subOk = subOk && sub[(size_t)x * 4 + c] == P(1 + x, 1, 1, c);
    Check(subOk, "glGetTextureSubImage trả đúng slice z=1");

    // z+d vượt depth → INVALID_VALUE.
    glGetTextureSubImage(tex, 0, 0, 0, 1, 1, 1, 2, 0x1908, 0x1401, 4, sub.data());
    Check(glGetError() == 0x0501, "GetTextureSubImage z+d vượt depth → INVALID_VALUE");

    // glGetnTexImage phải có buffer đủ cho volume.
    std::fill(out.begin(), out.end(), 0);
    glGetnTexImage(0x806F, 0, 0x1908, 0x1401, (GLsizei)out.size(), out.data());
    Check(glGetError() == 0, "glGetnTexImage 3D no error");
    Check(out == src, "glGetnTexImage 3D trả đủ volume");
    glGetnTexImage(0x806F, 0, 0x1908, 0x1401, 4, out.data());
    Check(glGetError() == 0x0502, "glGetnTexImage buffer nhỏ → INVALID_OPERATION");

    // TEXTURE_2D_ARRAY: glGetTexImage trả đủ layer.
    GLuint arr = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A /*TEXTURE_2D_ARRAY*/, arr);
    glTexImage3D(0x8C1A, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, src.data());
    Check(glGetError() == 0, "setup 2D array");
    std::fill(out.begin(), out.end(), 0);
    glGetTexImage(0x8C1A, 0, 0x1908, 0x1401, out.data());
    Check(glGetError() == 0, "glGetTexImage 2D array no error");
    Check(out == src, "glGetTexImage 2D array trả đủ layer");

    // PACK_IMAGE_HEIGHT tạo khoảng trống giữa các layer: layer 1 nằm ở
    // offset 3*rowLen, không phải 2*rowLen.
    glPixelStorei(0x0D05 /*PACK_ALIGNMENT*/, 1);
    glPixelStorei(0x806C /*PACK_IMAGE_HEIGHT*/, 3);
    std::vector<uint8_t> gap((size_t)3 * 3 * 4 * 2, 0xEE);
    glGetTexImage(0x8C1A, 0, 0x1908, 0x1401, gap.data());
    Check(glGetError() == 0, "2D array PACK_IMAGE_HEIGHT no error");
    size_t row = (size_t)W * 4;
    bool gapOk = true;
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                for (int c = 0; c < 4; ++c) {
                    size_t off = ((size_t)z * 3 + y) * row + (size_t)x * 4 + c;
                    if (gap[off] != P(x, y, z, c)) gapOk = false;
                }
    Check(gapOk, "PACK_IMAGE_HEIGHT đặt layer đúng stride");

    glPixelStorei(0x806C, 0);
    glPixelStorei(0x0D05, 4);
    glDeleteTextures(1, &arr);
    glDeleteTextures(1, &tex);
    if (gFails) return 1;
    printf("test_get_texture_3d PASS\n");
    return 0;
}
