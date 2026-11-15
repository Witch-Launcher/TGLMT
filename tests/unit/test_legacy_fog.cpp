// test_legacy_fog.cpp — OpenGL fixed-function FOG (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §3.12 "Fog" (PDF p.365–367), văn bản
// giống hệt OpenGL 2.1 §3.10. Ba phương trình (spec eq. 3.29/3.30/3.31):
//   GL_EXP   (3.29):  f = exp(-d*c)
//   GL_EXP2  (3.30):  f = exp(-(d*c)^2)
//   GL_LINEAR(3.31):  f = (e - c) / (e - s)
// f được clamp vào [0,1]; fog chỉ ảnh hưởng RGB, KHÔNG đổi alpha;
// blending: C = f*Cr + (1-f)*Cf.
//
// Kiểm tra:
//  1. Mặc định theo spec (bảng state 6.16).
//  2. Round-trip glFogf/glFogi/glFogfv/glFogiv.
//  3. Lỗi: pname sai / mode sai ⇒ INVALID_ENUM; density < 0 ⇒ INVALID_VALUE.
//  4. Core Profile ⇒ entry point bị gate (INVALID_OPERATION) và pname bị
//     từ chối (INVALID_ENUM).
//  5. Compatibility Profile ⇒ chạy thật.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {
constexpr GLenum kFogMode = GL_FOG_MODE;
constexpr GLenum kFogDensity = GL_FOG_DENSITY;
constexpr GLenum kFogStart = GL_FOG_START;
constexpr GLenum kFogEnd = GL_FOG_END;
constexpr GLenum kFogColor = GL_FOG_COLOR;
constexpr GLenum kFogCoordSrc = GL_FOG_COORDINATE_SOURCE;
constexpr GLenum kFog = 0x0B60;

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }
void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}
}  // namespace

// ---------------------------------------------------------------------------
// 1. Mặc định (spec bảng state 6.16, PDF p.519)
// ---------------------------------------------------------------------------
static void TestDefaults() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    GLint mode = -1;
    glGetIntegerv(kFogMode, &mode);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetIntegerv(GL_FOG_MODE) hợp lệ");
    TGLMT_CHECK(mode == (GLint)GL_EXP, "FOG_MODE mặc định = GL_EXP [spec 4.2C p.367]");

    GLfloat density = -1.0f, start = -1.0f, end = -1.0f;
    glGetFloatv(kFogDensity, &density);
    glGetFloatv(kFogStart, &start);
    glGetFloatv(kFogEnd, &end);
    TGLMT_CHECK(Near(density, 1.0f), "FOG_DENSITY mặc định = 1.0");
    TGLMT_CHECK(Near(start, 0.0f), "FOG_START mặc định = 0.0");
    TGLMT_CHECK(Near(end, 1.0f), "FOG_END mặc định = 1.0");

    GLfloat color[4] = {1, 1, 1, 1};
    glGetFloatv(kFogColor, color);
    TGLMT_CHECK(Near(color[0], 0.0f) && Near(color[1], 0.0f) && Near(color[2], 0.0f) &&
                    Near(color[3], 0.0f),
                "FOG_COLOR mặc định = (0,0,0,0)");

    GLint src = -1;
    glGetIntegerv(kFogCoordSrc, &src);
    TGLMT_CHECK(src == (GLint)GL_FRAGMENT_DEPTH,
                "FOG_COORD_SRC mặc định = FRAGMENT_DEPTH [spec p.367]");

    GLint enabled = -1;
    glGetIntegerv(kFog, &enabled);
    TGLMT_CHECK(enabled == 0, "fog mặc định TẮT");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. Round-trip
