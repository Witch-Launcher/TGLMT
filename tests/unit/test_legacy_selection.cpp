// test_legacy_selection.cpp — SELECTION / FEEDBACK / RENDER MODE / NAME STACK
// (OpenGL Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §5.2 "Selection" (PDF p.477–479),
//       §5.3 "Feedback" (PDF p.479–481), state table §6.2 Table 6.79
//       (PDF p.606: NAME STACK DEPTH / RENDER MODE / SELECTION BUFFER POINTER
//       & SIZE / FEEDBACK BUFFER POINTER, SIZE, TYPE).
// Bản GL 2.1 tương đương: glspec21.pdf §5.2 (PDF p.250–252), §5.3 (p.252–254).
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

namespace tglmt {
namespace gl {
// Hook draw path (chưa cắm trong src/gl/gl_legacy_draw.cpp — xem báo cáo):
// xử lý selection/feedback cho batch immediate-mode đã ghép primitive.
bool LegacySelectionFeedbackProcessImmediate(Context& c);
}  // namespace gl
}  // namespace tglmt

namespace {

// Xoá hết lỗi cũ — error queue là FIFO (AGENTS.md §9.1).
void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

// Độ sâu tối đa của name stack = GL_MAX_NAME_STACK_DEPTH (0x0D37) mà
// glGetIntegerv trả về (src/gl/gl_get.cpp:257 → 32).
constexpr size_t kNameStackMax = 32;

}  // namespace

// ---------------------------------------------------------------------------
// 1. glRenderMode: chuyển mode + lỗi INVALID_ENUM.
// ---------------------------------------------------------------------------
static void TestRenderMode() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Mode đầu đúng của GL là GL_RENDER (spec §6.2 Table 6.79: RENDER MODE
    // initial = RENDER). LƯU Ý: include/tglmt/LegacyState.h:502 đặt mặc định
    // 0x1C01 (GL_FEEDBACK — sai theo registry) nên TGLMT tự chuẩn hoá về
    // GL_RENDER ở lệnh selection/feedback đầu tiên (giá trị mà
    // glRenderMode trả về chứng minh điều đó).
    GLint prev = glRenderMode(GL_SELECT);
    TGLMT_CHECK(prev == GL_RENDER,
                "mode trước khi vào SELECT = GL_RENDER [spec §6.2]");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_SELECT, "mode đổi sang GL_SELECT");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "chuyển mode hợp lệ không sinh lỗi");

    // Mode không hợp lệ ⇒ INVALID_ENUM, mode không đổi.
    DrainErrors();
    glRenderMode(0x1234);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glRenderMode(0x1234) ⇒ GL_INVALID_ENUM [spec §5.2]");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_SELECT,
                "mode không đổi sau INVALID_ENUM");

    // RENDER -> FEEDBACK: trả về mode trước đó.
    DrainErrors();
    glRenderMode(GL_RENDER);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "thoát SELECT không sinh lỗi");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_RENDER, "mode đổi về GL_RENDER");
    prev = glRenderMode(GL_FEEDBACK);
    TGLMT_CHECK(prev == GL_RENDER, "glRenderMode(GL_FEEDBACK) trả về mode trước = GL_RENDER");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_FEEDBACK, "mode đổi sang GL_FEEDBACK");

    // FEEDBACK -> RENDER: theo spec §5.3 trả về SỐ GIÁ TRỊ đã ghi vào
    // feedback buffer (0 — chưa gì được ghi).
    prev = glRenderMode(GL_RENDER);
    TGLMT_CHECK(prev == 0, "thoát FEEDBACK trả về số giá trị đã ghi (=0) [spec §5.3]");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_RENDER, "mode về GL_RENDER");
}

