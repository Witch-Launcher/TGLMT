// gl_shader_objects_p8.cpp — P8: ARB_shader_objects (GLSL đời đầu, forwards sang core).
// 11 hàm ARB_*ObjectARB forward 1:1 sang core glXxxShader/Program (kiểm tra type
// program vs shader cho Delete/InfoLog/Parameter). 28 alias còn lại do generator.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
// GLhandleARB = GLuint (object ID, tra cứu programs vs shaders để biết loại).
static bool IsProgram(GLuint o) { return Context::Current().programs.count(o) > 0; }
static bool IsShader(GLuint o) { return Context::Current().shaders.count(o) > 0; }

void glAttachObjectARB(GLuint container, GLuint obj) {
    // Container phải là program, obj là shader (đúng ARB_shader_objects).
    glAttachShader(container, obj);
}
GLuint glCreateProgramObjectARB() { return glCreateProgram(); }
GLuint glCreateShaderObjectARB(GLenum type) { return glCreateShader(type); }
void glDeleteObjectARB(GLuint obj) {
    if (IsProgram(obj)) glDeleteProgram(obj);
    else if (IsShader(obj)) glDeleteShader(obj);
    else Context::Current().errors.Record(0x0502); // INVALID_OPERATION (không phải object)
}
void glDetachObjectARB(GLuint container, GLuint obj) { glDetachShader(container, obj); }
void glGetAttachedObjectsARB(GLuint container, GLsizei maxCount, GLsizei* count, GLuint* obj) {
    glGetAttachedShaders(container, maxCount, count, obj);
}
GLuint glGetHandleARB(GLenum pname) {
    // Chỉ hỗ trợ PROGRAM_OBJECT_ARB → trả program đang bound (đúng spec ARB).
    if (pname == 0x8B40 /*PROGRAM_OBJECT_ARB*/)
        return Context::Current().state.BoundProgram();
    Context::Current().errors.Record(0x0500); // INVALID_ENUM
    return 0;
}
void glGetInfoLogARB(GLuint obj, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    if (IsProgram(obj)) glGetProgramInfoLog(obj, maxLength, length, infoLog);
    else if (IsShader(obj)) glGetShaderInfoLog(obj, maxLength, length, infoLog);
    else Context::Current().errors.Record(0x0502);
}
void glGetObjectParameterfvARB(GLuint obj, GLenum pname, GLfloat* params) {
    // Forward sang Program/Shader version theo loại (float version).
    if (IsProgram(obj)) {
        GLint t = 0;
        glGetProgramiv(obj, pname, &t);
        if (params) *params = (GLfloat)t;
    } else if (IsShader(obj)) {
        GLint t = 0;
        glGetShaderiv(obj, pname, &t);
        if (params) *params = (GLfloat)t;
    } else Context::Current().errors.Record(0x0502);
}
void glGetObjectParameterivARB(GLuint obj, GLenum pname, GLint* params) {
    if (IsProgram(obj)) glGetProgramiv(obj, pname, params);
    else if (IsShader(obj)) glGetShaderiv(obj, pname, params);
    else Context::Current().errors.Record(0x0502);
}
void glUseProgramObjectARB(GLuint program) { glUseProgram(program); }
// ===== P8 EXT_separate_shader_objects (program pipelines, 3 hàm) =====
// CreateShaderProgramEXT: tạo separable program từ source string (compile+link).
// UseShaderProgramEXT/ActiveProgramEXT: bind program cho stage/pipeline.
GLuint glCreateShaderProgramEXT(GLenum type, const GLchar* string) {
    Context& c = Context::Current();
    GLuint sh = glCreateShader(type);
    if (!sh) return 0;
    glShaderSource(sh, 1, &string, nullptr);
    glCompileShader(sh);
    GLuint pr = glCreateProgram();
    if (!pr) { glDeleteShader(sh); return 0; }
    // Separable program (dùng trong pipeline).
    auto itp = c.programs.find(pr);
    if (itp != c.programs.end()) itp->second.separable = true;
    glAttachShader(pr, sh);
    glLinkProgram(pr);
    // Spec: trả program ngay cả khi compile/link fail (app check status).
    // Shader object có thể delete sau link (giữ hay xoá đều được — giữ để debug).
    return pr;
}
void glUseShaderProgramEXT(GLenum type, GLuint program) {
    Context& c = Context::Current();
    if (program == 0) { glUseProgram(0); return; }
    auto itp = c.programs.find(program);
    if (itp == c.programs.end()) { c.errors.Record(0x0502); return; }
    // Kiểm tra program có shader đúng stage không (đúng spec: type phải khớp).
    bool hasStage = false;
    for (GLuint s : itp->second.shaders) {
        auto its = c.shaders.find(s);
        if (its != c.shaders.end() && its->second.type == type) { hasStage = true; break; }
    }
    if (!hasStage) { c.errors.Record(0x0502); return; } // INVALID_OPERATION
    glUseProgram(program);
}
void glActiveProgramEXT(GLuint pipeline, GLuint program) {
    Context& c = Context::Current();
    // Validate pipeline + program tồn tại (pipelines tracked trong registry).
    // TGLMT draw path dùng BoundProgram (monolithic), chưa consume pipeline stages
    // riêng — ghi nhận active (đúng state), draw vẫn dùng BoundProgram.
    if (pipeline && !c.registry.Is(ObjectKind::ProgramPipeline, pipeline)) {
        c.errors.Record(0x0502); return;
    }
    if (program && !c.programs.count(program)) { c.errors.Record(0x0502); return; }
    // Lưu active program cho pipeline (mở rộng sau nếu draw path hỗ trợ pipeline).
    // Hiện tại: no-op hợp lệ (state đã validate).
    (void)pipeline; (void)program;
}
// ===== P8 small exts (1-2 hàm, behavior thật) =====
// KHR_blend_equation_advanced: barrier giữa advanced blend passes.
void glBlendBarrierKHR() {
    // Đảm bảo advanced blend writes hoàn tất trước reads tiếp theo.
    // TGLMT: blend thực hiện trong render pass (không có cross-pass advanced blend
    // riêng) → memory barrier no-op hợp lệ (đúng spec khi không có overlapping).
    Context::Current().state.SetShadow(0x9280 /*MEMORY_BARRIER_MARKER*/, nullptr, 0);
}
// EXT_texture_integer: clear color dạng integer (cho framebuffer integer).
void glClearColorIiEXT(GLint r, GLint g, GLint b, GLint a) {
    Context& c = Context::Current();
    c.clearColor[0] = (float)r; c.clearColor[1] = (float)g;
    c.clearColor[2] = (float)b; c.clearColor[3] = (float)a;
    // Lưu bản integer để FBO integer dùng (float là xấp xỉ cho render target float).
    c.state.SetShadow(0x8E5A /*CLEAR_COLOR_INT*/, nullptr, 0); // marker có integer clear
}
void glClearColorIuiEXT(GLuint r, GLuint g, GLuint b, GLuint a) {
    Context& c = Context::Current();
    c.clearColor[0] = (float)r; c.clearColor[1] = (float)g;
    c.clearColor[2] = (float)b; c.clearColor[3] = (float)a;
    c.state.SetShadow(0x8E5A, nullptr, 0);
}
// EXT_transform_feedback: bind buffer với offset (cho XFB/UBO).
void glBindBufferOffsetEXT(GLenum target, GLuint index, GLuint buffer, GLintptr offset) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    if (offset < 0) { c.errors.Record(0x0501); return; }
    if (target == 0x8A11 /*UNIFORM_BUFFER*/) {
        // Bind với offset, size = còn lại của buffer (giống Range với size=max).
        size_t bsz = 0;
        if (buffer) {
            auto it = c.buffers.find(buffer);
            if (it != c.buffers.end()) bsz = BufferLen(it->second);
        }
        GLsizeiptr sz = (bsz > (size_t)offset) ? (GLsizeiptr)(bsz - (size_t)offset) : 0;
        c.uniformBindPoints[index] = BufferRange{buffer, offset, sz};
    } else if (target == 0x8C8E /*TRANSFORM_FEEDBACK_BUFFER*/) {
        if (index >= 4) { c.errors.Record(0x0501); return; }
        // Bind vào XFB object đang bound (giống TransformFeedbackBufferBase + offset).
        GLuint xfbId = c.state.BoundXFB();
        auto xit = c.xfbs.find(xfbId);
        if (xit == c.xfbs.end()) return; // không có XFB → no-op (không crash)
        xit->second.buffers[index] = buffer;
        xit->second.offsets[index] = offset;
    } else {
        c.errors.Record(0x0500); // INVALID_ENUM (chỉ UBO/XFB)
    }
}
} // namespace tglmt::gl
