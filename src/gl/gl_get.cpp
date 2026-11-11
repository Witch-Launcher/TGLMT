// gl_get.cpp — Mọi glGet* đọc shadow CPU (Metal không cho query GPU đồng bộ).
// Nguyên tắc: Set* đã lưu shadow → Get* trả shadow; chưa Set → giá trị mặc định spec.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/glsl_type.h"
#include "tglmt/UniformQuery.h"
#include "tglmt/limits.h"
#include "tglmt/gl46_pnames.h"
#include <algorithm>
#include <cstring>
#include <vector>
using namespace tglmt;
namespace tglmt::gl { GLint ProgramBinaryLength(GLuint p); } // gl_program.cpp
// Danh sách format nén TGLMT hỗ trợ (gl_texture.cpp) — phục vụ query
// GL_NUM_COMPRESSED_TEXTURE_FORMATS / GL_COMPRESSED_TEXTURE_FORMATS.
namespace tglmt::gl { const std::vector<GLenum>& CompressedTextureFormats(); }

// ARB_parallel_shader_compile: TGLMT dịch GLSL→MSL và biên dịch ĐỒNG BỘ tại
// link (không có pool thread) → "số thread biên dịch" = giá trị app đặt, tối
// thiểu 1. Spec ghi initial 0xFFFFFFFF ("implementation tự quyết"), nhưng
// trả -1 khiến app tưởng không có thread nào → trả 1 (đúng thực tế TGLMT).
static GLint MaxShaderCompilerThreads(const Context& c) {
    return c.maxShaderCompilerThreads > 0 ? (GLint)c.maxShaderCompilerThreads : 1;
}

// Trạng thái blend của draw buffer `idx` (clamp về phạm vi hợp lệ).
static const tglmt::BlendState& BlendAt(Context& c, GLuint idx) {
    const auto& B = c.state.Blend();
    return B[idx < B.size() ? idx : 0];
}

