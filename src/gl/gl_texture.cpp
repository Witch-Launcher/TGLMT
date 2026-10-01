// gl_texture.cpp — Textures: MTLTextureDescriptor + replaceRegion.
// Spec §8 (Textures). PixelStore alignment áp dụng khi upload (spec Table 8.x).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace tglmt;

// IR staging cho texture uploads (upload coalescing):
// glTexSubImage* chỉ update shadow + stage region; replaceRegion dồn đến flush
// (trước draw sampling texture / blit / readback). Nhiều sub-uploads kề nhau
// trong 1 frame (font atlas streaming) gộp thành 1 bbox → 1 replaceRegion.
static void StageTexRegion(Context& c, GLuint texId, uint32_t x, uint32_t y, uint32_t w,
                           uint32_t h, GLenum format, GLenum type) {
    if (!w || !h) return;
    // Deferred full: chỉ flush khi texture này đã dùng trong pass đang mở.
    // Atlas streaming (font glyphs) stage hàng chục regions/frame mà không phá batching.
    if (c.pendingEncoder && c.MustFlushForTextureStage(texId)) c.FlushPendingEncoder();
    c.pendingTexRegions[texId].push_back(Context::TexRegion{x, y, w, h, format, type});
    ++c.appleStats.texCoalesced;
}

