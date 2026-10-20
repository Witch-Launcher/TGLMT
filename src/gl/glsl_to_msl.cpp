// GLSLConverter.cpp — xem GLSLConverter.h về subset được hỗ trợ.
// Mọi thứ ngoài subset → ok=false + log rõ ràng (không đoán mò sinh code sai).
#include "tglmt/GLSLConverter.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <set>
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
// P6g image helpers (file-scope, dùng ở parser + rewrite + emission).
// Kind từ tên kiểu GLSL ('2' 2D, 'A' 2DArray, 'C' Cube, '3' 3D; 0 = 1D/Rect/Buffer/MS).
static char ImageKind(const std::string& t) {
    std::string u = t;
    if (u.rfind("iimage", 0) == 0 || u.rfind("uimage", 0) == 0) u = u.substr(1);
    else if (u.rfind("image", 0) != 0) return 0;
    std::string suf = u.substr(5); // sau "image"
    if (suf == "2D") return '2';
    if (suf == "2DArray") return 'A';
    // Cube: MSL texturecube KHÔNG có .read/.write (probe thật) → 0 = ngoài subset.
    if (suf == "3D") return '3';
    return 0;
}
static std::string ImageElem(const std::string& t) {
    if (t.rfind("iimage", 0) == 0) return "int";
    if (t.rfind("uimage", 0) == 0) return "uint";
    return "float";
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
// Zero literal đúng kiểu MSL (fragment out khởi tạo trước khi body ghi).
static std::string MSLZero(const std::string& t) {
    if (t == "float") return "0.0";
    if (t == "int") return "0";
    if (t == "uint") return "0u";
    if (t.rfind("float", 0) == 0) return t + "(0.0)";
    if (t.rfind("uint", 0) == 0) return t + "(0u)";
    if (t.rfind("int", 0) == 0) return t + "(0)";
    return t + "(0.0)";
}
// Layout `constant` buffer theo quy tắc MSL tự nhiên (không phải std140,
// không phải 16-hết): scalar 4, vec2 8, vec3/vec4 16, mat cột-nào-align-nấy
// (float3x3 = 3 cột float3 stride 16 = 48). Test pixel-proof kiểm chứng thực tế.
static size_t MSLAlign(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    if (m == "float2x2") return 8;
    // Metal float3/int3/uint3 align 16 (dù size 12): scalar sau vec3 ở +12
    // là đọc vào padding (Sodium u_RegionOffset → CurrentTime/RegionID lệch
    // 4B → fade đóng băng từng chunk). Luôn align 16 cho vector 3 phần tử.
    return 16; // vec3/vec4/mat3/mat4 và mọi vector 3-4 phần tử
}
static size_t MSLSize(const std::string& m) {
    if (m == "float" || m == "int" || m == "uint" || m == "bool") return 4;
    if (m == "float2" || m == "int2" || m == "uint2") return 8;
    // vec3 pad 16 để member tiếp theo khớp Metal (upload copy min(size, data)
    // nên 12B từ glUniform3f vẫn đủ, 4B pad giữ 0).
    if (m == "float3" || m == "int3" || m == "uint3") return 16;
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
// Đếm số phần tử initializer của mảng unsized `vec3[](...)` / `{...}`.
// 0 = không nhận diện được (caller fail rõ thay vì bịa size).
static std::vector<std::string> SplitCallArgs(const std::string& s);
static int CountInitElems(const std::string& initRaw) {
    std::string init = Trim(initRaw);
    if (init.empty()) return 0;
    if (init[0] == '{') {
        int d = 0, n = 0;
        bool any = false;
        for (size_t i = 0; i < init.size(); ++i) {
            char c = init[i];
            if (c == '{' || c == '(') { ++d; continue; }
            if (c == '}' || c == ')') { --d; if (d == 0) break; continue; }
            if (d >= 1 && !isspace((unsigned char)c)) any = true;
            if (c == ',' && d == 1) ++n;
        }
        if (!any) return 0;
        return n + 1;
    }
    // `vec3[](...)` / `{...}`-less: args trong ngoặc ngoài cùng
    size_t lb = init.find('[');
    size_t lp = init.find('(', lb == std::string::npos ? 0 : lb);
    if (lp == std::string::npos) return 0;
    int d = 0;
    size_t rp = lp;
    for (size_t i = lp; i < init.size(); ++i) {
        if (init[i] == '(') ++d;
        else if (init[i] == ')') {
            if (--d == 0) { rp = i; break; }
        }
    }
    if (rp <= lp) return 0;
    auto args = SplitCallArgs(init.substr(lp + 1, rp - lp - 1));
    if (args.size() == 1 && Trim(args[0]).empty()) return 0;
    return (int)args.size();
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
                             std::vector<std::pair<std::string, int>>& out,
                             bool allowUnsized = false) {
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
            if (ns.empty()) {
                // mảng unsized: chỉ TCS per-vertex `in/out T x[]` (chuẩn hoá = N sau).
                if (!allowUnsized) return false; // giữ nghiêm chỗ khác (hoisted/const/...)
                arr = -1;
            } else {
                for (char c : ns)
                    if (!isdigit((unsigned char)c)) return false;
                arr = atoi(ns.c_str());
                if (arr <= 0 || arr > 4096) return false;
            }
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
// allowTessArrays: TCS per-vertex `in/out` dạng mảng (unsized `[]` → -1, sized giữ).
// Trả false nếu không khớp.
static bool ParseDecls(const std::string& stmt, bool isVertex,
                       std::string& dir, std::vector<GLSLVar>& vars, bool& isUniform,
                       bool allowTessArrays = false) {
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
    if (!SplitDeclarators(restDecls, ds, allowTessArrays)) return false;
    for (auto& [nm, arr] : ds) {
        // mảng in/out (vd `out vec4 c[2]`) ngoài subset MRT đơn giản → từ chối rõ
        // (MRT dùng nhiều `out` riêng, không phải array).
        // P-Tess T1: TCS per-vertex BẮT BUỘC mảng (unsized `[]`→-1, sized giữ số).
        if (!isUniform && arr != 0 && !allowTessArrays) return false;
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
                    // cho phép size: GLSL hợp lệ cả `vec2[]( ... )` và `vec2[4]( ... )`
                    size_t ks = k;
                    while (ks < src.size() &&
                           (isdigit((unsigned char)src[ks]) || isspace((unsigned char)src[ks]))) ++ks;
                    if (ks < src.size() && src[ks] == ']') {
                        size_t m = skipSp(ks + 1);
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
                        // `type[N] ident` (GLSL 4.x style — endPortalEffect) →
                        // `type ident[N]` (MSL bắt buộc ngoặc SAU tên).
                        std::string sz = Trim(src.substr(k, ks - k));
                        bool szOk = !sz.empty();
                        for (char c : sz)
                            if (!isdigit((unsigned char)c)) szOk = false;
                        if (szOk && m < src.size() &&
                            (isalpha((unsigned char)src[m]) || src[m] == '_')) {
                            size_t e2 = m;
                            while (e2 < src.size() && IsIdentChar(src[e2], e2 == m)) ++e2;
                            out += tn + " " + src.substr(m, e2 - m) + "[" + sz + "]";
                            i = e2;
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
    // 4. `tglmt_u.cameraPositionInt.<sw> -/+ <float>`: GLSL implicit int→float;
    // MSL không implicit intN↔floatN (clouds mainClouds.glsl cameraPositionBIM).
    {
        const std::string base = "tglmt_u.cameraPositionInt";
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            size_t p = text.find(base, i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            bool lb = p > 0 && IsIdentChar(text[p - 1], false);
            size_t e = p + base.size();
            bool rb = e < text.size() && IsIdentChar(text[e], false);
            if (lb || rb) { out += text.substr(i, e - i); i = e; continue; }
            // đã bọc floatN( chưa?
            bool wrapped = false;
            if (p >= 7 && text[p - 1] == '(') {
                std::string pre = text.substr(p - 7, 6);
                if (pre == "float2" || pre == "float3" || pre == "float4") wrapped = true;
            }
            // yêu cầu swizzle `.xy...`
            if (e >= text.size() || text[e] != '.') { out += text.substr(i, e - i); i = e; continue; }
            size_t sw = e + 1;
            while (sw < text.size() && strchr("xyzwrgba", text[sw]) && sw - e <= 4) ++sw;
            int n = (int)(sw - e - 1);
            std::string access = text.substr(p, sw - p);
            // lookahead op `-`/`+`
            size_t q = sw;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            bool op = q < text.size() && (text[q] == '-' || text[q] == '+') &&
                      q + 1 < text.size() && text[q + 1] == ' ';
            if (!op || n < 2 || n > 4 || wrapped) {
                out += text.substr(i, sw - i);
                i = sw;
                continue;
            }
            out += text.substr(i, p - i);
            out += "float" + std::to_string(n) + "(" + access + ")";
            i = sw;
        }
        text.swap(out);
    }
    // 4b. RHS clouds: `modFactorM * (CP.xz / modFactorM)` cho kết quả int2 (GLSL
    // ngầm đổi float) → bọc float2 (đã có float2(...) LHS từ rule 4).
    {
        const std::string rhs = "modFactorM * (tglmt_u.cameraPositionInt.xz / modFactorM)";
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            size_t p = text.find(rhs, i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            bool wrapped = p >= 7 && text.compare(p - 7, 7, "float2(") == 0;
            if (wrapped) { out += text.substr(i, p - i + rhs.size()); i = p + rhs.size(); continue; }
            out += text.substr(i, p - i);
            out += "float2(" + rhs + ")";
            i = p + rhs.size();
        }
        text.swap(out);
    }
}
// ============ Swizzle stpq → xyzw ====================================
// GLSL có 3 bộ swizzle rời nhau (xyzw / rgba / stpq); MSL chỉ nhận xyzw/rgba.
// Heuristic: `.` + 1..4 ký tự [stpq] sau identifier/')'/']' và kết thúc
// bằng non-word → đổi sang xyzw. Phân biệt swizzle vs FIELD ACCESS: identifier
// ngay trước dấu chấm là struct instance (`_in`, `_out`, `tglmt_u`, `ubo_*`,
// `ssbo_*`, biến local/param kiểu struct) → là truy cập field, GIỮ NGUYÊN
// (field tên `p`/`st` phải giữ — regression basic_vs `vec4(p, 1)`).
// Swizzle thật trên member vector (`_in.texCoord.st`, `tglmt_u.camPos.q`) vẫn
// đổi vì identifier trước chấm là tên member, không phải struct instance.
static void CollectStructVarsInText(const std::string& text,
                                    const std::vector<std::string>& structNames,
                                    std::set<std::string>& roots) {
    for (auto& sn : structNames) {
        size_t p = 0;
        while ((p = text.find(sn, p)) != std::string::npos) {
            size_t e = p + sn.size();
            bool l = p > 0 && IsIdentChar(text[p - 1], false);
            bool r = e < text.size() && IsIdentChar(text[e], false);
            if (l || r) { p = e; continue; }
            // khai báo `StructName [&] var` (cho phép whitespace, một `&`).
            size_t q = e;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            if (q < text.size() && text[q] == '&') {
                ++q;
                while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            }
            size_t vs = q;
            while (q < text.size() && IsIdentChar(text[q], false)) ++q;
            if (q > vs) roots.insert(text.substr(vs, q - vs));
            p = e;
        }
    }
}
static void NormalizeSwizzles(std::string& text, const std::set<std::string>& structRoots) {
    static const char* kFrom = "stpq";
    static const char* kTo = "xyzw";
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '.' || i == 0) { out += text[i]; continue; }
        char prev = text[i - 1];
        bool baseOk = prev == '_' || prev == ')' || prev == ']' ||
                      (std::isalpha((unsigned char)prev) != 0);
        if (!baseOk) { out += text[i]; continue; }
        if (std::isalpha((unsigned char)prev) || prev == '_') {
            // identifier ngay trước chấm → struct instance? giữ nguyên field.
            size_t ls = i - 1;
            while (ls > 0 && IsIdentChar(text[ls - 1], false)) --ls;
            if (structRoots.count(text.substr(ls, i - ls))) { out += text[i]; continue; }
        }
        size_t j = i + 1;
        while (j < text.size() && strchr(kFrom, text[j]) && j - i <= 4) ++j;
        size_t len = j - i - 1;
        bool endOk = j >= text.size() ||
                     !(std::isalnum((unsigned char)text[j]) || text[j] == '_');
        if (len == 0 || len > 4 || !endOk) { out += text[i]; continue; }
        out += '.';
        for (size_t k = i + 1; k < j; ++k) out += kTo[strchr(kFrom, text[k]) - kFrom];
        i = j - 1;
    }
        text.swap(out);
}
// ============ Relational builtins (entities fsh) ====
// GLSL `lessThan(a,b)`/`equal(a,b)`/... → MSL operator vector `(a) < (b)`
// (đã quan sát bằng metal thật: undeclared identifier 'lessThan'; MSL có sẵn
// toán tử so sánh vector cho sẵn).
static void ExpandRelationalBuiltins(std::string& text) {
    static const std::pair<const char*, const char*> kOps[] = {
        {"lessThanEqual", "<="}, {"greaterThanEqual", ">="}, {"lessThan", "<"},
        {"greaterThan", ">"},    {"notEqual", "!="},         {"equal", "=="},
        {nullptr, nullptr}};
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        bool hit = false;
        for (const auto* o = kOps; o->first; ++o) {
            std::string nm = o->first;
            if (text.compare(i, nm.size(), nm) != 0) continue;
            if (i > 0 && IsIdentChar(text[i - 1], false)) continue;
            size_t j = i + nm.size();
            if (j < text.size() && IsIdentChar(text[j], false)) continue;
            while (j < text.size() && isspace((unsigned char)text[j])) ++j;
            if (j >= text.size() || text[j] != '(') continue;
            int d = 0;
            size_t e = j;
            for (; e < text.size(); ++e) {
                if (text[e] == '(') ++d;
                else if (text[e] == ')') {
                    if (--d == 0) break;
                }
            }
            if (e >= text.size()) continue;
            std::vector<std::string> args = SplitCallArgs(text.substr(j + 1, e - j - 1));
            if (args.size() != 2) continue;
            out += "(" + Trim(args[0]) + " " + o->second + " " + Trim(args[1]) + ")";
            i = e + 1;
            hit = true;
            break;
        }
        if (!hit) {
            out += text[i];
            ++i;
        }
    }
    text.swap(out);
}
// ============ int-family → float trong khởi tạo (`float2 x = int2(...)`) ====
// GLSL implicit int→float (hand/skytextured/textured: `vec2 tSize = textureSize`
// → converter emit `int2(...)`; terrain: `vec2 atlasSizeM = atlasSize` với
// `uniform ivec2 atlasSize`). MSL cấm — bọc `floatN(<token>)` (identity nếu đã float).
// Chỉ match RHS mở đầu bằng ctor int-family (`int2(`/`uint(`...) hoặc
// `tglmt_u.<field>` có type int-family (intUniforms).
static void CoerceIntInitToFloat(std::string& text, const std::set<std::string>& intUniforms) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        // LHS: `float` / `float2|3|4` + ident + `=`
        std::string lhsType;
        if (text.compare(i, 5, "float") != 0 ||
            (i > 0 && IsIdentChar(text[i - 1], false))) {
            out += text[i];
            ++i;
            continue;
        }
        size_t p = i + 5;
        if (p < text.size() && (text[p] == '2' || text[p] == '3' || text[p] == '4')) {
            lhsType = "float" + std::string(1, text[p]);
            ++p;
        } else {
            lhsType = "float";
        }
        if (p < text.size() && IsIdentChar(text[p], false)) {
            out += text[i];
            ++i;
            continue;
        }
        size_t q = p;
        while (q < text.size() && isspace((unsigned char)text[q])) ++q;
        size_t id = q;
        if (id >= text.size() || !(isalpha((unsigned char)text[id]) || text[id] == '_')) {
            out += text[i];
            ++i;
            continue;
        }
        size_t ide = id;
        while (ide < text.size() && IsIdentChar(text[ide], ide == id)) ++ide;
        size_t r = ide;
        while (r < text.size() && isspace((unsigned char)text[r])) ++r;
        if (r >= text.size() || text[r] != '=' ||
            (r + 1 < text.size() && text[r + 1] == '=')) {
            out += text[i];
            ++i;
            continue;
        }
        size_t v = r + 1;
        while (v < text.size() && isspace((unsigned char)text[v])) ++v;
        // RHS form 1: ctor int-family
        size_t wrapEnd = std::string::npos, wrapDim = 0;
        for (const char* k : {"int", "uint"}) {
            std::string kk = k;
            if (text.compare(v, kk.size(), kk) != 0) continue;
            if (v > 0 && IsIdentChar(text[v - 1], false)) continue;
            size_t u = v + kk.size();
            size_t dim = 1;
            if (u < text.size() && (text[u] == '2' || text[u] == '3' || text[u] == '4')) {
                dim = (size_t)(text[u] - '0');
                ++u;
            }
            if (u < text.size() && IsIdentChar(text[u], false)) continue;
            if (u >= text.size() || text[u] != '(') continue;
            int d = 0;
            size_t e = u;
            for (; e < text.size(); ++e) {
                if (text[e] == '(') ++d;
                else if (text[e] == ')') {
                    if (--d == 0) break;
                }
            }
            if (e >= text.size()) continue;
            wrapEnd = e + 1;
            wrapDim = dim;
            break;
        }
        // RHS form 2: tglmt_u.<field> với field int-family (cùng dim hoặc scalar)
        if (wrapEnd == std::string::npos && text.compare(v, 8, "tglmt_u.") == 0) {
            size_t f = v + 8;
            size_t fe = f;
            while (fe < text.size() && IsIdentChar(text[fe], fe == f)) ++fe;
            std::string field = text.substr(f, fe - f);
            if (intUniforms.count(field)) {
                wrapEnd = fe;
                wrapDim = 0; // scalar/không rõ → bọc theo dim LHS nếu LHS vector
            }
        }
        if (wrapEnd == std::string::npos) {
            out += text[i];
            ++i;
            continue;
        }
        std::string w = lhsType;
        if (wrapDim > 1) {
            // call trả intN → wrap CÙNG dim
            if (lhsType.size() == 5) w = "float" + std::to_string(wrapDim);
            else if (lhsType != "float" + std::to_string(wrapDim)) {
                out += text[i];
                ++i;
                continue; // dim lệch (float2 = int3(...)) — không đoán
            }
            (void)wrapDim;
        }
        out += text.substr(i, v - i);
        out += w + "(" + text.substr(v, wrapEnd - v) + ")";
        i = wrapEnd;
    }
    text.swap(out);
}
// ============ ctor floatN: arg int-family (implicit GLSL) → floatM(arg) ========
// GLSL cho phép implicit int→float TỪNG COMPONENT trong vector ctor:
// `vec4(iris_UV2, 0.0, 1.0)` với `in ivec2 iris_UV2;` — ĐÚNG dạng Iris sinh ra
// (VanillaTransformer.java:101 / VanillaCoreTransformer.java:115 — patch
// gl_MultiTexCoord1 → vec4(iris_UV2, 0.0, 1.0), attr ivec2). MSL KHÔNG:
// float4(int2, float, float) → "no matching constructor" (thật: CR+DH
// TERRAIN_SOLID `float4(iris_UV2, 0.0f, 1.0f)`). Bọc arg bằng `floatM(...)`
// (M = dim của arg) khi arg int-family đã biết từ chính text: param helper /
// local khai báo (`int2 x =`, `int i =`), call `intN(`/`uintN(`/`int(`/`uint(`
// trọn arg, swizzle của base đã biết (`.x` → float, `.xy` → float2). Không suy
// được kiểu → giữ nguyên (tránh bọc oan). Chạy cho cả body lẫn helper — line
// lỗi thật nằm trong helper (GetLightMapCoordinates).
static std::map<std::string, int> CollectIntFamNames(const std::string& text) {
    std::map<std::string, int> m;
    for (size_t i = 0; i + 4 < text.size(); ++i) {
        bool isU = text.compare(i, 4, "uint") == 0;
        if (!isU && text.compare(i, 3, "int") != 0) continue;
        if (i > 0 && IsIdentChar(text[i - 1], false)) continue;
        size_t p = i + (isU ? 4 : 3);
        int dim = 1;
        if (p < text.size() && (text[p] == '2' || text[p] == '3' || text[p] == '4')) {
            dim = text[p] - '0';
            ++p;
        } else if (p < text.size() && IsIdentChar(text[p], false)) {
            continue; // `integer`, `intFoo`…
        }
        // Cần separator space: `int foo` / `int2 foo` — loại call `int(...)`.
        if (p >= text.size() || !isspace((unsigned char)text[p])) continue;
        while (p < text.size() && isspace((unsigned char)text[p])) ++p;
        if (p >= text.size() || !(isalpha((unsigned char)text[p]) || text[p] == '_')) continue;
        size_t e = p;
        while (e < text.size() && IsIdentChar(text[e], e == p)) ++e;
        size_t q = e;
        while (q < text.size() && isspace((unsigned char)text[q])) ++q;
        if (q < text.size() &&
            (text[q] == '=' || text[q] == ';' || text[q] == ',' || text[q] == ')')) {
            std::string name = text.substr(p, e - p);
            if (!m.count(name)) m[name] = dim;
        }
        i = e - 1;
    }
    return m;
}

static void CoerceIntArgsInFloatCtorsImpl(std::string& text,
                                          const std::map<std::string, int>& intDim);

// CollectIntFamNames quét theo TÊN, không theo scope. Tên vừa khai báo int-family
// (vd `int minOf(ivec3 x)` của CR) vừa là float local ở hàm khác (Sodium inject
// `decode_diamond`: `float x`… `return vec2(x, y);`) → wrap nhầm thành
// `float2(float3(x), y)` (game CR sodium:chunk_shader_for_shadow fail). Tên nào
// cũng có khai báo KHÁC int-family (float/floatN/floatNxM/bool/half/struct/…)
// trong cùng text thì bỏ khỏi map (ambiguous → không wrap — an toàn hơn wrap sai).
static void EraseNamesAlsoDeclaredNonInt(const std::string& text,
                                         std::map<std::string, int>& m) {
    if (m.empty()) return;
    static const std::set<std::string> kKeywords = {
        "return", "if", "else", "for", "while", "do", "switch", "case", "break",
        "continue", "discard", "struct", "const", "in", "out", "inout", "void",
        "using", "typedef", "true", "false", "thread", "constant", "device",
        "threadgroup", "kernel", "vertex", "fragment", "static", "inline"};
    for (size_t i = 0; i + 2 < text.size(); ++i) {
        if (!(isalpha((unsigned char)text[i]) || text[i] == '_')) continue;
        if (i > 0 && IsIdentChar(text[i - 1], false)) continue;
        size_t e = i;
        while (e < text.size() && IsIdentChar(text[e], e == i)) ++e;
        std::string ty = text.substr(i, e - i);
        if (ty == "int" || ty == "uint" || ty == "int2" || ty == "int3" ||
            ty == "int4" || ty == "uint2" || ty == "uint3" || ty == "uint4")
            continue; // int-family — không phải conflict
        if (kKeywords.count(ty)) continue;
        // `TYPE NAME` + kết thúc khai báo (loại call `TYPE(`).
        size_t p = e;
        if (p >= text.size() || !isspace((unsigned char)text[p])) continue;
        while (p < text.size() && isspace((unsigned char)text[p])) ++p;
        if (p >= text.size() || !(isalpha((unsigned char)text[p]) || text[p] == '_'))
            continue;
        size_t ne = p;
        while (ne < text.size() && IsIdentChar(text[ne], ne == p)) ++ne;
        size_t q = ne;
        while (q < text.size() && isspace((unsigned char)text[q])) ++q;
        if (q >= text.size() ||
            (text[q] != '=' && text[q] != ';' && text[q] != ',' && text[q] != ')'))
            continue;
        m.erase(text.substr(p, ne - p));
        i = ne - 1;
    }
}

static int IntFamArgDim(const std::string& arg, const std::map<std::string, int>& intDim) {
    std::string t = Trim(arg);
    if (t.empty()) return 0;
    // call ctor int-family TRỌN arg: intN(...)/uintN(...)/int(...)/uint(...)
    {
        int dim = 0;
        size_t u = 0;
        if (t.compare(0, 4, "uint") == 0) {
            dim = 1;
            u = 4;
        } else if (t.compare(0, 3, "int") == 0) {
            dim = 1;
            u = 3;
        }
        if (dim == 1) {
            if (u < t.size() && (t[u] == '2' || t[u] == '3' || t[u] == '4')) {
                dim = t[u] - '0';
                ++u;
            }
            if (u < t.size() && t[u] == '(' && t.back() == ')') {
                int d = 0;
                bool whole = false;
                for (size_t e = u; e < t.size(); ++e) {
                    if (t[e] == '(') ++d;
                    else if (t[e] == ')') {
                        if (--d == 0) {
                            whole = (e + 1 == t.size());
                            break;
                        }
                    }
                }
                if (whole) return dim;
            }
        }
    }
    auto look = [&](const std::string& n) -> int {
        auto it = intDim.find(n);
        return it == intDim.end() ? 0 : it->second;
    };
    // Exact-match TRƯỚC khi tách dấu chấm: khóa có chấm (`tglmt_u.<uniform>`,
    // `ubo_/ssbo_<block>.<member>` — IE_COMPAT `tglmt_u.iris_LightUV`).
    if (int exact = look(t); exact) return exact;
    size_t dot = t.find('.');
    if (dot == std::string::npos) {
        bool bare = isalpha((unsigned char)t[0]) || t[0] == '_';
        for (size_t k = 1; k < t.size() && bare; ++k)
            if (!IsIdentChar(t[k], false)) bare = false;
        return bare ? look(t) : 0;
    }
    std::string base = t.substr(0, dot), sw = t.substr(dot + 1);
    if (base == "_in") { // `_in.NAME` / `_in.NAME.swizzle`
        size_t d2 = sw.find('.');
        std::string nm = d2 == std::string::npos ? sw : sw.substr(0, d2);
        std::string sw2 = d2 == std::string::npos ? "" : sw.substr(d2 + 1);
        int bd = look(nm);
        if (!bd) return 0;
        if (sw2.empty()) return bd;
        for (char c : sw2)
            if (!strchr("xyzw", c)) return 0;
        return (int)sw2.size();
    }
    if (base == "tglmt_u") { // `tglmt_u.NAME` / `tglmt_u.NAME.swizzle` (uniform)
        size_t d2 = sw.find('.');
        std::string nm = d2 == std::string::npos ? sw : sw.substr(0, d2);
        std::string sw2 = d2 == std::string::npos ? "" : sw.substr(d2 + 1);
        int bd = look("tglmt_u." + nm);
        if (!bd) return 0;
        if (sw2.empty()) return bd;
        for (char c : sw2)
            if (!strchr("xyzw", c)) return 0;
        return (int)sw2.size();
    }
    if (base.compare(0, 4, "ubo_") == 0 || base.compare(0, 5, "ssbo_") == 0) {
        // `ubo_<block>.<member>[.swizzle]` — member là segment đầu của `sw`.
        size_t d2 = sw.find('.');
        std::string nm = d2 == std::string::npos ? sw : sw.substr(0, d2);
        std::string sw2 = d2 == std::string::npos ? "" : sw.substr(d2 + 1);
        int bd = look(base + "." + nm);
        if (!bd) return 0;
        if (sw2.empty()) return bd;
        for (char c : sw2)
            if (!strchr("xyzw", c)) return 0;
        return (int)sw2.size();
    }
    int bd = look(base);
    if (!bd) return 0;
    if (sw.empty() || sw.size() > 4) return bd;
    for (char c : sw)
        if (!strchr("xyzw", c)) return 0;
    return (int)sw.size();
}

static void CoerceIntArgsInFloatCtorsImpl(std::string& text,
                                          const std::map<std::string, int>& intDim) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        bool hit = false;
        for (char d = '2'; d <= '4' && !hit; ++d) {
            std::string ctor = std::string("float") + d + "(";
            if (text.compare(i, ctor.size(), ctor) == 0 &&
                (i == 0 || !IsIdentChar(text[i - 1], false)))
                hit = true;
        }
        if (!hit) {
            out += text[i];
            ++i;
            continue;
        }
        size_t lp = i + 6; // "floatN(" → '('
        int dd = 0;
        size_t close = lp;
        for (; close < text.size(); ++close) {
            if (text[close] == '(') ++dd;
            else if (text[close] == ')') {
                if (--dd == 0) break;
            }
        }
        if (close >= text.size()) {
            out += text[i];
            ++i;
            continue;
        }
        std::string core = text.substr(lp + 1, close - lp - 1);
        CoerceIntArgsInFloatCtorsImpl(core, intDim); // ctor lồng nhau trước
        // split top-level args
        std::vector<std::string> args;
        int d2 = 0;
        size_t from = 0;
        for (size_t e = 0; e < core.size(); ++e) {
            if (core[e] == '(') ++d2;
            else if (core[e] == ')') --d2;
            else if (core[e] == ',' && d2 == 0) {
                args.push_back(core.substr(from, e - from));
                from = e + 1;
            }
        }
        args.push_back(core.substr(from));
        std::string joined;
        for (size_t k = 0; k < args.size(); ++k) {
            std::string t = Trim(args[k]);
            int ad = IntFamArgDim(t, intDim);
            if (ad == 1) t = "float(" + t + ")";
            else if (ad >= 2) t = "float" + std::to_string(ad) + "(" + t + ")";
            if (k) joined += ", ";
            joined += t;
        }
        out += text.substr(i, 7);
        out += joined;
        out += ")";
        i = close + 1;
    }
    text.swap(out);
}

// Dim int-family từ mslType: "int"/"uint" → 1, "intN"/"uintN" → N, còn lại 0.
static int IntFamDimOfMslType(const std::string& mslType) {
    int dim = 0;
    if (mslType.compare(0, 4, "uint") == 0) dim = 1;
    else if (mslType.compare(0, 3, "int") == 0) dim = 1;
    if (!dim) return 0;
    if (mslType.size() > 3 && mslType[3] >= '2' && mslType[3] <= '4')
        dim = mslType[3] - '0';
    return dim;
}

// Dim int-family của attribute/varying (body dùng tên TRẦN lúc rule chạy —
// tiền tố `_in.` chỉ thêm sau — nên quét text không thấy, phải lấy từ mslType).
// prefix: khóa lưu map — uniform `tglmt_u.<name>`; member `ubo_/ssbo_<block>.<name>`.
static void AddMslIntFamDims(const std::vector<GLSLVar>& vars,
                             std::map<std::string, int>& m,
                             const std::string& prefix = "") {
    for (auto& v : vars) {
        int dim = IntFamDimOfMslType(v.mslType);
        if (!dim) continue;
        if (!m.count(prefix + v.name)) m[prefix + v.name] = dim;
    }
}

