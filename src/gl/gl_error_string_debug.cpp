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
    "GL_ARB_copy_buffer",
    "GL_ARB_draw_indirect",
    "GL_ARB_map_buffer_range",
    "GL_ARB_sampler_objects",
    "GL_ARB_texture_storage",
    "GL_ARB_uniform_buffer_object",
    "GL_ARB_vertex_array_object",
    "GL_OES_element_index_uint",
    // P3 batch-4: behavior ĐÃ implement (xem gl_get_extra/gl_apple_draw):
    //  - internalformat_query(_2): glGetInternalformativ trả data THẬT (format
    //    supported/renderable/component size), 0 cho pname chưa implement.
    //  - depth_clamp: glEnable(0x864F) → encoder setDepthClipMode Clamp/Clip.
    "GL_ARB_internalformat_query",
    "GL_ARB_internalformat_query2",
    "GL_ARB_depth_clamp",
    // P3 batch-5: vertex-format behaviors — ToMTLVertexFormat map THẬT
    // (xem AppleMetalBridge.mm):
    //  - half_float_vertex: HALF_FLOAT (0x140B) → MTLVertexFormatHalf2/3/4,
    //    GLTypeSize stride=2. (Trước bị loại nhầm do grep sai chỗ.)
    //  - vertex_type_2_10_10_10_rev: INT_2_10_10_10_REV/UINT_2_10_10_10_REV
    //    (size=4, norm) → MTLVertexFormatInt/UInt1010102Normalized.
    //  - vertex_array_bgra (+EXT): type=GL_BGRA size=4 norm →
    //    MTLVertexFormatUChar4Normalized_BGRA.
    "GL_ARB_half_float_vertex",
    "GL_ARB_vertex_type_2_10_10_10_rev",
    "GL_ARB_vertex_array_bgra",
    "GL_EXT_vertex_array_bgra",
    // P3 batch-6: texture_swizzle ĐÃ implement (trước đây loại — sampler không
    // swizzle được trên Metal; giờ dùng texture-view swizzle, xem AppleTexture::
    // swizzledView + SwzOrBase trong gl_apple_draw): glTexParm decode swz[] →
    // bind lúc draw qua MTLTextureSwizzle; glGetTexParameteriv(0x8E46) round-trip.
    "GL_EXT_texture_swizzle",
    // P3 batch-7: get_program_binary ĐÃ implement (gl_program.cpp): binary =
    // GLSL gốc; glGetProgramBinary serialize, glProgramBinary parse + relink
    // (location sinh lại đúng khi link). Format lạ → INVALID_ENUM trung thực.
    // glGetIntegerv(NUM/FORMATS) + glGetProgramiv(LENGTH) trả thật.
    "GL_ARB_get_program_binary",
    // ===== P3 batch-8: 20 ext — behavior ĐÃ verify thật trước khi advertise. =====
    // (a) Wrap: ToMTLWrap thêm case 0x8743 → MTLSamplerAddressModeMirrorClampToEdge
    "GL_ARB_texture_mirror_clamp_to_edge",
    "GL_EXT_texture_mirror_clamp_to_edge",
    // (b) ARB_swizzle = hành vi EXT_swizzle vừa implement (bind view swizzled)
    "GL_ARB_texture_swizzle",
    // (c) API-side, hàm thật:
    "GL_ARB_draw_elements_base_vertex",
    "GL_ARB_copy_image",
    "GL_ARB_get_texture_sub_image",
    "GL_ARB_multi_draw_indirect",
    "GL_ARB_texture_buffer_range",
    "GL_ARB_vertex_attrib_binding",
    "GL_ARB_sync",
    "GL_ARB_timer_query",
    "GL_NV_polygon_mode",
    // (d) Alias ext — symbol extern "C" + hàm core thật (gl_ext_aliases_gen):
    "GL_MESA_sampler_objects",
    "GL_AMD_multi_draw_indirect",
    "GL_NV_copy_buffer",
    "GL_NV_copy_image",
    "GL_NV_draw_instanced",
    "GL_NV_draw_buffers",
    "GL_NV_framebuffer_blit",
    "GL_NV_instanced_arrays",
    "GL_NV_non_square_matrices",
    "GL_ATI_draw_buffers",
    "GL_OES_get_program_binary",
    // ===== P3 batch-9: robustness + separate stencil — hàm thật đã verify. =====
    //  - glGetnUniform*/glReadnPixels: đọc CÓ GIỚI HẠN bufSize (thực thi guard
    //    0x0501/0x0502 thật) — đây là cốt lõi robustness, làm thật được.
    //  - glGetGraphicsResetStatus trả NO_ERROR (0x8252): Metal không reset GPU
    //    → trung thực "chưa từng reset".
    //  - glGetnColorTable/Histogram/Minmap/Map*: điếm core (đã remove) → trả 0
    //    trung thực thay vì crash.
    "GL_ARB_robustness",
    "GL_KHR_robustness",
    "GL_EXT_robustness",
    //  - glStencilFuncSeparateATI/glStencilOpSeparateATI → core separate stencil
    //    (StencilFront/Back state thật, đã render được).
    "GL_ATI_separate_stencil",
    // ===== P3 batch-10: enum-only ext có behavior thật. =====
    //  - occlusion_query2: ANY_SAMPLES_PASSED ĐÃ implement (visibility result)
    "GL_ARB_occlusion_query2",
    //  - multi_bind: glBindBuffersBase/Range loop glBindBufferBase thật
    "GL_ARB_multi_bind",
    //  - map_buffer_alignment: glGet(MIN_MAP_BUFFER_ALIGNMENT)=16 thật
    "GL_ARB_map_buffer_alignment",
    //  - explicit_uniform_location: glGet(MAX_UNIFORM_LOCATIONS)=1024 thật
    "GL_ARB_explicit_uniform_location",
    //  - clear_texture: glClearTexImage/ glClearTexSubImage ĐÃ điền shadow +
    //    SyncRegionToGPU (format/type UNSIGNED_BYTE/FLOAT/HALF_FLOAT/USHORT/UINT)
    "GL_EXT_clear_texture",
    "GL_ARB_clear_texture",
    //  - clip_control: SetClipControl + áp viewport Y-flip / clear depth /
    //    shader depth map (ClipOrigin/ClipDepth được consume) + glGet round-trip
    "GL_ARB_clip_control",
    // ===== P6f: compute shader (ARB_compute_shader) — dispatch THẬT trên Metal. =====
    //  - converter GLSL→kernel MSL (localSize + gl_*ID + SSBO/uniform/texture) +
    //    link tạo compute pipeline + glDispatchCompute → dispatchThreads (e2e 64/64 PASS).
    //  - glGet limit compute (COUNT/SIZE indexed + SHARED/UNIFORM/TEXTURE/INVOC) thật.
    //  - atomic counters / image uniforms = 0 (chưa impl — app fallback đường khác).
    "GL_ARB_compute_shader",
    // ===== P6g: image load/store (ARB_shader_image_load_store) — texel RW THẬT. =====
    //  - converter layout(format,binding) image2D/Array/3D → texture RW MSL
    //    (imageLoad/Store/Size → .read/.write/get_*) + backend glBindImageTexture
    //    + bind ở dispatch/draw (e2e compute imageStore→texture PASS).
    //  - imageCube/Buffer/1D/Rect/MS + imageAtomic* → fail trung thực (MSL không có).
    //  - glGet MAX_IMAGE_UNITS/COMBINED/VERTEX/FRAGMENT/COMPUTE_IMAGE_UNIFORMS thật.
    "GL_ARB_shader_image_load_store",
    // ===== P4: debug + discard + parallel-compile — behavior THẬT, test verify. =====
    //  - KHR_debug: callback/control/insert/log/labels/groups (ring 128, filter rules).
    //  - EXT_debug_marker: push/pop/insert (stack, log vào KHR_debug).
    //  - EXT_discard_framebuffer: validate + no-op hint (tile memory, TGLMT quản lý显式).
    //  - ARB_parallel_shader_compile: MaxShaderCompilerThreadsARB hint (ghi nhận).
    "GL_KHR_debug",
    "GL_EXT_debug_marker",
    "GL_EXT_discard_framebuffer",
    "GL_ARB_parallel_shader_compile",
    // ===== P4/P5 easy wins (1-2 hàm, behavior thật, test verify). =====
    //  - ARB_compute_variable_group_size: DispatchComputeGroupSizeARB (override local_size).
    //  - EXT_debug_label: LabelObjectEXT (dùng chung storage với KHR_debug ObjectLabel).
    //  - ARB_ES3_2_compatibility: PrimitiveBoundingBoxARB (hint, no-op hợp lệ).
    "GL_ARB_compute_variable_group_size",
    "GL_EXT_debug_label",
    "GL_ARB_ES3_2_compatibility",
    // ===== P5: buffer storage (ARB_buffer_storage) — immutable buffer THẬT. =====
    //  - glBufferStorage (+Named) ĐÃ implement (immutable, flags, GPU upload).
    "GL_ARB_buffer_storage",
    // ===== P5: direct state access (ARB_direct_state_access) — 97 hàm THẬT. =====
    //  - Texture/buffer/framebuffer/VAO/sampler/query DSA ĐÃ implement đầy đủ
    //    (không bind, thao tác trực tiếp object). Verify bằng coverage + test.
    "GL_ARB_direct_state_access",
    // LOẠI enum-ext behavior CHƯA implement đúng (nói dối nếu advertise):
    //  seamless_cube_map (cần shader), framebuffer_sRGB (cần format-toggle),
    //  stencil_texturing, rgtc (không decode — glCompressedTexImage2D no-op),
    //  occlusion ANY_SAMPLES (cần GPU query), clear_texture (no-op),
    //  vertex_attrib_64bit (Double không render),
    //  vertex_type_10f_11f_11f_rev (Metal không có Float101111) → P4/P7.
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
} // namespace tglmt::gl
namespace tglmt {
GLuint tglmt_num_extensions() { return gl::kNumExts; }
} // namespace tglmt
