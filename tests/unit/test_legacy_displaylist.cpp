// test_legacy_displaylist.cpp — 8 entry point display list (Compatibility).
//
// Spec: OpenGL 2.1 §5.4 "Display Lists" (glspec21.pdf PDF p.256–259 ⇒ spec
// p.242–245). Các trích dẫn quan trọng dùng trong test:
//
//  p.242  "A display list is simply a group of GL commands and arguments that
//          has been stored for subsequent execution. ... This causes the
//          commands saved in the display list to be executed, in order, just
//          as if they were issued without using a display list. If n = 0,
//          then the error INVALID VALUE is generated."
//  p.242  "If mode is COMPILE, then commands are not executed as they are
//          placed in the display list. If mode is COMPILE AND EXECUTE then
//          commands are executed as they are encountered, then placed in the
//          display list. If n = 0, then the error INVALID VALUE is generated."
//  p.242  "The error INVALID OPERATION is generated if EndList is called
//          without a previous matching NewList, or if NewList is called a
//          second time before calling EndList."
//  p.243  "uint GenLists( sizei s ); returns an integer n such that the
//          indices n, . . . , n+s-1 are previously unused ... GenLists also
//          has the effect of creating an empty display list for each of the
//          indices n, . . . , n+s-1, so that these indices all become used.
//          GenLists returns 0 if there is no group of s contiguous previously
//          unused display list indices, or if s = 0."
//  p.243  "If the mode supplied to NewList is COMPILE AND EXECUTE, then the
//          appropriate lists are executed, but the CallList or CallLists,
//          rather than those lists' constituent commands, is placed in the
//          list under construction."
//  p.244  "boolean IsList( uint list ); returns TRUE if list is the index of
//          some display list."
//  p.244  "void DeleteLists( uint list, sizei range ); ... Indices to which
//          no display list corresponds are ignored. If range = 0, nothing
//          happens."
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null — không có GPU).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cstdio>

using namespace tglmt;

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

// Vẽ 1 tam giác (immediate mode).
void DrawTriangle() {
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
}

