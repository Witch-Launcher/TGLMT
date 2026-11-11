// test_mc_boot_path.cpp — mô phỏng đúng chuỗi lệnh MC 26.2 gọi lúc boot.
//
// Crash thật (2026-11-10): "OpenGL error 1281" tại GlDevice.createBuffer.
// Nguyên nhân: TGLMT từ chối flags hợp lệ của glBufferStorage.
//
// MC 26.2 (không có ARB_direct_state_access → dùng DirectStateAccess$Emulated):
//   GlConst.bufferUsageToGlFlag(usage) → flags
//   glBindBuffer(target, id)
//   glBufferStorage(target, size, flags)
//   glBindBuffer(target, 0)
// rồi (nếu usage có MAP_*): glBindBuffer → glMapBufferRange → glBindBuffer(0).
//
// Test này gọi đúng chuỗi đó với các giá trị MC thực sự dùng, rồi kiểm tra
// buffer đọc lại được (điều MC cần để không phải assert lỗi GL).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_mc_boot_path FAIL: %s\n", msg); ++gFails; }
}

// Gần đúng GlConst.bufferUsageToGlFlag của MC (javap xác nhận):
//   usage&1 (USAGE_MAP_READ)  → 0x41 = MAP_READ_BIT|MAP_PERSISTENT_BIT
//   usage&2 (USAGE_MAP_WRITE) → 0x42 = MAP_WRITE_BIT|MAP_PERSISTENT_BIT
//   usage&8 (USAGE_COPY_DST)  → 0x100 = DYNAMIC_STORAGE_BIT
//   usage&4 (USAGE_HINT_CLIENT_STORAGE) → 0x200 = CLIENT_STORAGE_BIT
static GLbitfield McUsageToGlFlag(int usage) {
    GLbitfield f = 0;
    if (usage & 1) f |= 0x0041;
    if (usage & 2) f |= 0x0042;
    if (usage & 8) f |= 0x0100;
    if (usage & 4) f |= 0x0200;
    return f;
}

// Usage của MC: MAP_READ=1 MAP_WRITE=2 HINT_CLIENT_STORAGE=4 COPY_DST=8
// COPY_SRC=16 VERTEX=32 INDEX=64 UNIFORM=128 UNIFORM_TEXEL_BUFFER=256
static const int kMcUsageUniformCopyDst = 128 | 8;   // ProjectionMatrixBuffer
static const int kMcUsageVertex        = 32;
static const int kMcUsageMapReadWrite  = 1 | 2;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // --- 1. ProjectionMatrixBuffer: UNIFORM | COPY_DST, size 64 ---
    {
        GLuint id = 0;
        glGenBuffers(1, &id);
        glBindBuffer(0x8A11 /*GL_UNIFORM_BUFFER*/, id);
        glBufferStorage(0x8A11, 64, nullptr, McUsageToGlFlag(kMcUsageUniformCopyDst));
        Check(glGetError() == GL_NO_ERROR, "MC: UNIFORM|COPY_DST buffer tạo được");
        glBindBuffer(0x8A11, 0);
        Check(glGetError() == GL_NO_ERROR, "MC: unbind sau BufferStorage không lỗi");

        // Buffer phải dùng được: ghi rồi đọc lại.
        float v[16];
        for (int i = 0; i < 16; ++i) v[i] = (float)i * 0.5f;
        glBindBuffer(0x8A11, id);
        glBufferSubData(0x8A11, 0, sizeof(v), v);
        Check(glGetError() == GL_NO_ERROR, "MC: ghi vào buffer immutable không lỗi");
        float out[16] = {0};
        glGetBufferSubData(0x8A11, 0, sizeof(out), out);
        Check(glGetError() == GL_NO_ERROR, "MC: đọc buffer không lỗi");
        bool same = true;
        for (int i = 0; i < 16; ++i) if (out[i] != v[i]) same = false;
        Check(same, "MC: nội dung buffer khớp");
        glBindBuffer(0x8A11, 0);
        glDeleteBuffers(1, &id);
    }

    // --- 2. Buffer có MAP_READ|MAP_WRITE: MC map persistent rồi dùng ---
    {
        GLuint id = 0;
        glGenBuffers(1, &id);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, id);
        glBufferStorage(0x8892, 256, nullptr, McUsageToGlFlag(kMcUsageMapReadWrite));
        Check(glGetError() == GL_NO_ERROR, "MC: buffer MAP_READ|MAP_WRITE tạo được");
        // tryMapBufferPersistent: access = MAP_READ|MAP_WRITE|MAP_PERSISTENT
        // (MC dựng từ usage; MC có thể thêm bit lạ → TGLMT không được chặn).
        void* p = glMapBufferRange(0x8892, 0, 256, 0x0001 | 0x0002 | 0x0040);
        Check(glGetError() == GL_NO_ERROR, "MC: mapBufferRange persistent không lỗi");
        Check(p != nullptr, "MC: mapBufferRange trả con trỏ");
        if (p) {
            float* f = (float*)p;
            for (int i = 0; i < 16; ++i) f[i] = (float)i;
        }
        glUnmapBuffer(0x8892);
        Check(glGetError() == GL_NO_ERROR, "MC: unmapBuffer không lỗi");
        glBindBuffer(0x8892, 0);
        glDeleteBuffers(1, &id);
    }

    // --- 3. Buffer VERTEX (usage 32 → flags 0) ---
    {
        GLuint id = 0;
        glGenBuffers(1, &id);
        glBindBuffer(0x8892, id);
        glBufferStorage(0x8892, 1024, nullptr, McUsageToGlFlag(kMcUsageVertex));
        Check(glGetError() == GL_NO_ERROR, "MC: buffer VERTEX tạo được");
        glBindBuffer(0x8892, 0);
        glDeleteBuffers(1, &id);
    }

    // --- 4. Lỗi GL dính (sticky error) phải KHÔNG còn sau chuỗi trên ---
    // MC abort khi _getError() != 0 ở 5 chỗ → mọi lệnh phải sạch lỗi.
    Check(glGetError() == GL_NO_ERROR, "MC: không còn lỗi GL dính sau chuỗi boot");

    if (gFails == 0) { printf("test_mc_boot_path: OK\n"); return 0; }
    return 1;
}
