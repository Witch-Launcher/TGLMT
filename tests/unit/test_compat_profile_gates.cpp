// test_compat_profile_gates.cpp — Core Profile phải TỪ CHỐI state
// fixed-function, còn Compatibility Profile phải cho phép.
//
// Nguồn chân lý: docs/khronos/gl.xml (Khronos OpenGL-Registry).
//   • Enable cap chỉ có ở Compatibility Profile = enum group "EnableCap"
//     KHÔNG có trong core 4.6.
//   • Pname chỉ có ở Compatibility Profile = enum group "GetPName" (+ nhóm
//     bổ sung) KHÔNG có trong core 4.6.
// Bảng sinh tự động: tools/gen_compat_tables.py → src/gl/gl_compat_tables_gen.cpp.
//
// Spec: GL 4.6 Core §17.1 (Enable/Disable) — "An INVALID_ENUM error is
// generated if cap is not one of the listed values"; GL 4.6 Core §22.1
// (state queries) — pname không hợp lệ ⇒ INVALID_ENUM; GL 4.6 Core
// App. E.2.2 "Removed Features" liệt kê các state fixed-function bị gỡ.
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null — không có GPU).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/VersionGate.h"
#include "test_util.h"
#include <cstdio>

using namespace tglmt;

namespace {
void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* MakeCtx(TGLMTProfile p) {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = p;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

// Một số cap fixed-function tiêu biểu (giá trị từ registry).
constexpr GLenum kLighting = GL_LIGHTING;
constexpr GLenum kFog = GL_FOG;
constexpr GLenum kAlphaTest = GL_ALPHA_TEST;
constexpr GLenum kColorMaterial = GL_COLOR_MATERIAL;
constexpr GLenum kLight0 = GL_LIGHT0;
constexpr GLenum kLight7 = GL_LIGHT7;
constexpr GLenum kNormalize = GL_NORMALIZE;
constexpr GLenum kPolygonStipple = GL_POLYGON_STIPPLE;
constexpr GLenum kTexGenS = GL_TEXTURE_GEN_S;
constexpr GLenum kTexGenQ = GL_TEXTURE_GEN_Q;
constexpr GLenum kPointSmooth = GL_POINT_SMOOTH;
constexpr GLenum kLineStipple = GL_LINE_STIPPLE;

// Pname fixed-function tiêu biểu.
constexpr GLenum pLighting = GL_LIGHTING;
constexpr GLenum pShadeModel = GL_SHADE_MODEL;
constexpr GLenum pFogMode = GL_FOG_MODE;
constexpr GLenum pFogDensity = GL_FOG_DENSITY;
constexpr GLenum pLightModelAmbient = GL_LIGHT_MODEL_AMBIENT;
constexpr GLenum pColorMaterialFace = GL_COLOR_MATERIAL_FACE;
constexpr GLenum pModelviewMatrix = GL_MODELVIEW_MATRIX;
constexpr GLenum pCurrentColor = GL_CURRENT_COLOR;
constexpr GLenum pMaxLights = GL_MAX_LIGHTS;
}  // namespace

// ---------------------------------------------------------------------------
// 1. Bảng sinh tự động phải tự nhất quán với registry.
// ---------------------------------------------------------------------------
static void TestGeneratedTables() {
    TGLMT_CHECK(CompatOnlyEnableCapCount() >= 40,
                "bảng compat-only enable cap có ≥ 40 entry");
    TGLMT_CHECK(CompatOnlyGetPNameCount() >= 150,
                "bảng compat-only GetPName có ≥ 150 entry");

    // Các cap cốt lõi của fixed-function phải có trong bảng.
    for (GLenum cap : {kLighting, kFog, kAlphaTest, kColorMaterial, kLight0, kLight7,
                       kNormalize, kPolygonStipple, kTexGenS, kTexGenQ, kPointSmooth,
                       kLineStipple}) {
        TGLMT_CHECK(IsCompatOnlyEnableCap((unsigned)cap),
                    "cap fixed-function phải nằm trong bảng compat-only");
    }
    // ALIAS: 0x3000 = GL_CLIP_PLANE0 (compat) + GL_CLIP_DISTANCE0 (core) ⇒
    // giá trị đó hợp lệ ở Core qua tên core ⇒ KHÔNG được reject.
    TGLMT_CHECK(!IsCompatOnlyEnableCap(0x3000u),
                "0x3000 (GL_CLIP_PLANE0/GL_CLIP_DISTANCE0 alias) không được reject ở Core");
    TGLMT_CHECK(!IsCompatOnlyEnableCap(0x3001u), "0x3001 alias cũng không reject");
    // Cap CORE không được nằm trong bảng.
    for (GLenum cap : {0x0B71 /*GL_DEPTH_TEST*/, 0x0BE2 /*GL_BLEND*/,
                       0x0B44 /*GL_CULL_FACE*/, 0x8B4F /*GL_SCISSOR_TEST*/}) {
        TGLMT_CHECK(!IsCompatOnlyEnableCap((unsigned)cap),
                    "cap core không được nằm trong bảng compat-only");
    }
    // Pname.
    for (GLenum p : {pLighting, pShadeModel, pFogMode, pFogDensity, pLightModelAmbient,
                     pColorMaterialFace, pModelviewMatrix, pCurrentColor, pMaxLights}) {
        TGLMT_CHECK(IsCompatOnlyGetPName((unsigned)p),
                    "pname fixed-function phải nằm trong bảng compat-only");
    }
    // Pname CORE không được nằm trong bảng.
    for (GLenum p : {0x0BA2 /*GL_VIEWPORT*/, 0x0B73 /*GL_DEPTH_CLEAR_VALUE*/,
                     0x8B4F /*GL_SCISSOR_TEST*/}) {
        TGLMT_CHECK(!IsCompatOnlyGetPName((unsigned)p),
                    "pname core không được nằm trong bảng compat-only");
    }
    // GL_TEXTURE_ENV_MODE / GL_TEXTURE_GEN_MODE KHÔNG phải pname của glGet
    // (truy vấn bằng glGetTexEnvfv / glGetTexGenfv) ⇒ không được reject.
    TGLMT_CHECK(!IsCompatOnlyGetPName((unsigned)GL_TEXTURE_ENV_MODE),
                "GL_TEXTURE_ENV_MODE không phải pname của glGet");
    TGLMT_CHECK(!IsCompatOnlyGetPName((unsigned)GL_TEXTURE_GEN_MODE),
                "GL_TEXTURE_GEN_MODE không phải pname của glGet");
}

// ---------------------------------------------------------------------------
// 2. Core Profile: enable cap + pname phải bị từ chối.
// ---------------------------------------------------------------------------
static void TestCoreRejects() {
    TGLMTContext* ctx = MakeCtx(TGLMT_PROFILE_CORE);
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    for (GLenum cap : {kLighting, kFog, kAlphaTest, kColorMaterial, kLight0, kLight7,
                       kNormalize, kPolygonStipple, kTexGenS, kTexGenQ, kPointSmooth,
                       kLineStipple}) {
        DrainErrors();
        glEnable(cap);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "Core Profile: glEnable(cap fixed-function) phải INVALID_ENUM");
        TGLMT_CHECK(glIsEnabled(cap) == GL_FALSE,
                    "Core Profile: cap bị từ chối ⇒ KHÔNG được bật (state không nói dối)");
    }
    // glDisable cũng phải từ chối.
    DrainErrors();
    glDisable(kLighting);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glDisable(cap fixed-function) phải INVALID_ENUM");

