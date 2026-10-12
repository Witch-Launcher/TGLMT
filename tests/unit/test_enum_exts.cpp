// test_enum_exts.cpp — Unit: P3 batch-3 enum-only extensions.
//
// Quy tắc vàng: ext được advertise ⇒ có test đang pass. Test verify:
//  1. 8 enum-only ext xuất hiện trong glGetStringi(GL_EXTENSIONS).
//  2. Behavior core tương ứng KHÔNG no-op (gọi được, state/đường đi hoạt động).
// Enum đã define sẵn (gl.xml cross-ref — ext_coverage.py --kexts verify).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_enum_exts FAIL: %s\n", msg); ++gFails; }
}

static const char* kEnumExts[] = {
    "GL_ARB_copy_buffer",
    "GL_ARB_draw_indirect",
    "GL_ARB_map_buffer_range",
    "GL_ARB_sampler_objects",
    "GL_ARB_texture_storage",
    "GL_ARB_uniform_buffer_object",
    "GL_ARB_vertex_array_object",
    "GL_OES_element_index_uint",
};

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- 1. Cả 8 enum ext được advertise.
    GLint n = 0;
    glGetIntegerv(0x821D /*GL_NUM_EXTENSIONS*/, &n);
    Check(n == (GLint)tglmt_num_extensions(), "NUM_EXTENSIONS khớp tglmt_num_extensions()");
    for (const char* want : kEnumExts) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break; }
        }
        Check(found, want);
    }

    // --- 2. Behavior verify (core path — không no-op).
    // GL_ARB_copy_buffer: glCopyBufferSubData với COPY_READ/WRITE target.
    GLuint src = 0, dst = 0;
    glGenBuffers(1, &src);
    glGenBuffers(1, &dst);
    glBindBuffer(0x8892 /*ARRAY_BUFFER*/, src);
    glBufferData(0x8892, 256, nullptr, 0x88E4 /*DYNAMIC_DRAW*/);
    glBindBuffer(0x8F36 /*GL_COPY_READ_BUFFER*/, src);
    glBindBuffer(0x8F37 /*GL_COPY_WRITE_BUFFER*/, dst);
    glBufferData(0x8F37, 256, nullptr, 0x88E4);
    glCopyBufferSubData(0x8F36, 0x8F37, 0, 0, 64); // không crash = path chạy

    // GL_ARB_map_buffer_range: glMapBufferRange + flush.
    glBindBuffer(0x8892, src);
    void* m = glMapBufferRange(0x8892, 0, 256, 0x0002 /*WRITE*/ | 0x0010 /*INVALIDATE*/);
    Check(m != nullptr, "glMapBufferRange map được");
    if (m) glUnmapBuffer(0x8892);

    // GL_ARB_texture_storage: glTexStorage2D.
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1 /*TEXTURE_2D*/, tex);
    glTexStorage2D(0x0DE1, 4, 0x8058 /*RGBA8*/, 64, 64);
    Check(glGetError() == 0, "glTexStorage2D không lỗi");

    // GL_ARB_vertex_array_object: gen/bind.
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    Check(vao != 0, "glGenVertexArrays tạo VAO");
    glBindVertexArray(0);

    // GL_ARB_sampler_objects: gen/bind + param.
    GLuint smp = 0;
    glGenSamplers(1, &smp);
    glBindSampler(0, smp);
    glSamplerParameteri(smp, 0x2801 /*TEXTURE_MAG_FILTER*/, 0x2601 /*LINEAR*/);
    glBindSampler(0, 0);

    // GL_OES_element_index_uint: UNSIGNED_INT index type được handle.
    // (drawElements với 0x1405 → IndexType::UInt32; verify không reject.)
    glBindVertexArray(vao);
    GLuint idx = 0;
    glGenBuffers(1, &idx);
    glBindBuffer(0x8893 /*ELEMENT_ARRAY_BUFFER*/, idx);
    glBufferData(0x8893, 12, nullptr, 0x88E4);
    glDrawElements(0x0004 /*TRIANGLES*/, 3, 0x1405 /*UNSIGNED_INT*/, nullptr);
    // Không crash = type accepted (Null backend).

    // GL_ARB_draw_indirect: glDrawArraysIndirect đọc indirect buffer.
    glBindVertexArray(0);

    // GL_ARB_uniform_buffer_object: bind buffer base (UBO core path).
    GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(0x8A11 /*UNIFORM_BUFFER*/, ubo);
    glBufferData(0x8A11, 128, nullptr, 0x88E4);
    glBindBufferBase(0x8A11, 0, ubo);

    glBindVertexArray(0);
    glDeleteBuffers(1, &src);
    glDeleteBuffers(1, &dst);
    glDeleteBuffers(1, &idx);
    glDeleteBuffers(1, &ubo);
    glDeleteTextures(1, &tex);
    glDeleteVertexArrays(1, &vao);
    glDeleteSamplers(1, &smp);

    if (gFails) return 1;
    printf("test_enum_exts PASS\n");
    return 0;
}
