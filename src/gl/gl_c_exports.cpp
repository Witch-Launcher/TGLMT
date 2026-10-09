// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.
// extern "C" forward 1:1 sang tglmt::gl:: để LWJGL dlsym("glXxx") thấy.
// Mỗi wrapper đo thời gian gọi (tglmt::GlProfileScope) để log gltop#
// chỉ ra hàm gl* nào nuốt frame time.
#include "tglmt/gl46.h"
#include "tglmt/GlProfile.h"
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
    GlProfileScope _gp(0);
    gl::glActiveShaderProgram(pipeline, program);
}
void glActiveTexture(GLenum texture) {
    GlProfileScope _gp(1);
    gl::glActiveTexture(texture);
}
void glAttachShader(GLuint program, GLuint shader) {
    GlProfileScope _gp(2);
    gl::glAttachShader(program, shader);
}
void glBeginConditionalRender(GLuint id, GLenum mode) {
    GlProfileScope _gp(3);
    gl::glBeginConditionalRender(id, mode);
}
void glBeginQuery(GLenum target, GLuint id) {
    GlProfileScope _gp(4);
    gl::glBeginQuery(target, id);
}
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) {
    GlProfileScope _gp(5);
    gl::glBeginQueryIndexed(target, index, id);
}
void glBeginTransformFeedback(GLenum primitiveMode) {
    GlProfileScope _gp(6);
    gl::glBeginTransformFeedback(primitiveMode);
}
void glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
    GlProfileScope _gp(7);
    gl::glBindAttribLocation(program, index, name);
}
void glBindBuffer(GLenum target, GLuint buffer) {
    GlProfileScope _gp(8);
    gl::glBindBuffer(target, buffer);
}
void glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    GlProfileScope _gp(9);
    gl::glBindBufferBase(target, index, buffer);
}
void glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(10);
    gl::glBindBufferRange(target, index, buffer, offset, size);
}
void glBindBuffersBase(GLenum target, GLuint first, GLsizei count, const GLuint *buffers) {
    GlProfileScope _gp(11);
    gl::glBindBuffersBase(target, first, count, buffers);
}
void glBindBuffersRange(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes) {
    GlProfileScope _gp(12);
    gl::glBindBuffersRange(target, first, count, buffers, offsets, sizes);
}
void glBindFragDataLocation(GLuint program, GLuint color, const GLchar *name) {
    GlProfileScope _gp(13);
    gl::glBindFragDataLocation(program, color, name);
}
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name) {
    GlProfileScope _gp(14);
    gl::glBindFragDataLocationIndexed(program, colorNumber, index, name);
}
void glBindFramebuffer(GLenum target, GLuint framebuffer) {
    GlProfileScope _gp(15);
    gl::glBindFramebuffer(target, framebuffer);
}
void glBindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) {
    GlProfileScope _gp(16);
    gl::glBindImageTexture(unit, texture, level, layered, layer, access, format);
}
void glBindImageTextures(GLuint first, GLsizei count, const GLuint *textures) {
    GlProfileScope _gp(17);
    gl::glBindImageTextures(first, count, textures);
}
void glBindProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(18);
    gl::glBindProgramPipeline(pipeline);
}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
    GlProfileScope _gp(19);
    gl::glBindRenderbuffer(target, renderbuffer);
}
void glBindSampler(GLuint unit, GLuint sampler) {
    GlProfileScope _gp(20);
    gl::glBindSampler(unit, sampler);
}
void glBindSamplers(GLuint first, GLsizei count, const GLuint *samplers) {
    GlProfileScope _gp(21);
    gl::glBindSamplers(first, count, samplers);
}
void glBindTexture(GLenum target, GLuint texture) {
    GlProfileScope _gp(22);
    gl::glBindTexture(target, texture);
}
void glBindTextureUnit(GLuint unit, GLuint texture) {
    GlProfileScope _gp(23);
    gl::glBindTextureUnit(unit, texture);
}
void glBindTextures(GLuint first, GLsizei count, const GLuint *textures) {
    GlProfileScope _gp(24);
    gl::glBindTextures(first, count, textures);
}
void glBindTransformFeedback(GLenum target, GLuint id) {
    GlProfileScope _gp(25);
    gl::glBindTransformFeedback(target, id);
}
void glBindVertexArray(GLuint array) {
    GlProfileScope _gp(26);
    gl::glBindVertexArray(array);
}
void glBindVertexBuffer(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    GlProfileScope _gp(27);
    gl::glBindVertexBuffer(bindingindex, buffer, offset, stride);
}
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    GlProfileScope _gp(28);
    gl::glBindVertexBuffers(first, count, buffers, offsets, strides);
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(29);
    gl::glBlendColor(red, green, blue, alpha);
}
void glBlendEquation(GLenum mode) {
    GlProfileScope _gp(30);
    gl::glBlendEquation(mode);
}
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
    GlProfileScope _gp(31);
    gl::glBlendEquationSeparate(modeRGB, modeAlpha);
}
void glBlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
    GlProfileScope _gp(32);
    gl::glBlendEquationSeparatei(buf, modeRGB, modeAlpha);
}
void glBlendEquationi(GLuint buf, GLenum mode) {
    GlProfileScope _gp(33);
    gl::glBlendEquationi(buf, mode);
}
void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GlProfileScope _gp(34);
    gl::glBlendFunc(sfactor, dfactor);
}
void glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
    GlProfileScope _gp(35);
    gl::glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
}
void glBlendFuncSeparatei(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    GlProfileScope _gp(36);
    gl::glBlendFuncSeparatei(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}
void glBlendFunci(GLuint buf, GLenum src, GLenum dst) {
    GlProfileScope _gp(37);
    gl::glBlendFunci(buf, src, dst);
}
void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    GlProfileScope _gp(38);
    gl::glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    GlProfileScope _gp(39);
    gl::glBlitNamedFramebuffer(readFramebuffer, drawFramebuffer, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
void glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
    GlProfileScope _gp(40);
    gl::glBufferData(target, size, data, usage);
}
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {
    GlProfileScope _gp(41);
    gl::glBufferStorage(target, size, data, flags);
}
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
    GlProfileScope _gp(42);
    gl::glBufferSubData(target, offset, size, data);
}
GLenum glCheckFramebufferStatus(GLenum target) {
    GlProfileScope _gp(43);
    return gl::glCheckFramebufferStatus(target);
}
GLenum glCheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    GlProfileScope _gp(44);
    return gl::glCheckNamedFramebufferStatus(framebuffer, target);
}
void glClampColor(GLenum target, GLenum clamp) {
    GlProfileScope _gp(45);
    gl::glClampColor(target, clamp);
}
void glClear(GLbitfield mask) {
    GlProfileScope _gp(46);
    gl::glClear(mask);
}
void glClearBufferData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(47);
    gl::glClearBufferData(target, internalformat, format, type, data);
}
void glClearBufferSubData(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(48);
    gl::glClearBufferSubData(target, internalformat, offset, size, format, type, data);
}
void glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GlProfileScope _gp(49);
    gl::glClearBufferfi(buffer, drawbuffer, depth, stencil);
}
void glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GlProfileScope _gp(50);
    gl::glClearBufferfv(buffer, drawbuffer, value);
}
void glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) {
    GlProfileScope _gp(51);
    gl::glClearBufferiv(buffer, drawbuffer, value);
}
void glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GlProfileScope _gp(52);
    gl::glClearBufferuiv(buffer, drawbuffer, value);
}
void glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GlProfileScope _gp(53);
    gl::glClearColor(red, green, blue, alpha);
}
void glClearDepth(GLdouble depth) {
    GlProfileScope _gp(54);
    gl::glClearDepth(depth);
}
void glClearDepthf(GLfloat d) {
    GlProfileScope _gp(55);
    gl::glClearDepthf(d);
}
void glClearNamedBufferData(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(56);
    gl::glClearNamedBufferData(buffer, internalformat, format, type, data);
}
void glClearNamedBufferSubData(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(57);
    gl::glClearNamedBufferSubData(buffer, internalformat, offset, size, format, type, data);
}
void glClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
    GlProfileScope _gp(58);
    gl::glClearNamedFramebufferfi(framebuffer, buffer, drawbuffer, depth, stencil);
}
void glClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value) {
    GlProfileScope _gp(59);
    gl::glClearNamedFramebufferfv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value) {
    GlProfileScope _gp(60);
    gl::glClearNamedFramebufferiv(framebuffer, buffer, drawbuffer, value);
}
void glClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value) {
    GlProfileScope _gp(61);
    gl::glClearNamedFramebufferuiv(framebuffer, buffer, drawbuffer, value);
}
void glClearStencil(GLint s) {
    GlProfileScope _gp(62);
    gl::glClearStencil(s);
}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(63);
    gl::glClearTexImage(texture, level, format, type, data);
}
void glClearTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data) {
    GlProfileScope _gp(64);
    gl::glClearTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
}
GLenum glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    GlProfileScope _gp(65);
    return gl::glClientWaitSync(sync, flags, timeout);
}
void glClipControl(GLenum origin, GLenum depth) {
    GlProfileScope _gp(66);
    gl::glClipControl(origin, depth);
}
void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GlProfileScope _gp(67);
    gl::glColorMask(red, green, blue, alpha);
}
void glColorMaski(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    GlProfileScope _gp(68);
    gl::glColorMaski(index, r, g, b, a);
}
void glColorP3ui(GLenum type, GLuint color) {
    GlProfileScope _gp(69);
    gl::glColorP3ui(type, color);
}
void glColorP3uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(70);
    gl::glColorP3uiv(type, color);
}
void glColorP4ui(GLenum type, GLuint color) {
    GlProfileScope _gp(71);
    gl::glColorP4ui(type, color);
}
void glColorP4uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(72);
    gl::glColorP4uiv(type, color);
}
void glCompileShader(GLuint shader) {
    GlProfileScope _gp(73);
    gl::glCompileShader(shader);
}
void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(74);
    gl::glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, data);
}
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(75);
    gl::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}
void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(76);
    gl::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
}
void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(77);
    gl::glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
}
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(78);
    gl::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(79);
    gl::glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCompressedTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(80);
    gl::glCompressedTextureSubImage1D(texture, level, xoffset, width, format, imageSize, data);
}
void glCompressedTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(81);
    gl::glCompressedTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, imageSize, data);
}
void glCompressedTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
    GlProfileScope _gp(82);
    gl::glCompressedTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}
void glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    GlProfileScope _gp(83);
    gl::glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
    GlProfileScope _gp(84);
    gl::glCopyImageSubData(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
}
void glCopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
    GlProfileScope _gp(85);
    gl::glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    GlProfileScope _gp(86);
    gl::glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GlProfileScope _gp(87);
    gl::glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GlProfileScope _gp(88);
    gl::glCopyTexSubImage1D(target, level, xoffset, x, y, width);
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(89);
    gl::glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(90);
    gl::glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCopyTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GlProfileScope _gp(91);
    gl::glCopyTextureSubImage1D(texture, level, xoffset, x, y, width);
}
void glCopyTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(92);
    gl::glCopyTextureSubImage2D(texture, level, xoffset, yoffset, x, y, width, height);
}
void glCopyTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(93);
    gl::glCopyTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, x, y, width, height);
}
void glCreateBuffers(GLsizei n, GLuint *buffers) {
    GlProfileScope _gp(94);
    gl::glCreateBuffers(n, buffers);
}
void glCreateFramebuffers(GLsizei n, GLuint *framebuffers) {
    GlProfileScope _gp(95);
    gl::glCreateFramebuffers(n, framebuffers);
}
GLuint glCreateProgram(void) {
    GlProfileScope _gp(96);
    return gl::glCreateProgram();
}
void glCreateProgramPipelines(GLsizei n, GLuint *pipelines) {
    GlProfileScope _gp(97);
    gl::glCreateProgramPipelines(n, pipelines);
}
void glCreateQueries(GLenum target, GLsizei n, GLuint *ids) {
    GlProfileScope _gp(98);
    gl::glCreateQueries(target, n, ids);
}
void glCreateRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GlProfileScope _gp(99);
    gl::glCreateRenderbuffers(n, renderbuffers);
}
void glCreateSamplers(GLsizei n, GLuint *samplers) {
    GlProfileScope _gp(100);
    gl::glCreateSamplers(n, samplers);
}
GLuint glCreateShader(GLenum type) {
    GlProfileScope _gp(101);
    return gl::glCreateShader(type);
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei count, const GLchar *const*strings) {
    GlProfileScope _gp(102);
    return gl::glCreateShaderProgramv(type, count, strings);
}
void glCreateTextures(GLenum target, GLsizei n, GLuint *textures) {
    GlProfileScope _gp(103);
    gl::glCreateTextures(target, n, textures);
}
void glCreateTransformFeedbacks(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(104);
    gl::glCreateTransformFeedbacks(n, ids);
}
void glCreateVertexArrays(GLsizei n, GLuint *arrays) {
    GlProfileScope _gp(105);
    gl::glCreateVertexArrays(n, arrays);
}
void glCullFace(GLenum mode) {
    GlProfileScope _gp(106);
    gl::glCullFace(mode);
}
void glDebugMessageCallback(GLDEBUGPROC callback, const void *userParam) {
    GlProfileScope _gp(107);
    gl::glDebugMessageCallback(callback, userParam);
}
void glDebugMessageControl(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) {
    GlProfileScope _gp(108);
    gl::glDebugMessageControl(source, type, severity, count, ids, enabled);
}
void glDebugMessageInsert(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) {
    GlProfileScope _gp(109);
    gl::glDebugMessageInsert(source, type, id, severity, length, buf);
}
void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
    GlProfileScope _gp(110);
    gl::glDeleteBuffers(n, buffers);
}
void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
    GlProfileScope _gp(111);
    gl::glDeleteFramebuffers(n, framebuffers);
}
void glDeleteProgram(GLuint program) {
    GlProfileScope _gp(112);
    gl::glDeleteProgram(program);
}
void glDeleteProgramPipelines(GLsizei n, const GLuint *pipelines) {
    GlProfileScope _gp(113);
    gl::glDeleteProgramPipelines(n, pipelines);
}
void glDeleteQueries(GLsizei n, const GLuint *ids) {
    GlProfileScope _gp(114);
    gl::glDeleteQueries(n, ids);
}
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
    GlProfileScope _gp(115);
    gl::glDeleteRenderbuffers(n, renderbuffers);
}
void glDeleteSamplers(GLsizei count, const GLuint *samplers) {
    GlProfileScope _gp(116);
    gl::glDeleteSamplers(count, samplers);
}
void glDeleteShader(GLuint shader) {
    GlProfileScope _gp(117);
    gl::glDeleteShader(shader);
}
void glDeleteSync(GLsync sync) {
    GlProfileScope _gp(118);
    gl::glDeleteSync(sync);
}
void glDeleteTextures(GLsizei n, const GLuint *textures) {
    GlProfileScope _gp(119);
    gl::glDeleteTextures(n, textures);
}
void glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids) {
    GlProfileScope _gp(120);
    gl::glDeleteTransformFeedbacks(n, ids);
}
void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
    GlProfileScope _gp(121);
    gl::glDeleteVertexArrays(n, arrays);
}
void glDepthFunc(GLenum func) {
    GlProfileScope _gp(122);
    gl::glDepthFunc(func);
}
void glDepthMask(GLboolean flag) {
    GlProfileScope _gp(123);
    gl::glDepthMask(flag);
}
void glDepthRange(GLdouble n, GLdouble f) {
    GlProfileScope _gp(124);
    gl::glDepthRange(n, f);
}
void glDepthRangeArrayv(GLuint first, GLsizei count, const GLdouble *v) {
    GlProfileScope _gp(125);
    gl::glDepthRangeArrayv(first, count, v);
}
void glDepthRangeIndexed(GLuint index, GLdouble n, GLdouble f) {
    GlProfileScope _gp(126);
    gl::glDepthRangeIndexed(index, n, f);
}
void glDepthRangef(GLfloat n, GLfloat f) {
    GlProfileScope _gp(127);
    gl::glDepthRangef(n, f);
}
void glDetachShader(GLuint program, GLuint shader) {
    GlProfileScope _gp(128);
    gl::glDetachShader(program, shader);
}
void glDisable(GLenum cap) {
    GlProfileScope _gp(129);
    gl::glDisable(cap);
}
void glDisableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    GlProfileScope _gp(130);
    gl::glDisableVertexArrayAttrib(vaobj, index);
}
void glDisableVertexAttribArray(GLuint index) {
    GlProfileScope _gp(131);
    gl::glDisableVertexAttribArray(index);
}
void glDisablei(GLenum target, GLuint index) {
    GlProfileScope _gp(132);
    gl::glDisablei(target, index);
}
void glDispatchCompute(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z) {
    GlProfileScope _gp(133);
    gl::glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
}
void glDispatchComputeIndirect(GLintptr indirect) {
    GlProfileScope _gp(134);
    gl::glDispatchComputeIndirect(indirect);
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    GlProfileScope _gp(135);
    gl::glDrawArrays(mode, first, count);
}
void glDrawArraysIndirect(GLenum mode, const void *indirect) {
    GlProfileScope _gp(136);
    gl::glDrawArraysIndirect(mode, indirect);
}
void glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
    GlProfileScope _gp(137);
    gl::glDrawArraysInstanced(mode, first, count, instancecount);
}
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
    GlProfileScope _gp(138);
    gl::glDrawArraysInstancedBaseInstance(mode, first, count, instancecount, baseinstance);
}
void glDrawBuffer(GLenum buf) {
    GlProfileScope _gp(139);
    gl::glDrawBuffer(buf);
}
void glDrawBuffers(GLsizei n, const GLenum *bufs) {
    GlProfileScope _gp(140);
    gl::glDrawBuffers(n, bufs);
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    GlProfileScope _gp(141);
    gl::glDrawElements(mode, count, type, indices);
}
void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GlProfileScope _gp(142);
    gl::glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}
void glDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {
    GlProfileScope _gp(143);
    gl::glDrawElementsIndirect(mode, type, indirect);
}
void glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
    GlProfileScope _gp(144);
    gl::glDrawElementsInstanced(mode, count, type, indices, instancecount);
}
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance) {
    GlProfileScope _gp(145);
    gl::glDrawElementsInstancedBaseInstance(mode, count, type, indices, instancecount, baseinstance);
}
void glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
    GlProfileScope _gp(146);
    gl::glDrawElementsInstancedBaseVertex(mode, count, type, indices, instancecount, basevertex);
}
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
    GlProfileScope _gp(147);
    gl::glDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instancecount, basevertex, baseinstance);
}
void glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
    GlProfileScope _gp(148);
    gl::glDrawRangeElements(mode, start, end, count, type, indices);
}
void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
    GlProfileScope _gp(149);
    gl::glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}
void glDrawTransformFeedback(GLenum mode, GLuint id) {
    GlProfileScope _gp(150);
    gl::glDrawTransformFeedback(mode, id);
}
void glDrawTransformFeedbackInstanced(GLenum mode, GLuint id, GLsizei instancecount) {
    GlProfileScope _gp(151);
    gl::glDrawTransformFeedbackInstanced(mode, id, instancecount);
}
void glDrawTransformFeedbackStream(GLenum mode, GLuint id, GLuint stream) {
    GlProfileScope _gp(152);
    gl::glDrawTransformFeedbackStream(mode, id, stream);
}
void glDrawTransformFeedbackStreamInstanced(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount) {
    GlProfileScope _gp(153);
    gl::glDrawTransformFeedbackStreamInstanced(mode, id, stream, instancecount);
}
void glEnable(GLenum cap) {
    GlProfileScope _gp(154);
    gl::glEnable(cap);
}
void glEnableVertexArrayAttrib(GLuint vaobj, GLuint index) {
    GlProfileScope _gp(155);
    gl::glEnableVertexArrayAttrib(vaobj, index);
}
void glEnableVertexAttribArray(GLuint index) {
    GlProfileScope _gp(156);
    gl::glEnableVertexAttribArray(index);
}
void glEnablei(GLenum target, GLuint index) {
    GlProfileScope _gp(157);
    gl::glEnablei(target, index);
}
void glEndConditionalRender(void) {
    GlProfileScope _gp(158);
    gl::glEndConditionalRender();
}
void glEndQuery(GLenum target) {
    GlProfileScope _gp(159);
    gl::glEndQuery(target);
}
void glEndQueryIndexed(GLenum target, GLuint index) {
    GlProfileScope _gp(160);
    gl::glEndQueryIndexed(target, index);
}
void glEndTransformFeedback(void) {
    GlProfileScope _gp(161);
    gl::glEndTransformFeedback();
}
GLsync glFenceSync(GLenum condition, GLbitfield flags) {
    GlProfileScope _gp(162);
    return gl::glFenceSync(condition, flags);
}
void glFinish(void) {
    GlProfileScope _gp(163);
    gl::glFinish();
}
void glFlush(void) {
    GlProfileScope _gp(164);
    gl::glFlush();
}
void glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(165);
    gl::glFlushMappedBufferRange(target, offset, length);
}
void glFlushMappedNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(166);
    gl::glFlushMappedNamedBufferRange(buffer, offset, length);
}
void glFramebufferParameteri(GLenum target, GLenum pname, GLint param) {
    GlProfileScope _gp(167);
    gl::glFramebufferParameteri(target, pname, param);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GlProfileScope _gp(168);
    gl::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}
void glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level) {
    GlProfileScope _gp(169);
    gl::glFramebufferTexture(target, attachment, texture, level);
}
void glFramebufferTexture1D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GlProfileScope _gp(170);
    gl::glFramebufferTexture1D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GlProfileScope _gp(171);
    gl::glFramebufferTexture2D(target, attachment, textarget, texture, level);
}
void glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
    GlProfileScope _gp(172);
    gl::glFramebufferTexture3D(target, attachment, textarget, texture, level, zoffset);
}
void glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GlProfileScope _gp(173);
    gl::glFramebufferTextureLayer(target, attachment, texture, level, layer);
}
void glFrontFace(GLenum mode) {
    GlProfileScope _gp(174);
    gl::glFrontFace(mode);
}
void glGenBuffers(GLsizei n, GLuint *buffers) {
    GlProfileScope _gp(175);
    gl::glGenBuffers(n, buffers);
}
void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
    GlProfileScope _gp(176);
    gl::glGenFramebuffers(n, framebuffers);
}
void glGenProgramPipelines(GLsizei n, GLuint *pipelines) {
    GlProfileScope _gp(177);
    gl::glGenProgramPipelines(n, pipelines);
}
void glGenQueries(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(178);
    gl::glGenQueries(n, ids);
}
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
    GlProfileScope _gp(179);
    gl::glGenRenderbuffers(n, renderbuffers);
}
void glGenSamplers(GLsizei count, GLuint *samplers) {
    GlProfileScope _gp(180);
    gl::glGenSamplers(count, samplers);
}
void glGenTextures(GLsizei n, GLuint *textures) {
    GlProfileScope _gp(181);
    gl::glGenTextures(n, textures);
}
void glGenTransformFeedbacks(GLsizei n, GLuint *ids) {
    GlProfileScope _gp(182);
    gl::glGenTransformFeedbacks(n, ids);
}
void glGenVertexArrays(GLsizei n, GLuint *arrays) {
    GlProfileScope _gp(183);
    gl::glGenVertexArrays(n, arrays);
}
void glGenerateMipmap(GLenum target) {
    GlProfileScope _gp(184);
    gl::glGenerateMipmap(target);
}
void glGenerateTextureMipmap(GLuint texture) {
    GlProfileScope _gp(185);
    gl::glGenerateTextureMipmap(texture);
}
void glGetActiveAtomicCounterBufferiv(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params) {
    GlProfileScope _gp(186);
    gl::glGetActiveAtomicCounterBufferiv(program, bufferIndex, pname, params);
}
void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(187);
    gl::glGetActiveAttrib(program, index, bufSize, length, size, type, name);
}
void glGetActiveSubroutineName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(188);
    gl::glGetActiveSubroutineName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformName(GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(189);
    gl::glGetActiveSubroutineUniformName(program, shadertype, index, bufSize, length, name);
}
void glGetActiveSubroutineUniformiv(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values) {
    GlProfileScope _gp(190);
    gl::glGetActiveSubroutineUniformiv(program, shadertype, index, pname, values);
}
void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(191);
    gl::glGetActiveUniform(program, index, bufSize, length, size, type, name);
}
void glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) {
    GlProfileScope _gp(192);
    gl::glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
}
void glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) {
    GlProfileScope _gp(193);
    gl::glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
}
void glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName) {
    GlProfileScope _gp(194);
    gl::glGetActiveUniformName(program, uniformIndex, bufSize, length, uniformName);
}
void glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) {
    GlProfileScope _gp(195);
    gl::glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
}
void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
    GlProfileScope _gp(196);
    gl::glGetAttachedShaders(program, maxCount, count, shaders);
}
GLint glGetAttribLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(197);
    return gl::glGetAttribLocation(program, name);
}
void glGetBooleani_v(GLenum target, GLuint index, GLboolean *data) {
    GlProfileScope _gp(198);
    gl::glGetBooleani_v(target, index, data);
}
void glGetBooleanv(GLenum pname, GLboolean *data) {
    GlProfileScope _gp(199);
    gl::glGetBooleanv(pname, data);
}
void glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(200);
    gl::glGetBufferParameteri64v(target, pname, params);
}
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(201);
    gl::glGetBufferParameteriv(target, pname, params);
}
void glGetBufferPointerv(GLenum target, GLenum pname, void **params) {
    GlProfileScope _gp(202);
    gl::glGetBufferPointerv(target, pname, params);
}
void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data) {
    GlProfileScope _gp(203);
    gl::glGetBufferSubData(target, offset, size, data);
}
void glGetCompressedTexImage(GLenum target, GLint level, void *img) {
    GlProfileScope _gp(204);
    gl::glGetCompressedTexImage(target, level, img);
}
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(205);
    gl::glGetCompressedTextureImage(texture, level, bufSize, pixels);
}
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(206);
    gl::glGetCompressedTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, bufSize, pixels);
}
GLuint glGetDebugMessageLog(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) {
    GlProfileScope _gp(207);
    return gl::glGetDebugMessageLog(count, bufSize, sources, types, ids, severities, lengths, messageLog);
}
void glGetDoublei_v(GLenum target, GLuint index, GLdouble *data) {
    GlProfileScope _gp(208);
    gl::glGetDoublei_v(target, index, data);
}
void glGetDoublev(GLenum pname, GLdouble *data) {
    GlProfileScope _gp(209);
    gl::glGetDoublev(pname, data);
}
GLenum glGetError(void) {
    GlProfileScope _gp(210);
    return gl::glGetError();
}
void glGetFloati_v(GLenum target, GLuint index, GLfloat *data) {
    GlProfileScope _gp(211);
    gl::glGetFloati_v(target, index, data);
}
void glGetFloatv(GLenum pname, GLfloat *data) {
    GlProfileScope _gp(212);
    gl::glGetFloatv(pname, data);
}
GLint glGetFragDataIndex(GLuint program, const GLchar *name) {
    GlProfileScope _gp(213);
    return gl::glGetFragDataIndex(program, name);
}
GLint glGetFragDataLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(214);
    return gl::glGetFragDataLocation(program, name);
}
void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    GlProfileScope _gp(215);
    gl::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}
void glGetFramebufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(216);
    gl::glGetFramebufferParameteriv(target, pname, params);
}
GLenum glGetGraphicsResetStatus(void) {
    GlProfileScope _gp(217);
    return gl::glGetGraphicsResetStatus();
}
void glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) {
    GlProfileScope _gp(218);
    gl::glGetInteger64i_v(target, index, data);
}
void glGetInteger64v(GLenum pname, GLint64 *data) {
    GlProfileScope _gp(219);
    gl::glGetInteger64v(pname, data);
}
void glGetIntegeri_v(GLenum target, GLuint index, GLint *data) {
    GlProfileScope _gp(220);
    gl::glGetIntegeri_v(target, index, data);
}
void glGetIntegerv(GLenum pname, GLint *data) {
    GlProfileScope _gp(221);
    gl::glGetIntegerv(pname, data);
}
void glGetInternalformati64v(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params) {
    GlProfileScope _gp(222);
    gl::glGetInternalformati64v(target, internalformat, pname, count, params);
}
void glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params) {
    GlProfileScope _gp(223);
    gl::glGetInternalformativ(target, internalformat, pname, count, params);
}
void glGetMultisamplefv(GLenum pname, GLuint index, GLfloat *val) {
    GlProfileScope _gp(224);
    gl::glGetMultisamplefv(pname, index, val);
}
void glGetNamedBufferParameteri64v(GLuint buffer, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(225);
    gl::glGetNamedBufferParameteri64v(buffer, pname, params);
}
void glGetNamedBufferParameteriv(GLuint buffer, GLenum pname, GLint *params) {
    GlProfileScope _gp(226);
    gl::glGetNamedBufferParameteriv(buffer, pname, params);
}
void glGetNamedBufferPointerv(GLuint buffer, GLenum pname, void **params) {
    GlProfileScope _gp(227);
    gl::glGetNamedBufferPointerv(buffer, pname, params);
}
void glGetNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data) {
    GlProfileScope _gp(228);
    gl::glGetNamedBufferSubData(buffer, offset, size, data);
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params) {
    GlProfileScope _gp(229);
    gl::glGetNamedFramebufferAttachmentParameteriv(framebuffer, attachment, pname, params);
}
void glGetNamedFramebufferParameteriv(GLuint framebuffer, GLenum pname, GLint *param) {
    GlProfileScope _gp(230);
    gl::glGetNamedFramebufferParameteriv(framebuffer, pname, param);
}
void glGetNamedRenderbufferParameteriv(GLuint renderbuffer, GLenum pname, GLint *params) {
    GlProfileScope _gp(231);
    gl::glGetNamedRenderbufferParameteriv(renderbuffer, pname, params);
}
void glGetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label) {
    GlProfileScope _gp(232);
    gl::glGetObjectLabel(identifier, name, bufSize, length, label);
}
void glGetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label) {
    GlProfileScope _gp(233);
    gl::glGetObjectPtrLabel(ptr, bufSize, length, label);
}
void glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
    GlProfileScope _gp(234);
    gl::glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
}
void glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(235);
    gl::glGetProgramInfoLog(program, bufSize, length, infoLog);
}
void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint *params) {
    GlProfileScope _gp(236);
    gl::glGetProgramInterfaceiv(program, programInterface, pname, params);
}
void glGetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(237);
    gl::glGetProgramPipelineInfoLog(pipeline, bufSize, length, infoLog);
}
void glGetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params) {
    GlProfileScope _gp(238);
    gl::glGetProgramPipelineiv(pipeline, pname, params);
}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(239);
    return gl::glGetProgramResourceIndex(program, programInterface, name);
}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(240);
    return gl::glGetProgramResourceLocation(program, programInterface, name);
}
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar *name) {
    GlProfileScope _gp(241);
    return gl::glGetProgramResourceLocationIndex(program, programInterface, name);
}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) {
    GlProfileScope _gp(242);
    gl::glGetProgramResourceName(program, programInterface, index, bufSize, length, name);
}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params) {
    GlProfileScope _gp(243);
    gl::glGetProgramResourceiv(program, programInterface, index, propCount, props, count, length, params);
}
void glGetProgramStageiv(GLuint program, GLenum shadertype, GLenum pname, GLint *values) {
    GlProfileScope _gp(244);
    gl::glGetProgramStageiv(program, shadertype, pname, values);
}
void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
    GlProfileScope _gp(245);
    gl::glGetProgramiv(program, pname, params);
}
void glGetQueryBufferObjecti64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(246);
    gl::glGetQueryBufferObjecti64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(247);
    gl::glGetQueryBufferObjectiv(id, buffer, pname, offset);
}
void glGetQueryBufferObjectui64v(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(248);
    gl::glGetQueryBufferObjectui64v(id, buffer, pname, offset);
}
void glGetQueryBufferObjectuiv(GLuint id, GLuint buffer, GLenum pname, GLintptr offset) {
    GlProfileScope _gp(249);
    gl::glGetQueryBufferObjectuiv(id, buffer, pname, offset);
}
void glGetQueryIndexediv(GLenum target, GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(250);
    gl::glGetQueryIndexediv(target, index, pname, params);
}
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) {
    GlProfileScope _gp(251);
    gl::glGetQueryObjecti64v(id, pname, params);
}
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) {
    GlProfileScope _gp(252);
    gl::glGetQueryObjectiv(id, pname, params);
}
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) {
    GlProfileScope _gp(253);
    gl::glGetQueryObjectui64v(id, pname, params);
}
void glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {
    GlProfileScope _gp(254);
    gl::glGetQueryObjectuiv(id, pname, params);
}
void glGetQueryiv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(255);
    gl::glGetQueryiv(target, pname, params);
}
void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(256);
    gl::glGetRenderbufferParameteriv(target, pname, params);
}
void glGetSamplerParameterIiv(GLuint sampler, GLenum pname, GLint *params) {
    GlProfileScope _gp(257);
    gl::glGetSamplerParameterIiv(sampler, pname, params);
}
void glGetSamplerParameterIuiv(GLuint sampler, GLenum pname, GLuint *params) {
    GlProfileScope _gp(258);
    gl::glGetSamplerParameterIuiv(sampler, pname, params);
}
void glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(259);
    gl::glGetSamplerParameterfv(sampler, pname, params);
}
void glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params) {
    GlProfileScope _gp(260);
    gl::glGetSamplerParameteriv(sampler, pname, params);
}
void glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
    GlProfileScope _gp(261);
    gl::glGetShaderInfoLog(shader, bufSize, length, infoLog);
}
void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
    GlProfileScope _gp(262);
    gl::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
}
void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
    GlProfileScope _gp(263);
    gl::glGetShaderSource(shader, bufSize, length, source);
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
    GlProfileScope _gp(264);
    gl::glGetShaderiv(shader, pname, params);
}
const GLubyte * glGetString(GLenum name) {
    GlProfileScope _gp(265);
    return gl::glGetString(name);
}
const GLubyte * glGetStringi(GLenum name, GLuint index) {
    GlProfileScope _gp(266);
    return gl::glGetStringi(name, index);
}
GLuint glGetSubroutineIndex(GLuint program, GLenum shadertype, const GLchar *name) {
    GlProfileScope _gp(267);
    return gl::glGetSubroutineIndex(program, shadertype, name);
}
GLint glGetSubroutineUniformLocation(GLuint program, GLenum shadertype, const GLchar *name) {
    GlProfileScope _gp(268);
    return gl::glGetSubroutineUniformLocation(program, shadertype, name);
}
void glGetSynciv(GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values) {
    GlProfileScope _gp(269);
    gl::glGetSynciv(sync, pname, count, length, values);
}
void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels) {
    GlProfileScope _gp(270);
    gl::glGetTexImage(target, level, format, type, pixels);
}
void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(271);
    gl::glGetTexLevelParameterfv(target, level, pname, params);
}
void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    GlProfileScope _gp(272);
    gl::glGetTexLevelParameteriv(target, level, pname, params);
}
void glGetTexParameterIiv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(273);
    gl::glGetTexParameterIiv(target, pname, params);
}
void glGetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params) {
    GlProfileScope _gp(274);
    gl::glGetTexParameterIuiv(target, pname, params);
}
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(275);
    gl::glGetTexParameterfv(target, pname, params);
}
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    GlProfileScope _gp(276);
    gl::glGetTexParameteriv(target, pname, params);
}
void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(277);
    gl::glGetTextureImage(texture, level, format, type, bufSize, pixels);
}
void glGetTextureLevelParameterfv(GLuint texture, GLint level, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(278);
    gl::glGetTextureLevelParameterfv(texture, level, pname, params);
}
void glGetTextureLevelParameteriv(GLuint texture, GLint level, GLenum pname, GLint *params) {
    GlProfileScope _gp(279);
    gl::glGetTextureLevelParameteriv(texture, level, pname, params);
}
void glGetTextureParameterIiv(GLuint texture, GLenum pname, GLint *params) {
    GlProfileScope _gp(280);
    gl::glGetTextureParameterIiv(texture, pname, params);
}
void glGetTextureParameterIuiv(GLuint texture, GLenum pname, GLuint *params) {
    GlProfileScope _gp(281);
    gl::glGetTextureParameterIuiv(texture, pname, params);
}
void glGetTextureParameterfv(GLuint texture, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(282);
    gl::glGetTextureParameterfv(texture, pname, params);
}
void glGetTextureParameteriv(GLuint texture, GLenum pname, GLint *params) {
    GlProfileScope _gp(283);
    gl::glGetTextureParameteriv(texture, pname, params);
}
void glGetTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(284);
    gl::glGetTextureSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, bufSize, pixels);
}
void glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) {
    GlProfileScope _gp(285);
    gl::glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
}
void glGetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index, GLint64 *param) {
    GlProfileScope _gp(286);
    gl::glGetTransformFeedbacki64_v(xfb, pname, index, param);
}
void glGetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index, GLint *param) {
    GlProfileScope _gp(287);
    gl::glGetTransformFeedbacki_v(xfb, pname, index, param);
}
void glGetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param) {
    GlProfileScope _gp(288);
    gl::glGetTransformFeedbackiv(xfb, pname, param);
}
GLuint glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) {
    GlProfileScope _gp(289);
    return gl::glGetUniformBlockIndex(program, uniformBlockName);
}
void glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) {
    GlProfileScope _gp(290);
    gl::glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
}
GLint glGetUniformLocation(GLuint program, const GLchar *name) {
    GlProfileScope _gp(291);
    return gl::glGetUniformLocation(program, name);
}
void glGetUniformSubroutineuiv(GLenum shadertype, GLint location, GLuint *params) {
    GlProfileScope _gp(292);
    gl::glGetUniformSubroutineuiv(shadertype, location, params);
}
void glGetUniformdv(GLuint program, GLint location, GLdouble *params) {
    GlProfileScope _gp(293);
    gl::glGetUniformdv(program, location, params);
}
void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
    GlProfileScope _gp(294);
    gl::glGetUniformfv(program, location, params);
}
void glGetUniformiv(GLuint program, GLint location, GLint *params) {
    GlProfileScope _gp(295);
    gl::glGetUniformiv(program, location, params);
}
void glGetUniformuiv(GLuint program, GLint location, GLuint *params) {
    GlProfileScope _gp(296);
    gl::glGetUniformuiv(program, location, params);
}
void glGetVertexArrayIndexed64iv(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param) {
    GlProfileScope _gp(297);
    gl::glGetVertexArrayIndexed64iv(vaobj, index, pname, param);
}
void glGetVertexArrayIndexediv(GLuint vaobj, GLuint index, GLenum pname, GLint *param) {
    GlProfileScope _gp(298);
    gl::glGetVertexArrayIndexediv(vaobj, index, pname, param);
}
void glGetVertexArrayiv(GLuint vaobj, GLenum pname, GLint *param) {
    GlProfileScope _gp(299);
    gl::glGetVertexArrayiv(vaobj, pname, param);
}
void glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(300);
    gl::glGetVertexAttribIiv(index, pname, params);
}
void glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params) {
    GlProfileScope _gp(301);
    gl::glGetVertexAttribIuiv(index, pname, params);
}
void glGetVertexAttribLdv(GLuint index, GLenum pname, GLdouble *params) {
    GlProfileScope _gp(302);
    gl::glGetVertexAttribLdv(index, pname, params);
}
void glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
    GlProfileScope _gp(303);
    gl::glGetVertexAttribPointerv(index, pname, pointer);
}
void glGetVertexAttribdv(GLuint index, GLenum pname, GLdouble *params) {
    GlProfileScope _gp(304);
    gl::glGetVertexAttribdv(index, pname, params);
}
void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
    GlProfileScope _gp(305);
    gl::glGetVertexAttribfv(index, pname, params);
}
void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
    GlProfileScope _gp(306);
    gl::glGetVertexAttribiv(index, pname, params);
}
void glGetnColorTable(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table) {
    GlProfileScope _gp(307);
    gl::glGetnColorTable(target, format, type, bufSize, table);
}
void glGetnCompressedTexImage(GLenum target, GLint lod, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(308);
    gl::glGetnCompressedTexImage(target, lod, bufSize, pixels);
}
void glGetnConvolutionFilter(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image) {
    GlProfileScope _gp(309);
    gl::glGetnConvolutionFilter(target, format, type, bufSize, image);
}
void glGetnHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    GlProfileScope _gp(310);
    gl::glGetnHistogram(target, reset, format, type, bufSize, values);
}
void glGetnMapdv(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v) {
    GlProfileScope _gp(311);
    gl::glGetnMapdv(target, query, bufSize, v);
}
void glGetnMapfv(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v) {
    GlProfileScope _gp(312);
    gl::glGetnMapfv(target, query, bufSize, v);
}
void glGetnMapiv(GLenum target, GLenum query, GLsizei bufSize, GLint *v) {
    GlProfileScope _gp(313);
    gl::glGetnMapiv(target, query, bufSize, v);
}
void glGetnMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values) {
    GlProfileScope _gp(314);
    gl::glGetnMinmax(target, reset, format, type, bufSize, values);
}
void glGetnPixelMapfv(GLenum map, GLsizei bufSize, GLfloat *values) {
    GlProfileScope _gp(315);
    gl::glGetnPixelMapfv(map, bufSize, values);
}
void glGetnPixelMapuiv(GLenum map, GLsizei bufSize, GLuint *values) {
    GlProfileScope _gp(316);
    gl::glGetnPixelMapuiv(map, bufSize, values);
}
void glGetnPixelMapusv(GLenum map, GLsizei bufSize, GLushort *values) {
    GlProfileScope _gp(317);
    gl::glGetnPixelMapusv(map, bufSize, values);
}
void glGetnPolygonStipple(GLsizei bufSize, GLubyte *pattern) {
    GlProfileScope _gp(318);
    gl::glGetnPolygonStipple(bufSize, pattern);
}
void glGetnSeparableFilter(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span) {
    GlProfileScope _gp(319);
    gl::glGetnSeparableFilter(target, format, type, rowBufSize, row, columnBufSize, column, span);
}
void glGetnTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels) {
    GlProfileScope _gp(320);
    gl::glGetnTexImage(target, level, format, type, bufSize, pixels);
}
void glGetnUniformdv(GLuint program, GLint location, GLsizei bufSize, GLdouble *params) {
    GlProfileScope _gp(321);
    gl::glGetnUniformdv(program, location, bufSize, params);
}
void glGetnUniformfv(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) {
    GlProfileScope _gp(322);
    gl::glGetnUniformfv(program, location, bufSize, params);
}
void glGetnUniformiv(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
    GlProfileScope _gp(323);
    gl::glGetnUniformiv(program, location, bufSize, params);
}
void glGetnUniformuiv(GLuint program, GLint location, GLsizei bufSize, GLuint *params) {
    GlProfileScope _gp(324);
    gl::glGetnUniformuiv(program, location, bufSize, params);
}
void glHint(GLenum target, GLenum mode) {
    GlProfileScope _gp(325);
    gl::glHint(target, mode);
}
void glInvalidateBufferData(GLuint buffer) {
    GlProfileScope _gp(326);
    gl::glInvalidateBufferData(buffer);
}
void glInvalidateBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr length) {
    GlProfileScope _gp(327);
    gl::glInvalidateBufferSubData(buffer, offset, length);
}
void glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
    GlProfileScope _gp(328);
    gl::glInvalidateFramebuffer(target, numAttachments, attachments);
}
void glInvalidateNamedFramebufferData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments) {
    GlProfileScope _gp(329);
    gl::glInvalidateNamedFramebufferData(framebuffer, numAttachments, attachments);
}
void glInvalidateNamedFramebufferSubData(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(330);
    gl::glInvalidateNamedFramebufferSubData(framebuffer, numAttachments, attachments, x, y, width, height);
}
void glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(331);
    gl::glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
}
void glInvalidateTexImage(GLuint texture, GLint level) {
    GlProfileScope _gp(332);
    gl::glInvalidateTexImage(texture, level);
}
void glInvalidateTexSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(333);
    gl::glInvalidateTexSubImage(texture, level, xoffset, yoffset, zoffset, width, height, depth);
}
GLboolean glIsBuffer(GLuint buffer) {
    GlProfileScope _gp(334);
    return gl::glIsBuffer(buffer);
}
GLboolean glIsEnabled(GLenum cap) {
    GlProfileScope _gp(335);
    return gl::glIsEnabled(cap);
}
GLboolean glIsEnabledi(GLenum target, GLuint index) {
    GlProfileScope _gp(336);
    return gl::glIsEnabledi(target, index);
}
GLboolean glIsFramebuffer(GLuint framebuffer) {
    GlProfileScope _gp(337);
    return gl::glIsFramebuffer(framebuffer);
}
GLboolean glIsProgram(GLuint program) {
    GlProfileScope _gp(338);
    return gl::glIsProgram(program);
}
GLboolean glIsProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(339);
    return gl::glIsProgramPipeline(pipeline);
}
GLboolean glIsQuery(GLuint id) {
    GlProfileScope _gp(340);
    return gl::glIsQuery(id);
}
GLboolean glIsRenderbuffer(GLuint renderbuffer) {
    GlProfileScope _gp(341);
    return gl::glIsRenderbuffer(renderbuffer);
}
GLboolean glIsSampler(GLuint sampler) {
    GlProfileScope _gp(342);
    return gl::glIsSampler(sampler);
}
GLboolean glIsShader(GLuint shader) {
    GlProfileScope _gp(343);
    return gl::glIsShader(shader);
}
GLboolean glIsSync(GLsync sync) {
    GlProfileScope _gp(344);
    return gl::glIsSync(sync);
}
GLboolean glIsTexture(GLuint texture) {
    GlProfileScope _gp(345);
    return gl::glIsTexture(texture);
}
GLboolean glIsTransformFeedback(GLuint id) {
    GlProfileScope _gp(346);
    return gl::glIsTransformFeedback(id);
}
GLboolean glIsVertexArray(GLuint array) {
    GlProfileScope _gp(347);
    return gl::glIsVertexArray(array);
}
void glLineWidth(GLfloat width) {
    GlProfileScope _gp(348);
    gl::glLineWidth(width);
}
void glLinkProgram(GLuint program) {
    GlProfileScope _gp(349);
    gl::glLinkProgram(program);
}
void glLogicOp(GLenum opcode) {
    GlProfileScope _gp(350);
    gl::glLogicOp(opcode);
}
void * glMapBuffer(GLenum target, GLenum access) {
    GlProfileScope _gp(351);
    return gl::glMapBuffer(target, access);
}
void * glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GlProfileScope _gp(352);
    return gl::glMapBufferRange(target, offset, length, access);
}
void * glMapNamedBuffer(GLuint buffer, GLenum access) {
    GlProfileScope _gp(353);
    return gl::glMapNamedBuffer(buffer, access);
}
void * glMapNamedBufferRange(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access) {
    GlProfileScope _gp(354);
    return gl::glMapNamedBufferRange(buffer, offset, length, access);
}
void glMemoryBarrier(GLbitfield barriers) {
    GlProfileScope _gp(355);
    gl::glMemoryBarrier(barriers);
}
void glMemoryBarrierByRegion(GLbitfield barriers) {
    GlProfileScope _gp(356);
    gl::glMemoryBarrierByRegion(barriers);
}
void glMinSampleShading(GLfloat value) {
    GlProfileScope _gp(357);
    gl::glMinSampleShading(value);
}
void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount) {
    GlProfileScope _gp(358);
    gl::glMultiDrawArrays(mode, first, count, drawcount);
}
void glMultiDrawArraysIndirect(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) {
    GlProfileScope _gp(359);
    gl::glMultiDrawArraysIndirect(mode, indirect, drawcount, stride);
}
void glMultiDrawArraysIndirectCount(GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    GlProfileScope _gp(360);
    gl::glMultiDrawArraysIndirectCount(mode, indirect, drawcount, maxdrawcount, stride);
}
void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount) {
    GlProfileScope _gp(361);
    gl::glMultiDrawElements(mode, count, type, indices, drawcount);
}
void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex) {
    GlProfileScope _gp(362);
    gl::glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex);
}
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {
    GlProfileScope _gp(363);
    gl::glMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride);
}
void glMultiDrawElementsIndirectCount(GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    GlProfileScope _gp(364);
    gl::glMultiDrawElementsIndirectCount(mode, type, indirect, drawcount, maxdrawcount, stride);
}
void glMultiTexCoordP1ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(365);
    gl::glMultiTexCoordP1ui(texture, type, coords);
}
void glMultiTexCoordP1uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(366);
    gl::glMultiTexCoordP1uiv(texture, type, coords);
}
void glMultiTexCoordP2ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(367);
    gl::glMultiTexCoordP2ui(texture, type, coords);
}
void glMultiTexCoordP2uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(368);
    gl::glMultiTexCoordP2uiv(texture, type, coords);
}
void glMultiTexCoordP3ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(369);
    gl::glMultiTexCoordP3ui(texture, type, coords);
}
void glMultiTexCoordP3uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(370);
    gl::glMultiTexCoordP3uiv(texture, type, coords);
}
void glMultiTexCoordP4ui(GLenum texture, GLenum type, GLuint coords) {
    GlProfileScope _gp(371);
    gl::glMultiTexCoordP4ui(texture, type, coords);
}
void glMultiTexCoordP4uiv(GLenum texture, GLenum type, const GLuint *coords) {
    GlProfileScope _gp(372);
    gl::glMultiTexCoordP4uiv(texture, type, coords);
}
void glNamedBufferData(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage) {
    GlProfileScope _gp(373);
    gl::glNamedBufferData(buffer, size, data, usage);
}
void glNamedBufferStorage(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags) {
    GlProfileScope _gp(374);
    gl::glNamedBufferStorage(buffer, size, data, flags);
}
void glNamedBufferSubData(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data) {
    GlProfileScope _gp(375);
    gl::glNamedBufferSubData(buffer, offset, size, data);
}
void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf) {
    GlProfileScope _gp(376);
    gl::glNamedFramebufferDrawBuffer(framebuffer, buf);
}
void glNamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n, const GLenum *bufs) {
    GlProfileScope _gp(377);
    gl::glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}
