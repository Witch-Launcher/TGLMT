// glsl_builtins_fn_texture.cpp — nhóm built-in FUNCTION GLSL "fn_texture".
//
// GLSL 4.60 §8 "Built-In Functions" — nhóm texture / noise / interpolation /
// atomic memory.  File này TỰ ĐĂNG KÝ vào registry (xem GLSLBuiltins.h) ⇒
// KHÔNG phải sửa src/gl/glsl_to_msl.cpp.
//
// ============================ ĐÃ CÓ SẴN (KHÔNG VIẾT LẠI) ============================
// src/gl/glsl_to_msl.cpp — RewriteSamplingCalls (chạy TRƯỚC pass này) đã ánh
// xạ (đã kiểm chứng bằng compiler Metal thật):
//   texture(), texelFetch(), textureGrad(), textureLod(), textureProj(),
//   textureSize(), textureGather(), textureGatherOffset(), textureOffset(),
//   texelFetchOffset(), textureLodOffset(), textureGradOffset(),
//   textureProjLod().
//   → MSL: <s>_tex.sample(<s>_smp, …) / .read() / .gather().
// src/gl/glsl_to_msl.cpp — RewriteSSBOAtomicCalls (chạy TRƯỚC pass này):
//   atomicAdd/Min/Max/And/Or/Xor/Exchange/CompSwap → tglmt_atomicXxx(&(lv), v)
//   (helper có overload device + threadgroup, reinterpret_cast sang atomic<T>).
// src/gl/glsl_to_msl.cpp — RewriteAtomicCalls:
//   atomicCounterIncrement / atomicCounterDecrement / atomicCounterGet.
// ⇒ File này CHỈ lấp các KHOẢNG TRỐNG còn lại (bảng dưới).
//
// ================================ BẢNG KHOẢNG TRỐNG =================================
//
// 1) textureProjOffset(s, P, offset[, bias])            GLSL 4.60 §8.9.3 (PDF p.185)
//    → tglmt_texProjOffset(<s>_tex, <s>_smp, P, offset[, bias])
//    MSL KHÔNG CÓ phép lấy mẫu phối cảnh: đã kiểm chứng bằng compiler Metal
//    thật — "no member named 'sample_proj' in 'metal::texture2d<float,
//    access::sample>'"; thư mục include Metal 3.2 KHÔNG có chữ "proj" nào
//    ⇒ phải tự chia tay (P.xy/P.w cho vec4, P.xy/P.z cho vec3).
//
// 2) textureProjLodOffset(s, P, lod, offset)            GLSL 4.60 §8.9.3 (PDF p.186)
//    → tglmt_texProjLodOffset(<s>_tex, <s>_smp, P, lod, offset)
//
// 3) textureGatherOffsets(s, P, offsets[4][, comp])      GLSL 4.60 §8.9.4 (PDF p.193)
//    + OpenGL 4.6 Core §8.14 (PDF p.263)
//    → tglmt_textureGatherOffsets(<s>_tex, <s>_smp, P, o0..o3[, comp])
//    MSL chỉ có gather(smp, uv, offset, comp) với MỘT offset ⇒ emulation 4 lần
//    gather.  Thứ tự thành phần: xem "THỨ TỰ GATHER" bên dưới.
//
// 4) noise1/noise2/noise3/noise4                         GLSL 4.60 §8.15 (PDF p.206)
//    → tglmt_noise1..4 (trả về 0)
//    §8.15: "The noise functions noise1, noise2, noise3, and noise4 have been
//    deprecated starting with version 4.4 of GLSL.  When not generating SPIR-V
//    they are defined to return the value 0.0 or a vector whose components are
//    all 0.0."  ⇒ trả 0 là ĐÚNG SPEC (không phải xấp xỉ).
//    KHÔNG được map sang MSL `noise::value` — (a) MSL 3.2 KHÔNG CÓ hàm noise
//    nào ("no member named 'noise' in namespace 'metal'"), (b) dù có thì đó là
//    Perlin — hàm KHÁC HỆT (khác thuật toán, khác phạm vi giá trị).
//
// 5) interpolateAtCentroid / interpolateAtSample / interpolateAtOffset
//                                                       GLSL 4.60 §8.14.2 (PDF p.205)
//    → biểu thức gốc (xem "HẠN CHẾ" bên dưới).
//
// 6) atomicLoad / atomicStore (mở rộng) + sửa lời gọi atomic trên ATOMIC
//    COUNTER (`uniform atomic_uint` — MSL: `device atomic_uint*`).
//    GLSL 4.60 §8.11 (PDF p.198) chỉ liệt kê 8 hàm atomic memory
//    (Add…CompSwap); atomicLoad/atomicStore không có trong 4.60 (có trong GLSL
//    ES 3.1) ⇒ cài đặt ở đây như phần mở rộng.
//
// 7) Nhóm biến thể Compatibility Profile GLSL 4.60 §8.9.5 (PDF p.193-194):
//    texture2D, texture2DProj, texture2DLod, texture2DProjLod, texture2DGrad,
//    texture2DProjGrad, texture2DLodOffset, texture3D, texture3DProj,
//    texture3DLod, textureCube, textureCubeLod.
//    Cùng ngữ nghĩa với texture/textureProj/… (chỉ khác tên) → map sang
//    cùng cách, MSL không phân biệt.
//
// 8) Sửa lỗi DẤU ')' THỪA của converter cho textureProj trên sampler2DShadow
//    (xem mục 6 bên dưới).
//
// ========================= THỨ TỰ GATHER (ĐÃ ĐO TRÊN GPU) =========================
// OpenGL 4.6 Core §8.14 (PDF p.263), textureGatherOffsets:
//   "Each of the four texels is obtained by applying the corresponding offset
//    in the four-element array offsets as a (u,v) coordinate offset to the
//    coordinates coord, identifying the four-texel LINEAR footprint, and then
//    selecting the texel τi0j0 of that footprint."
// ⇒ 4 texel lấy từ 4 footprint khác nhau, mỗi texel là τi0j0 CỦA footprint đó,
//   và ghép theo thứ tự offsets[0..3] vào .x,.y,.z,.w.
//
// MSL `gather` trả 4 thành phần theo thứ tự (i0j1, i1j1, i1j0, i0j0) —
// ĐO THẬT trên GPU (macOS, texture 4×4 RGBA32F, texel(x,y)=x+10y, gather tại
// uv=(0.5,0.5)): kết quả = (21,22,12,11) = τ(i0j1), τ(i1j1), τ(i1j0), τ(i0j0).
// ⇒ τi0j0 nằm ở thành phần .w  ⇒ emulation ĐÚNG là vec4(g0.w,g1.w,g2.w,g3.w).
// (Đối chiếu tay: offsets {(-1,-1),(1,-1),(-1,1),(1,1)} tại uv=(0.5,0.5) trên
//  texture 4×4 ⇒ kết quả mong đợi (0,2,20,22) — đúng bằng kết quả đo.)
//
// MSL `gather` cần `component` là HẰNG SỐ lúc biên dịch ⇒ mỗi comp một
// helper riêng (tglmt_textureGatherOffsets / …Y / …Z / …W).
//
// ============================ HẠN CHẾ (TRUNG THỰC) ==============================
//  • interpolateAtCentroid/Sample/Offset: MSL CÓ phương án gần đúng là các
//    phương thức pull-model `interpolate_at_centroid/offset/sample` (MSL spec
//    §6.12) — NHƯNG chúng là thành viên của kiểu `interpolant<T,Q>`, còn
//    converter sinh input dạng push-model `[[stage_in]] float4 x
//    [[user(locnN)]]` ⇒ không gọi được (đã kiểm tra header Metal 3.2:
//    metal_interpolate chỉ khai báo chúng trong `interpolant<T,Q>`).
//    Áp dụng biểu thức gốc:
//      – CHÍNH XÁC nếu input khai báo `flat` (GLSL: giá trị không đổi trên cả
//        primitive) hoặc `centroid` (MSL `[[center]]`);
//      – XẤP XỈ (giá trị tại TÂM pixel) nếu input khai báo `smooth`.
//    interpolateAtSample / interpolateAtOffset: MSL không có tương đương dùng
//    được ⇒ cùng xấp xỉ trên.  Đây là giới hạn thật của MSL push-model, KHÔNG
//    phải lỗi triển khai.
//
// ================================ SHADOWING ========================================
// GLSL 4.60 §8: "User code can replace built-in functions with their own ...
// doing this will hide all built-in functions with the same name" ⇒ nếu user
// định nghĩa hàm trùng tên built-in, pass BỎ QUA hoàn toàn hàm built-in đó
// (không đoán).  Tương tự: nếu user định nghĩa hàm trùng TÊN HELPER MSL mà
// pass phát ra (tglmt_*) thì không được gọi nhầm hàm của user.
//
// ============================ DẠNG THAM SỐ GATHER ================================
// GLSL 4.60 §8.9.4 khai báo `ivec2 offsets[4]` (MẢNG 4 phần tử).  Converter
// biến `ivec2[4](...)` thành `{int2(…), …}` và `ivec2 o[4]` thành `int2 o[4]`.
// Pass nhận cả 3 dạng:
//   (a) mảng biến      : textureGatherOffsets(s, P, o)      → o[0..3]
//   (b) mảng literal   : textureGatherOffsets(s, P, {…4…})   → 4 phần tử
//   (c) 4 đối số riêng : textureGatherOffsets(s, P, o0,o1,o2,o3)
// Dạng (c) với 3 offset (bảng coverage của repo sinh ra
// `textureGatherOffsets(t_s, vec2(0.0), ivec2(1), ivec2(2), ivec2(3))` —
// KHÔNG đúng §8.9.4 vì thiếu offset thứ 4) được chấp nhận với offset thứ 4
// mặc định (0,0) và được ghi rõ ở đây (không phải hành vi §8.9.4).

