// test_legacy_matrix.cpp — T1: OpenGL fixed-function matrix stack.
//
// Kiểm chứng theo spec OpenGL 1.x/2.1 (giữ lại trong Compatibility Profile):
//  - Ma trận column-major 4x4, m[0..3] = cột 0.
//  - glMultMatrix: current = current * M (M biến đổi trước).
//  - glLoadTransposeMatrix: đọc row-major ⇒ transpose khi nạp.
//  - glTranslate/glRotate/glScale nhân vào bên phải.
//  - glOrtho/glFrustum CHỈ hợp lệ ở matrix mode = GL_PROJECTION.
//  - Push/Pop vượt độ sâu ⇒ GL_STACK_OVERFLOW / GL_STACK_UNDERFLOW.
//  - Ở Core Profile 3.2+ các entry point này không tồn tại ⇒ INVALID_OPERATION.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "tglmt/gl_legacy_types.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace {

// Hằng số enum lấy từ header của TGLMT (gl46_types.h cho core,
// gl_legacy_types.h cho compat-only) — KHÔNG hardcode ở đây để tránh lệch
// với registry.

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

// Error queue của OpenGL là FIFO ⇒ phải xoá hết lỗi cũ trước khi kiểm tra
// "lời gọi này sinh lỗi gì", nếu không sẽ đọc nhầm lỗi cũ.
void DrainErrors() {
    while (glGetError() != 0) {}
}

void GetMatrix(GLenum pname, GLfloat out[16]) {
    glGetFloatv(pname, out);
}

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

}  // namespace

