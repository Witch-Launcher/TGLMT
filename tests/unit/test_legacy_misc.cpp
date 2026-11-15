// test_legacy_misc.cpp — RESIDENCY / PRIORITY / FEEDBACK MARKER (misc) +
//                        TRUY VẤN TEXTURE ENVIRONMENT & TEXTURE GENERATION.
//
// Spec: OpenGL 2.1 §3.8.12 "Texturing" (glspec21.pdf PDF p.183–184):
//       AreTexturesResident ("...or if the implementation does not distinguish
//       a working set"; "If any of the names in textures are unused or are
//       zero, FALSE is returned, the error INVALID_VALUE is generated"),
//       PrioritizeTextures ("Each priority value is clamped to the range
//       [0,1] before it is assigned").
//       §5.3 "Feedback" (PDF p.240): "PassThrough may not occur between Begin
//       and End. It has no effect when the GL is not in feedback mode."
//       Table 6.19 (PDF p.284) + §3.8.13 (p.184–187): các pname truy vấn
//       được của texture environment / texture generation.
//
// Kiểm tra:
//  1. glAreTexturesResident(0, NULL, NULL) ⇒ GL_TRUE; n < 0 ⇒
//     GL_INVALID_VALUE; tên chưa dùng / tên 0 ⇒ FALSE + GL_INVALID_VALUE.
//  2. glPassThrough ngoài feedback mode ⇒ không lỗi; trong glBegin/glEnd ⇒
//     GL_INVALID_OPERATION.
//  3. glPrioritizeTextures: n < 0 ⇒ GL_INVALID_VALUE; priority ngoài [0,1] ⇒
//     GL_INVALID_VALUE; priority hợp lệ ⇒ lưu vào texture.
//  4. glGetTexEnvfv/iv: round-trip với glTexEnv*, target/pname sai ⇒
//     GL_INVALID_ENUM.
//  5. glGetTexGenfv/iv/dv: round-trip với glTexGen*, coord/pname sai ⇒
//     GL_INVALID_ENUM.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {

