// gl_get.cpp — Mọi glGet* đọc shadow CPU (Metal không cho query GPU đồng bộ).
// Nguyên tắc: Set* đã lưu shadow → Get* trả shadow; chưa Set → giá trị mặc định spec.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>
using namespace tglmt;

static void OutI(Context& c, GLenum p, GLint* v, GLuint idx = 0) {
    GLint t = 0;
    if (c.state.GetShadow(p, &t, 4, idx)) { *v = t; return; }
    switch (p) {
        case 0x0D33: *v = 4; break; // MAX_TEXTURE_SIZE default expose
        case 0x0B70: *v = 0; break;
        default: *v = 0; break;
    }
}

// Bảng limits tĩnh (A11 baseline, đúng GL 4.6 minimums hoặc hơn).
// Mỗi entry 0 đều là crash-tiềm-năng kiểu DynamicUniformStorage (/ by zero
// khi UNIFORM_BUFFER_OFFSET_ALIGNMENT = 0) nên bảng này phải đầy đủ.
static bool StaticLimit(GLenum p, GLint* v) {
    switch (p) {
        case 0x821B: *v = 4; break;      // MAJOR_VERSION
        case 0x821C: *v = 6; break;      // MINOR_VERSION
        case 0x821D: *v = (GLint)tglmt_num_extensions(); break; // khớp glGetStringi
        case 0x821E: *v = 0; break;      // CONTEXT_FLAGS (không debug bit)
        case 0x0D33: *v = 8192; break;   // MAX_TEXTURE_SIZE
        case 0x8073: *v = 2048; break;   // MAX_3D_TEXTURE_SIZE
        case 0x851C: *v = 8192; break;   // MAX_CUBE_MAP_TEXTURE_SIZE
        case 0x88FF: *v = 2048; break;   // MAX_ARRAY_TEXTURE_LAYERS
        case 0x84E8: *v = 8192; break;   // MAX_RENDERBUFFER_SIZE
        case 0x84F8: *v = 8192; break;   // MAX_RECTANGLE_TEXTURE_SIZE
        case 0x8CDF: *v = 8; break;      // MAX_COLOR_ATTACHMENTS
        case 0x8078: *v = 8; break;      // MAX_DRAW_BUFFERS
        case 0x8869: *v = 16; break;     // MAX_VERTEX_ATTRIBS
        case 0x8872: *v = 32; break;     // MAX_TEXTURE_IMAGE_UNITS
        case 0x8B4C: *v = 16; break;     // MAX_VERTEX_TEXTURE_IMAGE_UNITS
        case 0x8B4D: *v = 48; break;     // MAX_COMBINED_TEXTURE_IMAGE_UNITS
        case 0x8B4A: *v = 4096; break;   // MAX_VERTEX_UNIFORM_COMPONENTS
        case 0x8B49: *v = 4096; break;   // MAX_FRAGMENT_UNIFORM_COMPONENTS
        case 0x8B4B: *v = 128; break;    // MAX_VARYING_COMPONENTS
        case 0x8D57: *v = 4; break;      // MAX_SAMPLES (A11 4x MSAA)
        case 0x910E: *v = 4; break;      // MAX_COLOR_TEXTURE_SAMPLES
        case 0x910F: *v = 4; break;      // MAX_DEPTH_TEXTURE_SAMPLES
        case 0x9110: *v = 4; break;      // MAX_INTEGER_SAMPLES
        case 0x8A2F: *v = 36; break;     // MAX_UNIFORM_BUFFER_BINDINGS
        case 0x8A30: *v = 65536; break;  // MAX_UNIFORM_BLOCK_SIZE
        case 0x8A34: *v = 256; break;    // UNIFORM_BUFFER_OFFSET_ALIGNMENT (0 = crash MC)
        case 0x8A2B: *v = 14; break;     // MAX_VERTEX_UNIFORM_BLOCKS
        case 0x8A2D: *v = 14; break;     // MAX_FRAGMENT_UNIFORM_BLOCKS
        case 0x8A2E: *v = 28; break;     // MAX_COMBINED_UNIFORM_BLOCKS
        case 0x8C2B: *v = 65536; break;  // MAX_TEXTURE_BUFFER_SIZE
        case 0x80E8: *v = 1048576; break;// MAX_ELEMENTS_VERTICES
        case 0x80E9: *v = 1048576; break;// MAX_ELEMENTS_INDICES
        default: return false;
    }
    return true;
}

