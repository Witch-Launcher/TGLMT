// test_legacy_texenv.cpp — OpenGL fixed-function TEXTURE ENVIRONMENT &
// GENERATION (Compatibility Profile).
//
// Spec: OpenGL 1.3 §3.8.12 "Texture Environments and Texture Functions"
//       (glspec13.pdf PDF p.146–151) + §2.11 "Generating Texture Coordinates"
//       (PDF p.38–39); OpenGL 4.2 Compatibility Profile §3.10.17 (p.347–352).
//
// Kiểm tra:
//  1. Mặc định texture environment (GL 1.3 §3.8.12 p.147): MODULATE, env
//     colour (0,0,0,0), combiner RGB/ALPHA = MODULATE, SRC0/1/2 =
//     TEXTURE/PREVIOUS/CONSTANT, OPERAND0/1_RGB = SRC_COLOR, OPERAND2_RGB =
//     SRC_ALPHA, OPERAND*_ALPHA = SRC_ALPHA.
//  2. Mặc định TexGen (GL 1.3 §2.11 p.39): EYE_LINEAR cho mọi coord, p của
//     S = (1,0,0,0), của T = (0,1,0,0), của R/Q = (0,0,0,0).
//  3. Round-trip glTexEnv*/glTexGen*.
//  4. Texture env LÀ PER-TEXTURE-UNIT (mỗi unit có state riêng).
//  5. Lỗi: target/pname/mode sai ⇒ INVALID_ENUM; R/Q với SPHERE_MAP ⇒
//     INVALID_ENUM (spec §2.11 p.38).
//  6. Core Profile ⇒ entry point bị gate (INVALID_OPERATION).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {
constexpr GLenum kTextureEnv = GL_TEXTURE_ENV;
constexpr GLenum kTextureEnvMode = GL_TEXTURE_ENV_MODE;
constexpr GLenum kTextureEnvColor = GL_TEXTURE_ENV_COLOR;
constexpr GLenum kCombineRGB = GL_COMBINE_RGB;
constexpr GLenum kCombineAlpha = GL_COMBINE_ALPHA;
constexpr GLenum kRgbScale = GL_RGB_SCALE;
constexpr GLenum kAlphaScale = GL_ALPHA_SCALE;
constexpr GLenum kSrc0RGB = GL_SRC0_RGB;
constexpr GLenum kSrc1RGB = GL_SRC1_RGB;
constexpr GLenum kSrc2RGB = GL_SRC2_RGB;
constexpr GLenum kSrc0Alpha = GL_SRC0_ALPHA;
constexpr GLenum kSrc1Alpha = GL_SRC1_ALPHA;
constexpr GLenum kOperand0RGB = GL_OPERAND0_RGB;
constexpr GLenum kOperand1RGB = GL_OPERAND1_RGB;
constexpr GLenum kOperand2RGB = GL_OPERAND2_RGB;
constexpr GLenum kOperand0Alpha = GL_OPERAND0_ALPHA;
constexpr GLenum kTextureGenMode = GL_TEXTURE_GEN_MODE;
constexpr GLenum kObjectPlane = GL_OBJECT_PLANE;
constexpr GLenum kEyePlane = GL_EYE_PLANE;
constexpr GLenum kS = GL_S, kT = GL_T, kR = GL_R, kQ = GL_Q;

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
// 1+2. Mặc định
// ---------------------------------------------------------------------------
static void TestDefaults() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Texture environment của unit 0 (spec GL 1.3 §3.8.12 p.147).
    const LegacyTexEnv& e = c.legacy.texEnv[0];
    TGLMT_CHECK(e.mode == GL_MODULATE, "TEXTURE_ENV_MODE mặc định = GL_MODULATE");
    TGLMT_CHECK(Near(e.color[0], 0.0f) && Near(e.color[3], 0.0f),
                "TEXTURE_ENV_COLOR mặc định = (0,0,0,0)");
    TGLMT_CHECK(e.combineRGB == GL_MODULATE, "COMBINE_RGB mặc định = GL_MODULATE");
    TGLMT_CHECK(e.combineAlpha == GL_MODULATE, "COMBINE_ALPHA mặc định = GL_MODULATE");
    // Nguồn (SOURCE) mặc định: SRC0 = TEXTURE, SRC1 = PREVIOUS,
    // SRC2 = CONSTANT — cho cả RGB và ALPHA (spec GL 1.3 p.147).
    TGLMT_CHECK(e.src0RGB == GL_TEXTURE && e.src1RGB == GL_PREVIOUS &&
                    e.src2RGB == GL_CONSTANT,
                "SRCn_RGB mặc định = TEXTURE/PREVIOUS/CONSTANT");
    TGLMT_CHECK(e.src0Alpha == GL_TEXTURE && e.src1Alpha == GL_PREVIOUS &&
                    e.src2Alpha == GL_CONSTANT,
                "SRCn_ALPHA mặc định = TEXTURE/PREVIOUS/CONSTANT");
    // Operand mặc định: RGB operand 0/1 = SRC_COLOR, RGB operand 2 = SRC_ALPHA,
    // mọi operand ALPHA = SRC_ALPHA.
    TGLMT_CHECK(e.operand0RGB == GL_SRC_COLOR && e.operand1RGB == GL_SRC_COLOR &&
                    e.operand2RGB == GL_SRC_ALPHA,
                "OPERANDn_RGB mặc định = SRC_COLOR/SRC_COLOR/SRC_ALPHA");
    TGLMT_CHECK(e.operand0Alpha == GL_SRC_ALPHA && e.operand1Alpha == GL_SRC_ALPHA &&
                    e.operand2Alpha == GL_SRC_ALPHA,
                "OPERANDn_ALPHA mặc định = SRC_ALPHA");
    TGLMT_CHECK(e.rgbScale == 1 && e.alphaScale == 1, "RGB_SCALE/ALPHA_SCALE mặc định = 1");

    // TexGen mặc định (spec GL 1.3 §2.11 p.39): EYE_LINEAR, p(S)=(1,0,0,0),
    // p(T)=(0,1,0,0), p(R)=p(Q)=(0,0,0,0).
    const LegacyTexGen& tg = c.legacy.texGen[0];
    for (int i = 0; i < 4; ++i)
        TGLMT_CHECK(tg.coord[i].mode == GL_EYE_LINEAR,
                    "TexGen mặc định = EYE_LINEAR cho mọi coord");
    TGLMT_CHECK(Near(tg.coord[0].plane[0], 1.0f) && Near(tg.coord[0].plane[1], 0.0f),
                "TexGen S plane mặc định = (1,0,0,0)");
    TGLMT_CHECK(Near(tg.coord[1].plane[1], 1.0f) && Near(tg.coord[1].plane[0], 0.0f),
                "TexGen T plane mặc định = (0,1,0,0)");
    TGLMT_CHECK(Near(tg.coord[2].plane[0], 0.0f) && Near(tg.coord[3].plane[0], 0.0f),
                "TexGen R/Q plane mặc định = (0,0,0,0)");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3+4. Round-trip + per-unit state