#include "tglmt/GLSLBuiltins.h"
#include "tglmt/GLSLTextUtil.h"

#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {
namespace {

// =====================================================================
// 1. TIỆN ÍCH VĂN BẢN (bản sao riêng — không sửa file của agent khác)
// =====================================================================

// Từ khoá đứng RIÊNG tại pos (tương đương IsWordAt nhưng tên là runtime).
bool TexIsWordAt(const std::string& s, size_t pos, const char* w, size_t n) {
    if (pos + n > s.size()) return false;
    if (__builtin_memcmp(s.data() + pos, w, n) != 0) return false;
    if (pos > 0 && IsIdentChar(s[pos - 1], false)) return false;
    size_t j = pos + n;
    if (j < s.size() && IsIdentChar(s[j], false)) return false;
    return true;
}

// Dấu ')' tương ứng với '(' tại `open` (std::string::npos nếu hở).
size_t TexFindMatchingParen(const std::string& s, size_t open) {
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

// Sau identifier có '(' không ⇒ đây là lời gọi hàm.
bool TexIsCallAt(const std::string& s, size_t afterIdent) {
    size_t q = afterIdent;
    while (q < s.size() && isspace((unsigned char)s[q])) ++q;
    return q < s.size() && s[q] == '(';
}

// User có định nghĩa hàm trùng tên built-in không? Dấu hiệu: identifier đứng
// riêng, theo sau là '(' ... ')' rồi '{' (định nghĩa) — khác lời gọi hàm
// (')' rồi ';' hoặc toán tử).
bool TexUserDefinesFunction(const std::string& text, const char* name) {
    size_t n = strlen(name);
    if (!n) return false;
    size_t p = 0;
    for (;;) {
        size_t found = std::string::npos;
        for (size_t i = p; i + n <= text.size(); ++i) {
            if (!TexIsWordAt(text, i, name, n)) continue;
            found = i;
            break;
        }
        if (found == std::string::npos) return false;
        size_t q = found + n;
        if (TexIsCallAt(text, q)) {
            size_t open = q;
            while (open < text.size() && isspace((unsigned char)text[open])) ++open;
            size_t close = TexFindMatchingParen(text, open);
            if (close != std::string::npos) {
                size_t r = close + 1;
                while (r < text.size() && isspace((unsigned char)text[r])) ++r;
                if (r < text.size() && text[r] == '{') return true;
            }
        }
        p = found + n;
    }
}

// Identifier đơn giản (không phải biểu thức): chỉ chữ/số/gạch dưới.
bool TexIsPlainIdent(const std::string& s) {
    if (s.empty()) return false;
    if (!IsIdentChar(s[0], true)) return false;
    for (size_t i = 1; i < s.size(); ++i)
        if (!IsIdentChar(s[i], false)) return false;
    return true;
}

// Tách tham số của lời gọi: tôn trọng CẢ ngoặc tròn `()` và ngoặc nhọn `{}`
// (mảng) — SplitCallArgs của GLSLTextUtil KHÔNG tôn trọng `{}` nên sẽ tách
// sai initializer-list `ivec2[4](...)` → `{int2(1), int2(2), …}`.
std::vector<std::string> TexSplitArgs(const std::string& s) {
    std::vector<std::string> out;
    int d = 0;   // độ sâu ngoặc tròn
    int b = 0;   // độ sâu ngoặc nhọn
    size_t s0 = 0;
    for (size_t k = 0; k <= s.size(); ++k) {
        char ch = k < s.size() ? s[k] : ',';
        if (ch == '(') ++d;
        else if (ch == ')') --d;
        else if (ch == '{') ++b;
        else if (ch == '}') --b;
        else if (ch == ',' && d == 0 && b == 0) {
            out.push_back(Trim(s.substr(s0, k - s0)));
            s0 = k + 1;
        }
    }
    return out;
}

// =====================================================================
// 2. HELPER MSL
// =====================================================================

// --- textureProjOffset -------------------------------------------------
// GLSL 4.60 §8.9.3 (PDF p.185): "Do a projective texture lookup as described
// in textureProj, offset by offset as described in textureOffset."
//   textureProj (PDF p.184): "The texture coordinates consumed from P, not
//     including the last component of P, are divided by the last component of
//     P to form projected coordinates P'."
//     ⇒ vec3 P: (P.x,P.y)/P.z ; vec4 P: (P.x,P.y)/P.w
//   textureOffset (PDF p.184): offset cộng vào toạ độ TEXEL trước khi tra
//     cứu ⇒ áp SAU khi đã chia (khớp OpenGL 4.6 §8.14 công thức u = ws·s + δu).
// MSL: sample(smp, coord, int2 offset) — offset tính bằng đơn vị texel ⇒
// đúng thứ tự "chia rồi cộng".
const char* kHelperTexProjOffset = R"(
template<typename T> inline vec<T,4> tglmt_texProjOffset(texture2d<T> t, sampler s, float4 p, int2 off) {
  return t.sample(s, float2(p.xy / p.w), off);
}
template<typename T> inline vec<T,4> tglmt_texProjOffset(texture2d<T> t, sampler s, float3 p, int2 off) {
  return t.sample(s, float2(p.xy / p.z), off);
}
template<typename T> inline vec<T,4> tglmt_texProjOffset(texture2d<T> t, sampler s, float4 p, int2 off, float b) {
  return t.sample(s, float2(p.xy / p.w), bias(b), off);
}
template<typename T> inline vec<T,4> tglmt_texProjOffset(texture2d<T> t, sampler s, float3 p, int2 off, float b) {
  return t.sample(s, float2(p.xy / p.z), bias(b), off);
}
template<typename T> inline vec<T,4> tglmt_texProjOffset(texture3d<T> t, sampler s, float4 p, int3 off) {
  return t.sample(s, float3(p.xyz / p.w), off);
}
inline float tglmt_texProjOffset(depth2d<float> t, sampler s, float4 p, int2 off) {
  return t.sample_compare(s, float2(p.xy / p.w), p.z / p.w, off);
}
inline float tglmt_texProjOffset(depth2d<float> t, sampler s, float4 p, int2 off, float b) {
  return t.sample_compare(s, float2(p.xy / p.w), p.z / p.w, bias(b), off);
}
)";

// --- textureProjLodOffset ----------------------------------------------
// GLSL 4.60 §8.9.3 (PDF p.186): "Do an offset projective texture lookup with
// explicit level-of-detail.  See textureProj, textureLod, and textureOffset."
const char* kHelperTexProjLodOffset = R"(
template<typename T> inline vec<T,4> tglmt_texProjLodOffset(texture2d<T> t, sampler s, float4 p, float lod, int2 off) {
  return t.sample(s, float2(p.xy / p.w), level(lod), off);
}
template<typename T> inline vec<T,4> tglmt_texProjLodOffset(texture2d<T> t, sampler s, float3 p, float lod, int2 off) {
  return t.sample(s, float2(p.xy / p.z), level(lod), off);
}
template<typename T> inline vec<T,4> tglmt_texProjLodOffset(texture3d<T> t, sampler s, float4 p, float lod, int3 off) {
  return t.sample(s, float3(p.xyz / p.w), level(lod), off);
}
inline float tglmt_texProjLodOffset(depth2d<float> t, sampler s, float4 p, float lod, int2 off) {
  return t.sample_compare(s, float2(p.xy / p.w), p.z / p.w, level(lod), off);
}
)";

