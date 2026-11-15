// test_context_version.cpp — T0: context versioned + Core/Compatibility profile.
//
// Kiểm chứng (mọi khẳng định đều từ spec, không suy đoán):
//  1. Tạo được context cho MỌI version 1.0..4.6 và profile hợp lệ.
//  2. glGetString(GL_VERSION) phản ánh version THẬT của context.
//  3. GL_MAJOR_VERSION / GL_MINOR_VERSION / GL_CONTEXT_FLAGS /
//     GL_CONTEXT_PROFILE_MASK phản ánh context (spec Table 22.1).
//  4. Pname chỉ tồn tại từ version nào đó (GL_MAJOR_VERSION từ 3.0,
//     GL_CONTEXT_PROFILE_MASK từ 3.2) ⇒ ở context cũ phải sinh
//     GL_INVALID_ENUM (spec §22.1).
//  5. Entry point không tồn tại trong (version, profile) ⇒ GL_INVALID_OPERATION
//     (spec §2.3), và KHÔNG được thực thi.
//  6. getProcAddress trả NULL cho entry point không có trong context.
//
// Backend: "null" (headless) — test này kiểm tra state machine, không cần GPU.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/VersionGate.h"
#include "test_util.h"
#include <cstdio>
#include <cstring>
#include <string>

// Test gọi PUBLIC C ABI (include/tglmt/gl_c_api.h) — đó mới là lớp mà app
// dùng, và cũng là lớp có version-gate. (Lớp C++ nội bộ tglmt::gl:: không
// gate để các test cũ trong repo không bị đổi hành vi.)
using namespace tglmt;

