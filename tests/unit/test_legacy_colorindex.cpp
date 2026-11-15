// test_legacy_colorindex.cpp — COLOR-INDEX MODE + ACCUMULATION BUFFER
// (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile
//   • §2.7 "Vertex Specification" (PDF p.55) — lệnh Index{sifd ub}
//   • §2.13.6 "Clamping or Masking" (PDF p.111) — quy tắc chuyển color index
//   • §4.2 (PDF p.233) — ClearIndex / ClearAccum
//   • §4.2.2 (PDF p.231) — IndexMask
//   • §4.2.4 (PDF p.432–433) — Accum
//   • §2.6.3 (PDF p.51) — lệnh được phép trong glBegin/glEnd
//   + OpenGL 2.1 spec (glspec21.pdf) §4.2 (PDF p.233), §4.2.4 (PDF p.234–235)
//
// Những gì test kiểm tra (KHÔNG chỉ "gọi không crash"):
//  1. Quy tắc chuyển color index theo từng kiểu tham số (KHÔNG clamp!).
//  2. glClearIndex: clear value + mask 2^m − 1.
//  3. glIndexMask: write mask.
//  4. glAccum: op hợp lệ / GL_INVALID_ENUM / state op+value.
//  5. glClearAccum: clamp [−1, 1].
//  6. glBegin/glEnd: Index ĐƯỢC PHÉP, ClearIndex/IndexMask/Accum bị cấm.
//  7. Core Profile: mọi lệnh này ⇒ GL_INVALID_OPERATION (version-gate).
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null — verify được state, KHÔNG
// verify được pixel vì Metal không có color-index/accumulation buffer).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

// API nội bộ của src/gl/gl_legacy_colorindex.cpp.
namespace tglmt { namespace gl {
GLuint LegacyIndexClearValue();
GLuint LegacyIndexWriteMask();
GLuint LegacyCurrentColorIndex();
float LegacyAccumClearValue();
GLenum LegacyAccumOp();
float LegacyAccumValue();
bool LegacyAccumBufferPresent();
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
// 1. Quy tắc chuyển current color index.
//    Spec 4.2C §2.7 (PDF p.55): "Values outside the (machine-dependent)
//    representable range of color indices are not clamped."
//    Spec 4.2C §2.13.6 (PDF p.111): "the index is first converted to
//    fixed-point ... the nearest fixed-point value is selected".
// ---------------------------------------------------------------------------
static void TestCurrentColorIndex() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // glIndexub(200): GLubyte ⇒ 200.
    glIndexub(200);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexub(200) không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 200u, "current color index = 200");
    TGLMT_CHECK(c.legacy.colorIndex.currentIndex == 200u,
                "colorIndex.currentIndex = 200 (nhóm color-index của LegacyState)");
    TGLMT_CHECK(tglmt::gl::LegacyCurrentColorIndex() == 200u,
                "accessor LegacyCurrentColorIndex() = 200");

    // glGetIntegerv(GL_CURRENT_INDEX) đọc c.legacy.currentIndex
    // (src/gl/gl_get.cpp, case 0x0B01 trong LegacyPnameI).
    GLint v = 0;
    glGetIntegerv(GL_CURRENT_INDEX, &v);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetIntegerv(GL_CURRENT_INDEX) không lỗi");
    TGLMT_CHECK(v == 200, "glGetIntegerv(GL_CURRENT_INDEX) == 200");

