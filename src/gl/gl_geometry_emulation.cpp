// gl_geometry_emulation.cpp — Emulation GEOMETRY SHADER (GS) cho TGLMT.
//
// Metal không có stage geometry shader. Theo quyết định A6 (docs/DECISIONS.md)
// GS BẮT BUỘC phải emulation. Thiết kế (xem docs/GEOMETRY_SHADER_EMULATION.md):
//
//   VS (compute prepass "capture")  ──► buffer capture (VS output theo vertex)
//            │
//            ▼
//   GS → MSL compute kernel          ──► buffer output Mở RỘNG (GS output)
//   (1 thread / 1 input primitive)   ──► buffer index (point/line/triangle list)
//            │
//            ▼
//   passthrough vertex (đọc buffer)  ──► fragment shader
//
// Spec: OpenGL 4.2 Compatibility §2.16 (Geometry Shaders) — bản GL 4.6 Core
// tương ứng §11.3; GLSL 3.20 §4.3.8.1/§4.3.8.2 + §8.10.
#include "tglmt/GeometryEmulation.h"
#include "tglmt/Context.h"
#include "tglmt/uniform_pack.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>

using namespace tglmt;

namespace tglmt {

const char* GsPrimTypeName(GsPrimType t) {
    switch (t) {
        case GsPrimType::Points: return "points";
        case GsPrimType::Lines: return "lines";
        case GsPrimType::LinesAdjacency: return "lines_adjacency";
        case GsPrimType::Triangles: return "triangles";
        case GsPrimType::TrianglesAdjacency: return "triangles_adjacency";
        case GsPrimType::LineStrip: return "line_strip";
        case GsPrimType::TriangleStrip: return "triangle_strip";
        default: return "unknown";
    }
}

GsPrimType GsPrimTypeFromGLSL(const std::string& kw) {
    if (kw == "points") return GsPrimType::Points;
    if (kw == "lines") return GsPrimType::Lines;
    if (kw == "lines_adjacency") return GsPrimType::LinesAdjacency;
    if (kw == "triangles") return GsPrimType::Triangles;
    if (kw == "triangles_adjacency") return GsPrimType::TrianglesAdjacency;
    if (kw == "line_strip") return GsPrimType::LineStrip;
    if (kw == "triangle_strip") return GsPrimType::TriangleStrip;
    return GsPrimType::Unknown;
}

GLenum GsPrimTypeToGL(GsPrimType t) {
    switch (t) {
        case GsPrimType::Points: return GL_POINTS;
        case GsPrimType::Lines: return GL_LINES;
        case GsPrimType::LinesAdjacency: return GL_LINES_ADJACENCY;
        case GsPrimType::Triangles: return GL_TRIANGLES;
        case GsPrimType::TrianglesAdjacency: return GL_TRIANGLES_ADJACENCY;
        case GsPrimType::LineStrip: return GL_LINE_STRIP;
        case GsPrimType::TriangleStrip: return GL_TRIANGLE_STRIP;
        default: return 0;
    }
}

namespace {

// Mã ghép primitive (tính từ mode của lệnh draw) — dùng trong kernel để map
// "vertex thứ k của primitive p" → "slot vertex trong buffer capture".
enum {
    kGsModePoints = 0,
    kGsModeLines = 1,
    kGsModeLineStrip = 2,
    kGsModeLineLoop = 3,
    kGsModeTriangles = 4,
    kGsModeTriangleStrip = 5,
    kGsModeTriangleFan = 6,
};

// Số primitive output sinh ra từ `g` vertex của MỘT primitive output.
// (OpenGL 4.2C §2.16.2: "The vertices output by the geometry shader are
//  assembled into points, lines, or triangles based on the output primitive
//  type in the manner described in section 2.6.1".)
int GsPrimsInGroup(GsPrimType out, int g) {
    switch (out) {
        case GsPrimType::Points: return g;
        case GsPrimType::LineStrip: return (g >= 2) ? g - 1 : 0;
        case GsPrimType::TriangleStrip: return (g >= 3) ? g - 2 : 0;
        default: return 0;
    }
}

// Số index tối đa một lần gọi GS có thể sinh (trường hợp emission động).
int GsMaxIndexPerInput(GsPrimType out, int maxVerts) {
    switch (out) {
        case GsPrimType::Points: return maxVerts;
        case GsPrimType::LineStrip: return (maxVerts >= 2) ? 2 * (maxVerts - 1) : 0;
        case GsPrimType::TriangleStrip: return (maxVerts >= 3) ? 3 * (maxVerts - 2) : 0;
        default: return 0;
    }
}

// max_vertices tối đa chấp nhận (GL 4.2C §2.16.3: giới hạn implementation-
// dependent; TGLMT chọn 32 để buffer mở rộng vừa sức A11).
constexpr int kGsMaxVerticesLimit = 32;
// Số input/output varying của GS tối đa (giới hạn buffer slot của kernel).
constexpr int kGsMaxInputs = 8;
constexpr int kGsMaxOutputs = 8;
// Slot buffer uniform trong kernel GS (28 — khớp capture kernel dùng 28..30 cho
// index/draw/desc nên phải chọn slot khác; 28 < 31 giới hạn Metal).
constexpr int kGsSlotUniform = 28;
// Số thread mỗi threadgroup của kernel GS (1 thread / 1 input primitive).
constexpr uint32_t kGsThreadsPerGroup = 64;

// =============================================================================
// Tiện ích chuỗi
// =============================================================================
bool IsIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
}

std::string Trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && isspace((unsigned char)s[b])) ++b;
    while (e > b && isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}

// Xoá comment (// và /* */) nhưng giữ newline (số dòng không đổi).
std::string StripGsComments(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        if (i + 1 < s.size() && s[i] == '/' && s[i + 1] == '/') {
            while (i < s.size() && s[i] != '\n') { out.push_back(' '); ++i; }
        } else if (i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) {
                out.push_back(s[i] == '\n' ? '\n' : ' ');
                ++i;
            }
            if (i + 1 < s.size()) { out += "  "; i += 2; }
        } else {
            out.push_back(s[i++]);
        }
    }
    return out;
}

// Tách danh sách theo dấu phẩy ở cấp ngoài cùng.
std::vector<std::string> SplitTopLevel(const std::string& s, char sep) {
    std::vector<std::string> out;
    int depth = 0;
    std::string cur;
    for (char c : s) {
        if (c == '(' || c == '[') ++depth;
        else if (c == ')' || c == ']') --depth;
        if (c == sep && depth == 0) { out.push_back(Trim(cur)); cur.clear(); continue; }
        cur.push_back(c);
    }
    out.push_back(Trim(cur));
    return out;
}

bool IsWordAt(const std::string& t, size_t p, const char* w) {
    size_t n = strlen(w);
    if (p + n > t.size() || t.compare(p, n, w) != 0) return false;
    if (p > 0 && IsIdentChar(t[p - 1], false)) return false;
    if (p + n < t.size() && IsIdentChar(t[p + n], false)) return false;
    return true;
}

// =============================================================================
// Kiểu GLSL → MSL
// =============================================================================
bool GsMslType(const std::string& t, std::string* out) {
    if (t == "float") { *out = "float"; return true; }
    if (t == "vec2") { *out = "float2"; return true; }
    if (t == "vec3") { *out = "float3"; return true; }
    if (t == "vec4") { *out = "float4"; return true; }
    if (t == "int") { *out = "int"; return true; }
    if (t == "ivec2") { *out = "int2"; return true; }
    if (t == "ivec3") { *out = "int3"; return true; }
    if (t == "ivec4") { *out = "int4"; return true; }
    if (t == "uint") { *out = "uint"; return true; }
    if (t == "uvec2") { *out = "uint2"; return true; }
    if (t == "uvec3") { *out = "uint3"; return true; }
    if (t == "uvec4") { *out = "uint4"; return true; }
    if (t == "bool") { *out = "bool"; return true; }
    if (t == "bvec2") { *out = "bool2"; return true; }
    if (t == "bvec3") { *out = "bool3"; return true; }
    if (t == "bvec4") { *out = "bool4"; return true; }
    return false;   // mat*/sampler*/image/double → ngoài subset
}

// Kích thước phần tử của buffer capture (mảng MSL đều đóng gói chặt).
size_t GsCaptureElemSize(const std::string& mslType) {
    if (mslType == "float" || mslType == "int" || mslType == "uint") return 4;
    if (mslType == "bool") return 1;
    if (mslType == "float2" || mslType == "int2" || mslType == "uint2") return 8;
    if (mslType == "float3" || mslType == "int3" || mslType == "uint3") return 16;
    if (mslType == "float4" || mslType == "int4" || mslType == "uint4") return 16;
    if (mslType == "bool2") return 2;
    if (mslType == "bool3" || mslType == "bool4") return 4;
    return 0;
}

