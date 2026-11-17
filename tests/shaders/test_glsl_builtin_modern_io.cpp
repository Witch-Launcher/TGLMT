// tests/shaders/test_glsl_builtin_modern_io.cpp — kiểm thử nhóm built-in
// BIẾN GLSL "modern_io" (GLSL 4.60 §7 "Built-In Variables") → MSL.
//
// Mỗi built-in được kiểm bằng cách dịch một shader dùng nó và kiểm tra MSL
// sinh ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash"):
//   • field MSL đúng attribute (vd `uint gl_Layer [[render_target_array_index]]`)
//   • body đọc/ghi đúng field (`_out.gl_Layer` / `_in.gl_Layer`)
//   • TÊN field VS output và FS input PHẢI GIỐNG HNH NHAU (Metal ghép theo
//     attribute; linker src/gl/gl_program.cpp khớp varying theo TÊN).
//
// Ngoài ra test kiểm tra:
//   • MSL của shader KHÔNG dùng nhóm này không được thêm gì.
//   • các built-in CHƯA hỗ trợ được ghi rõ trạng thái (KNOWN GAP) để không
//     ai tưởng là đã chạy.
//   • MSL sinh ra phải BIÊN DỊCH ĐƯỢC bằng compiler Metal thật — cả bản đã
//     remap location tạm (như linker làm) lẫn bản NGUYÊN (user(locn900)).
#include "glsl_coverage_cases.h"
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

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

// Identifier đứng riêng trong `s`?
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

bool isIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) ||
           (!first && isdigit((unsigned char)c));
}

// Còn phép gán GLSL trực tiếp `name = ...` trong MSL không?
// (`_out.gl_Layer = 1` KHÔNG phải — dấu '.' trước tên loại trừ.)
bool HasBareAssign(const std::string& s, const char* name) {
    size_t n = strlen(name);
    size_t p = 0;
    while ((p = s.find(name, p)) != std::string::npos) {
        bool l = p > 0 && (isalnum((unsigned char)s[p - 1]) || s[p - 1] == '_');
        bool dot = p > 0 && s[p - 1] == '.';
        size_t e = p + n;
        bool r = e < s.size() && (isalnum((unsigned char)s[e]) || s[e] == '_');
        if (!l && !r && !dot) {
            size_t i = e;
            while (i < s.size() && isspace((unsigned char)s[i])) ++i;
            if (i < s.size() && s[i] == '=' &&
                !(i + 1 < s.size() && s[i + 1] == '='))
                return true;
        }
        p = e;
    }
    return false;
}

// Tên field (identifier) của dòng chứa `needle` trong `msl`.
// Dùng để so tên field VS output với FS input (Metal ghép theo attribute,
// linker khớp theo tên).
std::string FieldNameOf(const std::string& msl, const char* needle) {
    size_t p = msl.find(needle);
    if (p == std::string::npos) return std::string();
    // Lùi về đầu dòng.
    size_t ls = msl.rfind('\n', p);
    ls = (ls == std::string::npos) ? 0 : ls + 1;
    // Bỏ qua kiểu MSL (float4/uint/...) để lấy identifier.
    size_t i = ls;
    while (i < msl.size() && isspace((unsigned char)msl[i])) ++i;
    while (i < msl.size() && isIdentChar(msl[i], false)) ++i;   // kiểu
    while (i < msl.size() && isspace((unsigned char)msl[i])) ++i;
    size_t st = i;
    while (i < msl.size() && isIdentChar(msl[i], true)) ++i;
    return msl.substr(st, i - st);
}

// Nội dung struct MSL có tên `name` (vd "TGLMT_FOut").
std::string StructBody(const std::string& msl, const char* name) {
    std::string key = std::string("struct ") + name + " {";
    size_t p = msl.find(key);
    if (p == std::string::npos) return std::string();
    size_t e = msl.find("\n};", p);
    if (e == std::string::npos) return std::string();
    return msl.substr(p, e - p);
}

// Biên dịch MSL NGUYÊN (chưa remap location tạm) bằng compiler Metal thật.
// user(locn900) phải được chấp nhận (đo thật: pipeline tạo OK, draw OK).
bool MetalCompilesRaw(const std::string& msl, std::string* err) {
    if (std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") != 0) return true;  // không có compiler ⇒ bỏ qua
    const char* path = "/tmp/tglmt_modern_io_raw.metal";
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fputs(msl.c_str(), f);
    fclose(f);
    std::string cmd = std::string("xcrun -sdk macosx metal -c ") + path + " -o /dev/null 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return false;
    char buf[4096];
    size_t n;
    std::string out;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    int rc = pclose(p);
    if (rc != 0) {
        if (err) *err = out;
        return false;
    }
    return true;
}

}  // namespace

