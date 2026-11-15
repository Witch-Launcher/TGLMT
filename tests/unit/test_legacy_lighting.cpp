// test_legacy_lighting.cpp — OpenGL fixed-function LIGHTING & MATERIALS.
//
// Spec: OpenGL 2.1 §2.14 (PDF p.59–70) + OpenGL 4.2 Compatibility Profile
// §2.13 + bảng state 6.16/6.17/6.18 (PDF p.519–521). Đã đối chiếu trực tiếp
// với glspec21.pdf và glspec42.compatibility.
//
// Kiểm tra:
//  1. Mặc định đúng spec (material, light, light model).
//  2. glLight/glMaterial/glLightModel/glColorMaterial/glShadeModel round-trip.
//  3. glLight POSITION/SPOT_DIRECTION transform bằng modelview TẠI LÚC GỌI.
//  4. Ràng buộc giá trị (spot exponent/cutoff, attenuation, shininess) →
//     GL_INVALID_VALUE; enum sai → GL_INVALID_ENUM.
//  5. glGetLightfv/glGetMaterialiv trả đúng (2× cho front+back).
//  6. Core Profile ⇒ mọi entry point này bị version-gate (INVALID_OPERATION).
//  7. Compatibility Profile ⇒ chạy thật, không lỗi.
// D-003: version-gate nằm ở LỚP C ABI (gl_c_api.h) — test phải gọi qua
// đó để kiểm tra gate, KHÔNG gọi tglmt::gl:: (lớp C++ nội bộ, không gate).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace {
constexpr GLenum kLight0 = GL_LIGHT0;
constexpr GLenum kAmbient = GL_AMBIENT;
constexpr GLenum kDiffuse = GL_DIFFUSE;
constexpr GLenum kSpecular = GL_SPECULAR;
constexpr GLenum kPosition = GL_POSITION;
constexpr GLenum kSpotDirection = GL_SPOT_DIRECTION;
constexpr GLenum kSpotExponent = GL_SPOT_EXPONENT;
constexpr GLenum kSpotCutoff = GL_SPOT_CUTOFF;
constexpr GLenum kEmission = GL_EMISSION;
constexpr GLenum kShininess = GL_SHININESS;
constexpr GLenum kAmbientAndDiffuse = GL_AMBIENT_AND_DIFFUSE;
constexpr GLenum kFront = GL_FRONT;
constexpr GLenum kBack = GL_BACK;
constexpr GLenum kFrontAndBack = GL_FRONT_AND_BACK;
constexpr GLenum kLightModelAmbient = GL_LIGHT_MODEL_AMBIENT;
constexpr GLenum kLightModelLocalViewer = GL_LIGHT_MODEL_LOCAL_VIEWER;
constexpr GLenum kLightModelTwoSide = GL_LIGHT_MODEL_TWO_SIDE;
constexpr GLenum kLightModelColorControl = GL_LIGHT_MODEL_COLOR_CONTROL;
constexpr GLenum kSingleColor = GL_SINGLE_COLOR;
constexpr GLenum kSeparateSpecular = GL_SEPARATE_SPECULAR_COLOR;
constexpr GLenum kSmooth = GL_SMOOTH;
constexpr GLenum kFlat = GL_FLAT;
// Registry values (docs/khronos/gl.xml): GL_CONSTANT_ATTENUATION = 0x1207,
// GL_LINEAR_ATTENUATION = 0x1208, GL_QUADRATIC_ATTENUATION = 0x1209.
constexpr GLenum kConstAttenuation = GL_CONSTANT_ATTENUATION;
constexpr GLenum kLinearAttenuation = GL_LINEAR_ATTENUATION;
constexpr GLenum kQuadAttenuation = GL_QUADRATIC_ATTENUATION;

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

