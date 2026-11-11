// gl_buffer.cpp — Buffer objects: Metal MTLBuffer via IDevice + shadow CPU.
// Spec: glspec46.core.pdf §6 (Buffer Objects). Metal: newBuffer/newBufferWithBytes,
// contents()+memcpy, didModifyRange, blit copyFromBuffer.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <utility>
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

// GL_ARB_buffer_storage: flags hợp lệ của glBufferStorage chỉ gồm
// MAP_READ_BIT | MAP_WRITE_BIT | MAP_PERSISTENT_BIT | MAP_COHERENT_BIT.
// Spec §6.2 (GL 4.6 core): bit lạ → INVALID_VALUE; MAP_PERSISTENT mà thiếu
// READ/WRITE → INVALID_VALUE; MAP_COHERENT mà thiếu MAP_PERSISTENT →
// INVALID_VALUE. Trước đây TGLMT nhận mọi flags → app sai vẫn "chạy" được
// và query BUFFER_STORAGE_FLAGS trả về giá trị không hợp lệ.
static bool ValidStorageFlags(GLbitfield f) {
    // GL 4.6 §6.2 + ARB_buffer_storage: flags hợp lệ gồm 6 bit —
    //   MAP_READ_BIT 0x01, MAP_WRITE_BIT 0x02, MAP_PERSISTENT_BIT 0x40,
    //   MAP_COHERENT_BIT 0x80, DYNAMIC_STORAGE_BIT 0x100, CLIENT_STORAGE_BIT 0x200.
    // THIẾU 0x100/0x200 là Minecraft 26.2 crash ngay lúc boot: MC dùng
    // GlConst.bufferUsageToGlFlag() để map usage → flags, trong đó
    // USAGE_COPY_DST → 0x100 (DYNAMIC_STORAGE_BIT) và USAGE_HINT_CLIENT_STORAGE
    // → 0x200 (CLIENT_STORAGE_BIT); cả hai đều hợp lệ theo spec.
    const GLbitfield kKnown = 0x0001 /*MAP_READ_BIT*/ | 0x0002 /*MAP_WRITE_BIT*/ |
                              0x0040 /*MAP_PERSISTENT_BIT*/ | 0x0080 /*MAP_COHERENT_BIT*/ |
                              0x0100 /*DYNAMIC_STORAGE_BIT*/ | 0x0200 /*CLIENT_STORAGE_BIT*/;
    if (f & ~kKnown) return false;                       // bit lạ
    if ((f & 0x0040) && !(f & 0x0003)) return false;     // PERSISTENT cần READ|WRITE
    if ((f & 0x0080) && !(f & 0x0040)) return false;     // COHERENT cần PERSISTENT
    return true;
}

namespace tglmt {
// IR staging public: ghi nhận range bẩn, merge với range cuối nếu kề/chồng lấn.
// Mỗi SubData deferred đều đếm bufferCoalesced (bằng chứng 10 updates → staging).
// Thứ tự GL: chỉ flush encoder đang mở khi buffer này ĐÃ dùng trong pass
// (conditional-flush, deferred full). Stage "lạ" giữ nguyên batching.
// src != nullptr → payload được carry theo range (shadow CPU rỗng — P1 Apple).
// Khi carry chỉ merge được range LIỀN SÁT ĐÚNG (không gap): byte của phần gap
// không tồn tại ở stage time, để flush resolve từ GPU/shadow cũ (đã đúng).
void Context::StageBufferRange(GLuint bufId, size_t off, size_t len, const uint8_t* src) {
    if (!len) return;
    if (pendingEncoder && MustFlushForBufferStage(bufId)) FlushPendingEncoder();
    auto& vec = pendingBufRanges[bufId];
    const bool carry = (src != nullptr);
    if (!vec.empty()) {
        auto& last = vec.back();
        size_t lastEnd = last.off + last.len;
        size_t wantEnd = off + len;
        if (carry) {
            if (off == lastEnd && last.bytes.size() == last.len) {
                last.bytes.insert(last.bytes.end(), src, src + len);
                last.len = wantEnd - last.off;
                ++appleStats.bufferCoalesced;
                return;
            }
        } else if (off <= lastEnd + 64) {
            // Merge khi chồng lấn hoặc kề nhau (gap <= 64B thì lấp luôn cho đỡ fragment).
            size_t newEnd = std::max(lastEnd, wantEnd);
            size_t newOff = std::min(last.off, off);
            last.off = newOff;
            last.len = newEnd - newOff;
            ++appleStats.bufferCoalesced;
            return;
        }
    }
    BufRange r{off, len, {}};
    if (carry) r.bytes.assign(src, src + len);
    vec.push_back(std::move(r));
    ++appleStats.bufferCoalesced;
}

std::vector<uint8_t>* Context::MutablePbo(GLuint buf, size_t end) {
    auto it = buffers.find(buf);
    if (it == buffers.end()) return nullptr;
    BufferObject& b = it->second;
    if (b.data.empty()) {
        SyncComputeWritesTo(buf); // P1: compute có thể đang ghi buffer này
        FlushBufferStaging(buf); // staged ranges chưa lên GPU
        if (b.gpu && b.gpu->contents()) {
            const uint8_t* g = (const uint8_t*)b.gpu->contents();
            size_t have = std::min(BufferLen(b), b.gpu->length());
            b.data.assign(g, g + have);
        }
    }
    if (end > b.data.size()) b.data.resize(end, 0);
    if (end > b.size) b.size = end;
    return &b.data;
}

Context::BufferView Context::SyncBuffer(GLuint buf) {
    auto it = buffers.find(buf);
    if (it == buffers.end()) return {};
    BufferObject& b = it->second;
    // Shadow rỗng (Apple backend đã bỏ nhân bản CPU) → nội dung nằm ở GPU,
    // phải xả staged ranges trước khi đọc. P1: compute đang ghi buffer này →
    // chờ GPU execute xong trước (contents() sẽ coherent).
    if (b.data.empty()) {
        SyncComputeWritesTo(buf);
        FlushBufferStaging(buf);
    }
    return {BufferBytes(b), BufferLen(b)};
}
} // namespace tglmt

// Wrapper nội bộ giữ nguyên mọi call-site trong file.
static void StageBufRange(Context& c, GLuint bufId, size_t off, size_t len,
                          const uint8_t* src = nullptr) {
    c.StageBufferRange(bufId, off, len, src);
}

