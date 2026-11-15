#pragma once
// GeometryEmulation.h — Emulation GEOMETRY SHADER (GS) cho TGLMT (quyết định A6).
//
// VÌ SAO CÓ FILE NÀY
// ------------------
// Metal KHÔNG có stage geometry shader. Theo A6 (docs/DECISIONS.md) GS BẮT BUỘC
// phải được emulation, không được kết luận HARDWARE_LIMITED. Trước đây TGLMT
// link program có GS OK (src/gl/gl_program.cpp) rồi draw path TỪ CHỐI vẽ
// (src/gl/gl_apple_draw.cpp) ⇒ "link OK nhưng vẽ câm" — vi phạm chính nếp P6f
// của repo (fail rõ tại link).
//
// THIẾT KẾ (giống tiền lệ tessellation: TCS → MSL compute kernel)
// --------------------------------------------------------------
//   VS (compute prepass "capture")  ──► buffer capture (VS output theo vertex)
//            │
//            ▼
//   GS → MSL compute kernel          ──► buffer output MỞ RỘNG (GS output)
//   (1 thread / 1 input primitive)   ──► buffer index (point/line/triangle list)
//            │
//            ▼
//   passthrough vertex (đọc buffer)  ──► fragment shader
//
// Spec: OpenGL 4.2 Compatibility §2.16 (Geometry Shaders) — bản GL 4.6 Core
// tương ứng là §11.3; GLSL 3.20 §4.3.8.1/§4.3.8.2 (layout qualifier) và
// §8.10 (EmitVertex/EndPrimitive). Chi tiết + bảng ánh xạ: docs/GEOMETRY_SHADER_EMULATION.md.
//
// LƯU Ý PHẠM VI: file này KHÔNG được thêm field vào Context.h (write-scope của
// task), nên thông tin program GS nằm trong bảng side-table (GsProgramStore)
// trong src/gl/gl_geometry_emulation.cpp, khoá theo (program id, linkGen).
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "tglmt/gl46_types.h"
#include "tglmt/MetalInterface.h"

