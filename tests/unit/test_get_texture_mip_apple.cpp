// test_get_texture_mip_apple.cpp — glGetTextureImage/glGetTexImage level>0 phải
// đọc GPU mip thật khi shadow per-level không tồn tại (immutable storage).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_get_texture_mip_apple FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (!ctx.device || ctx.device->isNull()) {
        printf("test_get_texture_mip_apple SKIP: no metal device\n");
        return 0;
    }
    GLuint fbo = 0, tex = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);

    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexStorage2D(0x0DE1, 3, 0x8058, 8, 8);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "2D mip1 FBO");
    glViewport(0, 0, 4, 4);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    unsigned char got[4 * 4 * 4] = {0};
    glGetTextureImage(tex, 1, 0x1908, 0x1401, sizeof(got), got);
    Check(glGetError() == 0 && got[2] > 200 && got[0] < 50,
          "glGetTextureImage 2D mip1 GPU");

    GLuint arr = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A, arr);
    glTexStorage3D(0x8C1A, 3, 0x8058, 8, 8, 2);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arr, 1, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "array mip1 layer1 FBO");
    glViewport(0, 0, 4, 4);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char agot[4 * 4 * 2 * 4] = {0};
    glGetTextureImage(arr, 1, 0x1908, 0x1401, sizeof(agot), agot);
    Check(glGetError() == 0 && agot[(size_t)4 * 4 * 4 + 1] > 200,
          "glGetTextureImage array mip1 layer1 GPU");

    GLuint cube = 0;
    glGenTextures(1, &cube);
    glBindTexture(0x8513, cube);
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> z(8 * 8 * 4, 0);
        glTexImage2D(0x8515 + f, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, z.data());
        glTexImage2D(0x8515 + f, 1, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
    }
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x8519, cube, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "cube +X mip1 FBO");
    glViewport(0, 0, 4, 4);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char cgot[4 * 4 * 4] = {0};
    glGetTexImage(0x8519, 1, 0x1908, 0x1401, cgot);
    Check(glGetError() == 0 && cgot[0] > 200 && cgot[1] < 50,
          "glGetTexImage cube +X mip1 GPU");

    unsigned char sub[2 * 2 * 4];
    for (int i = 0; i < 2 * 2; ++i) {
        sub[i * 4 + 0] = 0; sub[i * 4 + 1] = 255;
        sub[i * 4 + 2] = 0; sub[i * 4 + 3] = 255;
    }
    glTexSubImage2D(0x8519, 1, 1, 1, 2, 2, 0x1908, 0x1401, sub);
    unsigned char sgot[4 * 4 * 4] = {0};
    glGetTexImage(0x8519, 1, 0x1908, 0x1401, sgot);
    Check(glGetError() == 0 && sgot[0] > 200 &&
          sgot[((size_t)1 * 4 + 1) * 4 + 1] > 200,
          "cube mip1 subimage giữ pixel ngoài vùng + ghi vùng mới");

    glDeleteTextures(1, &cube);
    glDeleteTextures(1, &arr);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fbo);
    if (gFails) return 1;
    printf("test_get_texture_mip_apple PASS\n");
    return 0;
}
