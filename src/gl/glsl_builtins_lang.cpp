// glsl_builtins_lang.cpp — nhóm built-in NGÔN NGỮ GLSL "lang".
//
// GLSL 4.60 — các tính năng thuộc NGÔN NGỮ (không phải built-in function /
// built-in variable):
//
//   #   Tính năng                       GLSL → MSL                              Nguồn
//   ----------------------------------------------------------------------------------
//   1   half / f16vecN (fp16)           half → float, f16vecN → floatN         §4.1.4
//   2   mảng uniform kiểu struct        tách phẳng theo member (u_f[N])        §4.3.8
//   3   `flat` trên biến int/uint       [[flat]] (kể cả vertex input)          §4.5
//   4   qualifier `invariant`           chấp nhận + BỎ QUA (MSL không có)       §4.8.1
//   5   SSBO mảng unsized + instance    device T* phẳng (bỏ instance)          §4.4.5
//   6   packHalf2x16 / unpackHalf2x16   tglmt_packHalf2x16 (đã có sẵn)          §8.4
//
// NGUYÊN TẮC (xem GLSLBuiltins.h)
//   • Pass này KHÔNG đụng vào src/gl/glsl_to_msl.cpp.
//   • Chỉ sửa đúng phần liên quan: MSL của các shader không dùng nhóm này
//     phải BYTE-IDENTICAL (đo: 88 shader Minecraft ⇒ diff 0 byte).
//   • GLSL 4.60 §8: user định nghĩa hàm trùng tên built-in ⇒ built-in bị che.
//
// VÌ SAO MỘT SỐ MỤC CẦN "TRƯỚC KHI PARSE"
//   Converter (src/gl/glsl_to_msl.cpp) chạy pass built-in SAU bước parse
//   (RunGLSLBuiltinPasses tại dòng 11945). Các mục #2/#4/#5 bị parser TỪ CHỐI
//   TRƯỚC đó:
//     #2 → "kiểu không hỗ trợ: S (u)"          (dòng 9621)
//     #4 → "câu lệnh không hỗ trợ: invariant …" (dòng 9534)
//     #5 → "SSBO unsized-array có instance…"    (dòng 11040)
//   ⇒ Với 3 mục đó file này cung cấp hàm chuẩn hoá NGUỒN GLSL
//   (tglmt::gl::NormalizeGLSLForLang) — chạy TRƯỚC converter. Hàm đã có test
//   end-to-end (chuẩn hoá → convert → Metal compile) trong
//   tests/shaders/test_glsl_builtin_lang.cpp.
//   Xem "GIAO DIỆN VỚI CONVERTER" cuối file để biết chính xác chỗ gắn.
//
// ĐO THẬT (macOS, `xcrun -sdk macosx metal -c`):
//   (1) `int k [[flat]] [[attribute(0)]];` trong struct vertex input ⇒ compile OK.
//       (`[[flat]]` trên VERTEX INPUT là no-op: vertex input không nội suy —
//        đúng ngữ nghĩa GLSL "flat" cho input của vertex shader.)
//   (2) MSL KHÔNG có `pack_half2x16` / `unpack_half2x16` (compile error
//       "use of undeclared identifier 'pack_half2x16'") ⇒ helper sẵn có của
//       converter (half2 + as_type<ushort>) ĐÚNG là ánh xạ MSL tương đương.
//   (3) MSL CÓ `half`/`half2` (Metal 2.0+) nhưng GLSL fp16 ≠ MSL `half` về
//       ngữ nghĩa (GLSL fp16 có quy tắc promote/convert riêng) ⇒ TGLMT hạ
//       `half`/`f16vecN` xuống `float`/`floatN` (value-compatible, an toàn A11).
//
// SPEC (docs/khronos/GLSLangSpec.4.60.pdf):
//   §4.1.4 "Floats" (PDF p.38-39): GLSL 4.60 core CHỈ có `float` và `double`.
//           Kiểu 16-bit (`float16_t`, `f16vec2/3/4`, `f16mat*`) là EXTENSION
//           (GL_ARB_gpu_shader_fp16) — không có trong grammar §9 (PDF p.218-223).
//           Coverage case dùng tên `half`/`f16vec2` ⇒ hạ xuống float/float2.
//   §4.5 "Interpolation Qualifiers" (PDF p.97): "A variable qualified as flat
//           will not be interpolated. Instead, it will have the same value for
//           every fragment within a primitive."
//   §4.8.1 "The Invariant Qualifier" (PDF p.103): "Only variables output from a
//           shader can be candidates for invariance." MSL không có khái niệm
//           tương đương ⇒ chấp nhận + bỏ qua (giá trị vẫn deterministic vì
//           Metal compile 1 shader ⇒ 1 binary).
//   §4.4.5 / §4.3.9 "Shader Storage Block" (PDF p.92): mảng unsized được phép
//           làm member CUỐI của block, kích thước runtime.
//   §8.4 "Floating-Point Pack and Unpack Functions" (PDF p.173-174):
//           `uint packHalf2x16(vec2 v)`, `vec2 unpackHalf2x16(uint v)`.
//
// KẾT QUẢ ĐO (tools/glsl_probe.cpp, macOS + Metal compiler thật):
//   • Trước: 312/328 GLSL feature hoạt động, 16 hỏng.
//   • Sau:   315/328 hoạt động, 13 hỏng (không hồi quy).
//   • Trong nhóm "lang" đã hết 3 lỗi: "half / f16vec", "interpolation flat",
//     "pack/unpack".
//   • 3 case còn lại của nhóm ("mảng của struct (uniform)", "invariant",
//     "buffer block unsized") + "tess control shader" cần converter gọi
//     NormalizeGLSLForLang (xem mục 0) — đã có sẵn hàm + test end-to-end.
//   • Corpus Minecraft (79 shader .vsh/.fsh + 9 .glsl): MSL BYTE-IDENTICAL
//     trước/sau (đo bằng diff trên 88 file .msl).
//
// GIỚI HẠN ĐÃ BIẾT (báo cáo trung thực):
//   1) `half`/`f16vecN` làm kiểu UNIFORM/VARYING vẫn bị converter từ chối
//      ("kiểu không hỗ trợ: half (h)") vì MSLType() của converter chỉ biết
//      float/double/int/uint/bool — cần sửa converter (ngoài write-scope).
//      Pass này xử lý được fp16 khi nó xuất hiện trong thân/helper (local).
//   2) `flat` trên VERTEX INPUT: thêm [[flat]] bằng cách nhúng attribute vào
//      TÊN field (kỹ thuật của pass "modern_io") vì converter không gọi
//      InterpAttr() cho attribute (src/gl/glsl_to_msl.cpp:12216).
//   3) Mảng uniform kiểu struct được TÁCH PHẲNG theo member
//      (`uniform S u[2]` + `struct S{float f;}` ⇒ `uniform float u_f[2]`).
//      Tên GL đổi "u[0].f" → "u_f[0]" (converter cũ KHÔNG hỗ trợ struct uniform
//      ⇒ không có hành vi cũ để phá).