void glNamedFramebufferParameteri(GLuint framebuffer, GLenum pname, GLint param) {
    GlProfileScope _gp(378);
    gl::glNamedFramebufferParameteri(framebuffer, pname, param);
}
void glNamedFramebufferReadBuffer(GLuint framebuffer, GLenum src) {
    GlProfileScope _gp(379);
    gl::glNamedFramebufferReadBuffer(framebuffer, src);
}
void glNamedFramebufferRenderbuffer(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    GlProfileScope _gp(380);
    gl::glNamedFramebufferRenderbuffer(framebuffer, attachment, renderbuffertarget, renderbuffer);
}
void glNamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    GlProfileScope _gp(381);
    gl::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTextureLayer(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    GlProfileScope _gp(382);
    gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, layer);
}
void glNamedRenderbufferStorage(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(383);
    gl::glNamedRenderbufferStorage(renderbuffer, internalformat, width, height);
}
void glNamedRenderbufferStorageMultisample(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(384);
    gl::glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height);
}
void glNormalP3ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(385);
    gl::glNormalP3ui(type, coords);
}
void glNormalP3uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(386);
    gl::glNormalP3uiv(type, coords);
}
void glObjectLabel(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) {
    GlProfileScope _gp(387);
    gl::glObjectLabel(identifier, name, length, label);
}
void glObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label) {
    GlProfileScope _gp(388);
    gl::glObjectPtrLabel(ptr, length, label);
}
void glPatchParameterfv(GLenum pname, const GLfloat *values) {
    GlProfileScope _gp(389);
    gl::glPatchParameterfv(pname, values);
}
void glPatchParameteri(GLenum pname, GLint value) {
    GlProfileScope _gp(390);
    gl::glPatchParameteri(pname, value);
}
void glPauseTransformFeedback(void) {
    GlProfileScope _gp(391);
    gl::glPauseTransformFeedback();
}
void glPixelStoref(GLenum pname, GLfloat param) {
    GlProfileScope _gp(392);
    gl::glPixelStoref(pname, param);
}
void glPixelStorei(GLenum pname, GLint param) {
    GlProfileScope _gp(393);
    gl::glPixelStorei(pname, param);
}
void glPointParameterf(GLenum pname, GLfloat param) {
    GlProfileScope _gp(394);
    gl::glPointParameterf(pname, param);
}
void glPointParameterfv(GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(395);
    gl::glPointParameterfv(pname, params);
}
void glPointParameteri(GLenum pname, GLint param) {
    GlProfileScope _gp(396);
    gl::glPointParameteri(pname, param);
}
void glPointParameteriv(GLenum pname, const GLint *params) {
    GlProfileScope _gp(397);
    gl::glPointParameteriv(pname, params);
}
void glPointSize(GLfloat size) {
    GlProfileScope _gp(398);
    gl::glPointSize(size);
}
void glPolygonMode(GLenum face, GLenum mode) {
    GlProfileScope _gp(399);
    gl::glPolygonMode(face, mode);
}
void glPolygonOffset(GLfloat factor, GLfloat units) {
    GlProfileScope _gp(400);
    gl::glPolygonOffset(factor, units);
}
void glPolygonOffsetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
    GlProfileScope _gp(401);
    gl::glPolygonOffsetClamp(factor, units, clamp);
}
void glPopDebugGroup(void) {
    GlProfileScope _gp(402);
    gl::glPopDebugGroup();
}
void glPrimitiveRestartIndex(GLuint index) {
    GlProfileScope _gp(403);
    gl::glPrimitiveRestartIndex(index);
}
void glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) {
    GlProfileScope _gp(404);
    gl::glProgramBinary(program, binaryFormat, binary, length);
}
void glProgramParameteri(GLuint program, GLenum pname, GLint value) {
    GlProfileScope _gp(405);
    gl::glProgramParameteri(program, pname, value);
}
void glProgramUniform1d(GLuint program, GLint location, GLdouble v0) {
    GlProfileScope _gp(406);
    gl::glProgramUniform1d(program, location, v0);
}
void glProgramUniform1dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(407);
    gl::glProgramUniform1dv(program, location, count, value);
}
void glProgramUniform1f(GLuint program, GLint location, GLfloat v0) {
    GlProfileScope _gp(408);
    gl::glProgramUniform1f(program, location, v0);
}
void glProgramUniform1fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(409);
    gl::glProgramUniform1fv(program, location, count, value);
}
void glProgramUniform1i(GLuint program, GLint location, GLint v0) {
    GlProfileScope _gp(410);
    gl::glProgramUniform1i(program, location, v0);
}
void glProgramUniform1iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(411);
    gl::glProgramUniform1iv(program, location, count, value);
}
void glProgramUniform1ui(GLuint program, GLint location, GLuint v0) {
    GlProfileScope _gp(412);
    gl::glProgramUniform1ui(program, location, v0);
}
void glProgramUniform1uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(413);
    gl::glProgramUniform1uiv(program, location, count, value);
}
void glProgramUniform2d(GLuint program, GLint location, GLdouble v0, GLdouble v1) {
    GlProfileScope _gp(414);
    gl::glProgramUniform2d(program, location, v0, v1);
}
void glProgramUniform2dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(415);
    gl::glProgramUniform2dv(program, location, count, value);
}
void glProgramUniform2f(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
    GlProfileScope _gp(416);
    gl::glProgramUniform2f(program, location, v0, v1);
}
void glProgramUniform2fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(417);
    gl::glProgramUniform2fv(program, location, count, value);
}
void glProgramUniform2i(GLuint program, GLint location, GLint v0, GLint v1) {
    GlProfileScope _gp(418);
    gl::glProgramUniform2i(program, location, v0, v1);
}
void glProgramUniform2iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(419);
    gl::glProgramUniform2iv(program, location, count, value);
}
void glProgramUniform2ui(GLuint program, GLint location, GLuint v0, GLuint v1) {
    GlProfileScope _gp(420);
    gl::glProgramUniform2ui(program, location, v0, v1);
}
void glProgramUniform2uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(421);
    gl::glProgramUniform2uiv(program, location, count, value);
}
void glProgramUniform3d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2) {
    GlProfileScope _gp(422);
    gl::glProgramUniform3d(program, location, v0, v1, v2);
}
void glProgramUniform3dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(423);
    gl::glProgramUniform3dv(program, location, count, value);
}
void glProgramUniform3f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GlProfileScope _gp(424);
    gl::glProgramUniform3f(program, location, v0, v1, v2);
}
void glProgramUniform3fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(425);
    gl::glProgramUniform3fv(program, location, count, value);
}
void glProgramUniform3i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
    GlProfileScope _gp(426);
    gl::glProgramUniform3i(program, location, v0, v1, v2);
}
void glProgramUniform3iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(427);
    gl::glProgramUniform3iv(program, location, count, value);
}
void glProgramUniform3ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GlProfileScope _gp(428);
    gl::glProgramUniform3ui(program, location, v0, v1, v2);
}
void glProgramUniform3uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(429);
    gl::glProgramUniform3uiv(program, location, count, value);
}
void glProgramUniform4d(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3) {
    GlProfileScope _gp(430);
    gl::glProgramUniform4d(program, location, v0, v1, v2, v3);
}
void glProgramUniform4dv(GLuint program, GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(431);
    gl::glProgramUniform4dv(program, location, count, value);
}
void glProgramUniform4f(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GlProfileScope _gp(432);
    gl::glProgramUniform4f(program, location, v0, v1, v2, v3);
}
void glProgramUniform4fv(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(433);
    gl::glProgramUniform4fv(program, location, count, value);
}
void glProgramUniform4i(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GlProfileScope _gp(434);
    gl::glProgramUniform4i(program, location, v0, v1, v2, v3);
}
void glProgramUniform4iv(GLuint program, GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(435);
    gl::glProgramUniform4iv(program, location, count, value);
}
void glProgramUniform4ui(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GlProfileScope _gp(436);
    gl::glProgramUniform4ui(program, location, v0, v1, v2, v3);
}
void glProgramUniform4uiv(GLuint program, GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(437);
    gl::glProgramUniform4uiv(program, location, count, value);
}
void glProgramUniformMatrix2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(438);
    gl::glProgramUniformMatrix2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(439);
    gl::glProgramUniformMatrix2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(440);
    gl::glProgramUniformMatrix2x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(441);
    gl::glProgramUniformMatrix2x3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(442);
    gl::glProgramUniformMatrix2x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix2x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(443);
    gl::glProgramUniformMatrix2x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(444);
    gl::glProgramUniformMatrix3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(445);
    gl::glProgramUniformMatrix3fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(446);
    gl::glProgramUniformMatrix3x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(447);
    gl::glProgramUniformMatrix3x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(448);
    gl::glProgramUniformMatrix3x4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix3x4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(449);
    gl::glProgramUniformMatrix3x4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(450);
    gl::glProgramUniformMatrix4dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(451);
    gl::glProgramUniformMatrix4fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(452);
    gl::glProgramUniformMatrix4x2dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x2fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(453);
    gl::glProgramUniformMatrix4x2fv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3dv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(454);
    gl::glProgramUniformMatrix4x3dv(program, location, count, transpose, value);
}
void glProgramUniformMatrix4x3fv(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(455);
    gl::glProgramUniformMatrix4x3fv(program, location, count, transpose, value);
}
void glProvokingVertex(GLenum mode) {
    GlProfileScope _gp(456);
    gl::glProvokingVertex(mode);
}
void glPushDebugGroup(GLenum source, GLuint id, GLsizei length, const GLchar *message) {
    GlProfileScope _gp(457);
    gl::glPushDebugGroup(source, id, length, message);
}
void glQueryCounter(GLuint id, GLenum target) {
    GlProfileScope _gp(458);
    gl::glQueryCounter(id, target);
}
void glReadBuffer(GLenum src) {
    GlProfileScope _gp(459);
    gl::glReadBuffer(src);
}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
    GlProfileScope _gp(460);
    gl::glReadPixels(x, y, width, height, format, type, pixels);
}
void glReadnPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) {
    GlProfileScope _gp(461);
    gl::glReadnPixels(x, y, width, height, format, type, bufSize, data);
}
void glReleaseShaderCompiler(void) {
    GlProfileScope _gp(462);
    gl::glReleaseShaderCompiler();
}
void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(463);
    gl::glRenderbufferStorage(target, internalformat, width, height);
}
void glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(464);
    gl::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}