// Context Compatibility 4.6 — các entry point fixed-function chỉ tồn tại ở
// profile này. Context mặc định của TGLMT là 4.6 CORE ⇒ dùng sai sẽ bị gate.
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
// Phần 1: mặc định theo spec (spec Table 2.10 / glspec21 PDF p.61)
// ---------------------------------------------------------------------------
static void TestDefaults() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestDefaults");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // Material front: acm=(0.2,0.2,0.2,1) dcm=(0.8,0.8,0.8,1)
    //                 scm=(0,0,0,1) ecm=(0,0,0,1) srm=0
    GLfloat m[8] = {};
    glGetMaterialfv(kFront, kAmbient, m);
    TGLMT_CHECK(Near(m[0], 0.2f) && Near(m[1], 0.2f) && Near(m[2], 0.2f) && Near(m[3], 1.0f),
                "material AMBIENT mặc định = (0.2,0.2,0.2,1) [spec Table 2.10]");
    glGetMaterialfv(kFront, kDiffuse, m);
    TGLMT_CHECK(Near(m[0], 0.8f) && Near(m[1], 0.8f) && Near(m[2], 0.8f) && Near(m[3], 1.0f),
                "material DIFFUSE mặc định = (0.8,0.8,0.8,1) [spec Table 2.10]");
    glGetMaterialfv(kFront, kSpecular, m);
    TGLMT_CHECK(Near(m[0], 0.0f) && Near(m[1], 0.0f) && Near(m[2], 0.0f) && Near(m[3], 1.0f),
                "material SPECULAR mặc định = (0,0,0,1)");
    glGetMaterialfv(kFront, kEmission, m);
    TGLMT_CHECK(Near(m[0], 0.0f) && Near(m[3], 1.0f), "material EMISSION mặc định = (0,0,0,1)");
    GLfloat shin = -1.0f;
    glGetMaterialfv(kFront, kShininess, &shin);
    TGLMT_CHECK(Near(shin, 0.0f), "material SHININESS mặc định = 0");

    // Light 0: dcli = scli = (1,1,1,1);  Light i>0: (0,0,0,1)
    GLfloat l[4] = {};
    glGetLightfv(kLight0, kDiffuse, l);
    TGLMT_CHECK(Near(l[0], 1.0f) && Near(l[3], 1.0f),
                "LIGHT0 DIFFUSE mặc định = (1,1,1,1) [spec: dcli(i=0) khác i>0]");
    glGetLightfv(kLight0 + 1, kDiffuse, l);
    TGLMT_CHECK(Near(l[0], 0.0f) && Near(l[3], 1.0f),
                "LIGHT1 DIFFUSE mặc định = (0,0,0,1)");
    glGetLightfv(kLight0, kSpecular, l);
    TGLMT_CHECK(Near(l[0], 1.0f), "LIGHT0 SPECULAR mặc định = (1,1,1,1)");
    glGetLightfv(kLight0, kAmbient, l);
    TGLMT_CHECK(Near(l[0], 0.0f) && Near(l[3], 1.0f), "LIGHT0 AMBIENT mặc định = (0,0,0,1)");

    // Light position mặc định (0,0,1,0) — w=0 ⇒ nguồn sáng ở vô cùng
    GLfloat p[4] = {};
    glGetLightfv(kLight0, kPosition, p);
    TGLMT_CHECK(Near(p[0], 0.0f) && Near(p[1], 0.0f) && Near(p[2], 1.0f) && Near(p[3], 0.0f),
                "LIGHT POSITION mặc định = (0,0,1,0) [spec Table 2.10]");
    GLfloat sd[3] = {};
    glGetLightfv(kLight0, kSpotDirection, sd);
    TGLMT_CHECK(Near(sd[0], 0.0f) && Near(sd[1], 0.0f) && Near(sd[2], -1.0f),
                "LIGHT SPOT_DIRECTION mặc định = (0,0,-1)");
    GLfloat se = -1.0f, sc = -1.0f;
    glGetLightfv(kLight0, kSpotExponent, &se);
    glGetLightfv(kLight0, kSpotCutoff, &sc);
    TGLMT_CHECK(Near(se, 0.0f), "LIGHT SPOT_EXPONENT mặc định = 0");
    TGLMT_CHECK(Near(sc, 180.0f), "LIGHT SPOT_CUTOFF mặc định = 180");
    GLfloat k0 = -1, k1 = -1, k2 = -1;
    glGetLightfv(kLight0, kConstAttenuation, &k0);
    glGetLightfv(kLight0, kLinearAttenuation, &k1);
    glGetLightfv(kLight0, kQuadAttenuation, &k2);
    TGLMT_CHECK(Near(k0, 1.0f) && Near(k1, 0.0f) && Near(k2, 0.0f),
                "attenuation mặc định = (1,0,0)");

    // Light model: acs=(0.2,0.2,0.2,1), vbs=FALSE, tbs=FALSE, ces=SINGLE_COLOR
    GLfloat amb[4] = {};
    glGetFloatv(kLightModelAmbient, amb);
    TGLMT_CHECK(Near(amb[0], 0.2f) && Near(amb[3], 1.0f),
                "LIGHT_MODEL_AMBIENT mặc định = (0.2,0.2,0.2,1)");
    GLint lv = -1, ts = -1, cc = -1;
    glGetIntegerv(kLightModelLocalViewer, &lv);
    glGetIntegerv(kLightModelTwoSide, &ts);
    glGetIntegerv(kLightModelColorControl, &cc);
    TGLMT_CHECK(lv == 0, "LIGHT_MODEL_LOCAL_VIEWER mặc định = FALSE [spec Table 2.10]");
    TGLMT_CHECK(ts == 0, "LIGHT_MODEL_TWO_SIDE mặc định = FALSE");
    TGLMT_CHECK(cc == (GLint)kSingleColor, "LIGHT_MODEL_COLOR_CONTROL mặc định = SINGLE_COLOR");

    // Shade model mặc định SMOOTH; ColorMaterial face/mode mặc định
    GLint sm = -1, cmf = -1, cmp = -1;
    glGetIntegerv(0x0B54 /*GL_SHADE_MODEL*/, &sm);
    glGetIntegerv(0x0B55 /*GL_COLOR_MATERIAL_FACE*/, &cmf);
    glGetIntegerv(0x0B56 /*GL_COLOR_MATERIAL_PARAMETER*/, &cmp);
    TGLMT_CHECK(sm == (GLint)kSmooth, "SHADE_MODEL mặc định = SMOOTH");
    TGLMT_CHECK(cmf == (GLint)kFrontAndBack,
                "COLOR_MATERIAL_FACE mặc định = FRONT_AND_BACK [spec §2.14.4]");
    TGLMT_CHECK(cmp == (GLint)kAmbientAndDiffuse,
                "COLOR_MATERIAL_PARAMETER mặc định = AMBIENT_AND_DIFFUSE [spec §2.14.4]");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "truy vấn state không sinh lỗi");
    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// Phần 2: round-trip setter/getter
