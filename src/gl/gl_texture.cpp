// gl_texture.cpp — Textures: MTLTextureDescriptor + replaceRegion.
// Spec §8 (Textures). PixelStore alignment áp dụng khi upload (spec Table 8.x).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <set>
#include <string>
#include <vector>
using namespace tglmt;

static size_t Bpp(GLenum format, GLenum type);                          // def dưới file
static bool SyncRegionToGPU(Context& c, TextureObject& tx, GLint xoff, GLint yoff, GLsizei w,
                            GLsizei h, const uint8_t* srcRows, size_t srcRowLen, GLenum format,
                            GLenum type);

// IR staging cho texture uploads (upload coalescing):
// glTexSubImage* ghi region kèm PAYLOAD bytes (P1) + stage; replaceRegion dồn đến
// flush (trước draw sampling texture / blit / readback). Payload đi theo region
// nên flush không cần shadow CPU → bỏ được bản sao base-level từng texture.
static void StageTexRegion(Context& c, GLuint texId, uint32_t x, uint32_t y, uint32_t w,
                           uint32_t h, GLenum format, GLenum type, const uint8_t* tight) {
    if (!w || !h) return;
    // Deferred full: chỉ flush khi texture này đã dùng trong pass đang mở.
    // Atlas streaming (font glyphs) stage hàng chục regions/frame mà không phá batching.
    if (c.pendingEncoder && c.MustFlushForTextureStage(texId)) c.FlushPendingEncoder();
    Context::TexRegion r{x, y, w, h, format, type, {}};
    if (tight) {
        size_t bpp = Bpp(format, type);
        r.bytes.assign(tight, tight + (size_t)w * h * bpp);
    }
    c.pendingTexRegions[texId].push_back(std::move(r));
    ++c.appleStats.texCoalesced;
}

// Diag (DiagOn): tỉ lệ pixel shadow khác 0 — trả lời "texture/atlas có data
// thật không". Readback GPU thật chỉ ≤512×512 (stall); shadow là bản upload.
static void LogShadowNZ(Context& c, const TextureObject& tx, GLuint id, const char* tag) {
    if (!c.DiagOn()) return;
    // Dedupe (tag,id): TexImage NULL-alloc (n=NULL, nz=0) là bình thường, lặp mỗi
    // frame → nuốt sạch cap 24. Giữ 1 dòng/id, cap 32 để thấy đủ id khác.
    static std::set<uint64_t> seen;
    uint64_t key = (uint64_t)id << 32 ^ std::hash<std::string>{}(tag);
    if (seen.size() >= 32 || !seen.insert(key).second) return;
    int n = (int)seen.size();
    if (tx.pixels.empty() || !tx.w || !tx.h) {
        fprintf(stderr, "[TGLMT] shnz#%d %s tex=%u %ux%u EMPTY-shadow\n", n, tag, id, tx.w,
                tx.h);
        fflush(stderr);
        return;
    }
    size_t npx = (size_t)tx.w * tx.h;
    size_t bpp = tx.pixels.size() / npx;
    if (!bpp) {
        fprintf(stderr, "[TGLMT] shnz#%d %s tex=%u %ux%u shadow-short=%zu\n", n, tag, id,
                tx.w, tx.h, tx.pixels.size());
        fflush(stderr);
        return;
    }
    size_t stride = std::max<size_t>(1, npx / 8192);
    size_t nz = 0, tot = 0;
    for (size_t i = 0; i < npx; i += stride) {
        const uint8_t* p = tx.pixels.data() + i * bpp;
        bool any = false;
        for (size_t b = 0; b < bpp; ++b)
            if (p[b]) { any = true; break; }
        nz += any;
        ++tot;
    }
    fprintf(stderr, "[TGLMT] shnz#%d %s tex=%u %ux%u bpp=%zu nz=%zu/%zu\n", n, tag, id,
            tx.w, tx.h, bpp, nz, tot);
    fflush(stderr);
}

// Upload 1 rect tight-ish (nguồn có pitch srcRowLen) lên GPU: conversion giống
// hệt đường bbox cũ + cùng bộ log (texnop / texupd / texWasRTFlush / texFlushes).
// Dùng cho cả 2 nguồn: shadow CPU (backend giữ shadow) và payload carry (P1).
static void UploadTexRect(Context& c, TextureObject& tx, GLuint texId, uint32_t x0,
                          uint32_t y0, uint32_t bw, uint32_t bh, GLenum fmt, GLenum typ,
                          const uint8_t* srcRows, size_t srcRowLen) {
    if (!bw || !bh || !srcRows) return;
    bool synced = false;
    if (typ == 0x1401 && (fmt == 0x1908 || fmt == 0x1903 || fmt == 0x8227 || fmt == 0x1906)) {
        size_t bpp = Bpp(fmt, typ);
        std::vector<uint8_t> tight((size_t)bw * bh * bpp);
        for (uint32_t r = 0; r < bh; ++r)
            memcpy(tight.data() + (size_t)r * bw * bpp, srcRows + (size_t)r * srcRowLen,
                   (size_t)bw * bpp);
        synced = c.device->updateTexture(tx.gpu.get(), x0, y0, bw, bh, tight.data(),
                                         (size_t)bw * bpp);
    } else if (typ == 0x1401 && fmt == 0x80E1) {
        std::vector<uint8_t> rgba((size_t)bw * bh * 4);
        for (uint32_t r = 0; r < bh; ++r)
            for (uint32_t x = 0; x < bw; ++x) {
                const uint8_t* s = srcRows + (size_t)r * srcRowLen + (size_t)x * 4;
                uint8_t* d = rgba.data() + ((size_t)r * bw + x) * 4;
                d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = s[3];
            }
        synced = c.device->updateTexture(tx.gpu.get(), x0, y0, bw, bh, rgba.data(),
                                         (size_t)bw * 4);
    } else if (fmt == 0x1907 && typ == 0x1401) {
        std::vector<uint8_t> rgba((size_t)bw * bh * 4);
        for (uint32_t r = 0; r < bh; ++r)
            for (uint32_t x = 0; x < bw; ++x) {
                const uint8_t* s = srcRows + (size_t)r * srcRowLen + (size_t)x * 3;
                uint8_t* d = rgba.data() + ((size_t)r * bw + x) * 4;
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
            }
        synced = c.device->updateTexture(tx.gpu.get(), x0, y0, bw, bh, rgba.data(),
                                         (size_t)bw * 4);
    } else {
        // LogDebug lặng khi app không gắn debugCb (vanilla iOS) → stderr khi DiagOn.
        synced = true; // đã log texnop riêng, không kể là update fail
        static int nUnh = 0;
        if (c.DiagOn() && ++nUnh <= 32)
            fprintf(stderr,
                    "[TGLMT] texnop#%d id=%u fmt=0x%x ty=0x%x bbox=(%u,%u %ux%u) giu-shadow\n",
                    nUnh, texId, fmt, typ, x0, y0, bw, bh);
        c.LogDebug(0, 0, 0, 0,
                   "FlushTextureStaging: format/type chưa upload GPU (giữ shadow)");
    }
    if (c.DiagOn() && !synced) {
        static int nUpdFail = 0;
        if (++nUpdFail <= 16)
            fprintf(stderr,
                    "[TGLMT] texupd#%d id=%u fmt=0x%x ty=0x%x bbox=(%u,%u %ux%u) updateTexture=false\n",
                    nUpdFail, texId, fmt, typ, x0, y0, bw, bh);
    }
    if (tx.wasRT) ++c.appleStats.texWasRTFlush;
    ++c.appleStats.texFlushes;
}

