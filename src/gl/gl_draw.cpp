// gl_draw.cpp — Draw calls → MTLRenderCommandEncoder.drawPrimitives/drawIndexed.
// Spec §10.4. Metal NDC z 0..1 (convert trong shader), y-flip ở viewport.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLAppleDraw.h"
using namespace tglmt;

static metal::PrimitiveType ToPrim(GLenum m, Context& c) {
    switch (m) {
        case 0x0000: return metal::PrimitiveType::Point;
        case 0x0001: return metal::PrimitiveType::Line;
        case 0x0002: return metal::PrimitiveType::LineStrip;
        case 0x0003: return metal::PrimitiveType::LineStrip; // LINE_LOOP → strip khép kín (M5b khép đỉnh cuối)
        case 0x0004: return metal::PrimitiveType::Triangle;
        case 0x0005: return metal::PrimitiveType::TriangleStrip;
        case 0x0006: return metal::PrimitiveType::Fan;
        case 0x000E: return metal::PrimitiveType::Patches;
        default: c.errors.Record(0x0500); return metal::PrimitiveType::Triangle;
    }
}
static metal::IndexType ToIndex(GLenum t, Context& c) {
    if (t == 0x1401) return metal::IndexType::UInt16; // UNSIGNED_BYTE → expand CPU lên U16
    if (t == 0x1403) return metal::IndexType::UInt16; // UNSIGNED_SHORT
    if (t == 0x1405) return metal::IndexType::UInt32; // UNSIGNED_INT
    c.errors.Record(0x0500); return metal::IndexType::UInt32;
}
static size_t IndexElemSize(GLenum t) {
    if (t == 0x1401) return 1;
    if (t == 0x1403) return 2;
    return 4;
}
// Đọc index thứ k (0-based) từ shadow EBO hoặc client pointer. Trả false nếu vượt biên.
// Hỗ trợ UBYTE (0x1401) cho vanilla (MC dùng UBYTE cho một số chunk/quad).
static bool ReadIndex(Context& c, GLuint ebo, const void* cli, GLenum type,
                      size_t byteOff, GLsizei k, uint32_t& out) {
    size_t elem = IndexElemSize(type);
    if (cli) {
        const uint8_t* p = (const uint8_t*)cli + (size_t)k * elem;
        if (elem == 1) out = *p;
        else if (elem == 2) { uint16_t v; memcpy(&v, p, 2); out = v; }
        else { uint32_t v; memcpy(&v, p, 4); out = v; }
        return true;
    }
    auto it = c.buffers.find(ebo);
    if (it == c.buffers.end()) return false;
    size_t at = byteOff + (size_t)k * elem;
    if (at + elem > it->second.data.size()) return false;
    const uint8_t* p = it->second.data.data() + at;
    if (elem == 1) out = *p;
    else if (elem == 2) out = (uint32_t)(p[0] | (p[1] << 8));
    else out = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    return true;
}
// Mở rộng UBYTE indices thành U16 buffer tạm (Metal chỉ có U16/U32).
// Trả buffer rỗng khi type không phải UBYTE.
static std::vector<uint8_t> ExpandUByteIndices(const uint8_t* src, GLsizei count) {
    std::vector<uint8_t> out((size_t)count * 2);
    for (GLsizei i = 0; i < count; ++i) {
        out[(size_t)i * 2] = src[i];
        out[(size_t)i * 2 + 1] = 0;
    }
    return out;
}

