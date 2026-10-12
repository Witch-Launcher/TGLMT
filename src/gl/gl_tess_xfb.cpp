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
// Đổi GLSL type string (converter đã parse) sang (GLenum GL type, số component).
// Spec §7.1 glGetTransformFeedbackVarying: type là GL type của varying, size là
// SỐ COMPONENT (không phải số phần tử mảng — mảng handled riêng qua arraySize).
// Bảng cover float/vecN/int/ivecN/uint/uvecN (99% varying của MC + Sodium +
// shaderpack Iris). Ma trận / types khác fallback (1, FLOAT) + ghi chú.
static bool GlslTypeToGL(const std::string& t, GLenum* glType, GLsizei* comps) {
    if (t == "float")  { *glType = 0x1406; *comps = 1; return true; } // GL_FLOAT
    if (t == "vec2")   { *glType = 0x1406; *comps = 2; return true; }
    if (t == "vec3")   { *glType = 0x1406; *comps = 3; return true; }
    if (t == "vec4")   { *glType = 0x1406; *comps = 4; return true; }
    if (t == "int")    { *glType = 0x1404; *comps = 1; return true; } // GL_INT
    if (t == "ivec2")  { *glType = 0x1404; *comps = 2; return true; }
    if (t == "ivec3")  { *glType = 0x1404; *comps = 3; return true; }
    if (t == "ivec4")  { *glType = 0x1404; *comps = 4; return true; }
    if (t == "uint")   { *glType = 0x1405; *comps = 1; return true; } // GL_UNSIGNED_INT
    if (t == "uvec2")  { *glType = 0x1405; *comps = 2; return true; }
    if (t == "uvec3")  { *glType = 0x1405; *comps = 3; return true; }
    if (t == "uvec4")  { *glType = 0x1405; *comps = 4; return true; }
    return false; // mat*/double*/array → caller fallback
}
void glGetTransformFeedbackVarying(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLsizei* s, GLenum* t, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (i >= it->second.xfbVaryings.size()) { c.errors.Record(0x0501); return; }
    const std::string& nm = it->second.xfbVaryings[i];
    // Reflection THẬT từ vertex shader đã compile (GLSLConvertResult::outputs chứa
    // glslType + name). Hành vi cũ hardcode (GL_FLOAT, 1) → app tính sai độ rộng
    // varying rồi ghi XFB lệch offset. Tìm theo tên trong mọi attached VS.
    GLenum glType = 0x1406;
    GLsizei comps = 1;
    bool found = false;
    for (GLuint sid : it->second.shaders) {
        auto sit = c.shaders.find(sid);
        if (sit == c.shaders.end()) continue;
        if (sit->second.type != 0x8B31 /*VERTEX_SHADER*/) continue;
        for (auto& v : sit->second.conv.outputs) {
            if (v.name == nm) {
                if (!GlslTypeToGL(v.glslType, &glType, &comps)) {
                    // mat*/types khác: size = số cột (mat4 → 4), type = FLOAT_MAT4.
                    if (v.glslType == "mat4")       { glType = 0x140C; comps = 4; } // FLOAT_MAT4
                    else if (v.glslType == "mat3")  { glType = 0x140B; comps = 3; } // FLOAT_MAT3
                    else if (v.glslType == "mat2")  { glType = 0x140A; comps = 2; } // FLOAT_MAT2
                    else                            { glType = 0x1406; comps = 1; } // fallback
                }
                if (v.arraySize > 0) comps = comps * v.arraySize; // mảng: tổng component
                found = true;
                break;
            }
        }
        if (found) break;
    }
    if (s) *s = comps;
    if (t) *t = glType;
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
