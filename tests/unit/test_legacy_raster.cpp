// test_legacy_raster.cpp — CURRENT RASTER POSITION, WINDOW POSITION, CLIP
// PLANES, POLYGON/LINE STIPPLE, POINT PARAMETERS (Compatibility Profile).
//
// Spec (đã đối chiếu trực tiếp với PDF):
//   • GL 2.1 §2.12 Clipping (glspec21.pdf PDF p.52–54), §2.13 Current Raster
//     Position (PDF p.54–56), §3.4.2 Line Stipple (PDF p.104–105), §3.5.2
//     Stippling (PDF p.110), §6.1.3 GetClipPlane (PDF p.249), §6.1.5
//     GetPolygonStipple (PDF p.253).
//   • GL 4.2 Compatibility §2.23/§2.25 (PDF p.214–217), §3.5.2 (p.235),
//     §3.6.2 (p.241), §6.1.3 (p.477), §6.1.6 (p.485).
//   • GL 1.4 §3.3 Points / PointParameters (glspec14.pdf PDF p.79–80).
//   • GL 4.6 Compatibility §3.5.2: factor của LineStipple CLAMP về [1,256]
//     (KHÔNG sinh lỗi) — khác với man page cũ.
//
// Kiểm tra:
//   1. RasterPos: transform modelview→projection→viewport, valid flag, w = wc,
//      biến thể int/short chuyển thẳng (KHÔNG Table 2.9).
//   2. WindowPos: đặt trực tiếp window coords, zw theo DepthRange, KHÔNG clip.
//   3. ClipPlane: transform p·M^-1 sang EYE SPACE + round-trip glGetClipPlane.
//   4. PolygonStipple: 32×32 bit (128 byte) đóng gói MSB-first, round-trip.
//   5. LineStipple: pattern + repeat (factor clamp theo spec).
//   6. PointParameters: MIN/MAX/FADE/DISTANCE_ATTENUATION + lỗi enum/value.
//   7. Cấm trong glBegin/glEnd (GL 4.2C §2.6.3).
//   8. Core Profile ⇒ version-gate ⇒ INVALID_OPERATION.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt;

// Hàm dùng chung cho glLineStipple / glPointParameter* (src/gl/gl_raster.cpp
// đã có tên entry point nhưng no-op / chỉ pname Core — file đó ngoài
// write-scope nên logic đặt ở src/gl/gl_legacy_raster.cpp).
namespace tglmt {
namespace gl {
void LegacySetLineStipple(GLint factor, GLushort pattern);
void LegacySetPointParameterf(GLenum pname, GLfloat param);
void LegacySetPointParameterfv(GLenum pname, const GLfloat* params);
void LegacySetPointParameteri(GLenum pname, GLint param);
void LegacySetPointParameteriv(GLenum pname, const GLint* params);
}  // namespace gl
}  // namespace tglmt

