// gl_tess_xfb.cpp — Tessellation (đường Metal drawPatches) + XFB emulate + geometry fallback.
// Spec GL: glspec46.core.pdf §11 (Tessellation), §13.x (XFB). Spec Metal: MSL spec
// §5.1.1.1 (post-tessellation vertex function), §5.2.3.2/Table 5.3 (patch inputs),
// MTLTriangleTessellationFactorsHalf (factor buffer).
// Metal KHÔNG có transform feedback / geometry shader native:
//  - XFB: VS-only capture thật bằng compute prepass (interleaved/separate).
//    XFB sau GS/TES và các stage nâng cao vẫn chưa có (ghi rõ).
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
static XfbObject* BoundXFB(Context& c, bool createDefault = false) {
    GLuint id = c.state.BoundXFB();
    auto it = c.xfbs.find(id);
    if (it == c.xfbs.end() && id == 0 && createDefault) {
        XfbObject x; x.id = 0;
        it = c.xfbs.emplace(0, x).first;
    }
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
void glBeginTransformFeedback(GLenum m) {
    Context& c = Context::Current();
    if (m != 0x0000 && m != 0x0001 && m != 0x0002 && m != 0x0003 &&
        m != 0x0004 && m != 0x0005 && m != 0x0006 && m != 0x000E) {
        c.errors.Record(0x0500); return;
    }
    XfbObject* x = BoundXFB(c, true);
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
    if (b >= 4) { c.errors.Record(0x0501); return; }
    auto it = c.xfbs.find(x);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return; }
    if (buf && !c.buffers.count(buf)) { c.errors.Record(0x0502); return; }
    it->second.buffers[b] = buf;
    it->second.offsets[b] = 0;
    it->second.sizes[b] = 0;
}
void glTransformFeedbackBufferRange(GLuint x, GLuint b, GLuint buf, GLintptr o, GLsizeiptr s) {
    Context& c = Context::Current();
    if (b >= 4) { c.errors.Record(0x0501); return; }
    if (o < 0 || s < 0) { c.errors.Record(0x0501); return; }
    auto it = c.xfbs.find(x);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return; }
    if (buf && !c.buffers.count(buf)) { c.errors.Record(0x0502); return; }
    it->second.buffers[b] = buf;
    it->second.offsets[b] = o;
    it->second.sizes[b] = s;
}
// Spec §7.1 glGetTransformFeedbackVarying: type là GL type của varying, size là
// SỐ COMPONENT (không phải số phần tử mảng — mảng handled riêng qua arraySize).
// Bảng cover float/vecN/int/ivecN/uint/uvecN/double/dvecN. Trả GL type của
// element; size (số phần tử) do caller quyết định theo arraySize/reflection.
static bool XfbArrayElem(const std::string& name, std::string& base, int& index) {
    size_t lb = name.rfind('[');
    if (lb == std::string::npos || name.empty() || name.back() != ']') return false;
    std::string ns = name.substr(lb + 1, name.size() - lb - 2);
    if (ns.empty()) return false;
    for (char ch : ns) if (!isdigit((unsigned char)ch)) return false;
    base = name.substr(0, lb);
    index = atoi(ns.c_str());
    return !base.empty();
}
static bool GlslTypeToGL(const std::string& t, GLenum* glType) {
    if (t == "float")  { *glType = 0x1406; return true; } // GL_FLOAT
    if (t == "vec2")   { *glType = 0x8B50; return true; } // GL_FLOAT_VEC2
    if (t == "vec3")   { *glType = 0x8B51; return true; } // GL_FLOAT_VEC3
    if (t == "vec4")   { *glType = 0x8B52; return true; } // GL_FLOAT_VEC4
    if (t == "int")    { *glType = 0x1404; return true; } // GL_INT
    if (t == "ivec2")  { *glType = 0x8B53; return true; } // GL_INT_VEC2
    if (t == "ivec3")  { *glType = 0x8B54; return true; } // GL_INT_VEC3
    if (t == "ivec4")  { *glType = 0x8B55; return true; } // GL_INT_VEC4
    if (t == "uint")   { *glType = 0x1405; return true; } // GL_UNSIGNED_INT
    if (t == "uvec2")  { *glType = 0x8DC6; return true; } // GL_UNSIGNED_INT_VEC2
    if (t == "uvec3")  { *glType = 0x8DC7; return true; } // GL_UNSIGNED_INT_VEC3
    if (t == "uvec4")  { *glType = 0x8DC8; return true; } // GL_UNSIGNED_INT_VEC4
    if (t == "double") { *glType = 0x140A; return true; } // GL_DOUBLE
    if (t == "dvec2")  { *glType = 0x8FFC; return true; } // GL_DOUBLE_VEC2
    if (t == "dvec3")  { *glType = 0x8FFD; return true; } // GL_DOUBLE_VEC3
    if (t == "dvec4")  { *glType = 0x8FFE; return true; } // GL_DOUBLE_VEC4
    return false; // mat*/array → caller fallback
}
void glGetTransformFeedbackVarying(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLsizei* s, GLenum* t, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (i >= it->second.xfbVaryings.size()) { c.errors.Record(0x0501); return; }
    const std::string& nm = it->second.xfbVaryings[i];
    // Reflection THẬT từ vertex shader đã compile. Hỗ trợ tên mảng
    // (`vArr[1]`) và ma trận: size của TF varying là số phần tử kiểu, không
    // nhân component như code cũ.
    std::string base = nm;
    int elem = -1;
    bool isElem = XfbArrayElem(nm, base, elem);
    std::string outName = nm;
    GLenum glType = 0x1406;
    GLsizei size = 1;
    bool found = false;
    for (GLuint sid : it->second.shaders) {
        auto sit = c.shaders.find(sid);
        if (sit == c.shaders.end()) continue;
        if (sit->second.type != 0x8B31 /*VERTEX_SHADER*/) continue;
        for (auto& v : sit->second.conv.outputs) {
            if (v.name != base) continue;
            if (!GlslTypeToGL(v.glslType, &glType)) {
                if (v.glslType == "mat4" || v.glslType == "mat4x4") glType = 0x8B5C;
                else if (v.glslType == "mat3" || v.glslType == "mat3x3") glType = 0x8B5B;
                else if (v.glslType == "mat2" || v.glslType == "mat2x2") glType = 0x8B5A;
                else glType = 0x1406;
            }
            if (isElem) {
                size = 1;
                if (elem < 0 || elem >= (v.arraySize > 0 ? v.arraySize : 1)) {
                    c.errors.Record(0x0501);
                    return;
                }
            } else if (v.arraySize > 0) {
                size = v.arraySize;
                outName = base + "[0]";
            }
            found = true;
            break;
        }
        if (found) break;
    }
    // Matrix bị converter tách thành cột `name_c0..` ở stage-out thường. Dựng
    // lại type FLOAT_MATk (size = 1) cho reflection XFB.
    if (!found) {
        int cols = 0, colArray = 0;
        std::string colMsl;
        bool colArrayConsistent = true;
        for (int ci = 0; ci < 4; ++ci) {
            std::string cn = base + "_c" + std::to_string(ci);
            bool got = false;
            for (GLuint sid : it->second.shaders) {
                auto sit = c.shaders.find(sid);
                if (sit == c.shaders.end() || sit->second.type != 0x8B31) continue;
                for (auto& v : sit->second.conv.outputs) {
                    if (v.name != cn) continue;
                    if (ci == 0) { colMsl = v.mslType; colArray = v.arraySize; }
                    else if (v.mslType != colMsl || v.arraySize != colArray) {
                        colArrayConsistent = false;
                        got = false; break;
                    }
                    ++cols; got = true; break;
                }
                if (got) break;
            }
            if (!got) break;
        }
        if (!colArrayConsistent) {
            c.errors.Record(0x0501);
            return;
        }
        if (cols >= 2 && colMsl.size() >= 6 && colMsl.rfind("float", 0) == 0) {
            int rows = colMsl.size() == 5 ? 1 : (colMsl[5] - '0');
            if (rows >= 1 && rows <= 4) {
                switch (cols * 10 + rows) {
                    case 22: glType = 0x8B5A; break; // FLOAT_MAT2
                    case 33: glType = 0x8B5B; break; // FLOAT_MAT3
                    case 44: glType = 0x8B5C; break; // FLOAT_MAT4
                    case 23: glType = 0x8B65; break; // FLOAT_MAT2x3
                    case 24: glType = 0x8B66; break; // FLOAT_MAT2x4
                    case 32: glType = 0x8B67; break; // FLOAT_MAT3x2
                    case 34: glType = 0x8B68; break; // FLOAT_MAT3x4
                    case 42: glType = 0x8B69; break; // FLOAT_MAT4x2
                    case 43: glType = 0x8B6A; break; // FLOAT_MAT4x3
                    default: glType = 0x1406; break;
                }
                if (colArray > 0) {
                    if (isElem) {
                        if (elem < 0 || elem >= colArray) { c.errors.Record(0x0501); return; }
                        size = 1;
                    } else {
                        size = colArray;
                        outName = base + "[0]";
                    }
                } else {
                    if (isElem && elem != 0) { c.errors.Record(0x0501); return; }
                    size = 1;
                }
                found = true;
            }
        }
    }
    if (s) *s = size;
    if (t) *t = glType;
    size_t k = std::min((size_t)(n > 0 ? n - 1 : 0), outName.size());
    if (name && n > 0) { memcpy(name, outName.c_str(), k); name[k] = 0; }
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
    else if (p == 0x8C84) *v = (GLint)it->second.offsets[i]; // ..._START
    else if (p == 0x8C85) {
        size_t sz = it->second.sizes[i];
        if (sz == 0 && it->second.buffers[i]) {
            auto bit = c.buffers.find(it->second.buffers[i]);
            if (bit != c.buffers.end()) sz = (size_t)BufferLen(bit->second);
        }
        *v = (GLint)sz; // ..._SIZE
    } else *v = 0;
}
void glGetTransformFeedbacki64_v(GLuint x, GLenum p, GLuint i, GLint64* v) {
    Context& c = Context::Current();
    auto it = c.xfbs.find(x);
    if (it == c.xfbs.end() || i >= 4) { c.errors.Record(0x0502); return; }
    if (p == 0x8C8F) *v = (GLint64)it->second.buffers[i];
    else if (p == 0x8C84) *v = (GLint64)it->second.offsets[i];
    else if (p == 0x8C85) {
        size_t sz = it->second.sizes[i];
        if (sz == 0 && it->second.buffers[i]) {
            auto bit = c.buffers.find(it->second.buffers[i]);
            if (bit != c.buffers.end()) sz = (size_t)BufferLen(bit->second);
        }
        *v = (GLint64)sz;
    } else *v = 0;
}
} // namespace tglmt::gl