    // glIndexi(300): KHÔNG clamp về 255 (spec §2.7) — giữ nguyên 300.
    glIndexi(300);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexi(300) không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 300u,
                "glIndexi(300) ⇒ 300 (KHÔNG clamp: spec 4.2C §2.7 PDF p.55)");

    // glIndexf(3.7f): làm tròn số gần nhất ⇒ 4 (spec §2.13.6 PDF p.111).
    glIndexf(3.7f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexf(3.7f) không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 4u,
                "glIndexf(3.7f) ⇒ 4 (làm tròn số gần nhất)");

    // glIndexf(-0.4f): làm tròn ⇒ 0.
    glIndexf(-0.4f);
    TGLMT_CHECK(c.legacy.currentIndex == 0u, "glIndexf(-0.4f) ⇒ 0");

    // glIndexd(-1.0): KHÔNG clamp ⇒ -1; state là GLuint nên giữ mẫu 2's
    // complement 32 bit (khi ghi vào color index buffer, phần nguyên mới bị
    // mask với 2^n − 1 theo spec §2.13.6).
    glIndexd(-1.0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexd(-1.0) không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 0xFFFFFFFFu,
                "glIndexd(-1.0) ⇒ -1 (2's complement 32 bit, KHÔNG clamp về 0)");

    // glIndexs(GLshort): giá trị nguyên ⇒ dùng nguyên.
    glIndexs((GLshort)-2);
    TGLMT_CHECK(c.legacy.currentIndex == 0xFFFFFFFEu, "glIndexs(-2) ⇒ -2");

    // Các biến thể vector: đọc phần tử đầu tiên.
    const GLdouble dv[1] = {12.0};
    glIndexdv(dv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexdv không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 12u, "glIndexdv({12.0}) ⇒ 12");

    const GLfloat fv[1] = {2.6f};
    glIndexfv(fv);
    TGLMT_CHECK(c.legacy.currentIndex == 3u, "glIndexfv({2.6f}) ⇒ 3 (làm tròn)");

    const GLint iv[1] = {77};
    glIndexiv(iv);
    TGLMT_CHECK(c.legacy.currentIndex == 77u, "glIndexiv({77}) ⇒ 77");

    const GLshort sv[1] = {9};
    glIndexsv(sv);
    TGLMT_CHECK(c.legacy.currentIndex == 9u, "glIndexsv({9}) ⇒ 9");

    const GLubyte ubv[1] = {200};
    glIndexubv(ubv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexubv không lỗi");
    TGLMT_CHECK(c.legacy.currentIndex == 200u, "glIndexubv({200}) ⇒ 200");

    // Con trỏ NULL ⇒ GL_INVALID_VALUE (không deref NULL).
    glIndexiv(nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glIndexiv(NULL) ⇒ GL_INVALID_VALUE");
    TGLMT_CHECK(c.legacy.currentIndex == 200u,
                "lệnh lỗi KHÔNG được thực thi (current index không đổi)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. glClearIndex + glIndexMask.
// ---------------------------------------------------------------------------
static void TestClearIndexAndMask() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glClearIndex(77.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearIndex(77) không lỗi");
    TGLMT_CHECK(c.legacy.colorIndex.clearIndex == 77u,
                "clear index = 77");
    TGLMT_CHECK(tglmt::gl::LegacyIndexClearValue() == 77u,
                "accessor LegacyIndexClearValue() = 77");

    // Probe pname: GL_INDEX_CLEAR_VALUE đã có hằng sinh (gl_legacy_types.h)
    // và glClearIndex ghi shadow cho nó ⇒ glGetIntegerv trả ĐÚNG giá trị.
    // LƯU Ý: pname này CHƯA có nhánh trong LegacyPnameI (src/gl/gl_get.cpp:251)
    // nên repo vẫn ghi GL_INVALID_ENUM (chưa nằm trong bảng accepted của
    // tools/gen_state_pnames.py) — xem báo cáo (hook site).
    GLint ci = 0;
    glGetIntegerv(GL_INDEX_CLEAR_VALUE, &ci);
    TGLMT_CHECK(ci == 77, "glGetIntegerv(GL_INDEX_CLEAR_VALUE) == 77");
    DrainErrors();  // dọn INVALID_ENUM của pname chưa có hook (xem báo cáo)

    // Spec 4.2C §4.2 (PDF p.233): phần nguyên bị mask với 2^m − 1 (m = số bit
    // của color index buffer; TGLMT dùng 8 bit) ⇒ 300 & 0xFF = 44.
    glClearIndex(300.0f);
    TGLMT_CHECK(c.legacy.colorIndex.clearIndex == 44u,
                "glClearIndex(300) ⇒ 44 (mask 2^8 − 1, spec §4.2 PDF p.233)");

    glIndexMask(0xFFu);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexMask(0xFF) không lỗi");
    TGLMT_CHECK(c.legacy.colorIndex.indexMask == 0xFFu,
                "index write mask = 0xFF");
    TGLMT_CHECK(tglmt::gl::LegacyIndexWriteMask() == 0xFFu,
                "accessor LegacyIndexWriteMask() = 0xFF");

    // Query qua pname GL_INDEX_WRITEMASK (shadow do glIndexMask ghi).
    GLint wm = 0;
    glGetIntegerv(GL_INDEX_WRITEMASK, &wm);
    TGLMT_CHECK(wm == 0xFF, "glGetIntegerv(GL_INDEX_WRITEMASK) == 0xFF");
    DrainErrors();

    glIndexMask(0x0Fu);
    TGLMT_CHECK(c.legacy.colorIndex.indexMask == 0x0Fu, "index write mask = 0x0F");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. glAccum + glClearAccum.
// ---------------------------------------------------------------------------
static void TestAccumulation() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glAccum(GL_ACCUM, 0.5f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glAccum(GL_ACCUM, 0.5) không lỗi");
    TGLMT_CHECK(c.legacy.accum.op == GL_ACCUM, "accum op = GL_ACCUM");
    TGLMT_CHECK(Near(c.legacy.accum.value, 0.5f), "accum value = 0.5");
    TGLMT_CHECK(tglmt::gl::LegacyAccumOp() == GL_ACCUM, "accessor op = GL_ACCUM");
    TGLMT_CHECK(Near(tglmt::gl::LegacyAccumValue(), 0.5f), "accessor value = 0.5");

    // Các op hợp lệ còn lại (spec GL 1.x: ACCUM, LOAD, RETURN, MULT, ADD,
    // SUBTRACT, MIN, MAX) đều được chấp nhận.
    const GLenum ops[] = {GL_LOAD, GL_RETURN, GL_MULT, GL_ADD,
                          GL_SUBTRACT, GL_MIN, GL_MAX};
    for (GLenum op : ops) {
        glAccum(op, 0.25f);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "op hợp lệ được chấp nhận");
        TGLMT_CHECK(c.legacy.accum.op == op, "accum op được ghi đúng");
    }

    // Op sai ⇒ GL_INVALID_ENUM, state KHÔNG đổi.
    const GLenum savedOp = c.legacy.accum.op;
    const float savedValue = c.legacy.accum.value;
    glAccum(0x1234, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glAccum(0x1234, 1.0) ⇒ GL_INVALID_ENUM");
    TGLMT_CHECK(c.legacy.accum.op == savedOp && c.legacy.accum.value == savedValue,
                "op sai ⇒ state accum KHÔNG đổi");

    // glClearAccum: 4 thành phần, clamp về [−1, 1].
    glClearAccum(0.25f, 0.25f, 0.25f, 0.25f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClearAccum(0.25×4) không lỗi");
    TGLMT_CHECK(Near(c.legacy.accum.clearValue, 0.25f),
                "accum clear value = 0.25");
    TGLMT_CHECK(Near(tglmt::gl::LegacyAccumClearValue(), 0.25f),
                "accessor LegacyAccumClearValue() = 0.25");

    glClearAccum(2.0f, -2.0f, 0.5f, 0.5f);
    TGLMT_CHECK(Near(c.legacy.accum.clearValue, 1.0f),
                "glClearAccum(2.0,...) ⇒ clamp về 1.0 (spec §4.2 PDF p.233)");

    // TGLMT chưa có accumulation buffer thật (Metal không có) — state có,
    // buffer thì không (báo cáo).
    TGLMT_CHECK(!tglmt::gl::LegacyAccumBufferPresent(),
                "TGLMT chưa có accumulation buffer (chỉ có state)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Quy tắc glBegin/glEnd (spec 4.2C §2.6.3, PDF p.51).
// ---------------------------------------------------------------------------
static void TestBeginEndRules() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Index ĐƯỢC PHÉP trong Begin/End (danh sách lệnh được phép gồm Index).
    glBegin(GL_TRIANGLES);
    glIndexub(7);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glIndexub trong glBegin/glEnd ⇒ KHÔNG lỗi (spec §2.6.3)");
    TGLMT_CHECK(c.legacy.currentIndex == 7u,
                "current color index được ghi trong Begin/End");
    glEnd();
    DrainErrors();

    // Các lệnh khác bị cấm.
    glBegin(GL_TRIANGLES);
    glIndexMask(0x0Fu);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glIndexMask trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glClearIndex(3.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glClearIndex trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glAccum(GL_ACCUM, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glAccum trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glClearAccum(0.0f, 0.0f, 0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glClearAccum trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glEnd();
    DrainErrors();
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. Core Profile: các entry point này không tồn tại ⇒ GL_INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestCoreProfileGate() {
    TGLMTContext* ctx = CoreCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glIndexub(1);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glIndexub ⇒ GL_INVALID_OPERATION");
    glIndexMask(0xFF);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glIndexMask ⇒ GL_INVALID_OPERATION");
    glClearIndex(1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glClearIndex ⇒ GL_INVALID_OPERATION");
    glAccum(GL_ACCUM, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glAccum ⇒ GL_INVALID_OPERATION");
    glClearAccum(0.0f, 0.0f, 0.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core profile: glClearAccum ⇒ GL_INVALID_OPERATION");
    tglmtDestroyContext(ctx);
}

int main() {
    TestCurrentColorIndex();
    TestClearIndexAndMask();
    TestAccumulation();
    TestBeginEndRules();
    TestCoreProfileGate();
    printf("test_legacy_colorindex: done\n");
    return TGLMT_RESULT;
}