namespace {

bool Near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

TGLMTContext* CoreCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

// Viewport cố định cho test: (0,0,64,64) ⇒ ox = oy = 32, px = py = 64.
constexpr int kVpW = 64;
constexpr int kVpH = 64;

// ---------------------------------------------------------------------------
// 1. RASTER POSITION
// ---------------------------------------------------------------------------
// Spec GL 2.1 §2.13 (PDF p.54–55):
//   - RasterPos3: x,y,z với w = 1; RasterPos2: x,y với z = 0, w = 1.
//   - "transformed by the current model-view and projection matrices"
//   - viewport transform (GL 2.1 §2.11.1 PDF p.43):
//       xw = (px/2)·xd + ox,  yw = (py/2)·yd + oy,
//       zw = ((f−n)/2)·zd + (n+f)/2,  ox = x + w/2, oy = y + h/2.
//   - "If the 'point' is culled, ... the valid bit is cleared."
static void TestRasterPosition() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestRasterPosition");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();
    glViewport(0, 0, kVpW, kVpH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    DrainErrors();

    const LegacyState& L = Context::Current().legacy;

    // --- glRasterPos3f(0.5, -0.25, 0.5) ---
    glRasterPos3f(0.5f, -0.25f, 0.5f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos3f ⇒ không lỗi");
    TGLMT_CHECK(L.raster.valid, "raster pos trong clip volume ⇒ valid = TRUE");
    // xw = 32·0.5 + 32 = 48 ; yw = 32·(−0.25) + 32 = 24 ; zw = 0.5·0.5 + 0.5 = 0.75
    TGLMT_CHECK(Near(L.raster.x, 48.0f), "xw = 48 (viewport transform §2.11.1)");
    TGLMT_CHECK(Near(L.raster.y, 24.0f), "yw = 24");
    TGLMT_CHECK(Near(L.raster.z, 0.75f), "zw = 0.75 (depth range 0..1)");
    TGLMT_CHECK(Near(L.raster.w, 1.0f), "wc = 1 (clip w, không project)");
    TGLMT_CHECK(Near(L.raster.depth, 0.75f), "associated depth = zw");
    // Raster distance = khoảng cách tới gốc eye (sau modelview):
    // sqrt(0.5² + 0.25² + 0.5²) = 0.75
    TGLMT_CHECK(Near(L.raster.fogCoord, 0.75f), "raster distance = |eye| = 0.75");

    // --- glRasterPos2f: z = 0, w = 1 (spec: "z implicitly set to 0") ---
    glRasterPos2f(0.5f, 0.25f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2f ⇒ không lỗi");
    TGLMT_CHECK(L.raster.valid, "glRasterPos2f ⇒ valid");
    TGLMT_CHECK(Near(L.raster.x, 48.0f) && Near(L.raster.y, 40.0f),
                "glRasterPos2f(0.5,0.25) ⇒ xw=48, yw=40");
    TGLMT_CHECK(Near(L.raster.z, 0.5f),
                "zw = 0.5 (object z = 0 ⇒ zd = 0 ⇒ zw = (n+f)/2)");
    TGLMT_CHECK(Near(L.raster.w, 1.0f), "glRasterPos2 ⇒ w = 1");

    // --- glRasterPos4f: w tường minh ---
    glRasterPos4f(0.5f, 0.25f, 0.5f, 2.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos4f ⇒ không lỗi");
    TGLMT_CHECK(L.raster.valid, "glRasterPos4f(w=2) ⇒ valid");
    // xd = 0.5/2 = 0.25 ; yd = 0.125 ; zd = 0.25
    TGLMT_CHECK(Near(L.raster.x, 40.0f) && Near(L.raster.y, 36.0f),
                "glRasterPos4f(w=2) ⇒ xw=40, yw=36");
    TGLMT_CHECK(Near(L.raster.z, 0.625f), "glRasterPos4f(w=2) ⇒ zw=0.625");
    TGLMT_CHECK(Near(L.raster.w, 2.0f), "wc = w = 2");

    // --- biến thể double ---
    glRasterPos2d(0.5, 0.25);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2d ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 48.0f) && Near(L.raster.y, 40.0f),
                "glRasterPos2d ⇒ xw=48, yw=40");

    // --- biến thể vector ---
    const GLfloat v2[2] = {0.5f, 0.25f};
    glRasterPos2fv(v2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2fv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 48.0f), "glRasterPos2fv ⇒ xw=48");
    const GLdouble v3[3] = {0.5, -0.25, 0.5};
    glRasterPos3dv(v3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos3dv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 48.0f) && Near(L.raster.y, 24.0f),
                "glRasterPos3dv ⇒ xw=48, yw=24");