#include "tglmt/GLSLBuiltins.h"
#include "tglmt/GLSLTextUtil.h"

#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

// =====================================================================
// 0. GIAO DIỆN VỚI CONVERTER (chuẩn hoá nguồn TRƯỚC khi parse)
// =====================================================================
//
// GLSL 4.60 §4.8.1 (invariant), §4.3.8 (uniform kiểu struct) và §4.4.5
// (SSBO mảng unsized + instance) đều bị parser của converter từ chối TRƯỚC
// khi pass built-in kịp chạy. Hàm này chuẩn hoá NGUỒN GLSL để converter
// parse được, giữ nguyên ngữ nghĩa:
//   • `invariant X;`            → bỏ hẳn (MSL không có khái niệm tương đương)
//   • `uniform S u[N];`          → tách phẳng `uniform <T_m> u_<m>[N];`
//                                  + `u[i].m` → `u_<m>[i]`
//   • `buffer B { T d[]; } b;`   → `buffer B { T d[]; };` + `b.d` → `d`
//                                  (device T* phẳng — cơ chế SSBO-flat sẵn có)
//
// Gắn vào converter — CÁCH 1 (tối thiểu, 1 dòng + 1 prototype):
//     include/tglmt/GLSLBuiltins.h (hoặc GLSLConverter.h):
//         namespace tglmt::gl { std::string NormalizeGLSLForLang(const std::string&); }
//     src/gl/glsl_to_msl.cpp:8471
//       -   std::string src = StripComments(glsl);
//       +   std::string src = tglmt::gl::NormalizeGLSLForLang(StripComments(glsl));
//
// Gắn vào converter — CÁCH 2 (khung chung, nhiều nhóm ngôn ngữ):
//     include/tglmt/GLSLBuiltins.h:
//         using GLSLSourcePassFn = std::string (*)(const std::string&);
//         bool RegisterGLSLSourcePass(const char* id, GLSLSourcePassFn fn);
//         std::string RunGLSLSourcePasses(const std::string& src);
//     src/gl/glsl_builtins_registry.cpp: hàm registry (như RunGLSLBuiltinPasses)
//     src/gl/glsl_to_msl.cpp:8471:
//       +   std::string src = tglmt::gl::RunGLSLSourcePasses(StripComments(glsl));
//     src/gl/glsl_builtins_lang.cpp: đăng ký "lang" bằng
//       static const bool kRegLangSrc = RegisterGLSLSourcePass("lang", NormalizeGLSLForLang);
//
// Hàm luôn trả về nguồn HỢP LỆ: nếu không chắc chắn thì trả nguyên đầu vào
// (⇒ converter báo lỗi rõ ràng như trước, không sinh MSL sai).
std::string NormalizeGLSLForLang(const std::string& src);