// ---------------------------------------------------------------------------
// 2. glSelectBuffer: lỗi + buffer client memory.
// ---------------------------------------------------------------------------
static void TestSelectBuffer() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLuint buf[16] = {0};

    // size < 0 ⇒ INVALID_VALUE, buffer KHÔNG đổi.
    glSelectBuffer(-1, buf);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glSelectBuffer(-1, buf) ⇒ GL_INVALID_VALUE [spec §5.2]");
    TGLMT_CHECK(c.legacy.selection.selectBuffer == nullptr,
                "buffer select không đổi khi INVALID_VALUE");
    TGLMT_CHECK(c.legacy.selection.selectCapacity == 0, "sức chứa select không đổi");

    // size hợp lệ ⇒ lưu con trỏ + sức chứa (client memory).
    DrainErrors();
    glSelectBuffer(16, buf);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glSelectBuffer(16, buf) hợp lệ");
    TGLMT_CHECK(c.legacy.selection.selectBuffer == reinterpret_cast<GLint*>(buf),
                "con trỏ select buffer được lưu");
    TGLMT_CHECK(c.legacy.selection.selectCapacity == 16, "sức chứa select = 16");

    // buffer = NULL ⇒ selection bị tắt, KHÔNG sinh lỗi.
    DrainErrors();
    glSelectBuffer(4, nullptr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glSelectBuffer(4, NULL) không sinh lỗi");
    TGLMT_CHECK(c.legacy.selection.selectBuffer == nullptr, "selection bị tắt (buffer NULL)");
    TGLMT_CHECK(c.legacy.selection.selectCapacity == 4, "sức chứa select vẫn = 4");

    // size == 0 ⇒ không có chỗ ghi hit record ⇒ selection bị tắt, không lỗi.
    DrainErrors();
    glSelectBuffer(0, buf);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glSelectBuffer(0, buf) không sinh lỗi");
    TGLMT_CHECK(c.legacy.selection.selectCapacity == 0, "sức chứa select = 0");

    // Mode vẫn đổi được sau khi selection bị tắt.
    GLint prev = glRenderMode(GL_SELECT);
    TGLMT_CHECK(prev == GL_RENDER, "vẫn vào được GL_SELECT khi selection bị tắt");
    TGLMT_CHECK(c.legacy.selection.renderMode == GL_SELECT, "mode = GL_SELECT");

    // Gọi SelectBuffer khi đang ở selection mode ⇒ INVALID_OPERATION (§5.2).
    DrainErrors();
    glSelectBuffer(16, buf);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glSelectBuffer khi ở selection mode ⇒ GL_INVALID_OPERATION [spec §5.2]");
}

// ---------------------------------------------------------------------------
// 3. glFeedbackBuffer: lỗi + lưu type/sức chứa.
// ---------------------------------------------------------------------------
static void TestFeedbackBuffer() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLfloat fb[64] = {0.0f};

    // type sai ⇒ INVALID_ENUM.
    glFeedbackBuffer(8, 0x1234, fb);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                "glFeedbackBuffer(8, 0x1234) ⇒ GL_INVALID_ENUM [spec §5.3]");
    TGLMT_CHECK(c.legacy.selection.feedbackCapacity == 0,
                "feedback buffer không đổi khi INVALID_ENUM");

    // size < 0 ⇒ INVALID_VALUE.
    DrainErrors();
    glFeedbackBuffer(-1, GL_2D, fb);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glFeedbackBuffer(-1, GL_2D) ⇒ GL_INVALID_VALUE [spec §5.3]");

    // Hợp lệ ⇒ lưu con trỏ + sức chứa.
    DrainErrors();
    glFeedbackBuffer(32, GL_3D, fb);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFeedbackBuffer(32, GL_3D, fb) hợp lệ");
    TGLMT_CHECK(c.legacy.selection.feedbackBuffer == fb, "con trỏ feedback buffer được lưu");
    TGLMT_CHECK(c.legacy.selection.feedbackCapacity == 32, "sức chứa feedback = 32");

    // buffer = NULL ⇒ feedback bị tắt, không sinh lỗi.
    DrainErrors();
    glFeedbackBuffer(8, GL_2D, nullptr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFeedbackBuffer(..., NULL) không sinh lỗi");
    TGLMT_CHECK(c.legacy.selection.feedbackBuffer == nullptr, "feedback bị tắt");

    // size == 0 ⇒ không có chỗ ghi khối feedback ⇒ feedback bị tắt, không lỗi.
    DrainErrors();
    glFeedbackBuffer(0, GL_2D, fb);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFeedbackBuffer(0, GL_2D, fb) không sinh lỗi");
    TGLMT_CHECK(c.legacy.selection.feedbackCapacity == 0, "sức chứa feedback = 0");

    // Gọi FeedbackBuffer khi đang ở feedback mode ⇒ INVALID_OPERATION (§5.3).
    DrainErrors();
    glRenderMode(GL_FEEDBACK);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "vào GL_FEEDBACK hợp lệ");
    glFeedbackBuffer(32, GL_3D, fb);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glFeedbackBuffer khi ở feedback mode ⇒ GL_INVALID_OPERATION [spec §5.3]");
}

