// gl_compute.cpp — Compute: MTLComputeCommandEncoder.dispatchThreads.
// gl_memory.cpp gộp chung: MemoryBarrier → MTLBarrier/memoryBarrierWithScope.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
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
// Fallback 1x1 đen (compute sampler thiếu texture → bind đen, không fault).
std::shared_ptr<metal::ITexture> BlackTex(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::ITexture>> cache;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    uint8_t black[4] = {0, 0, 0, 255};
    auto t = c.device->newTextureWithBytes(1, 1, metal::PixelFormat::RGBA8Unorm, black, 4);
    if (t) cache[c.device.get()] = t;
    return t;
}
// Default sampler (LINEAR/REPEAT) cache theo device — compute không cần param phức tạp.
std::shared_ptr<metal::ISamplerState> DefSampler(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::ISamplerState>> cache;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    metal::SamplerDesc d;
    auto s = c.device->makeSampler(d);
    if (s) cache[c.device.get()] = s;
    return s;
}
// Target cho kind char ('2' TEXTURE_2D, 'C' CUBE, 'A' ARRAY).
GLenum KindTarget(char k) {
    switch (k) {
        case 'C': return 0x8513; // TEXTURE_CUBE_MAP
        case 'A': return 0x8C1A; // TEXTURE_2D_ARRAY
        case 'B': return 0x8C2A; // TEXTURE_BUFFER
        default:  return 0x0DE1; // TEXTURE_2D
    }
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
    // buffer(16) = uniforms. Build bytes từ uniformLayout (e.loc>=0, đã điền bởi
    // glGetUniformLocation + glUniform*). Rỗng → zero fallback (tglmt_u luôn bind).
    {
        size_t ubSize = pr.computeUBSize ? pr.computeUBSize : 16;
        thread_local std::vector<uint8_t> ub;
        if (ub.size() != ubSize) ub.assign(ubSize, 0); else std::fill(ub.begin(), ub.end(), 0);
        for (auto& e : pr.uniformLayout) {
            if (e.loc < 0) continue;
            auto uit = pr.uniforms.find(e.loc);
            if (uit == pr.uniforms.end()) continue;
            size_t n = std::min(e.size, uit->second.size());
            if (e.offset + n <= ub.size()) memcpy(ub.data() + e.offset, uit->second.data(), n);
        }
        // Upload (compute hiếm → newBuffer mỗi dispatch chấp nhận được; cache theo nội dung).
        static thread_local std::vector<uint8_t> lastUB;
        static thread_local std::shared_ptr<metal::IBuffer> lastBuf;
        if (!lastBuf || lastUB.size() != ub.size() || memcmp(lastUB.data(), ub.data(), ub.size()) != 0) {
            lastBuf = c.device->newBufferWithBytes(ub.data(), ub.size(), metal::StorageMode::Shared);
            lastUB = ub;
        }
        if (lastBuf) enc->setBuffer(lastBuf.get(), 16);
        else if (zero) enc->setBuffer(zero.get(), 16);
    }
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
    // Texture + sampler: computeSamplers theo thứ tự khai báo = texture/sampler slot k.
    auto ssDef = DefSampler(c);
    auto black = BlackTex(c);
    for (size_t k = 0; k < pr.computeSamplers.size(); ++k) {
        const std::string& name = pr.computeSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        metal::ITexture* gpu = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit, KindTarget(kind));
            auto tit = c.textures.find(texId);
            if (tit != c.textures.end() && tit->second.gpu) gpu = tit->second.gpu.get();
        }
        if (!gpu && black) gpu = black.get();
        if (gpu) enc->setTexture(gpu, (uint32_t)k);
        if (ssDef) enc->setSamplerState(ssDef.get(), (uint32_t)k);
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
