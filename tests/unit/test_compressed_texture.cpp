// test_compressed_texture.cpp — Compressed texture thật (GL 4.6 §8.7 + §8.11).
// Trước đây TGLMT trả GL_INVALID_ENUM cho mọi lệnh nén (stub) và
// GL_NUM_COMPRESSED_TEXTURE_FORMATS = 0. Nay các format BC/ETC2/EAC/RGTC/BPTC/
// ASTC được nhận thật: upload block nén, glGetCompressedTexImage trả lại đúng
// byte gốc, query trả danh sách format hỗ trợ.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_compressed_texture FAIL: %s\n", msg);
        ++gFails;
    }
}
// Đọi hết queue lỗi, so 1 lỗi đầu với lỗi spec mong đợi.
static void Expect(const char* name, GLenum expected) {
    GLenum got = glGetError();
    if (got != expected) {
        printf("test_compressed_texture FAIL: %s got=0x%x expected=0x%x\n", name, got, expected);
        ++gFails;
    }
    while (glGetError() != 0) {}
}

// Số byte của 1 ảnh nén (byte mỗi block × số block), theo bảng format.
static size_t CompressedBytes(GLsizei w, GLsizei h, size_t blockBytes, size_t bw, size_t bh) {
    size_t blocks = (size_t)((w + (GLsizei)bw - 1) / (GLsizei)bw) *
                    (size_t)((h + (GLsizei)bh - 1) / (GLsizei)bh);
    return blocks * blockBytes;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- Query danh sách format nén (spec §8.5) ---
    GLint n = 0;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
    Check(n > 0, "GL_NUM_COMPRESSED_TEXTURE_FORMATS > 0");
    std::vector<GLint> fmts((size_t)(n > 0 ? n : 1), -1);
    if (n > 0) glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, fmts.data());
    // Format đầu tiên theo Table 8.14 (GL 4.6 core) là COMPRESSED_RED_RGTC1.
    Check(fmts[0] == (GLint)GL_COMPRESSED_RED_RGTC1,
          "GL_COMPRESSED_TEXTURE_FORMATS[0] = COMPRESSED_RED_RGTC1");
    bool allValid = true;
    for (GLint i = 0; i < n; ++i)
        if (fmts[(size_t)i] == 0) allValid = false;
    Check(allValid, "mọi format trong danh sách khác 0");
    // Danh sách phải chứa đúng format ta dùng ở test.
    bool hasBptc = false, hasEtc2 = false, hasR11 = false, hasDxt1 = false;
    for (GLint i = 0; i < n; ++i) {
        if (fmts[(size_t)i] == (GLint)GL_COMPRESSED_RGBA_BPTC_UNORM) hasBptc = true;
        if (fmts[(size_t)i] == (GLint)GL_COMPRESSED_RGB8_ETC2) hasEtc2 = true;
        if (fmts[(size_t)i] == (GLint)GL_COMPRESSED_R11_EAC) hasR11 = true;
        // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1 (gl.xml; không có trong
        // gl46_types.h vì S3TC đã bị gỡ khỏi core 4.6).
        if (fmts[(size_t)i] == 0x83F1) hasDxt1 = true;
    }
    Check(hasBptc, "danh sách có COMPRESSED_RGBA_BPTC_UNORM");
    Check(hasEtc2, "danh sách có COMPRESSED_RGB8_ETC2");
    Check(hasR11, "danh sách có COMPRESSED_R11_EAC");
    Check(hasDxt1, "danh sách có COMPRESSED_RGBA_S3TC_DXT1_EXT");

    // --- Upload ảnh nén BPTC (BC7): block 4x4, 16 byte ---
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    const GLsizei W = 8, H = 8;
    size_t bptcSize = CompressedBytes(W, H, 16, 4, 4);   // 2×2 block × 16B
    Check(bptcSize == 64, "kích thước ảnh BC7 8x8 = 64 byte");
    std::vector<uint8_t> bptc(bptcSize);
    for (size_t i = 0; i < bptc.size(); ++i) bptc[i] = (uint8_t)(i * 7 + 3);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 0,
                           (GLsizei)bptc.size(), bptc.data());
    Expect("glCompressedTexImage2D(BC7)", GL_NO_ERROR);

    // Get lại phải trả ĐÚNG byte gốc (spec §8.7: dữ liệu trả về nạp lại được).
    std::vector<uint8_t> got(bptcSize + 16, 0xCD);
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, got.data());
    Expect("glGetCompressedTexImage(BC7)", GL_NO_ERROR);
    Check(memcmp(got.data(), bptc.data(), bptcSize) == 0,
          "glGetCompressedTexImage trả đúng byte nén");
    Check(got[bptcSize] == 0xCD, "glGetCompressedTexImage không ghi dư");

    // Query kích thước ảnh nén của level (GL_TEXTURE_COMPRESSED_IMAGE_SIZE).
    GLint imgSize = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, 0x86A0 /*TEXTURE_COMPRESSED_IMAGE_SIZE*/,
                             &imgSize);
    Check(imgSize == (GLint)bptcSize, "TEXTURE_COMPRESSED_IMAGE_SIZE = 64");

    // --- ETC2 RGB8: block 4x4, 8 byte (kích thước block khác) ---
    GLuint texEtc = 0;
    glGenTextures(1, &texEtc);
    glBindTexture(GL_TEXTURE_2D, texEtc);
    size_t etcSize = CompressedBytes(4, 4, 8, 4, 4);   // 1 block × 8B
    Check(etcSize == 8, "kích thước ảnh ETC2 4x4 = 8 byte");
    std::vector<uint8_t> etc(etcSize);
    for (size_t i = 0; i < etc.size(); ++i) etc[i] = (uint8_t)(0xF0 - i);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB8_ETC2, 4, 4, 0,
                           (GLsizei)etc.size(), etc.data());
    Expect("glCompressedTexImage2D(ETC2)", GL_NO_ERROR);
    std::vector<uint8_t> gotEtc(etcSize, 0);
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, gotEtc.data());
    Expect("glGetCompressedTexImage(ETC2)", GL_NO_ERROR);
    Check(memcmp(gotEtc.data(), etc.data(), etcSize) == 0,
          "ETC2: glGetCompressedTexImage trả đúng byte nén");

    // --- R11 EAC (1 channel, 8 byte/block) ---
    GLuint texR11 = 0;
    glGenTextures(1, &texR11);
    glBindTexture(GL_TEXTURE_2D, texR11);
    std::vector<uint8_t> r11(8, 0x5A);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_R11_EAC, 4, 4, 0, 8, r11.data());
    Expect("glCompressedTexImage2D(R11_EAC)", GL_NO_ERROR);
    std::vector<uint8_t> gotR11(8, 0);
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, gotR11.data());
    Expect("glGetCompressedTexImage(R11_EAC)", GL_NO_ERROR);
    Check(memcmp(gotR11.data(), r11.data(), 8) == 0, "R11_EAC trả đúng byte nén");

    // --- Format KHÔNG hỗ trợ → GL_INVALID_ENUM (spec §8.7) ---
    glBindTexture(GL_TEXTURE_2D, tex);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, W, H, 0, (GLsizei)bptcSize,
                           bptc.data());
    Expect("generic compressed format", GL_INVALID_ENUM);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, 0x1234 /*enum lạ*/, W, H, 0, (GLsizei)bptcSize,
                           bptc.data());
    Expect("format nén lạ", GL_INVALID_ENUM);
    // Target không hợp lệ với ảnh nén.
    glCompressedTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 0,
                           (GLsizei)bptcSize, bptc.data());
    Expect("target rectangle", GL_INVALID_ENUM);

    // --- border ≠ 0 → GL_INVALID_VALUE (spec §8.5) ---
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 1,
                           (GLsizei)bptcSize, bptc.data());
    Expect("border != 0", GL_INVALID_VALUE);
    // --- imageSize sai → GL_INVALID_VALUE (spec §8.7) ---
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 0,
                           (GLsizei)bptcSize + 8, bptc.data());
    Expect("imageSize không khớp", GL_INVALID_VALUE);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 0,
                           (GLsizei)bptcSize - 8, bptc.data());
    Expect("imageSize thiếu", GL_INVALID_VALUE);
    // --- kích thước âm → GL_INVALID_VALUE ---
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, -4, H, 0,
                           (GLsizei)bptcSize, bptc.data());
    Expect("width âm", GL_INVALID_VALUE);

    // --- Đường DSA: glCompressedTextureSubImage2D + glGetCompressedTextureImage ---
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, W, H, 0,
                           (GLsizei)bptcSize, bptc.data());
    Expect("nạp lại BC7", GL_NO_ERROR);
    std::vector<uint8_t> block(16);
    for (size_t i = 0; i < block.size(); ++i) block[i] = (uint8_t)(0xA0 + i);
    glCompressedTextureSubImage2D(tex, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA_BPTC_UNORM,
                                  (GLsizei)block.size(), block.data());
    Expect("glCompressedTextureSubImage2D", GL_NO_ERROR);
    std::vector<uint8_t> gotDsa(bptcSize, 0);
    glGetCompressedTextureImage(tex, 0, (GLsizei)gotDsa.size(), gotDsa.data());
    Expect("glGetCompressedTextureImage", GL_NO_ERROR);
    Check(memcmp(gotDsa.data(), block.data(), block.size()) == 0,
          "DSA: block đầu đã bị thay bằng dữ liệu mới");
    Check(memcmp(gotDsa.data() + block.size(), bptc.data() + block.size(),
                 bptcSize - block.size()) == 0,
          "DSA: các block còn lại giữ nguyên");
    // bufSize nhỏ hơn dữ liệu → INVALID_OPERATION (spec §8.11).
    glGetCompressedTextureImage(tex, 0, 8, gotDsa.data());
    Expect("bufSize quá nhỏ", GL_INVALID_OPERATION);
    // Format của lệnh sub-image phải khớp format texture.
    glCompressedTextureSubImage2D(tex, 0, 0, 0, 4, 4, GL_COMPRESSED_RGB8_ETC2, 8,
                                  block.data());
    Expect("format không khớp texture", GL_INVALID_OPERATION);
    // Format generic → INVALID_ENUM.
    glCompressedTextureSubImage2D(tex, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA, 8, block.data());
    Expect("DSA format generic", GL_INVALID_ENUM);
    // Texture không tồn tại → INVALID_OPERATION.
    glCompressedTextureSubImage2D(9999, 0, 0, 0, 4, 4, GL_COMPRESSED_RGBA_BPTC_UNORM, 16,
                                  block.data());
    Expect("DSA texture không tồn tại", GL_INVALID_OPERATION);

    // --- Đọc ảnh nén trên texture KHÔNG nén → INVALID_OPERATION (spec §8.11) ---
    GLuint plain = 0;
    glGenTextures(1, &plain);
    glBindTexture(GL_TEXTURE_2D, plain);
    std::vector<uint8_t> rgba(4 * 4 * 4, 0x11);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    Expect("glTexImage2D thường", GL_NO_ERROR);
    glGetCompressedTexImage(GL_TEXTURE_2D, 0, gotDsa.data());
    Expect("đọc compressed trên texture thường", GL_INVALID_OPERATION);
    glGetnCompressedTexImage(GL_TEXTURE_2D, 0, (GLsizei)gotDsa.size(), gotDsa.data());
    Expect("glGetnCompressedTexImage trên texture thường", GL_INVALID_OPERATION);
    glGetCompressedTextureImage(plain, 0, (GLsizei)gotDsa.size(), gotDsa.data());
    Expect("DSA đọc compressed trên texture thường", GL_INVALID_OPERATION);
    glGetCompressedTextureSubImage(plain, 0, 0, 0, 0, 4, 4, 1, (GLsizei)gotDsa.size(),
                                   gotDsa.data());
    Expect("DSA sub đọc compressed trên texture thường", GL_INVALID_OPERATION);

    // --- Ảnh nén 1D: GL không định nghĩa format nén 1D → INVALID_ENUM (spec §8.7) ---
    glCompressedTexImage1D(GL_TEXTURE_1D, 0, GL_COMPRESSED_RGBA_BPTC_UNORM, 4, 0, 16,
                           block.data());
    Expect("glCompressedTexImage1D", GL_INVALID_ENUM);
    glCompressedTextureSubImage1D(tex, 0, 0, 4, GL_COMPRESSED_RGBA_BPTC_UNORM, 16,
                                  block.data());
    Expect("glCompressedTextureSubImage1D", GL_INVALID_ENUM);

    // --- Ảnh nén 3D: 2 slice, mỗi slice 1 block ETC2 (8 byte) ---
    GLuint tex3d = 0;
    glGenTextures(1, &tex3d);
    glBindTexture(GL_TEXTURE_3D, tex3d);
    std::vector<uint8_t> vol(16, 0);
    for (size_t i = 0; i < vol.size(); ++i) vol[i] = (uint8_t)(i + 1);
    glCompressedTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGB8_ETC2, 4, 4, 2, 0, 16,
                           vol.data());
    Expect("glCompressedTexImage3D", GL_NO_ERROR);
    std::vector<uint8_t> gotVol(16, 0);
    glGetCompressedTexImage(GL_TEXTURE_3D, 0, gotVol.data());
    Expect("glGetCompressedTexImage(3D)", GL_NO_ERROR);
    Check(memcmp(gotVol.data(), vol.data(), vol.size()) == 0,
          "3D: glGetCompressedTexImage trả đúng byte nén");
    // Sub-image 3D: thay 1 block trong slice 1.
    std::vector<uint8_t> volBlock(8, 0xEE);
    glCompressedTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 1, 4, 4, 1, GL_COMPRESSED_RGB8_ETC2, 8,
                              volBlock.data());
    Expect("glCompressedTexSubImage3D", GL_NO_ERROR);
    glGetCompressedTexImage(GL_TEXTURE_3D, 0, gotVol.data());
    Expect("glGetCompressedTexImage(3D) sau sub", GL_NO_ERROR);
    Check(memcmp(gotVol.data() + 8, volBlock.data(), 8) == 0,
          "3D: sub-image thay đúng block trong slice 1");
    Check(memcmp(gotVol.data(), vol.data(), 8) == 0, "3D: slice 0 giữ nguyên");

    // --- glGetIntegeri_v cho danh sách format nén ---
    GLint one = -1;
    glGetIntegeri_v(GL_COMPRESSED_TEXTURE_FORMATS, 0, &one);
    Check(one == (GLint)GL_COMPRESSED_RED_RGTC1,
          "glGetIntegeri_v(COMPRESSED_TEXTURE_FORMATS, 0)");

    if (gFails) return 1;
    printf("test_compressed_texture PASS\n");
    return 0;
}
