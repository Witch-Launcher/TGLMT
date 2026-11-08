// test_polygon_offset_apple.cpp — glPolygonOffset must reach Metal's depth bias.
// Two overlapping full-screen quads: far red at z=0.9, near green at z=0.8.
// Without offset GL_LESS keeps green. With a large positive offset the near
// quad is pushed behind the far one, so red must survive. A shadow-only
// implementation (no setDepthBias) leaves green in both cases.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "uniform float uZ;\n"
    "void main() { gl_Position = vec4(aPos.xy, uZ, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location=0) out vec4 oColor;\n"
    "uniform vec4 uColor;\n"
    "void main() { oColor = uColor; }\n";

static GLuint MakeProgram() {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("test_polygon_offset_apple FAIL: link: %s\n", log);
        return 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

static bool ReadCenter(unsigned char out[4]) {
    unsigned char px[32 * 32 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 32, 32, 0x1908, 0x1401, px);
    if (glGetError() != 0) return false;
    memcpy(out, px + (16 * 32 + 16) * 4, 4);
    return true;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_polygon_offset_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTargetWithDepth(32, 32, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_polygon_offset_apple FAIL: target\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint colorTex = 0, depthRbo = 0, fbo = 0;
    glGenTextures(1, &colorTex);
    glBindTexture(0x0DE1, colorTex);
    glTexImage2D(0x0DE1, 0, 0x8058, 32, 32, 0, 0x1908, 0x1401, nullptr);
    glGenRenderbuffers(1, &depthRbo);
    glBindRenderbuffer(0x8D41, depthRbo);
    glRenderbufferStorage(0x8D41, 0x81A6 /*DEPTH_COMPONENT24*/, 32, 32);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0 /*COLOR_ATTACHMENT0*/, 0x0DE1, colorTex, 0);
    glFramebufferRenderbuffer(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x8D41, depthRbo);
    GLenum drawBuf = 0x8CE0;
    glDrawBuffers(1, &drawBuf);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5 /*FRAMEBUFFER_COMPLETE*/) {
        printf("test_polygon_offset_apple FAIL: FBO incomplete\n");
        return 1;
    }

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    const float quad[6 * 3] = {-1, -1, 0, 1, -1, 0, 1, 1, 0,
                               -1, -1, 0, 1, 1, 0, -1, 1, 0};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, 0x1406, 0, 0, (void*)0);

    GLuint prog = MakeProgram();
    if (!prog) return 1;
    glUseProgram(prog);
    GLint lZ = glGetUniformLocation(prog, "uZ");
    GLint lColor = glGetUniformLocation(prog, "uColor");

    glViewport(0, 0, 32, 32);
    glDisable(0x0BE2); // blend
    glDisable(0x0B44); // cull
    glEnable(0x0B71);  // depth test
    glDepthFunc(0x0201); // GL_LESS
    glClearDepth(1.0);
    glClearColor(0, 0, 0, 1);

    auto draw = [&](float z, float r, float g, float b) {
        glUniform1f(lZ, z);
        glUniform4f(lColor, r, g, b, 1.0f);
        glDrawArrays(0x0004, 0, 6);
    };
    auto clear = [&]() {
        glClear(0x00004000 | 0x00000100); // COLOR | DEPTH
    };

    clear();
    glDisable(0x8037); // GL_POLYGON_OFFSET_FILL
    draw(0.82f, 1, 0, 0);
    draw(0.8f, 0, 1, 0);
    unsigned char c[4] = {};
    if (!ReadCenter(c) || c[1] < 200 || c[0] > 50) {
        printf("test_polygon_offset_apple FAIL: baseline expected green, got (%u,%u,%u)\n",
               c[0], c[1], c[2]);
        return 1;
    }

    clear();
    draw(0.82f, 1, 0, 0);
    draw(0.99f, 0, 1, 0);
    if (!ReadCenter(c) || c[0] < 200 || c[1] > 50) {
        printf("test_polygon_offset_apple FAIL: depth test expected red, got (%u,%u,%u)\n",
               c[0], c[1], c[2]);
        return 1;
    }

    clear();
    glDisable(0x8037);
    draw(0.82f, 1, 0, 0);
    glEnable(0x8037);
    glPolygonOffset(0.0f, 1000000.0f);
    draw(0.8f, 0, 1, 0);
    if (!ReadCenter(c) || c[0] < 200 || c[1] > 50) {
        printf("test_polygon_offset_apple FAIL: offset expected red, got (%u,%u,%u)\n",
               c[0], c[1], c[2]);
        return 1;
    }

    glPolygonOffset(0, 0);
    glDisable(0x8037);
    printf("test_polygon_offset_apple PASS (baseline green, positive offset red)\n");
    return 0;
}
