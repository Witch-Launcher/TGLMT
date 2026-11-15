// gl_legacy_typevariants.cpp — Tier A: các entry point immediate-mode với
// tham số kiểu SỐ NGUYÊN (short / int / byte / ubyte / ushort / uint).
//
// Spec: OpenGL 2.1 §2.7 "Vertex Specification" (glspec21.pdf PDF p.34–35),
// §2.14 "Colors and Coloring" + Table 2.9 (glspec21.pdf PDF p.73 = trang in
// 59). Văn bản tương đương giữ nguyên trong OpenGL 4.2 Compatibility Profile
// §2.7 (glspec42.compatibility.pdf PDF p.52–53) và §2.1.2 (PDF p.31).
//
// Table 2.9 (GL 2.1 spec PDF p.59) — chuyển thành phần nguyên → float:
//
//   GL Type   Conversion
//   ubyte     c / (2^8  - 1)
//   byte      (2c + 1) / (2^8  - 1)
//   ushort    c / (2^16 - 1)
//   short     (2c + 1) / (2^16 - 1)
//   uint      c / (2^32 - 1)
//   int       (2c + 1) / (2^32 - 1)
//   float     c
//   double    c
//
//   "Table 2.9: Component conversions. Color, normal, and depth components,
//    (c), are converted to an internal floating-point representation, (f),
//    using the equations in this table."
//
// ÁP DỤNG:
//  • Color / SecondaryColor  — spec §2.14: "the conversion of integer color
//    components (R, G, B, and A) to floating-point values is discussed in
//    section 2.14" ⇒ Table 2.9 (ký hiệu UNSIGNED cho ubyte/ushort/uint,
//    SIGNED cho byte/short/int).
//  • Normal — spec §2.7: "Byte, short, or integer values passed to Normal
//    are converted to floating-point values as indicated for the corresponding
//    (signed) type in table 2.9."
//  • Vertex / TexCoord / MultiTexCoord — cùng bảng Table 2.9 (nhóm "signed
//    / unsigned component" của spec §2.7 §2.1.2 trong GL 4.2C).
//
// MỌI biến thể mới ở đây chỉ làm đúng MỘT việc: chuyển kiểu rồi gọi lại
// biến thể float/double đã có sẵn (glVertex3f, glColor3f, glNormal3f,
// glTexCoord3f, glMultiTexCoord3f, glSecondaryColor3f). Nhờ vậy hành vi
// (ghi state, kiểm tra Begin/End, kiểm tra target texture, xử lý con trỏ
// NULL) GIỐNG HỆT biến thể float — chỉ khác đúng phép chuyển đổi kiểu.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"

using namespace tglmt;

