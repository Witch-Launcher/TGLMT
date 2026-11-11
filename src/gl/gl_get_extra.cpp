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
// --- Compressed texture: helper dùng chung với gl_texture.cpp ---
// (bảng format nén + layout shadow nén nằm ở gl_texture.cpp)
bool IsCompressedInternalFormat(GLenum ifmt);
size_t CompressedLevelImageSize(GLenum ifmt, GLsizei w, GLsizei h, GLsizei d);
void CompressedBlockDims(GLenum ifmt, GLint* bw, GLint* bh);
GLint CompressedBlockBytes(GLenum ifmt);
const std::vector<uint8_t>* CompressedLevelData(const TextureObject& tx, int face, GLint level);
void CompressedLevelExtent(const TextureObject& tx, int face, GLint level, GLsizei* w,
                            GLsizei* h, GLsizei* d);

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
    // Texture lưu ở dạng NÉN: shadow chứa block nén chứ không phải texel →
    // GetTexImage không giải mã được (spec: format nén không "Copyable").
    if (IsCompressedInternalFormat(tx.internalFormat)) { c.errors.Record(0x0502); return; }
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
// --- Get*CompressedTexImage (GL 4.6 §8.11) ---
// TGLMT giữ bytes nén GỐC trong shadow CPU → trả lại đúng byte-for-byte, đúng
// spec §8.7 (dữ liệu trả về phải nạp lại được vào texture cùng format).
// Copy ra client memory hoặc PIXEL_PACK_BUFFER (pixels = offset trong PBO).
static bool CompressedCopyOut(Context& c, const uint8_t* src, size_t n, void* pixels) {
    if (!n) return true;
    GLuint packBuf = c.state.BoundBuffer(0x88EB /*PIXEL_PACK_BUFFER*/);
    if (packBuf) {
        auto bit = c.buffers.find(packBuf);
        if (bit == c.buffers.end()) { c.errors.Record(0x0502); return false; }
        size_t off = (size_t)pixels;
        std::vector<uint8_t>* pbo = c.MutablePbo(packBuf, off + n);
        if (!pbo) { c.errors.Record(0x0502); return false; }
        memcpy(pbo->data() + off, src, n);
        // PBO: đồng bộ GPU buffer để Map/GetBufferSubData sau thấy dữ liệu mới.
        if (!bit->second.data.empty()) {
            size_t sz = bit->second.data.size();
            auto nb = c.device->newBufferWithBytes(bit->second.data.data(), sz,
                                                   metal::StorageMode::Shared);
            if (nb) bit->second.gpu = nb;
        }
        return true;
    }
    if (!pixels) { c.errors.Record(0x0501); return false; }
    memcpy(pixels, src, n);
    return true;
}
// Đọc ảnh nén (toàn level hoặc vùng con) ra buffer.
// w=h=d=0 → lấy TOÀN level. bounded=true → kiểm tra bufSize (GL 4.5).
static void CompressedGetImpl(Context& c, TextureObject& tx, int face, GLint level,
        GLint xoff, GLint yoff, GLint zoff, GLsizei w, GLsizei h, GLsizei d,
        GLsizei bufSize, void* pixels, bool bounded) {
    // Texture không lưu ở dạng nén → không có gì để trả (spec §8.11).
    if (!IsCompressedInternalFormat(tx.internalFormat)) { c.errors.Record(0x0502); return; }
    if (level < 0 || level >= tx.levels) { c.errors.Record(0x0501); return; }
    GLsizei lw, lh, ld;
    CompressedLevelExtent(tx, face, level, &lw, &lh, &ld);
    const std::vector<uint8_t>* data = CompressedLevelData(tx, face, level);
    size_t levelSize = CompressedLevelImageSize(tx.internalFormat, lw, lh, ld);
    if (!data || data->size() < levelSize) { c.errors.Record(0x0502); return; }
    GLint bw = 0, bh = 0;
    CompressedBlockDims(tx.internalFormat, &bw, &bh);
    GLint bbytes = CompressedBlockBytes(tx.internalFormat);
    if (bw <= 0 || bh <= 0 || bbytes <= 0) { c.errors.Record(0x0502); return; }
    if (tx.isCube && face < 0) {
        // Cube map: hành vi như gọi GetCompressedTexImage cho từng face (spec §8.11).
        std::vector<uint8_t> all;
        for (int f = 0; f < 6; ++f) {
            GLsizei fw, fh, fd;
            CompressedLevelExtent(tx, f, level, &fw, &fh, &fd);
            const std::vector<uint8_t>* fd8 = CompressedLevelData(tx, f, level);
            size_t sz = CompressedLevelImageSize(tx.internalFormat, fw, fh, fd);
            if (!fd8 || fd8->size() < sz) { c.errors.Record(0x0502); return; }
            all.insert(all.end(), fd8->begin(), fd8->begin() + (long)sz);
        }
        if (bounded && (size_t)bufSize < all.size()) { c.errors.Record(0x0502); return; }
        CompressedCopyOut(c, all.data(), all.size(), pixels);
        return;
    }
    if (!w && !h && !d) {
        // Toàn level.
        if (bounded && (size_t)bufSize < levelSize) { c.errors.Record(0x0502); return; }
        CompressedCopyOut(c, data->data(), levelSize, pixels);
        return;
    }
    // Vùng con: offset/kích thước phải canh block (spec §8.11).
    if (xoff < 0 || yoff < 0 || zoff < 0 || w < 0 || h < 0 || d < 0) {
        c.errors.Record(0x0501); return;
    }
    if (xoff + w > lw || yoff + h > lh || zoff + d > ld) { c.errors.Record(0x0501); return; }
    bool full = (xoff == 0 && yoff == 0 && zoff == 0 && w == lw && h == lh && d == ld);
    if (!full && (xoff % bw || yoff % bh || w % bw || h % bh)) {
        c.errors.Record(0x0501); return;
    }
    uint32_t lvlBpr = (uint32_t)((lw + bw - 1) / bw);
    uint32_t lvlBpc = (uint32_t)((lh + bh - 1) / bh);
    uint32_t bx0 = (uint32_t)xoff / bw, by0 = (uint32_t)yoff / bh;
    uint32_t nbw = (uint32_t)((w + bw - 1) / bw), nbh = (uint32_t)((h + bh - 1) / bh);
    size_t sliceBytes = (size_t)lvlBpr * lvlBpc * bbytes;
    size_t n = (size_t)nbw * nbh * (d > 0 ? (size_t)d : 1) * bbytes;
    if (bounded && (size_t)bufSize < n) { c.errors.Record(0x0502); return; }
    std::vector<uint8_t> out;
    out.resize(n);
    for (GLsizei z = 0; z < (d > 0 ? d : 1); ++z) {
        const uint8_t* srcSlice = data->data() + (size_t)z * sliceBytes;
        uint8_t* dstSlice = out.data() + (size_t)z * nbw * nbh * bbytes;
        for (uint32_t r = 0; r < nbh; ++r) {
            const uint8_t* s = srcSlice + ((size_t)(by0 + r) * lvlBpr + bx0) * bbytes;
            uint8_t* d = dstSlice + (size_t)r * nbw * bbytes;
            memcpy(d, s, (size_t)nbw * bbytes);
        }
    }
    CompressedCopyOut(c, out.data(), n, pixels);
}
void glGetCompressedTexImage(GLenum target, GLint level, void* img) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), target);
    auto it = c.textures.find(id);
    int face = -1;
    bool isFace = CubeFaceIndex(target, &face);
    if (it == c.textures.end() ||
        (it->second.target != target && !(isFace && it->second.target == 0x8513))) {
        c.errors.Record(0x0502); return;
    }
    CompressedGetImpl(c, it->second, face, level, 0, 0, 0, 0, 0, 0, 0, img, false);
}
void glGetnCompressedTexImage(GLenum target, GLint level, GLsizei bufSize, void* pixels) {
    Context& c = Context::Current();
    if (bufSize < 0) { c.errors.Record(0x0501); return; }
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), target);
    auto it = c.textures.find(id);
    int face = -1;
    bool isFace = CubeFaceIndex(target, &face);
    if (it == c.textures.end() ||
        (it->second.target != target && !(isFace && it->second.target == 0x8513))) {
        c.errors.Record(0x0502); return;
    }
    CompressedGetImpl(c, it->second, face, level, 0, 0, 0, 0, 0, 0, bufSize, pixels, true);
}
void glGetCompressedTextureImage(GLuint texture, GLint level, GLsizei bufSize, void* pixels) {
    Context& c = Context::Current();
    auto it = c.textures.find(texture);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (bufSize < 0) { c.errors.Record(0x0501); return; }
    CompressedGetImpl(c, it->second, -1, level, 0, 0, 0, 0, 0, 0, bufSize, pixels, true);
}
void glGetCompressedTextureSubImage(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
                                    GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                    GLsizei bufSize, void* pixels) {
    Context& c = Context::Current();
    auto it = c.textures.find(texture);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (bufSize < 0) { c.errors.Record(0x0501); return; }
    CompressedGetImpl(c, it->second, -1, level, xoffset, yoffset, zoffset, width, height, depth,
                      bufSize, pixels, true);
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
    // Kích thước block của format nén (spec Table 8.14): 0 nếu texture không nén.
    if (p == 0x82B1 || p == 0x82B2 || p == 0x82B3) {
        GLint bw = 0, bh = 0;
        CompressedBlockDims(it->second.internalFormat, &bw, &bh);
        if (p == 0x82B1) *v = bw;                     // COMPRESSED_BLOCK_WIDTH
        else if (p == 0x82B2) *v = bh;                // COMPRESSED_BLOCK_HEIGHT
        else *v = CompressedBlockBytes(it->second.internalFormat); // BLOCK_SIZE
        return;
    }
    // Float-valued params (TEXTURE_MAX_ANISOTROPY) lưu riêng → đọc từ đó.
    auto ff = it->second.fparams.find(p);
    if (ff != it->second.fparams.end()) { *v = (GLint)ff->second; return; }
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0 : f->second;
}
void glGetTexParameterfv(GLenum t, GLenum p, GLfloat* v) {
    Context& c = Context::Current();
    GLuint id = c.state.BoundTexture(c.state.ActiveTexture(), t);
    auto it = c.textures.find(id);
    if (it != c.textures.end() && TexMatchesTarget(it->second, t)) {
        // GL_TEXTURE_BORDER_COLOR (0x1004) là float[4] — 4 thành phần nằm ở
        // 0x1004..0x1007 (quy ước lưu, xem glTexParameterfv).
        if (p == 0x1004 /*GL_TEXTURE_BORDER_COLOR*/) {
            for (int i = 0; i < 4; ++i) {
                auto ff = it->second.fparams.find(0x1004 + i);
                v[i] = (ff == it->second.fparams.end()) ? 0.0f : ff->second;
            }
            return;
        }
        auto ff = it->second.fparams.find(p);
        if (ff != it->second.fparams.end()) { *v = ff->second; return; }
    }
    GLint x=0; glGetTexParameteriv(t,p,&x); *v=(GLfloat)x;
}
void glGetTexParameterIiv(GLenum t, GLenum p, GLint* v) { glGetTexParameteriv(t,p,v); }
void glGetTexParameterIuiv(GLenum t, GLenum p, GLuint* v) { GLint x=0; glGetTexParameteriv(t,p,&x); *v=(GLuint)x; }
void glGetTextureParameteriv(GLuint t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (p == 0x8E46) { SwzArray(it->second, v); return; } // SWIZZLE_RGBA (4 pt)
    // Kích thước block của format nén (spec Table 8.14): 0 nếu texture không nén.
    if (p == 0x82B1 || p == 0x82B2 || p == 0x82B3) {
        GLint bw = 0, bh = 0;
        CompressedBlockDims(it->second.internalFormat, &bw, &bh);
        if (p == 0x82B1) *v = bw;
        else if (p == 0x82B2) *v = bh;
        else *v = CompressedBlockBytes(it->second.internalFormat);
        return;
    }
    auto ff = it->second.fparams.find(p);
    if (ff != it->second.fparams.end()) { *v = (GLint)ff->second; return; }
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0 : f->second;
}
void glGetTextureParameterfv(GLuint t, GLenum p, GLfloat* v) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); *v = 0; return; }
    // GL_TEXTURE_BORDER_COLOR là float[4] (4 thành phần nằm ở 0x1004..0x1007).
    if (p == 0x1004 /*GL_TEXTURE_BORDER_COLOR*/) {
        for (int i = 0; i < 4; ++i) {
            auto ff = it->second.fparams.find(0x1004 + i);
            v[i] = (ff == it->second.fparams.end()) ? 0.0f : ff->second;
        }
        return;
    }
    // Float-valued params phải đọc từ fparams — đọc qua bản int sẽ CẮT mất
    // phần thập phân (vd GL_TEXTURE_LOD_BIAS 0.5 → 0).
    auto ff = it->second.fparams.find(p);
    if (ff != it->second.fparams.end()) { *v = ff->second; return; }
    auto f = it->second.params.find(p);
    *v = (f == it->second.params.end()) ? 0.0f : (GLfloat)f->second;
}
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
    // Kích thước ảnh nén của level (spec §8.11): 0 nếu texture không nén.
    else if (p == 0x86A0) // TEXTURE_COMPRESSED_IMAGE_SIZE
        *v = (GLint)CompressedLevelImageSize(tx.internalFormat, lw, lh,
                                              tx.d ? (GLsizei)tx.d : 1);
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
    else if (p == 0x8071) *v = (GLint)it->second.d;   // GL_TEXTURE_DEPTH (3D/array)
    else if (p == 0x8848) *v = (GLint)it->second.d;   // GL_TEXTURE_DEPTH_SIZE
    else if (p == 0x1003) *v = (GLint)it->second.internalFormat;
    else if (p == 0x9106) *v = (GLint)it->second.samples;
    else if (p == 0x9107) *v = 1;
    else if (p == 0x86A0) // TEXTURE_COMPRESSED_IMAGE_SIZE
        *v = (GLint)CompressedLevelImageSize(it->second.internalFormat,
                                              (GLsizei)it->second.w,
                                              (GLsizei)it->second.h,
                                              it->second.d ? (GLsizei)it->second.d : 1);
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
// glGetFramebufferAttachmentParameteriv — đọc attachment thật từ FBO
// (colorTex/depthTex/stencilTex/depthStencilTex + level/layer).
// fbo==0 → FBO đang bind (quy ước TGLMT: first-FBO nếu không track bound).
static void FboAttachParam(Context& c, GLuint fbo, GLenum a, GLenum p, GLint* v) {
    if (!v) return;
    if (fbo == 0) {
        if (c.fbos.empty()) { *v = 0; return; }
        fbo = c.fbos.begin()->first;
    }
    auto fit = c.fbos.find(fbo);
    if (fit == c.fbos.end()) { c.errors.Record(0x0501); *v = 0; return; }
    auto& f = fit->second;
    GLuint obj = 0; GLint level = 0, layer = -1; GLenum objType = 0; // 0 = NONE
    // QUAN TRỌNG: FBO lưu attachment theo INDEX DÀY (0..7) còn hàm này nhận
    // ENUM (GL_COLOR_ATTACHMENT0..7). Trước đây tra bằng enum ⇒ không tìm
    // thấy ⇒ OBJECT_TYPE luôn NONE (bug M3 trong audit chưa thực sự được sửa).
    if (a >= 0x8CE0 && a <= 0x8CE7) { // COLOR_ATTACHMENT0..7
        const GLenum k = a - 0x8CE0;
        auto ct = f.colorTex.find(k);
        if (ct != f.colorTex.end() && ct->second) {
            obj = ct->second;
            auto ot = f.colorObjType.find(k);
            objType = (ot != f.colorObjType.end()) ? ot->second : 0x8DE1; // TEXTURE
            auto on = f.colorObjName.find(k);
            obj = (on != f.colorObjName.end()) ? on->second : ct->second;
            auto lv = f.colorLevel.find(k);
            level = (lv != f.colorLevel.end()) ? lv->second : 0;
            auto ly = f.colorLayer.find(k);
            layer = (ly != f.colorLayer.end()) ? ly->second : -1;
        }
    } else if (a == 0x8D00 /*DEPTH*/) {
        if (f.depthTex) {
            obj = f.depthObjName ? f.depthObjName : f.depthTex;
            objType = f.depthObjType ? f.depthObjType : 0x8DE1;
            level = f.depthLevel; layer = f.depthLayer;
        }
    } else if (a == 0x8D20 /*STENCIL*/) {
        if (f.stencilTex) {
            obj = f.stencilObjName ? f.stencilObjName : f.stencilTex;
            objType = f.stencilObjType ? f.stencilObjType : 0x8DE1;
            level = f.stencilLevel; layer = f.stencilLayer;
        }
    } else if (a == 0x821A /*DEPTH_STENCIL*/) {
        if (f.depthStencilTex) {
            obj = f.depthStencilObjName ? f.depthStencilObjName : f.depthStencilTex;
            objType = f.depthStencilObjType ? f.depthStencilObjType : 0x8DE1;
            level = f.depthStencilLevel; layer = f.depthStencilLayer;
        }
    }
    switch (p) {
        case 0x8CD0: *v = (GLint)objType; break;              // OBJECT_TYPE (TEXTURE=0x8DE1 / NONE=0)
        case 0x8CD1: *v = (GLint)obj; break;                  // OBJECT_NAME
        case 0x8CD2: *v = level; break;                       // TEXTURE_LEVEL
        case 0x8CD3: *v = 0; break;                           // CUBE_MAP_FACE (TEXTURE_2D → 0)
        case 0x8CD4: *v = layer; break;                       // TEXTURE_LAYER
        default: *v = 0; break;
    }
}
void glGetFramebufferAttachmentParameteriv(GLenum t, GLenum a, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (!v) return;
    // Chỉ FBO (GL_FRAMEBUFFER/READ/DRAW). Default FB → INVALID_OPERATION.
    if (t != 0x8D40 && t != 0x8CA9 && t != 0x8CA8) {
        c.errors.Record(0x0502); *v = 0; return;
    }
    FboAttachParam(c, 0, a, p, v);
}
void glGetNamedFramebufferAttachmentParameteriv(GLuint f, GLenum a, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); if (v) *v = 0; return; }
    FboAttachParam(c, f, a, p, v);
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
    else if (p == 0x88FE) *v = (GLint)a.divisor;  // ..._DIVISOR (GL_VERTEX_ATTRIB_ARRAY_DIVISOR = 0x88FE)
    // GL_ARB_vertex_attrib_binding: enum đúng là 0x82D4/0x82D5 (trước đây dùng
    // nhầm 0x82EB/0x82EC → query trả 0 cho đúng thuộc tính).
    else if (p == 0x82D4) *v = (GLint)a.binding;  // VERTEX_ATTRIB_BINDING
    else if (p == 0x82D5) *v = (GLint)a.relativeOffset; // VERTEX_ATTRIB_RELATIVE_OFFSET
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
    Context& c = Context::Current();
    // GL 4.6 §10.3: pname phải là VERTEX_ATTRIB_ARRAY_POINTER (0x8898F) →
    // nếu không thì INVALID_ENUM.
    if (p != 0x8645 /*GL_VERTEX_ATTRIB_ARRAY_POINTER*/) {
        c.errors.Record(0x0500);
        *v = nullptr;
        return;
    }
    auto it = c.vaos.find(c.state.BoundVAO());
    if (it == c.vaos.end() || i >= 16) { c.errors.Record(0x0501); *v=nullptr; return; }
    const VertexAttrib& a = it->second.attribs[i];
    if (!a.buffer) { *v = nullptr; return; }
    auto bit = c.buffers.find(a.buffer);
    if (bit == c.buffers.end()) { *v = nullptr; return; }
    // Trả về địa chỉ bộ nhớ đang được ánh xạ cho attribute (spec: "the address
    // of the memory currently mapped to the given attribute").
    uint8_t* base = nullptr;
    if (bit->second.gpu) base = (uint8_t*)bit->second.gpu->contents();
    else if (!bit->second.data.empty()) base = bit->second.data.data();
    *v = base ? (void*)(base + a.offset) : nullptr;
}
// GL 4.6 §10.3/§10.4 — GetVertexArrayiv / GetVertexArrayIndexediv /
// GetVertexArrayIndexed64iv: trả state THẬT của VAO (trước đây luôn trả 0
// ⇒ app không đọc được trạng thái VAO).
//   pname cho dạng indexed: thuộc tính của attribute `index`.
//   pname cho dạng không indexed: ELEMENT_ARRAY_BUFFER_BINDING.
static bool VertexArrayAttribPname(GLenum p, int& kind, GLint* out) {
    // kind: 0=enabled 1=size 2=stride 3=type 4=normalized 5=integer
    //       6=divisor 7=binding 8=relative_offset
    switch (p) {
        case 0x8622: kind = 0; return true;  // VERTEX_ATTRIB_ARRAY_ENABLED
        case 0x8623: kind = 1; return true;  // ..._SIZE
        case 0x8624: kind = 2; return true;  // ..._STRIDE
        case 0x8625: kind = 3; return true;  // ..._ARRAY_TYPE
        case 0x886A: kind = 4; return true;  // ..._ARRAY_NORMALIZED
        case 0x88FE: kind = 5; return true;  // ..._DIVISOR (0x88FE)
        case 0x82D4: kind = 6; return true;  // VERTEX_ATTRIB_BINDING
        case 0x82D5: kind = 7; return true;  // VERTEX_ATTRIB_RELATIVE_OFFSET
        default: return false;
    }
}
static void VertexArrayAttribValue(const VertexAttrib& a, int kind, GLint* out) {
    switch (kind) {
        case 0: *out = a.enabled ? 1 : 0; break;
        case 1: *out = a.size; break;
        case 2: *out = a.stride; break;
        case 3: *out = (GLint)a.type; break;
        case 4: *out = a.normalized ? 1 : 0; break;
        case 5: *out = a.divisor; break;
        case 6: *out = (GLint)a.binding; break;
        case 7: *out = (GLint)a.relativeOffset; break;
        default: *out = 0; break;
    }
}
void glGetVertexArrayiv(GLuint v, GLenum p, GLint* x) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); *x = 0; return; }
    // Không indexed: chỉ ELEMENT_ARRAY_BUFFER_BINDING hợp lệ.
    // GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8895 (0x889F4 là giá trị KHÔNG hợp lệ
    // → trước đây nhánh này không bao giờ chạy ⇒ luôn trả 0 + INVALID_ENUM).
    if (p == 0x8895 /*ELEMENT_ARRAY_BUFFER_BINDING*/) { *x = (GLint)it->second.elementBuffer; return; }
    int kind = 0;
    if (!VertexArrayAttribPname(p, kind, x)) { c.errors.Record(0x0500); *x = 0; return; }
    // Dạng không indexed lấy attribute 0.
    VertexArrayAttribValue(it->second.attribs[0], kind, x);
}
void glGetVertexArrayIndexediv(GLuint v, GLuint i, GLenum p, GLint* x) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); *x = 0; return; }
    int kind = 0;
    if (!VertexArrayAttribPname(p, kind, x)) { c.errors.Record(0x0500); *x = 0; return; }
    if (i >= it->second.attribs.size()) { c.errors.Record(0x0501); *x = 0; return; }
    VertexArrayAttribValue(it->second.attribs[i], kind, x);
}
void glGetVertexArrayIndexed64iv(GLuint v, GLuint i, GLenum p, GLint64* x) {
    Context& c = Context::Current();
    auto it = c.vaos.find(v);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); *x = 0; return; }
    if (i >= it->second.attribs.size()) { c.errors.Record(0x0501); *x = 0; return; }
    // Dạng 64-bit chỉ dùng cho VERTEX_ATTRIB_RELATIVE_OFFSET.
    if (p != 0x82D5) { c.errors.Record(0x0500); *x = 0; return; }
    *x = (GLint64)it->second.attribs[i].relativeOffset;
}
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
    if (c.state.GetShadow(0x825A /*PROGRAM_PIPELINE_BINDING*/, &pl, 4) && pl) {
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
        if (ui != it->second.uniforms.end() && ui->second.bytes.size() >= 4)
            memcpy(&ordinal, ui->second.bytes.data(), 4);
        auto ti = si->second.typeImpls.find(u.second);
        if (ti != si->second.typeImpls.end() && ordinal >= 0 &&
            (size_t)ordinal < ti->second.size()) {
            auto gi = si->second.implIndex.find(ti->second[(size_t)ordinal]);
            if (gi != si->second.implIndex.end()) *v = gi->second;
        }
        return;
    }
}
// glGetUniformIndices — tên → index vào activeUniforms (spec §7.10).
// Không tìm thấy → GL_INVALID_INDEX (0xFFFFFFFF). Mảng tên có thể lack "[0]"
// nên thử cả name và name+"[0]".
void glGetUniformIndices(GLuint p, GLsizei n, const GLchar* const* names, GLuint* ix) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (!ix) return;
    auto& aus = it->second.activeUniforms;
    for (GLsizei i = 0; i < n; ++i) {
        ix[i] = 0xFFFFFFFFu;
        if (!names || !names[i]) continue;
        std::string want = names[i];
        for (size_t k = 0; k < aus.size(); ++k) {
            if (aus[k].name == want) { ix[i] = (GLuint)k; break; }
            // name "arr" khớp active "arr[0]"
            if (aus[k].name.size() == want.size() + 3 &&
                aus[k].name.compare(0, want.size(), want) == 0 &&
                aus[k].name.compare(want.size(), 3, "[0]") == 0) {
                ix[i] = (GLuint)k; break;
            }
        }
    }
}
// --- ARB_internalformat_query / ARB_internalformat_query2 (glspec46.core.pdf §22.3) ---
// Bảng format: PHẢI khớp switch ToMetalFormat của gl_texture.cpp — đây là nguồn
// sự thật về format TGLMT tạo được (trước đây bảng ở đây thiếu nhiều format
// nên GL_INTERNALFORMAT_SUPPORTED trả sai).
struct IfmtDesc {
    GLenum fmt;
    int r, g, b, a;       // số bit component màu (0 = format không có)
    int depth, stencil;   // số bit depth/stencil
    GLenum compType;      // kiểu lưu component (GL_UNSIGNED_NORMALIZED/…)
    bool colorRender, depthRender, stencilRender;
    bool filterable;      // Metal: format unorm/float16 lọc được, integer/float32 thì không
};
static const IfmtDesc kIfmt[] = {
    // fmt,                       r,  g,  b,  a, depth, sten, compType,          color, depth, sten, filter
    {0x8058, 8, 8, 8, 8,  0, 0, 0x8C17, true,  false, false, true},  // RGBA8
    {0x8C43, 8, 8, 8, 8,  0, 0, 0x8C17, true,  false, false, true},  // SRGB8_ALPHA8
    {0x8C41, 8, 8, 8, 0,  0, 0, 0x8C17, true,  false, false, true},  // SRGB8
    {0x8051, 8, 8, 8, 0,  0, 0, 0x8C17, true,  false, false, true},  // RGB8
    {0x8F96, 8, 8, 8, 0,  0, 0, 0x8F9C, true,  false, false, true},  // RGB8_SNORM
    {0x8F97, 8, 8, 8, 8,  0, 0, 0x8F9C, true,  false, false, true},  // RGBA8_SNORM
    {0x8229, 8, 0, 0, 0,  0, 0, 0x8C17, true,  false, false, true},  // R8
    {0x822F, 8, 0, 0, 0,  0, 0, 0x8F9C, true,  false, false, true},  // R8_SNORM
    {0x822B, 8, 8, 0, 0,  0, 0, 0x8C17, true,  false, false, true},  // RG8
    {0x805B, 16,16,16,16, 0, 0, 0x8C17, true,  false, false, true},  // RGBA16
    {0x881A, 16,16,16,16, 0, 0, 0x1406, true,  false, false, true},  // RGBA16F
    {0x881B, 16,16,16,0,  0, 0, 0x1406, true,  false, false, true},  // RGB16F
    {0x8C3A, 11,11,10,0,  0, 0, 0x1406, true,  false, false, true},  // R11F_G11F_B10F
    {0x822D, 16, 0, 0, 0,  0, 0, 0x1406, true,  false, false, true},  // R16F
    {0x8814, 32,32,32,32, 0, 0, 0x1406, true,  false, false, false}, // RGBA32F
    {0x8815, 32,32,32,0,  0, 0, 0x1406, true,  false, false, false}, // RGB32F
    {0x822E, 32, 0, 0, 0,  0, 0, 0x1406, true,  false, false, false}, // R32F
    {0x8231, 8, 0, 0, 0,   0, 0, 0x1404, true,  false, false, false}, // R8I
    {0x8232, 8, 0, 0, 0,   0, 0, 0x1405, true,  false, false, false}, // R8UI
    {0x8233, 16, 0, 0, 0,  0, 0, 0x1404, true,  false, false, false}, // R16I
    {0x8234, 16, 0, 0, 0,  0, 0, 0x1405, true,  false, false, false}, // R16UI
    {0x8235, 32, 0, 0, 0,  0, 0, 0x1404, true,  false, false, false}, // R32I
    {0x8236, 32, 0, 0, 0,  0, 0, 0x1405, true,  false, false, false}, // R32UI
    {0x81A5, 0, 0, 0, 0,  16, 0, 0x1406, false, true,  false, false}, // DEPTH_COMPONENT16
    {0x81A6, 0, 0, 0, 0,  24, 0, 0x1406, false, true,  false, false}, // DEPTH_COMPONENT24
    {0x81A7, 0, 0, 0, 0,  32, 0, 0x1406, false, true,  false, false}, // DEPTH_COMPONENT32
    {0x8CAC, 0, 0, 0, 0,  32, 0, 0x1406, false, true,  false, false}, // DEPTH_COMPONENT32F
    {0x88F0, 0, 0, 0, 0,  24, 8, 0x1406, false, true,  true,  false}, // DEPTH24_STENCIL8
};
static const IfmtDesc* IfmtDescFor(GLenum f) {
    for (const auto& d : kIfmt) if (d.fmt == f) return &d;
    return nullptr;
}
static bool IfmtSupported(GLenum f) { return IfmtDescFor(f) != nullptr; }
static bool IfmtColor(GLenum f) { const auto* d = IfmtDescFor(f); return d && d->colorRender; }
static bool IfmtDepth(GLenum f) { const auto* d = IfmtDescFor(f); return d && d->depthRender; }
static bool IfmtStencil(GLenum f) { const auto* d = IfmtDescFor(f); return d && d->stencilRender; }