// ---------------------------------------------------------------------------
static void TestRoundTrip() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestRoundTrip");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    const GLfloat amb[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    glMaterialfv(kFront, kAmbient, amb);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMaterialfv(FRONT, AMBIENT)");
    GLfloat got[8] = {};
    glGetMaterialfv(kFront, kAmbient, got);
    TGLMT_CHECK(Near(got[0], 0.1f) && Near(got[3], 0.4f), "material ambient round-trip");

    // GL_FRONT_AND_BACK ghi cả 2 mặt
    const GLfloat dif[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    glMaterialfv(kFrontAndBack, kDiffuse, dif);
    glGetMaterialfv(kFront, kDiffuse, got);
    GLfloat gotBack[8] = {};
    glGetMaterialfv(kBack, kDiffuse, gotBack);
    TGLMT_CHECK(Near(got[0], 0.5f), "FRONT_AND_BACK diffuse áp cho front");
    TGLMT_CHECK(Near(gotBack[0], 0.5f), "FRONT_AND_BACK diffuse áp cho back");

    // glGetMaterialfv(FRONT) trả CHỈ giá trị của mặt front (spec 4.2C §6.1.3:
    // "face must be either FRONT or BACK ... into data"). Không được ghi
    // 2×count giá trị vào buffer của caller (ghi vượt ⇒ memory corruption).
    GLfloat frontOnly[4] = {};
    glGetMaterialfv(kFront, kAmbient, frontOnly);
    TGLMT_CHECK(Near(frontOnly[0], 0.1f),
                "glGetMaterialfv(FRONT) trả giá trị của mặt front");
    // FRONT_AND_BACK (tương thích Mesa) trả front rồi back liên tiếp.
    // Đặt cả 2 mặt để phân biệt được front và back.
    const GLfloat amb2[4] = {0.31f, 0.32f, 0.33f, 0.34f};
    glMaterialfv(kFrontAndBack, kAmbient, amb2);
    GLfloat both[8] = {};
    glGetMaterialfv(kFrontAndBack, kAmbient, both);
    TGLMT_CHECK(Near(both[0], 0.31f) && Near(both[4], 0.31f),
                "glGetMaterialfv(FRONT_AND_BACK) trả front+back liên tiếp");
    // Khôi phục ambient front để các đoạn sau không đổi kỳ vọng.
    glMaterialfv(kFront, kAmbient, amb);

    // AMBIENT_AND_DIFFUSE set cả acm và dcm
    const GLfloat ad[4] = {0.9f, 0.8f, 0.7f, 0.6f};
    glMaterialfv(kFront, kAmbientAndDiffuse, ad);
    glGetMaterialfv(kFront, kAmbient, got);
    TGLMT_CHECK(Near(got[0], 0.9f), "AMBIENT_AND_DIFFUSE set acm");
    glGetMaterialfv(kFront, kDiffuse, got);
    TGLMT_CHECK(Near(got[0], 0.9f), "AMBIENT_AND_DIFFUSE set dcm");

    // shininess qua bản "i" (chuyển kiểu int → float theo Table 2.9)
    // SHININESS là tham số REAL (không phải màu) ⇒ chuyển kiểu TRỰC TIẾP
    // (spec §2.14.2: Table 2.9 chỉ áp cho color parameters).
    glMateriali(kFront, kShininess, 64);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMateriali(SHININESS, 64)");
    GLfloat sh = -1;
    glGetMaterialfv(kFront, kShininess, &sh);
    TGLMT_CHECK(Near(sh, 64.0f), "shininess sau glMateriali = 64 (chuyển trực tiếp)");

    // glLight round-trip
    const GLfloat lp[4] = {1.0f, 2.0f, 3.0f, 1.0f};
    glLightfv(kLight0, kPosition, lp);
    GLfloat lpOut[4] = {};
    glGetLightfv(kLight0, kPosition, lpOut);
    TGLMT_CHECK(Near(lpOut[0], 1.0f) && Near(lpOut[1], 2.0f) && Near(lpOut[2], 3.0f) &&
                    Near(lpOut[3], 1.0f),
                "light position round-trip (modelview identity)");

    // glLightModel
    glLightModeli(kLightModelLocalViewer, GL_TRUE);
    GLint lv = 0;
    glGetIntegerv(kLightModelLocalViewer, &lv);
    TGLMT_CHECK(lv == 1, "LIGHT_MODEL_LOCAL_VIEWER bật được");
    glLightModeli(kLightModelTwoSide, GL_TRUE);
    GLint ts = 0;
    glGetIntegerv(kLightModelTwoSide, &ts);
    TGLMT_CHECK(ts == 1, "LIGHT_MODEL_TWO_SIDE bật được");
    glLightModeli(kLightModelColorControl, kSeparateSpecular);
    GLint cc = 0;
    glGetIntegerv(kLightModelColorControl, &cc);
    TGLMT_CHECK(cc == (GLint)kSeparateSpecular, "COLOR_CONTROL = SEPARATE_SPECULAR");
    const GLfloat scenAmb[4] = {0.25f, 0.25f, 0.25f, 1.0f};
    glLightModelfv(kLightModelAmbient, scenAmb);
    GLfloat ga[4] = {};
    glGetFloatv(kLightModelAmbient, ga);
    TGLMT_CHECK(Near(ga[0], 0.25f), "LIGHT_MODEL_AMBIENT round-trip");

    // glColorMaterial
    glColorMaterial(kBack, kSpecular);
    GLint cmf = 0, cmp = 0;
    glGetIntegerv(0x0B55, &cmf);
    glGetIntegerv(0x0B56, &cmp);
    TGLMT_CHECK(cmf == (GLint)kBack && cmp == (GLint)kSpecular, "glColorMaterial round-trip");

    // glShadeModel
    glShadeModel(kFlat);
    GLint sm = 0;
    glGetIntegerv(0x0B54, &sm);
    TGLMT_CHECK(sm == (GLint)kFlat, "glShadeModel(FLAT) round-trip");
    glShadeModel(kSmooth);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "các lệnh hợp lệ không sinh lỗi");
    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// Phần 3: transform vị trí/spot direction bằng modelview TẠI LÚC GỌI
