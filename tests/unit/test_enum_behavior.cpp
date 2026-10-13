// test_enum_behavior.cpp — Unit: P3 batch-4 + batch-5 enum-ext behaviors.
//
// Quy tắc vàng: ext advertise ⇒ test pass. Verify behavior THẬT:
//  1. internalformat_query: glGetInternalformativ trả INTERNALFORMAT_SUPPORTED
//     = 1 cho format map thật (RGBA8), = 0 cho format lạ; COLOR_RENDERABLE=1.
//  2. depth_clamp: glEnable(GL_DEPTH_CLAMP) → glIsEnabled = GL_TRUE.
//  3. batch-5 vertex-format: glVertexAttribPointer chấp nhận HALF_FLOAT /
//     INT_2_10_10_10_REV / UINT_2_10_10_10_REV / GL_BGRA không INVALID_ENUM
//     và store type (ToMTLVertexFormat map Metal thật).
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
static const char* kBatch5[] = {
    "GL_ARB_half_float_vertex", "GL_ARB_vertex_type_2_10_10_10_rev",
    "GL_ARB_vertex_array_bgra", "GL_EXT_vertex_array_bgra",
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

    // --- P3 batch-5: vertex-format behaviors (advertise ⇒ verify).
    for (const char* want : kBatch5) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break;        }
        }
        Check(found, want);
    }
    // Behavior: glVertexAttribPointer chấp nhận 3 type mới KHÔNG INVALID_ENUM
    // và store đúng type (ToMTLVertexFormat map thật trên Metal thật).
    while (glGetError() != 0) {} // drain
    GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glBindBuffer(0x8892 /*ARRAY_BUFFER*/, 0);
    struct C { GLenum type; GLenum want; };
    const C kCases[] = {
        {0x140B /*HALF_FLOAT*/, 0x140B},
        {0x8368 /*INT_2_10_10_10_REV*/, 0x8368},
        {0x8367 /*UNSIGNED_INT_2_10_10_10_REV*/, 0x8367},
        {0x80E1 /*GL_BGRA*/, 0x80E1},
    };
    for (const C& t : kCases) {
        glVertexAttribPointer(0, 4, t.type, GL_TRUE, 0, nullptr);
        Check(glGetError() == 0, "attrib type không INVALID_ENUM");
        GLenum got = 0;
        glGetVertexAttribiv(0, 0x8626 /*ARRAY_TYPE*/, (GLint*)&got);
        Check(got == t.want, "attrib type store đúng");
    }
    glBindVertexArray(0); glDeleteVertexArrays(1, &vao);

    if (gFails) return 1;
    printf("test_enum_behavior PASS\n");
    return 0;
}
