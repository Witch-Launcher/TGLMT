// gl_buffer.cpp — Buffer objects: Metal MTLBuffer via IDevice + shadow CPU.
// Spec: glspec46.core.pdf §6 (Buffer Objects). Metal: newBuffer/newBufferWithBytes,
// contents()+memcpy, didModifyRange, blit copyFromBuffer.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
using namespace tglmt;

static BufferObject* BoundBuf(GLenum target, bool create = false) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundBuffer(target);
    if (!id) { c.errors.Record(0x0502); return nullptr; } // INVALID_OPERATION
    auto it = c.buffers.find(id);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    (void)create;
    return &it->second;
}

namespace tglmt {
// IR staging public: ghi nhận range bẩn, merge với range cuối nếu kề/chồng lấn.
// Mỗi SubData deferred đều đếm bufferCoalesced (bằng chứng 10 updates → staging).
// Thứ tự GL: chỉ flush encoder đang mở khi buffer này ĐÃ dùng trong pass
// (conditional-flush, deferred full). Stage "lạ" giữ nguyên batching.
void Context::StageBufferRange(GLuint bufId, size_t off, size_t len) {
    if (!len) return;
    if (pendingEncoder && MustFlushForBufferStage(bufId)) FlushPendingEncoder();
    auto& vec = pendingBufRanges[bufId];
    if (!vec.empty()) {
        auto& last = vec.back();
        size_t lastEnd = last.off + last.len;
        size_t wantEnd = off + len;
        // Merge khi chồng lấn hoặc kề nhau (gap <= 64B thì lấp luôn cho đỡ fragment).
        if (off <= lastEnd + 64) {
            size_t newEnd = std::max(lastEnd, wantEnd);
            size_t newOff = std::min(last.off, off);
            last.off = newOff;
            last.len = newEnd - newOff;
            ++appleStats.bufferCoalesced;
            return;
        }
    }
    vec.push_back(BufRange{off, len});
    ++appleStats.bufferCoalesced;
}
} // namespace tglmt

// Wrapper nội bộ giữ nguyên mọi call-site trong file.
static void StageBufRange(Context& c, GLuint bufId, size_t off, size_t len) {
    c.StageBufferRange(bufId, off, len);
}

namespace tglmt {
// Flush 1 buffer: merge toàn bộ ranges (sort + gộp), 1 memcpy+didModify mỗi đoạn.
void Context::FlushBufferStaging(GLuint buf) {
    auto it = pendingBufRanges.find(buf);
    if (it == pendingBufRanges.end() || it->second.empty()) return;
    auto bit = buffers.find(buf);
    if (bit == buffers.end()) { pendingBufRanges.erase(it); return; }
    BufferObject& bo = bit->second;
    auto& vec = it->second;
    std::sort(vec.begin(), vec.end(),
              [](const BufRange& a, const BufRange& b) { return a.off < b.off; });
    // Gộp chồng lấn/kề (gap <= 256B: copy thêm vài trăm byte rẻ hơn 1 didModify).
    std::vector<BufRange> merged;
    for (auto& r : vec) {
        if (!merged.empty()) {
            auto& m = merged.back();
            size_t mEnd = m.off + m.len;
            if (r.off <= mEnd + 256) {
                size_t e = std::max(mEnd, r.off + r.len);
                m.len = e - m.off;
                continue;
            }
        }
        merged.push_back(r);
    }
    if (bo.gpu) {
        size_t gpuLen = bo.gpu->length();
        for (auto& m : merged) {
            if (m.off >= bo.data.size() || m.off >= gpuLen) continue;
            size_t n = std::min({m.len, bo.data.size() - m.off, gpuLen - m.off});
            if (!n) continue;
            memcpy((uint8_t*)bo.gpu->contents() + m.off, bo.data.data() + m.off, n);
            bo.gpu->didModifyRange(m.off, n);
        }
    }
    ++appleStats.bufferFlushes;
    pendingBufRanges.erase(it);
}
void Context::FlushAllBufferStaging() {
    if (pendingBufRanges.empty()) return;
    // Copy keys trước vì FlushBufferStaging xóa entry trong map.
    std::vector<GLuint> ids;
    ids.reserve(pendingBufRanges.size());
    for (auto& kv : pendingBufRanges) ids.push_back(kv.first);
    for (GLuint id : ids) FlushBufferStaging(id);
}
} // namespace tglmt

