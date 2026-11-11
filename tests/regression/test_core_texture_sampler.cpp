// Test nhóm Texture/Sampler của GL 4.6 Core — 25 entry point:
//   glIsTexture, glPixelStoref,
//   glTexParameter{f,fv,iv,Iiv,Iuiv}, glTextureParameter{f,fv,iv,Iiv,Iuiv},
//   glTexStorage1D, glTextureStorage1D, glTextureStorage3D,
//   glTexSubImage1D, glTextureSubImage3D,
//   glCopyTextureSubImage2D, glGenerateTextureMipmap,
//   glCreateSamplers, glIsSampler, glBindSamplers,
//   glSamplerParameter{f,fv,iv,Iiv,Iuiv}.
//
// Kiểm chứng theo GL 4.6 §8.1–§8.3:
//   • glTexParameter* và glTextureParameter* phải CÓ GIÁ TRỊ GIỐNG NHAU
//     (bản DSA trước đây ép float→int ⇒ mất độ chính xác, mất swizzle).
//   • Con trỏ NULL → INVALID_VALUE (trước đây deref ⇒ CRASH).
//   • GL_TEXTURE_SWIZZLE_RGBA (4 phần tử) phải tách đúng 4 swizzle.
//   • GL_TEXTURE_BORDER_COLOR (float[4]) phải giữ cả 4 thành phần.
//   • glTexStorage1D: levels<1 hoặc levels vượt log2(w)+1 → INVALID_VALUE.
//   • glBindSamplers: count âm → INVALID_VALUE; vượt số texture unit →
//     INVALID_OPERATION.

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }

// Đọc lại giá trị texture param (int) qua glGetTexParameteriv.
// Getter DSA (theo texture id) — glGetTexParameteriv/fv nhận TARGET, không
// nhận id, nên phải dùng glGetTextureParameteriv/fv.
GLint TexParamInt(GLuint tex, GLenum pname) {
    GLint v = -12345;
    glGetTextureParameteriv(tex, pname, &v);
    return v;
}
GLfloat TexParamFloat(GLuint tex, GLenum pname) {
    GLfloat v = -12345.0f;
    glGetTextureParameterfv(tex, pname, &v);
    return v;
}