// Biên dịch 1 list chứa 1 tam giác (mode do tham số).
void CompileTriangle(GLuint list, GLenum mode) {
    glNewList(list, mode);
    DrawTriangle();
    glEndList();
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. Vòng đời list + mã lỗi (spec p.242).
// ---------------------------------------------------------------------------
static void TestLifecycle() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // --- list hợp lệ ---
    glNewList(1, GL_COMPILE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNewList(1, GL_COMPILE) hợp lệ");
    glEndList();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEndList hợp lệ");
    TGLMT_CHECK(glIsList(1) == GL_TRUE, "glIsList(1) == GL_TRUE sau glEndList");
    TGLMT_CHECK(glIsList(99) == GL_FALSE, "glIsList(99) == GL_FALSE (chỉ số không tồn tại)");
    TGLMT_CHECK(glIsList(0) == GL_FALSE, "glIsList(0) == GL_FALSE (0 không phải chỉ số list)");

    // --- glNewList(0) ⇒ INVALID_VALUE (spec p.242) ---
    DrainErrors();
    glNewList(0, GL_COMPILE);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glNewList(0, ...) ⇒ GL_INVALID_VALUE [spec §5.4 p.242]");
    // Không được mở capture ⇒ glEndList tiếp theo phải là INVALID_OPERATION.
    DrainErrors();
    glEndList();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glNewList(0) không được mở capture ⇒ glEndList ⇒ INVALID_OPERATION");

    // --- mode sai ⇒ INVALID_ENUM ---
    DrainErrors();
    glNewList(2, 0x1234);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glNewList(1, 0x1234) ⇒ GL_INVALID_ENUM");
    DrainErrors();
    glEndList();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glNewList với mode sai không được mở capture");

    // --- glEndList không có glNewList ⇒ INVALID_OPERATION (spec p.242) ---
    DrainErrors();
    glEndList();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glEndList không có glNewList ⇒ GL_INVALID_OPERATION [spec §5.4 p.242]");

    // --- glNewList lần 2 trước glEndList ⇒ INVALID_OPERATION (spec p.242) ---
    DrainErrors();
    glNewList(3, GL_COMPILE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNewList(3) lần 1 hợp lệ");
    glNewList(4, GL_COMPILE);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glNewList lần 2 trước glEndList ⇒ GL_INVALID_OPERATION [spec §5.4 p.242]");
    glEndList();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEndList sau lỗi vẫn đóng list 3");
    TGLMT_CHECK(glIsList(3) == GL_TRUE, "list 3 được tạo");
    TGLMT_CHECK(glIsList(4) == GL_FALSE,
                "list 4 KHÔNG được tạo (glNewList lần 2 đã lỗi)");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. glGenLists / glDeleteLists / glIsList (spec p.243–244).
// ---------------------------------------------------------------------------
static void TestGenDelete() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // --- glGenLists(3) ⇒ block 3 chỉ số LIÊN TIẾP chưa dùng ---
    GLuint n = glGenLists(3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGenLists(3) không lỗi");
    TGLMT_CHECK(n != 0, "glGenLists(3) trả chỉ số khác 0");
    // Spec p.243: "GenLists also has the effect of creating an empty display
    // list for each of the indices n, . . . , n+s-1, so that these indices
    // all become used."
    TGLMT_CHECK(glIsList(n) == GL_TRUE, "glIsList(n) == GL_TRUE (list rỗng cũng là list)");
    TGLMT_CHECK(glIsList(n + 1) == GL_TRUE, "glIsList(n+1) == GL_TRUE");
    TGLMT_CHECK(glIsList(n + 2) == GL_TRUE, "glIsList(n+2) == GL_TRUE");

    // Block thứ 2 phải KHÔNG trùng block thứ nhất (đã dùng).
    GLuint m = glGenLists(3);
    TGLMT_CHECK(m != 0, "glGenLists(3) lần 2 trả chỉ số khác 0");
    TGLMT_CHECK(m != n && m != n + 1 && m != n + 2,
                "glGenLists trả block KHÔNG trùng block đã dùng");
    TGLMT_CHECK(m == n + 3, "block thứ 2 liên tiếp ngay sau block thứ nhất");

    // --- glDeleteLists(n, 2) ⇒ 2 list đầu biến mất, list thứ 3 còn ---
    DrainErrors();
    glDeleteLists(n, 2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDeleteLists không lỗi");
    TGLMT_CHECK(glIsList(n) == GL_FALSE, "glIsList(n) == GL_FALSE sau glDeleteLists");
    TGLMT_CHECK(glIsList(n + 1) == GL_FALSE, "glIsList(n+1) == GL_FALSE sau glDeleteLists");
    TGLMT_CHECK(glIsList(n + 2) == GL_TRUE, "glIsList(n+2) còn TRUE (chỉ xoá 2)");

    // --- glDeleteLists(x, 0) ⇒ không làm gì (spec p.244) ---
    DrainErrors();
    glDeleteLists(n + 2, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDeleteLists(x, 0) không lỗi");
    TGLMT_CHECK(glIsList(n + 2) == GL_TRUE, "glDeleteLists(x, 0) không xoá gì");

    // --- chỉ số không tồn tại bị bỏ qua ---
    DrainErrors();
    glDeleteLists(900000, 3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDeleteLists trên chỉ số không tồn tại ⇒ không lỗi");

    // --- range < 0 ⇒ INVALID_VALUE; range == 0 ⇒ trả 0 ---
    DrainErrors();
    GLuint r = glGenLists(-1);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glGenLists(-1) ⇒ GL_INVALID_VALUE");
    TGLMT_CHECK(r == 0, "glGenLists(-1) trả 0");
    DrainErrors();
    r = glGenLists(0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGenLists(0) không lỗi");
    TGLMT_CHECK(r == 0, "glGenLists(0) trả 0 [spec p.243]");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. GL_COMPILE không vẽ lúc biên dịch; glCallList thì vẽ (spec p.242).
// ---------------------------------------------------------------------------
static void TestCompileAndCall() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const uint64_t v0 = c.pipeStats.vertsSubmitted;
    const uint64_t p0 = c.pipeStats.primsSubmitted;

    CompileTriangle(10, GL_COMPILE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "biên dịch tam giác trong list không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0,
                "GL_COMPILE: KHÔNG vẽ lúc biên dịch [spec §5.4 p.242]");
    TGLMT_CHECK(c.pipeStats.primsSubmitted == p0,
                "GL_COMPILE: KHÔNG có primitive nào được submit");

    glCallList(10);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallList(10) không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 3,
                "glCallList(10) ⇒ 3 đỉnh được vẽ");
    TGLMT_CHECK(c.pipeStats.primsSubmitted == p0 + 1,
                "glCallList(10) ⇒ 1 primitive được vẽ");

    glCallList(10);
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6,
                "glCallList(10) lần 2 ⇒ vẽ lại (3 đỉnh nữa)");

    // --- glCallList(0) ⇒ INVALID_VALUE (spec p.242) ---
    DrainErrors();
    glCallList(0);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glCallList(0) ⇒ GL_INVALID_VALUE [spec §5.4 p.242]");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6, "glCallList(0) không vẽ gì");

    // --- glCallList với chỉ số không tồn tại ⇒ không lỗi, không vẽ ---
    DrainErrors();
    glCallList(4242);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glCallList với chỉ số không tồn tại ⇒ không lỗi [spec p.243]");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6,
                "glCallList chỉ số không tồn tại ⇒ không vẽ gì");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. GL_COMPILE_AND_EXECUTE vừa thực thi vừa ghi (spec p.242).
// ---------------------------------------------------------------------------
static void TestCompileAndExecute() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const uint64_t v0 = c.pipeStats.vertsSubmitted;
    CompileTriangle(20, GL_COMPILE_AND_EXECUTE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "biên dịch+thực thi không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 3,
                "GL_COMPILE_AND_EXECUTE: vẽ NGAY lúc biên dịch [spec §5.4 p.242]");
    TGLMT_CHECK(glIsList(20) == GL_TRUE, "list 20 được tạo");

    glCallList(20);
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6,
                "glCallList(20) vẽ lại tam giác đã ghi");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. glCallLists: thứ tự, kiểu offset, listBase (spec p.243).
// ---------------------------------------------------------------------------
static void TestCallLists() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    CompileTriangle(30, GL_COMPILE);
    CompileTriangle(31, GL_COMPILE);
    const uint64_t v0 = c.pipeStats.vertsSubmitted;

    // --- 2 list, kiểu GL_UNSIGNED_INT ---
    GLuint arr[2] = {30u, 31u};
    glCallLists(2, GL_UNSIGNED_INT, arr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallLists(2, GL_UNSIGNED_INT) không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6,
                "glCallLists thực thi CẢ HAI list (3+3 đỉnh)");

    // --- n == 0 ⇒ không làm gì ---
    DrainErrors();
    glCallLists(0, GL_UNSIGNED_INT, arr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallLists(0, ...) không lỗi [spec p.243]");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 6, "glCallLists(0) không vẽ gì");

    // --- n < 0 ⇒ INVALID_VALUE ---
    DrainErrors();
    glCallLists(-1, GL_UNSIGNED_INT, arr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glCallLists(-1, ...) ⇒ GL_INVALID_VALUE");

    // --- type sai ⇒ INVALID_ENUM ---
    DrainErrors();
    glCallLists(1, 0x1234, arr);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glCallLists với type sai ⇒ GL_INVALID_ENUM");

    // --- GL_FLOAT: cắt phần thập phân (spec p.243) ---
    CompileTriangle(32, GL_COMPILE);
    const uint64_t v1 = c.pipeStats.vertsSubmitted;
    GLfloat f = 32.9f;  // truncate ⇒ 32
    glCallLists(1, GL_FLOAT, &f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallLists với GL_FLOAT không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v1 + 3,
                "GL_FLOAT: offset 32.9 bị CẮT về 32 [spec p.243]");

    // --- GL_2_BYTES: ghép big-endian ---
    const uint64_t v2 = c.pipeStats.vertsSubmitted;
    GLubyte off2[2] = {0u, 30u};  // offset = (0 << 8) + 30 = 30
    glCallLists(1, GL_2_BYTES, off2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallLists với GL_2_BYTES không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v2 + 3,
                "GL_2_BYTES: offset big-endian (0<<8)+30 = 30 [spec p.243]");

    // --- GL_3_BYTES + glListBase ---
    CompileTriangle(40, GL_COMPILE);  // list ở chỉ số 40 = base(10) + offset(30)
    const uint64_t v3 = c.pipeStats.vertsSubmitted;
    GLubyte off3[3] = {0u, 0u, 30u};  // offset = 30
    glListBase(10);
    glCallLists(1, GL_3_BYTES, off3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallLists với GL_3_BYTES + listBase không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v3 + 3,
                "GL_3_BYTES + glListBase(10): offset 30 + base 10 = list 40 [spec p.243]");
    glListBase(0);

    // --- listBase sai ⇒ list không tồn tại ⇒ không vẽ gì ---
    const uint64_t v4 = c.pipeStats.vertsSubmitted;
    glListBase(1000);
    glCallLists(1, GL_3_BYTES, off3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "listBase sai ⇒ không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v4,
                "listBase sai ⇒ chỉ số không tồn tại ⇒ không vẽ gì");
    glListBase(0);

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6. List lồng nhau: ghi LỆNH call, không ghi nội dung (spec p.243).
// ---------------------------------------------------------------------------
static void TestNestedList() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    CompileTriangle(50, GL_COMPILE);
    const uint64_t v0 = c.pipeStats.vertsSubmitted;

    // List 51 chứa lệnh glCallList(50).
    glNewList(51, GL_COMPILE);
    glCallList(50);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallList trong list không lỗi");
    glEndList();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEndList sau glCallList không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0,
                "GL_COMPILE: list lồng nhau KHÔNG được thực thi lúc biên dịch [spec p.243]");

    glCallList(51);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallList(51) không lỗi");
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v0 + 3,
                "glCallList(51) thực thi list con (50) ⇒ 3 đỉnh [spec p.243]");

    // --- GL_COMPILE_AND_EXECUTE: list con ĐƯỢC thực thi ngay, nhưng lệnh
    //     call (không phải nội dung của list con) mới được ghi (spec p.243).
    const uint64_t v1 = c.pipeStats.vertsSubmitted;
    glNewList(52, GL_COMPILE_AND_EXECUTE);
    glCallList(50);
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v1 + 3,
                "GL_COMPILE_AND_EXECUTE: list con được thực thi ngay [spec p.243]");
    glEndList();
    glCallList(52);
    TGLMT_CHECK(c.pipeStats.vertsSubmitted == v1 + 6,
                "list 52 phát lại gọi lại list 50 (3 đỉnh nữa)");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 7. CLIENT state được chụp lúc GHI; lệnh trong list KHÔNG được thực thi
//    khi mode = GL_COMPILE (spec p.242).
// ---------------------------------------------------------------------------
static void TestClientStateCapturedAtCompileTime() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    // Current color = GREEN trước khi biên dịch.
    glColor3f(0.0f, 1.0f, 0.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glColor3f ngoài list không lỗi");

    glNewList(60, GL_COMPILE);
    glColor3f(1.0f, 0.0f, 0.0f);  // RED — nằm TRONG list
    DrawTriangle();
    glEndList();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "biên dịch list có glColor không lỗi");

    // Spec p.242: mode = COMPILE ⇒ "commands are not executed as they are
    // placed in the display list" ⇒ current color PHẢI giữ nguyên GREEN.
    GLfloat col[4] = {0, 0, 0, 0};
    glGetFloatv(GL_CURRENT_COLOR, col);
    TGLMT_CHECK(col[0] == 0.0f && col[1] == 1.0f && col[2] == 0.0f,
                "GL_COMPILE: glColor trong list KHÔNG được thực thi lúc biên dịch [spec p.242]");

    // Đổi current color ra ngoài list rồi gọi list ⇒ lệnh color TRONG list
    // được thực thi lúc replay ⇒ current color = RED.
    glColor3f(0.0f, 0.0f, 1.0f);  // BLUE
    glCallList(60);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallList(60) không lỗi");
    glGetFloatv(GL_CURRENT_COLOR, col);
    TGLMT_CHECK(col[0] == 1.0f && col[1] == 0.0f && col[2] == 0.0f,
                "glCallList thực thi lệnh glColor đã ghi trong list (current color = RED)");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

int main() {
    TestLifecycle();
    TestGenDelete();
    TestCompileAndCall();
    TestCompileAndExecute();
    TestCallLists();
    TestNestedList();
    TestClientStateCapturedAtCompileTime();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_displaylist: OK\n");
        return 0;
    }
    return 1;
}