namespace tglmt::gl {
void glGetIntegerv(GLenum p, GLint* v) {
    Context& c = Context::Current();
    // một số pname đặc biệt trả trực tiếp từ object/state
    switch (p) {
        case 0x0BA2: *v = (GLint)c.state.BoundVAO(); return;      // VERTEX_ARRAY_BINDING
        case 0x8B8D: *v = (GLint)c.state.BoundProgram(); return;  // CURRENT_PROGRAM
        case 0x0C10: *v = 0; return;                               // SCISSOR_TEST off default
        case 0x0B71: *v = c.state.IsEnabled(0x0B71) ? 1 : 0; return; // DEPTH_TEST
        case 0x0BE2: *v = c.state.IsEnabled(0x0BE2) ? 1 : 0; return; // BLEND
        case 0x0B44: *v = c.state.IsEnabled(0x0B44) ? 1 : 0; return; // CULL_FACE
        case 0x0D3A: v[0] = 8192; v[1] = 8192; return;             // MAX_VIEWPORT_DIMS (2 giá trị)
        case 0x1F02: break;
        default: break;
    }
    if (StaticLimit(p, v)) return;
    OutI(c, p, v);
}
void glGetIntegeri_v(GLenum p, GLuint i, GLint* v) { OutI(Context::Current(), p, v, i); }
void glGetInteger64v(GLenum p, GLint64* v) { GLint t=0; glGetIntegerv(p,&t); *v=t; }
void glGetInteger64i_v(GLenum p, GLuint i, GLint64* v) { GLint t=0; glGetIntegeri_v(p,i,&t); *v=t; }
void glGetBooleanv(GLenum p, GLboolean* v) { GLint t=0; glGetIntegerv(p,&t); *v=t?1:0; }
void glGetBooleani_v(GLenum p, GLuint i, GLboolean* v) { GLint t=0; glGetIntegeri_v(p,i,&t); *v=t?1:0; }
void glGetFloatv(GLenum p, GLfloat* v) {
    Context& c = Context::Current();
    GLfloat t = 0;
    if (p == 0x0B66) { // DEPTH_CLEAR_VALUE
        *v = (GLfloat)c.clearDepth; return;
    }
    if (p == 0x0C22) { // COLOR_CLEAR_VALUE
        for (int i=0;i<4;i++) v[i]=c.clearColor[i]; return;
    }
    if (p == 0x0B22) { v[0] = 1; v[1] = 1; return; } // ALIASED_LINE_WIDTH_RANGE
    if (p == 0x0B12) { v[0] = 1; v[1] = 1; return; } // POINT_SIZE_RANGE (cap 1, an toàn)
    if (p == 0x84FF) { *v = 16; return; }            // MAX_TEXTURE_MAX_ANISOTROPY
    if (c.state.GetShadow(p, &t, 4)) { *v = t; return; }
    *v = 0;
}
void glGetFloati_v(GLenum p, GLuint i, GLfloat* v) {
    GLfloat t=0;
    if (Context::Current().state.GetShadow(p,&t,4,i)) *v=t; else *v=0;
}
void glGetDoublev(GLenum p, GLdouble* v) { GLfloat t=0; glGetFloatv(p,&t); *v=t; }
void glGetDoublei_v(GLenum p, GLuint i, GLdouble* v) { GLfloat t=0; glGetFloati_v(p,i,&t); *v=t; }
void glGetBufferParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundBuffer(t);
    auto it = c.buffers.find(id);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8764) *v = (GLint)it->second.data.size();      // BUFFER_SIZE
    else if (p == 0x8765) *v = it->second.usage;              // BUFFER_USAGE
    else if (p == 0x88BB) *v = it->second.mapped ? 1 : 0;     // BUFFER_MAPPED
    else *v = 0;
}
void glGetBufferParameteri64v(GLenum t, GLenum p, GLint64* v) { GLint x=0; glGetBufferParameteriv(t,p,&x); *v=x; }
void glGetBufferPointerv(GLenum t, GLenum p, void** v) {
    Context& c = Context::Current();
    (void)p;
    GLuint id = c.state.BoundBuffer(t);
    auto it = c.buffers.find(id);
    *v = (it == c.buffers.end()) ? nullptr : (void*)it->second.data.data();
}
void glGetNamedBufferParameteriv(GLuint b, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8764) *v = (GLint)it->second.data.size();
    else if (p == 0x8765) *v = it->second.usage;
    else *v = 0;
}
void glGetNamedBufferParameteri64v(GLuint b, GLenum p, GLint64* v) { GLint x=0; glGetNamedBufferParameteriv(b,p,&x); *v=x; }
void glGetNamedBufferPointerv(GLuint b, GLenum p, void** v) {
    (void)p;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    *v = (it == c.buffers.end()) ? nullptr : (void*)it->second.data.data();
}
void glGetShaderiv(GLuint s, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8B81) *v = it->second.compiled ? 1 : 0;      // COMPILE_STATUS
    else if (p == 0x8B84) *v = (GLint)it->second.infoLog.size(); // INFO_LOG_LENGTH
    else if (p == 0x8B88) *v = (GLint)it->second.source.size();  // SHADER_SOURCE_LENGTH
    else if (p == 0x8B4F) *v = it->second.type;             // SHADER_TYPE
    else *v = 0;
}
void glGetShaderInfoLog(GLuint s, GLsizei n, GLsizei* l, GLchar* log) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    size_t k = std::min((size_t)(n > 0 ? n - 1 : 0), it->second.infoLog.size());
    if (k && log) memcpy(log, it->second.infoLog.c_str(), k);
    if (log && n > 0) log[k] = 0;
    if (l) *l = (GLsizei)k;
}
void glGetProgramiv(GLuint p, GLenum q, GLint* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (q == 0x8B82) *v = it->second.linked ? 1 : 0;       // LINK_STATUS
    else if (q == 0x8B84) *v = (GLint)it->second.infoLog.size();
    else if (q == 0x8B86) *v = (GLint)it->second.shaders.size(); // ATTACHED_SHADERS
    // ACTIVE_UNIFORM_BLOCKS (0x8A36=35382): GlProgram.setupUniforms path#2
    // loop [0,count) gọi glGetActiveUniformBlockName tìm builtin {Projection,
    // Lighting, Fog, Globals} không khai trong pipeline. Stub trả 0 → loop
    // không chạy → Globals KHÔNG BAO GIỜ ĐƯỢC BIND cho prog ngoài pipeline
    // desc (terrain/blur) → CameraBlockPos rác, MenuBlurRadius rác.
    else if (q == 0x8A36) *v = (GLint)it->second.uniformBlocks.size();
    else *v = 0;
}
void glGetProgramInfoLog(GLuint p, GLsizei n, GLsizei* l, GLchar* log) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    size_t k = std::min((size_t)(n > 0 ? n - 1 : 0), it->second.infoLog.size());
    if (k && log) memcpy(log, it->second.infoLog.c_str(), k);
    if (log && n > 0) log[k] = 0;
    if (l) *l = (GLsizei)k;
}
void glGetActiveAttrib(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLint* s, GLenum* t, GLchar* name) {
    (void)p;(void)i; if(l)*l=0; if(s)*s=0; if(t)*t=0x1406; if(name&&n>0)name[0]=0;
}
void glGetActiveUniform(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLint* s, GLenum* t, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (l) *l = 0; if (s) *s = 1; if (t) *t = 0x1406;
    if (name && n > 0) {
        // trả tên uniform thứ i (sắp xếp theo loc để ổn định)
        GLuint k = 0;
        for (auto& [nm, loc] : it->second.uniformLoc) {
            if (k++ == i) { strncpy(name, nm.c_str(), (size_t)n - 1); name[n-1] = 0; if(l)*l=(GLsizei)strlen(name); return; }
        }
        name[0] = 0;
    }
}
// Tìm block theo index (index space = uniformBlocks[].index dense 0..N-1,
// cùng không gian với glGetUniformBlockIndex — khớp GL vì linker gán dense).
static ProgramObject::UniformBlock* FindUniformBlock(Context& c, GLuint p, GLuint b) {
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return nullptr; }
    for (auto& ub : it->second.uniformBlocks)
        if (ub.index == b) return &ub;
    c.errors.Record(0x0501 /*INVALID_VALUE: uniform block index vượt range*/);
    return nullptr;
}
void glGetActiveUniformBlockiv(GLuint p, GLuint b, GLenum q, GLint* v) {
    if (!v) return;
    Context& c = Context::Current();
    auto* ub = FindUniformBlock(c, p, b);
    if (!ub) { *v = 0; return; }
    switch (q) {
        case 0x8A3F: *v = (GLint)ub->binding; break;          // UNIFORM_BLOCK_BINDING
        case 0x8A40: *v = (GLint)ub->minSize; break;          // UNIFORM_BLOCK_DATA_SIZE
        // NAME_LENGTH gồm NUL (GL spec). LWJGL glGetActiveUniformBlockName(p,i)
        // (2-arg) query enum này TRƯỚC rồi malloc(bufSize): trả 0 → bufSize=0 →
        // tên rỗng → GlProgram.setupUniforms path#2 skip builtin (Globals) →
        // không gán point → draw bind nhầm buffer point0 (uboSmall zero fallback).
        case 0x8A41: *v = (GLint)ub->name.size() + 1; break;   // UNIFORM_BLOCK_NAME_LENGTH
        // Flag theo stage thật (vsBlocks/fsBlocks): block gộp 2 stage → cả 2 = 1.
        // 0x8A45 là REFERENCED_BY_GEOMETRY_SHADER (TGLMT không có geometry → 0),
        // fragment là 0x8A46 (trước đây ghi nhầm 0x8A45 = fragment).
        case 0x8A44: {                                        // REFERENCED_BY_VERTEX_SHADER
            auto pit = c.programs.find(p);
            *v = (pit != c.programs.end() &&
                  std::find(pit->second.vsBlocks.begin(), pit->second.vsBlocks.end(),
                            ub->name) != pit->second.vsBlocks.end()) ? 1 : 0;
            break;
        }
        case 0x8A45: *v = 0; break;                           // REFERENCED_BY_GEOMETRY_SHADER
        case 0x8A46: {                                        // REFERENCED_BY_FRAGMENT_SHADER
            auto pit = c.programs.find(p);
            *v = (pit != c.programs.end() &&
                  std::find(pit->second.fsBlocks.begin(), pit->second.fsBlocks.end(),
                            ub->name) != pit->second.fsBlocks.end()) ? 1 : 0;
            break;
        }
        default: *v = 0; break;
    }
}
// GlProgram.setupUniforms path#2: glGetActiveUniformBlockName(programId, i)
// với i ∈ [0, ACTIVE_UNIFORM_BLOCKS) → tên block (vd "Globals").
void glGetActiveUniformBlockName(GLuint p, GLuint b, GLsizei n, GLsizei* l, GLchar* name) {
    Context& c = Context::Current();
    auto* ub = FindUniformBlock(c, p, b);
    if (!ub) {
        if (l) *l = 0;
        if (name && n > 0) name[0] = 0;
        return;
    }
    size_t k = std::min((size_t)(n > 0 ? n - 1 : 0), ub->name.size());
    if (k && name) memcpy(name, ub->name.c_str(), k);
    if (name && n > 0) name[k] = 0;
    if (l) *l = (GLsizei)k;
}
void glGetActiveUniformName(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLchar* name) { (void)p;(void)i; if(l)*l=0; if(name&&n>0)name[0]=0; }
void glGetActiveUniformsiv(GLuint p, GLsizei n, const GLuint* idx, GLenum q, GLint* v) { (void)p;(void)idx;(void)q; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetActiveAtomicCounterBufferiv(GLuint p, GLuint b, GLenum q, GLint* v) { (void)p;(void)b;(void)q; *v=0; }
GLint glGetAttribLocation(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !n) { c.errors.Record(0x0502); return -1; }
    auto f = it->second.attribLoc.find(n);
    // Chưa link / không có attrib → -1 (đúng GL). Link fail → attribLoc rỗng → -1.
    return (f == it->second.attribLoc.end()) ? -1 : f->second;
}
GLint glGetFragDataLocation(GLuint p, const GLchar* n) { (void)p;(void)n; return -1; }
GLint glGetFragDataIndex(GLuint p, const GLchar* n) { (void)p;(void)n; return -1; }
void glGetUniformfv(GLuint p, GLint l, GLfloat* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto f = it->second.uniforms.find(l);
    if (f == it->second.uniforms.end()) { v[0] = 0; return; }
    memcpy(v, f->second.data(), f->second.size());
}
void glGetUniformiv(GLuint p, GLint l, GLint* v) { glGetUniformfv(p, l, (GLfloat*)v); }
void glGetUniformuiv(GLuint p, GLint l, GLuint* v) { glGetUniformfv(p, l, (GLfloat*)v); }
void glGetUniformdv(GLuint p, GLint l, GLdouble* v) {
    GLfloat tmp[16] = {0};
    glGetUniformfv(p, l, tmp);
    for (int i = 0; i < 16; ++i) v[i] = tmp[i];
}
// NOTE: glGetnUniform* định nghĩa duy nhất ở gl_packed_robust.cpp (có bufSize guard).
// Không định nghĩa ở đây để tránh duplicate-symbol khi link strict (iOS).
} // namespace tglmt::gl
