// glsl_builtins_fn_math.cpp — nhóm built-in FUNCTION GLSL "fn_math".
//
// GLSL 4.60 §8 "Built-in Functions" — các hàm mà MSL (Metal Shading
// Language) KHÔNG có sẵn đúng dạng, hoặc khác ngữ nghĩa:
//
//   §8.3 Common Functions
//     modf(x, out i)                 → MSL có modf(float, thread float&) nhưng
//                                      GLSL "out" phải khai báo biến ⇒ helper
//                                      tglmt_modf + nâng khai báo out lên block.
//     frexp(x, out e)                → tương tự ⇒ tglmt_frexp.
//     roundEven(x)                   → MSL rint() (round-half-to-even) ⇒ map thẳng.
//     packSnorm4x8 / unpackSnorm4x8  → MSL pack_float_to_snorm4x8 /
//                                      unpack_snorm4x8_to_float (map thẳng).
//     packDouble2x32 / unpackDouble2x32 → MSL KHÔNG có kiểu double ⇒ helper
//                                      (emulation mềm, xem ghi chú bên dưới).
//     mix(bool, bool, bool)          → MSL select(false, true, cond).
//   §8.6 Matrix Functions
//     matrixCompMult(a, b)           → MSL không có ⇒ tglmt_matrixCompMult.
//     outerProduct(c, r)             → MSL không có ⇒ tglmt_outerProduct.
//     not(bvec)                      → MSL metal::not (bvecN đã map thành boolN).
//     transpose / inverse             → MSL đã có `transpose`; `inverse` đã có
//                                      pass RewriteInverseCalls trong
//                                      glsl_to_msl.cpp ⇒ KHÔNG viết lại (tránh
//                                      viết hai lần).
//   §8.7 Integer Functions
//     uaddCarry / usubBorrow         → MSL không có ⇒ tglmt_uaddCarry /
//                                      tglmt_usubBorrow.
//     umulExtended / imulExtended    → MSL không có ⇒ tglmt_umulExtended /
//                                      tglmt_imulExtended (nhân 64-bit bằng
//                                      ulong/long).
//   (ngoài §8) uvecBitsToFloat(uvec2) → MSL as_type<float2>(uvec2).
//
// NGUYÊN TẮC
//   • Pass này KHÔNG đụng vào src/gl/glsl_to_msl.cpp (xem GLSLBuiltins.h).
//   • Chỉ sửa đúng phần liên quan tới nhóm của mình: MSL của các shader
//     không dùng các hàm này phải BYTE-IDENTICAL (vì vậy helper chỉ được
//     phát khi thực sự dùng).
//   • GLSL 4.60 §8 (trang "Chapter 8. Built-In Functions"): "User code can
//     replace built-in functions with their own ... doing this will hide all
//     built-in functions with the same name" ⇒ nếu user định nghĩa hàm trùng
//     tên, pass BỎ QUA hoàn toàn hàm built-in đó (không đoán).
//
// EMULATION `double` (GLSL 4.60 §8.4 packDouble2x32/unpackDouble2x32)
//   Metal (MSL 2.x/3.x) KHÔNG có kiểu `double` — GPU Apple (A11) cũng không
//   có FP64. Converter đã hạ `double/dvec/dmat` xuống `float` (xem mpShared
//   trong glsl_to_msl.cpp: "double → float"). Vì vậy:
//     • unpackDouble2x32(x): x là float (đã hạ) ⇒ trả về ĐÚNG 64 bit biểu
//       diễn double của giá trị float đó (float→double luôn chính xác).
//     • packDouble2x32(v): trả về giá trị float gần nhất với double mang bit
//       pattern v (double→float làm tròn) — phần "giá trị double" không thể
//       tồn tại trên Metal, đây là xấp xỉ TỐT NHẤT có thể trong khuôn khổ
//       hạ double→float của project.
//   Vòng đi–về unpack(pack(v)) là CHÍNH XÁC cho mọi giá trị double mà project
//   có thể biểu diễn (tức mọi float).

