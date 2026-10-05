// gl_apple_draw.cpp — M5b: GL dispatch lái GPU Metal thật (chỉ Apple backend).
// Mỗi glDraw* vừa ghi trace (giữ nguyên hành vi Null/tests) vừa encode thật khi đủ
// điều kiện; thiếu điều kiện → trace-only + debug log (trung thực, không crash).
// Hỗ trợ: VAO→descriptor, program đã link (converter MSL), uniforms gộp buffer(16),
// sampler/texture theo unit, FBO (0=default target, khác=wrap texture), depth,
// blend0, cull, fillMode LINE, scissor, viewport convert, baseVertex, instancing.
// Chưa (trace-only + log): PATCHES-tess/GS, stencil, baseInstance!=0, indirect GPU.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLConvert.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace tglmt {

// kind sampler → target GL tương ứng: resolve binding THEO TARGET (per-target
// slots), không để cube/buffer cùng unit clobber 2D (bug shadow/GUI sample sai).
static GLenum KindTarget(char kind) {
    switch (kind) {
        case 'C': return 0x8513; // TEXTURE_CUBE_MAP
        case 'B': return 0x8C2A; // TEXTURE_BUFFER
        case 'A': return 0x8C1A; // TEXTURE_2D_ARRAY
        default: return 0x0DE1;  // TEXTURE_2D (+shadow 'S' approx 2D)
    }
}
static char KindOf(const std::unordered_map<std::string, char>& kinds, const std::string& n) {
    auto it = kinds.find(n);
    return it == kinds.end() ? '2' : it->second;
}

// Deferred full ring upload: lấy (buf,off) từ Context ring, memcpy + didModify.
// 0 MTLBuffer alloc trong frame. Fallback newBuffer khi ring đầy/Null + đếm tempAllocs.
static size_t RingUpload(Context& c, const void* data, size_t n,
                         metal::IBuffer*& bufOut,
                         std::shared_ptr<metal::IBuffer>& keepOut) {
    bufOut = nullptr;
    if (!data) { data = ""; }
    if (!n) n = 1;
    auto [rb, off] = c.RingAlloc(n, 256);
    if (rb) {
        memcpy((uint8_t*)rb->contents() + off, data, n);
        rb->didModifyRange(off, n);
        bufOut = rb;
        keepOut.reset(); // ring sống theo Context (3 frames), không cần pendingKeep
        return off;
    }
    ++c.appleStats.tempAllocs;
    keepOut = c.device->newBufferWithBytes(data, n, metal::StorageMode::Shared);
    if (!keepOut) return 0;
    c.pendingKeep.push_back(keepOut);
    bufOut = keepOut.get();
    return 0;
}

static std::shared_ptr<metal::IBuffer> TempUpload(Context& c, const void* data, size_t n) {
    // Legacy path (chỉ còn cho fallback ngoài draw-hot): vẫn newBuffer + đếm.
    ++c.appleStats.tempAllocs;
    return c.device->newBufferWithBytes(data, n ? n : 1, metal::StorageMode::Shared);
}

// Fallback chống GPU fault trên A11 (status=5 SubmissionsIgnored đã quan sát):
// argument texture/buffer KHÔNG BAO GIỜ được để trống — GL quy định incomplete
// texture = (0,0,0,1), UBO unbound = undefined (ta chọn 0, an toàn hơn fault).
// Cache theo device (thread_local như SamplerForUnit).
static std::shared_ptr<metal::ITexture> FallbackBlackTex(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::ITexture>> cache;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    uint8_t black[4] = {0, 0, 0, 255};
    auto t = c.device->newTextureWithBytes(1, 1, metal::PixelFormat::RGBA8Unorm, black, 4);
    if (t) cache[c.device.get()] = t;
    return t;
}
static std::shared_ptr<metal::ITexture> FallbackBlackCube(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::ITexture>> cache;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    auto t = c.device->newCubeTexture(1, metal::PixelFormat::RGBA8Unorm);
    if (t) {
        uint8_t black[4] = {0, 0, 0, 255};
        for (uint32_t f = 0; f < 6; ++f)
            c.device->updateCubeFace(t.get(), f, black, 4);
        cache[c.device.get()] = t;
    }
    return t;
}
static std::shared_ptr<metal::IBuffer> FallbackZeroBuf(Context& c) {
    static thread_local std::map<metal::IDevice*, std::shared_ptr<metal::IBuffer>> cache;
    auto it = cache.find(c.device.get());
    if (it != cache.end() && it->second) return it->second;
    uint8_t z[256] = {0};
    auto b = c.device->newBufferWithBytes(z, sizeof(z), metal::StorageMode::Shared);
    if (b) cache[c.device.get()] = b;
    return b;
}

// Sampler state cho 1 unit: SamplerObject đã bind, else dựng từ TextureObject.params
// (glTexParameter), else default LINEAR/REPEAT. Cache theo khóa đơn giản.
static std::shared_ptr<metal::ISamplerState> SamplerForUnit(Context& c, GLuint unit,
                                                            const TextureObject& tex) {
    static thread_local std::map<uint64_t, std::shared_ptr<metal::ISamplerState>> cache;
    GLuint sid = c.state.BoundSampler(unit);
    uint32_t minF = 0x2601, magF = 0x2601, sW = 0x2901, tW = 0x2901;
    float aniso = 1.0f, lodMin = 0.0f, lodMax = 1000.0f;
    auto fromParams = [&](const std::unordered_map<GLenum, GLint>& p) {
        auto g = [&](GLenum k, uint32_t d) {
            auto it = p.find(k);
            return it == p.end() ? d : (uint32_t)it->second;
        };
        minF = g(0x2801, minF); magF = g(0x2800, magF);
        sW = g(0x2802, sW); tW = g(0x2803, tW);
    };
    // LOD clamp: GL_TEXTURE_MIN_LOD/MAX_LOD (sampler) + GL_TEXTURE_BASE_LEVEL/
    // MAX_LEVEL (texture). Minecraft set MAX_LOD=0 cho sampler không mipmap và
    // BASE/MAX_LEVEL=0 cho texture ở MỌI draw → phải tôn trọng, nếu không Metal
    // tự clamp 0..1000 và đọc ngoài atlas.
    auto lodFrom = [](const std::unordered_map<GLenum, GLint>& p, float def) {
        auto it = p.find(0x813B); // GL_TEXTURE_MAX_LOD
        return (it != p.end()) ? (float)it->second : def;
    };
    uint64_t key = 0;
    auto sit = c.samplers.find(sid);
    if (sid && sit != c.samplers.end()) {
        fromParams(sit->second.iparams);
        auto ff = sit->second.fparams.find(0x84FE); // TEXTURE_MAX_ANISOTROPY
        if (ff != sit->second.fparams.end()) aniso = ff->second;
        lodMax = lodFrom(sit->second.iparams, 1000.0f);
        // Key PHẢI chứa toàn bộ params (min/mag/wrap/aniso/lod): game đổi filter
        // runtime (video settings Filtering/anisotropy, resource reload) qua
        // glSamplerParameteri trên CÙNG sampler id. Key sid-only trả sampler
        // Metal CŨ (sai filter/wrap → shimmer/đen viền) mà không recompile.
        uint64_t h = 1469598103934665603ull; // FNV-1a 64
        auto mix = [&](uint64_t v) { h ^= v; h *= 1099511628211ull; };
        mix(minF); mix(magF); mix(sW); mix(tW);
        {
            uint32_t ab;
            static_assert(sizeof(ab) == sizeof(aniso), "float bits");
            memcpy(&ab, &aniso, sizeof(ab));
            mix(ab);
        }
        { uint32_t lb; memcpy(&lb, &lodMax, 4); mix(lb); }
        key = h ^ ((uint64_t)sid * 0x9E3779B97F4A7C15ull); // sid + params
    } else {
        fromParams(tex.params);
        lodMax = lodFrom(tex.params, 1000.0f);
        auto bl = tex.params.find(0x813C); // GL_TEXTURE_BASE_LEVEL
        auto xl = tex.params.find(0x813D); // GL_TEXTURE_MAX_LEVEL
        if (bl != tex.params.end()) lodMin = (float)bl->second;
        if (xl != tex.params.end()) lodMax = (float)xl->second;
        key = ((uint64_t)tex.id << 32) | 0x54455800u; // 'TEX\0'
        key ^= ((uint64_t)minF << 48) ^ ((uint64_t)magF << 52);
        key ^= ((uint64_t)(sW & 0xF) << 56) ^ ((uint64_t)(tW & 0xF) << 60);
        { uint32_t a1, a2; memcpy(&a1, &lodMin, 4); memcpy(&a2, &lodMax, 4);
          key ^= ((uint64_t)a1 << 8) ^ ((uint64_t)a2 << 20); }
    }
    // Texture 1 level + minfilter mipmap (logo/widgets/sprite/blur-src UI):
    // tắt lọc mip để A11 không fetch LOD>0 (fault SubmissionsIgnored/đen).
    // Sampler cache phải phân biệt (cùng texture, khác noMip).
    bool singleLevel = !tex.gpu || tex.gpu->levelCount() <= 1;
    bool mipMin = (minF == 0x2700 || minF == 0x2701 || minF == 0x2702 || minF == 0x2703);
    bool noMip = singleLevel;
    if (noMip) key ^= (uint64_t)1 << 62;
    if (noMip && mipMin) {
        ++c.appleStats.mipBase; // đếm để frame log chứng minh fix có chạy trên máy
        static std::set<uint32_t> loggedMip;
        if (loggedMip.size() < 16 && loggedMip.insert(tex.id).second) {
            char b[128];
            snprintf(b, sizeof(b), "AppleDrawGL: mip->base tex#%u min=0x%x (1 level)",
                     tex.id, minF);
            c.LogDebug(0, 0, 0, 0, b);
        }
    }
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    metal::SamplerDesc d;
    d.minFilter = minF; d.magFilter = magF; d.sWrap = sW; d.tWrap = tW; d.maxAniso = aniso;
    d.noMip = noMip;
    d.lodMin = lodMin; d.lodMax = lodMax;
    auto s = c.device->makeSampler(d);
    if (s) cache[key] = s;
    return s;
}

// Clear đã xảy ra ngay (immediate) → dọn deferred còn treo: draw kế tiếp không
// được clear thêm lần nữa với mask/color cũ (deferred chỉ dành cho fallback Null).
static inline void ConsumeDeferredClear(Context& c) {
    c.applePendingClear = false;
    c.appleClearMask = 0;
}

// glClear NGAY TRÊN FBO ĐANG BIND (đúng GL: clear không phụ thuộc draw tiêu thụ).
// Thay thế cơ chế deferred cũ (applePendingClear): deferred bị ghi đè khi 2 glClear
// liên tiếp (clear đầu mất) và bị áp lên draw FBO khác (clrmiss) → sky transparent,
// sprite/item atlas bị wipe, blur source rỗng.
// Trả false → caller giữ deferred (Null backend / chưa có target / encoder fail).
bool AppleClearNow(GLbitfield m) {
    Context& c = Context::Current();
    if (!m || !c.device || c.device->isNull()) return false;
    GLuint fbo = c.state.BoundDrawFBO();
    // Resolve target (mượn đúng logic AppleDrawGL): FBO 0 → default, khác → wrap color+depth.
    std::shared_ptr<metal::IRenderTarget> target;
    bool hasDepthAny = false;
    if (fbo == 0) {
        target = c.device->defaultRenderTarget();
    } else {
        auto fit = c.fbos.find(fbo);
        if (fit == c.fbos.end()) return false;
        auto cit = fit->second.colorTex.find(0);
        auto tit = cit != fit->second.colorTex.end() ? c.textures.find(cit->second)
                                                     : c.textures.end();
        metal::ITexture* col = (tit != c.textures.end() && tit->second.gpu) ? tit->second.gpu.get()
                                                                           : nullptr;
        // Như AppleDrawGL: clear vào mip view level>0 của texture 1-level là
        // no-op (tránh clear NHẦM level 0 = wipe atlas). Tiêu thụ deferred.
        if (col) {
            auto lit = fit->second.colorLevel.find(0);
            GLint fboLevel = (lit == fit->second.colorLevel.end()) ? 0 : lit->second;
            if (fboLevel > 0 && tit->second.gpu->levelCount() <= (uint32_t)fboLevel) {
                ++c.appleStats.mipLevelSkipped;
                ConsumeDeferredClear(c);
                return true;
            }
        }
        metal::ITexture* dep = nullptr;
        if (fit->second.depthTex) {
            auto dit = c.textures.find(fit->second.depthTex);
            if (dit != c.textures.end() && dit->second.gpu) dep = dit->second.gpu.get();
        }
        if (!col && !dep) return false;
        hasDepthAny = (dep != nullptr);
        target = c.WrapTarget(fbo, col, dep);
        if (!target) return false;
    }
    if (fbo == 0 && target) hasDepthAny = target->hasDepth();
    if (!target) return false;
    metal::LoadOp cl = (m & 0x00004000u) ? metal::LoadOp::Clear : metal::LoadOp::Load;
    metal::LoadOp dl = (m & 0x00000100u) ? metal::LoadOp::Clear : metal::LoadOp::Load;
    // Scissor: GL clear tôn trọng SCISSOR_TEST (GuiItemAtlas clear vùng slot).
    // Metal loadAction=Clear KHÔNG clip theo setScissorRect (đã kiểm chứng
    // integration test) → color clear theo vùng qua fillRegionColor (blit);
    // depth clear full (vô hại: mỗi slot clear ngay trước draw của nó).
    // fill fail (MSAA/sRGB/format lạ) → fallback clear toàn attachment + diag.
    metal::ScissorRect sc{};
    const metal::ScissorRect* scp = nullptr;
    if (c.state.IsEnabled(0x0C11)) {
        auto s = c.state.GetScissor();
        bool upper = c.state.ClipOrigin() == 0x8CA2;
        sc = GLScissorToMetal(s.x, s.y, s.w, s.h, (int)target->height(), upper);
        if (sc.x >= target->width() || sc.y >= target->height()) { ConsumeDeferredClear(c); return true; }
        if (sc.x + sc.w > target->width()) sc.w = target->width() - sc.x;
        if (sc.y + sc.h > target->height()) sc.h = target->height() - sc.y;
        if (sc.w == 0 || sc.h == 0) { ConsumeDeferredClear(c); return true; }
        scp = &sc;
    }
    // Thứ tự Metal: upload staged phải nằm TRƯỚC clear (queue order = commit order).
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    if (c.pendingEncoder) c.FlushPendingEncoder(); // draw trước end trước clear
    metal::LoadOp clEff = cl, dlEff = dl;
    if (scp && cl == metal::LoadOp::Clear) {
        metal::ClearColor cc{(double)c.clearColor[0], (double)c.clearColor[1],
                             (double)c.clearColor[2], (double)c.clearColor[3]};
        // Region clear + depth clear gộp 1 command buffer (GuiItemAtlas ~300
        // lần/frame — 2 CB/ô là quá nhiều command buffer).
        const bool wantDepth = (dl == metal::LoadOp::Clear) && hasDepthAny;
        bool done = wantDepth ? c.device->fillRegionColorAndDepth(
                                    target.get(), scp->x, scp->y, scp->w, scp->h, cc,
                                    c.clearDepth)
                              : c.device->fillRegionColor(target.get(), scp->x, scp->y,
                                                          scp->w, scp->h, cc);
        if (done) {
            clEff = metal::LoadOp::Load; // color đã ghi đúng vùng
            if (wantDepth) dlEff = metal::LoadOp::Load;
        } else {
            // KHÔNG bao giờ fallback sang clear TOÀN attachment: GuiItemAtlas clear
            // 1 ô/item/frame vào CÙNG 1 texture — clear cả texture xoá sạch các
            // ô đã bake trước đó ⇒ "túi đồ mất texture, chỉ vài cái còn". Bỏ clear
            // màu (ô sắp được vẽ đè) an toàn hơn nhiều so với mất cả atlas.
            ++c.appleStats.clearRegionFails;
            clEff = metal::LoadOp::Load;
            static uint64_t nFb = 0;
            if (++nFb <= 8)
                fprintf(stderr,
                        "[TGLMT] fillregionfail#%llu fbo=%u (%ux%u @ %u,%u) → giữ nội dung, "
                        "KHÔNG clear cả atlas\n",
                        (unsigned long long)nFb, (uint32_t)fbo, scp->w, scp->h, scp->x, scp->y);
        }
    }
    if (clEff == metal::LoadOp::Load && dlEff == metal::LoadOp::Load) {
        ConsumeDeferredClear(c); // color scissor fill xong + không depth → xong
        return true;
    }
    auto enc = c.device->makeClearEncoder(target.get(),
            metal::ClearColor{(double)c.clearColor[0], (double)c.clearColor[1],
                              (double)c.clearColor[2], (double)c.clearColor[3]},
            c.clearDepth, clEff, dlEff, nullptr);
    if (!enc) {
        if (c.DiagOn()) {
            static int nClrNil = 0;
            if (++nClrNil <= 8)
                fprintf(stderr, "[TGLMT] encnilclr# fbo=%u cl=%d dl=%d\n",
                        (unsigned)fbo, (int)clEff, (int)dlEff);
        }
        return false;
    }
    (void)enc->endAndCommitNoWait(); // clear-only: end ngay, KHÔNG để pending
    // Clear đã xảy ra NGAY → bỏ deferred còn treo (Renderer wildcard / fallback
    // cũ) để draw kế tiếp không clear thêm lần nữa với mask/color cũ.
    ConsumeDeferredClear(c);
    return true;
}