void glResumeTransformFeedback(void) {
    GlProfileScope _gp(465);
    gl::glResumeTransformFeedback();
}
void glSampleCoverage(GLfloat value, GLboolean invert) {
    GlProfileScope _gp(466);
    gl::glSampleCoverage(value, invert);
}
void glSampleMaski(GLuint maskNumber, GLbitfield mask) {
    GlProfileScope _gp(467);
    gl::glSampleMaski(maskNumber, mask);
}
void glSamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *param) {
    GlProfileScope _gp(468);
    gl::glSamplerParameterIiv(sampler, pname, param);
}
void glSamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *param) {
    GlProfileScope _gp(469);
    gl::glSamplerParameterIuiv(sampler, pname, param);
}
void glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
    GlProfileScope _gp(470);
    gl::glSamplerParameterf(sampler, pname, param);
}
void glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) {
    GlProfileScope _gp(471);
    gl::glSamplerParameterfv(sampler, pname, param);
}
void glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
    GlProfileScope _gp(472);
    gl::glSamplerParameteri(sampler, pname, param);
}
void glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) {
    GlProfileScope _gp(473);
    gl::glSamplerParameteriv(sampler, pname, param);
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(474);
    gl::glScissor(x, y, width, height);
}
void glScissorArrayv(GLuint first, GLsizei count, const GLint *v) {
    GlProfileScope _gp(475);
    gl::glScissorArrayv(first, count, v);
}
void glScissorIndexed(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
    GlProfileScope _gp(476);
    gl::glScissorIndexed(index, left, bottom, width, height);
}
void glScissorIndexedv(GLuint index, const GLint *v) {
    GlProfileScope _gp(477);
    gl::glScissorIndexedv(index, v);
}
void glSecondaryColorP3ui(GLenum type, GLuint color) {
    GlProfileScope _gp(478);
    gl::glSecondaryColorP3ui(type, color);
}
void glSecondaryColorP3uiv(GLenum type, const GLuint *color) {
    GlProfileScope _gp(479);
    gl::glSecondaryColorP3uiv(type, color);
}
void glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryFormat, const void *binary, GLsizei length) {
    GlProfileScope _gp(480);
    gl::glShaderBinary(count, shaders, binaryFormat, binary, length);
}
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
    GlProfileScope _gp(481);
    gl::glShaderSource(shader, count, string, length);
}
void glShaderStorageBlockBinding(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding) {
    GlProfileScope _gp(482);
    gl::glShaderStorageBlockBinding(program, storageBlockIndex, storageBlockBinding);
}
void glSpecializeShader(GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue) {
    GlProfileScope _gp(483);
    gl::glSpecializeShader(shader, pEntryPoint, numSpecializationConstants, pConstantIndex, pConstantValue);
}
void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    GlProfileScope _gp(484);
    gl::glStencilFunc(func, ref, mask);
}
void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
    GlProfileScope _gp(485);
    gl::glStencilFuncSeparate(face, func, ref, mask);
}
void glStencilMask(GLuint mask) {
    GlProfileScope _gp(486);
    gl::glStencilMask(mask);
}
void glStencilMaskSeparate(GLenum face, GLuint mask) {
    GlProfileScope _gp(487);
    gl::glStencilMaskSeparate(face, mask);
}
void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    GlProfileScope _gp(488);
    gl::glStencilOp(fail, zfail, zpass);
}
void glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
    GlProfileScope _gp(489);
    gl::glStencilOpSeparate(face, sfail, dpfail, dppass);
}
void glTexBuffer(GLenum target, GLenum internalformat, GLuint buffer) {
    GlProfileScope _gp(490);
    gl::glTexBuffer(target, internalformat, buffer);
}
void glTexBufferRange(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(491);
    gl::glTexBufferRange(target, internalformat, buffer, offset, size);
}
void glTexCoordP1ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(492);
    gl::glTexCoordP1ui(type, coords);
}
void glTexCoordP1uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(493);
    gl::glTexCoordP1uiv(type, coords);
}
void glTexCoordP2ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(494);
    gl::glTexCoordP2ui(type, coords);
}
void glTexCoordP2uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(495);
    gl::glTexCoordP2uiv(type, coords);
}
void glTexCoordP3ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(496);
    gl::glTexCoordP3ui(type, coords);
}
void glTexCoordP3uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(497);
    gl::glTexCoordP3uiv(type, coords);
}
void glTexCoordP4ui(GLenum type, GLuint coords) {
    GlProfileScope _gp(498);
    gl::glTexCoordP4ui(type, coords);
}
void glTexCoordP4uiv(GLenum type, const GLuint *coords) {
    GlProfileScope _gp(499);
    gl::glTexCoordP4uiv(type, coords);
}
void glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(500);
    gl::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(501);
    gl::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(502);
    gl::glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(503);
    gl::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}
void glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(504);
    gl::glTexImage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexParameterIiv(GLenum target, GLenum pname, const GLint *params) {
    GlProfileScope _gp(505);
    gl::glTexParameterIiv(target, pname, params);
}
void glTexParameterIuiv(GLenum target, GLenum pname, const GLuint *params) {
    GlProfileScope _gp(506);
    gl::glTexParameterIuiv(target, pname, params);
}
void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    GlProfileScope _gp(507);
    gl::glTexParameterf(target, pname, param);
}
void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    GlProfileScope _gp(508);
    gl::glTexParameterfv(target, pname, params);
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GlProfileScope _gp(509);
    gl::glTexParameteri(target, pname, param);
}
void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    GlProfileScope _gp(510);
    gl::glTexParameteriv(target, pname, params);
}
void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
    GlProfileScope _gp(511);
    gl::glTexStorage1D(target, levels, internalformat, width);
}
void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(512);
    gl::glTexStorage2D(target, levels, internalformat, width, height);
}
void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(513);
    gl::glTexStorage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}
void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(514);
    gl::glTexStorage3D(target, levels, internalformat, width, height, depth);
}
void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(515);
    gl::glTexStorage3DMultisample(target, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(516);
    gl::glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
}
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(517);
    gl::glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(518);
    gl::glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureBarrier(void) {
    GlProfileScope _gp(519);
    gl::glTextureBarrier();
}
void glTextureBuffer(GLuint texture, GLenum internalformat, GLuint buffer) {
    GlProfileScope _gp(520);
    gl::glTextureBuffer(texture, internalformat, buffer);
}
void glTextureBufferRange(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(521);
    gl::glTextureBufferRange(texture, internalformat, buffer, offset, size);
}
void glTextureParameterIiv(GLuint texture, GLenum pname, const GLint *params) {
    GlProfileScope _gp(522);
    gl::glTextureParameterIiv(texture, pname, params);
}
void glTextureParameterIuiv(GLuint texture, GLenum pname, const GLuint *params) {
    GlProfileScope _gp(523);
    gl::glTextureParameterIuiv(texture, pname, params);
}
void glTextureParameterf(GLuint texture, GLenum pname, GLfloat param) {
    GlProfileScope _gp(524);
    gl::glTextureParameterf(texture, pname, param);
}
void glTextureParameterfv(GLuint texture, GLenum pname, const GLfloat *param) {
    GlProfileScope _gp(525);
    gl::glTextureParameterfv(texture, pname, param);
}
void glTextureParameteri(GLuint texture, GLenum pname, GLint param) {
    GlProfileScope _gp(526);
    gl::glTextureParameteri(texture, pname, param);
}
void glTextureParameteriv(GLuint texture, GLenum pname, const GLint *param) {
    GlProfileScope _gp(527);
    gl::glTextureParameteriv(texture, pname, param);
}
void glTextureStorage1D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    GlProfileScope _gp(528);
    gl::glTextureStorage1D(texture, levels, internalformat, width);
}
void glTextureStorage2D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    GlProfileScope _gp(529);
    gl::glTextureStorage2D(texture, levels, internalformat, width, height);
}
void glTextureStorage2DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(530);
    gl::glTextureStorage2DMultisample(texture, samples, internalformat, width, height, fixedsamplelocations);
}
void glTextureStorage3D(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    GlProfileScope _gp(531);
    gl::glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}
