// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.
// 657 hàm OpenGL 4.6 CORE Profile trong namespace tglmt::gl.
// + 42 hàm chỉ có ở COMPATIBILITY profile (không phải core).
// Nguồn đếm: tools/gl46_catalog.py (khronos reg.py PASS 1 semantics).
#pragma once
#include "tglmt/gl46_types.h"

namespace tglmt { namespace gl {
void glActiveShaderProgram(GLuint pipeline, GLuint program); // introduced 4.1
void glActiveTexture(GLenum texture); // introduced 1.3
void glAttachShader(GLuint program, GLuint shader); // introduced 2.0
void glBeginConditionalRender(GLuint id, GLenum mode); // introduced 3.0
void glBeginQuery(GLenum target, GLuint id); // introduced 1.5
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id); // introduced 4.0
void glBeginTransformFeedback(GLenum primitiveMode); // introduced 3.0
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name); // introduced 2.0
void glBindBuffer(GLenum target, GLuint buffer); // introduced 1.5
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer); // introduced 3.0
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced 3.0
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers); // introduced 4.4
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes); // introduced 4.4
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name); // introduced 3.0
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name); // introduced 3.3
void glBindFramebuffer(GLenum target, GLuint framebuffer); // introduced 3.0
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format); // introduced 4.2
void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures); // introduced 4.4
void glBindProgramPipeline(GLuint pipeline); // introduced 4.1
void glBindRenderbuffer(GLenum target, GLuint renderbuffer); // introduced 3.0
void glBindSampler(GLuint unit, GLuint sampler); // introduced 3.3
void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers); // introduced 4.4
void glBindTexture(GLenum target, GLuint texture); // introduced 1.1
void glBindTextureUnit(GLuint unit, GLuint texture); // introduced 4.5
void glBindTextures(GLuint first, GLsizei count, const GLuint *textures); // introduced 4.4
void glBindTransformFeedback(GLenum target, GLuint id); // introduced 4.0
void glBindVertexArray(GLuint array); // introduced 3.0
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride); // introduced 4.3
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides); // introduced 4.4
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // introduced 1.4
void glBlendEquation(GLenum mode); // introduced 1.4
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha); // introduced 2.0
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha); // introduced 4.0
void glBlendEquationi(GLuint buf, GLenum mode); // introduced 4.0
void glBlendFunc(GLenum sfactor, GLenum dfactor); // introduced 1.0
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha); // introduced 1.4
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha); // introduced 4.0
void glBlendFunci(GLuint buf, GLenum src, GLenum dst); // introduced 4.0
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter); // introduced 3.0
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter); // introduced 4.5
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage); // introduced 1.5
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags); // introduced 4.4
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data); // introduced 1.5
GLenum glCheckFramebufferStatus(GLenum target); // introduced 3.0
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target); // introduced 4.5
void glClampColor(GLenum target, GLenum clamp); // introduced 3.0
void glClear(GLbitfield mask); // introduced 1.0
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data); // introduced 4.3
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data); // introduced 4.3
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil); // introduced 3.0
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value); // introduced 3.0
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value); // introduced 3.0
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value); // introduced 3.0
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // introduced 1.0
void glClearDepth(GLdouble depth); // introduced 1.0
void glClearDepthf(GLfloat d); // introduced 4.1
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data); // introduced 4.5
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data); // introduced 4.5
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil); // introduced 4.5
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value); // introduced 4.5
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value); // introduced 4.5
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value); // introduced 4.5
void glClearStencil(GLint s); // introduced 1.0
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data); // introduced 4.4
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data); // introduced 4.4
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout); // introduced 3.2
void glClipControl(GLenum origin, GLenum depth); // introduced 4.5
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha); // introduced 1.0
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a); // introduced 3.0
void glCompileShader(GLuint shader); // introduced 2.0
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data); // introduced 1.3
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data); // introduced 4.5
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data); // introduced 4.5
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data); // introduced 4.5
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size); // introduced 3.1
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth); // introduced 4.3
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size); // introduced 4.5
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border); // introduced 1.1
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border); // introduced 1.1
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); // introduced 1.1
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 1.1
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 1.2
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); // introduced 4.5
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 4.5
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 4.5
void glCreateBuffers(GLsizei n, GLuint *buffers); // introduced 4.5
void glCreateFramebuffers(GLsizei n, GLuint *framebuffers); // introduced 4.5
GLuint glCreateProgram(void); // introduced 2.0
void glCreateProgramPipelines(GLsizei n, GLuint *pipelines); // introduced 4.5
void glCreateQueries(GLenum target, GLsizei n, GLuint *ids); // introduced 4.5
void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers); // introduced 4.5
void glCreateSamplers(GLsizei n, GLuint *samplers); // introduced 4.5
GLuint glCreateShader(GLenum type); // introduced 2.0
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings); // introduced 4.1
void glCreateTextures(GLenum target, GLsizei n, GLuint *textures); // introduced 4.5
void glCreateTransformFeedbacks(GLsizei n, GLuint *ids); // introduced 4.5
void glCreateVertexArrays(GLsizei n, GLuint *arrays); // introduced 4.5
void glCullFace(GLenum mode); // introduced 1.0
void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam); // introduced 4.3
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled); // introduced 4.3
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf); // introduced 4.3
void glDeleteBuffers(GLsizei n, const GLuint *buffers); // introduced 1.5
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers); // introduced 3.0
void glDeleteProgram(GLuint program); // introduced 2.0
void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines); // introduced 4.1
void glDeleteQueries(GLsizei n, const GLuint *ids); // introduced 1.5
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers); // introduced 3.0
void glDeleteSamplers(GLsizei count, const GLuint *samplers); // introduced 3.3
void glDeleteShader(GLuint shader); // introduced 2.0
void glDeleteSync(GLsync sync); // introduced 3.2
void glDeleteTextures(GLsizei n, const GLuint *textures); // introduced 1.1
void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids); // introduced 4.0
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays); // introduced 3.0
void glDepthFunc(GLenum func); // introduced 1.0
void glDepthMask(GLboolean flag); // introduced 1.0
void glDepthRange(GLdouble n, GLdouble f); // introduced 1.0
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v); // introduced 4.1
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f); // introduced 4.1
void glDepthRangef(GLfloat n, GLfloat f); // introduced 4.1
void glDetachShader(GLuint program, GLuint shader); // introduced 2.0
void glDisable(GLenum cap); // introduced 1.0
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index); // introduced 4.5
void glDisableVertexAttribArray(GLuint index); // introduced 2.0
void glDisablei(GLenum target, GLuint index); // introduced 3.0
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z); // introduced 4.3
void glDispatchComputeIndirect(GLintptr indirect); // introduced 4.3
void glDrawArrays(GLenum mode, GLint first, GLsizei count); // introduced 1.1
void glDrawArraysIndirect(GLenum mode, const void *indirect); // introduced 4.0
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount); // introduced 3.1
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance); // introduced 4.2
void glDrawBuffer(GLenum buf); // introduced 1.0
void glDrawBuffers(GLsizei n, const GLenum *bufs); // introduced 2.0
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices); // introduced 1.1
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex); // introduced 3.2
void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect); // introduced 4.0
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount); // introduced 3.1
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance); // introduced 4.2
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex); // introduced 3.2
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance); // introduced 4.2
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices); // introduced 1.2
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex); // introduced 3.2
void glDrawTransformFeedback(GLenum mode, GLuint id); // introduced 4.0
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount); // introduced 4.2
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream); // introduced 4.0
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount); // introduced 4.2
void glEnable(GLenum cap); // introduced 1.0
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index); // introduced 4.5
void glEnableVertexAttribArray(GLuint index); // introduced 2.0
void glEnablei(GLenum target, GLuint index); // introduced 3.0
void glEndConditionalRender(void); // introduced 3.0
void glEndQuery(GLenum target); // introduced 1.5
void glEndQueryIndexed(GLenum target, GLuint index); // introduced 4.0
void glEndTransformFeedback(void); // introduced 3.0
GLsync glFenceSync(GLenum condition, GLbitfield flags); // introduced 3.2
void glFinish(void); // introduced 1.0
void glFlush(void); // introduced 1.0
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length); // introduced 3.0
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length); // introduced 4.5
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param); // introduced 4.3
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer); // introduced 3.0
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level); // introduced 3.2
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); // introduced 3.0
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); // introduced 3.0
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset); // introduced 3.0
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer); // introduced 3.0
void glFrontFace(GLenum mode); // introduced 1.0
void glGenBuffers(GLsizei n, GLuint *buffers); // introduced 1.5
void glGenFramebuffers(GLsizei n, GLuint *framebuffers); // introduced 3.0
void glGenProgramPipelines(GLsizei n, GLuint *pipelines); // introduced 4.1
void glGenQueries(GLsizei n, GLuint *ids); // introduced 1.5
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers); // introduced 3.0
void glGenSamplers(GLsizei count, GLuint *samplers); // introduced 3.3
void glGenTextures(GLsizei n, GLuint *textures); // introduced 1.1
void glGenTransformFeedbacks(GLsizei n, GLuint *ids); // introduced 4.0
void glGenVertexArrays(GLsizei n, GLuint *arrays); // introduced 3.0
void glGenerateMipmap(GLenum target); // introduced 3.0
void glGenerateTextureMipmap(GLuint texture); // introduced 4.5
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params); // introduced 4.2
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name); // introduced 2.0
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced 4.0
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced 4.0
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values); // introduced 4.0
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name); // introduced 2.0
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName); // introduced 3.1
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params); // introduced 3.1
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName); // introduced 3.1
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params); // introduced 3.1
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders); // introduced 2.0
GLint glGetAttribLocation(GLuint program, const GLchar *name); // introduced 2.0
void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data); // introduced 3.0
void glGetBooleanv(GLenum pname, GLboolean *data); // introduced 1.0
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params); // introduced 3.2
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced 1.5
void glGetBufferPointerv(GLenum target, GLenum pname, void **params); // introduced 1.5
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data); // introduced 1.5
void glGetCompressedTexImage(GLenum target, GLint level, void *img); // introduced 1.3
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels); // introduced 4.5
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels); // introduced 4.5
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog); // introduced 4.3
void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data); // introduced 4.1
void glGetDoublev(GLenum pname, GLdouble *data); // introduced 1.0
GLenum glGetError(void); // introduced 1.0
void glGetFloati_v(GLenum target, GLuint index, GLfloat *data); // introduced 4.1
void glGetFloatv(GLenum pname, GLfloat *data); // introduced 1.0
GLint glGetFragDataIndex(GLuint program, const GLchar *name); // introduced 3.3
GLint glGetFragDataLocation(GLuint program, const GLchar *name); // introduced 3.0
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params); // introduced 3.0
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced 4.3
GLenum glGetGraphicsResetStatus(void); // introduced 4.5
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data); // introduced 3.2
void glGetInteger64v(GLenum pname, GLint64 *data); // introduced 3.2
void glGetIntegeri_v(GLenum target, GLuint index, GLint *data); // introduced 3.0
void glGetIntegerv(GLenum pname, GLint *data); // introduced 1.0
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params); // introduced 4.3
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params); // introduced 4.2
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val); // introduced 3.2
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params); // introduced 4.5
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params); // introduced 4.5
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params); // introduced 4.5
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data); // introduced 4.5
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params); // introduced 4.5
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param); // introduced 4.5
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params); // introduced 4.5
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label); // introduced 4.3
void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label); // introduced 4.3
void glGetPointerv(GLenum pname, void **params); // introduced 4.3
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary); // introduced 4.1
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced 2.0
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params); // introduced 4.3
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced 4.1
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params); // introduced 4.1
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name); // introduced 4.3
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name); // introduced 4.3
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name); // introduced 4.3
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced 4.3
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params); // introduced 4.3
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values); // introduced 4.0
void glGetProgramiv(GLuint program, GLenum pname, GLint *params); // introduced 2.0
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced 4.5
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced 4.5
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced 4.5
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced 4.5
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params); // introduced 4.0
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params); // introduced 3.3
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params); // introduced 1.5
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params); // introduced 3.3
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params); // introduced 1.5
void glGetQueryiv(GLenum target, GLenum pname, GLint *params); // introduced 1.5
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced 3.0
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params); // introduced 3.3
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params); // introduced 3.3
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params); // introduced 3.3
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params); // introduced 3.3
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced 2.0
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision); // introduced 4.1
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source); // introduced 2.0
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params); // introduced 2.0
const GLubyte * glGetString(GLenum name); // introduced 1.0
const GLubyte * glGetStringi(GLenum name, GLuint index); // introduced 3.0
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name); // introduced 4.0
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name); // introduced 4.0
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values); // introduced 3.2
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels); // introduced 1.0
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params); // introduced 1.0
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params); // introduced 1.0
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params); // introduced 3.0
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params); // introduced 3.0
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params); // introduced 1.0
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params); // introduced 1.0
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced 4.5
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params); // introduced 4.5
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params); // introduced 4.5
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params); // introduced 4.5
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params); // introduced 4.5
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params); // introduced 4.5
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params); // introduced 4.5
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced 4.5
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name); // introduced 3.0
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param); // introduced 4.5
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param); // introduced 4.5
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param); // introduced 4.5
GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName); // introduced 3.1
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices); // introduced 3.1
GLint glGetUniformLocation(GLuint program, const GLchar *name); // introduced 2.0
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params); // introduced 4.0
void glGetUniformdv(GLuint program, GLint location, GLdouble *params); // introduced 4.0
void glGetUniformfv(GLuint program, GLint location, GLfloat *params); // introduced 2.0
void glGetUniformiv(GLuint program, GLint location, GLint *params); // introduced 2.0
void glGetUniformuiv(GLuint program, GLint location, GLuint *params); // introduced 3.0
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param); // introduced 4.5
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param); // introduced 4.5
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param); // introduced 4.5
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params); // introduced 3.0
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params); // introduced 3.0
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params); // introduced 4.1
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer); // introduced 2.0
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params); // introduced 2.0
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params); // introduced 2.0
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params); // introduced 2.0
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels); // introduced 4.5
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced 4.5
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params); // introduced 4.5
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params); // introduced 4.5
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params); // introduced 4.5
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params); // introduced 4.5
void glHint(GLenum target, GLenum mode); // introduced 1.0
void glInvalidateBufferData(GLuint buffer); // introduced 4.3
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length); // introduced 4.3
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments); // introduced 4.3
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments); // introduced 4.5
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 4.5
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height); // introduced 4.3
void glInvalidateTexImage(GLuint texture, GLint level); // introduced 4.3
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth); // introduced 4.3
GLboolean glIsBuffer(GLuint buffer); // introduced 1.5
GLboolean glIsEnabled(GLenum cap); // introduced 1.0
GLboolean glIsEnabledi(GLenum target, GLuint index); // introduced 3.0
GLboolean glIsFramebuffer(GLuint framebuffer); // introduced 3.0
GLboolean glIsProgram(GLuint program); // introduced 2.0
GLboolean glIsProgramPipeline(GLuint pipeline); // introduced 4.1
GLboolean glIsQuery(GLuint id); // introduced 1.5
GLboolean glIsRenderbuffer(GLuint renderbuffer); // introduced 3.0
GLboolean glIsSampler(GLuint sampler); // introduced 3.3
GLboolean glIsShader(GLuint shader); // introduced 2.0
GLboolean glIsSync(GLsync sync); // introduced 3.2
GLboolean glIsTexture(GLuint texture); // introduced 1.1
GLboolean glIsTransformFeedback(GLuint id); // introduced 4.0
GLboolean glIsVertexArray(GLuint array); // introduced 3.0
void glLineWidth(GLfloat width); // introduced 1.0
void glLinkProgram(GLuint program); // introduced 2.0
void glLogicOp(GLenum opcode); // introduced 1.0
void * glMapBuffer(GLenum target, GLenum access); // introduced 1.5
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access); // introduced 3.0
void * glMapNamedBuffer(GLuint buffer, GLenum access); // introduced 4.5
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access); // introduced 4.5
void glMemoryBarrier(GLbitfield barriers); // introduced 4.2
void glMemoryBarrierByRegion(GLbitfield barriers); // introduced 4.5
void glMinSampleShading(GLfloat value); // introduced 4.0
void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount); // introduced 1.4
void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride); // introduced 4.3
void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride); // introduced 4.6
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount); // introduced 1.4
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex); // introduced 3.2
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride); // introduced 4.3
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride); // introduced 4.6
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage); // introduced 4.5
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags); // introduced 4.5
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data); // introduced 4.5
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf); // introduced 4.5
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs); // introduced 4.5
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param); // introduced 4.5
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src); // introduced 4.5
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer); // introduced 4.5
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level); // introduced 4.5
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer); // introduced 4.5
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height); // introduced 4.5
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height); // introduced 4.5
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label); // introduced 4.3
void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label); // introduced 4.3
void glPatchParameterfv(GLenum pname, const GLfloat *values); // introduced 4.0
void glPatchParameteri(GLenum pname, GLint value); // introduced 4.0
void glPauseTransformFeedback(void); // introduced 4.0
void glPixelStoref(GLenum pname, GLfloat param); // introduced 1.0
void glPixelStorei(GLenum pname, GLint param); // introduced 1.0
void glPointParameterf(GLenum pname, GLfloat param); // introduced 1.4
void glPointParameterfv(GLenum pname, const GLfloat *params); // introduced 1.4
void glPointParameteri(GLenum pname, GLint param); // introduced 1.4
void glPointParameteriv(GLenum pname, const GLint *params); // introduced 1.4
void glPointSize(GLfloat size); // introduced 1.0
void glPolygonMode(GLenum face, GLenum mode); // introduced 1.0
void glPolygonOffset(GLfloat factor, GLfloat units); // introduced 1.1
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp); // introduced 4.6
void glPopDebugGroup(void); // introduced 4.3
void glPrimitiveRestartIndex(GLuint index); // introduced 3.1
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length); // introduced 4.1
void glProgramParameteri(GLuint program, GLenum pname, GLint value); // introduced 4.1
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0); // introduced 4.1
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced 4.1
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0); // introduced 4.1
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced 4.1
void glProgramUniform1i(GLuint program, GLint location, GLint v0); // introduced 4.1
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced 4.1
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0); // introduced 4.1
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced 4.1
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1); // introduced 4.1
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced 4.1
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1); // introduced 4.1
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced 4.1
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1); // introduced 4.1
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced 4.1
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1); // introduced 4.1
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced 4.1
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2); // introduced 4.1
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced 4.1
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2); // introduced 4.1
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced 4.1
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2); // introduced 4.1
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced 4.1
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2); // introduced 4.1
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced 4.1
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3); // introduced 4.1
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced 4.1
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3); // introduced 4.1
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced 4.1
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3); // introduced 4.1
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced 4.1
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3); // introduced 4.1
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced 4.1
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.1
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 4.1
void glProvokingVertex(GLenum mode); // introduced 3.2
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message); // introduced 4.3
void glQueryCounter(GLuint id, GLenum target); // introduced 3.3
void glReadBuffer(GLenum src); // introduced 1.0
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels); // introduced 1.0
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data); // introduced 4.5
void glReleaseShaderCompiler(void); // introduced 4.1
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height); // introduced 3.0
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height); // introduced 3.0
void glResumeTransformFeedback(void); // introduced 4.0
void glSampleCoverage(GLfloat value, GLboolean invert); // introduced 1.3
void glSampleMaski(GLuint maskNumber, GLbitfield mask); // introduced 3.2
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param); // introduced 3.3
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param); // introduced 3.3
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param); // introduced 3.3
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param); // introduced 3.3
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param); // introduced 3.3
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param); // introduced 3.3
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height); // introduced 1.0
void glScissorArrayv(GLuint first, GLsizei count, const GLint *v); // introduced 4.1
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height); // introduced 4.1
void glScissorIndexedv(GLuint index, const GLint *v); // introduced 4.1
void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length); // introduced 4.1
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length); // introduced 2.0
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding); // introduced 4.3
void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue); // introduced 4.6
void glStencilFunc(GLenum func, GLint ref, GLuint mask); // introduced 1.0
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask); // introduced 2.0
void glStencilMask(GLuint mask); // introduced 1.0
void glStencilMaskSeparate(GLenum face, GLuint mask); // introduced 2.0
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass); // introduced 1.0
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass); // introduced 2.0
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer); // introduced 3.1
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced 4.3
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels); // introduced 1.0
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels); // introduced 1.0
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced 3.2
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels); // introduced 1.2
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced 3.2
void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params); // introduced 3.0
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params); // introduced 3.0
void glTexParameterf(GLenum target, GLenum pname, GLfloat param); // introduced 1.0
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params); // introduced 1.0
void glTexParameteri(GLenum target, GLenum pname, GLint param); // introduced 1.0
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params); // introduced 1.0
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width); // introduced 4.2
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height); // introduced 4.2
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced 4.3
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth); // introduced 4.2
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced 4.3
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels); // introduced 1.1
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels); // introduced 1.1
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels); // introduced 1.2
void glTextureBarrier(void); // introduced 4.5
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer); // introduced 4.5
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced 4.5
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params); // introduced 4.5
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params); // introduced 4.5
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param); // introduced 4.5
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param); // introduced 4.5
void glTextureParameteri(GLuint texture, GLenum pname, GLint param); // introduced 4.5
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param); // introduced 4.5
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width); // introduced 4.5
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height); // introduced 4.5
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced 4.5
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth); // introduced 4.5
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced 4.5
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels); // introduced 4.5
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels); // introduced 4.5
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels); // introduced 4.5
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers); // introduced 4.3
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer); // introduced 4.5
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced 4.5
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode); // introduced 3.0
void glUniform1d(GLint location, GLdouble x); // introduced 4.0
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value); // introduced 4.0
void glUniform1f(GLint location, GLfloat v0); // introduced 2.0
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value); // introduced 2.0
void glUniform1i(GLint location, GLint v0); // introduced 2.0
void glUniform1iv(GLint location, GLsizei count, const GLint *value); // introduced 2.0
void glUniform1ui(GLint location, GLuint v0); // introduced 3.0
void glUniform1uiv(GLint location, GLsizei count, const GLuint *value); // introduced 3.0
void glUniform2d(GLint location, GLdouble x, GLdouble y); // introduced 4.0
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value); // introduced 4.0
void glUniform2f(GLint location, GLfloat v0, GLfloat v1); // introduced 2.0
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value); // introduced 2.0
void glUniform2i(GLint location, GLint v0, GLint v1); // introduced 2.0
void glUniform2iv(GLint location, GLsizei count, const GLint *value); // introduced 2.0
void glUniform2ui(GLint location, GLuint v0, GLuint v1); // introduced 3.0
void glUniform2uiv(GLint location, GLsizei count, const GLuint *value); // introduced 3.0
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z); // introduced 4.0
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value); // introduced 4.0
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2); // introduced 2.0
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value); // introduced 2.0
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2); // introduced 2.0
void glUniform3iv(GLint location, GLsizei count, const GLint *value); // introduced 2.0
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2); // introduced 3.0
void glUniform3uiv(GLint location, GLsizei count, const GLuint *value); // introduced 3.0
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced 4.0
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value); // introduced 4.0
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3); // introduced 2.0
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value); // introduced 2.0
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3); // introduced 2.0
void glUniform4iv(GLint location, GLsizei count, const GLint *value); // introduced 2.0
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3); // introduced 3.0
void glUniform4uiv(GLint location, GLsizei count, const GLuint *value); // introduced 3.0
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding); // introduced 3.1
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.0
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.0
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.0
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced 4.0
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced 2.1
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices); // introduced 4.0
GLboolean glUnmapBuffer(GLenum target); // introduced 1.5
GLboolean glUnmapNamedBuffer(GLuint buffer); // introduced 4.5
void glUseProgram(GLuint program); // introduced 2.0
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program); // introduced 4.1
void glValidateProgram(GLuint program); // introduced 2.0
void glValidateProgramPipeline(GLuint pipeline); // introduced 4.1
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex); // introduced 4.5
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset); // introduced 4.5
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced 4.5
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced 4.5
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor); // introduced 4.5
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer); // introduced 4.5
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride); // introduced 4.5
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides); // introduced 4.5
void glVertexAttrib1d(GLuint index, GLdouble x); // introduced 2.0
void glVertexAttrib1dv(GLuint index, const GLdouble *v); // introduced 2.0
void glVertexAttrib1f(GLuint index, GLfloat x); // introduced 2.0
void glVertexAttrib1fv(GLuint index, const GLfloat *v); // introduced 2.0
void glVertexAttrib1s(GLuint index, GLshort x); // introduced 2.0
void glVertexAttrib1sv(GLuint index, const GLshort *v); // introduced 2.0
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y); // introduced 2.0
void glVertexAttrib2dv(GLuint index, const GLdouble *v); // introduced 2.0
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y); // introduced 2.0
void glVertexAttrib2fv(GLuint index, const GLfloat *v); // introduced 2.0
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y); // introduced 2.0
void glVertexAttrib2sv(GLuint index, const GLshort *v); // introduced 2.0
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z); // introduced 2.0
void glVertexAttrib3dv(GLuint index, const GLdouble *v); // introduced 2.0
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z); // introduced 2.0
void glVertexAttrib3fv(GLuint index, const GLfloat *v); // introduced 2.0
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z); // introduced 2.0
void glVertexAttrib3sv(GLuint index, const GLshort *v); // introduced 2.0
void glVertexAttrib4Nbv(GLuint index, const GLbyte *v); // introduced 2.0
void glVertexAttrib4Niv(GLuint index, const GLint *v); // introduced 2.0
void glVertexAttrib4Nsv(GLuint index, const GLshort *v); // introduced 2.0
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w); // introduced 2.0
void glVertexAttrib4Nubv(GLuint index, const GLubyte *v); // introduced 2.0
void glVertexAttrib4Nuiv(GLuint index, const GLuint *v); // introduced 2.0
void glVertexAttrib4Nusv(GLuint index, const GLushort *v); // introduced 2.0
void glVertexAttrib4bv(GLuint index, const GLbyte *v); // introduced 2.0
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced 2.0
void glVertexAttrib4dv(GLuint index, const GLdouble *v); // introduced 2.0
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); // introduced 2.0
void glVertexAttrib4fv(GLuint index, const GLfloat *v); // introduced 2.0
void glVertexAttrib4iv(GLuint index, const GLint *v); // introduced 2.0
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w); // introduced 2.0
void glVertexAttrib4sv(GLuint index, const GLshort *v); // introduced 2.0
void glVertexAttrib4ubv(GLuint index, const GLubyte *v); // introduced 2.0
void glVertexAttrib4uiv(GLuint index, const GLuint *v); // introduced 2.0
void glVertexAttrib4usv(GLuint index, const GLushort *v); // introduced 2.0
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex); // introduced 4.3
void glVertexAttribDivisor(GLuint index, GLuint divisor); // introduced 3.3
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset); // introduced 4.3
void glVertexAttribI1i(GLuint index, GLint x); // introduced 3.0
void glVertexAttribI1iv(GLuint index, const GLint *v); // introduced 3.0
void glVertexAttribI1ui(GLuint index, GLuint x); // introduced 3.0
void glVertexAttribI1uiv(GLuint index, const GLuint *v); // introduced 3.0
void glVertexAttribI2i(GLuint index, GLint x, GLint y); // introduced 3.0
void glVertexAttribI2iv(GLuint index, const GLint *v); // introduced 3.0
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y); // introduced 3.0
void glVertexAttribI2uiv(GLuint index, const GLuint *v); // introduced 3.0
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z); // introduced 3.0
void glVertexAttribI3iv(GLuint index, const GLint *v); // introduced 3.0
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z); // introduced 3.0
void glVertexAttribI3uiv(GLuint index, const GLuint *v); // introduced 3.0
void glVertexAttribI4bv(GLuint index, const GLbyte *v); // introduced 3.0
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w); // introduced 3.0
void glVertexAttribI4iv(GLuint index, const GLint *v); // introduced 3.0
void glVertexAttribI4sv(GLuint index, const GLshort *v); // introduced 3.0
void glVertexAttribI4ubv(GLuint index, const GLubyte *v); // introduced 3.0
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w); // introduced 3.0
void glVertexAttribI4uiv(GLuint index, const GLuint *v); // introduced 3.0
void glVertexAttribI4usv(GLuint index, const GLushort *v); // introduced 3.0
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced 4.3
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer); // introduced 3.0
void glVertexAttribL1d(GLuint index, GLdouble x); // introduced 4.1
void glVertexAttribL1dv(GLuint index, const GLdouble *v); // introduced 4.1
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y); // introduced 4.1
void glVertexAttribL2dv(GLuint index, const GLdouble *v); // introduced 4.1
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z); // introduced 4.1
void glVertexAttribL3dv(GLuint index, const GLdouble *v); // introduced 4.1
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced 4.1
void glVertexAttribL4dv(GLuint index, const GLdouble *v); // introduced 4.1
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced 4.3
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer); // introduced 4.1
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced 3.3
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced 3.3
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced 3.3
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced 3.3
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced 3.3
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced 3.3
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced 3.3
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced 3.3
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer); // introduced 2.0
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor); // introduced 4.3
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height); // introduced 1.0
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v); // introduced 4.1
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h); // introduced 4.1
void glViewportIndexedfv(GLuint index, const GLfloat *v); // introduced 4.1
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout); // introduced 3.2
// --- OpenGL 4.6 COMPATIBILITY-profile-only (KHÔNG thuộc Core Profile) ---
// Cần cho GL_ARB_vertex_type_2_10_10_10_rev + GL_ARB_robustness aliases.
void glColorP3ui(GLenum type, GLuint color); // COMPAT-ONLY, introduced 3.3
void glColorP3uiv(GLenum type, const GLuint *color); // COMPAT-ONLY, introduced 3.3
void glColorP4ui(GLenum type, GLuint color); // COMPAT-ONLY, introduced 3.3
void glColorP4uiv(GLenum type, const GLuint *color); // COMPAT-ONLY, introduced 3.3
void glGetnColorTable(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table); // COMPAT-ONLY, introduced 4.5
void glGetnConvolutionFilter(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image); // COMPAT-ONLY, introduced 4.5
void glGetnHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values); // COMPAT-ONLY, introduced 4.5
void glGetnMapdv(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v); // COMPAT-ONLY, introduced 4.5
void glGetnMapfv(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v); // COMPAT-ONLY, introduced 4.5
void glGetnMapiv(GLenum target, GLenum query, GLsizei bufSize, GLint *v); // COMPAT-ONLY, introduced 4.5
void glGetnMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values); // COMPAT-ONLY, introduced 4.5
void glGetnPixelMapfv(GLenum map, GLsizei bufSize, GLfloat *values); // COMPAT-ONLY, introduced 4.5
void glGetnPixelMapuiv(GLenum map, GLsizei bufSize, GLuint *values); // COMPAT-ONLY, introduced 4.5
void glGetnPixelMapusv(GLenum map, GLsizei bufSize, GLushort *values); // COMPAT-ONLY, introduced 4.5
void glGetnPolygonStipple(GLsizei bufSize, GLubyte *pattern); // COMPAT-ONLY, introduced 4.5
void glGetnSeparableFilter(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span); // COMPAT-ONLY, introduced 4.5
void glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glNormalP3ui(GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glNormalP3uiv(GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glSecondaryColorP3ui(GLenum type, GLuint color); // COMPAT-ONLY, introduced 3.3
void glSecondaryColorP3uiv(GLenum type, const GLuint *color); // COMPAT-ONLY, introduced 3.3
void glTexCoordP1ui(GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP1uiv(GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP2ui(GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP2uiv(GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP3ui(GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP3uiv(GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP4ui(GLenum type, GLuint coords); // COMPAT-ONLY, introduced 3.3
void glTexCoordP4uiv(GLenum type, const GLuint *coords); // COMPAT-ONLY, introduced 3.3
void glVertexP2ui(GLenum type, GLuint value); // COMPAT-ONLY, introduced 3.3
void glVertexP2uiv(GLenum type, const GLuint *value); // COMPAT-ONLY, introduced 3.3
void glVertexP3ui(GLenum type, GLuint value); // COMPAT-ONLY, introduced 3.3
void glVertexP3uiv(GLenum type, const GLuint *value); // COMPAT-ONLY, introduced 3.3
void glVertexP4ui(GLenum type, GLuint value); // COMPAT-ONLY, introduced 3.3
void glVertexP4uiv(GLenum type, const GLuint *value); // COMPAT-ONLY, introduced 3.3
} // namespace gl
int GetCoreFunctionCount(); // = 657
int GetCompatOnlyFunctionCount(); // = 42
const char* GetCoreFunctionName(int i);
int GetCoreFunctionTableSize();
GLuint tglmt_num_extensions();
} // namespace tglmt
