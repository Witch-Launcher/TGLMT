// test_copy_tex_3d_mip.cpp — glCopyTexSubImage3D/DSA copy vào mip level > 0.
// Trước đây level != 0 bị trả INVALID_OPERATION dù texture có mip 3D hợp lệ.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint t3 = 0;
    glGenTextures(1, &t3);
    glBindTexture(0x806F, t3);
    glTexImage3D(0x806F, 0, 0x8058, 4, 4, 4, 0, 0x1908, 0x1401, nullptr);
    glTexImage3D(0x806F, 1, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, nullptr);
    if (glGetError() != 0) {
        printf("test_copy_tex_3d_mip FAIL: setup 3D mip\n");
        return 1;
    }

    GLuint src = 0, fbo = 0;
    glGenTextures(1, &src);
    glBindTexture(0x0DE1, src);
    std::vector<unsigned char> px(2 * 2 * 4);
    for (int i = 0; i < 4; ++i) {
        px[i * 4 + 0] = 200;
        px[i * 4 + 1] = 10 + i;
        px[i * 4 + 2] = 30;
        px[i * 4 + 3] = 255;
    }
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px.data());
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, src, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    if (glGetError() != 0) {
        printf("test_copy_tex_3d_mip FAIL: setup FBO\n");
        return 1;
    }

    // DSA copy full mip 1 (2x2x2), source chỉ là một mặt 2x2 → copy 1 slice.
    glCopyTextureSubImage3D(t3, 1, 0, 0, 1, 0, 0, 2, 2);
    if (glGetError() != 0) {
        printf("test_copy_tex_3d_mip FAIL: glCopyTextureSubImage3D level 1\n");
        return 1;
    }
    auto& ml = ctx.textures[t3].mipData[1];
    if (ml.pixels.size() != (size_t)2 * 2 * 2 * 4) {
        printf("test_copy_tex_3d_mip FAIL: mip 1 size %zu\n", ml.pixels.size());
        return 1;
    }
    const unsigned char* p1 = ml.pixels.data() + (size_t)1 * 2 * 2 * 4;
    if (p1[0] != 200 || p1[1] != 10 || p1[2] != 30 || p1[3] != 255) {
        printf("test_copy_tex_3d_mip FAIL: mip 1 slice 1 pixels (%u,%u,%u,%u)\n",
               p1[0], p1[1], p1[2], p1[3]);
        return 1;
    }

    // Đọc lại qua glGetTextureImage(level=1): phải thấy slice 1 vừa copy.
    std::vector<unsigned char> got((size_t)2 * 2 * 2 * 4, 0);
    glGetTextureImage(t3, 1, 0x1908, 0x1401, (GLsizei)got.size(), got.data());
    if (glGetError() != 0) {
        printf("test_copy_tex_3d_mip FAIL: glGetTextureImage level 1\n");
        return 1;
    }
    if (got[(size_t)1 * 2 * 2 * 4] != 200 || got[(size_t)1 * 2 * 2 * 4 + 1] != 10) {
        printf("test_copy_tex_3d_mip FAIL: readback mip 1 slice 1\n");
        return 1;
    }

    // Bound-target path cũng phải copy đúng level 0.
    glBindTexture(0x806F, t3);
    glCopyTexSubImage3D(0x806F, 0, 1, 1, 2, 0, 0, 1, 1);
    if (glGetError() != 0) {
        printf("test_copy_tex_3d_mip FAIL: glCopyTexSubImage3D\n");
        return 1;
    }

    printf("test_copy_tex_3d_mip PASS\n");
    return 0;
}
