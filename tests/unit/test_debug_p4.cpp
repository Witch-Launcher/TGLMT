// test_debug_p4.cpp — P4: KHR_debug + EXT_debug_marker + EXT_discard_framebuffer
// + ARB_parallel_shader_compile. Verify behavior thật (callback, log, labels,
// marker stack, discard validation). Chạy cả 2 backend (không cần Metal device).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

// Forward-declare base impls (tglmt::gl namespace, generator sinh suffix aliases).
// Test gọi base trực tiếp (tránh phụ thuộc generated header cho ext mới).
namespace tglmt { namespace gl {
void glMaxShaderCompilerThreads(GLuint count);
void glDiscardFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);
void glPushGroupMarker(GLsizei length, const GLchar* marker);
void glPopGroupMarker();
void glInsertEventMarker(GLsizei length, const GLchar* marker);
// KHR_debug ptr-label + pointerv (chưa có trong generated gl46.h).
void glObjectPtrLabel(const void* ptr, GLsizei length, const GLchar* label);
void glGetObjectPtrLabel(const void* ptr, GLsizei bufSize, GLsizei* length, GLchar* label);
void glGetPointerv(GLenum pname, void** params);
} }

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_debug_p4 FAIL: %s\n", msg); ++gFails; }
}

// Callback capture cho KHR_debug test.
static int gCbCount = 0;
static GLenum gCbSource = 0, gCbType = 0, gCbSev = 0;
static GLuint gCbId = 0;
static std::string gCbMsg;
static void DebugCb(GLenum src, GLenum type, GLuint id, GLenum sev,
                      GLsizei len, const GLchar* msg, const void* user) {
    (void)user;
    ++gCbCount;
    gCbSource = src; gCbType = type; gCbId = id; gCbSev = sev;
    gCbMsg.assign(msg ? msg : "", (len >= 0) ? (size_t)len : 0);
}