// Tên biến MSL cho output field (buffer output mở rộng).
std::string GsOutVar(const std::string& name) {
    if (name == "gl_Position") return "tglmt_out_pos";
    if (name == "gl_PointSize") return "tglmt_out_psize";
    if (name == "gl_Layer") return "tglmt_out_layer";
    if (name == "gl_ViewportIndex") return "tglmt_out_vpidx";
    return "tglmt_out_" + name;
}
std::string GsCurVar(const std::string& name) {
    if (name == "gl_Position") return "tglmt_cur_pos";
    if (name == "gl_PointSize") return "tglmt_cur_psize";
    if (name == "gl_Layer") return "tglmt_cur_layer";
    if (name == "gl_ViewportIndex") return "tglmt_cur_vpidx";
    return "tglmt_cur_" + name;
}

// =============================================================================
// Phân tích thân main: số EmitVertex/EndPrimitive + nhóm primitive
// =============================================================================
struct EmitScan {
    int totalEmit = 0;
    int totalEnd = 0;
    bool dynamic = false;        // Emit/End nằm trong nhánh điều kiện / vòng lặp
    std::vector<int> groups;     // số vertex mỗi nhóm (EndPrimitive chốt nhóm)
    int curGroup = 0;
};

// Duyệt thân main: đếm EmitVertex/EndPrimitive, phát hiện chúng nằm trong
// if/for/while/switch (⇒ emission phụ thuộc dữ liệu ⇒ không tĩnh).
EmitScan ScanGsEmission(const std::string& body) {
    EmitScan st;
    std::vector<bool> blockCond;   // mỗi khối { } có nhánh điều kiện hay không
    bool pendingCond = false;
    int paren = 0;                 // độ sâu ngoặc (để ';' trong for-header không
                                   // xoá cờ điều kiện)
    size_t i = 0;
    auto conditional = [&]() {
        if (pendingCond) return true;
        return !blockCond.empty() && blockCond.back();
    };
    while (i < body.size()) {
        char c = body[i];
        if (c == '(') ++paren;
        else if (c == ')') --paren;
        if (isalpha((unsigned char)c) || c == '_') {
            size_t b = i;
            while (i < body.size() && IsIdentChar(body[i], false)) ++i;
            std::string id = body.substr(b, i - b);
            if (id == "if" || id == "else" || id == "for" || id == "while" ||
                id == "switch" || id == "do") {
                pendingCond = true;
            } else if (id == "EmitVertex") {
                ++st.totalEmit;
                ++st.curGroup;
                if (conditional()) st.dynamic = true;
            } else if (id == "EndPrimitive") {
                ++st.totalEnd;
                st.groups.push_back(st.curGroup);
                st.curGroup = 0;
                if (conditional()) st.dynamic = true;
            } else if (id == "EmitStreamVertex" || id == "EndStreamPrimitive") {
                st.dynamic = true;
            } else if (id == "return") {
                // `return` giữa chừng ⇒ số vertex emit phụ thuộc đường đi ⇒
                // không tĩnh được (an toàn: dùng đường atomic).
                st.dynamic = true;
            }
            continue;
        }
        if (c == '{') {
            bool cond = pendingCond || (!blockCond.empty() && blockCond.back());
            blockCond.push_back(cond);
            pendingCond = false;
            ++i;
            continue;
        }
        if (c == '}') {
            if (!blockCond.empty()) blockCond.pop_back();
            pendingCond = false;
            ++i;
            continue;
        }
        if (c == ';') {
            if (paren == 0) pendingCond = false;
            ++i;
            continue;
        }
        ++i;
    }
    if (st.curGroup > 0) st.groups.push_back(st.curGroup);   // nhóm chưa đóng
    return st;
}

// =============================================================================
// Parse GS: layout + khai báo + thân main
// =============================================================================
struct GsParse {
    bool ok = false;
    std::string log;
    GsPrimType inPrim = GsPrimType::Unknown;
    GsPrimType outPrim = GsPrimType::Unknown;
    int maxVertices = 0;
    int invocations = 1;
    std::vector<GsVar> inputs;
    std::vector<GsVar> outputs;
    std::vector<GsVar> uniforms;
    std::string mainBody;
};

// Tìm thân main: sau "main" mở '{' → khớp '}' tương ứng.
bool FindMainBody(const std::string& src, std::string* body, std::string* err) {
    size_t p = 0;
    while ((p = src.find("main", p)) != std::string::npos) {
        if (p > 0 && IsIdentChar(src[p - 1], false)) { p += 4; continue; }
        size_t q = p + 4;
        if (q < src.size() && IsIdentChar(src[q], false)) { p = q; continue; }
        while (q < src.size() && isspace((unsigned char)src[q])) ++q;
        if (q >= src.size() || src[q] != '(') { p = q; continue; }
        int depth = 0;
        size_t r = q;
        for (; r < src.size(); ++r) {
            if (src[r] == '(') ++depth;
            else if (src[r] == ')') { --depth; if (depth == 0) { ++r; break; } }
        }
        while (r < src.size() && isspace((unsigned char)src[r])) ++r;
        if (r >= src.size() || src[r] != '{') { p = r; continue; }
        int bdepth = 0;
        size_t s = r;
        for (; s < src.size(); ++s) {
            if (src[s] == '{') ++bdepth;
            else if (src[s] == '}') { --bdepth; if (bdepth == 0) break; }
        }
        if (s >= src.size()) { *err = "thiếu '}' đóng main"; return false; }
        *body = src.substr(r + 1, s - r - 1);
        return true;
    }
    *err = "không tìm thấy hàm main()";
    return false;
}

