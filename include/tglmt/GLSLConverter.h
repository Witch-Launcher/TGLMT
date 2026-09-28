#pragma once
// GLSLConverter — dịch GLSL ES 3.0/GL 4.60 (subset có tài liệu) sang MSL.
// KHÔNG đoán: ngoài subset sẽ báo lỗi rõ ràng (ok=false + log). Mọi shader của
// app Aquarium nằm trong subset; test pixel-proof so với spirv-cross.
//
// Subset hỗ trợ (vertex + fragment):
//  - `#version xxx` (bỏ qua, chấp nhận 300 es / 460 core)
//  - `layout(location=N) in/out` với float, vec2/3/4, mat4 (+ uint/int biến thể)
//  - `uniform` float/vec/mat + sampler2D (uniform block KHÔNG hỗ trợ → lỗi rõ)
//  - varyings in/out khớp location 2 stage; gl_Position/gl_PointSize (vertex),
//    gl_PointCoord (fragment → param [[point_coord]]),
//    gl_FragCoord (fragment → param [[position]])
//  - `texture(sampler, uv)` [, bias] → .sample(); texelFetch → LỖI rõ
//  - `discard;` → `discard_fragment();`
//  - hàm/thân lệnh/biểu thức: pass-through (MSL tương thích C-like), trừ tên
//    varying/uniform/attribute được viết lại thành struct access.
//  - global `const` → thêm `constant`; global không-const → LỖI rõ.
// Quy ước sinh: vertex fn `TGLMT_vs`, fragment fn `TGLMT_fs`; varyings qua
// `[[user(locnN)]]` (đúng cách spirv-cross nối stage); uniforms vào
// `constant TGLMTUniforms& tglmt_u [[buffer(16)]]`; vertex input `TGLMT_VIn
// ... [[stage_in]]`; fragment input `TGLMT_FIn ... [[stage_in]]`.
// Kỷ luật code (không đoán mò): khai báo trước hàm; tên biến cục bộ/hàm helper
// không trùng tên varying/uniform/attribute; main không `return` sớm có giá trị
// (về `return;` trần được viết lại); không mảng/uniform-block/double/texelFetch.
#include <cstdint>
#include <string>
#include <vector>

namespace tglmt {

struct GLSLVar {
    std::string glslType;   // "vec3", "mat4", "sampler2D", ...
    std::string name;
    int location = -1;      // explicit layout, hoặc >=kTempLocBase (linker gán sau)
    std::string mslType;    // "float3", "float4x4", ...
    size_t uniformOffset = 0; // chỉ cho uniforms
    size_t uniformSize = 0;
    int arraySize = 0;      // 0 = không mảng, >0 = uniform array[N] (vanilla/Sodium)
    bool isSampler = false;
    bool isBuffer = false;       // samplerBuffer/isamplerBuffer/usamplerBuffer
    std::string sampleType = "float"; // float/int/uint (cho buffer + read)
    bool isCube = false;   // samplerCube → texturecube (panorama)
    bool isArray = false;  // sampler2DArray → texture2d_array
    bool isShadow = false; // sampler2DShadow → texture2D approx (bỏ compare ref)
};
// Uniform block (UBO read-only): `layout(std140) uniform Block { members }`.
struct GLSLBlock {
    std::string name;
    std::vector<GLSLVar> members;
    size_t bufferSize = 0;
};
// Location tạm cho varying/attribute thiếu layout: LinkProgram viết lại số thật.
constexpr int kTempLocBase = 900;

struct GLSLConvertResult {
    bool ok = false;
    std::string log;
    std::string msl;                 // 1 stage (gọi riêng cho vs/fs)
    std::string entryPoint = "TGLMT_vs"; // hoặc TGLMT_fs
    std::vector<GLSLVar> inputs;     // attributes (vs) / varyings-in (fs)
    std::vector<GLSLVar> outputs;    // varyings-out (vs) / color-out (fs)
    std::vector<GLSLVar> uniforms;   // cả sampler2D (isSampler=true)
    std::vector<GLSLVar> samplers;   // tách riêng cho tiện bind
    std::vector<GLSLBlock> blocks;   // UBO blocks (read-only)
    size_t uniformBufferSize = 0;    // sizeof(TGLMTUniforms), đã align
    bool usesPointSize = false;
    bool usesPointCoord = false;
    bool usesFragCoord = false;
    // screenquad vanilla không attribute, dựng đỉnh từ gl_VertexID/gl_InstanceID.
    bool usesVertexID = false;
    bool usesInstanceID = false;
    bool isVertex = true;
};

// Convert 1 shader stage. stage = 0x8B31 (VERTEX) hoặc 0x8B30 (FRAGMENT).
GLSLConvertResult ConvertGLSLtoMSL(const std::string& glsl, uint32_t stage);

} // namespace tglmt
