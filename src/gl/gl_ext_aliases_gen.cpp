// AUTO-GENERATED bởi tools/gen_ext_aliases.py — KHÔNG SỬA TAY.
// Alias extension functions: extern "C" forward 1:1 sang core-đã-implement.
// Chỉ sinh khi signature gl.xml TRÙNG core-base (trung thực, không đoán).
// LWJGL dlsym("glXxxARB") thấy symbol này khi extension được advertise.
#include "tglmt/gl46.h"
using namespace tglmt;
namespace tglmt { namespace gl {
GLenum glClientWaitSync(GLsync a0, GLbitfield a1, GLuint64 a2);
void glDeleteSync(GLsync a0);
GLsync glFenceSync(GLenum a0, GLbitfield a1);
void glGetInteger64v(GLenum a0, GLint64 * a1);
void glGetSynciv(GLsync a0, GLenum a1, GLsizei a2, GLsizei * a3, GLint * a4);
GLboolean glIsSync(GLsync a0);
void glWaitSync(GLsync a0, GLbitfield a1, GLuint64 a2);
void glBindVertexArray(GLuint a0);
void glDeleteVertexArrays(GLsizei a0, const GLuint * a1);
void glGenVertexArrays(GLsizei a0, GLuint * a1);
GLboolean glIsVertexArray(GLuint a0);
void glClampColor(GLenum a0, GLenum a1);
void glDebugMessageControl(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, const GLuint * a4, GLboolean a5);
void glDebugMessageInsert(GLenum a0, GLenum a1, GLuint a2, GLenum a3, GLsizei a4, const GLchar * a5);
GLuint glGetDebugMessageLog(GLuint a0, GLsizei a1, GLenum * a2, GLenum * a3, GLuint * a4, GLenum * a5, GLsizei * a6, GLchar * a7);
void glDrawBuffers(GLsizei a0, const GLenum * a1);
void glBlendEquationSeparatei(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationi(GLuint a0, GLenum a1);
void glBlendFuncSeparatei(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFunci(GLuint a0, GLenum a1, GLenum a2);
void glDrawArraysInstanced(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawElementsInstanced(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glSpecializeShader(GLuint a0, const GLchar * a1, GLuint a2, const GLuint * a3, const GLuint * a4);
void glMultiDrawArraysIndirectCount(GLenum a0, const void * a1, GLintptr a2, GLsizei a3, GLsizei a4);
void glMultiDrawElementsIndirectCount(GLenum a0, GLenum a1, const void * a2, GLintptr a3, GLsizei a4, GLsizei a5);
void glVertexAttribDivisor(GLuint a0, GLuint a1);
void glSampleCoverage(GLfloat a0, GLboolean a1);
void glBeginQuery(GLenum a0, GLuint a1);
void glDeleteQueries(GLsizei a0, const GLuint * a1);
void glEndQuery(GLenum a0);
void glGenQueries(GLsizei a0, GLuint * a1);
void glGetQueryObjectiv(GLuint a0, GLenum a1, GLint * a2);
void glGetQueryObjectuiv(GLuint a0, GLenum a1, GLuint * a2);
void glGetQueryiv(GLenum a0, GLenum a1, GLint * a2);
GLboolean glIsQuery(GLuint a0);
void glPointParameterf(GLenum a0, GLfloat a1);
void glPointParameterfv(GLenum a0, const GLfloat * a1);
GLenum glGetGraphicsResetStatus(void);
void glGetnColorTable(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4);
void glGetnCompressedTexImage(GLenum a0, GLint a1, GLsizei a2, void * a3);
void glGetnConvolutionFilter(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4);
void glGetnHistogram(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnMapdv(GLenum a0, GLenum a1, GLsizei a2, GLdouble * a3);
void glGetnMapfv(GLenum a0, GLenum a1, GLsizei a2, GLfloat * a3);
void glGetnMapiv(GLenum a0, GLenum a1, GLsizei a2, GLint * a3);
void glGetnMinmax(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnPixelMapfv(GLenum a0, GLsizei a1, GLfloat * a2);
void glGetnPixelMapuiv(GLenum a0, GLsizei a1, GLuint * a2);
void glGetnPixelMapusv(GLenum a0, GLsizei a1, GLushort * a2);
void glGetnPolygonStipple(GLsizei a0, GLubyte * a1);
void glGetnSeparableFilter(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4, GLsizei a5, void * a6, void * a7);
void glGetnTexImage(GLenum a0, GLint a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnUniformdv(GLuint a0, GLint a1, GLsizei a2, GLdouble * a3);
void glGetnUniformfv(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformiv(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glGetnUniformuiv(GLuint a0, GLint a1, GLsizei a2, GLuint * a3);
void glReadnPixels(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glMinSampleShading(GLfloat a0);
void glTexBuffer(GLenum a0, GLenum a1, GLuint a2);
void glCompressedTexImage1D(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLint a4, GLsizei a5, const void * a6);
void glCompressedTexImage2D(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLint a5, GLsizei a6, const void * a7);
void glCompressedTexImage3D(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8);
void glCompressedTexSubImage1D(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLsizei a5, const void * a6);
void glCompressedTexSubImage2D(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLsizei a7, const void * a8);
void glCompressedTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10);
void glGetCompressedTexImage(GLenum a0, GLint a1, void * a2);
void glBindBuffer(GLenum a0, GLuint a1);
void glDeleteBuffers(GLsizei a0, const GLuint * a1);
void glGenBuffers(GLsizei a0, GLuint * a1);
void glGetBufferParameteriv(GLenum a0, GLenum a1, GLint * a2);
void glGetBufferPointerv(GLenum a0, GLenum a1, void ** a2);
GLboolean glIsBuffer(GLuint a0);
void * glMapBuffer(GLenum a0, GLenum a1);
GLboolean glUnmapBuffer(GLenum a0);
void glDisableVertexAttribArray(GLuint a0);
void glEnableVertexAttribArray(GLuint a0);
void glGetVertexAttribPointerv(GLuint a0, GLenum a1, void ** a2);
void glGetVertexAttribdv(GLuint a0, GLenum a1, GLdouble * a2);
void glGetVertexAttribfv(GLuint a0, GLenum a1, GLfloat * a2);
void glGetVertexAttribiv(GLuint a0, GLenum a1, GLint * a2);
void glVertexAttrib1d(GLuint a0, GLdouble a1);
void glVertexAttrib1dv(GLuint a0, const GLdouble * a1);
void glVertexAttrib1f(GLuint a0, GLfloat a1);
void glVertexAttrib1fv(GLuint a0, const GLfloat * a1);
void glVertexAttrib1s(GLuint a0, GLshort a1);
void glVertexAttrib1sv(GLuint a0, const GLshort * a1);
void glVertexAttrib2d(GLuint a0, GLdouble a1, GLdouble a2);
void glVertexAttrib2dv(GLuint a0, const GLdouble * a1);
void glVertexAttrib2f(GLuint a0, GLfloat a1, GLfloat a2);
void glVertexAttrib2fv(GLuint a0, const GLfloat * a1);
void glVertexAttrib2s(GLuint a0, GLshort a1, GLshort a2);
void glVertexAttrib2sv(GLuint a0, const GLshort * a1);
void glVertexAttrib3d(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3);
void glVertexAttrib3dv(GLuint a0, const GLdouble * a1);
void glVertexAttrib3f(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glVertexAttrib3fv(GLuint a0, const GLfloat * a1);
void glVertexAttrib3s(GLuint a0, GLshort a1, GLshort a2, GLshort a3);
void glVertexAttrib3sv(GLuint a0, const GLshort * a1);
void glVertexAttrib4Nbv(GLuint a0, const GLbyte * a1);
void glVertexAttrib4Niv(GLuint a0, const GLint * a1);
void glVertexAttrib4Nsv(GLuint a0, const GLshort * a1);
void glVertexAttrib4Nub(GLuint a0, GLubyte a1, GLubyte a2, GLubyte a3, GLubyte a4);
void glVertexAttrib4Nubv(GLuint a0, const GLubyte * a1);
void glVertexAttrib4Nuiv(GLuint a0, const GLuint * a1);
void glVertexAttrib4Nusv(GLuint a0, const GLushort * a1);
void glVertexAttrib4bv(GLuint a0, const GLbyte * a1);
void glVertexAttrib4d(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glVertexAttrib4dv(GLuint a0, const GLdouble * a1);
void glVertexAttrib4f(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glVertexAttrib4fv(GLuint a0, const GLfloat * a1);
void glVertexAttrib4iv(GLuint a0, const GLint * a1);
void glVertexAttrib4s(GLuint a0, GLshort a1, GLshort a2, GLshort a3, GLshort a4);
void glVertexAttrib4sv(GLuint a0, const GLshort * a1);
void glVertexAttrib4ubv(GLuint a0, const GLubyte * a1);
void glVertexAttrib4uiv(GLuint a0, const GLuint * a1);
void glVertexAttrib4usv(GLuint a0, const GLushort * a1);
void glVertexAttribPointer(GLuint a0, GLint a1, GLenum a2, GLboolean a3, GLsizei a4, const void * a5);
void glDrawArraysInstancedBaseInstance(GLenum a0, GLint a1, GLsizei a2, GLsizei a3, GLuint a4);
void glDrawElementsInstancedBaseInstance(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLuint a5);
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5, GLuint a6);
void glBlendColor(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glBlendEquationSeparate(GLenum a0, GLenum a1);
void glBindFragDataLocation(GLuint a0, GLuint a1, const GLchar * a2);
void glBindFragDataLocationIndexed(GLuint a0, GLuint a1, GLuint a2, const GLchar * a3);
GLint glGetFragDataIndex(GLuint a0, const GLchar * a1);
GLint glGetProgramResourceLocationIndex(GLuint a0, GLenum a1, const GLchar * a2);
void glBlendFuncSeparate(GLenum a0, GLenum a1, GLenum a2, GLenum a3);
void glBlendEquation(GLenum a0);
void glBufferStorage(GLenum a0, GLsizeiptr a1, const void * a2, GLbitfield a3);
void glClearTexImage(GLuint a0, GLint a1, GLenum a2, GLenum a3, const void * a4);
void glClearTexSubImage(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glClipControl(GLenum a0, GLenum a1);
void glCopyImageSubData(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14);
void glCopyTexImage1D(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLint a6);
void glCopyTexImage2D(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLint a7);
void glCopyTexSubImage1D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5);
void glCopyTexSubImage2D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLsizei a6, GLsizei a7);
void glCopyTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8);
void glBeginQuery(GLenum a0, GLuint a1);
void glDeleteQueries(GLsizei a0, const GLuint * a1);
void glEndQuery(GLenum a0);
void glGenQueries(GLsizei a0, GLuint * a1);
void glGetInteger64v(GLenum a0, GLint64 * a1);
void glGetQueryObjecti64v(GLuint a0, GLenum a1, GLint64 * a2);
void glGetQueryObjectiv(GLuint a0, GLenum a1, GLint * a2);
void glGetQueryObjectui64v(GLuint a0, GLenum a1, GLuint64 * a2);
void glGetQueryObjectuiv(GLuint a0, GLenum a1, GLuint * a2);
void glGetQueryiv(GLenum a0, GLenum a1, GLint * a2);
GLboolean glIsQuery(GLuint a0);
void glQueryCounter(GLuint a0, GLenum a1);
void glDrawBuffers(GLsizei a0, const GLenum * a1);
void glBlendEquationSeparatei(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationi(GLuint a0, GLenum a1);
void glBlendFuncSeparatei(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFunci(GLuint a0, GLenum a1, GLenum a2);
void glColorMaski(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4);
void glDisablei(GLenum a0, GLuint a1);
void glEnablei(GLenum a0, GLuint a1);
GLboolean glIsEnabledi(GLenum a0, GLuint a1);
void glDrawElementsBaseVertex(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4);
void glDrawElementsInstancedBaseVertex(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5);
void glDrawRangeElementsBaseVertex(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6);
void glMultiDrawElementsBaseVertex(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4, const GLint * a5);
void glDrawArraysInstanced(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawElementsInstanced(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glDrawRangeElements(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5);
void glDrawTransformFeedback(GLenum a0, GLuint a1);
void glDrawTransformFeedbackInstanced(GLenum a0, GLuint a1, GLsizei a2);
void glBlitFramebuffer(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9);
void glRenderbufferStorageMultisample(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glBindFramebuffer(GLenum a0, GLuint a1);
void glBindRenderbuffer(GLenum a0, GLuint a1);
GLenum glCheckFramebufferStatus(GLenum a0);
void glDeleteFramebuffers(GLsizei a0, const GLuint * a1);
void glDeleteRenderbuffers(GLsizei a0, const GLuint * a1);
void glFramebufferRenderbuffer(GLenum a0, GLenum a1, GLenum a2, GLuint a3);
void glFramebufferTexture1D(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glFramebufferTexture2D(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glFramebufferTexture3D(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5);
void glGenFramebuffers(GLsizei a0, GLuint * a1);
void glGenRenderbuffers(GLsizei a0, GLuint * a1);
void glGenerateMipmap(GLenum a0);
void glGetFramebufferAttachmentParameteriv(GLenum a0, GLenum a1, GLenum a2, GLint * a3);
void glGetRenderbufferParameteriv(GLenum a0, GLenum a1, GLint * a2);
GLboolean glIsFramebuffer(GLuint a0);
GLboolean glIsRenderbuffer(GLuint a0);
void glRenderbufferStorage(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3);
void glFramebufferTexture(GLenum a0, GLenum a1, GLuint a2, GLint a3);
void glProgramParameteri(GLuint a0, GLenum a1, GLint a2);
void glBindFragDataLocation(GLuint a0, GLuint a1, const GLchar * a2);
GLint glGetFragDataLocation(GLuint a0, const GLchar * a1);
void glGetUniformuiv(GLuint a0, GLint a1, GLuint * a2);
void glGetVertexAttribIiv(GLuint a0, GLenum a1, GLint * a2);
void glGetVertexAttribIuiv(GLuint a0, GLenum a1, GLuint * a2);
void glUniform1ui(GLint a0, GLuint a1);
void glUniform1uiv(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform2ui(GLint a0, GLuint a1, GLuint a2);
void glUniform2uiv(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform3ui(GLint a0, GLuint a1, GLuint a2, GLuint a3);
void glUniform3uiv(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform4ui(GLint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4);
void glUniform4uiv(GLint a0, GLsizei a1, const GLuint * a2);
void glVertexAttribI1i(GLuint a0, GLint a1);
void glVertexAttribI1iv(GLuint a0, const GLint * a1);
void glVertexAttribI1ui(GLuint a0, GLuint a1);
void glVertexAttribI1uiv(GLuint a0, const GLuint * a1);
void glVertexAttribI2i(GLuint a0, GLint a1, GLint a2);
void glVertexAttribI2iv(GLuint a0, const GLint * a1);
void glVertexAttribI2ui(GLuint a0, GLuint a1, GLuint a2);
void glVertexAttribI2uiv(GLuint a0, const GLuint * a1);
void glVertexAttribI3i(GLuint a0, GLint a1, GLint a2, GLint a3);
void glVertexAttribI3iv(GLuint a0, const GLint * a1);
void glVertexAttribI3ui(GLuint a0, GLuint a1, GLuint a2, GLuint a3);
void glVertexAttribI3uiv(GLuint a0, const GLuint * a1);
void glVertexAttribI4bv(GLuint a0, const GLbyte * a1);
void glVertexAttribI4i(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4);
void glVertexAttribI4iv(GLuint a0, const GLint * a1);
void glVertexAttribI4sv(GLuint a0, const GLshort * a1);
void glVertexAttribI4ubv(GLuint a0, const GLubyte * a1);
void glVertexAttribI4ui(GLuint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4);
void glVertexAttribI4uiv(GLuint a0, const GLuint * a1);
void glVertexAttribI4usv(GLuint a0, const GLushort * a1);
void glVertexAttribIPointer(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4);
void glDrawArraysInstanced(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawElementsInstanced(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glVertexAttribDivisor(GLuint a0, GLuint a1);
void glFlushMappedBufferRange(GLenum a0, GLintptr a1, GLsizeiptr a2);
void * glMapBufferRange(GLenum a0, GLintptr a1, GLsizeiptr a2, GLbitfield a3);
void glMultiDrawArrays(GLenum a0, const GLint * a1, const GLsizei * a2, GLsizei a3);
void glMultiDrawElements(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4);
void glMultiDrawArraysIndirect(GLenum a0, const void * a1, GLsizei a2, GLsizei a3);
void glMultiDrawElementsIndirect(GLenum a0, GLenum a1, const void * a2, GLsizei a3, GLsizei a4);
void glBeginQuery(GLenum a0, GLuint a1);
void glDeleteQueries(GLsizei a0, const GLuint * a1);
void glEndQuery(GLenum a0);
void glGenQueries(GLsizei a0, GLuint * a1);
void glGetQueryObjectuiv(GLuint a0, GLenum a1, GLuint * a2);
void glGetQueryiv(GLenum a0, GLenum a1, GLint * a2);
GLboolean glIsQuery(GLuint a0);
void glPointParameterf(GLenum a0, GLfloat a1);
void glPointParameterfv(GLenum a0, const GLfloat * a1);
void glPolygonOffset(GLfloat a0, GLfloat a1);
void glPolygonOffsetClamp(GLfloat a0, GLfloat a1, GLfloat a2);
void glProvokingVertex(GLenum a0);
GLenum glGetGraphicsResetStatus(void);
void glGetnUniformfv(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformiv(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glReadnPixels(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glMemoryBarrier(GLbitfield a0);
void glTexSubImage1D(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLenum a5, const void * a6);
void glTexSubImage2D(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLenum a7, const void * a8);
void glPatchParameteri(GLenum a0, GLint a1);
void glTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glFramebufferTextureLayer(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLint a4);
void glGetSamplerParameterIiv(GLuint a0, GLenum a1, GLint * a2);
void glGetSamplerParameterIuiv(GLuint a0, GLenum a1, GLuint * a2);
void glGetTexParameterIiv(GLenum a0, GLenum a1, GLint * a2);
void glGetTexParameterIuiv(GLenum a0, GLenum a1, GLuint * a2);
void glSamplerParameterIiv(GLuint a0, GLenum a1, const GLint * a2);
void glSamplerParameterIuiv(GLuint a0, GLenum a1, const GLuint * a2);
void glTexParameterIiv(GLenum a0, GLenum a1, const GLint * a2);
void glTexParameterIuiv(GLenum a0, GLenum a1, const GLuint * a2);
void glTexBuffer(GLenum a0, GLenum a1, GLuint a2);
void glTexBufferRange(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glTexBuffer(GLenum a0, GLenum a1, GLuint a2);
void glTexStorage1D(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3);
void glTexStorage2D(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glTexStorage3D(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5);
void glTextureView(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7);
void glGetQueryObjecti64v(GLuint a0, GLenum a1, GLint64 * a2);
void glGetQueryObjectui64v(GLuint a0, GLenum a1, GLuint64 * a2);
void glGetVertexAttribLdv(GLuint a0, GLenum a1, GLdouble * a2);
void glVertexAttribL1d(GLuint a0, GLdouble a1);
void glVertexAttribL1dv(GLuint a0, const GLdouble * a1);
void glVertexAttribL2d(GLuint a0, GLdouble a1, GLdouble a2);
void glVertexAttribL2dv(GLuint a0, const GLdouble * a1);
void glVertexAttribL3d(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3);
void glVertexAttribL3dv(GLuint a0, const GLdouble * a1);
void glVertexAttribL4d(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glVertexAttribL4dv(GLuint a0, const GLdouble * a1);
void glVertexAttribLPointer(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4);
GLenum glGetGraphicsResetStatus(void);
void glGetnUniformfv(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformiv(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glGetnUniformuiv(GLuint a0, GLint a1, GLsizei a2, GLuint * a3);
void glReadnPixels(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glBlendEquationSeparate(GLenum a0, GLenum a1);
void glBlendFuncSeparate(GLenum a0, GLenum a1, GLenum a2, GLenum a3);
void glBlendEquation(GLenum a0);
void glCopyImageSubData(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14);
void glBlendEquationSeparatei(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationi(GLuint a0, GLenum a1);
void glBlendFuncSeparatei(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFunci(GLuint a0, GLenum a1, GLenum a2);
void glColorMaski(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4);
void glDisablei(GLenum a0, GLuint a1);
void glEnablei(GLenum a0, GLuint a1);
GLboolean glIsEnabledi(GLenum a0, GLuint a1);
void glDrawElementsBaseVertex(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4);
void glDrawElementsInstancedBaseVertex(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5);
void glDrawRangeElementsBaseVertex(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6);
void glMultiDrawElementsBaseVertex(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4, const GLint * a5);
void glBindFramebuffer(GLenum a0, GLuint a1);
void glBindRenderbuffer(GLenum a0, GLuint a1);
GLenum glCheckFramebufferStatus(GLenum a0);
void glDeleteFramebuffers(GLsizei a0, const GLuint * a1);
void glDeleteRenderbuffers(GLsizei a0, const GLuint * a1);
void glFramebufferRenderbuffer(GLenum a0, GLenum a1, GLenum a2, GLuint a3);
void glFramebufferTexture2D(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glGenFramebuffers(GLsizei a0, GLuint * a1);
void glGenRenderbuffers(GLsizei a0, GLuint * a1);
void glGenerateMipmap(GLenum a0);
void glGetFramebufferAttachmentParameteriv(GLenum a0, GLenum a1, GLenum a2, GLint * a3);
void glGetRenderbufferParameteriv(GLenum a0, GLenum a1, GLint * a2);
GLboolean glIsFramebuffer(GLuint a0);
GLboolean glIsRenderbuffer(GLuint a0);
void glRenderbufferStorage(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3);
void glFramebufferTexture(GLenum a0, GLenum a1, GLuint a2, GLint a3);
void glGetProgramBinary(GLuint a0, GLsizei a1, GLsizei * a2, GLenum * a3, void * a4);
void glGetBufferPointerv(GLenum a0, GLenum a1, void ** a2);
void * glMapBuffer(GLenum a0, GLenum a1);
GLboolean glUnmapBuffer(GLenum a0);
void glMinSampleShading(GLfloat a0);
void glPatchParameteri(GLenum a0, GLint a1);
void glCompressedTexImage3D(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8);
void glCompressedTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10);
void glCopyTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8);
void glFramebufferTexture3D(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5);
void glTexSubImage3D(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glGetSamplerParameterIiv(GLuint a0, GLenum a1, GLint * a2);
void glGetSamplerParameterIuiv(GLuint a0, GLenum a1, GLuint * a2);
void glGetTexParameterIiv(GLenum a0, GLenum a1, GLint * a2);
void glGetTexParameterIuiv(GLenum a0, GLenum a1, GLuint * a2);
void glSamplerParameterIiv(GLuint a0, GLenum a1, const GLint * a2);
void glSamplerParameterIuiv(GLuint a0, GLenum a1, const GLuint * a2);
void glTexParameterIiv(GLenum a0, GLenum a1, const GLint * a2);
void glTexParameterIuiv(GLenum a0, GLenum a1, const GLuint * a2);
void glTexBuffer(GLenum a0, GLenum a1, GLuint a2);
void glTexBufferRange(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glTexStorage3DMultisample(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLboolean a6);
void glTextureView(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7);
void glBindVertexArray(GLuint a0);
void glDeleteVertexArrays(GLsizei a0, const GLuint * a1);
void glGenVertexArrays(GLsizei a0, GLuint * a1);
GLboolean glIsVertexArray(GLuint a0);
} }

extern "C" {
GLenum glClientWaitSyncAPPLE(GLsync a0, GLbitfield a1, GLuint64 a2) { tglmt::gl::glClientWaitSync(a0, a1, a2); }
void glDeleteSyncAPPLE(GLsync a0) { tglmt::gl::glDeleteSync(a0); }
GLsync glFenceSyncAPPLE(GLenum a0, GLbitfield a1) { tglmt::gl::glFenceSync(a0, a1); }
void glGetInteger64vAPPLE(GLenum a0, GLint64 * a1) { tglmt::gl::glGetInteger64v(a0, a1); }
void glGetSyncivAPPLE(GLsync a0, GLenum a1, GLsizei a2, GLsizei * a3, GLint * a4) { tglmt::gl::glGetSynciv(a0, a1, a2, a3, a4); }
GLboolean glIsSyncAPPLE(GLsync a0) { tglmt::gl::glIsSync(a0); }
void glWaitSyncAPPLE(GLsync a0, GLbitfield a1, GLuint64 a2) { tglmt::gl::glWaitSync(a0, a1, a2); }
void glBindVertexArrayAPPLE(GLuint a0) { tglmt::gl::glBindVertexArray(a0); }
void glDeleteVertexArraysAPPLE(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteVertexArrays(a0, a1); }
void glGenVertexArraysAPPLE(GLsizei a0, GLuint * a1) { tglmt::gl::glGenVertexArrays(a0, a1); }
GLboolean glIsVertexArrayAPPLE(GLuint a0) { tglmt::gl::glIsVertexArray(a0); }
void glClampColorARB(GLenum a0, GLenum a1) { tglmt::gl::glClampColor(a0, a1); }
void glDebugMessageControlARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, const GLuint * a4, GLboolean a5) { tglmt::gl::glDebugMessageControl(a0, a1, a2, a3, a4, a5); }
void glDebugMessageInsertARB(GLenum a0, GLenum a1, GLuint a2, GLenum a3, GLsizei a4, const GLchar * a5) { tglmt::gl::glDebugMessageInsert(a0, a1, a2, a3, a4, a5); }
GLuint glGetDebugMessageLogARB(GLuint a0, GLsizei a1, GLenum * a2, GLenum * a3, GLuint * a4, GLenum * a5, GLsizei * a6, GLchar * a7) { tglmt::gl::glGetDebugMessageLog(a0, a1, a2, a3, a4, a5, a6, a7); }
void glDrawBuffersARB(GLsizei a0, const GLenum * a1) { tglmt::gl::glDrawBuffers(a0, a1); }
void glBlendEquationSeparateiARB(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendEquationSeparatei(a0, a1, a2); }
void glBlendEquationiARB(GLuint a0, GLenum a1) { tglmt::gl::glBlendEquationi(a0, a1); }
void glBlendFuncSeparateiARB(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4) { tglmt::gl::glBlendFuncSeparatei(a0, a1, a2, a3, a4); }
void glBlendFunciARB(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendFunci(a0, a1, a2); }
void glDrawArraysInstancedARB(GLenum a0, GLint a1, GLsizei a2, GLsizei a3) { tglmt::gl::glDrawArraysInstanced(a0, a1, a2, a3); }
void glDrawElementsInstancedARB(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4) { tglmt::gl::glDrawElementsInstanced(a0, a1, a2, a3, a4); }
void glSpecializeShaderARB(GLuint a0, const GLchar * a1, GLuint a2, const GLuint * a3, const GLuint * a4) { tglmt::gl::glSpecializeShader(a0, a1, a2, a3, a4); }
void glMultiDrawArraysIndirectCountARB(GLenum a0, const void * a1, GLintptr a2, GLsizei a3, GLsizei a4) { tglmt::gl::glMultiDrawArraysIndirectCount(a0, a1, a2, a3, a4); }
void glMultiDrawElementsIndirectCountARB(GLenum a0, GLenum a1, const void * a2, GLintptr a3, GLsizei a4, GLsizei a5) { tglmt::gl::glMultiDrawElementsIndirectCount(a0, a1, a2, a3, a4, a5); }
void glVertexAttribDivisorARB(GLuint a0, GLuint a1) { tglmt::gl::glVertexAttribDivisor(a0, a1); }
void glSampleCoverageARB(GLfloat a0, GLboolean a1) { tglmt::gl::glSampleCoverage(a0, a1); }
void glBeginQueryARB(GLenum a0, GLuint a1) { tglmt::gl::glBeginQuery(a0, a1); }
void glDeleteQueriesARB(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteQueries(a0, a1); }
void glEndQueryARB(GLenum a0) { tglmt::gl::glEndQuery(a0); }
void glGenQueriesARB(GLsizei a0, GLuint * a1) { tglmt::gl::glGenQueries(a0, a1); }
void glGetQueryObjectivARB(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetQueryObjectiv(a0, a1, a2); }
void glGetQueryObjectuivARB(GLuint a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetQueryObjectuiv(a0, a1, a2); }
void glGetQueryivARB(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetQueryiv(a0, a1, a2); }
GLboolean glIsQueryARB(GLuint a0) { tglmt::gl::glIsQuery(a0); }
void glPointParameterfARB(GLenum a0, GLfloat a1) { tglmt::gl::glPointParameterf(a0, a1); }
void glPointParameterfvARB(GLenum a0, const GLfloat * a1) { tglmt::gl::glPointParameterfv(a0, a1); }
GLenum glGetGraphicsResetStatusARB(void) { tglmt::gl::glGetGraphicsResetStatus(); }
void glGetnColorTableARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4) { tglmt::gl::glGetnColorTable(a0, a1, a2, a3, a4); }
void glGetnCompressedTexImageARB(GLenum a0, GLint a1, GLsizei a2, void * a3) { tglmt::gl::glGetnCompressedTexImage(a0, a1, a2, a3); }
void glGetnConvolutionFilterARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4) { tglmt::gl::glGetnConvolutionFilter(a0, a1, a2, a3, a4); }
void glGetnHistogramARB(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5) { tglmt::gl::glGetnHistogram(a0, a1, a2, a3, a4, a5); }
void glGetnMapdvARB(GLenum a0, GLenum a1, GLsizei a2, GLdouble * a3) { tglmt::gl::glGetnMapdv(a0, a1, a2, a3); }
void glGetnMapfvARB(GLenum a0, GLenum a1, GLsizei a2, GLfloat * a3) { tglmt::gl::glGetnMapfv(a0, a1, a2, a3); }
void glGetnMapivARB(GLenum a0, GLenum a1, GLsizei a2, GLint * a3) { tglmt::gl::glGetnMapiv(a0, a1, a2, a3); }
void glGetnMinmaxARB(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5) { tglmt::gl::glGetnMinmax(a0, a1, a2, a3, a4, a5); }
void glGetnPixelMapfvARB(GLenum a0, GLsizei a1, GLfloat * a2) { tglmt::gl::glGetnPixelMapfv(a0, a1, a2); }
void glGetnPixelMapuivARB(GLenum a0, GLsizei a1, GLuint * a2) { tglmt::gl::glGetnPixelMapuiv(a0, a1, a2); }
void glGetnPixelMapusvARB(GLenum a0, GLsizei a1, GLushort * a2) { tglmt::gl::glGetnPixelMapusv(a0, a1, a2); }
void glGetnPolygonStippleARB(GLsizei a0, GLubyte * a1) { tglmt::gl::glGetnPolygonStipple(a0, a1); }
void glGetnSeparableFilterARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4, GLsizei a5, void * a6, void * a7) { tglmt::gl::glGetnSeparableFilter(a0, a1, a2, a3, a4, a5, a6, a7); }
void glGetnTexImageARB(GLenum a0, GLint a1, GLenum a2, GLenum a3, GLsizei a4, void * a5) { tglmt::gl::glGetnTexImage(a0, a1, a2, a3, a4, a5); }
void glGetnUniformdvARB(GLuint a0, GLint a1, GLsizei a2, GLdouble * a3) { tglmt::gl::glGetnUniformdv(a0, a1, a2, a3); }
void glGetnUniformfvARB(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3) { tglmt::gl::glGetnUniformfv(a0, a1, a2, a3); }
void glGetnUniformivARB(GLuint a0, GLint a1, GLsizei a2, GLint * a3) { tglmt::gl::glGetnUniformiv(a0, a1, a2, a3); }
void glGetnUniformuivARB(GLuint a0, GLint a1, GLsizei a2, GLuint * a3) { tglmt::gl::glGetnUniformuiv(a0, a1, a2, a3); }
void glReadnPixelsARB(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7) { tglmt::gl::glReadnPixels(a0, a1, a2, a3, a4, a5, a6, a7); }
void glMinSampleShadingARB(GLfloat a0) { tglmt::gl::glMinSampleShading(a0); }
void glTexBufferARB(GLenum a0, GLenum a1, GLuint a2) { tglmt::gl::glTexBuffer(a0, a1, a2); }
void glCompressedTexImage1DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLint a4, GLsizei a5, const void * a6) { tglmt::gl::glCompressedTexImage1D(a0, a1, a2, a3, a4, a5, a6); }
void glCompressedTexImage2DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLint a5, GLsizei a6, const void * a7) { tglmt::gl::glCompressedTexImage2D(a0, a1, a2, a3, a4, a5, a6, a7); }
void glCompressedTexImage3DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8) { tglmt::gl::glCompressedTexImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glCompressedTexSubImage1DARB(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLsizei a5, const void * a6) { tglmt::gl::glCompressedTexSubImage1D(a0, a1, a2, a3, a4, a5, a6); }
void glCompressedTexSubImage2DARB(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLsizei a7, const void * a8) { tglmt::gl::glCompressedTexSubImage2D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glCompressedTexSubImage3DARB(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10) { tglmt::gl::glCompressedTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
void glGetCompressedTexImageARB(GLenum a0, GLint a1, void * a2) { tglmt::gl::glGetCompressedTexImage(a0, a1, a2); }
void glBindBufferARB(GLenum a0, GLuint a1) { tglmt::gl::glBindBuffer(a0, a1); }
void glDeleteBuffersARB(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteBuffers(a0, a1); }
void glGenBuffersARB(GLsizei a0, GLuint * a1) { tglmt::gl::glGenBuffers(a0, a1); }
void glGetBufferParameterivARB(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetBufferParameteriv(a0, a1, a2); }
void glGetBufferPointervARB(GLenum a0, GLenum a1, void ** a2) { tglmt::gl::glGetBufferPointerv(a0, a1, a2); }
GLboolean glIsBufferARB(GLuint a0) { tglmt::gl::glIsBuffer(a0); }
void * glMapBufferARB(GLenum a0, GLenum a1) { tglmt::gl::glMapBuffer(a0, a1); }
GLboolean glUnmapBufferARB(GLenum a0) { tglmt::gl::glUnmapBuffer(a0); }
void glDisableVertexAttribArrayARB(GLuint a0) { tglmt::gl::glDisableVertexAttribArray(a0); }
void glEnableVertexAttribArrayARB(GLuint a0) { tglmt::gl::glEnableVertexAttribArray(a0); }
void glGetVertexAttribPointervARB(GLuint a0, GLenum a1, void ** a2) { tglmt::gl::glGetVertexAttribPointerv(a0, a1, a2); }
void glGetVertexAttribdvARB(GLuint a0, GLenum a1, GLdouble * a2) { tglmt::gl::glGetVertexAttribdv(a0, a1, a2); }
void glGetVertexAttribfvARB(GLuint a0, GLenum a1, GLfloat * a2) { tglmt::gl::glGetVertexAttribfv(a0, a1, a2); }
void glGetVertexAttribivARB(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetVertexAttribiv(a0, a1, a2); }
void glVertexAttrib1dARB(GLuint a0, GLdouble a1) { tglmt::gl::glVertexAttrib1d(a0, a1); }
void glVertexAttrib1dvARB(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttrib1dv(a0, a1); }
void glVertexAttrib1fARB(GLuint a0, GLfloat a1) { tglmt::gl::glVertexAttrib1f(a0, a1); }
void glVertexAttrib1fvARB(GLuint a0, const GLfloat * a1) { tglmt::gl::glVertexAttrib1fv(a0, a1); }
void glVertexAttrib1sARB(GLuint a0, GLshort a1) { tglmt::gl::glVertexAttrib1s(a0, a1); }
void glVertexAttrib1svARB(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttrib1sv(a0, a1); }
void glVertexAttrib2dARB(GLuint a0, GLdouble a1, GLdouble a2) { tglmt::gl::glVertexAttrib2d(a0, a1, a2); }
void glVertexAttrib2dvARB(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttrib2dv(a0, a1); }
void glVertexAttrib2fARB(GLuint a0, GLfloat a1, GLfloat a2) { tglmt::gl::glVertexAttrib2f(a0, a1, a2); }
void glVertexAttrib2fvARB(GLuint a0, const GLfloat * a1) { tglmt::gl::glVertexAttrib2fv(a0, a1); }
void glVertexAttrib2sARB(GLuint a0, GLshort a1, GLshort a2) { tglmt::gl::glVertexAttrib2s(a0, a1, a2); }
void glVertexAttrib2svARB(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttrib2sv(a0, a1); }
void glVertexAttrib3dARB(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3) { tglmt::gl::glVertexAttrib3d(a0, a1, a2, a3); }
void glVertexAttrib3dvARB(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttrib3dv(a0, a1); }
void glVertexAttrib3fARB(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3) { tglmt::gl::glVertexAttrib3f(a0, a1, a2, a3); }
void glVertexAttrib3fvARB(GLuint a0, const GLfloat * a1) { tglmt::gl::glVertexAttrib3fv(a0, a1); }
void glVertexAttrib3sARB(GLuint a0, GLshort a1, GLshort a2, GLshort a3) { tglmt::gl::glVertexAttrib3s(a0, a1, a2, a3); }
void glVertexAttrib3svARB(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttrib3sv(a0, a1); }
void glVertexAttrib4NbvARB(GLuint a0, const GLbyte * a1) { tglmt::gl::glVertexAttrib4Nbv(a0, a1); }
void glVertexAttrib4NivARB(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttrib4Niv(a0, a1); }
void glVertexAttrib4NsvARB(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttrib4Nsv(a0, a1); }
void glVertexAttrib4NubARB(GLuint a0, GLubyte a1, GLubyte a2, GLubyte a3, GLubyte a4) { tglmt::gl::glVertexAttrib4Nub(a0, a1, a2, a3, a4); }
void glVertexAttrib4NubvARB(GLuint a0, const GLubyte * a1) { tglmt::gl::glVertexAttrib4Nubv(a0, a1); }
void glVertexAttrib4NuivARB(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttrib4Nuiv(a0, a1); }
void glVertexAttrib4NusvARB(GLuint a0, const GLushort * a1) { tglmt::gl::glVertexAttrib4Nusv(a0, a1); }
void glVertexAttrib4bvARB(GLuint a0, const GLbyte * a1) { tglmt::gl::glVertexAttrib4bv(a0, a1); }
void glVertexAttrib4dARB(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4) { tglmt::gl::glVertexAttrib4d(a0, a1, a2, a3, a4); }
void glVertexAttrib4dvARB(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttrib4dv(a0, a1); }
void glVertexAttrib4fARB(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4) { tglmt::gl::glVertexAttrib4f(a0, a1, a2, a3, a4); }
void glVertexAttrib4fvARB(GLuint a0, const GLfloat * a1) { tglmt::gl::glVertexAttrib4fv(a0, a1); }
void glVertexAttrib4ivARB(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttrib4iv(a0, a1); }
void glVertexAttrib4sARB(GLuint a0, GLshort a1, GLshort a2, GLshort a3, GLshort a4) { tglmt::gl::glVertexAttrib4s(a0, a1, a2, a3, a4); }
void glVertexAttrib4svARB(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttrib4sv(a0, a1); }
void glVertexAttrib4ubvARB(GLuint a0, const GLubyte * a1) { tglmt::gl::glVertexAttrib4ubv(a0, a1); }
void glVertexAttrib4uivARB(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttrib4uiv(a0, a1); }
void glVertexAttrib4usvARB(GLuint a0, const GLushort * a1) { tglmt::gl::glVertexAttrib4usv(a0, a1); }
void glVertexAttribPointerARB(GLuint a0, GLint a1, GLenum a2, GLboolean a3, GLsizei a4, const void * a5) { tglmt::gl::glVertexAttribPointer(a0, a1, a2, a3, a4, a5); }
void glDrawArraysInstancedBaseInstanceEXT(GLenum a0, GLint a1, GLsizei a2, GLsizei a3, GLuint a4) { tglmt::gl::glDrawArraysInstancedBaseInstance(a0, a1, a2, a3, a4); }
void glDrawElementsInstancedBaseInstanceEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLuint a5) { tglmt::gl::glDrawElementsInstancedBaseInstance(a0, a1, a2, a3, a4, a5); }
void glDrawElementsInstancedBaseVertexBaseInstanceEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5, GLuint a6) { tglmt::gl::glDrawElementsInstancedBaseVertexBaseInstance(a0, a1, a2, a3, a4, a5, a6); }
void glBlendColorEXT(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3) { tglmt::gl::glBlendColor(a0, a1, a2, a3); }
void glBlendEquationSeparateEXT(GLenum a0, GLenum a1) { tglmt::gl::glBlendEquationSeparate(a0, a1); }
void glBindFragDataLocationEXT(GLuint a0, GLuint a1, const GLchar * a2) { tglmt::gl::glBindFragDataLocation(a0, a1, a2); }
void glBindFragDataLocationIndexedEXT(GLuint a0, GLuint a1, GLuint a2, const GLchar * a3) { tglmt::gl::glBindFragDataLocationIndexed(a0, a1, a2, a3); }
GLint glGetFragDataIndexEXT(GLuint a0, const GLchar * a1) { tglmt::gl::glGetFragDataIndex(a0, a1); }
GLint glGetProgramResourceLocationIndexEXT(GLuint a0, GLenum a1, const GLchar * a2) { tglmt::gl::glGetProgramResourceLocationIndex(a0, a1, a2); }
void glBlendFuncSeparateEXT(GLenum a0, GLenum a1, GLenum a2, GLenum a3) { tglmt::gl::glBlendFuncSeparate(a0, a1, a2, a3); }
void glBlendEquationEXT(GLenum a0) { tglmt::gl::glBlendEquation(a0); }
void glBufferStorageEXT(GLenum a0, GLsizeiptr a1, const void * a2, GLbitfield a3) { tglmt::gl::glBufferStorage(a0, a1, a2, a3); }
void glClearTexImageEXT(GLuint a0, GLint a1, GLenum a2, GLenum a3, const void * a4) { tglmt::gl::glClearTexImage(a0, a1, a2, a3, a4); }
void glClearTexSubImageEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10) { tglmt::gl::glClearTexSubImage(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
void glClipControlEXT(GLenum a0, GLenum a1) { tglmt::gl::glClipControl(a0, a1); }
void glCopyImageSubDataEXT(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14) { tglmt::gl::glCopyImageSubData(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14); }
void glCopyTexImage1DEXT(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLint a6) { tglmt::gl::glCopyTexImage1D(a0, a1, a2, a3, a4, a5, a6); }
void glCopyTexImage2DEXT(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLint a7) { tglmt::gl::glCopyTexImage2D(a0, a1, a2, a3, a4, a5, a6, a7); }
void glCopyTexSubImage1DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5) { tglmt::gl::glCopyTexSubImage1D(a0, a1, a2, a3, a4, a5); }
void glCopyTexSubImage2DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLsizei a6, GLsizei a7) { tglmt::gl::glCopyTexSubImage2D(a0, a1, a2, a3, a4, a5, a6, a7); }
void glCopyTexSubImage3DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8) { tglmt::gl::glCopyTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glBeginQueryEXT(GLenum a0, GLuint a1) { tglmt::gl::glBeginQuery(a0, a1); }
void glDeleteQueriesEXT(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteQueries(a0, a1); }
void glEndQueryEXT(GLenum a0) { tglmt::gl::glEndQuery(a0); }
void glGenQueriesEXT(GLsizei a0, GLuint * a1) { tglmt::gl::glGenQueries(a0, a1); }
void glGetInteger64vEXT(GLenum a0, GLint64 * a1) { tglmt::gl::glGetInteger64v(a0, a1); }
void glGetQueryObjecti64vEXT(GLuint a0, GLenum a1, GLint64 * a2) { tglmt::gl::glGetQueryObjecti64v(a0, a1, a2); }
void glGetQueryObjectivEXT(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetQueryObjectiv(a0, a1, a2); }
void glGetQueryObjectui64vEXT(GLuint a0, GLenum a1, GLuint64 * a2) { tglmt::gl::glGetQueryObjectui64v(a0, a1, a2); }
void glGetQueryObjectuivEXT(GLuint a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetQueryObjectuiv(a0, a1, a2); }
void glGetQueryivEXT(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetQueryiv(a0, a1, a2); }
GLboolean glIsQueryEXT(GLuint a0) { tglmt::gl::glIsQuery(a0); }
void glQueryCounterEXT(GLuint a0, GLenum a1) { tglmt::gl::glQueryCounter(a0, a1); }
void glDrawBuffersEXT(GLsizei a0, const GLenum * a1) { tglmt::gl::glDrawBuffers(a0, a1); }
void glBlendEquationSeparateiEXT(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendEquationSeparatei(a0, a1, a2); }
void glBlendEquationiEXT(GLuint a0, GLenum a1) { tglmt::gl::glBlendEquationi(a0, a1); }
void glBlendFuncSeparateiEXT(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4) { tglmt::gl::glBlendFuncSeparatei(a0, a1, a2, a3, a4); }
void glBlendFunciEXT(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendFunci(a0, a1, a2); }
void glColorMaskiEXT(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4) { tglmt::gl::glColorMaski(a0, a1, a2, a3, a4); }
void glDisableiEXT(GLenum a0, GLuint a1) { tglmt::gl::glDisablei(a0, a1); }
void glEnableiEXT(GLenum a0, GLuint a1) { tglmt::gl::glEnablei(a0, a1); }
GLboolean glIsEnablediEXT(GLenum a0, GLuint a1) { tglmt::gl::glIsEnabledi(a0, a1); }
void glDrawElementsBaseVertexEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4) { tglmt::gl::glDrawElementsBaseVertex(a0, a1, a2, a3, a4); }
void glDrawElementsInstancedBaseVertexEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5) { tglmt::gl::glDrawElementsInstancedBaseVertex(a0, a1, a2, a3, a4, a5); }
void glDrawRangeElementsBaseVertexEXT(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6) { tglmt::gl::glDrawRangeElementsBaseVertex(a0, a1, a2, a3, a4, a5, a6); }
void glMultiDrawElementsBaseVertexEXT(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4, const GLint * a5) { tglmt::gl::glMultiDrawElementsBaseVertex(a0, a1, a2, a3, a4, a5); }
void glDrawArraysInstancedEXT(GLenum a0, GLint a1, GLsizei a2, GLsizei a3) { tglmt::gl::glDrawArraysInstanced(a0, a1, a2, a3); }
void glDrawElementsInstancedEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4) { tglmt::gl::glDrawElementsInstanced(a0, a1, a2, a3, a4); }
void glDrawRangeElementsEXT(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5) { tglmt::gl::glDrawRangeElements(a0, a1, a2, a3, a4, a5); }
void glDrawTransformFeedbackEXT(GLenum a0, GLuint a1) { tglmt::gl::glDrawTransformFeedback(a0, a1); }
void glDrawTransformFeedbackInstancedEXT(GLenum a0, GLuint a1, GLsizei a2) { tglmt::gl::glDrawTransformFeedbackInstanced(a0, a1, a2); }
void glBlitFramebufferEXT(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9) { tglmt::gl::glBlitFramebuffer(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }
void glRenderbufferStorageMultisampleEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4) { tglmt::gl::glRenderbufferStorageMultisample(a0, a1, a2, a3, a4); }
void glBindFramebufferEXT(GLenum a0, GLuint a1) { tglmt::gl::glBindFramebuffer(a0, a1); }
void glBindRenderbufferEXT(GLenum a0, GLuint a1) { tglmt::gl::glBindRenderbuffer(a0, a1); }
GLenum glCheckFramebufferStatusEXT(GLenum a0) { tglmt::gl::glCheckFramebufferStatus(a0); }
void glDeleteFramebuffersEXT(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteFramebuffers(a0, a1); }
void glDeleteRenderbuffersEXT(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteRenderbuffers(a0, a1); }
void glFramebufferRenderbufferEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3) { tglmt::gl::glFramebufferRenderbuffer(a0, a1, a2, a3); }
void glFramebufferTexture1DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4) { tglmt::gl::glFramebufferTexture1D(a0, a1, a2, a3, a4); }
void glFramebufferTexture2DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4) { tglmt::gl::glFramebufferTexture2D(a0, a1, a2, a3, a4); }
void glFramebufferTexture3DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5) { tglmt::gl::glFramebufferTexture3D(a0, a1, a2, a3, a4, a5); }
void glGenFramebuffersEXT(GLsizei a0, GLuint * a1) { tglmt::gl::glGenFramebuffers(a0, a1); }
void glGenRenderbuffersEXT(GLsizei a0, GLuint * a1) { tglmt::gl::glGenRenderbuffers(a0, a1); }
void glGenerateMipmapEXT(GLenum a0) { tglmt::gl::glGenerateMipmap(a0); }
void glGetFramebufferAttachmentParameterivEXT(GLenum a0, GLenum a1, GLenum a2, GLint * a3) { tglmt::gl::glGetFramebufferAttachmentParameteriv(a0, a1, a2, a3); }
void glGetRenderbufferParameterivEXT(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetRenderbufferParameteriv(a0, a1, a2); }
GLboolean glIsFramebufferEXT(GLuint a0) { tglmt::gl::glIsFramebuffer(a0); }
GLboolean glIsRenderbufferEXT(GLuint a0) { tglmt::gl::glIsRenderbuffer(a0); }
void glRenderbufferStorageEXT(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3) { tglmt::gl::glRenderbufferStorage(a0, a1, a2, a3); }
void glFramebufferTextureEXT(GLenum a0, GLenum a1, GLuint a2, GLint a3) { tglmt::gl::glFramebufferTexture(a0, a1, a2, a3); }
void glProgramParameteriEXT(GLuint a0, GLenum a1, GLint a2) { tglmt::gl::glProgramParameteri(a0, a1, a2); }
GLint glGetFragDataLocationEXT(GLuint a0, const GLchar * a1) { tglmt::gl::glGetFragDataLocation(a0, a1); }
void glGetUniformuivEXT(GLuint a0, GLint a1, GLuint * a2) { tglmt::gl::glGetUniformuiv(a0, a1, a2); }
void glGetVertexAttribIivEXT(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetVertexAttribIiv(a0, a1, a2); }
void glGetVertexAttribIuivEXT(GLuint a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetVertexAttribIuiv(a0, a1, a2); }
void glUniform1uiEXT(GLint a0, GLuint a1) { tglmt::gl::glUniform1ui(a0, a1); }
void glUniform1uivEXT(GLint a0, GLsizei a1, const GLuint * a2) { tglmt::gl::glUniform1uiv(a0, a1, a2); }
void glUniform2uiEXT(GLint a0, GLuint a1, GLuint a2) { tglmt::gl::glUniform2ui(a0, a1, a2); }
void glUniform2uivEXT(GLint a0, GLsizei a1, const GLuint * a2) { tglmt::gl::glUniform2uiv(a0, a1, a2); }
void glUniform3uiEXT(GLint a0, GLuint a1, GLuint a2, GLuint a3) { tglmt::gl::glUniform3ui(a0, a1, a2, a3); }
void glUniform3uivEXT(GLint a0, GLsizei a1, const GLuint * a2) { tglmt::gl::glUniform3uiv(a0, a1, a2); }
void glUniform4uiEXT(GLint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4) { tglmt::gl::glUniform4ui(a0, a1, a2, a3, a4); }
void glUniform4uivEXT(GLint a0, GLsizei a1, const GLuint * a2) { tglmt::gl::glUniform4uiv(a0, a1, a2); }
void glVertexAttribI1iEXT(GLuint a0, GLint a1) { tglmt::gl::glVertexAttribI1i(a0, a1); }
void glVertexAttribI1ivEXT(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttribI1iv(a0, a1); }
void glVertexAttribI1uiEXT(GLuint a0, GLuint a1) { tglmt::gl::glVertexAttribI1ui(a0, a1); }
void glVertexAttribI1uivEXT(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttribI1uiv(a0, a1); }
void glVertexAttribI2iEXT(GLuint a0, GLint a1, GLint a2) { tglmt::gl::glVertexAttribI2i(a0, a1, a2); }
void glVertexAttribI2ivEXT(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttribI2iv(a0, a1); }
void glVertexAttribI2uiEXT(GLuint a0, GLuint a1, GLuint a2) { tglmt::gl::glVertexAttribI2ui(a0, a1, a2); }
void glVertexAttribI2uivEXT(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttribI2uiv(a0, a1); }
void glVertexAttribI3iEXT(GLuint a0, GLint a1, GLint a2, GLint a3) { tglmt::gl::glVertexAttribI3i(a0, a1, a2, a3); }
void glVertexAttribI3ivEXT(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttribI3iv(a0, a1); }
void glVertexAttribI3uiEXT(GLuint a0, GLuint a1, GLuint a2, GLuint a3) { tglmt::gl::glVertexAttribI3ui(a0, a1, a2, a3); }
void glVertexAttribI3uivEXT(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttribI3uiv(a0, a1); }
void glVertexAttribI4bvEXT(GLuint a0, const GLbyte * a1) { tglmt::gl::glVertexAttribI4bv(a0, a1); }
void glVertexAttribI4iEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4) { tglmt::gl::glVertexAttribI4i(a0, a1, a2, a3, a4); }
void glVertexAttribI4ivEXT(GLuint a0, const GLint * a1) { tglmt::gl::glVertexAttribI4iv(a0, a1); }
void glVertexAttribI4svEXT(GLuint a0, const GLshort * a1) { tglmt::gl::glVertexAttribI4sv(a0, a1); }
void glVertexAttribI4ubvEXT(GLuint a0, const GLubyte * a1) { tglmt::gl::glVertexAttribI4ubv(a0, a1); }
void glVertexAttribI4uiEXT(GLuint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4) { tglmt::gl::glVertexAttribI4ui(a0, a1, a2, a3, a4); }
void glVertexAttribI4uivEXT(GLuint a0, const GLuint * a1) { tglmt::gl::glVertexAttribI4uiv(a0, a1); }
void glVertexAttribI4usvEXT(GLuint a0, const GLushort * a1) { tglmt::gl::glVertexAttribI4usv(a0, a1); }
void glVertexAttribIPointerEXT(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4) { tglmt::gl::glVertexAttribIPointer(a0, a1, a2, a3, a4); }
void glVertexAttribDivisorEXT(GLuint a0, GLuint a1) { tglmt::gl::glVertexAttribDivisor(a0, a1); }
void glFlushMappedBufferRangeEXT(GLenum a0, GLintptr a1, GLsizeiptr a2) { tglmt::gl::glFlushMappedBufferRange(a0, a1, a2); }
void * glMapBufferRangeEXT(GLenum a0, GLintptr a1, GLsizeiptr a2, GLbitfield a3) { tglmt::gl::glMapBufferRange(a0, a1, a2, a3); }
void glMultiDrawArraysEXT(GLenum a0, const GLint * a1, const GLsizei * a2, GLsizei a3) { tglmt::gl::glMultiDrawArrays(a0, a1, a2, a3); }
void glMultiDrawElementsEXT(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4) { tglmt::gl::glMultiDrawElements(a0, a1, a2, a3, a4); }
void glMultiDrawArraysIndirectEXT(GLenum a0, const void * a1, GLsizei a2, GLsizei a3) { tglmt::gl::glMultiDrawArraysIndirect(a0, a1, a2, a3); }
void glMultiDrawElementsIndirectEXT(GLenum a0, GLenum a1, const void * a2, GLsizei a3, GLsizei a4) { tglmt::gl::glMultiDrawElementsIndirect(a0, a1, a2, a3, a4); }
void glPointParameterfEXT(GLenum a0, GLfloat a1) { tglmt::gl::glPointParameterf(a0, a1); }
void glPointParameterfvEXT(GLenum a0, const GLfloat * a1) { tglmt::gl::glPointParameterfv(a0, a1); }
void glPolygonOffsetEXT(GLfloat a0, GLfloat a1) { tglmt::gl::glPolygonOffset(a0, a1); }
void glPolygonOffsetClampEXT(GLfloat a0, GLfloat a1, GLfloat a2) { tglmt::gl::glPolygonOffsetClamp(a0, a1, a2); }
void glProvokingVertexEXT(GLenum a0) { tglmt::gl::glProvokingVertex(a0); }
GLenum glGetGraphicsResetStatusEXT(void) { tglmt::gl::glGetGraphicsResetStatus(); }
void glGetnUniformfvEXT(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3) { tglmt::gl::glGetnUniformfv(a0, a1, a2, a3); }
void glGetnUniformivEXT(GLuint a0, GLint a1, GLsizei a2, GLint * a3) { tglmt::gl::glGetnUniformiv(a0, a1, a2, a3); }
void glReadnPixelsEXT(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7) { tglmt::gl::glReadnPixels(a0, a1, a2, a3, a4, a5, a6, a7); }
void glMemoryBarrierEXT(GLbitfield a0) { tglmt::gl::glMemoryBarrier(a0); }
void glTexSubImage1DEXT(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLenum a5, const void * a6) { tglmt::gl::glTexSubImage1D(a0, a1, a2, a3, a4, a5, a6); }
void glTexSubImage2DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLenum a7, const void * a8) { tglmt::gl::glTexSubImage2D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glPatchParameteriEXT(GLenum a0, GLint a1) { tglmt::gl::glPatchParameteri(a0, a1); }
void glTexSubImage3DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10) { tglmt::gl::glTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
void glFramebufferTextureLayerEXT(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLint a4) { tglmt::gl::glFramebufferTextureLayer(a0, a1, a2, a3, a4); }
void glGetSamplerParameterIivEXT(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetSamplerParameterIiv(a0, a1, a2); }
void glGetSamplerParameterIuivEXT(GLuint a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetSamplerParameterIuiv(a0, a1, a2); }
void glGetTexParameterIivEXT(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetTexParameterIiv(a0, a1, a2); }
void glGetTexParameterIuivEXT(GLenum a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetTexParameterIuiv(a0, a1, a2); }
void glSamplerParameterIivEXT(GLuint a0, GLenum a1, const GLint * a2) { tglmt::gl::glSamplerParameterIiv(a0, a1, a2); }
void glSamplerParameterIuivEXT(GLuint a0, GLenum a1, const GLuint * a2) { tglmt::gl::glSamplerParameterIuiv(a0, a1, a2); }
void glTexParameterIivEXT(GLenum a0, GLenum a1, const GLint * a2) { tglmt::gl::glTexParameterIiv(a0, a1, a2); }
void glTexParameterIuivEXT(GLenum a0, GLenum a1, const GLuint * a2) { tglmt::gl::glTexParameterIuiv(a0, a1, a2); }
void glTexBufferEXT(GLenum a0, GLenum a1, GLuint a2) { tglmt::gl::glTexBuffer(a0, a1, a2); }
void glTexBufferRangeEXT(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4) { tglmt::gl::glTexBufferRange(a0, a1, a2, a3, a4); }
void glTexStorage1DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3) { tglmt::gl::glTexStorage1D(a0, a1, a2, a3); }
void glTexStorage2DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4) { tglmt::gl::glTexStorage2D(a0, a1, a2, a3, a4); }
void glTexStorage3DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5) { tglmt::gl::glTexStorage3D(a0, a1, a2, a3, a4, a5); }
void glTextureViewEXT(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7) { tglmt::gl::glTextureView(a0, a1, a2, a3, a4, a5, a6, a7); }
void glGetVertexAttribLdvEXT(GLuint a0, GLenum a1, GLdouble * a2) { tglmt::gl::glGetVertexAttribLdv(a0, a1, a2); }
void glVertexAttribL1dEXT(GLuint a0, GLdouble a1) { tglmt::gl::glVertexAttribL1d(a0, a1); }
void glVertexAttribL1dvEXT(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttribL1dv(a0, a1); }
void glVertexAttribL2dEXT(GLuint a0, GLdouble a1, GLdouble a2) { tglmt::gl::glVertexAttribL2d(a0, a1, a2); }
void glVertexAttribL2dvEXT(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttribL2dv(a0, a1); }
void glVertexAttribL3dEXT(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3) { tglmt::gl::glVertexAttribL3d(a0, a1, a2, a3); }
void glVertexAttribL3dvEXT(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttribL3dv(a0, a1); }
void glVertexAttribL4dEXT(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4) { tglmt::gl::glVertexAttribL4d(a0, a1, a2, a3, a4); }
void glVertexAttribL4dvEXT(GLuint a0, const GLdouble * a1) { tglmt::gl::glVertexAttribL4dv(a0, a1); }
void glVertexAttribLPointerEXT(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4) { tglmt::gl::glVertexAttribLPointer(a0, a1, a2, a3, a4); }
GLenum glGetGraphicsResetStatusKHR(void) { tglmt::gl::glGetGraphicsResetStatus(); }
void glGetnUniformfvKHR(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3) { tglmt::gl::glGetnUniformfv(a0, a1, a2, a3); }
void glGetnUniformivKHR(GLuint a0, GLint a1, GLsizei a2, GLint * a3) { tglmt::gl::glGetnUniformiv(a0, a1, a2, a3); }
void glGetnUniformuivKHR(GLuint a0, GLint a1, GLsizei a2, GLuint * a3) { tglmt::gl::glGetnUniformuiv(a0, a1, a2, a3); }
void glReadnPixelsKHR(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7) { tglmt::gl::glReadnPixels(a0, a1, a2, a3, a4, a5, a6, a7); }
void glBlendEquationSeparateOES(GLenum a0, GLenum a1) { tglmt::gl::glBlendEquationSeparate(a0, a1); }
void glBlendFuncSeparateOES(GLenum a0, GLenum a1, GLenum a2, GLenum a3) { tglmt::gl::glBlendFuncSeparate(a0, a1, a2, a3); }
void glBlendEquationOES(GLenum a0) { tglmt::gl::glBlendEquation(a0); }
void glCopyImageSubDataOES(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14) { tglmt::gl::glCopyImageSubData(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14); }
void glBlendEquationSeparateiOES(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendEquationSeparatei(a0, a1, a2); }
void glBlendEquationiOES(GLuint a0, GLenum a1) { tglmt::gl::glBlendEquationi(a0, a1); }
void glBlendFuncSeparateiOES(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4) { tglmt::gl::glBlendFuncSeparatei(a0, a1, a2, a3, a4); }
void glBlendFunciOES(GLuint a0, GLenum a1, GLenum a2) { tglmt::gl::glBlendFunci(a0, a1, a2); }
void glColorMaskiOES(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4) { tglmt::gl::glColorMaski(a0, a1, a2, a3, a4); }
void glDisableiOES(GLenum a0, GLuint a1) { tglmt::gl::glDisablei(a0, a1); }
void glEnableiOES(GLenum a0, GLuint a1) { tglmt::gl::glEnablei(a0, a1); }
GLboolean glIsEnablediOES(GLenum a0, GLuint a1) { tglmt::gl::glIsEnabledi(a0, a1); }
void glDrawElementsBaseVertexOES(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4) { tglmt::gl::glDrawElementsBaseVertex(a0, a1, a2, a3, a4); }
void glDrawElementsInstancedBaseVertexOES(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5) { tglmt::gl::glDrawElementsInstancedBaseVertex(a0, a1, a2, a3, a4, a5); }
void glDrawRangeElementsBaseVertexOES(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6) { tglmt::gl::glDrawRangeElementsBaseVertex(a0, a1, a2, a3, a4, a5, a6); }
void glBindFramebufferOES(GLenum a0, GLuint a1) { tglmt::gl::glBindFramebuffer(a0, a1); }
void glBindRenderbufferOES(GLenum a0, GLuint a1) { tglmt::gl::glBindRenderbuffer(a0, a1); }
GLenum glCheckFramebufferStatusOES(GLenum a0) { tglmt::gl::glCheckFramebufferStatus(a0); }
void glDeleteFramebuffersOES(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteFramebuffers(a0, a1); }
void glDeleteRenderbuffersOES(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteRenderbuffers(a0, a1); }
void glFramebufferRenderbufferOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3) { tglmt::gl::glFramebufferRenderbuffer(a0, a1, a2, a3); }
void glFramebufferTexture2DOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4) { tglmt::gl::glFramebufferTexture2D(a0, a1, a2, a3, a4); }
void glGenFramebuffersOES(GLsizei a0, GLuint * a1) { tglmt::gl::glGenFramebuffers(a0, a1); }
void glGenRenderbuffersOES(GLsizei a0, GLuint * a1) { tglmt::gl::glGenRenderbuffers(a0, a1); }
void glGenerateMipmapOES(GLenum a0) { tglmt::gl::glGenerateMipmap(a0); }
void glGetFramebufferAttachmentParameterivOES(GLenum a0, GLenum a1, GLenum a2, GLint * a3) { tglmt::gl::glGetFramebufferAttachmentParameteriv(a0, a1, a2, a3); }
void glGetRenderbufferParameterivOES(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetRenderbufferParameteriv(a0, a1, a2); }
GLboolean glIsFramebufferOES(GLuint a0) { tglmt::gl::glIsFramebuffer(a0); }
GLboolean glIsRenderbufferOES(GLuint a0) { tglmt::gl::glIsRenderbuffer(a0); }
void glRenderbufferStorageOES(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3) { tglmt::gl::glRenderbufferStorage(a0, a1, a2, a3); }
void glFramebufferTextureOES(GLenum a0, GLenum a1, GLuint a2, GLint a3) { tglmt::gl::glFramebufferTexture(a0, a1, a2, a3); }
void glGetProgramBinaryOES(GLuint a0, GLsizei a1, GLsizei * a2, GLenum * a3, void * a4) { tglmt::gl::glGetProgramBinary(a0, a1, a2, a3, a4); }
void glGetBufferPointervOES(GLenum a0, GLenum a1, void ** a2) { tglmt::gl::glGetBufferPointerv(a0, a1, a2); }
void * glMapBufferOES(GLenum a0, GLenum a1) { tglmt::gl::glMapBuffer(a0, a1); }
GLboolean glUnmapBufferOES(GLenum a0) { tglmt::gl::glUnmapBuffer(a0); }
void glMinSampleShadingOES(GLfloat a0) { tglmt::gl::glMinSampleShading(a0); }
void glPatchParameteriOES(GLenum a0, GLint a1) { tglmt::gl::glPatchParameteri(a0, a1); }
void glCompressedTexImage3DOES(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8) { tglmt::gl::glCompressedTexImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glCompressedTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10) { tglmt::gl::glCompressedTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
void glCopyTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8) { tglmt::gl::glCopyTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8); }
void glFramebufferTexture3DOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5) { tglmt::gl::glFramebufferTexture3D(a0, a1, a2, a3, a4, a5); }
void glTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10) { tglmt::gl::glTexSubImage3D(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }
void glGetSamplerParameterIivOES(GLuint a0, GLenum a1, GLint * a2) { tglmt::gl::glGetSamplerParameterIiv(a0, a1, a2); }
void glGetSamplerParameterIuivOES(GLuint a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetSamplerParameterIuiv(a0, a1, a2); }
void glGetTexParameterIivOES(GLenum a0, GLenum a1, GLint * a2) { tglmt::gl::glGetTexParameterIiv(a0, a1, a2); }
void glGetTexParameterIuivOES(GLenum a0, GLenum a1, GLuint * a2) { tglmt::gl::glGetTexParameterIuiv(a0, a1, a2); }
void glSamplerParameterIivOES(GLuint a0, GLenum a1, const GLint * a2) { tglmt::gl::glSamplerParameterIiv(a0, a1, a2); }
void glSamplerParameterIuivOES(GLuint a0, GLenum a1, const GLuint * a2) { tglmt::gl::glSamplerParameterIuiv(a0, a1, a2); }
void glTexParameterIivOES(GLenum a0, GLenum a1, const GLint * a2) { tglmt::gl::glTexParameterIiv(a0, a1, a2); }
void glTexParameterIuivOES(GLenum a0, GLenum a1, const GLuint * a2) { tglmt::gl::glTexParameterIuiv(a0, a1, a2); }
void glTexBufferOES(GLenum a0, GLenum a1, GLuint a2) { tglmt::gl::glTexBuffer(a0, a1, a2); }
void glTexBufferRangeOES(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4) { tglmt::gl::glTexBufferRange(a0, a1, a2, a3, a4); }
void glTexStorage3DMultisampleOES(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLboolean a6) { tglmt::gl::glTexStorage3DMultisample(a0, a1, a2, a3, a4, a5, a6); }
void glTextureViewOES(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7) { tglmt::gl::glTextureView(a0, a1, a2, a3, a4, a5, a6, a7); }
void glBindVertexArrayOES(GLuint a0) { tglmt::gl::glBindVertexArray(a0); }
void glDeleteVertexArraysOES(GLsizei a0, const GLuint * a1) { tglmt::gl::glDeleteVertexArrays(a0, a1); }
void glGenVertexArraysOES(GLsizei a0, GLuint * a1) { tglmt::gl::glGenVertexArrays(a0, a1); }
GLboolean glIsVertexArrayOES(GLuint a0) { tglmt::gl::glIsVertexArray(a0); }
} // extern "C"