constexpr GLenum kTextureEnv = GL_TEXTURE_ENV;
constexpr GLenum kTextureEnvMode = GL_TEXTURE_ENV_MODE;
constexpr GLenum kTextureEnvColor = GL_TEXTURE_ENV_COLOR;
constexpr GLenum kRgbScale = GL_RGB_SCALE;
constexpr GLenum kReplace = GL_REPLACE;
constexpr GLenum kTextureGenMode = GL_TEXTURE_GEN_MODE;
constexpr GLenum kEyePlane = GL_EYE_PLANE;
constexpr GLenum kObjectPlane = GL_OBJECT_PLANE;
constexpr GLenum kObjectLinear = GL_OBJECT_LINEAR;
constexpr GLenum kS = GL_S, kT = GL_T, kR = GL_R, kQ = GL_Q;
constexpr GLenum kTexture2D = GL_TEXTURE_2D;
constexpr GLenum kBadEnum = 0x8123;  // không phải enum hợp lệ nào
constexpr GLenum kPolygon = GL_POLYGON;

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
// 1. AreTexturesResident
// ---------------------------------------------------------------------------
static void TestResidency() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // n = 0 ⇒ "all of the 0 texture objects" ⇒ TRUE.
    TGLMT_CHECK(glAreTexturesResident(0, nullptr, nullptr) == GL_TRUE,
                "glAreTexturesResident(0, NULL, NULL) ⇒ GL_TRUE");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glAreTexturesResident(0,...) ⇒ không lỗi");

    // n < 0 ⇒ INVALID_VALUE.
    DrainErrors();
    glAreTexturesResident(-1, nullptr, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "n < 0 ⇒ GL_INVALID_VALUE");

    // Texture thật ⇒ TRUE (TGLMT không phân biệt working set).
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    DrainErrors();
    TGLMT_CHECK(glAreTexturesResident(1, &tex, nullptr) == GL_TRUE,
                "texture đã tạo ⇒ GL_TRUE");

    // residences có thể NULL ⇒ không lỗi.
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "residences = NULL ⇒ không lỗi");

    // Tên chưa dùng ⇒ FALSE + INVALID_VALUE.
    GLuint unused = 0x7FFFFFFFu;
    DrainErrors();
    TGLMT_CHECK(glAreTexturesResident(1, &unused, nullptr) == GL_FALSE,
                "tên chưa dùng ⇒ GL_FALSE");
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "tên chưa dùng ⇒ GL_INVALID_VALUE");

    // Tên 0 (default texture) ⇒ FALSE + INVALID_VALUE.
    GLuint zero = 0;
    DrainErrors();
    TGLMT_CHECK(glAreTexturesResident(1, &zero, nullptr) == GL_FALSE,
                "tên 0 ⇒ GL_FALSE");
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "tên 0 ⇒ GL_INVALID_VALUE");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. PassThrough
// ---------------------------------------------------------------------------
static void TestPassThrough() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Ngoài feedback mode ⇒ không có tác dụng, KHÔNG lỗi.
    glPassThrough(1.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glPassThrough ngoài feedback mode ⇒ không lỗi");
    TGLMT_CHECK(c.legacy.selection.feedbackUsed == 0,
                "glPassThrough ngoài feedback mode ⇒ không ghi vào feedback buffer");

    // Trong glBegin/glEnd ⇒ INVALID_OPERATION ("PassThrough may not occur
    // between Begin and End", GL 2.1 §5.3 p.240).
    DrainErrors();
    glBegin(kPolygon);
    glPassThrough(1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPassThrough trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glEnd();
    DrainErrors();

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. PrioritizeTextures
// ---------------------------------------------------------------------------
static void TestPrioritize() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);

    // n < 0 ⇒ INVALID_VALUE.
    DrainErrors();
    glPrioritizeTextures(-1, &tex, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "n < 0 ⇒ GL_INVALID_VALUE");

    // Priority hợp lệ ⇒ không lỗi, giá trị được lưu (đã clamp theo spec).
    const GLfloat ok[1] = {0.5f};
    DrainErrors();
    glPrioritizeTextures(1, &tex, ok);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "priority hợp lệ ⇒ không lỗi");
    auto tit = c.textures.find(tex);
    TGLMT_CHECK(tit != c.textures.end(), "texture tồn tại trong context");
    if (tit != c.textures.end()) {
        auto pit = tit->second.fparams.find(GL_TEXTURE_PRIORITY);
        TGLMT_CHECK(pit != tit->second.fparams.end() && Near(pit->second, 0.5f),
                    "priority 0.5 được lưu vào texture");
    }

    // Priority NGOÀI [0,1] ⇒ GL_INVALID_VALUE (task slice này; spec GL 2.1
    // §3.8.12 p.184 chỉ yêu cầu clamp — xem báo cáo phiên).
    const GLfloat hi[1] = {2.0f};
    DrainErrors();
    glPrioritizeTextures(1, &tex, hi);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "priority > 1 ⇒ GL_INVALID_VALUE");
    const GLfloat lo[1] = {-1.0f};
    DrainErrors();
    glPrioritizeTextures(1, &tex, lo);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "priority < 0 ⇒ GL_INVALID_VALUE");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. glGetTexEnvfv / glGetTexEnviv
