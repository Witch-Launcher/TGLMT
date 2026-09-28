// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.
// extern "C" forward 1:1 sang tglmt::gl:: để LWJGL dlsym("glXxx") thấy.
#include "tglmt/gl46.h"
using namespace tglmt;
namespace tglmt { namespace gl {
void glGetPointerv(GLenum p, void** v);
void glLineStipple(GLint a, GLushort b);
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c);
} }

#ifdef __cplusplus
extern "C" {
#endif

void glActiveShaderProgram(GLuint pipeline, GLuint program) {
    gl::glActiveShaderProgram(pipeline, program);
}
void glActiveTexture(GLenum texture) {
    gl::glActiveTexture(texture);
}
void glAttachShader(GLuint program, GLuint shader) {
    gl::glAttachShader(program, shader);
}
void glBeginConditionalRender(GLuint id, GLenum mode) {
    gl::glBeginConditionalRender(id, mode);
}
void glBeginQuery(GLenum target, GLuint id) {
    gl::glBeginQuery(target, id);
}
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {
    gl::glBeginQueryIndexed(target, index, id);
}
void glBeginTransformFeedback(GLenum primitiveMode) {
    gl::glBeginTransformFeedback(primitiveMode);
}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
    gl::glBindAttribLocation(program, index, name);
}
void glBindBuffer(GLenum target, GLuint buffer) {
    gl::glBindBuffer(target, buffer);
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    gl::glBindBufferBase(target, index, buffer);
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    gl::glBindBufferRange(target, index, buffer, offset, size);
}
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers) {
    gl::glBindBuffersBase(target, first, count, buffers);
}
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes) {
    gl::glBindBuffersRange(target, first, count, buffers, offsets, sizes);
}
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name) {
    gl::glBindFragDataLocation(program, color, name);
}
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name) {
    gl::glBindFragDataLocationIndexed(program, colorNumber, index, name);
}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    gl::glBindFramebuffer(target, framebuffer);
}
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    gl::glBindImageTexture(unit, texture, level, layered, layer, access, format);
}
void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures) {
    gl::glBindImageTextures(first, count, textures);
}
void glBindProgramPipeline(GLuint pipeline) {
    gl::glBindProgramPipeline(pipeline);
}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    gl::glBindRenderbuffer(target, renderbuffer);
}
void glBindSampler(GLuint unit, GLuint sampler) {
    gl::glBindSampler(unit, sampler);
}
void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers) {
    gl::glBindSamplers(first, count, samplers);
}
void glBindTexture(GLenum target, GLuint texture) {
    gl::glBindTexture(target, texture);
}
void glBindTextureUnit(GLuint unit, GLuint texture) {
    gl::glBindTextureUnit(unit, texture);
}
void glBindTextures(GLuint first, GLsizei count, const GLuint *textures) {
    gl::glBindTextures(first, count, textures);
}
void glBindTransformFeedback(GLenum target, GLuint id) {
    gl::glBindTransformFeedback(target, id);
}
void glBindVertexArray(GLuint array) {
    gl::glBindVertexArray(array);
}
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    gl::glBindVertexBuffer(bindingindex, buffer, offset, stride);
}
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    gl::glBindVertexBuffers(first, count, buffers, offsets, strides);
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    gl::glBlendColor(red, green, blue, alpha);
}
void glBlendEquation(GLenum mode) {
    gl::glBlendEquation(mode);
}
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    gl::glBlendEquationSeparate(modeRGB, modeAlpha);
}
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    gl::glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}
void glBlendEquationi(GLuint buf, GLenum mode) {
    gl::glBlendEquationi(buf, mode);
}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    gl::glBlendFunc(sfactor, dfactor);
}
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    gl::glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    gl::glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {
    gl::glBlendFunci(buf, src, dst);
}
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    gl::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    gl::glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    gl::glBufferData(target, size, data, usage);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {
    gl::glBufferStorage(target, size, data, flags);
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    gl::glBufferSubData(target, offset, size, data);
}
GLenum glCheckFramebufferStatus(GLenum target) {
    return gl::glCheckFramebufferStatus(target);
}
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    return gl::glCheckNamedFramebufferStatus(framebuffer, target);
}
void glClampColor(GLenum target, GLenum clamp) {
    gl::glClampColor(target, clamp);
}
void glClear(GLbitfield mask) {
    gl::glClear(mask);
}
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    gl::glClearBufferData(target, internalformat, format, type, data);
}
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    gl::glClearBufferSubData(target, internalformat, offset, size, format, type, data);
}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    gl::glClearBufferfi(buffer, drawbuffer, depth, stencil);
}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    gl::glClearBufferfv(buffer, drawbuffer, value);
}
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) {
    gl::glClearBufferiv(buffer, drawbuffer, value);
}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    gl::glClearBufferuiv(buffer, drawbuffer, value);
}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    gl::glClearColor(red, green, blue, alpha);
}
void glClearDepth(GLdouble depth) {
    gl::glClearDepth(depth);
}
void glClearDepthf(GLfloat d) {
    gl::glClearDepthf(d);
}
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    gl::glClearNamedBufferData(buffer, internalformat, format, type, data);
}
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    gl::glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data);
}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    gl::glClearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth, stencil);
}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    gl::glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value) {
    gl::glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value) {
    gl::glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value);
}
void glClearStencil(GLint s) {
    gl::glClearStencil(s);
}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {
    gl::glClearTexImage(texture, level, format, type, data);
}
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data) {
    gl::glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    return gl::glClientWaitSync(sync, flags, timeout);
}
void glClipControl(GLenum origin, GLenum depth) {
    gl::glClipControl(origin, depth);
}
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    gl::glColorMask(red, green, blue, alpha);
}
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    gl::glColorMaski(index, r, g, b, a);
}
void glColorP3ui(GLenum type, GLuint color) {
    gl::glColorP3ui(type, color);
}
void glColorP3uiv(GLenum type, const GLuint *color) {
    gl::glColorP3uiv(type, color);
}
void glColorP4ui(GLenum type, GLuint color) {
    gl::glColorP4ui(type, color);
}
void glColorP4uiv(GLenum type, const GLuint *color) {
    gl::glColorP4uiv(type, color);
}
void glCompileShader(GLuint shader) {
    gl::glCompileShader(shader);
}
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) {
    gl::glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    gl::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {
    gl::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
}
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data);
}
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    gl::glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    gl::glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    gl::glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
}
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    gl::glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    gl::glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    gl::glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    gl::glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    gl::glCopyTextureSubImage1D(texture, level, xoffset, x, y, width);
}
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glCopyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glCopyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCreateBuffers(GLsizei n, GLuint *buffers) {
    gl::glCreateBuffers(n, buffers);
}
void glCreateFramebuffers(GLsizei n, GLuint *framebuffers) {
    gl::glCreateFramebuffers(n, framebuffers);
}
GLuint glCreateProgram(void) {
    return gl::glCreateProgram();
}
void glCreateProgramPipelines(GLsizei n, GLuint *pipelines) {
    gl::glCreateProgramPipelines(n, pipelines);
}
void glCreateQueries(GLenum target, GLsizei n, GLuint *ids) {
    gl::glCreateQueries(target, n, ids);
}
void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    gl::glCreateRenderbuffers(n, renderbuffers);
}
void glCreateSamplers(GLsizei n, GLuint *samplers) {
    gl::glCreateSamplers(n, samplers);
}
GLuint glCreateShader(GLenum type) {
    return gl::glCreateShader(type);
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings) {
    return gl::glCreateShaderProgramv(type, count, strings);
}
void glCreateTextures(GLenum target, GLsizei n, GLuint *textures) {
    gl::glCreateTextures(target, n, textures);
}
void glCreateTransformFeedbacks(GLsizei n, GLuint *ids) {
    gl::glCreateTransformFeedbacks(n, ids);
}
void glCreateVertexArrays(GLsizei n, GLuint *arrays) {
    gl::glCreateVertexArrays(n, arrays);
}
void glCullFace(GLenum mode) {
    gl::glCullFace(mode);
}
void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) {
    gl::glDebugMessageCallback(callback, userParam);
}
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) {
    gl::glDebugMessageControl(source, type, severity, count, ids, enabled);
}
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) {
    gl::glDebugMessageInsert(source, type, id, severity, length, buf);
}
void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    gl::glDeleteBuffers(n, buffers);
}
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    gl::glDeleteFramebuffers(n, framebuffers);
}
void glDeleteProgram(GLuint program) {
    gl::glDeleteProgram(program);
}
void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines) {
    gl::glDeleteProgramPipelines(n, pipelines);
}
void glDeleteQueries(GLsizei n, const GLuint *ids) {
    gl::glDeleteQueries(n, ids);
}
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    gl::glDeleteRenderbuffers(n, renderbuffers);
}
void glDeleteSamplers(GLsizei count, const GLuint *samplers) {
    gl::glDeleteSamplers(count, samplers);
}
void glDeleteShader(GLuint shader) {
    gl::glDeleteShader(shader);
}
void glDeleteSync(GLsync sync) {
    gl::glDeleteSync(sync);
}
void glDeleteTextures(GLsizei n, const GLuint *textures) {
    gl::glDeleteTextures(n, textures);
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids) {
    gl::glDeleteTransformFeedbacks(n, ids);
}
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    gl::glDeleteVertexArrays(n, arrays);
}
void glDepthFunc(GLenum func) {
    gl::glDepthFunc(func);
}
void glDepthMask(GLboolean flag) {
    gl::glDepthMask(flag);
}
void glDepthRange(GLdouble n, GLdouble f) {
    gl::glDepthRange(n, f);
}
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v) {
    gl::glDepthRangeArrayv(first, count, v);
}
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) {
    gl::glDepthRangeIndexed(index, n, f);
}
void glDepthRangef(GLfloat n, GLfloat f) {
    gl::glDepthRangef(n, f);
}
void glDetachShader(GLuint program, GLuint shader) {
    gl::glDetachShader(program, shader);
}
void glDisable(GLenum cap) {
    gl::glDisable(cap);
}
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    gl::glDisableVertexArrayAttrib(vaobj, index);
}
void glDisableVertexAttribArray(GLuint index) {
    gl::glDisableVertexAttribArray(index);
}
void glDisablei(GLenum target, GLuint index) {
    gl::glDisablei(target, index);
}
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    gl::glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}
void glDispatchComputeIndirect(GLintptr indirect) {
    gl::glDispatchComputeIndirect(indirect);
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    gl::glDrawArrays(mode, first, count);
}
void glDrawArraysIndirect(GLenum mode, const void *indirect) {
    gl::glDrawArraysIndirect(mode, indirect);
}
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    gl::glDrawArraysInstanced(mode, first, count, instancecount);
}
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
    gl::glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance);
}
void glDrawBuffer(GLenum buf) {
    gl::glDrawBuffer(buf);
}
void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    gl::glDrawBuffers(n, bufs);
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    gl::glDrawElements(mode, count, type, indices);
}
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    gl::glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}
void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {
    gl::glDrawElementsIndirect(mode, type, indirect);
}
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    gl::glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance) {
    gl::glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
}
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
    gl::glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
    gl::glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    gl::glDrawRangeElements(mode, start, end, count, type, indices);
}
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    gl::glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}
void glDrawTransformFeedback(GLenum mode, GLuint id) {
    gl::glDrawTransformFeedback(mode, id);
}
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) {
    gl::glDrawTransformFeedbackInstanced(mode, id, instancecount);
}
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) {
    gl::glDrawTransformFeedbackStream(mode, id, stream);
}
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) {
    gl::glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount);
}
void glEnable(GLenum cap) {
    gl::glEnable(cap);
}
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    gl::glEnableVertexArrayAttrib(vaobj, index);
}
void glEnableVertexAttribArray(GLuint index) {
    gl::glEnableVertexAttribArray(index);
}
void glEnablei(GLenum target, GLuint index) {
    gl::glEnablei(target, index);
}
void glEndConditionalRender(void) {
    gl::glEndConditionalRender();
}
void glEndQuery(GLenum target) {
    gl::glEndQuery(target);
}
void glEndQueryIndexed(GLenum target, GLuint index) {
    gl::glEndQueryIndexed(target, index);
}
void glEndTransformFeedback(void) {
    gl::glEndTransformFeedback();
}
GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    return gl::glFenceSync(condition, flags);
}
void glFinish(void) {
    gl::glFinish();
}
void glFlush(void) {
    gl::glFlush();
}
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    gl::glFlushMappedBufferRange(target, offset, length);
}
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    gl::glFlushMappedNamedBufferRange(buffer, offset, length);
}
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
    gl::glFramebufferParameteri(target, pname, param);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    gl::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    gl::glFramebufferTexture(target, attachment, texture, level);
}
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    gl::glFramebufferTexture1D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    gl::glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    gl::glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    gl::glFramebufferTextureLayer(target, attachment, texture, level, layer);
}
void glFrontFace(GLenum mode) {
    gl::glFrontFace(mode);
}
void glGenBuffers(GLsizei n, GLuint *buffers) {
    gl::glGenBuffers(n, buffers);
}
void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    gl::glGenFramebuffers(n, framebuffers);
}
void glGenProgramPipelines(GLsizei n, GLuint *pipelines) {
    gl::glGenProgramPipelines(n, pipelines);
}
void glGenQueries(GLsizei n, GLuint *ids) {
    gl::glGenQueries(n, ids);
}
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    gl::glGenRenderbuffers(n, renderbuffers);
}
void glGenSamplers(GLsizei count, GLuint *samplers) {
    gl::glGenSamplers(count, samplers);
}
void glGenTextures(GLsizei n, GLuint *textures) {
    gl::glGenTextures(n, textures);
}
void glGenTransformFeedbacks(GLsizei n, GLuint *ids) {
    gl::glGenTransformFeedbacks(n, ids);
}
void glGenVertexArrays(GLsizei n, GLuint *arrays) {
    gl::glGenVertexArrays(n, arrays);
}
void glGenerateMipmap(GLenum target) {
    gl::glGenerateMipmap(target);
}
void glGenerateTextureMipmap(GLuint texture) {
    gl::glGenerateTextureMipmap(texture);
}
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params) {
    gl::glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params);
}
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    gl::glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    gl::glGetActiveSubroutineName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    gl::glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values) {
    gl::glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values);
}
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    gl::glGetActiveUniform(program, index, bufSize, length, size, type, name);
}
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) {
    gl::glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) {
    gl::glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName) {
    gl::glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) {
    gl::glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    gl::glGetAttachedShaders(program, maxCount, count, shaders);
}
GLint glGetAttribLocation(GLuint program, const GLchar *name) {
    return gl::glGetAttribLocation(program, name);
}
void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data) {
    gl::glGetBooleani_v(target, index, data);
}
void glGetBooleanv(GLenum pname, GLboolean *data) {
    gl::glGetBooleanv(pname, data);
}
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) {
    gl::glGetBufferParameteri64v(target, pname, params);
}
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetBufferParameteriv(target, pname, params);
}
void glGetBufferPointerv(GLenum target, GLenum pname, void **params) {
    gl::glGetBufferPointerv(target, pname, params);
}
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data) {
    gl::glGetBufferSubData(target, offset, size, data);
}
void glGetCompressedTexImage(GLenum target, GLint level, void *img) {
    gl::glGetCompressedTexImage(target, level, img);
}
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels) {
    gl::glGetCompressedTextureImage(texture, level, bufSize, pixels);
}
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels) {
    gl::glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels);
}
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) {
    return gl::glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog);
}
void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data) {
    gl::glGetDoublei_v(target, index, data);
}
void glGetDoublev(GLenum pname, GLdouble *data) {
    gl::glGetDoublev(pname, data);
}
GLenum glGetError(void) {
    return gl::glGetError();
}
void glGetFloati_v(GLenum target, GLuint index, GLfloat *data) {
    gl::glGetFloati_v(target, index, data);
}
void glGetFloatv(GLenum pname, GLfloat *data) {
    gl::glGetFloatv(pname, data);
}
GLint glGetFragDataIndex(GLuint program, const GLchar *name) {
    return gl::glGetFragDataIndex(program, name);
}
GLint glGetFragDataLocation(GLuint program, const GLchar *name) {
    return gl::glGetFragDataLocation(program, name);
}
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    gl::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetFramebufferParameteriv(target, pname, params);
}
GLenum glGetGraphicsResetStatus(void) {
    return gl::glGetGraphicsResetStatus();
}
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) {
    gl::glGetInteger64i_v(target, index, data);
}
void glGetInteger64v(GLenum pname, GLint64 *data) {
    gl::glGetInteger64v(pname, data);
}
void glGetIntegeri_v(GLenum target, GLuint index, GLint *data) {
    gl::glGetIntegeri_v(target, index, data);
}
void glGetIntegerv(GLenum pname, GLint *data) {
    gl::glGetIntegerv(pname, data);
}
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params) {
    gl::glGetInternalformati64v(target, internalformat, pname, count, params);
}
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params) {
    gl::glGetInternalformativ(target, internalformat, pname, count, params);
}
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val) {
    gl::glGetMultisamplefv(pname, index, val);
}
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params) {
    gl::glGetNamedBufferParameteri64v(buffer, pname, params);
}
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params) {
    gl::glGetNamedBufferParameteriv(buffer, pname, params);
}
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params) {
    gl::glGetNamedBufferPointerv(buffer, pname, params);
}
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data) {
    gl::glGetNamedBufferSubData(buffer, offset, size, data);
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) {
    gl::glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, params);
}
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param) {
    gl::glGetNamedFramebufferParameteriv(framebuffer, pname, param);
}
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params) {
    gl::glGetNamedRenderbufferParameteriv(renderbuffer, pname, params);
}
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label) {
    gl::glGetObjectLabel(identifier, name, bufSize, length, label);
}
void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label) {
    gl::glGetObjectPtrLabel(ptr, bufSize, length, label);
}
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
    gl::glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    gl::glGetProgramInfoLog(program, bufSize, length, infoLog);
}
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params) {
    gl::glGetProgramInterfaceiv(program, programInterface, pname, params);
}
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    gl::glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
}
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params) {
    gl::glGetProgramPipelineiv(pipeline, pname, params);
}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    return gl::glGetProgramResourceIndex(program, programInterface, name);
}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name) {
    return gl::glGetProgramResourceLocation(program, programInterface, name);
}
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    return gl::glGetProgramResourceLocationIndex(program, programInterface, name);
}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    gl::glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params) {
    gl::glGetProgramResourceiv(program, programInterface, index, propCount, props, count, length, params);
}
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values) {
    gl::glGetProgramStageiv(program, shadertype, pname, values);
}
void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    gl::glGetProgramiv(program, pname, params);
}
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    gl::glGetQueryBufferObjecti64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    gl::glGetQueryBufferObjectiv(id, buffer, pname, offset);
}
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    gl::glGetQueryBufferObjectui64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    gl::glGetQueryBufferObjectuiv(id, buffer, pname, offset);
}
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params) {
    gl::glGetQueryIndexediv(target, index, pname, params);
}
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) {
    gl::glGetQueryObjecti64v(id, pname, params);
}
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) {
    gl::glGetQueryObjectiv(id, pname, params);
}
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) {
    gl::glGetQueryObjectui64v(id, pname, params);
}
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {
    gl::glGetQueryObjectuiv(id, pname, params);
}
void glGetQueryiv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetQueryiv(target, pname, params);
}
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetRenderbufferParameteriv(target, pname, params);
}
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params) {
    gl::glGetSamplerParameterIiv(sampler, pname, params);
}
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params) {
    gl::glGetSamplerParameterIuiv(sampler, pname, params);
}
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params) {
    gl::glGetSamplerParameterfv(sampler, pname, params);
}
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params) {
    gl::glGetSamplerParameteriv(sampler, pname, params);
}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    gl::glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    gl::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    gl::glGetShaderSource(shader, bufSize, length, source);
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    gl::glGetShaderiv(shader, pname, params);
}
const GLubyte * glGetString(GLenum name) {
    return gl::glGetString(name);
}
const GLubyte * glGetStringi(GLenum name, GLuint index) {
    return gl::glGetStringi(name, index);
}
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name) {
    return gl::glGetSubroutineIndex(program, shadertype, name);
}
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name) {
    return gl::glGetSubroutineUniformLocation(program, shadertype, name);
}
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values) {
    gl::glGetSynciv(sync, pname, count, length, values);
}
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels) {
    gl::glGetTexImage(target, level, format, type, pixels);
}
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    gl::glGetTexLevelParameterfv(target, level, pname, params);
}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    gl::glGetTexLevelParameteriv(target, level, pname, params);
}
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetTexParameterIiv(target, pname, params);
}
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params) {
    gl::glGetTexParameterIuiv(target, pname, params);
}
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    gl::glGetTexParameterfv(target, pname, params);
}
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    gl::glGetTexParameteriv(target, pname, params);
}
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    gl::glGetTextureImage(texture, level, format, type, bufSize, pixels);
}
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params) {
    gl::glGetTextureLevelParameterfv(texture, level, pname, params);
}
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params) {
    gl::glGetTextureLevelParameteriv(texture, level, pname, params);
}
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params) {
    gl::glGetTextureParameterIiv(texture, pname, params);
}
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params) {
    gl::glGetTextureParameterIuiv(texture, pname, params);
}
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params) {
    gl::glGetTextureParameterfv(texture, pname, params);
}
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params) {
    gl::glGetTextureParameteriv(texture, pname, params);
}
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    gl::glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
}
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) {
    gl::glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param) {
    gl::glGetTransformFeedbacki64_v(xfb, pname, index, param);
}
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param) {
    gl::glGetTransformFeedbacki_v(xfb, pname, index, param);
}
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param) {
    gl::glGetTransformFeedbackiv(xfb, pname, param);
}
GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) {
    return gl::glGetUniformBlockIndex(program, uniformBlockName);
}
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) {
    gl::glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}
