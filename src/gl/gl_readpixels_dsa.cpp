// gl_readpixels_dsa.cpp — vét 47 hàm còn lại:
// ReadPixels (readback FBO → blit/getTexture), TexBuffer/View (buffer-backed texture),
// TransformFeedbackVaryings, VertexArray* DSA, VertexAttrib s/b-variants, misc state.
// Spec: glspec46.core.pdf §18.2 (ReadPixels), §8.9 (Buffer Textures), §10.3 (VAO DSA).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>
using namespace tglmt;

namespace tglmt::gl {
// --- ReadPixels: đọc từ READ framebuffer (spec §18.2). GPU path khi có device thật:
// bọc texture/default-target rồi readback (toàn bộ vùng; x/y/offset vùng M5b tinh chỉnh).
// Chỉ RGBA/UNSIGNED_BYTE (format/type khác → INVALID_OPERATION trung thực, M5b mở rộng).
static size_t ReadPixelsBpp(GLenum format, GLenum type) {
    size_t comps = 0;
    switch (format) {
        case 0x1908: case 0x80E1: comps = 4; break; // RGBA / BGRA
        case 0x1907: comps = 3; break;              // RGB
        case 0x8227: comps = 2; break;              // RG
        case 0x1903: comps = 1; break;              // RED
        default: return 0;
    }
    switch (type) {
        case 0x1401: return comps;              // UNSIGNED_BYTE
        case 0x1403: return comps * 2;          // UNSIGNED_SHORT
        case 0x1406: return comps * 4;          // FLOAT
        case 0x8035: case 0x8367:               // UNSIGNED_INT_8_8_8_8(_REV)
            return comps == 4 ? 4 : 0;
        default: return 0;
    }
}
static size_t ReadPixelsRowLen(Context& c, size_t w, size_t bpp) {
    GLint rl = c.state.PixelStore().packRowLength;
    size_t elems = (rl > 0) ? (size_t)rl : w;
    GLint al = c.state.PixelStore().packAlignment;
    size_t align = (al == 1 || al == 2 || al == 4 || al == 8) ? (size_t)al : 4;
    return ((elems * bpp + align - 1) / align) * align;
}
static void StoreReadPixels(uint8_t* d, const uint8_t* s, GLenum format, GLenum type) {
    uint8_t r = s[0], g = s[1], b = s[2], a = s[3];
    uint8_t comps[4] = {r, g, b, a};
    size_t n = 4;
    switch (format) {
        case 0x80E1: comps[0] = b; comps[2] = r; n = 4; break; // BGRA
        case 0x1907: n = 3; break;                            // RGB
        case 0x8227: n = 2; break;                            // RG
        case 0x1903: n = 1; break;                            // RED
        default: break;
    }
    if (type == 0x1401) {
        for (size_t i = 0; i < n; ++i) d[i] = comps[i];
    } else if (type == 0x1403) {
        uint16_t* o = (uint16_t*)d;
        for (size_t i = 0; i < n; ++i) o[i] = (uint16_t)(comps[i] * 257);
    } else if (type == 0x1406) {
        float* o = (float*)d;
        for (size_t i = 0; i < n; ++i) o[i] = comps[i] / 255.0f;
    } else if (type == 0x8035) { // UNSIGNED_INT_8_8_8_8
        uint32_t v = 0;
        for (size_t i = 0; i < n; ++i) v |= (uint32_t)comps[i] << (8 * i);
        memcpy(d, &v, 4);
    } else if (type == 0x8367) { // UNSIGNED_INT_8_8_8_8_REV
        uint32_t v = 0;
        for (size_t i = 0; i < n; ++i) v |= (uint32_t)comps[i] << (8 * (n - 1 - i));
        memcpy(d, &v, 4);
    }
}
static void CopyReadRegion(std::vector<uint8_t>& out, const uint8_t* full,
                           size_t tw, size_t th, GLint x, GLint y,
                           GLsizei w, GLsizei h) {
    out.assign((size_t)w * (size_t)h * 4, 0);
    for (GLsizei r = 0; r < h; ++r) {
        GLint srcRow = y + r;
        if (srcRow < 0 || (size_t)srcRow >= th) continue;
        GLint sx = std::max<GLint>(x, 0);
        GLint ex = std::min<GLint>(x + w, (GLint)tw);
        if (ex <= sx) continue;
        size_t copyW = (size_t)(ex - sx);
        size_t dstX = (size_t)(sx - x);
        memcpy(out.data() + ((size_t)r * w + dstX) * 4,
               full + ((size_t)srcRow * tw + (size_t)sx) * 4, copyW * 4);
    }
}
void glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, void* p) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; }
    // PACK PBO bound → p là offset (offset 0 hợp lệ). Không PBO + NULL mới lỗi.
    GLuint packBuf = c.state.BoundBuffer(0x88EB /*PIXEL_PACK_BUFFER*/);
    if (!p && !packBuf) { c.errors.Record(0x0501); return; }
    size_t outBpp = ReadPixelsBpp(f, ty);
    if (!outBpp) { c.errors.Record(0x0502); return; }
    if (!w || !h) return;

    // Đọc vùng (x,y,w,h) thành RGBA8 tight; ngoài biên giữ 0.
    std::vector<uint8_t> rgba((size_t)w * h * 4, 0);
    bool got = false;
    GLuint readFbo = c.state.BoundReadFBO();
    const TextureObject* tex = nullptr;
    GLint fboLevel = 0, fboLayer = -1;
    if (readFbo != 0) {
        auto fit = c.fbos.find(readFbo);
        if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        GLint slot = c.ReadBufferSlot();
        if (slot < 0) { c.errors.Record(0x0502); return; } // READ_BUFFER=GL_NONE
        auto cit = fit->second.colorTex.find(slot);
        if (cit == fit->second.colorTex.end()) { c.errors.Record(0x0502); return; }
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end()) { c.errors.Record(0x0502); return; }
        tex = &tit->second;
        auto lit = fit->second.colorLevel.find(slot);
        if (lit != fit->second.colorLevel.end()) fboLevel = lit->second;
        auto yit = fit->second.colorLayer.find(slot);
        if (yit != fit->second.colorLayer.end()) fboLayer = yit->second;
    }
    if (c.device && !c.device->isNull()) {
        c.FlushPendingEncoder();
        c.FlushAllBufferStaging();
        c.FlushAllTextureStaging();
        c.CommitAndWait();
        std::shared_ptr<metal::IRenderTarget> tgt;
        if (tex) {
            if (!tex->gpu) { c.errors.Record(0x0502); return; }
            std::shared_ptr<metal::ITexture> view;
            if (tex->isCube || tex->d > 1) {
                GLint layer = fboLayer >= 0 ? fboLayer : 0;
                view = tex->gpu->textureLayerView((uint32_t)fboLevel, (uint32_t)layer);
            } else if (fboLevel > 0) {
                view = tex->gpu->textureView(tex->gpu->pixelFormat(),
                                             (uint32_t)fboLevel, 1, 0, 1);
            }
            tgt = c.device->wrapAsTarget(view ? view.get() : tex->gpu.get(), nullptr);
        } else {
            tgt = c.device->defaultRenderTarget();
        }
        if (tgt) {
            size_t tgtBpp = 4;
            if (tex && tex->gpu) {
                tgtBpp = metal::PixelFormatBytes(tex->gpu->pixelFormat());
                if (tgtBpp == 0) tgtBpp = 4;
            }
            if (tgtBpp != 4) {
                c.LogDebug(0, 0, 0, 0,
                           "glReadPixels: target HDR/không 4B/px → dùng shadow");
            } else {
                std::vector<uint8_t> full((size_t)tgt->width() * tgt->height() * 4, 0);
                if (tgt->readback(full.data(), (size_t)tgt->width() * 4)) {
                    CopyReadRegion(rgba, full.data(), tgt->width(), tgt->height(),
                                   x, y, w, h);
                    got = true;
                }
            }
        }
        if (!got) c.LogDebug(0, 0, 0, 0, "glReadPixels: GPU readback fail, dùng shadow");
    }
    if (!got) {
        if (tex) {
            const GLint lw = (GLint)((tex->w >> fboLevel) ? (tex->w >> fboLevel) : 1);
            const GLint lh = (GLint)((tex->h >> fboLevel) ? (tex->h >> fboLevel) : 1);
            const uint8_t* sp = nullptr;
            std::vector<uint8_t> gpuPx;
            const std::vector<uint8_t>* stored = nullptr;
            if (tex->isCube && fboLayer >= 0 && fboLayer < 6) {
                stored = &tex->faces[fboLayer];
            } else if (tex->d > 1 && fboLayer >= 0 &&
                       tex->pixels.size() >= (size_t)(fboLayer + 1) * lw * lh * 4) {
                stored = &tex->pixels;
            } else if (fboLevel > 0 && !tex->mipData.empty()) {
                auto mit = tex->mipData.find(fboLevel);
                if (mit != tex->mipData.end()) stored = &mit->second.pixels;
            } else {
                stored = &tex->pixels;
            }
            std::vector<uint8_t> layerOwn;
            if (stored && tex->d > 1 && fboLayer >= 0 && stored == &tex->pixels) {
                const size_t off = (size_t)fboLayer * lw * lh * 4;
                if (stored->size() >= off + (size_t)lw * lh * 4) {
                    layerOwn.assign(stored->begin() + off, stored->begin() + off + lw * lh * 4);
                    stored = &layerOwn;
                }
            }
            if (stored && stored->size() >= (size_t)lw * lh * 4) sp = stored->data();
            if (!sp) {
                GLint readLayer = (tex->isCube || tex->d > 1)
                    ? (fboLayer >= 0 ? fboLayer : 0) : -1;
                if (c.ReadTextureGPU(*tex, gpuPx, readLayer, fboLevel))
                    sp = gpuPx.data();
            }
            if (sp) CopyReadRegion(rgba, sp, lw, lh, x, y, w, h);
        } else {
            uint8_t c4[4] = {(uint8_t)(c.clearColor[0] * 255), (uint8_t)(c.clearColor[1] * 255),
                             (uint8_t)(c.clearColor[2] * 255), (uint8_t)(c.clearColor[3] * 255)};
            for (size_t i = 0; i < (size_t)w * h; ++i) memcpy(rgba.data() + i * 4, c4, 4);
        }
    }

    size_t dstRow = ReadPixelsRowLen(c, (size_t)w, outBpp);
    size_t skip = (size_t)c.state.PixelStore().packSkipRows * dstRow +
                  (size_t)c.state.PixelStore().packSkipPixels * outBpp;
    size_t need = skip + (size_t)(h - 1) * dstRow + (size_t)w * outBpp;
    uint8_t* dstBase = nullptr;
    std::vector<uint8_t>* pboVec = nullptr;
    size_t pboOff = 0;
    if (packBuf) {
        auto bit = c.buffers.find(packBuf);
        if (bit == c.buffers.end()) { c.errors.Record(0x0502); return; }
        pboOff = (size_t)p;
        pboVec = c.MutablePbo(packBuf, pboOff + need);
        if (!pboVec) { c.errors.Record(0x0502); return; }
        dstBase = pboVec->data() + pboOff + skip;
    } else {
        dstBase = (uint8_t*)p + skip;
    }
    for (GLsizei r = 0; r < h; ++r) {
        uint8_t* drow = dstBase + (size_t)r * dstRow;
        const uint8_t* srow = rgba.data() + (size_t)r * w * 4;
        for (GLsizei i = 0; i < w; ++i)
            StoreReadPixels(drow + (size_t)i * outBpp, srow + (size_t)i * 4, f, ty);
    }
    if (pboVec) c.StageBufferRange(packBuf, pboOff, need);
}
void glReadnPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, GLsizei n, void* p) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; }
    size_t bpp = ReadPixelsBpp(f, ty);
    if (!bpp) { glReadPixels(x, y, w, h, f, ty, p); return; } // báo INVALID_OPERATION
    // KHR_robustness: không được ghi quá bufSize; tính cả PACK_ROW_LENGTH/SKIP.
    size_t row = ReadPixelsRowLen(c, (size_t)w, bpp);
    size_t skip = (size_t)c.state.PixelStore().packSkipRows * row +
                  (size_t)c.state.PixelStore().packSkipPixels * bpp;
    size_t need = skip;
    if (w > 0 && h > 0) need += (size_t)(h - 1) * row + (size_t)w * bpp;
    if ((size_t)n < need) { c.errors.Record(0x0502); return; }
    glReadPixels(x, y, w, h, f, ty, p);
}
// --- misc state ---
void glClampColor(GLenum t, GLenum cl) { Context::Current().state.SetShadow(t, &cl, 4); }
void glProvokingVertex(GLenum m) { Context::Current().state.SetShadow(0x8E4F /*PROVOKING_VERTEX*/, &m, 4); }
void glTextureBarrier() {
    Context& c = Context::Current();
    // Metal không có lệnh barrier cấp texture như GL. Đóng encoder hiện tại
    // (end + commitNoWait) là điểm thứ tự đủ mạnh: command buffer cùng queue
    // chạy in-order và Metal hazard-track resource, nên draw/read sau barrier
    // thấy write trước barrier. Null backend không có hazard GPU → no-op.
    if (c.device && !c.device->isNull()) c.FlushPendingEncoder();
}
void glReleaseShaderCompiler() {}
void glShaderBinary(GLsizei n, const GLuint* s, GLenum f, const void* b, GLsizei l) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; }          // INVALID_VALUE
    if (n && (!s || !b)) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    (void)l;
    // Không có binary format nào (NUM_SHADER_BINARY_FORMATS = 0) → mọi format
    // đều không được hỗ trợ; spec ARB_ES2_compatibility: INVALID_ENUM.
    c.errors.Record(0x0500);
}
static GLuint PipeStage(const ProgramPipelineObject& pp, GLenum q) {
    GLbitfield bit = 0;
    switch (q) {
        case 0x8B31: bit = 0x1; break;  // VERTEX_SHADER
        case 0x8B30: bit = 0x2; break;  // FRAGMENT_SHADER
        case 0x8DD9: bit = 0x4; break;  // GEOMETRY_SHADER
        case 0x8E88: bit = 0x8; break;  // TESS_CONTROL_SHADER
        case 0x8E87: bit = 0x10; break; // TESS_EVALUATION_SHADER
        case 0x91B9: bit = 0x20; break; // COMPUTE_SHADER
        default: return 0;
    }
    auto it = pp.stagePrograms.find(bit);
    return it == pp.stagePrograms.end() ? 0 : it->second;
}
void glValidateProgramPipeline(GLuint p) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(p);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); return; }
    bool ok = true;
    for (GLbitfield bit : {0x1u, 0x2u}) {
        auto st = it->second.stagePrograms.find(bit);
        if (st == it->second.stagePrograms.end() || st->second == 0) { ok = false; break; }
        auto pit = c.programs.find(st->second);
        if (pit == c.programs.end() || !pit->second.linked) { ok = false; break; }
    }
    it->second.validateStatus = ok ? 1 : 0;
    it->second.infoLog = ok ? "" : "error: pipeline requires linked vertex and fragment stages";
}
void glGetProgramPipelineiv(GLuint p, GLenum q, GLint* v) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(p);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); return; }
    if (!v) return;
    switch (q) {
        case 0x8259: *v = (GLint)it->second.activeProgram; break; // ACTIVE_PROGRAM
        case 0x8B31: case 0x8B30: case 0x8DD9:
        case 0x8E88: case 0x8E87: case 0x91B9:
            *v = (GLint)PipeStage(it->second, q); break;
        case 0x8B83: *v = (GLint)it->second.validateStatus; break; // VALIDATE_STATUS
        case 0x8B84: *v = (GLint)it->second.infoLog.size() + 1; break; // INFO_LOG_LENGTH
        default: c.errors.Record(0x0500); break;
    }
}
void glGetProgramPipelineInfoLog(GLuint p, GLsizei n, GLsizei* l, GLchar* log) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(p);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); if (l) *l = 0; return; }
    GLsizei k = (n > 0) ? std::min<GLsizei>(n - 1, (GLsizei)it->second.infoLog.size()) : 0;
    if (log && k > 0) memcpy(log, it->second.infoLog.data(), (size_t)k);
    if (log && n > 0) log[k] = 0;
    if (l) *l = k;
}
// --- buffer textures: Metal buffer-backed texture (newTextureWithBuffer) ---
static void BindBufTex(GLuint tex, GLenum inf, GLuint buf) {
    Context& c = Context::Current();
    auto itt = c.textures.find(tex);
    auto itb = c.buffers.find(buf);
    if (itt == c.textures.end()) { c.errors.Record(0x0502); return; }
    itt->second.internalFormat = inf;
    if (itb == c.buffers.end()) return;
    Context::BufferView bv = c.SyncBuffer(buf); // shadow có thể rỗng (P1)
    if (!bv.p) { c.errors.Record(0x0502); return; }
    // CACHE: Minecraft gọi glTexBuffer MỖI FRAME. Trước đây mỗi lần đều
    // alloc tmp (n=710178 ≈ 2.8MB) + newTextureWithBytes lại → đo được
    // 2.1–6ms/call (gltop#2) + churn RAM/GPU texture (đóng góp jetsam).
    // Chỉ upload lại khi buffer / ifmt / contentGen đổi.
    {
        auto bit = c.buffers.find(buf);
        if (bit != c.buffers.end() && itt->second.gpu && !c.device->isNull() &&
            itt->second.buftexBuf == buf && itt->second.buftexGen == bit->second.contentGen)
            return;
    }
    // GPU: buffer texture → texture Nx1 để shader .read(index).
    // Bảng format thật (MC 26.1 dùng cả R8I — bug "sampdeny#... ifmt=0x8231"):
    //   0x8235 R32I(4B)  0x8236 R32UI(4B)  0x822E R32F(4B)
    //   0x8231 R8I(1B)   0x8232 R8UI(1B)
    //   0x8233 R16I(2B)  0x8234 R16UI(2B)  0x822D R16F(2B)
    // Format không có trong bảng: shadow only + log bufnop# (DiagOn).
    struct BufTexEnt { GLenum gl; size_t elem; metal::PixelFormat pf; };
    static const BufTexEnt kBufTex[] = {
        {0x8235, 4, metal::PixelFormat::R32Sint},
        {0x8236, 4, metal::PixelFormat::R32Uint},
        {0x822E, 4, metal::PixelFormat::R32Float},
        {0x8231, 1, metal::PixelFormat::R8Sint},
        {0x8232, 1, metal::PixelFormat::R8Uint},
        {0x8233, 2, metal::PixelFormat::R16Sint},
        {0x8234, 2, metal::PixelFormat::R16Uint},
        {0x822D, 2, metal::PixelFormat::R16Float},
    };
    const BufTexEnt* pick = nullptr;
    for (auto& e : kBufTex)
        if (e.gl == inf) { pick = &e; break; }
    if (pick) {
        size_t elem = pick->elem;
        metal::PixelFormat pf = pick->pf;
        size_t n = bv.size / elem;
        if (n == 0) n = 1;
        // Metal giới hạn width 16384 (A11 validate → abort). n=710178 từng crash:
        // `MTLTextureDescriptor has width (710178) greater than ... 16384`.
        // Đóng khung W=min(n,16384) hàng, H=ceil(n/W) — shader đọc theo
        // (i%W, i/W) (xem glsl_to_msl texelFetch buffer) nên index < n vẫn đúng.
        const size_t kMaxW = 16384;
        uint32_t W = (uint32_t)std::min<size_t>(n, kMaxW);
        uint32_t H = (uint32_t)((n + W - 1) / W);
        itt->second.w = W;
        itt->second.h = H;
        std::vector<uint8_t> tmp((size_t)W * H * elem, 0);
        memcpy(tmp.data(), bv.p, std::min(tmp.size(), bv.size));
        itt->second.gpu = c.device->newTextureWithBytes(
            W, H, pf, tmp.data(), (size_t)W * elem);
        if (!itt->second.gpu)
            c.LogDebug(0, 0, 0, 0, "glTexBuffer: GPU int texture fail, shadow only");
        // P1: bảng buffer texture lớn (n=710178 ≈ 2.8MB) từng nhân đôi CPU+GPU.
        // GPU texture là nguồn cho shader; shadow chỉ giữ khi Null / gpu tạo lỗi.
        if (c.device->isNull() || !itt->second.gpu)
            itt->second.pixels = std::move(tmp);
        else
            std::vector<uint8_t>().swap(itt->second.pixels);
        {
            auto bit = c.buffers.find(buf);
            if (bit != c.buffers.end()) {
                itt->second.buftexBuf = buf;
                itt->second.buftexGen = bit->second.contentGen;
            }
        }
        if (c.DiagOn()) {
            static std::set<uint64_t> seenBB;
            uint64_t key = ((uint64_t)tex << 20) ^ ((uint64_t)inf << 4) ^
                           (uint64_t)(n >> 10);
            if (seenBB.size() < 24 && seenBB.insert(key).second) {
                static int nBB = 0;
                fprintf(stderr,
                        "[TGLMT] bufbind#%d tex#%u ifmt=0x%04x bytes=%zu n=%zu %ux%u gpu=%d\n",
                        ++nBB, tex, inf, bv.size, n, W, H,
                        (int)(itt->second.gpu != nullptr));
                fflush(stderr);
            }
        }
    } else {
        // Format chưa có đường upload GPU → shadow LÀ nguồn duy nhất: phải giữ.
        itt->second.pixels.assign(bv.p, bv.p + bv.size);
        if (c.DiagOn()) {
            static int nBN = 0;
            if (++nBN <= 8)
                fprintf(stderr, "[TGLMT] bufnop#%d ifmt=0x%04x (buffer texture format chưa hỗ trợ, shadow only)\n",
                        nBN, inf);
            fflush(stderr);
        }
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
        Context::BufferView bv = c.SyncBuffer(b); // shadow có thể rỗng (P1)
        if (!bv.p) { c.errors.Record(0x0502); return; }
        if ((size_t)o > bv.size) { c.errors.Record(0x0501); return; }
        size_t len = s ? std::min((size_t)s, bv.size - (size_t)o)
                       : bv.size - (size_t)o;
        // bind bản slice qua buffer tạm nội bộ để tái dùng BindBufTex
        GLuint tmp = 0;
        c.registry.Create(ObjectKind::Buffer, 1, &tmp);
        c.buffers[tmp].data.assign(bv.p + o, bv.p + o + len);
        c.buffers[tmp].size = len;
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
// glTextureView đã chuyển sang gl_texture.cpp (P10 ARB_texture_view, tạo Metal
// texture view thật với mip/slice subset thay vì copy nguyên object). Xoá stub ở đây.
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
