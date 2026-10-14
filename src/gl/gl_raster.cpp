// gl_raster.cpp — Raster state: enable/cap, blend, depth/stencil, cull, viewport,
// scissor, polygon offset/clamp (GL 4.6), clip control, line/point.
// Metal: bake vào MTLRenderPipelineDescriptor.colorAttachments / MTLDepthStencilDescriptor.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <vector>
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
// P9 viewport_array (ARB/OES/NV forwards sang core DepthRangeArray/Indexed).
// ARB_viewport_array gl.xml liệt kê NV-suffixed (quirk) — forward về core.
void glDepthRangeArraydvNV(GLuint f, GLsizei n, const GLdouble* v) { glDepthRangeArrayv(f, n, v); }
void glDepthRangeIndexeddNV(GLuint i, GLdouble n, GLdouble f) { glDepthRangeIndexed(i, n, f); }
// OES float versions → convert sang double, forward core.
void glDepthRangeArrayfvOES(GLuint f, GLsizei n, const GLfloat* v) {
    // Convert float→double (depth range cần precision — dùng thread_local buf).
    thread_local std::vector<double> tmp;
    tmp.resize((size_t)n * 2);
    for (GLsizei i = 0; i < n * 2; ++i) tmp[(size_t)i] = (double)v[i];
    glDepthRangeArrayv(f, n, tmp.data());
}
void glDepthRangeIndexedfOES(GLuint i, GLfloat n, GLfloat f) { glDepthRangeIndexed(i, (GLdouble)n, (GLdouble)f); }
void glStencilFunc(GLenum f, GLint r, GLuint m) {
    // P9: tôn trọng ActiveStencilFace (EXT_stencil_two_side). Mặc định BOTH → cả 2.
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 /*FRONT*/ || face == 0x0408 /*BOTH*/) {
        c.state.StencilFront().func = f;
        c.state.StencilFront().ref = r;
        c.state.StencilFront().valueMask = m;
    }
    if (face == 0x0405 /*BACK*/ || face == 0x0408 /*BOTH*/) {
        c.state.StencilBack().func = f;
        c.state.StencilBack().ref = r;
        c.state.StencilBack().valueMask = m;
    }
}
void glStencilFuncSeparate(GLenum face, GLenum f, GLint r, GLuint m) {
    Context& c = Context::Current();
    // Sửa bug: FRONT=0x0404, BACK=0x0405, BOTH=0x0408 (trước đây check 0x0409 sai).
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().func = f, c.state.StencilFront().ref = r, c.state.StencilFront().valueMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().func = f, c.state.StencilBack().ref = r, c.state.StencilBack().valueMask = m;
}
void glStencilOp(GLenum sf, GLenum dpf, GLenum dpp) {
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 || face == 0x0408) { auto& fr = c.state.StencilFront(); fr.sfail = sf; fr.dpfail = dpf; fr.dppass = dpp; }
    if (face == 0x0405 || face == 0x0408) { auto& bk = c.state.StencilBack(); bk.sfail = sf; bk.dpfail = dpf; bk.dppass = dpp; }
}
void glStencilOpSeparate(GLenum face, GLenum sf, GLenum dpf, GLenum dpp) {
    Context& c = Context::Current();
    // Sửa bug face check (như FuncSeparate).
    if (face == 0x0404 || face == 0x0408) { auto& fr = c.state.StencilFront(); fr.sfail = sf; fr.dpfail = dpf; fr.dppass = dpp; }
    if (face == 0x0405 || face == 0x0408) { auto& bk = c.state.StencilBack(); bk.sfail = sf; bk.dpfail = dpf; bk.dppass = dpp; }
}
void glStencilMask(GLuint m) {
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().writeMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().writeMask = m;
}
void glStencilMaskSeparate(GLenum face, GLuint m) {
    Context& c = Context::Current();
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().writeMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().writeMask = m;
}
// P9 EXT_stencil_two_side: chọn face cho StencilFunc/Op/Mask không-Separate.
void glActiveStencilFaceEXT(GLenum face) {
    Context& c = Context::Current();
    if (face != 0x0404 && face != 0x0405 && face != 0x0408) {
        c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    c.state.SetActiveStencilFace(face);
}
void glCullFace(GLenum m) { Context::Current().state.SetCullFace(m); }
void glFrontFace(GLenum m) { Context::Current().state.SetFrontFace(m); }
void glPolygonMode(GLenum f, GLenum m) { Context::Current().state.SetPolygonMode(f, m); }
// glPolygonOffset không có tham số clamp: theo convention Vulkan/Metal, clamp==0
// nghĩa là KHÔNG clamp bias. (Trước đây truyền 1e30f → giá trị rác trong shadow
// glGet + không bao giờ tới Metal vì depth bias chẳng được encode.)
void glPolygonOffset(GLfloat f, GLfloat u) { Context::Current().state.SetPolygonOffset(f, u, 0.0f); }
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
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    uint8_t m = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
    Context& c = Context::Current();
    for (int i = 0; i < 8; ++i) c.state.SetColorMask((GLuint)i, m);
}
void glColorMaski(GLuint i, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    uint8_t m = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
    if ((int)i < 8) Context::Current().state.SetColorMask(i, m);
}
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) { (void)a;(void)b;(void)c_; }
void glLogicOp(GLenum o) { Context::Current().state.SetShadow(0x0BF1, &o, 4); }
void glSampleCoverage(GLfloat v, GLboolean inv) { (void)v;(void)inv; }
void glSampleMaski(GLuint m, GLbitfield mask) { (void)m;(void)mask; }
void glMinSampleShading(GLfloat v) { Context::Current().state.SetShadow(0x8C51, &v, 4); }
void glHint(GLenum t, GLenum m) { (void)t;(void)m; }
void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }
} // namespace tglmt::gl