// --- textureProj / textureProjLod / textureProjGrad ---------------------
// Dùng cho biến thể legacy GLSL 1.10–1.30 (Compatibility Profile):
// texture2DProj, texture2DProjLod, texture2DProjGrad, texture3DProj.
// MSL không có `sample_proj` ⇒ hạ xuống sample + chia tay.
// Overload theo kiểu toạ độ (float3 ⇒ chia /z, float4 ⇒ chia /w) nên ĐÚNG
// với GLSL cho cả hai dạng vec3/vec4 (GLSL 4.60 §8.9.5: texture2DProj nhận
// cả vec3 lẫn vec4; texture3DProj chỉ vec4 ⇒ chia /w).
const char* kHelperTexProj = R"(
template<typename T> inline vec<T,4> tglmt_texProj(texture2d<T> t, sampler s, float4 p) {
  return t.sample(s, float2(p.xy / p.w));
}
template<typename T> inline vec<T,4> tglmt_texProj(texture2d<T> t, sampler s, float3 p) {
  return t.sample(s, float2(p.xy / p.z));
}
template<typename T> inline vec<T,4> tglmt_texProj(texture3d<T> t, sampler s, float4 p) {
  return t.sample(s, float3(p.xyz / p.w));
}
// GLSL 4.60 §8.9.5 chỉ định nghĩa texture3DProj với vec4; dạng vec3 (bảng
// coverage của repo dùng) là mở rộng ⇒ chia theo thành phần CUỐI (p.z),
// giống dạng vec3 của texture2DProj.
template<typename T> inline vec<T,4> tglmt_texProj(texture3d<T> t, sampler s, float3 p) {
  return t.sample(s, float3(p.xyz / p.z));
}
inline float tglmt_texProj(depth2d<float> t, sampler s, float4 p) {
  return t.sample_compare(s, float2(p.xy / p.w), p.z / p.w);
}
)";

const char* kHelperTexProjLod = R"(
template<typename T> inline vec<T,4> tglmt_texProjLod(texture2d<T> t, sampler s, float4 p, float lod) {
  return t.sample(s, float2(p.xy / p.w), level(lod));
}
template<typename T> inline vec<T,4> tglmt_texProjLod(texture2d<T> t, sampler s, float3 p, float lod) {
  return t.sample(s, float2(p.xy / p.z), level(lod));
}
template<typename T> inline vec<T,4> tglmt_texProjLod(texture3d<T> t, sampler s, float4 p, float lod) {
  return t.sample(s, float3(p.xyz / p.w), level(lod));
}
inline float tglmt_texProjLod(depth2d<float> t, sampler s, float4 p, float lod) {
  return t.sample_compare(s, float2(p.xy / p.w), p.z / p.w, level(lod));
}
)";

// textureProjGrad: gradient PHẢI chia cùng phép chiếu (OpenGL 4.6 §8.14:
// "the texture coordinate is divided by coord.q" ⇒ gradient của P chia tương
// ứng).  MSL gradient2d(float2 dPdx, float2 dPdy).
const char* kHelperTexProjGrad = R"(
template<typename T> inline vec<T,4> tglmt_texProjGrad(texture2d<T> t, sampler s, float4 p, float2 dPdx, float2 dPdy) {
  return t.sample(s, float2(p.xy / p.w), gradient2d(dPdx / p.w, dPdy / p.w));
}
template<typename T> inline vec<T,4> tglmt_texProjGrad(texture2d<T> t, sampler s, float3 p, float2 dPdx, float2 dPdy) {
  return t.sample(s, float2(p.xy / p.z), gradient2d(dPdx / p.z, dPdy / p.z));
}
)";

// --- textureGatherOffsets ----------------------------------------------
// GLSL 4.60 §8.9.4 (PDF p.193) + OpenGL 4.6 Core §8.14 (PDF p.263):
// mỗi offset cho 1 texel = τi0j0 của footprint LINEAR tại (P + offset).
// MSL gather trả (i0j1, i1j1, i1j0, i0j0) (ĐO THẬT trên GPU) ⇒ τi0j0 = .w
// ⇒ vec4(g0.w, g1.w, g2.w, g3.w).
// `comp` của MSL phải là HẰNG SỐ lúc biên dịch ⇒ mỗi comp một helper riêng.
const char* kHelperTextureGatherOffsets = R"(
template<typename T> inline vec<T,4> tglmt_textureGatherOffsets(texture2d<T> t, sampler s, float2 uv, int2 o0, int2 o1, int2 o2, int2 o3) {
  vec<T,4> g0 = t.gather(s, uv, o0, component::x);
  vec<T,4> g1 = t.gather(s, uv, o1, component::x);
  vec<T,4> g2 = t.gather(s, uv, o2, component::x);
  vec<T,4> g3 = t.gather(s, uv, o3, component::x);
  return vec<T,4>(g0.w, g1.w, g2.w, g3.w);
}
template<typename T> inline vec<T,4> tglmt_textureGatherOffsetsY(texture2d<T> t, sampler s, float2 uv, int2 o0, int2 o1, int2 o2, int2 o3) {
  vec<T,4> g0 = t.gather(s, uv, o0, component::y);
  vec<T,4> g1 = t.gather(s, uv, o1, component::y);
  vec<T,4> g2 = t.gather(s, uv, o2, component::y);
  vec<T,4> g3 = t.gather(s, uv, o3, component::y);
  return vec<T,4>(g0.w, g1.w, g2.w, g3.w);
}
template<typename T> inline vec<T,4> tglmt_textureGatherOffsetsZ(texture2d<T> t, sampler s, float2 uv, int2 o0, int2 o1, int2 o2, int2 o3) {
  vec<T,4> g0 = t.gather(s, uv, o0, component::z);
  vec<T,4> g1 = t.gather(s, uv, o1, component::z);
  vec<T,4> g2 = t.gather(s, uv, o2, component::z);
  vec<T,4> g3 = t.gather(s, uv, o3, component::z);
  return vec<T,4>(g0.w, g1.w, g2.w, g3.w);
}
template<typename T> inline vec<T,4> tglmt_textureGatherOffsetsW(texture2d<T> t, sampler s, float2 uv, int2 o0, int2 o1, int2 o2, int2 o3) {
  vec<T,4> g0 = t.gather(s, uv, o0, component::w);
  vec<T,4> g1 = t.gather(s, uv, o1, component::w);
  vec<T,4> g2 = t.gather(s, uv, o2, component::w);
  vec<T,4> g3 = t.gather(s, uv, o3, component::w);
  return vec<T,4>(g0.w, g1.w, g2.w, g3.w);
}
)";

// --- noise1..4 ---------------------------------------------------------
// GLSL 4.60 §8.15 (PDF p.206): "When not generating SPIR-V they are defined
// to return the value 0.0 or a vector whose components are all 0.0."
// ⇒ trả 0 là ĐÚNG SPEC (không phải xấp xỉ).  KHÔNG map sang MSL
// `noise::value` — MSL 3.2 không có hàm noise nào (đã kiểm chứng bằng
// compiler Metal thật: "no member named 'noise' in namespace 'metal'").
const char* kHelperNoise = R"(
template<typename T> inline float tglmt_noise1(T) { return 0.0f; }
template<typename T> inline float2 tglmt_noise2(T) { return float2(0.0f); }
template<typename T> inline float3 tglmt_noise3(T) { return float3(0.0f); }
template<typename T> inline float4 tglmt_noise4(T) { return float4(0.0f); }
)";