static void EmitDraw(GLenum mode, GLsizei count, GLenum type, const void* idx, GLsizei inst = 1,
                     GLint baseVertex = 0, GLuint baseInstance = 0, GLint first = 0) {
    Context& c = Context::Current();
    if (count < 0 || inst < 1) { c.errors.Record(0x0501); return; }
    if (count == 0) return; // no-op đúng spec (không encode draw-0)
    auto prim = ToPrim(mode, c);
    // Deferred full: Apple path bỏ trace-encoder thừa (1 GL → 1 Metal thay vì 2 encoders).
    // Null backend giữ trace để unit test so khớp hành vi.
    bool isApple = c.device && !c.device->isNull();
    std::shared_ptr<metal::IEncoder> enc;
    if (!isApple) enc = c.device->makeEncoder();
    else ++c.appleStats.traceSkipped;
    // type==0: glDrawArrays (non-indexed). type!=0: glDrawElements* (indexed) —
    // KỂ CẢ khi idx==nullptr vì đó là byte-offset 0 vào EBO (spec §10.4)!
    // (Bug cũ: offset 0 bị nhầm thành non-indexed → đọc lố VBO. macOS thoát nhờ
    // zero-padding, iOS đọc trúng rác → neon. Đã đối chiếu spec + ảnh thiết bị.)
    if (type == 0) {
        if (first < 0) { c.errors.Record(0x0501); return; }
        if (enc) enc->drawPrimitives(prim, (uint32_t)first, (uint32_t)count, (uint32_t)inst);
        // M5b: encode GPU thật song song với trace (thiếu điều kiện → trace-only)
        AppleDrawGL(mode, count, 0, nullptr, false, 0, 0, inst, 0, baseInstance, first);
    } else {
        // index buffer: nếu idx là offset vào ELEMENT_ARRAY_BUFFER thì dùng buffer đó,
        // ngược lại (client pointer) thì upload tạm — đúng spec §10.4
        GLuint ebo = 0;
        auto vaoIt = c.vaos.find(c.state.BoundVAO());
        if (vaoIt != c.vaos.end()) ebo = vaoIt->second.elementBuffer;
        if (!ebo) ebo = c.state.BoundBuffer(0x8893); // ELEMENT_ARRAY_BUFFER
        // Không EBO mà idx==nullptr: không có nguồn index (client pointer NULL) →
        // INVALID_OPERATION thay vì đọc bừa/đoán (đúng GL hơn drawPrimitives cũ).
        if (!idx && (ebo == 0 || c.buffers.find(ebo) == c.buffers.end())) {
            c.errors.Record(0x0502);
            return;
        }
        std::shared_ptr<metal::IBuffer> ib;
        size_t off = 0;
        auto it = c.buffers.find(ebo);
        const void* cliPtr = nullptr; // non-null khi dùng client pointer
        bool shadowUpload = false;    // true: ib là temp, offset gốc giữ trong origOff
        size_t origOff = 0;
        // UBYTE (0x1401): Metal không có → expand CPU lên U16 ngay tại đây.
        // Giữ type gốc cho ReadIndex/restart, nhưng trace/GPU dùng U16.
        GLenum effType = type;
        std::vector<uint8_t> ubExpand; // giữ sống trong suốt EmitDraw
        const void* effCliPtr = nullptr;
        if (type == 0x1401) effType = 0x1403;
        // Deferred full: Apple path không cần ib trace (AppleDrawGL tự suy từ
        // shadow/GPU + ring). Bỏ hết newBufferWithBytes ở đây → 0 alloc/draw.
        // Chỉ giữ ubExpand CPU (UBYTE→U16) vì AppleDrawGL cần U16.
        // Apple path vẫn cần off/cliPtr/effCliPtr/origOff để truyền cho AppleDrawGL.
        if (isApple) {
            auto itA = c.buffers.find(ebo);
            if (itA != c.buffers.end() && itA->second.gpu && type != 0x1401) {
                off = (size_t)idx;
            } else if (ebo && itA != c.buffers.end()) {
                size_t elem = IndexElemSize(type);
                size_t at = (size_t)idx;
                if (at + (size_t)count * elem > itA->second.data.size()) { c.errors.Record(0x0501); return; }
                if (type == 0x1401) {
                    ubExpand = ExpandUByteIndices(itA->second.data.data() + at, count);
                    effCliPtr = ubExpand.data();
                    off = 0; shadowUpload = true; origOff = at;
                } else {
                    off = 0; shadowUpload = true; origOff = at;
                }
            } else if (ebo == 0 || itA == c.buffers.end()) {
                // client pointer path (EBO rỗng): AppleDrawGL đọc trực tiếp từ idx
                if (type == 0x1401) {
                    ubExpand = ExpandUByteIndices((const uint8_t*)idx, count);
                    cliPtr = idx; effCliPtr = ubExpand.data();
                } else {
                    cliPtr = idx;
                }
                off = 0;
            }
        } else {
            if (it != c.buffers.end() && it->second.gpu && type != 0x1401) { ib = it->second.gpu; off = (size_t)idx; }
            else if (ebo && it != c.buffers.end()) {
            // EBO có shadow nhưng chưa có gpu buffer: upload shadow vùng cần vẽ
            size_t elem = IndexElemSize(type);
            size_t at = (size_t)idx;
            if (at + (size_t)count * elem > it->second.data.size()) { c.errors.Record(0x0501); return; }
            if (type == 0x1401) {
                ubExpand = ExpandUByteIndices(it->second.data.data() + at, count);
                ib = c.device->newBufferWithBytes(ubExpand.data(), ubExpand.size(), metal::StorageMode::Shared);
                off = 0; shadowUpload = true; origOff = at;
                // cliPtr giữ null để restart đọc từ shadow gốc qua ReadIndex
            } else {
                ib = c.device->newBufferWithBytes(it->second.data.data() + at,
                                                  (size_t)count * elem, metal::StorageMode::Shared);
                off = 0;
                shadowUpload = true;
                origOff = at;
            }
        } else {
            size_t elem = IndexElemSize(type);
            if (type == 0x1401) {
                ubExpand = ExpandUByteIndices((const uint8_t*)idx, count);
                ib = c.device->newBufferWithBytes(ubExpand.data(), ubExpand.size(), metal::StorageMode::Shared);
                cliPtr = idx; effCliPtr = ubExpand.data();
                off = 0;
            } else {
                ib = c.device->newBufferWithBytes(idx, (size_t)count * elem, metal::StorageMode::Shared);
                cliPtr = idx;
                off = 0;
            }
        }
        }
        // Primitive restart (spec §10.3.5, Metal không có): tách strip/fan tại
        // restart index thành nhiều draw con — hành vi raster hệt nhau.
        // A11/vanilla: chunk strip ít dùng restart, nhưng MC block outline có thể dùng.
        bool restart = c.state.IsEnabled(0x8F9D) || c.state.IsEnabled(0x8D69);
        bool stripish = (prim == metal::PrimitiveType::LineStrip ||
                         prim == metal::PrimitiveType::TriangleStrip ||
                         prim == metal::PrimitiveType::Fan);
        uint32_t rIdx = 0xFFFFFFFFu;
        if (type == 0x1401) rIdx = 0xFFu;
        else if (type == 0x1403) rIdx = 0xFFFFu;
        if (c.state.IsEnabled(0x8F9D)) {
            uint32_t custom = rIdx;
            if (c.state.GetShadow(0x8F9E, &custom, 4)) rIdx = custom;
        }
        // UBYTE restart so sánh trên giá trị gốc 8-bit (rIdx đã là 0xFF/custom&0xFF)
        if (restart && stripish && (type == 0x1401 || type == 0x1403 || type == 0x1405)) {
            GLsizei runStart = 0;
            auto flushRun = [&](GLsizei s, GLsizei n) {
                if (n <= 0) return;
                size_t elem = IndexElemSize(effType);
                if (cliPtr) {
                    const void* runPtr = (type == 0x1401)
                        ? (const void*)((const uint8_t*)effCliPtr + (size_t)s * 2)
                        : (const void*)((const uint8_t*)cliPtr + (size_t)s * IndexElemSize(type));
                    if (enc) enc->drawIndexed(prim, (uint32_t)n, ToIndex(effType, c), ib.get(),
                                     (type == 0x1401) ? (size_t)s * 2 : (size_t)s * IndexElemSize(type),
                                     (uint32_t)inst);
                    AppleDrawGL(mode, n, effType, runPtr,
                                true, 0, 0, inst, baseVertex, baseInstance, 0);
                } else {
                    // EBO-bound: ib đã là expanded-U16 hoặc shadow-upload; offset tính theo effType
                    size_t runOff;
                    if (type == 0x1401) runOff = (size_t)s * 2; // expanded buffer từ 0
                    else runOff = off + (size_t)s * IndexElemSize(type);
                    if (enc) enc->drawIndexed(prim, (uint32_t)n, ToIndex(effType, c), ib.get(),
                                     runOff, (uint32_t)inst);
                    size_t base = (type == 0x1401) ? 0 : (shadowUpload ? origOff : off);
                    size_t gpuOff = (type == 0x1401) ? (size_t)s * 2 : base + (size_t)s * IndexElemSize(type);
                    AppleDrawGL(mode, n, effType, nullptr, true, gpuOff, ebo,
                                inst, baseVertex, baseInstance, 0);
                }
            };
            for (GLsizei k = 0; k < count; ++k) {
                uint32_t v = 0;
                size_t byteOff = cliPtr ? 0 : off;
                const void* cli = cliPtr ? cliPtr : nullptr;
                // với EBO-bound: đọc từ shadow (byteOff = offset trong buffer)
                if (!cli) {
                    if (!ReadIndex(c, ebo, nullptr, type, off, k, v)) { c.errors.Record(0x0501); return; }
                } else if (!ReadIndex(c, 0, cli, type, 0, k, v)) { c.errors.Record(0x0501); return; }
                (void)byteOff;
                if (v == rIdx) { flushRun(runStart, k - runStart); runStart = k + 1; }
            }
            flushRun(runStart, count - runStart);
        } else {
            size_t traceOff = (type == 0x1401) ? 0 : off;
            if (enc) enc->drawIndexed(prim, (uint32_t)count, ToIndex(effType, c), ib.get(), traceOff, (uint32_t)inst);
            AppleDrawGL(mode, count, effType, (type == 0x1401 && cliPtr) ? effCliPtr : (cliPtr ? idx : nullptr), true,
                        (type == 0x1401) ? 0 : (cliPtr ? 0 : (shadowUpload ? origOff : off)),
                        (type == 0x1401 && cliPtr) ? 0 : (cliPtr ? 0 : ebo), inst, baseVertex, baseInstance, 0);
        }
    }
    if (enc) enc->endEncoding();
    // XFB capture hook: draw trong phiên active (không pause) cộng số đỉnh đã capture.
    // Metal không có TF native nên đây là emulation bằng đếm — tính varying thật cần M5b.
    GLuint xfb = c.state.BoundXFB();
    auto xit = c.xfbs.find(xfb);
    if (xfb && xit != c.xfbs.end() && xit->second.active && !xit->second.paused)
        xit->second.capturedCount += (uint32_t)count;
}

