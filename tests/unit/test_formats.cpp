// test_formats.cpp — internalFormat GL → PixelFormat Metal (mọi backend, không cần GPU).
#include "tglmt/gl46.h"
#include "test_util.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    using PF = tglmt::metal::PixelFormat;
    tglmt::GLuint t;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    glPixelStorei(0x0CF2, 1);
    unsigned char px[16] = {0};
    glTexImage2D(0x0DE1, 0, 0x8C43, 2, 2, 0, 0x1908, 0x1401, px); // SRGB8_ALPHA8
    TGLMT_CHECK((ctx.textures[t].gpu), "ctx.textures[t].gpu");
    TGLMT_CHECK((ctx.textures[t].gpu->pixelFormat() == PF::RGBA8Unorm_sRGB), "ctx.textures[t].gpu->pixelFormat() == PF::RGBA8Unorm_sRGB");
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px); // RGBA8
    TGLMT_CHECK((ctx.textures[t].gpu->pixelFormat() == PF::RGBA8Unorm), "ctx.textures[t].gpu->pixelFormat() == PF::RGBA8Unorm");
    tglmt::GLuint t2;
    glCreateTextures(0x0DE1, 1, &t2);
    glTextureStorage2D(t2, 1, 0x8229, 4, 4); // R8
    TGLMT_CHECK((ctx.textures[t2].gpu->pixelFormat() == PF::R8Unorm), "ctx.textures[t2].gpu->pixelFormat() == PF::R8Unorm");
    printf("test_formats PASS\n");
    return TGLMT_RESULT;
}