namespace tglmt {
// Flush 1 texture: gộp regions cùng format thành bbox duy nhất. Nguồn: shadow
// nếu còn, ngược lại payload carry (P1) chép vào bbox trung gian — vẫn 1 upload.
void Context::FlushTextureStaging(GLuint texId) {
    auto pit = pendingTexRegions.find(texId);
    if (pit == pendingTexRegions.end() || pit->second.empty()) return;
    auto tit = textures.find(texId);
    if (tit == textures.end() || !tit->second.gpu) {
        pendingTexRegions.erase(pit);
        return;
    }
    TextureObject& tx = tit->second;
    // In-flight guard: texture này vừa được sample bởi encoder ĐÃ commit mà GPU
    // chưa execute (lastReadGen == waitGen) → replaceRegion lúc này đổi nội dung
    // draw cũ đang đọc → vỡ texture/đốm. Đợi GPU trước (hiếm, ~4 lần/600 frame).
    if (device && tx.lastReadGen == waitGen) {
        FlushPendingEncoder();
        CommitAndWait();
        ++appleStats.texWaitStall;
    }
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
            if (tx.wasRT) {
                // wasRT: shadow chỉ chứa bytes app thật sự upload (glTexSubImage);
                // phần còn lại của texture chỉ sống trên GPU (bake qua FBO). Gộp
                // bbox qua vùng giữa 2 region sẽ memcpy số 0 đè nội dung GPU →
                // mất texture (kho đồ / block atlas). Chỉ gộp region chồng lấn/chạm.
                const TexRegion& nr = regs[j + 1];
                bool nearX = nr.x <= x1 + 4 && x0 <= nr.x + nr.w + 4;
                bool nearY = nr.y <= y1 + 4 && y0 <= nr.y + nr.h + 4;
                if (!nearX || !nearY) break;
            }
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
                if (tx.pixels.size() >= (size_t)tx.h * texRow) {
                    // Shadow còn (Null backend / format không upload được) → 1 bbox,
                    // đọc thẳng từ shadow, không cần bản đệm.
                    UploadTexRect(*this, tx, texId, x0, y0, bw, bh, fmt, typ,
                                  tx.pixels.data() + (size_t)y0 * texRow +
                                      (size_t)x0 * bpp,
                                  texRow);
                } else {
                    // P1: shadow base-level đã bỏ trên Apple → nguồn là payload
                    // carry của từng region. Vẫn GỘP thành 1 upload (giữ đặc tính
                    // coalesce) bằng cách chép payload vào bbox trung gian — không
                    // đọc shadow, không đụng pixel chưa từng staged (wasRT atlas).
                    // Chỉ gộp được khi các region ghép chính xác bbox (không hở,
                    // không đè): nếu có hở, hở sẽ nhận 0 → vỡ nội dung GPU đã bake
                    // → fallback upload từng region (đúng từng vùng).
                    uint64_t cover = 0;
                    bool allBytes = true;
                    for (size_t k = i; k <= j && allBytes; ++k) {
                        const TexRegion& r = regs[k];
                        if (r.bytes.empty()) { allBytes = false; break; }
                        if (r.x < tx.w && r.y < tx.h)
                            cover += (uint64_t)std::min(r.w, tx.w - r.x) *
                                     (uint64_t)std::min(r.h, tx.h - r.y);
                    }
                    if (allBytes && cover == (uint64_t)bw * (uint64_t)bh) {
                        std::vector<uint8_t> box((size_t)bw * bh * bpp, 0);
                        for (size_t k = i; k <= j; ++k) {
                            const TexRegion& r = regs[k];
                            uint32_t cw = std::min(r.w, tx.w - r.x);
                            uint32_t ch = std::min(r.h, tx.h - r.y);
                            cw = std::min(cw, bw - (r.x - x0));
                            ch = std::min(ch, bh - (r.y - y0));
                            if (!cw || !ch) continue;
                            size_t rStride = (size_t)r.w * bpp; // bytes tight theo r.w
                            for (uint32_t rr = 0; rr < ch; ++rr) {
                                uint8_t* d = box.data() +
                                             (((size_t)(r.y - y0) + rr) * bw +
                                              (r.x - x0)) * bpp;
                                memcpy(d, r.bytes.data() + (size_t)rr * rStride,
                                       (size_t)cw * bpp);
                            }
                        }
                        UploadTexRect(*this, tx, texId, x0, y0, bw, bh, fmt, typ,
                                      box.data(), (size_t)bw * bpp);
                    } else {
                        for (size_t k = i; k <= j; ++k) {
                            const TexRegion& r = regs[k];
                            if (r.bytes.empty()) continue; // không nguồn → bỏ (đúng)
                            if (r.x >= tx.w || r.y >= tx.h) continue;
                            uint32_t cw = std::min(r.w, tx.w - r.x);
                            uint32_t ch = std::min(r.h, tx.h - r.y);
                            if (!cw || !ch) continue;
                            UploadTexRect(*this, tx, texId, r.x, r.y, cw, ch, r.format,
                                          r.type, r.bytes.data(),
                                          (size_t)cw * bppOf(r.format, r.type));
                        }
                    }
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
        // Shader pack (CR pipelineSettings): trước đây rơi default RGBA8Unorm
        // → mất HDR/snorm (deferred lighting bị kẹp [0,1] rồi tối/đen) và log
        // "internalFormat 0x... lạ". Không map thẳng G11F_B10F: A11 filterable
        // RG11B10F không đảm bảo renderable → dùng RGBA16F (chuẩn cho HDR).
        case 0x8C3A: return metal::PixelFormat::RGBA16Float;       // R11F_G11F_B10F
        case 0x881B: return metal::PixelFormat::RGBA16Float;       // RGB16F
        case 0x881A: return metal::PixelFormat::RGBA16Float;       // RGBA16F
        case 0x805B: return metal::PixelFormat::RGBA16Float;       // RGBA16 (unorm→float)
        case 0x8814: return metal::PixelFormat::RGBA32Float;       // RGBA32F
        case 0x8815: return metal::PixelFormat::RGBA32Float;       // RGB32F
        case 0x8051: return metal::PixelFormat::RGBA8Unorm;        // RGB8
        case 0x8F96: return metal::PixelFormat::RGBA8Snorm;        // RGB8_SNORM
        case 0x8F97: return metal::PixelFormat::RGBA8Snorm;        // RGBA8_SNORM
        case 0x822F: return metal::PixelFormat::R8Snorm;           // R8_SNORM
        case 0x8C41: return metal::PixelFormat::RGBA8Unorm_sRGB;   // SRGB8
        case 0x8231: return metal::PixelFormat::R8Sint;            // R8I
        case 0x8232: return metal::PixelFormat::R8Uint;            // R8UI
        case 0x8233: return metal::PixelFormat::R16Sint;           // R16I
        case 0x8234: return metal::PixelFormat::R16Uint;           // R16UI
        case 0x822D: return metal::PixelFormat::R16Float;          // R16F
        case 0x822E: return metal::PixelFormat::R32Float;          // R32F (buffer)
        case 0x8235: return metal::PixelFormat::R32Sint;           // R32I (CloudFaces)
        case 0x8236: return metal::PixelFormat::R32Uint;           // R32UI (buffer)
        case 0x81A5: case 0x81A6: case 0x8CAC:                     // DEPTH16/24/32F
            return metal::PixelFormat::Depth32Float;
        case 0x81A7: {                                             // DEPTH_COMPONENT32
            // fixico8: GuiItemAtlas depth (GlConst DEPTH32→33191) từng rơi
            // default → RGBA8Unorm → depth-attachment sai format vs pipeline
            // Depth32Float → encoder nil/depth-test rác → bake icon bị reject.
            static bool once = false;
            if (!once) {
                once = true;
                if (Context::Current().DiagOn())
                    fprintf(stderr,
                            "[TGLMT] depthfmt# 0x81A7 DEPTH_COMPONENT32 → Depth32Float (fixico8)\n");
            }
            return metal::PixelFormat::Depth32Float;
        }
        case 0x88F0: return metal::PixelFormat::Depth24Stencil8;   // DEPTH24_STENCIL8
        default:
            if (Context::Current().DiagOn())
                fprintf(stderr,
                        "[TGLMT] ToMetalFormat: internalFormat 0x%X lạ → RGBA8Unorm\n",
                        (unsigned)internalFormat);
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
// P3: bytes/pixel có xét type cho texture 3D (FLOAT/INT/UINT 4 byte/element).
// Bpp() cũ bỏ qua type (đúng cho UBYTE union formats của path 2D) — không dùng
// cho 3D float (R32F...) được.
static size_t BppTyped3D(GLenum format, GLenum type) {
    size_t b = Bpp(format, type);
    switch (type) {
        // packed 32-bit types: 1 phần tử 4 byte bất kể format component.
        case 0x8035: case 0x8367: case 0x8368: // 8_8_8_8(_REV) / 2_10_10_10_REV
        case 0x8C3B: case 0x8C3E:             // 10F_11F_11F_REV / 5_9_9_9_REV
        case 0x84FA:                           // UNSIGNED_INT_24_8
            return 4;
        case 0x1400: case 0x1401: return b;      // BYTE / UNSIGNED_BYTE
        case 0x1402: case 0x1403: return b * 2;  // SHORT / UNSIGNED_SHORT
        case 0x140B: return b * 2;               // HALF_FLOAT
        case 0x1404: case 0x1405: case 0x1406: return b * 4; // INT/UINT/FLOAT
        case 0x140A: return b * 8;               // DOUBLE
        default: break;
    }
    return b;
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
    // PBO có thể vừa được glBufferSubData stage → SyncBuffer xả trước khi đọc.
    // Shadow CPU có thể rỗng (Apple backend — P1) → đọc thẳng GPU memory chung.
    Context::BufferView v = c.SyncBuffer(up);
    if (!v.p) { ok = false; return nullptr; }
    size_t off = (size_t)pixels;
    if (off + total > v.size) { ok = false; return nullptr; }
    ok = true;
    return v.p + off;
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
    for (GLsizei i = 0; i < n && i < 4; ++i) c.RecordTexBind('G', 0, t[i], 0);
}
void glCreateTextures(GLenum target, GLsizei n, GLuint* t) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Texture, n, t);
    for (GLsizei i = 0; i < n; ++i) { c.textures[t[i]] = TextureObject{t[i]}; c.textures[t[i]].target = target; }
}
void glDeleteTextures(GLsizei n, const GLuint* t) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Texture, n, t);
    ++c.objectGen; // cache wrap-target: texture có thể bị giải phóng/ID tái dùng
    // P1b: entry cũ giữ shared_ptr<IRenderTarget> → AppleTarget giữ id<MTLTexture>
    // strong, pin texture đã xoá tới khi bị round-robin ghi đè. Xoá ngay.
    if (n > 0)
        for (auto& e : c.wrapCache) e = Context::WrapEntry{};
    for (GLsizei i = 0; i < n; ++i) {
        c.RecordTexBind('D', 0, t[i], 0);
        c.textures.erase(t[i]);
        c.pendingTexRegions.erase(t[i]);
        // Spec §8.1: delete = unbind khỏi mọi unit. Không xóa → stale id:
        // draw resolve thiếu → fallback đen; id tái sử dụng → sample nhầm.
        c.state.UnbindTextureEverywhere(t[i]);
        // Spec §9 (FBO completeness): attachment trỏ texture đã xóa → detach
        // (về 0). Không detach: id tái sử dụng cho atlas mới (resource reload
        // khi đổi setting) thì FBO cũ trỏ NHẦM sang atlas mới → draw bake cũ
        // smear đè atlas mới → đen/mất texture sau "tắt đi mở lại".
        for (auto& [fid, fbo] : c.fbos) {
            for (auto it = fbo.colorTex.begin(); it != fbo.colorTex.end();) {
                if (it->second == t[i]) {
                    fbo.colorLevel.erase(it->first);
                    it = fbo.colorTex.erase(it);
                } else ++it;
            }
            if (fbo.depthTex == t[i]) { fbo.depthTex = 0; fbo.depthLevel = 0; }
            if (fbo.stencilTex == t[i]) { fbo.stencilTex = 0; fbo.stencilLevel = 0; }
            if (fbo.depthStencilTex == t[i]) { fbo.depthStencilTex = 0; fbo.depthStencilLevel = 0; }
        }
    }
}
GLboolean glIsTexture(GLuint t) {
    return Context::Current().registry.Is(ObjectKind::Texture, t) ? 1 : 0;
}
void glBindTexture(GLenum target, GLuint t) {
    Context& c = Context::Current();
    c.RecordTexBind('B', c.state.ActiveTexture(), t, target);
    if (t && !c.textures.count(t)) {
        // Heal (Fix #6): MC đã ghi cache của nó = id (guarded _bindTexture đã
        // chạy) trong khi object chưa có trong Context → reject ở đây làm state
        // lệch VĨNH VIỄN: draw resolve thiếu → fallback đen (mất chữ/GUI), upload
        // BoundTex 0x0502 → MẤT glyph/sprite đến lần retry. Real GL bind name mới
        // tự tạo object → tạo rỗng tại đây để state đi theo MC (nội dung sẽ có
        // khi MC upload). Ghi log + dump texring để thấy chuỗi bind gây ra.
        c.textures[t] = TextureObject{t};
        c.textures[t].target = target;
        c.state.BindTextureUnit(c.state.ActiveTexture(), t, target);
        static uint64_t n = 0;
        if (++n <= 32) {
            fprintf(stderr, "[TGLMT] bindheal#%llu tex=%u tgt=0x%x (id chua co, tao rong)\n",
                    (unsigned long long)n, t, target);
            fflush(stderr);
        }
        if (c.DiagOn() && n <= 8) c.DumpTexBindRing("bindheal");
        return;
    }
    if (t) c.textures[t].target = target;
    c.state.BindTextureUnit(c.state.ActiveTexture(), t, target);
}
void glBindTextureUnit(GLuint u, GLuint t) {
    Context& c = Context::Current();
    // DSA: binding point theo target CỦA OBJECT (spec §8.1 glBindTextureUnit).
    GLenum tgt = 0x0DE1;
    if (t) {
        auto it = c.textures.find(t);
        if (it != c.textures.end() && it->second.target) tgt = it->second.target;
    }
    c.RecordTexBind('U', u, t, tgt);
    c.state.BindTextureUnit(u, t, tgt);
}
void glBindTextures(GLuint f, GLsizei n, const GLuint* t) {
    Context& c = Context::Current();
    for (GLsizei i = 0; i < n; ++i) {
        GLuint id = t ? t[i] : 0;
        GLenum tgt = 0x0DE1;
        if (id) {
            auto it = c.textures.find(id);
            if (it != c.textures.end() && it->second.target) tgt = it->second.target;
        }
        c.RecordTexBind('S', f + i, id, tgt);
        c.state.BindTextureUnit(f + i, id, tgt);
    }
}
void glActiveTexture(GLenum tex) {
    Context& c = Context::Current();
    if (tex < 0x84C0 || tex >= 0x84C0 + 32) { c.errors.Record(0x0500); return; }
    c.RecordTexBind('A', tex - 0x84C0, 0, 0);
    c.state.SetActiveTexture(tex - 0x84C0);
}
// Spec §8.1: mọi lệnh Tex* (không DSA) tác động lên texture đang bind tại
// active unit. Kiểm tra target khớp; sai → INVALID_OPERATION (core profile).
// Ngoại lệ: face cubemap (POSITIVE_X..NEGATIVE_Z) thuộc texture CUBE đã bind.
static bool IsCubeFace(GLenum t) { return t >= 0x8515 && t <= 0x851A; }
static TextureObject* BoundTex(Context& c, GLenum target) {
    GLuint unit = c.state.ActiveTexture();
    // Per-target: resolve slot theo target Tex* sắp dùng (face → slot cube),
    // không để cube/buffer bind khác target đánh clobber (upload nhầm/sai object).
    GLuint id = c.state.BoundTexture(unit, target);
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
// Nâng texture GPU lên mip chain nhiều level, giữ nguyên các level đã có bằng
// blit view. Dùng khi app phát sinh level>0 SAU khi base đã lên GPU (legacy
// glTexImage2D/glTexSubImage2D, không phải immutable glTexStorage2D).
static bool PromoteGpuMipChain(Context& c, TextureObject& t, uint32_t want) {
    if (c.device->isNull() || !t.gpu || t.wasRT || !t.w || !t.h) return false;
    uint32_t oldLevels = t.gpu->levelCount();
    if (oldLevels >= want) return true;
    c.FlushPendingEncoder();
    c.FlushAllTextureStaging();
    auto ng = c.device->newTextureLevels(t.w, t.h, ToMetalFormat(t.internalFormat), want);
    if (!ng) return false;
    auto fmt = t.gpu->pixelFormat();
    bool ok = true;
    for (uint32_t lv = 0; lv < oldLevels && ok; ++lv) {
        uint32_t lw = t.w >> lv, lh = t.h >> lv;
        if (!lw) lw = 1;
        if (!lh) lh = 1;
        if (lv == 0) {
            ok = c.device->blitCopy(t.gpu.get(), ng.get(), 0, 0, lw, lh, 0, 0);
        } else {
            auto sv = t.gpu->textureView(fmt, lv, 1, 0, 1);
            auto dv = ng->textureView(fmt, lv, 1, 0, 1);
            ok = sv && dv && c.device->blitCopy(sv.get(), dv.get(), 0, 0, lw, lh, 0, 0);
        }
    }
    if (!ok) return false;
    t.gpu = ng;
    t.levelViews.clear();
    t.layerViews.clear();
    return true;
}

// Legacy cube face level>0: realloc cube GPU mip chain, giữ base 6 face, upload
// shadow mip riêng từng face. Tránh PromoteGpuMipChain 2D làm mất cube type.
static bool PromoteGpuMipChainCube(Context& c, TextureObject& t, uint32_t want) {
    if (c.device->isNull() || !t.gpu || t.wasRT || !t.w || !t.h) return false;
    if (t.target != 0x8513 || !t.isCube) return false;
    uint32_t oldLevels = t.gpu->levelCount();
    if (oldLevels >= want) return true;
    c.FlushPendingEncoder();
    c.FlushAllTextureStaging();
    auto ng = c.device->newCubeTextureLevels(t.w, want, ToMetalFormat(t.internalFormat));
    if (!ng) return false;
    bool ok = true;
    for (uint32_t face = 0; face < 6 && ok; ++face) {
        if (t.faces[face].size() >= (size_t)t.w * t.h * 4)
            ok = c.device->updateCubeFaceLevel(ng.get(), 0, face, t.faces[face].data(),
                                               (size_t)t.w * 4);
        for (uint32_t lv = 1; lv < want && ok; ++lv) {
            auto it = t.faceMips[face].find((GLint)lv);
            if (it == t.faceMips[face].end() || it->second.pixels.empty()) continue;
            uint32_t lw = t.w >> lv, lh = t.h >> lv;
            if (!lw) lw = 1;
            if (!lh) lh = 1;
            if (it->second.pixels.size() < (size_t)lw * lh * 4) continue;
            ok = c.device->updateCubeFaceLevel(ng.get(), lv, face,
                                               it->second.pixels.data(), (size_t)lw * 4);
        }
    }
    if (!ok) return false;
    t.gpu = ng;
    t.levelViews.clear();
    t.layerViews.clear();
    return true;
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; }
    // GL_TEXTURE_CUBE_MAP không phải target hợp lệ của glTexImage2D (phải dùng
    // từng face POSITIVE_X..NEGATIVE_Z). Trước đây bị nhầm với proxy → nuốt
    // lệnh, app tưởng đã tạo storage.
    if (target == 0x8513) { c.errors.Record(0x0500); return; }
    // Proxy texture (GlDevice probe max size): không đổi state, không lỗi
    // (đúng spec §8 — probe rồi fallback 8192 khi quá lớn). Lưu spec để query
    // GetTexLevelParameteriv trả lời (quá 8192 → 0), tránh lỗi oan tồn lỗi
    // pending mà vanilla quy cho call sau (crash copyTobuffer 1282).
    switch (target) {
        case 0x8063: case 0x8064: case 0x8070: case 0x84F7: case 0x851B:
        case 0x8C19: case 0x8C1B: case 0x900B: {
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
        if (++nBig <= 256) {
            char ds[32];
            if (hasUnpack) snprintf(ds, sizeof(ds), "PBO+%zu", (size_t)pixels);
            else snprintf(ds, sizeof(ds), "%s", pixels ? "data" : "NULL");
            fprintf(stderr,
                    "[TGLMT] bigTexImage#%d id=%u tgt=0x%x lv=%d %dx%d ifmt=0x%x fmt=0x%x ty=0x%x %s\n",
                    nBig, tp->id, target, level, w, h, internalformat, format, type, ds);
            fflush(stderr);
        }
    }
    // Mip levels >0: ghi nhận số levels + shadow per-level (vanilla 26.x alloc
    // mọi mip qua TexImage2D NULL rồi upload từng level: MipmapGenerator +
    // writeToTexture per-mip). GPU TGLMT hiện chỉ giữ base level (non-mipmapped,
    // sampler NotMipmapped) nên KHÔNG đụng base; shadow per-level phục vụ
    // GetTexImage(level>0)/CPU fallback + mipmapped tương lai. Bug cũ: chỉ đếm
    // levels, SubImage level>0 bị drop câm → readback mip đen.
    if (level > 0) {
        // Cubemap face: mip phải lưu theo FACE, không dùng mipData 2D và không
        // được PromoteGpuMipChain (sẽ thay cube bằng texture 2D).
        if (IsCubeFace(target) && tx.target == 0x8513) {
            int face = (int)(target - 0x8515);
            tx.isCube = true;
            if (!tx.w && w > 0) { tx.w = (uint32_t)w << level; tx.h = (uint32_t)h << level; }
            if (level + 1 > tx.levels) tx.levels = level + 1;
            if (w > 0 && h > 0) {
                auto& ml = tx.faceMips[face][level];
                ml.w = w; ml.h = h; ml.d = 0;
                size_t bpp = Bpp(format, type);
                ml.pixels.assign((size_t)w * h * bpp, 0);
                if (hasData && (format == 0x1908 || format == 0x1903 || format == 0x8227) &&
                    type == 0x1401) {
                    size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
                    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
                    const uint8_t* src = pixBase + skip;
                    for (GLsizei r = 0; r < h; ++r)
                        memcpy(ml.pixels.data() + (size_t)r * w * bpp, src + r * rowLen,
                               (size_t)w * bpp);
                }
            }
            if (!tx.gpu && tx.w)
                tx.gpu = c.device->newCubeTextureLevels(tx.w, (uint32_t)tx.levels,
                                                        ToMetalFormat(tx.internalFormat));
            if (!c.device->isNull() && tx.gpu && !tx.wasRT && tx.w && tx.h)
                PromoteGpuMipChainCube(c, tx, (uint32_t)tx.levels);
            if (tx.gpu && hasData && tx.gpu->levelCount() > (uint32_t)level &&
                w > 0 && h > 0 && type == 0x1401 &&
                (format == 0x1908 || format == 0x1903 || format == 0x8227)) {
                auto& ml = tx.faceMips[face][level];
                size_t bpp = Bpp(format, type);
                if (!ml.pixels.empty())
                    c.device->updateCubeFaceLevel(tx.gpu.get(), (uint32_t)level,
                                                  (uint32_t)face, ml.pixels.data(),
                                                  (size_t)w * bpp);
            }
            (void)border;
            return;
        }
        if (level + 1 > tx.levels) tx.levels = level + 1;
        bool gpuFmt = hasData && type == 0x1401 &&
                      (format == 0x1908 || format == 0x1903 || format == 0x8227);
        std::vector<uint8_t> unpacked;
        if (w > 0 && h > 0) {
            auto& ml = tx.mipData[level];
            ml.w = w; ml.h = h;
            size_t bpp = Bpp(format, type);
            if (gpuFmt) {
                unpacked.resize((size_t)w * h * bpp);
                size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
                size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                              (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
                const uint8_t* src = pixBase + skip;
                for (GLsizei r = 0; r < h; ++r)
                    memcpy(unpacked.data() + (size_t)r * w * bpp, src + r * rowLen,
                           (size_t)w * bpp);
            } else {
                // Format không lên GPU được (hoặc pixels=NULL): shadow là bản
                // duy nhất, giữ nguyên hành vi cũ.
                ml.pixels.assign((size_t)w * h * bpp, 0);
                if (hasData) {
                    size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
                    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
                    const uint8_t* src = pixBase + skip;
                    for (GLsizei r = 0; r < h; ++r)
                        memcpy(ml.pixels.data() + (size_t)r * w * bpp, src + r * rowLen,
                               (size_t)w * bpp);
                }
            }
        }
        // Legacy glTexImage2D level>0: nâng texture GPU lên mip chain. Spec
        // §8.5: TexImage2D level>0 định nghĩa storage cho level đó, kể cả
        // pixels=NULL (nội dung chưa xác định); GPU phải có đủ level để FBO
        // attach/readback level>0 không assert. Trước đây chỉ nâng khi có data
        // thật nên `TexImage2D(NULL)` + `glFramebufferTexture2D(level=1)` làm
        // Metal view vượt mipmapLevelCount. Texture đang là render target
        // (wasRT) thì không realloc giữa pass — vẫn giữ guard đó.
        if (!c.device->isNull() && tx.gpu && !tx.wasRT &&
            w > 0 && h > 0 && tx.w > 0 && tx.h > 0) {
            PromoteGpuMipChain(c, tx, (uint32_t)tx.levels);
            if (tx.gpu && tx.gpu->levelCount() > (uint32_t)level &&
                hasData && type == 0x1401 &&
                (format == 0x1908 || format == 0x1903 || format == 0x8227)) {
                size_t bpp = Bpp(format, type);
                bool upOk = bpp && unpacked.size() >= (size_t)w * h * bpp &&
                            c.device->updateTextureLevel(tx.gpu.get(), (uint32_t)level,
                                                         0, 0, (uint32_t)w, (uint32_t)h,
                                                         unpacked.data(), (size_t)w * bpp);
                // P1b: GPU đã giữ level → bỏ bản sao CPU (mip chain 126MB).
                if (!upOk) tx.mipData[level].pixels = std::move(unpacked);
            } else if (!unpacked.empty()) {
                tx.mipData[level].pixels = std::move(unpacked);
            }
        } else if (!unpacked.empty()) {
            tx.mipData[level].pixels = std::move(unpacked);
        }
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
    // Realloc level 0 = storage MỚI (resource reload đổi setting/atlas mới):
    // xóa mip shadows cũ (sai size) + wasRT cũ (FBO bake của đời trước). Giữ
    // lại là smear atlas mới bằng nội dung GPU/shadow cũ → đen sau reload.
    tx.mipData.clear();
    bool wasRTBefore = tx.wasRT;
    tx.wasRT = false;
    size_t n = (size_t)w * h * Bpp(format, type);
    // P1: unpack vào buffer TẠM. Bản shadow base-level chỉ được GIỮ lại khi
    // (a) device Null — shadow LÀ tài nguyên, hoặc (b) format không có đường
    // upload lên GPU — shadow là bản nội dung duy nhất. Còn lại: GPU buffer là
    // Shared memory đã là bản đúng → drop bản sao CPU (2043MB jetsam).
    std::vector<uint8_t> unpacked(n, 0);
    if (hasData) {
        // áp unpackAlignment + ROW_LENGTH (spec §8.5)
        size_t bpp = Bpp(format, type);
        size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
        size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                      c.state.PixelStore().unpackSkipPixels * bpp;
        const uint8_t* src = pixBase + skip;
        for (GLsizei r = 0; r < h; ++r) memcpy(unpacked.data() + r * w * bpp, src + r * rowLen, (size_t)w * bpp);
    }
    // Re-specify texture ĐANG LÀ render target của encoder đang mở: encoder cũ
    // giữ wrapper (AppleTarget) của gpu ĐỜI CŨ → draw kế tiếp trong cùng pass
    // re-use pendingTarget và ghi vào gpu đã bị thay (app sample gpu MỚI → rỗng:
    // atlas bake mất, GUI/terrain đen). Flush TRƯỚC khi thay gpu.
    bool wasCurrentRT = (c.pendingEncoder && c.IsPendingColorId(tp->id));
    if (wasCurrentRT) c.FlushPendingEncoder();
    if (c.DiagOn() && (wasRTBefore || wasCurrentRT || (w >= 256 && h >= 256))) {
        static int nSwap = 0;
        if (++nSwap <= 64)
            fprintf(stderr, "[TGLMT] gpuswap#%d id=%u %dx%d wasRT=%d curRT=%d data=%d\n",
                    nSwap, tp->id, w, h, (int)wasRTBefore, (int)wasCurrentRT,
                    (int)(hasData != 0));
    }
    // Trước khi cấp GPU texture lớn (Voxy world re-create): nếu RAM trống thấp,
    // trả shadow/pool TGLMT giữ thêm thay vì để Jetsam giết app.
    c.MaybeTrimForAllocation(n);
    tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
    c.pendingTexRegions.erase(tp->id); // realloc GPU mới đã có full data → staging cũ vô nghĩa
    // Upload base level lên GPU (bug cũ: tạo texture rỗng → sampling đen).
    // Vanilla atlas RGBA/UBYTE tight; JPG panorama là RGB/UBYTE → expand alpha 255.
    // Font RED8 (LUMINANCE) → R8 raw. BGRA/UBYTE (widgets/gui) → swizzle R<->B.
    // Mọi format khác giữ shadow + log.
    // Format/type có đường upload lên GPU không (quyết định giữ shadow hay drop).
    bool gpuFmt =
        ((format == 0x1908 || format == 0x1903 || format == 0x8227) && type == 0x1401) ||
        (format == 0x80E1 && type == 0x1401) || (format == 0x1907 && type == 0x1401) ||
        ((format == 0x1908 || format == 0x80E1) &&
         (type == 0x8367 || type == 0x8368 || type == 0x1405 || type == 0x1404));
    if (tx.gpu && hasData && w > 0 && h > 0) {
        if ((format == 0x1908 || format == 0x1903 || format == 0x8227) && type == 0x1401) {
            // đã unpack tight vào `unpacked` → upload trực tiếp
            size_t bpp = Bpp(format, type);
            if (unpacked.size() >= (size_t)w * h * bpp)
                c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                        unpacked.data(), (size_t)w * bpp);
        } else if (format == 0x80E1 && type == 0x1401) {
            // BGRA → RGBA (đảo R/B). `unpacked` đang giữ BGRA raw; dựng RGBA tight.
            std::vector<uint8_t> rgba((size_t)w * h * 4);
            for (GLsizei r = 0; r < h; ++r)
                for (GLsizei x = 0; x < w; ++x) {
                    const uint8_t* s = unpacked.data() + ((size_t)r * w + x) * 4;
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
                       unpacked.data() + (size_t)r * w * 4, (size_t)w * 4);
            if (format == 0x80E1)
                for (size_t i = 0; i < (size_t)w * h; ++i) std::swap(raw[i * 4], raw[i * 4 + 2]);
            c.device->updateTexture(tx.gpu.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                    raw.data(), (size_t)w * 4);
            c.LogDebug(0, 0, 0, 0, "glTexImage2D: RGBA/BGRA UINT raw upload");
        } else {
            // LogDebug lặng (không debugCb trên iOS) → in stderr khi DiagOn.
            static int nUnhImg = 0;
            if (c.DiagOn() && ++nUnhImg <= 64)
                fprintf(stderr,
                        "[TGLMT] texnop-img#%d id=%u %dx%d fmt=0x%x ty=0x%x giu-shadow\n",
                        nUnhImg, tp->id, w, h, format, type);
            c.LogDebug(0, 0, 0, 0, "glTexImage2D: format/type chưa upload GPU (giữ shadow)");
        }
    }
    // P1: quyết định giữ/bỏ shadow base-level (xem chú thích ở khai báo `unpacked`).
    if (c.device->isNull() || !tx.gpu || !gpuFmt) {
        tx.pixels = std::move(unpacked);
    } else {
        std::vector<uint8_t>().swap(tx.pixels);
    }
    // Chẩn đoán: shadow có data thật không (trả lời "atlas rỗng?").
    // Bỏ qua upload NULL (glTexImage n=NULL = alloc rỗng, nz=0 — bình thường)
    // và bỏ qua khi shadow đã được drop chủ đích (P1) — log EMPTY là nhiễu.
    if (c.DiagOn() && pixels && (w >= 256 || h >= 256) && !tx.pixels.empty())
        LogShadowNZ(c, tx, tp->id, "teximage");
    (void)border; (void)level;
}
void glTexImage1D(GLenum t, GLint l, GLint inf, GLsizei w, GLint b, GLenum f, GLenum ty, const void* p) {
    glTexImage2D(t, l, inf, w, 1, b, f, ty, p);
}
// Legacy glTexImage3D(level>0) trên 3D base 1-level: realloc GPU mip chain và
// upload lại từ shadow (3D shadow luôn được giữ). 2D array không giữ shadow nên
// đọc lại base từ GPU một lần rồi upload vào chain mới.
static bool PromoteGpuMipChain3D(Context& c, TextureObject& t, uint32_t want) {
    if (c.device->isNull() || !t.gpu || t.wasRT || !t.w || !t.h || !t.d) return false;
    if (t.target != 0x806F) return false;
    if (t.gpu->levelCount() >= want) return true;
    const size_t bpp = 4;
    if (t.pixels.size() < (size_t)t.w * t.h * t.d * bpp) return false;
    c.FlushPendingEncoder();
    c.FlushAllTextureStaging();
    auto ng = c.device->newTexture3DLevels(t.w, t.h, t.d, want,
                                           ToMetalFormat(t.internalFormat));
    if (!ng) return false;
    bool ok = true;
    for (uint32_t lv = 0; lv < want && ok; ++lv) {
        uint32_t lw = t.w >> lv, lh = t.h >> lv, ld = t.d >> lv;
        if (!lw) lw = 1;
        if (!lh) lh = 1;
        if (!ld) ld = 1;
        const uint8_t* base = nullptr;
        if (lv == 0) base = t.pixels.data();
        else {
            auto it = t.mipData.find((GLint)lv);
            if (it == t.mipData.end() || it->second.pixels.empty()) continue;
            base = it->second.pixels.data();
        }
        for (uint32_t z = 0; z < ld && ok; ++z)
            ok = c.device->updateTexture3DLevel(ng.get(), lv, z, 0, 0, lw, lh,
                    base + (size_t)z * lw * lh * bpp, (size_t)lw * bpp);
    }
    if (!ok) return false;
    t.gpu = ng;
    t.levelViews.clear();
    t.layerViews.clear();
    return true;
}
static bool PromoteGpuMipChainArray(Context& c, TextureObject& t, uint32_t want) {
    if (c.device->isNull() || !t.gpu || t.wasRT || !t.w || !t.h || !t.d) return false;
    if (t.target != 0x8C1A) return false;
    if (t.gpu->levelCount() >= want) return true;
    if (!TexReadableFromGPU(t)) return false;
    std::vector<uint8_t> base;
    if (!c.ReadTextureGPU(t, base, -1, 0)) return false;
    if (base.size() < (size_t)t.w * t.h * t.d * 4) return false;
    auto ng = c.device->newTexture2DArrayLevels(t.w, t.h, t.d, want,
                                                ToMetalFormat(t.internalFormat));
    if (!ng) return false;
    bool ok = true;
    for (uint32_t z = 0; z < t.d && ok; ++z)
        ok = c.device->updateTexture3DLevel(ng.get(), 0, z, 0, 0, t.w, t.h,
                base.data() + (size_t)z * t.w * t.h * 4, (size_t)t.w * 4);
    for (uint32_t lv = 1; lv < want && ok; ++lv) {
        uint32_t lw = t.w >> lv, lh = t.h >> lv;
        if (!lw) lw = 1;
        if (!lh) lh = 1;
        auto it = t.mipData.find((GLint)lv);
        if (it == t.mipData.end() || it->second.pixels.empty()) continue;
        if (it->second.pixels.size() < (size_t)lw * lh * t.d * 4) continue;
        for (uint32_t z = 0; z < t.d && ok; ++z)
            ok = c.device->updateTexture3DLevel(ng.get(), lv, z, 0, 0, lw, lh,
                    it->second.pixels.data() + (size_t)z * lw * lh * 4, (size_t)lw * 4);
    }
    if (!ok) return false;
    t.gpu = ng;
    t.levelViews.clear();
    t.layerViews.clear();
    return true;
}

void glTexImage3D(GLenum target, GLint level, GLint inf, GLsizei w, GLsizei h, GLsizei d, GLint b, GLenum f, GLenum ty, const void* p) {
    if (target == 0x8070 || target == 0x8C1B || target == 0x900B)
        return; // PROXY_3D/PROXY_2D_ARRAY/PROXY_CUBE_MAP_ARRAY: probe, no-op
    Context& c = Context::Current();
    if (target != 0x806F && target != 0x8C1A) { // 3D hoặc 2D_ARRAY; CUBE_MAP_ARRAY chưa hỗ trợ
        c.LogDebug(0,0,0,0,"glTexImage3D: target chưa hỗ trợ (chỉ 3D/2D_ARRAY)");
        return;
    }
    if (w < 0 || h < 0 || d < 0) { c.errors.Record(0x0501); return; }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    auto& tx = *tp;
    // PBO unpack (spec §8): pixels là offset khi UNPACK bound.
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    size_t bppAll = BppTyped3D(f, ty);
    size_t rowLenAll = (w > 0) ? UnpackRowLen(c, (size_t)w, bppAll) : 0;
    size_t skipAll = (size_t)c.state.PixelStore().unpackSkipRows * rowLenAll +
                     (size_t)c.state.PixelStore().unpackSkipPixels * bppAll;
    // 3D unpack giả định IMAGE_HEIGHT = h (không UNPACK_IMAGE_HEIGHT) — đúng cho
    // Iris custom 3D texture (tight per-slice); các trường hợp khác chưa gặp.
    size_t totalAll = skipAll + ((w > 0 && h > 0 && d > 0)
                       ? (size_t)d * (size_t)h * rowLenAll : 0);
    const uint8_t* pixBase = (const uint8_t*)p;
    if (hasUnpack) {
        bool okBase = true;
        pixBase = UnpackBase(c, p, totalAll, okBase);
        if (!okBase) { c.errors.Record(0x0501); return; }
    }
    bool hasData = hasUnpack || p;
    // Mip level>0: GPU giữ base (như 2D) → shadow per-level phục vụ readback.
    if (level > 0) {
        if (level + 1 > tx.levels) tx.levels = level + 1;
        const size_t bpp = BppTyped3D(f, ty);
        const bool gpuFmt =
            (ty == 0x1401 && (f == 0x1908 || f == 0x1903 || f == 0x8227)) ||
            (ty == 0x1406 && (f == 0x1903 || f == 0x1908));
        std::vector<uint8_t> unpacked;
        if (hasData && w > 0 && h > 0 && d > 0 && gpuFmt) {
            unpacked.resize((size_t)w * h * d * bpp);
            size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
            size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                          (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
            const uint8_t* src = pixBase + skip;
            for (GLsizei z = 0; z < d; ++z)
                for (GLsizei r = 0; r < h; ++r)
                    memcpy(unpacked.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                           src + ((size_t)z * h + r) * rowLen, (size_t)w * bpp);
        } else if (w > 0 && h > 0 && d > 0) {
            auto& ml = tx.mipData[level];
            ml.w = w; ml.h = h; ml.d = d;
            ml.pixels.assign((size_t)w * h * d * bpp, 0);
            if (hasData && ty == 0x1401 && (f == 0x1908 || f == 0x1903 || f == 0x8227)) {
                size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
                size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                              (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
                const uint8_t* src = pixBase + skip;
                for (GLsizei z = 0; z < d; ++z)
                    for (GLsizei r = 0; r < h; ++r)
                        memcpy(ml.pixels.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                               src + ((size_t)z * h + r) * rowLen, (size_t)w * bpp);
            }
        }
        // Immutable storage (glTexStorage3D) đã cấp đủ GPU level. Legacy
        // TexImage3D level>0: realloc GPU mip chain từ shadow (3D).
        if (!c.device->isNull() && tx.gpu && tx.levels > tx.gpu->levelCount()) {
            if (tx.target == 0x806F)
                PromoteGpuMipChain3D(c, tx, (uint32_t)tx.levels);
            else if (tx.target == 0x8C1A)
                PromoteGpuMipChainArray(c, tx, (uint32_t)tx.levels);
        }
        if (c.device && !c.device->isNull() && tx.gpu && hasData && w > 0 && h > 0 && d > 0 &&
            tx.gpu->levelCount() > (uint32_t)level && gpuFmt) {
            for (GLsizei z = 0; z < d; ++z)
                c.device->updateTexture3DLevel(
                    tx.gpu.get(), (uint32_t)level, (uint32_t)z,
                    0, 0, (uint32_t)w, (uint32_t)h,
                    unpacked.data() + (size_t)z * (size_t)h * (size_t)w * bpp,
                    (size_t)w * bpp);
            // P1b: GPU đã giữ level → bỏ shadow CPU.
            auto& ml = tx.mipData[level];
            ml.w = w; ml.h = h; ml.d = d;
            std::vector<uint8_t>().swap(ml.pixels);
        } else if (!unpacked.empty()) {
            auto& ml = tx.mipData[level];
            ml.w = w; ml.h = h; ml.d = d;
            ml.pixels = std::move(unpacked);
        }
        c.LogDebug(0, 0, 0, 0, "glTexImage3D: mip level>0 (GPU chain)");
        (void)b;
        return;
    }
    tx.isCube = false;
    tx.w = w; tx.h = h; tx.d = (uint32_t)d; tx.internalFormat = inf; tx.levels = level + 1;
    tx.mipData.clear();
    tx.levelViews.clear();
    tx.layerViews.clear();
    tx.wasRT = false;
    // Unpack tight toàn bộ (z-major, mỗi slice h rows) — như 2D nhưng × d slice.
    size_t n = (size_t)w * h * d * BppTyped3D(f, ty);
    std::vector<uint8_t> unpacked(n, 0);
    if (hasData && w > 0 && h > 0 && d > 0) {
        size_t bpp = BppTyped3D(f, ty);
        size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
        size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                      (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
        const uint8_t* src = pixBase + skip;
        for (GLsizei z = 0; z < d; ++z)
            for (GLsizei r = 0; r < h; ++r)
                memcpy(unpacked.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                       src + ((size_t)z * h + r) * rowLen, (size_t)w * bpp);
    }
    c.MaybeTrimForAllocation(n);
    tx.gpu = (target == 0x8C1A)
        ? c.device->newTexture2DArray((uint32_t)w, (uint32_t)h, (uint32_t)d, ToMetalFormat(inf))
        : c.device->newTexture3D((uint32_t)w, (uint32_t)h, (uint32_t)d, ToMetalFormat(inf));
    c.pendingTexRegions.erase(tp->id);
    // Format/type có đường upload: UBYTE tight (RGBA/RED/RG) hoặc FLOAT (RED/RGBA).
    // Layout pixel khớp thẳng Metal format (R8/RG8/RGBA8/R32F/RGBA32F) → raw upload.
    bool gpuFmt = (ty == 0x1401 && (f == 0x1908 || f == 0x1903 || f == 0x8227)) ||
                  (ty == 0x1406 && (f == 0x1903 || f == 0x1908));
    if (tx.gpu && hasData && w > 0 && h > 0 && d > 0 && gpuFmt) {
        size_t bpp = BppTyped3D(f, ty);
        bool ok = true;
        for (GLsizei z = 0; z < d && ok; ++z)
            ok = c.device->updateTexture3D(tx.gpu.get(), (uint32_t)z, 0, 0,
                    (uint32_t)w, (uint32_t)h,
                    unpacked.data() + (size_t)z * (size_t)h * (size_t)w * bpp,
                    (size_t)w * bpp);
        if (!ok) c.LogDebug(0, 0, 0, 0, "glTexImage3D: GPU upload fail");
    } else if (tx.gpu && hasData && !gpuFmt) {
        static int nUnh3 = 0;
        if (c.DiagOn() && ++nUnh3 <= 32)
            fprintf(stderr, "[TGLMT] tex3nop#%d id=%u %dx%dx%d fmt=0x%x ty=0x%x giu-shadow\n",
                    nUnh3, tp->id, w, h, d, f, ty);
        c.LogDebug(0, 0, 0, 0, "glTexImage3D: format/type chưa upload GPU (giữ shadow)");
    }
    // 3D giữ shadow để GetTexImage/GetTexSubImage trả đúng volume: Metal không
    // cho view 3D→2D nên không thể readback từng slice như 2D array. 3D texture
    // của shader pack thường nhỏ; 2D array vẫn bỏ shadow và đọc GPU theo layer.
    if (c.device->isNull() || !tx.gpu || !gpuFmt || target == 0x806F)
        tx.pixels = std::move(unpacked);
    else
        std::vector<uint8_t>().swap(tx.pixels);
    (void)b; (void)level;
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
        if (++nBigSub <= 256) {
            fprintf(stderr,
                    "[TGLMT] bigTexSub#%d id=%u tgt=0x%x lv=%d off=(%d,%d) %dx%d fmt=0x%x "
                    "ty=0x%x tex=%ux%u\n",
                    nBigSub, tp->id, target, level, xoff, yoff, w, h, format, type, t.w,
                    t.h);
            fflush(stderr);
        }
    }
    // Level>0: GPU chỉ giữ base. Lưu vào shadow per-level (đúng GL: level có
    // dữ liệu riêng, GetTexImage(level) đọc được) thay vì drop câm như trước.
    // Không sync GPU (texture non-mipmapped, sampler NotMipmapped → không sample).
    if (level > 0) {
        // Cubemap face mip: shadow RIÊNG face; không dùng mipData/Promote 2D
        // (sẽ đổi cube thành texture 2D và upload nhầm slice 0).
        if (IsCubeFace(target) && t.target == 0x8513) {
            int face = (int)(target - 0x8515);
            t.isCube = true;
            GLsizei lw = std::max(1, (GLsizei)t.w >> level);
            GLsizei lh = std::max(1, (GLsizei)t.h >> level);
            if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
                xoff + w > lw || yoff + h > lh) { c.errors.Record(0x0501); return; }
            if (level + 1 > t.levels) t.levels = level + 1;
            auto& ml = t.faceMips[face][level];
            ml.w = lw; ml.h = lh; ml.d = 0;
            size_t bpp = Bpp(format, type);
            bool okBase = true;
            size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
            size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                          (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
            size_t total = skip + (h > 0 ? ((size_t)(h - 1) * rowLen + (size_t)w * bpp) : 0);
            const uint8_t* pixBase = UnpackBase(c, pixels, total, okBase);
            if (!okBase) { c.errors.Record(0x0501); return; }
            const uint8_t* src = pixBase + skip;
            const size_t fullBytes = (size_t)lw * lh * bpp;
            const bool fullLevel = (xoff == 0 && yoff == 0 && w == lw && h == lh);
            std::vector<uint8_t> upload;
            const bool gpuFmt = (format == 0x1908 || format == 0x1903) && type == 0x1401;
            if (!fullLevel && ml.pixels.empty() && gpuFmt && !c.device->isNull() &&
                t.gpu && t.gpu->levelCount() > (uint32_t)level) {
                std::vector<uint8_t> cur;
                if (c.ReadTextureGPU(t, cur, face, level) &&
                    cur.size() >= (size_t)lw * lh * 4)
                    upload.swap(cur);
            }
            std::vector<uint8_t>& dstBuf =
                upload.empty() ? ml.pixels : upload;
            if (dstBuf.size() < fullBytes) dstBuf.resize(fullBytes, 0);
            for (GLsizei r = 0; r < h; ++r) {
                uint8_t* dst = dstBuf.data() +
                    ((size_t)(yoff + r) * lw + (size_t)xoff) * bpp;
                memcpy(dst, src + r * rowLen, (size_t)w * bpp);
            }
            if (!upload.empty()) ml.pixels.clear();
            const uint8_t* up = upload.empty() ? ml.pixels.data() : upload.data();
            if (!c.device->isNull() && t.gpu && !t.wasRT)
                PromoteGpuMipChainCube(c, t, (uint32_t)t.levels);
            if (t.gpu && t.gpu->levelCount() > (uint32_t)level && gpuFmt)
                c.device->updateCubeFaceLevel(t.gpu.get(), (uint32_t)level,
                                              (uint32_t)face, up, (size_t)lw * bpp);
            ++c.appleStats.mipStaged;
            return;
        }
        auto it = t.mipData.find(level);
        if (it == t.mipData.end()) { c.errors.Record(0x0501); return; }
        auto& ml = it->second;
        if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
            (size_t)(xoff + w) > (size_t)ml.w || (size_t)(yoff + h) > (size_t)ml.h) {
            c.errors.Record(0x0501); return;
        }
        size_t bpp = Bpp(format, type);
        size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
        size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                      (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
        size_t total = skip + (h > 0 ? ((size_t)(h - 1) * rowLen + (size_t)w * bpp) : 0);
        bool okBase = true;
        const uint8_t* pixBase = UnpackBase(c, pixels, total, okBase);
        if (!okBase) { c.errors.Record(0x0501); return; }
        const uint8_t* src = pixBase + skip;
        const size_t fullBytes = (size_t)ml.w * ml.h * bpp;
        // P1b: texture immutable (glTexStorage2D) đã có ĐỦ mip chain trên GPU →
        // shadow CPU per-level chỉ là bản sao (Voxy upload mọi mip qua
        // glTexSubImage2D ⇒ 126MB mip shadow, xem jetsam). updateTextureLevel
        // nhận offset nên KHÔNG cần đọc lại GPU để patch: chỉ upload vùng, bỏ shadow.
        const bool gpuFmt = (format == 0x1908 || format == 0x1903 || format == 0x8227) &&
                            type == 0x1401;
        const bool gpuHasLevel = c.device && !c.device->isNull() && t.gpu &&
                                 t.gpu->levelCount() > (uint32_t)level;
        const bool keepShadow = !(gpuFmt && gpuHasLevel);
        std::vector<uint8_t> tight((size_t)w * h * bpp);
        for (GLsizei r = 0; r < h; ++r)
            memcpy(tight.data() + (size_t)r * w * bpp, src + r * rowLen, (size_t)w * bpp);
        if (keepShadow) {
            // GPU không giữ level → shadow là nguồn duy nhất cho readback.
            if (ml.pixels.size() < fullBytes) ml.pixels.resize(fullBytes, 0);
            for (GLsizei r = 0; r < h; ++r) {
                uint8_t* dst = ml.pixels.data() +
                    ((size_t)(yoff + r) * ml.w + (size_t)xoff) * bpp;
                memcpy(dst, tight.data() + (size_t)r * w * bpp, (size_t)w * bpp);
            }
        }
        if (level + 1 > t.levels) t.levels = level + 1;
        bool upOk = false;
        if (c.device && !c.device->isNull() && t.gpu) {
            PromoteGpuMipChain(c, t, (uint32_t)t.levels);
            if (t.gpu->levelCount() > (uint32_t)level)
                upOk = c.device->updateTextureLevel(t.gpu.get(), (uint32_t)level,
                                                    (uint32_t)xoff, (uint32_t)yoff,
                                                    (uint32_t)w, (uint32_t)h, tight.data(),
                                                    (size_t)w * bpp);
        }
        if (!keepShadow) {
            if (upOk) {
                std::vector<uint8_t>().swap(ml.pixels);
            } else {
                // GPU từ chối level → giữ shadow (vùng ngoài patch coi như chưa xác định).
                ml.pixels.assign(fullBytes, 0);
                for (GLsizei r = 0; r < h; ++r) {
                    uint8_t* dst = ml.pixels.data() +
                        ((size_t)(yoff + r) * ml.w + (size_t)xoff) * bpp;
                    memcpy(dst, tight.data() + (size_t)r * w * bpp, (size_t)w * bpp);
                }
            }
        }
        ++c.appleStats.mipStaged;
        c.LogDebug(0, 0, 0, 0, "glTexSubImage2D: mip level>0 (GPU chain, drop shadow)");
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
    // P1: payload tight đi theo region → flush không cần shadow CPU. Shadow (nếu
    // còn) vẫn cập nhật cho readback/diag; shadow rỗng → KHÔNG hồi sinh (đó chính
    // là bản sao CPU mà P1 bỏ để chống jetsam).
    std::vector<uint8_t> payload((size_t)w * h * bpp);
    for (GLsizei r = 0; r < h; ++r)
        memcpy(payload.data() + (size_t)r * w * bpp, src + r * rowLen, (size_t)w * bpp);
    if (!dstPix->empty()) {
        if (dstPix->size() < (size_t)t.w * t.h * bpp)
            dstPix->resize((size_t)t.w * t.h * bpp, 0);
        for (GLsizei r = 0; r < h; ++r) {
            uint8_t* dst = dstPix->data() + ((size_t)(yoff + r) * t.w + (size_t)xoff) * bpp;
            memcpy(dst, payload.data() + (size_t)r * w * bpp, (size_t)w * bpp);
        }
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
                       format, type, payload.data());
    }
    // Diag: sub-upload vùng lớn → shadow có data thật không (atlas streaming).
    if (c.DiagOn() && pixels && (w >= 256 || h >= 256)) LogShadowNZ(c, t, tp->id, "texsub");
}
void glTexSubImage1D(GLenum t, GLint l, GLint x, GLsizei w, GLenum f, GLenum ty, const void* p) {
    glTexSubImage2D(t, l, x, 0, w, 1, f, ty, p);
}
// P3: core chung cho glTexSubImage3D + glTextureSubImage3D (DSA). Unpack theo
// slice (IMAGE_HEIGHT = h), cập nhật shadow (nếu còn) + upload GPU per-slice.
static void TexSubImage3DCore(Context& c, TextureObject& t, GLint level,
        GLint xoff, GLint yoff, GLint zoff, GLsizei w, GLsizei h, GLsizei d,
        GLenum format, GLenum type, const void* pixels) {
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    if (!pixels && !hasUnpack) { c.errors.Record(0x0501); return; }
    if (w < 0 || h < 0 || d < 0 || xoff < 0 || yoff < 0 || zoff < 0 ||
        (size_t)(xoff + w) > t.w || (size_t)(yoff + h) > t.h ||
        (size_t)(zoff + d) > (size_t)t.d) {
        c.errors.Record(0x0501); return;
    }
    size_t bpp = BppTyped3D(format, type);
    size_t rowLen = UnpackRowLen(c, (size_t)w, bpp);
    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
    size_t total = skip + ((w > 0 && h > 0 && d > 0) ? (size_t)d * (size_t)h * rowLen : 0);
    bool okBase = true;
    const uint8_t* pixBase = UnpackBase(c, pixels, total, okBase);
    if (!okBase) { c.errors.Record(0x0501); return; }
    const uint8_t* src = pixBase + skip;
    std::vector<uint8_t> payload((size_t)w * h * d * bpp);
    for (GLsizei z = 0; z < d; ++z)
        for (GLsizei r = 0; r < h; ++r)
            memcpy(payload.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                   src + ((size_t)z * h + r) * rowLen, (size_t)w * bpp);
    if (level > 0) {
        auto it = t.mipData.find(level);
        if (it == t.mipData.end() || it->second.d == 0) { c.errors.Record(0x0501); return; }
        auto& ml = it->second;
        if ((size_t)(xoff + w) > (size_t)ml.w || (size_t)(yoff + h) > (size_t)ml.h ||
            (size_t)(zoff + d) > (size_t)ml.d) { c.errors.Record(0x0501); return; }
        if (level + 1 > t.levels) t.levels = level + 1;
        const bool gpuFmt =
            (type == 0x1401 && (format == 0x1908 || format == 0x1903 || format == 0x8227)) ||
            (type == 0x1406 && (format == 0x1903 || format == 0x1908));
        const bool gpuHas = c.device && !c.device->isNull() && t.gpu && gpuFmt &&
                            t.gpu->levelCount() > (uint32_t)level;
        // P1b: GPU đã có level → shadow CPU là bản sao, không materialize (jetsam).
        if (!gpuHas) {
            if (ml.pixels.size() < (size_t)ml.w * ml.h * ml.d * bpp)
                ml.pixels.resize((size_t)ml.w * ml.h * ml.d * bpp, 0);
            for (GLsizei z = 0; z < d; ++z)
                for (GLsizei r = 0; r < h; ++r) {
                    uint8_t* dst = ml.pixels.data() +
                        ((((size_t)(zoff + z) * ml.h + (yoff + r)) * ml.w + (size_t)xoff)) * bpp;
                    memcpy(dst, payload.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                           (size_t)w * bpp);
                }
        }
        if (gpuHas) {
            for (GLsizei z = 0; z < d; ++z)
                c.device->updateTexture3DLevel(
                    t.gpu.get(), (uint32_t)level, (uint32_t)(zoff + z),
                    (uint32_t)xoff, (uint32_t)yoff, (uint32_t)w, (uint32_t)h,
                    payload.data() + (size_t)z * (size_t)h * (size_t)w * bpp,
                    (size_t)w * bpp);
            std::vector<uint8_t>().swap(ml.pixels);
        }
        ++c.appleStats.mipStaged;
        return;
    }
    if (!t.pixels.empty()) {
        if (t.pixels.size() < (size_t)t.w * t.h * (size_t)t.d * bpp)
            t.pixels.resize((size_t)t.w * t.h * (size_t)t.d * bpp, 0);
        for (GLsizei z = 0; z < d; ++z)
            for (GLsizei r = 0; r < h; ++r) {
                uint8_t* dst = t.pixels.data() +
                    ((((size_t)(zoff + z) * t.h + (yoff + r)) * t.w + (size_t)xoff)) * bpp;
                memcpy(dst, payload.data() + (((size_t)z * h + r) * (size_t)w) * bpp,
                       (size_t)w * bpp);
            }
    }
    bool gpuFmt = (type == 0x1401 && (format == 0x1908 || format == 0x1903 || format == 0x8227)) ||
                  (type == 0x1406 && (format == 0x1903 || format == 0x1908));
    if (t.gpu && gpuFmt) {
        // Upload thẳng per-slice (immediate — 3D là init-time, không hot path).
        for (GLsizei z = 0; z < d; ++z)
            c.device->updateTexture3D(t.gpu.get(), (uint32_t)(zoff + z),
                    (uint32_t)xoff, (uint32_t)yoff, (uint32_t)w, (uint32_t)h,
                    payload.data() + (size_t)z * (size_t)h * (size_t)w * bpp,
                    (size_t)w * bpp);
    } else if (t.gpu) {
        static int nUnh3s = 0;
        if (c.DiagOn() && ++nUnh3s <= 32)
            fprintf(stderr, "[TGLMT] tex3subnop#%d id=%u fmt=0x%x ty=0x%x giu-shadow\n",
                    nUnh3s, t.id, format, type);
        c.LogDebug(0, 0, 0, 0, "glTexSubImage3D: format/type chưa upload GPU (giữ shadow)");
    }
}
void glTexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint zoff,
        GLsizei w, GLsizei h, GLsizei d, GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    if (target != 0x806F && target != 0x8C1A) { // CUBE_MAP_ARRAY chưa hỗ trợ
        c.LogDebug(0, 0, 0, 0, "glTexSubImage3D: target chưa hỗ trợ (chỉ 3D/2D_ARRAY)");
        return;
    }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    TexSubImage3DCore(c, *tp, level, xoff, yoff, zoff, w, h, d, format, type, pixels);
}
void glTexParameterf(GLenum t, GLenum p, GLfloat v) { glTexParameteri(t, p, (GLint)v); }
void glTexParameterfv(GLenum t, GLenum p, const GLfloat* v) { glTexParameteri(t, p, (GLint)v[0]); }
// EXT_texture_swizzle: GL swizzle value → MTLTextureSwizzle code
// (0=Zero,1=One,2=R,3=G,4=B,5=A) để ITexture::swizzledView áp lúc draw.
static uint8_t SwzCode(GLint v) {
    switch ((GLenum)v) {
        case 0x0000: return 0; // GL_ZERO
        case 0x0001: return 1; // GL_ONE
        case 0x1904: return 3; // GL_GREEN
        case 0x1905: return 4; // GL_BLUE
        case 0x1906: return 5; // GL_ALPHA
        default: return 2;     // GL_RED (và mọi giá trị lạ → spec RED)
    }
}
void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    Context& c = Context::Current();
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    tp->params[pname] = param;
    switch (pname) { // decode cho bind swizzled view (gl_apple_draw)
        case 0x8E42: tp->swz[0] = SwzCode(param); break; // SWIZZLE_R
        case 0x8E43: tp->swz[1] = SwzCode(param); break; // SWIZZLE_G
        case 0x8E44: tp->swz[2] = SwzCode(param); break; // SWIZZLE_B
        case 0x8E45: tp->swz[3] = SwzCode(param); break; // SWIZZLE_A
        default: break;
    }
}
void glTexParameteriv(GLenum t, GLenum p, const GLint* v) {
    // GL_TEXTURE_SWIZZLE_RGBA (0x8E46) nhận mảng 4 phần tử [R,G,B,A] → tách
    // store từng GL_TEXTURE_SWIZZLE_{R,G,B,A}. Các pnames khác giữ v[0].
    if (p == 0x8E46 && v) {
        glTexParameteri(t, 0x8E42, v[0]); // SWIZZLE_R
        glTexParameteri(t, 0x8E43, v[1]); // SWIZZLE_G
        glTexParameteri(t, 0x8E44, v[2]); // SWIZZLE_B
        glTexParameteri(t, 0x8E45, v[3]); // SWIZZLE_A
        return;
    }
    glTexParameteri(t, p, v[0]);
}
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
    GLuint id = c.state.BoundTexture(unit, target); // per-target (cube/buffer không clobber 2D)
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
        case 0x806E: ps.unpackImageHeight = param; break; // UNPACK_IMAGE_HEIGHT (0x0CF6 là SWAP_BYTES)
        case 0x806D: ps.unpackSkipImages = param; break;  // UNPACK_SKIP_IMAGES (0x0CF7 không phải skip images)
        case 0x0D02: ps.packRowLength = param; break;     // PACK_ROW_LENGTH
        case 0x0D03: ps.packSkipRows = param; break;      // PACK_SKIP_ROWS
        case 0x0D04: ps.packSkipPixels = param; break;    // PACK_SKIP_PIXELS
        case 0x0D05: ps.packAlignment = param; break;     // PACK_ALIGNMENT
        case 0x806B: ps.packSkipImages = param; break;    // PACK_SKIP_IMAGES
        case 0x806C: ps.packImageHeight = param; break;   // PACK_IMAGE_HEIGHT
        default: c.state.SetShadow(pname, &param, 4); break;
    }
}
// 1D texture trong TGLMT = storage 2D (w × 1) dùng chung slot 0 với 2D.
void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei width, GLint border) {
    glCopyTexImage2D(target, level, internalformat, x, y, width, 1, border);
}
void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoff,
                         GLint x, GLint y, GLsizei width) {
    glCopyTexSubImage2D(target, level, xoff, 0, x, y, width, 1);
}
static bool ReadFBRegionRGBA(Context& c, GLuint readFbo, GLint x, GLint y,
                             GLsizei w, GLsizei h, std::vector<uint8_t>& out);
