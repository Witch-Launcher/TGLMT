// test_copy_1d.cpp — Unit: 1D copy texture paths (storage is 2D w×1).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_copy_1d FAIL: %s\n", m); ++Fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 8, H = 4;
    unsigned char src[H][W][4];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            src[y][x][0] = (unsigned char)(y * 40 + x);
            src[y][x][1] = (unsigned char)(y + 1);
            src[y][x][2] = 0x22;
            src[y][x][3] = 0xFF;
        }

    GLuint srcTex = 0, fbo = 0, dst1d = 0;
    glGenTextures(1, &srcTex);
    glBindTexture(0x0DE1, srcTex);
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, src);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, srcTex, 0);
    glReadBuffer(0x8CE0);
    Check(glGetError() == 0, "source FBO setup");

    glGenTextures(1, &dst1d);
    glBindTexture(0x0DE0 /*TEXTURE_1D*/, dst1d);
    glCopyTexImage1D(0x0DE0, 0, 0x8058, 0, 2, W, 0);
    Check(glGetError() == 0, "glCopyTexImage1D");
    GLint w = 0, h = 0;
    glGetTexLevelParameteriv(0x0DE0, 0, 0x1000 /*WIDTH*/, &w);
    glGetTexLevelParameteriv(0x0DE0, 0, 0x1001 /*HEIGHT*/, &h);
    Check(w == W && h == 1, "1D size");

    unsigned char out[W][4] = {};
    glGetTexImage(0x0DE0, 0, 0x1908, 0x1401, out);
    Check(glGetError() == 0, "glGetTexImage 1D");
    Check(memcmp(out, src[2], sizeof(out)) == 0, "copy row 2 to 1D");

    glCopyTexSubImage1D(0x0DE0, 0, 3, 0, 0, 3);
    Check(glGetError() == 0, "glCopyTexSubImage1D");
    glBindFramebuffer(0x8D40, 0);

    glCopyTextureSubImage1D(dst1d, 0, 0, 0, 1, 2);
    Check(glGetError() == 0, "glCopyTextureSubImage1D");

    const unsigned char patch[3][4] = {
        {0x11, 0x22, 0x33, 0xFF},
        {0x44, 0x55, 0x66, 0xFF},
        {0x77, 0x88, 0x99, 0xFF},
    };
    glTextureSubImage1D(dst1d, 0, 2, 3, 0x1908, 0x1401, patch);
    Check(glGetError() == 0, "glTextureSubImage1D");
    memset(out, 0, sizeof(out));
    glGetTexImage(0x0DE0, 0, 0x1908, 0x1401, out);
    Check(glGetError() == 0, "glGetTexImage after glTextureSubImage1D");
    Check(memcmp(out[2], patch, sizeof(patch)) == 0, "glTextureSubImage1D data");

    glDeleteTextures(1, &dst1d);
    glDeleteTextures(1, &srcTex);
    glDeleteFramebuffers(1, &fbo);
    return Fails ? 1 : 0;
}
