// test_legacy_attribstack.cpp — ATTRIBUTE STACKS (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §6.1.22 "Saving and Restoring State"
//       (.cache/spec/glspec42.compatibility.pdf, PDF p.529–531) + Table 6.5
//       + OpenGL 2.1 §6.1.15 (glspec21.pdf, PDF p.278–280) + Table 6.2
//       + §2.6.3 (4.2C PDF p.51) cho quy tắc trong glBegin/glEnd.
//
// Những gì test kiểm tra (KHÔNG chỉ "gọi không crash"):
//  1. glPushAttrib(0) / glPushClientAttrib(0) ⇒ GL_INVALID_OPERATION.
//  2. GL_CURRENT_BIT: current color được khôi phục khi pop.
//  3. GL_ENABLE_BIT: enable bit (GL_DEPTH_TEST) được khôi phục khi pop.
//  4. GL_TRANSFORM_BIT: ma trận modelview được khôi phục khi pop.
//  5. Tràn ngăn xếp ⇒ GL_STACK_OVERFLOW; pop trên ngăn xếp rỗng ⇒
//     GL_STACK_UNDERFLOW.
//  6. Client stack: glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT) ⇒
//     client vertex array được khôi phục khi pop.
//  7. Nhóm KHÔNG được push thì KHÔNG được restore (mask quyết định).
//  8. glBegin/glEnd ⇒ GL_INVALID_OPERATION cho cả 4 lệnh.
//  9. Core Profile ⇒ GL_INVALID_OPERATION (version-gate).
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null — verify được state).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

// API nội bộ của src/gl/gl_legacy_attribstack.cpp.
namespace tglmt { namespace gl {
GLsizei LegacyAttribStackDepth();
GLsizei LegacyClientAttribStackDepth();
GLbitfield LegacyAttribStackTopMask();
GLbitfield LegacyClientAttribStackTopMask();
int LegacyMaxAttribStackDepth();
int LegacyMaxClientAttribStackDepth();
} }

namespace {

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

bool Near(float a, float b, float eps = 1e-6f) { return std::fabs(a - b) <= eps; }

}  // namespace

// ---------------------------------------------------------------------------
// 1. mask = 0 ⇒ GL_INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestZeroMaskIsError() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glPushAttrib(0);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPushAttrib(0) ⇒ GL_INVALID_OPERATION");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 0,
                "glPushAttrib(0) KHÔNG push gì vào stack");

    glPushClientAttrib(0);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPushClientAttrib(0) ⇒ GL_INVALID_OPERATION");
    TGLMT_CHECK(tglmt::gl::LegacyClientAttribStackDepth() == 0,
                "glPushClientAttrib(0) KHÔNG push gì vào client stack");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. GL_CURRENT_BIT — current color (và normal/texcoord/index) được khôi phục.
// ---------------------------------------------------------------------------
static void TestCurrentBit() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glIndexub(11);
    DrainErrors();

    glPushAttrib(GL_CURRENT_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPushAttrib(GL_CURRENT_BIT) không lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 1, "stack depth = 1");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackTopMask() == GL_CURRENT_BIT,
                "mask của entry trên cùng = GL_CURRENT_BIT");

    // Đổi state trong "giữa".
    glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glIndexub(22);
    TGLMT_CHECK(Near(c.legacy.currentColor[0], 0.0f), "màu đã đổi");

    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopAttrib không lỗi");
    TGLMT_CHECK(Near(c.legacy.currentColor[0], 1.0f) &&
                    Near(c.legacy.currentColor[1], 0.0f) &&
                    Near(c.legacy.currentColor[2], 0.0f) &&
                    Near(c.legacy.currentColor[3], 1.0f),
                "current color được khôi phục về (1,0,0,1)");
    TGLMT_CHECK(Near(c.legacy.currentNormal[0], 0.0f) &&
                    Near(c.legacy.currentNormal[2], 1.0f),
                "current normal được khôi phục");
    TGLMT_CHECK(c.legacy.currentIndex == 11u,
                "current color index được khôi phục (11)");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 0, "stack rỗng sau pop");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. GL_ENABLE_BIT — enable bit được khôi phục.
