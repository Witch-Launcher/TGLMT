// test_texture_storage_3d_mip.cpp — glTexStorage3D phải cấp đủ mip chain cho
// 2D_ARRAY/3D (levels + dims per-level), không bỏ qua tham số levels.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_texture_storage_3d_mip FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint arr = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A, arr);
    glTexStorage3D(0x8C1A, 3, 0x8058, 4, 4, 2);
    Check(glGetError() == 0, "glTexStorage3D 2D array levels=3");
    auto& a = ctx.textures[arr];
    Check(a.levels == 3, "array levels=3");
    Check(a.mipData.count(1) && a.mipData[1].w == 2 && a.mipData[1].h == 2 &&
          a.mipData[1].d == 2, "array mip1 dims 2x2x2");
    Check(a.mipData.count(2) && a.mipData[2].w == 1 && a.mipData[2].h == 1 &&
          a.mipData[2].d == 2, "array mip2 dims 1x1x2 (layers giữ nguyên)");

    std::vector<unsigned char> px(2 * 2 * 4, 7);
    glTexSubImage3D(0x8C1A, 1, 0, 0, 1, 2, 2, 1, 0x1908, 0x1401, px.data());
    Check(glGetError() == 0, "glTexSubImage3D array level1 layer1");
    Check(a.mipData[1].pixels.size() == (size_t)2 * 2 * 2 * 4,
          "array mip1 shadow size");
    Check(a.mipData[1].pixels[(size_t)2 * 2 * 4] == 7,
          "array mip1 layer1 data");

    GLuint tex3 = 0;
    glGenTextures(1, &tex3);
    glBindTexture(0x806F, tex3);
    glTexStorage3D(0x806F, 3, 0x8058, 4, 4, 4);
    Check(glGetError() == 0, "glTexStorage3D 3D levels=3");
    auto& t = ctx.textures[tex3];
    Check(t.levels == 3, "3D levels=3");
    Check(t.mipData.count(1) && t.mipData[1].w == 2 && t.mipData[1].h == 2 &&
          t.mipData[1].d == 2, "3D mip1 dims 2x2x2");
    Check(t.mipData.count(2) && t.mipData[2].w == 1 && t.mipData[2].h == 1 &&
          t.mipData[2].d == 1, "3D mip2 dims 1x1x1");

    std::vector<unsigned char> px3(8, 9);
    glTexSubImage3D(0x806F, 1, 0, 0, 0, 2, 2, 2, 0x1908, 0x1401, px3.data());
    Check(glGetError() == 0, "glTexSubImage3D 3D level1");
    Check(t.mipData[1].pixels.size() == (size_t)2 * 2 * 2 * 4,
          "3D mip1 shadow size");

    // max level cho max(4,4,4) = 3; level 4 → INVALID_VALUE.
    GLuint bad = 0;
    glGenTextures(1, &bad);
    glBindTexture(0x8C1A, bad);
    glTexStorage3D(0x8C1A, 4, 0x8058, 4, 4, 2);
    Check(glGetError() == 0x0501, "levels quá max rejected");

    if (gFails) return 1;
    printf("test_texture_storage_3d_mip PASS\n");
    return 0;
}
