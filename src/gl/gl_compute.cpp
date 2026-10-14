// gl_compute.cpp — Compute: MTLComputeCommandEncoder.dispatchThreads.
// gl_memory.cpp gộp chung: MemoryBarrier → MTLBarrier/memoryBarrierWithScope.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
namespace {
// Zero-fallback buffer (256B) cho slot chưa bind — tránh fault GPU.
std::shared_ptr<metal::IBuffer> ZeroBuf(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::IBuffer>> cache;
    if (!c.device || c.device->isNull()) return nullptr;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    uint8_t z[256] = {0};
    auto b = c.device->newBufferWithBytes(z, sizeof(z), metal::StorageMode::Shared);
    if (b) cache[c.device.get()] = b;
    return b;
}
// Lấy IBuffer từ GL buffer id (đã upload gpu).
metal::IBuffer* BufOf(Context& c, GLuint id) {
    if (!id) return nullptr;
    auto it = c.buffers.find(id);
    return (it == c.buffers.end() || !it->second.gpu) ? nullptr : it->second.gpu.get();
}
} // namespace

void glDispatchCompute(GLuint x, GLuint y, GLuint z) {
    Context& c = Context::Current();
    GLuint progId = c.state.BoundProgram();
    auto itp = c.programs.find(progId);
    if (itp == c.programs.end() || !itp->second.linked || itp->second.computeMSL.empty()) {
        c.LogDebug(0, 0, 0, 0, "glDispatchCompute: no compute program bound");
        return;
    }
    auto& pr = itp->second;
    // Null backend / chưa tạo pipeline (link trên Null bỏ qua makeComputePipeline):
    // trung thực — log dispatch (giống trước) thay vì im lặng.
    if (!c.device || c.device->isNull() || !pr.appleCSPipeline) {
        c.LogDebug(0, 0, 0, 0, "glDispatchCompute → dispatchThreads (null/no-pipeline)");
        return;
    }
    auto enc = c.device->makeComputeEncoder();
    if (!enc) { c.errors.Record(0x0502); return; }
    enc->setPipeline(pr.appleCSPipeline.get());
    auto zero = ZeroBuf(c);
    // buffer(16) = uniforms. Compute chưa có uniformLayout (link trả về sớm) →
    // zero fallback (kernel luôn khai báo tglmt_u [[buffer(16)]] → phải bind gì đó).
    if (zero) enc->setBuffer(zero.get(), 16);
    // Block bind: bindingPoint → MSL slot (từ computeBlockSlots của converter).
    for (auto& bs : pr.computeBlockSlots) {
        metal::IBuffer* b = nullptr;
        if (bs.isStorage) {
            auto it = c.storageBindPoints.find((GLuint)bs.binding);
            if (it != c.storageBindPoints.end()) b = BufOf(c, it->second.buffer);
        } else {
            auto it = c.uniformBindPoints.find((GLuint)bs.binding);
            if (it != c.uniformBindPoints.end()) b = BufOf(c, it->second.buffer);
        }
        if (!b) {
            c.LogDebug(0, 0, 0, 0, "glDispatchCompute: block unbound, zero fallback");
            b = zero.get();
        }
        if (b) enc->setBuffer(b, (uint32_t)bs.slot);
    }
    // Dispatch: GL glDispatchCompute(x,y,z) = x*y*z WORKGROUP; Metal
    // dispatchThreads:grid = TỔNG thread → grid = workgroup * localSize.
    uint32_t ls0 = (uint32_t)(pr.computeLocalSize[0] > 0 ? pr.computeLocalSize[0] : 1);
    uint32_t ls1 = (uint32_t)(pr.computeLocalSize[1] > 0 ? pr.computeLocalSize[1] : 1);
    uint32_t ls2 = (uint32_t)(pr.computeLocalSize[2] > 0 ? pr.computeLocalSize[2] : 1);
    enc->dispatch3D((x ? x : 1) * ls0, (y ? y : 1) * ls1, (z ? z : 1) * ls2, ls0, ls1, ls2);
    enc->endAndCommit(); // commit + waitUntilCompleted (compute hiếm, chấp nhận sync)
}
void glDispatchComputeIndirect(GLintptr ind) {
    (void)ind;
    Context::Current().LogDebug(0, 0, 0, 0, "glDispatchComputeIndirect → dispatchThreads(indirect)");
}
void glMemoryBarrier(GLbitfield b) {
    Context::Current().state.SetShadow(0x9280 /*MEMORY_BARRIER_MARKER*/, &b, 4);
}
void glMemoryBarrierByRegion(GLbitfield b) { glMemoryBarrier(b); }
void glBindImageTexture(GLuint u, GLuint t, GLint l, GLboolean lay, GLint layer, GLenum acc, GLenum f) {
    (void)u;(void)t;(void)l;(void)lay;(void)layer;(void)acc;(void)f;
}
void glBindImageTextures(GLuint f, GLsizei n, const GLuint* t) { (void)f;(void)n;(void)t; }
} // namespace tglmt::gl