// ---------------------------------------------------------------------------
static void TestEnableBit() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glEnable(GL_DEPTH_TEST);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnable(GL_DEPTH_TEST) không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE, "depth test đang bật");

    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    TGLMT_CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE, "depth test đã tắt");

    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopAttrib không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE,
                "depth test được bật lại sau pop (GL_ENABLE_BIT)");

    // Ngược lại: push khi TẮT ⇒ pop phải tắt.
    glDisable(GL_DEPTH_TEST);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_DEPTH_TEST);
    glPopAttrib();
    TGLMT_CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE,
                "pop khôi phục trạng thái TẮT của enable bit");

    // Nhóm KHÔNG push thì KHÔNG restore: push GL_CURRENT_BIT ⇒ enable bit giữ
    // nguyên trạng thái lúc pop.
    glEnable(GL_DEPTH_TEST);
    glPushAttrib(GL_CURRENT_BIT);
    glDisable(GL_DEPTH_TEST);
    glPopAttrib();
    TGLMT_CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_FALSE,
                "chỉ push GL_CURRENT_BIT ⇒ enable bit KHÔNG được restore");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. GL_TRANSFORM_BIT — ma trận được khôi phục.
// ---------------------------------------------------------------------------
static void TestTransformBit() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    DrainErrors();

    glPushAttrib(GL_TRANSFORM_BIT);
    glTranslatef(5.0f, 6.0f, 7.0f);
    TGLMT_CHECK(!Near(c.legacy.modelview.top().m[12], 0.0f),
                "ma trận đã bị dịch");

    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopAttrib không lỗi");
    TGLMT_CHECK(Near(c.legacy.modelview.top().m[12], 0.0f) &&
                    Near(c.legacy.modelview.top().m[13], 0.0f) &&
                    Near(c.legacy.modelview.top().m[14], 0.0f),
                "ma trận modelview được khôi phục (GL_TRANSFORM_BIT)");
    TGLMT_CHECK(c.legacy.matrixMode == GL_MODELVIEW, "matrix mode được giữ");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4b. Các nhóm state khác: COLOR_BUFFER / ACCUM_BUFFER / FOG / VIEWPORT.