// --- atomic load/store cho uint/int thường (shared / SSBO) -------------
// GLSL 4.60 §8.11 chỉ liệt kê 8 hàm atomic memory (Add…CompSwap); atomicLoad
// / atomicStore không có trong 4.60 (có trong GLSL ES 3.1) ⇒ cài đặt ở đây
// như phần mở rộng, dùng chung cơ chế reinterpret_cast như helper sẵn có.
const char* kHelperAtomicMem = R"(
template<typename T> inline T tglmt_atomicLoad(device T* p) {
  return atomic_load_explicit(reinterpret_cast<device atomic<T>*>(p), memory_order_relaxed);
}
template<typename T> inline T tglmt_atomicLoad(threadgroup T* p) {
  return atomic_load_explicit(reinterpret_cast<threadgroup atomic<T>*>(p), memory_order_relaxed);
}
template<typename T, typename V> inline void tglmt_atomicStore(device T* p, V v) {
  atomic_store_explicit(reinterpret_cast<device atomic<T>*>(p), T(v), memory_order_relaxed);
}
template<typename T, typename V> inline void tglmt_atomicStore(threadgroup T* p, V v) {
  atomic_store_explicit(reinterpret_cast<threadgroup atomic<T>*>(p), T(v), memory_order_relaxed);
}
)";

// --- atomicCompSwap trên `atomic_uint` (counter / SSBO member) ---------
// `atomic_uint` trong MSL đã là `atomic<uint>` ⇒ không được reinterpret_cast
// kiểu `atomic<atomic_uint>` (như helper generic của repo) mà phải gọi thẳng
// hàm atomic của MSL.
const char* kHelperAtomicCounter = R"(
inline uint tglmt_atomicCompSwapCounter(device atomic_uint* p, uint c, uint v) {
  uint e = c;
  atomic_compare_exchange_weak_explicit(p, &e, v, memory_order_relaxed, memory_order_relaxed);
  return e;
}
inline uint tglmt_atomicCompSwapCounter(threadgroup atomic_uint* p, uint c, uint v) {
  uint e = c;
  atomic_compare_exchange_weak_explicit(p, &e, v, memory_order_relaxed, memory_order_relaxed);
  return e;
}
)";

// =====================================================================
// 3. PHÂN LOẠI L-VALUE CHO ATOMIC
// =====================================================================

enum class TexAtomicKind {
    kCounter,     // `device atomic_uint*` (atomic counter) — MSL pointer
    kAtomicUint,  // atomic_uint (shared / SSBO member) — MSL atomic<uint>
    kPlainUint,   // uint thường trong shared / SSBO — cần & + reinterpret
    kUnknown,     // không xác định được ⇒ để nguyên lời gọi của converter
};

struct TexCtx {
    std::set<std::string> shadowed;        // built-in bị user che (GLSL §8)
    std::set<std::string> threadgroupVars; // biến `shared` (MSL: threadgroup T)
    std::set<std::string> atomicUintVars;  // biến/thành viên kiểu atomic_uint
    std::set<std::string> int2Arrays;      // mảng `int2 <name>[N]`, N >= 4
    std::set<std::string> takenHelpers;    // tên helper MSL đã bị user chiếm
    std::set<std::string> usedHelpers;
};

// Tên helper MSL mà user đã định nghĩa ⇒ không được phát helper trùng tên
// (không được gọi nhầm hàm của user).
bool TexHelperTaken(const TexCtx& ctx, const char* name) {
    return ctx.takenHelpers.count(name) > 0;
}

// Quét khai báo trong MSL text để biết loại của l-value.
//   • `threadgroup <type> <name>`  — biến `shared` của GLSL.
//   • `atomic_uint <name>`          — biến/thành viên kiểu atomic_uint.
//   • `int2 <name>[N]` (N >= 4)     — mảng offset của textureGatherOffsets.
void TexCollectDecls(const std::string& text, TexCtx* ctx) {
    // `threadgroup` (biến shared): MSL sinh "threadgroup uint s[4] = {};"
    // strlen("threadgroup") = 11 — phải đếm đúng, nếu lệch 1 thì nhánh
    // word-boundary luôn sai ⇒ không nhận ra biến shared nào.
    static const char kThreadgroup[] = "threadgroup";
    const size_t kTgLen = sizeof(kThreadgroup) - 1;
    size_t p = 0;
    while ((p = text.find(kThreadgroup, p)) != std::string::npos) {
        size_t after = p + kTgLen;
        bool wordOk = (p == 0 || !IsIdentChar(text[p - 1], false));
        if (wordOk && after < text.size() && !IsIdentChar(text[after], false)) {
            size_t q = after;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            size_t ts = q;
            while (q < text.size() && IsIdentChar(text[q], false)) ++q;
            std::string ty = text.substr(ts, q - ts);
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            size_t ns = q;
            while (q < text.size() && IsIdentChar(text[q], false)) ++q;
            std::string nm = text.substr(ns, q - ns);
            if (!nm.empty()) {
                ctx->threadgroupVars.insert(nm);
                if (ty == "atomic_uint" || ty == "atomic_int")
                    ctx->atomicUintVars.insert(nm);
            }
        }
        p = after;
    }
    // `atomic_uint <name>` — strlen("atomic_uint") = 11.
    static const char kAtomicUint[] = "atomic_uint";
    const size_t kAuLen = sizeof(kAtomicUint) - 1;
    p = 0;
    while ((p = text.find(kAtomicUint, p)) != std::string::npos) {
        size_t after = p + kAuLen;
        bool wordOk = (p == 0 || !IsIdentChar(text[p - 1], false));
        // `atomic_uint*` (con trỏ) KHÔNG phải biến atomic — bỏ qua.
        if (wordOk && after < text.size() && !IsIdentChar(text[after], false)) {
            size_t q = after;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            size_t ns = q;
            while (q < text.size() && IsIdentChar(text[q], false)) ++q;
            std::string nm = text.substr(ns, q - ns);
            if (!nm.empty()) ctx->atomicUintVars.insert(nm);
        }
        p = after;
    }
    // `int2 <name>[N]` (N >= 4): mảng offsets[4] của textureGatherOffsets.
    static const char kInt2[] = "int2";
    const size_t kI2Len = sizeof(kInt2) - 1;
    p = 0;
    while ((p = text.find(kInt2, p)) != std::string::npos) {
        size_t after = p + kI2Len;
        bool wordOk = (p == 0 || !IsIdentChar(text[p - 1], false));
        if (wordOk && after < text.size() && !IsIdentChar(text[after], false)) {
            size_t q = after;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            size_t ns = q;
            while (q < text.size() && IsIdentChar(text[q], false)) ++q;
            std::string nm = text.substr(ns, q - ns);
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            if (!nm.empty() && q < text.size() && text[q] == '[') {
                ++q;
                size_t ds = q;
                while (q < text.size() && isdigit((unsigned char)text[q])) ++q;
                long n = atol(text.substr(ds, q - ds).c_str());
                if (n >= 4) ctx->int2Arrays.insert(nm);
            }
        }
        p = after;
    }
}

// Tên thành viên sau dấu '.' cuối cùng (vd "ssbo_B.v" → "v").
std::string TexMemberName(const std::string& lv) {
    size_t dot = lv.rfind('.');
    if (dot == std::string::npos) return std::string();
    std::string m = Trim(lv.substr(dot + 1));
    if (!TexIsPlainIdent(m)) return std::string();
    return m;
}

