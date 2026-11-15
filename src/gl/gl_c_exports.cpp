// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.
// extern "C" forward 1:1 sang tglmt::gl:: để LWJGL dlsym("glXxx") thấy.
// Mỗi wrapper đo thời gian gọi (tglmt::GlProfileScope) để log gltop#
// chỉ ra hàm gl* nào nuốt frame time.
// Version-gate: entry point không có trong (version, profile) của context
// hiện tại ⇒ GL_INVALID_OPERATION, không thực thi (GL 4.6 spec §2.3).
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/GlProfile.h"
#include "tglmt/VersionGate.h"
using namespace tglmt;
namespace tglmt { namespace gl {
void glGetPointerv(GLenum p, void** v);
void glLineStipple(GLint a, GLushort b);
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c);
} }

extern "C" {
void glActiveProgramEXT(GLuint a0);
void glActiveShaderProgramEXT(GLuint a0, GLuint a1);
void glActiveStencilFaceEXT(GLenum a0);
void glActiveTextureARB(GLenum a0);
void glAttachObjectARB(GLuint a0, GLuint a1);
void glBeginConditionalRenderNV(GLuint a0, GLenum a1);
void glBeginPerfMonitorAMD(GLuint monitor);
void glBeginPerfQueryINTEL(GLuint queryHandle);
void glBeginQueryARB(GLenum a0, GLuint a1);
void glBeginQueryEXT(GLenum a0, GLuint a1);
void glBeginTransformFeedbackEXT(GLenum a0);
void glBeginTransformFeedbackNV(GLenum a0);
void glBindAttribLocationARB(GLuint a0, GLuint a1, const GLchar * a2);
void glBindBufferARB(GLenum a0, GLuint a1);
void glBindBufferBaseEXT(GLenum a0, GLuint a1, GLuint a2);
void glBindBufferBaseNV(GLenum a0, GLuint a1, GLuint a2);
void glBindBufferOffsetEXT(GLenum a0, GLuint a1, GLuint a2, GLintptr a3);
void glBindBufferRangeEXT(GLenum a0, GLuint a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glBindBufferRangeNV(GLenum a0, GLuint a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glBindFragDataLocationEXT(GLuint a0, GLuint a1, const GLchar * a2);
void glBindFragDataLocationIndexedEXT(GLuint a0, GLuint a1, GLuint a2, const GLchar * a3);
void glBindFramebufferEXT(GLenum a0, GLuint a1);
void glBindFramebufferOES(GLenum a0, GLuint a1);
void glBindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture);
void glBindProgramPipelineEXT(GLuint a0);
void glBindRenderbufferEXT(GLenum a0, GLuint a1);
void glBindRenderbufferOES(GLenum a0, GLuint a1);
void glBindShadingRateImageNV(GLuint texture);
void glBindTextureEXT(GLenum a0, GLuint a1);
void glBindTransformFeedbackNV(GLenum a0, GLuint a1);
void glBindVertexArrayAPPLE(GLuint a0);
void glBindVertexArrayOES(GLuint a0);
void glBlendBarrierKHR(void);
void glBlendBarrierNV(void);
void glBlendColorEXT(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glBlendEquationEXT(GLenum a0);
void glBlendEquationOES(GLenum a0);
void glBlendEquationSeparateEXT(GLenum a0, GLenum a1);
void glBlendEquationSeparateOES(GLenum a0, GLenum a1);
void glBlendEquationSeparateiARB(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationSeparateiEXT(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationSeparateiOES(GLuint a0, GLenum a1, GLenum a2);
void glBlendEquationiARB(GLuint a0, GLenum a1);
void glBlendEquationiEXT(GLuint a0, GLenum a1);
void glBlendEquationiOES(GLuint a0, GLenum a1);
void glBlendFuncSeparateEXT(GLenum a0, GLenum a1, GLenum a2, GLenum a3);
void glBlendFuncSeparateOES(GLenum a0, GLenum a1, GLenum a2, GLenum a3);
void glBlendFuncSeparateiARB(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFuncSeparateiEXT(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFuncSeparateiOES(GLuint a0, GLenum a1, GLenum a2, GLenum a3, GLenum a4);
void glBlendFunciARB(GLuint a0, GLenum a1, GLenum a2);
void glBlendFunciEXT(GLuint a0, GLenum a1, GLenum a2);
void glBlendFunciOES(GLuint a0, GLenum a1, GLenum a2);
void glBlendParameteriNV(GLenum pname, GLint value);
void glBlitFramebufferANGLE(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9);
void glBlitFramebufferEXT(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9);
void glBlitFramebufferNV(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLint a7, GLbitfield a8, GLenum a9);
void glBufferAddressRangeNV(GLenum pname, GLuint index, GLuint64 address, GLsizeiptr length);
void glBufferAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset);
void glBufferDataARB(GLenum a0, GLsizeiptr a1, const void * a2, GLenum a3);
void glBufferPageCommitmentARB(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit);
void glBufferStorageEXT(GLenum a0, GLsizeiptr a1, const void * a2, GLbitfield a3);
void glBufferSubDataARB(GLenum a0, GLintptr a1, GLsizeiptr a2, const void * a3);
void glCallCommandListNV(GLuint list);
GLenum glCheckFramebufferStatusEXT(GLenum a0);
GLenum glCheckFramebufferStatusOES(GLenum a0);
GLenum glCheckNamedFramebufferStatusEXT(GLuint a0, GLenum a1);
void glClampColorARB(GLenum a0, GLenum a1);
void glClearColorIiEXT(GLint a0, GLint a1, GLint a2, GLint a3);
void glClearColorIuiEXT(GLuint a0, GLuint a1, GLuint a2, GLuint a3);
void glClearDepthdNV(GLdouble depth);
void glClearNamedBufferDataEXT(GLuint a0, GLenum a1, GLenum a2, GLenum a3, const void * a4);
void glClearNamedBufferSubDataEXT(GLuint a0, GLenum a1, GLsizeiptr a2, GLsizeiptr a3, GLenum a4, GLenum a5, const void * a6);
void glClearTexImageEXT(GLuint a0, GLint a1, GLenum a2, GLenum a3, const void * a4);
void glClearTexSubImageEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glClientAttribDefaultEXT(GLbitfield mask);
GLenum glClientWaitSyncAPPLE(GLsync a0, GLbitfield a1, GLuint64 a2);
void glClipControlEXT(GLenum a0, GLenum a1);
void glColorFormatNV(GLint size, GLenum type, GLsizei stride);
void glColorMaskIndexedEXT(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4);
void glColorMaskiEXT(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4);
void glColorMaskiOES(GLuint a0, GLboolean a1, GLboolean a2, GLboolean a3, GLboolean a4);
void glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void* data);
void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* table);
void glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void glColorTableParameteriv(GLenum target, GLenum pname, const GLint* params);
void glCommandListSegmentsNV(GLuint list, GLuint segments);
void glCompileCommandListNV(GLuint list);
void glCompileShaderARB(GLuint a0);
void glCompileShaderIncludeARB(GLuint shader, GLsizei count, const GLchar* const* path, const GLint* length);
void glCompressedMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLint border, GLsizei imageSize, const void* bits);
void glCompressedMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* bits);
void glCompressedMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* bits);
void glCompressedMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLsizei imageSize, const void* bits);
void glCompressedMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* bits);
void glCompressedMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* bits);
void glCompressedTexImage1DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLint a4, GLsizei a5, const void * a6);
void glCompressedTexImage2DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLint a5, GLsizei a6, const void * a7);
void glCompressedTexImage3DARB(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8);
void glCompressedTexImage3DOES(GLenum a0, GLint a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLint a6, GLsizei a7, const void * a8);
void glCompressedTexSubImage1DARB(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLsizei a5, const void * a6);
void glCompressedTexSubImage2DARB(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLsizei a7, const void * a8);
void glCompressedTexSubImage3DARB(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10);
void glCompressedTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLsizei a9, const void * a10);
void glCompressedTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLint border, GLsizei imageSize, const void* bits);
void glCompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* bits);
void glCompressedTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* bits);
void glCompressedTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLsizei imageSize, const void* bits);
void glCompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* bits);
void glCompressedTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* bits);
void glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* image);
void glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* image);
void glConvolutionParameterf(GLenum target, GLenum pname, GLfloat param);
void glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void glConvolutionParameteri(GLenum target, GLenum pname, GLint param);
void glConvolutionParameteriv(GLenum target, GLenum pname, const GLint* params);
void glCopyBufferSubDataNV(GLenum a0, GLenum a1, GLintptr a2, GLintptr a3, GLsizeiptr a4);
void glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
void glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
void glCopyImageSubDataEXT(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14);
void glCopyImageSubDataNV(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14);
void glCopyImageSubDataOES(GLuint a0, GLenum a1, GLint a2, GLint a3, GLint a4, GLint a5, GLuint a6, GLenum a7, GLint a8, GLint a9, GLint a10, GLint a11, GLsizei a12, GLsizei a13, GLsizei a14);
void glCopyMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLint border);
void glCopyMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLint x, GLint y, GLsizei width);
void glCopyMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint x, GLint y, GLsizei width, GLsizei height);
void glCopyMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLint x, GLint y, GLsizei width, GLsizei height);
void glCopyPathNV(GLuint resultPath, GLuint srcPath);
void glCopyTexImage1DEXT(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLint a6);
void glCopyTexImage2DEXT(GLenum a0, GLint a1, GLenum a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLint a7);
void glCopyTexSubImage1DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5);
void glCopyTexSubImage2DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLsizei a6, GLsizei a7);
void glCopyTexSubImage3DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8);
void glCopyTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6, GLsizei a7, GLsizei a8);
void glCopyTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLint border);
void glCopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glCopyTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLint x, GLint y, GLsizei width);
void glCopyTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint x, GLint y, GLsizei width, GLsizei height);
void glCopyTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLint x, GLint y, GLsizei width, GLsizei height);
void glCoverFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum c, GLenum tr, const GLfloat* tv);
void glCoverFillPathNV(GLuint path, GLenum coverMode);
void glCoverStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum c, GLenum tr, const GLfloat* tv);
void glCoverStrokePathNV(GLuint path, GLenum coverMode);
void glCoverageModulationNV(GLenum components);
void glCoverageModulationTableNV(GLsizei n, const GLfloat* v);
void glCreateCommandListsNV(GLsizei n, GLuint* lists);
void glCreatePerfQueryINTEL(GLuint queryId, GLuint* queryHandle);
GLuint glCreateProgramObjectARB(void);
GLuint glCreateShaderObjectARB(GLenum a0);
GLuint glCreateShaderProgramEXT(GLenum a0, const GLchar * a1);
GLuint glCreateShaderProgramvEXT(GLenum a0, GLsizei a1, const GLchar *const* a2);
void glCreateStatesNV(GLsizei n, GLuint* states);
void glDebugMessageCallbackARB(GLDEBUGPROC a0, const void * a1);
void glDebugMessageCallbackKHR(GLDEBUGPROC a0, const void * a1);
void glDebugMessageControlARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, const GLuint * a4, GLboolean a5);
void glDebugMessageControlKHR(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, const GLuint * a4, GLboolean a5);
void glDebugMessageInsertARB(GLenum a0, GLenum a1, GLuint a2, GLenum a3, GLsizei a4, const GLchar * a5);
void glDebugMessageInsertKHR(GLenum a0, GLenum a1, GLuint a2, GLenum a3, GLsizei a4, const GLchar * a5);
void glDeleteBuffersARB(GLsizei a0, const GLuint * a1);
void glDeleteCommandListsNV(GLsizei n, const GLuint* lists);
void glDeleteFramebuffersEXT(GLsizei a0, const GLuint * a1);
void glDeleteFramebuffersOES(GLsizei a0, const GLuint * a1);
void glDeleteNamedStringARB(GLint namelen, const GLchar* name);
void glDeleteObjectARB(GLuint a0);
void glDeletePathsNV(GLuint path, GLsizei range);
void glDeletePerfMonitorsAMD(GLsizei n, GLuint* monitors);
void glDeletePerfQueryINTEL(GLuint queryHandle);
void glDeleteProgramPipelinesEXT(GLsizei a0, const GLuint * a1);
void glDeleteQueriesARB(GLsizei a0, const GLuint * a1);
void glDeleteQueriesEXT(GLsizei a0, const GLuint * a1);
void glDeleteRenderbuffersEXT(GLsizei a0, const GLuint * a1);
void glDeleteRenderbuffersOES(GLsizei a0, const GLuint * a1);
void glDeleteStatesNV(GLsizei n, const GLuint* states);
void glDeleteSyncAPPLE(GLsync a0);
void glDeleteTexturesEXT(GLsizei a0, const GLuint * a1);
void glDeleteTransformFeedbacksNV(GLsizei a0, const GLuint * a1);
void glDeleteVertexArraysAPPLE(GLsizei a0, const GLuint * a1);
void glDeleteVertexArraysOES(GLsizei a0, const GLuint * a1);
void glDepthBoundsdNV(GLdouble n, GLdouble f);
void glDepthRangeArraydvNV(GLuint a0, GLsizei a1, const GLdouble * a2);
void glDepthRangeArrayfvOES(GLuint a0, GLsizei a1, const GLfloat * a2);
void glDepthRangeIndexeddNV(GLuint a0, GLdouble a1, GLdouble a2);
void glDepthRangeIndexedfOES(GLuint a0, GLfloat a1, GLfloat a2);
void glDepthRangedNV(GLdouble n, GLdouble f);
void glDetachObjectARB(GLuint a0, GLuint a1);
void glDisableClientStateIndexedEXT(GLenum array, GLuint index);
void glDisableClientStateiEXT(GLenum array, GLuint index);
void glDisableIndexedEXT(GLenum a0, GLuint a1);
void glDisableVertexArrayAttribEXT(GLuint a0, GLuint a1);
void glDisableVertexArrayEXT(GLuint vaobj, GLenum array);
void glDisableVertexAttribArrayARB(GLuint a0);
void glDisableiEXT(GLenum a0, GLuint a1);
void glDisableiNV(GLenum a0, GLuint a1);
void glDisableiOES(GLenum a0, GLuint a1);
void glDiscardFramebufferEXT(GLenum a0, GLsizei a1, const GLenum * a2);
void glDispatchComputeGroupSizeARB(GLuint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4, GLuint a5);
void glDrawArraysEXT(GLenum a0, GLint a1, GLsizei a2);
void glDrawArraysInstancedANGLE(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawArraysInstancedARB(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawArraysInstancedBaseInstanceEXT(GLenum a0, GLint a1, GLsizei a2, GLsizei a3, GLuint a4);
void glDrawArraysInstancedEXT(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawArraysInstancedNV(GLenum a0, GLint a1, GLsizei a2, GLsizei a3);
void glDrawBuffersARB(GLsizei a0, const GLenum * a1);
void glDrawBuffersATI(GLsizei a0, const GLenum * a1);
void glDrawBuffersEXT(GLsizei a0, const GLenum * a1);
void glDrawBuffersNV(GLsizei a0, const GLenum * a1);
void glDrawCommandsAddressNV(GLenum mode, const GLuint64* indirects, const GLsizei* sizes, GLuint count);
void glDrawCommandsNV(GLenum mode, GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, GLuint count);
void glDrawCommandsStatesAddressNV(const GLuint64* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glDrawCommandsStatesNV(GLuint buffer, const GLintptr* indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glDrawElementsBaseVertexEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4);
void glDrawElementsBaseVertexOES(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLint a4);
void glDrawElementsInstancedANGLE(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glDrawElementsInstancedARB(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glDrawElementsInstancedBaseInstanceEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLuint a5);
void glDrawElementsInstancedBaseVertexBaseInstanceEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5, GLuint a6);
void glDrawElementsInstancedBaseVertexEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5);
void glDrawElementsInstancedBaseVertexOES(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4, GLint a5);
void glDrawElementsInstancedEXT(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glDrawElementsInstancedNV(GLenum a0, GLsizei a1, GLenum a2, const void * a3, GLsizei a4);
void glDrawRangeElementsBaseVertexEXT(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6);
void glDrawRangeElementsBaseVertexOES(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5, GLint a6);
void glDrawRangeElementsEXT(GLenum a0, GLuint a1, GLuint a2, GLsizei a3, GLenum a4, const void * a5);
void glDrawTransformFeedbackEXT(GLenum a0, GLuint a1);
void glDrawTransformFeedbackInstancedEXT(GLenum a0, GLuint a1, GLsizei a2);
void glDrawTransformFeedbackNV(GLenum a0, GLuint a1);
void glEdgeFlagFormatNV(GLsizei stride);
void glEnableClientStateIndexedEXT(GLenum array, GLuint index);
void glEnableClientStateiEXT(GLenum array, GLuint index);
void glEnableIndexedEXT(GLenum a0, GLuint a1);
void glEnableVertexArrayAttribEXT(GLuint a0, GLuint a1);
void glEnableVertexArrayEXT(GLuint vaobj, GLenum array);
void glEnableVertexAttribArrayARB(GLuint a0);
void glEnableiEXT(GLenum a0, GLuint a1);
void glEnableiNV(GLenum a0, GLuint a1);
void glEnableiOES(GLenum a0, GLuint a1);
void glEndConditionalRenderNV(void);
void glEndPerfMonitorAMD(GLuint monitor);
void glEndPerfQueryINTEL(GLuint queryHandle);
void glEndQueryARB(GLenum a0);
void glEndQueryEXT(GLenum a0);
void glEndTransformFeedbackEXT(void);
void glEndTransformFeedbackNV(void);
void glEvaluateDepthValuesARB(void);
GLsync glFenceSyncAPPLE(GLenum a0, GLbitfield a1);
void glFlushMappedBufferRangeAPPLE(GLenum a0, GLintptr a1, GLsizeiptr a2);
void glFlushMappedBufferRangeEXT(GLenum a0, GLintptr a1, GLsizeiptr a2);
void glFlushMappedNamedBufferRangeEXT(GLuint a0, GLintptr a1, GLsizeiptr a2);
void glFogCoordFormatNV(GLenum type, GLsizei stride);
void glFragmentCoverageColorNV(GLuint r, GLuint g, GLuint b);
void glFramebufferDrawBufferEXT(GLuint framebuffer, GLenum mode);
void glFramebufferDrawBuffersEXT(GLuint framebuffer, GLsizei n, const GLenum* bufs);
void glFramebufferFetchBarrierEXT(void);
void glFramebufferParameteriMESA(GLenum a0, GLenum a1, GLint a2);
void glFramebufferReadBufferEXT(GLuint framebuffer, GLenum mode);
void glFramebufferRenderbufferEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3);
void glFramebufferRenderbufferOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3);
void glFramebufferSampleLocationsfvARB(GLenum target, GLuint start, GLsizei count, const GLfloat* v);
void glFramebufferSampleLocationsfvNV(GLenum target, GLuint start, GLsizei count, const GLfloat* v);
void glFramebufferShadingRateEXT(GLenum target, GLenum attachment, GLuint texture,
        GLint baseLayer, GLsizei numLayers, GLsizei texelWidth, GLsizei texelHeight);
void glFramebufferTexture1DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glFramebufferTexture2DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glFramebufferTexture2DMultisampleEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLsizei a5);
void glFramebufferTexture2DOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4);
void glFramebufferTexture3DEXT(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5);
void glFramebufferTexture3DOES(GLenum a0, GLenum a1, GLenum a2, GLuint a3, GLint a4, GLint a5);
void glFramebufferTextureARB(GLenum a0, GLenum a1, GLuint a2, GLint a3);
void glFramebufferTextureEXT(GLenum a0, GLenum a1, GLuint a2, GLint a3);
void glFramebufferTextureFaceARB(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLenum a4);
void glFramebufferTextureFaceEXT(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLenum a4);
void glFramebufferTextureLayerARB(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLint a4);
void glFramebufferTextureLayerEXT(GLenum a0, GLenum a1, GLuint a2, GLint a3, GLint a4);
void glFramebufferTextureMultiviewOVR(GLenum target, GLenum attachment, GLuint texture,
        GLint level, GLint baseViewIndex, GLsizei numViews);
void glFramebufferTextureOES(GLenum a0, GLenum a1, GLuint a2, GLint a3);
void glGenBuffersARB(GLsizei a0, GLuint * a1);
void glGenFramebuffersEXT(GLsizei a0, GLuint * a1);
void glGenFramebuffersOES(GLsizei a0, GLuint * a1);
GLuint glGenPathsNV(GLsizei range);
void glGenPerfMonitorsAMD(GLsizei n, GLuint* monitors);
void glGenProgramPipelinesEXT(GLsizei a0, GLuint * a1);
void glGenQueriesARB(GLsizei a0, GLuint * a1);
void glGenQueriesEXT(GLsizei a0, GLuint * a1);
void glGenRenderbuffersEXT(GLsizei a0, GLuint * a1);
void glGenRenderbuffersOES(GLsizei a0, GLuint * a1);
void glGenTexturesEXT(GLsizei a0, GLuint * a1);
void glGenTransformFeedbacksNV(GLsizei a0, GLuint * a1);
void glGenVertexArraysAPPLE(GLsizei a0, GLuint * a1);
void glGenVertexArraysOES(GLsizei a0, GLuint * a1);
void glGenerateMipmapEXT(GLenum a0);
void glGenerateMipmapOES(GLenum a0);
void glGenerateMultiTexMipmapEXT(GLenum texunit, GLenum target);
void glGenerateTextureMipmapEXT(GLuint texture, GLenum target);
void glGetActiveAttribARB(GLuint a0, GLuint a1, GLsizei a2, GLsizei * a3, GLint * a4, GLenum * a5, GLchar * a6);
void glGetActiveUniformARB(GLuint a0, GLuint a1, GLsizei a2, GLsizei * a3, GLint * a4, GLenum * a5, GLchar * a6);
void glGetAttachedObjectsARB(GLuint a0, GLsizei a1, GLsizei * a2, GLuint * a3);
GLint glGetAttribLocationARB(GLuint a0, const GLchar * a1);
void glGetBooleanIndexedvEXT(GLenum a0, GLuint a1, GLboolean * a2);
void glGetBufferParameterivARB(GLenum a0, GLenum a1, GLint * a2);
void glGetBufferParameterui64vNV(GLenum target, GLenum pname, GLuint64* params);
void glGetBufferPointervARB(GLenum a0, GLenum a1, void ** a2);
void glGetBufferPointervOES(GLenum a0, GLenum a1, void ** a2);
void glGetBufferSubDataARB(GLenum a0, GLintptr a1, GLsizeiptr a2, void * a3);
void glGetColorTable(GLenum target, GLenum format, GLenum type, void* table);
void glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetColorTableParameteriv(GLenum target, GLenum pname, GLint* params);
GLuint glGetCommandHeaderNV(GLenum tokenID, GLuint size);
void glGetCompressedMultiTexImageEXT(GLenum texunit, GLenum target, GLint lod, void* img);
void glGetCompressedTexImageARB(GLenum a0, GLint a1, void * a2);
void glGetCompressedTextureImageEXT(GLuint texture, GLenum target, GLint lod, void* img);
void glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, void* image);
void glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetCoverageModulationTableNV(GLsizei bufsize, GLfloat* v);
GLuint glGetDebugMessageLogARB(GLuint a0, GLsizei a1, GLenum * a2, GLenum * a3, GLuint * a4, GLenum * a5, GLsizei * a6, GLchar * a7);
GLuint glGetDebugMessageLogKHR(GLuint a0, GLsizei a1, GLenum * a2, GLenum * a3, GLuint * a4, GLenum * a5, GLsizei * a6, GLchar * a7);
void glGetDoubleIndexedvEXT(GLenum target, GLuint index, GLdouble* data);
void glGetDoublei_vEXT(GLenum a0, GLuint a1, GLdouble * a2);
void glGetFirstPerfQueryIdINTEL(GLuint* queryId);
void glGetFloatIndexedvEXT(GLenum target, GLuint index, GLfloat* data);
void glGetFloati_vEXT(GLenum a0, GLuint a1, GLfloat * a2);
void glGetFloati_vNV(GLenum a0, GLuint a1, GLfloat * a2);
void glGetFloati_vOES(GLenum a0, GLuint a1, GLfloat * a2);
GLint glGetFragDataIndexEXT(GLuint a0, const GLchar * a1);
GLint glGetFragDataLocationEXT(GLuint a0, const GLchar * a1);
void glGetFragmentShadingRatesEXT(GLsizei samples, GLsizei maxCount, GLsizei* count, GLenum* shadingRates);
void glGetFramebufferAttachmentParameterivEXT(GLenum a0, GLenum a1, GLenum a2, GLint * a3);
void glGetFramebufferAttachmentParameterivOES(GLenum a0, GLenum a1, GLenum a2, GLint * a3);
void glGetFramebufferParameterivEXT(GLuint framebuffer, GLenum pname, GLint* params);
void glGetFramebufferParameterivMESA(GLenum a0, GLenum a1, GLint * a2);
GLenum glGetGraphicsResetStatusARB(void);
GLenum glGetGraphicsResetStatusEXT(void);
GLenum glGetGraphicsResetStatusKHR(void);
GLuint glGetHandleARB(GLenum a0);
void glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values);
void glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetHistogramParameteriv(GLenum target, GLenum pname, GLint* params);
GLuint64 glGetImageHandleARB(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
GLuint64 glGetImageHandleNV(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
void glGetInfoLogARB(GLuint a0, GLsizei a1, GLsizei * a2, GLchar * a3);
void glGetInteger64vAPPLE(GLenum a0, GLint64 * a1);
void glGetInteger64vEXT(GLenum a0, GLint64 * a1);
void glGetIntegerIndexedvEXT(GLenum a0, GLuint a1, GLint * a2);
void glGetIntegeri_vEXT(GLenum a0, GLuint a1, GLint * a2);
void glGetIntegerui64i_vNV(GLenum value, GLuint index, GLuint64* result);
void glGetIntegerui64vNV(GLenum value, GLuint64* result);
void glGetInternalformatSampleivNV(GLenum target, GLenum internalformat, GLsizei samples,
        GLenum pname, GLsizei bufSize, GLint* params);
void glGetMemoryObjectDetachedResourcesuivNV(GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint* params);
void glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values);
void glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat* params);
void glGetMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params);
void glGetMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble* params);
void glGetMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat* params);
void glGetMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, GLint* params);
void glGetMultiTexImageEXT(GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void* pixels);
void glGetMultiTexLevelParameterfvEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat* params);
void glGetMultiTexLevelParameterivEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLint* params);
void glGetMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params);
void glGetMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, GLuint* params);
void glGetMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat* params);
void glGetMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params);
void glGetMultisamplefvNV(GLenum a0, GLuint a1, GLfloat * a2);
void glGetNamedBufferParameterivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetNamedBufferParameterui64vNV(GLuint buffer, GLenum pname, GLuint64* params);
void glGetNamedBufferPointervEXT(GLuint a0, GLenum a1, void ** a2);
void glGetNamedBufferSubDataEXT(GLuint a0, GLintptr a1, GLsizeiptr a2, void * a3);
void glGetNamedFramebufferAttachmentParameterivEXT(GLuint a0, GLenum a1, GLenum a2, GLint * a3);
void glGetNamedFramebufferParameterivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetNamedProgramLocalParameterIivEXT(GLuint program, GLenum target, GLuint index, GLint* params);
void glGetNamedProgramLocalParameterIuivEXT(GLuint program, GLenum target, GLuint index, GLuint* params);
void glGetNamedProgramLocalParameterdvEXT(GLuint program, GLenum target, GLuint index, GLdouble* params);
void glGetNamedProgramLocalParameterfvEXT(GLuint program, GLenum target, GLuint index, GLfloat* params);
void glGetNamedProgramStringEXT(GLuint program, GLenum target, GLenum pname, void* string);
void glGetNamedProgramivEXT(GLuint program, GLenum target, GLenum pname, GLint* params);
void glGetNamedRenderbufferParameterivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetNamedStringARB(GLint namelen, const GLchar* name, GLsizei bufSize, GLint* stringlen, GLchar* string);
void glGetNamedStringivARB(GLint namelen, const GLchar* name, GLenum pname, GLint* params);
void glGetNextPerfQueryIdINTEL(GLuint queryId, GLuint* nextQueryId);
void glGetObjectLabelEXT(GLenum a0, GLuint a1, GLsizei a2, GLsizei * a3, GLchar * a4);
void glGetObjectLabelKHR(GLenum a0, GLuint a1, GLsizei a2, GLsizei * a3, GLchar * a4);
void glGetObjectParameterfvARB(GLuint a0, GLenum a1, GLfloat * a2);
void glGetObjectParameterivARB(GLuint a0, GLenum a1, GLint * a2);
void glGetObjectPtrLabelKHR(const void * a0, GLsizei a1, GLsizei * a2, GLchar * a3);
void glGetPathColorGenfvNV(GLenum color, GLenum pname, GLfloat* value);
void glGetPathColorGenivNV(GLenum color, GLenum pname, GLint* value);
void glGetPathCommandsNV(GLuint path, GLubyte* commands);
void glGetPathCoordsNV(GLuint path, GLfloat* coords);
void glGetPathDashArrayNV(GLuint path, GLfloat* dashArray);
GLfloat glGetPathLengthNV(GLuint path, GLsizei startSegment, GLsizei numSegments);
void glGetPathMetricRangeNV(GLbitfield mask, GLuint first, GLsizei numPaths, GLsizei stride, GLfloat* metrics);
void glGetPathMetricsNV(GLbitfield mask, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLsizei stride, GLfloat* metrics);
void glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat* value);
void glGetPathParameterivNV(GLuint path, GLenum pname, GLint* value);
void glGetPathSpacingNV(GLenum mode, GLsizei numPaths, GLenum pathNameType, const void* paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat* returnedSpacing);
void glGetPathTexGenfvNV(GLenum texCoordSet, GLenum pname, GLfloat* value);
void glGetPathTexGenivNV(GLenum texCoordSet, GLenum pname, GLint* value);
void glGetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar* counterName,
        GLuint counterDescLength, GLchar* counterDesc, GLuint* counterOffset, GLuint* counterDataSize,
        GLuint* counterTypeEnum, GLuint* counterDataTypeEnum, GLuint64* rawCounterMaxValue);
void glGetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten);
void glGetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname, void* data);
void glGetPerfMonitorCounterStringAMD(GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, GLchar* counterString);
void glGetPerfMonitorCountersAMD(GLuint group, GLint* numCounters, GLint* maxActiveCounters,
        GLsizei counterSize, GLuint* counters);
void glGetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize, GLsizei* length, GLchar* groupString);
void glGetPerfMonitorGroupsAMD(GLint* numGroups, GLsizei groupsSize, GLuint* groups);
void glGetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags, GLsizei dataSize, void* data, GLuint* bytesWritten);
void glGetPerfQueryIdByNameINTEL(GLchar* queryName, GLuint* queryId);
void glGetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength, GLchar* queryName, GLuint* dataSize,
        GLuint* noCounters, GLuint* noInstances, GLuint* capsMask);
void glGetPointerIndexedvEXT(GLenum target, GLuint index, void** data);
void glGetPointeri_vEXT(GLenum pname, GLuint index, void** params);
void glGetPointervEXT(GLenum a0, void ** a1);
void glGetPointervKHR(GLenum a0, void ** a1);
void glGetProgramBinaryOES(GLuint a0, GLsizei a1, GLsizei * a2, GLenum * a3, void * a4);
void glGetProgramPipelineInfoLogEXT(GLuint a0, GLsizei a1, GLsizei * a2, GLchar * a3);
void glGetProgramPipelineivEXT(GLuint a0, GLenum a1, GLint * a2);
GLint glGetProgramResourceLocationIndexEXT(GLuint a0, GLenum a1, const GLchar * a2);
void glGetProgramResourcefvNV(GLuint program, GLenum iface, GLuint index, GLsizei propCount, const GLenum* props, GLsizei count, GLsizei* length, GLfloat* params);
void glGetProgramivNV(GLuint a0, GLenum a1, GLint * a2);
void glGetQueryObjecti64vEXT(GLuint a0, GLenum a1, GLint64 * a2);
void glGetQueryObjectivARB(GLuint a0, GLenum a1, GLint * a2);
void glGetQueryObjectivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetQueryObjectui64vEXT(GLuint a0, GLenum a1, GLuint64 * a2);
void glGetQueryObjectuivARB(GLuint a0, GLenum a1, GLuint * a2);
void glGetQueryObjectuivEXT(GLuint a0, GLenum a1, GLuint * a2);
void glGetQueryivARB(GLenum a0, GLenum a1, GLint * a2);
void glGetQueryivEXT(GLenum a0, GLenum a1, GLint * a2);
void glGetRenderbufferParameterivEXT(GLenum a0, GLenum a1, GLint * a2);
void glGetRenderbufferParameterivOES(GLenum a0, GLenum a1, GLint * a2);
void glGetSamplerParameterIivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetSamplerParameterIivOES(GLuint a0, GLenum a1, GLint * a2);
void glGetSamplerParameterIuivEXT(GLuint a0, GLenum a1, GLuint * a2);
void glGetSamplerParameterIuivOES(GLuint a0, GLenum a1, GLuint * a2);
void glGetSeparableFilter(GLenum target, GLenum format, GLenum type, void* row, void* column, void* span);
void glGetShaderSourceARB(GLuint a0, GLsizei a1, GLsizei * a2, GLchar * a3);
void glGetShadingRateImagePaletteNV(GLuint viewport, GLuint entry, GLenum* rate);
void glGetShadingRateSampleLocationivNV(GLenum rate, GLuint samples, GLuint index, GLint* location);
GLushort glGetStageIndexNV(GLenum shadertype);
void glGetSyncivAPPLE(GLsync a0, GLenum a1, GLsizei a2, GLsizei * a3, GLint * a4);
void glGetTexParameterIivEXT(GLenum a0, GLenum a1, GLint * a2);
void glGetTexParameterIivOES(GLenum a0, GLenum a1, GLint * a2);
void glGetTexParameterIuivEXT(GLenum a0, GLenum a1, GLuint * a2);
void glGetTexParameterIuivOES(GLenum a0, GLenum a1, GLuint * a2);
GLuint64 glGetTextureHandleARB(GLuint texture);
GLuint64 glGetTextureHandleNV(GLuint texture);
void glGetTextureImageEXT(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void* pixels);
void glGetTextureLevelParameterfvEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat* params);
void glGetTextureLevelParameterivEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params);
void glGetTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, GLint* params);
void glGetTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, GLuint* params);
void glGetTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, GLfloat* params);
void glGetTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, GLint* params);
GLuint64 glGetTextureSamplerHandleARB(GLuint texture, GLuint sampler);
GLuint64 glGetTextureSamplerHandleNV(GLuint texture, GLuint sampler);
void glGetTransformFeedbackVaryingEXT(GLuint a0, GLuint a1, GLsizei a2, GLsizei * a3, GLsizei * a4, GLenum * a5, GLchar * a6);
GLint glGetUniformLocationARB(GLuint a0, const GLchar * a1);
void glGetUniformfvARB(GLuint a0, GLint a1, GLfloat * a2);
void glGetUniformi64vARB(GLuint p, GLint l, GLint64* v);
void glGetUniformi64vNV(GLuint p, GLint l, GLint64* v);
void glGetUniformivARB(GLuint a0, GLint a1, GLint * a2);
void glGetUniformui64vARB(GLuint p, GLint l, GLuint64* v);
void glGetUniformui64vNV(GLuint p, GLint l, GLuint64* v);
void glGetUniformuivEXT(GLuint a0, GLint a1, GLuint * a2);
void glGetVertexArrayIntegeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, GLint* param);
void glGetVertexArrayIntegervEXT(GLuint vaobj, GLenum pname, GLint* param);
void glGetVertexArrayPointeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, void** param);
void glGetVertexArrayPointervEXT(GLuint vaobj, GLenum pname, void** param);
void glGetVertexAttribIivEXT(GLuint a0, GLenum a1, GLint * a2);
void glGetVertexAttribIuivEXT(GLuint a0, GLenum a1, GLuint * a2);
void glGetVertexAttribLdvEXT(GLuint a0, GLenum a1, GLdouble * a2);
void glGetVertexAttribLi64vNV(GLuint i, GLenum p, GLint64* v);
void glGetVertexAttribLui64vARB(GLuint index, GLenum pname, GLuint64* params);
void glGetVertexAttribLui64vNV(GLuint i, GLenum p, GLuint64* v);
void glGetVertexAttribPointervARB(GLuint a0, GLenum a1, void ** a2);
void glGetVertexAttribPointervNV(GLuint a0, GLenum a1, void ** a2);
void glGetVertexAttribdvARB(GLuint a0, GLenum a1, GLdouble * a2);
void glGetVertexAttribdvNV(GLuint a0, GLenum a1, GLdouble * a2);
void glGetVertexAttribfvARB(GLuint a0, GLenum a1, GLfloat * a2);
void glGetVertexAttribfvNV(GLuint a0, GLenum a1, GLfloat * a2);
void glGetVertexAttribivARB(GLuint a0, GLenum a1, GLint * a2);
void glGetVertexAttribivNV(GLuint a0, GLenum a1, GLint * a2);
void glGetnColorTableARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4);
void glGetnCompressedTexImageARB(GLenum a0, GLint a1, GLsizei a2, void * a3);
void glGetnConvolutionFilterARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4);
void glGetnHistogramARB(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnMapdvARB(GLenum a0, GLenum a1, GLsizei a2, GLdouble * a3);
void glGetnMapfvARB(GLenum a0, GLenum a1, GLsizei a2, GLfloat * a3);
void glGetnMapivARB(GLenum a0, GLenum a1, GLsizei a2, GLint * a3);
void glGetnMinmaxARB(GLenum a0, GLboolean a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnPixelMapfvARB(GLenum a0, GLsizei a1, GLfloat * a2);
void glGetnPixelMapuivARB(GLenum a0, GLsizei a1, GLuint * a2);
void glGetnPixelMapusvARB(GLenum a0, GLsizei a1, GLushort * a2);
void glGetnPolygonStippleARB(GLsizei a0, GLubyte * a1);
void glGetnSeparableFilterARB(GLenum a0, GLenum a1, GLenum a2, GLsizei a3, void * a4, GLsizei a5, void * a6, void * a7);
void glGetnTexImageARB(GLenum a0, GLint a1, GLenum a2, GLenum a3, GLsizei a4, void * a5);
void glGetnUniformdvARB(GLuint a0, GLint a1, GLsizei a2, GLdouble * a3);
void glGetnUniformfvARB(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformfvEXT(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformfvKHR(GLuint a0, GLint a1, GLsizei a2, GLfloat * a3);
void glGetnUniformi64vARB(GLuint p, GLint l, GLsizei n, GLint64* v);
void glGetnUniformivARB(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glGetnUniformivEXT(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glGetnUniformivKHR(GLuint a0, GLint a1, GLsizei a2, GLint * a3);
void glGetnUniformui64vARB(GLuint p, GLint l, GLsizei n, GLuint64* v);
void glGetnUniformuivARB(GLuint a0, GLint a1, GLsizei a2, GLuint * a3);
void glGetnUniformuivKHR(GLuint a0, GLint a1, GLsizei a2, GLuint * a3);
void glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
void glIndexFormatNV(GLenum type, GLsizei stride);
void glInsertEventMarkerEXT(GLsizei a0, const GLchar * a1);
void glInterpolatePathsNV(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
GLboolean glIsBufferARB(GLuint a0);
GLboolean glIsBufferResidentNV(GLenum target);
GLboolean glIsCommandListNV(GLuint list);
GLboolean glIsEnabledIndexedEXT(GLenum a0, GLuint a1);
GLboolean glIsEnablediEXT(GLenum a0, GLuint a1);
GLboolean glIsEnablediNV(GLenum a0, GLuint a1);
GLboolean glIsEnablediOES(GLenum a0, GLuint a1);
GLboolean glIsFramebufferEXT(GLuint a0);
GLboolean glIsFramebufferOES(GLuint a0);
GLboolean glIsImageHandleResidentARB(GLuint64 handle);
GLboolean glIsImageHandleResidentNV(GLuint64 handle);
GLboolean glIsNamedBufferResidentNV(GLuint buffer);
GLboolean glIsNamedStringARB(GLint namelen, const GLchar* name);
GLboolean glIsPathNV(GLuint path);
GLboolean glIsPointInFillPathNV(GLuint path, GLuint mask, GLfloat x, GLfloat y);
GLboolean glIsPointInStrokePathNV(GLuint path, GLfloat x, GLfloat y);
GLboolean glIsProgramARB(GLuint a0);
GLboolean glIsProgramNV(GLuint a0);
GLboolean glIsProgramPipelineEXT(GLuint a0);
GLboolean glIsQueryARB(GLuint a0);
GLboolean glIsQueryEXT(GLuint a0);
GLboolean glIsRenderbufferEXT(GLuint a0);
GLboolean glIsRenderbufferOES(GLuint a0);
GLboolean glIsStateNV(GLuint state);
GLboolean glIsSyncAPPLE(GLsync a0);
GLboolean glIsTextureEXT(GLuint a0);
GLboolean glIsTextureHandleResidentARB(GLuint64 handle);
GLboolean glIsTextureHandleResidentNV(GLuint64 handle);
GLboolean glIsTransformFeedbackNV(GLuint a0);
GLboolean glIsVertexArrayAPPLE(GLuint a0);
GLboolean glIsVertexArrayOES(GLuint a0);
void glLabelObjectEXT(GLenum a0, GLuint a1, GLsizei a2, const GLchar * a3);
void glLinkProgramARB(GLuint a0);
void glListDrawCommandsStatesClientNV(GLuint list, GLuint segment, const void** indirects, const GLsizei* sizes, const GLuint* states, const GLuint* fbos, GLuint count);
void glMakeBufferNonResidentNV(GLenum target);
void glMakeBufferResidentNV(GLenum target, GLenum access);
void glMakeImageHandleNonResidentARB(GLuint64 handle);
void glMakeImageHandleNonResidentNV(GLuint64 handle);
void glMakeImageHandleResidentARB(GLuint64 handle, GLenum access);
void glMakeImageHandleResidentNV(GLuint64 handle, GLenum access);
void glMakeNamedBufferNonResidentNV(GLuint buffer);
void glMakeNamedBufferResidentNV(GLuint buffer, GLenum access);
void glMakeTextureHandleNonResidentARB(GLuint64 handle);
void glMakeTextureHandleNonResidentNV(GLuint64 handle);
void glMakeTextureHandleResidentARB(GLuint64 handle);
void glMakeTextureHandleResidentNV(GLuint64 handle);
void * glMapBufferARB(GLenum a0, GLenum a1);
void * glMapBufferOES(GLenum a0, GLenum a1);
void * glMapBufferRangeEXT(GLenum a0, GLintptr a1, GLsizeiptr a2, GLbitfield a3);
void * glMapNamedBufferEXT(GLuint a0, GLenum a1);
void * glMapNamedBufferRangeEXT(GLuint a0, GLintptr a1, GLsizeiptr a2, GLbitfield a3);
void glMatrixFrustumEXT(GLenum mode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void glMatrixLoad3x2fNV(GLenum mode, const GLfloat* m);
void glMatrixLoad3x3fNV(GLenum mode, const GLfloat* m);
void glMatrixLoadIdentityEXT(GLenum mode);
void glMatrixLoadTranspose3x3fNV(GLenum mode, const GLfloat* m);
void glMatrixLoadTransposedEXT(GLenum mode, const GLdouble* m);
void glMatrixLoadTransposefEXT(GLenum mode, const GLfloat* m);
void glMatrixLoaddEXT(GLenum mode, const GLdouble* m);
void glMatrixLoadfEXT(GLenum mode, const GLfloat* m);
void glMatrixMult3x2fNV(GLenum mode, const GLfloat* m);
void glMatrixMult3x3fNV(GLenum mode, const GLfloat* m);
void glMatrixMultTranspose3x3fNV(GLenum mode, const GLfloat* m);
void glMatrixMultTransposedEXT(GLenum mode, const GLdouble* m);
void glMatrixMultTransposefEXT(GLenum mode, const GLfloat* m);
void glMatrixMultdEXT(GLenum mode, const GLdouble* m);
void glMatrixMultfEXT(GLenum mode, const GLfloat* m);
void glMatrixOrthoEXT(GLenum mode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f);
void glMatrixPopEXT(GLenum mode);
void glMatrixPushEXT(GLenum mode);
void glMatrixRotatedEXT(GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void glMatrixRotatefEXT(GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glMatrixScaledEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
void glMatrixScalefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
void glMatrixTranslatedEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
void glMatrixTranslatefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
void glMaxShaderCompilerThreadsARB(GLuint a0);
void glMaxShaderCompilerThreadsKHR(GLuint a0);
void glMemoryBarrierEXT(GLbitfield a0);
void glMinSampleShadingARB(GLfloat a0);
void glMinSampleShadingOES(GLfloat a0);
void glMinmax(GLenum target, GLenum internalformat, GLboolean sink);
void glMultiDrawArraysEXT(GLenum a0, const GLint * a1, const GLsizei * a2, GLsizei a3);
void glMultiDrawArraysIndirectAMD(GLenum a0, const void * a1, GLsizei a2, GLsizei a3);
void glMultiDrawArraysIndirectCountARB(GLenum a0, const void * a1, GLintptr a2, GLsizei a3, GLsizei a4);
void glMultiDrawArraysIndirectEXT(GLenum a0, const void * a1, GLsizei a2, GLsizei a3);
void glMultiDrawElementsBaseVertexEXT(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4, const GLint * a5);
void glMultiDrawElementsEXT(GLenum a0, const GLsizei * a1, GLenum a2, const void *const* a3, GLsizei a4);
void glMultiDrawElementsIndirectAMD(GLenum a0, GLenum a1, const void * a2, GLsizei a3, GLsizei a4);
void glMultiDrawElementsIndirectCountARB(GLenum a0, GLenum a1, const void * a2, GLintptr a3, GLsizei a4, GLsizei a5);
void glMultiDrawElementsIndirectEXT(GLenum a0, GLenum a1, const void * a2, GLsizei a3, GLsizei a4);
void glMultiTexBufferEXT(GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer);
void glMultiTexCoordPointerEXT(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void* pointer);
void glMultiTexEnvfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
void glMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat* params);
void glMultiTexEnviEXT(GLenum texunit, GLenum target, GLenum pname, GLint param);
void glMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params);
void glMultiTexGendEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble param);
void glMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLdouble* params);
void glMultiTexGenfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param);
void glMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params);
void glMultiTexGeniEXT(GLenum texunit, GLenum coord, GLenum pname, GLint param);
void glMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint* params);
void glMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels);
void glMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void glMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
void glMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params);
void glMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, const GLuint* params);
void glMultiTexParameterfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
void glMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat* params);
void glMultiTexParameteriEXT(GLenum texunit, GLenum target, GLenum pname, GLint param);
void glMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params);
void glMultiTexRenderbufferEXT(GLenum texunit, GLenum target, GLuint renderbuffer);
void glMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLenum type, const void* pixels);
void glMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
void glMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
void glNamedBufferAttachMemoryNV(GLuint buffer, GLuint memory, GLuint64 offset);
void glNamedBufferDataEXT(GLuint a0, GLsizeiptr a1, const void * a2, GLenum a3);
void glNamedBufferPageCommitmentARB(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit);
void glNamedBufferPageCommitmentEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit);
void glNamedBufferStorageEXT(GLuint a0, GLsizeiptr a1, const void * a2, GLbitfield a3);
void glNamedBufferSubDataEXT(GLuint a0, GLintptr a1, GLsizeiptr a2, const void * a3);
void glNamedCopyBufferSubDataEXT(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset,
        GLintptr writeOffset, GLsizeiptr size);
void glNamedFramebufferParameteriEXT(GLuint a0, GLenum a1, GLint a2);
void glNamedFramebufferRenderbufferEXT(GLuint a0, GLenum a1, GLenum a2, GLuint a3);
void glNamedFramebufferSampleLocationsfvARB(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat* v);
void glNamedFramebufferSampleLocationsfvNV(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat* v);
void glNamedFramebufferTexture1DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level);
void glNamedFramebufferTexture2DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level);
void glNamedFramebufferTexture3DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level, GLint zoffset);
void glNamedFramebufferTextureEXT(GLuint a0, GLenum a1, GLuint a2, GLint a3);
void glNamedFramebufferTextureFaceEXT(GLuint framebuffer, GLenum attachment, GLuint texture,
        GLint level, GLenum face);
void glNamedFramebufferTextureLayerEXT(GLuint a0, GLenum a1, GLuint a2, GLint a3, GLint a4);
void glNamedFramebufferTextureMultiviewOVR(GLuint framebuffer, GLenum attachment, GLuint texture,
        GLint level, GLint baseViewIndex, GLsizei numViews);
void glNamedProgramLocalParameter4dEXT(GLuint program, GLenum target, GLuint index,
        GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void glNamedProgramLocalParameter4dvEXT(GLuint program, GLenum target, GLuint index, const GLdouble* params);
void glNamedProgramLocalParameter4fEXT(GLuint program, GLenum target, GLuint index,
        GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void glNamedProgramLocalParameter4fvEXT(GLuint program, GLenum target, GLuint index, const GLfloat* params);
void glNamedProgramLocalParameterI4iEXT(GLuint program, GLenum target, GLuint index,
        GLint x, GLint y, GLint z, GLint w);
void glNamedProgramLocalParameterI4ivEXT(GLuint program, GLenum target, GLuint index, const GLint* params);
void glNamedProgramLocalParameterI4uiEXT(GLuint program, GLenum target, GLuint index,
        GLuint x, GLuint y, GLuint z, GLuint w);
void glNamedProgramLocalParameterI4uivEXT(GLuint program, GLenum target, GLuint index, const GLuint* params);
void glNamedProgramLocalParameters4fvEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat* params);
void glNamedProgramLocalParametersI4ivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLint* params);
void glNamedProgramLocalParametersI4uivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint* params);
void glNamedProgramStringEXT(GLuint program, GLenum target, GLenum format, GLsizei len, const void* string);
void glNamedRenderbufferStorageEXT(GLuint a0, GLenum a1, GLsizei a2, GLsizei a3);
void glNamedRenderbufferStorageMultisampleAdvancedAMD(GLuint renderbuffer, GLsizei samples, GLsizei rbsamples,
        GLenum internalformat, GLsizei width, GLsizei height);
void glNamedRenderbufferStorageMultisampleCoverageEXT(GLuint renderbuffer, GLsizei coverageSamples,
        GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);
void glNamedRenderbufferStorageMultisampleEXT(GLuint a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glNamedStringARB(GLenum type, GLint namelen, const GLchar* name, GLint stringlen, const GLchar* string);
void glNormalFormatNV(GLenum type, GLsizei stride);
void glObjectLabelKHR(GLenum a0, GLuint a1, GLsizei a2, const GLchar * a3);
void glObjectPtrLabelKHR(const void * a0, GLsizei a1, const GLchar * a2);
void glPatchParameteriEXT(GLenum a0, GLint a1);
void glPatchParameteriOES(GLenum a0, GLint a1);
void glPathColorGenNV(GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat* coeffs);
void glPathCommandsNV(GLuint path, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathCoordsNV(GLuint path, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathCoverDepthFuncNV(GLenum func);
void glPathDashArrayNV(GLuint path, GLsizei dashCount, const GLfloat* dashArray);
void glPathFogGenNV(GLenum genMode);
GLenum glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GLenum glPathGlyphIndexRangeNV(GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint* baseAndCount);
void glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
void glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget, const void* fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void* charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
GLenum glPathMemoryGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void* fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
void glPathParameterfNV(GLuint path, GLenum pname, GLfloat value);
void glPathParameterfvNV(GLuint path, GLenum pname, const GLfloat* value);
void glPathParameteriNV(GLuint path, GLenum pname, GLint value);
void glPathParameterivNV(GLuint path, GLenum pname, const GLint* value);
void glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units);
void glPathStencilFuncNV(GLenum func, GLint ref, GLuint mask);
void glPathStringNV(GLuint path, GLenum format, GLsizei length, const void* pathString);
void glPathSubCommandsNV(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte* commands, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathSubCoordsNV(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void* coords);
void glPathTexGenNV(GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat* coeffs);
void glPauseTransformFeedbackNV(void);
GLboolean glPointAlongPathNV(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat* x, GLfloat* y, GLfloat* tx, GLfloat* ty);
void glPointParameterfARB(GLenum a0, GLfloat a1);
void glPointParameterfEXT(GLenum a0, GLfloat a1);
void glPointParameterfSGIS(GLenum a0, GLfloat a1);
void glPointParameterfvARB(GLenum a0, const GLfloat * a1);
void glPointParameterfvEXT(GLenum a0, const GLfloat * a1);
void glPointParameterfvSGIS(GLenum a0, const GLfloat * a1);
void glPointParameteriNV(GLenum a0, GLint a1);
void glPointParameterivNV(GLenum a0, const GLint * a1);
void glPolygonModeNV(GLenum a0, GLenum a1);
void glPolygonOffsetClampEXT(GLfloat a0, GLfloat a1, GLfloat a2);
void glPolygonOffsetEXT(GLfloat a0, GLfloat a1);
void glPopDebugGroupKHR(void);
void glPopGroupMarkerEXT(void);
void glPrimitiveBoundingBoxARB(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4, GLfloat a5, GLfloat a6, GLfloat a7);
void glPrimitiveBoundingBoxEXT(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4, GLfloat a5, GLfloat a6, GLfloat a7);
void glPrimitiveBoundingBoxOES(GLfloat a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4, GLfloat a5, GLfloat a6, GLfloat a7);
void glPrimitiveRestartIndexNV(GLuint a0);
void glProgramBinaryOES(GLuint a0, GLenum a1, const void * a2, GLint a3);
void glProgramParameteriARB(GLuint a0, GLenum a1, GLint a2);
void glProgramParameteriEXT(GLuint a0, GLenum a1, GLint a2);
void glProgramPathFragmentInputGenNV(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat* coeffs);
void glProgramUniform1dEXT(GLuint a0, GLint a1, GLdouble a2);
void glProgramUniform1dvEXT(GLuint a0, GLint a1, GLsizei a2, const GLdouble * a3);
void glProgramUniform1fEXT(GLuint a0, GLint a1, GLfloat a2);
void glProgramUniform1fvEXT(GLuint a0, GLint a1, GLsizei a2, const GLfloat * a3);
void glProgramUniform1iEXT(GLuint a0, GLint a1, GLint a2);
void glProgramUniform1ivEXT(GLuint a0, GLint a1, GLsizei a2, const GLint * a3);
void glProgramUniform1uiEXT(GLuint a0, GLint a1, GLuint a2);
void glProgramUniform1uivEXT(GLuint a0, GLint a1, GLsizei a2, const GLuint * a3);
void glProgramUniform2dEXT(GLuint a0, GLint a1, GLdouble a2, GLdouble a3);
void glProgramUniform2dvEXT(GLuint a0, GLint a1, GLsizei a2, const GLdouble * a3);
void glProgramUniform2fEXT(GLuint a0, GLint a1, GLfloat a2, GLfloat a3);
void glProgramUniform2fvEXT(GLuint a0, GLint a1, GLsizei a2, const GLfloat * a3);
void glProgramUniform2iEXT(GLuint a0, GLint a1, GLint a2, GLint a3);
void glProgramUniform2ivEXT(GLuint a0, GLint a1, GLsizei a2, const GLint * a3);
void glProgramUniform2uiEXT(GLuint a0, GLint a1, GLuint a2, GLuint a3);
void glProgramUniform2uivEXT(GLuint a0, GLint a1, GLsizei a2, const GLuint * a3);
void glProgramUniform3dEXT(GLuint a0, GLint a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glProgramUniform3dvEXT(GLuint a0, GLint a1, GLsizei a2, const GLdouble * a3);
void glProgramUniform3fEXT(GLuint a0, GLint a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glProgramUniform3fvEXT(GLuint a0, GLint a1, GLsizei a2, const GLfloat * a3);
void glProgramUniform3iEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4);
void glProgramUniform3ivEXT(GLuint a0, GLint a1, GLsizei a2, const GLint * a3);
void glProgramUniform3uiEXT(GLuint a0, GLint a1, GLuint a2, GLuint a3, GLuint a4);
void glProgramUniform3uivEXT(GLuint a0, GLint a1, GLsizei a2, const GLuint * a3);
void glProgramUniform4dEXT(GLuint a0, GLint a1, GLdouble a2, GLdouble a3, GLdouble a4, GLdouble a5);
void glProgramUniform4dvEXT(GLuint a0, GLint a1, GLsizei a2, const GLdouble * a3);
void glProgramUniform4fEXT(GLuint a0, GLint a1, GLfloat a2, GLfloat a3, GLfloat a4, GLfloat a5);
void glProgramUniform4fvEXT(GLuint a0, GLint a1, GLsizei a2, const GLfloat * a3);
void glProgramUniform4iEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4, GLint a5);
void glProgramUniform4ivEXT(GLuint a0, GLint a1, GLsizei a2, const GLint * a3);
void glProgramUniform4uiEXT(GLuint a0, GLint a1, GLuint a2, GLuint a3, GLuint a4, GLuint a5);
void glProgramUniform4uivEXT(GLuint a0, GLint a1, GLsizei a2, const GLuint * a3);
void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value);
void glProgramUniformHandleui64NV(GLuint program, GLint location, GLuint64 value);
void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64* values);
void glProgramUniformHandleui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64* values);
void glProgramUniformMatrix2dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix2fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix2x3dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix2x3fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix2x4dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix2x4fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix3dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix3fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix3x2dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix3x2fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix3x4dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix3x4fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix4dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix4fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix4x2dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix4x2fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformMatrix4x3dvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLdouble * a4);
void glProgramUniformMatrix4x3fvEXT(GLuint a0, GLint a1, GLsizei a2, GLboolean a3, const GLfloat * a4);
void glProgramUniformui64NV(GLuint program, GLint location, GLuint64 value);
void glProgramUniformui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64* value);
void glProvokingVertexEXT(GLenum a0);
void glPushClientAttribDefaultEXT(GLbitfield mask);
void glPushDebugGroupKHR(GLenum a0, GLuint a1, GLsizei a2, const GLchar * a3);
void glPushGroupMarkerEXT(GLsizei a0, const GLchar * a1);
void glQueryCounterEXT(GLuint a0, GLenum a1);
void glRasterSamplesEXT(GLuint samples, GLboolean fixedsamplelocations);
void glReadBufferNV(GLenum a0);
void glReadnPixelsARB(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glReadnPixelsEXT(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glReadnPixelsKHR(GLint a0, GLint a1, GLsizei a2, GLsizei a3, GLenum a4, GLenum a5, GLsizei a6, void * a7);
void glRenderbufferStorageEXT(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3);
void glRenderbufferStorageMultisampleANGLE(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glRenderbufferStorageMultisampleAPPLE(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glRenderbufferStorageMultisampleAdvancedAMD(GLenum target, GLsizei samples, GLsizei rbsamples,
        GLenum internalformat, GLsizei width, GLsizei height);
void glRenderbufferStorageMultisampleEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glRenderbufferStorageMultisampleNV(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glRenderbufferStorageOES(GLenum a0, GLenum a1, GLsizei a2, GLsizei a3);
void glResetHistogram(GLenum target);
void glResetMemoryObjectParameterNV(GLuint memory, GLenum pname);
void glResetMinmax(GLenum target);
void glResolveDepthValuesNV(void);
void glResumeTransformFeedbackNV(void);
void glSampleCoverageARB(GLfloat a0, GLboolean a1);
void glSamplePatternEXT(GLenum a0);
void glSamplerParameterIivEXT(GLuint a0, GLenum a1, const GLint * a2);
void glSamplerParameterIivOES(GLuint a0, GLenum a1, const GLint * a2);
void glSamplerParameterIuivEXT(GLuint a0, GLenum a1, const GLuint * a2);
void glSamplerParameterIuivOES(GLuint a0, GLenum a1, const GLuint * a2);
void glScissorArrayvNV(GLuint a0, GLsizei a1, const GLint * a2);
void glScissorArrayvOES(GLuint a0, GLsizei a1, const GLint * a2);
void glScissorExclusiveArrayvNV(GLuint first, GLsizei count, const GLint* v);
void glScissorExclusiveNV(GLint x, GLint y, GLsizei w, GLsizei h);
void glScissorIndexedNV(GLuint a0, GLint a1, GLint a2, GLsizei a3, GLsizei a4);
void glScissorIndexedOES(GLuint a0, GLint a1, GLint a2, GLsizei a3, GLsizei a4);
void glScissorIndexedvNV(GLuint a0, const GLint * a1);
void glScissorIndexedvOES(GLuint a0, const GLint * a1);
void glSecondaryColorFormatNV(GLint size, GLenum type, GLsizei stride);
void glSelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* counterList);
void glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* row, const void* column);
void glShaderSourceARB(GLuint a0, GLsizei a1, const GLchar ** a2, const GLint * a3);
void glShadingRateCombinerOpsEXT(GLenum op0, GLenum op1);
void glShadingRateEXT(GLenum rate);
void glShadingRateImageBarrierNV(GLboolean synchronize);
void glShadingRateImagePaletteNV(GLuint viewport, GLuint first, GLsizei count, const GLenum* rates);
void glShadingRateSampleOrderCustomNV(GLenum rate, GLuint samples, const GLint* locations);
void glShadingRateSampleOrderNV(GLenum order);
void glSpecializeShaderARB(GLuint a0, const GLchar * a1, GLuint a2, const GLuint * a3, const GLuint * a4);
void glStateCaptureNV(GLuint state, GLenum mode);
void glStencilFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum f, GLuint m, GLenum tr, const GLfloat* tv);
void glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask);
void glStencilFuncSeparateATI(GLenum a0, GLenum a1, GLint a2, GLuint a3);
void glStencilOpSeparateATI(GLenum a0, GLenum a1, GLenum a2, GLenum a3);
void glStencilStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLint r, GLuint m, GLenum tr, const GLfloat* tv);
void glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask);
void glStencilThenCoverFillPathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLenum f, GLuint m, GLenum c, GLenum tr, const GLfloat* tv);
void glStencilThenCoverFillPathNV(GLuint path, GLenum f, GLuint m, GLenum c);
void glStencilThenCoverStrokePathInstancedNV(GLsizei n, GLenum t, const void* p, GLuint b, GLint r, GLuint m, GLenum c, GLenum tr, const GLfloat* tv);
void glStencilThenCoverStrokePathNV(GLuint path, GLint r, GLuint m, GLenum c);
void glTexAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset);
void glTexBufferARB(GLenum a0, GLenum a1, GLuint a2);
void glTexBufferEXT(GLenum a0, GLenum a1, GLuint a2);
void glTexBufferOES(GLenum a0, GLenum a1, GLuint a2);
void glTexBufferRangeEXT(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glTexBufferRangeOES(GLenum a0, GLenum a1, GLuint a2, GLintptr a3, GLsizeiptr a4);
void glTexCoordFormatNV(GLint size, GLenum type, GLsizei stride);
void glTexPageCommitmentARB(GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
void glTexParameterIivEXT(GLenum a0, GLenum a1, const GLint * a2);
void glTexParameterIivOES(GLenum a0, GLenum a1, const GLint * a2);
void glTexParameterIuivEXT(GLenum a0, GLenum a1, const GLuint * a2);
void glTexParameterIuivOES(GLenum a0, GLenum a1, const GLuint * a2);
void glTexStorage1DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3);
void glTexStorage2DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4);
void glTexStorage3DEXT(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5);
void glTexStorage3DMultisampleOES(GLenum a0, GLsizei a1, GLenum a2, GLsizei a3, GLsizei a4, GLsizei a5, GLboolean a6);
void glTexSubImage1DEXT(GLenum a0, GLint a1, GLint a2, GLsizei a3, GLenum a4, GLenum a5, const void * a6);
void glTexSubImage2DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLsizei a4, GLsizei a5, GLenum a6, GLenum a7, const void * a8);
void glTexSubImage3DEXT(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glTexSubImage3DOES(GLenum a0, GLint a1, GLint a2, GLint a3, GLint a4, GLsizei a5, GLsizei a6, GLsizei a7, GLenum a8, GLenum a9, const void * a10);
void glTextureAttachMemoryNV(GLuint texture, GLuint memory, GLuint64 offset);
void glTextureBarrierNV(void);
void glTextureBufferEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer);
void glTextureBufferRangeEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer,
        GLintptr offset, GLsizeiptr size);
void glTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels);
void glTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
void glTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
void glTexturePageCommitmentEXT(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
void glTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* params);
void glTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, const GLuint* params);
void glTextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param);
void glTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, const GLfloat* params);
void glTextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param);
void glTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* params);
void glTextureRenderbufferEXT(GLuint texture, GLenum target, GLuint renderbuffer);
void glTextureStorage1DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
void glTextureStorage2DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
void glTextureStorage2DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
void glTextureStorage3DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
void glTextureStorage3DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
void glTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLenum type, const void* pixels);
void glTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels);
void glTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels);
void glTextureViewEXT(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7);
void glTextureViewOES(GLuint a0, GLenum a1, GLuint a2, GLenum a3, GLuint a4, GLuint a5, GLuint a6, GLuint a7);
void glTransformFeedbackVaryingsEXT(GLuint a0, GLsizei a1, const GLchar *const* a2, GLenum a3);
void glTransformPathNV(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat* transformValues);
void glUniform1fARB(GLint a0, GLfloat a1);
void glUniform1fvARB(GLint a0, GLsizei a1, const GLfloat * a2);
void glUniform1iARB(GLint a0, GLint a1);
void glUniform1ivARB(GLint a0, GLsizei a1, const GLint * a2);
void glUniform1uiEXT(GLint a0, GLuint a1);
void glUniform1uivEXT(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform2fARB(GLint a0, GLfloat a1, GLfloat a2);
void glUniform2fvARB(GLint a0, GLsizei a1, const GLfloat * a2);
void glUniform2iARB(GLint a0, GLint a1, GLint a2);
void glUniform2ivARB(GLint a0, GLsizei a1, const GLint * a2);
void glUniform2uiEXT(GLint a0, GLuint a1, GLuint a2);
void glUniform2uivEXT(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform3fARB(GLint a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glUniform3fvARB(GLint a0, GLsizei a1, const GLfloat * a2);
void glUniform3iARB(GLint a0, GLint a1, GLint a2, GLint a3);
void glUniform3ivARB(GLint a0, GLsizei a1, const GLint * a2);
void glUniform3uiEXT(GLint a0, GLuint a1, GLuint a2, GLuint a3);
void glUniform3uivEXT(GLint a0, GLsizei a1, const GLuint * a2);
void glUniform4fARB(GLint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glUniform4fvARB(GLint a0, GLsizei a1, const GLfloat * a2);
void glUniform4iARB(GLint a0, GLint a1, GLint a2, GLint a3, GLint a4);
void glUniform4ivARB(GLint a0, GLsizei a1, const GLint * a2);
void glUniform4uiEXT(GLint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4);
void glUniform4uivEXT(GLint a0, GLsizei a1, const GLuint * a2);
void glUniformHandleui64ARB(GLint location, GLuint64 value);
void glUniformHandleui64NV(GLint location, GLuint64 value);
void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64* value);
void glUniformHandleui64vNV(GLint location, GLsizei count, const GLuint64* value);
void glUniformMatrix2fvARB(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix2x3fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix2x4fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix3fvARB(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix3x2fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix3x4fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix4fvARB(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix4x2fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformMatrix4x3fvNV(GLint a0, GLsizei a1, GLboolean a2, const GLfloat * a3);
void glUniformui64NV(GLint location, GLuint64 value);
void glUniformui64vNV(GLint location, GLsizei count, const GLuint64* value);
GLboolean glUnmapBufferARB(GLenum a0);
GLboolean glUnmapBufferOES(GLenum a0);
GLboolean glUnmapNamedBufferEXT(GLuint a0);
void glUseProgramObjectARB(GLuint a0);
void glUseProgramStagesEXT(GLuint a0, GLbitfield a1, GLuint a2);
void glUseShaderProgramEXT(GLenum a0, GLuint a1);
void glValidateProgramARB(GLuint a0);
void glValidateProgramPipelineEXT(GLuint a0);
void glVertexArrayBindVertexBufferEXT(GLuint vaobj, GLuint bindingindex, GLuint buffer,
        GLintptr offset, GLsizei stride);
void glVertexArrayColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset);
void glVertexArrayEdgeFlagOffsetEXT(GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset);
void glVertexArrayFogCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset);
void glVertexArrayIndexOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset);
void glVertexArrayMultiTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum texunit, GLint size,
        GLenum type, GLsizei stride, GLintptr offset);
void glVertexArrayNormalOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset);
void glVertexArraySecondaryColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset);
void glVertexArrayTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset);
void glVertexArrayVertexAttribBindingEXT(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
void glVertexArrayVertexAttribDivisorEXT(GLuint vaobj, GLuint index, GLuint divisor);
void glVertexArrayVertexAttribFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLboolean normalized, GLuint relativeoffset);
void glVertexArrayVertexAttribIFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLuint relativeoffset);
void glVertexArrayVertexAttribIOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLsizei stride, GLintptr offset);
void glVertexArrayVertexAttribLFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLuint relativeoffset);
void glVertexArrayVertexAttribLOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLsizei stride, GLintptr offset);
void glVertexArrayVertexAttribOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset);
void glVertexArrayVertexBindingDivisorEXT(GLuint vaobj, GLuint bindingindex, GLuint divisor);
void glVertexArrayVertexOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset);
void glVertexAttrib1dARB(GLuint a0, GLdouble a1);
void glVertexAttrib1dNV(GLuint a0, GLdouble a1);
void glVertexAttrib1dvARB(GLuint a0, const GLdouble * a1);
void glVertexAttrib1dvNV(GLuint a0, const GLdouble * a1);
void glVertexAttrib1fARB(GLuint a0, GLfloat a1);
void glVertexAttrib1fNV(GLuint a0, GLfloat a1);
void glVertexAttrib1fvARB(GLuint a0, const GLfloat * a1);
void glVertexAttrib1fvNV(GLuint a0, const GLfloat * a1);
void glVertexAttrib1sARB(GLuint a0, GLshort a1);
void glVertexAttrib1sNV(GLuint a0, GLshort a1);
void glVertexAttrib1svARB(GLuint a0, const GLshort * a1);
void glVertexAttrib1svNV(GLuint a0, const GLshort * a1);
void glVertexAttrib2dARB(GLuint a0, GLdouble a1, GLdouble a2);
void glVertexAttrib2dNV(GLuint a0, GLdouble a1, GLdouble a2);
void glVertexAttrib2dvARB(GLuint a0, const GLdouble * a1);
void glVertexAttrib2dvNV(GLuint a0, const GLdouble * a1);
void glVertexAttrib2fARB(GLuint a0, GLfloat a1, GLfloat a2);
void glVertexAttrib2fNV(GLuint a0, GLfloat a1, GLfloat a2);
void glVertexAttrib2fvARB(GLuint a0, const GLfloat * a1);
void glVertexAttrib2fvNV(GLuint a0, const GLfloat * a1);
void glVertexAttrib2sARB(GLuint a0, GLshort a1, GLshort a2);
void glVertexAttrib2sNV(GLuint a0, GLshort a1, GLshort a2);
void glVertexAttrib2svARB(GLuint a0, const GLshort * a1);
void glVertexAttrib2svNV(GLuint a0, const GLshort * a1);
void glVertexAttrib3dARB(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3);
void glVertexAttrib3dNV(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3);
void glVertexAttrib3dvARB(GLuint a0, const GLdouble * a1);
void glVertexAttrib3dvNV(GLuint a0, const GLdouble * a1);
void glVertexAttrib3fARB(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glVertexAttrib3fNV(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3);
void glVertexAttrib3fvARB(GLuint a0, const GLfloat * a1);
void glVertexAttrib3fvNV(GLuint a0, const GLfloat * a1);
void glVertexAttrib3sARB(GLuint a0, GLshort a1, GLshort a2, GLshort a3);
void glVertexAttrib3sNV(GLuint a0, GLshort a1, GLshort a2, GLshort a3);
void glVertexAttrib3svARB(GLuint a0, const GLshort * a1);
void glVertexAttrib3svNV(GLuint a0, const GLshort * a1);
void glVertexAttrib4NbvARB(GLuint a0, const GLbyte * a1);
void glVertexAttrib4NivARB(GLuint a0, const GLint * a1);
void glVertexAttrib4NsvARB(GLuint a0, const GLshort * a1);
void glVertexAttrib4NubARB(GLuint a0, GLubyte a1, GLubyte a2, GLubyte a3, GLubyte a4);
void glVertexAttrib4NubvARB(GLuint a0, const GLubyte * a1);
void glVertexAttrib4NuivARB(GLuint a0, const GLuint * a1);
void glVertexAttrib4NusvARB(GLuint a0, const GLushort * a1);
void glVertexAttrib4bvARB(GLuint a0, const GLbyte * a1);
void glVertexAttrib4dARB(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glVertexAttrib4dNV(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glVertexAttrib4dvARB(GLuint a0, const GLdouble * a1);
void glVertexAttrib4dvNV(GLuint a0, const GLdouble * a1);
void glVertexAttrib4fARB(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glVertexAttrib4fNV(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glVertexAttrib4fvARB(GLuint a0, const GLfloat * a1);
void glVertexAttrib4fvNV(GLuint a0, const GLfloat * a1);
void glVertexAttrib4ivARB(GLuint a0, const GLint * a1);
void glVertexAttrib4sARB(GLuint a0, GLshort a1, GLshort a2, GLshort a3, GLshort a4);
void glVertexAttrib4sNV(GLuint a0, GLshort a1, GLshort a2, GLshort a3, GLshort a4);
void glVertexAttrib4svARB(GLuint a0, const GLshort * a1);
void glVertexAttrib4svNV(GLuint a0, const GLshort * a1);
void glVertexAttrib4ubvARB(GLuint a0, const GLubyte * a1);
void glVertexAttrib4ubvNV(GLuint a0, const GLubyte * a1);
void glVertexAttrib4uivARB(GLuint a0, const GLuint * a1);
void glVertexAttrib4usvARB(GLuint a0, const GLushort * a1);
void glVertexAttribDivisorANGLE(GLuint a0, GLuint a1);
void glVertexAttribDivisorARB(GLuint a0, GLuint a1);
void glVertexAttribDivisorEXT(GLuint a0, GLuint a1);
void glVertexAttribDivisorNV(GLuint a0, GLuint a1);
void glVertexAttribFormatNV(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);
void glVertexAttribI1iEXT(GLuint a0, GLint a1);
void glVertexAttribI1ivEXT(GLuint a0, const GLint * a1);
void glVertexAttribI1uiEXT(GLuint a0, GLuint a1);
void glVertexAttribI1uivEXT(GLuint a0, const GLuint * a1);
void glVertexAttribI2iEXT(GLuint a0, GLint a1, GLint a2);
void glVertexAttribI2ivEXT(GLuint a0, const GLint * a1);
void glVertexAttribI2uiEXT(GLuint a0, GLuint a1, GLuint a2);
void glVertexAttribI2uivEXT(GLuint a0, const GLuint * a1);
void glVertexAttribI3iEXT(GLuint a0, GLint a1, GLint a2, GLint a3);
void glVertexAttribI3ivEXT(GLuint a0, const GLint * a1);
void glVertexAttribI3uiEXT(GLuint a0, GLuint a1, GLuint a2, GLuint a3);
void glVertexAttribI3uivEXT(GLuint a0, const GLuint * a1);
void glVertexAttribI4bvEXT(GLuint a0, const GLbyte * a1);
void glVertexAttribI4iEXT(GLuint a0, GLint a1, GLint a2, GLint a3, GLint a4);
void glVertexAttribI4ivEXT(GLuint a0, const GLint * a1);
void glVertexAttribI4svEXT(GLuint a0, const GLshort * a1);
void glVertexAttribI4ubvEXT(GLuint a0, const GLubyte * a1);
void glVertexAttribI4uiEXT(GLuint a0, GLuint a1, GLuint a2, GLuint a3, GLuint a4);
void glVertexAttribI4uivEXT(GLuint a0, const GLuint * a1);
void glVertexAttribI4usvEXT(GLuint a0, const GLushort * a1);
void glVertexAttribIFormatNV(GLuint index, GLint size, GLenum type, GLsizei stride);
void glVertexAttribIPointerEXT(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4);
void glVertexAttribL1dEXT(GLuint a0, GLdouble a1);
void glVertexAttribL1dvEXT(GLuint a0, const GLdouble * a1);
void glVertexAttribL1i64NV(GLuint i, GLint64 x);
void glVertexAttribL1i64vNV(GLuint i, const GLint64* v);
void glVertexAttribL1ui64ARB(GLuint index, GLuint64 x);
void glVertexAttribL1ui64NV(GLuint i, GLuint64 x);
void glVertexAttribL1ui64vARB(GLuint index, const GLuint64* v);
void glVertexAttribL1ui64vNV(GLuint i, const GLuint64* v);
void glVertexAttribL2dEXT(GLuint a0, GLdouble a1, GLdouble a2);
void glVertexAttribL2dvEXT(GLuint a0, const GLdouble * a1);
void glVertexAttribL2i64NV(GLuint i, GLint64 x, GLint64 y);
void glVertexAttribL2i64vNV(GLuint i, const GLint64* v);
void glVertexAttribL2ui64NV(GLuint i, GLuint64 x, GLuint64 y);
void glVertexAttribL2ui64vNV(GLuint i, const GLuint64* v);
void glVertexAttribL3dEXT(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3);
void glVertexAttribL3dvEXT(GLuint a0, const GLdouble * a1);
void glVertexAttribL3i64NV(GLuint i, GLint64 x, GLint64 y, GLint64 z);
void glVertexAttribL3i64vNV(GLuint i, const GLint64* v);
void glVertexAttribL3ui64NV(GLuint i, GLuint64 x, GLuint64 y, GLuint64 z);
void glVertexAttribL3ui64vNV(GLuint i, const GLuint64* v);
void glVertexAttribL4dEXT(GLuint a0, GLdouble a1, GLdouble a2, GLdouble a3, GLdouble a4);
void glVertexAttribL4dvEXT(GLuint a0, const GLdouble * a1);
void glVertexAttribL4i64NV(GLuint i, GLint64 x, GLint64 y, GLint64 z, GLint64 w);
void glVertexAttribL4i64vNV(GLuint i, const GLint64* v);
void glVertexAttribL4ui64NV(GLuint i, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w);
void glVertexAttribL4ui64vNV(GLuint i, const GLuint64* v);
void glVertexAttribLFormatNV(GLuint i, GLint size, GLenum type, GLsizei stride);
void glVertexAttribLPointerEXT(GLuint a0, GLint a1, GLenum a2, GLsizei a3, const void * a4);
void glVertexAttribPointerARB(GLuint a0, GLint a1, GLenum a2, GLboolean a3, GLsizei a4, const void * a5);
void glVertexFormatNV(GLint size, GLenum type, GLsizei stride);
void glViewportArrayvNV(GLuint a0, GLsizei a1, const GLfloat * a2);
void glViewportArrayvOES(GLuint a0, GLsizei a1, const GLfloat * a2);
void glViewportIndexedfNV(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glViewportIndexedfOES(GLuint a0, GLfloat a1, GLfloat a2, GLfloat a3, GLfloat a4);
void glViewportIndexedfvNV(GLuint a0, const GLfloat * a1);
void glViewportIndexedfvOES(GLuint a0, const GLfloat * a1);
void glViewportPositionWScaleNV(GLuint index, GLfloat xcoeff, GLfloat ycoeff);
void glViewportSwizzleNV(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew);
void glWaitSyncAPPLE(GLsync a0, GLbitfield a1, GLuint64 a2);
void glWeightPathsNV(GLuint resultPath, GLsizei numPaths, const GLuint* paths, const GLfloat* weights);
void glWindowRectanglesEXT(GLenum mode, GLsizei count, const GLint* box);
}

#ifdef __cplusplus
extern "C" {
#endif

void glActiveShaderProgram(GLuint pipeline, GLuint program) {
    GlProfileScope _gp(0);
    if (!tglmt::GateCheck(1)) return;
    gl::glActiveShaderProgram(pipeline, program);
}
void glActiveTexture(GLenum texture) {
    GlProfileScope _gp(1);
    if (!tglmt::GateCheck(2)) return;
    gl::glActiveTexture(texture);
}
void glAttachShader(GLuint program, GLuint shader) {
    GlProfileScope _gp(2);
    if (!tglmt::GateCheck(6)) return;
    gl::glAttachShader(program, shader);
}
void glBeginConditionalRender(GLuint id, GLenum mode) {
    GlProfileScope _gp(3);
    if (!tglmt::GateCheck(8)) return;
    gl::glBeginConditionalRender(id, mode);
}
void glBeginQuery(GLenum target, GLuint id) {
    GlProfileScope _gp(4);
    if (!tglmt::GateCheck(9)) return;
    gl::glBeginQuery(target, id);
}
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {
    GlProfileScope _gp(5);
    if (!tglmt::GateCheck(10)) return;
    gl::glBeginQueryIndexed(target, index, id);
}
void glBeginTransformFeedback(GLenum primitiveMode) {
    GlProfileScope _gp(6);
    if (!tglmt::GateCheck(11)) return;
    gl::glBeginTransformFeedback(primitiveMode);
}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
    GlProfileScope _gp(7);
    if (!tglmt::GateCheck(12)) return;
    gl::glBindAttribLocation(program, index, name);
}
void glBindBuffer(GLenum target, GLuint buffer) {
    GlProfileScope _gp(8);
    if (!tglmt::GateCheck(13)) return;
    gl::glBindBuffer(target, buffer);
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    GlProfileScope _gp(9);
    if (!tglmt::GateCheck(14)) return;
    gl::glBindBufferBase(target, index, buffer);
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(10);
    if (!tglmt::GateCheck(15)) return;
    gl::glBindBufferRange(target, index, buffer, offset, size);
}
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers) {
    GlProfileScope _gp(11);
    if (!tglmt::GateCheck(16)) return;
    gl::glBindBuffersBase(target, first, count, buffers);
}
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes) {
    GlProfileScope _gp(12);
    if (!tglmt::GateCheck(17)) return;
    gl::glBindBuffersRange(target, first, count, buffers, offsets, sizes);
}
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name) {
    GlProfileScope _gp(13);
    if (!tglmt::GateCheck(18)) return;
    gl::glBindFragDataLocation(program, color, name);
}
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name) {
    GlProfileScope _gp(14);
    if (!tglmt::GateCheck(19)) return;
    gl::glBindFragDataLocationIndexed(program, colorNumber, index, name);
}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    GlProfileScope _gp(15);
    if (!tglmt::GateCheck(20)) return;
    gl::glBindFramebuffer(target, framebuffer);
}
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    GlProfileScope _gp(16);
    if (!tglmt::GateCheck(21)) return;
    gl::glBindImageTexture(unit, texture, level, layered, layer, access, format);
}
void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures) {
    GlProfileScope _gp(17);
    if (!tglmt::GateCheck(22)) return;
    gl::glBindImageTextures(first, count, textures);
}
void glBindProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(18);
    if (!tglmt::GateCheck(23)) return;
    gl::glBindProgramPipeline(pipeline);
}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    GlProfileScope _gp(19);
    if (!tglmt::GateCheck(24)) return;
    gl::glBindRenderbuffer(target, renderbuffer);
}
void glBindSampler(GLuint unit, GLuint sampler) {
    GlProfileScope _gp(20);
    if (!tglmt::GateCheck(25)) return;
    gl::glBindSampler(unit, sampler);
}
void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers) {
    GlProfileScope _gp(21);
    if (!tglmt::GateCheck(26)) return;
    gl::glBindSamplers(first, count, samplers);
}
void glBindTexture(GLenum target, GLuint texture) {
    GlProfileScope _gp(22);
    if (!tglmt::GateCheck(27)) return;
    gl::glBindTexture(target, texture);
}
void glBindTextureUnit(GLuint unit, GLuint texture) {
    GlProfileScope _gp(23);
    if (!tglmt::GateCheck(28)) return;
    gl::glBindTextureUnit(unit, texture);
}
void glBindTextures(GLuint first, GLsizei count, const GLuint *textures) {
    GlProfileScope _gp(24);
    if (!tglmt::GateCheck(29)) return;
    gl::glBindTextures(first, count, textures);
}
void glBindTransformFeedback(GLenum target, GLuint id) {
    GlProfileScope _gp(25);
    if (!tglmt::GateCheck(30)) return;
    gl::glBindTransformFeedback(target, id);
}
void glBindVertexArray(GLuint array) {
    GlProfileScope _gp(26);
    if (!tglmt::GateCheck(31)) return;
    gl::glBindVertexArray(array);
}
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    GlProfileScope _gp(27);
    if (!tglmt::GateCheck(32)) return;
    gl::glBindVertexBuffer(bindingindex, buffer, offset, stride);
}
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    GlProfileScope _gp(28);
    if (!tglmt::GateCheck(33)) return;
    gl::glBindVertexBuffers(first, count, buffers, offsets, strides);
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(29);
    if (!tglmt::GateCheck(35)) return;
    gl::glBlendColor(red, green, blue, alpha);
}
void glBlendEquation(GLenum mode) {
    GlProfileScope _gp(30);
    if (!tglmt::GateCheck(36)) return;
    gl::glBlendEquation(mode);
}
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    GlProfileScope _gp(31);
    if (!tglmt::GateCheck(37)) return;
    gl::glBlendEquationSeparate(modeRGB, modeAlpha);
}
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    GlProfileScope _gp(32);
    if (!tglmt::GateCheck(38)) return;
    gl::glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}
void glBlendEquationi(GLuint buf, GLenum mode) {
    GlProfileScope _gp(33);
    if (!tglmt::GateCheck(39)) return;
    gl::glBlendEquationi(buf, mode);
}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GlProfileScope _gp(34);
    if (!tglmt::GateCheck(40)) return;
    gl::glBlendFunc(sfactor, dfactor);
}
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    GlProfileScope _gp(35);
    if (!tglmt::GateCheck(41)) return;
    gl::glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    GlProfileScope _gp(36);
    if (!tglmt::GateCheck(42)) return;
    gl::glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {
    GlProfileScope _gp(37);
    if (!tglmt::GateCheck(43)) return;
    gl::glBlendFunci(buf, src, dst);
}
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    GlProfileScope _gp(38);
    if (!tglmt::GateCheck(44)) return;
    gl::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    GlProfileScope _gp(39);
    if (!tglmt::GateCheck(45)) return;
    gl::glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    GlProfileScope _gp(40);
    if (!tglmt::GateCheck(46)) return;
    gl::glBufferData(target, size, data, usage);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {
    GlProfileScope _gp(41);
    if (!tglmt::GateCheck(47)) return;
    gl::glBufferStorage(target, size, data, flags);
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    GlProfileScope _gp(42);
    if (!tglmt::GateCheck(48)) return;
    gl::glBufferSubData(target, offset, size, data);
}
GLenum glCheckFramebufferStatus(GLenum target) {
    GlProfileScope _gp(43);
    if (!tglmt::GateCheck(51)) return 0;
    return gl::glCheckFramebufferStatus(target);
}
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    GlProfileScope _gp(44);
    if (!tglmt::GateCheck(52)) return 0;
    return gl::glCheckNamedFramebufferStatus(framebuffer, target);
}
void glClampColor(GLenum target, GLenum clamp) {
    GlProfileScope _gp(45);
    if (!tglmt::GateCheck(53)) return;
    gl::glClampColor(target, clamp);
}
void glClear(GLbitfield mask) {
    GlProfileScope _gp(46);
    if (!tglmt::GateCheck(54)) return;
    gl::glClear(mask);
}
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(47);
    if (!tglmt::GateCheck(56)) return;
    gl::glClearBufferData(target, internalformat, format, type, data);
}
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(48);
    if (!tglmt::GateCheck(57)) return;
    gl::glClearBufferSubData(target, internalformat, offset, size, format, type, data);
}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GlProfileScope _gp(49);
    if (!tglmt::GateCheck(58)) return;
    gl::glClearBufferfi(buffer, drawbuffer, depth, stencil);
}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GlProfileScope _gp(50);
    if (!tglmt::GateCheck(59)) return;
    gl::glClearBufferfv(buffer, drawbuffer, value);
}
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) {
    GlProfileScope _gp(51);
    if (!tglmt::GateCheck(60)) return;
    gl::glClearBufferiv(buffer, drawbuffer, value);
}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GlProfileScope _gp(52);
    if (!tglmt::GateCheck(61)) return;
    gl::glClearBufferuiv(buffer, drawbuffer, value);
}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(53);
    if (!tglmt::GateCheck(62)) return;
    gl::glClearColor(red, green, blue, alpha);
}
void glClearDepth(GLdouble depth) {
    GlProfileScope _gp(54);
    if (!tglmt::GateCheck(63)) return;
    gl::glClearDepth(depth);
}
void glClearDepthf(GLfloat d) {
    GlProfileScope _gp(55);
    if (!tglmt::GateCheck(64)) return;
    gl::glClearDepthf(d);
}
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(56);
    if (!tglmt::GateCheck(66)) return;
    gl::glClearNamedBufferData(buffer, internalformat, format, type, data);
}
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(57);
    if (!tglmt::GateCheck(67)) return;
    gl::glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data);
}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GlProfileScope _gp(58);
    if (!tglmt::GateCheck(68)) return;
    gl::glClearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth, stencil);
}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GlProfileScope _gp(59);
    if (!tglmt::GateCheck(69)) return;
    gl::glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value) {
    GlProfileScope _gp(60);
    if (!tglmt::GateCheck(70)) return;
    gl::glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GlProfileScope _gp(61);
    if (!tglmt::GateCheck(71)) return;
    gl::glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value);
}
void glClearStencil(GLint s) {
    GlProfileScope _gp(62);
    if (!tglmt::GateCheck(72)) return;
    gl::glClearStencil(s);
}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(63);
    if (!tglmt::GateCheck(73)) return;
    gl::glClearTexImage(texture, level, format, type, data);
}
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(64);
    if (!tglmt::GateCheck(74)) return;
    gl::glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    GlProfileScope _gp(65);
    if (!tglmt::GateCheck(76)) return 0;
    return gl::glClientWaitSync(sync, flags, timeout);
}
void glClipControl(GLenum origin, GLenum depth) {
    GlProfileScope _gp(66);
    if (!tglmt::GateCheck(77)) return;
    gl::glClipControl(origin, depth);
}
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GlProfileScope _gp(67);
    if (!tglmt::GateCheck(111)) return;
    gl::glColorMask(red, green, blue, alpha);
}
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    GlProfileScope _gp(68);
    if (!tglmt::GateCheck(112)) return;
    gl::glColorMaski(index, r, g, b, a);
}
void glCompileShader(GLuint shader) {
    GlProfileScope _gp(69);
    if (!tglmt::GateCheck(119)) return;
    gl::glCompileShader(shader);
}
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(70);
    if (!tglmt::GateCheck(120)) return;
    gl::glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(71);
    if (!tglmt::GateCheck(121)) return;
    gl::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(72);
    if (!tglmt::GateCheck(122)) return;
    gl::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(73);
    if (!tglmt::GateCheck(123)) return;
    gl::glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
}
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(74);
    if (!tglmt::GateCheck(124)) return;
    gl::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(75);
    if (!tglmt::GateCheck(125)) return;
    gl::glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(76);
    if (!tglmt::GateCheck(126)) return;
    gl::glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data);
}
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(77);
    if (!tglmt::GateCheck(127)) return;
    gl::glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(78);
    if (!tglmt::GateCheck(128)) return;
    gl::glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    GlProfileScope _gp(79);
    if (!tglmt::GateCheck(129)) return;
    gl::glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    GlProfileScope _gp(80);
    if (!tglmt::GateCheck(130)) return;
    gl::glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
}
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    GlProfileScope _gp(81);
    if (!tglmt::GateCheck(131)) return;
    gl::glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    GlProfileScope _gp(82);
    if (!tglmt::GateCheck(133)) return;
    gl::glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GlProfileScope _gp(83);
    if (!tglmt::GateCheck(134)) return;
    gl::glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GlProfileScope _gp(84);
    if (!tglmt::GateCheck(135)) return;
    gl::glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(85);
    if (!tglmt::GateCheck(136)) return;
    gl::glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(86);
    if (!tglmt::GateCheck(137)) return;
    gl::glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GlProfileScope _gp(87);
    if (!tglmt::GateCheck(138)) return;
    gl::glCopyTextureSubImage1D(texture, level, xoffset, x, y, width);
}
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(88);
    if (!tglmt::GateCheck(139)) return;
    gl::glCopyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(89);
    if (!tglmt::GateCheck(140)) return;
    gl::glCopyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCreateBuffers(GLsizei n, GLuint *buffers) {
    GlProfileScope _gp(90);
    if (!tglmt::GateCheck(141)) return;
    gl::glCreateBuffers(n, buffers);
}
void glCreateFramebuffers(GLsizei n, GLuint *framebuffers) {
    GlProfileScope _gp(91);
    if (!tglmt::GateCheck(142)) return;
    gl::glCreateFramebuffers(n, framebuffers);
}
GLuint glCreateProgram(void) {
    GlProfileScope _gp(92);
    if (!tglmt::GateCheck(143)) return 0;
    return gl::glCreateProgram();
}
void glCreateProgramPipelines(GLsizei n, GLuint *pipelines) {
    GlProfileScope _gp(93);
    if (!tglmt::GateCheck(144)) return;
    gl::glCreateProgramPipelines(n, pipelines);
}
void glCreateQueries(GLenum target, GLsizei n, GLuint *ids) {
    GlProfileScope _gp(94);
    if (!tglmt::GateCheck(145)) return;
    gl::glCreateQueries(target, n, ids);
}
void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GlProfileScope _gp(95);
    if (!tglmt::GateCheck(146)) return;
    gl::glCreateRenderbuffers(n, renderbuffers);
}
void glCreateSamplers(GLsizei n, GLuint *samplers) {
    GlProfileScope _gp(96);
    if (!tglmt::GateCheck(147)) return;
    gl::glCreateSamplers(n, samplers);
}
GLuint glCreateShader(GLenum type) {
    GlProfileScope _gp(97);
    if (!tglmt::GateCheck(148)) return 0;
    return gl::glCreateShader(type);
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings) {
    GlProfileScope _gp(98);
    if (!tglmt::GateCheck(149)) return 0;
    return gl::glCreateShaderProgramv(type, count, strings);
}
void glCreateTextures(GLenum target, GLsizei n, GLuint *textures) {
    GlProfileScope _gp(99);
    if (!tglmt::GateCheck(150)) return;
    gl::glCreateTextures(target, n, textures);
}
void glCreateTransformFeedbacks(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(100);
    if (!tglmt::GateCheck(151)) return;
    gl::glCreateTransformFeedbacks(n, ids);
}
void glCreateVertexArrays(GLsizei n, GLuint *arrays) {
    GlProfileScope _gp(101);
    if (!tglmt::GateCheck(152)) return;
    gl::glCreateVertexArrays(n, arrays);
}
void glCullFace(GLenum mode) {
    GlProfileScope _gp(102);
    if (!tglmt::GateCheck(153)) return;
    gl::glCullFace(mode);
}
void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) {
    GlProfileScope _gp(103);
    if (!tglmt::GateCheck(154)) return;
    gl::glDebugMessageCallback(callback, userParam);
}
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) {
    GlProfileScope _gp(104);
    if (!tglmt::GateCheck(155)) return;
    gl::glDebugMessageControl(source, type, severity, count, ids, enabled);
}
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) {
    GlProfileScope _gp(105);
    if (!tglmt::GateCheck(156)) return;
    gl::glDebugMessageInsert(source, type, id, severity, length, buf);
}
void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    GlProfileScope _gp(106);
    if (!tglmt::GateCheck(157)) return;
    gl::glDeleteBuffers(n, buffers);
}
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    GlProfileScope _gp(107);
    if (!tglmt::GateCheck(158)) return;
    gl::glDeleteFramebuffers(n, framebuffers);
}
void glDeleteProgram(GLuint program) {
    GlProfileScope _gp(108);
    if (!tglmt::GateCheck(160)) return;
    gl::glDeleteProgram(program);
}
void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines) {
    GlProfileScope _gp(109);
    if (!tglmt::GateCheck(161)) return;
    gl::glDeleteProgramPipelines(n, pipelines);
}
void glDeleteQueries(GLsizei n, const GLuint *ids) {
    GlProfileScope _gp(110);
    if (!tglmt::GateCheck(162)) return;
    gl::glDeleteQueries(n, ids);
}
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    GlProfileScope _gp(111);
    if (!tglmt::GateCheck(163)) return;
    gl::glDeleteRenderbuffers(n, renderbuffers);
}
void glDeleteSamplers(GLsizei count, const GLuint *samplers) {
    GlProfileScope _gp(112);
    if (!tglmt::GateCheck(164)) return;
    gl::glDeleteSamplers(count, samplers);
}
void glDeleteShader(GLuint shader) {
    GlProfileScope _gp(113);
    if (!tglmt::GateCheck(165)) return;
    gl::glDeleteShader(shader);
}
void glDeleteSync(GLsync sync) {
    GlProfileScope _gp(114);
    if (!tglmt::GateCheck(166)) return;
    gl::glDeleteSync(sync);
}
void glDeleteTextures(GLsizei n, const GLuint *textures) {
    GlProfileScope _gp(115);
    if (!tglmt::GateCheck(167)) return;
    gl::glDeleteTextures(n, textures);
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids) {
    GlProfileScope _gp(116);
    if (!tglmt::GateCheck(168)) return;
    gl::glDeleteTransformFeedbacks(n, ids);
}
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    GlProfileScope _gp(117);
    if (!tglmt::GateCheck(169)) return;
    gl::glDeleteVertexArrays(n, arrays);
}
void glDepthFunc(GLenum func) {
    GlProfileScope _gp(118);
    if (!tglmt::GateCheck(170)) return;
    gl::glDepthFunc(func);
}
void glDepthMask(GLboolean flag) {
    GlProfileScope _gp(119);
    if (!tglmt::GateCheck(171)) return;
    gl::glDepthMask(flag);
}
void glDepthRange(GLdouble n, GLdouble f) {
    GlProfileScope _gp(120);
    if (!tglmt::GateCheck(172)) return;
    gl::glDepthRange(n, f);
}
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v) {
    GlProfileScope _gp(121);
    if (!tglmt::GateCheck(173)) return;
    gl::glDepthRangeArrayv(first, count, v);
}
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) {
    GlProfileScope _gp(122);
    if (!tglmt::GateCheck(174)) return;
    gl::glDepthRangeIndexed(index, n, f);
}
void glDepthRangef(GLfloat n, GLfloat f) {
    GlProfileScope _gp(123);
    if (!tglmt::GateCheck(175)) return;
    gl::glDepthRangef(n, f);
}
void glDetachShader(GLuint program, GLuint shader) {
    GlProfileScope _gp(124);
    if (!tglmt::GateCheck(176)) return;
    gl::glDetachShader(program, shader);
}
void glDisable(GLenum cap) {
    GlProfileScope _gp(125);
    if (!tglmt::GateCheck(177)) return;
    gl::glDisable(cap);
}
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    GlProfileScope _gp(126);
    if (!tglmt::GateCheck(179)) return;
    gl::glDisableVertexArrayAttrib(vaobj, index);
}
void glDisableVertexAttribArray(GLuint index) {
    GlProfileScope _gp(127);
    if (!tglmt::GateCheck(180)) return;
    gl::glDisableVertexAttribArray(index);
}
void glDisablei(GLenum target, GLuint index) {
    GlProfileScope _gp(128);
    if (!tglmt::GateCheck(181)) return;
    gl::glDisablei(target, index);
}
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    GlProfileScope _gp(129);
    if (!tglmt::GateCheck(182)) return;
    gl::glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}
void glDispatchComputeIndirect(GLintptr indirect) {
    GlProfileScope _gp(130);
    if (!tglmt::GateCheck(183)) return;
    gl::glDispatchComputeIndirect(indirect);
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    GlProfileScope _gp(131);
    if (!tglmt::GateCheck(184)) return;
    gl::glDrawArrays(mode, first, count);
}
void glDrawArraysIndirect(GLenum mode, const void *indirect) {
    GlProfileScope _gp(132);
    if (!tglmt::GateCheck(185)) return;
    gl::glDrawArraysIndirect(mode, indirect);
}
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    GlProfileScope _gp(133);
    if (!tglmt::GateCheck(186)) return;
    gl::glDrawArraysInstanced(mode, first, count, instancecount);
}
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
    GlProfileScope _gp(134);
    if (!tglmt::GateCheck(187)) return;
    gl::glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance);
}
void glDrawBuffer(GLenum buf) {
    GlProfileScope _gp(135);
    if (!tglmt::GateCheck(188)) return;
    gl::glDrawBuffer(buf);
}
void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    GlProfileScope _gp(136);
    if (!tglmt::GateCheck(189)) return;
    gl::glDrawBuffers(n, bufs);
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    GlProfileScope _gp(137);
    if (!tglmt::GateCheck(190)) return;
    gl::glDrawElements(mode, count, type, indices);
}
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GlProfileScope _gp(138);
    if (!tglmt::GateCheck(191)) return;
    gl::glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}
void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {
    GlProfileScope _gp(139);
    if (!tglmt::GateCheck(192)) return;
    gl::glDrawElementsIndirect(mode, type, indirect);
}
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    GlProfileScope _gp(140);
    if (!tglmt::GateCheck(193)) return;
    gl::glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance) {
    GlProfileScope _gp(141);
    if (!tglmt::GateCheck(194)) return;
    gl::glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
}
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
    GlProfileScope _gp(142);
    if (!tglmt::GateCheck(195)) return;
    gl::glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
    GlProfileScope _gp(143);
    if (!tglmt::GateCheck(196)) return;
    gl::glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    GlProfileScope _gp(144);
    if (!tglmt::GateCheck(198)) return;
    gl::glDrawRangeElements(mode, start, end, count, type, indices);
}
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GlProfileScope _gp(145);
    if (!tglmt::GateCheck(199)) return;
    gl::glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}
void glDrawTransformFeedback(GLenum mode, GLuint id) {
    GlProfileScope _gp(146);
    if (!tglmt::GateCheck(200)) return;
    gl::glDrawTransformFeedback(mode, id);
}
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) {
    GlProfileScope _gp(147);
    if (!tglmt::GateCheck(201)) return;
    gl::glDrawTransformFeedbackInstanced(mode, id, instancecount);
}
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) {
    GlProfileScope _gp(148);
    if (!tglmt::GateCheck(202)) return;
    gl::glDrawTransformFeedbackStream(mode, id, stream);
}
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) {
    GlProfileScope _gp(149);
    if (!tglmt::GateCheck(203)) return;
    gl::glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount);
}
void glEnable(GLenum cap) {
    GlProfileScope _gp(150);
    if (!tglmt::GateCheck(207)) return;
    gl::glEnable(cap);
}
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    GlProfileScope _gp(151);
    if (!tglmt::GateCheck(209)) return;
    gl::glEnableVertexArrayAttrib(vaobj, index);
}
void glEnableVertexAttribArray(GLuint index) {
    GlProfileScope _gp(152);
    if (!tglmt::GateCheck(210)) return;
    gl::glEnableVertexAttribArray(index);
}
void glEnablei(GLenum target, GLuint index) {
    GlProfileScope _gp(153);
    if (!tglmt::GateCheck(211)) return;
    gl::glEnablei(target, index);
}
void glEndConditionalRender(void) {
    GlProfileScope _gp(154);
    if (!tglmt::GateCheck(213)) return;
    gl::glEndConditionalRender();
}
void glEndQuery(GLenum target) {
    GlProfileScope _gp(155);
    if (!tglmt::GateCheck(215)) return;
    gl::glEndQuery(target);
}
void glEndQueryIndexed(GLenum target, GLuint index) {
    GlProfileScope _gp(156);
    if (!tglmt::GateCheck(216)) return;
    gl::glEndQueryIndexed(target, index);
}
void glEndTransformFeedback(void) {
    GlProfileScope _gp(157);
    if (!tglmt::GateCheck(217)) return;
    gl::glEndTransformFeedback();
}
GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    GlProfileScope _gp(158);
    if (!tglmt::GateCheck(231)) return nullptr;
    return gl::glFenceSync(condition, flags);
}
void glFinish(void) {
    GlProfileScope _gp(159);
    if (!tglmt::GateCheck(232)) return;
    gl::glFinish();
}
void glFlush(void) {
    GlProfileScope _gp(160);
    if (!tglmt::GateCheck(233)) return;
    gl::glFlush();
}
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(161);
    if (!tglmt::GateCheck(234)) return;
    gl::glFlushMappedBufferRange(target, offset, length);
}
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(162);
    if (!tglmt::GateCheck(235)) return;
    gl::glFlushMappedNamedBufferRange(buffer, offset, length);
}
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
    GlProfileScope _gp(163);
    if (!tglmt::GateCheck(245)) return;
    gl::glFramebufferParameteri(target, pname, param);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GlProfileScope _gp(164);
    if (!tglmt::GateCheck(246)) return;
    gl::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    GlProfileScope _gp(165);
    if (!tglmt::GateCheck(247)) return;
    gl::glFramebufferTexture(target, attachment, texture, level);
}
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GlProfileScope _gp(166);
    if (!tglmt::GateCheck(248)) return;
    gl::glFramebufferTexture1D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GlProfileScope _gp(167);
    if (!tglmt::GateCheck(249)) return;
    gl::glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    GlProfileScope _gp(168);
    if (!tglmt::GateCheck(250)) return;
    gl::glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GlProfileScope _gp(169);
    if (!tglmt::GateCheck(251)) return;
    gl::glFramebufferTextureLayer(target, attachment, texture, level, layer);
}
void glFrontFace(GLenum mode) {
    GlProfileScope _gp(170);
    if (!tglmt::GateCheck(252)) return;
    gl::glFrontFace(mode);
}
void glGenBuffers(GLsizei n, GLuint *buffers) {
    GlProfileScope _gp(171);
    if (!tglmt::GateCheck(254)) return;
    gl::glGenBuffers(n, buffers);
}
void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    GlProfileScope _gp(172);
    if (!tglmt::GateCheck(255)) return;
    gl::glGenFramebuffers(n, framebuffers);
}
void glGenProgramPipelines(GLsizei n, GLuint *pipelines) {
    GlProfileScope _gp(173);
    if (!tglmt::GateCheck(257)) return;
    gl::glGenProgramPipelines(n, pipelines);
}
void glGenQueries(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(174);
    if (!tglmt::GateCheck(258)) return;
    gl::glGenQueries(n, ids);
}
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GlProfileScope _gp(175);
    if (!tglmt::GateCheck(259)) return;
    gl::glGenRenderbuffers(n, renderbuffers);
}
void glGenSamplers(GLsizei count, GLuint *samplers) {
    GlProfileScope _gp(176);
    if (!tglmt::GateCheck(260)) return;
    gl::glGenSamplers(count, samplers);
}
void glGenTextures(GLsizei n, GLuint *textures) {
    GlProfileScope _gp(177);
    if (!tglmt::GateCheck(261)) return;
    gl::glGenTextures(n, textures);
}
void glGenTransformFeedbacks(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(178);
    if (!tglmt::GateCheck(262)) return;
    gl::glGenTransformFeedbacks(n, ids);
}
void glGenVertexArrays(GLsizei n, GLuint *arrays) {
    GlProfileScope _gp(179);
    if (!tglmt::GateCheck(263)) return;
    gl::glGenVertexArrays(n, arrays);
}
void glGenerateMipmap(GLenum target) {
    GlProfileScope _gp(180);
    if (!tglmt::GateCheck(264)) return;
    gl::glGenerateMipmap(target);
}
void glGenerateTextureMipmap(GLuint texture) {
    GlProfileScope _gp(181);
    if (!tglmt::GateCheck(265)) return;
    gl::glGenerateTextureMipmap(texture);
}
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params) {
    GlProfileScope _gp(182);
    if (!tglmt::GateCheck(266)) return;
    gl::glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params);
}
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(183);
    if (!tglmt::GateCheck(267)) return;
    gl::glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(184);
    if (!tglmt::GateCheck(268)) return;
    gl::glGetActiveSubroutineName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(185);
    if (!tglmt::GateCheck(269)) return;
    gl::glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values) {
    GlProfileScope _gp(186);
    if (!tglmt::GateCheck(270)) return;
    gl::glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values);
}
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(187);
    if (!tglmt::GateCheck(271)) return;
    gl::glGetActiveUniform(program, index, bufSize, length, size, type, name);
}
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) {
    GlProfileScope _gp(188);
    if (!tglmt::GateCheck(272)) return;
    gl::glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) {
    GlProfileScope _gp(189);
    if (!tglmt::GateCheck(273)) return;
    gl::glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName) {
    GlProfileScope _gp(190);
    if (!tglmt::GateCheck(274)) return;
    gl::glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) {
    GlProfileScope _gp(191);
    if (!tglmt::GateCheck(275)) return;
    gl::glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    GlProfileScope _gp(192);
    if (!tglmt::GateCheck(276)) return;
    gl::glGetAttachedShaders(program, maxCount, count, shaders);
}
GLint glGetAttribLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(193);
    if (!tglmt::GateCheck(277)) return 0;
    return gl::glGetAttribLocation(program, name);
}
void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data) {
    GlProfileScope _gp(194);
    if (!tglmt::GateCheck(278)) return;
    gl::glGetBooleani_v(target, index, data);
}
void glGetBooleanv(GLenum pname, GLboolean *data) {
    GlProfileScope _gp(195);
    if (!tglmt::GateCheck(279)) return;
    gl::glGetBooleanv(pname, data);
}
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(196);
    if (!tglmt::GateCheck(280)) return;
    gl::glGetBufferParameteri64v(target, pname, params);
}
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(197);
    if (!tglmt::GateCheck(281)) return;
    gl::glGetBufferParameteriv(target, pname, params);
}
void glGetBufferPointerv(GLenum target, GLenum pname, void **params) {
    GlProfileScope _gp(198);
    if (!tglmt::GateCheck(282)) return;
    gl::glGetBufferPointerv(target, pname, params);
}
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data) {
    GlProfileScope _gp(199);
    if (!tglmt::GateCheck(283)) return;
    gl::glGetBufferSubData(target, offset, size, data);
}
void glGetCompressedTexImage(GLenum target, GLint level, void *img) {
    GlProfileScope _gp(200);
    if (!tglmt::GateCheck(285)) return;
    gl::glGetCompressedTexImage(target, level, img);
}
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(201);
    if (!tglmt::GateCheck(286)) return;
    gl::glGetCompressedTextureImage(texture, level, bufSize, pixels);
}
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(202);
    if (!tglmt::GateCheck(287)) return;
    gl::glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels);
}
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) {
    GlProfileScope _gp(203);
    if (!tglmt::GateCheck(288)) return 0;
    return gl::glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog);
}
void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data) {
    GlProfileScope _gp(204);
    if (!tglmt::GateCheck(289)) return;
    gl::glGetDoublei_v(target, index, data);
}
void glGetDoublev(GLenum pname, GLdouble *data) {
    GlProfileScope _gp(205);
    if (!tglmt::GateCheck(290)) return;
    gl::glGetDoublev(pname, data);
}
GLenum glGetError(void) {
    GlProfileScope _gp(206);
    if (!tglmt::GateCheck(291)) return 0;
    return gl::glGetError();
}
void glGetFloati_v(GLenum target, GLuint index, GLfloat *data) {
    GlProfileScope _gp(207);
    if (!tglmt::GateCheck(292)) return;
    gl::glGetFloati_v(target, index, data);
}
void glGetFloatv(GLenum pname, GLfloat *data) {
    GlProfileScope _gp(208);
    if (!tglmt::GateCheck(293)) return;
    gl::glGetFloatv(pname, data);
}
GLint glGetFragDataIndex(GLuint program, const GLchar *name) {
    GlProfileScope _gp(209);
    if (!tglmt::GateCheck(294)) return 0;
    return gl::glGetFragDataIndex(program, name);
}
GLint glGetFragDataLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(210);
    if (!tglmt::GateCheck(295)) return 0;
    return gl::glGetFragDataLocation(program, name);
}
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    GlProfileScope _gp(211);
    if (!tglmt::GateCheck(296)) return;
    gl::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(212);
    if (!tglmt::GateCheck(297)) return;
    gl::glGetFramebufferParameteriv(target, pname, params);
}
GLenum glGetGraphicsResetStatus(void) {
    GlProfileScope _gp(213);
    if (!tglmt::GateCheck(298)) return 0;
    return gl::glGetGraphicsResetStatus();
}
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) {
    GlProfileScope _gp(214);
    if (!tglmt::GateCheck(299)) return;
    gl::glGetInteger64i_v(target, index, data);
}
void glGetInteger64v(GLenum pname, GLint64 *data) {
    GlProfileScope _gp(215);
    if (!tglmt::GateCheck(300)) return;
    gl::glGetInteger64v(pname, data);
}
void glGetIntegeri_v(GLenum target, GLuint index, GLint *data) {
    GlProfileScope _gp(216);
    if (!tglmt::GateCheck(301)) return;
    gl::glGetIntegeri_v(target, index, data);
}
void glGetIntegerv(GLenum pname, GLint *data) {
    GlProfileScope _gp(217);
    if (!tglmt::GateCheck(302)) return;
    gl::glGetIntegerv(pname, data);
}
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params) {
    GlProfileScope _gp(218);
    if (!tglmt::GateCheck(303)) return;
    gl::glGetInternalformati64v(target, internalformat, pname, count, params);
}
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params) {
    GlProfileScope _gp(219);
    if (!tglmt::GateCheck(304)) return;
    gl::glGetInternalformativ(target, internalformat, pname, count, params);
}
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val) {
    GlProfileScope _gp(220);
    if (!tglmt::GateCheck(312)) return;
    gl::glGetMultisamplefv(pname, index, val);
}
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(221);
    if (!tglmt::GateCheck(313)) return;
    gl::glGetNamedBufferParameteri64v(buffer, pname, params);
}
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params) {
    GlProfileScope _gp(222);
    if (!tglmt::GateCheck(314)) return;
    gl::glGetNamedBufferParameteriv(buffer, pname, params);
}
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params) {
    GlProfileScope _gp(223);
    if (!tglmt::GateCheck(315)) return;
    gl::glGetNamedBufferPointerv(buffer, pname, params);
}
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data) {
    GlProfileScope _gp(224);
    if (!tglmt::GateCheck(316)) return;
    gl::glGetNamedBufferSubData(buffer, offset, size, data);
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) {
    GlProfileScope _gp(225);
    if (!tglmt::GateCheck(317)) return;
    gl::glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, params);
}
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param) {
    GlProfileScope _gp(226);
    if (!tglmt::GateCheck(318)) return;
    gl::glGetNamedFramebufferParameteriv(framebuffer, pname, param);
}
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params) {
    GlProfileScope _gp(227);
    if (!tglmt::GateCheck(319)) return;
    gl::glGetNamedRenderbufferParameteriv(renderbuffer, pname, params);
}
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label) {
    GlProfileScope _gp(228);
    if (!tglmt::GateCheck(320)) return;
    gl::glGetObjectLabel(identifier, name, bufSize, length, label);
}
void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label) {
    GlProfileScope _gp(229);
    if (!tglmt::GateCheck(321)) return;
    gl::glGetObjectPtrLabel(ptr, bufSize, length, label);
}
void glGetPointerv(GLenum pname, void **params) {
    GlProfileScope _gp(230);
    if (!tglmt::GateCheck(325)) return;
    gl::glGetPointerv(pname, params);
}
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
    GlProfileScope _gp(231);
    if (!tglmt::GateCheck(327)) return;
    gl::glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(232);
    if (!tglmt::GateCheck(328)) return;
    gl::glGetProgramInfoLog(program, bufSize, length, infoLog);
}
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params) {
    GlProfileScope _gp(233);
    if (!tglmt::GateCheck(329)) return;
    gl::glGetProgramInterfaceiv(program, programInterface, pname, params);
}
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(234);
    if (!tglmt::GateCheck(330)) return;
    gl::glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
}
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params) {
    GlProfileScope _gp(235);
    if (!tglmt::GateCheck(331)) return;
    gl::glGetProgramPipelineiv(pipeline, pname, params);
}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(236);
    if (!tglmt::GateCheck(332)) return 0;
    return gl::glGetProgramResourceIndex(program, programInterface, name);
}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(237);
    if (!tglmt::GateCheck(333)) return 0;
    return gl::glGetProgramResourceLocation(program, programInterface, name);
}
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(238);
    if (!tglmt::GateCheck(334)) return 0;
    return gl::glGetProgramResourceLocationIndex(program, programInterface, name);
}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(239);
    if (!tglmt::GateCheck(335)) return;
    gl::glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params) {
    GlProfileScope _gp(240);
    if (!tglmt::GateCheck(336)) return;
    gl::glGetProgramResourceiv(program, programInterface, index, propCount, props, count, length, params);
}
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values) {
    GlProfileScope _gp(241);
    if (!tglmt::GateCheck(337)) return;
    gl::glGetProgramStageiv(program, shadertype, pname, values);
}
void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    GlProfileScope _gp(242);
    if (!tglmt::GateCheck(338)) return;
    gl::glGetProgramiv(program, pname, params);
}
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(243);
    if (!tglmt::GateCheck(339)) return;
    gl::glGetQueryBufferObjecti64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(244);
    if (!tglmt::GateCheck(340)) return;
    gl::glGetQueryBufferObjectiv(id, buffer, pname, offset);
}
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(245);
    if (!tglmt::GateCheck(341)) return;
    gl::glGetQueryBufferObjectui64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(246);
    if (!tglmt::GateCheck(342)) return;
    gl::glGetQueryBufferObjectuiv(id, buffer, pname, offset);
}
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(247);
    if (!tglmt::GateCheck(343)) return;
    gl::glGetQueryIndexediv(target, index, pname, params);
}
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(248);
    if (!tglmt::GateCheck(344)) return;
    gl::glGetQueryObjecti64v(id, pname, params);
}
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) {
    GlProfileScope _gp(249);
    if (!tglmt::GateCheck(345)) return;
    gl::glGetQueryObjectiv(id, pname, params);
}
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) {
    GlProfileScope _gp(250);
    if (!tglmt::GateCheck(346)) return;
    gl::glGetQueryObjectui64v(id, pname, params);
}
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {
    GlProfileScope _gp(251);
    if (!tglmt::GateCheck(347)) return;
    gl::glGetQueryObjectuiv(id, pname, params);
}
void glGetQueryiv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(252);
    if (!tglmt::GateCheck(348)) return;
    gl::glGetQueryiv(target, pname, params);
}
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(253);
    if (!tglmt::GateCheck(349)) return;
    gl::glGetRenderbufferParameteriv(target, pname, params);
}
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params) {
    GlProfileScope _gp(254);
    if (!tglmt::GateCheck(350)) return;
    gl::glGetSamplerParameterIiv(sampler, pname, params);
}
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params) {
    GlProfileScope _gp(255);
    if (!tglmt::GateCheck(351)) return;
    gl::glGetSamplerParameterIuiv(sampler, pname, params);
}
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(256);
    if (!tglmt::GateCheck(352)) return;
    gl::glGetSamplerParameterfv(sampler, pname, params);
}
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params) {
    GlProfileScope _gp(257);
    if (!tglmt::GateCheck(353)) return;
    gl::glGetSamplerParameteriv(sampler, pname, params);
}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(258);
    if (!tglmt::GateCheck(354)) return;
    gl::glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    GlProfileScope _gp(259);
    if (!tglmt::GateCheck(355)) return;
    gl::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    GlProfileScope _gp(260);
    if (!tglmt::GateCheck(356)) return;
    gl::glGetShaderSource(shader, bufSize, length, source);
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    GlProfileScope _gp(261);
    if (!tglmt::GateCheck(357)) return;
    gl::glGetShaderiv(shader, pname, params);
}
const GLubyte * glGetString(GLenum name) {
    GlProfileScope _gp(262);
    if (!tglmt::GateCheck(358)) return nullptr;
    return gl::glGetString(name);
}
const GLubyte * glGetStringi(GLenum name, GLuint index) {
    GlProfileScope _gp(263);
    if (!tglmt::GateCheck(359)) return nullptr;
    return gl::glGetStringi(name, index);
}
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name) {
    GlProfileScope _gp(264);
    if (!tglmt::GateCheck(360)) return 0;
    return gl::glGetSubroutineIndex(program, shadertype, name);
}
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name) {
    GlProfileScope _gp(265);
    if (!tglmt::GateCheck(361)) return 0;
    return gl::glGetSubroutineUniformLocation(program, shadertype, name);
}
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values) {
    GlProfileScope _gp(266);
    if (!tglmt::GateCheck(362)) return;
    gl::glGetSynciv(sync, pname, count, length, values);
}
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels) {
    GlProfileScope _gp(267);
    if (!tglmt::GateCheck(368)) return;
    gl::glGetTexImage(target, level, format, type, pixels);
}
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(268);
    if (!tglmt::GateCheck(369)) return;
    gl::glGetTexLevelParameterfv(target, level, pname, params);
}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    GlProfileScope _gp(269);
    if (!tglmt::GateCheck(370)) return;
    gl::glGetTexLevelParameteriv(target, level, pname, params);
}
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(270);
    if (!tglmt::GateCheck(371)) return;
    gl::glGetTexParameterIiv(target, pname, params);
}
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params) {
    GlProfileScope _gp(271);
    if (!tglmt::GateCheck(372)) return;
    gl::glGetTexParameterIuiv(target, pname, params);
}
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(272);
    if (!tglmt::GateCheck(373)) return;
    gl::glGetTexParameterfv(target, pname, params);
}
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(273);
    if (!tglmt::GateCheck(374)) return;
    gl::glGetTexParameteriv(target, pname, params);
}
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(274);
    if (!tglmt::GateCheck(375)) return;
    gl::glGetTextureImage(texture, level, format, type, bufSize, pixels);
}
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(275);
    if (!tglmt::GateCheck(376)) return;
    gl::glGetTextureLevelParameterfv(texture, level, pname, params);
}
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params) {
    GlProfileScope _gp(276);
    if (!tglmt::GateCheck(377)) return;
    gl::glGetTextureLevelParameteriv(texture, level, pname, params);
}
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params) {
    GlProfileScope _gp(277);
    if (!tglmt::GateCheck(378)) return;
    gl::glGetTextureParameterIiv(texture, pname, params);
}
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params) {
    GlProfileScope _gp(278);
    if (!tglmt::GateCheck(379)) return;
    gl::glGetTextureParameterIuiv(texture, pname, params);
}
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(279);
    if (!tglmt::GateCheck(380)) return;
    gl::glGetTextureParameterfv(texture, pname, params);
}
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params) {
    GlProfileScope _gp(280);
    if (!tglmt::GateCheck(381)) return;
    gl::glGetTextureParameteriv(texture, pname, params);
}
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(281);
    if (!tglmt::GateCheck(382)) return;
    gl::glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
}
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(282);
    if (!tglmt::GateCheck(383)) return;
    gl::glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param) {
    GlProfileScope _gp(283);
    if (!tglmt::GateCheck(384)) return;
    gl::glGetTransformFeedbacki64_v(xfb, pname, index, param);
}
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param) {
    GlProfileScope _gp(284);
    if (!tglmt::GateCheck(385)) return;
    gl::glGetTransformFeedbacki_v(xfb, pname, index, param);
}
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param) {
    GlProfileScope _gp(285);
    if (!tglmt::GateCheck(386)) return;
    gl::glGetTransformFeedbackiv(xfb, pname, param);
}
GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) {
    GlProfileScope _gp(286);
    if (!tglmt::GateCheck(387)) return 0;
    return gl::glGetUniformBlockIndex(program, uniformBlockName);
}
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) {
    GlProfileScope _gp(287);
    if (!tglmt::GateCheck(388)) return;
    gl::glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}
GLint glGetUniformLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(288);
    if (!tglmt::GateCheck(389)) return 0;
    return gl::glGetUniformLocation(program, name);
}
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params) {
    GlProfileScope _gp(289);
    if (!tglmt::GateCheck(390)) return;
    gl::glGetUniformSubroutineuiv(shadertype, location, params);
}
void glGetUniformdv(GLuint program, GLint location, GLdouble *params) {
    GlProfileScope _gp(290);
    if (!tglmt::GateCheck(391)) return;
    gl::glGetUniformdv(program, location, params);
}
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
    GlProfileScope _gp(291);
    if (!tglmt::GateCheck(392)) return;
    gl::glGetUniformfv(program, location, params);
}
void glGetUniformiv(GLuint program, GLint location, GLint *params) {
    GlProfileScope _gp(292);
    if (!tglmt::GateCheck(393)) return;
    gl::glGetUniformiv(program, location, params);
}
void glGetUniformuiv(GLuint program, GLint location, GLuint *params) {
    GlProfileScope _gp(293);
    if (!tglmt::GateCheck(394)) return;
    gl::glGetUniformuiv(program, location, params);
}
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param) {
    GlProfileScope _gp(294);
    if (!tglmt::GateCheck(395)) return;
    gl::glGetVertexArrayIndexed64iv(vaobj, index, pname, param);
}
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param) {
    GlProfileScope _gp(295);
    if (!tglmt::GateCheck(396)) return;
    gl::glGetVertexArrayIndexediv(vaobj, index, pname, param);
}
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param) {
    GlProfileScope _gp(296);
    if (!tglmt::GateCheck(397)) return;
    gl::glGetVertexArrayiv(vaobj, pname, param);
}
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(297);
    if (!tglmt::GateCheck(398)) return;
    gl::glGetVertexAttribIiv(index, pname, params);
}
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params) {
    GlProfileScope _gp(298);
    if (!tglmt::GateCheck(399)) return;
    gl::glGetVertexAttribIuiv(index, pname, params);
}
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params) {
    GlProfileScope _gp(299);
    if (!tglmt::GateCheck(400)) return;
    gl::glGetVertexAttribLdv(index, pname, params);
}
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
    GlProfileScope _gp(300);
    if (!tglmt::GateCheck(401)) return;
    gl::glGetVertexAttribPointerv(index, pname, pointer);
}
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params) {
    GlProfileScope _gp(301);
    if (!tglmt::GateCheck(402)) return;
    gl::glGetVertexAttribdv(index, pname, params);
}
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(302);
    if (!tglmt::GateCheck(403)) return;
    gl::glGetVertexAttribfv(index, pname, params);
}
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(303);
    if (!tglmt::GateCheck(404)) return;
    gl::glGetVertexAttribiv(index, pname, params);
}
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(304);
    if (!tglmt::GateCheck(406)) return;
    gl::glGetnCompressedTexImage(target, lod, bufSize, pixels);
}
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(305);
    if (!tglmt::GateCheck(418)) return;
    gl::glGetnTexImage(target, level, format, type, bufSize, pixels);
}
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params) {
    GlProfileScope _gp(306);
    if (!tglmt::GateCheck(419)) return;
    gl::glGetnUniformdv(program, location, bufSize, params);
}
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) {
    GlProfileScope _gp(307);
    if (!tglmt::GateCheck(420)) return;
    gl::glGetnUniformfv(program, location, bufSize, params);
}
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
    GlProfileScope _gp(308);
    if (!tglmt::GateCheck(421)) return;
    gl::glGetnUniformiv(program, location, bufSize, params);
}
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params) {
    GlProfileScope _gp(309);
    if (!tglmt::GateCheck(422)) return;
    gl::glGetnUniformuiv(program, location, bufSize, params);
}
void glHint(GLenum target, GLenum mode) {
    GlProfileScope _gp(310);
    if (!tglmt::GateCheck(423)) return;
    gl::glHint(target, mode);
}
void glInvalidateBufferData(GLuint buffer) {
    GlProfileScope _gp(311);
    if (!tglmt::GateCheck(438)) return;
    gl::glInvalidateBufferData(buffer);
}
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(312);
    if (!tglmt::GateCheck(439)) return;
    gl::glInvalidateBufferSubData(buffer, offset, length);
}
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    GlProfileScope _gp(313);
    if (!tglmt::GateCheck(440)) return;
    gl::glInvalidateFramebuffer(target, numAttachments, attachments);
}
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments) {
    GlProfileScope _gp(314);
    if (!tglmt::GateCheck(441)) return;
    gl::glInvalidateNamedFramebufferData(framebuffer, numAttachments, attachments);
}
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(315);
    if (!tglmt::GateCheck(442)) return;
    gl::glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, attachments, x, y, width, height);
}
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(316);
    if (!tglmt::GateCheck(443)) return;
    gl::glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}
void glInvalidateTexImage(GLuint texture, GLint level) {
    GlProfileScope _gp(317);
    if (!tglmt::GateCheck(444)) return;
    gl::glInvalidateTexImage(texture, level);
}
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(318);
    if (!tglmt::GateCheck(445)) return;
    gl::glInvalidateTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth);
}
GLboolean glIsBuffer(GLuint buffer) {
    GlProfileScope _gp(319);
    if (!tglmt::GateCheck(446)) return 0;
    return gl::glIsBuffer(buffer);
}
GLboolean glIsEnabled(GLenum cap) {
    GlProfileScope _gp(320);
    if (!tglmt::GateCheck(447)) return 0;
    return gl::glIsEnabled(cap);
}
GLboolean glIsEnabledi(GLenum target, GLuint index) {
    GlProfileScope _gp(321);
    if (!tglmt::GateCheck(448)) return 0;
    return gl::glIsEnabledi(target, index);
}
GLboolean glIsFramebuffer(GLuint framebuffer) {
    GlProfileScope _gp(322);
    if (!tglmt::GateCheck(449)) return 0;
    return gl::glIsFramebuffer(framebuffer);
}
GLboolean glIsProgram(GLuint program) {
    GlProfileScope _gp(323);
    if (!tglmt::GateCheck(451)) return 0;
    return gl::glIsProgram(program);
}
GLboolean glIsProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(324);
    if (!tglmt::GateCheck(452)) return 0;
    return gl::glIsProgramPipeline(pipeline);
}
GLboolean glIsQuery(GLuint id) {
    GlProfileScope _gp(325);
    if (!tglmt::GateCheck(453)) return 0;
    return gl::glIsQuery(id);
}
GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    GlProfileScope _gp(326);
    if (!tglmt::GateCheck(454)) return 0;
    return gl::glIsRenderbuffer(renderbuffer);
}
GLboolean glIsSampler(GLuint sampler) {
    GlProfileScope _gp(327);
    if (!tglmt::GateCheck(455)) return 0;
    return gl::glIsSampler(sampler);
}
GLboolean glIsShader(GLuint shader) {
    GlProfileScope _gp(328);
    if (!tglmt::GateCheck(456)) return 0;
    return gl::glIsShader(shader);
}
GLboolean glIsSync(GLsync sync) {
    GlProfileScope _gp(329);
    if (!tglmt::GateCheck(457)) return 0;
    return gl::glIsSync(sync);
}
GLboolean glIsTexture(GLuint texture) {
    GlProfileScope _gp(330);
    if (!tglmt::GateCheck(458)) return 0;
    return gl::glIsTexture(texture);
}
GLboolean glIsTransformFeedback(GLuint id) {
    GlProfileScope _gp(331);
    if (!tglmt::GateCheck(459)) return 0;
    return gl::glIsTransformFeedback(id);
}
GLboolean glIsVertexArray(GLuint array) {
    GlProfileScope _gp(332);
    if (!tglmt::GateCheck(460)) return 0;
    return gl::glIsVertexArray(array);
}
void glLineWidth(GLfloat width) {
    GlProfileScope _gp(333);
    if (!tglmt::GateCheck(470)) return;
    gl::glLineWidth(width);
}
void glLinkProgram(GLuint program) {
    GlProfileScope _gp(334);
    if (!tglmt::GateCheck(471)) return;
    gl::glLinkProgram(program);
}
void glLogicOp(GLenum opcode) {
    GlProfileScope _gp(335);
    if (!tglmt::GateCheck(479)) return;
    gl::glLogicOp(opcode);
}
void * glMapBuffer(GLenum target, GLenum access) {
    GlProfileScope _gp(336);
    if (!tglmt::GateCheck(484)) return nullptr;
    return gl::glMapBuffer(target, access);
}
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GlProfileScope _gp(337);
    if (!tglmt::GateCheck(485)) return nullptr;
    return gl::glMapBufferRange(target, offset, length, access);
}
void * glMapNamedBuffer(GLuint buffer, GLenum access) {
    GlProfileScope _gp(338);
    if (!tglmt::GateCheck(490)) return nullptr;
    return gl::glMapNamedBuffer(buffer, access);
}
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GlProfileScope _gp(339);
    if (!tglmt::GateCheck(491)) return nullptr;
    return gl::glMapNamedBufferRange(buffer, offset, length, access);
}
void glMemoryBarrier(GLbitfield barriers) {
    GlProfileScope _gp(340);
    if (!tglmt::GateCheck(497)) return;
    gl::glMemoryBarrier(barriers);
}
void glMemoryBarrierByRegion(GLbitfield barriers) {
    GlProfileScope _gp(341);
    if (!tglmt::GateCheck(498)) return;
    gl::glMemoryBarrierByRegion(barriers);
}
void glMinSampleShading(GLfloat value) {
    GlProfileScope _gp(342);
    if (!tglmt::GateCheck(499)) return;
    gl::glMinSampleShading(value);
}
void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {
    GlProfileScope _gp(343);
    if (!tglmt::GateCheck(504)) return;
    gl::glMultiDrawArrays(mode, first, count, drawcount);
}
void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) {
    GlProfileScope _gp(344);
    if (!tglmt::GateCheck(505)) return;
    gl::glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}
void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    GlProfileScope _gp(345);
    if (!tglmt::GateCheck(506)) return;
    gl::glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
}
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount) {
    GlProfileScope _gp(346);
    if (!tglmt::GateCheck(507)) return;
    gl::glMultiDrawElements(mode, count, type, indices, drawcount);
}
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex) {
    GlProfileScope _gp(347);
    if (!tglmt::GateCheck(508)) return;
    gl::glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex);
}
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {
    GlProfileScope _gp(348);
    if (!tglmt::GateCheck(509)) return;
    gl::glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    GlProfileScope _gp(349);
    if (!tglmt::GateCheck(510)) return;
    gl::glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
}
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) {
    GlProfileScope _gp(350);
    if (!tglmt::GateCheck(551)) return;
    gl::glNamedBufferData(buffer, size, data, usage);
}
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags) {
    GlProfileScope _gp(351);
    if (!tglmt::GateCheck(552)) return;
    gl::glNamedBufferStorage(buffer, size, data, flags);
}
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) {
    GlProfileScope _gp(352);
    if (!tglmt::GateCheck(553)) return;
    gl::glNamedBufferSubData(buffer, offset, size, data);
}
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {
    GlProfileScope _gp(353);
    if (!tglmt::GateCheck(554)) return;
    gl::glNamedFramebufferDrawBuffer(framebuffer, buf);
}
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs) {
    GlProfileScope _gp(354);
    if (!tglmt::GateCheck(555)) return;
    gl::glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {
    GlProfileScope _gp(355);
    if (!tglmt::GateCheck(556)) return;
    gl::glNamedFramebufferParameteri(framebuffer, pname, param);
}
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {
    GlProfileScope _gp(356);
    if (!tglmt::GateCheck(557)) return;
    gl::glNamedFramebufferReadBuffer(framebuffer, src);
}
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GlProfileScope _gp(357);
    if (!tglmt::GateCheck(558)) return;
    gl::glNamedFramebufferRenderbuffer(framebuffer, attachment, renderbuffertarget, renderbuffer);
}
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    GlProfileScope _gp(358);
    if (!tglmt::GateCheck(559)) return;
    gl::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GlProfileScope _gp(359);
    if (!tglmt::GateCheck(560)) return;
    gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
}
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(360);
    if (!tglmt::GateCheck(561)) return;
    gl::glNamedRenderbufferStorage(renderbuffer, internalformat, width, height);
}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(361);
    if (!tglmt::GateCheck(562)) return;
    gl::glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height);
}
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) {
    GlProfileScope _gp(362);
    if (!tglmt::GateCheck(577)) return;
    gl::glObjectLabel(identifier, name, length, label);
}
void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label) {
    GlProfileScope _gp(363);
    if (!tglmt::GateCheck(578)) return;
    gl::glObjectPtrLabel(ptr, length, label);
}
void glPatchParameterfv(GLenum pname, const GLfloat *values) {
    GlProfileScope _gp(364);
    if (!tglmt::GateCheck(581)) return;
    gl::glPatchParameterfv(pname, values);
}
void glPatchParameteri(GLenum pname, GLint value) {
    GlProfileScope _gp(365);
    if (!tglmt::GateCheck(582)) return;
    gl::glPatchParameteri(pname, value);
}
void glPauseTransformFeedback(void) {
    GlProfileScope _gp(366);
    if (!tglmt::GateCheck(583)) return;
    gl::glPauseTransformFeedback();
}
void glPixelStoref(GLenum pname, GLfloat param) {
    GlProfileScope _gp(367);
    if (!tglmt::GateCheck(587)) return;
    gl::glPixelStoref(pname, param);
}
void glPixelStorei(GLenum pname, GLint param) {
    GlProfileScope _gp(368);
    if (!tglmt::GateCheck(588)) return;
    gl::glPixelStorei(pname, param);
}
void glPointParameterf(GLenum pname, GLfloat param) {
    GlProfileScope _gp(369);
    if (!tglmt::GateCheck(592)) return;
    gl::glPointParameterf(pname, param);
}
void glPointParameterfv(GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(370);
    if (!tglmt::GateCheck(593)) return;
    gl::glPointParameterfv(pname, params);
}
void glPointParameteri(GLenum pname, GLint param) {
    GlProfileScope _gp(371);
    if (!tglmt::GateCheck(594)) return;
    gl::glPointParameteri(pname, param);
}
void glPointParameteriv(GLenum pname, const GLint *params) {
    GlProfileScope _gp(372);
    if (!tglmt::GateCheck(595)) return;
    gl::glPointParameteriv(pname, params);
}
void glPointSize(GLfloat size) {
    GlProfileScope _gp(373);
    if (!tglmt::GateCheck(596)) return;
    gl::glPointSize(size);
}
void glPolygonMode(GLenum face, GLenum mode) {
    GlProfileScope _gp(374);
    if (!tglmt::GateCheck(597)) return;
    gl::glPolygonMode(face, mode);
}
void glPolygonOffset(GLfloat factor, GLfloat units) {
    GlProfileScope _gp(375);
    if (!tglmt::GateCheck(598)) return;
    gl::glPolygonOffset(factor, units);
}
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
    GlProfileScope _gp(376);
    if (!tglmt::GateCheck(599)) return;
    gl::glPolygonOffsetClamp(factor, units, clamp);
}
void glPopDebugGroup(void) {
    GlProfileScope _gp(377);
    if (!tglmt::GateCheck(603)) return;
    gl::glPopDebugGroup();
}
void glPrimitiveRestartIndex(GLuint index) {
    GlProfileScope _gp(378);
    if (!tglmt::GateCheck(606)) return;
    gl::glPrimitiveRestartIndex(index);
}
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) {
    GlProfileScope _gp(379);
    if (!tglmt::GateCheck(608)) return;
    gl::glProgramBinary(program, binaryFormat, binary, length);
}
void glProgramParameteri(GLuint program, GLenum pname, GLint value) {
    GlProfileScope _gp(380);
    if (!tglmt::GateCheck(609)) return;
    gl::glProgramParameteri(program, pname, value);
}
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0) {
    GlProfileScope _gp(381);
    if (!tglmt::GateCheck(610)) return;
    gl::glProgramUniform1d(program, location, v0);
}
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(382);
    if (!tglmt::GateCheck(611)) return;
    gl::glProgramUniform1dv(program, location, count, value);
}
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    GlProfileScope _gp(383);
    if (!tglmt::GateCheck(612)) return;
    gl::glProgramUniform1f(program, location, v0);
}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(384);
    if (!tglmt::GateCheck(613)) return;
    gl::glProgramUniform1fv(program, location, count, value);
}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    GlProfileScope _gp(385);
    if (!tglmt::GateCheck(614)) return;
    gl::glProgramUniform1i(program, location, v0);
}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(386);
    if (!tglmt::GateCheck(615)) return;
    gl::glProgramUniform1iv(program, location, count, value);
}
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
    GlProfileScope _gp(387);
    if (!tglmt::GateCheck(616)) return;
    gl::glProgramUniform1ui(program, location, v0);
}
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(388);
    if (!tglmt::GateCheck(617)) return;
    gl::glProgramUniform1uiv(program, location, count, value);
}
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) {
    GlProfileScope _gp(389);
    if (!tglmt::GateCheck(618)) return;
    gl::glProgramUniform2d(program, location, v0, v1);
}
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(390);
    if (!tglmt::GateCheck(619)) return;
    gl::glProgramUniform2dv(program, location, count, value);
}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    GlProfileScope _gp(391);
    if (!tglmt::GateCheck(620)) return;
    gl::glProgramUniform2f(program, location, v0, v1);
}
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(392);
    if (!tglmt::GateCheck(621)) return;
    gl::glProgramUniform2fv(program, location, count, value);
}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    GlProfileScope _gp(393);
    if (!tglmt::GateCheck(622)) return;
    gl::glProgramUniform2i(program, location, v0, v1);
}
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(394);
    if (!tglmt::GateCheck(623)) return;
    gl::glProgramUniform2iv(program, location, count, value);
}
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {
    GlProfileScope _gp(395);
    if (!tglmt::GateCheck(624)) return;
    gl::glProgramUniform2ui(program, location, v0, v1);
}
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(396);
    if (!tglmt::GateCheck(625)) return;
    gl::glProgramUniform2uiv(program, location, count, value);
}
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) {
    GlProfileScope _gp(397);
    if (!tglmt::GateCheck(626)) return;
    gl::glProgramUniform3d(program, location, v0, v1, v2);
}
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(398);
    if (!tglmt::GateCheck(627)) return;
    gl::glProgramUniform3dv(program, location, count, value);
}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GlProfileScope _gp(399);
    if (!tglmt::GateCheck(628)) return;
    gl::glProgramUniform3f(program, location, v0, v1, v2);
}
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(400);
    if (!tglmt::GateCheck(629)) return;
    gl::glProgramUniform3fv(program, location, count, value);
}
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    GlProfileScope _gp(401);
    if (!tglmt::GateCheck(630)) return;
    gl::glProgramUniform3i(program, location, v0, v1, v2);
}
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(402);
    if (!tglmt::GateCheck(631)) return;
    gl::glProgramUniform3iv(program, location, count, value);
}
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GlProfileScope _gp(403);
    if (!tglmt::GateCheck(632)) return;
    gl::glProgramUniform3ui(program, location, v0, v1, v2);
}
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(404);
    if (!tglmt::GateCheck(633)) return;
    gl::glProgramUniform3uiv(program, location, count, value);
}
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) {
    GlProfileScope _gp(405);
    if (!tglmt::GateCheck(634)) return;
    gl::glProgramUniform4d(program, location, v0, v1, v2, v3);
}
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(406);
    if (!tglmt::GateCheck(635)) return;
    gl::glProgramUniform4dv(program, location, count, value);
}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GlProfileScope _gp(407);
    if (!tglmt::GateCheck(636)) return;
    gl::glProgramUniform4f(program, location, v0, v1, v2, v3);
}
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(408);
    if (!tglmt::GateCheck(637)) return;
    gl::glProgramUniform4fv(program, location, count, value);
}
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GlProfileScope _gp(409);
    if (!tglmt::GateCheck(638)) return;
    gl::glProgramUniform4i(program, location, v0, v1, v2, v3);
}
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(410);
    if (!tglmt::GateCheck(639)) return;
    gl::glProgramUniform4iv(program, location, count, value);
}
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GlProfileScope _gp(411);
    if (!tglmt::GateCheck(640)) return;
    gl::glProgramUniform4ui(program, location, v0, v1, v2, v3);
}
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(412);
    if (!tglmt::GateCheck(641)) return;
    gl::glProgramUniform4uiv(program, location, count, value);
}
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(413);
    if (!tglmt::GateCheck(642)) return;
    gl::glProgramUniformMatrix2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(414);
    if (!tglmt::GateCheck(643)) return;
    gl::glProgramUniformMatrix2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(415);
    if (!tglmt::GateCheck(644)) return;
    gl::glProgramUniformMatrix2x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(416);
    if (!tglmt::GateCheck(645)) return;
    gl::glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(417);
    if (!tglmt::GateCheck(646)) return;
    gl::glProgramUniformMatrix2x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(418);
    if (!tglmt::GateCheck(647)) return;
    gl::glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(419);
    if (!tglmt::GateCheck(648)) return;
    gl::glProgramUniformMatrix3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(420);
    if (!tglmt::GateCheck(649)) return;
    gl::glProgramUniformMatrix3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(421);
    if (!tglmt::GateCheck(650)) return;
    gl::glProgramUniformMatrix3x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(422);
    if (!tglmt::GateCheck(651)) return;
    gl::glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(423);
    if (!tglmt::GateCheck(652)) return;
    gl::glProgramUniformMatrix3x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(424);
    if (!tglmt::GateCheck(653)) return;
    gl::glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(425);
    if (!tglmt::GateCheck(654)) return;
    gl::glProgramUniformMatrix4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(426);
    if (!tglmt::GateCheck(655)) return;
    gl::glProgramUniformMatrix4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(427);
    if (!tglmt::GateCheck(656)) return;
    gl::glProgramUniformMatrix4x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(428);
    if (!tglmt::GateCheck(657)) return;
    gl::glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(429);
    if (!tglmt::GateCheck(658)) return;
    gl::glProgramUniformMatrix4x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(430);
    if (!tglmt::GateCheck(659)) return;
    gl::glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
}
void glProvokingVertex(GLenum mode) {
    GlProfileScope _gp(431);
    if (!tglmt::GateCheck(660)) return;
    gl::glProvokingVertex(mode);
}
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message) {
    GlProfileScope _gp(432);
    if (!tglmt::GateCheck(663)) return;
    gl::glPushDebugGroup(source, id, length, message);
}
void glQueryCounter(GLuint id, GLenum target) {
    GlProfileScope _gp(433);
    if (!tglmt::GateCheck(666)) return;
    gl::glQueryCounter(id, target);
}
void glReadBuffer(GLenum src) {
    GlProfileScope _gp(434);
    if (!tglmt::GateCheck(691)) return;
    gl::glReadBuffer(src);
}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    GlProfileScope _gp(435);
    if (!tglmt::GateCheck(692)) return;
    gl::glReadPixels(x, y, width, height, format, type, pixels);
}
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) {
    GlProfileScope _gp(436);
    if (!tglmt::GateCheck(693)) return;
    gl::glReadnPixels(x, y, width, height, format, type, bufSize, data);
}
void glReleaseShaderCompiler(void) {
    GlProfileScope _gp(437);
    if (!tglmt::GateCheck(702)) return;
    gl::glReleaseShaderCompiler();
}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(438);
    if (!tglmt::GateCheck(704)) return;
    gl::glRenderbufferStorage(target, internalformat, width, height);
}
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(439);
    if (!tglmt::GateCheck(705)) return;
    gl::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}
void glResumeTransformFeedback(void) {
    GlProfileScope _gp(440);
    if (!tglmt::GateCheck(706)) return;
    gl::glResumeTransformFeedback();
}
void glSampleCoverage(GLfloat value, GLboolean invert) {
    GlProfileScope _gp(441);
    if (!tglmt::GateCheck(709)) return;
    gl::glSampleCoverage(value, invert);
}
void glSampleMaski(GLuint maskNumber, GLbitfield mask) {
    GlProfileScope _gp(442);
    if (!tglmt::GateCheck(710)) return;
    gl::glSampleMaski(maskNumber, mask);
}
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) {
    GlProfileScope _gp(443);
    if (!tglmt::GateCheck(711)) return;
    gl::glSamplerParameterIiv(sampler, pname, param);
}
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) {
    GlProfileScope _gp(444);
    if (!tglmt::GateCheck(712)) return;
    gl::glSamplerParameterIuiv(sampler, pname, param);
}
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    GlProfileScope _gp(445);
    if (!tglmt::GateCheck(713)) return;
    gl::glSamplerParameterf(sampler, pname, param);
}
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) {
    GlProfileScope _gp(446);
    if (!tglmt::GateCheck(714)) return;
    gl::glSamplerParameterfv(sampler, pname, param);
}
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    GlProfileScope _gp(447);
    if (!tglmt::GateCheck(715)) return;
    gl::glSamplerParameteri(sampler, pname, param);
}
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) {
    GlProfileScope _gp(448);
    if (!tglmt::GateCheck(716)) return;
    gl::glSamplerParameteriv(sampler, pname, param);
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(449);
    if (!tglmt::GateCheck(719)) return;
    gl::glScissor(x, y, width, height);
}
void glScissorArrayv(GLuint first, GLsizei count, const GLint *v) {
    GlProfileScope _gp(450);
    if (!tglmt::GateCheck(720)) return;
    gl::glScissorArrayv(first, count, v);
}
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
    GlProfileScope _gp(451);
    if (!tglmt::GateCheck(721)) return;
    gl::glScissorIndexed(index, left, bottom, width, height);
}
void glScissorIndexedv(GLuint index, const GLint *v) {
    GlProfileScope _gp(452);
    if (!tglmt::GateCheck(722)) return;
    gl::glScissorIndexedv(index, v);
}
void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length) {
    GlProfileScope _gp(453);
    if (!tglmt::GateCheck(744)) return;
    gl::glShaderBinary(count, shaders, binaryFormat, binary, length);
}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    GlProfileScope _gp(454);
    if (!tglmt::GateCheck(745)) return;
    gl::glShaderSource(shader, count, string, length);
}
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    GlProfileScope _gp(455);
    if (!tglmt::GateCheck(746)) return;
    gl::glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding);
}
void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue) {
    GlProfileScope _gp(456);
    if (!tglmt::GateCheck(747)) return;
    gl::glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    GlProfileScope _gp(457);
    if (!tglmt::GateCheck(748)) return;
    gl::glStencilFunc(func, ref, mask);
}
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    GlProfileScope _gp(458);
    if (!tglmt::GateCheck(749)) return;
    gl::glStencilFuncSeparate(face, func, ref, mask);
}
void glStencilMask(GLuint mask) {
    GlProfileScope _gp(459);
    if (!tglmt::GateCheck(750)) return;
    gl::glStencilMask(mask);
}
void glStencilMaskSeparate(GLenum face, GLuint mask) {
    GlProfileScope _gp(460);
    if (!tglmt::GateCheck(751)) return;
    gl::glStencilMaskSeparate(face, mask);
}
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    GlProfileScope _gp(461);
    if (!tglmt::GateCheck(752)) return;
    gl::glStencilOp(fail, zfail, zpass);
}
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    GlProfileScope _gp(462);
    if (!tglmt::GateCheck(753)) return;
    gl::glStencilOpSeparate(face, sfail, dpfail, dppass);
}
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
    GlProfileScope _gp(463);
    if (!tglmt::GateCheck(754)) return;
    gl::glTexBuffer(target, internalformat, buffer);
}
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(464);
    if (!tglmt::GateCheck(755)) return;
    gl::glTexBufferRange(target, internalformat, buffer, offset, size);
}
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(465);
    if (!tglmt::GateCheck(807)) return;
    gl::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(466);
    if (!tglmt::GateCheck(808)) return;
    gl::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(467);
    if (!tglmt::GateCheck(809)) return;
    gl::glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(468);
    if (!tglmt::GateCheck(810)) return;
    gl::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(469);
    if (!tglmt::GateCheck(811)) return;
    gl::glTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) {
    GlProfileScope _gp(470);
    if (!tglmt::GateCheck(812)) return;
    gl::glTexParameterIiv(target, pname, params);
}
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) {
    GlProfileScope _gp(471);
    if (!tglmt::GateCheck(813)) return;
    gl::glTexParameterIuiv(target, pname, params);
}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    GlProfileScope _gp(472);
    if (!tglmt::GateCheck(814)) return;
    gl::glTexParameterf(target, pname, param);
}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(473);
    if (!tglmt::GateCheck(815)) return;
    gl::glTexParameterfv(target, pname, params);
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GlProfileScope _gp(474);
    if (!tglmt::GateCheck(816)) return;
    gl::glTexParameteri(target, pname, param);
}
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    GlProfileScope _gp(475);
    if (!tglmt::GateCheck(817)) return;
    gl::glTexParameteriv(target, pname, params);
}
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
    GlProfileScope _gp(476);
    if (!tglmt::GateCheck(818)) return;
    gl::glTexStorage1D(target, levels, internalformat, width);
}
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(477);
    if (!tglmt::GateCheck(819)) return;
    gl::glTexStorage2D(target, levels, internalformat, width, height);
}
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(478);
    if (!tglmt::GateCheck(820)) return;
    gl::glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(479);
    if (!tglmt::GateCheck(821)) return;
    gl::glTexStorage3D(target, levels, internalformat, width, height, depth);
}
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(480);
    if (!tglmt::GateCheck(822)) return;
    gl::glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(481);
    if (!tglmt::GateCheck(823)) return;
    gl::glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(482);
    if (!tglmt::GateCheck(824)) return;
    gl::glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(483);
    if (!tglmt::GateCheck(825)) return;
    gl::glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureBarrier(void) {
    GlProfileScope _gp(484);
    if (!tglmt::GateCheck(826)) return;
    gl::glTextureBarrier();
}
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {
    GlProfileScope _gp(485);
    if (!tglmt::GateCheck(827)) return;
    gl::glTextureBuffer(texture, internalformat, buffer);
}
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(486);
    if (!tglmt::GateCheck(828)) return;
    gl::glTextureBufferRange(texture, internalformat, buffer, offset, size);
}
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params) {
    GlProfileScope _gp(487);
    if (!tglmt::GateCheck(829)) return;
    gl::glTextureParameterIiv(texture, pname, params);
}
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params) {
    GlProfileScope _gp(488);
    if (!tglmt::GateCheck(830)) return;
    gl::glTextureParameterIuiv(texture, pname, params);
}
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
    GlProfileScope _gp(489);
    if (!tglmt::GateCheck(831)) return;
    gl::glTextureParameterf(texture, pname, param);
}
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param) {
    GlProfileScope _gp(490);
    if (!tglmt::GateCheck(832)) return;
    gl::glTextureParameterfv(texture, pname, param);
}
void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
    GlProfileScope _gp(491);
    if (!tglmt::GateCheck(833)) return;
    gl::glTextureParameteri(texture, pname, param);
}
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param) {
    GlProfileScope _gp(492);
    if (!tglmt::GateCheck(834)) return;
    gl::glTextureParameteriv(texture, pname, param);
}
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    GlProfileScope _gp(493);
    if (!tglmt::GateCheck(835)) return;
    gl::glTextureStorage1D(texture, levels, internalformat, width);
}
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(494);
    if (!tglmt::GateCheck(836)) return;
    gl::glTextureStorage2D(texture, levels, internalformat, width, height);
}
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(495);
    if (!tglmt::GateCheck(837)) return;
    gl::glTextureStorage2DMultisample(texture, samples, internalformat, width, height, fixedsamplelocations);
}
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(496);
    if (!tglmt::GateCheck(838)) return;
    gl::glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(497);
    if (!tglmt::GateCheck(839)) return;
    gl::glTextureStorage3DMultisample(texture, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(498);
    if (!tglmt::GateCheck(840)) return;
    gl::glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(499);
    if (!tglmt::GateCheck(841)) return;
    gl::glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(500);
    if (!tglmt::GateCheck(842)) return;
    gl::glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
    GlProfileScope _gp(501);
    if (!tglmt::GateCheck(843)) return;
    gl::glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
}
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {
    GlProfileScope _gp(502);
    if (!tglmt::GateCheck(844)) return;
    gl::glTransformFeedbackBufferBase(xfb, index, buffer);
}
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(503);
    if (!tglmt::GateCheck(845)) return;
    gl::glTransformFeedbackBufferRange(xfb, index, buffer, offset, size);
}
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) {
    GlProfileScope _gp(504);
    if (!tglmt::GateCheck(846)) return;
    gl::glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}
void glUniform1d(GLint location, GLdouble x) {
    GlProfileScope _gp(505);
    if (!tglmt::GateCheck(849)) return;
    gl::glUniform1d(location, x);
}
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(506);
    if (!tglmt::GateCheck(850)) return;
    gl::glUniform1dv(location, count, value);
}
void glUniform1f(GLint location, GLfloat v0) {
    GlProfileScope _gp(507);
    if (!tglmt::GateCheck(851)) return;
    gl::glUniform1f(location, v0);
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(508);
    if (!tglmt::GateCheck(852)) return;
    gl::glUniform1fv(location, count, value);
}
void glUniform1i(GLint location, GLint v0) {
    GlProfileScope _gp(509);
    if (!tglmt::GateCheck(853)) return;
    gl::glUniform1i(location, v0);
}
void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(510);
    if (!tglmt::GateCheck(854)) return;
    gl::glUniform1iv(location, count, value);
}
void glUniform1ui(GLint location, GLuint v0) {
    GlProfileScope _gp(511);
    if (!tglmt::GateCheck(855)) return;
    gl::glUniform1ui(location, v0);
}
void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(512);
    if (!tglmt::GateCheck(856)) return;
    gl::glUniform1uiv(location, count, value);
}
void glUniform2d(GLint location, GLdouble x, GLdouble y) {
    GlProfileScope _gp(513);
    if (!tglmt::GateCheck(857)) return;
    gl::glUniform2d(location, x, y);
}
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(514);
    if (!tglmt::GateCheck(858)) return;
    gl::glUniform2dv(location, count, value);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GlProfileScope _gp(515);
    if (!tglmt::GateCheck(859)) return;
    gl::glUniform2f(location, v0, v1);
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(516);
    if (!tglmt::GateCheck(860)) return;
    gl::glUniform2fv(location, count, value);
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
    GlProfileScope _gp(517);
    if (!tglmt::GateCheck(861)) return;
    gl::glUniform2i(location, v0, v1);
}
void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(518);
    if (!tglmt::GateCheck(862)) return;
    gl::glUniform2iv(location, count, value);
}
void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    GlProfileScope _gp(519);
    if (!tglmt::GateCheck(863)) return;
    gl::glUniform2ui(location, v0, v1);
}
void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(520);
    if (!tglmt::GateCheck(864)) return;
    gl::glUniform2uiv(location, count, value);
}
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(521);
    if (!tglmt::GateCheck(865)) return;
    gl::glUniform3d(location, x, y, z);
}
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(522);
    if (!tglmt::GateCheck(866)) return;
    gl::glUniform3dv(location, count, value);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GlProfileScope _gp(523);
    if (!tglmt::GateCheck(867)) return;
    gl::glUniform3f(location, v0, v1, v2);
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(524);
    if (!tglmt::GateCheck(868)) return;
    gl::glUniform3fv(location, count, value);
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GlProfileScope _gp(525);
    if (!tglmt::GateCheck(869)) return;
    gl::glUniform3i(location, v0, v1, v2);
}
void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(526);
    if (!tglmt::GateCheck(870)) return;
    gl::glUniform3iv(location, count, value);
}
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GlProfileScope _gp(527);
    if (!tglmt::GateCheck(871)) return;
    gl::glUniform3ui(location, v0, v1, v2);
}
void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(528);
    if (!tglmt::GateCheck(872)) return;
    gl::glUniform3uiv(location, count, value);
}
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(529);
    if (!tglmt::GateCheck(873)) return;
    gl::glUniform4d(location, x, y, z, w);
}
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(530);
    if (!tglmt::GateCheck(874)) return;
    gl::glUniform4dv(location, count, value);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GlProfileScope _gp(531);
    if (!tglmt::GateCheck(875)) return;
    gl::glUniform4f(location, v0, v1, v2, v3);
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(532);
    if (!tglmt::GateCheck(876)) return;
    gl::glUniform4fv(location, count, value);
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GlProfileScope _gp(533);
    if (!tglmt::GateCheck(877)) return;
    gl::glUniform4i(location, v0, v1, v2, v3);
}
void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(534);
    if (!tglmt::GateCheck(878)) return;
    gl::glUniform4iv(location, count, value);
}
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GlProfileScope _gp(535);
    if (!tglmt::GateCheck(879)) return;
    gl::glUniform4ui(location, v0, v1, v2, v3);
}
void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(536);
    if (!tglmt::GateCheck(880)) return;
    gl::glUniform4uiv(location, count, value);
}
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    GlProfileScope _gp(537);
    if (!tglmt::GateCheck(881)) return;
    gl::glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(538);
    if (!tglmt::GateCheck(882)) return;
    gl::glUniformMatrix2dv(location, count, transpose, value);
}
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(539);
    if (!tglmt::GateCheck(883)) return;
    gl::glUniformMatrix2fv(location, count, transpose, value);
}
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(540);
    if (!tglmt::GateCheck(884)) return;
    gl::glUniformMatrix2x3dv(location, count, transpose, value);
}
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(541);
    if (!tglmt::GateCheck(885)) return;
    gl::glUniformMatrix2x3fv(location, count, transpose, value);
}
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(542);
    if (!tglmt::GateCheck(886)) return;
    gl::glUniformMatrix2x4dv(location, count, transpose, value);
}
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(543);
    if (!tglmt::GateCheck(887)) return;
    gl::glUniformMatrix2x4fv(location, count, transpose, value);
}
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(544);
    if (!tglmt::GateCheck(888)) return;
    gl::glUniformMatrix3dv(location, count, transpose, value);
}
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(545);
    if (!tglmt::GateCheck(889)) return;
    gl::glUniformMatrix3fv(location, count, transpose, value);
}
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(546);
    if (!tglmt::GateCheck(890)) return;
    gl::glUniformMatrix3x2dv(location, count, transpose, value);
}
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(547);
    if (!tglmt::GateCheck(891)) return;
    gl::glUniformMatrix3x2fv(location, count, transpose, value);
}
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(548);
    if (!tglmt::GateCheck(892)) return;
    gl::glUniformMatrix3x4dv(location, count, transpose, value);
}
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(549);
    if (!tglmt::GateCheck(893)) return;
    gl::glUniformMatrix3x4fv(location, count, transpose, value);
}
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(550);
    if (!tglmt::GateCheck(894)) return;
    gl::glUniformMatrix4dv(location, count, transpose, value);
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(551);
    if (!tglmt::GateCheck(895)) return;
    gl::glUniformMatrix4fv(location, count, transpose, value);
}
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(552);
    if (!tglmt::GateCheck(896)) return;
    gl::glUniformMatrix4x2dv(location, count, transpose, value);
}
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(553);
    if (!tglmt::GateCheck(897)) return;
    gl::glUniformMatrix4x2fv(location, count, transpose, value);
}
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(554);
    if (!tglmt::GateCheck(898)) return;
    gl::glUniformMatrix4x3dv(location, count, transpose, value);
}
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(555);
    if (!tglmt::GateCheck(899)) return;
    gl::glUniformMatrix4x3fv(location, count, transpose, value);
}
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices) {
    GlProfileScope _gp(556);
    if (!tglmt::GateCheck(900)) return;
    gl::glUniformSubroutinesuiv(shadertype, count, indices);
}
GLboolean glUnmapBuffer(GLenum target) {
    GlProfileScope _gp(557);
    if (!tglmt::GateCheck(901)) return 0;
    return gl::glUnmapBuffer(target);
}
GLboolean glUnmapNamedBuffer(GLuint buffer) {
    GlProfileScope _gp(558);
    if (!tglmt::GateCheck(902)) return 0;
    return gl::glUnmapNamedBuffer(buffer);
}
void glUseProgram(GLuint program) {
    GlProfileScope _gp(559);
    if (!tglmt::GateCheck(903)) return;
    gl::glUseProgram(program);
}
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
    GlProfileScope _gp(560);
    if (!tglmt::GateCheck(904)) return;
    gl::glUseProgramStages(pipeline, stages, program);
}
void glValidateProgram(GLuint program) {
    GlProfileScope _gp(561);
    if (!tglmt::GateCheck(905)) return;
    gl::glValidateProgram(program);
}
void glValidateProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(562);
    if (!tglmt::GateCheck(906)) return;
    gl::glValidateProgramPipeline(pipeline);
}
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
    GlProfileScope _gp(563);
    if (!tglmt::GateCheck(931)) return;
    gl::glVertexArrayAttribBinding(vaobj, attribindex, bindingindex);
}
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GlProfileScope _gp(564);
    if (!tglmt::GateCheck(932)) return;
    gl::glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset);
}
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(565);
    if (!tglmt::GateCheck(933)) return;
    gl::glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(566);
    if (!tglmt::GateCheck(934)) return;
    gl::glVertexArrayAttribLFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {
    GlProfileScope _gp(567);
    if (!tglmt::GateCheck(935)) return;
    gl::glVertexArrayBindingDivisor(vaobj, bindingindex, divisor);
}
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
    GlProfileScope _gp(568);
    if (!tglmt::GateCheck(936)) return;
    gl::glVertexArrayElementBuffer(vaobj, buffer);
}
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    GlProfileScope _gp(569);
    if (!tglmt::GateCheck(937)) return;
    gl::glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride);
}
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    GlProfileScope _gp(570);
    if (!tglmt::GateCheck(938)) return;
    gl::glVertexArrayVertexBuffers(vaobj, first, count, buffers, offsets, strides);
}
void glVertexAttrib1d(GLuint index, GLdouble x) {
    GlProfileScope _gp(571);
    if (!tglmt::GateCheck(939)) return;
    gl::glVertexAttrib1d(index, x);
}
void glVertexAttrib1dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(572);
    if (!tglmt::GateCheck(940)) return;
    gl::glVertexAttrib1dv(index, v);
}
void glVertexAttrib1f(GLuint index, GLfloat x) {
    GlProfileScope _gp(573);
    if (!tglmt::GateCheck(941)) return;
    gl::glVertexAttrib1f(index, x);
}
void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(574);
    if (!tglmt::GateCheck(942)) return;
    gl::glVertexAttrib1fv(index, v);
}
void glVertexAttrib1s(GLuint index, GLshort x) {
    GlProfileScope _gp(575);
    if (!tglmt::GateCheck(943)) return;
    gl::glVertexAttrib1s(index, x);
}
void glVertexAttrib1sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(576);
    if (!tglmt::GateCheck(944)) return;
    gl::glVertexAttrib1sv(index, v);
}
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {
    GlProfileScope _gp(577);
    if (!tglmt::GateCheck(945)) return;
    gl::glVertexAttrib2d(index, x, y);
}
void glVertexAttrib2dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(578);
    if (!tglmt::GateCheck(946)) return;
    gl::glVertexAttrib2dv(index, v);
}
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    GlProfileScope _gp(579);
    if (!tglmt::GateCheck(947)) return;
    gl::glVertexAttrib2f(index, x, y);
}
void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(580);
    if (!tglmt::GateCheck(948)) return;
    gl::glVertexAttrib2fv(index, v);
}
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {
    GlProfileScope _gp(581);
    if (!tglmt::GateCheck(949)) return;
    gl::glVertexAttrib2s(index, x, y);
}
void glVertexAttrib2sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(582);
    if (!tglmt::GateCheck(950)) return;
    gl::glVertexAttrib2sv(index, v);
}
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(583);
    if (!tglmt::GateCheck(951)) return;
    gl::glVertexAttrib3d(index, x, y, z);
}
void glVertexAttrib3dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(584);
    if (!tglmt::GateCheck(952)) return;
    gl::glVertexAttrib3dv(index, v);
}
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(585);
    if (!tglmt::GateCheck(953)) return;
    gl::glVertexAttrib3f(index, x, y, z);
}
void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(586);
    if (!tglmt::GateCheck(954)) return;
    gl::glVertexAttrib3fv(index, v);
}
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {
    GlProfileScope _gp(587);
    if (!tglmt::GateCheck(955)) return;
    gl::glVertexAttrib3s(index, x, y, z);
}
void glVertexAttrib3sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(588);
    if (!tglmt::GateCheck(956)) return;
    gl::glVertexAttrib3sv(index, v);
}
void glVertexAttrib4Nbv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(589);
    if (!tglmt::GateCheck(957)) return;
    gl::glVertexAttrib4Nbv(index, v);
}
void glVertexAttrib4Niv(GLuint index, const GLint *v) {
    GlProfileScope _gp(590);
    if (!tglmt::GateCheck(958)) return;
    gl::glVertexAttrib4Niv(index, v);
}
void glVertexAttrib4Nsv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(591);
    if (!tglmt::GateCheck(959)) return;
    gl::glVertexAttrib4Nsv(index, v);
}
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    GlProfileScope _gp(592);
    if (!tglmt::GateCheck(960)) return;
    gl::glVertexAttrib4Nub(index, x, y, z, w);
}
void glVertexAttrib4Nubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(593);
    if (!tglmt::GateCheck(961)) return;
    gl::glVertexAttrib4Nubv(index, v);
}
void glVertexAttrib4Nuiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(594);
    if (!tglmt::GateCheck(962)) return;
    gl::glVertexAttrib4Nuiv(index, v);
}
void glVertexAttrib4Nusv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(595);
    if (!tglmt::GateCheck(963)) return;
    gl::glVertexAttrib4Nusv(index, v);
}
void glVertexAttrib4bv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(596);
    if (!tglmt::GateCheck(964)) return;
    gl::glVertexAttrib4bv(index, v);
}
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(597);
    if (!tglmt::GateCheck(965)) return;
    gl::glVertexAttrib4d(index, x, y, z, w);
}
void glVertexAttrib4dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(598);
    if (!tglmt::GateCheck(966)) return;
    gl::glVertexAttrib4dv(index, v);
}
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GlProfileScope _gp(599);
    if (!tglmt::GateCheck(967)) return;
    gl::glVertexAttrib4f(index, x, y, z, w);
}
void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(600);
    if (!tglmt::GateCheck(968)) return;
    gl::glVertexAttrib4fv(index, v);
}
void glVertexAttrib4iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(601);
    if (!tglmt::GateCheck(969)) return;
    gl::glVertexAttrib4iv(index, v);
}
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {
    GlProfileScope _gp(602);
    if (!tglmt::GateCheck(970)) return;
    gl::glVertexAttrib4s(index, x, y, z, w);
}
void glVertexAttrib4sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(603);
    if (!tglmt::GateCheck(971)) return;
    gl::glVertexAttrib4sv(index, v);
}
void glVertexAttrib4ubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(604);
    if (!tglmt::GateCheck(972)) return;
    gl::glVertexAttrib4ubv(index, v);
}
void glVertexAttrib4uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(605);
    if (!tglmt::GateCheck(973)) return;
    gl::glVertexAttrib4uiv(index, v);
}
void glVertexAttrib4usv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(606);
    if (!tglmt::GateCheck(974)) return;
    gl::glVertexAttrib4usv(index, v);
}
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    GlProfileScope _gp(607);
    if (!tglmt::GateCheck(975)) return;
    gl::glVertexAttribBinding(attribindex, bindingindex);
}
void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    GlProfileScope _gp(608);
    if (!tglmt::GateCheck(976)) return;
    gl::glVertexAttribDivisor(index, divisor);
}
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GlProfileScope _gp(609);
    if (!tglmt::GateCheck(977)) return;
    gl::glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
}
void glVertexAttribI1i(GLuint index, GLint x) {
    GlProfileScope _gp(610);
    if (!tglmt::GateCheck(978)) return;
    gl::glVertexAttribI1i(index, x);
}
void glVertexAttribI1iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(611);
    if (!tglmt::GateCheck(979)) return;
    gl::glVertexAttribI1iv(index, v);
}
void glVertexAttribI1ui(GLuint index, GLuint x) {
    GlProfileScope _gp(612);
    if (!tglmt::GateCheck(980)) return;
    gl::glVertexAttribI1ui(index, x);
}
void glVertexAttribI1uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(613);
    if (!tglmt::GateCheck(981)) return;
    gl::glVertexAttribI1uiv(index, v);
}
void glVertexAttribI2i(GLuint index, GLint x, GLint y) {
    GlProfileScope _gp(614);
    if (!tglmt::GateCheck(982)) return;
    gl::glVertexAttribI2i(index, x, y);
}
void glVertexAttribI2iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(615);
    if (!tglmt::GateCheck(983)) return;
    gl::glVertexAttribI2iv(index, v);
}
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) {
    GlProfileScope _gp(616);
    if (!tglmt::GateCheck(984)) return;
    gl::glVertexAttribI2ui(index, x, y);
}
void glVertexAttribI2uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(617);
    if (!tglmt::GateCheck(985)) return;
    gl::glVertexAttribI2uiv(index, v);
}
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) {
    GlProfileScope _gp(618);
    if (!tglmt::GateCheck(986)) return;
    gl::glVertexAttribI3i(index, x, y, z);
}
void glVertexAttribI3iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(619);
    if (!tglmt::GateCheck(987)) return;
    gl::glVertexAttribI3iv(index, v);
}
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) {
    GlProfileScope _gp(620);
    if (!tglmt::GateCheck(988)) return;
    gl::glVertexAttribI3ui(index, x, y, z);
}
void glVertexAttribI3uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(621);
    if (!tglmt::GateCheck(989)) return;
    gl::glVertexAttribI3uiv(index, v);
}
void glVertexAttribI4bv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(622);
    if (!tglmt::GateCheck(990)) return;
    gl::glVertexAttribI4bv(index, v);
}
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    GlProfileScope _gp(623);
    if (!tglmt::GateCheck(991)) return;
    gl::glVertexAttribI4i(index, x, y, z, w);
}
void glVertexAttribI4iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(624);
    if (!tglmt::GateCheck(992)) return;
    gl::glVertexAttribI4iv(index, v);
}
void glVertexAttribI4sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(625);
    if (!tglmt::GateCheck(993)) return;
    gl::glVertexAttribI4sv(index, v);
}
void glVertexAttribI4ubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(626);
    if (!tglmt::GateCheck(994)) return;
    gl::glVertexAttribI4ubv(index, v);
}
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    GlProfileScope _gp(627);
    if (!tglmt::GateCheck(995)) return;
    gl::glVertexAttribI4ui(index, x, y, z, w);
}
void glVertexAttribI4uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(628);
    if (!tglmt::GateCheck(996)) return;
    gl::glVertexAttribI4uiv(index, v);
}
void glVertexAttribI4usv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(629);
    if (!tglmt::GateCheck(997)) return;
    gl::glVertexAttribI4usv(index, v);
}
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(630);
    if (!tglmt::GateCheck(998)) return;
    gl::glVertexAttribIFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(631);
    if (!tglmt::GateCheck(999)) return;
    gl::glVertexAttribIPointer(index, size, type, stride, pointer);
}
void glVertexAttribL1d(GLuint index, GLdouble x) {
    GlProfileScope _gp(632);
    if (!tglmt::GateCheck(1000)) return;
    gl::glVertexAttribL1d(index, x);
}
void glVertexAttribL1dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(633);
    if (!tglmt::GateCheck(1001)) return;
    gl::glVertexAttribL1dv(index, v);
}
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {
    GlProfileScope _gp(634);
    if (!tglmt::GateCheck(1002)) return;
    gl::glVertexAttribL2d(index, x, y);
}
void glVertexAttribL2dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(635);
    if (!tglmt::GateCheck(1003)) return;
    gl::glVertexAttribL2dv(index, v);
}
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(636);
    if (!tglmt::GateCheck(1004)) return;
    gl::glVertexAttribL3d(index, x, y, z);
}
void glVertexAttribL3dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(637);
    if (!tglmt::GateCheck(1005)) return;
    gl::glVertexAttribL3dv(index, v);
}
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(638);
    if (!tglmt::GateCheck(1006)) return;
    gl::glVertexAttribL4d(index, x, y, z, w);
}
void glVertexAttribL4dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(639);
    if (!tglmt::GateCheck(1007)) return;
    gl::glVertexAttribL4dv(index, v);
}
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(640);
    if (!tglmt::GateCheck(1008)) return;
    gl::glVertexAttribLFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(641);
    if (!tglmt::GateCheck(1009)) return;
    gl::glVertexAttribLPointer(index, size, type, stride, pointer);
}
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(642);
    if (!tglmt::GateCheck(1010)) return;
    gl::glVertexAttribP1ui(index, type, normalized, value);
}
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(643);
    if (!tglmt::GateCheck(1011)) return;
    gl::glVertexAttribP1uiv(index, type, normalized, value);
}
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(644);
    if (!tglmt::GateCheck(1012)) return;
    gl::glVertexAttribP2ui(index, type, normalized, value);
}
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(645);
    if (!tglmt::GateCheck(1013)) return;
    gl::glVertexAttribP2uiv(index, type, normalized, value);
}
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(646);
    if (!tglmt::GateCheck(1014)) return;
    gl::glVertexAttribP3ui(index, type, normalized, value);
}
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(647);
    if (!tglmt::GateCheck(1015)) return;
    gl::glVertexAttribP3uiv(index, type, normalized, value);
}
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(648);
    if (!tglmt::GateCheck(1016)) return;
    gl::glVertexAttribP4ui(index, type, normalized, value);
}
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(649);
    if (!tglmt::GateCheck(1017)) return;
    gl::glVertexAttribP4uiv(index, type, normalized, value);
}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(650);
    if (!tglmt::GateCheck(1018)) return;
    gl::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    GlProfileScope _gp(651);
    if (!tglmt::GateCheck(1019)) return;
    gl::glVertexBindingDivisor(bindingindex, divisor);
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(652);
    if (!tglmt::GateCheck(1027)) return;
    gl::glViewport(x, y, width, height);
}
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v) {
    GlProfileScope _gp(653);
    if (!tglmt::GateCheck(1028)) return;
    gl::glViewportArrayv(first, count, v);
}
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    GlProfileScope _gp(654);
    if (!tglmt::GateCheck(1029)) return;
    gl::glViewportIndexedf(index, x, y, w, h);
}
void glViewportIndexedfv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(655);
    if (!tglmt::GateCheck(1030)) return;
    gl::glViewportIndexedfv(index, v);
}
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    GlProfileScope _gp(656);
    if (!tglmt::GateCheck(1031)) return;
    gl::glWaitSync(sync, flags, timeout);
}
void glColorP3ui(GLenum type, GLuint color) {
    GlProfileScope _gp(657);
    if (!tglmt::GateCheck(114)) return;
    gl::glColorP3ui(type, color);
}
void glColorP3uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(658);
    if (!tglmt::GateCheck(115)) return;
    gl::glColorP3uiv(type, color);
}
void glColorP4ui(GLenum type, GLuint color) {
    GlProfileScope _gp(659);
    if (!tglmt::GateCheck(116)) return;
    gl::glColorP4ui(type, color);
}
void glColorP4uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(660);
    if (!tglmt::GateCheck(117)) return;
    gl::glColorP4uiv(type, color);
}
void glGetnColorTable(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table) {
    GlProfileScope _gp(661);
    if (!tglmt::GateCheck(405)) return;
    gl::glGetnColorTable(target, format, type, bufSize, table);
}
void glGetnConvolutionFilter(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image) {
    GlProfileScope _gp(662);
    if (!tglmt::GateCheck(407)) return;
    gl::glGetnConvolutionFilter(target, format, type, bufSize, image);
}
void glGetnHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    GlProfileScope _gp(663);
    if (!tglmt::GateCheck(408)) return;
    gl::glGetnHistogram(target, reset, format, type, bufSize, values);
}
void glGetnMapdv(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v) {
    GlProfileScope _gp(664);
    if (!tglmt::GateCheck(409)) return;
    gl::glGetnMapdv(target, query, bufSize, v);
}
void glGetnMapfv(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v) {
    GlProfileScope _gp(665);
    if (!tglmt::GateCheck(410)) return;
    gl::glGetnMapfv(target, query, bufSize, v);
}
void glGetnMapiv(GLenum target, GLenum query, GLsizei bufSize, GLint *v) {
    GlProfileScope _gp(666);
    if (!tglmt::GateCheck(411)) return;
    gl::glGetnMapiv(target, query, bufSize, v);
}
void glGetnMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    GlProfileScope _gp(667);
    if (!tglmt::GateCheck(412)) return;
    gl::glGetnMinmax(target, reset, format, type, bufSize, values);
}
void glGetnPixelMapfv(GLenum map, GLsizei bufSize, GLfloat *values) {
    GlProfileScope _gp(668);
    if (!tglmt::GateCheck(413)) return;
    gl::glGetnPixelMapfv(map, bufSize, values);
}
void glGetnPixelMapuiv(GLenum map, GLsizei bufSize, GLuint *values) {
    GlProfileScope _gp(669);
    if (!tglmt::GateCheck(414)) return;
    gl::glGetnPixelMapuiv(map, bufSize, values);
}
void glGetnPixelMapusv(GLenum map, GLsizei bufSize, GLushort *values) {
    GlProfileScope _gp(670);
    if (!tglmt::GateCheck(415)) return;
    gl::glGetnPixelMapusv(map, bufSize, values);
}
void glGetnPolygonStipple(GLsizei bufSize, GLubyte *pattern) {
    GlProfileScope _gp(671);
    if (!tglmt::GateCheck(416)) return;
    gl::glGetnPolygonStipple(bufSize, pattern);
}
void glGetnSeparableFilter(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span) {
    GlProfileScope _gp(672);
    if (!tglmt::GateCheck(417)) return;
    gl::glGetnSeparableFilter(target, format, type, rowBufSize, row, columnBufSize, column, span);
}
void glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(673);
    if (!tglmt::GateCheck(543)) return;
    gl::glMultiTexCoordP1ui(texture, type, coords);
}
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(674);
    if (!tglmt::GateCheck(544)) return;
    gl::glMultiTexCoordP1uiv(texture, type, coords);
}
void glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(675);
    if (!tglmt::GateCheck(545)) return;
    gl::glMultiTexCoordP2ui(texture, type, coords);
}
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(676);
    if (!tglmt::GateCheck(546)) return;
    gl::glMultiTexCoordP2uiv(texture, type, coords);
}
void glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(677);
    if (!tglmt::GateCheck(547)) return;
    gl::glMultiTexCoordP3ui(texture, type, coords);
}
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(678);
    if (!tglmt::GateCheck(548)) return;
    gl::glMultiTexCoordP3uiv(texture, type, coords);
}
void glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(679);
    if (!tglmt::GateCheck(549)) return;
    gl::glMultiTexCoordP4ui(texture, type, coords);
}
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(680);
    if (!tglmt::GateCheck(550)) return;
    gl::glMultiTexCoordP4uiv(texture, type, coords);
}
void glNormalP3ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(681);
    if (!tglmt::GateCheck(574)) return;
    gl::glNormalP3ui(type, coords);
}
void glNormalP3uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(682);
    if (!tglmt::GateCheck(575)) return;
    gl::glNormalP3uiv(type, coords);
}
void glSecondaryColorP3ui(GLenum type, GLuint color) {
    GlProfileScope _gp(683);
    if (!tglmt::GateCheck(739)) return;
    gl::glSecondaryColorP3ui(type, color);
}
void glSecondaryColorP3uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(684);
    if (!tglmt::GateCheck(740)) return;
    gl::glSecondaryColorP3uiv(type, color);
}
void glTexCoordP1ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(685);
    if (!tglmt::GateCheck(788)) return;
    gl::glTexCoordP1ui(type, coords);
}
void glTexCoordP1uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(686);
    if (!tglmt::GateCheck(789)) return;
    gl::glTexCoordP1uiv(type, coords);
}
void glTexCoordP2ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(687);
    if (!tglmt::GateCheck(790)) return;
    gl::glTexCoordP2ui(type, coords);
}
void glTexCoordP2uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(688);
    if (!tglmt::GateCheck(791)) return;
    gl::glTexCoordP2uiv(type, coords);
}
void glTexCoordP3ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(689);
    if (!tglmt::GateCheck(792)) return;
    gl::glTexCoordP3ui(type, coords);
}
void glTexCoordP3uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(690);
    if (!tglmt::GateCheck(793)) return;
    gl::glTexCoordP3uiv(type, coords);
}
void glTexCoordP4ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(691);
    if (!tglmt::GateCheck(794)) return;
    gl::glTexCoordP4ui(type, coords);
}
void glTexCoordP4uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(692);
    if (!tglmt::GateCheck(795)) return;
    gl::glTexCoordP4uiv(type, coords);
}
void glVertexP2ui(GLenum type, GLuint value) {
    GlProfileScope _gp(693);
    if (!tglmt::GateCheck(1020)) return;
    gl::glVertexP2ui(type, value);
}
void glVertexP2uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(694);
    if (!tglmt::GateCheck(1021)) return;
    gl::glVertexP2uiv(type, value);
}
void glVertexP3ui(GLenum type, GLuint value) {
    GlProfileScope _gp(695);
    if (!tglmt::GateCheck(1022)) return;
    gl::glVertexP3ui(type, value);
}
void glVertexP3uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(696);
    if (!tglmt::GateCheck(1023)) return;
    gl::glVertexP3uiv(type, value);
}
void glVertexP4ui(GLenum type, GLuint value) {
    GlProfileScope _gp(697);
    if (!tglmt::GateCheck(1024)) return;
    gl::glVertexP4ui(type, value);
}
void glVertexP4uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(698);
    if (!tglmt::GateCheck(1025)) return;
    gl::glVertexP4uiv(type, value);
}
void glAccum(GLenum op, GLfloat value) {
    GlProfileScope _gp(699);
    if (!tglmt::GateCheck(0)) return;
    gl::glAccum(op, value);
}
void glAlphaFunc(GLenum func, GLfloat ref) {
    GlProfileScope _gp(700);
    if (!tglmt::GateCheck(3)) return;
    gl::glAlphaFunc(func, ref);
}
GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences) {
    GlProfileScope _gp(701);
    if (!tglmt::GateCheck(4)) return 0;
    return gl::glAreTexturesResident(n, textures, residences);
}
void glArrayElement(GLint i) {
    GlProfileScope _gp(702);
    if (!tglmt::GateCheck(5)) return;
    gl::glArrayElement(i);
}
void glBegin(GLenum mode) {
    GlProfileScope _gp(703);
    if (!tglmt::GateCheck(7)) return;
    gl::glBegin(mode);
}
void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
    GlProfileScope _gp(704);
    if (!tglmt::GateCheck(34)) return;
    gl::glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}
void glCallList(GLuint list) {
    GlProfileScope _gp(705);
    if (!tglmt::GateCheck(49)) return;
    gl::glCallList(list);
}
void glCallLists(GLsizei n, GLenum type, const void *lists) {
    GlProfileScope _gp(706);
    if (!tglmt::GateCheck(50)) return;
    gl::glCallLists(n, type, lists);
}
void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(707);
    if (!tglmt::GateCheck(55)) return;
    gl::glClearAccum(red, green, blue, alpha);
}
void glClearIndex(GLfloat c) {
    GlProfileScope _gp(708);
    if (!tglmt::GateCheck(65)) return;
    gl::glClearIndex(c);
}
void glClientActiveTexture(GLenum texture) {
    GlProfileScope _gp(709);
    if (!tglmt::GateCheck(75)) return;
    gl::glClientActiveTexture(texture);
}
void glClipPlane(GLenum plane, const GLdouble *equation) {
    GlProfileScope _gp(710);
    if (!tglmt::GateCheck(78)) return;
    gl::glClipPlane(plane, equation);
}
void glColor3b(GLbyte red, GLbyte green, GLbyte blue) {
    GlProfileScope _gp(711);
    if (!tglmt::GateCheck(79)) return;
    gl::glColor3b(red, green, blue);
}
void glColor3bv(const GLbyte *v) {
    GlProfileScope _gp(712);
    if (!tglmt::GateCheck(80)) return;
    gl::glColor3bv(v);
}
void glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
    GlProfileScope _gp(713);
    if (!tglmt::GateCheck(81)) return;
    gl::glColor3d(red, green, blue);
}
void glColor3dv(const GLdouble *v) {
    GlProfileScope _gp(714);
    if (!tglmt::GateCheck(82)) return;
    gl::glColor3dv(v);
}
void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    GlProfileScope _gp(715);
    if (!tglmt::GateCheck(83)) return;
    gl::glColor3f(red, green, blue);
}
void glColor3fv(const GLfloat *v) {
    GlProfileScope _gp(716);
    if (!tglmt::GateCheck(84)) return;
    gl::glColor3fv(v);
}
void glColor3i(GLint red, GLint green, GLint blue) {
    GlProfileScope _gp(717);
    if (!tglmt::GateCheck(85)) return;
    gl::glColor3i(red, green, blue);
}
void glColor3iv(const GLint *v) {
    GlProfileScope _gp(718);
    if (!tglmt::GateCheck(86)) return;
    gl::glColor3iv(v);
}
void glColor3s(GLshort red, GLshort green, GLshort blue) {
    GlProfileScope _gp(719);
    if (!tglmt::GateCheck(87)) return;
    gl::glColor3s(red, green, blue);
}
void glColor3sv(const GLshort *v) {
    GlProfileScope _gp(720);
    if (!tglmt::GateCheck(88)) return;
    gl::glColor3sv(v);
}
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    GlProfileScope _gp(721);
    if (!tglmt::GateCheck(89)) return;
    gl::glColor3ub(red, green, blue);
}
void glColor3ubv(const GLubyte *v) {
    GlProfileScope _gp(722);
    if (!tglmt::GateCheck(90)) return;
    gl::glColor3ubv(v);
}
void glColor3ui(GLuint red, GLuint green, GLuint blue) {
    GlProfileScope _gp(723);
    if (!tglmt::GateCheck(91)) return;
    gl::glColor3ui(red, green, blue);
}
void glColor3uiv(const GLuint *v) {
    GlProfileScope _gp(724);
    if (!tglmt::GateCheck(92)) return;
    gl::glColor3uiv(v);
}
void glColor3us(GLushort red, GLushort green, GLushort blue) {
    GlProfileScope _gp(725);
    if (!tglmt::GateCheck(93)) return;
    gl::glColor3us(red, green, blue);
}
void glColor3usv(const GLushort *v) {
    GlProfileScope _gp(726);
    if (!tglmt::GateCheck(94)) return;
    gl::glColor3usv(v);
}
void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
    GlProfileScope _gp(727);
    if (!tglmt::GateCheck(95)) return;
    gl::glColor4b(red, green, blue, alpha);
}
void glColor4bv(const GLbyte *v) {
    GlProfileScope _gp(728);
    if (!tglmt::GateCheck(96)) return;
    gl::glColor4bv(v);
}
void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
    GlProfileScope _gp(729);
    if (!tglmt::GateCheck(97)) return;
    gl::glColor4d(red, green, blue, alpha);
}
void glColor4dv(const GLdouble *v) {
    GlProfileScope _gp(730);
    if (!tglmt::GateCheck(98)) return;
    gl::glColor4dv(v);
}
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(731);
    if (!tglmt::GateCheck(99)) return;
    gl::glColor4f(red, green, blue, alpha);
}
void glColor4fv(const GLfloat *v) {
    GlProfileScope _gp(732);
    if (!tglmt::GateCheck(100)) return;
    gl::glColor4fv(v);
}
void glColor4i(GLint red, GLint green, GLint blue, GLint alpha) {
    GlProfileScope _gp(733);
    if (!tglmt::GateCheck(101)) return;
    gl::glColor4i(red, green, blue, alpha);
}
void glColor4iv(const GLint *v) {
    GlProfileScope _gp(734);
    if (!tglmt::GateCheck(102)) return;
    gl::glColor4iv(v);
}
void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha) {
    GlProfileScope _gp(735);
    if (!tglmt::GateCheck(103)) return;
    gl::glColor4s(red, green, blue, alpha);
}
void glColor4sv(const GLshort *v) {
    GlProfileScope _gp(736);
    if (!tglmt::GateCheck(104)) return;
    gl::glColor4sv(v);
}
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    GlProfileScope _gp(737);
    if (!tglmt::GateCheck(105)) return;
    gl::glColor4ub(red, green, blue, alpha);
}
void glColor4ubv(const GLubyte *v) {
    GlProfileScope _gp(738);
    if (!tglmt::GateCheck(106)) return;
    gl::glColor4ubv(v);
}
void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha) {
    GlProfileScope _gp(739);
    if (!tglmt::GateCheck(107)) return;
    gl::glColor4ui(red, green, blue, alpha);
}
void glColor4uiv(const GLuint *v) {
    GlProfileScope _gp(740);
    if (!tglmt::GateCheck(108)) return;
    gl::glColor4uiv(v);
}
void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) {
    GlProfileScope _gp(741);
    if (!tglmt::GateCheck(109)) return;
    gl::glColor4us(red, green, blue, alpha);
}
void glColor4usv(const GLushort *v) {
    GlProfileScope _gp(742);
    if (!tglmt::GateCheck(110)) return;
    gl::glColor4usv(v);
}
void glColorMaterial(GLenum face, GLenum mode) {
    GlProfileScope _gp(743);
    if (!tglmt::GateCheck(113)) return;
    gl::glColorMaterial(face, mode);
}
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(744);
    if (!tglmt::GateCheck(118)) return;
    gl::glColorPointer(size, type, stride, pointer);
}
void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
    GlProfileScope _gp(745);
    if (!tglmt::GateCheck(132)) return;
    gl::glCopyPixels(x, y, width, height, type);
}
void glDeleteLists(GLuint list, GLsizei range) {
    GlProfileScope _gp(746);
    if (!tglmt::GateCheck(159)) return;
    gl::glDeleteLists(list, range);
}
void glDisableClientState(GLenum array) {
    GlProfileScope _gp(747);
    if (!tglmt::GateCheck(178)) return;
    gl::glDisableClientState(array);
}
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(748);
    if (!tglmt::GateCheck(197)) return;
    gl::glDrawPixels(width, height, format, type, pixels);
}
void glEdgeFlag(GLboolean flag) {
    GlProfileScope _gp(749);
    if (!tglmt::GateCheck(204)) return;
    gl::glEdgeFlag(flag);
}
void glEdgeFlagPointer(GLsizei stride, const void *pointer) {
    GlProfileScope _gp(750);
    if (!tglmt::GateCheck(205)) return;
    gl::glEdgeFlagPointer(stride, pointer);
}
void glEdgeFlagv(const GLboolean *flag) {
    GlProfileScope _gp(751);
    if (!tglmt::GateCheck(206)) return;
    gl::glEdgeFlagv(flag);
}
void glEnableClientState(GLenum array) {
    GlProfileScope _gp(752);
    if (!tglmt::GateCheck(208)) return;
    gl::glEnableClientState(array);
}
void glEnd(void) {
    GlProfileScope _gp(753);
    if (!tglmt::GateCheck(212)) return;
    gl::glEnd();
}
void glEndList(void) {
    GlProfileScope _gp(754);
    if (!tglmt::GateCheck(214)) return;
    gl::glEndList();
}
void glEvalCoord1d(GLdouble u) {
    GlProfileScope _gp(755);
    if (!tglmt::GateCheck(218)) return;
    gl::glEvalCoord1d(u);
}
void glEvalCoord1dv(const GLdouble *u) {
    GlProfileScope _gp(756);
    if (!tglmt::GateCheck(219)) return;
    gl::glEvalCoord1dv(u);
}
void glEvalCoord1f(GLfloat u) {
    GlProfileScope _gp(757);
    if (!tglmt::GateCheck(220)) return;
    gl::glEvalCoord1f(u);
}
void glEvalCoord1fv(const GLfloat *u) {
    GlProfileScope _gp(758);
    if (!tglmt::GateCheck(221)) return;
    gl::glEvalCoord1fv(u);
}
void glEvalCoord2d(GLdouble u, GLdouble v) {
    GlProfileScope _gp(759);
    if (!tglmt::GateCheck(222)) return;
    gl::glEvalCoord2d(u, v);
}
void glEvalCoord2dv(const GLdouble *u) {
    GlProfileScope _gp(760);
    if (!tglmt::GateCheck(223)) return;
    gl::glEvalCoord2dv(u);
}
void glEvalCoord2f(GLfloat u, GLfloat v) {
    GlProfileScope _gp(761);
    if (!tglmt::GateCheck(224)) return;
    gl::glEvalCoord2f(u, v);
}
void glEvalCoord2fv(const GLfloat *u) {
    GlProfileScope _gp(762);
    if (!tglmt::GateCheck(225)) return;
    gl::glEvalCoord2fv(u);
}
void glEvalMesh1(GLenum mode, GLint i1, GLint i2) {
    GlProfileScope _gp(763);
    if (!tglmt::GateCheck(226)) return;
    gl::glEvalMesh1(mode, i1, i2);
}
void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
    GlProfileScope _gp(764);
    if (!tglmt::GateCheck(227)) return;
    gl::glEvalMesh2(mode, i1, i2, j1, j2);
}
void glEvalPoint1(GLint i) {
    GlProfileScope _gp(765);
    if (!tglmt::GateCheck(228)) return;
    gl::glEvalPoint1(i);
}
void glEvalPoint2(GLint i, GLint j) {
    GlProfileScope _gp(766);
    if (!tglmt::GateCheck(229)) return;
    gl::glEvalPoint2(i, j);
}
void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer) {
    GlProfileScope _gp(767);
    if (!tglmt::GateCheck(230)) return;
    gl::glFeedbackBuffer(size, type, buffer);
}
void glFogCoordPointer(GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(768);
    if (!tglmt::GateCheck(236)) return;
    gl::glFogCoordPointer(type, stride, pointer);
}
void glFogCoordd(GLdouble coord) {
    GlProfileScope _gp(769);
    if (!tglmt::GateCheck(237)) return;
    gl::glFogCoordd(coord);
}
void glFogCoorddv(const GLdouble *coord) {
    GlProfileScope _gp(770);
    if (!tglmt::GateCheck(238)) return;
    gl::glFogCoorddv(coord);
}
void glFogCoordf(GLfloat coord) {
    GlProfileScope _gp(771);
    if (!tglmt::GateCheck(239)) return;
    gl::glFogCoordf(coord);
}
void glFogCoordfv(const GLfloat *coord) {
    GlProfileScope _gp(772);
    if (!tglmt::GateCheck(240)) return;
    gl::glFogCoordfv(coord);
}
void glFogf(GLenum pname, GLfloat param) {
    GlProfileScope _gp(773);
    if (!tglmt::GateCheck(241)) return;
    gl::glFogf(pname, param);
}
void glFogfv(GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(774);
    if (!tglmt::GateCheck(242)) return;
    gl::glFogfv(pname, params);
}
void glFogi(GLenum pname, GLint param) {
    GlProfileScope _gp(775);
    if (!tglmt::GateCheck(243)) return;
    gl::glFogi(pname, param);
}
void glFogiv(GLenum pname, const GLint *params) {
    GlProfileScope _gp(776);
    if (!tglmt::GateCheck(244)) return;
    gl::glFogiv(pname, params);
}
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GlProfileScope _gp(777);
    if (!tglmt::GateCheck(253)) return;
    gl::glFrustum(left, right, bottom, top, zNear, zFar);
}
GLuint glGenLists(GLsizei range) {
    GlProfileScope _gp(778);
    if (!tglmt::GateCheck(256)) return 0;
    return gl::glGenLists(range);
}
void glGetClipPlane(GLenum plane, GLdouble *equation) {
    GlProfileScope _gp(779);
    if (!tglmt::GateCheck(284)) return;
    gl::glGetClipPlane(plane, equation);
}
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(780);
    if (!tglmt::GateCheck(305)) return;
    gl::glGetLightfv(light, pname, params);
}
void glGetLightiv(GLenum light, GLenum pname, GLint *params) {
    GlProfileScope _gp(781);
    if (!tglmt::GateCheck(306)) return;
    gl::glGetLightiv(light, pname, params);
}
void glGetMapdv(GLenum target, GLenum query, GLdouble *v) {
    GlProfileScope _gp(782);
    if (!tglmt::GateCheck(307)) return;
    gl::glGetMapdv(target, query, v);
}
void glGetMapfv(GLenum target, GLenum query, GLfloat *v) {
    GlProfileScope _gp(783);
    if (!tglmt::GateCheck(308)) return;
    gl::glGetMapfv(target, query, v);
}
void glGetMapiv(GLenum target, GLenum query, GLint *v) {
    GlProfileScope _gp(784);
    if (!tglmt::GateCheck(309)) return;
    gl::glGetMapiv(target, query, v);
}
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(785);
    if (!tglmt::GateCheck(310)) return;
    gl::glGetMaterialfv(face, pname, params);
}
void glGetMaterialiv(GLenum face, GLenum pname, GLint *params) {
    GlProfileScope _gp(786);
    if (!tglmt::GateCheck(311)) return;
    gl::glGetMaterialiv(face, pname, params);
}
void glGetPixelMapfv(GLenum map, GLfloat *values) {
    GlProfileScope _gp(787);
    if (!tglmt::GateCheck(322)) return;
    gl::glGetPixelMapfv(map, values);
}
void glGetPixelMapuiv(GLenum map, GLuint *values) {
    GlProfileScope _gp(788);
    if (!tglmt::GateCheck(323)) return;
    gl::glGetPixelMapuiv(map, values);
}
void glGetPixelMapusv(GLenum map, GLushort *values) {
    GlProfileScope _gp(789);
    if (!tglmt::GateCheck(324)) return;
    gl::glGetPixelMapusv(map, values);
}
void glGetPolygonStipple(GLubyte *mask) {
    GlProfileScope _gp(790);
    if (!tglmt::GateCheck(326)) return;
    gl::glGetPolygonStipple(mask);
}
void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(791);
    if (!tglmt::GateCheck(363)) return;
    gl::glGetTexEnvfv(target, pname, params);
}
void glGetTexEnviv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(792);
    if (!tglmt::GateCheck(364)) return;
    gl::glGetTexEnviv(target, pname, params);
}
void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params) {
    GlProfileScope _gp(793);
    if (!tglmt::GateCheck(365)) return;
    gl::glGetTexGendv(coord, pname, params);
}
void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(794);
    if (!tglmt::GateCheck(366)) return;
    gl::glGetTexGenfv(coord, pname, params);
}
void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params) {
    GlProfileScope _gp(795);
    if (!tglmt::GateCheck(367)) return;
    gl::glGetTexGeniv(coord, pname, params);
}
void glIndexMask(GLuint mask) {
    GlProfileScope _gp(796);
    if (!tglmt::GateCheck(424)) return;
    gl::glIndexMask(mask);
}
void glIndexPointer(GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(797);
    if (!tglmt::GateCheck(425)) return;
    gl::glIndexPointer(type, stride, pointer);
}
void glIndexd(GLdouble c) {
    GlProfileScope _gp(798);
    if (!tglmt::GateCheck(426)) return;
    gl::glIndexd(c);
}
void glIndexdv(const GLdouble *c) {
    GlProfileScope _gp(799);
    if (!tglmt::GateCheck(427)) return;
    gl::glIndexdv(c);
}
void glIndexf(GLfloat c) {
    GlProfileScope _gp(800);
    if (!tglmt::GateCheck(428)) return;
    gl::glIndexf(c);
}
void glIndexfv(const GLfloat *c) {
    GlProfileScope _gp(801);
    if (!tglmt::GateCheck(429)) return;
    gl::glIndexfv(c);
}
void glIndexi(GLint c) {
    GlProfileScope _gp(802);
    if (!tglmt::GateCheck(430)) return;
    gl::glIndexi(c);
}
void glIndexiv(const GLint *c) {
    GlProfileScope _gp(803);
    if (!tglmt::GateCheck(431)) return;
    gl::glIndexiv(c);
}
void glIndexs(GLshort c) {
    GlProfileScope _gp(804);
    if (!tglmt::GateCheck(432)) return;
    gl::glIndexs(c);
}
void glIndexsv(const GLshort *c) {
    GlProfileScope _gp(805);
    if (!tglmt::GateCheck(433)) return;
    gl::glIndexsv(c);
}
void glIndexub(GLubyte c) {
    GlProfileScope _gp(806);
    if (!tglmt::GateCheck(434)) return;
    gl::glIndexub(c);
}
void glIndexubv(const GLubyte *c) {
    GlProfileScope _gp(807);
    if (!tglmt::GateCheck(435)) return;
    gl::glIndexubv(c);
}
void glInitNames(void) {
    GlProfileScope _gp(808);
    if (!tglmt::GateCheck(436)) return;
    gl::glInitNames();
}
void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(809);
    if (!tglmt::GateCheck(437)) return;
    gl::glInterleavedArrays(format, stride, pointer);
}
GLboolean glIsList(GLuint list) {
    GlProfileScope _gp(810);
    if (!tglmt::GateCheck(450)) return 0;
    return gl::glIsList(list);
}
void glLightModelf(GLenum pname, GLfloat param) {
    GlProfileScope _gp(811);
    if (!tglmt::GateCheck(461)) return;
    gl::glLightModelf(pname, param);
}
void glLightModelfv(GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(812);
    if (!tglmt::GateCheck(462)) return;
    gl::glLightModelfv(pname, params);
}
void glLightModeli(GLenum pname, GLint param) {
    GlProfileScope _gp(813);
    if (!tglmt::GateCheck(463)) return;
    gl::glLightModeli(pname, param);
}
void glLightModeliv(GLenum pname, const GLint *params) {
    GlProfileScope _gp(814);
    if (!tglmt::GateCheck(464)) return;
    gl::glLightModeliv(pname, params);
}
void glLightf(GLenum light, GLenum pname, GLfloat param) {
    GlProfileScope _gp(815);
    if (!tglmt::GateCheck(465)) return;
    gl::glLightf(light, pname, param);
}
void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(816);
    if (!tglmt::GateCheck(466)) return;
    gl::glLightfv(light, pname, params);
}
void glLighti(GLenum light, GLenum pname, GLint param) {
    GlProfileScope _gp(817);
    if (!tglmt::GateCheck(467)) return;
    gl::glLighti(light, pname, param);
}
void glLightiv(GLenum light, GLenum pname, const GLint *params) {
    GlProfileScope _gp(818);
    if (!tglmt::GateCheck(468)) return;
    gl::glLightiv(light, pname, params);
}
void glLineStipple(GLint factor, GLushort pattern) {
    GlProfileScope _gp(819);
    if (!tglmt::GateCheck(469)) return;
    gl::glLineStipple(factor, pattern);
}
void glListBase(GLuint base) {
    GlProfileScope _gp(820);
    if (!tglmt::GateCheck(472)) return;
    gl::glListBase(base);
}
void glLoadIdentity(void) {
    GlProfileScope _gp(821);
    if (!tglmt::GateCheck(473)) return;
    gl::glLoadIdentity();
}
void glLoadMatrixd(const GLdouble *m) {
    GlProfileScope _gp(822);
    if (!tglmt::GateCheck(474)) return;
    gl::glLoadMatrixd(m);
}
void glLoadMatrixf(const GLfloat *m) {
    GlProfileScope _gp(823);
    if (!tglmt::GateCheck(475)) return;
    gl::glLoadMatrixf(m);
}
void glLoadName(GLuint name) {
    GlProfileScope _gp(824);
    if (!tglmt::GateCheck(476)) return;
    gl::glLoadName(name);
}
void glLoadTransposeMatrixd(const GLdouble *m) {
    GlProfileScope _gp(825);
    if (!tglmt::GateCheck(477)) return;
    gl::glLoadTransposeMatrixd(m);
}
void glLoadTransposeMatrixf(const GLfloat *m) {
    GlProfileScope _gp(826);
    if (!tglmt::GateCheck(478)) return;
    gl::glLoadTransposeMatrixf(m);
}
void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
    GlProfileScope _gp(827);
    if (!tglmt::GateCheck(480)) return;
    gl::glMap1d(target, u1, u2, stride, order, points);
}
void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
    GlProfileScope _gp(828);
    if (!tglmt::GateCheck(481)) return;
    gl::glMap1f(target, u1, u2, stride, order, points);
}
void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
    GlProfileScope _gp(829);
    if (!tglmt::GateCheck(482)) return;
    gl::glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}
void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
    GlProfileScope _gp(830);
    if (!tglmt::GateCheck(483)) return;
    gl::glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}
void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) {
    GlProfileScope _gp(831);
    if (!tglmt::GateCheck(486)) return;
    gl::glMapGrid1d(un, u1, u2);
}
void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {
    GlProfileScope _gp(832);
    if (!tglmt::GateCheck(487)) return;
    gl::glMapGrid1f(un, u1, u2);
}
void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
    GlProfileScope _gp(833);
    if (!tglmt::GateCheck(488)) return;
    gl::glMapGrid2d(un, u1, u2, vn, v1, v2);
}
void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
    GlProfileScope _gp(834);
    if (!tglmt::GateCheck(489)) return;
    gl::glMapGrid2f(un, u1, u2, vn, v1, v2);
}
void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    GlProfileScope _gp(835);
    if (!tglmt::GateCheck(492)) return;
    gl::glMaterialf(face, pname, param);
}
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(836);
    if (!tglmt::GateCheck(493)) return;
    gl::glMaterialfv(face, pname, params);
}
void glMateriali(GLenum face, GLenum pname, GLint param) {
    GlProfileScope _gp(837);
    if (!tglmt::GateCheck(494)) return;
    gl::glMateriali(face, pname, param);
}
void glMaterialiv(GLenum face, GLenum pname, const GLint *params) {
    GlProfileScope _gp(838);
    if (!tglmt::GateCheck(495)) return;
    gl::glMaterialiv(face, pname, params);
}
void glMatrixMode(GLenum mode) {
    GlProfileScope _gp(839);
    if (!tglmt::GateCheck(496)) return;
    gl::glMatrixMode(mode);
}
void glMultMatrixd(const GLdouble *m) {
    GlProfileScope _gp(840);
    if (!tglmt::GateCheck(500)) return;
    gl::glMultMatrixd(m);
}
void glMultMatrixf(const GLfloat *m) {
    GlProfileScope _gp(841);
    if (!tglmt::GateCheck(501)) return;
    gl::glMultMatrixf(m);
}
void glMultTransposeMatrixd(const GLdouble *m) {
    GlProfileScope _gp(842);
    if (!tglmt::GateCheck(502)) return;
    gl::glMultTransposeMatrixd(m);
}
void glMultTransposeMatrixf(const GLfloat *m) {
    GlProfileScope _gp(843);
    if (!tglmt::GateCheck(503)) return;
    gl::glMultTransposeMatrixf(m);
}
void glMultiTexCoord1d(GLenum target, GLdouble s) {
    GlProfileScope _gp(844);
    if (!tglmt::GateCheck(511)) return;
    gl::glMultiTexCoord1d(target, s);
}
void glMultiTexCoord1dv(GLenum target, const GLdouble *v) {
    GlProfileScope _gp(845);
    if (!tglmt::GateCheck(512)) return;
    gl::glMultiTexCoord1dv(target, v);
}
void glMultiTexCoord1f(GLenum target, GLfloat s) {
    GlProfileScope _gp(846);
    if (!tglmt::GateCheck(513)) return;
    gl::glMultiTexCoord1f(target, s);
}
void glMultiTexCoord1fv(GLenum target, const GLfloat *v) {
    GlProfileScope _gp(847);
    if (!tglmt::GateCheck(514)) return;
    gl::glMultiTexCoord1fv(target, v);
}
void glMultiTexCoord1i(GLenum target, GLint s) {
    GlProfileScope _gp(848);
    if (!tglmt::GateCheck(515)) return;
    gl::glMultiTexCoord1i(target, s);
}
void glMultiTexCoord1iv(GLenum target, const GLint *v) {
    GlProfileScope _gp(849);
    if (!tglmt::GateCheck(516)) return;
    gl::glMultiTexCoord1iv(target, v);
}
void glMultiTexCoord1s(GLenum target, GLshort s) {
    GlProfileScope _gp(850);
    if (!tglmt::GateCheck(517)) return;
    gl::glMultiTexCoord1s(target, s);
}
void glMultiTexCoord1sv(GLenum target, const GLshort *v) {
    GlProfileScope _gp(851);
    if (!tglmt::GateCheck(518)) return;
    gl::glMultiTexCoord1sv(target, v);
}
void glMultiTexCoord2d(GLenum target, GLdouble s, GLdouble t) {
    GlProfileScope _gp(852);
    if (!tglmt::GateCheck(519)) return;
    gl::glMultiTexCoord2d(target, s, t);
}
void glMultiTexCoord2dv(GLenum target, const GLdouble *v) {
    GlProfileScope _gp(853);
    if (!tglmt::GateCheck(520)) return;
    gl::glMultiTexCoord2dv(target, v);
}
void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t) {
    GlProfileScope _gp(854);
    if (!tglmt::GateCheck(521)) return;
    gl::glMultiTexCoord2f(target, s, t);
}
void glMultiTexCoord2fv(GLenum target, const GLfloat *v) {
    GlProfileScope _gp(855);
    if (!tglmt::GateCheck(522)) return;
    gl::glMultiTexCoord2fv(target, v);
}
void glMultiTexCoord2i(GLenum target, GLint s, GLint t) {
    GlProfileScope _gp(856);
    if (!tglmt::GateCheck(523)) return;
    gl::glMultiTexCoord2i(target, s, t);
}
void glMultiTexCoord2iv(GLenum target, const GLint *v) {
    GlProfileScope _gp(857);
    if (!tglmt::GateCheck(524)) return;
    gl::glMultiTexCoord2iv(target, v);
}
void glMultiTexCoord2s(GLenum target, GLshort s, GLshort t) {
    GlProfileScope _gp(858);
    if (!tglmt::GateCheck(525)) return;
    gl::glMultiTexCoord2s(target, s, t);
}
void glMultiTexCoord2sv(GLenum target, const GLshort *v) {
    GlProfileScope _gp(859);
    if (!tglmt::GateCheck(526)) return;
    gl::glMultiTexCoord2sv(target, v);
}
void glMultiTexCoord3d(GLenum target, GLdouble s, GLdouble t, GLdouble r) {
    GlProfileScope _gp(860);
    if (!tglmt::GateCheck(527)) return;
    gl::glMultiTexCoord3d(target, s, t, r);
}
void glMultiTexCoord3dv(GLenum target, const GLdouble *v) {
    GlProfileScope _gp(861);
    if (!tglmt::GateCheck(528)) return;
    gl::glMultiTexCoord3dv(target, v);
}
void glMultiTexCoord3f(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
    GlProfileScope _gp(862);
    if (!tglmt::GateCheck(529)) return;
    gl::glMultiTexCoord3f(target, s, t, r);
}
void glMultiTexCoord3fv(GLenum target, const GLfloat *v) {
    GlProfileScope _gp(863);
    if (!tglmt::GateCheck(530)) return;
    gl::glMultiTexCoord3fv(target, v);
}
void glMultiTexCoord3i(GLenum target, GLint s, GLint t, GLint r) {
    GlProfileScope _gp(864);
    if (!tglmt::GateCheck(531)) return;
    gl::glMultiTexCoord3i(target, s, t, r);
}
void glMultiTexCoord3iv(GLenum target, const GLint *v) {
    GlProfileScope _gp(865);
    if (!tglmt::GateCheck(532)) return;
    gl::glMultiTexCoord3iv(target, v);
}
void glMultiTexCoord3s(GLenum target, GLshort s, GLshort t, GLshort r) {
    GlProfileScope _gp(866);
    if (!tglmt::GateCheck(533)) return;
    gl::glMultiTexCoord3s(target, s, t, r);
}
void glMultiTexCoord3sv(GLenum target, const GLshort *v) {
    GlProfileScope _gp(867);
    if (!tglmt::GateCheck(534)) return;
    gl::glMultiTexCoord3sv(target, v);
}
void glMultiTexCoord4d(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
    GlProfileScope _gp(868);
    if (!tglmt::GateCheck(535)) return;
    gl::glMultiTexCoord4d(target, s, t, r, q);
}
void glMultiTexCoord4dv(GLenum target, const GLdouble *v) {
    GlProfileScope _gp(869);
    if (!tglmt::GateCheck(536)) return;
    gl::glMultiTexCoord4dv(target, v);
}
void glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    GlProfileScope _gp(870);
    if (!tglmt::GateCheck(537)) return;
    gl::glMultiTexCoord4f(target, s, t, r, q);
}
void glMultiTexCoord4fv(GLenum target, const GLfloat *v) {
    GlProfileScope _gp(871);
    if (!tglmt::GateCheck(538)) return;
    gl::glMultiTexCoord4fv(target, v);
}
void glMultiTexCoord4i(GLenum target, GLint s, GLint t, GLint r, GLint q) {
    GlProfileScope _gp(872);
    if (!tglmt::GateCheck(539)) return;
    gl::glMultiTexCoord4i(target, s, t, r, q);
}
void glMultiTexCoord4iv(GLenum target, const GLint *v) {
    GlProfileScope _gp(873);
    if (!tglmt::GateCheck(540)) return;
    gl::glMultiTexCoord4iv(target, v);
}
void glMultiTexCoord4s(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q) {
    GlProfileScope _gp(874);
    if (!tglmt::GateCheck(541)) return;
    gl::glMultiTexCoord4s(target, s, t, r, q);
}
void glMultiTexCoord4sv(GLenum target, const GLshort *v) {
    GlProfileScope _gp(875);
    if (!tglmt::GateCheck(542)) return;
    gl::glMultiTexCoord4sv(target, v);
}
void glNewList(GLuint list, GLenum mode) {
    GlProfileScope _gp(876);
    if (!tglmt::GateCheck(563)) return;
    gl::glNewList(list, mode);
}
void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) {
    GlProfileScope _gp(877);
    if (!tglmt::GateCheck(564)) return;
    gl::glNormal3b(nx, ny, nz);
}
void glNormal3bv(const GLbyte *v) {
    GlProfileScope _gp(878);
    if (!tglmt::GateCheck(565)) return;
    gl::glNormal3bv(v);
}
void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
    GlProfileScope _gp(879);
    if (!tglmt::GateCheck(566)) return;
    gl::glNormal3d(nx, ny, nz);
}
void glNormal3dv(const GLdouble *v) {
    GlProfileScope _gp(880);
    if (!tglmt::GateCheck(567)) return;
    gl::glNormal3dv(v);
}
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    GlProfileScope _gp(881);
    if (!tglmt::GateCheck(568)) return;
    gl::glNormal3f(nx, ny, nz);
}
void glNormal3fv(const GLfloat *v) {
    GlProfileScope _gp(882);
    if (!tglmt::GateCheck(569)) return;
    gl::glNormal3fv(v);
}
void glNormal3i(GLint nx, GLint ny, GLint nz) {
    GlProfileScope _gp(883);
    if (!tglmt::GateCheck(570)) return;
    gl::glNormal3i(nx, ny, nz);
}
void glNormal3iv(const GLint *v) {
    GlProfileScope _gp(884);
    if (!tglmt::GateCheck(571)) return;
    gl::glNormal3iv(v);
}
void glNormal3s(GLshort nx, GLshort ny, GLshort nz) {
    GlProfileScope _gp(885);
    if (!tglmt::GateCheck(572)) return;
    gl::glNormal3s(nx, ny, nz);
}
void glNormal3sv(const GLshort *v) {
    GlProfileScope _gp(886);
    if (!tglmt::GateCheck(573)) return;
    gl::glNormal3sv(v);
}
void glNormalPointer(GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(887);
    if (!tglmt::GateCheck(576)) return;
    gl::glNormalPointer(type, stride, pointer);
}
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GlProfileScope _gp(888);
    if (!tglmt::GateCheck(579)) return;
    gl::glOrtho(left, right, bottom, top, zNear, zFar);
}
void glPassThrough(GLfloat token) {
    GlProfileScope _gp(889);
    if (!tglmt::GateCheck(580)) return;
    gl::glPassThrough(token);
}
void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values) {
    GlProfileScope _gp(890);
    if (!tglmt::GateCheck(584)) return;
    gl::glPixelMapfv(map, mapsize, values);
}
void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values) {
    GlProfileScope _gp(891);
    if (!tglmt::GateCheck(585)) return;
    gl::glPixelMapuiv(map, mapsize, values);
}
void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values) {
    GlProfileScope _gp(892);
    if (!tglmt::GateCheck(586)) return;
    gl::glPixelMapusv(map, mapsize, values);
}
void glPixelTransferf(GLenum pname, GLfloat param) {
    GlProfileScope _gp(893);
    if (!tglmt::GateCheck(589)) return;
    gl::glPixelTransferf(pname, param);
}
void glPixelTransferi(GLenum pname, GLint param) {
    GlProfileScope _gp(894);
    if (!tglmt::GateCheck(590)) return;
    gl::glPixelTransferi(pname, param);
}
void glPixelZoom(GLfloat xfactor, GLfloat yfactor) {
    GlProfileScope _gp(895);
    if (!tglmt::GateCheck(591)) return;
    gl::glPixelZoom(xfactor, yfactor);
}
void glPolygonStipple(const GLubyte *mask) {
    GlProfileScope _gp(896);
    if (!tglmt::GateCheck(600)) return;
    gl::glPolygonStipple(mask);
}
void glPopAttrib(void) {
    GlProfileScope _gp(897);
    if (!tglmt::GateCheck(601)) return;
    gl::glPopAttrib();
}
void glPopClientAttrib(void) {
    GlProfileScope _gp(898);
    if (!tglmt::GateCheck(602)) return;
    gl::glPopClientAttrib();
}
void glPopMatrix(void) {
    GlProfileScope _gp(899);
    if (!tglmt::GateCheck(604)) return;
    gl::glPopMatrix();
}
void glPopName(void) {
    GlProfileScope _gp(900);
    if (!tglmt::GateCheck(605)) return;
    gl::glPopName();
}
void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLfloat *priorities) {
    GlProfileScope _gp(901);
    if (!tglmt::GateCheck(607)) return;
    gl::glPrioritizeTextures(n, textures, priorities);
}
void glPushAttrib(GLbitfield mask) {
    GlProfileScope _gp(902);
    if (!tglmt::GateCheck(661)) return;
    gl::glPushAttrib(mask);
}
void glPushClientAttrib(GLbitfield mask) {
    GlProfileScope _gp(903);
    if (!tglmt::GateCheck(662)) return;
    gl::glPushClientAttrib(mask);
}
void glPushMatrix(void) {
    GlProfileScope _gp(904);
    if (!tglmt::GateCheck(664)) return;
    gl::glPushMatrix();
}
void glPushName(GLuint name) {
    GlProfileScope _gp(905);
    if (!tglmt::GateCheck(665)) return;
    gl::glPushName(name);
}
void glRasterPos2d(GLdouble x, GLdouble y) {
    GlProfileScope _gp(906);
    if (!tglmt::GateCheck(667)) return;
    gl::glRasterPos2d(x, y);
}
void glRasterPos2dv(const GLdouble *v) {
    GlProfileScope _gp(907);
    if (!tglmt::GateCheck(668)) return;
    gl::glRasterPos2dv(v);
}
void glRasterPos2f(GLfloat x, GLfloat y) {
    GlProfileScope _gp(908);
    if (!tglmt::GateCheck(669)) return;
    gl::glRasterPos2f(x, y);
}
void glRasterPos2fv(const GLfloat *v) {
    GlProfileScope _gp(909);
    if (!tglmt::GateCheck(670)) return;
    gl::glRasterPos2fv(v);
}
void glRasterPos2i(GLint x, GLint y) {
    GlProfileScope _gp(910);
    if (!tglmt::GateCheck(671)) return;
    gl::glRasterPos2i(x, y);
}
void glRasterPos2iv(const GLint *v) {
    GlProfileScope _gp(911);
    if (!tglmt::GateCheck(672)) return;
    gl::glRasterPos2iv(v);
}
void glRasterPos2s(GLshort x, GLshort y) {
    GlProfileScope _gp(912);
    if (!tglmt::GateCheck(673)) return;
    gl::glRasterPos2s(x, y);
}
void glRasterPos2sv(const GLshort *v) {
    GlProfileScope _gp(913);
    if (!tglmt::GateCheck(674)) return;
    gl::glRasterPos2sv(v);
}
void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(914);
    if (!tglmt::GateCheck(675)) return;
    gl::glRasterPos3d(x, y, z);
}
void glRasterPos3dv(const GLdouble *v) {
    GlProfileScope _gp(915);
    if (!tglmt::GateCheck(676)) return;
    gl::glRasterPos3dv(v);
}
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(916);
    if (!tglmt::GateCheck(677)) return;
    gl::glRasterPos3f(x, y, z);
}
void glRasterPos3fv(const GLfloat *v) {
    GlProfileScope _gp(917);
    if (!tglmt::GateCheck(678)) return;
    gl::glRasterPos3fv(v);
}
void glRasterPos3i(GLint x, GLint y, GLint z) {
    GlProfileScope _gp(918);
    if (!tglmt::GateCheck(679)) return;
    gl::glRasterPos3i(x, y, z);
}
void glRasterPos3iv(const GLint *v) {
    GlProfileScope _gp(919);
    if (!tglmt::GateCheck(680)) return;
    gl::glRasterPos3iv(v);
}
void glRasterPos3s(GLshort x, GLshort y, GLshort z) {
    GlProfileScope _gp(920);
    if (!tglmt::GateCheck(681)) return;
    gl::glRasterPos3s(x, y, z);
}
void glRasterPos3sv(const GLshort *v) {
    GlProfileScope _gp(921);
    if (!tglmt::GateCheck(682)) return;
    gl::glRasterPos3sv(v);
}
void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(922);
    if (!tglmt::GateCheck(683)) return;
    gl::glRasterPos4d(x, y, z, w);
}
void glRasterPos4dv(const GLdouble *v) {
    GlProfileScope _gp(923);
    if (!tglmt::GateCheck(684)) return;
    gl::glRasterPos4dv(v);
}
void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GlProfileScope _gp(924);
    if (!tglmt::GateCheck(685)) return;
    gl::glRasterPos4f(x, y, z, w);
}
void glRasterPos4fv(const GLfloat *v) {
    GlProfileScope _gp(925);
    if (!tglmt::GateCheck(686)) return;
    gl::glRasterPos4fv(v);
}
void glRasterPos4i(GLint x, GLint y, GLint z, GLint w) {
    GlProfileScope _gp(926);
    if (!tglmt::GateCheck(687)) return;
    gl::glRasterPos4i(x, y, z, w);
}
void glRasterPos4iv(const GLint *v) {
    GlProfileScope _gp(927);
    if (!tglmt::GateCheck(688)) return;
    gl::glRasterPos4iv(v);
}
void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    GlProfileScope _gp(928);
    if (!tglmt::GateCheck(689)) return;
    gl::glRasterPos4s(x, y, z, w);
}
void glRasterPos4sv(const GLshort *v) {
    GlProfileScope _gp(929);
    if (!tglmt::GateCheck(690)) return;
    gl::glRasterPos4sv(v);
}
void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
    GlProfileScope _gp(930);
    if (!tglmt::GateCheck(694)) return;
    gl::glRectd(x1, y1, x2, y2);
}
void glRectdv(const GLdouble *v1, const GLdouble *v2) {
    GlProfileScope _gp(931);
    if (!tglmt::GateCheck(695)) return;
    gl::glRectdv(v1, v2);
}
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
    GlProfileScope _gp(932);
    if (!tglmt::GateCheck(696)) return;
    gl::glRectf(x1, y1, x2, y2);
}
void glRectfv(const GLfloat *v1, const GLfloat *v2) {
    GlProfileScope _gp(933);
    if (!tglmt::GateCheck(697)) return;
    gl::glRectfv(v1, v2);
}
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
    GlProfileScope _gp(934);
    if (!tglmt::GateCheck(698)) return;
    gl::glRecti(x1, y1, x2, y2);
}
void glRectiv(const GLint *v1, const GLint *v2) {
    GlProfileScope _gp(935);
    if (!tglmt::GateCheck(699)) return;
    gl::glRectiv(v1, v2);
}
void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
    GlProfileScope _gp(936);
    if (!tglmt::GateCheck(700)) return;
    gl::glRects(x1, y1, x2, y2);
}
void glRectsv(const GLshort *v1, const GLshort *v2) {
    GlProfileScope _gp(937);
    if (!tglmt::GateCheck(701)) return;
    gl::glRectsv(v1, v2);
}
GLint glRenderMode(GLenum mode) {
    GlProfileScope _gp(938);
    if (!tglmt::GateCheck(703)) return 0;
    return gl::glRenderMode(mode);
}
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(939);
    if (!tglmt::GateCheck(707)) return;
    gl::glRotated(angle, x, y, z);
}
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(940);
    if (!tglmt::GateCheck(708)) return;
    gl::glRotatef(angle, x, y, z);
}
void glScaled(GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(941);
    if (!tglmt::GateCheck(717)) return;
    gl::glScaled(x, y, z);
}
void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(942);
    if (!tglmt::GateCheck(718)) return;
    gl::glScalef(x, y, z);
}
void glSecondaryColor3b(GLbyte red, GLbyte green, GLbyte blue) {
    GlProfileScope _gp(943);
    if (!tglmt::GateCheck(723)) return;
    gl::glSecondaryColor3b(red, green, blue);
}
void glSecondaryColor3bv(const GLbyte *v) {
    GlProfileScope _gp(944);
    if (!tglmt::GateCheck(724)) return;
    gl::glSecondaryColor3bv(v);
}
void glSecondaryColor3d(GLdouble red, GLdouble green, GLdouble blue) {
    GlProfileScope _gp(945);
    if (!tglmt::GateCheck(725)) return;
    gl::glSecondaryColor3d(red, green, blue);
}
void glSecondaryColor3dv(const GLdouble *v) {
    GlProfileScope _gp(946);
    if (!tglmt::GateCheck(726)) return;
    gl::glSecondaryColor3dv(v);
}
void glSecondaryColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    GlProfileScope _gp(947);
    if (!tglmt::GateCheck(727)) return;
    gl::glSecondaryColor3f(red, green, blue);
}
void glSecondaryColor3fv(const GLfloat *v) {
    GlProfileScope _gp(948);
    if (!tglmt::GateCheck(728)) return;
    gl::glSecondaryColor3fv(v);
}
void glSecondaryColor3i(GLint red, GLint green, GLint blue) {
    GlProfileScope _gp(949);
    if (!tglmt::GateCheck(729)) return;
    gl::glSecondaryColor3i(red, green, blue);
}
void glSecondaryColor3iv(const GLint *v) {
    GlProfileScope _gp(950);
    if (!tglmt::GateCheck(730)) return;
    gl::glSecondaryColor3iv(v);
}
void glSecondaryColor3s(GLshort red, GLshort green, GLshort blue) {
    GlProfileScope _gp(951);
    if (!tglmt::GateCheck(731)) return;
    gl::glSecondaryColor3s(red, green, blue);
}
void glSecondaryColor3sv(const GLshort *v) {
    GlProfileScope _gp(952);
    if (!tglmt::GateCheck(732)) return;
    gl::glSecondaryColor3sv(v);
}
void glSecondaryColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    GlProfileScope _gp(953);
    if (!tglmt::GateCheck(733)) return;
    gl::glSecondaryColor3ub(red, green, blue);
}
void glSecondaryColor3ubv(const GLubyte *v) {
    GlProfileScope _gp(954);
    if (!tglmt::GateCheck(734)) return;
    gl::glSecondaryColor3ubv(v);
}
void glSecondaryColor3ui(GLuint red, GLuint green, GLuint blue) {
    GlProfileScope _gp(955);
    if (!tglmt::GateCheck(735)) return;
    gl::glSecondaryColor3ui(red, green, blue);
}
void glSecondaryColor3uiv(const GLuint *v) {
    GlProfileScope _gp(956);
    if (!tglmt::GateCheck(736)) return;
    gl::glSecondaryColor3uiv(v);
}
void glSecondaryColor3us(GLushort red, GLushort green, GLushort blue) {
    GlProfileScope _gp(957);
    if (!tglmt::GateCheck(737)) return;
    gl::glSecondaryColor3us(red, green, blue);
}
void glSecondaryColor3usv(const GLushort *v) {
    GlProfileScope _gp(958);
    if (!tglmt::GateCheck(738)) return;
    gl::glSecondaryColor3usv(v);
}
void glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(959);
    if (!tglmt::GateCheck(741)) return;
    gl::glSecondaryColorPointer(size, type, stride, pointer);
}
void glSelectBuffer(GLsizei size, GLuint *buffer) {
    GlProfileScope _gp(960);
    if (!tglmt::GateCheck(742)) return;
    gl::glSelectBuffer(size, buffer);
}
void glShadeModel(GLenum mode) {
    GlProfileScope _gp(961);
    if (!tglmt::GateCheck(743)) return;
    gl::glShadeModel(mode);
}
void glTexCoord1d(GLdouble s) {
    GlProfileScope _gp(962);
    if (!tglmt::GateCheck(756)) return;
    gl::glTexCoord1d(s);
}
void glTexCoord1dv(const GLdouble *v) {
    GlProfileScope _gp(963);
    if (!tglmt::GateCheck(757)) return;
    gl::glTexCoord1dv(v);
}
void glTexCoord1f(GLfloat s) {
    GlProfileScope _gp(964);
    if (!tglmt::GateCheck(758)) return;
    gl::glTexCoord1f(s);
}
void glTexCoord1fv(const GLfloat *v) {
    GlProfileScope _gp(965);
    if (!tglmt::GateCheck(759)) return;
    gl::glTexCoord1fv(v);
}
void glTexCoord1i(GLint s) {
    GlProfileScope _gp(966);
    if (!tglmt::GateCheck(760)) return;
    gl::glTexCoord1i(s);
}
void glTexCoord1iv(const GLint *v) {
    GlProfileScope _gp(967);
    if (!tglmt::GateCheck(761)) return;
    gl::glTexCoord1iv(v);
}
void glTexCoord1s(GLshort s) {
    GlProfileScope _gp(968);
    if (!tglmt::GateCheck(762)) return;
    gl::glTexCoord1s(s);
}
void glTexCoord1sv(const GLshort *v) {
    GlProfileScope _gp(969);
    if (!tglmt::GateCheck(763)) return;
    gl::glTexCoord1sv(v);
}
void glTexCoord2d(GLdouble s, GLdouble t) {
    GlProfileScope _gp(970);
    if (!tglmt::GateCheck(764)) return;
    gl::glTexCoord2d(s, t);
}
void glTexCoord2dv(const GLdouble *v) {
    GlProfileScope _gp(971);
    if (!tglmt::GateCheck(765)) return;
    gl::glTexCoord2dv(v);
}
void glTexCoord2f(GLfloat s, GLfloat t) {
    GlProfileScope _gp(972);
    if (!tglmt::GateCheck(766)) return;
    gl::glTexCoord2f(s, t);
}
void glTexCoord2fv(const GLfloat *v) {
    GlProfileScope _gp(973);
    if (!tglmt::GateCheck(767)) return;
    gl::glTexCoord2fv(v);
}
void glTexCoord2i(GLint s, GLint t) {
    GlProfileScope _gp(974);
    if (!tglmt::GateCheck(768)) return;
    gl::glTexCoord2i(s, t);
}
void glTexCoord2iv(const GLint *v) {
    GlProfileScope _gp(975);
    if (!tglmt::GateCheck(769)) return;
    gl::glTexCoord2iv(v);
}
void glTexCoord2s(GLshort s, GLshort t) {
    GlProfileScope _gp(976);
    if (!tglmt::GateCheck(770)) return;
    gl::glTexCoord2s(s, t);
}
void glTexCoord2sv(const GLshort *v) {
    GlProfileScope _gp(977);
    if (!tglmt::GateCheck(771)) return;
    gl::glTexCoord2sv(v);
}
void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) {
    GlProfileScope _gp(978);
    if (!tglmt::GateCheck(772)) return;
    gl::glTexCoord3d(s, t, r);
}
void glTexCoord3dv(const GLdouble *v) {
    GlProfileScope _gp(979);
    if (!tglmt::GateCheck(773)) return;
    gl::glTexCoord3dv(v);
}
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
    GlProfileScope _gp(980);
    if (!tglmt::GateCheck(774)) return;
    gl::glTexCoord3f(s, t, r);
}
void glTexCoord3fv(const GLfloat *v) {
    GlProfileScope _gp(981);
    if (!tglmt::GateCheck(775)) return;
    gl::glTexCoord3fv(v);
}
void glTexCoord3i(GLint s, GLint t, GLint r) {
    GlProfileScope _gp(982);
    if (!tglmt::GateCheck(776)) return;
    gl::glTexCoord3i(s, t, r);
}
void glTexCoord3iv(const GLint *v) {
    GlProfileScope _gp(983);
    if (!tglmt::GateCheck(777)) return;
    gl::glTexCoord3iv(v);
}
void glTexCoord3s(GLshort s, GLshort t, GLshort r) {
    GlProfileScope _gp(984);
    if (!tglmt::GateCheck(778)) return;
    gl::glTexCoord3s(s, t, r);
}
void glTexCoord3sv(const GLshort *v) {
    GlProfileScope _gp(985);
    if (!tglmt::GateCheck(779)) return;
    gl::glTexCoord3sv(v);
}
void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
    GlProfileScope _gp(986);
    if (!tglmt::GateCheck(780)) return;
    gl::glTexCoord4d(s, t, r, q);
}
void glTexCoord4dv(const GLdouble *v) {
    GlProfileScope _gp(987);
    if (!tglmt::GateCheck(781)) return;
    gl::glTexCoord4dv(v);
}
void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    GlProfileScope _gp(988);
    if (!tglmt::GateCheck(782)) return;
    gl::glTexCoord4f(s, t, r, q);
}
void glTexCoord4fv(const GLfloat *v) {
    GlProfileScope _gp(989);
    if (!tglmt::GateCheck(783)) return;
    gl::glTexCoord4fv(v);
}
void glTexCoord4i(GLint s, GLint t, GLint r, GLint q) {
    GlProfileScope _gp(990);
    if (!tglmt::GateCheck(784)) return;
    gl::glTexCoord4i(s, t, r, q);
}
void glTexCoord4iv(const GLint *v) {
    GlProfileScope _gp(991);
    if (!tglmt::GateCheck(785)) return;
    gl::glTexCoord4iv(v);
}
void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) {
    GlProfileScope _gp(992);
    if (!tglmt::GateCheck(786)) return;
    gl::glTexCoord4s(s, t, r, q);
}
void glTexCoord4sv(const GLshort *v) {
    GlProfileScope _gp(993);
    if (!tglmt::GateCheck(787)) return;
    gl::glTexCoord4sv(v);
}
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(994);
    if (!tglmt::GateCheck(796)) return;
    gl::glTexCoordPointer(size, type, stride, pointer);
}
void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    GlProfileScope _gp(995);
    if (!tglmt::GateCheck(797)) return;
    gl::glTexEnvf(target, pname, param);
}
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(996);
    if (!tglmt::GateCheck(798)) return;
    gl::glTexEnvfv(target, pname, params);
}
void glTexEnvi(GLenum target, GLenum pname, GLint param) {
    GlProfileScope _gp(997);
    if (!tglmt::GateCheck(799)) return;
    gl::glTexEnvi(target, pname, param);
}
void glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
    GlProfileScope _gp(998);
    if (!tglmt::GateCheck(800)) return;
    gl::glTexEnviv(target, pname, params);
}
void glTexGend(GLenum coord, GLenum pname, GLdouble param) {
    GlProfileScope _gp(999);
    if (!tglmt::GateCheck(801)) return;
    gl::glTexGend(coord, pname, param);
}
void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params) {
    GlProfileScope _gp(1000);
    if (!tglmt::GateCheck(802)) return;
    gl::glTexGendv(coord, pname, params);
}
void glTexGenf(GLenum coord, GLenum pname, GLfloat param) {
    GlProfileScope _gp(1001);
    if (!tglmt::GateCheck(803)) return;
    gl::glTexGenf(coord, pname, param);
}
void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(1002);
    if (!tglmt::GateCheck(804)) return;
    gl::glTexGenfv(coord, pname, params);
}
void glTexGeni(GLenum coord, GLenum pname, GLint param) {
    GlProfileScope _gp(1003);
    if (!tglmt::GateCheck(805)) return;
    gl::glTexGeni(coord, pname, param);
}
void glTexGeniv(GLenum coord, GLenum pname, const GLint *params) {
    GlProfileScope _gp(1004);
    if (!tglmt::GateCheck(806)) return;
    gl::glTexGeniv(coord, pname, params);
}
void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(1005);
    if (!tglmt::GateCheck(847)) return;
    gl::glTranslated(x, y, z);
}
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(1006);
    if (!tglmt::GateCheck(848)) return;
    gl::glTranslatef(x, y, z);
}
void glVertex2d(GLdouble x, GLdouble y) {
    GlProfileScope _gp(1007);
    if (!tglmt::GateCheck(907)) return;
    gl::glVertex2d(x, y);
}
void glVertex2dv(const GLdouble *v) {
    GlProfileScope _gp(1008);
    if (!tglmt::GateCheck(908)) return;
    gl::glVertex2dv(v);
}
void glVertex2f(GLfloat x, GLfloat y) {
    GlProfileScope _gp(1009);
    if (!tglmt::GateCheck(909)) return;
    gl::glVertex2f(x, y);
}
void glVertex2fv(const GLfloat *v) {
    GlProfileScope _gp(1010);
    if (!tglmt::GateCheck(910)) return;
    gl::glVertex2fv(v);
}
void glVertex2i(GLint x, GLint y) {
    GlProfileScope _gp(1011);
    if (!tglmt::GateCheck(911)) return;
    gl::glVertex2i(x, y);
}
void glVertex2iv(const GLint *v) {
    GlProfileScope _gp(1012);
    if (!tglmt::GateCheck(912)) return;
    gl::glVertex2iv(v);
}
void glVertex2s(GLshort x, GLshort y) {
    GlProfileScope _gp(1013);
    if (!tglmt::GateCheck(913)) return;
    gl::glVertex2s(x, y);
}
void glVertex2sv(const GLshort *v) {
    GlProfileScope _gp(1014);
    if (!tglmt::GateCheck(914)) return;
    gl::glVertex2sv(v);
}
void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(1015);
    if (!tglmt::GateCheck(915)) return;
    gl::glVertex3d(x, y, z);
}
void glVertex3dv(const GLdouble *v) {
    GlProfileScope _gp(1016);
    if (!tglmt::GateCheck(916)) return;
    gl::glVertex3dv(v);
}
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(1017);
    if (!tglmt::GateCheck(917)) return;
    gl::glVertex3f(x, y, z);
}
void glVertex3fv(const GLfloat *v) {
    GlProfileScope _gp(1018);
    if (!tglmt::GateCheck(918)) return;
    gl::glVertex3fv(v);
}
void glVertex3i(GLint x, GLint y, GLint z) {
    GlProfileScope _gp(1019);
    if (!tglmt::GateCheck(919)) return;
    gl::glVertex3i(x, y, z);
}
void glVertex3iv(const GLint *v) {
    GlProfileScope _gp(1020);
    if (!tglmt::GateCheck(920)) return;
    gl::glVertex3iv(v);
}
void glVertex3s(GLshort x, GLshort y, GLshort z) {
    GlProfileScope _gp(1021);
    if (!tglmt::GateCheck(921)) return;
    gl::glVertex3s(x, y, z);
}
void glVertex3sv(const GLshort *v) {
    GlProfileScope _gp(1022);
    if (!tglmt::GateCheck(922)) return;
    gl::glVertex3sv(v);
}
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(1023);
    if (!tglmt::GateCheck(923)) return;
    gl::glVertex4d(x, y, z, w);
}
void glVertex4dv(const GLdouble *v) {
    GlProfileScope _gp(1024);
    if (!tglmt::GateCheck(924)) return;
    gl::glVertex4dv(v);
}
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GlProfileScope _gp(1025);
    if (!tglmt::GateCheck(925)) return;
    gl::glVertex4f(x, y, z, w);
}
void glVertex4fv(const GLfloat *v) {
    GlProfileScope _gp(1026);
    if (!tglmt::GateCheck(926)) return;
    gl::glVertex4fv(v);
}
void glVertex4i(GLint x, GLint y, GLint z, GLint w) {
    GlProfileScope _gp(1027);
    if (!tglmt::GateCheck(927)) return;
    gl::glVertex4i(x, y, z, w);
}
void glVertex4iv(const GLint *v) {
    GlProfileScope _gp(1028);
    if (!tglmt::GateCheck(928)) return;
    gl::glVertex4iv(v);
}
void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    GlProfileScope _gp(1029);
    if (!tglmt::GateCheck(929)) return;
    gl::glVertex4s(x, y, z, w);
}
void glVertex4sv(const GLshort *v) {
    GlProfileScope _gp(1030);
    if (!tglmt::GateCheck(930)) return;
    gl::glVertex4sv(v);
}
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(1031);
    if (!tglmt::GateCheck(1026)) return;
    gl::glVertexPointer(size, type, stride, pointer);
}
void glWindowPos2d(GLdouble x, GLdouble y) {
    GlProfileScope _gp(1032);
    if (!tglmt::GateCheck(1032)) return;
    gl::glWindowPos2d(x, y);
}
void glWindowPos2dv(const GLdouble *v) {
    GlProfileScope _gp(1033);
    if (!tglmt::GateCheck(1033)) return;
    gl::glWindowPos2dv(v);
}
void glWindowPos2f(GLfloat x, GLfloat y) {
    GlProfileScope _gp(1034);
    if (!tglmt::GateCheck(1034)) return;
    gl::glWindowPos2f(x, y);
}
void glWindowPos2fv(const GLfloat *v) {
    GlProfileScope _gp(1035);
    if (!tglmt::GateCheck(1035)) return;
    gl::glWindowPos2fv(v);
}
void glWindowPos2i(GLint x, GLint y) {
    GlProfileScope _gp(1036);
    if (!tglmt::GateCheck(1036)) return;
    gl::glWindowPos2i(x, y);
}
void glWindowPos2iv(const GLint *v) {
    GlProfileScope _gp(1037);
    if (!tglmt::GateCheck(1037)) return;
    gl::glWindowPos2iv(v);
}
void glWindowPos2s(GLshort x, GLshort y) {
    GlProfileScope _gp(1038);
    if (!tglmt::GateCheck(1038)) return;
    gl::glWindowPos2s(x, y);
}
void glWindowPos2sv(const GLshort *v) {
    GlProfileScope _gp(1039);
    if (!tglmt::GateCheck(1039)) return;
    gl::glWindowPos2sv(v);
}
void glWindowPos3d(GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(1040);
    if (!tglmt::GateCheck(1040)) return;
    gl::glWindowPos3d(x, y, z);
}
void glWindowPos3dv(const GLdouble *v) {
    GlProfileScope _gp(1041);
    if (!tglmt::GateCheck(1041)) return;
    gl::glWindowPos3dv(v);
}
void glWindowPos3f(GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(1042);
    if (!tglmt::GateCheck(1042)) return;
    gl::glWindowPos3f(x, y, z);
}
void glWindowPos3fv(const GLfloat *v) {
    GlProfileScope _gp(1043);
    if (!tglmt::GateCheck(1043)) return;
    gl::glWindowPos3fv(v);
}
void glWindowPos3i(GLint x, GLint y, GLint z) {
    GlProfileScope _gp(1044);
    if (!tglmt::GateCheck(1044)) return;
    gl::glWindowPos3i(x, y, z);
}
void glWindowPos3iv(const GLint *v) {
    GlProfileScope _gp(1045);
    if (!tglmt::GateCheck(1045)) return;
    gl::glWindowPos3iv(v);
}
void glWindowPos3s(GLshort x, GLshort y, GLshort z) {
    GlProfileScope _gp(1046);
    if (!tglmt::GateCheck(1046)) return;
    gl::glWindowPos3s(x, y, z);
}
void glWindowPos3sv(const GLshort *v) {
    GlProfileScope _gp(1047);
    if (!tglmt::GateCheck(1047)) return;
    gl::glWindowPos3sv(v);
}
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) {
    GlProfileScope _gp(1048);
    gl::glDepthRangeArrayfvNV(a, b, c_);
}

#ifdef __cplusplus
} // extern "C"
#endif

namespace tglmt {
GlCallStat gGlProfile[kGlProfileMax];
const char* const gGlProfileName[kGlProfileMax] = {
    "glActiveShaderProgram", // 0
    "glActiveTexture", // 1
    "glAttachShader", // 2
    "glBeginConditionalRender", // 3
    "glBeginQuery", // 4
    "glBeginQueryIndexed", // 5
    "glBeginTransformFeedback", // 6
    "glBindAttribLocation", // 7
    "glBindBuffer", // 8
    "glBindBufferBase", // 9
    "glBindBufferRange", // 10
    "glBindBuffersBase", // 11
    "glBindBuffersRange", // 12
    "glBindFragDataLocation", // 13
    "glBindFragDataLocationIndexed", // 14
    "glBindFramebuffer", // 15
    "glBindImageTexture", // 16
    "glBindImageTextures", // 17
    "glBindProgramPipeline", // 18
    "glBindRenderbuffer", // 19
    "glBindSampler", // 20
    "glBindSamplers", // 21
    "glBindTexture", // 22
    "glBindTextureUnit", // 23
    "glBindTextures", // 24
    "glBindTransformFeedback", // 25
    "glBindVertexArray", // 26
    "glBindVertexBuffer", // 27
    "glBindVertexBuffers", // 28
    "glBlendColor", // 29
    "glBlendEquation", // 30
    "glBlendEquationSeparate", // 31
    "glBlendEquationSeparatei", // 32
    "glBlendEquationi", // 33
    "glBlendFunc", // 34
    "glBlendFuncSeparate", // 35
    "glBlendFuncSeparatei", // 36
    "glBlendFunci", // 37
    "glBlitFramebuffer", // 38
    "glBlitNamedFramebuffer", // 39
    "glBufferData", // 40
    "glBufferStorage", // 41
    "glBufferSubData", // 42
    "glCheckFramebufferStatus", // 43
    "glCheckNamedFramebufferStatus", // 44
    "glClampColor", // 45
    "glClear", // 46
    "glClearBufferData", // 47
    "glClearBufferSubData", // 48
    "glClearBufferfi", // 49
    "glClearBufferfv", // 50
    "glClearBufferiv", // 51
    "glClearBufferuiv", // 52
    "glClearColor", // 53
    "glClearDepth", // 54
    "glClearDepthf", // 55
    "glClearNamedBufferData", // 56
    "glClearNamedBufferSubData", // 57
    "glClearNamedFramebufferfi", // 58
    "glClearNamedFramebufferfv", // 59
    "glClearNamedFramebufferiv", // 60
    "glClearNamedFramebufferuiv", // 61
    "glClearStencil", // 62
    "glClearTexImage", // 63
    "glClearTexSubImage", // 64
    "glClientWaitSync", // 65
    "glClipControl", // 66
    "glColorMask", // 67
    "glColorMaski", // 68
    "glCompileShader", // 69
    "glCompressedTexImage1D", // 70
    "glCompressedTexImage2D", // 71
    "glCompressedTexImage3D", // 72
    "glCompressedTexSubImage1D", // 73
    "glCompressedTexSubImage2D", // 74
    "glCompressedTexSubImage3D", // 75
    "glCompressedTextureSubImage1D", // 76
    "glCompressedTextureSubImage2D", // 77
    "glCompressedTextureSubImage3D", // 78
    "glCopyBufferSubData", // 79
    "glCopyImageSubData", // 80
    "glCopyNamedBufferSubData", // 81
    "glCopyTexImage1D", // 82
    "glCopyTexImage2D", // 83
    "glCopyTexSubImage1D", // 84
    "glCopyTexSubImage2D", // 85
    "glCopyTexSubImage3D", // 86
    "glCopyTextureSubImage1D", // 87
    "glCopyTextureSubImage2D", // 88
    "glCopyTextureSubImage3D", // 89
    "glCreateBuffers", // 90
    "glCreateFramebuffers", // 91
    "glCreateProgram", // 92
    "glCreateProgramPipelines", // 93
    "glCreateQueries", // 94
    "glCreateRenderbuffers", // 95
    "glCreateSamplers", // 96
    "glCreateShader", // 97
    "glCreateShaderProgramv", // 98
    "glCreateTextures", // 99
    "glCreateTransformFeedbacks", // 100
    "glCreateVertexArrays", // 101
    "glCullFace", // 102
    "glDebugMessageCallback", // 103
    "glDebugMessageControl", // 104
    "glDebugMessageInsert", // 105
    "glDeleteBuffers", // 106
    "glDeleteFramebuffers", // 107
    "glDeleteProgram", // 108
    "glDeleteProgramPipelines", // 109
    "glDeleteQueries", // 110
    "glDeleteRenderbuffers", // 111
    "glDeleteSamplers", // 112
    "glDeleteShader", // 113
    "glDeleteSync", // 114
    "glDeleteTextures", // 115
    "glDeleteTransformFeedbacks", // 116
    "glDeleteVertexArrays", // 117
    "glDepthFunc", // 118
    "glDepthMask", // 119
    "glDepthRange", // 120
    "glDepthRangeArrayv", // 121
    "glDepthRangeIndexed", // 122
    "glDepthRangef", // 123
    "glDetachShader", // 124
    "glDisable", // 125
    "glDisableVertexArrayAttrib", // 126
    "glDisableVertexAttribArray", // 127
    "glDisablei", // 128
    "glDispatchCompute", // 129
    "glDispatchComputeIndirect", // 130
    "glDrawArrays", // 131
    "glDrawArraysIndirect", // 132
    "glDrawArraysInstanced", // 133
    "glDrawArraysInstancedBaseInstance", // 134
    "glDrawBuffer", // 135
    "glDrawBuffers", // 136
    "glDrawElements", // 137
    "glDrawElementsBaseVertex", // 138
    "glDrawElementsIndirect", // 139
    "glDrawElementsInstanced", // 140
    "glDrawElementsInstancedBaseInstance", // 141
    "glDrawElementsInstancedBaseVertex", // 142
    "glDrawElementsInstancedBaseVertexBaseInstance", // 143
    "glDrawRangeElements", // 144
    "glDrawRangeElementsBaseVertex", // 145
    "glDrawTransformFeedback", // 146
    "glDrawTransformFeedbackInstanced", // 147
    "glDrawTransformFeedbackStream", // 148
    "glDrawTransformFeedbackStreamInstanced", // 149
    "glEnable", // 150
    "glEnableVertexArrayAttrib", // 151
    "glEnableVertexAttribArray", // 152
    "glEnablei", // 153
    "glEndConditionalRender", // 154
    "glEndQuery", // 155
    "glEndQueryIndexed", // 156
    "glEndTransformFeedback", // 157
    "glFenceSync", // 158
    "glFinish", // 159
    "glFlush", // 160
    "glFlushMappedBufferRange", // 161
    "glFlushMappedNamedBufferRange", // 162
    "glFramebufferParameteri", // 163
    "glFramebufferRenderbuffer", // 164
    "glFramebufferTexture", // 165
    "glFramebufferTexture1D", // 166
    "glFramebufferTexture2D", // 167
    "glFramebufferTexture3D", // 168
    "glFramebufferTextureLayer", // 169
    "glFrontFace", // 170
    "glGenBuffers", // 171
    "glGenFramebuffers", // 172
    "glGenProgramPipelines", // 173
    "glGenQueries", // 174
    "glGenRenderbuffers", // 175
    "glGenSamplers", // 176
    "glGenTextures", // 177
    "glGenTransformFeedbacks", // 178
    "glGenVertexArrays", // 179
    "glGenerateMipmap", // 180
    "glGenerateTextureMipmap", // 181
    "glGetActiveAtomicCounterBufferiv", // 182
    "glGetActiveAttrib", // 183
    "glGetActiveSubroutineName", // 184
    "glGetActiveSubroutineUniformName", // 185
    "glGetActiveSubroutineUniformiv", // 186
    "glGetActiveUniform", // 187
    "glGetActiveUniformBlockName", // 188
    "glGetActiveUniformBlockiv", // 189
    "glGetActiveUniformName", // 190
    "glGetActiveUniformsiv", // 191
    "glGetAttachedShaders", // 192
    "glGetAttribLocation", // 193
    "glGetBooleani_v", // 194
    "glGetBooleanv", // 195
    "glGetBufferParameteri64v", // 196
    "glGetBufferParameteriv", // 197
    "glGetBufferPointerv", // 198
    "glGetBufferSubData", // 199
    "glGetCompressedTexImage", // 200
    "glGetCompressedTextureImage", // 201
    "glGetCompressedTextureSubImage", // 202
    "glGetDebugMessageLog", // 203
    "glGetDoublei_v", // 204
    "glGetDoublev", // 205
    "glGetError", // 206
    "glGetFloati_v", // 207
    "glGetFloatv", // 208
    "glGetFragDataIndex", // 209
    "glGetFragDataLocation", // 210
    "glGetFramebufferAttachmentParameteriv", // 211
    "glGetFramebufferParameteriv", // 212
    "glGetGraphicsResetStatus", // 213
    "glGetInteger64i_v", // 214
    "glGetInteger64v", // 215
    "glGetIntegeri_v", // 216
    "glGetIntegerv", // 217
    "glGetInternalformati64v", // 218
    "glGetInternalformativ", // 219
    "glGetMultisamplefv", // 220
    "glGetNamedBufferParameteri64v", // 221
    "glGetNamedBufferParameteriv", // 222
    "glGetNamedBufferPointerv", // 223
    "glGetNamedBufferSubData", // 224
    "glGetNamedFramebufferAttachmentParameteriv", // 225
    "glGetNamedFramebufferParameteriv", // 226
    "glGetNamedRenderbufferParameteriv", // 227
    "glGetObjectLabel", // 228
    "glGetObjectPtrLabel", // 229
    "glGetPointerv", // 230
    "glGetProgramBinary", // 231
    "glGetProgramInfoLog", // 232
    "glGetProgramInterfaceiv", // 233
    "glGetProgramPipelineInfoLog", // 234
    "glGetProgramPipelineiv", // 235
    "glGetProgramResourceIndex", // 236
    "glGetProgramResourceLocation", // 237
    "glGetProgramResourceLocationIndex", // 238
    "glGetProgramResourceName", // 239
    "glGetProgramResourceiv", // 240
    "glGetProgramStageiv", // 241
    "glGetProgramiv", // 242
    "glGetQueryBufferObjecti64v", // 243
    "glGetQueryBufferObjectiv", // 244
    "glGetQueryBufferObjectui64v", // 245
    "glGetQueryBufferObjectuiv", // 246
    "glGetQueryIndexediv", // 247
    "glGetQueryObjecti64v", // 248
    "glGetQueryObjectiv", // 249
    "glGetQueryObjectui64v", // 250
    "glGetQueryObjectuiv", // 251
    "glGetQueryiv", // 252
    "glGetRenderbufferParameteriv", // 253
    "glGetSamplerParameterIiv", // 254
    "glGetSamplerParameterIuiv", // 255
    "glGetSamplerParameterfv", // 256
    "glGetSamplerParameteriv", // 257
    "glGetShaderInfoLog", // 258
    "glGetShaderPrecisionFormat", // 259
    "glGetShaderSource", // 260
    "glGetShaderiv", // 261
    "glGetString", // 262
    "glGetStringi", // 263
    "glGetSubroutineIndex", // 264
    "glGetSubroutineUniformLocation", // 265
    "glGetSynciv", // 266
    "glGetTexImage", // 267
    "glGetTexLevelParameterfv", // 268
    "glGetTexLevelParameteriv", // 269
    "glGetTexParameterIiv", // 270
    "glGetTexParameterIuiv", // 271
    "glGetTexParameterfv", // 272
    "glGetTexParameteriv", // 273
    "glGetTextureImage", // 274
    "glGetTextureLevelParameterfv", // 275
    "glGetTextureLevelParameteriv", // 276
    "glGetTextureParameterIiv", // 277
    "glGetTextureParameterIuiv", // 278
    "glGetTextureParameterfv", // 279
    "glGetTextureParameteriv", // 280
    "glGetTextureSubImage", // 281
    "glGetTransformFeedbackVarying", // 282
    "glGetTransformFeedbacki64_v", // 283
    "glGetTransformFeedbacki_v", // 284
    "glGetTransformFeedbackiv", // 285
    "glGetUniformBlockIndex", // 286
    "glGetUniformIndices", // 287
    "glGetUniformLocation", // 288
    "glGetUniformSubroutineuiv", // 289
    "glGetUniformdv", // 290
    "glGetUniformfv", // 291
    "glGetUniformiv", // 292
    "glGetUniformuiv", // 293
    "glGetVertexArrayIndexed64iv", // 294
    "glGetVertexArrayIndexediv", // 295
    "glGetVertexArrayiv", // 296
    "glGetVertexAttribIiv", // 297
    "glGetVertexAttribIuiv", // 298
    "glGetVertexAttribLdv", // 299
    "glGetVertexAttribPointerv", // 300
    "glGetVertexAttribdv", // 301
    "glGetVertexAttribfv", // 302
    "glGetVertexAttribiv", // 303
    "glGetnCompressedTexImage", // 304
    "glGetnTexImage", // 305
    "glGetnUniformdv", // 306
    "glGetnUniformfv", // 307
    "glGetnUniformiv", // 308
    "glGetnUniformuiv", // 309
    "glHint", // 310
    "glInvalidateBufferData", // 311
    "glInvalidateBufferSubData", // 312
    "glInvalidateFramebuffer", // 313
    "glInvalidateNamedFramebufferData", // 314
    "glInvalidateNamedFramebufferSubData", // 315
    "glInvalidateSubFramebuffer", // 316
    "glInvalidateTexImage", // 317
    "glInvalidateTexSubImage", // 318
    "glIsBuffer", // 319
    "glIsEnabled", // 320
    "glIsEnabledi", // 321
    "glIsFramebuffer", // 322
    "glIsProgram", // 323
    "glIsProgramPipeline", // 324
    "glIsQuery", // 325
    "glIsRenderbuffer", // 326
    "glIsSampler", // 327
    "glIsShader", // 328
    "glIsSync", // 329
    "glIsTexture", // 330
    "glIsTransformFeedback", // 331
    "glIsVertexArray", // 332
    "glLineWidth", // 333
    "glLinkProgram", // 334
    "glLogicOp", // 335
    "glMapBuffer", // 336
    "glMapBufferRange", // 337
    "glMapNamedBuffer", // 338
    "glMapNamedBufferRange", // 339
    "glMemoryBarrier", // 340
    "glMemoryBarrierByRegion", // 341
    "glMinSampleShading", // 342
    "glMultiDrawArrays", // 343
    "glMultiDrawArraysIndirect", // 344
    "glMultiDrawArraysIndirectCount", // 345
    "glMultiDrawElements", // 346
    "glMultiDrawElementsBaseVertex", // 347
    "glMultiDrawElementsIndirect", // 348
    "glMultiDrawElementsIndirectCount", // 349
    "glNamedBufferData", // 350
    "glNamedBufferStorage", // 351
    "glNamedBufferSubData", // 352
    "glNamedFramebufferDrawBuffer", // 353
    "glNamedFramebufferDrawBuffers", // 354
    "glNamedFramebufferParameteri", // 355
    "glNamedFramebufferReadBuffer", // 356
    "glNamedFramebufferRenderbuffer", // 357
    "glNamedFramebufferTexture", // 358
    "glNamedFramebufferTextureLayer", // 359
    "glNamedRenderbufferStorage", // 360
    "glNamedRenderbufferStorageMultisample", // 361
    "glObjectLabel", // 362
    "glObjectPtrLabel", // 363
    "glPatchParameterfv", // 364
    "glPatchParameteri", // 365
    "glPauseTransformFeedback", // 366
    "glPixelStoref", // 367
    "glPixelStorei", // 368
    "glPointParameterf", // 369
    "glPointParameterfv", // 370
    "glPointParameteri", // 371
    "glPointParameteriv", // 372
    "glPointSize", // 373
    "glPolygonMode", // 374
    "glPolygonOffset", // 375
    "glPolygonOffsetClamp", // 376
    "glPopDebugGroup", // 377
    "glPrimitiveRestartIndex", // 378
    "glProgramBinary", // 379
    "glProgramParameteri", // 380
    "glProgramUniform1d", // 381
    "glProgramUniform1dv", // 382
    "glProgramUniform1f", // 383
    "glProgramUniform1fv", // 384
    "glProgramUniform1i", // 385
    "glProgramUniform1iv", // 386
    "glProgramUniform1ui", // 387
    "glProgramUniform1uiv", // 388
    "glProgramUniform2d", // 389
    "glProgramUniform2dv", // 390
    "glProgramUniform2f", // 391
    "glProgramUniform2fv", // 392
    "glProgramUniform2i", // 393
    "glProgramUniform2iv", // 394
    "glProgramUniform2ui", // 395
    "glProgramUniform2uiv", // 396
    "glProgramUniform3d", // 397
    "glProgramUniform3dv", // 398
    "glProgramUniform3f", // 399
    "glProgramUniform3fv", // 400
    "glProgramUniform3i", // 401
    "glProgramUniform3iv", // 402
    "glProgramUniform3ui", // 403
    "glProgramUniform3uiv", // 404
    "glProgramUniform4d", // 405
    "glProgramUniform4dv", // 406
    "glProgramUniform4f", // 407
    "glProgramUniform4fv", // 408
    "glProgramUniform4i", // 409
    "glProgramUniform4iv", // 410
    "glProgramUniform4ui", // 411
    "glProgramUniform4uiv", // 412
    "glProgramUniformMatrix2dv", // 413
    "glProgramUniformMatrix2fv", // 414
    "glProgramUniformMatrix2x3dv", // 415
    "glProgramUniformMatrix2x3fv", // 416
    "glProgramUniformMatrix2x4dv", // 417
    "glProgramUniformMatrix2x4fv", // 418
    "glProgramUniformMatrix3dv", // 419
    "glProgramUniformMatrix3fv", // 420
    "glProgramUniformMatrix3x2dv", // 421
    "glProgramUniformMatrix3x2fv", // 422
    "glProgramUniformMatrix3x4dv", // 423
    "glProgramUniformMatrix3x4fv", // 424
    "glProgramUniformMatrix4dv", // 425
    "glProgramUniformMatrix4fv", // 426
    "glProgramUniformMatrix4x2dv", // 427
    "glProgramUniformMatrix4x2fv", // 428
    "glProgramUniformMatrix4x3dv", // 429
    "glProgramUniformMatrix4x3fv", // 430
    "glProvokingVertex", // 431
    "glPushDebugGroup", // 432
    "glQueryCounter", // 433
    "glReadBuffer", // 434
    "glReadPixels", // 435
    "glReadnPixels", // 436
    "glReleaseShaderCompiler", // 437
    "glRenderbufferStorage", // 438
    "glRenderbufferStorageMultisample", // 439
    "glResumeTransformFeedback", // 440
    "glSampleCoverage", // 441
    "glSampleMaski", // 442
    "glSamplerParameterIiv", // 443
    "glSamplerParameterIuiv", // 444
    "glSamplerParameterf", // 445
    "glSamplerParameterfv", // 446
    "glSamplerParameteri", // 447
    "glSamplerParameteriv", // 448
    "glScissor", // 449
    "glScissorArrayv", // 450
    "glScissorIndexed", // 451
    "glScissorIndexedv", // 452
    "glShaderBinary", // 453
    "glShaderSource", // 454
    "glShaderStorageBlockBinding", // 455
    "glSpecializeShader", // 456
    "glStencilFunc", // 457
    "glStencilFuncSeparate", // 458
    "glStencilMask", // 459
    "glStencilMaskSeparate", // 460
    "glStencilOp", // 461
    "glStencilOpSeparate", // 462
    "glTexBuffer", // 463
    "glTexBufferRange", // 464
    "glTexImage1D", // 465
    "glTexImage2D", // 466
    "glTexImage2DMultisample", // 467
    "glTexImage3D", // 468
    "glTexImage3DMultisample", // 469
    "glTexParameterIiv", // 470
    "glTexParameterIuiv", // 471
    "glTexParameterf", // 472
    "glTexParameterfv", // 473
    "glTexParameteri", // 474
    "glTexParameteriv", // 475
    "glTexStorage1D", // 476
    "glTexStorage2D", // 477
    "glTexStorage2DMultisample", // 478
    "glTexStorage3D", // 479
    "glTexStorage3DMultisample", // 480
    "glTexSubImage1D", // 481
    "glTexSubImage2D", // 482
    "glTexSubImage3D", // 483
    "glTextureBarrier", // 484
    "glTextureBuffer", // 485
    "glTextureBufferRange", // 486
    "glTextureParameterIiv", // 487
    "glTextureParameterIuiv", // 488
    "glTextureParameterf", // 489
    "glTextureParameterfv", // 490
    "glTextureParameteri", // 491
    "glTextureParameteriv", // 492
    "glTextureStorage1D", // 493
    "glTextureStorage2D", // 494
    "glTextureStorage2DMultisample", // 495
    "glTextureStorage3D", // 496
    "glTextureStorage3DMultisample", // 497
    "glTextureSubImage1D", // 498
    "glTextureSubImage2D", // 499
    "glTextureSubImage3D", // 500
    "glTextureView", // 501
    "glTransformFeedbackBufferBase", // 502
    "glTransformFeedbackBufferRange", // 503
    "glTransformFeedbackVaryings", // 504
    "glUniform1d", // 505
    "glUniform1dv", // 506
    "glUniform1f", // 507
    "glUniform1fv", // 508
    "glUniform1i", // 509
    "glUniform1iv", // 510
    "glUniform1ui", // 511
    "glUniform1uiv", // 512
    "glUniform2d", // 513
    "glUniform2dv", // 514
    "glUniform2f", // 515
    "glUniform2fv", // 516
    "glUniform2i", // 517
    "glUniform2iv", // 518
    "glUniform2ui", // 519
    "glUniform2uiv", // 520
    "glUniform3d", // 521
    "glUniform3dv", // 522
    "glUniform3f", // 523
    "glUniform3fv", // 524
    "glUniform3i", // 525
    "glUniform3iv", // 526
    "glUniform3ui", // 527
    "glUniform3uiv", // 528
    "glUniform4d", // 529
    "glUniform4dv", // 530
    "glUniform4f", // 531
    "glUniform4fv", // 532
    "glUniform4i", // 533
    "glUniform4iv", // 534
    "glUniform4ui", // 535
    "glUniform4uiv", // 536
    "glUniformBlockBinding", // 537
    "glUniformMatrix2dv", // 538
    "glUniformMatrix2fv", // 539
    "glUniformMatrix2x3dv", // 540
    "glUniformMatrix2x3fv", // 541
    "glUniformMatrix2x4dv", // 542
    "glUniformMatrix2x4fv", // 543
    "glUniformMatrix3dv", // 544
    "glUniformMatrix3fv", // 545
    "glUniformMatrix3x2dv", // 546
    "glUniformMatrix3x2fv", // 547
    "glUniformMatrix3x4dv", // 548
    "glUniformMatrix3x4fv", // 549
    "glUniformMatrix4dv", // 550
    "glUniformMatrix4fv", // 551
    "glUniformMatrix4x2dv", // 552
    "glUniformMatrix4x2fv", // 553
    "glUniformMatrix4x3dv", // 554
    "glUniformMatrix4x3fv", // 555
    "glUniformSubroutinesuiv", // 556
    "glUnmapBuffer", // 557
    "glUnmapNamedBuffer", // 558
    "glUseProgram", // 559
    "glUseProgramStages", // 560
    "glValidateProgram", // 561
    "glValidateProgramPipeline", // 562
    "glVertexArrayAttribBinding", // 563
    "glVertexArrayAttribFormat", // 564
    "glVertexArrayAttribIFormat", // 565
    "glVertexArrayAttribLFormat", // 566
    "glVertexArrayBindingDivisor", // 567
    "glVertexArrayElementBuffer", // 568
    "glVertexArrayVertexBuffer", // 569
    "glVertexArrayVertexBuffers", // 570
    "glVertexAttrib1d", // 571
    "glVertexAttrib1dv", // 572
    "glVertexAttrib1f", // 573
    "glVertexAttrib1fv", // 574
    "glVertexAttrib1s", // 575
    "glVertexAttrib1sv", // 576
    "glVertexAttrib2d", // 577
    "glVertexAttrib2dv", // 578
    "glVertexAttrib2f", // 579
    "glVertexAttrib2fv", // 580
    "glVertexAttrib2s", // 581
    "glVertexAttrib2sv", // 582
    "glVertexAttrib3d", // 583
    "glVertexAttrib3dv", // 584
    "glVertexAttrib3f", // 585
    "glVertexAttrib3fv", // 586
    "glVertexAttrib3s", // 587
    "glVertexAttrib3sv", // 588
    "glVertexAttrib4Nbv", // 589
    "glVertexAttrib4Niv", // 590
    "glVertexAttrib4Nsv", // 591
    "glVertexAttrib4Nub", // 592
    "glVertexAttrib4Nubv", // 593
    "glVertexAttrib4Nuiv", // 594
    "glVertexAttrib4Nusv", // 595
    "glVertexAttrib4bv", // 596
    "glVertexAttrib4d", // 597
    "glVertexAttrib4dv", // 598
    "glVertexAttrib4f", // 599
    "glVertexAttrib4fv", // 600
    "glVertexAttrib4iv", // 601
    "glVertexAttrib4s", // 602
    "glVertexAttrib4sv", // 603
    "glVertexAttrib4ubv", // 604
    "glVertexAttrib4uiv", // 605
    "glVertexAttrib4usv", // 606
    "glVertexAttribBinding", // 607
    "glVertexAttribDivisor", // 608
    "glVertexAttribFormat", // 609
    "glVertexAttribI1i", // 610
    "glVertexAttribI1iv", // 611
    "glVertexAttribI1ui", // 612
    "glVertexAttribI1uiv", // 613
    "glVertexAttribI2i", // 614
    "glVertexAttribI2iv", // 615
    "glVertexAttribI2ui", // 616
    "glVertexAttribI2uiv", // 617
    "glVertexAttribI3i", // 618
    "glVertexAttribI3iv", // 619
    "glVertexAttribI3ui", // 620
    "glVertexAttribI3uiv", // 621
    "glVertexAttribI4bv", // 622
    "glVertexAttribI4i", // 623
    "glVertexAttribI4iv", // 624
    "glVertexAttribI4sv", // 625
    "glVertexAttribI4ubv", // 626
    "glVertexAttribI4ui", // 627
    "glVertexAttribI4uiv", // 628
    "glVertexAttribI4usv", // 629
    "glVertexAttribIFormat", // 630
    "glVertexAttribIPointer", // 631
    "glVertexAttribL1d", // 632
    "glVertexAttribL1dv", // 633
    "glVertexAttribL2d", // 634
    "glVertexAttribL2dv", // 635
    "glVertexAttribL3d", // 636
    "glVertexAttribL3dv", // 637
    "glVertexAttribL4d", // 638
    "glVertexAttribL4dv", // 639
    "glVertexAttribLFormat", // 640
    "glVertexAttribLPointer", // 641
    "glVertexAttribP1ui", // 642
    "glVertexAttribP1uiv", // 643
    "glVertexAttribP2ui", // 644
    "glVertexAttribP2uiv", // 645
    "glVertexAttribP3ui", // 646
    "glVertexAttribP3uiv", // 647
    "glVertexAttribP4ui", // 648
    "glVertexAttribP4uiv", // 649
    "glVertexAttribPointer", // 650
    "glVertexBindingDivisor", // 651
    "glViewport", // 652
    "glViewportArrayv", // 653
    "glViewportIndexedf", // 654
    "glViewportIndexedfv", // 655
    "glWaitSync", // 656
    "glColorP3ui", // 657
    "glColorP3uiv", // 658
    "glColorP4ui", // 659
    "glColorP4uiv", // 660
    "glGetnColorTable", // 661
    "glGetnConvolutionFilter", // 662
    "glGetnHistogram", // 663
    "glGetnMapdv", // 664
    "glGetnMapfv", // 665
    "glGetnMapiv", // 666
    "glGetnMinmax", // 667
    "glGetnPixelMapfv", // 668
    "glGetnPixelMapuiv", // 669
    "glGetnPixelMapusv", // 670
    "glGetnPolygonStipple", // 671
    "glGetnSeparableFilter", // 672
    "glMultiTexCoordP1ui", // 673
    "glMultiTexCoordP1uiv", // 674
    "glMultiTexCoordP2ui", // 675
    "glMultiTexCoordP2uiv", // 676
    "glMultiTexCoordP3ui", // 677
    "glMultiTexCoordP3uiv", // 678
    "glMultiTexCoordP4ui", // 679
    "glMultiTexCoordP4uiv", // 680
    "glNormalP3ui", // 681
    "glNormalP3uiv", // 682
    "glSecondaryColorP3ui", // 683
    "glSecondaryColorP3uiv", // 684
    "glTexCoordP1ui", // 685
    "glTexCoordP1uiv", // 686
    "glTexCoordP2ui", // 687
    "glTexCoordP2uiv", // 688
    "glTexCoordP3ui", // 689
    "glTexCoordP3uiv", // 690
    "glTexCoordP4ui", // 691
    "glTexCoordP4uiv", // 692
    "glVertexP2ui", // 693
    "glVertexP2uiv", // 694
    "glVertexP3ui", // 695
    "glVertexP3uiv", // 696
    "glVertexP4ui", // 697
    "glVertexP4uiv", // 698
    "glAccum", // 699
    "glAlphaFunc", // 700
    "glAreTexturesResident", // 701
    "glArrayElement", // 702
    "glBegin", // 703
    "glBitmap", // 704
    "glCallList", // 705
    "glCallLists", // 706
    "glClearAccum", // 707
    "glClearIndex", // 708
    "glClientActiveTexture", // 709
    "glClipPlane", // 710
    "glColor3b", // 711
    "glColor3bv", // 712
    "glColor3d", // 713
    "glColor3dv", // 714
    "glColor3f", // 715
    "glColor3fv", // 716
    "glColor3i", // 717
    "glColor3iv", // 718
    "glColor3s", // 719
    "glColor3sv", // 720
    "glColor3ub", // 721
    "glColor3ubv", // 722
    "glColor3ui", // 723
    "glColor3uiv", // 724
    "glColor3us", // 725
    "glColor3usv", // 726
    "glColor4b", // 727
    "glColor4bv", // 728
    "glColor4d", // 729
    "glColor4dv", // 730
    "glColor4f", // 731
    "glColor4fv", // 732
    "glColor4i", // 733
    "glColor4iv", // 734
    "glColor4s", // 735
    "glColor4sv", // 736
    "glColor4ub", // 737
    "glColor4ubv", // 738
    "glColor4ui", // 739
    "glColor4uiv", // 740
    "glColor4us", // 741
    "glColor4usv", // 742
    "glColorMaterial", // 743
    "glColorPointer", // 744
    "glCopyPixels", // 745
    "glDeleteLists", // 746
    "glDisableClientState", // 747
    "glDrawPixels", // 748
    "glEdgeFlag", // 749
    "glEdgeFlagPointer", // 750
    "glEdgeFlagv", // 751
    "glEnableClientState", // 752
    "glEnd", // 753
    "glEndList", // 754
    "glEvalCoord1d", // 755
    "glEvalCoord1dv", // 756
    "glEvalCoord1f", // 757
    "glEvalCoord1fv", // 758
    "glEvalCoord2d", // 759
    "glEvalCoord2dv", // 760
    "glEvalCoord2f", // 761
    "glEvalCoord2fv", // 762
    "glEvalMesh1", // 763
    "glEvalMesh2", // 764
    "glEvalPoint1", // 765
    "glEvalPoint2", // 766
    "glFeedbackBuffer", // 767
    "glFogCoordPointer", // 768
    "glFogCoordd", // 769
    "glFogCoorddv", // 770
    "glFogCoordf", // 771
    "glFogCoordfv", // 772
    "glFogf", // 773
    "glFogfv", // 774
    "glFogi", // 775
    "glFogiv", // 776
    "glFrustum", // 777
    "glGenLists", // 778
    "glGetClipPlane", // 779
    "glGetLightfv", // 780
    "glGetLightiv", // 781
    "glGetMapdv", // 782
    "glGetMapfv", // 783
    "glGetMapiv", // 784
    "glGetMaterialfv", // 785
    "glGetMaterialiv", // 786
    "glGetPixelMapfv", // 787
    "glGetPixelMapuiv", // 788
    "glGetPixelMapusv", // 789
    "glGetPolygonStipple", // 790
    "glGetTexEnvfv", // 791
    "glGetTexEnviv", // 792
    "glGetTexGendv", // 793
    "glGetTexGenfv", // 794
    "glGetTexGeniv", // 795
    "glIndexMask", // 796
    "glIndexPointer", // 797
    "glIndexd", // 798
    "glIndexdv", // 799
    "glIndexf", // 800
    "glIndexfv", // 801
    "glIndexi", // 802
    "glIndexiv", // 803
    "glIndexs", // 804
    "glIndexsv", // 805
    "glIndexub", // 806
    "glIndexubv", // 807
    "glInitNames", // 808
    "glInterleavedArrays", // 809
    "glIsList", // 810
    "glLightModelf", // 811
    "glLightModelfv", // 812
    "glLightModeli", // 813
    "glLightModeliv", // 814
    "glLightf", // 815
    "glLightfv", // 816
    "glLighti", // 817
    "glLightiv", // 818
    "glLineStipple", // 819
    "glListBase", // 820
    "glLoadIdentity", // 821
    "glLoadMatrixd", // 822
    "glLoadMatrixf", // 823
    "glLoadName", // 824
    "glLoadTransposeMatrixd", // 825
    "glLoadTransposeMatrixf", // 826
    "glMap1d", // 827
    "glMap1f", // 828
    "glMap2d", // 829
    "glMap2f", // 830
    "glMapGrid1d", // 831
    "glMapGrid1f", // 832
    "glMapGrid2d", // 833
    "glMapGrid2f", // 834
    "glMaterialf", // 835
    "glMaterialfv", // 836
    "glMateriali", // 837
    "glMaterialiv", // 838
    "glMatrixMode", // 839
    "glMultMatrixd", // 840
    "glMultMatrixf", // 841
    "glMultTransposeMatrixd", // 842
    "glMultTransposeMatrixf", // 843
    "glMultiTexCoord1d", // 844
    "glMultiTexCoord1dv", // 845
    "glMultiTexCoord1f", // 846
    "glMultiTexCoord1fv", // 847
    "glMultiTexCoord1i", // 848
    "glMultiTexCoord1iv", // 849
    "glMultiTexCoord1s", // 850
    "glMultiTexCoord1sv", // 851
    "glMultiTexCoord2d", // 852
    "glMultiTexCoord2dv", // 853
    "glMultiTexCoord2f", // 854
    "glMultiTexCoord2fv", // 855
    "glMultiTexCoord2i", // 856
    "glMultiTexCoord2iv", // 857
    "glMultiTexCoord2s", // 858
    "glMultiTexCoord2sv", // 859
    "glMultiTexCoord3d", // 860
    "glMultiTexCoord3dv", // 861
    "glMultiTexCoord3f", // 862
    "glMultiTexCoord3fv", // 863
    "glMultiTexCoord3i", // 864
    "glMultiTexCoord3iv", // 865
    "glMultiTexCoord3s", // 866
    "glMultiTexCoord3sv", // 867
    "glMultiTexCoord4d", // 868
    "glMultiTexCoord4dv", // 869
    "glMultiTexCoord4f", // 870
    "glMultiTexCoord4fv", // 871
    "glMultiTexCoord4i", // 872
    "glMultiTexCoord4iv", // 873
    "glMultiTexCoord4s", // 874
    "glMultiTexCoord4sv", // 875
    "glNewList", // 876
    "glNormal3b", // 877
    "glNormal3bv", // 878
    "glNormal3d", // 879
    "glNormal3dv", // 880
    "glNormal3f", // 881
    "glNormal3fv", // 882
    "glNormal3i", // 883
    "glNormal3iv", // 884
    "glNormal3s", // 885
    "glNormal3sv", // 886
    "glNormalPointer", // 887
    "glOrtho", // 888
    "glPassThrough", // 889
    "glPixelMapfv", // 890
    "glPixelMapuiv", // 891
    "glPixelMapusv", // 892
    "glPixelTransferf", // 893
    "glPixelTransferi", // 894
    "glPixelZoom", // 895
    "glPolygonStipple", // 896
    "glPopAttrib", // 897
    "glPopClientAttrib", // 898
    "glPopMatrix", // 899
    "glPopName", // 900
    "glPrioritizeTextures", // 901
    "glPushAttrib", // 902
    "glPushClientAttrib", // 903
    "glPushMatrix", // 904
    "glPushName", // 905
    "glRasterPos2d", // 906
    "glRasterPos2dv", // 907
    "glRasterPos2f", // 908
    "glRasterPos2fv", // 909
    "glRasterPos2i", // 910
    "glRasterPos2iv", // 911
    "glRasterPos2s", // 912
    "glRasterPos2sv", // 913
    "glRasterPos3d", // 914
    "glRasterPos3dv", // 915
    "glRasterPos3f", // 916
    "glRasterPos3fv", // 917
    "glRasterPos3i", // 918
    "glRasterPos3iv", // 919
    "glRasterPos3s", // 920
    "glRasterPos3sv", // 921
    "glRasterPos4d", // 922
    "glRasterPos4dv", // 923
    "glRasterPos4f", // 924
    "glRasterPos4fv", // 925
    "glRasterPos4i", // 926
    "glRasterPos4iv", // 927
    "glRasterPos4s", // 928
    "glRasterPos4sv", // 929
    "glRectd", // 930
    "glRectdv", // 931
    "glRectf", // 932
    "glRectfv", // 933
    "glRecti", // 934
    "glRectiv", // 935
    "glRects", // 936
    "glRectsv", // 937
    "glRenderMode", // 938
    "glRotated", // 939
    "glRotatef", // 940
    "glScaled", // 941
    "glScalef", // 942
    "glSecondaryColor3b", // 943
    "glSecondaryColor3bv", // 944
    "glSecondaryColor3d", // 945
    "glSecondaryColor3dv", // 946
    "glSecondaryColor3f", // 947
    "glSecondaryColor3fv", // 948
    "glSecondaryColor3i", // 949
    "glSecondaryColor3iv", // 950
    "glSecondaryColor3s", // 951
    "glSecondaryColor3sv", // 952
    "glSecondaryColor3ub", // 953
    "glSecondaryColor3ubv", // 954
    "glSecondaryColor3ui", // 955
    "glSecondaryColor3uiv", // 956
    "glSecondaryColor3us", // 957
    "glSecondaryColor3usv", // 958
    "glSecondaryColorPointer", // 959
    "glSelectBuffer", // 960
    "glShadeModel", // 961
    "glTexCoord1d", // 962
    "glTexCoord1dv", // 963
    "glTexCoord1f", // 964
    "glTexCoord1fv", // 965
    "glTexCoord1i", // 966
    "glTexCoord1iv", // 967
    "glTexCoord1s", // 968
    "glTexCoord1sv", // 969
    "glTexCoord2d", // 970
    "glTexCoord2dv", // 971
    "glTexCoord2f", // 972
    "glTexCoord2fv", // 973
    "glTexCoord2i", // 974
    "glTexCoord2iv", // 975
    "glTexCoord2s", // 976
    "glTexCoord2sv", // 977
    "glTexCoord3d", // 978
    "glTexCoord3dv", // 979
    "glTexCoord3f", // 980
    "glTexCoord3fv", // 981
    "glTexCoord3i", // 982
    "glTexCoord3iv", // 983
    "glTexCoord3s", // 984
    "glTexCoord3sv", // 985
    "glTexCoord4d", // 986
    "glTexCoord4dv", // 987
    "glTexCoord4f", // 988
    "glTexCoord4fv", // 989
    "glTexCoord4i", // 990
    "glTexCoord4iv", // 991
    "glTexCoord4s", // 992
    "glTexCoord4sv", // 993
    "glTexCoordPointer", // 994
    "glTexEnvf", // 995
    "glTexEnvfv", // 996
    "glTexEnvi", // 997
    "glTexEnviv", // 998
    "glTexGend", // 999
    "glTexGendv", // 1000
    "glTexGenf", // 1001
    "glTexGenfv", // 1002
    "glTexGeni", // 1003
    "glTexGeniv", // 1004
    "glTranslated", // 1005
    "glTranslatef", // 1006
    "glVertex2d", // 1007
    "glVertex2dv", // 1008
    "glVertex2f", // 1009
    "glVertex2fv", // 1010
    "glVertex2i", // 1011
    "glVertex2iv", // 1012
    "glVertex2s", // 1013
    "glVertex2sv", // 1014
    "glVertex3d", // 1015
    "glVertex3dv", // 1016
    "glVertex3f", // 1017
    "glVertex3fv", // 1018
    "glVertex3i", // 1019
    "glVertex3iv", // 1020
    "glVertex3s", // 1021
    "glVertex3sv", // 1022
    "glVertex4d", // 1023
    "glVertex4dv", // 1024
    "glVertex4f", // 1025
    "glVertex4fv", // 1026
    "glVertex4i", // 1027
    "glVertex4iv", // 1028
    "glVertex4s", // 1029
    "glVertex4sv", // 1030
    "glVertexPointer", // 1031
    "glWindowPos2d", // 1032
    "glWindowPos2dv", // 1033
    "glWindowPos2f", // 1034
    "glWindowPos2fv", // 1035
    "glWindowPos2i", // 1036
    "glWindowPos2iv", // 1037
    "glWindowPos2s", // 1038
    "glWindowPos2sv", // 1039
    "glWindowPos3d", // 1040
    "glWindowPos3dv", // 1041
    "glWindowPos3f", // 1042
    "glWindowPos3fv", // 1043
    "glWindowPos3i", // 1044
    "glWindowPos3iv", // 1045
    "glWindowPos3s", // 1046
    "glWindowPos3sv", // 1047
    "glDepthRangeArrayfvNV", // 1048
};
int gGlProfileN = 1049;
const TglmtProcEntry kTglmtProcTable[] = {
    {"glAccum", (void*)&glAccum},
    {"glActiveProgramEXT", (void*)&glActiveProgramEXT},
    {"glActiveShaderProgram", (void*)&glActiveShaderProgram},
    {"glActiveShaderProgramEXT", (void*)&glActiveShaderProgramEXT},
    {"glActiveStencilFaceEXT", (void*)&glActiveStencilFaceEXT},
    {"glActiveTexture", (void*)&glActiveTexture},
    {"glActiveTextureARB", (void*)&glActiveTextureARB},
    {"glAlphaFunc", (void*)&glAlphaFunc},
    {"glAreTexturesResident", (void*)&glAreTexturesResident},
    {"glArrayElement", (void*)&glArrayElement},
    {"glAttachObjectARB", (void*)&glAttachObjectARB},
    {"glAttachShader", (void*)&glAttachShader},
    {"glBegin", (void*)&glBegin},
    {"glBeginConditionalRender", (void*)&glBeginConditionalRender},
    {"glBeginConditionalRenderNV", (void*)&glBeginConditionalRenderNV},
    {"glBeginPerfMonitorAMD", (void*)&glBeginPerfMonitorAMD},
    {"glBeginPerfQueryINTEL", (void*)&glBeginPerfQueryINTEL},
    {"glBeginQuery", (void*)&glBeginQuery},
    {"glBeginQueryARB", (void*)&glBeginQueryARB},
    {"glBeginQueryEXT", (void*)&glBeginQueryEXT},
    {"glBeginQueryIndexed", (void*)&glBeginQueryIndexed},
    {"glBeginTransformFeedback", (void*)&glBeginTransformFeedback},
    {"glBeginTransformFeedbackEXT", (void*)&glBeginTransformFeedbackEXT},
    {"glBeginTransformFeedbackNV", (void*)&glBeginTransformFeedbackNV},
    {"glBindAttribLocation", (void*)&glBindAttribLocation},
    {"glBindAttribLocationARB", (void*)&glBindAttribLocationARB},
    {"glBindBuffer", (void*)&glBindBuffer},
    {"glBindBufferARB", (void*)&glBindBufferARB},
    {"glBindBufferBase", (void*)&glBindBufferBase},
    {"glBindBufferBaseEXT", (void*)&glBindBufferBaseEXT},
    {"glBindBufferBaseNV", (void*)&glBindBufferBaseNV},
    {"glBindBufferOffsetEXT", (void*)&glBindBufferOffsetEXT},
    {"glBindBufferRange", (void*)&glBindBufferRange},
    {"glBindBufferRangeEXT", (void*)&glBindBufferRangeEXT},
    {"glBindBufferRangeNV", (void*)&glBindBufferRangeNV},
    {"glBindBuffersBase", (void*)&glBindBuffersBase},
    {"glBindBuffersRange", (void*)&glBindBuffersRange},
    {"glBindFragDataLocation", (void*)&glBindFragDataLocation},
    {"glBindFragDataLocationEXT", (void*)&glBindFragDataLocationEXT},
    {"glBindFragDataLocationIndexed", (void*)&glBindFragDataLocationIndexed},
    {"glBindFragDataLocationIndexedEXT", (void*)&glBindFragDataLocationIndexedEXT},
    {"glBindFramebuffer", (void*)&glBindFramebuffer},
    {"glBindFramebufferEXT", (void*)&glBindFramebufferEXT},
    {"glBindFramebufferOES", (void*)&glBindFramebufferOES},
    {"glBindImageTexture", (void*)&glBindImageTexture},
    {"glBindImageTextures", (void*)&glBindImageTextures},
    {"glBindMultiTextureEXT", (void*)&glBindMultiTextureEXT},
    {"glBindProgramPipeline", (void*)&glBindProgramPipeline},
    {"glBindProgramPipelineEXT", (void*)&glBindProgramPipelineEXT},
    {"glBindRenderbuffer", (void*)&glBindRenderbuffer},
    {"glBindRenderbufferEXT", (void*)&glBindRenderbufferEXT},
    {"glBindRenderbufferOES", (void*)&glBindRenderbufferOES},
    {"glBindSampler", (void*)&glBindSampler},
    {"glBindSamplers", (void*)&glBindSamplers},
    {"glBindShadingRateImageNV", (void*)&glBindShadingRateImageNV},
    {"glBindTexture", (void*)&glBindTexture},
    {"glBindTextureEXT", (void*)&glBindTextureEXT},
    {"glBindTextureUnit", (void*)&glBindTextureUnit},
    {"glBindTextures", (void*)&glBindTextures},
    {"glBindTransformFeedback", (void*)&glBindTransformFeedback},
    {"glBindTransformFeedbackNV", (void*)&glBindTransformFeedbackNV},
    {"glBindVertexArray", (void*)&glBindVertexArray},
    {"glBindVertexArrayAPPLE", (void*)&glBindVertexArrayAPPLE},
    {"glBindVertexArrayOES", (void*)&glBindVertexArrayOES},
    {"glBindVertexBuffer", (void*)&glBindVertexBuffer},
    {"glBindVertexBuffers", (void*)&glBindVertexBuffers},
    {"glBitmap", (void*)&glBitmap},
    {"glBlendBarrierKHR", (void*)&glBlendBarrierKHR},
    {"glBlendBarrierNV", (void*)&glBlendBarrierNV},
    {"glBlendColor", (void*)&glBlendColor},
    {"glBlendColorEXT", (void*)&glBlendColorEXT},
    {"glBlendEquation", (void*)&glBlendEquation},
    {"glBlendEquationEXT", (void*)&glBlendEquationEXT},
    {"glBlendEquationOES", (void*)&glBlendEquationOES},
    {"glBlendEquationSeparate", (void*)&glBlendEquationSeparate},
    {"glBlendEquationSeparateEXT", (void*)&glBlendEquationSeparateEXT},
    {"glBlendEquationSeparateOES", (void*)&glBlendEquationSeparateOES},
    {"glBlendEquationSeparatei", (void*)&glBlendEquationSeparatei},
    {"glBlendEquationSeparateiARB", (void*)&glBlendEquationSeparateiARB},
    {"glBlendEquationSeparateiEXT", (void*)&glBlendEquationSeparateiEXT},
    {"glBlendEquationSeparateiOES", (void*)&glBlendEquationSeparateiOES},
    {"glBlendEquationi", (void*)&glBlendEquationi},
    {"glBlendEquationiARB", (void*)&glBlendEquationiARB},
    {"glBlendEquationiEXT", (void*)&glBlendEquationiEXT},
    {"glBlendEquationiOES", (void*)&glBlendEquationiOES},
    {"glBlendFunc", (void*)&glBlendFunc},
    {"glBlendFuncSeparate", (void*)&glBlendFuncSeparate},
    {"glBlendFuncSeparateEXT", (void*)&glBlendFuncSeparateEXT},
    {"glBlendFuncSeparateOES", (void*)&glBlendFuncSeparateOES},
    {"glBlendFuncSeparatei", (void*)&glBlendFuncSeparatei},
    {"glBlendFuncSeparateiARB", (void*)&glBlendFuncSeparateiARB},
    {"glBlendFuncSeparateiEXT", (void*)&glBlendFuncSeparateiEXT},
    {"glBlendFuncSeparateiOES", (void*)&glBlendFuncSeparateiOES},
    {"glBlendFunci", (void*)&glBlendFunci},
    {"glBlendFunciARB", (void*)&glBlendFunciARB},
    {"glBlendFunciEXT", (void*)&glBlendFunciEXT},
    {"glBlendFunciOES", (void*)&glBlendFunciOES},
    {"glBlendParameteriNV", (void*)&glBlendParameteriNV},
    {"glBlitFramebuffer", (void*)&glBlitFramebuffer},
    {"glBlitFramebufferANGLE", (void*)&glBlitFramebufferANGLE},
    {"glBlitFramebufferEXT", (void*)&glBlitFramebufferEXT},
    {"glBlitFramebufferNV", (void*)&glBlitFramebufferNV},
    {"glBlitNamedFramebuffer", (void*)&glBlitNamedFramebuffer},
    {"glBufferAddressRangeNV", (void*)&glBufferAddressRangeNV},
    {"glBufferAttachMemoryNV", (void*)&glBufferAttachMemoryNV},
    {"glBufferData", (void*)&glBufferData},
    {"glBufferDataARB", (void*)&glBufferDataARB},
    {"glBufferPageCommitmentARB", (void*)&glBufferPageCommitmentARB},
    {"glBufferStorage", (void*)&glBufferStorage},
    {"glBufferStorageEXT", (void*)&glBufferStorageEXT},
    {"glBufferSubData", (void*)&glBufferSubData},
    {"glBufferSubDataARB", (void*)&glBufferSubDataARB},
    {"glCallCommandListNV", (void*)&glCallCommandListNV},
    {"glCallList", (void*)&glCallList},
    {"glCallLists", (void*)&glCallLists},
    {"glCheckFramebufferStatus", (void*)&glCheckFramebufferStatus},
    {"glCheckFramebufferStatusEXT", (void*)&glCheckFramebufferStatusEXT},
    {"glCheckFramebufferStatusOES", (void*)&glCheckFramebufferStatusOES},
    {"glCheckNamedFramebufferStatus", (void*)&glCheckNamedFramebufferStatus},
    {"glCheckNamedFramebufferStatusEXT", (void*)&glCheckNamedFramebufferStatusEXT},
    {"glClampColor", (void*)&glClampColor},
    {"glClampColorARB", (void*)&glClampColorARB},
    {"glClear", (void*)&glClear},
    {"glClearAccum", (void*)&glClearAccum},
    {"glClearBufferData", (void*)&glClearBufferData},
    {"glClearBufferSubData", (void*)&glClearBufferSubData},
    {"glClearBufferfi", (void*)&glClearBufferfi},
    {"glClearBufferfv", (void*)&glClearBufferfv},
    {"glClearBufferiv", (void*)&glClearBufferiv},
    {"glClearBufferuiv", (void*)&glClearBufferuiv},
    {"glClearColor", (void*)&glClearColor},
    {"glClearColorIiEXT", (void*)&glClearColorIiEXT},
    {"glClearColorIuiEXT", (void*)&glClearColorIuiEXT},
    {"glClearDepth", (void*)&glClearDepth},
    {"glClearDepthdNV", (void*)&glClearDepthdNV},
    {"glClearDepthf", (void*)&glClearDepthf},
    {"glClearIndex", (void*)&glClearIndex},
    {"glClearNamedBufferData", (void*)&glClearNamedBufferData},
    {"glClearNamedBufferDataEXT", (void*)&glClearNamedBufferDataEXT},
    {"glClearNamedBufferSubData", (void*)&glClearNamedBufferSubData},
    {"glClearNamedBufferSubDataEXT", (void*)&glClearNamedBufferSubDataEXT},
    {"glClearNamedFramebufferfi", (void*)&glClearNamedFramebufferfi},
    {"glClearNamedFramebufferfv", (void*)&glClearNamedFramebufferfv},
    {"glClearNamedFramebufferiv", (void*)&glClearNamedFramebufferiv},
    {"glClearNamedFramebufferuiv", (void*)&glClearNamedFramebufferuiv},
    {"glClearStencil", (void*)&glClearStencil},
    {"glClearTexImage", (void*)&glClearTexImage},
    {"glClearTexImageEXT", (void*)&glClearTexImageEXT},
    {"glClearTexSubImage", (void*)&glClearTexSubImage},
    {"glClearTexSubImageEXT", (void*)&glClearTexSubImageEXT},
    {"glClientActiveTexture", (void*)&glClientActiveTexture},
    {"glClientAttribDefaultEXT", (void*)&glClientAttribDefaultEXT},
    {"glClientWaitSync", (void*)&glClientWaitSync},
    {"glClientWaitSyncAPPLE", (void*)&glClientWaitSyncAPPLE},
    {"glClipControl", (void*)&glClipControl},
    {"glClipControlEXT", (void*)&glClipControlEXT},
    {"glClipPlane", (void*)&glClipPlane},
    {"glColor3b", (void*)&glColor3b},
    {"glColor3bv", (void*)&glColor3bv},
    {"glColor3d", (void*)&glColor3d},
    {"glColor3dv", (void*)&glColor3dv},
    {"glColor3f", (void*)&glColor3f},
    {"glColor3fv", (void*)&glColor3fv},
    {"glColor3i", (void*)&glColor3i},
    {"glColor3iv", (void*)&glColor3iv},
    {"glColor3s", (void*)&glColor3s},
    {"glColor3sv", (void*)&glColor3sv},
    {"glColor3ub", (void*)&glColor3ub},
    {"glColor3ubv", (void*)&glColor3ubv},
    {"glColor3ui", (void*)&glColor3ui},
    {"glColor3uiv", (void*)&glColor3uiv},
    {"glColor3us", (void*)&glColor3us},
    {"glColor3usv", (void*)&glColor3usv},
    {"glColor4b", (void*)&glColor4b},
    {"glColor4bv", (void*)&glColor4bv},
    {"glColor4d", (void*)&glColor4d},
    {"glColor4dv", (void*)&glColor4dv},
    {"glColor4f", (void*)&glColor4f},
    {"glColor4fv", (void*)&glColor4fv},
    {"glColor4i", (void*)&glColor4i},
    {"glColor4iv", (void*)&glColor4iv},
    {"glColor4s", (void*)&glColor4s},
    {"glColor4sv", (void*)&glColor4sv},
    {"glColor4ub", (void*)&glColor4ub},
    {"glColor4ubv", (void*)&glColor4ubv},
    {"glColor4ui", (void*)&glColor4ui},
    {"glColor4uiv", (void*)&glColor4uiv},
    {"glColor4us", (void*)&glColor4us},
    {"glColor4usv", (void*)&glColor4usv},
    {"glColorFormatNV", (void*)&glColorFormatNV},
    {"glColorMask", (void*)&glColorMask},
    {"glColorMaskIndexedEXT", (void*)&glColorMaskIndexedEXT},
    {"glColorMaski", (void*)&glColorMaski},
    {"glColorMaskiEXT", (void*)&glColorMaskiEXT},
    {"glColorMaskiOES", (void*)&glColorMaskiOES},
    {"glColorMaterial", (void*)&glColorMaterial},
    {"glColorP3ui", (void*)&glColorP3ui},
    {"glColorP3uiv", (void*)&glColorP3uiv},
    {"glColorP4ui", (void*)&glColorP4ui},
    {"glColorP4uiv", (void*)&glColorP4uiv},
    {"glColorPointer", (void*)&glColorPointer},
    {"glColorSubTable", (void*)&glColorSubTable},
    {"glColorTable", (void*)&glColorTable},
    {"glColorTableParameterfv", (void*)&glColorTableParameterfv},
    {"glColorTableParameteriv", (void*)&glColorTableParameteriv},
    {"glCommandListSegmentsNV", (void*)&glCommandListSegmentsNV},
    {"glCompileCommandListNV", (void*)&glCompileCommandListNV},
    {"glCompileShader", (void*)&glCompileShader},
    {"glCompileShaderARB", (void*)&glCompileShaderARB},
    {"glCompileShaderIncludeARB", (void*)&glCompileShaderIncludeARB},
    {"glCompressedMultiTexImage1DEXT", (void*)&glCompressedMultiTexImage1DEXT},
    {"glCompressedMultiTexImage2DEXT", (void*)&glCompressedMultiTexImage2DEXT},
    {"glCompressedMultiTexImage3DEXT", (void*)&glCompressedMultiTexImage3DEXT},
    {"glCompressedMultiTexSubImage1DEXT", (void*)&glCompressedMultiTexSubImage1DEXT},
    {"glCompressedMultiTexSubImage2DEXT", (void*)&glCompressedMultiTexSubImage2DEXT},
    {"glCompressedMultiTexSubImage3DEXT", (void*)&glCompressedMultiTexSubImage3DEXT},
    {"glCompressedTexImage1D", (void*)&glCompressedTexImage1D},
    {"glCompressedTexImage1DARB", (void*)&glCompressedTexImage1DARB},
    {"glCompressedTexImage2D", (void*)&glCompressedTexImage2D},
    {"glCompressedTexImage2DARB", (void*)&glCompressedTexImage2DARB},
    {"glCompressedTexImage3D", (void*)&glCompressedTexImage3D},
    {"glCompressedTexImage3DARB", (void*)&glCompressedTexImage3DARB},
    {"glCompressedTexImage3DOES", (void*)&glCompressedTexImage3DOES},
    {"glCompressedTexSubImage1D", (void*)&glCompressedTexSubImage1D},
    {"glCompressedTexSubImage1DARB", (void*)&glCompressedTexSubImage1DARB},
    {"glCompressedTexSubImage2D", (void*)&glCompressedTexSubImage2D},
    {"glCompressedTexSubImage2DARB", (void*)&glCompressedTexSubImage2DARB},
    {"glCompressedTexSubImage3D", (void*)&glCompressedTexSubImage3D},
    {"glCompressedTexSubImage3DARB", (void*)&glCompressedTexSubImage3DARB},
    {"glCompressedTexSubImage3DOES", (void*)&glCompressedTexSubImage3DOES},
    {"glCompressedTextureImage1DEXT", (void*)&glCompressedTextureImage1DEXT},
    {"glCompressedTextureImage2DEXT", (void*)&glCompressedTextureImage2DEXT},
    {"glCompressedTextureImage3DEXT", (void*)&glCompressedTextureImage3DEXT},
    {"glCompressedTextureSubImage1D", (void*)&glCompressedTextureSubImage1D},
    {"glCompressedTextureSubImage1DEXT", (void*)&glCompressedTextureSubImage1DEXT},
    {"glCompressedTextureSubImage2D", (void*)&glCompressedTextureSubImage2D},
    {"glCompressedTextureSubImage2DEXT", (void*)&glCompressedTextureSubImage2DEXT},
    {"glCompressedTextureSubImage3D", (void*)&glCompressedTextureSubImage3D},
    {"glCompressedTextureSubImage3DEXT", (void*)&glCompressedTextureSubImage3DEXT},
    {"glConvolutionFilter1D", (void*)&glConvolutionFilter1D},
    {"glConvolutionFilter2D", (void*)&glConvolutionFilter2D},
    {"glConvolutionParameterf", (void*)&glConvolutionParameterf},
    {"glConvolutionParameterfv", (void*)&glConvolutionParameterfv},
    {"glConvolutionParameteri", (void*)&glConvolutionParameteri},
    {"glConvolutionParameteriv", (void*)&glConvolutionParameteriv},
    {"glCopyBufferSubData", (void*)&glCopyBufferSubData},
    {"glCopyBufferSubDataNV", (void*)&glCopyBufferSubDataNV},
    {"glCopyColorSubTable", (void*)&glCopyColorSubTable},
    {"glCopyColorTable", (void*)&glCopyColorTable},
    {"glCopyConvolutionFilter1D", (void*)&glCopyConvolutionFilter1D},
    {"glCopyConvolutionFilter2D", (void*)&glCopyConvolutionFilter2D},
    {"glCopyImageSubData", (void*)&glCopyImageSubData},
    {"glCopyImageSubDataEXT", (void*)&glCopyImageSubDataEXT},
    {"glCopyImageSubDataNV", (void*)&glCopyImageSubDataNV},
    {"glCopyImageSubDataOES", (void*)&glCopyImageSubDataOES},
    {"glCopyMultiTexImage1DEXT", (void*)&glCopyMultiTexImage1DEXT},
    {"glCopyMultiTexImage2DEXT", (void*)&glCopyMultiTexImage2DEXT},
    {"glCopyMultiTexSubImage1DEXT", (void*)&glCopyMultiTexSubImage1DEXT},
    {"glCopyMultiTexSubImage2DEXT", (void*)&glCopyMultiTexSubImage2DEXT},
    {"glCopyMultiTexSubImage3DEXT", (void*)&glCopyMultiTexSubImage3DEXT},
    {"glCopyNamedBufferSubData", (void*)&glCopyNamedBufferSubData},
    {"glCopyPathNV", (void*)&glCopyPathNV},
    {"glCopyPixels", (void*)&glCopyPixels},
    {"glCopyTexImage1D", (void*)&glCopyTexImage1D},
    {"glCopyTexImage1DEXT", (void*)&glCopyTexImage1DEXT},
    {"glCopyTexImage2D", (void*)&glCopyTexImage2D},
    {"glCopyTexImage2DEXT", (void*)&glCopyTexImage2DEXT},
    {"glCopyTexSubImage1D", (void*)&glCopyTexSubImage1D},
    {"glCopyTexSubImage1DEXT", (void*)&glCopyTexSubImage1DEXT},
    {"glCopyTexSubImage2D", (void*)&glCopyTexSubImage2D},
    {"glCopyTexSubImage2DEXT", (void*)&glCopyTexSubImage2DEXT},
    {"glCopyTexSubImage3D", (void*)&glCopyTexSubImage3D},
    {"glCopyTexSubImage3DEXT", (void*)&glCopyTexSubImage3DEXT},
    {"glCopyTexSubImage3DOES", (void*)&glCopyTexSubImage3DOES},
    {"glCopyTextureImage1DEXT", (void*)&glCopyTextureImage1DEXT},
    {"glCopyTextureImage2DEXT", (void*)&glCopyTextureImage2DEXT},
    {"glCopyTextureSubImage1D", (void*)&glCopyTextureSubImage1D},
    {"glCopyTextureSubImage1DEXT", (void*)&glCopyTextureSubImage1DEXT},
    {"glCopyTextureSubImage2D", (void*)&glCopyTextureSubImage2D},
    {"glCopyTextureSubImage2DEXT", (void*)&glCopyTextureSubImage2DEXT},
    {"glCopyTextureSubImage3D", (void*)&glCopyTextureSubImage3D},
    {"glCopyTextureSubImage3DEXT", (void*)&glCopyTextureSubImage3DEXT},
    {"glCoverFillPathInstancedNV", (void*)&glCoverFillPathInstancedNV},
    {"glCoverFillPathNV", (void*)&glCoverFillPathNV},
    {"glCoverStrokePathInstancedNV", (void*)&glCoverStrokePathInstancedNV},
    {"glCoverStrokePathNV", (void*)&glCoverStrokePathNV},
    {"glCoverageModulationNV", (void*)&glCoverageModulationNV},
    {"glCoverageModulationTableNV", (void*)&glCoverageModulationTableNV},
    {"glCreateBuffers", (void*)&glCreateBuffers},
    {"glCreateCommandListsNV", (void*)&glCreateCommandListsNV},
    {"glCreateFramebuffers", (void*)&glCreateFramebuffers},
    {"glCreatePerfQueryINTEL", (void*)&glCreatePerfQueryINTEL},
    {"glCreateProgram", (void*)&glCreateProgram},
    {"glCreateProgramObjectARB", (void*)&glCreateProgramObjectARB},
    {"glCreateProgramPipelines", (void*)&glCreateProgramPipelines},
    {"glCreateQueries", (void*)&glCreateQueries},
    {"glCreateRenderbuffers", (void*)&glCreateRenderbuffers},
    {"glCreateSamplers", (void*)&glCreateSamplers},
    {"glCreateShader", (void*)&glCreateShader},
    {"glCreateShaderObjectARB", (void*)&glCreateShaderObjectARB},
    {"glCreateShaderProgramEXT", (void*)&glCreateShaderProgramEXT},
    {"glCreateShaderProgramv", (void*)&glCreateShaderProgramv},
    {"glCreateShaderProgramvEXT", (void*)&glCreateShaderProgramvEXT},
    {"glCreateStatesNV", (void*)&glCreateStatesNV},
    {"glCreateTextures", (void*)&glCreateTextures},
    {"glCreateTransformFeedbacks", (void*)&glCreateTransformFeedbacks},
    {"glCreateVertexArrays", (void*)&glCreateVertexArrays},
    {"glCullFace", (void*)&glCullFace},
    {"glDebugMessageCallback", (void*)&glDebugMessageCallback},
    {"glDebugMessageCallbackARB", (void*)&glDebugMessageCallbackARB},
    {"glDebugMessageCallbackKHR", (void*)&glDebugMessageCallbackKHR},
    {"glDebugMessageControl", (void*)&glDebugMessageControl},
    {"glDebugMessageControlARB", (void*)&glDebugMessageControlARB},
    {"glDebugMessageControlKHR", (void*)&glDebugMessageControlKHR},
    {"glDebugMessageInsert", (void*)&glDebugMessageInsert},
    {"glDebugMessageInsertARB", (void*)&glDebugMessageInsertARB},
    {"glDebugMessageInsertKHR", (void*)&glDebugMessageInsertKHR},
    {"glDeleteBuffers", (void*)&glDeleteBuffers},
    {"glDeleteBuffersARB", (void*)&glDeleteBuffersARB},
    {"glDeleteCommandListsNV", (void*)&glDeleteCommandListsNV},
    {"glDeleteFramebuffers", (void*)&glDeleteFramebuffers},
    {"glDeleteFramebuffersEXT", (void*)&glDeleteFramebuffersEXT},
    {"glDeleteFramebuffersOES", (void*)&glDeleteFramebuffersOES},
    {"glDeleteLists", (void*)&glDeleteLists},
    {"glDeleteNamedStringARB", (void*)&glDeleteNamedStringARB},
    {"glDeleteObjectARB", (void*)&glDeleteObjectARB},
    {"glDeletePathsNV", (void*)&glDeletePathsNV},
    {"glDeletePerfMonitorsAMD", (void*)&glDeletePerfMonitorsAMD},
    {"glDeletePerfQueryINTEL", (void*)&glDeletePerfQueryINTEL},
    {"glDeleteProgram", (void*)&glDeleteProgram},
    {"glDeleteProgramPipelines", (void*)&glDeleteProgramPipelines},
    {"glDeleteProgramPipelinesEXT", (void*)&glDeleteProgramPipelinesEXT},
    {"glDeleteQueries", (void*)&glDeleteQueries},
    {"glDeleteQueriesARB", (void*)&glDeleteQueriesARB},
    {"glDeleteQueriesEXT", (void*)&glDeleteQueriesEXT},
    {"glDeleteRenderbuffers", (void*)&glDeleteRenderbuffers},
    {"glDeleteRenderbuffersEXT", (void*)&glDeleteRenderbuffersEXT},
    {"glDeleteRenderbuffersOES", (void*)&glDeleteRenderbuffersOES},
    {"glDeleteSamplers", (void*)&glDeleteSamplers},
    {"glDeleteShader", (void*)&glDeleteShader},
    {"glDeleteStatesNV", (void*)&glDeleteStatesNV},
    {"glDeleteSync", (void*)&glDeleteSync},
    {"glDeleteSyncAPPLE", (void*)&glDeleteSyncAPPLE},
    {"glDeleteTextures", (void*)&glDeleteTextures},
    {"glDeleteTexturesEXT", (void*)&glDeleteTexturesEXT},
    {"glDeleteTransformFeedbacks", (void*)&glDeleteTransformFeedbacks},
    {"glDeleteTransformFeedbacksNV", (void*)&glDeleteTransformFeedbacksNV},
    {"glDeleteVertexArrays", (void*)&glDeleteVertexArrays},
    {"glDeleteVertexArraysAPPLE", (void*)&glDeleteVertexArraysAPPLE},
    {"glDeleteVertexArraysOES", (void*)&glDeleteVertexArraysOES},
    {"glDepthBoundsdNV", (void*)&glDepthBoundsdNV},
    {"glDepthFunc", (void*)&glDepthFunc},
    {"glDepthMask", (void*)&glDepthMask},
    {"glDepthRange", (void*)&glDepthRange},
    {"glDepthRangeArraydvNV", (void*)&glDepthRangeArraydvNV},
    {"glDepthRangeArrayfvNV", (void*)&glDepthRangeArrayfvNV},
    {"glDepthRangeArrayfvOES", (void*)&glDepthRangeArrayfvOES},
    {"glDepthRangeArrayv", (void*)&glDepthRangeArrayv},
    {"glDepthRangeIndexed", (void*)&glDepthRangeIndexed},
    {"glDepthRangeIndexeddNV", (void*)&glDepthRangeIndexeddNV},
    {"glDepthRangeIndexedfOES", (void*)&glDepthRangeIndexedfOES},
    {"glDepthRangedNV", (void*)&glDepthRangedNV},
    {"glDepthRangef", (void*)&glDepthRangef},
    {"glDetachObjectARB", (void*)&glDetachObjectARB},
    {"glDetachShader", (void*)&glDetachShader},
    {"glDisable", (void*)&glDisable},
    {"glDisableClientState", (void*)&glDisableClientState},
    {"glDisableClientStateIndexedEXT", (void*)&glDisableClientStateIndexedEXT},
    {"glDisableClientStateiEXT", (void*)&glDisableClientStateiEXT},
    {"glDisableIndexedEXT", (void*)&glDisableIndexedEXT},
    {"glDisableVertexArrayAttrib", (void*)&glDisableVertexArrayAttrib},
    {"glDisableVertexArrayAttribEXT", (void*)&glDisableVertexArrayAttribEXT},
    {"glDisableVertexArrayEXT", (void*)&glDisableVertexArrayEXT},
    {"glDisableVertexAttribArray", (void*)&glDisableVertexAttribArray},
    {"glDisableVertexAttribArrayARB", (void*)&glDisableVertexAttribArrayARB},
    {"glDisablei", (void*)&glDisablei},
    {"glDisableiEXT", (void*)&glDisableiEXT},
    {"glDisableiNV", (void*)&glDisableiNV},
    {"glDisableiOES", (void*)&glDisableiOES},
    {"glDiscardFramebufferEXT", (void*)&glDiscardFramebufferEXT},
    {"glDispatchCompute", (void*)&glDispatchCompute},
    {"glDispatchComputeGroupSizeARB", (void*)&glDispatchComputeGroupSizeARB},
    {"glDispatchComputeIndirect", (void*)&glDispatchComputeIndirect},
    {"glDrawArrays", (void*)&glDrawArrays},
    {"glDrawArraysEXT", (void*)&glDrawArraysEXT},
    {"glDrawArraysIndirect", (void*)&glDrawArraysIndirect},
    {"glDrawArraysInstanced", (void*)&glDrawArraysInstanced},
    {"glDrawArraysInstancedANGLE", (void*)&glDrawArraysInstancedANGLE},
    {"glDrawArraysInstancedARB", (void*)&glDrawArraysInstancedARB},
    {"glDrawArraysInstancedBaseInstance", (void*)&glDrawArraysInstancedBaseInstance},
    {"glDrawArraysInstancedBaseInstanceEXT", (void*)&glDrawArraysInstancedBaseInstanceEXT},
    {"glDrawArraysInstancedEXT", (void*)&glDrawArraysInstancedEXT},
    {"glDrawArraysInstancedNV", (void*)&glDrawArraysInstancedNV},
    {"glDrawBuffer", (void*)&glDrawBuffer},
    {"glDrawBuffers", (void*)&glDrawBuffers},
    {"glDrawBuffersARB", (void*)&glDrawBuffersARB},
    {"glDrawBuffersATI", (void*)&glDrawBuffersATI},
    {"glDrawBuffersEXT", (void*)&glDrawBuffersEXT},
    {"glDrawBuffersNV", (void*)&glDrawBuffersNV},
    {"glDrawCommandsAddressNV", (void*)&glDrawCommandsAddressNV},
    {"glDrawCommandsNV", (void*)&glDrawCommandsNV},
    {"glDrawCommandsStatesAddressNV", (void*)&glDrawCommandsStatesAddressNV},
    {"glDrawCommandsStatesNV", (void*)&glDrawCommandsStatesNV},
    {"glDrawElements", (void*)&glDrawElements},
    {"glDrawElementsBaseVertex", (void*)&glDrawElementsBaseVertex},
    {"glDrawElementsBaseVertexEXT", (void*)&glDrawElementsBaseVertexEXT},
    {"glDrawElementsBaseVertexOES", (void*)&glDrawElementsBaseVertexOES},
    {"glDrawElementsIndirect", (void*)&glDrawElementsIndirect},
    {"glDrawElementsInstanced", (void*)&glDrawElementsInstanced},
    {"glDrawElementsInstancedANGLE", (void*)&glDrawElementsInstancedANGLE},
    {"glDrawElementsInstancedARB", (void*)&glDrawElementsInstancedARB},
    {"glDrawElementsInstancedBaseInstance", (void*)&glDrawElementsInstancedBaseInstance},
    {"glDrawElementsInstancedBaseInstanceEXT", (void*)&glDrawElementsInstancedBaseInstanceEXT},
    {"glDrawElementsInstancedBaseVertex", (void*)&glDrawElementsInstancedBaseVertex},
    {"glDrawElementsInstancedBaseVertexBaseInstance", (void*)&glDrawElementsInstancedBaseVertexBaseInstance},
    {"glDrawElementsInstancedBaseVertexBaseInstanceEXT", (void*)&glDrawElementsInstancedBaseVertexBaseInstanceEXT},
    {"glDrawElementsInstancedBaseVertexEXT", (void*)&glDrawElementsInstancedBaseVertexEXT},
    {"glDrawElementsInstancedBaseVertexOES", (void*)&glDrawElementsInstancedBaseVertexOES},
    {"glDrawElementsInstancedEXT", (void*)&glDrawElementsInstancedEXT},
    {"glDrawElementsInstancedNV", (void*)&glDrawElementsInstancedNV},
    {"glDrawPixels", (void*)&glDrawPixels},
    {"glDrawRangeElements", (void*)&glDrawRangeElements},
    {"glDrawRangeElementsBaseVertex", (void*)&glDrawRangeElementsBaseVertex},
    {"glDrawRangeElementsBaseVertexEXT", (void*)&glDrawRangeElementsBaseVertexEXT},
    {"glDrawRangeElementsBaseVertexOES", (void*)&glDrawRangeElementsBaseVertexOES},
    {"glDrawRangeElementsEXT", (void*)&glDrawRangeElementsEXT},
    {"glDrawTransformFeedback", (void*)&glDrawTransformFeedback},
    {"glDrawTransformFeedbackEXT", (void*)&glDrawTransformFeedbackEXT},
    {"glDrawTransformFeedbackInstanced", (void*)&glDrawTransformFeedbackInstanced},
    {"glDrawTransformFeedbackInstancedEXT", (void*)&glDrawTransformFeedbackInstancedEXT},
    {"glDrawTransformFeedbackNV", (void*)&glDrawTransformFeedbackNV},
    {"glDrawTransformFeedbackStream", (void*)&glDrawTransformFeedbackStream},
    {"glDrawTransformFeedbackStreamInstanced", (void*)&glDrawTransformFeedbackStreamInstanced},
    {"glEdgeFlag", (void*)&glEdgeFlag},
    {"glEdgeFlagFormatNV", (void*)&glEdgeFlagFormatNV},
    {"glEdgeFlagPointer", (void*)&glEdgeFlagPointer},
    {"glEdgeFlagv", (void*)&glEdgeFlagv},
    {"glEnable", (void*)&glEnable},
    {"glEnableClientState", (void*)&glEnableClientState},
    {"glEnableClientStateIndexedEXT", (void*)&glEnableClientStateIndexedEXT},
    {"glEnableClientStateiEXT", (void*)&glEnableClientStateiEXT},
    {"glEnableIndexedEXT", (void*)&glEnableIndexedEXT},
    {"glEnableVertexArrayAttrib", (void*)&glEnableVertexArrayAttrib},
    {"glEnableVertexArrayAttribEXT", (void*)&glEnableVertexArrayAttribEXT},
    {"glEnableVertexArrayEXT", (void*)&glEnableVertexArrayEXT},
    {"glEnableVertexAttribArray", (void*)&glEnableVertexAttribArray},
    {"glEnableVertexAttribArrayARB", (void*)&glEnableVertexAttribArrayARB},
    {"glEnablei", (void*)&glEnablei},
    {"glEnableiEXT", (void*)&glEnableiEXT},
    {"glEnableiNV", (void*)&glEnableiNV},
    {"glEnableiOES", (void*)&glEnableiOES},
    {"glEnd", (void*)&glEnd},
    {"glEndConditionalRender", (void*)&glEndConditionalRender},
    {"glEndConditionalRenderNV", (void*)&glEndConditionalRenderNV},
    {"glEndList", (void*)&glEndList},
    {"glEndPerfMonitorAMD", (void*)&glEndPerfMonitorAMD},
    {"glEndPerfQueryINTEL", (void*)&glEndPerfQueryINTEL},
    {"glEndQuery", (void*)&glEndQuery},
    {"glEndQueryARB", (void*)&glEndQueryARB},
    {"glEndQueryEXT", (void*)&glEndQueryEXT},
    {"glEndQueryIndexed", (void*)&glEndQueryIndexed},
    {"glEndTransformFeedback", (void*)&glEndTransformFeedback},
    {"glEndTransformFeedbackEXT", (void*)&glEndTransformFeedbackEXT},
    {"glEndTransformFeedbackNV", (void*)&glEndTransformFeedbackNV},
    {"glEvalCoord1d", (void*)&glEvalCoord1d},
    {"glEvalCoord1dv", (void*)&glEvalCoord1dv},
    {"glEvalCoord1f", (void*)&glEvalCoord1f},
    {"glEvalCoord1fv", (void*)&glEvalCoord1fv},
    {"glEvalCoord2d", (void*)&glEvalCoord2d},
    {"glEvalCoord2dv", (void*)&glEvalCoord2dv},
    {"glEvalCoord2f", (void*)&glEvalCoord2f},
    {"glEvalCoord2fv", (void*)&glEvalCoord2fv},
    {"glEvalMesh1", (void*)&glEvalMesh1},
    {"glEvalMesh2", (void*)&glEvalMesh2},
    {"glEvalPoint1", (void*)&glEvalPoint1},
    {"glEvalPoint2", (void*)&glEvalPoint2},
    {"glEvaluateDepthValuesARB", (void*)&glEvaluateDepthValuesARB},
    {"glFeedbackBuffer", (void*)&glFeedbackBuffer},
    {"glFenceSync", (void*)&glFenceSync},
    {"glFenceSyncAPPLE", (void*)&glFenceSyncAPPLE},
    {"glFinish", (void*)&glFinish},
    {"glFlush", (void*)&glFlush},
    {"glFlushMappedBufferRange", (void*)&glFlushMappedBufferRange},
    {"glFlushMappedBufferRangeAPPLE", (void*)&glFlushMappedBufferRangeAPPLE},
    {"glFlushMappedBufferRangeEXT", (void*)&glFlushMappedBufferRangeEXT},
    {"glFlushMappedNamedBufferRange", (void*)&glFlushMappedNamedBufferRange},
    {"glFlushMappedNamedBufferRangeEXT", (void*)&glFlushMappedNamedBufferRangeEXT},
    {"glFogCoordFormatNV", (void*)&glFogCoordFormatNV},
    {"glFogCoordPointer", (void*)&glFogCoordPointer},
    {"glFogCoordd", (void*)&glFogCoordd},
    {"glFogCoorddv", (void*)&glFogCoorddv},
    {"glFogCoordf", (void*)&glFogCoordf},
    {"glFogCoordfv", (void*)&glFogCoordfv},
    {"glFogf", (void*)&glFogf},
    {"glFogfv", (void*)&glFogfv},
    {"glFogi", (void*)&glFogi},
    {"glFogiv", (void*)&glFogiv},
    {"glFragmentCoverageColorNV", (void*)&glFragmentCoverageColorNV},
    {"glFramebufferDrawBufferEXT", (void*)&glFramebufferDrawBufferEXT},
    {"glFramebufferDrawBuffersEXT", (void*)&glFramebufferDrawBuffersEXT},
    {"glFramebufferFetchBarrierEXT", (void*)&glFramebufferFetchBarrierEXT},
    {"glFramebufferParameteri", (void*)&glFramebufferParameteri},
    {"glFramebufferParameteriMESA", (void*)&glFramebufferParameteriMESA},
    {"glFramebufferReadBufferEXT", (void*)&glFramebufferReadBufferEXT},
    {"glFramebufferRenderbuffer", (void*)&glFramebufferRenderbuffer},
    {"glFramebufferRenderbufferEXT", (void*)&glFramebufferRenderbufferEXT},
    {"glFramebufferRenderbufferOES", (void*)&glFramebufferRenderbufferOES},
    {"glFramebufferSampleLocationsfvARB", (void*)&glFramebufferSampleLocationsfvARB},
    {"glFramebufferSampleLocationsfvNV", (void*)&glFramebufferSampleLocationsfvNV},
    {"glFramebufferShadingRateEXT", (void*)&glFramebufferShadingRateEXT},
    {"glFramebufferTexture", (void*)&glFramebufferTexture},
    {"glFramebufferTexture1D", (void*)&glFramebufferTexture1D},
    {"glFramebufferTexture1DEXT", (void*)&glFramebufferTexture1DEXT},
    {"glFramebufferTexture2D", (void*)&glFramebufferTexture2D},
    {"glFramebufferTexture2DEXT", (void*)&glFramebufferTexture2DEXT},
    {"glFramebufferTexture2DMultisampleEXT", (void*)&glFramebufferTexture2DMultisampleEXT},
    {"glFramebufferTexture2DOES", (void*)&glFramebufferTexture2DOES},
    {"glFramebufferTexture3D", (void*)&glFramebufferTexture3D},
    {"glFramebufferTexture3DEXT", (void*)&glFramebufferTexture3DEXT},
    {"glFramebufferTexture3DOES", (void*)&glFramebufferTexture3DOES},
    {"glFramebufferTextureARB", (void*)&glFramebufferTextureARB},
    {"glFramebufferTextureEXT", (void*)&glFramebufferTextureEXT},
    {"glFramebufferTextureFaceARB", (void*)&glFramebufferTextureFaceARB},
    {"glFramebufferTextureFaceEXT", (void*)&glFramebufferTextureFaceEXT},
    {"glFramebufferTextureLayer", (void*)&glFramebufferTextureLayer},
    {"glFramebufferTextureLayerARB", (void*)&glFramebufferTextureLayerARB},
    {"glFramebufferTextureLayerEXT", (void*)&glFramebufferTextureLayerEXT},
    {"glFramebufferTextureMultiviewOVR", (void*)&glFramebufferTextureMultiviewOVR},
    {"glFramebufferTextureOES", (void*)&glFramebufferTextureOES},
    {"glFrontFace", (void*)&glFrontFace},
    {"glFrustum", (void*)&glFrustum},
    {"glGenBuffers", (void*)&glGenBuffers},
    {"glGenBuffersARB", (void*)&glGenBuffersARB},
    {"glGenFramebuffers", (void*)&glGenFramebuffers},
    {"glGenFramebuffersEXT", (void*)&glGenFramebuffersEXT},
    {"glGenFramebuffersOES", (void*)&glGenFramebuffersOES},
    {"glGenLists", (void*)&glGenLists},
    {"glGenPathsNV", (void*)&glGenPathsNV},
    {"glGenPerfMonitorsAMD", (void*)&glGenPerfMonitorsAMD},
    {"glGenProgramPipelines", (void*)&glGenProgramPipelines},
    {"glGenProgramPipelinesEXT", (void*)&glGenProgramPipelinesEXT},
    {"glGenQueries", (void*)&glGenQueries},
    {"glGenQueriesARB", (void*)&glGenQueriesARB},
    {"glGenQueriesEXT", (void*)&glGenQueriesEXT},
    {"glGenRenderbuffers", (void*)&glGenRenderbuffers},
    {"glGenRenderbuffersEXT", (void*)&glGenRenderbuffersEXT},
    {"glGenRenderbuffersOES", (void*)&glGenRenderbuffersOES},
    {"glGenSamplers", (void*)&glGenSamplers},
    {"glGenTextures", (void*)&glGenTextures},
    {"glGenTexturesEXT", (void*)&glGenTexturesEXT},
    {"glGenTransformFeedbacks", (void*)&glGenTransformFeedbacks},
    {"glGenTransformFeedbacksNV", (void*)&glGenTransformFeedbacksNV},
    {"glGenVertexArrays", (void*)&glGenVertexArrays},
    {"glGenVertexArraysAPPLE", (void*)&glGenVertexArraysAPPLE},
    {"glGenVertexArraysOES", (void*)&glGenVertexArraysOES},
    {"glGenerateMipmap", (void*)&glGenerateMipmap},
    {"glGenerateMipmapEXT", (void*)&glGenerateMipmapEXT},
    {"glGenerateMipmapOES", (void*)&glGenerateMipmapOES},
    {"glGenerateMultiTexMipmapEXT", (void*)&glGenerateMultiTexMipmapEXT},
    {"glGenerateTextureMipmap", (void*)&glGenerateTextureMipmap},
    {"glGenerateTextureMipmapEXT", (void*)&glGenerateTextureMipmapEXT},
    {"glGetActiveAtomicCounterBufferiv", (void*)&glGetActiveAtomicCounterBufferiv},
    {"glGetActiveAttrib", (void*)&glGetActiveAttrib},
    {"glGetActiveAttribARB", (void*)&glGetActiveAttribARB},
    {"glGetActiveSubroutineName", (void*)&glGetActiveSubroutineName},
    {"glGetActiveSubroutineUniformName", (void*)&glGetActiveSubroutineUniformName},
    {"glGetActiveSubroutineUniformiv", (void*)&glGetActiveSubroutineUniformiv},
    {"glGetActiveUniform", (void*)&glGetActiveUniform},
    {"glGetActiveUniformARB", (void*)&glGetActiveUniformARB},
    {"glGetActiveUniformBlockName", (void*)&glGetActiveUniformBlockName},
    {"glGetActiveUniformBlockiv", (void*)&glGetActiveUniformBlockiv},
    {"glGetActiveUniformName", (void*)&glGetActiveUniformName},
    {"glGetActiveUniformsiv", (void*)&glGetActiveUniformsiv},
    {"glGetAttachedObjectsARB", (void*)&glGetAttachedObjectsARB},
    {"glGetAttachedShaders", (void*)&glGetAttachedShaders},
    {"glGetAttribLocation", (void*)&glGetAttribLocation},
    {"glGetAttribLocationARB", (void*)&glGetAttribLocationARB},
    {"glGetBooleanIndexedvEXT", (void*)&glGetBooleanIndexedvEXT},
    {"glGetBooleani_v", (void*)&glGetBooleani_v},
    {"glGetBooleanv", (void*)&glGetBooleanv},
    {"glGetBufferParameteri64v", (void*)&glGetBufferParameteri64v},
    {"glGetBufferParameteriv", (void*)&glGetBufferParameteriv},
    {"glGetBufferParameterivARB", (void*)&glGetBufferParameterivARB},
    {"glGetBufferParameterui64vNV", (void*)&glGetBufferParameterui64vNV},
    {"glGetBufferPointerv", (void*)&glGetBufferPointerv},
    {"glGetBufferPointervARB", (void*)&glGetBufferPointervARB},
    {"glGetBufferPointervOES", (void*)&glGetBufferPointervOES},
    {"glGetBufferSubData", (void*)&glGetBufferSubData},
    {"glGetBufferSubDataARB", (void*)&glGetBufferSubDataARB},
    {"glGetClipPlane", (void*)&glGetClipPlane},
    {"glGetColorTable", (void*)&glGetColorTable},
    {"glGetColorTableParameterfv", (void*)&glGetColorTableParameterfv},
    {"glGetColorTableParameteriv", (void*)&glGetColorTableParameteriv},
    {"glGetCommandHeaderNV", (void*)&glGetCommandHeaderNV},
    {"glGetCompressedMultiTexImageEXT", (void*)&glGetCompressedMultiTexImageEXT},
    {"glGetCompressedTexImage", (void*)&glGetCompressedTexImage},
    {"glGetCompressedTexImageARB", (void*)&glGetCompressedTexImageARB},
    {"glGetCompressedTextureImage", (void*)&glGetCompressedTextureImage},
    {"glGetCompressedTextureImageEXT", (void*)&glGetCompressedTextureImageEXT},
    {"glGetCompressedTextureSubImage", (void*)&glGetCompressedTextureSubImage},
    {"glGetConvolutionFilter", (void*)&glGetConvolutionFilter},
    {"glGetConvolutionParameterfv", (void*)&glGetConvolutionParameterfv},
    {"glGetConvolutionParameteriv", (void*)&glGetConvolutionParameteriv},
    {"glGetCoverageModulationTableNV", (void*)&glGetCoverageModulationTableNV},
    {"glGetDebugMessageLog", (void*)&glGetDebugMessageLog},
    {"glGetDebugMessageLogARB", (void*)&glGetDebugMessageLogARB},
    {"glGetDebugMessageLogKHR", (void*)&glGetDebugMessageLogKHR},
    {"glGetDoubleIndexedvEXT", (void*)&glGetDoubleIndexedvEXT},
    {"glGetDoublei_v", (void*)&glGetDoublei_v},
    {"glGetDoublei_vEXT", (void*)&glGetDoublei_vEXT},
    {"glGetDoublev", (void*)&glGetDoublev},
    {"glGetError", (void*)&glGetError},
    {"glGetFirstPerfQueryIdINTEL", (void*)&glGetFirstPerfQueryIdINTEL},
    {"glGetFloatIndexedvEXT", (void*)&glGetFloatIndexedvEXT},
    {"glGetFloati_v", (void*)&glGetFloati_v},
    {"glGetFloati_vEXT", (void*)&glGetFloati_vEXT},
    {"glGetFloati_vNV", (void*)&glGetFloati_vNV},
    {"glGetFloati_vOES", (void*)&glGetFloati_vOES},
    {"glGetFloatv", (void*)&glGetFloatv},
    {"glGetFragDataIndex", (void*)&glGetFragDataIndex},
    {"glGetFragDataIndexEXT", (void*)&glGetFragDataIndexEXT},
    {"glGetFragDataLocation", (void*)&glGetFragDataLocation},
    {"glGetFragDataLocationEXT", (void*)&glGetFragDataLocationEXT},
    {"glGetFragmentShadingRatesEXT", (void*)&glGetFragmentShadingRatesEXT},
    {"glGetFramebufferAttachmentParameteriv", (void*)&glGetFramebufferAttachmentParameteriv},
    {"glGetFramebufferAttachmentParameterivEXT", (void*)&glGetFramebufferAttachmentParameterivEXT},
    {"glGetFramebufferAttachmentParameterivOES", (void*)&glGetFramebufferAttachmentParameterivOES},
    {"glGetFramebufferParameteriv", (void*)&glGetFramebufferParameteriv},
    {"glGetFramebufferParameterivEXT", (void*)&glGetFramebufferParameterivEXT},
    {"glGetFramebufferParameterivMESA", (void*)&glGetFramebufferParameterivMESA},
    {"glGetGraphicsResetStatus", (void*)&glGetGraphicsResetStatus},
    {"glGetGraphicsResetStatusARB", (void*)&glGetGraphicsResetStatusARB},
    {"glGetGraphicsResetStatusEXT", (void*)&glGetGraphicsResetStatusEXT},
    {"glGetGraphicsResetStatusKHR", (void*)&glGetGraphicsResetStatusKHR},
    {"glGetHandleARB", (void*)&glGetHandleARB},
    {"glGetHistogram", (void*)&glGetHistogram},
    {"glGetHistogramParameterfv", (void*)&glGetHistogramParameterfv},
    {"glGetHistogramParameteriv", (void*)&glGetHistogramParameteriv},
    {"glGetImageHandleARB", (void*)&glGetImageHandleARB},
    {"glGetImageHandleNV", (void*)&glGetImageHandleNV},
    {"glGetInfoLogARB", (void*)&glGetInfoLogARB},
    {"glGetInteger64i_v", (void*)&glGetInteger64i_v},
    {"glGetInteger64v", (void*)&glGetInteger64v},
    {"glGetInteger64vAPPLE", (void*)&glGetInteger64vAPPLE},
    {"glGetInteger64vEXT", (void*)&glGetInteger64vEXT},
    {"glGetIntegerIndexedvEXT", (void*)&glGetIntegerIndexedvEXT},
    {"glGetIntegeri_v", (void*)&glGetIntegeri_v},
    {"glGetIntegeri_vEXT", (void*)&glGetIntegeri_vEXT},
    {"glGetIntegerui64i_vNV", (void*)&glGetIntegerui64i_vNV},
    {"glGetIntegerui64vNV", (void*)&glGetIntegerui64vNV},
    {"glGetIntegerv", (void*)&glGetIntegerv},
    {"glGetInternalformatSampleivNV", (void*)&glGetInternalformatSampleivNV},
    {"glGetInternalformati64v", (void*)&glGetInternalformati64v},
    {"glGetInternalformativ", (void*)&glGetInternalformativ},
    {"glGetLightfv", (void*)&glGetLightfv},
    {"glGetLightiv", (void*)&glGetLightiv},
    {"glGetMapdv", (void*)&glGetMapdv},
    {"glGetMapfv", (void*)&glGetMapfv},
    {"glGetMapiv", (void*)&glGetMapiv},
    {"glGetMaterialfv", (void*)&glGetMaterialfv},
    {"glGetMaterialiv", (void*)&glGetMaterialiv},
    {"glGetMemoryObjectDetachedResourcesuivNV", (void*)&glGetMemoryObjectDetachedResourcesuivNV},
    {"glGetMinmax", (void*)&glGetMinmax},
    {"glGetMinmaxParameterfv", (void*)&glGetMinmaxParameterfv},
    {"glGetMinmaxParameteriv", (void*)&glGetMinmaxParameteriv},
    {"glGetMultiTexEnvfvEXT", (void*)&glGetMultiTexEnvfvEXT},
    {"glGetMultiTexEnvivEXT", (void*)&glGetMultiTexEnvivEXT},
    {"glGetMultiTexGendvEXT", (void*)&glGetMultiTexGendvEXT},
    {"glGetMultiTexGenfvEXT", (void*)&glGetMultiTexGenfvEXT},
    {"glGetMultiTexGenivEXT", (void*)&glGetMultiTexGenivEXT},
    {"glGetMultiTexImageEXT", (void*)&glGetMultiTexImageEXT},
    {"glGetMultiTexLevelParameterfvEXT", (void*)&glGetMultiTexLevelParameterfvEXT},
    {"glGetMultiTexLevelParameterivEXT", (void*)&glGetMultiTexLevelParameterivEXT},
    {"glGetMultiTexParameterIivEXT", (void*)&glGetMultiTexParameterIivEXT},
    {"glGetMultiTexParameterIuivEXT", (void*)&glGetMultiTexParameterIuivEXT},
    {"glGetMultiTexParameterfvEXT", (void*)&glGetMultiTexParameterfvEXT},
    {"glGetMultiTexParameterivEXT", (void*)&glGetMultiTexParameterivEXT},
    {"glGetMultisamplefv", (void*)&glGetMultisamplefv},
    {"glGetMultisamplefvNV", (void*)&glGetMultisamplefvNV},
    {"glGetNamedBufferParameteri64v", (void*)&glGetNamedBufferParameteri64v},
    {"glGetNamedBufferParameteriv", (void*)&glGetNamedBufferParameteriv},
    {"glGetNamedBufferParameterivEXT", (void*)&glGetNamedBufferParameterivEXT},
    {"glGetNamedBufferParameterui64vNV", (void*)&glGetNamedBufferParameterui64vNV},
    {"glGetNamedBufferPointerv", (void*)&glGetNamedBufferPointerv},
    {"glGetNamedBufferPointervEXT", (void*)&glGetNamedBufferPointervEXT},
    {"glGetNamedBufferSubData", (void*)&glGetNamedBufferSubData},
    {"glGetNamedBufferSubDataEXT", (void*)&glGetNamedBufferSubDataEXT},
    {"glGetNamedFramebufferAttachmentParameteriv", (void*)&glGetNamedFramebufferAttachmentParameteriv},
    {"glGetNamedFramebufferAttachmentParameterivEXT", (void*)&glGetNamedFramebufferAttachmentParameterivEXT},
    {"glGetNamedFramebufferParameteriv", (void*)&glGetNamedFramebufferParameteriv},
    {"glGetNamedFramebufferParameterivEXT", (void*)&glGetNamedFramebufferParameterivEXT},
    {"glGetNamedProgramLocalParameterIivEXT", (void*)&glGetNamedProgramLocalParameterIivEXT},
    {"glGetNamedProgramLocalParameterIuivEXT", (void*)&glGetNamedProgramLocalParameterIuivEXT},
    {"glGetNamedProgramLocalParameterdvEXT", (void*)&glGetNamedProgramLocalParameterdvEXT},
    {"glGetNamedProgramLocalParameterfvEXT", (void*)&glGetNamedProgramLocalParameterfvEXT},
    {"glGetNamedProgramStringEXT", (void*)&glGetNamedProgramStringEXT},
    {"glGetNamedProgramivEXT", (void*)&glGetNamedProgramivEXT},
    {"glGetNamedRenderbufferParameteriv", (void*)&glGetNamedRenderbufferParameteriv},
    {"glGetNamedRenderbufferParameterivEXT", (void*)&glGetNamedRenderbufferParameterivEXT},
    {"glGetNamedStringARB", (void*)&glGetNamedStringARB},
    {"glGetNamedStringivARB", (void*)&glGetNamedStringivARB},
    {"glGetNextPerfQueryIdINTEL", (void*)&glGetNextPerfQueryIdINTEL},
    {"glGetObjectLabel", (void*)&glGetObjectLabel},
    {"glGetObjectLabelEXT", (void*)&glGetObjectLabelEXT},
    {"glGetObjectLabelKHR", (void*)&glGetObjectLabelKHR},
    {"glGetObjectParameterfvARB", (void*)&glGetObjectParameterfvARB},
    {"glGetObjectParameterivARB", (void*)&glGetObjectParameterivARB},
    {"glGetObjectPtrLabel", (void*)&glGetObjectPtrLabel},
    {"glGetObjectPtrLabelKHR", (void*)&glGetObjectPtrLabelKHR},
    {"glGetPathColorGenfvNV", (void*)&glGetPathColorGenfvNV},
    {"glGetPathColorGenivNV", (void*)&glGetPathColorGenivNV},
    {"glGetPathCommandsNV", (void*)&glGetPathCommandsNV},
    {"glGetPathCoordsNV", (void*)&glGetPathCoordsNV},
    {"glGetPathDashArrayNV", (void*)&glGetPathDashArrayNV},
    {"glGetPathLengthNV", (void*)&glGetPathLengthNV},
    {"glGetPathMetricRangeNV", (void*)&glGetPathMetricRangeNV},
    {"glGetPathMetricsNV", (void*)&glGetPathMetricsNV},
    {"glGetPathParameterfvNV", (void*)&glGetPathParameterfvNV},
    {"glGetPathParameterivNV", (void*)&glGetPathParameterivNV},
    {"glGetPathSpacingNV", (void*)&glGetPathSpacingNV},
    {"glGetPathTexGenfvNV", (void*)&glGetPathTexGenfvNV},
    {"glGetPathTexGenivNV", (void*)&glGetPathTexGenivNV},
    {"glGetPerfCounterInfoINTEL", (void*)&glGetPerfCounterInfoINTEL},
    {"glGetPerfMonitorCounterDataAMD", (void*)&glGetPerfMonitorCounterDataAMD},
    {"glGetPerfMonitorCounterInfoAMD", (void*)&glGetPerfMonitorCounterInfoAMD},
    {"glGetPerfMonitorCounterStringAMD", (void*)&glGetPerfMonitorCounterStringAMD},
    {"glGetPerfMonitorCountersAMD", (void*)&glGetPerfMonitorCountersAMD},
    {"glGetPerfMonitorGroupStringAMD", (void*)&glGetPerfMonitorGroupStringAMD},
    {"glGetPerfMonitorGroupsAMD", (void*)&glGetPerfMonitorGroupsAMD},
    {"glGetPerfQueryDataINTEL", (void*)&glGetPerfQueryDataINTEL},
    {"glGetPerfQueryIdByNameINTEL", (void*)&glGetPerfQueryIdByNameINTEL},
    {"glGetPerfQueryInfoINTEL", (void*)&glGetPerfQueryInfoINTEL},
    {"glGetPixelMapfv", (void*)&glGetPixelMapfv},
    {"glGetPixelMapuiv", (void*)&glGetPixelMapuiv},
    {"glGetPixelMapusv", (void*)&glGetPixelMapusv},
    {"glGetPointerIndexedvEXT", (void*)&glGetPointerIndexedvEXT},
    {"glGetPointeri_vEXT", (void*)&glGetPointeri_vEXT},
    {"glGetPointerv", (void*)&glGetPointerv},
    {"glGetPointervEXT", (void*)&glGetPointervEXT},
    {"glGetPointervKHR", (void*)&glGetPointervKHR},
    {"glGetPolygonStipple", (void*)&glGetPolygonStipple},
    {"glGetProgramBinary", (void*)&glGetProgramBinary},
    {"glGetProgramBinaryOES", (void*)&glGetProgramBinaryOES},
    {"glGetProgramInfoLog", (void*)&glGetProgramInfoLog},
    {"glGetProgramInterfaceiv", (void*)&glGetProgramInterfaceiv},
    {"glGetProgramPipelineInfoLog", (void*)&glGetProgramPipelineInfoLog},
    {"glGetProgramPipelineInfoLogEXT", (void*)&glGetProgramPipelineInfoLogEXT},
    {"glGetProgramPipelineiv", (void*)&glGetProgramPipelineiv},
    {"glGetProgramPipelineivEXT", (void*)&glGetProgramPipelineivEXT},
    {"glGetProgramResourceIndex", (void*)&glGetProgramResourceIndex},
    {"glGetProgramResourceLocation", (void*)&glGetProgramResourceLocation},
    {"glGetProgramResourceLocationIndex", (void*)&glGetProgramResourceLocationIndex},
    {"glGetProgramResourceLocationIndexEXT", (void*)&glGetProgramResourceLocationIndexEXT},
    {"glGetProgramResourceName", (void*)&glGetProgramResourceName},
    {"glGetProgramResourcefvNV", (void*)&glGetProgramResourcefvNV},
    {"glGetProgramResourceiv", (void*)&glGetProgramResourceiv},
    {"glGetProgramStageiv", (void*)&glGetProgramStageiv},
    {"glGetProgramiv", (void*)&glGetProgramiv},
    {"glGetProgramivNV", (void*)&glGetProgramivNV},
    {"glGetQueryBufferObjecti64v", (void*)&glGetQueryBufferObjecti64v},
    {"glGetQueryBufferObjectiv", (void*)&glGetQueryBufferObjectiv},
    {"glGetQueryBufferObjectui64v", (void*)&glGetQueryBufferObjectui64v},
    {"glGetQueryBufferObjectuiv", (void*)&glGetQueryBufferObjectuiv},
    {"glGetQueryIndexediv", (void*)&glGetQueryIndexediv},
    {"glGetQueryObjecti64v", (void*)&glGetQueryObjecti64v},
    {"glGetQueryObjecti64vEXT", (void*)&glGetQueryObjecti64vEXT},
    {"glGetQueryObjectiv", (void*)&glGetQueryObjectiv},
    {"glGetQueryObjectivARB", (void*)&glGetQueryObjectivARB},
    {"glGetQueryObjectivEXT", (void*)&glGetQueryObjectivEXT},
    {"glGetQueryObjectui64v", (void*)&glGetQueryObjectui64v},
    {"glGetQueryObjectui64vEXT", (void*)&glGetQueryObjectui64vEXT},
    {"glGetQueryObjectuiv", (void*)&glGetQueryObjectuiv},
    {"glGetQueryObjectuivARB", (void*)&glGetQueryObjectuivARB},
    {"glGetQueryObjectuivEXT", (void*)&glGetQueryObjectuivEXT},
    {"glGetQueryiv", (void*)&glGetQueryiv},
    {"glGetQueryivARB", (void*)&glGetQueryivARB},
    {"glGetQueryivEXT", (void*)&glGetQueryivEXT},
    {"glGetRenderbufferParameteriv", (void*)&glGetRenderbufferParameteriv},
    {"glGetRenderbufferParameterivEXT", (void*)&glGetRenderbufferParameterivEXT},
    {"glGetRenderbufferParameterivOES", (void*)&glGetRenderbufferParameterivOES},
    {"glGetSamplerParameterIiv", (void*)&glGetSamplerParameterIiv},
    {"glGetSamplerParameterIivEXT", (void*)&glGetSamplerParameterIivEXT},
    {"glGetSamplerParameterIivOES", (void*)&glGetSamplerParameterIivOES},
    {"glGetSamplerParameterIuiv", (void*)&glGetSamplerParameterIuiv},
    {"glGetSamplerParameterIuivEXT", (void*)&glGetSamplerParameterIuivEXT},
    {"glGetSamplerParameterIuivOES", (void*)&glGetSamplerParameterIuivOES},
    {"glGetSamplerParameterfv", (void*)&glGetSamplerParameterfv},
    {"glGetSamplerParameteriv", (void*)&glGetSamplerParameteriv},
    {"glGetSeparableFilter", (void*)&glGetSeparableFilter},
    {"glGetShaderInfoLog", (void*)&glGetShaderInfoLog},
    {"glGetShaderPrecisionFormat", (void*)&glGetShaderPrecisionFormat},
    {"glGetShaderSource", (void*)&glGetShaderSource},
    {"glGetShaderSourceARB", (void*)&glGetShaderSourceARB},
    {"glGetShaderiv", (void*)&glGetShaderiv},
    {"glGetShadingRateImagePaletteNV", (void*)&glGetShadingRateImagePaletteNV},
    {"glGetShadingRateSampleLocationivNV", (void*)&glGetShadingRateSampleLocationivNV},
    {"glGetStageIndexNV", (void*)&glGetStageIndexNV},
    {"glGetString", (void*)&glGetString},
    {"glGetStringi", (void*)&glGetStringi},
    {"glGetSubroutineIndex", (void*)&glGetSubroutineIndex},
    {"glGetSubroutineUniformLocation", (void*)&glGetSubroutineUniformLocation},
    {"glGetSynciv", (void*)&glGetSynciv},
    {"glGetSyncivAPPLE", (void*)&glGetSyncivAPPLE},
    {"glGetTexEnvfv", (void*)&glGetTexEnvfv},
    {"glGetTexEnviv", (void*)&glGetTexEnviv},
    {"glGetTexGendv", (void*)&glGetTexGendv},
    {"glGetTexGenfv", (void*)&glGetTexGenfv},
    {"glGetTexGeniv", (void*)&glGetTexGeniv},
    {"glGetTexImage", (void*)&glGetTexImage},
    {"glGetTexLevelParameterfv", (void*)&glGetTexLevelParameterfv},
    {"glGetTexLevelParameteriv", (void*)&glGetTexLevelParameteriv},
    {"glGetTexParameterIiv", (void*)&glGetTexParameterIiv},
    {"glGetTexParameterIivEXT", (void*)&glGetTexParameterIivEXT},
    {"glGetTexParameterIivOES", (void*)&glGetTexParameterIivOES},
    {"glGetTexParameterIuiv", (void*)&glGetTexParameterIuiv},
    {"glGetTexParameterIuivEXT", (void*)&glGetTexParameterIuivEXT},
    {"glGetTexParameterIuivOES", (void*)&glGetTexParameterIuivOES},
    {"glGetTexParameterfv", (void*)&glGetTexParameterfv},
    {"glGetTexParameteriv", (void*)&glGetTexParameteriv},
    {"glGetTextureHandleARB", (void*)&glGetTextureHandleARB},
    {"glGetTextureHandleNV", (void*)&glGetTextureHandleNV},
    {"glGetTextureImage", (void*)&glGetTextureImage},
    {"glGetTextureImageEXT", (void*)&glGetTextureImageEXT},
    {"glGetTextureLevelParameterfv", (void*)&glGetTextureLevelParameterfv},
    {"glGetTextureLevelParameterfvEXT", (void*)&glGetTextureLevelParameterfvEXT},
    {"glGetTextureLevelParameteriv", (void*)&glGetTextureLevelParameteriv},
    {"glGetTextureLevelParameterivEXT", (void*)&glGetTextureLevelParameterivEXT},
    {"glGetTextureParameterIiv", (void*)&glGetTextureParameterIiv},
    {"glGetTextureParameterIivEXT", (void*)&glGetTextureParameterIivEXT},
    {"glGetTextureParameterIuiv", (void*)&glGetTextureParameterIuiv},
    {"glGetTextureParameterIuivEXT", (void*)&glGetTextureParameterIuivEXT},
    {"glGetTextureParameterfv", (void*)&glGetTextureParameterfv},
    {"glGetTextureParameterfvEXT", (void*)&glGetTextureParameterfvEXT},
    {"glGetTextureParameteriv", (void*)&glGetTextureParameteriv},
    {"glGetTextureParameterivEXT", (void*)&glGetTextureParameterivEXT},
    {"glGetTextureSamplerHandleARB", (void*)&glGetTextureSamplerHandleARB},
    {"glGetTextureSamplerHandleNV", (void*)&glGetTextureSamplerHandleNV},
    {"glGetTextureSubImage", (void*)&glGetTextureSubImage},
    {"glGetTransformFeedbackVarying", (void*)&glGetTransformFeedbackVarying},
    {"glGetTransformFeedbackVaryingEXT", (void*)&glGetTransformFeedbackVaryingEXT},
    {"glGetTransformFeedbacki64_v", (void*)&glGetTransformFeedbacki64_v},
    {"glGetTransformFeedbacki_v", (void*)&glGetTransformFeedbacki_v},
    {"glGetTransformFeedbackiv", (void*)&glGetTransformFeedbackiv},
    {"glGetUniformBlockIndex", (void*)&glGetUniformBlockIndex},
    {"glGetUniformIndices", (void*)&glGetUniformIndices},
    {"glGetUniformLocation", (void*)&glGetUniformLocation},
    {"glGetUniformLocationARB", (void*)&glGetUniformLocationARB},
    {"glGetUniformSubroutineuiv", (void*)&glGetUniformSubroutineuiv},
    {"glGetUniformdv", (void*)&glGetUniformdv},
    {"glGetUniformfv", (void*)&glGetUniformfv},
    {"glGetUniformfvARB", (void*)&glGetUniformfvARB},
    {"glGetUniformi64vARB", (void*)&glGetUniformi64vARB},
    {"glGetUniformi64vNV", (void*)&glGetUniformi64vNV},
    {"glGetUniformiv", (void*)&glGetUniformiv},
    {"glGetUniformivARB", (void*)&glGetUniformivARB},
    {"glGetUniformui64vARB", (void*)&glGetUniformui64vARB},
    {"glGetUniformui64vNV", (void*)&glGetUniformui64vNV},
    {"glGetUniformuiv", (void*)&glGetUniformuiv},
    {"glGetUniformuivEXT", (void*)&glGetUniformuivEXT},
    {"glGetVertexArrayIndexed64iv", (void*)&glGetVertexArrayIndexed64iv},
    {"glGetVertexArrayIndexediv", (void*)&glGetVertexArrayIndexediv},
    {"glGetVertexArrayIntegeri_vEXT", (void*)&glGetVertexArrayIntegeri_vEXT},
    {"glGetVertexArrayIntegervEXT", (void*)&glGetVertexArrayIntegervEXT},
    {"glGetVertexArrayPointeri_vEXT", (void*)&glGetVertexArrayPointeri_vEXT},
    {"glGetVertexArrayPointervEXT", (void*)&glGetVertexArrayPointervEXT},
    {"glGetVertexArrayiv", (void*)&glGetVertexArrayiv},
    {"glGetVertexAttribIiv", (void*)&glGetVertexAttribIiv},
    {"glGetVertexAttribIivEXT", (void*)&glGetVertexAttribIivEXT},
    {"glGetVertexAttribIuiv", (void*)&glGetVertexAttribIuiv},
    {"glGetVertexAttribIuivEXT", (void*)&glGetVertexAttribIuivEXT},
    {"glGetVertexAttribLdv", (void*)&glGetVertexAttribLdv},
    {"glGetVertexAttribLdvEXT", (void*)&glGetVertexAttribLdvEXT},
    {"glGetVertexAttribLi64vNV", (void*)&glGetVertexAttribLi64vNV},
    {"glGetVertexAttribLui64vARB", (void*)&glGetVertexAttribLui64vARB},
    {"glGetVertexAttribLui64vNV", (void*)&glGetVertexAttribLui64vNV},
    {"glGetVertexAttribPointerv", (void*)&glGetVertexAttribPointerv},
    {"glGetVertexAttribPointervARB", (void*)&glGetVertexAttribPointervARB},
    {"glGetVertexAttribPointervNV", (void*)&glGetVertexAttribPointervNV},
    {"glGetVertexAttribdv", (void*)&glGetVertexAttribdv},
    {"glGetVertexAttribdvARB", (void*)&glGetVertexAttribdvARB},
    {"glGetVertexAttribdvNV", (void*)&glGetVertexAttribdvNV},
    {"glGetVertexAttribfv", (void*)&glGetVertexAttribfv},
    {"glGetVertexAttribfvARB", (void*)&glGetVertexAttribfvARB},
    {"glGetVertexAttribfvNV", (void*)&glGetVertexAttribfvNV},
    {"glGetVertexAttribiv", (void*)&glGetVertexAttribiv},
    {"glGetVertexAttribivARB", (void*)&glGetVertexAttribivARB},
    {"glGetVertexAttribivNV", (void*)&glGetVertexAttribivNV},
    {"glGetnColorTable", (void*)&glGetnColorTable},
    {"glGetnColorTableARB", (void*)&glGetnColorTableARB},
    {"glGetnCompressedTexImage", (void*)&glGetnCompressedTexImage},
    {"glGetnCompressedTexImageARB", (void*)&glGetnCompressedTexImageARB},
    {"glGetnConvolutionFilter", (void*)&glGetnConvolutionFilter},
    {"glGetnConvolutionFilterARB", (void*)&glGetnConvolutionFilterARB},
    {"glGetnHistogram", (void*)&glGetnHistogram},
    {"glGetnHistogramARB", (void*)&glGetnHistogramARB},
    {"glGetnMapdv", (void*)&glGetnMapdv},
    {"glGetnMapdvARB", (void*)&glGetnMapdvARB},
    {"glGetnMapfv", (void*)&glGetnMapfv},
    {"glGetnMapfvARB", (void*)&glGetnMapfvARB},
    {"glGetnMapiv", (void*)&glGetnMapiv},
    {"glGetnMapivARB", (void*)&glGetnMapivARB},
    {"glGetnMinmax", (void*)&glGetnMinmax},
    {"glGetnMinmaxARB", (void*)&glGetnMinmaxARB},
    {"glGetnPixelMapfv", (void*)&glGetnPixelMapfv},
    {"glGetnPixelMapfvARB", (void*)&glGetnPixelMapfvARB},
    {"glGetnPixelMapuiv", (void*)&glGetnPixelMapuiv},
    {"glGetnPixelMapuivARB", (void*)&glGetnPixelMapuivARB},
    {"glGetnPixelMapusv", (void*)&glGetnPixelMapusv},
    {"glGetnPixelMapusvARB", (void*)&glGetnPixelMapusvARB},
    {"glGetnPolygonStipple", (void*)&glGetnPolygonStipple},
    {"glGetnPolygonStippleARB", (void*)&glGetnPolygonStippleARB},
    {"glGetnSeparableFilter", (void*)&glGetnSeparableFilter},
    {"glGetnSeparableFilterARB", (void*)&glGetnSeparableFilterARB},
    {"glGetnTexImage", (void*)&glGetnTexImage},
    {"glGetnTexImageARB", (void*)&glGetnTexImageARB},
    {"glGetnUniformdv", (void*)&glGetnUniformdv},
    {"glGetnUniformdvARB", (void*)&glGetnUniformdvARB},
    {"glGetnUniformfv", (void*)&glGetnUniformfv},
    {"glGetnUniformfvARB", (void*)&glGetnUniformfvARB},
    {"glGetnUniformfvEXT", (void*)&glGetnUniformfvEXT},
    {"glGetnUniformfvKHR", (void*)&glGetnUniformfvKHR},
    {"glGetnUniformi64vARB", (void*)&glGetnUniformi64vARB},
    {"glGetnUniformiv", (void*)&glGetnUniformiv},
    {"glGetnUniformivARB", (void*)&glGetnUniformivARB},
    {"glGetnUniformivEXT", (void*)&glGetnUniformivEXT},
    {"glGetnUniformivKHR", (void*)&glGetnUniformivKHR},
    {"glGetnUniformui64vARB", (void*)&glGetnUniformui64vARB},
    {"glGetnUniformuiv", (void*)&glGetnUniformuiv},
    {"glGetnUniformuivARB", (void*)&glGetnUniformuivARB},
    {"glGetnUniformuivKHR", (void*)&glGetnUniformuivKHR},
    {"glHint", (void*)&glHint},
    {"glHistogram", (void*)&glHistogram},
    {"glIndexFormatNV", (void*)&glIndexFormatNV},
    {"glIndexMask", (void*)&glIndexMask},
    {"glIndexPointer", (void*)&glIndexPointer},
    {"glIndexd", (void*)&glIndexd},
    {"glIndexdv", (void*)&glIndexdv},
    {"glIndexf", (void*)&glIndexf},
    {"glIndexfv", (void*)&glIndexfv},
    {"glIndexi", (void*)&glIndexi},
    {"glIndexiv", (void*)&glIndexiv},
    {"glIndexs", (void*)&glIndexs},
    {"glIndexsv", (void*)&glIndexsv},
    {"glIndexub", (void*)&glIndexub},
    {"glIndexubv", (void*)&glIndexubv},
    {"glInitNames", (void*)&glInitNames},
    {"glInsertEventMarkerEXT", (void*)&glInsertEventMarkerEXT},
    {"glInterleavedArrays", (void*)&glInterleavedArrays},
    {"glInterpolatePathsNV", (void*)&glInterpolatePathsNV},
    {"glInvalidateBufferData", (void*)&glInvalidateBufferData},
    {"glInvalidateBufferSubData", (void*)&glInvalidateBufferSubData},
    {"glInvalidateFramebuffer", (void*)&glInvalidateFramebuffer},
    {"glInvalidateNamedFramebufferData", (void*)&glInvalidateNamedFramebufferData},
    {"glInvalidateNamedFramebufferSubData", (void*)&glInvalidateNamedFramebufferSubData},
    {"glInvalidateSubFramebuffer", (void*)&glInvalidateSubFramebuffer},
    {"glInvalidateTexImage", (void*)&glInvalidateTexImage},
    {"glInvalidateTexSubImage", (void*)&glInvalidateTexSubImage},
    {"glIsBuffer", (void*)&glIsBuffer},
    {"glIsBufferARB", (void*)&glIsBufferARB},
    {"glIsBufferResidentNV", (void*)&glIsBufferResidentNV},
    {"glIsCommandListNV", (void*)&glIsCommandListNV},
    {"glIsEnabled", (void*)&glIsEnabled},
    {"glIsEnabledIndexedEXT", (void*)&glIsEnabledIndexedEXT},
    {"glIsEnabledi", (void*)&glIsEnabledi},
    {"glIsEnablediEXT", (void*)&glIsEnablediEXT},
    {"glIsEnablediNV", (void*)&glIsEnablediNV},
    {"glIsEnablediOES", (void*)&glIsEnablediOES},
    {"glIsFramebuffer", (void*)&glIsFramebuffer},
    {"glIsFramebufferEXT", (void*)&glIsFramebufferEXT},
    {"glIsFramebufferOES", (void*)&glIsFramebufferOES},
    {"glIsImageHandleResidentARB", (void*)&glIsImageHandleResidentARB},
    {"glIsImageHandleResidentNV", (void*)&glIsImageHandleResidentNV},
    {"glIsList", (void*)&glIsList},
    {"glIsNamedBufferResidentNV", (void*)&glIsNamedBufferResidentNV},
    {"glIsNamedStringARB", (void*)&glIsNamedStringARB},
    {"glIsPathNV", (void*)&glIsPathNV},
    {"glIsPointInFillPathNV", (void*)&glIsPointInFillPathNV},
    {"glIsPointInStrokePathNV", (void*)&glIsPointInStrokePathNV},
    {"glIsProgram", (void*)&glIsProgram},
    {"glIsProgramARB", (void*)&glIsProgramARB},
    {"glIsProgramNV", (void*)&glIsProgramNV},
    {"glIsProgramPipeline", (void*)&glIsProgramPipeline},
    {"glIsProgramPipelineEXT", (void*)&glIsProgramPipelineEXT},
    {"glIsQuery", (void*)&glIsQuery},
    {"glIsQueryARB", (void*)&glIsQueryARB},
    {"glIsQueryEXT", (void*)&glIsQueryEXT},
    {"glIsRenderbuffer", (void*)&glIsRenderbuffer},
    {"glIsRenderbufferEXT", (void*)&glIsRenderbufferEXT},
    {"glIsRenderbufferOES", (void*)&glIsRenderbufferOES},
    {"glIsSampler", (void*)&glIsSampler},
    {"glIsShader", (void*)&glIsShader},
    {"glIsStateNV", (void*)&glIsStateNV},
    {"glIsSync", (void*)&glIsSync},
    {"glIsSyncAPPLE", (void*)&glIsSyncAPPLE},
    {"glIsTexture", (void*)&glIsTexture},
    {"glIsTextureEXT", (void*)&glIsTextureEXT},
    {"glIsTextureHandleResidentARB", (void*)&glIsTextureHandleResidentARB},
    {"glIsTextureHandleResidentNV", (void*)&glIsTextureHandleResidentNV},
    {"glIsTransformFeedback", (void*)&glIsTransformFeedback},
    {"glIsTransformFeedbackNV", (void*)&glIsTransformFeedbackNV},
    {"glIsVertexArray", (void*)&glIsVertexArray},
    {"glIsVertexArrayAPPLE", (void*)&glIsVertexArrayAPPLE},
    {"glIsVertexArrayOES", (void*)&glIsVertexArrayOES},
    {"glLabelObjectEXT", (void*)&glLabelObjectEXT},
    {"glLightModelf", (void*)&glLightModelf},
    {"glLightModelfv", (void*)&glLightModelfv},
    {"glLightModeli", (void*)&glLightModeli},
    {"glLightModeliv", (void*)&glLightModeliv},
    {"glLightf", (void*)&glLightf},
    {"glLightfv", (void*)&glLightfv},
    {"glLighti", (void*)&glLighti},
    {"glLightiv", (void*)&glLightiv},
    {"glLineStipple", (void*)&glLineStipple},
    {"glLineWidth", (void*)&glLineWidth},
    {"glLinkProgram", (void*)&glLinkProgram},
    {"glLinkProgramARB", (void*)&glLinkProgramARB},
    {"glListBase", (void*)&glListBase},
    {"glListDrawCommandsStatesClientNV", (void*)&glListDrawCommandsStatesClientNV},
    {"glLoadIdentity", (void*)&glLoadIdentity},
    {"glLoadMatrixd", (void*)&glLoadMatrixd},
    {"glLoadMatrixf", (void*)&glLoadMatrixf},
    {"glLoadName", (void*)&glLoadName},
    {"glLoadTransposeMatrixd", (void*)&glLoadTransposeMatrixd},
    {"glLoadTransposeMatrixf", (void*)&glLoadTransposeMatrixf},
    {"glLogicOp", (void*)&glLogicOp},
    {"glMakeBufferNonResidentNV", (void*)&glMakeBufferNonResidentNV},
    {"glMakeBufferResidentNV", (void*)&glMakeBufferResidentNV},
    {"glMakeImageHandleNonResidentARB", (void*)&glMakeImageHandleNonResidentARB},
    {"glMakeImageHandleNonResidentNV", (void*)&glMakeImageHandleNonResidentNV},
    {"glMakeImageHandleResidentARB", (void*)&glMakeImageHandleResidentARB},
    {"glMakeImageHandleResidentNV", (void*)&glMakeImageHandleResidentNV},
    {"glMakeNamedBufferNonResidentNV", (void*)&glMakeNamedBufferNonResidentNV},
    {"glMakeNamedBufferResidentNV", (void*)&glMakeNamedBufferResidentNV},
    {"glMakeTextureHandleNonResidentARB", (void*)&glMakeTextureHandleNonResidentARB},
    {"glMakeTextureHandleNonResidentNV", (void*)&glMakeTextureHandleNonResidentNV},
    {"glMakeTextureHandleResidentARB", (void*)&glMakeTextureHandleResidentARB},
    {"glMakeTextureHandleResidentNV", (void*)&glMakeTextureHandleResidentNV},
    {"glMap1d", (void*)&glMap1d},
    {"glMap1f", (void*)&glMap1f},
    {"glMap2d", (void*)&glMap2d},
    {"glMap2f", (void*)&glMap2f},
    {"glMapBuffer", (void*)&glMapBuffer},
    {"glMapBufferARB", (void*)&glMapBufferARB},
    {"glMapBufferOES", (void*)&glMapBufferOES},
    {"glMapBufferRange", (void*)&glMapBufferRange},
    {"glMapBufferRangeEXT", (void*)&glMapBufferRangeEXT},
    {"glMapGrid1d", (void*)&glMapGrid1d},
    {"glMapGrid1f", (void*)&glMapGrid1f},
    {"glMapGrid2d", (void*)&glMapGrid2d},
    {"glMapGrid2f", (void*)&glMapGrid2f},
    {"glMapNamedBuffer", (void*)&glMapNamedBuffer},
    {"glMapNamedBufferEXT", (void*)&glMapNamedBufferEXT},
    {"glMapNamedBufferRange", (void*)&glMapNamedBufferRange},
    {"glMapNamedBufferRangeEXT", (void*)&glMapNamedBufferRangeEXT},
    {"glMaterialf", (void*)&glMaterialf},
    {"glMaterialfv", (void*)&glMaterialfv},
    {"glMateriali", (void*)&glMateriali},
    {"glMaterialiv", (void*)&glMaterialiv},
    {"glMatrixFrustumEXT", (void*)&glMatrixFrustumEXT},
    {"glMatrixLoad3x2fNV", (void*)&glMatrixLoad3x2fNV},
    {"glMatrixLoad3x3fNV", (void*)&glMatrixLoad3x3fNV},
    {"glMatrixLoadIdentityEXT", (void*)&glMatrixLoadIdentityEXT},
    {"glMatrixLoadTranspose3x3fNV", (void*)&glMatrixLoadTranspose3x3fNV},
    {"glMatrixLoadTransposedEXT", (void*)&glMatrixLoadTransposedEXT},
    {"glMatrixLoadTransposefEXT", (void*)&glMatrixLoadTransposefEXT},
    {"glMatrixLoaddEXT", (void*)&glMatrixLoaddEXT},
    {"glMatrixLoadfEXT", (void*)&glMatrixLoadfEXT},
    {"glMatrixMode", (void*)&glMatrixMode},
    {"glMatrixMult3x2fNV", (void*)&glMatrixMult3x2fNV},
    {"glMatrixMult3x3fNV", (void*)&glMatrixMult3x3fNV},
    {"glMatrixMultTranspose3x3fNV", (void*)&glMatrixMultTranspose3x3fNV},
    {"glMatrixMultTransposedEXT", (void*)&glMatrixMultTransposedEXT},
    {"glMatrixMultTransposefEXT", (void*)&glMatrixMultTransposefEXT},
    {"glMatrixMultdEXT", (void*)&glMatrixMultdEXT},
    {"glMatrixMultfEXT", (void*)&glMatrixMultfEXT},
    {"glMatrixOrthoEXT", (void*)&glMatrixOrthoEXT},
    {"glMatrixPopEXT", (void*)&glMatrixPopEXT},
    {"glMatrixPushEXT", (void*)&glMatrixPushEXT},
    {"glMatrixRotatedEXT", (void*)&glMatrixRotatedEXT},
    {"glMatrixRotatefEXT", (void*)&glMatrixRotatefEXT},
    {"glMatrixScaledEXT", (void*)&glMatrixScaledEXT},
    {"glMatrixScalefEXT", (void*)&glMatrixScalefEXT},
    {"glMatrixTranslatedEXT", (void*)&glMatrixTranslatedEXT},
    {"glMatrixTranslatefEXT", (void*)&glMatrixTranslatefEXT},
    {"glMaxShaderCompilerThreadsARB", (void*)&glMaxShaderCompilerThreadsARB},
    {"glMaxShaderCompilerThreadsKHR", (void*)&glMaxShaderCompilerThreadsKHR},
    {"glMemoryBarrier", (void*)&glMemoryBarrier},
    {"glMemoryBarrierByRegion", (void*)&glMemoryBarrierByRegion},
    {"glMemoryBarrierEXT", (void*)&glMemoryBarrierEXT},
    {"glMinSampleShading", (void*)&glMinSampleShading},
    {"glMinSampleShadingARB", (void*)&glMinSampleShadingARB},
    {"glMinSampleShadingOES", (void*)&glMinSampleShadingOES},
    {"glMinmax", (void*)&glMinmax},
    {"glMultMatrixd", (void*)&glMultMatrixd},
    {"glMultMatrixf", (void*)&glMultMatrixf},
    {"glMultTransposeMatrixd", (void*)&glMultTransposeMatrixd},
    {"glMultTransposeMatrixf", (void*)&glMultTransposeMatrixf},
    {"glMultiDrawArrays", (void*)&glMultiDrawArrays},
    {"glMultiDrawArraysEXT", (void*)&glMultiDrawArraysEXT},
    {"glMultiDrawArraysIndirect", (void*)&glMultiDrawArraysIndirect},
    {"glMultiDrawArraysIndirectAMD", (void*)&glMultiDrawArraysIndirectAMD},
    {"glMultiDrawArraysIndirectCount", (void*)&glMultiDrawArraysIndirectCount},
    {"glMultiDrawArraysIndirectCountARB", (void*)&glMultiDrawArraysIndirectCountARB},
    {"glMultiDrawArraysIndirectEXT", (void*)&glMultiDrawArraysIndirectEXT},
    {"glMultiDrawElements", (void*)&glMultiDrawElements},
    {"glMultiDrawElementsBaseVertex", (void*)&glMultiDrawElementsBaseVertex},
    {"glMultiDrawElementsBaseVertexEXT", (void*)&glMultiDrawElementsBaseVertexEXT},
    {"glMultiDrawElementsEXT", (void*)&glMultiDrawElementsEXT},
    {"glMultiDrawElementsIndirect", (void*)&glMultiDrawElementsIndirect},
    {"glMultiDrawElementsIndirectAMD", (void*)&glMultiDrawElementsIndirectAMD},
    {"glMultiDrawElementsIndirectCount", (void*)&glMultiDrawElementsIndirectCount},
    {"glMultiDrawElementsIndirectCountARB", (void*)&glMultiDrawElementsIndirectCountARB},
    {"glMultiDrawElementsIndirectEXT", (void*)&glMultiDrawElementsIndirectEXT},
    {"glMultiTexBufferEXT", (void*)&glMultiTexBufferEXT},
    {"glMultiTexCoord1d", (void*)&glMultiTexCoord1d},
    {"glMultiTexCoord1dv", (void*)&glMultiTexCoord1dv},
    {"glMultiTexCoord1f", (void*)&glMultiTexCoord1f},
    {"glMultiTexCoord1fv", (void*)&glMultiTexCoord1fv},
    {"glMultiTexCoord1i", (void*)&glMultiTexCoord1i},
    {"glMultiTexCoord1iv", (void*)&glMultiTexCoord1iv},
    {"glMultiTexCoord1s", (void*)&glMultiTexCoord1s},
    {"glMultiTexCoord1sv", (void*)&glMultiTexCoord1sv},
    {"glMultiTexCoord2d", (void*)&glMultiTexCoord2d},
    {"glMultiTexCoord2dv", (void*)&glMultiTexCoord2dv},
    {"glMultiTexCoord2f", (void*)&glMultiTexCoord2f},
    {"glMultiTexCoord2fv", (void*)&glMultiTexCoord2fv},
    {"glMultiTexCoord2i", (void*)&glMultiTexCoord2i},
    {"glMultiTexCoord2iv", (void*)&glMultiTexCoord2iv},
    {"glMultiTexCoord2s", (void*)&glMultiTexCoord2s},
    {"glMultiTexCoord2sv", (void*)&glMultiTexCoord2sv},
    {"glMultiTexCoord3d", (void*)&glMultiTexCoord3d},
    {"glMultiTexCoord3dv", (void*)&glMultiTexCoord3dv},
    {"glMultiTexCoord3f", (void*)&glMultiTexCoord3f},
    {"glMultiTexCoord3fv", (void*)&glMultiTexCoord3fv},
    {"glMultiTexCoord3i", (void*)&glMultiTexCoord3i},
    {"glMultiTexCoord3iv", (void*)&glMultiTexCoord3iv},
    {"glMultiTexCoord3s", (void*)&glMultiTexCoord3s},
    {"glMultiTexCoord3sv", (void*)&glMultiTexCoord3sv},
    {"glMultiTexCoord4d", (void*)&glMultiTexCoord4d},
    {"glMultiTexCoord4dv", (void*)&glMultiTexCoord4dv},
    {"glMultiTexCoord4f", (void*)&glMultiTexCoord4f},
    {"glMultiTexCoord4fv", (void*)&glMultiTexCoord4fv},
    {"glMultiTexCoord4i", (void*)&glMultiTexCoord4i},
    {"glMultiTexCoord4iv", (void*)&glMultiTexCoord4iv},
    {"glMultiTexCoord4s", (void*)&glMultiTexCoord4s},
    {"glMultiTexCoord4sv", (void*)&glMultiTexCoord4sv},
    {"glMultiTexCoordP1ui", (void*)&glMultiTexCoordP1ui},
    {"glMultiTexCoordP1uiv", (void*)&glMultiTexCoordP1uiv},
    {"glMultiTexCoordP2ui", (void*)&glMultiTexCoordP2ui},
    {"glMultiTexCoordP2uiv", (void*)&glMultiTexCoordP2uiv},
    {"glMultiTexCoordP3ui", (void*)&glMultiTexCoordP3ui},
    {"glMultiTexCoordP3uiv", (void*)&glMultiTexCoordP3uiv},
    {"glMultiTexCoordP4ui", (void*)&glMultiTexCoordP4ui},
    {"glMultiTexCoordP4uiv", (void*)&glMultiTexCoordP4uiv},
    {"glMultiTexCoordPointerEXT", (void*)&glMultiTexCoordPointerEXT},
    {"glMultiTexEnvfEXT", (void*)&glMultiTexEnvfEXT},
    {"glMultiTexEnvfvEXT", (void*)&glMultiTexEnvfvEXT},
    {"glMultiTexEnviEXT", (void*)&glMultiTexEnviEXT},
    {"glMultiTexEnvivEXT", (void*)&glMultiTexEnvivEXT},
    {"glMultiTexGendEXT", (void*)&glMultiTexGendEXT},
    {"glMultiTexGendvEXT", (void*)&glMultiTexGendvEXT},
    {"glMultiTexGenfEXT", (void*)&glMultiTexGenfEXT},
    {"glMultiTexGenfvEXT", (void*)&glMultiTexGenfvEXT},
    {"glMultiTexGeniEXT", (void*)&glMultiTexGeniEXT},
    {"glMultiTexGenivEXT", (void*)&glMultiTexGenivEXT},
    {"glMultiTexImage1DEXT", (void*)&glMultiTexImage1DEXT},
    {"glMultiTexImage2DEXT", (void*)&glMultiTexImage2DEXT},
    {"glMultiTexImage3DEXT", (void*)&glMultiTexImage3DEXT},
    {"glMultiTexParameterIivEXT", (void*)&glMultiTexParameterIivEXT},
    {"glMultiTexParameterIuivEXT", (void*)&glMultiTexParameterIuivEXT},
    {"glMultiTexParameterfEXT", (void*)&glMultiTexParameterfEXT},
    {"glMultiTexParameterfvEXT", (void*)&glMultiTexParameterfvEXT},
    {"glMultiTexParameteriEXT", (void*)&glMultiTexParameteriEXT},
    {"glMultiTexParameterivEXT", (void*)&glMultiTexParameterivEXT},
    {"glMultiTexRenderbufferEXT", (void*)&glMultiTexRenderbufferEXT},
    {"glMultiTexSubImage1DEXT", (void*)&glMultiTexSubImage1DEXT},
    {"glMultiTexSubImage2DEXT", (void*)&glMultiTexSubImage2DEXT},
    {"glMultiTexSubImage3DEXT", (void*)&glMultiTexSubImage3DEXT},
    {"glNamedBufferAttachMemoryNV", (void*)&glNamedBufferAttachMemoryNV},
    {"glNamedBufferData", (void*)&glNamedBufferData},
    {"glNamedBufferDataEXT", (void*)&glNamedBufferDataEXT},
    {"glNamedBufferPageCommitmentARB", (void*)&glNamedBufferPageCommitmentARB},
    {"glNamedBufferPageCommitmentEXT", (void*)&glNamedBufferPageCommitmentEXT},
    {"glNamedBufferStorage", (void*)&glNamedBufferStorage},
    {"glNamedBufferStorageEXT", (void*)&glNamedBufferStorageEXT},
    {"glNamedBufferSubData", (void*)&glNamedBufferSubData},
    {"glNamedBufferSubDataEXT", (void*)&glNamedBufferSubDataEXT},
    {"glNamedCopyBufferSubDataEXT", (void*)&glNamedCopyBufferSubDataEXT},
    {"glNamedFramebufferDrawBuffer", (void*)&glNamedFramebufferDrawBuffer},
    {"glNamedFramebufferDrawBuffers", (void*)&glNamedFramebufferDrawBuffers},
    {"glNamedFramebufferParameteri", (void*)&glNamedFramebufferParameteri},
    {"glNamedFramebufferParameteriEXT", (void*)&glNamedFramebufferParameteriEXT},
    {"glNamedFramebufferReadBuffer", (void*)&glNamedFramebufferReadBuffer},
    {"glNamedFramebufferRenderbuffer", (void*)&glNamedFramebufferRenderbuffer},
    {"glNamedFramebufferRenderbufferEXT", (void*)&glNamedFramebufferRenderbufferEXT},
    {"glNamedFramebufferSampleLocationsfvARB", (void*)&glNamedFramebufferSampleLocationsfvARB},
    {"glNamedFramebufferSampleLocationsfvNV", (void*)&glNamedFramebufferSampleLocationsfvNV},
    {"glNamedFramebufferTexture", (void*)&glNamedFramebufferTexture},
    {"glNamedFramebufferTexture1DEXT", (void*)&glNamedFramebufferTexture1DEXT},
    {"glNamedFramebufferTexture2DEXT", (void*)&glNamedFramebufferTexture2DEXT},
    {"glNamedFramebufferTexture3DEXT", (void*)&glNamedFramebufferTexture3DEXT},
    {"glNamedFramebufferTextureEXT", (void*)&glNamedFramebufferTextureEXT},
    {"glNamedFramebufferTextureFaceEXT", (void*)&glNamedFramebufferTextureFaceEXT},
    {"glNamedFramebufferTextureLayer", (void*)&glNamedFramebufferTextureLayer},
    {"glNamedFramebufferTextureLayerEXT", (void*)&glNamedFramebufferTextureLayerEXT},
    {"glNamedFramebufferTextureMultiviewOVR", (void*)&glNamedFramebufferTextureMultiviewOVR},
    {"glNamedProgramLocalParameter4dEXT", (void*)&glNamedProgramLocalParameter4dEXT},
    {"glNamedProgramLocalParameter4dvEXT", (void*)&glNamedProgramLocalParameter4dvEXT},
    {"glNamedProgramLocalParameter4fEXT", (void*)&glNamedProgramLocalParameter4fEXT},
    {"glNamedProgramLocalParameter4fvEXT", (void*)&glNamedProgramLocalParameter4fvEXT},
    {"glNamedProgramLocalParameterI4iEXT", (void*)&glNamedProgramLocalParameterI4iEXT},
    {"glNamedProgramLocalParameterI4ivEXT", (void*)&glNamedProgramLocalParameterI4ivEXT},
    {"glNamedProgramLocalParameterI4uiEXT", (void*)&glNamedProgramLocalParameterI4uiEXT},
    {"glNamedProgramLocalParameterI4uivEXT", (void*)&glNamedProgramLocalParameterI4uivEXT},
    {"glNamedProgramLocalParameters4fvEXT", (void*)&glNamedProgramLocalParameters4fvEXT},
    {"glNamedProgramLocalParametersI4ivEXT", (void*)&glNamedProgramLocalParametersI4ivEXT},
    {"glNamedProgramLocalParametersI4uivEXT", (void*)&glNamedProgramLocalParametersI4uivEXT},
    {"glNamedProgramStringEXT", (void*)&glNamedProgramStringEXT},
    {"glNamedRenderbufferStorage", (void*)&glNamedRenderbufferStorage},
    {"glNamedRenderbufferStorageEXT", (void*)&glNamedRenderbufferStorageEXT},
    {"glNamedRenderbufferStorageMultisample", (void*)&glNamedRenderbufferStorageMultisample},
    {"glNamedRenderbufferStorageMultisampleAdvancedAMD", (void*)&glNamedRenderbufferStorageMultisampleAdvancedAMD},
    {"glNamedRenderbufferStorageMultisampleCoverageEXT", (void*)&glNamedRenderbufferStorageMultisampleCoverageEXT},
    {"glNamedRenderbufferStorageMultisampleEXT", (void*)&glNamedRenderbufferStorageMultisampleEXT},
    {"glNamedStringARB", (void*)&glNamedStringARB},
    {"glNewList", (void*)&glNewList},
    {"glNormal3b", (void*)&glNormal3b},
    {"glNormal3bv", (void*)&glNormal3bv},
    {"glNormal3d", (void*)&glNormal3d},
    {"glNormal3dv", (void*)&glNormal3dv},
    {"glNormal3f", (void*)&glNormal3f},
    {"glNormal3fv", (void*)&glNormal3fv},
    {"glNormal3i", (void*)&glNormal3i},
    {"glNormal3iv", (void*)&glNormal3iv},
    {"glNormal3s", (void*)&glNormal3s},
    {"glNormal3sv", (void*)&glNormal3sv},
    {"glNormalFormatNV", (void*)&glNormalFormatNV},
    {"glNormalP3ui", (void*)&glNormalP3ui},
    {"glNormalP3uiv", (void*)&glNormalP3uiv},
    {"glNormalPointer", (void*)&glNormalPointer},
    {"glObjectLabel", (void*)&glObjectLabel},
    {"glObjectLabelKHR", (void*)&glObjectLabelKHR},
    {"glObjectPtrLabel", (void*)&glObjectPtrLabel},
    {"glObjectPtrLabelKHR", (void*)&glObjectPtrLabelKHR},
    {"glOrtho", (void*)&glOrtho},
    {"glPassThrough", (void*)&glPassThrough},
    {"glPatchParameterfv", (void*)&glPatchParameterfv},
    {"glPatchParameteri", (void*)&glPatchParameteri},
    {"glPatchParameteriEXT", (void*)&glPatchParameteriEXT},
    {"glPatchParameteriOES", (void*)&glPatchParameteriOES},
    {"glPathColorGenNV", (void*)&glPathColorGenNV},
    {"glPathCommandsNV", (void*)&glPathCommandsNV},
    {"glPathCoordsNV", (void*)&glPathCoordsNV},
    {"glPathCoverDepthFuncNV", (void*)&glPathCoverDepthFuncNV},
    {"glPathDashArrayNV", (void*)&glPathDashArrayNV},
    {"glPathFogGenNV", (void*)&glPathFogGenNV},
    {"glPathGlyphIndexArrayNV", (void*)&glPathGlyphIndexArrayNV},
    {"glPathGlyphIndexRangeNV", (void*)&glPathGlyphIndexRangeNV},
    {"glPathGlyphRangeNV", (void*)&glPathGlyphRangeNV},
    {"glPathGlyphsNV", (void*)&glPathGlyphsNV},
    {"glPathMemoryGlyphIndexArrayNV", (void*)&glPathMemoryGlyphIndexArrayNV},
    {"glPathParameterfNV", (void*)&glPathParameterfNV},
    {"glPathParameterfvNV", (void*)&glPathParameterfvNV},
    {"glPathParameteriNV", (void*)&glPathParameteriNV},
    {"glPathParameterivNV", (void*)&glPathParameterivNV},
    {"glPathStencilDepthOffsetNV", (void*)&glPathStencilDepthOffsetNV},
    {"glPathStencilFuncNV", (void*)&glPathStencilFuncNV},
    {"glPathStringNV", (void*)&glPathStringNV},
    {"glPathSubCommandsNV", (void*)&glPathSubCommandsNV},
    {"glPathSubCoordsNV", (void*)&glPathSubCoordsNV},
    {"glPathTexGenNV", (void*)&glPathTexGenNV},
    {"glPauseTransformFeedback", (void*)&glPauseTransformFeedback},
    {"glPauseTransformFeedbackNV", (void*)&glPauseTransformFeedbackNV},
    {"glPixelMapfv", (void*)&glPixelMapfv},
    {"glPixelMapuiv", (void*)&glPixelMapuiv},
    {"glPixelMapusv", (void*)&glPixelMapusv},
    {"glPixelStoref", (void*)&glPixelStoref},
    {"glPixelStorei", (void*)&glPixelStorei},
    {"glPixelTransferf", (void*)&glPixelTransferf},
    {"glPixelTransferi", (void*)&glPixelTransferi},
    {"glPixelZoom", (void*)&glPixelZoom},
    {"glPointAlongPathNV", (void*)&glPointAlongPathNV},
    {"glPointParameterf", (void*)&glPointParameterf},
    {"glPointParameterfARB", (void*)&glPointParameterfARB},
    {"glPointParameterfEXT", (void*)&glPointParameterfEXT},
    {"glPointParameterfSGIS", (void*)&glPointParameterfSGIS},
    {"glPointParameterfv", (void*)&glPointParameterfv},
    {"glPointParameterfvARB", (void*)&glPointParameterfvARB},
    {"glPointParameterfvEXT", (void*)&glPointParameterfvEXT},
    {"glPointParameterfvSGIS", (void*)&glPointParameterfvSGIS},
    {"glPointParameteri", (void*)&glPointParameteri},
    {"glPointParameteriNV", (void*)&glPointParameteriNV},
    {"glPointParameteriv", (void*)&glPointParameteriv},
    {"glPointParameterivNV", (void*)&glPointParameterivNV},
    {"glPointSize", (void*)&glPointSize},
    {"glPolygonMode", (void*)&glPolygonMode},
    {"glPolygonModeNV", (void*)&glPolygonModeNV},
    {"glPolygonOffset", (void*)&glPolygonOffset},
    {"glPolygonOffsetClamp", (void*)&glPolygonOffsetClamp},
    {"glPolygonOffsetClampEXT", (void*)&glPolygonOffsetClampEXT},
    {"glPolygonOffsetEXT", (void*)&glPolygonOffsetEXT},
    {"glPolygonStipple", (void*)&glPolygonStipple},
    {"glPopAttrib", (void*)&glPopAttrib},
    {"glPopClientAttrib", (void*)&glPopClientAttrib},
    {"glPopDebugGroup", (void*)&glPopDebugGroup},
    {"glPopDebugGroupKHR", (void*)&glPopDebugGroupKHR},
    {"glPopGroupMarkerEXT", (void*)&glPopGroupMarkerEXT},
    {"glPopMatrix", (void*)&glPopMatrix},
    {"glPopName", (void*)&glPopName},
    {"glPrimitiveBoundingBoxARB", (void*)&glPrimitiveBoundingBoxARB},
    {"glPrimitiveBoundingBoxEXT", (void*)&glPrimitiveBoundingBoxEXT},
    {"glPrimitiveBoundingBoxOES", (void*)&glPrimitiveBoundingBoxOES},
    {"glPrimitiveRestartIndex", (void*)&glPrimitiveRestartIndex},
    {"glPrimitiveRestartIndexNV", (void*)&glPrimitiveRestartIndexNV},
    {"glPrioritizeTextures", (void*)&glPrioritizeTextures},
    {"glProgramBinary", (void*)&glProgramBinary},
    {"glProgramBinaryOES", (void*)&glProgramBinaryOES},
    {"glProgramParameteri", (void*)&glProgramParameteri},
    {"glProgramParameteriARB", (void*)&glProgramParameteriARB},
    {"glProgramParameteriEXT", (void*)&glProgramParameteriEXT},
    {"glProgramPathFragmentInputGenNV", (void*)&glProgramPathFragmentInputGenNV},
    {"glProgramUniform1d", (void*)&glProgramUniform1d},
    {"glProgramUniform1dEXT", (void*)&glProgramUniform1dEXT},
    {"glProgramUniform1dv", (void*)&glProgramUniform1dv},
    {"glProgramUniform1dvEXT", (void*)&glProgramUniform1dvEXT},
    {"glProgramUniform1f", (void*)&glProgramUniform1f},
    {"glProgramUniform1fEXT", (void*)&glProgramUniform1fEXT},
    {"glProgramUniform1fv", (void*)&glProgramUniform1fv},
    {"glProgramUniform1fvEXT", (void*)&glProgramUniform1fvEXT},
    {"glProgramUniform1i", (void*)&glProgramUniform1i},
    {"glProgramUniform1iEXT", (void*)&glProgramUniform1iEXT},
    {"glProgramUniform1iv", (void*)&glProgramUniform1iv},
    {"glProgramUniform1ivEXT", (void*)&glProgramUniform1ivEXT},
    {"glProgramUniform1ui", (void*)&glProgramUniform1ui},
    {"glProgramUniform1uiEXT", (void*)&glProgramUniform1uiEXT},
    {"glProgramUniform1uiv", (void*)&glProgramUniform1uiv},
    {"glProgramUniform1uivEXT", (void*)&glProgramUniform1uivEXT},
    {"glProgramUniform2d", (void*)&glProgramUniform2d},
    {"glProgramUniform2dEXT", (void*)&glProgramUniform2dEXT},
    {"glProgramUniform2dv", (void*)&glProgramUniform2dv},
    {"glProgramUniform2dvEXT", (void*)&glProgramUniform2dvEXT},
    {"glProgramUniform2f", (void*)&glProgramUniform2f},
    {"glProgramUniform2fEXT", (void*)&glProgramUniform2fEXT},
    {"glProgramUniform2fv", (void*)&glProgramUniform2fv},
    {"glProgramUniform2fvEXT", (void*)&glProgramUniform2fvEXT},
    {"glProgramUniform2i", (void*)&glProgramUniform2i},
    {"glProgramUniform2iEXT", (void*)&glProgramUniform2iEXT},
    {"glProgramUniform2iv", (void*)&glProgramUniform2iv},
    {"glProgramUniform2ivEXT", (void*)&glProgramUniform2ivEXT},
    {"glProgramUniform2ui", (void*)&glProgramUniform2ui},
    {"glProgramUniform2uiEXT", (void*)&glProgramUniform2uiEXT},
    {"glProgramUniform2uiv", (void*)&glProgramUniform2uiv},
    {"glProgramUniform2uivEXT", (void*)&glProgramUniform2uivEXT},
    {"glProgramUniform3d", (void*)&glProgramUniform3d},
    {"glProgramUniform3dEXT", (void*)&glProgramUniform3dEXT},
    {"glProgramUniform3dv", (void*)&glProgramUniform3dv},
    {"glProgramUniform3dvEXT", (void*)&glProgramUniform3dvEXT},
    {"glProgramUniform3f", (void*)&glProgramUniform3f},
    {"glProgramUniform3fEXT", (void*)&glProgramUniform3fEXT},
    {"glProgramUniform3fv", (void*)&glProgramUniform3fv},
    {"glProgramUniform3fvEXT", (void*)&glProgramUniform3fvEXT},
    {"glProgramUniform3i", (void*)&glProgramUniform3i},
    {"glProgramUniform3iEXT", (void*)&glProgramUniform3iEXT},
    {"glProgramUniform3iv", (void*)&glProgramUniform3iv},
    {"glProgramUniform3ivEXT", (void*)&glProgramUniform3ivEXT},
    {"glProgramUniform3ui", (void*)&glProgramUniform3ui},
    {"glProgramUniform3uiEXT", (void*)&glProgramUniform3uiEXT},
    {"glProgramUniform3uiv", (void*)&glProgramUniform3uiv},
    {"glProgramUniform3uivEXT", (void*)&glProgramUniform3uivEXT},
    {"glProgramUniform4d", (void*)&glProgramUniform4d},
    {"glProgramUniform4dEXT", (void*)&glProgramUniform4dEXT},
    {"glProgramUniform4dv", (void*)&glProgramUniform4dv},
    {"glProgramUniform4dvEXT", (void*)&glProgramUniform4dvEXT},
    {"glProgramUniform4f", (void*)&glProgramUniform4f},
    {"glProgramUniform4fEXT", (void*)&glProgramUniform4fEXT},
    {"glProgramUniform4fv", (void*)&glProgramUniform4fv},
    {"glProgramUniform4fvEXT", (void*)&glProgramUniform4fvEXT},
    {"glProgramUniform4i", (void*)&glProgramUniform4i},
    {"glProgramUniform4iEXT", (void*)&glProgramUniform4iEXT},
    {"glProgramUniform4iv", (void*)&glProgramUniform4iv},
    {"glProgramUniform4ivEXT", (void*)&glProgramUniform4ivEXT},
    {"glProgramUniform4ui", (void*)&glProgramUniform4ui},
    {"glProgramUniform4uiEXT", (void*)&glProgramUniform4uiEXT},
    {"glProgramUniform4uiv", (void*)&glProgramUniform4uiv},
    {"glProgramUniform4uivEXT", (void*)&glProgramUniform4uivEXT},
    {"glProgramUniformHandleui64ARB", (void*)&glProgramUniformHandleui64ARB},
    {"glProgramUniformHandleui64NV", (void*)&glProgramUniformHandleui64NV},
    {"glProgramUniformHandleui64vARB", (void*)&glProgramUniformHandleui64vARB},
    {"glProgramUniformHandleui64vNV", (void*)&glProgramUniformHandleui64vNV},
    {"glProgramUniformMatrix2dv", (void*)&glProgramUniformMatrix2dv},
    {"glProgramUniformMatrix2dvEXT", (void*)&glProgramUniformMatrix2dvEXT},
    {"glProgramUniformMatrix2fv", (void*)&glProgramUniformMatrix2fv},
    {"glProgramUniformMatrix2fvEXT", (void*)&glProgramUniformMatrix2fvEXT},
    {"glProgramUniformMatrix2x3dv", (void*)&glProgramUniformMatrix2x3dv},
    {"glProgramUniformMatrix2x3dvEXT", (void*)&glProgramUniformMatrix2x3dvEXT},
    {"glProgramUniformMatrix2x3fv", (void*)&glProgramUniformMatrix2x3fv},
    {"glProgramUniformMatrix2x3fvEXT", (void*)&glProgramUniformMatrix2x3fvEXT},
    {"glProgramUniformMatrix2x4dv", (void*)&glProgramUniformMatrix2x4dv},
    {"glProgramUniformMatrix2x4dvEXT", (void*)&glProgramUniformMatrix2x4dvEXT},
    {"glProgramUniformMatrix2x4fv", (void*)&glProgramUniformMatrix2x4fv},
    {"glProgramUniformMatrix2x4fvEXT", (void*)&glProgramUniformMatrix2x4fvEXT},
    {"glProgramUniformMatrix3dv", (void*)&glProgramUniformMatrix3dv},
    {"glProgramUniformMatrix3dvEXT", (void*)&glProgramUniformMatrix3dvEXT},
    {"glProgramUniformMatrix3fv", (void*)&glProgramUniformMatrix3fv},
    {"glProgramUniformMatrix3fvEXT", (void*)&glProgramUniformMatrix3fvEXT},
    {"glProgramUniformMatrix3x2dv", (void*)&glProgramUniformMatrix3x2dv},
    {"glProgramUniformMatrix3x2dvEXT", (void*)&glProgramUniformMatrix3x2dvEXT},
    {"glProgramUniformMatrix3x2fv", (void*)&glProgramUniformMatrix3x2fv},
    {"glProgramUniformMatrix3x2fvEXT", (void*)&glProgramUniformMatrix3x2fvEXT},
    {"glProgramUniformMatrix3x4dv", (void*)&glProgramUniformMatrix3x4dv},
    {"glProgramUniformMatrix3x4dvEXT", (void*)&glProgramUniformMatrix3x4dvEXT},
    {"glProgramUniformMatrix3x4fv", (void*)&glProgramUniformMatrix3x4fv},
    {"glProgramUniformMatrix3x4fvEXT", (void*)&glProgramUniformMatrix3x4fvEXT},
    {"glProgramUniformMatrix4dv", (void*)&glProgramUniformMatrix4dv},
    {"glProgramUniformMatrix4dvEXT", (void*)&glProgramUniformMatrix4dvEXT},
    {"glProgramUniformMatrix4fv", (void*)&glProgramUniformMatrix4fv},
    {"glProgramUniformMatrix4fvEXT", (void*)&glProgramUniformMatrix4fvEXT},
    {"glProgramUniformMatrix4x2dv", (void*)&glProgramUniformMatrix4x2dv},
    {"glProgramUniformMatrix4x2dvEXT", (void*)&glProgramUniformMatrix4x2dvEXT},
    {"glProgramUniformMatrix4x2fv", (void*)&glProgramUniformMatrix4x2fv},
    {"glProgramUniformMatrix4x2fvEXT", (void*)&glProgramUniformMatrix4x2fvEXT},
    {"glProgramUniformMatrix4x3dv", (void*)&glProgramUniformMatrix4x3dv},
    {"glProgramUniformMatrix4x3dvEXT", (void*)&glProgramUniformMatrix4x3dvEXT},
    {"glProgramUniformMatrix4x3fv", (void*)&glProgramUniformMatrix4x3fv},
    {"glProgramUniformMatrix4x3fvEXT", (void*)&glProgramUniformMatrix4x3fvEXT},
    {"glProgramUniformui64NV", (void*)&glProgramUniformui64NV},
    {"glProgramUniformui64vNV", (void*)&glProgramUniformui64vNV},
    {"glProvokingVertex", (void*)&glProvokingVertex},
    {"glProvokingVertexEXT", (void*)&glProvokingVertexEXT},
    {"glPushAttrib", (void*)&glPushAttrib},
    {"glPushClientAttrib", (void*)&glPushClientAttrib},
    {"glPushClientAttribDefaultEXT", (void*)&glPushClientAttribDefaultEXT},
    {"glPushDebugGroup", (void*)&glPushDebugGroup},
    {"glPushDebugGroupKHR", (void*)&glPushDebugGroupKHR},
    {"glPushGroupMarkerEXT", (void*)&glPushGroupMarkerEXT},
    {"glPushMatrix", (void*)&glPushMatrix},
    {"glPushName", (void*)&glPushName},
    {"glQueryCounter", (void*)&glQueryCounter},
    {"glQueryCounterEXT", (void*)&glQueryCounterEXT},
    {"glRasterPos2d", (void*)&glRasterPos2d},
    {"glRasterPos2dv", (void*)&glRasterPos2dv},
    {"glRasterPos2f", (void*)&glRasterPos2f},
    {"glRasterPos2fv", (void*)&glRasterPos2fv},
    {"glRasterPos2i", (void*)&glRasterPos2i},
    {"glRasterPos2iv", (void*)&glRasterPos2iv},
    {"glRasterPos2s", (void*)&glRasterPos2s},
    {"glRasterPos2sv", (void*)&glRasterPos2sv},
    {"glRasterPos3d", (void*)&glRasterPos3d},
    {"glRasterPos3dv", (void*)&glRasterPos3dv},
    {"glRasterPos3f", (void*)&glRasterPos3f},
    {"glRasterPos3fv", (void*)&glRasterPos3fv},
    {"glRasterPos3i", (void*)&glRasterPos3i},
    {"glRasterPos3iv", (void*)&glRasterPos3iv},
    {"glRasterPos3s", (void*)&glRasterPos3s},
    {"glRasterPos3sv", (void*)&glRasterPos3sv},
    {"glRasterPos4d", (void*)&glRasterPos4d},
    {"glRasterPos4dv", (void*)&glRasterPos4dv},
    {"glRasterPos4f", (void*)&glRasterPos4f},
    {"glRasterPos4fv", (void*)&glRasterPos4fv},
    {"glRasterPos4i", (void*)&glRasterPos4i},
    {"glRasterPos4iv", (void*)&glRasterPos4iv},
    {"glRasterPos4s", (void*)&glRasterPos4s},
    {"glRasterPos4sv", (void*)&glRasterPos4sv},
    {"glRasterSamplesEXT", (void*)&glRasterSamplesEXT},
    {"glReadBuffer", (void*)&glReadBuffer},
    {"glReadBufferNV", (void*)&glReadBufferNV},
    {"glReadPixels", (void*)&glReadPixels},
    {"glReadnPixels", (void*)&glReadnPixels},
    {"glReadnPixelsARB", (void*)&glReadnPixelsARB},
    {"glReadnPixelsEXT", (void*)&glReadnPixelsEXT},
    {"glReadnPixelsKHR", (void*)&glReadnPixelsKHR},
    {"glRectd", (void*)&glRectd},
    {"glRectdv", (void*)&glRectdv},
    {"glRectf", (void*)&glRectf},
    {"glRectfv", (void*)&glRectfv},
    {"glRecti", (void*)&glRecti},
    {"glRectiv", (void*)&glRectiv},
    {"glRects", (void*)&glRects},
    {"glRectsv", (void*)&glRectsv},
    {"glReleaseShaderCompiler", (void*)&glReleaseShaderCompiler},
    {"glRenderMode", (void*)&glRenderMode},
    {"glRenderbufferStorage", (void*)&glRenderbufferStorage},
    {"glRenderbufferStorageEXT", (void*)&glRenderbufferStorageEXT},
    {"glRenderbufferStorageMultisample", (void*)&glRenderbufferStorageMultisample},
    {"glRenderbufferStorageMultisampleANGLE", (void*)&glRenderbufferStorageMultisampleANGLE},
    {"glRenderbufferStorageMultisampleAPPLE", (void*)&glRenderbufferStorageMultisampleAPPLE},
    {"glRenderbufferStorageMultisampleAdvancedAMD", (void*)&glRenderbufferStorageMultisampleAdvancedAMD},
    {"glRenderbufferStorageMultisampleEXT", (void*)&glRenderbufferStorageMultisampleEXT},
    {"glRenderbufferStorageMultisampleNV", (void*)&glRenderbufferStorageMultisampleNV},
    {"glRenderbufferStorageOES", (void*)&glRenderbufferStorageOES},
    {"glResetHistogram", (void*)&glResetHistogram},
    {"glResetMemoryObjectParameterNV", (void*)&glResetMemoryObjectParameterNV},
    {"glResetMinmax", (void*)&glResetMinmax},
    {"glResolveDepthValuesNV", (void*)&glResolveDepthValuesNV},
    {"glResumeTransformFeedback", (void*)&glResumeTransformFeedback},
    {"glResumeTransformFeedbackNV", (void*)&glResumeTransformFeedbackNV},
    {"glRotated", (void*)&glRotated},
    {"glRotatef", (void*)&glRotatef},
    {"glSampleCoverage", (void*)&glSampleCoverage},
    {"glSampleCoverageARB", (void*)&glSampleCoverageARB},
    {"glSampleMaski", (void*)&glSampleMaski},
    {"glSamplePatternEXT", (void*)&glSamplePatternEXT},
    {"glSamplerParameterIiv", (void*)&glSamplerParameterIiv},
    {"glSamplerParameterIivEXT", (void*)&glSamplerParameterIivEXT},
    {"glSamplerParameterIivOES", (void*)&glSamplerParameterIivOES},
    {"glSamplerParameterIuiv", (void*)&glSamplerParameterIuiv},
    {"glSamplerParameterIuivEXT", (void*)&glSamplerParameterIuivEXT},
    {"glSamplerParameterIuivOES", (void*)&glSamplerParameterIuivOES},
    {"glSamplerParameterf", (void*)&glSamplerParameterf},
    {"glSamplerParameterfv", (void*)&glSamplerParameterfv},
    {"glSamplerParameteri", (void*)&glSamplerParameteri},
    {"glSamplerParameteriv", (void*)&glSamplerParameteriv},
    {"glScaled", (void*)&glScaled},
    {"glScalef", (void*)&glScalef},
    {"glScissor", (void*)&glScissor},
    {"glScissorArrayv", (void*)&glScissorArrayv},
    {"glScissorArrayvNV", (void*)&glScissorArrayvNV},
    {"glScissorArrayvOES", (void*)&glScissorArrayvOES},
    {"glScissorExclusiveArrayvNV", (void*)&glScissorExclusiveArrayvNV},
    {"glScissorExclusiveNV", (void*)&glScissorExclusiveNV},
    {"glScissorIndexed", (void*)&glScissorIndexed},
    {"glScissorIndexedNV", (void*)&glScissorIndexedNV},
    {"glScissorIndexedOES", (void*)&glScissorIndexedOES},
    {"glScissorIndexedv", (void*)&glScissorIndexedv},
    {"glScissorIndexedvNV", (void*)&glScissorIndexedvNV},
    {"glScissorIndexedvOES", (void*)&glScissorIndexedvOES},
    {"glSecondaryColor3b", (void*)&glSecondaryColor3b},
    {"glSecondaryColor3bv", (void*)&glSecondaryColor3bv},
    {"glSecondaryColor3d", (void*)&glSecondaryColor3d},
    {"glSecondaryColor3dv", (void*)&glSecondaryColor3dv},
    {"glSecondaryColor3f", (void*)&glSecondaryColor3f},
    {"glSecondaryColor3fv", (void*)&glSecondaryColor3fv},
    {"glSecondaryColor3i", (void*)&glSecondaryColor3i},
    {"glSecondaryColor3iv", (void*)&glSecondaryColor3iv},
    {"glSecondaryColor3s", (void*)&glSecondaryColor3s},
    {"glSecondaryColor3sv", (void*)&glSecondaryColor3sv},
    {"glSecondaryColor3ub", (void*)&glSecondaryColor3ub},
    {"glSecondaryColor3ubv", (void*)&glSecondaryColor3ubv},
    {"glSecondaryColor3ui", (void*)&glSecondaryColor3ui},
    {"glSecondaryColor3uiv", (void*)&glSecondaryColor3uiv},
    {"glSecondaryColor3us", (void*)&glSecondaryColor3us},
    {"glSecondaryColor3usv", (void*)&glSecondaryColor3usv},
    {"glSecondaryColorFormatNV", (void*)&glSecondaryColorFormatNV},
    {"glSecondaryColorP3ui", (void*)&glSecondaryColorP3ui},
    {"glSecondaryColorP3uiv", (void*)&glSecondaryColorP3uiv},
    {"glSecondaryColorPointer", (void*)&glSecondaryColorPointer},
    {"glSelectBuffer", (void*)&glSelectBuffer},
    {"glSelectPerfMonitorCountersAMD", (void*)&glSelectPerfMonitorCountersAMD},
    {"glSeparableFilter2D", (void*)&glSeparableFilter2D},
    {"glShadeModel", (void*)&glShadeModel},
    {"glShaderBinary", (void*)&glShaderBinary},
    {"glShaderSource", (void*)&glShaderSource},
    {"glShaderSourceARB", (void*)&glShaderSourceARB},
    {"glShaderStorageBlockBinding", (void*)&glShaderStorageBlockBinding},
    {"glShadingRateCombinerOpsEXT", (void*)&glShadingRateCombinerOpsEXT},
    {"glShadingRateEXT", (void*)&glShadingRateEXT},
    {"glShadingRateImageBarrierNV", (void*)&glShadingRateImageBarrierNV},
    {"glShadingRateImagePaletteNV", (void*)&glShadingRateImagePaletteNV},
    {"glShadingRateSampleOrderCustomNV", (void*)&glShadingRateSampleOrderCustomNV},
    {"glShadingRateSampleOrderNV", (void*)&glShadingRateSampleOrderNV},
    {"glSpecializeShader", (void*)&glSpecializeShader},
    {"glSpecializeShaderARB", (void*)&glSpecializeShaderARB},
    {"glStateCaptureNV", (void*)&glStateCaptureNV},
    {"glStencilFillPathInstancedNV", (void*)&glStencilFillPathInstancedNV},
    {"glStencilFillPathNV", (void*)&glStencilFillPathNV},
    {"glStencilFunc", (void*)&glStencilFunc},
    {"glStencilFuncSeparate", (void*)&glStencilFuncSeparate},
    {"glStencilFuncSeparateATI", (void*)&glStencilFuncSeparateATI},
    {"glStencilMask", (void*)&glStencilMask},
    {"glStencilMaskSeparate", (void*)&glStencilMaskSeparate},
    {"glStencilOp", (void*)&glStencilOp},
    {"glStencilOpSeparate", (void*)&glStencilOpSeparate},
    {"glStencilOpSeparateATI", (void*)&glStencilOpSeparateATI},
    {"glStencilStrokePathInstancedNV", (void*)&glStencilStrokePathInstancedNV},
    {"glStencilStrokePathNV", (void*)&glStencilStrokePathNV},
    {"glStencilThenCoverFillPathInstancedNV", (void*)&glStencilThenCoverFillPathInstancedNV},
    {"glStencilThenCoverFillPathNV", (void*)&glStencilThenCoverFillPathNV},
    {"glStencilThenCoverStrokePathInstancedNV", (void*)&glStencilThenCoverStrokePathInstancedNV},
    {"glStencilThenCoverStrokePathNV", (void*)&glStencilThenCoverStrokePathNV},
    {"glTexAttachMemoryNV", (void*)&glTexAttachMemoryNV},
    {"glTexBuffer", (void*)&glTexBuffer},
    {"glTexBufferARB", (void*)&glTexBufferARB},
    {"glTexBufferEXT", (void*)&glTexBufferEXT},
    {"glTexBufferOES", (void*)&glTexBufferOES},
    {"glTexBufferRange", (void*)&glTexBufferRange},
    {"glTexBufferRangeEXT", (void*)&glTexBufferRangeEXT},
    {"glTexBufferRangeOES", (void*)&glTexBufferRangeOES},
    {"glTexCoord1d", (void*)&glTexCoord1d},
    {"glTexCoord1dv", (void*)&glTexCoord1dv},
    {"glTexCoord1f", (void*)&glTexCoord1f},
    {"glTexCoord1fv", (void*)&glTexCoord1fv},
    {"glTexCoord1i", (void*)&glTexCoord1i},
    {"glTexCoord1iv", (void*)&glTexCoord1iv},
    {"glTexCoord1s", (void*)&glTexCoord1s},
    {"glTexCoord1sv", (void*)&glTexCoord1sv},
    {"glTexCoord2d", (void*)&glTexCoord2d},
    {"glTexCoord2dv", (void*)&glTexCoord2dv},
    {"glTexCoord2f", (void*)&glTexCoord2f},
    {"glTexCoord2fv", (void*)&glTexCoord2fv},
    {"glTexCoord2i", (void*)&glTexCoord2i},
    {"glTexCoord2iv", (void*)&glTexCoord2iv},
    {"glTexCoord2s", (void*)&glTexCoord2s},
    {"glTexCoord2sv", (void*)&glTexCoord2sv},
    {"glTexCoord3d", (void*)&glTexCoord3d},
    {"glTexCoord3dv", (void*)&glTexCoord3dv},
    {"glTexCoord3f", (void*)&glTexCoord3f},
    {"glTexCoord3fv", (void*)&glTexCoord3fv},
    {"glTexCoord3i", (void*)&glTexCoord3i},
    {"glTexCoord3iv", (void*)&glTexCoord3iv},
    {"glTexCoord3s", (void*)&glTexCoord3s},
    {"glTexCoord3sv", (void*)&glTexCoord3sv},
    {"glTexCoord4d", (void*)&glTexCoord4d},
    {"glTexCoord4dv", (void*)&glTexCoord4dv},
    {"glTexCoord4f", (void*)&glTexCoord4f},
    {"glTexCoord4fv", (void*)&glTexCoord4fv},
    {"glTexCoord4i", (void*)&glTexCoord4i},
    {"glTexCoord4iv", (void*)&glTexCoord4iv},
    {"glTexCoord4s", (void*)&glTexCoord4s},
    {"glTexCoord4sv", (void*)&glTexCoord4sv},
    {"glTexCoordFormatNV", (void*)&glTexCoordFormatNV},
    {"glTexCoordP1ui", (void*)&glTexCoordP1ui},
    {"glTexCoordP1uiv", (void*)&glTexCoordP1uiv},
    {"glTexCoordP2ui", (void*)&glTexCoordP2ui},
    {"glTexCoordP2uiv", (void*)&glTexCoordP2uiv},
    {"glTexCoordP3ui", (void*)&glTexCoordP3ui},
    {"glTexCoordP3uiv", (void*)&glTexCoordP3uiv},
    {"glTexCoordP4ui", (void*)&glTexCoordP4ui},
    {"glTexCoordP4uiv", (void*)&glTexCoordP4uiv},
    {"glTexCoordPointer", (void*)&glTexCoordPointer},
    {"glTexEnvf", (void*)&glTexEnvf},
    {"glTexEnvfv", (void*)&glTexEnvfv},
    {"glTexEnvi", (void*)&glTexEnvi},
    {"glTexEnviv", (void*)&glTexEnviv},
    {"glTexGend", (void*)&glTexGend},
    {"glTexGendv", (void*)&glTexGendv},
    {"glTexGenf", (void*)&glTexGenf},
    {"glTexGenfv", (void*)&glTexGenfv},
    {"glTexGeni", (void*)&glTexGeni},
    {"glTexGeniv", (void*)&glTexGeniv},
    {"glTexImage1D", (void*)&glTexImage1D},
    {"glTexImage2D", (void*)&glTexImage2D},
    {"glTexImage2DMultisample", (void*)&glTexImage2DMultisample},
    {"glTexImage3D", (void*)&glTexImage3D},
    {"glTexImage3DMultisample", (void*)&glTexImage3DMultisample},
    {"glTexPageCommitmentARB", (void*)&glTexPageCommitmentARB},
    {"glTexParameterIiv", (void*)&glTexParameterIiv},
    {"glTexParameterIivEXT", (void*)&glTexParameterIivEXT},
    {"glTexParameterIivOES", (void*)&glTexParameterIivOES},
    {"glTexParameterIuiv", (void*)&glTexParameterIuiv},
    {"glTexParameterIuivEXT", (void*)&glTexParameterIuivEXT},
    {"glTexParameterIuivOES", (void*)&glTexParameterIuivOES},
    {"glTexParameterf", (void*)&glTexParameterf},
    {"glTexParameterfv", (void*)&glTexParameterfv},
    {"glTexParameteri", (void*)&glTexParameteri},
    {"glTexParameteriv", (void*)&glTexParameteriv},
    {"glTexStorage1D", (void*)&glTexStorage1D},
    {"glTexStorage1DEXT", (void*)&glTexStorage1DEXT},
    {"glTexStorage2D", (void*)&glTexStorage2D},
    {"glTexStorage2DEXT", (void*)&glTexStorage2DEXT},
    {"glTexStorage2DMultisample", (void*)&glTexStorage2DMultisample},
    {"glTexStorage3D", (void*)&glTexStorage3D},
    {"glTexStorage3DEXT", (void*)&glTexStorage3DEXT},
    {"glTexStorage3DMultisample", (void*)&glTexStorage3DMultisample},
    {"glTexStorage3DMultisampleOES", (void*)&glTexStorage3DMultisampleOES},
    {"glTexSubImage1D", (void*)&glTexSubImage1D},
    {"glTexSubImage1DEXT", (void*)&glTexSubImage1DEXT},
    {"glTexSubImage2D", (void*)&glTexSubImage2D},
    {"glTexSubImage2DEXT", (void*)&glTexSubImage2DEXT},
    {"glTexSubImage3D", (void*)&glTexSubImage3D},
    {"glTexSubImage3DEXT", (void*)&glTexSubImage3DEXT},
    {"glTexSubImage3DOES", (void*)&glTexSubImage3DOES},
    {"glTextureAttachMemoryNV", (void*)&glTextureAttachMemoryNV},
    {"glTextureBarrier", (void*)&glTextureBarrier},
    {"glTextureBarrierNV", (void*)&glTextureBarrierNV},
    {"glTextureBuffer", (void*)&glTextureBuffer},
    {"glTextureBufferEXT", (void*)&glTextureBufferEXT},
    {"glTextureBufferRange", (void*)&glTextureBufferRange},
    {"glTextureBufferRangeEXT", (void*)&glTextureBufferRangeEXT},
    {"glTextureImage1DEXT", (void*)&glTextureImage1DEXT},
    {"glTextureImage2DEXT", (void*)&glTextureImage2DEXT},
    {"glTextureImage3DEXT", (void*)&glTextureImage3DEXT},
    {"glTexturePageCommitmentEXT", (void*)&glTexturePageCommitmentEXT},
    {"glTextureParameterIiv", (void*)&glTextureParameterIiv},
    {"glTextureParameterIivEXT", (void*)&glTextureParameterIivEXT},
    {"glTextureParameterIuiv", (void*)&glTextureParameterIuiv},
    {"glTextureParameterIuivEXT", (void*)&glTextureParameterIuivEXT},
    {"glTextureParameterf", (void*)&glTextureParameterf},
    {"glTextureParameterfEXT", (void*)&glTextureParameterfEXT},
    {"glTextureParameterfv", (void*)&glTextureParameterfv},
    {"glTextureParameterfvEXT", (void*)&glTextureParameterfvEXT},
    {"glTextureParameteri", (void*)&glTextureParameteri},
    {"glTextureParameteriEXT", (void*)&glTextureParameteriEXT},
    {"glTextureParameteriv", (void*)&glTextureParameteriv},
    {"glTextureParameterivEXT", (void*)&glTextureParameterivEXT},
    {"glTextureRenderbufferEXT", (void*)&glTextureRenderbufferEXT},
    {"glTextureStorage1D", (void*)&glTextureStorage1D},
    {"glTextureStorage1DEXT", (void*)&glTextureStorage1DEXT},
    {"glTextureStorage2D", (void*)&glTextureStorage2D},
    {"glTextureStorage2DEXT", (void*)&glTextureStorage2DEXT},
    {"glTextureStorage2DMultisample", (void*)&glTextureStorage2DMultisample},
    {"glTextureStorage2DMultisampleEXT", (void*)&glTextureStorage2DMultisampleEXT},
    {"glTextureStorage3D", (void*)&glTextureStorage3D},
    {"glTextureStorage3DEXT", (void*)&glTextureStorage3DEXT},
    {"glTextureStorage3DMultisample", (void*)&glTextureStorage3DMultisample},
    {"glTextureStorage3DMultisampleEXT", (void*)&glTextureStorage3DMultisampleEXT},
    {"glTextureSubImage1D", (void*)&glTextureSubImage1D},
    {"glTextureSubImage1DEXT", (void*)&glTextureSubImage1DEXT},
    {"glTextureSubImage2D", (void*)&glTextureSubImage2D},
    {"glTextureSubImage2DEXT", (void*)&glTextureSubImage2DEXT},
    {"glTextureSubImage3D", (void*)&glTextureSubImage3D},
    {"glTextureSubImage3DEXT", (void*)&glTextureSubImage3DEXT},
    {"glTextureView", (void*)&glTextureView},
    {"glTextureViewEXT", (void*)&glTextureViewEXT},
    {"glTextureViewOES", (void*)&glTextureViewOES},
    {"glTransformFeedbackBufferBase", (void*)&glTransformFeedbackBufferBase},
    {"glTransformFeedbackBufferRange", (void*)&glTransformFeedbackBufferRange},
    {"glTransformFeedbackVaryings", (void*)&glTransformFeedbackVaryings},
    {"glTransformFeedbackVaryingsEXT", (void*)&glTransformFeedbackVaryingsEXT},
    {"glTransformPathNV", (void*)&glTransformPathNV},
    {"glTranslated", (void*)&glTranslated},
    {"glTranslatef", (void*)&glTranslatef},
    {"glUniform1d", (void*)&glUniform1d},
    {"glUniform1dv", (void*)&glUniform1dv},
    {"glUniform1f", (void*)&glUniform1f},
    {"glUniform1fARB", (void*)&glUniform1fARB},
    {"glUniform1fv", (void*)&glUniform1fv},
    {"glUniform1fvARB", (void*)&glUniform1fvARB},
    {"glUniform1i", (void*)&glUniform1i},
    {"glUniform1iARB", (void*)&glUniform1iARB},
    {"glUniform1iv", (void*)&glUniform1iv},
    {"glUniform1ivARB", (void*)&glUniform1ivARB},
    {"glUniform1ui", (void*)&glUniform1ui},
    {"glUniform1uiEXT", (void*)&glUniform1uiEXT},
    {"glUniform1uiv", (void*)&glUniform1uiv},
    {"glUniform1uivEXT", (void*)&glUniform1uivEXT},
    {"glUniform2d", (void*)&glUniform2d},
    {"glUniform2dv", (void*)&glUniform2dv},
    {"glUniform2f", (void*)&glUniform2f},
    {"glUniform2fARB", (void*)&glUniform2fARB},
    {"glUniform2fv", (void*)&glUniform2fv},
    {"glUniform2fvARB", (void*)&glUniform2fvARB},
    {"glUniform2i", (void*)&glUniform2i},
    {"glUniform2iARB", (void*)&glUniform2iARB},
    {"glUniform2iv", (void*)&glUniform2iv},
    {"glUniform2ivARB", (void*)&glUniform2ivARB},
    {"glUniform2ui", (void*)&glUniform2ui},
    {"glUniform2uiEXT", (void*)&glUniform2uiEXT},
    {"glUniform2uiv", (void*)&glUniform2uiv},
    {"glUniform2uivEXT", (void*)&glUniform2uivEXT},
    {"glUniform3d", (void*)&glUniform3d},
    {"glUniform3dv", (void*)&glUniform3dv},
    {"glUniform3f", (void*)&glUniform3f},
    {"glUniform3fARB", (void*)&glUniform3fARB},
    {"glUniform3fv", (void*)&glUniform3fv},
    {"glUniform3fvARB", (void*)&glUniform3fvARB},
    {"glUniform3i", (void*)&glUniform3i},
    {"glUniform3iARB", (void*)&glUniform3iARB},
    {"glUniform3iv", (void*)&glUniform3iv},
    {"glUniform3ivARB", (void*)&glUniform3ivARB},
    {"glUniform3ui", (void*)&glUniform3ui},
    {"glUniform3uiEXT", (void*)&glUniform3uiEXT},
    {"glUniform3uiv", (void*)&glUniform3uiv},
    {"glUniform3uivEXT", (void*)&glUniform3uivEXT},
    {"glUniform4d", (void*)&glUniform4d},
    {"glUniform4dv", (void*)&glUniform4dv},
    {"glUniform4f", (void*)&glUniform4f},
    {"glUniform4fARB", (void*)&glUniform4fARB},
    {"glUniform4fv", (void*)&glUniform4fv},
    {"glUniform4fvARB", (void*)&glUniform4fvARB},
    {"glUniform4i", (void*)&glUniform4i},
    {"glUniform4iARB", (void*)&glUniform4iARB},
    {"glUniform4iv", (void*)&glUniform4iv},
    {"glUniform4ivARB", (void*)&glUniform4ivARB},
    {"glUniform4ui", (void*)&glUniform4ui},
    {"glUniform4uiEXT", (void*)&glUniform4uiEXT},
    {"glUniform4uiv", (void*)&glUniform4uiv},
    {"glUniform4uivEXT", (void*)&glUniform4uivEXT},
    {"glUniformBlockBinding", (void*)&glUniformBlockBinding},
    {"glUniformHandleui64ARB", (void*)&glUniformHandleui64ARB},
    {"glUniformHandleui64NV", (void*)&glUniformHandleui64NV},
    {"glUniformHandleui64vARB", (void*)&glUniformHandleui64vARB},
    {"glUniformHandleui64vNV", (void*)&glUniformHandleui64vNV},
    {"glUniformMatrix2dv", (void*)&glUniformMatrix2dv},
    {"glUniformMatrix2fv", (void*)&glUniformMatrix2fv},
    {"glUniformMatrix2fvARB", (void*)&glUniformMatrix2fvARB},
    {"glUniformMatrix2x3dv", (void*)&glUniformMatrix2x3dv},
    {"glUniformMatrix2x3fv", (void*)&glUniformMatrix2x3fv},
    {"glUniformMatrix2x3fvNV", (void*)&glUniformMatrix2x3fvNV},
    {"glUniformMatrix2x4dv", (void*)&glUniformMatrix2x4dv},
    {"glUniformMatrix2x4fv", (void*)&glUniformMatrix2x4fv},
    {"glUniformMatrix2x4fvNV", (void*)&glUniformMatrix2x4fvNV},
    {"glUniformMatrix3dv", (void*)&glUniformMatrix3dv},
    {"glUniformMatrix3fv", (void*)&glUniformMatrix3fv},
    {"glUniformMatrix3fvARB", (void*)&glUniformMatrix3fvARB},
    {"glUniformMatrix3x2dv", (void*)&glUniformMatrix3x2dv},
    {"glUniformMatrix3x2fv", (void*)&glUniformMatrix3x2fv},
    {"glUniformMatrix3x2fvNV", (void*)&glUniformMatrix3x2fvNV},
    {"glUniformMatrix3x4dv", (void*)&glUniformMatrix3x4dv},
    {"glUniformMatrix3x4fv", (void*)&glUniformMatrix3x4fv},
    {"glUniformMatrix3x4fvNV", (void*)&glUniformMatrix3x4fvNV},
    {"glUniformMatrix4dv", (void*)&glUniformMatrix4dv},
    {"glUniformMatrix4fv", (void*)&glUniformMatrix4fv},
    {"glUniformMatrix4fvARB", (void*)&glUniformMatrix4fvARB},
    {"glUniformMatrix4x2dv", (void*)&glUniformMatrix4x2dv},
    {"glUniformMatrix4x2fv", (void*)&glUniformMatrix4x2fv},
    {"glUniformMatrix4x2fvNV", (void*)&glUniformMatrix4x2fvNV},
    {"glUniformMatrix4x3dv", (void*)&glUniformMatrix4x3dv},
    {"glUniformMatrix4x3fv", (void*)&glUniformMatrix4x3fv},
    {"glUniformMatrix4x3fvNV", (void*)&glUniformMatrix4x3fvNV},
    {"glUniformSubroutinesuiv", (void*)&glUniformSubroutinesuiv},
    {"glUniformui64NV", (void*)&glUniformui64NV},
    {"glUniformui64vNV", (void*)&glUniformui64vNV},
    {"glUnmapBuffer", (void*)&glUnmapBuffer},
    {"glUnmapBufferARB", (void*)&glUnmapBufferARB},
    {"glUnmapBufferOES", (void*)&glUnmapBufferOES},
    {"glUnmapNamedBuffer", (void*)&glUnmapNamedBuffer},
    {"glUnmapNamedBufferEXT", (void*)&glUnmapNamedBufferEXT},
    {"glUseProgram", (void*)&glUseProgram},
    {"glUseProgramObjectARB", (void*)&glUseProgramObjectARB},
    {"glUseProgramStages", (void*)&glUseProgramStages},
    {"glUseProgramStagesEXT", (void*)&glUseProgramStagesEXT},
    {"glUseShaderProgramEXT", (void*)&glUseShaderProgramEXT},
    {"glValidateProgram", (void*)&glValidateProgram},
    {"glValidateProgramARB", (void*)&glValidateProgramARB},
    {"glValidateProgramPipeline", (void*)&glValidateProgramPipeline},
    {"glValidateProgramPipelineEXT", (void*)&glValidateProgramPipelineEXT},
    {"glVertex2d", (void*)&glVertex2d},
    {"glVertex2dv", (void*)&glVertex2dv},
    {"glVertex2f", (void*)&glVertex2f},
    {"glVertex2fv", (void*)&glVertex2fv},
    {"glVertex2i", (void*)&glVertex2i},
    {"glVertex2iv", (void*)&glVertex2iv},
    {"glVertex2s", (void*)&glVertex2s},
    {"glVertex2sv", (void*)&glVertex2sv},
    {"glVertex3d", (void*)&glVertex3d},
    {"glVertex3dv", (void*)&glVertex3dv},
    {"glVertex3f", (void*)&glVertex3f},
    {"glVertex3fv", (void*)&glVertex3fv},
    {"glVertex3i", (void*)&glVertex3i},
    {"glVertex3iv", (void*)&glVertex3iv},
    {"glVertex3s", (void*)&glVertex3s},
    {"glVertex3sv", (void*)&glVertex3sv},
    {"glVertex4d", (void*)&glVertex4d},
    {"glVertex4dv", (void*)&glVertex4dv},
    {"glVertex4f", (void*)&glVertex4f},
    {"glVertex4fv", (void*)&glVertex4fv},
    {"glVertex4i", (void*)&glVertex4i},
    {"glVertex4iv", (void*)&glVertex4iv},
    {"glVertex4s", (void*)&glVertex4s},
    {"glVertex4sv", (void*)&glVertex4sv},
    {"glVertexArrayAttribBinding", (void*)&glVertexArrayAttribBinding},
    {"glVertexArrayAttribFormat", (void*)&glVertexArrayAttribFormat},
    {"glVertexArrayAttribIFormat", (void*)&glVertexArrayAttribIFormat},
    {"glVertexArrayAttribLFormat", (void*)&glVertexArrayAttribLFormat},
    {"glVertexArrayBindVertexBufferEXT", (void*)&glVertexArrayBindVertexBufferEXT},
    {"glVertexArrayBindingDivisor", (void*)&glVertexArrayBindingDivisor},
    {"glVertexArrayColorOffsetEXT", (void*)&glVertexArrayColorOffsetEXT},
    {"glVertexArrayEdgeFlagOffsetEXT", (void*)&glVertexArrayEdgeFlagOffsetEXT},
    {"glVertexArrayElementBuffer", (void*)&glVertexArrayElementBuffer},
    {"glVertexArrayFogCoordOffsetEXT", (void*)&glVertexArrayFogCoordOffsetEXT},
    {"glVertexArrayIndexOffsetEXT", (void*)&glVertexArrayIndexOffsetEXT},
    {"glVertexArrayMultiTexCoordOffsetEXT", (void*)&glVertexArrayMultiTexCoordOffsetEXT},
    {"glVertexArrayNormalOffsetEXT", (void*)&glVertexArrayNormalOffsetEXT},
    {"glVertexArraySecondaryColorOffsetEXT", (void*)&glVertexArraySecondaryColorOffsetEXT},
    {"glVertexArrayTexCoordOffsetEXT", (void*)&glVertexArrayTexCoordOffsetEXT},
    {"glVertexArrayVertexAttribBindingEXT", (void*)&glVertexArrayVertexAttribBindingEXT},
    {"glVertexArrayVertexAttribDivisorEXT", (void*)&glVertexArrayVertexAttribDivisorEXT},
    {"glVertexArrayVertexAttribFormatEXT", (void*)&glVertexArrayVertexAttribFormatEXT},
    {"glVertexArrayVertexAttribIFormatEXT", (void*)&glVertexArrayVertexAttribIFormatEXT},
    {"glVertexArrayVertexAttribIOffsetEXT", (void*)&glVertexArrayVertexAttribIOffsetEXT},
    {"glVertexArrayVertexAttribLFormatEXT", (void*)&glVertexArrayVertexAttribLFormatEXT},
    {"glVertexArrayVertexAttribLOffsetEXT", (void*)&glVertexArrayVertexAttribLOffsetEXT},
    {"glVertexArrayVertexAttribOffsetEXT", (void*)&glVertexArrayVertexAttribOffsetEXT},
    {"glVertexArrayVertexBindingDivisorEXT", (void*)&glVertexArrayVertexBindingDivisorEXT},
    {"glVertexArrayVertexBuffer", (void*)&glVertexArrayVertexBuffer},
    {"glVertexArrayVertexBuffers", (void*)&glVertexArrayVertexBuffers},
    {"glVertexArrayVertexOffsetEXT", (void*)&glVertexArrayVertexOffsetEXT},
    {"glVertexAttrib1d", (void*)&glVertexAttrib1d},
    {"glVertexAttrib1dARB", (void*)&glVertexAttrib1dARB},
    {"glVertexAttrib1dNV", (void*)&glVertexAttrib1dNV},
    {"glVertexAttrib1dv", (void*)&glVertexAttrib1dv},
    {"glVertexAttrib1dvARB", (void*)&glVertexAttrib1dvARB},
    {"glVertexAttrib1dvNV", (void*)&glVertexAttrib1dvNV},
    {"glVertexAttrib1f", (void*)&glVertexAttrib1f},
    {"glVertexAttrib1fARB", (void*)&glVertexAttrib1fARB},
    {"glVertexAttrib1fNV", (void*)&glVertexAttrib1fNV},
    {"glVertexAttrib1fv", (void*)&glVertexAttrib1fv},
    {"glVertexAttrib1fvARB", (void*)&glVertexAttrib1fvARB},
    {"glVertexAttrib1fvNV", (void*)&glVertexAttrib1fvNV},
    {"glVertexAttrib1s", (void*)&glVertexAttrib1s},
    {"glVertexAttrib1sARB", (void*)&glVertexAttrib1sARB},
    {"glVertexAttrib1sNV", (void*)&glVertexAttrib1sNV},
    {"glVertexAttrib1sv", (void*)&glVertexAttrib1sv},
    {"glVertexAttrib1svARB", (void*)&glVertexAttrib1svARB},
    {"glVertexAttrib1svNV", (void*)&glVertexAttrib1svNV},
    {"glVertexAttrib2d", (void*)&glVertexAttrib2d},
    {"glVertexAttrib2dARB", (void*)&glVertexAttrib2dARB},
    {"glVertexAttrib2dNV", (void*)&glVertexAttrib2dNV},
    {"glVertexAttrib2dv", (void*)&glVertexAttrib2dv},
    {"glVertexAttrib2dvARB", (void*)&glVertexAttrib2dvARB},
    {"glVertexAttrib2dvNV", (void*)&glVertexAttrib2dvNV},
    {"glVertexAttrib2f", (void*)&glVertexAttrib2f},
    {"glVertexAttrib2fARB", (void*)&glVertexAttrib2fARB},
    {"glVertexAttrib2fNV", (void*)&glVertexAttrib2fNV},
    {"glVertexAttrib2fv", (void*)&glVertexAttrib2fv},
    {"glVertexAttrib2fvARB", (void*)&glVertexAttrib2fvARB},
    {"glVertexAttrib2fvNV", (void*)&glVertexAttrib2fvNV},
    {"glVertexAttrib2s", (void*)&glVertexAttrib2s},
    {"glVertexAttrib2sARB", (void*)&glVertexAttrib2sARB},
    {"glVertexAttrib2sNV", (void*)&glVertexAttrib2sNV},
    {"glVertexAttrib2sv", (void*)&glVertexAttrib2sv},
    {"glVertexAttrib2svARB", (void*)&glVertexAttrib2svARB},
    {"glVertexAttrib2svNV", (void*)&glVertexAttrib2svNV},
    {"glVertexAttrib3d", (void*)&glVertexAttrib3d},
    {"glVertexAttrib3dARB", (void*)&glVertexAttrib3dARB},
    {"glVertexAttrib3dNV", (void*)&glVertexAttrib3dNV},
    {"glVertexAttrib3dv", (void*)&glVertexAttrib3dv},
    {"glVertexAttrib3dvARB", (void*)&glVertexAttrib3dvARB},
    {"glVertexAttrib3dvNV", (void*)&glVertexAttrib3dvNV},
    {"glVertexAttrib3f", (void*)&glVertexAttrib3f},
    {"glVertexAttrib3fARB", (void*)&glVertexAttrib3fARB},
    {"glVertexAttrib3fNV", (void*)&glVertexAttrib3fNV},
    {"glVertexAttrib3fv", (void*)&glVertexAttrib3fv},
    {"glVertexAttrib3fvARB", (void*)&glVertexAttrib3fvARB},
    {"glVertexAttrib3fvNV", (void*)&glVertexAttrib3fvNV},
    {"glVertexAttrib3s", (void*)&glVertexAttrib3s},
    {"glVertexAttrib3sARB", (void*)&glVertexAttrib3sARB},
    {"glVertexAttrib3sNV", (void*)&glVertexAttrib3sNV},
    {"glVertexAttrib3sv", (void*)&glVertexAttrib3sv},
    {"glVertexAttrib3svARB", (void*)&glVertexAttrib3svARB},
    {"glVertexAttrib3svNV", (void*)&glVertexAttrib3svNV},
    {"glVertexAttrib4Nbv", (void*)&glVertexAttrib4Nbv},
    {"glVertexAttrib4NbvARB", (void*)&glVertexAttrib4NbvARB},
    {"glVertexAttrib4Niv", (void*)&glVertexAttrib4Niv},
    {"glVertexAttrib4NivARB", (void*)&glVertexAttrib4NivARB},
    {"glVertexAttrib4Nsv", (void*)&glVertexAttrib4Nsv},
    {"glVertexAttrib4NsvARB", (void*)&glVertexAttrib4NsvARB},
    {"glVertexAttrib4Nub", (void*)&glVertexAttrib4Nub},
    {"glVertexAttrib4NubARB", (void*)&glVertexAttrib4NubARB},
    {"glVertexAttrib4Nubv", (void*)&glVertexAttrib4Nubv},
    {"glVertexAttrib4NubvARB", (void*)&glVertexAttrib4NubvARB},
    {"glVertexAttrib4Nuiv", (void*)&glVertexAttrib4Nuiv},
    {"glVertexAttrib4NuivARB", (void*)&glVertexAttrib4NuivARB},
    {"glVertexAttrib4Nusv", (void*)&glVertexAttrib4Nusv},
    {"glVertexAttrib4NusvARB", (void*)&glVertexAttrib4NusvARB},
    {"glVertexAttrib4bv", (void*)&glVertexAttrib4bv},
    {"glVertexAttrib4bvARB", (void*)&glVertexAttrib4bvARB},
    {"glVertexAttrib4d", (void*)&glVertexAttrib4d},
    {"glVertexAttrib4dARB", (void*)&glVertexAttrib4dARB},
    {"glVertexAttrib4dNV", (void*)&glVertexAttrib4dNV},
    {"glVertexAttrib4dv", (void*)&glVertexAttrib4dv},
    {"glVertexAttrib4dvARB", (void*)&glVertexAttrib4dvARB},
    {"glVertexAttrib4dvNV", (void*)&glVertexAttrib4dvNV},
    {"glVertexAttrib4f", (void*)&glVertexAttrib4f},
    {"glVertexAttrib4fARB", (void*)&glVertexAttrib4fARB},
    {"glVertexAttrib4fNV", (void*)&glVertexAttrib4fNV},
    {"glVertexAttrib4fv", (void*)&glVertexAttrib4fv},
    {"glVertexAttrib4fvARB", (void*)&glVertexAttrib4fvARB},
    {"glVertexAttrib4fvNV", (void*)&glVertexAttrib4fvNV},
    {"glVertexAttrib4iv", (void*)&glVertexAttrib4iv},
    {"glVertexAttrib4ivARB", (void*)&glVertexAttrib4ivARB},
    {"glVertexAttrib4s", (void*)&glVertexAttrib4s},
    {"glVertexAttrib4sARB", (void*)&glVertexAttrib4sARB},
    {"glVertexAttrib4sNV", (void*)&glVertexAttrib4sNV},
    {"glVertexAttrib4sv", (void*)&glVertexAttrib4sv},
    {"glVertexAttrib4svARB", (void*)&glVertexAttrib4svARB},
    {"glVertexAttrib4svNV", (void*)&glVertexAttrib4svNV},
    {"glVertexAttrib4ubv", (void*)&glVertexAttrib4ubv},
    {"glVertexAttrib4ubvARB", (void*)&glVertexAttrib4ubvARB},
    {"glVertexAttrib4ubvNV", (void*)&glVertexAttrib4ubvNV},
    {"glVertexAttrib4uiv", (void*)&glVertexAttrib4uiv},
    {"glVertexAttrib4uivARB", (void*)&glVertexAttrib4uivARB},
    {"glVertexAttrib4usv", (void*)&glVertexAttrib4usv},
    {"glVertexAttrib4usvARB", (void*)&glVertexAttrib4usvARB},
    {"glVertexAttribBinding", (void*)&glVertexAttribBinding},
    {"glVertexAttribDivisor", (void*)&glVertexAttribDivisor},
    {"glVertexAttribDivisorANGLE", (void*)&glVertexAttribDivisorANGLE},
    {"glVertexAttribDivisorARB", (void*)&glVertexAttribDivisorARB},
    {"glVertexAttribDivisorEXT", (void*)&glVertexAttribDivisorEXT},
    {"glVertexAttribDivisorNV", (void*)&glVertexAttribDivisorNV},
    {"glVertexAttribFormat", (void*)&glVertexAttribFormat},
    {"glVertexAttribFormatNV", (void*)&glVertexAttribFormatNV},
    {"glVertexAttribI1i", (void*)&glVertexAttribI1i},
    {"glVertexAttribI1iEXT", (void*)&glVertexAttribI1iEXT},
    {"glVertexAttribI1iv", (void*)&glVertexAttribI1iv},
    {"glVertexAttribI1ivEXT", (void*)&glVertexAttribI1ivEXT},
    {"glVertexAttribI1ui", (void*)&glVertexAttribI1ui},
    {"glVertexAttribI1uiEXT", (void*)&glVertexAttribI1uiEXT},
    {"glVertexAttribI1uiv", (void*)&glVertexAttribI1uiv},
    {"glVertexAttribI1uivEXT", (void*)&glVertexAttribI1uivEXT},
    {"glVertexAttribI2i", (void*)&glVertexAttribI2i},
    {"glVertexAttribI2iEXT", (void*)&glVertexAttribI2iEXT},
    {"glVertexAttribI2iv", (void*)&glVertexAttribI2iv},
    {"glVertexAttribI2ivEXT", (void*)&glVertexAttribI2ivEXT},
    {"glVertexAttribI2ui", (void*)&glVertexAttribI2ui},
    {"glVertexAttribI2uiEXT", (void*)&glVertexAttribI2uiEXT},
    {"glVertexAttribI2uiv", (void*)&glVertexAttribI2uiv},
    {"glVertexAttribI2uivEXT", (void*)&glVertexAttribI2uivEXT},
    {"glVertexAttribI3i", (void*)&glVertexAttribI3i},
    {"glVertexAttribI3iEXT", (void*)&glVertexAttribI3iEXT},
    {"glVertexAttribI3iv", (void*)&glVertexAttribI3iv},
    {"glVertexAttribI3ivEXT", (void*)&glVertexAttribI3ivEXT},
    {"glVertexAttribI3ui", (void*)&glVertexAttribI3ui},
    {"glVertexAttribI3uiEXT", (void*)&glVertexAttribI3uiEXT},
    {"glVertexAttribI3uiv", (void*)&glVertexAttribI3uiv},
    {"glVertexAttribI3uivEXT", (void*)&glVertexAttribI3uivEXT},
    {"glVertexAttribI4bv", (void*)&glVertexAttribI4bv},
    {"glVertexAttribI4bvEXT", (void*)&glVertexAttribI4bvEXT},
    {"glVertexAttribI4i", (void*)&glVertexAttribI4i},
    {"glVertexAttribI4iEXT", (void*)&glVertexAttribI4iEXT},
    {"glVertexAttribI4iv", (void*)&glVertexAttribI4iv},
    {"glVertexAttribI4ivEXT", (void*)&glVertexAttribI4ivEXT},
    {"glVertexAttribI4sv", (void*)&glVertexAttribI4sv},
    {"glVertexAttribI4svEXT", (void*)&glVertexAttribI4svEXT},
    {"glVertexAttribI4ubv", (void*)&glVertexAttribI4ubv},
    {"glVertexAttribI4ubvEXT", (void*)&glVertexAttribI4ubvEXT},
    {"glVertexAttribI4ui", (void*)&glVertexAttribI4ui},
    {"glVertexAttribI4uiEXT", (void*)&glVertexAttribI4uiEXT},
    {"glVertexAttribI4uiv", (void*)&glVertexAttribI4uiv},
    {"glVertexAttribI4uivEXT", (void*)&glVertexAttribI4uivEXT},
    {"glVertexAttribI4usv", (void*)&glVertexAttribI4usv},
    {"glVertexAttribI4usvEXT", (void*)&glVertexAttribI4usvEXT},
    {"glVertexAttribIFormat", (void*)&glVertexAttribIFormat},
    {"glVertexAttribIFormatNV", (void*)&glVertexAttribIFormatNV},
    {"glVertexAttribIPointer", (void*)&glVertexAttribIPointer},
    {"glVertexAttribIPointerEXT", (void*)&glVertexAttribIPointerEXT},
    {"glVertexAttribL1d", (void*)&glVertexAttribL1d},
    {"glVertexAttribL1dEXT", (void*)&glVertexAttribL1dEXT},
    {"glVertexAttribL1dv", (void*)&glVertexAttribL1dv},
    {"glVertexAttribL1dvEXT", (void*)&glVertexAttribL1dvEXT},
    {"glVertexAttribL1i64NV", (void*)&glVertexAttribL1i64NV},
    {"glVertexAttribL1i64vNV", (void*)&glVertexAttribL1i64vNV},
    {"glVertexAttribL1ui64ARB", (void*)&glVertexAttribL1ui64ARB},
    {"glVertexAttribL1ui64NV", (void*)&glVertexAttribL1ui64NV},
    {"glVertexAttribL1ui64vARB", (void*)&glVertexAttribL1ui64vARB},
    {"glVertexAttribL1ui64vNV", (void*)&glVertexAttribL1ui64vNV},
    {"glVertexAttribL2d", (void*)&glVertexAttribL2d},
    {"glVertexAttribL2dEXT", (void*)&glVertexAttribL2dEXT},
    {"glVertexAttribL2dv", (void*)&glVertexAttribL2dv},
    {"glVertexAttribL2dvEXT", (void*)&glVertexAttribL2dvEXT},
    {"glVertexAttribL2i64NV", (void*)&glVertexAttribL2i64NV},
    {"glVertexAttribL2i64vNV", (void*)&glVertexAttribL2i64vNV},
    {"glVertexAttribL2ui64NV", (void*)&glVertexAttribL2ui64NV},
    {"glVertexAttribL2ui64vNV", (void*)&glVertexAttribL2ui64vNV},
    {"glVertexAttribL3d", (void*)&glVertexAttribL3d},
    {"glVertexAttribL3dEXT", (void*)&glVertexAttribL3dEXT},
    {"glVertexAttribL3dv", (void*)&glVertexAttribL3dv},
    {"glVertexAttribL3dvEXT", (void*)&glVertexAttribL3dvEXT},
    {"glVertexAttribL3i64NV", (void*)&glVertexAttribL3i64NV},
    {"glVertexAttribL3i64vNV", (void*)&glVertexAttribL3i64vNV},
    {"glVertexAttribL3ui64NV", (void*)&glVertexAttribL3ui64NV},
    {"glVertexAttribL3ui64vNV", (void*)&glVertexAttribL3ui64vNV},
    {"glVertexAttribL4d", (void*)&glVertexAttribL4d},
    {"glVertexAttribL4dEXT", (void*)&glVertexAttribL4dEXT},
    {"glVertexAttribL4dv", (void*)&glVertexAttribL4dv},
    {"glVertexAttribL4dvEXT", (void*)&glVertexAttribL4dvEXT},
    {"glVertexAttribL4i64NV", (void*)&glVertexAttribL4i64NV},
    {"glVertexAttribL4i64vNV", (void*)&glVertexAttribL4i64vNV},
    {"glVertexAttribL4ui64NV", (void*)&glVertexAttribL4ui64NV},
    {"glVertexAttribL4ui64vNV", (void*)&glVertexAttribL4ui64vNV},
    {"glVertexAttribLFormat", (void*)&glVertexAttribLFormat},
    {"glVertexAttribLFormatNV", (void*)&glVertexAttribLFormatNV},
    {"glVertexAttribLPointer", (void*)&glVertexAttribLPointer},
    {"glVertexAttribLPointerEXT", (void*)&glVertexAttribLPointerEXT},
    {"glVertexAttribP1ui", (void*)&glVertexAttribP1ui},
    {"glVertexAttribP1uiv", (void*)&glVertexAttribP1uiv},
    {"glVertexAttribP2ui", (void*)&glVertexAttribP2ui},
    {"glVertexAttribP2uiv", (void*)&glVertexAttribP2uiv},
    {"glVertexAttribP3ui", (void*)&glVertexAttribP3ui},
    {"glVertexAttribP3uiv", (void*)&glVertexAttribP3uiv},
    {"glVertexAttribP4ui", (void*)&glVertexAttribP4ui},
    {"glVertexAttribP4uiv", (void*)&glVertexAttribP4uiv},
    {"glVertexAttribPointer", (void*)&glVertexAttribPointer},
    {"glVertexAttribPointerARB", (void*)&glVertexAttribPointerARB},
    {"glVertexBindingDivisor", (void*)&glVertexBindingDivisor},
    {"glVertexFormatNV", (void*)&glVertexFormatNV},
    {"glVertexP2ui", (void*)&glVertexP2ui},
    {"glVertexP2uiv", (void*)&glVertexP2uiv},
    {"glVertexP3ui", (void*)&glVertexP3ui},
    {"glVertexP3uiv", (void*)&glVertexP3uiv},
    {"glVertexP4ui", (void*)&glVertexP4ui},
    {"glVertexP4uiv", (void*)&glVertexP4uiv},
    {"glVertexPointer", (void*)&glVertexPointer},
    {"glViewport", (void*)&glViewport},
    {"glViewportArrayv", (void*)&glViewportArrayv},
    {"glViewportArrayvNV", (void*)&glViewportArrayvNV},
    {"glViewportArrayvOES", (void*)&glViewportArrayvOES},
    {"glViewportIndexedf", (void*)&glViewportIndexedf},
    {"glViewportIndexedfNV", (void*)&glViewportIndexedfNV},
    {"glViewportIndexedfOES", (void*)&glViewportIndexedfOES},
    {"glViewportIndexedfv", (void*)&glViewportIndexedfv},
    {"glViewportIndexedfvNV", (void*)&glViewportIndexedfvNV},
    {"glViewportIndexedfvOES", (void*)&glViewportIndexedfvOES},
    {"glViewportPositionWScaleNV", (void*)&glViewportPositionWScaleNV},
    {"glViewportSwizzleNV", (void*)&glViewportSwizzleNV},
    {"glWaitSync", (void*)&glWaitSync},
    {"glWaitSyncAPPLE", (void*)&glWaitSyncAPPLE},
    {"glWeightPathsNV", (void*)&glWeightPathsNV},
    {"glWindowPos2d", (void*)&glWindowPos2d},
    {"glWindowPos2dv", (void*)&glWindowPos2dv},
    {"glWindowPos2f", (void*)&glWindowPos2f},
    {"glWindowPos2fv", (void*)&glWindowPos2fv},
    {"glWindowPos2i", (void*)&glWindowPos2i},
    {"glWindowPos2iv", (void*)&glWindowPos2iv},
    {"glWindowPos2s", (void*)&glWindowPos2s},
    {"glWindowPos2sv", (void*)&glWindowPos2sv},
    {"glWindowPos3d", (void*)&glWindowPos3d},
    {"glWindowPos3dv", (void*)&glWindowPos3dv},
    {"glWindowPos3f", (void*)&glWindowPos3f},
    {"glWindowPos3fv", (void*)&glWindowPos3fv},
    {"glWindowPos3i", (void*)&glWindowPos3i},
    {"glWindowPos3iv", (void*)&glWindowPos3iv},
    {"glWindowPos3s", (void*)&glWindowPos3s},
    {"glWindowPos3sv", (void*)&glWindowPos3sv},
    {"glWindowRectanglesEXT", (void*)&glWindowRectanglesEXT},
};
const int kTglmtProcCount = 2115;
} // namespace tglmt
