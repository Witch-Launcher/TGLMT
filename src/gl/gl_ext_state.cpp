// gl_ext_state.cpp — stateful entries for small glcore extensions.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>

using namespace tglmt;
namespace tglmt { namespace gl { void glBlendBarrierKHR(void); } }

extern "C" {

void glTextureStorage1DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width) {
    tglmt::gl::glTextureStorage1D(texture, levels, internalformat, width);
}
void glTextureStorage2DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
    tglmt::gl::glTextureStorage2D(texture, levels, internalformat, width, height);
}
void glTextureStorage3DEXT(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
    tglmt::gl::glTextureStorage3D(texture, levels, internalformat, width, height, depth);
}

static FramebufferObject* FBOById(GLuint id) {
    Context& c = Context::Current();
    auto it = c.fbos.find(id);
    return it == c.fbos.end() ? nullptr : &it->second;
}
static FramebufferObject* CurrentFBO(GLenum target) {
    Context& c = Context::Current();
    GLuint id = (target == 0x8CA8) ? c.state.BoundReadFBO() : c.state.BoundDrawFBO();
    return FBOById(id);
}
static void SetSampleLocations(FramebufferObject* f, GLuint start, GLsizei count, const GLfloat* v) {
    Context& c = Context::Current();
    if (!f) { c.errors.Record(0x0502); return; }
    if (start > 32 || count < 0 || start + (GLuint)count > 32 || (count && !v)) {
        c.errors.Record(0x0501); return;
    }
    if (f->sampleLocations.size() < (size_t)(start + count) * 2)
        f->sampleLocations.resize((size_t)(start + count) * 2, 0.0f);
    if (count) std::memcpy(f->sampleLocations.data() + (size_t)start * 2, v,
                           (size_t)count * 2 * sizeof(GLfloat));
}
void glFramebufferSampleLocationsfvARB(GLenum target, GLuint start, GLsizei count, const GLfloat* v) {
    SetSampleLocations(CurrentFBO(target), start, count, v);
}
void glNamedFramebufferSampleLocationsfvARB(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat* v) {
    SetSampleLocations(FBOById(framebuffer), start, count, v);
}
void glFramebufferSampleLocationsfvNV(GLenum target, GLuint start, GLsizei count, const GLfloat* v) {
    glFramebufferSampleLocationsfvARB(target, start, count, v);
}
void glNamedFramebufferSampleLocationsfvNV(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat* v) {
    glNamedFramebufferSampleLocationsfvARB(framebuffer, start, count, v);
}
void glEvaluateDepthValuesARB(void) {
    Context::Current().FlushPendingEncoder();
}
void glResolveDepthValuesNV(void) { glEvaluateDepthValuesARB(); }

void glScissorExclusiveNV(GLint x, GLint y, GLsizei w, GLsizei h) {
    tglmt::gl::glScissor(x, y, w, h);
}
void glScissorExclusiveArrayvNV(GLuint first, GLsizei count, const GLint* v) {
    tglmt::gl::glScissorArrayv(first, count, v);
}

void glWindowRectanglesEXT(GLenum mode, GLsizei count, const GLint* box) {
    Context& c = Context::Current();
    if (mode != 0x82F2 && mode != 0x82F3) { c.errors.Record(0x0500); return; } // INCLUSIVE/EXCLUSIVE
    if (count < 0 || count > 32 || (count && !box)) { c.errors.Record(0x0501); return; }
    c.ext.windowRectMode = mode;
    c.ext.windowRects.assign((size_t)count * 4, 0);
    if (count) std::memcpy(c.ext.windowRects.data(), box, (size_t)count * 4 * sizeof(GLint));
}

void glRasterSamplesEXT(GLuint samples, GLboolean fixedsamplelocations) {
    Context& c = Context::Current();
    (void)fixedsamplelocations;
    if (samples > 32) { c.errors.Record(0x0501); return; }
    c.ext.rasterSamples = (GLint)samples;
}
void glViewportPositionWScaleNV(GLuint index, GLfloat xcoeff, GLfloat ycoeff) {
    Context& c = Context::Current();
    if (index >= 32) { c.errors.Record(0x0501); return; }
    c.ext.viewportWScale[index * 2] = xcoeff;
    c.ext.viewportWScale[index * 2 + 1] = ycoeff;
}
void glViewportSwizzleNV(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew) {
    Context& c = Context::Current();
    if (index >= 32) { c.errors.Record(0x0501); return; }
    auto ok = [](GLenum e) { return e >= 0x90E0 && e <= 0x90E7; };
    if (!ok(swizzlex) || !ok(swizzley) || !ok(swizzlez) || !ok(swizzlew)) {
        c.errors.Record(0x0500); return;
    }
    c.ext.viewportSwizzle[index][0] = swizzlex;
    c.ext.viewportSwizzle[index][1] = swizzley;
    c.ext.viewportSwizzle[index][2] = swizzlez;
    c.ext.viewportSwizzle[index][3] = swizzlew;
}
void glFragmentCoverageColorNV(GLuint r, GLuint g, GLuint b) {
    Context& c = Context::Current();
    c.ext.coverageColor[0] = (GLfloat)(r & 0xFF) / 255.0f;
    c.ext.coverageColor[1] = (GLfloat)(g & 0xFF) / 255.0f;
    c.ext.coverageColor[2] = (GLfloat)(b & 0xFF) / 255.0f;
    c.ext.coverageColor[3] = 1.0f;
}
void glBlendBarrierNV(void) { tglmt::gl::glBlendBarrierKHR(); }
void glBlendParameteriNV(GLenum pname, GLint value) {
    Context& c = Context::Current();
    if (pname != 0x9283) { c.errors.Record(0x0500); return; } // BLEND_OVERLAP_NV
    if (value != 0 && value != 1) { c.errors.Record(0x0501); return; }
    c.ext.blendParam = value;
}
void glCoverageModulationNV(GLenum components) {
    Context& c = Context::Current();
    if (components < 0x9367 || components > 0x936A) { c.errors.Record(0x0500); return; }
    c.ext.coverageModulation = (GLint)components;
}
void glCoverageModulationTableNV(GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    if (n < 0 || n > 16 || (n && !v)) { c.errors.Record(0x0501); return; }
    c.ext.coverageTable.assign((size_t)n, 1.0f);
    if (n) std::memcpy(c.ext.coverageTable.data(), v, (size_t)n * sizeof(GLfloat));
}
void glGetCoverageModulationTableNV(GLsizei bufsize, GLfloat* v) {
    Context& c = Context::Current();
    if (bufsize < 0 || (bufsize && !v)) { c.errors.Record(0x0501); return; }
    GLsizei n = std::min<GLsizei>(bufsize, (GLsizei)c.ext.coverageTable.size());
    if (n) std::memcpy(v, c.ext.coverageTable.data(), (size_t)n * sizeof(GLfloat));
}