GLint glGetUniformLocation(GLuint program, const GLchar *name) {
    return gl::glGetUniformLocation(program, name);
}
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params) {
    gl::glGetUniformSubroutineuiv(shadertype, location, params);
}
void glGetUniformdv(GLuint program, GLint location, GLdouble *params) {
    gl::glGetUniformdv(program, location, params);
}
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
    gl::glGetUniformfv(program, location, params);
}
void glGetUniformiv(GLuint program, GLint location, GLint *params) {
    gl::glGetUniformiv(program, location, params);
}
void glGetUniformuiv(GLuint program, GLint location, GLuint *params) {
    gl::glGetUniformuiv(program, location, params);
}
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param) {
    gl::glGetVertexArrayIndexed64iv(vaobj, index, pname, param);
}
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param) {
    gl::glGetVertexArrayIndexediv(vaobj, index, pname, param);
}
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param) {
    gl::glGetVertexArrayiv(vaobj, pname, param);
}
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params) {
    gl::glGetVertexAttribIiv(index, pname, params);
}
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params) {
    gl::glGetVertexAttribIuiv(index, pname, params);
}
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params) {
    gl::glGetVertexAttribLdv(index, pname, params);
}
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
    gl::glGetVertexAttribPointerv(index, pname, pointer);
}
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params) {
    gl::glGetVertexAttribdv(index, pname, params);
}
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
    gl::glGetVertexAttribfv(index, pname, params);
}
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
    gl::glGetVertexAttribiv(index, pname, params);
}
void glGetnColorTable(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table) {
    gl::glGetnColorTable(target, format, type, bufSize, table);
}
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels) {
    gl::glGetnCompressedTexImage(target, lod, bufSize, pixels);
}
void glGetnConvolutionFilter(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image) {
    gl::glGetnConvolutionFilter(target, format, type, bufSize, image);
}
void glGetnHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    gl::glGetnHistogram(target, reset, format, type, bufSize, values);
}
void glGetnMapdv(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v) {
    gl::glGetnMapdv(target, query, bufSize, v);
}
void glGetnMapfv(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v) {
    gl::glGetnMapfv(target, query, bufSize, v);
}
void glGetnMapiv(GLenum target, GLenum query, GLsizei bufSize, GLint *v) {
    gl::glGetnMapiv(target, query, bufSize, v);
}
void glGetnMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    gl::glGetnMinmax(target, reset, format, type, bufSize, values);
}
void glGetnPixelMapfv(GLenum map, GLsizei bufSize, GLfloat *values) {
    gl::glGetnPixelMapfv(map, bufSize, values);
}
void glGetnPixelMapuiv(GLenum map, GLsizei bufSize, GLuint *values) {
    gl::glGetnPixelMapuiv(map, bufSize, values);
}
void glGetnPixelMapusv(GLenum map, GLsizei bufSize, GLushort *values) {
    gl::glGetnPixelMapusv(map, bufSize, values);
}
void glGetnPolygonStipple(GLsizei bufSize, GLubyte *pattern) {
    gl::glGetnPolygonStipple(bufSize, pattern);
}
void glGetnSeparableFilter(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span) {
    gl::glGetnSeparableFilter(target, format, type, rowBufSize, row, columnBufSize, column, span);
}
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    gl::glGetnTexImage(target, level, format, type, bufSize, pixels);
}
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params) {
    gl::glGetnUniformdv(program, location, bufSize, params);
}
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) {
    gl::glGetnUniformfv(program, location, bufSize, params);
}
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
    gl::glGetnUniformiv(program, location, bufSize, params);
}
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params) {
    gl::glGetnUniformuiv(program, location, bufSize, params);
}
void glHint(GLenum target, GLenum mode) {
    gl::glHint(target, mode);
}
void glInvalidateBufferData(GLuint buffer) {
    gl::glInvalidateBufferData(buffer);
}
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    gl::glInvalidateBufferSubData(buffer, offset, length);
}
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    gl::glInvalidateFramebuffer(target, numAttachments, attachments);
}
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments) {
    gl::glInvalidateNamedFramebufferData(framebuffer, numAttachments, attachments);
}
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, attachments, x, y, width, height);
}
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}
void glInvalidateTexImage(GLuint texture, GLint level) {
    gl::glInvalidateTexImage(texture, level);
}
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {
    gl::glInvalidateTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth);
}
GLboolean glIsBuffer(GLuint buffer) {
    return gl::glIsBuffer(buffer);
}
GLboolean glIsEnabled(GLenum cap) {
    return gl::glIsEnabled(cap);
}
GLboolean glIsEnabledi(GLenum target, GLuint index) {
    return gl::glIsEnabledi(target, index);
}
GLboolean glIsFramebuffer(GLuint framebuffer) {
    return gl::glIsFramebuffer(framebuffer);
}
GLboolean glIsProgram(GLuint program) {
    return gl::glIsProgram(program);
}
GLboolean glIsProgramPipeline(GLuint pipeline) {
    return gl::glIsProgramPipeline(pipeline);
}
GLboolean glIsQuery(GLuint id) {
    return gl::glIsQuery(id);
}
GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    return gl::glIsRenderbuffer(renderbuffer);
}
GLboolean glIsSampler(GLuint sampler) {
    return gl::glIsSampler(sampler);
}
GLboolean glIsShader(GLuint shader) {
    return gl::glIsShader(shader);
}
GLboolean glIsSync(GLsync sync) {
    return gl::glIsSync(sync);
}
GLboolean glIsTexture(GLuint texture) {
    return gl::glIsTexture(texture);
}
GLboolean glIsTransformFeedback(GLuint id) {
    return gl::glIsTransformFeedback(id);
}
GLboolean glIsVertexArray(GLuint array) {
    return gl::glIsVertexArray(array);
}
void glLineWidth(GLfloat width) {
    gl::glLineWidth(width);
}
void glLinkProgram(GLuint program) {
    gl::glLinkProgram(program);
}
void glLogicOp(GLenum opcode) {
    gl::glLogicOp(opcode);
}
void * glMapBuffer(GLenum target, GLenum access) {
    return gl::glMapBuffer(target, access);
}
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    return gl::glMapBufferRange(target, offset, length, access);
}
void * glMapNamedBuffer(GLuint buffer, GLenum access) {
    return gl::glMapNamedBuffer(buffer, access);
}
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    return gl::glMapNamedBufferRange(buffer, offset, length, access);
}
void glMemoryBarrier(GLbitfield barriers) {
    gl::glMemoryBarrier(barriers);
}
void glMemoryBarrierByRegion(GLbitfield barriers) {
    gl::glMemoryBarrierByRegion(barriers);
}
void glMinSampleShading(GLfloat value) {
    gl::glMinSampleShading(value);
}
void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {
    gl::glMultiDrawArrays(mode, first, count, drawcount);
}
void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) {
    gl::glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}
