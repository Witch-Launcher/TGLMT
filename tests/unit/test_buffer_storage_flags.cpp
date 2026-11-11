// test_buffer_storage_flags.cpp — GL_ARB_buffer_storage (GL 4.4 core §6.2).
//
// Regression: Minecraft 26.2 crash lúc boot ("OpenGL error 1281" =
// GL_INVALID_VALUE) vì TGLMT từ chối flags hợp lệ.
//
// MC dùng GlConst.bufferUsageToGlFlag() để map usage → flags:
//   usage & USAGE_MAP_READ (1)          → 0x41 = MAP_READ_BIT|MAP_PERSISTENT_BIT
//   usage & USAGE_MAP_WRITE (2)         → 0x42 = MAP_WRITE_BIT|MAP_PERSISTENT_BIT
//   usage & USAGE_COPY_DST (8)          → 0x100 = DYNAMIC_STORAGE_BIT
//   usage & USAGE_HINT_CLIENT_STORAGE(4)→ 0x200 = CLIENT_STORAGE_BIT
// Cả 6 bit đều hợp lệ theo spec (ARB_buffer_storage.txt "New Tokens").
// Thiếu 0x100/0x200 trong mask → MC boot fail.
//
// Test ở đây dùng ĐÚNG các giá trị MC truyền vào.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_buffer_storage_flags FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // --- Các flags MC thực sự truyền (mọi tổ hợp đều phải hợp lệ) ---
    const GLbitfield mcFlags[] = {
        0x0100,                                     // COPY_DST → DYNAMIC_STORAGE_BIT
        0x0200,                                     // HINT_CLIENT_STORAGE → CLIENT_STORAGE_BIT
        0x0100 | 0x0200,                            // cả hai
        0x0041,                                     // MAP_READ|MAP_PERSISTENT
        0x0042,                                     // MAP_WRITE|MAP_PERSISTENT
        0x0100 | 0x0042,                            // DYNAMIC_STORAGE + MAP_WRITE|PERSISTENT
        0x0003,                                     // MAP_READ|MAP_WRITE
        0x0043,                                     // MAP_READ|MAP_WRITE|PERSISTENT
        0x0000,                                     // không cờ nào (hợp lệ)
    };
    for (size_t i = 0; i < sizeof(mcFlags) / sizeof(mcFlags[0]); ++i) {
        GLuint b = 0;
        glGenBuffers(1, &b);
        glBindBuffer(0x8A11 /*GL_UNIFORM_BUFFER*/, b);
        glBufferStorage(0x8A11, 64, nullptr, mcFlags[i]);
        GLenum e = glGetError();
        Check(e == GL_NO_ERROR, "glBufferStorage flags hợp lệ không lỗi");
        // Buffer phải tồn tại và đúng size.
        GLint sz = 0;
        glGetBufferParameteriv(0x8A11, 0x8764 /*BUFFER_SIZE*/, &sz);
        Check(sz == 64, "BUFFER_SIZE sau BufferStorage");
        // BUFFER_STORAGE_FLAGS phải trả đúng flags đã đặt.
        GLint sf = -1;
        glGetBufferParameteriv(0x8A11, 0x8220 /*BUFFER_STORAGE_FLAGS*/, &sf);
        Check(sf == (GLint)mcFlags[i], "BUFFER_STORAGE_FLAGS trả đúng");
        glBindBuffer(0x8A11, 0);
        glDeleteBuffers(1, &b);
    }

    // --- Ghi/đọc trên buffer immutable (đường MC dùng: BufferStorage rồi
    //     BufferSubData mỗi frame) ---
    {
        GLuint b = 0;
        glGenBuffers(1, &b);
        glBindBuffer(0x8A11, b);
        glBufferStorage(0x8A11, 128, nullptr, 0x0100 /*DYNAMIC_STORAGE_BIT*/);
        Check(glGetError() == GL_NO_ERROR, "BufferStorage immutable cho UBO");
        float v[16];
        for (int i = 0; i < 16; ++i) v[i] = (float)i;
        glBufferSubData(0x8A11, 16, sizeof(v), v);
        Check(glGetError() == GL_NO_ERROR, "BufferSubData trên buffer immutable");
        float out[16] = {0};
        glGetBufferSubData(0x8A11, 16, sizeof(out), out);
        Check(glGetError() == GL_NO_ERROR, "GetBufferSubData trên buffer immutable");
        bool same = true;
        for (int i = 0; i < 16; ++i) if (out[i] != (float)i) same = false;
        Check(same, "nội dung buffer immutable đọc lại đúng");
        glBindBuffer(0x8A11, 0);
        glDeleteBuffers(1, &b);
    }

    // --- Flags thực sự sai theo spec vẫn phải báo lỗi ---
    {
        GLuint b = 0;
        glGenBuffers(1, &b);
        glBindBuffer(0x8A11, b);
        glBufferStorage(0x8A11, 64, nullptr, 0x0400 /*bit không hợp lệ*/);
        Check(glGetError() == GL_INVALID_VALUE, "bit lạ → INVALID_VALUE");
        glBufferStorage(0x8A11, 64, nullptr, 0x0040 /*PERSISTENT mà thiếu READ|WRITE*/);
        Check(glGetError() == GL_INVALID_VALUE, "PERSISTENT thiếu READ|WRITE → INVALID_VALUE");
        glBufferStorage(0x8A11, 64, nullptr, 0x0080 /*COHERENT mà thiếu PERSISTENT*/);
        Check(glGetError() == GL_INVALID_VALUE, "COHERENT thiếu PERSISTENT → INVALID_VALUE");
        glBufferStorage(0x8A11, 0, nullptr, 0x0100);
        Check(glGetError() == GL_INVALID_VALUE, "size <= 0 → INVALID_VALUE");
        glBindBuffer(0x8A11, 0);
        glDeleteBuffers(1, &b);
    }

    if (gFails == 0) { printf("test_buffer_storage_flags: OK\n"); return 0; }
    return 1;
}
