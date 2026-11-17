// glsl_builtins_modern_io.cpp — nhóm built-in BIẾN GLSL "modern_io".
//
// GLSL 4.60 §7 "Built-In Variables" (docs/khronos/GLSLangSpec.4.60.pdf) — các
// biến built-in mà MSL (Metal Shading Language) diễn đạt bằng ATTRIBUTE ĐẶC
// BIỆT, không phải varying `[[user(locnN)]]` thuần.
//
//   GLSL                            MSL tương đương                     Nguồn
//   ------------------------------------------------------------------------------
//   gl_FragDepth       (FS out)    float depth [[depth(any)]]           converter (có sẵn)
//   gl_SampleMask[0]   (FS out)    uint  smask [[sample_mask]]           converter (có sẵn)
//   gl_SampleMaskIn[0] (FS in)     uint  x [[sample_mask]]               FILE NÀY
//   gl_Layer           (FS in)     uint  x [[render_target_array_index]] FILE NÀY
//   gl_ViewportIndex   (FS in)     uint  x [[viewport_array_index]]      FILE NÀY
//   gl_Layer           (VS out)    uint  x [[render_target_array_index]] FILE NÀY
//   gl_ViewportIndex   (VS out)    uint  x [[viewport_array_index]]      FILE NÀY
//   gl_PrimitiveID     (FS in)     uint  x [[primitive_id]]              FILE NÀY
//   gl_NumSamples      (FS uniform) — MSL KHÔNG CÓ attribute tương đương  xem "GIỚI HẠN"
//   gl_BaseVertex      (VS in)     uint  x [[base_vertex]] (param hàm)   xem "GIỚI HẠN"
//   gl_BaseInstance    (VS in)     uint  x [[base_instance]] (param hàm)  converter (có sẵn)
//   gl_DrawID          (VS in)     — MSL không có [[draw_index]]         xem "GIỚI HẠN"
//   gl_ClipDistance[1] (VS out)    Metal chỉ có MỘT [[clip_distance]]   xem "GIỚI HẠN"
//
// SPEC (GLSL 4.60 — số trang là trang PDF của docs/khronos/GLSLangSpec.4.60.pdf):
//   §7.1.1 "Vertex Shader Special Variables" (PDF p.143-145)
//       in int gl_DrawID;  in int gl_BaseVertex;  in int gl_BaseInstance;
//       out gl_PerVertex { vec4 gl_Position; float gl_PointSize;
//                          float gl_ClipDistance[]; float gl_CullDistance[]; };
//   §7.1.4 "Geometry Shader Special Variables" (PDF p.147-148)
//       out int gl_PrimitiveID;  out int gl_Layer;  out int gl_ViewportIndex;
//   §7.1.5 "Fragment Shader Special Variables" (PDF p.149-152)
//       in  int gl_PrimitiveID;  in  int gl_SampleMaskIn[];
//       in  int gl_Layer;         in  int gl_ViewportIndex;
//       out float gl_FragDepth;   out int gl_SampleMask[];
//   §7.1.5 (PDF p.150): "The input variable gl_SampleMaskIn indicates the set of
//       samples covered by the primitive generating the fragment during
//       multisample rasterization."
//   §7.1.5 (PDF p.150): "The output array gl_SampleMask[] sets the sample mask
//       for the fragment being processed. Coverage for the current fragment
//       will become the logical AND of the coverage mask and the output
//       gl_SampleMask. [...] If a shader does not statically assign a value to
//       gl_SampleMask, the sample mask has no effect on the processing of a
//       fragment."
//   §7.1.5 (PDF p.151): "The input variable gl_PrimitiveID is filled with the
//       value written to the gl_PrimitiveID geometry shader output, if a
//       geometry shader is present. Otherwise, it is filled with the number of
//       primitives processed by the shader since the current set of rendering
//       primitives was started."
//   §7.1.5 (PDF p.151): "The input variable gl_Layer is filled with the value
//       written to the gl_Layer geometry shader output [...] If the geometry
//       stage makes no static assignment to gl_Layer, the input value in the
//       fragment stage will be zero."
//   §7.4 "Built-In Uniform State" (PDF p.159): "uniform int gl_NumSamples;"
//       + "These variables are only guaranteed to be available in the fragment
//       stage."
//   OpenGL 4.6 Core §9.8 "Layered Framebuffers" (glspec46.core.pdf PDF p.357):
//       "The layer number for a fragment is zero if • geometry shaders are
//       disabled, or • the current geometry shader does not statically assign
//       a value to the built-in output variable gl_Layer."  ⇒ trong GL,
//       gl_Layer là output của GEOMETRY SHADER, không phải của vertex shader;
//       bản VS ở đây là đường tương đương MSL (Metal không có GS ⇒
//       [[render_target_array_index]] là output của vertex function).
//
// MSL (Metal Shading Language Specification 4.x):
//   Table 5.2 — vertex function INPUT attributes: base_instance, base_vertex,
//                instance_id, vertex_id.
//   Table 5.4 — vertex function OUTPUT (return struct) attributes: position,
//                point_size, clip_distance, render_target_array_index,
//                viewport_array_index.
//   Table 5.5 — fragment function INPUT attributes: sample_id, sample_mask
//                ("The set of samples covered by the primitive generating the
//                fragment during multisample rasterization"), primitive_id,
//                render_target_array_index, viewport_array_index.
//   Table 5.7 — fragment function OUTPUT (return struct) attributes:
//                color(m), depth, sample_mask.
//
// ĐO THẬT (macOS 14, GPU Intel — nhãn môi trường OTHER_GPU, KHÔNG phải A11;
//   đo bằng Metal compiler + MTLRenderPipelineState thật, không phải suy đoán):
//   (1) `uint p [[primitive_id]] [[user(locnN)]] [[flat]]` trong struct FIn:
//       giá trị đọc được là PRIMITIVE ID (0 cho primitive đầu, 1 cho primitive
//       thứ hai) — attribute đặc biệt THẮNG varying user(locnN).
//   (2) `uint m [[sample_mask]] [[user(locnN)]] [[flat]]`: giá trị = mặt nạ phủ
//       thật (0x1 với target 1 sample) ⇒ đúng ngữ nghĩa GLSL gl_SampleMaskIn[0].
//   (3) `[[viewport_array_index]]` VS out → FS in: giá trị truyền đúng (9).
//       Kể cả khi SỐ LOCN KHÁC NHAU giữa 2 bên (Metal ghép theo attribute, không
//       theo locn) ⇒ 2 stage có thể dùng location tạm khác nhau, vẫn đúng.
//   (4) `[[render_target_array_index]]` trong VS out ⇒ Metal pipeline creation
//       FAIL nếu pipeline descriptor không khai `inputPrimitiveTopology`
//       ("Vertex shader writes render_target_array_index but inputPrimitiveTopology
//       is not specified"). Khai `MTLPrimitiveTopologyClassTriangle` ⇒ OK, và
//       FS đọc đúng giá trị VS ghi.  ⇒ BACKEND phải khai inputPrimitiveTopology
//       (xem "GIỚI HẠN" mục 5).
//   (5) FS đọc `[[render_target_array_index]]` mà VS KHÔNG ghi ⇒ Metal pipeline
//       FAIL ("fragment shader is reading the render_target_array_index but the
//       vertex shader does not write it").  ⇒ FS chỉ đọc gl_Layer khi VS ghi
//       (đúng GL: gl_Layer là output của stage trước).
//   (6) `[[user(locn900)]]` (location tạm) ĐƯỢC chấp nhận: pipeline creation OK,
//       draw OK, giá trị truyền đúng (42) — Metal không giới hạn locn của
//       user interpolant ⇒ location tạm 900+ của pass là AN TOÀN khi linker
//       chưa gán lại (xem "GIỚI HẠN" mục 6).
//   (7) MSL KHÔNG có `[[sample_count]]`: compiler báo "unknown attribute
//       'sample_count' ignored" ⇒ gl_NumSamples không có đối chiếu shader.
//   (8) MSL KHÔNG có `[[draw_index]]` cho vertex function: compiler báo "unknown
//       attribute 'draw_index' ignored" ⇒ gl_DrawID không có đối chiếu shader.
//   (9) Metal chỉ có MỘT clip distance: `[[clip_distance(2)]]` ⇒ "clip_distance
//       attribute takes no arguments"; `float cd[2] [[clip_distance]]` ⇒
//       "'clip_distance' attribute cannot be applied to types".
//
// VÌ SAO DÙNG "TÊN FIELD CHỨA ATTRIBUTE"?
//   Converter (src/gl/glsl_to_msl.cpp) emit field varying theo khuôn
//       "  <mslType> <name> [[user(locnN)]]" + InterpAttr(v)
//   nên không thể thêm `[[render_target_array_index]]` vào field mà không sửa
//   converter (write-scope của agent khác).  Tên field trong MSL là một
//   identifier; đặt attribute VÀO TÊN ⇒ dòng emit thành
//       uint gl_Layer [[render_target_array_index]] [[user(locn900)]] [[flat]];
//   Metal parse đúng: tên field vẫn là `gl_Layer` (attribute tách riêng) ⇒
//   `_out.gl_Layer` / `_in.gl_Layer` vẫn hợp lệ.  ĐÃ ĐO THẬT (mục 1-5 ở trên).
//   Hệ quả: hai bên (VS output / FS input) phải dùng CÙNG chuỗi tên để
//   (a) linker (src/gl/gl_program.cpp) khớp theo tên nếu sau này converter
//   đồng bộ lại R.inputs/R.outputs, và (b) Metal ghép attribute.  Pass này luôn
//   sinh cùng một chuỗi cho cả hai bên.
//
// GIỚI HẠN ĐÃ BIẾT (báo cáo trung thực — KHÔNG giả lập):
//   1) gl_NumSamples: MSL KHÔNG có attribute sample-count (đo mục 7).  Giá trị
//      này là thuộc tính FRAMEBUFFER ("the total number of samples in the
//      framebuffer", GLSL 4.60 p.150) ⇒ phải do CPU (backend) cấp.  Cần:
//      (a) converter khai uniform + map tên, (b) backend ghi giá trị vào
//      uniform buffer mỗi lần draw.  Pass này KHÔNG làm (a) vì thêm uniform
//      trong pass ⇒ struct MSL lớn hơn `R.uniformBufferSize` đã tính
//      (converter tính layout uniform TRƯỚC khi chạy pass) ⇒ buffer Metal nhỏ
//      hơn struct ⇒ Metal reject.  Chi tiết thay đổi cụ thể ở cuối file.
//   2) gl_BaseVertex: MSL có `[[base_vertex]]` nhưng attribute này chỉ hợp lệ
//      với THAM SỐ HÀM vertex (đo: compile OK), KHÔNG hợp lệ trong struct
//      [[stage_in]] ⇒ bắt buộc phải thêm tham số vào hàm vertex ⇒ cần sửa
//      converter (chi tiết ở cuối file).  May mắn: Metal `drawPrimitives:`
//      có `vertexStart` = base vertex ⇒ KHÔNG cần backend đổi gì.
//   3) gl_DrawID: converter đã fail sớm ("gl_DrawID ngoài subset") TRƯỚC khi
//      pass này chạy ⇒ không sửa được ở đây.  MSL cũng không có
//      `[[draw_index]]` cho vertex function (đo mục 8) — MSL 3.x chỉ có
//      draw_index cho mesh shader (Metal không có mesh shader trên iOS).
//      ⇒ giá trị phải do CPU truyền vào (uniform do backend ghi mỗi draw).
//   4) gl_ClipDistance[1]: Metal chỉ hỗ trợ MỘT `[[clip_distance]]` (đo mục
//      9) ⇒ index > 0 không thể map chính xác.  Xấp xếp min(d0,d1) SAI về
//      ngữ nghĩa (nội suy là tuyến tính, min không tuyến tính ⇒ cắt nhiều hơn
//      GL) ⇒ KHÔNG làm.  gl_ClipDistance[0] chạy đúng (converter).
//   5) gl_Layer trong VS: Metal pipeline cần `inputPrimitiveTopology` (đo mục
//      4) ⇒ backend (AppleDrawGL) phải khai
//      `pipeline.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle`
//      (theo topology của draw) cho pipeline dùng gl_Layer.
//   6) Location tạm 900+ của pass KHÔNG được linker gán lại: converter copy
//      `R.inputs`/`R.outputs` vào result TRƯỚC khi chạy pass built-in
//      (src/gl/glsl_to_msl.cpp:10014-10015, 11556) ⇒ linker không thấy các
//      varying của pass.  ĐO THẬT mục 6: Metal vẫn chấp nhận user(locn900) và
//      truyền giá trị đúng ⇒ chưa lỗi, NHƯNG nếu muốn location "sạch" thì cần
//      converter đồng bộ lại R.inputs/R.outputs sau khi chạy pass (1 dòng).
//   7) Built-in dùng trong HÀM PHỤ (không phải main) không được hỗ trợ:
//      converter chỉ rewrite trong thân main; helper là hàm tự do, không có
//      `_in`/`_out` ⇒ cần đổi tên biến qua tham số (như `entryBuiltins` của
//      converter) ⇒ cần sửa converter.
//
// THAY ĐỔI CỤ THỂ CẦN LÀM (ngoài write-scope của pass này) — để đóng nốt
// gl_BaseVertex / gl_NumSamples / gl_DrawID và cho location của pass "sạch":
//
// (A) gl_BaseVertex — src/gl/glsl_to_msl.cpp, nhánh `if (R.isVertex)` (ngay
//     cạnh khối gl_BaseInstance ở ~dòng 10743):
//         bool useBaseVertex = false;                       // cùng khai báo useBaseInstance
//         if (body.find("gl_BaseVertex") != std::string::npos) {
//             useBaseVertex = true;
//             mpVary.emplace_back("gl_BaseVertex", "tglmt_baseVertex");
//         }
//     và trong phần emit vertex function (~dòng 12752, cạnh `useBaseInstance`):
//         if (useBaseVertex) msl_out << ",\n    uint tglmt_baseVertex [[base_vertex]]";
//     ⇒ KHÔNG cần backend đổi: Metal `drawPrimitives:vertexStart:vertexCount:`
//     dùng `vertexStart` làm base vertex ⇒ [[base_vertex]] trùng gl_BaseVertex.
//
// (B) gl_NumSamples — src/gl/glsl_to_msl.cpp, TRƯỚC khối tính layout uniform
//     (~dòng 9913) và nhánh fragment (~dòng 10911):
//         // thêm vào `uniforms`: {name:"gl_NumSamples", glslType:"int",
//         //                       mslType:"int"} ⇒ phải thêm TRƯỚC khi tính
//         //                       uniformOffset/uniformBufferSize (dòng 9913-9938)
//         mpVary.emplace_back("gl_NumSamples", "tglmt_u.gl_NumSamples");
//     + backend (src/gl/gl_apple_draw.cpp): ghi số sample của framebuffer vào
//     slot uniform đó TRƯỚC MỖI draw (giá trị = sampleCount của target).
//
// (C) gl_DrawID — MSL không có attribute (đo mục 8) ⇒ giá trị phải do CPU
//     truyền vào: cùng cơ chế uniform như (B) (backend ghi số thứ tự draw
//     mỗi lần draw) + bỏ lệnh fail sớm ở src/gl/glsl_to_msl.cpp ~dòng 10750.
//
// (D) Location của pass được linker gán lại — src/gl/glsl_to_msl.cpp, ngay
//     SAU `RunGLSLBuiltinPasses(bctx)` (~dòng 11945):
//         R.inputs  = ins;     // varying do pass thêm phải vào R.inputs
//         R.outputs = outs;    //   ⇒ linker (src/gl/gl_program.cpp:1186-1228)
//     Hiện tại R.inputs/R.outputs được copy ở dòng 10014-10015 và 11556 —
//     TRƯỚC pass ⇒ linker không thấy varying của pass ⇒ không gán lại
//     location 900+.  ĐO THẬT mục 6: Metal vẫn chấp nhận user(locn900) và
//     truyền giá trị đúng ⇒ chưa lỗi, nhưng nên sửa cho "sạch".
//     (Lưu ý: `ctx.mp` HIỆN KHÔNG CÓ TÁC DỤNG — `mp` được áp cho body ở
//     dòng 11395 TRƯỚC khi pass chạy ở 11945 ⇒ pass phải tự sửa `body`.)
//
// (E) gl_Layer cần backend khai `inputPrimitiveTopology` cho pipeline (đo mục 4)
//     — src/gl/gl_apple_draw.cpp (nơi tạo MTLRenderPipelineDescriptor):
//         pipeline.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
//     (theo topology của draw: Point/Line/Triangle).
//
// GLSL 4.60 §3.6: mọi identifier bắt đầu bằng "gl_" là RESERVED ⇒ thấy
// "gl_Layer" trong body ⇒ đó CHẮC CHẮN là built-in, không phải biến của user
// (không cần logic "user che built-in" như với built-in hàm).

