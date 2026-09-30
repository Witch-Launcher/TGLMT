// test_screenshot_pbo_apple.cpp — tái hiện vanilla Screenshot.takeScreenshot:
// FBO color texture (NULL data như MC), draw vào, rồi PBO-bound
// glGetTextureSubImage full-size RGBA/UBYTE. Không được có GL error
// (device crash copyTobuffer 1282 khi vào world).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_screenshot_pbo SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;

    const int W = 256, H = 128;
    auto target = ctx.device->makeRenderTarget(W, H, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    // Texture framebuffer-size, NULL data (y hệt MC texture 41).
    tglmt::GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, nullptr);
    if (glGetError() != 0) { printf("FAIL teximage\n"); return 1; }

    tglmt::GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, tex, 0);
    if (glGetError() != 0) { printf("FAIL fbo attach\n"); return 1; }

    // Draw đỏ vào FBO (program tối giản, fullscreen triangle).
    tglmt::GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float verts[6] = {-1, -1, 3, -1, -1, 3};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    tglmt::GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { printf("FAIL link\n"); return 1; }
    glUseProgram(p);
    glViewport(0, 0, W, H);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) { printf("FAIL draw\n"); return 1; }

    // Screenshot path: PBO + GetTextureSubImage full RGBA/UBYTE (y MC copyTobuffer).
    tglmt::GLuint pbo = 0;
    glGenBuffers(1, &pbo);
    glBindBuffer(0x88EB, pbo);
    size_t need = (size_t)W * H * 4;
    std::vector<uint8_t> zero(need, 0);
    glBufferData(0x88EB, (tglmt::GLsizeiptr)need, zero.data(), 0x88E8);
    if (glGetError() != 0) { printf("FAIL pbo data\n"); return 1; }
    glGetTextureSubImage(tex, 0, 0, 0, 0, W, H, 0, 0x1908, 0x1401, (tglmt::GLsizei)need,
                         (void*)0);
    tglmt::GLenum e = glGetError();
    if (e != 0) {
        printf("FAIL copyTobuffer path: GL error 0x%x\n", e);
        return 1;
    }
    // PBO phải chứa đỏ (draw vào FBO), không phải đen.
    {
        auto it = ctx.buffers.find(pbo);
        if (it == ctx.buffers.end() || it->second.data.size() < need) {
            printf("FAIL pbo size\n");
            return 1;
        }
        const uint8_t* d = it->second.data.data();
        // giữa ảnh phải đỏ
        const uint8_t* mid = d + ((size_t)(H / 2) * W + W / 2) * 4;
        printf("pbo mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
        if (!(mid[0] > 200 && mid[1] < 4 && mid[2] < 4)) {
            printf("FAIL pbo pixels (want red, GPU refresh broken?)\n");
            return 1;
        }
    }
    printf("test_screenshot_pbo PASS\n");
    return 0;
}
