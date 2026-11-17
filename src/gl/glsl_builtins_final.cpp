// glsl_builtins_final.cpp — đóng nốt các gap built-in GLSL CUỐI CÙNG.
//
// Nhóm này gồm 3 việc (mục 1, 2, 3, 5 của đặc tả; mục 4 — gl_ClipDistance[1]
// — là honest error, xử lý tại converter + ghi chú test, xem cuối file):
//
//   1. gl_DrawID (VS, GLSL 4.60 §7.1.1, PDF p.143-145; OpenGL 4.6 Core
//      §11.1.3.9, glspec46.core.pdf PDF p.426):
//        in int gl_DrawID;
//      Index của lệnh draw trong danh sách MultiDraw* (0-based); với lệnh
//      MultiDraw* "the value of gl_DrawID is the zero-based draw index"; với
//      lệnh draw đơn, giá trị luôn là 0 ("In non-MultiDraw* commands, the
//      value of gl_DrawID is always zero").
//      Metal KHÔNG có `[[draw_index]]` cho vertex function (probe Metal
//      compiler: "unknown attribute 'draw_index' ignored" + "invalid type
//      'uint' (aka 'unsigned int') for input declaration in a vertex
//      function") ⇒ giá trị phải do CPU truyền vào.
//      CÁCH LÀM: thêm uniform `int tglmt_DrawID` (location -1 — linker gán)
//      vào *ctx.uniforms; body được rewrite `gl_DrawID` →
//      `tglmt_u.tglmt_DrawID`. Struct TGLMTUniforms tự có member mới (converter
//      emit theo danh sách `uniforms` SAU khi pass chạy + tính lại layout).
//      Linker (R.uniforms được đồng bộ lại sau pass, xem glsl_to_msl.cpp)
//      gán location ⇒ backend điền giá trị mỗi draw (xem REP BACKEND ở cuối
//      file — KHÔNG fake giá trị 0 trong converter).
//
//   2. gl_NumSamples (FS, GLSL 4.60 §7.4 "Built-In Uniform State", PDF p.159):
//        uniform int gl_NumSamples;
//      "These variables are only guaranteed to be available in the fragment
//      stage." Giá trị = tổng số sample của framebuffer hiện tại (1 với
//      non-MSAA), xem mô tả gl_SampleID ở PDF p.150: "gl_NumSamples is the
//      total number of samples in the framebuffer, or 1 if rendering to a
//      non-multisample framebuffer."
//      Metal KHÔNG có attribute sample-count cho fragment function (probe:
//      "unknown attribute 'sample_count' ignored"), và
//      `texture2d<float>::sample_count` KHÔNG tồn tại trên toolchain Metal
//      hiện tại (probe: "no member named 'sample_count' in
//      'metal::texture2d<float, metal::access::sample, ...>'") ⇒ KHÔNG thể
//      đọc từ texture (và cũng SAI NGỮ NGHĨA — gl_NumSamples là số sample
//      của RENDER TARGET, không phải của texture bound).
//      CÁCH LÀM: giống gl_DrawID — uniform `int tglmt_NumSamples`.
//
//   3. gl_BaseVertex (VS): xử lý tại converter (src/gl/glsl_to_msl.cpp,
//      nhánh R.isVertex — tham số `uint tglmt_baseVertex [[base_vertex]]`
//      của hàm vertex TGLMT_vs; MSL chỉ chấp nhận attribute đó làm THAM SỐ
//      HÀM vertex, không hợp lệ trong [[stage_in]]). Pass này KHÔNG đụng.
//
//   4. TCS (tessellation control) thiếu khối out gl_PerVertex:
//      GLSL 4.60 §7.1.2 (PDF p.145) — TCS intrinsically khai báo
//        out gl_PerVertex { vec4 gl_Position; float gl_PointSize;
//                           float gl_ClipDistance[]; float gl_CullDistance[]; } gl_out[];
//      (GLSL 4.0 §7.3 dùng cùng ngôn ngữ — converter hiện tại yêu cầu khai
//      báo TƯỜNG MINH ⇒ shader dùng đúng form intrinsic bị từ chối oan.)
//      CÁCH LÀM: pass MỨC NGUỒN — nếu nguồn có dấu hiệu TCS
//      (`layout(vertices = N) out;`) và CHƯA khai báo khối gl_out, chèn
//        out gl_PerVertex { vec4 gl_Position; float gl_PointSize; } gl_out[];
//      trước `void main`. (Phần thành viên chỉ gl_Position + gl_PointSize —
//      khớp subset TCS của repo: `gl_out[i].gl_ClipDistance` fail RÕ trong
//      RewriteTessControl — TCS gl_ClipDistance chưa hỗ trợ, dùng TES.)
//      Nếu không chắc chắn TCS ⇒ KHÔNG động vào nguồn (converter báo lỗi
//      rõ như trước — không sinh MSL sai).
//
// NGUYÊN TẮC
//   • Pass KHÔNG đụng src/gl/glsl_to_msl.cpp (chỉ converter sửa minimum).
//   • Shader không dùng nhóm này ⇒ MSL BYTE-IDENTICAL (chỉ thêm uniform khi
//     THẤY identifier gl_* tương ứng; gl_DrawID/gl_NumSamples là tên reserved
//     §3.6 ⇒ thấy là CHẮC CHẮN built-in).
//   • Mọi rewrite đều word-boundary (không dính vào identifier dài hơn).
//
// REP BACKEND (file + hàm + giá trị) — việc CÒN LẠI (ngoài write-scope của
// pass này) để gl_DrawID/gl_NumSamples đọc được giá trị thật:
//   • gl_DrawID: src/gl/gl_apple_draw.cpp, hàm AppleDrawGL (đoạn
//     "Uniforms tách VS/FS — IR staging cache + ring upload", ~dòng 2880-2920
//     — vòng `for (auto& e : pr.uniformLayout)` nạp buffer VS).
//     Sau vòng đó, thêm: nếu e.isVS && e.name == "tglmt_DrawID" && e.loc >= 0
//     ⇒ ghi `uint32_t drawOrdinal` vào vsb.data() + e.offset (VS-local: e.offset
//     đã là offset tuyệt đối với VS base 0). Giá trị = chỉ số 0-based của lệnh
//     draw trong MultiDraw* hiện tại; lệnh draw đơn ⇒ 0 (GL §11.1.3.9).
//     (TGLMT hiện CHƯA có glMultiDraw* ⇒ giá trị đúng cho mọi draw hiện tại
//     là 0; khi multi-draw được thêm, mỗi dispatch con phải ghi ordinal của
//     nó — KHÔNG hardcode 0: buffer VS được zero trước khi nạp.)
//   • gl_NumSamples: src/gl/gl_apple_draw.cpp, CÙNG đoạn nạp uniform, với
//     e.isVS == false; nếu e.name == "tglmt_NumSamples" && e.loc >= 0 ⇒ ghi
//     `uint32_t sampleCount` = (uint32_t)target->sampleCount()
//     (giá trị này đã có sẵn trong AppleDrawGL — target->sampleCount(), xem
//     chỗ dùng ở ~dòng 2650/4785) — tức số sample của RENDER TARGET đang
//     bind, đúng ngữ nghĩa GLSL §7.4.
//   • gl_BaseVertex: src/gl/glsl_to_msl.cpp chỉ phát tham số
//     `uint tglmt_baseVertex [[base_vertex]]` cho hàm vertex TGLMT_vs
//     (converter-emitted). BACKEND (ngoài write-scope) phải:
//       (a) src/metal/AppleMetalBridge.mm — `drawIndexedBaseInstance` đang
//           gọi `drawIndexedPrimitives:... baseVertex:0 baseInstance:` (~dòng
//           911-921): truyền baseVertex THẬT thay vì 0 (hoặc thêm API mới
//           drawIndexedBaseVertex nếu IRenderEncoder cần),
//       (b) src/gl/gl_apple_draw.cpp — hàm AppleDrawGL (draw call ~dòng
//           3790-3830): với program có MSL chứa "[[base_vertex]]", BỎ đường
//           IdxRewrite (cộng baseVertex vào index, ~dòng 3618-3770) và đi qua
//           drawIndexedPrimitives:... baseVertex:<giá trị GL> thay vào; nếu
//           vẫn rewrite index VÀ Metal cũng cộng baseVertex ⇒ đỉnh bị lệch
//           đôi. Draw non-indexed: GL báo gl_BaseVertex = 0 (GL §11.1.3.9)
//           — backend KHÔNG truyền gì, Metal trả 0 (NOT_VERIFIED trên
//           thiết bị thật — cần đo lại).
//     Tới khi (a)+(b) xong, [[base_vertex]] đọc 0 ⇒ gl_BaseVertex = 0 (sai
//     với glDrawElementsBaseVertex(baseVertex != 0) nhưng ĐÚNG với lệnh draw
//     không baseVertex — GL value 0).
//   • Reflection: uniform `tglmt_DrawID`/`tglmt_NumSamples` nay xuất hiện
//     trong active uniforms (tăng GetProgramResourceiv/glGetUniformLocation).
//     Loc của mọi uniform CŨ giữ nguyên (uniform mới được GÁN SAU cùng).

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
// 1. TIỆN ÍCH VĂN BẢN (word-boundary)
// =====================================================================