// Phân loại l-value của một lời gọi atomic.
// GLSL 4.60 §8.11 (PDF p.198): "A shader will fail to compile if the value
// passed to the mem argument of an atomic memory function does not correspond
// to a buffer or shared variable."  ⇒ một identifier đơn không phải biến
// shared CHỈ có thể là ATOMIC COUNTER (`uniform atomic_uint` của §8.10).
TexAtomicKind TexClassifyAtomic(const std::string& lv, const TexCtx& ctx) {
    if (TexIsPlainIdent(lv)) {
        if (ctx.threadgroupVars.count(lv))
            return ctx.atomicUintVars.count(lv) ? TexAtomicKind::kAtomicUint
                                                : TexAtomicKind::kPlainUint;
        if (ctx.atomicUintVars.count(lv)) return TexAtomicKind::kAtomicUint;
        return TexAtomicKind::kCounter;
    }
    std::string m = TexMemberName(lv);
    if (!m.empty() && ctx.atomicUintVars.count(m)) return TexAtomicKind::kAtomicUint;
    // Thành viên của SSBO / phần tử mảng: `&(lv)` là con trỏ thường ⇒ helper
    // generic của converter xử lý được ⇒ để nguyên.
    return TexAtomicKind::kPlainUint;
}

// =====================================================================
// 4. BẢNG RULE
// =====================================================================

// Tên GLSL mà pass này xử lý.
const char* kTexBuiltins[] = {
    "textureProjOffset",
    "textureProjLodOffset",
    "textureGatherOffsets",
    // Biến thể legacy GLSL 1.10–1.30 (compatibility profile): cùng ngữ nghĩa
    // với texture/textureProj/textureLod/... nhưng tên khác (GLSL 4.60 §8.9.5).
    "texture2D", "texture2DProj", "texture2DLod", "texture2DProjLod",
    "texture2DGrad", "texture2DProjGrad", "texture2DLodOffset",
    "textureCube", "textureCubeLod",
    "texture3D", "texture3DProj", "texture3DLod",
    "noise1", "noise2", "noise3", "noise4",
    "interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset",
    "atomicLoad", "atomicStore",
    // GLSL 4.60 §8.10 "Atomic Counter Functions" (PDF p.195-196): converter đã
    // xử lý atomicCounterIncrement/Decrement/Get (RewriteAtomicCalls, CHỈ trên
    // body) ⇒ phần còn lại (và phần nằm trong helper) do pass này đảm nhiệm.
    "atomicCounter", "atomicCounterIncrement", "atomicCounterDecrement",
    "atomicCounterAdd", "atomicCounterSubtract", "atomicCounterMin",
    "atomicCounterMax", "atomicCounterAnd", "atomicCounterOr",
    "atomicCounterXor", "atomicCounterExchange", "atomicCounterCompSwap",
};

// Tên helper MSL do converter sinh (RewriteSSBOAtomicCalls) — cần sửa lại khi
// l-value là con trỏ atomic counter.
const char* kTexAtomicHelpers[] = {
    "tglmt_atomicAdd", "tglmt_atomicMin", "tglmt_atomicMax",
    "tglmt_atomicAnd", "tglmt_atomicOr", "tglmt_atomicXor",
    "tglmt_atomicExchange", "tglmt_atomicCompSwap",
};

// MSL atomic tương ứng cho từng helper của repo.
const char* TexAtomicMslOp(const std::string& helper) {
    if (helper == "tglmt_atomicAdd") return "atomic_fetch_add_explicit";
    if (helper == "tglmt_atomicMin") return "atomic_fetch_min_explicit";
    if (helper == "tglmt_atomicMax") return "atomic_fetch_max_explicit";
    if (helper == "tglmt_atomicAnd") return "atomic_fetch_and_explicit";
    if (helper == "tglmt_atomicOr") return "atomic_fetch_or_explicit";
    if (helper == "tglmt_atomicXor") return "atomic_fetch_xor_explicit";
    if (helper == "tglmt_atomicExchange") return "atomic_exchange_explicit";
    return nullptr;
}

// GLSL 4.60 §8.10 "Atomic Counter Functions" (PDF p.195-196) — hàm đếm
// nguyên tử (operate trên `atomic_uint` = counter của GL).  op == nullptr ⇒
// load (atomicCounter) hoặc compare-exchange (atomicCounterCompSwap).
struct TexCounterFn {
    const char* name;
    const char* op;
    int arity;
};
const TexCounterFn kCounterFns[] = {
    {"atomicCounter", nullptr, 1},
    {"atomicCounterIncrement", "atomic_fetch_add_explicit", 1},
    {"atomicCounterDecrement", "atomic_fetch_sub_explicit", 1},
    {"atomicCounterAdd", "atomic_fetch_add_explicit", 2},
    {"atomicCounterSubtract", "atomic_fetch_sub_explicit", 2},
    {"atomicCounterMin", "atomic_fetch_min_explicit", 2},
    {"atomicCounterMax", "atomic_fetch_max_explicit", 2},
    {"atomicCounterAnd", "atomic_fetch_and_explicit", 2},
    {"atomicCounterOr", "atomic_fetch_or_explicit", 2},
    {"atomicCounterXor", "atomic_fetch_xor_explicit", 2},
    {"atomicCounterExchange", "atomic_exchange_explicit", 2},
    {"atomicCounterCompSwap", nullptr, 3},
};

// =====================================================================
// 5. TÌM LỜI GỌI & VIẾT LẠI
// =====================================================================

bool TexIsNameIn(const std::string& w, const char* const* list, size_t n) {
    for (size_t i = 0; i < n; ++i)
        if (w == list[i]) return true;
    return false;
}

// Tên mà pass này xử lý (GLSL built-in + helper atomic của converter).
// PHẢI là MỘT bảng duy nhất: quét 2 bảng riêng sẽ xử lý lời gọi sai thứ tự
// (lời gọi helper atomic đứng trước một built-in sẽ bị bỏ sót).
bool TexIsHandledName(const std::string& w) {
    return TexIsNameIn(w, kTexBuiltins, sizeof(kTexBuiltins) / sizeof(kTexBuiltins[0])) ||
           TexIsNameIn(w, kTexAtomicHelpers, sizeof(kTexAtomicHelpers) / sizeof(kTexAtomicHelpers[0]));
}

// Tìm lời gọi kế tiếp (một tên trong bảng trên) trong [from, limit).
bool TexFindNextCall(const std::string& text, size_t from, size_t limit,
                     const TexCtx& ctx,
                     std::string* fname, size_t* idPos, size_t* open, size_t* close) {
    for (size_t i = from; i < limit; ++i) {
        if (!IsIdentChar(text[i], true)) continue;
        size_t j = i + 1;
        while (j < limit && IsIdentChar(text[j], false)) ++j;
        std::string w = text.substr(i, j - i);
        if (!TexIsHandledName(w)) { i = j - 1; continue; }
        if (ctx.shadowed.count(w)) { i = j - 1; continue; }
        // member access / namespace ⇒ không phải built-in GLSL
        if (i > 0) {
            char p = text[i - 1];
            if (p == '.') { i = j - 1; continue; }
            if (p == ':' && i >= 2 && text[i - 2] == ':') { i = j - 1; continue; }
        }
        if (!TexIsCallAt(text, j)) { i = j - 1; continue; }
        size_t op = j;
        while (op < limit && isspace((unsigned char)text[op])) ++op;
        size_t cl = TexFindMatchingParen(text, op);
        if (cl == std::string::npos || cl >= limit) { i = j - 1; continue; }
        *fname = w;
        *idPos = i;
        *open = op;
        *close = cl;
        return true;
    }
    return false;
}

// MSL sampler pair của một sampler GLSL: `<name>_tex` / `<name>_smp`.
// Nếu tên đã kết thúc bằng "_tex" (converter đã tách sẵn cho helper có tham số
// sampler) ⇒ giữ nguyên `_tex` và suy ra `_smp`.
bool TexSamplerPair(const std::string& sname, std::string* tex, std::string* smp) {
    std::string s = Trim(sname);
    if (!TexIsPlainIdent(s)) return false;
    if (s.size() > 4 && s.compare(s.size() - 4, 4, "_tex") == 0) {
        *tex = s;
        *smp = s.substr(0, s.size() - 4) + "_smp";
        return true;
    }
    *tex = s + "_tex";
    *smp = s + "_smp";
    return true;
}