// Thực thi 1 draw GL trên Apple backend. Trả true nếu đã encode thật
// (trace vẫn do EmitDraw ghi riêng).
// first: vertexStart cho non-indexed (chunk mesh first!=0 nhiều).
bool AppleDrawGL(GLenum mode, GLsizei count, GLenum indexType, const void* indexData,
                 bool indexed, size_t indexByteOff, GLuint eboId, GLsizei inst,
                 GLint baseVertex, GLuint baseInstance, GLint first) {
    Context& c = Context::Current();
    if (c.device->isNull()) return false;
    if (count <= 0) return true; // no-op đã lọc ở EmitDraw, giữ an toàn
    c.appleStats.drawsAttempted++;
    if (baseInstance != 0) {
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: baseInstance trace-only (Metal non-indirect)");
        return false;
    }
    // 1. Program đã link + Apple libs
    GLuint prog = c.state.BoundProgram();
    auto pit = c.programs.find(prog);
    if (pit == c.programs.end() || !pit->second.linked) { c.appleStats.noProgram++; return false; }
    ProgramObject& pr = pit->second;
    if (pr.hasTessStages || pr.hasGeometryStage) {
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: tess/GS trace-only (M5b-future)");
        return false;
    }
    if (!pr.appleVS || !pr.appleFS) { c.appleStats.noProgram++; return false; } // vertex-only/Null-link
    if (mode == 0x000E /*PATCHES*/) return false;
    if (baseVertex != 0 && !indexed) return false;
    // FAN/LOOP expand CPU cho GPU (Metal không có FAN, LOOP hở 1 cạnh).
    // Non-indexed FAN/LOOP chuyển thành indexed tam giác/strip khép kín qua index tạm.
    // Indexed FAN chuyển thành triangle list, indexed LOOP thêm index đầu vào cuối.
    std::vector<uint8_t> fanLoopExpand; // giữ sống suốt draw
    GLenum effMode = mode;
    GLsizei effCount = count;
    bool effIndexed = indexed;
    GLenum effIndexType = indexType;
    const void* effIndexData = indexData;
    size_t effIndexOff = indexByteOff;
    GLuint effEbo = eboId;
    GLint effFirst = first;
    if (mode == 0x0006 /*FAN*/) {
        if (count < 3) return true; // fan <3 đỉnh = no-op
        if (indexed) {
            // đọc indices gốc (U16/U32, UBYTE đã expand ở EmitDraw) rồi triangulate (0,i,i+1)
            size_t elem = (indexType == 0x1403) ? 2 : 4;
            const uint8_t* src = nullptr;
            std::vector<uint8_t> tmp;
            auto bit = c.buffers.find(eboId);
            if (bit != c.buffers.end() && indexByteOff + (size_t)count * elem <= bit->second.data.size())
                src = bit->second.data.data() + indexByteOff;
            else if (indexData && eboId == 0) src = (const uint8_t*)indexData;
            if (!src) return false;
            size_t triCount = (size_t)(count - 2) * 3;
            fanLoopExpand.resize(triCount * elem);
            for (GLsizei i = 0; i < count - 2; ++i) {
                for (int k = 0; k < 3; ++k) {
                    GLsizei srcIdx = (k == 0) ? 0 : (k == 1 ? i + 1 : i + 2);
                    memcpy(fanLoopExpand.data() + ((size_t)i * 3 + k) * elem, src + (size_t)srcIdx * elem, elem);
                }
            }
            effMode = 0x0004; effCount = (GLsizei)triCount; effIndexed = true;
            effIndexData = fanLoopExpand.data(); effIndexOff = 0; effEbo = 0;
            if (baseVertex != 0) return false; // fan+baseVertex: EmitDraw chưa rewrite expand → trace-only an toàn
        } else {
            // non-indexed fan (first, count): indices first, first+i+1, first+i+2
            size_t triCount = (size_t)(count - 2) * 3;
            fanLoopExpand.resize(triCount * 4);
            uint32_t* dst = (uint32_t*)fanLoopExpand.data();
            for (GLsizei i = 0; i < count - 2; ++i) {
                dst[i * 3] = (uint32_t)(first);
                dst[i * 3 + 1] = (uint32_t)(first + i + 1);
                dst[i * 3 + 2] = (uint32_t)(first + i + 2);
            }
            effMode = 0x0004; effCount = (GLsizei)triCount; effIndexed = true;
            effIndexType = 0x1405; effIndexData = fanLoopExpand.data(); effIndexOff = 0; effEbo = 0;
            effFirst = 0;
        }
    } else if (mode == 0x0003 /*LINE_LOOP*/) {
        if (indexed) {
            size_t elem = (indexType == 0x1403) ? 2 : 4;
            const uint8_t* src = nullptr;
            auto bit = c.buffers.find(eboId);
            if (bit != c.buffers.end() && indexByteOff + (size_t)count * elem <= bit->second.data.size())
                src = bit->second.data.data() + indexByteOff;
            else if (indexData && eboId == 0) src = (const uint8_t*)indexData;
            if (!src) return false;
            fanLoopExpand.resize(((size_t)count + 1) * elem);
            memcpy(fanLoopExpand.data(), src, (size_t)count * elem);
            memcpy(fanLoopExpand.data() + (size_t)count * elem, src, elem); // khép đỉnh đầu
            effMode = 0x0002; effCount = count + 1; effIndexed = true;
            effIndexData = fanLoopExpand.data(); effIndexOff = 0; effEbo = 0;
            if (baseVertex != 0) return false;
        } else {
            // non-indexed loop: indices first..first+count-1 + first
            fanLoopExpand.resize(((size_t)count + 1) * 4);
            uint32_t* dst = (uint32_t*)fanLoopExpand.data();
            for (GLsizei i = 0; i < count; ++i) dst[i] = (uint32_t)(first + i);
            dst[count] = (uint32_t)first;
            effMode = 0x0002; effCount = count + 1; effIndexed = true;
            effIndexType = 0x1405; effIndexData = fanLoopExpand.data(); effIndexOff = 0; effEbo = 0;
            effFirst = 0;
        }
    }
    // dùng eff* từ đây trở đi
    mode = effMode; count = effCount; indexed = effIndexed;
    indexType = effIndexType; indexData = effIndexData; indexByteOff = effIndexOff;
    eboId = effEbo; first = effFirst;
    metal::PrimitiveType prim;
    switch (mode) {
        case 0x0000: prim = metal::PrimitiveType::Point; break;
        case 0x0001: prim = metal::PrimitiveType::Line; break;
        case 0x0002: prim = metal::PrimitiveType::LineStrip; break;
        case 0x0004: prim = metal::PrimitiveType::Triangle; break;
        case 0x0005: prim = metal::PrimitiveType::TriangleStrip; break;
        default: return false; // FAN đã expand ở trên, còn lại trace-only
    }
    // FRONT_AND_BACK cull = bỏ draw (đúng GL: không primitive nào qua)
    if (c.state.IsEnabled(0x0B44) && c.state.CullMode() == 0x0408) return true; // đã "xử lý"
    // 2. VAO → descriptor + buffers
    GLuint vao = c.state.BoundVAO();
    auto vaoIt = c.vaos.find(vao);
    if (vaoIt == c.vaos.end()) { c.appleStats.miscFail++; return false; } // VAO 0
    VertexArrayObject& v = vaoIt->second;
    // 2a. PHẢI flush staging TRƯỚC khi chụp con trỏ buffer của draw này.
    // glBufferSubData/glNamedBufferSubData chỉ ghi shadow + stage; GPU copy dồn
    // tới FlushBufferStaging, mà hàm đó CÓ THỂ đổi identity BufferObject::gpu
    // (xoay sang pool slot khi GPU còn đọc buffer cũ). Trước đây flush nằm
    // SAU vòng lặy attrib ⇒ bindMap giữ pointer buffer CŨ trong khi
    // setVertexBuffer bind buffer MỚI (và ngược lại ở index buffer) ⇒ draw N+1
    // vẽ geometry của draw N với index/texture/matrix của draw N+1. Đó chính
    // là lỗi "model chồng lên nhau / có mảnh model gốc dưới chân / nhân vật
    // đứng lại ở vị trí cũ rồi mới bay tới" (Minecraft tái dùng 1 vertex buffer
    // cho MỌI batch cùng VertexFormat nên lỗi này xảy ra mỗi batch).
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    auto GLTypeSize = [](GLenum t) -> uint32_t {
        switch (t) {
            case 0x1400: case 0x1401: return 1; // BYTE/UBYTE
            case 0x1402: case 0x1403: case 0x140B: return 2; // SHORT/USHORT/HALF
            case 0x1404: case 0x1405: case 0x1406: return 4; // INT/UINT/FLOAT
            case 0x140A: case 0x140C: return 8; // DOUBLE (không render được, pipeline sẽ nil)
            case 0x8368: case 0x8DC6: case 0x8C3B: return 4; // packed 10_10_10 / 11_11_10
            default: return 4;
        }
    };
    // Mảng cố định 16 attrib: tránh cấp phát vector mỗi draw (hàng trăm draw/
    // frame × malloc/free + toàn bộ phần tử chép lại khi PipelineKey so sánh).
    metal::CustomAttrib cas[16];
    uint32_t casN = 0;
    // GL buffer id + offset (KHÔNG giữ shared_ptr: identity có thể đổi sau khi
    // flush — xem 2a). Giải ra shared_ptr ngay trước khi encode.
    struct BindSlot { GLuint id = 0; size_t off = 0; };
    BindSlot bindMap[16];
    uint32_t bindMask = 0;
    for (int i = 0; i < 16; ++i) {
        const VertexAttrib& a = v.attribs[i];
        if (!a.enabled) continue;
        auto bit = c.buffers.find(a.buffer);
        if (bit == c.buffers.end() || !bit->second.gpu) return false;
        // Địa chỉ effective = bindings[binding].offset (base) + relativeOffset
        // (descriptor) — spec §10.3.1. Trước đây dùng chung a.offset cho cả hai
        // nên base+relative bị nhân đôi / relative bị mất (đen màn hình khi game
        // dùng Separate path: Format/Binding trước, BindVertexBuffer sau).
        size_t base = 0;
        GLuint effDivisor = a.divisor;
        if (a.binding < v.bindings.size()) {
            GLintptr bo = v.bindings[a.binding].offset;
            base = bo > 0 ? (size_t)bo : 0;
            if (effDivisor == 0) effDivisor = v.bindings[a.binding].divisor;
        }
        metal::CustomAttrib ca;
        ca.loc = (uint32_t)i; ca.size = (uint32_t)a.size; ca.type = a.type;
        ca.normalized = a.normalized ? true : false;
        ca.offset = (uint32_t)a.relativeOffset;
        ca.bufferIndex = a.binding;
        ca.stride = (uint32_t)a.stride;
        ca.divisor = effDivisor;
        if (casN < 16) cas[casN++] = ca;
        if (a.binding < 16 && !(bindMask & (1u << a.binding))) {
            bindMask |= (1u << a.binding);
            bindMap[a.binding].id = a.buffer;
            bindMap[a.binding].off = base;
        }
    }
    if (casN == 0) {
        // Draw không attribute (screenquad suy từ vertex_id): pipeline descriptor
        // rỗng, không bind vertex buffer. Không phải lỗi (trước đây noPipeline oan).
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: attributeless draw (vertex_id)");
    }
    for (uint32_t ai2 = 0; ai2 < casN; ++ai2) {
        metal::CustomAttrib& ca = cas[ai2];
        // stride 0 = tightly packed theo chính attribute (spec §10.3.1), không
        // cộng offset (bug cũ cộng relative vào stride làm đỉnh thưa sai).
        if (ca.stride == 0) ca.stride = ca.size * GLTypeSize(ca.type);
    }
    // Validator đọc vượt buffer: rẻ (O(attribs) phép tính, không scan vertices).
    // Giữ luôn-on để test_mc_validate + chẩn đoán A11 fault, không gate DIAG.
    if (!indexed) {
        static std::set<std::pair<GLuint, int>> warnedRange;
        for (uint32_t ai2 = 0; ai2 < casN; ++ai2) {
            const metal::CustomAttrib& ca = cas[ai2];
            if (ca.bufferIndex >= 16 || !(bindMask & (1u << ca.bufferIndex))) continue;
            auto bit = c.buffers.find(bindMap[ca.bufferIndex].id);
            if (bit == c.buffers.end() || !bit->second.gpu) continue;
            size_t gpuLen = bit->second.gpu->length();
            size_t elemBytes = (size_t)ca.size * GLTypeSize(ca.type);
            size_t lastN = ca.divisor > 0 ? (inst > 0 ? (size_t)inst - 1 : 0)
                                          : (size_t)first + (size_t)(count > 0 ? count - 1 : 0);
            size_t fetchEnd = bindMap[ca.bufferIndex].off + ca.offset + lastN * ca.stride + elemBytes;
            if (fetchEnd > gpuLen && warnedRange.size() < 16 &&
                warnedRange.insert({prog, (int)ca.loc}).second) {
                ++c.appleStats.rangeWarn;
                char b[192];
                snprintf(b, sizeof(b),
                         "AppleDrawGL: RANGE prog@%u slot%d fetchEnd=%zu > bufLen=%zu "
                         "(first=%d count=%d inst=%d)",
                         prog, (int)ca.loc, fetchEnd, gpuLen, first, count, inst);
                c.LogDebug(0, 0, 0, 0, b);
            }
        }
    }
    // 3. Target: FBO 0 → default; khác → wrap colorTex[0] (+depth nếu có)
    std::shared_ptr<metal::IRenderTarget> target;
    bool hasDepthTex = false;
    GLuint drawColorTexId = 0; // texture đích (so hazard feedback ở dưới)
    if (c.state.BoundDrawFBO() == 0) {
        target = c.device->defaultRenderTarget();
        if (!target) { c.appleStats.noTarget++; return false; } // app chưa đặt target
    } else {
        auto fit = c.fbos.find(c.state.BoundDrawFBO());
        if (fit == c.fbos.end()) return false;
        auto cit = fit->second.colorTex.find(0);
        if (cit == fit->second.colorTex.end()) return false;
        drawColorTexId = cit->second;
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end() || !tit->second.gpu) return false;
        // Mip view (animate bake per-mip của TextureAtlas 26.x): FBO attach
        // level>0 nhưng texture Metal chỉ có 1 level (non-mipmapped). Vẽ vào
        // đây = alias level 0 → smear atlas (bake mip-N sai vị trí/scale đè
        // lên base). Level 0 đã bake đúng + sampler NotMipmapped không bao giờ
        // đọc mip>0 → BỎ QUA an toàn (đúng pixels + đỡ GPU). Khi nào texture
        // thật sự mipmapped thì vẽ bình thường.
        {
            auto lit = fit->second.colorLevel.find(0);
            GLint fboLevel = (lit == fit->second.colorLevel.end()) ? 0 : lit->second;
            if (fboLevel > 0 && tit->second.gpu->levelCount() <= (uint32_t)fboLevel) {
                ++c.appleStats.mipLevelSkipped;
                return true; // đã "xử lý": no-op đúng (không error, không đen)
            }
        }
        metal::ITexture* dep = nullptr;
        if (fit->second.depthTex) {
            auto dit = c.textures.find(fit->second.depthTex);
            if (dit != c.textures.end() && dit->second.gpu) {
                dep = dit->second.gpu.get();
                hasDepthTex = true;
            }
        }
        target = c.WrapTarget(c.state.BoundDrawFBO(), tit->second.gpu.get(), dep);
        if (!target) { c.appleStats.noTarget++; return false; }
        // fixico8: log 1 lần/FBO format color+depth THẬT (depth RGBA8 = sai với
        // pipeline Depth32Float → encoder nil → icon bake bị bỏ).
        if (c.DiagOn()) {
            static std::set<GLuint> seenTgt;
            if (seenTgt.insert(c.state.BoundDrawFBO()).second)
                fprintf(stderr,
                        "[TGLMT] tgt# fbo=%u colorTex=%u depthTex=%u hasDepth=%d "
                        "colorFmt=%u depthFmt=%u\n",
                        (unsigned)c.state.BoundDrawFBO(), (unsigned)drawColorTexId,
                        (unsigned)fit->second.depthTex, (int)hasDepthTex,
                        (unsigned)tit->second.gpu->pixelFormat(),
                        dep ? (unsigned)dep->pixelFormat() : 0u);
        }
    }
    // 3b. Staging đã flush ở 2a (TRƯỚC khi chụp con trỏ buffer) — cố ý bỏ
    // FlushAllBufferStaging ở đây: gọi lại sau khi đã chụp bindMap là chính là
    // nguồn gốc lỗi vertex buffer một-batch-trễ.
    // 3c. Feedback hazard pre-scan (TBDR đúng): nếu draw này sample đúng texture
    // đang render (drawColorTexId), split pass trước khi reuse encoder.
    // Apple Best Practices: sampling-dependency giữa 2 encoders cùng target thì
    // KHÔNG merge được. Flush ở đây để draws trước commit với Store, draw này
    // mở pass mới với Load (thấy dữ liệu cũ, không fault).
    if (drawColorTexId != 0 && c.pendingEncoder) {
        bool hz = false;
        for (auto& sn : pr.vsSamplers) {
            auto uit = pr.samplerUnits.find(sn);
            GLuint u = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
            if (u < 32 && c.state.AnyBoundAtUnit(u, drawColorTexId)) { hz = true; break; }
        }
        if (!hz) {
            for (auto& sn : pr.fsSamplers) {
                auto uit = pr.samplerUnits.find(sn);
                GLuint u = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
                if (u < 32 && c.state.AnyBoundAtUnit(u, drawColorTexId)) { hz = true; break; }
            }
        }
        if (hz) {
            c.FlushPendingEncoder();
            ++c.appleStats.hazardSplits;
        }
    }
    // 4. Pipeline: depth khi depthTest bật (+ có depth thật), blend khi BLEND bật
    metal::PipelineOpts opts;
    opts.depth = c.state.IsEnabled(0x0B71) && (c.state.BoundDrawFBO() == 0 ? false : hasDepthTex);
    opts.blend = c.state.IsEnabled(0x0BE2);
    opts.colorWriteMask = c.state.ColorMask(0);
    if (opts.blend) {
        const BlendState& b = c.state.Blend()[0];
        opts.blend0.enabled = true;
        opts.blend0.srcRGB = b.srcRGB; opts.blend0.dstRGB = b.dstRGB;
        opts.blend0.srcAlpha = b.srcAlpha; opts.blend0.dstAlpha = b.dstAlpha;
        opts.blend0.rgbOp = b.rgbEq; opts.blend0.alphaOp = b.alphaEq;
    }
    // IR: identity của render pass đang muốn (để quyết định reuse encoder).
    // Default FB: colorTex=0; FBO: colorTex id thật. hasDepth phân biệt pass có depth.
    GLuint curDrawFBO = c.state.BoundDrawFBO();
    GLuint curColorTex = drawColorTexId; // 0 cho default
    // Thử reuse pipeline đã có khi cùng prog+target format (tránh rebuild key string
    // + mutex mỗi draw). So sánh nativeHandle sau khi lookup; lookup vẫn gọi vì
    // bridge đã cache (rẻ hơn compile). Bước tiếp theo (M5c) sẽ cache key ở Context
    // để bỏ cả lookup khi inputs giống hệt.
    // NOTE: target cho pipeline key phải là target sẽ dùng (pendingTarget khi reuse,
    // target mới khi tạo mới). Khi reuse, pendingTarget và target mới cùng format/size
    // (đã kiểm tra FBO id + hasDepth), nên dùng format của pendingTarget cũng đúng.
    metal::PixelFormat pipeFmt = target->pixelFormat();
    if (c.pendingEncoder && c.pendingTarget && !c.applePendingClear &&
        c.pendingDrawFBO == curDrawFBO && c.pendingColorTex == curColorTex &&
        c.pendingHasDepth == opts.depth) {
        pipeFmt = c.pendingTarget->pixelFormat();
    }
    // IR pipeline-key cache: nếu mọi input giống pending → tái dùng pipeline,
    // BỎ CẢ bridge lookup (build string + mutex). Trước đây mỗi draw vẫn lookup.
    std::shared_ptr<metal::IRenderPipeline> pipe;
    {
        Context::PipelineKey want;
        want.vsLib = pr.appleVS.get();
        want.fsLib = pr.appleFS.get();
        want.fmt = pipeFmt;
        want.stride = casN ? cas[0].stride : 0;
        want.depth = opts.depth;
        want.blend = opts.blend;
        want.colorWriteMask = opts.colorWriteMask;
        if (opts.blend) want.blend0 = opts.blend0;
        want.nAttribs = casN;
        for (uint32_t k = 0; k < casN; ++k) want.attribs[k] = cas[k];
        if (c.pendingPipeValid && c.cachedPipe && want == c.pendingPipeKey) {
            pipe = c.cachedPipe;
            c.appleStats.pipelineLookupSkipped++;
        } else {
            c.appleStats.pipelineLookups++;
            pipe = c.device->makeCustomPipeline(pr.appleVS.get(), "TGLMT_vs", pr.appleFS.get(),
                                                "TGLMT_fs", pipeFmt,
                                                casN ? cas : nullptr, casN,
                                                casN ? cas[0].stride : 0, &opts);
            if (pipe) {
                c.pendingPipeKey = std::move(want);
                c.pendingPipeValid = true;
                c.cachedPipe = pipe;
            } else {
                c.pendingPipeValid = false;
                c.cachedPipe.reset();
            }
        }
    }
    if (!pipe) {
        c.appleStats.noPipeline++;
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: pipeline nil (format attrib chưa hỗ trợ?)");
        return false;
    }
    // 5. Encoder IR (deferred materialization): 1 encoder cho N draw liên tiếp.
    // Trước đây: mỗi draw = 1 commandBuffer + 1 renderPass + commit (1 GL → 3 Metal).
    // Giờ: cùng target + cùng hasDepth + không có pendingClear → tái dùng encoder,
    // chỉ setPipeline khi nativeHandle đổi, chỉ set* khi dirty.
    std::shared_ptr<metal::IRenderEncoder> enc;
    bool isNewEncoder = false;
    {
        bool canReuse = c.pendingEncoder && c.pendingTarget && !c.applePendingClear &&
                        c.pendingDrawFBO == curDrawFBO && c.pendingColorTex == curColorTex &&
                        c.pendingHasDepth == opts.depth;
        if (canReuse) {
            enc = c.pendingEncoder;
            target = c.pendingTarget; // dùng target đang mở (tránh wrap mới mỗi draw)
            c.appleStats.encoderReused++;
        } else {
            if (c.pendingEncoder) c.FlushPendingEncoder();
            metal::LoadOp cl = metal::LoadOp::Load, dl = metal::LoadOp::Load;
            if (c.applePendingClear) {
                // clrmiss#: glClear ghi vào fbo A nhưng draw tiêu thụ đang bound
                // fbo B → clear áp sai target (Metal loadAction gắn theo encoder
                // của draw này; clear cho A mất). Nguyên nhân nghi ngờ sky-blue
                // clear bị lightmap/GUI draw "ăn" → world không clear → đen.
                if (c.DiagOn() && c.appleClearFBO != 0xFFFFFFFFu &&
                    c.appleClearFBO != (uint32_t)curDrawFBO) {
                    static uint64_t nMiss = 0;
                    if (nMiss++ < 32)
                        fprintf(stderr,
                                "[TGLMT] clrmiss# clearFbo=%u drawFbo=%u mask=0x%x "
                                "rgba=(%.3f,%.3f,%.3f,%.3f)\n",
                                c.appleClearFBO, (uint32_t)curDrawFBO, c.appleClearMask,
                                c.clearColor[0], c.clearColor[1], c.clearColor[2],
                                c.clearColor[3]);
                }
                if (c.appleClearMask & 0x00004000u) cl = metal::LoadOp::Clear; // COLOR_BUFFER_BIT
                if (c.appleClearMask & 0x00000100u) dl = metal::LoadOp::Clear; // DEPTH_BUFFER_BIT
                c.applePendingClear = false;
                c.appleClearMask = 0;
            }
            enc = c.device->makeRenderEncoderActions(
                target.get(), pipe.get(),
                metal::ClearColor{(double)c.clearColor[0], (double)c.clearColor[1],
                                 (double)c.clearColor[2], (double)c.clearColor[3]},
                c.clearDepth, cl, dl);
            if (!enc) {
                c.appleStats.miscFail++;
                // fixico8: encoder nil thường = depth-attachment sai format
                // (FBO depth RGBA8 vs pipeline Depth32Float) → draw bị bỏ im lặng.
                if (c.DiagOn()) {
                    static uint64_t nEncNil = 0;
                    if (nEncNil++ < 24)
                        fprintf(stderr,
                                "[TGLMT] encnil# fbo=%u colorTex=%u depth=%d pipeFmt=%u "
                                "cl=%d dl=%d\n",
                                (unsigned)curDrawFBO, (unsigned)curColorTex,
                                (int)opts.depth, (unsigned)pipeFmt, (int)cl, (int)dl);
                }
                return false;
            }
            c.pendingEncoder = enc;
            c.pendingTarget = target;
            c.pendingPipeline = pipe;
            c.pendingHasDepth = opts.depth;
            c.pendingDrawFBO = curDrawFBO;
            c.pendingColorTex = curColorTex;
            // Encoder mới đã có pipeline (bridge set ở creation) + chưa có state nào:
            // đánh dấu mọi shadow là invalid để lần set đầu luôn encode.
            c.pendingViewportValid = false;
            c.pendingCullValid = false;
            c.pendingBlendValid = false;
            c.pendingDepthValid = false;
            c.pendingDepthState.reset();
            c.pendingFillValid = false;
            c.pendingScissorValid = false;
            c.appleStats.encodersCreated++;
            isNewEncoder = true;
        }
        // Reused encoder nhưng pipeline khác → đổi pipeline giữa pass (Metal cho phép
        // khi hasDepth giống nhau, đã gate ở canReuse).
        if (!isNewEncoder) {
            uint64_t wantH = pipe->nativeHandle();
            uint64_t haveH = c.pendingPipeline ? c.pendingPipeline->nativeHandle() : 0;
            if (haveH && wantH == haveH) {
                c.appleStats.pipelineReused++; // 0 Metal cho pipeline
            } else {
                enc->setPipeline(pipe.get());
                c.pendingPipeline = pipe;
            }
        }
    }
    if (!enc) { c.appleStats.miscFail++; return false; }
    // Viewport/scissor (GL→Metal convert) — IR dirty-check: chỉ encode khi đổi.
    {
        ViewportState vp0 = c.state.GetViewport(0);
        float th = (float)target->height();
        bool upper = c.state.ClipOrigin() == 0x8CA2; // UPPER_LEFT (đã đối chiếu gl.xml)
        bool z10 = c.state.ClipDepth() == 0x935F;    // ZERO_TO_ONE (đã đối chiếu gl.xml)
        metal::Viewport mvp = GLViewportToMetal(vp0.x, vp0.y, vp0.w, vp0.h, vp0.n, vp0.f,
                                               th, upper, z10);
        // Viewport chưa set (w=0) → fullscreen target (cùng dấu với converted:
        // GL-correct = h<0; UPPER_LEFT = h>0)
        if (vp0.w <= 0 || vp0.h <= 0) {
            double H = (double)target->height();
            double W = (double)target->width();
            mvp = upper ? metal::Viewport{0, 0, W, H, 0, 1}
                        : metal::Viewport{0, H, W, -H, 0, 1};
        }
        bool vpSame = c.pendingViewportValid &&
                      c.pendingViewport.x==mvp.x && c.pendingViewport.y==mvp.y &&
                      c.pendingViewport.w==mvp.w && c.pendingViewport.h==mvp.h &&
                      c.pendingViewport.n==mvp.n && c.pendingViewport.f==mvp.f;
        if (!vpSame) {
            enc->setViewport(mvp);
            c.pendingViewport = mvp;
            c.pendingViewportValid = true;
        } else {
            c.appleStats.stateSkipped++;
        }
        bool scissorOn = c.state.IsEnabled(0x0C11);
        if (scissorOn) { // SCISSOR_TEST
            auto sc = c.state.GetScissor();
            metal::ScissorRect mr = GLScissorToMetal(sc.x, sc.y, sc.w, sc.h, (int)target->height(),
                                                   upper);
            // kẹp vào target (Metal_scissor vượt biên → lỗi validation)
            if (mr.x < target->width() && mr.y < target->height()) {
                if (mr.x + mr.w > target->width()) mr.w = target->width() - mr.x;
                if (mr.y + mr.h > target->height()) mr.h = target->height() - mr.y;
                bool scSame = c.pendingScissorValid && c.pendingScissorEnabled &&
                              c.pendingScissor.x==mr.x && c.pendingScissor.y==mr.y &&
                              c.pendingScissor.w==mr.w && c.pendingScissor.h==mr.h;
                if (!scSame) {
                    enc->setScissorRect(mr);
                    c.pendingScissor = mr;
                    c.pendingScissorEnabled = true;
                    c.pendingScissorValid = true;
                } else {
                    c.appleStats.stateSkipped++;
                }
            }
        } else if (c.pendingScissorValid && c.pendingScissorEnabled) {
            // FIX scis7: tắt scissor sau khi đã bật → PHẢI set encoder về full
            // target. Trước đây chỉ clear flag → encoder giữ scissorRect cũ →
            // các draw sau trong CÙNG pass bị clip vào rect 80x80 cũ (mất chữ
            // nút + mất icon GUI có scissorArea=null — GuiRenderer.executeDraw
            // gọi renderPass.disableScissor() khi scissorArea==null).
            metal::ScissorRect full{0, 0, (uint32_t)target->width(),
                                    (uint32_t)target->height()};
            enc->setScissorRect(full);
            c.pendingScissor = full;
            c.pendingScissorEnabled = false;
            c.pendingScissorValid = true;
        }
    }
    // Cull / fillMode / blendColor / depthState — dirty-check từng món.
    {
        bool cullOn = c.state.IsEnabled(0x0B44);
        uint32_t cullM = c.state.CullMode(), frontF = c.state.FrontFace();
        // GL-correct viewport (h<0) lật orientation visual trong Metal fb-space:
        // face CCW của GL xuất hiện CW → phải đảo frontFace để cull đúng GL
        // (verify tích hợp: neg-vp + front CCW → cull; front CW → sống).
        if (c.state.ClipOrigin() != 0x8CA2)
            frontF = (frontF == 0x0900) ? 0x0901u : 0x0900u;
        bool cullSame = c.pendingCullValid && c.pendingCullEnabled==cullOn &&
                        c.pendingCullMode==cullM && c.pendingFrontFace==frontF;
        if (!cullSame) {
            enc->setCullMode(cullOn, cullM, frontF);
            c.pendingCullEnabled = cullOn;
            c.pendingCullMode = cullM;
            c.pendingFrontFace = frontF;
            c.pendingCullValid = true;
        } else {
            c.appleStats.stateSkipped++;
        }
    }
    {
        bool wantLines = (c.state.PolygonMode() == 0x1B01);
        bool fillSame = c.pendingFillValid && c.pendingFillLines == wantLines;
        // Metal default là FILL: chỉ encode khi muốn LINES (tránh 1 Metal call mỗi draw
        // cho case FILL phổ biến). Khi từ LINES về FILL trong cùng encoder, phải
        // encode lại FILL (không thì kẹt LINES).
        if (!fillSame) {
            enc->setTriangleFillModeLines(wantLines);
            c.pendingFillLines = wantLines;
            c.pendingFillValid = true;
        } else {
            c.appleStats.stateSkipped++;
        }
    }
    {
        float bc[4];
        c.state.GetBlendColor(bc);
        bool bSame = c.pendingBlendValid &&
                     c.pendingBlend[0]==bc[0] && c.pendingBlend[1]==bc[1] &&
                     c.pendingBlend[2]==bc[2] && c.pendingBlend[3]==bc[3];
        // Blend color chỉ ảnh hưởng khi blend bật CONSTANT_*: vẫn encode 1 lần đầu
        // (pending invalid), sau đó skip khi trùng — đúng "0 Metal khi không đổi".
        if (!bSame) {
            enc->setBlendColor(bc[0], bc[1], bc[2], bc[3]);
            c.pendingBlend[0]=bc[0]; c.pendingBlend[1]=bc[1];
            c.pendingBlend[2]=bc[2]; c.pendingBlend[3]=bc[3];
            c.pendingBlendValid = true;
        } else {
            c.appleStats.stateSkipped++;
        }
    }
    std::shared_ptr<metal::IDepthStencilState> dss;
    if (opts.depth) {
        uint32_t df = c.state.Depth().func;
        bool dw = c.state.Depth().writeMask;
        bool dSame = c.pendingDepthValid && c.pendingDepthFunc==df &&
                     c.pendingDepthMask==dw && c.pendingDepthState;
        if (dSame) {
            c.appleStats.depthReused++;
        } else {
            dss = c.device->makeDepthStencilState(df, dw); // đã cache 16 states ở bridge
            if (!dss) { c.appleStats.miscFail++; return false; }
            enc->setDepthStencilState(dss.get());
            c.pendingDepthFunc = df;
            c.pendingDepthMask = dw;
            c.pendingDepthState = dss;
            c.pendingDepthValid = true;
        }
    } else if (c.pendingDepthValid) {
        // Từ depth → không depth trong cùng pass: pipeline đã đổi sang non-depth
        // (hasDepth gate flush), depth attachment không còn ý nghĩa → xóa shadow.
        c.pendingDepthValid = false;
        c.pendingDepthState.reset();
    }
    // Vertex buffers theo binding + ghi nhận cho conditional-flush.
    // Giải shared_ptr ĐỘT NÀY (sau mọi flush) để bind đúng buffer vừa ghi.
    for (int bi = 0; bi < 16; ++bi) {
        if (!(bindMask & (1u << bi))) continue;
        auto bit = c.buffers.find(bindMap[bi].id);
        if (bit == c.buffers.end() || !bit->second.gpu) { c.appleStats.miscFail++; return false; }
        enc->setVertexBuffer(bit->second.gpu.get(), bindMap[bi].off, (uint32_t)bi);
        c.NoteBufferUsed(bindMap[bi].id);
    }
    // Uniforms tách VS/FS — IR staging cache + ring upload (0 alloc khi đổi):
    // nếu bytes giống lần trước cùng program thì tái dùng buffer+offset cũ.
    std::shared_ptr<metal::IBuffer> vsUbuf, fsUbuf;
    size_t vsUoff = 0, fsUoff = 0;
    {
        size_t vsSize = pr.vsUBSize ? pr.vsUBSize : 16;
        size_t fsSize = pr.fsUBSize ? pr.fsUBSize : 16;
        // Scratch tái dùng (thread_local): cấp phát vector mỗi draw là malloc
        // + memset 2 lần × hàng trăm draw/frame.
        thread_local std::vector<uint8_t> vsb, fsb;
        if (vsb.size() != vsSize) vsb.assign(vsSize, 0); else std::fill(vsb.begin(), vsb.end(), 0);
        if (fsb.size() != fsSize) fsb.assign(fsSize, 0); else std::fill(fsb.begin(), fsb.end(), 0);
        for (auto& e : pr.uniformLayout) {
            if (e.loc < 0) continue;
            auto uit = pr.uniforms.find(e.loc);
            if (uit == pr.uniforms.end()) continue;
            // e.offset hiện là tuyệt đối gộp (vs [0,vsUB) + fs [align16, ...)).
            // Chuyển về stage-local: vs giữ nguyên, fs trừ base align16.
            size_t base = e.isVS ? 0 : ((pr.vsUBSize + 15) & ~((size_t)15));
            size_t local = (e.offset >= base) ? e.offset - base : e.offset;
            std::vector<uint8_t>* dst = e.isVS ? &vsb : &fsb;
            size_t n = std::min(e.size, uit->second.size());
            if (local + n <= dst->size()) memcpy(dst->data() + local, uit->second.data(), n);
        }
        auto& uc = c.uniformCache;
        bool sameProg = uc.valid && uc.prog == prog;
        bool sameVS = sameProg && uc.vsBytes.size()==vsb.size() &&
                      memcmp(uc.vsBytes.data(), vsb.data(), vsb.size())==0 && uc.vsBuf;
        bool sameFS = sameProg && uc.fsBytes.size()==fsb.size() &&
                      memcmp(uc.fsBytes.data(), fsb.data(), fsb.size())==0 && uc.fsBuf;
        if (sameVS && sameFS) {
            vsUbuf = uc.vsBuf; vsUoff = uc.vsOff;
            fsUbuf = uc.fsBuf; fsUoff = uc.fsOff;
            c.appleStats.uniformReused++;
        } else {
            // Chỉ upload stage đã đổi (stage còn lại tái dùng nếu giống).
            if (sameVS) {
                vsUbuf = uc.vsBuf; vsUoff = uc.vsOff;
                c.appleStats.uniformReused++;
            } else {
                metal::IBuffer* raw = nullptr;
                std::shared_ptr<metal::IBuffer> keep;
                size_t off = RingUpload(c, vsb.data(), vsb.size(), raw, keep);
                if (raw) {
                    // Ring hit: giữ shared_ptr ring slot để sống qua flush.
                    if (!keep) {
                        for (size_t i = 0; i < Context::kRingFrames; ++i)
                            if (c.ringBuf[i] && c.ringBuf[i].get() == raw) { keep = c.ringBuf[i]; break; }
                    }
                    vsUbuf = keep; vsUoff = off;
                    if (vsUbuf) { uc.vsBytes = vsb; uc.vsBuf = vsUbuf; uc.vsOff = off; }
                }
            }
            if (sameFS) {
                fsUbuf = uc.fsBuf; fsUoff = uc.fsOff;
                c.appleStats.uniformReused++;
            } else {
                metal::IBuffer* raw = nullptr;
                std::shared_ptr<metal::IBuffer> keep;
                size_t off = RingUpload(c, fsb.data(), fsb.size(), raw, keep);
                if (raw) {
                    if (!keep) {
                        for (size_t i = 0; i < Context::kRingFrames; ++i)
                            if (c.ringBuf[i] && c.ringBuf[i].get() == raw) { keep = c.ringBuf[i]; break; }
                    }
                    fsUbuf = keep; fsUoff = off;
                    if (fsUbuf) { uc.fsBytes = fsb; uc.fsBuf = fsUbuf; uc.fsOff = off; }
                }
            }
            uc.prog = prog;
            uc.valid = (vsUbuf && fsUbuf);
            // Ring buffers sống theo Context; fallback newBuffer giữ qua pendingKeep
            // (RingUpload đã push pendingKeep cho fallback).
        }
        if (!vsUbuf || !fsUbuf) { c.appleStats.miscFail++; return false; }
        enc->setVertexBuffer(vsUbuf.get(), vsUoff, 16);
        enc->setFragmentBuffer(fsUbuf.get(), fsUoff, 16);
    }
    // UBO read-only (vanilla 1.17+/Sodium): bind PER-STAGE theo thứ tự khai báo
    // của stage đó (khớp [[buffer(17+bi)]] trong MSL: bi = index trong stage).
    // Block → (tên → bindingPoint từ glUniformBlockBinding) → GL buffer
    // (BindBufferBase/Range). Gộp thứ tự vs-trước bind chung cả 2 stage sẽ lệch
    // khi vs/fs khai báo khác nhau (post blur) → đọc nhầm buffer → treo GPU.
    // Block thiếu buffer → bind zero fallback (đúng hơn fault GPU).
    // zero-fallback buffer là singleton cache → giữ 1 shared_ptr cho cả draw
    // (trước đây push_back mỗi slot UB thiếu buffer = atomic inc/dec mỗi draw).
    std::shared_ptr<metal::IBuffer> uboKeepOne;
    {
        uboKeepOne = FallbackZeroBuf(c);
        auto bindZeroV = [&](int s) {
            if (!uboKeepOne) return;
            enc->setVertexBuffer(uboKeepOne.get(), 0, (uint32_t)s);
        };
        auto bindZeroF = [&](int s) {
            if (!uboKeepOne) return;
            enc->setFragmentBuffer(uboKeepOne.get(), 0, (uint32_t)s);
        };
        struct UboBind { metal::IBuffer* buf = nullptr; size_t off = 0; std::shared_ptr<metal::IBuffer> keep; };
        auto uploadBlock = [&](const std::string& nm) -> UboBind {
            UboBind r;
            GLuint point = 0;
            size_t need = 0, trueNeed = 0;
            for (auto& b : pr.uniformBlocks)
                if (b.name == nm) {
                    point = b.binding;
                    need = b.minSize;
                    trueNeed = b.trueSize ? b.trueSize : b.minSize;
                    break;
                }
            auto bit = c.uniformBindPoints.find(point);
            if (bit == c.uniformBindPoints.end() || !bit->second.buffer) {
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " unbound, zero fallback");
                if (c.DiagOn()) {
                    // Point chưa từng glBindBufferBase → Globals không bao giờ
                    // được nạp → scenery sai ánh sáng. Dedupe (prog,name) cap 32.
                    static std::set<uint64_t> loggedMiss;
                    uint64_t key = ((uint64_t)prog << 32) ^
                                   std::hash<std::string>{}(nm);
                    if (loggedMiss.size() < 32 && loggedMiss.insert(key).second) {
                        fprintf(stderr, "[TGLMT] uboMissing#%zu prog@%u %s point=%u\n",
                                loggedMiss.size(), prog, nm.c_str(), point);
                        fflush(stderr);
                    }
                }
                return r;
            }
            auto t = c.buffers.find(bit->second.buffer);
            if (t == c.buffers.end() || t->second.data.empty()) {
                if (c.DiagOn()) {
                    static std::set<uint64_t> loggedNodata;
                    uint64_t nk = ((uint64_t)prog << 32) ^ std::hash<std::string>{}(nm);
                    if (loggedNodata.size() < 32 && loggedNodata.insert(nk).second) {
                        fprintf(stderr,
                                "[TGLMT] uboNodata#%zu prog@%u %s point=%u buf=%u%s\n",
                                loggedNodata.size(), prog, nm.c_str(), point,
                                bit->second.buffer, (t == c.buffers.end()) ? " missing" : " empty");
                        fflush(stderr);
                    }
                }
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " nodata, zero fallback");
                return r;
            }
            // Ghi nhận UBO source cho conditional-flush (SubData sau draw này mà
            // đụng buffer này thì phải split, còn không thì giữ batching).
            c.NoteBufferUsed(bit->second.buffer);
            size_t off = (size_t)bit->second.offset;
            if (off >= t->second.data.size()) {
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " offset vuot, zero fallback");
                return r;
            }
            size_t len = bit->second.size ? (size_t)bit->second.size
                                          : t->second.data.size() - off;
            len = std::min(len, t->second.data.size() - off);
            if (!len) return r;
            // Buffer thiếu hơn HẾT struct shader đọc (trueSize: misbound, vd
            // Globals đọc nhầm buffer SamplerInfo 16B trong khi struct 52B) →
            // đọc OOB hoặc rác điều khiển loop (MenuBlurRadius khổng lồ → treo
            // GPU → iOS ban submissions, đen + đứng hình). Zero fallback giữ GPU
            // sống (GL coi là undefined, blur thành sharp còn hơn fault).
            //
            // Nguong dung trueSize (KHONG phai minSize da lam tron 140→144):
            // app bind dung struct that (khong gom pad cuoi) → chi thieu chinh
            // pad std140. Zero-fallback luc do PHA SO MA NGHIA (ProjectionMatrix/
            // SpriteMatrix = 0 → gl_Position = 0 → draw degenerate → atlas bake +
            // terrain + GUI VO HINH). Thuong gap: SpriteAnimationInfo 140/144,
            // Fog 40/48, Globals 56/64, BlurConfig 12/16.
            if (need && len < trueNeed) {
                ++c.appleStats.uboSmall;
                static std::set<uint64_t> loggedSmall;
                uint64_t sk = ((uint64_t)prog << 32) ^ std::hash<std::string>{}(nm);
                if (loggedSmall.size() < 32 && loggedSmall.insert(sk).second) {
                    if (c.DiagOn()) {
                        fprintf(stderr,
                                "[TGLMT] uboSmall#%zu prog@%u %s have=%zu need=%zu true=%zu "
                                "point=%u buf=%u\n",
                                loggedSmall.size(), prog, nm.c_str(), len, need, trueNeed,
                                point, bit->second.buffer);
                        fflush(stderr);
                    }
                    char b[160];
                    snprintf(b, sizeof(b),
                             "AppleDrawGL: UBO %s small %zuB < need %zuB, zero fallback",
                             nm.c_str(), len, need);
                    c.LogDebug(0, 0, 0, 0, b);
                }
                return r;
            }
            if (need && len < need) {
                ++c.appleStats.uboPad;
                static std::set<uint64_t> loggedPad;
                uint64_t pk = ((uint64_t)prog << 32) ^ std::hash<std::string>{}(nm);
                if (c.DiagOn() && loggedPad.size() < 32 && loggedPad.insert(pk).second) {
                    fprintf(stderr,
                            "[TGLMT] uboPad#%zu prog@%u %s have=%zu need=%zu true=%zu "
                            "point=%u buf=%u pad-std140\n",
                            loggedPad.size(), prog, nm.c_str(), len, need, trueNeed, point,
                            bit->second.buffer);
                    fflush(stderr);
                }
            }
            // Pad 0 lên bội số 16 (std140 pad; A11 TBDR nghiêm OOB).
            // Ring upload trực tiếp từ shadow; padBuf chỉ alloc khi len lẻ 16.
            // Buffer đang mapped (map trả con trỏ GPU): shadow CHƯA phản ánh
            // bytes game vừa ghi → phải đọc từ gpu->contents(), không đọc shadow
            // (đọc shadow = ma trận cũ → entity méo/chớp).
            size_t padded = (len + 15) & ~((size_t)15);
            // Không bao giờ upload ngắn hơn minSize (shader đọc tới đó → OOB A11).
            if (need > padded) padded = (need + 15) & ~((size_t)15);
            const uint8_t* srcPtr = nullptr;
            if (t->second.mapped && t->second.gpu) {
                const uint8_t* g = (const uint8_t*)t->second.gpu->contents();
                if (g) {
                    srcPtr = g + off;
                    ++c.appleStats.uboFromMap;
                }
            }
            if (!srcPtr) srcPtr = t->second.data.data() + off;
            const void* upPtr = srcPtr;
            std::vector<uint8_t> padBuf;
            size_t upLen = len;
            if (padded != len) {
                padBuf.assign(padded, 0);
                memcpy(padBuf.data(), srcPtr, len);
                upPtr = padBuf.data();
                upLen = padded;
            }
            metal::IBuffer* raw = nullptr;
            std::shared_ptr<metal::IBuffer> keep;
            size_t roff = RingUpload(c, upPtr, upLen, raw, keep);
            if (!raw) return r;
            if (!keep) {
                for (size_t i = 0; i < Context::kRingFrames; ++i)
                    if (c.ringBuf[i] && c.ringBuf[i].get() == raw) { keep = c.ringBuf[i]; break; }
            }
            // Giữ sống qua encode: ring slot sống theo Context, fallback đã
            // được RingUpload push vào pendingKeep (giữ tới khi commit).
            (void)keep;
            r.buf = raw; r.off = roff;
            return r;
        };
        // Tương thích ngược: program link trước khi có vsBlocks/fsBlocks
        // (link cũ) → vsBlocks/fsBlocks rỗng → dùng merged order cho cả 2 stage.
        bool legacy = pr.vsBlocks.empty() && pr.fsBlocks.empty();
        if (legacy) {
            int slot = 17;
            for (auto& b : pr.uniformBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(b.name);
                if (!ub.buf) {
                    if (uboKeepOne) {
                        enc->setVertexBuffer(uboKeepOne.get(), 0, (uint32_t)slot);
                        enc->setFragmentBuffer(uboKeepOne.get(), 0, (uint32_t)slot);
                    }
                } else {
                    enc->setVertexBuffer(ub.buf, ub.off, (uint32_t)slot);
                    enc->setFragmentBuffer(ub.buf, ub.off, (uint32_t)slot);
                }
                ++slot;
            }
        } else {
            int slot = 17;
            for (auto& nm : pr.vsBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(nm);
                if (ub.buf) enc->setVertexBuffer(ub.buf, ub.off, (uint32_t)slot);
                else bindZeroV(slot);
                ++slot;
            }
            slot = 17;
            for (auto& nm : pr.fsBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(nm);
                if (ub.buf) enc->setFragmentBuffer(ub.buf, ub.off, (uint32_t)slot);
                else bindZeroF(slot);
                ++slot;
            }
        }
    }
    // Sampler/texture theo glUniform1i unit → MSL slot k (fix bug bind theo unit):
    // FS samplers (theo thứ tự khai báo) → fragment texture(k)/sampler(k).
    // VS samplers → vertex texture(k)/sampler(k). Mặc định unit 0 đúng GL.
    // Thiếu texture/không khớp loại → bind fallback ĐEN (đúng GL incomplete =
    // (0,0,0,1)) thay vì bỏ trống argument gây GPU fault trên A11.
    // Cube (panorama samplerCube) bind được khi texture là CUBE thật.
    auto kindOk = [&](const std::string& name, GLenum target) {
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        // Array: chưa có GPU texture đúng loại → đen an toàn (giữ skip).
        if (kind == 'A') return false;
        // Cube: chỉ khi target là CUBE (GPU luôn là cube thật sau fix).
        if (kind == 'C') return target == 0x8513;
        // target==0: DSA bind (glBindTextureUnit) không ghi target → tin tưởng.
        // Chỉ chặn mismatch CHẮC CHẮN (cả hai đã biết mà khác nhau).
        if (target == 0) return true;
        switch (kind) {
            case 'B': return target == 0x8C2A; // TEXTURE_BUFFER
            default: return target == 0x0DE1;  // TEXTURE_2D (+shadow approx)
        }
    };
    // Feedback hazard: render vào texture đồng thời sample nó (TBDR fault).
    // Deferred full: split pass (flush encoder đang mở) thay vì chỉ log.
    // Apple Best Practices: 2 encoders cùng target có sampling-dependency ở giữa
    // thì KHÔNG merge được → phải split. Đếm hazardSplits để test chứng minh.
    bool feedbackHazard = false;
    auto hazardCheck = [&](GLuint texId) {
        if (drawColorTexId && texId == drawColorTexId) {
            feedbackHazard = true;
            static std::set<GLuint> warnedHz;
            if (warnedHz.size() < 16 && warnedHz.insert(prog).second) {
                ++c.appleStats.hazardWarn;
                char b[128];
                snprintf(b, sizeof(b), "AppleDrawGL: FEEDBACK prog@%u sample tex#%u dang render",
                         prog, texId);
                c.LogDebug(0, 0, 0, 0, b);
            }
        }
    };
    // Dedupe log sampler: cùng (prog,unit,tex) lặp mọi frame → 1 lần. Cap 48
    // để 1 issue (tex nop full) không nuốt issue khác (với cap 16 cũ).
    static std::set<uint64_t> sSampSeen;
    auto sampKey = [&](GLuint u, GLuint t) -> uint64_t {
        return ((uint64_t)prog << 40) | ((uint64_t)u << 20) | (uint64_t)t;
    };
    auto sampLogNew = [&](uint64_t k) {
        return sSampSeen.size() < 48 && sSampSeen.insert(k).second;
    };
    for (size_t k = 0; k < pr.vsSamplers.size(); ++k) {
        const std::string& name = pr.vsSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        // array: bind fallback ĐEN (kindOk=false) thay vì `continue` bỏ trống slot
        // → slot giữ texture CỦA DRAW TRƯỚC (stale) → sai texture/head đen.
        const TextureObject* txp = nullptr;
        metal::ITexture* gpu = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit, KindTarget(kind));
            auto tit = c.textures.find(texId);
            if (tit != c.textures.end() && tit->second.gpu &&
                kindOk(name, tit->second.target)) {
                txp = &tit->second;
                gpu = txp->gpu.get();
                hazardCheck(texId);
                c.NoteTextureUsed(texId);
            } else if (tit != c.textures.end()) {
                if (c.DiagOn()) {
                    static int nDenyV = 0;
                    if (sampLogNew(sampKey(unit, texId))) {
                        fprintf(stderr,
                                "[TGLMT] sampdenyV#%d prog@%u vs=%s unit=%u tex#%u tgt=0x%x ifmt=0x%x gpu=%d\n",
                                ++nDenyV, prog, name.c_str(), unit, texId,
                                tit->second.target, tit->second.internalFormat,
                                (int)(tit->second.gpu != nullptr));
                        fflush(stderr);
                    }
                } else {
                    ++c.appleStats.diagSkipped;
                }
                c.LogDebug(0, 0, 0, 0,
                           "AppleDrawGL: VS sampler " + name + " thieu/khop, fallback den");
            }
        }
        std::shared_ptr<metal::ISamplerState> ss;
        if (!gpu) {
            gpu = (kind == 'C' ? FallbackBlackCube(c) : FallbackBlackTex(c)).get();
            if (!gpu) continue; // backend nghẽn, bỏ qua trung thực
            static thread_local TextureObject dummyTex;
            ss = SamplerForUnit(c, 0, dummyTex);
        } else {
            ss = SamplerForUnit(c, unit, *txp);
        }
        enc->setVertexTexture(gpu, (uint32_t)k);
        if (ss) enc->setVertexSamplerState(ss.get(), (uint32_t)k);
    }
    for (size_t k = 0; k < pr.fsSamplers.size(); ++k) {
        const std::string& name = pr.fsSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        // array: bind fallback ĐEN (kindOk=false) thay vì `continue` bỏ trống slot
        // → slot giữ texture CỦA DRAW TRƯỚC (stale) → sai texture/head đen.
        const TextureObject* txp = nullptr;
        metal::ITexture* gpu = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit, KindTarget(kind));
            auto tit = c.textures.find(texId);
            // Fix #6: shadow-like — verify id + fingerprint shadow.png MỖI draw.
            // Fingerprint đọc shadow pixels (glTexSubImage ghi vào đây ngay) →
            // bắt cả bind-sai (unit trỏ texture khác) lẫn upload ghi đè nội
            // dung; lệch → shadowmis# + dump texring 96 call bind gần nhất.
            if (pr.shadowLike && c.DiagOn() && tit != c.textures.end()) {
                struct Arm { GLuint id = 0; uint64_t fp = 0; };
                static std::map<GLuint, Arm> armed; // prog → (texId, fp) đã xác nhận
                static int nSig = 0, nMis = 0;
                auto fpOf = [](const TextureObject& t, double* avgRgb) -> uint64_t {
                    uint64_t fp = ((uint64_t)t.w << 32) | (uint64_t)t.h;
                    size_t npx = (size_t)t.w * (size_t)t.h;
                    size_t bpp = npx ? t.pixels.size() / npx : 0;
                    if (!npx || bpp < 4) { if (avgRgb) *avgRgb = -1.0; return fp; }
                    size_t stride = std::max<size_t>(1, npx / 1024);
                    uint64_t mix = 1469598103934665603ull; // FNV offset basis
                    uint32_t rgbSum = 0, cnt = 0;
                    for (size_t i = 0; i < npx; i += stride) {
                        const uint8_t* q = t.pixels.data() + i * bpp;
                        rgbSum += (uint32_t)q[0] + q[1] + q[2];
                        mix = (mix ^ ((uint64_t)q[0] << 16 | (uint64_t)q[1] << 8 | q[2])) *
                              1099511628211ull; // FNV prime
                        mix ^= (uint64_t)q[3] << 40;
                        ++cnt;
                    }
                    if (avgRgb) *avgRgb = cnt ? (double)rgbSum / (cnt * 3.0) : -1.0;
                    return fp ^ mix ^ ((uint64_t)cnt << 40);
                };
                double avgRgb = -1.0;
                uint64_t fp = fpOf(tit->second, &avgRgb);
                auto ait = armed.find(prog);
                if (ait == armed.end()) {
                    // Arm khi đúng chữ ký shadow.png (64×64, RGB≈0): tránh program
                    // khác cùng tag DT+Fog+Proj+1 sampler gây nhiễu.
                    bool sig = tit->second.w == 64 && tit->second.h == 64 &&
                               avgRgb >= 0.0 && avgRgb < 4.0;
                    if (sig) {
                        armed[prog] = Arm{texId, fp};
                        fprintf(stderr, "[TGLMT] shadowbind# prog@%u tex=%u fp=%llx\n",
                                prog, texId, (unsigned long long)fp);
                        fflush(stderr);
                    } else if (++nSig <= 4) {
                        fprintf(stderr,
                                "[TGLMT] shadowsig#%d prog@%u tex=%u %ux%u avgRgb=%.1f (khong phai shadow.png?)\n",
                                nSig, prog, texId, tit->second.w, tit->second.h, avgRgb);
                        fflush(stderr);
                        c.DumpTexBindRing("shadowsig");
                    }
                } else if (texId != ait->second.id || fp != ait->second.fp) {
                    if (nMis < 8) {
                        ++nMis;
                        fprintf(stderr,
                                "[TGLMT] shadowmis#%d prog@%u exp tex=%u fp=%llx -> got tex=%u fp=%llx %ux%u avgRgb=%.1f\n",
                                nMis, prog, ait->second.id, (unsigned long long)ait->second.fp,
                                texId, (unsigned long long)fp, tit->second.w, tit->second.h,
                                avgRgb);
                        fflush(stderr);
                        c.DumpTexBindRing("shadowmis");
                    }
                    ait->second = Arm{texId, fp}; // re-arm: mỗi lần đổi 1 dump
                }
            }
            if (tit != c.textures.end() && tit->second.gpu &&
                kindOk(name, tit->second.target)) {
                txp = &tit->second;
                gpu = txp->gpu.get();
                hazardCheck(texId);
                c.NoteTextureUsed(texId);
                // Soi texture GUI: chỉ khi TGLMT_DIAG=1, và KHÔNG readback GPU
                // trong frame nóng (readback = sync stall ms). Dùng shadow pixels.
                // Cũ: w==256 && h∈{256,128} → atlas 512 (widgets/sprites) không
                // bao giờ lọt (guitex#=0 dù mất icon) → mở rộng ≥256×≥128.
                if (c.DiagOn() && txp->w >= 256 && txp->h >= 128) {
                    static std::set<GLuint> loggedGui;
                    if (loggedGui.size() < 16 && loggedGui.insert(texId).second) {
                        uint32_t mf = 0x2601, gf = 0x2601;
                        auto gp = [&](GLenum kk, uint32_t d) {
                            auto f2 = txp->params.find(kk);
                            return f2 == txp->params.end() ? d : (uint32_t)f2->second;
                        };
                        mf = gp(0x2801, mf); gf = gp(0x2800, gf);
                        std::string px0 = "none";
                        if (!txp->pixels.empty() && txp->pixels.size() >= 4) {
                            char pb[32];
                            snprintf(pb, sizeof(pb), "sh%02X%02X%02X%02X",
                                     txp->pixels[0], txp->pixels[1], txp->pixels[2],
                                     txp->pixels[3]);
                            px0 = pb;
                        }
                        // Deferred full: bỏ GPU readback trong draw (stall).
                        // Shadow là đủ cho chẩn đoán format/filter/state.
                        std::string gpx = "skip-rb";
                        auto sc = c.state.GetScissor();
                        ViewportState vpg = c.state.GetViewport(0);
                        const BlendState& bb = c.state.Blend()[0];
                        char b[512];
                        snprintf(b, sizeof(b),
                                 "[TGLMT] guitex prog@%u samp=%s unit=%u tex#%u %ux%u "
                                 "min=0x%x mag=0x%x ifmt=0x%x lv=%u px0=%s+%s "
                                 "blend=%d(%x->%x) depth=%d cull=%d scis=%d[%d,%d,%d,%d] "
                                 "vp=%.0f,%.0f,%.0f,%.0f",
                                 prog, name.c_str(), unit, texId, txp->w, txp->h, mf, gf,
                                 txp->internalFormat, txp->levels, px0.c_str(), gpx.c_str(),
                                 (int)c.state.IsEnabled(0x0BE2), bb.srcRGB, bb.dstRGB,
                                 (int)c.state.IsEnabled(0x0B71),
                                 (int)c.state.IsEnabled(0x0B44),
                                 (int)c.state.IsEnabled(0x0C11), sc.x, sc.y, sc.w, sc.h,
                                 vpg.x, vpg.y, vpg.w, vpg.h);
                        fprintf(stderr, "%s\n", b);
                        fflush(stderr);
                    }
                } else if (!c.DiagOn() && txp->w >= 256) {
                    ++c.appleStats.diagSkipped;
                }
            } else if (tit != c.textures.end()) {
                // kindOk từ chối hoặc thiếu GPU: sample đen → quad trong suốt →
                // discard. Chỉ log khi DIAG để giữ throughput.
                if (c.DiagOn()) {
                    static int nDeny = 0;
                    if (sampLogNew(sampKey(unit, texId))) {
                        fprintf(stderr,
                                "[TGLMT] sampdeny#%d prog@%u fs=%s unit=%u tex#%u %ux%u "
                                "tgt=0x%x ifmt=0x%x gpu=%d\n",
                                ++nDeny, prog, name.c_str(), unit, texId, tit->second.w,
                                tit->second.h, tit->second.target,
                                tit->second.internalFormat,
                                (int)(tit->second.gpu != nullptr));
                        fflush(stderr);
                    }
                } else {
                    ++c.appleStats.diagSkipped;
                }
                c.LogDebug(0, 0, 0, 0,
                           "AppleDrawGL: FS sampler " + name + " thieu/khop, fallback den");
            } else {
                // texId 0/unknown: sampler không bind gì (GL incomplete = đen).
                if (c.DiagOn()) {
                    static int nMiss = 0;
                    if (sampLogNew(sampKey(unit, texId))) {
                        fprintf(stderr, "[TGLMT] sampmiss#%d prog@%u fs=%s unit=%u tex#%u\n",
                                ++nMiss, prog, name.c_str(), unit, texId);
                        fflush(stderr);
                    }
                } else {
                    ++c.appleStats.diagSkipped;
                }
            }
        }
        std::shared_ptr<metal::ISamplerState> ss;
        if (!gpu) {
            gpu = (kind == 'C' ? FallbackBlackCube(c) : FallbackBlackTex(c)).get();
            if (!gpu) continue; // backend nghẽn, bỏ qua trung thực
            static thread_local TextureObject dummyTex;
            ss = SamplerForUnit(c, 0, dummyTex);
        } else {
            ss = SamplerForUnit(c, unit, *txp);
        }
        enc->setFragmentTexture(gpu, (uint32_t)k);
        if (ss) enc->setFragmentSamplerState(ss.get(), (uint32_t)k);
    }
    // Tương thích ngược: program cũ không có sampler list (link trước fix) → bind legacy theo unit
    if (pr.fsSamplers.empty() && pr.vsSamplers.empty()) {
        for (GLuint unit = 0; unit < 32; ++unit) {
            GLuint texId = c.state.BoundTexture(unit);
            if (!texId) continue;
            auto tit = c.textures.find(texId);
            if (tit == c.textures.end() || !tit->second.gpu) continue;
            c.NoteTextureUsed(texId);
            enc->setFragmentTexture(tit->second.gpu.get(), unit);
            auto ss = SamplerForUnit(c, unit, tit->second);
            if (ss) enc->setFragmentSamplerState(ss.get(), unit);
        }
    }
    // Index buffer. Metal drawIndexed KHÔNG có baseVertex → emulate bằng cách cộng
    // baseVertex vào từng giá trị index (đúng GL). baseVertex âm làm index âm → lỗi.
    // Deferred full: fast-path EBO GPU giữ nguyên (0 copy); còn lại ring-upload.
    std::shared_ptr<metal::IBuffer> ibKeep;
    metal::IBuffer* ibRaw = nullptr;
    size_t ioff = 0;
    std::shared_ptr<metal::IBuffer> ibOwned; // giữ fallback newBuffer sống đến flush
    metal::IndexType ity = metal::IndexType::UInt32;
    auto ibGet = [&]() -> metal::IBuffer* { return ibRaw ? ibRaw : ibOwned.get(); };
    if (indexed) {
        if (indexType != 0x1403 && indexType != 0x1405) return false;
        ity = (indexType == 0x1403) ? metal::IndexType::UInt16 : metal::IndexType::UInt32;
        size_t elem = (indexType == 0x1403) ? 2 : 4;
        auto bit = c.buffers.find(eboId);
        const uint8_t* srcBytes = nullptr;
        if (bit != c.buffers.end() && indexByteOff + (size_t)count * elem <= bit->second.data.size()) {
            srcBytes = bit->second.data.data() + indexByteOff; // shadow luôn có (BufferData giữ)
        } else if (indexData && eboId == 0) {
            srcBytes = (const uint8_t*)indexData; // client pointer
        }
        if (eboId) c.NoteBufferUsed(eboId);
        if (baseVertex == 0) {
            if (bit != c.buffers.end() && bit->second.gpu && srcBytes &&
                indexByteOff + (size_t)count * elem <= bit->second.gpu->length()) {
                ibOwned = bit->second.gpu; // fast path: dùng thẳng EBO GPU
                ibRaw = ibOwned.get();
                ioff = indexByteOff;
            } else if (srcBytes) {
                size_t roff = RingUpload(c, srcBytes, (size_t)count * elem, ibRaw, ibKeep);
                if (ibKeep) { /*fallback đã push pendingKeep*/ }
                else if (ibRaw) {
                    for (size_t i = 0; i < Context::kRingFrames; ++i)
                        if (c.ringBuf[i] && c.ringBuf[i].get() == ibRaw) { ibOwned = c.ringBuf[i]; break; }
                } else {
                    ibOwned = ibKeep;
                }
                ioff = roff;
                if (!ibGet()) return false;
            } else {
                return false;
            }
        } else {
            if (!srcBytes) return false;
            // Viết lại indices + baseVertex vào buffer tạm (ring, 0 alloc)
            std::vector<uint8_t> rewritten((size_t)count * elem);
            for (GLsizei k = 0; k < count; ++k) {
                int64_t v = (elem == 2) ? (int64_t)(srcBytes[2 * k] | (srcBytes[2 * k + 1] << 8))
                                        : (int64_t)(srcBytes[4 * k] | (srcBytes[4 * k + 1] << 8) |
                                                   (srcBytes[4 * k + 2] << 16) | (srcBytes[4 * k + 3] << 24));
                int64_t nv = v + (int64_t)baseVertex;
                if (nv < 0 || (elem == 2 && nv > 0xFFFF) || (elem == 4 && (uint64_t)nv > 0xFFFFFFFFu)) {
                    c.errors.Record(0x0502); // index âm/tràn sau cộng baseVertex
                    return false;
                }
                if (elem == 2) {
                    rewritten[2 * k] = (uint8_t)nv;
                    rewritten[2 * k + 1] = (uint8_t)(nv >> 8);
                } else {
                    rewritten[4 * k] = (uint8_t)nv;
                    rewritten[4 * k + 1] = (uint8_t)(nv >> 8);
                    rewritten[4 * k + 2] = (uint8_t)(nv >> 16);
                    rewritten[4 * k + 3] = (uint8_t)(nv >> 24);
                }
            }
            metal::IBuffer* raw = nullptr;
            size_t roff = RingUpload(c, rewritten.data(), rewritten.size(), raw, ibKeep);
            if (raw && !ibKeep) {
                for (size_t i = 0; i < Context::kRingFrames; ++i)
                    if (c.ringBuf[i] && c.ringBuf[i].get() == raw) { ibOwned = c.ringBuf[i]; break; }
            } else if (ibKeep) {
                ibOwned.reset();
            }
            ibRaw = raw;
            ioff = roff;
            if (!ibGet() && !ibKeep) { c.appleStats.miscFail++; return false; }
        }
        if (!ibGet() && !ibKeep) { c.appleStats.miscFail++; return false; }
        // Validator index max: quét tối đa 4k index đầu, chỉ khi prog chưa warn
        // (sau khi warn thì bỏ qua để giữ throughput — steady-state 0 scan).
        // Test D (count=3) vẫn warn lần đầu. Release vẫn an toàn vì fault đã
        // chặn bằng fallback + zero-buf, validator chỉ để chẩn đoán.
        if (srcBytes) {
            static std::set<GLuint> warnedIdx;
            if (warnedIdx.size() < 16 && warnedIdx.find(prog) == warnedIdx.end()) {
            uint64_t mx = 0;
            GLsizei scan = count > 4096 ? 4096 : count;
            for (GLsizei k = 0; k < scan; ++k)
                mx = std::max(mx, (elem == 2)
                                        ? (uint64_t)(srcBytes[2 * k] | (srcBytes[2 * k + 1] << 8))
                                        : (uint64_t)(srcBytes[4 * k] | (srcBytes[4 * k + 1] << 8) |
                                                     (srcBytes[4 * k + 2] << 16) |
                                                     (srcBytes[4 * k + 3] << 24)));
            int64_t want = (int64_t)mx + (int64_t)baseVertex;
            if (want >= 0) {
                for (uint32_t ai2 = 0; ai2 < casN; ++ai2) {
                    const metal::CustomAttrib& ca = cas[ai2];
                    if (ca.bufferIndex >= 16 || !(bindMask & (1u << ca.bufferIndex))) continue;
                    if (!ca.stride) continue;
                    auto bbit = c.buffers.find(bindMap[ca.bufferIndex].id);
                    if (bbit == c.buffers.end() || !bbit->second.gpu) continue;
                    size_t base2 = bindMap[ca.bufferIndex].off;
                    size_t gpuLen = bbit->second.gpu->length();
                    size_t elemBytes = (size_t)ca.size * GLTypeSize(ca.type);
                    size_t cap = 0;
                    if (ca.divisor > 0) {
                        cap = gpuLen; // per-instance: fetch nhỏ, bỏ qua
                    } else if (gpuLen > base2 + ca.offset + elemBytes) {
                        cap = (gpuLen - base2 - ca.offset - elemBytes) / ca.stride + 1;
                    }
                    if (!ca.divisor && (uint64_t)want >= cap && warnedIdx.size() < 16 &&
                        warnedIdx.insert(prog).second) {
                        ++c.appleStats.rangeWarn;
                        char b[192];
                        snprintf(b, sizeof(b),
                                 "AppleDrawGL: IDXRANGE prog@%u maxIdx=%llu cap=%zu (count=%d)",
                                 prog, (unsigned long long)want, cap, count);
                        c.LogDebug(0, 0, 0, 0, b);
                        break;
                    }
                }
            }
            }
        }
    }
    // IR deferred: ring buffers sống theo Context (3 frames); fallback newBuffer
    // giữ qua pendingKeep (RingUpload đã push). Giữ thêm ibOwned ring slot.
    std::shared_ptr<metal::IBuffer> ibHold = ibOwned ? ibOwned : ibKeep;
    if (indexed && ibHold) {
        // Ring slot: giữ shared_ptr để sống qua commit async.
        // Fallback: đã trong pendingKeep, giữ thêm ở đây cho chắc.
        bool isRing = false;
        for (size_t i = 0; i < Context::kRingFrames; ++i)
            if (c.ringBuf[i] && ibHold == c.ringBuf[i]) { isRing = true; break; }
        if (!isRing) c.pendingKeep.push_back(ibHold);
    }
    if (indexed) enc->drawIndexed(prim, (uint32_t)count, ity, ibGet() ? ibGet() : ibHold.get(), ioff, (uint32_t)inst);
    else enc->drawPrimitives(prim, (uint32_t)first, (uint32_t)count, (uint32_t)inst);
    // IR: KHÔNG commit mỗi draw. Encoder giữ mở để batch N draw → 1 commit ở flush
    // (ReadPixels/Blit/present/EndFrame/target đổi/glClear). Trước đây mỗi draw =
    // 1 commandBuffer + commit (1 GL → 3 Metal). Giờ N draw cùng pass = 1 encoder.
    // Ngữ cảnh cho FIRST-fault handler bất đồng bộ (A11 ban sau fault hàng loạt).
    if (c.DiagOn()) {
        char b[160];
        snprintf(b, sizeof(b), "prog@%u vao@%u mode=0x%x count=%d idx=%d fbo=%u",
                 prog, vao, mode, count, (int)indexed, c.state.BoundDrawFBO());
        c.device->noteDrawContext(b);
    }
    c.appleStats.drawsEncoded++;
    c.appleStats.progEncoded[prog]++;
    // fboDraw#: ĐẾM draw THẬT mỗi FBO (không qua quota dedup 8/prog) — trả lời
    // "bake atlas có chạy hàng trăm draw không hay chỉ 8 draw". Kèm size encoder
    // đang mở: lệch vs FBO = vẽ vào sai target (nguyên nhân atlas rỗng).
    if (c.DiagOn() && c.state.BoundDrawFBO() != 0) {
        static std::map<std::pair<GLuint, GLuint>, uint64_t> nFbo;
        auto& n = nFbo[{c.state.BoundDrawFBO(), prog}];
        uint64_t cur = ++n;
        if (cur <= 4 || (cur % 256) == 0) {
            uint32_t ew = 0, eh = 0;
            if (c.pendingTarget) { ew = c.pendingTarget->width(); eh = c.pendingTarget->height(); }
            fprintf(stderr,
                    "[TGLMT] fboDraw# fbo=%u tex=%u prog=%u n=%llu enc=%ux%u "
                    "encTex=%u encFbo=%u\n",
                    (unsigned)c.state.BoundDrawFBO(), (unsigned)drawColorTexId,
                    prog, (unsigned long long)cur, ew, eh,
                    (unsigned)c.pendingColorTex, (unsigned)c.pendingDrawFBO);
            fflush(stderr);
        }
    }
    // Chẩn đoán đen màn hình: chỉ khi TGLMT_DIAG=1. Release bỏ qua toàn bộ
    // dump (fprintf + wrapAsTarget readback trong dump = sync stall).
    if (c.DiagOn())
    {
        static std::set<std::tuple<GLuint, GLuint, GLuint>> loggedDraws;
        static std::map<GLuint, int> perProgDrawLogs;
        GLuint tex0 = c.state.BoundTexture(0);
        // Atlasprobe: lần đầu terrain draw (program có ChunkSection) — đọc GPU
        // atlas unit0 (1 lần, ~2MB). Shadow rỗng (shnz=0) KHÔNG chứng minh GPU
        // rỗng; nếu GPU nz=0 → atlas đen = nguyên nhân world đen trực tiếp.
        {
            static bool atlasProbed = false;
            if (!atlasProbed) {
                for (auto& ub : pr.uniformBlocks) {
                    if (ub.name != "ChunkSection") continue;
                    atlasProbed = true;
                    // Đọc MỌI atlas lớn đã từng là render-target (blocks/gui/items)
                    // thay vì chỉ tex0: trả lời "tất cả atlas rỗng hay chỉ 1 cái".
                    int nProbe = 0;
                    for (auto& kv : c.textures) {
                        auto& t = kv.second;
                        if (!t.wasRT || !t.gpu || t.w < 512 || t.h < 512) continue;
                        if (nProbe++ >= 8) break;
                        size_t w = t.w, h = t.h;
                        std::vector<uint8_t> fb(w * h * 4, 0);
                        // Readback KHÔNG flush/commit = thấy dữ liệu cũ → kết luận
                        // sai ("atlas rỗng" oan). Bắt buộc commit trước khi đọc.
                        c.FlushPendingEncoder();
                        c.CommitAndWait();
                        auto wt = c.device->wrapAsTarget(t.gpu.get(), nullptr);
                        if (wt && wt->readback(fb.data(), w * 4)) {
                            // Lưới 2D (không phải stride dọc): stride dọc chỉ chọn
                            // 16 cột → bỏ sót sprite 16px. Lưới ≥16px/cột TRÙM mọi
                            // sprite 16px nên nz mới phản ánh coverage thật.
                            size_t nx = std::min<size_t>(128, w), ny = std::min<size_t>(128, h);
                            size_t nz = 0, tot = 0, bandNz[16] = {0};
                            for (size_t by = 0; by < ny; ++by) {
                                size_t y = by * h / ny;
                                for (size_t bx = 0; bx < nx; ++bx) {
                                    size_t x = bx * w / nx;
                                    const uint8_t* q = fb.data() + (y * w + x) * 4;
                                    ++tot;
                                    if (q[0] | q[1] | q[2] | q[3]) {
                                        ++nz;
                                        bandNz[std::min<size_t>(15, by * 16 / ny)]++;
                                    }
                                }
                            }
                            auto px = [&](size_t x, size_t y, unsigned* o) {
                                const uint8_t* p = fb.data() + (y * w + x) * 4;
                                o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3];
                            };
                            unsigned p00[4], pmid[4], pnn[4];
                            px(0, 0, p00);
                            px(w / 2, h / 2, pmid);
                            px(w - 1, h - 1, pnn);
                            char bands[160];
                            snprintf(bands, sizeof(bands),
                                     "%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu",
                                     bandNz[0], bandNz[1], bandNz[2], bandNz[3], bandNz[4],
                                     bandNz[5], bandNz[6], bandNz[7], bandNz[8], bandNz[9],
                                     bandNz[10], bandNz[11], bandNz[12], bandNz[13],
                                     bandNz[14], bandNz[15]);
                            fprintf(stderr,
                                    "[TGLMT] atlasprobe# tex=%u %zux%zu nz=%zu/%zu "
                                    "px00=(%u,%u,%u,%u) mid=(%u,%u,%u,%u) "
                                    "pxnn=(%u,%u,%u,%u) prog@%u\n"
                                    "[TGLMT] atlasbands# tex=%u b0..b15=%s\n",
                                    kv.first, w, h, nz, tot, p00[0], p00[1], p00[2], p00[3],
                                    pmid[0], pmid[1], pmid[2], pmid[3], pnn[0], pnn[1],
                                    pnn[2], pnn[3], prog, kv.first, bands);
                        } else {
                            fprintf(stderr, "[TGLMT] atlasprobe# tex=%u readback FAIL\n",
                                    kv.first);
                        }
                    }
                    break;
                }
            }
        }
        // Deep probe: 1 lần/MỌI program ở draw đầu — evidence runtime chẩn đoán
        // "block sai texture + sai ánh sáng" (terrain) và GUI sprite (nếu có):
        // sampler map (name→unit→tex + wrap/filter + shadowAvg), readback
        // lightmap 16x16, hexdump UBO đã bind, VAO attrib + data thô vertex.
        {
            static std::set<GLuint> deepProbed;
            if (deepProbed.insert(prog).second) {
                auto hexdump = [&](const uint8_t* p, size_t n) {
                    std::string s;
                    char b[4];
                    for (size_t i = 0; i < n; ++i) {
                        snprintf(b, sizeof(b), "%02X", p[i]);
                        s += b;
                    }
                    return s;
                };
                auto dumpSamp = [&](const char* stage, const std::vector<std::string>& lst) {
                    for (auto& nm : lst) {
                        GLuint unit = 0;
                        auto uit = pr.samplerUnits.find(nm);
                        if (uit != pr.samplerUnits.end()) unit = uit->second;
                        GLuint tx = c.state.BoundTexture(unit, KindTarget(KindOf(pr.samplerKind, nm)));
                        auto tit = c.textures.find(tx);
                        unsigned w = 0, h = 0, wrapS = 0, wrapT = 0, minF = 0, magF = 0;
                        double savg = -1.0;
                        if (tit != c.textures.end()) {
                            auto& t = tit->second;
                            w = t.w; h = t.h;
                            auto g = [&](GLenum k, unsigned d) {
                                auto it2 = t.params.find(k);
                                return it2 == t.params.end() ? d : (unsigned)it2->second;
                            };
                            wrapS = g(0x2802, 0x2901); wrapT = g(0x2803, 0x2901);
                            minF = g(0x2801, 0x2601); magF = g(0x2800, 0x2601);
                            if (!t.pixels.empty()) {
                                size_t pxn = t.pixels.size() / 4;
                                size_t stride = std::max<size_t>(1, pxn / 4096);
                                double sum = 0; size_t cnt = 0;
                                for (size_t i = 0; i + 3 < t.pixels.size(); i += 4 * stride) {
                                    sum += t.pixels[i] + t.pixels[i + 1] + t.pixels[i + 2];
                                    ++cnt;
                                }
                                if (cnt) savg = sum / (cnt * 3.0);
                            }
                        }
                        fprintf(stderr,
                                "[TGLMT] dsamp#%u %s %s unit=%u tex=%u %ux%u "
                                "wrap=(0x%x,0x%x) min=0x%x mag=0x%x shadowAvg=%.1f\n",
                                prog, stage, nm.c_str(), unit, tx, w, h,
                                wrapS, wrapT, minF, magF, savg);
                    }
                };
                dumpSamp("vs", pr.vsSamplers);
                dumpSamp("fs", pr.fsSamplers);
                // Lightmap: texture 16x16 bound → readback GPU content (1 lần).
                {
                    std::set<GLuint> seen;
                    auto rb = [&](const std::vector<std::string>& lst) {
                        for (auto& nm : lst) {
                            auto uit = pr.samplerUnits.find(nm);
                            GLuint unit = uit == pr.samplerUnits.end() ? 0 : uit->second;
                            GLuint tx = c.state.BoundTexture(unit, KindTarget(KindOf(pr.samplerKind, nm)));
                            if (!seen.insert(tx).second) continue;
                            auto tit = c.textures.find(tx);
                            if (tit == c.textures.end() || !tit->second.gpu) continue;
                            if (tit->second.w != 16 || tit->second.h != 16) continue;
                            c.FlushPendingEncoder();
                            c.CommitAndWait();
                            std::vector<uint8_t> fb(16 * 16 * 4, 0);
                            auto wt = c.device->wrapAsTarget(tit->second.gpu.get(), nullptr);
                            if (wt && wt->readback(fb.data(), 16 * 4)) {
                                double avg = 0;
                                for (int i = 0; i < 256; ++i)
                                    avg += fb[i * 4] + fb[i * 4 + 1] + fb[i * 4 + 2];
                                avg /= 256.0 * 3.0;
                                const uint8_t* m = fb.data() + (8 * 16 + 8) * 4;
                                const uint8_t* c15 = fb.data() + (15 * 16 + 15) * 4;
                                fprintf(stderr,
                                        "[TGLMT] lightmaprb# tex=%u avg=%.1f "
                                        "px00=(%u,%u,%u,%u) mid=(%u,%u,%u,%u) c15=(%u,%u,%u,%u)\n",
                                        tx, avg, fb[0], fb[1], fb[2], fb[3],
                                        m[0], m[1], m[2], m[3], c15[0], c15[1], c15[2], c15[3]);
                            } else {
                                fprintf(stderr, "[TGLMT] lightmaprb# tex=%u FAIL\n", tx);
                            }
                        }
                    };
                    rb(pr.vsSamplers);
                    rb(pr.fsSamplers);
                }
                // Hexdump UBO thật đang bind (phát hiện nội dung 0/sai offset).
                for (auto& ub : pr.uniformBlocks) {
                    auto bit = c.uniformBindPoints.find(ub.binding);
                    if (bit == c.uniformBindPoints.end() || !bit->second.buffer) continue;
                    auto bt = c.buffers.find(bit->second.buffer);
                    if (bt == c.buffers.end()) continue;
                    const auto& d = bt->second.data;
                    size_t off = (size_t)bit->second.offset;
                    size_t avail = (off < d.size()) ? d.size() - off : 0;
                    size_t len = std::min<size_t>(avail, std::min<size_t>(96, ub.minSize));
                    if (!len) continue;
                    fprintf(stderr, "[TGLMT] dubo#%u %s point=%u buf=%u off=%zu len=%zu: %s\n",
                            prog, ub.name.c_str(), ub.binding, bit->second.buffer,
                            off, len, hexdump(d.data() + off, len).c_str());
                }
                // VAO: layout attrib + 32 byte đầu (Position/Color/UV0/UV2).
                auto vit = c.vaos.find(vao);
                if (vit != c.vaos.end()) {
                    for (int a = 0; a < 4; ++a) {
                        auto& at = vit->second.attribs[a];
                        fprintf(stderr,
                                "[TGLMT] dvao#%u attr%d en=%d buf=%u size=%d "
                                "type=0x%x norm=%d stride=%d off=%zu rel=%zu\n",
                                prog, a, (int)at.enabled, at.buffer, at.size, at.type,
                                (int)at.normalized, at.stride, at.offset, at.relativeOffset);
                        if (!at.enabled || !at.buffer) continue;
                        auto bt = c.buffers.find(at.buffer);
                        if (bt == c.buffers.end()) continue;
                        const auto& d = bt->second.data;
                        if (at.offset >= d.size()) continue;
                        size_t take = std::min<size_t>(32, d.size() - at.offset);
                        fprintf(stderr, "[TGLMT] dvdat#%u attr%d: %s\n",
                                prog, a, hexdump(d.data() + at.offset, take).c_str());
                    }
                }
                fflush(stderr);
            }
        }
        // Blurread: input chuỗi blur (InSampler) = nội dung TRƯỚC blur. Flush
        // encoder + commit để scene draws đã render xong rồi mới getBytes.
        // Throttle 5s (chuỗi blur 6 pass cùng frame, không cần probe mỗi pass).
        {
            auto iu = pr.samplerUnits.find("InSampler");
            if (iu != pr.samplerUnits.end()) {
                static uint64_t lastMs = 0;
                static bool firstB = true;
                uint64_t nowMs =
                    (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
                if (firstB || nowMs - lastMs >= 5000) {
                    firstB = false;
                    lastMs = nowMs;
                    GLuint btex = c.state.BoundTexture(iu->second,
                                                    KindTarget(KindOf(pr.samplerKind, "InSampler")));
                    auto tit = c.textures.find(btex);
                    if (tit != c.textures.end() && tit->second.gpu && tit->second.w > 0 &&
                        tit->second.h > 0) {
                        c.FlushPendingEncoder();
                        c.CommitAndWait();
                        size_t w = tit->second.w, h = tit->second.h;
                        std::vector<uint8_t> fb(w * h * 4, 0);
                        auto wt = c.device->wrapAsTarget(tit->second.gpu.get(), nullptr);
                        if (wt && wt->readback(fb.data(), w * 4)) {
                            size_t stride = std::max<size_t>(1, (w * h) / 8192), nz = 0,
                                                                                     tot = 0;
                            for (size_t i = 0; i < w * h; i += stride) {
                                const uint8_t* q = fb.data() + i * 4;
                                if (q[0] | q[1] | q[2] | q[3]) ++nz;
                                ++tot;
                            }
                            auto px = [&](size_t x, size_t y, unsigned* o) {
                                const uint8_t* p = fb.data() + (y * w + x) * 4;
                                o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3];
                            };
                            unsigned top[4], ctr[4];
                            px(w / 2, h / 8, top);
                            px(w / 2, h / 2, ctr);
                            fprintf(stderr,
                                    "[TGLMT] blurread# tex=%u %zux%zu nz=%zu/%zu "
                                    "top=(%u,%u,%u,%u) ctr=(%u,%u,%u,%u)\n",
                                    btex, w, h, nz, tot, top[0], top[1], top[2], top[3],
                                    ctr[0], ctr[1], ctr[2], ctr[3]);
                        }
                    }
                }
            }
        }
        // IconAtlas probe: texture ĐANG SAMPLE từng LÀ render-target (GuiItemAtlas
        // bake → content chỉ có trên GPU, shadow CPU luôn rỗng). Throttle theo
        // TỪNG texture (mỗi tex ≥4s, tổng cap 6 — bản scis7 cap-3 global bị
        // tex cũ ăn hết nên atlas mới không bao giờ được đọc). Kèm band profile
        // 16 hàng ngang: content nằm top/bottom → lộ bake bị lật hoặc trống.
        {
            static int nIcon = 0;
            static std::map<GLuint, uint64_t> lastIco;
            GLuint icoTex = 0;
            auto checkRT = [&](const std::vector<std::string>& lst) {
                if (icoTex) return;
                for (auto& nm : lst) {
                    auto uit = pr.samplerUnits.find(nm);
                    GLuint unit = uit == pr.samplerUnits.end() ? 0 : uit->second;
                    GLuint tx = c.state.BoundTexture(unit, KindTarget(KindOf(pr.samplerKind, nm)));
                    auto tit = c.textures.find(tx);
                    if (tit == c.textures.end() || !tit->second.wasRT) continue;
                    if (tit->second.gpu && tit->second.w >= 512 && tit->second.h >= 512 &&
                        tit->second.w == tit->second.h) {
                        icoTex = tx;
                        return;
                    }
                }
            };
            checkRT(pr.fsSamplers);
            checkRT(pr.vsSamplers);
            if (icoTex && nIcon < 16) {
                uint64_t nowMs = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch())
                                     .count();
                auto lit = lastIco.find(icoTex);
                if (lit == lastIco.end() || nowMs - lit->second >= 4000) {
                    lastIco[icoTex] = nowMs;
                    ++nIcon;
                    auto tit = c.textures.find(icoTex);
                    size_t w = tit->second.w, h = tit->second.h;
                    c.FlushPendingEncoder();
                    c.CommitAndWait();
                    std::vector<uint8_t> fb(w * h * 4, 0);
                    auto wt = c.device->wrapAsTarget(tit->second.gpu.get(), nullptr);
                    if (wt && wt->readback(fb.data(), w * 4)) {
                        size_t stride = std::max<size_t>(1, (w * h) / 8192), nz = 0, tot = 0;
                        size_t bandNz[16] = {0};
                        for (size_t i = 0; i < w * h; i += stride) {
                            const uint8_t* q = fb.data() + i * 4;
                            size_t y = i / w;
                            size_t b = std::min<size_t>(15, y * 16 / h);
                            if (q[0] | q[1] | q[2] | q[3]) {
                                ++nz;
                                ++bandNz[b];
                            }
                            ++tot;
                        }
                        auto px = [&](size_t x, size_t y, unsigned* o) {
                            const uint8_t* p = fb.data() + (y * w + x) * 4;
                            o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3];
                        };
                        unsigned p00[4], pmid[4], pend[4];
                        px(0, 0, p00);
                        px(w / 2, h / 2, pmid);
                        px(w - 1, h - 1, pend);
                        char bands[128];
                        snprintf(bands, sizeof(bands),
                                 "%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%zu",
                                 bandNz[0], bandNz[1], bandNz[2], bandNz[3], bandNz[4],
                                 bandNz[5], bandNz[6], bandNz[7], bandNz[8], bandNz[9],
                                 bandNz[10], bandNz[11], bandNz[12], bandNz[13], bandNz[14],
                                 bandNz[15]);
                        fprintf(stderr,
                                "[TGLMT] iconatlas# n=%d tex=%u %zux%zu nz=%zu/%zu "
                                "px00=(%u,%u,%u,%u) mid=(%u,%u,%u,%u) "
                                "end=(%u,%u,%u,%u) prog@%u\n"
                                "[TGLMT] iconatlasbands# tex=%u b0..b15(top→bot)=%s\n",
                                nIcon, icoTex, w, h, nz, tot, p00[0], p00[1], p00[2],
                                p00[3], pmid[0], pmid[1], pmid[2], pmid[3], pend[0],
                                pend[1], pend[2], pend[3], prog, icoTex, bands);
                    } else {
                        fprintf(stderr, "[TGLMT] iconatlas# tex=%u FAIL prog@%u\n", icoTex, prog);
                    }
                }
            }
        }
        auto key = std::make_tuple(prog, vao, tex0);
        // Quota per-prog ≤8 key + tổng 384: trước đây cap 96 global bị
        // prog@1091 sprite (bake) ăn 94/96 trước f600 → không thấy draw in-world.
        bool allow = loggedDraws.count(key) == 0 &&
                     perProgDrawLogs[prog] < 8 && loggedDraws.size() < 384;
        if (allow) {
            loggedDraws.insert(key);
            ++perProgDrawLogs[prog];
            auto hex = [](const uint8_t* d, size_t n) {
                static const char* H = "0123456789ABCDEF";
                std::string s;
                for (size_t i = 0; i < n; ++i) {
                    s += H[(d[i] >> 4) & 15];
                    s += H[d[i] & 15];
                }
                return s;
            };
            fprintf(stderr, "[TGLMT] draw prog@%u vao@%u mode=0x%x count=%d indexed=%d inst=%d\n",
                    prog, vao, mode, count, (int)indexed, inst);
            // FBO đích + texture đích (so feedback: sample đúng texture đang
            // render là hazard trên TBDR).
            {
                GLuint dfbo = c.state.BoundDrawFBO();
                GLuint dtex = 0;
                if (dfbo != 0) {
                    auto fit = c.fbos.find(dfbo);
                    if (fit != c.fbos.end()) {
                        auto cit = fit->second.colorTex.find(0);
                        if (cit != fit->second.colorTex.end()) dtex = cit->second;
                    }
                }
                fprintf(stderr, "[TGLMT]   target fbo=%u tex=%u (0 = default/màn hình)\n",
                        dfbo, dtex);
            }
            // Index/EBO/baseVertex/first: draw indexed sai ở đây là đen toàn bộ
            // mà không error nào (indices rác → degenerate).
            int64_t firstVtxApplied = first;
            // fixico8: đỉnh GIỮA draw (phát hiện batch trộn nhiều slot/entity
            // vào 1 draw — scissor chỉ giữ slot cuối → bake các slot kia mất).
            int64_t midVtxApplied =
                indexed ? -1 : (int64_t)first + (count > 0 ? (GLsizei)(count / 2) : 0);
            {
                GLint bv = baseVertex, fst = first;
                GLuint ebo = eboId;
                fprintf(stderr, "[TGLMT]   idx baseVertex=%d first=%d ebo=%u idxOff=%zu type=0x%x\n",
                        bv, fst, ebo, indexByteOff, indexType);
                if (indexed && (indexType == 0x1403 || indexType == 0x1405)) {
                    size_t elem = (indexType == 0x1403) ? 2 : 4;
                    const uint8_t* srcBytes = nullptr;
                    size_t avail = 0;
                    auto bit = c.buffers.find(ebo);
                    if (bit != c.buffers.end()) {
                        if (bit->second.gpu &&
                            indexByteOff < bit->second.gpu->length()) {
                            srcBytes = (const uint8_t*)bit->second.gpu->contents() + indexByteOff;
                            avail = bit->second.gpu->length() - indexByteOff;
                        } else if (indexByteOff < bit->second.data.size()) {
                            srcBytes = bit->second.data.data() + indexByteOff;
                            avail = bit->second.data.size() - indexByteOff;
                        }
                    } else if (indexData && ebo == 0) {
                        srcBytes = (const uint8_t*)indexData;
                        avail = (size_t)count * elem;
                    }
                    std::string idxs;
                    for (GLsizei k = 0; k < count && k < 6; ++k) {
                        if (!srcBytes || (size_t)(k + 1) * elem > avail) {
                            idxs += "? ";
                            continue;
                        }
                        int64_t v = (elem == 2)
                                        ? (int64_t)(srcBytes[2 * k] | (srcBytes[2 * k + 1] << 8))
                                        : (int64_t)(srcBytes[4 * k] | (srcBytes[4 * k + 1] << 8) |
                                                    (srcBytes[4 * k + 2] << 16) |
                                                    (srcBytes[4 * k + 3] << 24));
                        if (k == 0) firstVtxApplied = v + (int64_t)bv;
                        idxs += std::to_string(v + (int64_t)bv) + " ";
                    }
                    fprintf(stderr, "[TGLMT]   idx first6(base applied)=[%s]\n", idxs.c_str());
                    if (srcBytes && count > 1) {
                        size_t mk = (size_t)count / 2;
                        if ((mk + 1) * elem <= avail) {
                            int64_t v = (elem == 2)
                                            ? (int64_t)(srcBytes[2 * mk] |
                                                        (srcBytes[2 * mk + 1] << 8))
                                            : (int64_t)(srcBytes[4 * mk] |
                                                        (srcBytes[4 * mk + 1] << 8) |
                                                        (srcBytes[4 * mk + 2] << 16) |
                                                        (srcBytes[4 * mk + 3] << 24));
                            midVtxApplied = v + (int64_t)bv;
                        }
                    }
                }
            }
            // Attributes: tên (tra từ attribLoc), format, binding/rel/stride,
            // buffer + 24 byte đầu đỉnh 0 (đọc từ GPU = sự thật card thấy).
            for (int i = 0; i < 16; ++i) {
                const VertexAttrib& a = v.attribs[i];
                if (!a.enabled) continue;
                std::string nm = "?";
                for (auto& kv : pr.attribLoc)
                    if ((int)kv.second == i) {
                        nm = kv.first;
                        break;
                    }
                size_t base = 0;
                if (a.binding < v.bindings.size() && v.bindings[a.binding].offset > 0)
                    base = (size_t)v.bindings[a.binding].offset;
                std::string bytes = "nogpu";
                const uint8_t* fptr = nullptr;
                size_t favail = 0;
                auto bbit = c.buffers.find(a.buffer);
                if (bbit != c.buffers.end()) {
                    size_t avail = 0;
                    const uint8_t* ptr = nullptr;
                    if (bbit->second.gpu && bbit->second.gpu->length() > base) {
                        ptr = (const uint8_t*)bbit->second.gpu->contents() + base;
                        avail = bbit->second.gpu->length() - base;
                    } else if (bbit->second.data.size() > base) {
                        ptr = bbit->second.data.data() + base;
                        avail = bbit->second.data.size() - base;
                    }
                    if (ptr) bytes = hex(ptr, std::min<size_t>(avail, 24));
                    if (ptr && a.relativeOffset < avail) {
                        fptr = ptr + a.relativeOffset;
                        favail = avail - a.relativeOffset;
                    }
                }
                // Probe geo7: Position FLOAT → decode thực (hex đọc nhầm màu áo
                // thành "garbage"). f0 = đỉnh 0, fN = đỉnh ĐẦU TIÊN draw lấy
                // (firstVtxApplied), fM = đỉnh GIỮA draw — soi trực tiếp
                // geometry bị flatten/phẳng/trộn batch.
                std::string fdec;
                if (fptr && nm == "Position" && a.type == 0x1406 && a.size >= 3) {
                    auto fv = [&](size_t byteOff, const char* lbl) {
                        if (byteOff + (size_t)a.size * 4 > favail) return;
                        const float* fp = (const float*)(fptr + byteOff);
                        char b2[96];
                        snprintf(b2, sizeof(b2), " %s=[%.5g,%.5g,%.5g]", lbl, fp[0], fp[1],
                                 a.size >= 3 ? fp[2] : 0.0f);
                        fdec += b2;
                    };
                    fv(0, "f0");
                    if (firstVtxApplied >= 0 && a.stride > 0)
                        fv((size_t)firstVtxApplied * (size_t)a.stride, "fN");
                    if (midVtxApplied >= 0 && midVtxApplied != firstVtxApplied && a.stride > 0)
                        fv((size_t)midVtxApplied * (size_t)a.stride, "fM");
                } else if (fptr && nm == "UV0" && a.type == 0x1406 && a.size >= 2) {
                    // fixico8: UV blit icon — soi slot atlas mà quad SAMPLE
                    // (uv0/uvN phải nằm trong slot [u0,u1]x[v1,v0] của item).
                    auto fu = [&](size_t byteOff, const char* lbl) {
                        if (byteOff + 8 > favail) return;
                        const float* fp = (const float*)(fptr + byteOff);
                        char b2[64];
                        snprintf(b2, sizeof(b2), " %s=[%.4g,%.4g]", lbl, fp[0], fp[1]);
                        fdec += b2;
                    };
                    fu(0, "uv0");
                    if (firstVtxApplied >= 0 && a.stride > 0)
                        fu((size_t)firstVtxApplied * (size_t)a.stride, "uvN");
                }
                fprintf(stderr,
                        "[TGLMT]   attr slot%d=%s %dx0x%x%s bind=%u rel=%zu stride=%d "
                        "buf=%u base=%zu v0=[%s]%s\n",
                        i, nm.c_str(), a.size, a.type, a.normalized ? "N" : "", a.binding,
                        a.relativeOffset, a.stride, a.buffer, base, bytes.c_str(),
                        fdec.c_str());
            }
            // UBO per-stage (vsSlots/fsSlots khớp [[buffer(17+bi)]] MSL).
            auto slotOf = [&](const std::vector<std::string>& lst,
                              const std::string& nm) -> int {
                for (size_t i = 0; i < lst.size(); ++i)
                    if (lst[i] == nm) return 17 + (int)i;
                return -1;
            };
            for (auto& b : pr.uniformBlocks) {
                int vsSlot = slotOf(pr.vsBlocks, b.name);
                int fsSlot = slotOf(pr.fsBlocks, b.name);
                auto bit = c.uniformBindPoints.find(b.binding);
                if (bit == c.uniformBindPoints.end() || !bit->second.buffer) {
                    fprintf(stderr, "[TGLMT]   ubo %s: UNBOUND (binding %u vsSlot=%d fsSlot=%d)\n",
                            b.name.c_str(), b.binding, vsSlot, fsSlot);
                    continue;
                }
                auto t = c.buffers.find(bit->second.buffer);
                size_t off = (bit->second.offset > 0) ? (size_t)bit->second.offset : 0;
                size_t have =
                    (t == c.buffers.end() || off >= t->second.data.size())
                        ? 0
                        : t->second.data.size() - off;
                if (!have) {
                    fprintf(stderr, "[TGLMT]   ubo %s: NODATA (have 0B)\n", b.name.c_str());
                    continue;
                }
                const float* m = (const float*)(t->second.data.data() + off);
                size_t nf = have / 4;
                auto F = [&](size_t k) { return k < nf ? m[k] : 0.0f; };
                // Full vec4 đầu (11 vec4 = 176B: ModelViewMat + ColorModulator +
                // ModelOffset + TextureMat cho DynamicTransforms): ColorModulator
                // = 0 là đen toàn bộ menu dù matrices đúng (mọi FS đều nhân nó).
                int nvec = (int)std::min<size_t>(have / 16, 11);
                std::string full;
                char fb[48];
                for (int vi = 0; vi < nvec; ++vi) {
                    snprintf(fb, sizeof(fb), "%s(%.4g,%.4g,%.4g,%.4g)", vi ? " " : "", F(vi * 4),
                             F(vi * 4 + 1), F(vi * 4 + 2), F(vi * 4 + 3));
                    full += fb;
                }
                fprintf(stderr,
                        "[TGLMT]   ubo %s (bindpt %u vsSlot=%d fsSlot=%d, buf %u+%zu, have %zuB need %zuB): "
                        "diag=(%g,%g,%g,%g) row0=(%g,%g,%g,%g)\n[TGLMT]     full=[%s]\n",
                        b.name.c_str(), b.binding, vsSlot, fsSlot, bit->second.buffer, off,
                        have, b.minSize, F(0), F(5),
                        F(10), F(15), F(0), F(1), F(2), F(3), full.c_str());
            }
            // Texture theo sampler (unit, id, WxH, format, gpu?, pixel đầu).
            auto dumpSamp = [&](const std::string& name, bool isVS) {
                GLuint unit = 0;
                auto uit = pr.samplerUnits.find(name);
                if (uit != pr.samplerUnits.end()) unit = uit->second;
                GLuint texId = c.state.BoundTexture(unit, KindTarget(KindOf(pr.samplerKind, name)));
                auto tit = c.textures.find(texId);
                if (tit == c.textures.end()) {
                    fprintf(stderr, "[TGLMT]   samp %s: unit=%u tex#%u MISSING\n", name.c_str(),
                            unit, texId);
                    return;
                }
                auto& tx = tit->second;
                // Minfilter thực (quyết định mipmap completeness): sampler object
                // đã bind, else texture params, else default GL (NEAREST_MIPMAP).
                uint32_t minF = 0x2601, magF = 0x2601;
                {
                    GLuint sid = c.state.BoundSampler(unit);
                    auto sit = c.samplers.find(sid);
                    const std::unordered_map<GLenum, GLint>* pp = &tx.params;
                    if (sid && sit != c.samplers.end()) pp = &sit->second.iparams;
                    auto g = [&](GLenum k, uint32_t d) {
                        auto it = pp->find(k);
                        return it == pp->end() ? d : (uint32_t)it->second;
                    };
                    minF = g(0x2801, minF);
                    magF = g(0x2800, magF);
                }
                std::string px0 = "none";
                if (!tx.pixels.empty() && tx.pixels.size() >= 4) {
                    char b[32];
                    snprintf(b, sizeof(b), "%02X%02X%02X%02X", tx.pixels[0], tx.pixels[1],
                             tx.pixels[2], tx.pixels[3]);
                    px0 = std::string("shadow") + b;
                }
                // Occupancy shadow: tỉ lệ pixel khác 0 (sample ≤8K) — "atlas rỗng
                // → FS discard → sprite/nút biến mất" trả lời ngay tại draw này.
                std::string occ;
                if (!tx.pixels.empty() && tx.w && tx.h) {
                    size_t npx = (size_t)tx.w * tx.h;
                    size_t bppS = tx.pixels.size() / npx;
                    if (bppS) {
                        size_t stride = std::max<size_t>(1, npx / 8192);
                        size_t nz = 0, tot = 0;
                        for (size_t i = 0; i < npx; i += stride) {
                            const uint8_t* q = tx.pixels.data() + i * bppS;
                            bool any = false;
                            for (size_t b = 0; b < bppS; ++b)
                                if (q[b]) { any = true; break; }
                            nz += any;
                            ++tot;
                        }
                        char ob[48];
                        snprintf(ob, sizeof(ob), " shnz=%zu/%zu", nz, tot);
                        occ = ob;
                    } else {
                        occ = " shnz=short";
                    }
                }
                // Ground truth GPU cho texture vừa/nhỏ (atlas lớn đọc tốn RAM).
                // 512 (logo/widgets/menu) đọc 1 lần/combo để soi nội dung thật.
                if (tx.gpu && tx.w <= 512 && tx.h <= 512 && tx.w > 0 && tx.h > 0) {
                    auto wt = c.device->wrapAsTarget(tx.gpu.get(), nullptr);
                    if (wt) {
                        std::vector<uint8_t> tb((size_t)tx.w * tx.h * 4, 0);
                        if (wt->readback(tb.data(), (size_t)tx.w * 4)) {
                            char b[32];
                            snprintf(b, sizeof(b), "gpu%02X%02X%02X%02X", tb[0], tb[1], tb[2],
                                     tb[3]);
                            px0 += std::string("+") + b;
                            // gpunz: 0 = GPU trỏng/trắng trong suốt dù shadow có data
                            // (upload không tới card → bug texture path).
                            size_t npx = (size_t)tx.w * tx.h;
                            size_t stride = std::max<size_t>(1, npx / 8192);
                            size_t nz = 0, tot = 0;
                            for (size_t i = 0; i < npx; i += stride) {
                                const uint8_t* q = tb.data() + i * 4;
                                bool any = q[0] | q[1] | q[2] | q[3];
                                nz += any;
                                ++tot;
                            }
                            char ob[48];
                            snprintf(ob, sizeof(ob), " gpunz=%zu/%zu", nz, tot);
                            occ += ob;
                        }
                    }
                }
                fprintf(stderr,
                        "[TGLMT]   samp %s (%s): unit=%u tex=%u %ux%u fmt=0x%x gpu=%d "
                        "min=0x%x mag=0x%x lv=%d tgt=0x%x px0=%s%s\n",
                        name.c_str(), isVS ? "vs" : "fs", unit, texId, tx.w, tx.h,
                        tx.internalFormat, (int)(tx.gpu != nullptr), minF, magF, tx.levels,
                        tx.target, px0.c_str(), occ.c_str());
            };
            for (auto& s : pr.vsSamplers) dumpSamp(s, true);
            for (auto& s : pr.fsSamplers) dumpSamp(s, false);
            // State raster.
            {
                float bc[4];
                c.state.GetBlendColor(bc);
                auto sc = c.state.GetScissor();
                ViewportState vp0 = c.state.GetViewport(0);
                uint32_t rtW = target ? target->width() : 0, rtH = target ? target->height() : 0;
                uint32_t ff = c.state.FrontFace();
                if (c.state.ClipOrigin() != 0x8CA2)
                    ff = (ff == 0x0900) ? 0x0901u : 0x0900u;
                fprintf(stderr,
                        "[TGLMT]   state blend=%d cull=%d/%d depth=%d scissor=%d[%d,%d,%d,%d] "
                        "clear=(%.2f,%.2f,%.2f,%.2f) "
                        "vp=(%.0f,%.0f,%.0fx%.0f) rt=%ux%u frontF=0x%x mask=%d\n",
                        (int)c.state.IsEnabled(0x0BE2), (int)c.state.IsEnabled(0x0B44),
                        c.state.CullMode(), (int)c.state.IsEnabled(0x0B71),
                        (int)c.state.IsEnabled(0x0C11), sc.x, sc.y, sc.w, sc.h, c.clearColor[0],
                        c.clearColor[1], c.clearColor[2], c.clearColor[3],
                        vp0.x, vp0.y, vp0.w, vp0.h, rtW, rtH, ff,
                        (int)c.state.ColorMask(0));
            }
            fflush(stderr);
        }
    } else {
        ++c.appleStats.diagSkipped;
    }
    return true;
}

} // namespace tglmt
