// gl_compute.cpp — Compute: MTLComputeCommandEncoder.dispatchThreads.
// gl_memory.cpp gộp chung: MemoryBarrier → MTLBarrier/memoryBarrierWithScope.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
#include <cstdio>
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
// Target cho kind char ('2' TEXTURE_2D, 'C' CUBE, 'A' ARRAY, '3' TEXTURE_3D;
// P3: '1'/'i'/'u' như 2D, 'x'/'y' như 3D).
GLenum KindTarget(char k) {
    switch (k) {
        case 'C': return 0x8513; // TEXTURE_CUBE_MAP
        case 'A': return 0x8C1A; // TEXTURE_2D_ARRAY
        case 'B': case 'I': case 'U': return 0x8C2A; // TEXTURE_BUFFER (float/int/uint)
        case '3': case 'x': case 'y': return 0x806F; // TEXTURE_3D
        default:  return 0x0DE1; // TEXTURE_2D
    }
}
// P1: flush TOÀN BỘ staging mà dispatch sắp đọc/ghi (SSBO/UBO block bind,
// atomic, sampled texture, image) TRƯỚC khi encode — dispatch chạy trên GPU
// phải thấy dữ liệu mới nhất từ glBufferSubData/glTexSubImage. FlushBufferStaging
// tự commit encoder render đang mở nếu buffer đó là nguồn của pass (đúng thứ tự).
void FlushComputeResources(Context& c, ProgramObject& pr) {
    for (auto& bs : pr.computeBlockSlots) {
        // P4: SSBO dùng binding override (glShaderStorageBlockBinding) nếu có;
        // binding -1 (không layout) = 0 theo spec.
        GLuint bind = bs.isStorage ? pr.EffectiveSsboBinding(bs.name, bs.binding)
                                   : (GLuint)(bs.binding < 0 ? 0 : bs.binding);
        auto& map = bs.isStorage ? c.storageBindPoints : c.uniformBindPoints;
        auto it = map.find(bind);
        if (it != map.end() && it->second.buffer) c.FlushBufferStaging(it->second.buffer);
    }
    for (auto& as : pr.computeAtomics) {
        auto it = c.atomicBindPoints.find((GLuint)as.binding);
        if (it != c.atomicBindPoints.end() && it->second.buffer)
            c.FlushBufferStaging(it->second.buffer);
    }
    for (auto& name : pr.computeSamplers) {
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        if (unit >= 32) continue;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        GLuint texId = c.state.BoundTexture(unit, KindTarget(kind));
        if (texId) c.FlushTextureStaging(texId);
    }
    for (auto& isl : pr.computeImages) {
        auto bit = c.imageBindPoints.find((GLuint)isl.binding);
        if (bit != c.imageBindPoints.end() && bit->second.texture)
            c.FlushTextureStaging(bit->second.texture);
    }
}
// P1: encoder cho dispatch. Deferred (mặc định): tái dùng pendingCompute (1 CB
// cho N dispatch liên tiếp). Sync (TGLMT_COMPUTE_SYNC=1 hoặc backend ép):
// encoder mới + endAndCommit ngay ở caller. Trả nullptr + record lỗi nếu fail.
std::shared_ptr<metal::IComputeEncoder> AcquireComputeEncoder(Context& c, bool& syncMode) {
    syncMode = c.opt.computeSync;
    // Draw đang encode phải commit TRƯỚC CB compute (queue order = commit order);
    // bất biến: pendingCompute và pendingEncoder không cùng tồn tại.
    if (c.pendingEncoder) c.FlushPendingEncoder();
    if (syncMode) {
        auto enc = c.device->makeComputeEncoder();
        if (!enc) c.errors.Record(0x0502);
        return enc;
    }
    if (!c.pendingCompute) c.pendingCompute = c.device->makeComputeEncoder();
    if (!c.pendingCompute) { c.errors.Record(0x0502); return nullptr; }
    return c.pendingCompute;
}
} // namespace

// Forward: helper dispatch chung (định nghĩa sau wrappers).
static void DispatchComputeImpl(GLuint x, GLuint y, GLuint z,
                                int o0, int o1, int o2, bool useOverride);