    // --- biến thể int/short: chuyển THẲNG sang float (KHÔNG Table 2.9) ---
    // Spec GL 2.1 Table 2.9 (PDF p.59) chỉ áp cho "Color, normal, and depth
    // components"; §2.13 nói "The coordinates are treated as if they were
    // specified in a Vertex command" ⇒ int 1 ⇒ 1.0 (không phải 7e-10).
    glRasterPos2i(1, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2i ⇒ không lỗi");
    TGLMT_CHECK(L.raster.valid, "glRasterPos2i(1,0) ⇒ trong clip volume ⇒ valid");
    TGLMT_CHECK(Near(L.raster.x, 64.0f),
                "glRasterPos2i(1,0) ⇒ x=1.0 ⇒ xw=64 (KHÔNG Table 2.9: 32)");
    TGLMT_CHECK(Near(L.raster.y, 32.0f), "glRasterPos2i(1,0) ⇒ yw=32");
    glRasterPos2s(1, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2s ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 64.0f),
                "glRasterPos2s(1,0) ⇒ x=1.0 ⇒ xw=64 (KHÔNG Table 2.9)");
    const GLint iv2[2] = {1, 0};
    glRasterPos2iv(iv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2iv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 64.0f), "glRasterPos2iv ⇒ xw=64");
    const GLshort sv2[2] = {1, 0};
    glRasterPos2sv(sv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRasterPos2sv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 64.0f), "glRasterPos2sv ⇒ xw=64");

    // --- vertex bị CULL (ngoài clip volume) ⇒ valid = FALSE ---
    glRasterPos3f(0.0f, 2.0f, 0.0f);  // yd = 2 > 1 ⇒ culled
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "điểm ngoài clip volume ⇒ không lỗi");
    TGLMT_CHECK(!L.raster.valid, "điểm ngoài clip volume ⇒ valid = FALSE");

    // --- con trỏ NULL ---
    DrainErrors();
    glRasterPos3fv(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glRasterPos3fv(NULL) ⇒ INVALID_VALUE");
    glRasterPos2dv(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glRasterPos2dv(NULL) ⇒ INVALID_VALUE");
}

// ---------------------------------------------------------------------------
// 2. WINDOW POSITION
// ---------------------------------------------------------------------------
// Spec GL 2.1 §2.13 (PDF p.55–56):
//   xw = x ; yw = y ; zw = n (z≤0) | f (z≥1) | n + z(f−n) ; wc = 1
//   "Lighting, texture coordinate generation and transformation, and clipping are
//    NOT performed by the WindowPos functions ... and the valid bit is set."
static void TestWindowPosition() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestWindowPosition");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();
    glViewport(0, 0, kVpW, kVpH);
    const LegacyState& L = Context::Current().legacy;

    glWindowPos3f(10.0f, 20.0f, 0.5f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3f ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 10.0f) && Near(L.raster.y, 20.0f),
                "glWindowPos3f ⇒ window coords trực tiếp (không transform)");
    TGLMT_CHECK(Near(L.raster.z, 0.5f), "zw = n + z(f−n) = 0.5");
    TGLMT_CHECK(Near(L.raster.w, 1.0f), "wc = 1");
    TGLMT_CHECK(L.raster.valid, "WindowPos không clip ⇒ valid luôn TRUE");

    glWindowPos2f(3.0f, 4.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2f ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 3.0f) && Near(L.raster.y, 4.0f),
                "glWindowPos2f ⇒ xw=3, yw=4");
    TGLMT_CHECK(Near(L.raster.z, 0.0f), "glWindowPos2f (z=0) ⇒ zw = n = 0");

    // WindowPos KHÔNG bị clip ⇒ điểm ngoài viewport vẫn valid.
    glWindowPos3f(-100.0f, -100.0f, 2.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3f ngoài viewport ⇒ không lỗi");
    TGLMT_CHECK(L.raster.valid, "WindowPos ngoài viewport vẫn valid (spec không clip)");
    TGLMT_CHECK(Near(L.raster.z, 1.0f), "zw = f = 1 khi z ≥ 1");

    // Biến thể còn lại.
    glWindowPos2d(1.0, 2.0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2d ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 1.0f) && Near(L.raster.y, 2.0f), "glWindowPos2d ⇒ xw=1,yw=2");
    glWindowPos2dv((const GLdouble[]){3.0, 4.0});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2dv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 3.0f), "glWindowPos2dv ⇒ xw=3");
    glWindowPos2i(5, 6);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2i ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 5.0f) && Near(L.raster.y, 6.0f),
                "glWindowPos2i ⇒ x=5 (chuyển thẳng), y=6");
    glWindowPos2s(7, 8);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2s ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 7.0f), "glWindowPos2s ⇒ x=7 (chuyển thẳng)");
    glWindowPos2iv((const GLint[]){9, 10});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2iv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 9.0f), "glWindowPos2iv ⇒ xw=9");
    glWindowPos2sv((const GLshort[]){11, 12});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2sv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 11.0f), "glWindowPos2sv ⇒ xw=11");
    glWindowPos2fv((const GLfloat[]){13.0f, 14.0f});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos2fv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 13.0f), "glWindowPos2fv ⇒ xw=13");
    glWindowPos3d(1.0, 2.0, 0.25);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3d ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.z, 0.25f), "glWindowPos3d ⇒ zw=0.25");
    glWindowPos3dv((const GLdouble[]){1.0, 2.0, 0.75});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3dv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.z, 0.75f), "glWindowPos3dv ⇒ zw=0.75");
    glWindowPos3i(1, 2, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3i ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 1.0f), "glWindowPos3i ⇒ x=1 (chuyển thẳng)");
    glWindowPos3iv((const GLint[]){3, 4, 0});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3iv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 3.0f), "glWindowPos3iv ⇒ xw=3");
    glWindowPos3s(5, 6, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3s ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 5.0f), "glWindowPos3s ⇒ x=5 (chuyển thẳng)");
    glWindowPos3sv((const GLshort[]){7, 8, 0});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3sv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 7.0f), "glWindowPos3sv ⇒ xw=7");
    glWindowPos3fv((const GLfloat[]){9.0f, 10.0f, 0.25f});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWindowPos3fv ⇒ không lỗi");
    TGLMT_CHECK(Near(L.raster.x, 9.0f), "glWindowPos3fv ⇒ xw=9");

    DrainErrors();
    glWindowPos3fv(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glWindowPos3fv(NULL) ⇒ INVALID_VALUE");
}

