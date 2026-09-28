#include "tglmt/Context.h"
namespace tglmt {
thread_local Context* Context::tCurrent_ = nullptr;
Context* Context::sFallback_ = nullptr;
std::mutex Context::sMu_;

Context::Context(const std::string& backend) : backendName(backend) {
    device = metal::CreateDevice(backend);
    if (!device) { // vd apple backend nhưng máy không có MTLDevice → fallback Null, ghi rõ
        backendName = backend + "(fallback:null,no-MTL-device)";
        device = metal::CreateDevice("null");
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
