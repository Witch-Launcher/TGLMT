// test_framebuffer_state.cpp — Unit: FBO attach/status, clear, blend/depth, sync/query.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    tglmt::GLuint fbo, tex;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo); // FRAMEBUFFER
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    unsigned char px[4 * 64 * 64] = {0};
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, px);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    assert(glCheckFramebufferStatus(0x8D40) == 0x8CD5); // COMPLETE
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(0x00004000); // COLOR_BUFFER_BIT
    assert(ctx.clearColor[0] == 0.2f);
    glEnable(0x0BE2); // BLEND
    glBlendFunc(0x0302, 0x0303); // SRC_ALPHA, ONE_MINUS_SRC_ALPHA
    assert(ctx.state.Blend()[0].srcRGB == 0x0302);
    glPolygonOffsetClamp(1.0f, 1.0f, 0.5f); // GL 4.6 mới
    glClipControl(0x8CA0, 0x8CA1);
    // sync + query
    tglmt::GLsync s = glFenceSync(0x9116, 0);
    assert(glIsSync(s) == 1);
    assert(glClientWaitSync(s, 0, 1000) == 0x911A); // ALREADY_SIGNALED (Null)
    tglmt::GLuint q; glGenQueries(1, &q);
    glBeginQuery(0x8C2F, q); // SAMPLES_PASSED
    glEndQuery(0x8C2F);
    tglmt::GLint avail = 0; glGetQueryObjectiv(q, 0x8867, &avail);
    assert(avail == 1);
    printf("test_framebuffer_state PASS\n");
    return 0;
}
