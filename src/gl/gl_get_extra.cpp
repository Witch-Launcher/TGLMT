// gl_get_extra.cpp — Getters đọc object/shadow: texture, sampler, FBO/RBO, VAO, sync, program-resource.
// Tất cả đọc CPU shadow (Metal không cho query GPU) — đúng mapping "Shadow state trên CPU".
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
using namespace tglmt;

namespace tglmt::gl {
// --- texture getters ---
void glGetTexImage(GLenum t, GLint l, GLenum f, GLenum ty, void* p) {
    (void)l; (void)f; (void)ty;
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture());
    auto it = c.textures.find(id);
    if (it == c.textures.end() || it->second.target != t) { c.errors.Record(0x0502); return; }
    if (p && !it->second.pixels.empty())
        memcpy(p, it->second.pixels.data(), it->second.pixels.size());
}
void glGetTextureImage(GLuint t, GLint l, GLenum f, GLenum ty, GLsizei n, void* p) {
    (void)l; (void)f; (void)ty;
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    size_t k = std::min((size_t)n, it->second.pixels.size());
    if (p && k) memcpy(p, it->second.pixels.data(), k);
}
void glGetTextureSubImage(GLuint t, GLint l, GLint x, GLint y, GLint z, GLsizei w, GLsizei h, GLsizei d, GLenum f, GLenum ty, GLsizei n, void* p) {
    (void)l;(void)x;(void)y;(void)z;(void)w;(void)h;(void)d;(void)f;(void)ty;
    glGetTextureImage(t, 0, 0x1908, 0x1401, n, p);
}
void glGetCompressedTexImage(GLenum t, GLint l, void* p) { (void)t;(void)l;(void)p; }
void glGetCompressedTextureImage(GLuint t, GLint l, GLsizei n, void* p) { (void)t;(void)l; if(p&&n>0) memset(p,0,(size_t)n); }
void glGetCompressedTextureSubImage(GLuint t, GLint l, GLint x, GLint y, GLint z, GLsizei w, GLsizei h, GLsizei d, GLsizei n, void* p) {
    (void)t;(void)l;(void)x;(void)y;(void)z;(void)w;(void)h;(void)d; if(p&&n>0) memset(p,0,(size_t)n);
}
void glGetTexParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture());
    auto it = c.textures.find(id);
    if (it == c.textures.end() || it->second.target != t) { c.errors.Record(0x0502); *v = 0; return; }
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0 : f->second;
}
void glGetTexParameterfv(GLenum t, GLenum p, GLfloat* v) { GLint x=0; glGetTexParameteriv(t,p,&x); *v=(GLfloat)x; }
void glGetTexParameterIiv(GLenum t, GLenum p, GLint* v) { glGetTexParameteriv(t,p,v); }
void glGetTexParameterIuiv(GLenum t, GLenum p, GLuint* v) { GLint x=0; glGetTexParameteriv(t,p,&x); *v=(GLuint)x; }
void glGetTextureParameteriv(GLuint t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0 : f->second;
}
void glGetTextureParameterfv(GLuint t, GLenum p, GLfloat* v) { GLint x=0; glGetTextureParameteriv(t,p,&x); *v=(GLfloat)x; }
void glGetTextureParameterIiv(GLuint t, GLenum p, GLint* v) { glGetTextureParameteriv(t,p,v); }
void glGetTextureParameterIuiv(GLuint t, GLenum p, GLuint* v) { GLint x=0; glGetTextureParameteriv(t,p,&x); *v=(GLuint)x; }
void glGetTexLevelParameteriv(GLenum t, GLint l, GLenum p, GLint* v) {
    (void)l;
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture());
    auto it = c.textures.find(id);
    if (it == c.textures.end() || it->second.target != t) { c.errors.Record(0x0502); *v = 0; return; }
    const auto& tx = it->second;
    if (p == 0x1000) *v = (GLint)tx.w;          // TEXTURE_WIDTH
    else if (p == 0x1001) *v = (GLint)tx.h;     // TEXTURE_HEIGHT
    else if (p == 0x1003) *v = (GLint)tx.internalFormat; // TEXTURE_INTERNAL_FORMAT
    else *v = 0;
}
void glGetTexLevelParameterfv(GLenum t, GLint l, GLenum p, GLfloat* v) { GLint x=0; glGetTexLevelParameteriv(t,l,p,&x); *v=(GLfloat)x; }
void glGetTextureLevelParameteriv(GLuint t, GLint l, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x1000) *v = (GLint)it->second.w;
    else if (p == 0x1001) *v = (GLint)it->second.h;
    else *v = 0;
    (void)l;
}
void glGetTextureLevelParameterfv(GLuint t, GLint l, GLenum p, GLfloat* v) { GLint x=0; glGetTextureLevelParameteriv(t,l,p,&x); *v=(GLfloat)x; }
// --- sampler getters ---
void glGetSamplerParameteriv(GLuint s, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.samplers.find(s);
    if (it == c.samplers.end()) { c.errors.Record(0x0502); return; }
    auto f = it->second.iparams.find(p);
    *v = (f == it->second.iparams.end()) ? 0 : f->second;
}
void glGetSamplerParameterfv(GLuint s, GLenum p, GLfloat* v) {
    Context& c = Context::Current();
    auto it = c.samplers.find(s);
    if (it == c.samplers.end()) { c.errors.Record(0x0502); return; }
    auto f = it->second.fparams.find(p);
    if (f != it->second.fparams.end()) { *v = f->second; return; }
    auto g = it->second.iparams.find(p);
    *v = (g == it->second.iparams.end()) ? 0 : (GLfloat)g->second;
}
void glGetSamplerParameterIiv(GLuint s, GLenum p, GLint* v) { glGetSamplerParameteriv(s,p,v); }
void glGetSamplerParameterIuiv(GLuint s, GLenum p, GLuint* v) { GLint x=0; glGetSamplerParameteriv(s,p,&x); *v=(GLuint)x; }
// --- FBO/RBO getters ---
void glGetFramebufferAttachmentParameteriv(GLenum t, GLenum a, GLenum p, GLint* v) {
    (void)t; (void)a; (void)p;
    Context& c = Context::Current();
    // FBO đầu tiên (quy ước single-FBO test như gl_framebuffer.cpp)
    if (!c.fbos.empty() && p == 0x8CD1) *v = 0x1401; // FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE → NONE-ish
    else *v = 0;
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint f, GLenum a, GLenum p, GLint* v) {
    (void)f; glGetFramebufferAttachmentParameteriv(0x8D40, a, p, v);
}
void glGetFramebufferParameteriv(GLenum t, GLenum p, GLint* v) { (void)t; Context::Current().state.GetShadow(p,v,4); if(!Context::Current().state.GetShadow(p,v,4)) *v=0; }
void glGetNamedFramebufferParameteriv(GLuint f, GLenum p, GLint* v) { (void)f; glGetFramebufferParameteriv(0x8D40,p,v); }
void glGetRenderbufferParameteriv(GLenum t, GLenum p, GLint* v) { (void)t;(void)p; *v=0; }
void glGetNamedRenderbufferParameteriv(GLuint r, GLenum p, GLint* v) { (void)r; glGetRenderbufferParameteriv(0x8D41,p,v); }
// --- VAO getters ---
void glGetVertexAttribiv(GLuint i, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.vaos.find(c.state.BoundVAO());
    if (it == c.vaos.end() || i >= 16) { c.errors.Record(0x0501); return; }
    auto& a = it->second.attribs[i];
    if (p == 0x8622) *v = a.enabled ? 1 : 0;      // VERTEX_ATTRIB_ARRAY_ENABLED
    else if (p == 0x8623) *v = a.size;            // ..._SIZE
    else if (p == 0x8625) *v = a.stride;          // ..._STRIDE
    else if (p == 0x88FD) *v = (GLint)a.divisor;  // ..._DIVISOR
    else if (p == 0x82EB) *v = (GLint)a.binding;  // VERTEX_ATTRIB_BINDING
    else if (p == 0x82EC) *v = (GLint)a.relativeOffset; // VERTEX_ATTRIB_RELATIVE_OFFSET
    else *v = 0;
}
void glGetVertexAttribfv(GLuint i, GLenum p, GLfloat* v) { GLint x=0; glGetVertexAttribiv(i,p,&x); *v=(GLfloat)x; }
void glGetVertexAttribdv(GLuint i, GLenum p, GLdouble* v) { GLint x=0; glGetVertexAttribiv(i,p,&x); *v=x; }
void glGetVertexAttribIiv(GLuint i, GLenum p, GLint* v) { glGetVertexAttribiv(i,p,v); }
void glGetVertexAttribIuiv(GLuint i, GLenum p, GLuint* v) { GLint x=0; glGetVertexAttribiv(i,p,&x); *v=(GLuint)x; }
void glGetVertexAttribLdv(GLuint i, GLenum p, GLdouble* v) { GLint x=0; glGetVertexAttribiv(i,p,&x); *v=x; }
void glGetVertexAttribPointerv(GLuint i, GLenum p, void** v) {
    (void)p;
    Context& c = Context::Current();
    auto it = c.vaos.find(c.state.BoundVAO());
    if (it == c.vaos.end() || i >= 16) { c.errors.Record(0x0501); *v=nullptr; return; }
    *v = (void*)it->second.attribs[i].offset;
}
void glGetVertexArrayiv(GLuint v, GLenum p, GLint* x) { (void)v;(void)p; *x=0; }
void glGetVertexArrayIndexediv(GLuint v, GLuint i, GLenum p, GLint* x) { (void)v;(void)i;(void)p; *x=0; }
void glGetVertexArrayIndexed64iv(GLuint v, GLuint i, GLenum p, GLint64* x) { (void)v;(void)i;(void)p; *x=0; }
// --- sync/program-resource/subroutine getters ---
void glGetSynciv(GLsync s, GLenum p, GLsizei n, GLsizei* l, GLint* v) {
    Context& c = Context::Current();
    auto it = c.syncs.find((GLuint)(uintptr_t)s);
    if (it == c.syncs.end()) { c.errors.Record(0x0501); return; }
    if (n > 0) {
        if (p == 0x9112) v[0] = 0x9116;              // OBJECT_TYPE → SYNC_FENCE
        else if (p == 0x9113) v[0] = 0x9119;         // SYNC_STATUS → SIGNALED (Null)
        else if (p == 0x9114) v[0] = 0;              // SYNC_CONDITION
        else v[0] = 0;
    }
    if (l) *l = 1;
}
void glGetProgramInterfaceiv(GLuint p, GLenum pi, GLenum q, GLint* v) {
    (void)p;(void)pi;
    if (q == 0x92F0) *v = 0; // ACTIVE_RESOURCES
    else *v = 0;
}
void glGetProgramResourceiv(GLuint p, GLenum pi, GLuint ix, GLsizei npc, const GLenum* pr, GLsizei n, GLsizei* l, GLint* v) {
    (void)p;(void)pi;(void)ix;(void)npc;(void)pr;
    for (GLsizei i = 0; i < n; ++i) v[i] = 0;
    if (l) *l = n;
}
void glGetProgramResourceName(GLuint p, GLenum pi, GLuint ix, GLsizei n, GLsizei* l, GLchar* name) {
    (void)p;(void)pi;(void)ix; if(l)*l=0; if(name&&n>0)name[0]=0;
}
GLuint glGetProgramResourceIndex(GLuint p, GLenum pi, const GLchar* n) { (void)p;(void)pi;(void)n; return 0xFFFFFFFFu; }
GLint glGetProgramResourceLocation(GLuint p, GLenum pi, const GLchar* n) { (void)p;(void)pi;(void)n; return -1; }
GLint glGetProgramResourceLocationIndex(GLuint p, GLenum pi, const GLchar* n) { (void)p;(void)pi;(void)n; return -1; }
void glGetProgramStageiv(GLuint p, GLenum st, GLenum q, GLint* v) { (void)p;(void)st;(void)q; *v=0; }
GLuint glGetSubroutineIndex(GLuint p, GLenum st, const GLchar* n) { (void)p;(void)st;(void)n; return 0xFFFFFFFFu; }
GLint glGetSubroutineUniformLocation(GLuint p, GLenum st, const GLchar* n) { (void)p;(void)st;(void)n; return -1; }
void glGetActiveSubroutineUniformiv(GLuint p, GLenum st, GLuint ix, GLenum q, GLint* v) { (void)p;(void)st;(void)ix;(void)q; *v=0; }
void glGetActiveSubroutineUniformName(GLuint p, GLenum st, GLuint ix, GLsizei n, GLsizei* l, GLchar* name) { (void)p;(void)st;(void)ix; if(l)*l=0; if(name&&n>0)name[0]=0; }
void glGetActiveSubroutineName(GLuint p, GLenum st, GLuint ix, GLsizei n, GLsizei* l, GLchar* name) { (void)p;(void)st;(void)ix; if(l)*l=0; if(name&&n>0)name[0]=0; }
void glGetUniformSubroutineuiv(GLenum st, GLint l, GLuint* v) { (void)st;(void)l; *v=0; }
void glGetUniformIndices(GLuint p, GLsizei n, const GLchar* const* names, GLuint* ix) { (void)p; for(GLsizei i=0;i<n;++i) ix[i]=0xFFFFFFFFu; (void)names; }
void glGetInternalformativ(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint* v) { (void)t;(void)inf;(void)p; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetInternalformati64v(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint64* v) { (void)t;(void)inf;(void)p; for(GLsizei i=0;i<n;++i)v[i]=0; }
void glGetMultisamplefv(GLenum p, GLuint i, GLfloat* v) { (void)p;(void)i; *v=0; }
void glGetShaderPrecisionFormat(GLenum st, GLenum p, GLint* r, GLint* v) { (void)st;(void)p; *r=0; *v=0; }
} // namespace tglmt::gl
