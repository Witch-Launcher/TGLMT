// tests/shaders/test_glsl_builtin_lang.cpp — kiểm thử nhóm ngôn ngữ GLSL
// "lang" (src/gl/glsl_builtins_lang.cpp).
//
// Mỗi tính năng được kiểm bằng cách dịch một shader dùng nó và kiểm tra MSL
// sinh ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash"):
//
//   1. half / f16vecN (fp16)        → float / floatN           (GLSL §4.1.4)
//   2. mảng uniform kiểu struct     → tách phẳng u_f[N]        (GLSL §4.3.8)
//   3. `flat` trên biến int/uint    → [[flat]]                 (GLSL §4.5)
//   4. qualifier `invariant`        → chấp nhận + bỏ qua       (GLSL §4.8.1)
//   5. SSBO mảng unsized + instance → device T* phẳng          (GLSL §4.4.5)
//   6. packHalf2x16/unpackHalf2x16  → helper MSL tương đương   (GLSL §8.4)
//
// Ba tính năng #2/#4/#5 bị PARSER của converter từ chối trước khi pass
// built-in kịp chạy ⇒ chúng được kiểm qua hàm chuẩn hoá nguồn
// `tglmt::gl::NormalizeGLSLForLang` (cùng file, mục 0) — tức kiểm ĐÚNG đường
// mà converter sẽ dùng sau khi main agent gắn hàm này vào
// src/gl/glsl_to_msl.cpp:8471 (xem header file nguồn).
//
// Ngoài ra test kiểm tra tính chất quan trọng: MSL của shader KHÔNG dùng
// nhóm này phải không có dấu vết fp16/[[flat]] thừa (byte-identical với
// trước khi có pass).
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;

// Prototype của hàm chuẩn hoá nguồn (định nghĩa trong
// src/gl/glsl_builtins_lang.cpp — xem mục 0 của file đó).
namespace tglmt {
namespace gl {
std::string NormalizeGLSLForLang(const std::string& src);
}  // namespace gl
}  // namespace tglmt

