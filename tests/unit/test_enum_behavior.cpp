// test_enum_behavior.cpp — Unit: P3 batch-4 enum-ext behaviors ĐÃ implement.
//
// Quy tắc vàng: ext advertise ⇒ test pass. Verify behavior THẬT:
//  1. internalformat_query: glGetInternalformativ trả INTERNALFORMAT_SUPPORTED
//     = 1 cho format map thật (RGBA8), = 0 cho format lạ; COLOR_RENDERABLE=1.
//  2. depth_clamp: glEnable(GL_DEPTH_CLAMP) → glIsEnabled = GL_TRUE.
// (texture_swizzle CHƯA advertise — cần texture-view swizzle trên Metal.)
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_enum_behavior FAIL: %s\n", msg); ++gFails; }
}

static const char* kBatch4[] = {
    "GL_ARB_internalformat_query", "GL_ARB_internalformat_query2",
    "GL_ARB_depth_clamp",
};

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- 1. Cả 3 ext batch-4 được advertise.
    GLint n = 0;
    glGetIntegerv(0x821D /*GL_NUM_EXTENSIONS*/, &n);
    Check(n == (GLint)tglmt_num_extensions(), "NUM_EXTENSIONS khớp");
    for (const char* want : kBatch4) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break; }
        }
        Check(found, want);
    }

    // --- 2. internalformat_query: INTERNALFORMAT_SUPPORTED.
    GLint sup = -1;
    glGetInternalformativ(0x0DE1 /*TEXTURE_2D*/, 0x8058 /*RGBA8*/,
                          0x826F /*INTERNALFORMAT_SUPPORTED*/, 1, &sup);
    Check(sup == 1, "RGBA8 supported = 1");
    GLint weird = -1;
    glGetInternalformativ(0x0DE1, 0x1234 /*format lạ*/,
                          0x826F, 1, &weird);
    Check(weird == 0, "format lạ supported = 0 (trung thực)");
    GLint colorR = -1;
    glGetInternalformativ(0x0DE1, 0x8058 /*RGBA8*/,
                          0x8286 /*COLOR_RENDERABLE*/, 1, &colorR);
    Check(colorR == 1, "RGBA8 color renderable = 1");

    // --- 3. depth_clamp: glEnable → glIsEnabled.
    glDisable(0x864F /*GL_DEPTH_CLAMP*/);
    Check(glIsEnabled(0x864F) == GL_FALSE, "depth_clamp mặc định off");
    glEnable(0x864F);
    Check(glIsEnabled(0x864F) == GL_TRUE, "glEnable(GL_DEPTH_CLAMP) → IsEnabled=TRUE");
    glDisable(0x864F);

    if (gFails) return 1;
    printf("test_enum_behavior PASS\n");
    return 0;
}