// ---------------------------------------------------------------------------
static void TestTexEnvQueries() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Round-trip với glTexEnv*.
    glTexEnvi(kTextureEnv, kTextureEnvMode, kReplace);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvi ⇒ không lỗi");
    GLint mode = 0;
    glGetTexEnviv(kTextureEnv, kTextureEnvMode, &mode);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetTexEnviv ⇒ không lỗi");
    TGLMT_CHECK(mode == (GLint)kReplace, "TEXTURE_ENV_MODE round-trip = GL_REPLACE");

    const GLfloat col[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    glTexEnvfv(kTextureEnv, kTextureEnvColor, col);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvfv(TEXTURE_ENV_COLOR) ⇒ không lỗi");
    GLfloat gotCol[4] = {0, 0, 0, 0};
    glGetTexEnvfv(kTextureEnv, kTextureEnvColor, gotCol);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetTexEnvfv ⇒ không lỗi");
    TGLMT_CHECK(Near(gotCol[0], 0.1f) && Near(gotCol[1], 0.2f) &&
                    Near(gotCol[2], 0.3f) && Near(gotCol[3], 0.4f),
                "TEXTURE_ENV_COLOR round-trip");

    glTexEnvf(kTextureEnv, kRgbScale, 2.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexEnvf(RGB_SCALE, 2) ⇒ không lỗi");
    GLfloat rgbScale = 0.0f;
    glGetTexEnvfv(kTextureEnv, kRgbScale, &rgbScale);
    TGLMT_CHECK(Near(rgbScale, 2.0f), "RGB_SCALE round-trip = 2.0");

    // Target sai ⇒ INVALID_ENUM; pname sai ⇒ INVALID_ENUM.
    DrainErrors();
    glGetTexEnviv(kBadEnum, kTextureEnvMode, &mode);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "target sai ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glGetTexEnviv(kTextureEnv, kBadEnum, &mode);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "pname sai ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glGetTexEnvfv(kTextureEnv, kBadEnum, gotCol);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetTexEnvfv pname sai ⇒ GL_INVALID_ENUM");

    // Truy vấn áp cho texture unit ĐANG HOẠT ĐỘNG.
    glActiveTexture(GL_TEXTURE1);
    DrainErrors();
    glGetTexEnviv(kTextureEnv, kTextureEnvMode, &mode);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "truy vấn trên unit 1 ⇒ không lỗi");
    TGLMT_CHECK(mode == (GLint)GL_MODULATE,
                "unit 1 có texture env riêng (mặc định MODULATE)");
    glActiveTexture(GL_TEXTURE0);

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. glGetTexGenfv / glGetTexGeniv / glGetTexGendv
// ---------------------------------------------------------------------------
static void TestTexGenQueries() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glTexGeni(kS, kTextureGenMode, kObjectLinear);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexGeni ⇒ không lỗi");
    GLint mode = 0;
    glGetTexGeniv(kS, kTextureGenMode, &mode);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetTexGeniv ⇒ không lỗi");
    TGLMT_CHECK(mode == (GLint)kObjectLinear, "TEXTURE_GEN_MODE round-trip = OBJECT_LINEAR");

    const GLfloat plane[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    glTexGenfv(kT, kEyePlane, plane);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexGenfv(EYE_PLANE) ⇒ không lỗi");
    GLfloat gotPlane[4] = {0, 0, 0, 0};
    glGetTexGenfv(kT, kEyePlane, gotPlane);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetTexGenfv ⇒ không lỗi");
    TGLMT_CHECK(Near(gotPlane[0], 1.0f) && Near(gotPlane[1], 2.0f) &&
                    Near(gotPlane[2], 3.0f) && Near(gotPlane[3], 4.0f),
                "EYE_PLANE round-trip");
    GLdouble gotD[4] = {0, 0, 0, 0};
    glGetTexGendv(kT, kEyePlane, gotD);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetTexGendv ⇒ không lỗi");
    TGLMT_CHECK(Near((float)gotD[0], 1.0f) && Near((float)gotD[3], 4.0f),
                "glGetTexGendv trả về cùng giá trị");

    // coord sai ⇒ INVALID_ENUM; pname sai ⇒ INVALID_ENUM.
    DrainErrors();
    glGetTexGeniv(kBadEnum, kTextureGenMode, &mode);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "coord sai ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glGetTexGeniv(kS, kBadEnum, &mode);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "pname sai ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glGetTexGenfv(kT, kBadEnum, gotPlane);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetTexGenfv pname sai ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glGetTexGendv(kT, kBadEnum, gotD);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetTexGendv pname sai ⇒ GL_INVALID_ENUM");

    tglmtDestroyContext(ctx);
}

int main() {
    TestResidency();
    TestPassThrough();
    TestPrioritize();
    TestTexEnvQueries();
    TestTexGenQueries();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_misc: OK\n");
        return 0;
    }
    return 1;
}
