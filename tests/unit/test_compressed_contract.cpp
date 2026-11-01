// test_compressed_contract.cpp — TGLMT supports no compressed texture format
// (GL_NUM_COMPRESSED_TEXTURE_FORMATS == 0). The API must reject compressed
// uploads with INVALID_ENUM and compressed reads with INVALID_OPERATION instead
// of silently accepting them and leaving an empty texture.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Expect(const char* name, GLenum expected) {
    GLenum got = glGetError();
    if (got != expected) {
        printf("test_compressed_contract FAIL: %s got=0x%x expected=0x%x\n", name, got, expected);
        ++gFails;
    }
    while (glGetError() != 0) {}
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLint n = -1;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
    if (n != 0) {
        printf("test_compressed_contract FAIL: formats=%d expected=0\n", n);
        ++gFails;
    }

    unsigned char data[64] = {};
    const GLuint tex = 1;

    glCompressedTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGBA, 4, 0, 64, data);
    Expect("CompressedTexImage1D", GL_INVALID_ENUM);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, 4, 4, 0, 64, data);
    Expect("CompressedTexImage2D", GL_INVALID_ENUM);
    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA, 4, 4, 4, 0, 64, data);
    Expect("CompressedTexImage3D", GL_INVALID_ENUM);

    glCompressedTexSubImage1D(GL_TEXTURE_1D, 0, 0, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTexSubImage1D", GL_INVALID_ENUM);
    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTexSubImage2D", GL_INVALID_ENUM);
    glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, 4, 4, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTexSubImage3D", GL_INVALID_ENUM);

    glCompressedTextureSubImage1D(tex, 0, 0, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTextureSubImage1D", GL_INVALID_ENUM);
    glCompressedTextureSubImage2D(tex, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTextureSubImage2D", GL_INVALID_ENUM);
    glCompressedTextureSubImage3D(tex, 0, 0, 0, 0, 4, 4, 4, GL_COMPRESSED_RGBA, 64, data);
    Expect("CompressedTextureSubImage3D", GL_INVALID_ENUM);

    glGetCompressedTexImage(GL_TEXTURE_2D, 0, data);
    Expect("GetCompressedTexImage", GL_INVALID_OPERATION);
    glGetCompressedTextureImage(tex, 0, 64, data);
    Expect("GetCompressedTextureImage", GL_INVALID_OPERATION);
    glGetCompressedTextureSubImage(tex, 0, 0, 0, 0, 4, 4, 4, 64, data);
    Expect("GetCompressedTextureSubImage", GL_INVALID_OPERATION);
    glGetnCompressedTexImage(GL_TEXTURE_2D, 0, 64, data);
    Expect("GetnCompressedTexImage", GL_INVALID_OPERATION);

    if (gFails) return 1;
    printf("test_compressed_contract PASS\n");
    return 0;
}
