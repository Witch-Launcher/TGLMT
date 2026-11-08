// test_texture_target_validation.cpp — target không hợp lệ của glTexImage2D
// phải INVALID_ENUM; proxy target phải no-op không lỗi, không tạo storage.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint cube = 0;
    glGenTextures(1, &cube);
    glBindTexture(0x8513 /*TEXTURE_CUBE_MAP*/, cube);
    unsigned char px[16] = {0};
    glTexImage2D(0x8513, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px);
    if (glGetError() != 0x0500) {
        printf("test_texture_target_validation FAIL: cube map target phải INVALID_ENUM\n");
        return 1;
    }
    if (ctx.textures[cube].w != 0 || ctx.textures[cube].gpu) {
        printf("test_texture_target_validation FAIL: không được tạo storage\n");
        return 1;
    }

    // Proxy cube map: probe hợp lệ, no-op, không lỗi.
    glTexImage2D(0x851B /*PROXY_TEXTURE_CUBE_MAP*/, 0, 0x8058, 2, 2, 0,
                 0x1908, 0x1401, nullptr);
    if (glGetError() != 0) {
        printf("test_texture_target_validation FAIL: proxy cube lỗi\n");
        return 1;
    }

    GLint qw = -1, qh = -1, qf = -1;
    glGetTexLevelParameteriv(0x851B, 0, 0x1000, &qw);
    glGetTexLevelParameteriv(0x851B, 0, 0x1001, &qh);
    glGetTexLevelParameteriv(0x851B, 0, 0x1003, &qf);
    if (glGetError() != 0 || qw != 2 || qh != 2 || qf != 0x8058) {
        printf("test_texture_target_validation FAIL: proxy cube query %d %d 0x%x\n", qw, qh, qf);
        return 1;
    }

    GLuint arr = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A /*TEXTURE_2D_ARRAY*/, arr);
    glTexImage3D(0x8C1B /*PROXY_TEXTURE_2D_ARRAY*/, 0, 0x8058, 2, 2, 2, 0,
                 0x1908, 0x1401, nullptr);
    if (glGetError() != 0) {
        printf("test_texture_target_validation FAIL: proxy 2D array lỗi\n");
        return 1;
    }
    glTexImage3D(0x8C1A, 0, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, nullptr);
    if (glGetError() != 0 || ctx.textures[arr].w != 2 || ctx.textures[arr].d != 2) {
        printf("test_texture_target_validation FAIL: 2D array storage\n");
        return 1;
    }

    // OpenGL has no 3D multisample texture target. The *3DMultisample entry
    // points take GL_TEXTURE_2D_MULTISAMPLE_ARRAY (0x9102); GL_TEXTURE_3D
    // must be rejected, not silently treated as MSAA 3D.
    GLuint ms3d = 0;
    glGenTextures(1, &ms3d);
    glBindTexture(0x806F /*TEXTURE_3D*/, ms3d);
    glTexImage3DMultisample(0x806F, 4, 0x8058, 2, 2, 2, GL_TRUE);
    if (glGetError() != 0x0500 || ctx.textures[ms3d].w != 0) {
        printf("test_texture_target_validation FAIL: glTexImage3DMultisample(TEXTURE_3D) phải INVALID_ENUM, không tạo storage\n");
        return 1;
    }
    glTexStorage3DMultisample(0x806F, 4, 0x8058, 2, 2, 2, GL_TRUE);
    if (glGetError() != 0x0500 || ctx.textures[ms3d].w != 0) {
        printf("test_texture_target_validation FAIL: glTexStorage3DMultisample(TEXTURE_3D) phải INVALID_ENUM\n");
        return 1;
    }

    GLuint msarr = 0;
    glGenTextures(1, &msarr);
    glBindTexture(0x9102 /*TEXTURE_2D_MULTISAMPLE_ARRAY*/, msarr);
    glTexImage3DMultisample(0x9102, 4, 0x8058, 2, 3, 2, GL_TRUE);
    if (glGetError() != 0 || ctx.textures[msarr].w != 2 || ctx.textures[msarr].h != 3 ||
        ctx.textures[msarr].d != 2) {
        printf("test_texture_target_validation FAIL: 2D multisample array storage\n");
        return 1;
    }
    glTexStorage3DMultisample(0x9102, 4, 0x8058, 2, 2, 2, GL_TRUE);
    if (glGetError() != 0) {
        printf("test_texture_target_validation FAIL: glTexStorage3DMultisample(2D_MS_ARRAY) lỗi\n");
        return 1;
    }

    printf("test_texture_target_validation PASS\n");
    return 0;
}