// ---------------------------------------------------------------------------
// 4. Name stack: InitNames / PushName / PopName / LoadName.
// ---------------------------------------------------------------------------
static void TestNameStack() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const auto& st = c.legacy.selection;

    // glInitNames: stack = {0}, current name = 0.
    glInitNames();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInitNames không sinh lỗi");
    TGLMT_CHECK(st.nameStack.size() == 1, "sau glInitNames stack có 1 phần tử");
    TGLMT_CHECK(st.nameStack.size() == 1 && st.nameStack[0] == 0,
                "phần tử khởi tạo của name stack = 0");
    TGLMT_CHECK(st.currentName == 0, "current name sau glInitNames = 0");

    // Push 3 rồi 4.
    glPushName(3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPushName(3) hợp lệ");
    glPushName(4);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPushName(4) hợp lệ");
    TGLMT_CHECK(st.nameStack.size() == 3, "stack có 3 phần tử sau 2 lần push");
    TGLMT_CHECK(st.nameStack[1] == 3 && st.nameStack[2] == 4,
                "stack chứa 3 rồi 4 (đáy trước)");
    TGLMT_CHECK(st.currentName == 4, "current name = 4 (đỉnh stack)");

    // Pop ⇒ còn 3 ở đỉnh.
    glPopName();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPopName hợp lệ");
    TGLMT_CHECK(st.nameStack.size() == 2, "stack còn 2 phần tử sau pop");
    TGLMT_CHECK(st.nameStack.back() == 3, "đỉnh stack sau pop = 3");

    // Pop hết về phần tử khởi tạo ⇒ lần pop kế tiếp là INVALID_OPERATION.
    glPopName();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pop phần tử thứ 2 hợp lệ");
    TGLMT_CHECK(st.nameStack.size() == 1, "chỉ còn phần tử khởi tạo");
    DrainErrors();
    glPopName();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "pop khi chỉ còn phần tử khởi tạo ⇒ GL_INVALID_OPERATION [spec §5.2]");
    TGLMT_CHECK(st.nameStack.size() == 1, "stack không bị thay đổi khi lỗi");

    // LoadName thay phần tử đỉnh.
    DrainErrors();
    glLoadName(7);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glLoadName(7) hợp lệ");
    TGLMT_CHECK(st.nameStack.back() == 7, "đỉnh stack = 7 sau glLoadName");
    TGLMT_CHECK(st.currentName == 7, "current name = 7 sau glLoadName");

    // name vượt GL_MAX_NAME_STACK_DEPTH ⇒ INVALID_VALUE.
    DrainErrors();
    glLoadName((GLuint)(kNameStackMax + 1));
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glLoadName vượt GL_MAX_NAME_STACK_DEPTH ⇒ GL_INVALID_VALUE [spec §5.2]");
    TGLMT_CHECK(st.nameStack.back() == 7, "đỉnh stack không đổi khi INVALID_VALUE");

    DrainErrors();
    glPushName((GLuint)(kNameStackMax + 1));
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glPushName vượt GL_MAX_NAME_STACK_DEPTH ⇒ GL_INVALID_VALUE [spec §5.2]");

    // Push vượt độ sâu stack ⇒ INVALID_VALUE.
    glInitNames();
    for (size_t i = 0; i + 1 < kNameStackMax; ++i) glPushName((GLuint)(i + 1));
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "push tới giới hạn độ sâu không lỗi");
    TGLMT_CHECK(st.nameStack.size() == kNameStackMax,
                "stack đầy (32 phần tử)");
    DrainErrors();
    glPushName(1);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "push khi stack đầy ⇒ GL_INVALID_VALUE [spec §5.2]");
    TGLMT_CHECK(st.nameStack.size() == kNameStackMax, "stack không đổi khi đầy");

    // RenderMode xoá name stack (spec §5.2: "The name stack is cleared ...
    // whenever RenderMode is called").
    glInitNames();
    glPushName(9);
    TGLMT_CHECK(st.nameStack.size() == 2, "stack = {0,9} trước khi đổi mode");
    glRenderMode(GL_SELECT);
    TGLMT_CHECK(st.nameStack.size() == 1 && st.nameStack[0] == 0,
                "glRenderMode xoá name stack về {0} [spec §5.2]");
}

