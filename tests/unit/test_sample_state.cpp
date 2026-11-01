// test_sample_state.cpp — glSampleCoverage / glSampleMaski state.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_sample_state FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    glSampleCoverage(0.25f, GL_TRUE);
    Check(glGetError() == GL_NO_ERROR, "sample coverage");
    GLfloat v = 0;
    glGetFloatv(0x80AA /*SAMPLE_COVERAGE_VALUE*/, &v);
    Check(v > 0.249f && v < 0.251f, "coverage value query");
    GLint inv = 0;
    glGetIntegerv(0x80AB /*SAMPLE_COVERAGE_INVERT*/, &inv);
    Check(inv == 1, "coverage invert query");
    glSampleCoverage(2.0f, GL_FALSE);
    glGetFloatv(0x80AA, &v);
    Check(v == 1.0f, "coverage clamp");

    glSampleMaski(0, 0x5u);
    Check(glGetError() == GL_NO_ERROR, "sample mask word 0");
    GLint mask = 0;
    glGetIntegerv(0x8E52 /*SAMPLE_MASK_VALUE*/, &mask);
    Check(mask == 0x5, "sample mask query");
    while (glGetError() != GL_NO_ERROR) {}
    glSampleMaski(1, 0x1u);
    Check(glGetError() == GL_INVALID_VALUE, "sample mask index >= MAX_SAMPLE_MASK_WORDS");

    if (gFails == 0) { printf("test_sample_state: OK\n"); return 0; }
    return 1;
}
