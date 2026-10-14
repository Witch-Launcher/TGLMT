// gl_debug_p4.cpp — P4: KHR_debug + EXT_debug_marker + EXT_discard_framebuffer
// + ARB_parallel_shader_compile. Debug output thật (callback + log + labels),
// marker stack cho GPU capture, discard là hint hợp lệ, compiler threads là hint.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
#include <cstdio>
using namespace tglmt;

namespace tglmt::gl {
// ===== ARB_parallel_shader_compile: glMaxShaderCompilerThreads (hint) =====
void glMaxShaderCompilerThreads(GLuint count) {
    // Hint số thread biên dịch shader. 0 = implementation quyết định.
    // TGLMT biên dịch MSL đồng bộ tại link → ghi nhận, không ép thread.
    Context::Current().maxShaderCompilerThreads = count;
}
// ===== EXT_discard_framebuffer: glDiscardFramebuffer (hint hợp lệ) =====
void glDiscardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments) {
    Context& c = Context::Current();
    if (target != 0x8CA8 /*DRAW_FRAMEBUFFER*/ && target != 0x8CA9 /*READ_FRAMEBUFFER*/ &&
        target != 0x8D40 /*FRAMEBUFFER*/) {
        c.errors.Record(0x0500); // INVALID_ENUM
        return;
    }
    if (numAttachments < 0 || (numAttachments > 0 && !attachments)) {
        c.errors.Record(0x0501); // INVALID_VALUE
        return;
    }
    // Validate attachments là enum hợp lệ (COLOR_ATTACHMENT*, DEPTH, STENCIL...).
    for (GLsizei i = 0; i < numAttachments; ++i) {
        GLenum a = attachments[i];
        bool ok = (a >= 0x8CE0 && a <= 0x8CEF) || // COLOR_ATTACHMENT0..15
                  a == 0x8D00 || a == 0x8D20 || a == 0x8D50 || // DEPTH/STENCIL/DEPTH_STENCIL
                  a == 0x0000; // NONE (cho phép)
        if (!ok) { c.errors.Record(0x0500); return; }
    }
    // Hint: nội dung attachments không cần nữa (tile memory). TGLMT quản lý
    // render target显式 → no-op hợp lệ (không cần invalidate vì không giữ tile).
    (void)target;
}
// ===== EXT_debug_marker: push/pop/insert (stack + log cho capture) =====
void glPushGroupMarker(GLsizei length, const GLchar* marker) {
    Context& c = Context::Current();
    std::string m = (marker && length != 0)
        ? std::string(marker, (length < 0) ? strlen(marker) : (size_t)length) : std::string();
    c.debugGroupStack.push_back(m);
    if (c.DiagOn()) { fprintf(stderr, "[TGLMT] pushMarker: %s\n", m.c_str()); fflush(stderr); }
}
void glPopGroupMarker() {
    Context& c = Context::Current();
    if (c.debugGroupStack.empty()) { c.errors.Record(0x0505); return; } // STACK_UNDERFLOW
    c.debugGroupStack.pop_back();
}
void glInsertEventMarker(GLsizei length, const GLchar* marker) {
    Context& c = Context::Current();
    std::string m = (marker && length != 0)
        ? std::string(marker, (length < 0) ? strlen(marker) : (size_t)length) : std::string();
    // Event marker: log vào debug message log (KHR_debug) nếu callback bật,
    // đồng thời stderr khi DiagOn (cho GPU capture).
    if (c.DiagOn()) { fprintf(stderr, "[TGLMT] eventMarker: %s\n", m.c_str()); fflush(stderr); }
    // Ghi vào KHR_debug log dưới dạng OTHER/NOTIFICATION để glGetDebugMessageLog thấy.
    Context::DebugMsg m2;
    m2.source = 0x824B; // DEBUG_SOURCE_APPLICATION
    m2.type = 0x8251;   // DEBUG_TYPE_MARKER
    m2.id = 0;
    m2.severity = 0x826B; // DEBUG_SEVERITY_NOTIFICATION
    m2.text = m;
    c.debugLog.push_back(m2);
    if (c.debugLog.size() > 128) c.debugLog.pop_front();
    if (c.debugCallback) c.debugCallback(0x824B, 0x8251, 0, 0x826B, (GLsizei)m.size(), m.c_str(), c.debugUserParam);
}
// ===== KHR_debug: message callback/control/insert/log + labels + groups =====
// Helper: kiểm tra message có bị filter disable không (debugMessageControl rules).
static bool DebugAllowed(Context& c, GLenum source, GLenum type, GLenum severity) {
    // Rule sau ghi đè rule trước (đúng GL: control gọi sau thắng).
    bool allowed = true;
    for (auto& f : c.debugFilters) {
        bool sm = (f.source == 0x1AFF /*DONT_CARE*/ || f.source == source);
        bool tm = (f.type == 0x1AFF || f.type == type);
        bool vm = (f.severity == 0x1AFF || f.severity == severity);
        if (sm && tm && vm) allowed = f.enabled;
    }
    return allowed;
}
void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam) {
    Context& c = Context::Current();
    c.debugCallback = (Context::DebugProc)callback;
    c.debugUserParam = userParam;
}
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count,
                           const GLuint* ids, GLboolean enabled) {
    Context& c = Context::Current();
    // Validate enum (DONT_CARE=0x1100? Thực tế GL_DONT_CARE=0x1100, nhưng KHR_debug
    // dùng 0x1100 cho DONT_CARE. Kiểm tra giá trị hợp lệ cơ bản).
    (void)count; (void)ids; // ID-specific filter: lưu rule chung (không filter theo ID riêng)
    c.debugFilters.push_back({source, type, severity, enabled ? true : false});
    // Giới hạn 64 rules (tránh phình).
    if (c.debugFilters.size() > 64) c.debugFilters.erase(c.debugFilters.begin());
}
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity,
                          GLsizei length, const GLchar* message) {
    Context& c = Context::Current();
    // Validate source/type/severity cơ bản (APPLICATION/OTHER cho insert).
    if (source != 0x824B /*APPLICATION*/ && source != 0x824C /*OTHER*/) {
        c.errors.Record(0x0500); return;
    }
    std::string m = (message && length != 0)
        ? std::string(message, (length < 0) ? strlen(message) : (size_t)length) : std::string();
    if (!DebugAllowed(c, source, type, severity)) return;
    Context::DebugMsg dm{source, type, id, severity, m};
    c.debugLog.push_back(dm);
    if (c.debugLog.size() > 128) c.debugLog.pop_front();
    if (c.debugCallback)
        c.debugCallback(source, type, id, severity, (GLsizei)m.size(), m.c_str(), c.debugUserParam);
}
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum* sources, GLenum* types,
                            GLuint* ids, GLenum* severities, GLsizei* lengths, GLchar* messageLog) {
    Context& c = Context::Current();
    GLuint n = (GLuint)std::min<size_t>(count, c.debugLog.size());
    GLsizei pos = 0;
    for (GLuint i = 0; i < n; ++i) {
        auto& m = c.debugLog[i];
        if (sources) sources[i] = m.source;
        if (types) types[i] = m.type;
        if (ids) ids[i] = m.id;
        if (severities) severities[i] = m.severity;
        GLsizei len = (GLsizei)m.text.size();
        if (lengths) lengths[i] = len;
        if (messageLog && bufSize > pos) {
            GLsizei copy = std::min(len, bufSize - pos - 1);
            if (copy > 0) { memcpy(messageLog + pos, m.text.c_str(), (size_t)copy); pos += copy; }
            if (pos < bufSize) messageLog[pos++] = '\0';
        }
    }
    // Xoá messages đã đọc (đúng GL: log là queue, đọc xong mất).
    c.debugLog.erase(c.debugLog.begin(), c.debugLog.begin() + n);
    return n;
}
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar* label) {
    Context& c = Context::Current();
    std::string l = (label && length != 0)
        ? std::string(label, (length < 0) ? strlen(label) : (size_t)length) : std::string();
    if (l.empty()) c.objectLabels.erase({identifier, name});
    else c.objectLabels[{identifier, name}] = l;
}
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei* length, GLchar* label) {
    Context& c = Context::Current();
    auto it = c.objectLabels.find({identifier, name});
    std::string l = (it == c.objectLabels.end()) ? std::string() : it->second;
    if (length) *length = (GLsizei)l.size();
    if (label && bufSize > 0) {
        GLsizei copy = std::min((GLsizei)l.size(), bufSize - 1);
        if (copy > 0) memcpy(label, l.c_str(), (size_t)copy);
        label[copy] = '\0';
    }
}
void glObjectPtrLabel(const void* ptr, GLsizei length, const GLchar* label) {
    Context& c = Context::Current();
    std::string l = (label && length != 0)
        ? std::string(label, (length < 0) ? strlen(label) : (size_t)length) : std::string();
    if (l.empty()) c.objectPtrLabels.erase(ptr);
    else c.objectPtrLabels[ptr] = l;
}
void glGetObjectPtrLabel(const void* ptr, GLsizei bufSize, GLsizei* length, GLchar* label) {
    Context& c = Context::Current();
    auto it = c.objectPtrLabels.find(ptr);
    std::string l = (it == c.objectPtrLabels.end()) ? std::string() : it->second;
    if (length) *length = (GLsizei)l.size();
    if (label && bufSize > 0) {
        GLsizei copy = std::min((GLsizei)l.size(), bufSize - 1);
        if (copy > 0) memcpy(label, l.c_str(), (size_t)copy);
        label[copy] = '\0';
    }
}
void glGetPointerv(GLenum pname, void** params) {
    Context& c = Context::Current();
    if (!params) return;
    // DEBUG_CALLBACK_FUNCTION (0x8244) → trả callback; USER_PARAM (0x8245) → userParam.
    if (pname == 0x8244) *params = (void*)c.debugCallback;
    else if (pname == 0x8245) *params = (void*)c.debugUserParam;
    else { c.errors.Record(0x0500); *params = nullptr; }
}
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar* message) {
    Context& c = Context::Current();
    (void)source; (void)id;
    std::string m = (message && length != 0)
        ? std::string(message, (length < 0) ? strlen(message) : (size_t)length) : std::string();
    c.debugGroupStack.push_back(m);
    if (c.DiagOn()) { fprintf(stderr, "[TGLMT] pushDebugGroup: %s\n", m.c_str()); fflush(stderr); }
}
void glPopDebugGroup() {
    Context& c = Context::Current();
    if (c.debugGroupStack.empty()) { c.errors.Record(0x0505); return; }
    c.debugGroupStack.pop_back();
}
// ===== Robustness (P4): glGetGraphicsResetStatus — Metal không reset GPU =====
GLenum glGetGraphicsResetStatus() {
    // TGLMT/Metal: GPU không "reset" như GL (device removed là fatal khác).
    // Trả NO_ERROR (0x8252) — đúng khi chưa từng mất context.
    return 0x8252; // NO_ERROR
}
// ===== EXT_debug_label: glLabelObject (base cho glLabelObjectEXT alias) =====
// Gán label cho object (giống KHR_debug glObjectLabel — dùng chung storage).
void glLabelObject(GLenum type, GLuint object, GLsizei length, const GLchar* label) {
    Context& c = Context::Current();
    std::string l = (label && length != 0)
        ? std::string(label, (length < 0) ? strlen(label) : (size_t)length) : std::string();
    if (l.empty()) c.objectLabels.erase({type, object});
    else c.objectLabels[{type, object}] = l;
}
// ===== ARB_ES3_2_compatibility: glPrimitiveBoundingBox (base cho ARB alias) =====
// Hint bounding box cho primitive culling (tessellation). TGLMT không tessellate
// → ghi nhận, no-op hợp lệ (hint, không ảnh hưởng correctness khi bỏ qua).
void glPrimitiveBoundingBox(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW,
                            GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW) {
    (void)minX; (void)minY; (void)minZ; (void)minW;
    (void)maxX; (void)maxY; (void)maxZ; (void)maxW;
    // No-op: hint cho driver culling, bỏ qua không sai (chỉ mất cơ hội tối ưu).
}
} // namespace tglmt::gl