namespace {

struct VerSlot { int major; int minor; };

// Toàn bộ version Desktop OpenGL trong phạm vi (không bỏ minor nào).
const VerSlot kAllVersions[] = {
    {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5},
    {2, 0}, {2, 1},
    {3, 0}, {3, 1},
    {3, 2}, {3, 3},
    {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6},
};

// Profile hợp lệ cho từng version:
//  - < 3.2: registry không có khái niệm profile ⇒ chỉ Legacy.
//  - >= 3.2: Core và Compatibility đều hợp lệ (Legacy ở >= 3.2 là mâu thuẫn).
bool ProfileValid(int major, int minor, TGLMTProfile p) {
    if (major < 3 || (major == 3 && minor < 2)) return p == TGLMT_PROFILE_LEGACY;
    return p == TGLMT_PROFILE_CORE || p == TGLMT_PROFILE_COMPATIBILITY;
}

TGLMTContext* MakeCtx(int major, int minor, TGLMTProfile p, uint32_t flags = 0) {
    TGLMTContextAttribs a{};
    a.majorVersion = major;
    a.minorVersion = minor;
    a.profile = (int)p;
    a.flags = flags;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

}  // namespace

int main() {
    // ---- 1. Tạo context cho mọi version × profile hợp lệ ------------------
    int created = 0;
    for (const VerSlot& v : kAllVersions) {
        const TGLMTProfile profiles[] = {TGLMT_PROFILE_LEGACY, TGLMT_PROFILE_CORE,
                                         TGLMT_PROFILE_COMPATIBILITY};
        for (TGLMTProfile p : profiles) {
            TGLMTContext* c = MakeCtx(v.major, v.minor, p);
            if (!ProfileValid(v.major, v.minor, p)) {
                TGLMT_CHECK(c == nullptr,
                            "profile không hợp lệ với version ⇒ phải từ chối tạo");
                continue;
            }
            TGLMT_CHECK(c != nullptr, "tạo context được cho version/profile hợp lệ");
            if (!c) continue;
            ++created;
            tglmtMakeCurrent(c);

            // ---- 2. GL_VERSION phản ánh context ----------------------------
            char want[32];
            snprintf(want, sizeof(want), "%d.%d", v.major, v.minor);
            const char* ver = (const char*)glGetString(0x1F02 /*GL_VERSION*/);
            TGLMT_CHECK(ver && strncmp(ver, want, strlen(want)) == 0,
                        "glGetString(GL_VERSION) phải bắt đầu bằng <major>.<minor>");
            TGLMT_CHECK(glGetError() == 0, "glGetString không sinh lỗi");

            // ---- 3. pname version/profile ---------------------------------
            GLint maj = -1, min = -1, mask = -1, flags = -1;
            if (v.major >= 3) {
                glGetIntegerv(0x821B /*MAJOR_VERSION*/, &maj);
                glGetIntegerv(0x821C /*MINOR_VERSION*/, &min);
                glGetIntegerv(0x821E /*CONTEXT_FLAGS*/, &flags);
                TGLMT_CHECK(glGetError() == 0, "pname 3.0+ hợp lệ ⇒ không lỗi");
                TGLMT_CHECK(maj == v.major, "GL_MAJOR_VERSION khớp context");
                TGLMT_CHECK(min == v.minor, "GL_MINOR_VERSION khớp context");
                TGLMT_CHECK(flags == 0, "CONTEXT_FLAGS mặc định = 0");
            } else {
                // GL < 3.0: MAJOR_VERSION / MINOR_VERSION / CONTEXT_FLAGS không
                // tồn tại ⇒ phải GL_INVALID_ENUM.
                glGetIntegerv(0x821B, &maj);
                TGLMT_CHECK(glGetError() == 0x0500,
                            "pname của GL3.0 trong context 1.x ⇒ INVALID_ENUM");
                while (glGetError() != 0) {}
            }
            if (v.major >= 3 && (v.major > 3 || v.minor >= 2)) {
                glGetIntegerv(0x9126 /*CONTEXT_PROFILE_MASK*/, &mask);
                TGLMT_CHECK(glGetError() == 0, "CONTEXT_PROFILE_MASK hợp lệ ở 3.2+");
                GLint want_mask = (p == TGLMT_PROFILE_CORE) ? 0x1 : 0x2;
                TGLMT_CHECK(mask == want_mask,
                            "CONTEXT_PROFILE_MASK đúng bit core/compatibility");
            } else if (v.major >= 3 && v.minor < 2) {
                glGetIntegerv(0x9126, &mask);
                TGLMT_CHECK(glGetError() == 0x0500,
                            "CONTEXT_PROFILE_MASK ở GL3.0/3.1 ⇒ INVALID_ENUM");
                while (glGetError() != 0) {}
            }

            // ---- 4. SHADING_LANGUAGE_VERSION ------------------------------
            const char* sl = (const char*)glGetString(0x8B8C /*SHADING_LANGUAGE_VERSION*/);
            if (v.major < 2) {
                TGLMT_CHECK(glGetError() == 0x0500,
                            "SHADING_LANGUAGE_VERSION ở GL1.x ⇒ INVALID_ENUM");
                while (glGetError() != 0) {}
            } else {
                TGLMT_CHECK(sl && sl[0] != '\0', "SHADING_LANGUAGE_VERSION khác rỗng");
            }

            tglmtMakeCurrent(nullptr);
            tglmtDestroyContext(c);
        }
    }
    TGLMT_CHECK(created == 10 + 9 * 2,
                "số context tạo được = 10 legacy + 9 version × 2 profile");

    // ---- 5. Version-gate: entry point không có trong context ⇒ INVALID_OPERATION
    {
        // GL 3.3 Core: glBufferStorage (GL 4.4) không được phép.
        TGLMTContext* c = MakeCtx(3, 3, TGLMT_PROFILE_CORE);
        TGLMT_CHECK(c != nullptr, "tạo context GL 3.3 Core");
        tglmtMakeCurrent(c);
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        TGLMT_CHECK(glGetError() == 0, "glGenBuffers hợp lệ ở GL 3.3");
        glBufferStorage(0x8892, 16, nullptr, 0x88E4);
        TGLMT_CHECK(glGetError() == 0x0502,
                    "glBufferStorage ở GL 3.3 Core ⇒ INVALID_OPERATION");
        // glGetPointerv bị bỏ core ở 3.2 nhưng được yêu cầu lại ở 4.3 ⇒ ở 3.3
        // Core vẫn phải bị chặn.
        void* ptr = nullptr;
        glGetPointerv(0x0D20, &ptr);
        TGLMT_CHECK(glGetError() == 0x0502,
                    "glGetPointerv ở GL 3.3 Core ⇒ INVALID_OPERATION");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        // Cùng lệnh trong GL 4.6 Core ⇒ hợp lệ.
        TGLMTContext* c = MakeCtx(4, 6, TGLMT_PROFILE_CORE);
        tglmtMakeCurrent(c);
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        glBindBuffer(0x8892, buf);
        unsigned char data[16] = {0};
        // flags của glBufferStorage là MAP_*/DYNAMIC_*, KHÔNG phải usage.
        glBufferStorage(0x8892, sizeof(data), data, 0);
        TGLMT_CHECK(glGetError() == 0, "glBufferStorage ở GL 4.6 Core ⇒ không lỗi");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        // GL 1.0 Legacy: glGenBuffers (GL 1.5) không tồn tại ⇒ INVALID_OPERATION.
        TGLMTContext* c = MakeCtx(1, 0, TGLMT_PROFILE_LEGACY);
        tglmtMakeCurrent(c);
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        TGLMT_CHECK(glGetError() == 0x0502,
                    "glGenBuffers ở GL 1.0 ⇒ INVALID_OPERATION");
        // glClear (GL 1.0) thì hợp lệ.
        glClear(0x4000 /*COLOR_BUFFER_BIT*/);
        TGLMT_CHECK(glGetError() == 0, "glClear ở GL 1.0 ⇒ không lỗi");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        // GL 4.6 Core: glBegin (legacy, chỉ compat) ⇒ INVALID_OPERATION.
        TGLMTContext* c = MakeCtx(4, 6, TGLMT_PROFILE_CORE);
        tglmtMakeCurrent(c);
        glBegin(0x0004 /*GL_TRIANGLES*/);
        TGLMT_CHECK(glGetError() == 0x0502,
                    "glBegin ở GL 4.6 Core ⇒ INVALID_OPERATION");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        // GL 4.6 Compatibility: glBegin hợp lệ (không gate chặn).
        TGLMTContext* c = MakeCtx(4, 6, TGLMT_PROFILE_COMPATIBILITY);
        tglmtMakeCurrent(c);
        glBegin(0x0004);
        // Chưa implement draw path ⇒ stub ghi UNIMPLEMENTED nhưng KHÔNG gate.
        TGLMT_CHECK(glGetError() != 0x0502,
                    "glBegin ở GL 4.6 Compatibility ⇒ không bị version-gate chặn");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }

    // ---- 6. getProcAddress theo (version, profile) -------------------------
    {
        TGLMTContext* c = MakeCtx(4, 6, TGLMT_PROFILE_CORE);
        tglmtMakeCurrent(c);
        TGLMT_CHECK(tglmtGetProcAddress("glClear") != nullptr,
                    "getProcAddress(glClear) ở 4.6 Core ⇒ khác null");
        TGLMT_CHECK(tglmtGetProcAddress("glBegin") == nullptr,
                    "getProcAddress(glBegin) ở 4.6 Core ⇒ null");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        TGLMTContext* c = MakeCtx(4, 6, TGLMT_PROFILE_COMPATIBILITY);
        tglmtMakeCurrent(c);
        TGLMT_CHECK(tglmtGetProcAddress("glBegin") != nullptr,
                    "getProcAddress(glBegin) ở 4.6 Compatibility ⇒ khác null");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }
    {
        TGLMTContext* c = MakeCtx(2, 0, TGLMT_PROFILE_LEGACY);
        tglmtMakeCurrent(c);
        TGLMT_CHECK(tglmtGetProcAddress("glBegin") != nullptr,
                    "getProcAddress(glBegin) ở GL 2.0 Legacy ⇒ khác null");
        TGLMT_CHECK(tglmtGetProcAddress("glBufferStorage") == nullptr,
                    "getProcAddress(glBufferStorage) ở GL 2.0 ⇒ null");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(c);
    }

    // ---- 7. Attribs không hợp lệ bị từ chối --------------------------------
    {
        TGLMTContext* c = MakeCtx(5, 0, TGLMT_PROFILE_CORE);
        TGLMT_CHECK(c == nullptr, "version 5.0 ngoài phạm vi ⇒ từ chối");
        c = MakeCtx(4, 6, TGLMT_PROFILE_COMPATIBILITY,
                    TGLMT_CONTEXT_FLAG_FORWARD_COMPATIBLE);
        TGLMT_CHECK(c == nullptr,
                    "Compatibility + forward-compatible ⇒ mâu thuẫn ⇒ từ chối");
        c = MakeCtx(4, 6, TGLMT_PROFILE_CORE, TGLMT_CONTEXT_FLAG_NO_ERROR);
        TGLMT_CHECK(c == nullptr, "NO_ERROR mà không DEBUG ⇒ từ chối");
        c = MakeCtx(4, 6, TGLMT_PROFILE_CORE,
                    TGLMT_CONTEXT_FLAG_DEBUG | TGLMT_CONTEXT_FLAG_NO_ERROR);
        TGLMT_CHECK(c != nullptr, "DEBUG + NO_ERROR ⇒ hợp lệ");
        tglmtDestroyContext(c);
    }

    // ---- 8. Bảng gate tự nhất quán với registry ----------------------------
    {
        // GL 4.6 Core: đúng 657 lệnh; GL 4.6 Compat: 1048.
        int core46 = 0, compat46 = 0;
        for (int i = 0; i < kVersionGateCount; ++i) {
            const uint64_t m = kVersionGateTable[i].avail;
            const bool core = (m >> 18) & 1ULL;
            const bool compat = (m >> (18 + kGlCompatBitBase)) & 1ULL;
            if (core) ++core46;
            if (compat) ++compat46;
        }
        TGLMT_CHECK(core46 == 657, "GL 4.6 Core có đúng 657 lệnh trong bảng gate");
        TGLMT_CHECK(compat46 == 1048, "GL 4.6 Compat có đúng 1048 lệnh trong bảng gate");
    }

    printf("[test_context_version] OK (%d lỗi)\n", tglmt_test::Failures());
    return TGLMT_RESULT;
}