// ---------------------------------------------------------------------------
// 3. CLIP PLANE
// ---------------------------------------------------------------------------
// Spec GL 2.1 §2.12 (PDF p.52–53): p' = (p1 p2 p3 p4)·M^-1 (EYE SPACE).
// Spec GL 2.1 §6.1.3 (PDF p.249): GetClipPlane trả hệ số EYE SPACE.
static void TestClipPlane() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestClipPlane");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    DrainErrors();

    GLdouble eq[4] = {0.0, 0.0, 1.0, 0.0};
    glClipPlane(GL_CLIP_PLANE0, eq);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClipPlane ⇒ không lỗi");

    GLdouble out[4] = {9, 9, 9, 9};
    glGetClipPlane(GL_CLIP_PLANE0, out);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetClipPlane ⇒ không lỗi");
    TGLMT_CHECK(Near((float)out[0], 0.0f) && Near((float)out[1], 0.0f) &&
                    Near((float)out[2], 1.0f) && Near((float)out[3], 0.0f),
                "modelview identity ⇒ round-trip (0,0,1,0)");

    // Modelview có translate ⇒ phẳng z=0 trong OBJECT space nằm ở eye z=10
    // ⇒ hệ số eye-space = (0,0,1,−10).
    glTranslatef(0.0f, 0.0f, 10.0f);
    const GLdouble eq2[4] = {0.0, 0.0, 1.0, 0.0};
    glClipPlane(GL_CLIP_PLANE0, eq2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClipPlane (có translate) ⇒ không lỗi");
    GLdouble out2[4] = {9, 9, 9, 9};
    glGetClipPlane(GL_CLIP_PLANE0, out2);
    TGLMT_CHECK(Near((float)out2[0], 0.0f) && Near((float)out2[1], 0.0f) &&
                    Near((float)out2[2], 1.0f) && Near((float)out2[3], -10.0f),
                "p·M^-1 với M = translate(0,0,10) ⇒ (0,0,1,−10) [EYE SPACE]");

    // Plane khác (GL_CLIP_PLANE5) độc lập với plane 0.
    const GLdouble eq3[4] = {1.0, 0.0, 0.0, -5.0};
    glClipPlane(GL_CLIP_PLANE5, eq3);
    GLdouble out3[4] = {0, 0, 0, 0};
    glGetClipPlane(GL_CLIP_PLANE5, out3);
    TGLMT_CHECK(Near((float)out3[0], 1.0f) && Near((float)out3[3], -5.0f),
                "GL_CLIP_PLANE5 lưu/đọc riêng");

    // Enum sai ⇒ INVALID_ENUM.
    DrainErrors();
    glClipPlane(GL_CLIP_PLANE0 + 6, eq);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glClipPlane(plane ngoài 0..5) ⇒ INVALID_ENUM");
    glGetClipPlane(GL_CLIP_PLANE0 + 6, out);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetClipPlane(plane ngoài 0..5) ⇒ INVALID_ENUM");
    glClipPlane(GL_CLIP_PLANE0 + 6, eq);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "lỗi enum phải lặp lại mỗi lần (FIFO)");

    // Con trỏ NULL.
    DrainErrors();
    glClipPlane(GL_CLIP_PLANE0, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glClipPlane(NULL) ⇒ INVALID_VALUE");
    glGetClipPlane(GL_CLIP_PLANE0, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glGetClipPlane(NULL) ⇒ INVALID_VALUE");
}

// ---------------------------------------------------------------------------
// 4. POLYGON STIPPLE
// ---------------------------------------------------------------------------
// Spec GL 2.1 §3.5.2 (PDF p.110): pattern 32×32 BIT đóng gói như DrawPixels
// (width=height=32, type=BITMAP, format=COLOR_INDEX) ⇒ 128 byte; pixel đầu
// mỗi byte ở bit MSB (UNPACK_LSB_FIRST = FALSE).
// Spec GL 2.1 §6.1.5 (PDF p.253): GetPolygonStipple đóng gói lại theo
// ReadPixels ⇒ 128 byte.
static void TestPolygonStipple() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestPolygonStipple");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // Trạng thái đầu: pattern all ones (spec §3.5.5 "initial stipple pattern
    // is all ones").
    GLubyte init[128];
    std::memset(init, 0xAA, sizeof(init));
    glGetPolygonStipple(init);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetPolygonStipple ⇒ không lỗi");
    bool allOnes = true;
    for (int i = 0; i < 128; ++i) allOnes = allOnes && (init[i] == 0xFF);
    TGLMT_CHECK(allOnes, "pattern khởi tạo = all ones (128 byte 0xFF)");

    // Pattern có bit (x=3, y=5) ⇒ byte 5*4 + 0 = 20, bit 7−(3%8) = 4 ⇒ 0x10.
    GLubyte pat[128];
    std::memset(pat, 0, sizeof(pat));
    pat[20] = 0x10;
    glPolygonStipple(pat);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPolygonStipple ⇒ không lỗi");

    GLubyte out[128];
    std::memset(out, 0x55, sizeof(out));
    glGetPolygonStipple(out);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetPolygonStipple ⇒ không lỗi");
    TGLMT_CHECK(out[20] == 0x10,
                "bit (x=3,y=5) nằm ở byte 20 bit 4 (MSB-first) = 0x10");
    bool restZero = true;
    for (int i = 0; i < 128; ++i) {
        if (i == 20) continue;
        restZero = restZero && (out[i] == 0x00);
    }
    TGLMT_CHECK(restZero, "các byte còn lại của pattern = 0");

    // Round-trip byte-for-byte với pattern "đậm".
    GLubyte dense[128];
    for (int i = 0; i < 128; ++i) dense[i] = (GLubyte)((i * 37 + 11) & 0xFF);
    glPolygonStipple(dense);
    GLubyte denseOut[128];
    std::memset(denseOut, 0, sizeof(denseOut));
    glGetPolygonStipple(denseOut);
    TGLMT_CHECK(std::memcmp(dense, denseOut, 128) == 0,
                "glPolygonStipple → glGetPolygonStipple round-trip 128 byte");

    // Con trỏ NULL.
    DrainErrors();
    glPolygonStipple(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glPolygonStipple(NULL) ⇒ INVALID_VALUE");
    glGetPolygonStipple(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glGetPolygonStipple(NULL) ⇒ INVALID_VALUE");
}

// ---------------------------------------------------------------------------
// 5. LINE STIPPLE
// ---------------------------------------------------------------------------
// Spec GL 4.6C §3.5.2 (PDF p.235 tương đương GL 2.1 §3.4.2 PDF p.104):
//   "factor is clamped to the range [1, 256]" ⇒ factor ≤ 0 KHÔNG sinh lỗi.
//   "The bits of p are numbered with 0 being the least significant" ⇒ LSB trước.
static void TestLineStipple() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestLineStipple");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    tglmt::gl::LegacySetLineStipple(3, 0xAAAA);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glLineStipple(3, 0xAAAA) ⇒ không lỗi");
    TGLMT_CHECK(Context::Current().legacy.stipple.linePattern == 0xAAAA,
                "line pattern = 0xAAAA");
    TGLMT_CHECK(Context::Current().legacy.stipple.lineRepeat == 3,
                "line repeat = factor = 3");

    // factor ≤ 0 ⇒ CLAMP về 1 (spec), KHÔNG phải INVALID_VALUE.
    tglmt::gl::LegacySetLineStipple(0, 0xFF);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glLineStipple(0,·) ⇒ KHÔNG lỗi (spec: factor clamp [1,256])");
    TGLMT_CHECK(Context::Current().legacy.stipple.lineRepeat == 1,
                "factor 0 ⇒ repeat = 1 (clamp theo spec)");
    tglmt::gl::LegacySetLineStipple(1000, 0x0F0F);
    TGLMT_CHECK(Context::Current().legacy.stipple.lineRepeat == 256,
                "factor 1000 ⇒ repeat = 256 (clamp theo spec)");
    TGLMT_CHECK(Context::Current().legacy.stipple.linePattern == 0x0F0F,
                "line pattern = 0x0F0F");
}

