#pragma once
// LegacyDraw — đường draw fixed-function (OpenGL Compatibility Profile).
//
// TGLMT đã có ĐẦY ĐỦ state fixed-function (LegacyState: ngăn xếp ma trận,
// current vertex attributes, lighting, fog, texenv/texgen) và đã ghép
// primitive immediate-mode (src/gl/gl_legacy_vertex.cpp:100 AssemblePrimitives)
// — nhưng CHƯA có đường vẽ: glEnd chỉ gom data, không rasterize.
//
// File này + src/gl/gl_legacy_draw.cpp là đường đó. Kiến trúc bám sát
// docs/LEGACY_TO_METAL_MAPPING.md §2 (state block / generated shader /
// pipeline key / coexistence / uniform upload), §4 (ma trận), §5 (lighting),
// §6 (fog), §8 (texture env + texgen):
//
//   • Chương trình fixed-function là MỘT cặp metal::ILibrary (VS + FS) sinh ra,
//     nằm NGOÀI Context::programs ⇒ KHÔNG phải GL program object (không xuất
//     hiện trong glGetProgramiv, glUseProgram không chọn được) —
//     xem Context::legacyProgram + Context::legacy.program.
//   • PipelineKey hiện có (vsLib/fsLib/programGen + attribs[]) ĐÃ đủ để tách
//     chương trình legacy khỏi mọi program modern ⇒ KHÔNG sửa Context::PipelineKey.
//   • Giá trị state (light, material, fog, ma trận, texenv colour) đi qua ĐÚNG
//     đường uniform staging sẵn có (pr.uniformLayout + pr.uniforms ⇒
//     src/gl/gl_apple_draw.cpp:2817-2941) — không nhân bản cơ chế.
//   • "Hình dạng" chương trình (lighting bật/tắt, số texture unit, mode của
//     texture env, ...) được BAKED vào MSL ⇒ đổi hình dạng ⇒ sinh lại
//     chương trình (linkGen mới ⇒ pipeline mới trong PipelineKey).
//   • Vertex data: immediateVertices/immediatePrimitives → 1 vertex buffer +
//     1 index buffer, layout vertex CỐ ĐỊNH (6 × float4) ⇒ PipelineKey::stride
//     ổn định.
#include <cstdint>
#include <string>

