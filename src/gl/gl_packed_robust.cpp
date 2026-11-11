// gl_packed_robust.cpp — 42 hàm mapping-thiếu:
//  30 packed-attribute P* (GL 3.3, unpack CPU theo spec §10.2) +
//  12 robustness Getn* (GL 4.5 KHR_robustness, đọc có kiểm tra bufSize).
// Spec: glspec46.core.pdf Table 10.2 (packed formats) + KHR_robustness behavior.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/UniformQuery.h"
#include <cstring>
#include <cmath>
using namespace tglmt;

// --- Packed vertex data (ARB_vertex_type_2_10_10_10_rev + ARB_vertex_type_10f_11f_11f_rev)
// Bố cục 32-bit (glspec46.core.pdf §10.3.8, Table 10.4/10.6):
//   UNSIGNED_INT_2_10_10_10_REV / INT_2_10_10_10_REV : w(2) | z(10) | y(10) | x(10)
//   UNSIGNED_INT_10F_11F_11F_REV                      : z(10) | y(11) | x(11)  (chỉ 3 comp)
// Component số nguyên → float theo eq 2.1/2.2 (§2.3.5.1): có dấu phải clamp xuống -1.0.

// Unsigned 11-bit float (glspec46.core.pdf §2.3.4.3): không có bit dấu, E 5-bit, M 6-bit.
static float DecodeUFloat11(uint32_t n) {
    const uint32_t e = (n >> 6) & 0x1Fu, m = n & 0x3Fu;
    if (e == 0) return m == 0 ? 0.0f : std::ldexp((float)m / 64.0f, -14);
    if (e == 31) return m == 0 ? INFINITY : NAN;
    return std::ldexp(1.0f + (float)m / 64.0f, (int)e - 15);
}
// Unsigned 10-bit float (§2.3.4.4): không có bit dấu, E 5-bit, M 5-bit.
static float DecodeUFloat10(uint32_t n) {
    const uint32_t e = (n >> 5) & 0x1Fu, m = n & 0x1Fu;
    if (e == 0) return m == 0 ? 0.0f : std::ldexp((float)m / 32.0f, -14);
    if (e == 31) return m == 0 ? INFINITY : NAN;
    return std::ldexp(1.0f + (float)m / 32.0f, (int)e - 15);
}

// Số component thật mang trong packed word (0 = type không hợp lệ).
static int PackedTypeComponents(GLenum type) {
    switch (type) {
        case 0x8368: // UNSIGNED_INT_2_10_10_10_REV
        case 0x8D9F: // INT_2_10_10_10_REV
            return 4;
        case 0x8C3B: // UNSIGNED_INT_10F_11F_11F_REV
            return 3;
        default:
            return 0;
    }
}

// Unpack 1 uint32 packed thành 4 float theo type. Trả số component ghi vào out[4].
static int Unpack(GLenum type, uint32_t v, float out[4]) {
    switch (type) {
        case 0x8D9F: { // INT_2_10_10_10_REV: x,y,z 10-bit signed, w 2-bit signed
            // eq 2.2: f = max(c / (2^(b-1) - 1), -1.0) — giá trị âm biên (-512, -2) bị clamp.
            auto sx = [](uint32_t x, int b) -> float {
                int32_t s = (int32_t)(x << (32 - b)) >> (32 - b);
                float f = (float)s / (float)((1 << (b - 1)) - 1);
                return f < -1.0f ? -1.0f : f;
            };
            out[0] = sx(v & 0x3FF, 10); out[1] = sx((v >> 10) & 0x3FF, 10);
            out[2] = sx((v >> 20) & 0x3FF, 10); out[3] = sx((v >> 30) & 0x3, 2);
            return 4;
        }
        case 0x8368: { // UNSIGNED_INT_2_10_10_10_REV — eq 2.1: f = c / (2^b - 1)
            out[0] = (v & 0x3FF) / 1023.0f; out[1] = ((v >> 10) & 0x3FF) / 1023.0f;
            out[2] = ((v >> 20) & 0x3FF) / 1023.0f; out[3] = ((v >> 30) & 0x3) / 3.0f;
            return 4;
        }
        case 0x8C3B: { // UNSIGNED_INT_10F_11F_11F_REV: x,y 11-bit float, z 10-bit float
            out[0] = DecodeUFloat11(v & 0x7FFu);
            out[1] = DecodeUFloat11((v >> 11) & 0x7FFu);
            out[2] = DecodeUFloat10((v >> 22) & 0x3FFu);
            out[3] = 1.0f;
            return 3;
        }
        default: return 0;
    }
}

