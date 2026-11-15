// gl_legacy_colorindex.cpp — COLOR-INDEX MODE + ACCUMULATION BUFFER
// (OpenGL Compatibility Profile).
//
// Spec (đã đối chiếu văn bản PDF, không suy đoán):
//   • OpenGL 4.2 Compatibility Profile §2.7 "Vertex Specification"
//     (.cache/spec/glspec42.compatibility.pdf, PDF p.55):
//       "void Index{sifd ub}(T index); void Index{sifd ub}v(const T index);
//        updates the current (single-valued) color index. It takes one
//        argument, the value to which the current color index should be set.
//        Values outside the (machine-dependent) representable range of color
//        indices are not clamped."
//     (OpenGL 2.1 §2.7, glspec21.pdf PDF p.36 — câu chữ GIỐNG HỆT).
//     ⇒ color index KHÔNG bị clamp; KHÔNG dùng Table 2.9 (quy tắc đó áp cho
//       color COMPONENT, không áp cho color INDEX — GL 2.1 §2.14, PDF p.73:
//       "In color index mode, a single-valued color index is not mapped.").
//   • §2.13.6 "Clamping or Masking" (PDF p.111; GL 2.1 §2.14.6, PDF p.83):
//       "For a color index, the index is first converted to fixed-point with an
//        unspecified number of bits to the right of the binary point; the
//        nearest fixed-point value is selected. Then, the bits to the right of
//        the binary point are left alone while the integer portion is masked
//        (bitwise ANDed) with 2^n - 1, where n is the number of bits in a
//        color in the color index buffer."
//     ⇒ float/double → làm tròn số gần nhất; phần nguyên mới bị mask (mask
//       áp khi GHI vào color index buffer, không phải lúc set current index).
//   • §4.2.2 "Fine Control of Buffer Updates" (PDF p.427) — IndexMask:
//       "The least significant n bits of mask, where n is the number of bits
//        in a color index buffer, specify a mask. Where a 1 appears in this
//        mask, the corresponding bit in the color index buffer ... is written;
//        where a 0 appears, the bit is not written."
//   • §4.2 "Whole Framebuffer Operations" (PDF p.429) — ClearIndex/ClearAccum:
//       ClearIndex(float): "index is converted to a fixed-point value with
//         unspecified precision ...; the integer part of this value is then
//         masked with 2^m - 1, where m is the number of bits in a color index
//         value stored in the framebuffer."
//       ClearAccum(float r, float g, float b, float a) (PDF p.429–430): "takes
//         four floating-point arguments that are the values, in order, to
//         which to set the R, G, B, and A values of the accumulation buffer
//         ... These values are clamped to the range [-1, 1] when they are
//         specified."
//   • §4.2.4 "The Accumulation Buffer" (PDF p.432–433; GL 2.1 §4.2.4,
//     glspec21.pdf PDF p.234–235) — Accum:
//       "void Accum(enum op, float value); ... The possible operations are
//        ACCUM, LOAD, RETURN, MULT, and ADD." + "If there is no accumulation
//        buffer; if the DRAW_FRAMEBUFFER and READ_FRAMEBUFFER bindings do not
//        refer to the same object; or if the GL is in color index mode,
//        Accum generates the error INVALID_OPERATION."
//   • §2.6.3 "GL Commands within Begin / End" (PDF p.51): Index commands được
//     PHÉP trong glBegin/glEnd ("Vertex, Color, SecondaryColor, Index,
//     Normal, TexCoord and MultiTexCoord, ..."); mọi lệnh khác (kể cả
//     ClearIndex/IndexMask/Accum/ClearAccum) ⇒ INVALID_OPERATION.
//
// 14 entry point trong file này:
//   glClearIndex, glIndexMask,
//   glIndex{d,dv,f,fv,i,iv,s,sv,ub,ubv},
//   glAccum, glClearAccum.
//
// LƯU Ý VỀ ACCUMULATION BUFFER:
//   Metal KHÔNG có accumulation buffer ⇒ TGLMT chỉ giữ STATE (op/value/
//   clearValue) theo đúng spec, KHÔNG giả lập buffer (xem báo cáo: cần
//   một texture float RGBA + full-screen pass — điểm hook được ghi rõ).
//   LƯU Ý (sai lệch CÓ CHỦ ĐÍCH): theo spec 4.2C §4.2.4 (PDF p.433), khi
//   KHÔNG có accumulation buffer thì glAccum phải sinh INVALID_OPERATION.
//   TGLMT ghi state thay vì báo lỗi, vì sẽ gắn buffer thật (emulation) sau;
//   xem mục "SAI LỆCH" trong báo cáo.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <cmath>