void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    gl::glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
}
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount) {
    gl::glMultiDrawElements(mode, count, type, indices, drawcount);
}
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex) {
    gl::glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex);
}
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {
    gl::glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    gl::glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
}
void glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint coords) {
    gl::glMultiTexCoordP1ui(texture, type, coords);
}
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint *coords) {
    gl::glMultiTexCoordP1uiv(texture, type, coords);
}
void glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint coords) {
    gl::glMultiTexCoordP2ui(texture, type, coords);
}
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint *coords) {
    gl::glMultiTexCoordP2uiv(texture, type, coords);
}
void glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint coords) {
    gl::glMultiTexCoordP3ui(texture, type, coords);
}
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint *coords) {
    gl::glMultiTexCoordP3uiv(texture, type, coords);
}
void glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint coords) {
    gl::glMultiTexCoordP4ui(texture, type, coords);
}
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint *coords) {
    gl::glMultiTexCoordP4uiv(texture, type, coords);
}
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) {
    gl::glNamedBufferData(buffer, size, data, usage);
}
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags) {
    gl::glNamedBufferStorage(buffer, size, data, flags);
}
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) {
    gl::glNamedBufferSubData(buffer, offset, size, data);
}
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {
    gl::glNamedFramebufferDrawBuffer(framebuffer, buf);
}
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs) {
    gl::glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {
    gl::glNamedFramebufferParameteri(framebuffer, pname, param);
}
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {
    gl::glNamedFramebufferReadBuffer(framebuffer, src);
}
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    gl::glNamedFramebufferRenderbuffer(framebuffer, attachment, renderbuffertarget, renderbuffer);
}
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    gl::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
}
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glNamedRenderbufferStorage(renderbuffer, internalformat, width, height);
}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height);
}
void glNormalP3ui(GLenum type, GLuint coords) {
    gl::glNormalP3ui(type, coords);
}
void glNormalP3uiv(GLenum type, const GLuint *coords) {
    gl::glNormalP3uiv(type, coords);
}
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) {
    gl::glObjectLabel(identifier, name, length, label);
}
void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label) {
    gl::glObjectPtrLabel(ptr, length, label);
}
void glPatchParameterfv(GLenum pname, const GLfloat *values) {
    gl::glPatchParameterfv(pname, values);
}
void glPatchParameteri(GLenum pname, GLint value) {
    gl::glPatchParameteri(pname, value);
}
void glPauseTransformFeedback(void) {
    gl::glPauseTransformFeedback();
}
void glPixelStoref(GLenum pname, GLfloat param) {
    gl::glPixelStoref(pname, param);
}
void glPixelStorei(GLenum pname, GLint param) {
    gl::glPixelStorei(pname, param);
}
void glPointParameterf(GLenum pname, GLfloat param) {
    gl::glPointParameterf(pname, param);
}
void glPointParameterfv(GLenum pname, const GLfloat *params) {
    gl::glPointParameterfv(pname, params);
}
void glPointParameteri(GLenum pname, GLint param) {
    gl::glPointParameteri(pname, param);
}
void glPointParameteriv(GLenum pname, const GLint *params) {
    gl::glPointParameteriv(pname, params);
}
void glPointSize(GLfloat size) {
    gl::glPointSize(size);
}
void glPolygonMode(GLenum face, GLenum mode) {
    gl::glPolygonMode(face, mode);
}
void glPolygonOffset(GLfloat factor, GLfloat units) {
    gl::glPolygonOffset(factor, units);
}
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
    gl::glPolygonOffsetClamp(factor, units, clamp);
}
void glPopDebugGroup(void) {
    gl::glPopDebugGroup();
}
void glPrimitiveRestartIndex(GLuint index) {
    gl::glPrimitiveRestartIndex(index);
}
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) {
    gl::glProgramBinary(program, binaryFormat, binary, length);
}
void glProgramParameteri(GLuint program, GLenum pname, GLint value) {
    gl::glProgramParameteri(program, pname, value);
}
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0) {
    gl::glProgramUniform1d(program, location, v0);
}
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    gl::glProgramUniform1dv(program, location, count, value);
}
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    gl::glProgramUniform1f(program, location, v0);
}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    gl::glProgramUniform1fv(program, location, count, value);
}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    gl::glProgramUniform1i(program, location, v0);
}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    gl::glProgramUniform1iv(program, location, count, value);
}
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
    gl::glProgramUniform1ui(program, location, v0);
}
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    gl::glProgramUniform1uiv(program, location, count, value);
}
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) {
    gl::glProgramUniform2d(program, location, v0, v1);
}
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    gl::glProgramUniform2dv(program, location, count, value);
}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    gl::glProgramUniform2f(program, location, v0, v1);
}
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    gl::glProgramUniform2fv(program, location, count, value);
}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    gl::glProgramUniform2i(program, location, v0, v1);
}
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    gl::glProgramUniform2iv(program, location, count, value);
}
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {
    gl::glProgramUniform2ui(program, location, v0, v1);
}
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    gl::glProgramUniform2uiv(program, location, count, value);
}
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) {
    gl::glProgramUniform3d(program, location, v0, v1, v2);
}
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    gl::glProgramUniform3dv(program, location, count, value);
}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    gl::glProgramUniform3f(program, location, v0, v1, v2);
}
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    gl::glProgramUniform3fv(program, location, count, value);
}
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    gl::glProgramUniform3i(program, location, v0, v1, v2);
}
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    gl::glProgramUniform3iv(program, location, count, value);
}
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    gl::glProgramUniform3ui(program, location, v0, v1, v2);
}
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    gl::glProgramUniform3uiv(program, location, count, value);
}
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) {
    gl::glProgramUniform4d(program, location, v0, v1, v2, v3);
}
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    gl::glProgramUniform4dv(program, location, count, value);
}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    gl::glProgramUniform4f(program, location, v0, v1, v2, v3);
}
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    gl::glProgramUniform4fv(program, location, count, value);
}
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    gl::glProgramUniform4i(program, location, v0, v1, v2, v3);
}
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    gl::glProgramUniform4iv(program, location, count, value);
}
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    gl::glProgramUniform4ui(program, location, v0, v1, v2, v3);
}
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    gl::glProgramUniform4uiv(program, location, count, value);
}
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix2x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix2x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix3x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix3x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix4x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glProgramUniformMatrix4x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
}
void glProvokingVertex(GLenum mode) {
    gl::glProvokingVertex(mode);
}
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message) {
    gl::glPushDebugGroup(source, id, length, message);
}
void glQueryCounter(GLuint id, GLenum target) {
    gl::glQueryCounter(id, target);
}
void glReadBuffer(GLenum src) {
    gl::glReadBuffer(src);
}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    gl::glReadPixels(x, y, width, height, format, type, pixels);
}
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) {
    gl::glReadnPixels(x, y, width, height, format, type, bufSize, data);
}
void glReleaseShaderCompiler(void) {
    gl::glReleaseShaderCompiler();
}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glRenderbufferStorage(target, internalformat, width, height);
}
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}
void glResumeTransformFeedback(void) {
    gl::glResumeTransformFeedback();
}
void glSampleCoverage(GLfloat value, GLboolean invert) {
    gl::glSampleCoverage(value, invert);
}
void glSampleMaski(GLuint maskNumber, GLbitfield mask) {
    gl::glSampleMaski(maskNumber, mask);
}
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) {
    gl::glSamplerParameterIiv(sampler, pname, param);
}
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) {
    gl::glSamplerParameterIuiv(sampler, pname, param);
}
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    gl::glSamplerParameterf(sampler, pname, param);
}
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) {
    gl::glSamplerParameterfv(sampler, pname, param);
}
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    gl::glSamplerParameteri(sampler, pname, param);
}
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) {
    gl::glSamplerParameteriv(sampler, pname, param);
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glScissor(x, y, width, height);
}
void glScissorArrayv(GLuint first, GLsizei count, const GLint *v) {
    gl::glScissorArrayv(first, count, v);
}
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
    gl::glScissorIndexed(index, left, bottom, width, height);
}
void glScissorIndexedv(GLuint index, const GLint *v) {
    gl::glScissorIndexedv(index, v);
}
void glSecondaryColorP3ui(GLenum type, GLuint color) {
    gl::glSecondaryColorP3ui(type, color);
}
void glSecondaryColorP3uiv(GLenum type, const GLuint *color) {
    gl::glSecondaryColorP3uiv(type, color);
}
void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length) {
    gl::glShaderBinary(count, shaders, binaryFormat, binary, length);
}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    gl::glShaderSource(shader, count, string, length);
}
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    gl::glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding);
}
void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue) {
    gl::glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    gl::glStencilFunc(func, ref, mask);
}
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    gl::glStencilFuncSeparate(face, func, ref, mask);
}
void glStencilMask(GLuint mask) {
    gl::glStencilMask(mask);
}
void glStencilMaskSeparate(GLenum face, GLuint mask) {
    gl::glStencilMaskSeparate(face, mask);
}
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    gl::glStencilOp(fail, zfail, zpass);
}
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    gl::glStencilOpSeparate(face, sfail, dpfail, dppass);
}
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
    gl::glTexBuffer(target, internalformat, buffer);
}
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    gl::glTexBufferRange(target, internalformat, buffer, offset, size);
}
void glTexCoordP1ui(GLenum type, GLuint coords) {
    gl::glTexCoordP1ui(type, coords);
}
void glTexCoordP1uiv(GLenum type, const GLuint *coords) {
    gl::glTexCoordP1uiv(type, coords);
}
void glTexCoordP2ui(GLenum type, GLuint coords) {
    gl::glTexCoordP2ui(type, coords);
}
void glTexCoordP2uiv(GLenum type, const GLuint *coords) {
    gl::glTexCoordP2uiv(type, coords);
}
void glTexCoordP3ui(GLenum type, GLuint coords) {
    gl::glTexCoordP3ui(type, coords);
}
void glTexCoordP3uiv(GLenum type, const GLuint *coords) {
    gl::glTexCoordP3uiv(type, coords);
}
void glTexCoordP4ui(GLenum type, GLuint coords) {
    gl::glTexCoordP4ui(type, coords);
}
void glTexCoordP4uiv(GLenum type, const GLuint *coords) {
    gl::glTexCoordP4uiv(type, coords);
}
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) {
    gl::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    gl::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    gl::glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {
    gl::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    gl::glTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) {
    gl::glTexParameterIiv(target, pname, params);
}
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) {
    gl::glTexParameterIuiv(target, pname, params);
}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    gl::glTexParameterf(target, pname, param);
}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    gl::glTexParameterfv(target, pname, params);
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    gl::glTexParameteri(target, pname, param);
}
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    gl::glTexParameteriv(target, pname, params);
}
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
    gl::glTexStorage1D(target, levels, internalformat, width);
}
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glTexStorage2D(target, levels, internalformat, width, height);
}
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    gl::glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    gl::glTexStorage3D(target, levels, internalformat, width, height, depth);
}
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    gl::glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    gl::glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    gl::glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    gl::glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureBarrier(void) {
    gl::glTextureBarrier();
}
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {
    gl::glTextureBuffer(texture, internalformat, buffer);
}
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    gl::glTextureBufferRange(texture, internalformat, buffer, offset, size);
}
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params) {
    gl::glTextureParameterIiv(texture, pname, params);
}
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params) {
    gl::glTextureParameterIuiv(texture, pname, params);
}
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
    gl::glTextureParameterf(texture, pname, param);
}
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param) {
    gl::glTextureParameterfv(texture, pname, param);
}
void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
    gl::glTextureParameteri(texture, pname, param);
}
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param) {
    gl::glTextureParameteriv(texture, pname, param);
}
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    gl::glTextureStorage1D(texture, levels, internalformat, width);
}
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    gl::glTextureStorage2D(texture, levels, internalformat, width, height);
}
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    gl::glTextureStorage2DMultisample(texture, samples, internalformat, width, height, fixedsamplelocations);
}
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    gl::glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    gl::glTextureStorage3DMultisample(texture, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    gl::glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    gl::glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    gl::glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
    gl::glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
}
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {
    gl::glTransformFeedbackBufferBase(xfb, index, buffer);
}
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    gl::glTransformFeedbackBufferRange(xfb, index, buffer, offset, size);
}
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) {
    gl::glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}