static void CoerceIntArgsInFloatCtors(std::string& text, const std::vector<GLSLVar>& vars,
                                      const std::vector<GLSLVar>& uniforms,
                                      const std::vector<GLSLBlock>& blocks) {
    if (text.find("float2(") == std::string::npos &&
        text.find("float3(") == std::string::npos &&
        text.find("float4(") == std::string::npos)
        return;
    std::map<std::string, int> intDim = CollectIntFamNames(text);
    AddMslIntFamDims(vars, intDim);
    // Loose uniform (Iris IE_COMPAT `uniform ivec2 iris_LightUV;` →
    // `float4(tglmt_u.iris_LightUV, 0.0f, 1.0f)`).
    AddMslIntFamDims(uniforms, intDim, "tglmt_u.");
    // Member UBO/SSBO (text dùng `ubo_<block>.<member>`).
    for (auto& b : blocks)
        AddMslIntFamDims(b.members, intDim, (b.isStorage ? "ssbo_" : "ubo_") + b.name + ".");
    // Scope-blind: tên vừa int-family (param `ivec3 x`) vừa float local hàm khác.
    EraseNamesAlsoDeclaredNonInt(text, intDim);
    if (intDim.empty()) return;
    CoerceIntArgsInFloatCtorsImpl(text, intDim);
}
// ============ `if (v != v)` với v vector → `any(...)` (block vsh) ====
// GLSL driver leniency `if (bvec)`; MSL cần bool tường minh. Fire CHỈ khi:
// hai vế giống hệt nhau VÀ resolve được kiểu vector (bảng vecVars + knownVec).
// `any()` = semantics NaN-check idiom (fireflies fix): NaN ở bất kỳ thành phần.
static void WrapVectorConditions(std::string& text, const std::set<std::string>& knownVec,
                                 const std::set<std::string>& vecHelpers) {
    std::set<std::string> vecs = knownVec;
    // thu tên vector từ khai báo `floatN name` trong chính text (locals/params)
    for (size_t i = 0; i < text.size(); ++i) {
        if (text.compare(i, 5, "float") != 0 || (i > 0 && IsIdentChar(text[i - 1], false)))
            continue;
        if (i + 5 >= text.size() || !isdigit((unsigned char)text[i + 5])) continue;
        size_t p = i + 6;
        if (p < text.size() && !(text[p] == ' ' || text[p] == '\t')) continue;
        size_t q = p;
        while (q < text.size() && isspace((unsigned char)text[q])) ++q;
        if (q >= text.size() || !(isalpha((unsigned char)text[q]) || text[q] == '_')) continue;
        size_t e = q;
        while (e < text.size() && IsIdentChar(text[e], e == q)) ++e;
        vecs.insert(text.substr(q, e - q));
    }
    std::function<bool(std::string)> resolveVec = [&](std::string a) -> bool {
        a = Trim(a);
        if (a.empty()) return false;
        // full match trước (knownVec chứa `_out.X`/`_in.X`) — hoặc bare ident.
        {
            bool ok = true;
            for (size_t k = 0; k < a.size(); ++k)
                if (!IsIdentChar(a[k], k == 0) && a[k] != '.') {
                    ok = false;
                    break;
                }
            if (ok && vecs.count(a)) return true;
        }
        // ident.swizzle / _out.X.swizzle → base = phần trước dấu chấm CUỐI.
        // Swizzle ≥2 ký tự mới là vector (`x.y` scalar → false).
        size_t dot = a.rfind('.');
        if (dot != std::string::npos && dot > 0) {
            std::string base = a.substr(0, dot);
            std::string sw = a.substr(dot + 1);
            bool swOk = sw.size() >= 2 && sw.size() <= 4;
            for (char c : sw)
                if (!strchr("xyzwrgba", c)) swOk = false;
            if (swOk && vecs.count(base)) return true;
        }
        // floatN(...) ctor → vector
        for (char d = '2'; d <= '4'; ++d) {
            std::string ctor = std::string("float") + d + "(";
            if (a.compare(0, ctor.size(), ctor) == 0) return true;
        }
        // builtin giữ nguyên shape theo arg đầu (clamp/max/min/abs/...):
        // `dif == clamp(dif, float3(..), float3(..))` (entities fsh)
        for (const char* bn : {"clamp", "max", "min", "abs", "mix", "sign", "floor", "ceil",
                               "fract", "normalize", "saturate", "pow", "mod", "step",
                               "smoothstep", "sqrt", "exp2", "log2", "rsqrt", "fma"}) {
            std::string b = bn;
            if (a.compare(0, b.size(), b) != 0) continue;
            if (a.size() <= b.size() || a[b.size()] != '(') continue;
            int d2 = 0;
            size_t e = b.size();
            for (; e < a.size(); ++e) {
                if (a[e] == '(') ++d2;
                else if (a[e] == ')') {
                    if (--d2 == 0) break;
                }
            }
            if (e >= a.size()) continue;
            std::vector<std::string> args = SplitCallArgs(a.substr(b.size() + 1, e - b.size() - 1));
            if (args.empty()) continue;
            // arg đầu vector → kết quả vector (clamp/max/min áp dụng chung shape);
            // với smoothstep/step arg đầu là scalar → kiểm tra arg CUỐI thay thế.
            if (b == "smoothstep" || b == "step") {
                if (resolveVec(args.back())) return true;
            } else if (resolveVec(args[0])) {
                return true;
            }
        }
        // helper pack trả vector (`clamp01(x)`) → vector
        {
            size_t lp2 = a.find('(');
            if (lp2 != std::string::npos && lp2 > 0) {
                bool bare = isalpha((unsigned char)a[0]) || a[0] == '_';
                for (size_t k = 1; k < lp2 && bare; ++k)
                    if (!IsIdentChar(a[k], false)) bare = false;
                if (bare && vecHelpers.count(a.substr(0, lp2))) return true;
            }
        }
        return vecs.count(a) > 0;
    };
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        if (text.compare(i, 2, "if") != 0 ||
            (i > 0 && IsIdentChar(text[i - 1], false))) {
            out += text[i];
            ++i;
            continue;
        }
        size_t j = i + 2;
        if (j < text.size() && IsIdentChar(text[j], false)) {
            out += text[i];
            ++i;
            continue;
        }
        while (j < text.size() && isspace((unsigned char)text[j])) ++j;
        if (j >= text.size() || text[j] != '(') {
            out += text[i];
            ++i;
            continue;
        }
        int d = 0;
        size_t e = j;
        for (; e < text.size(); ++e) {
            if (text[e] == '(') ++d;
            else if (text[e] == ')') {
                if (--d == 0) break;
            }
        }
        if (e >= text.size()) {
            out += text[i];
            ++i;
            continue;
        }
        std::string cond = text.substr(j + 1, e - j - 1);
        std::string orig = cond;
        // Mọi `==`/`!=` depth-0: vế-vector → bọc all()/any(). GLSL `vec == vec`
        // trả 1 bool (all-equal) còn MSL trả boolN → phải bọc; `!=` → any()
        // (NaN-check idiom `v != v`). `<`/`>` trên vector không xuất hiện trực
        // tiếp trong if (GLSL invalid). Pool `&&`/`||` được giữ.
        struct WOp {
            size_t pos;
            bool all;
        };
        std::vector<WOp> wops;
        {
            int dd = 0;
            for (size_t k = 0; k + 1 < cond.size(); ++k) {
                char c = cond[k];
                if (c == '(' || c == '[') { ++dd; continue; }
                if (c == ')' || c == ']') { --dd; continue; }
                if (dd != 0) continue;
                if ((c == '=' || c == '!') && cond[k + 1] == '=') {
                    wops.push_back({k, c == '='});
                    ++k;
                }
            }
        }
        for (size_t w = wops.size(); w > 0; --w) {
            size_t opPos = wops[w - 1].pos;
            size_t ls = opPos;
            {
                int d2 = 0;
                while (ls > 0) {
                    char c = cond[ls - 1];
                    if (c == ')' || c == ']') { ++d2; --ls; continue; }
                    if (c == '(' || c == '[') {
                        if (d2 == 0) break;
                        --d2;
                        --ls;
                        continue;
                    }
                    if (d2 != 0) { --ls; continue; }
                    if (c == '&' || c == '|' || c == '?' || c == ':' || c == ',' ||
                        c == '!' || c == ';' || c == '{' || c == '}')
                        break;
                    --ls;
                }
            }
            size_t re2 = opPos + 2;
            {
                int d2 = 0;
                while (re2 < cond.size()) {
                    char c = cond[re2];
                    if (c == '(' || c == '[') { ++d2; ++re2; continue; }
                    if (c == ')' || c == ']') {
                        if (d2 == 0) break;
                        --d2;
                        ++re2;
                        continue;
                    }
                    if (d2 != 0) { ++re2; continue; }
                    if (c == '&' || c == '|' || c == '?' || c == ':' || c == ',' ||
                        c == '!' || c == ';' || c == '{' || c == '}')
                        break;
                    ++re2;
                }
            }
            std::string lhs = cond.substr(ls, opPos - ls);
            std::string rhs = cond.substr(opPos + 2, re2 - opPos - 2);
            if (!resolveVec(lhs) || !resolveVec(rhs)) continue;
            std::string rep = std::string(wops[w - 1].all ? "all(" : "any(") + Trim(lhs) +
                              (wops[w - 1].all ? " == " : " != ") + Trim(rhs) + ")";
            cond = cond.substr(0, ls) + rep + cond.substr(re2);
        }
        if (cond == orig) {
            out += text.substr(i, e + 1 - i);
        } else {
            out += "if (" + cond + ")";
        }
        i = e + 1;
    }
    text.swap(out);
}
// ============ mat4(đơn-arg) expansion (end_portal + Iris inject) ====
// GLSL `mat4(mat2 m)`/`mat4(mat3 m)`/`mat4(float)` — MSL không có các ctor này
// (đã quan sát bằng metal thật: `mat4(1.0)` từ Iris CompositeTransformer).
// Rewrite MỌI `float4x4(X)` đơn-arg (không phẩy depth-0) thành helper overload
// `tglmt_mat4_ctor(X)`; bản 16-float (có phẩy) giữ nguyên.
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
        // text tại p là float4x4(; nếu đã wrap thì không còn float4x4( đơn-arg
        std::string inner = Trim(text.substr(lp + 1, k - lp - 1));
        out += text.substr(i, p - i);
        out += "tglmt_mat4_ctor(" + inner + ")";
        i = k + 1;
    }
    text.swap(out);
}
// ============ mat3(mat4) expansion (Iris helpers ViewToPlayer...) ====
// GLSL `mat3(mat4 m)` hợp lệ (lấy khối 3x3 trên-trái); MSL KHÔNG chuyển được
// float4x4→float3x3. Chỉ rewrite khi arg là tên mat4 ĐÃ BIẾT (uniform/UBO/hoist)
// để không phá `mat3(float)` (đường chéo) hay 3-vec ctor.
static void ExpandMat3FromMat4(std::string& text,
                               const std::vector<std::string>& mat4Names) {
    if (mat4Names.empty()) return;
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        size_t p = text.find("float3x3(", i);
        if (p == std::string::npos) { out += text.substr(i); break; }
        bool l = p > 0 && IsIdentChar(text[p - 1], false);
        if (l) { out += text.substr(i, p - i + 1); i = p + 1; continue; }
        size_t lp = p + 8;
        int d = 0;
        size_t k = lp;
        for (; k < text.size(); ++k) {
            if (text[k] == '(') ++d;
            else if (text[k] == ')') {
                if (--d == 0) break;
            }
        }
        if (k >= text.size()) { out += text.substr(i); break; }
        std::string inner = Trim(text.substr(lp + 1, k - lp - 1));
        bool hit = false;
        for (auto& mn : mat4Names)
            if (inner == mn) { hit = true; break; }
        if (!hit) { out += text.substr(i, k - i + 1); i = k + 1; continue; }
        out += text.substr(i, p - i);
        out += "tglmt_mat3_ctor(" + inner + ")";
        i = k + 1;
    }
    text.swap(out);
}
// ============ struct ctor expansion (Iris inject `iris_FogParameters(...)`) ====
// GLSL có constructor ngầm `S(args)` cho struct; MSL (C++) KHÔNG (probe thật:
// "no matching constructor"). Rewrite `S(args)` → `S{args}` (aggregate init).
static std::vector<std::string> CollectStructNames(const std::string& text) {
    std::vector<std::string> out;
    size_t p = 0;
    while ((p = text.find("struct", p)) != std::string::npos) {
        bool l = p > 0 && IsIdentChar(text[p - 1], false);
        size_t e = p + 6;
        bool r = e < text.size() && IsIdentChar(text[e], false);
        if (l || r) { p = e; continue; }
        size_t s = e;
        while (s < text.size() && isspace((unsigned char)text[s])) ++s;
        size_t t = s;
        while (t < text.size() && IsIdentChar(text[t], t == s)) ++t;
        if (t > s) out.push_back(text.substr(s, t - s));
        p = t > p ? t : p + 1;
    }
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}
static void ExpandStructCtors(std::string& text, const std::vector<std::string>& structNames) {
    for (auto& sn : structNames) {
        if (sn.empty()) continue;
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            size_t p = text.find(sn, i);
            if (p == std::string::npos) { out += text.substr(i); break; }
            bool l = p > 0 && IsIdentChar(text[p - 1], false);
            size_t e = p + sn.size();
            bool r = e < text.size() && IsIdentChar(text[e], false);
            if (l || r) { out += text.substr(i, e - i); i = e; continue; }
            size_t q = e;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            if (q >= text.size() || text[q] != '(') { out += text.substr(i, e - i); i = e; continue; }
            size_t b = p;
            while (b > 0 && isspace((unsigned char)text[b - 1])) --b;
            if (b > 0 && text[b - 1] == '.') { out += text.substr(i, e - i); i = e; continue; }
            int d = 0;
            size_t k = q;
            for (; k < text.size(); ++k) {
                if (text[k] == '(') ++d;
                else if (text[k] == ')') {
                    if (--d == 0) break;
                }
            }
            if (k >= text.size()) { out += text.substr(i); break; }
            out += text.substr(i, p - i);
            out += sn + "{" + text.substr(q + 1, k - q - 1) + "}";
            i = k + 1;
        }
        text.swap(out);
    }
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
// Tên helper trả về vector (vec2/3/4, ivec*, uvec*, floatN) — cho resolveVec
// của WrapVectorConditions (`if (v.xyz == clamp01(x))` — clamp01 là helper pack
// nên không nằm trong builtin list). Overload cùng tên khác shape → bỏ (mơ hồ).
static std::set<std::string> HelperVecReturns(const std::string& restGLSL) {
    std::map<std::string, bool> rets; // name → isVec
    for (auto& h : ParseHelperDefs(restGLSL, true)) {
        size_t ns = restGLSL.find(h.name, h.defStart);
        if (ns == std::string::npos || ns == h.defStart) continue;
        std::string rt = Trim(restGLSL.substr(h.defStart, ns - h.defStart));
        if (rt.find(' ') != std::string::npos) continue; // qualifier lạ → bỏ
        bool isVec = (rt == "vec2" || rt == "vec3" || rt == "vec4" ||
                      rt == "float2" || rt == "float3" || rt == "float4" ||
                      rt == "ivec2" || rt == "ivec3" || rt == "ivec4" ||
                      rt == "uvec2" || rt == "uvec3" || rt == "uvec4");
        auto it = rets.find(h.name);
        if (it == rets.end()) rets[h.name] = isVec;
        else if (it->second != isVec) rets.erase(it); // overload khác shape → mơ hồ
    }
    std::set<std::string> out;
    for (auto& [n, v] : rets)
        if (v) out.insert(n);
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
// Helper param trùng tên uniform/UBO/SSBO (key trong mpShared): RewriteIdents sẽ
// phá signature (`float far` → `float tglmt_u.far` — invalid MSL). Đổi tên param
// thầm uniform thành `name_param` trong TOÀN BỘ extent helper (GLSL: param shadow
// cả thân hàm; mọi use trong thân là param). Bỏ qua member access `x.name`.
static void RenameParamShadowUniforms(std::string& text,
                                      const std::vector<std::pair<std::string, std::string>>& mp) {
    auto isKey = [&](const std::string& n) {
        for (auto& kv : mp)
            if (kv.first == n) return true;
        return false;
    };
    std::vector<HelperDef> defs = ParseHelperDefs(text, false);
    for (size_t idx = defs.size(); idx-- > 0;) { // giảm dần: offset phía trước không đổi
        HelperDef& h = defs[idx];
        if (!h.hasBody) continue;
        std::vector<std::string> hits;
        for (auto& p : SplitCallArgs(h.params)) {
            std::string nm = ParamBaseName(p);
            if (!nm.empty() && isKey(nm) &&
                std::find(hits.begin(), hits.end(), nm) == hits.end())
                hits.push_back(nm);
        }
        if (hits.empty()) continue;
        size_t a = h.defStart, b = h.bodyEnd;
        std::string seg = text.substr(a, b - a + 1);
        for (auto& nm : hits) {
            std::string out;
            size_t i = 0;
            while (i < seg.size()) {
                size_t p = seg.find(nm, i);
                if (p == std::string::npos) { out += seg.substr(i); break; }
                bool l = p > 0 && IsIdentChar(seg[p - 1], false);
                size_t e = p + nm.size();
                bool r = e < seg.size() && IsIdentChar(seg[e], false);
                if (l || r) { out += seg.substr(i, e - i); i = e; continue; }
                size_t q = p;
                while (q > 0 && isspace((unsigned char)seg[q - 1])) --q;
                if (q > 0 && seg[q - 1] == '.') { // member access — giữ
                    out += seg.substr(i, e - i);
                    i = e;
                    continue;
                }
                out += seg.substr(i, p - i);
                out += nm + "_param";
                i = e;
            }
            seg.swap(out);
        }
        text.replace(a, b - a + 1, seg);
    }
}
// ===== Local decl trùng tên uniform/UBO (GLSL shadowing) → name_lcl =====
// GLSL: `int blockEntityId = mat;` trong thân hàm shadow uniform cùng tên; mọi
// use sau decl (trong scope block đó) là LOCAL. RewriteIdents sẽ phá decl thành
// `int tglmt_u.blockEntityId = ...` (invalid MSL — terrain.fsh thật).
// Thu: (1) map `{`→`}`; (2) tìm decl `TYPE name` (name ∈ keys, depth ≥ 1);
// (3) mọi occurrence trong [decl.pos, decl.blockEnd) thuộc local innermost →
// đổi tên name+suffix. Áp replacement từ phải sang trái (offset gốc ổn định).
static void RenameLocalShadowUniforms(
    std::string& text, const std::vector<std::pair<std::string, std::string>>& mp) {
    auto isKey = [&](const std::string& n) {
        for (auto& kv : mp)
            if (kv.first == n) return true;
        return false;
    };
    // map brace start → end
    std::unordered_map<size_t, size_t> braceEnd;
    {
        std::vector<size_t> st;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '{')
                st.push_back(i);
            else if (text[i] == '}') {
                if (st.empty()) continue;
                braceEnd[st.back()] = i;
                st.pop_back();
            }
        }
    }
    auto enclosingBraceStart = [&](size_t pos) -> size_t {
        int depth = 0;
        for (size_t k = pos; k-- > 0;) {
            if (text[k] == '}') ++depth;
            else if (text[k] == '{') {
                if (depth == 0) return k;
                --depth;
            }
        }
        return std::string::npos;
    };
    // struct body? (trước `{` là tên struct + keyword `struct`) — field trùng tên
    // uniform KHÔNG được đổi (access `s.field` ngoài block sẽ lệch).
    auto isStructBrace = [&](size_t bs) {
        size_t w = bs;
        while (w > 0 && isspace((unsigned char)text[w - 1])) --w;
        size_t we = w;
        while (we > 0 && IsIdentChar(text[we - 1], false)) --we;
        size_t v = we;
        while (v > 0 && isspace((unsigned char)text[v - 1])) --v;
        size_t ve = v;
        while (ve > 0 && IsIdentChar(text[ve - 1], false)) --ve;
        return text.substr(ve, v - ve) == "struct";
    };
    static const char* kTypes[] = {"float", "float2", "float3", "float4", "vec2", "vec3",
                                   "vec4", "int", "ivec2", "ivec3", "ivec4", "uint", "uvec2",
                                   "uvec3", "uvec4", "bool", "bvec2", "bvec3", "bvec4",
                                   nullptr};
    struct Decl {
        size_t pos;
        std::string name;
        size_t end;
    };
    std::vector<Decl> decls;
    for (size_t i = 0; i < text.size(); ++i) {
        if (!(isalpha((unsigned char)text[i]) || text[i] == '_')) continue;
        size_t e = i;
        while (e < text.size() && IsIdentChar(text[e], false)) ++e;
        std::string nm = text.substr(i, e - i);
        if (!isKey(nm)) { i = e - 1; continue; }
        // prev word phải là type keyword (bỏ qua space/*/&)
        size_t q = i;
        while (q > 0 && (isspace((unsigned char)text[q - 1]) || text[q - 1] == '*' ||
                         text[q - 1] == '&'))
            --q;
        size_t we = q;
        while (we > 0 && IsIdentChar(text[we - 1], false)) --we;
        std::string prev = text.substr(we, q - we);
        bool isType = false;
        for (const char** t = kTypes; *t && !isType; ++t)
            if (prev == *t) isType = true;
        if (!isType) { i = e - 1; continue; }
        // char sau name ∈ `= ; , [ )` (decl, không phải call arg)
        size_t s = e;
        while (s < text.size() && isspace((unsigned char)text[s])) ++s;
        if (s >= text.size() || (text[s] != '=' && text[s] != ';' && text[s] != ',' &&
                                 text[s] != '[' && text[s] != ')')) {
            i = e - 1;
            continue;
        }
        size_t bs = enclosingBraceStart(i);
        if (bs == std::string::npos || isStructBrace(bs)) {
            i = e - 1;
            continue;
        }
        auto itEnd = braceEnd.find(bs);
        decls.push_back({i, nm, itEnd == braceEnd.end() ? text.size() : itEnd->second});
        i = e - 1;
    }
    if (decls.empty()) return;
    struct Rep {
        size_t pos, len;
        std::string name;
    };
    std::vector<Rep> reps;
    std::set<std::string> names;
    for (auto& d : decls) names.insert(d.name);
    for (auto& nm : names) {
        size_t p = 0;
        while ((p = text.find(nm, p)) != std::string::npos) {
            size_t e = p + nm.size();
            bool l = p > 0 && IsIdentChar(text[p - 1], false);
            bool r = e < text.size() && IsIdentChar(text[e], false);
            if (l || r) { p = e; continue; }
            size_t q = p;
            while (q > 0 && isspace((unsigned char)text[q - 1])) --q;
            if (q > 0 && text[q - 1] == '.') { p = e; continue; }
            // innermost decl chứa p
            const Decl* best = nullptr;
            for (auto& d : decls)
                if (d.name == nm && p >= d.pos && p < d.end)
                    if (!best || d.pos > best->pos) best = &d;
            if (best) reps.push_back({p, nm.size(), nm});
            p = e;
        }
    }
    std::sort(reps.begin(), reps.end(),
              [](const Rep& a, const Rep& b) { return a.pos > b.pos; });
    for (auto& r : reps)
        text = text.substr(0, r.pos) + r.name + "_lcl" + text.substr(r.pos + r.len);
}
// GLSL param qualifier → MSL (MSL không có inout/out param):
//  - `inout T x` / `out T x` → `thread T& x` (call-site lvalue bind tự nhiên).
//  - `T x[N]` (in hoặc inout) → `thread T (&x)[N]` (MSL không truyền mảng by value).
//  - `in T x` → `T x`; `const in T x` → `const T x`.
// Không qualifier (đã là MSL / hoist-threading `thread T (&g)[N]`) → giữ nguyên.
static std::string RewriteOneParamMSL(const std::string& p) {
    std::string t = Trim(p);
    bool byRef = false, isConst = false, hadQual = false;
    for (;;) {
        if (t.rfind("const ", 0) == 0 || t == "const") { isConst = true; hadQual = true; t = Trim(t.substr(5)); continue; }
        if (t.rfind("inout ", 0) == 0) { byRef = true; hadQual = true; t = Trim(t.substr(6)); continue; }
        if (t.rfind("out ", 0) == 0) { byRef = true; hadQual = true; t = Trim(t.substr(4)); continue; }
        if (t.rfind("in ", 0) == 0) { hadQual = true; t = Trim(t.substr(3)); continue; }
        break;
    }
    if (!hadQual) return p;
    size_t lb = t.find('[');
    std::string core = Trim(lb == std::string::npos ? t : t.substr(0, lb));
    std::string arr = (lb == std::string::npos) ? std::string() : Trim(t.substr(lb));
    size_t sp = core.find_last_of(" \t");
    if (sp == std::string::npos) return p; // không đủ type+name — để metal báo
    std::string ty = Trim(core.substr(0, sp));
    std::string nm = Trim(core.substr(sp + 1));
    if (!arr.empty() && (arr == "[]" || arr.rfind("[&", 0) == 0)) return p; // unsized/đã ref
    if (!arr.empty()) {
        if (nm.rfind("(&", 0) == 0) return "thread " + ty + " " + nm + arr;
        return (isConst ? "const " : "") + std::string("thread ") + ty + " (&" + nm + ")" + arr;
    }
    if (byRef) return isConst ? ("const thread " + ty + "& " + nm) : ("thread " + ty + "& " + nm);
    return (isConst ? "const " : "") + ty + " " + nm;
}
// Rewrite param list của MỌI function definition ở depth 0: `ret name(inout T x,...){`.
// Chạy trên helpersMSL (post mp-rewrite) — trước ThreadHelpersMSL (engine chỉ đọc
// tên param cuối nên `thread T& x`/`thread T (&x)[N]` vẫn parse đúng).
static void RewriteParamQualifiers(std::string& text) {
    std::vector<std::pair<size_t, size_t>> spans; // (lp+1, rp) của param list
    int depth = 0;
    size_t i = 0;
    while (i < text.size()) {
        char ch = text[i];
        if (ch == '"') {
            size_t j = text.find('"', i + 1);
            i = (j == std::string::npos) ? text.size() : j + 1;
            continue;
        }
        if (ch == '{') { ++depth; ++i; continue; }
        if (ch == '}') { --depth; ++i; continue; }
        if (ch != '(' || depth != 0) { ++i; continue; }
        size_t ns = i;
        while (ns > 0 && isspace((unsigned char)text[ns - 1])) --ns;
        size_t ne = ns;
        while (ns > 0 && IsIdentChar(text[ns - 1], false)) --ns;
        std::string fn = text.substr(ns, ne - ns);
        static const char* kKw[] = {"if", "for", "while", "switch", "return", "main", nullptr};
        bool skip = fn.empty();
        for (const char** kw = kKw; *kw; ++kw)
            if (fn == *kw) skip = true;
        if (skip) { ++i; continue; }
        int d = 0;
        size_t e = i;
        for (; e < text.size(); ++e) {
            if (text[e] == '(') ++d;
            else if (text[e] == ')') { if (--d == 0) break; }
        }
        if (e >= text.size()) break;
        size_t b = e + 1;
        while (b < text.size() && isspace((unsigned char)text[b])) ++b;
        if (b < text.size() && text[b] == '{') {
            std::string plist = text.substr(i + 1, e - i - 1);
            auto parts = SplitCallArgs(plist);
            if (!(parts.size() == 1 && Trim(parts[0]).empty())) {
                bool changed = false;
                for (auto& pp : parts)
                    if (RewriteOneParamMSL(Trim(pp)) != Trim(pp)) changed = true;
                if (changed) spans.push_back({i + 1, e});
            }
        }
        i = e + 1;
    }
    for (size_t k = spans.size(); k-- > 0;) {
        auto& [a, b] = spans[k];
        std::string plist = text.substr(a, b - a);
        auto parts = SplitCallArgs(plist);
        std::string np;
        for (size_t q = 0; q < parts.size(); ++q) {
            if (q) np += ", ";
            np += RewriteOneParamMSL(Trim(parts[q]));
        }
        text = text.substr(0, a) + np + text.substr(b);
    }
}
// ============ inout ref nhận swizzle (`DoWave(playerPos.xyz, ...)`) ====
// MSL `thread T&` không bind được swizzle (non-const ref to vector element —
// terrain vsh/fsh). GLSL inout + swizzle = copy-in/copy-out → mở call-statement
// thành block + temp: `{ T t = EXPR; f(t,...); EXPR = t; }` (EXPR là lvalue).
// Chỉ fire khi call đứng riêng như STATEMENT (prev `;`/`{`/`}`), ngược lại giữ
// nguyên (metal báo trung thực).
static void BindSwizzleInoutCalls(std::string& text, const std::string& helpersMSL) {
    // def → các arg vị trí ref + type (từ header `thread T [(&n)[N]|& n]`)
    std::map<std::string, std::map<size_t, std::string>> refInfo;
    for (auto& d : ParseHelperDefs(helpersMSL, true)) {
        auto parts = SplitCallArgs(d.params);
        if (parts.size() == 1 && Trim(parts[0]).empty()) parts.clear();
        for (size_t i = 0; i < parts.size(); ++i) {
            std::string pt = Trim(parts[i]);
            size_t tw = pt.find("thread ");
            if (tw == std::string::npos || pt.find('&') == std::string::npos) continue;
            std::string rest = Trim(pt.substr(tw + 7));
            size_t amp = rest.find('&');
            if (amp == std::string::npos) continue;
            std::string ty = Trim(rest.substr(0, amp));
            if (!ty.empty() && ty.back() == '(') ty = Trim(ty.substr(0, ty.size() - 1));
            if (ty.empty()) continue;
            refInfo[d.name][i] = ty;
        }
    }
    if (refInfo.empty()) return;
    auto swizzleBase = [](const std::string& a, std::string& base, std::string& sw) -> bool {
        std::string t = Trim(a);
        size_t dot = t.rfind('.');
        if (dot == std::string::npos || dot == 0) return false;
        base = t.substr(0, dot);
        sw = t.substr(dot + 1);
        if (sw.empty() || sw.size() > 4) return false;
        for (char c : sw)
            if (!strchr("xyzwrgba", c)) return false;
        // base là lvalue path: ident(.ident)* — `_out.position.xyz` hợp lệ.
        if (!(isalpha((unsigned char)base[0]) || base[0] == '_')) return false;
        for (size_t k = 0; k < base.size(); ++k)
            if (!IsIdentChar(base[k], k == 0) && base[k] != '.') return false;
        if (base.find("..") != std::string::npos || base.back() == '.') return false;
        return true;
    };
    std::string out;
    size_t i = 0;
    int tmpN = 0;
    while (i < text.size()) {
        if (!(isalpha((unsigned char)text[i]) || text[i] == '_')) {
            out += text[i];
            ++i;
            continue;
        }
        size_t e = i;
        while (e < text.size() && IsIdentChar(text[e], false)) ++e;
        std::string nm = text.substr(i, e - i);
        auto rit = refInfo.find(nm);
        size_t q = e;
        while (q < text.size() && isspace((unsigned char)text[q])) ++q;
        bool isCall = (q < text.size() && text[q] == '(');
        if (!isCall) {
            out += text.substr(i, e - i);
            i = e;
            continue;
        }
        // call PHẢI là statement: prev (output) là `;`/`{`/`}` — hoặc `)`/`else`
        // (if/for/while không braces: `if (c)\n f(playerPos.xyz);` — block vẫn hợp lệ).
        size_t pb = out.size();
        while (pb > 0 && isspace((unsigned char)out[pb - 1])) --pb;
        bool stmtOk = (pb == 0) || out[pb - 1] == ';' || out[pb - 1] == '{' ||
                      out[pb - 1] == '}' || out[pb - 1] == ')';
        if (!stmtOk && pb >= 4 && out.compare(pb - 4, 4, "else") == 0) {
            bool lw = pb == 4 || !IsIdentChar(out[pb - 5], false);
            stmtOk = lw;
        }
        if (!stmtOk || rit == refInfo.end()) {
            out += text.substr(i, e - i);
            i = e;
            continue;
        }
        int d = 0;
        size_t c = q;
        for (; c < text.size(); ++c) {
            if (text[c] == '(') ++d;
            else if (text[c] == ')') {
                if (--d == 0) break;
            }
        }
        if (c >= text.size()) {
            out += text.substr(i);
            break;
        }
        std::vector<std::string> args = SplitCallArgs(text.substr(q + 1, c - q - 1));
        if (args.size() == 1 && Trim(args[0]).empty()) args.clear();
        bool any = false;
        std::vector<std::pair<size_t, std::string>> tmps; // (argIdx, base)
        std::vector<std::pair<size_t, std::string>> types;
        for (auto& [idx, ty] : rit->second) {
            if (idx >= args.size()) continue;
            std::string base, sw;
            if (!swizzleBase(args[idx], base, sw)) continue;
            tmps.emplace_back(idx, Trim(args[idx]));
            types.emplace_back(idx, ty);
            any = true;
        }
        if (!any) {
            out += text.substr(i, e - i);
            i = e;
            continue;
        }
        // emit block
        std::string blk = "{ ";
        std::vector<std::string> tmpNames;
        for (size_t k = 0; k < tmps.size(); ++k) {
            std::string tn = "tglmt_io_tmp_" + std::to_string(tmpN++);
            tmpNames.push_back(tn);
            blk += types[k].second + " " + tn + " = " + tmps[k].second + "; ";
            args[tmps[k].first] = tn;
        }
        blk += nm + "(";
        for (size_t k = 0; k < args.size(); ++k) {
            if (k) blk += ", ";
            blk += Trim(args[k]);
        }
        blk += "); ";
        for (size_t k = 0; k < tmps.size(); ++k) blk += tmps[k].second + " = " + tmpNames[k] + "; ";
        blk += "}";
        out += blk;
        i = c + 1;
        // bỏ `;` ngay sau (đã có trong block) — nếu có
        size_t s2 = i;
        while (s2 < text.size() && isspace((unsigned char)text[s2])) ++s2;
        if (s2 < text.size() && text[s2] == ';') i = s2 + 1;
    }
    text.swap(out);
}
// Loại sampler theo từ kiểu GLSL: '2' 2D, 'S' shadow, 'C' cube, 'A' array,
// 'B' buffer-float, 'I' buffer-int, 'U' buffer-uint, 0 = không phải sampler.
// P3: '1' sampler1D (approx texture2d + eps y), '3' sampler3D (texture3d<float>),
// 'i' isampler2D, 'u' usampler2D, 'x' isampler3D, 'y' usampler3D.
static char SamplerKindOf(const std::string& glslType) {
    if (glslType == "sampler2D") return '2';
    if (glslType == "sampler2DShadow") return 'S';
    if (glslType == "samplerCube") return 'C';
    if (glslType == "sampler2DArray") return 'A';
    if (glslType == "samplerBuffer") return 'B';
    if (glslType == "isamplerBuffer") return 'I';
    if (glslType == "usamplerBuffer") return 'U';
    if (glslType == "sampler1D") return '1';
    if (glslType == "sampler3D") return '3';
    if (glslType == "isampler2D") return 'i';
    if (glslType == "usampler2D") return 'u';
    if (glslType == "isampler3D") return 'x';
    if (glslType == "usampler3D") return 'y';
    return 0;
}
// Kind sampler → MSL texture type (declaration của _tex param).
static std::string SamplerTexType(char kind, const std::string& elem = "float") {
    switch (kind) {
        case 'C': return "texturecube<float>";
        case 'A': return "texture2d_array<float>";
        case 'I': return "texture2d<int>";
        case 'U': return "texture2d<uint>";
        case 'B': return "texture2d<" + elem + ">";
        case '1': return "texture2d<" + elem + ">";       // 1D ≈ 2D w×1
        case '3': return "texture3d<" + elem + ">";       // 3D
        case 'i': return "texture2d<int>";
        case 'u': return "texture2d<uint>";
        case 'x': return "texture3d<int>";
        case 'y': return "texture3d<uint>";
        default: return "texture2d<float>";
    }
}
// P3: kind sampler → elem (float/int/uint) cho entry-level _tex param.
static std::string SamplerElemOf(char kind) {
    switch (kind) {
        case 'I': case 'i': case 'x': return "int";
        case 'U': case 'u': case 'y': return "uint";
        default: return "float";
    }
}
// Bảng sampler cho rewrite sampling calls: tên → (kind, elem).
struct SampEntry {
    char kind = '2'; // 1/2/S/3/C/A/B/I/U/i/u/x/y (xem SamplerKindOf)
    std::string elem = "float";
};
using SampTable = std::map<std::string, SampEntry>;
// P3: entry cho 1 sampler var đã parse (dùng chung các chỗ build SampTable).
static SampEntry EntryOfSampler(const GLSLVar& s) {
    SampEntry e;
    if (s.isBuffer)       e.kind = (s.sampleType == "int") ? 'I' : (s.sampleType == "uint") ? 'U' : 'B';
    else if (s.isCube)    e.kind = 'C';
    else if (s.isArray)   e.kind = 'A';
    else if (s.isShadow)  e.kind = 'S';
    else if (s.is1D)      e.kind = '1';
    else if (s.is3D)      e.kind = (s.sampleType == "int") ? 'x' : (s.sampleType == "uint") ? 'y' : '3';
    else if (s.sampleType == "int")  e.kind = 'i';
    else if (s.sampleType == "uint") e.kind = 'u';
    else                  e.kind = '2';
    e.elem = s.sampleType;
    return e;
}
// P6g: bảng image (tên → kind '2'/'A'/'C'/'3') cho rewrite imageLoad/Store/Size.
using ImgTable = std::map<std::string, char>;
// P-Tess T1: rewrite TCS body → MSL kernel body.
// Thread = (invocation, patch): tglmt_tcs_tid=[[thread_position_in_grid]].x/y,
// tglmt_patchID (= .y) do entry emit. N = layout(vertices).
// Quy ước buffer (T4 bind theo): in/out per-vertex `tcs_in|out_<nm>[patchID*N+e]`,
// patch `tcs_patch_<nm>[patchID]` (array patch: [patchID*M+e]), gl_in position/
// PointSize `tcs_in_gl_Position|PointSize[...]`, TessLevel → tcs_factors[pid].edge|inside.
struct TessVar {
    std::string name;
    bool isInput = false;  // ins (false = outs)
    bool isPatch = false;
    int arraySize = 0;     // per-vertex: N (đã chuẩn hoá); patch array: M; patch scalar: 0
};
static bool RewriteTessControl(std::string& io, int N, const std::vector<TessVar>& vars,
                               bool hasGlIn, std::string& err) {
    std::string& body = io;
    auto isHead = [&](size_t p, const std::string& w) -> bool {
        if (body.compare(p, w.size(), w) != 0) return false;
        if (p > 0 && IsIdentChar(body[p - 1], false)) return false;
        size_t e = p + w.size();
        if (e < body.size() && IsIdentChar(body[e], false)) return false;
        return true;
    };
    auto isHeadC = [&](size_t p, const char* w) -> bool {
        return isHead(p, std::string(w));
    };
    // 1. gl_PatchVerticesIn → N literal.
    {
        std::string out;
        size_t i = 0;
        const std::string k = "gl_PatchVerticesIn";
        while (i < body.size()) {
            size_t p = body.find(k, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            if (!isHead(p, k)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
            out += body.substr(i, p - i);
            out += std::to_string(N);
            i = p + k.size();
        }
        body = out;
    }
    // 2. gl_InvocationID → tid.x ; gl_PrimitiveID → tid.y (patch ID).
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            bool matched = false;
            for (auto [g, m] : {std::pair<const char*, const char*>{"gl_InvocationID", "tglmt_tcs_tid.x"},
                                {"gl_PrimitiveID", "tglmt_tcs_tid.y"}}) {
                if (isHeadC(i, g)) { out += m; i += strlen(g); matched = true; break; }
            }
            if (!matched) out += body[i++];
        }
        body = out;
    }
    // 3. barrier() → threadgroup_barrier (đồng bộ invocation trong patch;
    // T4 dispatch threadgroup=(N,1,1)).
    {
        std::string out;
        size_t i = 0;
        const std::string k = "barrier";
        while (i < body.size()) {
            size_t p = body.find(k, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            if (!isHead(p, k)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
            size_t lp = p + k.size();
            while (lp < body.size() && isspace((unsigned char)body[lp])) ++lp;
            if (lp + 1 >= body.size() || body[lp] != '(' || body[lp + 1] != ')')
                { err = "barrier() TCS không tham số"; return false; }
            out += body.substr(i, p - i);
            out += "threadgroup_barrier(mem_flags::mem_device);";
            i = lp + 2;
        }
        body = out;
    }
    // 4. gl_TessLevelOuter[k]/Inner[k] → tcs_factors[pid].edge|inside[k].
    // Chỉ số literal (pattern thực tế); Outer 0..2, Inner 0 (triangles).
    // [3]/[1] là quad-only → fail (T1 triangles). Non-literal/bare → fail rõ.
    auto rewriteTessLevel = [&](const std::string& nm, bool isOuter) -> bool {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            size_t p = body.find(nm, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            if (!isHead(p, nm)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
            size_t q = p + nm.size();
            while (q < body.size() && isspace((unsigned char)body[q])) ++q;
            if (q >= body.size() || body[q] != '[') {
                err = nm + " phải có chỉ số [...] literal";
                return false;
            }
            size_t rb = body.find(']', q);
            if (rb == std::string::npos) { err = nm + " thiếu ]"; return false; }
            std::string idxs = Trim(body.substr(q + 1, rb - q - 1));
            bool allDigit = !idxs.empty();
            for (char c : idxs) if (!isdigit((unsigned char)c)) allDigit = false;
            if (!allDigit) { err = nm + " chỉ số phải literal (T1): " + idxs; return false; }
            int k = atoi(idxs.c_str());
            out += body.substr(i, p - i);
            if (isOuter) {
                if (k < 0 || k > 2) { err = "gl_TessLevelOuter[3] là quad-only (T1 triangles)"; return false; }
                out += "tcs_factors[tglmt_patchID].edgeTessellationFactor[" + idxs + "]";
            } else {
                if (k != 0) { err = "gl_TessLevelInner[1] là quad-only (T1 triangles)"; return false; }
                out += "tcs_factors[tglmt_patchID].insideTessellationFactor";
            }
            i = rb + 1;
        }
        body = out;
        return true;
    };
    if (!rewriteTessLevel("gl_TessLevelOuter", true)) return false;
    if (!rewriteTessLevel("gl_TessLevelInner", false)) return false;
    // 5. gl_in[e].gl_Position|gl_PointSize → tcs_in_gl_*[...]; .length() → N.
    // Member khác (gl_ClipDistance...) → fail rõ.
    {
        std::string out;
        size_t i = 0;
        const std::string k = "gl_in";
        while (i < body.size()) {
            size_t p = body.find(k, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            if (!isHead(p, k)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
            if (!hasGlIn) { err = "gl_in chưa khai báo (thiếu khối `in gl_PerVertex {...} gl_in[];`)"; return false; }
            size_t q = p + k.size();
            while (q < body.size() && isspace((unsigned char)body[q])) ++q;
            if (q < body.size() && body[q] == '.') {
                // gl_in.length() → N
                size_t r = q + 1;
                while (r < body.size() && isspace((unsigned char)body[r])) ++r;
                std::string what;
                size_t rr = r;
                while (rr < body.size() && IsIdentChar(body[rr], rr == r)) ++rr;
                what = body.substr(r, rr - r);
                if (what == "length" && rr < body.size() && body[rr] == '(') {
                    size_t rb = body.find(')', rr);
                    if (rb == std::string::npos) { err = "gl_in.length() thiếu )"; return false; }
                    out += body.substr(i, p - i);
                    out += std::to_string(N);
                    i = rb + 1;
                    continue;
                }
                err = "gl_in dùng sai (cần gl_in[e].member hoặc gl_in.length())";
                return false;
            }
            if (q >= body.size() || body[q] != '[') { err = "gl_in trần ngoài subset (cần gl_in[e].member)"; return false; }
            int dd = 0;
            size_t m = q;
            for (; m < body.size(); ++m) {
                if (body[m] == '[') ++dd;
                else if (body[m] == ']') { if (--dd == 0) break; }
            }
            if (m >= body.size()) { err = "gl_in[...] thiếu ]"; return false; }
            std::string idx = Trim(body.substr(q + 1, m - q - 1));
            size_t d = m + 1;
            while (d < body.size() && isspace((unsigned char)body[d])) ++d;
            if (d >= body.size() || body[d] != '.') { err = "gl_in[...] phải đi với .member"; return false; }
            size_t mn = d + 1;
            while (mn < body.size() && isspace((unsigned char)body[mn])) ++mn;
            size_t me = mn;
            while (me < body.size() && IsIdentChar(body[me], me == mn)) ++me;
            std::string mem = body.substr(mn, me - mn);
            out += body.substr(i, p - i);
            if (mem == "gl_Position")
                out += "tcs_in_gl_Position[tglmt_patchID*" + std::to_string(N) + " + (" + idx + ")]";
            else if (mem == "gl_PointSize")
                out += "tcs_in_gl_PointSize[tglmt_patchID*" + std::to_string(N) + " + (" + idx + ")]";
            else { err = "gl_in." + mem + " ngoài subset T1 (chỉ gl_Position/gl_PointSize)"; return false; }
            i = me;
        }
        body = out;
    }
    // 6. Mảng per-vertex đã khai báo: `nm[e]` → tcs_in|out_nm[PID*N + (e)];
    // `nm.length()` → N. Bare `nm` còn sót → fail (truyền mảng vào helper — T1).
    // Same-name in+out (passthrough Iris): LHS gán (=,++,--,op=) → out-form,
    // còn lại → in-form (đọc thường là input; ghi input là lỗi GLSL).
    for (auto& v : vars) {
        if (v.isPatch) continue;
        std::string inForm = "tcs_in_" + v.name + "[tglmt_patchID*" + std::to_string(N) + " + (";
        std::string outForm = "tcs_out_" + v.name + "[tglmt_patchID*" + std::to_string(N) + " + (";
        bool hasIn = v.isInput, hasOut = !v.isInput;
        // (vars chứa 2 entry cùng tên khi vừa in vừa out — tìm cả hai.)
        for (auto& w : vars) {
            if (w.name != v.name || w.isPatch) continue;
            if (w.isInput) hasIn = true; else hasOut = true;
        }
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            size_t p = body.find(v.name, i);
            if (p == std::string::npos) { out += body.substr(i); break; }
            if (!isHead(p, v.name)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
            size_t q = p + v.name.size();
            size_t qs = q;
            while (qs < body.size() && isspace((unsigned char)body[qs])) ++qs;
            // `.length()` → N (trước check `[`, vì `length` không phải index).
            if (qs < body.size() && body[qs] == '.') {
                size_t r = qs + 1;
                while (r < body.size() && isspace((unsigned char)body[r])) ++r;
                size_t rr = r;
                while (rr < body.size() && IsIdentChar(body[rr], rr == r)) ++rr;
                if (body.substr(r, rr - r) == "length" && rr < body.size() && body[rr] == '(') {
                    size_t rb = body.find(')', rr);
                    if (rb == std::string::npos) { err = v.name + ".length() thiếu )"; return false; }
                    out += body.substr(i, p - i);
                    out += std::to_string(N);
                    i = rb + 1;
                    continue;
                }
                // `.xyz` swizzle trên mảng?? invalid GLSL — để lọt sẽ lỗi MSL khó hiểu.
                // Fail rõ luôn (an toàn hơn để RewriteIdents/MSL báo mơ hồ).
                err = v.name + " dùng như scalar (mảng per-vertex phải index [e]): " + v.name;
                return false;
            }
            if (qs >= body.size() || body[qs] != '[') {
                err = "mảng per-vertex `" + v.name + "` dùng không index (truyền mảng vào helper?) ngoài subset T1";
                return false;
            }
            int dd = 0;
            size_t m = qs;
            for (; m < body.size(); ++m) {
                if (body[m] == '[') ++dd;
                else if (body[m] == ']') { if (--dd == 0) break; }
            }
            if (m >= body.size()) { err = v.name + "[...] thiếu ]"; return false; }
            std::string idx = Trim(body.substr(qs + 1, m - qs - 1));
            // LHS gán? (sau `]`: `=` (không `==`), `++`, `--`, `op=`).
            size_t a = m + 1;
            while (a < body.size() && isspace((unsigned char)body[a])) ++a;
            bool isWrite = false;
            if (a < body.size()) {
                if (body[a] == '=') isWrite = !(a + 1 < body.size() && body[a + 1] == '=');
                else if ((body[a] == '+' || body[a] == '-') && a + 1 < body.size() && body[a + 1] == body[a]) isWrite = true;
                else if ((body[a] == '+' || body[a] == '-' || body[a] == '*' || body[a] == '/' ||
                          body[a] == '%' || body[a] == '&' || body[a] == '|' || body[a] == '^') &&
                         a + 1 < body.size() && body[a + 1] == '=') isWrite = true;
            }
            out += body.substr(i, p - i);
            if (isWrite && hasOut) out += outForm + idx + ")]";
            else if (!isWrite && hasIn) out += inForm + idx + ")]";
            else if (hasOut) out += outForm + idx + ")]";
            else out += inForm + idx + ")]";
            i = m + 1;
        }
        body = out;
    }
    // 7. Patch vars: scalar `pn` → tcs_patch_pn[pid] (`pn[` → fail);
    // array `pa[e]` → tcs_patch_pa[pid*M + (e)] ; bare → fail ; `.length()` → M.
    for (auto& v : vars) {
        if (!v.isPatch) continue;
        if (v.arraySize == 0) {
            std::string out;
            size_t i = 0;
            while (i < body.size()) {
                size_t p = body.find(v.name, i);
                if (p == std::string::npos) { out += body.substr(i); break; }
                if (!isHead(p, v.name)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
                size_t q = p + v.name.size();
                size_t qs = q;
                while (qs < body.size() && isspace((unsigned char)body[qs])) ++qs;
                if (qs < body.size() && body[qs] == '[') {
                    err = "patch scalar `" + v.name + "` không index được";
                    return false;
                }
                out += body.substr(i, p - i);
                out += "tcs_patch_" + v.name + "[tglmt_patchID]";
                i = q;
            }
            body = out;
        } else {
            std::string form = "tcs_patch_" + v.name + "[tglmt_patchID*" + std::to_string(v.arraySize) + " + (";
            std::string out;
            size_t i = 0;
            while (i < body.size()) {
                size_t p = body.find(v.name, i);
                if (p == std::string::npos) { out += body.substr(i); break; }
                if (!isHead(p, v.name)) { out += body.substr(i, p - i + 1); i = p + 1; continue; }
                size_t q = p + v.name.size();
                size_t qs = q;
                while (qs < body.size() && isspace((unsigned char)body[qs])) ++qs;
                if (qs < body.size() && body[qs] == '.') {
                    size_t r = qs + 1;
                    while (r < body.size() && isspace((unsigned char)body[r])) ++r;
                    size_t rr = r;
                    while (rr < body.size() && IsIdentChar(body[rr], rr == r)) ++rr;
                    if (body.substr(r, rr - r) == "length" && rr < body.size() && body[rr] == '(') {
                        size_t rb = body.find(')', rr);
                        if (rb == std::string::npos) { err = v.name + ".length() thiếu )"; return false; }
                        out += body.substr(i, p - i);
                        out += std::to_string(v.arraySize);
                        i = rb + 1;
                        continue;
                    }
                    err = "patch array `" + v.name + "` dùng như scalar";
                    return false;
                }
                if (qs >= body.size() || body[qs] != '[') {
                    err = "patch array `" + v.name + "` dùng không index ngoài subset T1";
                    return false;
                }
                int dd = 0;
                size_t m = qs;
                for (; m < body.size(); ++m) {
                    if (body[m] == '[') ++dd;
                    else if (body[m] == ']') { if (--dd == 0) break; }
                }
                if (m >= body.size()) { err = v.name + "[...] thiếu ]"; return false; }
                std::string idx = Trim(body.substr(qs + 1, m - qs - 1));
                out += body.substr(i, p - i);
                out += form + idx + ")]";
                i = m + 1;
            }
            body = out;
        }
    }
    return true;
}
// P9: rewrite atomicCounterIncrement/Decrement/Get → MSL atomics explicit.
// Increment(ac) → fetch_add(ac,1u,relaxed); Decrement → fetch_sub; Get → load.
static bool RewriteAtomicCalls(std::string& io, const std::vector<std::string>& names, std::string& err) {
    std::string& body = io;
    auto isHead = [&](size_t p, const char* name) -> bool {
        size_t n = strlen(name);
        if (body.compare(p, n, name) != 0) return false;
        if (p > 0 && IsIdentChar(body[p - 1], false)) return false;
        size_t e = p + n;
        if (e < body.size() && IsIdentChar(body[e], false)) return false;
        return true;
    };
    std::string out;
    size_t i = 0;
    while (i < body.size()) {
        bool matched = false;
        for (const char* fn : {"atomicCounterIncrement", "atomicCounterDecrement", "atomicCounterGet"}) {
            if (!isHead(i, fn)) continue;
            size_t lp = i + strlen(fn);
            while (lp < body.size() && isspace((unsigned char)body[lp])) ++lp;
            if (lp >= body.size() || body[lp] != '(') continue;
            // Tìm `)` đóng (args đơn giản, không nested parens phức tạp — atomic chỉ có 1 arg).
            int dd = 0;
            size_t m = lp;
            for (; m < body.size(); ++m) {
                if (body[m] == '(') ++dd;
                else if (body[m] == ')') { if (--dd == 0) break; }
            }
            if (m >= body.size()) { err = std::string(fn) + " ngoặc lỗi"; return false; }
            std::string arg = Trim(body.substr(lp + 1, m - lp - 1));
            // Verify arg là atomic counter đã khai báo (nếu names không rỗng).
            if (!names.empty()) {
                bool found = false;
                for (auto& nm : names) if (nm == arg) { found = true; break; }
                if (!found) { err = std::string(fn) + " counter chưa khai báo: " + arg; return false; }
            }
            if (!strcmp(fn, "atomicCounterIncrement"))
                out += "atomic_fetch_add_explicit((" + arg + "), 1u, memory_order_relaxed)";
            else if (!strcmp(fn, "atomicCounterDecrement"))
                out += "atomic_fetch_sub_explicit((" + arg + "), 1u, memory_order_relaxed)";
            else
                out += "atomic_load_explicit((" + arg + "), memory_order_relaxed)";
            i = m + 1;
            matched = true;
            break;
        }
        if (!matched) out += body[i++];
    }
    body = out;
    return true;
}
// Viết lại imageLoad(img, coord) → img.read(uintN(coord)),
// imageStore(img, coord, v) → img.write(v, uintN(coord)),
// imageSize(img) → intN(get_width/height/depth).
// imageAtomic* → fail trung thực (MSL không có image atomics).
static bool RewriteImageCalls(std::string& io, const ImgTable& tab, std::string& err) {
    std::string& body = io;
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
    auto isHead = [&](size_t p, const char* name) -> bool {
        size_t n = strlen(name);
        if (body.compare(p, n, name) != 0) return false;
        if (p > 0 && IsIdentChar(body[p - 1], false)) return false;
        size_t e = p + n;
        if (e < body.size() && IsIdentChar(body[e], false)) return false;
        return true;
    };
    // imageAtomic* → fail rõ (không rewrite, không để lọt sang MSL).
    for (const char* an : {"imageAtomicAdd", "imageAtomicMin", "imageAtomicMax",
                           "imageAtomicAnd", "imageAtomicOr", "imageAtomicXor",
                           "imageAtomicExchange", "imageAtomicCompSwap"}) {
        size_t p = 0;
        while ((p = body.find(an, p)) != std::string::npos) {
            if (isHead(p, an)) { err = std::string(an) + " ngoài subset (MSL không có image atomics)"; return false; }
            p += strlen(an);
        }
    }
    std::string out;
    size_t i = 0;
    while (i < body.size()) {
        bool matched = false;
        for (const char* fn : {"imageLoad", "imageStore", "imageSize"}) {
            if (!isHead(i, fn)) continue;
            size_t lp = i + strlen(fn);
            while (lp < body.size() && isspace((unsigned char)body[lp])) ++lp;
            if (lp >= body.size() || body[lp] != '(') continue;
            size_t kend = 0;
            std::vector<std::string> args;
            if (!splitArgs(lp, kend, args)) { err = std::string(fn) + " cú pháp lỗi"; return false; }
            if (args.empty()) { err = std::string(fn) + " thiếu image"; return false; }
            std::string img = args[0];
            auto it = tab.find(img);
            if (it == tab.end()) { err = std::string(fn) + " image không khai báo: " + img; return false; }
            char kind = it->second;
            // Cube: MSL texturecube KHÔNG có .read/.write (probe: no matching member)
            // → imageCube ngoài subset.
            if (kind == 'C') { err = "imageCube ngoài subset (MSL texturecube không có .read/.write)"; return false; }
            bool isArr = (kind == 'A');
            bool is3 = (kind == '3');
            // 2D: uint2(coord); 3D: uint3(coord); Array: tách ivec3 → uint2(.xy)+uint(.z).
            if (!strcmp(fn, "imageLoad")) {
                if (args.size() != 2) { err = "imageLoad cần 2 args"; return false; }
                if (isArr)
                    out += img + ".read(uint2((" + args[1] + ").xy), uint((" + args[1] + ").z))";
                else if (is3)
                    out += img + ".read(uint3(" + args[1] + "))";
                else
                    out += img + ".read(uint2(" + args[1] + "))";
            } else if (!strcmp(fn, "imageStore")) {
                if (args.size() != 3) { err = "imageStore cần 3 args"; return false; }
                if (isArr)
                    out += img + ".write((" + args[2] + "), uint2((" + args[1] + ").xy), uint((" + args[1] + ").z))";
                else if (is3)
                    out += img + ".write((" + args[2] + "), uint3(" + args[1] + "))";
                else
                    out += img + ".write((" + args[2] + "), uint2(" + args[1] + "))";
            } else { // imageSize
                if (args.size() != 1) { err = "imageSize cần 1 arg"; return false; }
                if (kind == '2')
                    out += "int2(int(" + img + ".get_width()), int(" + img + ".get_height()))";
                else if (kind == 'A')
                    out += "int3(int(" + img + ".get_width()), int(" + img + ".get_height()), int(" + img + ".get_array_size()))";
                else // '3'
                    out += "int3(int(" + img + ".get_width()), int(" + img + ".get_height()), int(" + img + ".get_depth()))";
            }
            i = kend + 1;
            matched = true;
            break;
        }
        if (!matched) out += body[i++];
    }
    body = out;
    return true;
}
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
            } else if (sv.kind == '1') {
                // P3: sampler1D ≈ texture2d w×1 — y=0.5 rơi đúng texel hàng 0.
                rep = sname + "_tex.sample(" + sname + "_smp, float2(" + args[1] + ", 0.5)" + bias + ")";
            } else if (sv.kind == '3' || sv.kind == 'x' || sv.kind == 'y') {
                // P3: sampler3D → texture3d<float/int/uint>.
                rep = sname + "_tex.sample(" + sname + "_smp, float3(" + args[1] + ")" + bias + ")";
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
                // Buffer texture đóng khung W=min(n,16384) hàng (Metal giới hạn
                // width 16384; n=710178 → validate fail → abort). Đọc index i:
                //   x = i % W, y = i / W (clamp y về hàng cuối → OOB-safe, đúng
                //   GL "đọc quá size = undefined" thay vì GPU fault).
                std::string ix = "uint(" + args[1] + ")";
                std::string tn = sname + "_tex";
                rep = tn + ".read(uint2(" + ix + " % " + tn + ".get_width(), min(" + ix +
                      " / " + tn + ".get_width(), " + tn + ".get_height() - 1u)))";
            } else {
                if (args.size() < 2 || args.size() > 3) { err = "texelFetch 2D cần (sampler, coord[, lod])"; return false; }
                std::string lod = args.size() == 3 ? args[2] : "0";
                if (sv.kind == '1')
                    rep = sname + "_tex.read(uint2(uint(" + args[1] + "), 0u), " + lod + ")";
                else if (sv.kind == '3' || sv.kind == 'x' || sv.kind == 'y')
                    rep = sname + "_tex.read(uint3(" + args[1] + "), " + lod + ")";
                else
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
            if (sv.kind == '1')
                out += sname + "_tex.sample(" + sname + "_smp, float2(" + args[1] + ", 0.5), float(" + args[2] + "))";
            else if (sv.kind == '3' || sv.kind == 'x' || sv.kind == 'y')
                out += sname + "_tex.sample(" + sname + "_smp, float3(" + args[1] + "), float(" + args[2] + "))";
            else if (sv.kind == '2')
                out += sname + "_tex.sample(" + sname + "_smp, float2(" + args[1] + "), float(" + args[2] + "))";
            else { err = "textureLod chỉ hỗ trợ sampler2D/1D/3D"; return false; }
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
            // vec3 → chia /z, vec4 → /w (GLSL textureProj). Phát hiện qua literal
            // `vec3(`/`vec4(`; biến không rõ → mặc định /w (vec4, đúng end_portal).
            bool isVec3 = coord.rfind("vec3(", 0) == 0;
            out += sname + "_tex.sample(" + sname + "_smp, float2((" + coord + ").xy / (" +
                   coord + ")." + (isVec3 ? "z" : "w") + ")" + bias + ")";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureSize(sampler, lod) → get_width/get_height ----
    // Đã kiểm chứng MSL: get_width(uint)/get_height(uint) compile thật.
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureSize")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 11);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureSize(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 2) { err = "textureSize cần (sampler, lod)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureSize sampler không khai báo: " + sname; return false; }
            std::string lod = "uint(" + args[1] + ")";
            std::string tn = sname + "_tex";
            std::string rep;
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') {
                rep = "int(" + tn + ".get_width())"; // buffer: chỉ width
            } else if (sv.kind == 'A') {
                // array: ivec3(w,h,array_size)
                rep = "int3(int(" + tn + ".get_width(" + lod + ")), int(" + tn +
                      ".get_height(" + lod + ")), int(" + tn + ".get_array_size()))";
            } else if (sv.kind == 'C' || sv.kind == '2' || sv.kind == 'S' ||
                       sv.kind == 'i' || sv.kind == 'u') {
                rep = "int2(int(" + tn + ".get_width(" + lod + ")), int(" + tn +
                      ".get_height(" + lod + ")))";
            } else if (sv.kind == '1') {
                rep = "int(" + tn + ".get_width(" + lod + "))";
            } else if (sv.kind == '3' || sv.kind == 'x' || sv.kind == 'y') {
                rep = "int3(int(" + tn + ".get_width(" + lod + ")), int(" + tn +
                      ".get_height(" + lod + ")), int(" + tn + ".get_depth(" + lod + ")))";
            } else {
                err = "textureSize không hỗ trợ sampler kind cho " + sname;
                return false;
            }
            out += rep;
            i = k + 1;
        }
        body = out;
    }
    // ---- textureGather(sampler, comp[, offset]) → .gather() ----
    // MSL: t.gather(smp, uv[, int2 offset]) — đã kiểm chứng compile thật.
    // GLSL gather(s, P, comp): comp là component index; Metal gather luôn lấy
    // 4 texel quanh P (tương ứng comp=0..3 qua swizzle sau). Giữ gather nguyên
    // (comp!=0罕见) — nếu comp!=0 sẽ swizzle kết quả theo comp.
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureGather")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 13);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureGather(...) ngoặc không cân bằng"; return false; }
            if (args.size() < 2 || args.size() > 4) { err = "textureGather cần (sampler, uv[, comp[, offset]])"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureGather sampler không khai báo: " + sname; return false; }
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') {
                err = "textureGather trên buffer sampler: " + sname;
                return false;
            }
            std::string tn = sname + "_tex", sn = sname + "_smp";
            std::string uv = "float2(" + args[1] + ")";
            std::string comp = "";
            std::string off = "";
            if (args.size() >= 3) comp = Trim(args[2]);
            if (args.size() == 4) off = ", int2(" + Trim(args[3]) + ")";
            else if (args.size() == 3 && comp.find_first_not_of("0123456789") == std::string::npos) {
                // 3 args: (s, uv, comp) — comp là số
            }
            std::string rep = tn + ".gather(" + sn + ", " + uv + off + ")";
            // comp != 0: swizzle kết quả (Metal gather lấy component 0 mặc định)
            if (!comp.empty() && comp != "0") {
                const char* comps[4] = {"x", "y", "z", "w"};
                int ci = atoi(comp.c_str());
                if (ci >= 0 && ci < 4) rep = "(" + rep + ")." + comps[ci];
                else { err = "textureGather comp phải hằng số 0..3: " + comp; return false; }
            }
            out += rep;
            i = k + 1;
        }
        body = out;
    }
    // ---- textureOffset(sampler, uv, offset[, bias]) → sample + int2 offset ----
    // MSL: t.sample(smp, uv, int2 offset[, bias]) — đã kiểm chứng compile thật.
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureOffset")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 13);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureOffset(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 3 && args.size() != 4) { err = "textureOffset cần (sampler, uv, offset[, bias])"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureOffset sampler không khai báo: " + sname; return false; }
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') { err = "textureOffset trên buffer: " + sname; return false; }
            std::string tn = sname + "_tex", sn = sname + "_smp";
            std::string uv = sv.kind == 'S' ? "float2((" + args[1] + ").xy)" : "float2(" + args[1] + ")";
            // MSL: sample(s, coord, bias, offset) — bias đứng TRƯỚC offset.
            std::string bias = args.size() == 4 ? ", bias(float(" + args[3] + "))" : "";
            out += tn + ".sample(" + sn + ", " + uv + bias + ", int2(" + args[2] + "))";
            i = k + 1;
        }
        body = out;
    }
    // ---- texelFetchOffset(sampler, coord, lod, offset) → read + offset cộng tay ----
    // MSL .read không có offset; GLSL texelFetchOffset = fetch tại coord+offset
    // → read(uint2(coord+offset), lod) — đúng GL (đã kiểm chứng read(uint2,uint) thật).
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "texelFetchOffset")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 16);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "texelFetchOffset(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 4) { err = "texelFetchOffset cần (sampler, coord, lod, offset)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "texelFetchOffset sampler không khai báo: " + sname; return false; }
            std::string tn = sname + "_tex";
            out += tn + ".read(uint2((" + args[1] + ") + (" + args[3] + ")), uint(" + args[2] + "))";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureLodOffset(sampler, uv, lod, offset) → sample + level + offset ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureLodOffset")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 16);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureLodOffset(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 4) { err = "textureLodOffset cần (sampler, uv, lod, offset)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureLodOffset sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2' && sv.kind != 'S') { err = "textureLodOffset chỉ hỗ trợ sampler2D/Shadow: " + sname; return false; }
            std::string tn = sname + "_tex", sn = sname + "_smp";
            out += tn + ".sample(" + sn + ", float2(" + args[1] + "), level(float(" + args[2] +
                   ")), int2(" + args[3] + "))";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureGradOffset(sampler, uv, dPdx, dPdy, offset) → sample + gradient + offset ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureGradOffset")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 17);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureGradOffset(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 5) { err = "textureGradOffset cần (sampler, uv, dPdx, dPdy, offset)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureGradOffset sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2') { err = "textureGradOffset chỉ hỗ trợ sampler2D"; return false; }
            std::string tn = sname + "_tex", sn = sname + "_smp";
            out += tn + ".sample(" + sn + ", float2(" + args[1] + "), gradient2d(float2(" + args[2] +
                   "), float2(" + args[3] + ")), int2(" + args[4] + "))";
            i = k + 1;
        }
        body = out;
    }
    // ---- textureGatherOffset(sampler, uv, offset[, comp]) → gather + int2 offset ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureGatherOffset")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 19);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureGatherOffset(...) ngoặc không cân bằng"; return false; }
            if (args.size() < 3 || args.size() > 4) { err = "textureGatherOffset cần (sampler, uv, offset[, comp])"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureGatherOffset sampler không khai báo: " + sname; return false; }
            if (sv.kind == 'B' || sv.kind == 'I' || sv.kind == 'U') { err = "textureGatherOffset trên buffer: " + sname; return false; }
            std::string tn = sname + "_tex", sn = sname + "_smp";
            std::string rep = tn + ".gather(" + sn + ", float2(" + args[1] + "), int2(" + args[2] + "))";
            if (args.size() == 4) {
                std::string comp = Trim(args[3]);
                if (comp != "0" && comp.find_first_not_of("0123456789") == std::string::npos) {
                    const char* comps[4] = {"x", "y", "z", "w"};
                    int ci = atoi(comp.c_str());
                    if (ci >= 0 && ci < 4) rep = "(" + rep + ")." + comps[ci];
                }
            }
            out += rep;
            i = k + 1;
        }
        body = out;
    }
    // ---- textureProjLod(sampler, coord, lod) → proj chia tay + sample level ----
    {
        std::string out;
        size_t i = 0;
        while (i < body.size()) {
            if (!isHead(i, "textureProjLod")) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 14);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            std::vector<std::string> args;
            size_t k = 0;
            if (!splitArgs(lp, k, args)) { err = "textureProjLod(...) ngoặc không cân bằng"; return false; }
            if (args.size() != 3) { err = "textureProjLod cần (sampler, coord, lod)"; return false; }
            std::string sname = Trim(args[0]);
            SampEntry sv;
            if (!findSamp(sname, sv)) { err = "textureProjLod sampler không khai báo: " + sname; return false; }
            if (sv.kind != '2') { err = "textureProjLod chỉ hỗ trợ sampler2D"; return false; }
            std::string coord = Trim(args[1]);
            bool isVec3 = coord.rfind("vec3(", 0) == 0;
            std::string tn = sname + "_tex", sn = sname + "_smp";
            out += tn + ".sample(" + sn + ", float2((" + coord + ").xy / (" + coord + ")." +
                   (isVec3 ? "z" : "w") + "), level(float(" + args[2] + ")))";
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
// ============ uint/int → float implicit trong `*`/`/` (Sodium) ====
// GLSL cho implicit int/uint→float trong phép toán số học (`uint3 * float`); MSL cấm → bọc
// floatN()/float() phía uint KHI CHỨNG MINH được cả hai phía:
//  - một bên là call `f(...)` mà f (helper def trong rest) trả về uint/uint2/3/4
//    (hoặc int*), và
//  - bên kia là float-literal (`4.0`, `0.5f`), floatN(...) ctor, hoặc ident float
//    đã biết (const float global trong rest).
// Chỉ `*` và `/` (bỏ qua `*=`/`/=` — mixed compound-assign đã là lỗi GLSL;
// không chạm so sánh, bitwise, `+`/`-`). Không chạm operand ngoặc `(...)`
// (không chứng minh được type), member access, hay bare ident (kể cả const uint).
// Quan sát thực tế (Sodium block_layer_opaque.vsh, crash 2026-10-15):
//   `(_deinterleave_u20x3(a_Position) * VERTEX_SCALE)` (uint3*float-const)
//   `_get_relative_chunk_coord(pos) * vec3(16.0)` (uint3*float3-ctor)
// An toàn sai số: uint→float bảo toàn (<2^24; tọa độ Sodium <2^20).
// Code từng fail Metal vẫn fail (message khác) — không biến lỗi câm thành đúng:
// rule chỉ fire khi một bên CHẮC uint-call và bên kia CHẮC float.
static void PromoteUintVecFloatMixing(std::string& text, const std::string& restGLSL) {
    // 1. helper nào trả về uint/int (cả spellings GLSL vì rest chưa RI).
    // Trùng tên khác kiểu → bỏ (mơ hồ, không đoán).
    std::map<std::string, std::string> uintRets; // fname → "float"/"float2/3/4"
    {
        auto wrapOf = [](const std::string& rt) -> std::string {
            if (rt == "uint" || rt == "int") return "float";
            // float-family: đánh dấu "?" để mismatch với int-family → erase (mơ hồ).
            // Trước đây trả "" → skip → map giữ wrap "float" của overload int
            // (bug thật: `pow2(color.rgb)` bị bọc thành `float(pow2(...))` — vô lệnh).
            if (rt == "float" || rt == "vec2" || rt == "float2" || rt == "vec3" ||
                rt == "float3" || rt == "vec4" || rt == "float4")
                return "?";
            if (rt == "uint2" || rt == "int2" || rt == "uvec2" || rt == "ivec2") return "float2";
            if (rt == "uint3" || rt == "int3" || rt == "uvec3" || rt == "ivec3") return "float3";
            if (rt == "uint4" || rt == "int4" || rt == "uvec4" || rt == "ivec4") return "float4";
            return "";
        };
        for (auto& h : ParseHelperDefs(restGLSL, true)) {
            size_t ns = restGLSL.find(h.name, h.defStart);
            if (ns == std::string::npos || ns == h.defStart) continue;
            std::string rt = Trim(restGLSL.substr(h.defStart, ns - h.defStart));
            if (rt.find(' ') != std::string::npos) continue; // `static`/qualifier lạ → bỏ
            std::string w = wrapOf(rt);
            if (w.empty()) continue;
            auto it = uintRets.find(h.name);
            if (it == uintRets.end()) uintRets[h.name] = w;
            else if (it->second != w) uintRets.erase(it); // overload khác kiểu → mơ hồ
        }
        for (auto it = uintRets.begin(); it != uintRets.end();)
            it = (it->second == "?") ? uintRets.erase(it) : std::next(it);
    }
    if (uintRets.empty()) return;
    // 2. const float globals trong rest (`const float X` / `constant float X`).
    // Chỉ depth 0 (top-level): loại main-locals `const float k` (kẻo suy luận
    // sai scope khi trùng tên ở helper khác — vd helper param `int k`).
    std::vector<std::string> floatConsts;
    {
        int depth = 0;
        bool inStr = false;
        for (size_t k = 0; k < restGLSL.size(); ++k) {
            char ch = restGLSL[k];
            if (inStr) { if (ch == '"') inStr = false; continue; }
            if (ch == '"') { inStr = true; continue; }
            if (ch == '{') { ++depth; continue; }
            if (ch == '}') { --depth; continue; }
            if (depth != 0) continue;
            for (const char* kw : {"constant float ", "const float "}) {
                size_t kl = strlen(kw);
                if (restGLSL.compare(k, kl, kw) != 0) continue;
                bool lb = k > 0 && IsIdentChar(restGLSL[k - 1], false);
                if (lb) continue;
                size_t s = k + kl;
                while (s < restGLSL.size() && isspace((unsigned char)restGLSL[s])) ++s;
                size_t e = s;
                while (e < restGLSL.size() && IsIdentChar(restGLSL[e], e == s)) ++e;
                if (e > s) floatConsts.push_back(restGLSL.substr(s, e - s));
                break;
            }
        }
        std::sort(floatConsts.begin(), floatConsts.end());
        floatConsts.erase(std::unique(floatConsts.begin(), floatConsts.end()), floatConsts.end());
    }
    auto isFloatCtor = [](const std::string& w) {
        return w == "float" || w == "float2" || w == "float3" || w == "float4";
    };
    auto isFloatIdent = [&](const std::string& w) {
        for (auto& c : floatConsts)
            if (c == w) return true;
        return false;
    };
    // float literal? (`4.0`, `0.5f`, `.5` — phải chứa '.').
    auto isFloatLit = [](const std::string& tok) {
        if (tok.empty()) return false;
        size_t i = (tok[0] == '.' ? 1 : 0);
        bool dot = false, dig = false;
        for (; i < tok.size(); ++i) {
            char c = tok[i];
            if (c == '.') { if (dot) return false; dot = true; }
            else if (isdigit((unsigned char)c)) dig = true;
            else if ((c == 'f' || c == 'F') && i + 1 == tok.size()) break;
            else return false;
        }
        return dot && dig;
    };
    auto isBareName = [&](const std::string& e) {
        if (e.empty() || !(isalpha((unsigned char)e[0]) || e[0] == '_')) return false;
        for (size_t q = 1; q < e.size(); ++q)
            if (!IsIdentChar(e[q], false)) return false;
        return true;
    };
    // Đọc operand ngay TRƯỚC op (bỏ spaces): trả về {kind, text, callSpan}.
    // kind: 'call' (fname + span), 'ctor' (floatN call), 'lit' (float literal),
    // 'ident' (const-float), 'other'.
    struct Opnd { char kind = 0; std::string text, fname; size_t a = 0, b = 0; };
    auto readBack = [&](size_t opPos, Opnd& o) -> bool {
        size_t e = opPos;
        while (e > 0 && isspace((unsigned char)text[e - 1])) --e;
        if (e == 0) return false;
        char c = text[e - 1];
        if (c == ')') {
            int dd = 0;
            size_t k = e;
            for (; k > 0; --k) {
                if (text[k - 1] == ')') ++dd;
                else if (text[k - 1] == '(') { if (--dd == 0) break; }
            }
            if (k == 0) return false;
            size_t lp = k - 1; // '('
            size_t fs = lp;
            while (fs > 0 && isspace((unsigned char)text[fs - 1])) --fs;
            size_t fe = fs;
            while (fe > 0 && IsIdentChar(text[fe - 1], false)) --fe;
            std::string fn = text.substr(fe, fs - fe);
            if (!isBareName(fn)) { o.kind = 'o'; o.text = text.substr(lp, e - lp); o.a = lp; o.b = e; return true; }
            o.kind = 'c'; o.fname = fn; o.a = fe; o.b = e;
            o.text = text.substr(fe, e - fe);
            return true;
        }
        // token (literal/ident) kết thúc tại e. Member access (`a.b`) không chứng
        // minh được type → kind 'o' ở dưới (bỏ qua, an toàn).
        size_t s = e;
        while (s > 0 && (IsIdentChar(text[s - 1], false) || text[s - 1] == '.')) --s;
        std::string tok = text.substr(s, e - s);
        if (isFloatLit(tok)) { o.kind = 'l'; o.text = tok; o.a = s; o.b = e; return true; }
        if (isBareName(tok)) { o.kind = 'i'; o.text = tok; o.a = s; o.b = e; return true; }
        o.kind = 'o'; o.text = tok; o.a = s; o.b = e;
        return true;
    };
    auto readFwd = [&](size_t opPos, Opnd& o) -> bool {
        size_t s = opPos + 1;
        while (s < text.size() && isspace((unsigned char)text[s])) ++s;
        if (s < text.size() && (text[s] == '-' || text[s] == '+')) {
            // unary trước operand phức tạp (vd `* -f(...)`) — ngoài scope hẹp
            o.kind = 'o'; o.text = ""; o.a = o.b = s;
            return true;
        }
        if (s >= text.size()) return false;
        if (isdigit((unsigned char)text[s]) || text[s] == '.') {
            size_t e = s;
            while (e < text.size() && (isdigit((unsigned char)text[e]) || text[e] == '.' ||
                                       text[e] == 'f' || text[e] == 'F' || text[e] == 'e' ||
                                       text[e] == 'E' || text[e] == '+' || text[e] == '-')) {
                // `+`/`-` chỉ hợp lệ trong exponent (`1e-3`); đơn giản: dừng ở op
                if ((text[e] == '+' || text[e] == '-') && e > s &&
                    text[e - 1] != 'e' && text[e - 1] != 'E')
                    break;
                ++e;
            }
            std::string tok = text.substr(s, e - s);
            if (isFloatLit(tok)) { o.kind = 'l'; o.text = tok; o.a = s; o.b = e; return true; }
            o.kind = 'o'; o.text = tok; o.a = s; o.b = e;
            return true;
        }
        if (isalpha((unsigned char)text[s]) || text[s] == '_') {
            size_t e = s + 1;
            while (e < text.size() && IsIdentChar(text[e], false)) ++e;
            std::string w = text.substr(s, e - s);
            size_t q = e;
            while (q < text.size() && isspace((unsigned char)text[q])) ++q;
            if (q < text.size() && text[q] == '(') {
                // call: cân bằng ngoặc để lấy span
                int dd = 0;
                size_t m = q;
                for (; m < text.size(); ++m) {
                    if (text[m] == '(') ++dd;
                    else if (text[m] == ')') { if (--dd == 0) break; }
                }
                if (m >= text.size()) return false;
                o.kind = 'c'; o.fname = w; o.a = s; o.b = m + 1;
                o.text = text.substr(s, m + 1 - s);
                return true;
            }
            o.kind = 'i'; o.text = w; o.a = s; o.b = e;
            return true;
        }
        o.kind = 'o'; o.text = ""; o.a = o.b = s;
        return true;
    };
    auto isFloatProven = [&](const Opnd& o) -> bool {
        if (o.kind == 'l') return true;
        if (o.kind == 'c' && isFloatCtor(o.fname)) return true;
        if (o.kind == 'i' && isFloatIdent(o.text)) return true;
        return false;
    };
    auto isUintCall = [&](const Opnd& o, std::string& wrap) -> bool {
        if (o.kind != 'c') return false;
        auto it = uintRets.find(o.fname);
        if (it == uintRets.end()) return false;
        wrap = it->second;
        return true;
    };
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        size_t p = std::string::npos;
        char op = 0;
        for (size_t k = i; k < text.size(); ++k) {
            if (text[k] == '*' || text[k] == '/') { p = k; op = text[k]; break; }
        }
        if (p == std::string::npos) { out += text.substr(i); break; }
        // loại `*=` `/=` (kế là `=`) và `//` (đã strip comments nhưng chắc ăn)
        if (p + 1 < text.size() && (text[p + 1] == '=' || (op == '/' && text[p + 1] == '/'))) {
            out += text.substr(i, p - i + 1); i = p + 1; continue;
        }
        // prev-nonspace là `=`? (`a *= b`, `x == ...` không chứa */ đơn — vẫn loại cho chắc)
        {
            size_t q = p;
            while (q > 0 && isspace((unsigned char)text[q - 1])) --q;
            if (q > 0 && (text[q - 1] == '=' || text[q - 1] == '!' || text[q - 1] == '<' ||
                          text[q - 1] == '>' || text[q - 1] == '&' || text[q - 1] == '|' ||
                          text[q - 1] == '^' || text[q - 1] == '%' || text[q - 1] == '(' ||
                          text[q - 1] == ',' || text[q - 1] == ';' || text[q - 1] == '{' ||
                          text[q - 1] == '}' || text[q - 1] == '[' || text[q - 1] == ':' ||
                          text[q - 1] == '?' || text[q - 1] == '+' || text[q - 1] == '-' ||
                          text[q - 1] == '*' || text[q - 1] == '/')) {
                out += text.substr(i, p - i + 1); i = p + 1; continue;
            }
        }
        (void)op;
        Opnd L, Rr;
        if (!readBack(p, L) || !readFwd(p, Rr)) { out += text.substr(i, p - i + 1); i = p + 1; continue; }
        std::string wrap;
        bool fireL = isUintCall(L, wrap) && isFloatProven(Rr);
        bool fireR = false;
        if (!fireL) {
            // LHS float-proven + RHS uint-call (đối xứng)
            bool lok = (L.kind == 'l') || (L.kind == 'c' && isFloatCtor(L.fname)) ||
                       (L.kind == 'i' && isFloatIdent(L.text));
            if (lok && isUintCall(Rr, wrap)) fireR = true;
        }
        if (!fireL && !fireR) { out += text.substr(i, p - i + 1); i = p + 1; continue; }
        // Fire LHS: bọc operand + EMIT LUÔN đoạn giữa tới hết operator, i qua op.
        // (KHÔNG được i = L.b: text gốc không đổi nên vòng sau tìm lại đúng op này
        // → fire lại vô hạn. Từng treo Sodium chunk ở đây.)
        out += text.substr(i, (fireL ? L.a : Rr.a) - i);
        if (fireL) {
            out += wrap + "(" + text.substr(L.a, L.b - L.a) + ")";
            out += text.substr(L.b, p + 1 - L.b);
            i = p + 1;
        } else {
            out += wrap + "(" + text.substr(Rr.a, Rr.b - Rr.a) + ")";
            i = Rr.b;
        }
        // NOTE: chỉ bọc 1 phía/operator; chuỗi `a*b*c` xử lý dần từng vòng (an toàn).
    }
    text.swap(out);
}
// ============ mix(x, y, boolN(...)) → mix(x, y, floatN(boolN(...))) (Sodium) ====
// MSL không có `mix(floatN, floatN, boolN)` (đã quan sát metal thật:
// `no matching function for call to 'mix'`). Bọc selector bool-vector ctor
// bằng floatN(...) — `floatN(boolN)` là explicit conversion hợp lệ.
    // Chỉ khi arg3 là `bool2(`/`bool3(`/`bool4(` ctor; bool scalar (`mix(f,f,b)`)
