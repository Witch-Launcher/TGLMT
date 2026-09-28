// GLSLConverter.cpp — xem GLSLConverter.h về subset được hỗ trợ.
// Mọi thứ ngoài subset → ok=false + log rõ ràng (không đoán mò sinh code sai).
#include "tglmt/GLSLConverter.h"
#include <cctype>
#include <sstream>

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
// Parse "layout(location=N) in|out TYPE name" hoặc "in|out TYPE name" hoặc
// "uniform TYPE name[N]" (array cho vanilla/Sodium). Trả false nếu không khớp.
static bool ParseDecl(const std::string& stmt, bool isVertex,
                      std::string& dir, GLSLVar& v, bool& isUniform) {
    std::string s = Trim(stmt);
    if (!s.empty() && s.back() == ';') s.pop_back();
    s = Trim(s);
    isUniform = false;
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
               t == "writeonly";
    };
    auto parseArray = [](std::string& name, int& arr) {
        arr = 0;
        size_t lb = name.find('[');
        if (lb == std::string::npos) return true;
        size_t rb = name.find(']', lb);
        if (rb == std::string::npos) return false;
        std::string n = Trim(name.substr(lb + 1, rb - lb - 1));
        if (n.empty()) return false; // unsized array ngoài subset (trừ sampler? vẫn cần size)
        arr = atoi(n.c_str());
        if (arr <= 0 || arr > 1024) return false;
        name = Trim(name.substr(0, lb));
        // tên mảng có thể dính ", ..." ? caller chỉ cho 1 decl/dòng nên OK
        return true;
    };
    if (w.empty()) return false;
    if (w[0] == "uniform") {
        size_t k = 1;
        while (k < w.size() && isQual(w[k])) ++k;
        if (k + 1 >= w.size() && (k >= w.size())) return false;
        // uniform block (`uniform Block {`) → caller xử lý, không phải single uniform
        if (w[k].find('{') != std::string::npos) return false;
        if (k + 1 >= w.size()) return false;
        // gộp tên có thể chứa "[N]" dính hoặc tách ("x[4]" hoặc "x [4]" → iss tách?)
        std::string nm = w[k + 1];
        for (size_t q = k + 2; q < w.size(); ++q) nm += w[q];
        int arr = 0;
        if (!parseArray(nm, arr)) return false;
        isUniform = true;
        dir = "uniform";
        v.glslType = w[k];
        v.name = nm;
        v.location = -1;
        v.arraySize = arr;
        return true;
    }
    if (w[0] == "in" || w[0] == "out") {
        size_t k = 1;
        while (k < w.size() && isQual(w[k])) ++k;
        if (k + 1 >= w.size()) return false;
        std::string nm = w[k + 1];
        for (size_t q = k + 2; q < w.size(); ++q) nm += w[q];
        int arr = 0;
        if (!parseArray(nm, arr)) return false;
        // in/out array (vd `out vec4 c[2]`) ngoài subset MRT đơn giản → từ chối rõ
        // (MRT dùng nhiều `out` riêng, không phải array).
        if (arr != 0) return false;
        dir = w[0];
        v.glslType = w[k];
        v.name = nm;
        v.location = loc;
        (void)isVertex;
        return true;
    }
    return false;
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
    // Tiền xử lý: kiểm tra #version, bỏ precision, cấm directive khác.
    {
        std::istringstream iss(src);
        std::string line, kept;
        bool sawVersion = false;
        while (std::getline(iss, line)) {
            std::string t = Trim(line);
            if (!t.empty() && t[0] == '#') {
                if (t.rfind("#version", 0) == 0) {
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
                return fail("directive không hỗ trợ: " + t);
            }
            if (t.rfind("precision", 0) == 0) continue; // MSL bỏ qua precision
            kept += line + "\n";
        }
        if (!sawVersion) return fail("thiếu '#version' (GLSL yêu cầu)");
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
        for (auto& st : stmts) {
            std::string t = Trim(st.text);
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
            // Khai báo kiểm tra TRƯỚC (layout(...) có ngoặc nhưng vẫn là decl!)
            if (!startsKw(t)) {
                if (t.find('{') != std::string::npos || t.find('}') != std::string::npos ||
                    t.find("void") != std::string::npos || t.find('(') != std::string::npos) {
                    rest += st.text;
                    continue;
                }
                return fail("câu lệnh không hỗ trợ: " + t);
            }
            std::string dir;
            GLSLVar v;
            bool isU = false;
            if (ParseDecl(t, R.isVertex, dir, v, isU)) {
                bool okT = false;
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
            } else if (!t.empty() && t != ";") {
                // const global hoặc thứ khác
                if (t.rfind("const", 0) == 0) {
                    rest += "constant " + t; // MSL: global const phải có `constant`
                } else {
                    return fail("khai báo không hỗ trợ: " + t);
                }
            } else {
                rest += st.text;
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
    if (!R.isVertex) {
        if (outs.empty() || outs.size() > 8)
            return fail("fragment cần 1..8 out (hiện có " + std::to_string(outs.size()) + ")");
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
    } else {
        for (auto& v : ins) mp.emplace_back(v.name, "_in." + v.name);
        mp.emplace_back("gl_PointCoord", "tglmt_pointCoord");
        if (body.find("gl_PointCoord") != std::string::npos) R.usesPointCoord = true;
        mp.emplace_back("gl_FragCoord", "tglmt_fragCoord");
        if (body.find("gl_FragCoord") != std::string::npos) R.usesFragCoord = true;
        if (isMRT) {
            for (auto& o : outs) mp.emplace_back(o.name, "_out.mrt" + std::to_string(o.location));
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
    if (body.find("texelFetch") != std::string::npos)
        return fail("texelFetch ngoài subset (dùng texture() với NEAREST)");
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
    // Khai báo texture/sampler params (index theo thứ tự khai báo sampler)
    std::string texParams;
    for (size_t k = 0; k < R.samplers.size(); ++k) {
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
