// gl_get_extra.cpp — Getters đọc object/shadow: texture, sampler, FBO/RBO, VAO, sync, program-resource.
// Texture pixels: làm TƯƠI từ GPU trước khi trả (draws đi GPU-only, shadow cũ
// toàn 0 → screenshot đen; vanilla crash 1282 ở copyTextureToBuffer cũng xuất
// phát từ đường này). Hỗ trợ PBO (PIXEL_PACK_BUFFER), vùng x/y/w/h, PACK state.
// KHÔNG flip hàng (giữ đúng thứ tự upload; chỉ glReadPixels flip).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace tglmt;

namespace tglmt::gl {
// --- texture getters ---
// Cube face target dùng chung object với GL_TEXTURE_CUBE_MAP; các getter phải
// chấp nhận face target thay vì đòi target lưu == target truy vấn.
static bool IsCubeFaceTarget(GLenum t) { return t >= 0x8515 && t <= 0x851A; }
static bool TexMatchesTarget(const TextureObject& tx, GLenum t) {
    return tx.target == t || (tx.target == 0x8513 && IsCubeFaceTarget(t));
}
// Pitch PACK đúng spec §8.11: ROW_LENGTH (pixel, 0 = width) rồi align.
static size_t PackRowLen(Context& c, size_t w, size_t bpp) {
    GLint rl = c.state.PixelStore().packRowLength;
    size_t elems = (rl > 0) ? (size_t)rl : w;
    GLint al = c.state.PixelStore().packAlignment;
    size_t align = (al == 1 || al == 2 || al == 4 || al == 8) ? (size_t)al : 4;
    return ((elems * bpp + align - 1) / align) * align;
}
// Số byte/pixel đích của glGetTexImage theo format/type (GL Table 8.8).
static size_t GetPixelsBpp(GLenum format, GLenum type) {
    size_t comps = 0;
    switch (format) {
        case 0x1908: case 0x80E1: comps = 4; break; // RGBA / BGRA
        case 0x1907: comps = 3; break;              // RGB
        case 0x8227: comps = 2; break;              // RG
        case 0x1903: comps = 1; break;              // RED
        default: return 0;
    }
    switch (type) {
        case 0x1401: return comps;
        case 0x1403: return comps * 2;
        case 0x1406: return comps * 4;
        case 0x8035: case 0x8367: case 0x8368:
            return comps == 4 ? 4 : 0;
        default: return 0;
    }
}
// Ghi 1 texel RGBA8 nguồn sang format/type đích (đúng Table 8.8).
static void StoreTexel(uint8_t* d, const uint8_t* s, GLenum format, GLenum type) {
    uint8_t v[4] = {s[0], s[1], s[2], s[3]};
    size_t n = 4;
    switch (format) {
        case 0x80E1: std::swap(v[0], v[2]); break; // BGRA
        case 0x1907: n = 3; break;                  // RGB
        case 0x8227: n = 2; break;                  // RG
        case 0x1903: n = 1; break;                  // RED
        default: break;
    }
    if (type == 0x1401) {
        memcpy(d, v, n);
    } else if (type == 0x1403) {
        for (size_t i = 0; i < n; ++i) {
            uint16_t x = (uint16_t)(v[i] * 257);
            memcpy(d + i * 2, &x, 2);
        }
    } else if (type == 0x1406) {
        for (size_t i = 0; i < n; ++i) {
            float x = v[i] / 255.0f;
            memcpy(d + i * 4, &x, 4);
        }
    } else if (type == 0x8035) { // UNSIGNED_INT_8_8_8_8
        uint32_t x = (uint32_t)v[0] | ((uint32_t)v[1] << 8) |
                     ((uint32_t)v[2] << 16) | ((uint32_t)v[3] << 24);
        memcpy(d, &x, 4);
    } else if (type == 0x8367) { // UNSIGNED_INT_8_8_8_8_REV
        uint32_t x = (uint32_t)v[3] | ((uint32_t)v[2] << 8) |
                     ((uint32_t)v[1] << 16) | ((uint32_t)v[0] << 24);
        memcpy(d, &x, 4);
    } else if (type == 0x8368) { // UNSIGNED_INT_2_10_10_10_REV
        uint32_t r = ((uint32_t)v[0] * 1023u + 127u) / 255u;
        uint32_t g = ((uint32_t)v[1] * 1023u + 127u) / 255u;
        uint32_t b = ((uint32_t)v[2] * 1023u + 127u) / 255u;
        uint32_t a = ((uint32_t)v[3] * 3u + 127u) / 255u;
        uint32_t x = r | (g << 10) | (b << 20) | (a << 30);
        memcpy(d, &x, 4);
    }
}
// Đọc vùng (x,y,w,h) level 0 của texture vào *dst (đã trừ PBO), KHÔNG flip.
// Hỗ trợ RGBA/BGRA × {UNSIGNED_BYTE, UNSIGNED_INT_8_8_8_8,
// UNSIGNED_INT_8_8_8_8_REV} (screenshot vanilla dùng packed); còn lại 0x0502.
static void GetTexSubImpl(Context& c, TextureObject& tx, GLint level, GLint x, GLint y,
                          GLint z, GLsizei w, GLsizei h, GLsizei d, GLenum format,
                          GLenum type, GLsizei bufSize, void* pixels, int face = -1) {
    // Khi PIXEL_PACK_BUFFER bound, `pixels` là byte OFFSET vào PBO (offset 0 hợp lệ,
    // vanilla screenshot truyền 0). Chỉ báo NULL khi không có PBO (bug cũ: từ chối
    // offset 0 → copyTobuffer crash 1282 khi vào world).
    bool hasPBO = c.state.BoundBuffer(0x88EB /*PIXEL_PACK_BUFFER*/) != 0;
    if (!pixels && !hasPBO) { c.errors.Record(0x0501); return; }
    if (level < 0 || x < 0 || y < 0 || z < 0 || w < 0 || h < 0 || d < 0) {
        c.errors.Record(0x0501); return;
    }
    if (tx.levels > 0 && level >= tx.levels) { c.errors.Record(0x0501); return; }
    // Mip level>0: đọc từ shadow per-level (TexImage/SubImage level>0 đã lưu).
    // Trước đây trả 0 câm → copyTextureToBuffer(mip)/screenshot mip đen.
    // P1: shadow base-level có thể đã bị bỏ (Apple backend). Bản đọc từ GPU sống
    // ở đây — KHÔNG ghi ngược vào tx.pixels, nếu không shadow sẽ hồi sinh và
    // nuốt mất đúng phần bộ nhớ mà P1 tiết kiệm.
    std::vector<uint8_t> gpuScratch, zeroScratch;
    const std::vector<uint8_t>* srcPix = &tx.pixels;
    GLsizei tw = (GLsizei)tx.w, th = (GLsizei)tx.h;
    GLsizei td = tx.d ? (GLsizei)tx.d : 1;
    bool srcValid = false;
    if (level != 0) {
        // 2D array: layer count KHÔNG co theo mip; 3D: depth co theo mip.
        const bool isArray = (tx.target == 0x8C1A);
        const TextureObject::MipLevel* mlp = nullptr;
        if (face >= 0 && tx.isCube) {
            auto fit = tx.faceMips[face].find(level);
            if (fit != tx.faceMips[face].end()) mlp = &fit->second;
        }
        if (!mlp) {
            auto mit = tx.mipData.find(level);
            if (mit != tx.mipData.end()) mlp = &mit->second;
        }
        if (!mlp) {
            tw = std::max(1, (GLsizei)tx.w >> level);
            th = std::max(1, (GLsizei)tx.h >> level);
            td = isArray ? (tx.d ? (GLsizei)tx.d : 1)
                         : (tx.d ? std::max(1, (GLsizei)tx.d >> level) : 1);
        } else {
            srcPix = &mlp->pixels;
            tw = mlp->w; th = mlp->h;
            td = mlp->d ? (GLsizei)mlp->d : 1;
            if (srcPix->size() >= (size_t)tw * th * td * 4) srcValid = true;
        }
        // Immutable storage không giữ shadow per-level; đọc GPU đúng level
        // (2D view cho level>0, layer view cho array/cube face). 3D vẫn shadow.
        if (!srcValid && TexReadableFromGPU(tx) &&
            c.ReadTextureGPU(tx, gpuScratch, face, level)) {
            srcPix = &gpuScratch;
            srcValid = true;
        }
        if (!srcValid) {
            zeroScratch.assign((size_t)tw * th * td * 4, 0);
            srcPix = &zeroScratch;
        }
    }
    size_t dstBpp = GetPixelsBpp(format, type);
    if (!dstBpp) {
        // Chẩn đoán crash copyTobuffer trên máy: stderr (LogDebug câm vì MC không
        // đăng ký debug callback). Throttle 8 lần.
        static int nUnsup = 0;
        if (++nUnsup <= 8) {
            fprintf(stderr,
                    "[TGLMT] GetTexSub UNSUPPORTED fmt=0x%x type=0x%x tex=%ux%u lv=%d\n",
                    format, type, tx.w, tx.h, level);
            fflush(stderr);
        }
        c.errors.Record(0x0502); // type/format ngoài subset (M5b mở rộng)
        return;
    }
    if ((size_t)(x + w) > (size_t)tw || (size_t)(y + h) > (size_t)th ||
        (size_t)(z + d) > (size_t)td) {
        static int nOob = 0;
        if (++nOob <= 8) {
            fprintf(stderr,
                    "[TGLMT] GetTexSub OOB x=%d y=%d z=%d w=%d h=%d d=%d tex=%dx%dx%d lv=%d\n",
                    x, y, z, w, h, d, tw, th, td, level);
            fflush(stderr);
        }
        c.errors.Record(0x0501); return;
    }
    // Đọc thẳng từ GPU (đúng pixels cho screenshot sau render).
    // Chỉ level 0 có storage GPU (mip>0 đọc shadow đã lưu ở trên).
    // ReadTextureGPU đọc đủ layer của 2D array; 3D giữ shadow CPU (Metal không
    // cho view 3D→2D) nên đường này chỉ dùng khi shadow thực sự có/đọc được.
    if (level == 0 && c.ReadTextureGPU(tx, gpuScratch, face)) srcPix = &gpuScratch;
    else if (level == 0 && face >= 0) srcPix = &tx.faces[face];
    if (srcPix->size() < (size_t)tw * th * td * 4) {
        static int nShort = 0;
        if (++nShort <= 8) {
            fprintf(stderr, "[TGLMT] GetTexSub SHORT shadow %zu < %dx%dx%d*4 lv=%d\n",
                    srcPix->size(), tw, th, td, level);
            fflush(stderr);
        }
        c.errors.Record(0x0502); return;
    }
    size_t dstRow = PackRowLen(c, (size_t)w, dstBpp);
    GLint packImgH = c.state.PixelStore().packImageHeight;
    size_t imageHeight = (packImgH > 0) ? (size_t)packImgH : (size_t)h;
    size_t imgStride = imageHeight * dstRow;
    size_t skip = (size_t)c.state.PixelStore().packSkipImages * imgStride +
                  (size_t)c.state.PixelStore().packSkipRows * dstRow +
                  (size_t)c.state.PixelStore().packSkipPixels * dstBpp;
    size_t need = skip;
    if (d > 0 && h > 0 && w > 0)
        need += (size_t)(d - 1) * imgStride +
                (size_t)(h - 1) * dstRow + (size_t)w * dstBpp;
    if ((size_t)bufSize < need) {
        static int nSmall = 0;
        if (++nSmall <= 8) {
            fprintf(stderr,
                    "[TGLMT] GetTexSub SMALLBUF buf=%d need=%zu (w=%d h=%d d=%d row=%zu "
                    "tex=%ux%ux%u)\n",
                    bufSize, need, w, h, d, dstRow, tx.w, tx.h, tx.d);
            fflush(stderr);
        }
        c.errors.Record(0x0502); return;
    }
    // Đích ghi: con trỏ thật, hoặc OFFSET vào PBO khi PIXEL_PACK_BUFFER bound
    // (screenshot vanilla). Phải phân biệt TRƯỚC khi ghi (ghi nhầm offset như
    // con trỏ = segfault/ghi bậy địa chỉ thấp).
    uint8_t* dstBase = nullptr;
    std::vector<uint8_t>* pboVec = nullptr;
    size_t pboOff = 0;
    GLuint packBuf = c.state.BoundBuffer(0x88EB /*PIXEL_PACK_BUFFER*/);
    if (packBuf) {
        auto bit = c.buffers.find(packBuf);
        if (bit == c.buffers.end()) { c.errors.Record(0x0502); return; }
        pboOff = (size_t)pixels;
        // PBO pack ghi thẳng vào shadow → MutablePbo materialize nếu Apple
        // backend đã bỏ shadow (P1), rồi resize đủ [0, pboOff+need).
        pboVec = c.MutablePbo(packBuf, pboOff + need);
        if (!pboVec) { c.errors.Record(0x0502); return; }
        dstBase = pboVec->data() + pboOff + skip;
    } else {
        dstBase = (uint8_t*)pixels + skip;
    }
    // KHÔNG flip hàng: GetTexImage trả đúng thứ tự GL-order (row 0 = đáy trước)
    // — toàn pipeline (render, upload, readback) cùng quy ước GL-order.
    for (GLsizei zi = 0; zi < d; ++zi) {
        uint8_t* dst = dstBase + (size_t)zi * imgStride;
        for (GLsizei r = 0; r < h; ++r) {
            const uint8_t* s = srcPix->data() +
                (((size_t)(z + zi) * (size_t)th + (size_t)(y + r)) * (size_t)tw +
                 (size_t)x) * 4;
            uint8_t* drow = dst + (size_t)r * dstRow;
            for (GLsizei i = 0; i < w; ++i)
                StoreTexel(drow + (size_t)i * dstBpp, s + (size_t)i * 4, format, type);
        }
    }
    // PBO: đồng bộ GPU buffer để Map/GetBufferSubData sau thấy dữ liệu mới.
    if (pboVec) {
        auto bit = c.buffers.find(packBuf);
        if (bit != c.buffers.end() && bit->second.gpu && !bit->second.data.empty()) {
            size_t n = bit->second.data.size();
            auto nb = c.device->newBufferWithBytes(bit->second.data.data(), n,
                                                   metal::StorageMode::Shared);
            if (nb) bit->second.gpu = nb;
        }
    }
}
static bool CubeFaceIndex(GLenum t, int* face) {
    if (t < 0x8515 || t > 0x851A) return false;
    *face = (int)(t - 0x8515);
    return true;
}
// Mip shadow đúng nguồn: cube face dùng faceMips[face], còn lại mipData.
static const TextureObject::MipLevel* FindMipShadow(const TextureObject& tx,
                                                    GLint level, int face) {
    if (face >= 0 && tx.isCube) {
        auto it = tx.faceMips[face].find(level);
        if (it != tx.faceMips[face].end()) return &it->second;
    }
    auto it = tx.mipData.find(level);
    return it == tx.mipData.end() ? nullptr : &it->second;
}
void glGetTexImage(GLenum t, GLint l, GLenum f, GLenum ty, void* p) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), t);
    auto it = c.textures.find(id);
    int face = -1;
    bool isFace = CubeFaceIndex(t, &face);
    if (it == c.textures.end() ||
        (it->second.target != t && !(isFace && it->second.target == 0x8513))) {
        c.errors.Record(0x0502); return;
    }
    auto& tx = it->second;
    if (isFace) face = (int)(t - 0x8515);
    GLsizei tw = (GLsizei)tx.w, th = (GLsizei)tx.h;
    GLsizei td = tx.d ? (GLsizei)tx.d : 1;
    if (l > 0) {
        const TextureObject::MipLevel* ml = FindMipShadow(tx, l, face);
        if (ml) {
            tw = ml->w; th = ml->h;
            td = ml->d ? (GLsizei)ml->d : 1;
        } else if (l < tx.levels) {
            tw = std::max(1, (GLsizei)tx.w >> l);
            th = std::max(1, (GLsizei)tx.h >> l);
            td = (tx.target == 0x8C1A) ? (tx.d ? (GLsizei)tx.d : 1)
                 : (tx.d ? std::max(1, (GLsizei)tx.d >> l) : 1);
        } else { c.errors.Record(0x0501); return; }
    } else if (l < 0) { c.errors.Record(0x0501); return; }
    // glGetTexImage không có bufSize: kích thước đích do PACK_* quyết định.
    // Không được lấy tight w*h*d*4 làm giới hạn (PACK_IMAGE_HEIGHT/ROW_LENGTH
    // có thể làm vùng ghi lớn hơn) — người gọi tự cấp đủ theo GL spec.
    GetTexSubImpl(c, tx, l, 0, 0, 0, tw, th, isFace ? 1 : td, f, ty, 0x7fffffff, p, face);
}
void glGetTextureImage(GLuint t, GLint l, GLenum f, GLenum ty, GLsizei n, void* p) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    auto& tx = it->second;
    GLsizei tw = (GLsizei)tx.w, th = (GLsizei)tx.h;
    GLsizei td = tx.d ? (GLsizei)tx.d : 1;
    if (l > 0) {
        const TextureObject::MipLevel* ml = FindMipShadow(tx, l, -1);
        if (ml) {
            tw = ml->w; th = ml->h;
            td = ml->d ? (GLsizei)ml->d : 1;
        } else if (l < tx.levels) {
            tw = std::max(1, (GLsizei)tx.w >> l);
            th = std::max(1, (GLsizei)tx.h >> l);
            td = (tx.target == 0x8C1A) ? (tx.d ? (GLsizei)tx.d : 1)
                 : (tx.d ? std::max(1, (GLsizei)tx.d >> l) : 1);
        } else { c.errors.Record(0x0501); return; }
    } else if (l < 0) { c.errors.Record(0x0501); return; }
    GetTexSubImpl(c, tx, l, 0, 0, 0, tw, th, td, f, ty, n, p);
}
void glGetTextureSubImage(GLuint t, GLint l, GLint x, GLint y, GLint z, GLsizei w,
                          GLsizei h, GLsizei d, GLenum f, GLenum ty, GLsizei n, void* p) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    auto& tx = it->second;
    // GL spec: zoffset/depth chỉ có nghĩa với 3D / 2D array / cube array.
    // Với 2D/cube/1D, chúng bị bỏ qua — vanilla screenshot gọi depth=0 và vẫn
    // mong đọc đủ ảnh (bug cũ: nhận 0 ảnh → PBO đen).
    bool volume = (tx.target == 0x806F /*TEXTURE_3D*/ ||
                   tx.target == 0x8C1A /*TEXTURE_2D_ARRAY*/ ||
                   tx.target == 0x9009 /*TEXTURE_CUBE_MAP_ARRAY*/);
    if (!volume) { z = 0; d = 1; }
    GetTexSubImpl(c, tx, l, x, y, z, w, h, d, f, ty, n, p);
}
void glGetnTexImage(GLenum t, GLint l, GLenum f, GLenum ty, GLsizei n, void* p) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), t);
    auto it = c.textures.find(id);
    int face = -1;
    bool isFace = CubeFaceIndex(t, &face);
    if (it == c.textures.end() ||
        (it->second.target != t && !(isFace && it->second.target == 0x8513))) {
        c.errors.Record(0x0502); return;
    }
    auto& tx = it->second;
    GLsizei tw = (GLsizei)tx.w, th = (GLsizei)tx.h;
    GLsizei td = tx.d ? (GLsizei)tx.d : 1;
    if (l > 0) {
        const TextureObject::MipLevel* ml = FindMipShadow(tx, l, face);
        if (ml) {
            tw = ml->w; th = ml->h;
            td = ml->d ? (GLsizei)ml->d : 1;
        } else if (l < tx.levels) {
            tw = std::max(1, (GLsizei)tx.w >> l);
            th = std::max(1, (GLsizei)tx.h >> l);
            td = (tx.target == 0x8C1A) ? (tx.d ? (GLsizei)tx.d : 1)
                 : (tx.d ? std::max(1, (GLsizei)tx.d >> l) : 1);
        } else { c.errors.Record(0x0501); return; }
    }
    GetTexSubImpl(c, tx, l, 0, 0, 0, tw, th, isFace ? 1 : td, f, ty, n, p, face);
}
// Không có compressed texture nào được tạo (xem gl_texture.cpp) → đọc compressed
// image là INVALID_OPERATION, không trả bộ đệm 0 giả.
void glGetCompressedTexImage(GLenum, GLint, void*) { Context::Current().errors.Record(0x0502); }
void glGetCompressedTextureImage(GLuint, GLint, GLsizei, void*) { Context::Current().errors.Record(0x0502); }
void glGetCompressedTextureSubImage(GLuint, GLint, GLint, GLint, GLint,
                                    GLsizei, GLsizei, GLsizei, GLsizei, void*) {
    Context::Current().errors.Record(0x0502);
}
// EXT_texture_swizzle: Metal code → GL swizzle value (round-trip glGet).
static void SwzArray(const TextureObject& tx, GLint* v) {
    static const GLint kGl[6] = {0x0000 /*ZERO*/, 0x0001 /*ONE*/, 0x1903 /*RED*/,
                                 0x1904 /*GREEN*/, 0x1905 /*BLUE*/, 0x1906 /*ALPHA*/};
    for (int i = 0; i < 4; ++i) v[i] = kGl[tx.swz[i] < 6 ? tx.swz[i] : 2];
}
void glGetTexParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), t);
    auto it = c.textures.find(id);
    if (it == c.textures.end() || !TexMatchesTarget(it->second, t)) {
        c.errors.Record(0x0502); *v = 0; return;
    }
    if (p == 0x8E46 && v) { SwzArray(it->second, v); return; } // SWIZZLE_RGBA (4 pt)
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
    if (p == 0x8E46) { SwzArray(it->second, v); return; } // SWIZZLE_RGBA (4 pt)
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0 : f->second;
}
void glGetTextureParameterfv(GLuint t, GLenum p, GLfloat* v) { GLint x=0; glGetTextureParameteriv(t,p,&x); *v=(GLfloat)x; }
void glGetTextureParameterIiv(GLuint t, GLenum p, GLint* v) { glGetTextureParameteriv(t,p,v); }
void glGetTextureParameterIuiv(GLuint t, GLenum p, GLuint* v) { GLint x=0; glGetTextureParameteriv(t,p,&x); *v=(GLuint)x; }
void glGetTexLevelParameteriv(GLenum t, GLint l, GLenum p, GLint* v) {
    Context& c = Context::Current();
    // PROXY target: trả spec probe (không lỗi) — game dò max texture lúc boot.
    switch (t) {
        case 0x8063: case 0x8064: case 0x8070: case 0x84F7: case 0x851B:
        case 0x8C19: case 0x8C1B: case 0x900B: {
            auto f = c.proxyTex.find(t);
            if (p == 0x1000) *v = (f == c.proxyTex.end()) ? 0 : (GLint)f->second.w;
            else if (p == 0x1001) *v = (f == c.proxyTex.end()) ? 0 : (GLint)f->second.h;
            else if (p == 0x1003)
                *v = (f == c.proxyTex.end()) ? 0 : (GLint)f->second.ifmt;
            else *v = 0;
            return;
        }
        default: break;
    }
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), t);
    auto it = c.textures.find(id);
    if (it == c.textures.end() || !TexMatchesTarget(it->second, t)) {
        c.errors.Record(0x0502); *v = 0; return;
    }
    const auto& tx = it->second;
    // Width/Height theo level thật (vanilla alloc mọi mip rồi query để stitch).
    GLsizei lw = (GLsizei)tx.w, lh = (GLsizei)tx.h;
    if (l > 0) {
        auto mit = tx.mipData.find(l);
        if (mit == tx.mipData.end()) {
            if (p == 0x1000 || p == 0x1001) *v = 0;
            else if (p == 0x1003) *v = (GLint)tx.internalFormat;
            else *v = 0;
            return;
        }
        lw = mit->second.w; lh = mit->second.h;
    } else if (l < 0) { c.errors.Record(0x0501); *v = 0; return; }
    if (p == 0x1000) *v = (GLint)lw;          // TEXTURE_WIDTH
    else if (p == 0x1001) *v = (GLint)lh;     // TEXTURE_HEIGHT
    else if (p == 0x1003) *v = (GLint)tx.internalFormat; // TEXTURE_INTERNAL_FORMAT
    else if (p == 0x9106) *v = (GLint)tx.samples; // TEXTURE_SAMPLES
    else if (p == 0x9107) *v = 1;             // TEXTURE_FIXED_SAMPLE_LOCATIONS
    else *v = 0;
}
void glGetTexLevelParameterfv(GLenum t, GLint l, GLenum p, GLfloat* v) { GLint x=0; glGetTexLevelParameteriv(t,l,p,&x); *v=(GLfloat)x; }
void glGetTextureLevelParameteriv(GLuint t, GLint l, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (l < 0) { c.errors.Record(0x0501); return; }
    if (l > 0) {
        auto mit = it->second.mipData.find(l);
        if (mit == it->second.mipData.end()) {
            if (p == 0x1000 || p == 0x1001) *v = 0;
            else *v = 0;
            return;
        }
        if (p == 0x1000) *v = (GLint)mit->second.w;
        else if (p == 0x1001) *v = (GLint)mit->second.h;
        else *v = 0;
        return;
    }
    if (p == 0x1000) *v = (GLint)it->second.w;
    else if (p == 0x1001) *v = (GLint)it->second.h;
    else if (p == 0x1003) *v = (GLint)it->second.internalFormat;
    else if (p == 0x9106) *v = (GLint)it->second.samples;
    else if (p == 0x9107) *v = 1;
    else *v = 0;
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
// glGetFramebufferParameteriv/Named đã chuyển sang gl_framebuffer.cpp (P10 ARB_framebuffer_no_attachments,
// đọc defaults từ FBO object thật thay vì shadow). Xoá stubs ở đây để khỏi duplicate symbol.
static void RenderbufferComponentSizes(GLenum inf, GLint* red, GLint* green,
                                       GLint* blue, GLint* alpha, GLint* depth,
                                       GLint* stencil) {
    *red = *green = *blue = *alpha = *depth = *stencil = 0;
    switch (inf) {
        case 0x8058: case 0x8C43: case 0x8F97: *red = *green = *blue = *alpha = 8; break;
        case 0x8051: case 0x8F96: case 0x8C41: *red = *green = *blue = 8; break;
        case 0x805B: *red = *green = *blue = *alpha = 16; break;
        case 0x881A: case 0x881B: *red = *green = *blue = *alpha = 16; break;
        case 0x8814: case 0x8815: *red = *green = *blue = *alpha = 32; break;
        case 0x8C3A: *red = 11; *green = 11; *blue = 10; break;
        case 0x8229: case 0x822F: case 0x8231: case 0x8232: *red = 8; break;
        case 0x822B: *red = *green = 8; break;
        case 0x822D: case 0x8233: case 0x8234: *red = 16; break;
        case 0x822E: case 0x8235: case 0x8236: *red = 32; break;
        case 0x81A5: *depth = 16; break;
        case 0x81A6: *depth = 24; break;
        case 0x81A7: case 0x8CAC: *depth = 32; break;
        case 0x88F0: *depth = 24; *stencil = 8; break;
        default: break;
    }
}
static bool RenderbufferParam(RenderbufferObject& rb, GLenum p, GLint* v) {
    GLint r, g, b, a, d, s;
    RenderbufferComponentSizes(rb.internalFormat, &r, &g, &b, &a, &d, &s);
    switch (p) {
        case 0x8D42: *v = rb.w; return true;                 // WIDTH
        case 0x8D43: *v = rb.h; return true;                 // HEIGHT
        case 0x8D44: *v = (GLint)rb.internalFormat; return true;
        case 0x8CAB: *v = rb.samples; return true;           // SAMPLES
        case 0x8D50: *v = r; return true;                     // RED_SIZE
        case 0x8D51: *v = g; return true;                     // GREEN_SIZE
        case 0x8D52: *v = b; return true;                     // BLUE_SIZE
        case 0x8D53: *v = a; return true;                     // ALPHA_SIZE
        case 0x8D54: *v = d; return true;                     // DEPTH_SIZE
        case 0x8D55: *v = s; return true;                     // STENCIL_SIZE
        default: return false;
    }
}
void glGetRenderbufferParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (t != 0x8D41) { c.errors.Record(0x0500); return; }
    GLuint id = 0;
    if (!c.state.GetShadow(0x8CA7 /*RENDERBUFFER_BINDING*/, &id, sizeof(id)) || !id) {
        c.errors.Record(0x0502); return;
    }
    auto it = c.renderbuffers.find(id);
    if (it == c.renderbuffers.end()) { c.errors.Record(0x0502); return; }
    if (!RenderbufferParam(it->second, p, v)) c.errors.Record(0x0500);
}
void glGetNamedRenderbufferParameteriv(GLuint r, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.renderbuffers.find(r);
    if (it == c.renderbuffers.end()) { c.errors.Record(0x0502); return; }
    if (!RenderbufferParam(it->second, p, v)) c.errors.Record(0x0500);
}
// --- VAO getters ---
void glGetVertexAttribiv(GLuint i, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    if (p == 0x8626 /*CURRENT_VERTEX_ATTRIB*/) {
        double d[4] = {0.0, 0.0, 0.0, 1.0};
        c.state.GetShadow(0x8626, d, sizeof(d), i);
        for (int k = 0; k < 4; ++k) v[k] = (GLint)d[k];
        return;
    }
    auto it = c.vaos.find(c.state.BoundVAO());
    if (it == c.vaos.end()) { c.errors.Record(0x0501); return; }
    auto& a = it->second.attribs[i];
    if (p == 0x8622) *v = a.enabled ? 1 : 0;      // VERTEX_ATTRIB_ARRAY_ENABLED
    else if (p == 0x8623) *v = a.size;            // ..._SIZE
    else if (p == 0x8624) *v = a.stride;          // ..._STRIDE
    else if (p == 0x8625) *v = (GLint)a.type;     // ..._ARRAY_TYPE
    else if (p == 0x886A) *v = a.normalized ? 1 : 0;   // ..._ARRAY_NORMALIZED
    else if (p == 0x88FD) *v = (GLint)a.divisor;  // ..._DIVISOR
    else if (p == 0x82EB) *v = (GLint)a.binding;  // VERTEX_ATTRIB_BINDING
    else if (p == 0x82EC) *v = (GLint)a.relativeOffset; // VERTEX_ATTRIB_RELATIVE_OFFSET
    else *v = 0;
}
void glGetVertexAttribfv(GLuint i, GLenum p, GLfloat* v) {
    if (p == 0x8626) {
        Context& c = Context::Current();
        double d[4] = {0.0, 0.0, 0.0, 1.0};
        c.state.GetShadow(0x8626, d, sizeof(d), i);
        for (int k = 0; k < 4; ++k) v[k] = (GLfloat)d[k];
        return;
    }
    GLint x=0; glGetVertexAttribiv(i,p,&x); *v=(GLfloat)x;
}
void glGetVertexAttribdv(GLuint i, GLenum p, GLdouble* v) {
    if (p == 0x8626) {
        Context& c = Context::Current();
        double d[4] = {0.0, 0.0, 0.0, 1.0};
        c.state.GetShadow(0x8626, d, sizeof(d), i);
        for (int k = 0; k < 4; ++k) v[k] = d[k];
        return;
    }
    GLint x=0; glGetVertexAttribiv(i,p,&x); *v=x;
}
void glGetVertexAttribIiv(GLuint i, GLenum p, GLint* v) { glGetVertexAttribiv(i,p,v); }
void glGetVertexAttribIuiv(GLuint i, GLenum p, GLuint* v) {
    if (p == 0x8626) {
        Context& c = Context::Current();
        double d[4] = {0.0, 0.0, 0.0, 1.0};
        c.state.GetShadow(0x8626, d, sizeof(d), i);
        for (int k = 0; k < 4; ++k) v[k] = (GLuint)d[k];
        return;
    }
    GLint x=0; glGetVertexAttribiv(i,p,&x); *v=(GLuint)x;
}
void glGetVertexAttribLdv(GLuint i, GLenum p, GLdouble* v) {
    if (p == 0x8626) { glGetVertexAttribdv(i,p,v); return; }
    GLint x=0; glGetVertexAttribiv(i,p,&x); *v=x;
}
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
    if (n < 0) { c.errors.Record(0x0501); return; }
    GLint out;
    switch (p) {
        case 0x9112: out = 0x9116; break; // OBJECT_TYPE → SYNC_FENCE
        case 0x9113: out = 0x9117; break; // SYNC_CONDITION → GPU_COMMANDS_COMPLETE
        case 0x9114: out = 0x9119; break; // SYNC_STATUS → SIGNALED (Null)
        case 0x9115: out = 0; break;      // SYNC_FLAGS
        default: c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    if (n > 0) v[0] = out;
    if (l) *l = n > 0 ? 1 : 0;
}
// glGetProgramInterfaceiv/Resource* đã chuyển sang gl_program_interface_p10.cpp (P10
// ARB_program_interface_query, introspect từ converter data thật thay vì stub 0).
// Xoá stubs ở đây để khỏi duplicate symbol.
static bool SubStage(GLenum st) {
    return st == 0x8B31 || st == 0x8B30 || st == 0x8DD9 || st == 0x8E88 || st == 0x8E87;
}
void glGetProgramStageiv(GLuint p, GLenum st, GLenum q, GLint* v) {
    Context& c = Context::Current();
    if (!v) return;
    *v = 0;
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !it->second.linked) { c.errors.Record(0x0502); return; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end()) return; // stage không có subroutine
    auto& info = si->second;
    switch (q) {
        case 0x8DE5: *v = (GLint)info.uniforms.size(); break; // ACTIVE_SUBROUTINE_UNIFORMS
        case 0x8DE6: { // ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH
            size_t m = 0;
            for (auto& u : info.uniforms) m = std::max(m, u.first.size());
            *v = (GLint)m;
            break;
        }
        case 0x8DE7: *v = (GLint)info.impls.size(); break; // ACTIVE_SUBROUTINES
        case 0x8DE8: { // ACTIVE_SUBROUTINE_MAX_LENGTH
            size_t m = 0;
            for (auto& n : info.impls) m = std::max(m, n.size());
            *v = (GLint)m;
            break;
        }
        default: c.errors.Record(0x0500); break;
    }
}
GLuint glGetSubroutineIndex(GLuint p, GLenum st, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return 0xFFFFFFFFu; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return 0xFFFFFFFFu; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end() || !n) return 0xFFFFFFFFu;
    auto ii = si->second.implIndex.find(n);
    return ii == si->second.implIndex.end() ? 0xFFFFFFFFu : ii->second;
}
GLint glGetSubroutineUniformLocation(GLuint p, GLenum st, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return -1; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return -1; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end() || !n) return -1;
    for (auto& u : si->second.uniforms)
        if (u.first == n) {
            auto li = it->second.uniformLoc.find(n);
            return li == it->second.uniformLoc.end() ? -1 : li->second;
        }
    return -1;
}
void glGetActiveSubroutineUniformiv(GLuint p, GLenum st, GLuint ix, GLenum q, GLint* v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end() || ix >= si->second.uniforms.size() || !v) {
        c.errors.Record(0x0501); return;
    }
    auto& u = si->second.uniforms[ix];
    auto ti = si->second.typeImpls.find(u.second);
    const std::vector<std::string> empty;
    const auto& list = (ti == si->second.typeImpls.end()) ? empty : ti->second;
    switch (q) {
        case 0x8E4A: v[0] = (GLint)list.size(); break; // NUM_COMPATIBLE_SUBROUTINES
        case 0x8E4B: // COMPATIBLE_SUBROUTINES
            for (size_t i = 0; i < list.size(); ++i) {
                auto gi = si->second.implIndex.find(list[i]);
                v[i] = (gi == si->second.implIndex.end()) ? 0 : (GLint)gi->second;
            }
            break;
        case 0x8A38: v[0] = 1; break; // UNIFORM_SIZE
        case 0x8A39: v[0] = (GLint)u.first.size() + 1; break; // UNIFORM_NAME_LENGTH
        default: c.errors.Record(0x0500); break;
    }
}
void glGetActiveSubroutineUniformName(GLuint p, GLenum st, GLuint ix, GLsizei n,
                                      GLsizei* l, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end() || ix >= si->second.uniforms.size()) {
        c.errors.Record(0x0501); return;
    }
    const std::string& nm = si->second.uniforms[ix].first;
    GLsizei k = (n > 0) ? std::min<GLsizei>(n - 1, (GLsizei)nm.size()) : 0;
    if (name && k > 0) memcpy(name, nm.data(), (size_t)k);
    if (name && n > 0) name[k] = 0;
    if (l) *l = k;
}
void glGetActiveSubroutineName(GLuint p, GLenum st, GLuint ix, GLsizei n,
                               GLsizei* l, GLchar* name) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (!SubStage(st)) { c.errors.Record(0x0500); return; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end() || ix >= si->second.impls.size()) {
        c.errors.Record(0x0501); return;
    }
    const std::string& nm = si->second.impls[ix];
    GLsizei k = (n > 0) ? std::min<GLsizei>(n - 1, (GLsizei)nm.size()) : 0;
    if (name && k > 0) memcpy(name, nm.data(), (size_t)k);
    if (name && n > 0) name[k] = 0;
    if (l) *l = k;
}
void glGetUniformSubroutineuiv(GLenum st, GLint l, GLuint* v) {
    Context& c = Context::Current();
    if (!v) return;
    *v = 0;
    if (!SubStage(st)) { c.errors.Record(0x0500); return; }
    GLuint p = 0, pl = 0;
    if (c.state.GetShadow(0x8254, &pl, 4) && pl) {
        auto pit = c.pipelines.find(pl);
        if (pit != c.pipelines.end()) p = pit->second.activeProgram;
    }
    if (!p) p = c.state.BoundProgram();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto si = it->second.subroutines.find(st);
    if (si == it->second.subroutines.end()) return;
    for (auto& u : si->second.uniforms) {
        auto li = it->second.uniformLoc.find(u.first);
        if (li == it->second.uniformLoc.end() || li->second != l) continue;
        auto ui = it->second.uniforms.find(l);
        GLint ordinal = 0;
        if (ui != it->second.uniforms.end() && ui->second.size() >= 4)
            memcpy(&ordinal, ui->second.data(), 4);
        auto ti = si->second.typeImpls.find(u.second);
        if (ti != si->second.typeImpls.end() && ordinal >= 0 &&
            (size_t)ordinal < ti->second.size()) {
            auto gi = si->second.implIndex.find(ti->second[(size_t)ordinal]);
            if (gi != si->second.implIndex.end()) *v = gi->second;
        }
        return;
    }
}
void glGetUniformIndices(GLuint p, GLsizei n, const GLchar* const* names, GLuint* ix) { (void)p; for(GLsizei i=0;i<n;++i) ix[i]=0xFFFFFFFFu; (void)names; }
// Trả internalformat có HỖ TRỢ THẬT (đúng ToMetalFormat switch — KHÔNG fallback
// RGBA8). Trả false cho format lạ = trung thực (chúng ta không tạo đúng format).
static bool IfmtSupported(GLenum f) {
    switch (f) {
        case 0x8058: // RGBA8
        case 0x8C43: // SRGB8_ALPHA8
        case 0x8229: // R8
        case 0x822B: // RG8
        case 0x822E: // R32F
        case 0x8235: // R32I
        case 0x8236: // R32UI
        case 0x81A5: // DEPTH_COMPONENT16
        case 0x81A6: // DEPTH_COMPONENT24
        case 0x81A7: // DEPTH_COMPONENT32
        case 0x8CAC: // DEPTH_COMPONENT32F
        case 0x88F0: // DEPTH24_STENCIL8
            return true;
        default: return false;
    }
}
static bool IfmtColor(GLenum f) {
    return f==0x8058||f==0x8C43||f==0x8229||f==0x822B||f==0x822E||f==0x8235||f==0x8236;
}
static bool IfmtDepth(GLenum f) {
    return f==0x81A5||f==0x81A6||f==0x81A7||f==0x8CAC||f==0x88F0;
}
static void InternalFormatQuery(GLenum inf, GLenum p, GLint64* v, GLsizei n) {
    auto set = [&](GLint64 x) { for (GLsizei i=0;i<n;++i) v[i]=x; };
    switch (p) {
        case 0x826F: set(IfmtSupported(inf)); break;            // INTERNALFORMAT_SUPPORTED
        case 0x8270: set(inf); break;                           // INTERNALFORMAT_PREFERRED
        case 0x9380: set(1); break;                             // NUM_SAMPLE_COUNTS (không MSAA query)
        case 0x8286: set(IfmtColor(inf)); break;                // COLOR_RENDERABLE
        case 0x8287: set(IfmtDepth(inf)); break;                // DEPTH_RENDERABLE
        // Component sizes — trả đúng cho format map thật, 0 cho lạ.
        case 0x8274: set(inf==0x8058||inf==0x8C43?8:0); break;  // RED_SIZE
        case 0x8275: set(inf==0x8058||inf==0x8C43||inf==0x822B?8:0); break; // GREEN_SIZE
        case 0x8276: set(inf==0x8058||inf==0x8C43||inf==0x822B?8:0); break; // BLUE_SIZE
        case 0x8277: set(inf==0x8058||inf==0x8C43?8:inf==0x8C43?8:0); break; // ALPHA_SIZE
        case 0x8273: set(IfmtSupported(inf)?1:0); break;        // DEPTH_BITS-ish (trung thực)
        default: set(0); break; // chưa implement pnames khác → 0 (không bịa)
    }
}
void glGetInternalformativ(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint* v) {
    (void)t;
    GLint64 tmp[64]; GLsizei m = n > 64 ? 64 : n;
    InternalFormatQuery(inf, p, tmp, m);
    for (GLsizei i=0;i<n;++i) v[i]=(GLint)(i<m?tmp[i]:0);
}
void glGetInternalformati64v(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint64* v) {
    (void)t;
    InternalFormatQuery(inf, p, v, n);
}
static GLsizei DrawSampleCount(Context& c) {
    auto it = c.fbos.find(c.state.BoundDrawFBO());
    if (it == c.fbos.end()) return 1;
    auto& f = it->second;
    if (f.defaultSamples > 0) return (GLsizei)f.defaultSamples;
    auto sampleOf = [&](GLuint texId) -> GLsizei {
        auto t = c.textures.find(texId);
        if (t == c.textures.end()) return 1;
        return (GLsizei)std::max((GLsizei)1, (GLsizei)t->second.samples);
    };
    for (auto& kv : f.colorTex) {
        GLsizei s = sampleOf(kv.second);
        if (s > 1) return s;
    }
    if (f.depthTex) { GLsizei s = sampleOf(f.depthTex); if (s > 1) return s; }
    if (f.depthStencilTex) { GLsizei s = sampleOf(f.depthStencilTex); if (s > 1) return s; }
    return 1;
}
void glGetMultisamplefv(GLenum p, GLuint i, GLfloat* v) {
    Context& c = Context::Current();
    if (!v) return;
    if (p == 0x8E50 /*SAMPLE_LOCATION*/) {
        GLsizei samples = DrawSampleCount(c);
        if (i >= (GLuint)samples) { c.errors.Record(0x0501); return; } // INVALID_VALUE
        float xy[64];
        for (size_t k = 0; k < 64; ++k) xy[k] = 0.5f;
        if (c.device) c.device->getDefaultSamplePositions((uint32_t)samples, xy);
        v[0] = xy[i * 2];
        v[1] = xy[i * 2 + 1];
        return;
    }
    if (p == 0x9341 /*PROGRAMMABLE_SAMPLE_LOCATION_ARB*/) {
        if (i >= 32) { c.errors.Record(0x0501); return; }
        const std::vector<GLfloat>* table = nullptr;
        auto it = c.fbos.find(c.state.BoundDrawFBO());
        if (it != c.fbos.end()) table = &it->second.sampleLocations;
        else table = &c.ext.defaultSampleLocations;
        if (table && (size_t)i * 2 + 1 < table->size()) {
            v[0] = (*table)[i * 2];
            v[1] = (*table)[i * 2 + 1];
        } else {
            v[0] = v[1] = 0.5f;
        }
        return;
    }
    c.errors.Record(0x0500); // INVALID_ENUM
}
void glGetShaderPrecisionFormat(GLenum st, GLenum p, GLint* r, GLint* v) {
    Context& c = Context::Current();
    if (st != 0x8B31 /*VERTEX_SHADER*/ && st != 0x8B30 /*FRAGMENT_SHADER*/ &&
        st != 0x91B9 /*COMPUTE_SHADER*/) {
        c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    if (p != 0x8DF0 && p != 0x8DF1 && p != 0x8DF2 && // LOW/MEDIUM/HIGH_FLOAT
        p != 0x8DF3 && p != 0x8DF4 && p != 0x8DF5) {  // LOW/MEDIUM/HIGH_INT
        c.errors.Record(0x0500); return;
    }
    // Metal maps all three GLSL precision qualifiers to IEEE-754 binary32 or
    // 32-bit integers. GL 4.6 §7.15: range = exponent range, precision = bits.
    if (p == 0x8DF0 || p == 0x8DF1 || p == 0x8DF2) {
        if (r) { r[0] = 127; r[1] = 127; }
        if (v) *v = 23;
    } else {
        if (r) { r[0] = 31; r[1] = 31; }
        if (v) *v = 0;
    }
}
} // namespace tglmt::gl