// MSL có overload nên giữ nguyên; bool-vector BIẾN (không ctor) vẫn fail rõ
// (hiếm — code từng fail vẫn fail, không đoán type biến).
static void PromoteBoolVecMix(std::string& text) {
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        size_t p = text.find("mix(", i);
        if (p == std::string::npos) { out += text.substr(i); break; }
        bool l = p > 0 && IsIdentChar(text[p - 1], false);
        if (l) { out += text.substr(i, p - i + 1); i = p + 1; continue; }
        // cân bằng ngoặc từ '(' của mix(
        size_t lp = p + 3;
        int dd = 0;
        size_t m = lp;
        for (; m < text.size(); ++m) {
            if (text[m] == '(') ++dd;
            else if (text[m] == ')') { if (--dd == 0) break; }
        }
        if (m >= text.size()) { out += text.substr(i); break; }
        std::string inside = text.substr(lp + 1, m - lp - 1);
        std::vector<std::string> args = SplitCallArgs(inside);
        if (args.size() == 3) {
            std::string a3 = Trim(args[2]);
            std::string wrap;
            if (a3.rfind("bool2(", 0) == 0) wrap = "float2";
            else if (a3.rfind("bool3(", 0) == 0) wrap = "float3";
            else if (a3.rfind("bool4(", 0) == 0) wrap = "float4";
            if (!wrap.empty()) {
                out += text.substr(i, p - i);
                out += "mix(" + args[0] + ", " + args[1] + ", " + wrap + "(" + a3 + "))";
                i = m + 1;
                continue;
            }
            // GLSL mix(vec, vec, bvec-biểu-thức) — MSL không có overload bvec →
            // select(x, y, mask) (cùng ngữ nghĩa c ? y : x). Heuristic: 3rd arg
            // chứa op so sánh ở tầng ngoài cùng (GLSL scalar-bool vào mix vec là
            // invalid → chỉ còn bvec). Chuẩn hoá bỏ 1 lớp ngoặc bao ngoài trước
            // (`(color < x)` vs ExpandRelationalBuiltins `(color) < (x)`).
            if (a3.size() > 2 && a3.front() == '(' && a3.back() == ')') {
                std::string core = a3;
                int dd2 = 0;
                bool enclosed = true;
                for (size_t k = 0; k < core.size(); ++k) {
                    if (core[k] == '(') ++dd2;
                    else if (core[k] == ')') {
                        if (--dd2 == 0 && k + 1 < core.size()) {
                            enclosed = false;
                            break;
                        }
                    }
                }
                if (enclosed) core = Trim(core.substr(1, core.size() - 2));
                bool cmp = false;
                dd2 = 0;
                for (size_t k = 0; k < core.size(); ++k) {
                    char c = core[k];
                    if (c == '(' || c == '[') { ++dd2; continue; }
                    if (c == ')' || c == ']') { --dd2; continue; }
                    if (dd2 != 0) continue;
                    if (c == '<' || c == '>') { cmp = true; break; }
                    if ((c == '=' || c == '!') && k + 1 < core.size() && core[k + 1] == '=') {
                        cmp = true;
                        break;
                    }
                }
                if (cmp) {
                    out += text.substr(i, p - i);
                    out += "select(" + Trim(args[0]) + ", " + Trim(args[1]) + ", " + a3 + ")";
                    i = m + 1;
                    continue;
                }
            }
        }
        out += text.substr(i, p - i + 4);
        i = p + 4;
    }
    text.swap(out);
}
// GLSL ngầm chuyển int→float ở builtin args (`min(1, floatExpr)`), MSL bắt
// khớp overload chính xác → ambiguous (composite.fsh `min(1, x.z)`). Bọc
// int-literal thành `float(N)` khi call min/max/clamp có arg float-ish khác
// (float literal / float-ctor / swizzle `ident.xyzw`). Heuristic — không suy
// luận biến int (int-swizzle hiếm; metal step sẽ bắt nếu nhầm).
static void PromoteIntLitInMinMaxClamp(std::string& text) {
    static const char* kFns[] = {"min(", "max(", "clamp(", nullptr};
    auto isIntLit = [](std::string a) {
        a = Trim(a);
        if (a.empty()) return false;
        size_t s = (a[0] == '-' || a[0] == '+') ? 1 : 0;
        if (s >= a.size()) return false;
        for (size_t q = s; q < a.size(); ++q)
            if (!isdigit((unsigned char)a[q])) return false;
        return true;
    };
    auto isFloatish = [](const std::string& raw) {
        std::string a = Trim(raw);
        if (a.empty()) return false;
        for (size_t q = 0; q + 1 < a.size(); ++q)
            if (a[q] == '.' && isdigit((unsigned char)a[q + 1])) return true;
        static const char* kCt[] = {"float(", "float2(", "float3(", "float4(",
                                    "vec2(", "vec3(", "vec4(", nullptr};
        for (const char** cp = kCt; *cp; ++cp)
            if (a.rfind(*cp, 0) == 0) return true;
        size_t dot = a.find('.');
        if (dot != std::string::npos && dot > 0 && dot + 1 < a.size()) {
            bool lOk = isalpha((unsigned char)a[0]) || a[0] == '_';
            for (size_t q = 1; q < dot && lOk; ++q)
                if (!IsIdentChar(a[q], false)) lOk = false;
            bool rOk = true;
            for (size_t q = dot + 1; q < a.size() && rOk; ++q)
                if (a[q] != 'x' && a[q] != 'y' && a[q] != 'z' && a[q] != 'w' &&
                    a[q] != 'r' && a[q] != 'g' && a[q] != 'b' && a[q] != 'a')
                    rOk = false;
            if (lOk && rOk) return true;
        }
        return false;
    };
    std::string out;
    size_t i = 0;
    while (i < text.size()) {
        size_t best = std::string::npos;
        size_t fnLen = 0;
        for (const char** f = kFns; *f; ++f) {
            size_t p = text.find(*f, i);
            if (p != std::string::npos && (best == std::string::npos || p < best)) {
                best = p;
                fnLen = strlen(*f) - 1;
            }
        }
        if (best == std::string::npos) { out += text.substr(i); break; }
        size_t lp = best + fnLen; // tại '('
        bool lb = best > 0 && IsIdentChar(text[best - 1], false);
        if (lb) { out += text.substr(i, lp - i); i = lp; continue; }
        int dd = 0;
        size_t m = lp;
        for (; m < text.size(); ++m) {
            if (text[m] == '(') ++dd;
            else if (text[m] == ')') { if (--dd == 0) break; }
        }
        if (m >= text.size()) { out += text.substr(i); break; }
        std::vector<std::string> args = SplitCallArgs(text.substr(lp + 1, m - lp - 1));
        bool hasIntLit = false, hasFloatish = false;
        for (auto& a : args) {
            if (isIntLit(a)) hasIntLit = true;
            else if (isFloatish(a)) hasFloatish = true;
        }
        if (args.size() < 2 || !hasIntLit || !hasFloatish) {
            out += text.substr(i, m - i + 1);
            i = m + 1;
            continue;
        }
        out += text.substr(i, lp - i + 1); // "min(" ...
        for (size_t k = 0; k < args.size(); ++k) {
            if (k) out += ", ";
            if (isIntLit(args[k])) out += "float(" + Trim(args[k]) + ")";
            else out += Trim(args[k]);
        }
        out += ")";
        i = m + 1;
    }
    text.swap(out);
}
static bool ThreadHelpersMSL(
    std::string& helpersMSL, std::string& body,
    const std::map<std::string, std::vector<std::pair<std::string, SampEntry>>>& helperSamp,
    const SampTable& globalSamp, const std::vector<std::string>& uboBlocks, bool isVertex,
    const std::vector<GLSLVar>& vertexAttrs, const std::vector<GLSLVar>& outVars,
    const std::vector<std::pair<std::string, std::string>>& entryBuiltins,
    std::string& err) {
    // P-Sodium: attribute (vs) / varying (fs) map (name → mslType) để thread vào
    // helpers đọc trực tiếp (vd a_TexCoord trong _vert_init; `texCoord` trong
    // helper deferred1.fsh). Body dùng `_in.X`, helper nhận param cùng tên.
    std::map<std::string, std::string> attrTypes;
    for (auto& a : vertexAttrs) attrTypes[a.name] = a.mslType;
    // O: out-varying của VS đọc trong helper (`wave *= clamp(lmCoord.y ...)`):
    // helper nhận `thread T&` cùng tên, body truyền `_out.X` (read+write-through).
    std::map<std::string, std::string> outTypes;
    if (isVertex)
        for (auto& o : outVars) outTypes[o.name] = o.mslType;
    // VS SHADOW_LINES (Iris isNewLines — VanillaTransformer.java:196-224): Iris
    // rename main → irisMain rồi sinh wrapper main + helper `iris_widen_lines`,
    // cả hai đọc/ghi `gl_Position` TRỰC TIẾP trong HELPER. mpVary chỉ rewrite
    // body → helper giữ `gl_Position` → MSL "undeclared identifier". Rewrite
    // helper về tên out-var ảo `tglmt_position` để dùng nguyên cơ chế O:
    // (thread float4& + argFor sinh `_out.tglmt_position`).
    if (isVertex) {
        bool found = false;
        size_t p = 0;
        while ((p = helpersMSL.find("gl_Position", p)) != std::string::npos) {
            size_t e = p + 11;
            bool l = p > 0 && IsIdentChar(helpersMSL[p - 1], false);
            bool r = e < helpersMSL.size() && IsIdentChar(helpersMSL[e], false);
            if (!l && !r) {
                found = true;
                break;
            }
            p = e;
        }
        if (found) {
            std::string out;
            out.reserve(helpersMSL.size());
            size_t i = 0;
            while (i < helpersMSL.size()) {
                if (helpersMSL.compare(i, 11, "gl_Position") == 0 &&
                    (i == 0 || !IsIdentChar(helpersMSL[i - 1], false)) &&
                    (i + 11 >= helpersMSL.size() ||
                     !IsIdentChar(helpersMSL[i + 11], false))) {
                    out += "tglmt_position";
                    i += 11;
                    continue;
                }
                out += helpersMSL[i];
                ++i;
            }
            helpersMSL.swap(out);
            outTypes["tglmt_position"] = "float4";
        }
    }
    // Entry builtins đọc được trong helper (tglmt_fragCoord/...): thread như
    // attribute — param cùng tên, body truyền tên entry, helper truyền param.
    std::map<std::string, std::string> builtinTypes;
    for (auto& eb : entryBuiltins) builtinTypes[eb.first] = eb.second;
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
        // P3: mọi kind sample()-được đều cần _smp; chỉ B/I/U (buffer read()) không.
        if (k != 'B' && k != 'I' && k != 'U') items.push_back("T:" + sname + "_smp");
    };
    // GLSL overload CÙNG arity khác kiểu tham số (Complementary: pow2 int/float/vec2/3/4,
    // sqrt*, clamp01...). MSL cũng overload được → nếu mọi candidate cùng shape gọi được
    // (req y hệt + splitKind y hệt theo vị trí) thì chọn candidate đầu: call site giữ tên,
    // MSL tự chọn overload theo kiểu tại compile time. Khác req/kind → vẫn mơ hồ thật.
    auto resolveOverload = [&](const std::string& nm, int arity, Def*& out) -> bool {
        Def* f = nullptr;
        for (auto& d : defs) {
            if (d.name != nm || d.origCount != arity) continue;
            if (!f) { f = &d; continue; }
            if (d.req != f->req || d.formalNames.size() != f->formalNames.size()) return false;
            for (size_t i = 0; i < d.formalNames.size(); ++i) {
                char ka = 0, kb = 0;
                auto ia = d.splitKind.find(d.formalNames[i]);
                auto ib = f->splitKind.find(f->formalNames[i]);
                if (ia != d.splitKind.end()) ka = ia->second;
                if (ib != f->splitKind.end()) kb = ib->second;
                if (ka != kb) return false;
            }
        }
        out = f;
        return f != nullptr;
    };
    auto declFor = [&](const std::string& item) -> std::string {
        if (item == "U:") return "constant TGLMTUniforms& tglmt_u";
        if (item.rfind("B:", 0) == 0) return "constant TGLMTUBO_" + item.substr(2) + "& ubo_" + item.substr(2);
        // P-Sodium: attribute → value param CÙNG TÊN (body helper dùng ngay, khỏi rewrite).
        if (item.rfind("A:", 0) == 0) {
            std::string nm = item.substr(2);
            auto it = attrTypes.find(nm);
            if (it == attrTypes.end()) return "";
            return it->second + " " + nm;
        }
        // O: out-varying của VS — thread reference (helper có thể đọc/ghi).
        if (item.rfind("O:", 0) == 0) {
            std::string nm = item.substr(2);
            auto it = outTypes.find(nm);
            if (it == outTypes.end()) return "";
            return "thread " + it->second + "& " + nm;
        }
        if (item.rfind("V:", 0) == 0) {
            std::string nm = item.substr(2);
            auto it = builtinTypes.find(nm);
            if (it == builtinTypes.end()) return "";
            return it->second + " " + nm;
        }
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
    auto argFor = [&](const std::string& item, bool isBody) -> std::string {
        if (item == "U:") return "tglmt_u";
        if (item.rfind("B:", 0) == 0) return "ubo_" + item.substr(2);
        if (item.rfind("T:", 0) == 0) return item.substr(2);
        // P-Sodium: attribute — main có stage_in (`_in.X`), helper dùng param cùng tên.
        if (item.rfind("A:", 0) == 0) {
            std::string nm = item.substr(2);
            return isBody ? ("_in." + nm) : nm;
        }
        // Entry builtin (tglmt_fragCoord/...): tên giống nhau ở mọi scope.
        if (item.rfind("V:", 0) == 0) return item.substr(2);
        // O: out-varying — body truyền `_out.X`, helper truyền param cùng tên.
        if (item.rfind("O:", 0) == 0) {
            std::string nm = item.substr(2);
            return isBody ? ("_out." + nm) : nm;
        }
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
        // Attribute (vs) / varying (fs) đọc trong helper tên thô → thread param
        // (body truyền `_in.X`). Formal trùng tên (shadowing) thì dùng param sẵn.
        for (auto& [anm, aty] : attrTypes) {
            (void)aty;
            if (isOwn(anm)) continue;
            if (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, anm))
                mine.push_back("A:" + anm);
        }
        for (auto& [bnm, bty] : builtinTypes) {
            (void)bty;
            if (isOwn(bnm)) continue;
            if (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, bnm))
                mine.push_back("V:" + bnm);
        }
        for (auto& [onm, oty] : outTypes) {
            (void)oty;
            if (isOwn(onm)) continue;
            if (!hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, onm)) continue;
            // local cùng tên (GLSL shadowing) → helper đọc local, không phải varying
            bool localDecl = false;
            {
                size_t p = d.bodyStart;
                while ((p = helpersMSL.find(onm, p)) != std::string::npos &&
                       p < d.bodyEnd + 1) {
                    size_t pe = p + onm.size();
                    if (pe < helpersMSL.size() && IsIdentChar(helpersMSL[pe], false)) {
                        p = pe;
                        continue;
                    }
                    size_t q = p;
                    while (q > d.bodyStart && isspace((unsigned char)helpersMSL[q - 1])) --q;
                    size_t we = q;
                    while (we > d.bodyStart && IsIdentChar(helpersMSL[we - 1], false)) --we;
                    std::string prev = helpersMSL.substr(we, q - we);
                    if (prev == "float" || prev == "float2" || prev == "float3" ||
                        prev == "float4" || prev == "int" || prev == "uint" ||
                        prev == "bool") {
                        localDecl = true;
                        break;
                    }
                    p = pe;
                }
            }
            if (!localDecl) mine.push_back("O:" + onm);
        }
        for (auto& [sn, se] : globalSamp) {
            (void)se;
            bool used = (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, sn + "_tex") && !isOwn(sn + "_tex")) ||
                        (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, sn + "_smp") && !isOwn(sn + "_smp")) ||
                        // sampler global truyền làm ARG cho helper khác (chưa expand
                        // `_tex/_smp` ở phase này): `textureCatmullRom(colortex2, ...)`
                        // → phải thread pair vào caller (composite6.fsh DoTAA).
                        (hasWordIn(helpersMSL, d.bodyStart, d.bodyEnd + 1, sn) && !isOwn(sn));
            if (used) {
                std::vector<std::string> items;
                globalPairItems(sn, items);
                for (auto& it : items) {
                    std::string nm = it.substr(2);
                    if (!isOwn(nm)) mine.push_back(it);
                }
            }
        }
        for (const char* vw : {"_in", "_out"}) {
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
                int nmatch = 0;
                for (auto& d : defs)
                    if (d.name == cs.callee && (int)cs.actuals.size() == d.origCount) {
                        if (!target) target = &d;
                        ++nmatch;
                    }
                if (nmatch == 0) {
                    err = "helper " + cs.callee + " sai arity (co " +
                          std::to_string(cs.actuals.size()) + ")";
                    return false;
                }
                if (nmatch > 1 && !resolveOverload(cs.callee, (int)cs.actuals.size(), target)) {
                    err = "helper " + cs.callee + " gọi mơ hồ (trùng arity n=" +
                          std::to_string(cs.actuals.size()) + " defs=";
                    for (auto& d2 : defs)
                        if (d2.name == cs.callee)
                            err += d2.key + "@" + std::to_string(d2.lp) + " ";
                    err += ")";
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
                        else if (it.rfind("A:", 0) == 0) a = it.substr(2);
                        else if (it.rfind("O:", 0) == 0) a = "_out." + it.substr(2);
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
                    if (k != 'B' && k != 'I' && k != 'U') {
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
        // Rewrite call sites kiểu đệ quy từ trong ra ngoài: call lồng nhau
        // (TAAJitter trong ScreenToView) phải nhận inner args TRƯỚC rồi mới map
        // outer — batching edit theo offset dễ hỏng khi span chồng lấn.
        bool rwOk = true;
        std::vector<std::string> rwNames;
        for (auto& d : defs) rwNames.push_back(d.name);
        std::sort(rwNames.begin(), rwNames.end());
        rwNames.erase(std::unique(rwNames.begin(), rwNames.end()), rwNames.end());
        auto expandIn = [&](const std::string& text, bool isBody, auto&& self) -> std::string {
            std::string out;
            int depth = 0;
            size_t i = 0;
            while (i < text.size() && rwOk) {
                char ch = text[i];
                if (isalpha((unsigned char)ch) || ch == '_') {
                    size_t e = i;
                    while (e < text.size() && IsIdentChar(text[e], false)) ++e;
                    std::string nm = text.substr(i, e - i);
                    size_t q = e;
                    while (q < text.size() && isspace((unsigned char)text[q])) ++q;
                    bool isCall = q < text.size() && text[q] == '(' &&
                                  std::binary_search(rwNames.begin(), rwNames.end(), nm);
                    if (!isCall) {
                        out += text.substr(i, e - i);
                        i = e;
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
                    if (c >= text.size()) { out += text.substr(i); break; }
                    // def header (`... ) {`) hoặc prototype (`;` depth<1) — không phải call
                    size_t b = c + 1;
                    while (b < text.size() && isspace((unsigned char)text[b])) ++b;
                    bool isHeader = b < text.size() && text[b] == '{';
                    bool isProto = !isBody && depth < 1 && b < text.size() && text[b] == ';';
                    if (isHeader || isProto) {
                        out += text.substr(i, c + 1 - i);
                        i = c + 1;
                        continue;
                    }
                    std::string inner = self(text.substr(q + 1, c - q - 1), isBody, self);
                    if (!rwOk) break;
                    std::vector<std::string> actuals = SplitCallArgs(inner);
                    if (actuals.size() == 1 && Trim(actuals[0]).empty()) actuals.clear();
                    Def* target = nullptr;
                    int nmatch = 0;
                    for (auto& d : defs)
                        if (d.name == nm && (int)actuals.size() == d.origCount) {
                            if (!target) target = &d;
                            ++nmatch;
                        }
                    if (nmatch == 0) {
                        err = "helper " + nm + " sai arity (co " + std::to_string(actuals.size()) + ")";
                        rwOk = false;
                        break;
                    }
                    if (nmatch > 1 && !resolveOverload(nm, (int)actuals.size(), target)) {
                        err = "helper " + nm + " gọi mơ hồ (trùng arity kids=";
                        for (auto& d2 : defs)
                            if (d2.name == nm) err += d2.key + " ";
                        err += ")";
                        rwOk = false;
                        break;
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
                            if (ai >= actuals.size()) { err = "thiếu arg cho sampler " + fn; rwOk = false; break; }
                            std::string E = Trim(actuals[ai++]);
                            if (!isBareName(E)) { err = "arg sampler phức tạp ngoài subset: " + E; rwOk = false; break; }
                            emitArg(E + "_tex");
                            char kk = sk->second;
                            if (kk == '2' || kk == 'S' || kk == 'C' || kk == 'A') emitArg(E + "_smp");
                            continue;
                        }
                        if (ai >= actuals.size()) { err = "thiếu arg cho " + fn; rwOk = false; break; }
                        emitArg(actuals[ai++]);
                    }
                    if (!rwOk) break;
                    if (ai != actuals.size()) { err = "thừa arg khi gọi " + target->name; rwOk = false; break; }
                    for (auto& item : target->req) {
                        std::string a = argFor(item, isBody);
                        if (!a.empty()) emitArg(a);
                    }
                    out += text.substr(i, q + 1 - i);
                    out += rebuilt;
                    out += ")";
                    i = c + 1;
                    continue;
                }
                if (ch == '{') ++depth;
                else if (ch == '}') --depth;
                out += ch;
                ++i;
            }
            if (!rwOk) { out += text.substr(i); }
            return out;
        };
        {
            std::string nb = expandIn(body, true, expandIn);
            if (!rwOk) return false;
            body.swap(nb);
            std::string nh = expandIn(helpersMSL, false, expandIn);
            if (!rwOk) return false;
            helpersMSL.swap(nh);
        }
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
    R.isCompute = (stage == 0x91B9);
    R.isTessControl = (stage == 0x8E88);
    R.isTessEval = (stage == 0x8E87);
    R.entryPoint = R.isVertex ? "TGLMT_vs" : (R.isCompute ? "TGLMT_cs" : (R.isTessControl ? "TGLMT_tcs" : "TGLMT_fs"));
    auto fail = [&](const std::string& m) { R.ok = false; R.log = m; return R; };
    // P-Tess T2 chưa làm: TES fail rõ ngay đầu (gate gl_shader cũng chặn).
    if (R.isTessEval)
        return fail("tessellation evaluation shader chưa hỗ trợ (P-Tess T2)");

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
                        t.find("430") == std::string::npos &&
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
                // Chỉ thị lạ (#error, #import sót, ...): trong nhánh ifdef TẮT thì
                // bỏ qua (đúng C-preprocessor/GLSL — chỉ if*/else/endif có hiệu lực
                // trong skipped groups; vd Sodium guard `#else #error ... #endif`).
                // Nhánh ACTIVE gặp #error → fail thật (đúng GL: #error luôn là lỗi
                // biên dịch), giữ message gốc của shader trong log.
                if (!active()) continue;
                return fail("directive không hỗ trợ: " + t);
            }
            if (!active()) continue; // nhánh ifdef tắt
            if (t.rfind("precision", 0) == 0) continue; // MSL bỏ qua precision
            // P6f compute: `layout(local_size_x=8[,y,z]) in;` → bắt localSize, bỏ dòng
            // (MSL không có local_size declaration; dispatch dùng threadsPerThreadgroup).
            if (R.isCompute && t.rfind("layout", 0) == 0 && t.find("local_size") != std::string::npos) {
                auto grab = [&](const char* key, int def) {
                    size_t p = t.find(key);
                    if (p == std::string::npos) return def;
                    p = t.find('=', p);
                    if (p == std::string::npos) return def;
                    return atoi(t.c_str() + p + 1);
                };
                R.localSize[0] = grab("local_size_x", 1);
                R.localSize[1] = grab("local_size_y", 1);
                R.localSize[2] = grab("local_size_z", 1);
                if (R.localSize[0] < 1 || R.localSize[1] < 1 || R.localSize[2] < 1)
                    return fail("local_size phải >= 1: " + t);
                continue; // bỏ dòng layout local_size
            }
            // P-Tess T1: `layout(vertices = N) out;` → bắt patch vertices, bỏ dòng
            // (MSL không có khai báo tương đương; T4 dispatch grid=(N, numPatches,1)).
            // Chỉ khớp đúng dạng patch-vertices (sau ')' phải là `out`), tránh
            // nhầm layout khác chứa chữ "vertices" (vd gl_PatchVerticesIn trong body
            // không bao giờ ở đầu dòng top-level dạng này — vẫn kiểm chặt).
            if (R.isTessControl && t.rfind("layout", 0) == 0 && t.find("vertices") != std::string::npos) {
                size_t rp = t.find(')');
                std::string after = (rp == std::string::npos) ? "" : Trim(t.substr(rp + 1));
                // bỏ ';' cuối để so sánh (`out;` / `out`)
                if (!after.empty() && after.back() == ';') { after.pop_back(); after = Trim(after); }
                if (after == "out") {
                    size_t vp = t.find("vertices");
                    size_t eq = t.find('=', vp);
                    if (eq == std::string::npos) return fail("layout(vertices) thiếu =: " + t);
                    int n = atoi(t.c_str() + eq + 1);
                    if (n < 1 || n > 32) return fail("layout(vertices) phải 1..32 (GL MAX_PATCH_VERTICES): " + t);
                    if (R.tessVertices > 0) return fail("layout(vertices) trùng: " + t);
                    R.tessVertices = n;
                    continue; // bỏ dòng layout vertices
                }
                // layout lạ chứa "vertices" (không phải patch decl) → để flow chung fail rõ.
            }
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
    // P6e subroutine: type decls, impls (type→func), uniforms (var→type).
    // Dispatch fn sinh sau scan (switch trên uniform int, MSL không có fn pointer).
    struct SubType { std::string name, ret, params; };
    struct SubImpl { std::string type, name; };
    struct SubVar { std::string var, type; };
    std::vector<SubType> subTypes;
    std::vector<SubImpl> subImpls;
    std::vector<SubVar> subVars;
    std::vector<GLSLBlock> blocks;
    std::vector<HoistedGlobal> hoisted; // global mutable → main-local + thread-ref
    // P-Tess T1: khối gl_in `in gl_PerVertex {...} gl_in[];` đã khai báo chưa
    // (body dùng gl_in mà thiếu block = lỗi GLSL → fail rõ ở rewrite).
    bool hasGlInBlock = false;
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
            if (s.rfind("layout", 0) != 0 && s.rfind("uniform", 0) != 0 &&
                s.rfind("buffer", 0) != 0) return false;
            // layout(binding=N[, ...]) trước `uniform`: N là binding point thật
            // (GL: glUniformBlockBinding mặc định = layout binding, KHÔNG phải 0).
            // Bỏ qua = Globals đọc nhầm point 0 (đụng SamplerInfo 16B → zero fallback).
            out.binding = -1;
            if (s.rfind("layout", 0) == 0) {
                size_t lp = s.find('(');
                size_t rp = (lp == std::string::npos) ? std::string::npos : s.find(')', lp);
                if (lp != std::string::npos && rp != std::string::npos) {
                    std::string q = s.substr(lp + 1, rp - lp - 1);
                    size_t bp = q.find("binding");
                    if (bp != std::string::npos) {
                        size_t eq = q.find('=', bp);
                        if (eq != std::string::npos) out.binding = atoi(q.c_str() + eq + 1);
                    }
                }
            }
            // P6f SSBO: `buffer B { ... };` (read-write) vs UBO `uniform B { ... };`.
            size_t ub = s.find("uniform");
            size_t kb = (ub == std::string::npos) ? s.find("buffer") : std::string::npos;
            bool storage = false;
            size_t kw = ub, kwLen = 7;
            if (ub == std::string::npos && kb != std::string::npos) {
                storage = true; kw = kb; kwLen = 6;
            }
            if (kw == std::string::npos) return false;
            out.isStorage = storage;
            size_t brace = s.find('{', kw);
            if (brace == std::string::npos) return false;
            size_t close = s.find('}', brace);
            if (close == std::string::npos) return false;
            // tên block: từ sau "uniform"/"buffer" tới '{'
            std::string hdr = Trim(s.substr(kw + kwLen, brace - kw - kwLen));
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
                    // SSBO cho phép mảng không kích thước (float v[];) → arr = -1.
                    std::string cnt = Trim(mname.substr(lb + 1, rb - lb - 1));
                    if (cnt.empty()) {
                        if (!storage) return false; // UBO phải có kích thước
                        arr = -1;
                    } else {
                        arr = atoi(cnt.c_str());
                        if (arr <= 0 || arr > 1024) return false;
                    }
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
        // P6g image: kind từ tên kiểu GLSL ('2' 2D, 'A' 2DArray, 'C' Cube, '3' 3D).
        // Buffer/1D/Rect/MS hiếm → ngoài subset (fail rõ ở parser).
        // (ImageKind/ImageElem là static file-scope, dùng ở parser + rewrite.)
        // P6g: `layout(fmt[, binding=N]) uniform [readonly|writeonly] image2D img;`
        // → GLSLVar isImage. Format bắt buộc theo GLSL (không validate chi tiết,
        // backend bind texture thật). Binding -1 = chưa khai báo → glUniform1i.
        auto tryParseImage = [&](const std::string& t, GLSLVar& out) -> bool {
            std::string s = Trim(t);
            if (!s.empty() && s.back() == ';') s.pop_back();
            s = Trim(s);
            int binding = -1;
            if (s.rfind("layout", 0) == 0) {
                size_t lp = s.find('('), rp = s.find(')');
                if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return false;
                std::string inside = s.substr(lp + 1, rp - lp - 1);
                size_t bp = inside.find("binding");
                if (bp != std::string::npos) {
                    size_t eq = inside.find('=', bp);
                    if (eq != std::string::npos) binding = atoi(inside.c_str() + eq + 1);
                }
                s = Trim(s.substr(rp + 1));
            }
            std::istringstream iss(s);
            std::vector<std::string> w; std::string tok;
            while (iss >> tok) w.push_back(tok);
            if (w.empty()) return false;
            // Qualifier có thể đứng TRƯỚC hoặc SAU `uniform`
            // (GLSL: `writeonly uniform image3D img;` — Complementary shadowcomp).
            size_t k = 0;
            char access = 'B';
            bool sawUniform = false;
            while (k < w.size()) {
                if (w[k] == "uniform") { sawUniform = true; ++k; }
                else if (w[k] == "readonly") { access = 'R'; ++k; }
                else if (w[k] == "writeonly") { access = 'W'; ++k; }
                else if (w[k] == "coherent" || w[k] == "volatile" || w[k] == "restrict" ||
                         w[k] == "highp" || w[k] == "mediump" || w[k] == "lowp") { ++k; }
                else break;
            }
            if (!sawUniform || k + 1 >= w.size()) return false;
            std::string itype = w[k], iname = w[k + 1];
            if (!iname.empty() && iname.back() == ';') iname.pop_back();
            if (iname.find('[') != std::string::npos) return false; // image array ngoài subset
            char kind = ImageKind(itype);
            if (!kind) return false;
            // chỉ 1 declarator (image đơn, không `image2D a, b;`)
            if (k + 2 < w.size()) return false;
            out.glslType = itype; out.name = iname;
            out.isImage = true; out.imageAccess = access; out.imageBinding = binding;
            std::string elem = ImageElem(itype);
            std::string acc = (access == 'R') ? "access::read"
                            : (access == 'W') ? "access::write" : "access::read_write";
            std::string base = (kind == '2') ? "texture2d" : (kind == 'A') ? "texture2d_array"
                             : (kind == 'C') ? "texturecube" : "texture3d";
            out.mslType = base + "<" + elem + ", " + acc + ">";
            return true;
        };
        // P9 atomic counter: `layout(binding=N, offset=M) uniform atomic_uint ac;`
        // → GLSLVar isAtomicCounter. Binding/offset bắt buộc (GLSL yêu cầu layout).
        auto tryParseAtomic = [&](const std::string& t, GLSLVar& out) -> bool {
            std::string s = Trim(t);
            if (!s.empty() && s.back() == ';') s.pop_back();
            s = Trim(s);
            int binding = -1, offset = 0;
            if (s.rfind("layout", 0) == 0) {
                size_t lp = s.find('('), rp = s.find(')');
                if (lp == std::string::npos || rp == std::string::npos || rp <= lp) return false;
                std::string inside = s.substr(lp + 1, rp - lp - 1);
                size_t bp = inside.find("binding");
                if (bp != std::string::npos) {
                    size_t eq = inside.find('=', bp);
                    if (eq != std::string::npos) binding = atoi(inside.c_str() + eq + 1);
                }
                size_t op = inside.find("offset");
                if (op != std::string::npos) {
                    size_t eq = inside.find('=', op);
                    if (eq != std::string::npos) offset = atoi(inside.c_str() + eq + 1);
                }
                s = Trim(s.substr(rp + 1));
            }
            // s phải là `uniform atomic_uint name`
            std::istringstream iss(s);
            std::vector<std::string> w; std::string tok;
            while (iss >> tok) w.push_back(tok);
            if (w.size() < 3 || w[0] != "uniform") return false;
            // w[1] có thể là qualifier? atomic_uint không có readonly/writeonly.
            // Kiểm tra w[1]=="atomic_uint", w[2]==name (hoặc w có precision qualifier).
            size_t k = 1;
            while (k < w.size() && (w[k] == "highp" || w[k] == "mediump" || w[k] == "lowp")) ++k;
            if (k + 1 >= w.size() || w[k] != "atomic_uint") return false;
            std::string aname = w[k + 1];
            if (!aname.empty() && aname.back() == ';') aname.pop_back();
            if (aname.find('[') != std::string::npos || k + 2 < w.size()) return false;
            if (binding < 0) return false; // layout binding bắt buộc cho atomic
            out.glslType = "atomic_uint"; out.name = aname;
            out.isAtomicCounter = true; out.atomicBinding = binding; out.atomicOffset = offset;
            out.mslType = "device atomic_uint*";
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
            // P-Tess T1: khối gl_in `in gl_PerVertex {...} gl_in[];` cũng vậy
            // (không tách `gl_in[];` ra — parser cần cả khối để ghi nhận khai báo).
            auto segStartsUbo = [&](size_t s) {
                size_t p = s;
                while (p < t.size() && isspace((unsigned char)t[p])) ++p;
                if (t.compare(p, 6, "layout") == 0 || t.compare(p, 7, "uniform") == 0 ||
                    t.compare(p, 6, "buffer") == 0)
                    return true;
                // gl_in block: bắt đầu `in ` + chứa gl_PerVertex + gl_in.
                // Hẹp (3 điều kiện) để không chạm struct/function thường.
                if (t.compare(p, 3, "in ") == 0 && t.find("gl_PerVertex") != std::string::npos &&
                    t.find("gl_in") != std::string::npos)
                    return true;
                return false;
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
                            // `} lightmapInfo;`: instance name thuộc về UBO.
                            // P-Tess T1: `} gl_in[N];` cũng vậy (sized gl_in).
                            size_t k = j + 1;
                            while (k < t.size() && IsIdentChar(t[k], false)) ++k;
                            // nuốt `[N]` nếu có (gl_in sized; UBO array-instance vẫn
                            // fail ở tryParseUBO như cũ — chỉ đổi message).
                            if (k < t.size() && t[k] == '[') {
                                size_t rb = t.find(']', k);
                                if (rb != std::string::npos) k = rb + 1;
                            }
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
            // UBO/SSBO block (chứa '{' nhưng là decl)
            {
                GLSLBlock b;
                if ((t.rfind("layout", 0) == 0 || t.rfind("uniform", 0) == 0 ||
                     t.rfind("buffer", 0) == 0) &&
                    t.find('{') != std::string::npos && tryParseUBO(t, b)) {
                    blocks.push_back(b);
                    continue;
                }
            }
            // P6g image: `layout(...) uniform image2D img;` (không `{`, không phải UBO).
            {
                GLSLVar iv;
                if ((t.rfind("layout", 0) == 0 || t.rfind("uniform", 0) == 0 ||
                     t.find("uniform") != std::string::npos) &&
                    t.find("image") != std::string::npos && t.find('{') == std::string::npos &&
                    tryParseImage(t, iv)) {
                    R.images.push_back(iv);
                    continue;
                }
            }
            // P9 atomic: `layout(binding,offset) uniform atomic_uint ac;`.
            {
                GLSLVar av;
                if ((t.rfind("layout", 0) == 0 || t.rfind("uniform", 0) == 0) &&
                    t.find("atomic_uint") != std::string::npos && t.find('{') == std::string::npos &&
                    tryParseAtomic(t, av)) {
                    R.atomicCounters.push_back(av);
                    continue;
                }
            }
            // P-Tess: `patch in|out ...` (per-patch) — strip prefix, ParseDecls bên
            // dưới xử lý như decl thường, đánh dấu isPatch để validate + rewrite
            // buffer theo patch (index = patchID, không stride N).
            // TCS: chỉ `patch out` hợp lệ (`patch in` ở TCS là lỗi GL — patch do
            // TCS tạo ra). TES (T2): ngược lại, chỉ `patch in`.
            // Khối gl_in `in gl_PerVertex {...} gl_in[];` xử lý riêng ngay dưới.
            bool isPatchDecl = false;
            if (R.isTessControl || R.isTessEval) {
                std::string tp = t, layoutPart;
                if (tp.rfind("layout", 0) == 0) {
                    size_t rp = tp.find(')');
                    if (rp != std::string::npos) {
                        layoutPart = tp.substr(0, rp + 1);
                        tp = Trim(tp.substr(rp + 1));
                    }
                }
                if (tp.rfind("patch ", 0) == 0) {
                    std::string afterPatch = Trim(tp.substr(6));
                    bool isOut = (afterPatch.rfind("out", 0) == 0 &&
                                  (afterPatch.size() == 3 || isspace((unsigned char)afterPatch[3])));
                    bool isIn = (afterPatch.rfind("in", 0) == 0 &&
                                 (afterPatch.size() == 2 || isspace((unsigned char)afterPatch[2])));
                    if (!isOut && !isIn) return fail("patch phải đi với in/out: " + t);
                    if (R.isTessControl && !isOut)
                        return fail("TCS `patch in` không hợp lệ (patch do TCS tạo ra): " + t);
                    if (R.isTessEval && isOut)
                        return fail("TES `patch out` không hợp lệ: " + t);
                    isPatchDecl = true;
                    t = layoutPart.empty() ? afterPatch : layoutPart + " " + afterPatch;
                }
            }
            // P-Tess T1: khối gl_in `in gl_PerVertex { ... } gl_in[];` — KHÔNG phải
            // hàm (dù có `{`): ghi nhận khai báo, không emit vào rest (Iris dùng
            // gl_in[].gl_Position cho tess level theo khoảng cách — pattern chính).
            // Instance phải tên `gl_in` (đúng spec); size `[]`/sized đều nhận.
            // Segment phải BẮT ĐẦU bằng `in ` (main/helper chứa gl_in thì không).
            if (R.isTessControl && t.rfind("in ", 0) == 0 &&
                t.find("gl_PerVertex") != std::string::npos &&
                t.find("gl_in") != std::string::npos && t.find('{') != std::string::npos) {
                std::string s = Trim(t);
                // dạng: `in gl_PerVertex { decls } gl_in [N];`
                size_t close = s.rfind('}');
                if (s.rfind("in ", 0) != 0 || close == std::string::npos)
                    return fail("khối gl_in sai cú pháp: " + t);
                std::string tail = Trim(s.substr(close + 1));
                if (!tail.empty() && tail.back() == ';') { tail.pop_back(); tail = Trim(tail); }
                // tail = `gl_in` hoặc `gl_in[N]`
                size_t lb = tail.find('[');
                std::string inst = (lb == std::string::npos) ? tail : Trim(tail.substr(0, lb));
                if (inst != "gl_in") return fail("khối gl_in phải đặt tên instance `gl_in`: " + t);
                hasGlInBlock = true;
                continue;
            }
            // P6e subroutine: 3 dạng (type decl / impl / uniform).
            // Impl `subroutine(T) ret f(...) {...}` → strip prefix, hàm thường + ghi nhận.
            if (t.rfind("subroutine(", 0) == 0 && t.find('{') != std::string::npos) {
                size_t rp = t.find(')');
                if (rp == std::string::npos) return fail("subroutine impl thiếu ): " + t);
                std::string stype = Trim(t.substr(11, rp - 11)); // sau "subroutine("
                std::string stripped = Trim(t.substr(rp + 1)); // `ret f(...) {...}`
                // tách tên hàm: `ret name(` → name
                size_t lp2 = stripped.find('(');
                if (lp2 == std::string::npos) return fail("subroutine impl thiếu (: " + t);
                std::string before = Trim(stripped.substr(0, lp2));
                size_t sp = before.find_last_of(" \t*");
                if (sp == std::string::npos) return fail("subroutine impl thiếu tên: " + t);
                std::string fname = Trim(before.substr(sp + 1));
                subImpls.push_back({stype, fname});
                rest += stripped;
                if (!stripped.empty() && stripped.back() != ';' && stripped.back() != '}') rest += ";";
                rest += "\n";
                continue;
            }
            // Type `subroutine ret Type(params);` / uniform `subroutine uniform Type var;`
            // (không `{` — xử lý trước ParseDecls).
            if (t.rfind("subroutine", 0) == 0 && t.find('{') == std::string::npos) {
                std::string s = Trim(t);
                if (!s.empty() && s.back() == ';') s.pop_back();
                s = Trim(s);
                if (s.rfind("subroutine uniform", 0) == 0) {
                    // `subroutine uniform Type var;` → uniform int var (index cho dispatch).
                    std::string rr = Trim(s.substr(18));
                    std::istringstream iss(rr);
                    std::string stype, svar;
                    iss >> stype >> svar;
                    if (stype.empty() || svar.empty()) return fail("subroutine uniform lỗi: " + t);
                    subVars.push_back({svar, stype});
                    GLSLVar uv; uv.glslType = "int"; uv.mslType = "int"; uv.name = svar;
                    uniforms.push_back(uv); // vào tglmt_u, app set bằng glUniform1i
                    continue;
                } else {
                    // `subroutine ret Type(params);` → ghi nhận signature, không emit.
                    std::string rr = Trim(s.substr(10)); // sau "subroutine"
                    size_t lp2 = rr.find('('), rp2 = rr.find(')');
                    if (lp2 == std::string::npos || rp2 == std::string::npos || rp2 <= lp2)
                        return fail("subroutine type lỗi: " + t);
                    std::string before = Trim(rr.substr(0, lp2)); // `ret Type`
                    size_t sp = before.find_last_of(" \t*");
                    if (sp == std::string::npos) return fail("subroutine type thiếu tên: " + t);
                    std::string tret = Trim(before.substr(0, sp));
                    std::string tname = Trim(before.substr(sp + 1));
                    std::string tparams = Trim(rr.substr(lp2 + 1, rp2 - lp2 - 1));
                    subTypes.push_back({tname, tret, tparams});
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
            // Bao gồm: kiểu builtin, KIỂU STRUCT USER (identifier thuần — vd Iris
            // inject `iris_FogParameters irisInt_Fog = ...;`), mảng `vec3[] x = ...`
            // (bracket trên TYPE token, size suy từ initializer).
            {
                std::string mm = t;
                if (!mm.empty() && mm.back() == ';') mm.pop_back();
                mm = Trim(mm);
                std::istringstream ms(mm);
                std::string w0;
                ms >> w0;
                // Tách array suffix trên type token: `vec3[]`/`vec4[2]`.
                int typeArr = 0; // 0 = không mảng; -1 = unsized; >0 = sized
                bool typeHasBrackets = false;
                std::string baseType = w0;
                size_t lb0 = w0.find('[');
                bool typeOk = true;
                if (lb0 != std::string::npos) {
                    size_t rb0 = w0.find(']', lb0);
                    if (rb0 == std::string::npos) {
                        typeOk = false;
                    } else {
                        std::string inside = Trim(w0.substr(lb0 + 1, rb0 - lb0 - 1));
                        typeHasBrackets = true;
                        baseType = w0.substr(0, lb0);
                        if (inside.empty()) {
                            typeArr = -1;
                        } else {
                            bool digits = !inside.empty();
                            for (char c : inside)
                                if (!isdigit((unsigned char)c)) digits = false;
                            if (!digits) typeOk = false;
                            else typeArr = atoi(inside.c_str());
                        }
                    }
                }
                bool hOk = false;
                std::string hMt;
                if (typeOk) {
                    hMt = MSLType(baseType, hOk);
                    if (!hOk) {
                        // Kiểu user struct: identifier thuần → hoist như type thường.
                        bool plain = !baseType.empty() &&
                                     (isalpha((unsigned char)baseType[0]) || baseType[0] == '_');
                        for (size_t q = 1; plain && q < baseType.size(); ++q)
                            if (!IsIdentChar(baseType[q], false)) plain = false;
                        if (plain) { hOk = true; hMt = baseType; }
                    }
                }
                static const char* kStmtKw[] = {"if",  "for",  "while", "return", "void",
                                                "struct", "break", "continue", "discard",
                                                "else", "do", "switch", "const", "in",
                                                "out", "inout", "uniform", "attribute",
                                                "varying", "layout", "subroutine",
                                                "precision", "lowp", "mediump", "highp",
                                                "flat", "smooth", "noperspective",
                                                "centroid", "sample", "invariant",
                                                "precise", "patch", "coherent",
                                                "volatile", "restrict", "readonly",
                                                "writeonly", "shared", "buffer", nullptr};
                bool reserved = false;
                for (const char** kw = kStmtKw; *kw; ++kw)
                    if (w0 == *kw) reserved = true;
                if (hOk && !reserved && hMt != "sampler") {
                    std::string mrest = Trim(mm.substr(w0.size()));
                    std::vector<std::pair<std::string, int>> ds;
                    if (!mrest.empty() && SplitDeclarators(mrest, ds)) {
                        std::vector<HoistedGlobal> tmp;
                        bool okAll = true;
                        for (auto& [nm, arr] : ds) {
                            HoistedGlobal hg;
                            hg.glslType = baseType;
                            hg.mslType = hMt;
                            hg.name = nm;
                            int sz = arr > 0 ? arr : typeArr;
                            hg.init = ExtractDeclaratorInit(mrest, nm);
                            if (sz == -1 && typeHasBrackets) {
                                sz = CountInitElems(hg.init);
                                if (sz <= 0) { okAll = false; break; }
                            }
                            hg.arraySize = sz;
                            tmp.push_back(hg);
                        }
                        if (okAll) {
                            for (auto& h : tmp) hoisted.push_back(h);
                            continue;
                        }
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
            bool isTessIO = (R.isTessControl || R.isTessEval) && !isPatchDecl;
            if (ParseDecls(t, R.isVertex, dir, vs, isU, isTessIO || isPatchDecl)) {
                for (auto& v : vs) {
                    bool okT = false;
                    // P-Tess: mảng per-vertex/patch. Chuẩn hoá + validate ở đây
                    // (ParseDecls đã cho qua nhờ flag; R.tessVertices có từ preprocessor).
                    //  - patch scalar: giữ nguyên (index = patchID).
                    //  - patch array: phải ghi kích thước (unsized không rõ size buffer).
                    //  - TCS per-vertex (non-patch in/out): BẮT BUỘC mảng; unsized
                    //    `[]` (arr=-1) → = tessVertices; sized phải == tessVertices
                    //    (stride kernel dùng N — lệch là OOB; fail rõ thay vì UB câm).
                    //    Scalar per-vertex ở TCS là lỗi GLSL → fail rõ.
                    //  - TES (T2): per-vertex in/out là SCALAR (nội suy xong); array → fail.
                    if ((R.isTessControl || R.isTessEval) && !isU) {
                        v.isPatch = isPatchDecl;
                        if (R.isTessControl && R.tessVertices < 1)
                            return fail("thiếu layout(vertices=N) out (TCS bắt buộc)");
                        if (isPatchDecl) {
                            if (v.arraySize == -1)
                                return fail("patch array phải ghi kích thước (unsized không rõ size buffer): " + v.name);
                        } else if (R.isTessControl) {
                            if (v.arraySize == 0)
                                return fail("TCS per-vertex in/out phải là mảng []: " + v.name);
                            if (v.arraySize == -1) v.arraySize = R.tessVertices;
                            else if (v.arraySize != R.tessVertices)
                                return fail("mảng per-vertex phải khớp layout(vertices=" +
                                            std::to_string(R.tessVertices) + "): " + v.name);
                        } else { // TES: per-vertex in/out scalar
                            if (v.arraySize != 0)
                                return fail("TES per-vertex in/out là scalar (đã nội suy), không phải mảng: " + v.name);
                        }
                    }
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
                                v.glslType == "samplerCube" || v.glslType == "sampler2DArray" ||
                                v.glslType == "sampler1D" || v.glslType == "sampler3D" ||
                                v.glslType == "isampler2D" || v.glslType == "usampler2D" ||
                                v.glslType == "isampler3D" || v.glslType == "usampler3D")) {
                        // samplerCube → texturecube (panorama); Array → texture2d_array;
                        // Shadow → texture2d approx (bỏ compare ref, xem texture() bên dưới).
                        // P3: 1D → texture2d (storage w×1); 3D → texture3d; i/u → <int>/<uint>.
                        v.mslType = "sampler";
                        v.isSampler = true;
                        v.isCube = (v.glslType == "samplerCube");
                        v.isArray = (v.glslType == "sampler2DArray");
                        v.isShadow = (v.glslType == "sampler2DShadow");
                        v.is1D = (v.glslType == "sampler1D");
                        v.is3D = (v.glslType == "sampler3D" || v.glslType == "isampler3D" ||
                                  v.glslType == "usampler3D");
                        if (v.glslType == "isampler2D" || v.glslType == "isampler3D")
                            v.sampleType = "int";
                        else if (v.glslType == "usampler2D" || v.glslType == "usampler3D")
                            v.sampleType = "uint";
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
                    // mảng const: `vec2[]` → elem `vec2`.
                    // P-Sodium: size trên TYPE `float[4] x` (chunk_material ALPHA_CUTOFF)
                    // hợp lệ GLSL — tách size, check khớp số phần tử init (real GL cũng lỗi
                    // nếu lệch).
                    bool cIsArr = false;
                    int cArrSize = 0;
                    if (ctype.size() > 2 && ctype.substr(ctype.size() - 2) == "[]") {
                        cIsArr = true;
                        ctype = ctype.substr(0, ctype.size() - 2);
                    } else {
                        size_t lb = ctype.find('[');
                        if (lb != std::string::npos) {
                            size_t rb = ctype.find(']', lb);
                            if (rb == std::string::npos) return fail("kiểu mảng const sai: " + t);
                            std::string ns = Trim(ctype.substr(lb + 1, rb - lb - 1));
                            if (ns.empty()) {
                                cIsArr = true;
                                ctype = Trim(ctype.substr(0, lb));
                            } else {
                                bool allD = true;
                                for (char c : ns) if (!isdigit((unsigned char)c)) allD = false;
                                if (!allD || (rb + 1) != ctype.size())
                                    return fail("kiểu mảng const sai (chỉ `T[]`/`T[N]` đơn): " + t);
                                cIsArr = true;
                                cArrSize = atoi(ns.c_str());
                                if (cArrSize <= 0) return fail("kích thước mảng const phải >0: " + t);
                                ctype = Trim(ctype.substr(0, lb));
                            }
                        }
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
                        // Size trên DECLARATOR `const float a[12] = ...` (MakeUp
                        // fxaa.glsl) — GLSL hợp lệ; trước đây rơi vào nhánh
                        // `carr != 0` → fail oan. Gộp về cùng đường với `T[N] a`.
                        bool dIsArr = cIsArr;
                        int dArrSize = cArrSize;
                        if (carr != 0) {
                            if (dIsArr) return fail("mảng const 2 chiều ngoài subset: " + t);
                            dIsArr = true;
                            dArrSize = carr;
                        }
                        if (dIsArr) {
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
                            // Size ghi trên type (`float[4]`) phải khớp số phần tử init.
                            if (dArrSize > 0 && (int)elems.size() != dArrSize)
                                return fail("số phần tử init lệch size khai báo: " + t);
                            std::string joined;
                            for (size_t k = 0; k < elems.size(); ++k)
                                joined += (k ? ", " : "") + elems[k];
                            rest += "constant " + cm + " " + cnm + "[" + std::to_string(elems.size()) +
                                    "] = {" + joined + "};\n";
                        } else {
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
    // P-Tess T1: TCS bắt buộc có layout(vertices=N) (kiểm tra sau scan để bắt cả
    // shader không có decl in/out nào — trong scan chỉ check khi gặp decl).
    if (R.isTessControl && R.tessVertices < 1)
        return fail("thiếu layout(vertices=N) out (TCS bắt buộc)");
    // Kiểm tra ma trận làm attribute (Metal attribute tối đa float4)
    // Vanilla Blaze3D dùng `in vec3 Position;` KHÔNG layout → cho phép, linker gán
    // qua glBindAttribLocation/GetAttribLocation (đúng GL, trước đây fail oan).
    if (R.isVertex) {
        for (auto& a : ins) {
            if (a.mslType.find("float") == 0 && a.mslType.find("x") != std::string::npos)
                return fail("attribute ma trận không hỗ trợ (Metal attribute ≤ float4): " + a.name);
        }
    }
    // Varying matN (VS out / FS in — MakeUp `out mat4 modeli_times_projectioni;`):
    // Metal user(locn) chỉ nhận scalar/vector, matrix nằm trong TGLMT_VOut/FIn →
    // "field of illegal type 'float4x4'" / "invalid return type 'TGLMT_VOut'".
    // Tách thành các cột floatR (name_c0..cN-1); body dùng local CÙNG TÊN (FS
    // ghép cột _in ở đầu entry; VS copy local → _out cột trước kZConv/return).
    // Attribute ma trận đã fail sớm ở trên (:5172) — không áp cho ins của VS.
    std::string matVaryOutDecl, matVaryOutCopy, matVaryInDecl;
    {
        bool fragStage = !R.isVertex && !R.isCompute && !R.isTessControl;
        auto expandMats = [&](std::vector<GLSLVar>& vars, bool isOut) {
            std::vector<GLSLVar> nv;
            for (auto& v : vars) {
                int rows = 0, cols = 0;
                if (sscanf(v.mslType.c_str(), "float%dx%d", &rows, &cols) == 2 &&
                    rows >= 2 && rows <= 4 && cols >= 2 && cols <= 4) {
                    std::string colT = "float" + std::to_string(rows);
                    for (int i = 0; i < cols; ++i) {
                        GLSLVar c = v;
                        c.name = v.name + "_c" + std::to_string(i);
                        c.mslType = colT;
                        if (v.location >= 0 && v.location < 900) c.location = v.location + i;
                        nv.push_back(c);
                    }
                    if (isOut) {
                        matVaryOutDecl += v.mslType + " " + v.name + " = {};\n";
                        for (int i = 0; i < cols; ++i)
                            matVaryOutCopy += "_out." + v.name + "_c" + std::to_string(i) +
                                              " = " + v.name + "[" + std::to_string(i) + "]; ";
                    } else {
                        matVaryInDecl += v.mslType + " " + v.name + " = " + v.mslType + "(";
                        for (int i = 0; i < cols; ++i) {
                            if (i) matVaryInDecl += ", ";
                            matVaryInDecl += "_in." + v.name + "_c" + std::to_string(i);
                        }
                        matVaryInDecl += ");\n";
                    }
                } else {
                    nv.push_back(v);
                }
            }
            vars.swap(nv);
        };
        if (R.isVertex) expandMats(outs, true);
        else if (fragStage) expandMats(ins, false);
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
    if (!R.isVertex && !R.isCompute && !R.isTessControl) { // P6f/T1: compute/tess không có out kiểu fragment
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
            // 0 out hợp lệ: chỉ discard / chỉ ghi depth (Complementary
            // gbuffers_clouds.fsh: `void main() { discard; }`). Metal cho
            // `fragment void` hoặc struct chỉ depth.
            legacyFrag = hasWord("gl_FragColor") || hasWord("gl_FragData");
        } else if (outs.size() > 8) {
            return fail("fragment cần 1..8 out (hiện có " + std::to_string(outs.size()) + ")");
        }
        // Fragment out: MRT vec4 thường gặp, nhưng GL cho phép scalar/vecN khớp
        // attachment 1-3 kênh (Iris centerDepth: `out float` + R32F).
        for (auto& o : outs) {
            static const char* kFragOk[] = {"float","float2","float3","float4",
                                            "int","int2","int3","int4",
                                            "uint","uint2","uint3","uint4"};
            bool okT = false;
            for (auto* t : kFragOk) if (o.mslType == t) { okT = true; break; }
            if (!okT) return fail("fragment out kiểu không hỗ trợ (" + o.name + ": " + o.mslType + ")");
        }
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
                if (u.binding < 0 && b.binding >= 0) u.binding = b.binding;
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

    // Varying tên trùng builtin member của TGLMT_VOut? → KHÔNG đổi varying; thay vào
    // đó builtin member đã tên `tglmt_position`/`tglmt_ptSize` (xem emit struct).
    // (shadow.vsh: `out vec4 position;` từng đụng `float4 position [[position]]`.)

    // Uniform int-family (cho CoerceIntInitToFloat: `float2 x = tglmt_u.<field>`).
    std::set<std::string> intUniforms;
    for (auto& u : uniforms)
        if (u.glslType == "int" || u.glslType == "ivec2" || u.glslType == "ivec3" ||
            u.glslType == "ivec4" || u.glslType == "uint" || u.glslType == "uvec2" ||
            u.glslType == "uvec3" || u.glslType == "uvec4")
            intUniforms.insert(u.name);

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
        // Vị trí khai báo local `TYPE name` đầu tiên trong text (-1 nếu không có).
        // Dùng cho: (a) directUses helper — local trùng tên global (GLSL shadowing)
        // nghĩa là helper đọc LOCAL, không thread global; (b) step 7 — local trong
        // main trùng tên hoisted global (MSL hoist cùng scope → redefinition).
        // topLevel=true: chỉ nhận decl cùng scope main (dep 0) — local trong block
        // lồng nhau là shadow HỢP LỆ ở MSL, không được rename (END gbuffers_water
        // `float2 wind` nested; rename sẽ phá call-arg ngoài block).
        // Lưu ý: text ở phase này còn spelling GLSL (vec2...); phải check
        // word-boundary cuối tên (NdotU vs NdotUM — từng hồi quy NdotU/nightFactor).
        // Kiểu user struct chưa nhận diện (metal step sẽ bắt nếu sót).
        auto findLocalDeclPos = [&](const std::string& b, const std::string& g,
                                    bool topLevel = false) -> size_t {
            size_t p = 0;
            while ((p = b.find(g, p)) != std::string::npos) {
                size_t pe = p + g.size();
                bool endOk = pe >= b.size() || !IsIdentChar(b[pe], false);
                if (!endOk) { p = pe; continue; }
                size_t q = p;
                while (q > 0 && isspace((unsigned char)b[q - 1])) --q;
                size_t we = q;
                while (we > 0 && IsIdentChar(b[we - 1], false)) --we;
                std::string prev = b.substr(we, q - we);
                if (prev == "float" || prev == "vec2" || prev == "vec3" || prev == "vec4" ||
                    prev == "float2" || prev == "float3" || prev == "float4" ||
                    prev == "int" || prev == "ivec2" || prev == "ivec3" || prev == "ivec4" ||
                    prev == "uint" || prev == "uvec2" || prev == "uvec3" || prev == "uvec4" ||
                    prev == "bool" || prev == "bvec2" || prev == "bvec3" || prev == "bvec4") {
                    if (!topLevel) return we;
                    int dep = 0;
                    for (size_t k = 0; k < we; ++k) {
                        if (b[k] == '{') ++dep;
                        else if (b[k] == '}') --dep;
                    }
                    if (dep == 0) return we;
                }
                p = pe;
            }
            return std::string::npos;
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
            // Local cùng tên global trong body (GLSL shadowing) → helper đọc LOCAL,
            // KHÔNG được thread global (shadow.vsh: `vec2 lmCoord;` global + helper
            // `vec2 GetLightMapCoordinates() { vec2 lmCoord = ...; }` từng thành
            // `thread float2& lmCoord` + local → MSL redefinition).
            auto declaresLocal = [&](const std::string& b, const std::string& g) {
                return findLocalDeclPos(b, g) != std::string::npos;
            };
            for (auto& g : hoisted)
                if (!isParam(g.name) && !declaresLocal(hb, g.name) && hasWordIn(hb, g.name)) {
                    auto& v = directUses[h.name];
                    if (std::find(v.begin(), v.end(), g.name) == v.end()) v.push_back(g.name);
                }
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
        // Collision hoisted-global vs local CÙNG SCOPE main (top-level): MSL hoist
        // global thành local đầu main → redefinition (composite.fsh `ivec2 texelCoord`
        // từ common.glsl vs local cùng tên đầu main). Tính TRƯỚC step 5/6 để:
        // header helper + call-arg thread + decl dùng cùng tên mới. Local trong
        // block lồng nhau bỏ qua (shadow hợp lệ ở MSL).
        std::map<std::string, std::string> gRen; // old → new
        for (auto& g : hoisted) {
            if (findLocalDeclPos(body, g.name, true) == std::string::npos) continue;
            std::string nn = g.name + "_tglmt_g";
            for (int lim = 0; lim < 8 && (hasWordIn(body, nn) || hasWordIn(helpersOnly, nn)); ++lim)
                nn += "_";
            gRen[g.name] = nn;
        }
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
            // P-Sodium: helper không param gốc (`f()`) + thread-refs → `(, thread...)`
            // là MSL lỗi (`expected expression`). Bỏ ", " đầu khi params rỗng.
            if (!add.empty()) {
                if (newParams.empty() && add.rfind(", ", 0) == 0) newParams = add.substr(2);
                else newParams += add;
            }
            helpersOnly.replace(h.lp + 1, h.rp - h.lp - 1, newParams);
        }
        // 6. Viết lại call sites trong body + helper bodies: `H(args)` → `H(args, g...)`.
        // Bỏ qua header definitions (đã chứa thread params).
        auto appendCallArgs = [&](std::string& text, size_t skipStart, size_t skipEnd,
                                  const std::string& hname, const std::vector<std::string>& gs,
                                  bool applyRen) {
            if (gs.empty()) return;
            std::vector<std::string> sg = gs;
            std::sort(sg.begin(), sg.end());
            std::string add;
            // applyRen: call trong main trỏ hoisted global đã đổi tên (cùng scope
            // redecl); call trong helper giữ tên cũ (helper nhận global qua
            // thread-param cùng tên, tên mới không tồn tại trong scope helper).
            for (auto& gn : sg) {
                auto it = applyRen ? gRen.find(gn) : gRen.end();
                add += ", " + (it != gRen.end() ? it->second : gn);
            }
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
                // Header định nghĩa (`f(...) {`), gồm cả overload thứ 2+ cùng tên:
                // không phải call site — chèn args vào đây sẽ phá params
                // (Complementary: GetLinearDepth/1 và /3 cùng dùng farMinusNear).
                size_t nb = c + 1;
                while (nb < text.size() && isspace((unsigned char)text[nb])) ++nb;
                if (nb < text.size() && text[nb] == '{') {
                    p = c + 1;
                    continue;
                }
                // P-Sodium: call rỗng args `f()` + thread-refs → `f(, g...)` lỗi MSL.
                // Bỏ ", " đầu khi giữa ngoặc chỉ có spaces.
                {
                    std::string inner = Trim(text.substr(q + 1, c - q - 1));
                    std::string ins = (inner.empty() && add.rfind(", ", 0) == 0) ? add.substr(2) : add;
                    text.insert(c, ins);
                    p = c + ins.size() + 1;
                }
            }
        };
        std::set<std::string> callSiteDone;
        for (auto& h : helpers) {
            auto& gs = req[h.name];
            if (gs.empty()) continue;
            // overload cùng tên dùng chung req (union) → rewrite 1 lần/tên, tránh
            // chèn args lặp vào cùng call site (Complementary GetLinearDepth/1+/3).
            if (!callSiteDone.insert(h.name).second) continue;
            // calls trong body main
            appendCallArgs(body, 0, 0, h.name, gs, true);
            // calls trong helpers khác (kể cả chính nó — đệ quy truyền tiếp)
            // NOTE: helpersOnly đã bị đổi offset ở bước 5; parse lại span body cho an toàn:
        }
        // Parse lại spans sau bước 5 rồi rewrite calls trong helpersOnly.
        {
            std::set<std::string> callSiteDone2;
            // dựng lại danh sách body spans bằng cách tìm lại từng helper
            for (auto& h : helpers) {
                if (!callSiteDone2.insert(h.name).second) continue;
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
                appendCallArgs(helpersOnly, defp, depr, h.name, gs, false);
            }
        }
        // 7. Phát locals vào đầu main body.
        {
            // gRen (tính ở bước 4.5): hoisted global trùng tên local CÙNG SCOPE
            // main đã đổi tên. Rename use prologue [0, decl) của tên cũ trong main
            // (vùng đó ở GLSL vẫn trỏ global); call-arg thread trong main đã dùng
            // tên mới từ appendCallArgs; decl/init phát với tên mới.
            for (auto& [old, nn] : gRen) {
                size_t dp = findLocalDeclPos(body, old, true);
                if (dp == std::string::npos) continue;
                std::vector<std::pair<size_t, size_t>> occ;
                size_t p = 0;
                while ((p = body.find(old, p)) != std::string::npos && p < dp) {
                    size_t e = p + old.size();
                    bool l = p > 0 && IsIdentChar(body[p - 1], false);
                    bool dot = p > 0 && body[p - 1] == '.';
                    bool r = e < body.size() && IsIdentChar(body[e], false);
                    if (!l && !r && !dot) occ.push_back({p, e});
                    p = e;
                }
                for (auto it = occ.rbegin(); it != occ.rend(); ++it)
                    body.replace(it->first, it->second - it->first, nn);
            }
            std::string decls;
            for (auto& g : hoisted) {
                std::string init = RewriteTypeNames(RewriteArrayCtors(g.init));
                for (auto& [a, b] : gRen) {
                    std::vector<std::pair<size_t, size_t>> occ;
                    size_t p = 0;
                    while ((p = init.find(a, p)) != std::string::npos) {
                        size_t e = p + a.size();
                        bool l = p > 0 && IsIdentChar(init[p - 1], false);
                        bool dot = p > 0 && init[p - 1] == '.';
                        bool r = e < init.size() && IsIdentChar(init[e], false);
                        if (!l && !r && !dot) occ.push_back({p, e});
                        p = e;
                    }
                    for (auto it = occ.rbegin(); it != occ.rend(); ++it)
                        init.replace(it->first, it->second - it->first, b);
                }
                auto it = gRen.find(g.name);
                std::string gname = it != gRen.end() ? it->second : g.name;
                std::string d = g.mslType + " " + gname;
                if (g.arraySize > 0) d += "[" + std::to_string(g.arraySize) + "]";
                if (!init.empty()) d += " = " + init;
                else d += " = {}"; // global GLSL không init = zero (GL spec)
                decls += d + ";\n";
            }
            // Update tên để bước sau (mat4Names cho `float3x3(<mat4 var>)`) dùng
            // tên mới (body đã viết lại các use).
            for (auto& g : hoisted) {
                auto it = gRen.find(g.name);
                if (it != gRen.end()) g.name = it->second;
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
                e.elem = SamplerElemOf(k);
                helperSamp[h.name].emplace_back(nm, e);
            }
        }
    }
    // Sampling calls trong helpers (rest): table = global + sampler params
    // của MỌI helper (first-wins khi trùng tên — đủ cho vanilla).
    {
        SampTable tab;
        for (auto& s : R.samplers) {
            SampEntry e = EntryOfSampler(s);
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
    const std::string kZConv = " _out.tglmt_position.z = _out.tglmt_position.z * 0.5 + _out.tglmt_position.w * 0.5;";
    bool isMRT = !R.isVertex && outs.size() > 1;
    // P6d: gl_* builtin cần param stage riêng (đã kiểm chứng metal thật).
    // Local (không thêm vào public struct) — detect trước, emit param ở assembly.
    bool useStencilRef = false, useSampleID = false, useSamplePos = false;
    bool useBaseInstance = false, useFragDepth = false, useSampleMask = false;
    // P6f compute thread-position params (chỉ emit param khi body dùng — probe thật).
    bool useGlobalID = false, useLocalID = false, useWorkGroupID = false;
    bool useNumWG = false, useLocalIndex = false, useWorkGroupSize = false;
    // P-Tess T1: TCS dùng factor buffer / gl_in (emit param/buffer theo mức dùng).
    // tglmt_tcs_tid + tglmt_patchID luôn emit (rẻ, tránh flag bug).
    bool useTcsFactors = false, useTcsGlIn = false;
    bool useTcsTid = true;
    if (R.isVertex) {
        for (auto& a : ins) mpVary.emplace_back(a.name, "_in." + a.name);
        for (auto& v : outs) mpVary.emplace_back(v.name, "_out." + v.name);
        mpVary.emplace_back("gl_Position", "_out.tglmt_position");
        mpVary.emplace_back("gl_PointSize", "_out.tglmt_ptSize");
        if (body.find("gl_PointSize") != std::string::npos) R.usesPointSize = true;
        // screenquad (lightmap/blit): đỉnh suy từ index, không cần attribute.
        // GLSL int → MSL uint (bitwise tương đương).
        mpVary.emplace_back("gl_VertexID", "tglmt_vertexID");
        if (body.find("gl_VertexID") != std::string::npos ||
            rest.find("gl_VertexID") != std::string::npos)
            R.usesVertexID = true;
        mpVary.emplace_back("gl_InstanceID", "tglmt_instanceID");
        if (body.find("gl_InstanceID") != std::string::npos ||
            rest.find("gl_InstanceID") != std::string::npos)
            R.usesInstanceID = true;
        // gl_BaseInstance: MSL [[base_instance]] (đã kiểm chứng vertex compile thật).
        if (body.find("gl_BaseInstance") != std::string::npos) {
            useBaseInstance = true;
            mpVary.emplace_back("gl_BaseInstance", "tglmt_baseInstance");
        }
        // gl_DrawID: Metal Intel không có [[draw_index]] (probe: unknown attr) →
        // fail trung thực tại convert thay vì lỗi MSL mơ hồ ở link.
        if (body.find("gl_DrawID") != std::string::npos)
            return fail("gl_DrawID ngoài subset (Metal không có [[draw_index]])");
    } else if (R.isCompute) {
        // P6f compute: gl_*ID → kernel thread-position params (probe Metal thật OK).
        // localSize bắt ở preprocessor (dòng layout local_size bị bỏ).
        if (body.find("gl_GlobalInvocationID") != std::string::npos) {
            useGlobalID = true;
            mpVary.emplace_back("gl_GlobalInvocationID", "tglmt_globalID");
        }
        if (body.find("gl_LocalInvocationID") != std::string::npos) {
            useLocalID = true;
            mpVary.emplace_back("gl_LocalInvocationID", "tglmt_localID");
        }
        if (body.find("gl_WorkGroupID") != std::string::npos) {
            useWorkGroupID = true;
            mpVary.emplace_back("gl_WorkGroupID", "tglmt_workGroupID");
        }
        if (body.find("gl_NumWorkGroups") != std::string::npos) {
            useNumWG = true;
            mpVary.emplace_back("gl_NumWorkGroups", "tglmt_numWorkGroups");
        }
        // [[thread_index_in_threadgroup]] = local linear index (gl_LocalInvocationIndex).
        if (body.find("gl_LocalInvocationIndex") != std::string::npos) {
            useLocalIndex = true;
            mpVary.emplace_back("gl_LocalInvocationIndex", "tglmt_localIndex");
        }
        // gl_WorkGroupSize: không có builtin MSL → hằng uint3 emit trong kernel.
        if (body.find("gl_WorkGroupSize") != std::string::npos) {
            useWorkGroupSize = true;
            mpVary.emplace_back("gl_WorkGroupSize", "tglmt_workGroupSize");
        }
    } else if (R.isTessControl) {
        // P-Tess T1: TCS builtins. gl_InvocationID/gl_PrimitiveID/TessLevel/gl_in/
        // mảng per-vertex/patch do RewriteTessControl xử lý (cần match ngoặc) —
        // ở đây chỉ detect flags dùng cho emission + fail builtins sai-stage.
        // (mpVary KHÔNG map tên tess: tránh double-rewrite với custom rewrite.)
        auto hasWord = [&](const char* w) {
            size_t n = strlen(w), p = 0;
            while ((p = body.find(w, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(body[p - 1], false);
                size_t e = p + n;
                bool r = e < body.size() && IsIdentChar(body[e], false);
                if (!l && !r) return true;
                p = e;
            }
            return false;
        };
        if (hasWord("gl_TessLevelOuter") || hasWord("gl_TessLevelInner")) useTcsFactors = true;
        if (hasWord("gl_in")) useTcsGlIn = true;
        // (useTcsTid luôn true — tid param + patchID local emit mọi kernel TCS.)
        // Builtin sai stage trong TCS → fail rõ ngay convert (thay vì MSL mơ hồ ở link).
        // gl_Position/gl_PointSize: TCS không ghi (dùng out var). gl_TessCoord: TES-only.
        // gl_PrimitiveID HỢP LỆ (= patch ID) nên không có trong list. gl_out không tồn tại.
        // Lưu ý: gl_in[i].gl_Position là member access (rewrite riêng) — check dưới
        // bỏ qua occurrence đứng sau '.' (xem RewriteTessControl; ở đây check thô,
        // member access sẽ được rewrite trước khi... — KHÔNG: check này chạy TRƯỚC
        // rewrite, nên `gl_in[0].gl_Position` sẽ match "gl_Position"! Phải loại trừ
        // occurrence có '.' trước nó (bỏ spaces). Xử lý trong loop fail bên dưới.
        const char* wrongStage[] = {
            "gl_Position", "gl_PointSize", "gl_TessCoord", "gl_FragCoord",
            "gl_FrontFacing", "gl_PointCoord", "gl_VertexID", "gl_InstanceID",
            "gl_DrawID", "gl_BaseInstance", "gl_BaseVertex",
            "gl_GlobalInvocationID", "gl_LocalInvocationID", "gl_WorkGroupID",
            "gl_NumWorkGroups", "gl_LocalInvocationIndex", "gl_WorkGroupSize",
            "gl_HelperInvocation", "gl_SampleID", "gl_SamplePosition",
            "gl_SampleMask", "gl_FragDepth", "gl_StencilRef", "gl_out", nullptr
        };
        for (const char** ww = wrongStage; *ww; ++ww) {
            size_t n = strlen(*ww), p = 0;
            while ((p = body.find(*ww, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(body[p - 1], false);
                size_t e = p + n;
                bool r = e < body.size() && IsIdentChar(body[e], false);
                if (!l && !r) {
                    // member access `x.gl_XXX` (vd gl_in[i].gl_Position): bỏ qua ở đây,
                    // RewriteTessControl validate member sau (chỉ Position/PointSize).
                    size_t q = p;
                    while (q > 0 && isspace((unsigned char)body[q - 1])) --q;
                    if (q > 0 && body[q - 1] == '.') { p = e; continue; }
                    return fail(std::string(*ww) + " không dùng được trong TCS");
                }
                p = e;
            }
        }
    } else {
        for (auto& v : ins) mpVary.emplace_back(v.name, "_in." + v.name);
        mpVary.emplace_back("gl_PointCoord", "tglmt_pointCoord");
        if (body.find("gl_PointCoord") != std::string::npos ||
            rest.find("gl_PointCoord") != std::string::npos)
            R.usesPointCoord = true;
        mpVary.emplace_back("gl_FragCoord", "tglmt_fragCoord");
        if (body.find("gl_FragCoord") != std::string::npos ||
            rest.find("gl_FragCoord") != std::string::npos)
            R.usesFragCoord = true;
        // gl_FrontFacing (entity.fsh nhánh PER_FACE_LIGHTING): Metal không có
        // builtin tương ứng trong body — phải là param [[front_facing]].
        // Không map là `use of undeclared identifier` trên máy (crash 26.1.2
        // 2026-09-28: 8 pipeline entity_*). Chỉ fragment mới có front-facing.
        mpVary.emplace_back("gl_FrontFacing", "tglmt_frontFacing");
        if (body.find("gl_FrontFacing") != std::string::npos ||
            rest.find("gl_FrontFacing") != std::string::npos)
            R.usesFrontFacing = true;
        // P6d fragment input builtins (param stage, đã kiểm chứng metal thật).
        if (body.find("gl_StencilRef") != std::string::npos) {
            useStencilRef = true;
            mpVary.emplace_back("gl_StencilRef", "tglmt_stencilRef");
        }
        if (body.find("gl_SampleID") != std::string::npos) {
            useSampleID = true;
            mpVary.emplace_back("gl_SampleID", "tglmt_sampleID");
        }
        if (body.find("gl_SamplePosition") != std::string::npos) {
            useSamplePos = true;
            mpVary.emplace_back("gl_SamplePosition", "tglmt_samplePosition");
        }
        // P6d fragment OUTPUT builtins (cần struct return với [[depth]/[sample_mask]]).
        if (body.find("gl_FragDepth") != std::string::npos) {
            useFragDepth = true;
            mpVary.emplace_back("gl_FragDepth", "_out.depth"); // ghi thẳng vào struct out
        }
        if (body.find("gl_SampleMask") != std::string::npos) useSampleMask = true;
        // gl_HelperInvocation: Metal Intel không có [[helper_invocation]] → fail rõ.
        if (body.find("gl_HelperInvocation") != std::string::npos)
            return fail("gl_HelperInvocation ngoài subset (Metal không có [[helper_invocation]])");
        if (isMRT) {
            for (auto& o : outs) mpVary.emplace_back(o.name, "_out.mrt" + std::to_string(o.location));
        } else if (legacyFrag) {
            // end_portal compat: gl_FragColor / gl_FragData[0] → out duy nhất
            mpVary.emplace_back("gl_FragColor", "tglmt_fragColor");
        } else if (!outs.empty()) {
            // out color: tên biến out → tglmt_fragColor (return ở cuối)
            mpVary.emplace_back(outs[0].name, "tglmt_fragColor");
        }
    }
    for (auto& u : uniforms) {
        if (u.isSampler) continue;
        mpShared.emplace_back(u.name, "tglmt_u." + u.name);
    }
    // Entry builtins đọc trong HELPER: helper rewrite chỉ dùng mpShared, nên cần
    // map tương tự mpVary (ThreadHelpersMSL thread param cùng tên — đọc-only).
    if (R.isVertex) {
        if (R.usesVertexID) mpShared.emplace_back("gl_VertexID", "tglmt_vertexID");
        if (R.usesInstanceID) mpShared.emplace_back("gl_InstanceID", "tglmt_instanceID");
    } else if (!R.isCompute && !R.isTessControl && !R.isTessEval) {
        if (R.usesFragCoord) mpShared.emplace_back("gl_FragCoord", "tglmt_fragCoord");
        if (R.usesPointCoord) mpShared.emplace_back("gl_PointCoord", "tglmt_pointCoord");
        if (R.usesFrontFacing) mpShared.emplace_back("gl_FrontFacing", "tglmt_frontFacing");
    }
    // UBO/SSBO members → param. UBO: `ubo_Block.member` (constant, buffer(17+ubi)).
    // SSBO struct: `ssbo_Block.member` (device); SSBO unsized-array single-member:
    // flatten device T* (param tên member). Metal buffer index max = 30.
    // P4: vertex/graphics cũng nhận SSBO — UBO lẫn SSBO dùng CHUNG dải 17+30,
    // gán tuần tự theo thứ tự khai báo (không chồng slot nhau). Compute/tess giữ
    // nguyên: SSBO 0+sbi (0..15 trống cho kernel), UBO 17+ubi.
    struct SSBOFlat { std::string type, param; int slot; };
    std::vector<SSBOFlat> ssboFlat;
    std::vector<int> ssboSlot(blocks.size(), -1); // storage-struct slot; -1 = UBO/flat
    std::vector<int> blkEmitSlot(blocks.size(), -1); // slot [[buffer]] emit (UBO + SSBO)
    int uboCnt = 0, ssboCnt = 0;
    bool kernelLike = R.isCompute || R.isTessControl;
    int gfxNext = 17; // graphics: con trỏ slot tuần tự cho MỌI block (UBO + SSBO)
    for (size_t bi = 0; bi < blocks.size(); ++bi) {
        auto& b = blocks[bi];
        if (b.isStorage) {
            int slot = kernelLike ? ssboCnt : gfxNext++;
            if (kernelLike && slot > 15) return fail("SSBO vượt slot compute (0..15, max 30)");
            if (!kernelLike && slot > 30) return fail("SSBO graphics vượt slot buffer(30)");
            bool hasUnsized = false;
            for (auto& m : b.members) if (m.arraySize == -1) hasUnsized = true;
            if (hasUnsized) {
                // Subset: block 1 member mảng unsized, KHÔNG instance (pattern compute).
                if (b.members.size() != 1) return fail("SSBO nhiều member + mảng unsized ngoài subset");
                std::string mn = b.members[0].name;
                if (mn.find('.') != std::string::npos) return fail("SSBO unsized-array có instance ngoài subset");
                ssboFlat.push_back({b.members[0].mslType, mn, slot});
                R.blockSlots.push_back({b.binding, slot, true, b.name});
                ++ssboCnt;
                continue; // member name = param name → không cần mpShared (identity)
            }
            ssboSlot[bi] = slot;
            R.blockSlots.push_back({b.binding, slot, true, b.name});
            ++ssboCnt;
            std::string inst;
            for (auto& m : b.members) {
                size_t dot = m.name.find('.');
                if (dot != std::string::npos && inst.empty()) inst = m.name.substr(0, dot);
            }
            if (!inst.empty()) mpShared.emplace_back(inst, "ssbo_" + b.name);
            for (auto& m : b.members) {
                size_t dot = m.name.find('.');
                std::string access = (dot == std::string::npos) ? m.name : m.name.substr(dot + 1);
                mpShared.emplace_back(m.name, "ssbo_" + b.name + "." + access);
            }
            continue;
        }
        ++uboCnt;
        int uslot = kernelLike ? (17 + uboCnt - 1) : gfxNext++;
        if (!kernelLike && uslot > 30) return fail("UBO graphics vượt slot buffer(30)");
        blkEmitSlot[bi] = uslot;
        R.blockSlots.push_back({b.binding, uslot, false, b.name});
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
    // P-Sodium: bvecN → boolN (MSL không có bvec; `mix(f,f,boolN)` + `boolN(uintN)` hợp lệ).
    // Chỉ expression-level (mpShared): KHAI BÁO bvec varying/uniform vẫn fail trung thực
    // ở MSLType (không bịa buffer layout/align cho bool vectors — chưa kiểm chứng Metal).
    mpShared.emplace_back("bvec2", "bool2");
    mpShared.emplace_back("bvec3", "bool3");
    mpShared.emplace_back("bvec4", "bool4");
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
    // `inversesqrt`: Metal stdlib tên `rsqrt` (metal thật: undeclared identifier).
    mpShared.emplace_back("inversesqrt", "rsqrt");
    // `radians`/`degrees`: Metal stdlib KHÔNG có (đã quan sát bằng metal thật:
    // `use of undeclared identifier 'radians'`). Hạ về helper nhân PI/180.
    // Lưu ý matrix.glsl có param tên `radians` (`mat2_rotate_z(float radians)`)
    // — map cả param lẫn call sang `tglmt_radians` nên vẫn nhất quán
    // (param `float tglmt_radians`, body `cos(tglmt_radians)`, call
    // `tglmt_radians(...)`).
    mpShared.emplace_back("radians", "tglmt_radians");
    mpShared.emplace_back("degrees", "tglmt_degrees");
    // `mod`: Metal stdlib KHÔNG có (chỉ fmod — khác ngữ nghĩa dấu theo số bị
    // chia). GLSL mod(x,y) = x - y*floor(x/y) → helper prelude overload.
    mpShared.emplace_back("mod", "tglmt_mod");
    // ---- Bitcast GLSL → MSL as_type<> (đã kiểm chứng metal thật compile) ----
    // floatBitsToInt(x) → as_type<int>(x): RewriteIdents thay identifier, ngoặc
    // (x) giữ nguyên → as_type<int>(x). Đúng.
    mpShared.emplace_back("floatBitsToInt", "as_type<int>");
    mpShared.emplace_back("floatBitsToUint", "as_type<uint>");
    mpShared.emplace_back("intBitsToFloat", "as_type<float>");
    mpShared.emplace_back("uintBitsToFloat", "as_type<float>");
    // ---- pack/unpack GLSL → MSL pack_float_to_* / unpack_*_to_float ----
    // (đã kiểm chứng: pack_float_to_unorm2x16/snorm2x16/unorm4x8 + unpack
    //  _to_float compile thật; vecN→floatN nhờ type map cùng pass RewriteIdents)
    mpShared.emplace_back("packUnorm2x16", "pack_float_to_unorm2x16");
    mpShared.emplace_back("unpackUnorm2x16", "unpack_unorm2x16_to_float");
    mpShared.emplace_back("packSnorm2x16", "pack_float_to_snorm2x16");
    mpShared.emplace_back("unpackSnorm2x16", "unpack_snorm2x16_to_float");
    mpShared.emplace_back("packUnorm4x8", "pack_float_to_unorm4x8");
    mpShared.emplace_back("unpackUnorm4x8", "unpack_unorm4x8_to_float");
    // ---- bitfield GLSL → MSL (MSL extract/insert_bits nhận int arg thật) ----
    mpShared.emplace_back("bitfieldReverse", "reverse_bits");
    mpShared.emplace_back("bitfieldExtract", "extract_bits");
    mpShared.emplace_back("bitfieldInsert", "insert_bits");
    // ---- findLSB/findMSB/packHalf2x16 → helper prelude (cần bọc arg) ----
    mpShared.emplace_back("findLSB", "tglmt_findLSB");
    mpShared.emplace_back("findMSB", "tglmt_findMSB");
    mpShared.emplace_back("packHalf2x16", "tglmt_packHalf2x16");
    mpShared.emplace_back("unpackHalf2x16", "tglmt_unpackHalf2x16");
    // ---- Subgroup/invocation GLSL → MSL simdgroup (đã kiểm chứng metal thật,
    //  chạy được cả fragment lẫn kernel). RewriteIdents khớp token đầy đủ nên
    //  `subgroupBroadcast` không khớp nhầm `subgroupBroadcastFirst` (thứ tự无关).
    mpShared.emplace_back("subgroupAdd", "simd_sum");
    mpShared.emplace_back("subgroupMax", "simd_max");
    mpShared.emplace_back("subgroupMin", "simd_min");
    mpShared.emplace_back("subgroupAnd", "simd_and");
    mpShared.emplace_back("subgroupOr", "simd_or");
    mpShared.emplace_back("subgroupXor", "simd_xor");
    mpShared.emplace_back("subgroupProduct", "simd_product");
    mpShared.emplace_back("subgroupBroadcast", "simd_broadcast");
    mpShared.emplace_back("subgroupShuffle", "simd_shuffle");
    mpShared.emplace_back("subgroupShuffleXor", "simd_shuffle_xor");
    mpShared.emplace_back("subgroupShuffleUp", "simd_shuffle_up");
    mpShared.emplace_back("subgroupShuffleDown", "simd_shuffle_down");
    mpShared.emplace_back("subgroupBroadcastFirst", "simd_broadcast_first");
    mpShared.emplace_back("subgroupInclusiveAdd", "simd_prefix_inclusive_sum");
    mpShared.emplace_back("subgroupExclusiveAdd", "simd_prefix_exclusive_sum");
    mpShared.emplace_back("subgroupInclusiveMul", "simd_prefix_inclusive_product");
    // elect(): GLSL chọn đúng 1 lane active; simd_is_first() = lane 0 (đúng khi
    //  cả simdgroup active — trường hợp phổ biến). Ghi chú là xấp xỉ.
    mpShared.emplace_back("subgroupElect", "simd_is_first");
    // ballot/vote trả uvec4/bool → cần bọc qua prelude helper.
    mpShared.emplace_back("subgroupBallot", "tglmt_subgroupBallot");
    mpShared.emplace_back("ballotARB", "tglmt_subgroupBallot");
    mpShared.emplace_back("subgroupAny", "tglmt_subgroupAny");
    mpShared.emplace_back("subgroupAll", "tglmt_subgroupAll");
    mpShared.emplace_back("anyInvocation", "tglmt_subgroupAny");
    mpShared.emplace_back("allInvocation", "tglmt_subgroupAll");
    mpShared.emplace_back("ballotBitCountARB", "tglmt_ballotBitCount");
    mpShared.emplace_back("ballotBitCount", "tglmt_ballotBitCount");
    // mp gộp cho main body = vary + shared (helpers chỉ dùng shared).
    mp.insert(mp.end(), mpVary.begin(), mpVary.end());
    mp.insert(mp.end(), mpShared.begin(), mpShared.end());
    // Sampling calls (texture/texelFetch/textureGrad) trên body. Helpers
    // trong rest được xử lý riêng ở dưới (cần sampler params của từng helper).
    {
        SampTable tab;
        for (auto& s : R.samplers) {
            SampEntry e = EntryOfSampler(s);
            tab[s.name] = e;
        }
        std::string err;
        if (!RewriteSamplingCalls(body, tab, err)) return fail(err);
    }
    // P6g image calls (imageLoad/Store/Size) trên body. Helper dùng image → fail
    // rõ (thread image params phức tạp, hiếm gặp). `rest` còn chứa main nên strip
    // main trước khi check helper.
    {
        ImgTable itab;
        for (auto& im : R.images) {
            char kind = ImageKind(im.glslType);
            if (kind) itab[im.name] = kind;
        }
        std::string err;
        if (!RewriteImageCalls(body, itab, err)) return fail(err);
        // P9 atomic counters: rewrite atomicCounter* → MSL atomics.
        {
            std::vector<std::string> anames;
            for (auto& ac : R.atomicCounters) anames.push_back(ac.name);
            if (!RewriteAtomicCalls(body, anames, err)) return fail(err);
        }
        // P-Tess T1: rewrite TCS body (gl_InvocationID/TessLevel/gl_in/mảng).
        // Chạy trước RewriteIdents; mpVary TCS đã detect flags + fail sai-stage.
        if (R.isTessControl) {
            std::vector<TessVar> tvars;
            for (auto& v : ins) tvars.push_back({v.name, true, v.isPatch, v.arraySize});
            for (auto& v : outs) tvars.push_back({v.name, false, v.isPatch, v.arraySize});
            if (!RewriteTessControl(body, R.tessVertices, tvars, hasGlInBlock, err))
                return fail(err);
        }
        // Strip main khỏi rest → helpersOnly, rồi check image trong helper.
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
                        if (m < helpersOnly.size()) { helpersOnly.erase(p, m - p + 1); break; }
                    }
                }
                p = helpersOnly.find("void", p + 1);
            }
        }
        for (const char* fn : {"imageLoad", "imageStore", "imageSize"}) {
            size_t p = 0;
            while ((p = helpersOnly.find(fn, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(helpersOnly[p - 1], false);
                size_t e = p + strlen(fn);
                bool r = e < helpersOnly.size() && IsIdentChar(helpersOnly[e], false);
                if (!l && !r) return fail("image trong helper function ngoài subset");
                p = e;
            }
        }
        // P-Tess T1: tess builtin/var trong helper → fail rõ (helper không nhận
        // tess params — thread phức tạp; inline vào main hoặc truyền phần tử).
        // Chỉ check builtin (không false-positive param trùng tên); var khai báo
        // check kèm (param trùng tên hiếm — message hướng dẫn rename/inline).
        if (R.isTessControl) {
            auto hasWordH = [&](const std::string& w) {
                size_t p = 0;
                while ((p = helpersOnly.find(w, p)) != std::string::npos) {
                    bool l = p > 0 && IsIdentChar(helpersOnly[p - 1], false);
                    size_t e = p + w.size();
                    bool r = e < helpersOnly.size() && IsIdentChar(helpersOnly[e], false);
                    if (!l && !r) return true;
                    p = e;
                }
                return false;
            };
            for (const char* b : {"gl_InvocationID", "gl_PrimitiveID", "gl_PatchVerticesIn",
                                  "gl_TessLevelOuter", "gl_TessLevelInner", "gl_in", "barrier"}) {
                if (hasWordH(b))
                    return fail(std::string("tess builtin `") + b + "` trong helper ngoài subset T1 (inline vào main)");
            }
            for (auto& v : ins)
                if (hasWordH(v.name))
                    return fail("mảng tess `" + v.name + "` trong helper ngoài subset T1 (truyền phần tử vd f(" + v.name + "[i]))");
            for (auto& v : outs)
                if (hasWordH(v.name))
                    return fail("mảng tess `" + v.name + "` trong helper ngoài subset T1 (truyền phần tử vd f(" + v.name + "[i]))");
        }
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
    // gl_SampleMask[0] → _out.smask (trước RewriteIdents, bỏ `[0]` như gl_FragData).
    // GLSL ghi mask bit 0..N-1 vào gl_SampleMask[0]; MSL [[sample_mask]] uint tương ứng.
    if (!R.isVertex && useSampleMask) {
        std::string nb;
        size_t i = 0;
        while (i < body.size()) {
            size_t p = body.find("gl_SampleMask", i);
            if (p == std::string::npos) { nb += body.substr(i); break; }
            bool l = p > 0 && IsIdentChar(body[p - 1], false);
            size_t b = body.find('[', p + 13);
            size_t e2 = (b == std::string::npos) ? std::string::npos : body.find(']', b);
            if (!l && b != std::string::npos && e2 != std::string::npos &&
                atoi(body.substr(b + 1, e2 - b - 1).c_str()) == 0) {
                nb += body.substr(i, p - i);
                nb += "_out.smask";
                i = e2 + 1;
                continue;
            }
            // gl_SampleMask[N>0] ngoài subset (MSL chỉ có mask output duy nhất).
            if (!l && b != std::string::npos && e2 != std::string::npos)
                return fail("gl_SampleMask[N>0] ngoài subset");
            nb += body.substr(i, p - i + 13);
            i = p + 13;
        }
        body = nb;
    }
    RenameLocalShadowUniforms(body, mp);
    body = RewriteIdents(RewriteArrayCtors(body), mp);
    // P6e subroutine dispatch: sinh hàm `tglmt_sub_<var>(params, idx)` switch trên
    // uniform int (MSL không có function pointer). Call `tglmt_u.var(` (sau RI)
    // → `tglmt_sub_var(args, tglmt_u.var)`. Dispatch append vào rest (sau impls).
    if (!subVars.empty()) {
        for (auto& sv : subVars) {
            // Tìm signature type.
            const SubType* st = nullptr;
            for (auto& t : subTypes) if (t.name == sv.type) { st = &t; break; }
            if (!st) return fail("subroutine type chưa khai báo: " + sv.type);
            // Impls của type (theo thứ tự khai báo = index).
            std::vector<std::string> impls;
            for (auto& im : subImpls) if (im.type == sv.type) impls.push_back(im.name);
            if (impls.empty()) return fail("subroutine không có implementation: " + sv.type);
            // Convert ret + params sang MSL.
            bool okT = false;
            std::string mslRet = MSLType(st->ret, okT);
            if (!okT && st->ret != "void") return fail("subroutine ret ngoài subset: " + st->ret);
            // Parse params `vec4 v, float f` → [(mslType, name)].
            std::vector<std::pair<std::string,std::string>> mparams;
            {
                std::string ps = Trim(st->params);
                if (!ps.empty() && ps != "void") {
                    std::istringstream pss(ps);
                    std::string seg;
                    while (std::getline(pss, seg, ',')) {
                        seg = Trim(seg);
                        if (seg.empty()) continue;
                        size_t sp = seg.find_last_of(" \t*");
                        if (sp == std::string::npos) return fail("subroutine param lỗi: " + seg);
                        std::string pt = Trim(seg.substr(0, sp));
                        std::string pn = Trim(seg.substr(sp + 1));
                        // bỏ mảng param (ngoài subset)
                        if (pn.find('[') != std::string::npos) return fail("subroutine param mảng ngoài subset");
                        bool okP = false;
                        std::string mpt = MSLType(pt, okP);
                        if (!okP) return fail("subroutine param type ngoài subset: " + pt);
                        mparams.emplace_back(mpt, pn);
                    }
                }
            }
            // Sinh dispatch fn.
            std::string dfn = "tglmt_sub_" + sv.var;
            std::string dsig = (st->ret == "void" ? "void" : mslRet) + " " + dfn + "(";
            std::string argNames;
            for (size_t k = 0; k < mparams.size(); ++k) {
                if (k) { dsig += ", "; argNames += ", "; }
                dsig += mparams[k].first + " " + mparams[k].second;
                argNames += mparams[k].second;
            }
            if (!mparams.empty()) dsig += ", ";
            dsig += "int tglmt_sub_idx)";
            std::string dbody = dsig + " {\n";
            for (size_t k = 0; k < impls.size(); ++k) {
                dbody += (k ? "  else " : "  ") + std::string("if (tglmt_sub_idx == ") +
                         std::to_string(k) + ") " +
                         (st->ret == "void" ? "" : "return ") + impls[k] + "(" + argNames + ");\n";
            }
            if (st->ret != "void") dbody += "  return " + mslRet + "(0);\n";
            dbody += "}\n";
            rest += dbody;
            // Rewrite call trong body: `tglmt_u.var(` → `tglmt_sub_var(args, tglmt_u.var)`.
            std::string target = "tglmt_u." + sv.var + "(";
            std::string nb2;
            size_t i2 = 0;
            while (i2 < body.size()) {
                size_t p = body.find(target, i2);
                if (p == std::string::npos) { nb2 += body.substr(i2); break; }
                // word-boundary trước `tglmt_u` (tránh `xtglmt_u.var(`).
                if (p > 0 && IsIdentChar(body[p - 1], false)) { nb2 += body.substr(i2, p - i2 + 1); i2 = p + 1; continue; }
                nb2 += body.substr(i2, p - i2);
                // match parens từ `(` cuối target.
                size_t lp = p + target.size() - 1; // vị trí `(`
                int dd = 0;
                size_t m = lp;
                for (; m < body.size(); ++m) {
                    if (body[m] == '(') ++dd;
                    else if (body[m] == ')') { if (--dd == 0) break; }
                }
                if (m >= body.size()) return fail("subroutine call ngoặc lỗi: " + sv.var);
                std::string args = body.substr(lp + 1, m - lp - 1);
                nb2 += dfn + "(" + args;
                if (!Trim(args).empty()) nb2 += ", ";
                nb2 += "tglmt_u." + sv.var + ")";
                i2 = m + 1;
            }
            body = nb2;
        }
    }
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
        // P-Sodium: attribute đọc trong HELPER (tên thô, vd a_TexCoord trong
        // _vert_init) cũng ACTIVE — strip chỉ theo main sẽ làm helper treo
        // (`use of undeclared identifier` ở Metal compile). Quét bodies của
        // helper defs (trừ headers để khỏi false-positive param trùng tên;
        // main đã tách khỏi rest nên không lẫn).
        // Không làm là crash Sodium chunk (block_layer_opaque.vsh, 2026-10-15).
        std::vector<std::pair<size_t, size_t>> helperBodies;
        for (auto& h : ParseHelperDefs(rest, true))
            if (h.hasBody) helperBodies.emplace_back(h.bodyStart, h.bodyEnd + 1);
        auto usesAttrHelper = [&](const std::string& nm) {
            for (auto& [from, to] : helperBodies) {
                size_t p = from;
                while ((p = rest.find(nm, p)) != std::string::npos && p < to) {
                    bool l = p > 0 && IsIdentChar(rest[p - 1], false);
                    size_t e = p + nm.size();
                    bool r = e < rest.size() && IsIdentChar(rest[e], false);
                    if (!l && !r) return true;
                    p = e;
                }
            }
            return false;
        };
        std::vector<GLSLVar> active;
        for (auto& a : ins)
            if (usesAttr(a.name) || usesAttrHelper(a.name)) active.push_back(a);
        ins.swap(active);
        R.inputs = ins; // conv mang đi link dùng bản đã strip (struct MSL dưới cũng dùng ins)
    }
    // GLSL implicit int→float (sample_lightmap/terrain) + mat4(mat2) + shadowing:
    // chạy trên MSL (sau RI) vì pattern đã là `float2`/`ubo_`/`float4x4`.
    // mat4 names (post-mp) cho `float3x3(<mat4 var>)`: uniform/UBO/SSBO/hoist.
    std::vector<std::string> mat4Names;
    for (auto& u : uniforms)
        if (!u.isSampler && u.mslType == "float4x4")
            mat4Names.push_back("tglmt_u." + u.name);
    for (auto& b : blocks) {
        std::string pref = (b.isStorage ? "ssbo_" : "ubo_") + b.name + ".";
        for (auto& m : b.members)
            if (m.mslType == "float4x4") mat4Names.push_back(pref + m.name);
    }
    for (auto& g : hoisted)
        if (g.mslType == "float4x4") mat4Names.push_back(g.name);
    // Struct GLSL (kể cả inject Iris) → brace-init cho ctor `S(...)`.
    std::vector<std::string> structNames = CollectStructNames(glsl);
    // Helper GLSL trả vector — resolveVec cho `if (v == helper(...))`.
    std::set<std::string> vecHelpers = HelperVecReturns(rest);
    {
        PromoteVanillaIntMixing(body);
        // GLSL int-lit + float arg (`min(1, x.z)`) → MSL ambiguous.
        PromoteIntLitInMinMaxClamp(body);
        // P-Sodium: uint/int→float implicit trong `*`/`/` (vd uint3-call * float).
        PromoteUintVecFloatMixing(body, rest);
        // P-Sodium: mix(x,y,boolN(...)) → mix(x,y,floatN(...)) (MSL thiếu overload).
        PromoteBoolVecMix(body);
        ExpandMat4FromMat2(body);
        ExpandMat3FromMat4(body, mat4Names);
        ExpandStructCtors(body, structNames);
        {
            // Struct instance trong MSL: `_in`/`_out`/`tglmt_u` + UBO/SSBO + local struct.
            std::set<std::string> swRoots = {"_in", "_out", "tglmt_u"};
            for (auto& b : blocks) swRoots.insert((b.isStorage ? "ssbo_" : "ubo_") + b.name);
            CollectStructVarsInText(body, structNames, swRoots);
            NormalizeSwizzles(body, swRoots);
        }
        ExpandRelationalBuiltins(body);
        // mix(x,y,lessThan(a,b)) → 3rd arg `(a < b)` chỉ có SAU relational expand.
        PromoteBoolVecMix(body);
        CoerceIntInitToFloat(body, intUniforms);
        // Arg int-family trong ctor floatN (Iris: vec4(iris_UV2, 0.0, 1.0));
        // ins/outs cấp dim cho tên TRẦN trong body (chưa có tiền tố _in.).
        {
            std::vector<GLSLVar> iv = ins;
            iv.insert(iv.end(), outs.begin(), outs.end());
            CoerceIntArgsInFloatCtors(body, iv, uniforms, blocks);
        }
        {
            std::set<std::string> knownVec;
            if (R.isVertex)
                for (auto& v : outs) knownVec.insert("_out." + v.name);
            else
                for (auto& v : ins) knownVec.insert("_in." + v.name);
            if (R.isVertex) knownVec.insert("_out.tglmt_position");
            WrapVectorConditions(body, knownVec, vecHelpers);
        }
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
        std::string retV = matVaryOutCopy + kZConv + " return _out;";
        std::string retF = isMRT ? "return _out;" : "return tglmt_fragColor;";
        std::string ret = R.isVertex ? retV : ((R.isCompute || R.isTessControl) ? "return;" : retF);
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
        // Param helper trùng tên uniform/UBO phải đổi tên TRƯỚC RewriteIdents
        // (`float far` → nếu không sẽ thành `float tglmt_u.far` — invalid MSL).
        RenameParamShadowUniforms(helpersOnly, mpShared);
        RenameLocalShadowUniforms(helpersOnly, mpShared);
        std::string helpersMSL = RewriteIdents(RewriteArrayCtors(helpersOnly), mpShared);
        // Cùng promotion/expansion trên helpers (sample_lightmap `uv/256.0` và
        // end_portal `float4x4(mat2)` đều nằm trong helper, không phải body).
        // KHÔNG RenameShadowedVars ở đây (định nghĩa hàm luôn kèm `(` nên giữ).
        PromoteVanillaIntMixing(helpersMSL);
        // GLSL int-lit + float arg (`min(1, x.z)`) → MSL ambiguous.
        PromoteIntLitInMinMaxClamp(helpersMSL);
        // P-Sodium: uint/int→float implicit trong `*`/`/` (helpers _deinterleave...).
        PromoteUintVecFloatMixing(helpersMSL, rest);
        // P-Sodium: mix(x,y,boolN(...)) → mix(x,y,floatN(...)).
        PromoteBoolVecMix(helpersMSL);
        ExpandMat4FromMat2(helpersMSL);
        ExpandMat3FromMat4(helpersMSL, mat4Names);
        ExpandStructCtors(helpersMSL, structNames);
        {
            std::set<std::string> swRoots = {"_in", "_out", "tglmt_u"};
            for (auto& b : blocks) swRoots.insert((b.isStorage ? "ssbo_" : "ubo_") + b.name);
            CollectStructVarsInText(helpersMSL, structNames, swRoots);
            NormalizeSwizzles(helpersMSL, swRoots);
        }
        ExpandRelationalBuiltins(helpersMSL);
        // mix(x,y,lessThan(a,b)) → 3rd arg `(a < b)` chỉ có SAU relational expand.
        PromoteBoolVecMix(helpersMSL);
        CoerceIntInitToFloat(helpersMSL, intUniforms);
        // Arg int-family trong ctor floatN (line lỗi thật TERRAIN_SOLID nằm
        // trong helper GetLightMapCoordinates — param `int2 iris_UV2` quét từ
        // chính text helper; ins/outs phòng helper tham chiếu tên trần).
        {
            std::vector<GLSLVar> iv = ins;
            iv.insert(iv.end(), outs.begin(), outs.end());
            CoerceIntArgsInFloatCtors(helpersMSL, iv, uniforms, blocks);
        }
        {
            std::set<std::string> knownVec;
            if (R.isVertex)
                for (auto& v : outs) knownVec.insert("_out." + v.name);
            WrapVectorConditions(helpersMSL, knownVec, vecHelpers);
        }
        // GLSL inout/out param → MSL thread T& (sau mp-rewrite để type đã float3).
        RewriteParamQualifiers(helpersMSL);
        // global sampler table (cho declOf global pairs)
        SampTable globalSamp;
        for (auto& s : R.samplers) {
            SampEntry e = EntryOfSampler(s);
            globalSamp[s.name] = e;
        }
        std::vector<std::string> uboNames;
        for (auto& b : blocks) uboNames.push_back(b.name);
        std::string err;
        std::vector<std::pair<std::string, std::string>> entryBuiltins;
        if (R.isVertex) {
            if (R.usesVertexID) entryBuiltins.emplace_back("tglmt_vertexID", "uint");
            if (R.usesInstanceID) entryBuiltins.emplace_back("tglmt_instanceID", "uint");
        } else if (!R.isCompute && !R.isTessControl && !R.isTessEval) {
            if (R.usesFragCoord) entryBuiltins.emplace_back("tglmt_fragCoord", "float4");
            if (R.usesPointCoord) entryBuiltins.emplace_back("tglmt_pointCoord", "float2");
            if (R.usesFrontFacing) entryBuiltins.emplace_back("tglmt_frontFacing", "bool");
        }
        if (!ThreadHelpersMSL(helpersMSL, body, helperSamp, globalSamp, uboNames, R.isVertex, ins,
                              R.outputs, entryBuiltins, err))
            return fail(err);
        // inout `thread T&` + actual swizzle → block + temp (sau khi params đã final).
        BindSwizzleInoutCalls(body, helpersMSL);
        BindSwizzleInoutCalls(helpersMSL, helpersMSL);
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
        bool useM4C = (body.find("tglmt_mat4_ctor") != std::string::npos) ||
                      (rest.find("tglmt_mat4_ctor") != std::string::npos);
        bool useM3C = (body.find("tglmt_mat3_ctor") != std::string::npos) ||
                      (rest.find("tglmt_mat3_ctor") != std::string::npos);
        bool useMod = (body.find("tglmt_mod") != std::string::npos) ||
                      (rest.find("tglmt_mod") != std::string::npos);
        bool useFLSB = (body.find("tglmt_findLSB") != std::string::npos) ||
                       (rest.find("tglmt_findLSB") != std::string::npos);
        bool useFMSB = (body.find("tglmt_findMSB") != std::string::npos) ||
                       (rest.find("tglmt_findMSB") != std::string::npos);
        bool usePH = (body.find("tglmt_packHalf2x16") != std::string::npos) ||
                     (rest.find("tglmt_packHalf2x16") != std::string::npos);
        bool useUPH = (body.find("tglmt_unpackHalf2x16") != std::string::npos) ||
                      (rest.find("tglmt_unpackHalf2x16") != std::string::npos);
        if (useRad)
            msl_out << "float tglmt_radians(float d) { return d * 0.017453292519943295f; }\n";
        if (useDeg)
            msl_out << "float tglmt_degrees(float r) { return r * 57.29577951308232f; }\n";
        if (useM4C) {
            msl_out << "float4x4 tglmt_mat4_ctor(float s) { return float4x4(s); }\n";
            msl_out << "float4x4 tglmt_mat4_ctor(float2x2 m) { return float4x4(float4(m[0], 0.0, 0.0), float4(m[1], 0.0, 0.0), float4(0.0, 0.0, 1.0, 0.0), float4(0.0, 0.0, 0.0, 1.0)); }\n";
            msl_out << "float4x4 tglmt_mat4_ctor(float3x3 m) { return float4x4(float4(m[0], 0.0), float4(m[1], 0.0), float4(m[2], 0.0), float4(0.0, 0.0, 0.0, 1.0)); }\n";
            msl_out << "float4x4 tglmt_mat4_ctor(float4x4 m) { return m; }\n";
        }
        if (useM3C)
            msl_out << "float3x3 tglmt_mat3_ctor(float4x4 m) { return float3x3(m[0].xyz, m[1].xyz, m[2].xyz); }\n";
        if (useMod) {
            msl_out << "float tglmt_mod(float x, float y) { return x - y * floor(x / y); }\n";
            msl_out << "float2 tglmt_mod(float2 x, float2 y) { return x - y * floor(x / y); }\n";
            msl_out << "float2 tglmt_mod(float2 x, float y) { return x - y * floor(x / y); }\n";
            msl_out << "float3 tglmt_mod(float3 x, float3 y) { return x - y * floor(x / y); }\n";
            msl_out << "float3 tglmt_mod(float3 x, float y) { return x - y * floor(x / y); }\n";
            msl_out << "float4 tglmt_mod(float4 x, float4 y) { return x - y * floor(x / y); }\n";
            msl_out << "float4 tglmt_mod(float4 x, float y) { return x - y * floor(x / y); }\n";
        }
        // findLSB = ctz (bit 0 thấp nhất đang set); findMSB = 31-clz (bit cao
        // nhất đang set). uint/int overload — đã kiểm chứng ctz/clz compile thật.
        if (useFLSB) {
            msl_out << "int tglmt_findLSB(int x) { return int(ctz(uint(x))); }\n";
            msl_out << "uint tglmt_findLSB(uint x) { return ctz(x); }\n";
        }
        if (useFMSB) {
            msl_out << "int tglmt_findMSB(int x) { return int(31u - clz(uint(x))); }\n";
            msl_out << "uint tglmt_findMSB(uint x) { return 31u - clz(x); }\n";
        }
        // packHalf2x16: float→half (round) rồi ghép 2×16bit. MSL không có pack
        // half → dùng as_type<ushort>(half) lấy bit half (đã kiểm chứng thật).
        if (usePH)
            msl_out << "uint tglmt_packHalf2x16(float2 v) { half2 h = half2(v.x, v.y);"
                       " ushort ux = as_type<ushort>(h.x); ushort uy = as_type<ushort>(h.y);"
                       " return uint(ux) | (uint(uy) << 16); }\n";
        if (useUPH)
            msl_out << "float2 tglmt_unpackHalf2x16(uint x) { ushort ux = ushort(x & 0xFFFFu);"
                       " ushort uy = ushort(x >> 16); half hx = as_type<half>(ux);"
                       " half hy = as_type<half>(uy); return float2(float(hx), float(hy)); }\n";
        // ---- Subgroup ballot/vote → MSL simd (đã kiểm chứng: simd_ballot trả
        //  simd_vote, cast ulong = mask, .any()/.all() compile thật) ----
        bool useBallot = (body.find("tglmt_subgroupBallot") != std::string::npos) ||
                         (rest.find("tglmt_subgroupBallot") != std::string::npos);
        bool useVote = (body.find("tglmt_subgroupAny") != std::string::npos) ||
                       (rest.find("tglmt_subgroupAny") != std::string::npos) ||
                       (body.find("tglmt_subgroupAll") != std::string::npos) ||
                       (rest.find("tglmt_subgroupAll") != std::string::npos);
        bool useBCnt = (body.find("tglmt_ballotBitCount") != std::string::npos) ||
                       (rest.find("tglmt_ballotBitCount") != std::string::npos);
        if (useBallot)
            msl_out << "uint4 tglmt_subgroupBallot(bool cond) { ulong b = (ulong)simd_ballot(cond);"
                       " return uint4(uint(b), uint(b >> 32), 0u, 0u); }\n";
        if (useVote) {
            msl_out << "bool tglmt_subgroupAny(bool cond) { return simd_ballot(cond).any(); }\n";
            msl_out << "bool tglmt_subgroupAll(bool cond) { return simd_ballot(cond).all(); }\n";
        }
        if (useBCnt)
            msl_out << "uint tglmt_ballotBitCount(uint4 b) { return popcount(b.x) + popcount(b.y)"
                       " + popcount(b.z) + popcount(b.w); }\n";
    }
    if (R.isVertex) {
        msl_out << "struct TGLMT_VIn {\n";
        for (auto& a : ins) msl_out << "  " << a.mslType << " " << a.name << " [[attribute(" << a.location << ")]];\n";
        msl_out << "};\nstruct TGLMT_VOut {\n  float4 tglmt_position [[position]];\n";
        if (R.usesPointSize) msl_out << "  float tglmt_ptSize [[point_size]];\n";
        for (auto& v : outs) msl_out << "  " << v.mslType << " " << v.name << " [[user(locn" << v.location << ")]];\n";
        msl_out << "};\n";
    } else if (R.isCompute) {
        // P6f: compute không có in/varying struct (gl_*ID là kernel params).
    } else if (R.isTessControl) {
        // P-Tess T1: TCS không có varying struct (in/out/patch là buffer params).
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
    // UBO structs (read-only) + SSBO structs (device). Flat SSBO (unsized array)
    // đã ghi vào ssboFlat ở trên — không emit struct, chỉ device T* param.
    {
        int ubi = 0;
        for (size_t bi = 0; bi < blocks.size(); ++bi) {
            auto& b = blocks[bi];
            if (b.isStorage) {
                if (ssboSlot[bi] == -1) continue; // flat — đã xử lý
                msl_out << "struct TGLMTSSBO_" << b.name << " {\n";
                for (auto& m : b.members) {
                    size_t dot = m.name.find('.');
                    std::string shortN = (dot == std::string::npos) ? m.name : m.name.substr(dot + 1);
                    if (m.arraySize > 0)
                        msl_out << "  " << m.mslType << " " << shortN << "[" << m.arraySize << "];\n";
                    else
                        msl_out << "  " << m.mslType << " " << shortN << ";\n";
                }
                msl_out << "};\n";
                continue;
            }
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
            ++ubi;
        }
    }
    // Param buffer cho block: UBO constant (slot từ blkEmitSlot — graphics 17+k
    // tuần tự chung SSBO; kernel 17+ubi), SSBO-struct device (ssboSlot), SSBO-flat
    // device T* (slot trong ssboFlat). Dùng chung cho 4 entry (vertex/frag/compute).
    std::string blockParams;
    {
        for (size_t bi = 0; bi < blocks.size(); ++bi) {
            auto& b = blocks[bi];
            if (b.isStorage) {
                if (ssboSlot[bi] == -1) continue; // flat — param thêm ở ssboFlat loop
                blockParams += ",\n    device TGLMTSSBO_" + b.name + "& ssbo_" + b.name +
                               " [[buffer(" + std::to_string(ssboSlot[bi]) + ")]]";
                continue;
            }
            blockParams += ",\n    constant TGLMTUBO_" + b.name + "& ubo_" + b.name +
                           " [[buffer(" + std::to_string(blkEmitSlot[bi]) + ")]]";
        }
        for (auto& f : ssboFlat)
            blockParams += ",\n    device " + f.type + "* " + f.param +
                           " [[buffer(" + std::to_string(f.slot) + ")]]";
        // P9 atomic counters: device atomic_uint* tại buffer slot.
        // Compute/TCS-kernel: sau SSBOs (0+). Graphics: SAU block cuối (P4 — UBO/
        // SSBO graphics dùng 17+k tuần tự); giữ 24 khi ít block như cũ, fail nếu vượt 30.
        {
            int baseSlot = 0;
            if (!kernelLike) {
                baseSlot = std::max(24, 17 + (int)blocks.size());
                if (!R.atomicCounters.empty() && baseSlot + (int)R.atomicCounters.size() - 1 > 30)
                    return fail("atomic counter graphics vượt slot buffer(30)");
            } else {
                int ssboCnt2 = (int)ssboFlat.size();
                for (size_t bi = 0; bi < blocks.size(); ++bi)
                    if (blocks[bi].isStorage && ssboSlot[bi] != -1) ++ssboCnt2;
                baseSlot = ssboCnt2;
            }
            for (size_t k = 0; k < R.atomicCounters.size(); ++k) {
                int slot = baseSlot + (int)k;
                if (slot > 30 || (!kernelLike && slot < 24)) return fail("atomic counter slot ngoài phạm vi");
                if (kernelLike && slot > 15) return fail("atomic counter vượt slot kernel (0..15)");
                blockParams += ",\n    device atomic_uint* " + R.atomicCounters[k].name +
                               " [[buffer(" + std::to_string(slot) + ")]]";
                R.atomicSlots.push_back({R.atomicCounters[k].name,
                                         R.atomicCounters[k].atomicBinding,
                                         R.atomicCounters[k].atomicOffset, slot});
            }
        }
    }
    // Prune sampler KHÔNG dùng theo stage: Metal giới hạn 16 sampler/stage mà
    // pack khai báo chung mọi texture (deferred1.vsh khai báo 23 → vượt 16).
    // Scan body+rest (helpers) theo `<name>_tex`/`<name>_smp` (đã RewriteIdents).
    // Index giữ nguyên thứ tự khai báo → backend bind khớp (gl_program copy
    // conv.samplers đã prune; image slot = R.samplers.size()+k nhất quán).
    {
        std::vector<GLSLVar> used;
        auto wordRef = [](const std::string& hay, const std::string& w) {
            size_t p = 0;
            while ((p = hay.find(w, p)) != std::string::npos) {
                bool l = p > 0 && IsIdentChar(hay[p - 1], false);
                size_t e = p + w.size();
                bool r = e < hay.size() && IsIdentChar(hay[e], false);
                if (!l && !r) return true;
                p = e;
            }
            return false;
        };
        for (auto& s : R.samplers) {
            std::string a = s.name + "_tex", b = s.name + "_smp";
            if (wordRef(body, a) || wordRef(body, b) || wordRef(rest, a) || wordRef(rest, b))
                used.push_back(s);
        }
        R.samplers.swap(used);
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
                          : s.isArray ? "texture2d_array<float>"
                          : s.is1D ? "texture2d<" + s.sampleType + ">"        // P3: 1D ≈ 2D
                          : s.is3D ? "texture3d<" + s.sampleType + ">"        // P3: 3D thật
                                   : "texture2d<" + s.sampleType + ">";       // 2D (i/u → int/uint)
        texParams += ", " + ttype + " " + s.name + "_tex [[texture(" + ks +
                     ")]], sampler " + s.name + "_smp [[sampler(" + ks + ")]]";
    }
    // P6g image params: texture slot nối tiếp sampler (không sampler state —
    // imageLoad/Store là texel access, không sample). Param tên = tên image GLSL
    // (body đã rewrite `imageLoad(img,..)` → `img.read(..)` giữ tên).
    for (size_t k = 0; k < R.images.size(); ++k) {
        auto& im = R.images[k];
        std::string ks = std::to_string(R.samplers.size() + k);
        // mslType đã là `texture2d<float, access::read_write>` từ parser.
        texParams += ", " + im.mslType + " " + im.name + " [[texture(" + ks + ")]]";
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
    // Varying matN: local matrix cùng tên (FS ghép từ cột _in; VS khai báo + copy
    // ra _out cột trước kZConv). Chèn sau MỌI pass rewrite body.
    if (!matVaryInDecl.empty()) body = matVaryInDecl + body;
    if (!matVaryOutDecl.empty()) body = matVaryOutDecl + body;
    if (R.isVertex) {
        msl_out << "vertex TGLMT_VOut TGLMT_vs(TGLMT_VIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        if (R.usesVertexID) msl_out << ",\n    uint tglmt_vertexID [[vertex_id]]";
        if (R.usesInstanceID) msl_out << ",\n    uint tglmt_instanceID [[instance_id]]";
        if (useBaseInstance) msl_out << ",\n    uint tglmt_baseInstance [[base_instance]]";
        msl_out << texParams << ") {\n  TGLMT_VOut _out = {};\n" << body << "\n"
                << matVaryOutCopy << kZConv << "\n  return _out;\n}\n";
    } else if (R.isCompute) {
        // P6f: compute → kernel. Thread-position params emit theo mức body dùng
        // (đã probe Metal thật: thread_position_in_grid/threadgroup/thread_index OK).
        msl_out << "kernel void TGLMT_cs(constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        if (useGlobalID) msl_out << ",\n    uint3 tglmt_globalID [[thread_position_in_grid]]";
        if (useLocalID) msl_out << ",\n    uint3 tglmt_localID [[thread_position_in_threadgroup]]";
        if (useWorkGroupID) msl_out << ",\n    uint3 tglmt_workGroupID [[threadgroup_position_in_grid]]";
        if (useNumWG) msl_out << ",\n    uint3 tglmt_numWorkGroups [[threadgroups_per_grid]]";
        if (useLocalIndex) msl_out << ",\n    uint tglmt_localIndex [[thread_index_in_threadgroup]]";
        msl_out << texParams << ") {\n";
        // gl_WorkGroupSize: hằng localSize đọc từ layout(local_size_x/y/z) in;
        if (useWorkGroupSize)
            msl_out << "  const uint3 tglmt_workGroupSize = uint3(" << R.localSize[0] << "u, "
                    << R.localSize[1] << "u, " << R.localSize[2] << "u);\n";
        msl_out << body << "\n}\n";
    } else if (R.isTessControl) {
        // P-Tess T1: TCS → kernel (MoltenVK/SPIRV-Cross pattern: TCS là compute,
        // TES là post-tess vertex, tessellator cố định của Metal ở giữa).
        // Thread = (invocation, patch): T4 dispatch grid=(N, numPatches, 1),
        // threadgroup=(N,1,1) để barrier() đồng bộ đúng N invocation/patch.
        msl_out << "kernel void TGLMT_tcs(constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        // Buffer slot tess I/O: sau UBO cuối (17+numUBO), bound-check ≤30.
        // SSBO/atomic (0+, compute-like) + sampler/image (texture space) không đụng.
        int tbase = 17;
        for (auto& b : blocks) if (!b.isStorage) ++tbase;
        auto takeSlot = [&]() -> int { return tbase++; };
        // Per-vertex inputs → constant T* (T4 điền từ VS stage / control points).
        for (auto& v : ins) {
            if (v.isPatch) continue; // TCS patch-in đã fail ở parse
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    constant " << v.mslType << "* tcs_in_" << v.name
                    << " [[buffer(" << s << ")]]";
            R.tessSlots.push_back({v.name, s, true, false, false, v.mslType, R.tessVertices});
        }
        // gl_in (VS gl_Position/PointSize) — buffer riêng, T4 điền từ VS outputs.
        // Emit theo tham chiếu post-rewrite (tránh param thừa).
        if (useTcsGlIn && body.find("tcs_in_gl_Position") != std::string::npos) {
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    constant float4* tcs_in_gl_Position [[buffer(" << s << ")]]";
            R.tessSlots.push_back({"gl_Position", s, true, false, true, "float4", R.tessVertices});
        }
        if (useTcsGlIn && body.find("tcs_in_gl_PointSize") != std::string::npos) {
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    constant float* tcs_in_gl_PointSize [[buffer(" << s << ")]]";
            R.tessSlots.push_back({"gl_PointSize", s, true, false, true, "float", R.tessVertices});
        }
        // Per-vertex outputs → device T* (T4 + TES đọc qua patch buffer).
        for (auto& v : outs) {
            if (v.isPatch) continue;
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    device " << v.mslType << "* tcs_out_" << v.name
                    << " [[buffer(" << s << ")]]";
            R.tessSlots.push_back({v.name, s, false, false, false, v.mslType, R.tessVertices});
        }
        // Patch outputs → device T* (scalar: 1/patch; array[M]: M/patch).
        for (auto& v : outs) {
            if (!v.isPatch) continue;
            int cnt = v.arraySize > 0 ? v.arraySize : 1;
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    device " << v.mslType << "* tcs_patch_" << v.name
                    << " [[buffer(" << s << ")]]";
            R.tessSlots.push_back({v.name, s, false, true, false, v.mslType, cnt});
        }
        // Factor buffer triangles (Half — KHỚP backend makeTessPipeline Half format
        // + test_tess_patch_apple 8B/patch). T4 bind + drawPatches đọc.
        if (useTcsFactors) {
            int s = takeSlot();
            if (s > 30) return fail("TCS hết buffer slot (max 30)");
            msl_out << ",\n    device MTLTriangleTessellationFactorsHalf* tcs_factors [[buffer(" << s << ")]]";
            R.tessFactorSlot = s;
        }
        // tid param + patchID local: luôn emit (useTcsTid hằng true — rẻ, tránh flag bug).
        if (useTcsTid) msl_out << ",\n    uint3 tglmt_tcs_tid [[thread_position_in_grid]]";
        msl_out << texParams << ") {\n  uint tglmt_patchID = tglmt_tcs_tid.y;\n" << body << "\n}\n";
    } else if (isMRT || useFragDepth || useSampleMask) {
        // Fragment trả STRUCT khi: MRT, hoặc có output builtin depth/sample_mask.
        // (gl_FragDepth → _out.depth; gl_SampleMask[0] → _out.smask.)
        // Single out: kiểu theo khai báo (float cho R32F, ...); legacy = float4.
        std::string fragT = (!isMRT && !legacyFrag && !outs.empty()) ? outs[0].mslType : "float4";
        msl_out << "struct TGLMT_FOut {\n";
        if (isMRT) {
            for (auto& o : outs) msl_out << "  " << o.mslType << " mrt" << o.location << " [[color(" << o.location << ")]];\n";
        } else if (!outs.empty()) {
            msl_out << "  " << fragT << " c [[color(0)]];\n";
        }
        if (useFragDepth) msl_out << "  float depth [[depth(any)]];\n";
        if (useSampleMask) msl_out << "  uint smask [[sample_mask]];\n";
        msl_out << "};\n";
        msl_out << "fragment TGLMT_FOut TGLMT_fs(TGLMT_FIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        if (R.usesPointCoord) msl_out << ",\n    float2 tglmt_pointCoord [[point_coord]]";
        if (R.usesFragCoord) msl_out << ",\n    float4 tglmt_fragCoord [[position]]";
        if (R.usesFrontFacing) msl_out << ",\n    bool tglmt_frontFacing [[front_facing]]";
        if (useStencilRef) msl_out << ",\n    uint tglmt_stencilRef [[stencil_ref]]";
        if (useSampleID) msl_out << ",\n    uint tglmt_sampleID [[sample_id]]";
        if (useSamplePos) msl_out << ",\n    float2 tglmt_samplePosition [[sample_position]]";
        msl_out << texParams << ") {\n  TGLMT_FOut _out = {};\n";
        // Single-out: body vẫn ghi vào tglmt_fragColor → ghi vào _out.c ở cuối.
        if (!isMRT && !outs.empty()) msl_out << "  " << fragT << " tglmt_fragColor = " << MSLZero(fragT) << ";\n";
        msl_out << body << "\n";
        if (!isMRT && !outs.empty()) msl_out << "  _out.c = tglmt_fragColor;\n";
        msl_out << "  return _out;\n}\n";
    } else if (outs.empty() && !legacyFrag) {
        // Fragment không có color output (chỉ discard) → `fragment void` hợp lệ
        // trong MSL; pipeline backend bỏ qua color attachment ghi.
        msl_out << "fragment void TGLMT_fs(TGLMT_FIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        if (R.usesPointCoord) msl_out << ",\n    float2 tglmt_pointCoord [[point_coord]]";
        if (R.usesFragCoord) msl_out << ",\n    float4 tglmt_fragCoord [[position]]";
        if (R.usesFrontFacing) msl_out << ",\n    bool tglmt_frontFacing [[front_facing]]";
        if (useStencilRef) msl_out << ",\n    uint tglmt_stencilRef [[stencil_ref]]";
        if (useSampleID) msl_out << ",\n    uint tglmt_sampleID [[sample_id]]";
        if (useSamplePos) msl_out << ",\n    float2 tglmt_samplePosition [[sample_position]]";
        msl_out << texParams << ") {\n" << body << "\n}\n";
    } else {
        std::string fragT = (!legacyFrag && !outs.empty()) ? outs[0].mslType : "float4";
        std::string fragZero = MSLZero(fragT);
        msl_out << "fragment " << fragT << " TGLMT_fs(TGLMT_FIn _in [[stage_in]],\n"
                   "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
        msl_out << blockParams;
        if (R.usesPointCoord) msl_out << ",\n    float2 tglmt_pointCoord [[point_coord]]";
        if (R.usesFragCoord) msl_out << ",\n    float4 tglmt_fragCoord [[position]]";
        if (R.usesFrontFacing) msl_out << ",\n    bool tglmt_frontFacing [[front_facing]]";
        if (useStencilRef) msl_out << ",\n    uint tglmt_stencilRef [[stencil_ref]]";
        if (useSampleID) msl_out << ",\n    uint tglmt_sampleID [[sample_id]]";
        if (useSamplePos) msl_out << ",\n    float2 tglmt_samplePosition [[sample_position]]";
        msl_out << texParams << ") {\n  " << fragT << " tglmt_fragColor = " << fragZero << ";\n" << body
                << "\n  return tglmt_fragColor;\n}\n";
    }
    R.msl = msl_out.str();
    R.ok = true;
    return R;
}

} // namespace tglmt