void glRenderbufferStorageMultisampleAdvancedAMD(GLenum target, GLsizei samples, GLsizei rbsamples,
        GLenum internalformat, GLsizei width, GLsizei height) {
    if (rbsamples != 0 && rbsamples != samples) {
        Context::Current().errors.Record(0x0501); return;
    }
    tglmt::gl::glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}
void glNamedRenderbufferStorageMultisampleAdvancedAMD(GLuint renderbuffer, GLsizei samples, GLsizei rbsamples,
        GLenum internalformat, GLsizei width, GLsizei height) {
    if (rbsamples != 0 && rbsamples != samples) {
        Context::Current().errors.Record(0x0501); return;
    }
    tglmt::gl::glNamedRenderbufferStorageMultisample(renderbuffer, samples, internalformat, width, height);
}

void glGetInternalformatSampleivNV(GLenum target, GLenum internalformat, GLsizei samples,
        GLenum pname, GLsizei bufSize, GLint* params) {
    Context& c = Context::Current();
    (void)target; (void)internalformat;
    if (samples < 0 || bufSize < 0 || (bufSize && !params)) { c.errors.Record(0x0501); return; }
    if (pname != 0x9380 && pname != 0x9381) { c.errors.Record(0x0500); return; }
    if (bufSize > 0) params[0] = 0;
}
void glFramebufferFetchBarrierEXT(void) { tglmt::gl::glBlendBarrierKHR(); }

void glClearDepthdNV(GLdouble depth) { tglmt::gl::glClearDepth(depth); }
void glDepthRangedNV(GLdouble n, GLdouble f) { tglmt::gl::glDepthRange(n, f); }
void glDepthBoundsdNV(GLdouble n, GLdouble f) {
    Context& c = Context::Current();
    if (n < 0 || n > 1 || f < 0 || f > 1 || n > f) { c.errors.Record(0x0501); return; }
    c.ext.depthBounds[0] = n; c.ext.depthBounds[1] = f;
}

void glBufferPageCommitmentARB(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit) {
    Context& c = Context::Current();
    GLuint b = c.state.BoundBuffer(target);
    auto it = c.buffers.find(b);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (offset < 0 || size < 0 || (size_t)(offset + size) > BufferLen(it->second)) {
        c.errors.Record(0x0501); return;
    }
    (void)commit; // full allocation: page commitment is already true
}
void glNamedBufferPageCommitmentARB(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit) {
    Context& c = Context::Current();
    auto it = c.buffers.find(buffer);
    if (it == c.buffers.end()) { c.errors.Record(0x0502); return; }
    if (offset < 0 || size < 0 || (size_t)(offset + size) > BufferLen(it->second)) {
        c.errors.Record(0x0501); return;
    }
    (void)commit;
}
void glNamedBufferPageCommitmentEXT(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit) {
    glNamedBufferPageCommitmentARB(buffer, offset, size, commit);
}
void glTexPageCommitmentARB(GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit) {
    Context& c = Context::Current();
    TextureObject* t = nullptr;
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), target);
    auto it = c.textures.find(id);
    if (it != c.textures.end()) t = &it->second;
    if (!t) { c.errors.Record(0x0502); return; }
    if (level < 0 || xoffset < 0 || yoffset < 0 || zoffset < 0 ||
        width < 0 || height < 0 || depth < 0 ||
        (size_t)(xoffset + width) > t->w || (size_t)(yoffset + height) > t->h ||
        (size_t)(zoffset + depth) > std::max<uint32_t>(t->d, 1)) {
        c.errors.Record(0x0501); return;
    }
    (void)commit;
}

void glFramebufferTextureMultiviewOVR(GLenum target, GLenum attachment, GLuint texture,
        GLint level, GLint baseViewIndex, GLsizei numViews) {
    if (numViews < 1 || baseViewIndex < 0) {
        Context::Current().errors.Record(0x0501); return;
    }
    tglmt::gl::glFramebufferTextureLayer(target, attachment, texture, level, baseViewIndex);
}
void glNamedFramebufferTextureMultiviewOVR(GLuint framebuffer, GLenum attachment, GLuint texture,
        GLint level, GLint baseViewIndex, GLsizei numViews) {
    if (numViews < 1 || baseViewIndex < 0) {
        Context::Current().errors.Record(0x0501); return;
    }
    tglmt::gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, baseViewIndex);
}

} // extern "C"
