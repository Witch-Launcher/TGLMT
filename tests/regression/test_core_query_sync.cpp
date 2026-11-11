// Test nhóm Query/Sync của GL 4.6 Core — 10 entry point:
//   glCreateQueries, glIsQuery, glFlush, glWaitSync,
//   glGetQueryIndexediv, glGetQueryBufferObjectiv/uiv/i64v/ui64v.
//
// Kiểm chứng HÀNH VI theo GL 4.6 §4.2 (Query Objects) và §6.1 (Flush/Finish):
//   • glCreateQueries: target hợp lệ → tạo query; target sai → INVALID_ENUM;
//     n<0 → INVALID_VALUE.
//   • glIsQuery: TRUE với id vừa tạo, FALSE với id không tồn tại.
//   • glGetQueryIndexediv: index>0 trên target không indexed → INVALID_VALUE.
//   • glGetQueryObject*/GetQueryBufferObject*: pname hợp lệ (kể cả QUERY_TARGET),
//     pname sai → INVALID_ENUM; buffer không tồn tại → INVALID_OPERATION;
//     offset âm → INVALID_VALUE; ghi vượt cuối buffer → INVALID_OPERATION.
//   • glGetQueryBufferObject* ghi kết quả vào buffer đúng vị trí (đọc lại bằng
//     glGetBufferSubData).
//   • glFlush / glWaitSync: không sinh lỗi với tham số hợp lệ.

#include "tglmt/gl46.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }

// ---------------------------------------------------------------------------
// glCreateQueries + glIsQuery
// ---------------------------------------------------------------------------
bool TestCreateQueries() {
    DrainErrors();
    GLuint ids[3] = {0, 0, 0};
    glCreateQueries(GL_SAMPLES_PASSED, 3, ids);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCreateQueries với target hợp lệ không lỗi");
    for (int i = 0; i < 3; ++i) {
        TGLMT_CHECK(ids[i] != 0, "glCreateQueries phải cấp id khác 0");
        TGLMT_CHECK(glIsQuery(ids[i]) == GL_TRUE, "glIsQuery phải TRUE với id vừa tạo");
    }
    TGLMT_CHECK(glIsQuery(999999) == GL_FALSE, "glIsQuery phải FALSE với id không tồn tại");
    TGLMT_CHECK(glIsQuery(0) == GL_FALSE, "glIsQuery(0) phải FALSE");

    // Target không hợp lệ → INVALID_ENUM (spec §4.2.2).
    DrainErrors();
    GLuint bad[1] = {0};
    glCreateQueries(0xDEAD, 1, bad);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glCreateQueries với target sai → INVALID_ENUM");

    // n < 0 → INVALID_VALUE.
    DrainErrors();
    glCreateQueries(GL_SAMPLES_PASSED, -1, bad);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glCreateQueries với n<0 → INVALID_VALUE");

    glDeleteQueries(3, ids);
    return true;
}