//     (Table 6.5, PDF p.531: mỗi bit là một nhóm state riêng.)
// ---------------------------------------------------------------------------
static void TestOtherServerGroups() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // --- GL_COLOR_BUFFER_BIT: blend + color index clear value/mask ---
    glBlendFunc(GL_ONE, GL_ZERO);
    glClearIndex(77.0f);
    glIndexMask(0xFFu);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glClearColor(0.2f, 0.3f, 0.4f, 0.5f);
    DrainErrors();

    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearIndex(12.0f);
    glIndexMask(0x0Fu);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop sau khi đổi nhóm color buffer");
    GLint srcRGB = 0;
    glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
    TGLMT_CHECK(srcRGB == GL_ONE, "blend func được khôi phục (GL_COLOR_BUFFER_BIT)");
    TGLMT_CHECK(c.legacy.colorIndex.clearIndex == 77u,
                "clear index được khôi phục (GL_COLOR_BUFFER_BIT)");
    TGLMT_CHECK(c.legacy.colorIndex.indexMask == 0xFFu,
                "index write mask được khôi phục (GL_COLOR_BUFFER_BIT)");
    GLint ci = 0;
    glGetIntegerv(GL_INDEX_CLEAR_VALUE, &ci);
    TGLMT_CHECK(ci == 77, "glGetIntegerv(GL_INDEX_CLEAR_VALUE) khớp state sau pop");
    // Color write mask + color clear value cũng thuộc nhóm color-buffer.
    TGLMT_CHECK(c.state.ColorMask(0) == 0x5,
                "color write mask được khôi phục (GL_COLOR_BUFFER_BIT)");
    TGLMT_CHECK(Near(c.clearColor[0], 0.2f) && Near(c.clearColor[3], 0.5f),
                "color clear value được khôi phục (GL_COLOR_BUFFER_BIT)");
    DrainErrors();

    // --- GL_ACCUM_BUFFER_BIT: clear value của accumulation buffer ---
    glClearAccum(0.5f, 0.25f, 0.125f, 1.0f);
    DrainErrors();
    glPushAttrib(GL_ACCUM_BUFFER_BIT);
    glClearAccum(0.0f, 0.0f, 0.0f, 0.0f);
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop nhóm accum buffer");
    TGLMT_CHECK(Near(c.legacy.accum.clearValue, 0.5f),
                "accum clear value được khôi phục (GL_ACCUM_BUFFER_BIT)");
    GLfloat ac = 0.0f;
    glGetFloatv(GL_ACCUM_CLEAR_VALUE, &ac);
    TGLMT_CHECK(Near(ac, 0.5f),
                "glGetFloatv(GL_ACCUM_CLEAR_VALUE) khớp state sau pop");
    DrainErrors();

    // --- GL_FOG_BIT: màu fog ---
    const GLfloat fog0[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    const GLfloat fog1[4] = {0.9f, 0.9f, 0.9f, 1.0f};
    glFogfv(GL_FOG_COLOR, fog0);
    DrainErrors();
    glPushAttrib(GL_FOG_BIT);
    glFogfv(GL_FOG_COLOR, fog1);
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop nhóm fog");
    TGLMT_CHECK(Near(c.legacy.fog.color[0], 0.1f) &&
                    Near(c.legacy.fog.color[1], 0.2f),
                "fog color được khôi phục (GL_FOG_BIT)");

    // --- GL_VIEWPORT_BIT: viewport ---
    glViewport(0, 0, 320, 240);
    DrainErrors();
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, 64, 48);
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop nhóm viewport");
    TGLMT_CHECK(Near(c.state.GetViewport(0).w, 320.0f) &&
                    Near(c.state.GetViewport(0).h, 240.0f),
                "viewport được khôi phục (GL_VIEWPORT_BIT)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4c. Bit KHÔNG tương ứng nhóm nào ⇒ bị bỏ qua (spec §6.1.22, PDF p.530:
//     "Bits set in mask that do not correspond to an attribute group are
//     ignored").
// ---------------------------------------------------------------------------
static void TestUnknownMaskBitIgnored() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    const GLbitfield kUnknownBit = 0x01000000u;  // không thuộc nhóm nào
    glPushAttrib(kUnknownBit);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "bit lạ (không thuộc nhóm) không gây lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 1,
                "vẫn chiếm 1 entry trong ngăn xếp");
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop entry chỉ có bit lạ");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 0, "stack rỗng");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. Tràn / tràn ngược ngăn xếp.
// ---------------------------------------------------------------------------
static void TestStackOverflowUnderflow() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    const int depth = tglmt::gl::LegacyMaxAttribStackDepth();
    TGLMT_CHECK(depth >= 16,
                "độ sâu attribute stack ≥ 16 (spec §6.1.22 PDF p.530)");

    for (int i = 0; i < depth; ++i) glPushAttrib(GL_CURRENT_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "16 lần push ⇒ không lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == depth,
                "stack depth = 16");

    // Query độ sâu (spec §6.1.22, PDF p.530: depth ≥ 16).
    GLint maxDepth = 0;
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH, &maxDepth);
    TGLMT_CHECK(maxDepth >= 16,
                "glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH) >= 16");
    GLint curDepth = 0;
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, &curDepth);
    TGLMT_CHECK(curDepth == depth,
                "glGetIntegerv(GL_ATTRIB_STACK_DEPTH) == độ sâu hiện tại");
    GLint maxClientDepth = 0;
    glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &maxClientDepth);
    TGLMT_CHECK(maxClientDepth >= 16,
                "glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH) >= 16");
    DrainErrors();

    glPushAttrib(GL_CURRENT_BIT);
    TGLMT_CHECK(glGetError() == GL_STACK_OVERFLOW,
                "push thứ 17 ⇒ GL_STACK_OVERFLOW (0x0503)");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == depth,
                "push bị từ chối ⇒ depth KHÔNG đổi");

    for (int i = 0; i < depth; ++i) glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop hết ⇒ không lỗi");

    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_STACK_UNDERFLOW,
                "pop trên stack rỗng ⇒ GL_STACK_UNDERFLOW (0x0504)");

    // Client stack: cùng quy tắc.
    const int cdepth = tglmt::gl::LegacyMaxClientAttribStackDepth();
    TGLMT_CHECK(cdepth >= 16, "độ sâu client attribute stack ≥ 16");
    for (int i = 0; i < cdepth; ++i) glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "16 lần push client ⇒ không lỗi");
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    TGLMT_CHECK(glGetError() == GL_STACK_OVERFLOW,
                "push client thứ 17 ⇒ GL_STACK_OVERFLOW");
    for (int i = 0; i < cdepth; ++i) glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop hết client stack ⇒ không lỗi");
    glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_STACK_UNDERFLOW,
                "pop client trên stack rỗng ⇒ GL_STACK_UNDERFLOW");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6. Client attribute stack: vertex array được khôi phục.
