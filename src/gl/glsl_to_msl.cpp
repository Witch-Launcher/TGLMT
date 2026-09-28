// GLSLConverter.cpp — xem GLSLConverter.h về subset được hỗ trợ.
// Mọi thứ ngoài subset → ok=false + log rõ ràng (không đoán mò sinh code sai).
#include "tglmt/GLSLConverter.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <utility>

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
// Tách "A, B[4], C = vec2(1.0, 2.0)" → [(A,0),(B,4),(C,0)].
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
    std::string rest; // phần còn lại (hàm, main, const global)
    {
        auto stmts = SplitTopLevel(src);
        // Tách main + hàm ra khỏi decl: decl chỉ là statement 1 dòng không chứa '{'.
        // SplitTopLevel đã tách theo ';' ở depth 0 → các hàm (có '{') nằm trong "phần dư".
        // Làm lại đơn giản: duyệt từng dòng logic — với shader có style 1-decl-1-dòng.
        // Cách chắc chắn hơn: với mỗi stmt, nếu chứa '{' hoặc "void main" → cho vào rest.
        // UBO block chứa '{' nhưng vẫn là decl → nhận diện TRƯỚC qua từ khóa uniform+{.
        auto startsKw = [&](const std::string& t) {
            return t.rfind("layout", 0) == 0 || t.rfind("in ", 0) == 0 ||
                   t.rfind("out ", 0) == 0 || t.rfind("uniform ", 0) == 0 ||
                   t.rfind("const ", 0) == 0;
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
        // Hàng đợi xử lý: stmt lẫn `hàm...} decl;` được tách (decl sau hàm
        // rất thường gặp ở vanilla: uniform/varying sau helper function).
        std::vector<std::string> queue;
        for (auto& st : stmts) queue.push_back(st.text);
        auto peelMixed = [](const std::string& t, std::string& funcPart, std::string& tail) -> bool {
            int depth = 0;
            size_t lastZero = std::string::npos;
            bool inStr = false;
            for (size_t i = 0; i < t.size(); ++i) {
                char ch = t[i];
                if (inStr) { if (ch == '"') inStr = false; continue; }
                if (ch == '"') { inStr = true; continue; }
                if (ch == '{') ++depth;
                else if (ch == '}') {
                    --depth;
                    if (depth == 0) lastZero = i + 1;
                }
            }
            if (lastZero == std::string::npos) return false;
            std::string tl = Trim(t.substr(lastZero));
            if (tl.empty()) return false;
            funcPart = t.substr(0, lastZero);
            tail = tl;
            return true;
        };
        for (size_t qi = 0; qi < queue.size(); ++qi) {
            std::string t = Trim(queue[qi]);
            if (t.empty() || t == ";") continue; // khoảng trắng thừa, bỏ qua
            // UBO block TRƯỚC (chứa '{' nhưng là decl)
            {
                GLSLBlock b;
                // UBO có thể bị SplitTopLevel cắt sai vì chứa ';' bên trong ở depth>0?
                // SplitTopLevel chỉ cắt ở depth 0 nên block nguyên vẹn (có '{...}' + ';' cuối).
                if ((t.rfind("layout", 0) == 0 || t.rfind("uniform", 0) == 0) &&
                    t.find('{') != std::string::npos && tryParseUBO(t, b)) {
                    blocks.push_back(b);
                    continue;
                }
            }
            // Tách `hàm...} decl;` lẫn nhau: func vào rest, decl xử lý tiếp.
            // (UBO sau hàm (`} uniform B {...};`) đã bắt ở trên qua tail check dưới.)
            if (t.find('{') != std::string::npos) {
                std::string funcPart, tail;
                if (peelMixed(t, funcPart, tail)) {
                    rest += funcPart;
                    // tail có thể là UBO → thử trước
                    GLSLBlock b;
                    std::string tt = Trim(tail);
                    if ((tt.rfind("layout", 0) == 0 || tt.rfind("uniform", 0) == 0) &&
                        tt.find('{') != std::string::npos && tryParseUBO(tt, b)) {
                        blocks.push_back(b);
                        continue;
                    }
                    queue.insert(queue.begin() + qi + 1, tail);
                    continue;
                }
                // nguyên khối hàm, không decl lẫn
                rest += queue[qi];
                continue;
            }
            // Khai báo kiểm tra TRƯỚC (layout(...) có ngoặc nhưng vẫn là decl!)
            if (!startsKw(t)) {
                if (t.find('}') != std::string::npos ||
                    t.find("void") != std::string::npos || t.find('(') != std::string::npos) {
                    rest += queue[qi];
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
                        // vanilla dùng sampler2D; Shadow/Cube/Array map về texture2d (giới hạn A11)
                        v.mslType = "sampler";
                        v.isSampler = true;
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
                // const global (có thể nhiều declarator): `constant T a = ..; ...`
                if (t.rfind("const", 0) == 0) {
                    std::string body = Trim(t.substr(5)); // bỏ 1 "const" (tránh `constant const`)
                    if (!body.empty() && body.back() == ';') body.pop_back();
                    // tách `T a = 1, b = 2` → head type + declarators
                    std::istringstream cs(body);
                    std::vector<std::string> cw; std::string ctk;
                    while (cs >> ctk) cw.push_back(ctk);
                    if (cw.empty()) return fail("khai báo không hỗ trợ: " + t);
                    std::string ctype = cw[0];
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
                        if (carr != 0) return fail("mảng const ngoài subset: " + t);
                        // giữ initializer gốc: tìm lại `cnm = ...` trong crest
                        std::string init;
                        size_t fp = crest.find(cnm);
                        if (fp != std::string::npos) {
                            size_t eq = crest.find('=', fp + cnm.size());
                            if (eq != std::string::npos) {
                                size_t cm2 = crest.find(',', eq);
                                init = Trim(crest.substr(eq + 1, cm2 == std::string::npos
                                                                    ? std::string::npos : cm2 - eq - 1));
                            }
                        }
                        // viết lại tên kiểu GLSL trong init (vec→float)
                        for (auto& rp : std::vector<std::pair<std::string,std::string>>{
                                 {"vec2","float2"},{"vec3","float3"},{"vec4","float4"},
                                 {"mat2","float2x2"},{"mat3","float3x3"},{"mat4","float4x4"}}) {
                            size_t pp = 0;
                            while ((pp = init.find(rp.first, pp)) != std::string::npos) {
                                bool l = pp > 0 && IsIdentChar(init[pp-1], false);
                                size_t e2 = pp + rp.first.size();
                                bool r2 = e2 < init.size() && IsIdentChar(init[e2], false);
                                if (!l && !r2) { init.replace(pp, rp.first.size(), rp.second); pp += rp.second.size(); }
                                else ++pp;
                            }
                        }
                        rest += "constant " + cm + " " + cnm + (init.empty() ? "" : " = " + init) + ";\n";
                    }
                } else {
                    return fail("khai báo không hỗ trợ: " + t);
                }
            } else {
                rest += queue[qi];
            }
        }
    }
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
    // Bảng viết lại identifier
    std::vector<std::pair<std::string, std::string>> mp;
    // NDC-z convert: GL clip z [-w,w] → Metal [0,w] (mặc định NEGATIVE_ONE_TO_ONE).
    // Chèn ` _out.position.z = _out.position.z*0.5 + _out.position.w*0.5;`
    // trước mọi return vertex (kể cả return sớm). ZERO_TO_ONE cần recompile (log ở link).
    const std::string kZConv = " _out.position.z = _out.position.z * 0.5 + _out.position.w * 0.5;";
    bool isMRT = !R.isVertex && outs.size() > 1;
    if (R.isVertex) {
        for (auto& a : ins) mp.emplace_back(a.name, "_in." + a.name);
        for (auto& v : outs) mp.emplace_back(v.name, "_out." + v.name);
        mp.emplace_back("gl_Position", "_out.position");
        mp.emplace_back("gl_PointSize", "_out.pointSize");
        if (body.find("gl_PointSize") != std::string::npos) R.usesPointSize = true;
        // screenquad (lightmap/blit): đỉnh suy từ index, không cần attribute.
        // GLSL int → MSL uint (bitwise tương đương).
        mp.emplace_back("gl_VertexID", "tglmt_vertexID");
        if (body.find("gl_VertexID") != std::string::npos) R.usesVertexID = true;
        mp.emplace_back("gl_InstanceID", "tglmt_instanceID");
        if (body.find("gl_InstanceID") != std::string::npos) R.usesInstanceID = true;
    } else {
        for (auto& v : ins) mp.emplace_back(v.name, "_in." + v.name);
        mp.emplace_back("gl_PointCoord", "tglmt_pointCoord");
        if (body.find("gl_PointCoord") != std::string::npos) R.usesPointCoord = true;
        mp.emplace_back("gl_FragCoord", "tglmt_fragCoord");
        if (body.find("gl_FragCoord") != std::string::npos) R.usesFragCoord = true;
        if (isMRT) {
            for (auto& o : outs) mp.emplace_back(o.name, "_out.mrt" + std::to_string(o.location));
        } else if (legacyFrag) {
            // end_portal compat: gl_FragColor / gl_FragData[0] → out duy nhất
            mp.emplace_back("gl_FragColor", "tglmt_fragColor");
        } else {
            // out color: tên biến out → tglmt_fragColor (return ở cuối)
            mp.emplace_back(outs[0].name, "tglmt_fragColor");
        }
    }
    for (auto& u : uniforms) {
        if (u.isSampler) continue;
        mp.emplace_back(u.name, "tglmt_u." + u.name);
    }
    // UBO members: `member` hoặc `inst.member` → `ubo_Block.member`
    for (auto& b : blocks) {
        for (auto& m : b.members) {
            size_t dot = m.name.find('.');
            std::string access = (dot == std::string::npos) ? m.name : m.name.substr(dot + 1);
            // instance prefix giữ nguyên ở GLSL? MSL struct không có instance → map cả 2 dạng
            mp.emplace_back(m.name, "ubo_" + b.name + "." + access);
            if (dot == std::string::npos) {
                // không instance: body dùng `member` trực tiếp
            }
        }
    }
    // Tên kiểu GLSL trong thân hàm → MSL nghiêm (runtime compiler không chấp
    // nhận vec3 như offline `metal -c` — đã quan sát). double không tồn tại ở MSL.
    mp.emplace_back("vec2", "float2");
    mp.emplace_back("vec3", "float3");
    mp.emplace_back("vec4", "float4");
    mp.emplace_back("ivec2", "int2");
    mp.emplace_back("ivec3", "int3");
    mp.emplace_back("ivec4", "int4");
    mp.emplace_back("uvec2", "uint2");
    mp.emplace_back("uvec3", "uint3");
    mp.emplace_back("uvec4", "uint4");
    mp.emplace_back("mat2", "float2x2");
    mp.emplace_back("mat3", "float3x3");
    mp.emplace_back("mat4", "float4x4");
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
    mp.emplace_back("discard", "discard_fragment()");
    // texture(sampler, uv[, bias]) → sampler_tex.sample(sampler_smp, uv[, bias])
    // (xử lý trước RewriteIdents bằng scan ngoặc)
    {
        std::string out;
        size_t i = 0;
        auto isSampler = [&](const std::string& w) {
            for (auto& s : R.samplers)
                if (s.name == w) return true;
            return false;
        };
        while (i < body.size()) {
            if ((body.compare(i, 7, "texture") == 0) &&
                (i == 0 || !IsIdentChar(body[i - 1], false)) &&
                (i + 7 >= body.size() || !IsIdentChar(body[i + 7], false))) {
                size_t lp = body.find('(', i + 7);
                if (lp == std::string::npos) { out += body.substr(i); break; }
                // tách args cấp 1
                int d = 0;
                size_t k = lp;
                std::vector<std::string> args;
                size_t a0 = lp + 1;
                for (; k < body.size(); ++k) {
                    if (body[k] == '(') ++d;
                    else if (body[k] == ')') {
                        if (--d == 0) break;
                    } else if (body[k] == ',' && d == 1) {
                        args.push_back(Trim(body.substr(a0, k - a0)));
                        a0 = k + 1;
                    }
                }
                if (k >= body.size()) return fail("texture(...) ngoặc không cân bằng");
                args.push_back(Trim(body.substr(a0, k - a0)));
                if (args.size() < 2 || args.size() > 3 || !isSampler(Trim(args[0])))
                    return fail("texture(...) chỉ hỗ trợ texture(sampler2D, uv[, bias])");
                std::string sname = Trim(args[0]);
                std::string rep = sname + "_tex.sample(" + sname + "_smp, " + args[1];
                if (args.size() == 3) rep += ", " + args[2];
                rep += ")";
                out += rep;
                i = k + 1;
                continue;
            }
            out += body[i];
            ++i;
        }
        body = out;
    }
    // texelFetch(sampler, coord[, lod]) → .read() (entity VS, CloudFaces).
    // Buffer sampler: read(uint2(index, 0)). Sampler 2D: read(uint2(coord), lod).
    {
        auto isSampler = [&](const std::string& w, const GLSLVar*& out) {
            for (auto& s : R.samplers)
                if (s.name == w) { out = &s; return true; }
            out = nullptr;
            return false;
        };
        std::string out;
        size_t i = 0;
        auto isWordCharAt = [&](size_t p) { return p < body.size() && IsIdentChar(body[p], false); };
        while (i < body.size()) {
            bool head = (body.compare(i, 10, "texelFetch") == 0) &&
                        (i == 0 || !IsIdentChar(body[i - 1], false)) && !isWordCharAt(i + 10);
            if (!head) { out += body[i]; ++i; continue; }
            size_t lp = body.find('(', i + 10);
            if (lp == std::string::npos) { out += body.substr(i); break; }
            int d = 0;
            size_t k = lp;
            std::vector<std::string> args;
            size_t a0 = lp + 1;
            for (; k < body.size(); ++k) {
                if (body[k] == '(') ++d;
                else if (body[k] == ')') { if (--d == 0) break; }
                else if (body[k] == ',' && d == 1) {
                    args.push_back(Trim(body.substr(a0, k - a0)));
                    a0 = k + 1;
                }
            }
            if (k >= body.size()) return fail("texelFetch(...) ngoặc không cân bằng");
            args.push_back(Trim(body.substr(a0, k - a0)));
            const GLSLVar* sv = nullptr;
            if (args.empty() || !isSampler(Trim(args[0]), sv))
                return fail("texelFetch chỉ hỗ trợ texelFetch(sampler, coord[, lod])");
            std::string sname = Trim(args[0]);
            std::string rep;
            if (sv->isBuffer) {
                if (args.size() != 2)
                    return fail("texelFetch buffer chỉ 2 args (sampler, index)");
                rep = sname + "_tex.read(uint2(uint(" + args[1] + "), 0u))";
            } else {
                if (args.size() < 2 || args.size() > 3)
                    return fail("texelFetch 2D cần (sampler, coord[, lod])");
                std::string lod = args.size() == 3 ? args[2] : "0";
                rep = sname + "_tex.read(uint2(" + args[1] + "), " + lod + ")";
            }
            out += rep;
            i = k + 1;
        }
        body = out;
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
    body = RewriteIdents(body, mp);
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

    // Sinh MSL
    std::ostringstream msl_out;
    msl_out << "#include <metal_stdlib>\nusing namespace metal;\n";
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
        if (R.samplers[k].isBuffer) {
            texParams += ", texture2d<" + R.samplers[k].sampleType + "> " +
                         R.samplers[k].name + "_tex [[texture(" + std::to_string(k) + ")]]";
            continue;
        }
        texParams += ", texture2d<float> " + R.samplers[k].name + "_tex [[texture(" +
                     std::to_string(k) + ")]], sampler " + R.samplers[k].name + "_smp [[sampler(" +
                     std::to_string(k) + ")]]";
    }
    // Phần rest (hàm helper + const global): viết lại identifiers rồi giữ lại,
    // NHƯNG bỏ thân main cũ (đã trích body).
    {
        // Bỏ hàm main cũ khỏi rest: tìm "void main" tới '}' cân bằng và xóa.
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
            if (bad) return fail("khai báo in/out/uniform/layout đặt sau hàm hoặc sai cú pháp (converter yêu cầu khai báo trước hàm)");
        }
        msl_out << RewriteIdents(noMain, mp);
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
        msl_out << texParams << ") {\n  float4 tglmt_fragColor = float4(0.0);\n" << body
                << "\n  return tglmt_fragColor;\n}\n";
    }
    R.msl = msl_out.str();
    R.ok = true;
    return R;
}

} // namespace tglmt
