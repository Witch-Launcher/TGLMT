// Test nhóm Buffer của GL 4.6 Core — 8 entry point:
//   glBindBuffersRange, glCopyNamedBufferSubData,
//   glFlushMappedBufferRange, glFlushMappedNamedBufferRange,
//   glInvalidateBufferData, glInvalidateBufferSubData,
//   glMapNamedBuffer, glNamedBufferStorage.
//
// Kiểm chứng theo GL 4.6 §6.1–§6.5:
//   • glNamedBufferStorage: tạo store bất biến; gọi lại → INVALID_OPERATION;
//     size<=0 → INVALID_VALUE; flags sai → INVALID_VALUE.
//   • glMapNamedBuffer: access sai → INVALID_ENUM; buffer đã map → INVALID_OPERATION;
//     buffer không tồn tại → INVALID_OPERATION.
//   • glFlushMapped*BufferRange: buffer không map → INVALID_OPERATION; không có
//     MAP_FLUSH_EXPLICIT_BIT → INVALID_OPERATION; offset/length âm → INVALID_VALUE;
//     offset+length > kích thước mapping → INVALID_VALUE.
//   • glInvalidateBuffer*: buffer 0/không tồn tại → INVALID_VALUE; offset/length
//     âm → INVALID_VALUE; buffer đang map (không persistent) → INVALID_OPERATION.
//   • glCopyNamedBufferSubData: copy đúng dữ liệu; vượt cuối buffer → INVALID_VALUE.
//   • glBindBuffersRange: count âm → INVALID_VALUE; buffers==NULL → reset về
//     unbound với offset/size MẶC ĐỊNH; buffer không tồn tại → INVALID_OPERATION.

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }

GLuint MakeBuffer(size_t bytes, const void* data = nullptr) {
    GLuint b = 0;
    glCreateBuffers(1, &b);
    glNamedBufferStorage(b, (GLsizeiptr)bytes, data, 0);
    return b;
}

