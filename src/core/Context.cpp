#include "tglmt/Context.h"
#include <cstdio>
#include <cstdlib>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstring>
#endif
namespace tglmt {
thread_local Context* Context::tCurrent_ = nullptr;
Context* Context::sFallback_ = nullptr;
std::mutex Context::sMu_;

// Log mặc định ra stderr (launcher thu vào latestlog) để diagnostic
// tạo-device không bao giờ câm, kể cả khi app chưa gắn debug callback.
static void CtxLog(const std::string& m) { fprintf(stderr, "[TGLMT] %s\n", m.c_str()); }

// Symbolication vòng sau: raw PC trong glerr# lines cần slide để trừ ra
// file-offset rồi tra nm trên dylib unstripped (giữ ở build-ios-pkg).
// Log 1 lần khi tạo Context đầu tiên.
static void LogSelfSlide() {
#if defined(__APPLE__)
    static bool done = false;
    if (done) return;
    done = true;
    uint32_t n = _dyld_image_count();
    for (uint32_t i = 0; i < n; ++i) {
        const char* nm = _dyld_get_image_name(i);
        if (nm && strstr(nm, "libtglmt") != nullptr) {
            intptr_t slide = _dyld_get_image_vmaddr_slide(i);
            fprintf(stderr, "[TGLMT] symslide lib=%s slide=0x%lx\n", nm, (long)slide);
            fflush(stderr);
            return;
        }
    }
    fprintf(stderr, "[TGLMT] symslide libtglmt NOT FOUND in %u images\n", n);
    fflush(stderr);
#endif
}

Context::Context(const std::string& backend) : backendName(backend) {
    LogSelfSlide();
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
    // Đếm command buffer để biết GPU còn frame nào đang bay (thay cho
    // commitAndWait mỗi frame — xem Context::ThrottleGpu).
    if (device) device->setCommandBufferCounters(&cbCommitted, &cbCompleted);
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
        pendingUsedBuffers.clear();
        pendingUsedTextures.clear();
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
    pendingUsedBuffers.clear();
    pendingUsedTextures.clear();
    // uniformCache giữ (bytes so sánh vẫn đúng sau flush).
    // Ring buffers giữ sống (triple-buffer, xoay theo frame chứ không theo flush
    // để GPU async vẫn đọc an toàn). Cursor không reset ở đây.
}
void Context::InvalidatePendingOnTargetChange() {
    // Gọi khi FBO bind đổi: nếu target GPU khác target đang encode → flush.
    // So sánh ở AppleDrawGL bằng con trỏ target thật (chính xác hơn id GL).
    // Ở đây chỉ là hook dự phòng (flush mù) cho các đường đổi FBO chưa soi target.
    (void)0;
}
// Deferred full: diag gate — getenv 1 lần, cache static.
// iOS: app không kế thừa env từ shell → build -DTGLMT_FORCE_DIAG=ON (luôn true).
bool Context::DiagOn() {
#if defined(TGLMT_FORCE_DIAG)
    return true;
#else
    static int cached = -1;
    if (cached < 0) {
        const char* e = std::getenv("TGLMT_DIAG");
        cached = (e && (e[0] == '1' || e[0] == 'y' || e[0] == 'Y')) ? 1 : 0;
    }
    return cached == 1;
#endif
}
void Context::NoteBufferUsed(GLuint buf) {
    if (!buf) return;
    if (!UsedHas(pendingUsedBuffers, buf)) pendingUsedBuffers.push_back({buf});
}
// GPU đã execute xong mọi command đã commit → buffer đọc trước đó an toàn để
// ghi tại chỗ. Tăng waitGen để FlushBufferStaging phân biệt in-flight.
void Context::CommitAndWait() {
    if (device) device->commitAndWait();
    ++waitGen;
    cbCompleted.store(cbCommitted.load(std::memory_order_relaxed),
                      std::memory_order_relaxed);
}
// Chặn CPU chỉ khi vượt ngân sách frame đang bay. Trước đây SwapBuffers gọi
// commitAndWait() MỖI frame → CPU và Metal nối tiếp (không pipeline) và mọi
// frame đều phải chờ GPU xong trước khi dựng frame sau ⇒ thêm ~1 frame input
// latency, thấy rõ khi lia cam nhanh. Giờ chỉ chặn khi thực sự tắc.
void Context::ThrottleGpu() {
    if (!device || device->isNull()) return;
    // 1 vòng đủ: present đã commit nhưng chưa execute → 1 frame đang bay.
    // Giới hạn số vòng để không bao giờ treo nếu backend không cập nhật counter
    // (đã commit 1 lần rồi vẫn phải thoát).
    for (int guard = 0; guard < 8; ++guard) {
        if ((cbCommitted.load(std::memory_order_relaxed) -
             cbCompleted.load(std::memory_order_relaxed)) < kMaxFramesInFlight)
            return;
        CommitAndWait();
        ++appleStats.framesThrottled;
    }
}
// Dump texring: 96 call bind gần nhất, chron order (cũ → mới) để thấy chuỗi
// bind dẫn đến divergence (đổ tại bindheal#/shadowmis#).
void Context::DumpTexBindRing(const char* tag) {
    fprintf(stderr, "[TGLMT] texring#%s n=%u\n", tag ? tag : "?", texRingCount);
    uint32_t start = (texRingCount < kTexRingN) ? 0 : texRingHead;
    for (uint32_t i = 0; i < texRingCount; ++i) {
        const TexBindRec& r = texRing[(start + i) % kTexRingN];
        fprintf(stderr, "[TGLMT]   seq=%llu %c unit=%u tex=%u tgt=0x%x\n",
                (unsigned long long)r.seq, r.op ? r.op : '?', r.unit, r.tex, r.target);
    }
    fflush(stderr);
}
void Context::NoteTextureUsed(GLuint tex) {
    if (!tex) return;
    if (!UsedHas(pendingUsedTextures, tex)) pendingUsedTextures.push_back({tex});
}
// Conditional-flush: chỉ flush khi tài nguyên stage đã được dùng trong pass
// đang mở. Stage "lạ" (scratch/PBO/atlas chưa bind) thì giữ batching.
bool Context::MustFlushForBufferStage(GLuint buf) {
    if (!pendingEncoder) return false;
    if (!UsedHas(pendingUsedBuffers, buf)) {
        ++appleStats.flushAvoided;
        return false;
    }
    return true;
}
bool Context::MustFlushForTextureStage(GLuint tex) {
    if (!pendingEncoder) return false;
    if (!UsedHas(pendingUsedTextures, tex)) {
        ++appleStats.flushAvoided;
        return false;
    }
    return true;
}
// Cache target FBO: wrapAsTarget cấp phát AppleTarget + đánh dấu depth-init mỗi
// draw. Cache theo (gen, fbo, color, depth) — gen tăng khi texture/FBO bị xoá
// nên không bao giờ trả nhầm target đã bị giải phóng.
std::shared_ptr<metal::IRenderTarget> Context::WrapTarget(GLuint fbo,
        metal::ITexture* col, metal::ITexture* dep) {
    for (int i = 0; i < kWrapCacheN; ++i) {
        WrapEntry& e = wrapCache[i];
        if (e.tgt && e.gen == objectGen && e.fbo == fbo && e.col == col && e.dep == dep)
            return e.tgt;
    }
    auto tgt = device->wrapAsTarget(col, dep);
    WrapEntry& e = wrapCache[wrapNext];
    wrapNext = (wrapNext + 1) % kWrapCacheN;
    e.gen = objectGen; e.fbo = fbo; e.col = col; e.dep = dep; e.tgt = tgt;
    return tgt;
}
// Ring allocator: bump-pointer trên 1 trong 3 buffers, xoay theo frameSeq.
// Mỗi buffer 4MB Shared, align 256 (uniform/index yêu cầu). Khi tràn hoặc
// chưa init hoặc Null backend → trả {nullptr,0} để caller fallback newBuffer.
std::pair<metal::IBuffer*, size_t> Context::RingAlloc(size_t n, size_t align) {
    if (!device || device->isNull() || n == 0 || n > kRingSize / 2) return {nullptr, 0};
    if (align < 16) align = 16;
    uint64_t slot = frameSeq % kRingFrames;
    if (!ringBuf[slot]) {
        ringBuf[slot] = device->newBuffer(kRingSize, metal::StorageMode::Shared);
        ringCursor[slot] = 0;
        if (!ringBuf[slot]) return {nullptr, 0};
    }
    size_t cur = ringCursor[slot];
    size_t aligned = (cur + align - 1) & ~(align - 1);
    if (aligned + n > kRingSize) {
        // Tràn trong cùng frame: xoay sang slot kế tiếp (triple-buffer cho phép
        // vì slot khác frame chưa commit xong? An toàn nhất: fallback newBuffer
        // + đếm wrap để đo áp lực. Không overwrite slot khác đang bay.
        ++appleStats.ringWraps;
        return {nullptr, 0};
    }
    ringCursor[slot] = aligned + n;
    ++appleStats.ringAllocs;
    return {ringBuf[slot].get(), aligned};
}
void Context::NextFrame() {
    ++frameSeq;
    uint64_t slot = frameSeq % kRingFrames;
    ringCursor[slot] = 0; // slot này đã qua 2 frames, GPU xong → tái dùng an toàn
    if (frameSeq > 0 && (frameSeq % kRingFrames) == 0) ++appleStats.ringWraps;
}
// PSO prewarm: đảm bảo program đã có appleVS/FS libraries (MSL compile đắt nhất
// đã xong ở link-time/loading screen). Warm thêm 16 depth states trong bridge
// cache để draw đầu không alloc. KHÔNG tạo pipeline generic vì descriptor phải
// khớp attribs của VAO thật (tạo sai key chỉ tốn compile vô ích + NIL trung thực
// như đã quan sát). Pipeline VAO-specific tạo ở draw đầu (ms, đã có shader warm).
// Launcher gọi sau glLinkProgram cho mỗi vanilla program lúc loading.
bool Context::PrewarmPipelineForProgram(GLuint prog) {
    auto it = programs.find(prog);
    if (it == programs.end() || !it->second.linked) return false;
    if (!device || device->isNull()) return false;
    ProgramObject& pr = it->second;
    if (!pr.appleVS || !pr.appleFS) return false;
    // Warm depth cache (8 funcs × 2 masks). Rẻ, luôn đúng, tránh alloc draw đầu.
    static const uint32_t kFuncs[8] = {0x0200, 0x0201, 0x0202, 0x0203,
                                       0x0204, 0x0205, 0x0206, 0x0207};
    for (uint32_t f : kFuncs) {
        (void)device->makeDepthStencilState(f, true);
        (void)device->makeDepthStencilState(f, false);
    }
    ++appleStats.psoPrewarmed;
    return true;
}
} // namespace tglmt