// ---------------------------------------------------------------------------
// 5. Error queue là FIFO: thứ tự lỗi giữ nguyên.
// ---------------------------------------------------------------------------
static void TestErrorQueueFifo() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    GLuint buf[4] = {0};
    GLfloat fb[4] = {0.0f};

    glFeedbackBuffer(8, 0x1234, fb);   // INVALID_ENUM
    glSelectBuffer(-1, buf);           // INVALID_VALUE
    glRenderMode(0x1234);              // INVALID_ENUM
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "lỗi đầu = INVALID_ENUM");
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "lỗi thứ hai = INVALID_VALUE");
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "lỗi thứ ba = INVALID_ENUM");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "hàng đợi lỗi đã rỗng");
}

// ---------------------------------------------------------------------------
// 6. Core Profile: các entry point này KHÔNG tồn tại ⇒ INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestCoreProfileGate() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glRenderMode(GL_SELECT);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glRenderMode ⇒ GL_INVALID_OPERATION (version gate)");
    glPushName(1);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glPushName ⇒ GL_INVALID_OPERATION (version gate)");
}

// ---------------------------------------------------------------------------
// 7. Selection: hit record ghi vào selection buffer (spec §5.2, PDF p.478–479).
// ---------------------------------------------------------------------------
static void TestSelectionHitRecord() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLuint sel[64] = {0};
    glSelectBuffer(64, sel);
    glRenderMode(GL_SELECT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "vào selection mode hợp lệ");

    // Viewport 64×64, tam giác ở z = -0.5 (trong clip volume).
    glViewport(0, 0, 64, 64);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.0f, 0.5f, -0.5f);
    glEnd();
    DrainErrors();

    // Hook draw path: dự kiến cắm trong LegacyDrawImmediate
    // (src/gl/gl_legacy_draw.cpp — ngoài write-scope). Nếu hook CHƯA được cắm
    // thì rời selection mode trả về 0 hit record ⇒ tự gọi hook thủ công để
    // kiểm chứng được logic ghi hit record (test vẫn xanh khi hook được cắm).
    GLint ret = glRenderMode(GL_RENDER);
    if (ret == 0) {
        glRenderMode(GL_SELECT);
        bool suppressed = tglmt::gl::LegacySelectionFeedbackProcessImmediate(c);
        TGLMT_CHECK(suppressed, "hook báo phải bỏ rasterization khi ở selection mode");
        ret = glRenderMode(GL_RENDER);
    }
    TGLMT_CHECK(ret == 1, "1 hit record được ghi [spec §5.2]");
    TGLMT_CHECK(sel[0] == 1, "hit record[0] = số phần tử name stack (=1)");
    TGLMT_CHECK(sel[1] == 0x40000000u,
                "hit record[1] = min z = 0.25*(2^32-1) [spec §5.2]");
    TGLMT_CHECK(sel[2] == 0x40000000u,
                "hit record[2] = max z = 0.25*(2^32-1) [spec §5.2]");
    TGLMT_CHECK(sel[3] == 0, "hit record[3] = name stack (0)");
}