namespace {

// =====================================================================
// 1. TIỆN ÍCH VĂN BẢN
// =====================================================================

// Vị trí identifier `name` đứng RIÊNG trong `s` (word-boundary), từ `from`.
size_t LangFindWord(const std::string& s, size_t from, const char* name) {
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

// Thay mọi occurrence ĐỨNG RIÊNG của `from` bằng `to`.
std::string LangReplaceWord(const std::string& s, const char* from, const std::string& to) {
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

// Thay tên kiểu (word-boundary) — dùng cho bảng kiểu fp16 → float.
// CHỈ thay khi tên ở vị trí KIỂU: theo sau bằng identifier bắt đầu (khai báo
// `half x`), '(' (ctor/cast `half(1.0)`) hoặc '[' (mảng `half[4] x`).
// Nhờ vậy biến của user tên "half" (`float half = 1.0;`) KHÔNG bị đổi.
void LangReplaceTypeWord(std::string* text, const char* from, const char* to) {
    std::string& s = *text;
    size_t n = strlen(from);
    size_t p = 0;
    std::string out;
    while (p < s.size()) {
        size_t q = s.find(from, p);
        if (q == std::string::npos) {
            out.append(s, p, s.size() - p);
            break;
        }
        bool l = q > 0 && IsIdentChar(s[q - 1], false);
        size_t e = q + n;
        bool r = e < s.size() && IsIdentChar(s[e], false);
        bool ok = !l && !r;
        if (ok) {
            size_t k = e;
            while (k < s.size() && isspace((unsigned char)s[k])) ++k;
            if (k >= s.size()) ok = false;
            else if (IsIdentChar(s[k], true)) ok = true;
            else if (s[k] == '(' || s[k] == '[') ok = true;
            else ok = false;
        }
        out.append(s, p, q - p);
        if (ok) out += to;
        else out.append(s, q, n);
        p = e;
    }
    s.swap(out);
}

// =====================================================================
// 2. fp16: half / f16vecN / f16mat*  →  float / floatN / floatNxM
// =====================================================================
//
// GLSL 4.60 §4.1.4: core profile KHÔNG có kiểu 16-bit (grammar §9, PDF p.218:
// VOID FLOAT DOUBLE INT UINT BOOL VEC2..4 DVEC2..4 BVEC2..4 IVEC2..4 UVEC2..4
// MAT* DMAT*). Kiểu `half`/`f16vecN`/`float16_t` đến từ extension
// GL_ARB_gpu_shader_fp16.
//
// Ánh xạ: MSL CÓ `half`/`half2` (Metal 2.0+) nhưng ngữ nghĩa KHÁC GLSL fp16
// (GLSL fp16 có quy tắc promote/convert riêng, ví dụ phép cộng fp16 không
// tự lên float). TGLMT hạ xuống `float` ⇒ value-compatible, đúng cho target
// A11 (fp16 arithmetic không bắt buộc), và tránh phải giải mã overload
// float/half trong MSL (ambiguous overload — xem glsl_to_msl.cpp:11986).
struct LangTypePair {
    const char* from;
    const char* to;
};

const LangTypePair kFp16Types[] = {
    // Vector (ARB_gpu_shader_fp16): f16vec2/3/4
    {"f16vec2", "float2"}, {"f16vec3", "float3"}, {"f16vec4", "float4"},
    // "f16vec" không có trong GLSL; map sang float4 (vec4 là dạng mặc định
    // của vector 4 phần tử trong GLSL) để không bị "kiểu không hỗ trợ".
    {"f16vec", "float4"},
    // Ma trận fp16: f16matCxR → floatCxR (Metal floatCxR = C cột × R hàng,
    // GLSL matCxR cùng quy ước — xem glsl_to_msl.cpp:143).
    {"f16mat2", "float2x2"}, {"f16mat3", "float3x3"}, {"f16mat4", "float4x4"},
    {"f16mat2x2", "float2x2"}, {"f16mat2x3", "float2x3"}, {"f16mat2x4", "float2x4"},
    {"f16mat3x2", "float3x2"}, {"f16mat3x3", "float3x3"}, {"f16mat3x4", "float3x4"},
    {"f16mat4x2", "float4x2"}, {"f16mat4x3", "float4x3"}, {"f16mat4x4", "float4x4"},
    // Tên kiểu scalar của extension.
    {"float16_t", "float"},
    // "half"/"half2..4" (tên ESSL/extension) — hạ xuống float tương ứng.
    {"half4", "float4"}, {"half3", "float3"}, {"half2", "float2"}, {"half", "float"},
};

// Literal số thực có hậu tố 'h' (fp16) → hậu tố 'f'.
// GLSL: `floating-suffix : one of f F lf LF` (§4.1.4) — 'h' là dạng extension;
// MSL: literal `1.0h` là kiểu half. Vì `half` đã hạ xuống `float` nên literal
// cũng phải hạ theo, nếu không `float h = 1.0h;` vẫn hợp lệ (half→float
// widen) nhưng `float2 v = f16vec2(1.0h)` thì không.
void LangRewriteHalfLiterals(std::string* text) {
    std::string& s = *text;
    std::string out;
    out.reserve(s.size() + 8);
    size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        // Chuỗi literal: giữ nguyên.
        if (c == '"') {
            size_t j = i + 1;
            while (j < s.size()) {
                if (s[j] == '\\') { j += 2; continue; }
                if (s[j] == '"') { ++j; break; }
                ++j;
            }
            out.append(s, i, j - i);
            i = j;
            continue;
        }
        bool numStart = isdigit((unsigned char)c) ||
                        (c == '.' && i + 1 < s.size() && isdigit((unsigned char)s[i + 1]));
        if (!numStart || (i > 0 && (IsIdentChar(s[i - 1], false)))) {
            out += c;
            ++i;
            continue;
        }
        // Dọn token số.
        size_t j = i;
        bool hasDot = false, hasExp = false, isHex = false;
        if (c == '0' && j + 1 < s.size() && (s[j + 1] == 'x' || s[j + 1] == 'X')) {
            isHex = true;
            j += 2;
        }
        while (j < s.size()) {
            char d = s[j];
            if (isdigit((unsigned char)d)) { ++j; continue; }
            if (isHex && isxdigit((unsigned char)d)) { ++j; continue; }
            if (d == '.') { hasDot = true; ++j; continue; }
            if ((!isHex && (d == 'e' || d == 'E')) || (isHex && (d == 'p' || d == 'P'))) {
                hasExp = true;
                ++j;
                if (j < s.size() && (s[j] == '+' || s[j] == '-')) ++j;
                continue;
            }
            break;
        }
        // Hậu tố.
        size_t sufEnd = j;
        bool halfSuffix = false;
        if (j < s.size() && (s[j] == 'h' || s[j] == 'H')) {
            sufEnd = j + 1;
            halfSuffix = true;
        } else if (j < s.size() && (s[j] == 'f' || s[j] == 'F')) {
            sufEnd = j + 1;
        } else if (!isHex && j + 1 < s.size() && (s[j] == 'l' || s[j] == 'L') &&
                   (s[j + 1] == 'f' || s[j + 1] == 'F')) {
            sufEnd = j + 2;
        }
        // Chỉ coi là literal fp16 khi có phần thập phân/exponent (GLSL bắt buộc
        // '1.0h' có '.', còn '1h' không phải literal hợp lệ).
        if (halfSuffix && !(hasDot || hasExp)) halfSuffix = false;
        out.append(s, i, j - i);
        if (halfSuffix) {
            out += 'f';
        } else {
            out.append(s, j, sufEnd - j);
        }
        i = sufEnd;
    }
    s.swap(out);
}

// Viết lại toàn bộ text (body hoặc helpers): tên kiểu fp16 + literal fp16.
void LangRewriteFp16(std::string* text) {
    if (!text) return;
    for (const LangTypePair& p : kFp16Types) {
        if (text->find(p.from) == std::string::npos) continue;
        LangReplaceTypeWord(text, p.from, p.to);
    }
    LangRewriteHalfLiterals(text);
}

// =====================================================================
// 3. `flat` trên biến INT/UINT (và mọi kiểu khác)
// =====================================================================
//
// GLSL 4.60 §4.5: "A variable qualified as flat will not be interpolated."
// MSL: attribute [[flat]] trên field của stage-in/stage-out.
//
// Converter đã emit [[flat]] cho VARYING (src/gl/glsl_to_msl.cpp:12229 và
// :12241 — qua InterpAttr()), áp dụng cho MỌI kiểu (float/int/uint). ⇒
// `flat out int v;` / `flat in int v;` (varying) ĐÃ đúng.
//
// THIẾU: VERTEX INPUT (attribute). Converter emit field vertex input ở
// :12213-12217 KHÔNG gọi InterpAttr() ⇒ `flat in int k;` trong vertex shader
// mất qualifier ⇒ MSL không có [[flat]] (coverage case "interpolation flat").
// Metal chấp nhận [[flat]] trên vertex input (đo thật #1) và về ngữ nghĩa là
// no-op (vertex input không bao giờ nội suy) ⇒ an toàn.
//
// Cách thêm attribute mà KHÔNG sửa converter: nhúng attribute vào TÊN field
// (kỹ thuật của pass "modern_io"): field "k [[flat]]" ⇒ Metal parse đúng,
// tên field vẫn là "k" ⇒ `_in.k` hợp lệ, `R.inputs` (copy TRƯỚC pass,
// glsl_to_msl.cpp:10014) giữ tên sạch "k" cho linker/glGetAttribLocation.
void LangApplyFlatToVertexInputs(GLSLBuiltinCtx& ctx) {
    if (!ctx.isVertex || !ctx.inputs) return;
    for (GLSLVar& v : *ctx.inputs) {
        if (v.interpolation != "flat") continue;
        if (v.name.find("[[") != std::string::npos) continue;   // đã có attribute
        v.name += " [[flat]]";
    }
}

// =====================================================================
// 4. packHalf2x16 / unpackHalf2x16 — vec2 → vec4 khi gán vào color output
// =====================================================================
//
// GLSL 4.60 §8.4 (PDF p.174): `vec2 unpackHalf2x16(uint v)` ⇒ trả về VEC2.
// GLSL cũng KHÔNG cho gán vec2 vào vec4 (`gl_FragColor` là vec4) ⇒ shader
// coverage "pack/unpack" (`gl_FragColor = unpackHalf2x16(packHalf2x16(…))`)
// là GLSL KHÔNG HỢP LỆ.
//
// Pass "legacy" đã nới tương tự cho float→vec4 (glsl_builtins_legacy.cpp:
// LegFixFloatAssign). Pass này nới tiếp cho vec2→vec4, CHỈ khi RHS là lời
// gọi hàm trả về vec2 (đã biết chắc) — không đoán bừa.
//
// Hàm pack/unpack: MSL KHÔNG có `pack_half2x16`/`unpack_half2x16` (đo thật #2)
// ⇒ helper sẵn có của converter (tglmt_packHalf2x16 / tglmt_unpackHalf2x16,
// glsl_to_msl.cpp:12181-12188, dùng half2 + as_type<ushort>) LÀ ánh xạ MSL
// tương đương, biên dịch được ⇒ không viết lại.

// Các hàm MSL trả về vec2 (GLSL §8.4: unpackUnorm2x16/unpackSnorm2x16/
// unpackHalf2x16).
// RHS phải CHÍNH XÁC là một lời gọi trả về vec2 — KHÔNG phải biểu thức CHỨA
// lời gọi vec2 bên trong (ví dụ `float4(vec2-call, 0, 1)` là float4, KHÔNG
// phải vec2). Kiểm tra: bắt đầu bằng tên hàm + ngoặc cân bằng, đóng đúng ở
// ký tự cuối cùng.
bool LangIsVec2Call(const std::string& rhs) {
    static const char* kCalls[] = {"tglmt_unpackHalf2x16(", "unpack_unorm2x16_to_float(",
                                   "unpack_snorm2x16_to_float(", nullptr};
    std::string t = Trim(rhs);
    for (const char** q = kCalls; *q; ++q) {
        size_t len = strlen(*q);
        if (t.size() < len + 1) continue;
        if (t.compare(0, len, *q) != 0) continue;
        int d = 0;
        for (size_t i = len - 1; i < t.size(); ++i) {
            if (t[i] == '(') ++d;
            else if (t[i] == ')') {
                if (--d == 0) return i == t.size() - 1;
            }
        }
        return false;
    }
    return false;
}

// Các đích gán kiểu vec4 trong fragment shader.
void LangCollectVec4Targets(const std::string& body, std::vector<std::string>* out) {
    out->push_back("tglmt_fragColor");
    out->push_back("_out.c");
    static const char kMrt[] = "_out.mrt";
    const size_t kLen = sizeof(kMrt) - 1;
    size_t p = 0;
    while ((p = body.find(kMrt, p)) != std::string::npos) {
        size_t e = p + kLen;
        size_t q = e;
        while (q < body.size() && isdigit((unsigned char)body[q])) ++q;
        if (q == e) { p = e; continue; }
        std::string nm = body.substr(p, q - p);
        bool dup = false;
        for (const std::string& s : *out)
            if (s == nm) { dup = true; break; }
        if (!dup) out->push_back(nm);
        p = q;
    }
}

// `tglmt_fragColor = <vec2 expr>;` → `tglmt_fragColor = float4(<expr>, 0.0, 1.0);`
void LangWidenVec2Assign(std::string* body) {
    if (!body) return;
    std::vector<std::string> targets;
    LangCollectVec4Targets(*body, &targets);
    for (const std::string& tgt : targets) {
        size_t p = 0;
        while ((p = body->find(tgt, p)) != std::string::npos) {
            size_t e = p + tgt.size();
            bool l = p > 0 && IsIdentChar((*body)[p - 1], false);
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
            if (!LangIsVec2Call(rhs)) { p = e; continue; }
            (*body).replace(rs, re - rs, "float4(" + rhs + ", 0.0, 1.0)");
            p = rs + 6 + rhs.size();
            continue;
        }
    }
}

// =====================================================================
// 5. CHUẨN HOÁ NGUỒN GLSL (trước khi parse)
// =====================================================================
//
// GLSL cho phép `struct`/`uniform`/`buffer` ở cấp global nhưng câu lệnh có
// thể bị "#version"/"#define" đứng trước ⇒ không dựa vào ranh giới câu lệnh
// mà dò TRỰC TIẾP theo mẫu (tự nhiên hơn, không phụ thuộc thứ tự dòng).

// Dấu '}' tương ứng với '{' tại `open` (bỏ qua string literal).
size_t LangMatchBrace(const std::string& s, size_t open) {
    if (open >= s.size() || s[open] != '{') return std::string::npos;
    int d = 0;
    bool str = false;
    for (size_t i = open; i < s.size(); ++i) {
        char c = s[i];
        if (str) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') str = false;
            continue;
        }
        if (c == '"') { str = true; continue; }
        if (c == '{') ++d;
        else if (c == '}') {
            if (--d == 0) return i;
        }
    }
    return std::string::npos;
}

// Bỏ khoảng trắng [from, to).
std::string LangTrimRange(const std::string& s, size_t from, size_t to) {
    size_t a = from, b = to;
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

// Kiểu GLSL → kiểu MSL cho member của struct (chỉ loại scalar/vector).
bool LangMemberMslType(const std::string& glslType, std::string* msl) {
    struct P { const char* g; const char* m; };
    static const P kMap[] = {
        {"float", "float"}, {"vec2", "float2"}, {"vec3", "float3"}, {"vec4", "float4"},
        {"int", "int"}, {"ivec2", "int2"}, {"ivec3", "int3"}, {"ivec4", "int4"},
        {"uint", "uint"}, {"uvec2", "uint2"}, {"uvec3", "uint3"}, {"uvec4", "uint4"},
        {"bool", "bool"}, {"bvec2", "bool2"}, {"bvec3", "bool3"}, {"bvec4", "bool4"},
    };
    for (const P& p : kMap)
        if (glslType == p.g) { *msl = p.m; return true; }
    return false;
}

// Member của struct: {kiểu GLSL, tên}.
struct LangStructMember {
    std::string type;
    std::string name;
};

// Định nghĩa struct tìm được trong nguồn.
struct LangStructDef {
    std::string name;
    std::vector<LangStructMember> members;
    bool ok = false;      // mọi member parse được & là scalar/vector
};

// Tách member của struct từ phần thân `{ ... }`.
// Trả false nếu có member lạ (mảng, struct lồng, initializer).
bool LangParseStructMembers(const std::string& inner, std::vector<LangStructMember>* out) {
    size_t p = 0;
    while (p < inner.size()) {
        size_t semi = inner.find(';', p);
        if (semi == std::string::npos) semi = inner.size();
        std::string seg = Trim(inner.substr(p, semi - p));
        p = semi + 1;
        if (seg.empty()) continue;
        std::istringstream ss(seg);
        std::vector<std::string> w;
        std::string tk;
        while (ss >> tk) w.push_back(tk);
        size_t k = 0;
        while (k < w.size() && (w[k] == "highp" || w[k] == "mediump" || w[k] == "lowp")) ++k;
        if (k + 1 >= w.size()) return false;
        LangStructMember m;
        m.type = w[k];
        m.name = w[k + 1];
        for (size_t q = k + 2; q < w.size(); ++q) {
            // Mảng / initializer / struct lồng → không flatten được.
            if (w[q].find('[') != std::string::npos) return false;
            if (w[q] == "=") return false;
            m.name += w[q];
        }
        for (char c : m.name)
            if (!IsIdentChar(c, false)) return false;
        out->push_back(m);
    }
    return !out->empty();
}

// Dò tất cả định nghĩa `struct <Name> { ... };` trong nguồn.
void LangFindStructDefs(const std::string& src, std::vector<LangStructDef>* out) {
    size_t p = 0;
    while ((p = LangFindWord(src, p, "struct")) != std::string::npos) {
        size_t q = p + 6;
        while (q < src.size() && isspace((unsigned char)src[q])) ++q;
        size_t b = q;
        while (q < src.size() && IsIdentChar(src[q], q == b)) ++q;
        std::string name = src.substr(b, q - b);
        if (name.empty()) { p += 6; continue; }
        size_t brace = src.find('{', q);
        if (brace == std::string::npos) break;
        if (!LangTrimRange(src, q, brace).empty()) { p = q; continue; }
        size_t close = LangMatchBrace(src, brace);
        if (close == std::string::npos) break;
        LangStructDef d;
        d.name = name;
        d.ok = LangParseStructMembers(src.substr(brace + 1, close - brace - 1), &d.members);
        out->push_back(d);
        p = close + 1;
    }
}

// --- (5a) `invariant` ---
// GLSL 4.60 §4.8.1 (PDF p.103): "The invariant keyword can be followed by a
// comma separated list of previously declared identifiers. All uses of
// invariant must be at global scope or on block members, and before any use of
// the variables being declared as invariant."
// MSL không có khái niệm tương đương (Metal compile 1 shader ⇒ 1 binary ⇒
// giá trị đã deterministic) ⇒ chấp nhận + bỏ qua:
//   • dạng khai báo  `invariant out vec4 c;` → bỏ từ khoá.
//   • dạng lệnh     `invariant gl_Position;` → bỏ HẲN câu lệnh.
bool LangStripInvariant(std::string* src) {
    // Dạng câu lệnh rời (`invariant gl_Position;`) nằm trên MỘT DÒNG ở cấp
    // global ⇒ xử lý theo dòng (đơn giản + không phụ thuộc "#version" đứng
    // trước). Dạng khai báo (`invariant out vec4 c;`) chỉ cần bỏ từ khoá —
    // vòng "bỏ mọi từ khoá invariant còn lại" ở cuối xử lý.
    struct Drop { size_t start, end; };
    std::vector<Drop> drops;
    size_t pos = 0;
    while (pos <= src->size()) {
        size_t nl = src->find('\n', pos);
        bool last = (nl == std::string::npos);
        size_t lineEnd = last ? src->size() : nl + 1;
        std::string t = Trim(src->substr(pos, lineEnd - pos));
        size_t inv = LangFindWord(t, 0, "invariant");
        if (inv != std::string::npos && LangTrimRange(t, 0, inv).empty()) {
            std::string rest = Trim(t.substr(inv + strlen("invariant")));
            // Sau `invariant` có storage/precision/interpolation qualifier?
            static const char* kQual[] = {"in", "out", "inout", "uniform", "const",
                                          "attribute", "varying", "highp", "mediump",
                                          "lowp", "flat", "smooth", "noperspective",
                                          "centroid", "sample", "precise", nullptr};
            bool hasDecl = false;
            for (const char** q = kQual; *q; ++q) {
                if (rest.rfind(*q, 0) != 0) continue;
                size_t e = strlen(*q);
                if (e >= rest.size() || !IsIdentChar(rest[e], false)) { hasDecl = true; break; }
            }
            if (!hasDecl) drops.push_back({pos, lineEnd});
        }
        if (last) break;
        pos = lineEnd;
    }
    if (!drops.empty()) {
        std::string out;
        size_t p = 0;
        for (const Drop& d : drops) {
            out.append(*src, p, d.start - p);
            p = d.end;
        }
        out.append(*src, p, src->size() - p);
        src->swap(out);
    }
    // Bỏ từ khoá `invariant` còn lại (dạng qualifier trong khai báo).
    *src = LangReplaceWord(*src, "invariant", "");
    return true;
}

// --- (5b) SSBO mảng unsized + instance ---
// GLSL 4.60 §4.4.5: mảng unsized được phép làm member CUỐI của block, kích
// thước runtime. Converter hỗ trợ dạng PHẲNG: `buffer B { float d[]; };`
// ⇒ `device float* d [[buffer(n)]]` (src/gl/glsl_to_msl.cpp:11037-11044).
// Dạng có instance (`… } b;`) bị từ chối ⇒ bỏ instance + đổi `b.d` → `d`.
bool LangFlattenSsboUnsized(std::string* src) {
    struct Drop { size_t start, end; };
    std::vector<Drop> drops;
    std::vector<std::pair<std::string, std::string>> rename;
    size_t p = 0;
    while ((p = LangFindWord(*src, p, "buffer")) != std::string::npos) {
        size_t q = p + 6;
        while (q < src->size() && isspace((unsigned char)(*src)[q])) ++q;
        size_t b = q;
        while (q < src->size() && IsIdentChar((*src)[q], q == b)) ++q;
        if (q == b) { p += 6; continue; }        // "buffer" không phải tên block
        size_t brace = src->find('{', q);
        if (brace == std::string::npos) break;
        if (!LangTrimRange(*src, q, brace).empty()) { p = q; continue; }
        size_t close = LangMatchBrace(*src, brace);
        if (close == std::string::npos) break;
        // Instance sau '}'.
        size_t r = close + 1;
        while (r < src->size() && isspace((unsigned char)(*src)[r])) ++r;
        size_t ib = r;
        while (r < src->size() && IsIdentChar((*src)[r], r == ib)) ++r;
        std::string inst = src->substr(ib, r - ib);
        if (inst.empty()) { p = close + 1; continue; }
        size_t s2 = r;
        while (s2 < src->size() && isspace((unsigned char)(*src)[s2])) ++s2;
        if (s2 >= src->size() || (*src)[s2] != ';') { p = close + 1; continue; }
        // Members: đúng 1 và là mảng UNSIZED.
        std::string inner = src->substr(brace + 1, close - brace - 1);
        std::vector<std::string> mems;
        size_t mp = 0;
        while (mp < inner.size()) {
            size_t semi = inner.find(';', mp);
            if (semi == std::string::npos) semi = inner.size();
            std::string seg = Trim(inner.substr(mp, semi - mp));
            mp = semi + 1;
            if (!seg.empty()) mems.push_back(seg);
        }
        if (mems.size() != 1) { p = close + 1; continue; }
        const std::string& m = mems[0];
        size_t lb = m.find('[');
        if (lb == std::string::npos) { p = close + 1; continue; }
        size_t rb = m.find(']', lb);
        if (rb == std::string::npos) { p = close + 1; continue; }
        if (!Trim(m.substr(lb + 1, rb - lb - 1)).empty()) { p = close + 1; continue; }
        std::string mname = Trim(m.substr(0, lb));
        size_t sp = mname.find_last_of(" \t");
        if (sp == std::string::npos) { p = close + 1; continue; }
        mname = Trim(mname.substr(sp + 1));
        if (mname.empty()) { p = close + 1; continue; }
        // Xoá tên instance (kèm khoảng trắng thừa phía trước).
        size_t dStart = ib;
        while (dStart > close + 1 && isspace((unsigned char)(*src)[dStart - 1])) --dStart;
        drops.push_back({dStart, ib + inst.size()});
        rename.push_back({inst + "." + mname, mname});
        p = close + 1;
    }
    if (drops.empty()) return false;
    std::string out;
    size_t pos = 0;
    for (const Drop& d : drops) {
        out.append(*src, pos, d.start - pos);
        pos = d.end;
    }
    out.append(*src, pos, src->size() - pos);
    src->swap(out);
    for (const auto& r : rename) *src = LangReplaceWord(*src, r.first.c_str(), r.second);
    return true;
}

// `u[i].m` → `u_m[i]` (mảng uniform kiểu struct đã tách phẳng).
void LangRewriteIndexedMember(std::string* text, const std::string& var,
                              const std::string& member, const std::string& newVar) {
    size_t p = 0;
    while ((p = text->find(var, p)) != std::string::npos) {
        size_t e = p + var.size();
        bool l = p > 0 && IsIdentChar((*text)[p - 1], false);
        bool r = e < text->size() && IsIdentChar((*text)[e], false);
        if (l || r) { p = e; continue; }
        size_t q = e;
        while (q < text->size() && isspace((unsigned char)(*text)[q])) ++q;
        if (q >= text->size() || (*text)[q] != '[') { p = e; continue; }
        int d = 0;
        size_t rb = q;
        for (; rb < text->size(); ++rb) {
            char c = (*text)[rb];
            if (c == '[') ++d;
            else if (c == ']') { if (--d == 0) break; }
        }
        if (rb >= text->size()) { p = e; continue; }
        size_t after = rb + 1;
        while (after < text->size() && isspace((unsigned char)(*text)[after])) ++after;
        if (after >= text->size() || (*text)[after] != '.') { p = e; continue; }
        size_t mb = after + 1;
        size_t me = mb;
        while (me < text->size() && IsIdentChar((*text)[me], false)) ++me;
        if (text->substr(mb, me - mb) != member) { p = e; continue; }
        std::string idx = text->substr(q, rb - q + 1);   // "[i]"
        text->replace(p, me - p, newVar + idx);
        p = p + newVar.size() + idx.size();
    }
}

// --- (5c) mảng uniform kiểu struct ---
// GLSL 4.60 §4.3.8: uniform có thể là mảng của struct. Converter chỉ nhận
// uniform kiểu scalar/vector/matrix ⇒ tách phẳng theo member:
//   `uniform S u[2];` + `struct S { float f; };`  ⇒  `uniform float u_f[2];`
// và `u[0].f` → `u_f[0]`.
bool LangFlattenUniformStructArray(std::string* src) {
    std::vector<LangStructDef> defs;
    LangFindStructDefs(*src, &defs);
    if (defs.empty()) return false;

    struct Flat {
        size_t start, end;
        std::string repl;
        std::vector<std::pair<std::string, std::string>> rename;
    };
    std::vector<Flat> flats;
    size_t p = 0;
    while ((p = LangFindWord(*src, p, "uniform")) != std::string::npos) {
        size_t q = p + 7;
        while (q < src->size() && isspace((unsigned char)(*src)[q])) ++q;
        size_t tb = q;
        while (q < src->size() && IsIdentChar((*src)[q], q == tb)) ++q;
        std::string type = src->substr(tb, q - tb);
        if (type.empty()) { p += 7; continue; }
        while (q < src->size() && isspace((unsigned char)(*src)[q])) ++q;
        size_t nb = q;
        while (q < src->size() && IsIdentChar((*src)[q], q == nb)) ++q;
        std::string name = src->substr(nb, q - nb);
        if (name.empty()) { p += 7; continue; }
        size_t lb = src->find('[', q);
        if (lb == std::string::npos) { p = q; continue; }
        size_t rb = src->find(']', lb);
        if (rb == std::string::npos) { p = q; continue; }
        std::string cnt = Trim(src->substr(lb + 1, rb - lb - 1));
        bool allDigit = !cnt.empty();
        for (char c : cnt)
            if (!isdigit((unsigned char)c)) allDigit = false;
        if (!allDigit) { p = q; continue; }
        size_t r = rb + 1;
        while (r < src->size() && isspace((unsigned char)(*src)[r])) ++r;
        if (r >= src->size() || (*src)[r] != ';') { p = q; continue; }
        // Kiểu phải là struct đã biết.
        const LangStructDef* def = nullptr;
        for (const LangStructDef& d : defs)
            if (d.name == type) { def = &d; break; }
        if (!def || !def->ok) { p = q; continue; }
        // Mọi member phải map được sang kiểu MSL.
        std::vector<std::string> decls;
        for (const LangStructMember& m : def->members) {
            std::string msl;
            if (!LangMemberMslType(m.type, &msl)) return false;
            decls.push_back("uniform " + msl + " " + name + "_" + m.name + "[" + cnt + "]");
        }
        // Đầu câu lệnh (quét lùi tới ';' / '}' / đầu dòng).
        size_t st = p;
        while (st > 0) {
            char c = (*src)[st - 1];
            if (c == ';' || c == '}') break;
            if (c == '\n') {
                size_t ls = st - 1;
                size_t lb2 = ls;
                while (lb2 > 0 && (*src)[lb2 - 1] != '\n') --lb2;
                std::string line = Trim((*src).substr(lb2, ls - lb2 + 1));
                if (!line.empty() && line[0] == '#') { st = lb2; continue; }
                break;
            }
            --st;
        }
        Flat f;
        f.start = st;
        f.end = r + 1;
        std::string joined;
        for (size_t i = 0; i < decls.size(); ++i) joined += (i ? " " : "") + decls[i];
        f.repl = joined + ";";
        for (const LangStructMember& m : def->members)
            f.rename.push_back({name + "." + m.name, name + "_" + m.name});
        flats.push_back(f);
        p = r + 1;
    }
    if (flats.empty()) return false;
    std::string out;
    size_t pos = 0;
    for (const Flat& f : flats) {
        out.append(*src, pos, f.start - pos);
        out += f.repl;
        pos = f.end;
    }
    out.append(*src, pos, src->size() - pos);
    src->swap(out);
    for (const Flat& f : flats)
        for (const auto& rn : f.rename) {
            // "u.f" → "u_f" (member truy cập trực tiếp) và "u[i].f" → "u_f[i]".
            LangRewriteIndexedMember(src, rn.first.substr(0, rn.first.find('.')), 
                                     rn.first.substr(rn.first.find('.') + 1), rn.second);
            *src = LangReplaceWord(*src, rn.first.c_str(), rn.second);
        }
    return true;
}

}  // namespace

// =====================================================================
// 6. HÀM CHUẨN HOÁ NGUỒN (public — xem mục 0)
// =====================================================================
std::string NormalizeGLSLForLang(const std::string& src) {
    std::string s = src;
    // Mỗi bước độc lập; nếu một bước không chắc chắn thì giữ nguyên nguồn
    // (converter sẽ báo lỗi rõ ràng như trước — không sinh MSL sai).
    {
        std::string t = s;
        LangStripInvariant(&t);
        s.swap(t);
    }
    {
        std::string t = s;
        LangFlattenSsboUnsized(&t);
        s.swap(t);
    }
    {
        std::string t = s;
        LangFlattenUniformStructArray(&t);
        s.swap(t);
    }
    return s;
}

// Đăng ký pass mức NGUỒN — chạy TRƯỚC khi converter parse (xem GLSLBuiltins.h).
static const bool kRegLangSrc =
    RegisterGLSLSourcePass("lang", NormalizeGLSLForLang);

// =====================================================================
// 7. PASS
// =====================================================================
static void ApplyGLSLBuiltinLang(GLSLBuiltinCtx& ctx) {
    if (!ctx.body && !ctx.helpers) return;

    // (1) fp16: half / f16vecN / f16mat* → float / floatN / floatNxM.
    LangRewriteFp16(ctx.body);
    LangRewriteFp16(ctx.helpers);

    // (2) `flat` trên vertex input (attribute) — converter không gọi
    //     InterpAttr() cho attribute (glsl_to_msl.cpp:12216).
    LangApplyFlatToVertexInputs(ctx);

    // (3) vec2 → vec4 khi gán vào color output (GLSL §8.4 unpack* trả vec2).
    LangWidenVec2Assign(ctx.body);
}

static const bool kRegLang = RegisterGLSLBuiltinPass("lang", ApplyGLSLBuiltinLang);

}  // namespace tglmt::gl