void glTextureStorage3DMultisample(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    GlProfileScope _gp(532);
    gl::glTextureStorage3DMultisample(texture, samples, internalformat, width, height, depth, fixedsamplelocations);
}
void glTextureSubImage1D(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(533);
    gl::glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
void glTextureSubImage2D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(534);
    gl::glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTextureSubImage3D(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
    GlProfileScope _gp(535);
    gl::glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
    GlProfileScope _gp(536);
    gl::glTextureView(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
}
void glTransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer) {
    GlProfileScope _gp(537);
    gl::glTransformFeedbackBufferBase(xfb, index, buffer);
}
void glTransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
    GlProfileScope _gp(538);
    gl::glTransformFeedbackBufferRange(xfb, index, buffer, offset, size);
}
void glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) {
    GlProfileScope _gp(539);
    gl::glTransformFeedbackVaryings(program, count, varyings, bufferMode);
}
void glUniform1d(GLint location, GLdouble x) {
    GlProfileScope _gp(540);
    gl::glUniform1d(location, x);
}
void glUniform1dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(541);
    gl::glUniform1dv(location, count, value);
}
void glUniform1f(GLint location, GLfloat v0) {
    GlProfileScope _gp(542);
    gl::glUniform1f(location, v0);
}
void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(543);
    gl::glUniform1fv(location, count, value);
}
void glUniform1i(GLint location, GLint v0) {
    GlProfileScope _gp(544);
    gl::glUniform1i(location, v0);
}
void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(545);
    gl::glUniform1iv(location, count, value);
}
void glUniform1ui(GLint location, GLuint v0) {
    GlProfileScope _gp(546);
    gl::glUniform1ui(location, v0);
}
void glUniform1uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(547);
    gl::glUniform1uiv(location, count, value);
}
void glUniform2d(GLint location, GLdouble x, GLdouble y) {
    GlProfileScope _gp(548);
    gl::glUniform2d(location, x, y);
}
void glUniform2dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(549);
    gl::glUniform2dv(location, count, value);
}
void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
    GlProfileScope _gp(550);
    gl::glUniform2f(location, v0, v1);
}
void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(551);
    gl::glUniform2fv(location, count, value);
}
void glUniform2i(GLint location, GLint v0, GLint v1) {
    GlProfileScope _gp(552);
    gl::glUniform2i(location, v0, v1);
}
void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(553);
    gl::glUniform2iv(location, count, value);
}
void glUniform2ui(GLint location, GLuint v0, GLuint v1) {
    GlProfileScope _gp(554);
    gl::glUniform2ui(location, v0, v1);
}
void glUniform2uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(555);
    gl::glUniform2uiv(location, count, value);
}
void glUniform3d(GLint location, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(556);
    gl::glUniform3d(location, x, y, z);
}
void glUniform3dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(557);
    gl::glUniform3dv(location, count, value);
}
void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    GlProfileScope _gp(558);
    gl::glUniform3f(location, v0, v1, v2);
}
void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(559);
    gl::glUniform3fv(location, count, value);
}
void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
    GlProfileScope _gp(560);
    gl::glUniform3i(location, v0, v1, v2);
}
void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(561);
    gl::glUniform3iv(location, count, value);
}
void glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
    GlProfileScope _gp(562);
    gl::glUniform3ui(location, v0, v1, v2);
}
void glUniform3uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(563);
    gl::glUniform3uiv(location, count, value);
}
void glUniform4d(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(564);
    gl::glUniform4d(location, x, y, z, w);
}
void glUniform4dv(GLint location, GLsizei count, const GLdouble *value) {
    GlProfileScope _gp(565);
    gl::glUniform4dv(location, count, value);
}
void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    GlProfileScope _gp(566);
    gl::glUniform4f(location, v0, v1, v2, v3);
}
void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
    GlProfileScope _gp(567);
    gl::glUniform4fv(location, count, value);
}
void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
    GlProfileScope _gp(568);
    gl::glUniform4i(location, v0, v1, v2, v3);
}
void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
    GlProfileScope _gp(569);
    gl::glUniform4iv(location, count, value);
}
void glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
    GlProfileScope _gp(570);
    gl::glUniform4ui(location, v0, v1, v2, v3);
}
void glUniform4uiv(GLint location, GLsizei count, const GLuint *value) {
    GlProfileScope _gp(571);
    gl::glUniform4uiv(location, count, value);
}
void glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
    GlProfileScope _gp(572);
    gl::glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
}
void glUniformMatrix2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(573);
    gl::glUniformMatrix2dv(location, count, transpose, value);
}
void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(574);
    gl::glUniformMatrix2fv(location, count, transpose, value);
}
void glUniformMatrix2x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(575);
    gl::glUniformMatrix2x3dv(location, count, transpose, value);
}
void glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(576);
    gl::glUniformMatrix2x3fv(location, count, transpose, value);
}
void glUniformMatrix2x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(577);
    gl::glUniformMatrix2x4dv(location, count, transpose, value);
}
void glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(578);
    gl::glUniformMatrix2x4fv(location, count, transpose, value);
}
void glUniformMatrix3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(579);
    gl::glUniformMatrix3dv(location, count, transpose, value);
}
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(580);
    gl::glUniformMatrix3fv(location, count, transpose, value);
}
void glUniformMatrix3x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(581);
    gl::glUniformMatrix3x2dv(location, count, transpose, value);
}
void glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(582);
    gl::glUniformMatrix3x2fv(location, count, transpose, value);
}
void glUniformMatrix3x4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(583);
    gl::glUniformMatrix3x4dv(location, count, transpose, value);
}
void glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(584);
    gl::glUniformMatrix3x4fv(location, count, transpose, value);
}
void glUniformMatrix4dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(585);
    gl::glUniformMatrix4dv(location, count, transpose, value);
}
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(586);
    gl::glUniformMatrix4fv(location, count, transpose, value);
}
void glUniformMatrix4x2dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(587);
    gl::glUniformMatrix4x2dv(location, count, transpose, value);
}
void glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(588);
    gl::glUniformMatrix4x2fv(location, count, transpose, value);
}
void glUniformMatrix4x3dv(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value) {
    GlProfileScope _gp(589);
    gl::glUniformMatrix4x3dv(location, count, transpose, value);
}
void glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
    GlProfileScope _gp(590);
    gl::glUniformMatrix4x3fv(location, count, transpose, value);
}
void glUniformSubroutinesuiv(GLenum shadertype, GLsizei count, const GLuint *indices) {
    GlProfileScope _gp(591);
    gl::glUniformSubroutinesuiv(shadertype, count, indices);
}
GLboolean glUnmapBuffer(GLenum target) {
    GlProfileScope _gp(592);
    return gl::glUnmapBuffer(target);
}
GLboolean glUnmapNamedBuffer(GLuint buffer) {
    GlProfileScope _gp(593);
    return gl::glUnmapNamedBuffer(buffer);
}
void glUseProgram(GLuint program) {
    GlProfileScope _gp(594);
    gl::glUseProgram(program);
}
void glUseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program) {
    GlProfileScope _gp(595);
    gl::glUseProgramStages(pipeline, stages, program);
}
void glValidateProgram(GLuint program) {
    GlProfileScope _gp(596);
    gl::glValidateProgram(program);
}
void glValidateProgramPipeline(GLuint pipeline) {
    GlProfileScope _gp(597);
    gl::glValidateProgramPipeline(pipeline);
}
void glVertexArrayAttribBinding(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
    GlProfileScope _gp(598);
    gl::glVertexArrayAttribBinding(vaobj, attribindex, bindingindex);
}
void glVertexArrayAttribFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GlProfileScope _gp(599);
    gl::glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset);
}
void glVertexArrayAttribIFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(600);
    gl::glVertexArrayAttribIFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayAttribLFormat(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(601);
    gl::glVertexArrayAttribLFormat(vaobj, attribindex, size, type, relativeoffset);
}
void glVertexArrayBindingDivisor(GLuint vaobj, GLuint bindingindex, GLuint divisor) {
    GlProfileScope _gp(602);
    gl::glVertexArrayBindingDivisor(vaobj, bindingindex, divisor);
}
void glVertexArrayElementBuffer(GLuint vaobj, GLuint buffer) {
    GlProfileScope _gp(603);
    gl::glVertexArrayElementBuffer(vaobj, buffer);
}
void glVertexArrayVertexBuffer(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride) {
    GlProfileScope _gp(604);
    gl::glVertexArrayVertexBuffer(vaobj, bindingindex, buffer, offset, stride);
}
void glVertexArrayVertexBuffers(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    GlProfileScope _gp(605);
    gl::glVertexArrayVertexBuffers(vaobj, first, count, buffers, offsets, strides);
}
void glVertexAttrib1d(GLuint index, GLdouble x) {
    GlProfileScope _gp(606);
    gl::glVertexAttrib1d(index, x);
}
void glVertexAttrib1dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(607);
    gl::glVertexAttrib1dv(index, v);
}
void glVertexAttrib1f(GLuint index, GLfloat x) {
    GlProfileScope _gp(608);
    gl::glVertexAttrib1f(index, x);
}
void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(609);
    gl::glVertexAttrib1fv(index, v);
}
void glVertexAttrib1s(GLuint index, GLshort x) {
    GlProfileScope _gp(610);
    gl::glVertexAttrib1s(index, x);
}
void glVertexAttrib1sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(611);
    gl::glVertexAttrib1sv(index, v);
}
void glVertexAttrib2d(GLuint index, GLdouble x, GLdouble y) {
    GlProfileScope _gp(612);
    gl::glVertexAttrib2d(index, x, y);
}
void glVertexAttrib2dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(613);
    gl::glVertexAttrib2dv(index, v);
}
void glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
    GlProfileScope _gp(614);
    gl::glVertexAttrib2f(index, x, y);
}
void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(615);
    gl::glVertexAttrib2fv(index, v);
}
void glVertexAttrib2s(GLuint index, GLshort x, GLshort y) {
    GlProfileScope _gp(616);
    gl::glVertexAttrib2s(index, x, y);
}
void glVertexAttrib2sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(617);
    gl::glVertexAttrib2sv(index, v);
}
void glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(618);
    gl::glVertexAttrib3d(index, x, y, z);
}
void glVertexAttrib3dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(619);
    gl::glVertexAttrib3dv(index, v);
}
void glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
    GlProfileScope _gp(620);
    gl::glVertexAttrib3f(index, x, y, z);
}
void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(621);
    gl::glVertexAttrib3fv(index, v);
}
void glVertexAttrib3s(GLuint index, GLshort x, GLshort y, GLshort z) {
    GlProfileScope _gp(622);
    gl::glVertexAttrib3s(index, x, y, z);
}
void glVertexAttrib3sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(623);
    gl::glVertexAttrib3sv(index, v);
}
void glVertexAttrib4Nbv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(624);
    gl::glVertexAttrib4Nbv(index, v);
}
void glVertexAttrib4Niv(GLuint index, const GLint *v) {
    GlProfileScope _gp(625);
    gl::glVertexAttrib4Niv(index, v);
}
void glVertexAttrib4Nsv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(626);
    gl::glVertexAttrib4Nsv(index, v);
}
void glVertexAttrib4Nub(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) {
    GlProfileScope _gp(627);
    gl::glVertexAttrib4Nub(index, x, y, z, w);
}
void glVertexAttrib4Nubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(628);
    gl::glVertexAttrib4Nubv(index, v);
}
void glVertexAttrib4Nuiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(629);
    gl::glVertexAttrib4Nuiv(index, v);
}
void glVertexAttrib4Nusv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(630);
    gl::glVertexAttrib4Nusv(index, v);
}
void glVertexAttrib4bv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(631);
    gl::glVertexAttrib4bv(index, v);
}
void glVertexAttrib4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(632);
    gl::glVertexAttrib4d(index, x, y, z, w);
}
void glVertexAttrib4dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(633);
    gl::glVertexAttrib4dv(index, v);
}
void glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GlProfileScope _gp(634);
    gl::glVertexAttrib4f(index, x, y, z, w);
}
void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(635);
    gl::glVertexAttrib4fv(index, v);
}
void glVertexAttrib4iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(636);
    gl::glVertexAttrib4iv(index, v);
}
void glVertexAttrib4s(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) {
    GlProfileScope _gp(637);
    gl::glVertexAttrib4s(index, x, y, z, w);
}
void glVertexAttrib4sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(638);
    gl::glVertexAttrib4sv(index, v);
}
void glVertexAttrib4ubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(639);
    gl::glVertexAttrib4ubv(index, v);
}
void glVertexAttrib4uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(640);
    gl::glVertexAttrib4uiv(index, v);
}
void glVertexAttrib4usv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(641);
    gl::glVertexAttrib4usv(index, v);
}
void glVertexAttribBinding(GLuint attribindex, GLuint bindingindex) {
    GlProfileScope _gp(642);
    gl::glVertexAttribBinding(attribindex, bindingindex);
}
void glVertexAttribDivisor(GLuint index, GLuint divisor) {
    GlProfileScope _gp(643);
    gl::glVertexAttribDivisor(index, divisor);
}
void glVertexAttribFormat(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset) {
    GlProfileScope _gp(644);
    gl::glVertexAttribFormat(attribindex, size, type, normalized, relativeoffset);
}
void glVertexAttribI1i(GLuint index, GLint x) {
    GlProfileScope _gp(645);
    gl::glVertexAttribI1i(index, x);
}
void glVertexAttribI1iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(646);
    gl::glVertexAttribI1iv(index, v);
}
void glVertexAttribI1ui(GLuint index, GLuint x) {
    GlProfileScope _gp(647);
    gl::glVertexAttribI1ui(index, x);
}
void glVertexAttribI1uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(648);
    gl::glVertexAttribI1uiv(index, v);
}
void glVertexAttribI2i(GLuint index, GLint x, GLint y) {
    GlProfileScope _gp(649);
    gl::glVertexAttribI2i(index, x, y);
}
void glVertexAttribI2iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(650);
    gl::glVertexAttribI2iv(index, v);
}
void glVertexAttribI2ui(GLuint index, GLuint x, GLuint y) {
    GlProfileScope _gp(651);
    gl::glVertexAttribI2ui(index, x, y);
}
void glVertexAttribI2uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(652);
    gl::glVertexAttribI2uiv(index, v);
}
void glVertexAttribI3i(GLuint index, GLint x, GLint y, GLint z) {
    GlProfileScope _gp(653);
    gl::glVertexAttribI3i(index, x, y, z);
}
void glVertexAttribI3iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(654);
    gl::glVertexAttribI3iv(index, v);
}
void glVertexAttribI3ui(GLuint index, GLuint x, GLuint y, GLuint z) {
    GlProfileScope _gp(655);
    gl::glVertexAttribI3ui(index, x, y, z);
}
void glVertexAttribI3uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(656);
    gl::glVertexAttribI3uiv(index, v);
}
void glVertexAttribI4bv(GLuint index, const GLbyte *v) {
    GlProfileScope _gp(657);
    gl::glVertexAttribI4bv(index, v);
}
void glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
    GlProfileScope _gp(658);
    gl::glVertexAttribI4i(index, x, y, z, w);
}
void glVertexAttribI4iv(GLuint index, const GLint *v) {
    GlProfileScope _gp(659);
    gl::glVertexAttribI4iv(index, v);
}
void glVertexAttribI4sv(GLuint index, const GLshort *v) {
    GlProfileScope _gp(660);
    gl::glVertexAttribI4sv(index, v);
}
void glVertexAttribI4ubv(GLuint index, const GLubyte *v) {
    GlProfileScope _gp(661);
    gl::glVertexAttribI4ubv(index, v);
}
void glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
    GlProfileScope _gp(662);
    gl::glVertexAttribI4ui(index, x, y, z, w);
}
void glVertexAttribI4uiv(GLuint index, const GLuint *v) {
    GlProfileScope _gp(663);
    gl::glVertexAttribI4uiv(index, v);
}
void glVertexAttribI4usv(GLuint index, const GLushort *v) {
    GlProfileScope _gp(664);
    gl::glVertexAttribI4usv(index, v);
}
void glVertexAttribIFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(665);
    gl::glVertexAttribIFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(666);
    gl::glVertexAttribIPointer(index, size, type, stride, pointer);
}
void glVertexAttribL1d(GLuint index, GLdouble x) {
    GlProfileScope _gp(667);
    gl::glVertexAttribL1d(index, x);
}
void glVertexAttribL1dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(668);
    gl::glVertexAttribL1dv(index, v);
}
void glVertexAttribL2d(GLuint index, GLdouble x, GLdouble y) {
    GlProfileScope _gp(669);
    gl::glVertexAttribL2d(index, x, y);
}
void glVertexAttribL2dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(670);
    gl::glVertexAttribL2dv(index, v);
}
void glVertexAttribL3d(GLuint index, GLdouble x, GLdouble y, GLdouble z) {
    GlProfileScope _gp(671);
    gl::glVertexAttribL3d(index, x, y, z);
}
void glVertexAttribL3dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(672);
    gl::glVertexAttribL3dv(index, v);
}
void glVertexAttribL4d(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GlProfileScope _gp(673);
    gl::glVertexAttribL4d(index, x, y, z, w);
}
void glVertexAttribL4dv(GLuint index, const GLdouble *v) {
    GlProfileScope _gp(674);
    gl::glVertexAttribL4dv(index, v);
}
void glVertexAttribLFormat(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset) {
    GlProfileScope _gp(675);
    gl::glVertexAttribLFormat(attribindex, size, type, relativeoffset);
}
void glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(676);
    gl::glVertexAttribLPointer(index, size, type, stride, pointer);
}
void glVertexAttribP1ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(677);
    gl::glVertexAttribP1ui(index, type, normalized, value);
}
void glVertexAttribP1uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(678);
    gl::glVertexAttribP1uiv(index, type, normalized, value);
}
void glVertexAttribP2ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(679);
    gl::glVertexAttribP2ui(index, type, normalized, value);
}
void glVertexAttribP2uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(680);
    gl::glVertexAttribP2uiv(index, type, normalized, value);
}
void glVertexAttribP3ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(681);
    gl::glVertexAttribP3ui(index, type, normalized, value);
}
void glVertexAttribP3uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(682);
    gl::glVertexAttribP3uiv(index, type, normalized, value);
}
void glVertexAttribP4ui(GLuint index, GLenum type, GLboolean normalized, GLuint value) {
    GlProfileScope _gp(683);
    gl::glVertexAttribP4ui(index, type, normalized, value);
}
void glVertexAttribP4uiv(GLuint index, GLenum type, GLboolean normalized, const GLuint *value) {
    GlProfileScope _gp(684);
    gl::glVertexAttribP4uiv(index, type, normalized, value);
}
void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
    GlProfileScope _gp(685);
    gl::glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}
