// test_copy_image_3d.cpp — glCopyImageSubData/EXT_copy_image phải copy đủ volume
// trên Null backend (CPU shadow): zoffset/depth cho TEXTURE_3D và TEXTURE_2D_ARRAY.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_copy_image_3d FAIL: %s\n", msg); ++gFails; }
}
static void ExpectErr(GLenum want, const char* msg) {
    GLenum got = glGetError();
    if (got != want) {
        printf("test_copy_image_3d FAIL: %s: glerr 0x%04x != 0x%04x\n", msg, got, want);
        ++gFails;
    }
}
static unsigned char Pattern(int x, int y, int z, int c) {
    return (unsigned char)(3 + x * 5 + y * 17 + z * 29 + c * 53);
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 3, H = 2, D = 4;
    std::vector<uint8_t> src((size_t)W * H * D * 4);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                for (int c = 0; c < 4; ++c)
                    src[((((size_t)z * H + y) * W + x) * 4) + c] = Pattern(x, y, z, c);

    GLuint s3 = 0, d3 = 0;
    glGenTextures(1, &s3);
    glBindTexture(0x806F, s3);
    glTexImage3D(0x806F, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, src.data());
    glGenTextures(1, &d3);
    glBindTexture(0x806F, d3);
    std::vector<uint8_t> zero((size_t)W * H * D * 4, 0);
    glTexImage3D(0x806F, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, zero.data());
    Check(glGetError() == 0, "setup 3D source/dest");

    // Copy 2 slice từ src z=1 sang dst z=2; ngoài vùng đích vẫn zero.
    glCopyImageSubData(s3, 0x806F, 0, 0, 0, 1, d3, 0x806F, 0, 1, 1, 2, W - 1, H - 1, 2);
    Check(glGetError() == 0, "copy 3D d=2 không lỗi");
    auto& d = ctx.textures[d3];
    for (int z = 0; z < D; ++z) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const bool inCopy = (x >= 1 && y >= 1 && z >= 2 && z <= 3);
                const uint8_t* p = d.pixels.data() + ((((size_t)z * H + y) * W + x) * 4);
                for (int c = 0; c < 4; ++c) {
                    const unsigned char want = inCopy
                        ? Pattern(x - 1, y - 1, z - 2 + 1, c) : 0;
                    if (p[c] != want) {
                        printf("test_copy_image_3d FAIL: 3D dst (%d,%d,%d) c%d = %u != %u\n",
                               x, y, z, c, p[c], want);
                        ++gFails;
                    }
                }
            }
        }
    }

    // 2D array: layer 1 copy sang layer 3, giữ layer 0/2.
    GLuint sa = 0, da = 0;
    glGenTextures(1, &sa);
    glBindTexture(0x8C1A, sa);
    glTexImage3D(0x8C1A, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, src.data());
    glGenTextures(1, &da);
    glBindTexture(0x8C1A, da);
    glTexImage3D(0x8C1A, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, zero.data());
    Check(glGetError() == 0, "setup 2D array source/dest");
    glCopyImageSubData(sa, 0x8C1A, 0, 0, 0, 1, da, 0x8C1A, 0, 0, 0, 3, W, H, 1);
    Check(glGetError() == 0, "copy 2D array layer không lỗi");
    auto& daTex = ctx.textures[da];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            for (int c = 0; c < 4; ++c) {
                const uint8_t* p = daTex.pixels.data() + ((((size_t)3 * H + y) * W + x) * 4);
                const unsigned char want = Pattern(x, y, 1, c);
                if (p[c] != want) {
                    printf("test_copy_image_3d FAIL: array layer3 (%d,%d) c%d = %u != %u\n",
                           x, y, c, p[c], want);
                    ++gFails;
                }
            }

    // Vượt biên depth → INVALID_VALUE.
    glCopyImageSubData(s3, 0x806F, 0, 0, 0, 1, d3, 0x806F, 0, 0, 0, 3, W, H, 2);
    ExpectErr(0x0501, "copy 3D vượt depth");

    glDeleteTextures(1, &da);
    glDeleteTextures(1, &sa);
    glDeleteTextures(1, &d3);
    glDeleteTextures(1, &s3);
    if (gFails) return 1;
    printf("test_copy_image_3d PASS\n");
    return 0;
}
