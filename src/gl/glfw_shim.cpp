// glfw_shim.cpp — xem GLFWShim.h. Phase 1 vanilla boot, A11 tối thiểu.
#include "tglmt/GLFWShim.h"
#include "tglmt/Renderer.h"
#include "tglmt/Context.h"
#include "tglmt/gl46.h"

#include <cstring>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace tglmt {

struct ShimState {
    std::mutex mu;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<TGLMT_Window> win;
    std::string backend = "apple";
};

GLFWShim& GLFWShim::Instance() {
    static GLFWShim s;
    return s;
}

namespace {
ShimState& State() {
    static ShimState st;
    return st;
}
} // namespace

TGLMT_Window* GLFWShim::CreateWindow(uint32_t w, uint32_t h, const char* backend, bool depth) {
    auto& st = State();
    std::lock_guard<std::mutex> l(st.mu);
    std::string be = backend ? backend : "apple";
    auto r = std::make_unique<Renderer>();
    // A11: RGBA8Unorm khớp CAMetalLayer.pixelFormat mặc định của launcher.
    // Launcher PHẢI đặt layer.pixelFormat = MTLPixelFormatRGBA8Unorm, nếu không present từ chối.
    if (!r->Init(be, w ? w : 1, h ? h : 1, depth, metal::PixelFormat::RGBA8Unorm))
        return nullptr;
    st.renderer = std::move(r);
    st.backend = be;
    st.win = std::make_unique<TGLMT_Window>();
    st.win->w = w;
    st.win->h = h;
    // BeginFrame ngay để MakeCurrent + default target sẵn cho gl:: đầu tiên
    // (tránh rơi vào fallback Context — đúng quy ước Renderer.h).
    st.renderer->BeginFrame();
    Context::MakeCurrent(&st.renderer->context());
    return st.win.get();
}

void GLFWShim::DestroyWindow(TGLMT_Window*) {
    auto& st = State();
    std::lock_guard<std::mutex> l(st.mu);
    ClearCurrent();
    st.renderer.reset();
    st.win.reset();
}

bool GLFWShim::MakeCurrent(TGLMT_Window* w) {
    auto& st = State();
    if (!w || !st.renderer || st.win.get() != w) return false;
    // Mỗi thread gọi MakeCurrent riêng (thread_local). Launcher serialize bằng mutex ngoài
    // khi chunk thread + render thread xen kẽ (phase 1 đơn context).
    st.renderer->BeginFrame();
    Context::MakeCurrent(&st.renderer->context());
    return true;
}

void GLFWShim::ClearCurrent() {
    Context::MakeCurrent(nullptr);
}