// ---------------------------------------------------------------------------
// 6. POINT PARAMETERS
// ---------------------------------------------------------------------------
// Spec GL 1.4 §3.3 (PDF p.79–80).
static void TestPointParameters() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestPointParameters");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MAX, 2.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameterf(SIZE_MAX, 2.0) ⇒ không lỗi");
    TGLMT_CHECK(Near(Context::Current().legacy.pointParam.sizeMax, 2.0f),
                "pointParam.sizeMax = 2.0");

    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MIN, 0.5f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameterf(SIZE_MIN, 0.5) ⇒ không lỗi");
    TGLMT_CHECK(Near(Context::Current().legacy.pointParam.sizeMin, 0.5f),
                "pointParam.sizeMin = 0.5");

    tglmt::gl::LegacySetPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 4.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameterf(FADE_THRESHOLD, 4) ⇒ không lỗi");
    TGLMT_CHECK(Near(Context::Current().legacy.pointParam.fadeThreshold, 4.0f),
                "pointParam.fadeThreshold = 4.0");

    const GLfloat atten[3] = {1.5f, 0.25f, 0.125f};
    tglmt::gl::LegacySetPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, atten);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameterfv(DISTANCE_ATTENUATION) ⇒ không lỗi");
    const LegacyState& L = Context::Current().legacy;
    TGLMT_CHECK(Near(L.pointParam.distAttenuation[0], 1.5f) &&
                    Near(L.pointParam.distAttenuation[1], 0.25f) &&
                    Near(L.pointParam.distAttenuation[2], 0.125f),
                "pointParam.distAttenuation = (1.5, 0.25, 0.125)");

    // Lỗi: giá trị âm ⇒ INVALID_VALUE (spec GL 1.4 §3.3).
    DrainErrors();
    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MIN, -1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "SIZE_MIN âm ⇒ INVALID_VALUE");
    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MAX, -1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "SIZE_MAX âm ⇒ INVALID_VALUE");
    tglmt::gl::LegacySetPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, -1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "FADE_THRESHOLD âm ⇒ INVALID_VALUE");
    const GLfloat neg[3] = {-1.0f, 0.0f, 0.0f};
    tglmt::gl::LegacySetPointParameterfv(GL_POINT_SIZE_MAX, neg);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "PointParameterfv(SIZE_MAX, −1) ⇒ INVALID_VALUE");

    // Lỗi: pname lạ ⇒ INVALID_ENUM.
    DrainErrors();
    tglmt::gl::LegacySetPointParameterf(0x1234, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "pname lạ ⇒ INVALID_ENUM");
    tglmt::gl::LegacySetPointParameterfv(0x1234, atten);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "pname lạ (fv) ⇒ INVALID_ENUM");

    // min > max KHÔNG sinh lỗi (spec: "the point size after clamping is
    // undefined"), chỉ pname enum mới sinh lỗi.
    DrainErrors();
    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MIN, 10.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "SIZE_MIN > SIZE_MAX ⇒ KHÔNG lỗi (spec)");

    // POINT_SPRITE_COORD_ORIGIN (biến thể int).
    tglmt::gl::LegacySetPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameteri(SPRITE_COORD_ORIGIN) ⇒ không lỗi");
    TGLMT_CHECK((GLenum)L.pointParam.spriteCoordOrigin == GL_LOWER_LEFT,
                "spriteCoordOrigin = GL_LOWER_LEFT");
    tglmt::gl::LegacySetPointParameteriv(GL_POINT_SPRITE_COORD_ORIGIN,
                                         (const GLint[]){(GLint)GL_UPPER_LEFT});
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "PointParameteriv(SPRITE_COORD_ORIGIN) ⇒ không lỗi");
    TGLMT_CHECK((GLenum)L.pointParam.spriteCoordOrigin == GL_UPPER_LEFT,
                "spriteCoordOrigin = GL_UPPER_LEFT");
    DrainErrors();
    tglmt::gl::LegacySetPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, 1234);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "origin không hợp lệ ⇒ INVALID_VALUE");
    tglmt::gl::LegacySetPointParameteri(GL_POINT_SIZE_MAX, 3);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "pname float qua biến thể int ⇒ INVALID_ENUM");
}

