// test_fbo_3d_slice_apple.cpp — GL_TEXTURE_3D slice làm FBO color.
// Metal cấm view 2D từ 3D; render pass dùng depthPlane/level trên chính 3D
// texture. Test clear + draw vào từng slice và readback độc lập.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_fbo_3d_slice_apple FAIL: %s\n", msg); ++gFails; }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 c;\n"
    "void main(){ c = vec4(0,0,1,1); }\n";

static void ReadCenter(int w, int h, unsigned char* out) {
    unsigned char* px = new unsigned char[(size_t)w * h * 4];
    memset(px, 0, (size_t)w * h * 4);
    glReadPixels(0, 0, w, h, 0x1908, 0x1401, px);
    unsigned char* q = px + ((h / 2) * w + w / 2) * 4;
    memcpy(out, q, 4);
    delete[] px;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_fbo_3d_slice_apple SKIP: no MTL device\n");
        return 0;
    }
    while (glGetError() != GL_NO_ERROR) {}

    const int W = 8, H = 4, D = 2;
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x806F /*GL_TEXTURE_3D*/, tex);
    glTexImage3D(0x806F, 0, 0x8058 /*RGBA8*/, W, H, D, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == GL_NO_ERROR, "3D texture storage");
    auto tit = ctx.textures.find(tex);
    Check(tit != ctx.textures.end() && tit->second.gpu, "3D gpu texture created");

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 0);
    Check(glGetError() == GL_NO_ERROR, "attach 3D layer 0");
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "3D slice FBO complete");

    glViewport(0, 0, W, H);
    glDisable(0x0B71);
    glClearColor(0, 1, 0, 1);
    glClear(0x00004000);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    Check(glGetError() == GL_NO_ERROR, "clear layer 0 no GL error");
    unsigned char p0[4] = {};
    ReadCenter(W, H, p0);
    Check(glGetError() == GL_NO_ERROR, "read layer 0 no GL error");
    Check(p0[0] < 50 && p0[1] > 200 && p0[2] < 50, "layer 0 clear = green");

    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 1);
    Check(glGetError() == GL_NO_ERROR, "attach 3D layer 1");
    glClearColor(1, 0, 0, 1);
    glClear(0x00004000);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    unsigned char p1[4] = {};
    ReadCenter(W, H, p1);
    Check(p1[0] > 200 && p1[1] < 50 && p1[2] < 50, "layer 1 clear = red");

    // Layer 0 phải giữ nguyên green (depthPlane tách đúng slice).
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 0);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    ReadCenter(W, H, p0);
    Check(p0[0] < 50 && p0[1] > 200 && p0[2] < 50, "layer 0 unchanged after layer 1 clear");

    // Draw vào layer 1: render pass draw thường cũng phải dùng depthPlane.
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 1);
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr); glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs); glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    GLint linked = 0; glGetProgramiv(prog, 0x8B82, &linked);
    Check(linked != 0, "program link");
    glUseProgram(prog);

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    float tri[6] = {-1,-1, 3,-1, -1,3};
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    glDrawArrays(0x0004, 0, 3);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    Check(glGetError() == GL_NO_ERROR, "draw layer 1 no GL error");
    memset(p1, 0, sizeof(p1));
    ReadCenter(W, H, p1);
    Check(p1[0] < 50 && p1[1] < 50 && p1[2] > 200, "layer 1 draw = blue");

    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 0);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    ReadCenter(W, H, p0);
    Check(p0[0] < 50 && p0[1] > 200 && p0[2] < 50, "layer 0 still green after layer 1 draw");

    glDeleteProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    if (gFails == 0) { printf("test_fbo_3d_slice_apple PASS\n"); return 0; }
    printf("test_fbo_3d_slice_apple FAILED (%d)\n", gFails);
    return 1;
}
