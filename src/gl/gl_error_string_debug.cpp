// gl_error_string_debug.cpp — glGetError (CPU queue), glGetString(i), debug callback, KHR_no_error.
// Spec: glspec46.core.pdf §2.3 + KHR_no_error.txt (không hàm mới, chỉ context flag).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
using namespace tglmt;

namespace tglmt::gl {
GLenum glGetError() { return Context::Current().errors.GetError(); }
const GLubyte* glGetString(GLenum n) {
    static const GLubyte kVendor[] = "TGLMT";
    static const GLubyte kRenderer[] = "Apple GPU (Metal)";
    static const GLubyte kVersion[] = "4.6 (TGLMT Metal port)";
    static const GLubyte kSL[] = "4.60 (TGLMT MSL)";
    static const GLubyte kEmpty[] = "";
    switch (n) {
        case 0x1F00: return kVendor;    // VENDOR
        case 0x1F01: return kRenderer;  // RENDERER
        case 0x1F02: return kVersion;   // VERSION
        case 0x8B8C: return kSL;        // SHADING_LANGUAGE_VERSION
        default: Context::Current().errors.Record(0x0500); return kEmpty;
    }
}
// Danh sách extension: CỐ TÌNH giữ nguyên 6 mục cũ.
// KHÔNG thêm GL_ARB_direct_state_access / GL_ARB_vertex_attrib_binding /
// GL_ARB_buffer_storage dù TGLMT "làm được": LWJGL dựng Capabilities từ
// ĐÚNG danh sách string này, nên thêm vào sẽ đảo quyết định của Minecraft:
//   DirectStateAccess.Core  (glNamedBufferSubData thay glBufferSubData)
//   VertexArrayCache.Separate (glVertexAttribFormat/glBindVertexBuffer)
//   BufferStorage.Immutable    (persistent mapping UBO/terrain)
// Cả 3 đường đó chưa được kiểm chứng trên TGLMT và làm MC treo/mất dữ liệu
// (đo được: chỉ 12 frame rồi đứng, uboMap 0 → 48). Danh sách phải khớp với
// đường nào đã test, không phải đường nào "lý thuyết" hỗ trợ được.
static const char* const kExts[] = {
    "GL_ARB_gl_spirv",
    "GL_ARB_indirect_parameters",
    "GL_ARB_pipeline_statistics_query",
    "GL_ARB_polygon_offset_clamp",
    "GL_ARB_texture_filter_anisotropic",
    "GL_KHR_no_error",
};
static const GLuint kNumExts = (GLuint)(sizeof(kExts) / sizeof(kExts[0]));
const GLubyte* glGetStringi(GLenum n, GLuint i) {
    if (n == 0x1F03) { // EXTENSIONS
        if (i < kNumExts) return (const GLubyte*)kExts[i];
        return nullptr;
    }
    Context::Current().errors.Record(0x0500);
    return nullptr;
}
void glDebugMessageCallback(TGLMTDebugProc cb, const void* u) {
    Context& c = Context::Current();
    c.debugCb = (decltype(c.debugCb))cb; c.debugUser = u;
}
void glDebugMessageControl(GLenum s, GLenum t, GLenum sev, GLsizei n, const GLuint* ids, GLboolean on) {
    (void)s;(void)t;(void)sev;(void)n;(void)ids;(void)on;
}
void glDebugMessageInsert(GLenum s, GLenum t, GLuint id, GLenum sev, GLsizei len, const GLchar* msg) {
    Context& c = Context::Current();
    std::string m(msg ? msg : "", len < 0 ? strlen(msg ? msg : "") : (size_t)len);
    c.LogDebug(s, t, id, sev, m);
}
void glPushDebugGroup(GLenum s, GLuint id, GLsizei len, const GLchar* msg) { (void)s;(void)id;(void)len;(void)msg; }
void glPopDebugGroup() {}
void glObjectLabel(GLenum t, GLuint o, GLsizei len, const GLchar* l) { (void)t;(void)o;(void)len;(void)l; }
void glObjectPtrLabel(const void* p, GLsizei len, const GLchar* l) { (void)p;(void)len;(void)l; }
void glGetObjectLabel(GLenum t, GLuint o, GLsizei n, GLsizei* l, GLchar* b) { (void)t;(void)o;(void)n;(void)l;(void)b; }
void glGetObjectPtrLabel(const void* p, GLsizei n, GLsizei* l, GLchar* b) { (void)p;(void)n;(void)l;(void)b; }
GLenum glGetGraphicsResetStatus() { return 0x8252; } // NO_ERROR (GL 4.5 robustness)
void glGetPointerv(GLenum p, void** v) { (void)p; *v = nullptr; }
} // namespace tglmt::gl
namespace tglmt {
GLuint tglmt_num_extensions() { return gl::kNumExts; }
} // namespace tglmt
