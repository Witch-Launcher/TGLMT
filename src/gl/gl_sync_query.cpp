// gl_sync_query.cpp — Sync (MTLEvent/MTLSharedEvent) + Query (timer/occlusion/statistics).
// Spec §4 (Sync), §18 (Queries). ARB_pipeline_statistics_query, ARB_transform_feedback_overflow_query.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <chrono>
using namespace tglmt;

namespace tglmt::gl {
GLsync glFenceSync(GLenum c_, GLbitfield f) {
    (void)c_; (void)f;
    Context& c = Context::Current();
    GLuint id; c.registry.Create(ObjectKind::Sync, 1, &id);
    c.syncs[id] = SyncObject{id, false};
    // Metal: encodeSignalEvent tại vị trí hiện tại — Null: đánh dấu chưa signal, ClientWait sẽ signal ngay
    return (GLsync)(uintptr_t)id;
}
void glDeleteSync(GLsync s) {
    Context& c = Context::Current();
    GLuint id = (GLuint)(uintptr_t)s;
    GLuint a[1]={id}; c.registry.Delete(ObjectKind::Sync, 1, a);
    c.syncs.erase(id);
}
GLboolean glIsSync(GLsync s) {
    return Context::Current().syncs.count((GLuint)(uintptr_t)s) ? 1 : 0;
}
GLenum glClientWaitSync(GLsync s, GLbitfield f, GLuint64 t) {
    (void)f; (void)t;
    Context& c = Context::Current();
    auto it = c.syncs.find((GLuint)(uintptr_t)s);
    if (it == c.syncs.end()) { c.errors.Record(0x0501); return 0x911B; } // WAIT_FAILED
    it->second.signaled = true; // Null: luôn signal ngay (ALREADY_SIGNALED)
    return 0x911A;
}
void glWaitSync(GLsync s, GLbitfield f, GLuint64 t) {
    (void)f; (void)t;
    Context& c = Context::Current();
    auto it = c.syncs.find((GLuint)(uintptr_t)s);
    if (it != c.syncs.end()) it->second.signaled = true;
}
void glGenQueries(GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) c.queries[ids[i]] = QueryObject{ids[i]};
}
void glCreateQueries(GLenum t, GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) { c.queries[ids[i]] = QueryObject{ids[i]}; c.queries[ids[i]].target = t; }
}
void glDeleteQueries(GLsizei n, const GLuint* ids) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) c.queries.erase(ids[i]);
}
GLboolean glIsQuery(GLuint id) { return Context::Current().registry.Is(ObjectKind::Query, id) ? 1 : 0; }
void glBeginQuery(GLenum t, GLuint id) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    it->second.target = t; it->second.active = true; it->second.ready = false;
}
void glEndQuery(GLenum t) {
    Context& c = Context::Current();
    for (auto& [id, q] : c.queries) if (q.target == t && q.active) {
        q.active = false; q.ready = true;
        // visibilityResultMode / GPUStartTime-EndTime → Null: result 0 + ready
        q.result = 0;
    }
}
void glBeginQueryIndexed(GLenum t, GLuint idx, GLuint id) { (void)idx; glBeginQuery(t, id); }
void glEndQueryIndexed(GLenum t, GLuint idx) { (void)idx; glEndQuery(t); }
void glQueryCounter(GLuint id, GLenum t) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    it->second.target = t;
    // Metal GPUStartTime/GPUEndTime — Null: timestamp CPU nano
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    it->second.result = (GLuint64)now; it->second.ready = true;
}
void glGetQueryObjectiv(GLuint id, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8867) *v = it->second.ready ? 1 : 0;       // QUERY_RESULT_AVAILABLE
    else if (p == 0x8866) *v = (GLint)it->second.result;  // QUERY_RESULT
    else *v = 0;
}
void glGetQueryObjectuiv(GLuint id, GLenum p, GLuint* v) { GLint t=0; glGetQueryObjectiv(id,p,&t); *v=(GLuint)t; }
void glGetQueryObjecti64v(GLuint id, GLenum p, GLint64* v) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8867) *v = it->second.ready ? 1 : 0;
    else *v = (GLint64)it->second.result;
}
void glGetQueryObjectui64v(GLuint id, GLenum p, GLuint64* v) { GLint64 t=0; glGetQueryObjecti64v(id,p,&t); *v=(GLuint64)t; }
void glGetQueryBufferObjectiv(GLuint a, GLuint b, GLenum c_, GLintptr d) { (void)a;(void)b;(void)c_;(void)d; }
void glGetQueryBufferObjectuiv(GLuint a, GLuint b, GLenum c_, GLintptr d) { (void)a;(void)b;(void)c_;(void)d; }
void glGetQueryBufferObjecti64v(GLuint a, GLuint b, GLenum c_, GLintptr d) { (void)a;(void)b;(void)c_;(void)d; }
void glGetQueryBufferObjectui64v(GLuint a, GLuint b, GLenum c_, GLintptr d) { (void)a;(void)b;(void)c_;(void)d; }
void glGetQueryiv(GLenum t, GLenum p, GLint* v) { (void)t;(void)p; *v = 0; }
void glGetQueryIndexediv(GLenum t, GLuint i, GLenum p, GLint* v) { (void)t;(void)i;(void)p; *v = 0; }
void glBeginConditionalRender(GLuint id, GLenum m) { (void)id;(void)m; }
void glEndConditionalRender() {}
void glFinish() { Context::Current().FlushPendingEncoder(); Context::Current().device->commitAndWait(); }
void glFlush() { Context::Current().FlushPendingEncoder(); }
} // namespace tglmt::gl
