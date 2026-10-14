// test_p10_sodium_iris.cpp — P10: framebuffer_no_attachments + texture_view +
// program_interface_query (Sodium/Iris dùng). Chạy cả 2 backend.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace tglmt { namespace gl {
void glFramebufferParameteri(GLenum t, GLenum p, GLint v);
void glGetFramebufferParameteriv(GLenum t, GLenum p, GLint* v);
void glTextureView(GLuint t, GLenum tgt, GLuint orig, GLenum fmt, GLuint ml, GLuint nl, GLuint mly, GLuint nly);
void glGetProgramInterfaceiv(GLuint p, GLenum iface, GLenum pn, GLint* v);
GLuint glGetProgramResourceIndex(GLuint p, GLenum iface, const GLchar* n);
void glGetProgramResourceName(GLuint p, GLenum iface, GLuint idx, GLsizei ns, GLsizei* l, GLchar* n);
GLint glGetProgramResourceLocation(GLuint p, GLenum iface, const GLchar* n);
} }

using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_p10_sodium_iris FAIL: %s\n", msg); ++gFails; }
}

int main() {
    // ===== ARB_framebuffer_no_attachments: defaults round-trip =====
    {
        GLuint fb = 0;
        glCreateFramebuffers(1, &fb);
        glBindFramebuffer(0x8D40, fb);
        glFramebufferParameteri(0x8D40, 0x9310 /*DEFAULT_WIDTH*/, 800);
        glFramebufferParameteri(0x8D40, 0x9311 /*DEFAULT_HEIGHT*/, 600);
        Check(glGetError() == 0, "FramebufferParameteri lỗi");
        GLint w = 0, h = 0;
        glGetFramebufferParameteriv(0x8D40, 0x9310, &w);
        glGetFramebufferParameteriv(0x8D40, 0x9311, &h);
        Check(w == 800 && h == 600, "FramebufferParameter round-trip sai");
        // Pname lạ → INVALID_ENUM.
        glFramebufferParameteri(0x8D40, 0x9999, 1);
        Check(glGetError() == 0x0500, "FramebufferParameteri lạ phải INVALID_ENUM");
        glBindFramebuffer(0x8D40, 0);
        glDeleteFramebuffers(1, &fb);
    }
    // ===== ARB_texture_view: view chia sẻ storage (mip subset) =====
    {
        GLuint tex = 0, view = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1, tex);
        unsigned char px[16*16*4] = {0};
        glTexImage2D(0x0DE1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, px);
        glTexImage2D(0x0DE1, 1, 0x8058, 8, 8, 0, 0x1908, 0x1401, px);
        glGenTextures(1, &view);
        // View mip 1 (8x8) của tex.
        glTextureView(view, 0x0DE1, tex, 0x8058, 1, 1, 0, 1);
        Check(glGetError() == 0, "TextureView lỗi");
        // View phải có kích thước 8x8 (mip 1).
        GLint w = 0;
        glBindTexture(0x0DE1, view);
        glGetTexLevelParameteriv(0x0DE1, 0, 0x1000 /*WIDTH*/, &w);
        Check(w == 8, "TextureView size sai (phải 8x8 mip1)");
        // View của view → INVALID_OPERATION.
        GLuint view2 = 0; glGenTextures(1, &view2);
        glTextureView(view2, 0x0DE1, view, 0x8058, 0, 1, 0, 1);
        Check(glGetError() == 0x0502, "TextureView của view phải INVALID_OPERATION");
        glDeleteTextures(1, &tex);
        glDeleteTextures(1, &view);
        glDeleteTextures(1, &view2);
    }
    // ===== ARB_program_interface_query: UNIFORM/BLOCK introspect =====
    {
        const char* vs = "#version 460 core\nlayout(location=0) in vec3 p;\nuniform mat4 mvp;\nvoid main(){ gl_Position = mvp * vec4(p,1); }\n";
        const char* fs = "#version 460 core\nuniform vec4 color;\nout vec4 c;\nvoid main(){ c = color; }\n";
        GLuint shv = glCreateShader(0x8B31), shf = glCreateShader(0x8B30);
        glShaderSource(shv, 1, &vs, nullptr); glCompileShader(shv);
        glShaderSource(shf, 1, &fs, nullptr); glCompileShader(shf);
        GLuint pr = glCreateProgram();
        glAttachShader(pr, shv); glAttachShader(pr, shf); glLinkProgram(pr);
        GLint ok = 0; glGetProgramiv(pr, 0x8B82, &ok);
        Check(ok == 1, "Program link fail");
        // ACTIVE_RESOURCES cho UNIFORM (mvp + color = 2).
        GLint n = 0;
        glGetProgramInterfaceiv(pr, 0x92E1 /*UNIFORM*/, 0x92F5 /*ACTIVE_RESOURCES*/, &n);
        Check(n == 2, "UNIFORM ACTIVE_RESOURCES phải 2");
        // Index by name.
        GLuint idx = glGetProgramResourceIndex(pr, 0x92E1, "mvp");
        Check(idx != 0xFFFFFFFFu, "GetProgramResourceIndex mvp fail");
        GLuint bad = glGetProgramResourceIndex(pr, 0x92E1, "nonexistent");
        Check(bad == 0xFFFFFFFFu, "GetProgramResourceIndex lạ phải INVALID_INDEX");
        // Name by index.
        char name[64] = {0}; GLsizei len = 0;
        glGetProgramResourceName(pr, 0x92E1, idx, sizeof(name), &len, name);
        Check(strcmp(name, "mvp") == 0, "GetProgramResourceName sai");
        // Location by name (phải khớp glGetUniformLocation).
        GLint loc1 = glGetProgramResourceLocation(pr, 0x92E1, "color");
        GLint loc2 = glGetUniformLocation(pr, "color");
        Check(loc1 == loc2 && loc1 >= 0, "GetProgramResourceLocation không khớp UniformLocation");
        glDeleteProgram(pr);
    }

    if (gFails == 0) { printf("test_p10_sodium_iris: all OK\n"); return 0; }
    printf("test_p10_sodium_iris: %d FAIL\n", gFails);
    return 1;
}
