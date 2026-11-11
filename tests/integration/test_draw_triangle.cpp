// test_draw_triangle.cpp — Integration: VAO+VBO+draw tam giác, kiểm drawTrace (Null backend).
// Tương đương test_draw.c trong đề bài (glDrawArrays 3 đỉnh).
#include "tglmt/gl46.h"
#include "test_util.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    tglmt::GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float verts[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    TGLMT_CHECK((glGetError() == 0), "glGetError() == 0");
    ctx.device->clearTrace();
    glDrawArrays(0x0004, 0, 3); // TRIANGLES
    auto& tr = ctx.device->drawTrace();
    TGLMT_CHECK((tr.size() == 1), "tr.size() == 1");
    TGLMT_CHECK((tr[0].count == 3 && !tr[0].indexed && tr[0].instanceCount == 1), "tr[0].count == 3 && !tr[0].indexed && tr[0].instanceCount == 1");
    // DrawElements index
    ctx.device->clearTrace();
    tglmt::GLuint idx[] = {0, 1, 2};
    glDrawElements(0x0004, 3, 0x1405, idx); // UNSIGNED_INT, client pointer
    TGLMT_CHECK((ctx.device->drawTrace().size() == 1), "ctx.device->drawTrace().size() == 1");
    TGLMT_CHECK((ctx.device->drawTrace()[0].indexed), "ctx.device->drawTrace()[0].indexed");
    // Viewport + enable depth (bake pipeline, không crash)
    glViewport(0, 0, 256, 256);
    glEnable(0x0B71); // DEPTH_TEST
    glDepthFunc(0x0203); // LESS
    TGLMT_CHECK((tglmt::gl::glIsEnabled(0x0B71) == 1), "tglmt::gl::glIsEnabled(0x0B71) == 1");
    printf("test_draw_triangle PASS\n");
    return TGLMT_RESULT;
}