// Parse 1 khai báo `in`/`out`/`uniform` của GS.
bool ParseGsDecl(const std::string& decl, GsParse* out, std::string* err) {
    std::string d = Trim(decl);
    if (d.empty()) return true;
    // Bỏ qualifier phổ biến (ghi nhớ qualifier nội suy).
    std::string interp;
    const char* quals[] = {"const ", "smooth ", "flat ", "noperspective ", "centroid ",
                           "invariant ", "precise ", "highp ", "mediump ", "lowp "};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const char* q : quals) {
            size_t l = strlen(q);
            if (d.compare(0, l, q) == 0) {
                std::string w = Trim(std::string(q));
                if (w == "flat" || w == "noperspective" || w == "centroid") interp = w;
                d = Trim(d.substr(l));
                changed = true;
            }
        }
    }
    // Block `in gl_PerVertex { ... } gl_in[];` → bỏ qua (built-in có sẵn).
    if (d.find("gl_PerVertex") != std::string::npos) return true;
    // `layout(...) in;` / `layout(...) out;`
    if (d.compare(0, 6, "layout") == 0) {
        size_t lp = d.find('(');
        size_t rp = d.find(')', lp == std::string::npos ? 0 : lp);
        if (lp == std::string::npos || rp == std::string::npos) {
            *err = "layout(...) không hợp lệ: " + d;
            return false;
        }
        std::string inner = d.substr(lp + 1, rp - lp - 1);
        std::string rest = Trim(d.substr(rp + 1));
        bool isIn = (rest == "in");
        bool isOut = (rest == "out");
        if (!isIn && !isOut) {
            *err = "GS chỉ hỗ trợ layout(...) cho input/output: " + d;
            return false;
        }
        for (const auto& item : SplitTopLevel(inner, ',')) {
            std::string it = Trim(item);
            if (it.empty()) continue;
            size_t eq = it.find('=');
            if (eq == std::string::npos) {
                GsPrimType pt = GsPrimTypeFromGLSL(it);
                if (pt == GsPrimType::Unknown) {
                    *err = "layout qualifier không hợp lệ cho GS: " + it;
                    return false;
                }
                if (isIn) {
                    if (pt == GsPrimType::LineStrip || pt == GsPrimType::TriangleStrip) {
                        *err = "input primitive type của GS chỉ được points/lines/"
                               "lines_adjacency/triangles/triangles_adjacency";
                        return false;
                    }
                    if (pt == GsPrimType::LinesAdjacency ||
                        pt == GsPrimType::TrianglesAdjacency) {
                        *err = "input primitive adjacency của GS chưa hỗ trợ trong "
                               "emulation (chỉ points/lines/triangles)";
                        return false;
                    }
                    if (out->inPrim != GsPrimType::Unknown && out->inPrim != pt) {
                        *err = "GS khai báo nhiều input primitive type khác nhau";
                        return false;
                    }
                    out->inPrim = pt;
                } else {
                    if (pt != GsPrimType::Points && pt != GsPrimType::LineStrip &&
                        pt != GsPrimType::TriangleStrip) {
                        *err = "output primitive type của GS chỉ được points/"
                               "line_strip/triangle_strip (OpenGL 4.2C §2.16.2)";
                        return false;
                    }
                    if (out->outPrim != GsPrimType::Unknown && out->outPrim != pt) {
                        *err = "GS khai báo nhiều output primitive type khác nhau";
                        return false;
                    }
                    out->outPrim = pt;
                }
                continue;
            }
            std::string key = Trim(it.substr(0, eq));
            std::string val = Trim(it.substr(eq + 1));
            if (key == "max_vertices") {
                out->maxVertices = atoi(val.c_str());
            } else if (key == "invocations") {
                out->invocations = atoi(val.c_str());
            } else if (key == "vertices") {
                *err = "layout(vertices=...) không dùng cho GS";
                return false;
            } else if (key == "location" || key == "binding" || key == "std140" ||
                       key == "std430" || key == "shared" || key == "row_major" ||
                       key == "column_major" || key == "origin_upper_left" ||
                       key == "pixel_center_integer" || key == "early_fragment_tests" ||
                       key == "depth_any" || key == "depth_greater" || key == "depth_less" ||
                       key == "depth_unchanged" || key == "layout" ||
                       key == "stream" || key == "xfb_buffer" || key == "xfb_offset" ||
                       key == "xfb_stride" || key == "index" || key == "component") {
                // qualifier không ảnh hưởng emulation → bỏ qua.
            } else {
                *err = "layout qualifier chưa hỗ trợ cho GS: " + key;
                return false;
            }
        }
        return true;
    }
    // `in <type> <name>;` / `out <type> <name>;` / `uniform <type> <name>;`
    bool isIn = false, isOut = false, isUniform = false;
    size_t pos = 0;
    auto eatWord = [&](const char* w) -> bool {
        size_t l = strlen(w);
        if (d.compare(pos, l, w) == 0) {
            size_t after = pos + l;
            if (after >= d.size() || !IsIdentChar(d[after], false)) {
                pos = after;
                while (pos < d.size() && isspace((unsigned char)d[pos])) ++pos;
                return true;
            }
        }
        return false;
    };
    if (eatWord("in")) isIn = true;
    else if (eatWord("out")) isOut = true;
    else if (eatWord("uniform")) isUniform = true;
    else {
        // Không phải khai báo GS đã biết (vd helper function) → báo lỗi rõ:
        // helper function không được copy sang MSL ⇒ sẽ gọi hàm chưa định nghĩa.
        if (d.find('{') != std::string::npos || d.find('(') != std::string::npos) {
            *err = "GS chỉ hỗ trợ thân main trực tiếp (hàm phụ chưa hỗ trợ): " + d;
            return false;
        }
        return true;
    }
    // Phần còn lại: <type> <name> [ [n] ] ;
    std::string rest = d.substr(pos);
    // Bỏ mảng ở cuối.
    bool isArray = false;
    size_t br = rest.rfind('[');
    if (br != std::string::npos && !rest.empty() && rest.back() == ']') {
        isArray = true;
        rest = Trim(rest.substr(0, br));
    }
    size_t sp = rest.find_last_of(" \t");
    if (sp == std::string::npos) {
        *err = "không đọc được khai báo GS: " + d;
        return false;
    }
    std::string type = Trim(rest.substr(0, sp));
    std::string name = Trim(rest.substr(sp + 1));
    if (name.empty() || type.empty()) {
        *err = "không đọc được khai báo GS: " + d;
        return false;
    }
    std::string msl;
    if (!GsMslType(type, &msl)) {
        if (isUniform) {
            *err = "uniform của GS có kiểu chưa hỗ trợ: " + type +
                   " (sampler/image/matrix chưa hỗ trợ trong emulation GS)";
        } else {
            *err = "biến của GS có kiểu chưa hỗ trợ: " + type;
        }
        return false;
    }
    if (isArray) {
        *err = "mảng trong GS chưa hỗ trợ: " + name;
        return false;
    }
    GsVar v;
    v.name = name;
    v.glslType = type;
    v.mslType = msl;
    v.interp = interp;
    if (isIn) out->inputs.push_back(v);
    else if (isOut) out->outputs.push_back(v);
    else if (isUniform) out->uniforms.push_back(v);
    return true;
}

GsParse ParseGsSource(const std::string& rawSrc) {
    GsParse p;
    // Preprocessor: chỉ #version được chấp nhận (semantic khác ⇒ fail rõ).
    std::string src;
    {
        std::string noComment = StripGsComments(rawSrc);
        size_t pos = 0;
        while (pos <= noComment.size()) {
            size_t nl = noComment.find('\n', pos);
            std::string line = noComment.substr(
                pos, (nl == std::string::npos ? noComment.size() : nl) - pos);
            std::string t = Trim(line);
            if (!t.empty() && t[0] == '#') {
                if (t.compare(0, 8, "#version") != 0) {
                    p.log = "preprocessor directive chưa hỗ trợ trong GS: " + t;
                    return p;
                }
            } else {
                src += line;
                src += "\n";
            }
            if (nl == std::string::npos) break;
            pos = nl + 1;
        }
    }
    std::string body;
    std::string err;
    if (!FindMainBody(src, &body, &err)) { p.log = err; return p; }
    // Phần khai báo = trước main.
    size_t mainPos = src.find("main");
    std::string decls = (mainPos == std::string::npos) ? src : src.substr(0, mainPos);
    // Tách theo ';' ở depth 0.
    {
        int depth = 0;
        std::string cur;
        for (size_t i = 0; i < decls.size(); ++i) {
            char c = decls[i];
            if (c == '{' || c == '(') ++depth;
            else if (c == '}' || c == ')') --depth;
            if (c == ';' && depth == 0) {
                if (!ParseGsDecl(cur, &p, &err)) { p.log = err; return p; }
                cur.clear();
                continue;
            }
            cur.push_back(c);
        }
    }
    if (p.inPrim == GsPrimType::Unknown) {
        p.log = "GS thiếu input layout qualifier (vd layout(triangles) in;)";
        return p;
    }
    if (p.outPrim == GsPrimType::Unknown) {
        p.log = "GS thiếu output layout qualifier (vd layout(triangle_strip, max_vertices=3) out;)";
        return p;
    }
    if (p.maxVertices <= 0) {
        p.log = "GS thiếu max_vertices trong output layout qualifier";
        return p;
    }
    if (p.maxVertices > kGsMaxVerticesLimit) {
        p.log = "max_vertices = " + std::to_string(p.maxVertices) +
                " vượt ngưỡng emulation GS (" + std::to_string(kGsMaxVerticesLimit) + ")";
        return p;
    }
    if (p.invocations != 1) {
        p.log = "GS nhiều invocation (invocations=" + std::to_string(p.invocations) +
                ") chưa hỗ trợ trong emulation GS";
        return p;
    }
    if ((int)p.inputs.size() > kGsMaxInputs) {
        p.log = "GS có quá nhiều input (>" + std::to_string(kGsMaxInputs) + ")";
        return p;
    }
    if ((int)p.outputs.size() > kGsMaxOutputs) {
        p.log = "GS có quá nhiều output (>" + std::to_string(kGsMaxOutputs) + ")";
        return p;
    }
    p.mainBody = body;
    p.ok = true;
    return p;
}

// =============================================================================
// Viết lại thân GLSL → MSL
// =============================================================================
struct RewriteCtx {
    const GsTranslation* gs = nullptr;
    std::string uniformPrefix = "tglmt_u.";
    // Offset index cố định của từng lệnh EndPrimitive (trường hợp tĩnh).
    std::vector<int> groupIndexOffset;
    bool staticEmission = false;
    int primSize = 3;      // số index mỗi primitive output (points=1, line=2, tri=3)
    int inVerts = 3;       // số vertex mỗi primitive input
    int maxVerts = 3;      // max_vertices
    int tmpIndices = 96;   // kích thước mảng index tạm
};

bool IsGsUnsupportedBuiltin(const std::string& id, std::string* why) {
    if (id == "gl_ClipDistance" || id == "gl_CullDistance") {
        *why = id + " chưa hỗ trợ trong emulation GS (Metal chỉ hỗ trợ 1 clip distance)";
        return true;
    }
    if (id == "EmitStreamVertex" || id == "EndStreamPrimitive") {
        *why = "vertex stream (EmitStreamVertex) chưa hỗ trợ trong emulation GS";
        return true;
    }
    if (id == "gl_PrimitiveID") {
        *why = "gl_PrimitiveID (output của GS) chưa hỗ trợ trong emulation GS";
        return true;
    }
    if (id == "gl_VertexID" || id == "gl_InstanceID") {
        *why = id + " không tồn tại trong geometry shader";
        return true;
    }
    return false;
}

