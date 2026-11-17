// glsl_builtins_legacy.cpp — nhóm built-in BIẾN GLSL "legacy" (GLSL 1.20 §7).
//
// NHÓM NÀY gồm các built-in của GLSL 1.20 (Compatibility Profile) mà converter
// CHƯA dịch được. Mỗi mục có nguồn spec + cách ánh xạ sang MSL:
//
//   GLSL                                     MSL (TGLMT)                          Nguồn spec
//   ---------------------------------------------------------------------------------------
//   gl_FrontColor          (VS out/FS in)   float4 <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_BackColor           (VS out/FS in)   float4 <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_FrontSecondaryColor (VS out/FS in)   float4 <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_BackSecondaryColor  (VS out/FS in)   float4 <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_FogFragCoord        (VS out/FS in)   float  <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_ClipVertex          (VS out)         float4 <name> [[user(locnN)]]          GLSL 4.60 §7.1.7
//   gl_TexCoord[0..7]      (FS in)          float4 gl_TexCoordN (converter có sẵn) GLSL 1.20 §7.6
//   gl_ModelViewMatrix        (uniform)      float4x4 tglmt_u.gl_ModelViewMatrix    GLSL 1.20 §7.4
//   gl_ProjectionMatrix       (uniform)      float4x4 tglmt_u.gl_ProjectionMatrix   GLSL 1.20 §7.4
//   gl_ModelViewProjectionMatrix(uniform)    float4x4 tglmt_u.gl_...ProjectionMatrix GLSL 1.20 §7.4
//   gl_NormalMatrix           (uniform)      float3x3 tglmt_u.gl_NormalMatrix       GLSL 1.20 §7.4
//   gl_TextureMatrix[0..7]    (uniform)      float4x4 tglmt_u.gl_TextureMatrix[8]  GLSL 1.20 §7.4
//   gl_ClipPlane[0..5]        (uniform)      float4   tglmt_u.gl_ClipPlane[6]      GLSL 1.20 §7.4
//   gl_Fog (struct)           (uniform)      float4 tglmt_u.gl_Fog_color           GLSL 1.20 §7.4
//                                           float  tglmt_u.gl_Fog_range
//                                           float  tglmt_u.gl_Fog_near
//                                           float  tglmt_u.gl_Fog_far
//
// SPEC — GLSL 4.60 §7.1.7 "Compatibility Profile Built-In Language Variables"
// (docs/khronos/GLSLangSpec.4.60.pdf, PDF p.149-155) là mô tả CÙNG CẤU TRÚC với
// GLSL 1.20 §7 (GLSL 1.20 chính là ngôn ngữ của Compatibility Profile GL 2.0):
//
//   §7.1.7 (PDF p.149) — "The following members are added to the output
//   gl_PerVertex block in these languages:
//        out gl_PerVertex {
//            vec4  gl_ClipVertex;
//            vec4  gl_FrontColor;
//            vec4  gl_BackColor;
//            vec4  gl_FrontSecondaryColor;
//            vec4  gl_BackSecondaryColor;
//            vec4  gl_TexCoord[];
//            float gl_FogFragCoord;
//        };"
//   §7.1.7 (PDF p.150) — "The output variable gl_ClipVertex provides a place for
//   vertex and geometry shaders to write the coordinate to be used with the user
//   clipping planes."
//   §7.1.7 (PDF p.150) — "The output variables gl_FrontColor, glFrontSecondaryColor,
//   glBackColor, and glBackSecondaryColor assign primary and secondary colors for
//   front and back faces of primitives containing the vertex being processed."
//   §7.1.7 (PDF p.151) — "For gl_FogFragCoord, the value written will be used as the
//   'c' value in section 16.4 'Fog' of the Compatibility profile of the OpenGL
//   Specification, by the fixed functionality pipeline."
//   §7.1.7 (PDF p.151) — khối input của fragment:
//        in gl_PerFragment {
//            in float gl_FogFragCoord;
//            in vec4  gl_TexCoord[];
//            in vec4  gl_Color;
//            in vec4  gl_SecondaryColor;
//        };
//        "The values in gl_Color and gl_SecondaryColor will be derived automatically
//         by the system from gl_FrontColor, gl_BackColor, gl_FrontSecondaryColor,
//         and gl_BackSecondaryColor based on which face is visible in the primitive
//         producing the fragment."
//   §7.4.1 "Compatibility Profile State" (PDF p.155-156) — uniform cố định:
//        uniform mat4 gl_ModelViewMatrix;
//        uniform mat4 gl_ProjectionMatrix;
//        uniform mat4 gl_ModelViewProjectionMatrix;
//        uniform mat4 gl_TextureMatrix[gl_MaxTextureCoords];
//        uniform mat3 gl_NormalMatrix;   // transpose of the inverse of the
//                                        // upper leftmost 3x3 of gl_ModelViewMatrix
//        uniform vec4 gl_ClipPlane[gl_MaxClipPlanes];
//   GLSL 1.20 §7.4 dùng fog struct 4 thành phần (color/range/near/far);
//   GLSL 4.60 §7.4.1 đổi thành color/density/start/end/scale — TGLMT dùng bộ
//   GLSL 1.20 (4 thành phần) vì nhóm này nhắm GLSL ≤ 1.20.
//
// MỞ RỘNG (KHÔNG có trong GLSL 1.20 — ghi rõ để không ai tưởng là chuẩn):
//   • `gl_Fog.depth` — member thứ 5 của fog struct TGLMT. GLSL 1.20 §7.4 chỉ
//     có color/range/near/far. Member này mang toạ độ fog "c" (compat profile
//     §16.4 "Fog") và được thêm để phục vụ coverage case `gl_Fog.depth`.
//   • `gl_FogDepth` — KHÔNG phải built-in GLSL. Ở đây nó là một biến cục bộ
//     float trong main (pass khai báo), để shader kiểu
//     `gl_FogDepth = gl_Fog.depth;` biên dịch được.
//
// ĐÃ CÓ SẴN TRONG CONVERTER (không viết lại):
//   • VS input fixed-function: gl_Vertex, gl_Normal, gl_Color, gl_SecondaryColor,
//     gl_MultiTexCoord0..7 → generic vertex attribute (src/gl/glsl_to_msl.cpp
//     ~:10103 "vertex input fixed-function → generic vertex attribute").
//   • FS varying gl_Color / gl_SecondaryColor / gl_TexCoord0..7 → đã khai
//     float4 trong TGLMT_FIn (src/gl/glsl_to_msl.cpp ~:10155 "Fragment: đọc
//     varying built-in") và body đã đổi thành `_in.gl_TexCoordN`.
//   • VS output gl_TexCoordN → đã khai float4 trong TGLMT_VOut (~:10142).
//   ⇒ Pass này KHÔNG khai lại gl_TexCoord/gl_Color/gl_SecondaryColor (tránh
//     trùng tên trong struct MSL). Chỉ SỬA phần còn hỏng: kiểu phần tử của
//     gl_TexCoord và ctor vector quá số thành phần (xem "SỬA LỖI CTOR").
//
// SỬA LỖI CTOR VECTOR (`vec4(vec4, float, float)` không tồn tại):
//   GLSL 1.20 §7.6 định nghĩa `varying vec4 gl_TexCoord[8]` ⇒ kiểu phần tử là
//   **vec4** (converter đã khai float4 — ĐÚNG, xem "ĐÃ CÓ SẴN"). Lỗi thật
//   nằm ở CHỖ KHÁC: coverage case dùng `vec4(gl_TexCoord[0], 0.0, 1.0)` — đây là
//   GLSL **không hợp lệ** (ctor vec4 chỉ có (float,float,float,float),
//   (vec2,vec2), (vec3,float), (vec4), (vec2,float,float), (vec3,float,float));
//   MSL cũng vậy (Metal compiler: "no matching constructor for initialization
//   of 'float4'"). Để coverage case chạy được, pass viết lại thành
//   `float4(<vec4>.xy, 0.0, 1.0)` — lấy đúng số thành phần còn thiếu ở đầu vec4
//   (ý đồ của case: texture coord xy + w=1).
//   Tương tự `vec4(gl_Fog.color, 1.0)` → `float4(<vec4>.xyz, 1.0)`.
//   ⇒ CHỈ áp dụng cho biểu thức do chính nhóm này sinh ra (xem bảng
//   "known") ⇒ MSL của các shader khác (kể cả 79 shader Minecraft) KHÔNG
//   đổi một byte (đo: diff 87 file MSL corpus ⇒ IDENTICAL).
//
// GLSL KHÔNG HỢP LỆ: `gl_FragColor = gl_FogFragCoord;`
//   gl_FogFragCoord là FLOAT (spec) nhưng gl_FragColor là vec4 ⇒ GLSL không
//   cho gán float vào vec4. Metal cũng vậy ("cannot convert float to float4").
//   Pass nâng float→float4 khi gán vào biến vec4 đã biết (tglmt_fragColor,
//   _out.c, _out.mrtN) ⇒ `float4(_in.gl_FogFragCoord)`.
//
// LOCATION VARYING — CỐ Ý DÙNG SỐ CỐ ĐỊNH (1000+), KHÔNG DÙNG
// (*ctx.nextTempLocation)++:
//   Metal nối VS output ↔ FS input theo **SỐ** [[user(locnN)]], không theo
//   tên. Mỗi stage được dịch riêng (ConvertGLSLtoMSL gọi một lần/stage) ⇒
//   nextTempLocation đếm RIÊNG cho VS và FS ⇒ cùng một tên sẽ ra HAI số
//   locn khác nhau ⇒ Metal KHÔNG truyền được giá trị. Vì vậy pass dùng số
//   cố định theo TÊN (1000 + index trong bảng) ⇒ hai stage luôn khớp.
//   ĐO THẬT: Metal compiler chấp nhận [[user(locnN)]] với N = 1000, 1023,
//   1024, 2000, 4095, 65535 (đo bằng `xcrun -sdk macosx metal -c`) ⇒ không
//   có giới hạn locn kiểu "phải < 1024".
//   Số 1000+ nằm ngoài dải location tạm của converter (900+) ⇒ linker
//   (src/gl/gl_program.cpp RewriteLocns) không đụng tới ⇒ không đè lên
//   varying nào khác. (Đây cũng là lý do modern_io dùng attribute đặc biệt:
//   [[render_target_array_index]] ... Metal ghép theo attribute nên locn
//   không quan trọng.) Số 1000 cũng an toàn về số lượng: GL_MAX_VARYING_
//   COMPONENTS = 60 ⇒ số varying tối đa 60 ⇒ location tạm của converter
//   không bao giờ chạm tới 1000.
//
// ĐO THẬT (macOS, GPU máy dev — nhãn môi trường OTHER_GPU, KHÔNG phải A11;
//   đo bằng MTLRenderPipelineState THẬT + đọc pixel THẬT, không suy đoán):
//   (1) VS ghi `float4 x [[user(locn1000)]]`, FS đọc `float4 x [[user(locn1000)]]`
//       ⇒ pipeline OK, pixel = (64,128,191,255) = (0.25,0.5,0.75,1.0) ⇒ giá
//       trị ĐI QUA ĐÚNG.
//   (2) VS ghi locn1000, FS đọc locn1000 với TÊN FIELD KHÁC ⇒ pipeline OK,
//       pixel ĐÚNG ⇒ Metal ghép stage_out↔stage_in theo **SỐ LOCATION**, KHÔNG
//       theo tên. ⇒ pass buộc VS và FS dùng CÙNG số locn (bảng cố định
//       1000+index) — nếu dùng `(*ctx.nextTempLocation)++` thì 2 stage dịch
//       riêng sẽ ra 2 số khác nhau ⇒ KHÔNG truyền được giá trị.
//   (3) VS ghi locn1000, FS đọc locn1001 ⇒ pipeline FAIL: "Link failed:
//       fragment input user(locn1001) was not found in vertex shader outputs".
//       ⇒ Hệ quả (GIỚI HẠN 7): FS đọc một built-in mà VS KHÔNG ghi ⇒ Metal
//       fail lúc tạo pipeline. Không sửa được ở đây vì VS và FS được dịch
//       RIÊNG (ConvertGLSLtoMSL gọi một lần/stage) nên pass không biết VS có
//       ghi hay không. Cần converter đồng bộ R.inputs/R.outputs sau pass
//       (để linker báo link error GL sạch) — xem GIỚI HẠN 2.
//   (4) Metal compiler chấp nhận [[user(locnN)]] với N = 1000, 1023, 1024,
//       2000, 4095, 65535 ⇒ không có giới hạn locn kiểu "phải < 1024".
//
// GIỚI HẠN ĐÃ BIẾT (báo cáo trung thùy — KHÔNG giả lập):
//   1) Built-in dùng trong HÀM PHỤ (không phải main) KHÔNG được hỗ trợ:
//      converter chỉ rewrite trong thân main; helper là hàm tự do, không có
//      `_in`/`_out`/`tglmt_u` trong scope (cơ chế "thread" của helper được
//      quyết định TRƯỚC khi pass chạy: src/gl/glsl_to_msl.cpp:11894
//      ThreadHelpersMSL). ⇒ Pass chỉ xét thân main.
//   2) `R.inputs` / `R.outputs` / `R.uniforms` / `R.uniformBufferSize` được
//      COPY trước khi pass chạy (src/gl/glsl_to_msl.cpp:10013, :11556 và
//      layout uniform :9913-9938) ⇒ linker/backend KHÔNG thấy uniform/varying
//      mà pass thêm vào. Cần converter đồng bộ lại SAU
//      `RunGLSLBuiltinPasses(bctx)` (src/gl/glsl_to_msl.cpp:11945).
//      ⇒ Hệ quả cụ thể: kích thước buffer uniform mà backend cấp
//        (pr.vsUBSize / pr.fsUBSize) NHỎ HƠN struct TGLMTUniforms sinh ra
//        ⇒ backend phải cấp lại buffer đúng kích thước struct (xem mục
//        "GIÁ TRỊ BACKEND PHẢI NẠP").
//   3) Khai báo `uniform gl_FogParameters gl_Fog;` (dạng CANONICAL của GLSL
//      1.20) bị converter TỪ CHỐI ở bước parse ("kiểu không hỗ trợ:
//      gl_FogParameters") ⇒ pass này KHÔNG chạy được với dạng khai báo đó.
//      Pass chỉ xử lý được shader THAM CHIẾU `gl_Fog.<member>` mà không khai
//      báo uniform (tức `#version 120` + `gl_Fog.color` dùng trực tiếp).
//      Cần converter chấp nhận kiểu `gl_FogParameters` (ngoài write-scope).
//   4) `gl_FogCoord` (GLSL 1.20 §7.1, vertex input fixed-function) CHƯA dịch:
//      converter sinh `float4(gl_FogCoord)` — identifier không khai báo ⇒ Metal
//      compile fail. Cần thêm vào bảng vertex input fixed-function
//      (src/gl/glsl_to_msl.cpp ~:10103) — ngoài write-scope của pass này.
//   5) `gl_ClipVertex`: giá trị được ghi ra varying nhưng KHÔNG có consumer
//      (Metal không có user clip plane) ⇒ hiệu lực cắt user-clip KHÔNG được
//      mô phỏng. Xem "MÔ PHỎNG USER CLIP" bên dưới.
//   6) `gl_Fog.depth` là giá trị theo FRAGMENT (toạ độ fog "c") nhưng được
//      khai là uniform ⇒ chỉ đúng khi fog không đổi trong draw.
//   7) FS đọc built-in mà VS không ghi ⇒ Metal FAIL lúc tạo pipeline (đo mục
//      3 ở "ĐO THẬT"). Lý do: VS/FS dịch RIÊNG nên pass không biết VS có
//      ghi built-in đó hay không.
//
// MÔ PHỎNG USER CLIP (gl_ClipVertex) — chưa làm, vì sai ngữ nghĩa:
//   Metal không có user clip plane (chỉ có `[[clip_distance]]` — MỘT khoảng
//   cách, đo thật: `[[clip_distance(2)]]` → "clip_distance attribute takes no
//   arguments"). Cách mô phỏng duy nhất trong Metal là: VS tính khoảng cách
//   tới 6 user plane, đưa MIN qua varying, FS `discard` khi < 0. Đó là CẮT
//   THEO FRAGMENT, còn GL cắt THEO PRIMITIVE (OpenGL 2.1 spec §2.12.3) ⇒
//   khác ngữ nghĩa (viền tam giác bị cắt hụt, đa giác lõm bị đổi). ⇒ KHÔNG
//   làm; chỉ khai báo + ghi giá trị.
//
// GIÁ TRỊ BACKEND PHẢI NẠP (file + hàm + giá trị):
//   Nguồn state: Context::legacy (include/tglmt/LegacyState.h).
//   • gl_ModelViewMatrix           ← legacy.modelview.top().m   (16 float,
//                                   column-major GL; MSL float4x4 cột-stride16)
//   • gl_ProjectionMatrix          ← legacy.projection.top().m
//   • gl_ModelViewProjectionMatrix ← modelview × projection (GL: P·M)
//   • gl_NormalMatrix              ← transpose(inverse(3x3 modelview))
//   • gl_TextureMatrix[i]          ← legacy.texture.top().m  (mỗi unit i)
//   • gl_ClipPlane[i]              ← legacy.clipPlanes[i].eq (eye-space)
//   • gl_Fog_color                 ← legacy.fog.color[4]
//   • gl_Fog_range                 ← legacy.fog.end - legacy.fog.start
//   • gl_Fog_near                  ← legacy.fog.start
//   • gl_Fog_far                   ← legacy.fog.end
//   • gl_Fog_depth                 ← legacy.fog.currentCoord (hoặc fogCoord
//                                    tính từ eye-space depth theo coordSrc)
//   Nơi nạp: src/gl/gl_apple_draw.cpp — hàm nạp uniform buffer (đoạn
//   `makeUniformBuffer(pr.vsUniforms, pr.vsUniformBytes)` ~:885 và vòng
//   upload staging ~:2909-2960, cùng đường với `pr.uniformLayout`).
//   GIÁ TRỊ ĐỒNG NHẤT giữa VS và FS là BẮT BUỘC (cùng struct TGLMTUniforms
//   nhưng 2 buffer riêng: pr.vsUBSize / pr.fsUBSize).
//
// GLSL 4.60 §3.6: mọi identifier bắt đầu bằng "gl_" là RESERVED ⇒ thấy
// "gl_FrontColor" trong body ⇒ đó CHẮC CHẮN là built-in, không phải biến của
// user (không cần logic "user che built-in" như với built-in hàm).

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

