// test_modern_ext_objects.cpp — DSA, bindless, imaging, perf, shading, include,
// path and command-list object behavior.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

extern "C" {
void glTextureImage2DEXT(GLuint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void glGetTextureLevelParameterivEXT(GLuint, GLenum, GLint, GLenum, GLint*);
void glBindMultiTextureEXT(GLenum, GLenum, GLuint);
void glMultiTexImage2DEXT(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void glEnableClientStateIndexedEXT(GLenum, GLuint);
void glMultiTexCoordPointerEXT(GLenum, GLint, GLenum, GLsizei, const void*);
void glGetPointerIndexedvEXT(GLenum, GLuint, void**);
void glMatrixLoadIdentityEXT(GLenum);
void glMatrixPushEXT(GLenum);
void glMatrixTranslatefEXT(GLenum, GLfloat, GLfloat, GLfloat);
void glMatrixPopEXT(GLenum);

GLuint64 glGetTextureHandleARB(GLuint);
GLuint64 glGetImageHandleARB(GLuint, GLint, GLboolean, GLint, GLenum);
GLboolean glIsTextureHandleResidentARB(GLuint64);
void glMakeTextureHandleResidentARB(GLuint64);
void glMakeTextureHandleNonResidentARB(GLuint64);

void glColorTable(GLenum, GLenum, GLsizei, GLenum, GLenum, const void*);
void glColorTableParameterfv(GLenum, GLenum, const GLfloat*);
void glGetColorTableParameteriv(GLenum, GLenum, GLint*);
void glGetColorTableParameterfv(GLenum, GLenum, GLfloat*);
void glConvolutionParameteri(GLenum, GLenum, GLint);
void glGetConvolutionParameteriv(GLenum, GLenum, GLint*);
void glHistogram(GLenum, GLsizei, GLenum, GLboolean);
void glGetHistogramParameteriv(GLenum, GLenum, GLint*);
void glMinmax(GLenum, GLenum, GLboolean);
void glGetMinmaxParameteriv(GLenum, GLenum, GLint*);

void glGenPerfMonitorsAMD(GLsizei, GLuint*);
void glBeginPerfMonitorAMD(GLuint);
void glEndPerfMonitorAMD(GLuint);
void glGetPerfMonitorCounterDataAMD(GLuint, GLenum, GLsizei, GLuint*, GLint*);
void glDeletePerfMonitorsAMD(GLsizei, GLuint*);
void glCreatePerfQueryINTEL(GLuint, GLuint*);
void glBeginPerfQueryINTEL(GLuint);
void glEndPerfQueryINTEL(GLuint);
void glDeletePerfQueryINTEL(GLuint);

void glShadingRateEXT(GLenum);
void glShadingRateCombinerOpsEXT(GLenum, GLenum);
void glGetFragmentShadingRatesEXT(GLsizei, GLsizei, GLsizei*, GLenum*);
void glNamedStringARB(GLenum, GLint, const GLchar*, GLint, const GLchar*);
GLboolean glIsNamedStringARB(GLint, const GLchar*);
void glGetNamedStringARB(GLint, const GLchar*, GLsizei, GLint*, GLchar*);
void glGetNamedStringivARB(GLint, const GLchar*, GLenum, GLint*);
void glDeleteNamedStringARB(GLint, const GLchar*);

GLuint glGenPathsNV(GLsizei);
void glDeletePathsNV(GLuint, GLsizei);
GLboolean glIsPathNV(GLuint);
void glStencilFillPathNV(GLuint, GLenum, GLuint);
void glCreateCommandListsNV(GLsizei, GLuint*);
void glDeleteCommandListsNV(GLsizei, const GLuint*);
GLboolean glIsCommandListNV(GLuint);
void glCreateStatesNV(GLsizei, GLuint*);
void glDeleteStatesNV(GLsizei, const GLuint*);
GLboolean glIsStateNV(GLuint);
void glCallCommandListNV(GLuint);
}

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_modern_ext_objects FAIL: %s\n", m); ++Fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint tex = 0, tex2 = 0;
    glGenTextures(1, &tex);
    glGenTextures(1, &tex2);
    glTextureImage2DEXT(tex, GL_TEXTURE_2D, 0, GL_RGBA8, 8, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    Check(glGetError() == 0, "DSA texture image");
    GLint w = 0;
    glGetTextureLevelParameterivEXT(tex, GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    Check(w == 8, "DSA texture width");
    glBindMultiTextureEXT(GL_TEXTURE0 + 1, GL_TEXTURE_2D, tex2);
    Check(glGetError() == 0, "DSA bind multi texture");
    GLint bound = 0;
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    Check(bound == (GLint)tex2, "DSA multi texture binding");

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
    glEnableClientStateIndexedEXT(0x8078, 2);
    glMultiTexCoordPointerEXT(GL_TEXTURE0 + 2, 4, GL_FLOAT, 0, (const void*)(uintptr_t)0x1000);
    void* ptr = nullptr;
    glGetPointerIndexedvEXT(0x8078, 2, &ptr);
    Check(ptr == (void*)(uintptr_t)0x1000, "DSA indexed client pointer");
    glMatrixLoadIdentityEXT(0x1701);
    glMatrixPushEXT(0x1701);
    glMatrixTranslatefEXT(0x1701, 1.0f, 2.0f, 0.0f);
    glMatrixPopEXT(0x1701);
    Check(glGetError() == 0, "DSA matrix stack");

    GLuint64 handle = glGetTextureHandleARB(tex);
    Check(handle != 0, "bindless texture handle");
    Check(!glIsTextureHandleResidentARB(handle), "bindless starts non-resident");
    glMakeTextureHandleResidentARB(handle);
    Check(glIsTextureHandleResidentARB(handle), "bindless resident");
    glMakeTextureHandleNonResidentARB(handle);
    Check(!glIsTextureHandleResidentARB(handle), "bindless non-resident");
    GLuint64 image = glGetImageHandleARB(tex, 0, GL_TRUE, 0, GL_RGBA8);
    Check(image != 0, "bindless image handle");

    const GLubyte table[8] = {0,0,0,255, 255,255,255,255};
    glColorTable(0x80D0, GL_RGBA8, 2, GL_RGBA, GL_UNSIGNED_BYTE, table);
    Check(glGetError() == 0, "imaging color table");
    GLint width = 0;
    glGetColorTableParameteriv(0x80D0, 0x80D9, &width);
    Check(width == 2, "imaging color table width");
    GLfloat scale[4] = {2.0f, 2.0f, 2.0f, 2.0f};
    glColorTableParameterfv(0x80D0, 0x80D6, scale);
    GLfloat gotScale[4] = {};
    glGetColorTableParameterfv(0x80D0, 0x80D6, gotScale);
    Check(gotScale[0] == 2.0f, "imaging color table scale");
    glConvolutionParameteri(0x8011, 0x8013, 0x8151);
    GLint borderMode = 0;
    glGetConvolutionParameteriv(0x8011, 0x8013, &borderMode);
    Check(borderMode == 0x8151, "imaging convolution border mode");
    glHistogram(0x8024, 64, GL_RGBA8, GL_TRUE);
    GLint histWidth = 0;
    glGetHistogramParameteriv(0x8024, 0x8026, &histWidth);
    Check(histWidth == 64, "imaging histogram width");
    glMinmax(0x802E, GL_RGBA8, GL_FALSE);
    GLint minmaxFormat = 0;
    glGetMinmaxParameteriv(0x802E, 0x802F, &minmaxFormat);
    Check(minmaxFormat == GL_RGBA8, "imaging minmax format");

    GLuint monitor = 0;
    glGenPerfMonitorsAMD(1, &monitor);
    Check(monitor != 0, "AMD perf monitor id");
    glBeginPerfMonitorAMD(monitor);
    glEndPerfMonitorAMD(monitor);
    GLuint data[2] = {1, 2};
    GLint written = -1;
    glGetPerfMonitorCounterDataAMD(monitor, 0x8B65, 2, data, &written);
    Check(glGetError() == 0 && data[0] == 0 && written == 0, "AMD perf counter data");
    glDeletePerfMonitorsAMD(1, &monitor);
    GLuint query = 0;
    glCreatePerfQueryINTEL(1, &query);
    Check(query != 0, "INTEL perf query id");
    glBeginPerfQueryINTEL(query);
    glEndPerfQueryINTEL(query);
    Check(glGetError() == 0, "INTEL perf query begin/end");
    glDeletePerfQueryINTEL(query);

    glShadingRateEXT(0x96A6);
    Check(glGetError() == 0, "shading rate");
    glShadingRateEXT(GL_TEXTURE_2D);
    Check(glGetError() == GL_INVALID_ENUM, "shading rate invalid");
    glShadingRateCombinerOpsEXT(0x96D2, 0x96D3);
    Check(glGetError() == 0, "shading rate combiner");
    GLsizei rateCount = 0;
    GLenum rates[9] = {};
    glGetFragmentShadingRatesEXT(1, 9, &rateCount, rates);
    Check(rateCount == 9 && rates[0] == 0x96A6, "fragment shading rates");

    const GLchar* name = "/tglmt.glsl";
    const GLchar* body = "void main(){}";
    glNamedStringARB(0x8DAE, -1, name, -1, body);
    Check(glIsNamedStringARB(-1, name), "named string exists");
    GLint len = 0;
    GLchar buf[32] = {};
    glGetNamedStringARB(-1, name, 32, &len, buf);
    Check(len == (GLint)strlen(body) && strcmp(buf, body) == 0, "named string content");
    GLint nlen = 0;
    glGetNamedStringivARB(-1, name, 0x8DE9, &nlen);
    Check(nlen == (GLint)strlen(body) + 1, "named string length includes null");
    glDeleteNamedStringARB(-1, name);
    Check(!glIsNamedStringARB(-1, name), "named string deleted");
    glDeleteNamedStringARB(-1, name);
    Check(glGetError() == GL_INVALID_OPERATION, "delete missing named string");

    GLuint paths = glGenPathsNV(3);
    Check(paths != 0, "path range");
    Check(glIsPathNV(paths) && glIsPathNV(paths + 2), "path range members");
    glStencilFillPathNV(paths, 0x0B00, 0xFF);
    Check(glGetError() == GL_INVALID_OPERATION, "path rendering unsupported");
    glDeletePathsNV(paths, 3);
    Check(!glIsPathNV(paths), "path range deleted");

    GLuint lists[2] = {}, states[2] = {};
    glCreateCommandListsNV(2, lists);
    glCreateStatesNV(2, states);
    Check(lists[0] && lists[1] && glIsCommandListNV(lists[0]), "command lists");
    Check(states[0] && states[1] && glIsStateNV(states[1]), "command states");
    glCallCommandListNV(lists[0]);
    Check(glGetError() == GL_INVALID_OPERATION, "command list execution unsupported");
    glDeleteCommandListsNV(2, lists);
    glDeleteStatesNV(2, states);
    Check(!glIsCommandListNV(lists[0]) && !glIsStateNV(states[0]), "command objects deleted");

    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &tex);
    glDeleteTextures(1, &tex2);
    return Fails ? 1 : 0;
}
