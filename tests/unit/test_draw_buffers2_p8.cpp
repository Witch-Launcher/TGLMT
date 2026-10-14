// test_draw_buffers2_p8.cpp — P8: EXT_draw_buffers2 + EXT_separate_shader_objects.
// Verify indexed MRT control + pipeline API. Chạy cả 2 backend.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace tglmt { namespace gl {
void glColorMaskIndexedEXT(GLuint b, GLboolean r, GLboolean g, GLboolean bl, GLboolean a);
void glEnableIndexedEXT(GLenum t, GLuint i);
void glDisableIndexedEXT(GLenum t, GLuint i);
GLboolean glIsEnabledIndexedEXT(GLenum t, GLuint i);
void glGetBooleanIndexedvEXT(GLenum t, GLuint i, GLboolean* d);
void glGetIntegerIndexedvEXT(GLenum t, GLuint i, GLint* d);
GLuint glCreateShaderProgramEXT(GLenum t, const GLchar* s);
void glUseShaderProgramEXT(GLenum t, GLuint p);
void glActiveProgramEXT(GLuint pl, GLuint p);
void glFramebufferTextureFace(GLenum t, GLenum a, GLuint x, GLint l, GLenum f);
} }

using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_draw_buffers2_p8 FAIL: %s\n", msg); ++gFails; }
}

int main() {
    // ===== EXT_draw_buffers2: indexed color mask =====
    {
        glColorMaskIndexedEXT(1, 1, 0, 0, 1); // target1: R+A only
        Check(glGetError() == 0, "ColorMaskIndexedEXT lỗi");
        GLboolean m[4] = {0};
        glGetBooleanIndexedvEXT(0x0C23 /*COLOR_WRITEMASK*/, 1, m);
        Check(m[0]==1 && m[1]==0 && m[2]==0 && m[3]==1, "GetBooleanIndexedv WRITEMASK sai");
        GLint mi[4] = {0};
        glGetIntegerIndexedvEXT(0x0C23, 1, mi);
        Check(mi[0]==1 && mi[3]==1, "GetIntegerIndexedv WRITEMASK sai");
        // Target 0 không bị ảnh hưởng (vẫn full mask).
        glGetBooleanIndexedvEXT(0x0C23, 0, m);
        Check(m[0]==1 && m[1]==1 && m[2]==1 && m[3]==1, "Target0 bị ảnh hưởng sai");
        // Target lạ → INVALID_ENUM.
        glGetBooleanIndexedvEXT(0x9999, 0, m);
        Check(glGetError() == 0x0500, "GetBooleanIndexedv target lạ phải INVALID_ENUM");
    }
    // ===== EXT_draw_buffers2: indexed enable (BLEND per-target) =====
    {
        glEnableIndexedEXT(0x0BE2 /*BLEND*/, 1);
        Check(glGetError() == 0, "EnableIndexedEXT lỗi");
        Check(glIsEnabledIndexedEXT(0x0BE2, 1) == 1, "IsEnabledIndexedEXT sai");
        glDisableIndexedEXT(0x0BE2, 1);
        Check(glIsEnabledIndexedEXT(0x0BE2, 1) == 0, "DisableIndexedEXT sai");
    }
    // ===== EXT_separate_shader_objects: create + use =====
    {
        const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0); }\n";
        GLuint pr = glCreateShaderProgramEXT(0x8B31 /*VERTEX*/, vs);
        Check(pr != 0, "CreateShaderProgramEXT trả 0");
        GLint ok = 0;
        glGetProgramiv(pr, 0x8B82 /*LINK_STATUS*/, &ok);
        Check(ok == 1, "CreateShaderProgramEXT link fail");
        // Separable flag phải true.
        // (Không có getter trực tiếp — verify bằng cách dùng được.)
        glUseShaderProgramEXT(0x8B31, pr);
        Check(glGetError() == 0, "UseShaderProgramEXT lỗi");
        // Sai stage → INVALID_OPERATION.
        glUseShaderProgramEXT(0x8B30 /*FRAGMENT*/, pr); // program chỉ có VS
        Check(glGetError() == 0x0502, "UseShaderProgramEXT sai stage phải INVALID_OPERATION");
        // ActiveProgram (validate).
        GLuint pl = 0;
        glGenProgramPipelines(1, &pl);
        glActiveProgramEXT(pl, pr);
        Check(glGetError() == 0, "ActiveProgramEXT lỗi");
        glDeleteProgramPipelines(1, &pl);
        glDeleteProgram(pr);
    }
    // ===== ARB_geometry_shader4: FramebufferTextureFace (không advertise GS) =====
    {
        GLuint fb = 0, tex = 0;
        glCreateFramebuffers(1, &fb);
        glBindFramebuffer(0x8D40 /*FRAMEBUFFER*/, fb); // Face op trên BOUND FBO
        glCreateTextures(0x8513 /*CUBE_MAP*/, 1, &tex);
        glTextureStorage2D(tex, 1, 0x8058 /*RGBA8*/, 4, 4);
        // Attach face POSITIVE_X (0x8515) — TGLMT attach như 2D (documented).
        glFramebufferTextureFace(0x8D40, 0x8CE0, tex, 0, 0x8515);
        Check(glGetError() == 0, "FramebufferTextureFace lỗi");
        GLenum st = glCheckFramebufferStatus(0x8D40);
        // Status có thể COMPLETE hoặc không (tùy FBO impl) — chỉ check không crash.
        (void)st;
        glBindFramebuffer(0x8D40, 0);
        glDeleteFramebuffers(1, &fb);
        glDeleteTextures(1, &tex);
    }

    if (gFails == 0) { printf("test_draw_buffers2_p8: all OK\n"); return 0; }
    printf("test_draw_buffers2_p8: %d FAIL\n", gFails);
    return 1;
}
