// test_query_objects.cpp — Unit: query targets, glGetQueryiv, query-buffer objects.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_query_objects FAIL: %s\n", m); ++Fails; }
}
static void Clear() { while (glGetError() != 0) {} }

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLint bits = -1;
    glGetQueryiv(0x8914 /*SAMPLES_PASSED*/, 0x8864 /*COUNTER_BITS*/, &bits);
    Check(bits == 1, "SAMPLES_PASSED counter bits");
    glGetQueryiv(0x88BF /*TIME_ELAPSED*/, 0x8864, &bits);
    Check(bits == 64, "TIME_ELAPSED counter bits");

    GLuint q = 0;
    glGenQueries(1, &q);
    GLint cur = -1;
    glGetQueryiv(0x8914, 0x8865 /*CURRENT_QUERY*/, &cur);
    Check(cur == 0, "no current query");
    glBeginQuery(0x8914, q);
    Check(glGetError() == 0, "begin SAMPLES_PASSED");
    glGetQueryiv(0x8914, 0x8865, &cur);
    Check(cur == (GLint)q, "current query id");
    glBeginQuery(0x8914, q);
    Check(glGetError() == GL_INVALID_OPERATION, "nested begin rejected");
    glEndQuery(0x8914);
    Check(glGetError() == 0, "end query");

    GLuint v = 0xFFFFFFFFu;
    glGetQueryObjectuiv(q, 0x8867 /*AVAILABLE*/, &v);
    Check(v == 1, "query available");
    glGetQueryObjectuiv(q, 0x8866 /*RESULT*/, &v);
    Check(v == 0, "null occlusion result");

    Clear();
    glGetQueryObjectuiv(q, 0x1234, &v);
    Check(glGetError() == GL_INVALID_ENUM, "bad query pname");
    glEndQuery(0x8914);
    Check(glGetError() == GL_INVALID_OPERATION, "end without begin");
    glBeginQuery(0x1234, q);
    Check(glGetError() == GL_INVALID_ENUM, "bad query target");

    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    glNamedBufferData(buf, 16, nullptr, 0x88E8 /*DYNAMIC_DRAW*/);
    glGetQueryBufferObjectuiv(q, buf, 0x8866, 0);
    GLuint out = 0xFFFFFFFFu;
    glGetNamedBufferSubData(buf, 0, sizeof(out), &out);
    Check(out == 0, "query result written to buffer");

    GLuint64 out64 = 0xFFFFFFFFFFFFFFFFull;
    glGetQueryBufferObjectui64v(q, buf, 0x8866, 8);
    glGetNamedBufferSubData(buf, 8, sizeof(out64), &out64);
    Check(out64 == 0, "64-bit query result written to buffer");

    glDeleteQueries(1, &q);
    glDeleteBuffers(1, &buf);
    return Fails ? 1 : 0;
}
