// test_elements_offset0.cpp — Hồi quy bug nghiêm trọng: glDrawElements với EBO
// bound và offset NULL ((void*)0) PHẢI đi đường indexed (spec §10.4), không được
// nhầm thành drawPrimitives (đọc lố VBO: macOS thoát nhờ zero-pad, iOS ra neon).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    tglmt::GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float v[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(v), v, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    tglmt::GLuint idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    assert(ctx.vaos[vao].elementBuffer == ebo); // EBO là state của VAO
    ctx.device->clearTrace();
    glDrawElements(0x0004, 6, 0x1405, (void*)0); // offset 0 + EBO → INDEXED
    auto& tr = ctx.device->drawTrace();
    assert(tr.size() == 1);
    assert(tr[0].indexed && tr[0].count == 6);
    // Không EBO + NULL → INVALID_OPERATION (thay vì đọc bừa).
    // (Gỡ cả EBO khỏi VAO lẫn binding global.)
    glBindVertexArray(0);
    glBindBuffer(0x8893, 0);
    tglmt::GLuint vao2;
    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2); // VAO trống, không EBO
    ctx.device->clearTrace();
    glDrawElements(0x0004, 3, 0x1405, nullptr);
    assert(glGetError() == 0x0502);
    assert(ctx.device->drawTrace().empty());
    printf("test_elements_offset0 PASS\n");
    return 0;
}
