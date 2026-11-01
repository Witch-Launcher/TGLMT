// test_copy_3d.cpp — Unit: glCopyTexSubImage3D writes one volume slice.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_copy_3d FAIL: %s\n", m); ++Fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 4, H = 4, D = 3;
    unsigned char src[H][W][4];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            src[y][x][0] = (unsigned char)(10 + y * W + x);
            src[y][x][1] = (unsigned char)(y * 10 + x);
            src[y][x][2] = 3; src[y][x][3] = 255;
        }

    GLuint srcTex = 0, fbo = 0, tex3d = 0;
    glGenTextures(1, &srcTex);
    glBindTexture(0x0DE1, srcTex);
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, src);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, srcTex, 0);
    glReadBuffer(0x8CE0);

    glGenTextures(1, &tex3d);
    glBindTexture(0x806F /*TEXTURE_3D*/, tex3d);
    glTexImage3D(0x806F, 0, 0x8058, W, H, D, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "3D storage");
    glCopyTexSubImage3D(0x806F, 0, 1, 2, 1, 0, 1, 2, 2);
    Check(glGetError() == 0, "copy slice");

    auto& t = ctx.textures[tex3d];
    Check(t.d == D, "3D depth");
    Check(t.pixels.size() >= (size_t)W * H * D * 4, "3D shadow size");
    const unsigned char* row = t.pixels.data() +
        (((size_t)1 * H + 2) * W + 1) * 4;
    Check(memcmp(row, src[1], 2 * 4) == 0, "slice row 0");
    const unsigned char* row2 = t.pixels.data() +
        (((size_t)1 * H + 3) * W + 1) * 4;
    Check(memcmp(row2, src[2], 2 * 4) == 0, "slice row 1");

    glCopyTextureSubImage3D(tex3d, 0, 0, 0, 2, 0, 0, 1, 1);
    Check(glGetError() == 0, "DSA copy slice");

    glBindFramebuffer(0x8D40, 0);
    glDeleteTextures(1, &tex3d);
    glDeleteTextures(1, &srcTex);
    glDeleteFramebuffers(1, &fbo);
    return Fails ? 1 : 0;
}