static void OutI(Context& c, GLenum p, GLint* v, GLuint idx = 0) {
    GLint t = 0;
    // GL_SCISSOR_BOX (0x0C10) là 4 giá trị (x, y, w, h) — đọc shadow 16 byte
    // thay vì 4 byte để glGetIntegeri_v trả đủ box (spec §14.9.2).
    if (p == 0x0C10) {
        GLint box4[4] = {0, 0, 0, 0};
        if (c.state.GetShadow(p, box4, sizeof(box4), idx)) {
            v[0] = box4[0]; v[1] = box4[1]; v[2] = box4[2]; v[3] = box4[3];
            return;
        }
    }
    // §17.3.2: trạng thái blend là PER DRAW BUFFER → glGetIntegeri_v phải đọc
    // theo index (trước đây luôn đọc buffer 0 ⇒ app đọc sai khi blend khác
    // nhau cho từng draw buffer).
    switch (p) {
        case 0x80C9: *v = (GLint)BlendAt(c, idx).srcRGB; return;
        case 0x80C8: *v = (GLint)BlendAt(c, idx).dstRGB; return;
        case 0x80CB: *v = (GLint)BlendAt(c, idx).srcAlpha; return;
        case 0x80CA: *v = (GLint)BlendAt(c, idx).dstAlpha; return;
        case 0x0BE1: *v = (GLint)BlendAt(c, idx).srcRGB; return;
        case 0x0BE0: *v = (GLint)BlendAt(c, idx).dstRGB; return;
        case 0x8009: *v = (GLint)BlendAt(c, idx).rgbEq; return;
        case 0x883D: *v = (GLint)BlendAt(c, idx).alphaEq; return;
        default: break;
    }
    if (c.state.GetShadow(p, &t, 4, idx)) { *v = t; return; }
    switch (p) {
        case 0x0D33: *v = 4; break; // MAX_TEXTURE_SIZE default expose
        case 0x0B70: *v = 0; break;
        case 0x8E7D: *v = 32; break; // MAX_PATCH_VERTICES (GL 4.6 minimum)
        case 0x8E7E: *v = (GLint)(c.device ? c.device->maxTessellationFactor() : 16); break;
        case 0x886C: *v = 120; break; // MAX_TESS_CONTROL_INPUT_COMPONENTS
        case 0x8E83: *v = 120; break; // MAX_TESS_CONTROL_OUTPUT_COMPONENTS
        case 0x8E85: *v = 3960; break; // MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS
        case 0x886D: *v = 120; break; // MAX_TESS_EVALUATION_INPUT_COMPONENTS
        case 0x8E86: *v = 120; break; // MAX_TESS_EVALUATION_OUTPUT_COMPONENTS
        case 0x8E84: *v = 120; break; // MAX_TESS_PATCH_COMPONENTS
        // P6f compute indexed limits (ARB_compute_shader): theo dimension (idx 0/1/2).
        case 0x91BE: // MAX_COMPUTE_WORK_GROUP_COUNT
            *v = (idx == 0) ? 65535 : (idx == 1) ? 65535 : 65535; break;
        case 0x91BF: // MAX_COMPUTE_WORK_GROUP_SIZE
            *v = (idx == 0) ? 1024 : (idx == 1) ? 1024 : 64; break;
        // P4: indexed buffer binding queries — đọc thẳng map bind point thật
        // (glBindBufferBase/Range ghi vào đây), không nói dối sau khi advertise SSBO.
        case 0x8919: *v = (GLint)c.state.BoundSampler(idx); break; // SAMPLER_BINDING (glGetIntegeri_v)
        case 0x90D3: { auto it = c.storageBindPoints.find(idx); // SHADER_STORAGE_BUFFER_BINDING
            *v = (it == c.storageBindPoints.end()) ? 0 : (GLint)it->second.buffer; break; }
        case 0x90D4: { auto it = c.storageBindPoints.find(idx); // SHADER_STORAGE_BUFFER_START
            *v = (it == c.storageBindPoints.end()) ? 0 : (GLint)it->second.offset; break; }
        case 0x90D5: { auto it = c.storageBindPoints.find(idx); // SHADER_STORAGE_BUFFER_SIZE
            // size=0 = BindBufferBase (toàn bộ buffer) → trả size buffer thật.
            if (it == c.storageBindPoints.end()) *v = 0;
            else if (it->second.size) *v = (GLint)it->second.size;
            else {
                auto bt = c.buffers.find(it->second.buffer);
                *v = (bt == c.buffers.end()) ? 0 : (GLint)BufferLen(bt->second);
            }
            break; }
        case 0x8A28: { auto it = c.uniformBindPoints.find(idx); // UNIFORM_BUFFER_BINDING
            *v = (it == c.uniformBindPoints.end()) ? 0 : (GLint)it->second.buffer; break; }
        case 0x8A29: { auto it = c.uniformBindPoints.find(idx); // UNIFORM_BUFFER_START
            *v = (it == c.uniformBindPoints.end()) ? 0 : (GLint)it->second.offset; break; }
        case 0x8A2A: { auto it = c.uniformBindPoints.find(idx); // UNIFORM_BUFFER_SIZE
            if (it == c.uniformBindPoints.end()) *v = 0;
            else if (it->second.size) *v = (GLint)it->second.size;
            else {
                auto bt = c.buffers.find(it->second.buffer);
                *v = (bt == c.buffers.end()) ? 0 : (GLint)BufferLen(bt->second);
            }
            break; }
        case 0x92C1: { auto it = c.atomicBindPoints.find(idx); // ATOMIC_COUNTER_BUFFER_BINDING
            *v = (it == c.atomicBindPoints.end()) ? 0 : (GLint)it->second.buffer; break; }
        case 0x92C2: { auto it = c.atomicBindPoints.find(idx); // ATOMIC_COUNTER_BUFFER_START
            *v = (it == c.atomicBindPoints.end()) ? 0 : (GLint)it->second.offset; break; }
        case 0x8E52: *v = (GLint)0xFFFFFFFFu; break; // SAMPLE_MASK_VALUE mặc định
        case 0x92C3: { auto it = c.atomicBindPoints.find(idx); // ATOMIC_COUNTER_BUFFER_SIZE
            if (it == c.atomicBindPoints.end()) *v = 0;
            else if (it->second.size) *v = (GLint)it->second.size;
            else {
                auto bt = c.buffers.find(it->second.buffer);
                *v = (bt == c.buffers.end()) ? 0 : (GLint)BufferLen(bt->second);
            }
            break; }
        // glGetIntegeri_v(GL_BLEND, i) — indexed blend enable (spec §4.1.2 /
        // ARB_draw_buffers_blend). OutI cũ default *v=0 → query LUÔN false dù
        // glEnablei(GL_BLEND,i) đã bật → app (Sodium/MC) tắt blend nhầm.
        case 0x0BE2: *v = c.state.IsEnabled(0x0BE2, idx) ? 1 : 0; break;
        // COMPRESSED_TEXTURE_FORMATS là mảng (GetIntegerv), nhưng app hay đọc
        // bằng glGetIntegeri_v → trả phần tử thứ idx thay vì 0.
        case 0x86A3: {
            const std::vector<GLenum>& fmts = tglmt::gl::CompressedTextureFormats();
            *v = (idx < fmts.size()) ? (GLint)fmts[idx] : 0;
            break;
        }
        // GL_LOGIC_OP mặc định = GL_COPY (0x1503) (spec §17.4.3). GetShadow ở
        // trên đã trả giá trị nếu glLogicOp đã gọi; chưa gọi ⇒ COPY.
        case 0x0BF1: *v = 0x1503; break;
        // GL_SAMPLE_ALPHA_TO_COVERAGE không indexed; nhưng glGetBooleani_v
        // cũng đi qua đây cho các pname indexed khác.
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
        // CONTEXT_FLAGS (0x821E): không debug bit → 0. CONTEXT_PROFILE_MASK (0x9126):
        // TGLMT là GL 4.6 CORE (không compatibility) → bắt buộc trả CORE_PROFILE_BIT
        // (0x1). Trả 0 là nói dối: app check mask để chọn core/compat path sẽ thấy
        // "không phải core" rồi rơi vào nhánh compat không kiểm chứng.
        case 0x821E: *v = 0; break;      // CONTEXT_FLAGS (không debug bit)
        case 0x9126: *v = 0x00000001; break; // CONTEXT_PROFILE_MASK → CORE_PROFILE_BIT
        case 0x0D33: *v = 8192; break;   // MAX_TEXTURE_SIZE
        case 0x8073: *v = 2048; break;   // MAX_3D_TEXTURE_SIZE
        case 0x851C: *v = 8192; break;   // MAX_CUBE_MAP_TEXTURE_SIZE
        case 0x88FF: *v = 2048; break;   // MAX_ARRAY_TEXTURE_LAYERS
        case 0x84E8: *v = 8192; break;   // MAX_RENDERBUFFER_SIZE
        case 0x84F8: *v = 8192; break;   // MAX_RECTANGLE_TEXTURE_SIZE
        case 0x8CDF: *v = 8; break;      // MAX_COLOR_ATTACHMENTS
        case 0x8824: *v = 8; break;      // MAX_DRAW_BUFFERS (Iris GlFramebuffer check!)
        case 0x88FC: *v = 1; break;      // MAX_DUAL_SOURCE_DRAW_BUFFERS (GL 3.3 core)
        case 0x8869: *v = 16; break;     // MAX_VERTEX_ATTRIBS
        case 0x8872: *v = 32; break;     // MAX_TEXTURE_IMAGE_UNITS
        case 0x8B4C: *v = 16; break;     // MAX_VERTEX_TEXTURE_IMAGE_UNITS
        case 0x8B4D: *v = tglmt::kMaxCombinedTextureImageUnits; break; // MAX_COMBINED_TEXTURE_IMAGE_UNITS
        case 0x8B4A: *v = 4096; break;   // MAX_VERTEX_UNIFORM_COMPONENTS
        case 0x8B49: *v = 4096; break;   // MAX_FRAGMENT_UNIFORM_COMPONENTS
        case 0x8B4B: *v = 128; break;    // MAX_VARYING_COMPONENTS
        case 0x8D57: *v = 4; break;      // MAX_SAMPLES (A11 4x MSAA)
        case 0x8E59: *v = 1; break;      // MAX_SAMPLE_MASK_WORDS
        case 0x910E: *v = 4; break;      // MAX_COLOR_TEXTURE_SAMPLES
        case 0x910F: *v = 4; break;      // MAX_DEPTH_TEXTURE_SAMPLES
        case 0x9110: *v = 4; break;      // MAX_INTEGER_SAMPLES
        // ===== ARB_sample_locations (Metal: getDefaultSamplePositions 1/16 px) =====
        case 0x933D: *v = 4; break;      // SAMPLE_LOCATION_SUBPIXEL_BITS_ARB
        case 0x933E: *v = 1; break;      // SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB
        case 0x933F: *v = 1; break;      // SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB
        case 0x9340: *v = 32; break;     // PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_ARB
        // ===== GL 4.1 / ARB_ES2_compatibility queries =====
        case 0x8DFA: *v = 1; break;      // SHADER_COMPILER
        case 0x8DF9: *v = 0; break;      // NUM_SHADER_BINARY_FORMATS (không binary format)
        case 0x8DFB: *v = 1024; break;   // MAX_VERTEX_UNIFORM_VECTORS
        case 0x8DFC: *v = 32; break;     // MAX_VARYING_VECTORS
        case 0x8DFD: *v = 1024; break;   // MAX_FRAGMENT_UNIFORM_VECTORS
        case 0x8B9A: *v = 0x1401; break; // IMPLEMENTATION_COLOR_READ_TYPE = UNSIGNED_BYTE
        case 0x8B9B: *v = 0x1908; break; // IMPLEMENTATION_COLOR_READ_FORMAT = RGBA
        case 0x8A2F: *v = 36; break;     // MAX_UNIFORM_BUFFER_BINDINGS
        case 0x8A30: *v = 65536; break;  // MAX_UNIFORM_BLOCK_SIZE
        case 0x8A34: *v = 256; break;    // UNIFORM_BUFFER_OFFSET_ALIGNMENT (0 = crash MC)
        case 0x8A2B: *v = 14; break;     // MAX_VERTEX_UNIFORM_BLOCKS
        case 0x8A2D: *v = 14; break;     // MAX_FRAGMENT_UNIFORM_BLOCKS
        case 0x8A2E: *v = 28; break;     // MAX_COMBINED_UNIFORM_BLOCKS
        case 0x8C2B: *v = 65536; break;  // MAX_TEXTURE_BUFFER_SIZE
        case 0x80E8: *v = 1048576; break;// MAX_ELEMENTS_VERTICES
        case 0x80E9: *v = 1048576; break;// MAX_ELEMENTS_INDICES
        // ===== P6f compute (ARB_compute_shader) — limit thật, atomic/image = 0 =====
        case 0x8262: *v = 32768; break;  // MAX_COMPUTE_SHARED_MEMORY_SIZE (32KB A11)
        case 0x8263: *v = 1024; break;   // MAX_COMPUTE_UNIFORM_COMPONENTS
        case 0x8264: *v = 4; break;      // MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS (P9: atomic OK)
        case 0x8265: *v = 8; break;      // MAX_COMPUTE_ATOMIC_COUNTERS (P9: atomic OK)
        case 0x8266: *v = 1024; break;   // MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS
        case 0x90EB: *v = 1024; break;   // MAX_COMPUTE_WORK_GROUP_INVOCATIONS (A11)
        case 0x91BB: *v = 14; break;     // MAX_COMPUTE_UNIFORM_BLOCKS
        case 0x91BC: *v = 16; break;     // MAX_COMPUTE_TEXTURE_IMAGE_UNITS
        case 0x91BD: *v = 8; break;      // MAX_COMPUTE_IMAGE_UNIFORMS (P6g: image OK)
        // ===== P6g image (ARB_shader_image_load_store) =====
        case 0x8F38: *v = tglmt::kMaxImageUnits; break;      // MAX_IMAGE_UNITS
        case 0x90CF: *v = 8; break;      // MAX_COMBINED_IMAGE_UNIFORMS
        case 0x906D: *v = 4; break;      // MAX_IMAGE_SAMPLES
        case 0x90CA: *v = 8; break;      // MAX_VERTEX_IMAGE_UNIFORMS
        case 0x90CE: *v = 8; break;      // MAX_FRAGMENT_IMAGE_UNIFORMS
        // ===== P9 atomic counters (ARB_shader_atomic_counters, vertex/fragment/compute) =====
        case 0x92D2: *v = 8; break;      // MAX_VERTEX_ATOMIC_COUNTERS
        case 0x92D6: *v = 8; break;      // MAX_FRAGMENT_ATOMIC_COUNTERS
        case 0x92CC: *v = 1; break;      // MAX_VERTEX_ATOMIC_COUNTER_BUFFERS
        case 0x92D0: *v = 1; break;      // MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS
        case 0x92D7: *v = 16; break;     // MAX_COMBINED_ATOMIC_COUNTERS
        case 0x92D1: *v = 5; break;      // MAX_COMBINED_ATOMIC_COUNTER_BUFFERS (1+1+... conserv)
        // ===== P6f SSBO (ARB_shader_storage_buffer_object) — binding 0..15 compute =====
        case 0x90DD: *v = 16; break;     // MAX_SHADER_STORAGE_BUFFER_BINDINGS
        case 0x90DE: *v = 134217728; break; // MAX_SHADER_STORAGE_BLOCK_SIZE (128MB)
        case 0x90DF: *v = 4; break;      // SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT
        // ===== P4 SSBO graphics: block limits (graphics 17..30 chung UBO, compute 0..15) =====
        case 0x90D6: *v = 8; break;      // MAX_VERTEX_SHADER_STORAGE_BLOCKS (conservative)
        case 0x90D7: *v = 8; break;      // MAX_GEOMETRY_SHADER_STORAGE_BLOCKS
        case 0x90D8: *v = 8; break;      // MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS
        case 0x90D9: *v = 8; break;      // MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS
        case 0x90DA: *v = 8; break;      // MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
        case 0x90DB: *v = 16; break;     // MAX_COMPUTE_SHADER_STORAGE_BLOCKS
        case 0x90DC: *v = 32; break;     // MAX_COMBINED_SHADER_STORAGE_BLOCKS
        default: return false;
    }
    return true;
}