// ===========================================================================
// glIsTexture + glPixelStoref
// ===========================================================================
bool TestIsTextureAndPixelStore() {
    GLuint t = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &t);
    TGLMT_CHECK(glIsTexture(t) == GL_TRUE, "glIsTexture phải TRUE với texture vừa tạo");
    TGLMT_CHECK(glIsTexture(999999) == GL_FALSE, "glIsTexture phải FALSE với id không tồn tại");
    TGLMT_CHECK(glIsTexture(0) == GL_FALSE, "glIsTexture(0) phải FALSE");

    // glPixelStoref: GL 4.6 Core KHÔNG có glGetPixelStorei/glGetPixelStorefv
    // (spec §8.1 và Khronos registry đều không liệt kê → catalog 657 là đúng),
    // nên chỉ kiểm được phía lỗi.
    DrainErrors();
    glPixelStoref(GL_UNPACK_ALIGNMENT, 4.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelStoref hợp lệ không lỗi");
    glPixelStoref(GL_UNPACK_ALIGNMENT, 1.0f);
    // pname không hợp lệ → INVALID_ENUM.
    DrainErrors();
    glPixelStoref(0xDEAD, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glPixelStoref với pname sai → INVALID_ENUM");

    glDeleteTextures(1, &t);
    return true;
}

// ===========================================================================
// glTexParameter* vs glTextureParameter* (10 hàm)
// ===========================================================================
bool TestTextureParameters() {
    GLuint tb = 0, td = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tb);
    glCreateTextures(GL_TEXTURE_2D, 1, &td);
    glBindTexture(GL_TEXTURE_2D, tb);

    // --- int param ---
    DrainErrors();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexParameteri hợp lệ không lỗi");
    TGLMT_CHECK(TexParamInt(tb, GL_TEXTURE_MIN_FILTER) == GL_NEAREST,
                "glTexParameteri phải lưu đúng giá trị");

    // Bản DSA phải cho KẾT QUẢ GIỐNG HỆT bản bound.
    DrainErrors();
    glTextureParameteri(td, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureParameteri hợp lệ không lỗi");
    TGLMT_CHECK(TexParamInt(td, GL_TEXTURE_MIN_FILTER) == TexParamInt(tb, GL_TEXTURE_MIN_FILTER),
                "glTextureParameteri phải cho kết quả giống glTexParameteri");

    // --- float param: độ chính xác phải giữ (trước đây bản DSA ép về int) ---
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.5f);
    glTextureParameterf(td, GL_TEXTURE_LOD_BIAS, 0.5f);
    const GLfloat fb = TexParamFloat(tb, GL_TEXTURE_LOD_BIAS);
    const GLfloat fd = TexParamFloat(td, GL_TEXTURE_LOD_BIAS);
    TGLMT_CHECK(fb == 0.5f, "glTexParameterf phải giữ độ chính xác float");
    TGLMT_CHECK(fd == 0.5f,
                "glTextureParameterf phải giữ độ chính xác float (trước đây ép về int → 0)");

    // --- vector param: BORDER_COLOR float[4] ---
    const GLfloat border[4] = {0.25f, 0.5f, 0.75f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTextureParameterfv(td, GL_TEXTURE_BORDER_COLOR, border);
    {
        GLfloat got[4] = {0};
        glGetTextureParameterfv(tb, GL_TEXTURE_BORDER_COLOR, got);
        TGLMT_CHECK(got[0] == 0.25f && got[3] == 1.0f,
                    "glTexParameterfv BORDER_COLOR phải giữ cả 4 thành phần");
        GLfloat gotd[4] = {0};
        glGetTextureParameterfv(td, GL_TEXTURE_BORDER_COLOR, gotd);
        TGLMT_CHECK(gotd[0] == 0.25f && gotd[3] == 1.0f,
                    "glTextureParameterfv BORDER_COLOR phải giữ cả 4 thành phần");
    }

    // --- swizzle RGBA (4 phần tử) ---
    const GLint swz[4] = {GL_GREEN, GL_BLUE, GL_ALPHA, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swz);
    glTextureParameteriv(td, GL_TEXTURE_SWIZZLE_RGBA, swz);
    TGLMT_CHECK(TexParamInt(tb, GL_TEXTURE_SWIZZLE_R) == GL_GREEN,
                "glTexParameteriv SWIZZLE_RGBA phải tách đúng 4 swizzle");
    TGLMT_CHECK(TexParamInt(tb, GL_TEXTURE_SWIZZLE_A) == GL_RED,
                "glTexParameteriv SWIZZLE_RGBA phải tách đúng 4 swizzle");
    TGLMT_CHECK(TexParamInt(td, GL_TEXTURE_SWIZZLE_R) == TexParamInt(tb, GL_TEXTURE_SWIZZLE_R),
                "glTextureParameteriv SWIZZLE_RGBA phải khớp bản bound");
    TGLMT_CHECK(TexParamInt(td, GL_TEXTURE_SWIZZLE_A) == TexParamInt(tb, GL_TEXTURE_SWIZZLE_A),
                "glTextureParameteriv SWIZZLE_RGBA phải khớp bản bound");

    // --- Iiv / Iuiv ---
    glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (const GLint[]){7});
    glTextureParameterIiv(td, GL_TEXTURE_MAX_LEVEL, (const GLint[]){7});
    TGLMT_CHECK(TexParamInt(tb, GL_TEXTURE_MAX_LEVEL) == 7, "glTexParameterIiv phải lưu đúng");
    TGLMT_CHECK(TexParamInt(td, GL_TEXTURE_MAX_LEVEL) == 7, "glTextureParameterIiv phải lưu đúng");
    glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (const GLuint[]){5});
    glTextureParameterIuiv(td, GL_TEXTURE_MAX_LEVEL, (const GLuint[]){5});
    TGLMT_CHECK(TexParamInt(tb, GL_TEXTURE_MAX_LEVEL) == 5, "glTexParameterIuiv phải lưu đúng");
    TGLMT_CHECK(TexParamInt(td, GL_TEXTURE_MAX_LEVEL) == 5, "glTextureParameterIuiv phải lưu đúng");

    // --- NULL pointer → INVALID_VALUE (trước đây CRASH) ---
    DrainErrors();
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTexParameteriv với con trỏ NULL → INVALID_VALUE (không được crash)");
    DrainErrors();
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTexParameterfv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTexParameterIiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTexParameterIiv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTexParameterIuiv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTexParameterIuiv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTextureParameteriv(td, GL_TEXTURE_MIN_FILTER, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureParameteriv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTextureParameterfv(td, GL_TEXTURE_MIN_FILTER, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureParameterfv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTextureParameterIiv(td, GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureParameterIiv với con trỏ NULL → INVALID_VALUE");
    DrainErrors();
    glTextureParameterIuiv(td, GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureParameterIuiv với con trỏ NULL → INVALID_VALUE");

    // Texture không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glTextureParameteri(999999, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glTextureParameteri với texture không tồn tại → INVALID_OPERATION");

    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tb);
    glDeleteTextures(1, &td);
    return true;
}

// ===========================================================================
// Storage: glTexStorage1D / glTextureStorage1D / glTextureStorage3D
// ===========================================================================
bool TestTextureStorage() {
    // --- 1D ---
    GLuint t1 = 0;
    glCreateTextures(GL_TEXTURE_1D, 1, &t1);
    DrainErrors();
    glTextureStorage1D(t1, 1, GL_RGBA8, 16);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureStorage1D hợp lệ không lỗi");
    GLint w = 0;
    glGetTextureLevelParameteriv(t1, 0, GL_TEXTURE_WIDTH, &w);
    TGLMT_CHECK(w == 16, "glTextureStorage1D phải đặt kích thước đúng");

    // levels < 1 → INVALID_VALUE.
    GLuint t2 = 0;
    glCreateTextures(GL_TEXTURE_1D, 1, &t2);
    DrainErrors();
    glTextureStorage1D(t2, 0, GL_RGBA8, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureStorage1D levels<1 → INVALID_VALUE");
    // levels vượt log2(w)+1 → INVALID_VALUE (w=16 → tối đa 5 mip).
    DrainErrors();
    glTextureStorage1D(t2, 99, GL_RGBA8, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureStorage1D levels vượt giới hạn → INVALID_VALUE");

    // Bản bound (glTexStorage1D) qua texture đang bind.
    glBindTexture(GL_TEXTURE_1D, t2);
    DrainErrors();
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGBA8, 8);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexStorage1D hợp lệ không lỗi");
    glGetTextureLevelParameteriv(t2, 0, GL_TEXTURE_WIDTH, &w);
    TGLMT_CHECK(w == 8, "glTexStorage1D phải đặt kích thước đúng");

    // --- 3D ---
    GLuint t3 = 0;
    glCreateTextures(GL_TEXTURE_3D, 1, &t3);
    DrainErrors();
    glTextureStorage3D(t3, 1, GL_RGBA8, 4, 4, 4);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureStorage3D hợp lệ không lỗi");
    GLint d = 0;
    glGetTextureLevelParameteriv(t3, 0, GL_TEXTURE_DEPTH, &d);
    TGLMT_CHECK(d == 4, "glTextureStorage3D phải đặt depth đúng");
    DrainErrors();
    glTextureStorage3D(t3, 0, GL_RGBA8, 4, 4, 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureStorage3D levels<1 → INVALID_VALUE");
    DrainErrors();
    glTextureStorage3D(t3, 1, GL_RGBA8, 0, 4, 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureStorage3D kích thước 0 → INVALID_VALUE");

    glBindTexture(GL_TEXTURE_1D, 0);
    glDeleteTextures(1, &t1);
    glDeleteTextures(1, &t2);
    glDeleteTextures(1, &t3);
    return true;
}

// ===========================================================================
// Sub-image + mipmap + copy
// ===========================================================================
bool TestTextureSubImageAndMipmap() {
    // --- glTexSubImage1D (bản bound) ---
    GLuint t = 0;
    glCreateTextures(GL_TEXTURE_1D, 1, &t);
    glBindTexture(GL_TEXTURE_1D, t);
    glTexStorage1D(GL_TEXTURE_1D, 1, GL_RGBA8, 8);
    const GLubyte px[8 * 4] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                               17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    DrainErrors();
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 8, GL_RGBA, GL_UNSIGNED_BYTE, px);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexSubImage1D hợp lệ không lỗi");
    // Vượt kích thước texture → INVALID_VALUE.
    DrainErrors();
    glTexSubImage1D(GL_TEXTURE_1D, 0, 4, 8, GL_RGBA, GL_UNSIGNED_BYTE, px);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTexSubImage1D vượt kích thước texture → INVALID_VALUE");
    glBindTexture(GL_TEXTURE_1D, 0);

    // --- glTextureSubImage3D (DSA) ---
    GLuint t3 = 0;
    glCreateTextures(GL_TEXTURE_3D, 1, &t3);
    glTextureStorage3D(t3, 1, GL_RGBA8, 4, 4, 2);
    const GLubyte px3[4 * 4 * 2 * 4] = {0};
    DrainErrors();
    glTextureSubImage3D(t3, 0, 0, 0, 0, 4, 4, 2, GL_RGBA, GL_UNSIGNED_BYTE, px3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTextureSubImage3D hợp lệ không lỗi");
    DrainErrors();
    glTextureSubImage3D(t3, 0, 0, 0, 0, 8, 8, 2, GL_RGBA, GL_UNSIGNED_BYTE, px3);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glTextureSubImage3D vượt kích thước → INVALID_VALUE");
    DrainErrors();
    glTextureSubImage3D(999999, 0, 0, 0, 0, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px3);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glTextureSubImage3D với texture không tồn tại → INVALID_OPERATION");

    // --- glGenerateTextureMipmap ---
    GLuint tm = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tm);
    glTextureStorage2D(tm, 3, GL_RGBA8, 16, 16);
    DrainErrors();
    glGenerateTextureMipmap(tm);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGenerateTextureMipmap không lỗi");
    DrainErrors();
    glGenerateTextureMipmap(999999);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGenerateTextureMipmap với texture không tồn tại → INVALID_OPERATION");

    // --- glCopyTextureSubImage2D ---
    GLuint tc = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &tc);
    glTextureStorage2D(tc, 1, GL_RGBA8, 8, 8);
    DrainErrors();
    glCopyTextureSubImage2D(tc, 0, 0, 0, 0, 0, 4, 4);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCopyTextureSubImage2D không lỗi");
    DrainErrors();
    glCopyTextureSubImage2D(999999, 0, 0, 0, 0, 0, 4, 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glCopyTextureSubImage2D với texture không tồn tại → INVALID_OPERATION");

    glDeleteTextures(1, &t);
    glDeleteTextures(1, &t3);
    glDeleteTextures(1, &tm);
    glDeleteTextures(1, &tc);
    return true;
}

// ===========================================================================
// Sampler: glCreateSamplers / glIsSampler / glBindSamplers / glSamplerParameter*
// ===========================================================================
bool TestSamplers() {
    GLuint s[2] = {0, 0};
    glCreateSamplers(2, s);
    TGLMT_CHECK(s[0] != 0 && s[1] != 0, "glCreateSamplers phải cấp id");
    TGLMT_CHECK(glIsSampler(s[0]) == GL_TRUE, "glIsSampler phải TRUE với sampler vừa tạo");
    TGLMT_CHECK(glIsSampler(999999) == GL_FALSE, "glIsSampler phải FALSE với id không tồn tại");

    // int param
    DrainErrors();
    glSamplerParameteri(s[0], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glSamplerParameteri hợp lệ không lỗi");
    GLint v = 0;
    glGetSamplerParameteriv(s[0], GL_TEXTURE_MIN_FILTER, &v);
    TGLMT_CHECK(v == GL_LINEAR, "glSamplerParameteri phải lưu đúng giá trị");

    // float param: phải đọc lại được (bản f trước đây chỉ lưu fparams còn
    // draw path đọc iparams ⇒ mất).
    glSamplerParameterf(s[0], GL_TEXTURE_MAX_ANISOTROPY, 8.0f);
    GLfloat fa = 0;
    glGetSamplerParameterfv(s[0], GL_TEXTURE_MAX_ANISOTROPY, &fa);
    TGLMT_CHECK(fa == 8.0f, "glSamplerParameterf phải lưu đúng giá trị float");

    // iv / fv / Iiv / Iuiv
    glSamplerParameteriv(s[0], GL_TEXTURE_MAG_FILTER, (const GLint[]){GL_NEAREST});
    glGetSamplerParameteriv(s[0], GL_TEXTURE_MAG_FILTER, &v);
    TGLMT_CHECK(v == GL_NEAREST, "glSamplerParameteriv phải lưu đúng");
    glSamplerParameterfv(s[0], GL_TEXTURE_MAX_ANISOTROPY, (const GLfloat[]){4.0f});
    glGetSamplerParameterfv(s[0], GL_TEXTURE_MAX_ANISOTROPY, &fa);
    TGLMT_CHECK(fa == 4.0f, "glSamplerParameterfv phải lưu đúng");
    glSamplerParameterIiv(s[0], GL_TEXTURE_MAX_LEVEL, (const GLint[]){3});
    glGetSamplerParameteriv(s[0], GL_TEXTURE_MAX_LEVEL, &v);
    TGLMT_CHECK(v == 3, "glSamplerParameterIiv phải lưu đúng");
    glSamplerParameterIuiv(s[0], GL_TEXTURE_MAX_LEVEL, (const GLuint[]){2});
    glGetSamplerParameteriv(s[0], GL_TEXTURE_MAX_LEVEL, &v);
    TGLMT_CHECK(v == 2, "glSamplerParameterIuiv phải lưu đúng");

    // NULL → INVALID_VALUE (trước đây CRASH).
    DrainErrors();
    glSamplerParameteriv(s[0], GL_TEXTURE_MIN_FILTER, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glSamplerParameteriv NULL → INVALID_VALUE");
    DrainErrors();
    glSamplerParameterfv(s[0], GL_TEXTURE_MAX_ANISOTROPY, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glSamplerParameterfv NULL → INVALID_VALUE");
    DrainErrors();
    glSamplerParameterIiv(s[0], GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glSamplerParameterIiv NULL → INVALID_VALUE");
    DrainErrors();
    glSamplerParameterIuiv(s[0], GL_TEXTURE_MAX_LEVEL, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glSamplerParameterIuiv NULL → INVALID_VALUE");

    // Sampler không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glSamplerParameteri(999999, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glSamplerParameteri với sampler không tồn tại → INVALID_OPERATION");

    // glBindSamplers
    DrainErrors();
    glBindSamplers(0, 2, s);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBindSamplers hợp lệ không lỗi");
    GLint bound = 0;
    glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &bound);
    TGLMT_CHECK((GLuint)bound == s[0], "glBindSamplers phải bind sampler vào unit 0");
    glGetIntegeri_v(GL_SAMPLER_BINDING, 1, &bound);
    TGLMT_CHECK((GLuint)bound == s[1], "glBindSamplers phải bind sampler vào unit 1");

    // count âm → INVALID_VALUE.
    DrainErrors();
    glBindSamplers(0, -1, s);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glBindSamplers count<0 → INVALID_VALUE");
    // first+count vượt số texture unit → INVALID_OPERATION.
    DrainErrors();
    glBindSamplers(30, 100, s);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glBindSamplers vượt số texture unit → INVALID_OPERATION");

    glBindSamplers(0, 2, nullptr);
    glDeleteSamplers(2, s);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core texture/sampler entry points (GL 4.6 §8.1–§8.3) ===\n");
    static tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    const Case kCases[] = {
        {"glIsTexture + glPixelStoref", TestIsTextureAndPixelStore},
        {"glTexParameter* / glTextureParameter*", TestTextureParameters},
        {"glTexStorage1D / glTextureStorage1D / glTextureStorage3D", TestTextureStorage},
        {"glTexSubImage1D / glTextureSubImage3D / mipmap / copy", TestTextureSubImageAndMipmap},
        {"sampler: create/bind/param", TestSamplers},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
