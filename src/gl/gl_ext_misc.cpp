// gl_ext_misc.cpp — small unsupported/stateful glcore extension entries.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;
extern "C" {
void glRenderbufferStorageMultisampleCoverageNV(GLenum target, GLsizei coverageSamples,
        GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);
void glApplyFramebufferAttachmentCMAAINTEL(void);
void glSubpixelPrecisionBiasNV(GLuint xbits, GLuint ybits);
void glConservativeRasterParameterfNV(GLenum pname, GLfloat value);
void glConservativeRasterParameteriNV(GLenum pname, GLint param);
void glBufferPageCommitmentARB(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit);
void glNamedBufferPageCommitmentARB(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit);
void glBufferPageCommitmentMemNV(GLenum target, GLintptr offset, GLsizeiptr size, GLuint memory,
        GLuint64 memOffset, GLboolean commit);
void glNamedBufferPageCommitmentMemNV(GLuint buffer, GLintptr offset, GLsizeiptr size, GLuint memory,
        GLuint64 memOffset, GLboolean commit);
void glTexPageCommitmentMemNV(GLenum target, GLint layer, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset, GLboolean commit);
void glTexturePageCommitmentMemNV(GLuint texture, GLint layer, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset, GLboolean commit);
void glTexturePageCommitmentEXT(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
void glEGLImageTargetTexStorageEXT(GLenum target, void* image, const GLint* attrib_list);
void glEGLImageTargetTextureStorageEXT(GLuint texture, void* image, const GLint* attrib_list);
GLsync glCreateSyncFromCLeventARB(void* context, void* event, GLbitfield flags);
void glMultiDrawArraysIndirectBindlessNV(GLenum mode, const void* indirect, GLsizei drawCount,
        GLsizei stride, GLint vertexBufferCount);
void glMultiDrawElementsIndirectBindlessNV(GLenum mode, GLenum type, const void* indirect,
        GLsizei drawCount, GLsizei stride, GLint vertexBufferCount);
void glMultiDrawArraysIndirectBindlessCountNV(GLenum mode, const void* indirect, GLsizei drawCount,
        GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);
void glMultiDrawElementsIndirectBindlessCountNV(GLenum mode, GLenum type, const void* indirect,
        GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);
void glDrawVkImageNV(GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1,
        GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);
void (*glGetVkProcAddrNV(const GLchar* name))(void);
void glSignalVkFenceNV(GLuint64 vkFence);
void glSignalVkSemaphoreNV(GLuint64 vkSemaphore);
void glWaitVkSemaphoreNV(GLuint64 vkSemaphore);
void glDrawMeshTasksEXT(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
void glDrawMeshTasksIndirectEXT(GLintptr indirect);
void glMultiDrawMeshTasksIndirectEXT(GLintptr indirect, GLsizei drawcount, GLsizei stride);
void glMultiDrawMeshTasksIndirectCountEXT(GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);
void glDrawMeshTasksNV(GLuint first, GLuint count);
void glDrawMeshTasksIndirectNV(GLintptr indirect);
void glMultiDrawMeshTasksIndirectNV(GLintptr indirect, GLsizei drawcount, GLsizei stride);
void glMultiDrawMeshTasksIndirectCountNV(GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);
}

void glRenderbufferStorageMultisampleCoverageNV(GLenum target, GLsizei coverageSamples,
        GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) {
    (void)coverageSamples;
    tglmt::gl::glRenderbufferStorageMultisample(target, colorSamples, internalformat, width, height);
}
void glApplyFramebufferAttachmentCMAAINTEL(void) {
    Context::Current().errors.Record(0x0502);
}
void glSubpixelPrecisionBiasNV(GLuint xbits, GLuint ybits) {
    Context& c = Context::Current();
    if (xbits > 16 || ybits > 16) { c.errors.Record(0x0501); return; }
    c.ext.subpixelBias[0] = xbits; c.ext.subpixelBias[1] = ybits;
}
void glConservativeRasterParameterfNV(GLenum pname, GLfloat value) {
    Context& c = Context::Current();
    if (pname != 0x9379) { c.errors.Record(0x0500); return; } // CONSERVATIVE_RASTER_DILATE_NV
    if (value < 0) { c.errors.Record(0x0501); return; }
    c.ext.conservativeDilate = value;
}
void glConservativeRasterParameteriNV(GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (pname != 0x954D) { c.errors.Record(0x0500); return; } // CONSERVATIVE_RASTER_MODE_NV
    if (param < 0 || param > 2) { c.errors.Record(0x0501); return; }
    c.ext.conservativeMode = param;
}
void glBufferPageCommitmentMemNV(GLenum target, GLintptr offset, GLsizeiptr size, GLuint memory,
        GLuint64 memOffset, GLboolean commit) {
    if (memory != 0) { Context::Current().errors.Record(0x0502); return; }
    (void)memOffset; glBufferPageCommitmentARB(target, offset, size, commit);
}
void glNamedBufferPageCommitmentMemNV(GLuint buffer, GLintptr offset, GLsizeiptr size, GLuint memory,
        GLuint64 memOffset, GLboolean commit) {
    if (memory != 0) { Context::Current().errors.Record(0x0502); return; }
    (void)memOffset; glNamedBufferPageCommitmentARB(buffer, offset, size, commit);
}
void glTexPageCommitmentMemNV(GLenum target, GLint layer, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset, GLboolean commit) {
    (void)target;(void)layer;(void)level;(void)xoffset;(void)yoffset;(void)zoffset;
    (void)width;(void)height;(void)depth;(void)offset;(void)commit;
    if (memory != 0) { Context::Current().errors.Record(0x0502); return; }
}
void glTexturePageCommitmentMemNV(GLuint texture, GLint layer, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset, GLboolean commit) {
    (void)layer;(void)offset;
    if (memory != 0) { Context::Current().errors.Record(0x0502); return; }
    glTexturePageCommitmentEXT(texture, level, xoffset, yoffset, zoffset, width, height, depth, commit);
}
void glEGLImageTargetTexStorageEXT(GLenum target, void* image, const GLint* attrib_list) {
    (void)target;(void)image;(void)attrib_list; Context::Current().errors.Record(0x0502);
}
void glEGLImageTargetTextureStorageEXT(GLuint texture, void* image, const GLint* attrib_list) {
    (void)texture;(void)image;(void)attrib_list; Context::Current().errors.Record(0x0502);
}
GLsync glCreateSyncFromCLeventARB(void* context, void* event, GLbitfield flags) {
    (void)context;(void)event;(void)flags; Context::Current().errors.Record(0x0502); return nullptr;
}
void glMultiDrawArraysIndirectBindlessNV(GLenum mode, const void* indirect, GLsizei drawCount,
        GLsizei stride, GLint vertexBufferCount) {
    (void)mode;(void)indirect;(void)drawCount;(void)stride;(void)vertexBufferCount;
    Context::Current().errors.Record(0x0502);
}
void glMultiDrawElementsIndirectBindlessNV(GLenum mode, GLenum type, const void* indirect,
        GLsizei drawCount, GLsizei stride, GLint vertexBufferCount) {
    (void)mode;(void)type;(void)indirect;(void)drawCount;(void)stride;(void)vertexBufferCount;
    Context::Current().errors.Record(0x0502);
}
void glMultiDrawArraysIndirectBindlessCountNV(GLenum mode, const void* indirect, GLsizei drawCount,
        GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount) {
    (void)mode;(void)indirect;(void)drawCount;(void)maxDrawCount;(void)stride;(void)vertexBufferCount;
    Context::Current().errors.Record(0x0502);
}
void glMultiDrawElementsIndirectBindlessCountNV(GLenum mode, GLenum type, const void* indirect,
        GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount) {
    (void)mode;(void)type;(void)indirect;(void)drawCount;(void)maxDrawCount;(void)stride;(void)vertexBufferCount;
    Context::Current().errors.Record(0x0502);
}
void glDrawVkImageNV(GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1,
        GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1) {
    (void)vkImage;(void)sampler;(void)x0;(void)y0;(void)x1;(void)y1;(void)z;
    (void)s0;(void)t0;(void)s1;(void)t1; Context::Current().errors.Record(0x0502);
}
void (*glGetVkProcAddrNV(const GLchar* name))(void) {
    (void)name; Context::Current().errors.Record(0x0502); return nullptr;
}
void glSignalVkFenceNV(GLuint64 vkFence) { (void)vkFence; Context::Current().errors.Record(0x0502); }
void glSignalVkSemaphoreNV(GLuint64 vkSemaphore) { (void)vkSemaphore; Context::Current().errors.Record(0x0502); }
void glWaitVkSemaphoreNV(GLuint64 vkSemaphore) { (void)vkSemaphore; Context::Current().errors.Record(0x0502); }
void glDrawMeshTasksEXT(GLuint x, GLuint y, GLuint z) { (void)x;(void)y;(void)z; Context::Current().errors.Record(0x0502); }
void glDrawMeshTasksIndirectEXT(GLintptr indirect) { (void)indirect; Context::Current().errors.Record(0x0502); }
void glMultiDrawMeshTasksIndirectEXT(GLintptr indirect, GLsizei drawcount, GLsizei stride) {
    (void)indirect;(void)drawcount;(void)stride; Context::Current().errors.Record(0x0502);
}
void glMultiDrawMeshTasksIndirectCountEXT(GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    (void)indirect;(void)drawcount;(void)maxdrawcount;(void)stride; Context::Current().errors.Record(0x0502);
}
void glDrawMeshTasksNV(GLuint first, GLuint count) { (void)first;(void)count; Context::Current().errors.Record(0x0502); }
void glDrawMeshTasksIndirectNV(GLintptr indirect) { (void)indirect; Context::Current().errors.Record(0x0502); }
void glMultiDrawMeshTasksIndirectNV(GLintptr indirect, GLsizei drawcount, GLsizei stride) {
    (void)indirect;(void)drawcount;(void)stride; Context::Current().errors.Record(0x0502);
}
void glMultiDrawMeshTasksIndirectCountNV(GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    (void)indirect;(void)drawcount;(void)maxdrawcount;(void)stride; Context::Current().errors.Record(0x0502);
}
