// test_copy_image_cube_apple.cpp — GPU path blitCopy từng face của cube map.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_copy_image_cube_apple FAIL: %s\n", msg); ++gFails; }
}
static void Fill(std::vector<unsigned char>& px, int face) {
    px.assign(2 * 2 * 4, 0);
    for (int i = 0; i < 4; ++i) {
        px[i * 4 + 0] = (unsigned char)(40 + face * 20);
        px[i * 4 + 1] = (unsigned char)(80 + face * 10);
        px[i * 4 + 2] = (unsigned char)(120 + face * 5);
        px[i * 4 + 3] = 255;
    }
}
int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (!ctx.device || ctx.device->isNull()) {
        printf("test_copy_image_cube_apple SKIP: no metal device\n");
        return 0;
    }
    GLuint src = 0, dst = 0;
    glGenTextures(1, &src);
    glBindTexture(0x8513, src);
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> px;
        Fill(px, f);
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px.data());
    }
    glGenTextures(1, &dst);
    glBindTexture(0x8513, dst);
    std::vector<unsigned char> zero(2 * 2 * 4, 0);
    for (int f = 0; f < 6; ++f)
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, zero.data());
    Check(glGetError() == 0, "setup cube GPU");

    glCopyImageSubData(src, 0x8513, 0, 0, 0, 2, dst, 0x8513, 0, 0, 0, 4, 2, 2, 2);
    Check(glGetError() == 0, "glCopyImageSubData cube GPU");
    for (int f = 0; f < 2; ++f) {
        std::vector<unsigned char> got(2 * 2 * 4, 0);
        glGetTexImage((GLenum)(0x8515 + 4 + f), 0, 0x1908, 0x1401, got.data());
        Check(glGetError() == 0, "glGetTexImage cube face");
        std::vector<unsigned char> want;
        Fill(want, 2 + f);
        Check(got == want, "face pixels khớp");
    }
    if (gFails) return 1;
    printf("test_copy_image_cube_apple PASS\n");
    return 0;
}