int main() {
    // Gắn context Null (test không cần Metal device).
    // Context::Current() mặc định đã là Null fallback.
    Context& c = Context::Current();

    // ===== ARB_parallel_shader_compile: hint, không lỗi =====
    glMaxShaderCompilerThreads(4);
    Check(glGetError() == 0, "MaxShaderCompilerThreadsARB lỗi");
    glMaxShaderCompilerThreads(0);
    Check(glGetError() == 0, "MaxShaderCompilerThreadsARB(0) lỗi");

    // ===== EXT_discard_framebuffer: validate + no-op =====
    {
        GLenum atts[2] = {0x8CE0, 0x8D00}; // COLOR0 + DEPTH
        glDiscardFramebuffer(0x8D40, 2, atts); // FRAMEBUFFER
        Check(glGetError() == 0, "DiscardFramebuffer hợp lệ lỗi");
        glDiscardFramebuffer(0x1234, 2, atts); // target lạ
        Check(glGetError() == 0x0500, "DiscardFramebuffer target lạ phải INVALID_ENUM");
        glDiscardFramebuffer(0x8D40, 1, nullptr); // null ptr với count>0
        Check(glGetError() == 0x0501, "DiscardFramebuffer null ptr phải INVALID_VALUE");
        GLenum bad[1] = {0x9999};
        glDiscardFramebuffer(0x8D40, 1, bad);
        Check(glGetError() == 0x0500, "DiscardFramebuffer attachment lạ phải INVALID_ENUM");
    }

    // ===== EXT_debug_marker: stack + event =====
    {
        glPushGroupMarker(0, "outer");
        glPushGroupMarker(-1, "inner"); // -1 = null-terminated
        Check(glGetError() == 0, "PushGroupMarker lỗi");
        glPopGroupMarker();
        glPopGroupMarker();
        Check(glGetError() == 0, "PopGroupMarker lỗi");
        glPopGroupMarker(); // stack rỗng
        Check(glGetError() == 0x0505, "PopGroupMarker rỗng phải STACK_UNDERFLOW");
        glInsertEventMarker(-1, "test-event");
        Check(glGetError() == 0, "InsertEventMarker lỗi");
        // Event marker phải vào KHR_debug log (type MARKER).
        GLenum src = 0, type = 0, sev = 0; GLuint id = 0; GLsizei len = 0;
        char buf[256] = {0};
        GLuint n = glGetDebugMessageLog(1, sizeof(buf), &src, &type, &id, &sev, &len, buf);
        Check(n == 1 && type == 0x8251, "EventMarker phải vào debug log (MARKER)");
    }

    // ===== KHR_debug: labels round-trip =====
    {
        glObjectLabel(0x82E1 /*TEXTURE*/, 5, -1, "my-tex");
        Check(glGetError() == 0, "ObjectLabel lỗi");
        char buf[64] = {0}; GLsizei len = 0;
        glGetObjectLabel(0x82E1, 5, sizeof(buf), &len, buf);
        Check(len == 6 && strcmp(buf, "my-tex") == 0, "GetObjectLabel round-trip sai");
        // Xoá label (empty string).
        glObjectLabel(0x82E1, 5, 0, "");
        glGetObjectLabel(0x82E1, 5, sizeof(buf), &len, buf);
        Check(len == 0, "ObjectLabel xoá sai");
        // Ptr label.
        int dummy = 0;
        glObjectPtrLabel(&dummy, -1, "ptr-label");
        glGetObjectPtrLabel(&dummy, sizeof(buf), &len, buf);
        Check(strcmp(buf, "ptr-label") == 0, "GetObjectPtrLabel round-trip sai");
    }

    // ===== KHR_debug: callback + insert + log =====
    {
        gCbCount = 0;
        glDebugMessageCallback(DebugCb, nullptr);
        Check(glGetError() == 0, "DebugMessageCallback lỗi");
        // GetPointerv trả callback.
        void* p = nullptr;
        glGetPointerv(0x8244 /*DEBUG_CALLBACK_FUNCTION*/, &p);
        Check(p == (void*)DebugCb, "GetPointerv CALLBACK_FUNCTION sai");
        // Insert → callback phải chạy.
        glDebugMessageInsert(0x824B /*APPLICATION*/, 0x824C /*ERROR*/, 42, 0x9146 /*HIGH*/,
                             -1, "test-msg");
        Check(gCbCount == 1, "Callback không chạy sau Insert");
        Check(gCbSource == 0x824B && gCbId == 42 && gCbMsg == "test-msg", "Callback params sai");
        // Log phải có message (insert cũng ghi log).
        // (Đã đọc 1 cái ở marker test, giờ log có insert message.)
        // Control: disable APPLICATION/ERROR/HIGH → insert không gọi callback.
        gCbCount = 0;
        glDebugMessageControl(0x824B, 0x824C, 0x9146, 0, nullptr, 0); // disable
        glDebugMessageInsert(0x824B, 0x824C, 43, 0x9146, -1, "blocked-msg");
        Check(gCbCount == 0, "Control disable không chặn callback");
        // Enable lại.
        glDebugMessageControl(0x824B, 0x824C, 0x9146, 0, nullptr, 1);
        glDebugMessageInsert(0x824B, 0x824C, 44, 0x9146, -1, "allowed-msg");
        Check(gCbCount == 1, "Control enable không mở callback");
        // Source lạ → INVALID_ENUM.
        glDebugMessageInsert(0x9999, 0x824C, 1, 0x9146, -1, "bad");
        Check(glGetError() == 0x0500, "Insert source lạ phải INVALID_ENUM");
    }

    // ===== KHR_debug: push/pop group =====
    {
        glPushDebugGroup(0x824B, 1, -1, "grp1");
        glPushDebugGroup(0x824B, 2, -1, "grp2");
        Check(glGetError() == 0, "PushDebugGroup lỗi");
        glPopDebugGroup();
        glPopDebugGroup();
        Check(glGetError() == 0, "PopDebugGroup lỗi");
        glPopDebugGroup(); // rỗng (đã pop hết marker + group trước đó? marker stack riêng)
        // Lưu ý: debugGroupStack dùng chung với marker EXT. Đã pop hết ở trên.
        // Giờ stack có 0 phần tử (2 push + 2 pop) → pop nữa lỗi.
        Check(glGetError() == 0x0505, "PopDebugGroup rỗng phải STACK_UNDERFLOW");
    }

    if (gFails == 0) { printf("test_debug_p4: all OK\n"); return 0; }
    printf("test_debug_p4: %d FAIL\n", gFails);
    return 1;
}