// (spec GL 2.1 §2.14.2, glspec21 PDF p.66)
// ---------------------------------------------------------------------------
static void TestLightPositionTransform() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestLightPositionTransform");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // Dịch modelview +10 trên X rồi đặt vị trí light ⇒ vị trí lưu = 11.
    glMatrixMode(0x1700 /*GL_MODELVIEW*/);
    glLoadIdentity();
    glTranslatef(10.0f, 0.0f, 0.0f);
    // w = 1 ⇒ nguồn sáng CÓ VỊ TRÍ (positional) ⇒ phần tịch của modelview
    // áp vào. (w = 0 ⇒ directional: điểm ở vô cùng, phần tịch không áp.)
    const GLfloat lp[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(kLight0, kPosition, lp);
    GLfloat out[4] = {};
    glGetLightfv(kLight0, kPosition, out);
    TGLMT_CHECK(Near(out[0], 11.0f), "glLight POSITION transform bằng modelview tại lúc gọi");
    TGLMT_CHECK(Near(out[3], 1.0f), "w của positional light giữ nguyên 1");

    // Đổi modelview SAU ⇒ light KHÔNG di chuyển (spec: transform tại lúc gọi).
    glLoadIdentity();
    glTranslatef(100.0f, 0.0f, 0.0f);
    glGetLightfv(kLight0, kPosition, out);
    TGLMT_CHECK(Near(out[0], 11.0f),
                "đổi modelview sau glLight KHÔNG di chuyển light đã đặt");

    // Spot direction: transform bằng 3x3 trên-trái (bỏ phần tịch).
    glMatrixMode(0x1700);
    glLoadIdentity();
    glTranslatef(5.0f, 7.0f, 9.0f);
    const GLfloat sd[3] = {0.0f, 0.0f, -1.0f};
    glLightfv(kLight0, kSpotDirection, sd);
    GLfloat sout[3] = {};
    glGetLightfv(kLight0, kSpotDirection, sout);
    TGLMT_CHECK(Near(sout[0], 0.0f) && Near(sout[1], 0.0f) && Near(sout[2], -1.0f),
                "SPOT_DIRECTION không bị dịch bởi phần tịch của modelview");
    glLoadIdentity();
    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// Phần 4: ràng buộc giá trị & lỗi
// ---------------------------------------------------------------------------
static void TestErrors() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestErrors");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // Enum sai → GL_INVALID_ENUM
    glLightf(0x9999 /*không phải GL_LIGHTi*/, kDiffuse, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glLight với light enum sai → INVALID_ENUM");
    glLightf(kLight0, 0x9999 /*pname sai*/, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glLight với pname sai → INVALID_ENUM");
    // pname nhiều giá trị dùng bản scalar → INVALID_ENUM (spec §2.14.2:
    // "If param corresponds to a multi-valued parameter, the error INVALID_ENUM
    //  results.")
    glLightf(kLight0, kDiffuse, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glLight scalar với pname 4 giá trị (DIFFUSE) → INVALID_ENUM");
    // pname 1 giá trị dùng bản scalar → hợp lệ.
    glLightf(kLight0, kSpotExponent, 8.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glLight scalar với pname 1 giá trị (SPOT_EXPONENT) → OK");
    glMaterialf(kFront, kAmbient, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glMaterial scalar với pname 4 giá trị → INVALID_ENUM");
    glMaterialf(0x9999 /*face sai*/, kAmbient, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glMaterial với face sai → INVALID_ENUM");
    glLightModelf(kLightModelAmbient, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glLightModel scalar với LIGHT_MODEL_AMBIENT → INVALID_ENUM");
    glLightModeli(kLightModelColorControl, 0x9999);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glLightModel COLOR_CONTROL sai → INVALID_ENUM");
    glColorMaterial(kFront, 0x9999);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glColorMaterial mode sai → INVALID_ENUM");
    glColorMaterial(0x9999, kAmbient);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glColorMaterial face sai → INVALID_ENUM");
    glShadeModel(0x9999);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glShadeModel mode sai → INVALID_ENUM");

    // Ràng buộc giá trị → GL_INVALID_VALUE (spec Table 2.10)
    glLightf(kLight0, kSpotExponent, 129.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "spot exponent > 128 → INVALID_VALUE");
    glLightf(kLight0, kSpotExponent, -1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "spot exponent < 0 → INVALID_VALUE");
    glLightf(kLight0, kSpotCutoff, 91.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "spot cutoff 91 → INVALID_VALUE");
    glLightf(kLight0, kSpotCutoff, 90.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "spot cutoff 90 hợp lệ");
    glLightf(kLight0, kSpotCutoff, 180.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "spot cutoff 180 hợp lệ");
    glLightf(kLight0, kConstAttenuation, -1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "attenuation âm → INVALID_VALUE");
    glMaterialf(kFront, kShininess, 129.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "shininess > 128 → INVALID_VALUE");
    glMaterialf(kFront, kShininess, -0.5f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "shininess < 0 → INVALID_VALUE");

    // Một giá trị sai trong mảng ⇒ lệnh bị từ chối TOÀN BỘ (state không đổi).
    // Đặt mốc rõ ràng trước để so sánh không bị ảnh hưởng bởi lệnh trước đó.
    const GLfloat goodExp[1] = {16.0f};
    glLightfv(kLight0, kSpotExponent, goodExp);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glLightfv spot exponent hợp lệ");
    const GLfloat badExp[1] = {200.0f};
    glLightfv(kLight0, kSpotExponent, badExp);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glLightfv spot exponent sai → INVALID_VALUE");
    GLfloat se = -1.0f;
    glGetLightfv(kLight0, kSpotExponent, &se);
    TGLMT_CHECK(Near(se, 16.0f),
                "giá trị sai KHÔNG được ghi (lệnh bị từ chối toàn bộ)");
    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// Phần 5: Core Profile phải gate các entry point này
// ---------------------------------------------------------------------------
static void TestCoreProfileGating() {
    TGLMTContextAttribs a = {};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    TGLMTContext* c = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(c != nullptr, "tạo được context Core 4.6");
    if (!c) return;
    tglmtMakeCurrent(c);
    DrainErrors();

    // Các entry point này KHÔNG tồn tại ở Core Profile ⇒ INVALID_OPERATION.
    glLightf(kLight0, kDiffuse, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glLight phải INVALID_OPERATION");
    glMaterialf(kFront, kAmbient, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glMaterial phải INVALID_OPERATION");
    glLightModelf(kLightModelLocalViewer, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glLightModel phải INVALID_OPERATION");
    glColorMaterial(kFront, kAmbient);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glColorMaterial phải INVALID_OPERATION");
    glShadeModel(kFlat);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glShadeModel phải INVALID_OPERATION");
    glGetLightfv(kLight0, kDiffuse, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glGetLightfv phải INVALID_OPERATION");
    glGetMaterialfv(kFront, kAmbient, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glGetMaterialfv phải INVALID_OPERATION");

    // Pname fixed-function ở Core Profile ⇒ INVALID_ENUM.
    GLint v = 0;
    glGetIntegerv(0x0B50 /*GL_LIGHTING*/, &v);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glGetIntegerv(GL_LIGHTING) phải INVALID_ENUM");
    glGetIntegerv(0x0B54 /*GL_SHADE_MODEL*/, &v);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "Core Profile: glGetIntegerv(GL_SHADE_MODEL) phải INVALID_ENUM");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(c);
}

// ---------------------------------------------------------------------------
// Phần 6: Compatibility Profile — các lệnh phải chạy thật
// ---------------------------------------------------------------------------
static void TestCompatibilityProfile() {
    TGLMTContextAttribs a = {};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    TGLMTContext* c = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(c != nullptr, "tạo được context Compatibility 4.6");
    if (!c) return;
    tglmtMakeCurrent(c);
    DrainErrors();

    const GLfloat amb[4] = {0.11f, 0.22f, 0.33f, 0.44f};
    glMaterialfv(kFront, kAmbient, amb);
    glLightfv(kLight0, kDiffuse, amb);
    glLightModelfv(kLightModelAmbient, amb);
    glColorMaterial(kFrontAndBack, kAmbientAndDiffuse);
    glShadeModel(kFlat);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Compatibility Profile: các lệnh lighting chạy không lỗi");

    GLfloat got[4] = {};
    glGetMaterialfv(kFront, kAmbient, got);
    TGLMT_CHECK(Near(got[0], 0.11f), "Compatibility Profile: đọc lại được material");
    glGetLightfv(kLight0, kDiffuse, got);
    TGLMT_CHECK(Near(got[0], 0.11f), "Compatibility Profile: đọc lại được light");

    // Pname fixed-function ở Compatibility Profile ⇒ hợp lệ.
    GLint v = -1;
    glGetIntegerv(0x0B50 /*GL_LIGHTING*/, &v);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Compatibility Profile: glGetIntegerv(GL_LIGHTING) hợp lệ");
    TGLMT_CHECK(v == 0, "GL_LIGHTING mặc định tắt");

    // glEnable(GL_LIGHTING) ở Compatibility Profile phải được chấp nhận.
    glEnable(0x0B50 /*GL_LIGHTING*/);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Compatibility Profile: glEnable(GL_LIGHTING) hợp lệ");
    glGetIntegerv(0x0B50, &v);
    TGLMT_CHECK(v == 1, "glEnable(GL_LIGHTING) phản ánh vào state");
    glDisable(0x0B50);
    glGetIntegerv(0x0B50, &v);
    TGLMT_CHECK(v == 0, "glDisable(GL_LIGHTING) phản ánh vào state");

    // glEnable(GL_LIGHT0) cũng phải được chấp nhận.
    glEnable(kLight0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "Compatibility Profile: glEnable(GL_LIGHT0) hợp lệ");
    TGLMT_CHECK(glIsEnabled(kLight0) == GL_TRUE, "glIsEnabled(GL_LIGHT0) = TRUE");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(c);
}

int main() {
    TestDefaults();
    TestRoundTrip();
    TestLightPositionTransform();
    TestErrors();
    TestCoreProfileGating();
    TestCompatibilityProfile();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_lighting: OK\n");
        return 0;
    }
    return 1;
}