namespace tglmt {
// Flush 1 texture: gộp regions cùng format thành bbox duy nhất rồi SyncRegionToGPU
// từ shadow (tight). Khác format → flush từng region riêng (không gộp sai conversion).
void Context::FlushTextureStaging(GLuint texId) {
    auto pit = pendingTexRegions.find(texId);
    if (pit == pendingTexRegions.end() || pit->second.empty()) return;
    auto tit = textures.find(texId);
    if (tit == textures.end() || !tit->second.gpu) {
        pendingTexRegions.erase(pit);
        return;
    }
    TextureObject& tx = tit->second;
    // Nhóm theo (format,type): vanilla dùng 1 format nên thường chỉ 1 nhóm → 1 bbox.
    // Sắp xếp để nhóm cùng format kề nhau.
    auto& regs = pit->second;
    std::sort(regs.begin(), regs.end(), [](const TexRegion& a, const TexRegion& b) {
        if (a.format != b.format) return a.format < b.format;
        return a.type < b.type;
    });
    size_t i = 0;
    // Bpp/format helpers cục bộ (tránh phụ thuộc hàm static dưới).
    auto bppOf = [](GLenum f, GLenum t) -> size_t {
        (void)t;
        switch (f) {
            case 0x1907: return 3;
            case 0x1908: return 4;
            case 0x80E1: return 4;
            case 0x1903: case 0x1906: case 0x1904: case 0x1905: case 0x1902:
            case 0x1901: case 0x1909: return 1;
            case 0x8227: return 2;
            default: return 4;
        }
    };
    while (i < regs.size()) {
        size_t j = i;
        GLenum fmt = regs[i].format, typ = regs[i].type;
        uint32_t x0 = regs[i].x, y0 = regs[i].y;
        uint32_t x1 = regs[i].x + regs[i].w, y1 = regs[i].y + regs[i].h;
        while (j + 1 < regs.size() && regs[j + 1].format == fmt && regs[j + 1].type == typ) {
            ++j;
            x0 = std::min(x0, regs[j].x);
            y0 = std::min(y0, regs[j].y);
            x1 = std::max(x1, regs[j].x + regs[j].w);
            y1 = std::max(y1, regs[j].y + regs[j].h);
        }
        // Kẹp bbox vào texture thật (an toàn khi regions cũ từ trước resize).
        if (x0 < tx.w && y0 < tx.h) {
            uint32_t bw = std::min(x1 - x0, tx.w - x0);
            uint32_t bh = std::min(y1 - y0, tx.h - y0);
            if (bw && bh) {
                size_t bpp = bppOf(fmt, typ);
                size_t texRow = (size_t)tx.w * bpp;
                // Shadow có thể ngắn (R8/RG8 font): chỉ sync khi đủ chỗ.
                if (tx.pixels.size() >= (size_t)tx.h * texRow) {
                    const uint8_t* srcRows = tx.pixels.data() + (size_t)y0 * texRow + (size_t)x0 * bpp;
                    // SyncRegionToGPU khai báo ở dưới trong file — forward qua lambda?
                    // Gọi trực tiếp device->updateTexture cho path raw tight;
                    // các path conversion (BGRA/RGB) xử lý gọn tại đây.
                    bool synced = false;
                    if (typ == 0x1401 && (fmt == 0x1908 || fmt == 0x1903 || fmt == 0x8227 || fmt == 0x1906)) {
                        std::vector<uint8_t> tight((size_t)bw * bh * bpp);
                        for (uint32_t r = 0; r < bh; ++r)
                            memcpy(tight.data() + (size_t)r * bw * bpp,
                                   srcRows + (size_t)r * texRow, (size_t)bw * bpp);
                        synced = device->updateTexture(tx.gpu.get(), x0, y0, bw, bh,
                                                       tight.data(), (size_t)bw * bpp);
                    } else if (typ == 0x1401 && fmt == 0x80E1) {
                        std::vector<uint8_t> rgba((size_t)bw * bh * 4);
                        for (uint32_t r = 0; r < bh; ++r)
                            for (uint32_t x = 0; x < bw; ++x) {
                                const uint8_t* s = srcRows + (size_t)r * texRow + (size_t)x * 4;
                                uint8_t* d = rgba.data() + ((size_t)r * bw + x) * 4;
                                d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = s[3];
                            }
                        synced = device->updateTexture(tx.gpu.get(), x0, y0, bw, bh,
                                                       rgba.data(), (size_t)bw * 4);
                    } else if (fmt == 0x1907 && typ == 0x1401) {
                        std::vector<uint8_t> rgba((size_t)bw * bh * 4);
                        for (uint32_t r = 0; r < bh; ++r)
                            for (uint32_t x = 0; x < bw; ++x) {
                                const uint8_t* s = srcRows + (size_t)r * texRow + (size_t)x * 3;
                                uint8_t* d = rgba.data() + ((size_t)r * bw + x) * 4;
                                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
                            }
                        synced = device->updateTexture(tx.gpu.get(), x0, y0, bw, bh,
                                                       rgba.data(), (size_t)bw * 4);
                    } else {
                        LogDebug(0, 0, 0, 0,
                                 "FlushTextureStaging: format/type chưa upload GPU (giữ shadow)");
                    }
                    (void)synced;
                    ++appleStats.texFlushes;
                }
            }
        }
        i = j + 1;
    }
    pendingTexRegions.erase(pit);
}
void Context::FlushAllTextureStaging() {
    if (pendingTexRegions.empty()) return;
    std::vector<GLuint> ids;
    ids.reserve(pendingTexRegions.size());
    for (auto& kv : pendingTexRegions) ids.push_back(kv.first);
    for (GLuint id : ids) FlushTextureStaging(id);
}
} // namespace tglmt

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
        case 0x1903: return 1; // RED (font RED8 — LUMINANCE upload của game)
        case 0x1906: return 1; // ALPHA
        case 0x1904: return 1; // GREEN (legacy)
        case 0x1905: return 1; // BLUE (legacy)
        case 0x8227: return 2; // RG (LUMINANCE_ALPHA upload của game)
        default: return 4;
    }
}
// Pitch unpack đúng spec §8.5: ROW_LENGTH (pixel, 0 = width) rồi align.
// Game 26.x set ROW_LENGTH = image width qua GlCommandEncoder.writeToTexture
// (font glyph sub-upload) — bỏ qua là đọc sai hàng (chữ hỏng).
static size_t UnpackRowLen(Context& c, size_t w, size_t bpp) {
    GLint rl = c.state.PixelStore().unpackRowLength;
    size_t elems = (rl > 0) ? (size_t)rl : w;
    GLint al = c.state.PixelStore().unpackAlignment;
    size_t align = (al == 1 || al == 2 || al == 4 || al == 8) ? (size_t)al : 4;
    return ((elems * bpp + align - 1) / align) * align;
}
// PBO unpack (spec §8: PIXEL_UNPACK_BUFFER bound → `pixels` là byte OFFSET).
// MC 26.x stream texture uploads qua PBO (writeToTexture). Bỏ qua = đọc rác
// (offset diễn như con trỏ) hoặc alloc rỗng khi offset 0 (bug nút mất + 501).
// Đọc từ PBO SHADOW (authoritative: SubData/Map-Unmap luôn sync shadow).
// Trả nullptr + ok=false khi thiếu PBO/vượt biên (caller Record tương ứng).
static const uint8_t* UnpackBase(Context& c, const void* pixels, size_t total, bool& ok) {
    GLuint up = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/);
    if (!up) { ok = true; return (const uint8_t*)pixels; }
    auto it = c.buffers.find(up);
    if (it == c.buffers.end()) { ok = false; return nullptr; }
    size_t off = (size_t)pixels;
    if (off + total > it->second.data.size()) { ok = false; return nullptr; }
    ok = true;
    return it->second.data.data() + off;
}
// Sync 1 vùng pixels lên GPU: RGBA/RED/RG/UBYTE raw (tight), RGB/UBYTE expand
// alpha 255. srcRows trỏ vùng (xoff,yoff,w,h) với pitch srcRowLen. Trả true
// nếu đã sync. RED/RG raw đúng cho R8/RG8 GPU (font RED8 của game).
// BGRA/UBYTE (widgets/gui trên một số path Blaze3D) → swizzle R<->B sang RGBA.
static bool SyncRegionToGPU(Context& c, TextureObject& tx, GLint xoff, GLint yoff, GLsizei w,
                            GLsizei h, const uint8_t* srcRows, size_t srcRowLen, GLenum format,
                            GLenum type) {
    if (!tx.gpu || w <= 0 || h <= 0) return false;
    if (type == 0x1401 &&
        (format == 0x1908 || format == 0x1903 || format == 0x8227 || format == 0x1906)) {
        size_t bpp = Bpp(format, type);
        std::vector<uint8_t> tight((size_t)w * h * bpp);
        for (GLsizei r = 0; r < h; ++r)
            memcpy(tight.data() + (size_t)r * w * bpp, srcRows + r * srcRowLen,
                   (size_t)w * bpp);
        return c.device->updateTexture(tx.gpu.get(), (uint32_t)xoff, (uint32_t)yoff,
                                       (uint32_t)w, (uint32_t)h, tight.data(),
                                       (size_t)w * bpp);
    }
    // BGRA/UBYTE (0x80E1): GPU TGLMT là RGBA8 → đảo R/B. Trước đây rơi vào
    // "giữ shadow" (GPU đen) trong khi shadow có data → quad textured (widgets
    // nút menu) sample đen/alpha 0 → discard → nút chỉ còn chữ.
    if (type == 0x1401 && format == 0x80E1) {
        std::vector<uint8_t> rgba((size_t)w * h * 4);
        for (GLsizei r = 0; r < h; ++r)
            for (GLsizei x = 0; x < w; ++x) {
                const uint8_t* s = srcRows + r * srcRowLen + (size_t)x * 4;
                uint8_t* d = rgba.data() + ((size_t)r * w + x) * 4;
                d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = s[3];
            }
        return c.device->updateTexture(tx.gpu.get(), (uint32_t)xoff, (uint32_t)yoff,
                                       (uint32_t)w, (uint32_t)h, rgba.data(),
                                       (size_t)w * 4);
    }
    if (format == 0x1907 && type == 0x1401) {
        std::vector<uint8_t> rgba((size_t)w * h * 4);
        for (GLsizei r = 0; r < h; ++r)
            for (GLsizei x = 0; x < w; ++x) {
                rgba[((size_t)r * w + x) * 4 + 0] = srcRows[r * srcRowLen + (size_t)x * 3 + 0];
                rgba[((size_t)r * w + x) * 4 + 1] = srcRows[r * srcRowLen + (size_t)x * 3 + 1];
                rgba[((size_t)r * w + x) * 4 + 2] = srcRows[r * srcRowLen + (size_t)x * 3 + 2];
                rgba[((size_t)r * w + x) * 4 + 3] = 255;
            }
        return c.device->updateTexture(tx.gpu.get(), (uint32_t)xoff, (uint32_t)yoff,
                                       (uint32_t)w, (uint32_t)h, rgba.data(), (size_t)w * 4);
    }
    // Fallback cuối cho RGBA/BGRA 4B với type UINT đóng gói (REV,...):
    // upload raw còn hơn đen (nút xám không lệch màu nhiều; alpha sai vẫn hơn
    // discard toàn bộ). Ghi log 1 lần để chẩn đoán.
    if ((format == 0x1908 || format == 0x80E1) &&
        (type == 0x8367 || type == 0x8368 || type == 0x1405 || type == 0x1404)) {
        static bool loggedRaw = false;
        if (!loggedRaw) {
            loggedRaw = true;
            c.LogDebug(0, 0, 0, 0, "SyncRegionToGPU: RGBA/BGRA UINT raw upload (màu có thể lệch)");
        }
        std::vector<uint8_t> tight((size_t)w * h * 4);
        for (GLsizei r = 0; r < h; ++r)
            memcpy(tight.data() + (size_t)r * w * 4, srcRows + r * srcRowLen,
                   (size_t)w * 4);
        // BGRA + UINT: đảo R/B cho đúng RGBA GPU.
        if (format == 0x80E1) {
            for (size_t i = 0; i < (size_t)w * h; ++i) std::swap(tight[i * 4], tight[i * 4 + 2]);
        }
        return c.device->updateTexture(tx.gpu.get(), (uint32_t)xoff, (uint32_t)yoff,
                                       (uint32_t)w, (uint32_t)h, tight.data(),
                                       (size_t)w * 4);
    }
    return false;
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
    for (GLsizei i = 0; i < n; ++i) {
        c.textures.erase(t[i]);
        c.pendingTexRegions.erase(t[i]);
    }
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
    // (đúng spec §8 — probe rồi fallback 8192 khi quá lớn). Lưu spec để query
    // GetTexLevelParameteriv trả lời (quá 8192 → 0), tránh lỗi oan tồn lỗi
    // pending mà vanilla quy cho call sau (crash copyTobuffer 1282).
    switch (target) {
        case 0x8063: case 0x8064: case 0x8070: case 0x8071: case 0x8513: case 0x8C19: {
            ProxyTex px;
            if (w > 0 && h > 0 && w <= 8192 && h <= 8192) {
                px.w = w; px.h = h; px.ifmt = (GLenum)internalformat;
            }
            c.proxyTex[target] = px;
            return;
        }
        default: break;
    }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return; // lỗi đã record trong BoundTex
    auto& tx = *tp;
    // PBO unpack (spec §8): pixels là offset khi UNPACK bound. MC 26.x stream
    // uploads qua PBO — bỏ qua = alloc rỗng (offset 0) / đọc rác (offset !=0).
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    size_t bppAll = Bpp(format, type);
    size_t rowLenAll = (w > 0) ? UnpackRowLen(c, (size_t)w, bppAll) : 0;
    size_t skipAll = (size_t)c.state.PixelStore().unpackSkipRows * rowLenAll +
                     (size_t)c.state.PixelStore().unpackSkipPixels * bppAll;
    size_t totalAll = skipAll + ((w > 0 && h > 0) ? ((size_t)(h - 1) * rowLenAll + (size_t)w * bppAll) : 0);
    const uint8_t* pixBase = (const uint8_t*)pixels;
    if (hasUnpack) {
        bool okBase = true;
        pixBase = UnpackBase(c, pixels, totalAll, okBase);
        if (!okBase) { c.errors.Record(0x0501); return; }
    }
    bool hasData = hasUnpack || pixels;
    // Chẩn đoán texture lớn rỗng: chỉ khi TGLMT_DIAG=1.
    if (c.DiagOn() && (w >= 256 || h >= 256)) {
        static int nBig = 0;
        if (++nBig <= 12) {
            char ds[32];
            if (hasUnpack) snprintf(ds, sizeof(ds), "PBO+%zu", (size_t)pixels);
            else snprintf(ds, sizeof(ds), "%s", pixels ? "data" : "NULL");
            fprintf(stderr,
                    "[TGLMT] bigTexImage#%d id=%u tgt=0x%x lv=%d %dx%d ifmt=0x%x fmt=0x%x ty=0x%x %s\n",
                    nBig, tp->id, target, level, w, h, internalformat, format, type, ds);
            fflush(stderr);
        }
    }
    // Mip levels >0: chỉ ghi nhận số levels, KHÔNG đụng base (bug cũ: ghi đè
    // w/h/pixels bằng level nhỏ nhất → TexSubImage level 0 fail bounds →
    // texture rỗng. Game 26.x alloc mọi mip qua TexImage2D NULL trước).
    // GPU TGLMT chỉ giữ base level (generateMipmap cần texture mipmapped).
    if (level > 0) {
        if (level + 1 > tx.levels) tx.levels = level + 1;
        c.LogDebug(0, 0, 0, 0, "glTexImage2D: mip level>0 giữ base (GPU base-level)");
        (void)border;
        return;
    }
    // Upload face cubemap: mỗi face shadow riêng + GPU cube thật (panorama
    // vanilla samplerCube). Trước đây placeholder 2D + skip bind = đen.
    if (IsCubeFace(target) && tx.target == 0x8513) {
        int face = (int)(target - 0x8515);
        size_t bpp = Bpp(format, type);
        if (tx.isCube && (tx.w != (uint32_t)w || tx.h != (uint32_t)h)) {
            c.errors.Record(0x0501); return; // face lệch size (spec §8.5)
        }
        tx.isCube = true;
        tx.w = w; tx.h = h; tx.internalFormat = internalformat; tx.levels = level + 1;
        tx.faces[face].assign((size_t)w * h * bpp, 0);
        if (hasData) {
            size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
            size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                          (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
            const uint8_t* src = pixBase + skip;
            for (GLsizei r = 0; r < h; ++r)
                memcpy(tx.faces[face].data() + (size_t)r * w * bpp, src + r * rowLen,
                       (size_t)w * bpp);
        }
        if (!tx.gpu) // tạo 1 lần ở face đầu tiên thấy
            tx.gpu = c.device->newCubeTexture((uint32_t)w, ToMetalFormat(internalformat));
        if (tx.gpu && hasData && type == 0x1401 && w > 0 && h > 0 &&
            (format == 0x1908 || format == 0x1903) && (uint32_t)w == tx.w) {
            size_t bpr = (size_t)w * bpp;
            std::vector<uint8_t> tight((size_t)w * h * bpp);
            for (GLsizei r = 0; r < h; ++r)
                memcpy(tight.data() + (size_t)r * w * bpp,
                       tx.faces[face].data() + (size_t)r * w * bpp, (size_t)w * bpp);
            if (!c.device->updateCubeFace(tx.gpu.get(), (uint32_t)face, tight.data(), bpr))
                c.LogDebug(0, 0, 0, 0, "glTexImage2D cubemap face: GPU upload fail");
        } else if (hasData && !(format == 0x1908 || format == 0x1903)) {
            c.LogDebug(0, 0, 0, 0, "glTexImage2D cubemap face: format chưa upload GPU (giữ shadow)");
        }
        tx.pixels = tx.faces[0]; // mirror face 0 cho diag + fallback CPU
        (void)border;
        return;
    }
    tx.isCube = false;
    tx.w = w; tx.h = h; tx.internalFormat = internalformat; tx.levels = level + 1;
    size_t n = (size_t)w * h * Bpp(format, type);
    tx.pixels.assign(n, 0);
    if (hasData) {
        // áp unpackAlignment + ROW_LENGTH (spec §8.5)
        size_t bpp = Bpp(format, type);
        size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
        size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                      (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
        const uint8_t* src = pixBase + skip;
        for (GLsizei r = 0; r < h; ++r) memcpy(tx.pixels.data() + r * w * bpp, src + r * rowLen, (size_t)w * bpp);
    }
    tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
    c.pendingTexRegions.erase(tp->id); // realloc GPU mới đã có full data → staging cũ vô nghĩa
    // Upload base level lên GPU (bug cũ: tạo texture rỗng → sampling đen).
    // Vanilla atlas RGBA/UBYTE tight; JPG panorama là RGB/UBYTE → expand alpha 255.
    // Font RED8 (LUMINANCE) → R8 raw. BGRA/UBYTE (widgets/gui) → swizzle R<->B.
    // Mọi format khác giữ shadow + log.
    if (tx.gpu && hasData && w > 0 && h > 0) {
        if ((format == 0x1908 || format == 0x1903 || format == 0x8227) && type == 0x1401) {
            // pixels đã unpack vào tx.pixels tight → upload trực tiếp
            size_t bpp = Bpp(format, type);
            if (tx.pixels.size() >= (size_t)w * h * bpp)
                c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                        tx.pixels.data(), (size_t)w * bpp);
        } else if (format == 0x80E1 && type == 0x1401) {
            // BGRA → RGBA (đảo R/B). tx.pixels đang giữ BGRA raw; dựng RGBA tight.
            std::vector<uint8_t> rgba((size_t)w * h * 4);
            for (GLsizei r = 0; r < h; ++r)
                for (GLsizei x = 0; x < w; ++x) {
                    const uint8_t* s = tx.pixels.data() + ((size_t)r * w + x) * 4;
                    uint8_t* d = rgba.data() + ((size_t)r * w + x) * 4;
                    d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = s[3];
                }
            c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                    rgba.data(), (size_t)w * 4);
        } else if (format == 0x1907 && type == 0x1401) {
            // RGB → RGBA (alpha 255), tôn trọng unpack pitch của source
            size_t rowLen = UnpackRowLen(c, (size_t)w, 3);
            size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                          (size_t)c.state.PixelStore().unpackSkipPixels * 3;
            const uint8_t* src = pixBase + skip;
            std::vector<uint8_t> rgba((size_t)w * h * 4);
            for (GLsizei r = 0; r < h; ++r)
                for (GLsizei x = 0; x < w; ++x) {
                    rgba[((size_t)r * w + x) * 4 + 0] = src[r * rowLen + (size_t)x * 3 + 0];
                    rgba[((size_t)r * w + x) * 4 + 1] = src[r * rowLen + (size_t)x * 3 + 1];
                    rgba[((size_t)r * w + x) * 4 + 2] = src[r * rowLen + (size_t)x * 3 + 2];
                    rgba[((size_t)r * w + x) * 4 + 3] = 255;
                }
            c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                    rgba.data(), (size_t)w * 4);
        } else if ((format == 0x1908 || format == 0x80E1) &&
                   (type == 0x8367 || type == 0x8368 || type == 0x1405 || type == 0x1404)) {
            // RGBA/BGRA UINT đóng gói: upload raw (BGRA đảo R/B). Còn hơn đen.
            std::vector<uint8_t> raw((size_t)w * h * 4);
            for (GLsizei r = 0; r < h; ++r)
                memcpy(raw.data() + (size_t)r * w * 4,
                       tx.pixels.data() + (size_t)r * w * 4, (size_t)w * 4);
            if (format == 0x80E1)
                for (size_t i = 0; i < (size_t)w * h; ++i) std::swap(raw[i * 4], raw[i * 4 + 2]);
            c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                    raw.data(), (size_t)w * 4);
            c.LogDebug(0, 0, 0, 0, "glTexImage2D: RGBA/BGRA UINT raw upload");
        } else {
            c.LogDebug(0, 0, 0, 0, "glTexImage2D: format/type chưa upload GPU (giữ shadow)");
        }
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
    // PBO unpack: pixels là offset khi UNPACK bound (offset 0 hợp lệ).
    // Bug cũ: !pixels → 0501 oan (501 còn lại trên máy) + offset nonzero đọc rác.
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    if (!tp || (!pixels && !hasUnpack)) { if (!pixels) c.errors.Record(0x0501); return; }
    auto& t = *tp;
    if (c.DiagOn() && (w >= 256 || h >= 256)) {
        static int nBigSub = 0;
        if (++nBigSub <= 8) {
            fprintf(stderr,
                    "[TGLMT] bigTexSub#%d id=%u tgt=0x%x lv=%d off=(%d,%d) %dx%d fmt=0x%x "
                    "ty=0x%x tex=%ux%u\n",
                    nBigSub, tp->id, target, level, xoff, yoff, w, h, format, type, t.w,
                    t.h);
            fflush(stderr);
        }
    }
    // Level>0: GPU chỉ giữ base, không corrupt base shadow (xem TexImage).
    if (level > 0) {
        c.LogDebug(0, 0, 0, 0, "glTexSubImage2D: mip level>0 bỏ qua (GPU base-level)");
        return;
    }
    // SubImage lên face cubemap: ghi vào face slot + sync GPU face đó.
    std::vector<uint8_t>* dstPix = &t.pixels;
    int cubeFace = -1;
    if (IsCubeFace(target) && t.target == 0x8513 && t.isCube) {
        cubeFace = (int)(target - 0x8515);
        dstPix = &t.faces[cubeFace];
    }
    size_t bpp = Bpp(format, type);
    // Copy đúng vùng (xoff,yoff), kẹp biên theo spec §8.5 (lệch biên → INVALID_VALUE)
    if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
        (size_t)(xoff + w) > t.w || (size_t)(yoff + h) > t.h) {
        c.errors.Record(0x0501); return;
    }
    size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
    size_t total = skip + (h > 0 ? ((size_t)(h - 1) * rowLen + (size_t)w * bpp) : 0);
    bool okBase = true;
    const uint8_t* pixBase = UnpackBase(c, pixels, total, okBase);
    if (!okBase) { c.errors.Record(0x0501); return; }
    const uint8_t* src = pixBase + skip;
    if (dstPix->size() < (size_t)t.w * t.h * bpp) dstPix->resize((size_t)t.w * t.h * bpp, 0);
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* dst = dstPix->data() + ((size_t)(yoff + r) * t.w + (size_t)xoff) * bpp;
        memcpy(dst, src + r * rowLen, (size_t)w * bpp);
    }
    if (dstPix != &t.pixels && !t.faces[0].empty()) t.pixels = t.faces[0]; // mirror face 0
    // IR deferred: cubemap face giữ sync ngay (hiếm, panorama init, không phải hot path).
    // 2D thường: stage region, GPU replaceRegion dồn đến flush trước draw sampling.
    // Trước đây: mỗi SubImage = 1 replaceRegion ngay (N uploads → N Metal calls).
    if (cubeFace >= 0) {
        if (t.gpu && (format == 0x1908 || format == 0x1903) && type == 0x1401) {
            size_t bpr = (size_t)t.w * bpp;
            std::vector<uint8_t> tight((size_t)t.w * t.h * bpp);
            for (uint32_t r = 0; r < t.h; ++r)
                memcpy(tight.data() + (size_t)r * t.w * bpp,
                       dstPix->data() + (size_t)r * t.w * bpp, (size_t)t.w * bpp);
            if (!c.device->updateCubeFace(t.gpu.get(), (uint32_t)cubeFace, tight.data(), bpr))
                c.LogDebug(0, 0, 0, 0, "glTexSubImage2D: cube face GPU sync fail");
        }
    } else {
        StageTexRegion(c, tp->id, (uint32_t)xoff, (uint32_t)yoff, (uint32_t)w, (uint32_t)h,
                       format, type);
    }
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
    // Enum đúng spec Table 8.x (đã đối chiếu client.jar 26.1.2:
    // GlCommandEncoder.writeToTexture set ROW_LENGTH/ALIGNMENT/SKIPs).
    // Bug cũ: 0x0CF2 (ROW_LENGTH) nhầm thành unpackAlignment,
    // 0x0CF5 (UNPACK_ALIGNMENT) nhầm sang pack → glyph upload sai pitch.
    switch (pname) {
        case 0x0CF2: ps.unpackRowLength = param; break;   // UNPACK_ROW_LENGTH
        case 0x0CF3: ps.unpackSkipRows = param; break;    // UNPACK_SKIP_ROWS
        case 0x0CF4: ps.unpackSkipPixels = param; break;  // UNPACK_SKIP_PIXELS
        case 0x0CF5: ps.unpackAlignment = param; break;   // UNPACK_ALIGNMENT
        case 0x0CF6: ps.unpackImageHeight = param; break; // UNPACK_IMAGE_HEIGHT
        case 0x0CF7: ps.unpackSkipImages = param; break;  // UNPACK_SKIP_IMAGES
        case 0x0D02: ps.packRowLength = param; break;     // PACK_ROW_LENGTH
        case 0x0D03: ps.packSkipRows = param; break;      // PACK_SKIP_ROWS
        case 0x0D04: ps.packSkipPixels = param; break;    // PACK_SKIP_PIXELS
        case 0x0D05: ps.packAlignment = param; break;     // PACK_ALIGNMENT
        default: c.state.SetShadow(pname, &param, 4); break;
    }
}
void glCopyTexImage1D(GLenum a, GLint b, GLenum d, GLint e, GLint f, GLsizei g, GLint h) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glCopyTexImage2D(GLenum a, GLint b, GLenum d, GLint e, GLint f, GLsizei g, GLsizei h, GLint i) { (void)a;(void)b;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyTexSubImage1D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLsizei f) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f; }
// Copy framebuffer (READ) → texture (blur/post-chain, menu loading đen nếu stub).
// Quy ước thô như blit (không flip; chỉ ReadPixels flip): copy raw GPU→GPU khi
// cùng format, rồi sync shadow từ GPU để GetTexImage/ReadPixels sau đó thấy mới.
static void CopyFBToTexture(Context& c, TextureObject& dst, GLint level,
                            GLint xoff, GLint yoff, GLint x, GLint y,
                            GLsizei w, GLsizei h) {
    if (level != 0) {
        c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage: level>0 bỏ qua (GPU base-level)");
        return;
    }
    if (w <= 0 || h <= 0) { c.errors.Record(0x0501); return; }
    if (xoff < 0 || yoff < 0 || x < 0 || y < 0 ||
        (size_t)(xoff + w) > dst.w || (size_t)(yoff + h) > dst.h) {
        c.errors.Record(0x0501); return;
    }
    GLuint readFbo = c.state.BoundReadFBO();
    if (!c.device || c.device->isNull()) {
        // Null backend: copy shadow nếu nguồn là FBO texture.
        if (readFbo != 0) {
            auto fit = c.fbos.find(readFbo);
            if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
            auto cit = fit->second.colorTex.find(0);
            if (cit == fit->second.colorTex.end()) return;
            auto sit = c.textures.find(cit->second);
            if (sit == c.textures.end() || sit->second.pixels.empty()) return;
            auto& src = sit->second;
            size_t bpp = 4;
            if (dst.pixels.size() < (size_t)dst.w * dst.h * bpp)
                dst.pixels.resize((size_t)dst.w * dst.h * bpp, 0);
            if (src.pixels.size() < (size_t)src.w * src.h * bpp) return;
            for (GLsizei r = 0; r < h; ++r) {
                if ((size_t)(y + r) >= src.h || (size_t)(yoff + r) >= dst.h) break;
                memcpy(dst.pixels.data() + ((size_t)(yoff + r) * dst.w + xoff) * bpp,
                       src.pixels.data() + ((size_t)(y + r) * src.w + x) * bpp,
                       (size_t)w * bpp);
            }
        }
        return;
    }
    if (!dst.gpu) { c.errors.Record(0x0502); return; }
    c.FlushPendingEncoder(); // IR: commit batch trước khi copy (không stale TBDR)
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    c.device->commitAndWait(); // xả draws NoWait trước khi copy (không stale TBDR)
    bool gpuOk = false;
    if (readFbo == 0) {
        auto def = c.device->defaultRenderTarget();
        if (!def) { c.errors.Record(0x0502); return; }
        if ((uint32_t)(x + w) > def->width() || (uint32_t)(y + h) > def->height()) {
            c.errors.Record(0x0501); return;
        }
        if (def->pixelFormat() == dst.gpu->pixelFormat()) {
            gpuOk = c.device->blitFromTarget(def.get(), dst.gpu.get(),
                                             (uint32_t)x, (uint32_t)y,
                                             (uint32_t)w, (uint32_t)h,
                                             (uint32_t)xoff, (uint32_t)yoff);
        }
        if (!gpuOk) {
            // Khác format (default RGBA8 vs dest sRGB/R8...) hoặc blit fail:
            // readback màn hình rồi update vùng (đúng pixels, hơi chậm nhưng hiếm).
            std::vector<uint8_t> scr((size_t)def->width() * def->height() * 4, 0);
            if (def->readback(scr.data(), (size_t)def->width() * 4)) {
                std::vector<uint8_t> region((size_t)w * h * 4);
                for (GLsizei r = 0; r < h; ++r)
                    memcpy(region.data() + (size_t)r * w * 4,
                           scr.data() + ((size_t)(y + r) * def->width() + x) * 4,
                           (size_t)w * 4);
                // Dest sRGB/R8: upload raw 4B (sai nhẹ màu còn hơn đen); R8/RG8
                // chỉ lấy kênh R (blur/menu không dùng R8 làm đích copy).
                if (dst.gpu->pixelFormat() == metal::PixelFormat::R8Unorm) {
                    std::vector<uint8_t> r1((size_t)w * h);
                    for (size_t i = 0; i < (size_t)w * h; ++i) r1[i] = region[i * 4];
                    gpuOk = c.device->updateTexture(dst.gpu.get(), (uint32_t)xoff,
                                                    (uint32_t)yoff, (uint32_t)w,
                                                    (uint32_t)h, r1.data(), (size_t)w);
                } else {
                    gpuOk = c.device->updateTexture(dst.gpu.get(), (uint32_t)xoff,
                                                    (uint32_t)yoff, (uint32_t)w,
                                                    (uint32_t)h, region.data(),
                                                    (size_t)w * 4);
                }
            }
            if (!gpuOk)
                c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage: default→tex fallback fail");
        }
    } else {
        auto fit = c.fbos.find(readFbo);
        if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        auto cit = fit->second.colorTex.find(0);
        if (cit == fit->second.colorTex.end()) return;
        auto sit = c.textures.find(cit->second);
        if (sit == c.textures.end() || !sit->second.gpu) return;
        auto& src = sit->second;
        if ((size_t)(x + w) > src.w || (size_t)(y + h) > src.h) {
            c.errors.Record(0x0501); return;
        }
        if (src.gpu->pixelFormat() == dst.gpu->pixelFormat()) {
            gpuOk = c.device->blitCopy(src.gpu.get(), dst.gpu.get(),
                                       (uint32_t)x, (uint32_t)y,
                                       (uint32_t)w, (uint32_t)h,
                                       (uint32_t)xoff, (uint32_t)yoff);
        }
        if (!gpuOk) {
            c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage: FBO→tex khác format/fail, CPU shadow");
            size_t bpp = 4;
            if (src.pixels.size() >= (size_t)src.w * src.h * bpp &&
                dst.pixels.size() >= (size_t)dst.w * dst.h * bpp) {
                for (GLsizei r = 0; r < h; ++r)
                    memcpy(dst.pixels.data() + ((size_t)(yoff + r) * dst.w + xoff) * bpp,
                           src.pixels.data() + ((size_t)(y + r) * src.w + x) * bpp,
                           (size_t)w * bpp);
                c.device->updateTexture(dst.gpu.get(), 0, 0, dst.w, dst.h,
                                        dst.pixels.data(), (size_t)dst.w * bpp);
                gpuOk = true;
            }
        }
    }
    // Không sync shadow đích từ GPU ở đây (giữ 60fps): sampling blur/post dùng
    // GPU mới; GetTexImage lên blur texture hiếm. Shadow cũ không ảnh hưởng draw.
    (void)gpuOk;
}
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff,
                         GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    {
        static uint64_t n = 0;
        ++c.appleStats.copyTex;
        if (c.DiagOn() && ++n <= 5) {
            fprintf(stderr, "[TGLMT] copyTexSub#%llu tgt=0x%x level=%d off=(%d,%d) src=(%d,%d) size=%dx%d readFbo=%u\n",
                    (unsigned long long)n, target, level, xoff, yoff, x, y, w, h,
                    c.state.BoundReadFBO());
            fflush(stderr);
        }
    }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    CopyFBToTexture(c, *tp, level, xoff, yoff, x, y, w, h);
}
void glCopyTexSubImage3D(GLenum a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyTextureSubImage1D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f; }
void glCopyTextureSubImage2D(GLuint tex, GLint level, GLint xoff, GLint yoff,
                             GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    ++c.appleStats.copyTex;
    auto it = c.textures.find(tex);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    CopyFBToTexture(c, it->second, level, xoff, yoff, x, y, w, h);
}
void glCopyTextureSubImage3D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLint f, GLint g, GLsizei h, GLsizei i) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i; }
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel,
                        GLint srcX, GLint srcY, GLint srcZ,
                        GLuint dstName, GLenum dstTarget, GLint dstLevel,
                        GLint dstX, GLint dstY, GLint dstZ,
                        GLsizei w, GLsizei h, GLsizei d) {
    Context& c = Context::Current();
    (void)srcTarget; (void)dstTarget; (void)srcZ; (void)dstZ; (void)d;
    if (srcLevel != 0 || dstLevel != 0) {
        c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: level>0 bỏ qua (GPU base-level)");
        return;
    }
    if (w <= 0 || h <= 0) { c.errors.Record(0x0501); return; }
    auto sit = c.textures.find(srcName);
    auto dit = c.textures.find(dstName);
    if (sit == c.textures.end() || dit == c.textures.end()) {
        c.errors.Record(0x0502); return;
    }
    auto& src = sit->second;
    auto& dst = dit->second;
    if (srcX < 0 || srcY < 0 || dstX < 0 || dstY < 0 ||
        (size_t)(srcX + w) > src.w || (size_t)(srcY + h) > src.h ||
        (size_t)(dstX + w) > dst.w || (size_t)(dstY + h) > dst.h) {
        c.errors.Record(0x0501); return;
    }
    if (!c.device || c.device->isNull()) {
        if (!src.pixels.empty() && !dst.pixels.empty() &&
            src.pixels.size() >= (size_t)src.w * src.h * 4 &&
            dst.pixels.size() >= (size_t)dst.w * dst.h * 4) {
            for (GLsizei r = 0; r < h; ++r)
                memcpy(dst.pixels.data() + ((size_t)(dstY + r) * dst.w + dstX) * 4,
                       src.pixels.data() + ((size_t)(srcY + r) * src.w + srcX) * 4,
                       (size_t)w * 4);
        }
        return;
    }
    if (!src.gpu || !dst.gpu) { c.errors.Record(0x0502); return; }
    c.FlushPendingEncoder(); // IR: commit batch trước khi blit copy
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    c.device->commitAndWait();
    if (src.gpu->pixelFormat() == dst.gpu->pixelFormat() &&
        c.device->blitCopy(src.gpu.get(), dst.gpu.get(),
                           (uint32_t)srcX, (uint32_t)srcY,
                           (uint32_t)w, (uint32_t)h,
                           (uint32_t)dstX, (uint32_t)dstY)) {
        // GPU-only, không readback (giữ fps; sampling dùng GPU).
        return;
    }
    c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: khác format/fail, CPU shadow");
    if (src.pixels.size() >= (size_t)src.w * src.h * 4 &&
        dst.pixels.size() >= (size_t)dst.w * dst.h * 4) {
        for (GLsizei r = 0; r < h; ++r)
            memcpy(dst.pixels.data() + ((size_t)(dstY + r) * dst.w + dstX) * 4,
                   src.pixels.data() + ((size_t)(srcY + r) * src.w + srcX) * 4,
                   (size_t)w * 4);
        c.device->updateTexture(dst.gpu.get(), 0, 0, dst.w, dst.h,
                                dst.pixels.data(), (size_t)dst.w * 4);
    }
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
    // PBO unpack: p là offset khi UNPACK bound (offset 0 hợp lệ).
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    if (!p && !hasUnpack) { c.errors.Record(0x0501); return; }
    if ((w >= 256 || h >= 256)) {
        static int nBigDSA = 0;
        if (++nBigDSA <= 8) {
            fprintf(stderr,
                    "[TGLMT] bigTexSubDSA#%d id=%u lv=%d off=(%d,%d) %dx%d fmt=0x%x ty=0x%x "
                    "tex=%ux%u\n",
                    nBigDSA, t, l, x, y, w, h, f, ty, it->second.w, it->second.h);
            fflush(stderr);
        }
    }
    if (l > 0) {
        c.LogDebug(0, 0, 0, 0, "glTextureSubImage2D: mip level>0 bỏ qua (GPU base-level)");
        return;
    }
    auto& tx = it->second;
    // Mirror bản bound (glTexSubImage2D): tôn trọng x/y + unpack pitch, sync GPU.
    // Game 26.x upload texture qua DSA khi direct_state_access bật.
    size_t bpp = Bpp(f, ty);
    if (x < 0 || y < 0 || w < 0 || h < 0 || (size_t)(x + w) > tx.w || (size_t)(y + h) > tx.h) {
        c.errors.Record(0x0501);
        return;
    }
    size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
    if (tx.pixels.size() < (size_t)tx.w * tx.h * bpp) tx.pixels.resize((size_t)tx.w * tx.h * bpp, 0);
    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
    size_t total = skip + (h > 0 ? ((size_t)(h - 1) * rowLen + (size_t)w * bpp) : 0);
    bool okBase = true;
    const uint8_t* pixBase = UnpackBase(c, p, total, okBase);
    if (!okBase) { c.errors.Record(0x0501); return; }
    const uint8_t* src = pixBase + skip;
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* dst = tx.pixels.data() + ((size_t)(y + r) * tx.w + (size_t)x) * bpp;
        memcpy(dst, src + r * rowLen, (size_t)w * bpp);
    }
    // IR deferred: stage thay vì sync GPU ngay (DSA path của game 26.x).
    (void)src; (void)rowLen;
    StageTexRegion(c, t, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, f, ty);
}
void glTextureSubImage3D(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h, GLenum i, GLenum j, const void* k) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k; }
} // namespace tglmt::gl
