// gl_sampler.cpp — Samplers → MTLSamplerDescriptor/MTLSamplerState.
// ARB_texture_filter_anisotropic → maxAnisotropy.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
void glGenSamplers(GLsizei n, GLuint* s) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Sampler, n, s);
    for (GLsizei i = 0; i < n; ++i) c.samplers[s[i]] = SamplerObject{s[i]};
}
void glCreateSamplers(GLsizei n, GLuint* s) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Sampler, n, s);
    for (GLsizei i = 0; i < n; ++i) c.samplers[s[i]] = SamplerObject{s[i]};
}
void glDeleteSamplers(GLsizei n, const GLuint* s) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Sampler, n, s);
    for (GLsizei i = 0; i < n; ++i) c.samplers.erase(s[i]);
}
GLboolean glIsSampler(GLuint s) {
    return Context::Current().registry.Is(ObjectKind::Sampler, s) ? 1 : 0;
}
void glBindSampler(GLuint u, GLuint s) { Context::Current().state.BindSampler(u, s); }
void glBindSamplers(GLuint f, GLsizei n, const GLuint* s) {
    for (GLsizei i = 0; i < n; ++i) glBindSampler(f + i, s ? s[i] : 0);
}
static void SetS(GLuint s, GLenum p, GLint v) {
    Context& c = Context::Current();
    auto it = c.samplers.find(s);
    if (it == c.samplers.end()) { c.errors.Record(0x0502); return; }
    it->second.iparams[p] = v;
}
void glSamplerParameteri(GLuint s, GLenum p, GLint v) { SetS(s, p, v); }
void glSamplerParameterf(GLuint s, GLenum p, GLfloat v) {
    Context& c = Context::Current();
    auto it = c.samplers.find(s);
    if (it == c.samplers.end()) { c.errors.Record(0x0502); return; }
    it->second.fparams[p] = v;
}
void glSamplerParameteriv(GLuint s, GLenum p, const GLint* v) { SetS(s, p, v[0]); }
void glSamplerParameterfv(GLuint s, GLenum p, const GLfloat* v) { glSamplerParameterf(s, p, v[0]); }
void glSamplerParameterIiv(GLuint s, GLenum p, const GLint* v) { SetS(s, p, v[0]); }
void glSamplerParameterIuiv(GLuint s, GLenum p, const GLuint* v) { SetS(s, p, (GLint)v[0]); }
} // namespace tglmt::gl
