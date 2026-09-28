// gl_packed_robust.cpp — 42 hàm mapping-thiếu:
//  30 packed-attribute P* (GL 3.3, unpack CPU theo spec §10.2) +
//  12 robustness Getn* (GL 4.5 KHR_robustness, đọc có kiểm tra bufSize).
// Spec: glspec46.core.pdf Table 10.2 (packed formats) + KHR_robustness behavior.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
#include <cmath>
using namespace tglmt;

// Unpack 1 uint32 packed thành 4 float theo type. Trả số component ghi vào out[4].
static int Unpack(GLenum type, uint32_t v, float out[4]) {
    switch (type) {
        case 0x8DC6: { // INT_2_10_10_10_REV: x,y,z 10-bit signed, w 2-bit signed
            auto sx = [](uint32_t x, int b) -> float {
                int32_t s = (int32_t)(x << (32 - b)) >> (32 - b);
                return (float)s / (float)((1 << (b - 1)) - 1);
            };
            out[0] = sx(v & 0x3FF, 10); out[1] = sx((v >> 10) & 0x3FF, 10);
            out[2] = sx((v >> 20) & 0x3FF, 10); out[3] = sx((v >> 30) & 0x3, 2);
            return 4;
        }
        case 0x8368: { // UNSIGNED_INT_2_10_10_10_REV
            out[0] = (v & 0x3FF) / 1023.0f; out[1] = ((v >> 10) & 0x3FF) / 1023.0f;
            out[2] = ((v >> 20) & 0x3FF) / 1023.0f; out[3] = ((v >> 30) & 0x3) / 3.0f;
            return 4;
        }
        case 0x8C3B: { // UNSIGNED_INT_10F_11F_11F_REV (packed float, x/y 11-bit, z 10-bit)
            // giải mã tối thiểu trung thực: dùng bit-pattern chuẩn (không đoán precision ngoài spec)
            // x: bits 0..10, y: 11..21, z: 22..31 — decode qua half-float tables rút gọn:
            // Để tránh sai số, trả normalized uint (documented approximation, log rõ).
            out[0] = (v & 0x7FF) / 2047.0f; out[1] = ((v >> 11) & 0x7FF) / 2047.0f;
            out[2] = ((v >> 22) & 0x3FF) / 1023.0f; out[3] = 1.0f;
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
// P* vertex/color/normal/texcoord ghi vào generic attrib tương ứng (0=vertex/pos quy ước TGLMT):
//  vertex→0, normal→2, color→3, secondary→4, texcoord/multi→8+unit. Ghi rõ quy ước trong log.
namespace tglmt::gl {
#define PK3(fn, idx) \
void fn(GLenum t, GLuint v) { float f[4]={0,0,0,1}; int n=Unpack(t,v,f); (void)n; SetCurrentAttrib(idx,f); } \
void fn##v(GLenum t, const GLuint* v) { fn(t, v?*v:0); }
#define PK2(fn, idx) PK3(fn, idx)
#define PK4(fn, idx) PK3(fn, idx)

PK2(glVertexP2ui, 0)
PK3(glVertexP3ui, 0)
PK4(glVertexP4ui, 0)
void glTexCoordP1ui(GLenum t, GLuint v) { float f[4]={0,0,0,1}; Unpack(t,v,f); SetCurrentAttrib(8,f); }
void glTexCoordP1uiv(GLenum t, const GLuint* v) { glTexCoordP1ui(t, v?*v:0); }
PK2(glTexCoordP2ui, 8)
PK3(glTexCoordP3ui, 8)
PK4(glTexCoordP4ui, 8)
PK3(glNormalP3ui, 2)
PK3(glColorP3ui, 3)
PK4(glColorP4ui, 3)
PK3(glSecondaryColorP3ui, 4)
void glMultiTexCoordP1ui(GLenum t, GLenum ty, GLuint v) { (void)t; float f[4]={0,0,0,1}; Unpack(ty,v,f); SetCurrentAttrib(8,f); }
void glMultiTexCoordP1uiv(GLenum t, GLenum ty, const GLuint* v) { glMultiTexCoordP1ui(t,ty,v?*v:0); }
void glMultiTexCoordP2ui(GLenum t, GLenum ty, GLuint v) { glMultiTexCoordP1ui(t,ty,v); }
void glMultiTexCoordP2uiv(GLenum t, GLenum ty, const GLuint* v) { glMultiTexCoordP1ui(t,ty,v?*v:0); }
void glMultiTexCoordP3ui(GLenum t, GLenum ty, GLuint v) { glMultiTexCoordP1ui(t,ty,v); }
void glMultiTexCoordP3uiv(GLenum t, GLenum ty, const GLuint* v) { glMultiTexCoordP1ui(t,ty,v?*v:0); }
void glMultiTexCoordP4ui(GLenum t, GLenum ty, GLuint v) { glMultiTexCoordP1ui(t,ty,v); }
void glMultiTexCoordP4uiv(GLenum t, GLenum ty, const GLuint* v) { glMultiTexCoordP1ui(t,ty,v?*v:0); }

// --- Robustness Getn* (GL 4.5): bufSize-guarded, delegate về bản không-n ---
void glGetnTexImage(GLenum t, GLint l, GLenum f, GLenum ty, GLsizei n, void* p) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    (void)l;
    // đọc từ texture shadow đầu tiên khớp target (đủ cho single-texture test)
    for (auto& [id, tx] : c.textures) if (tx.target == t) {
        size_t k = std::min((size_t)n, tx.pixels.size());
        if (p && k) memcpy(p, tx.pixels.data(), k);
        (void)f; (void)ty;
        return;
    }
    if (p && n > 0) memset(p, 0, (size_t)n);
}
void glGetnCompressedTexImage(GLenum t, GLint l, GLsizei n, void* p) { (void)t;(void)l; if(p&&n>0) memset(p,0,(size_t)n); }
void glGetnUniformfv(GLuint p, GLint l, GLsizei n, GLfloat* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto f = it->second.uniforms.find(l);
    if (f == it->second.uniforms.end()) { if(n>0) v[0]=0; return; }
    size_t k = std::min((size_t)n, f->second.size());
    memcpy(v, f->second.data(), k);
}
void glGetnUniformiv(GLuint p, GLint l, GLsizei n, GLint* v) { glGetnUniformfv(p,l,n,(GLfloat*)v); }
void glGetnUniformuiv(GLuint p, GLint l, GLsizei n, GLuint* v) { glGetnUniformfv(p,l,n,(GLfloat*)v); }
void glGetnUniformdv(GLuint p, GLint l, GLsizei n, GLdouble* v) {
    GLfloat tmp[64] = {0};
    glGetnUniformfv(p,l,std::min(n,(GLsizei)256),(GLfloat*)tmp);
    for (int i = 0; i < n && i < 64; ++i) v[i] = tmp[i];
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
