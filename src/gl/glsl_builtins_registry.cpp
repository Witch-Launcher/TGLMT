// glsl_builtins_registry.cpp — registry của các pass dịch built-in GLSL.
//
// Mỗi nhóm built-in nằm ở file riêng (src/gl/glsl_builtins_<group>.cpp) và
// **tự đăng ký** qua static initializer ⇒ thêm nhóm MỚI không cần sửa file
// này (tránh xung đột khi nhiều người làm song song).
//
// Xem include/tglmt/GLSLBuiltins.h để biết cách viết một pass.
#include "tglmt/GLSLBuiltins.h"

#include <algorithm>
#include <string>
#include <vector>

namespace tglmt::gl {
namespace {

struct Entry {
    std::string id;
    GLSLBuiltinPassFn fn;
};

// Đăng ký trước khi main() chạy (static init) — thứ tự đăng ký = thứ tự
// chạy. Để thứ tự ỔN ĐỊNH, registry gọi theo (id) — không phụ thuộc thứ tự
// link. Nhờ vậy kết quả không đổi giữa các bản build.
std::vector<Entry>& Registry() {
    static std::vector<Entry> reg;
    return reg;
}

// ---- Pass mức NGUỒN (chạy trước khi converter parse) ---------------------
struct SrcEntry {
    std::string id;
    GLSLSourcePassFn fn;
};

std::vector<SrcEntry>& SrcRegistry() {
    static std::vector<SrcEntry> reg;
    return reg;
}

}  // namespace

bool RegisterGLSLSourcePass(const char* id, GLSLSourcePassFn fn) {
    if (!id || !fn) return false;
    SrcRegistry().push_back({std::string(id), fn});
    return true;
}

std::string RunGLSLSourcePasses(const std::string& src) {
    std::vector<SrcEntry> reg = SrcRegistry();
    std::sort(reg.begin(), reg.end(),
              [](const SrcEntry& a, const SrcEntry& b) { return a.id < b.id; });
    std::string cur = src;
    for (const SrcEntry& e : reg) cur = e.fn(cur);
    return cur;
}

std::vector<std::string> GLSLSourcePassIds() {
    std::vector<SrcEntry> reg = SrcRegistry();
    std::sort(reg.begin(), reg.end(),
              [](const SrcEntry& a, const SrcEntry& b) { return a.id < b.id; });
    std::vector<std::string> out;
    out.reserve(reg.size());
    for (const SrcEntry& e : reg) out.push_back(e.id);
    return out;
}

bool RegisterGLSLBuiltinPass(const char* id, GLSLBuiltinPassFn fn) {
    if (!id || !fn) return false;
    Registry().push_back({std::string(id), fn});
    return true;
}

void RunGLSLBuiltinPasses(GLSLBuiltinCtx& ctx) {
    // Sắp xếp theo id ⇒ thứ tự chạy ỔN ĐỊNH, không phụ thuộc thứ tự link.
    std::vector<Entry> reg = Registry();
    std::sort(reg.begin(), reg.end(),
              [](const Entry& a, const Entry& b) { return a.id < b.id; });
    for (const Entry& e : reg) e.fn(ctx);
}

std::vector<std::string> GLSLBuiltinPassIds() {
    std::vector<Entry> reg = Registry();
    std::sort(reg.begin(), reg.end(),
              [](const Entry& a, const Entry& b) { return a.id < b.id; });
    std::vector<std::string> out;
    out.reserve(reg.size());
    for (const Entry& e : reg) out.push_back(e.id);
    return out;
}

}  // namespace tglmt::gl