namespace tglmt::gl {
void glGetIntegerv(GLenum p, GLint* v) {
    Context& c = Context::Current();
    // một số pname đặc biệt trả trực tiếp từ object/state
    switch (p) {
        case 0x85B5: *v = (GLint)c.state.BoundVAO(); return;      // VERTEX_ARRAY_BINDING
        case 0x8B8D: *v = (GLint)c.state.BoundProgram(); return;  // CURRENT_PROGRAM
        case 0x0C11: *v = c.state.IsEnabled(0x0C11) ? 1 : 0; return; // SCISSOR_TEST
        case 0x0B71: *v = c.state.IsEnabled(0x0B71) ? 1 : 0; return; // DEPTH_TEST
        case 0x0BE2: *v = c.state.IsEnabled(0x0BE2) ? 1 : 0; return; // BLEND
        case 0x0B44: *v = c.state.IsEnabled(0x0B44) ? 1 : 0; return; // CULL_FACE
        case 0x0BF2: *v = c.state.IsEnabled(0x0BF2) ? 1 : 0; return; // COLOR_LOGIC_OP
        case 0x0D3A: v[0] = 8192; v[1] = 8192; return;             // MAX_VIEWPORT_DIMS (2 giá trị)
        // --- Core query pname: trả state THẬT (trước đây rơi về default 0) ---
        // Buffer bindings (spec §8.1): GL_ARRAY_BUFFER_BINDING 0x8894,
        // GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895, GL_PIXEL_PACK/UNPACK 0x88ED/0x88EF,
        // GL_DISPATCH_INDIRECT_BUFFER_BINDING 0x90EF.
        case 0x8894: *v = (GLint)c.state.BoundBuffer(0x8892 /*ARRAY_BUFFER*/); return;
        case 0x8895: *v = (GLint)c.state.BoundBuffer(0x8893 /*ELEMENT_ARRAY_BUFFER*/); return;
        case 0x88ED: *v = (GLint)c.state.BoundBuffer(0x88EB /*PIXEL_PACK_BUFFER*/); return;
        case 0x88EF: *v = (GLint)c.state.BoundBuffer(0x88EE /*PIXEL_UNPACK_BUFFER*/); return;
        case 0x90EF: *v = (GLint)c.state.BoundBuffer(0x90EE /*DISPATCH_INDIRECT_BUFFER*/); return;
        // GL_VIEWPORT (0x0BA2) — 4 giá trị từ viewport index 0.
        case 0x0BA2: { const auto vp = c.state.GetViewport(0);
                       v[0] = (GLint)vp.x; v[1] = (GLint)vp.y;
                       v[2] = (GLint)vp.w; v[3] = (GLint)vp.h; return; }
        // GL_SCISSOR_BOX (0x0C10) — 4 giá trị.
        case 0x0C10: { const auto sb = c.state.GetScissor();
                       v[0] = sb.x; v[1] = sb.y; v[2] = sb.w; v[3] = sb.h; return; }
        // GL_COLOR_WRITEMASK (0x0C23) — 4 GLboolean.
        case 0x0C23: { const uint8_t m = c.state.ColorMask(0);
                       v[0] = (m & 1) ? 1 : 0; v[1] = (m & 2) ? 1 : 0;
                       v[2] = (m & 4) ? 1 : 0; v[3] = (m & 8) ? 1 : 0; return; }
        // GL_DEPTH_FUNC (0x0B74) / GL_DEPTH_WRITEMASK (0x0B72).
        case 0x0B74: *v = (GLint)c.state.Depth().func; return;
        case 0x0B72: *v = c.state.Depth().writeMask ? 1 : 0; return;
        // GL_BLEND_COLOR (0x8005) — 4 float.
        case 0x8005: { float bc[4]; c.state.GetBlendColor(bc);
                       for (int i = 0; i < 4; ++i) v[i] = (GLint)bc[i]; return; }
        // GL_BLEND_SRC_RGB/DST_RGB/SRC_ALPHA/DST_ALPHA + SRC/DST (legacy).
        // §17.3.2: trạng thái blend là PER DRAW BUFFER → phải đọc theo `idx`
        // (glGetIntegeri_v). Trước đây luôn đọc buffer 0 ⇒ đọc sai khi app
        // dùng blend khác nhau cho từng draw buffer.
        case 0x80C9: *v = (GLint)c.state.Blend()[0].srcRGB; return;
        case 0x80C8: *v = (GLint)c.state.Blend()[0].dstRGB; return;
        case 0x80CB: *v = (GLint)c.state.Blend()[0].srcAlpha; return;
        case 0x80CA: *v = (GLint)c.state.Blend()[0].dstAlpha; return;
        case 0x0BE1: *v = (GLint)c.state.Blend()[0].srcRGB; return;
        case 0x0BE0: *v = (GLint)c.state.Blend()[0].dstRGB; return;
        // GL_BLEND_EQUATION_RGB (0x8009) / _ALPHA (0x883D).
        case 0x8009: *v = (GLint)c.state.Blend()[0].rgbEq; return;
        case 0x883D: *v = (GLint)c.state.Blend()[0].alphaEq; return;
        // GL_POLYGON_OFFSET_FACTOR (0x8038) / UNITS (0x2A00) / CLAMP (0x8037).
        case 0x8038: *v = (GLint)c.state.PolygonFactor(); return;
        case 0x2A00: *v = (GLint)c.state.PolygonUnits(); return;
        case 0x8E1B: *v = (GLint)c.state.PolygonClamp(); return; // POLYGON_OFFSET_CLAMP
        // GL_PROGRAM_PIPELINE_BINDING (0x825A) — trước đây dùng nhầm 0x8254
        // (GL_INNOCENT_CONTEXT_RESET) nên query luôn trả 0.
        case 0x825A: { GLuint pl = 0; c.state.GetShadow(0x825A, &pl, 4);
                       *v = (GLint)pl; return; }
        // GL_PROVOKING_VERTEX (0x8E4F) — state đã lưu, giờ query được.
        case 0x8E4F: { GLenum pv = 0x8E4E /*LAST_VERTEX_CONVENTION*/;
                       c.state.GetShadow(0x8E4F, &pv, 4);
                       *v = (GLint)pv; return; }
        // GL_POINT_SIZE (0x0B11) — state đã lưu.
        case 0x0B11: { GLfloat ps = 1.0f; c.state.GetShadow(0x0B11, &ps, 4);
                       *v = (GLint)ps; return; }
        // GL_MAX_VIEWPORTS (0x825B) / GL_MAX_CLIP_DISTANCES (0x0D32).
        // StateTracker giữ kMaxViewports=16 viewport (viewport+scissor array) →
        // phải trả 16, không phải 1 (app chia sẻ ≤ giá trị này mới an toàn).
        case 0x825B: *v = kMaxViewports; return;
        case 0x0D32: *v = 8; return;   // GL 4.6 core minimum = 8
        // GL_MAX_TEXTURE_MAX_ANISOTROPY (0x84FF) — GL 4.6 core yêu cầu ≥ 2;
        // Metal hỗ trợ tới 16. Query bằng GetIntegerv cũng phải trả giá trị
        // (trước đây chỉ GetFloatv trả 16, GetIntegerv trả 0 → app tưởng
        // không hỗ trợ anisotropy).
        case 0x84FF: *v = 16; return;
        // GL_CLAMP_READ_COLOR (0x891C) — state read clamp (spec §18.2.8).
        case 0x891C: { GLint cc = 0; c.state.GetShadow(0x891C, &cc, 4);
                       *v = cc; return; }
        // --- Enable caps còn lại (core 4.6) ---
        case 0x0B90: *v = c.state.IsEnabled(0x0B90) ? 1 : 0; return; // STENCIL_TEST
        case 0x0BD0: *v = c.state.IsEnabled(0x0BD0) ? 1 : 0; return; // DITHER
        case 0x0B20: *v = c.state.IsEnabled(0x0B20) ? 1 : 0; return; // LINE_SMOOTH
        case 0x0B41: *v = c.state.IsEnabled(0x0B41) ? 1 : 0; return; // POLYGON_SMOOTH
        case 0x809D: *v = c.state.IsEnabled(0x809D) ? 1 : 0; return; // MULTISAMPLE
        case 0x809E: *v = c.state.IsEnabled(0x809E) ? 1 : 0; return; // SAMPLE_ALPHA_TO_COVERAGE
        case 0x809F: *v = c.state.IsEnabled(0x809F) ? 1 : 0; return; // SAMPLE_ALPHA_TO_ONE
        case 0x8E51: *v = c.state.IsEnabled(0x8E51) ? 1 : 0; return; // SAMPLE_MASK
        case 0x8C36: *v = c.state.IsEnabled(0x8C36) ? 1 : 0; return; // SAMPLE_SHADING
        case 0x8642: *v = c.state.IsEnabled(0x8642) ? 1 : 0; return; // PROGRAM_POINT_SIZE
        case 0x8B8B: *v = c.state.IsEnabled(0x8B8B) ? 1 : 0; return; // FRAGMENT_SHADER_DERIVATIVE_HINT
        // --- Mode / constant state ---
        case 0x0B45: *v = (GLint)c.state.CullMode(); return;   // CULL_FACE_MODE
        case 0x0B46: *v = (GLint)c.state.FrontFace(); return;  // FRONT_FACE
        case 0x0B91: *v = c.clearStencil; return;              // STENCIL_CLEAR_VALUE
        case 0x0C32: *v = 1; return;                          // DOUBLEBUFFER
        case 0x0C33: *v = 0; return;                          // STEREO
        case 0x8074: *v = (GLint)c.state.BoundVAO(); return;  // VERTEX_ARRAY (core 4.6: VAO 0 = default)
        case 0x0D50: *v = 8; return;                          // SUBPIXEL_BITS (Metal: 8)
        case 0x825C: *v = 0; return;                          // VIEWPORT_SUBPIXEL_BITS
        case 0x825D: v[0] = 0; v[1] = 8192; return;           // VIEWPORT_BOUNDS_RANGE
        case 0x825E: *v = 0x8E4E; return;                     // LAYER_PROVOKING_VERTEX (LAST)
        case 0x825F: *v = 0x8E4E; return;                     // VIEWPORT_INDEX_PROVOKING_VERTEX (LAST)
        case 0x826C: *v = 64; return;                         // MAX_DEBUG_GROUP_STACK_DEPTH
        case 0x826D: *v = 0; return;                          // DEBUG_GROUP_STACK_DEPTH
        case 0x82E8: *v = 256; return;                        // MAX_LABEL_LENGTH
        case 0x84FD: *v = 15; return;                         // MAX_TEXTURE_LOD_BIAS
        case 0x84EF: *v = c.state.IsEnabled(0x84EF) ? 1 : 0; return; // TEXTURE_COMPRESSION_HINT
        case 0x8F9E: *v = 0; return;                          // PRIMITIVE_RESTART_INDEX
        // Compressed texture (spec §8.5): NUM_COMPRESSED_TEXTURE_FORMATS là số
        // format nén hỗ trợ; COMPRESSED_TEXTURE_FORMATS trả mảng GLenum đúng
        // số phần tử đó (GetIntegerv, không phải GetIntegerv_v).
        case 0x86A2: *v = (GLint)tglmt::gl::CompressedTextureFormats().size(); return;
        case 0x86A3: {
            const std::vector<GLenum>& fmts = tglmt::gl::CompressedTextureFormats();
            for (size_t i = 0; i < fmts.size(); ++i) v[i] = (GLint)fmts[i];
            return;
        }
        case 0x9111: *v = 0; return;                          // MAX_SERVER_WAIT_TIMEOUT (ns)
        case 0x919F: *v = 4; return;                          // TEXTURE_BUFFER_OFFSET_ALIGNMENT
        case 0x846E: v[0] = 1; v[1] = 1; return;              // ALIASED_LINE_WIDTH_RANGE
        case 0x0B13: *v = 0; return;                          // POINT_SIZE_GRANULARITY
        case 0x0B23: *v = 0; return;                          // LINE_WIDTH_GRANULARITY
        case 0x0C52: *v = c.state.IsEnabled(0x0C52) ? 1 : 0; return; // LINE_SMOOTH_HINT
        case 0x0C53: *v = c.state.IsEnabled(0x0C53) ? 1 : 0; return; // POLYGON_SMOOTH_HINT
        // --- Pixel store (pack/unpack) ---
        case 0x0D05: *v = c.state.PixelStore().packAlignment; return;   // PACK_ALIGNMENT
        case 0x0CF5: *v = c.state.PixelStore().unpackAlignment; return; // UNPACK_ALIGNMENT
        case 0x0D02: *v = c.state.PixelStore().packRowLength; return;  // PACK_ROW_LENGTH
        case 0x0CF2: *v = c.state.PixelStore().unpackRowLength; return;// UNPACK_ROW_LENGTH
        case 0x0D03: *v = c.state.PixelStore().packSkipRows; return;   // PACK_SKIP_ROWS
        case 0x0CF3: *v = c.state.PixelStore().unpackSkipRows; return; // UNPACK_SKIP_ROWS
        case 0x0D04: *v = c.state.PixelStore().packSkipPixels; return; // PACK_SKIP_PIXELS
        case 0x0CF4: *v = c.state.PixelStore().unpackSkipPixels; return;// UNPACK_SKIP_PIXELS
        case 0x806C: *v = c.state.PixelStore().packImageHeight; return;  // PACK_IMAGE_HEIGHT
        case 0x806E: *v = c.state.PixelStore().unpackImageHeight; return;// UNPACK_IMAGE_HEIGHT
        case 0x806B: *v = c.state.PixelStore().packSkipImages; return;   // PACK_SKIP_IMAGES
        case 0x806D: *v = c.state.PixelStore().unpackSkipImages; return; // UNPACK_SKIP_IMAGES
        case 0x0D00: *v = 0; return;                          // PACK_SWAP_BYTES
        case 0x0D01: *v = 0; return;                          // PACK_LSB_FIRST
        case 0x0CF0: *v = 0; return;                          // UNPACK_SWAP_BYTES
        case 0x0CF1: *v = 0; return;                          // UNPACK_LSB_FIRST
        // --- Texture target enums + bindings ---
        case 0x0DE0: *v = c.state.IsEnabled(0x0DE0) ? 1 : 0; return; // TEXTURE_1D
        case 0x0DE1: *v = c.state.IsEnabled(0x0DE1) ? 1 : 0; return; // TEXTURE_2D
        case 0x8068: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x0DE0); return; // TEXTURE_BINDING_1D
        case 0x8514: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x8513); return; // TEXTURE_BINDING_CUBE_MAP
        case 0x8C1C: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x8C18); return; // TEXTURE_BINDING_1D_ARRAY
        case 0x9104: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x9100); return; // TEXTURE_BINDING_2D_MULTISAMPLE
        case 0x9105: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x9102); return; // TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY
        // --- Program / vertex-attrib limits ---
        case 0x8904: *v = -8; return;                         // MIN_PROGRAM_TEXEL_OFFSET
        case 0x8905: *v = 7; return;                          // MAX_PROGRAM_TEXEL_OFFSET
        case 0x8A2C: *v = 12; return;                         // MAX_GEOMETRY_UNIFORM_BLOCKS
        case 0x8A31: *v = 4096; return;                       // MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS
        case 0x8A32: *v = 4096; return;                       // MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS
        case 0x8A33: *v = 4096; return;                       // MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS
        case 0x8DDF: *v = 1024; return;                       // MAX_GEOMETRY_UNIFORM_COMPONENTS
        case 0x8E89: *v = 12; return;                         // MAX_TESS_CONTROL_UNIFORM_BLOCKS
        case 0x8E8A: *v = 12; return;                         // MAX_TESS_EVALUATION_UNIFORM_BLOCKS
        case 0x8C29: *v = 16; return;                         // MAX_GEOMETRY_TEXTURE_IMAGE_UNITS
        case 0x92D3: *v = 8; return;                          // MAX_TESS_CONTROL_ATOMIC_COUNTERS
        case 0x92D4: *v = 8; return;                          // MAX_TESS_EVALUATION_ATOMIC_COUNTERS
        case 0x92D5: *v = 8; return;                          // MAX_GEOMETRY_ATOMIC_COUNTERS
        case 0x9122: *v = 64; return;                         // MAX_VERTEX_OUTPUT_COMPONENTS
        case 0x9123: *v = 64; return;                         // MAX_GEOMETRY_INPUT_COMPONENTS
        case 0x9124: *v = 128; return;                        // MAX_GEOMETRY_OUTPUT_COMPONENTS
        case 0x9125: *v = 128; return;                        // MAX_FRAGMENT_INPUT_COMPONENTS
        case 0x9315: *v = 8192; return;                       // MAX_FRAMEBUFFER_WIDTH
        case 0x9316: *v = 8192; return;                       // MAX_FRAMEBUFFER_HEIGHT
        case 0x9317: *v = 256; return;                       // MAX_FRAMEBUFFER_LAYERS
        case 0x9318: *v = 4; return;                          // MAX_FRAMEBUFFER_SAMPLES
        case 0x82DA: *v = 16; return;                         // MAX_VERTEX_ATTRIB_BINDINGS
        case 0x82D9: *v = 2048; return;                       // MAX_VERTEX_ATTRIB_RELATIVE_OFFSET
        case 0x82D6: *v = 0; return;                          // VERTEX_BINDING_DIVISOR
        case 0x82D7: *v = 0; return;                          // VERTEX_BINDING_OFFSET
        case 0x82D8: *v = 0; return;                          // VERTEX_BINDING_STRIDE
        // --- Stencil back face ---
        case 0x8CA4: *v = (GLint)c.state.StencilBack().valueMask; return;  // STENCIL_BACK_VALUE_MASK
        case 0x8CA5: *v = (GLint)c.state.StencilBack().writeMask; return;  // STENCIL_BACK_WRITEMASK

        // GL_NV_polygon_mode: glPolygonMode lưu polyMode_ (FRONT_AND_BACK chung).
        // Refpage: POLYGON_MODE trả 2 phần tử [front, back] — TGLMT dùng chung 1 mode.
        case 0x0B40: v[0] = (GLint)c.state.PolygonMode();
                     v[1] = (GLint)c.state.PolygonMode(); return;  // POLYGON_MODE
        // Stencil state: GL 1.0 (front) + GL 2.0 (back) — ATI_separate_stencil
        // advertise ⇒ query được. Trước đây stencil chỉ set, không đọc → nói dối.
        case 0x0B92: *v = (GLint)c.state.StencilFront().func; return;       // STENCIL_FUNC
        case 0x0B93: *v = (GLint)c.state.StencilFront().valueMask; return;  // STENCIL_VALUE_MASK
        case 0x0B97: *v = c.state.StencilFront().ref; return;               // STENCIL_REF
        case 0x0B98: *v = (GLint)c.state.StencilFront().writeMask; return;  // STENCIL_WRITEMASK
        case 0x0B94: *v = (GLint)c.state.StencilFront().sfail; return;      // STENCIL_FAIL
        case 0x0B95: *v = (GLint)c.state.StencilFront().dpfail; return;     // STENCIL_PASS_DEPTH_FAIL
        case 0x0B96: *v = (GLint)c.state.StencilFront().dppass; return;     // STENCIL_PASS_DEPTH_PASS
        case 0x8800: *v = (GLint)c.state.StencilBack().func; return;        // STENCIL_BACK_FUNC
        case 0x8801: *v = (GLint)c.state.StencilBack().sfail; return;       // STENCIL_BACK_FAIL
        case 0x8802: *v = (GLint)c.state.StencilBack().dpfail; return;      // STENCIL_BACK_PASS_DEPTH_FAIL
        case 0x8803: *v = (GLint)c.state.StencilBack().dppass; return;      // STENCIL_BACK_PASS_DEPTH_PASS
        case 0x8CA3: *v = c.state.StencilBack().ref; return;                // STENCIL_BACK_REF
        // GL_ARB_get_program_binary: TGLMT có 1 format duy nhất (GLSL gốc).
        case 0x87FE: *v = 1; return;                               // NUM_PROGRAM_BINARY_FORMATS
        case 0x87FF: *v = (GLint)0x54474D54; return;               // PROGRAM_BINARY_FORMATS[0] = 'TGMT'
        // GL_ARB_map_buffer_alignment: base map至少 16-aligned (std::vector/malloc;
        // MTLBuffer contents page-aligned). App đảm bảo offset chia hết → pointer aligned.
        case 0x90BC: *v = 16; return;                              // MIN_MAP_BUFFER_ALIGNMENT
        // GL_ARB_parallel_shader_compile: số thread biên dịch shader app yêu cầu
        // (glMaxShaderCompilerThreads). TGLMT biên dịch đồng bộ → tối thiểu 1.
        case 0x91B0: *v = MaxShaderCompilerThreads(c); return;
        // GL_ARB_explicit_uniform_location: GL 4.3 đòi tối thiểu 1024; TGLMT
        // dùng map → mọi location < 1024 hoạt động.
        case 0x826E: *v = 1024; return;                            // MAX_UNIFORM_LOCATIONS
        // GL_ARB_clip_control: glClipControl lưu + áp (viewport Y-flip, clear
        // depth, shader depth map) → round-trip query được.
        case 0x935C: *v = (GLint)c.state.ClipOrigin(); return;     // CLIP_ORIGIN
        case 0x935D: *v = (GLint)c.state.ClipDepth(); return;      // CLIP_DEPTH
        // Bindings thật — Iris PBR (TextureManipulationUtil/TextureInfoCache) query
        // rồi restore; trả 0 sẽ rebind nhầm khiến texture/FBO lệch cache của MC.
        case 0x8CA6: *v = (GLint)c.state.BoundDrawFBO(); return;   // FRAMEBUFFER_BINDING (= DRAW_FRAMEBUFFER_BINDING)
        case 0x8CAA: *v = (GLint)c.state.BoundReadFBO(); return;   // READ_FRAMEBUFFER_BINDING
        // Per-FBO (hoặc default FB shadow): DRAW_BUFFER/READ_BUFFER/DRAW_BUFFERi.
        case 0x0C01: *v = (GLint)c.CurrentDrawBuffer(0); return;   // DRAW_BUFFER (= DRAW_BUFFER0)
        case 0x0C02: *v = (GLint)c.CurrentReadBuffer(); return;    // READ_BUFFER
        case 0x8825: case 0x8826: case 0x8827: case 0x8828:        // DRAW_BUFFER0..3
        case 0x8829: case 0x882A: case 0x882B: case 0x882C:        // DRAW_BUFFER4..7
            *v = (GLint)c.CurrentDrawBuffer(p - 0x8825); return;
        case 0x84E0: *v = (GLint)c.state.ActiveTexture(); return;  // ACTIVE_TEXTURE
        case 0x8069: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x0DE1); return; // TEXTURE_BINDING_2D
        // (0x8516 = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y — enum CUBE_MAP_BINDING thật là
        //  0x8514, đã xử lý ở nhóm query mới. Xoá case sai để không trả nhầm.)
        case 0x8C2C: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x8C2A); return; // TEXTURE_BINDING_BUFFER
        case 0x8C1D: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x8C1A); return; // TEXTURE_BINDING_2D_ARRAY
        case 0x806A: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x806F); return; // TEXTURE_BINDING_3D
        case 0x84F6: *v = (GLint)c.state.BoundTexture(c.state.ActiveTexture(), 0x84F5); return; // TEXTURE_BINDING_RECTANGLE
        case 0x1F02: break;
        default: break;
    }
    if (StaticLimit(p, v)) return;
    OutI(c, p, v);
    // GL 4.6 §22.1: "An INVALID_ENUM error is generated if pname is not one of
    // the accepted values." Trước đây pname lạ âm thầm trả 0 — mất đúng cơ
    // chế app dùng để phát hiện driver không tuân thủ.
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
void glGetIntegeri_v(GLenum p, GLuint i, GLint* v) {
    Context& c = Context::Current();
    OutI(c, p, v, i);
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
// glGetInteger64v: HÀNH VI CŨ đọc GLint (32-bit) rồi ép sang 64-bit → mọi giá trị
// >2^31-1 bị cắt cụt. App dùng nó cho timestamp (GL_TIMESTAMP), buffer size lớn,
// GL_MAX_* 64-bit. Sửa: thử shadow 8-trước (SetShadow với n=8), rồi 4-trước,
// rồi StaticLimit, rồi giá trị mặc định spec (0).
void glGetInteger64v(GLenum p, GLint64* v) {
    Context& c = Context::Current();
    if (c.state.GetShadow(p, v, 8)) return;      // 8-byte shadow (timestamp/size thật)
    GLint t = 0;
    if (c.state.GetShadow(p, &t, 4)) { *v = t; return; }
    if (p == 0x0C01) { *v = c.CurrentDrawBuffer(0); return; }
    if (p == 0x0C02) { *v = c.CurrentReadBuffer(); return; }
    if (p >= 0x8825 && p <= 0x882C) { *v = c.CurrentDrawBuffer(p - 0x8825); return; }
    // ARB_parallel_shader_compile: GetInteger64v cũng phải trả cùng giá trị
    // với GetIntegerv (spec: token nhận bởi cả hai).
    if (p == 0x91B0) { *v = MaxShaderCompilerThreads(c); return; }
    if (StaticLimit(p, &t)) { *v = t; return; }
    // Các pname 64-bit spec mà shadow/StaticLimit chưa cover.
    if (p == 0x8E28) { // GL_TIMESTAMP (glQueryCounter đã lưu shadow 8-byte)
        *v = 0; return;
    }
    *v = 0;
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
void glGetInteger64i_v(GLenum p, GLuint i, GLint64* v) {
    Context& c = Context::Current();
    if (c.state.GetShadow(p, v, 8, i)) return;
    GLint t = 0;
    if (c.state.GetShadow(p, &t, 4, i)) { *v = t; return; }
    OutI(c, p, &t, i);
    *v = t;
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
void glGetBooleanv(GLenum p, GLboolean* v) { GLint t=0; glGetIntegerv(p,&t); *v=t?1:0; }
void glGetBooleani_v(GLenum p, GLuint i, GLboolean* v) { GLint t=0; glGetIntegeri_v(p,i,&t); *v=t?1:0; }
void glGetFloatv(GLenum p, GLfloat* v) {
    Context& c = Context::Current();
    GLfloat t = 0;
    if (p == 0x0B73) { // DEPTH_CLEAR_VALUE (trước đây dùng nhầm 0x0B66 = GL_FOG_COLOR)
        *v = (GLfloat)c.clearDepth; return;
    }
    if (p == 0x0C01) { *v = (GLfloat)c.CurrentDrawBuffer(0); return; }
    if (p == 0x0C02) { *v = (GLfloat)c.CurrentReadBuffer(); return; }
    if (p >= 0x8825 && p <= 0x882C) { *v = (GLfloat)c.CurrentDrawBuffer(p - 0x8825); return; }
    if (p == 0x0C22) { // COLOR_CLEAR_VALUE
        for (int i=0;i<4;i++) v[i]=c.clearColor[i]; return;
    }
    if (p == 0x0B22) { v[0] = 1; v[1] = 1; return; } // ALIASED_LINE_WIDTH_RANGE
    if (p == 0x0B12) { v[0] = 1; v[1] = 1; return; } // POINT_SIZE_RANGE (cap 1, an toàn)
    if (p == 0x0BF0 || p == 0x0BF1) {                // LOGIC_OP_MODE / LOGIC_OP
        GLint op = 0x1503 /*GL_COPY*/;
        c.state.GetShadow(0x0BF1, &op, 4);
        *v = (GLfloat)op; return;
    }
    if (p == 0x8C37) {                                // MIN_SAMPLE_SHADING_VALUE
        GLfloat mss = 0.0f;
        c.state.GetShadow(0x8C37, &mss, 4);
        *v = mss; return;
    }
    if (p == 0x0B11) {                                // POINT_SIZE
        GLfloat ps = 1.0f;
        c.state.GetShadow(0x0B11, &ps, 4);
        *v = ps; return;
    }
    if (p == 0x84FF) { *v = 16; return; }            // MAX_TEXTURE_MAX_ANISOTROPY
    if (p == 0x80AA) {                               // SAMPLE_COVERAGE_VALUE mặc định
        if (!c.state.GetShadow(p, &t, 4)) t = 1.0f;
        *v = t; return;
    }
    if (p == 0x8E74) { // PATCH_DEFAULT_OUTER_LEVEL (mặc định 1,1,1,1)
        const float* d = c.state.PatchDefaultOuter();
        for (int i = 0; i < 4; ++i) v[i] = d[i];
        return;
    }
    if (p == 0x8E73) { // PATCH_DEFAULT_INNER_LEVEL (mặc định 1,1)
        const float* d = c.state.PatchDefaultInner();
        v[0] = d[0]; v[1] = d[1];
        return;
    }
    // Float-typed core pname: VIEWPORT / SCISSOR_BOX / COLOR_WRITEMASK /
    // BLEND_COLOR / DEPTH_RANGE / COLOR_CLEAR_VALUE (đã ở trên).
    if (p == 0x0BA2) { // VIEWPORT
        const auto vp = c.state.GetViewport(0);
        v[0] = vp.x; v[1] = vp.y; v[2] = vp.w; v[3] = vp.h; return;
    }
    if (p == 0x0C10) { // SCISSOR_BOX
        const auto sb = c.state.GetScissor();
        v[0] = (GLfloat)sb.x; v[1] = (GLfloat)sb.y;
        v[2] = (GLfloat)sb.w; v[3] = (GLfloat)sb.h; return;
    }
    if (p == 0x0C23) { // COLOR_WRITEMASK
        const uint8_t m = c.state.ColorMask(0);
        v[0] = (m & 1) ? 1 : 0; v[1] = (m & 2) ? 1 : 0;
        v[2] = (m & 4) ? 1 : 0; v[3] = (m & 8) ? 1 : 0; return;
    }
    if (p == 0x8005) { // BLEND_COLOR
        float bc[4]; c.state.GetBlendColor(bc);
        for (int i = 0; i < 4; ++i) v[i] = bc[i]; return;
    }
    if (p == 0x0B70) { // DEPTH_RANGE
        const auto vp = c.state.GetViewport(0);
        v[0] = vp.n; v[1] = vp.f; return;
    }
    if (p == 0x0B21) { // LINE_WIDTH
        GLfloat lw = 1.0f;
        c.state.GetShadow(0x0B21, &lw, 4);
        *v = lw; return;
    }
    if (p == 0x8128) { // POINT_FADE_THRESHOLD_SIZE
        GLfloat pft = 1.0f;
        c.state.GetShadow(0x8128, &pft, 4);
        *v = pft; return;
    }
    if (p == 0x846E) { v[0] = 1; v[1] = 1; return; } // ALIASED_LINE_WIDTH_RANGE
    if (c.state.GetShadow(p, &t, 4)) { *v = t; return; }
    *v = 0;
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
void glGetFloati_v(GLenum p, GLuint i, GLfloat* v) {
    Context& c = Context::Current();
    // GL_DEPTH_RANGE (0x0B70) là 2 giá trị (n, f) và LƯU RIÊNG cho từng
    // viewport index (glDepthRangeIndexed/Arrayv). Shadow lưu 16 byte
    // (2 double) → đọc 8 byte, không phải 4.
    if (p == 0x0B70) {
        double dr[2] = {0.0, 1.0};
        if (!c.state.GetShadow(p, dr, sizeof(dr), i)) { dr[0] = 0.0; dr[1] = 1.0; }
        v[0] = (GLfloat)dr[0];
        v[1] = (GLfloat)dr[1];
        return;
    }
    // GL_VIEWPORT (0x0BA2) là 4 giá trị và RIÊNG cho từng viewport index
    // (glViewportIndexedf/fv) → đọc từ state, không phải shadow 4 byte.
    if (p == 0x0BA2) {
        const auto vp = c.state.GetViewport((int)i);
        v[0] = vp.x; v[1] = vp.y; v[2] = vp.w; v[3] = vp.h;
        return;
    }
    GLfloat t=0;
    if (c.state.GetShadow(p,&t,4,i)) *v=t; else *v=0;
    if (!IsAcceptedPname(p)) c.errors.Record(0x0500);
}
void glGetDoublev(GLenum p, GLdouble* v) { GLfloat t=0; glGetFloatv(p,&t); *v=t; }
void glGetDoublei_v(GLenum p, GLuint i, GLdouble* v) {
    Context& c = Context::Current();
    if (p == 0x0B70) {  // GL_DEPTH_RANGE: 2 double theo viewport index
        double dr[2] = {0.0, 1.0};
        if (!c.state.GetShadow(p, dr, sizeof(dr), i)) { dr[0] = 0.0; dr[1] = 1.0; }
        v[0] = dr[0];
        v[1] = dr[1];
        return;
    }
    GLfloat t=0; glGetFloati_v(p,i,&t); *v=t;
}
void glGetBufferParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundBuffer(t);
    auto it = c.buffers.find(id);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8764) *v = (GLint)BufferLen(it->second);       // BUFFER_SIZE
    else if (p == 0x8765) *v = it->second.usage;              // BUFFER_USAGE
    else if (p == 0x88BB) *v = it->second.mapped ? 1 : 0;     // BUFFER_MAPPED
    // GL_ARB_buffer_storage: flags đã truyền cho glBufferStorage (đọc lại
    // để app biết buffer có MAP_* quyền gì) — trước đây rơi về 0.
    else if (p == 0x8220) *v = (GLint)it->second.storageFlags; // BUFFER_STORAGE_FLAGS
    else *v = 0;
}
void glGetBufferParameteri64v(GLenum t, GLenum p, GLint64* v) { GLint x=0; glGetBufferParameteriv(t,p,&x); *v=x; }
void glGetBufferPointerv(GLenum t, GLenum p, void** v) {
    Context& c = Context::Current();
    (void)p;
    GLuint id = c.state.BoundBuffer(t);
    auto it = c.buffers.find(id);
    *v = (it == c.buffers.end()) ? nullptr : (void*)BufferBytes(it->second);
}
void glGetNamedBufferParameteriv(GLuint b, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8764) *v = (GLint)BufferLen(it->second);
    else if (p == 0x8765) *v = it->second.usage;
    else if (p == 0x8220) *v = (GLint)it->second.storageFlags; // BUFFER_STORAGE_FLAGS
    else *v = 0;
}
void glGetNamedBufferParameteri64v(GLuint b, GLenum p, GLint64* v) { GLint x=0; glGetNamedBufferParameteriv(b,p,&x); *v=x; }
void glGetNamedBufferPointerv(GLuint b, GLenum p, void** v) {
    (void)p;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    *v = (it == c.buffers.end()) ? nullptr : (void*)BufferBytes(it->second);
}
void glGetShaderiv(GLuint s, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8B81) *v = it->second.compiled ? 1 : 0;      // COMPILE_STATUS
    else if (p == 0x8B84) *v = (GLint)it->second.infoLog.size(); // INFO_LOG_LENGTH
    else if (p == 0x8B88) *v = (GLint)it->second.source.size();  // SHADER_SOURCE_LENGTH
    else if (p == 0x8B4F) *v = it->second.type;             // SHADER_TYPE
    // GL_ARB_parallel_shader_compile: TGLMT biên dịch shader ĐỒNG BỘ (không có
    // compile async) → khi app hỏi thì việc biên dịch đã xong → TRUE.
    else if (p == 0x91B1) *v = 1;                           // COMPLETION_STATUS_ARB
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
    else if (q == 0x8B83) *v = it->second.validateStatus ? 1 : 0; // VALIDATE_STATUS
    else if (q == 0x8B84) *v = (GLint)it->second.infoLog.size();
    else if (q == 0x8B85) *v = (GLint)it->second.shaders.size(); // ATTACHED_SHADERS
    else if (q == 0x8258) *v = it->second.separable ? 1 : 0; // PROGRAM_SEPARABLE
    // ACTIVE_UNIFORMS (0x8B86): stub cũ map 0x8B86 → ATTACHED_SHADERS (sai mã;
    // ATTACHED_SHADERS = 0x8B85) → Iris ProgramUniforms chỉ thấy 2 uniform, và
    // glGetActiveUniform trả FLOAT cho mọi tên → mat4 (iris_LightmapTextureMatrix)
    // bị tắt. Trả số active uniform reflect lúc link (kể cả sampler).
    else if (q == 0x8B86) *v = (GLint)it->second.activeUniforms.size();
    // ACTIVE_ATTRIBUTES / MAX_LENGTH (spec §7.11) — Iris/MC query sau link.
    else if (q == 0x8B89) *v = (GLint)it->second.activeAttribs.size();
    else if (q == 0x8B8A) {
        size_t mx = 0;
        for (auto& a : it->second.activeAttribs) mx = std::max(mx, a.name.size());
        *v = (GLint)(mx ? mx + 1 : 0); // NUL
    }
    else if (q == 0x8B87) { // ACTIVE_UNIFORM_MAX_LENGTH
        size_t mx = 0;
        for (auto& u : it->second.activeUniforms) mx = std::max(mx, u.name.size());
        *v = (GLint)(mx ? mx + 1 : 0);
    }
    // ACTIVE_ATOMIC_COUNTER_BUFFERS (0x92D9, gl.xml) — số binding duy nhất.
    else if (q == 0x92D9) {
        std::vector<int> binds;
        auto cnt = [&](const std::vector<ProgramObject::AtomicSlot>& vv) {
            for (auto& s : vv)
                if (std::find(binds.begin(), binds.end(), s.binding) == binds.end())
                    binds.push_back(s.binding);
        };
        cnt(it->second.computeAtomics); cnt(it->second.vsAtomics);
        cnt(it->second.fsAtomics); cnt(it->second.tcsAtomics); cnt(it->second.tesAtomics);
        *v = (GLint)binds.size();
    }
    // ACTIVE_UNIFORM_BLOCKS (0x8A36=35382): GlProgram.setupUniforms path#2
    // loop [0,count) gọi glGetActiveUniformBlockName tìm builtin {Projection,
    // Lighting, Fog, Globals} không khai trong pipeline. Stub trả 0 → loop
    // không chạy → Globals KHÔNG BAO GIỜ ĐƯỢC BIND cho prog ngoài pipeline
    // desc (terrain/blur) → CameraBlockPos rác, MenuBlurRadius rác.
    else if (q == 0x8A36) *v = (GLint)it->second.uniformBlocks.size();
    // GL_COMPUTE_WORK_GROUP_SIZE (0x8267, GL 4.3 §7.12.3): trả 3 int
    // local_size_x/y/z (Iris ComputeProgram đọc int[3] ngay sau link để tính
    // số workgroup). Chưa có CS → 0 cả 3 (giống Mesa — query vô nghĩa ngoài CS).
    else if (q == 0x8267) {
        if (v) {
            GLint lx = 0, ly = 0, lz = 0;
            for (GLuint s : it->second.shaders) {
                auto sit = c.shaders.find(s);
                if (sit != c.shaders.end() && sit->second.type == 0x91B9) {
                    lx = it->second.computeLocalSize[0];
                    ly = it->second.computeLocalSize[1];
                    lz = it->second.computeLocalSize[2];
                    break;
                }
            }
            v[0] = lx; v[1] = ly; v[2] = lz;
        }
    }
    // GL_ARB_get_program_binary
    else if (q == 0x8741) *v = gl::ProgramBinaryLength(p); // PROGRAM_BINARY_LENGTH
    else if (q == 0x87FE) *v = 1; // NUM_PROGRAM_BINARY_FORMATS (context-level)
    // PROGRAM_BINARY_RETRIEVABLE_HINT (0x8257): app đã bật/tắt hint chưa →
    // query trả đúng trạng thái đó (spec §7.3).
    else if (q == 0x8257) *v = it->second.binaryRetrievable ? 1 : 0;
    // GL_ARB_parallel_shader_compile: link chạy ĐỒNG BỘ trong glLinkProgram
    // → khi app hỏi thì đã xong → TRUE.
    else if (q == 0x91B1) *v = 1; // COMPLETION_STATUS_ARB
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
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (l) *l = 0; if (s) *s = 1; if (t) *t = 0x1406;
    auto& aas = it->second.activeAttribs;
    if (i >= aas.size()) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    const auto& aa = aas[i];
    if (s) *s = aa.size;
    if (t) *t = aa.type;
    if (name && n > 0) {
        strncpy(name, aa.name.c_str(), (size_t)n - 1);
        name[n - 1] = 0;
        if (l) *l = (GLsizei)strlen(name);
    }
}
void glGetActiveUniform(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLint* s, GLenum* t, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (l) *l = 0; if (s) *s = 1; if (t) *t = 0x1406;
    auto& aus = it->second.activeUniforms;
    if (i >= aus.size()) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    const auto& au = aus[i];
    if (s) *s = au.size;
    if (t) *t = au.type;
    if (name && n > 0) {
        strncpy(name, au.name.c_str(), (size_t)n - 1);
        name[n - 1] = 0;
        if (l) *l = (GLsizei)strlen(name);
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
void glGetActiveUniformName(GLuint p, GLuint i, GLsizei n, GLsizei* l, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (l) *l = 0;
    auto& aus = it->second.activeUniforms;
    if (i >= aus.size()) { c.errors.Record(0x0501); return; }
    if (name && n > 0) {
        strncpy(name, aus[i].name.c_str(), (size_t)n - 1);
        name[n - 1] = 0;
        if (l) *l = (GLsizei)strlen(name);
    }
}
// glGetActiveUniformsiv — query theo index vào activeUniforms (spec §7.11).
// UNIFORM_BLOCK_INDEX = -1 khi không nằm trong block (TGLMT không xếp block
// theo uniform → -1 trung thực). OFFSET/ARRAY_STRIDE/MATRIX_STRIDE đọc từ
// uniformLayout nếu khớp tên; chưa layout → 0.
void glGetActiveUniformsiv(GLuint p, GLsizei n, const GLuint* idx, GLenum q, GLint* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (!v || !idx) return;
    auto& aus = it->second.activeUniforms;
    for (GLsizei k = 0; k < n; ++k) {
        GLint out = 0;
        if (idx[k] >= aus.size()) { c.errors.Record(0x0501); v[k] = 0; continue; }
        const auto& au = aus[idx[k]];
        switch (q) {
            case 0x8A37: out = (GLint)au.type; break;                 // UNIFORM_TYPE
            case 0x8A38: out = au.size; break;                        // UNIFORM_SIZE
            case 0x8A39: out = (GLint)au.name.size() + 1; break;      // UNIFORM_NAME_LENGTH (NUL)
            case 0x8A3A: out = -1; break;                             // UNIFORM_BLOCK_INDEX (-1 = không block)
            case 0x8A3B: {                                            // UNIFORM_OFFSET
                for (auto& ul : it->second.uniformLayout)
                    if (ul.name == au.name || (au.name.size() > 3 &&
                        au.name.compare(au.name.size() - 3, 3, "[0]") == 0 &&
                        ul.name == au.name.substr(0, au.name.size() - 3))) {
                        out = (GLint)ul.offset; break;
                    }
                break;
            }
            case 0x8A3C: out = 0; break;                              // UNIFORM_ARRAY_STRIDE (std140: size/N)
            case 0x8A3D: out = 0; break;                              // UNIFORM_MATRIX_STRIDE
            case 0x8A3E: out = 0; break;                              // UNIFORM_IS_ROW_MAJOR
            default: out = 0; break;
        }
        v[k] = out;
    }
}
// glGetActiveAtomicCounterBufferiv — atomic buffers nhóm theo binding.
// Pnames (gl.xml): GL_BUFFER_BINDING=0x9302, GL_BUFFER_DATA_SIZE=0x9303,
// GL_ACTIVE_ATOMIC_COUNTER_BUFFERS (glGetProgramiv) = 0x92D9.
void glGetActiveAtomicCounterBufferiv(GLuint p, GLuint b, GLenum q, GLint* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); if (v) *v = 0; return; }
    if (!v) return;
    // Gom atomic slots theo binding (binding = buffer index space).
    std::vector<const ProgramObject::AtomicSlot*> slots;
    auto collect = [&](const std::vector<ProgramObject::AtomicSlot>& vec) {
        for (auto& s : vec) slots.push_back(&s);
    };
    collect(it->second.computeAtomics);
    collect(it->second.vsAtomics);
    collect(it->second.fsAtomics);
    collect(it->second.tcsAtomics);
    collect(it->second.tesAtomics);
    // Đếm binding duy nhất theo thứ tự gặp.
    std::vector<int> bindings;
    for (auto* s : slots)
        if (std::find(bindings.begin(), bindings.end(), s->binding) == bindings.end())
            bindings.push_back(s->binding);
    if (b >= bindings.size()) { c.errors.Record(0x0501); *v = 0; return; }
    int bind = bindings[b];
    switch (q) {
        case 0x9302: { // GL_BUFFER_BINDING (atomic buffer binding point)
            auto ait = c.atomicBindPoints.find((GLuint)bind);
            *v = (ait == c.atomicBindPoints.end()) ? 0 : (GLint)ait->second.buffer;
            break;
        }
        case 0x9303: { // GL_BUFFER_DATA_SIZE — tổng offset+size của slot trong binding
            size_t maxEnd = 0;
            for (auto* s : slots)
                if (s->binding == bind)
                    maxEnd = std::max(maxEnd, (size_t)(s->offset + 4)); // atomic_uint = 4B
            *v = (GLint)maxEnd;
            break;
        }
        default: *v = 0; break;
    }
}
GLint glGetAttribLocation(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !n) { c.errors.Record(0x0502); return -1; }
    auto f = it->second.attribLoc.find(n);
    // Chưa link / không có attrib → -1 (đúng GL). Link fail → attribLoc rỗng → -1.
    return (f == it->second.attribLoc.end()) ? -1 : f->second;
}
GLint glGetFragDataLocation(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !it->second.linked) { c.errors.Record(0x0502); return -1; }
    if (!n) { c.errors.Record(0x0501); return -1; }
    for (auto& o : it->second.fragOutputs)
        if (o.name == n) return o.location;
    return -1;
}
GLint glGetFragDataIndex(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !it->second.linked) { c.errors.Record(0x0502); return -1; }
    if (!n) { c.errors.Record(0x0501); return -1; }
    for (auto& o : it->second.fragOutputs)
        if (o.name == n) return (GLint)o.index;
    return -1;
}
// GL 4.6 §7.14 — GetUniform*:
//   INVALID_VALUE     : program không phải tên program/shader object
//   INVALID_OPERATION : program là shader object, hoặc chưa link thành công,
//                        hoặc location không hợp lệ với program đó.
// Trước đây: program lạ → INVALID_OPERATION (sai), location không hợp lệ →
// ghi 0 và KHÔNG sinh lỗi (sai), và glGetUniformdv đọc byte của uniform
// double qua bộ đệm float ⇒ trả về giá trị rác.
//
// Kiểu lưu trữ: uniform bool được nạp bằng Uniform1i/1ui/1f (spec cho
// phép) nhưng phải đọc lại được 0/1 → luôn chuẩn hoá về int khi nạp.
static bool UniformValueFloats(const ProgramObject::UniformSlot::Spec& sp) {
    switch (sp.type) {
        case 0x1406: case 0x8B50: case 0x8B51: case 0x8B52: return true;
        default: return false;
    }
}

