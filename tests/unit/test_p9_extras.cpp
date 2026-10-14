// test_p9_extras.cpp — P9: stencil_two_side + viewport_array + atomic_counters.
// Verify behavior thật. Chạy cả 2 backend (atomic e2e chỉ Apple, Null skip phần dispatch).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace tglmt { namespace gl {
void glActiveStencilFaceEXT(GLenum face);
void glDepthRangeArraydvNV(GLuint f, GLsizei n, const GLdouble* v);
void glDepthRangeIndexeddNV(GLuint i, GLdouble n, GLdouble f);
void glDepthRangeArrayfvOES(GLuint f, GLsizei n, const GLfloat* v);
void glDepthRangeIndexedfOES(GLuint i, GLfloat n, GLfloat f);
} }

using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_p9_extras FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context& c = Context::Current();

    // ===== EXT_stencil_two_side: ActiveFace + Separate đúng =====
    {
        // Mặc định BOTH: StencilFunc set cả 2.
        glStencilFunc(0x0202 /*EQUAL*/, 1, 0xFF);
        Check(c.state.StencilFront().func == 0x0202 && c.state.StencilBack().func == 0x0202,
              "StencilFunc mặc định phải set cả 2 faces");
        // Active BACK: chỉ set Back.
        glActiveStencilFaceEXT(0x0405 /*BACK*/);
        Check(glGetError() == 0, "ActiveStencilFaceEXT BACK lỗi");
        glStencilFunc(0x0203 /*LEQUAL*/, 2, 0x0F);
        Check(c.state.StencilFront().func == 0x0202 && c.state.StencilBack().func == 0x0203,
              "ActiveFace BACK phải chỉ set Back");
        Check(c.state.StencilBack().ref == 2, "ActiveFace BACK ref sai");
        // Active FRONT: chỉ set Front.
        glActiveStencilFaceEXT(0x0404 /*FRONT*/);
        // Lưu Back sfail hiện tại để verify không đổi.
        GLenum backSfailBefore = c.state.StencilBack().sfail;
        glStencilOp(0x1E00 /*KEEP*/, 0x1E01 /*ZERO*/, 0x1E02 /*REPLACE*/);
        Check(c.state.StencilFront().sfail == 0x1E00 && c.state.StencilBack().sfail == backSfailBefore,
              "ActiveFace FRONT Op phải chỉ set Front");
        // Separate vẫn ghi đè đúng face (sửa bug 0x0409).
        glStencilFuncSeparate(0x0405 /*BACK*/, 0x0200 /*NEVER*/, 3, 0xFF);
        Check(c.state.StencilBack().func == 0x0200 && c.state.StencilFront().func == 0x0202,
              "Separate BACK sai (bug 0x0409 chưa sửa?)");
        glStencilFuncSeparate(0x0404 /*FRONT*/, 0x0201 /*LESS*/, 4, 0xFF);
        Check(c.state.StencilFront().func == 0x0201, "Separate FRONT sai");
        // Face lạ → INVALID_ENUM.
        glActiveStencilFaceEXT(0x9999);
        Check(glGetError() == 0x0500, "ActiveFace lạ phải INVALID_ENUM");
        // Reset về BOTH.
        glActiveStencilFaceEXT(0x0408);
    }

    // ===== Viewport_array forwards (NV/OES → core) =====
    {
        GLdouble d[4] = {0.0, 1.0, 0.2, 0.8};
        glDepthRangeArraydvNV(0, 2, d);
        Check(glGetError() == 0, "DepthRangeArraydvNV lỗi");
        glDepthRangeIndexeddNV(1, 0.1, 0.9);
        Check(glGetError() == 0, "DepthRangeIndexeddNV lỗi");
        GLfloat f[4] = {0.0f, 1.0f, 0.3f, 0.7f};
        glDepthRangeArrayfvOES(0, 2, f);
        Check(glGetError() == 0, "DepthRangeArrayfvOES lỗi");
        glDepthRangeIndexedfOES(0, 0.0f, 1.0f);
        Check(glGetError() == 0, "DepthRangeIndexedfOES lỗi");
    }

    // ===== Atomic limits (glGet) =====
    {
        GLint v = 0;
        glGetIntegerv(0x8265 /*MAX_COMPUTE_ATOMIC_COUNTERS*/, &v);
        Check(v == 8, "MAX_COMPUTE_ATOMIC_COUNTERS phải 8");
        glGetIntegerv(0x92D2 /*MAX_VERTEX_ATOMIC_COUNTERS*/, &v);
        Check(v == 8, "MAX_VERTEX_ATOMIC_COUNTERS phải 8");
    }

    if (gFails == 0) { printf("test_p9_extras: all OK\n"); return 0; }
    printf("test_p9_extras: %d FAIL\n", gFails);
    return 1;
}