#include "tglmt/GLSLBuiltins.h"
#include "tglmt/GLSLTextUtil.h"

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {
namespace {

// =====================================================================
// 1. TIỆN ÍCH VĂN BẢN
// =====================================================================

// Có từ khoá `w` đứng riêng trong `s` không?
bool MioHasWord(const std::string& s, const char* w) {
    size_t n = strlen(w);
    size_t p = 0;
    while ((p = s.find(w, p)) != std::string::npos) {
        bool l = p > 0 && IsIdentChar(s[p - 1], false);
        size_t e = p + n;
        bool r = e < s.size() && IsIdentChar(s[e], false);
        if (!l && !r) return true;
        p = e;
    }
    return false;
}

// Thay mọi occurrence đứng riêng của `from` bằng `to`.
std::string MioReplaceWord(const std::string& s, const char* from, const std::string& to) {
    std::string out;
    out.reserve(s.size() + 16);
    size_t n = strlen(from);
    size_t p = 0;
    for (;;) {
        size_t q = s.find(from, p);
        if (q == std::string::npos) {
            out += s.substr(p);
            break;
        }
        bool l = q > 0 && IsIdentChar(s[q - 1], false);
        size_t e = q + n;
        bool r = e < s.size() && IsIdentChar(s[e], false);
        if (!l && !r) {
            out += s.substr(p, q - p);
            out += to;
        } else {
            out += s.substr(p, e - p);
        }
        p = e;
    }
    return out;
}

// Vị trí `w` đứng riêng đầu tiên trong `s` (npos = không có).
size_t MioFindWord(const std::string& s, const char* w) {
    size_t n = strlen(w);
    size_t p = 0;
    while ((p = s.find(w, p)) != std::string::npos) {
        bool l = p > 0 && IsIdentChar(s[p - 1], false);
        size_t e = p + n;
        bool r = e < s.size() && IsIdentChar(s[e], false);
        if (!l && !r) return p;
        p = e;
    }
    return std::string::npos;
}

// =====================================================================
// 2. THÊM VARYING (VS output / FS input)
// =====================================================================

// Attribute MSL đặc biệt — nhúng vào TÊN field (xem header để giải thích).
const char* const kAttrRenderTargetArray = "[[render_target_array_index]]";
const char* const kAttrViewportArray = "[[viewport_array_index]]";
const char* const kAttrSampleMask = "[[sample_mask]]";
const char* const kAttrPrimitiveID = "[[primitive_id]]";

// Tên field MSL = "<glslName> <attr>" (xem header).
std::string MioFieldName(const char* glslName, const char* attr) {
    return std::string(glslName) + " " + attr;
}

// Thêm một VS output (varying). `glslType` là kiểu GLSL (int), `mslType` là
// kiểu MSL (uint — `[[render_target_array_index]]` chỉ nhận
// uchar/ushort/uint theo MSL Table 5.4).
void MioAddVsOutput(GLSLBuiltinCtx& ctx, const char* glslName, const char* mslType,
                    const char* attr) {
    if (!ctx.outputs || !ctx.nextTempLocation) return;
    GLSLVar v;
    v.glslType = "int";
    v.mslType = mslType;
    v.name = MioFieldName(glslName, attr);
    v.location = (*ctx.nextTempLocation)++;
    v.interpolation = "flat";   // biến nguyên ⇒ không nội suy
    ctx.outputs->push_back(v);
}

// Thêm một FS input (varying). Tên phải KHỚP chính xác tên VS output để
// linker (src/gl/gl_program.cpp) khớp theo tên (và Metal ghép theo attribute).
void MioAddFsInput(GLSLBuiltinCtx& ctx, const char* glslName, const char* mslType,
                   const char* attr) {
    if (!ctx.inputs || !ctx.nextTempLocation) return;
    GLSLVar v;
    v.glslType = "int";
    v.mslType = mslType;
    v.name = MioFieldName(glslName, attr);
    v.location = (*ctx.nextTempLocation)++;
    v.interpolation = "flat";
    ctx.inputs->push_back(v);
}

// =====================================================================
// 3. gl_SampleMask / gl_SampleMaskIn
// =====================================================================

// Converter (src/gl/glsl_to_msl.cpp:11368-11392) rewrite `gl_SampleMask[0]` →
// `_out.smask` (field `uint smask [[sample_mask]]` của struct FOut).
// BUG của converter: so khớp theo PREFIX, không kiểm word-boundary ⇒
// `gl_SampleMaskIn[0]` CŨNG bị rewrite thành `_out.smask` — tức đọc giá trị
// của OUTPUT mask thay vì INPUT mask.
//
// Pass này phân biệt bằng vị trí lvalue/rvalue:
//   • `_out.smask = ...`  (ghi)  → gl_SampleMask[0]  (OUTPUT) → giữ nguyên.
//   • `_out.smask` dùng làm rvalue → gl_SampleMaskIn[0] (INPUT) → đổi sang
//     `_in.gl_SampleMaskIn` (field `uint ... [[sample_mask]]` của struct FIn).
// Ghi chú: GLSL cho phép đọc gl_SampleMask[0] (output) — trường hợp đó bị
// hiểu thành gl_SampleMaskIn[0].  Rất hiếm; ghi rõ ở đây.
enum class MioSmaskUse { kWrite, kRead };

// Occurrence `tok` tại `q` có phải LVALUE (được gán) không?
// Nhận diện CẢ hậu tố (`x++`, `x += 1`) và tiền tố (`++x`).
bool MioIsWriteUse(const std::string& s, size_t after, size_t before) {
    // Hậu tố: ký tự ngay sau token.
    size_t i = after;
    while (i < s.size() && isspace((unsigned char)s[i])) ++i;
    if (i < s.size()) {
        char c = s[i];
        if ((c == '+' || c == '-') && i + 1 < s.size() && s[i + 1] == c) return true;
        if (c == '=' && !(i + 1 < s.size() && s[i + 1] == '=')) return true;
        static const char* const kOps[] = {"+=", "-=", "*=", "/=", "%=",
                                           "&=", "|=", "^=", "<<=", ">>="};
        for (const char* op : kOps)
            if (s.compare(i, strlen(op), op) == 0) return true;
    }
    // Tiền tố: `++x` / `--x` ⇒ token đứng sau toán tử.
    size_t j = before;
    while (j > 0 && isspace((unsigned char)s[j - 1])) --j;
    if (j >= 2) {
        char c1 = s[j - 1], c2 = s[j - 2];
        if ((c1 == '+' || c1 == '-') && c1 == c2) return true;
    }
    return false;
}

// Phân loại + viết lại các use của `_out.smask`.
// Trả về true nếu có ÍT NHẤT một use đọc (⇒ cần thêm FS input).
bool MioRewriteSampleMaskIn(GLSLBuiltinCtx& ctx, bool* hadWrite) {
    if (!ctx.body) return false;
    *hadWrite = false;
    static const char kTok[] = "_out.smask";
    const size_t kLen = sizeof(kTok) - 1;
    const std::string& src = *ctx.body;
    std::string out;
    out.reserve(src.size() + 16);
    size_t p = 0;
    bool anyRead = false;
    for (;;) {
        size_t q = src.find(kTok, p);
        if (q == std::string::npos) {
            out += src.substr(p);
            break;
        }
        bool l = q > 0 && IsIdentChar(src[q - 1], false);
        size_t e = q + kLen;
        bool r = e < src.size() && IsIdentChar(src[e], false);
        if (l || r) {
            out += src.substr(p, e - p);
            p = e;
            continue;
        }
        out += src.substr(p, q - p);
        if (MioIsWriteUse(src, e, q)) {
            out += kTok;
            *hadWrite = true;
        } else {
            out += "_in.gl_SampleMaskIn";
            anyRead = true;
        }
        p = e;
    }
    if (!anyRead) return false;
    ctx.body->swap(out);
    return true;
}

// =====================================================================
// 4. gl_FragDepth / gl_SampleMask[0]: bảo đảm FS có color output
// =====================================================================

// Khi FS dùng gl_FragDepth hoặc gl_SampleMask, converter phát fragment
// function trả STRUCT (TGLMT_FOut) vì cần field `depth` / `sample_mask`.
// Nếu FS dùng `gl_FragColor` (legacy) thì `outs` RỖNG ⇒ struct KHÔNG có
// field color ⇒ biến `tglmt_fragColor` KHÔNG được khai báo ⇒ Metal compile
// fail ("use of undeclared identifier 'tglmt_fragColor'").
//
// Fix (không cần sửa converter): khai báo MỘT fragment output color ⇒
// converter emit `float4 c [[color(0)]];` trong struct FOut, khai báo
// `float4 tglmt_fragColor = float4(0.0);` và gán `_out.c = tglmt_fragColor;`
// ở cuối fragment function.
void MioEnsureFragColorOutput(GLSLBuiltinCtx& ctx) {
    if (!ctx.outputs || !ctx.body) return;
    if (!ctx.outputs->empty()) return;                        // đã có color output
    if (MioFindWord(*ctx.body, "tglmt_fragColor") == std::string::npos) return;
    GLSLVar v;
    v.glslType = "vec4";
    v.mslType = "float4";
    v.name = "gl_FragColor";     // tên legacy đúng ngữ nghĩa GL (glGetFragDataLocation)
    v.location = 0;
    ctx.outputs->push_back(v);
}

}  // namespace

// =====================================================================
// 5. PASS
// =====================================================================

static void ApplyGLSLBuiltinModernIO(GLSLBuiltinCtx& ctx) {
    if (!ctx.body) return;
    if (ctx.isGeometry) return;   // GS: converter chưa hỗ trợ stage này
    std::string& body = *ctx.body;

    // ---------------- VERTEX ----------------
    if (ctx.isVertex) {
        // gl_Layer → MSL `[[render_target_array_index]]` (MSL Table 5.4:
        // output của vertex function).  Trong GL, gl_Layer là output của
        // GEOMETRY SHADER (GLSL 4.60 §7.1.4, PDF p.148; GL 4.6 §9.8
        // p.357) — Metal không có GS nên MSL đặt attribute này ở vertex
        // output; đây là đường tương đương (xem "ĐO THẬT" mục 4: pipeline
        // cần `inputPrimitiveTopology` ⇒ backend phải khai).
        if (MioHasWord(body, "gl_Layer")) {
            MioAddVsOutput(ctx, "gl_Layer", "uint", kAttrRenderTargetArray);
            body = MioReplaceWord(body, "gl_Layer", "_out.gl_Layer");
        }
        // gl_ViewportIndex → `[[viewport_array_index]]` (MSL Table 5.4).
        if (MioHasWord(body, "gl_ViewportIndex")) {
            MioAddVsOutput(ctx, "gl_ViewportIndex", "uint", kAttrViewportArray);
            body = MioReplaceWord(body, "gl_ViewportIndex", "_out.gl_ViewportIndex");
        }
        // gl_BaseInstance / gl_BaseVertex / gl_DrawID: xem mục "GIỚI HẠN".
        //   gl_BaseInstance: converter đã map → `[[base_instance]]` (chạy sẵn).
        //   gl_BaseVertex:  cần vertex function PARAMETER `[[base_vertex]]`.
        //   gl_DrawID:      converter fail sớm + MSL không có `[[draw_index]]`.
        return;
    }

    // ---------------- FRAGMENT ----------------
    if (!ctx.isFragment) return;

    // gl_Layer / gl_ViewportIndex là INPUT của fragment (GLSL 4.60 §7.1.5,
    // PDF p.149: "in int gl_Layer; in int gl_ViewportIndex;").
    // Tên field phải giống hệt tên VS output để Metal ghép attribute (đo mục 3)
    // và để linker khớp theo tên nếu converter được đồng bộ lại.
    if (MioHasWord(body, "gl_Layer")) {
        MioAddFsInput(ctx, "gl_Layer", "uint", kAttrRenderTargetArray);
        body = MioReplaceWord(body, "gl_Layer", "_in.gl_Layer");
    }
    if (MioHasWord(body, "gl_ViewportIndex")) {
        MioAddFsInput(ctx, "gl_ViewportIndex", "uint", kAttrViewportArray);
        body = MioReplaceWord(body, "gl_ViewportIndex", "_in.gl_ViewportIndex");
    }

    // gl_PrimitiveID (GLSL 4.60 §7.1.5, PDF p.149 + mô tả p.151) → MSL
    // `[[primitive_id]]` (MSL Table 5.5).  ĐO THẬT: giá trị = chỉ số primitive
    // (0 cho primitive đầu) ⇒ đúng ngữ nghĩa GLSL khi không có GS.
    if (MioHasWord(body, "gl_PrimitiveID")) {
        MioAddFsInput(ctx, "gl_PrimitiveID", "uint", kAttrPrimitiveID);
        body = MioReplaceWord(body, "gl_PrimitiveID", "_in.gl_PrimitiveID");
    }

    // gl_SampleMaskIn[0] (GLSL 4.60 §7.1.5, PDF p.149-150) → MSL
    // `[[sample_mask]]` làm fragment INPUT (MSL Table 5.5).  ĐO THẬT: giá trị =
    // mặt nạ phủ thật (0x1 với target 1 sample) ⇒ đúng ngữ nghĩa GLSL.
    // gl_SampleMask[0] (OUTPUT) đã do converter map → `_out.smask`.
    if (MioHasWord(body, "_out.smask")) {
        bool hadWrite = false;
        bool hadRead = MioRewriteSampleMaskIn(ctx, &hadWrite);
        if (hadRead) {
            MioAddFsInput(ctx, "gl_SampleMaskIn", "uint", kAttrSampleMask);
            if (!hadWrite) {
                // Converter vẫn phát field OUTPUT `uint smask [[sample_mask]]`
                // trong struct FOut (vì body có chứa "gl_SampleMask").  GLSL
                // 4.60 p.150: "If a shader does not statically assign a value to
                // gl_SampleMask, the sample mask has no effect on the processing
                // of a fragment." ⇒ mask mặc định phải là "bật tất cả"
                // (0xFFFFFFFF) để không đổi coverage.  Nếu để 0 ⇒ Metal loại BỎ
                // toàn bộ sample ⇒ mất hình.
                body = " _out.smask = 0xFFFFFFFFu;" + body;
            }
        }
    }

    // gl_FragDepth (GLSL 4.60 §7.1.5, PDF p.149) → converter đã map → `_out.depth`
    // (`float depth [[depth(any)]]` của struct FOut).  Việc còn lại: bảo đảm
    // fragment function có color output (xem MioEnsureFragColorOutput).
    bool usesDepth = MioFindWord(body, "_out.depth") != std::string::npos;
    bool usesMaskOut = MioFindWord(body, "_out.smask") != std::string::npos;
    if (usesDepth || usesMaskOut) MioEnsureFragColorOutput(ctx);
}

static const bool kRegModernIO = RegisterGLSLBuiltinPass("modern_io", ApplyGLSLBuiltinModernIO);

}  // namespace tglmt::gl
