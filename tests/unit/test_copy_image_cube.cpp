// test_copy_image_cube.cpp — glCopyImageSubData phải hiểu target cube map/face,
// không được bỏ qua srcTarget/dstTarget. Null backend đọc shadow faces.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_copy_image_cube FAIL: %s\n", msg); ++gFails; }
}
static void Err(GLenum want, const char* msg) {
    GLenum got = glGetError();
    if (got != want) {
        printf("test_copy_image_cube FAIL: %s: 0x%04x != 0x%04x\n", msg, got, want);
        ++gFails;
    }
}
static void FillFace(std::vector<unsigned char>& out, int face) {
    out.assign(2 * 2 * 4, 0);
    for (int i = 0; i < 4; ++i) {
        out[i * 4 + 0] = (unsigned char)(10 + face * 20);
        out[i * 4 + 1] = (unsigned char)(20 + face * 10);
        out[i * 4 + 2] = (unsigned char)(30 + face * 5);
        out[i * 4 + 3] = 255;
    }
}
static bool FaceIs(const std::vector<unsigned char>& f, int face) {
    if (f.size() != 2 * 2 * 4) return false;
    for (int i = 0; i < 4; ++i) {
        if (f[i * 4 + 0] != (unsigned char)(10 + face * 20) ||
            f[i * 4 + 1] != (unsigned char)(20 + face * 10) ||
            f[i * 4 + 2] != (unsigned char)(30 + face * 5) ||
            f[i * 4 + 3] != 255) return false;
    }
    return true;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint src = 0, dst = 0;
    glGenTextures(1, &src);
    glBindTexture(0x8513, src);
    for (int f = 0; f < 6; ++f) {
        std::vector<unsigned char> px;
        FillFace(px, f);
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px.data());
    }
    glGenTextures(1, &dst);
    glBindTexture(0x8513, dst);
    std::vector<unsigned char> zero(2 * 2 * 4, 0);
    for (int f = 0; f < 6; ++f)
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, zero.data());
    Check(glGetError() == 0, "setup cube");

    // Cube map target: z là face index; copy 2 face liền nhau.
    glCopyImageSubData(src, 0x8513, 0, 0, 0, 2, dst, 0x8513, 0, 0, 0, 4, 2, 2, 2);
    Check(glGetError() == 0, "copy cube faces 2..3 -> 4..5");
    auto& d = ctx.textures[dst];
    Check(FaceIs(d.faces[4], 2), "dest face 4 = source face 2");
    Check(FaceIs(d.faces[5], 3), "dest face 5 = source face 3");
    Check(!FaceIs(d.faces[3], 1), "dest face 3 không bị đổi");

    // Cube-face target: face chỉ định trong target, z/depth phải 0/1.
    glCopyImageSubData(src, 0x8515 + 0, 0, 0, 0, 0,
                       dst, 0x8515 + 1, 0, 0, 0, 0, 2, 2, 1);
    Check(glGetError() == 0, "copy face target +0 -> +1");
    Check(FaceIs(d.faces[1], 0), "dest face 1 = source face 0");

    // z/depth vượt face target → INVALID_VALUE.
    glCopyImageSubData(src, 0x8515, 0, 0, 0, 0,
                       dst, 0x8516, 0, 0, 0, 0, 2, 2, 2);
    Err(0x0501, "face target d=2 rejected");

    // Target không khớp object → INVALID_OPERATION.
    glCopyImageSubData(src, 0x0DE1, 0, 0, 0, 0,
                       dst, 0x8513, 0, 0, 0, 0, 2, 2, 1);
    Err(0x0502, "2D target trên cube object rejected");
    // Target lạ → INVALID_ENUM.
    glCopyImageSubData(src, 0x9999, 0, 0, 0, 0,
                       dst, 0x8513, 0, 0, 0, 0, 2, 2, 1);
    Err(0x0500, "invalid target rejected");
    // Cube mip chưa hỗ trợ → INVALID_OPERATION, không copy nhầm face 0.
    glCopyImageSubData(src, 0x8513, 1, 0, 0, 0,
                       dst, 0x8513, 1, 0, 0, 0, 1, 1, 1);
    Err(0x0502, "cube mip rejected");

    if (gFails) return 1;
    printf("test_copy_image_cube PASS\n");
    return 0;
}