namespace tglmt {

class Context;
struct ProgramObject;

namespace gl {

// Số texture unit mà đường fixed-function hỗ trợ.
// LegacyVertex chỉ giữ toạ độ texture của unit 0 (texCoord) và unit 1
// (texCoord1) — xem include/tglmt/LegacyState.h:110. TexGen vẫn sinh được
// toạ độ cho unit 0..1 (tính trên CPU trước khi đẩy lên GPU).
constexpr int kLegacyTexUnits = 2;

// Layout vertex của đường fixed-function (byte offset trong 1 đỉnh).
// 6 × float4 = 96 byte. Cố định ⇒ PipelineKey::stride ổn định, pipeline
// cache không bị phân mảnh.
struct LegacyVertexLayout {
    static constexpr uint32_t kStride = 96;
    static constexpr uint32_t kPos = 0;        // float4 (x, y, z, w)
    static constexpr uint32_t kNormal = 16;    // float4 (xyz)
    static constexpr uint32_t kColor = 32;     // float4
    static constexpr uint32_t kTex0 = 48;      // float4
    static constexpr uint32_t kTex1 = 64;      // float4
    static constexpr uint32_t kSecColor = 80;  // float4
};

// "Hình dạng" chương trình fixed-function: các đặc trưng được BAKED vào MSL.
// Đổi shape ⇒ sinh lại chương trình (linkGen tăng ⇒ pipeline mới).
struct LegacyProgramShape {
    bool lighting = false;         // GL_LIGHTING
    bool fog = false;              // GL_FOG
    bool alphaTest = false;        // GL_ALPHA_TEST
    bool separateSpecular = false; // GL_LIGHT_MODEL_COLOR_CONTROL
    bool twoSided = false;         // GL_LIGHT_MODEL_TWO_SIDE
    bool normalize = false;        // GL_NORMALIZE
    bool rescaleNormal = false;    // GL_RESCALE_NORMAL
    bool flatShading = false;      // GL_FLAT (shade model) — varying màu dùng [[flat]]
    int numTexUnits = 0;           // số unit có texture (0..kLegacyTexUnits)
    // Mode của texture environment từng unit (GL_REPLACE/MODULATE/DECAL/
    // BLEND/ADD/GL_COMBINE) — BAKED.
    uint32_t texEnvMode[kLegacyTexUnits] = {0, 0};
    // Hàm combiner (chỉ dùng khi mode == GL_COMBINE) — BAKED.
    uint32_t combineRGB[kLegacyTexUnits] = {0, 0};
    uint32_t combineAlpha[kLegacyTexUnits] = {0, 0};
    // Quy tắc ánh xạ texel → (Cs, As) theo base format của texture — BAKED.
    uint32_t srcRule[kLegacyTexUnits] = {0, 0};
    // Quy tắc Av (alpha) theo base format + mode — BAKED.
    uint32_t alphaRule[kLegacyTexUnits] = {0, 0};
    // Bitmask glColorMaterial: bit0=ambient, bit1=diffuse, bit2=specular,
    // bit3=emission (áp cho cả front+back; mặt điều khiển riêng xem
    // LegacyColorMaterialMask()).
    uint32_t colorMaterialMask = 0;
    uint32_t colorMaterialMaskBack = 0;
    uint64_t key = 0;  // chữ ký của shape (dùng để quyết định sinh lại)
};

// Context có được phép dùng fixed-function không?
// Chỉ Compatibility Profile (và profile "legacy" của GL < 3.2). Core Profile
// không có fixed-function (GL 4.6 Core §17.1) ⇒ hành vi cũ (noProgram).
bool LegacyFixedFunctionAllowed(const Context& c);

// Chọn chương trình cho MỘT draw của đường draw dùng chung.
// Trả true (và ghi con trỏ chương trình vào `out`) khi draw đó phải đi qua
// đường fixed-function, tức là:
//   • KHÔNG có program object nào trong use (GL 2.1 §2.15 / GL 4.6 Core
//     §1.2.4: "if a program object is in use, the programmable pipeline
//     runs; otherwise fixed-function is used"), và
//   • context cho phép fixed-function (không phải Core Profile), và
//   • draw này LÀ draw fixed-function thật (batch immediate-mode do
//     LegacyDrawImmediate phát ⇒ Context::legacy.drawActive).
// Một draw MODERN (glDrawArrays/glDrawElements) không có program vẫn KHÔNG
// được đi qua đường này: vertex data của nó nằm ở vertex array của app, không
// nằm trong layout vertex legacy ⇒ nếu đi nhầm sẽ đọc buffer legacy stale.
bool LegacySelectProgram(Context& c, GLuint program, ProgramObject*& out);

// Tính "hình dạng" hiện tại từ state fixed-function.
LegacyProgramShape LegacyComputeShape(const Context& c);

// Sinh (hoặc tái dùng) chương trình fixed-function: MSL + uniform layout +
// compile library. Trả false nếu không sinh được.
bool LegacyEnsureProgram(Context& c);

// Đóng gói state fixed-function vào uniform block của chương trình
// (pr.uniformLayout + pr.uniforms) ⇒ đi qua đường staging sẵn có.
void LegacyUploadUniforms(Context& c);

// Dựng vertex buffer + index buffer từ immediateVertices/immediatePrimitives.
bool LegacyBuildGeometry(Context& c);

// Vẽ batch immediate-mode đang chờ (immediateVertices + immediatePrimitives).
// Đây là hàm mà glEnd() phải gọi — xem docs/LEGACY_DRAW_PATH.md §"Follow-up
// bắt buộc" (src/gl/gl_legacy_vertex.cpp ngoài write-scope của task này).
bool LegacyDrawImmediate(Context& c);

// Vẽ batch immediate-mode nếu có (dùng để flush trước một draw khác, giữ
// đúng thứ tự GL). Không làm gì nếu batch đã vẽ hoặc không có batch.
bool LegacyFlushPendingImmediate(Context& c);

}  // namespace gl
}  // namespace tglmt
