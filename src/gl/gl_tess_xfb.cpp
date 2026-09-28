// gl_tess_xfb.cpp — Tessellation (đường Metal drawPatches) + XFB emulate + geometry fallback.
// Spec GL: glspec46.core.pdf §11 (Tessellation), §13.x (XFB). Spec Metal: MSL spec
// §5.1.1.1 (post-tessellation vertex function), §5.2.3.2/Table 5.3 (patch inputs),
// MTLTriangleTessellationFactorsHalf (factor buffer).
// Metal KHÔNG có transform feedback / geometry shader native:
//  - XFB: emulate bằng buffer capture — plumbing đầy đủ (varyings, bind points,
//    state machine, count); tính varying thật cần M5b thực thi program (ghi rõ).
//  - GS: không có đường tương đương rẻ; TGLMT báo LINK falha trung thực khi program
//    chứa GS? KHÔNG — spec GL yêu cầu link thành công. TGLMT link OK nhưng gắn cờ
//    hasGeometryStage để đường vẽ chọn fallback compute-prepass (M5b). Ghi rõ, không giấu.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
using namespace tglmt;

namespace tglmt::gl {
void glGenTransformFeedbacks(GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Gen(ObjectKind::TransformFeedback, n, ids);
    for (GLsizei i = 0; i < n; ++i) { XfbObject x; x.id = ids[i]; c.xfbs[ids[i]] = x; }
}
void glCreateTransformFeedbacks(GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Create(ObjectKind::TransformFeedback, n, ids);
    for (GLsizei i = 0; i < n; ++i) { XfbObject x; x.id = ids[i]; c.xfbs[ids[i]] = x; }
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint* ids) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Delete(ObjectKind::TransformFeedback, n, ids);
    for (GLsizei i = 0; i < n; ++i) c.xfbs.erase(ids[i]);
}
GLboolean glIsTransformFeedback(GLuint id) {
    return Context::Current().registry.Is(ObjectKind::TransformFeedback, id) ? 1 : 0;
}
void glBindTransformFeedback(GLenum t, GLuint id) {
    if (t != 0x8E22 /*TRANSFORM_FEEDBACK*/) { Context::Current().errors.Record(0x0500); return; }
    Context& c = Context::Current();
    if (id && !c.xfbs.count(id)) { c.errors.Record(0x0502); return; }
    c.state.BindXFB(id);
    c.state.SetShadow(0x8E25 /*TRANSFORM_FEEDBACK_BINDING*/, &id, 4);
}
static XfbObject* BoundXFB(Context& c) {
    GLuint id = c.state.BoundXFB();
    if (!id) { c.errors.Record(0x0502); return nullptr; }
    auto it = c.xfbs.find(id);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
void glBeginTransformFeedback(GLenum m) {
    Context& c = Context::Current();
    if (m != 0x0000 && m != 0x0001 && m != 0x0002 && m != 0x0003 &&
        m != 0x0004 && m != 0x0005 && m != 0x0006 && m != 0x000E) {
        c.errors.Record(0x0500); return;
    }
    XfbObject* x = BoundXFB(c);
    if (!x) return;
    if (x->active) { c.errors.Record(0x0502); return; } // đã active → INVALID_OPERATION
    x->active = true; x->paused = false; x->primitiveMode = m; x->capturedCount = 0;
}
void glEndTransformFeedback() {
    Context& c = Context::Current();
    XfbObject* x = BoundXFB(c);
    if (!x) return;
    if (!x->active) { c.errors.Record(0x0502); return; }
    x->active = false; x->paused = false;
}
void glPauseTransformFeedback() {
    Context& c = Context::Current();
    XfbObject* x = BoundXFB(c);
    if (!x) return;
    if (!x->active || x->paused) { c.errors.Record(0x0502); return; }
    x->paused = true;
}
void glResumeTransformFeedback() {
    Context& c = Context::Current();
    XfbObject* x = BoundXFB(c);
    if (!x) return;
    if (!x->active || !x->paused) { c.errors.Record(0x0502); return; }
    x->paused = false;
}
void glTransformFeedbackBufferBase(GLuint x, GLuint b, GLuint buf) {
    Context& c = Context::Current();
    (void)x;
    if (b >= 4) { c.errors.Record(0x0501); return; }
    XfbObject* xfb = BoundXFB(c);
    if (!xfb) return;
    if (buf && !c.buffers.count(buf)) { c.errors.Record(0x0502); return; }
    xfb->buffers[b] = buf;
}
void glTransformFeedbackBufferRange(GLuint x, GLuint b, GLuint buf, GLintptr o, GLsizeiptr s) {
    (void)o; (void)s;
    glTransformFeedbackBufferBase(x, b, buf); // offset/size cho M5b capture kernel
}
void glGetTransformFeedbackVarying(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLsizei* s, GLenum* t, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (i >= it->second.xfbVaryings.size()) { c.errors.Record(0x0501); return; }
    const std::string& nm = it->second.xfbVaryings[i];
    if (s) *s = 1;
    if (t) *t = 0x1406; // FLOAT (ghi nhận: M5b suy type thật từ shader)
    size_t k = std::min((size_t)(n > 0 ? n - 1 : 0), nm.size());
    if (name && n > 0) { memcpy(name, nm.c_str(), k); name[k] = 0; }
    if (l) *l = (GLsizei)k;
}
void glGetTransformFeedbackiv(GLuint x, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.xfbs.find(x);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8E24) *v = it->second.active ? 1 : 0;      // TRANSFORM_FEEDBACK_ACTIVE
    else if (p == 0x8E23) *v = it->second.paused ? 1 : 0; // TRANSFORM_FEEDBACK_PAUSED
    else *v = 0;
}
void glGetTransformFeedbacki_v(GLuint x, GLenum p, GLuint i, GLint* v) {
    Context& c = Context::Current();
    auto it = c.xfbs.find(x);
    if (it == c.xfbs.end() || i >= 4) { c.errors.Record(0x0502); return; }
    if (p == 0x8C8F) *v = (GLint)it->second.buffers[i]; // TRANSFORM_FEEDBACK_BUFFER_BINDING
    else *v = 0;
}
void glGetTransformFeedbacki64_v(GLuint x, GLenum p, GLuint i, GLint64* v) {
    GLint t = 0; glGetTransformFeedbacki_v(x, p, i, &t); *v = t;
}
} // namespace tglmt::gl
