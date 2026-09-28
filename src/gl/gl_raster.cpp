// gl_raster.cpp — Raster state: enable/cap, blend, depth/stencil, cull, viewport,
// scissor, polygon offset/clamp (GL 4.6), clip control, line/point.
// Metal: bake vào MTLRenderPipelineDescriptor.colorAttachments / MTLDepthStencilDescriptor.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
void glEnable(GLenum cap) { Context::Current().state.SetEnabled(cap, true); }
void glDisable(GLenum cap) { Context::Current().state.SetEnabled(cap, false); }
void glEnablei(GLenum t, GLuint i) { Context::Current().state.SetEnabled(t, true, i); }
void glDisablei(GLenum t, GLuint i) { Context::Current().state.SetEnabled(t, false, i); }
GLboolean glIsEnabled(GLenum cap) { return Context::Current().state.IsEnabled(cap) ? 1 : 0; }
GLboolean glIsEnabledi(GLenum t, GLuint i) { return Context::Current().state.IsEnabled(t, i) ? 1 : 0; }
void glBlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { Context::Current().state.SetBlendColor(r, g, b, a); }
void glBlendEquation(GLenum m) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.rgbEq = m; b.alphaEq = m; }
}
void glBlendEquationSeparate(GLenum r, GLenum a) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.rgbEq = r; b.alphaEq = a; }
}
void glBlendEquationi(GLuint i, GLenum m) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].rgbEq = m; B[i].alphaEq = m; }
}
void glBlendEquationSeparatei(GLuint i, GLenum r, GLenum a) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].rgbEq = r; B[i].alphaEq = a; }
}
void glBlendFunc(GLenum s, GLenum d) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.srcRGB = s; b.dstRGB = d; b.srcAlpha = s; b.dstAlpha = d; }
}
void glBlendFuncSeparate(GLenum srgb, GLenum drgb, GLenum sa, GLenum da) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.srcRGB = srgb; b.dstRGB = drgb; b.srcAlpha = sa; b.dstAlpha = da; }
}
void glBlendFunci(GLuint i, GLenum s, GLenum d) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].srcRGB = s; B[i].dstRGB = d; B[i].srcAlpha = s; B[i].dstAlpha = d; }
}
void glBlendFuncSeparatei(GLuint i, GLenum srgb, GLenum drgb, GLenum sa, GLenum da) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].srcRGB = srgb; B[i].dstRGB = drgb; B[i].srcAlpha = sa; B[i].dstAlpha = da; }
}
void glDepthFunc(GLenum f) { Context::Current().state.Depth().func = f; }
void glDepthMask(GLboolean m) { Context::Current().state.Depth().writeMask = m ? true : false; }
void glDepthRange(GLdouble n, GLdouble f) { Context::Current().state.SetDepthRange(n, f); }
void glDepthRangef(GLfloat n, GLfloat f) { Context::Current().state.SetDepthRangef(n, f, 0); }
void glDepthRangeArrayv(GLuint f, GLsizei n, const GLdouble* v) {
    for (GLsizei i = 0; i < n; ++i) Context::Current().state.SetDepthRange(v[2*i], v[2*i+1], f + i);
}
void glDepthRangeIndexed(GLuint i, GLdouble n, GLdouble f) { Context::Current().state.SetDepthRange(n, f, i); }
void glStencilFunc(GLenum f, GLint r, GLuint m) {
    Context::Current().state.StencilFront().func = f;
    Context::Current().state.StencilFront().ref = r;
    Context::Current().state.StencilFront().valueMask = m;
    Context::Current().state.StencilBack() = Context::Current().state.StencilFront();
}
void glStencilFuncSeparate(GLenum face, GLenum f, GLint r, GLuint m) {
    Context& c = Context::Current();
    if (face == 0x0408 || face == 0x0404) c.state.StencilFront().func = f, c.state.StencilFront().ref = r, c.state.StencilFront().valueMask = m;
    if (face == 0x0409 || face == 0x0404) c.state.StencilBack().func = f, c.state.StencilBack().ref = r, c.state.StencilBack().valueMask = m;
}
void glStencilOp(GLenum sf, GLenum dpf, GLenum dpp) {
    auto& fr = Context::Current().state.StencilFront();
    fr.sfail = sf; fr.dpfail = dpf; fr.dppass = dpp;
    Context::Current().state.StencilBack() = fr;
}
void glStencilOpSeparate(GLenum face, GLenum sf, GLenum dpf, GLenum dpp) {
    Context& c = Context::Current();
    auto apply = [&](StencilFace& s){ s.sfail = sf; s.dpfail = dpf; s.dppass = dpp; };
    if (face == 0x0408 || face == 0x0404) apply(c.state.StencilFront());
    if (face == 0x0409 || face == 0x0404) apply(c.state.StencilBack());
}
void glStencilMask(GLuint m) {
    Context::Current().state.StencilFront().writeMask = m;
    Context::Current().state.StencilBack().writeMask = m;
}
void glStencilMaskSeparate(GLenum face, GLuint m) {
    Context& c = Context::Current();
    if (face == 0x0408 || face == 0x0404) c.state.StencilFront().writeMask = m;
    if (face == 0x0409 || face == 0x0404) c.state.StencilBack().writeMask = m;
}
void glCullFace(GLenum m) { Context::Current().state.SetCullFace(m); }
void glFrontFace(GLenum m) { Context::Current().state.SetFrontFace(m); }
void glPolygonMode(GLenum f, GLenum m) { Context::Current().state.SetPolygonMode(f, m); }
void glPolygonOffset(GLfloat f, GLfloat u) { Context::Current().state.SetPolygonOffset(f, u, 1e30f); }
void glPolygonOffsetClamp(GLfloat f, GLfloat u, GLfloat c) { Context::Current().state.SetPolygonOffset(f, u, c); }
void glLineWidth(GLfloat w) { Context::Current().state.SetLineWidth(w); }
void glPointSize(GLfloat s) { Context::Current().state.SetShadow(0x0B11, &s, 4); }
void glPointParameterf(GLenum p, GLfloat v) { Context::Current().state.SetShadow(p, &v, 4); }
void glPointParameterfv(GLenum p, const GLfloat* v) { Context::Current().state.SetShadow(p, v, 4); }
void glPointParameteri(GLenum p, GLint v) { Context::Current().state.SetShadow(p, &v, 4); }
void glPointParameteriv(GLenum p, const GLint* v) { Context::Current().state.SetShadow(p, v, 4); }
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { Context::Current().state.SetViewport(0, (float)x, (float)y, (float)w, (float)h); }
void glViewportArrayv(GLuint f, GLsizei n, const GLfloat* v) {
    for (GLsizei i = 0; i < n; ++i) Context::Current().state.SetViewport(f + i, v[4*i], v[4*i+1], v[4*i+2], v[4*i+3]);
}
void glViewportIndexedf(GLuint i, GLfloat x, GLfloat y, GLfloat w, GLfloat h) { Context::Current().state.SetViewport(i, x, y, w, h); }
void glViewportIndexedfv(GLuint i, const GLfloat* v) { Context::Current().state.SetViewport(i, v[0], v[1], v[2], v[3]); }
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    Context::Current().state.SetScissor(x, y, w, h);
}
void glScissorArrayv(GLuint f, GLsizei n, const GLint* v) {
    if (n > 0 && v) Context::Current().state.SetScissor(v[0], v[1], v[2], v[3]);
    (void)f;
}
void glScissorIndexed(GLuint i, GLint x, GLint y, GLsizei w, GLsizei h) {
    if (i == 0) Context::Current().state.SetScissor(x, y, w, h);
}
void glScissorIndexedv(GLuint i, const GLint* v) {
    if (i == 0 && v) Context::Current().state.SetScissor(v[0], v[1], v[2], v[3]);
}
void glClipControl(GLenum o, GLenum d) { Context::Current().state.SetClipControl(o, d); }
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) { (void)r;(void)g;(void)b;(void)a; }
void glColorMaski(GLuint i, GLboolean r, GLboolean g, GLboolean b, GLboolean a) { (void)i;(void)r;(void)g;(void)b;(void)a; }
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) { (void)a;(void)b;(void)c_; }
void glLogicOp(GLenum o) { Context::Current().state.SetShadow(0x0BF1, &o, 4); }
void glSampleCoverage(GLfloat v, GLboolean inv) { (void)v;(void)inv; }
void glSampleMaski(GLuint m, GLbitfield mask) { (void)m;(void)mask; }
void glMinSampleShading(GLfloat v) { Context::Current().state.SetShadow(0x8C51, &v, 4); }
void glHint(GLenum t, GLenum m) { (void)t;(void)m; }
void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }
} // namespace tglmt::gl