void glDispatchCompute(GLuint x, GLuint y, GLuint z) {
    DispatchComputeImpl(x, y, z, 0, 0, 0, false);
}
// ARB_compute_variable_group_size: dispatch với group size EXPLICIT (override
// layout local_size của shader). Dùng cho shader không có local_size cố định.
void glDispatchComputeGroupSize(GLuint x, GLuint y, GLuint z, GLuint gx, GLuint gy, GLuint gz) {
    DispatchComputeImpl(x, y, z, (int)gx, (int)gy, (int)gz, true);
}
// Helper chung: dispatch với localSize từ shader hoặc override.
static void DispatchComputeImpl(GLuint x, GLuint y, GLuint z,
                                int o0, int o1, int o2, bool useOverride) {
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
    // P1: flush staging nguồn (SSBO/UBO/atomic/image/sampler) trước khi encode.
    FlushComputeResources(c, pr);
    bool syncMode = false;
    auto enc = AcquireComputeEncoder(c, syncMode);
    if (!enc) return;
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
    // SSBO (isStorage) có thể bị shader GHI → đánh dấu để CPU đọc map/Get* phải
    // chờ compute (SyncComputeWritesTo).
    for (auto& bs : pr.computeBlockSlots) {
        metal::IBuffer* b = nullptr;
        size_t off = 0;
        GLuint bind = bs.isStorage ? pr.EffectiveSsboBinding(bs.name, bs.binding)
                                   : (GLuint)(bs.binding < 0 ? 0 : bs.binding);
        if (bs.isStorage) {
            auto it = c.storageBindPoints.find(bind);
            if (it != c.storageBindPoints.end()) {
                b = BufOf(c, it->second.buffer);
                off = (size_t)it->second.offset; // P4: honor glBindBufferRange
                c.NoteComputeWriter(it->second.buffer);
            }
        } else {
            auto it = c.uniformBindPoints.find(bind);
            if (it != c.uniformBindPoints.end()) b = BufOf(c, it->second.buffer);
        }
        if (!b) {
            c.LogDebug(0, 0, 0, 0, "glDispatchCompute: block unbound, zero fallback");
            b = zero.get();
        }
        if (b) {
            if (bs.isStorage) enc->setBuffer(b, (uint32_t)bs.slot, off);
            else enc->setBuffer(b, (uint32_t)bs.slot);
        }
    }
    // P9 atomic counters: bind atomicBindPoints[binding] → buffer(slot) + offset(layout).
    for (auto& as : pr.computeAtomics) {
        metal::IBuffer* b = nullptr;
        auto it = c.atomicBindPoints.find((GLuint)as.binding);
        if (it != c.atomicBindPoints.end()) {
            b = BufOf(c, it->second.buffer);
            c.NoteComputeWriter(it->second.buffer);
        }
        if (!b) {
            c.LogDebug(0, 0, 0, 0, "glDispatchCompute: atomic unbound, zero fallback");
            b = zero.get();
        }
        if (b) enc->setBuffer(b, (uint32_t)as.slot, (size_t)as.offset);
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
    // P6g image: bind texture từ imageBindPoints[unit] vào texture(slot).
    // Image texture KHÔNG cần sampler state (texel access, không sample).
    for (auto& isl : pr.computeImages) {
        metal::ITexture* gpu = nullptr;
        auto bit = c.imageBindPoints.find((GLuint)isl.binding);
        if (bit != c.imageBindPoints.end() && bit->second.texture) {
            auto tit = c.textures.find(bit->second.texture);
            if (tit != c.textures.end() && tit->second.gpu) gpu = tit->second.gpu.get();
        }
        if (!gpu) {
            c.LogDebug(0, 0, 0, 0, "glDispatchCompute: image unbound, fallback đen");
            if (black) gpu = black.get();
        }
        if (gpu) enc->setTexture(gpu, (uint32_t)isl.slot);
    }
    // Dispatch: GL glDispatchCompute(x,y,z) = x*y*z WORKGROUP; Metal
    // dispatchThreads:grid = TỔNG thread → grid = workgroup * localSize.
    // GroupSizeARB: localSize = override (gx,gy,gz), không phải từ shader.
    uint32_t ls0, ls1, ls2;
    if (useOverride) {
        ls0 = (uint32_t)(o0 > 0 ? o0 : 1);
        ls1 = (uint32_t)(o1 > 0 ? o1 : 1);
        ls2 = (uint32_t)(o2 > 0 ? o2 : 1);
    } else {
        ls0 = (uint32_t)(pr.computeLocalSize[0] > 0 ? pr.computeLocalSize[0] : 1);
        ls1 = (uint32_t)(pr.computeLocalSize[1] > 0 ? pr.computeLocalSize[1] : 1);
        ls2 = (uint32_t)(pr.computeLocalSize[2] > 0 ? pr.computeLocalSize[2] : 1);
    }
    // GL §7.12.1: bất kỳ num_groups nào = 0 → không có work (skip hẳn — Metal
    // dispatchThreads fault khi grid 0, và clamp lên 1 sẽ dispatch SAI theo GL).
    if (x && y && z) {
        enc->dispatch3D((uint32_t)x * ls0, (uint32_t)y * ls1, (uint32_t)z * ls2,
                        ls0, ls1, ls2);
    }
    ++c.appleStats.computeDispatches;
    if (syncMode) {
        enc->endAndCommit(); // commit + waitUntilCompleted (sync mode)
    } else {
        // Deferred: dồn vào pendingCompute; commit tại điểm cần thứ tự
        // (draw/clear/barrier/readback/finish/frame end — xem FlushPendingCompute).
        ++c.pendingComputeDispatches;
    }
}
// GL §7.12.1: params (3×uint32 num_workgroups) đọc từ buffer bound vào
// GL_DISPATCH_INDIRECT_BUFFER (0x90EE) tại byte offset — GPU quyết định lúc
// execute (thường do compute pass trước ghi, kèm GL_COMMAND_BARRIER_BIT).
// Metal map thẳng: dispatchThreadgroupsWithIndirectBuffer (cùng layout struct).
// Không đọc args bằng CPU: args có thể do GPU ghi (zero hoá cull → 0 group =
// no-op hợp lệ theo GL; Metal không validate nội dung indirect buffer).
void glDispatchComputeIndirect(GLintptr ind) {
    Context& c = Context::Current();
    if (ind < 0 || (ind & 0x3)) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    GLuint progId = c.state.BoundProgram();
    auto itp = c.programs.find(progId);
    if (itp == c.programs.end() || !itp->second.linked || itp->second.computeMSL.empty()) {
        c.LogDebug(0, 0, 0, 0, "glDispatchComputeIndirect: no compute program bound");
        return;
    }
    auto& pr = itp->second;
    GLuint ib = c.state.BoundBuffer(0x90EE); // DISPATCH_INDIRECT_BUFFER
    auto bit = c.buffers.find(ib);
    if (!ib || bit == c.buffers.end()) { c.errors.Record(0x0502); return; } // INVALID_OPERATION
    // GL 4.3 §7.12.1: source quá end buffer → INVALID_OPERATION (Mesa: Size < end).
    if (BufferLen(bit->second) < (size_t)ind + 12) { c.errors.Record(0x0502); return; }
    if (!c.device || c.device->isNull() || !pr.appleCSPipeline) {
        c.LogDebug(0, 0, 0, 0,
                   "glDispatchComputeIndirect → dispatchThreadgroups(indirect) (null/no-pipeline)");
        return;
    }
    metal::IBuffer* gpu = bit->second.gpu ? bit->second.gpu.get() : nullptr;
    if (!gpu) {
        // Buffer có size nhưng chưa có MTLBuffer (chưa upload) — trung thực, không fault.
        c.LogDebug(0, 0, 0, 0, "glDispatchComputeIndirect: indirect buffer chưa upload");
        return;
    }
    // P1: args buffer + mọi nguồn khác phải staged → GPU trước khi encode.
    FlushComputeResources(c, pr);
    c.FlushBufferStaging(ib);
    bool syncMode = false;
    auto enc = AcquireComputeEncoder(c, syncMode);
    if (!enc) return;
    enc->setPipeline(pr.appleCSPipeline.get());
    auto zero = ZeroBuf(c);
    { // uniforms: đồng bộ với đường dispatch thường (slot 16)
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
        auto ubuf = c.device->newBufferWithBytes(ub.data(), ub.size(), metal::StorageMode::Shared);
        if (ubuf) enc->setBuffer(ubuf.get(), 16);
        else if (zero) enc->setBuffer(zero.get(), 16);
    }
    for (auto& bs : pr.computeBlockSlots) {
        metal::IBuffer* b = nullptr;
        size_t off = 0;
        GLuint bind = bs.isStorage ? pr.EffectiveSsboBinding(bs.name, bs.binding)
                                   : (GLuint)(bs.binding < 0 ? 0 : bs.binding);
        if (bs.isStorage) {
            auto it = c.storageBindPoints.find(bind);
            if (it != c.storageBindPoints.end()) {
                b = BufOf(c, it->second.buffer);
                off = (size_t)it->second.offset; // P4: honor glBindBufferRange
                c.NoteComputeWriter(it->second.buffer);
            }
        } else {
            auto it = c.uniformBindPoints.find(bind);
            if (it != c.uniformBindPoints.end()) b = BufOf(c, it->second.buffer);
        }
        if (!b) {
            c.LogDebug(0, 0, 0, 0, "glDispatchComputeIndirect: block unbound, zero fallback");
            b = zero.get();
        }
        if (b) {
            if (bs.isStorage) enc->setBuffer(b, (uint32_t)bs.slot, off);
            else enc->setBuffer(b, (uint32_t)bs.slot);
        }
    }
    for (auto& as : pr.computeAtomics) {
        metal::IBuffer* b = nullptr;
        auto it = c.atomicBindPoints.find((GLuint)as.binding);
        if (it != c.atomicBindPoints.end()) {
            b = BufOf(c, it->second.buffer);
            c.NoteComputeWriter(it->second.buffer);
        }
        if (!b) b = zero.get();
        if (b) enc->setBuffer(b, (uint32_t)as.slot, (size_t)as.offset);
    }
    auto ssDef = DefSampler(c);
    auto black = BlackTex(c);
    for (size_t k = 0; k < pr.computeSamplers.size(); ++k) {
        const std::string& name = pr.computeSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        metal::ITexture* gpuTex = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit, KindTarget(kind));
            auto tit = c.textures.find(texId);
            if (tit != c.textures.end() && tit->second.gpu) gpuTex = tit->second.gpu.get();
        }
        if (!gpuTex && black) gpuTex = black.get();
        if (gpuTex) enc->setTexture(gpuTex, (uint32_t)k);
        if (ssDef) enc->setSamplerState(ssDef.get(), (uint32_t)k);
    }
    for (auto& isl : pr.computeImages) {
        metal::ITexture* gpuTex = nullptr;
        auto bit2 = c.imageBindPoints.find((GLuint)isl.binding);
        if (bit2 != c.imageBindPoints.end() && bit2->second.texture) {
            auto tit = c.textures.find(bit2->second.texture);
            if (tit != c.textures.end() && tit->second.gpu) gpuTex = tit->second.gpu.get();
        }
        if (!gpuTex && black) gpuTex = black.get();
        if (gpuTex) enc->setTexture(gpuTex, (uint32_t)isl.slot);
    }
    uint32_t ls0 = (uint32_t)(pr.computeLocalSize[0] > 0 ? pr.computeLocalSize[0] : 1);
    uint32_t ls1 = (uint32_t)(pr.computeLocalSize[1] > 0 ? pr.computeLocalSize[1] : 1);
    uint32_t ls2 = (uint32_t)(pr.computeLocalSize[2] > 0 ? pr.computeLocalSize[2] : 1);
    enc->dispatchThreadgroupsIndirect(gpu, (size_t)ind, ls0, ls1, ls2);
    ++c.appleStats.computeDispatches;
    if (syncMode) enc->endAndCommit();
    else ++c.pendingComputeDispatches;
}
void glMemoryBarrier(GLbitfield b) {
    Context& c = Context::Current();
    c.state.SetShadow(0x9280 /*MEMORY_BARRIER_MARKER*/, &b, 4);
    // P1: barrier = điểm cần thứ tự writes-trước → reads-sau. Commit compute CB
    // đang dồn: mọi CB sau commit đó thấy kết quả (queue in-order + hazard
    // tracking). CPU readback tự sync qua CommitAndWait (wait=true).
    c.FlushPendingCompute(false);
}
void glMemoryBarrierByRegion(GLbitfield b) { glMemoryBarrier(b); }
void glBindImageTexture(GLuint u, GLuint t, GLint l, GLboolean lay, GLint layer, GLenum acc, GLenum f) {
    Context& c = Context::Current();
    if (t && !c.textures.count(t)) { c.errors.Record(0x0502); return; }
    // P6g: ghi image binding (unit → texture + params). Backend bind texture thật
    // vào image slot ở dispatch/draw. Level/layer != 0 → log (dùng base texture).
    c.imageBindPoints[u] = {t, l, lay, layer, acc, f};
    if ((l != 0 || layer != 0) && c.DiagOn()) {
        fprintf(stderr, "[TGLMT] imagebind unit %u tex %u level=%d layer=%d (dùng base)\n",
                u, t, (int)l, (int)layer);
        fflush(stderr);
    }
}
void glBindImageTextures(GLuint f, GLsizei n, const GLuint* t) {
    Context& c = Context::Current();
    for (GLsizei i = 0; i < n; ++i) {
        GLuint tex = (t && i >= 0) ? t[i] : 0;
        if (tex && !c.textures.count(tex)) { c.errors.Record(0x0502); continue; }
        c.imageBindPoints[f + i] = {tex, 0, 0, 0, 0x88B9 /*READ_WRITE*/, 0};
    }
}
} // namespace tglmt::gl
