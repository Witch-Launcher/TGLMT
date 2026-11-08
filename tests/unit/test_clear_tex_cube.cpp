// test_clear_tex_cube.cpp — glClearTexSubImage/glClearTexImage trên cube map:
// zoffset là face (0..5), depth phải 1; clear face 2 không được đụng face khác.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_clear_tex_cube FAIL: %s\n", msg); ++gFails; }
}
static unsigned char P(int x, int y, int face, int c) {
    return (unsigned char)(17 + x * 5 + y * 19 + face * 37 + c * 71);
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    const int S = 3;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x8513 /*TEXTURE_CUBE_MAP*/, tex);
    for (int f = 0; f < 6; ++f) {
        std::vector<uint8_t> px((size_t)S * S * 4);
        for (int y = 0; y < S; ++y)
            for (int x = 0; x < S; ++x)
                for (int c = 0; c < 4; ++c)
                    px[((size_t)y * S + x) * 4 + c] = P(x, y, f, c);
        glTexImage2D(0x8515 + f, 0, 0x8058, S, S, 0, 0x1908, 0x1401, px.data());
    }
    Check(glGetError() == 0, "setup cube faces");
    auto& t = ctx.textures[tex];
    Check(t.isCube && t.faces[0].size() == (size_t)S * S * 4, "cube shadow 6 faces");

    const unsigned char red[4] = {255, 0, 0, 255};
    glClearTexSubImage(tex, 0, 1, 1, 2, 1, 1, 1, 0x1908, 0x1401, red);
    Check(glGetError() == 0, "clear cube face 2 no error");
    for (int f = 0; f < 6; ++f) {
        for (int y = 0; y < S; ++y)
            for (int x = 0; x < S; ++x)
                for (int c = 0; c < 4; ++c) {
                    const uint8_t got = t.faces[f][((size_t)y * S + x) * 4 + c];
                    const bool hit = (f == 2 && x == 1 && y == 1);
                    const uint8_t want = hit ? red[c] : P(x, y, f, c);
                    if (got != want) {
                        printf("test_clear_tex_cube FAIL: face %d (%d,%d) c%d = %u != %u\n",
                               f, x, y, c, got, want);
                        ++gFails;
                    }
                }
    }
    // face 0 mirror phải giữ nguyên (không bị clear nhầm).
    Check(t.pixels == t.faces[0], "mirror face 0 vẫn khớp");

    // z ngoài 0..5 → INVALID_VALUE; depth != 1 → INVALID_OPERATION.
    glClearTexSubImage(tex, 0, 0, 0, 6, 1, 1, 1, 0x1908, 0x1401, red);
    Check(glGetError() == 0x0501, "z=6 ngoài cube face → INVALID_VALUE");
    glClearTexSubImage(tex, 0, 0, 0, 2, 1, 1, 2, 0x1908, 0x1401, red);
    Check(glGetError() == 0x0502, "depth=2 cho cube face → INVALID_OPERATION");

    // ClearTexImage phải clear cả 6 face.
    glClearTexImage(tex, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "glClearTexImage cube no error");
    bool allZero = true;
    for (int f = 0; f < 6; ++f)
        for (uint8_t v : t.faces[f]) if (v != 0) allZero = false;
    Check(allZero, "glClearTexImage clear cả 6 face");

    glDeleteTextures(1, &tex);
    if (gFails) return 1;
    printf("test_clear_tex_cube PASS\n");
    return 0;
}
