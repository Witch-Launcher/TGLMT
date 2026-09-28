#include "tglmt/Context.h"
#include <cstdio>
namespace tglmt {
thread_local Context* Context::tCurrent_ = nullptr;
Context* Context::sFallback_ = nullptr;
std::mutex Context::sMu_;

// Log mặc định ra stderr (launcher thu vào latestlog) để diagnostic
// tạo-device không bao giờ câm, kể cả khi app chưa gắn debug callback.
static void CtxLog(const std::string& m) { fprintf(stderr, "[TGLMT] %s\n", m.c_str()); }

Context::Context(const std::string& backend) : backendName(backend) {
    device = metal::CreateDevice(backend, CtxLog);
    if (!device) { // vd apple backend nhưng máy không có MTLDevice → fallback Null, ghi rõ
        backendName = backend + "(fallback:null,no-MTL-device)";
        device = metal::CreateDevice("null", CtxLog);
    } else if (backend == "apple" && device->isNull()) {
        // CreateDevice không bao giờ trả null (tự Null fallback) nên nhánh
        // trên không tới được — tag tên để chẩn đoán phân biệt dylib build
        // thiếu Apple backend vs MTLCreateSystemDefaultDevice nil trên máy.
        backendName = backend + "(null-fallback)";
    }
}
Context& Context::Current() {
    if (tCurrent_) return *tCurrent_;
    std::lock_guard<std::mutex> l(sMu_);
    if (!sFallback_) sFallback_ = new Context("null");
    return *sFallback_;
}
void Context::MakeCurrent(Context* ctx) { tCurrent_ = ctx; }
void Context::LogDebug(GLenum src, GLenum type, GLuint id, GLenum sev, const std::string& msg) {
    if (debugCb) debugCb(src, type, id, sev, (GLsizei)msg.size(), msg.c_str(), debugUser);
}
} // namespace tglmt