bool GLFWShim::SwapBuffers(TGLMT_Window* w, void* metalLayer) {
    auto& st = State();
    if (!w || !st.renderer || st.win.get() != w) return false;
    Context::MakeCurrent(&st.renderer->context());
    // Xả draws NoWait trước present (đúng, không stale trên TBDR A11).
    // IR: flush batch encoder mở trước (nếu không present thiếu hình).
    if (st.renderer->hasRealGPU()) {
        st.renderer->context().FlushPendingEncoder();
        st.renderer->context().device->commitAndWait();
    }
    bool ok = st.renderer->EndFrame(metalLayer);
    // Chẩn đoán đen màn hình trên máy thật: log counters throttled.
    // drawsEnc < drawsAtt nhiều => pipeline/target rớt ở AppleDrawGL;
    // drawsEnc ~= drawsAtt mà vẫn đen => dữ liệu (vertex/uniform/texture).
    // Treo (không thêm dòng) vs chậm (dt lớn) phân biệt bằng timestamp từng swap
    // đầu + viewport (viewport rỗng => không fragment nào qua).
    // stderr được launcher ghi vào latestlog (dòng [gc] là bằng chứng).
    {
        using clock = std::chrono::steady_clock;
        static uint64_t nSwap = 0;
        static uint64_t pAtt = 0, pEnc = 0;
        static clock::time_point pT = clock::now();
        ++nSwap;
        bool first12 = nSwap <= 12;
        if (first12 || nSwap % 600 == 0) {
            auto now = clock::now();
            long dt = (long)std::chrono::duration_cast<std::chrono::milliseconds>(now - pT).count();
            auto& a = st.renderer->context().appleStats;
            uint32_t tw = 0, th = 0;
            auto tgt = st.renderer->context().device->defaultRenderTarget();
            if (tgt) {
                tw = tgt->width();
                th = tgt->height();
            }
            ViewportState vp = st.renderer->context().state.GetViewport(0);
            // Oracle nội dung: đọc pixel giữa default target (xác nhận đen thật
            // hay present sai). glReadPixels đã xả GPU + flip đúng spec.
            // PHẢI bind READ=0 trước: game để READ trỏ FBO scratch sau blit
            // (oracle cũ đọc nhầm buffer rỗng). Xong trả lại READ cũ.
            unsigned center[4] = {0, 0, 0, 0};
            {
                unsigned char one[4] = {0, 0, 0, 0};
                if (tw > 0 && th > 0) {
                    GLuint savedRead = st.renderer->context().state.BoundReadFBO();
                    tglmt::gl::glBindFramebuffer(0x8CA8, 0); // READ_FRAMEBUFFER → default
                    tglmt::gl::glReadPixels((GLint)(tw / 2), (GLint)(th / 2), 1, 1, 0x1908,
                                            0x1401, one);
                    if (savedRead != 0)
                        tglmt::gl::glBindFramebuffer(0x8CA8, savedRead);
                    center[0] = one[0];
                    center[1] = one[1];
                    center[2] = one[2];
                    center[3] = one[3];
                }
            }
            fprintf(stderr,
                    "[TGLMT] build=b3-gettex2 frame=%llu dt=%ldms dAtt=%llu dEnc=%llu "
                    "att=%llu enc=%llu progs=%zu noProg=%llu noTgt=%llu noPipe=%llu "
                    "misc=%llu clears=%llu blits=%llu copyTex=%llu mipBase=%llu "
                    "range=%llu hazard=%llu uboSmall=%llu target=%p %ux%u "
                    "vp=%.0fx%.0f@%.0f,%.0f center=(%u,%u,%u,%u) present=%d\n",
                    (unsigned long long)nSwap, dt, (unsigned long long)(a.drawsAttempted - pAtt),
                    (unsigned long long)(a.drawsEncoded - pEnc),
                    (unsigned long long)a.drawsAttempted, (unsigned long long)a.drawsEncoded,
                    a.progEncoded.size(), (unsigned long long)a.noProgram,
                    (unsigned long long)a.noTarget, (unsigned long long)a.noPipeline,
                    (unsigned long long)a.miscFail, (unsigned long long)a.glClears,
                    (unsigned long long)a.blits, (unsigned long long)a.copyTex,
                    (unsigned long long)a.mipBase,
                    (unsigned long long)a.rangeWarn, (unsigned long long)a.hazardWarn,
                    (unsigned long long)a.uboSmall,
                    (const void*)tgt.get(), tw, th, vp.x, vp.y, vp.w, vp.h, center[0],
                    center[1], center[2], center[3], (int)ok);
            fflush(stderr);
            pAtt = a.drawsAttempted;
            pEnc = a.drawsEncoded;
            pT = now;
        }
    }
    // Frame kế tiếp: BeginFrame để target/load sẵn sàng (giữ quy ước Begin→gl→End).
    st.renderer->BeginFrame();
    Context::MakeCurrent(&st.renderer->context());
    return ok;
}

void GLFWShim::SwapInterval(int interval) {
    auto& st = State();
    if (st.win) st.win->swapInterval = interval;
    // A11 vsync thực hiện ở CADisplayLink của launcher (interval 0 = bỏ vsync benchmark).
}

void GLFWShim::Resize(TGLMT_Window* w, uint32_t width, uint32_t height) {
    auto& st = State();
    if (!w || !st.renderer || st.win.get() != w || !width || !height) return;
    if (st.win->w == width && st.win->h == height) return;
    st.win->w = width;
    st.win->h = height;
    st.renderer->Resize(width, height);
}

bool GLFWShim::HasRealGPU() const {
    auto& st = State();
    return st.renderer && st.renderer->hasRealGPU();
}

uint64_t GLFWShim::DrawsEncoded() const {
    auto& st = State();
    return st.renderer ? st.renderer->stats().drawsEncoded : 0;
}

const char* GLFWShim::BackendName() const {
    auto& st = State();
    static thread_local std::string cache;
    if (!st.renderer) return "none";
    cache = st.renderer->backendName();
    return cache.c_str();
}