// ---------------------------------------------------------------------------
// 8. Feedback: khối giá trị ghi vào feedback buffer (spec §5.3, bảng 5.2).
// ---------------------------------------------------------------------------
static void TestFeedbackBlock() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLfloat fb[64] = {0.0f};
    glFeedbackBuffer(64, GL_3D, fb);
    glRenderMode(GL_FEEDBACK);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "vào feedback mode hợp lệ");

    glViewport(0, 0, 64, 64);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glBegin(GL_TRIANGLES);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.0f, 0.5f, -0.5f);
    glEnd();
    DrainErrors();

    // Hook draw path: nếu CHƯA được cắm vào LegacyDrawImmediate (ngoài
    // write-scope) thì rời feedback mode trả về 0 ⇒ tự gọi hook thủ công.
    GLint ret = glRenderMode(GL_RENDER);
    if (ret == 0) {
        glRenderMode(GL_FEEDBACK);
        bool suppressed = tglmt::gl::LegacySelectionFeedbackProcessImmediate(c);
        TGLMT_CHECK(suppressed, "hook báo phải bỏ rasterization khi ở feedback mode");
        ret = glRenderMode(GL_RENDER);
    }
    // Cấu trúc khối polygon (spec Figure 5.2, PDF p.462):
    //   POLYGON_TOKEN n vertex vertex vertex
    //   = 1 (token) + 1 (n) + 3 đỉnh × 3 giá trị (GL_3D: x,y,z) = 11.
    TGLMT_CHECK(ret == 11, "11 giá trị feedback đã ghi [spec §5.3 / hình 5.2]");
    TGLMT_CHECK(fb[0] == 3.0f, "token = POLYGON (0x0003) [spec hình 5.2]");
    TGLMT_CHECK(fb[1] == 3.0f, "n = số đỉnh của polygon (=3) [spec hình 5.2]");
    TGLMT_CHECK(fb[2] == 16.0f && fb[3] == 16.0f,
                "đỉnh 0: toạ độ window x,y = (16,16)");
    TGLMT_CHECK(fb[4] == 0.25f, "đỉnh 0: z window = 0.25");
    TGLMT_CHECK(fb[5] == 48.0f && fb[6] == 16.0f,
                "đỉnh 1: toạ độ window x,y = (48,16)");
    TGLMT_CHECK(fb[8] == 32.0f && fb[9] == 48.0f,
                "đỉnh 2: toạ độ window x,y = (32,48)");
}

// ---------------------------------------------------------------------------
// 9. Lệnh name stack trong glBegin/glEnd ⇒ INVALID_OPERATION
//    (spec GL 4.2C §2.6.3, PDF p.51).
// ---------------------------------------------------------------------------
static void TestNameStackInBeginEnd() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility 4.6");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glInitNames();
    const size_t depthBefore = c.legacy.selection.nameStack.size();

    glBegin(GL_TRIANGLES);
    glPushName(5);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPushName trong Begin/End ⇒ GL_INVALID_OPERATION [spec §2.6.3]");
    glPopName();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glPopName trong Begin/End ⇒ GL_INVALID_OPERATION [spec §2.6.3]");
    glLoadName(6);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glLoadName trong Begin/End ⇒ GL_INVALID_OPERATION [spec §2.6.3]");
    glInitNames();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glInitNames trong Begin/End ⇒ GL_INVALID_OPERATION [spec §2.6.3]");
    TGLMT_CHECK(c.legacy.selection.nameStack.size() == depthBefore,
                "name stack không đổi khi lệnh bị chặn trong Begin/End");
    glEnd();
    DrainErrors();

    // Ngoài Begin/End thì lệnh chạy bình thường.
    glPushName(5);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPushName ngoài Begin/End hợp lệ");
    TGLMT_CHECK(c.legacy.selection.nameStack.back() == 5, "đỉnh stack = 5");
}

int main() {
    TestRenderMode();
    TestSelectBuffer();
    TestFeedbackBuffer();
    TestNameStack();
    TestErrorQueueFifo();
    TestCoreProfileGate();
    TestSelectionHitRecord();
    TestFeedbackBlock();
    TestNameStackInBeginEnd();
    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_selection: OK\n");
    } else {
        printf("test_legacy_selection: %d FAILURES\n", tglmt_test::Failures());
    }
    return TGLMT_RESULT;
}
