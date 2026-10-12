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
// Danh sách extension: CỐ TÌNH giữ nguyên các mục cũ.
// KHÔNG thêm GL_ARB_direct_state_access / GL_ARB_vertex_attrib_binding /
// GL_ARB_buffer_storage dù TGLMT "làm được": LWJGL dựng Capabilities từ
// ĐÚNG danh sách string này, nên thêm vào sẽ đảo quyết định của Minecraft:
//   DirectStateAccess.Core  (glNamedBufferSubData thay glBufferSubData)
//   VertexArrayCache.Separate (glVertexAttribFormat/glBindVertexBuffer)
//   BufferStorage.Immutable    (persistent mapping UBO/terrain)
// Cả 3 đường đó chưa được kiểm chứng trên TGLMT và làm MC treo/mất dữ liệu
// (đo được: chỉ 12 frame rồi đứng, uboMap 0 → 48). Danh sách phải khớp với
// đường nào đã test, không phải đường nào "lý thuyết" hỗ trợ được.
//
// ĐÃ BỎ GL_ARB_pipeline_statistics_query (P0.3): extension này bắt buộc mọi
// target query phải trả kết quả thật, trong đó có GL_FRAGMENT_SHADER_INVOCATIONS_
// ARB — Metal không có counter nào đếm được số fragment shader invocation. TGLMT
// vẫn ĐẾM thật các target tính được (vertex/primitive/VS-invocation, xem
// Context::PipeStats) và glBeginQuery vẫn nhận các target đó, nhưng KHÔNG quảng
// cáo extension cho tới khi đường counter GPU (P5) chạy thật. Quảng cáo extension
// rồi trả 0 cho fragment invocations = nói dối, đúng thứ hạng "No-op accepted"
// mà vocabulary docs cấm.
static const char* const kExts[] = {
    "GL_ARB_gl_spirv",
    "GL_ARB_indirect_parameters",
    "GL_ARB_polygon_offset_clamp",
    "GL_ARB_texture_filter_anisotropic",
    "GL_KHR_no_error",
    // P3 batch-1: alias-ready — mọi hàm = alias của core-đã-implement
    // (glXxxARB forward 1:1 sang glXxx, xem gl_ext_aliases_gen.cpp).
    // Mirror core-promoted → MC dùng core path, không flip sang path chưa verify.
    "GL_ARB_color_buffer_float",
    "GL_ARB_draw_buffers",
    "GL_ARB_draw_instanced",
    "GL_ARB_instanced_arrays",
    "GL_ARB_multisample",
    "GL_ARB_occlusion_query",
    "GL_ARB_point_parameters",
    "GL_ARB_sample_shading",
    "GL_ARB_texture_buffer_object",
    "GL_ARB_vertex_buffer_object",
    // P3 batch-2: alias + purpose-safe + core-base KHÔNG no-op.
    // Loại geometry/tess/buffer_storage (capability không hỗ trợ thật),
    // loại ext có core-base no-op (texture_compression/copy_texture/fbo-
    // renderbuffer/texture3D/get_program_binary) — advertise capability no-op
    // = nói dối. 47 ext mirror core functionality đang chạy.
    "GL_APPLE_sync",
    "GL_APPLE_vertex_array_object",
    "GL_ARB_draw_buffers_blend",
    "GL_EXT_base_instance",
    "GL_EXT_blend_color",
    "GL_EXT_blend_equation_separate",
    "GL_EXT_blend_func_separate",
    "GL_EXT_blend_minmax",
    "GL_EXT_clip_control",
    "GL_EXT_copy_image",
    "GL_EXT_disjoint_timer_query",
    "GL_EXT_draw_buffers",
    "GL_EXT_draw_buffers_indexed",
    "GL_EXT_draw_elements_base_vertex",
    "GL_EXT_draw_instanced",
    "GL_EXT_draw_range_elements",
    "GL_EXT_draw_transform_feedback",
    "GL_EXT_framebuffer_blit",
    "GL_EXT_instanced_arrays",
    "GL_EXT_map_buffer_range",
    "GL_EXT_multi_draw_arrays",
    "GL_EXT_multi_draw_indirect",
    "GL_EXT_occlusion_query_boolean",
    "GL_EXT_point_parameters",
    "GL_EXT_polygon_offset",
    "GL_EXT_polygon_offset_clamp",
    "GL_EXT_provoking_vertex",
    "GL_EXT_subtexture",
    "GL_EXT_texture_array",
    "GL_EXT_texture_border_clamp",
    "GL_EXT_texture_buffer",
    "GL_EXT_texture_buffer_object",
    "GL_EXT_texture_view",
    "GL_EXT_timer_query",
    "GL_OES_blend_equation_separate",
    "GL_OES_blend_func_separate",
    "GL_OES_blend_subtract",
    "GL_OES_copy_image",
    "GL_OES_draw_buffers_indexed",
    "GL_OES_draw_elements_base_vertex",
    "GL_OES_mapbuffer",
    "GL_OES_sample_shading",
    "GL_OES_texture_border_clamp",
    "GL_OES_texture_buffer",
    "GL_OES_texture_storage_multisample_2d_array",
    "GL_OES_texture_view",
    "GL_OES_vertex_array_object",
    // P3 batch-3: enum-only exts — behavior = core đã verify thật, enums đã define.
    // (bind map generic → COPY_READ/WRITE OK; indirect đọc buffer OK; map/UBO/
    // sampler/texStorage/VAO core OK; UNSIGNED_INT index handle OK.)
    // LOẠI enum-ext behavior chưa implement: depth_clamp/seamless/sRGB/stencil-
    // texturing (enable không handle), texture_swizzle (không apply), rgtc (không
    // decode), occlusion ANY_SAMPLES (không handle), internalformat_query (trả 0),
    // vertex_attrib_64bit (double không render), clear_texture/get_program_binary
    // (no-op), half_float_vertex (HALF_FLOAT không handle) → nói dối nếu advertise.
    "GL_ARB_copy_buffer",
    "GL_ARB_draw_indirect",
    "GL_ARB_map_buffer_range",
    "GL_ARB_sampler_objects",
    "GL_ARB_texture_storage",
    "GL_ARB_uniform_buffer_object",
    "GL_ARB_vertex_array_object",
    "GL_OES_element_index_uint",
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