void* GLFWShim::GetProcAddress(const char* name) {
    if (!name) return nullptr;
    // Bảng tra tối thiểu cho LWJGL vanilla boot (draw/state/texture/shader/program/FBO/sync).
    // Đủ để dlsym không trả null cho path vanilla; mod mở rộng (tess/GS/compute) đã có
    // symbol trong lib (698/698) nhưng GetProcAddress trả null trung thực khi chưa hỗ trợ GPU.
    static const std::unordered_map<std::string, void*> kProc = {
#define TGLMT_PROC(n) {#n, (void*)&tglmt::gl::n},
        TGLMT_PROC(glDrawArrays) TGLMT_PROC(glDrawElements)
        TGLMT_PROC(glDrawArraysInstanced) TGLMT_PROC(glDrawElementsInstanced)
        TGLMT_PROC(glDrawElementsBaseVertex) TGLMT_PROC(glDrawRangeElements)
        TGLMT_PROC(glDrawArraysIndirect) TGLMT_PROC(glDrawElementsIndirect)
        TGLMT_PROC(glMultiDrawArrays) TGLMT_PROC(glMultiDrawElements)
        TGLMT_PROC(glGenBuffers) TGLMT_PROC(glBindBuffer) TGLMT_PROC(glBufferData)
        TGLMT_PROC(glBufferSubData) TGLMT_PROC(glDeleteBuffers)
        TGLMT_PROC(glGenVertexArrays) TGLMT_PROC(glBindVertexArray)
        TGLMT_PROC(glEnableVertexAttribArray) TGLMT_PROC(glVertexAttribPointer)
        TGLMT_PROC(glVertexAttribDivisor) TGLMT_PROC(glBindVertexBuffer)
        TGLMT_PROC(glGenTextures) TGLMT_PROC(glBindTexture) TGLMT_PROC(glTexImage2D)
        TGLMT_PROC(glTexSubImage2D) TGLMT_PROC(glTexParameteri) TGLMT_PROC(glGenerateMipmap)
        TGLMT_PROC(glActiveTexture) TGLMT_PROC(glBindSampler) TGLMT_PROC(glGenSamplers)
        TGLMT_PROC(glCreateShader) TGLMT_PROC(glShaderSource) TGLMT_PROC(glCompileShader)
        TGLMT_PROC(glCreateProgram) TGLMT_PROC(glAttachShader) TGLMT_PROC(glLinkProgram)
        TGLMT_PROC(glUseProgram) TGLMT_PROC(glGetUniformLocation) TGLMT_PROC(glUniform1i)
        TGLMT_PROC(glUniformMatrix4fv) TGLMT_PROC(glUniform3fv) TGLMT_PROC(glUniform4fv)
        TGLMT_PROC(glGenFramebuffers) TGLMT_PROC(glBindFramebuffer)
        TGLMT_PROC(glFramebufferTexture2D) TGLMT_PROC(glCheckFramebufferStatus)
        TGLMT_PROC(glBlitFramebuffer) TGLMT_PROC(glDrawBuffers) TGLMT_PROC(glReadBuffer)
        TGLMT_PROC(glClear) TGLMT_PROC(glClearColor) TGLMT_PROC(glClearDepthf)
        TGLMT_PROC(glEnable) TGLMT_PROC(glDisable) TGLMT_PROC(glBlendFunc)
        TGLMT_PROC(glDepthFunc) TGLMT_PROC(glViewport) TGLMT_PROC(glScissor)
        TGLMT_PROC(glReadPixels) TGLMT_PROC(glGetError) TGLMT_PROC(glGetString)
        TGLMT_PROC(glFenceSync) TGLMT_PROC(glClientWaitSync) TGLMT_PROC(glDeleteSync)
        TGLMT_PROC(glGenQueries) TGLMT_PROC(glBeginQuery) TGLMT_PROC(glEndQuery)
        TGLMT_PROC(glBindBufferBase) TGLMT_PROC(glBindBufferRange)
        TGLMT_PROC(glGetUniformBlockIndex) TGLMT_PROC(glUniformBlockBinding)
#undef TGLMT_PROC
    };
    auto it = kProc.find(name);
    return it == kProc.end() ? nullptr : it->second;
}

extern "C" {
TGLMT_Window* TGLMT_CreateWindow(uint32_t w, uint32_t h, const char* backend, int depth) {
    return GLFWShim::Instance().CreateWindow(w, h, backend, depth != 0);
}
void TGLMT_DestroyWindow(TGLMT_Window* w) { GLFWShim::Instance().DestroyWindow(w); }
int TGLMT_MakeCurrent(TGLMT_Window* w) { return GLFWShim::Instance().MakeCurrent(w) ? 1 : 0; }
void TGLMT_ClearCurrent(void) { GLFWShim::ClearCurrent(); }
int TGLMT_SwapBuffers(TGLMT_Window* w, void* metalLayer) {
    return GLFWShim::Instance().SwapBuffers(w, metalLayer) ? 1 : 0;
}
void TGLMT_SwapInterval(int interval) { GLFWShim::Instance().SwapInterval(interval); }
void* TGLMT_GetProcAddress(const char* name) { return GLFWShim::Instance().GetProcAddress(name); }
int TGLMT_HasRealGPU(void) { return GLFWShim::Instance().HasRealGPU() ? 1 : 0; }
void TGLMT_ResizeWindow(TGLMT_Window* w, uint32_t width, uint32_t height) {
    GLFWShim::Instance().Resize(w, width, height);
}
const char* TGLMT_BackendName(void) { return GLFWShim::Instance().BackendName(); }
} // extern "C"

} // namespace tglmt
