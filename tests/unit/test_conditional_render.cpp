// test_conditional_render.cpp — Unit: conditional draw gating.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_conditional_render FAIL: %s\n", m); ++Fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint passQ = 0, failQ = 0;
    glGenQueries(1, &passQ);
    glGenQueries(1, &failQ);

    glQueryCounter(passQ, 0x8E28 /*TIMESTAMP*/); // non-zero result
    GLuint64 ts = 0;
    glGetQueryObjectui64v(passQ, 0x8866 /*RESULT*/, &ts);
    Check(ts != 0, "timestamp result non-zero");

    glBeginQuery(0x8914 /*SAMPLES_PASSED*/, failQ);
    glEndQuery(0x8914); // Null: zero fragments
    GLuint samples = 1;
    glGetQueryObjectuiv(failQ, 0x8866, &samples);
    Check(samples == 0, "null samples result zero");

    uint64_t before = ctx.pipeStats.vertsSubmitted;
    glBeginConditionalRender(passQ, 0x8E13 /*QUERY_WAIT*/);
    glDrawArrays(0x0004 /*TRIANGLES*/, 0, 3);
    glEndConditionalRender();
    Check(glGetError() == 0, "conditional pass no error");
    Check(ctx.pipeStats.vertsSubmitted == before + 3, "zero-false draw submitted");

    before = ctx.pipeStats.vertsSubmitted;
    glBeginConditionalRender(failQ, 0x8E13);
    glDrawArrays(0x0004, 0, 3);
    glEndConditionalRender();
    Check(glGetError() == 0, "conditional fail no error");
    Check(ctx.pipeStats.vertsSubmitted == before, "false draw skipped");

    glBeginConditionalRender(failQ, 0x8E14 /*QUERY_NO_WAIT*/);
    glDrawArrays(0x0004, 0, 3);
    glEndConditionalRender();
    Check(ctx.pipeStats.vertsSubmitted == before, "no-wait false draw skipped");

    while (glGetError() != 0) {}
    glEndConditionalRender();
    Check(glGetError() == GL_INVALID_OPERATION, "end without begin rejected");
    glBeginConditionalRender(passQ, 0x1234);
    Check(glGetError() == GL_INVALID_ENUM, "bad conditional mode");

    glDeleteQueries(1, &passQ);
    glDeleteQueries(1, &failQ);
    return Fails ? 1 : 0;
}
