// test_attr_fill_apple.cpp — Attribute shader đọc mà GL array TẮT (đúng ca Iris
// "TEXTURED": ShaderKey.TEXTURED bind theo DefaultVertexFormat.POSITION_TEX nhưng
// EntityPatcher inject "in ivec3 iris_Entity" — format không có Entity nên array
// không bao giờ enabled). Metal BẮT vertex descriptor chứa MỌI attribute shader
// đọc ("Vertex attribute ent(2) is missing from the vertex descriptor" — probe
// thật) → AppleDrawGL phải bù entry trỏ zero buffer. Không có fix: pipeline nil →
// draw bị bỏ → pixel nền giữ nguyên. Thiếu GPU → SKIP. Sai pixel → FAIL thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "in ivec3 iris_Entity;\n" // KHÔNG layout, KHÔNG enable array — như Iris thật
    "layout(location = 0) out vec3 vE;\n"
    "void main() { vE = vec3(iris_Entity) * 0.01; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec3 vE;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(0.2 + vE.r, 0.8, 0.4, 1.0); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_attr_fill_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_attr_fill_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    if (!ok) {
        printf("test_attr_fill_apple FAIL: vs compile\n");
        return 1;
    }
    glGetShaderiv(fs, 0x8B81, &ok);
    if (!ok) {
        printf("test_attr_fill_apple FAIL: fs compile\n");
        return 1;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    glGetProgramiv(prog, 0x8B82, &ok);
    if (!ok) {
        GLchar log[1024] = {0};
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        printf("test_attr_fill_apple FAIL: link: %s\n", log);
        return 1;
    }
    // Lộ trình auto-assign: iris_Entity phải nhận index trống nhỏ nhất (1), không
    // đè pos@0 (ca aliasing đã fix ở gl_program.cpp).
    GLint entLoc = glGetAttribLocation(prog, "iris_Entity");
    if (entLoc != 1) {
        printf("test_attr_fill_apple FAIL: iris_Entity loc=%d (mong 1)\n", (int)entLoc);
        return 1;
    }

    // VAO CHỈ enable array 0 (như POSITION_TEX); array 1 (iris_Entity) tắt.
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float tri[6] = {-1, -1, 3, -1, -1, 3}; // phủ kín 64×64
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);

    glUseProgram(prog);
    glBindFramebuffer(0x8CA9, 0);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    uint64_t noPipeBefore = ctx.appleStats.noPipeline;
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) {
        printf("test_attr_fill_apple FAIL: gl error sau draw\n");
        return 1;
    }
    if (ctx.appleStats.drawsEncoded <= encBefore) {
        printf("test_attr_fill_apple FAIL: draw khong duoc encode\n");
        return 1;
    }
    if (ctx.appleStats.noPipeline != noPipeBefore) {
        printf("test_attr_fill_apple FAIL: pipeline nil (descriptor thieu attr?)\n");
        return 1;
    }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* q = px + (32 * 64 + 32) * 4;
    printf("center=(%u,%u,%u,%u)\n", q[0], q[1], q[2], q[3]);
    // vE = 0 khi iris_Entity đọc từ zero buffer → (0.2, 0.8, 0.4) = (51, 204, 102).
    // Nếu fill trỏ nhầm buffer (vd position as int) thì vE phi mã → pixel lệch.
    if (!(q[0] > 40 && q[0] < 62 && q[1] > 193 && q[1] < 216 && q[2] > 91 && q[2] < 113)) {
        printf("test_attr_fill_apple FAIL: pixel sai (fill attribute loi?)\n");
        return 1;
    }
    printf("test_attr_fill_apple PASS (fill attribute zero, link loc 0/1)\n");
    return 0;
}
