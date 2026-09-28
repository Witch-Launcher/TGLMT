// gl_texture.cpp — Textures: MTLTextureDescriptor + replaceRegion.
// Spec §8 (Textures). PixelStore alignment áp dụng khi upload (spec Table 8.x).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
using namespace tglmt;

// internalFormat GL (giá trị đã đối chiếu gl46_types.h) → PixelFormat Metal.
// Không đoán: chỉ map các format core chắc chắn; còn lại RGBA8Unorm + debug log.
static metal::PixelFormat ToMetalFormat(GLenum internalFormat) {
    switch (internalFormat) {
        case 0x8058: return metal::PixelFormat::RGBA8Unorm;        // RGBA8
        case 0x8C43: return metal::PixelFormat::RGBA8Unorm_sRGB;   // SRGB8_ALPHA8
        case 0x8229: return metal::PixelFormat::R8Unorm;           // R8
        case 0x822B: return metal::PixelFormat::RG8Unorm;          // RG8
        case 0x822E: return metal::PixelFormat::R32Float;          // R32F (buffer)
        case 0x8235: return metal::PixelFormat::R32Sint;           // R32I (CloudFaces)
        case 0x8236: return metal::PixelFormat::R32Uint;           // R32UI (buffer)
        case 0x81A5: case 0x81A6: case 0x8CAC:                     // DEPTH16/24/32F
            return metal::PixelFormat::Depth32Float;
        case 0x88F0: return metal::PixelFormat::Depth24Stencil8;   // DEPTH24_STENCIL8
        default:
            Context::Current().LogDebug(0, 0, 0, 0,
                "ToMetalFormat: internalFormat lạ → RGBA8Unorm (M5b mở rộng)");
            return metal::PixelFormat::RGBA8Unorm;
    }
}
static size_t Bpp(GLenum format, GLenum type) {
    (void)type;
    switch (format) {
        case 0x1907: return 3; // RGB
        case 0x1908: return 4; // RGBA
        case 0x1902: return 1; // DEPTH
        case 0x1901: return 1; // STENCIL
        case 0x1909: return 1; // LUMINANCE-ish
        default: return 4;
    }
}