namespace tglmt::gl {
namespace {

// ---------------------------------------------------------------------------
// Table 2.9 — chuyển thành phần nguyên sang float.
// Dùng số thực double ở mọi phép tính để tránh tràn số nguyên (ví dụ
// 2*c+1 với c = INT_MIN = -2147483648 ⇒ -4294967295 vượt giới hạn int32).
// ---------------------------------------------------------------------------
constexpr double kUByteDen = 255.0;            // 2^8  - 1
constexpr double kUShortDen = 65535.0;          // 2^16 - 1
constexpr double kUIntDen = 4294967295.0;       // 2^32 - 1

// Signed: (2c + 1) / (2^n - 1)  — Table 2.9.
float FromSignedByte(GLbyte v) {
    return (float)((2.0 * (double)v + 1.0) / kUByteDen);
}
float FromSignedShort(GLshort v) {
    return (float)((2.0 * (double)v + 1.0) / kUShortDen);
}
float FromSignedInt(GLint v) {
    return (float)((2.0 * (double)v + 1.0) / kUIntDen);
}

// Unsigned: c / (2^n - 1)  — Table 2.9.
float FromUnsignedByte(GLubyte v) {
    return (float)((double)v / kUByteDen);
}
float FromUnsignedShort(GLushort v) {
    return (float)((double)v / kUShortDen);
}
float FromUnsignedInt(GLuint v) {
    return (float)((double)v / kUIntDen);
}

// ---------------------------------------------------------------------------
// Vertex / TexCoord — chuyển TRỰC TIẾP sang float, KHÔNG dùng Table 2.9.
//
// Spec GL 2.1 §2.14 (glspec21.pdf PDF p.73, in p.59): "Table 2.9: Component
// conversions. COLOR, NORMAL, and DEPTH components, (c), are converted to an
// internal floating-point representation using the equations in this table."
// ⇒ Table 2.9 KHÔNG áp cho toạ độ đỉnh (Vertex) hay toạ độ texture
// (TexCoord/MultiTexCoord): các lệnh đó chuyển thẳng sang float.
//
// (Lưu ý: GL 4.2C §2.1.2 eq (2.1)/(2.2) chỉ nói về fixed-point NORMALIZED
// cho generic vertex attributes + pixel component — cũng không áp cho
// glVertex/glTexCoord của Compatibility Profile.)
// ---------------------------------------------------------------------------
float FromDirectInt(GLint v) { return (float)v; }
float FromDirectShort(GLshort v) { return (float)v; }

// Con trỏ NULL: giữ ĐÚNG hành vi của biến thể float/double đã có
// (src/gl/gl_legacy_vertex.cpp) — cùng mã lỗi, cùng điều kiện.
// GHI CHÚ: mã lỗi hiện tại của cả nhóm là GL_INVALID_ENUM (0x0500) vì biến
// thể float/double đã có dùng vậy. Theo man page Khronos, con trỏ NULL phải
// sinh GL_INVALID_VALUE (0x0501). Sửa mã lỗi này phải sửa CẢ nhóm
// (gl_legacy_vertex.cpp) cho nhất quán — việc đó ngoài phạm vi file này.
inline void NullPointer() {
    Context::Current().errors.Record(GL_INVALID_ENUM);
}

}  // namespace

// ---------------------------------------------------------------------------
// Vertex — glVertex{234}{i,s}{,v}
// Spec §2.7: "A call to any Vertex command specifies four coordinates: x, y,
// z, and w."  Vertex2 ⇒ z = 0, w = 1; Vertex3 ⇒ w = 1 (do glVertex*f xử lý).
// ---------------------------------------------------------------------------
void glVertex2i(GLint x, GLint y) {
    glVertex2f(FromDirectInt(x), FromDirectInt(y));
}
void glVertex2iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glVertex2i(v[0], v[1]);
}
void glVertex2s(GLshort x, GLshort y) {
    glVertex2f(FromDirectShort(x), FromDirectShort(y));
}
void glVertex2sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glVertex2s(v[0], v[1]);
}

void glVertex3i(GLint x, GLint y, GLint z) {
    glVertex3f(FromDirectInt(x), FromDirectInt(y), FromDirectInt(z));
}
void glVertex3iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glVertex3i(v[0], v[1], v[2]);
}
void glVertex3s(GLshort x, GLshort y, GLshort z) {
    glVertex3f(FromDirectShort(x), FromDirectShort(y), FromDirectShort(z));
}
void glVertex3sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glVertex3s(v[0], v[1], v[2]);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w) {
    glVertex4f(FromDirectInt(x), FromDirectInt(y), FromDirectInt(z),
               FromDirectInt(w));
}
void glVertex4iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glVertex4i(v[0], v[1], v[2], v[3]);
}
void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    glVertex4f(FromDirectShort(x), FromDirectShort(y), FromDirectShort(z),
               FromDirectShort(w));
}
void glVertex4sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glVertex4s(v[0], v[1], v[2], v[3]);
}

