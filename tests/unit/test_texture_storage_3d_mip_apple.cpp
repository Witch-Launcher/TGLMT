// test_texture_storage_3d_mip_apple.cpp — GPU array/3D phải có đủ mip levels;
// FBO array mip level>0 phải attach/read được (Metal view đúng level/slice).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_texture_storage_3d_mip_apple FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (!ctx.device || ctx.device->isNull()) {
        printf("test_texture_storage_3d_mip_apple SKIP: no metal device\n");
        return 0;
    }
    GLuint arr = 0, fbo = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A, arr);
    glTexStorage3D(0x8C1A, 3, 0x8058, 4, 4, 2);
    auto& a = ctx.textures[arr];
    Check(a.gpu && a.gpu->levelCount() == 3, "array GPU levelCount=3");
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arr, 1, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "array mip1 layer1 FBO");
    glViewport(0, 0, 2, 2);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    unsigned char p[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, p);
    Check(glGetError() == 0 && p[2] > 200 && p[0] < 50,
          "array mip1 clear/read");

    GLuint tex3 = 0;
    glGenTextures(1, &tex3);
    glBindTexture(0x806F, tex3);
    glTexStorage3D(0x806F, 3, 0x8058, 4, 4, 4);
    auto& t = ctx.textures[tex3];
    Check(t.gpu && t.gpu->levelCount() == 3, "3D GPU levelCount=3");
    unsigned char px[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) px[i] = (unsigned char)(10 + i);
    glTexSubImage3D(0x806F, 1, 0, 0, 0, 2, 2, 2, 0x1908, 0x1401, px);
    Check(glGetError() == 0, "glTexSubImage3D 3D level1 GPU");
    unsigned char got[2 * 2 * 2 * 4] = {0};
    glGetTextureImage(tex3, 1, 0x1908, 0x1401, sizeof(got), got);
    Check(glGetError() == 0 && got[0] == 10, "3D mip1 readback");

    // Legacy glTexImage3D(level>0) trên base 1-level: phải promote GPU mip
    // chain từ shadow, không giữ texture non-mipmapped.
    GLuint legacy = 0;
    glGenTextures(1, &legacy);
    glBindTexture(0x806F, legacy);
    unsigned char base[4 * 4 * 4 * 4];
    for (int i = 0; i < 4 * 4 * 4 * 4; ++i) base[i] = (unsigned char)(i & 0xFF);
    glTexImage3D(0x806F, 0, 0x8058, 4, 4, 4, 0, 0x1908, 0x1401, base);
    unsigned char m1[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) m1[i] = (unsigned char)(100 + i);
    glTexImage3D(0x806F, 1, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, m1);
    auto& lg = ctx.textures[legacy];
    Check(glGetError() == 0, "legacy 3D level1 no error");
    Check(lg.gpu && lg.gpu->levelCount() == 2, "legacy 3D promote levelCount=2");
    unsigned char gm[2 * 2 * 2 * 4] = {0};
    glGetTextureImage(legacy, 1, 0x1908, 0x1401, sizeof(gm), gm);
    Check(glGetError() == 0 && gm[0] == 100, "legacy 3D mip1 readback");

    GLuint legacyArr = 0;
    glGenTextures(1, &legacyArr);
    glBindTexture(0x8C1A, legacyArr);
    unsigned char abase[4 * 4 * 2 * 4];
    for (int i = 0; i < 4 * 4 * 2 * 4; ++i) abase[i] = (unsigned char)(i & 0xFF);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, abase);
    unsigned char am1[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) am1[i] = (unsigned char)(50 + i);
    glTexImage3D(0x8C1A, 1, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, am1);
    auto& la = ctx.textures[legacyArr];
    Check(glGetError() == 0, "legacy array level1 no error");
    Check(la.gpu && la.gpu->levelCount() == 2, "legacy array promote levelCount=2");
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, legacyArr, 1, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "legacy array mip1 FBO");
    glViewport(0, 0, 2, 2);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char ap[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, ap);
    Check(glGetError() == 0 && ap[1] > 200 && ap[0] < 50,
          "legacy array mip1 clear/read");

    glDeleteTextures(1, &legacyArr);
    glDeleteTextures(1, &legacy);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex3);
    glDeleteTextures(1, &arr);
    if (gFails) return 1;
    printf("test_texture_storage_3d_mip_apple PASS\n");
    return 0;
}