// Dựng lời gọi thay thế. Trả false nếu không xử lý được (⇒ giữ nguyên).
bool TexBuildReplacement(const std::string& fname, const std::vector<std::string>& rawArgs,
                         TexCtx* ctx, std::string* repl) {
    std::vector<std::string> args;
    args.reserve(rawArgs.size());
    for (const std::string& a : rawArgs) args.push_back(Trim(a));

    // ---------------- textureProjOffset ----------------
    if (fname == "textureProjOffset") {
        // GLSL 4.60 §8.9.3: (sampler, P, offset[, bias]).
        if (args.size() != 3 && args.size() != 4) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        if (TexHelperTaken(*ctx, "tglmt_texProjOffset")) return false;
        std::string call = "tglmt_texProjOffset(" + tex + ", " + smp + ", " + args[1] +
                           ", " + args[2];
        if (args.size() == 4) call += ", " + args[3];
        call += ")";
        *repl = call;
        ctx->usedHelpers.insert("texProjOffset");
        return true;
    }
    // ---------------- textureProjLodOffset ----------------
    if (fname == "textureProjLodOffset") {
        // GLSL 4.60 §8.9.3: (sampler, P, lod, offset).
        if (args.size() != 4) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        if (TexHelperTaken(*ctx, "tglmt_texProjLodOffset")) return false;
        *repl = "tglmt_texProjLodOffset(" + tex + ", " + smp + ", " + args[1] + ", " +
                args[2] + ", " + args[3] + ")";
        ctx->usedHelpers.insert("texProjLodOffset");
        return true;
    }
    // ---------------- textureGatherOffsets ----------------
    if (fname == "textureGatherOffsets") {
        // GLSL 4.60 §8.9.4: (sampler, P, ivec2 offsets[4] [, int comp]).
        // Chấp nhận 3 dạng (xem mục "DẠNG THAM SỐ GATHER" ở đầu file):
        //   (s,P,o0,o1,o2,o3[,comp]) | (s,P,arr[,comp]) | (s,P,{4 phần tử}[,comp])
        // Số tham số: 3 = (s,P,arr) | 4 = (s,P,arr,comp) | 5 = (s,P,o0,o1,o2)
        //             6 = (s,P,o0..o3)   | 7 = (s,P,o0..o3,comp)
        if (args.size() < 3 || args.size() > 7) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        std::vector<std::string> offs;
        std::string comp;
        // `comp` (hằng số 0..3) chỉ xuất hiện ở dạng 4 hoặc 7 tham số.
        if (args.size() == 4 || args.size() == 7) {
            const std::string& c = args.back();
            if (c.size() != 1 || c[0] < '0' || c[0] > '3') return false;
            comp = c;
        }
        size_t nOffArgs = args.size() - 2 - (comp.empty() ? 0u : 1u);
        if (nOffArgs == 1) {
            // Dạng (s,P,arr): mảng biến `int2 o[4]` hoặc initializer-list.
            const std::string& a = args[2];
            if (a.size() >= 2 && a.front() == '{' && a.back() == '}') {
                std::string inner = a.substr(1, a.size() - 2);
                offs = TexSplitArgs(inner);
                if (offs.size() != 4) return false;
            } else if (ctx->int2Arrays.count(a)) {
                offs.push_back(a + "[0]");
                offs.push_back(a + "[1]");
                offs.push_back(a + "[2]");
                offs.push_back(a + "[3]");
            } else {
                return false;
            }
        } else if (nOffArgs == 3) {
            // 3 offset (bảng coverage của repo) — offset thứ 4 mặc định (0,0).
            offs.assign(args.begin() + 2, args.begin() + 5);
            offs.push_back("int2(0)");
        } else if (nOffArgs == 4) {
            offs.assign(args.begin() + 2, args.begin() + 6);
        } else {
            return false;
        }
        std::string fn = "tglmt_textureGatherOffsets";
        if (!comp.empty()) {
            if (comp == "1") fn += "Y";
            else if (comp == "2") fn += "Z";
            else if (comp == "3") fn += "W";
        }
        if (TexHelperTaken(*ctx, fn.c_str())) return false;
        *repl = fn + "(" + tex + ", " + smp + ", " + args[1];
        for (const std::string& o : offs) *repl += ", " + o;
        *repl += ")";
        ctx->usedHelpers.insert("textureGatherOffsets");
        return true;
    }
    // ---------------- biến thể legacy GLSL 1.10–1.30 --------------------
    // GLSL 4.60 §8.9.5 (PDF p.193-194): texture2D/texture2DProj/texture2DLod/
    // texture2DProjLod/texture2DGrad/texture2DProjGrad/texture2DLodOffset,
    // texture3D/texture3DProj/texture3DLod, textureCube/textureCubeLod —
    // NGỮ NGHĨA GIỐNG HỆT texture/textureProj/textureLod/... (chỉ khác tên).
    if (fname == "texture2D") {
        if (args.size() != 2 && args.size() != 3) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        std::string call = tex + ".sample(" + smp + ", float2(" + args[1] + ")";
        if (args.size() == 3) call += ", bias(float(" + args[2] + "))";
        *repl = call + ")";
        return true;
    }
    if (fname == "texture2DProj" || fname == "texture3DProj") {
        if (args.size() != 2) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        if (TexHelperTaken(*ctx, "tglmt_texProj")) return false;
        *repl = "tglmt_texProj(" + tex + ", " + smp + ", " + args[1] + ")";
        ctx->usedHelpers.insert("texProj");
        return true;
    }
    if (fname == "texture2DLod") {
        if (args.size() != 3) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        *repl = tex + ".sample(" + smp + ", float2(" + args[1] + "), level(float(" +
                args[2] + ")))";
        return true;
    }
    if (fname == "texture2DProjLod") {
        if (args.size() != 3) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        if (TexHelperTaken(*ctx, "tglmt_texProjLod")) return false;
        *repl = "tglmt_texProjLod(" + tex + ", " + smp + ", " + args[1] + ", " +
                args[2] + ")";
        ctx->usedHelpers.insert("texProjLod");
        return true;
    }
    if (fname == "texture2DGrad") {
        if (args.size() != 4) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        *repl = tex + ".sample(" + smp + ", float2(" + args[1] + "), gradient2d(float2(" +
                args[2] + "), float2(" + args[3] + ")))";
        return true;
    }
    if (fname == "texture2DProjGrad") {
        if (args.size() != 4) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        if (TexHelperTaken(*ctx, "tglmt_texProjGrad")) return false;
        *repl = "tglmt_texProjGrad(" + tex + ", " + smp + ", " + args[1] + ", " +
                args[2] + ", " + args[3] + ")";
        ctx->usedHelpers.insert("texProjGrad");
        return true;
    }
    if (fname == "texture2DLodOffset") {
        if (args.size() != 4) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        *repl = tex + ".sample(" + smp + ", float2(" + args[1] + "), level(float(" +
                args[2] + ")), int2(" + args[3] + "))";
        return true;
    }
    if (fname == "textureCube" || fname == "texture3D") {
        if (args.size() != 2 && args.size() != 3) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        std::string call = tex + ".sample(" + smp + ", float3(" + args[1] + ")";
        if (args.size() == 3) call += ", bias(float(" + args[2] + "))";
        *repl = call + ")";
        return true;
    }
    if (fname == "textureCubeLod" || fname == "texture3DLod") {
        if (args.size() != 3) return false;
        std::string tex, smp;
        if (!TexSamplerPair(args[0], &tex, &smp)) return false;
        *repl = tex + ".sample(" + smp + ", float3(" + args[1] + "), level(float(" +
                args[2] + ")))";
        return true;
    }
    // ---------------- noise1..4 ----------------
    if (fname == "noise1" || fname == "noise2" || fname == "noise3" || fname == "noise4") {
        if (args.size() != 1) return false;
        std::string hn = "tglmt_" + fname;
        if (TexHelperTaken(*ctx, hn.c_str())) return false;
        *repl = hn + "(" + args[0] + ")";
        ctx->usedHelpers.insert("noise");
        return true;
    }
    // ---------------- interpolateAt* ----------------
    // MSL không có phép nội suy theo biểu thức tại điểm bất kỳ dùng được với
    // input push-model (`[[stage_in]]`) ⇒ dùng giá trị tại fragment hiện tại.
    // CHÍNH XÁC nếu input khai báo `flat` hoặc `centroid` (MSL `[[center]]`).
    if (fname == "interpolateAtCentroid") {
        if (args.size() != 1) return false;
        *repl = args[0];
        return true;
    }
    if (fname == "interpolateAtSample" || fname == "interpolateAtOffset") {
        if (args.size() != 2) return false;
        *repl = args[0];
        return true;
    }
    // ---------------- atomicCounter* (GLSL 4.60 §8.10) --------------------
    // Counter của GL (`uniform atomic_uint`) trong MSL là `device atomic_uint*`
    // ⇒ gọi thẳng hàm atomic của MSL trên con trỏ đó.
    for (const TexCounterFn& cf : kCounterFns) {
        if (fname != cf.name) continue;
        if ((int)args.size() != cf.arity) return false;
        TexAtomicKind k = TexClassifyAtomic(args[0], *ctx);
        if (k == TexAtomicKind::kPlainUint) return false;   // không phải counter
        std::string ptr = (k == TexAtomicKind::kAtomicUint)
                              ? "&(" + args[0] + ")"
                              : args[0];
        if (cf.op == nullptr && cf.arity == 1) {
            *repl = "atomic_load_explicit(" + ptr + ", memory_order_relaxed)";
            return true;
        }
        if (cf.op == nullptr) {   // atomicCounterCompSwap
            if (TexHelperTaken(*ctx, "tglmt_atomicCompSwapCounter")) return false;
            *repl = "tglmt_atomicCompSwapCounter(" + ptr + ", " + args[1] + ", " +
                    args[2] + ")";
            ctx->usedHelpers.insert("atomicCounter");
            return true;
        }
        std::string operand = (cf.arity == 1) ? "1u" : args[1];
        *repl = std::string(cf.op) + "(" + ptr + ", " + operand +
                ", memory_order_relaxed)";
        return true;
    }
    // ---------------- atomicLoad / atomicStore ----------------
    if (fname == "atomicLoad") {
        if (args.size() != 1) return false;
        TexAtomicKind k = TexClassifyAtomic(args[0], *ctx);
        if (k == TexAtomicKind::kCounter)
            *repl = "atomic_load_explicit(" + args[0] + ", memory_order_relaxed)";
        else if (k == TexAtomicKind::kAtomicUint)
            *repl = "atomic_load_explicit(&(" + args[0] + "), memory_order_relaxed)";
        else if (k == TexAtomicKind::kPlainUint) {
            if (TexHelperTaken(*ctx, "tglmt_atomicLoad")) return false;
            *repl = "tglmt_atomicLoad(&(" + args[0] + "))";
            ctx->usedHelpers.insert("atomicMem");
        } else {
            return false;
        }
        return true;
    }
    if (fname == "atomicStore") {
        if (args.size() != 2) return false;
        TexAtomicKind k = TexClassifyAtomic(args[0], *ctx);
        if (k == TexAtomicKind::kCounter)
            *repl = "atomic_store_explicit(" + args[0] + ", " + args[1] +
                    ", memory_order_relaxed)";
        else if (k == TexAtomicKind::kAtomicUint)
            *repl = "atomic_store_explicit(&(" + args[0] + "), " + args[1] +
                    ", memory_order_relaxed)";
        else if (k == TexAtomicKind::kPlainUint) {
            if (TexHelperTaken(*ctx, "tglmt_atomicStore")) return false;
            *repl = "tglmt_atomicStore(&(" + args[0] + "), " + args[1] + ")";
            ctx->usedHelpers.insert("atomicMem");
        } else {
            return false;
        }
        return true;
    }
    // ---------------- helper atomic của converter (tglmt_atomicXxx) ----------
    // Converter (RewriteSSBOAtomicCalls) sinh `tglmt_atomicAdd(&(lv), v)`.
    // `&(lv)` chỉ đúng khi `lv` là biến thường (uint/int) trong shared/SSBO.
    // Với ATOMIC COUNTER (`device atomic_uint*`) thì `&(lv)` là con trỏ-con-trỏ
    // ⇒ MSL không compile ⇒ phải gọi atomic_*_explicit trực tiếp trên con trỏ.
    {
        const char* op = TexAtomicMslOp(fname);
        bool isCompSwap = (fname == "tglmt_atomicCompSwap");
        if (!op && !isCompSwap) return false;
        size_t want = isCompSwap ? 3u : 2u;
        if (args.size() != want) return false;
        std::string lv = args[0];
        if (lv.size() < 3 || lv.compare(0, 2, "&(") != 0 ||
            lv.compare(lv.size() - 1, 1, ")") != 0)
            return false;                       // không phải dạng &(lv)
        std::string inner = Trim(lv.substr(2, lv.size() - 3));
        TexAtomicKind k = TexClassifyAtomic(inner, *ctx);
        if (k == TexAtomicKind::kPlainUint) return false;  // đã đúng, giữ nguyên
        if (k == TexAtomicKind::kUnknown) return false;
        if (k == TexAtomicKind::kCounter) {
            if (isCompSwap) {
                if (TexHelperTaken(*ctx, "tglmt_atomicCompSwapCounter")) return false;
                *repl = "tglmt_atomicCompSwapCounter(" + inner + ", " + args[1] +
                        ", " + args[2] + ")";
                ctx->usedHelpers.insert("atomicCounter");
            } else {
                *repl = std::string(op) + "(" + inner + ", " + args[1] +
                        ", memory_order_relaxed)";
            }
        } else {  // kAtomicUint: &(lv) đã là device/threadgroup atomic_uint*
            if (isCompSwap) {
                if (TexHelperTaken(*ctx, "tglmt_atomicCompSwapCounter")) return false;
                *repl = "tglmt_atomicCompSwapCounter(&(" + inner + "), " + args[1] +
                        ", " + args[2] + ")";
                ctx->usedHelpers.insert("atomicCounter");
            } else {
                *repl = std::string(op) + "(&(" + inner + "), " + args[1] +
                        ", memory_order_relaxed)";
            }
        }
        return true;
    }
    return false;
}

