// test_readpixels_layer.cpp — glReadPixels đọc đúng layer/face/mip của FBO.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("%s FAIL: %s\n", tag, msg); ++fails; } } while (0)

static void testArrayLayer() {
    const char* tag = "array-layer";
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x8C1A, tex); // TEXTURE_2D_ARRAY
    std::vector<unsigned char> data((size_t)2 * 2 * 3 * 4);
    for (int z = 0; z < 3; ++z)
        for (int i = 0; i < 4; ++i) {
            data[((size_t)z * 4 + i) * 4 + 0] = (unsigned char)(30 + z * 50);
            data[((size_t)z * 4 + i) * 4 + 1] = 1;
            data[((size_t)z * 4 + i) * 4 + 2] = 2;
            data[((size_t)z * 4 + i) * 4 + 3] = 255;
        }
    glTexImage3D(0x8C1A, 0, 0x8058, 2, 2, 3, 0, 0x1908, 0x1401, data.data());
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 2);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    CHECK(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");
    unsigned char out[2 * 2 * 4] = {0};
    glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, out);
    CHECK(glGetError() == 0, "glReadPixels layer");
    CHECK(out[0] == 130 && out[1] == 1 && out[2] == 2 && out[3] == 255,
          "layer 2 pixel");
}

static void testCubeFace() {
    const char* tag = "cube-face";
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x8513, tex); // TEXTURE_CUBE_MAP
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> data((size_t)2 * 2 * 4);
        for (int i = 0; i < 4; ++i) {
            data[i * 4 + 0] = (unsigned char)(10 + f * 30);
            data[i * 4 + 1] = 9;
            data[i * 4 + 2] = 8;
            data[i * 4 + 3] = 255;
        }
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, data.data());
    }
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x8515 + 3, tex, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    CHECK(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");
    unsigned char out[2 * 2 * 4] = {0};
    glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, out);
    CHECK(glGetError() == 0, "glReadPixels cube face");
    CHECK(out[0] == 100 && out[1] == 9 && out[2] == 8 && out[3] == 255,
          "face +3 pixel");
}

static void testMipLevel() {
    const char* tag = "mip-level";
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex); // TEXTURE_2D
    for (int i = 0; i < 4 * 4; ++i) {
        unsigned char px[4] = {1, 2, 3, 255};
        glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, px);
    }
    unsigned char mip[2 * 2 * 4];
    for (int i = 0; i < 4; ++i) {
        mip[i * 4 + 0] = 77; mip[i * 4 + 1] = 66;
        mip[i * 4 + 2] = 55; mip[i * 4 + 3] = 255;
    }
    glTexImage2D(0x0DE1, 1, 0x8058, 2, 2, 0, 0x1908, 0x1401, mip);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    CHECK(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");
    unsigned char out[2 * 2 * 4] = {0};
    glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, out);
    CHECK(glGetError() == 0, "glReadPixels mip");
    CHECK(out[0] == 77 && out[1] == 66 && out[2] == 55 && out[3] == 255,
          "mip level 1 pixel");
}

int main() {
    testArrayLayer();
    testCubeFace();
    testMipLevel();
    if (fails) return 1;
    printf("test_readpixels_layer PASS\n");
    return 0;
}