// Số phần tử (theo kiểu lưu trữ) của một uniform.
static int UniformStoreElemCount(const ProgramObject::UniformSlot::Spec& sp, size_t storeBytes,
                                 uint32_t storeType) {
    int cols = 0, rows = 0;
    int comps = UniformComponentCount(sp.type);
    if (!comps && UniformTypeIsMatrix(sp.type, cols, rows)) comps = cols * rows;
    if (!comps) comps = 1;  // sampler / image / không rõ
    if (storeBytes) {
        const size_t per = (storeType == 0x140A) ? sizeof(GLdouble) : sizeof(GLfloat);
        const size_t n = storeBytes / per;
        if (n) return (int)n;
    }
    return comps * (sp.arraySize > 0 ? sp.arraySize : 1);
}

// Đọc giá trị uniform đã lưu và chuyển sang kiểu yêu cầu.
//   want: GL_FLOAT / GL_DOUBLE / GL_INT / GL_UNSIGNED_INT
// Trả về false nếu đã sinh lỗi (theo spec) — không ghi gì vào out.
bool ReadUniformValueTyped(Context& c, ProgramObject& pr, GLint l, uint32_t want, void* out) {
    auto sit = pr.uniforms.find(l);
    if (sit == pr.uniforms.end()) {
        // "location is not a valid location for program" → INVALID_OPERATION.
        c.errors.Record(0x0502);
        return false;
    }
    const ProgramObject::UniformSlot::Spec& sp = sit->second.spec;
    const size_t storeBytes = sit->second.bytes.size();
    const uint32_t storeType = (sp.storeType ? sp.storeType
                                              : (UniformTypeIsDouble(sp.type) ? 0x140A : 0x1406));
    const int n = UniformStoreElemCount(sp, storeBytes, storeType);
    const bool isDoubleStore = (storeType == 0x140A);

    // Chuyển từ kiểu lưu sang kiểu yêu cầu.
    double* dout = (double*)out;
    float* fout = (float*)out;
    GLint* iout = (GLint*)out;
    GLuint* uout = (GLuint*)out;
    const uint8_t* src = sit->second.bytes.empty() ? nullptr : sit->second.bytes.data();
    for (int i = 0; i < n; ++i) {
        double dv = 0.0;
        if (src) {
            // Đọc theo ĐÚNG kiểu đã lưu (float/int/uint/double) — trước đây
            // mọi thứ trừ double đều bị đọc như float ⇒ giá trị int/uint sai.
            if (isDoubleStore) {
                double tmp;
                memcpy(&tmp, src + (size_t)i * sizeof(double), sizeof(double));
                dv = tmp;
            } else if (storeType == 0x1404) {
                GLint tmp;
                memcpy(&tmp, src + (size_t)i * sizeof(GLint), sizeof(GLint));
                dv = (double)tmp;
            } else if (storeType == 0x1405) {
                GLuint tmp;
                memcpy(&tmp, src + (size_t)i * sizeof(GLuint), sizeof(GLuint));
                dv = (double)tmp;
            } else {
                float tmp;
                memcpy(&tmp, src + (size_t)i * sizeof(float), sizeof(float));
                dv = (double)tmp;
            }
        }
        switch (want) {
            case 0x140A: dout[i] = dv; break;
            case 0x1404: iout[i] = (GLint)dv; break;
            case 0x1405: uout[i] = (GLuint)dv; break;
            default: fout[i] = (float)dv; break;
        }
    }
    return true;
}