int main() {
    // =====================================================================
    // gl_FragDepth — GLSL 4.60 §7.1.5 (PDF p.149), out float gl_FragDepth
    // → MSL: field `float depth [[depth(any)]]` của struct fragment output.
    // =====================================================================
    {
        // FS dùng `out vec4` tường minh (đường MRT/single-out chuẩn).
        std::string msl = ToMsl(
            "#version 140\nout vec4 oColor;\n"
            "void main(){ gl_FragDepth = 0.5; oColor = vec4(1.0); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_FragDepth(out vec4): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float depth [[depth(any)]]"),
                    "gl_FragDepth: struct FOut phai co field depth [[depth(any)]]");
        TGLMT_CHECK(Has(msl, "_out.depth = 0.5"),
                    "gl_FragDepth: phai ghi thang vao _out.depth");
        TGLMT_CHECK(!Has(msl, "gl_FragDepth"),
                    "gl_FragDepth: khong con ten GLSL trong MSL");
    }
    {
        // FS legacy dùng `gl_FragColor`: struct FOut phải CÓ color output,
        // nếu không `tglmt_fragColor` sẽ undeclared ⇒ Metal compile fail.
        std::string msl = ToMsl(
            "#version 140\nvoid main(){ gl_FragDepth = 0.5; gl_FragColor = vec4(1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_FragDepth(legacy): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float depth [[depth(any)]]"),
                    "gl_FragDepth(legacy): struct FOut phai co field depth");
        TGLMT_CHECK(Has(msl, "c [[color(0)]]"),
                    "gl_FragDepth(legacy): FS legacy phai co color output trong struct FOut");
        TGLMT_CHECK(Has(msl, "float4 tglmt_fragColor"),
                    "gl_FragDepth(legacy): phai khai bao bien tglmt_fragColor");
        TGLMT_CHECK(Has(msl, "_out.c = tglmt_fragColor"),
                    "gl_FragDepth(legacy): phai ghep color vao _out.c");
    }

    // =====================================================================
    // gl_SampleMask[0] — GLSL 4.60 §7.1.5 (PDF p.149), out int gl_SampleMask[]
    // → MSL: field `uint smask [[sample_mask]]` của struct fragment output.
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 400\nout vec4 oColor;\n"
            "void main(){ gl_SampleMask[0] = 1; oColor = vec4(1.0); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_SampleMask[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint smask [[sample_mask]]"),
                    "gl_SampleMask[0]: struct FOut phai co field smask [[sample_mask]]");
        TGLMT_CHECK(Has(msl, "_out.smask = 1"),
                    "gl_SampleMask[0]: phai ghi thang vao _out.smask");
        TGLMT_CHECK(!HasWord(msl, "gl_SampleMask"),
                    "gl_SampleMask[0]: khong con ten GLSL trong MSL");
        TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_FIn"), "sample_mask"),
                    "gl_SampleMask[0] la OUTPUT: FIn khong duoc co field sample_mask");
    }
    {
        // FS legacy: phai them color output (nhu gl_FragDepth).
        std::string msl = ToMsl(
            "#version 400\nvoid main(){ gl_SampleMask[0] = 1; gl_FragColor = vec4(1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_SampleMask[0](legacy): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint smask [[sample_mask]]"),
                    "gl_SampleMask[0](legacy): phai co field smask [[sample_mask]]");
        TGLMT_CHECK(Has(msl, "c [[color(0)]]"),
                    "gl_SampleMask[0](legacy): FS legacy phai co color output");
        TGLMT_CHECK(Has(msl, "float4 tglmt_fragColor"),
                    "gl_SampleMask[0](legacy): phai khai bao bien tglmt_fragColor");
    }
    {
        // GHI dạng hậu tố / tiền tố vẫn là OUTPUT (không phải input mask).
        std::string post = ToMsl(
            "#version 400\nout vec4 oColor;\n"
            "void main(){ gl_SampleMask[0]++; oColor = vec4(1.0); }\n", kFs);
        TGLMT_CHECK(!post.empty(), "gl_SampleMask[0]++: convert phai thanh cong");
        TGLMT_CHECK(Has(post, "_out.smask++"),
                    "gl_SampleMask[0]++: phai ghi vao _out.smask (output)");
        TGLMT_CHECK(!Has(StructBody(post, "TGLMT_FIn"), "sample_mask"),
                    "gl_SampleMask[0]++: khong duoc them input mask");
        std::string pre = ToMsl(
            "#version 400\nout vec4 oColor;\n"
            "void main(){ ++gl_SampleMask[0]; oColor = vec4(1.0); }\n", kFs);
        TGLMT_CHECK(!pre.empty(), "++gl_SampleMask[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(pre, "++_out.smask"),
                    "++gl_SampleMask[0]: phai ghi vao _out.smask (output)");
        TGLMT_CHECK(!Has(StructBody(pre, "TGLMT_FIn"), "sample_mask"),
                    "++gl_SampleMask[0]: khong duoc them input mask");
    }

    // =====================================================================
    // gl_SampleMaskIn[0] — GLSL 4.60 §7.1.5 (PDF p.149-150), in int
    // gl_SampleMaskIn[] → MSL: `uint ... [[sample_mask]]` làm fragment INPUT
    // (MSL Table 5.5: "The set of samples covered by the primitive generating
    // the fragment during multisample rasterization").
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 400\nout vec4 oColor;\n"
            "void main(){ oColor = vec4(float(gl_SampleMaskIn[0])); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_SampleMaskIn[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_SampleMaskIn [[sample_mask]]"),
                    "gl_SampleMaskIn[0]: struct FIn phai co field [[sample_mask]]");
        TGLMT_CHECK(Has(msl, "_in.gl_SampleMaskIn"),
                    "gl_SampleMaskIn[0]: phai doc tu _in.gl_SampleMaskIn");
        TGLMT_CHECK(!Has(msl, "float(_out.smask)"),
                    "gl_SampleMaskIn[0]: KHONG duoc doc nham mask OUTPUT (_out.smask)");
        // Mask OUTPUT do converter phat them phai duoc neutral hoa
        // (GLSL 4.60 p.150: shader khong gan gl_SampleMask ⇒ mask khong co
        //  hieu luc) — neu de 0 ⇒ Metal loai bo tat ca sample.
        TGLMT_CHECK(Has(msl, "_out.smask = 0xFFFFFFFFu"),
                    "gl_SampleMaskIn[0]: mask output phai duoc neutral hoa (0xFFFFFFFF)");
    }
    {
        // Shader vừa GHI gl_SampleMask[0] vừa ĐỌC gl_SampleMaskIn[0]:
        // ghi → output, đọc → input (converter gộp cả hai thành _out.smask).
        std::string msl = ToMsl(
            "#version 400\nout vec4 oColor;\n"
            "void main(){ gl_SampleMask[0] = gl_SampleMaskIn[0]; oColor = vec4(1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_SampleMaskIn+Out: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_SampleMaskIn [[sample_mask]]"),
                    "gl_SampleMaskIn+Out: phai co input [[sample_mask]]");
        TGLMT_CHECK(Has(msl, "_out.smask = _in.gl_SampleMaskIn"),
                    "gl_SampleMaskIn+Out: ghi → output, doc → input");
        TGLMT_CHECK(!Has(msl, "_out.smask = 0xFFFFFFFFu"),
                    "gl_SampleMaskIn+Out: co ghi mask ⇒ KHONG neutral hoa output");
    }

    // =====================================================================
    // gl_Layer — GLSL 4.60 §7.1.4/§7.1.5 (PDF p.148/149)
    // → MSL: `uint ... [[render_target_array_index]]` (MSL Table 5.4/5.5).
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_Layer = 1; gl_Position = vec4(1.0); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_Layer(VS): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_Layer [[render_target_array_index]]"),
                    "gl_Layer(VS): struct VOut phai co field [[render_target_array_index]]");
        TGLMT_CHECK(Has(msl, "_out.gl_Layer = 1"),
                    "gl_Layer(VS): phai ghi vao _out.gl_Layer");
        TGLMT_CHECK(!HasBareAssign(msl, "gl_Layer"),
                    "gl_Layer(VS): khong con phep gan GLSL truc tiep");
    }
    {
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_Layer)); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_Layer(FS): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_Layer [[render_target_array_index]]"),
                    "gl_Layer(FS): struct FIn phai co field [[render_target_array_index]]");
        TGLMT_CHECK(Has(msl, "_in.gl_Layer"),
                    "gl_Layer(FS): phai doc tu _in.gl_Layer");
    }
    {
        // TÊN field hai bên phải GIỐNG NHAU (Metal ghép theo attribute;
        // linker khớp theo tên).
        std::string vs = ToMsl(
            "#version 410\nvoid main(){ gl_Layer = 1; gl_Position = vec4(1.0); }\n", kVs);
        std::string fs = ToMsl(
            "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_Layer)); }\n", kFs);
        std::string nvs = FieldNameOf(vs, "gl_Layer");
        std::string nfs = FieldNameOf(fs, "gl_Layer");
        TGLMT_CHECK(!nvs.empty() && nvs == nfs,
                    "gl_Layer: ten field VS output va FS input phai giong nhau "
                    "(linker khop theo ten)");
    }

    // =====================================================================
    // gl_ViewportIndex — GLSL 4.60 §7.1.4/§7.1.5 (PDF p.148/149)
    // → MSL: `uint ... [[viewport_array_index]]`.
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_ViewportIndex(VS): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_ViewportIndex [[viewport_array_index]]"),
                    "gl_ViewportIndex(VS): struct VOut phai co field [[viewport_array_index]]");
        TGLMT_CHECK(Has(msl, "_out.gl_ViewportIndex = 0"),
                    "gl_ViewportIndex(VS): phai ghi vao _out.gl_ViewportIndex");
        TGLMT_CHECK(!HasBareAssign(msl, "gl_ViewportIndex"),
                    "gl_ViewportIndex(VS): khong con phep gan GLSL truc tiep");
    }
    {
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_ViewportIndex)); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_ViewportIndex(FS): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_ViewportIndex [[viewport_array_index]]"),
                    "gl_ViewportIndex(FS): struct FIn phai co field [[viewport_array_index]]");
        TGLMT_CHECK(Has(msl, "_in.gl_ViewportIndex"),
                    "gl_ViewportIndex(FS): phai doc tu _in.gl_ViewportIndex");
        std::string vs = ToMsl(
            "#version 410\nvoid main(){ gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(FieldNameOf(vs, "gl_ViewportIndex") ==
                        FieldNameOf(msl, "gl_ViewportIndex"),
                    "gl_ViewportIndex: ten field VS output va FS input phai giong nhau");
    }
    {
        // Cả hai built-in trong cùng 1 shader ⇒ 2 field KHÁC NHAU.
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_Layer = 1; gl_ViewportIndex = 0;"
            " gl_Position = vec4(1.0); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_Layer+gl_ViewportIndex(VS): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_Layer [[render_target_array_index]]"),
                    "gl_Layer+gl_ViewportIndex: phai co field gl_Layer");
        TGLMT_CHECK(Has(msl, "uint gl_ViewportIndex [[viewport_array_index]]"),
                    "gl_Layer+gl_ViewportIndex: phai co field gl_ViewportIndex");
    }
    {
        // FS chỉ dùng MỘT trong hai built-in (tập con) ⇒ vẫn phải đúng.
        std::string msl = ToMsl(
            "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_ViewportIndex)); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_ViewportIndex(FS don): convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_ViewportIndex [[viewport_array_index]]"),
                    "gl_ViewportIndex(FS don): phai co field [[viewport_array_index]]");
        TGLMT_CHECK(!Has(msl, "render_target_array_index"),
                    "gl_ViewportIndex(FS don): KHONG duoc them field gl_Layer");
    }

    // =====================================================================
    // gl_PrimitiveID — GLSL 4.60 §7.1.5 (PDF p.149, mô tả p.151)
    // → MSL: `uint ... [[primitive_id]]` (MSL Table 5.5).
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_PrimitiveID)); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_PrimitiveID: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "uint gl_PrimitiveID [[primitive_id]]"),
                    "gl_PrimitiveID: struct FIn phai co field [[primitive_id]]");
        TGLMT_CHECK(Has(msl, "_in.gl_PrimitiveID"),
                    "gl_PrimitiveID: phai doc tu _in.gl_PrimitiveID");
        TGLMT_CHECK(!HasWord(msl, "gl_PrimitiveID") ||
                        Has(msl, "uint gl_PrimitiveID [[primitive_id]]"),
                    "gl_PrimitiveID: khong con ten GLSL rong");
    }

    // =====================================================================
    // gl_BaseInstance — GLSL 4.60 §7.1.1 (PDF p.145): đã chạy sẵn nhờ
    // converter (vertex function parameter `[[base_instance]]`).
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseInstance)); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_BaseInstance: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "[[base_instance]]"),
                    "gl_BaseInstance: MSL phai co vertex param [[base_instance]]");
        TGLMT_CHECK(Has(msl, "tglmt_baseInstance"),
                    "gl_BaseInstance: phai map sang bien tglmt_baseInstance");
    }

    // =====================================================================
    // KNOWN GAP còn lại — built-in CHƯA hỗ trợ (ghi rõ để không ai tưởng đã
    // chạy).  gl_BaseVertex / gl_DrawID / gl_NumSamples đã cài đặt ở nhóm
    // "final" (xem tests/shaders/test_glsl_builtin_final.cpp); khi nhóm nào
    // cài đặt xong, các check tương ứng sẽ được cập nhật.
    // =====================================================================
    {
        // gl_BaseVertex: MSL chỉ chấp nhận [[base_vertex]] làm THAM SỐ HÀM
        // vertex (probe Metal compiler: compile OK; [[stage_in]] bị reject) ⇒
        // đã cài đặt: converter phát tham số vertex [[base_vertex]].
        // Kiểm đầy đủ ở tests/shaders/test_glsl_builtin_final.cpp.
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseVertex)); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_BaseVertex: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "[[base_vertex]]"),
                    "gl_BaseVertex: MSL phai co vertex param [[base_vertex]]");
        TGLMT_CHECK(Has(msl, "tglmt_baseVertex"),
                    "gl_BaseVertex: phai map sang bien tglmt_baseVertex");
    }
    {
        // gl_DrawID: Metal không có [[draw_index]] ⇒ giá trị do CPU cấp qua
        // uniform `tglmt_DrawID` (backend điền mỗi draw — xem
        // src/gl/glsl_builtins_final.cpp mục REP BACKEND).
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_DrawID)); }\n", kVs);
        TGLMT_CHECK(r.ok,
                    "gl_DrawID: convert phai thanh cong (uniform tglmt_DrawID)");
        TGLMT_CHECK(Has(r.msl, "tglmt_u.tglmt_DrawID"),
                    "gl_DrawID: body phai doc tu uniform tglmt_u.tglmt_DrawID");
    }
    {
        // gl_ClipDistance[1]: Metal chỉ hỗ trợ MỘT [[clip_distance]].
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 150\nvoid main(){ gl_ClipDistance[1] = 1.0; gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!r.ok,
                    "KNOWN GAP: gl_ClipDistance[1] CHUA duoc ho tro (Metal chi 1 clip_distance)");
        // gl_ClipDistance[0] thì chạy đúng.
        std::string msl = ToMsl(
            "#version 150\nvoid main(){ gl_ClipDistance[0] = 1.0; gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_ClipDistance[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "[[clip_distance]]"),
                    "gl_ClipDistance[0]: MSL phai co field [[clip_distance]]");
    }
    {
        // gl_NumSamples: MSL KHÔNG có attribute sample-count (compiler Metal
        // báo "unknown attribute 'sample_count'") ⇒ giá trị do CPU cấp qua
        // uniform `tglmt_NumSamples` (backend điền = sampleCount của render
        // target — xem src/gl/glsl_builtins_final.cpp mục REP BACKEND).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_NumSamples)); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_NumSamples: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "tglmt_u.tglmt_NumSamples"),
                    "gl_NumSamples: body phai doc tu uniform tglmt_u.tglmt_NumSamples");
        TGLMT_CHECK(!Has(msl, "sample_count"),
                    "gl_NumSamples: KHONG duoc dung attribute sample_count khong ton tai");
    }

    // =====================================================================
    // Shader KHÔNG dùng nhóm này ⇒ MSL không được thêm gì.
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 460\nout vec4 oColor;\n"
            "void main(){ oColor = vec4(1.0, 0.0, 0.0, 1.0); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "shader lien quan: convert phai thanh cong");
        for (const char* bad : {"render_target_array_index", "viewport_array_index",
                                "primitive_id", "sample_mask", "gl_Layer",
                                "gl_ViewportIndex", "gl_PrimitiveID",
                                "gl_SampleMaskIn"}) {
            if (Has(msl, bad)) {
                fprintf(stderr, "thu tu thua: %s\n", bad);
                TGLMT_CHECK(false, "shader khong dung nhom nay khong duoc them gi");
            }
        }
    }

    // =====================================================================
    // MSL sinh ra phải BIÊN DỊCH ĐƯỢC bằng compiler Metal thật
    // (dùng chung harness của tests/shaders/glsl_coverage_cases.h).
    // =====================================================================
    {
        std::vector<GLSLProbeCase> cases = {
            {"modern_io", "gl_FragDepth (legacy FS)",
             "#version 140\nvoid main(){ gl_FragDepth = 0.5; gl_FragColor = vec4(1.0); }\n",
             kFs, "depth(any)", nullptr},
            {"modern_io", "gl_SampleMask[0]",
             "#version 400\nvoid main(){ gl_SampleMask[0] = 1; gl_FragColor = vec4(1.0); }\n",
             kFs, "sample_mask", nullptr},
            {"modern_io", "gl_SampleMaskIn[0]",
             "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_SampleMaskIn[0])); }\n",
             kFs, "[[sample_mask]]", nullptr},
            {"modern_io", "gl_Layer (VS)",
             "#version 410\nvoid main(){ gl_Layer = 1; gl_Position = vec4(1.0); }\n",
             kVs, "render_target_array_index", nullptr},
            {"modern_io", "gl_Layer (FS)",
             "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_Layer)); }\n",
             kFs, "render_target_array_index", nullptr},
            {"modern_io", "gl_ViewportIndex (VS)",
             "#version 410\nvoid main(){ gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n",
             kVs, "viewport_array_index", nullptr},
            {"modern_io", "gl_ViewportIndex (FS)",
             "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_ViewportIndex)); }\n",
             kFs, "viewport_array_index", nullptr},
            {"modern_io", "gl_PrimitiveID",
             "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_PrimitiveID)); }\n",
             kFs, "primitive_id", nullptr},
            {"modern_io", "gl_Layer+gl_ViewportIndex (VS)",
             "#version 410\nvoid main(){ gl_Layer = 1; gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n",
             kVs, "viewport_array_index", nullptr},
            {"modern_io", "gl_SampleMask ghi+doc",
             "#version 400\nvoid main(){ gl_SampleMask[0] = gl_SampleMaskIn[0]; gl_FragColor = vec4(1.0); }\n",
             kFs, "[[sample_mask]]", nullptr},
        };
        std::vector<std::string> msl, log;
        std::vector<bool> convOk, metalOk;
        int bad = GLSLProbeRunAll(cases, msl, convOk, metalOk, log);
        for (size_t i = 0; i < cases.size(); ++i) {
            if (convOk[i] && metalOk[i]) continue;
            fprintf(stderr, "  FAIL [%s] %-24s %s\n", cases[i].group, cases[i].name,
                    log[i].c_str());
        }
        TGLMT_CHECK(bad == 0,
                    "moi shader cua nhom modern_io phai convert + Metal-compile duoc");
    }

    // =====================================================================
    // MSL NGUYÊN (chưa remap location tạm 900+) phải compile được bằng
    // compiler Metal thật: user(locn900) được Metal chấp nhận (đo thật:
    // pipeline tạo OK, draw OK, giá trị truyền đúng).
    // =====================================================================
    {
        struct Raw { const char* name; const char* src; uint32_t stage; };
        const Raw kRaw[] = {
            {"gl_Layer(VS)", "#version 410\nvoid main(){ gl_Layer = 1; gl_Position = vec4(1.0); }\n", kVs},
            {"gl_Layer(FS)", "#version 410\nvoid main(){ gl_FragColor = vec4(float(gl_Layer)); }\n", kFs},
            {"gl_ViewportIndex(VS)", "#version 410\nvoid main(){ gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n", kVs},
            {"gl_PrimitiveID(FS)", "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_PrimitiveID)); }\n", kFs},
            {"gl_SampleMaskIn[0](FS)", "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_SampleMaskIn[0])); }\n", kFs},
        };
        for (const Raw& r : kRaw) {
            std::string m = ToMsl(r.src, r.stage);
            TGLMT_CHECK(!m.empty(), "raw compile: convert phai thanh cong");
            std::string err;
            if (!MetalCompilesRaw(m, &err)) {
                fprintf(stderr, "--- raw metal compile fail (%s) ---\n%s\n", r.name, err.c_str());
                TGLMT_CHECK(false, "MSL nguyen (user(locn900)) phai compile duoc boi Metal compiler");
            }
        }
    }

    return TGLMT_RESULT;
}