namespace tglmt::gl {
void glGenTextures(GLsizei n, GLuint* t) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Texture, n, t);
    for (GLsizei i = 0; i < n; ++i) c.textures[t[i]] = TextureObject{t[i]};
}
void glCreateTextures(GLenum target, GLsizei n, GLuint* t) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Texture, n, t);
    for (GLsizei i = 0; i < n; ++i) { c.textures[t[i]] = TextureObject{t[i]}; c.textures[t[i]].target = target; }
}
void glDeleteTextures(GLsizei n, const GLuint* t) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Texture, n, t);
    for (GLsizei i = 0; i < n; ++i) c.textures.erase(t[i]);
}
GLboolean glIsTexture(GLuint t) {
    return Context::Current().registry.Is(ObjectKind::Texture, t) ? 1 : 0;
}
void glBindTexture(GLenum target, GLuint t) {
    Context& c = Context::Current();
    if (t && !c.textures.count(t)) { c.errors.Record(0x0502); return; }
    if (t) c.textures[t].target = target;
    c.state.BindTextureUnit(c.state.ActiveTexture(), t, target);
}
void glBindTextureUnit(GLuint u, GLuint t) {
    Context& c = Context::Current();
    c.state.BindTextureUnit(u, t);
}
void glBindTextures(GLuint f, GLsizei n, const GLuint* t) {
    Context& c = Context::Current();
    for (GLsizei i = 0; i < n; ++i) c.state.BindTextureUnit(f + i, t ? t[i] : 0);
}
void glActiveTexture(GLenum tex) {
    Context& c = Context::Current();
    if (tex < 0x84C0 || tex >= 0x84C0 + 32) { c.errors.Record(0x0500); return; }
    c.state.SetActiveTexture(tex - 0x84C0);
}
// Spec §8.1: mọi lệnh Tex* (không DSA) tác động lên texture đang bind tại
// active unit. Kiểm tra target khớp; sai → INVALID_OPERATION (core profile).
// Ngoại lệ: face cubemap (POSITIVE_X..NEGATIVE_Z) thuộc texture CUBE đã bind.
static bool IsCubeFace(GLenum t) { return t >= 0x8515 && t <= 0x851A; }
static TextureObject* BoundTex(Context& c, GLenum target) {
    GLuint unit = c.state.ActiveTexture();
    GLuint id = c.state.BoundTexture(unit);
    if (!id) { c.errors.Record(0x0502); return nullptr; }
    auto it = c.textures.find(id);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return nullptr; }
    if (it->second.target != target) {
        if (!(it->second.target == 0x8513 && IsCubeFace(target))) {
            c.errors.Record(0x0502); return nullptr;
        }
    }
    return &it->second;
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; }
    // Proxy texture (GlDevice probe max size): không đổi state, không lỗi
    // (đúng spec §8 — probe rồi fallback 8192 khi quá lớn).
    switch (target) {
        case 0x8063: case 0x8064: case 0x8070: case 0x8071: case 0x8513: case 0x8C19:
            return;
        default: break;
    }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return; // lỗi đã record trong BoundTex
    auto& tx = *tp;
    // Upload face cubemap: mỗi face shadow riêng, GPU placeholder = face 0.
    // (Cube Metal thật + sample vec3 là P1.) Không lỗi để GlDevice qua được.
    if (IsCubeFace(target) && tx.target == 0x8513) {
        int face = (int)(target - 0x8515);
        size_t bpp = Bpp(format, type);
        if (tx.isCube && (tx.w != (uint32_t)w || tx.h != (uint32_t)h)) {
            c.errors.Record(0x0501); return; // face lệch size (spec §8.5)
        }
        tx.isCube = true;
        tx.w = w; tx.h = h; tx.internalFormat = internalformat; tx.levels = level + 1;
        tx.faces[face].assign((size_t)w * h * bpp, 0);
        if (pixels) {
            GLint align = c.state.PixelStore().unpackAlignment;
            size_t rowLen = (((size_t)w * bpp + (size_t)align - 1) / (size_t)align) * (size_t)align;
            const uint8_t* src = (const uint8_t*)pixels;
            for (GLsizei r = 0; r < h; ++r)
                memcpy(tx.faces[face].data() + (size_t)r * w * bpp, src + r * rowLen, (size_t)w * bpp);
        }
        if (face == 0) { // mirror face 0 cho GetTexImage + GPU placeholder
            tx.pixels = tx.faces[0];
            tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
            if (tx.gpu && pixels && format == 0x1908 && type == 0x1401 && w > 0 && h > 0)
                c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                        tx.pixels.data(), (size_t)w * 4);
        } else if (!tx.gpu) {
            tx.pixels = tx.faces[face]; // chưa có face 0: mirror tạm để không đọc rác
            tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
        }
        if (face != 0)
            c.LogDebug(0, 0, 0, 0, "glTexImage2D cubemap face: shadow only, GPU placeholder face 0 (P1 cube)");
        (void)border; (void)level;
        return;
    }
    tx.isCube = false;
    tx.w = w; tx.h = h; tx.internalFormat = internalformat; tx.levels = level + 1;
    size_t n = (size_t)w * h * Bpp(format, type);
    tx.pixels.assign(n, 0);
    if (pixels) {
        // áp unpackAlignment (spec: row pitch = ceil(w*bpp / align)*align)
        GLint align = c.state.PixelStore().unpackAlignment;
        size_t bpp = Bpp(format, type);
        size_t rowLen = ((w * bpp + align - 1) / align) * align;
        const uint8_t* src = (const uint8_t*)pixels;
        for (GLsizei r = 0; r < h; ++r) memcpy(tx.pixels.data() + r * w * bpp, src + r * rowLen, (size_t)w * bpp);
    }
    tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
    // Upload base level lên GPU (bug cũ: tạo texture rỗng → sampling đen).
    // Chỉ RGBA8/UBYTE tight (vanilla atlas); format khác giữ shadow + log.
    if (tx.gpu && pixels && format == 0x1908 && type == 0x1401 && w > 0 && h > 0) {
        // pixels đã unpack vào tx.pixels tight → upload trực tiếp
        if (tx.pixels.size() >= (size_t)w * h * 4)
            c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                    tx.pixels.data(), (size_t)w * 4);
    }
    (void)border; (void)level;
}
void glTexImage1D(GLenum t, GLint l, GLint inf, GLsizei w, GLint b, GLenum f, GLenum ty, const void* p) {
    glTexImage2D(t, l, inf, w, 1, b, f, ty, p);
}
void glTexImage3D(GLenum target, GLint level, GLint inf, GLsizei w, GLsizei h, GLsizei d, GLint b, GLenum f, GLenum ty, const void* p) {
    if (target == 0x8070 || target == 0x8C19) return; // PROXY_3D/2D_ARRAY: probe, no-op
    (void)target;(void)level;(void)inf;(void)w;(void)h;(void)d;(void)b;(void)f;(void)ty;(void)p;
    // 3D → Metal type3D (M5 texture đầy đủ); hiện giữ shadow + log
    Context::Current().LogDebug(0,0,0,0,"glTexImage3D staged (Metal type3D ở M3)");
}
void glTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLsizei w, GLsizei h, GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    TextureObject* tp = BoundTex(c, target);
    if (!tp || !pixels) { if (!pixels) c.errors.Record(0x0501); return; }
    auto& t = *tp;
    // SubImage lên face cubemap: ghi vào face slot, sync GPU chỉ khi face 0.
    std::vector<uint8_t>* dstPix = &t.pixels;
    bool syncGPU = true;
    if (IsCubeFace(target) && t.target == 0x8513 && t.isCube) {
        int face = (int)(target - 0x8515);
        dstPix = &t.faces[face];
        syncGPU = (face == 0);
    }
    size_t bpp = Bpp(format, type);
    // Copy đúng vùng (xoff,yoff), kẹp biên theo spec §8.5 (lệch biên → INVALID_VALUE)
    if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
        (size_t)(xoff + w) > t.w || (size_t)(yoff + h) > t.h) {
        c.errors.Record(0x0501); return;
    }
    GLint align = c.state.PixelStore().unpackAlignment;
    size_t rowLen = ((size_t)w * bpp + (size_t)align - 1) / (size_t)align * (size_t)align;
    const uint8_t* src = (const uint8_t*)pixels;
    if (dstPix->size() < (size_t)t.w * t.h * bpp) dstPix->resize((size_t)t.w * t.h * bpp, 0);
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* dst = dstPix->data() + ((size_t)(yoff + r) * t.w + (size_t)xoff) * bpp;
        memcpy(dst, src + r * rowLen, (size_t)w * bpp);
    }
    if (dstPix != &t.pixels && !t.faces[0].empty()) t.pixels = t.faces[0]; // mirror face 0
    // Sync GPU vùng đã đổi (chunk atlas streaming mỗi frame) — A11 Shared coherent
    if (syncGPU && t.gpu && bpp == 4) {
        // pack thành tight rows cho replaceRegion
        std::vector<uint8_t> tight((size_t)w * h * 4);
        for (GLsizei r = 0; r < h; ++r)
            memcpy(tight.data() + (size_t)r * w * 4, src + r * rowLen, (size_t)w * 4);
        c.device->updateTexture(t.gpu.get(), (uint32_t)xoff, (uint32_t)yoff,
                                (uint32_t)w, (uint32_t)h, tight.data(), (size_t)w * 4);
    }
    (void)level;
}
void glTexSubImage1D(GLenum t, GLint l, GLint x, GLsizei w, GLenum f, GLenum ty, const void* p) {
    glTexSubImage2D(t, l, x, 0, w, 1, f, ty, p);
}
void glTexSubImage3D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLenum j, const void* k) {
    (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;
}
void glTexParameterf(GLenum t, GLenum p, GLfloat v) { glTexParameteri(t, p, (GLint)v); }
void glTexParameterfv(GLenum t, GLenum p, const GLfloat* v) { glTexParameteri(t, p, (GLint)v[0]); }
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    Context& c = Context::Current();
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    tp->params[pname] = param;
}
void glTexParameteriv(GLenum t, GLenum p, const GLint* v) { glTexParameteri(t, p, v[0]); }
void glTexParameterIiv(GLenum t, GLenum p, const GLint* v) { glTexParameteri(t, p, v[0]); }
void glTexParameterIuiv(GLenum t, GLenum p, const GLuint* v) { glTexParameteri(t, p, (GLint)v[0]); }
void glTextureParameterf(GLuint t, GLenum p, GLfloat v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    it->second.params[p] = (GLint)v;
}
void glTextureParameterfv(GLuint t, GLenum p, const GLfloat* v) { glTextureParameterf(t, p, v[0]); }
void glTextureParameteri(GLuint t, GLenum p, GLint v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    it->second.params[p] = v;
}
void glTextureParameteriv(GLuint t, GLenum p, const GLint* v) { glTextureParameteri(t, p, v[0]); }
void glTextureParameterIiv(GLuint t, GLenum p, const GLint* v) { glTextureParameteri(t, p, v[0]); }
void glTextureParameterIuiv(GLuint t, GLenum p, const GLuint* v) { glTextureParameteri(t, p, (GLint)v[0]); }
void glGenerateMipmap(GLenum target) {
    Context& c = Context::Current();
    // Metal: blit generateMipmapsForTexture — cần texture mipmapped.
    // Vanilla atlas: minFilter mipmap nhưng texture tạo non-mipmapped → GPU trả false,
    // giữ base-level (render được, shimmer nhẹ). M5c tạo mipmapped khi levels>1.
    GLuint unit = c.state.ActiveTexture();
    GLuint id = c.state.BoundTexture(unit);
    auto it = c.textures.find(id);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.gpu && c.device->generateMipmaps(it->second.gpu.get())) return;
    c.LogDebug(0,0,0,0,"glGenerateMipmap: base-level giữ (texture non-mipmapped, render được)");
    (void)target;
}
void glGenerateTextureMipmap(GLuint t) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.gpu && c.device->generateMipmaps(it->second.gpu.get())) return;
    c.LogDebug(0,0,0,0,"glGenerateTextureMipmap: base-level giữ (non-mipmapped)");
}
void glPixelStoref(GLenum p, GLfloat v) { glPixelStorei(p, (GLint)v); }
void glPixelStorei(GLenum pname, GLint param) {
    Context& c = Context::Current();
    auto& ps = c.state.PixelStore();
    switch (pname) {
        case 0x0CF5: ps.packAlignment = param; break;   // PACK_ALIGNMENT
        case 0x0CF2: ps.unpackAlignment = param; break; // UNPACK_ALIGNMENT
        default: c.state.SetShadow(pname, &param, 4); break;
    }
}
void glCopyTexImage1D(GLenum a, GLint b, GLenum d, GLint e, GLint f, GLsizei g, GLint h) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glCopyTexImage2D(GLenum a, GLint b, GLenum d, GLint e, GLint f, GLsizei g, GLsizei h, GLint i) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyTexSubImage1D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLsizei f) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f; }
void glCopyTexSubImage2D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLsizei g, GLsizei h) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glCopyTexSubImage3D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyTextureSubImage1D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f; }
void glCopyTextureSubImage2D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLsizei g, GLsizei h) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glCopyTextureSubImage3D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyImageSubData(GLuint a, GLenum b, GLint c_, GLint d, GLint e, GLint f, GLuint g, GLenum h, GLint i, GLint j, GLint k, GLint l, GLsizei m, GLsizei n, GLsizei o) {
    (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;(void)l;(void)m;(void)n;(void)o;
}
void glCompressedTexImage1D(GLenum a, GLint b, GLenum d, GLsizei e, GLint f, GLsizei g, const void* h) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glCompressedTexImage2D(GLenum a, GLint b, GLenum d, GLsizei e, GLsizei f, GLint g, GLsizei h, const void* i) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCompressedTexImage3D(GLenum a, GLint b, GLenum d, GLsizei e, GLsizei f, GLsizei g, GLint h, GLsizei i, const void* j) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j; }
void glCompressedTexSubImage1D(GLenum a, GLint b, GLint c_, GLsizei d, GLenum e, GLsizei f, const void* g) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g; }
void glCompressedTexSubImage2D(GLenum a, GLint b, GLint c_, GLint d, GLsizei e, GLsizei f, GLenum g, GLsizei h, const void* i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCompressedTexSubImage3D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLsizei j, const void* k) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; }
void glCompressedTextureSubImage1D(GLuint a, GLint b, GLint c_, GLsizei d, GLenum e, GLsizei f, const void* g) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g; }
void glCompressedTextureSubImage2D(GLuint a, GLint b, GLint c_, GLint d, GLsizei e, GLsizei f, GLenum g, GLsizei h, const void* i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCompressedTextureSubImage3D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLsizei j, const void* k) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; }
void glClearTexImage(GLuint a, GLint b, GLenum d, GLenum e, const void* f) { (void)a;(void)b;(void)d;(void)e;(void)f; }
void glClearTexSubImage(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLenum j, const void* k) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; }
void glInvalidateTexImage(GLuint a, GLint b) { (void)a;(void)b; }
void glInvalidateTexSubImage(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glTextureStorage1D(GLuint t, GLsizei l, GLenum inf, GLsizei w) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    it->second.w = w; it->second.h = 1; it->second.internalFormat = inf; it->second.levels = l;
    it->second.pixels.assign((size_t)w * 4, 0);
}
void glTextureStorage2D(GLuint t, GLsizei l, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    it->second.w = w; it->second.h = h; it->second.internalFormat = inf; it->second.levels = l;
    it->second.pixels.assign((size_t)w * h * 4, 0);
    it->second.gpu = c.device->newTexture(w, h, ToMetalFormat(it->second.internalFormat));
}
void glTextureStorage3D(GLuint a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g; }
void glTextureStorage2DMultisample(GLuint a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLboolean g) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g; }
void glTextureStorage3DMultisample(GLuint a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g, GLboolean h) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glTexStorage1D(GLenum a, GLsizei b, GLenum d, GLsizei e) { (void)a;(void)b;(void)d;(void)e; }
void glTexStorage2D(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f) { (void)a;(void)b;(void)d;(void)e;(void)f; }
void glTexStorage3D(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g; }
void glTexStorage2DMultisample(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLboolean g) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g; }
void glTexStorage3DMultisample(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g, GLboolean h) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glTextureSubImage1D(GLuint a, GLint b, GLint c_, GLsizei d, GLenum e, GLenum f, const void* g) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g; }
void glTextureSubImage2D(GLuint t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void* p) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    (void)l;(void)x;(void)y;
    size_t bpp = Bpp(f, ty);
    if (p && it->second.pixels.size() >= (size_t)w*h*bpp) memcpy(it->second.pixels.data(), p, (size_t)w*h*bpp);
}
void glTextureSubImage3D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLenum j, const void* k) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; }
} // namespace tglmt::gl
