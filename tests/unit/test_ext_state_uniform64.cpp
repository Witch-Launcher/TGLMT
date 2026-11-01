// test_ext_state_uniform64.cpp — stateful glcore extension entries + 64-bit uniforms.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdint>
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

extern "C" {
void glTextureStorage2DEXT(GLuint, GLsizei, GLenum, GLsizei, GLsizei);
void glWindowRectanglesEXT(GLenum, GLsizei, const GLint*);
void glRasterSamplesEXT(GLuint, GLboolean);
void glCoverageModulationTableNV(GLsizei, const GLfloat*);
void glGetCoverageModulationTableNV(GLsizei, GLfloat*);
void glBlendParameteriNV(GLenum, GLint);
void glClearDepthdNV(GLdouble);
void glDepthRangedNV(GLdouble, GLdouble);
void glDepthBoundsdNV(GLdouble, GLdouble);
void glBufferPageCommitmentARB(GLenum, GLintptr, GLsizeiptr, GLboolean);
void glProgramUniform1i64ARB(GLuint, GLint, GLint64);
void glGetUniformi64vARB(GLuint, GLint, GLint64*);
void glGetnUniformi64vARB(GLuint, GLint, GLsizei, GLint64*);
void glVertexAttribL1i64NV(GLuint, GLint64);
void glGetVertexAttribLi64vNV(GLuint, GLenum, GLint64*);
void glVertexAttribLFormatNV(GLuint, GLint, GLenum, GLsizei);
}

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_ext_state_uniform64 FAIL: %s\n", m); ++Fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTextureStorage2DEXT(tex, 1, GL_RGBA8, 32, 16);
    Check(glGetError() == 0, "texture storage EXT");
    GLint w = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    Check(w == 32, "texture storage EXT width");

    GLint rects[8] = {0, 0, 8, 8, 8, 8, 8, 8};
    glWindowRectanglesEXT(0x82F2, 2, rects);
    Check(glGetError() == 0, "window rectangles inclusive");
    glWindowRectanglesEXT(0x1234, 2, rects);
    Check(glGetError() == GL_INVALID_ENUM, "window rectangles mode");

    glRasterSamplesEXT(4, GL_TRUE);
    Check(glGetError() == 0, "raster samples");
    glRasterSamplesEXT(33, GL_TRUE);
    Check(glGetError() == GL_INVALID_VALUE, "raster samples limit");

    GLfloat table[4] = {0.0f, 1.0f, 0.5f, 0.25f};
    glCoverageModulationTableNV(4, table);
    GLfloat got[4] = {};
    glGetCoverageModulationTableNV(4, got);
    Check(got[0] == 0.0f && got[3] == 0.25f, "coverage modulation table");
    glBlendParameteriNV(0x9283, 1);
    Check(glGetError() == 0, "blend parameter");
    glBlendParameteriNV(0x9283, 2);
    Check(glGetError() == GL_INVALID_VALUE, "blend parameter value");

    glClearDepthdNV(0.25);
    glDepthRangedNV(0.1, 0.9);
    glDepthBoundsdNV(0.2, 0.8);
    Check(glGetError() == 0, "depth aliases");
    glDepthBoundsdNV(0.9, 0.1);
    Check(glGetError() == GL_INVALID_VALUE, "depth bounds order");

    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    glBufferPageCommitmentARB(GL_ARRAY_BUFFER, 0, 64, GL_TRUE);
    Check(glGetError() == 0, "page commitment valid");
    glBufferPageCommitmentARB(GL_ARRAY_BUFFER, 32, 64, GL_TRUE);
    Check(glGetError() == GL_INVALID_VALUE, "page commitment range");

    GLuint prog = glCreateProgram();
    glProgramUniform1i64ARB(prog, 3, (GLint64)0x1FFFFFFFFFFFFFLL);
    Check(glGetError() == 0, "program uniform int64");
    GLint64 out = 0;
    glGetUniformi64vARB(prog, 3, &out);
    Check(out == (GLint64)0x1FFFFFFFFFFFFFLL, "int64 precision");
    out = 0;
    glGetnUniformi64vARB(prog, 3, 8, &out);
    Check(out == (GLint64)0x1FFFFFFFFFFFFFLL, "getn int64");
    out = 0;
    glGetnUniformi64vARB(prog, 3, 4, &out);
    Check(glGetError() == GL_INVALID_OPERATION, "getn int64 short buffer");

    glVertexAttribL1i64NV(5, (GLint64)0x7FFFFFFFFFFFFFFFLL);
    GLint64 attr = 0;
    glGetVertexAttribLi64vNV(5, GL_CURRENT_VERTEX_ATTRIB, &attr);
    Check(attr == (GLint64)0x7FFFFFFFFFFFFFFFLL, "vertex attrib int64");

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribLFormatNV(0, 2, 0x140E, 16);
    Check(glGetError() == 0, "vertex attrib L format NV");
    glVertexAttribLFormatNV(0, 2, 0x1406, 16);
    Check(glGetError() == GL_INVALID_ENUM, "vertex attrib L format bad type");

    glDeleteProgram(prog);
    glDeleteBuffers(1, &buf);
    glDeleteTextures(1, &tex);
    glDeleteVertexArrays(1, &vao);
    return Fails ? 1 : 0;
}