// Target hợp lệ (table 22.2): 11 texture target + RENDERBUFFER.
static bool IfmtTargetOk(GLenum t) {
    switch (t) {
        case 0x0DE0: // TEXTURE_1D
        case 0x8C18: // TEXTURE_1D_ARRAY
        case 0x0DE1: // TEXTURE_2D
        case 0x8C1A: // TEXTURE_2D_ARRAY
        case 0x9100: // TEXTURE_2D_MULTISAMPLE
        case 0x9102: // TEXTURE_2D_MULTISAMPLE_ARRAY
        case 0x806F: // TEXTURE_3D
        case 0x8C2A: // TEXTURE_BUFFER
        case 0x8513: // TEXTURE_CUBE_MAP
        case 0x9009: // TEXTURE_CUBE_MAP_ARRAY
        case 0x84F5: // TEXTURE_RECTANGLE
        case 0x8D41: // RENDERBUFFER
            return true;
        default: return false;
    }
}
// Target hỗ trợ multisample: SAMPLES/NUM_SAMPLE_COUNTS chỉ có nghĩa với
// RENDERBUFFER, TEXTURE_2D_MULTISAMPLE, TEXTURE_2D_MULTISAMPLE_ARRAY
// (ARB_internalformat_query2: target khác → NUM_SAMPLE_COUNTS = 0).
static bool IfmtTargetMultiSample(GLenum t) {
    return t == 0x8D41 || t == 0x9100 || t == 0x9102;
}
// pname hợp lệ (group InternalFormatPName của gl.xml + DEPTH/STENCIL_COMPONENTS
// của ARB_internalformat_query2) — ngoài danh sách này → GL_INVALID_ENUM.
static bool IfmtPnameOk(GLenum p) {
    switch (p) {
        case 0x80A9: // SAMPLES
        case 0x8191: // GENERATE_MIPMAP
        case 0x826F: case 0x8270: case 0x8271: case 0x8272: case 0x8273:
        case 0x8274: case 0x8275: case 0x8276: case 0x8277: case 0x8278:
        case 0x8279: case 0x827A: case 0x827B: case 0x827C: case 0x827D:
        case 0x827E: case 0x827F: case 0x8280: case 0x8281: case 0x8282:
        case 0x8283: case 0x8284: case 0x8285: case 0x8286: case 0x8287:
        case 0x8288: case 0x8289: case 0x828A: case 0x828B: case 0x828C:
        case 0x828D: case 0x828E: case 0x828F: case 0x8290: case 0x8291:
        case 0x8292: case 0x8293: case 0x8295: case 0x8296: case 0x8297:
        case 0x8298: case 0x829A: case 0x829B: case 0x829C: case 0x829D:
        case 0x829E: case 0x829F: case 0x82A0: case 0x82A1: case 0x82A2:
        case 0x82A3: case 0x82A4: case 0x82A5: case 0x82A6: case 0x82A7:
        case 0x82A8: case 0x82A9: case 0x82AA: case 0x82AC: case 0x82AD:
        case 0x82AE: case 0x82AF: case 0x82B1: case 0x82B2: case 0x82B3:
        case 0x82B4: case 0x82B5: case 0x82B6: case 0x86A1: case 0x8F6E:
        case 0x90C7: case 0x9365: case 0x9380: // NUM_SAMPLE_COUNTS
            return true;
        default: return false;
    }
}
// Trả số giá trị đã ghi vào v (scalar → 1, SAMPLES → số sample count), hoặc -1
// nếu pname không hợp lệ (→ GL_INVALID_ENUM). Spec §22.3: "No more than count
// integers will be written into params" → không ghi thừa ra ngoài count.
static int InternalFormatQuery(GLenum target, GLenum inf, GLenum p, GLint64* v, GLsizei n) {
    if (!IfmtPnameOk(p)) return -1;
    if (n <= 0) return 0;
    const IfmtDesc* d = IfmtDescFor(inf);
    const bool supported = d != nullptr;
    const bool renderable = supported && (d->colorRender || d->depthRender || d->stencilRender);
    const bool layered = target == 0x8C18 || target == 0x8C1A || target == 0x9009 ||
                         target == 0x806F /*TEXTURE_3D*/;
    auto one = [&](GLint64 x) { v[0] = x; return 1; };
    switch (p) {
        case 0x826F: return one(supported ? 1 : 0);        // INTERNALFORMAT_SUPPORTED
        case 0x8270: return one(supported ? (GLint64)inf : 0); // INTERNALFORMAT_PREFERRED (NONE=0)
        case 0x8271: return one(d ? d->r : 0);            // RED_SIZE
        case 0x8272: return one(d ? d->g : 0);            // GREEN_SIZE
        case 0x8273: return one(d ? d->b : 0);            // BLUE_SIZE
        case 0x8274: return one(d ? d->a : 0);            // ALPHA_SIZE
        case 0x8275: return one(d ? d->depth : 0);        // DEPTH_SIZE
        case 0x8276: return one(d ? d->stencil : 0);      // STENCIL_SIZE
        case 0x8277: return one(0);                       // SHARED_SIZE (không format chia sẻ)
        case 0x8278: case 0x8279: case 0x827A: case 0x827B:
        case 0x827C: case 0x827D: {                       // *_TYPE
            if (!supported) return one(0);                // NONE
            GLint64 t = d->compType;
            // Component không có trong format → NONE.
            if ((p == 0x8278 && !d->r) || (p == 0x8279 && !d->g) ||
                (p == 0x827A && !d->b) || (p == 0x827B && !d->a) ||
                (p == 0x827C && !d->depth) || (p == 0x827D && !d->stencil))
                t = 0;
            return one(t);
        }
        case 0x827E: return one(supported ? 8192 : 0);    // MAX_WIDTH
        case 0x827F: return one(supported ? 8192 : 0);    // MAX_HEIGHT
        case 0x8280: return one(supported && target == 0x806F ? 8192 : 0); // MAX_DEPTH
        case 0x8281: return one(supported && layered ? 8192 : 0);          // MAX_LAYERS
        case 0x8282: return one(supported ? 8192LL * 8192LL : 0);         // MAX_COMBINED_DIMENSIONS
        case 0x8283: return one(d && (d->r || d->g || d->b || d->a) ? 1 : 0); // COLOR_COMPONENTS
        case 0x8284: return one(d && d->depth ? 1 : 0);   // DEPTH_COMPONENTS
        case 0x8285: return one(d && d->stencil ? 1 : 0); // STENCIL_COMPONENTS
        case 0x8286: return one(IfmtColor(inf) ? 1 : 0);  // COLOR_RENDERABLE
        case 0x8287: return one(IfmtDepth(inf) ? 1 : 0);  // DEPTH_RENDERABLE
        case 0x8288: return one(IfmtStencil(inf) ? 1 : 0);// STENCIL_RENDERABLE
        // NONE = 0, CAVEAT_SUPPORT = 1, FULL_SUPPORT = 2
        case 0x8289: return one(renderable ? 2 : 0);      // FRAMEBUFFER_RENDERABLE
        case 0x828A: return one(renderable && layered ? 2 : 0); // FRAMEBUFFER_RENDERABLE_LAYERED
        case 0x828B: return one(d && d->colorRender ? 2 : 0);    // FRAMEBUFFER_BLEND
        case 0x828C: return one(d && d->colorRender ? 2 : 0);    // READ_PIXELS
        case 0x828D: return one(d && d->colorRender ? 0x1908 /*RGBA*/ : 0); // READ_PIXELS_FORMAT
        case 0x828E: return one(d && d->colorRender ? 0x1401 /*UNSIGNED_BYTE*/ : 0); // READ_PIXELS_TYPE
        case 0x828F: return one(d ? 0x1908 /*RGBA*/ : 0);       // TEXTURE_IMAGE_FORMAT
        case 0x8290: return one(d ? 0x1401 /*UNSIGNED_BYTE*/ : 0); // TEXTURE_IMAGE_TYPE
        case 0x8291: return one(d ? 0x1908 : 0);               // GET_TEXTURE_IMAGE_FORMAT
        case 0x8292: return one(d ? 0x1401 : 0);               // GET_TEXTURE_IMAGE_TYPE
        case 0x8293: return one(d && d->colorRender ? 1 : 0);  // MIPMAP
        case 0x829A: return one(d && d->filterable ? 2 : 0);    // FILTER
        case 0x829F: return one(d ? 2 : 0);                    // FRAGMENT_TEXTURE
        case 0x82A0: return one(d ? 2 : 0);                    // COMPUTE_TEXTURE
        case 0x82B4: return one(renderable ? 2 : 0);           // CLEAR_BUFFER
        case 0x80A9: { // SAMPLES: sample count hỗ trợ, giảm dần (spec §22.3)
            if (!d || !renderable || !IfmtTargetMultiSample(target)) return 0;
            // Metal: MAX_SAMPLES = 4 → {4, 1}
            v[0] = 4; if (n > 1) v[1] = 1;
            return n > 1 ? 2 : 1;
        }
        case 0x9380: { // NUM_SAMPLE_COUNTS: số sample count mà SAMPLES sẽ trả
            if (!d || !renderable || !IfmtTargetMultiSample(target)) return one(0);
            return one(2);
        }
        default: return one(0); // pname hợp lệ nhưng chưa implement → 0 (không bịa)
    }
}
void glGetInternalformativ(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint* v) {
    Context& c = Context::Current();
    if (!IfmtTargetOk(t)) { c.errors.Record(0x0500); return; }  // INVALID_ENUM (target)
    if (!IfmtPnameOk(p)) { c.errors.Record(0x0500); return; }  // INVALID_ENUM (pname)
    if (n < 0) { c.errors.Record(0x0501); return; }            // INVALID_VALUE (count)
    if (!v || n == 0) return;
    GLint64 tmp[64];
    const GLsizei m = n > 64 ? 64 : n;
    const int cnt = InternalFormatQuery(t, inf, p, tmp, m);
    if (cnt < 0) { c.errors.Record(0x0500); return; }          // INVALID_ENUM (pname)
    for (int i = 0; i < cnt && i < (int)n; ++i) v[i] = (GLint)tmp[i];
}
void glGetInternalformati64v(GLenum t, GLenum inf, GLenum p, GLsizei n, GLint64* v) {
    Context& c = Context::Current();
    if (!IfmtTargetOk(t)) { c.errors.Record(0x0500); return; }
    if (!IfmtPnameOk(p)) { c.errors.Record(0x0500); return; }
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (!v || n == 0) return;
    InternalFormatQuery(t, inf, p, v, n);
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