static void SetCurrentAttrib(GLuint idx, const float f[4]) {
    Context& c = Context::Current();
    double d[4] = {f[0], f[1], f[2], f[3]};
    c.state.SetShadow(0x8626 /*CURRENT_VERTEX_ATTRIB*/, d, 32, idx);
}
// P* vertex/color/normal/texcoord ghi vào generic attrib theo layout fixed-function
// của GL (compat profile): vertex→0, normal→2, color→3, secondary→4, texcoord→8+unit.
// Trước đây PK2 trùng PK3 (VertexP2ui ghi 4 component) và MultiTexCoordP* bỏ qua
// tham số `texture` → mọi unit ghi đè lên attrib 8.
namespace tglmt::gl {

// Ghi `comps` component đầu của packed word vào generic attrib idx.
// Lệnh chỉ tiêu thụ n component đầu (spec ARB §"New Procedures": VertexP2ui→x,y;
// ColorP3ui→r,g,b) → phần còn lại giữ giá trị mặc định 0, w=1 (giống Vertex2f/3f).
// Type không thuộc 3 format packed → GL_INVALID_ENUM (ARB_vertex_type_2_10_10_10_rev
// §Errors). UNSIGNED_INT_10F_11F_11F_REV chỉ mang 3 component nên lệnh P4 (cần 4)
// → GL_INVALID_ENUM (ARB_vertex_type_10f_11f_11f_rev: "there are only 3 components
// present"); P1/P2/P3 vẫn dùng được vì §10.3.8 đã định nghĩa format này.
static void SetPackedAttrib(GLuint idx, GLenum type, uint32_t v, int comps) {
    Context& c = Context::Current();
    const int have = PackedTypeComponents(type);
    if (have == 0 || comps > have) { c.errors.Record(0x0500); return; }
    float f[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    Unpack(type, v, f);
    if (comps < 4) f[3] = 1.0f;
    if (comps < 3) f[2] = 0.0f;
    if (comps < 2) f[1] = 0.0f;
    SetCurrentAttrib(idx, f);
}

// MultiTexCoordP*: `texture` chọn texture unit → generic attrib 8+unit
// (spec ARB: "MultiTexCoordP[size]uiv(TEXTURE0 + j, ...)" — mỗi unit một attrib riêng).
static void SetMultiTexCoordAttrib(GLenum texture, GLenum type, uint32_t v, int comps) {
    if (texture < 0x84C0 /*GL_TEXTURE0*/ || texture >= 0x84C0 + 32) {
        Context::Current().errors.Record(0x0500); // INVALID_ENUM
        return;
    }
    SetPackedAttrib(8u + (GLuint)(texture - 0x84C0), type, v, comps);
}

#define PK_N(fn, idx, n) \
void fn(GLenum t, GLuint v) { SetPackedAttrib((idx), t, v, (n)); } \
void fn##v(GLenum t, const GLuint* v) { SetPackedAttrib((idx), t, v ? *v : 0u, (n)); }

#define MTP_N(fn, n) \
void fn(GLenum tex, GLenum t, GLuint v) { SetMultiTexCoordAttrib(tex, t, v, (n)); } \
void fn##v(GLenum tex, GLenum t, const GLuint* v) { SetMultiTexCoordAttrib(tex, t, v ? *v : 0u, (n)); }

PK_N(glVertexP2ui, 0, 2)
PK_N(glVertexP3ui, 0, 3)
PK_N(glVertexP4ui, 0, 4)
PK_N(glTexCoordP1ui, 8, 1)
PK_N(glTexCoordP2ui, 8, 2)
PK_N(glTexCoordP3ui, 8, 3)
PK_N(glTexCoordP4ui, 8, 4)
PK_N(glNormalP3ui, 2, 3)
PK_N(glColorP3ui, 3, 3)
PK_N(glColorP4ui, 3, 4)
PK_N(glSecondaryColorP3ui, 4, 3)
MTP_N(glMultiTexCoordP1ui, 1)
MTP_N(glMultiTexCoordP2ui, 2)
MTP_N(glMultiTexCoordP3ui, 3)
MTP_N(glMultiTexCoordP4ui, 4)

// --- Robustness Getn* (GL 4.5): bufSize-guarded, delegate về bản không-n ---
// glGetnTexImage chuyển sang gl_get_extra.cpp (dùng chung GetTexSubImpl tươi GPU).
// glGetnCompressedTexImage cũng chuyển sang gl_get_extra.cpp (compressed bytes thật).
// GL 4.6 §7.14 GetnUniform*: bufSize là SỐ BYTE của buffer caller.
//   INVALID_VALUE     : bufSize < 0
//   INVALID_OPERATION : program không hợp lệ / chưa link / location không hợp lệ
//   INVALID_OPERATION : buffer cần lớn hơn bufSize → KHÔNG ghi gì (không truncate)
// Dùng chung ReadUniformValueTyped để có đúng ngữ nghĩa chuyển đổi kiểu
// với glGetUniform* (trước đây glGetnUniformdv đọc double qua float ⇒ rác).
void glGetnUniformfv(GLuint p, GLint l, GLsizei n, GLfloat* v) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    // Số byte dữ liệu sẽ ghi: dùng kích thước đã lưu nếu có, nếu không thì
    // kích thước theo kiểu khai báo của uniform.
    size_t need = 0;
    auto f = pr->uniforms.find(l);
    if (f != pr->uniforms.end()) need = f->second.bytes.size();
    else {
        auto sit = pr->uniforms.find(l);
        if (sit == pr->uniforms.end()) { c.errors.Record(0x0502); return; }
        int cols = 0, rows = 0;
        int comps = UniformComponentCount(sit->second.spec.type);
        if (!comps && UniformTypeIsMatrix(sit->second.spec.type, cols, rows)) comps = cols * rows;
        if (!comps) comps = 1;
        const bool dbl = UniformTypeIsDouble(sit->second.spec.type);
        need = (size_t)comps * (sit->second.spec.arraySize > 0 ? sit->second.spec.arraySize : 1) *
               (dbl ? sizeof(GLdouble) : sizeof(GLfloat));
    }
    if ((size_t)n < need) { c.errors.Record(0x0502); return; }
    ReadUniformValueTyped(c, *pr, l, 0x1406, v);
}
void glGetnUniformiv(GLuint p, GLint l, GLsizei n, GLint* v) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x1404, v);
}
void glGetnUniformuiv(GLuint p, GLint l, GLsizei n, GLuint* v) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x1405, v);
}
void glGetnUniformdv(GLuint p, GLint l, GLsizei n, GLdouble* v) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x140A, v);
}
void glGetnMapdv(GLenum t, GLenum q, GLsizei n, GLdouble* v) { (void)t;(void)q; for(GLsizei i=0;i<n;++i)v[i]=0; } // evaluator (compat-lean, luôn 0 ở core)
void glGetnMapfv(GLenum t, GLenum q, GLsizei n, GLfloat* v) { (void)t;(void)q; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnMapiv(GLenum t, GLenum q, GLsizei n, GLint* v) { (void)t;(void)q; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnPixelMapfv(GLenum m, GLsizei n, GLfloat* v) { (void)m; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnPixelMapuiv(GLenum m, GLsizei n, GLuint* v) { (void)m; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnPixelMapusv(GLenum m, GLsizei n, GLushort* v) { (void)m; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnPolygonStipple(GLsizei n, GLubyte* v) { for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetnColorTable(GLenum t, GLenum f, GLenum ty, GLsizei n, void* v) { (void)t;(void)f;(void)ty; if(v&&n>0)memset(v,0,(size_t)n); }
void glGetnConvolutionFilter(GLenum t, GLenum f, GLenum ty, GLsizei n, void* v) { (void)t;(void)f;(void)ty; if(v&&n>0)memset(v,0,(size_t)n); }
void glGetnSeparableFilter(GLenum t, GLenum f, GLenum ty, GLsizei rn, void* r, GLsizei cn, void* col, void* s) { (void)t;(void)f;(void)ty; if(r&&rn>0)memset(r,0,(size_t)rn); if(col&&cn>0)memset(col,0,(size_t)cn); (void)s; }
void glGetnHistogram(GLenum t, GLboolean r, GLenum f, GLenum ty, GLsizei n, void* v) { (void)t;(void)r;(void)f;(void)ty; if(v&&n>0)memset(v,0,(size_t)n); }
void glGetnMinmax(GLenum t, GLboolean r, GLenum f, GLenum ty, GLsizei n, void* v) { (void)t;(void)r;(void)f;(void)ty; if(v&&n>0)memset(v,0,(size_t)n); }
} // namespace tglmt::gl
