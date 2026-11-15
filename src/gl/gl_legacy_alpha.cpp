// gl_legacy_alpha.cpp — OpenGL fixed-function ALPHA TEST (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §4.1.4 "Alpha Test" (PDF p.385) —
// văn bản giống hệt OpenGL 2.1 §3.4.1.
//
//   void AlphaFunc( enum func, clampf ref );
//   func = NEVER | ALWAYS | LESS | LEQUAL | EQUAL | GEQUAL | GREATER | NOTEQUAL
//   ref  = reference value (clamped vào [0,1] vì clampf)
//
// State: "The required state consists of the floating-point reference value, an
// eight-valued integer indicating the comparison function, and a bit indicating
// if the comparison is enabled or disabled. The initial state is for the
// reference value to be 0 and the function to be ALWAYS. Initially, the alpha
// test is disabled." (spec p.385)
//
// LƯU Ý: GL_ALPHA_TEST là enable cap CHỈ có ở Compatibility Profile — bảng
// sinh tự động tools/gen_compat_tables.py đã chặn nó ở Core Profile
// (GL_INVALID_ENUM). Ở đây chỉ cần lưu func + ref cho đường fixed-function
// dùng (src/gl/gl_legacy_draw.cpp) và cho glGet trả lại.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cmath>

using namespace tglmt;

namespace tglmt::gl {

namespace {
// Pname lưu state: GL_ALPHA_TEST_FUNC (0x0BC1) và GL_ALPHA_TEST_REF (0x0BC2).
// Dùng StateTracker::SetShadow để glGet đọc lại được (đường gl_get.cpp đã
// có sẵn cơ chế shadow cho mọi pname).
constexpr GLenum kAlphaTestFunc = GL_ALPHA_TEST_FUNC;
constexpr GLenum kAlphaTestRef = GL_ALPHA_TEST_REF;

// func hợp lệ (spec p.385). Giá trị lấy từ registry (gl46_types.h).
bool ValidAlphaFunc(GLenum f) {
    switch (f) {
        case GL_NEVER:
        case GL_ALWAYS:
        case GL_LESS:
        case GL_LEQUAL:
        case GL_EQUAL:
        case GL_GEQUAL:
        case GL_GREATER:
        case GL_NOTEQUAL:
            return true;
        default:
            return false;
    }
}
}  // namespace

void glAlphaFunc(GLenum func, GLfloat ref) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glAlphaFunc")) return;
    if (!ValidAlphaFunc(func)) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    // ref là clampf ⇒ clamp vào [0,1].
    float r = ref;
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    GLint f = (GLint)func;
    c.state.SetShadow(kAlphaTestFunc, &f, sizeof(f));
    c.state.SetShadow(kAlphaTestRef, &r, sizeof(r));
}

}  // namespace tglmt::gl