// ---------------------------------------------------------------------------
// 7. CẤM TRONG glBegin/glEnd (GL 4.2C §2.6.3 PDF p.51)
// ---------------------------------------------------------------------------
static void TestBeginEndForbidden() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "CompatCtx cho TestBeginEndForbidden");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glBegin(GL_POINTS);
    glRasterPos3f(0.0f, 0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glRasterPos trong Begin/End ⇒ INVALID_OPERATION");
    glEnd();

    DrainErrors();
    glBegin(GL_POINTS);
    glWindowPos3f(0.0f, 0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glWindowPos trong Begin/End ⇒ INVALID_OPERATION");
    glEnd();

    DrainErrors();
    GLubyte pat[128] = {0};
    glBegin(GL_POINTS);
    glPolygonStipple(pat);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPolygonStipple trong Begin/End ⇒ INVALID_OPERATION");
    glEnd();

    DrainErrors();
    glBegin(GL_POINTS);
    tglmt::gl::LegacySetLineStipple(2, 0xFFFF);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glLineStipple trong Begin/End ⇒ INVALID_OPERATION");
    glEnd();

    DrainErrors();
    glBegin(GL_POINTS);
    tglmt::gl::LegacySetPointParameterf(GL_POINT_SIZE_MAX, 3.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glPointParameter trong Begin/End không bị cấm (spec §2.6.3)");
    glEnd();
}

// ---------------------------------------------------------------------------
// 8. CORE PROFILE ⇒ VERSION-GATE (GL 4.6 §2.3)
// ---------------------------------------------------------------------------
static void TestCoreProfileGate() {
    TGLMTContext* ctx = CoreCtx();
    TGLMT_CHECK(ctx != nullptr, "CoreCtx cho TestCoreProfileGate");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glRasterPos3f(0.0f, 0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glRasterPos3f ⇒ INVALID_OPERATION (version-gate)");
    glWindowPos2f(0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glWindowPos2f ⇒ INVALID_OPERATION (version-gate)");
    glClipPlane(GL_CLIP_PLANE0, (const GLdouble[]){0, 0, 1, 0});
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glClipPlane ⇒ INVALID_OPERATION (version-gate)");
    glPolygonStipple((const GLubyte[]){0});
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glPolygonStipple ⇒ INVALID_OPERATION (version-gate)");
    glGetClipPlane(GL_CLIP_PLANE0, (GLdouble[]){0, 0, 0, 0});
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glGetClipPlane ⇒ INVALID_OPERATION (version-gate)");
    glGetPolygonStipple((GLubyte[]){0});
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glGetPolygonStipple ⇒ INVALID_OPERATION (version-gate)");
}

}  // namespace

int main() {
    TestRasterPosition();
    TestWindowPosition();
    TestClipPlane();
    TestPolygonStipple();
    TestLineStipple();
    TestPointParameters();
    TestBeginEndForbidden();
    TestCoreProfileGate();
    return TGLMT_RESULT;
}
