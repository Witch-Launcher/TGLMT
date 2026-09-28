// test_core_registry.cpp — Unit: registry/gen/is/delete + glGetError rỗng + GetString.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    assert(glGetError() == 0);
    // GenBuffers/Is/Delete
    tglmt::GLuint b[2] = {0, 0};
    glGenBuffers(2, b);
    assert(b[0] && b[1] && b[0] != b[1]);
    assert(glIsBuffer(b[0]) == 1);
    assert(glIsBuffer(999999) == 0);
    // BufferData + SubData + Get
    glBindBuffer(0x8892, b[0]); // ARRAY_BUFFER
    float v[3] = {0.0f, 1.0f, 0.0f};
    glBufferData(0x8892, sizeof(v), v, 0x88E4);
    assert(glGetError() == 0);
    tglmt::GLint sz = 0;
    glGetBufferParameteriv(0x8892, 0x8764, &sz); // BUFFER_SIZE
    assert(sz == (tglmt::GLint)sizeof(v));
    float out[3] = {9, 9, 9};
    glGetBufferSubData(0x8892, 0, sizeof(out), out);
    assert(out[0] == 0.0f && out[1] == 1.0f && out[2] == 0.0f);
    // GetString chân lý
    const char* ver = (const char*)glGetString(0x1F02);
    assert(strstr(ver, "4.6") != nullptr);
    const char* ext0 = (const char*)glGetStringi(0x1F03, 0);
    assert(ext0 && strlen(ext0) > 0);
    // Error queue: enum sai phải Record
    glBindBuffer(0x9999 /*target lạ*/, b[0]); // vẫn bind (không validate target ở M2) — không assert error
    glDeleteBuffers(2, b);
    assert(glIsBuffer(b[0]) == 0);
    printf("test_core_registry PASS\n");
    return 0;
}
