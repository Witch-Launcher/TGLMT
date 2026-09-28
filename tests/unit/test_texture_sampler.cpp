// test_texture_sampler.cpp — Unit: texture upload/readback + sampler + pixelstore.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    tglmt::GLuint t;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t); // TEXTURE_2D
    glPixelStorei(0x0CF2, 1); // UNPACK_ALIGNMENT=1
    unsigned char px[4 * 4] = {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px); // RGBA/UNSIGNED_BYTE
    assert(glGetError() == 0);
    assert(ctx.textures[t].pixels.size() == 16);
    assert(ctx.textures[t].pixels[0] == 255 && ctx.textures[t].pixels[1] == 0);
    glTexParameteri(0x0DE1, 0x2801, 0x2601); // MIN_FILTER LINEAR
    assert(ctx.textures[t].params[0x2801] == 0x2601);
    tglmt::GLuint s;
    glGenSamplers(1, &s);
    glSamplerParameteri(s, 0x2801, 0x2601);
    assert(ctx.samplers[s].iparams[0x2801] == 0x2601);
    glBindSampler(0, s);
    glGenerateMipmap(0x0DE1);
    printf("test_texture_sampler PASS\n");
    return 0;
}
