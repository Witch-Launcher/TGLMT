// test_m5b_gl_draw.cpp — M5b E2E: render HOÀN TOÀN qua API OpenGL 4.6 trên Apple
// backend (VAO+VBO+GLSL program+uniforms+glDraw*), đọc bằng glReadPixels.
// Không chạm M5 API trực tiếp (trừ setDefaultRenderTarget của shell).
// Thiếu GPU → SKIP. Sai pixel → FAIL thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec4 col;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "layout(location = 0) uniform mat4 uMVP;\n"
    "layout(location = 1) uniform float uBright;\n"
    "void main() {\n"
    "  vCol = col * uBright;\n"
    "  gl_Position = uMVP * vec4(pos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_m5b_gl_draw SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    // Default target như shell app làm (offscreen 64x64)
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // pos2 + col4, stride 24
    float verts[3 * 6] = {-0.5f, -0.5f, 1, 0, 0, 1, 0.5f, -0.5f, 1, 0, 0, 1,
                          0.0f, 0.5f, 1, 0, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, 0x1406, 0, 24, (void*)8);

    tglmt::GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    tglmt::GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    if (!ok) { printf("FAIL vs compile\n"); return 1; }
    glGetShaderiv(fs, 0x8B81, &ok);
    if (!ok) { printf("FAIL fs compile\n"); return 1; }
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { printf("FAIL link\n"); return 1; }
    // attrib locations linker gán/explicit
    if (glGetAttribLocation(p, "pos") != 0) { printf("FAIL attrib pos\n"); return 1; }
    if (glGetAttribLocation(p, "col") != 1) { printf("FAIL attrib col\n"); return 1; }
    glUseProgram(p);
    tglmt::GLint lmvp = glGetUniformLocation(p, "uMVP");
    tglmt::GLint lb = glGetUniformLocation(p, "uBright");
    float ident[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    glUniformMatrix4fv(lmvp, 1, 0, ident);
    glUniform1f(lb, 0.5f); // brightness một nửa → đỏ 127

    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1); // nền xanh để phân biệt clear vs draw
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) { printf("FAIL gl error\n"); return 1; }
    // Draw thứ 2 KHÔNG clear (LOAD): vẽ tam giác nhỏ hơn đè lên? — kiểm tra giữ nền:
    // đọc pixel góc (ngoài tam giác) phải còn xanh clear.
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    unsigned char* corner = px + (2 * 64 + 2) * 4;
    printf("mid=(%u,%u,%u) corner=(%u,%u,%u)\n", mid[0], mid[1], mid[2], corner[0], corner[1],
           corner[2]);
    // mid: đỏ*0.5 ≈ 127 (brightness uniform qua buffer gộp!)
    if (std::abs((int)mid[0] - 127) > 4 || mid[1] > 4 || mid[2] > 4) {
        printf("FAIL mid\n");
        return 1;
    }
    // corner: ngoài tam giác → xanh clear (chứng minh clear+draw+readback flip đúng)
    if (!(corner[2] > 200 && corner[0] < 4)) { printf("FAIL corner\n"); return 1; }
    printf("test_m5b_gl_draw PASS (pure-GL render on Metal)\n");
    return 0;
}
