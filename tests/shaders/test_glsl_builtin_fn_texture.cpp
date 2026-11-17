// tests/shaders/test_glsl_builtin_fn_texture.cpp — kiểm thử nhóm built-in
// function GLSL "fn_texture" (GLSL 4.60 §8) → MSL.
//
// Mỗi hàm được kiểm bằng cách dịch một shader dùng hàm đó và kiểm tra MSL
// sinh ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash").
//
// Ngoài ra test còn kiểm tra 3 tính chất quan trọng của pass:
//   • MSL của shader KHÔNG dùng nhóm này phải không có helper thừa.
//   • Nếu user tự định nghĩa hàm trùng tên built-in, GLSL 4.60 §8 quy định
//     built-in bị che ⇒ pass KHÔNG được viết lại lời gọi đó.
//   • MSL sinh ra phải CÂN BẰNG ngoặc (bắt được lỗi "dấu ')' thừa" của
//     converter ở nhánh textureProj + sampler2DShadow).
#include "tglmt/GLSLConverter.h"
#include "tglmt/gl46.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;

namespace {

// Hằng stage lấy từ header GL sinh tự động (không hardcode hex).
constexpr uint32_t kVs = GL_VERTEX_SHADER;
constexpr uint32_t kFs = GL_FRAGMENT_SHADER;
constexpr uint32_t kCs = GL_COMPUTE_SHADER;

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
// identifier khác, ví dụ "tglmt_noise1" không phải "noise1").
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

// Ngoặc tròn của MSL phải cân bằng (bắt lỗi ")' thừa" của converter).
bool ParensBalanced(const std::string& s) {
    int d = 0;
    for (char c : s) {
        if (c == '(') ++d;
        else if (c == ')') {
            --d;
            if (d < 0) return false;
        }
    }
    return d == 0;
}

// Shader fragment 1 câu lệnh (biến kết quả tên `_r`).
std::string Fs1(const std::string& decl, const std::string& body) {
    return "#version 460\n" + decl + "void main(){ vec4 _r = " + body +
           "; gl_FragColor = vec4(1.0); }\n";
}

// Shader compute 1 câu lệnh.
std::string Cs1(const std::string& decl, const std::string& body) {
    return "#version 460\nlayout(local_size_x=1) in;\n" + decl +
           "void main(){ uint _r = " + body + "; }\n";
}

}  // namespace