using namespace tglmt;

namespace tglmt::gl {

namespace {

// Số bit của một color index trong color index buffer. Spec coi đây là giá
// trị machine-dependent ("the number of bits in a color index value stored in
// the framebuffer"); TGLMT chọn 8 bit (giá trị điển hình của color index
// buffer trong GL 1.x/2.1). Giá trị này quyết định mask áp cho
// glClearIndex (2^m − 1) — xem spec 4.2C §4.2 (PDF p.429).
constexpr GLuint kColorIndexBits = 8;
constexpr GLuint kColorIndexMask = (1u << kColorIndexBits) - 1u;

// Con trỏ NULL cho các biến thể vector: Desktop GL không liệt kê lỗi cho
// trường hợp này — ta ghi GL_INVALID_VALUE để không deref NULL (an toàn).
// Cùng quy ước với src/gl/gl_legacy_raster.cpp:66-69.
inline void NullPointerError(Context& c) { c.errors.Record(GL_INVALID_VALUE); }

// Ghi current color index vào CẢ hai chỗ của state:
//   • Context::legacy.currentIndex          — chỗ glGetIntegerv(GL_CURRENT_INDEX)
//     đọc (src/gl/gl_get.cpp, case 0x0B01) và đường draw immediate dùng
//     (src/gl/gl_legacy_vertex.cpp:224);
//   • Context::legacy.colorIndex.currentIndex — nhóm COLOR-INDEX của
//     LegacyState (spec §2.7).
void SetCurrentColorIndex(GLuint idx) {
    Context& c = Context::Current();
    c.legacy.currentIndex = idx;
    c.legacy.colorIndex.currentIndex = idx;
}

// Chuyển tham số float/double của lệnh Index thành color index (số nguyên).
//
// Spec 4.2C §2.13.6 (PDF p.111): color index "is first converted to fixed-point
// ... the nearest fixed-point value is selected" ⇒ LÀM TRÌN số gần nhất.
// Spec 4.2C §2.7 (PDF p.55): "Values outside the (machine-dependent)
// representable range of color indices are NOT clamped" ⇒ KHÔNG clamp về
// [0, 255]; giá trị âm giữ theo mẫu 2's complement 32 bit (state là GLuint).
// (Khi ghi vào color index buffer, phần nguyên mới bị mask với 2^n − 1 —
// xem §2.13.6 — nên về mặt framebuffer giá trị âm hành xử như bị mask.)
GLuint ColorIndexFromFloat(double v) {
    if (!(v == v)) return 0;  // NaN ⇒ 0 (tránh UB khi ép kiểu)
    const double r = std::round(v);  // nửa ⇒ ra xa khỏi 0
    if (r >= 4294967295.0) return 0xFFFFFFFFu;
    if (r <= -4294967295.0) return 0u;  // -2^32 ≡ 0 (mod 2^32)
    // Ép qua int64_t để không lỗi "out of range" khi double > INT_MAX.
    return (GLuint)(int64_t)r;
}

// Op hợp lệ của glAccum.
//
// Spec 4.2C §4.2.4 (PDF p.432) và GL 2.1 §4.2.4 (glspec21.pdf PDF p.234) liệt
// kê ĐÚNG 5 op: "The possible operations are ACCUM, LOAD, RETURN, MULT, and
// ADD." Ba op SUBTRACT/MIN/MAX không có trong spec OpenGL nào (GL_SUBTRACT =
// 0x84E7, GL_MIN = 0x8007, GL_MAX = 0x8008 là hằng của texture-env/combiner).
// TGLMT chấp nhận CẢ 8 (superset) theo yêu cầu của slice này, để app legacy
// dùng op lạ không bị từ chối; op ngoài 8 ⇒ GL_INVALID_ENUM.
// (Nếu cần strict theo 4.2C thì bỏ 3 case GL_SUBTRACT/GL_MIN/GL_MAX.)
bool ValidAccumOp(GLenum op) {
    switch (op) {
        case GL_ACCUM:
        case GL_LOAD:
        case GL_RETURN:
        case GL_MULT:
        case GL_ADD:
        case GL_SUBTRACT:
        case GL_MIN:
        case GL_MAX:
            return true;
        default:
            return false;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Color index — current color index (spec 4.2C §2.7, PDF p.55)
// ---------------------------------------------------------------------------
// glIndex* được PHÉP gọi trong glBegin/glEnd (spec §2.6.3, PDF p.51: danh
// sách lệnh được phép gồm "Vertex, Color, SecondaryColor, Index, Normal,
// TexCoord and MultiTexCoord, ..."), nên KHÔNG sinh lỗi ở đây.

void glIndexd(GLdouble c) { SetCurrentColorIndex(ColorIndexFromFloat(c)); }
void glIndexf(GLfloat c) { SetCurrentColorIndex(ColorIndexFromFloat((double)c)); }
void glIndexi(GLint c) { SetCurrentColorIndex((GLuint)c); }   // KHÔNG clamp
void glIndexs(GLshort c) { SetCurrentColorIndex((GLuint)c); }
void glIndexub(GLubyte c) { SetCurrentColorIndex((GLuint)c); }

void glIndexdv(const GLdouble* c) {
    if (!c) { NullPointerError(Context::Current()); return; }
    glIndexd(c[0]);
}
void glIndexfv(const GLfloat* c) {
    if (!c) { NullPointerError(Context::Current()); return; }
    glIndexf(c[0]);
}
void glIndexiv(const GLint* c) {
    if (!c) { NullPointerError(Context::Current()); return; }
    glIndexi(c[0]);
}
void glIndexsv(const GLshort* c) {
    if (!c) { NullPointerError(Context::Current()); return; }
    glIndexs(c[0]);
}
void glIndexubv(const GLubyte* c) {
    if (!c) { NullPointerError(Context::Current()); return; }
    glIndexub(c[0]);
}

// ---------------------------------------------------------------------------
// glIndexMask — write mask của color index (spec 4.2C §4.2.2, PDF p.427)
// ---------------------------------------------------------------------------
void glIndexMask(GLuint mask) {
    Context& c = Context::Current();
    // Không nằm trong danh sách lệnh được phép trong glBegin/glEnd
    // (spec §2.6.3, PDF p.51) ⇒ INVALID_OPERATION.
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // "The least significant n bits of mask ... specify a mask" (n = số bit
    // của color index buffer). Ta lưu mask ĐẦY ĐỦ; phần bit cao bị bỏ qua
    // khi ghi vào color index buffer (nơi mask cuối cùng được áp dụng).
    c.legacy.colorIndex.indexMask = mask;
    c.state.SetShadow(GL_INDEX_WRITEMASK, &mask, sizeof(mask));
}

// ---------------------------------------------------------------------------
// glClearIndex — clear value của color index (spec 4.2C §4.2, PDF p.429)
// ---------------------------------------------------------------------------
void glClearIndex(GLfloat c) {
    Context& ctx = Context::Current();
    if (ctx.legacy.inBeginEnd) { ctx.errors.Record(GL_INVALID_OPERATION); return; }
    // "index is converted to a fixed-point value with unspecified precision to
    // the left of the binary point; the integer part of this value is then
    // masked with 2^m - 1, where m is the number of bits in a color index
    // value stored in the framebuffer."
    const GLuint v = ColorIndexFromFloat((double)c) & kColorIndexMask;
    ctx.legacy.colorIndex.clearIndex = v;
    ctx.state.SetShadow(GL_INDEX_CLEAR_VALUE, &v, sizeof(v));
}

// ---------------------------------------------------------------------------
// glAccum — accumulation buffer operation (spec 4.2C §4.2.4, PDF p.432–433)
// ---------------------------------------------------------------------------
void glAccum(GLenum op, GLfloat value) {
    Context& c = Context::Current();
    // Op sai ⇒ GL_INVALID_ENUM (điều kiện chung về enum của spec).
    if (!ValidAccumOp(op)) { c.errors.Record(GL_INVALID_ENUM); return; }
    // glAccum KHÔNG nằm trong danh sách lệnh được phép trong glBegin/glEnd
    // (spec §2.6.3, PDF p.51) ⇒ INVALID_OPERATION.
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // "or if the GL is in color index mode, Accum generates the error
    // INVALID_OPERATION" (spec §4.2.4, PDF p.433).
    if (c.legacy.colorIndex.indexMode) {
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    // TGLMT chưa có accumulation buffer (Metal không có) ⇒ chỉ ghi STATE;
    // buffer thật cần một float RGBA texture + full-screen pass (xem báo cáo).
    //
    // SAI LỆCH CÓ CHỦ ĐÍCH: spec 4.2C §4.2.4 (PDF p.433) nói "If there is no
    // accumulation buffer ... Accum generates the error INVALID_OPERATION".
    // TGLMT KHÔNG báo lỗi (vì sẽ gắn buffer thật ở tầng Metal sau) — app
    // legacy gọi glAccum sẽ không bị chặn. Khi buffer được gắn, nhánh
    // INVALID_OPERATION cho "không có accumulation buffer" phải được thêm vào
    // đây (xem báo cáo: điểm hook).
    c.legacy.accum.op = op;
    c.legacy.accum.value = value;
}

// ---------------------------------------------------------------------------
// glClearAccum — clear value của accumulation buffer
// (spec 4.2C §4.2, PDF p.429–430)
// ---------------------------------------------------------------------------
void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    Context& c = Context::Current();
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // "These values are clamped to the range [-1, 1] when they are specified."
    auto clampAccum = [](GLfloat v) {
        if (v < -1.0f) return -1.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    };
    const GLfloat r = clampAccum(red);
    const GLfloat g = clampAccum(green);
    const GLfloat b = clampAccum(blue);
    const GLfloat a = clampAccum(alpha);
    // LegacyAccum chỉ có MỘT trường clearValue (đúng với state GL_ACCUM_CLEAR_VALUE
    // — 1 float/pixel ở RGBA mode) ⇒ lưu thành phần R; 4 thành phần đầy đủ
    // được giữ trong shadow 16 byte cho tầng buffer sau này.
    // (glGetFloatv(GL_ACCUM_CLEAR_VALUE) đọc shadow ⇒ trả thành phần R; muốn
    // trả đủ RGBA cần thêm nhánh trong src/gl/gl_get.cpp:LegacyPnameF —
    // xem báo cáo, điểm hook.)
    c.legacy.accum.clearValue = r;
    const GLfloat rgba[4] = {r, g, b, a};
    c.state.SetShadow(GL_ACCUM_CLEAR_VALUE, rgba, sizeof(rgba));
}

// ---------------------------------------------------------------------------
// Accessor cho test + hook gl_get.cpp (báo cáo).
// ---------------------------------------------------------------------------
GLuint LegacyIndexClearValue() { return Context::Current().legacy.colorIndex.clearIndex; }
GLuint LegacyIndexWriteMask() { return Context::Current().legacy.colorIndex.indexMask; }
GLuint LegacyCurrentColorIndex() { return Context::Current().legacy.currentIndex; }
float LegacyAccumClearValue() { return Context::Current().legacy.accum.clearValue; }
GLenum LegacyAccumOp() { return Context::Current().legacy.accum.op; }
float LegacyAccumValue() { return Context::Current().legacy.accum.value; }
bool LegacyAccumBufferPresent() {
    // TGLMT chưa có accumulation buffer (Metal không có) — xem báo cáo.
    return false;
}

}  // namespace tglmt::gl