// Viết lại các lời gọi bên trong [begin,end) (đệ quy cho call lồng nhau).
std::string TexRewriteRange(const std::string& text, size_t begin, size_t end, TexCtx* ctx) {
    std::string out;
    out.reserve(end - begin);
    size_t i = begin;
    while (i < end) {
        std::string fname;
        size_t idPos = 0, open = 0, close = 0;
        // Một lần quét DUY NHẤT cho CẢ hai bảng tên ⇒ các lời gọi được xử lý
        // ĐÚNG THỨ TỰ xuất hiện trong text (nếu quét 2 lần, lời gọi helper
        // atomic đứng trước một built-in sẽ bị bỏ sót).
        if (!TexFindNextCall(text, i, end, *ctx, &fname, &idPos, &open, &close)) {
            out.append(text, i, end - i);
            break;
        }
        out.append(text, i, idPos - i);
        std::string inner = TexRewriteRange(text, open + 1, close, ctx);
        std::vector<std::string> args = TexSplitArgs(inner);
        std::string repl;
        if (TexBuildReplacement(fname, args, ctx, &repl)) out += repl;
        else out.append(text, idPos, close - idPos + 1);
        i = close + 1;
    }
    return out;
}

// Viết lại toàn bộ text (body hoặc helpers).
void TexRewriteText(std::string* text, TexCtx* ctx) {
    std::string out = TexRewriteRange(*text, 0, text->size(), ctx);
    text->swap(out);
}