int main() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context GL 4.6 Compatibility");
    if (!ctx) return 1;
    tglmtMakeCurrent(ctx);

    // ---- 1. Ma trận ban đầu = identity -------------------------------------
    {
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        bool id = true;
        for (int i = 0; i < 16; ++i) {
            const float want = (i % 5 == 0) ? 1.0f : 0.0f;
            if (!Near(m[i], want)) id = false;
        }
        TGLMT_CHECK(id, "modelview khởi tạo = identity");
        GLint depth = 0;
        glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
        TGLMT_CHECK(depth == 1, "độ sâu ngăn xếp ban đầu = 1");
        GLint maxDepth = 0;
        glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH, &maxDepth);
        TGLMT_CHECK(maxDepth == 32, "MAX_MODELVIEW_STACK_DEPTH = 32");
    }

    // ---- 2. glTranslatef ----------------------------------------------------
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(1.0f, 2.0f, 3.0f);
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        // M = I * T ⇒ cột 3 = (1,2,3,1)
        TGLMT_CHECK(Near(m[12], 1.0f) && Near(m[13], 2.0f) && Near(m[14], 3.0f) &&
                        Near(m[15], 1.0f),
                    "glTranslatef dịch phần tử tịnh theo cột cuối");
        TGLMT_CHECK(Near(m[0], 1.0f) && Near(m[5], 1.0f) && Near(m[10], 1.0f),
                    "glTranslatef giữ nguyên phần tuyến tính");
    }

    // ---- 3. Thứ tự nhân: current = current * M ------------------------------
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(10.0f, 0.0f, 0.0f);
        glScalef(2.0f, 2.0f, 2.0f);
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        // current = T * S ⇒ S áp trước, T sau ⇒ phần tịnh KHÔNG bị scale.
        TGLMT_CHECK(Near(m[12], 10.0f), "current = current * M (M áp trước)");
        TGLMT_CHECK(Near(m[0], 2.0f), "hệ số scale đúng");
    }
    {
        // Nếu sai thứ tự (S * T) thì phần tịnh sẽ bị scale thành 20.
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glScalef(2.0f, 2.0f, 2.0f);
        glTranslatef(10.0f, 0.0f, 0.0f);
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 20.0f),
                    "thứ tự nhân đúng: current = current * M (scale trước, translate sau)");
    }

    // ---- 4. glLoadTransposeMatrixf: nạp theo hàng ⇒ transpose --------------
    {
        // Hàng ma trận: [1 0 0 5; 0 1 0 6; 0 0 1 7; 0 0 0 1] (row-major input)
        const GLfloat rowMajor[16] = {1, 0, 0, 5,
                                       0, 1, 0, 6,
                                       0, 0, 1, 7,
                                       0, 0, 0, 1};
        glMatrixMode(GL_MODELVIEW);
        glLoadTransposeMatrixf(rowMajor);
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 5.0f) && Near(m[13], 6.0f) && Near(m[14], 7.0f),
                    "glLoadTransposeMatrixf transpose đúng (dịch theo cột cuối)");
    }

    // ---- 5. Push / Pop và giới hạn độ sâu ------------------------------------
    {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        GLint depth = 0;
        glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
        TGLMT_CHECK(depth == 2, "sau glPushMatrix độ sâu = 2");
        glTranslatef(1.0f, 0.0f, 0.0f);
        GLfloat m[16];
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 2.0f), "giá trị hiện tại đổi sau push");
        glPopMatrix();
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 1.0f), "glPopMatrix khôi phục giá trị đã push");
        glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, &depth);
        TGLMT_CHECK(depth == 1, "sau glPopMatrix độ sâu = 1");
    }
    {
        // Push tới khi đầy ⇒ GL_STACK_OVERFLOW.
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        DrainErrors();
        GLenum err = 0;
        for (int i = 0; i < 128 && err == 0; ++i) {
            glPushMatrix();
            err = glGetError();
        }
        TGLMT_CHECK(err == GL_STACK_OVERFLOW, "push vượt độ sâu ⇒ GL_STACK_OVERFLOW");
        // Pop về dưới đáy ⇒ GL_STACK_UNDERFLOW.
        DrainErrors();
        err = 0;
        for (int i = 0; i < 128 && err == 0; ++i) {
            glPopMatrix();
            err = glGetError();
        }
        TGLMT_CHECK(err == GL_STACK_UNDERFLOW, "pop dưới đáy ⇒ GL_STACK_UNDERFLOW");
    }

    // ---- 6. glOrtho / glFrustum chỉ hợp lệ ở GL_PROJECTION ------------------
    {
        glMatrixMode(GL_MODELVIEW);
        DrainErrors();
        glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glOrtho ở mode MODELVIEW ⇒ INVALID_OPERATION");
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        DrainErrors();
        glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        TGLMT_CHECK(glGetError() == 0, "glOrtho ở mode PROJECTION ⇒ không lỗi");
        GLfloat m[16];
        GetMatrix(GL_PROJECTION_MATRIX, m);
        TGLMT_CHECK(Near(m[0], 1.0f), "ortho: 2/(r-l) = 1");
        TGLMT_CHECK(Near(m[5], 1.0f), "ortho: 2/(t-b) = 1");
        TGLMT_CHECK(Near(m[10], -2.0f / 9.0f), "ortho: -2/(f-n) = -2/9");
        TGLMT_CHECK(Near(m[14], -11.0f / 9.0f), "ortho: -(f+n)/(f-n) = -11/9");
        // r == l ⇒ INVALID_VALUE
        DrainErrors();
        glOrtho(1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "ortho r==l ⇒ INVALID_VALUE");
    }
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        DrainErrors();
        glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 10.0);
        TGLMT_CHECK(glGetError() == 0, "glFrustum hợp lệ");
        GLfloat m[16];
        GetMatrix(GL_PROJECTION_MATRIX, m);
        TGLMT_CHECK(Near(m[0], 1.0f), "frustum: 2n/(r-l) = 1");
        TGLMT_CHECK(Near(m[10], -11.0f / 9.0f), "frustum: -(f+n)/(f-n)");
        TGLMT_CHECK(Near(m[14], -20.0f / 9.0f), "frustum: -2fn/(f-n) = -20/9");
    }

    // ---- 7. glMatrixMode với enum sai ⇒ INVALID_ENUM ------------------------
    {
        DrainErrors();
        glMatrixMode(0x9999);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glMatrixMode enum sai ⇒ INVALID_ENUM");
        glMatrixMode(GL_MODELVIEW);
    }

    // ---- 8. Ma trận texture là stack riêng ----------------------------------
    {
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glTranslatef(5.0f, 0.0f, 0.0f);
        GLfloat m[16];
        GetMatrix(GL_TEXTURE_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 5.0f), "texture matrix độc lập với modelview");
        GetMatrix(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(Near(m[12], 0.0f), "modelview không bị đổi bởi texture matrix");
    }

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);

    // ---- 9. Core Profile 4.6: các entry point này KHÔNG tồn tại --------------
    {
        TGLMTContextAttribs a{};
        a.majorVersion = 4;
        a.minorVersion = 6;
        a.profile = TGLMT_PROFILE_CORE;
        a.backend = "null";
        TGLMTContext* core = tglmtCreateContextAttribs(&a);
        TGLMT_CHECK(core != nullptr, "tạo được context GL 4.6 Core");
        tglmtMakeCurrent(core);
        DrainErrors();
        glMatrixMode(GL_MODELVIEW);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glMatrixMode ở GL 4.6 Core ⇒ INVALID_OPERATION");
        DrainErrors();
        GLfloat m[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, m);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glGetFloatv(GL_MODELVIEW_MATRIX) ở Core ⇒ INVALID_ENUM");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(core);
    }

    printf("[test_legacy_matrix] OK (%d lỗi)\n", tglmt_test::Failures());
    return TGLMT_RESULT;
}
