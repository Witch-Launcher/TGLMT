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
// IR lowering: flush encoder đang mở (nếu có). Commit KHÔNG đợi để giữ throughput;
// thứ tự đảm bảo bởi cùng queue. Xóa shadow Metal (viewport/cull/...) nhưng GIỮ
// pipeline cache của device và uniform cache (vẫn đúng sau flush).
void Context::FlushPendingEncoder() {
    if (!pendingEncoder) {
        pendingTarget.reset();
        pendingPipeline.reset();
        pendingHasDepth = false;
        pendingDrawFBO = 0xFFFFFFFFu;
        pendingColorTex = 0xFFFFFFFFu;
        pendingViewportValid = false;
        pendingCullValid = false;
        pendingBlendValid = false;
        pendingDepthValid = false;
        pendingDepthState.reset();
        pendingFillValid = false;
        pendingScissorValid = false;
        pendingKeep.clear();
        return;
    }
    // endAndCommitNoWait trả false khi encoder đã fail — vẫn phải xóa để draw sau
    // tạo encoder mới (không kẹt pending hỏng).
    (void)pendingEncoder->endAndCommitNoWait();
    pendingEncoder.reset();
    pendingTarget.reset();
    pendingPipeline.reset();
    pendingHasDepth = false;
    pendingDrawFBO = 0xFFFFFFFFu;
    pendingColorTex = 0xFFFFFFFFu;
    pendingViewportValid = false;
    pendingCullValid = false;
    pendingBlendValid = false;
    pendingDepthValid = false;
    pendingDepthState.reset();
    pendingFillValid = false;
    pendingScissorValid = false;
    pendingKeep.clear();
    // uniformCache giữ (bytes so sánh vẫn đúng sau flush).
}
void Context::InvalidatePendingOnTargetChange() {
    // Gọi khi FBO bind đổi: nếu target GPU khác target đang encode → flush.
    // So sánh ở AppleDrawGL bằng con trỏ target thật (chính xác hơn id GL).
    // Ở đây chỉ là hook dự phòng (flush mù) cho các đường đổi FBO chưa soi target.
    (void)0;
}
} // namespace tglmt
