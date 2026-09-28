// GLSLConverter.cpp — xem GLSLConverter.h về subset được hỗ trợ.
// Mọi thứ ngoài subset → ok=false + log rõ ràng (không đoán mò sinh code sai).
#include "tglmt/GLSLConverter.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tglmt {
namespace {

// ---- tiện ích chuỗi ----
static std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
static bool IsIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
}
// Bỏ comment // /* */, giữ nguyên string "..." (hiếm trong shader, vẫn xử lý đúng).
static std::string StripComments(const std::string& s) {
    std::string o;
    bool line = false, block = false, str = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i], n = i + 1 < s.size() ? s[i + 1] : 0;
        if (line) { if (c == '\n') { line = false; o += c; } continue; }
        if (block) { if (c == '*' && n == '/') { block = false; ++i; } continue; }
        if (str) { o += c; if (c == '"') str = false; continue; }
        if (c == '"') { str = true; o += c; continue; }
        if (c == '/' && n == '/') { line = true; continue; }
        if (c == '/' && n == '*') { block = true; ++i; continue; }
        o += c;
    }
    return o;
}
// Tách statement cấp depth 0 theo ';' (giữ nguyên index để thay thế).
struct Stmt { std::string text; size_t start, end; }; // end = sau ';'
static std::vector<Stmt> SplitTopLevel(const std::string& s) {
    std::vector<Stmt> out;
    int depth = 0;
    size_t start = 0;
    bool str = false;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (str) { if (c == '"') str = false; continue; }
        if (c == '"') { str = true; continue; }
        if (c == '{') ++depth;
        else if (c == '}') { --depth; }
        else if (c == ';' && depth == 0) {
            out.push_back({s.substr(start, i - start + 1), start, i + 1});
            start = i + 1;
        }
    }
    // Đuôi file sau ';' cuối (thường là thân main) — không được bỏ rơi.
    if (start < s.size()) {
        std::string tail = s.substr(start);
        bool blank = true;
        for (char c : tail)
            if (!isspace((unsigned char)c)) { blank = false; break; }
        if (!blank) out.push_back({tail, start, s.size()});
    }
    return out;
}
static std::string MSLType(const std::string& t, bool& ok) {
    ok = true;
    if (t == "float") return "float";
    if (t == "vec2") return "float2";
    if (t == "vec3") return "float3";
    if (t == "vec4") return "float4";
    if (t == "mat2") return "float2x2";
    if (t == "mat3") return "float3x3";
    if (t == "mat4") return "float4x4";
    if (t == "int") return "int";
    if (t == "ivec2") return "int2";
    if (t == "ivec3") return "int3";
    if (t == "ivec4") return "int4";
    if (t == "uint") return "uint";
    if (t == "uvec2") return "uint2";
    if (t == "uvec3") return "uint3";
    if (t == "uvec4") return "uint4";
    if (t == "bool") return "bool";
    ok = false;
    return t;
}
// Layout `constant` buffer theo quy tắc MSL tự nhiên (không phải std140,
// không phải 16-hết): scalar 4, vec2 8, vec3/vec4 16, mat cột-nào-align-nấy
// (float3x3 = 3 cột float3 stride 16 = 48). Test pixel-proof kiểm chứng thực tế.
static size_t MSLAlign(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    if (m == "float2x2") return 8;
    return 16; // vec3/vec4/mat3/mat4 và mọi vector 3-4 phần tử
}
static size_t MSLSize(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    if (m == "float3" || m == "int3" || m == "uint3") return 12;
    if (m == "float4" || m == "int4" || m == "uint4") return 16;
    if (m == "float2x2") return 16;
    if (m == "float3x3") return 48;
    if (m == "float4x4") return 64;
    return 0;
}
// Tìm `name = ...` (word-boundary, cắt ở `,` depth 0) trong declarator list.
static std::string ExtractDeclaratorInit(const std::string& crest, const std::string& name) {
    size_t fs = 0;
    while ((fs = crest.find(name, fs)) != std::string::npos) {
        bool wl = fs > 0 && IsIdentChar(crest[fs - 1], false);
        size_t fe = fs + name.size();
        bool wr = fe < crest.size() && IsIdentChar(crest[fe], false);
        if (!wl && !wr) break;
        fs = fe;
    }
    if (fs == std::string::npos) return "";
    size_t eq = crest.find('=', fs + name.size());
    if (eq == std::string::npos) return "";
    int dd = 0;
    size_t ce = eq + 1;
    for (; ce < crest.size(); ++ce) {
        if (crest[ce] == '(') ++dd;
        else if (crest[ce] == ')') --dd;
        else if (crest[ce] == ',' && dd == 0) break;
    }
    return Trim(crest.substr(eq + 1, ce - (eq + 1)));
}
// Global mutable (post OIT scratchpad) hoist về main-local + thread-ref.
struct HoistedGlobal {
    std::string glslType, mslType, name;
    int arraySize = 0;
    std::string init; // GLSL gốc, convert lúc phát
};
// Dấu phẩy trong ()/[] không tách; initializer (=...) bị bỏ (uniform lấy
// giá trị từ glUniform, không từ source). Tên mảng macro đã thay xong ở
// preprocessor nên [N] luôn là số ở đây.
static bool SplitDeclarators(const std::string& rest,
                             std::vector<std::pair<std::string, int>>& out) {
    std::vector<std::string> pieces;
    int pd = 0, bd = 0;
    size_t start = 0;
    for (size_t i = 0; i <= rest.size(); ++i) {
        char ch = i < rest.size() ? rest[i] : ',';
        if (ch == '(') ++pd;
        else if (ch == ')') --pd;
        else if (ch == '[') ++bd;
        else if (ch == ']') --bd;
        if (ch == ',' && pd == 0 && bd == 0) {
            pieces.push_back(Trim(rest.substr(start, i - start)));
            start = i + 1;
        }
    }
    for (auto& pc : pieces) {
        // bỏ "= ..." ở depth 0
        int d2 = 0;
        size_t cut = std::string::npos;
        for (size_t i = 0; i < pc.size(); ++i) {
            if (pc[i] == '(') ++d2;
            else if (pc[i] == ')') --d2;
            else if (pc[i] == '=' && d2 == 0) { cut = i; break; }
        }
        std::string nm = Trim(cut == std::string::npos ? pc : pc.substr(0, cut));
        int arr = 0;
        size_t lb = nm.find('[');
        if (lb != std::string::npos) {
            size_t rb = nm.find(']', lb);
            if (rb == std::string::npos) return false;
            std::string ns = Trim(nm.substr(lb + 1, rb - lb - 1));
            if (ns.empty()) return false; // mảng unsized ngoài subset
            for (char c : ns)
                if (!isdigit((unsigned char)c)) return false;
            arr = atoi(ns.c_str());
            if (arr <= 0 || arr > 4096) return false;
            nm = Trim(nm.substr(0, lb));
        }
        if (nm.empty()) return false;
        size_t p0 = 0;
        while (p0 < nm.size() && (IsIdentChar(nm[p0], p0 == 0))) ++p0;
        if (p0 != nm.size()) return false; // token lạ
        out.emplace_back(nm, arr);
    }
    return !out.empty();
}
// Parse "layout(location=N) in|out TYPE decls" / "in|out TYPE decls" /
// "uniform TYPE decls" (nhiều declarator, vd `uniform vec2 A, B[4];`).
// Trả false nếu không khớp.
static bool ParseDecls(const std::string& stmt, bool isVertex,
                       std::string& dir, std::vector<GLSLVar>& vars, bool& isUniform) {
    std::string s = Trim(stmt);
    if (!s.empty() && s.back() == ';') s.pop_back();
    s = Trim(s);
    isUniform = false;
    vars.clear();
    // Qualifier nội suy đứng đầu (`flat out ...`): bỏ qua khi parse, Metal
    // suy flat từ kiểu (int/uint tự flat; float giữ smooth — đúng GL mặc định).
    for (const char* q : {"flat ", "smooth ", "noperspective ", "centroid ",
                          "sample ", "invariant ", "precise "}) {
        if (s.rfind(q, 0) == 0) { s = Trim(s.substr(strlen(q))); break; }
    }
    int loc = -1;
    if (s.rfind("layout", 0) == 0) {
        size_t lp = s.find('('), rp = s.find(')');
        if (lp == std::string::npos || rp == std::string::npos) return false;
        std::string inside = s.substr(lp + 1, rp - lp - 1);
        size_t eq = inside.find("location");
        if (eq == std::string::npos) {
            // layout(std140)/layout(binding=N) cho UBO — không phải location, caller xử lý riêng
            return false;
        }
        size_t num = inside.find_first_of("0123456789", eq);
        if (num == std::string::npos) return false;
        loc = atoi(inside.c_str() + num);
        s = Trim(s.substr(rp + 1));
    }
    std::istringstream iss(s);
    std::vector<std::string> w;
    std::string tok;
    while (iss >> tok) w.push_back(tok);
    auto isQual = [](const std::string& t) {
        return t == "highp" || t == "mediump" || t == "lowp" || t == "flat" ||
               t == "smooth" || t == "noperspective" || t == "centroid" || t == "sample" ||
               t == "coherent" || t == "volatile" || t == "restrict" || t == "readonly" ||
               t == "writeonly" || t == "invariant";
    };
    if (w.empty()) return false;
    std::string type;
    std::string restDecls;
    if (w[0] == "uniform") {
        size_t k = 1;
        while (k < w.size() && isQual(w[k])) ++k;
        if (k >= w.size()) return false;
        if (w[k].find('{') != std::string::npos) return false; // uniform block
        type = w[k];
        restDecls.clear();
        for (size_t q = k + 1; q < w.size(); ++q) {
            if (!restDecls.empty()) restDecls += " ";
            restDecls += w[q];
        }
        if (restDecls.empty()) return false;
        isUniform = true;
        dir = "uniform";
    } else if (w[0] == "in" || w[0] == "out") {
        dir = w[0];
        size_t k = 1;
        while (k < w.size() && isQual(w[k])) ++k;
        if (k >= w.size()) return false;
        type = w[k];
        restDecls.clear();
        for (size_t q = k + 1; q < w.size(); ++q) {
            if (!restDecls.empty()) restDecls += " ";
            restDecls += w[q];
        }
        if (restDecls.empty()) return false;
    } else {
        return false;
    }
    std::vector<std::pair<std::string, int>> ds;
    if (!SplitDeclarators(restDecls, ds)) return false;
    for (auto& [nm, arr] : ds) {
        // mảng in/out (vd `out vec4 c[2]`) ngoài subset MRT đơn giản → từ chối rõ
        // (MRT dùng nhiều `out` riêng, không phải array).
        if (!isUniform && arr != 0) return false;
        GLSLVar v;
        v.glslType = type;
        v.name = nm;
        v.location = loc;
        v.arraySize = arr;
        vars.push_back(v);
    }
    (void)isVertex;
    return true;
}
// Wrapper 1-declarator cho code cũ (giữ để khỏi sửa nhiều).
static bool ParseDecl(const std::string& stmt, bool isVertex,
                      std::string& dir, GLSLVar& v, bool& isUniform) {
    std::vector<GLSLVar> vs;
    if (!ParseDecls(stmt, isVertex, dir, vs, isUniform) || vs.size() != 1) return false;
    v = vs[0];
    return true;
}
// Viết lại tên kiểu GLSL trong biểu thức (vec→float...), theo word-boundary.
static std::string RewriteTypeNames(std::string s) {
    for (auto& rp : std::vector<std::pair<std::string, std::string>>{
             {"vec2", "float2"}, {"vec3", "float3"}, {"vec4", "float4"},
             {"ivec2", "int2"}, {"ivec3", "int3"}, {"ivec4", "int4"},
             {"uvec2", "uint2"}, {"uvec3", "uint3"}, {"uvec4", "uint4"},
             {"mat2", "float2x2"}, {"mat3", "float3x3"}, {"mat4", "float4x4"}}) {
        size_t pp = 0;
        while ((pp = s.find(rp.first, pp)) != std::string::npos) {
            bool l = pp > 0 && IsIdentChar(s[pp - 1], false);
            size_t e2 = pp + rp.first.size();
            bool r2 = e2 < s.size() && IsIdentChar(s[e2], false);
            if (!l && !r2) {
                s.replace(pp, rp.first.size(), rp.second);
                pp += rp.second.size();
            } else {
                ++pp;
            }
        }
    }
    return s;
}
// `TYPE[](e1, e2, ...)` (array constructor: animate_sprite positions,
// end_portal COLORS, terrain offsets) → `{e1, e2, ...}` vì MSL không có
// cú pháp `float2[](...)`. Chạy TRƯỚC RewriteIdents trên body và helpers.
static std::string RewriteArrayCtors(const std::string& src) {
    static const char* kTypes[] = {"vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4",
                                   "uvec2", "uvec3", "uvec4", "mat2", "mat3", "mat4",
                                   "float", "int", "uint", nullptr};
    std::string out;
    size_t i = 0;
    auto skipSp = [&](size_t p) {
        while (p < src.size() && isspace((unsigned char)src[p])) ++p;
        return p;
    };
    while (i < src.size()) {
        bool hit = false;
        for (const char** tp = kTypes; *tp; ++tp) {
            std::string tn = *tp;
            if (src.compare(i, tn.size(), tn) == 0 &&
                (i == 0 || !IsIdentChar(src[i - 1], false))) {
                size_t j = skipSp(i + tn.size());
                if (j < src.size() && src[j] == '[') {
                    size_t k = skipSp(j + 1);
                    if (k < src.size() && src[k] == ']') {
                        size_t m = skipSp(k + 1);
                        if (m < src.size() && src[m] == '(') {
                            int d = 0;
                            size_t e = m;
                            for (; e < src.size(); ++e) {
                                if (src[e] == '(') ++d;
                                else if (src[e] == ')') {
                                    if (--d == 0) break;
                                }
                            }
                            if (e >= src.size()) break; // mất cân bằng → giữ nguyên
                            out += "{";
                            out += src.substr(m + 1, e - (m + 1));
                            out += "}";
                            i = e + 1;
                            hit = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!hit) {
            out += src[i];
            ++i;
        }
    }
    return out;
}
// ============ GLSL implicit int→float promotion (vanilla 26.1.2) ====
// GLSL 3.30 cho phép trộn ivec/vec trong số học (vd `ivec2 / float`,
// `float / ivec2`, `ivec3 + vec3`) bằng cách nâng int ngầm lên float.
// MSL cấm hoàn toàn (đã quan sát bằng metal thật trên máy). Ba idiom vanilla
// dưới đây là TOÀN BỘ trường hợp trong corpus 26.1.2 (đã quét bằng grep):
//  - sample_lightmap.glsl: `(uv / 256.0)` với uv:ivec2 → `float2(uv)/256.0`
//  - terrain.fsh: `1.0f / TextureSize` với TextureSize:ivec2 (UBO) → float2
//  - terrain.vsh: `(ChunkPosition - CameraBlockPos) + CameraOffset`
//    (ivec3-ivec3)+vec3 → float3(ivec3_expr)+vec3
// Viết tường minh floatN(...) — nếu input đã là float thì floatN(floatN)
// vẫn đúng (identity) nên rewrite an toàn, không đổi nghĩa khi đã float.
static void PromoteVanillaIntMixing(std::string& text) {
    // 1. `uv / 256.0` → `float2(uv) / 256.0` (helper sample_lightmap, uv bare).
    // Chỉ khi uv chưa được bọc (tránh double-wrap khi chạy 2 lần body+helper).
    {
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            size_t p = text.find("uv / 256.0", i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            bool already = false;
            if (p >= 7 && text.compare(p - 7, 7, "float2(") == 0) already = true;
            // word-boundary trước uv (tránh `auv / ...`)
            bool lb = p > 0 && IsIdentChar(text[p - 1], false);
            // `uv` trong `float2(uv)` đã bọc thì `p-7` là float2( → already=true
            if (already || lb) {
                out += text.substr(i, p - i + 2); // giữ nguyên tới `uv`
                i = p + 2;
                continue;
            }
            out += text.substr(i, p - i);
            out += "float2(uv) / 256.0";
            i = p + 10; // len("uv / 256.0")
        }
        text.swap(out);
    }
    // 2. `1.0f / <Expr>.TextureSize` → `1.0f / float2(<Expr>.TextureSize)`.
    // TextureSize là ivec2 trong UBO ChunkSection (terrain.fsh dùng 2 lần).
    {
        std::string out;
        size_t i = 0;
        const char* needle = "1.0f / ";
        while (i < text.size()) {
            size_t p = text.find(needle, i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            out += text.substr(i, p - i);
            size_t e = p + 7; // sau "1.0f / "
            // bỏ spaces (thường không có, nhưng chắc ăn)
            while (e < text.size() && isspace((unsigned char)text[e])) ++e;
            // nếu đã là float2( → bỏ qua
            if (text.compare(e, 7, "float2(") == 0) {
                out += needle;
                i = e;
                continue;
            }
            // đọc `ubo_X.TextureSize` (word [. word]*)
            size_t s = e;
            while (s < text.size() && (IsIdentChar(text[s], s == e) || text[s] == '.')) {
                // dừng ở ký tự không phải ident/dot; dot chỉ giữa words
                if (text[s] == '.') {
                    // dot phải theo sau bởi ident
                    if (s + 1 >= text.size() || !IsIdentChar(text[s + 1], true)) break;
                }
                ++s;
            }
            std::string expr = text.substr(e, s - e);
            if (expr.find("TextureSize") != std::string::npos && !expr.empty()) {
                out += "1.0f / float2(" + expr + ")";
                i = s;
            } else {
                out += needle;
                i = e;
            }
        }
        text.swap(out);
    }
    // 3. `(A.ChunkPosition - B.CameraBlockPos)` → `float3(...)` khi cộng vec3.
    // Pattern vanilla terrain.vsh: `(ubo_X.ChunkPosition - ubo_Y.CameraBlockPos)`
    {
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            size_t p = text.find("ChunkPosition - ", i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            // tìm '(' mở trước ChunkPosition (có thể có `ubo_X.` trước nữa)
            // Lùi để lấy toàn bộ `(expr - expr)`: tìm '(' gần nhất mà chưa đóng.
            size_t paren = text.rfind('(', p);
            // kiểm tra đã bọc float3( chưa
            bool already = false;
            if (paren != std::string::npos && paren >= 6 &&
                text.compare(paren - 6, 6, "float3") == 0)
                already = true;
            if (already || paren == std::string::npos) {
                out += text.substr(i, p - i + 1);
                i = p + 1;
                continue;
            }
            // tìm ')' đóng của cặp này (cân bằng đơn giản: tới ')' đầu ở depth 0)
            // Vì expr chỉ chứa `-` và dots, ')' đầu sau CameraBlockPos là đóng.
            size_t cb = text.find(')', p);
            if (cb == std::string::npos) { out += text.substr(i); break; }
            std::string inside = text.substr(paren + 1, cb - paren - 1);
            if (inside.find("ChunkPosition") == std::string::npos ||
                inside.find("CameraBlockPos") == std::string::npos) {
                out += text.substr(i, p - i + 1);
                i = p + 1;
                continue;
            }
            out += text.substr(i, paren - i);
            out += "float3(" + inside + ")";
            i = cb + 1;
        }
        text.swap(out);
    }
}
// ============ mat4(mat2) expansion (end_portal) ====
// GLSL `mat4(mat2 m)` nhúng 2x2 vào góc 4x4 (còn lại identity) — hợp lệ GL.
// MSL không có ctor `float4x4(float2x2)` (đã quan sát bằng metal thật).
// Rewrite MỌI `float4x4(X)` đơn-arg (không phẩy depth-0) thành helper
// `tglmt_mat4_from_mat2(X)`; bản 16-float (có phẩy) giữ nguyên.
static void ExpandMat4FromMat2(std::string& text) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        size_t p = text.find("float4x4(", i);
        if (p == std::string::npos) { out += text.substr(i); break; }
        bool l = p > 0 && IsIdentChar(text[p - 1], false);
        if (l) { out += text.substr(i, p - i + 1); i = p + 1; continue; }
        size_t lp = p + 10; // '(' at p+8? "float4x4(" len 9 → '(' cuối
        // "float4x4" 8 chars + "(" = 9
        lp = p + 8;
        // tìm ')' cân bằng + kiểm tra phẩy depth-0 bên trong
        int d = 0;
        size_t k = lp;
        bool hasComma = false;
        for (; k < text.size(); ++k) {
            if (text[k] == '(') ++d;
            else if (text[k] == ')') {
                if (--d == 0) break;
            } else if (text[k] == ',' && d == 1) {
                hasComma = true;
            }
        }
        if (k >= text.size()) { out += text.substr(i); break; }
        if (hasComma) {
            // 16-float ctor → giữ nguyên
            out += text.substr(i, k - i + 1);
            i = k + 1;
            continue;
        }
        // đã là helper? tránh double-wrap
        // kiểm tra 20 ký tự trước có phải tglmt_mat4_from_mat2( không — thực tế
        // text tại p là float4x4(, nếu đã wrap thì không còn float4x4( đơn-arg
        std::string inner = Trim(text.substr(lp + 1, k - lp - 1));
        out += text.substr(i, p - i);
        out += "tglmt_mat4_from_mat2(" + inner + ")";
        i = k + 1;
    }
    text.swap(out);
}
// ============ Rename locals shadowing helper names (lightmap notGamma) ====
// MSL (C++) dùng chung namespace cho hàm và biến: `float3 notGamma =
// notGamma(color);` thì RHS `notGamma` trỏ vào BIẾN đang khai báo (self),
// không phải hàm → "called object type 'float3' is not a function".
// GLSL cho phép (namespace riêng) nên converter phải đổi tên biến local.
// Quy tắc: trong body, mọi `H` KHÔNG theo sau bởi `(` (call) thì là biến →
// `H_var`. Định nghĩa hàm trong helpers giữ nguyên (luôn theo sau `(`).
static void RenameShadowedVars(std::string& body,
                               const std::vector<std::string>& helperNames) {
    for (auto& H : helperNames) {
        if (H.empty()) continue;
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            size_t p = body.find(H, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            bool l = p > 0 && IsIdentChar(body[p - 1], false);
            size_t e = p + H.size();
            bool r = e < body.size() && IsIdentChar(body[e], false);
            if (l || r) { out += body.substr(i, e - i); i = e; continue; }
            // lookahead `(` sau spaces → call/def → giữ
            size_t q = e;
            while (q < body.size() && isspace((unsigned char)body[q])) ++q;
            bool isCall = (q < body.size() && body[q] == '(');
            if (isCall) {
                out += body.substr(i, e - i);
                i = e;
            } else {
                out += body.substr(i, p - i);
                out += H + "_var";
                i = e;
            }
        }
        body.swap(out);
    }
}
// ============ Helper-aware rewriting (sampler qua helper, uniforms/UBO) ====
// Vanilla helpers (fog/light/sample_lightmap) nhận sampler/varying-explicit
// qua param; Metal cần (texture,sampler) tách rời + uniforms/UBO thread vào.
// Machinery dưới parse definitions một lần (pre-mp, tên GLSL) rồi dùng lại
// post-mp cho split/expand/thread.
struct HelperDef {
    std::string name;
    std::string params; // chuỗi params gốc (chưa split)
    size_t defStart = 0; // vị trí bắt đầu header (kiểu trả về) để skip
    size_t lp = 0;      // vị trí '(' trong text đã parse
    size_t rp = 0;      // vị trí ')' đóng
    size_t bodyStart = 0, bodyEnd = 0; // thân {...} (có thể npos nếu chỉ khai báo)
    bool hasBody = false;
};
// Tìm `name(...) {` ở depth 0 (bỏ qua main nếu skipMain). Không fail, chỉ thu thập.
static std::vector<HelperDef> ParseHelperDefs(const std::string& text, bool skipMain = true) {
    std::vector<HelperDef> out;
    size_t i = 0;
    int depth = 0;
    static const char* kKw[] = {"if", "for", "while", "return", "switch", nullptr};
    while (i < text.size()) {
        char ch = text[i];
        if (ch == '"') {
            size_t j = text.find('"', i + 1);
            i = (j == std::string::npos) ? text.size() : j + 1;
            continue;
        }
        if (ch == '{') ++depth;
        else if (ch == '}') --depth;
        if (depth == 0 && (isalpha((unsigned char)ch) || ch == '_')) {
            size_t j = i;
            while (j < text.size() && IsIdentChar(text[j], j == i)) ++j;
            std::string w1 = text.substr(i, j - i);
            size_t k = j;
            while (k < text.size() && isspace((unsigned char)text[k])) ++k;
            // tên hàm là word THỨ HAI (`ret name(`); word đầu là kiểu trả về
            size_t j2 = k;
            while (j2 < text.size() && IsIdentChar(text[j2], j2 == k)) ++j2;
            std::string w2 = text.substr(k, j2 - k);
            size_t m = j2;
            while (m < text.size() && isspace((unsigned char)text[m])) ++m;
            bool isKw = false;
            for (const char** kw = kKw; *kw; ++kw)
                if (w1 == *kw) isKw = true;
            if (!isKw && !w1.empty() && !w2.empty() && m < text.size() && text[m] == '(' &&
                (skipMain ? w2 != "main" : true)) {
                int dd = 0;
                size_t e = m;
                for (; e < text.size(); ++e) {
                    if (text[e] == '(') ++dd;
                    else if (text[e] == ')') {
                        if (--dd == 0) break;
                    }
                }
                if (e < text.size()) {
                    HelperDef h;
                    h.name = w2;
                    h.params = text.substr(m + 1, e - m - 1);
                    h.defStart = i; // bắt đầu từ kiểu trả về để skip cả tên
                    h.lp = m;
                    h.rp = e;
                    size_t b = e + 1;
                    while (b < text.size() && isspace((unsigned char)text[b])) ++b;
                    if (b < text.size() && text[b] == '{') {
                        int d2 = 0;
                        size_t c = b;
                        for (; c < text.size(); ++c) {
                            if (text[c] == '{') ++d2;
                            else if (text[c] == '}') {
                                if (--d2 == 0) break;
                            }
                        }
                        if (c < text.size()) {
                            h.bodyStart = b;
                            h.bodyEnd = c;
                            h.hasBody = true;
                        }
                    }
                    out.push_back(h);
                    i = h.hasBody ? h.bodyEnd + 1 : h.rp + 1;
                    continue;
                }
            }
        }
        ++i;
    }
    return out;
}
// Tách args call theo dấu phẩy depth 0 (ngoặc tròn).
static std::vector<std::string> SplitCallArgs(const std::string& s) {
    std::vector<std::string> out;
    int d = 0;
    size_t s0 = 0;
    for (size_t k = 0; k <= s.size(); ++k) {
        char ch = k < s.size() ? s[k] : ',';
        if (ch == '(') ++d;
        else if (ch == ')') --d;
        else if (ch == ',' && d == 0) {
            out.push_back(Trim(s.substr(s0, k - s0)));
            s0 = k + 1;
        }
    }
    return out;
}
// Tên param cuối trong khai báo param (`const vec3 &x[4]` → x).
static std::string ParamBaseName(const std::string& p) {
    std::string t = Trim(p);
    size_t lb = t.find('[');
    std::string core = Trim(lb == std::string::npos ? t : t.substr(0, lb));
    size_t sp = core.find_last_of(" \t*&");
    std::string nm = (sp == std::string::npos) ? core : Trim(core.substr(sp + 1));
    return nm;
}
// Loại sampler theo từ kiểu GLSL: '2' 2D, 'S' shadow, 'C' cube, 'A' array,
// 'B' buffer-float, 'I' buffer-int, 'U' buffer-uint, 0 = không phải sampler.
static char SamplerKindOf(const std::string& glslType) {
    if (glslType == "sampler2D") return '2';
    if (glslType == "sampler2DShadow") return 'S';
    if (glslType == "samplerCube") return 'C';
    if (glslType == "sampler2DArray") return 'A';
    if (glslType == "samplerBuffer") return 'B';
    if (glslType == "isamplerBuffer") return 'I';
    if (glslType == "usamplerBuffer") return 'U';
    return 0;
}
// Kiểu texture MSL theo kind sampler (cho split param).
static std::string SamplerTexType(char kind, const std::string& elem = "float") {
    switch (kind) {
        case 'C': return "texturecube<float>";
        case 'A': return "texture2d_array<float>";
        case 'I': return "texture2d<int>";
        case 'U': return "texture2d<uint>";
        case 'B': return "texture2d<" + elem + ">";
        default: return "texture2d<float>";
    }
}
// Bảng sampler cho rewrite sampling calls: tên → (kind, elem).
struct SampEntry {
    char kind = '2'; // 2/S/C/A/B/I/U (xem SamplerKindOf)
    std::string elem = "float";
};
using SampTable = std::map<std::string, SampEntry>;
// Viết lại texture()/texelFetch()/textureGrad() trong text (body hoặc helper).
// Trả false + err khi ngoài subset. Table phải chứa MỌI sampler dùng trong text
// (global R.samplers + sampler params của helper chứa text đó).
static bool RewriteSamplingCalls(std::string& io, const SampTable& tab, std::string& err) {
    std::string& body = io;
    auto findSamp = [&](const std::string& w, SampEntry& e) {
        auto it = tab.find(w);
        if (it == tab.end()) return false;
        e = it->second;
        return true;
    };
    auto splitArgs = [&](size_t lp, size_t& kend, std::vector<std::string>& args) -> bool {
        int d = 0;
        size_t k = lp, a0 = lp + 1;
        for (; k < body.size(); ++k) {
            if (body[k] == '(') ++d;
            else if (body[k] == ')') {
                if (--d == 0) break;
            } else if (body[k] == ',' && d == 1) {
                args.push_back(Trim(body.substr(a0, k - a0)));
                a0 = k + 1;
            }
        }
        if (k >= body.size()) return false;
        args.push_back(Trim(body.substr(a0, k - a0)));
        kend = k;
        return true;
    };
    auto isHead = [&](size_t i, const char* w) {
        size_t n = strlen(w);
        return body.compare(i, n, w) == 0 && (i == 0 || !IsIdentChar(body[i - 1], false)) &&
               (i + n >= body.size() || !IsIdentChar(body[i + n], false));
    };
    // ---- texture() ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "texture")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 7);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "texture(...) ngoặc không cân bằng"; return false; }
            if (args.size() < 2 || args.size() > 3) { err = "texture(...) cần (sampler, uv[, bias])"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "texture(...) sampler không khai báo: " + sname; return false; }
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') {
                err = "texture() trên buffer sampler (dùng texelFetch): " + sname;
                return false;
            }
            std::string rep;
            std::string bias = args.size() == 3 ? ", float(" + args[2] + ")" : "";
            if (sv.kind == 'C') {
                rep = sname + "_tex.sample(" + sname + "_smp, float3(" + args[1] + ")" + bias + ")";
            } else if (sv.kind == 'A') {
                rep = sname + "_tex.sample(" + sname + "_smp, float2((" + args[1] + ").xy), uint((" +
                      args[1] + ").z)" + bias + ")";
            } else {
                std::string uv = sv.kind == 'S' ? "float2((" + args[1] + ").xy)" : "float2(" + args[1] + ")";
                rep = sname + "_tex.sample(" + sname + "_smp, " + uv + bias + ")";
            }
            out += rep;
            i = k + 1;
        }
        body = out;
    }
    // ---- texelFetch() ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "texelFetch")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 10);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "texelFetch(...) ngoặc không cân bằng"; return false; }
            SampEntry sv;
            if (args.empty() || !findSamp(Trim(args[0]), sv)) {
                err = "texelFetch chỉ hỗ trợ texelFetch(sampler, coord[, lod])";
                return false;
            }
            std::string sname = Trim(args[0]);
            std::string rep;
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') {
                if (args.size() != 2) { err = "texelFetch buffer chỉ 2 args (sampler, index)"; return false; }
                rep = sname + "_tex.read(uint2(uint(" + args[1] + "), 0u))";
            } else {
                if (args.size() < 2 || args.size() > 3) { err = "texelFetch 2D cần (sampler, coord[, lod])"; return false; }
                std::string lod = args.size() == 3 ? args[2] : "0";
                rep = sname + "_tex.read(uint2(" + args[1] + "), " + lod + ")";
            }
            out += rep;
            i = k + 1;
        }
        body = out;
    }
    // ---- textureGrad() → sample + gradient2d (terrain) ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureGrad")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 11);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureGrad(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 4) { err = "textureGrad cần (sampler, uv, dPdx, dPdy)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureGrad sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2') { err = "textureGrad chỉ hỗ trợ sampler2D"; return false; }
            out += sname + "_tex.sample(" + sname + "_smp, float2(" + args[1] + "), gradient2d(float2(" +
                   args[2] + "), float2(" + args[3] + ")))";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureLod() → sample với lod tường minh (animate_sprite).
    // Đã kiểm chứng `t.sample(s, uv, lod)` compile được bằng metal thật.
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureLod")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 10);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureLod(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 3) { err = "textureLod cần (sampler, uv, lod)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureLod sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2') { err = "textureLod chỉ hỗ trợ sampler2D"; return false; }
            out += sname + "_tex.sample(" + sname + "_smp, float2(" + args[1] + "), float(" + args[2] + "))";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureProj() → sample với chia phối cảnh (end_portal).
    // GLSL textureProj(sampler, vec4) = texture(sampler, coord.xy/coord.w).
    // Vanilla end_portal dùng 2 biến thể: (Sampler, vec4) và (Sampler, vec4, bias).
    // Metal không có textureProj nên hạ về sample + chia tay (đúng GL, đã kiểm
    // chứng bằng metal thật: float4 * float4x4 cho row-vector vẫn compile).
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureProj")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 11);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureProj(...) ngoặc không cân bằng"; return false; }
            if (args.size() < 2 || args.size() > 3) { err = "textureProj cần (sampler, coord[, bias])"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureProj sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2') { err = "textureProj chỉ hỗ trợ sampler2D"; return false; }
            std::string coord = Trim(args[1]);
            std::string bias = args.size() == 3 ? ", float(" + args[2] + ")" : "";
            // vec4 → xy/w (GL spec; vanilla end_portal chỉ dùng vec4, w luôn != 0
            // vì là clip w nên chia trực tiếp, không guard để giữ đúng GL).
            out += sname + "_tex.sample(" + sname + "_smp, float2((" + coord + ").xy / (" +
                   coord + ").w)" + bias + ")";
            i = k + 1;
        }
        body = out;
    }
    return true;
}
// Thay identifier dạng token (không chạm substring), theo bảng ánh xạ.
static std::string RewriteIdents(const std::string& src,
                                 const std::vector<std::pair<std::string, std::string>>& mp) {
    std::string o;
    size_t i = 0;
    while (i < src.size()) {
        char c = src[i];
        if (c == '"') { // giữ string
            size_t j = src.find('"', i + 1);
            if (j == std::string::npos) j = src.size() - 1;
            o += src.substr(i, j - i + 1);
            i = j + 1;
            continue;
        }
        if (IsIdentChar(c, true)) {
            size_t j = i + 1;
            while (j < src.size() && IsIdentChar(src[j], false)) ++j;
            std::string w = src.substr(i, j - i);
            bool hit = false;
            for (auto& kv : mp)
                if (kv.first == w) { o += kv.second; hit = true; break; }
            if (!hit) o += w;
            i = j;
            continue;
        }
        o += c;
        ++i;
    }
    return o;
}
// POST-MP ENGINE cho helpers: split sampler params + thread tglmt_u/ubo/pairs.
// Chạy trên MSL text (sau RewriteIdents): helpersMSL + body(main).
// - Split: param sampler `s` → `(texture S_tex, sampler S_smp)` (buffer: tex-only).
// - Thread: helper dùng `tglmt_u.`/`ubo_B.`/global `S_tex` → thêm param + args ở calls.
// - Varying `_in./_out.` trong helper → fail trung thực (phải truyền param).
// helperSamp: record pre-mp (helper → [(paramName, kind/elem)]).
// POST-MP ENGINE cho helpers: split sampler params + thread tglmt_u/ubo/pairs.
// Chạy trên MSL text (sau RewriteIdents): helpersMSL + body(main).
// Overloads phân biệt bằng số formal gốc (sampleNearest/3 vs /6).
// - Split: param sampler `s` → `(texture S_tex, sampler S_smp)` (buffer: tex-only).
// - Thread: helper dùng `tglmt_u.`/`ubo_B.`/global `S_tex` → thêm param + args ở calls.
// - Varying `_in./_out.` trong helper → fail trung thực (phải truyền param).
// helperSamp: record pre-mp (helper → [(paramName, kind/elem)]).
static bool ThreadHelpersMSL(
    std::string& helpersMSL, std::string& body,
    const std::map<std::string, std::vector<std::pair<std::string, SampEntry>>>& helperSamp,
    const SampTable& globalSamp, const std::vector<std::string>& uboBlocks, bool isVertex,
    std::string& err) {
    (void)isVertex;
    struct Def {
        std::string name;
        std::string params; // MSL params gốc (pre-split)
        size_t defStart = 0; // đầu header (để xoá def trùng + skip calls)
        size_t lp = 0, rp = 0, bodyStart = 0, bodyEnd = 0;
        bool hasBody = false;
        int origCount = 0;
        std::string key; // name + "/" + origCount
        std::vector<std::string> formalNames;
        std::map<std::string, char> splitKind; // formal sampler → kind
        std::map<std::string, std::string> splitElem;
        std::vector<std::string> req; // items cần trong scope (sorted unique)
    };
    auto hasWordIn = [&](const std::string& hay, size_t from, size_t to, const std::string& w) {
        size_t n = w.size(), p = from;
        while ((p = hay.find(w, p)) != std::string::npos && p < to) {
            bool l = p > 0 && IsIdentChar(hay[p - 1], false);
            size_t e = p + n;
            bool r = e < hay.size() && IsIdentChar(hay[e], false);
            if (!l && !r) return true;
            p = e;
        }
        return false;
    };
    auto isBareName = [&](const std::string& e) {
        if (e.empty() || !(isalpha((unsigned char)e[0]) || e[0] == '_')) return false;
        for (size_t q = 1; q < e.size(); ++q)
            if (!IsIdentChar(e[q], false)) return false;
        return true;
    };
    // ---- parse defs + dedupe trùng byte-identical (import lặp) ----
    // Blaze3D import projection.glsl 2 lần trong end_portal: dedupe ở đây
    // (parse → tìm cặp trùng → xoá desc → parse lại). Chỉ trùng TOÀN BỘ
    // (tên+params+body) mới xoá; khác body là overload/redef thật.
    auto parseAllDefs = [&](std::vector<Def>& out) {
        out.clear();
        for (auto& h : ParseHelperDefs(helpersMSL, true)) {
            Def d;
            d.name = h.name;
            d.params = h.params;
            d.defStart = h.defStart;
            d.lp = h.lp;
            d.rp = h.rp;
            d.bodyStart = h.bodyStart;
            d.bodyEnd = h.bodyEnd;
            d.hasBody = h.hasBody;
            std::vector<std::string> pieces = SplitCallArgs(h.params);
            if (pieces.size() == 1 && Trim(pieces[0]).empty()) pieces.clear();
            d.origCount = (int)pieces.size();
            d.key = d.name + "/" + std::to_string(d.origCount);
            for (auto& pp : pieces) d.formalNames.push_back(ParamBaseName(Trim(pp)));
            for (auto& fn : d.formalNames) {
                if (fn.empty()) continue;
                char kind = 0;
                std::string elem = "float";
                auto rit = helperSamp.find(d.name);
                if (rit != helperSamp.end())
                    for (auto& [pn, se] : rit->second)
                        if (pn == fn) { kind = se.kind; elem = se.elem; }
                if (kind == 0) {
                    for (auto& pp : pieces) {
                        if (ParamBaseName(Trim(pp)) != fn) continue;
                        std::string low = Trim(pp);
                        if (low.find("sampler") != std::string::npos && low.find("texture") == std::string::npos)
                            kind = '2';
                        else if (low.find("texturecube") != std::string::npos) kind = 'C';
                        else if (low.find("texture2d_array") != std::string::npos) kind = 'A';
                        else if (low.find("texture2d<int>") != std::string::npos) { kind = 'I'; elem = "int"; }
                        else if (low.find("texture2d<uint>") != std::string::npos) { kind = 'U'; elem = "uint"; }
                        else if (low.find("texture2d<") != std::string::npos) kind = '2';
                        break;
                    }
                }
                if (kind != 0) {
                    d.splitKind[fn] = kind;
                    d.splitElem[fn] = elem;
                }
            }
            out.push_back(d);
        }
    };
    std::vector<Def> defs;
    parseAllDefs(defs);
    {
        struct Sig {
            size_t a, b;
        };
        std::vector<Sig> eraseSpans;
        for (size_t i = 0; i < defs.size(); ++i) {
            for (size_t j = 0; j < i; ++j) {
                if (defs[i].name != defs[j].name || defs[i].params != defs[j].params ||
                    defs[i].hasBody != defs[j].hasBody)
                    continue;
                std::string bi = defs[i].hasBody
                                     ? helpersMSL.substr(defs[i].bodyStart, defs[i].bodyEnd - defs[i].bodyStart + 1)
                                     : "";
                std::string bj = defs[j].hasBody
                                     ? helpersMSL.substr(defs[j].bodyStart, defs[j].bodyEnd - defs[j].bodyStart + 1)
                                     : "";
                if (bi != bj) continue; // khác body = overload/redef thật, giữ
                size_t ae = defs[i].hasBody ? defs[i].bodyEnd + 1 : defs[i].rp + 1;
                eraseSpans.push_back({defs[i].defStart, ae});
                break; // mỗi def trùng chỉ xoá 1 lần (bản đầu giữ lại)
            }
        }
        std::sort(eraseSpans.begin(), eraseSpans.end(),
                  [](const Sig& a, const Sig& b) { return a.a > b.a; });
        for (auto& sp : eraseSpans) helpersMSL.erase(sp.a, sp.b - sp.a);
        if (!eraseSpans.empty()) parseAllDefs(defs); // spans lệch → parse lại
    }
    auto globalPairItems = [&](const std::string& sname, std::vector<std::string>& items) {
        auto git = globalSamp.find(sname);
        if (git == globalSamp.end()) return;
        char k = git->second.kind;
        items.push_back("T:" + sname + "_tex");
        if (k == '2' || k == 'S' || k == 'C' || k == 'A') items.push_back("T:" + sname + "_smp");
    };
    auto declFor = [&](const std::string& item) -> std::string {
        if (item == "U:") return "constant TGLMTUniforms& tglmt_u";
        if (item.rfind("B:", 0) == 0) return "constant TGLMTUBO_" + item.substr(2) + "& ubo_" + item.substr(2);
        if (item.rfind("T:", 0) == 0) {
            std::string n = item.substr(2);
            if (n.size() > 4 && n.substr(n.size() - 4) == "_smp") return "sampler " + n;
            std::string base = (n.size() > 4 && n.substr(n.size() - 4) == "_tex")
                                   ? n.substr(0, n.size() - 4)
                                   : n;
            char k = '2';
            std::string elem = "float";
            auto git = globalSamp.find(base);
            if (git != globalSamp.end()) { k = git->second.kind; elem = git->second.elem; }
            return SamplerTexType(k, elem) + " " + n;
        }
        return "";
    };
    auto argFor = [&](const std::string& item) -> std::string {
        if (item == "U:") return "tglmt_u";
        if (item.rfind("B:", 0) == 0) return "ubo_" + item.substr(2);
        if (item.rfind("T:", 0) == 0) return item.substr(2);
        return "";
    };
    // ---- direct req (trên bodies gốc) ----
    for (auto& d : defs) {
        if (!d.hasBody) continue;
        std::vector<std::string> own = d.formalNames;
        for (auto& [fn, k] : d.splitKind) {
            (void)k;
            own.push_back(fn + "_tex");
            own.push_back(fn + "_smp");
        }
        auto isOwn = [&](const std::string& w) {
            for (auto& o : own)
                if (o == w) return true;
            return false;
        };
        std::vector<std::string> mine;
        if (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, "tglmt_u")) mine.push_back("U:");
        for (auto& ub : uboBlocks)
            if (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, "ubo_" + ub)) mine.push_back("B:" + ub);
        for (auto& [sn, se] : globalSamp) {
            (void)se;
            bool used = (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, sn + "_tex") && !isOwn(sn + "_tex")) ||
                        (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, sn + "_smp") && !isOwn(sn + "_smp"));
            if (used) {
                std::vector<std::string> items;
                globalPairItems(sn, items);
                for (auto& it : items) {
                    std::string nm = it.substr(2);
                    if (!isOwn(nm)) mine.push_back(it);
                }
            }
        }
        for (const char* vw : {"_in", "_out", "tglmt_vertexID", "tglmt_instanceID",
                               "tglmt_pointCoord", "tglmt_fragCoord", "gl_FrontFacing"}) {
            if (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, vw)) {
                err = std::string("helper ") + d.name + " dùng " + vw + " trực tiếp (phải truyền param)";
                return false;
            }
        }
        std::sort(mine.begin(), mine.end());
        mine.erase(std::unique(mine.begin(), mine.end()), mine.end());
        d.req = mine;
    }
    // ---- calls: collect + resolve overload + inherit (fixpoint) ----
    struct CallSite {
        int scope; // -1 body, else def idx
        size_t lparen, rparen;
        std::string callee;
        std::vector<std::string> actuals;
        Def* target = nullptr;
    };
    auto collectFor = [&](const std::string& text, size_t from, size_t to, int scope, bool isBody,
                          const std::string& cname, std::vector<CallSite>& out) {
        size_t p = from;
        while ((p = text.find(cname, p)) != std::string::npos && p < to) {
            bool l = p > 0 && IsIdentChar(text[p - 1], false);
            size_t e = p + cname.size();
            bool r = e < text.size() && IsIdentChar(text[e], false);
            if (l || r) { p = e; continue; }
            size_t q = e;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            if (q >= text.size() || text[q] != '(') { p = e; continue; }
            int dd = 0;
            size_t c = q;
            for (; c < text.size() && c < to; ++c) {
                if (text[c] == '(') ++dd;
                else if (text[c] == ')') {
                    if (--dd == 0) break;
                }
            }
            if (c >= text.size() || c >= to) break;
            // depth tại p: definition header (theo sau `{`) luôn bỏ.
            // Prototype (`;` ở depth 0) chỉ tồn tại trong helpersMSL, không có
            // trong body (body là ruột main) nên chỉ loại ở đó.
            int depth = 0;
            for (size_t k = from; k < p; ++k) {
                if (text[k] == '{') ++depth;
                else if (text[k] == '}') --depth;
            }
            size_t b = c + 1;
            while (b < text.size() && isspace((unsigned char)text[b])) ++b;
            if (b < text.size() && text[b] == '{') { p = c + 1; continue; }
            if (!isBody && depth < 1 && b < text.size() && text[b] == ';') { p = c + 1; continue; }
            CallSite cs;
            cs.scope = scope;
            cs.lparen = q;
            cs.rparen = c;
            cs.callee = cname;
            cs.actuals = SplitCallArgs(text.substr(q + 1, c - q - 1));
            if (cs.actuals.size() == 1 && Trim(cs.actuals[0]).empty()) cs.actuals.clear();
            out.push_back(cs);
            p = c + 1;
        }
    };
    std::vector<CallSite> calls;
    {
        // mọi tên callee đã biết
        std::vector<std::string> names;
        for (auto& d : defs) names.push_back(d.name);
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        for (auto& nm : names) collectFor(body, 0, body.size(), -1, true, nm, calls);
        for (size_t di = 0; di < defs.size(); ++di) {
            if (!defs[di].hasBody) continue;
            for (auto& nm : names)
                collectFor(helpersMSL, defs[di].bodyStart, defs[di].bodyEnd + 1, (int)di, false, nm, calls);
        }
    }
    // resolve overload từng call (theo arity) + inherit req (fixpoint)
    {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& cs : calls) {
                Def* target = nullptr;
                for (auto& d : defs)
                    if (d.name == cs.callee && (int)cs.actuals.size() == d.origCount) {
                        if (target) {
                            err = "helper " + cs.callee + " gọi mơ hồ (trùng arity n=" +
                                  std::to_string(cs.actuals.size()) + " defs=";
                            for (auto& d2 : defs)
                                if (d2.name == cs.callee)
                                    err += d2.key + "@" + std::to_string(d2.lp) + " ";
                            err += ")";
                            return false;
                        }
                        target = &d;
                    }
                if (!target) {
                    err = "helper " + cs.callee + " sai arity (co " +
                          std::to_string(cs.actuals.size()) + ")";
                    return false;
                }
                cs.target = target;
                // scope names của caller (để biết actual đã có trong scope chưa)
                std::vector<std::string> scopeNames;
                if (cs.scope >= 0) {
                    for (auto& fn : defs[(size_t)cs.scope].formalNames) {
                        scopeNames.push_back(fn);
                        if (defs[(size_t)cs.scope].splitKind.count(fn)) {
                            scopeNames.push_back(fn + "_tex");
                            scopeNames.push_back(fn + "_smp");
                        }
                    }
                    // threaded names đã có cũng tính (fixpoint nhiều vòng)
                    for (auto& it : defs[(size_t)cs.scope].req) {
                        std::string a;
                        if (it == "U:") a = "tglmt_u";
                        else if (it.rfind("B:", 0) == 0) a = "ubo_" + it.substr(2);
                        else if (it.rfind("T:", 0) == 0) a = it.substr(2);
                        if (!a.empty()) scopeNames.push_back(a);
                    }
                }
                auto inScope = [&](const std::string& w) {
                    for (auto& n : scopeNames)
                        if (n == w) return true;
                    return false;
                };
                // inherit từng req item của target
                for (auto& item : target->req) {
                    // item từ formal sampler của target? → map qua actual
                    bool mapped = false;
                    if (item.rfind("T:", 0) == 0) {
                        std::string n = item.substr(2);
                        std::string base = n;
                        if (base.size() > 4 && (base.substr(base.size() - 4) == "_tex" ||
                                                base.substr(base.size() - 4) == "_smp"))
                            base = base.substr(0, base.size() - 4);
                        auto sk = target->splitKind.find(base);
                        if (sk != target->splitKind.end() &&
                            globalSamp.find(base) == globalSamp.end()) {
                            // formal của target: tìm vị trí → actual E
                            int fidx = -1;
                            for (size_t fi = 0; fi < target->formalNames.size(); ++fi)
                                if (target->formalNames[fi] == base) fidx = (int)fi;
                            if (fidx < 0 || (size_t)fidx >= cs.actuals.size()) {
                                err = "map formal thất bại: " + base;
                                return false;
                            }
                            std::string E = Trim(cs.actuals[(size_t)fidx]);
                            if (!isBareName(E)) {
                                err = "arg sampler phức tạp ngoài subset: " + E;
                                return false;
                            }
                            // E trong scope caller?
                            if (cs.scope >= 0) {
                                bool own = false;
                                for (auto& sn : scopeNames)
                                    if (sn == E || sn == E + "_tex" || sn == E + "_smp") {
                                        own = true;
                                        break;
                                    }
                                bool callerSplit = defs[(size_t)cs.scope].splitKind.count(E) > 0;
                                if (callerSplit || own) continue;
                            }
                            // E global sampler → caller cần pair của E
                            auto git = globalSamp.find(E);
                            if (git == globalSamp.end()) {
                                err = "sampler không rõ nguồn: " + E;
                                return false;
                            }
                            std::vector<std::string> items;
                            globalPairItems(E, items);
                            for (auto& it2 : items) {
                                if (cs.scope < 0) continue;
                                auto& vv = defs[(size_t)cs.scope].req;
                                if (std::find(vv.begin(), vv.end(), it2) == vv.end()) {
                                    vv.push_back(it2);
                                    changed = true;
                                }
                            }
                            mapped = true;
                        }
                    }
                    if (mapped) continue;
                    if (cs.scope < 0) continue; // main có sẵn mọi global
                    {
                        auto& vv = defs[(size_t)cs.scope].req;
                        if (std::find(vv.begin(), vv.end(), item) == vv.end()) {
                            vv.push_back(item);
                            changed = true;
                        }
                    }
                    (void)inScope;
                }
            }
            for (auto& d : defs) {
                std::sort(d.req.begin(), d.req.end());
                d.req.erase(std::unique(d.req.begin(), d.req.end()), d.req.end());
            }
        }
    }
    // ---- áp dụng edits: headers (split + append) ----
    {
        struct Edit {
            size_t l, r;
            std::string rep;
        };
        std::vector<Edit> edits;
        // parse lại spans header hiện tại (chưa mutate gì từ đầu phase → dùng defs)
        for (auto& d : defs) {
            // rebuild params: split sampler + append threaded req
            std::vector<std::string> pieces = SplitCallArgs(d.params);
            if (pieces.size() == 1 && Trim(pieces[0]).empty()) pieces.clear();
            std::string rebuilt;
            bool first = true;
            auto emitP = [&](const std::string& a) {
                if (!first) rebuilt += ", ";
                rebuilt += a;
                first = false;
            };
            for (auto& pp : pieces) {
                std::string pt = Trim(pp);
                if (pt.empty()) continue;
                std::string nm = ParamBaseName(pt);
                auto sk = d.splitKind.find(nm);
                if (sk != d.splitKind.end()) {
                    char k = sk->second;
                    std::string elem = "float";
                    auto se = d.splitElem.find(nm);
                    if (se != d.splitElem.end()) elem = se->second;
                    if (k == '2' || k == 'S' || k == 'C' || k == 'A') {
                        emitP(SamplerTexType(k, elem) + " " + nm + "_tex");
                        emitP("sampler " + nm + "_smp");
                    } else {
                        emitP(SamplerTexType(k, elem) + " " + nm + "_tex");
                    }
                    continue;
                }
                emitP(pt);
            }
            for (auto& item : d.req) {
                std::string dd2 = declFor(item);
                if (!dd2.empty()) emitP(dd2);
            }
            edits.push_back({d.lp + 1, d.rp, rebuilt});
        }
        std::sort(edits.begin(), edits.end(), [](const Edit& a, const Edit& b) { return a.l > b.l; });
        for (auto& e : edits) helpersMSL.replace(e.l, e.r - e.l, e.rep);
    }
    // ---- áp dụng edits: calls (split expand + append threaded) ----
    {
        struct E2 {
            size_t l, r;
            std::string rep;
        };
        // recompute vì headers đã đổi length: thu thập calls mới theo tên
        auto expandIn = [&](std::string& text, bool isBody) -> bool {
            struct EE {
                size_t l, r;
                std::string rep;
            };
            std::vector<EE> edits;
            // mọi tên helper đã biết
            std::vector<std::string> names;
            for (auto& d : defs) names.push_back(d.name);
            std::sort(names.begin(), names.end());
            names.erase(std::unique(names.begin(), names.end()), names.end());
            for (auto& nm : names) {
                size_t p = 0;
                while ((p = text.find(nm, p)) != std::string::npos) {
                    bool l = p > 0 && IsIdentChar(text[p - 1], false);
                    size_t e = p + nm.size();
                    bool r = e < text.size() && IsIdentChar(text[e], false);
                    if (l || r) { p = e; continue; }
                    size_t q = e;
                    while (q < text.size() && isspace((unsigned char)text[q])) ++q;
                    if (q >= text.size() || text[q] != '(') { p = e; continue; }
                    int dd = 0;
                    size_t c = q;
                    for (; c < text.size(); ++c) {
                        if (text[c] == '(') ++dd;
                        else if (text[c] == ')') {
                            if (--dd == 0) break;
                        }
                    }
                    if (c >= text.size()) break;
                    // bỏ def headers (theo sau là `{`). Prototype (`;` depth 0)
                    // chỉ tồn tại trong helpersMSL — body là ruột main nên mọi
                    // `name(` ở đó đều là call thật.
                    size_t b = c + 1;
                    while (b < text.size() && isspace((unsigned char)text[b])) ++b;
                    // depth tại p
                    int depth = 0;
                    for (size_t k = 0; k < p; ++k) {
                        if (text[k] == '{') ++depth;
                        else if (text[k] == '}') --depth;
                    }
                    if (b < text.size() && text[b] == '{') { p = c + 1; continue; }
                    if (!isBody && depth < 1 && b < text.size() && text[b] == ';') { p = c + 1; continue; }
                    std::vector<std::string> actuals = SplitCallArgs(text.substr(q + 1, c - q - 1));
                    if (actuals.size() == 1 && Trim(actuals[0]).empty()) actuals.clear();
                    Def* target = nullptr;
                    int matchCount = 0;
                    for (auto& d : defs)
                        if (d.name == nm && (int)actuals.size() == d.origCount) {
                            if (target) {
                                err = "helper " + nm + " gọi mơ hồ (trùng arity kids=";
                                for (auto& d2 : defs)
                                    if (d2.name == nm) err += d2.key + " ";
                                err += ")";
                                return false;
                            }
                            target = &d;
                        }
                    if (!target) {
                        err = "helper " + nm + " sai arity (co " + std::to_string(actuals.size()) + ")";
                        return false;
                    }
                    std::string rebuilt;
                    size_t ai = 0;
                    bool first = true;
                    auto emitArg = [&](const std::string& a) {
                        if (!first) rebuilt += ", ";
                        rebuilt += a;
                        first = false;
                    };
                    for (auto& fn : target->formalNames) {
                        auto sk = target->splitKind.find(fn);
                        if (sk != target->splitKind.end()) {
                            if (ai >= actuals.size()) { err = "thiếu arg cho sampler " + fn; return false; }
                            std::string E = Trim(actuals[ai++]);
                            if (!isBareName(E)) { err = "arg sampler phức tạp ngoài subset: " + E; return false; }
                            emitArg(E + "_tex");
                            char kk = sk->second;
                            if (kk == '2' || kk == 'S' || kk == 'C' || kk == 'A') emitArg(E + "_smp");
                            continue;
                        }
                        if (ai >= actuals.size()) { err = "thiếu arg cho " + fn; return false; }
                        emitArg(actuals[ai++]);
                    }
                    if (ai != actuals.size()) { err = "thừa arg khi gọi " + target->name; return false; }
                    for (auto& item : target->req) {
                        std::string a = argFor(item);
                        if (!a.empty()) emitArg(a);
                    }
                    edits.push_back({q + 1, c, rebuilt});
                    p = c + 1;
                }
            }
            std::sort(edits.begin(), edits.end(), [](const EE& a, const EE& b) { return a.l > b.l; });
            for (auto& e : edits) text.replace(e.l, e.r - e.l, e.rep);
            return true;
        };
        if (!expandIn(body, true)) return false;
        if (!expandIn(helpersMSL, false)) return false;
    }
    return true;
}
// Tìm thân main: "void main" + '(' ... ')' + '{' ... '}' cân bằng. Trả inner.
static bool ExtractMainBody(const std::string& src, std::string& inner, std::string& err) {
    size_t p = src.find("void");
    while (p != std::string::npos) {
        size_t q = p + 4;
        while (q < src.size() && isspace((unsigned char)src[q])) ++q;
        if (src.compare(q, 4, "main") == 0) {
            size_t r = q + 4;
            while (r < src.size() && isspace((unsigned char)src[r])) ++r;
            if (r < src.size() && src[r] == '(') {
                int d = 0;
                size_t k = r;
                for (; k < src.size(); ++k) {
                    if (src[k] == '(') ++d;
                    else if (src[k] == ')') { if (--d == 0) break; }
                }
                size_t b = src.find('{', k);
                if (b == std::string::npos) { err = "main thiếu thân {"; return false; }
                int dd = 0;
                for (size_t m = b; m < src.size(); ++m) {
                    if (src[m] == '{') ++dd;
                    else if (src[m] == '}') {
                        if (--dd == 0) { inner = src.substr(b + 1, m - b - 1); return true; }
                    }
                }
                err = "main ngoặc {} không cân bằng";
                return false;
            }
        }
        p = src.find("void", p + 1);
    }
    err = "không tìm thấy 'void main' (GLSL 4.60 §6.1)";
    return false;
}

} // namespace

