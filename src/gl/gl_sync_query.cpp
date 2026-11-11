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
    (void)t;
    Context& c = Context::Current();
    // GL 4.6 §6.1: "An INVALID_VALUE error is generated if flags contains a bit
    // other than GL_SYNC_FLUSH_COMMANDS_BIT." Trước đây flags bị bỏ qua.
    if (f & ~GLbitfield(0x00000001u)) { c.errors.Record(0x0501); return; }
    auto it = c.syncs.find((GLuint)(uintptr_t)s);
    if (it != c.syncs.end()) it->second.signaled = true;
}
void glGenQueries(GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) c.queries[ids[i]] = QueryObject{ids[i]};
}
static bool IsQueryTarget(GLenum t) {
    switch (t) {
        case 0x8914: case 0x8C2F: case 0x8D6A: // samples
        case 0x88BF: case 0x8E28:             // time elapsed / timestamp
        case 0x8C87: case 0x8C88:             // primitives generated / TF written
        case 0x82EE: case 0x82EF: case 0x82F0: case 0x82F1: case 0x82F2:
        case 0x82F3: case 0x82F4: case 0x82F5: case 0x82F6: case 0x82F7:
        case 0x887F: case 0x82EC: case 0x82ED:
            return true;
        default: return false;
    }
}

void glCreateQueries(GLenum t, GLsizei n, GLuint* ids) {
    Context& c = Context::Current();
    // GL 4.6 §4.2.2: INVALID_ENUM nếu target không phải query target hợp lệ;
    // INVALID_VALUE nếu n âm. Trước đây cả hai đều bị nuốt im lặng.
    if (!IsQueryTarget(t)) { c.errors.Record(0x0500); return; }
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (n > 0 && !ids) { c.errors.Record(0x0501); return; }
    c.registry.Create(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) { c.queries[ids[i]] = QueryObject{ids[i]}; c.queries[ids[i]].target = t; }
}
void glDeleteQueries(GLsizei n, const GLuint* ids) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Query, n, ids);
    for (GLsizei i = 0; i < n; ++i) c.queries.erase(ids[i]);
}
GLboolean glIsQuery(GLuint id) { return Context::Current().registry.Is(ObjectKind::Query, id) ? 1 : 0; }

// ARB_pipeline_statistics_query — đọc software counter (Context::PipeStats).
// Target theo extension spec (ARB_pipeline_statistics_query.txt New Tokens):
//   VERTICES_SUBMITTED_ARB=0x82EE, PRIMITIVES_SUBMITTED_ARB=0x82EF,
//   VERTEX_SHADER_INVOCATIONS_ARB=0x82F0, TESS_CONTROL_SHADER_PATCHES_ARB=0x82F1,
//   TESS_EVALUATION_SHADER_INVOCATIONS_ARB=0x82F2,
//   GEOMETRY_SHADER_INVOCATIONS=0x887F, GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB=0x82F3,
//   FRAGMENT_SHADER_INVOCATIONS_ARB=0x82F4, COMPUTE_SHADER_INVOCATIONS_ARB=0x82F5,
//   CLIPPING_INPUT_PRIMITIVES_ARB=0x82F6, CLIPPING_OUTPUT_PRIMITIVES_ARB=0x82F7.
// Trả false nếu target không phải statistics (fallback 0 + ready).
static bool PipeStatOf(const Context::PipeStats& s, GLenum t, GLuint64* out) {
    switch (t) {
        case 0x82EE: *out = s.vertsSubmitted; return true;      // VERTICES_SUBMITTED_ARB
        case 0x82EF: *out = s.primsSubmitted; return true;      // PRIMITIVES_SUBMITTED_ARB
        case 0x82F0: *out = s.vsInvocations; return true;       // VERTEX_SHADER_INVOCATIONS_ARB
        case 0x887F: *out = s.gsInvocations; return true;       // GEOMETRY_SHADER_INVOCATIONS
        case 0x82F3: *out = s.gsPrimsWritten; return true;      // GEOM_SHADER_PRIMITIVES_EMITTED_ARB
        case 0x82F4: *out = s.fsInvocations; return true;       // FRAGMENT_SHADER_INVOCATIONS_ARB
        case 0x82F5: *out = s.csInvocations; return true;       // COMPUTE_SHADER_INVOCATIONS_ARB
        case 0x8C87: *out = s.primsSubmitted; return true;       // PRIMITIVES_GENERATED
        case 0x8C88: *out = s.xfbPrimsWritten; return true;      // TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
        case 0x82EC: *out = s.xfbOverflow; return true;          // TRANSFORM_FEEDBACK_OVERFLOW
        case 0x82ED: *out = s.xfbStreamOverflow[0]; return true; // non-indexed = stream 0
        // TESS_CONTROL_SHADER_PATCHES / TESS_EVALUATION_SHADER_INVOCATIONS /
        // CLIPPING_* : chưa có counter thật (P6 tess) → 0, không quảng cáo ext.
        case 0x82F1: case 0x82F2: case 0x82F6: case 0x82F7:
            *out = 0; return true;
        default: return false;
    }
}

