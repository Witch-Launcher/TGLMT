// test_get_texture_cube.cpp — glGetTexImage/glGetnTexImage cho từng face cube.
// Trước đây face target bị từ chối (target texture = CUBE_MAP != face) nên app
// không đọc được cubemap sau render/upload.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint cube = 0;
    glGenTextures(1, &cube);
    glBindTexture(0x8513, cube);
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> px(3 * 2 * 4, 0);
        for (int i = 0; i < 3 * 2; ++i) {
            px[i * 4 + 0] = (unsigned char)(10 + f);
            px[i * 4 + 1] = (unsigned char)(100 + f);
            px[i * 4 + 2] = (unsigned char)(200 + f);
            px[i * 4 + 3] = 255;
        }
        glTexImage2D(0x8515 + f, 0, 0x8058, 3, 2, 0, 0x1908, 0x1401, px.data());
        if (glGetError() != 0) {
            printf("test_get_texture_cube FAIL: upload face %d\n", f);
            return 1;
        }
    }
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> got(3 * 2 * 4, 0);
        glGetTexImage(0x8515 + f, 0, 0x1908, 0x1401, got.data());
        if (glGetError() != 0) {
            printf("test_get_texture_cube FAIL: glGetTexImage face %d\n", f);
            return 1;
        }
        if (got[0] != 10 + f || got[1] != 100 + f || got[2] != 200 + f) {
            printf("test_get_texture_cube FAIL: face %d pixels (%u,%u,%u)\n",
                   f, got[0], got[1], got[2]);
            return 1;
        }
    }
    GLint qw = 0, qh = 0;
    glGetTexLevelParameteriv(0x8515, 0, 0x1000, &qw);
    glGetTexLevelParameteriv(0x8515, 0, 0x1001, &qh);
    if (glGetError() != 0 || qw != 3 || qh != 2) {
        printf("test_get_texture_cube FAIL: face level query %d %d\n", qw, qh);
        return 1;
    }
    GLint wrap = 0;
    glGetTexParameteriv(0x8516, 0x2802 /*WRAP_S*/, &wrap);
    if (glGetError() != 0) {
        printf("test_get_texture_cube FAIL: face parameter query\n");
        return 1;
    }

    std::vector<unsigned char> got(3 * 2 * 4, 0);
    glGetnTexImage(0x8518, 0, 0x1908, 0x1401, (GLsizei)got.size(), got.data());
    if (glGetError() != 0 || got[0] != 13) {
        printf("test_get_texture_cube FAIL: glGetnTexImage face 3\n");
        return 1;
    }
    printf("test_get_texture_cube PASS\n");
    return 0;
}
