// gl_readpixels_dsa.cpp — vét 47 hàm còn lại:
// ReadPixels (readback FBO → blit/getTexture), TexBuffer/View (buffer-backed texture),
// TransformFeedbackVaryings, VertexArray* DSA, VertexAttrib s/b-variants, misc state.
// Spec: glspec46.core.pdf §18.2 (ReadPixels), §8.9 (Buffer Textures), §10.3 (VAO DSA).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>
#include <vector>
using namespace tglmt;

namespace tglmt::gl {
// --- ReadPixels: đọc từ READ framebuffer (spec §18.2). GPU path khi có device thật:
// bọc texture/default-target rồi readback (toàn bộ vùng; x/y/offset vùng M5b tinh chỉnh).
// Chỉ RGBA/UNSIGNED_BYTE (format/type khác → INVALID_OPERATION trung thực, M5b mở rộng).
void glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, void* p) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; }
    if (!p) { c.errors.Record(0x0501); return; }
    if ((f != 0x1908 /*RGBA*/ && f != 0x80E1 /*BGRA*/) || ty != 0x1401 /*UNSIGNED_BYTE*/) {
        c.errors.Record(0x0502); // M5b mở rộng FLOAT/DEPTH reads
        return;
    }
    (void)x; (void)y;
    GLuint readFbo = c.state.BoundReadFBO();
    const TextureObject* tex = nullptr;
    if (readFbo != 0) {
        auto fit = c.fbos.find(readFbo);
        if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        auto cit = fit->second.colorTex.find(0); // READ_BUFFER đơn giản: attachment 0
        if (cit == fit->second.colorTex.end()) { c.errors.Record(0x0502); return; }
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end()) { c.errors.Record(0x0502); return; }
        tex = &tit->second;
    }
    // GPU path: bọc rồi readback toàn target (x/y/w/h đầy đủ ở M5b)
    if (c.device && !c.device->isNull()) {
        c.device->commitAndWait(); // xả draws NoWait trước khi đọc (đúng, không stale)
        std::shared_ptr<metal::IRenderTarget> tgt;
        if (tex) {
            if (!tex->gpu) { c.errors.Record(0x0502); return; }
            tgt = c.device->wrapAsTarget(tex->gpu.get(), nullptr);
        } else {
            tgt = c.device->defaultRenderTarget();
        }
        if (tgt) {
            size_t need = (size_t)w * h * 4;
            std::vector<uint8_t> full((size_t)tgt->width() * tgt->height() * 4, 0);
            if (tgt->readback(full.data(), (size_t)tgt->width() * 4)) {
                // GL origin bottom-left vs Metal top-left: lật hàng (đúng spec §18.2).
                size_t tw = tgt->width(), th = tgt->height();
                for (GLsizei r = 0; r < h; ++r) {
                    GLsizei srcRow = (GLsizei)th - 1 - (y + r);
                    if (srcRow < 0 || (size_t)srcRow >= th) {
                        memset((uint8_t*)p + (size_t)r * w * 4, 0, (size_t)w * 4);
                        continue;
                    }
                    size_t copyW = std::min((size_t)w, tw > (size_t)x ? tw - (size_t)x : 0);
                    if (copyW)
                        memcpy((uint8_t*)p + (size_t)r * w * 4,
                               full.data() + ((size_t)srcRow * tw + (size_t)x) * 4, copyW * 4);
                }
                (void)need;
                return;
            }
        }
        // GPU readback thất bại → rơi xuống shadow (ghi log, không im lặng)
        c.LogDebug(0, 0, 0, 0, "glReadPixels: GPU readback fail, dùng shadow");
    }
    // Shadow path (Null backend hoặc fallback)
    if (tex) {
        size_t k = std::min(tex->pixels.size(), (size_t)w * h * 4);
        memcpy(p, tex->pixels.data(), k);
        return;
    }
    // default framebuffer trên Null: trả clear color
    uint8_t px[4] = {(uint8_t)(c.clearColor[0]*255), (uint8_t)(c.clearColor[1]*255),
                     (uint8_t)(c.clearColor[2]*255), (uint8_t)(c.clearColor[3]*255)};
    for (GLsizei i = 0; i < w * h; ++i) memcpy((uint8_t*)p + i * 4, px, 4);
}
void glReadnPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, GLsizei n, void* p) {
    if (n < 0) { Context::Current().errors.Record(0x0501); return; }
    // robustness: KHR_robustness yêu cầu không ghi quá bufSize — ở đây RGBA8 ⇒ cần w*h*4
    if ((size_t)n < (size_t)w * h * 4) { Context::Current().errors.Record(0x0502); return; }
    glReadPixels(x, y, w, h, f, ty, p);
}
// --- misc state ---
void glClampColor(GLenum t, GLenum cl) { Context::Current().state.SetShadow(t, &cl, 4); }
void glProvokingVertex(GLenum m) { Context::Current().state.SetShadow(0x8E4F /*PROVOKING_VERTEX*/, &m, 4); }
void glTextureBarrier() {} // Metal: encoder barrier — Null: no-op (đúng vì không có hazard CPU)
void glReleaseShaderCompiler() {}
void glShaderBinary(GLsizei n, const GLuint* s, GLenum f, const void* b, GLsizei l) {
    (void)n;(void)s;(void)f;(void)b;(void)l;
    Context::Current().LogDebug(0,0,0,0,"glShaderBinary: binary format lạ → dùng glShaderSource thay thế");
}
void glTexImage2DMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h, GLboolean f) {
    (void)t;(void)s;(void)inf;(void)w;(void)h;(void)f;
}
void glTexImage3DMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h, GLsizei d, GLboolean f) {
    (void)t;(void)s;(void)inf;(void)w;(void)h;(void)d;(void)f;
}
void glValidateProgramPipeline(GLuint p) { (void)p; }
GLuint glGetDebugMessageLog(GLuint n, GLsizei s, GLenum* src, GLenum* ty, GLuint* ids, GLenum* sev, GLsizei* len, GLchar* log) {
    (void)n;(void)s;(void)src;(void)ty;(void)ids;(void)sev;(void)len;(void)log; return 0;
}
void glGetProgramPipelineiv(GLuint p, GLenum q, GLint* v) { (void)p;(void)q; *v=0; }
void glGetProgramPipelineInfoLog(GLuint p, GLsizei n, GLsizei* l, GLchar* log) {
    (void)p; if(l)*l=0; if(log&&n>0)log[0]=0;
}
// --- buffer textures: Metal buffer-backed texture (newTextureWithBuffer) ---
static void BindBufTex(GLuint tex, GLenum inf, GLuint buf) {
    Context& c = Context::Current();
    auto itt = c.textures.find(tex);
    auto itb = c.buffers.find(buf);
    if (itt == c.textures.end()) { c.errors.Record(0x0502); return; }
    itt->second.internalFormat = inf;
    if (itb == c.buffers.end()) return;
    itt->second.pixels = itb->second.data; // view CPU
    // GPU: R32I/R32UI/R32F → texture int Nx1 để shader .read(index).
    // Format khác: shadow only (đủ cho probe, P1 mở rộng).
    if (inf == 0x8235 || inf == 0x8236 || inf == 0x822E) {
        size_t n = itb->second.data.size() / 4;
        if (n == 0) n = 1;
        itt->second.w = (uint32_t)n;
        itt->second.h = 1;
        std::vector<uint8_t> tmp(n * 4, 0);
        memcpy(tmp.data(), itb->second.data.data(),
               std::min(tmp.size(), itb->second.data.size()));
        itt->second.pixels = tmp;
        itt->second.gpu = c.device->newTextureWithBytes(
            (uint32_t)n, 1, itt->second.internalFormat == 0x8235 ? metal::PixelFormat::R32Sint
                         : itt->second.internalFormat == 0x8236 ? metal::PixelFormat::R32Uint
                                                                 : metal::PixelFormat::R32Float,
            tmp.data(), n * 4);
        if (!itt->second.gpu)
            c.LogDebug(0, 0, 0, 0, "glTexBuffer: GPU int texture fail, shadow only");
    }
}
void glTexBuffer(GLenum t, GLenum inf, GLuint b) {
    Context& c = Context::Current();
    for (auto& [id, tx] : c.textures) if (tx.target == t) { BindBufTex(id, inf, b); return; }
    c.errors.Record(0x0502);
}
void glTexBufferRange(GLenum t, GLenum inf, GLuint b, GLintptr o, GLsizeiptr s) {
    // Range: slice buffer [o, o+s) trước khi bind (CloudFaces dùng full nên ít gặp).
    Context& c = Context::Current();
    if (o < 0 || s < 0) { c.errors.Record(0x0501); return; }
    if ((o != 0 || s != 0)) {
        auto itb = c.buffers.find(b);
        if (itb == c.buffers.end()) { c.errors.Record(0x0502); return; }
        if ((size_t)o > itb->second.data.size()) { c.errors.Record(0x0501); return; }
        size_t len = s ? std::min((size_t)s, itb->second.data.size() - (size_t)o)
                       : itb->second.data.size() - (size_t)o;
        // bind bản slice qua buffer tạm nội bộ để tái dùng BindBufTex
        GLuint tmp = 0;
        c.registry.Create(ObjectKind::Buffer, 1, &tmp);
        c.buffers[tmp].data.assign(itb->second.data.begin() + o,
                                   itb->second.data.begin() + o + len);
        c.buffers[tmp].gpu = c.device->newBufferWithBytes(c.buffers[tmp].data.data(), len,
                                                          metal::StorageMode::Shared);
        glTexBuffer(t, inf, tmp);
        return;
    }
    glTexBuffer(t, inf, b);
}
void glTextureBuffer(GLuint t, GLenum inf, GLuint b) { BindBufTex(t, inf, b); }
void glTextureBufferRange(GLuint t, GLenum inf, GLuint b, GLintptr o, GLsizeiptr s) {
    (void)o; (void)s; BindBufTex(t, inf, b);
}
void glTextureView(GLuint t, GLenum tgt, GLuint orig, GLenum inf, GLuint ml, GLuint nl, GLuint mlay, GLuint nlay) {
    Context& c = Context::Current();
    auto ito = c.textures.find(orig);
    auto itt = c.textures.find(t);
    if (ito == c.textures.end() || itt == c.textures.end()) { c.errors.Record(0x0502); return; }
    itt->second = ito->second; itt->second.id = t; itt->second.target = tgt;
    itt->second.internalFormat = inf;
    (void)ml; (void)nl; (void)mlay; (void)nlay;
}
// --- XFB varyings ---
void glTransformFeedbackVaryings(GLuint p, GLsizei n, const GLchar* const* v, GLenum m) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (m != 0x8C8C && m != 0x8C8D) { c.errors.Record(0x0500); return; } // INTERLEAVED/SEPARATE_ATTRIBS
    it->second.xfbVaryings.clear();
    for (GLsizei i = 0; i < n && v; ++i) it->second.xfbVaryings.emplace_back(v[i] ? v[i] : "");
    it->second.xfbBufferMode = m;
}
// --- VAO DSA ---
void glVertexArrayAttribBinding(GLuint v, GLuint ai, GLuint bi) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (ai >= 16) { c.errors.Record(0x0501); return; }
    if (bi >= it->second.bindings.size()) { c.errors.Record(0x0501); return; }
    it->second.attribs[ai].binding = bi;
    VAOSyncAttribOffset(it->second, ai);
}
void glVertexArrayAttribFormat(GLuint v, GLuint ai, GLint s, GLenum t, GLboolean n, GLuint r) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (ai >= 16) { c.errors.Record(0x0501); return; }
    it->second.attribs[ai].size = s; it->second.attribs[ai].type = t;
    it->second.attribs[ai].normalized = n; it->second.attribs[ai].relativeOffset = r;
    VAOSyncAttribOffset(it->second, ai);
}
void glVertexArrayAttribIFormat(GLuint v, GLuint ai, GLint s, GLenum t, GLuint r) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (ai >= 16) { c.errors.Record(0x0501); return; }
    it->second.attribs[ai].size = s; it->second.attribs[ai].type = t;
    it->second.attribs[ai].relativeOffset = r; it->second.attribs[ai].isInt = true;
    VAOSyncAttribOffset(it->second, ai);
}
void glVertexArrayAttribLFormat(GLuint v, GLuint ai, GLint s, GLenum t, GLuint r) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (ai >= 16) { c.errors.Record(0x0501); return; }
    it->second.attribs[ai].size = s; it->second.attribs[ai].type = t;
    it->second.attribs[ai].relativeOffset = r; it->second.attribs[ai].isLong = true;
    VAOSyncAttribOffset(it->second, ai);
}
void glVertexArrayBindingDivisor(GLuint v, GLuint bi, GLuint d) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (bi >= it->second.bindings.size()) { c.errors.Record(0x0501); return; }
    it->second.bindings[bi].divisor = d;
    for (auto& a : it->second.attribs) if (a.binding == bi) a.divisor = d;
}
void glVertexArrayElementBuffer(GLuint v, GLuint b) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    it->second.elementBuffer = b;
}
void glVertexArrayVertexBuffer(GLuint v, GLuint bi, GLuint b, GLintptr o, GLsizei s) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    if (bi >= it->second.bindings.size()) { c.errors.Record(0x0501); return; }
    it->second.bindings[bi].buffer = b;
    it->second.bindings[bi].offset = o;
    it->second.bindings[bi].stride = s;
    // GIỮ relativeOffset từng attrib (spec §10.3.1) — xem glBindVertexBuffer.
    for (GLuint k = 0; k < (GLuint)it->second.attribs.size(); ++k) {
        auto& a = it->second.attribs[k];
        if (a.binding == bi) { a.buffer = b; a.stride = s; VAOSyncAttribOffset(it->second, k); }
    }
}
void glVertexArrayVertexBuffers(GLuint v, GLuint f, GLsizei n, const GLuint* b, const GLintptr* o, const GLsizei* s) {
    for (GLsizei i = 0; i < n; ++i) glVertexArrayVertexBuffer(v, f + i, b ? b[i] : 0, o ? o[i] : 0, s ? s[i] : 0);
}
// --- VertexAttrib s/b-variants (đủ 17 hàm): chuẩn hoá về double[4] shadow ---
static void SA(GLuint i, double x, double y, double z, double w) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    double d[4] = {x, y, z, w};
    c.state.SetShadow(0x8626, d, 32, i);
}
void glVertexAttrib1s(GLuint i, GLshort x) { SA(i,x,0,0,1); }
void glVertexAttrib1sv(GLuint i, const GLshort* v) { SA(i,v[0],0,0,1); }
void glVertexAttrib2s(GLuint i, GLshort x, GLshort y) { SA(i,x,y,0,1); }
void glVertexAttrib2sv(GLuint i, const GLshort* v) { SA(i,v[0],v[1],0,1); }
void glVertexAttrib3s(GLuint i, GLshort x, GLshort y, GLshort z) { SA(i,x,y,z,1); }
void glVertexAttrib3sv(GLuint i, const GLshort* v) { SA(i,v[0],v[1],v[2],1); }
void glVertexAttrib4s(GLuint i, GLshort x, GLshort y, GLshort z, GLshort w) { SA(i,x,y,z,w); }
void glVertexAttrib4sv(GLuint i, const GLshort* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4bv(GLuint i, const GLbyte* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4iv(GLuint i, const GLint* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4ubv(GLuint i, const GLubyte* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4uiv(GLuint i, const GLuint* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4usv(GLuint i, const GLushort* v) { SA(i,v[0],v[1],v[2],v[3]); }
void glVertexAttrib4Nub(GLuint i, GLubyte x, GLubyte y, GLubyte z, GLubyte w) { SA(i,x/255.0,y/255.0,z/255.0,w/255.0); }
void glVertexAttrib4Nubv(GLuint i, const GLubyte* v) { SA(i,v[0]/255.0,v[1]/255.0,v[2]/255.0,v[3]/255.0); }
void glVertexAttrib4Nbv(GLuint i, const GLbyte* v) { SA(i,(v[0]/127.0),(v[1]/127.0),(v[2]/127.0),(v[3]/127.0)); }
void glVertexAttrib4Niv(GLuint i, const GLint* v) { SA(i,v[0]/2147483647.0,v[1]/2147483647.0,v[2]/2147483647.0,v[3]/2147483647.0); }
void glVertexAttrib4Nsv(GLuint i, const GLshort* v) { SA(i,v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0); }
void glVertexAttrib4Nuiv(GLuint i, const GLuint* v) { SA(i,v[0]/4294967295.0,v[1]/4294967295.0,v[2]/4294967295.0,v[3]/4294967295.0); }
void glVertexAttrib4Nusv(GLuint i, const GLushort* v) { SA(i,v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0); }
} // namespace tglmt::gl