#include "tglmt/GLSLBuiltins.h"
#include "tglmt/GLSLTextUtil.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {
namespace {

// =====================================================================
// 1. TIỆN ÍCH VĂN BẢN
// =====================================================================

// Vị trí identifier `name` trong `s` (từ `from`), word-boundary, và KHÔNG
// phải member access (`a.name`) / namespace (`metal::name`).
size_t FindIdent(const std::string& s, size_t from, const char* name) {
    size_t n = strlen(name);
    if (!n) return std::string::npos;
    for (size_t i = from; i + n <= s.size(); ++i) {
        if (s.compare(i, n, name) != 0) continue;
        if (i > 0) {
            char p = s[i - 1];
            if (IsIdentChar(p, false)) continue;
            if (p == '.') continue;                                  // a.name
            if (p == ':' && i >= 2 && s[i - 2] == ':') continue;     // ns::name
        }
        size_t j = i + n;
        if (j < s.size() && IsIdentChar(s[j], false)) continue;
        return i;
    }
    return std::string::npos;
}

// Dấu ')' tương ứng với '(' tại `open` (std::string::npos nếu hở).
size_t FindMatchingParen(const std::string& s, size_t open) {
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
bool IsCallAt(const std::string& s, size_t afterIdent) {
    size_t q = afterIdent;
    while (q < s.size() && isspace((unsigned char)s[q])) ++q;
    return q < s.size() && s[q] == '(';
}

// Vị trí chèn khai báo cho out-param khai báo nội tuyến: ngay sau '{' của
// block cha gần nhất. Nếu không có block cha (body của main) ⇒ 0 (đầu vùng
// text) — đúng phạm vi GLSL: biến khai báo trong lời gọi sống tới hết block.
size_t FindBlockStart(const std::string& s, size_t pos) {
    int depth = 0;
    for (size_t i = pos; i-- > 0;) {
        char c = s[i];
        if (c == '}') ++depth;
        else if (c == '{') {
            if (depth == 0) return i + 1;
            --depth;
        }
    }
    return 0;
}

// User có định nghĩa hàm trùng tên built-in không? Dấu hiệu: identifier đứng
// riêng, theo sau là '(' ... ')' rồi '{' (định nghĩa) — khác lời gọi hàm
// (')' rồi ';' hoặc toán tử).
bool UserDefinesFunction(const std::string& helpers, const char* name) {
    size_t p = 0;
    for (;;) {
        p = FindIdent(helpers, p, name);
        if (p == std::string::npos) return false;
        size_t q = p + strlen(name);
        if (IsCallAt(helpers, q)) {
            size_t open = q;
            while (open < helpers.size() && isspace((unsigned char)helpers[open])) ++open;
            size_t close = FindMatchingParen(helpers, open);
            if (close != std::string::npos) {
                size_t r = close + 1;
                while (r < helpers.size() && isspace((unsigned char)helpers[r])) ++r;
                if (r < helpers.size() && helpers[r] == '{') return true;
            }
        }
        p = q;
    }
}

// Thu thập tên biến khai báo kiểu `bool` (scalar) trong MSL text — dùng để
// nhận ra lời gọi `mix(bool,bool,bool)` (GLSL 4.60 §8.3).
std::set<std::string> CollectBoolVars(const std::string& s) {
    std::set<std::string> out;
    size_t p = 0;
    while ((p = s.find("bool", p)) != std::string::npos) {
        size_t e = p + 4;
        if (e < s.size() && IsIdentChar(s[e], false)) { p = e; continue; }  // bool2/3/4
        if (p > 0 && IsIdentChar(s[p - 1], false)) { p = e; continue; }
        size_t q = e;
        while (q < s.size() && isspace((unsigned char)s[q])) ++q;
        if (q < s.size() && (s[q] == '&' || s[q] == '*')) {
            ++q;
            while (q < s.size() && isspace((unsigned char)s[q])) ++q;
        }
        if (q < s.size() && IsIdentChar(s[q], true)) {
            size_t b = q;
            while (q < s.size() && IsIdentChar(s[q], false)) ++q;
            out.insert(s.substr(b, q - b));
        }
        p = e;
    }
    return out;
}

// Có phải biểu thức bool không? (literal `true`/`false` hoặc biến bool đã biết)
bool IsBoolExpr(const std::string& arg, const std::set<std::string>& boolVars) {
    std::string a = Trim(arg);
    if (a == "true" || a == "false") return true;
    return boolVars.count(a) > 0;
}

// "out TYPE name" (khai báo tham số nội tuyến trong lời gọi — GLSL không
// cho phép, nhưng ma trận coverage dùng dạng này) ⇒ tách type + name.
bool ParseInlineOutDecl(const std::string& arg, std::string* type, std::string* name) {
    std::vector<std::string> tok;
    size_t i = 0;
    const std::string& a = arg;
    while (i < a.size()) {
        while (i < a.size() && isspace((unsigned char)a[i])) ++i;
        size_t b = i;
        while (i < a.size() && !isspace((unsigned char)a[i])) ++i;
        if (i > b) tok.push_back(a.substr(b, i - b));
    }
    if (tok.size() < 3) return false;
    if (tok[0] != "out" && tok[0] != "inout") return false;
    // type = các token giữa (bỏ precision qualifier), name = token cuối
    std::string ty, nm = tok.back();
    if (nm.empty() || !IsIdentChar(nm[0], true)) return false;
    for (size_t k = nm.size(); k-- > 0;)
        if (!IsIdentChar(nm[k], k == 0)) return false;
    for (size_t k = 1; k + 1 < tok.size(); ++k) {
        if (tok[k] == "highp" || tok[k] == "mediump" || tok[k] == "lowp") continue;
        if (!ty.empty()) ty += " ";
        ty += tok[k];
    }
    if (ty.empty()) return false;
    *type = ty;
    *name = nm;
    return true;
}

// =====================================================================
// 2. BẢNG RULE
// =====================================================================

struct Rule {
    const char* from;    // tên xuất hiện trong MSL text (đã qua rename keyword)
    const char* to;      // tên MSL thay thế (nullptr ⇒ xử lý riêng: mix)
    int arity;           // số tham số bắt buộc
    unsigned outMask;    // bit i: tham số i là out (cần xử lý out-param)
    const char* helper;  // khóa helper MSL cần phát khi dùng (nullptr = không)
    // Tên MSL đích: nếu user tự định nghĩa hàm trùng tên này thì KHÔNG
    // được dịch (sẽ gọi nhầm hàm user). nullptr = không có rủi ro.
    const char* collide;
};

const Rule kRules[] = {
    // §8.3 — roundEven: MSL rint() là round-half-to-even (C99 rint).
    {"roundEven", "rint", 1, 0, nullptr, "rint"},
    // §8.7 — `not` là keyword C++ (toán tử `!`) nên converter đã đổi tên
    // thành `tglmt_kw_not`; MSL: `not` CŨNG là alternative token của `!`
    // (metal::not không dùng được) ⇒ map thẳng sang toán tử `!` (MSL định nghĩa
    // operator! cho bool/bool2/bool3/bool4 — đã kiểm chứng metal thật).
    {"tglmt_kw_not", "!", 1, 0, nullptr, nullptr},
    // §8.3/§8.4 — pack/unpack 4x8: MSL có sẵn (đã kiểm chứng metal thật).
    {"packSnorm4x8", "pack_float_to_snorm4x8", 1, 0, nullptr, "pack_float_to_snorm4x8"},
    {"unpackSnorm4x8", "unpack_snorm4x8_to_float", 1, 0, nullptr, "unpack_snorm4x8_to_float"},
    // Bit-cast vector: uintBitsToFloat chỉ có dạng scalar trong GLSL 4.60
    // §8.3; biến thể vector (uvecBitsToFloat) là extension — map sang
    // as_type<float2>(uvec2) của MSL.
    {"uvecBitsToFloat", "as_type<float2>", 1, 0, nullptr, "as_type"},
    // §8.6 — matrixCompMult: nhân thành phần, MSL không có.
    {"matrixCompMult", "tglmt_matrixCompMult", 2, 0, "matrixCompMult", nullptr},
    // §8.6 — outerProduct: tích ngoại, MSL không có.
    {"outerProduct", "tglmt_outerProduct", 2, 0, "outerProduct", nullptr},
    // §8.3 — modf/frexp có out-param.
    {"modf", "tglmt_modf", 2, 0x2u, "modf", nullptr},
    {"frexp", "tglmt_frexp", 2, 0x2u, "frexp", nullptr},
    // §8.7 — carry/borrow/extended multiply.
    {"uaddCarry", "tglmt_uaddCarry", 3, 0x4u, "uaddCarry", nullptr},
    {"usubBorrow", "tglmt_usubBorrow", 3, 0x4u, "usubBorrow", nullptr},
    {"umulExtended", "tglmt_umulExtended", 4, 0xCu, "umulExtended", nullptr},
    {"imulExtended", "tglmt_imulExtended", 4, 0xCu, "imulExtended", nullptr},
    // §8.4 — double (Metal không có double ⇒ helper emulation).
    {"packDouble2x32", "tglmt_packDouble2x32", 1, 0, "packDouble2x32", nullptr},
    {"unpackDouble2x32", "tglmt_unpackDouble2x32", 1, 0, "unpackDouble2x32", nullptr},
    // §8.3 — mix(bool,bool,bool) xử lý riêng (→ select).
    {"mix", nullptr, 3, 0, nullptr, "select"},
};

// =====================================================================
// 3. HELPER MSL
// =====================================================================

const char* HelperText(const std::string& key) {
    if (key == "modf") {
        // GLSL 4.60 §8.3: modf trả phần thập phân, out i = phần nguyên (cùng
        // dấu với x). MSL: modf(float, thread float&) — cùng ngữ nghĩa.
        return
            "inline float tglmt_modf(float x, thread float& i) { return modf(x, i); }\n"
            "inline float2 tglmt_modf(float2 x, thread float2& i) { return modf(x, i); }\n"
            "inline float3 tglmt_modf(float3 x, thread float3& i) { return modf(x, i); }\n"
            "inline float4 tglmt_modf(float4 x, thread float4& i) { return modf(x, i); }\n";
    }
    if (key == "frexp") {
        // GLSL 4.60 §8.3: frexp tách x = significand * 2^exp, significand ∈
        // [0.5,1.0). MSL: frexp(float, thread int&).
        return
            "inline float tglmt_frexp(float x, thread int& e) { return frexp(x, e); }\n"
            "inline float2 tglmt_frexp(float2 x, thread int2& e) { return frexp(x, e); }\n"
            "inline float3 tglmt_frexp(float3 x, thread int3& e) { return frexp(x, e); }\n"
            "inline float4 tglmt_frexp(float4 x, thread int4& e) { return frexp(x, e); }\n";
    }
    if (key == "uaddCarry") {
        // GLSL 4.60 §8.7: carry = 1 nếu tổng >= 2^32.
        return
            "inline uint tglmt_uaddCarry(uint x, uint y, thread uint& carry) {\n"
            "  uint s = x + y; carry = (s < x) ? 1u : 0u; return s; }\n"
            "inline uint2 tglmt_uaddCarry(uint2 x, uint2 y, thread uint2& carry) {\n"
            "  uint2 s = x + y; carry = select(uint2(0u), uint2(1u), s < x); return s; }\n"
            "inline uint3 tglmt_uaddCarry(uint3 x, uint3 y, thread uint3& carry) {\n"
            "  uint3 s = x + y; carry = select(uint3(0u), uint3(1u), s < x); return s; }\n"
            "inline uint4 tglmt_uaddCarry(uint4 x, uint4 y, thread uint4& carry) {\n"
            "  uint4 s = x + y; carry = select(uint4(0u), uint4(1u), s < x); return s; }\n";
    }
    if (key == "usubBorrow") {
        // GLSL 4.60 §8.7: borrow = 1 nếu x < y.
        return
            "inline uint tglmt_usubBorrow(uint x, uint y, thread uint& borrow) {\n"
            "  uint d = x - y; borrow = (x < y) ? 1u : 0u; return d; }\n"
            "inline uint2 tglmt_usubBorrow(uint2 x, uint2 y, thread uint2& borrow) {\n"
            "  uint2 d = x - y; borrow = select(uint2(0u), uint2(1u), x < y); return d; }\n"
            "inline uint3 tglmt_usubBorrow(uint3 x, uint3 y, thread uint3& borrow) {\n"
            "  uint3 d = x - y; borrow = select(uint3(0u), uint3(1u), x < y); return d; }\n"
            "inline uint4 tglmt_usubBorrow(uint4 x, uint4 y, thread uint4& borrow) {\n"
            "  uint4 d = x - y; borrow = select(uint4(0u), uint4(1u), x < y); return d; }\n";
    }
    if (key == "umulExtended") {
        // GLSL 4.60 §8.7: nhân 32-bit → 64-bit; 32 bit thấp vào lsb, 32 bit
        // cao vào msb. Metal có ulong (64-bit) ⇒ nhân chính xác.
        return
            "inline void tglmt_umulExtended(uint x, uint y, thread uint& msb, thread uint& lsb) {\n"
            "  ulong p = ulong(x) * ulong(y); msb = uint(p >> 32); lsb = uint(p & 0xFFFFFFFFul); }\n"
            "inline void tglmt_umulExtended(uint2 x, uint2 y, thread uint2& msb, thread uint2& lsb) {\n"
            "  ulong p0 = ulong(x.x) * ulong(y.x); ulong p1 = ulong(x.y) * ulong(y.y);\n"
            "  msb = uint2(uint(p0 >> 32), uint(p1 >> 32));\n"
            "  lsb = uint2(uint(p0 & 0xFFFFFFFFul), uint(p1 & 0xFFFFFFFFul)); }\n"
            "inline void tglmt_umulExtended(uint3 x, uint3 y, thread uint3& msb, thread uint3& lsb) {\n"
            "  ulong p0 = ulong(x.x) * ulong(y.x); ulong p1 = ulong(x.y) * ulong(y.y);\n"
            "  ulong p2 = ulong(x.z) * ulong(y.z);\n"
            "  msb = uint3(uint(p0 >> 32), uint(p1 >> 32), uint(p2 >> 32));\n"
            "  lsb = uint3(uint(p0 & 0xFFFFFFFFul), uint(p1 & 0xFFFFFFFFul), uint(p2 & 0xFFFFFFFFul)); }\n"
            "inline void tglmt_umulExtended(uint4 x, uint4 y, thread uint4& msb, thread uint4& lsb) {\n"
            "  ulong p0 = ulong(x.x) * ulong(y.x); ulong p1 = ulong(x.y) * ulong(y.y);\n"
            "  ulong p2 = ulong(x.z) * ulong(y.z); ulong p3 = ulong(x.w) * ulong(y.w);\n"
            "  msb = uint4(uint(p0 >> 32), uint(p1 >> 32), uint(p2 >> 32), uint(p3 >> 32));\n"
            "  lsb = uint4(uint(p0 & 0xFFFFFFFFul), uint(p1 & 0xFFFFFFFFul),\n"
            "              uint(p2 & 0xFFFFFFFFul), uint(p3 & 0xFFFFFFFFul)); }\n";
    }
    if (key == "imulExtended") {
        // GLSL 4.60 §8.7: nhân có dấu 32-bit → 64-bit (long của Metal là 64-bit
        // có dấu). p >> 32 là shift số học ⇒ msb đúng cho số âm.
        return
            "inline void tglmt_imulExtended(int x, int y, thread int& msb, thread int& lsb) {\n"
            "  long p = long(x) * long(y); msb = int(p >> 32); lsb = int(p & 0xFFFFFFFFl); }\n"
            "inline void tglmt_imulExtended(int2 x, int2 y, thread int2& msb, thread int2& lsb) {\n"
            "  long p0 = long(x.x) * long(y.x); long p1 = long(x.y) * long(y.y);\n"
            "  msb = int2(int(p0 >> 32), int(p1 >> 32));\n"
            "  lsb = int2(int(p0 & 0xFFFFFFFFl), int(p1 & 0xFFFFFFFFl)); }\n"
            "inline void tglmt_imulExtended(int3 x, int3 y, thread int3& msb, thread int3& lsb) {\n"
            "  long p0 = long(x.x) * long(y.x); long p1 = long(x.y) * long(y.y);\n"
            "  long p2 = long(x.z) * long(y.z);\n"
            "  msb = int3(int(p0 >> 32), int(p1 >> 32), int(p2 >> 32));\n"
            "  lsb = int3(int(p0 & 0xFFFFFFFFl), int(p1 & 0xFFFFFFFFl), int(p2 & 0xFFFFFFFFl)); }\n"
            "inline void tglmt_imulExtended(int4 x, int4 y, thread int4& msb, thread int4& lsb) {\n"
            "  long p0 = long(x.x) * long(y.x); long p1 = long(x.y) * long(y.y);\n"
            "  long p2 = long(x.z) * long(y.z); long p3 = long(x.w) * long(y.w);\n"
            "  msb = int4(int(p0 >> 32), int(p1 >> 32), int(p2 >> 32), int(p3 >> 32));\n"
            "  lsb = int4(int(p0 & 0xFFFFFFFFl), int(p1 & 0xFFFFFFFFl),\n"
            "              int(p2 & 0xFFFFFFFFl), int(p3 & 0xFFFFFFFFl)); }\n";
    }
    if (key == "matrixCompMult") {
        // GLSL 4.60 §8.6: result[i][j] = x[i][j] * y[i][j] (nhân thành phần).
        return
            "inline float2x2 tglmt_matrixCompMult(float2x2 a, float2x2 b) {\n"
            "  return float2x2(a[0] * b[0], a[1] * b[1]); }\n"
            "inline float3x3 tglmt_matrixCompMult(float3x3 a, float3x3 b) {\n"
            "  return float3x3(a[0] * b[0], a[1] * b[1], a[2] * b[2]); }\n"
            "inline float4x4 tglmt_matrixCompMult(float4x4 a, float4x4 b) {\n"
            "  return float4x4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]); }\n";
    }
    if (key == "outerProduct") {
        // GLSL 4.60 §8.6: "Treats the first parameter c as a column vector and
        // the second parameter r as a row vector and does a linear algebraic
        // matrix multiply c * r" ⇒ cột j của kết quả = c * r[j].
        // GLSL matCxR = C cột × R hàng; MSL floatCxR cùng quy ước.
        return
            "inline float2x2 tglmt_outerProduct(float2 c, float2 r) {\n"
            "  return float2x2(c * r[0], c * r[1]); }\n"
            "inline float3x3 tglmt_outerProduct(float3 c, float3 r) {\n"
            "  return float3x3(c * r[0], c * r[1], c * r[2]); }\n"
            "inline float4x4 tglmt_outerProduct(float4 c, float4 r) {\n"
            "  return float4x4(c * r[0], c * r[1], c * r[2], c * r[3]); }\n"
            "inline float2x3 tglmt_outerProduct(float3 c, float2 r) {\n"
            "  return float2x3(c * r[0], c * r[1]); }\n"
            "inline float3x2 tglmt_outerProduct(float2 c, float3 r) {\n"
            "  return float3x2(c * r[0], c * r[1], c * r[2]); }\n"
            "inline float2x4 tglmt_outerProduct(float4 c, float2 r) {\n"
            "  return float2x4(c * r[0], c * r[1]); }\n"
            "inline float4x2 tglmt_outerProduct(float2 c, float4 r) {\n"
            "  return float4x2(c * r[0], c * r[1], c * r[2], c * r[3]); }\n"
            "inline float3x4 tglmt_outerProduct(float4 c, float3 r) {\n"
            "  return float3x4(c * r[0], c * r[1], c * r[2]); }\n"
            "inline float4x3 tglmt_outerProduct(float3 c, float4 r) {\n"
            "  return float4x3(c * r[0], c * r[1], c * r[2], c * r[3]); }\n";
    }
    if (key == "packDouble2x32") {
        // GLSL 4.60 §8.4: packDouble2x32(uvec2) → double có 64 bit = v.x (bit
        // thấp) | v.y (bit cao). Metal KHÔNG có double ⇒ trả về giá trị float
        // gần nhất (double→float làm tròn). float→double luôn chính xác nên
        // unpack(pack(v)) chính xác cho mọi double biểu diễn được.
        return
            "inline float tglmt_packDouble2x32(uint2 v) {\n"
            "  uint lo = v.x; uint hi = v.y;\n"
            "  uint sign = hi & 0x80000000u;\n"
            "  uint dexp = (hi >> 20) & 0x7FFu;\n"
            "  ulong dm = (ulong(hi & 0xFFFFFu) << 32) | ulong(lo);\n"
            "  if (dexp == 0u) return as_type<float>(sign);\n"
            "  if (dexp == 0x7FFu) {\n"
            "    if (dm == 0ul) return as_type<float>(sign | 0x7F800000u);\n"
            "    return as_type<float>(sign | 0x7FC00000u); }\n"
            "  ulong sig = (1ul << 52) | dm;\n"
            "  int ex = int(dexp) - 1023;\n"
            "  int ef; ulong f24;\n"
            "  if (ex >= -126) {\n"
            "    ef = ex;\n"
            "    ulong q = sig >> 29; ulong rem = sig & 0x1FFFFFFFul; ulong hmask = 1ul << 28;\n"
            "    if (rem > hmask || (rem == hmask && (q & 1ul) != 0ul)) {\n"
            "      q += 1ul; if (q == (1ul << 24)) { q >>= 1; ef += 1; } }\n"
            "    f24 = q;\n"
            "  } else {\n"
            "    ef = -126; int rs = -ex - 97;\n"
            "    if (rs >= 64) { f24 = 0ul; }\n"
            "    else {\n"
            "      ulong q = sig >> rs; ulong rem = sig & ((1ul << rs) - 1ul);\n"
            "      ulong hmask = 1ul << (rs - 1);\n"
            "      if (rem > hmask || (rem == hmask && (q & 1ul) != 0ul)) q += 1ul;\n"
            "      f24 = q; } }\n"
            "  if (ef > 127) return as_type<float>(sign | 0x7F800000u);\n"
            "  if (f24 == 0ul) return as_type<float>(sign);\n"
            "  uint frac = uint(f24 & 0x7FFFFFu);\n"
            "  return as_type<float>(sign | (uint(ef + 127) << 23) | frac);\n"
            "}\n";
    }
    if (key == "unpackDouble2x32") {
        // GLSL 4.60 §8.4: unpackDouble2x32(double) → uvec2 (bit thấp trước).
        // Metal không có double; ở đây `v` là float (đã hạ từ double) ⇒ trả
        // về 64 bit biểu diễn double CHÍNH XÁC của giá trị float đó.
        return
            "inline uint2 tglmt_unpackDouble2x32(float v) {\n"
            "  uint b = as_type<uint>(v);\n"
            "  uint sign = b & 0x80000000u;\n"
            "  uint fe = (b >> 23) & 0xFFu;\n"
            "  uint fm = b & 0x007FFFFFu;\n"
            "  uint mant52; uint dexp;\n"
            "  if (fe == 0u) {\n"
            "    if (fm == 0u) { mant52 = 0u; dexp = 0u; }\n"
            "    else {\n"
            "      int k = 0;\n"
            "      for (int i = 0; i < 23; ++i) if ((fm & (1u << i)) != 0u) k = i;\n"
            "      mant52 = (ulong(fm) << (52 - k)) & 0xFFFFFFFFFFFFFull;\n"
            "      dexp = uint(k + 874); } }\n"
            "  else if (fe == 255u) { mant52 = fm << 29; dexp = 0x7FFu; }\n"
            "  else { mant52 = fm << 29; dexp = fe + 896u; }\n"
            "  uint hi = (dexp << 20) | ((mant52 >> 32) & 0xFFFFFu) | sign;\n"
            "  uint lo = mant52 & 0xFFFFFFFFu;\n"
            "  return uint2(lo, hi);\n"
            "}\n";
    }
    return nullptr;
}

// =====================================================================
// 4. VIẾT LẠI
// =====================================================================

struct Ctx {
    std::set<std::string> usedHelpers;
    std::set<std::string> boolVars;
    std::set<std::string> shadowed;   // built-in bị user che (GLSL §8)
};

// Tìm lời gọi kế tiếp của một rule trong [from, limit).
bool FindNextCall(const std::string& text, size_t from, size_t limit, const Ctx& ctx,
                  const Rule** ruleOut, size_t* idPosOut, size_t* openOut,
                  size_t* closeOut) {
    for (size_t i = from; i < limit; ++i) {
        if (!IsIdentChar(text[i], true)) continue;
        size_t j = i + 1;
        while (j < limit && IsIdentChar(text[j], false)) ++j;
        const Rule* rule = nullptr;
        for (const Rule& r : kRules)
            if (text.compare(i, j - i, r.from) == 0) { rule = &r; break; }
        if (!rule) { i = j - 1; continue; }
        if (ctx.shadowed.count(rule->from)) { i = j - 1; continue; }
        // member access / namespace → không phải built-in GLSL
        if (i > 0) {
            char p = text[i - 1];
            if (p == '.') { i = j - 1; continue; }
            if (p == ':' && i >= 2 && text[i - 2] == ':') { i = j - 1; continue; }
        }
        if (!IsCallAt(text, j)) { i = j - 1; continue; }
        size_t open = j;
        while (open < limit && isspace((unsigned char)text[open])) ++open;
        size_t close = FindMatchingParen(text, open);
        if (close == std::string::npos || close >= limit) { i = j - 1; continue; }
        *ruleOut = rule;
        *idPosOut = i;
        *openOut = open;
        *closeOut = close;
        return true;
    }
    return false;
}

// Dựng text thay thế cho một lời gọi đã nhận diện.
//   rule  : rule tương ứng
//   args  : danh sách tham số (đã viết lại các call lồng nhau)
//   decls : (out) khai báo out nội tuyến cần nâng lên block cha
// Trả về false nếu không xử lý được (⇒ giữ nguyên lời gọi).
bool BuildReplacement(const Rule& rule, const std::vector<std::string>& args, Ctx* ctx,
                      std::vector<std::string>* decls, std::string* repl) {
    if (rule.arity >= 0 && (int)args.size() != rule.arity) return false;
    std::string newArgs;
    for (size_t a = 0; a < args.size(); ++a) {
        std::string arg = Trim(args[a]);
        bool isOut = (rule.outMask & (1u << a)) != 0;
        if (isOut && !arg.empty()) {
            std::string ty, nm;
            if (ParseInlineOutDecl(arg, &ty, &nm)) {
                // out-param khai báo nội tuyến: nâng lên block cha.
                decls->push_back(ty + " " + nm + ";");
                arg = nm;
            }
        }
        if (a) newArgs += ", ";
        newArgs += arg;
    }
    if (rule.to == nullptr) {
        // mix: chỉ đổi khi CẢ 3 tham số là biểu thức bool (GLSL 4.60 §8.3
        // chỉ có mix(bool,bool,bool) — bool scalar).
        for (size_t a = 0; a < args.size(); ++a)
            if (!IsBoolExpr(args[a], ctx->boolVars)) return false;
        *repl = "select(" + newArgs + ")";
        return true;
    }
    *repl = std::string(rule.to) + "(" + newArgs + ")";
    if (rule.helper) ctx->usedHelpers.insert(rule.helper);
    return true;
}

// Viết lại các lời gọi bên trong [begin,end) (đệ quy cho call lồng nhau).
// `decls` gom khai báo out cần nâng lên block cha.
std::string RewriteRange(const std::string& text, size_t begin, size_t end, Ctx* ctx,
                         std::vector<std::string>* decls) {
    std::string out;
    out.reserve(end - begin);
    size_t i = begin;
    while (i < end) {
        const Rule* rule = nullptr;
        size_t idPos = 0, open = 0, close = 0;
        if (!FindNextCall(text, i, end, *ctx, &rule, &idPos, &open, &close)) {
            out.append(text, i, end - i);
            break;
        }
        out.append(text, i, idPos - i);
        // Viết lại phần tham số (đệ quy) trước.
        std::string inner = RewriteRange(text, open + 1, close, ctx, decls);
        std::vector<std::string> args = SplitCallArgs(inner);
        std::string repl;
        if (!BuildReplacement(*rule, args, ctx, decls, &repl)) {
            out.append(text, idPos, close - idPos + 1);
        } else {
            out += repl;
        }
        i = close + 1;
    }
    return out;
}

// Viết lại toàn bộ text (body hoặc helpers). Các khai báo out được chèn vào
// block cha của lời gọi tương ứng.
void RewriteText(std::string* text, Ctx* ctx) {
    const std::string& src = *text;
    struct Event {
        size_t pos;
        int kind;        // 0 = chèn, 1 = thay thế
        size_t end;
        std::string text;
    };
    std::vector<Event> events;
    size_t i = 0;
    while (i < src.size()) {
        const Rule* rule = nullptr;
        size_t idPos = 0, open = 0, close = 0;
        if (!FindNextCall(src, i, src.size(), *ctx, &rule, &idPos, &open, &close)) break;
        std::vector<std::string> decls;
        std::string inner = RewriteRange(src, open + 1, close, ctx, &decls);
        std::vector<std::string> args = SplitCallArgs(inner);
        std::string repl;
        if (BuildReplacement(*rule, args, ctx, &decls, &repl)) {
            events.push_back({idPos, 1, close + 1, repl});
            if (!decls.empty()) {
                std::string d;
                for (const std::string& s : decls) d += s + " ";
                size_t bs = FindBlockStart(src, idPos);
                events.push_back({bs, 0, bs, d});
            }
        }
        i = close + 1;
    }
    if (events.empty()) return;
    // Dựng output theo thứ tự TĂNG dẫn vị trí (mọi vị trí đều tính trên
    // `src` ⇒ không bị dịch bởi các edit khác). Cùng vị trí: khai báo chèn
    // (kind 0) đứng TRƯỚC lời gọi được thay thế (kind 1) — biến out phải
    // được khai báo trước khi dùng.
    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.pos != b.pos) return a.pos < b.pos;
        return a.kind < b.kind;
    });
    std::string out;
    out.reserve(src.size() + 64);
    size_t pos = 0;
    for (const Event& e : events) {
        if (e.pos < pos) continue;          // đã bị phủ bởi event trước
        out.append(src, pos, e.pos - pos);
        out += e.text;
        pos = (e.kind == 1) ? e.end : e.pos;
    }
    out.append(src, pos, src.size() - pos);
    text->swap(out);
}

}  // namespace

