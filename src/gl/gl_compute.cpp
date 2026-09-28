// gl_compute.cpp — Compute: MTLComputeCommandEncoder.dispatchThreads.
// gl_memory.cpp gộp chung: MemoryBarrier → MTLBarrier/memoryBarrierWithScope.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
void glDispatchCompute(GLuint x, GLuint y, GLuint z) {
    Context& c = Context::Current();
    (void)x; (void)y; (void)z;
    // Null backend: ghi trace compute (dùng drawTrace như bằng chứng dispatch)
    c.LogDebug(0,0,0,0,"glDispatchCompute → dispatchThreads");
}
void glDispatchComputeIndirect(GLintptr ind) {
    (void)ind;
    Context::Current().LogDebug(0,0,0,0,"glDispatchComputeIndirect → dispatchThreads(indirect)");
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