void glUniform1d(GLint location, GLdouble x) {
    gl::glUniform1d(location, x);
}
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value) {
    gl::glUniform1dv(location, count, value);
}
void glUniform1f(GLint location, GLfloat v0) {
    gl::glUniform1f(location, v0);
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    gl::glUniform1fv(location, count, value);
}
void glUniform1i(GLint location, GLint v0) {
    gl::glUniform1i(location, v0);
}
void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
    gl::glUniform1iv(location, count, value);
}
void glUniform1ui(GLint location, GLuint v0) {
    gl::glUniform1ui(location, v0);
}
void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) {
    gl::glUniform1uiv(location, count, value);
}
void glUniform2d(GLint location, GLdouble x, GLdouble y) {
    gl::glUniform2d(location, x, y);
}
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value) {
    gl::glUniform2dv(location, count, value);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    gl::glUniform2f(location, v0, v1);
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    gl::glUniform2fv(location, count, value);
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
    gl::glUniform2i(location, v0, v1);
}
void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
    gl::glUniform2iv(location, count, value);
}
void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    gl::glUniform2ui(location, v0, v1);
}
void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) {
    gl::glUniform2uiv(location, count, value);
}
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {
    gl::glUniform3d(location, x, y, z);
}
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value) {
    gl::glUniform3dv(location, count, value);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    gl::glUniform3f(location, v0, v1, v2);
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    gl::glUniform3fv(location, count, value);
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    gl::glUniform3i(location, v0, v1, v2);
}
void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
    gl::glUniform3iv(location, count, value);
}
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    gl::glUniform3ui(location, v0, v1, v2);
}
void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) {
    gl::glUniform3uiv(location, count, value);
}
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    gl::glUniform4d(location, x, y, z, w);
}
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value) {
    gl::glUniform4dv(location, count, value);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    gl::glUniform4f(location, v0, v1, v2, v3);
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    gl::glUniform4fv(location, count, value);
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    gl::glUniform4i(location, v0, v1, v2, v3);
}
void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
    gl::glUniform4iv(location, count, value);
}
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    gl::glUniform4ui(location, v0, v1, v2, v3);
}
void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) {
    gl::glUniform4uiv(location, count, value);
}
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    gl::glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix2dv(location, count, transpose, value);
}
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix2fv(location, count, transpose, value);
}
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix2x3dv(location, count, transpose, value);
}
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix2x3fv(location, count, transpose, value);
}
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix2x4dv(location, count, transpose, value);
}
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix2x4fv(location, count, transpose, value);
}
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix3dv(location, count, transpose, value);
}
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix3fv(location, count, transpose, value);
}
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix3x2dv(location, count, transpose, value);
}
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix3x2fv(location, count, transpose, value);
}
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix3x4dv(location, count, transpose, value);
}
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix3x4fv(location, count, transpose, value);
}
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix4dv(location, count, transpose, value);
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix4fv(location, count, transpose, value);
}
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix4x2dv(location, count, transpose, value);
}
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix4x2fv(location, count, transpose, value);
}
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    gl::glUniformMatrix4x3dv(location, count, transpose, value);
}
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    gl::glUniformMatrix4x3fv(location, count, transpose, value);
}
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices) {
    gl::glUniformSubroutinesuiv(shadertype, count, indices);
}
GLboolean glUnmapBuffer(GLenum target) {
    return gl::glUnmapBuffer(target);
}
GLboolean glUnmapNamedBuffer(GLuint buffer) {
    return gl::glUnmapNamedBuffer(buffer);
}
void glUseProgram(GLuint program) {
    gl::glUseProgram(program);
}
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
    gl::glUseProgramStages(pipeline, stages, program);
}
void glValidateProgram(GLuint program) {
    gl::glValidateProgram(program);
}
void glValidateProgramPipeline(GLuint pipeline) {
    gl::glValidateProgramPipeline(pipeline);
}
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
    gl::glVertexArrayAttribBinding(vaobj, attribindex, bindingindex);
}
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    gl::glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset);
}
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    gl::glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    gl::glVertexArrayAttribLFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {
    gl::glVertexArrayBindingDivisor(vaobj, bindingindex, divisor);
}
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
    gl::glVertexArrayElementBuffer(vaobj, buffer);
}
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    gl::glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride);
}
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    gl::glVertexArrayVertexBuffers(vaobj, first, count, buffers, offsets, strides);
}
void glVertexAttrib1d(GLuint index, GLdouble x) {
    gl::glVertexAttrib1d(index, x);
}
void glVertexAttrib1dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttrib1dv(index, v);
}
void glVertexAttrib1f(GLuint index, GLfloat x) {
    gl::glVertexAttrib1f(index, x);
}
void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
    gl::glVertexAttrib1fv(index, v);
}
void glVertexAttrib1s(GLuint index, GLshort x) {
    gl::glVertexAttrib1s(index, x);
}
void glVertexAttrib1sv(GLuint index, const GLshort *v) {
    gl::glVertexAttrib1sv(index, v);
}
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {
    gl::glVertexAttrib2d(index, x, y);
}
void glVertexAttrib2dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttrib2dv(index, v);
}
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    gl::glVertexAttrib2f(index, x, y);
}
void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
    gl::glVertexAttrib2fv(index, v);
}
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {
    gl::glVertexAttrib2s(index, x, y);
}
void glVertexAttrib2sv(GLuint index, const GLshort *v) {
    gl::glVertexAttrib2sv(index, v);
}
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    gl::glVertexAttrib3d(index, x, y, z);
}
void glVertexAttrib3dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttrib3dv(index, v);
}
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    gl::glVertexAttrib3f(index, x, y, z);
}
void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
    gl::glVertexAttrib3fv(index, v);
}
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {
    gl::glVertexAttrib3s(index, x, y, z);
}
void glVertexAttrib3sv(GLuint index, const GLshort *v) {
    gl::glVertexAttrib3sv(index, v);
}
void glVertexAttrib4Nbv(GLuint index, const GLbyte *v) {
    gl::glVertexAttrib4Nbv(index, v);
}
void glVertexAttrib4Niv(GLuint index, const GLint *v) {
    gl::glVertexAttrib4Niv(index, v);
}
void glVertexAttrib4Nsv(GLuint index, const GLshort *v) {
    gl::glVertexAttrib4Nsv(index, v);
}
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    gl::glVertexAttrib4Nub(index, x, y, z, w);
}
void glVertexAttrib4Nubv(GLuint index, const GLubyte *v) {
    gl::glVertexAttrib4Nubv(index, v);
}
void glVertexAttrib4Nuiv(GLuint index, const GLuint *v) {
    gl::glVertexAttrib4Nuiv(index, v);
}
void glVertexAttrib4Nusv(GLuint index, const GLushort *v) {
    gl::glVertexAttrib4Nusv(index, v);
}
void glVertexAttrib4bv(GLuint index, const GLbyte *v) {
    gl::glVertexAttrib4bv(index, v);
}
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    gl::glVertexAttrib4d(index, x, y, z, w);
}
void glVertexAttrib4dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttrib4dv(index, v);
}
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    gl::glVertexAttrib4f(index, x, y, z, w);
}
void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
    gl::glVertexAttrib4fv(index, v);
}
void glVertexAttrib4iv(GLuint index, const GLint *v) {
    gl::glVertexAttrib4iv(index, v);
}
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {
    gl::glVertexAttrib4s(index, x, y, z, w);
}
void glVertexAttrib4sv(GLuint index, const GLshort *v) {
    gl::glVertexAttrib4sv(index, v);
}
void glVertexAttrib4ubv(GLuint index, const GLubyte *v) {
    gl::glVertexAttrib4ubv(index, v);
}
void glVertexAttrib4uiv(GLuint index, const GLuint *v) {
    gl::glVertexAttrib4uiv(index, v);
}
void glVertexAttrib4usv(GLuint index, const GLushort *v) {
    gl::glVertexAttrib4usv(index, v);
}
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    gl::glVertexAttribBinding(attribindex, bindingindex);
}
void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    gl::glVertexAttribDivisor(index, divisor);
}
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    gl::glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
}
void glVertexAttribI1i(GLuint index, GLint x) {
    gl::glVertexAttribI1i(index, x);
}
void glVertexAttribI1iv(GLuint index, const GLint *v) {
    gl::glVertexAttribI1iv(index, v);
}
void glVertexAttribI1ui(GLuint index, GLuint x) {
    gl::glVertexAttribI1ui(index, x);
}
void glVertexAttribI1uiv(GLuint index, const GLuint *v) {
    gl::glVertexAttribI1uiv(index, v);
}
void glVertexAttribI2i(GLuint index, GLint x, GLint y) {
    gl::glVertexAttribI2i(index, x, y);
}
void glVertexAttribI2iv(GLuint index, const GLint *v) {
    gl::glVertexAttribI2iv(index, v);
}
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) {
    gl::glVertexAttribI2ui(index, x, y);
}
void glVertexAttribI2uiv(GLuint index, const GLuint *v) {
    gl::glVertexAttribI2uiv(index, v);
}
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) {
    gl::glVertexAttribI3i(index, x, y, z);
}
void glVertexAttribI3iv(GLuint index, const GLint *v) {
    gl::glVertexAttribI3iv(index, v);
}
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) {
    gl::glVertexAttribI3ui(index, x, y, z);
}
void glVertexAttribI3uiv(GLuint index, const GLuint *v) {
    gl::glVertexAttribI3uiv(index, v);
}
void glVertexAttribI4bv(GLuint index, const GLbyte *v) {
    gl::glVertexAttribI4bv(index, v);
}
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    gl::glVertexAttribI4i(index, x, y, z, w);
}
void glVertexAttribI4iv(GLuint index, const GLint *v) {
    gl::glVertexAttribI4iv(index, v);
}
void glVertexAttribI4sv(GLuint index, const GLshort *v) {
    gl::glVertexAttribI4sv(index, v);
}
void glVertexAttribI4ubv(GLuint index, const GLubyte *v) {
    gl::glVertexAttribI4ubv(index, v);
}
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    gl::glVertexAttribI4ui(index, x, y, z, w);
}
void glVertexAttribI4uiv(GLuint index, const GLuint *v) {
    gl::glVertexAttribI4uiv(index, v);
}
void glVertexAttribI4usv(GLuint index, const GLushort *v) {
    gl::glVertexAttribI4usv(index, v);
}
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    gl::glVertexAttribIFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    gl::glVertexAttribIPointer(index, size, type, stride, pointer);
}
void glVertexAttribL1d(GLuint index, GLdouble x) {
    gl::glVertexAttribL1d(index, x);
}
void glVertexAttribL1dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttribL1dv(index, v);
}
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {
    gl::glVertexAttribL2d(index, x, y);
}
void glVertexAttribL2dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttribL2dv(index, v);
}
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    gl::glVertexAttribL3d(index, x, y, z);
}
void glVertexAttribL3dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttribL3dv(index, v);
}
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    gl::glVertexAttribL4d(index, x, y, z, w);
}
void glVertexAttribL4dv(GLuint index, const GLdouble *v) {
    gl::glVertexAttribL4dv(index, v);
}
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    gl::glVertexAttribLFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    gl::glVertexAttribLPointer(index, size, type, stride, pointer);
}
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    gl::glVertexAttribP1ui(index, type, normalized, value);
}
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    gl::glVertexAttribP1uiv(index, type, normalized, value);
}
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    gl::glVertexAttribP2ui(index, type, normalized, value);
}
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    gl::glVertexAttribP2uiv(index, type, normalized, value);
}
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    gl::glVertexAttribP3ui(index, type, normalized, value);
}
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    gl::glVertexAttribP3uiv(index, type, normalized, value);
}
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    gl::glVertexAttribP4ui(index, type, normalized, value);
}
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    gl::glVertexAttribP4uiv(index, type, normalized, value);
}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    gl::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    gl::glVertexBindingDivisor(bindingindex, divisor);
}
void glVertexP2ui(GLenum type, GLuint value) {
    gl::glVertexP2ui(type, value);
}
void glVertexP2uiv(GLenum type, const GLuint *value) {
    gl::glVertexP2uiv(type, value);
}
void glVertexP3ui(GLenum type, GLuint value) {
    gl::glVertexP3ui(type, value);
}
void glVertexP3uiv(GLenum type, const GLuint *value) {
    gl::glVertexP3uiv(type, value);
}
void glVertexP4ui(GLenum type, GLuint value) {
    gl::glVertexP4ui(type, value);
}
void glVertexP4uiv(GLenum type, const GLuint *value) {
    gl::glVertexP4uiv(type, value);
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    gl::glViewport(x, y, width, height);
}
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v) {
    gl::glViewportArrayv(first, count, v);
}
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    gl::glViewportIndexedf(index, x, y, w, h);
}
void glViewportIndexedfv(GLuint index, const GLfloat *v) {
    gl::glViewportIndexedfv(index, v);
}
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    gl::glWaitSync(sync, flags, timeout);
}
void glLineStipple(GLint a, GLushort b) {
    gl::glLineStipple(a, b);
}
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) {
    gl::glDepthRangeArrayfvNV(a, b, c_);
}
void glGetPointerv(GLenum p, void** v) {
    gl::glGetPointerv(p, v);
}

#ifdef __cplusplus
} // extern "C"
#endif
