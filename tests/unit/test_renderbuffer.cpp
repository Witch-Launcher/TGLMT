// test_renderbuffer.cpp — Unit: renderbuffer storage + FBO attachment.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_renderbuffer FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint rb = 0;
    glGenRenderbuffers(1, &rb);
    Check(rb != 0 && glIsRenderbuffer(rb), "gen renderbuffer");
    glBindRenderbuffer(0x8D41 /*RENDERBUFFER*/, rb);
    glRenderbufferStorage(0x8D41, 0x8058 /*RGBA8*/, 64, 32);
    Check(glGetError() == 0, "storage RGBA8");

    GLint v = -1;
    glGetRenderbufferParameteriv(0x8D41, 0x8D42 /*WIDTH*/, &v);
    Check(v == 64, "width");
    glGetRenderbufferParameteriv(0x8D41, 0x8D43 /*HEIGHT*/, &v);
    Check(v == 32, "height");
    glGetRenderbufferParameteriv(0x8D41, 0x8D44 /*INTERNAL_FORMAT*/, &v);
    Check(v == 0x8058, "internal format");
    glGetRenderbufferParameteriv(0x8D41, 0x8CAB /*SAMPLES*/, &v);
    Check(v == 0, "single-sample reports 0");
    glGetRenderbufferParameteriv(0x8D41, 0x8D50 /*RED_SIZE*/, &v);
    Check(v == 8, "red size");

    glGetIntegerv(0x8CA7 /*RENDERBUFFER_BINDING*/, &v);
    Check(v == (GLint)rb, "binding query");

    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40 /*FRAMEBUFFER*/, fbo);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD7, "empty FBO missing attachment");
    glFramebufferRenderbuffer(0x8D40, 0x8CE0 /*COLOR_ATTACHMENT0*/, 0x8D41, rb);
    Check(glGetError() == 0, "attach color RBO");
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete with color RBO");

    GLuint depthRb = 0;
    glGenRenderbuffers(1, &depthRb);
    glBindRenderbuffer(0x8D41, depthRb);
    glRenderbufferStorage(0x8D41, 0x81A6 /*DEPTH_COMPONENT24*/, 64, 32);
    glFramebufferRenderbuffer(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x8D41, depthRb);
    Check(glGetError() == 0, "attach depth RBO");
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete with depth RBO");

    glFramebufferRenderbuffer(0x8D40, 0x8D00, 0x8D41, 0);
    Check(glGetError() == 0, "detach depth RBO");
    glBindFramebuffer(0x8D40, 0);

    GLuint dsRb = 0;
    glCreateRenderbuffers(1, &dsRb);
    // Cùng kích thước với color attachment 64x32 — FBO complete theo spec.
    glNamedRenderbufferStorage(dsRb, 0x88F0 /*DEPTH24_STENCIL8*/, 64, 32);
    glGetNamedRenderbufferParameteriv(dsRb, 0x8D42, &v);
    Check(v == 64, "named width");
    glGetNamedRenderbufferParameteriv(dsRb, 0x8D54 /*DEPTH_SIZE*/, &v);
    Check(v == 24, "depth size");
    glGetNamedRenderbufferParameteriv(dsRb, 0x8D55 /*STENCIL_SIZE*/, &v);
    Check(v == 8, "stencil size");
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferRenderbuffer(0x8D40, 0x821A /*DEPTH_STENCIL_ATTACHMENT*/, 0x8D41, dsRb);
    Check(glGetError() == 0, "attach depth-stencil RBO");
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete with depth-stencil RBO");
    Check(glCheckNamedFramebufferStatus(fbo, 0x8D40) == 0x8CD5, "named FBO status complete");
    glBindFramebuffer(0x8D40, 0);

    while (glGetError() != 0) {}
    glBindRenderbuffer(0x1234, dsRb);
    Check(glGetError() == GL_INVALID_ENUM, "bad bind target");
    glRenderbufferStorage(0x8D41, 0x1234, 1, 1);
    Check(glGetError() == GL_INVALID_ENUM, "bad internal format");
    glRenderbufferStorageMultisample(0x8D41, 4, 0x8058, 16, 16);
    Check(glGetError() == GL_NO_ERROR, "MSAA RGBA8 storage accepted");
    GLint ms = 0;
    glGetRenderbufferParameteriv(0x8D41, 0x8CAB /*SAMPLES*/, &ms);
    Check(ms == 4, "MSAA renderbuffer samples");
    glRenderbufferStorageMultisample(0x8D41, 4, 0x8051 /*RGB8*/, 16, 16);
    Check(glGetError() == GL_INVALID_OPERATION, "MSAA non-RGBA8 format rejected");
    glRenderbufferStorageMultisample(0x8D41, 8, 0x8058, 16, 16);
    Check(glGetError() == GL_INVALID_OPERATION, "unsupported sample count rejected");

    glDeleteRenderbuffers(1, &rb);
    glDeleteRenderbuffers(1, &depthRb);
    glDeleteRenderbuffers(1, &dsRb);
    glDeleteFramebuffers(1, &fbo);
    Check(!glIsRenderbuffer(rb), "delete renderbuffer");
    return gFails ? 1 : 0;
}
