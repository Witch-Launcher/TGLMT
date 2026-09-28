// gl_framebuffer.cpp — FBO/RBO → MTLRenderPassDescriptor.
// Spec §9 (Framebuffer). Metal: colorAttachments[i]/depth/stencil, load/store.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
void glGenFramebuffers(GLsizei n, GLuint* f) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Framebuffer, n, f);
    for (GLsizei i = 0; i < n; ++i) c.fbos[f[i]] = FramebufferObject{f[i]};
}
void glCreateFramebuffers(GLsizei n, GLuint* f) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Framebuffer, n, f);
    for (GLsizei i = 0; i < n; ++i) c.fbos[f[i]] = FramebufferObject{f[i]};
}
void glDeleteFramebuffers(GLsizei n, const GLuint* f) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Framebuffer, n, f);
    for (GLsizei i = 0; i < n; ++i) c.fbos.erase(f[i]);
}
GLboolean glIsFramebuffer(GLuint f) {
    return Context::Current().registry.Is(ObjectKind::Framebuffer, f) ? 1 : 0;
}
void glBindFramebuffer(GLenum t, GLuint f) {
    Context& c = Context::Current();
    if (f && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    c.state.BindFBO(t, f);
}
void glBindRenderbuffer(GLenum t, GLuint r) {
    (void)t;
    Context& c = Context::Current();
    c.state.SetShadow(0x8D41 /*RENDERBUFFER_BINDING*/, &r, 4);
}
void glGenRenderbuffers(GLsizei n, GLuint* r) { Context::Current().registry.Gen(ObjectKind::Renderbuffer, n, r); }
void glCreateRenderbuffers(GLsizei n, GLuint* r) { Context::Current().registry.Create(ObjectKind::Renderbuffer, n, r); }
void glDeleteRenderbuffers(GLsizei n, const GLuint* r) { Context::Current().registry.Delete(ObjectKind::Renderbuffer, n, r); }
GLboolean glIsRenderbuffer(GLuint r) { return Context::Current().registry.Is(ObjectKind::Renderbuffer, r) ? 1 : 0; }
void glRenderbufferStorage(GLenum t, GLenum inf, GLsizei w, GLsizei h) { (void)t;(void)inf;(void)w;(void)h; }
void glRenderbufferStorageMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h) { (void)t;(void)s;(void)inf;(void)w;(void)h; }
void glNamedRenderbufferStorage(GLuint r, GLenum inf, GLsizei w, GLsizei h) { (void)r;(void)inf;(void)w;(void)h; }
void glNamedRenderbufferStorageMultisample(GLuint r, GLsizei s, GLenum inf, GLsizei w, GLsizei h) { (void)r;(void)s;(void)inf;(void)w;(void)h; }
void glFramebufferRenderbuffer(GLenum t, GLenum a, GLenum rt, GLuint r) { (void)t;(void)a;(void)rt;(void)r; }
void glNamedFramebufferRenderbuffer(GLuint f, GLenum a, GLenum rt, GLuint r) { (void)f;(void)a;(void)rt;(void)r; }
void glFramebufferTexture(GLenum t, GLenum a, GLuint x, GLint l) { (void)t;(void)a;(void)x;(void)l; }
void glFramebufferTexture1D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l) { (void)t;(void)a;(void)tx;(void)x;(void)l; }
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l) {
    Context& c = Context::Current();
    // Spec §9.2: attach vào DRAW framebuffer hiện bind; FBO 0 (default) không attach được.
    GLuint fbo = c.state.BoundDrawFBO();
    if (fbo == 0) { c.errors.Record(0x0502); return; } // INVALID_OPERATION trên default FB
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    auto& f = it->second;
    if (a == 0x8CE0) f.colorTex[0] = x; // COLOR_ATTACHMENT0
    else if (a >= 0x8CE0 && a <= 0x8CE7) f.colorTex[a - 0x8CE0] = x; // COLOR_ATTACHMENTn
    else if (a == 0x8D00) f.depthTex = x;
    else if (a == 0x8D20) f.stencilTex = x;
    else if (a == 0x821A) f.depthStencilTex = x; // DEPTH_STENCIL_ATTACHMENT
    else { c.errors.Record(0x0500); return; }
    auto txt = c.textures.find(x);
    if (txt != c.textures.end()) { f.w = txt->second.w; f.h = txt->second.h; }
    (void)t; (void)tx; (void)l;
}
void glFramebufferTexture3D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l, GLint z) { (void)t;(void)a;(void)tx;(void)x;(void)l;(void)z; }
void glFramebufferTextureLayer(GLenum t, GLenum a, GLuint x, GLint l, GLint layer) { (void)t;(void)a;(void)x;(void)l;(void)layer; }
// Helper attach tôn trọng fbo chỉ định (fix bug bỏ qua `f` ở Named variants).
static bool AttachToFBO(Context& c, GLuint fbo, GLenum attach, GLuint tex) {
    if (fbo == 0) { c.errors.Record(0x0502); return false; } // default FB không attach
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return false; }
    auto& f = it->second;
    if (attach == 0x8CE0) f.colorTex[0] = tex;
    else if (attach >= 0x8CE0 && attach <= 0x8CE7) f.colorTex[attach - 0x8CE0] = tex;
    else if (attach == 0x8D00) f.depthTex = tex;
    else if (attach == 0x8D20) f.stencilTex = tex;
    else if (attach == 0x821A) f.depthStencilTex = tex;
    else { c.errors.Record(0x0500); return false; }
    auto txt = c.textures.find(tex);
    if (txt != c.textures.end() && tex) { f.w = txt->second.w; f.h = txt->second.h; }
    return true;
}
void glNamedFramebufferTexture(GLuint f, GLenum a, GLuint x, GLint l) { (void)l; AttachToFBO(Context::Current(), f, a, x); }
void glNamedFramebufferTextureLayer(GLuint f, GLenum a, GLuint x, GLint l, GLint layer) { (void)f;(void)a;(void)x;(void)l;(void)layer; }
void glFramebufferParameteri(GLenum t, GLenum p, GLint v) { (void)t; Context::Current().state.SetShadow(p, &v, 4); }
void glNamedFramebufferParameteri(GLuint f, GLenum p, GLint v) { (void)f; Context::Current().state.SetShadow(p, &v, 4); }
GLenum glCheckFramebufferStatus(GLenum t) {
    (void)t; return 0x8CD5; // FRAMEBUFFER_COMPLETE (Null backend luôn complete nếu attach hợp lệ)
}
GLenum glCheckNamedFramebufferStatus(GLuint f, GLenum t) { (void)f; return glCheckFramebufferStatus(t); }
void glDrawBuffer(GLenum b) {
    Context& c = Context::Current();
    GLuint fbo = c.state.BoundDrawFBO();
    if (fbo == 0) { c.state.SetShadow(0x0C01, &b, 4); return; }
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    it->second.drawBuffers = {b};
}
void glDrawBuffers(GLsizei n, const GLenum* b) {
    Context& c = Context::Current();
    if (n < 0 || n > 8) { c.errors.Record(0x0501); return; }
    GLuint fbo = c.state.BoundDrawFBO();
    if (fbo == 0) { // default FB: chỉ lưu shadow (single buffer)
        if (n > 0) c.state.SetShadow(0x0C01, &b[0], 4);
        return;
    }
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    it->second.drawBuffers.assign(b, b + n);
}
void glNamedFramebufferDrawBuffer(GLuint f, GLenum b) {
    Context& c = Context::Current();
    auto it = c.fbos.find(f);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    it->second.drawBuffers = {b};
}
void glNamedFramebufferDrawBuffers(GLuint f, GLsizei n, const GLenum* b) {
    Context& c = Context::Current();
    if (n < 0 || n > 8) { c.errors.Record(0x0501); return; }
    auto it = c.fbos.find(f);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    it->second.drawBuffers.assign(b, b + n);
}
void glReadBuffer(GLenum b) { Context::Current().state.SetShadow(0x0C02 /*READ_BUFFER*/, &b, 4); }
void glNamedFramebufferReadBuffer(GLuint f, GLenum b) { (void)f; glReadBuffer(b); }
void glBlitFramebuffer(GLint s0, GLint s1, GLint s2, GLint s3, GLint d0, GLint d1, GLint d2, GLint d3, GLbitfield m, GLenum f) {
    Context& c = Context::Current();
    GLuint readFbo = c.state.BoundReadFBO();
    GLuint drawFbo = c.state.BoundDrawFBO();
    // Default FB (0) tham gia blit: headless/present path, hiện sync shadow + log (M5c present-blit).
    if (readFbo == 0 || drawFbo == 0) {
        c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: default-FB blit giữ shadow (present xử lý ở SwapBuffers)");
        return;
    }
    auto rit = c.fbos.find(readFbo);
    auto wit = c.fbos.find(drawFbo);
    if (rit == c.fbos.end() || wit == c.fbos.end()) { c.errors.Record(0x0502); return; }
    int sw = s2 - s0, sh = s3 - s1, dw = d2 - d0, dh = d3 - d1;
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) { c.errors.Record(0x0501); return; }
    bool scaled = (sw != dw || sh != dh);
    // LINEAR filter với scale cần sampled-quad (M5c); NEAREST scale nguyên có thể blit lặp — hiện fallback CPU.
    if ((m & 0x00004000u) && rit->second.colorTex.count(0) && wit->second.colorTex.count(0)) {
        auto rs = c.textures.find(rit->second.colorTex[0]);
        auto ws = c.textures.find(wit->second.colorTex[0]);
        if (rs != c.textures.end() && ws != c.textures.end() && rs->second.gpu && ws->second.gpu) {
            if (!scaled && f == 0x2600 /*NEAREST*/) {
                // cùng size + NEAREST → blitCopy GPU thật (A11 TBDR copy, không resolve scale)
                uint32_t sx = s0 < 0 ? 0 : (uint32_t)s0;
                uint32_t sy = s1 < 0 ? 0 : (uint32_t)s1;
                if (c.device->blitCopy(rs->second.gpu.get(), ws->second.gpu.get(),
                                       sx, sy, (uint32_t)sw, (uint32_t)sh, (uint32_t)d0, (uint32_t)d1)) {
                    // sync shadow CPU (đúng GL: GetTexImage/ReadPixels sau blit thấy mới)
                    ws->second.pixels = rs->second.pixels;
                    return;
                }
                c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: GPU blit fail, fallback CPU shadow");
            } else {
                c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: scaled/LINEAR fallback CPU shadow (M5c sampled-quad)");
            }
        }
        // CPU shadow copy (đúng khi cùng format RGBA8; khác size thì nearest đơn giản)
        auto rs2 = c.textures.find(rit->second.colorTex[0]);
        auto ws2 = c.textures.find(wit->second.colorTex[0]);
        if (rs2 != c.textures.end() && ws2 != c.textures.end() && !rs2->second.pixels.empty()) {
            // đảm bảo dst đủ chỗ
            if (ws2->second.pixels.size() < rs2->second.pixels.size())
                ws2->second.pixels.resize(rs2->second.pixels.size(), 0);
            if (!scaled) ws2->second.pixels = rs2->second.pixels;
            else {
                // nearest scale CPU (đủ cho vanilla framebuffer copy khác res khi xoay)
                uint32_t rw = rs2->second.w, rh = rs2->second.h;
                uint32_t ww = ws2->second.w, wh = ws2->second.h;
                if (rw && rh && ww && wh && rs2->second.pixels.size() >= (size_t)rw * rh * 4 &&
                    ws2->second.pixels.size() >= (size_t)ww * wh * 4) {
                    for (uint32_t y = 0; y < (uint32_t)dh && (uint32_t)(d1 + y) < wh; ++y)
                        for (uint32_t x = 0; x < (uint32_t)dw && (uint32_t)(d0 + x) < ww; ++x) {
                            uint32_t sxp = (uint32_t)s0 + x * (uint32_t)sw / (uint32_t)dw;
                            uint32_t syp = (uint32_t)s1 + y * (uint32_t)sh / (uint32_t)dh;
                            if (sxp >= rw || syp >= rh) continue;
                            memcpy(ws2->second.pixels.data() + (((size_t)(d1 + y) * ww + (d0 + x)) * 4),
                                   rs2->second.pixels.data() + ((size_t)syp * rw + sxp) * 4, 4);
                        }
                }
            }
            // sync GPU dst từ shadow (đúng pixels cho draw tiếp theo)
            if (ws2->second.gpu && !ws2->second.pixels.empty())
                c.device->updateTexture(ws2->second.gpu.get(), 0, 0, ws2->second.w, ws2->second.h,
                                        ws2->second.pixels.data(), (size_t)ws2->second.w * 4);
        }
    }
    if (m & 0x00000100u) { // DEPTH_BUFFER_BIT
        auto rs = rit->second.depthTex ? c.textures.find(rit->second.depthTex) : c.textures.end();
        auto ws = wit->second.depthTex ? c.textures.find(wit->second.depthTex) : c.textures.end();
        if (rs != c.textures.end() && ws != c.textures.end() && rs->second.gpu && ws->second.gpu && !scaled)
            c.device->blitCopy(rs->second.gpu.get(), ws->second.gpu.get(),
                               (uint32_t)(s0 < 0 ? 0 : s0), (uint32_t)(s1 < 0 ? 0 : s1),
                               (uint32_t)sw, (uint32_t)sh, (uint32_t)d0, (uint32_t)d1);
    }
}
void glBlitNamedFramebuffer(GLuint r, GLuint d, GLint s0, GLint s1, GLint s2, GLint s3, GLint d0, GLint d1, GLint d2, GLint d3, GLbitfield m, GLenum f) {
    (void)r;(void)d; glBlitFramebuffer(s0,s1,s2,s3,d0,d1,d2,d3,m,f);
}
void glInvalidateFramebuffer(GLenum t, GLsizei n, const GLenum* a) { (void)t;(void)n;(void)a; }
void glInvalidateSubFramebuffer(GLenum t, GLsizei n, const GLenum* a, GLint x, GLint y, GLsizei w, GLsizei h) { (void)t;(void)n;(void)a;(void)x;(void)y;(void)w;(void)h; }
void glInvalidateNamedFramebufferData(GLuint f, GLsizei n, const GLenum* a) { (void)f;(void)n;(void)a; }
void glInvalidateNamedFramebufferSubData(GLuint f, GLsizei n, const GLenum* a, GLint x, GLint y, GLsizei w, GLsizei h) { (void)f;(void)n;(void)a;(void)x;(void)y;(void)w;(void)h; }
void glClear(GLbitfield m) {
    Context& c = Context::Current();
    // Metal: renderPass loadAction=Clear + clearColor/Depth/Stencil — Null: ghi shadow để test đọc
    c.state.SetShadow(0x0B00 /*CLEAR_STATE_MARKER*/, &m, 4);
    c.applePendingClear = true; // M5b: draw Apple kế tiếp clear, sau đó LOAD
    c.appleClearMask |= m;
}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    Context& c = Context::Current();
    c.clearColor[0]=r; c.clearColor[1]=g; c.clearColor[2]=b; c.clearColor[3]=a;
}
void glClearDepth(GLdouble d) { Context::Current().clearDepth = d; }
void glClearDepthf(GLfloat d) { Context::Current().clearDepth = d; }
void glClearStencil(GLint s) { Context::Current().clearStencil = s; }
void glClearBufferfv(GLenum b, GLint d, const GLfloat* v) {
    Context& c = Context::Current();
    // COLOR (0x1800) với drawbuffer d: set clearColor shadow + pending clear cho draw kế tiếp.
    // Metal loadActionClear chỉ clear toàn target 1 lần — per-attachment clear MRT cần
    // clear từng pass (M5c). Hiện đúng cho single-target vanilla (d==0).
    if (b == 0x1800 /*COLOR*/ && v) {
        if (d == 0) {
            c.clearColor[0] = v[0]; c.clearColor[1] = v[1]; c.clearColor[2] = v[2];
            c.clearColor[3] = (d >= 0 && v) ? v[3] : 1.0f;
            if (v) { c.clearColor[0] = v[0]; c.clearColor[1] = v[1]; c.clearColor[2] = v[2]; c.clearColor[3] = v[3]; }
            c.applePendingClear = true;
            c.appleClearMask |= 0x00004000u;
        } else {
            c.LogDebug(0, 0, 0, 0, "glClearBufferfv COLOR drawbuffer>0 giữ shadow (MRT clear M5c)");
        }
        return;
    }
    if (b == 0x1801 /*DEPTH*/ && v) {
        c.clearDepth = v[0];
        c.applePendingClear = true;
        c.appleClearMask |= 0x00000100u;
        return;
    }
    if (b == 0x1802 /*STENCIL*/ && v) { c.clearStencil = (GLint)v[0]; return; }
    (void)d;
}
void glClearBufferiv(GLenum b, GLint d, const GLint* v) {
    GLfloat f[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearBufferfv(b, d, f);
}
void glClearBufferuiv(GLenum b, GLint d, const GLuint* v) {
    GLfloat f[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearBufferfv(b, d, f);
}
void glClearBufferfi(GLenum b, GLint d, GLfloat dep, GLint st) {
    Context& c = Context::Current();
    (void)b; (void)d;
    c.clearDepth = dep; c.clearStencil = st;
    c.applePendingClear = true;
    c.appleClearMask |= 0x00000100u | 0x00000400u;
}
void glClearNamedFramebufferfv(GLuint f, GLenum b, GLint d, const GLfloat* v) {
    // DSA: clear FBO chỉ định — hiện tôn trọng f (bind tạm), đúng hơn bản cũ bỏ qua.
    Context& c = Context::Current();
    GLuint saved = c.state.BoundDrawFBO();
    if (f != saved) {
        auto it = c.fbos.find(f);
        if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
        c.state.BindFBO(0x8CA9 /*DRAW_FRAMEBUFFER*/, f);
        glClearBufferfv(b, d, v);
        c.state.BindFBO(0x8CA9, saved);
        return;
    }
    glClearBufferfv(b, d, v);
}
void glClearNamedFramebufferiv(GLuint f, GLenum b, GLint d, const GLint* v) {
    GLfloat fl[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearNamedFramebufferfv(f, b, d, fl);
}
void glClearNamedFramebufferuiv(GLuint f, GLenum b, GLint d, const GLuint* v) {
    GLfloat fl[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearNamedFramebufferfv(f, b, d, fl);
}
void glClearNamedFramebufferfi(GLuint f, GLenum b, GLint d, GLfloat dep, GLint st) {
    Context& c = Context::Current();
    GLuint saved = c.state.BoundDrawFBO();
    if (f != saved) {
        auto it = c.fbos.find(f);
        if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
        c.state.BindFBO(0x8CA9, f);
        glClearBufferfi(b, d, dep, st);
        c.state.BindFBO(0x8CA9, saved);
        return;
    }
    glClearBufferfi(b, d, dep, st);
}
} // namespace tglmt::gl