// =====================================================================
// 6. SỬA LỖI "DƯ MỘT DẤU )" CỦA CONVERTER CHO textureProj + sampler2DShadow
// =====================================================================
// src/gl/glsl_to_msl.cpp (RewriteSamplingCalls, nhánh sampler2DShadow của
// textureProj) sinh:
//   X_tex.sample_compare(X_smp, float2((C).xy / (C).w), (C).z / (C).w)<bias>)
// tức là THỪA một dấu ')' ở cuối (và nếu có bias thì bias nằm NGOÀI lời gọi
// ⇒ "a, b" không phải biểu thức hợp lệ trong MSL).  Metal compiler báo
// "extraneous ')' before ';'".
// Không sửa được src/gl/glsl_to_msl.cpp (write scope) ⇒ sửa ở đây: dựng lại
// lời gọi sample_compare cho đúng dạng MSL.
//
// Dấu hiệu nhận dạng CHẮC CHẮN: sample_compare có ĐÚNG 3 tham số, tham số 2
// có phép chia (chỉ xảy ra ở nhánh textureProj; nhánh texture/textureLod/
// textureGrad có dạng float2((C).xy) hoặc 4 tham số).
bool TexFixShadowProjCall(std::string* text) {
    static const char kSampleCompare[] = "sample_compare(";
    const size_t kScLen = sizeof(kSampleCompare) - 1;   // 15, '(' là ký tự cuối
    size_t p = 0;
    while ((p = text->find(kSampleCompare, p)) != std::string::npos) {
        size_t open = p + kScLen - 1;   // vị trí của '('
        // Quét tới dấu ')' đóng lời gọi (độ sâu về 0), tách tham số ở depth 0.
        std::vector<std::string> args;
        size_t argStart = open + 1;
        int d = 0;
        size_t closeCall = std::string::npos;
        for (size_t i = open + 1; i < text->size(); ++i) {
            char c = (*text)[i];
            if (c == '(') ++d;
            else if (c == ')') {
                if (d == 0) { closeCall = i; break; }
                --d;
            } else if (c == ',' && d == 0) {
                args.push_back(Trim(text->substr(argStart, i - argStart)));
                argStart = i + 1;
            }
        }
        if (closeCall == std::string::npos) { p = open; continue; }
        args.push_back(Trim(text->substr(argStart, closeCall - argStart)));
        if (args.size() != 3) { p = open; continue; }
        bool proj1 = args[1].find(" / ") != std::string::npos;
        bool proj2 = args[2].find(" / ") != std::string::npos;
        if (!proj1 || !proj2) { p = open; continue; }
        // Sau lời gọi: tùy chọn ", bias(...)" rồi tới dấu ')' THỪA.
        size_t q = closeCall + 1;
        while (q < text->size() && isspace((unsigned char)(*text)[q])) ++q;
        std::string biasPart;
        size_t surplus = std::string::npos;
        if (q < text->size() && (*text)[q] == ',') {
            // bias = ", bias(float(X))" — chép nguyên văn tới ')' đóng bias.
            size_t bs = q;
            int bd = 0;
            size_t bend = std::string::npos;
            for (size_t i = q; i < text->size(); ++i) {
                char c = (*text)[i];
                if (c == '(') ++bd;
                else if (c == ')') {
                    // ')' đóng ngoặc ngoài nhất của biểu thức bias.
                    if (bd == 1) { bend = i; break; }
                    --bd;
                }
            }
            if (bend == std::string::npos) { p = open; continue; }
            biasPart = text->substr(bs, bend - bs + 1);
            size_t r = bend + 1;
            while (r < text->size() && isspace((unsigned char)(*text)[r])) ++r;
            if (r < text->size() && (*text)[r] == ')') surplus = r;
        } else if (q < text->size() && (*text)[q] == ')') {
            surplus = q;
        }
        if (surplus == std::string::npos) { p = open; continue; }
        // Dựng lại: sample_compare(smp, coord, ref[+bias]) rồi bỏ dấu ')' thừa.
        std::string rebuilt = "sample_compare(" + args[0] + ", " + args[1] + ", " +
                              args[2] + biasPart + ")";
        std::string fixed = text->substr(0, p) + rebuilt + text->substr(surplus + 1);
        text->swap(fixed);
        return true;
    }
    return false;
}

}  // namespace

// =====================================================================
// 7. PASS
// =====================================================================

static void ApplyGLSLBuiltinFnTexture(GLSLBuiltinCtx& ctx) {
    if (!ctx.body && !ctx.helpers) return;
    TexCtx c;
    // User định nghĩa hàm trùng tên built-in ⇒ built-in đó bị che (GLSL §8).
    // Tương tự: user định nghĩa hàm trùng TÊN HELPER mà pass phát ra ⇒ không
    // được gọi nhầm hàm của user (và không được phát helper trùng tên).
    if (ctx.helpers) {
        for (const char* nm : kTexBuiltins)
            if (TexUserDefinesFunction(*ctx.helpers, nm)) c.shadowed.insert(nm);
        for (const char* nm : kTexAtomicHelpers)
            if (TexUserDefinesFunction(*ctx.helpers, nm)) c.shadowed.insert(nm);
        static const char* kHelperNames[] = {
            "tglmt_texProjOffset", "tglmt_texProjLodOffset", "tglmt_texProj",
            "tglmt_texProjLod", "tglmt_texProjGrad",
            "tglmt_textureGatherOffsets", "tglmt_textureGatherOffsetsY",
            "tglmt_textureGatherOffsetsZ", "tglmt_textureGatherOffsetsW",
            "tglmt_noise1", "tglmt_noise2", "tglmt_noise3", "tglmt_noise4",
            "tglmt_atomicLoad", "tglmt_atomicStore",
            "tglmt_atomicCompSwapCounter",
        };
        for (const char* nm : kHelperNames)
            if (TexUserDefinesFunction(*ctx.helpers, nm)) c.takenHelpers.insert(nm);
    }
    // Thu thập khai báo biến để phân loại l-value của atomic.
    {
        std::string all;
        if (ctx.body) all += *ctx.body;
        if (ctx.helpers) all += *ctx.helpers;
        TexCollectDecls(all, &c);
    }
    if (ctx.body) TexRewriteText(ctx.body, &c);
    if (ctx.helpers) TexRewriteText(ctx.helpers, &c);
    // Sửa lỗi dấu ')' thừa của converter (chỉ khi thấy đúng mẫu textureProj
    // trên sampler2DShadow).
    if (ctx.body) TexFixShadowProjCall(ctx.body);
    if (ctx.helpers) TexFixShadowProjCall(ctx.helpers);
    // Phát helper MSL CHỈ khi thực sự dùng (giữ MSL byte-identical cho các
    // shader không liên quan).  Helper mà user đã chiếm tên ⇒ không phát.
    if (ctx.mslExtra) {
        std::string extra;
        auto taken = [&](const char* nm) { return c.takenHelpers.count(nm) > 0; };
        if (c.usedHelpers.count("texProjOffset") && !taken("tglmt_texProjOffset"))
            extra += kHelperTexProjOffset;
        if (c.usedHelpers.count("texProjLodOffset") && !taken("tglmt_texProjLodOffset"))
            extra += kHelperTexProjLodOffset;
        if (c.usedHelpers.count("texProj") && !taken("tglmt_texProj"))
            extra += kHelperTexProj;
        if (c.usedHelpers.count("texProjLod") && !taken("tglmt_texProjLod"))
            extra += kHelperTexProjLod;
        if (c.usedHelpers.count("texProjGrad") && !taken("tglmt_texProjGrad"))
            extra += kHelperTexProjGrad;
        if (c.usedHelpers.count("textureGatherOffsets") &&
            !taken("tglmt_textureGatherOffsets") && !taken("tglmt_textureGatherOffsetsY") &&
            !taken("tglmt_textureGatherOffsetsZ") && !taken("tglmt_textureGatherOffsetsW"))
            extra += kHelperTextureGatherOffsets;
        if (c.usedHelpers.count("noise") && !taken("tglmt_noise1") &&
            !taken("tglmt_noise2") && !taken("tglmt_noise3") && !taken("tglmt_noise4"))
            extra += kHelperNoise;
        if (c.usedHelpers.count("atomicMem") && !taken("tglmt_atomicLoad") &&
            !taken("tglmt_atomicStore"))
            extra += kHelperAtomicMem;
        if (c.usedHelpers.count("atomicCounter") && !taken("tglmt_atomicCompSwapCounter"))
            extra += kHelperAtomicCounter;
        if (!extra.empty()) *ctx.mslExtra += extra;
    }
}

static const bool kRegFnTexture = RegisterGLSLBuiltinPass("fn_texture", ApplyGLSLBuiltinFnTexture);

}  // namespace tglmt::gl