// ---------------------------------------------------------------------------
static void TestClientAttribStack() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    static const float verts[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
    glVertexPointer(3, GL_FLOAT, 0, verts);
    glEnableClientState(GL_VERTEX_ARRAY);
    glClientActiveTexture(GL_TEXTURE1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "thiết lập client array không lỗi");

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT) không lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyClientAttribStackDepth() == 1,
                "client stack depth = 1");
    TGLMT_CHECK(tglmt::gl::LegacyClientAttribStackTopMask() ==
                    GL_CLIENT_VERTEX_ARRAY_BIT,
                "mask của client entry = GL_CLIENT_VERTEX_ARRAY_BIT");

    // Đổi client state trong "giữa".
    glVertexPointer(2, GL_DOUBLE, 0, nullptr);
    glDisableClientState(GL_VERTEX_ARRAY);
    glClientActiveTexture(GL_TEXTURE3);
    TGLMT_CHECK(!c.legacy.arrayPointers[0].enabled, "vertex array đã tắt");

    glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopClientAttrib không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[0].enabled,
                "vertex array được bật lại sau pop");
    TGLMT_CHECK(c.legacy.arrayPointers[0].size == 3 &&
                    c.legacy.arrayPointers[0].type == GL_FLOAT &&
                    c.legacy.arrayPointers[0].stride == 0 &&
                    c.legacy.arrayPointers[0].pointer == verts,
                "vertex pointer được khôi phục (3, FLOAT, stride 0, con trỏ gốc)");
    TGLMT_CHECK(c.legacy.clientActiveTexture == 1u,
                "client active texture được khôi phục (1)");
    TGLMT_CHECK(tglmt::gl::LegacyClientAttribStackDepth() == 0,
                "client stack rỗng sau pop");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6b. Client stack: GL_CLIENT_PIXEL_STORE_BIT.
// ---------------------------------------------------------------------------
static void TestClientPixelStoreBit() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelStorei(GL_UNPACK_ALIGNMENT, 1)");
    TGLMT_CHECK(c.state.PixelStore().unpackAlignment == 1, "unpack alignment = 1");

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT) không lỗi");
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    TGLMT_CHECK(c.state.PixelStore().unpackAlignment == 8, "unpack alignment = 8");

    glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopClientAttrib không lỗi");
    TGLMT_CHECK(c.state.PixelStore().unpackAlignment == 1,
                "unpack alignment được khôi phục (GL_CLIENT_PIXEL_STORE_BIT)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 7. Lệnh bị cấm trong glBegin/glEnd (spec 4.2C §2.6.3, PDF p.51).
// ---------------------------------------------------------------------------
static void TestForbiddenInsideBeginEnd() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(GL_TRIANGLES);
    glPushAttrib(GL_CURRENT_BIT);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPushAttrib trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    TGLMT_CHECK(tglmt::gl::LegacyAttribStackDepth() == 0,
                "lệnh bị cấm KHÔNG được thực thi");
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPopAttrib trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPushClientAttrib trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPopClientAttrib trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glEnd();
    DrainErrors();
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 8. Core Profile: các entry point này không tồn tại ⇒ GL_INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestCoreProfileGate() {
    TGLMTContext* ctx = CoreCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glPushAttrib(GL_CURRENT_BIT);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glPushAttrib ⇒ GL_INVALID_OPERATION");
    glPopAttrib();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glPopAttrib ⇒ GL_INVALID_OPERATION");
    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glPushClientAttrib ⇒ GL_INVALID_OPERATION");
    glPopClientAttrib();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glPopClientAttrib ⇒ GL_INVALID_OPERATION");
    tglmtDestroyContext(ctx);
}

int main() {
    TestZeroMaskIsError();
    TestCurrentBit();
    TestEnableBit();
    TestTransformBit();
    TestOtherServerGroups();
    TestUnknownMaskBitIgnored();
    TestStackOverflowUnderflow();
    TestClientAttribStack();
    TestClientPixelStoreBit();
    TestForbiddenInsideBeginEnd();
    TestCoreProfileGate();
    printf("test_legacy_attribstack: done\n");
    return TGLMT_RESULT;
}
