// Renderer.cpp — xem Renderer.h.
#include "tglmt/Renderer.h"

#include "tglmt/Context.h"
#include "tglmt/gl46.h"

namespace tglmt {

struct Renderer::Impl {
    std::unique_ptr<Context> ctx;
    std::shared_ptr<metal::IRenderTarget> target;
    uint32_t w = 0, h = 0;
    bool depth = true;
    metal::PixelFormat fmt = metal::PixelFormat::RGBA8Unorm;
    std::string backend = "null";
    bool initialized = false;
};

Renderer::Renderer() : impl_(new Impl()) {}
Renderer::~Renderer() = default;

bool Renderer::Init(const std::string& backend, uint32_t w, uint32_t h, bool depth,
                    metal::PixelFormat fmt) {
    if (backend != "apple" && backend != "null") return false;
    if (w == 0 || h == 0) return false;
    impl_->ctx.reset(new Context(backend));
    impl_->backend = backend;
    impl_->w = w;
    impl_->h = h;
    impl_->depth = depth;
    impl_->fmt = fmt;
    impl_->target.reset();
    impl_->initialized = true;
    return true;
}

void Renderer::Resize(uint32_t w, uint32_t h) {
    if (w == 0 || h == 0) return;
    impl_->w = w;
    impl_->h = h;
    impl_->target.reset(); // dựng lại ở BeginFrame kế tiếp
    if (impl_->ctx) {
        impl_->ctx->FlushPendingEncoder(); // IR: target cũ hết hiệu lực
        impl_->ctx->applePendingClear = true; // target mới phải clear lại
        impl_->ctx->appleClearMask = 0xFFFFFFFFu;
    }
}

bool Renderer::BeginFrame() {
    if (!impl_->initialized || !impl_->ctx) return false;
    Context::MakeCurrent(impl_->ctx.get());
    if (!impl_->target) {
        if (impl_->depth)
            impl_->target = impl_->ctx->device->makeRenderTargetWithDepth(impl_->w, impl_->h,
                                                                         impl_->fmt);
        else
            impl_->target = impl_->ctx->device->makeRenderTarget(impl_->w, impl_->h, impl_->fmt);
        if (impl_->target) {
            impl_->ctx->device->setDefaultRenderTarget(impl_->target);
            impl_->ctx->applePendingClear = true;
            impl_->ctx->appleClearMask = 0xFFFFFFFFu;
        }
    }
    if (!impl_->target && !impl_->ctx->device->isNull()) return false;
    return true;
}

bool Renderer::EndFrame(void* metalLayer) {
    if (!impl_->initialized || !impl_->ctx) return false;
    // IR: flush batch encoder trước khi present (nếu không, draws còn nằm trong
    // encoder mở chưa commit → present thiếu hình).
    Context::MakeCurrent(impl_->ctx.get());
    impl_->ctx->FlushPendingEncoder();
    if (!metalLayer) return true; // headless: không present vẫn coi như xong
    if (!impl_->target) return false;
    return impl_->ctx->device->presentTarget(impl_->target.get(), metalLayer);
}

bool Renderer::ReadPixels(int x, int y, int w, int h, void* out) {
    if (!impl_->initialized || !impl_->ctx || !out) return false;
    if (w <= 0 || h <= 0) return false;
    Context::MakeCurrent(impl_->ctx.get());
    gl::glReadPixels(x, y, w, h, 0x1908 /*GL_RGBA*/, 0x1401 /*GL_UNSIGNED_BYTE*/, out);
    return gl::glGetError() == 0;
}

Context& Renderer::context() { return *impl_->ctx; }

bool Renderer::hasRealGPU() const {
    return impl_->ctx && impl_->ctx->device && !impl_->ctx->device->isNull();
}

RendererStats Renderer::stats() const {
    RendererStats s;
    if (!impl_->ctx) return s;
    const auto& a = impl_->ctx->appleStats;
    s.drawsAttempted = a.drawsAttempted;
    s.drawsEncoded = a.drawsEncoded;
    s.noProgram = a.noProgram;
    s.noTarget = a.noTarget;
    s.noPipeline = a.noPipeline;
    s.miscFail = a.miscFail;
    return s;
}

std::string Renderer::backendName() const {
    if (!impl_->ctx) return "";
    return impl_->ctx->backendName;
}

uint32_t Renderer::width() const { return impl_->w; }
uint32_t Renderer::height() const { return impl_->h; }
metal::PixelFormat Renderer::colorFormat() const { return impl_->fmt; }

} // namespace tglmt
