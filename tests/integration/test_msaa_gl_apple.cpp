// test_msaa_gl_apple.cpp — GL MSAA 2D storage + FBO draw + resolve readback.
// Skip on no Metal; on Null covered by probe/contract tests.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_msaa_gl_apple FAIL: %s\n", msg); ++gFails; }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 c;\n"
    "void main(){ c = vec4(1,0,0,1); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_msaa_gl_apple SKIP: no MTL device\n");
        return 0;
    }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x9100 /*TEXTURE_2D_MULTISAMPLE*/, tex);
    glTexImage2DMultisample(0x9100, 4, 0x8058 /*RGBA8*/, 64, 64, GL_TRUE);
    Check(glGetError() == GL_NO_ERROR, "MSAA texture storage");
    auto tit = ctx.textures.find(tex);
    Check(tit != ctx.textures.end() && tit->second.samples == 4, "texture samples reflected");
    Check(tit != ctx.textures.end() && tit->second.msaaTarget != nullptr, "MSAA target created");
    GLint samples = 0;
    glGetTexLevelParameteriv(0x9100, 0, 0x9106 /*TEXTURE_SAMPLES*/, &samples);
    Check(samples == 4, "TEXTURE_SAMPLES query");

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40 /*FRAMEBUFFER*/, fbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x9100, tex, 0);
    Check(glGetError() == GL_NO_ERROR, "attach MSAA texture");
    Check(glCheckFramebufferStatus(0x8CA9) == 0x8CD5, "FBO complete");

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
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    Check(glGetError() == GL_NO_ERROR, "draw no GL error");
    Check(ctx.appleStats.noPipeline == 0, "pipeline compiled with rasterSampleCount");

    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    Check(glGetError() == GL_NO_ERROR, "readback no GL error");
    unsigned char* q = px + (32 * 64 + 32) * 4;

    unsigned char direct[64 * 64 * 4] = {};
    if (tit != ctx.textures.end() && tit->second.msaaTarget)
        tit->second.msaaTarget->readback(direct, 64 * 4);
    unsigned char* dq = direct + (32 * 64 + 32) * 4;

    Check(q[0] > 200 && q[1] < 50 && q[2] < 50, "resolved center is red");
    Check(dq[0] > 200 && dq[1] < 50 && dq[2] < 50, "MSAA resolve texture is red");

    // DSA multisample storage + multisample renderbuffer attachment.
    GLuint tex2 = 0;
    glCreateTextures(0x9100, 1, &tex2);
    glTextureStorage2DMultisample(tex2, 4, 0x8058, 32, 32, GL_TRUE);
    Check(glGetError() == GL_NO_ERROR, "DSA MSAA storage");
    glBindTexture(0x9100, tex2);
    glTexStorage2DMultisample(0x9100, 4, 0x8058, 32, 32, GL_TRUE);
    Check(glGetError() == 0x0502, "MSAA storage immutable");
    glTexImage2DMultisample(0x9100, 4, 0x8058, 32, 32, GL_TRUE);
    Check(glGetError() == 0x0502, "MSAA TexImage after storage immutable");
    auto t2 = ctx.textures.find(tex2);
    Check(t2 != ctx.textures.end() && t2->second.msaaTarget != nullptr, "DSA MSAA target");

    // sRGB MSAA: target/resolve phải giữ đúng format, không hard-code RGBA8.
    GLuint stex = 0;
    glGenTextures(1, &stex);
    glBindTexture(0x9100, stex);
    glTexImage2DMultisample(0x9100, 4, 0x8C43 /*SRGB8_ALPHA8*/, 32, 32, GL_TRUE);
    Check(glGetError() == GL_NO_ERROR, "sRGB MSAA storage");
    auto st = ctx.textures.find(stex);
    Check(st != ctx.textures.end() && st->second.msaaTarget != nullptr, "sRGB MSAA target");
    if (st != ctx.textures.end() && st->second.msaaTarget) {
        auto* sct = st->second.msaaTarget->colorTexture(0);
        Check(sct && sct->pixelFormat() == metal::PixelFormat::RGBA8Unorm_sRGB,
              "sRGB MSAA color format đúng");
    }
    glDeleteTextures(1, &stex);

    GLuint rbo = 0, fbo2 = 0;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(0x8D41, rbo);
    glRenderbufferStorageMultisample(0x8D41, 4, 0x8058, 64, 64);
    Check(glGetError() == GL_NO_ERROR, "MSAA renderbuffer storage");
    glGenFramebuffers(1, &fbo2);
    glBindFramebuffer(0x8D40, fbo2);
    glFramebufferRenderbuffer(0x8D40, 0x8CE0, 0x8D41, rbo);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "MSAA RBO FBO complete");
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    q = px + (32 * 64 + 32) * 4;
    Check(q[0] > 200 && q[1] < 50 && q[2] < 50, "MSAA RBO resolve readback is red");

    glDeleteProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &fbo2);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &tex);
    glDeleteTextures(1, &tex2);
    if (gFails == 0) { printf("test_msaa_gl_apple PASS\n"); return 0; }
    return 1;
}