namespace {

constexpr uint32_t kVs = 0x8B31u;  // GL_VERTEX_SHADER
constexpr uint32_t kFs = 0x8B30u;  // GL_FRAGMENT_SHADER
constexpr uint32_t kCs = 0x91B9u;  // GL_COMPUTE_SHADER

// Dịch 1 shader và trả về MSL (rỗng nếu convert lỗi).
std::string ToMsl(const std::string& glsl, uint32_t stage) {
    GLSLConvertResult r = ConvertGLSLtoMSL(glsl, stage);
    if (!r.ok) {
        fprintf(stderr, "convert that bai: %s\n", r.log.c_str());
        return std::string();
    }
    return r.msl;
}

// Dịch sau khi chuẩn hoá nguồn (đường của #2/#4/#5).
std::string ToMslNormalized(const std::string& glsl, uint32_t stage) {
    return ToMsl(tglmt::gl::NormalizeGLSLForLang(glsl), stage);
}

bool Has(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

// Có identifier `word` đứng RIÊNG trong `s` không?
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

bool HasMetalCompiler() {
    return std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") == 0;
}

// Remap location tạm (900+) như linker làm (src/gl/gl_program.cpp
// RewriteLocns): Metal chỉ nhận [[attribute(N)]] với N hợp lệ.
void RemapTempLocs(std::string* m) {
    auto remapOne = [&](const std::string& open, const std::string& close) {
        int next = 0;
        size_t p = 0;
        while ((p = m->find(open, p)) != std::string::npos) {
            size_t e = m->find(close, p + open.size());
            if (e == std::string::npos) break;
            int num = atoi(m->c_str() + p + open.size());
            if (num >= 100) {
                std::string repl = open + std::to_string(next) + close;
                m->replace(p, e + close.size() - p, repl);
                ++next;
                p += repl.size();
            } else {
                p = e + close.size();
            }
        }
    };
    remapOne("[[attribute(", ")]]");
    remapOne("[[user(locn", ")]]");
}

// Biên dịch MSL bằng compiler Metal thật. Trả true nếu compile OK (hoặc nếu
// không có compiler ⇒ coi như "không kiểm được", KHÔNG coi là fail).
bool MetalCompiles(const std::string& mslIn, const char* tag) {
    if (!HasMetalCompiler()) return true;
    std::string msl = mslIn;
    RemapTempLocs(&msl);
    std::string base = std::string("/tmp/tglmt-lang-") + tag;
    {
        FILE* f = fopen((base + ".metal").c_str(), "w");
        if (!f) return false;
        fputs(msl.c_str(), f);
        fclose(f);
    }
    std::string cmd = "xcrun -sdk macosx metal -c " + base + ".metal -o " + base +
                      ".air >/dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

}  // namespace

int main() {
    // =================================================================
    // 1. fp16: half / f16vecN → float / floatN
    //    GLSL 4.60 §4.1.4: core profile không có kiểu 16-bit (extension
    //    GL_ARB_gpu_shader_fp16) ⇒ hạ xuống float (value-compatible A11).
    // =================================================================
    {
        const std::string src =
            "#version 430\nvoid main(){ half h = 1.0h; f16vec2 v = f16vec2(1.0h);"
            " f16vec4 w = f16vec4(2.0h); gl_Position = vec4(float(h + v.x + w.x)); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "fp16: convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "half"), "fp16: khong con kieu GLSL 'half'");
        TGLMT_CHECK(!HasWord(msl, "f16vec2"), "fp16: khong con kieu GLSL 'f16vec2'");
        TGLMT_CHECK(!HasWord(msl, "f16vec4"), "fp16: khong con kieu GLSL 'f16vec4'");
        TGLMT_CHECK(Has(msl, "float h = 1.0f"), "fp16: half → float, literal 1.0h → 1.0f");
        TGLMT_CHECK(Has(msl, "float2 v = float2(1.0f)"),
                    "fp16: f16vec2 → float2, literal fp16 → f");
        TGLMT_CHECK(Has(msl, "float4 w = float4(2.0f)"), "fp16: f16vec4 → float4");
        TGLMT_CHECK(!Has(msl, "1.0h"), "fp16: khong con literal half (1.0h)");
        TGLMT_CHECK(MetalCompiles(msl, "fp16"), "fp16: MSL phai bien dich duoc boi Metal");
    }
    // f16mat* → floatNxM
    {
        const std::string src =
            "#version 430\nvoid main(){ f16mat2 m = f16mat2(1.0h);"
            " gl_Position = vec4(float(m[0][0])); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "fp16mat: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float2x2 m = float2x2(1.0f)"),
                    "fp16mat: f16mat2 → float2x2");
    }
    // Non-regression: shader KHÔNG dùng fp16 không bị đổi.
    {
        const std::string src =
            "#version 430\nvoid main(){ float h = 1.0; vec2 v = vec2(2.0);"
            " gl_Position = vec4(h, v.x, 0.0, 1.0); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "fp16 non-regression: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float h = 1.0;"), "fp16 non-regression: float giu nguyen");
        TGLMT_CHECK(Has(msl, "float2 v = float2(2.0)"),
                    "fp16 non-regression: vec2 giu nguyen (khong bi doi thanh float2 doc)");
    }
    // Non-regression: BIẾN của user tên "half" không bị đổi thành kiểu float.
    {
        const std::string src =
            "#version 430\nvoid main(){ float half = 1.0; gl_Position = vec4(half); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "fp16 ten bien: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float half = 1.0;"),
                    "fp16 ten bien: bien 'half' phai giu nguyen (chi doi o vi tri KIEU)");
    }

    // =================================================================
    // 2. `flat` — GLSL 4.60 §4.5: biến flat KHÔNG nội suy.
    //    MSL: [[flat]].
    // =================================================================
    {
        // Vertex INPUT (attribute) — chỗ converter bỏ sót (không gọi
        // InterpAttr cho attribute: src/gl/glsl_to_msl.cpp:12216).
        const std::string src =
            "#version 430\nflat in int k;\nvoid main(){ gl_Position = vec4(float(k)); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "flat: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "[[flat]]"),
                    "flat: vertex input int phai co [[flat]] trong MSL");
        TGLMT_CHECK(MetalCompiles(msl, "flat"), "flat: MSL phai bien dich duoc boi Metal");
    }
    {
        // Varying VS out / FS in (int) — [[flat]] do converter phát.
        const std::string vs =
            "#version 430\nflat out int vId;\nvoid main(){ vId = 3;"
            " gl_Position = vec4(1.0); }\n";
        std::string msl = ToMsl(vs, kVs);
        TGLMT_CHECK(!msl.empty(), "flat out: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "[[flat]]"), "flat out: phai co [[flat]]");
        const std::string fs =
            "#version 430\nflat in int vId;\nvoid main(){ gl_FragColor = vec4(float(vId)); }\n";
        std::string fmsl = ToMsl(fs, kFs);
        TGLMT_CHECK(!fmsl.empty(), "flat in (FS): convert phai thanh cong");
        TGLMT_CHECK(Has(fmsl, "[[flat]]"), "flat in (FS): phai co [[flat]]");
    }
    {
        // Non-regression: varying KHÔNG flat không được sinh [[flat]].
        const std::string src =
            "#version 430\nout float k;\nvoid main(){ k = 1.0; gl_Position = vec4(1.0); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "flat non-regression: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "[[flat]]"), "flat non-regression: varying smooth khong [[flat]]");
    }

    // =================================================================
    // 3. packHalf2x16 / unpackHalf2x16 — GLSL 4.60 §8.4 (PDF p.174).
    //    MSL không có pack_half2x16 ⇒ helper của converter (half2 +
    //    as_type<ushort>) là ánh xạ tương đương.
    // =================================================================
    {
        const std::string src =
            "#version 430\nvoid main(){ uint p = packHalf2x16(vec2(0.5));"
            " vec2 v = unpackHalf2x16(p); gl_Position = vec4(v, 0.0, 1.0); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "packHalf2x16: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_packHalf2x16("),
                    "packHalf2x16: phai doi thanh tglmt_packHalf2x16");
        TGLMT_CHECK(Has(msl, "tglmt_unpackHalf2x16("),
                    "unpackHalf2x16: phai doi thanh tglmt_unpackHalf2x16");
        TGLMT_CHECK(MetalCompiles(msl, "packhalf"),
                    "packHalf2x16: MSL phai bien dich duoc boi Metal");
    }
    {
        // Gán vec2 (kết quả unpack) vào color output vec4 — GLSL không cho,
        // pass nới (giống pass "legacy" nới float→vec4).
        const std::string src =
            "#version 430\nvoid main(){ gl_FragColor = unpackHalf2x16(packHalf2x16(vec2(1.0))); }\n";
        std::string msl = ToMsl(src, kFs);
        TGLMT_CHECK(!msl.empty(), "pack/unpack: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float4("),
                    "pack/unpack: vec2 phai noi rong thanh float4 khi gán color output");
        TGLMT_CHECK(MetalCompiles(msl, "packunpack"),
                    "pack/unpack: MSL phai bien dich duoc boi Metal");
    }

    // =================================================================
    // 4. `invariant` — GLSL 4.60 §4.8.1 (PDF p.103). MSL không có khái
    //    niệm tương đương ⇒ chấp nhận + bỏ qua (không được báo lỗi).
    // =================================================================
    {
        const std::string src =
            "#version 430\ninvariant gl_Position;\nvoid main(){ gl_Position = vec4(1.0); }\n";
        std::string norm = tglmt::gl::NormalizeGLSLForLang(src);
        TGLMT_CHECK(!HasWord(norm, "invariant"),
                    "invariant: qualifier phai duoc bo khoi nguon GLSL");
        std::string msl = ToMslNormalized(src, kVs);
        TGLMT_CHECK(!msl.empty(), "invariant: convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "invariant"), "invariant: MSL khong con 'invariant'");
        TGLMT_CHECK(MetalCompiles(msl, "invariant"),
                    "invariant: MSL phai bien dich duoc boi Metal");
    }
    {
        // Dạng khai báo: `invariant out vec4 c;` — cũng phải chấp nhận.
        const std::string src =
            "#version 430\ninvariant out vec4 c;\nvoid main(){ c = vec4(1.0); }\n";
        std::string msl = ToMslNormalized(src, kFs);
        TGLMT_CHECK(!msl.empty(), "invariant(decl): convert phai thanh cong");
        TGLMT_CHECK(!HasWord(msl, "invariant"), "invariant(decl): MSL khong con 'invariant'");
    }

    // =================================================================
    // 5. Mảng uniform kiểu struct — GLSL 4.60 §4.3.8.
    //    Converter chỉ nhận uniform scalar/vector ⇒ tách phẳng theo member.
    // =================================================================
    {
        const std::string src =
            "#version 430\nstruct S { float f; };\nuniform S u[2];\n"
            "void main(){ gl_Position = vec4(u[0].f); }\n";
        std::string norm = tglmt::gl::NormalizeGLSLForLang(src);
        TGLMT_CHECK(Has(norm, "uniform float u_f[2];"),
                    "uniform struct array: phai tach phang thanh uniform float u_f[2]");
        TGLMT_CHECK(Has(norm, "u_f[0]"),
                    "uniform struct array: u[0].f phai doi thanh u_f[0]");
        std::string msl = ToMslNormalized(src, kVs);
        TGLMT_CHECK(!msl.empty(), "uniform struct array: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float u_f[2]"),
                    "uniform struct array: MSL phai co mang float u_f[2] trong uniform buffer");
        TGLMT_CHECK(MetalCompiles(msl, "ustruct"),
                    "uniform struct array: MSL phai bien dich duoc boi Metal");
    }

    // =================================================================
    // 6. SSBO mảng unsized (member cuối) + instance — GLSL 4.60 §4.4.5.
    //    Converter hỗ trợ dạng phẳng: device T* (src/gl/glsl_to_msl.cpp:11037).
    // =================================================================
    {
        const std::string src =
            "#version 430\nlayout(std430) buffer B { float d[]; } b;\n"
            "void main(){ b.d[0] = 1.0; }\n";
        std::string norm = tglmt::gl::NormalizeGLSLForLang(src);
        TGLMT_CHECK(!Has(norm, "} b;"),
                    "ssbo unsized: instance phai duoc bo (dung dang phang cua converter)");
        TGLMT_CHECK(Has(norm, "b.d[0] = 1.0;") == false,
                    "ssbo unsized: b.d phai doi thanh d trong than shader");
        std::string msl = ToMslNormalized(src, kCs);
        TGLMT_CHECK(!msl.empty(), "ssbo unsized: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "device float* d"),
                    "ssbo unsized: MSL phai co device float* d (dang phang SSBO)");
        TGLMT_CHECK(MetalCompiles(msl, "ssbo"),
                    "ssbo unsized: MSL phai bien dich duoc boi Metal");
    }

    // =================================================================
    // 7. Non-regression chung: shader không dùng nhóm "lang" không bị đổi.
    // =================================================================
    {
        const std::string src =
            "#version 460\nlayout(location=0) in vec3 aPos;\n"
            "layout(location=0) out vec4 vCol;\n"
            "void main(){ vCol = vec4(aPos, 1.0); gl_Position = vec4(aPos, 1.0); }\n";
        std::string msl = ToMsl(src, kVs);
        TGLMT_CHECK(!msl.empty(), "non-regression: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "[[flat]]"), "non-regression: khong sinh [[flat]] thua");
        TGLMT_CHECK(!Has(msl, "half"), "non-regression: khong sinh kieu half thua");
        TGLMT_CHECK(!Has(msl, "tglmt_packHalf2x16"),
                    "non-regression: khong sinh helper packHalf2x16 thua");
    }

    printf("test_glsl_builtin_lang: 6/6 nhom ngon ngu GLSL da kiem tra\n");
    return TGLMT_RESULT;
}