// Khối code sinh cho EmitVertex(): copy giá trị output hiện tại vào buffer mở rộng.
std::string MakeGsEmitBlock(const RewriteCtx& ctx) {
    std::string s = "{ if (tglmt_n < tglmt_p.outVertsCap) {";
    for (const auto& f : ctx.gs->outFields) {
        s += " " + GsOutVar(f.name) + "[tglmt_obase + tglmt_n] = " + GsCurVar(f.name) + ";";
    }
    s += " tglmt_n = tglmt_n + 1u; } }";
    return s;
}

// Khối code sinh cho EndPrimitive(): sinh index cho primitive output.
std::string MakeGsEndBlock(const RewriteCtx& ctx, int callIndex) {
    std::string s;
    s += "{ uint tglmt_k = 0u; uint tglmt_i = tglmt_pstart;";
    switch (ctx.gs->outPrim) {
        case GsPrimType::Points:
            s += " while (tglmt_i < tglmt_n) { tglmt_tmp[tglmt_k] = tglmt_obase + tglmt_i;"
                " tglmt_k = tglmt_k + 1u; tglmt_i = tglmt_i + 1u; }";
            break;
        case GsPrimType::LineStrip:
            s += " while (tglmt_i + 1u < tglmt_n) { tglmt_tmp[tglmt_k] = tglmt_obase + tglmt_i;"
                " tglmt_tmp[tglmt_k + 1u] = tglmt_obase + tglmt_i + 1u;"
                " tglmt_k = tglmt_k + 2u; tglmt_i = tglmt_i + 1u; }";
            break;
        default:   // TriangleStrip
            s += " while (tglmt_i + 2u < tglmt_n) { uint tglmt_t = tglmt_i - tglmt_pstart;"
                " if ((tglmt_t & 1u) == 0u) { tglmt_tmp[tglmt_k] = tglmt_obase + tglmt_i;"
                " tglmt_tmp[tglmt_k + 1u] = tglmt_obase + tglmt_i + 1u;"
                " tglmt_tmp[tglmt_k + 2u] = tglmt_obase + tglmt_i + 2u; }"
                " else { tglmt_tmp[tglmt_k] = tglmt_obase + tglmt_i + 1u;"
                " tglmt_tmp[tglmt_k + 1u] = tglmt_obase + tglmt_i;"
                " tglmt_tmp[tglmt_k + 2u] = tglmt_obase + tglmt_i + 2u; }"
                " tglmt_k = tglmt_k + 3u; tglmt_i = tglmt_i + 1u; }";
            break;
    }
    s += " if (tglmt_k > 0u) {";
    if (ctx.staticEmission) {
        int off = (callIndex >= 0 && callIndex < (int)ctx.groupIndexOffset.size())
                      ? ctx.groupIndexOffset[callIndex] : 0;
        s += " uint tglmt_base2 = tglmt_ibase + " + std::to_string(off) + "u;";
    } else {
        s += " uint tglmt_base2 = metal::atomic_fetch_add_explicit(&tglmt_cnt[0], tglmt_k,"
            " metal::memory_order_relaxed);";
    }
    s += " for (uint tglmt_j = 0u; tglmt_j < tglmt_k; ++tglmt_j)"
         " tglmt_idx[tglmt_base2 + tglmt_j] = tglmt_tmp[tglmt_j];";
    if (!ctx.staticEmission) {
        s += " metal::atomic_fetch_add_explicit(&tglmt_cnt[2], tglmt_k / " +
             std::to_string(ctx.primSize) + "u, metal::memory_order_relaxed);";
    }
    s += " }";
    s += " tglmt_pstart = tglmt_n; }";
    return s;
}

// Tìm biểu thức trong ngoặc vuông sau vị trí p (trả về text index + vị trí sau ']').
bool ExtractBracketIndex(const std::string& s, size_t p, std::string* idx, size_t* after) {
    while (p < s.size() && isspace((unsigned char)s[p])) ++p;
    if (p >= s.size() || s[p] != '[') return false;
    int depth = 0;
    size_t k = p;
    for (; k < s.size(); ++k) {
        if (s[k] == '[') ++depth;
        else if (s[k] == ']') { --depth; if (depth == 0) break; }
    }
    if (k >= s.size()) return false;
    *idx = Trim(s.substr(p + 1, k - p - 1));
    *after = k + 1;
    return true;
}

// Biểu thức index của mảng vào buffer capture.
std::string GsCaptureIndex(const std::string& idx) {
    return "tglmt_base + tglmt_slotof(tglmt_prim, (" + idx +
           "), tglmt_p.mode, tglmt_p.vertsPerInst, tglmt_p.inVerts)";
}

