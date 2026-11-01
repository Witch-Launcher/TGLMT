// gl_vertex.cpp — VAO + vertex attrib. Metal: MTLVertexDescriptor + setVertexBuffer.
// Spec: glspec46.core.pdf §10 (Vertex Specification).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
void glGenVertexArrays(GLsizei n, GLuint* a) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::VertexArray, n, a);
    for (GLsizei i = 0; i < n; ++i) { VertexArrayObject v; v.id = a[i]; c.vaos[a[i]] = v; }
}
void glCreateVertexArrays(GLsizei n, GLuint* a) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::VertexArray, n, a);
    for (GLsizei i = 0; i < n; ++i) { VertexArrayObject v; v.id = a[i]; c.vaos[a[i]] = v; }
}
void glDeleteVertexArrays(GLsizei n, const GLuint* a) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::VertexArray, n, a);
    for (GLsizei i = 0; i < n; ++i) c.vaos.erase(a[i]);
}
GLboolean glIsVertexArray(GLuint a) {
    return Context::Current().registry.Is(ObjectKind::VertexArray, a) ? 1 : 0;
}
void glBindVertexArray(GLuint a) {
    Context& c = Context::Current();
    if (a && !c.vaos.count(a)) { c.errors.Record(0x0502); return; }
    c.state.BindVAO(a);
}
static VertexArrayObject* CurVAO(Context& c) {
    GLuint id = c.state.BoundVAO();
    if (!id) { c.errors.Record(0x0502); return nullptr; }
    auto it = c.vaos.find(id);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
void glEnableVertexAttribArray(GLuint i) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].enabled = true;
}
void glDisableVertexAttribArray(GLuint i) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].enabled = false;
}
void glEnableVertexArrayAttrib(GLuint va, GLuint i) {
    Context& c = Context::Current();
    auto it = c.vaos.find(va);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (i >= 16) { c.errors.Record(0x0501); return; }
    it->second.attribs[i].enabled = true;
}
void glDisableVertexArrayAttrib(GLuint va, GLuint i) {
    Context& c = Context::Current();
    auto it = c.vaos.find(va);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    it->second.attribs[i].enabled = false;
}
void glVertexAttribPointer(GLuint i, GLint size, GLenum type, GLboolean norm, GLsizei stride, const void* ptr) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    auto& a = v->attribs[i];
    a.size = size; a.type = type; a.normalized = norm; a.stride = stride;
    a.relativeOffset = (size_t)ptr;
    a.buffer = c.state.BoundBuffer(0x8892); // ARRAY_BUFFER tại thời điểm gọi (spec §10.3)
    a.isInt = false; a.isLong = false;
    // Legacy: binding giữ nguyên (mặc định 0), offset binding = 0 → effective = ptr.
    if (a.binding < v->bindings.size()) {
        v->bindings[a.binding].buffer = a.buffer;
        v->bindings[a.binding].offset = 0;
        v->bindings[a.binding].stride = stride;
    }
    VAOSyncAttribOffset(*v, i);
}
void glVertexAttribIPointer(GLuint i, GLint size, GLenum type, GLsizei stride, const void* ptr) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    auto& a = v->attribs[i];
    a.size = size; a.type = type; a.stride = stride; a.relativeOffset = (size_t)ptr;
    a.buffer = c.state.BoundBuffer(0x8892); a.isInt = true;
    // Format integer: normalized/isLong của lần gọi trước không còn ý nghĩa
    // (VAO tái dùng cho format khác → descriptor sai âm thầm, như IFormat).
    a.normalized = 0; a.isLong = false;
    if (a.binding < v->bindings.size()) {
        v->bindings[a.binding].buffer = a.buffer;
        v->bindings[a.binding].offset = 0;
        v->bindings[a.binding].stride = stride;
    }
    VAOSyncAttribOffset(*v, i);
}
void glVertexAttribLPointer(GLuint i, GLint size, GLenum type, GLsizei stride, const void* ptr) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    auto& a = v->attribs[i];
    a.size = size; a.type = type; a.stride = stride; a.relativeOffset = (size_t)ptr;
    a.buffer = c.state.BoundBuffer(0x8892); a.isLong = true;
    // Như IPointer: reset flag format cũ (tái dùng VAO khác format).
    a.normalized = 0; a.isInt = false;
    if (a.binding < v->bindings.size()) {
        v->bindings[a.binding].buffer = a.buffer;
        v->bindings[a.binding].offset = 0;
        v->bindings[a.binding].stride = stride;
    }
    VAOSyncAttribOffset(*v, i);
}
void glVertexAttribFormat(GLuint i, GLint size, GLenum type, GLboolean norm, GLuint rel) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].size = size; v->attribs[i].type = type;
    v->attribs[i].normalized = norm; v->attribs[i].relativeOffset = rel;
    VAOSyncAttribOffset(*v, i);
}
void glVertexAttribIFormat(GLuint i, GLint size, GLenum type, GLuint rel) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].size = size; v->attribs[i].type = type; v->attribs[i].relativeOffset = rel; v->attribs[i].isInt = true;
    // Format integer: normalized/isLong của lần gọi trước không còn ý nghĩa
    // (VAO có thể được dùng lại cho format khác → descriptor sai âm thầm).
    v->attribs[i].normalized = 0; v->attribs[i].isLong = false;
    VAOSyncAttribOffset(*v, i);
}
void glVertexAttribLFormat(GLuint i, GLint size, GLenum type, GLuint rel) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].size = size; v->attribs[i].type = type; v->attribs[i].relativeOffset = rel; v->attribs[i].isLong = true;
    v->attribs[i].normalized = 0; v->attribs[i].isInt = false;
    VAOSyncAttribOffset(*v, i);
}
void glBindVertexBuffer(GLuint bi, GLuint buf, GLintptr off, GLsizei stride) {
    Context& c = Context::Current();
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    if (bi >= v->bindings.size()) { c.errors.Record(0x0501); return; }
    v->bindings[bi].buffer = buf;
    v->bindings[bi].offset = off;
    v->bindings[bi].stride = stride;
    // Gắn buffer/stride vào mọi attrib dùng binding này; GIỮ relativeOffset
    // (spec §10.3.1: effective = binding.offset + relative). Ghi đè offset ở đây
    // từng làm mọi attribute đọc từ đầu buffer → đen màn hình trên máy (26.x dùng
    // Separate path: Format/Binding trước, BindVertexBuffer sau).
    for (GLuint k = 0; k < (GLuint)v->attribs.size(); ++k) {
        auto& a = v->attribs[k];
        if (a.binding == bi) { a.buffer = buf; a.stride = stride; VAOSyncAttribOffset(*v, k); }
    }
}
void glBindVertexBuffers(GLuint f, GLsizei n, const GLuint* b, const GLintptr* o, const GLsizei* s) {
    for (GLsizei i = 0; i < n; ++i) glBindVertexBuffer(f + i, b ? b[i] : 0, o ? o[i] : 0, s ? s[i] : 0);
}
void glVertexBindingDivisor(GLuint bi, GLuint d) {
    Context& c = Context::Current();
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    if (bi >= v->bindings.size()) { c.errors.Record(0x0501); return; }
    v->bindings[bi].divisor = d;
    for (auto& a : v->attribs) if (a.binding == bi) a.divisor = d;
}
void glVertexAttribBinding(GLuint ai, GLuint bi) {
    Context& c = Context::Current();
    if (ai >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    if (bi >= v->bindings.size()) { c.errors.Record(0x0501); return; }
    v->attribs[ai].binding = bi;
    VAOSyncAttribOffset(*v, ai); // base đổi theo binding mới
}
void glVertexAttribDivisor(GLuint i, GLuint d) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    VertexArrayObject* v = CurVAO(c);
    if (!v) return;
    v->attribs[i].divisor = d;
    // Divisor là trạng thái của binding (spec §10.3.1) → đồng bộ cả hai.
    if (v->attribs[i].binding < v->bindings.size()) v->bindings[v->attribs[i].binding].divisor = d;
}
// glVertexAttrib* (set current generic values) — shadow để shader mặc định đọc
static void SetAttribValue(GLuint i, const double v[4]) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    c.state.SetShadow(0x8626 /*CURRENT_VERTEX_ATTRIB*/, v, 32, i);
}
void glVertexAttrib1f(GLuint i, GLfloat x) { double v[4]={x,0,0,1}; SetAttribValue(i,v); }
void glVertexAttrib1fv(GLuint i, const GLfloat* v) { double d[4]={v[0],0,0,1}; SetAttribValue(i,d); }
void glVertexAttrib2f(GLuint i, GLfloat x, GLfloat y) { double v[4]={x,y,0,1}; SetAttribValue(i,v); }
void glVertexAttrib2fv(GLuint i, const GLfloat* v) { double d[4]={v[0],v[1],0,1}; SetAttribValue(i,d); }
void glVertexAttrib3f(GLuint i, GLfloat x, GLfloat y, GLfloat z) { double v[4]={x,y,z,1}; SetAttribValue(i,v); }
void glVertexAttrib3fv(GLuint i, const GLfloat* v) { double d[4]={v[0],v[1],v[2],1}; SetAttribValue(i,d); }
void glVertexAttrib4f(GLuint i, GLfloat x, GLfloat y, GLfloat z, GLfloat w) { double v[4]={x,y,z,w}; SetAttribValue(i,v); }
void glVertexAttrib4fv(GLuint i, const GLfloat* v) { double d[4]={v[0],v[1],v[2],v[3]}; SetAttribValue(i,d); }
void glVertexAttrib1d(GLuint i, GLdouble x) { double v[4]={x,0,0,1}; SetAttribValue(i,v); }
void glVertexAttrib1dv(GLuint i, const GLdouble* v) { double d[4]={v[0],0,0,1}; SetAttribValue(i,d); }
void glVertexAttrib2d(GLuint i, GLdouble x, GLdouble y) { double v[4]={x,y,0,1}; SetAttribValue(i,v); }
void glVertexAttrib2dv(GLuint i, const GLdouble* v) { double d[4]={v[0],v[1],0,1}; SetAttribValue(i,d); }
void glVertexAttrib3d(GLuint i, GLdouble x, GLdouble y, GLdouble z) { double v[4]={x,y,z,1}; SetAttribValue(i,v); }
void glVertexAttrib3dv(GLuint i, const GLdouble* v) { double d[4]={v[0],v[1],v[2],1}; SetAttribValue(i,d); }
void glVertexAttrib4d(GLuint i, GLdouble x, GLdouble y, GLdouble z, GLdouble w) { double v[4]={x,y,z,w}; SetAttribValue(i,v); }
void glVertexAttrib4dv(GLuint i, const GLdouble* v) { SetAttribValue(i,v); }
void glVertexAttribI1i(GLuint i, GLint x) { double v[4]={(double)x,0,0,1}; SetAttribValue(i,v); }
void glVertexAttribI1iv(GLuint i, const GLint* v) { double d[4]={ (double)v[0],0,0,1}; SetAttribValue(i,d); }
void glVertexAttribI1ui(GLuint i, GLuint x) { double v[4]={(double)x,0,0,1}; SetAttribValue(i,v); }
void glVertexAttribI1uiv(GLuint i, const GLuint* v) { double d[4]={ (double)v[0],0,0,1}; SetAttribValue(i,d); }
void glVertexAttribI2i(GLuint i, GLint x, GLint y) { double v[4]={(double)x,(double)y,0,1}; SetAttribValue(i,v); }
void glVertexAttribI2iv(GLuint i, const GLint* v) { double d[4]={(double)v[0],(double)v[1],0,1}; SetAttribValue(i,d); }
void glVertexAttribI2ui(GLuint i, GLuint x, GLuint y) { double v[4]={(double)x,(double)y,0,1}; SetAttribValue(i,v); }
void glVertexAttribI2uiv(GLuint i, const GLuint* v) { double d[4]={(double)v[0],(double)v[1],0,1}; SetAttribValue(i,d); }
void glVertexAttribI3i(GLuint i, GLint x, GLint y, GLint z) { double v[4]={(double)x,(double)y,(double)z,1}; SetAttribValue(i,v); }
void glVertexAttribI3iv(GLuint i, const GLint* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],1}; SetAttribValue(i,d); }
void glVertexAttribI3ui(GLuint i, GLuint x, GLuint y, GLuint z) { double v[4]={(double)x,(double)y,(double)z,1}; SetAttribValue(i,v); }
void glVertexAttribI3uiv(GLuint i, const GLuint* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],1}; SetAttribValue(i,d); }
void glVertexAttribI4i(GLuint i, GLint x, GLint y, GLint z, GLint w) { double v[4]={(double)x,(double)y,(double)z,(double)w}; SetAttribValue(i,v); }
void glVertexAttribI4iv(GLuint i, const GLint* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribI4ui(GLuint i, GLuint x, GLuint y, GLuint z, GLuint w) { double v[4]={(double)x,(double)y,(double)z,(double)w}; SetAttribValue(i,v); }
void glVertexAttribI4uiv(GLuint i, const GLuint* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribI4bv(GLuint i, const GLbyte* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribI4sv(GLuint i, const GLshort* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribI4ubv(GLuint i, const GLubyte* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribI4usv(GLuint i, const GLushort* v) { double d[4]={(double)v[0],(double)v[1],(double)v[2],(double)v[3]}; SetAttribValue(i,d); }
void glVertexAttribL1d(GLuint i, GLdouble x) { double v[4]={x,0,0,1}; SetAttribValue(i,v); }
void glVertexAttribL1dv(GLuint i, const GLdouble* v) { double d[4]={v[0],0,0,1}; SetAttribValue(i,d); }
void glVertexAttribL2d(GLuint i, GLdouble x, GLdouble y) { double v[4]={x,y,0,1}; SetAttribValue(i,v); }
void glVertexAttribL2dv(GLuint i, const GLdouble* v) { double d[4]={v[0],v[1],0,1}; SetAttribValue(i,d); }
void glVertexAttribL3d(GLuint i, GLdouble x, GLdouble y, GLdouble z) { double v[4]={x,y,z,1}; SetAttribValue(i,v); }
void glVertexAttribL3dv(GLuint i, const GLdouble* v) { double d[4]={v[0],v[1],v[2],1}; SetAttribValue(i,d); }
void glVertexAttribL4d(GLuint i, GLdouble x, GLdouble y, GLdouble z, GLdouble w) { double v[4]={x,y,z,w}; SetAttribValue(i,v); }
void glVertexAttribL4dv(GLuint i, const GLdouble* v) { SetAttribValue(i,v); }
static bool DecodePackedAttrib(GLenum type, GLboolean normalized, GLuint value, double out[4]) {
    if (type == 0x8D9F) { // GL_INT_2_10_10_10_REV
        auto sx = [](uint32_t x, int bits) -> int {
            return (int32_t)(x << (32 - bits)) >> (32 - bits);
        };
        const int c[4] = {sx(value & 0x3FFu, 10), sx((value >> 10) & 0x3FFu, 10),
                          sx((value >> 20) & 0x3FFu, 10), sx((value >> 30) & 0x3u, 2)};
        for (int k = 0; k < 4; ++k) {
            if (!normalized) out[k] = (double)c[k];
            else {
                const double scale = (k == 3) ? 1.0 : 511.0;
                out[k] = (double)c[k] / scale;
                if (out[k] < -1.0) out[k] = -1.0;
            }
        }
        return true;
    }
    if (type == 0x8368) { // GL_UNSIGNED_INT_2_10_10_10_REV
        const uint32_t c[4] = {value & 0x3FFu, (value >> 10) & 0x3FFu,
                               (value >> 20) & 0x3FFu, (value >> 30) & 0x3u};
        for (int k = 0; k < 4; ++k) {
            if (!normalized) out[k] = (double)c[k];
            else out[k] = (double)c[k] / ((k == 3) ? 3.0 : 1023.0);
        }
        return true;
    }
    return false;
}
static void SetPackedAttrib(GLuint i, GLenum type, GLboolean normalized, GLuint value, int components) {
    Context& c = Context::Current();
    double out[4] = {0.0, 0.0, 0.0, 1.0};
    if (!DecodePackedAttrib(type, normalized, value, out)) {
        c.errors.Record(0x0500); // GL_INVALID_ENUM
        return;
    }
    if (components < 2) out[1] = 0.0;
    if (components < 3) out[2] = 0.0;
    if (components < 4) out[3] = 1.0;
    SetAttribValue(i, out);
}
void glVertexAttribP1ui(GLuint i, GLenum t, GLboolean n, GLuint v) { SetPackedAttrib(i, t, n, v, 1); }
void glVertexAttribP1uiv(GLuint i, GLenum t, GLboolean n, const GLuint* v) { SetPackedAttrib(i, t, n, v ? *v : 0, 1); }
void glVertexAttribP2ui(GLuint i, GLenum t, GLboolean n, GLuint v) { SetPackedAttrib(i, t, n, v, 2); }
void glVertexAttribP2uiv(GLuint i, GLenum t, GLboolean n, const GLuint* v) { SetPackedAttrib(i, t, n, v ? *v : 0, 2); }
void glVertexAttribP3ui(GLuint i, GLenum t, GLboolean n, GLuint v) { SetPackedAttrib(i, t, n, v, 3); }
void glVertexAttribP3uiv(GLuint i, GLenum t, GLboolean n, const GLuint* v) { SetPackedAttrib(i, t, n, v ? *v : 0, 3); }
void glVertexAttribP4ui(GLuint i, GLenum t, GLboolean n, GLuint v) { SetPackedAttrib(i, t, n, v, 4); }
void glVertexAttribP4uiv(GLuint i, GLenum t, GLboolean n, const GLuint* v) { SetPackedAttrib(i, t, n, v ? *v : 0, 4); }
} // namespace tglmt::gl
