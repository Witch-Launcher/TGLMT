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
//    gl_FragCoord (fragment → param [[position]]),
//    gl_FrontFacing (fragment → param [[front_facing]], entity PER_FACE_LIGHTING)
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
    // P-Tess T1: mảng per-vertex TCS `in/out T x[]` (unsized) → -1 lúc parse,
    // chuẩn hoá = tessVertices sau đó. SSBO unsized cũng dùng -1 (quy ước chung).
    bool isPatch = false;  // P-Tess: `patch out/in` (per-patch, khác per-vertex)
    bool isSampler = false;
    bool isBuffer = false;       // samplerBuffer/isamplerBuffer/usamplerBuffer
    std::string sampleType = "float"; // float/int/uint (cho buffer + read)
    bool isCube = false;   // samplerCube → texturecube (panorama)
    bool isArray = false;  // sampler2DArray → texture2d_array
    bool isShadow = false; // sampler2DShadow → texture2D approx (bỏ compare ref)
    // P6g image: `layout(rgba8[, binding=N]) uniform [readonly|writeonly] image2D img;`
    // → texture2d<T, access::RW> (imageLoad/Store → .read/.write). access: 'R' read,
    // 'W' write, 'B' read_write (mặc định). binding: layout binding, -1 = glUniform1i.
    bool isImage = false;
    char imageAccess = 'B';
    int imageBinding = -1;
    // P9 atomic counter: `layout(binding=N, offset=M) uniform atomic_uint ac;`
    // → device atomic_uint* (atomicCounterIncrement/Decrement/Get → MSL atomics).
    bool isAtomicCounter = false;
    int atomicBinding = -1;
    int atomicOffset = 0;
};
// Uniform block (UBO read-only): `layout(std140[, binding=N]) uniform Block { ... }`.
struct GLSLBlock {
    std::string name;
    std::vector<GLSLVar> members;
    size_t bufferSize = 0;
    int binding = -1; // layout(binding=N): -1 = không khai báo (mặc định 0)
    // P6f SSBO: block `buffer B { ... };` → device (read-write), khác UBO (constant).
    // member có mảng không kích thước (float v[]) → arraySize = -1 (MSL: device T*).
    bool isStorage = false;
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
    std::vector<GLSLVar> images;     // P6g: image vars (thứ tự khai báo = texture slot)
    std::vector<GLSLVar> atomicCounters; // P9: atomic_uint vars (buffer slot)
    std::vector<GLSLBlock> blocks;   // UBO blocks (read-only)
    size_t uniformBufferSize = 0;    // sizeof(TGLMTUniforms), đã align
    bool usesPointSize = false;
    bool usesPointCoord = false;
    bool usesFragCoord = false;
    bool usesFrontFacing = false;
    // screenquad vanilla không attribute, dựng đỉnh từ gl_VertexID/gl_InstanceID.
    bool usesVertexID = false;
    bool usesInstanceID = false;
    bool isVertex = true;
    // P6f: compute shader (stage 0x91B9). localSize = workgroup size đọc từ
    // `layout(local_size_x/y/z) in;` — backend cần để dispatch threadsPerThreadgroup.
    bool isCompute = false;
    int localSize[3] = {1, 1, 1};
    // P-Tess: tessellation control (0x8E88) / evaluation (0x8E87, T2).
    // TCS → MSL kernel (MoltenVK/SPIRV-Cross pattern): thread = (invocation, patch).
    bool isTessControl = false;
    bool isTessEval = false;
    // layout(vertices=N) out (TCS bắt buộc): số invocation/patch, stride cho
    // buffer per-vertex. T4 dispatch grid=(N, numPatches,1), threadgroup=(N,1,1).
    int tessVertices = 0;
    // Buffer slot cho tess I/O (T4 bind theo bảng này; tess I/O không có GL binding).
    struct TessIOSlot {
        std::string name;   // tên var GLSL (gl_in.* ghi "gl_Position"/"gl_PointSize")
        int slot = -1;      // [[buffer(slot)]]
        bool isInput = false;
        bool isPatch = false;
        bool isGlIn = false; // từ khối gl_in (vị trí VS stage, T4 điền)
        std::string mslType; // kiểu phần tử (cho T4 tính sizeof)
        int count = 0;       // số phần tử mỗi patch (= tessVertices hoặc size patch array)
    };
    std::vector<TessIOSlot> tessSlots;
    // Factor buffer (MTLTriangleTessellationFactorsHalf, 8B/patch): -1 = không dùng.
    int tessFactorSlot = -1;
    // P6f backend: map block → MSL buffer slot. GL bind theo bindingPoint
    // (glBindBufferBase), converter gán slot theo thứ tự khai báo → backend
    // cần bảng này để bind storageBindPoints[binding] → buffer(slot).
    struct BlockSlot { int binding; int slot; bool isStorage; std::string name; };
    std::vector<BlockSlot> blockSlots;
    // P9 atomic: (tên, binding, offset, slot) — backend bind atomicBindPoints[binding]→buffer(slot+offset).
    struct AtomicSlot { std::string name; int binding; int offset; int slot; };
    std::vector<AtomicSlot> atomicSlots;
};

// Convert 1 shader stage. stage = 0x8B31 (VERTEX), 0x8B30 (FRAGMENT),
// 0x91B9 (COMPUTE), 0x8E88 (TESS_CONTROL). 0x8E87 (TESS_EVAL, T2) hiện fail rõ.
GLSLConvertResult ConvertGLSLtoMSL(const std::string& glsl, uint32_t stage);

} // namespace tglmt