std::string RewriteGsBody(const std::string& body, const RewriteCtx& ctx, std::string* err) {
    std::string out;
    out.reserve(body.size() + 512);
    size_t i = 0;
    int endCall = 0;
    while (i < body.size()) {
        char c = body[i];
        if (isalpha((unsigned char)c) || c == '_') {
            size_t b = i;
            while (i < body.size() && IsIdentChar(body[i], false)) ++i;
            std::string id = body.substr(b, i - b);
            // --- gl_in[i].member ---
            if (id == "gl_in") {
                std::string idx;
                size_t after = 0;
                if (!ExtractBracketIndex(body, i, &idx, &after)) {
                    *err = "gl_in phải được truy cập dạng gl_in[i].member";
                    return out;
                }
                size_t m = after;
                while (m < body.size() && isspace((unsigned char)body[m])) ++m;
                if (m >= body.size() || body[m] != '.') {
                    *err = "gl_in[...] phải được truy cập dạng gl_in[i].member";
                    return out;
                }
                ++m;
                size_t mb = m;
                while (m < body.size() && IsIdentChar(body[m], false)) ++m;
                std::string member = body.substr(mb, m - mb);
                if (member != "gl_Position" && member != "gl_PointSize") {
                    *err = "gl_in[]." + member + " chưa hỗ trợ trong emulation GS";
                    return out;
                }
                out += "tglmt_cap_" + member + "[" + GsCaptureIndex(idx) + "]";
                i = m;
                continue;
            }
            // --- EmitVertex() / EndPrimitive() ---
            if (id == "EmitVertex" || id == "EndPrimitive") {
                size_t j = i;
                while (j < body.size() && isspace((unsigned char)body[j])) ++j;
                if (j + 1 < body.size() && body[j] == '(' && body[j + 1] == ')') {
                    i = j + 2;
                }
                if (id == "EmitVertex") {
                    out += MakeGsEmitBlock(ctx);
                } else {
                    out += MakeGsEndBlock(ctx, endCall);
                    ++endCall;
                }
                continue;
            }
            std::string why;
            if (IsGsUnsupportedBuiltin(id, &why)) { *err = why; return out; }
            // --- built-in output của GS ---
            if (id == "gl_Position") { out += GsCurVar(id); continue; }
            if (id == "gl_PointSize") { out += GsCurVar(id); continue; }
            if (id == "gl_Layer") { out += GsCurVar(id); continue; }
            if (id == "gl_ViewportIndex") { out += GsCurVar(id); continue; }
            // --- built-in input của GS ---
            if (id == "gl_PrimitiveIDIn") { out += "tglmt_primid"; continue; }
            if (id == "gl_InvocationID") { out += "tglmt_inv"; continue; }
            // --- kiểu GLSL → kiểu MSL (biến cục bộ trong thân main) ---
            {
                std::string msl;
                if (GsMslType(id, &msl)) { out += msl; continue; }
            }
            // --- input của GS (mảng per-vertex) ---
            bool matched = false;
            if (ctx.gs) {
                for (const auto& v : ctx.gs->inputs) {
                    if (v.name != id) continue;
                    std::string idx;
                    size_t after = 0;
                    if (!ExtractBracketIndex(body, i, &idx, &after)) {
                        *err = "input của GS phải được truy cập dạng " + id + "[i]";
                        return out;
                    }
                    out += "tglmt_cap_" + id + "[" + GsCaptureIndex(idx) + "]";
                    i = after;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            // --- output của GS ---
            if (ctx.gs) {
                for (const auto& v : ctx.gs->outputs) {
                    if (v.name != id) continue;
                    out += GsCurVar(id);
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            // --- uniform của GS ---
            if (ctx.gs) {
                for (const auto& v : ctx.gs->uniforms) {
                    if (v.name != id) continue;
                    out += ctx.uniformPrefix + id;
                    matched = true;
                    break;
                }
            }
            if (matched) continue;
            out += id;
            continue;
        }
        out.push_back(c);
        ++i;
    }
    return out;
}

// =============================================================================
// Sinh MSL
// =============================================================================
// Hàm file-scope: "vertex thứ k của primitive p" → "slot vertex trong capture".
std::string BuildGsSlotFn() {
    return
        "inline uint tglmt_slotof(uint tglmt_prim, uint tglmt_k, uint tglmt_mode,\n"
        "                      uint tglmt_n, uint tglmt_in) {\n"
        "  uint tglmt_s = 0u;\n"
        "  switch (tglmt_mode) {\n"
        "    case 0u: tglmt_s = tglmt_prim; break;\n"                       // POINTS
        "    case 1u: tglmt_s = tglmt_prim * 2u + tglmt_k; break;\n"         // LINES
        "    case 2u: tglmt_s = tglmt_prim + tglmt_k; break;\n"              // LINE_STRIP
        "    case 3u: tglmt_s = (tglmt_prim + tglmt_k) % tglmt_n; break;\n" // LINE_LOOP
        "    case 4u: tglmt_s = tglmt_prim * 3u + tglmt_k; break;\n"         // TRIANGLES
        "    case 5u: tglmt_s = tglmt_prim + tglmt_k; break;\n"              // TRIANGLE_STRIP
        "    default: tglmt_s = (tglmt_k == 0u) ? 0u : (tglmt_prim + tglmt_k); break;\n"
        "  }\n"
        "  return (tglmt_k < tglmt_in) ? tglmt_s : (tglmt_in - 1u);\n"
        "}\n";
}

// Danh sách binding của kernel: VS output (varying) → buffer capture.
// Trả false + log nếu input của GS không có VS output tương ứng (GL: GS đọc
// output của VS; thiếu ⇒ link error).
bool GsNeededBindings(const GsTranslation& gs, const std::vector<GsVar>& vsOutputs,
                      std::vector<GsVar>* out, std::string* err) {
    out->clear();
    GsVar pos;
    pos.name = "gl_Position";
    pos.glslType = "vec4";
    pos.mslType = "float4";
    out->push_back(pos);
    GsVar ps;
    ps.name = "gl_PointSize";
    ps.glslType = "float";
    ps.mslType = "float";
    out->push_back(ps);
    for (const auto& in : gs.inputs) {
        GsVar v;
        v.name = in.name;
        v.glslType = in.glslType;
        v.mslType = in.mslType;
        bool found = false;
        for (const auto& o : vsOutputs) {
            if (o.name != in.name) continue;
            found = true;
            if (o.mslType != in.mslType) {
                *err = "type mismatch GS input `" + in.name + "`: GS khai " + in.glslType +
                       " nhưng VS output là " + o.glslType;
                return false;
            }
            break;
        }
        if (!found) {
            *err = "GS input `" + in.name + "` không có vertex shader output tương ứng";
            return false;
        }
        out->push_back(v);
    }
    return true;
}

} // namespace

// =============================================================================
// TranslateGeometryShader
// =============================================================================
GsTranslation TranslateGeometryShader(const std::string& glsl, const std::string& uniformStructMsl,
                                      const std::vector<GsVar>& vsOutputs,
                                      bool interfaceCheck) {
    GsTranslation t;
    GsParse p = ParseGsSource(glsl);
    if (!p.ok) { t.log = p.log; return t; }
    t.inPrim = p.inPrim;
    t.outPrim = p.outPrim;
    t.maxVertices = p.maxVertices;
    t.invocations = p.invocations;
    t.inputs = p.inputs;
    t.outputs = p.outputs;
    t.uniforms = p.uniforms;
    // Field output mở rộng: built-in trước, rồi output của user.
    {
        GsOutField f;
        f.name = "gl_Position"; f.mslType = "float4"; f.elemSize = 16;
        t.outFields.push_back(f);
        f.name = "gl_PointSize"; f.mslType = "float"; f.elemSize = 4;
        t.outFields.push_back(f);
        f.name = "gl_Layer"; f.mslType = "uint"; f.elemSize = 4;
        t.outFields.push_back(f);
        f.name = "gl_ViewportIndex"; f.mslType = "uint"; f.elemSize = 4;
        t.outFields.push_back(f);
        for (const auto& o : t.outputs) {
            GsOutField uf;
            uf.name = o.name;
            uf.mslType = o.mslType;
            uf.elemSize = GsCaptureElemSize(o.mslType);
            uf.isUser = true;
            if (!uf.elemSize) {
                t.log = "output của GS có kiểu chưa hỗ trợ: " + o.glslType;
                return t;
            }
            t.outFields.push_back(uf);
        }
    }
    // Phân tích tĩnh emission.
    EmitScan sc = ScanGsEmission(p.mainBody);
    t.emitCalls = sc.totalEmit;
    t.endCalls = sc.totalEnd;
    t.staticEmission = !sc.dynamic;
    // Số index mỗi primitive output.
    int primSize = GsIndicesPerPrim(t.outPrim);
    if (primSize <= 0) {
        t.log = "output primitive type không hỗ trợ";
        return t;
    }
    t.maxIndexPerInput = GsMaxIndexPerInput(t.outPrim, t.maxVertices);
    if (t.staticEmission) {
        t.vertsPerInput = sc.totalEmit;
        int prims = 0;
        int idx = 0;
        for (int g : sc.groups) {
            int np = GsPrimsInGroup(t.outPrim, g);
            prims += np;
            idx += np * primSize;
        }
        t.primsPerInput = prims;
        t.staticIndexPerPrim = idx;
        // max_vertices giới hạn SỐ VERTEX emit trong MỘT lần gọi GS
        // (GLSL 3.20 §4.3.8.2) — vượt ⇒ undefined ⇒ báo lỗi ở link.
        if (t.vertsPerInput > t.maxVertices) {
            t.log = "GS emit " + std::to_string(t.vertsPerInput) +
                    " vertex/lần gọi vượt max_vertices = " + std::to_string(t.maxVertices) +
                    " (GLSL 3.20 §4.3.8.2: hành vi undefined)";
            return t;
        }
        if (sc.totalEmit == 0) {
            t.log = "GS không gọi EmitVertex() — không có vertex nào được sinh";
            return t;
        }
    }
    // Offset index cố định cho từng EndPrimitive (trường hợp tĩnh).
    RewriteCtx ctx;
    ctx.gs = &t;
    ctx.staticEmission = t.staticEmission;
    ctx.primSize = primSize;
    ctx.inVerts = GsPrimInputVertices(t.inPrim);
    ctx.maxVerts = t.maxVertices;
    ctx.tmpIndices = (t.maxIndexPerInput > 0 ? t.maxIndexPerInput : 3 * t.maxVertices) + 4;
    if (t.staticEmission) {
        int off = 0;
        for (int g : sc.groups) {
            ctx.groupIndexOffset.push_back(off);
            off += GsPrimsInGroup(t.outPrim, g) * primSize;
        }
        // Offset cho EndPrimitive IMPLICIT cuối shader (nhóm không đóng bằng
        // EndPrimitive tường minh).
        ctx.groupIndexOffset.push_back(off);
    }
    std::string err;
    std::string body = RewriteGsBody(p.mainBody, ctx, &err);
    if (!err.empty()) { t.log = err; return t; }
    // EndPrimitive IMPLICIT: "When a geometry shader terminates, the current
    // output primitive is automatically completed." (GLSL 3.20 §8.10) — GS không
    // gọi EndPrimitive() vẫn phải sinh primitive cuối.
    body += MakeGsEndBlock(ctx, sc.totalEnd);
    // Binding buffer của kernel.
    std::string bindErr;
    if (interfaceCheck) {
        if (!GsNeededBindings(t, vsOutputs, &t.bindings, &bindErr)) {
            t.log = bindErr;
            return t;
        }
    } else {
        // Compile-time: chưa biết output của VS ⇒ bỏ kiểm tra binding.
        t.bindings.clear();
    }
    const std::vector<GsVar>& bindings = t.bindings;
    // ---- Sinh MSL kernel ----
    int slot = 0;
    std::string msl;
    if (!t.uniforms.empty()) {
        if (uniformStructMsl.empty()) {
            if (interfaceCheck) {
                t.log = "GS dùng uniform nhưng vertex shader không có struct "
                        "TGLMTUniforms (không thể dùng chung uniform buffer với VS)";
                return t;
            }
            // Compile-time: struct rỗng là đủ để kiểm tra cú pháp.
            msl += "struct TGLMTUniforms { };\n";
        } else {
            msl += uniformStructMsl;
            msl += "\n";
        }
    }
    msl += BuildGsSlotFn();
    msl += "struct TGLMT_GsParams {\n"
           "  uint primsPerInst;\n"
           "  uint vertsPerInst;\n"
           "  uint inVerts;\n"
           "  uint maxVerts;\n"
           "  uint outVertsCap;\n"
           "  uint idxPerPrim;\n"
           "  uint instances;\n"
           "  uint mode;\n"
           "};\n";
    msl += "kernel void TGLMT_gs(\n    uint tglmt_tid [[thread_position_in_grid]],\n";
    for (size_t i = 0; i < bindings.size(); ++i) {
        msl += "    device const " + bindings[i].mslType + "* tglmt_cap_" + bindings[i].name +
               " [[buffer(" + std::to_string(slot++) + ")]],\n";
    }
    msl += "    constant TGLMT_GsParams& tglmt_p [[buffer(" + std::to_string(slot++) + ")]],\n";
    for (const auto& f : t.outFields) {
        msl += "    device " + f.mslType + "* " + GsOutVar(f.name) +
               " [[buffer(" + std::to_string(slot++) + ")]],\n";
    }
    msl += "    device uint* tglmt_idx [[buffer(" + std::to_string(slot++) + ")]],\n";
    msl += "    device metal::atomic_uint* tglmt_cnt [[buffer(" + std::to_string(slot++) + ")]]";
    if (!t.uniforms.empty()) {
        msl += ",\n    constant TGLMTUniforms& tglmt_u [[buffer(" +
               std::to_string(kGsSlotUniform) + ")]]";
    }
    msl += ") {\n";
    msl += "  const uint tglmt_prim = tglmt_tid % tglmt_p.primsPerInst;\n"
           "  const uint tglmt_inst = tglmt_tid / tglmt_p.primsPerInst;\n"
           // gl_PrimitiveIDIn = số primitive đã xử lý kể từ Begin (OpenGL 4.2C
           // §2.16.4): đếm liên tiếp qua các instance của cùng lệnh draw.
           "  const uint tglmt_primid = tglmt_inst * tglmt_p.primsPerInst + tglmt_prim;\n"
           "  const uint tglmt_inv = 0u;\n"
           "  const uint tglmt_base = tglmt_inst * tglmt_p.vertsPerInst;\n"
           "  const uint tglmt_obase = tglmt_tid * tglmt_p.outVertsCap;\n"
           "  const uint tglmt_ibase = tglmt_tid * tglmt_p.idxPerPrim;\n"
           "  uint tglmt_n = 0u;\n"
           "  uint tglmt_pstart = 0u;\n";
    msl += "  uint tglmt_tmp[" + std::to_string(ctx.tmpIndices) + "];\n";
    // Giá trị hiện tại của các output ( EmitVertex copy sang buffer ).
    msl += "  float4 tglmt_cur_pos = float4(0.0f, 0.0f, 0.0f, 1.0f);\n"
           "  float tglmt_cur_psize = 1.0f;\n"
           "  uint tglmt_cur_layer = 0u;\n"
           "  uint tglmt_cur_vpidx = 0u;\n";
    for (const auto& o : t.outputs) {
        std::string zero = "0.0f";
        if (o.mslType == "int" || o.mslType == "uint") zero = "0u";
        else if (o.mslType == "bool") zero = "false";
        else if (o.mslType.size() > 4 && o.mslType.back() == '2') zero = o.mslType + "(0.0f)";
        else if (o.mslType.size() > 4 && o.mslType.back() == '3') zero = o.mslType + "(0.0f)";
        else if (o.mslType.size() > 4 && o.mslType.back() == '4') zero = o.mslType + "(0.0f)";
        msl += "  " + o.mslType + " " + GsCurVar(o.name) + " = " + zero + ";\n";
    }
    msl += body;
    msl += "\n}\n";
    t.msl = msl;
    t.ok = true;
    return t;
}

// =============================================================================
// Passthrough vertex: đọc buffer GS output → trả varying cho FS
// =============================================================================
// Attribute nội suy MSL cho qualifier GLSL (khớp converter: src/gl/glsl_to_msl.cpp).
static std::string GsInterpAttr(const std::string& interp) {
    if (interp == "flat") return " [[flat]]";
    if (interp == "noperspective") return " [[center_no_perspective]]";
    if (interp == "centroid") return " [[center]]";
    return "";
}

std::string BuildGeometryPassthroughMsl(const GsTranslation& gs,
                                        const std::vector<GsFsField>& fsFields) {
    std::string msl;
    msl += "struct TGLMT_GsVOut {\n";
    msl += "  float4 gl_Position [[position]];\n";
    for (const auto& f : fsFields) {
        msl += "  " + f.mslType + " " + f.name + GsInterpAttr(f.interp) + " [[user(locn" +
               std::to_string(f.location) + ")]];\n";
    }
    msl += "};\n";
    msl += "vertex TGLMT_GsVOut TGLMT_gs_vs(uint tglmt_vid [[vertex_id]],\n";
    int slot = 0;
    msl += "                                 device const float4* tglmt_out_pos [[buffer(" +
           std::to_string(slot++) + ")]]";
    for (const auto& f : fsFields) {
        msl += ",\n                                 device const " + f.mslType + "* " +
               GsOutVar(f.name) + " [[buffer(" + std::to_string(slot++) + ")]]";
    }
    msl += ") {\n  TGLMT_GsVOut tglmt_o = {};\n"
           "  tglmt_o.gl_Position = tglmt_out_pos[tglmt_vid];\n";
    for (const auto& f : fsFields) {
        msl += "  tglmt_o." + f.name + " = " + GsOutVar(f.name) + "[tglmt_vid];\n";
    }
    msl += "  return tglmt_o;\n}\n";
    return msl;
}

// =============================================================================
// PlanGeometryExpansion
// =============================================================================
GsPlan PlanGeometryExpansion(const GsTranslation& gs, GLenum drawMode, GLsizei count,
                             GLsizei inst) {
    GsPlan plan;
    if (count <= 0 || inst <= 0) return plan;
    // Số primitive input + kiểm tra tương thích mode (OpenGL 4.2C §2.16.1).
    uint64_t prims = 0;
    bool compatible = false;
    uint32_t mode = kGsModeTriangles;
    switch (drawMode) {
        case GL_POINTS:
            prims = (uint64_t)count;
            compatible = (gs.inPrim == GsPrimType::Points);
            mode = kGsModePoints;
            break;
        case GL_LINES:
            prims = (uint64_t)count / 2u;
            compatible = (gs.inPrim == GsPrimType::Lines);
            mode = kGsModeLines;
            break;
        case GL_LINE_STRIP:
            prims = (count >= 2) ? (uint64_t)(count - 1) : 0u;
            compatible = (gs.inPrim == GsPrimType::Lines);
            mode = kGsModeLineStrip;
            break;
        case GL_LINE_LOOP:
            // Line loop = line strip + đoạn đóng (OpenGL 4.2C §2.6.1).
            prims = (count >= 2) ? (uint64_t)count : 0u;
            compatible = (gs.inPrim == GsPrimType::Lines);
            mode = kGsModeLineLoop;
            break;
        case GL_TRIANGLES:
            prims = (uint64_t)count / 3u;
            compatible = (gs.inPrim == GsPrimType::Triangles);
            mode = kGsModeTriangles;
            break;
        case GL_TRIANGLE_STRIP:
            prims = (count >= 3) ? (uint64_t)(count - 2) : 0u;
            compatible = (gs.inPrim == GsPrimType::Triangles);
            mode = kGsModeTriangleStrip;
            break;
        case GL_TRIANGLE_FAN:
            prims = (count >= 3) ? (uint64_t)(count - 2) : 0u;
            compatible = (gs.inPrim == GsPrimType::Triangles);
            mode = kGsModeTriangleFan;
            break;
        default:
            return plan;   // mode không hợp lệ (đã bị validate ở EmitDraw)
    }
    plan.modeCompatible = compatible;
    if (!compatible) return plan;
    plan.mode = mode;
    plan.inputPrimitives = prims;
    plan.invocations = prims * (uint64_t)inst * (uint64_t)gs.invocations;
    if (gs.staticEmission) {
        plan.outputVertices = prims * (uint64_t)inst * (uint64_t)gs.vertsPerInput;
        plan.outputPrimitives = prims * (uint64_t)inst * (uint64_t)gs.primsPerInput;
        plan.exact = true;
    } else {
        // Emission phụ thuộc dữ liệu: chỉ biết trên GPU (đọc counter sau pass).
        plan.dynamic = true;
        plan.outputVertices = prims * (uint64_t)inst * (uint64_t)gs.maxVertices;
        plan.outputPrimitives = 0;
    }
    return plan;
}

// =============================================================================
// Side-table program GS
// =============================================================================
namespace gl {
namespace {
std::mutex& GsStoreMutex() {
    static std::mutex m;
    return m;
}
std::map<GLuint, GsProgramInfo>& GsStore() {
    static std::map<GLuint, GsProgramInfo> s;
    return s;
}
} // namespace

void StoreGeometryProgramInfo(const GsProgramInfo& info) {
    std::lock_guard<std::mutex> lk(GsStoreMutex());
    GsStore()[info.program] = info;
}

bool GetGeometryProgramInfo(GLuint program, uint64_t linkGen, GsProgramInfo* out) {
    std::lock_guard<std::mutex> lk(GsStoreMutex());
    auto it = GsStore().find(program);
    if (it == GsStore().end()) return false;
    if (it->second.linkGen != linkGen) return false;
    if (out) *out = it->second;
    return true;
}

void DropGeometryProgramInfo(GLuint program) {
    std::lock_guard<std::mutex> lk(GsStoreMutex());
    GsStore().erase(program);
}

bool GeometryStageActive(Context& c) {
    GLuint prog = c.state.BoundProgram();
    if (!prog) return false;
    auto it = c.programs.find(prog);
    if (it == c.programs.end()) return false;
    // Chỉ khi link THÀNH CÔNG: link fail có thể để lại side-table của lần
    // link trước (cùng linkGen) ⇒ không được dùng.
    return it->second.linked && it->second.hasGeometryStage;
}

namespace {
// Layout buffer cho kernel capture — PHẢI khớp TGLMTAttrDesc trong MSL
// (converter sinh ra, xem src/gl/glsl_to_msl.cpp).
struct GsAttrDesc {
    uint32_t type, size, normalized, stride, offset, binding, divisor, pad;
    float current[4];
};
// PHẢI khớp TGLMTDrawInfo trong MSL.
struct GsDrawInfo {
    uint32_t indexed, indexType;
    int32_t first, baseVertex;
    uint32_t count, baseInstance, instanceCount, pad1;
};

// Buffer uniform stage-local (layout giống draw path / tess path).
metal::IBuffer* MakeGsUniformBuffer(Context& c, ProgramObject& pr,
                                    const std::vector<ProgramObject::StageUniform>& list,
                                    size_t bytes,
                                    std::vector<std::shared_ptr<metal::IBuffer>>& keep) {
    size_t sz = std::max<size_t>(16, (bytes + 15u) & ~size_t(15));
    std::vector<uint8_t> data(sz, 0);
    for (auto& e : list) {
        auto lit = pr.uniformLoc.find(e.name);
        if (lit == pr.uniformLoc.end()) continue;
        auto uit = pr.uniforms.find(lit->second);
        if (uit == pr.uniforms.end() || uit->second.bytes.empty()) continue;
        int mcols = 0, mrows = 0;
        if (!e.glslType.empty() && GlslMatrixDims(e.glslType, mcols, mrows)) {
            const size_t tight = (size_t)mcols * (size_t)mrows * sizeof(float);
            const size_t mslMat = MslMatrixSize(e.glslType);
            const size_t cnt = (e.arraySize > 0) ? (size_t)e.arraySize : 1;
            if (e.offset + mslMat * cnt > data.size()) continue;
            if (uit->second.bytes.size() < tight * cnt) continue;
            for (size_t i = 0; i < cnt; ++i)
                PackGlslMatrixToMsl(e.glslType, uit->second.bytes.data() + i * tight, tight,
                                    data.data() + e.offset + i * mslMat, mslMat);
            continue;
        }
        size_t n = std::min(e.size, uit->second.bytes.size());
        if (e.offset + n <= data.size())
            memcpy(data.data() + e.offset, uit->second.bytes.data(), n);
    }
    auto b = c.device->newBufferWithBytes(data.data(), data.size(), metal::StorageMode::Shared);
    if (!b) return nullptr;
    keep.push_back(b);
    return b.get();
}

uint32_t GsGlTypeSize(GLenum t) {
    switch (t) {
        case 0x1400: case 0x1401: return 1;
        case 0x1402: case 0x1403: case 0x140B: return 2;
        case 0x1404: case 0x1405: case 0x1406: case 0x140C: return 4;
        default: return 0;
    }
}
} // namespace

bool GeometryStageDraw(Context& c, GLenum mode, GLsizei count, GLsizei inst, bool indexed,
                       GLenum indexType, const void* indexData, size_t indexByteOff,
                       GLuint eboId, GLint baseVertex, GLuint baseInstance, GLint first) {
    GLuint prog = c.state.BoundProgram();
    auto pit = c.programs.find(prog);
    if (pit == c.programs.end() || !pit->second.linked) return false;
    ProgramObject& pr = pit->second;
    GsProgramInfo info;
    if (!GetGeometryProgramInfo(prog, pr.linkGen, &info)) return false;
    const GsTranslation& gs = info.gs;
    GsPlan plan = PlanGeometryExpansion(gs, mode, count, inst);
    if (!plan.modeCompatible) {
        // OpenGL 4.2C §2.16.1: INVALID_OPERATION khi mode không tương thích
        // với input primitive type của GS.
        c.errors.Record(GL_INVALID_OPERATION);
        return false;
    }
    // Counter query (OpenGL 4.2C §2.16 / ARB_pipeline_statistics_query).
    c.pipeStats.gsInvocations += plan.invocations;
    if (plan.exact) c.pipeStats.gsPrimsWritten += plan.outputPrimitives;
    if (!c.device || c.device->isNull()) return false;   // backend test: không GPU
    if (!info.capPipe || !info.gsPipe || !info.passLib || !info.fsLib) return false;
    const uint32_t primsPerInst = (uint32_t)plan.inputPrimitives;
    const uint32_t totalPrims = primsPerInst * (uint32_t)inst;
    if (totalPrims == 0) return true;

    std::vector<std::shared_ptr<metal::IBuffer>> keep;
    auto alloc = [&](size_t bytes) -> metal::IBuffer* {
        auto b = c.device->newBuffer(bytes ? bytes : 16, metal::StorageMode::Shared);
        if (!b) return nullptr;
        keep.push_back(b);
        return b.get();
    };
    // Thứ tự GL: draw đang encode phải commit trước compute pass.
    c.FlushPendingEncoder();
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();

    // ---- 1. VS capture prepass: chạy VS thật (compute) → buffer VS output ----
    std::map<std::string, metal::IBuffer*> captureByName;
    metal::IBuffer* vsUB = MakeGsUniformBuffer(c, pr, pr.vsUniforms, pr.vsUniformBytes, keep);
    if (!vsUB) return false;
    {
        if (pr.vsCaptureSlots.empty()) return false;
        if (pr.vsCaptureIndexSlot < 0 || pr.vsCaptureDrawSlot < 0 || pr.vsCaptureDescSlot < 0)
            return false;
        GLuint vao = c.state.BoundVAO();
        auto vaoIt = c.vaos.find(vao);
        if (vaoIt == c.vaos.end()) return false;
        VertexArrayObject& v = vaoIt->second;
        GsAttrDesc desc[16] = {};
        metal::IBuffer* bindBuf[16] = {};
        size_t bindOff[16] = {};
        for (int i = 0; i < 16; ++i) {
            const VertexAttrib& a = v.attribs[i];
            if (!a.enabled) continue;
            auto bit = c.buffers.find(a.buffer);
            if (bit == c.buffers.end() || !bit->second.gpu) return false;
            uint32_t ts = GsGlTypeSize(a.type);
            if (!ts) return false;
            size_t base = 0;
            if (a.binding < v.bindings.size() && v.bindings[a.binding].offset > 0)
                base = (size_t)v.bindings[a.binding].offset;
            uint32_t stride = (uint32_t)a.stride;
            if (!stride) stride = (uint32_t)a.size * ts;
            GLuint divisor = a.divisor;
            if (!divisor && a.binding < v.bindings.size()) divisor = v.bindings[a.binding].divisor;
            desc[i] = {a.type, (uint32_t)a.size, (uint32_t)(a.normalized ? 1 : 0), stride,
                       (uint32_t)a.relativeOffset, a.binding, divisor, 0,
                       {0.0f, 0.0f, 0.0f, 1.0f}};
            if (a.binding < 16 && !bindBuf[a.binding]) {
                bindBuf[a.binding] = bit->second.gpu.get();
                bindOff[a.binding] = base;
            }
        }
        for (auto& fa : pr.attribFill) {
            if (fa.loc < 0 || fa.loc > 15) continue;
            if (v.attribs[fa.loc].enabled) continue;
            double cur[4] = {0.0, 0.0, 0.0, 1.0};
            c.state.GetShadow(0x8626 /*CURRENT_VERTEX_ATTRIB*/, cur, sizeof(cur), fa.loc);
            desc[fa.loc].type = fa.cls == 0 ? 0x1406 : (fa.cls == 1 ? 0x1404 : 0x1405);
            desc[fa.loc].size = fa.comps;
            desc[fa.loc].binding = 0xFFFFFFFFu;
            desc[fa.loc].current[0] = (float)cur[0];
            desc[fa.loc].current[1] = (float)cur[1];
            desc[fa.loc].current[2] = (float)cur[2];
            desc[fa.loc].current[3] = (float)cur[3];
        }
        metal::IBuffer* zero = alloc(16);
        if (!zero) return false;
        for (int b = 0; b < 16; ++b)
            if (!bindBuf[b]) { bindBuf[b] = zero; bindOff[b] = 0; }
        auto descSp = c.device->newBufferWithBytes(desc, sizeof(desc), metal::StorageMode::Shared);
        if (!descSp) return false;
        keep.push_back(descSp);
        GsDrawInfo di{};
        di.indexed = indexed ? 1u : 0u;
        di.indexType = indexType;
        di.first = (int32_t)first;
        di.baseVertex = (int32_t)baseVertex;
        di.count = (uint32_t)count;
        di.baseInstance = (uint32_t)baseInstance;
        di.instanceCount = (uint32_t)inst;
        auto drawSp = c.device->newBufferWithBytes(&di, sizeof(di), metal::StorageMode::Shared);
        if (!drawSp) return false;
        keep.push_back(drawSp);
        metal::IBuffer* indexBuf = zero;
        size_t indexOff = 0;
        if (indexed) {
            if (indexType != 0x1403 && indexType != 0x1405) return false;
            if (eboId) {
                auto bit = c.buffers.find(eboId);
                if (bit == c.buffers.end() || !bit->second.gpu) return false;
                indexBuf = bit->second.gpu.get();
                indexOff = indexByteOff;
            } else {
                if (!indexData) return false;
                size_t elem = (indexType == 0x1403) ? 2 : 4;
                auto ib = c.device->newBufferWithBytes(indexData, (size_t)count * elem,
                                                       metal::StorageMode::Shared);
                if (!ib) return false;
                keep.push_back(ib);
                indexBuf = ib.get();
            }
        }
        const uint32_t totalVerts = (uint32_t)count * (uint32_t)inst;
        for (auto& s : pr.vsCaptureSlots) {
            size_t es = GsCaptureElemSize(s.type);
            if (!es) return false;
            // Varying dạng mảng: capture ghi `cap[tid * n + i]` ⇒ buffer phải
            // đủ totalVerts * n phần tử.
            size_t per = (s.arraySize > 0) ? (size_t)s.arraySize : 1u;
            metal::IBuffer* b = alloc((size_t)totalVerts * per * es);
            if (!b) return false;
            captureByName[s.name] = b;
        }
        auto ce = c.device->makeComputeEncoder();
        if (!ce) return false;
        ce->setPipeline(info.capPipe.get());
        for (int b = 0; b < 16; ++b) ce->setBuffer(bindBuf[b], (uint32_t)b, bindOff[b]);
        ce->setBuffer(vsUB, (uint32_t)16);
        for (auto& s : pr.vsCaptureSlots) {
            auto it = captureByName.find(s.name);
            if (it == captureByName.end()) return false;
            ce->setBuffer(it->second, (uint32_t)s.slot);
        }
        ce->setBuffer(indexBuf, (uint32_t)pr.vsCaptureIndexSlot, indexOff);
        ce->setBuffer(drawSp.get(), (uint32_t)pr.vsCaptureDrawSlot);
        ce->setBuffer(descSp.get(), (uint32_t)pr.vsCaptureDescSlot);
        uint32_t groups = (totalVerts + kGsThreadsPerGroup - 1u) / kGsThreadsPerGroup;
        ce->dispatchGroups3D(groups, 1, 1, kGsThreadsPerGroup, 1, 1);
        if (!ce->endAndCommit()) return false;
    }

    // ---- 2. GS kernel: input primitive → vertex mở rộng + index ----
    const uint32_t inVerts = (uint32_t)GsPrimInputVertices(gs.inPrim);
    const uint32_t outVertsCap = gs.staticEmission
                                     ? (uint32_t)std::max(1, gs.vertsPerInput)
                                     : (uint32_t)gs.maxVertices;
    const int primSize = GsIndicesPerPrim(gs.outPrim);
    uint32_t idxPerPrim = 0;
    if (gs.staticEmission) {
        idxPerPrim = (uint32_t)std::max(0, gs.staticIndexPerPrim);
    }
    const uint32_t idxCapacity = gs.staticEmission
                                     ? (uint32_t)std::max(1u, idxPerPrim)
                                     : (uint32_t)std::max(1, gs.maxIndexPerInput);
    // Buffer output mở rộng: mỗi field 1 buffer (không cần layout struct MSL).
    std::map<std::string, metal::IBuffer*> outByName;
    for (const auto& f : gs.outFields) {
        size_t bytes = (size_t)totalPrims * outVertsCap * f.elemSize;
        metal::IBuffer* b = alloc(bytes);
        if (!b) return false;
        outByName[f.name] = b;
    }
    metal::IBuffer* gsIdx = alloc((size_t)totalPrims * idxCapacity * 4u);
    if (!gsIdx) return false;
    uint32_t counters[4] = {0, 0, 0, 0};
    auto cntSp = c.device->newBufferWithBytes(counters, sizeof(counters),
                                               metal::StorageMode::Shared);
    if (!cntSp) return false;
    keep.push_back(cntSp);
    {
        struct GsParams {
            uint32_t primsPerInst, vertsPerInst, inVerts, maxVerts;
            uint32_t outVertsCap, idxPerPrim, instances, mode;
        } params;
        params.primsPerInst = primsPerInst;
        params.vertsPerInst = (uint32_t)count;
        params.inVerts = inVerts;
        params.maxVerts = (uint32_t)gs.maxVertices;
        params.outVertsCap = outVertsCap;
        params.idxPerPrim = idxPerPrim;
        params.instances = (uint32_t)inst;
        params.mode = plan.mode;
        auto paramSp = c.device->newBufferWithBytes(&params, sizeof(params),
                                                    metal::StorageMode::Shared);
        if (!paramSp) return false;
        keep.push_back(paramSp);
        auto ce = c.device->makeComputeEncoder();
        if (!ce) return false;
        ce->setPipeline(info.gsPipe.get());
        int slot = 0;
        for (const auto& b : info.gs.bindings) {
            auto it = captureByName.find(b.name);
            if (it == captureByName.end()) return false;
            ce->setBuffer(it->second, (uint32_t)slot);
            ++slot;
        }
        ce->setBuffer(paramSp.get(), (uint32_t)slot);
        ++slot;
        for (const auto& f : gs.outFields) {
            auto it = outByName.find(f.name);
            if (it == outByName.end()) return false;
            ce->setBuffer(it->second, (uint32_t)slot);
            ++slot;
        }
        ce->setBuffer(gsIdx, (uint32_t)slot);
        ++slot;
        ce->setBuffer(cntSp.get(), (uint32_t)slot);
        if (!gs.uniforms.empty()) ce->setBuffer(vsUB, (uint32_t)kGsSlotUniform);
        uint32_t groups = (totalPrims + kGsThreadsPerGroup - 1u) / kGsThreadsPerGroup;
        ce->dispatchGroups3D(groups, 1, 1, kGsThreadsPerGroup, 1, 1);
        if (!ce->endAndCommit()) return false;
    }
    // ---- 3. Đọc counter (chỉ cần khi emission động) ----
    uint32_t indexCount = 0;
    if (gs.staticEmission) {
        indexCount = totalPrims * idxPerPrim;
    } else {
        // Số index thật chỉ biết sau khi GPU chạy xong ⇒ commit + chờ.
        c.CommitAndWait();
        const uint32_t* cc = (const uint32_t*)cntSp->contents();
        if (!cc) return false;
        indexCount = cc[0];
        c.pipeStats.gsPrimsWritten += cc[2];
    }
    if (indexCount == 0) return true;

    // ---- 4. Vẽ: passthrough vertex + FS ----
    GsDrawBuffers bufs;
    auto posIt = outByName.find("gl_Position");
    if (posIt == outByName.end()) return false;
    bufs.outBufs.push_back(posIt->second);
    for (const auto& f : info.fsFields) {
        auto it = outByName.find(f.name);
        if (it == outByName.end()) return false;
        bufs.outBufs.push_back(it->second);
    }
    bufs.idx = gsIdx;
    bufs.indexCount = indexCount;
    bufs.prim = (gs.outPrim == GsPrimType::Points) ? metal::PrimitiveType::Point
               : (gs.outPrim == GsPrimType::LineStrip) ? metal::PrimitiveType::Line
                                                       : metal::PrimitiveType::Triangle;
    // Uniform buffer của FS (dùng chung layout stage-local của FS).
    bufs.fsUniform = MakeGsUniformBuffer(c, pr, pr.fsUniforms, pr.fsUniformBytes, keep);
    return AppleDrawGeometryExpanded(c, prog, info, bufs);
}
} // namespace gl
} // namespace tglmt