namespace tglmt {

class Context;   // forward decl (định nghĩa đầy đủ trong tglmt/Context.h)

// ---- Kiểu primitive của GS (GLSL layout keyword) ----------------------------
// Input  (GLSL 3.20 §4.3.8.1): points, lines, lines_adjacency, triangles,
//                                  triangles_adjacency.
// Output (GLSL 3.20 §4.3.8.2): points, line_strip, triangle_strip  (chỉ 3 loại,
//   theo OpenGL 4.2C §2.16.2 — "lines"/"triangles" KHÔNG hợp lệ làm output).
enum class GsPrimType : uint8_t {
    Unknown = 0,
    Points,
    Lines,
    LinesAdjacency,
    Triangles,
    TrianglesAdjacency,
    LineStrip,       // chỉ dùng làm OUTPUT
    TriangleStrip,   // chỉ dùng làm OUTPUT
};

// Số đỉnh của MỘT primitive input (OpenGL 4.2C §2.16.1).
inline int GsPrimInputVertices(GsPrimType t) {
    switch (t) {
        case GsPrimType::Points: return 1;
        case GsPrimType::Lines: return 2;
        case GsPrimType::LinesAdjacency: return 4;
        case GsPrimType::Triangles: return 3;
        case GsPrimType::TrianglesAdjacency: return 6;
        default: return 0;
    }
}
// Số chỉ số index sinh ra cho MỘT primitive output (triangle/line list).
inline int GsIndicesPerPrim(GsPrimType out) {
    switch (out) {
        case GsPrimType::Points: return 1;
        case GsPrimType::LineStrip:
        case GsPrimType::Lines: return 2;
        case GsPrimType::TriangleStrip:
        case GsPrimType::Triangles: return 3;
        default: return 0;
    }
}
// Tên GLSL của layout qualifier (để log/báo lỗi).
const char* GsPrimTypeName(GsPrimType t);
// Parse layout keyword → enum. Trả Unknown nếu không hợp lệ.
GsPrimType GsPrimTypeFromGLSL(const std::string& kw);
// Enum GL tương ứng (GL_POINTS/GL_LINES/GL_TRIANGLES/GL_LINE_STRIP/
// GL_TRIANGLE_STRIP) — dùng cho glGetProgramiv(GL_GEOMETRY_INPUT_TYPE/OUTPUT_TYPE).
GLenum GsPrimTypeToGL(GsPrimType t);

// ---- Biến của GS ------------------------------------------------------------
struct GsVar {
    std::string name;
    std::string glslType;   // "vec4"
    std::string mslType;    // "float4"
    std::string interp;     // "" (smooth) | "flat" | "noperspective" | "centroid"
};

// Một field của vertex output mở rộng. MỖI field = MỘT buffer riêng ⇒ không
// phải tính layout struct MSL (float3 có stride 12 nhưng align 16 — dễ lệch).
struct GsOutField {
    std::string name;      // "gl_Position", "gl_PointSize", "gl_Layer",
                           // "gl_ViewportIndex" hoặc tên output của user
    std::string mslType;   // float4 / float / uint / ...
    size_t elemSize = 0;   // byte mỗi phần tử trong buffer
    bool isUser = false;   // true = output do user khai báo
};

// Field mà FS cần (đã gán location GL ở đường link chung).
struct GsFsField {
    std::string name;
    std::string mslType;
    std::string interp;     // "" (smooth) | "flat" | "noperspective" | "centroid"
    int location = -1;
};

// ---- Kết quả dịch GS → MSL compute kernel ----------------------------------
struct GsTranslation {
    bool ok = false;
    std::string log;          // lý do fail (link fail với message này)
    GsPrimType inPrim = GsPrimType::Unknown;
    GsPrimType outPrim = GsPrimType::Unknown;
    int maxVertices = 0;      // layout(max_vertices = N)
    int invocations = 1;      // layout(invocations = N) — >1 chưa hỗ trợ
    std::vector<GsVar> inputs;    // `in` của GS (mảng per-vertex)
    std::vector<GsVar> outputs;   // `out` của GS (per-vertex)
    std::vector<GsVar> uniforms;  // `uniform` của GS (không sampler)
    // Buffer capture mà kernel GS đọc: gl_Position, gl_PointSize, rồi tới các
    // input của GS (thứ tự khai báo). Thứ tự này = thứ tự slot [[buffer()]].
    std::vector<GsVar> bindings;
    // Field output mở rộng (mỗi field 1 buffer).
    std::vector<GsOutField> outFields;
    // Phân tích TĨNH số lần EmitVertex/EndPrimitive (để biết trước số vertex /
    // primitive mỗi input primitive ⇒ counter & sizing không cần đoán).
    bool staticEmission = false;  // true nếu mọi Emit/End nằm trên đường thẳng
    int emitCalls = 0;            // số lệnh EmitVertex() trong thân main
    int endCalls = 0;             // số lệnh EndPrimitive() trong thân main
    int vertsPerInput = 0;        // tổng vertex emit / input primitive (static)
    int primsPerInput = 0;        // số primitive output / input primitive (static)
    int staticIndexPerPrim = 0;   // số index / input primitive (static) — layout index cố định
    int maxIndexPerInput = 0;     // số index TỐI ĐA / input primitive (trường hợp động)
    std::string msl;              // kernel MSL hoàn chỉnh
};

// Dịch 1 GS source → thông tin + MSL compute kernel.
// `uniformStructMsl`: text "struct TGLMTUniforms { ... };" lấy từ MSL của VS
//   (dùng chung layout uniform của stage đó — GS dùng chung uniform với VS).
// `vsOutputs`: mapping tên varying (VS output) → kiểu phần tử.
// Trả về ok=false + log khi GS nằm ngoài subset (link phải fail rõ).
// `interfaceCheck` = false ⇒ BỎ các lỗi phụ thuộc interface (VS output,
// uniform struct dùng chung). Dùng cho compile-time: lúc glCompileShader chưa
// biết VS/FS nên chỉ kiểm tra cú pháp + định dạng GS.
GsTranslation TranslateGeometryShader(const std::string& glsl,
                                      const std::string& uniformStructMsl,
                                      const std::vector<GsVar>& vsOutputs,
                                      bool interfaceCheck = true);

// ---- Kế hoạch mở rộng (CPU, trước khi chạy GPU) ----------------------------
struct GsPlan {
    bool modeCompatible = false;  // mode draw vs input primitive của GS (§2.16.1)
    uint64_t inputPrimitives = 0;  // primitive input mỗi instance
    uint64_t invocations = 0;      // số lần GS được gọi (query GS_INVOCATIONS)
    uint64_t outputVertices = 0;   // vertex đầu ra (mở rộng)
    uint64_t outputPrimitives = 0; // primitive đầu ra (query GS_PRIMITIVES_EMITTED)
    bool exact = false;            // true nếu outputVertices/Primitives biết chắc
    bool dynamic = false;          // true nếu emission phụ thuộc dữ liệu
    uint32_t mode = 0;             // mã ghép primitive (xem kGsMode* trong .cpp)
};
// Tính kế hoạch cho 1 draw. `count` = số đỉnh, `inst` = số instance.
GsPlan PlanGeometryExpansion(const GsTranslation& gs, GLenum drawMode, GLsizei count,
                             GLsizei inst);

// ---- Thông tin program GS (side-table) --------------------------------------
struct GsProgramInfo {
    GLuint program = 0;
    uint64_t linkGen = 0;
    GsTranslation gs;
    // Field FS cần (từ GS output) + location GL (link path gán).
    std::vector<GsFsField> fsFields;
    // Metal objects (chỉ có khi device thật).
    std::shared_ptr<metal::ILibrary> gsLib, capLib, passLib, fsLib;
    std::shared_ptr<metal::IComputePipeline> gsPipe, capPipe;
};

// Buffer + tham số cho lượt vẽ cuối (passthrough vertex + FS).
struct GsDrawBuffers {
    // Buffer output mở rộng theo thứ tự field: gl_Position trước, rồi các
    // field FS đọc (đúng thứ tự [[buffer()]] của passthrough vertex).
    std::vector<metal::IBuffer*> outBufs;
    metal::IBuffer* idx = nullptr;
    metal::IBuffer* fsUniform = nullptr;   // uniform buffer của FS (buffer(16))
    uint32_t indexCount = 0;
    metal::PrimitiveType prim = metal::PrimitiveType::Triangle;
};

// Sinh MSL passthrough vertex (đọc buffer GS output → trả varying cho FS).
// `fsFields` = các field FS cần, đã gán location GL (link path gán).
std::string BuildGeometryPassthroughMsl(const GsTranslation& gs,
                                        const std::vector<GsFsField>& fsFields);

namespace gl {

// Ghi/đọc side-table program GS (linkGen để tránh dùng info cũ sau relink).
void StoreGeometryProgramInfo(const GsProgramInfo& info);
bool GetGeometryProgramInfo(GLuint program, uint64_t linkGen, GsProgramInfo* out);
void DropGeometryProgramInfo(GLuint program);

// Program đang bind có GS hay không (dùng ở draw path).
bool GeometryStageActive(Context& c);

// Đường draw của GS: accounting (counter query) + mở rộng thật trên GPU.
// Trả true nếu đã encode draw thật (Apple). Trên backend không có GPU
// (Null) chỉ cập nhật counter, trả false (không có gì để vẽ).
bool GeometryStageDraw(Context& c, GLenum mode, GLsizei count, GLsizei inst, bool indexed,
                       GLenum indexType, const void* indexData, size_t indexByteOff,
                       GLuint eboId, GLint baseVertex, GLuint baseInstance, GLint first);

// Encode lượt vẽ cuối (passthrough vertex + FS) — CÀI ĐẶT trong
// src/gl/gl_apple_draw.cpp (cần sampler/texture machinery của draw path).
bool AppleDrawGeometryExpanded(Context& c, GLuint program, const GsProgramInfo& info,
                               const GsDrawBuffers& bufs);

} // namespace gl
} // namespace tglmt