namespace tglmt::gl {
void glDrawArrays(GLenum m, GLint f, GLsizei c) { EmitDraw(m, c, 0, nullptr, 1, 0, 0, f); }
void glDrawArraysInstanced(GLenum m, GLint f, GLsizei c, GLsizei n) { EmitDraw(m, c, 0, nullptr, n, 0, 0, f); }
void glDrawArraysInstancedBaseInstance(GLenum m, GLint f, GLsizei c, GLsizei n, GLuint b) { EmitDraw(m, c, 0, nullptr, n, 0, b, f); }
void glDrawElements(GLenum m, GLsizei c, GLenum t, const void* i) { EmitDraw(m, c, t, i); }
void glDrawElementsInstanced(GLenum m, GLsizei c, GLenum t, const void* i, GLsizei n) { EmitDraw(m, c, t, i, n); }
void glDrawElementsBaseVertex(GLenum m, GLsizei c, GLenum t, const void* i, GLint b) { EmitDraw(m, c, t, i, 1, b); }
void glDrawElementsInstancedBaseVertex(GLenum m, GLsizei c, GLenum t, const void* i, GLsizei n, GLint b) { EmitDraw(m, c, t, i, n, b); }
void glDrawElementsInstancedBaseInstance(GLenum m, GLsizei c, GLenum t, const void* i, GLsizei n, GLuint b) { EmitDraw(m, c, t, i, n, 0, b); }
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum m, GLsizei c, GLenum t, const void* i, GLsizei n, GLint bv, GLuint bi) { EmitDraw(m, c, t, i, n, bv, bi); }
void glDrawRangeElements(GLenum m, GLuint s, GLuint e, GLsizei c, GLenum t, const void* i) { (void)s;(void)e; EmitDraw(m, c, t, i); }
void glDrawRangeElementsBaseVertex(GLenum m, GLuint s, GLuint e, GLsizei c, GLenum t, const void* i, GLint b) { (void)s;(void)e; EmitDraw(m, c, t, i, 1, b); }
// Đọc indirect command: nếu DRAW_INDIRECT_BUFFER bound thì `ind` là byte-offset
// vào buffer đó (spec §10.4), ngược lại là host pointer. Vanilla chunk multidraw
// dùng buffer-bound; Sodium indirect-count dùng PARAMETER_BUFFER.
static bool ReadArraysCmd(const void* ind, GLuint* count, GLuint* inst, GLuint* first, GLuint* baseInst) {
    Context& c = Context::Current();
    GLuint dib = c.state.BoundBuffer(0x8F3F); // DRAW_INDIRECT_BUFFER
    struct Cmd { GLuint count, instanceCount, first, baseInstance; };
    if (dib) {
        auto it = c.buffers.find(dib);
        if (it == c.buffers.end()) { c.errors.Record(0x0502); return false; }
        size_t at = (size_t)ind;
        if (at + sizeof(Cmd) > it->second.data.size()) { c.errors.Record(0x0501); return false; }
        Cmd k; memcpy(&k, it->second.data.data() + at, sizeof(k));
        *count = k.count; *inst = k.instanceCount; *first = k.first; *baseInst = k.baseInstance;
        return true;
    }
    const Cmd* k = (const Cmd*)ind;
    if (!k) { c.errors.Record(0x0501); return false; }
    *count = k->count; *inst = k->instanceCount; *first = k->first; *baseInst = k->baseInstance;
    return true;
}
static bool ReadElementsCmd(const void* ind, GLuint* count, GLuint* inst, GLuint* firstIdx,
                            GLint* baseVtx, GLuint* baseInst) {
    Context& c = Context::Current();
    GLuint dib = c.state.BoundBuffer(0x8F3F);
    struct Cmd { GLuint count, instanceCount, firstIndex, baseVertex, baseInstance; };
    if (dib) {
        auto it = c.buffers.find(dib);
        if (it == c.buffers.end()) { c.errors.Record(0x0502); return false; }
        size_t at = (size_t)ind;
        if (at + sizeof(Cmd) > it->second.data.size()) { c.errors.Record(0x0501); return false; }
        Cmd k; memcpy(&k, it->second.data.data() + at, sizeof(k));
        *count = k.count; *inst = k.instanceCount; *firstIdx = k.firstIndex;
        *baseVtx = (GLint)k.baseVertex; *baseInst = k.baseInstance;
        return true;
    }
    const Cmd* k = (const Cmd*)ind;
    if (!k) { c.errors.Record(0x0501); return false; }
    *count = k->count; *inst = k->instanceCount; *firstIdx = k->firstIndex;
    *baseVtx = (GLint)k->baseVertex; *baseInst = k->baseInstance;
    return true;
}
void glDrawArraysIndirect(GLenum m, const void* ind) {
    GLuint count = 0, inst = 1, first = 0, baseInst = 0;
    if (!ReadArraysCmd(ind, &count, &inst, &first, &baseInst)) return;
    EmitDraw(m, (GLsizei)count, 0, nullptr, (GLsizei)(inst ? inst : 1), 0, baseInst, (GLint)first);
}
void glDrawElementsIndirect(GLenum m, GLenum t, const void* ind) {
    GLuint count = 0, inst = 1, firstIdx = 0, baseInst = 0; GLint baseVtx = 0;
    if (!ReadElementsCmd(ind, &count, &inst, &firstIdx, &baseVtx, &baseInst)) return;
    // firstIndex là số index (element), phải nhân elemSize ra byte-offset (bug cũ truyền nguyên)
    size_t elem = IndexElemSize(t);
    EmitDraw(m, (GLsizei)count, t, (const void*)(firstIdx * elem),
             (GLsizei)(inst ? inst : 1), baseVtx, baseInst);
}
void glMultiDrawArrays(GLenum m, const GLint* f, const GLsizei* c, GLsizei n) {
    if (n < 0) { Context::Current().errors.Record(0x0501); return; }
    // Deferred full: N sub-draws chung 1 pendingEncoder (batching). Đếm để test chứng minh.
    for (GLsizei i = 0; i < n; ++i) { EmitDraw(m, c ? c[i] : 0, 0, nullptr, 1, 0, 0, f ? f[i] : 0); Context::Current().appleStats.multidrawBatched++; }
}
void glMultiDrawElements(GLenum m, const GLsizei* c, GLenum t, const void* const* idx, GLsizei n) {
    if (n < 0) { Context::Current().errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) { EmitDraw(m, c ? c[i] : 0, t, idx ? idx[i] : nullptr); Context::Current().appleStats.multidrawBatched++; }
}
void glMultiDrawElementsBaseVertex(GLenum m, const GLsizei* c, GLenum t, const void* const* idx, GLsizei n, const GLint* b) {
    if (n < 0) { Context::Current().errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) { EmitDraw(m, c ? c[i] : 0, t, idx ? idx[i] : nullptr, 1, b ? b[i] : 0); Context::Current().appleStats.multidrawBatched++; }
}
void glMultiDrawArraysIndirect(GLenum m, const void* ind, GLsizei dc, GLsizei s) {
    Context& c = Context::Current();
    if (dc < 0) { c.errors.Record(0x0501); return; }
    // stride 0 = tight 16 byte; stride !=0 phải >=16 (spec). Bug cũ vứt stride.
    size_t stride = (s == 0) ? 16 : (size_t)s;
    if (s != 0 && stride < 16) { c.errors.Record(0x0501); return; }
    GLuint dib = c.state.BoundBuffer(0x8F3F);
    for (GLsizei i = 0; i < dc; ++i) {
        const void* cmd;
        if (dib) cmd = (const void*)(uintptr_t)((size_t)ind + i * stride);
        else cmd = (const void*)((const uint8_t*)ind + i * stride);
        glDrawArraysIndirect(m, cmd);
    }
}
void glMultiDrawElementsIndirect(GLenum m, GLenum t, const void* ind, GLsizei dc, GLsizei s) {
    Context& c = Context::Current();
    if (dc < 0) { c.errors.Record(0x0501); return; }
    size_t stride = (s == 0) ? 20 : (size_t)s;
    if (s != 0 && stride < 20) { c.errors.Record(0x0501); return; }
    GLuint dib = c.state.BoundBuffer(0x8F3F);
    for (GLsizei i = 0; i < dc; ++i) {
        const void* cmd;
        if (dib) cmd = (const void*)(uintptr_t)((size_t)ind + i * stride);
        else cmd = (const void*)((const uint8_t*)ind + i * stride);
        glDrawElementsIndirect(m, t, cmd);
    }
}
void glMultiDrawArraysIndirectCount(GLenum m, const void* ind, GLintptr doff, GLsizei md, GLsizei s) {
    // ARB_indirect_parameters: drawcount đọc từ PARAMETER_BUFFER tại doff
    Context& c = Context::Current();
    if (md < 0 || doff < 0) { c.errors.Record(0x0501); return; }
    size_t stride = (s == 0) ? 16 : (size_t)s;
    if (s != 0 && stride < 16) { c.errors.Record(0x0501); return; }
    GLuint pb = c.state.BoundBuffer(0x80EE); // PARAMETER_BUFFER
    GLsizei dc = md;
    auto it = c.buffers.find(pb);
    if (it != c.buffers.end() && (size_t)doff + 4 <= it->second.data.size())
        memcpy(&dc, it->second.data.data() + doff, 4);
    else if (pb) { c.errors.Record(0x0502); return; }
    if (dc < 0) dc = 0;
    if (dc > md) dc = md;
    GLuint dib = c.state.BoundBuffer(0x8F3F);
    for (GLsizei i = 0; i < dc; ++i) {
        const void* cmd = dib ? (const void*)(uintptr_t)(i * stride)
                              : (const void*)((const uint8_t*)ind + i * stride);
        // khi DIB bound, `ind` là offset cơ sở: cộng thêm
        if (dib && ind) cmd = (const void*)(uintptr_t)((size_t)ind + i * stride);
        glDrawArraysIndirect(m, cmd);
    }
}
void glMultiDrawElementsIndirectCount(GLenum m, GLenum t, const void* ind, GLintptr doff, GLsizei md, GLsizei s) {
    Context& c = Context::Current();
    if (md < 0 || doff < 0) { c.errors.Record(0x0501); return; }
    size_t stride = (s == 0) ? 20 : (size_t)s;
    if (s != 0 && stride < 20) { c.errors.Record(0x0501); return; }
    GLuint pb = c.state.BoundBuffer(0x80EE);
    GLsizei dc = md;
    auto it = c.buffers.find(pb);
    if (it != c.buffers.end() && (size_t)doff + 4 <= it->second.data.size())
        memcpy(&dc, it->second.data.data() + doff, 4);
    else if (pb) { c.errors.Record(0x0502); return; }
    if (dc < 0) dc = 0;
    if (dc > md) dc = md;
    GLuint dib = c.state.BoundBuffer(0x8F3F);
    for (GLsizei i = 0; i < dc; ++i) {
        const void* cmd = dib ? (const void*)(uintptr_t)(i * stride)
                              : (const void*)((const uint8_t*)ind + i * stride);
        if (dib && ind) cmd = (const void*)(uintptr_t)((size_t)ind + i * stride);
        glDrawElementsIndirect(m, t, cmd);
    }
}
void glDrawTransformFeedback(GLenum m, GLuint id) {
    Context& c = Context::Current();
    auto it = c.xfbs.find(id);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return; }
    if (it->second.active) { c.errors.Record(0x0502); return; } // spec: XFB phải không active
    // Metal không có TF native: vẽ lại đúng số đỉnh đã capture (buffer-capture emulation).
    // Tính varying thật cần M5b thực thi program — capturedCount hiện được cập nhật ở
    // glEndTransformFeedback từ số draw đã trace trong phiên active (xấp xỉ đúng thứ tự).
    EmitDraw(m, (GLsizei)it->second.capturedCount, 0, nullptr);
}
void glDrawTransformFeedbackInstanced(GLenum m, GLuint id, GLsizei n) {
    Context& c = Context::Current();
    auto it = c.xfbs.find(id);
    if (it == c.xfbs.end()) { c.errors.Record(0x0502); return; }
    if (it->second.active) { c.errors.Record(0x0502); return; }
    EmitDraw(m, (GLsizei)it->second.capturedCount, 0, nullptr, n);
}
void glDrawTransformFeedbackStream(GLenum m, GLuint id, GLuint s) {
    (void)s;
    if (s != 0) Context::Current().LogDebug(0,0,0,0,
        "glDrawTransformFeedbackStream: stream>0 dùng chung capturedCount (M5b tách stream)");
    glDrawTransformFeedback(m, id);
}
void glDrawTransformFeedbackStreamInstanced(GLenum m, GLuint id, GLuint s, GLsizei n) {
    (void)s; (void)n; glDrawTransformFeedbackStream(m, id, s);
}
void glPrimitiveRestartIndex(GLuint i) { Context::Current().state.SetShadow(0x8F9E, &i, 4); }
void glPatchParameterfv(GLenum p, const GLfloat* v) {
    Context& c = Context::Current();
    c.state.SetShadow(p, v, 4);
    if (p == 0x8E72 && v) { // PATCH_VERTICES không phải float — ignore, dùng PatchParameteri
    }
}
void glPatchParameteri(GLenum p, GLint v) {
    Context& c = Context::Current();
    c.state.SetShadow(p, &v, 4);
    if (p == 0x8E72 /*PATCH_VERTICES*/) c.state.SetPatchVertices(v);
}
} // namespace tglmt::gl