// Copy framebuffer → MỘT mip level > 0. Metal render target không ghi trực tiếp
// vào mip view qua readback path; đọc vùng RGBA rồi upload bằng
// updateTextureLevel (rare: mip copy, không phải hot path). Chỉ RGBA8/sRGB
// 4B/px; format khác giữ base (log trung thực, không ghi rác).
static void CopyFBToTextureMip(Context& c, TextureObject& dst, GLint level,
                               GLint xoff, GLint yoff, GLint x, GLint y,
                               GLsizei w, GLsizei h) {
    if (level < 0) { c.errors.Record(0x0501); return; }
    if (dst.levels > 0 && level >= (GLint)dst.levels) {
        c.errors.Record(0x0501); return;
    }
    if (w <= 0 || h <= 0) { c.errors.Record(0x0501); return; }
    uint32_t lw = dst.w >> level, lh = dst.h >> level;
    if (!lw) lw = 1;
    if (!lh) lh = 1;
    if (xoff < 0 || yoff < 0 || x < 0 || y < 0 ||
        (size_t)(xoff + w) > lw || (size_t)(yoff + h) > lh) {
        c.errors.Record(0x0501); return;
    }
    if (!dst.gpu) { c.errors.Record(0x0502); return; }
    auto fmt = dst.gpu->pixelFormat();
    std::vector<uint8_t> reg;
    if (!ReadFBRegionRGBA(c, c.state.BoundReadFBO(), x, y, w, h, reg)) {
        c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage: mip source readback fail");
        return;
    }
    if (fmt == metal::PixelFormat::RGBA8Unorm ||
        fmt == metal::PixelFormat::RGBA8Unorm_sRGB) {
        if (dst.gpu->levelCount() > (uint32_t)level) {
            if (!c.device->updateTextureLevel(dst.gpu.get(), (uint32_t)level,
                                              (uint32_t)xoff, (uint32_t)yoff,
                                              (uint32_t)w, (uint32_t)h,
                                              reg.data(), (size_t)w * 4))
                c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage: mip upload fail");
        } else {
            // Legacy texture chỉ có base trên GPU: giữ mip shadow per-level.
            auto& ml = dst.mipData[level];
            ml.w = lw; ml.h = lh; ml.d = 1;
            ml.pixels.assign((size_t)lw * lh * 4, 0);
            for (GLsizei r = 0; r < h; ++r)
                memcpy(ml.pixels.data() + ((size_t)(yoff + r) * lw + (size_t)xoff) * 4,
                       reg.data() + (size_t)r * (size_t)w * 4, (size_t)w * 4);
        }
    } else {
        c.LogDebug(0, 0, 0, 0,
                   "glCopyTexSubImage: mip level>0 chỉ hỗ trợ RGBA8 (giữ base)");
    }
}
// Copy framebuffer (READ) → texture (blur/post-chain, menu loading đen nếu stub).
// Quy ước GL-order (hàng 0 = đáy GL) cho cả 2 đầu → copy raw GPU→GPU không flip,
// rồi sync shadow từ GPU để GetTexImage/ReadPixels sau đó thấy mới.
static void CopyFBToTexture(Context& c, TextureObject& dst, GLint level,
                            GLint xoff, GLint yoff, GLint x, GLint y,
                            GLsizei w, GLsizei h) {
    if (level != 0) {
        CopyFBToTextureMip(c, dst, level, xoff, yoff, x, y, w, h);
        return;
    }
    if (w <= 0 || h <= 0) { c.errors.Record(0x0501); return; }
    if (xoff < 0 || yoff < 0 || x < 0 || y < 0 ||
        (size_t)(xoff + w) > dst.w || (size_t)(yoff + h) > dst.h) {
        c.errors.Record(0x0501); return;
    }
    GLuint readFbo = c.state.BoundReadFBO();
    GLint readSlot = c.ReadBufferSlot(); // nguồn là READ_BUFFER (mặc định att0)
    if (readFbo != 0 && readSlot < 0) { c.errors.Record(0x0502); return; }
    if (!c.device || c.device->isNull()) {
        // Null backend: copy shadow nếu nguồn là FBO texture.
        if (readFbo != 0) {
            auto fit = c.fbos.find(readFbo);
            if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
            auto cit = fit->second.colorTex.find(readSlot);
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
    c.FlushPendingEncoder(); // end encoder để draws đi trước blit trong queue
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    // KHÔNG CommitAndWait: blitCopy/blitFromTarget tự tạo CB trên cùng serial
    // queue (AppleMetalBridge blitTex) → GPU đảm bảo thứ tự sau draws vừa
    // commit. Chờ ở đây làm mỗi copy 11-15ms (MakeUp 3387 calls = 39.7s stall).
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
        auto cit = fit->second.colorTex.find(readSlot);
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
            // P1: nguồn đọc từ GPU (shadow có thể rỗng trên Apple), không hồi sinh.
            std::vector<uint8_t> srcOwn;
            const std::vector<uint8_t>* sPx = nullptr;
            if (c.ReadTextureGPU(src, srcOwn)) sPx = &srcOwn;
            else if (src.pixels.size() >= (size_t)src.w * src.h * bpp) sPx = &src.pixels;
            if (sPx) {
                std::vector<uint8_t> own;
                std::vector<uint8_t>* dPx = &dst.pixels;
                if (dPx->size() < (size_t)dst.w * dst.h * bpp) {
                    own.assign((size_t)dst.w * dst.h * bpp, 0);
                    if (!dPx->empty()) memcpy(own.data(), dPx->data(), dPx->size());
                    dPx = &own;
                }
                for (GLsizei r = 0; r < h; ++r) {
                    if ((size_t)(y + r) >= src.h || (size_t)(yoff + r) >= dst.h) break;
                    memcpy(dPx->data() + ((size_t)(yoff + r) * dst.w + xoff) * bpp,
                           sPx->data() + ((size_t)(y + r) * src.w + x) * bpp,
                           (size_t)w * bpp);
                }
                if (dPx->size() >= (size_t)dst.w * dst.h * bpp)
                    c.device->updateTexture(dst.gpu.get(), 0, 0, dst.w, dst.h,
                                            dPx->data(), (size_t)dst.w * bpp);
                if (dPx == &own && !TexReadableFromGPU(dst)) dst.pixels = std::move(own);
                gpuOk = true;
            }
        }
    }
    // Không sync shadow đích từ GPU ở đây (giữ 60fps): sampling blur/post dùng
    // GPU mới; GetTexImage lên blur texture hiếm. Shadow cũ không ảnh hưởng draw.
    (void)gpuOk;
}
// Depth/stencil internalformat (spec 4.6 §8.3 + ARB_depth_texture + 4.4 stencil).
// Với các format này nguồn copy KHÔNG phải color read buffer mà là depth buffer
// của read framebuffer ("depth component data is taken from the depth buffer").
static bool IsDepthInternalFormat(GLenum f) {
    switch (f) {
        case 0x1902: // DEPTH_COMPONENT
        case 0x81A5: // DEPTH_COMPONENT16
        case 0x81A6: // DEPTH_COMPONENT24
        case 0x81A7: // DEPTH_COMPONENT32
        case 0x8CAC: // DEPTH_COMPONENT32F
        case 0x88F0: // DEPTH24_STENCIL8
            return true;
        default: return false;
    }
}
// Đọc vùng (x,y,w,h) từ read FB ra RGBA 4bpp (chỉ dùng cho cube-face copy —
// đường 2D dùng GPU blit). Nguồn: default drawable hoặc color attachment 0 của FBO.
static bool ReadFBRegionRGBA(Context& c, GLuint readFbo, GLint x, GLint y,
                             GLsizei w, GLsizei h, std::vector<uint8_t>& out) {
    out.assign((size_t)w * h * 4, 0);
    if (!c.device || c.device->isNull()) return false;
    if (readFbo == 0) {
        auto def = c.device->defaultRenderTarget();
        if (!def || x < 0 || y < 0 ||
            (GLint)(x + w) > (GLint)def->width() || (GLint)(y + h) > (GLint)def->height())
            return false;
        std::vector<uint8_t> scr((size_t)def->width() * def->height() * 4, 0);
        if (!def->readback(scr.data(), (size_t)def->width() * 4)) return false;
        for (GLsizei r = 0; r < h; ++r)
            memcpy(out.data() + (size_t)r * w * 4,
                   scr.data() + ((size_t)(y + r) * def->width() + x) * 4, (size_t)w * 4);
        return true;
    }
    auto fit = c.fbos.find(readFbo);
    if (fit == c.fbos.end()) return false;
    GLint slot = c.ReadBufferSlot();
    if (slot < 0) return false;
    auto cit = fit->second.colorTex.find(slot);
    if (cit == fit->second.colorTex.end()) return false;
    auto sit = c.textures.find(cit->second);
    if (sit == c.textures.end() || x < 0 || y < 0 ||
        (size_t)(x + w) > sit->second.w || (size_t)(y + h) > sit->second.h)
        return false;
    std::vector<uint8_t> srcOwn;
    const std::vector<uint8_t>* sp = nullptr;
    if (c.ReadTextureGPU(sit->second, srcOwn)) sp = &srcOwn;
    else if (sit->second.pixels.size() >= (size_t)sit->second.w * sit->second.h * 4)
        sp = &sit->second.pixels;
    if (!sp) return false;
    for (GLsizei r = 0; r < h; ++r)
        memcpy(out.data() + (size_t)r * w * 4,
               sp->data() + ((size_t)(y + r) * sit->second.w + x) * 4, (size_t)w * 4);
    return true;
}
// glCopyTexImage2D (spec 4.6 §18.3.2): ĐỊNH NGHĨA LẠI image level của texture
// đang bind (kích thước + internalformat) rồi copy vùng rect từ read framebuffer
// vào offset (0,0). Nguồn: color read buffer, RIÊNG internalformat depth/stencil
// → depth buffer của read FB (INVALID_OPERATION nếu read FB không có depth).
// Iris dùng đúng đường depth: RenderTargets.copyPreTranslucentDepth/copyPreHandDepth
// → copy depth vào texture depth (noTranslucents/noHand) để depth-test shader pack.
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei w, GLsizei h, GLint border) {
    Context& c = Context::Current();
    ++c.appleStats.copyTex;
    if (c.DiagOn()) {
        static uint64_t n = 0;
        if (++n <= 5) {
            fprintf(stderr,
                    "[TGLMT] copyTexImage#%llu tgt=0x%x level=%d ifmt=0x%x src=(%d,%d) size=%dx%d border=%d readFbo=%u\n",
                    (unsigned long long)n, target, level, internalformat, x, y, w, h,
                    border, c.state.BoundReadFBO());
            fflush(stderr);
        }
    }
    // Spec: target ∈ {2D, 6 cube faces} (INVALID_ENUM); border phải 0, level >= 0,
    // w/h trong [0, MAX_TEXTURE_SIZE] (INVALID_VALUE). MAX_TEXTURE_SIZE expose = 8192.
    if (target != 0x0DE1 && target != 0x0DE0 && !IsCubeFace(target)) { c.errors.Record(0x0500); return; }
    if (level < 0 || border != 0 || w < 0 || h < 0 || w > 8192 || h > 8192) {
        c.errors.Record(0x0501); return;
    }
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    auto& tx = *tp;
    bool depthFmt = IsDepthInternalFormat(internalformat);
    GLuint readFbo = c.state.BoundReadFBO();
    const TextureObject* depthSrc = nullptr;
    if (depthFmt) {
        // "no depth buffer" → INVALID_OPERATION, không đụng storage (spec).
        if (readFbo != 0) {
            auto fit = c.fbos.find(readFbo);
            if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
            GLuint did = fit->second.depthTex ? fit->second.depthTex
                                              : fit->second.depthStencilTex;
            if (!did) { c.errors.Record(0x0502); return; }
            auto dit = c.textures.find(did);
            if (dit != c.textures.end()) depthSrc = &dit->second;
        } else {
            auto def = c.device ? c.device->defaultRenderTarget() : nullptr;
            if (!def || !def->hasDepth()) { c.errors.Record(0x0502); return; }
            // default FB có depth nhưng drawable không có đường blit depth ra
            // texture → định nghĩa storage, bỏ copy (log trung thực).
        }
    }
    // Mip level > 0: giống glTexImage2D — GPU chỉ giữ base level, shadow metadata.
    if (level > 0) {
        if (level + 1 > tx.levels) tx.levels = level + 1;
        if (w > 0 && h > 0) {
            auto& ml = tx.mipData[level];
            ml.w = w; ml.h = h;
            ml.pixels.assign((size_t)w * h * 4, 0);
        }
        c.LogDebug(0, 0, 0, 0, "glCopyTexImage2D: level>0 lưu shadow (GPU base-level)");
        return;
    }
    // Cube face: định nghĩa per-face shadow + (color) đọc nguồn → updateCubeFace.
    // GPU cube chỉ RGBA8/R8 (bridge) → depth cube trung thực bỏ copy.
    if (IsCubeFace(target)) {
        int face = (int)(target - 0x8515);
        if (tx.isCube && w > 0 && h > 0 && (tx.w != (uint32_t)w || tx.h != (uint32_t)h)) {
            c.errors.Record(0x0501); return; // face lệch size (spec §8.5)
        }
        tx.isCube = true;
        tx.w = (uint32_t)w; tx.h = (uint32_t)h;
        tx.internalFormat = internalformat; tx.levels = 1;
        tx.wasRT = false;
        tx.faces[face].assign((size_t)w * h * 4, 0);
        if (w > 0 && h > 0 && !depthFmt) {
            std::vector<uint8_t> reg;
            if (ReadFBRegionRGBA(c, readFbo, x, y, w, h, reg)) {
                tx.faces[face] = std::move(reg);
                if (!tx.gpu)
                    tx.gpu = c.device->newCubeTexture((uint32_t)w, ToMetalFormat(internalformat));
                if (tx.gpu && tx.gpu->width() == (uint32_t)w)
                    c.device->updateCubeFace(tx.gpu.get(), (uint32_t)face,
                                             tx.faces[face].data(), (size_t)w * 4);
            } else {
                c.LogDebug(0, 0, 0, 0, "glCopyTexImage2D: cube face nguồn ngoài/đọc fail, shadow zero");
            }
        } else if (w > 0 && h > 0) {
            c.LogDebug(0, 0, 0, 0, "glCopyTexImage2D: cube face depth chưa hỗ trợ GPU, shadow zero");
        }
        tx.pixels = tx.faces[0];
        return;
    }
    // 2D: định nghĩa lại storage level 0 (giống glTexImage2D realloc).
    tx.isCube = false;
    tx.w = w; tx.h = h; tx.internalFormat = internalformat; tx.levels = 1;
    tx.mipData.clear();
    tx.wasRT = false;
    // Texture này đang là color HOẶC depth attachment của encoder đang mở:
    // wrapper target giữ gpu cũ → flush trước khi thay gpu (stale TBDR).
    bool wasCurrentRT = (c.pendingEncoder && c.IsPendingColorId(tp->id));
    if (!wasCurrentRT && c.pendingEncoder && c.pendingDrawFBO != 0) {
        auto pfit = c.fbos.find(c.pendingDrawFBO);
        if (pfit != c.fbos.end() && pfit->second.depthTex == tp->id) wasCurrentRT = true;
    }
    if (wasCurrentRT) c.FlushPendingEncoder();
    if (w > 0 && h > 0)
        tx.gpu = c.device->newTexture(w, h, ToMetalFormat(internalformat));
    else
        tx.gpu.reset(); // NULL image (spec: w/h = 0 → NULL texture), không copy
    c.pendingTexRegions.erase(tp->id);
    if (w <= 0 || h <= 0) return;
    if (depthFmt) {
        if (!depthSrc || !depthSrc->gpu || !tx.gpu) {
            c.LogDebug(0, 0, 0, 0, depthSrc ? "glCopyTexImage2D: depth GPU thiếu — bỏ copy"
                                            : "glCopyTexImage2D: default-FB depth chưa hỗ trợ, storage mới");
            return;
        }
        if (depthSrc->gpu->pixelFormat() != tx.gpu->pixelFormat()) {
            c.LogDebug(0, 0, 0, 0, "glCopyTexImage2D: depth format nguồn≠đích — bỏ copy");
            return;
        }
        // Vùng ngoài nguồn = undefined (spec) → clamp, không INVALID_VALUE.
        GLint cx = x < 0 ? 0 : x, cy = y < 0 ? 0 : y;
        GLsizei cw = w - (cx - x), ch = h - (cy - y);
        if ((size_t)(cx + cw) > depthSrc->w) cw = (GLsizei)depthSrc->w - cx;
        if ((size_t)(cy + ch) > depthSrc->h) ch = (GLsizei)depthSrc->h - cy;
        if (cw > (GLsizei)tx.w) cw = (GLsizei)tx.w;
        if (ch > (GLsizei)tx.h) ch = (GLsizei)tx.h;
        if (cw <= 0 || ch <= 0) return;
        c.FlushPendingEncoder();
        c.FlushAllBufferStaging();
        c.FlushAllTextureStaging();
        c.CommitAndWait(); // read FB phải render xong trước khi blit depth
        // blitCopy → bridge tự markDepthInited(dst): first-attach kế tiếp LOAD
        // (không first-clear) — đúng ngữ nghĩa Iris depth copy + depth-test.
        if (!c.device->blitCopy(depthSrc->gpu.get(), tx.gpu.get(),
                                (uint32_t)cx, (uint32_t)cy, (uint32_t)cw, (uint32_t)ch,
                                0, 0))
            c.LogDebug(0, 0, 0, 0, "glCopyTexImage2D: depth blit fail (giữ storage mới)");
        return;
    }
    // Color: dùng machinery CopyFBToTexture (FBO/default/null + CPU fallback).
    CopyFBToTexture(c, tx, 0, 0, 0, x, y, w, h);
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
// Copy 1 slice (zoff) của TEXTURE_3D từ read framebuffer. GPU: readback RGBA
// rồi upload đúng slice; Null: copy shadow trực tiếp.
static void CopyFBToTexture3D(Context& c, TextureObject& dst, GLint level,
                              GLint xoff, GLint yoff, GLint zoff,
                              GLint x, GLint y, GLsizei w, GLsizei h) {
    if (level < 0) { c.errors.Record(0x0501); return; }
    GLint lw = (GLint)dst.w, lh = (GLint)dst.h;
    GLint ld = dst.d ? (GLint)dst.d : 1;
    if (level > 0) {
        auto mit = dst.mipData.find(level);
        if (mit == dst.mipData.end() || mit->second.d == 0) {
            c.errors.Record(0x0501); return;
        }
        lw = mit->second.w; lh = mit->second.h; ld = mit->second.d;
    }
    if (w <= 0 || h <= 0 || xoff < 0 || yoff < 0 || zoff < 0 ||
        (size_t)(xoff + w) > (size_t)lw || (size_t)(yoff + h) > (size_t)lh ||
        (size_t)zoff >= (size_t)ld) {
        c.errors.Record(0x0501); return;
    }
    GLuint readFbo = c.state.BoundReadFBO();
    if (!c.device || c.device->isNull()) {
        if (readFbo == 0) return;
        auto fit = c.fbos.find(readFbo);
        if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        GLint slot = c.ReadBufferSlot();
        auto cit = fit->second.colorTex.find(slot);
        if (cit == fit->second.colorTex.end()) return;
        auto sit = c.textures.find(cit->second);
        if (sit == c.textures.end() || sit->second.pixels.empty()) return;
        auto& src = sit->second;
        if (src.pixels.size() < (size_t)src.w * src.h * 4) return;
        std::vector<uint8_t>* dstPix = nullptr;
        if (level > 0) {
            dstPix = &dst.mipData[level].pixels;
            if (dstPix->size() < (size_t)lw * lh * ld * 4)
                dstPix->assign((size_t)lw * lh * ld * 4, 0);
        } else {
            dstPix = &dst.pixels;
            if (dstPix->size() < (size_t)lw * lh * ld * 4)
                dstPix->resize((size_t)lw * lh * ld * 4, 0);
        }
        for (GLsizei r = 0; r < h; ++r) {
            if ((size_t)(y + r) >= src.h) break;
            memcpy(dstPix->data() + (((size_t)zoff * lh + (yoff + r)) * lw + xoff) * 4,
                   src.pixels.data() + ((size_t)(y + r) * src.w + x) * 4,
                   (size_t)w * 4);
        }
        return;
    }
    std::vector<uint8_t> reg;
    if (!ReadFBRegionRGBA(c, readFbo, x, y, w, h, reg)) {
        c.LogDebug(0, 0, 0, 0, "glCopyTexSubImage3D: readback fail");
        return;
    }
    glTextureSubImage3D(dst.id, level, xoff, yoff, zoff, w, h, 1,
                        0x1908 /*RGBA*/, 0x1401 /*UNSIGNED_BYTE*/, reg.data());
}
void glCopyTexSubImage3D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint zoff,
                         GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    TextureObject* tp = BoundTex(c, target);
    if (!tp) return;
    if (target != 0x806F) { c.errors.Record(0x0500); return; }
    CopyFBToTexture3D(c, *tp, level, xoff, yoff, zoff, x, y, w, h);
}
void glCopyTextureSubImage1D(GLuint tex, GLint level, GLint xoff, GLint x, GLint y, GLsizei width) {
    glCopyTextureSubImage2D(tex, level, xoff, 0, x, y, width, 1);
}
void glCopyTextureSubImage2D(GLuint tex, GLint level, GLint xoff, GLint yoff,
                             GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    ++c.appleStats.copyTex;
    auto it = c.textures.find(tex);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    CopyFBToTexture(c, it->second, level, xoff, yoff, x, y, w, h);
}
void glCopyTextureSubImage3D(GLuint tex, GLint level, GLint xoff, GLint yoff, GLint zoff,
                             GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    auto it = c.textures.find(tex);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.target != 0x806F) { c.errors.Record(0x0500); return; }
    CopyFBToTexture3D(c, it->second, level, xoff, yoff, zoff, x, y, w, h);
}
struct CopyTargetInfo {
    bool cube = false;   // cube map hoặc cube face
    bool array = false;  // 2D array
    bool volume = false; // 3D
    int face = -1;       // cube face target (0..5), -1 = cube map/khác
};
static bool ParseCopyTarget(GLenum t, CopyTargetInfo& o) {
    switch (t) {
        case 0x0DE1: return true;                 // TEXTURE_2D
        case 0x8C1A: o.array = true; return true; // TEXTURE_2D_ARRAY
        case 0x806F: o.volume = true; return true;// TEXTURE_3D
        case 0x8513: o.cube = true; return true;  // TEXTURE_CUBE_MAP
        case 0x8515: case 0x8516: case 0x8517:
        case 0x8518: case 0x8519: case 0x851A:
            o.cube = true; o.face = (int)(t - 0x8515); return true;
        default: return false;
    }
}
static bool CopyObjectMatches(const TextureObject& tx, const CopyTargetInfo& ti) {
    if (ti.cube) return tx.isCube;                 // cube map / face
    if (ti.array) return tx.target == 0x8C1A;
    if (ti.volume) return tx.target == 0x806F;
    return tx.target == 0x0DE1 && !tx.isCube;      // 2D
}
void glCopyImageSubData(GLuint srcName, GLenum srcTarget, GLint srcLevel,
                        GLint srcX, GLint srcY, GLint srcZ,
                        GLuint dstName, GLenum dstTarget, GLint dstLevel,
                        GLint dstX, GLint dstY, GLint dstZ,
                        GLsizei w, GLsizei h, GLsizei d) {
    Context& c = Context::Current();
    if (w <= 0 || h <= 0 || d <= 0) { c.errors.Record(0x0501); return; }
    if (srcLevel < 0 || dstLevel < 0) { c.errors.Record(0x0501); return; }
    CopyTargetInfo sti, dti;
    if (!ParseCopyTarget(srcTarget, sti) || !ParseCopyTarget(dstTarget, dti)) {
        c.errors.Record(0x0500); return;
    }
    auto sit = c.textures.find(srcName);
    auto dit = c.textures.find(dstName);
    if (sit == c.textures.end() || dit == c.textures.end()) {
        c.errors.Record(0x0502); return;
    }
    auto& src = sit->second;
    auto& dst = dit->second;
    if (!CopyObjectMatches(src, sti) || !CopyObjectMatches(dst, dti)) {
        c.errors.Record(0x0502); return;
    }
    // Cube face mip chưa có shadow/GPU face-mip; reject thay vì đọc face 0.
    if ((src.isCube || dst.isCube) && (srcLevel > 0 || dstLevel > 0)) {
        c.errors.Record(0x0502); return;
    }
    auto dimAt = [](uint32_t base, GLint level) -> uint32_t {
        uint32_t v = base >> level; return v ? v : 1;
    };
    uint32_t sw = dimAt(src.w, srcLevel), sh = dimAt(src.h, srcLevel);
    uint32_t dw = dimAt(dst.w, dstLevel), dh = dimAt(dst.h, dstLevel);
    uint32_t sd = src.isCube ? 6 : (src.d ? src.d : 1);
    uint32_t dd = dst.isCube ? 6 : (dst.d ? dst.d : 1);
    if (srcLevel > 0) {
        auto mit = src.mipData.find(srcLevel);
        if (mit == src.mipData.end()) { c.errors.Record(0x0501); return; }
        sw = (uint32_t)mit->second.w; sh = (uint32_t)mit->second.h;
        sd = mit->second.d ? (uint32_t)mit->second.d : (src.d ? src.d : 1);
    }
    if (dstLevel > 0) {
        auto mit = dst.mipData.find(dstLevel);
        if (mit == dst.mipData.end()) { c.errors.Record(0x0501); return; }
        dw = (uint32_t)mit->second.w; dh = (uint32_t)mit->second.h;
        dd = mit->second.d ? (uint32_t)mit->second.d : (dst.d ? dst.d : 1);
    }
    // Với cube-face target, z là 0 và depth chỉ 1 (face chỉ định nằm trong target).
    int szBase = sti.face >= 0 ? sti.face : 0;
    int dzBase = dti.face >= 0 ? dti.face : 0;
    uint32_t sdz = sti.face >= 0 ? 1 : sd;
    uint32_t ddz = dti.face >= 0 ? 1 : dd;
    if (srcX < 0 || srcY < 0 || srcZ < 0 || dstX < 0 || dstY < 0 || dstZ < 0 ||
        (size_t)(srcX + w) > sw || (size_t)(srcY + h) > sh ||
        (size_t)(srcZ + d) > sdz || (size_t)(dstX + w) > dw ||
        (size_t)(dstY + h) > dh || (size_t)(dstZ + d) > ddz) {
        c.errors.Record(0x0501); return;
    }
    // GPU path: cube copy từng face qua textureLayerView (không dùng blit 3D
    // vì MTLTextureTypeCube không có slice z). 3D/array giữ blit slice cũ.
    if (c.device && !c.device->isNull()) {
        if (src.isCube) {
            if (!src.gpu || !dst.gpu) { c.errors.Record(0x0502); return; }
            bool ok = true;
            for (GLsizei k = 0; k < d && ok; ++k) {
                auto sv = src.gpu->textureLayerView(0, (uint32_t)(szBase + srcZ + k));
                auto dv = dst.gpu->textureLayerView(0, (uint32_t)(dzBase + dstZ + k));
                ok = sv && dv && c.device->blitCopy(
                    sv.get(), dv.get(), (uint32_t)srcX, (uint32_t)srcY,
                    (uint32_t)w, (uint32_t)h, (uint32_t)dstX, (uint32_t)dstY);
            }
            if (ok) return;
            c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: cube blit fail -> CPU shadow");
        } else {
            std::shared_ptr<metal::ITexture> srcView, dstView;
            if (srcLevel > 0 || dstLevel > 0) {
                if (!src.gpu || !dst.gpu ||
                    src.gpu->levelCount() <= (uint32_t)srcLevel ||
                    dst.gpu->levelCount() <= (uint32_t)dstLevel) {
                    c.errors.Record(0x0502); return;
                }
                srcView = src.gpu->textureView(src.gpu->pixelFormat(),
                                               (uint32_t)srcLevel, 1, 0, 1);
                dstView = dst.gpu->textureView(dst.gpu->pixelFormat(),
                                               (uint32_t)dstLevel, 1, 0, 1);
                if (!srcView || !dstView) { c.errors.Record(0x0502); return; }
            }
            metal::ITexture* srcGpu = srcView ? srcView.get() : src.gpu.get();
            metal::ITexture* dstGpu = dstView ? dstView.get() : dst.gpu.get();
            if (!srcGpu || !dstGpu) { c.errors.Record(0x0502); return; }
            if (srcGpu->pixelFormat() == dstGpu->pixelFormat()) {
                const bool multiSlice = d > 1 || srcZ != 0 || dstZ != 0;
                bool blitOk = multiSlice
                    ? c.device->blitCopySlices(srcGpu, dstGpu,
                        (uint32_t)srcX, (uint32_t)srcY, (uint32_t)srcZ,
                        (uint32_t)dstX, (uint32_t)dstY, (uint32_t)dstZ,
                        (uint32_t)w, (uint32_t)h, (uint32_t)d)
                    : c.device->blitCopy(srcGpu, dstGpu,
                        (uint32_t)srcX, (uint32_t)srcY, (uint32_t)w, (uint32_t)h,
                        (uint32_t)dstX, (uint32_t)dstY);
                if (blitOk) return;
            }
            c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: blit fail -> CPU shadow");
        }
    }
    // CPU shadow fallback: volume/array/cube, đúng z/face. Chỉ copy khi cả hai
    // phía có slice hợp lệ; thiếu shadow (GPU-only) → bỏ qua trung thực.
    auto shadowSlice = [](const TextureObject& t, GLint level, int z,
                          uint32_t lw, uint32_t lh, uint32_t ld)
        -> const std::vector<uint8_t>* {
        if (level > 0) {
            auto it = t.mipData.find(level);
            if (it == t.mipData.end()) return nullptr;
            return &it->second.pixels;
        }
        if (t.isCube) {
            if (z < 0 || z >= 6) return nullptr;
            return &t.faces[z];
        }
        (void)ld;
        return &t.pixels;
    };
    auto shadowOffset = [](const TextureObject& t, GLint level, int z,
                           uint32_t lw, uint32_t lh) -> size_t {
        if (t.isCube || level > 0) return 0; // cube: face vector; mip: base của level
        return (size_t)z * lw * lh * 4;
    };
    for (GLsizei k = 0; k < d; ++k) {
        const int sz = szBase + srcZ + k, dz = dzBase + dstZ + k;
        const std::vector<uint8_t>* sSlice = shadowSlice(src, srcLevel, sz, sw, sh, sd);
        std::vector<uint8_t>* dSlice = const_cast<std::vector<uint8_t>*>(
            shadowSlice(dst, dstLevel, dz, dw, dh, dd));
        if (!sSlice || !dSlice) {
            c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: shadow thiếu slice (bỏ qua)");
            return;
        }
        const size_t sOff = shadowOffset(src, srcLevel, sz, sw, sh);
        const size_t dOff = shadowOffset(dst, dstLevel, dz, dw, dh);
        const size_t sNeed = sOff + (size_t)sh * sw * 4;
        const size_t dNeed = dOff + (size_t)dh * dw * 4;
        if (sSlice->size() < sNeed || dSlice->size() < dNeed) {
            c.LogDebug(0, 0, 0, 0, "glCopyImageSubData: shadow slice ngắn (bỏ qua)");
            return;
        }
        for (GLsizei r = 0; r < h; ++r) {
            memcpy(dSlice->data() + dOff + ((size_t)(dstY + r) * dw + dstX) * 4,
                   sSlice->data() + sOff + ((size_t)(srcY + r) * sw + srcX) * 4,
                   (size_t)w * 4);
        }
    }
}
// TGLMT chưa map compressed texture format nào: GL_NUM_COMPRESSED_TEXTURE_FORMATS
// = 0. Theo spec, internalformat/format nén không hỗ trợ → INVALID_ENUM; không
// được im lặng nhận rồi để texture rỗng. Khi thêm ASTC/ETC2/BPTC, thay các
// hàm này bằng đường upload thật và cập nhật probe.
static void CompressedUnsupported() { Context::Current().errors.Record(0x0500); }
void glCompressedTexImage1D(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTexImage3D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTexSubImage1D(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTextureSubImage1D(GLuint, GLint, GLint, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTextureSubImage2D(GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
void glCompressedTextureSubImage3D(GLuint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const void*) { CompressedUnsupported(); }
// Ghi 1 texel clear-value (format,type,data) vào dst (srcBpp bytes).
// Hỗ trợ UNSIGNED_BYTE / FLOAT / HALF_FLOAT / USHORT / UINT + NULL (zero).
// Trả false nếu format/type không hỗ trợ → caller INVALID_ENUM (trung thực).
static bool WriteClearTexel(GLenum format, GLenum type, const void* data,
                            uint8_t* dst, size_t srcBpp) {
    if (!data) { memset(dst, 0, srcBpp); return true; }
    switch (type) {
        case 0x1401: // UNSIGNED_BYTE — copy trực tiếp (RGB=3, RGBA=4, RED=1, ...)
            memcpy(dst, data, srcBpp); return true;
        case 0x1406: { // FLOAT → normalize theo format
            const float* f = (const float*)data;
            // Số component theo format
            size_t nc = (format == 0x1907) ? 3 : (format == 0x1908) ? 4 :
                        (format == 0x8227) ? 2 : 1; // RGB / RGBA / RG / RED-etc
            // srcBpp bytes đích: nếu bpp == nc thì là float data, else uchar data.
            if (srcBpp == nc * 4) { memcpy(dst, f, srcBpp); return true; }
            for (size_t i = 0; i < nc && i < srcBpp; ++i) {
                float v = f[i]; if (v < 0) v = 0; if (v > 1) v = 1;
                dst[i] = (uint8_t)(v * 255.0f + 0.5f);
            }
            for (size_t i = nc; i < srcBpp; ++i) dst[i] = 0;
            return true;
        }
        case 0x140B: { // HALF_FLOAT (2B/component) → uchar (xấp xỉ)
            // Đơn giản: coi như half → convert approx sang uchar nếu dst là uchar.
            const uint16_t* h = (const uint16_t*)data;
            size_t nc = (format == 0x1907) ? 3 : (format == 0x1908) ? 4 :
                        (format == 0x8227) ? 2 : 1;
            if (srcBpp == nc * 2) { memcpy(dst, h, srcBpp); return true; }
            for (size_t i = 0; i < nc && i < srcBpp; ++i) {
                // half → float → uchar (đầy đủ ở đây không cần, chỉ cần phổ biến)
                uint16_t hh = h[i]; uint32_t sign = (hh >> 15) & 1;
                uint32_t exp = (hh >> 10) & 0x1F, mant = hh & 0x3FF;
                float fv = 0.f;
                if (exp == 0) fv = mant * 5.9604644775390625e-8f;
                else if (exp == 31) fv = mant ? 0.f : 1e30f;
                else fv = (1 + mant / 1024.0f) * powf(2.0f, (int)exp - 15);
                if (sign) fv = -fv;
                if (fv < 0) fv = 0; if (fv > 1) fv = 1;
                dst[i] = (uint8_t)(fv * 255.0f + 0.5f);
            }
            for (size_t i = nc; i < srcBpp; ++i) dst[i] = 0;
            return true;
        }
        case 0x1403: // UNSIGNED_SHORT — copy trực tiếp nếu dst là ushort
            memcpy(dst, data, srcBpp); return true;
        case 0x1405: // UNSIGNED_INT — copy trực tiếp
            memcpy(dst, data, srcBpp); return true;
        case 0x140A: // GL_DOUBLE — copy trực tiếp (dst cùng cỡ; FLOAT đã xử ở 0x1406)
            memcpy(dst, data, srcBpp); return true;
        default:
            return false; // không đoán — trung thực INVALID_ENUM
    }
}
static void ClearTexLevel(Context& c, TextureObject& t, GLint level, GLint xoff, GLint yoff,
                          GLsizei w, GLsizei h, GLenum format, GLenum type, const void* data) {
    size_t srcBpp = Bpp(format, type);
    // Xác định pixel đích (base level → t.pixels; level>0 → mipData)
    std::vector<uint8_t>* dstPix = &t.pixels;
    GLsizei lw = (GLsizei)t.w, lh = (GLsizei)t.h;
    if (level == 0) {
        // base level
    } else {
        auto it = t.mipData.find(level);
        if (it == t.mipData.end()) { c.errors.Record(0x0500); return; } // level chưa alloc
        dstPix = &it->second.pixels;
        lw = it->second.w; lh = it->second.h;
    }
    if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
        (size_t)(xoff + w) > (size_t)lw || (size_t)(yoff + h) > (size_t)lh) {
        c.errors.Record(0x0501); return;
    }
    if (w <= 0 || h <= 0) return;
    // Pixel format phải tương thích internalFormat (spec §8.11: format phải khớp
    // 1 trong các cơ sở của ifmt). Kiểm lỏng: bỏ qua khi lệch (vẫn điền shadow).
    uint8_t texel[16] = {0};
    if (!WriteClearTexel(format, type, data, texel, srcBpp)) {
        c.errors.Record(0x0500); return; // INVALID_ENUM — format/type không hỗ trợ
    }
    // Upload GPU (base level hoặc mip). P1b: mip chain của texture immutable
    // đã nằm trên GPU → clear thẳng lên GPU, không materialize lại shadow CPU
    // (else sẽ hồi sinh đúng 126MB vừa bỏ).
    const bool gpuFmt = (format == 0x1908 || format == 0x1903 || format == 0x8227) &&
                        type == 0x1401;
    const bool gpuLevel = c.device && !c.device->isNull() && t.gpu &&
                          t.gpu->levelCount() > (uint32_t)level;
    if (level > 0 && gpuFmt && gpuLevel) {
        std::vector<uint8_t> tight((size_t)w * h * srcBpp, 0);
        for (GLsizei r = 0; r < h; ++r)
            for (GLsizei x = 0; x < w; ++x)
                memcpy(tight.data() + ((size_t)r * w + x) * srcBpp, texel, srcBpp);
        if (c.device->updateTextureLevel(t.gpu.get(), (uint32_t)level,
                                         (uint32_t)xoff, (uint32_t)yoff,
                                         (uint32_t)w, (uint32_t)h, tight.data(),
                                         (size_t)w * srcBpp)) {
            ++c.appleStats.mipStaged;
            return;
        }
    }
    // Điền shadow tight (row-major, bpp theo src format)
    if (dstPix->size() < (size_t)lw * lh * srcBpp)
        dstPix->resize((size_t)lw * lh * srcBpp, 0);
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* row = dstPix->data() + ((size_t)(yoff + r) * lw + (size_t)xoff) * srcBpp;
        for (GLsizei x = 0; x < w; ++x) memcpy(row + (size_t)x * srcBpp, texel, srcBpp);
    }
    if (level == 0) {
        std::vector<uint8_t> tight((size_t)w * h * srcBpp);
        for (GLsizei r = 0; r < h; ++r)
            memcpy(tight.data() + (size_t)r * w * srcBpp,
                   dstPix->data() + ((size_t)(yoff + r) * lw + (size_t)xoff) * srcBpp,
                   (size_t)w * srcBpp);
        SyncRegionToGPU(c, t, xoff, yoff, w, h, tight.data(), (size_t)w * srcBpp, format, type);
    } else {
        ++c.appleStats.mipStaged;
    }
}
// Cube map: zoffset là face (0..5), depth phải = 1. Mỗi face có shadow riêng
// (t.faces[face]); GPU upload cả face qua updateCubeFace sau khi clear.
static void ClearTexFace(Context& c, TextureObject& t, GLint face, GLint xoff, GLint yoff,
                         GLsizei w, GLsizei h, GLenum format, GLenum type, const void* data) {
    if (face < 0 || face > 5 || !t.isCube) { c.errors.Record(0x0501); return; }
    if (xoff < 0 || yoff < 0 || w < 0 || h < 0 ||
        (size_t)(xoff + w) > (size_t)t.w || (size_t)(yoff + h) > (size_t)t.h) {
        c.errors.Record(0x0501); return;
    }
    if (!w || !h) return;
    size_t bpp = Bpp(format, type);
    uint8_t texel[16] = {0};
    if (!WriteClearTexel(format, type, data, texel, bpp)) {
        c.errors.Record(0x0500); return;
    }
    auto& facePix = t.faces[face];
    if (facePix.size() < (size_t)t.w * t.h * bpp)
        facePix.resize((size_t)t.w * t.h * bpp, 0);
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* row = facePix.data() +
            ((size_t)(yoff + r) * (size_t)t.w + (size_t)xoff) * bpp;
        for (GLsizei x = 0; x < w; ++x) memcpy(row + (size_t)x * bpp, texel, bpp);
    }
    if (face == 0) t.pixels = facePix;
    if (t.gpu && (format == 0x1908 || format == 0x1903 || format == 0x80E1) &&
        type == 0x1401) {
        c.device->updateCubeFace(t.gpu.get(), (uint32_t)face, facePix.data(),
                                 (size_t)t.w * bpp);
    }
}
static bool ClearTexLevelDims(const TextureObject& t, GLint level,
                              GLint* lw, GLint* lh, GLint* ld) {
    if (level < 0) return false;
    if (level == 0) {
        *lw = (GLint)t.w; *lh = (GLint)t.h;
        *ld = t.d ? (GLint)t.d : 1;
        return true;
    }
    auto it = t.mipData.find(level);
    if (it == t.mipData.end()) return false;
    *lw = it->second.w; *lh = it->second.h;
    *ld = it->second.d ? (GLint)it->second.d : 1;
    return true;
}
static bool IsVolumeTexture(const TextureObject& t) {
    return t.d > 1 || t.target == 0x806F /*TEXTURE_3D*/ ||
           t.target == 0x8C1A /*TEXTURE_2D_ARRAY*/;
}
static void ClearTexVolume(Context& c, TextureObject& t, GLint level,
                           GLint xoff, GLint yoff, GLint zoff,
                           GLsizei w, GLsizei h, GLsizei d,
                           GLenum format, GLenum type, const void* data) {
    GLint lw = 0, lh = 0, ld = 0;
    if (!ClearTexLevelDims(t, level, &lw, &lh, &ld)) {
        c.errors.Record(0x0501); return;
    }
    if (w < 0 || h < 0 || d < 0 || xoff < 0 || yoff < 0 || zoff < 0 ||
        (size_t)(xoff + w) > (size_t)lw || (size_t)(yoff + h) > (size_t)lh ||
        (size_t)(zoff + d) > (size_t)ld) {
        c.errors.Record(0x0501); return;
    }
    if (!w || !h || !d) return;
    size_t bpp = BppTyped3D(format, type);
    uint8_t texel[16] = {0};
    if (!WriteClearTexel(format, type, data, texel, bpp)) {
        c.errors.Record(0x0500); return;
    }
    std::vector<uint8_t> payload((size_t)w * h * d * bpp);
    for (size_t i = 0; i < (size_t)w * h * d; ++i)
        memcpy(payload.data() + i * bpp, texel, bpp);
    TexSubImage3DCore(c, t, level, xoff, yoff, zoff, w, h, d, format, type, payload.data());
}
void glClearTexImage(GLuint tex, GLint level, GLenum format, GLenum type, const void* data) {
    Context& c = Context::Current();
    auto it = c.textures.find(tex);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    GLint w = 0, h = 0, d = 0;
    if (!ClearTexLevelDims(it->second, level, &w, &h, &d)) {
        c.errors.Record(0x0501); return;
    }
    if (it->second.isCube) {
        if (level == 0) {
            for (int face = 0; face < 6; ++face)
                ClearTexFace(c, it->second, face, 0, 0, w, h, format, type, data);
        } else {
            ClearTexLevel(c, it->second, level, 0, 0, w, h, format, type, data);
        }
        return;
    }
    if (IsVolumeTexture(it->second))
        ClearTexVolume(c, it->second, level, 0, 0, 0, w, h, d, format, type, data);
    else
        ClearTexLevel(c, it->second, level, 0, 0, w, h, format, type, data);
}
void glClearTexSubImage(GLuint tex, GLint level, GLint x, GLint y, GLint z,
                        GLsizei w, GLsizei h, GLsizei d, GLenum format, GLenum type,
                        const void* data) {
    Context& c = Context::Current();
    auto it = c.textures.find(tex);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.isCube) {
        if (level != 0 || d != 1) { c.errors.Record(0x0502); return; }
        ClearTexFace(c, it->second, z, x, y, w, h, format, type, data);
        return;
    }
    if (IsVolumeTexture(it->second)) {
        ClearTexVolume(c, it->second, level, x, y, z, w, h, d, format, type, data);
        return;
    }
    if (z != 0 || d != 1) { c.errors.Record(0x0501); return; }
    ClearTexLevel(c, it->second, level, x, y, w, h, format, type, data);
}
void glInvalidateTexImage(GLuint a, GLint b) { (void)a;(void)b; }
void glInvalidateTexSubImage(GLuint a, GLint b, GLint c_, GLint d, GLint e, GLsizei f, GLsizei g, GLsizei h) { (void)a;(void)b;(void)c_;(void)d;(void)e;(void)f;(void)g;(void)h; }
void glTextureStorage1D(GLuint t, GLsizei l, GLenum inf, GLsizei w) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    it->second.w = w; it->second.h = 1; it->second.internalFormat = inf; it->second.levels = l;
    // P1: immutable storage chỉ CẤP chỗ (không có dữ liệu) → shadow toàn số 0,
    // không có thông tin. Giữ lại đúng như glTexImage2D: Null / không có gpu /
    // format không đọc được từ GPU.
    if (c.device->isNull() || !it->second.gpu || !TexReadableFromGPU(it->second))
        it->second.pixels.assign((size_t)w * 4, 0);
    else
        std::vector<uint8_t>().swap(it->second.pixels);
}
void glTextureStorage2D(GLuint t, GLsizei l, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (l < 1) { c.errors.Record(0x0501); return; }
    {
        GLsizei maxL = 1;
        for (GLsizei m = (w > h ? w : h); m > 1; m >>= 1) ++maxL;
        if (l > maxL) { c.errors.Record(0x0501); return; }
    }
    it->second.w = w; it->second.h = h; it->second.internalFormat = inf; it->second.levels = l;
    // glTexStorage cấp đủ mip chain; shadow per-level để glTexSubImage(level>0)
    // có đích hợp lệ và upload lên GPU level tương ứng.
    it->second.mipData.clear();
    for (GLint lv = 1; lv < l; ++lv) {
        auto& ml = it->second.mipData[lv];
        ml.w = w >> lv; if (ml.w < 1) ml.w = 1;
        ml.h = h >> lv; if (ml.h < 1) ml.h = 1;
        ml.d = 0;
    }
    bool wasRTD = it->second.wasRT;
    bool wasCurRTD = (c.pendingEncoder && c.IsPendingColorId(t));
    if (wasCurRTD) c.FlushPendingEncoder();
    if (c.DiagOn() && (wasRTD || wasCurRTD || (w >= 256 && h >= 256))) {
        static int nSwapD = 0;
        if (++nSwapD <= 64)
            fprintf(stderr, "[TGLMT] gpuswapDSA#%d id=%u %dx%d wasRT=%d curRT=%d\n",
                    nSwapD, t, w, h, (int)wasRTD, (int)wasCurRTD);
    }
    it->second.wasRT = false;
    c.pendingTexRegions.erase(t);
    it->second.levelViews.clear();
    it->second.layerViews.clear();
    // Voxy world re-create: texture immutable 384MB + full mip chain. Trim trước
    // nếu RAM trống thấp (xem Context::MaybeTrimForAllocation).
    c.MaybeTrimForAllocation((size_t)w * (size_t)h * 4 * 4 / 3);
    it->second.gpu = c.device->newTextureLevels(
        (uint32_t)w, (uint32_t)h, ToMetalFormat(it->second.internalFormat), (uint32_t)l);
    // P1: storage chỉ cấp chỗ → shadow toàn số 0, không thông tin. Cùng luật với
    // glTexImage2D: giữ shadow chỉ khi Null / không có gpu / format không đọc
    // được từ GPU. (Atlas màu 2048² = 16MB shadow ×N — đây là một phần jetsam.)
    if (c.device->isNull() || !it->second.gpu || !TexReadableFromGPU(it->second))
        it->second.pixels.assign((size_t)w * h * 4, 0);
    else
        std::vector<uint8_t>().swap(it->second.pixels);
}
void glTextureStorage3D(GLuint t, GLsizei l, GLenum inf, GLsizei w, GLsizei h, GLsizei d) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (w < 1 || h < 1 || d < 1 || l < 1) { c.errors.Record(0x0501); return; }
    GLsizei maxDim = w > h ? w : h;
    if (d > maxDim) maxDim = d;
    GLsizei maxL = 1;
    for (GLsizei m = maxDim; m > 1; m >>= 1) ++maxL;
    if (l > maxL) { c.errors.Record(0x0501); return; }
    const bool isArray = (it->second.target == 0x8C1A);
    it->second.w = w; it->second.h = h; it->second.d = d;
    it->second.internalFormat = inf; it->second.levels = l;
    it->second.isCube = false;
    it->second.wasRT = false;
    it->second.levelViews.clear();
    it->second.layerViews.clear();
    // Immutable storage cấp đủ mip chain; shadow per-level (rỗng) để
    // glTexSubImage3D(level>0) có đích hợp lệ, giống glTextureStorage2D.
    it->second.mipData.clear();
    for (GLint lv = 1; lv < l; ++lv) {
        auto& ml = it->second.mipData[lv];
        ml.w = (GLsizei)w >> lv; if (ml.w < 1) ml.w = 1;
        ml.h = (GLsizei)h >> lv; if (ml.h < 1) ml.h = 1;
        ml.d = isArray ? d : ((GLsizei)d >> lv);
        if (ml.d < 1) ml.d = 1;
    }
    c.pendingTexRegions.erase(t);
    it->second.gpu = isArray
        ? c.device->newTexture2DArrayLevels((uint32_t)w, (uint32_t)h, (uint32_t)d,
                                            (uint32_t)l, ToMetalFormat(inf))
        : c.device->newTexture3DLevels((uint32_t)w, (uint32_t)h, (uint32_t)d,
                                       (uint32_t)l, ToMetalFormat(inf));
    // Cùng luật shadow với glTextureStorage2D: chỉ giữ khi Null / không gpu /
    // format không đọc được từ GPU.
    if (c.device->isNull() || !it->second.gpu || !TexReadableFromGPU(it->second))
        it->second.pixels.assign((size_t)w * h * (size_t)d * 4, 0);
    else
        std::vector<uint8_t>().swap(it->second.pixels);
}
// MSAA storage 2D: backend tạo multisample color + resolve; gpu = resolve để
// sampling/readback thấy kết quả sau pass. Chỉ RGBA8/sRGB8A8 được hỗ trợ (các
// format khác cần pipeline format riêng — báo lỗi thay vì âm thầm single-sample).
static bool MSAAFormatOK(GLenum inf) { return inf == 0x8058 || inf == 0x8C43; }
static bool SetupMSAA2D(Context& c, TextureObject& tx, GLsizei samples, GLenum inf,
                        GLsizei w, GLsizei h) {
    if (tx.immutableFormat) { c.errors.Record(0x0502); return false; }
    if (samples < 1 || samples > 4) { c.errors.Record(0x0501); return false; }
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return false; }
    if (!MSAAFormatOK(inf)) { c.errors.Record(0x0502); return false; }
    GLuint keepId = tx.id;
    tx = TextureObject{};
    tx.id = keepId;
    tx.target = 0x9100; // TEXTURE_2D_MULTISAMPLE
    tx.levels = 1;
    tx.internalFormat = inf;
    tx.w = (uint32_t)w; tx.h = (uint32_t)h;
    tx.samples = samples;
    if (w == 0 || h == 0) return true;
    if (!c.device || c.device->isNull()) {
        tx.pixels.assign((size_t)w * (size_t)h * 4, 0);
        return true;
    }
    auto t = c.device->makeMSAATarget((uint32_t)w, (uint32_t)h, (uint32_t)samples,
                                     ToMetalFormat(inf));
    if (!t) { c.errors.Record(0x0502); return false; }
    tx.msaaTarget = t;
    tx.gpu = t->resolvedTexture();
    if (!tx.gpu) { c.errors.Record(0x0502); return false; }
    return true;
}
static bool SetupMSAA2DArray(Context& c, TextureObject& tx, GLsizei samples, GLenum inf,
                             GLsizei w, GLsizei h, GLsizei d) {
    if (tx.immutableFormat) { c.errors.Record(0x0502); return false; }
    if (samples < 1 || samples > 4) { c.errors.Record(0x0501); return false; }
    if (w < 0 || h < 0 || d < 0) { c.errors.Record(0x0501); return false; }
    if (!MSAAFormatOK(inf)) { c.errors.Record(0x0502); return false; }
    GLuint keepId = tx.id;
    tx = TextureObject{};
    tx.id = keepId;
    tx.target = 0x9102; // TEXTURE_2D_MULTISAMPLE_ARRAY
    tx.levels = 1;
    tx.internalFormat = inf;
    tx.w = (uint32_t)w; tx.h = (uint32_t)h; tx.d = (uint32_t)d;
    tx.samples = samples;
    if (w == 0 || h == 0 || d == 0) return true;
    if (!c.device || c.device->isNull()) {
        tx.pixels.assign((size_t)w * (size_t)h * (size_t)d * 4, 0);
        return true;
    }
    auto t = c.device->makeMSAATargetLayered((uint32_t)w, (uint32_t)h, (uint32_t)d,
                                              (uint32_t)samples, ToMetalFormat(inf));
    if (!t) { c.errors.Record(0x0502); return false; }
    tx.msaaTarget = t;
    tx.gpu = t->resolvedTexture();
    if (!tx.gpu) { c.errors.Record(0x0502); return false; }
    return true;
}
void glTextureStorage2DMultisample(GLuint a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLboolean g) {
    Context& c = Context::Current();
    (void)g;
    auto it = c.textures.find(a);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.target != 0x9100) { c.errors.Record(0x0500); return; }
    if (SetupMSAA2D(c, it->second, b, d, e, f))
        it->second.immutableFormat = true;
}
void glTextureStorage3DMultisample(GLuint a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g, GLboolean h) {
    Context& c = Context::Current();
    (void)h;
    auto it = c.textures.find(a);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    if (it->second.target != 0x9102) { c.errors.Record(0x0500); return; }
    if (SetupMSAA2DArray(c, it->second, b, d, e, f, g))
        it->second.immutableFormat = true;
}
void glTexImage2DMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h, GLboolean f) {
    Context& c = Context::Current();
    (void)f;
    if (t != 0x9100) { c.errors.Record(0x0500); return; }
    TextureObject* tp = BoundTex(c, t);
    if (!tp) return;
    SetupMSAA2D(c, *tp, s, inf, w, h);
}
void glTexImage3DMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h, GLsizei d, GLboolean f) {
    Context& c = Context::Current();
    (void)f;
    if (t != 0x9102) { c.errors.Record(0x0500); return; }
    TextureObject* tp = BoundTex(c, t);
    if (!tp) return;
    SetupMSAA2DArray(c, *tp, s, inf, w, h, d);
}
// glTexStorage* (non-DSA, GL 4.2/4.3): resolve texture đang bind trên active unit
// rồi gọi DSA thật. HÀNH VI CŨ LÀ NO-OP → texture không bao giờ có GPU allocation
// → glTexSubImage2D chỉ ghi shadow → draw ra MTLTexture nil/đen. Sodium dùng
// immutable storage (chuẩn 4.5+) nên mọi texture Sodium đều rơi vào nhánh này.
// Route sang DSA để có GPU allocation thật (glTextureStorage1D/2D đã làm thật).
static bool BoundTexIdForStorage(Context& c, GLenum target, GLuint* out) {
    GLuint unit = c.state.ActiveTexture();
    GLuint id = c.state.BoundTexture(unit, target);
    if (!id) { c.errors.Record(0x0502); return false; }
    if (!c.textures.count(id)) { c.errors.Record(0x0502); return false; }
    *out = id;
    return true;
}
void glTexStorage1D(GLenum a, GLsizei b, GLenum d, GLsizei e) {
    Context& c = Context::Current();
    GLuint t;
    if (!BoundTexIdForStorage(c, a, &t)) return;
    glTextureStorage1D(t, b, d, e);
}
void glTexStorage2D(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f) {
    Context& c = Context::Current();
    GLuint t;
    if (!BoundTexIdForStorage(c, a, &t)) return;
    glTextureStorage2D(t, b, d, e, f);
}
void glTexStorage3D(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g) {
    Context& c = Context::Current();
    GLuint t;
    if (!BoundTexIdForStorage(c, a, &t)) return;
    glTextureStorage3D(t, b, d, e, f, g);
}
void glTexStorage2DMultisample(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLboolean g) {
    Context& c = Context::Current();
    GLuint t;
    if (!BoundTexIdForStorage(c, a, &t)) return;
    glTextureStorage2DMultisample(t, b, d, e, f, g);
}
void glTexStorage3DMultisample(GLenum a, GLsizei b, GLenum d, GLsizei e, GLsizei f, GLsizei g, GLboolean h) {
    Context& c = Context::Current();
    GLuint t;
    if (!BoundTexIdForStorage(c, a, &t)) return;
    glTextureStorage3DMultisample(t, b, d, e, f, g, h);
}
void glTextureSubImage1D(GLuint t, GLint level, GLint xoff, GLsizei width,
                         GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    // TGLMT biểu diễn 1D bằng storage 2D (w × 1); target vẫn phải là 1D để
    // không âm thầm sửa texture 2D cùng tên.
    if (it->second.target != 0x0DE0) { c.errors.Record(0x0502); return; }
    // glTexSubImage1D chỉ tác động texture đang bind. Giữ nguyên binding của
    // active unit rồi khôi phục để DSA không làm lệch state của app.
    GLuint unit = c.state.ActiveTexture();
    GLuint prev = c.state.BoundTexture(unit, 0x0DE0);
    GLenum prevTarget = c.state.BoundTextureTarget(unit);
    c.state.BindTextureUnit(unit, t, 0x0DE0);
    glTexSubImage1D(0x0DE0, level, xoff, width, format, type, pixels);
    c.state.BindTextureUnit(unit, prev, prev ? prevTarget : 0);
}
void glTextureSubImage3D(GLuint t, GLint level, GLint xoff, GLint yoff, GLint zoff,
        GLsizei w, GLsizei h, GLsizei d, GLenum format, GLenum type, const void* pixels) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    TexSubImage3DCore(c, it->second, level, xoff, yoff, zoff, w, h, d, format, type, pixels);
}
void glTextureSubImage2D(GLuint t, GLint l, GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, const void* p) {
    Context& c = Context::Current();
    auto it = c.textures.find(t);
    if (it == c.textures.end()) { c.errors.Record(0x0502); return; }
    // PBO unpack: p là offset khi UNPACK bound (offset 0 hợp lệ).
    bool hasUnpack = c.state.BoundBuffer(0x88EC /*PIXEL_UNPACK_BUFFER*/) != 0;
    if (!p && !hasUnpack) { c.errors.Record(0x0501); return; }
    if ((w >= 256 || h >= 256)) {
            static int nBigDSA = 0;
            if (++nBigDSA <= 256) {
            fprintf(stderr,
                    "[TGLMT] bigTexSubDSA#%d id=%u lv=%d off=(%d,%d) %dx%d fmt=0x%x ty=0x%x "
                    "tex=%ux%u\n",
                    nBigDSA, t, l, x, y, w, h, f, ty, it->second.w, it->second.h);
            fflush(stderr);
        }
    }
    if (l > 0) {
        auto& tx0 = it->second;
        auto md = tx0.mipData.find(l);
        if (md == tx0.mipData.end()) { c.errors.Record(0x0501); return; }
        auto& ml = md->second;
        if (x < 0 || y < 0 || w < 0 || h < 0 ||
            (size_t)(x + w) > (size_t)ml.w || (size_t)(y + h) > (size_t)ml.h) {
            c.errors.Record(0x0501); return;
        }
        size_t bpp0 = Bpp(f, ty);
        size_t rowLen0 = UnpackRowLen(c, (size_t)w, bpp0);
        size_t skip0 = (size_t)c.state.PixelStore().unpackSkipRows * rowLen0 +
                       (size_t)c.state.PixelStore().unpackSkipPixels * bpp0;
        size_t total0 = skip0 + (h > 0 ? ((size_t)(h - 1) * rowLen0 + (size_t)w * bpp0) : 0);
        bool ok0 = true;
        const uint8_t* base0 = UnpackBase(c, p, total0, ok0);
        if (!ok0) { c.errors.Record(0x0501); return; }
        const uint8_t* src0 = base0 + skip0;
        if (ml.pixels.size() < (size_t)ml.w * ml.h * bpp0)
            ml.pixels.resize((size_t)ml.w * ml.h * bpp0, 0);
        std::vector<uint8_t> tight0((size_t)w * h * bpp0);
        for (GLsizei r = 0; r < h; ++r) {
            memcpy(ml.pixels.data() + ((size_t)(y + r) * ml.w + (size_t)x) * bpp0,
                   src0 + r * rowLen0, (size_t)w * bpp0);
            memcpy(tight0.data() + (size_t)r * w * bpp0, src0 + r * rowLen0,
                   (size_t)w * bpp0);
        }
        if (l + 1 > tx0.levels) tx0.levels = l + 1;
        if (tx0.gpu && c.device && !c.device->isNull()) {
            PromoteGpuMipChain(c, tx0, (uint32_t)tx0.levels);
            if (tx0.gpu->levelCount() > (uint32_t)l)
                c.device->updateTextureLevel(tx0.gpu.get(), (uint32_t)l, (uint32_t)x,
                                             (uint32_t)y, (uint32_t)w, (uint32_t)h,
                                             tight0.data(), (size_t)w * bpp0);
        }
        ++c.appleStats.mipStaged;
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
    size_t skip = (size_t)c.state.PixelStore().unpackSkipRows * rowLen +
                  (size_t)c.state.PixelStore().unpackSkipPixels * bpp;
    size_t total = skip + (h > 0 ? ((size_t)(h - 1) * rowLen + (size_t)w * bpp) : 0);
    bool okBase = true;
    const uint8_t* pixBase = UnpackBase(c, p, total, okBase);
    if (!okBase) { c.errors.Record(0x0501); return; }
    const uint8_t* src = pixBase + skip;
    std::vector<uint8_t> payload((size_t)w * h * bpp);
    for (GLsizei r = 0; r < h; ++r)
        memcpy(payload.data() + (size_t)r * w * bpp, src + r * rowLen, (size_t)w * bpp);
    if (!tx.pixels.empty()) {
        if (tx.pixels.size() < (size_t)tx.w * tx.h * bpp)
            tx.pixels.resize((size_t)tx.w * tx.h * bpp, 0);
        for (GLsizei r = 0; r < h; ++r) {
            uint8_t* dst = tx.pixels.data() + ((size_t)(y + r) * tx.w + (size_t)x) * bpp;
            memcpy(dst, payload.data() + (size_t)r * w * bpp, (size_t)w * bpp);
        }
    }
    // IR deferred: stage thay vì sync GPU ngay (DSA path của game 26.x).
    (void)src; (void)rowLen;
    StageTexRegion(c, t, (uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, f, ty,
                   payload.data());
}
// P10 ARB_texture_view: tạo view chia sẻ storage (vanilla 26.x mip bake dùng).
// View trỏ tới subset mips/layers của gốc, có thể reinterpret format (tương thích).
void glTextureView(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat,
                   GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
    Context& c = Context::Current();
    auto ito = c.textures.find(origtexture);
    if (ito == c.textures.end()) { c.errors.Record(0x0502); return; } // INVALID_OPERATION
    if (ito->second.viewOf != 0) { c.errors.Record(0x0502); return; } // view của view → lỗi
    if (numlevels == 0 || numlayers == 0) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    // Tạo/ghi đè TextureObject cho view (texture phải là tên mới hoặc chưa dùng?
    // Spec: texture phải chưa có storage (mới gen hoặc đã delete). TGLMT: cho phép
    // ghi đè nếu chưa có gpu (tránh leak); nếu đã có gpu → lỗi để an toàn.
    auto itv = c.textures.find(texture);
    if (itv != c.textures.end() && itv->second.gpu) { c.errors.Record(0x0502); return; }
    TextureObject& v = c.textures[texture]; // tạo mới hoặc reuse rỗng
    v.id = texture;
    v.target = target;
    v.viewOf = origtexture;
    v.viewBaseLevel = (GLint)minlevel; v.viewNumLevels = (GLint)numlevels;
    v.viewBaseLayer = (GLint)minlayer; v.viewNumLayers = (GLint)numlayers;
    v.viewFormat = internalformat;
    // Kích thước view = mip minlevel của gốc (w>>minlevel).
    v.w = ito->second.w >> minlevel; v.h = ito->second.h >> minlevel;
    if (v.w == 0) v.w = 1; if (v.h == 0) v.h = 1;
    v.internalFormat = internalformat;
    v.levels = (GLsizei)numlevels;
    // GPU: tạo Metal texture view chia sẻ storage (đúng mip/slice).
    // Format: internalformat GL → PixelFormat (dùng ToMetalFormat, fallback base).
    if (ito->second.gpu && c.device && !c.device->isNull()) {
        metal::PixelFormat vfmt = ToMetalFormat(internalformat);
        // Nếu internalformat==0 hoặc không map (trả RGBA8 mặc định nhưng gốc khác),
        // dùng format gốc để view tương thích (đúng khi view cùng format).
        if (internalformat == 0) vfmt = ito->second.gpu->pixelFormat();
        auto views = ito->second.gpu->textureView(vfmt, minlevel, numlevels, minlayer, numlayers);
        if (views) v.gpu = views;
        else v.gpu = ito->second.gpu; // fallback: share base (đúng khi cùng format/mip0)
    }
    // CPU (Null): view đọc qua original's storage + baseLevel (xử lý ở GetTexImage/readback).
}
} // namespace tglmt::gl