void glVertexBindingDivisor(GLuint bindingindex, GLuint divisor) {
    GlProfileScope _gp(686);
    gl::glVertexBindingDivisor(bindingindex, divisor);
}
void glVertexP2ui(GLenum type, GLuint value) {
    GlProfileScope _gp(687);
    gl::glVertexP2ui(type, value);
}
void glVertexP2uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(688);
    gl::glVertexP2uiv(type, value);
}
void glVertexP3ui(GLenum type, GLuint value) {
    GlProfileScope _gp(689);
    gl::glVertexP3ui(type, value);
}
void glVertexP3uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(690);
    gl::glVertexP3uiv(type, value);
}
void glVertexP4ui(GLenum type, GLuint value) {
    GlProfileScope _gp(691);
    gl::glVertexP4ui(type, value);
}
void glVertexP4uiv(GLenum type, const GLuint *value) {
    GlProfileScope _gp(692);
    gl::glVertexP4uiv(type, value);
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GlProfileScope _gp(693);
    gl::glViewport(x, y, width, height);
}
void glViewportArrayv(GLuint first, GLsizei count, const GLfloat *v) {
    GlProfileScope _gp(694);
    gl::glViewportArrayv(first, count, v);
}
void glViewportIndexedf(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    GlProfileScope _gp(695);
    gl::glViewportIndexedf(index, x, y, w, h);
}
void glViewportIndexedfv(GLuint index, const GLfloat *v) {
    GlProfileScope _gp(696);
    gl::glViewportIndexedfv(index, v);
}
void glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
    GlProfileScope _gp(697);
    gl::glWaitSync(sync, flags, timeout);
}
void glLineStipple(GLint a, GLushort b) {
    GlProfileScope _gp(698);
    gl::glLineStipple(a, b);
}
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) {
    GlProfileScope _gp(699);
    gl::glDepthRangeArrayfvNV(a, b, c_);
}
void glGetPointerv(GLenum p, void** v) {
    GlProfileScope _gp(700);
    gl::glGetPointerv(p, v);
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
    "glColorP3ui", // 69
    "glColorP3uiv", // 70
    "glColorP4ui", // 71
    "glColorP4uiv", // 72
    "glCompileShader", // 73
    "glCompressedTexImage1D", // 74
    "glCompressedTexImage2D", // 75
    "glCompressedTexImage3D", // 76
    "glCompressedTexSubImage1D", // 77
    "glCompressedTexSubImage2D", // 78
    "glCompressedTexSubImage3D", // 79
    "glCompressedTextureSubImage1D", // 80
    "glCompressedTextureSubImage2D", // 81
    "glCompressedTextureSubImage3D", // 82
    "glCopyBufferSubData", // 83
    "glCopyImageSubData", // 84
    "glCopyNamedBufferSubData", // 85
    "glCopyTexImage1D", // 86
    "glCopyTexImage2D", // 87
    "glCopyTexSubImage1D", // 88
    "glCopyTexSubImage2D", // 89
    "glCopyTexSubImage3D", // 90
    "glCopyTextureSubImage1D", // 91
    "glCopyTextureSubImage2D", // 92
    "glCopyTextureSubImage3D", // 93
    "glCreateBuffers", // 94
    "glCreateFramebuffers", // 95
    "glCreateProgram", // 96
    "glCreateProgramPipelines", // 97
    "glCreateQueries", // 98
    "glCreateRenderbuffers", // 99
    "glCreateSamplers", // 100
    "glCreateShader", // 101
    "glCreateShaderProgramv", // 102
    "glCreateTextures", // 103
    "glCreateTransformFeedbacks", // 104
    "glCreateVertexArrays", // 105
    "glCullFace", // 106
    "glDebugMessageCallback", // 107
    "glDebugMessageControl", // 108
    "glDebugMessageInsert", // 109
    "glDeleteBuffers", // 110
    "glDeleteFramebuffers", // 111
    "glDeleteProgram", // 112
    "glDeleteProgramPipelines", // 113
    "glDeleteQueries", // 114
    "glDeleteRenderbuffers", // 115
    "glDeleteSamplers", // 116
    "glDeleteShader", // 117
    "glDeleteSync", // 118
    "glDeleteTextures", // 119
    "glDeleteTransformFeedbacks", // 120
    "glDeleteVertexArrays", // 121
    "glDepthFunc", // 122
    "glDepthMask", // 123
    "glDepthRange", // 124
    "glDepthRangeArrayv", // 125
    "glDepthRangeIndexed", // 126
    "glDepthRangef", // 127
    "glDetachShader", // 128
    "glDisable", // 129
    "glDisableVertexArrayAttrib", // 130
    "glDisableVertexAttribArray", // 131
    "glDisablei", // 132
    "glDispatchCompute", // 133
    "glDispatchComputeIndirect", // 134
    "glDrawArrays", // 135
    "glDrawArraysIndirect", // 136
    "glDrawArraysInstanced", // 137
    "glDrawArraysInstancedBaseInstance", // 138
    "glDrawBuffer", // 139
    "glDrawBuffers", // 140
    "glDrawElements", // 141
    "glDrawElementsBaseVertex", // 142
    "glDrawElementsIndirect", // 143
    "glDrawElementsInstanced", // 144
    "glDrawElementsInstancedBaseInstance", // 145
    "glDrawElementsInstancedBaseVertex", // 146
    "glDrawElementsInstancedBaseVertexBaseInstance", // 147
    "glDrawRangeElements", // 148
    "glDrawRangeElementsBaseVertex", // 149
    "glDrawTransformFeedback", // 150
    "glDrawTransformFeedbackInstanced", // 151
    "glDrawTransformFeedbackStream", // 152
    "glDrawTransformFeedbackStreamInstanced", // 153
    "glEnable", // 154
    "glEnableVertexArrayAttrib", // 155
    "glEnableVertexAttribArray", // 156
    "glEnablei", // 157
    "glEndConditionalRender", // 158
    "glEndQuery", // 159
    "glEndQueryIndexed", // 160
    "glEndTransformFeedback", // 161
    "glFenceSync", // 162
    "glFinish", // 163
    "glFlush", // 164
    "glFlushMappedBufferRange", // 165
    "glFlushMappedNamedBufferRange", // 166
    "glFramebufferParameteri", // 167
    "glFramebufferRenderbuffer", // 168
    "glFramebufferTexture", // 169
    "glFramebufferTexture1D", // 170
    "glFramebufferTexture2D", // 171
    "glFramebufferTexture3D", // 172
    "glFramebufferTextureLayer", // 173
    "glFrontFace", // 174
    "glGenBuffers", // 175
    "glGenFramebuffers", // 176
    "glGenProgramPipelines", // 177
    "glGenQueries", // 178
    "glGenRenderbuffers", // 179
    "glGenSamplers", // 180
    "glGenTextures", // 181
    "glGenTransformFeedbacks", // 182
    "glGenVertexArrays", // 183
    "glGenerateMipmap", // 184
    "glGenerateTextureMipmap", // 185
    "glGetActiveAtomicCounterBufferiv", // 186
    "glGetActiveAttrib", // 187
    "glGetActiveSubroutineName", // 188
    "glGetActiveSubroutineUniformName", // 189
    "glGetActiveSubroutineUniformiv", // 190
    "glGetActiveUniform", // 191
    "glGetActiveUniformBlockName", // 192
    "glGetActiveUniformBlockiv", // 193
    "glGetActiveUniformName", // 194
    "glGetActiveUniformsiv", // 195
    "glGetAttachedShaders", // 196
    "glGetAttribLocation", // 197
    "glGetBooleani_v", // 198
    "glGetBooleanv", // 199
    "glGetBufferParameteri64v", // 200
    "glGetBufferParameteriv", // 201
    "glGetBufferPointerv", // 202
    "glGetBufferSubData", // 203
    "glGetCompressedTexImage", // 204
    "glGetCompressedTextureImage", // 205
    "glGetCompressedTextureSubImage", // 206
    "glGetDebugMessageLog", // 207
    "glGetDoublei_v", // 208
    "glGetDoublev", // 209
    "glGetError", // 210
    "glGetFloati_v", // 211
    "glGetFloatv", // 212
    "glGetFragDataIndex", // 213
    "glGetFragDataLocation", // 214
    "glGetFramebufferAttachmentParameteriv", // 215
    "glGetFramebufferParameteriv", // 216
    "glGetGraphicsResetStatus", // 217
    "glGetInteger64i_v", // 218
    "glGetInteger64v", // 219
    "glGetIntegeri_v", // 220
    "glGetIntegerv", // 221
    "glGetInternalformati64v", // 222
    "glGetInternalformativ", // 223
    "glGetMultisamplefv", // 224
    "glGetNamedBufferParameteri64v", // 225
    "glGetNamedBufferParameteriv", // 226
    "glGetNamedBufferPointerv", // 227
    "glGetNamedBufferSubData", // 228
    "glGetNamedFramebufferAttachmentParameteriv", // 229
    "glGetNamedFramebufferParameteriv", // 230
    "glGetNamedRenderbufferParameteriv", // 231
    "glGetObjectLabel", // 232
    "glGetObjectPtrLabel", // 233
    "glGetProgramBinary", // 234
    "glGetProgramInfoLog", // 235
    "glGetProgramInterfaceiv", // 236
    "glGetProgramPipelineInfoLog", // 237
    "glGetProgramPipelineiv", // 238
    "glGetProgramResourceIndex", // 239
    "glGetProgramResourceLocation", // 240
    "glGetProgramResourceLocationIndex", // 241
    "glGetProgramResourceName", // 242
    "glGetProgramResourceiv", // 243
    "glGetProgramStageiv", // 244
    "glGetProgramiv", // 245
    "glGetQueryBufferObjecti64v", // 246
    "glGetQueryBufferObjectiv", // 247
    "glGetQueryBufferObjectui64v", // 248
    "glGetQueryBufferObjectuiv", // 249
    "glGetQueryIndexediv", // 250
    "glGetQueryObjecti64v", // 251
    "glGetQueryObjectiv", // 252
    "glGetQueryObjectui64v", // 253
    "glGetQueryObjectuiv", // 254
    "glGetQueryiv", // 255
    "glGetRenderbufferParameteriv", // 256
    "glGetSamplerParameterIiv", // 257
    "glGetSamplerParameterIuiv", // 258
    "glGetSamplerParameterfv", // 259
    "glGetSamplerParameteriv", // 260
    "glGetShaderInfoLog", // 261
    "glGetShaderPrecisionFormat", // 262
    "glGetShaderSource", // 263
    "glGetShaderiv", // 264
    "glGetString", // 265
    "glGetStringi", // 266
    "glGetSubroutineIndex", // 267
    "glGetSubroutineUniformLocation", // 268
    "glGetSynciv", // 269
    "glGetTexImage", // 270
    "glGetTexLevelParameterfv", // 271
    "glGetTexLevelParameteriv", // 272
    "glGetTexParameterIiv", // 273
    "glGetTexParameterIuiv", // 274
    "glGetTexParameterfv", // 275
    "glGetTexParameteriv", // 276
    "glGetTextureImage", // 277
    "glGetTextureLevelParameterfv", // 278
    "glGetTextureLevelParameteriv", // 279
    "glGetTextureParameterIiv", // 280
    "glGetTextureParameterIuiv", // 281
    "glGetTextureParameterfv", // 282
    "glGetTextureParameteriv", // 283
    "glGetTextureSubImage", // 284
    "glGetTransformFeedbackVarying", // 285
    "glGetTransformFeedbacki64_v", // 286
    "glGetTransformFeedbacki_v", // 287
    "glGetTransformFeedbackiv", // 288
    "glGetUniformBlockIndex", // 289
    "glGetUniformIndices", // 290
    "glGetUniformLocation", // 291
    "glGetUniformSubroutineuiv", // 292
    "glGetUniformdv", // 293
    "glGetUniformfv", // 294
    "glGetUniformiv", // 295
    "glGetUniformuiv", // 296
    "glGetVertexArrayIndexed64iv", // 297
    "glGetVertexArrayIndexediv", // 298
    "glGetVertexArrayiv", // 299
    "glGetVertexAttribIiv", // 300
    "glGetVertexAttribIuiv", // 301
    "glGetVertexAttribLdv", // 302
    "glGetVertexAttribPointerv", // 303
    "glGetVertexAttribdv", // 304
    "glGetVertexAttribfv", // 305
    "glGetVertexAttribiv", // 306
    "glGetnColorTable", // 307
    "glGetnCompressedTexImage", // 308
    "glGetnConvolutionFilter", // 309
    "glGetnHistogram", // 310
    "glGetnMapdv", // 311
    "glGetnMapfv", // 312
    "glGetnMapiv", // 313
    "glGetnMinmax", // 314
    "glGetnPixelMapfv", // 315
    "glGetnPixelMapuiv", // 316
    "glGetnPixelMapusv", // 317
    "glGetnPolygonStipple", // 318
    "glGetnSeparableFilter", // 319
    "glGetnTexImage", // 320
    "glGetnUniformdv", // 321
    "glGetnUniformfv", // 322
    "glGetnUniformiv", // 323
    "glGetnUniformuiv", // 324
    "glHint", // 325
    "glInvalidateBufferData", // 326
    "glInvalidateBufferSubData", // 327
    "glInvalidateFramebuffer", // 328
    "glInvalidateNamedFramebufferData", // 329
    "glInvalidateNamedFramebufferSubData", // 330
    "glInvalidateSubFramebuffer", // 331
    "glInvalidateTexImage", // 332
    "glInvalidateTexSubImage", // 333
    "glIsBuffer", // 334
    "glIsEnabled", // 335
    "glIsEnabledi", // 336
    "glIsFramebuffer", // 337
    "glIsProgram", // 338
    "glIsProgramPipeline", // 339
    "glIsQuery", // 340
    "glIsRenderbuffer", // 341
    "glIsSampler", // 342
    "glIsShader", // 343
    "glIsSync", // 344
    "glIsTexture", // 345
    "glIsTransformFeedback", // 346
    "glIsVertexArray", // 347
    "glLineWidth", // 348
    "glLinkProgram", // 349
    "glLogicOp", // 350
    "glMapBuffer", // 351
    "glMapBufferRange", // 352
    "glMapNamedBuffer", // 353
    "glMapNamedBufferRange", // 354
    "glMemoryBarrier", // 355
    "glMemoryBarrierByRegion", // 356
    "glMinSampleShading", // 357
    "glMultiDrawArrays", // 358
    "glMultiDrawArraysIndirect", // 359
    "glMultiDrawArraysIndirectCount", // 360
    "glMultiDrawElements", // 361
    "glMultiDrawElementsBaseVertex", // 362
    "glMultiDrawElementsIndirect", // 363
    "glMultiDrawElementsIndirectCount", // 364
    "glMultiTexCoordP1ui", // 365
    "glMultiTexCoordP1uiv", // 366
    "glMultiTexCoordP2ui", // 367
    "glMultiTexCoordP2uiv", // 368
    "glMultiTexCoordP3ui", // 369
    "glMultiTexCoordP3uiv", // 370
    "glMultiTexCoordP4ui", // 371
    "glMultiTexCoordP4uiv", // 372
    "glNamedBufferData", // 373
    "glNamedBufferStorage", // 374
    "glNamedBufferSubData", // 375
    "glNamedFramebufferDrawBuffer", // 376
    "glNamedFramebufferDrawBuffers", // 377
    "glNamedFramebufferParameteri", // 378
    "glNamedFramebufferReadBuffer", // 379
    "glNamedFramebufferRenderbuffer", // 380
    "glNamedFramebufferTexture", // 381
    "glNamedFramebufferTextureLayer", // 382
    "glNamedRenderbufferStorage", // 383
    "glNamedRenderbufferStorageMultisample", // 384
    "glNormalP3ui", // 385
    "glNormalP3uiv", // 386
    "glObjectLabel", // 387
    "glObjectPtrLabel", // 388
    "glPatchParameterfv", // 389
    "glPatchParameteri", // 390
    "glPauseTransformFeedback", // 391
    "glPixelStoref", // 392
    "glPixelStorei", // 393
    "glPointParameterf", // 394
    "glPointParameterfv", // 395
    "glPointParameteri", // 396
    "glPointParameteriv", // 397
    "glPointSize", // 398
    "glPolygonMode", // 399
    "glPolygonOffset", // 400
    "glPolygonOffsetClamp", // 401
    "glPopDebugGroup", // 402
    "glPrimitiveRestartIndex", // 403
    "glProgramBinary", // 404
    "glProgramParameteri", // 405
    "glProgramUniform1d", // 406
    "glProgramUniform1dv", // 407
    "glProgramUniform1f", // 408
    "glProgramUniform1fv", // 409
    "glProgramUniform1i", // 410
    "glProgramUniform1iv", // 411
    "glProgramUniform1ui", // 412
    "glProgramUniform1uiv", // 413
    "glProgramUniform2d", // 414
    "glProgramUniform2dv", // 415
    "glProgramUniform2f", // 416
    "glProgramUniform2fv", // 417
    "glProgramUniform2i", // 418
    "glProgramUniform2iv", // 419
    "glProgramUniform2ui", // 420
    "glProgramUniform2uiv", // 421
    "glProgramUniform3d", // 422
    "glProgramUniform3dv", // 423
    "glProgramUniform3f", // 424
    "glProgramUniform3fv", // 425
    "glProgramUniform3i", // 426
    "glProgramUniform3iv", // 427
    "glProgramUniform3ui", // 428
    "glProgramUniform3uiv", // 429
    "glProgramUniform4d", // 430
    "glProgramUniform4dv", // 431
    "glProgramUniform4f", // 432
    "glProgramUniform4fv", // 433
    "glProgramUniform4i", // 434
    "glProgramUniform4iv", // 435
    "glProgramUniform4ui", // 436
    "glProgramUniform4uiv", // 437
    "glProgramUniformMatrix2dv", // 438
    "glProgramUniformMatrix2fv", // 439
    "glProgramUniformMatrix2x3dv", // 440
    "glProgramUniformMatrix2x3fv", // 441
    "glProgramUniformMatrix2x4dv", // 442
    "glProgramUniformMatrix2x4fv", // 443
    "glProgramUniformMatrix3dv", // 444
    "glProgramUniformMatrix3fv", // 445
    "glProgramUniformMatrix3x2dv", // 446
    "glProgramUniformMatrix3x2fv", // 447
    "glProgramUniformMatrix3x4dv", // 448
    "glProgramUniformMatrix3x4fv", // 449
    "glProgramUniformMatrix4dv", // 450
    "glProgramUniformMatrix4fv", // 451
    "glProgramUniformMatrix4x2dv", // 452
    "glProgramUniformMatrix4x2fv", // 453
    "glProgramUniformMatrix4x3dv", // 454
    "glProgramUniformMatrix4x3fv", // 455
    "glProvokingVertex", // 456
    "glPushDebugGroup", // 457
    "glQueryCounter", // 458
    "glReadBuffer", // 459
    "glReadPixels", // 460
    "glReadnPixels", // 461
    "glReleaseShaderCompiler", // 462
    "glRenderbufferStorage", // 463
    "glRenderbufferStorageMultisample", // 464
    "glResumeTransformFeedback", // 465
    "glSampleCoverage", // 466
    "glSampleMaski", // 467
    "glSamplerParameterIiv", // 468
    "glSamplerParameterIuiv", // 469
    "glSamplerParameterf", // 470
    "glSamplerParameterfv", // 471
    "glSamplerParameteri", // 472
    "glSamplerParameteriv", // 473
    "glScissor", // 474
    "glScissorArrayv", // 475
    "glScissorIndexed", // 476
    "glScissorIndexedv", // 477
    "glSecondaryColorP3ui", // 478
    "glSecondaryColorP3uiv", // 479
    "glShaderBinary", // 480
    "glShaderSource", // 481
    "glShaderStorageBlockBinding", // 482
    "glSpecializeShader", // 483
    "glStencilFunc", // 484
    "glStencilFuncSeparate", // 485
    "glStencilMask", // 486
    "glStencilMaskSeparate", // 487
    "glStencilOp", // 488
    "glStencilOpSeparate", // 489
    "glTexBuffer", // 490
    "glTexBufferRange", // 491
    "glTexCoordP1ui", // 492
    "glTexCoordP1uiv", // 493
    "glTexCoordP2ui", // 494
    "glTexCoordP2uiv", // 495
    "glTexCoordP3ui", // 496
    "glTexCoordP3uiv", // 497
    "glTexCoordP4ui", // 498
    "glTexCoordP4uiv", // 499
    "glTexImage1D", // 500
    "glTexImage2D", // 501
    "glTexImage2DMultisample", // 502
    "glTexImage3D", // 503
    "glTexImage3DMultisample", // 504
    "glTexParameterIiv", // 505
    "glTexParameterIuiv", // 506
    "glTexParameterf", // 507
    "glTexParameterfv", // 508
    "glTexParameteri", // 509
    "glTexParameteriv", // 510
    "glTexStorage1D", // 511
    "glTexStorage2D", // 512
    "glTexStorage2DMultisample", // 513
    "glTexStorage3D", // 514
    "glTexStorage3DMultisample", // 515
    "glTexSubImage1D", // 516
    "glTexSubImage2D", // 517
    "glTexSubImage3D", // 518
    "glTextureBarrier", // 519
    "glTextureBuffer", // 520
    "glTextureBufferRange", // 521
    "glTextureParameterIiv", // 522
    "glTextureParameterIuiv", // 523
    "glTextureParameterf", // 524
    "glTextureParameterfv", // 525
    "glTextureParameteri", // 526
    "glTextureParameteriv", // 527
    "glTextureStorage1D", // 528
    "glTextureStorage2D", // 529
    "glTextureStorage2DMultisample", // 530
    "glTextureStorage3D", // 531
    "glTextureStorage3DMultisample", // 532
    "glTextureSubImage1D", // 533
    "glTextureSubImage2D", // 534
    "glTextureSubImage3D", // 535
    "glTextureView", // 536
    "glTransformFeedbackBufferBase", // 537
    "glTransformFeedbackBufferRange", // 538
    "glTransformFeedbackVaryings", // 539
    "glUniform1d", // 540
    "glUniform1dv", // 541
    "glUniform1f", // 542
    "glUniform1fv", // 543
    "glUniform1i", // 544
    "glUniform1iv", // 545
    "glUniform1ui", // 546
    "glUniform1uiv", // 547
    "glUniform2d", // 548
    "glUniform2dv", // 549
    "glUniform2f", // 550
    "glUniform2fv", // 551
    "glUniform2i", // 552
    "glUniform2iv", // 553
    "glUniform2ui", // 554
    "glUniform2uiv", // 555
    "glUniform3d", // 556
    "glUniform3dv", // 557
    "glUniform3f", // 558
    "glUniform3fv", // 559
    "glUniform3i", // 560
    "glUniform3iv", // 561
    "glUniform3ui", // 562
    "glUniform3uiv", // 563
    "glUniform4d", // 564
    "glUniform4dv", // 565
    "glUniform4f", // 566
    "glUniform4fv", // 567
    "glUniform4i", // 568
    "glUniform4iv", // 569
    "glUniform4ui", // 570
    "glUniform4uiv", // 571
    "glUniformBlockBinding", // 572
    "glUniformMatrix2dv", // 573
    "glUniformMatrix2fv", // 574
    "glUniformMatrix2x3dv", // 575
    "glUniformMatrix2x3fv", // 576
    "glUniformMatrix2x4dv", // 577
    "glUniformMatrix2x4fv", // 578
    "glUniformMatrix3dv", // 579
    "glUniformMatrix3fv", // 580
    "glUniformMatrix3x2dv", // 581
    "glUniformMatrix3x2fv", // 582
    "glUniformMatrix3x4dv", // 583
    "glUniformMatrix3x4fv", // 584
    "glUniformMatrix4dv", // 585
    "glUniformMatrix4fv", // 586
    "glUniformMatrix4x2dv", // 587
    "glUniformMatrix4x2fv", // 588
    "glUniformMatrix4x3dv", // 589
    "glUniformMatrix4x3fv", // 590
    "glUniformSubroutinesuiv", // 591
    "glUnmapBuffer", // 592
    "glUnmapNamedBuffer", // 593
    "glUseProgram", // 594
    "glUseProgramStages", // 595
    "glValidateProgram", // 596
    "glValidateProgramPipeline", // 597
    "glVertexArrayAttribBinding", // 598
    "glVertexArrayAttribFormat", // 599
    "glVertexArrayAttribIFormat", // 600
    "glVertexArrayAttribLFormat", // 601
    "glVertexArrayBindingDivisor", // 602
    "glVertexArrayElementBuffer", // 603
    "glVertexArrayVertexBuffer", // 604
    "glVertexArrayVertexBuffers", // 605
    "glVertexAttrib1d", // 606
    "glVertexAttrib1dv", // 607
    "glVertexAttrib1f", // 608
    "glVertexAttrib1fv", // 609
    "glVertexAttrib1s", // 610
    "glVertexAttrib1sv", // 611
    "glVertexAttrib2d", // 612
    "glVertexAttrib2dv", // 613
    "glVertexAttrib2f", // 614
    "glVertexAttrib2fv", // 615
    "glVertexAttrib2s", // 616
    "glVertexAttrib2sv", // 617
    "glVertexAttrib3d", // 618
    "glVertexAttrib3dv", // 619
    "glVertexAttrib3f", // 620
    "glVertexAttrib3fv", // 621
    "glVertexAttrib3s", // 622
    "glVertexAttrib3sv", // 623
    "glVertexAttrib4Nbv", // 624
    "glVertexAttrib4Niv", // 625
    "glVertexAttrib4Nsv", // 626
    "glVertexAttrib4Nub", // 627
    "glVertexAttrib4Nubv", // 628
    "glVertexAttrib4Nuiv", // 629
    "glVertexAttrib4Nusv", // 630
    "glVertexAttrib4bv", // 631
    "glVertexAttrib4d", // 632
    "glVertexAttrib4dv", // 633
    "glVertexAttrib4f", // 634
    "glVertexAttrib4fv", // 635
    "glVertexAttrib4iv", // 636
    "glVertexAttrib4s", // 637
    "glVertexAttrib4sv", // 638
    "glVertexAttrib4ubv", // 639
    "glVertexAttrib4uiv", // 640
    "glVertexAttrib4usv", // 641
    "glVertexAttribBinding", // 642
    "glVertexAttribDivisor", // 643
    "glVertexAttribFormat", // 644
    "glVertexAttribI1i", // 645
    "glVertexAttribI1iv", // 646
    "glVertexAttribI1ui", // 647
    "glVertexAttribI1uiv", // 648
    "glVertexAttribI2i", // 649
    "glVertexAttribI2iv", // 650
    "glVertexAttribI2ui", // 651
    "glVertexAttribI2uiv", // 652
    "glVertexAttribI3i", // 653
    "glVertexAttribI3iv", // 654
    "glVertexAttribI3ui", // 655
    "glVertexAttribI3uiv", // 656
    "glVertexAttribI4bv", // 657
    "glVertexAttribI4i", // 658
    "glVertexAttribI4iv", // 659
    "glVertexAttribI4sv", // 660
    "glVertexAttribI4ubv", // 661
    "glVertexAttribI4ui", // 662
    "glVertexAttribI4uiv", // 663
    "glVertexAttribI4usv", // 664
    "glVertexAttribIFormat", // 665
    "glVertexAttribIPointer", // 666
    "glVertexAttribL1d", // 667
    "glVertexAttribL1dv", // 668
    "glVertexAttribL2d", // 669
    "glVertexAttribL2dv", // 670
    "glVertexAttribL3d", // 671
    "glVertexAttribL3dv", // 672
    "glVertexAttribL4d", // 673
    "glVertexAttribL4dv", // 674
    "glVertexAttribLFormat", // 675
    "glVertexAttribLPointer", // 676
    "glVertexAttribP1ui", // 677
    "glVertexAttribP1uiv", // 678
    "glVertexAttribP2ui", // 679
    "glVertexAttribP2uiv", // 680
    "glVertexAttribP3ui", // 681
    "glVertexAttribP3uiv", // 682
    "glVertexAttribP4ui", // 683
    "glVertexAttribP4uiv", // 684
    "glVertexAttribPointer", // 685
    "glVertexBindingDivisor", // 686
    "glVertexP2ui", // 687
    "glVertexP2uiv", // 688
    "glVertexP3ui", // 689
    "glVertexP3uiv", // 690
    "glVertexP4ui", // 691
    "glVertexP4uiv", // 692
    "glViewport", // 693
    "glViewportArrayv", // 694
    "glViewportIndexedf", // 695
    "glViewportIndexedfv", // 696
    "glWaitSync", // 697
    "glLineStipple", // 698
    "glDepthRangeArrayfvNV", // 699
    "glGetPointerv", // 700
};
int gGlProfileN = 701;
} // namespace tglmt