// =====================================================================
// 5. PASS
// =====================================================================

static void ApplyGLSLBuiltinFnMath(GLSLBuiltinCtx& ctx) {
    if (!ctx.body && !ctx.helpers) return;
    Ctx c;
    // User định nghĩa hàm trùng tên built-in ⇒ built-in đó bị che (GLSL 4.60
    // §8: "doing this will hide all built-in functions with the same name").
    if (ctx.helpers) {
        for (const Rule& r : kRules) {
            // User định nghĩa hàm trùng tên built-in ⇒ built-in bị che.
            if (UserDefinesFunction(*ctx.helpers, r.from)) c.shadowed.insert(r.from);
            // Tên MSL đích bị user chiếm ⇒ không dịch (tránh gọi nhầm).
            if (r.collide && UserDefinesFunction(*ctx.helpers, r.collide))
                c.shadowed.insert(r.from);
        }
    }
    // Biến bool đã biết (để nhận mix(bool,bool,bool)).
    {
        std::string all;
        if (ctx.body) all += *ctx.body;
        if (ctx.helpers) all += *ctx.helpers;
        c.boolVars = CollectBoolVars(all);
    }
    if (ctx.body) RewriteText(ctx.body, &c);
    if (ctx.helpers) RewriteText(ctx.helpers, &c);
    // Phát helper MSL CHỈ khi thực sự dùng (giữ MSL byte-identical cho
    // các shader không liên quan).
    if (ctx.mslExtra) {
        std::string extra;
        for (const char* key : {"modf", "frexp", "uaddCarry", "usubBorrow",
                                "umulExtended", "imulExtended", "matrixCompMult",
                                "outerProduct", "packDouble2x32", "unpackDouble2x32"}) {
            if (!c.usedHelpers.count(key)) continue;
            const char* h = HelperText(key);
            if (h) extra += h;
        }
        if (!extra.empty()) *ctx.mslExtra += extra;
    }
}

static const bool kRegFnMath = RegisterGLSLBuiltinPass("fn_math", ApplyGLSLBuiltinFnMath);

}  // namespace tglmt::gl