namespace tglmt::gl {
void glGenBuffers(GLsizei n, GLuint* buffers) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Gen(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) c.buffers[buffers[i]] = BufferObject{buffers[i]};
}
void glCreateBuffers(GLsizei n, GLuint* buffers) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) c.buffers[buffers[i]] = BufferObject{buffers[i]};
}
void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Delete(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) {
        c.buffers.erase(buffers[i]);
        c.pendingBufRanges.erase(buffers[i]);
    }
}
GLboolean glIsBuffer(GLuint b) {
    return Context::Current().registry.Is(ObjectKind::Buffer, b) ? 1 : 0;
}
void glBindBuffer(GLenum target, GLuint buffer) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    c.state.BindBuffer(target, buffer);
    if (buffer) c.buffers[buffer].target = target;
    // Spec §6.1/§10.3: ELEMENT_ARRAY_BUFFER là state của VAO hiện tại, không phải global.
    if (target == 0x8893 /*ELEMENT_ARRAY_BUFFER*/) {
        GLuint vao = c.state.BoundVAO();
        auto it = c.vaos.find(vao);
        if (it != c.vaos.end()) it->second.elementBuffer = buffer;
    }
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    if (target == 0x8A11 /*UNIFORM_BUFFER*/) {
        c.uniformBindPoints[index] = BufferRange{buffer, 0, 0};
        // Chẩn đoán misbound UBO: chỉ khi TGLMT_DIAG=1 (release giữ 60fps).
        if (c.DiagOn()) {
            static int nUB = 0;
            if (++nUB <= 400 && buffer) {
                auto it = c.buffers.find(buffer);
                size_t sz = (it == c.buffers.end()) ? 0 : it->second.data.size();
                fprintf(stderr, "[TGLMT] ubobind#%d point %u -> buf %u (%zuB)\n",
                        nUB, index, buffer, sz);
                fflush(stderr);
            }
        }
    } else if (target == 0x90D2 /*SHADER_STORAGE_BUFFER*/) {
        c.storageBindPoints[index] = BufferRange{buffer, 0, 0};
    }
    glBindBuffer(target, buffer);
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr o, GLsizeiptr s) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    if (o < 0 || s < 0) { c.errors.Record(0x0501); return; }
    if (target == 0x8A11) {
        c.uniformBindPoints[index] = BufferRange{buffer, o, s};
        if (c.DiagOn()) {
            static int nUBR = 0;
            if (++nUBR <= 400 && buffer) {
                fprintf(stderr, "[TGLMT] uborange#%d point %u -> buf %u off=%ld size=%ld\n",
                        nUBR, index, buffer, (long)o, (long)s);
                fflush(stderr);
            }
        }
    } else if (target == 0x90D2) {
        c.storageBindPoints[index] = BufferRange{buffer, o, s};
    }
    glBindBuffer(target, buffer);
}
void glBindBuffersBase(GLenum t, GLuint f, GLsizei n, const GLuint* b) {
    for (GLsizei i = 0; i < n; ++i) glBindBufferBase(t, f + i, b ? b[i] : 0);
}
void glBindBuffersRange(GLenum t, GLuint f, GLsizei n, const GLuint* b, const GLintptr* o, const GLsizeiptr* s) {
    for (GLsizei i = 0; i < n; ++i) glBindBufferRange(t, f + i, b ? b[i] : 0, o ? o[i] : 0, s ? s[i] : 0);
}
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    Context& c = Context::Current();
    if (size < 0) { c.errors.Record(0x0501); return; }
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    bo->data.assign((const uint8_t*)(data ? data : nullptr), (const uint8_t*)(data ? data : nullptr) + (data ? size : 0));
    if (!data) bo->data.assign((size_t)size, 0);
    bo->usage = usage;
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    c.pendingBufRanges.erase(bid); // realloc → staging cũ vô nghĩa
}
void glNamedBufferData(GLuint b, GLsizeiptr size, const void* data, GLenum usage) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    GLuint saved = c.state.BoundBuffer(it->second.target ? it->second.target : 0x8892);
    c.state.BindBuffer(0x8892, b); // ARRAY_BUFFER scratch
    // trực tiếp không qua target
    it->second.data.assign(size ? (size_t)size : 0, 0);
    if (data && size) memcpy(it->second.data.data(), data, (size_t)size);
    it->second.usage = usage;
    it->second.gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                          : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    c.pendingBufRanges.erase(b);
    c.state.BindBuffer(0x8892, saved);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    bo->data.assign((size_t)size, 0);
    if (data) memcpy(bo->data.data(), data, (size_t)size);
    bo->storageFlags = flags;
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    c.pendingBufRanges.erase(bid);
}
void glNamedBufferStorage(GLuint b, GLsizeiptr size, const void* data, GLbitfield flags) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    it->second.data.assign((size_t)size, 0);
    if (data) memcpy(it->second.data.data(), data, (size_t)size);
    it->second.storageFlags = flags;
    it->second.gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                          : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    c.pendingBufRanges.erase(b);
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo || !data) { if(!data) c.errors.Record(0x0501); return; }
    if (offset < 0 || size < 0 || (size_t)(offset + size) > bo->data.size()) { c.errors.Record(0x0501); return; }
    memcpy(bo->data.data() + offset, data, (size_t)size);
    // IR deferred: chỉ stage range, GPU copy dồn đến flush (trước draw/readback).
    // Trước đây: memcpy GPU + didModify ngay mỗi call (10 calls → 10 Metal ops).
    StageBufRange(c, bid, (size_t)offset, (size_t)size);
}
void glNamedBufferSubData(GLuint b, GLintptr off, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > it->second.data.size()) { c.errors.Record(0x0501); return; }
    memcpy(it->second.data.data() + off, data, (size_t)size);
    // Game 26.x update buffer per-frame qua writeToBuffer → glNamedBufferSubData.
    // IR: stage thay vì sync GPU ngay (tránh stale đã fix trước đây bằng flush đúng chỗ).
    StageBufRange(c, b, (size_t)off, (size_t)size);
}
void glGetBufferSubData(GLenum target, GLintptr off, GLsizeiptr size, void* data) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    if (off < 0 || size < 0 || (size_t)(off + size) > bo->data.size()) { c.errors.Record(0x0501); return; }
    memcpy(data, bo->data.data() + off, (size_t)size);
}
void glGetNamedBufferSubData(GLuint b, GLintptr off, GLsizeiptr size, void* data) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > it->second.data.size()) { c.errors.Record(0x0501); return; }
    memcpy(data, it->second.data.data() + off, (size_t)size);
}
void glCopyBufferSubData(GLenum rt, GLenum wt, GLintptr ro, GLintptr wo, GLsizeiptr size) {
    Context& c = Context::Current();
    GLuint r = c.state.BoundBuffer(rt), w = c.state.BoundBuffer(wt);
    auto itR = c.buffers.find(r), itW = c.buffers.find(w);
    if (itR == c.buffers.end() || itW == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (ro < 0 || wo < 0 || size < 0) { c.errors.Record(0x0501); return; }
    // Metal: blit copyFromBuffer — ở Null backend copy CPU
    if ((size_t)(ro + size) > itR->second.data.size() || (size_t)(wo + size) > itW->second.data.size()) { c.errors.Record(0x0501); return; }
    // Đọc nguồn phải flush staging nguồn trước (shadow nguồn mới nhất sau flush).
    c.FlushBufferStaging(r);
    memmove(itW->second.data.data() + wo, itR->second.data.data() + ro, (size_t)size);
    StageBufRange(c, w, (size_t)wo, (size_t)size);
}
void glCopyNamedBufferSubData(GLuint r, GLuint w, GLintptr ro, GLintptr wo, GLsizeiptr size) {
    Context& c = Context::Current();
    auto itR = c.buffers.find(r), itW = c.buffers.find(w);
    if (itR == c.buffers.end() || itW == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if ((size_t)(ro + size) > itR->second.data.size() || (size_t)(wo + size) > itW->second.data.size()) { c.errors.Record(0x0501); return; }
    c.FlushBufferStaging(r);
    memmove(itW->second.data.data() + wo, itR->second.data.data() + ro, (size_t)size);
    StageBufRange(c, w, (size_t)wo, (size_t)size);
}
void* glMapBuffer(GLenum target, GLenum access) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    (void)access;
    // IR: flush staging trước để GPU có dữ liệu mới nhất trước khi app ghi trực tiếp.
    c.FlushBufferStaging(bid);
    bo->mapped = true; bo->mapOffset = 0; bo->mapLength = bo->data.size();
    if (bo->gpu) return bo->gpu->contents();
    return bo->data.data();
}
void* glMapBufferRange(GLenum target, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    if (off < 0 || len < 0 || (size_t)(off + len) > bo->data.size()) { c.errors.Record(0x0501); return nullptr; }
    c.FlushBufferStaging(bid);
    bo->mapped = true; bo->mapOffset = (size_t)off; bo->mapLength = (size_t)len; bo->mapAccess = access;
    if (bo->gpu) return (uint8_t*)bo->gpu->contents() + off;
    return bo->data.data() + off;
}
void* glMapNamedBuffer(GLuint b, GLenum access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    (void)access;
    c.FlushBufferStaging(b);
    it->second.mapped = true;
    it->second.mapOffset = 0;
    it->second.mapLength = it->second.data.size();
    // Trả con trỏ GPU khi có (như bản bound) để write thấy ngay trên GPU;
    // Unmap sẽ didModify + chép ngược shadow.
    if (it->second.gpu) return it->second.gpu->contents();
    return it->second.data.data();
}
void* glMapNamedBufferRange(GLuint b, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    (void)access;
    if (off < 0 || len < 0 || (size_t)(off + len) > it->second.data.size()) { c.errors.Record(0x0501); return nullptr; }
    c.FlushBufferStaging(b);
    it->second.mapped = true;
    it->second.mapOffset = (size_t)off;
    it->second.mapLength = (size_t)len;
    it->second.mapAccess = access;
    if (it->second.gpu) return (uint8_t*)it->second.gpu->contents() + off;
    return it->second.data.data() + off;
}
GLboolean glUnmapBuffer(GLenum target) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return 0;
    bo->mapped = false;
    // Map đã flush staging trước đó; Unmap ghi trực tiếp GPU nên staging còn lại
    // (nếu có SubData xen giữa Map/Unmap) phải xóa để tránh ghi đè dữ liệu map.
    // An toàn nhất: flush staging còn lại TRƯỚC khi chép map về? Map range đã
    // didModify riêng; staged ranges ngoài map range vẫn cần. Giữ đơn giản đúng:
    // flush staging trước, rồi mới xử lý map (map dữ liệu mới nhất thắng).
    c.FlushBufferStaging(bid);
    if (bo->gpu) {
        bo->gpu->didModifyRange(bo->mapOffset, bo->mapLength);
        // StorageModeShared: CPU và GPU chung bộ nhớ nhưng shadow vector của TGLMT
        // là bản riêng → chép ngược về để GetBufferSubData/Map sau thấy dữ liệu mới.
        size_t end = std::min(bo->mapOffset + bo->mapLength, bo->data.size());
        end = std::min(end, bo->gpu->length());
        if (end > bo->mapOffset && bo->mapOffset < bo->data.size())
            memcpy(bo->data.data() + bo->mapOffset,
                   (const uint8_t*)bo->gpu->contents() + bo->mapOffset, end - bo->mapOffset);
    }
    return 1;
}
GLboolean glUnmapNamedBuffer(GLuint b) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return 0; }
    it->second.mapped = false;
    c.FlushBufferStaging(b);
    // Mirror bản bound: didModify + chép ngược shadow (map trả con trỏ GPU).
    if (it->second.gpu) {
        it->second.gpu->didModifyRange(it->second.mapOffset, it->second.mapLength);
        size_t end = std::min(it->second.mapOffset + it->second.mapLength, it->second.data.size());
        end = std::min(end, it->second.gpu->length());
        if (end > it->second.mapOffset && it->second.mapOffset < it->second.data.size())
            memcpy(it->second.data.data() + it->second.mapOffset,
                   (const uint8_t*)it->second.gpu->contents() + it->second.mapOffset,
                   end - it->second.mapOffset);
    }
    return 1;
}
void glFlushMappedBufferRange(GLenum target, GLintptr off, GLsizeiptr len) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    if (bo->gpu) bo->gpu->didModifyRange((size_t)off, (size_t)len);
}
void glFlushMappedNamedBufferRange(GLuint b, GLintptr off, GLsizeiptr len) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (it->second.gpu) it->second.gpu->didModifyRange((size_t)off, (size_t)len);
}
void glClearBufferData(GLenum t, GLenum inf, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(t);
    if (!bo) return;
    // Metal RenderPass clear tương đương; ở buffer: fill 0 hoặc pattern byte đầu
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    std::fill(bo->data.begin(), bo->data.end(), fill);
    if (bo->gpu) {
        memset(bo->gpu->contents(), fill, std::min(bo->data.size(), bo->gpu->length()));
        bo->gpu->didModifyRange(0, std::min(bo->data.size(), bo->gpu->length()));
    }
}
void glClearBufferSubData(GLenum t, GLenum inf, GLintptr off, GLsizeiptr size, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(t);
    BufferObject* bo = BoundBuf(t);
    if (!bo) return;
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    if (off < 0 || size < 0 || (size_t)(off + size) > bo->data.size()) { c.errors.Record(0x0501); return; }
    std::fill(bo->data.begin() + off, bo->data.begin() + off + size, fill);
    StageBufRange(c, bid, (size_t)off, (size_t)size);
}
void glClearNamedBufferData(GLuint b, GLenum inf, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    std::fill(it->second.data.begin(), it->second.data.end(), fill);
    if (it->second.gpu) {
        memset(it->second.gpu->contents(), fill, std::min(it->second.data.size(), it->second.gpu->length()));
        it->second.gpu->didModifyRange(0, std::min(it->second.data.size(), it->second.gpu->length()));
    }
}
void glClearNamedBufferSubData(GLuint b, GLenum inf, GLintptr off, GLsizeiptr size, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    if (off < 0 || size < 0 || (size_t)(off + size) > it->second.data.size()) { c.errors.Record(0x0501); return; }
    std::fill(it->second.data.begin() + off, it->second.data.begin() + off + size, fill);
    StageBufRange(c, b, (size_t)off, (size_t)size);
}
void glInvalidateBufferData(GLuint b) {
    Context& c = Context::Current();
    if (!c.buffers.count(b)) { c.errors.Record(0x0502); return; }
    // Metal storeAction=DontCare — không cần làm gì, giữ shadow
}
void glInvalidateBufferSubData(GLuint b, GLintptr o, GLsizeiptr s) {
    (void)o; (void)s; glInvalidateBufferData(b);
}
} // namespace tglmt::gl