bool FinalFindWord(const std::string& s, size_t from, const char* w, size_t* pos) {
    size_t n = strlen(w);
    if (!n) return false;
    size_t p = from;
    while ((p = s.find(w, p)) != std::string::npos) {
        bool l = p > 0 && IsIdentChar(s[p - 1], false);
        size_t e = p + n;
        bool r = e < s.size() && IsIdentChar(s[e], false);
        if (!l && !r) {
            if (pos) *pos = p;
            return true;
        }
        p = e;
    }
    return false;
}

std::string FinalReplaceWord(const std::string& s, const char* from,
                             const std::string& to) {
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
// 2. THÊM UNIFORM INT (linker gán location; converter tính lại layout
//    uniform SAU khi pass chạy — xem glsl_to_msl.cpp ~dòng 11950)
// =====================================================================

void FinalAddIntUniform(GLSLBuiltinCtx& ctx, const char* name) {
    if (!ctx.uniforms) return;
    for (auto& u : *ctx.uniforms)
        if (u.name == name) return;  // đã có ⇒ không thêm lại
    GLSLVar v;
    v.glslType = "int";
    v.mslType = "int";
    v.name = name;
    v.location = -1;   // linker (gl_program.cpp) gán location thật
    v.arraySize = 0;
    ctx.uniforms->push_back(v);
}

// =====================================================================
// 3. PASS MỨC NGUỒN: chèn khối gl_out intrinsic cho TCS (mục 4)
// =====================================================================

// Có layout(...vertices...) — dấu hiệu TCS (layout(vertices = N) out;).
// Duyệt mọi `layout (` và kiểm tra bên trong ngoặc có từ `vertices`.
bool FinalTcsSignal(const std::string& s) {
    size_t p = 0;
    while ((p = s.find("layout", p)) != std::string::npos) {
        if (p > 0 && IsIdentChar(s[p - 1], false)) {
            p += 6;
            continue;
        }
        size_t q = p + 6;
        while (q < s.size() && isspace((unsigned char)s[q])) ++q;
        if (q >= s.size() || s[q] != '(') {
            p = q;
            continue;
        }
        int depth = 0;
        size_t i = q;
        bool closed = false;
        for (; i < s.size(); ++i) {
            if (s[i] == '(') ++depth;
            else if (s[i] == ')') {
                if (--depth == 0) {
                    ++i;
                    closed = true;
                    break;
                }
            }
        }
        std::string inner = closed ? s.substr(q + 1, i - q - 2) : s.substr(q + 1);
        size_t pos = std::string::npos;
        if (FinalFindWord(inner, 0, "vertices", &pos)) return true;
        p = closed ? i : s.size();
    }
    return false;
}

// Statement chứa `pos` (tách bởi ';' / '}' gần nhất phía trước).
std::string FinalStatementAt(const std::string& s, size_t pos) {
    size_t ls = 0;
    for (size_t i = pos; i-- > 0;) {
        if (s[i] == ';' || s[i] == '}') {
            ls = i + 1;
            break;
        }
    }
    size_t len = s.size() - ls;
    if (len > 4096) len = 4096;  // đủ cho mọi khối gl_PerVertex thực tế
    return s.substr(ls, len);
}

// Nguồn đã khai báo khối `out gl_PerVertex { ... } gl_out[];` chưa?
bool FinalHasExplicitGlOutBlock(const std::string& s) {
    size_t p = 0;
    while ((p = s.find("gl_PerVertex", p)) != std::string::npos) {
        std::string stmt = FinalStatementAt(s, p);
        size_t pos = std::string::npos;
        if (FinalFindWord(stmt, 0, "out", &pos) &&
            stmt.find("gl_out") != std::string::npos)
            return true;
        p += 12;  // strlen("gl_PerVertex")
    }
    return false;
}

// GLSL 4.60 §7.1.2 (PDF p.145): TCS intrinsically khai báo
//   out gl_PerVertex { ... } gl_out[];
// Converter hiện chỉ nhận khối khai báo TƯỜNG MINH ⇒ shader dùng đúng form
// intrinsic (hợp lệ) bị từ chối oan "gl_out chưa khai báo". Chèn khai báo
// nếu: (a) nguồn là TCS (có layout(vertices=...)), (b) chưa khai báo gl_out,
// (c) tìm thấy `void main` để chèn trước. Không chắc ⇒ trả nguyên (an toàn).
std::string FinalInjectImplicitGlOut(const std::string& src) {
    if (!FinalTcsSignal(src)) return src;
    if (FinalHasExplicitGlOutBlock(src)) return src;  // linker-block lặp lại là thừa
    size_t p = src.find("void main");
    if (p == std::string::npos) return src;           // không chắc ⇒ không đụng
    static const char kBlock[] =
        "out gl_PerVertex { vec4 gl_Position; float gl_PointSize; } gl_out[];\n";
    return src.substr(0, p) + kBlock + src.substr(p);
}

}  // namespace

