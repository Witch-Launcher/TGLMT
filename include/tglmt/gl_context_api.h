#pragma once
// TGLMT — public C ABI cho tạo / quản lý GL context.
//
// TGLMT hỗ trợ TOÀN BỘ Desktop OpenGL 1.0–4.6, Core Profile lẫn Compatibility
// Profile. Một "context" ở đây = một bộ state OpenGL + device Metal, tương
// đương GLX/WGL context. App chọn version/profile lúc tạo; TGLMT sau đó gate
// mọi entry point theo (version, profile) đó — gọi entry point không có trong
// context sẽ sinh GL_INVALID_OPERATION (GL 4.6 Core spec §2.3).
//
// Quy tắc tạo context (xem Context::SetVersionProfile):
//   - major/minor trong 1.0..4.6.
//   - Core / Compatibility Profile chỉ hợp lệ từ 3.2 trở lên.
//   - < 3.2 không có khái niệm profile ⇒ TGLMT_PROFILE_LEGACY.
//   - Compatibility Profile không được FORWARD_COMPATIBLE.
//   - NO_ERROR chỉ hợp lệ với DEBUG context.
//
// ABI: thuần C, không exception/RTTI/STL đi qua ranh giới.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TGLMTContext TGLMTContext;

typedef enum TGLMTProfile {
    TGLMT_PROFILE_LEGACY = 0,        // GL 1.0–3.1 (một bộ API duy nhất)
    TGLMT_PROFILE_CORE = 1,          // GL >= 3.2 Core Profile
    TGLMT_PROFILE_COMPATIBILITY = 2, // GL >= 3.2 Compatibility Profile
    // Tự chọn: Core nếu version >= 3.2, Legacy nếu version < 3.2.
    // Đây là giá trị mặc định của TGLMTContextAttribs.
    TGLMT_PROFILE_AUTO = 3,
} TGLMTProfile;

#define TGLMT_CONTEXT_FLAG_FORWARD_COMPATIBLE 0x00000001u
#define TGLMT_CONTEXT_FLAG_DEBUG              0x00000002u
#define TGLMT_CONTEXT_FLAG_ROBUST_ACCESS      0x00000004u
#define TGLMT_CONTEXT_FLAG_NO_ERROR           0x00000008u

typedef struct TGLMTContextAttribs {
    int majorVersion;   // 1..4 (mặc định 4)
    int minorVersion;   // 0..6 (mặc định 6)
    int profile;        // TGLMTProfile (mặc định TGLMT_PROFILE_CORE)
    uint32_t flags;     // TGLMT_CONTEXT_FLAG_* (mặc định 0)
    // Backend Metal: "apple" (thiết bị thật) | "null" (headless, để test).
    // NULL ⇒ "null" nếu không dựng được device Apple.
    const char* backend;
} TGLMTContextAttribs;

// Tạo context. Trả NULL nếu tham số không hợp lệ (version ngoài 1.0..4.6,
// profile không hợp lệ với version, flag mâu thuẫn).
TGLMTContext* tglmtCreateContextAttribs(const TGLMTContextAttribs* attribs);

// Hủy context. Mọi resource của context bị giải phóng.
void tglmtDestroyContext(TGLMTContext* ctx);

// Ràng context hiện tại của thread này (thread_local).
void tglmtMakeCurrent(TGLMTContext* ctx);
TGLMTContext* tglmtGetCurrentContext(void);

// Đọc lại thuộc tính context. Trả 0 nếu ctx null.
int tglmtGetContextAttribs(TGLMTContext* ctx, TGLMTContextAttribs* out);

// getProcAddress: trả con trỏ hàm C ABI cho tên "glXxx" nếu entry point đó
// hợp lệ trong context hiện tại (version + profile). Trả NULL nếu không.
void* tglmtGetProcAddress(const char* name);

// Chuỗi version đúng của context hiện tại (tương đương glGetString(GL_VERSION)).
const char* tglmtGetVersionString(void);

#ifdef __cplusplus
}  // extern "C"
#endif