// Kiểm tra program theo spec cho GetUniform*.
ProgramObject* ProgramForGetUniform(Context& c, GLuint p) {
    auto it = c.programs.find(p);
    if (it != c.programs.end()) {
        if (!it->second.linked) { c.errors.Record(0x0502); return nullptr; }
        return &it->second;
    }
    if (c.shaders.count(p)) { c.errors.Record(0x0502); return nullptr; }
    c.errors.Record(0x0501);  // không phải program/shader → INVALID_VALUE
    return nullptr;
}

void glGetUniformfv(GLuint p, GLint l, GLfloat* v) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x1406, v);
}
void glGetUniformiv(GLuint p, GLint l, GLint* v) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x1404, v);
}
void glGetUniformuiv(GLuint p, GLint l, GLuint* v) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x1405, v);
}
void glGetUniformdv(GLuint p, GLint l, GLdouble* v) {
    Context& c = Context::Current();
    ProgramObject* pr = ProgramForGetUniform(c, p);
    if (!pr) return;
    ReadUniformValueTyped(c, *pr, l, 0x140A, v);
}
// NOTE: glGetnUniform* định nghĩa duy nhất ở gl_packed_robust.cpp (có bufSize guard).
// Không định nghĩa ở đây để tránh duplicate-symbol khi link strict (iOS).
} // namespace tglmt::gl
