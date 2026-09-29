// test_readpixels_packed.cpp — ReadPixels readback, P* packed unpack, Getn* robustness, VAO DSA.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    // --- ReadPixels từ clear color (default framebuffer) ---
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    unsigned char px[16] = {0};
    glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, px); // RGBA/UBYTE
    assert(px[0] == 255 && px[1] == 0 && px[2] == 0 && px[3] == 255);
    assert(glGetError() == 0);
    // --- ReadnPixels bufSize guard ---
    glReadnPixels(0, 0, 2, 2, 0x1908, 0x1401, 16, px);
    assert(glGetError() == 0);
    glReadnPixels(0, 0, 2, 2, 0x1908, 0x1401, 4, px); // quá nhỏ → INVALID_OPERATION
    assert(glGetError() == 0x0502);
    // --- P* packed: glVertexP3ui UNSIGNED_INT_2_10_10_10_REV max → ~1.0 ---
    glVertexP3ui(0x8368, 0x3FFFFFFF); // x=y=z=1023 max, w=3 max
    double cur[4] = {0};
    ctx.state.GetShadow(0x8626, cur, 32, 0);
    assert(cur[0] > 0.99 && cur[1] > 0.99 && cur[2] > 0.99);
    glColorP4ui(0x8368, 0x3FFFFFFF);
    // --- TexCoordP1 (từng thiếu trong mapping) ---
    glTexCoordP1ui(0x8368, 0x3FF);
    // --- GetnUniform ---
    const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0.0); }";
    tglmt::GLuint s = glCreateShader(0x8B31);
    glShaderSource(s, 1, &vs, nullptr);
    glCompileShader(s);
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, s);
    glLinkProgram(p);
    glUseProgram(p);
    tglmt::GLint loc = glGetUniformLocation(p, "u");
    glUniform1f(loc, 3.14f);
    float v = 0;
    glGetnUniformfv(p, loc, 4, &v);
    assert(v > 3.13f && v < 3.15f);
    // --- VAO DSA ---
    tglmt::GLuint vao, vbo;
    glCreateVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 8);
    glVertexArrayAttribFormat(vao, 0, 2, 0x1406, 0, 0);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayElementBuffer(vao, vbo);
    glEnableVertexArrayAttrib(vao, 0);
    assert(ctx.vaos[vao].attribs[0].enabled);
    assert(ctx.vaos[vao].elementBuffer == vbo);
    // --- TexBuffer view ---
    tglmt::GLuint t;
    glGenTextures(1, &t);
    glBindTexture(0x8C2A, t); // TEXTURE_BUFFER
    float bufdata[4] = {1, 2, 3, 4};
    glBindBuffer(0x8C2A, vbo);
    glBufferData(0x8C2A, sizeof(bufdata), bufdata, 0x88E4);
    glTexBuffer(0x8C2A, 0x822E /*R32F*/, vbo);
    assert(ctx.textures[t].pixels.size() == sizeof(bufdata));
    // --- GetTexImage roundtrip ---
    tglmt::GLuint t2;
    glGenTextures(1, &t2);
    glBindTexture(0x0DE1, t2);
    unsigned char img[16] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 12, 13, 14, 15, 16};
    glPixelStorei(0x0CF5, 1); // UNPACK_ALIGNMENT=1 (tight; 0x0CF2 là ROW_LENGTH)
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, img);
    unsigned char back[16] = {0};
    glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, back);
    assert(memcmp(img, back, 16) == 0);
    printf("test_readpixels_packed PASS\n");
    return 0;
}
