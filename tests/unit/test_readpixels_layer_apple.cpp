// test_readpixels_layer_apple.cpp — GPU path: render vào layer/face/mip rồi
// glReadPixels phải đọc đúng view (textureLayerView/textureView).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_readpixels_layer_apple FAIL: %s\n", msg); ++gFails; }
}

static void ClearAndRead(GLuint fbo, int w, int h, float r, float g, float b) {
    glBindFramebuffer(0x8D40, fbo);
    glViewport(0, 0, w, h);
    glClearColor(r, g, b, 1.0f);
    glClear(0x00004000);
    unsigned char out[4 * 4 * 4] = {0};
    glReadPixels(0, 0, w, h, 0x1908, 0x1401, out);
    Check(glGetError() == 0, "glReadPixels");
    Check(out[0] == (unsigned char)(r * 255.0f + 0.5f) &&
          out[1] == (unsigned char)(g * 255.0f + 0.5f) &&
          out[2] == (unsigned char)(b * 255.0f + 0.5f) && out[3] == 255,
          "pixel màu clear");
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (!ctx.device || ctx.device->isNull()) {
        printf("test_readpixels_layer_apple SKIP: no metal device\n");
        return 0;
    }

    GLuint arr = 0, fbo = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A, arr);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arr, 0, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "array FBO complete");
    ClearAndRead(fbo, 4, 4, 0.0f, 1.0f, 0.0f);
    // Layer 0 phải vẫn đen (clear chỉ layer 1).
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arr, 0, 0);
    unsigned char l0[16] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, l0);
    Check(l0[0] == 0 && l0[1] == 0 && l0[2] == 0, "layer 0 không bị clear");

    GLuint cube = 0;
    glGenTextures(1, &cube);
    glBindTexture(0x8513, cube);
    for (int f = 0; f < 6; ++f)
        glTexImage2D(0x8515 + f, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x8517, cube, 0); // face +2
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "cube FBO complete");
    ClearAndRead(fbo, 4, 4, 1.0f, 0.0f, 0.0f);

    GLuint mip = 0;
    glGenTextures(1, &mip);
    glBindTexture(0x0DE1, mip);
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
    glTexImage2D(0x0DE1, 1, 0x8058, 2, 2, 0, 0x1908, 0x1401, nullptr);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, mip, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "mip FBO complete");
    ClearAndRead(fbo, 2, 2, 0.0f, 0.0f, 1.0f);

    if (gFails) return 1;
    printf("test_readpixels_layer_apple PASS\n");
    return 0;
}