// Occlusion ANY_SAMPLES (EXT_occlusion_query_boolean) + SAMPLES_PASSED
// (ARB_occlusion_query, core 4.6): Metal visibility result buffer.
// Mỗi draw trong query ghi kết quả vào 1 slot RIÊNG (Metal ghi ĐÈ offset →
// nếu mọi draw dùng chung offset thì chỉ còn kết quả draw cuối):
//   - mode Counting (GPU family Apple3+): số sample pass depth+stencil.
//   - mode Boolean (GPU cũ): 0/1 theo draw.
// Device cộng dồn các slot → tổng của CẢ query (xem AppleMetalBridge).
static bool IsOcclusionTarget(GLenum t) {
    return t == 0x8C2F /*ANY_SAMPLES_PASSED*/ ||
           t == 0x8D6A /*ANY_SAMPLES_PASSED_CONSERVATIVE*/ ||
           t == 0x8914 /*SAMPLES_PASSED*/;
}

void glBeginQuery(GLenum t, GLuint id) {
    Context& c = Context::Current();
    if (!IsQueryTarget(t)) { c.errors.Record(0x0500); return; }
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    if (it->second.active) { c.errors.Record(0x0502); return; }
    for (auto& [qid, q] : c.queries)
        if (q.active && q.target == t) { c.errors.Record(0x0502); return; }
    it->second.target = t; it->second.active = true; it->second.ready = false;
    it->second.occlSlot = UINT64_MAX;
    GLuint64 v = 0;
    if (PipeStatOf(c.pipeStats, t, &v)) it->second.pipeSnapshot = v;
    // Occlusion: chỉ backend thật mới set visibility (Null không render → 0).
    if (IsOcclusionTarget(t) && c.device && !c.device->isNull()) {
        metal::IBuffer* vis = c.device->visibilityBuffer();
        if (vis && vis->contents()) {
            static uint64_t next = 0;
            c.occlSlot = (next++ % Context::kOcclNumSlots) * 8;
            c.occlActiveQ = id;
            it->second.occlSlot = c.occlSlot;
            // Mỗi draw của query sẽ ghi 1 slot riêng (device xoay vòng 256
            // slot) → tổng các slot = số sample của CẢ query.
            c.device->beginVisibilityQuery(id);
        }
    }
    // TIME_ELAPSED: Metal chỉ có timestamp ở ranh giới command buffer → tách
    // lệnh TRƯỚC query vào CB riêng (mốc A) để khi query xong ta đo được
    // thời gian GPU của lệnh trong query.
    if (t == 0x88BF /*TIME_ELAPSED*/ && c.device && !c.device->isNull()) {
        c.FlushPendingEncoder();
        c.device->beginTimerQuery(id);
    }
}
void glEndQuery(GLenum t) {
    Context& c = Context::Current();
    if (!IsQueryTarget(t)) { c.errors.Record(0x0500); return; }
    bool found = false;
    for (auto& [id, q] : c.queries) if (q.target == t && q.active) {
        found = true;
        q.active = false;
        if (IsOcclusionTarget(t) && q.occlSlot != UINT64_MAX) {
            // Chưa đọc được ngay — GPU có thể还在 encode. ready=false, đọc
            // sau trong glGetQueryObjectiv (CommitAndWait trước).
            q.ready = false;
            c.occlPendingRead = true;
            if (c.occlActiveQ == id) c.occlActiveQ = 0;
        } else if (t == 0x88BF /*TIME_ELAPSED*/) {
            // Tách lệnh TRONG query vào CB riêng (mốc B). Kết quả chỉ có sau
            // khi GPU xong → ready=false, đọc ở glGetQueryObject*iv.
            if (c.device && !c.device->isNull()) {
                c.FlushPendingEncoder();
                c.device->endTimerQuery(id);
            }
            q.ready = false;
        } else {
            q.ready = true;
            GLuint64 v = 0;
            if (t == 0x82ED /*TRANSFORM_FEEDBACK_STREAM_OVERFLOW*/ && q.stream < 4)
                q.result = c.pipeStats.xfbStreamOverflow[q.stream] - q.pipeSnapshot;
            else if (PipeStatOf(c.pipeStats, t, &v)) q.result = v - q.pipeSnapshot;
            else q.result = 0;
        }
    }
    if (!found) c.errors.Record(0x0502); // no active query for target
}
void glBeginQueryIndexed(GLenum t, GLuint idx, GLuint id) {
    if (t == 0x82ED /*TRANSFORM_FEEDBACK_STREAM_OVERFLOW*/) {
        Context& c = Context::Current();
        if (idx >= 4) { c.errors.Record(0x0501); return; }
        glBeginQuery(t, id);
        auto it = c.queries.find(id);
        if (it != c.queries.end()) {
            it->second.stream = idx;
            it->second.pipeSnapshot = c.pipeStats.xfbStreamOverflow[idx];
        }
        return;
    }
    glBeginQuery(t, id);
}
void glEndQueryIndexed(GLenum t, GLuint idx) { (void)idx; glEndQuery(t); }
void glQueryCounter(GLuint id, GLenum t) {
    Context& c = Context::Current();
    if (t != 0x8E28 /*TIMESTAMP*/) { c.errors.Record(0x0500); return; }
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    if (it->second.active) { c.errors.Record(0x0502); return; }
    it->second.target = t;
    // Metal GPUStartTime/GPUEndTime — Null: timestamp CPU nano
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    it->second.result = (GLuint64)now; it->second.ready = true;
    // shadow 8-byte cho glGetInteger64v(GL_TIMESTAMP) — trước đây chỉ lưu trong
    // QueryObject → glGetInteger64v phải đọc QueryObject mới ra giá trị thật.
    if (t == 0x8E28 /*GL_TIMESTAMP*/)
        c.state.SetShadow(0x8E28, &it->second.result, 8);
}
// GL_GPU_DISJOINT_EXT (0x8FBB): TRUE = số đo không đáng tin.
//  - TIME_ELAPSED: TRUE khi GPU không cho timestamp đo được.
//  - TIMESTAMP: TGLMT đo bằng đồng hồ CPU (không phải đồng hồ GPU)
//    → app không được coi nó là GPU timestamp → disjoint=TRUE.
static int DisjointFlag(Context& c, const QueryObject& q) {
    if (q.target == 0x8E28 /*TIMESTAMP*/) return 1;
    if (q.target != 0x88BF /*TIME_ELAPSED*/) return 0;
    if (!c.device || c.device->isNull()) return 1;
    return c.device->timerQueryDisjoint(q.id) ? 1 : 0;
}
// Đọc kết quả query khi GPU đã chạy xong:
//  - Occlusion: tổng số sample (visibility buffer của Metal).
//  - TIME_ELAPSED: chênh lệch 2 mốc thời gian GPU trên đồng hồ GPU.
// Backend Null (không render, không có GPU) → kết quả 0, query vẫn available.
//
// blocking=false → KHÔNG chờ GPU: chỉ đọc được nếu GPU đã xong (dùng cho
// GL_QUERY_RESULT_AVAILABLE — MC poll mỗi frame; chờ GPU ở đây = stall mỗi
// frame = mất frame rate).
static void ResolveQueryResult(Context& c, QueryObject& q, bool blocking) {
    if (q.ready) return;
    if (IsOcclusionTarget(q.target) && q.occlSlot != UINT64_MAX) {
        if (!c.device || c.device->isNull()) { q.result = 0; q.ready = true; return; }
        if (!blocking) return;   // chưa biết GPU xong chưa → để AVAILABLE=FALSE
        c.FlushPendingEncoder();   // end + commit các draw đang encode
        c.CommitAndWait();         // chờ GPU xong
        uint64_t total = c.device->visibilitySampleTotal(q.id);
        if (q.target == 0x8914 /*SAMPLES_PASSED*/) {
            // Spec §4.2.1: SAMPLES_PASSED = SỐ sample pass depth test. Metal
            // chỉ có số ở Counting mode (GPU family Apple3+); GPU cũ chỉ có
            // boolean → không có số để trả → 0 (không giả số).
            q.result = c.device->visibilityCountingSupported() ? total : 0;
        } else {
            // ANY_SAMPLES_PASSED(_CONSERVATIVE): spec yêu cầu boolean.
            q.result = total ? 1 : 0;
        }
        q.ready = true;
        return;
    }
    if (q.target == 0x88BF /*TIME_ELAPSED*/) {
        if (!c.device || c.device->isNull()) { q.result = 0; q.ready = true; return; }
        if (!blocking) {
            // Chỉ đọc timestamp khi cả hai mốc đã Completed (không chờ).
            if (!c.device->timerQueryReady(q.id)) return;
            q.result = c.device->timerQueryElapsedNs(q.id);
            q.ready = true;
            return;
        }
        c.FlushPendingEncoder();
        c.CommitAndWait();         // mốc thời gian GPU chỉ đọc được khi xong
        q.result = c.device->timerQueryElapsedNs(q.id);
        q.ready = true;
        return;
    }
    q.ready = true;
}
// GL 4.6 §4.2.2 — GetQueryObject*:
//   INVALID_OPERATION : id không phải query object, HOẶC query đang active
//   INVALID_ENUM      : pname không phải QUERY_RESULT / QUERY_RESULT_AVAILABLE /
//                       QUERY_RESULT_NO_WAIT / QUERY_TARGET
//   QUERY_TARGET (0x82EA) trả về target của query object.
void glGetQueryObjectiv(GLuint id, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    QueryObject& q = it->second;
    if (q.active) { c.errors.Record(0x0502); return; }  // query đang active
    if (p == 0x82EA /*QUERY_TARGET*/) { *v = (GLint)q.target; return; }
    // GL_QUERY_RESULT_AVAILABLE (0x8867): KHÔNG chờ GPU — chỉ báo trạng thái
    // đã biết. Spec §4.2.1 cho phép trả FALSE kể cả query đã xong, và MC
    // poll pname này MỖI FRAME (GlTimerQuery.getValue) → chờ GPU ở đây sẽ
    // stall toàn bộ pipeline mỗi frame.
    if (p == 0x8867) {
        ResolveQueryResult(c, q, /*blocking=*/false);
        *v = q.ready ? 1 : 0;
        return;
    }
    ResolveQueryResult(c, q, /*blocking=*/true);
    if (p == 0x8866) *v = (GLint)q.result;         // QUERY_RESULT
    else if (p == 0x9194) *v = q.ready ? (GLint)q.result : 0; // RESULT_NO_WAIT
    else if (p == 0x8FBB /*GL_GPU_DISJOINT_EXT*/) *v = DisjointFlag(c, q);
    else { c.errors.Record(0x0500); *v = 0; }
}
void glGetQueryObjectuiv(GLuint id, GLenum p, GLuint* v) { GLint t=0; glGetQueryObjectiv(id,p,&t); *v=(GLuint)t; }
void glGetQueryObjecti64v(GLuint id, GLenum p, GLint64* v) {
    Context& c = Context::Current();
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    QueryObject& q = it->second;
    if (q.active) { c.errors.Record(0x0502); return; }
    if (p == 0x82EA /*QUERY_TARGET*/) { *v = (GLint64)q.target; return; }
    // AVAILABLE (0x8867) → không chờ GPU (xem glGetQueryObjectiv).
    if (p == 0x8867) {
        ResolveQueryResult(c, q, /*blocking=*/false);
        *v = q.ready ? 1 : 0;
        return;
    }
    ResolveQueryResult(c, q, /*blocking=*/true);
    if (p == 0x8866) *v = (GLint64)q.result;
    else if (p == 0x9194) *v = q.ready ? (GLint64)q.result : 0;
    else if (p == 0x8FBB /*GL_GPU_DISJOINT_EXT*/) *v = DisjointFlag(c, q);
    else { c.errors.Record(0x0500); *v = 0; }
}
void glGetQueryObjectui64v(GLuint id, GLenum p, GLuint64* v) { GLint64 t=0; glGetQueryObjecti64v(id,p,&t); *v=(GLuint64)t; }
static GLint QueryCounterBits(GLenum t) {
    if (t == 0x8C2F || t == 0x8D6A || t == 0x8914) return 1;
    if (t == 0x88BF || t == 0x8E28 || t == 0x8C87 || t == 0x8C88) return 64;
    if (t == 0x82EE || t == 0x82EF || t == 0x82F0 || t == 0x82F1 || t == 0x82F2 ||
        t == 0x82F3 || t == 0x82F4 || t == 0x82F5 || t == 0x82F6 || t == 0x82F7 ||
        t == 0x887F) return 64;
    if (t == 0x82EC || t == 0x82ED) return 1;
    return 0;
}
static void QueryBufferWrite(GLuint id, GLuint buffer, GLenum p, GLintptr off, bool is64) {
    Context& c = Context::Current();
    // GL 4.6 §4.2.2: INVALID_ENUM nếu pname không hợp lệ; INVALID_OPERATION nếu
    // buffer không tồn tại; INVALID_VALUE nếu offset âm; INVALID_OPERATION nếu
    // ghi vượt quá cuối buffer.
    if (p != 0x8866 && p != 0x8867 && p != 0x9194 && p != 0x82EA /*QUERY_TARGET*/) {
        c.errors.Record(0x0500);
        return;
    }
    if (off < 0) { c.errors.Record(0x0501); return; }
    auto bit = c.buffers.find(buffer);
    if (bit == c.buffers.end()) { c.errors.Record(0x0502); return; }
    const size_t n = is64 ? sizeof(GLuint64) : sizeof(GLuint);
    const size_t bufSize = bit->second.size;  // kích thước logích của buffer
    if ((size_t)off + n > bufSize) { c.errors.Record(0x0502); return; }
    GLuint64 v = 0;
    if (is64) glGetQueryObjectui64v(id, p, &v);
    else { GLuint t = 0; glGetQueryObjectuiv(id, p, &t); v = t; }
    glNamedBufferSubData(buffer, off, (GLsizeiptr)n, &v);
}
void glGetQueryBufferObjectiv(GLuint id, GLuint b, GLenum p, GLintptr off) { QueryBufferWrite(id, b, p, off, false); }
void glGetQueryBufferObjectuiv(GLuint id, GLuint b, GLenum p, GLintptr off) { QueryBufferWrite(id, b, p, off, false); }
void glGetQueryBufferObjecti64v(GLuint id, GLuint b, GLenum p, GLintptr off) { QueryBufferWrite(id, b, p, off, true); }
void glGetQueryBufferObjectui64v(GLuint id, GLuint b, GLenum p, GLintptr off) { QueryBufferWrite(id, b, p, off, true); }
void glGetQueryiv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (!v) { c.errors.Record(0x0501); return; }
    if (!IsQueryTarget(t)) { c.errors.Record(0x0500); return; }
    if (p == 0x8865 /*CURRENT_QUERY*/) {
        *v = 0;
        for (auto& [id, q] : c.queries) if (q.active && q.target == t) { *v = (GLint)id; break; }
    } else if (p == 0x8864 /*QUERY_COUNTER_BITS*/) {
        *v = QueryCounterBits(t);
    } else { c.errors.Record(0x0500); *v = 0; }
}
// glGetQueryIndexediv — index 0 luôn OK; index >0 chỉ hợp lệ với target
// indexed (TRANSFORM_FEEDBACK_STREAM_OVERFLOW stream). Target thường:
// index phải 0 → INVALID_VALUE (spec §4.2.1).
void glGetQueryIndexediv(GLenum t, GLuint i, GLenum p, GLint* v) {
    if (v) *v = 0;
    // Indexed targets: TRANSFORM_FEEDBACK_STREAM_OVERFLOW (0x8C8F) theo stream.
    bool indexedTarget = (t == 0x8C8F /*TRANSFORM_FEEDBACK_STREAM_OVERFLOW*/ ||
                          t == 0x8E8E /*TRANSFORM_FEEDBACK_OVERFLOW?*/);
    if (!indexedTarget && i != 0) {
        Context::Current().errors.Record(0x0501 /*INVALID_VALUE*/);
        return;
    }
    glGetQueryiv(t, p, v);
    // Stream overflow: result per-query đã lưu trong QueryObject (stream field).
    if (indexedTarget && v) {
        // glGetQueryiv trả tổng — cho index i>0 trả 0 (chưa track per-stream
        // overflow riêng; stream 0 = giá trị glGetQueryiv).
        if (i != 0) *v = 0;
    }
}
void glBeginConditionalRender(GLuint id, GLenum m) {
    Context& c = Context::Current();
    if (m != 0x8E13 && m != 0x8E14 && m != 0x8E15 && m != 0x8E16) {
        c.errors.Record(0x0500); return; // WAIT / NO_WAIT / BY_REGION_*
    }
    auto it = c.queries.find(id);
    if (it == c.queries.end()) { c.errors.Record(0x0502); return; }
    if (it->second.active || c.conditionalRender.active) { c.errors.Record(0x0502); return; }
    c.conditionalRender.query = id;
    c.conditionalRender.mode = m;
    c.conditionalRender.active = true;
}
void glEndConditionalRender() {
    Context& c = Context::Current();
    if (!c.conditionalRender.active) { c.errors.Record(0x0502); return; }
    c.conditionalRender.active = false;
    c.conditionalRender.query = 0;
    c.conditionalRender.mode = 0;
}
void glFinish() { auto& c = Context::Current(); c.FlushPendingEncoder(); c.FlushAllBufferStaging(); c.FlushAllTextureStaging(); c.CommitAndWait(); }
void glFlush() { auto& c = Context::Current(); c.FlushPendingEncoder(); c.FlushAllBufferStaging(); c.FlushAllTextureStaging(); }
} // namespace tglmt::gl
