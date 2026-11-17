// test_regression_default_context_gate.cpp — Hồi quy: Context tạo trực tiếp
// phải gọi được extern "C" API (LWJGL/launcher path qua Renderer/GLFWShim).
//
// Lỗi đã sửa:
//  R-gate: mọi Context tạo trực tiếp bằng `Context(backend)` (đường
//  Renderer/GLFWShim, fallback context, test) có glGateMask TOÀN 0 vì
//  SetVersionProfile không bao giờ được gọi ⇒ VersionGate từ chối MỌI entry
//  point ở lớp C ABI (gl_c_exports.cpp) ⇒ ghi GL_INVALID_OPERATION (0x502)
//  và bỏ qua lời gọi. Hậu quả ghi nhận trên iPhone 8 Plus: LWJGL
//  GL.createCapabilities() thấy glGetError()!=0 và GL_MAJOR_VERSION=0 ⇒
//  ném IllegalStateException "There is no OpenGL context current in the
//  current thread" (latestlog.txt / crash-2026-11-17_14.27.15-client.txt).
//  Fix: Context constructor dựng gate mask mặc định (4.6 Core, đúng với
//  glMajor/glMinor/glProfile/glVersionSlot mặc định trong Context.h).
//
// Test này gọi thẳng PUBLIC C ABI (gl_c_api.h) — lớp duy nhất có version-gate,
// và là lớp LWJGL dùng qua dlsym.
#include "tglmt/gl_c_api.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstring>

using namespace tglmt;

int main() {
    // ---- 1. Context mặc định (như Renderer tạo) phải cho qua gate ----------
    {
        Context ctx("null");
        Context::MakeCurrent(&ctx);
        // Dọn mọi lỗi cũ rồi mới kiểm tra.
        while (glGetError() != 0) {
        }
        GLenum e = glGetError();
        TGLMT_CHECK(e == 0, "Context mặc định: glGetError()==0 (không INVALID_OPERATION)");

        GLint maj = 0, min = 0;
        glGetIntegerv(0x821B /*GL_MAJOR_VERSION*/, &maj);
        glGetIntegerv(0x821C /*GL_MINOR_VERSION*/, &min);
        TGLMT_CHECK(glGetError() == 0, "Context mặc định: glGetIntegerv không sinh lỗi");
        TGLMT_CHECK(maj == 4 && min == 6, "Context mặc định = GL 4.6 (theo Context.h defaults)");

        const GLubyte* v = glGetString(0x1F02 /*GL_VERSION*/);
        TGLMT_CHECK(v != nullptr, "Context mặc định: glGetString(GL_VERSION) != NULL");
        TGLMT_CHECK(glGetError() == 0, "Context mặc định: glGetString không sinh lỗi");

        Context::MakeCurrent(nullptr);
    }
    // ---- 2. Fallback context (Context::Current khi chưa MakeCurrent) -------
    // Context::Current() tạo sFallback_ trực tiếp — cũng phải qua gate được.
    {
        while (glGetError() != 0) {
        }
        Context& fb = Context::Current();
        Context::MakeCurrent(&fb);
        GLenum e = glGetError();
        TGLMT_CHECK(e == 0, "Fallback context: glGetError()==0");
        Context::MakeCurrent(nullptr);
    }
    // ---- 3. SetVersionProfile vẫn ghi đè mask đúng cách (idempotent) -------
    {
        Context ctx("null");
        Context::MakeCurrent(&ctx);
        const bool ok = ctx.SetVersionProfile(2, 1, GlProfile::Legacy, 0);
        TGLMT_CHECK(ok, "SetVersionProfile(2.1 Legacy) hợp lệ");
        // Ở context 2.1, entry point của GL 4.6 phải bị gate từ chối.
        while (glGetError() != 0) {
        }
        GLuint buf = 0;
        glGenBuffers(1, &buf); // GL 1.5 ⇒ hợp lệ ở 2.1
        TGLMT_CHECK(glGetError() == 0, "2.1: glGenBuffers (GL1.5) cho qua");
        glGenVertexArrays(1, nullptr); // GL 3.0 ⇒ không hợp lệ ở 2.1
        TGLMT_CHECK(glGetError() == 0x0502, "2.1: glGenVertexArrays (GL3.0) → INVALID_OPERATION");
        Context::MakeCurrent(nullptr);
    }
    return TGLMT_RESULT;
}