int main() {
    // ================= textureProj (§8.9.3, PDF p.184) =================
    // MSL KHÔNG có phép lấy mẫu phối cảnh (đã kiểm chứng: "no member named
    // 'sample_proj'") ⇒ converter hạ về sample + chia tay.
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureProj(s, vec4(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProj: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "s_tex.sample(s_smp,"),
                    "textureProj: phai doi thanh <s>_tex.sample(<s>_smp, …)");
        TGLMT_CHECK(Has(msl, ").xy / ("), "textureProj: phai chia toa do phoi canh");
        TGLMT_CHECK(Has(msl, ").w)"), "textureProj: vec4 phai chia theo .w");
    }
    // sampler2DShadow: converter sinh THỪA một dấu ')' ⇒ pass phải sửa lại,
    // và bias phải nằm TRONG lời gọi sample_compare.
    {
        std::string msl = ToMsl(Fs1("uniform sampler2DShadow sh;",
                                     "textureProj(sh, vec4(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProj shadow: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "sh_tex.sample_compare(sh_smp,"),
                    "textureProj shadow: phai doi thanh sample_compare");
        TGLMT_CHECK(ParensBalanced(msl),
                    "textureProj shadow: MSL phai co ngoac can bang (het dau ')' thua)");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler2DShadow sh;",
                                     "textureProj(sh, vec4(0.5), 0.1)"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProj shadow+bias: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "bias(float(0.1))"),
                    "textureProj shadow+bias: bias phai nam trong loi goi sample_compare");
        TGLMT_CHECK(ParensBalanced(msl),
                    "textureProj shadow+bias: MSL phai co ngoac can bang");
    }

    // ================= textureProjOffset (§8.9.3, PDF p.185) =================
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureProjOffset(s, vec3(0.5), ivec2(1))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProjOffset: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_texProjOffset(s_tex, s_smp, float3(0.5), int2(1))"),
                    "textureProjOffset: phai doi thanh tglmt_texProjOffset");
        TGLMT_CHECK(Has(msl, "inline vec<T,4> tglmt_texProjOffset(texture2d<T> t, sampler s, float3 p, int2 off)"),
                    "textureProjOffset: phai co helper (vec3 ⇒ chia /z)");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureProjOffset(s, vec4(0.5), ivec2(1))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProjOffset vec4: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_texProjOffset(s_tex, s_smp, float4(0.5), int2(1))"),
                    "textureProjOffset vec4: phai doi thanh tglmt_texProjOffset");
    }

    // ================= textureProjLod (§8.9.3, PDF p.186) =================
    // Converter đã xử lý (RewriteSamplingCalls) ⇒ pass không viết lại.
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureProjLod(s, vec3(0.5), 0.0)"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProjLod: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "s_tex.sample(s_smp,"),
                    "textureProjLod: phai doi thanh sample");
        TGLMT_CHECK(Has(msl, "level(float(0.0))"),
                    "textureProjLod: phai truyen level");
        TGLMT_CHECK(!Has(msl, "tglmt_texProjLod"),
                    "textureProjLod: khong duoc viet lai (converter da xu ly)");
    }

    // ================= textureProjLodOffset (§8.9.3, PDF p.186) =================
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureProjLodOffset(s, vec3(0.5), 0.0, ivec2(1))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureProjLodOffset: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_texProjLodOffset(s_tex, s_smp, float3(0.5), 0.0, int2(1))"),
                    "textureProjLodOffset: phai doi thanh tglmt_texProjLodOffset");
        TGLMT_CHECK(Has(msl, "inline vec<T,4> tglmt_texProjLodOffset(texture2d<T> t, sampler s, float3 p, float lod, int2 off)"),
                    "textureProjLodOffset: phai co helper");
    }

    // ================= textureGatherOffsets (§8.9.4, PDF p.193) =================
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureGatherOffsets(s, vec2(0.5), ivec2(1), ivec2(2), ivec2(3), ivec2(4))"),
                                kFs);
        TGLMT_CHECK(!msl.empty(), "textureGatherOffsets: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_textureGatherOffsets(s_tex, s_smp, float2(0.5), int2(1), int2(2), int2(3), int2(4))"),
                    "textureGatherOffsets: phai doi thanh tglmt_textureGatherOffsets");
        TGLMT_CHECK(Has(msl, "t.gather(s, uv, o0, component::x)"),
                    "textureGatherOffsets: MSL khong co gather 4 offset ⇒ phai emulation 4 lan gather");
        TGLMT_CHECK(Has(msl, "return vec<T,4>(g0.w, g1.w, g2.w, g3.w)"),
                    "textureGatherOffsets: phai lay texel i0j0 (thanh phan .w) cua moi footprint");
    }
    // Dạng `comp` (0..3) ⇒ MSL `component` phải là hằng số ⇒ helper riêng.
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;",
                                     "textureGatherOffsets(s, vec2(0.5), ivec2(1), ivec2(2), ivec2(3), ivec2(4), 1)"),
                                kFs);
        TGLMT_CHECK(!msl.empty(), "textureGatherOffsets comp: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_textureGatherOffsetsY(s_tex, s_smp, float2(0.5), int2(1), int2(2), int2(3), int2(4))"),
                    "textureGatherOffsets comp=1: phai dung helper rieng (component::y)");
        TGLMT_CHECK(Has(msl, "component::y"),
                    "textureGatherOffsets comp=1: phai truyen component::y cho gather");
    }
    // Dạng mảng offsets[4] (đúng §8.9.4): biến mảng.
    {
        std::string msl = ToMsl(
            "#version 460\nuniform sampler2D s;\n"
            "void main(){ ivec2 o[4] = ivec2[4](ivec2(1),ivec2(2),ivec2(3),ivec2(4));\n"
            "  vec4 _r = textureGatherOffsets(s, vec2(0.5), o); gl_FragColor = vec4(1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "textureGatherOffsets array: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_textureGatherOffsets(s_tex, s_smp, float2(0.5), o[0], o[1], o[2], o[3])"),
                    "textureGatherOffsets array: phai trich 4 phan tu cua mang offsets");
    }
    // Dạng initializer-list trực tiếp.
    {
        std::string msl = ToMsl(
            "#version 460\nuniform sampler2D s;\n"
            "void main(){ vec4 _r = textureGatherOffsets(s, vec2(0.5), ivec2[4](ivec2(1),ivec2(2),ivec2(3),ivec2(4)));\n"
            "  gl_FragColor = vec4(1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "textureGatherOffsets init list: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_textureGatherOffsets(s_tex, s_smp, float2(0.5), int2(1), int2(2), int2(3), int2(4))"),
                    "textureGatherOffsets init list: phai tach 4 phan tu cua initializer-list");
    }

    // ================= noise1..4 (§8.15, PDF p.206) =================
    // §8.15: "When not generating SPIR-V they are defined to return the value
    // 0.0 or a vector whose components are all 0.0." ⇒ trả 0 là ĐÚNG SPEC.
    {
        std::string msl = ToMsl(Fs1("", "vec4(noise1(vec3(0.5)), 0.0, 0.0, 1.0)"), kFs);
        TGLMT_CHECK(!msl.empty(), "noise1: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_noise1(float3(0.5))"),
                    "noise1: phai doi thanh tglmt_noise1");
        TGLMT_CHECK(Has(msl, "inline float tglmt_noise1(T) { return 0.0f; }"),
                    "noise1: helper phai tra 0 (theo GLSL 4.60 §8.15)");
        TGLMT_CHECK(!Has(msl, "noise::value"),
                    "noise1: KHONG duoc map sang noise::value (MSL khong co ham noise)");
    }
    {
        std::string msl = ToMsl(Fs1("", "vec4(noise2(vec3(0.5)), 0.0, 0.0, 1.0)"), kFs);
        TGLMT_CHECK(!msl.empty(), "noise2: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_noise2(float3(0.5))"),
                    "noise2: phai doi thanh tglmt_noise2");
    }
    {
        std::string msl = ToMsl(Fs1("", "vec4(noise3(vec3(0.5)), 0.0, 1.0)"), kFs);
        TGLMT_CHECK(!msl.empty(), "noise3: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_noise3(float3(0.5))"),
                    "noise3: phai doi thanh tglmt_noise3");
    }
    {
        std::string msl = ToMsl(Fs1("", "noise4(vec3(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "noise4: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_noise4(float3(0.5))"),
                    "noise4: phai doi thanh tglmt_noise4");
        TGLMT_CHECK(Has(msl, "inline float4 tglmt_noise4(T) { return float4(0.0f); }"),
                    "noise4: helper phai tra vec4(0)");
    }

    // ================= interpolateAt* (§8.14.2, PDF p.205) =================
    // MSL pull-model (interpolant<T,Q>) không dùng được với input push-model
    // ⇒ áp biểu thức gốc (đúng với input `flat`/`centroid`).
    {
        std::string msl = ToMsl(Fs1("", "vec4(interpolateAtCentroid(1.0))"), kFs);
        TGLMT_CHECK(!msl.empty(), "interpolateAtCentroid: convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "interpolateAtCentroid"),
                    "interpolateAtCentroid: phai thay bang bieu thuc goc");
        TGLMT_CHECK(Has(msl, "float4(1.0)"), "interpolateAtCentroid: giu nguyen gia tri input");
    }
    {
        std::string msl = ToMsl(Fs1("", "vec4(interpolateAtSample(1.0, 0))"), kFs);
        TGLMT_CHECK(!msl.empty(), "interpolateAtSample: convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "interpolateAtSample"),
                    "interpolateAtSample: phai thay bang bieu thuc goc");
    }
    {
        std::string msl = ToMsl(Fs1("", "vec4(interpolateAtOffset(1.0, vec2(0.0)))"), kFs);
        TGLMT_CHECK(!msl.empty(), "interpolateAtOffset: convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "interpolateAtOffset"),
                    "interpolateAtOffset: phai thay bang bieu thuc goc");
    }

    // ================= Atomic counter (§8.10) + atomic memory (§8.11) =========
    // `uniform atomic_uint` (counter) ⇒ MSL `device atomic_uint*` ⇒ phải gọi
    // atomic_*_explicit TRÊN CON TRỎ (không phải &(lv)).
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicAdd(t_ac, 1u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicAdd counter: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_fetch_add_explicit(t_ac, 1u, memory_order_relaxed)"),
                    "atomicAdd counter: phai goi atomic_fetch_add_explicit tren con tro");
        TGLMT_CHECK(!Has(msl, "tglmt_atomicAdd(&("),
                    "atomicAdd counter: & (con tro) khong hop le cho atomic_uint*");
    }
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicCompSwap(t_ac, 1u, 2u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicCompSwap counter: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_atomicCompSwapCounter(t_ac, 1u, 2u)"),
                    "atomicCompSwap counter: phai dung helper atomic_compare_exchange");
    }
    // atomicCounter* (đọc/ghi counter) — §8.10.
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicCounter(t_ac)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicCounter: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_load_explicit(t_ac, memory_order_relaxed)"),
                    "atomicCounter: phai doi thanh atomic_load_explicit");
    }
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicCounterAdd(t_ac, 2u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicCounterAdd: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_fetch_add_explicit(t_ac, 2u, memory_order_relaxed)"),
                    "atomicCounterAdd: phai doi thanh atomic_fetch_add_explicit");
    }
    // Biến `shared` thường ⇒ helper generic của converter (threadgroup uint*).
    {
        std::string msl = ToMsl(Cs1("shared uint s;", "atomicAdd(s, 1u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicAdd shared uint: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_atomicAdd(&(s), 1u)"),
                    "atomicAdd shared uint: phai giu helper generic (threadgroup uint*)");
    }
    // Biến `shared` kiểu atomic_uint ⇒ atomic_fetch_*_explicit(&lv).
    {
        std::string msl = ToMsl(Cs1("shared atomic_uint c;", "atomicAdd(c, 1u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicAdd shared atomic_uint: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_fetch_add_explicit(&(c), 1u, memory_order_relaxed)"),
                    "atomicAdd shared atomic_uint: phai goi atomic_fetch_add_explicit(&c)");
    }
    // atomicLoad / atomicStore (mở rộng).
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicLoad(t_ac)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicLoad: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_load_explicit(t_ac, memory_order_relaxed)"),
                    "atomicLoad: phai doi thanh atomic_load_explicit");
    }
    {
        std::string msl = ToMsl(Cs1("layout(binding=0) uniform atomic_uint t_ac;",
                                     "atomicStore(t_ac, 3u)"), kCs);
        TGLMT_CHECK(!msl.empty(), "atomicStore: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "atomic_store_explicit(t_ac, 3u, memory_order_relaxed)"),
                    "atomicStore: phai doi thanh atomic_store_explicit");
    }

    // ================= Biến thể Compatibility Profile (§8.9.5) =================
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;", "texture2D(s, vec2(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "texture2D: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "s_tex.sample(s_smp,"),
                    "texture2D: phai doi thanh <s>_tex.sample(<s>_smp, …)");
        TGLMT_CHECK(!HasWord(msl, "texture2D"), "texture2D: khong con ten GLSL");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;", "texture2DLod(s, vec2(0.5), 0.0)"), kFs);
        TGLMT_CHECK(!msl.empty(), "texture2DLod: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "level(float(0.0))"), "texture2DLod: phai truyen level");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;", "texture2DProj(s, vec3(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "texture2DProj: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_texProj(s_tex, s_smp, float3(0.5))"),
                    "texture2DProj: phai doi thanh tglmt_texProj");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;", "texture2DGrad(s, vec2(0.5), vec2(1.0), vec2(1.0))"), kFs);
        TGLMT_CHECK(!msl.empty(), "texture2DGrad: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "gradient2d("), "texture2DGrad: phai truyen gradient2d");
    }
    {
        std::string msl = ToMsl(Fs1("uniform samplerCube c;", "textureCube(c, vec3(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "textureCube: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "c_tex.sample(c_smp,"), "textureCube: phai doi thanh sample float3");
    }
    {
        std::string msl = ToMsl(Fs1("uniform sampler3D t3;", "texture3D(t3, vec3(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "texture3D: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "t3_tex.sample(t3_smp,"), "texture3D: phai doi thanh sample float3");
    }

    // ============ Shader KHÔNG dùng nhóm này: không được thêm helper ============
    {
        std::string msl = ToMsl(Fs1("uniform sampler2D s;", "texture(s, vec2(0.5))"), kFs);
        TGLMT_CHECK(!msl.empty(), "shader lien quan: convert phai thanh cong");
        for (const char* h : {"tglmt_texProjOffset", "tglmt_texProjLodOffset",
                              "tglmt_texProj", "tglmt_texProjLod", "tglmt_texProjGrad",
                              "tglmt_textureGatherOffsets", "tglmt_noise1",
                              "tglmt_atomicLoad", "tglmt_atomicStore",
                              "tglmt_atomicCompSwapCounter"}) {
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
            "float noise1(vec3 x){ return x.x; }\n"
            "void main(){ float n = noise1(vec3(1.0)); gl_Position = vec4(n); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user shadow noise1: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_noise1"),
                    "user shadow built-in noise1: khong duoc viet lai thanh tglmt_noise1");
        TGLMT_CHECK(Has(msl, "noise1(float3(1.0))"),
                    "user shadow built-in noise1: loi goi cua user phai giu nguyen");
    }
    {
        std::string glsl =
            "#version 460\n"
            "uniform sampler2D s;\n"
            "vec4 textureProjOffset(sampler2D t, vec3 p, ivec2 o){ return vec4(1.0); }\n"
            "void main(){ vec4 c = textureProjOffset(s, vec3(0.5), ivec2(1));\n"
            "  gl_Position = c; }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user shadow textureProjOffset: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_texProjOffset"),
                    "user shadow built-in textureProjOffset: khong duoc viet lai");
        TGLMT_CHECK(Has(msl, "textureProjOffset("),
                    "user shadow built-in textureProjOffset: loi goi phai giu nguyen");
    }
    {
        // Tên user trùng TÊN HELPER MSL của pass ⇒ không được gọi nhầm.
        std::string glsl =
            "#version 460\n"
            "float tglmt_noise1(vec3 x){ return x.x; }\n"
            "void main(){ float n = tglmt_noise1(vec3(1.0)); gl_Position = vec4(n); }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user helper ten trung: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "inline float tglmt_noise1(T)"),
                    "user dinh nghia tglmt_noise1: khong duoc phat helper trung ten");
    }
    {
        // User chiếm tên helper của pass ⇒ pass KHÔNG được phát helper trùng
        // tên (tránh định nghĩa trùng trong MSL) và không được gọi nhầm.
        std::string glsl =
            "#version 460\n"
            "uniform sampler2D s;\n"
            "vec4 tglmt_texProjOffset(sampler2D t, vec3 p, ivec2 o){ return vec4(1.0); }\n"
            "void main(){ vec4 c = textureProjOffset(s, vec3(0.5), ivec2(1));\n"
            "  gl_Position = c; }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "user chiem ten helper: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "inline vec<T,4> tglmt_texProjOffset(texture2d<T>"),
                    "user chiem ten helper: khong duoc phat helper trung ten");
    }

    // ============ Lời gọi trong HÀM HELPER của user cũng phải được dịch =======
    {
        std::string glsl =
            "#version 460\n"
            "uniform sampler2D s;\n"
            "vec4 myproj(sampler2D t, vec3 p, ivec2 o){ return textureProjOffset(t, p, o); }\n"
            "void main(){ vec4 c = myproj(s, vec3(0.5), ivec2(1)); gl_Position = c; }\n";
        std::string msl = ToMsl(glsl, kVs);
        TGLMT_CHECK(!msl.empty(), "helper function: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_texProjOffset("),
                    "textureProjOffset trong ham helper: phai doi thanh tglmt_texProjOffset");
    }

    // ============ "Bếp nồi": một shader dùng TẤT CẢ hàm của nhóm ============
    {
        std::string glsl =
            "#version 460\n"
            "uniform sampler2D s;\n"
            "uniform sampler2DShadow sh;\n"
            "void main(){\n"
            "  vec4 a = textureProj(s, vec4(0.5));\n"
            "  vec4 b = textureProjOffset(s, vec3(0.5), ivec2(1));\n"
            "  vec4 c = textureProjLod(s, vec3(0.5), 0.0);\n"
            "  vec4 d = textureProjLodOffset(s, vec3(0.5), 0.0, ivec2(1));\n"
            "  vec4 e = textureGatherOffsets(s, vec2(0.5), ivec2(1), ivec2(2), ivec2(3), ivec2(4));\n"
            "  vec4 f = textureProj(sh, vec4(0.5));\n"
            "  vec4 g = vec4(noise1(vec3(0.5)));\n"
            "  gl_FragColor = a+b+c+d+e+f+g;\n"
            "}\n";
        std::string msl = ToMsl(glsl, kFs);
        TGLMT_CHECK(!msl.empty(), "kitchen sink: convert phai thanh cong");
        for (const char* want : {"tglmt_texProjOffset(", "tglmt_texProjLodOffset(",
                                 "tglmt_textureGatherOffsets(", "tglmt_noise1(",
                                 "sample_compare("}) {
            if (!Has(msl, want)) {
                fprintf(stderr, "thieu mau: %s\n", want);
                TGLMT_CHECK(false, "kitchen sink: phai co tat ca dang MSL");
            }
        }
        TGLMT_CHECK(ParensBalanced(msl), "kitchen sink: MSL phai co ngoac can bang");
    }

    return TGLMT_RESULT;
}