    // Pname.
    GLint iv = 0;
    GLfloat fv = 0.0f;
    for (GLenum p : {pLighting, pShadeModel, pFogMode, pColorMaterialFace,
                     pModelviewMatrix, pCurrentColor}) {
        DrainErrors();
        glGetIntegerv(p, &iv);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "Core Profile: glGetIntegerv(pname fixed-function) phải INVALID_ENUM");
        DrainErrors();
        glGetFloatv(p, &fv);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "Core Profile: glGetFloatv(pname fixed-function) phải INVALID_ENUM");
    }
    DrainErrors();
    glGetFloatv(pFogDensity, &fv);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glGetFloatv(GL_FOG_DENSITY) phải INVALID_ENUM");

    // Cap CORE vẫn dùng được bình thường.
    DrainErrors();
    glEnable(0x0B71 /*GL_DEPTH_TEST*/);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "Core Profile: cap core vẫn hợp lệ");
    glDisable(0x0B71);

    // 0x3000 = GL_CLIP_DISTANCE0 ở Core ⇒ phải hợp lệ (alias với
    // GL_CLIP_PLANE0 của compat).
    DrainErrors();
    glEnable(0x3000 /*GL_CLIP_DISTANCE0*/);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Core Profile: glEnable(0x3000 = GL_CLIP_DISTANCE0) hợp lệ");
    glDisable(0x3000);

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. Compatibility Profile: phải cho phép tất cả.
// ---------------------------------------------------------------------------
static void TestCompatAccepts() {
    TGLMTContext* ctx = MakeCtx(TGLMT_PROFILE_COMPATIBILITY);
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    for (GLenum cap : {kLighting, kFog, kAlphaTest, kColorMaterial, kLight0,
                       kNormalize, kTexGenS, kPointSmooth, kLineStipple}) {
        DrainErrors();
        glEnable(cap);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                    "Compatibility Profile: glEnable(cap fixed-function) hợp lệ");
        TGLMT_CHECK(glIsEnabled(cap) == GL_TRUE,
                    "Compatibility Profile: cap phải bật được và phản ánh vào state");
        glDisable(cap);
    }

    GLint iv = 0;
    GLfloat fv = 0.0f;
    for (GLenum p : {pLighting, pShadeModel, pFogMode, pColorMaterialFace,
                     pModelviewMatrix, pCurrentColor}) {
        DrainErrors();
        glGetIntegerv(p, &iv);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                    "Compatibility Profile: glGetIntegerv(pname fixed-function) hợp lệ");
    }
    DrainErrors();
    glGetFloatv(pFogDensity, &fv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Compatibility Profile: glGetFloatv(GL_FOG_DENSITY) hợp lệ");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Context Legacy (GL < 3.2) cũng phải cho phép (fixed-function là chuẩn
//    của GL 1.x, không có khái niệm profile).
// ---------------------------------------------------------------------------
static void TestLegacyProfileAccepts() {
    TGLMTContextAttribs a{};
    a.majorVersion = 1;
    a.minorVersion = 5;
    a.profile = TGLMT_PROFILE_LEGACY;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context GL 1.5 Legacy");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glEnable(kLighting);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Legacy 1.5: glEnable(GL_LIGHTING) hợp lệ");
    TGLMT_CHECK(glIsEnabled(kLighting) == GL_TRUE, "Legacy 1.5: GL_LIGHTING bật được");
    GLint iv = 0;
    glGetIntegerv(pLighting, &iv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Legacy 1.5: glGetIntegerv(GL_LIGHTING) hợp lệ");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

int main() {
    TestGeneratedTables();
    TestCoreRejects();
    TestCompatAccepts();
    TestLegacyProfileAccepts();

    if (tglmt_test::Failures() == 0) {
        printf("test_compat_profile_gates: OK\n");
        return 0;
    }
    return 1;
}