// ---------------------------------------------------------------------------
// Color — KHÔNG có trong file này.
// glColor{34}{i,s}{,v} đã được implement trong src/gl/gl_legacy_vertex.cpp
// (nên không nằm trong danh sách stub). File đó dùng CHUNG đúng bộ phép
// chuyển Table 2.9 sau khi được sửa: ColorSignedByte / ColorSignedShort /
// ColorSignedInt / ColorUnsignedByte / ColorUnsignedShort /
// ColorUnsignedInt (src/gl/gl_legacy_vertex.cpp:264-281).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Normal — glNormal3{b,i,s}{,v}
// Spec §2.7: "Byte, short, or integer values passed to Normal are converted
// to floating-point values as indicated for the corresponding (signed) type in
// table 2.9."
// ---------------------------------------------------------------------------
void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) {
    glNormal3f(FromSignedByte(nx), FromSignedByte(ny), FromSignedByte(nz));
}
void glNormal3bv(const GLbyte* v) {
    if (!v) { NullPointer(); return; }
    glNormal3b(v[0], v[1], v[2]);
}
void glNormal3i(GLint nx, GLint ny, GLint nz) {
    glNormal3f(FromSignedInt(nx), FromSignedInt(ny), FromSignedInt(nz));
}
void glNormal3iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glNormal3i(v[0], v[1], v[2]);
}
void glNormal3s(GLshort nx, GLshort ny, GLshort nz) {
    glNormal3f(FromSignedShort(nx), FromSignedShort(ny), FromSignedShort(nz));
}
void glNormal3sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glNormal3s(v[0], v[1], v[2]);
}

// ---------------------------------------------------------------------------
// Texture coordinates — glTexCoord{1234}{i,s}{,v}
// Ghi vào texture unit ĐANG HOẠT ĐỘNG (giống hệt glTexCoord*f).
// TexCoord1 ⇒ t = r = 0, q = 1; TexCoord2 ⇒ r = 0, q = 1; TexCoord3 ⇒ q = 1.
// (Các giá trị mặc định do glTexCoord*f đặt sẵn.)
// ---------------------------------------------------------------------------
void glTexCoord1i(GLint s) { glTexCoord1f(FromDirectInt(s)); }
void glTexCoord1iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord1i(v[0]);
}
void glTexCoord1s(GLshort s) { glTexCoord1f(FromDirectShort(s)); }
void glTexCoord1sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord1s(v[0]);
}

void glTexCoord2i(GLint s, GLint t) {
    glTexCoord2f(FromDirectInt(s), FromDirectInt(t));
}
void glTexCoord2iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord2i(v[0], v[1]);
}
void glTexCoord2s(GLshort s, GLshort t) {
    glTexCoord2f(FromDirectShort(s), FromDirectShort(t));
}
void glTexCoord2sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord2s(v[0], v[1]);
}

void glTexCoord3i(GLint s, GLint t, GLint r) {
    glTexCoord3f(FromDirectInt(s), FromDirectInt(t), FromDirectInt(r));
}
void glTexCoord3iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord3i(v[0], v[1], v[2]);
}
void glTexCoord3s(GLshort s, GLshort t, GLshort r) {
    glTexCoord3f(FromDirectShort(s), FromDirectShort(t), FromDirectShort(r));
}
void glTexCoord3sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord3s(v[0], v[1], v[2]);
}

void glTexCoord4i(GLint s, GLint t, GLint r, GLint q) {
    glTexCoord4f(FromDirectInt(s), FromDirectInt(t), FromDirectInt(r),
                 FromDirectInt(q));
}
void glTexCoord4iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord4i(v[0], v[1], v[2], v[3]);
}
void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) {
    glTexCoord4f(FromDirectShort(s), FromDirectShort(t), FromDirectShort(r),
                 FromDirectShort(q));
}
void glTexCoord4sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glTexCoord4s(v[0], v[1], v[2], v[3]);
}

// ---------------------------------------------------------------------------
// Multi-texture coordinates — glMultiTexCoord{1234}{i,s}{,v}
// Ghi vào unit của TARGET, KHÔNG đổi unit đang hoạt động (spec §2.7:
// "take the coordinate set to be modified as the texture parameter").
// ---------------------------------------------------------------------------
void glMultiTexCoord1i(GLenum target, GLint s) {
    glMultiTexCoord1f(target, FromDirectInt(s));
}
void glMultiTexCoord1iv(GLenum target, const GLint* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord1i(target, v[0]);
}
void glMultiTexCoord1s(GLenum target, GLshort s) {
    glMultiTexCoord1f(target, FromDirectShort(s));
}
void glMultiTexCoord1sv(GLenum target, const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord1s(target, v[0]);
}