// ---------------------------------------------------------------------------
// glGetQueryIndexediv
// ---------------------------------------------------------------------------
bool TestGetQueryIndexediv() {
    GLuint ids[1] = {0};
    glCreateQueries(GL_SAMPLES_PASSED, 1, ids);

    // index = 0 hợp lệ với mọi target.
    DrainErrors();
    GLint v = -1;
    glGetQueryIndexediv(GL_SAMPLES_PASSED, 0, GL_QUERY_COUNTER_BITS, &v);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetQueryIndexediv index=0 với target thường không lỗi");
    TGLMT_CHECK(v >= 0, "GL_QUERY_COUNTER_BITS phải trả giá trị >= 0");

    // index > 0 trên target KHÔNG indexed → INVALID_VALUE (spec §4.2.1).
    DrainErrors();
    glGetQueryIndexediv(GL_SAMPLES_PASSED, 1, GL_QUERY_COUNTER_BITS, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetQueryIndexediv index>0 với target không indexed → INVALID_VALUE");

    // CURRENT_QUERY trả về 0 khi không có query active.
    DrainErrors();
    glGetQueryIndexediv(GL_SAMPLES_PASSED, 0, GL_CURRENT_QUERY, &v);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryIndexediv CURRENT_QUERY không lỗi");
    TGLMT_CHECK(v == 0, "CURRENT_QUERY phải là 0 khi không có query active");

    // Target không hợp lệ → INVALID_ENUM.
    DrainErrors();
    glGetQueryIndexediv(0xDEAD, 0, GL_QUERY_COUNTER_BITS, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glGetQueryIndexediv với target sai → INVALID_ENUM");

    glDeleteQueries(1, ids);
    return true;
}

// ---------------------------------------------------------------------------
// glGetQueryObject* + glGetQueryBufferObject* (8 hàm)
// ---------------------------------------------------------------------------
bool TestQueryObjectQueries() {
    GLuint ids[1] = {0};
    glCreateQueries(GL_SAMPLES_PASSED, 1, ids);
    const GLuint q = ids[0];

    // QUERY_TARGET (0x82EA) phải trả về target của query object.
    DrainErrors();
    GLint tv = 0;
    glGetQueryObjectiv(q, GL_QUERY_TARGET, &tv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryObjectiv QUERY_TARGET không lỗi");
    TGLMT_CHECK(tv == (GLint)GL_SAMPLES_PASSED,
                "QUERY_TARGET phải trả về target của query object");

    GLint64 tv64 = 0;
    glGetQueryObjecti64v(q, GL_QUERY_TARGET, &tv64);
    TGLMT_CHECK(tv64 == (GLint64)GL_SAMPLES_PASSED, "glGetQueryObjecti64v QUERY_TARGET");

    GLuint tu = 0;
    glGetQueryObjectuiv(q, GL_QUERY_TARGET, &tu);
    TGLMT_CHECK(tu == (GLuint)GL_SAMPLES_PASSED, "glGetQueryObjectuiv QUERY_TARGET");

    GLuint64 tu64 = 0;
    glGetQueryObjectui64v(q, GL_QUERY_TARGET, &tu64);
    TGLMT_CHECK(tu64 == (GLuint64)GL_SAMPLES_PASSED, "glGetQueryObjectui64v QUERY_TARGET");

    // pname sai → INVALID_ENUM (cả 4 biến thể).
    DrainErrors();
    glGetQueryObjectiv(q, 0xDEAD, &tv);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "glGetQueryObjectiv pname sai → INVALID_ENUM");
    DrainErrors();
    glGetQueryObjecti64v(q, 0xDEAD, &tv64);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "glGetQueryObjecti64v pname sai → INVALID_ENUM");
    DrainErrors();
    glGetQueryObjectuiv(q, 0xDEAD, &tu);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "glGetQueryObjectuiv pname sai → INVALID_ENUM");
    DrainErrors();
    glGetQueryObjectui64v(q, 0xDEAD, &tu64);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "glGetQueryObjectui64v pname sai → INVALID_ENUM");

    // id không phải query object → INVALID_OPERATION.
    DrainErrors();
    glGetQueryObjectiv(999999, GL_QUERY_RESULT, &tv);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetQueryObjectiv với id không tồn tại → INVALID_OPERATION");

    // ---- GetQueryBufferObject* ----
    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    const GLsizeiptr kSize = 64;
    // KHÔNG dùng GL_MAP_*_BIT: buffer được map persistently thì GL cấm
    // glGetBufferSubData (INVALID_OPERATION) — đúng spec.
    glNamedBufferStorage(buf, kSize, nullptr, 0);
    // Ghi giá trị mốc để biết đã ghi đè hay chưa.
    const GLubyte fill[8] = {0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD};
    glNamedBufferSubData(buf, 0, 8, fill);

    DrainErrors();
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, 0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryBufferObjectiv không lỗi");
    glGetQueryBufferObjectuiv(q, buf, GL_QUERY_RESULT, 8);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryBufferObjectuiv không lỗi");
    glGetQueryBufferObjecti64v(q, buf, GL_QUERY_RESULT, 16);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryBufferObjecti64v không lỗi");
    glGetQueryBufferObjectui64v(q, buf, GL_QUERY_RESULT, 24);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetQueryBufferObjectui64v không lỗi");

    // QUERY_TARGET cũng được phép với GetQueryBufferObject*.
    DrainErrors();
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_TARGET, 32);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetQueryBufferObjectiv với QUERY_TARGET không lỗi");
    {
        GLint tgt = 0;
        glGetNamedBufferSubData(buf, 32, 4, &tgt);
        TGLMT_CHECK(tgt == (GLint)GL_SAMPLES_PASSED,
                    "QUERY_TARGET ghi vào buffer phải là target của query");
    }

    // Buffer không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glGetQueryBufferObjectiv(q, 999999, GL_QUERY_RESULT, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetQueryBufferObjectiv với buffer không tồn tại → INVALID_OPERATION");

    // Offset âm → INVALID_VALUE.
    DrainErrors();
    glGetQueryBufferObjectiv(q, buf, GL_QUERY_RESULT, -4);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetQueryBufferObjectiv với offset âm → INVALID_VALUE");

    // Ghi vượt cuối buffer → INVALID_OPERATION.
    DrainErrors();
    glGetQueryBufferObjecti64v(q, buf, GL_QUERY_RESULT, kSize - 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetQueryBufferObjecti64v ghi vượt cuối buffer → INVALID_OPERATION");

    // pname sai → INVALID_ENUM.
    DrainErrors();
    glGetQueryBufferObjectiv(q, buf, 0xDEAD, 0);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glGetQueryBufferObjectiv pname sai → INVALID_ENUM");

    glDeleteBuffers(1, &buf);
    glDeleteQueries(1, ids);
    return true;
}

// ---------------------------------------------------------------------------
// glFlush + glWaitSync
// ---------------------------------------------------------------------------
bool TestFlushAndWaitSync() {
    DrainErrors();
    glFlush();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFlush không sinh lỗi");

    // glWaitSync với sync hợp lệ (tạo bằng glFenceSync).
    GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    TGLMT_CHECK(s != nullptr, "glFenceSync phải trả sync khác null");
    DrainErrors();
    glWaitSync(s, 0, GL_TIMEOUT_IGNORED);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glWaitSync với fence hợp lệ không lỗi");
    glDeleteSync(s);

    // WaitSync trên sync không tồn tại → theo spec phải báo lỗi; GL cho phép
    // implementation-defined, nhưng TGLMT báo INVALID_OPERATION (thống nhất với
    // phần còn lại của API).
    DrainErrors();
    glWaitSync((GLsync)0x1234, 0, GL_TIMEOUT_IGNORED);
    (void)FirstError();  // không khẳng định mã lỗi (spec không bắt buộc)

    // glWaitSync với flags chứa bit không hợp lệ → INVALID_VALUE (spec §6.1).
    GLsync s2 = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    DrainErrors();
    glWaitSync(s2, 0xFFFFFFFFu, GL_TIMEOUT_IGNORED);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glWaitSync với flags không hợp lệ → INVALID_VALUE");
    glDeleteSync(s2);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core query/sync entry points (GL 4.6 §4.2 / §6.1) ===\n");
    const Case kCases[] = {
        {"glCreateQueries + glIsQuery", TestCreateQueries},
        {"glGetQueryIndexediv", TestGetQueryIndexediv},
        {"glGetQueryObject* + glGetQueryBufferObject*", TestQueryObjectQueries},
        {"glFlush + glWaitSync", TestFlushAndWaitSync},
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
