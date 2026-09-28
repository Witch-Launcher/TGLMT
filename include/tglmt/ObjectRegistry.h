#pragma once
// Object registry — cấp ID như glGen*/glCreate*, kiểm tra glIs*.
// Mọi object Metal thật (MTLBuffer/Texture/...) được giữ trong backend,
// registry chỉ giữ metadata + pending-delete. Thread-safe.
#include "tglmt/gl46_types.h"
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>

namespace tglmt {

enum class ObjectKind {
    Buffer, Texture, Sampler, VertexArray, Framebuffer, Renderbuffer,
    Program, Shader, ProgramPipeline, Query, Sync, TransformFeedback
};

class ObjectRegistry {
public:
    void Gen(ObjectKind kind, GLsizei n, GLuint* ids);
    void Create(ObjectKind kind, GLsizei n, GLuint* ids); // glCreate* (DSA): cấp + khởi tạo rỗng
    void Delete(ObjectKind kind, GLsizei n, const GLuint* ids);
    bool Is(ObjectKind kind, GLuint id) const;
    GLuint NextId();
    size_t LiveCount(ObjectKind kind) const;
private:
    mutable std::mutex mu_;
    GLuint next_ = 1;
    std::unordered_map<ObjectKind, std::unordered_set<GLuint>> live_;
};

} // namespace tglmt