// ---------------------------------------------------------------------------
static void TestRoundTripAndPerUnit() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glTexEnvi(kTextureEnv, kTextureEnvMode, GL_REPLACE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvi(TEXTURE_ENV_MODE, REPLACE)");
    TGLMT_CHECK(c.legacy.texEnv[0].mode == GL_REPLACE, "unit 0 mode = REPLACE");

    // Unit 1 có state RIÊNG (spec GL 1.3 §3.8.12: "the current texture
    // environment" = environment của texture unit ĐANG HOẠT ĐỘNG).
    glActiveTexture(GL_TEXTURE1);
    glTexEnvi(kTextureEnv, kTextureEnvMode, GL_DECAL);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvi trên unit 1");
    TGLMT_CHECK(c.legacy.texEnv[1].mode == GL_DECAL, "unit 1 mode = DECAL");
    TGLMT_CHECK(c.legacy.texEnv[0].mode == GL_REPLACE,
                "unit 0 KHÔNG bị đổi khi đổi unit 1 (state per-unit)");

    // Quay lại unit 0.
    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(kTextureEnv, kTextureEnvMode, GL_MODULATE);
    TGLMT_CHECK(c.legacy.texEnv[0].mode == GL_MODULATE, "unit 0 mode cập nhật lại được");

    // Env colour clamp vào [0,1] (spec GL 1.3 §3.8.12 p.147).
    const GLfloat col[4] = {-0.5f, 0.5f, 1.5f, 2.0f};
    glTexEnvfv(kTextureEnv, kTextureEnvColor, col);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvfv(TEXTURE_ENV_COLOR)");
    TGLMT_CHECK(Near(c.legacy.texEnv[0].color[0], 0.0f) &&
                    Near(c.legacy.texEnv[0].color[1], 0.5f) &&
                    Near(c.legacy.texEnv[0].color[2], 1.0f) &&
                    Near(c.legacy.texEnv[0].color[3], 1.0f),
                "TEXTURE_ENV_COLOR clamp vào [0,1]");

    // Combiner arguments.
    glTexEnvi(kTextureEnv, kCombineRGB, GL_ADD);
    glTexEnvi(kTextureEnv, kCombineAlpha, GL_INTERPOLATE);
    glTexEnvi(kTextureEnv, kSrc0RGB, GL_PREVIOUS);
    glTexEnvi(kTextureEnv, kSrc1Alpha, GL_CONSTANT);
    glTexEnvi(kTextureEnv, kOperand0RGB, GL_ONE_MINUS_SRC_COLOR);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvi combiner args hợp lệ");
    TGLMT_CHECK(c.legacy.texEnv[0].combineRGB == GL_ADD, "COMBINE_RGB = ADD");
    TGLMT_CHECK(c.legacy.texEnv[0].combineAlpha == GL_INTERPOLATE, "COMBINE_ALPHA = INTERPOLATE");
    TGLMT_CHECK(c.legacy.texEnv[0].src0RGB == GL_PREVIOUS, "SRC0_RGB = PREVIOUS");
    TGLMT_CHECK(c.legacy.texEnv[0].src1Alpha == GL_CONSTANT, "SRC1_ALPHA = CONSTANT");
    TGLMT_CHECK(c.legacy.texEnv[0].operand0RGB == GL_ONE_MINUS_SRC_COLOR,
                "OPERAND0_RGB = ONE_MINUS_SRC_COLOR");

    // RGB_SCALE / ALPHA_SCALE: 1, 2 hoặc 4.
    glTexEnvf(kTextureEnv, kRgbScale, 4.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvf(RGB_SCALE, 4)");
    TGLMT_CHECK(c.legacy.texEnv[0].rgbScale == 4, "RGB_SCALE = 4");
    glTexEnvf(kTextureEnv, kRgbScale, 3.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "RGB_SCALE = 3 ⇒ INVALID_ENUM");

    // TexGen round-trip.
    glTexGeni(kS, kTextureGenMode, GL_OBJECT_LINEAR);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexGeni(S, TEXTURE_GEN_MODE, OBJECT_LINEAR)");
    TGLMT_CHECK(c.legacy.texGen[0].coord[0].mode == GL_OBJECT_LINEAR, "TexGen S mode = OBJECT_LINEAR");
    glTexGeni(kT, kTextureGenMode, GL_SPHERE_MAP);
    TGLMT_CHECK(c.legacy.texGen[0].coord[1].mode == GL_SPHERE_MAP, "TexGen T mode = SPHERE_MAP");

    // Plane: bản "fv" đặt cả 4 thành phần.
    const GLfloat plane[4] = {0.0f, 1.0f, 0.0f, 2.0f};
    glTexGenfv(kS, kObjectPlane, plane);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexGenfv(S, OBJECT_PLANE)");
    TGLMT_CHECK(Near(c.legacy.texGen[0].coord[0].plane[1], 1.0f) &&
                    Near(c.legacy.texGen[0].coord[0].plane[3], 2.0f),
                "TexGen S plane = (0,1,0,2)");

    // TexGen cũng per-unit.
    glActiveTexture(GL_TEXTURE1);
    glTexGeni(kS, kTextureGenMode, GL_SPHERE_MAP);
    TGLMT_CHECK(c.legacy.texGen[1].coord[0].mode == GL_SPHERE_MAP,
                "TexGen cũng per-texture-unit");
    TGLMT_CHECK(c.legacy.texGen[0].coord[0].mode == GL_OBJECT_LINEAR,
                "TexGen unit 0 không đổi khi đổi unit 1");
    glActiveTexture(GL_TEXTURE0);

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. Lỗi
// ---------------------------------------------------------------------------
static void TestErrors() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glTexEnvi(0x9999 /*target sai*/, kTextureEnvMode, GL_MODULATE);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexEnv target sai → INVALID_ENUM");
    glTexEnvi(kTextureEnv, 0x9999 /*pname sai*/, GL_MODULATE);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexEnv pname sai → INVALID_ENUM");
    glTexEnvi(kTextureEnv, kTextureEnvMode, 0x9999 /*mode sai*/);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexEnv mode sai → INVALID_ENUM");
    glTexEnvi(kTextureEnv, kCombineRGB, GL_DOT3_RGBA);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "COMBINE_RGB = DOT3_RGBA hợp lệ");
    glTexEnvi(kTextureEnv, kCombineAlpha, GL_DOT3_RGB);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "COMBINE_ALPHA = DOT3_RGB ⇒ INVALID_ENUM (chỉ RGB có DOT3)");
    glTexEnvi(kTextureEnv, kSrc0RGB, GL_SRC_COLOR);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "SRC0_RGB = SRC_COLOR ⇒ INVALID_ENUM (nguồn không phải operand)");

    // TexGen: coord sai, pname sai, mode sai.
    glTexGeni(0x9999, kTextureGenMode, GL_OBJECT_LINEAR);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexGen coord sai → INVALID_ENUM");
    glTexGeni(kS, 0x9999, GL_OBJECT_LINEAR);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexGen pname sai → INVALID_ENUM");
    glTexGeni(kS, kTextureGenMode, 0x9999);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glTexGen mode sai → INVALID_ENUM");

    // SPHERE_MAP / REFLECTION_MAP / NORMAL_MAP không hợp lệ với R, Q
    // (spec GL 1.3 §2.11 p.38–39: "Calling TexGen with a coord of either R or
    // Q when pname indicates SPHERE MAP generates the error INVALID_ENUM").
    glTexGeni(kR, kTextureGenMode, GL_SPHERE_MAP);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "TexGen(R, SPHERE_MAP) → INVALID_ENUM");
    glTexGeni(kQ, kTextureGenMode, GL_SPHERE_MAP);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "TexGen(Q, SPHERE_MAP) → INVALID_ENUM");
    glTexGeni(kQ, kTextureGenMode, GL_REFLECTION_MAP);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "TexGen(Q, REFLECTION_MAP) → INVALID_ENUM");
    glTexGeni(kQ, kTextureGenMode, GL_NORMAL_MAP);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "TexGen(Q, NORMAL_MAP) → INVALID_ENUM");
    // Nhưng S/T thì hợp lệ.
    glTexGeni(kS, kTextureGenMode, GL_SPHERE_MAP);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "TexGen(S, SPHERE_MAP) hợp lệ");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6. Core Profile: gate
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

    glTexEnvi(kTextureEnv, kTextureEnvMode, GL_REPLACE);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glTexEnvi phải INVALID_OPERATION");
    glTexGeni(kS, kTextureGenMode, GL_OBJECT_LINEAR);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glTexGeni phải INVALID_OPERATION");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

int main() {
    TestDefaults();
    TestRoundTripAndPerUnit();
    TestErrors();
    TestCoreProfile();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_texenv: OK\n");
        return 0;
    }
    return 1;
}