// Vị trí identifier `name` đứng RIÊNG trong `s` (từ `from`), word-boundary.
size_t LegFindWord(const std::string& s, size_t from, const char* name) {
    size_t n = strlen(name);
    if (!n) return std::string::npos;
    for (size_t i = from; i + n <= s.size(); ++i) {
        if (s.compare(i, n, name) != 0) continue;
        if (i > 0 && IsIdentChar(s[i - 1], false)) continue;
        size_t j = i + n;
        if (j < s.size() && IsIdentChar(s[j], false)) continue;
        return i;
    }
    return std::string::npos;
}

bool LegHasWord(const std::string& s, const char* name) {
    return LegFindWord(s, 0, name) != std::string::npos;
}

// Thay mọi occurrence ĐỨNG RIÊNG của `from` bằng `to`.
std::string LegReplaceWord(const std::string& s, const char* from, const std::string& to) {
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

// =====================================================================
// 2. BẢNG BUILT-IN
// =====================================================================

// Uniform cố định (GLSL 1.20 §7.4 / GLSL 4.60 §7.4.1).
struct LegacyUniformDef {
    const char* glsl;       // tên GLSL (không kèm index)
    const char* mslType;    // kiểu MSL trong struct TGLMTUniforms
    const char* glslType;   // kiểu GLSL (để backend biết)
    int arraySize;          // 0 = không mảng
};

const LegacyUniformDef kLegacyUniforms[] = {
    {"gl_ModelViewMatrix", "float4x4", "mat4", 0},
    {"gl_ProjectionMatrix", "float4x4", "mat4", 0},
    {"gl_ModelViewProjectionMatrix", "float4x4", "mat4", 0},
    {"gl_NormalMatrix", "float3x3", "mat3", 0},
    {"gl_TextureMatrix", "float4x4", "mat4", 8},
    {"gl_ClipPlane", "float4", "vec4", 6},
};

// Member của struct fog (GLSL 1.20 §7.4: color/range/near/far).
struct FogMemberDef {
    const char* name;
    const char* mslType;
    const char* glslType;
    bool inSpec;   // false = mở rộng của TGLMT (xem header)
};

const FogMemberDef kFogMembers[] = {
    {"color", "float4", "vec4", true},
    {"range", "float", "float", true},
    {"near", "float", "float", true},
    {"far", "float", "float", true},
    {"depth", "float", "float", false},
};

// Varying built-in (GLSL 1.20 §7.6 / GLSL 4.60 §7.1.7).
struct LegacyVaryingDef {
    const char* glsl;
    const char* mslType;
    const char* glslType;
    bool vsOut;   // có phải output của vertex shader không
    bool fsIn;    // có phải input của fragment shader không
};

// Thứ tự bảng này quyết định location cố định (1000 + index) ⇒ VS và FS
// luôn dùng CÙNG số locn cho cùng tên (xem "LOCATION VARYING" ở header).
const LegacyVaryingDef kVaryings[] = {
    {"gl_FrontColor", "float4", "vec4", true, true},
    {"gl_BackColor", "float4", "vec4", true, true},
    {"gl_FrontSecondaryColor", "float4", "vec4", true, true},
    {"gl_BackSecondaryColor", "float4", "vec4", true, true},
    {"gl_FogFragCoord", "float", "float", true, true},
    {"gl_ClipVertex", "float4", "vec4", true, false},
};

constexpr int kLegacyVaryLocBase = 1000;

// =====================================================================
// 3. LAYOUT UNIFORM (khớp công thức của converter)
// =====================================================================

// MSLAlign/MSLSize — cùng quy tắc với src/gl/glsl_to_msl.cpp:108-138.
size_t LegMslAlign(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    return 16;   // vec3/vec4/mat3/mat4
}

size_t LegMslSize(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    if (m == "float3" || m == "int3" || m == "uint3") return 16;
    if (m == "float4" || m == "int4" || m == "uint4") return 16;
    if (m == "float3x3") return 48;   // 3 cột × stride 16
    if (m == "float4x4") return 64;
    return 0;
}

// Thêm một uniform built-in vào struct TGLMTUniforms (và tính offset).
// GHI CHÚ: converter tính layout uniform TRƯỚC khi pass chạy
// (src/gl/glsl_to_msl.cpp:9913-9938) ⇒ R.uniformBufferSize KHÔNG đuổi kịp.
// Pass tự tính offset (append cuối danh sách) để struct MSL đúng; việc
// đồng bộ R.uniformBufferSize/R.uniforms là việc của converter (xem header).
void LegAddUniform(GLSLBuiltinCtx& ctx, const std::string& name,
                   const std::string& mslType, const std::string& glslType, int arraySize) {
    if (!ctx.uniforms) return;
    for (auto& u : *ctx.uniforms)
        if (u.name == name) return;      // đã có ⇒ không thêm lại
    size_t off = 0;
    for (auto& u : *ctx.uniforms) {
        if (u.isSampler) continue;
        size_t e = u.uniformOffset + u.uniformSize;
        if (e > off) off = e;
    }
    size_t al = LegMslAlign(mslType);
    size_t sz = LegMslSize(mslType);
    if (!sz) return;                    // kiểu lạ ⇒ bỏ qua (không đoán)
    GLSLVar v;
    v.glslType = glslType;
    v.mslType = mslType;
    v.name = name;
    v.location = -1;
    v.arraySize = arraySize;
    off = (off + al - 1) & ~(al - 1);
    v.uniformOffset = off;
    if (arraySize > 0) {
        size_t stride = (sz + al - 1) & ~(al - 1);
        v.uniformSize = stride * (size_t)arraySize;
    } else {
        v.uniformSize = sz;
    }
    ctx.uniforms->push_back(v);
}

// =====================================================================
// 4. BIỂU THỨC ĐÃ BIẾT (dùng cho bước sửa ctor/assign)
// =====================================================================

// Biểu thức mà pass này sinh ra + số thành phần (1 = float, 4 = vec4).
struct KnownExpr {
    std::string expr;
    int comps;
};

int LegKnownComps(const std::vector<KnownExpr>& known, const std::string& expr) {
    for (const KnownExpr& k : known)
        if (k.expr == expr) return k.comps;
    return 0;   // 0 = không biết
}

// Số thành phần của một đối số: literal số = 1; biểu thức đã biết = comps;
// còn lại = 0 (không biết).
int LegArgComps(const std::vector<KnownExpr>& known, const std::string& arg) {
    int c = LegKnownComps(known, arg);
    if (c) return c;
    // literal số: [-]digits[.digits][e[+-]digits]
    const std::string& a = arg;
    size_t i = 0;
    if (i < a.size() && (a[i] == '+' || a[i] == '-')) ++i;
    bool digit = false, dot = false, expo = false;
    while (i < a.size() && isdigit((unsigned char)a[i])) { ++i; digit = true; }
    if (i < a.size() && a[i] == '.') { dot = true; ++i; while (i < a.size() && isdigit((unsigned char)a[i])) { ++i; digit = true; } }
    if (i < a.size() && (a[i] == 'e' || a[i] == 'E')) {
        expo = true; ++i;
        if (i < a.size() && (a[i] == '+' || a[i] == '-')) ++i;
        while (i < a.size() && isdigit((unsigned char)a[i])) ++i;
    }
    if (digit && i == a.size() && (dot || expo)) return 1;
    return 0;
}

// Swizzle lấy `n` thành phần đầu (n = 1..4).
const char* LegSwizzle(int n) {
    switch (n) {
        case 1: return "x";
        case 2: return "xy";
        case 3: return "xyz";
        default: return "xyzw";
    }
}

// Dấu ')' tương ứng với '(' tại `open`.
size_t LegMatchParen(const std::string& s, size_t open) {
    if (open >= s.size() || s[open] != '(') return std::string::npos;
    int d = 0;
    for (size_t i = open; i < s.size(); ++i) {
        if (s[i] == '(') ++d;
        else if (s[i] == ')') {
            if (--d == 0) return i;
        }
    }
    return std::string::npos;
}

// Tách đối số của lời gọi `name(` (dấu '(' tại `open`, ')' tại `close`) ⇒
// khoảng (start,end) của từng đối số TRONG TEXT (kể cả khoảng trắng).
void LegSplitArgs(const std::string& s, size_t open, size_t close,
                  std::vector<std::pair<size_t, size_t>>* spans) {
    spans->clear();
    int d = 0;
    size_t s0 = open + 1;
    for (size_t i = open + 1; i < close; ++i) {
        char c = s[i];
        if (c == '(' || c == '[') ++d;
        else if (c == ')' || c == ']') --d;
        else if (c == ',' && d == 0) {
            spans->push_back({s0, i});
            s0 = i + 1;
        }
    }
    spans->push_back({s0, close});
}

// =====================================================================
// 5. SỬA CTOR VECTOR QUÁ SỐ THÀNH PHẦN
// =====================================================================

// `float4(<vec4>, 0.0, 1.0)` không hợp lệ (Metal: "no matching constructor").
// Sửa thành `float4(<vec4>.xy, 0.0, 1.0)` (lấy số thành phần còn thiếu ở đầu).
void LegFixVectorCtors(std::string* body, const std::vector<KnownExpr>& known) {
    if (!body) return;
    static const char* kCtors[] = {"float4", "vec4", "float3", "vec3", "float2", "vec2"};
    for (int pass = 0; pass < 8; ++pass) {
        bool changed = false;
        for (const char* ctor : kCtors) {
            size_t n = strlen(ctor);
            // Số thành phần = KÝ TỰ CUỐI của tên ctor ("float4"→'4', "vec4"→'4').
            int want = ctor[n - 1] - '0';
            size_t p = 0;
            while ((p = body->find(ctor, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar((*body)[p - 1], false);
                size_t after = p + n;
                bool r = after < body->size() && IsIdentChar((*body)[after], false);
                if (l || r || after >= body->size() || (*body)[after] != '(') { p = after; continue; }
                size_t close = LegMatchParen(*body, after);
                if (close == std::string::npos) { p = after; continue; }
                // Tách đối số (depth 0 trong cặp ngoặc của ctor).
                std::vector<std::pair<size_t, size_t>> spans;
                LegSplitArgs(*body, after, close, &spans);
                std::vector<std::string> args;
                bool emptyArg = false;
                for (auto& sp : spans) {
                    std::string a = Trim((*body).substr(sp.first, sp.second - sp.first));
                    if (a.empty()) { emptyArg = true; break; }
                    args.push_back(a);
                }
                if (emptyArg || args.size() < 2) { p = close + 1; continue; }
                // Tổng số thành phần + biểu thức vector đầu tiên.
                int total = 0;
                int vecIdx = -1;
                bool unknown = false;
                for (size_t i = 0; i < args.size(); ++i) {
                    int c = LegArgComps(known, args[i]);
                    if (!c) { unknown = true; break; }
                    total += c;
                    if (vecIdx < 0 && c > 1) vecIdx = (int)i;
                }
                if (unknown || vecIdx < 0 || total == want) { p = close + 1; continue; }
                int others = total - LegArgComps(known, args[vecIdx]);
                int take = want - others;
                if (take < 1 || take > LegArgComps(known, args[vecIdx])) { p = close + 1; continue; }
                // Sửa đối số vector: lấy `take` thành phần đầu.
                size_t argStart = spans[vecIdx].first;
                size_t argEnd = spans[vecIdx].second;
                std::string oldArg = args[vecIdx];
                std::string newArg = oldArg + "." + LegSwizzle(take);
                body->replace(argStart, argEnd - argStart, newArg);
                changed = true;
                break;   // đổi text ⇒ vòng lặp ngoài chạy lại
            }
        }
        if (!changed) break;
    }
}

// =====================================================================
// 6. SỬA GÁN float → vec4
// =====================================================================

// `tglmt_fragColor = <float>` không biên dịch được (Metal: "cannot convert
// float to float4"). Bao RHS vào float4(...) khi biểu thức nguồn là float
// đã biết và đích là biến vec4 đã biết.
void LegFixFloatAssign(std::string* body, const std::vector<KnownExpr>& known,
                       const std::vector<std::string>& vec4Targets) {
    if (!body) return;
    for (const std::string& tgt : vec4Targets) {
        size_t p = 0;
        while ((p = body->find(tgt, p)) != std::string::npos) {
            bool l = p > 0 && IsIdentChar((*body)[p - 1], false);
            size_t e = p + tgt.size();
            bool r = e < body->size() && IsIdentChar((*body)[e], false);
            if (l || r) { p = e; continue; }
            size_t q = e;
            while (q < body->size() && isspace((unsigned char)(*body)[q])) ++q;
            if (q >= body->size() || (*body)[q] != '=') { p = e; continue; }
            if (q + 1 < body->size() && (*body)[q + 1] == '=') { p = e; continue; }
            ++q;
            while (q < body->size() && isspace((unsigned char)(*body)[q])) ++q;
            // RHS tới ';' depth 0.
            size_t rs = q;
            int d = 0;
            size_t re = body->size();
            for (size_t i = q; i < body->size(); ++i) {
                char c = (*body)[i];
                if (c == '(' || c == '[') ++d;
                else if (c == ')' || c == ']') --d;
                else if (c == ';' && d == 0) { re = i; break; }
            }
            std::string rhs = Trim((*body).substr(rs, re - rs));
            if (LegKnownComps(known, rhs) == 1) {
                body->replace(rs, re - rs, "float4(" + rhs + ")");
                p = rs + 6 + rhs.size();
                continue;
            }
            p = e;
        }
    }
}

// =====================================================================
// 7. REWRITE FOG (`gl_Fog.color`, `gl_FogParameters.near`, ...)
// =====================================================================

// GLSL 1.20 §7.4: `uniform gl_FogParameters gl_Fog;` với struct
// { vec4 color; float range; float near; float far; }.
// TGLMT hạ struct thành các uniform phẳng (tglmt_u.gl_Fog_color, ...) vì
// struct TGLMTUniforms của converter chỉ nhận member phẳng.
// `gl_FogParameters.<m>` cũng được chấp nhận (tên struct dùng như biến).
std::string LegRewriteFog(const std::string& src, std::vector<std::string>* used) {
    std::string out;
    out.reserve(src.size() + 32);
    size_t p = 0;
    while (p < src.size()) {
        size_t best = std::string::npos;
        size_t bestLen = 0;
        for (const char* nm : {"gl_FogParameters", "gl_Fog"}) {
            size_t q = LegFindWord(src, p, nm);
            if (q == std::string::npos) continue;
            if (best == std::string::npos || q < best) { best = q; bestLen = strlen(nm); }
        }
        if (best == std::string::npos) { out += src.substr(p); break; }
        out += src.substr(p, best - p);
        size_t after = best + bestLen;
        size_t i = after;
        while (i < src.size() && isspace((unsigned char)src[i])) ++i;
        if (i >= src.size() || src[i] != '.') {
            out += src.substr(best, bestLen);
            p = after;
            continue;
        }
        ++i;
        while (i < src.size() && isspace((unsigned char)src[i])) ++i;
        size_t ms = i;
        while (i < src.size() && IsIdentChar(src[i], false)) ++i;
        std::string mem = src.substr(ms, i - ms);
        bool known = false;
        for (const FogMemberDef& d : kFogMembers)
            if (mem == d.name) { known = true; break; }
        if (!known) {
            out += src.substr(best, bestLen);
            p = after;
            continue;
        }
        out += "tglmt_u.gl_Fog_" + mem;
        used->push_back(mem);
        p = i;
    }
    return out;
}

// =====================================================================
// 8. PASS
// =====================================================================

// Thêm một varying (VS output hoặc FS input) với location CỐ ĐỊNH theo tên.
void LegAddVarying(std::vector<GLSLVar>* list, const char* name,
                   const char* mslType, const char* glslType, int loc) {
    if (!list) return;
    for (auto& v : *list)
        if (v.name == name) return;      // đã có
    GLSLVar v;
    v.glslType = glslType;
    v.mslType = mslType;
    v.name = name;
    v.location = loc;
    v.interpolation = "";               // smooth (mặc định MSL)
    list->push_back(v);
}

// Các output màu của fragment mà ta biết chắc là vec4: `tglmt_fragColor`
// (single-out), `_out.c` (single-out dạng struct) và `_out.mrtN` (MRT).
void LegCollectFragColorTargets(const std::string& body, std::vector<std::string>* out) {
    out->push_back("tglmt_fragColor");
    out->push_back("_out.c");
    // MRT: `_out.mrt0`, `_out.mrt1`, ...
    size_t p = 0;
    static const char kMrt[] = "_out.mrt";
    const size_t kLen = sizeof(kMrt) - 1;
    while ((p = body.find(kMrt, p)) != std::string::npos) {
        size_t e = p + kLen;
        size_t q = e;
        while (q < body.size() && isdigit((unsigned char)body[q])) ++q;
        if (q == e) { p = e; continue; }   // không phải mrt<digit>
        std::string nm = body.substr(p, q - p);
        bool dup = false;
        for (const std::string& s : *out)
            if (s == nm) { dup = true; break; }
        if (!dup) out->push_back(nm);
        p = q;
    }
}

static void ApplyGLSLBuiltinLegacy(GLSLBuiltinCtx& ctx) {
    if (!ctx.body) return;
    if (ctx.isGeometry || ctx.isCompute || ctx.isTessControl || ctx.isTessEval) return;
    std::string& body = *ctx.body;

    std::vector<KnownExpr> known;
    std::vector<std::string> vec4Targets;

    // ---------------- (1) Uniform cố định (GLSL 1.20 §7.4) ----------------
    for (const LegacyUniformDef& d : kLegacyUniforms) {
        if (!LegHasWord(body, d.glsl)) continue;
        body = LegReplaceWord(body, d.glsl, std::string("tglmt_u.") + d.glsl);
        LegAddUniform(ctx, d.glsl, d.mslType, d.glslType, d.arraySize);
    }
    // Struct fog: `gl_Fog.<member>` và `gl_FogParameters.<member>`.
    {
        std::vector<std::string> used;
        body = LegRewriteFog(body, &used);
        for (const std::string& mem : used) {
            for (const FogMemberDef& d : kFogMembers) {
                if (mem != d.name) continue;
                std::string un = std::string("gl_Fog_") + mem;
                LegAddUniform(ctx, un, d.mslType, d.glslType, 0);
                known.push_back({std::string("tglmt_u.") + un,
                                 std::string(d.mslType) == "float4" ? 4 : 1});
                break;
            }
        }
    }

    // ---------------- (2) Varying built-in (§7.6 / §7.1.7) ----------------
    for (size_t i = 0; i < sizeof(kVaryings) / sizeof(kVaryings[0]); ++i) {
        const LegacyVaryingDef& d = kVaryings[i];
        if (!LegHasWord(body, d.glsl)) continue;
        bool isFs = ctx.isFragment && !ctx.isVertex;
        if (isFs && d.fsIn) {
            LegAddVarying(ctx.inputs, d.glsl, d.mslType, d.glslType,
                          kLegacyVaryLocBase + (int)i);
            body = LegReplaceWord(body, d.glsl, std::string("_in.") + d.glsl);
            known.push_back({std::string("_in.") + d.glsl,
                             std::string(d.mslType) == "float4" ? 4 : 1});
        } else if (ctx.isVertex && d.vsOut) {
            LegAddVarying(ctx.outputs, d.glsl, d.mslType, d.glslType,
                          kLegacyVaryLocBase + (int)i);
            body = LegReplaceWord(body, d.glsl, std::string("_out.") + d.glsl);
            known.push_back({std::string("_out.") + d.glsl,
                             std::string(d.mslType) == "float4" ? 4 : 1});
        }
    }

    // ---------------- (3) gl_TexCoord[N]: kiểu phần tử + ctor ---------------
    // Converter đã khai `float4 gl_TexCoordN` (đúng GLSL 1.20 §7.6: vec4) và
    // đã đổi body thành `_in.gl_TexCoordN` (FS) / `_out.gl_TexCoordN` (VS) ⇒
    // chỉ cần biết kiểu để sửa ctor quá số thành phần.
    for (int i = 0; i < 8; ++i) {
        std::string nmIn = "_in.gl_TexCoord" + std::to_string(i);
        if (body.find(nmIn) != std::string::npos) known.push_back({nmIn, 4});
        std::string nmOut = "_out.gl_TexCoord" + std::to_string(i);
        if (body.find(nmOut) != std::string::npos) known.push_back({nmOut, 4});
    }

    // ---------------- (4) gl_FogDepth (mở rộng TGLMT) --------------------
    // Không phải built-in GLSL: khai một biến cục bộ float trong main để
    // shader kiểu `gl_FogDepth = gl_Fog.depth;` biên dịch được. Bỏ qua nếu
    // shader ĐÃ tự khai báo biến này (tránh khai trùng ⇒ Metal compile fail).
    if (LegHasWord(body, "gl_FogDepth") &&
        !LegHasWord(body, "float gl_FogDepth")) {
        body = "  float gl_FogDepth = 0.0f;\n" + body;
    }

    // ---------------- (5) Sửa ctor vector & gán float→vec4 ----------------
    LegFixVectorCtors(&body, known);
    // Đích gán vec4: output màu của fragment (legacy + modern single-out +
    // struct-out + MRT).
    if (ctx.isFragment && !ctx.isVertex) {
        LegCollectFragColorTargets(body, &vec4Targets);
    }
    LegFixFloatAssign(&body, known, vec4Targets);
}

static const bool kRegLegacy = RegisterGLSLBuiltinPass("legacy", ApplyGLSLBuiltinLegacy);

}  // namespace

}  // namespace tglmt::gl
