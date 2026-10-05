// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.
// 698 hàm OpenGL 4.6 Core Profile trong namespace tglmt::gl.
#pragma once
#include "tglmt/gl46_types.h"

namespace tglmt { namespace gl {
void glActiveShaderProgram(GLuint pipeline, GLuint program); // introduced GL_VERSION_4_1
void glActiveTexture(GLenum texture); // introduced GL_VERSION_1_3
void glAttachShader(GLuint program, GLuint shader); // introduced GL_VERSION_2_0
void glBeginConditionalRender(GLuint id, GLenum mode); // introduced GL_VERSION_3_0
void glBeginQuery(GLenum target, GLuint id); // introduced GL_VERSION_1_5
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id); // introduced GL_VERSION_4_0
void glBeginTransformFeedback(GLenum primitiveMode); // introduced GL_VERSION_3_0
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name); // introduced GL_VERSION_2_0
void glBindBuffer(GLenum target, GLuint buffer); // introduced GL_VERSION_1_5
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer); // introduced GL_VERSION_3_1
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced GL_VERSION_3_1
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers); // introduced GL_VERSION_4_4
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes); // introduced GL_VERSION_4_4
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name); // introduced GL_VERSION_3_0
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name); // introduced GL_VERSION_3_3
void glBindFramebuffer(GLenum target, GLuint framebuffer); // introduced GL_VERSION_3_0
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format); // introduced GL_VERSION_4_2
void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures); // introduced GL_VERSION_4_4
void glBindProgramPipeline(GLuint pipeline); // introduced GL_VERSION_4_1
void glBindRenderbuffer(GLenum target, GLuint renderbuffer); // introduced GL_VERSION_3_0
void glBindSampler(GLuint unit, GLuint sampler); // introduced GL_VERSION_3_3
void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers); // introduced GL_VERSION_4_4
void glBindTexture(GLenum target, GLuint texture); // introduced GL_VERSION_1_1
void glBindTextureUnit(GLuint unit, GLuint texture); // introduced GL_VERSION_4_5
void glBindTextures(GLuint first, GLsizei count, const GLuint *textures); // introduced GL_VERSION_4_4
void glBindTransformFeedback(GLenum target, GLuint id); // introduced GL_VERSION_4_0
void glBindVertexArray(GLuint array); // introduced GL_VERSION_3_0
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride); // introduced GL_VERSION_4_3
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides); // introduced GL_VERSION_4_4
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // introduced GL_VERSION_1_4
void glBlendEquation(GLenum mode); // introduced GL_VERSION_1_4
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha); // introduced GL_VERSION_2_0
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha); // introduced GL_VERSION_4_0
void glBlendEquationi(GLuint buf, GLenum mode); // introduced GL_VERSION_4_0
void glBlendFunc(GLenum sfactor, GLenum dfactor); // introduced GL_VERSION_1_0
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha); // introduced GL_VERSION_1_4
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha); // introduced GL_VERSION_4_0
void glBlendFunci(GLuint buf, GLenum src, GLenum dst); // introduced GL_VERSION_4_0
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter); // introduced GL_VERSION_3_0
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter); // introduced GL_VERSION_4_5
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage); // introduced GL_VERSION_1_5
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags); // introduced GL_VERSION_4_4
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data); // introduced GL_VERSION_1_5
GLenum glCheckFramebufferStatus(GLenum target); // introduced GL_VERSION_3_0
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target); // introduced GL_VERSION_4_5
void glClampColor(GLenum target, GLenum clamp); // introduced GL_VERSION_3_0
void glClear(GLbitfield mask); // introduced GL_VERSION_1_0
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_3
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_3
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil); // introduced GL_VERSION_3_0
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value); // introduced GL_VERSION_3_0
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value); // introduced GL_VERSION_3_0
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value); // introduced GL_VERSION_3_0
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // introduced GL_VERSION_1_0
void glClearDepth(GLdouble depth); // introduced GL_VERSION_1_0
void glClearDepthf(GLfloat d); // introduced GL_VERSION_4_1
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_5
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_5
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil); // introduced GL_VERSION_4_5
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value); // introduced GL_VERSION_4_5
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value); // introduced GL_VERSION_4_5
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value); // introduced GL_VERSION_4_5
void glClearStencil(GLint s); // introduced GL_VERSION_1_0
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_4
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data); // introduced GL_VERSION_4_4
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout); // introduced GL_VERSION_3_2
void glClipControl(GLenum origin, GLenum depth); // introduced GL_VERSION_4_5
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha); // introduced GL_VERSION_1_0
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a); // introduced GL_VERSION_3_0
void glColorP3ui(GLenum type, GLuint color); // introduced GL_VERSION_3_3
void glColorP3uiv(GLenum type, const GLuint *color); // introduced GL_VERSION_3_3
void glColorP4ui(GLenum type, GLuint color); // introduced GL_VERSION_3_3
void glColorP4uiv(GLenum type, const GLuint *color); // introduced GL_VERSION_3_3
void glCompileShader(GLuint shader); // introduced GL_VERSION_2_0
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_1_3
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_4_5
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_4_5
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data); // introduced GL_VERSION_4_5
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size); // introduced GL_VERSION_3_1
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth); // introduced GL_VERSION_4_3
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size); // introduced GL_VERSION_4_5
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border); // introduced GL_VERSION_1_1
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border); // introduced GL_VERSION_1_1
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); // introduced GL_VERSION_1_1
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_1_1
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_1_2
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); // introduced GL_VERSION_4_5
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glCreateBuffers(GLsizei n, GLuint *buffers); // introduced GL_VERSION_4_5
void glCreateFramebuffers(GLsizei n, GLuint *framebuffers); // introduced GL_VERSION_4_5
GLuint glCreateProgram(void); // introduced GL_VERSION_2_0
void glCreateProgramPipelines(GLsizei n, GLuint *pipelines); // introduced GL_VERSION_4_5
void glCreateQueries(GLenum target, GLsizei n, GLuint *ids); // introduced GL_VERSION_4_5
void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers); // introduced GL_VERSION_4_5
void glCreateSamplers(GLsizei n, GLuint *samplers); // introduced GL_VERSION_4_5
GLuint glCreateShader(GLenum type); // introduced GL_VERSION_2_0
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings); // introduced GL_VERSION_4_1
void glCreateTextures(GLenum target, GLsizei n, GLuint *textures); // introduced GL_VERSION_4_5
void glCreateTransformFeedbacks(GLsizei n, GLuint *ids); // introduced GL_VERSION_4_5
void glCreateVertexArrays(GLsizei n, GLuint *arrays); // introduced GL_VERSION_4_5
void glCullFace(GLenum mode); // introduced GL_VERSION_1_0
void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam); // introduced GL_VERSION_4_3
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled); // introduced GL_VERSION_4_3
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf); // introduced GL_VERSION_4_3
void glDeleteBuffers(GLsizei n, const GLuint *buffers); // introduced GL_VERSION_1_5
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers); // introduced GL_VERSION_3_0
void glDeleteProgram(GLuint program); // introduced GL_VERSION_2_0
void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines); // introduced GL_VERSION_4_1
void glDeleteQueries(GLsizei n, const GLuint *ids); // introduced GL_VERSION_1_5
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers); // introduced GL_VERSION_3_0
void glDeleteSamplers(GLsizei count, const GLuint *samplers); // introduced GL_VERSION_3_3
void glDeleteShader(GLuint shader); // introduced GL_VERSION_2_0
void glDeleteSync(GLsync sync); // introduced GL_VERSION_3_2
void glDeleteTextures(GLsizei n, const GLuint *textures); // introduced GL_VERSION_1_1
void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids); // introduced GL_VERSION_4_0
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays); // introduced GL_VERSION_3_0
void glDepthFunc(GLenum func); // introduced GL_VERSION_1_0
void glDepthMask(GLboolean flag); // introduced GL_VERSION_1_0
void glDepthRange(GLdouble n, GLdouble f); // introduced GL_VERSION_1_0
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v); // introduced GL_VERSION_4_1
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f); // introduced GL_VERSION_4_1
void glDepthRangef(GLfloat n, GLfloat f); // introduced GL_VERSION_4_1
void glDetachShader(GLuint program, GLuint shader); // introduced GL_VERSION_2_0
void glDisable(GLenum cap); // introduced GL_VERSION_1_0
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index); // introduced GL_VERSION_4_5
void glDisableVertexAttribArray(GLuint index); // introduced GL_VERSION_2_0
void glDisablei(GLenum target, GLuint index); // introduced GL_VERSION_3_0
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z); // introduced GL_VERSION_4_3
void glDispatchComputeIndirect(GLintptr indirect); // introduced GL_VERSION_4_3
void glDrawArrays(GLenum mode, GLint first, GLsizei count); // introduced GL_VERSION_1_1
void glDrawArraysIndirect(GLenum mode, const void *indirect); // introduced GL_VERSION_4_0
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount); // introduced GL_VERSION_3_1
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance); // introduced GL_VERSION_4_2
void glDrawBuffer(GLenum buf); // introduced GL_VERSION_1_0
void glDrawBuffers(GLsizei n, const GLenum *bufs); // introduced GL_VERSION_2_0
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices); // introduced GL_VERSION_1_1
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex); // introduced GL_VERSION_3_2
void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect); // introduced GL_VERSION_4_0
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount); // introduced GL_VERSION_3_1
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance); // introduced GL_VERSION_4_2
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex); // introduced GL_VERSION_3_2
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance); // introduced GL_VERSION_4_2
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices); // introduced GL_VERSION_1_2
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex); // introduced GL_VERSION_3_2
void glDrawTransformFeedback(GLenum mode, GLuint id); // introduced GL_VERSION_4_0
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount); // introduced GL_VERSION_4_2
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream); // introduced GL_VERSION_4_0
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount); // introduced GL_VERSION_4_2
void glEnable(GLenum cap); // introduced GL_VERSION_1_0
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index); // introduced GL_VERSION_4_5
void glEnableVertexAttribArray(GLuint index); // introduced GL_VERSION_2_0
void glEnablei(GLenum target, GLuint index); // introduced GL_VERSION_3_0
void glEndConditionalRender(void); // introduced GL_VERSION_3_0
void glEndQuery(GLenum target); // introduced GL_VERSION_1_5
void glEndQueryIndexed(GLenum target, GLuint index); // introduced GL_VERSION_4_0
void glEndTransformFeedback(void); // introduced GL_VERSION_3_0
GLsync glFenceSync(GLenum condition, GLbitfield flags); // introduced GL_VERSION_3_2
void glFinish(void); // introduced GL_VERSION_1_0
void glFlush(void); // introduced GL_VERSION_1_0
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length); // introduced GL_VERSION_3_0
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length); // introduced GL_VERSION_4_5
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param); // introduced GL_VERSION_4_3
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer); // introduced GL_VERSION_3_0
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level); // introduced GL_VERSION_3_2
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); // introduced GL_VERSION_3_0
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level); // introduced GL_VERSION_3_0
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset); // introduced GL_VERSION_3_0
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer); // introduced GL_VERSION_3_0
void glFrontFace(GLenum mode); // introduced GL_VERSION_1_0
void glGenBuffers(GLsizei n, GLuint *buffers); // introduced GL_VERSION_1_5
void glGenFramebuffers(GLsizei n, GLuint *framebuffers); // introduced GL_VERSION_3_0
void glGenProgramPipelines(GLsizei n, GLuint *pipelines); // introduced GL_VERSION_4_1
void glGenQueries(GLsizei n, GLuint *ids); // introduced GL_VERSION_1_5
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers); // introduced GL_VERSION_3_0
void glGenSamplers(GLsizei count, GLuint *samplers); // introduced GL_VERSION_3_3
void glGenTextures(GLsizei n, GLuint *textures); // introduced GL_VERSION_1_1
void glGenTransformFeedbacks(GLsizei n, GLuint *ids); // introduced GL_VERSION_4_0
void glGenVertexArrays(GLsizei n, GLuint *arrays); // introduced GL_VERSION_3_0
void glGenerateMipmap(GLenum target); // introduced GL_VERSION_3_0
void glGenerateTextureMipmap(GLuint texture); // introduced GL_VERSION_4_5
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params); // introduced GL_VERSION_4_2
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name); // introduced GL_VERSION_2_0
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced GL_VERSION_4_0
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced GL_VERSION_4_0
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values); // introduced GL_VERSION_4_0
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name); // introduced GL_VERSION_2_0
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName); // introduced GL_VERSION_3_1
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params); // introduced GL_VERSION_3_1
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName); // introduced GL_VERSION_3_1
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params); // introduced GL_VERSION_3_1
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders); // introduced GL_VERSION_2_0
GLint glGetAttribLocation(GLuint program, const GLchar *name); // introduced GL_VERSION_2_0
void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data); // introduced GL_VERSION_3_0
void glGetBooleanv(GLenum pname, GLboolean *data); // introduced GL_VERSION_1_0
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params); // introduced GL_VERSION_3_2
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_1_5
void glGetBufferPointerv(GLenum target, GLenum pname, void **params); // introduced GL_VERSION_1_5
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data); // introduced GL_VERSION_1_5
void glGetCompressedTexImage(GLenum target, GLint level, void *img); // introduced GL_VERSION_1_3
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog); // introduced GL_VERSION_4_3
void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data); // introduced GL_VERSION_4_1
void glGetDoublev(GLenum pname, GLdouble *data); // introduced GL_VERSION_1_0
GLenum glGetError(void); // introduced GL_VERSION_1_0
void glGetFloati_v(GLenum target, GLuint index, GLfloat *data); // introduced GL_VERSION_4_1
void glGetFloatv(GLenum pname, GLfloat *data); // introduced GL_VERSION_1_0
GLint glGetFragDataIndex(GLuint program, const GLchar *name); // introduced GL_VERSION_3_3
GLint glGetFragDataLocation(GLuint program, const GLchar *name); // introduced GL_VERSION_3_0
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params); // introduced GL_VERSION_3_0
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_4_3
GLenum glGetGraphicsResetStatus(void); // introduced GL_VERSION_4_5
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data); // introduced GL_VERSION_3_2
void glGetInteger64v(GLenum pname, GLint64 *data); // introduced GL_VERSION_3_2
void glGetIntegeri_v(GLenum target, GLuint index, GLint *data); // introduced GL_VERSION_3_1
void glGetIntegerv(GLenum pname, GLint *data); // introduced GL_VERSION_1_0
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params); // introduced GL_VERSION_4_3
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params); // introduced GL_VERSION_4_2
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val); // introduced GL_VERSION_3_2
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params); // introduced GL_VERSION_4_5
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params); // introduced GL_VERSION_4_5
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data); // introduced GL_VERSION_4_5
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param); // introduced GL_VERSION_4_5
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label); // introduced GL_VERSION_4_3
void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label); // introduced GL_VERSION_4_3
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary); // introduced GL_VERSION_4_1
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced GL_VERSION_2_0
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params); // introduced GL_VERSION_4_3
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced GL_VERSION_4_1
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params); // introduced GL_VERSION_4_1
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name); // introduced GL_VERSION_4_3
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name); // introduced GL_VERSION_4_3
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name); // introduced GL_VERSION_4_3
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name); // introduced GL_VERSION_4_3
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params); // introduced GL_VERSION_4_3
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values); // introduced GL_VERSION_4_0
void glGetProgramiv(GLuint program, GLenum pname, GLint *params); // introduced GL_VERSION_2_0
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced GL_VERSION_4_5
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced GL_VERSION_4_5
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced GL_VERSION_4_5
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset); // introduced GL_VERSION_4_5
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params); // introduced GL_VERSION_4_0
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params); // introduced GL_VERSION_3_3
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params); // introduced GL_VERSION_1_5
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params); // introduced GL_VERSION_3_3
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params); // introduced GL_VERSION_1_5
void glGetQueryiv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_1_5
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_3_0
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params); // introduced GL_VERSION_3_3
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params); // introduced GL_VERSION_3_3
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params); // introduced GL_VERSION_3_3
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params); // introduced GL_VERSION_3_3
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog); // introduced GL_VERSION_2_0
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision); // introduced GL_VERSION_4_1
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source); // introduced GL_VERSION_2_0
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params); // introduced GL_VERSION_2_0
const GLubyte * glGetString(GLenum name); // introduced GL_VERSION_1_0
const GLubyte * glGetStringi(GLenum name, GLuint index); // introduced GL_VERSION_3_0
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name); // introduced GL_VERSION_4_0
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name); // introduced GL_VERSION_4_0
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values); // introduced GL_VERSION_3_2
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels); // introduced GL_VERSION_1_0
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params); // introduced GL_VERSION_1_0
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params); // introduced GL_VERSION_1_0
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_3_0
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params); // introduced GL_VERSION_3_0
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params); // introduced GL_VERSION_1_0
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params); // introduced GL_VERSION_1_0
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params); // introduced GL_VERSION_4_5
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params); // introduced GL_VERSION_4_5
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params); // introduced GL_VERSION_4_5
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params); // introduced GL_VERSION_4_5
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name); // introduced GL_VERSION_3_0
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param); // introduced GL_VERSION_4_5
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param); // introduced GL_VERSION_4_5
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param); // introduced GL_VERSION_4_5
GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName); // introduced GL_VERSION_3_1
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices); // introduced GL_VERSION_3_1
GLint glGetUniformLocation(GLuint program, const GLchar *name); // introduced GL_VERSION_2_0
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params); // introduced GL_VERSION_4_0
void glGetUniformdv(GLuint program, GLint location, GLdouble *params); // introduced GL_VERSION_4_0
void glGetUniformfv(GLuint program, GLint location, GLfloat *params); // introduced GL_VERSION_2_0
void glGetUniformiv(GLuint program, GLint location, GLint *params); // introduced GL_VERSION_2_0
void glGetUniformuiv(GLuint program, GLint location, GLuint *params); // introduced GL_VERSION_3_0
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param); // introduced GL_VERSION_4_5
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param); // introduced GL_VERSION_4_5
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param); // introduced GL_VERSION_4_5
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params); // introduced GL_VERSION_3_0
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params); // introduced GL_VERSION_3_0
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params); // introduced GL_VERSION_4_1
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer); // introduced GL_VERSION_2_0
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params); // introduced GL_VERSION_2_0
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params); // introduced GL_VERSION_2_0
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params); // introduced GL_VERSION_2_0
void glGetnColorTable(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table); // introduced GL_VERSION_4_5
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
void glGetnConvolutionFilter(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image); // introduced GL_VERSION_4_5
void glGetnHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values); // introduced GL_VERSION_4_5
void glGetnMapdv(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v); // introduced GL_VERSION_4_5
void glGetnMapfv(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v); // introduced GL_VERSION_4_5
void glGetnMapiv(GLenum target, GLenum query, GLsizei bufSize, GLint *v); // introduced GL_VERSION_4_5
void glGetnMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values); // introduced GL_VERSION_4_5
void glGetnPixelMapfv(GLenum map, GLsizei bufSize, GLfloat *values); // introduced GL_VERSION_4_5
void glGetnPixelMapuiv(GLenum map, GLsizei bufSize, GLuint *values); // introduced GL_VERSION_4_5
void glGetnPixelMapusv(GLenum map, GLsizei bufSize, GLushort *values); // introduced GL_VERSION_4_5
void glGetnPolygonStipple(GLsizei bufSize, GLubyte *pattern); // introduced GL_VERSION_4_5
void glGetnSeparableFilter(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span); // introduced GL_VERSION_4_5
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels); // introduced GL_VERSION_4_5
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params); // introduced GL_VERSION_4_5
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params); // introduced GL_VERSION_4_5
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params); // introduced GL_VERSION_4_5
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params); // introduced GL_VERSION_4_5
void glHint(GLenum target, GLenum mode); // introduced GL_VERSION_1_0
void glInvalidateBufferData(GLuint buffer); // introduced GL_VERSION_4_3
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length); // introduced GL_VERSION_4_3
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments); // introduced GL_VERSION_4_3
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments); // introduced GL_VERSION_4_5
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_4_3
void glInvalidateTexImage(GLuint texture, GLint level); // introduced GL_VERSION_4_3
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth); // introduced GL_VERSION_4_3
GLboolean glIsBuffer(GLuint buffer); // introduced GL_VERSION_1_5
GLboolean glIsEnabled(GLenum cap); // introduced GL_VERSION_1_0
GLboolean glIsEnabledi(GLenum target, GLuint index); // introduced GL_VERSION_3_0
GLboolean glIsFramebuffer(GLuint framebuffer); // introduced GL_VERSION_3_0
GLboolean glIsProgram(GLuint program); // introduced GL_VERSION_2_0
GLboolean glIsProgramPipeline(GLuint pipeline); // introduced GL_VERSION_4_1
GLboolean glIsQuery(GLuint id); // introduced GL_VERSION_1_5
GLboolean glIsRenderbuffer(GLuint renderbuffer); // introduced GL_VERSION_3_0
GLboolean glIsSampler(GLuint sampler); // introduced GL_VERSION_3_3
GLboolean glIsShader(GLuint shader); // introduced GL_VERSION_2_0
GLboolean glIsSync(GLsync sync); // introduced GL_VERSION_3_2
GLboolean glIsTexture(GLuint texture); // introduced GL_VERSION_1_1
GLboolean glIsTransformFeedback(GLuint id); // introduced GL_VERSION_4_0
GLboolean glIsVertexArray(GLuint array); // introduced GL_VERSION_3_0
void glLineWidth(GLfloat width); // introduced GL_VERSION_1_0
void glLinkProgram(GLuint program); // introduced GL_VERSION_2_0
void glLogicOp(GLenum opcode); // introduced GL_VERSION_1_0
void * glMapBuffer(GLenum target, GLenum access); // introduced GL_VERSION_1_5
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access); // introduced GL_VERSION_3_0
void * glMapNamedBuffer(GLuint buffer, GLenum access); // introduced GL_VERSION_4_5
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access); // introduced GL_VERSION_4_5
void glMemoryBarrier(GLbitfield barriers); // introduced GL_VERSION_4_2
void glMemoryBarrierByRegion(GLbitfield barriers); // introduced GL_VERSION_4_5
void glMinSampleShading(GLfloat value); // introduced GL_VERSION_4_0
void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount); // introduced GL_VERSION_1_4
void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride); // introduced GL_VERSION_4_3
void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride); // introduced GL_VERSION_4_6
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount); // introduced GL_VERSION_1_4
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex); // introduced GL_VERSION_3_2
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride); // introduced GL_VERSION_4_3
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride); // introduced GL_VERSION_4_6
void glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage); // introduced GL_VERSION_4_5
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags); // introduced GL_VERSION_4_5
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data); // introduced GL_VERSION_4_5
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf); // introduced GL_VERSION_4_5
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs); // introduced GL_VERSION_4_5
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param); // introduced GL_VERSION_4_5
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src); // introduced GL_VERSION_4_5
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer); // introduced GL_VERSION_4_5
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level); // introduced GL_VERSION_4_5
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer); // introduced GL_VERSION_4_5
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glNormalP3ui(GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glNormalP3uiv(GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label); // introduced GL_VERSION_4_3
void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label); // introduced GL_VERSION_4_3
void glPatchParameterfv(GLenum pname, const GLfloat *values); // introduced GL_VERSION_4_0
void glPatchParameteri(GLenum pname, GLint value); // introduced GL_VERSION_4_0
void glPauseTransformFeedback(void); // introduced GL_VERSION_4_0
void glPixelStoref(GLenum pname, GLfloat param); // introduced GL_VERSION_1_0
void glPixelStorei(GLenum pname, GLint param); // introduced GL_VERSION_1_0
void glPointParameterf(GLenum pname, GLfloat param); // introduced GL_VERSION_1_4
void glPointParameterfv(GLenum pname, const GLfloat *params); // introduced GL_VERSION_1_4
void glPointParameteri(GLenum pname, GLint param); // introduced GL_VERSION_1_4
void glPointParameteriv(GLenum pname, const GLint *params); // introduced GL_VERSION_1_4
void glPointSize(GLfloat size); // introduced GL_VERSION_1_0
void glPolygonMode(GLenum face, GLenum mode); // introduced GL_VERSION_1_0
void glPolygonOffset(GLfloat factor, GLfloat units); // introduced GL_VERSION_1_1
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp); // introduced GL_VERSION_4_6
void glPopDebugGroup(void); // introduced GL_VERSION_4_3
void glPrimitiveRestartIndex(GLuint index); // introduced GL_VERSION_3_1
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length); // introduced GL_VERSION_4_1
void glProgramParameteri(GLuint program, GLenum pname, GLint value); // introduced GL_VERSION_4_1
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0); // introduced GL_VERSION_4_1
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0); // introduced GL_VERSION_4_1
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniform1i(GLuint program, GLint location, GLint v0); // introduced GL_VERSION_4_1
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_4_1
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0); // introduced GL_VERSION_4_1
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_4_1
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1); // introduced GL_VERSION_4_1
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1); // introduced GL_VERSION_4_1
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1); // introduced GL_VERSION_4_1
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_4_1
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1); // introduced GL_VERSION_4_1
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_4_1
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2); // introduced GL_VERSION_4_1
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2); // introduced GL_VERSION_4_1
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2); // introduced GL_VERSION_4_1
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_4_1
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2); // introduced GL_VERSION_4_1
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_4_1
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3); // introduced GL_VERSION_4_1
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3); // introduced GL_VERSION_4_1
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3); // introduced GL_VERSION_4_1
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_4_1
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3); // introduced GL_VERSION_4_1
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_1
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_4_1
void glProvokingVertex(GLenum mode); // introduced GL_VERSION_3_2
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message); // introduced GL_VERSION_4_3
void glQueryCounter(GLuint id, GLenum target); // introduced GL_VERSION_3_3
void glReadBuffer(GLenum src); // introduced GL_VERSION_1_0
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels); // introduced GL_VERSION_1_0
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data); // introduced GL_VERSION_4_5
void glReleaseShaderCompiler(void); // introduced GL_VERSION_4_1
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_3_0
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_3_0
void glResumeTransformFeedback(void); // introduced GL_VERSION_4_0
void glSampleCoverage(GLfloat value, GLboolean invert); // introduced GL_VERSION_1_3
void glSampleMaski(GLuint maskNumber, GLbitfield mask); // introduced GL_VERSION_3_2
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param); // introduced GL_VERSION_3_3
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param); // introduced GL_VERSION_3_3
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param); // introduced GL_VERSION_3_3
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param); // introduced GL_VERSION_3_3
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param); // introduced GL_VERSION_3_3
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param); // introduced GL_VERSION_3_3
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_1_0
void glScissorArrayv(GLuint first, GLsizei count, const GLint *v); // introduced GL_VERSION_4_1
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height); // introduced GL_VERSION_4_1
void glScissorIndexedv(GLuint index, const GLint *v); // introduced GL_VERSION_4_1
void glSecondaryColorP3ui(GLenum type, GLuint color); // introduced GL_VERSION_3_3
void glSecondaryColorP3uiv(GLenum type, const GLuint *color); // introduced GL_VERSION_3_3
void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length); // introduced GL_VERSION_4_1
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length); // introduced GL_VERSION_2_0
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding); // introduced GL_VERSION_4_3
void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue); // introduced GL_VERSION_4_6
void glStencilFunc(GLenum func, GLint ref, GLuint mask); // introduced GL_VERSION_1_0
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask); // introduced GL_VERSION_2_0
void glStencilMask(GLuint mask); // introduced GL_VERSION_1_0
void glStencilMaskSeparate(GLenum face, GLuint mask); // introduced GL_VERSION_2_0
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass); // introduced GL_VERSION_1_0
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass); // introduced GL_VERSION_2_0
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer); // introduced GL_VERSION_3_1
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced GL_VERSION_4_3
void glTexCoordP1ui(GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glTexCoordP1uiv(GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glTexCoordP2ui(GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glTexCoordP2uiv(GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glTexCoordP3ui(GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glTexCoordP3uiv(GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glTexCoordP4ui(GLenum type, GLuint coords); // introduced GL_VERSION_3_3
void glTexCoordP4uiv(GLenum type, const GLuint *coords); // introduced GL_VERSION_3_3
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_0
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_0
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced GL_VERSION_3_2
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_2
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced GL_VERSION_3_2
void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params); // introduced GL_VERSION_3_0
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params); // introduced GL_VERSION_3_0
void glTexParameterf(GLenum target, GLenum pname, GLfloat param); // introduced GL_VERSION_1_0
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params); // introduced GL_VERSION_1_0
void glTexParameteri(GLenum target, GLenum pname, GLint param); // introduced GL_VERSION_1_0
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params); // introduced GL_VERSION_1_0
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width); // introduced GL_VERSION_4_2
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_4_2
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced GL_VERSION_4_3
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth); // introduced GL_VERSION_4_2
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced GL_VERSION_4_3
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_1
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_1
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_1_2
void glTextureBarrier(void); // introduced GL_VERSION_4_5
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer); // introduced GL_VERSION_4_5
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced GL_VERSION_4_5
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params); // introduced GL_VERSION_4_5
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params); // introduced GL_VERSION_4_5
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param); // introduced GL_VERSION_4_5
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param); // introduced GL_VERSION_4_5
void glTextureParameteri(GLuint texture, GLenum pname, GLint param); // introduced GL_VERSION_4_5
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param); // introduced GL_VERSION_4_5
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width); // introduced GL_VERSION_4_5
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height); // introduced GL_VERSION_4_5
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations); // introduced GL_VERSION_4_5
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth); // introduced GL_VERSION_4_5
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations); // introduced GL_VERSION_4_5
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_4_5
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_4_5
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels); // introduced GL_VERSION_4_5
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers); // introduced GL_VERSION_4_3
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer); // introduced GL_VERSION_4_5
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size); // introduced GL_VERSION_4_5
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode); // introduced GL_VERSION_3_0
void glUniform1d(GLint location, GLdouble x); // introduced GL_VERSION_4_0
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniform1f(GLint location, GLfloat v0); // introduced GL_VERSION_2_0
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniform1i(GLint location, GLint v0); // introduced GL_VERSION_2_0
void glUniform1iv(GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_2_0
void glUniform1ui(GLint location, GLuint v0); // introduced GL_VERSION_3_0
void glUniform1uiv(GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_3_0
void glUniform2d(GLint location, GLdouble x, GLdouble y); // introduced GL_VERSION_4_0
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniform2f(GLint location, GLfloat v0, GLfloat v1); // introduced GL_VERSION_2_0
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniform2i(GLint location, GLint v0, GLint v1); // introduced GL_VERSION_2_0
void glUniform2iv(GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_2_0
void glUniform2ui(GLint location, GLuint v0, GLuint v1); // introduced GL_VERSION_3_0
void glUniform2uiv(GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_3_0
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z); // introduced GL_VERSION_4_0
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2); // introduced GL_VERSION_2_0
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2); // introduced GL_VERSION_2_0
void glUniform3iv(GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_2_0
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2); // introduced GL_VERSION_3_0
void glUniform3uiv(GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_3_0
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced GL_VERSION_4_0
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3); // introduced GL_VERSION_2_0
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3); // introduced GL_VERSION_2_0
void glUniform4iv(GLint location, GLsizei count, const GLint *value); // introduced GL_VERSION_2_0
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3); // introduced GL_VERSION_3_0
void glUniform4uiv(GLint location, GLsizei count, const GLuint *value); // introduced GL_VERSION_3_0
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding); // introduced GL_VERSION_3_1
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_0
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value); // introduced GL_VERSION_4_0
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value); // introduced GL_VERSION_2_1
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices); // introduced GL_VERSION_4_0
GLboolean glUnmapBuffer(GLenum target); // introduced GL_VERSION_1_5
GLboolean glUnmapNamedBuffer(GLuint buffer); // introduced GL_VERSION_4_5
void glUseProgram(GLuint program); // introduced GL_VERSION_2_0
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program); // introduced GL_VERSION_4_1
void glValidateProgram(GLuint program); // introduced GL_VERSION_2_0
void glValidateProgramPipeline(GLuint pipeline); // introduced GL_VERSION_4_1
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex); // introduced GL_VERSION_4_5
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset); // introduced GL_VERSION_4_5
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced GL_VERSION_4_5
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced GL_VERSION_4_5
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor); // introduced GL_VERSION_4_5
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer); // introduced GL_VERSION_4_5
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride); // introduced GL_VERSION_4_5
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides); // introduced GL_VERSION_4_5
void glVertexAttrib1d(GLuint index, GLdouble x); // introduced GL_VERSION_2_0
void glVertexAttrib1dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_2_0
void glVertexAttrib1f(GLuint index, GLfloat x); // introduced GL_VERSION_2_0
void glVertexAttrib1fv(GLuint index, const GLfloat *v); // introduced GL_VERSION_2_0
void glVertexAttrib1s(GLuint index, GLshort x); // introduced GL_VERSION_2_0
void glVertexAttrib1sv(GLuint index, const GLshort *v); // introduced GL_VERSION_2_0
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y); // introduced GL_VERSION_2_0
void glVertexAttrib2dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_2_0
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y); // introduced GL_VERSION_2_0
void glVertexAttrib2fv(GLuint index, const GLfloat *v); // introduced GL_VERSION_2_0
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y); // introduced GL_VERSION_2_0
void glVertexAttrib2sv(GLuint index, const GLshort *v); // introduced GL_VERSION_2_0
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z); // introduced GL_VERSION_2_0
void glVertexAttrib3dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_2_0
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z); // introduced GL_VERSION_2_0
void glVertexAttrib3fv(GLuint index, const GLfloat *v); // introduced GL_VERSION_2_0
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z); // introduced GL_VERSION_2_0
void glVertexAttrib3sv(GLuint index, const GLshort *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Nbv(GLuint index, const GLbyte *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Niv(GLuint index, const GLint *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Nsv(GLuint index, const GLshort *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w); // introduced GL_VERSION_2_0
void glVertexAttrib4Nubv(GLuint index, const GLubyte *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Nuiv(GLuint index, const GLuint *v); // introduced GL_VERSION_2_0
void glVertexAttrib4Nusv(GLuint index, const GLushort *v); // introduced GL_VERSION_2_0
void glVertexAttrib4bv(GLuint index, const GLbyte *v); // introduced GL_VERSION_2_0
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced GL_VERSION_2_0
void glVertexAttrib4dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_2_0
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); // introduced GL_VERSION_2_0
void glVertexAttrib4fv(GLuint index, const GLfloat *v); // introduced GL_VERSION_2_0
void glVertexAttrib4iv(GLuint index, const GLint *v); // introduced GL_VERSION_2_0
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w); // introduced GL_VERSION_2_0
void glVertexAttrib4sv(GLuint index, const GLshort *v); // introduced GL_VERSION_2_0
void glVertexAttrib4ubv(GLuint index, const GLubyte *v); // introduced GL_VERSION_2_0
void glVertexAttrib4uiv(GLuint index, const GLuint *v); // introduced GL_VERSION_2_0
void glVertexAttrib4usv(GLuint index, const GLushort *v); // introduced GL_VERSION_2_0
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex); // introduced GL_VERSION_4_3
void glVertexAttribDivisor(GLuint index, GLuint divisor); // introduced GL_VERSION_3_3
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset); // introduced GL_VERSION_4_3
void glVertexAttribI1i(GLuint index, GLint x); // introduced GL_VERSION_3_0
void glVertexAttribI1iv(GLuint index, const GLint *v); // introduced GL_VERSION_3_0
void glVertexAttribI1ui(GLuint index, GLuint x); // introduced GL_VERSION_3_0
void glVertexAttribI1uiv(GLuint index, const GLuint *v); // introduced GL_VERSION_3_0
void glVertexAttribI2i(GLuint index, GLint x, GLint y); // introduced GL_VERSION_3_0
void glVertexAttribI2iv(GLuint index, const GLint *v); // introduced GL_VERSION_3_0
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y); // introduced GL_VERSION_3_0
void glVertexAttribI2uiv(GLuint index, const GLuint *v); // introduced GL_VERSION_3_0
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z); // introduced GL_VERSION_3_0
void glVertexAttribI3iv(GLuint index, const GLint *v); // introduced GL_VERSION_3_0
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z); // introduced GL_VERSION_3_0
void glVertexAttribI3uiv(GLuint index, const GLuint *v); // introduced GL_VERSION_3_0
void glVertexAttribI4bv(GLuint index, const GLbyte *v); // introduced GL_VERSION_3_0
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w); // introduced GL_VERSION_3_0
void glVertexAttribI4iv(GLuint index, const GLint *v); // introduced GL_VERSION_3_0
void glVertexAttribI4sv(GLuint index, const GLshort *v); // introduced GL_VERSION_3_0
void glVertexAttribI4ubv(GLuint index, const GLubyte *v); // introduced GL_VERSION_3_0
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w); // introduced GL_VERSION_3_0
void glVertexAttribI4uiv(GLuint index, const GLuint *v); // introduced GL_VERSION_3_0
void glVertexAttribI4usv(GLuint index, const GLushort *v); // introduced GL_VERSION_3_0
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced GL_VERSION_4_3
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer); // introduced GL_VERSION_3_0
void glVertexAttribL1d(GLuint index, GLdouble x); // introduced GL_VERSION_4_1
void glVertexAttribL1dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_4_1
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y); // introduced GL_VERSION_4_1
void glVertexAttribL2dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_4_1
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z); // introduced GL_VERSION_4_1
void glVertexAttribL3dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_4_1
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); // introduced GL_VERSION_4_1
void glVertexAttribL4dv(GLuint index, const GLdouble *v); // introduced GL_VERSION_4_1
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset); // introduced GL_VERSION_4_3
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer); // introduced GL_VERSION_4_1
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced GL_VERSION_3_3
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced GL_VERSION_3_3
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced GL_VERSION_3_3
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value); // introduced GL_VERSION_3_3
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer); // introduced GL_VERSION_2_0
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor); // introduced GL_VERSION_4_3
void glVertexP2ui(GLenum type, GLuint value); // introduced GL_VERSION_3_3
void glVertexP2uiv(GLenum type, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexP3ui(GLenum type, GLuint value); // introduced GL_VERSION_3_3
void glVertexP3uiv(GLenum type, const GLuint *value); // introduced GL_VERSION_3_3
void glVertexP4ui(GLenum type, GLuint value); // introduced GL_VERSION_3_3
void glVertexP4uiv(GLenum type, const GLuint *value); // introduced GL_VERSION_3_3
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height); // introduced GL_VERSION_1_0
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v); // introduced GL_VERSION_4_1
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h); // introduced GL_VERSION_4_1
void glViewportIndexedfv(GLuint index, const GLfloat *v); // introduced GL_VERSION_4_1
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout); // introduced GL_VERSION_3_2
} // namespace gl
// Số extension glGetStringi liệt kê (glGetIntegerv(GL_NUM_EXTENSIONS) dùng
// hàm này — phải khớp tuyệt đối).
GLuint tglmt_num_extensions();
int GetCoreFunctionCount(); // = 698
} // namespace tglmt
