// tests/shaders/test_glsl_builtin_fn_math.cpp — kiểm thử nhóm built-in
// function GLSL "fn_math" (GLSL 4.60 §8) → MSL.
//
// Mỗi hàm được kiểm bằng cách dịch một shader dùng hàm đó và kiểm tra MSL
// sinh ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash").
//
// Ngoài ra test còn kiểm tra 2 tính chất quan trọng của pass:
//   • MSL của shader KHÔNG dùng nhóm này phải không có helper thừa (byte
//     identical với trước khi có pass).
//   • Nếu user tự định nghĩa hàm trùng tên built-in, GLSL 4.60 §8 quy định
//     built-in bị che ⇒ pass KHÔNG được viết lại lời gọi đó.
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;

namespace {

constexpr uint32_t kVs = 0x8B31u;  // GL_VERTEX_SHADER
constexpr uint32_t kFs = 0x8B30u;  // GL_FRAGMENT_SHADER

// Dịch 1 shader và trả về MSL (rỗng nếu convert lỗi).
std::string ToMsl(const std::string& glsl, uint32_t stage) {
    GLSLConvertResult r = ConvertGLSLtoMSL(glsl, stage);
    if (!r.ok) {
        fprintf(stderr, "convert that bai: %s\n", r.log.c_str());
        return std::string();
    }
    return r.msl;
}

bool Has(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

// Có identifier `word` đứng RIÊNG trong `s` không? (không phải hậu tố của
// identifier khác, ví dụ "tglmt_modf" không phải "modf").
bool HasWord(const std::string& s, const char* word) {
    size_t n = strlen(word);
    size_t p = 0;
    while ((p = s.find(word, p)) != std::string::npos) {
        bool l = p > 0 && (isalnum((unsigned char)s[p - 1]) || s[p - 1] == '_');
        size_t e = p + n;
        bool r = e < s.size() && (isalnum((unsigned char)s[e]) || s[e] == '_');
        if (!l && !r) return true;
        p = e;
    }
    return false;
}

// Shader vertex 1 câu lệnh.
std::string Vs1(const std::string& body) {
    return "#version 460\nvoid main(){ " + body + " gl_Position = vec4(1.0); }\n";
}

}  // namespace

int main() {
    // ================= modf (§8.3) =================
    {
        std::string msl = ToMsl(Vs1("float ip; float fp = modf(1.5, ip);"),
                                kVs);
        TGLMT_CHECK(!msl.empty(), "modf: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_modf(1.5, ip)"),
                    "modf: phai doi thanh tglmt_modf(x, i)");
        TGLMT_CHECK(Has(msl, "inline float tglmt_modf(float x, thread float& i)"),
                    "modf: phai co helper tglmt_modf(float, thread float&)");
        TGLMT_CHECK(!Has(msl, "out float"),
                    "modf: qualifier 'out' cua GLSL phai duoc lo het khi doi sang MSL");
    }
    // modf dạng khai báo out nội tuyến ⇒ biến phải được khai báo ở block.
    {
        std::string msl = ToMsl(Vs1("float fp = modf(1.5, out float ip);"), kVs);
        TGLMT_CHECK(!msl.empty(), "modf(out): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_modf(1.5, ip)"),
                    "modf(out): phai doi thanh tglmt_modf(x, i)");
        TGLMT_CHECK(Has(msl, "float ip;"),
                    "modf(out): phai khai bao bien out (float ip;)");
    }
    // modf vector
    {
        std::string msl = ToMsl(Vs1("vec2 ip; vec2 fp = modf(vec2(1.5), ip);"), kVs);
        TGLMT_CHECK(!msl.empty(), "modf(vec2): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_modf(float2(1.5), ip)"),
                    "modf(vec2): phai doi thanh tglmt_modf");
        TGLMT_CHECK(Has(msl, "inline float2 tglmt_modf(float2 x, thread float2& i)"),
                    "modf(vec2): phai co overload float2");
    }

    // ================= frexp (§8.3) =================
    {
        std::string msl = ToMsl(Vs1("int e; float m = frexp(1.5, e);"), kVs);
        TGLMT_CHECK(!msl.empty(), "frexp: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_frexp(1.5, e)"),
                    "frexp: phai doi thanh tglmt_frexp(x, e)");
        TGLMT_CHECK(Has(msl, "inline float tglmt_frexp(float x, thread int& e)"),
                    "frexp: phai co helper tglmt_frexp(float, thread int&)");
    }
    {
        std::string msl = ToMsl(Vs1("float m = frexp(1.5, out int e);"), kVs);
        TGLMT_CHECK(!msl.empty(), "frexp(out): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_frexp(1.5, e)"),
                    "frexp(out): phai doi thanh tglmt_frexp(x, e)");
        TGLMT_CHECK(Has(msl, "int e;"), "frexp(out): phai khai bao bien out (int e;)");
    }

    // ================= roundEven (§8.3) =================
    {
        std::string msl = ToMsl(Vs1("float r = roundEven(1.5);"), kVs);
        TGLMT_CHECK(!msl.empty(), "roundEven: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "rint(1.5)"),
                    "roundEven: phai doi thanh rint (round-half-to-even cua MSL)");
        TGLMT_CHECK(!Has(msl, "roundEven"),
                    "roundEven: khong con ten GLSL roundEven");
    }

    // ================= matrixCompMult (§8.6) =================
    {
        std::string msl = ToMsl(Vs1("mat4 m = matrixCompMult(mat4(1.0), mat4(2.0));"),
                                kVs);
        TGLMT_CHECK(!msl.empty(), "matrixCompMult: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_matrixCompMult("),
                    "matrixCompMult: phai doi thanh tglmt_matrixCompMult");
        TGLMT_CHECK(Has(msl, "inline float4x4 tglmt_matrixCompMult(float4x4 a, float4x4 b)"),
                    "matrixCompMult: phai co helper float4x4");
    }

    // ================= outerProduct (§8.6) =================
    {
        std::string msl = ToMsl(Vs1("mat3 m = outerProduct(vec3(1.0), vec3(2.0));"),
                                kVs);
        TGLMT_CHECK(!msl.empty(), "outerProduct: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_outerProduct("),
                    "outerProduct: phai doi thanh tglmt_outerProduct");
        TGLMT_CHECK(Has(msl, "inline float3x3 tglmt_outerProduct(float3 c, float3 r)"),
                    "outerProduct: phai co helper float3x3");
    }

    // ================= not (§8.7 Vector Relational) =================
    {
        std::string msl = ToMsl(Vs1("bvec2 b = not(bvec2(true));"), kVs);
        TGLMT_CHECK(!msl.empty(), "not: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "!(bool2(true))"),
                    "not: phai doi thanh toan tu ! (MSL khong dung duoc metal::not)");
        TGLMT_CHECK(!Has(msl, "tglmt_kw_not"),
                    "not: khong con ten da doi cua keyword C++ 'not'");
    }

    // ================= uaddCarry / usubBorrow (§8.8) =================
    {
        std::string msl = ToMsl(Vs1("uint c; uint s = uaddCarry(1u, 2u, c);"), kVs);
        TGLMT_CHECK(!msl.empty(), "uaddCarry: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_uaddCarry(1u, 2u, c)"),
                    "uaddCarry: phai doi thanh tglmt_uaddCarry");
        TGLMT_CHECK(Has(msl, "inline uint tglmt_uaddCarry(uint x, uint y, thread uint& carry)"),
                    "uaddCarry: phai co helper uint");
    }
    {
        std::string msl = ToMsl(Vs1("uint s = uaddCarry(1u, 2u, out uint c);"), kVs);
        TGLMT_CHECK(!msl.empty(), "uaddCarry(out): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_uaddCarry(1u, 2u, c)"),
                    "uaddCarry(out): phai doi thanh tglmt_uaddCarry");
        TGLMT_CHECK(Has(msl, "uint c;"), "uaddCarry(out): phai khai bao bien out");
    }
    {
        std::string msl = ToMsl(Vs1("uint b; uint d = usubBorrow(2u, 1u, b);"), kVs);
        TGLMT_CHECK(!msl.empty(), "usubBorrow: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_usubBorrow(2u, 1u, b)"),
                    "usubBorrow: phai doi thanh tglmt_usubBorrow");
        TGLMT_CHECK(Has(msl, "inline uint tglmt_usubBorrow(uint x, uint y, thread uint& borrow)"),
                    "usubBorrow: phai co helper uint");
    }

    // ================= umulExtended / imulExtended (§8.8) =================
    {
        std::string msl = ToMsl(Vs1("uint hi, lo; umulExtended(2u, 3u, hi, lo);"), kVs);
        TGLMT_CHECK(!msl.empty(), "umulExtended: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_umulExtended(2u, 3u, hi, lo)"),
                    "umulExtended: phai doi thanh tglmt_umulExtended");
        TGLMT_CHECK(Has(msl, "inline void tglmt_umulExtended(uint x, uint y, thread uint& msb, thread uint& lsb)"),
                    "umulExtended: phai co helper uint");
    }
    {
        std::string msl = ToMsl(Vs1("umulExtended(2u, 3u, out uint hi, out uint lo);"), kVs);
        TGLMT_CHECK(!msl.empty(), "umulExtended(out): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_umulExtended(2u, 3u, hi, lo)"),
                    "umulExtended(out): phai doi thanh tglmt_umulExtended");
        TGLMT_CHECK(Has(msl, "uint hi;") && Has(msl, "uint lo;"),
                    "umulExtended(out): phai khai bao ca bien out");
    }
    {
        std::string msl = ToMsl(Vs1("int hi, lo; imulExtended(2, 3, hi, lo);"), kVs);
        TGLMT_CHECK(!msl.empty(), "imulExtended: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_imulExtended(2, 3, hi, lo)"),
                    "imulExtended: phai doi thanh tglmt_imulExtended");
        TGLMT_CHECK(Has(msl, "inline void tglmt_imulExtended(int x, int y, thread int& msb, thread int& lsb)"),
                    "imulExtended: phai co helper int");
    }

    // ================= packSnorm4x8 / unpackSnorm4x8 (§8.4) =================
    {
        std::string msl = ToMsl(Vs1("uint p = packSnorm4x8(vec4(0.5));"), kVs);
        TGLMT_CHECK(!msl.empty(), "packSnorm4x8: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "pack_float_to_snorm4x8("),
                    "packSnorm4x8: phai doi thanh pack_float_to_snorm4x8");
        TGLMT_CHECK(!Has(msl, "packSnorm4x8"),
                    "packSnorm4x8: khong con ten GLSL");
    }
    {
        std::string msl = ToMsl(Vs1("vec4 v = unpackSnorm4x8(1u);"), kVs);
        TGLMT_CHECK(!msl.empty(), "unpackSnorm4x8: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "unpack_snorm4x8_to_float("),
                    "unpackSnorm4x8: phai doi thanh unpack_snorm4x8_to_float");
        TGLMT_CHECK(!Has(msl, "unpackSnorm4x8"),
                    "unpackSnorm4x8: khong con ten GLSL");
    }

    // ================= packDouble2x32 / unpackDouble2x32 (§8.4) =================
    {
        std::string msl = ToMsl(Vs1("double d = packDouble2x32(uvec2(1u));"), kVs);
        TGLMT_CHECK(!msl.empty(), "packDouble2x32: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_packDouble2x32("),
                    "packDouble2x32: phai doi thanh tglmt_packDouble2x32");
        TGLMT_CHECK(Has(msl, "inline float tglmt_packDouble2x32(uint2 v)"),
                    "packDouble2x32: phai co helper (emulation double->float)");
    }
    {
        std::string msl = ToMsl(Vs1("uvec2 u = unpackDouble2x32(1.0L);"), kVs);
        TGLMT_CHECK(!msl.empty(), "unpackDouble2x32: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_unpackDouble2x32("),
                    "unpackDouble2x32: phai doi thanh tglmt_unpackDouble2x32");
        TGLMT_CHECK(Has(msl, "inline uint2 tglmt_unpackDouble2x32(float v)"),
                    "unpackDouble2x32: phai co helper (emulation float->double bits)");
    }

    // ================= uvecBitsToFloat (§8.3, biến thể vector) =================
    {
        std::string msl = ToMsl(Vs1("vec2 f = uvecBitsToFloat(uvec2(1u));"), kVs);
        TGLMT_CHECK(!msl.empty(), "uvecBitsToFloat: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "as_type<float2>("),
                    "uvecBitsToFloat: phai doi thanh as_type<float2>(uvec2)");
        TGLMT_CHECK(!Has(msl, "uvecBitsToFloat"),
                    "uvecBitsToFloat: khong con ten GLSL");
    }

    // ================= mix(bool,bool,bool) (§8.3) =================
    {
        std::string msl = ToMsl(Vs1("bool b = mix(true, false, true);"), kVs);
        TGLMT_CHECK(!msl.empty(), "mix(bool): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "select(true, false, true)"),
                    "mix(bool): phai doi thanh select(false_val, true_val, cond)");
        TGLMT_CHECK(!Has(msl, "mix(true"),
                    "mix(bool): khong con loi goi GLSL mix");
    }
    {
        // mix thường (float) phải GIỮ NGUYÊN (không phải select).
        std::string msl = ToMsl(Vs1("float f = mix(0.0, 1.0, 0.5);"), kVs);
        TGLMT_CHECK(!msl.empty(), "mix(float): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "mix(0.0, 1.0, 0.5)"),
                    "mix(float): phai giu nguyen (lerp cua MSL)");
    }
    {
        // mix(bool,bool,bool) với BIẾN bool (không phải literal) cũng phải
        // thành select.
        std::string msl = ToMsl(Vs1("bool x = true; bool y = false;"
                                    " bool r = mix(x, y, x);"),
                                kVs);
        TGLMT_CHECK(!msl.empty(), "mix(bool var): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "select(x, y, x)"),
                    "mix(bool var): phai doi thanh select");
    }

    // ================= transpose / inverse: KHÔNG viết lại =================
    {
        std::string msl = ToMsl(Vs1("mat3 t = transpose(mat3(1.0));"), kVs);
        TGLMT_CHECK(!msl.empty(), "transpose: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "transpose("),
                    "transpose: MSL co transpose san (khong can helper)");
        TGLMT_CHECK(!Has(msl, "tglmt_transpose"),
                    "transpose: khong duoc viet lai (tranh trung lap)");
    }
    {
        std::string msl = ToMsl(Vs1("mat3 i = inverse(mat3(1.0));"), kVs);
        TGLMT_CHECK(!msl.empty(), "inverse: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_inverse("),
                    "inverse: pass RewriteInverseCalls da xu ly");
        TGLMT_CHECK(!Has(msl, "tglmt_tglmt_inverse"),
                    "inverse: khong duoc viet lai (tranh trung lap)");
    }

    // ============ Shader KHÔNG dùng nhóm này: không được thêm helper ============
    {
        std::string msl = ToMsl(Vs1("float f = sin(1.0) + cos(2.0);"), kVs);
        TGLMT_CHECK(!msl.empty(), "shader lien quan: convert phai thanh cong");
        for (const char* h : {"tglmt_modf", "tglmt_frexp", "tglmt_uaddCarry",
                              "tglmt_usubBorrow", "tglmt_umulExtended",
                              "tglmt_imulExtended", "tglmt_matrixCompMult",
                              "tglmt_outerProduct", "tglmt_packDouble2x32",
                              "tglmt_unpackDouble2x32"}) {
            if (Has(msl, h)) {
                fprintf(stderr, "helper thua: %s\n", h);
                TGLMT_CHECK(false, "shader khong dung nhom nay khong duoc them helper");
            }
        }
    }

    // ============ User định nghĩa hàm trùng tên ⇒ built-in bị che ============
    {
        std::string glsl =
            "#version 460\n"
            "float mymodf(float x, out float i){ i = floor(x); return x - i; }\n"
            "void main(){ float ip; float f = mymodf(1.5, ip); gl_Position = vec4(f); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user function: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_modf"),
                    "user dinh nghia ham trung ten: khong duoc them helper tglmt_modf");
    }
    {
        // Tên trùng đúng tên built-in ⇒ GLSL 4.60 §8: built-in bị che ⇒ pass
        // KHÔNG được viết lại lời gọi.
        std::string glsl =
            "#version 460\n"
            "float modf(float x, out float i){ i = floor(x); return x - i; }\n"
            "void main(){ float ip; float f = modf(1.5, ip); gl_Position = vec4(f); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user shadow: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_modf"),
                    "user shadow built-in modf: khong duoc viet lai thanh tglmt_modf");
        TGLMT_CHECK(Has(msl, "modf(1.5, ip)"),
                    "user shadow built-in modf: loi goi cua user phai giu nguyen");
    }
    {
        std::string glsl =
            "#version 460\n"
            "uint uaddCarry(uint x, uint y, out uint c){ c = 0u; return x + y; }\n"
            "void main(){ uint c; uint s = uaddCarry(1u, 2u, c); gl_Position = vec4(float(s)); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user shadow uaddCarry: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_uaddCarry"),
                    "user shadow built-in uaddCarry: khong duoc viet lai");
    }

    // ============ Lời gọi trong HÀM HELPER của user cũng phải được dịch ====
    {
        std::string glsl =
            "#version 460\n"
            "float myfrac(in float x, out int e){ return frexp(x, e); }\n"
            "void main(){ int e; float s = myfrac(1.5, e); gl_Position = vec4(s); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "helper function: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_frexp(x, e)"),
                    "frexp trong ham helper: phai doi thanh tglmt_frexp");
    }

    // ============ "Bếp nồi": một shader dùng TẤT CẢ hàm của nhóm ============
    {
        std::string glsl =
            "#version 460\n"
            "void main(){\n"
            "  float ip; float fp = modf(1.5, ip);\n"
            "  int e; float fr = frexp(1.5, e);\n"
            "  uint c; uint s = uaddCarry(1u, 2u, c);\n"
            "  uint b; uint d = usubBorrow(2u, 1u, b);\n"
            "  uint hi, lo; umulExtended(2u, 3u, hi, lo);\n"
            "  int ihi, ilo; imulExtended(2, 3, ihi, ilo);\n"
            "  float re = roundEven(1.5);\n"
            "  mat4 mm = matrixCompMult(mat4(1.0), mat4(2.0));\n"
            "  mat3 op = outerProduct(vec3(1.0), vec3(2.0));\n"
            "  uint pk = packSnorm4x8(vec4(0.5));\n"
            "  vec4 up = unpackSnorm4x8(pk);\n"
            "  vec2 bf = uvecBitsToFloat(uvec2(1u));\n"
            "  bool bl = mix(true, false, true);\n"
            "  bvec2 nb = not(bvec2(true, false));\n"
            "  gl_Position = vec4(fp+fr+re+mm[0][0]+op[0][0]+up.x+bf.x,\n"
            "                    float(s+d+hi+lo+ihi+ilo+pk), bl ? 1.0 : 0.0,\n"
            "                    nb.x ? 1.0 : 0.0);\n"
            "}\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "kitchen sink: convert phai thanh cong");
        for (const char* want : {"tglmt_modf(", "tglmt_frexp(", "tglmt_uaddCarry(",
                                 "tglmt_usubBorrow(", "tglmt_umulExtended(",
                                 "tglmt_imulExtended(", "rint(", "tglmt_matrixCompMult(",
                                 "tglmt_outerProduct(", "pack_float_to_snorm4x8(",
                                 "unpack_snorm4x8_to_float(", "as_type<float2>(",
                                 "select(true, false, true)", "!(bool2(true, false))"}) {
            if (!Has(msl, want)) {
                fprintf(stderr, "thieu mau: %s\n", want);
                TGLMT_CHECK(false, "kitchen sink: phai co tat ca dang MSL");
            }
        }
    }

    return TGLMT_RESULT;
}