GLSLConvertResult ConvertGLSLtoMSL(const std::string& glsl, uint32_t stage) {
    GLSLConvertResult R;
    R.isVertex = (stage == 0x8B31);
    R.entryPoint = R.isVertex ? "TGLMT_vs" : "TGLMT_fs";
    auto fail = [&](const std::string& m) { R.ok = false; R.log = m; return R; };

    std::string src = StripComments(glsl);
    // Tiền xử lý: #version, #define/#undef (object-like, kiểu Blaze3D prepend),
    // #ifdef/#ifndef/#else/#endif, #line (bỏ), #pragma/#extension (bỏ), precision (bỏ).
    // Blaze3D tiền tố mọi shader vanilla bằng defines biến thể + #line nên thiếu
    // bước này là fail hàng loạt (đã quan sát trên máy).
    {
        std::istringstream iss(src);
        std::string line, kept;
        bool sawVersion = false;
        std::unordered_map<std::string, std::string> macros;
        std::vector<bool> activeStack; // nhánh hiện tại có emit không
        std::vector<bool> takenStack;  // level này đã chạy nhánh nào chưa (cho elif/else)
        auto active = [&]() {
            for (bool b : activeStack) if (!b) return false;
            return true;
        };
        auto evalIf = [&](const std::string& expr) -> std::pair<bool, bool> {
            std::string e = Trim(expr);
            if (e.rfind("defined", 0) == 0) {
                size_t lp = e.find('('), rp = e.find(')');
                std::string nm = (lp != std::string::npos && rp != std::string::npos && rp > lp)
                    ? Trim(e.substr(lp + 1, rp - lp - 1)) : Trim(e.substr(7));
                return {macros.count(nm) > 0, true};
            }
            if (e.rfind("!defined", 0) == 0) {
                size_t lp = e.find('('), rp = e.find(')');
                std::string nm = (lp != std::string::npos && rp != std::string::npos && rp > lp)
                    ? Trim(e.substr(lp + 1, rp - lp - 1)) : Trim(e.substr(8));
                return {macros.count(nm) == 0, true};
            }
            // số nguyên trần: #if 0 / #if 1
            if (!e.empty() && (isdigit((unsigned char)e[0]) || e[0] == '-')) {
                return {atoi(e.c_str()) != 0, true};
            }
            // macro đơn trị số: #if ALPHA_CUTOUT (sau define số)
            auto it = macros.find(e);
            if (it != macros.end() && !it->second.empty()) {
                const std::string& v = Trim(it->second);
                if (!v.empty() && (isdigit((unsigned char)v[0]) || v[0] == '-' || v[0] == '.'))
                    return {atof(v.c_str()) != 0.0, true};
            }
            return {false, false}; // biểu thức lạ → fail trung thực ở caller
        };
        while (std::getline(iss, line)) {
            std::string t = Trim(line);
            if (!t.empty() && t[0] == '#') {
                std::istringstream ds(t.substr(1));
                std::string dir; ds >> dir;
                std::string restLine;
                std::getline(ds, restLine);
                restLine = Trim(restLine);
                if (dir == "version") {
                    sawVersion = true;
                    if (t.find("300") == std::string::npos && t.find("460") == std::string::npos &&
                        t.find("450") == std::string::npos && t.find("440") == std::string::npos &&
                        t.find("410") == std::string::npos && t.find("400") == std::string::npos &&
                        t.find("330") == std::string::npos && t.find("150") == std::string::npos &&
                        t.find("140") == std::string::npos && t.find("130") == std::string::npos &&
                        t.find("120") == std::string::npos && t.find("100") == std::string::npos)
                        return fail("phiên bản GLSL không nhận diện: " + t);
                    continue; // bỏ dòng version
                }
                if (dir == "line" || dir == "pragma" || dir == "extension") continue; // bỏ
                if (dir == "define") {
                    if (!active()) continue;
                    std::istringstream ms(restLine);
                    std::string nm; ms >> nm;
                    std::string val; std::getline(ms, val); val = Trim(val);
                    if (nm.empty()) return fail("define thiếu tên: " + t);
                    if (nm.find('(') != std::string::npos)
                        return fail("define hàm ngoài subset: " + t);
                    macros[nm] = val; // val rỗng = flag (dùng với ifdef)
                    continue;
                }
                if (dir == "undef") {
                    if (active()) macros.erase(restLine);
                    continue;
                }
                if (dir == "ifdef" || dir == "ifndef") {
                    bool parent = active();
                    bool cond = dir == "ifdef" ? (macros.count(restLine) > 0)
                                               : (macros.count(restLine) == 0);
                    activeStack.push_back(parent && cond);
                    takenStack.push_back(parent && cond);
                    continue;
                }
                if (dir == "if") {
                    bool parent = active();
                    auto [v, ok] = evalIf(restLine);
                    if (!ok) return fail("#if ngoài subset: " + t);
                    activeStack.push_back(parent && v);
                    takenStack.push_back(parent && v);
                    continue;
                }
                if (dir == "elif") {
                    if (activeStack.empty()) return fail("#elif lạc: " + t);
                    bool parent = true;
                    for (size_t k = 0; k + 1 < activeStack.size(); ++k)
                        if (!activeStack[k]) parent = false;
                    if (takenStack.back()) {
                        activeStack.back() = false; // nhánh trước đã chạy
                    } else if (parent) {
                        auto [v, ok] = evalIf(restLine);
                        if (!ok) return fail("#elif ngoài subset: " + t);
                        activeStack.back() = v;
                        takenStack.back() = v;
                    } else {
                        activeStack.back() = false;
                    }
                    continue;
                }
                if (dir == "else") {
                    if (activeStack.empty()) return fail("#else lạc: " + t);
                    bool parent = true;
                    for (size_t k = 0; k + 1 < activeStack.size(); ++k)
                        if (!activeStack[k]) parent = false;
                    activeStack.back() = parent && !takenStack.back();
                    takenStack.back() = true;
                    continue;
                }
                if (dir == "endif") {
                    if (activeStack.empty()) return fail("#endif lạc: " + t);
                    activeStack.pop_back();
                    takenStack.pop_back();
                    continue;
                }
                return fail("directive không hỗ trợ: " + t);
            }
            if (!active()) continue; // nhánh ifdef tắt
            if (t.rfind("precision", 0) == 0) continue; // MSL bỏ qua precision
            kept += line + "\n";
        }
        if (!activeStack.empty()) return fail("#ifdef/#if không đóng");
        if (!sawVersion) return fail("thiếu '#version' (GLSL yêu cầu)");
        // Thay macro (tên dài trước để tránh tiền tố, theo word-boundary).
        if (!macros.empty()) {
            std::vector<std::pair<std::string, std::string>> ms(macros.begin(), macros.end());
            std::sort(ms.begin(), ms.end(),
                      [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
            std::string out;
            size_t i = 0;
            while (i < kept.size()) {
                char ch = kept[i];
                if (ch == '"') { // giữ string literal
                    size_t j = kept.find('"', i + 1);
                    if (j == std::string::npos) j = kept.size() - 1;
                    out += kept.substr(i, j - i + 1);
                    i = j + 1;
                    continue;
                }
                if (IsIdentChar(ch, true)) {
                    size_t j = i + 1;
                    while (j < kept.size() && IsIdentChar(kept[j], false)) ++j;
                    std::string w = kept.substr(i, j - i);
                    bool hit = false;
                    for (auto& kv : ms)
                        if (kv.first == w) { out += kv.second; hit = true; break; }
                    if (!hit) out += w;
                    i = j;
                    continue;
                }
                out += ch;
                ++i;
            }
            kept = out;
        }
        src = kept;
    }
    // Quét khai báo top-level (bao gồm UBO blocks cho vanilla 1.17+/Sodium read-only)
    std::vector<GLSLVar> ins, outs, uniforms;
    std::vector<GLSLBlock> blocks;
    std::vector<HoistedGlobal> hoisted; // global mutable → main-local + thread-ref
    std::string rest; // phần còn lại (hàm, main, const global)
    {
        auto stmts = SplitTopLevel(src);
        // Tách main + hàm ra khỏi decl: xem splitSegments bên dưới (UBO dính
        // sau hàm được tách riêng, decl trước/sau hàm đều nhận).
        auto startsKw = [&](const std::string& t) {
            // Cho phép qualifier nội suy đứng trước in/out (flat/smooth/...):
            // `flat out vec4 X;` rất thường gặp ở vanilla (leash/lines).
            std::string u = t;
            for (const char* q : {"flat ", "smooth ", "noperspective ", "centroid ",
                                  "sample ", "invariant ", "precise "}) {
                if (u.rfind(q, 0) == 0) { u = Trim(u.substr(strlen(q))); break; }
            }
            return u.rfind("layout", 0) == 0 || u.rfind("in ", 0) == 0 ||
                   u.rfind("out ", 0) == 0 || u.rfind("uniform ", 0) == 0 ||
                   u.rfind("const ", 0) == 0;
        };
        auto tryParseUBO = [&](const std::string& t, GLSLBlock& out) -> bool {
            std::string s = Trim(t);
            if (s.rfind("layout", 0) != 0 && s.rfind("uniform", 0) != 0) return false;
            size_t ub = s.find("uniform");
            if (ub == std::string::npos) return false;
            size_t brace = s.find('{', ub);
            if (brace == std::string::npos) return false;
            size_t close = s.find('}', brace);
            if (close == std::string::npos) return false;
            // tên block: từ sau "uniform" tới '{'
            std::string hdr = Trim(s.substr(ub + 7, brace - ub - 7));
            if (hdr.empty()) return false;
            // hdr có thể là "BlockName" (block không instance) — instance named UBO
            // (`} inst;`) sẽ được flatten thành `inst.member` ở body rewrite (M5c tách sau).
            std::string tail = Trim(s.substr(close + 1));
            if (!tail.empty() && tail.back() == ';') tail.pop_back();
            tail = Trim(tail);
            // tail rỗng = không instance, tail = tên instance (không mảng instance ở vanilla)
            if (tail.find('[') != std::string::npos) return false;
            std::string inner = s.substr(brace + 1, close - brace - 1);
            // tách members theo ';'
            std::vector<GLSLVar> mems;
            std::istringstream mss(inner);
            std::string seg;
            while (std::getline(mss, seg, ';')) {
                std::string mt = Trim(seg);
                if (mt.empty()) continue;
                std::istringstream mi(mt);
                std::vector<std::string> mw; std::string tk;
                while (mi >> tk) mw.push_back(tk);
                if (mw.size() < 2) return false;
                // bỏ qualifier precision
                size_t kk = 0;
                while (kk < mw.size() && (mw[kk] == "highp" || mw[kk] == "mediump" || mw[kk] == "lowp"))
                    ++kk;
                if (kk + 1 >= mw.size()) return false;
                std::string mtype = mw[kk];
                std::string mname = mw[kk + 1];
                for (size_t q = kk + 2; q < mw.size(); ++q) mname += mw[q];
                int arr = 0;
                size_t lb = mname.find('[');
                if (lb != std::string::npos) {
                    size_t rb = mname.find(']', lb);
                    if (rb == std::string::npos) return false;
                    arr = atoi(mname.substr(lb + 1, rb - lb - 1).c_str());
                    if (arr <= 0 || arr > 1024) return false;
                    mname = Trim(mname.substr(0, lb));
                }
                bool okT = false;
                std::string mm = MSLType(mtype, okT);
                if (!okT) return false;
                GLSLVar mv; mv.glslType = mtype; mv.mslType = mm;
                mv.name = tail.empty() ? mname : tail + "." + mname;
                mv.arraySize = arr;
                mems.push_back(mv);
            }
            if (mems.empty()) return false;
            out.name = hdr;
            out.members = mems;
            return true;
        };
        // Hàng đợi xử lý: SplitTopLevel cắt theo `;` depth 0 nên `hàm...} decl;`
        // dính nhau; splitSegments bên dưới tách tiếp theo khối cân bằng.
        std::vector<std::string> queue;
        for (auto& st : stmts) queue.push_back(st.text);
        // Tách stmt thành segment theo khối `{...}` cân bằng ở depth 0:
        // `func(){} uniform X;` hay `func(){} UBO{...};` (UBO dính sau hàm
        // trước — rất thường gặp khi inline include) thành từng phần riêng.
        // Không tách được kiểu này là fail hàng loạt trên máy (đã quan sát).
        auto splitSegments = [](const std::string& t, std::vector<std::string>& segs) {
            size_t start = 0, i = 0;
            int depth = 0;
            bool inStr = false;
            // UBO `} name;`: block layout/uniform giữ luôn `name;` cùng segment.
            auto segStartsUbo = [&](size_t s) {
                size_t p = s;
                while (p < t.size() && isspace((unsigned char)t[p])) ++p;
                return (t.compare(p, 6, "layout") == 0 || t.compare(p, 7, "uniform") == 0);
            };
            auto flushTo = [&](size_t end) {
                std::string s = Trim(t.substr(start, end - start));
                if (!s.empty() && s != ";") segs.push_back(s);
            };
            while (i < t.size()) {
                char ch = t[i];
                if (inStr) { if (ch == '"') inStr = false; ++i; continue; }
                if (ch == '"') { inStr = true; ++i; continue; }
                if (ch == '{') ++depth;
                else if (ch == '}') {
                    --depth;
                    if (depth == 0) {
                        size_t end = i + 1;
                        size_t j = end;
                        while (j < t.size() && isspace((unsigned char)t[j])) ++j;
                        if (j < t.size() && t[j] == ';') {
                            end = j + 1; // `};` đi cùng block
                        } else if (segStartsUbo(start) && j < t.size() && IsIdentChar(t[j], true)) {
                            // `} lightmapInfo;`: instance name thuộc về UBO
                            size_t k = j + 1;
                            while (k < t.size() && IsIdentChar(t[k], false)) ++k;
                            size_t m = k;
                            while (m < t.size() && isspace((unsigned char)t[m])) ++m;
                            if (m < t.size() && t[m] == ';') end = m + 1;
                        }
                        flushTo(end);
                        start = end;
                        i = end;
                        continue;
                    }
                }
                ++i;
            }
            flushTo(t.size());
        };
        for (size_t qi = 0; qi < queue.size(); ++qi) {
            std::string t0 = Trim(queue[qi]);
            if (t0.empty() || t0 == ";") continue;
            std::vector<std::string> segs;
            splitSegments(queue[qi], segs);
            if (segs.empty()) continue;
            for (std::string t : segs) {
                t = Trim(t);
                if (t.empty() || t == ";") continue;
            // UBO block (chứa '{' nhưng là decl)
            {
                GLSLBlock b;
                if ((t.rfind("layout", 0) == 0 || t.rfind("uniform", 0) == 0) &&
                    t.find('{') != std::string::npos && tryParseUBO(t, b)) {
                    blocks.push_back(b);
                    continue;
                }
            }
            if (t.find('{') != std::string::npos) {
                // nguyên khối hàm/struct → rest (struct member types được viết
                // lại ở RewriteIdents bên dưới cùng rest)
                rest += t;
                if (!t.empty() && t.back() != ';' && t.back() != '}') rest += ";";
                rest += "\n";
                continue;
            }
            // Global mutable (post OIT scratchpad: `T name [= init];`): Metal cấm
            // global mutable nên hoist thành main-local; helper nào dùng chung
            // nhận thêm param thread-ref (xử lý sau khi trích body).
            {
                std::string mm = t;
                if (!mm.empty() && mm.back() == ';') mm.pop_back();
                mm = Trim(mm);
                std::istringstream ms(mm);
                std::string w0;
                ms >> w0;
                bool hOk = false;
                std::string hMt = MSLType(w0, hOk);
                static const char* kStmtKw[] = {"if",  "for",  "while", "return", "void",
                                                "struct", "break", "continue", "discard",
                                                "else", "do", "switch", nullptr};
                bool reserved = false;
                for (const char** kw = kStmtKw; *kw; ++kw)
                    if (w0 == *kw) reserved = true;
                if (hOk && !reserved && hMt != "sampler") {
                    std::string mrest = Trim(mm.substr(w0.size()));
                    std::vector<std::pair<std::string, int>> ds;
                    if (!mrest.empty() && SplitDeclarators(mrest, ds)) {
                        for (auto& [nm, arr] : ds) {
                            HoistedGlobal hg;
                            hg.glslType = w0;
                            hg.mslType = hMt;
                            hg.name = nm;
                            hg.arraySize = arr;
                            hg.init = ExtractDeclaratorInit(mrest, nm);
                            hoisted.push_back(hg);
                        }
                        continue;
                    }
                }
            }
            // Khai báo: layout(...) có ngoặc nhưng vẫn là decl, const có initializer.
            if (!startsKw(t)) {
                if (t.find('}') != std::string::npos ||
                    t.find("void") != std::string::npos || t.find('(') != std::string::npos) {
                    rest += t;
                    if (!t.empty() && t.back() != ';' && t.back() != '}') rest += ";";
                    rest += "\n";
                    continue;
                }
                return fail("câu lệnh không hỗ trợ: " + t);
            }
            std::string dir;
            std::vector<GLSLVar> vs;
            bool isU = false;
            if (ParseDecls(t, R.isVertex, dir, vs, isU)) {
                for (auto& v : vs) {
                    bool okT = false;
                    if (isU && (v.glslType == "samplerBuffer" || v.glslType == "isamplerBuffer" ||
                                v.glslType == "usamplerBuffer")) {
                        // Buffer texture (CloudFaces mây): emulate bằng texture2d<int>
                        // + read(index) ở host (xem texelFetch bên dưới + BindBufTex).
                        v.mslType = "sampler";
                        v.isSampler = true;
                        v.isBuffer = true;
                        v.sampleType = (v.glslType == "isamplerBuffer") ? "int"
                                     : (v.glslType == "usamplerBuffer") ? "uint" : "float";
                        uniforms.push_back(v);
                        continue;
                    }
                    if (isU && (v.glslType == "sampler2D" || v.glslType == "sampler2DShadow" ||
                                v.glslType == "samplerCube" || v.glslType == "sampler2DArray")) {
                        // samplerCube → texturecube (panorama); Array → texture2d_array;
                        // Shadow → texture2d approx (bỏ compare ref, xem texture() bên dưới).
                        v.mslType = "sampler";
                        v.isSampler = true;
                        v.isCube = (v.glslType == "samplerCube");
                        v.isArray = (v.glslType == "sampler2DArray");
                        v.isShadow = (v.glslType == "sampler2DShadow");
                        uniforms.push_back(v);
                        continue;
                    }
                    v.mslType = MSLType(v.glslType, okT);
                    if (!okT) return fail("kiểu không hỗ trợ: " + v.glslType + " (" + v.name + ")");
                    if (isU) {
                        uniforms.push_back(v);
                    } else if (dir == "in") {
                        ins.push_back(v);
                    } else {
                        outs.push_back(v);
                    }
                }
            } else if (!t.empty() && t != ";") {
                // const global (có thể nhiều declarator + mảng):
                // `const vec2[] P = vec2[](vec2(0,0), ...);` (animate_sprite)
                if (t.rfind("const", 0) == 0) {
                    std::string cbody = Trim(t.substr(5)); // bỏ 1 "const" (tránh `constant const`)
                    if (!cbody.empty() && cbody.back() == ';') cbody.pop_back();
                    // tách `T a = 1, b = 2` → head type + declarators
                    std::istringstream cs(cbody);
                    std::vector<std::string> cw; std::string ctk;
                    while (cs >> ctk) cw.push_back(ctk);
                    if (cw.empty()) return fail("khai báo không hỗ trợ: " + t);
                    std::string ctype = cw[0];
                    // mảng const: `vec2[]` → elem `vec2`
                    bool cIsArr = false;
                    if (ctype.size() > 2 && ctype.substr(ctype.size() - 2) == "[]") {
                        cIsArr = true;
                        ctype = ctype.substr(0, ctype.size() - 2);
                    }
                    std::string crest;
                    for (size_t q = 1; q < cw.size(); ++q) {
                        if (!crest.empty()) crest += " ";
                        crest += cw[q];
                    }
                    bool cok = false;
                    std::string cm = MSLType(ctype, cok);
                    if (!cok) return fail("kiểu không hỗ trợ: " + ctype);
                    std::vector<std::pair<std::string, int>> cds;
                    if (!SplitDeclarators(crest, cds)) return fail("khai báo không hỗ trợ: " + t);
                    for (auto& [cnm, carr] : cds) {
                        std::string init = ExtractDeclaratorInit(crest, cnm);
                        init = RewriteTypeNames(init);
                        if (cIsArr) {
                            if (carr != 0) return fail("mảng const 2 chiều ngoài subset: " + t);
                            // init dạng `vec2[](e1, e2, ...)` (đã RewriteTypeNames →
                            // `float2[](...)`) → đếm phần tử, phát `{...}`
                            size_t ob = init.find('(');
                            size_t cb = init.rfind(')');
                            if (ob == std::string::npos || cb == std::string::npos || cb <= ob)
                                return fail("init mảng const sai cú pháp: " + t);
                            std::string inner = init.substr(ob + 1, cb - ob - 1);
                            std::vector<std::string> elems;
                            {
                                int dd = 0;
                                size_t s0 = 0;
                                for (size_t k = 0; k <= inner.size(); ++k) {
                                    char ch = k < inner.size() ? inner[k] : ',';
                                    if (ch == '(') ++dd;
                                    else if (ch == ')') --dd;
                                    else if (ch == ',' && dd == 0) {
                                        std::string e2 = Trim(inner.substr(s0, k - s0));
                                        if (!e2.empty()) elems.push_back(e2);
                                        s0 = k + 1;
                                    }
                                }
                            }
                            if (elems.empty()) return fail("mảng const rỗng: " + t);
                            std::string joined;
                            for (size_t k = 0; k < elems.size(); ++k)
                                joined += (k ? ", " : "") + elems[k];
                            rest += "constant " + cm + " " + cnm + "[" + std::to_string(elems.size()) +
                                    "] = {" + joined + "};\n";
                        } else {
                            if (carr != 0) return fail("mảng const ngoài subset: " + t);
                            rest += "constant " + cm + " " + cnm + (init.empty() ? "" : " = " + init) + ";\n";
                        }
                    }
                } else {
                    return fail("khai báo không hỗ trợ: " + t);
                }
            } else {
                rest += t;
                if (!t.empty() && t.back() != ';' && t.back() != '}') rest += ";";
                rest += "\n";
            }
        } // for segs
        } // for queue
    } // block quét top-level
    // Kiểm tra ma trận làm attribute (Metal attribute tối đa float4)
    // Vanilla Blaze3D dùng `in vec3 Position;` KHÔNG layout → cho phép, linker gán
    // qua glBindAttribLocation/GetAttribLocation (đúng GL, trước đây fail oan).
    if (R.isVertex) {
        for (auto& a : ins) {
            if (a.mslType.find("float") == 0 && a.mslType.find("x") != std::string::npos)
                return fail("attribute ma trận không hỗ trợ (Metal attribute ≤ float4): " + a.name);
        }
    }
    // Varying/attribute thiếu location: linker gán ở LinkProgram (đúng GL).
    // Tạm đánh số 900+ordinal trong MSL, link viết lại số thật.
    {
        int tmp = 900;
        for (auto& v : outs)
            if (v.location < 0) v.location = tmp++;
        tmp = 900;
        for (auto& a : ins)
            if (a.location < 0) a.location = tmp++;
    }
    // Fragment: 1..8 out vec4 (MRT cho vanilla deferred/Iris; location = attachment).
    // Legacy gl_FragColor / gl_FragData[0] (end_portal compat): map về out duy nhất.
    bool legacyFrag = false;
    if (!R.isVertex) {
        if (outs.empty()) {
            auto hasWord = [&](const char* w) {
                size_t n = strlen(w), p = 0;
                while ((p = rest.find(w, p)) != std::string::npos) {
                    bool l = p > 0 && IsIdentChar(rest[p - 1], false);
                    size_t e = p + n;
                    bool r = e < rest.size() && IsIdentChar(rest[e], false);
                    if (!l && !r) return true;
                    p = e;
                }
                return false;
            };
            // gl_FragData[N>0] (MRT legacy) ngoài subset → fail rõ
            {
                size_t p = 0;
                while ((p = rest.find("gl_FragData", p)) != std::string::npos) {
                    size_t b = rest.find('[', p);
                    size_t e2 = (b == std::string::npos) ? std::string::npos : rest.find(']', b);
                    if (b != std::string::npos && e2 != std::string::npos) {
                        int idx = atoi(rest.substr(b + 1, e2 - b - 1).c_str());
                        if (idx != 0) return fail("gl_FragData[N>0] ngoài subset");
                    }
                    p += 11;
                }
            }
            if (hasWord("gl_FragColor") || hasWord("gl_FragData")) {
                legacyFrag = true;
            } else {
                return fail("fragment cần 1..8 out (hiện có 0)");
            }
        } else if (outs.size() > 8) {
            return fail("fragment cần 1..8 out (hiện có " + std::to_string(outs.size()) + ")");
        }
        for (auto& o : outs)
            if (o.mslType != "float4") return fail("fragment out phải vec4 (" + o.name + ")");
        // location trùng → link lỗi (đúng GL)
        for (size_t i = 0; i < outs.size(); ++i)
            for (size_t j = i + 1; j < outs.size(); ++j)
                if (outs[i].location >= 0 && outs[i].location == outs[j].location)
                    return fail("fragment out location trùng: " + std::to_string(outs[i].location));
    }
    // Uniform layout: natural alignment MSL (xem MSLAlign/MSLSize).
    // Array: stride = align_up(elemSize, elemAlign), total = stride*count.
    // vec3/float array stride 16/4 đúng MSL; upload tight→padded ở AppleDrawGL nếu cần.
    {
        size_t off = 0;
        for (auto& u : uniforms) {
            if (u.isSampler) continue;
            if (u.glslType == "sampler2D" || u.glslType == "sampler2DShadow" ||
                u.glslType == "samplerCube" || u.glslType == "sampler2DArray")
                continue;
            size_t al = MSLAlign(u.mslType);
            size_t sz = MSLSize(u.mslType);
            if (!sz) return fail("uniform kiểu lạ: " + u.glslType);
            off = (off + al - 1) & ~(al - 1);
            u.uniformOffset = off;
            if (u.arraySize > 0) {
                size_t stride = (sz + al - 1) & ~(al - 1);
                u.uniformSize = stride * (size_t)u.arraySize;
                off += u.uniformSize;
            } else {
                u.uniformSize = sz;
                off += sz;
            }
        }
        R.uniformBufferSize = (off + 15) & ~((size_t)15);
    }
    // UBO blocks layout (read-only, MSL natural giống default block để tái dùng upload path)
    // Dedupe: Blaze3D inline #moj_import bằng text nên cùng include có thể vào
    // 2 lần (end_portal.vsh import projection.glsl 2 lần). Trùng tên + trùng
    // members thì giữ bản đầu (đúng GL: cùng block khai báo 2 lần vẫn link được
    // nếu giống hệt); khác members là redef thật → fail trung thực.
    {
        std::vector<GLSLBlock> uniq;
        for (auto& b : blocks) {
            bool seen = false;
            for (auto& u : uniq) {
                if (u.name != b.name) continue;
                seen = true;
                if (u.members.size() != b.members.size()) {
                    return fail("UBO " + b.name + " khai báo lại khác members");
                }
                for (size_t mi = 0; mi < u.members.size(); ++mi) {
                    if (u.members[mi].name != b.members[mi].name ||
                        u.members[mi].glslType != b.members[mi].glslType ||
                        u.members[mi].arraySize != b.members[mi].arraySize) {
                        return fail("UBO " + b.name + " khai báo lại khác members");
                    }
                }
                break; // trùng hệt → bỏ bản sau
            }
            if (!seen) uniq.push_back(b);
        }
        blocks.swap(uniq);
    }
    for (auto& b : blocks) {
        size_t off = 0;
        for (auto& m : b.members) {
            size_t al = MSLAlign(m.mslType);
            size_t sz = MSLSize(m.mslType);
            if (!sz) return fail("UBO member kiểu lạ: " + m.glslType);
            off = (off + al - 1) & ~(al - 1);
            m.uniformOffset = off;
            if (m.arraySize > 0) {
                size_t stride = (sz + al - 1) & ~(al - 1);
                m.uniformSize = stride * (size_t)m.arraySize;
                off += m.uniformSize;
            } else {
                m.uniformSize = sz;
                off += sz;
            }
        }
        b.bufferSize = (off + 15) & ~((size_t)15);
    }
    // Tách sampler (mọi loại sampler)
    for (auto& u : uniforms)
        if (u.isSampler) R.samplers.push_back(u);
    R.uniforms = uniforms;
    R.inputs = ins;
    R.outputs = outs;
    R.blocks = blocks;

    // Thân main
    std::string body;
    {
        std::string err;
        if (!ExtractMainBody(rest, body, err)) return fail(err);
    }
    // Hoist global mutable (post OIT scratchpad) về main-local + thread-ref
    // cho helper. Metal cấm global mutable; pattern này chỉ gặp ở post shader
    // (transparency) — ngoài đó code bên dưới bỏ qua (hoisted rỗng).
    if (!hoisted.empty()) {
        // 1. Tách main khỏi rest → helpersOnly (giữ consts + helpers, mất main).
        std::string helpersOnly = rest;
        {
            size_t p = helpersOnly.find("void");
            while (p != std::string::npos) {
                size_t q = p + 4;
                while (q < helpersOnly.size() && isspace((unsigned char)helpersOnly[q])) ++q;
                if (helpersOnly.compare(q, 4, "main") == 0) {
                    size_t b = helpersOnly.find('{', q);
                    if (b != std::string::npos) {
                        int dd = 0;
                        size_t m = b;
                        for (; m < helpersOnly.size(); ++m) {
                            if (helpersOnly[m] == '{') ++dd;
                            else if (helpersOnly[m] == '}') {
                                if (--dd == 0) break;
                            }
                        }
                        if (m < helpersOnly.size()) {
                            helpersOnly.erase(p, m - p + 1);
                            break;
                        }
                    }
                }
                p = helpersOnly.find("void", p + 1);
            }
        }
        auto hasWordIn = [&](const std::string& hay, const std::string& w) {
            size_t n = w.size(), p = 0;
            while ((p = hay.find(w, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(hay[p - 1], false);
                size_t e = p + n;
                bool r = e < hay.size() && IsIdentChar(hay[e], false);
                if (!l && !r) return true;
                p = e;
            }
            return false;
        };
        struct Helper {
            std::string name, params, head;
            size_t lp = 0, rp = 0; // phạm vi ngoặc params trong helpersOnly
            size_t bodyStart = 0, bodyEnd = 0;
        };
        // 2. Parse helper definitions ở depth 0: `ret name(params) {`.
        std::vector<Helper> helpers;
        {
            size_t i = 0;
            int depth = 0;
            while (i < helpersOnly.size()) {
                if (helpersOnly[i] == '{') ++depth;
                else if (helpersOnly[i] == '}') --depth;
                // ứng viên header: `word word(...)` ở depth 0, không phải if/for/while/return
                if (depth == 0 && (isalpha((unsigned char)helpersOnly[i]) || helpersOnly[i] == '_')) {
                    size_t j = i;
                    while (j < helpersOnly.size() && IsIdentChar(helpersOnly[j], j == i)) ++j;
                    std::string w1 = helpersOnly.substr(i, j - i);
                    size_t k = j;
                    while (k < helpersOnly.size() && isspace((unsigned char)helpersOnly[k])) ++k;
                    size_t j2 = k;
                    while (j2 < helpersOnly.size() && IsIdentChar(helpersOnly[j2], j2 == k)) ++j2;
                    std::string w2 = helpersOnly.substr(k, j2 - k);
                    size_t m = j2;
                    while (m < helpersOnly.size() && isspace((unsigned char)helpersOnly[m])) ++m;
                    static const char* kKw[] = {"if", "for", "while", "return", "switch", nullptr};
                    bool isKw = false;
                    for (const char** kw = kKw; *kw; ++kw)
                        if (w1 == *kw) isKw = true;
                    if (!isKw && !w1.empty() && !w2.empty() && m < helpersOnly.size() && helpersOnly[m] == '(') {
                        int dd = 0;
                        size_t e = m;
                        for (; e < helpersOnly.size(); ++e) {
                            if (helpersOnly[e] == '(') ++dd;
                            else if (helpersOnly[e] == ')') {
                                if (--dd == 0) break;
                            }
                        }
                        if (e < helpersOnly.size()) {
                            size_t b = e + 1;
                            while (b < helpersOnly.size() && isspace((unsigned char)helpersOnly[b])) ++b;
                            if (b < helpersOnly.size() && helpersOnly[b] == '{') {
                                int d2 = 0;
                                size_t c = b;
                                for (; c < helpersOnly.size(); ++c) {
                                    if (helpersOnly[c] == '{') ++d2;
                                    else if (helpersOnly[c] == '}') {
                                        if (--d2 == 0) break;
                                    }
                                }
                                if (c < helpersOnly.size()) {
                                    Helper h;
                                    h.name = w2;
                                    h.params = helpersOnly.substr(m + 1, e - m - 1);
                                    h.head = helpersOnly.substr(i, e + 1 - i);
                                    h.lp = m;
                                    h.rp = e;
                                    h.bodyStart = b;
                                    h.bodyEnd = c;
                                    helpers.push_back(h);
                                    i = c + 1;
                                    depth = 0;
                                    continue;
                                }
                            }
                        }
                    }
                }
                ++i;
            }
        }
        // Tên param của helper (để loại shadow khi tìm global dùng chung).
        auto paramNames = [&](const std::string& params) {
            std::vector<std::string> ns;
            int pd = 0, bd = 0;
            size_t s0 = 0;
            auto push = [&](size_t a, size_t b) {
                std::string pc = Trim(params.substr(a, b - a));
                if (pc.empty()) return;
                size_t lb = pc.find('[');
                std::string core = Trim(lb == std::string::npos ? pc : pc.substr(0, lb));
                size_t sp = core.find_last_of(" \t*&");
                std::string nm = (sp == std::string::npos) ? core : Trim(core.substr(sp + 1));
                if (!nm.empty()) ns.push_back(nm);
            };
            for (size_t k = 0; k <= params.size(); ++k) {
                char ch = k < params.size() ? params[k] : ',';
                if (ch == '(') ++pd;
                else if (ch == ')') --pd;
                else if (ch == '[') ++bd;
                else if (ch == ']') --bd;
                else if (ch == ',' && pd == 0 && bd == 0) {
                    push(s0, k);
                    s0 = k + 1;
                }
            }
            return ns;
        };
        // 3. Direct uses (trừ param shadow) + call graph.
        std::map<std::string, std::vector<std::string>> directUses; // helper -> globals
        std::map<std::string, std::vector<std::string>> callees;    // helper -> helpers gọi
        for (auto& h : helpers) {
            std::string hb = helpersOnly.substr(h.bodyStart, h.bodyEnd - h.bodyStart + 1);
            std::vector<std::string> pns = paramNames(h.params);
            auto isParam = [&](const std::string& g) {
                for (auto& pn : pns)
                    if (pn == g) return true;
                return false;
            };
            for (auto& g : hoisted)
                if (!isParam(g.name) && hasWordIn(hb, g.name)) directUses[h.name].push_back(g.name);
            for (auto& o : helpers)
                if (o.name != h.name && hasWordIn(hb, o.name)) {
                    // gọi hàm o (word + `(` sau đó) — kiểm tra `(` để khỏi nhầm biến
                    size_t p = 0;
                    bool isCall = false;
                    while ((p = hb.find(o.name, p)) != std::string::npos) {
                        bool l = p > 0 && IsIdentChar(hb[p - 1], false);
                        size_t e = p + o.name.size();
                        bool r = e < hb.size() && IsIdentChar(hb[e], false);
                        if (!l && !r) {
                            size_t q = e;
                            while (q < hb.size() && isspace((unsigned char)hb[q])) ++q;
                            if (q < hb.size() && hb[q] == '(') { isCall = true; break; }
                        }
                        p = e;
                    }
                    if (isCall) callees[h.name].push_back(o.name);
                }
        }
        // 4. Fixpoint required sets (truyền qua).
        std::map<std::string, std::vector<std::string>> req = directUses;
        {
            bool changed = true;
            while (changed) {
                changed = false;
                for (auto& h : helpers) {
                    auto it = callees.find(h.name);
                    if (it == callees.end()) continue;
                    for (auto& cn : it->second) {
                        auto jt = req.find(cn);
                        if (jt == req.end()) continue;
                        for (auto& g : jt->second) {
                            auto& v = req[h.name];
                            if (std::find(v.begin(), v.end(), g) == v.end()) {
                                v.push_back(g);
                                changed = true;
                            }
                        }
                    }
                }
            }
        }
        // Helper không dùng global nào vẫn cần khai báo để vòng dưới bỏ qua.
        for (auto& h : helpers)
            if (!req.count(h.name)) req[h.name] = {};
        // 5. Viết lại header helper: thêm `, thread T &g` / `, thread T (&g)[N]`.
        auto gByName = [&](const std::string& n) -> const HoistedGlobal* {
            for (auto& g : hoisted)
                if (g.name == n) return &g;
            return nullptr;
        };
        // Sắp xếp helper theo vị trí giảm dần: sửa header ở vị trí lớn không
        // làm lệch span của helper chưa xử lý (vị trí nhỏ hơn).
        std::sort(helpers.begin(), helpers.end(),
                  [](const Helper& a, const Helper& b) { return a.lp > b.lp; });
        for (auto& h : helpers) {
            auto& gs = req[h.name];
            if (gs.empty()) continue;
            std::vector<std::string> sg = gs;
            std::sort(sg.begin(), sg.end());
            std::string add;
            for (auto& gn : sg) {
                const HoistedGlobal* g = gByName(gn);
                if (!g) continue;
                add += ", thread " + g->mslType + (g->arraySize > 0
                                                        ? " (&" + gn + ")[" + std::to_string(g->arraySize) + "]"
                                                        : " &" + gn);
            }
            std::string oldParams = helpersOnly.substr(h.lp + 1, h.rp - h.lp - 1);
            std::string newParams = Trim(oldParams);
            newParams += add;
            helpersOnly.replace(h.lp + 1, h.rp - h.lp - 1, newParams);
        }
        // 6. Viết lại call sites trong body + helper bodies: `H(args)` → `H(args, g...)`.
        // Bỏ qua header definitions (đã chứa thread params).
        auto appendCallArgs = [&](std::string& text, size_t skipStart, size_t skipEnd,
                                  const std::string& hname, const std::vector<std::string>& gs) {
            if (gs.empty()) return;
            std::vector<std::string> sg = gs;
            std::sort(sg.begin(), sg.end());
            std::string add;
            for (auto& gn : sg) add += ", " + gn;
            size_t p = 0;
            while ((p = text.find(hname, p)) != std::string::npos) {
                if (p >= skipStart && p < skipEnd) {
                    p += hname.size();
                    continue;
                }
                bool l = p > 0 && IsIdentChar(text[p - 1], false);
                size_t e = p + hname.size();
                bool r = e < text.size() && IsIdentChar(text[e], false);
                if (l || r) {
                    p = e;
                    continue;
                }
                size_t q = e;
                while (q < text.size() && isspace((unsigned char)text[q])) ++q;
                if (q >= text.size() || text[q] != '(') {
                    p = e;
                    continue;
                }
                int dd = 0;
                size_t c = q;
                for (; c < text.size(); ++c) {
                    if (text[c] == '(') ++dd;
                    else if (text[c] == ')') {
                        if (--dd == 0) break;
                    }
                }
                if (c >= text.size()) break;
                text.insert(c, add);
                p = c + add.size() + 1;
            }
        };
        for (auto& h : helpers) {
            auto& gs = req[h.name];
            if (gs.empty()) continue;
            // calls trong body main
            appendCallArgs(body, 0, 0, h.name, gs);
            // calls trong helpers khác (kể cả chính nó — đệ quy truyền tiếp)
            // NOTE: helpersOnly đã bị đổi offset ở bước 5; parse lại span body cho an toàn:
        }
        // Parse lại spans sau bước 5 rồi rewrite calls trong helpersOnly.
        {
            // dựng lại danh sách body spans bằng cách tìm lại từng helper
            for (auto& h : helpers) {
                auto& gs = req[h.name];
                if (gs.empty()) continue;
                // tìm definition hiện tại của h trong helpersOnly
                size_t dp = 0;
                bool found = false;
                size_t defp = 0, depr = 0;
                while ((dp = helpersOnly.find(h.name, dp)) != std::string::npos) {
                    bool l = dp > 0 && IsIdentChar(helpersOnly[dp - 1], false);
                    size_t e = dp + h.name.size();
                    bool r = e < helpersOnly.size() && IsIdentChar(helpersOnly[e], false);
                    if (!l && !r) {
                        size_t q = e;
                        while (q < helpersOnly.size() && isspace((unsigned char)helpersOnly[q])) ++q;
                        if (q < helpersOnly.size() && helpersOnly[q] == '(') {
                            int dd = 0;
                            size_t c = q;
                            for (; c < helpersOnly.size(); ++c) {
                                if (helpersOnly[c] == '(') ++dd;
                                else if (helpersOnly[c] == ')') {
                                    if (--dd == 0) break;
                                }
                            }
                            size_t b = c + 1;
                            while (b < helpersOnly.size() && isspace((unsigned char)helpersOnly[b])) ++b;
                            if (b < helpersOnly.size() && helpersOnly[b] == '{') {
                                defp = dp;
                                depr = c + 1;
                                found = true;
                                break;
                            }
                        }
                    }
                    dp = e;
                }
                if (!found) continue;
                // rewrite mọi call EXCEPT [defp, depr)
                appendCallArgs(helpersOnly, defp, depr, h.name, gs);
            }
        }
        // 7. Phát locals vào đầu main body.
        {
            std::string decls;
            for (auto& g : hoisted) {
                std::string init = RewriteTypeNames(RewriteArrayCtors(g.init));
                std::string d = g.mslType + " " + g.name;
                if (g.arraySize > 0) d += "[" + std::to_string(g.arraySize) + "]";
                if (!init.empty()) d += " = " + init;
                decls += d + ";\n";
            }
            body = decls + body;
        }
        rest = helpersOnly;
    }
    // Phase A — sampler params của helpers (GLSL): fog/light helpers nhận
    // sampler/varying-explicit qua param. Ghi lại kind để scan + engine dùng.
    // helperSamp[helper] = [(paramName, entry)] theo thứ tự khai báo.
    std::map<std::string, std::vector<std::pair<std::string, SampEntry>>> helperSamp;
    {
        auto defs = ParseHelperDefs(rest, true);
        for (auto& h : defs) {
            std::vector<std::string> parts = SplitCallArgs(h.params);
            for (auto& pp : parts) {
                std::string pt = Trim(pp);
                if (pt.empty()) continue;
                std::istringstream ps(pt);
                std::string w1;
                ps >> w1;
                if (w1 == "const" || w1 == "in" || w1 == "out" || w1 == "inout") ps >> w1;
                char k = SamplerKindOf(w1);
                if (k == 0) continue;
                std::string nm = ParamBaseName(pt);
                if (nm.empty()) continue;
                SampEntry e;
                e.kind = k;
                e.elem = (k == 'I') ? "int" : (k == 'U' ? "uint" : "float");
                helperSamp[h.name].emplace_back(nm, e);
            }
        }
    }
    // Sampling calls trong helpers (rest): table = global + sampler params
    // của MỌI helper (first-wins khi trùng tên — đủ cho vanilla).
    {
        SampTable tab;
        for (auto& s : R.samplers) {
            SampEntry e;
            e.kind = s.isBuffer ? (s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B')
                     : s.isCube   ? 'C'
                     : s.isArray  ? 'A'
                     : s.isShadow ? 'S'
                                  : '2';
            e.elem = s.sampleType;
            tab[s.name] = e;
        }
        for (auto& [hn, vec] : helperSamp)
            for (auto& [nm, e] : vec)
                if (!tab.count(nm)) tab[nm] = e;
        std::string err;
        if (!RewriteSamplingCalls(rest, tab, err)) return fail(err);
    }
    // Bảng viết lại identifier — TÁCH ĐÔI:
    // mpVary (varying _in/_out, gl_*): chỉ cho main body. Helper nào dùng
    // varying trực tiếp là fail trung thực ở engine bên dưới (phải truyền param).
    // mpShared (uniforms, UBO, types, discard, sampler types): body + helpers.
    std::vector<std::pair<std::string, std::string>> mpVary, mpShared, mp;
    // NDC-z convert: GL clip z [-w,w] → Metal [0,w] (mặc định NEGATIVE_ONE_TO_ONE).
    // Chèn ` _out.position.z = _out.position.z*0.5 + _out.position.w*0.5;`
    // trước mọi return vertex (kể cả return sớm). ZERO_TO_ONE cần recompile (log ở link).
    const std::string kZConv = " _out.position.z = _out.position.z * 0.5 + _out.position.w * 0.5;";
    bool isMRT = !R.isVertex && outs.size() > 1;
    if (R.isVertex) {
        for (auto& a : ins) mpVary.emplace_back(a.name, "_in." + a.name);
        for (auto& v : outs) mpVary.emplace_back(v.name, "_out." + v.name);
        mpVary.emplace_back("gl_Position", "_out.position");
        mpVary.emplace_back("gl_PointSize", "_out.pointSize");
        if (body.find("gl_PointSize") != std::string::npos) R.usesPointSize = true;
        // screenquad (lightmap/blit): đỉnh suy từ index, không cần attribute.
        // GLSL int → MSL uint (bitwise tương đương).
        mpVary.emplace_back("gl_VertexID", "tglmt_vertexID");
        if (body.find("gl_VertexID") != std::string::npos) R.usesVertexID = true;
        mpVary.emplace_back("gl_InstanceID", "tglmt_instanceID");
        if (body.find("gl_InstanceID") != std::string::npos) R.usesInstanceID = true;
    } else {
        for (auto& v : ins) mpVary.emplace_back(v.name, "_in." + v.name);
        mpVary.emplace_back("gl_PointCoord", "tglmt_pointCoord");
        if (body.find("gl_PointCoord") != std::string::npos) R.usesPointCoord = true;
        mpVary.emplace_back("gl_FragCoord", "tglmt_fragCoord");
        if (body.find("gl_FragCoord") != std::string::npos) R.usesFragCoord = true;
        // gl_FrontFacing (entity.fsh nhánh PER_FACE_LIGHTING): Metal không có
        // builtin tương ứng trong body — phải là param [[front_facing]].
        // Không map là `use of undeclared identifier` trên máy (crash 26.1.2
        // 2026-09-28: 8 pipeline entity_*). Chỉ fragment mới có front-facing.
        mpVary.emplace_back("gl_FrontFacing", "tglmt_frontFacing");
        if (body.find("gl_FrontFacing") != std::string::npos) R.usesFrontFacing = true;
        if (isMRT) {
            for (auto& o : outs) mpVary.emplace_back(o.name, "_out.mrt" + std::to_string(o.location));
        } else if (legacyFrag) {
            // end_portal compat: gl_FragColor / gl_FragData[0] → out duy nhất
            mpVary.emplace_back("gl_FragColor", "tglmt_fragColor");
        } else {
            // out color: tên biến out → tglmt_fragColor (return ở cuối)
            mpVary.emplace_back(outs[0].name, "tglmt_fragColor");
        }
    }
    for (auto& u : uniforms) {
        if (u.isSampler) continue;
        mpShared.emplace_back(u.name, "tglmt_u." + u.name);
    }
    // UBO members: `member` hoặc `inst.member` → `ubo_Block.member`.
    // Instance name (`lightmapInfo` trong `} lightmapInfo;`) map thẳng sang
    // struct để `lightmapInfo.BlockFactor` thành `ubo_LightmapInfo.BlockFactor`.
    for (auto& b : blocks) {
        std::string inst;
        for (auto& m : b.members) {
            size_t dot = m.name.find('.');
            if (dot != std::string::npos && inst.empty()) inst = m.name.substr(0, dot);
        }
        if (!inst.empty()) mpShared.emplace_back(inst, "ubo_" + b.name);
        for (auto& m : b.members) {
            size_t dot = m.name.find('.');
            std::string access = (dot == std::string::npos) ? m.name : m.name.substr(dot + 1);
            mpShared.emplace_back(m.name, "ubo_" + b.name + "." + access);
        }
    }
    // Tên kiểu GLSL trong thân hàm → MSL nghiêm (runtime compiler không chấp
    // nhận vec3 như offline `metal -c` — đã quan sát). double không tồn tại ở MSL.
    // Sampler types đi mpShared để helper params (`sampler2D s`) cũng convert.
    mpShared.emplace_back("vec2", "float2");
    mpShared.emplace_back("vec3", "float3");
    mpShared.emplace_back("vec4", "float4");
    mpShared.emplace_back("ivec2", "int2");
    mpShared.emplace_back("ivec3", "int3");
    mpShared.emplace_back("ivec4", "int4");
    mpShared.emplace_back("uvec2", "uint2");
    mpShared.emplace_back("uvec3", "uint3");
    mpShared.emplace_back("uvec4", "uint4");
    mpShared.emplace_back("mat2", "float2x2");
    mpShared.emplace_back("mat3", "float3x3");
    mpShared.emplace_back("mat4", "float4x4");
    mpShared.emplace_back("sampler2D", "sampler");
    mpShared.emplace_back("sampler2DShadow", "sampler");
    mpShared.emplace_back("samplerCube", "texturecube<float>");
    mpShared.emplace_back("sampler2DArray", "texture2d_array<float>");
    mpShared.emplace_back("samplerBuffer", "texture2d<float>");
    mpShared.emplace_back("isamplerBuffer", "texture2d<int>");
    mpShared.emplace_back("usamplerBuffer", "texture2d<uint>");
    for (const char* bad : {"double", "dvec2", "dvec3", "dvec4", "dmat2", "dmat3", "dmat4"}) {
        auto hasWord = [&](const std::string& src) {
            size_t p = 0;
            while ((p = src.find(bad, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(src[p - 1], false);
                size_t e = p + strlen(bad);
                bool r = e < src.size() && IsIdentChar(src[e], false);
                if (!l && !r) return true;
                p = e;
            }
            return false;
        };
        if (hasWord(body)) return fail("double/dvec ngoài subset (MSL không có double)");
    }
    // `discard;` → `discard_fragment();` (giữ ngoặc! bản thiếu ngoặc thành no-op câm).
    // Shared để helper cũng discard được.
    mpShared.emplace_back("discard", "discard_fragment()");
    // Builtins đạo hàm GLSL → Metal (terrain LOD): dFdx/dFdy chỉ khác chữ hoa.
    mpShared.emplace_back("dFdx", "dfdx");
    mpShared.emplace_back("dFdy", "dfdy");
    // `radians`/`degrees`: Metal stdlib KHÔNG có (đã quan sát bằng metal thật:
    // `use of undeclared identifier 'radians'`). Hạ về helper nhân PI/180.
    // Lưu ý matrix.glsl có param tên `radians` (`mat2_rotate_z(float radians)`)
    // — map cả param lẫn call sang `tglmt_radians` nên vẫn nhất quán
    // (param `float tglmt_radians`, body `cos(tglmt_radians)`, call
    // `tglmt_radians(...)`).
    mpShared.emplace_back("radians", "tglmt_radians");
    mpShared.emplace_back("degrees", "tglmt_degrees");
    // mp gộp cho main body = vary + shared (helpers chỉ dùng shared).
    mp.insert(mp.end(), mpVary.begin(), mpVary.end());
    mp.insert(mp.end(), mpShared.begin(), mpShared.end());
    // Sampling calls (texture/texelFetch/textureGrad) trên body. Helpers
    // trong rest được xử lý riêng ở dưới (cần sampler params của từng helper).
    {
        SampTable tab;
        for (auto& s : R.samplers) {
            SampEntry e;
            e.kind = s.isBuffer ? (s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B')
                     : s.isCube   ? 'C'
                     : s.isArray  ? 'A'
                     : s.isShadow ? 'S'
                                  : '2';
            e.elem = s.sampleType;
            tab[s.name] = e;
        }
        std::string err;
        if (!RewriteSamplingCalls(body, tab, err)) return fail(err);
    }
    // gl_FragData[0] → tglmt_fragColor (trước RewriteIdents để không còn `[0]` lẻ).
    if (!R.isVertex && legacyFrag) {
        std::string nb;
        size_t i = 0;
        while (i < body.size()) {
            size_t p = body.find("gl_FragData", i);
            if (p == std::string::npos) { nb += body.substr(i); break; }
            bool l = p > 0 && IsIdentChar(body[p - 1], false);
            size_t b = body.find('[', p + 11);
            size_t e2 = (b == std::string::npos) ? std::string::npos : body.find(']', b);
            if (!l && b != std::string::npos && e2 != std::string::npos &&
                atoi(body.substr(b + 1, e2 - b - 1).c_str()) == 0) {
                nb += body.substr(i, p - i);
                nb += "tglmt_fragColor";
                i = e2 + 1;
                continue;
            }
            nb += body.substr(i, p - i + 11);
            i = p + 11;
        }
        body = nb;
    }
    body = RewriteIdents(RewriteArrayCtors(body), mp);
    // Chỉ attribute ACTIVE (đọc trong main) mới vào TGLMT_VIn.
    // Desktop GL: attribute khai báo nhưng không dùng là inactive — không tốn
    // location, GetAttribLocation → -1. Blaze3D (GlProgram.link, kiểm chứng bằng
    // javap trên client.jar 26.1.2) bind attribute theo tên element của
    // VertexFormat cho MỌI program, kể cả attribute shader đó không dùng
    // (crumbling.vsh không đọc Normal nhưng game vẫn bind "Normal"→0 trong khi
    // Position auto→0). Giữ lại là Metal `attribute index used more than once`
    // → crash khởi động (2026-09-28). Strip theo `_in.NAME` trong body.
    // Fragment inputs là varyings (link kiểm khớp) — KHÔNG strip.
    if (R.isVertex && !ins.empty()) {
        auto usesAttr = [&](const std::string& nm) {
            std::string key = "_in." + nm;
            size_t p = 0;
            while ((p = body.find(key, p)) != std::string::npos) {
                size_t e = p + key.size();
                if (e >= body.size() || !IsIdentChar(body[e], false)) return true;
                p = e;
            }
            return false;
        };
        std::vector<GLSLVar> active;
        for (auto& a : ins)
            if (usesAttr(a.name)) active.push_back(a);
        ins.swap(active);
        R.inputs = ins; // conv mang đi link dùng bản đã strip (struct MSL dưới cũng dùng ins)
    }
    // GLSL implicit int→float (sample_lightmap/terrain) + mat4(mat2) + shadowing:
    // chạy trên MSL (sau RI) vì pattern đã là `float2`/`ubo_`/`float4x4`.
    {
        PromoteVanillaIntMixing(body);
        ExpandMat4FromMat2(body);
        // Tên helper (pre-mp, GLSL) để rename biến local trùng tên trong body.
        // Lấy từ rest (chưa RI) để có tên gốc như `notGamma`.
        std::vector<std::string> hnames;
        for (auto& h : ParseHelperDefs(rest, true)) hnames.push_back(h.name);
        std::sort(hnames.begin(), hnames.end());
        hnames.erase(std::unique(hnames.begin(), hnames.end()), hnames.end());
        RenameShadowedVars(body, hnames);
    }
    // `return;` trần trong main → vertex `z-convert + return _out;`, fragment MRT/single.
    {
        std::string out;
        std::string retV = kZConv + " return _out;";
        std::string retF = isMRT ? "return _out;" : "return tglmt_fragColor;";
        std::string ret = R.isVertex ? retV : retF;
        size_t i = 0;
        while (i < body.size()) {
            if (body.compare(i, 6, "return") == 0 &&
                (i == 0 || !IsIdentChar(body[i - 1], false)) &&
                (i + 6 >= body.size() || !IsIdentChar(body[i + 6], false))) {
                size_t j = i + 6;
                while (j < body.size() && isspace((unsigned char)body[j])) ++j;
                if (j < body.size() && body[j] == ';') {
                    out += ret;
                    i = j + 1;
                    continue;
                }
            }
            out += body[i];
            ++i;
        }
        body = out;
    }

    // POST-MP ENGINE cho helpers: split sampler params + thread tglmt_u/ubo
    // và global-sampler pairs. (mpShared đã tách; body đã RewriteIdents.)
    {
        // helpersOnly = rest trừ main (main đã trích ra body)
        std::string helpersOnly = rest;
        {
            size_t p = helpersOnly.find("void");
            while (p != std::string::npos) {
                size_t q = p + 4;
                while (q < helpersOnly.size() && isspace((unsigned char)helpersOnly[q])) ++q;
                if (helpersOnly.compare(q, 4, "main") == 0) {
                    size_t b = helpersOnly.find('{', q);
                    if (b != std::string::npos) {
                        int dd = 0;
                        size_t m = b;
                        for (; m < helpersOnly.size(); ++m) {
                            if (helpersOnly[m] == '{') ++dd;
                            else if (helpersOnly[m] == '}') {
                                if (--dd == 0) break;
                            }
                        }
                        if (m < helpersOnly.size()) {
                            helpersOnly.erase(p, m - p + 1);
                            break;
                        }
                    }
                }
                p = helpersOnly.find("void", p + 1);
            }
        }
        std::string helpersMSL = RewriteIdents(RewriteArrayCtors(helpersOnly), mpShared);
        // Cùng promotion/expansion trên helpers (sample_lightmap `uv/256.0` và
        // end_portal `float4x4(mat2)` đều nằm trong helper, không phải body).
        // KHÔNG RenameShadowedVars ở đây (định nghĩa hàm luôn kèm `(` nên giữ).
        PromoteVanillaIntMixing(helpersMSL);
        ExpandMat4FromMat2(helpersMSL);
        // global sampler table (cho declOf global pairs)
        SampTable globalSamp;
        for (auto& s : R.samplers) {
            SampEntry e;
            e.kind = s.isBuffer ? (s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B')
                     : s.isCube   ? 'C'
                     : s.isArray  ? 'A'
                     : s.isShadow ? 'S'
                                  : '2';
            e.elem = s.sampleType;
            globalSamp[s.name] = e;
        }
        std::vector<std::string> uboNames;
        for (auto& b : blocks) uboNames.push_back(b.name);
        std::string err;
        if (!ThreadHelpersMSL(helpersMSL, body, helperSamp, globalSamp, uboNames, R.isVertex, err))
            return fail(err);
        rest = helpersMSL;
    }

    // Sinh MSL
    std::ostringstream msl_out;
    msl_out << "#include <metal_stdlib>\nusing namespace metal;\n";
    // Builtins GLSL thiếu trong Metal (chỉ phát khi dùng — kiểm tra body+rest
    // để khỏi rác; `find` word-boundary đơn giản vì tên đã là tglmt_*).
    {
        bool useRad = (body.find("tglmt_radians") != std::string::npos) ||
                      (rest.find("tglmt_radians") != std::string::npos);
        bool useDeg = (body.find("tglmt_degrees") != std::string::npos) ||
                      (rest.find("tglmt_degrees") != std::string::npos);
        bool useM2 = (body.find("tglmt_mat4_from_mat2") != std::string::npos) ||
                     (rest.find("tglmt_mat4_from_mat2") != std::string::npos);
        if (useRad)
            msl_out << "float tglmt_radians(float d) { return d * 0.017453292519943295f; }\n";
        if (useDeg)
            msl_out << "float tglmt_degrees(float r) { return r * 57.29577951308232f; }\n";
        if (useM2)
            msl_out << "float4x4 tglmt_mat4_from_mat2(float2x2 m) { return float4x4(float4(m[0], 0.0, 0.0), float4(m[1], 0.0, 0.0), float4(0.0, 0.0, 1.0, 0.0), float4(0.0, 0.0, 0.0, 1.0)); }\n";
    }
    if (R.isVertex) {
        msl_out << "struct TGLMT_VIn {\n";
        for (auto& a : ins) msl_out << "  " << a.mslType << " " << a.name << " [[attribute(" << a.location << ")]];\n";
        msl_out << "};\nstruct TGLMT_VOut {\n  float4 position [[position]];\n";
        if (R.usesPointSize) msl_out << "  float pointSize [[point_size]];\n";
        for (auto& v : outs) msl_out << "  " << v.mslType << " " << v.name << " [[user(locn" << v.location << ")]];\n";
        msl_out << "};\n";
    } else {
        msl_out << "struct TGLMT_FIn {\n";
        for (auto& v : ins) msl_out << "  " << v.mslType << " " << v.name << " [[user(locn" << v.location << ")]];\n";
        msl_out << "};\n";
    }
    if (!uniforms.empty() || true) {
        msl_out << "struct TGLMTUniforms {\n";
        if (uniforms.empty()) {
            msl_out << "  float _pad;\n";
        } else {
            for (auto& u : uniforms) {
                if (u.isSampler) continue;
                if (u.arraySize > 0)
                    msl_out << "  " << u.mslType << " " << u.name << "[" << u.arraySize << "];\n";
                else
                    msl_out << "  " << u.mslType << " " << u.name << ";\n";
            }
            bool anyNonSampler = false;
            for (auto& u : uniforms)
                if (!u.isSampler) anyNonSampler = true;
            if (!anyNonSampler) msl_out << "  float _pad;\n";
        }
        msl_out << "};\n";
    }
    // UBO structs (read-only): 1 struct/block, bind ở buffer(17+k) theo thứ tự khai báo
    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        auto& b = blocks[bi];
        msl_out << "struct TGLMTUBO_" << b.name << " {\n";
        for (auto& m : b.members) {
            size_t dot = m.name.find('.');
            std::string shortN = (dot == std::string::npos) ? m.name : m.name.substr(dot + 1);
            if (m.arraySize > 0)
                msl_out << "  " << m.mslType << " " << shortN << "[" << m.arraySize << "];\n";
            else
                msl_out << "  " << m.mslType << " " << shortN << ";\n";
        }
        msl_out << "};\n";
    }
    // Khai báo texture/sampler params (index theo thứ tự khai báo sampler).
    // Buffer sampler (CloudFaces): texture2d<elem> KHÔNG sampler (chỉ .read).
    std::string texParams;
    for (size_t k = 0; k < R.samplers.size(); ++k) {
        auto& s = R.samplers[k];
        std::string ks = std::to_string(k);
        if (s.isBuffer) {
            texParams += ", texture2d<" + s.sampleType + "> " + s.name + "_tex [[texture(" + ks + ")]]";
            continue;
        }
        std::string ttype = s.isCube ? "texturecube<float>"
                          : s.isArray ? "texture2d_array<float>" : "texture2d<float>";
        texParams += ", " + ttype + " " + s.name + "_tex [[texture(" + ks +
                     ")]], sampler " + s.name + "_smp [[sampler(" + ks + ")]]";
    }
    // Phần rest: engine post-mp đã xử lý helpers (split/thread) + RI mpShared.
    // Chỉ còn kiểm tra an toàn rồi phát nguyên văn (KHÔNG RewriteIdents lại
    // để khỏi viết đè _tex/_smp/thread params).
    {
        // rest đã mất main từ engine; remover dưới là no-op giữ tương thích.
        std::string noMain = rest;
        size_t p = noMain.find("void");
        while (p != std::string::npos) {
            size_t q = p + 4;
            while (q < noMain.size() && isspace((unsigned char)noMain[q])) ++q;
            if (noMain.compare(q, 4, "main") == 0) {
                size_t b = noMain.find('{', q);
                if (b != std::string::npos) {
                    int dd = 0;
                    size_t m = b;
                    for (; m < noMain.size(); ++m) {
                        if (noMain[m] == '{') ++dd;
                        else if (noMain[m] == '}') {
                            if (--dd == 0) break;
                        }
                    }
                    if (m < noMain.size()) {
                        noMain.erase(p, m - p + 1);
                        break;
                    }
                }
            }
            p = noMain.find("void", p + 1);
        }
        // An toàn: khai báo in/out/uniform/layout còn sót (vd đặt sau hàm) → lỗi rõ.
        {
            bool bad = (noMain.find("layout") != std::string::npos);
            if (!bad) {
                std::istringstream lss(noMain);
                std::string ln;
                while (std::getline(lss, ln)) {
                    std::string t = Trim(ln);
                    if (t.rfind("in ", 0) == 0 || t.rfind("out ", 0) == 0 ||
                        t.rfind("uniform ", 0) == 0) {
                        bad = true;
                        break;
                    }
                }
            }
            if (bad) {
                // Hiện dòng lỗi (cắt 160 ký tự) để log máy chỉ đúng chỗ cần sửa.
                std::string culprit;
                {
                    std::istringstream lss2(noMain);
                    std::string ln2;
                    while (std::getline(lss2, ln2)) {
                        std::string t2 = Trim(ln2);
                        if (t2.rfind("in ", 0) == 0 ||
                            t2.rfind("out ", 0) == 0 || t2.rfind("uniform ", 0) == 0 ||
                            t2.find("layout") != std::string::npos) {
                            culprit = t2.substr(0, 160);
                            break;
                        }
                    }
                    if (culprit.empty() && noMain.find("layout") != std::string::npos)
                        culprit = "(layout trong function body?)";
                }
                return fail("khai báo in/out/uniform/layout đặt sau hàm hoặc sai cú pháp (converter yêu cầu khai báo trước hàm): " + culprit);
            }
        }
        msl_out << noMain;
    }
    if (R.isVertex) {
        msl_out << "vertex TGLMT_VOut TGLMT_vs(TGLMT_VIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        for (size_t bi = 0; bi < blocks.size(); ++bi)
            msl_out << ",\n    constant TGLMTUBO_" << blocks[bi].name << "& ubo_" << blocks[bi].name
                    << " [[buffer(" << (17 + bi) << ")]]";
        if (R.usesVertexID) msl_out << ",\n    uint tglmt_vertexID [[vertex_id]]";
        if (R.usesInstanceID) msl_out << ",\n    uint tglmt_instanceID [[instance_id]]";
        msl_out << texParams << ") {\n  TGLMT_VOut _out = {};\n" << body << "\n" << kZConv << "\n  return _out;\n}\n";
    } else if (isMRT) {
        msl_out << "struct TGLMT_FOut {\n";
        for (auto& o : outs) msl_out << "  float4 mrt" << o.location << " [[color(" << o.location << ")]];\n";
        msl_out << "};\n";
        msl_out << "fragment TGLMT_FOut TGLMT_fs(TGLMT_FIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        for (size_t bi = 0; bi < blocks.size(); ++bi)
            msl_out << ",\n    constant TGLMTUBO_" << blocks[bi].name << "& ubo_" << blocks[bi].name
                    << " [[buffer(" << (17 + bi) << ")]]";
        if (R.usesPointCoord) msl_out << ",\n    float2 tglmt_pointCoord [[point_coord]]";
        if (R.usesFragCoord) msl_out << ",\n    float4 tglmt_fragCoord [[position]]";
        if (R.usesFrontFacing) msl_out << ",\n    bool tglmt_frontFacing [[front_facing]]";
        msl_out << texParams << ") {\n  TGLMT_FOut _out = {};\n" << body
                << "\n  return _out;\n}\n";
    } else {
        msl_out << "fragment float4 TGLMT_fs(TGLMT_FIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        for (size_t bi = 0; bi < blocks.size(); ++bi)
            msl_out << ",\n    constant TGLMTUBO_" << blocks[bi].name << "& ubo_" << blocks[bi].name
                    << " [[buffer(" << (17 + bi) << ")]]";
        if (R.usesPointCoord) msl_out << ",\n    float2 tglmt_pointCoord [[point_coord]]";
        if (R.usesFragCoord) msl_out << ",\n    float4 tglmt_fragCoord [[position]]";
        if (R.usesFrontFacing) msl_out << ",\n    bool tglmt_frontFacing [[front_facing]]";
        msl_out << texParams << ") {\n  float4 tglmt_fragColor = float4(0.0);\n" << body
                << "\n  return tglmt_fragColor;\n}\n";
    }
    R.msl = msl_out.str();
    R.ok = true;
    return R;
}

} // namespace tglmt
