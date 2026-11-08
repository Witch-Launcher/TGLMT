// test_texture_3d_types.cpp — Bpp 3D phải xét type: HALF_FLOAT/USHORT/DOUBLE
// không được cấp thiếu bộ nhớ shadow (trước đây coi mọi type là 1 byte/component).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int fails = 0;
static void Check(bool ok, const char* what) {
    if (!ok) { printf("FAIL: %s\n", what); ++fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint half = 0;
    glGenTextures(1, &half);
    glBindTexture(0x806F, half);
    std::vector<unsigned char> hsrc(2 * 2 * 2 * 4 * 2);
    for (size_t i = 0; i < hsrc.size(); ++i) hsrc[i] = (unsigned char)(i * 7 + 3);
    glTexImage3D(0x806F, 0, 0x881A /*RGBA16F*/, 2, 2, 2, 0,
                 0x1908 /*RGBA*/, 0x140B /*HALF_FLOAT*/, hsrc.data());
    Check(glGetError() == 0, "RGBA16F/HALF upload no error");
    auto& th = ctx.textures[half];
    Check(th.pixels.size() == hsrc.size(), "RGBA16F shadow đủ 2 byte/component");
    Check(th.pixels == hsrc, "RGBA16F shadow giữ đúng bytes");

    GLuint us = 0;
    glGenTextures(1, &us);
    glBindTexture(0x806F, us);
    std::vector<unsigned char> ussrc(2 * 2 * 2 * 2);
    for (size_t i = 0; i < ussrc.size(); ++i) ussrc[i] = (unsigned char)(255 - i);
    glTexImage3D(0x806F, 0, 0x822E /*R16UI*/, 2, 2, 2, 0,
                 0x1903 /*RED*/, 0x1403 /*UNSIGNED_SHORT*/, ussrc.data());
    Check(glGetError() == 0, "R16UI/USHORT upload no error");
    auto& tu = ctx.textures[us];
    Check(tu.pixels.size() == ussrc.size(), "R16UI shadow đủ 2 byte/component");
    Check(tu.pixels == ussrc, "R16UI shadow giữ đúng bytes");

    GLuint dbl = 0;
    glGenTextures(1, &dbl);
    glBindTexture(0x806F, dbl);
    std::vector<unsigned char> dsrc(2 * 2 * 2 * 8);
    for (size_t i = 0; i < dsrc.size(); ++i) dsrc[i] = (unsigned char)(i * 3 + 1);
    glTexImage3D(0x806F, 0, 0x822F /*R32F*/, 2, 2, 2, 0,
                 0x1903 /*RED*/, 0x140A /*DOUBLE*/, dsrc.data());
    Check(glGetError() == 0, "R32F/DOUBLE upload no error");
    auto& td = ctx.textures[dbl];
    Check(td.pixels.size() == dsrc.size(), "DOUBLE shadow đủ 8 byte/component");

    // Sub-image HALF ở offset z=1 phải ghi đúng 2 byte/component, không tràn.
    std::vector<unsigned char> sub(2 * 2 * 2 * 2, 0xAB);
    glTexSubImage3D(0x806F, 0, 0, 0, 1, 2, 2, 1,
                    0x1908, 0x140B, sub.data());
    Check(glGetError() == 0, "HALF sub-image no error");
    Check(th.pixels.size() == hsrc.size(), "HALF sub-image không đổi size");

    if (fails) return 1;
    printf("test_texture_3d_types PASS\n");
    return 0;
}