// =====================================================================
// 4. PASS
// =====================================================================

static void ApplyGLSLBuiltinFinal(GLSLBuiltinCtx& ctx) {
    if (!ctx.body) return;
    std::string& body = *ctx.body;
    size_t pos = std::string::npos;

    if (ctx.isVertex) {
        // gl_DrawID (GLSL 4.60 §7.1.1, PDF p.143-145): VS-only. Metal không
        // có [[draw_index]] ⇒ uniform `tglmt_DrawID` do backend điền mỗi draw.
        // GIỚI HẠN (trung thực): builtin dùng trong HÀM HELPER (không phải
        // main) KHÔNG được dịch — pass chạy SAU khi ThreadHelpersMSL đã quyết
        // định thread `tglmt_u` vào helper nào ⇒ helper giữ nguyên `gl_DrawID`
        // → Metal báo "use of undeclared identifier 'gl_DrawID'" (lỗi rõ, không
        // âm thầm sai). Cùng giới hạn đã ghi ở glsl_builtins_modern_io.cpp
        // (mọi built-in dịch bằng built-in pass: gl_Layer/gl_PrimitiveID… đều
        // vậy). Fix đầy đủ: converter phải cho pass mức NGUỒN thêm uniform, hoặc
        // built-in pass chạy TRƯỚC ThreadHelpersMSL (sửa glsl_to_msl.cpp — ngoài
        // write-scope của nhóm này).
        if (FinalFindWord(body, 0, "gl_DrawID", &pos)) {
            FinalAddIntUniform(ctx, "tglmt_DrawID");
            body = FinalReplaceWord(body, "gl_DrawID", "tglmt_u.tglmt_DrawID");
        }
        // gl_BaseVertex: converter emit tham số [[base_vertex]] — không đụng.
        // gl_BaseInstance/gl_InstanceID/gl_VertexID: converter đã map.
        return;
    }

    if (!ctx.isFragment) return;

    // gl_NumSamples (GLSL 4.60 §7.4 "Built-In Uniform State", PDF p.159):
    // FS-only ("only guaranteed to be available in the fragment stage").
    // Metal không có attribute sample-count cho fragment function ⇒ uniform
    // `tglmt_NumSamples` do backend điền = sampleCount của render target.
    // GIỚI HẠN: hàm helper — xem ghi chú gl_DrawID ở trên.
    if (FinalFindWord(body, 0, "gl_NumSamples", &pos)) {
        FinalAddIntUniform(ctx, "tglmt_NumSamples");
        body = FinalReplaceWord(body, "gl_NumSamples", "tglmt_u.tglmt_NumSamples");
    }
}

static const bool kRegFinal = RegisterGLSLBuiltinPass("final", ApplyGLSLBuiltinFinal);
static const bool kRegFinalTcs = RegisterGLSLSourcePass("final_tcs", FinalInjectImplicitGlOut);

}  // namespace tglmt::gl