void glMultiTexCoord2i(GLenum target, GLint s, GLint t) {
    glMultiTexCoord2f(target, FromDirectInt(s), FromDirectInt(t));
}
void glMultiTexCoord2iv(GLenum target, const GLint* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord2i(target, v[0], v[1]);
}
void glMultiTexCoord2s(GLenum target, GLshort s, GLshort t) {
    glMultiTexCoord2f(target, FromDirectShort(s), FromDirectShort(t));
}
void glMultiTexCoord2sv(GLenum target, const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord2s(target, v[0], v[1]);
}

void glMultiTexCoord3i(GLenum target, GLint s, GLint t, GLint r) {
    glMultiTexCoord3f(target, FromDirectInt(s), FromDirectInt(t),
                      FromDirectInt(r));
}
void glMultiTexCoord3iv(GLenum target, const GLint* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord3i(target, v[0], v[1], v[2]);
}
void glMultiTexCoord3s(GLenum target, GLshort s, GLshort t, GLshort r) {
    glMultiTexCoord3f(target, FromDirectShort(s), FromDirectShort(t),
                      FromDirectShort(r));
}
void glMultiTexCoord3sv(GLenum target, const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord3s(target, v[0], v[1], v[2]);
}

void glMultiTexCoord4i(GLenum target, GLint s, GLint t, GLint r, GLint q) {
    glMultiTexCoord4f(target, FromDirectInt(s), FromDirectInt(t),
                      FromDirectInt(r), FromDirectInt(q));
}
void glMultiTexCoord4iv(GLenum target, const GLint* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord4i(target, v[0], v[1], v[2], v[3]);
}
void glMultiTexCoord4s(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
    glMultiTexCoord4f(target, FromDirectShort(s), FromDirectShort(t),
                      FromDirectShort(r), FromDirectShort(q));
}
void glMultiTexCoord4sv(GLenum target, const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glMultiTexCoord4s(target, v[0], v[1], v[2], v[3]);
}

// ---------------------------------------------------------------------------
// Secondary color — glSecondaryColor3{b,i,s,ub,ui,us}{,v}
// Spec §2.14: secondary color chỉ có biến thể 3 thành phần; Secondary A
// luôn bằng 1.0 (LegacyState::currentSecondaryColor chỉ có 3 thành phần).
// ---------------------------------------------------------------------------
void glSecondaryColor3b(GLbyte red, GLbyte green, GLbyte blue) {
    glSecondaryColor3f(FromSignedByte(red), FromSignedByte(green),
                       FromSignedByte(blue));
}
void glSecondaryColor3bv(const GLbyte* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3b(v[0], v[1], v[2]);
}
void glSecondaryColor3i(GLint red, GLint green, GLint blue) {
    glSecondaryColor3f(FromSignedInt(red), FromSignedInt(green),
                       FromSignedInt(blue));
}
void glSecondaryColor3iv(const GLint* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3i(v[0], v[1], v[2]);
}
void glSecondaryColor3s(GLshort red, GLshort green, GLshort blue) {
    glSecondaryColor3f(FromSignedShort(red), FromSignedShort(green),
                       FromSignedShort(blue));
}
void glSecondaryColor3sv(const GLshort* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3s(v[0], v[1], v[2]);
}
void glSecondaryColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    glSecondaryColor3f(FromUnsignedByte(red), FromUnsignedByte(green),
                       FromUnsignedByte(blue));
}
void glSecondaryColor3ubv(const GLubyte* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3ub(v[0], v[1], v[2]);
}
void glSecondaryColor3ui(GLuint red, GLuint green, GLuint blue) {
    glSecondaryColor3f(FromUnsignedInt(red), FromUnsignedInt(green),
                       FromUnsignedInt(blue));
}
void glSecondaryColor3uiv(const GLuint* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3ui(v[0], v[1], v[2]);
}
void glSecondaryColor3us(GLushort red, GLushort green, GLushort blue) {
    glSecondaryColor3f(FromUnsignedShort(red), FromUnsignedShort(green),
                       FromUnsignedShort(blue));
}
void glSecondaryColor3usv(const GLushort* v) {
    if (!v) { NullPointer(); return; }
    glSecondaryColor3us(v[0], v[1], v[2]);
}

}  // namespace tglmt::gl