// ---------------------------------------------------------------------------
static void TestRoundTrip() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glFogf(kFogDensity, 0.125f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogf(FOG_DENSITY)");
    GLfloat d = -1.0f;
    glGetFloatv(kFogDensity, &d);
    TGLMT_CHECK(Near(d, 0.125f), "FOG_DENSITY round-trip");

    glFogi(kFogMode, GL_LINEAR);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogi(FOG_MODE, LINEAR)");
    GLint m = -1;
    glGetIntegerv(kFogMode, &m);
    TGLMT_CHECK(m == (GLint)GL_LINEAR, "FOG_MODE round-trip");

    glFogi(kFogMode, GL_EXP2);
    glGetIntegerv(kFogMode, &m);
    TGLMT_CHECK(m == (GLint)GL_EXP2, "FOG_MODE = EXP2");

    glFogf(kFogStart, 2.5f);
    glFogf(kFogEnd, 75.0f);
    GLfloat s = -1, e = -1;
    glGetFloatv(kFogStart, &s);
    glGetFloatv(kFogEnd, &e);
    TGLMT_CHECK(Near(s, 2.5f) && Near(e, 75.0f), "FOG_START/FOG_END round-trip");

    const GLfloat col[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    glFogfv(kFogColor, col);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogfv(FOG_COLOR)");
    GLfloat gc[4] = {};
    glGetFloatv(kFogColor, gc);
    TGLMT_CHECK(Near(gc[0], 0.1f) && Near(gc[3], 0.4f), "FOG_COLOR round-trip");

    // Bản "iv" với FOG_COLOR: số nguyên → float theo Table 2.9.
    const GLint icol[4] = {0, 0, 0, 0};
    glFogiv(kFogColor, icol);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogiv(FOG_COLOR)");
    glGetFloatv(kFogColor, gc);
    TGLMT_CHECK(Near(gc[0], 0.0f), "glFogiv(FOG_COLOR) áp giá trị");

    // FOG_COORD_SRC
    glFogi(kFogCoordSrc, GL_FOG_COORD);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogi(FOG_COORD_SRC, FOG_COORD)");
    GLint src = -1;
    glGetIntegerv(kFogCoordSrc, &src);
    TGLMT_CHECK(src == (GLint)GL_FOG_COORD, "FOG_COORD_SRC round-trip");
    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. Lỗi
// ---------------------------------------------------------------------------
static void TestErrors() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glFogf(0x9999 /*pname sai*/, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glFogf pname sai → INVALID_ENUM");
    glFogi(kFogMode, 0x9999 /*mode sai*/);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glFogi FOG_MODE sai → INVALID_ENUM");
    glFogi(kFogCoordSrc, 0x9999);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glFogi FOG_COORD_SRC sai → INVALID_ENUM");
    // density < 0 ⇒ INVALID_VALUE (spec p.366).
    glFogf(kFogDensity, -0.5f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "FOG_DENSITY < 0 → INVALID_VALUE");
    // Bản scalar với pname 4 giá trị (FOG_COLOR) ⇒ INVALID_ENUM.
    glFogf(kFogColor, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glFogf scalar với FOG_COLOR (4 giá trị) → INVALID_ENUM");
    // ptr null ⇒ INVALID_VALUE
    glFogfv(kFogColor, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glFogfv với ptr null → INVALID_VALUE");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Core Profile: gate + pname reject
// ---------------------------------------------------------------------------
static void TestCoreProfile() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glFogf(kFogDensity, 0.5f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glFogf phải INVALID_OPERATION");
    glFogi(kFogMode, GL_LINEAR);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glFogi phải INVALID_OPERATION");

    GLfloat d = 0.0f;
    glGetFloatv(kFogDensity, &d);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glGetFloatv(GL_FOG_DENSITY) phải INVALID_ENUM");
    GLint m = 0;
    glGetIntegerv(kFogMode, &m);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glGetIntegerv(GL_FOG_MODE) phải INVALID_ENUM");
    // glEnable(GL_FOG) cũng phải bị từ chối ở Core.
    glEnable(kFog);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glEnable(GL_FOG) phải INVALID_ENUM");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. Compatibility Profile: chạy thật
// ---------------------------------------------------------------------------
static void TestCompatibilityProfile() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glFogf(kFogDensity, 0.25f);
    glFogi(kFogMode, GL_EXP2);
    const GLfloat col[4] = {0.5f, 0.25f, 0.125f, 1.0f};
    glFogfv(kFogColor, col);
    glEnable(kFog);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "Compatibility Profile: glFog* chạy không lỗi");
    TGLMT_CHECK(glIsEnabled(kFog) == GL_TRUE, "glEnable(GL_FOG) phản ánh vào state");

    GLfloat d = -1.0f;
    glGetFloatv(kFogDensity, &d);
    TGLMT_CHECK(Near(d, 0.25f), "Compatibility Profile: đọc lại FOG_DENSITY");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

int main() {
    TestDefaults();
    TestRoundTrip();
    TestErrors();
    TestCoreProfile();
    TestCompatibilityProfile();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_fog: OK\n");
        return 0;
    }
    return 1;
}
