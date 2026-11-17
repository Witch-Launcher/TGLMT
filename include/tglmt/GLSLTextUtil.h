// GLSLTextUtil.h — helper xử lý văn bản dùng chung cho converter GLSL→MSL.
//
// Tách ra khỏi src/gl/glsl_to_msl.cpp để các file pass built-in GLSL
// (src/gl/glsl_builtins_*.cpp) dùng chung mà không phải sửa cùng một file
// nguồn (xem docs/DECISIONS.md D-011).
//
// Tất cả hàm `inline` trong header ⇒ không đổi ABI, không đổi call-site.
#ifndef TGLMT_GLSL_TEXT_UTIL_H
#define TGLMT_GLSL_TEXT_UTIL_H

#include <cctype>
#include <cstring>
#include <string>
#include <vector>

namespace tglmt {

inline std::string Trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}
inline bool IsIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
}
// ---- so sánh nhanh với literal ----
// `s.compare(pos, count, "lit")` gọi strlen(literal) MỖI LẦN gọi ⇒ strlen +
// memcmp chiếm ~28% thời gian dịch GLSL→MSL (đo bằng `sample`). Với literal,
// độ dài biết lúc compile ⇒ so trực tiếp bằng memcmp.
// Nếu count != strlen(lit) → fallback sang compare() để giữ ĐÚNG semantics
// (compare() so substring với TOÀN BỘ literal, nên "ive" khớp "ivec").
template <size_t N>
inline bool LitEq(const std::string& s, size_t pos, size_t count, const char (&lit)[N]) {
    if (count == N - 1 && pos <= s.size())
        return pos + count <= s.size() && __builtin_memcmp(s.data() + pos, lit, count) == 0;
    return s.compare(pos, count, lit) == 0;
}
// Từ khoá đứng RIÊNG tại pos (không dính ident char trước/sau).
// Tương đương: s.compare(pos, n, w) == 0 && (pos == 0 || !IsIdentChar(s[pos-1]))
//                && (pos+n >= s.size() || !IsIdentChar(s[pos+n])).
template <size_t N>
inline bool IsWordAt(const std::string& s, size_t pos, const char (&w)[N]) {
    if (pos + (N - 1) > s.size()) return false;
    if (__builtin_memcmp(s.data() + pos, w, N - 1) != 0) return false;
    if (pos > 0 && IsIdentChar(s[pos - 1], false)) return false;
    size_t j = pos + (N - 1);
    if (j < s.size() && IsIdentChar(s[j], false)) return false;
    return true;
}
// Bỏ comment // /* */, giữ nguyên string "..." (hiếm trong shader, vẫn xử lý đúng).
inline std::string StripComments(const std::string& s) {
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
inline std::vector<std::string> SplitCallArgs(const std::string& s) {
    std::vector<std::string> out;
    int d = 0, ad = 0;
    size_t s0 = 0;
    for (size_t k = 0; k <= s.size(); ++k) {
        char ch = k < s.size() ? s[k] : ',';
        if (ch == '(') ++d;
        else if (ch == ')') --d;
        else if (ch == '<') {
            // MSL texture3d<float, access::write> — dấu phẩy trong template KHÔNG
            // phải phân cách param (image param helper, Photon lpv).
            size_t w = k;
            while (w > 0 && IsIdentChar(s[w - 1], false)) --w;
            std::string word = s.substr(w, k - w);
            if (word.rfind("texture", 0) == 0 || word.rfind("depth", 0) == 0) ++ad;
        } else if (ch == '>' && ad > 0) --ad;
        else if (ch == ',' && d == 0 && ad == 0) {
            out.push_back(Trim(s.substr(s0, k - s0)));
            s0 = k + 1;
        }
    }
    return out;
}

}  // namespace tglmt

#endif  // TGLMT_GLSL_TEXT_UTIL_H
