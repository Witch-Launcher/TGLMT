// test_sample_shading_state.cpp — GL_ARB_sample_shading / GL_OES_sample_shading
// + CORE GL 4.6 §14.3.1.1 (SAMPLE_SHADING enable + MIN_SAMPLE_SHADING_VALUE).
// Spec: giá trị clamp về [0,1]; query bằng GetFloatv(MIN_SAMPLE_SHADING_VALUE).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

// Alias ARB/OES (extern "C" trong gl_ext_aliases_gen.cpp) — khai báo tay như
// các test khác (test_ext_aliases.cpp) vì header gl46.h chỉ khai báo core.
extern "C" void glMinSampleShadingARB(GLfloat value);
extern "C" void glMinSampleShadingOES(GLfloat value);

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_sample_shading_state FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // Enable cap SAMPLE_SHADING (0x8C36) phải track được.
    GLboolean en = 1;
    glGetBooleanv(0x8C36, &en);
    Check(en == GL_FALSE, "SAMPLE_SHADING default off");
    glEnable(0x8C36);
    Check(glIsEnabled(0x8C36) == GL_TRUE, "glIsEnabled(SAMPLE_SHADING)");
    GLint ei = -1;
    glGetIntegerv(0x8C36, &ei);
    Check(ei == 1, "glGetIntegerv(SAMPLE_SHADING)");
    glDisable(0x8C36);
    Check(glIsEnabled(0x8C36) == GL_FALSE, "glDisable(SAMPLE_SHADING)");

    // MIN_SAMPLE_SHADING_VALUE (0x8C37) mặc định 0, clamp [0,1].
    GLfloat v = -1.0f;
    glGetFloatv(0x8C37, &v);
    Check(v >= 0.0f && v <= 1.0f, "MIN_SAMPLE_SHADING_VALUE default in range");

    glMinSampleShading(0.5f);
    Check(glGetError() == GL_NO_ERROR, "glMinSampleShading(0.5)");
    glGetFloatv(0x8C37, &v);
    Check(v > 0.499f && v < 0.501f, "MIN_SAMPLE_SHADING_VALUE round-trip");

    // Clamp trên (spec §14.3.1.1: "value is clamped to [0,1]").
    glMinSampleShading(2.0f);
    Check(glGetError() == GL_NO_ERROR, "glMinSampleShading(2.0) no error");
    glGetFloatv(0x8C37, &v);
    Check(v == 1.0f, "clamp high to 1.0");

    // Clamp dưới.
    glMinSampleShading(-1.0f);
    Check(glGetError() == GL_NO_ERROR, "glMinSampleShading(-1.0) no error");
    glGetFloatv(0x8C37, &v);
    Check(v == 0.0f, "clamp low to 0.0");

    // Alias ARB/OES (glMinSampleShadingARB/OES) phải dùng chung state.
    glMinSampleShadingARB(0.25f);
    Check(glGetError() == GL_NO_ERROR, "glMinSampleShadingARB");
    glGetFloatv(0x8C37, &v);
    Check(v > 0.249f && v < 0.251f, "ARB alias shares state");

    if (gFails == 0) { printf("test_sample_shading_state: OK\n"); return 0; }
    return 1;
}
