// gl_buffer.cpp — Buffer objects: Metal MTLBuffer via IDevice + shadow CPU.
// Spec: glspec46.core.pdf §6 (Buffer Objects). Metal: newBuffer/newBufferWithBytes,
// contents()+memcpy, didModifyRange, blit copyFromBuffer.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
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
    for (GLsizei i = 0; i < n; ++i) c.buffers.erase(buffers[i]);
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
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    bo->data.assign((const uint8_t*)(data ? data : nullptr), (const uint8_t*)(data ? data : nullptr) + (data ? size : 0));
    if (!data) bo->data.assign((size_t)size, 0);
    bo->usage = usage;
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
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
    c.state.BindBuffer(0x8892, saved);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return;
    bo->data.assign((size_t)size, 0);
    if (data) memcpy(bo->data.data(), data, (size_t)size);
    bo->storageFlags = flags;
    bo->gpu = data ? c.device->newBufferWithBytes(data, (size_t)size, metal::StorageMode::Shared)
                   : c.device->newBuffer((size_t)size, metal::StorageMode::Shared);
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
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo || !data) { if(!data) c.errors.Record(0x0501); return; }
    if (offset < 0 || size < 0 || (size_t)(offset + size) > bo->data.size()) { c.errors.Record(0x0501); return; }
    memcpy(bo->data.data() + offset, data, (size_t)size);
    if (bo->gpu && bo->gpu->length() >= (size_t)(offset + size)) {
        memcpy((uint8_t*)bo->gpu->contents() + offset, data, (size_t)size);
        bo->gpu->didModifyRange((size_t)offset, (size_t)size);
    }
}
void glNamedBufferSubData(GLuint b, GLintptr off, GLsizeiptr size, const void* data) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (off < 0 || size < 0 || (size_t)(off + size) > it->second.data.size()) { c.errors.Record(0x0501); return; }
    memcpy(it->second.data.data() + off, data, (size_t)size);
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
    memmove(itW->second.data.data() + wo, itR->second.data.data() + ro, (size_t)size);
}
void glCopyNamedBufferSubData(GLuint r, GLuint w, GLintptr ro, GLintptr wo, GLsizeiptr size) {
    Context& c = Context::Current();
    auto itR = c.buffers.find(r), itW = c.buffers.find(w);
    if (itR == c.buffers.end() || itW == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if ((size_t)(ro + size) > itR->second.data.size() || (size_t)(wo + size) > itW->second.data.size()) { c.errors.Record(0x0501); return; }
    memmove(itW->second.data.data() + wo, itR->second.data.data() + ro, (size_t)size);
}
void* glMapBuffer(GLenum target, GLenum access) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    (void)access;
    bo->mapped = true; bo->mapOffset = 0; bo->mapLength = bo->data.size();
    if (bo->gpu) return bo->gpu->contents();
    return bo->data.data();
}
void* glMapBufferRange(GLenum target, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return nullptr;
    if (off < 0 || len < 0 || (size_t)(off + len) > bo->data.size()) { c.errors.Record(0x0501); return nullptr; }
    bo->mapped = true; bo->mapOffset = (size_t)off; bo->mapLength = (size_t)len; bo->mapAccess = access;
    if (bo->gpu) return (uint8_t*)bo->gpu->contents() + off;
    return bo->data.data() + off;
}
void* glMapNamedBuffer(GLuint b, GLenum access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    (void)access; it->second.mapped = true;
    return it->second.data.data();
}
void* glMapNamedBufferRange(GLuint b, GLintptr off, GLsizeiptr len, GLbitfield access) {
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return nullptr; }
    (void)access;
    if (off < 0 || len < 0 || (size_t)(off + len) > it->second.data.size()) { c.errors.Record(0x0501); return nullptr; }
    it->second.mapped = true;
    return it->second.data.data() + off;
}
GLboolean glUnmapBuffer(GLenum target) {
    Context& c = Context::Current();
    BufferObject* bo = BoundBuf(target);
    if (!bo) return 0;
    bo->mapped = false;
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
    BufferObject* bo = BoundBuf(t);
    if (!bo) return;
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    if (off < 0 || size < 0 || (size_t)(off + size) > bo->data.size()) { c.errors.Record(0x0501); return; }
    std::fill(bo->data.begin() + off, bo->data.begin() + off + size, fill);
    if (bo->gpu && (size_t)(off + size) <= bo->gpu->length()) {
        memset((uint8_t*)bo->gpu->contents() + off, fill, (size_t)size);
        bo->gpu->didModifyRange((size_t)off, (size_t)size);
    }
}
void glClearNamedBufferData(GLuint b, GLenum inf, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    std::fill(it->second.data.begin(), it->second.data.end(), fill);
}
void glClearNamedBufferSubData(GLuint b, GLenum inf, GLintptr off, GLsizeiptr size, GLenum f, GLenum ty, const void* d) {
    (void)inf; (void)f; (void)ty;
    Context& c = Context::Current();
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    uint8_t fill = d ? *(const uint8_t*)d : 0;
    if (off < 0 || size < 0 || (size_t)(off + size) > it->second.data.size()) { c.errors.Record(0x0501); return; }
    std::fill(it->second.data.begin() + off, it->second.data.begin() + off + size, fill);
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
