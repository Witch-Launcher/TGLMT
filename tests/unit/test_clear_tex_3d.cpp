// test_clear_tex_3d.cpp — glClearTexSubImage/glClearTexImage phải clear đủ volume.
// Spec GL 4.6 §8.12: zoffset/depth áp dụng cho TEXTURE_3D và từng layer của
// TEXTURE_2D_ARRAY; dữ liệu ngoài vùng clear phải giữ nguyên.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_clear_tex_3d FAIL: %s\n", msg); ++gFails; }
}
static void ExpectErr(GLenum want, const char* msg) {
    GLenum got = glGetError();
    if (got != want) {
        printf("test_clear_tex_3d FAIL: %s: glerr 0x%04x != 0x%04x\n", msg, got, want);
        ++gFails;
    }
}

static unsigned char Pattern(int x, int y, int z, int c) {
    return (unsigned char)(10 + x * 3 + y * 7 + z * 11 + c * 41);
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 4, H = 3, D = 3;
    std::vector<uint8_t> src((size_t)W * H * D * 4);
    for (int z = 0; z < D; ++z)
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                for (int c = 0; c < 4; ++c)
                    src[((((size_t)z * H + y) * W + x) * 4) + c] = Pattern(x, y, z, c);

    GLuint tex3d = 0;
    glGenTextures(1, &tex3d);
    glBindTexture(0x806F /*TEXTURE_3D*/, tex3d);
    glTexImage3D(0x806F, 0, 0x8058 /*RGBA8*/, W, H, D, 0, 0x1908, 0x1401, src.data());
    Check(glGetError() == 0, "setup 3D storage");

    // Clear một texel ở giữa slice 1, giữ nguyên mọi pixel còn lại.
    const unsigned char red[4] = {255, 0, 0, 255};
    glClearTexSubImage(tex3d, 0, 1, 1, 1, 1, 1, 1, 0x1908, 0x1401, red);
    Check(glGetError() == 0, "clear 3D một texel không lỗi");
    auto& t = ctx.textures[tex3d];
    Check(t.pixels.size() >= (size_t)W * H * D * 4, "3D shadow đủ volume");
    for (int z = 0; z < D; ++z) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const uint8_t* p = t.pixels.data() + ((((size_t)z * H + y) * W + x) * 4);
                const bool hit = (z == 1 && y == 1 && x == 1);
                for (int c = 0; c < 4; ++c) {
                    const unsigned char want = hit ? red[c] : Pattern(x, y, z, c);
                    if (p[c] != want) {
                        printf("test_clear_tex_3d FAIL: 3D texel (%d,%d,%d) c%d = %u != %u\n",
                               x, y, z, c, p[c], want);
                        ++gFails;
                    }
                }
            }
        }
    }

    // Clear cả slice z=2 bằng NULL (zero) — chỉ slice đó đổi.
    glClearTexSubImage(tex3d, 0, 0, 0, 2, W, H, 1, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "clear slice 2 bằng NULL không lỗi");
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const uint8_t* p = t.pixels.data() + ((((size_t)2 * H + y) * W + x) * 4);
            Check(p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0,
                  "NULL clear slice 2 = zero");
        }
    const uint8_t* untouched = t.pixels.data() + ((((size_t)0 * H + 2) * W + 2) * 4);
    Check(untouched[0] == Pattern(2, 2, 0, 0) && untouched[1] == Pattern(2, 2, 0, 1),
          "slice 0 ngoài vùng clear giữ nguyên");

    // ClearTexImage phải clear toàn volume, không chỉ mặt 2D đầu tiên.
    glClearTexImage(tex3d, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "glClearTexImage 3D không lỗi");
    bool allZero = true;
    for (size_t i = 0; i < (size_t)W * H * D * 4; ++i)
        if (t.pixels[i] != 0) { allZero = false; break; }
    Check(allZero, "glClearTexImage clear toàn bộ slice 3D");

    // z+depth vượt biên → INVALID_VALUE, không đổi dữ liệu.
    glClearTexSubImage(tex3d, 0, 0, 0, 2, W, H, 2, 0x1908, 0x1401, red);
    ExpectErr(0x0501, "z+depth vượt biên");

    // TEXTURE_2D_ARRAY: z là layer; clear layer 1 không được clear layer 0/2.
    GLuint arr = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A /*TEXTURE_2D_ARRAY*/, arr);
    glTexImage3D(0x8C1A, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, src.data());
    Check(glGetError() == 0, "setup 2D array");
    glClearTexSubImage(arr, 0, 0, 0, 1, W, H, 1, 0x1908, 0x1401, red);
    Check(glGetError() == 0, "clear layer 1 của 2D array không lỗi");
    auto& a = ctx.textures[arr];
    for (int z = 0; z < D; ++z) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const uint8_t* p = a.pixels.data() + ((((size_t)z * H + y) * W + x) * 4);
                for (int c = 0; c < 4; ++c) {
                    const unsigned char want = (z == 1) ? red[c] : Pattern(x, y, z, c);
                    if (p[c] != want) {
                        printf("test_clear_tex_3d FAIL: array layer %d texel (%d,%d) c%d = %u != %u\n",
                               z, x, y, c, p[c], want);
                        ++gFails;
                    }
                }
            }
        }
    }

    glDeleteTextures(1, &arr);
    glDeleteTextures(1, &tex3d);
    if (gFails) return 1;
    printf("test_clear_tex_3d PASS\n");
    return 0;
}
