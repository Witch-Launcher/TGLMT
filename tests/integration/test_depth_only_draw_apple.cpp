// test_depth_only_draw_apple.cpp — Regression: draw vào FBO chỉ có depth
// (glDrawBuffer(GL_NONE), như Voxy HiZ) không được lấy format depth làm
// colorAttachments[0]. Trước fix, Metal abort:
//   "MTLPixelFormatDepth32Float_Stencil8 is not color renderable".
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 c;\n"
    "void main() { c = vec4(1.0); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_depth_only_draw_apple SKIP: no MTL device\n");
        return 0;
    }

    GLuint depth = 0;
    glGenTextures(1, &depth);
    glBindTexture(0x0DE1, depth);
    glTexImage2D(0x0DE1, 0, 0x8CAC /*DEPTH_COMPONENT32F*/, 64, 64, 0,
                 0x1902 /*DEPTH_COMPONENT*/, 0x1406 /*FLOAT*/, nullptr);

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9 /*DRAW_FRAMEBUFFER*/, fbo);
    glFramebufferTexture2D(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x0DE1, depth, 0);
    glNamedFramebufferDrawBuffer(fbo, 0x0404 /*GL_NONE*/);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5 /*COMPLETE*/) {
        printf("test_depth_only_draw_apple SKIP: depth-only FBO incomplete\n");
        return 0;
    }

    GLuint vs = glCreateShader(0x8B31);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(0x8B30);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    if (!ok) {
        printf("test_depth_only_draw_apple FAIL: link\n");
        return 1;
    }

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    const float quad[] = {-1, -1, 1, -1, 1, 1, -1, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);

    glUseProgram(prog);
    glViewport(0, 0, 64, 64);
    glEnable(0x0B71); // DEPTH_TEST
    glDepthFunc(0x0207); // ALWAYS
    glDepthMask(1);
    glDrawArrays(0x0006 /*TRIANGLE_FAN*/, 0, 4);

    glFinish(); // kết thúc/commit encoder trước khi Context teardown
    GLenum err = glGetError();
    if (err != 0) {
        printf("test_depth_only_draw_apple FAIL: gl error 0x%x\n", err);
        return 1;
    }
    printf("test_depth_only_draw_apple PASS\n");
    return 0;
}