namespace tglmt {
// Tổng byte các slot pool đang giữ. Chỉ gọi khi SẮP alloc slot mới (hiếm) nên
// O(#buffer) mỗi lần là vô hại — avoids phải maintain counter dễ drift.
static size_t PoolBytesOf(const std::unordered_map<GLuint, BufferObject>& bufs) {
    size_t n = 0;
    for (auto& kv : bufs)
        for (auto& s : kv.second.pool)
            if (s.buf) n += s.buf->length();
    return n;
}
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
    if (bo.gpu && !merged.empty()) {
        // In-flight write guard (bug: chớp/tia khi nhiều thực thể).
        // Commit encoder là NoWait → command buffer đã commit nhưng GPU CHƯA
        // execute; memcpy shadow→GPU tại chỗ lúc này đổi dữ liệu mà draw cũ đang
        // đọc (iOS shared memory) → vỡ hình/entity méo.
        // TRƯỚC: newBufferWithBytes(TOÀN BỘ shadow) mỗi lần. Minecraft tái dùng
        // 1 vertex buffer cho MỌI batch cùng VertexFormat ⇒ vài trăm lần
        // alloc+full-copy mỗi frame (vừa chậm vừa ép iOS ban submissions).
        // NAY: xoay sang 1 slot trong pool (buffer cũ vẫn sống vì Metal retain),
        // copy đúng range đã gộp; slot quay lại khi GPU đã execute xong tới
        // đợi dùng nó (usedGen <= cbCompleted).
        // Buffer đang mapped (MC persistent-map UBO): con trỏ game trỏ thẳng
        // vào gpu cũ → KHÔNG được đổi identity.
        const bool inFlight = device && !bo.mapped && GpuBusy();
        std::shared_ptr<metal::IBuffer> target = bo.gpu;
        bool rotated = false;
        const size_t need = BufferLen(bo) ? BufferLen(bo) : (size_t)1;
        if (inFlight) {
            // Số slot = opt.bufPoolSlots (TGLMT_BUF_POOL, launcher tab Đồ hoạ →
            // mục TGLMT). Mặc định 3 = đủ cho 2 frame đang bay. Giảm → tiết kiệm
            // RAM pool nhưng dễ chạm nhánh chờ (bufWaitStall) khi GPU tụt hậu.
            const size_t kPool = opt.bufPoolSlots ? opt.bufPoolSlots : 1;
            if (bo.pool.size() != kPool) bo.pool.resize(kPool); // hạ setting → trả slot
            const uint64_t done = cbCompleted.load(std::memory_order_relaxed);
            for (size_t i = 0; i < kPool; ++i) {
                BufferObject::GpuSlot& sl = bo.pool[(bo.poolNext + i) % kPool];
                if (sl.buf && sl.usedGen > done) continue; // GPU còn đang đọc slot này
                // Slot rảnh nhưng to gấp đôi mức cần → thay bằng đúng cỡ. Minecraft
                // hạ kích thước VBO theo chunk/world, slot cũ giữ peak mãi → pool
                // ăn RAM (đo bằng mem frame= … pool=). Bỏ qua slot nhỏ (tránh churn).
                if (sl.buf && sl.buf->length() >= need * 2 && sl.buf->length() >= (256 * 1024)) {
                    std::shared_ptr<metal::IBuffer> smaller =
                        device->newBuffer(need, metal::StorageMode::Shared);
                    if (smaller) {
                        sl.buf = std::move(smaller);
                        sl.usedGen = 0;
                    }
                }
                if (!sl.buf || sl.buf->length() < need) {
                    // Trần pool (TGLMT_POOL_MB, mặc định 192MB). Pool từng đo tới
                    // 320MB sau spike load chunk → góp phần chạm jetsam limit
                    // 2148MB. Vượt trần → bỏ qua slot này (slot khác có thể vừa,
                    // rốt cục rơi vào nhánh chờ) thay vì giữ thêm RAM.
                    if (opt.poolBudgetMB) {
                        const size_t budget = (size_t)opt.poolBudgetMB << 20;
                        const size_t used = PoolBytesOf(buffers);
                        const size_t cur = sl.buf ? sl.buf->length() : 0;
                        if (used > 0 && used - cur + need > budget) continue;
                    }
                    sl.buf = device->newBuffer(need, metal::StorageMode::Shared);
                    if (!sl.buf) break;
                    sl.usedGen = 0;
                }
                bo.poolNext = (bo.poolNext + i + 1) % kPool;
                target = sl.buf;
                rotated = true;
                break;
            }
            if (!rotated) {
                // Hết slot rảnh (GPU tụng lịch cao bất thường): chờ cho an toàn
                // tuyệt đối thay vì ghi đè dữ liệu đang bay.
                FlushPendingEncoder();
                CommitAndWait();
                ++appleStats.bufWaitStall;
                target = bo.gpu;
            } else {
                ++appleStats.bufRotated;
            }
        }
        // Nguồn dữ liệu đã flush trước: shadow CPU (Null/PBO) hoặc chính GPU
        // buffer cũ (Shared → đọc được). Khi shadow rỗng, `src` là bản CHƯA có
        // staged bytes → phải overlay payload sau.
        const uint8_t* src = nullptr;
        size_t srcLen = 0;
        if (!bo.data.empty()) {
            src = bo.data.data();
            srcLen = bo.data.size();
        } else if (bo.gpu && bo.gpu->contents()) {
            src = (const uint8_t*)bo.gpu->contents();
            srcLen = bo.gpu->length();
        }
        uint8_t* tgt = target ? (uint8_t*)target->contents() : nullptr;
        size_t gpuLen = target ? target->length() : 0;
        if (tgt) {
            const bool tgtIsSrc = (tgt == src);
            // Sửa bug slot xoay: slot mới (hoặc slot pool cũ) chứa RÁC ở phần không
            // staged → seed đúng phần gap từ buffer cũ TRƯỚC khi ghi payload.
            if (rotated && src && !tgtIsSrc) {
                size_t cap = std::min({need, srcLen, gpuLen});
                // Băng thông memcpy của lần xoay: mỗi lần xoay sao TOÀN BỘ buffer
                // (gap lấy từ buffer cũ + range mới) → số này lớn = FPS thấp do
                // memcpy chứ không do shader.
                appleStats.bufCopyBytes += cap;
                size_t pos = 0;
                for (auto& m : merged) {
                    if (m.off > pos) {
                        size_t g = std::min((size_t)m.off, cap);
                        if (g > pos) memcpy(tgt + pos, src + pos, g - pos);
                    }
                    size_t e = std::min({m.off + m.len, cap});
                    if (e > pos) pos = e;
                }
                if (pos < cap) memcpy(tgt + pos, src + pos, cap - pos);
            }
            for (auto& m : merged) {
                if (m.off >= srcLen || m.off >= gpuLen) continue;
                size_t n = std::min({m.len, srcLen - m.off, gpuLen - m.off});
                if (!n) continue;
                if (!tgtIsSrc) memcpy(tgt + m.off, src + m.off, n);
                target->didModifyRange(m.off, n);
            }
            // Overlay payload carry (shadow rỗng): staged bytes mới nhất đè lên GPU.
            for (auto& r : vec) {
                if (r.bytes.empty()) continue;
                if (r.off >= gpuLen) continue;
                size_t n = std::min(r.bytes.size(), gpuLen - r.off);
                if (!n) continue;
                memcpy(tgt + r.off, r.bytes.data(), n);
                target->didModifyRange(r.off, n);
            }
            if (rotated)
                for (auto& sl : bo.pool)
                    if (sl.buf == target)
                        sl.usedGen = cbCommitted.load(std::memory_order_relaxed);
            bo.gpu = target; // draw sau dùng đúng buffer vừa ghi
        }
    }
    ++bo.contentGen; // nội dung đổi thật → cache glTexBuffer phải upload lại
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
// Buffer target hợp lệ (GL 4.6 §6.1, table 6.1). Dùng để validate target cho
// các lệnh buffer (INVALID_ENUM nếu target không nằm trong bảng).
static bool IsBufferTarget(GLenum t) {
    switch (t) {
        case 0x8892:  // GL_ARRAY_BUFFER
        case 0x8893:  // GL_ELEMENT_ARRAY_BUFFER
        case 0x88EB:  // GL_PIXEL_PACK_BUFFER
        case 0x88EC:  // GL_PIXEL_UNPACK_BUFFER
        case 0x8A11:  // GL_UNIFORM_BUFFER
        case 0x8C8E:  // GL_TRANSFORM_FEEDBACK_BUFFER
        case 0x8C2A:  // GL_TEXTURE_BUFFER
        case 0x8F36:  // GL_COPY_READ_BUFFER
        case 0x8F37:  // GL_COPY_WRITE_BUFFER
        case 0x8F3F:  // GL_DRAW_INDIRECT_BUFFER
        case 0x80EE:  // GL_PARAMETER_BUFFER
        case 0x90EF:  // GL_DISPATCH_INDIRECT_BUFFER
        case 0x90D2:  // GL_SHADER_STORAGE_BUFFER
        case 0x91F2:  // GL_QUERY_BUFFER
        case 0x92C0:  // GL_ATOMIC_COUNTER_BUFFER
            return true;
        default:
            return false;
    }
}


void glGenBuffers(GLsizei n, GLuint* buffers) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Gen(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) {
        c.buffers[buffers[i]] = BufferObject{buffers[i]};
        c.buffers[buffers[i]].contentGen = c.bufGenNext++; // id duy nhất
    }
}
void glCreateBuffers(GLsizei n, GLuint* buffers) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) {
        c.buffers[buffers[i]] = BufferObject{buffers[i]};
        c.buffers[buffers[i]].contentGen = c.bufGenNext++; // id duy nhất
    }
}
void glDeleteBuffers(GLsizei n, const GLuint* buffers) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    c.registry.Delete(ObjectKind::Buffer, n, buffers);
    for (GLsizei i = 0; i < n; ++i) {
        GLuint b = buffers[i];
        c.buffers.erase(b);
        c.pendingBufRanges.erase(b);
        // GL 4.6 §6.1: "If a buffer object that is currently bound is deleted,
        // the binding reverts to 0 (the default buffer object)."  Trước đây
        // binding vẫn trỏ tới name đã xoá → glGetIntegerv(BUFFER_BINDING) trả
        // name chết, và lệnh sau đó sinh INVALID_OPERATION oan.
        c.state.UnbindBufferEverywhere(b);
        // Binding point lập chỉ mục (UBO/SSBO/atomic) cũng phải về 0.
        for (auto& kv : c.uniformBindPoints)
            if (kv.second.buffer == b) kv.second.buffer = 0;
        for (auto& kv : c.storageBindPoints)
            if (kv.second.buffer == b) kv.second.buffer = 0;
        for (auto& kv : c.atomicBindPoints)
            if (kv.second.buffer == b) kv.second.buffer = 0;
        // Element array buffer của VAO đang bind.
        for (auto& kv : c.vaos)
            if (kv.second.elementBuffer == b) kv.second.elementBuffer = 0;
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
// Dedupe log UBO bind: cùng (point,buf) lặp mọi frame → 1 dòng. Offsets biến
// đổi liên tục (DynamicTransforms 140B × ~90 offset/frame) làm cap 96 đầy
// trước → point 2/3 (Globals/Fog) không bao giờ lộ. Bỏ offset khỏi key.
static bool UboBindLogNew(GLuint point, GLuint buf, long, long) {
    static std::set<std::pair<GLuint, GLuint>> seen;
    if (seen.size() >= 64) return false;
    return seen.insert({point, buf}).second;
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    if (target == 0x8A11 /*UNIFORM_BUFFER*/) {
        c.uniformBindPoints[index] = BufferRange{buffer, 0, 0};
        // Chẩn đoán misbound UBO: chỉ khi TGLMT_DIAG=1 (release giữ 60fps).
        if (c.DiagOn() && buffer && UboBindLogNew(index, buffer, 0, 0)) {
            static int nUB = 0;
            auto it = c.buffers.find(buffer);
            size_t sz = (it == c.buffers.end()) ? 0 : BufferLen(it->second);
            fprintf(stderr, "[TGLMT] ubobind#%d point %u -> buf %u (%zuB)\n",
                    ++nUB, index, buffer, sz);
            fflush(stderr);
        }
    } else if (target == 0x90D2 /*SHADER_STORAGE_BUFFER*/) {
        c.storageBindPoints[index] = BufferRange{buffer, 0, 0};
    } else if (target == 0x92C0 /*ATOMIC_COUNTER_BUFFER*/) {
        // P9: atomic counter binding (offset lấy từ layout(offset) shader, không phải bind).
        c.atomicBindPoints[index] = BufferRange{buffer, 0, 0};
    } else if (target == 0x8C8E /*TRANSFORM_FEEDBACK_BUFFER*/) {
        if (index >= 4) { c.errors.Record(0x0501); return; }
        GLuint xid = c.state.BoundXFB();
        auto xit = c.xfbs.find(xid);
        if (xit == c.xfbs.end() && xid == 0) {
            XfbObject x; x.id = 0; xit = c.xfbs.emplace(0, x).first;
        }
        if (xit == c.xfbs.end()) { c.errors.Record(0x0502); return; }
        xit->second.buffers[index] = buffer;
        xit->second.offsets[index] = 0;
        xit->second.sizes[index] = 0;
        return; // TRANSFORM_FEEDBACK_BUFFER không phải target của glBindBuffer
    }
    glBindBuffer(target, buffer);
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr o, GLsizeiptr s) {
    Context& c = Context::Current();
    if (buffer && !c.buffers.count(buffer)) { c.errors.Record(0x0502); return; }
    // GL 4.6 §6.1: "An INVALID_VALUE error is generated if offset is less than
    // zero or size is less than or equal to zero." (size==0 chỉ hợp lệ khi
    // buffer==0 — dạng reset, offset/size bị bỏ qua theo spec).
    if (buffer) {
        if (o < 0 || s <= 0) { c.errors.Record(0x0501); return; }
        auto bit = c.buffers.find(buffer);
        const size_t blen = BufferLen(bit->second);
        if (blen && (size_t)o + (size_t)s > blen) {
            // §6.7.1: offset+size phải <= kích thước buffer.
            c.errors.Record(0x0501);
            return;
        }
    } else if (o < 0) {
        c.errors.Record(0x0501);
        return;
    }
    // §6.7.1: index vượt số binding point của target → INVALID_OPERATION.
    {
        GLint limit = -1;
        if (target == 0x8A11 /*UNIFORM_BUFFER*/) limit = 36;
        else if (target == 0x90D2 /*SHADER_STORAGE_BUFFER*/) limit = 8;
        else if (target == 0x92C0 /*ATOMIC_COUNTER_BUFFER*/) limit = 4;
        else if (target == 0x8C8E /*TRANSFORM_FEEDBACK_BUFFER*/) limit = 4;
        if (limit >= 0 && (GLint)index >= limit) { c.errors.Record(0x0502); return; }
    }
    if (target == 0x8A11) {
        c.uniformBindPoints[index] = BufferRange{buffer, o, s};
        if (c.DiagOn() && buffer && UboBindLogNew(index, buffer, (long)o, (long)s)) {
            static int nUBR = 0;
            fprintf(stderr, "[TGLMT] uborange#%d point %u -> buf %u off=%ld size=%ld\n",
                    ++nUBR, index, buffer, (long)o, (long)s);
            fflush(stderr);
        }
    } else if (target == 0x90D2) {
        c.storageBindPoints[index] = BufferRange{buffer, o, s};
    } else if (target == 0x92C0 /*ATOMIC_COUNTER_BUFFER*/) {
        c.atomicBindPoints[index] = BufferRange{buffer, o, s};
    } else if (target == 0x8C8E /*TRANSFORM_FEEDBACK_BUFFER*/) {
        if (index >= 4) { c.errors.Record(0x0501); return; }
        GLuint xid = c.state.BoundXFB();
        auto xit = c.xfbs.find(xid);
        if (xit == c.xfbs.end() && xid == 0) {
            XfbObject x; x.id = 0; xit = c.xfbs.emplace(0, x).first;
        }
        if (xit == c.xfbs.end()) { c.errors.Record(0x0502); return; }
        xit->second.buffers[index] = buffer;
        xit->second.offsets[index] = o;
        xit->second.sizes[index] = s;
        return;
    }
    glBindBuffer(target, buffer);
}
void glBindBuffersBase(GLenum t, GLuint f, GLsizei n, const GLuint* b) {
    Context& c = Context::Current();
    // GL 4.6 §6.1: INVALID_VALUE nếu count âm.
    if (n < 0) { c.errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) glBindBufferBase(t, f + i, b ? b[i] : 0);
}
void glBindBuffersRange(GLenum t, GLuint f, GLsizei n, const GLuint* b, const GLintptr* o, const GLsizeiptr* s) {
    Context& c = Context::Current();
    // GL 4.6 §6.1: INVALID_VALUE nếu count âm.
    if (n < 0) { c.errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) {
        if (!b) {
            // buffers == NULL → reset về trạng thái unbound, offset/size về
            // giá trị MẶC ĐỊNH (bỏ qua mảng offsets/sizes theo spec).
            glBindBufferRange(t, f + i, 0, 0, 0);
        } else {
            glBindBufferRange(t, f + i, b[i], o ? o[i] : 0, s ? s[i] : 0);
        }
    }
}
void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    Context& c = Context::Current();
    if (size < 0) { c.errors.Record(0x0501); return; }
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    if (bo->immutable) { c.errors.Record(0x0502); return; } // Storage rồi → INVALID_OPERATION
    // GL 4.4 §6.1 (persistent mapping): "If the buffer object is mapped, then
    // BufferData generates INVALID_OPERATION."  Trước đây lệnh này thay thế
    // bo->gpu ⇒ con trỏ mà app đang giữ (persistent map) trỏ vào vùng nhớ đã
    // giải phóng ⇒ USE-AFTER-FREE.
    if (bo->mapped) { c.errors.Record(0x0502); return; }
    // GL 4.6 §6.2, table 6.3: sau BufferData, BUFFER_STORAGE_FLAGS =
    // MAP_READ_BIT | MAP_WRITE_BIT | DYNAMIC_STORAGE_BIT. Thiếu điều này thì
    // MapBufferRange với MAP_READ/MAP_WRITE bị từ chối oan (spec §6.3 yêu cầu
    // bit tương ứng phải có trong storage flags).
    bo->storageFlags = 0x0001u /*MAP_READ_BIT*/ | 0x0002u /*MAP_WRITE_BIT*/ |
                       0x0100u /*DYNAMIC_STORAGE_BIT*/;
    bo->size = (size_t)size;
    bo->usage = usage;
    c.MaybeTrimForAllocation((size_t)size);
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    // P1 (bỏ nhân bản CPU): GPU buffer là Shared memory → bản của GPU đã là bản
    // đúng, không giữ thêm shadow. Giữ shadow khi: device Null (shadow LÀ tài
    // nguyên) hoặc upload thất bại (gpu==nullptr).
    // data==nullptr: spec undefined, trang mới của vm_allocate vốn đã zeros →
    // drop luôn. Đây chính là chỗ rò 418MB (MC allocate glBufferData(n, NULL)).
    if (bo->gpu && !c.device->isNull()) {
        std::vector<uint8_t>().swap(bo->data);
    } else {
        bo->data.assign((size_t)size, 0);
        if (data && size) memcpy(bo->data.data(), data, (size_t)size);
    }
    ++bo->contentGen;
    c.pendingBufRanges.erase(bid); // realloc → staging cũ vô nghĩa
}
void glNamedBufferData(GLuint b, GLsizeiptr size, const void* data, GLenum usage) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (it->second.immutable) { c.errors.Record(0x0502); return; }
    if (it->second.mapped) { c.errors.Record(0x0502); return; } // xem glBufferData
    GLuint saved = c.state.BoundBuffer(it->second.target ? it->second.target : 0x8892);
    c.state.BindBuffer(0x8892, b); // ARRAY_BUFFER scratch
    // trực tiếp không qua target
    it->second.size = (size_t)size;
    it->second.usage = usage;
    c.MaybeTrimForAllocation((size_t)size);
    it->second.gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                          : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    if (it->second.gpu && !c.device->isNull()) {
        std::vector<uint8_t>().swap(it->second.data);
    } else {
        it->second.data.assign(size ? (size_t)size : 0, 0);
        if (data && size) memcpy(it->second.data.data(), data, (size_t)size);
    }
    ++it->second.contentGen;
    c.pendingBufRanges.erase(b);
    c.state.BindBuffer(0x8892, saved);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    if (bo->immutable) { c.errors.Record(0x0502); return; } // đã Storage → INVALID_OPERATION
    if (bo->mapped) { c.errors.Record(0x0502); return; } // GL 4.4 §6.1 (xem glBufferData)
    // Spec §6.2: size <= 0 và flags sai → INVALID_VALUE (trước đây size âm
    // biến thành size_t khổng lồ → cấp phát vô hạn).
    if (size <= 0) { c.errors.Record(0x0501); return; }
    if (!ValidStorageFlags(flags)) { c.errors.Record(0x0501); return; }
    bo->size = (size_t)size;
    bo->storageFlags = flags;
    bo->immutable = true;
    c.MaybeTrimForAllocation((size_t)size);
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    if (bo->gpu && !c.device->isNull()) {
        std::vector<uint8_t>().swap(bo->data);
    } else {
        bo->data.assign((size_t)size, 0);
        if (data && size) memcpy(bo->data.data(), data, (size_t)size);
    }
    ++bo->contentGen;
    c.pendingBufRanges.erase(bid);
}
void glNamedBufferStorage(GLuint b, GLsizeiptr size, const void* data, GLbitfield flags) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (it->second.immutable) { c.errors.Record(0x0502); return; }
    if (it->second.mapped) { c.errors.Record(0x0502); return; } // GL 4.4 §6.1
    // Spec §6.2: size <= 0 và flags sai → INVALID_VALUE (xem glBufferStorage).
    if (size <= 0) { c.errors.Record(0x0501); return; }
    if (!ValidStorageFlags(flags)) { c.errors.Record(0x0501); return; }
    it->second.size = (size_t)size;
    it->second.storageFlags = flags;
    it->second.immutable = true;
    c.MaybeTrimForAllocation((size_t)size);
    it->second.gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                          : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
    if (it->second.gpu && !c.device->isNull()) {
        std::vector<uint8_t>().swap(it->second.data);
    } else {
        it->second.data.assign((size_t)size, 0);
        if (data && size) memcpy(it->second.data.data(), data, (size_t)size);
    }
    ++it->second.contentGen;
    c.pendingBufRanges.erase(b);
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo || !data) { if(!data) c.errors.Record(0x0501); return; }
    if (offset < 0 || size < 0 || (size_t)(offset + size) > BufferLen(*bo)) { c.errors.Record(0x0501); return; }
    // Shadow còn → ghi vào shadow như cũ. Shadow rỗng (Apple backend — P1) →
    // payload carry theo range, ghi GPU ở FlushBufferStaging.
    const uint8_t* carry = nullptr;
    if (!bo->data.empty()) {
        if ((size_t)(offset + size) > bo->data.size()) bo->data.resize((size_t)(offset + size), 0);
        memcpy(bo->data.data() + offset, data, (size_t)size);
    } else {
        carry = (const uint8_t*)data;
    }
    // IR deferred: chỉ stage range, GPU copy dồn đến flush (trước draw/readback).
    // Trước đây: memcpy GPU + didModify ngay mỗi call (10 calls → 10 Metal ops).
    StageBufRange(c, bid, (size_t)offset, (size_t)size, carry);
}
void glNamedBufferSubData(GLuint b, GLintptr off, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > BufferLen(it->second)) { c.errors.Record(0x0501); return; }
    const uint8_t* carry = nullptr;
    if (!it->second.data.empty()) {
        if ((size_t)(off + size) > it->second.data.size())
            it->second.data.resize((size_t)(off + size), 0);
        memcpy(it->second.data.data() + off, data, (size_t)size);
    } else {
        carry = (const uint8_t*)data;
    }
    // Game 26.x update buffer per-frame qua writeToBuffer → glNamedBufferSubData.
    // IR: stage thay vì sync GPU ngay (tránh stale đã fix trước đây bằng flush đúng chỗ).
    StageBufRange(c, b, (size_t)off, (size_t)size, carry);
}
void glGetBufferSubData(GLenum target, GLintptr off, GLsizeiptr size, void* data) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    if (off < 0 || size < 0 || (size_t)(off + size) > BufferLen(*bo)) { c.errors.Record(0x0501); return; }
    Context::BufferView v = c.SyncBuffer(c.state.BoundBuffer(target)); // xả staged ranges trước
    if (!v.p) { c.errors.Record(0x0502); return; }
    memcpy(data, v.p + off, (size_t)size);
}
void glGetNamedBufferSubData(GLuint b, GLintptr off, GLsizeiptr size, void* data) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > BufferLen(it->second)) { c.errors.Record(0x0501); return; }
    Context::BufferView v = c.SyncBuffer(b);
    if (!v.p) { c.errors.Record(0x0502); return; }
    memcpy(data, v.p + off, (size_t)size);
}
void glCopyBufferSubData(GLenum rt, GLenum wt, GLintptr ro, GLintptr wo, GLsizeiptr size) {
    Context& c = Context::Current();
    GLuint r = c.state.BoundBuffer(rt), w = c.state.BoundBuffer(wt);
    auto itR = c.buffers.find(r), itW = c.buffers.find(w);
    if (itR == c.buffers.end() || itW == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (ro < 0 || wo < 0 || size < 0) { c.errors.Record(0x0501); return; }
    // Metal: blit copyFromBuffer — ở Null backend copy CPU
    if ((size_t)(ro + size) > BufferLen(itR->second) || (size_t)(wo + size) > BufferLen(itW->second)) { c.errors.Record(0x0501); return; }
    // Nguồn phải sync staging trước (SyncBuffer xả range chưa ghi).
    Context::BufferView src = c.SyncBuffer(r);
    if (!src.p) { c.errors.Record(0x0502); return; }
    if (!itW->second.data.empty()) {
        memmove(itW->second.data.data() + wo, src.p + ro, (size_t)size);
        StageBufRange(c, w, (size_t)wo, (size_t)size); // shadow đích đã có payload
    } else {
        std::vector<uint8_t> tmp(src.p + ro, src.p + ro + size);
        StageBufRange(c, w, (size_t)wo, (size_t)size, tmp.data());
    }
}
void glCopyNamedBufferSubData(GLuint r, GLuint w, GLintptr ro, GLintptr wo, GLsizeiptr size) {
    Context& c = Context::Current();
    auto itR = c.buffers.find(r), itW = c.buffers.find(w);
    if (itR == c.buffers.end() || itW == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if ((size_t)(ro + size) > BufferLen(itR->second) || (size_t)(wo + size) > BufferLen(itW->second)) { c.errors.Record(0x0501); return; }
    Context::BufferView src = c.SyncBuffer(r);
    if (!src.p) { c.errors.Record(0x0502); return; }
    if (!itW->second.data.empty()) {
        memmove(itW->second.data.data() + wo, src.p + ro, (size_t)size);
        StageBufRange(c, w, (size_t)wo, (size_t)size);
    } else {
        std::vector<uint8_t> tmp(src.p + ro, src.p + ro + size);
        StageBufRange(c, w, (size_t)wo, (size_t)size, tmp.data());
    }
}
// GL 4.6 §6.3 — validate `access` của MapBufferRange/MapNamedBufferRange.
//   INVALID_VALUE     : access có bit ngoài tập hợp cho phép
//   INVALID_OPERATION : length==0; buffer đang map; không có READ cũng không có
//                       WRITE; READ + (INVALIDATE_RANGE|INVALIDATE_BUFFER|
//                       UNSYNCHRONIZED); FLUSH_EXPLICIT mà không WRITE;
//                       READ/WRITE/PERSISTENT/COHERENT mà storage flags không có
static bool MapAccessOk(Context& c, const BufferObject& bo, GLsizeiptr len, GLbitfield acc) {
    const GLbitfield kKnown = 0x0001 /*MAP_READ_BIT*/ | 0x0002 /*MAP_WRITE_BIT*/ |
                              0x0004 /*MAP_INVALIDATE_RANGE_BIT*/ |
                              0x0008 /*MAP_INVALIDATE_BUFFER_BIT*/ |
                              0x0010 /*MAP_FLUSH_EXPLICIT_BIT*/ |
                              0x0020 /*MAP_UNSYNCHRONIZED_BIT*/ |
                              0x0040 /*MAP_PERSISTENT_BIT*/ | 0x0080 /*MAP_COHERENT_BIT*/;
    if (acc & ~kKnown) { c.errors.Record(0x0501); return false; }
    if (len == 0) { c.errors.Record(0x0502); return false; }
    if (bo.mapped) { c.errors.Record(0x0502); return false; }
    const bool rd = (acc & 0x0001) != 0, wr = (acc & 0x0002) != 0;
    if (!rd && !wr) { c.errors.Record(0x0502); return false; }
    if (rd && (acc & (0x0004 | 0x0008 | 0x0020))) { c.errors.Record(0x0502); return false; }
    if ((acc & 0x0010) && !wr) { c.errors.Record(0x0502); return false; }
    const GLbitfield need = (acc & (0x0001 | 0x0002 | 0x0040 | 0x0080));
    if (need && ((need & bo.storageFlags) != need)) { c.errors.Record(0x0502); return false; }
    return true;
}

void* glMapBuffer(GLenum target, GLenum access) {
    Context& c = Context::Current();
    if (access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE) {
        c.errors.Record(0x0500);
        return nullptr;
    }
    if (!IsBufferTarget(target)) { c.errors.Record(0x0500); return nullptr; }
    if (!c.state.BoundBuffer(target)) { c.errors.Record(0x0502); return nullptr; }
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    // §6.3: MapBuffer tương đương MapBufferRange(target, 0, BUFFER_SIZE, flags)
    // với flags = MAP_READ_BIT | (MAP_WRITE_BIT nếu access cho phép ghi).
    const GLbitfield flags =
        (access == GL_READ_ONLY) ? 0x0001u : (access == GL_WRITE_ONLY) ? 0x0002u : 0x0003u;
    if (!MapAccessOk(c, *bo, (GLsizeiptr)BufferLen(*bo), flags)) return nullptr;
    // IR: flush staging trước để GPU có dữ liệu mới nhất trước khi app ghi trực tiếp.
    c.FlushBufferStaging(bid);
    bo->mapped = true; bo->mapOffset = 0; bo->mapLength = BufferLen(*bo);
    if (bo->gpu) return bo->gpu->contents();
    return bo->data.data();
}
void* glMapBufferRange(GLenum target, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    if (!IsBufferTarget(target)) { c.errors.Record(0x0500); return nullptr; }
    if (!c.state.BoundBuffer(target)) { c.errors.Record(0x0502); return nullptr; }
    GLuint bid = c.state.BoundBuffer(target);
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    if (off < 0 || len < 0 || (size_t)(off + len) > BufferLen(*bo)) { c.errors.Record(0x0501); return nullptr; }
    if (!MapAccessOk(c, *bo, len, access)) return nullptr;
    c.SyncComputeWritesTo(bid); // P1: compute đang ghi → chờ trước khi trả con trỏ
    c.FlushBufferStaging(bid);
    bo->mapped = true; bo->mapOffset = (size_t)off; bo->mapLength = (size_t)len; bo->mapAccess = access;
    // GL_ARB_map_buffer_alignment: con trỏ trả về phải "phản ánh allocation căn
    // theo MIN_MAP_BUFFER_ALIGNMENT" và (con trỏ - offset) phải là bội số của
    // alignment (spec §6.3). Offset KHÔNG bắt buộc là bội số — ARB issue 2 nêu
    // rõ offset 24 với alignment 64 vẫn hợp lệ → KHÔNG báo INVALID_VALUE, nếu
    // không sẽ phá app map sub-range ở offset lẻ (rất phổ biến). Thay vào đó ta
    // kiểm tra BASE thật có căn không: Metal contents() page-aligned, shadow CPU
    // malloc 16-aligned (= MIN_MAP_BUFFER_ALIGNMENT mà TGLMT trả).
    const size_t kAlign = 16; // GL_MIN_MAP_BUFFER_ALIGNMENT (glGetIntegerv 0x90BC)
    uint8_t* base = bo->gpu ? (uint8_t*)bo->gpu->contents() : bo->data.data();
    if (!base) return nullptr;
    if ((reinterpret_cast<uintptr_t>(base) % kAlign) != 0 && c.DiagOn())
        c.LogDebug(0, 0, 0, 0, "MapBufferRange: base lệch MIN_MAP_BUFFER_ALIGNMENT");
    return base + off;
}
void* glMapNamedBuffer(GLuint b, GLenum access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    // GL 4.6 §6.3: access phải là READ_ONLY / WRITE_ONLY / READ_WRITE.
    if (access != GL_READ_ONLY && access != GL_WRITE_ONLY && access != GL_READ_WRITE) {
        c.errors.Record(0x0500);
        return nullptr;
    }
    const GLbitfield flags =
        (access == GL_READ_ONLY) ? 0x0001u : (access == GL_WRITE_ONLY) ? 0x0002u : 0x0003u;
    if (!MapAccessOk(c, it->second, (GLsizeiptr)BufferLen(it->second), flags)) return nullptr;
    c.SyncComputeWritesTo(b); // P1
    c.FlushBufferStaging(b);
    it->second.mapped = true;
    it->second.mapOffset = 0;
    it->second.mapLength = BufferLen(it->second);
    // Trả con trỏ GPU khi có (như bản bound) để write thấy ngay trên GPU;
    // Unmap sẽ didModify + chép ngược shadow.
    if (it->second.gpu) return it->second.gpu->contents();
    return it->second.data.data();
}
void* glMapNamedBufferRange(GLuint b, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    if (off < 0 || len < 0 || (size_t)(off + len) > BufferLen(it->second)) { c.errors.Record(0x0501); return nullptr; }
    if (!MapAccessOk(c, it->second, len, access)) return nullptr;
    c.SyncComputeWritesTo(b); // P1
    c.FlushBufferStaging(b);
    it->second.mapped = true;
    it->second.mapOffset = (size_t)off;
    it->second.mapLength = (size_t)len;
    it->second.mapAccess = access;
    // Xem glMapBufferRange: offset lẻ vẫn hợp lệ (ARB issue 2), chỉ BASE
    // phải căn MIN_MAP_BUFFER_ALIGNMENT.
    const size_t kAlign = 16; // GL_MIN_MAP_BUFFER_ALIGNMENT
    uint8_t* base = it->second.gpu ? (uint8_t*)it->second.gpu->contents()
                                   : it->second.data.data();
    if (!base) return nullptr;
    if ((reinterpret_cast<uintptr_t>(base) % kAlign) != 0 && c.DiagOn())
        c.LogDebug(0, 0, 0, 0, "MapNamedBufferRange: base lệch MIN_MAP_BUFFER_ALIGNMENT");
    return base + off;
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
// GL 4.6 §6.3 — FlushMapped*BufferRange:
//   INVALID_ENUM      : target không hợp lệ (bản bound)
//   INVALID_OPERATION : zero bound to target; buffer không tồn tại; buffer KHÔNG
//                       đang map, hoặc map mà không có MAP_FLUSH_EXPLICIT_BIT
//   INVALID_VALUE     : offset/length âm, hoặc offset+length > kích thước mapping
static void FlushMappedRangeImpl(Context& c, BufferObject* bo, GLintptr off, GLsizeiptr len) {
    if (!bo) return;
    if (!bo->mapped) { c.errors.Record(0x0502); return; }
    // MAP_FLUSH_EXPLICIT_BIT là bit của MAP ACCESS (không phải storage flags).
    if (!(bo->mapAccess & 0x00000010u /*GL_MAP_FLUSH_EXPLICIT_BIT*/)) {
        c.errors.Record(0x0502);
        return;
    }
    if (off < 0 || len < 0) { c.errors.Record(0x0501); return; }
    if ((size_t)off + (size_t)len > bo->mapLength) { c.errors.Record(0x0501); return; }
    if (bo->gpu) bo->gpu->didModifyRange(bo->mapOffset + (size_t)off, (size_t)len);
}
void glFlushMappedBufferRange(GLenum target, GLintptr off, GLsizeiptr len) {
    Context& c = Context::Current();
    // target phải là buffer target hợp lệ (table 6.1).
    if (!IsBufferTarget(target)) { c.errors.Record(0x0500); return; }
    if (!c.state.BoundBuffer(target)) { c.errors.Record(0x0502); return; }
    BufferObject* bo = BoundBuf(target);
    FlushMappedRangeImpl(c, bo, off, len);
}
void glFlushMappedNamedBufferRange(GLuint b, GLintptr off, GLsizeiptr len) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    FlushMappedRangeImpl(c, &it->second, off, len);
}
// glClearBuffer*: `data` là GIÁ TRỊ theo internalformat, không phải byte.
// Voxy dùng glClearNamedBufferSubData(GL_R32UI, ..., &id) để ghi node id;
// nếu chỉ lấy byte đầu sẽ thành 0x21212121 → node id khổng lồ → GPU page fault.
static size_t ClearPatternBytes(GLenum inf) {
    switch (inf) {
        case 0x8232: case 0x8233: return 1;   // R8UI / R8I
        case 0x8234: case 0x8235: return 2;   // R16UI / R16I
        case 0x8236: case 0x8237: return 4;   // R32UI / R32I
        case 0x8229: return 1;                // R8
        case 0x822D: return 2;                // R16F
        case 0x822E: return 4;                // R32F
        case 0x8238: case 0x8239: return 2;   // RG8UI / RG8I
        case 0x823A: case 0x823B: return 4;   // RG16UI / RG16I
        case 0x823C: case 0x823D: return 8;   // RG32UI / RG32I
        case 0x8058: return 4;                // RGBA8
        case 0x8814: return 16;               // RGBA32F
        case 0x8D7C: case 0x8D7D: return 4;   // RGBA8UI / RGBA8I
        case 0x8D76: case 0x8D77: return 8;   // RGBA16UI / RGBA16I
        case 0x8D70: case 0x8D71: return 16;  // RGBA32UI / RGBA32I
        default: return 0;
    }
}
static bool ClearFormatSupported(GLenum inf) {
    const size_t es = ClearPatternBytes(inf);
    return es >= 1 && es <= 16;
}
static void FillClearRange(void* dst, size_t n, GLenum inf, const void* d) {
    size_t es = ClearPatternBytes(inf);
    if (es == 0 || es > 16) { // caller đã reject format lạ
        uint8_t f = d ? *(const uint8_t*)d : 0;
        memset(dst, f, n);
        return;
    }
    if (!d) { memset(dst, 0, n); return; }
    uint8_t pat[16] = {};
    memcpy(pat, d, es);
    for (size_t o = 0; o < n; o += es)
        memcpy((uint8_t*)dst + o, pat, std::min(es, n - o));
}
void glClearBufferData(GLenum t, GLenum inf, GLenum f, GLenum ty, const void* d) {
    (void)f; (void)ty;
    Context& c = Context::Current();
    if (!ClearFormatSupported(inf)) { c.errors.Record(0x0500); return; }
    GLuint bid = c.state.BoundBuffer(t);
    BufferObject* bo = BoundBuf(t);
    if (!bo) return;
    const size_t n = std::min(BufferLen(*bo), bo->gpu ? bo->gpu->length() : (size_t)-1);
    if (!bo->data.empty()) FillClearRange(bo->data.data(), bo->data.size(), inf, d);
    // Clear quyết định TOÀN BỘ nội dung → staged ranges cũ trở nên lỗi thời
    // (payload carry sẽ ghi đè clear). Xóa staging trước.
    c.pendingBufRanges.erase(bid);
    if (bo->gpu && n) {
        FillClearRange(bo->gpu->contents(), n, inf, d);
        bo->gpu->didModifyRange(0, n);
    }
}
void glClearBufferSubData(GLenum t, GLenum inf, GLintptr off, GLsizeiptr size, GLenum f, GLenum ty, const void* d) {
    (void)f; (void)ty;
    Context& c = Context::Current();
    if (!ClearFormatSupported(inf)) { c.errors.Record(0x0500); return; }
    GLuint bid = c.state.BoundBuffer(t);
    BufferObject* bo = BoundBuf(t);
    if (!bo) return;
    if (off < 0 || size < 0 || (size_t)(off + size) > BufferLen(*bo)) { c.errors.Record(0x0501); return; }
    const uint8_t* carry = nullptr;
    std::vector<uint8_t> tmp;
    if (!bo->data.empty()) {
        if ((size_t)(off + size) > bo->data.size()) bo->data.resize((size_t)(off + size), 0);
        FillClearRange(bo->data.data() + off, (size_t)size, inf, d);
    } else {
        tmp.assign((size_t)size, 0);
        FillClearRange(tmp.data(), (size_t)size, inf, d);
        carry = tmp.data();
    }
    StageBufRange(c, bid, (size_t)off, (size_t)size, carry);
}
void glClearNamedBufferData(GLuint b, GLenum inf, GLenum f, GLenum ty, const void* d) {
    (void)f; (void)ty;
    Context& c = Context::Current();
    if (!ClearFormatSupported(inf)) { c.errors.Record(0x0500); return; }
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    const size_t n = std::min(BufferLen(it->second),
                              it->second.gpu ? it->second.gpu->length() : (size_t)-1);
    if (!it->second.data.empty())
        FillClearRange(it->second.data.data(), it->second.data.size(), inf, d);
    c.pendingBufRanges.erase(b); // clear quyết định toàn bộ nội dung
    if (it->second.gpu && n) {
        FillClearRange(it->second.gpu->contents(), n, inf, d);
        it->second.gpu->didModifyRange(0, n);
    }
}
void glClearNamedBufferSubData(GLuint b, GLenum inf, GLintptr off, GLsizeiptr size, GLenum f, GLenum ty, const void* d) {
    (void)f; (void)ty;
    Context& c = Context::Current();
    if (!ClearFormatSupported(inf)) { c.errors.Record(0x0500); return; }
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > BufferLen(it->second)) { c.errors.Record(0x0501); return; }
    const uint8_t* carry = nullptr;
    std::vector<uint8_t> tmp;
    if (!it->second.data.empty()) {
        if ((size_t)(off + size) > it->second.data.size())
            it->second.data.resize((size_t)(off + size), 0);
        FillClearRange(it->second.data.data() + off, (size_t)size, inf, d);
    } else {
        tmp.assign((size_t)size, 0);
        FillClearRange(tmp.data(), (size_t)size, inf, d);
        carry = tmp.data();
    }
    StageBufRange(c, b, (size_t)off, (size_t)size, carry);
}
// GL 4.6 §6.5 — InvalidateBufferData/SubData:
//   INVALID_VALUE     : buffer bằng 0 hoặc không tồn tại; offset/length âm;
//                       offset+length > BUFFER_SIZE
//   INVALID_OPERATION : buffer đang map (không phải persistent)
static bool InvalidateBufferCheck(Context& c, GLuint b, GLintptr o, GLsizeiptr s) {
    if (b == 0 || !c.buffers.count(b)) { c.errors.Record(0x0501); return false; }
    if (o < 0 || s < 0) { c.errors.Record(0x0501); return false; }
    auto it = c.buffers.find(b);
    const size_t blen = BufferLen(it->second);
    if ((size_t)o + (size_t)s > blen) { c.errors.Record(0x0501); return false; }
    if (it->second.mapped && !(it->second.storageFlags & 0x00000040u /*MAP_PERSISTENT_BIT*/)) {
        c.errors.Record(0x0502);
        return false;
    }
    return true;
}
void glInvalidateBufferData(GLuint b) {
    Context& c = Context::Current();
    if (!InvalidateBufferCheck(c, b, 0, 0)) return;
    auto it = c.buffers.find(b);
    // Metal storeAction=DontCare — không cần làm gì, giữ shadow
    (void)it;
}
void glInvalidateBufferSubData(GLuint b, GLintptr o, GLsizeiptr s) {
    Context& c = Context::Current();
    if (!InvalidateBufferCheck(c, b, o, s)) return;
}
} // namespace tglmt::gl