// ===========================================================================
// glNamedBufferStorage
// ===========================================================================
bool TestNamedBufferStorage() {
    GLuint b = 0;
    glCreateBuffers(1, &b);
    // Dữ liệu khởi tạo phải đủ 16 byte (size của store) — trước đây truyền
    // mảng 4 byte cho store 16 byte ⇒ đọc tràn stack (ASan).
    const GLubyte init[16] = {1, 2, 3, 4};
    DrainErrors();
    glNamedBufferStorage(b, 16, init, GL_DYNAMIC_STORAGE_BIT);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNamedBufferStorage hợp lệ không lỗi");

    // Store bất biến: gọi lại → INVALID_OPERATION.
    DrainErrors();
    glNamedBufferStorage(b, 32, nullptr, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glNamedBufferStorage lần 2 trên store bất biến → INVALID_OPERATION");
    // BufferData cũng không được re-alloc store bất biến.
    DrainErrors();
    glBindBuffer(GL_ARRAY_BUFFER, b);
    glBufferData(GL_ARRAY_BUFFER, 32, init, GL_STATIC_DRAW);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glBufferData trên store bất biến → INVALID_OPERATION");
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // size <= 0 → INVALID_VALUE.
    GLuint b2 = 0;
    glCreateBuffers(1, &b2);
    DrainErrors();
    glNamedBufferStorage(b2, 0, nullptr, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glNamedBufferStorage size=0 → INVALID_VALUE");
    DrainErrors();
    glNamedBufferStorage(b2, -4, nullptr, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glNamedBufferStorage size<0 → INVALID_VALUE");

    // flags không hợp lệ → INVALID_VALUE.
    DrainErrors();
    glNamedBufferStorage(b2, 16, nullptr, 0xFFFFFFFFu);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glNamedBufferStorage flags sai → INVALID_VALUE");

    // Buffer không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glNamedBufferStorage(999999, 16, nullptr, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glNamedBufferStorage với buffer không tồn tại → INVALID_OPERATION");

    glDeleteBuffers(1, &b);
    glDeleteBuffers(1, &b2);
    return true;
}

// ===========================================================================
// glMapNamedBuffer
// ===========================================================================
bool TestMapNamedBuffer() {
    // §6.3: MAP_READ/MAP_WRITE trong access chỉ hợp lệ nếu buffer có
    // storage flags tương ứng → tạo buffer bằng glNamedBufferStorage với
    // MAP_READ_BIT|MAP_WRITE_BIT.
    GLuint b = 0;
    glCreateBuffers(1, &b);
    glNamedBufferStorage(b, 64, nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    const GLubyte init[4] = {9, 8, 7, 6};
    glNamedBufferSubData(b, 0, 4, init);

    // access sai → INVALID_ENUM.
    DrainErrors();
    void* p = glMapNamedBuffer(b, 0x1234);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glMapNamedBuffer access sai → INVALID_ENUM");
    TGLMT_CHECK(p == nullptr, "glMapNamedBuffer access sai phải trả null");

    // buffer không tồn tại → INVALID_OPERATION.
    DrainErrors();
    p = glMapNamedBuffer(999999, GL_READ_ONLY);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glMapNamedBuffer buffer không tồn tại → INVALID_OPERATION");
    TGLMT_CHECK(p == nullptr, "phải trả null");

    // Map hợp lệ → con trỏ khác null, ghi được, đọc lại đúng.
    DrainErrors();
    p = glMapNamedBuffer(b, GL_READ_WRITE);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMapNamedBuffer hợp lệ không lỗi");
    TGLMT_CHECK(p != nullptr, "glMapNamedBuffer phải trả con trỏ khác null");
    if (p) {
        ((GLubyte*)p)[0] = 0x42;
    }
    glUnmapNamedBuffer(b);

    GLubyte back[4] = {0};
    glGetNamedBufferSubData(b, 0, 4, back);
    TGLMT_CHECK(back[0] == 0x42, "glMapNamedBuffer: ghi qua con trỏ map phải đọc lại được");

    // Map lần hai khi ĐANG map (chưa unmap) → INVALID_OPERATION.
    DrainErrors();
    p = glMapNamedBuffer(b, GL_READ_ONLY);  // map hợp lệ (buffer chưa map)
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "map lại sau unmap phải hợp lệ");
    TGLMT_CHECK(p != nullptr, "map lại sau unmap phải trả con trỏ");
    DrainErrors();
    p = glMapNamedBuffer(b, GL_READ_ONLY);  // map lần 2 khi đang map → lỗi
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glMapNamedBuffer khi buffer đang map → INVALID_OPERATION");
    TGLMT_CHECK(p == nullptr, "phải trả null");
    glUnmapNamedBuffer(b);

    glDeleteBuffers(1, &b);
    return true;
}

// ===========================================================================
// glFlushMappedBufferRange / glFlushMappedNamedBufferRange
// ===========================================================================
bool TestFlushMappedRange() {
    // Buffer KHÔNG map → INVALID_OPERATION.
    GLuint b = MakeBuffer(64);
    DrainErrors();
    glFlushMappedNamedBufferRange(b, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glFlushMappedNamedBufferRange buffer chưa map → INVALID_OPERATION");

    // Buffer không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glFlushMappedNamedBufferRange(999999, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glFlushMappedNamedBufferRange buffer không tồn tại → INVALID_OPERATION");

    // Bản bound: target không hợp lệ → INVALID_ENUM; zero bound → INVALID_OPERATION.
    DrainErrors();
    glFlushMappedBufferRange(0xDEAD, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glFlushMappedBufferRange target sai → INVALID_ENUM");
    DrainErrors();
    glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glFlushMappedBufferRange khi zero bound → INVALID_OPERATION");

    // Buffer map nhưng KHÔNG có MAP_FLUSH_EXPLICIT_BIT → INVALID_OPERATION.
    GLuint b2 = 0;
    glCreateBuffers(1, &b2);
    glNamedBufferStorage(b2, 64, nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    glMapNamedBuffer(b2, GL_READ_WRITE);
    DrainErrors();
    glFlushMappedNamedBufferRange(b2, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glFlushMappedNamedBufferRange không có MAP_FLUSH_EXPLICIT_BIT → INVALID_OPERATION");
    glUnmapNamedBuffer(b2);

    // Có MAP_FLUSH_EXPLICIT_BIT trong MAP ACCESS → hợp lệ; offset/length âm →
    // INVALID_VALUE; offset+length > kích thước mapping → INVALID_VALUE.
    // (MAP_FLUSH_EXPLICIT_BIT là bit của ACCESS, không phải storage flags.)
    GLuint b3 = 0;
    glCreateBuffers(1, &b3);
    glNamedBufferStorage(b3, 64, nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    void* mp = glMapNamedBufferRange(b3, 0, 64, GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
    TGLMT_CHECK(mp != nullptr, "glMapNamedBufferRange với MAP_FLUSH_EXPLICIT phải được map");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMapNamedBufferRange hợp lệ không lỗi");
    DrainErrors();
    glFlushMappedNamedBufferRange(b3, 0, 16);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glFlushMappedNamedBufferRange hợp lệ không lỗi");
    DrainErrors();
    glFlushMappedNamedBufferRange(b3, -1, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glFlushMappedNamedBufferRange offset âm → INVALID_VALUE");
    DrainErrors();
    glFlushMappedNamedBufferRange(b3, 0, -1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glFlushMappedNamedBufferRange length âm → INVALID_VALUE");
    DrainErrors();
    glFlushMappedNamedBufferRange(b3, 60, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glFlushMappedNamedBufferRange vượt kích thước mapping → INVALID_VALUE");
    glUnmapNamedBuffer(b3);

    glDeleteBuffers(1, &b);
    glDeleteBuffers(1, &b2);
    glDeleteBuffers(1, &b3);
    return true;
}

// ===========================================================================
// glInvalidateBufferData / glInvalidateBufferSubData
// ===========================================================================
bool TestInvalidateBuffer() {
    GLuint b = MakeBuffer(64);

    // buffer = 0 → INVALID_VALUE (spec §6.5).
    DrainErrors();
    glInvalidateBufferData(0);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glInvalidateBufferData(0) → INVALID_VALUE");
    DrainErrors();
    glInvalidateBufferSubData(0, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glInvalidateBufferSubData(0) → INVALID_VALUE");

    // buffer không tồn tại → INVALID_VALUE.
    DrainErrors();
    glInvalidateBufferData(999999);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glInvalidateBufferData buffer không tồn tại → INVALID_VALUE");
    DrainErrors();
    glInvalidateBufferSubData(999999, 0, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glInvalidateBufferSubData buffer không tồn tại → INVALID_VALUE");

    // offset/length âm → INVALID_VALUE.
    DrainErrors();
    glInvalidateBufferSubData(b, -1, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glInvalidateBufferSubData offset âm → INVALID_VALUE");
    DrainErrors();
    glInvalidateBufferSubData(b, 0, -1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glInvalidateBufferSubData length âm → INVALID_VALUE");

    // offset+length > BUFFER_SIZE → INVALID_VALUE.
    DrainErrors();
    glInvalidateBufferSubData(b, 60, 16);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glInvalidateBufferSubData vượt BUFFER_SIZE → INVALID_VALUE");

    // Hợp lệ → không lỗi.
    DrainErrors();
    glInvalidateBufferData(b);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInvalidateBufferData hợp lệ không lỗi");
    DrainErrors();
    glInvalidateBufferSubData(b, 0, 16);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInvalidateBufferSubData hợp lệ không lỗi");

    // Buffer đang map (không persistent) → INVALID_OPERATION.
    GLuint b2 = 0;
    glCreateBuffers(1, &b2);
    glNamedBufferStorage(b2, 64, nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
    glMapNamedBuffer(b2, GL_READ_WRITE);
    DrainErrors();
    glInvalidateBufferData(b2);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glInvalidateBufferData khi buffer đang map → INVALID_OPERATION");
    glUnmapNamedBuffer(b2);
    glDeleteBuffers(1, &b2);

    glDeleteBuffers(1, &b);
    return true;
}

// ===========================================================================
// glCopyNamedBufferSubData
// ===========================================================================
bool TestCopyNamedBufferSubData() {
    const GLubyte src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const GLubyte zeros[8] = {0};
    GLuint r = MakeBuffer(8, src);
    GLuint w = MakeBuffer(8, zeros);

    DrainErrors();
    glCopyNamedBufferSubData(r, w, 2, 4, 4);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCopyNamedBufferSubData hợp lệ không lỗi");
    GLubyte out[8] = {0};
    glGetNamedBufferSubData(w, 0, 8, out);
    TGLMT_CHECK(out[0] == 0 && out[1] == 0 && out[4] == 3 && out[7] == 6,
                "glCopyNamedBufferSubData phải copy đúng byte (offset đích 4, dữ liệu 3..6)");

    // Vượt cuối buffer → INVALID_VALUE.
    DrainErrors();
    glCopyNamedBufferSubData(r, w, 0, 4, 8);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glCopyNamedBufferSubData ghi vượt cuối buffer đích → INVALID_VALUE");
    // Buffer không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glCopyNamedBufferSubData(r, 999999, 0, 0, 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glCopyNamedBufferSubData buffer đích không tồn tại → INVALID_OPERATION");

    glDeleteBuffers(1, &r);
    glDeleteBuffers(1, &w);
    return true;
}

// ===========================================================================
// glBindBuffersRange
// ===========================================================================
bool TestBindBuffersRange() {
    GLuint b0 = MakeBuffer(64);
    GLuint b1 = MakeBuffer(64);

    // count âm → INVALID_VALUE.
    DrainErrors();
    const GLuint bufs[1] = {b0};
    const GLintptr offs[1] = {0};
    const GLsizeiptr sizes[1] = {16};
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, -1, bufs, offs, sizes);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glBindBuffersRange count<0 → INVALID_VALUE");

    // Hợp lệ: bind 2 buffer vào 2 binding point.
    const GLuint bufs2[2] = {b0, b1};
    const GLintptr offs2[2] = {0, 16};
    const GLsizeiptr sizes2[2] = {16, 16};
    DrainErrors();
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, 2, bufs2, offs2, sizes2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBindBuffersRange hợp lệ không lỗi");
    {
        GLint v = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &v);
        TGLMT_CHECK(v >= 2, "MAX_UNIFORM_BUFFER_BINDINGS phải >= 2");
    }

    // buffers == NULL → reset về unbound (offset/size mặc định).
    DrainErrors();
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, 2, nullptr, offs2, sizes2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBindBuffersRange với buffers=NULL không lỗi");

    // Buffer không tồn tại → INVALID_OPERATION.
    const GLuint bad[1] = {999999};
    DrainErrors();
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, 1, bad, offs, sizes);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glBindBuffersRange buffer không tồn tại → INVALID_OPERATION");

    // size <= 0 → INVALID_VALUE.
    const GLsizeiptr zeroSize[1] = {0};
    const GLuint ok[1] = {b0};
    DrainErrors();
    glBindBuffersRange(GL_UNIFORM_BUFFER, 0, 1, ok, offs, zeroSize);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glBindBuffersRange size=0 → INVALID_VALUE");

    glDeleteBuffers(1, &b0);
    glDeleteBuffers(1, &b1);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core buffer entry points (GL 4.6 §6.1–§6.5) ===\n");
    static tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    const Case kCases[] = {
        {"glNamedBufferStorage", TestNamedBufferStorage},
        {"glMapNamedBuffer", TestMapNamedBuffer},
        {"glFlushMapped*BufferRange", TestFlushMappedRange},
        {"glInvalidateBuffer*", TestInvalidateBuffer},
        {"glCopyNamedBufferSubData", TestCopyNamedBufferSubData},
        {"glBindBuffersRange", TestBindBuffersRange},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
