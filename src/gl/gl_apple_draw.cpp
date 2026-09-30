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
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace tglmt {

static std::shared_ptr<metal::IBuffer> TempUpload(Context& c, const void* data, size_t n) {
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
    float aniso = 1.0f;
    auto fromParams = [&](const std::unordered_map<GLenum, GLint>& p) {
        auto g = [&](GLenum k, uint32_t d) {
            auto it = p.find(k);
            return it == p.end() ? d : (uint32_t)it->second;
        };
        minF = g(0x2801, minF); magF = g(0x2800, magF);
        sW = g(0x2802, sW); tW = g(0x2803, tW);
    };
    uint64_t key = 0;
    auto sit = c.samplers.find(sid);
    if (sid && sit != c.samplers.end()) {
        fromParams(sit->second.iparams);
        auto ff = sit->second.fparams.find(0x84FE); // TEXTURE_MAX_ANISOTROPY
        if (ff != sit->second.fparams.end()) aniso = ff->second;
        key = ((uint64_t)0x53414D50u << 32) | sid; // 'SAMP'
    } else {
        fromParams(tex.params);
        key = ((uint64_t)tex.id << 32) | 0x54455800u; // 'TEX\0'
        key ^= ((uint64_t)minF << 48) ^ ((uint64_t)magF << 52);
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
    auto s = c.device->makeSampler(d);
    if (s) cache[key] = s;
    return s;
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
    std::vector<metal::CustomAttrib> cas;
    std::map<GLuint, std::pair<std::shared_ptr<metal::IBuffer>, size_t>> bindMap;
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
        cas.push_back(ca);
        if (!bindMap.count(a.binding))
            bindMap[a.binding] = {bit->second.gpu, base};
    }
    if (cas.empty()) {
        // Draw không attribute (screenquad suy từ vertex_id): pipeline descriptor
        // rỗng, không bind vertex buffer. Không phải lỗi (trước đây noPipeline oan).
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: attributeless draw (vertex_id)");
    }
    for (auto& ca : cas) {
        // stride 0 = tightly packed theo chính attribute (spec §10.3.1), không
        // cộng offset (bug cũ cộng relative vào stride làm đỉnh thưa sai).
        if (ca.stride == 0) ca.stride = ca.size * GLTypeSize(ca.type);
    }
    // Validator đọc vượt buffer (TBDR A11 fault → SubmissionsIgnored + đứng
    // hình, trong khi desktop chỉ ra rác): chỉ LOG + đếm, không chặn draw.
    if (!indexed) {
        static std::set<std::pair<GLuint, int>> warnedRange;
        for (auto& ca : cas) {
            auto bit = bindMap.find(ca.bufferIndex);
            if (bit == bindMap.end() || !bit->second.first) continue;
            size_t gpuLen = bit->second.first->length();
            size_t elemBytes = (size_t)ca.size * GLTypeSize(ca.type);
            size_t lastN = ca.divisor > 0 ? (inst > 0 ? (size_t)inst - 1 : 0)
                                          : (size_t)first + (size_t)(count > 0 ? count - 1 : 0);
            size_t fetchEnd = bit->second.second + ca.offset + lastN * ca.stride + elemBytes;
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
        metal::ITexture* dep = nullptr;
        if (fit->second.depthTex) {
            auto dit = c.textures.find(fit->second.depthTex);
            if (dit != c.textures.end() && dit->second.gpu) {
                dep = dit->second.gpu.get();
                hasDepthTex = true;
            }
        }
        target = c.device->wrapAsTarget(tit->second.gpu.get(), dep);
        if (!target) { c.appleStats.noTarget++; return false; }
    }
    // 3b. IR staging flush (Deferred Resource Translation):
    // Buffer/texture SubData đã stage (chưa lên GPU) phải flush trước encode,
    // nếu không GPU stale (đen/mất hình). Flush ALL (không chỉ buffer/texture
    // draw này dùng) để bịt lỗ legacy path (program sampler list rỗng) và
    // vertex-texture ngoài danh sách — chi phí tương đương (pending map thường
    // chỉ vài entry, merge trong từng buffer/texture vẫn giữ nguyên).
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    // 4. Pipeline: depth khi depthTest bật (+ có depth thật), blend khi BLEND bật
    metal::PipelineOpts opts;
    opts.depth = c.state.IsEnabled(0x0B71) && (c.state.BoundDrawFBO() == 0 ? false : hasDepthTex);
    opts.blend = c.state.IsEnabled(0x0BE2);
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
        want.stride = cas.empty() ? 0 : cas[0].stride;
        want.depth = opts.depth;
        want.blend = opts.blend;
        if (opts.blend) want.blend0 = opts.blend0;
        want.attribs.assign(cas.begin(), cas.end());
        if (c.pendingPipeValid && c.cachedPipe && want == c.pendingPipeKey) {
            pipe = c.cachedPipe;
            c.appleStats.pipelineLookupSkipped++;
        } else {
            c.appleStats.pipelineLookups++;
            pipe = c.device->makeCustomPipeline(pr.appleVS.get(), "TGLMT_vs", pr.appleFS.get(),
                                                "TGLMT_fs", pipeFmt,
                                                cas.empty() ? nullptr : cas.data(),
                                                (uint32_t)cas.size(),
                                                cas.empty() ? 0 : cas[0].stride, &opts);
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
            if (!enc) { c.appleStats.miscFail++; return false; }
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
        // Viewport chưa set (w=0) → fullscreen target
        if (vp0.w <= 0 || vp0.h <= 0) {
            mvp = metal::Viewport{0, 0, (double)target->width(), (double)target->height(), 0, 1};
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
            // Tắt scissor sau khi đã bật: Metal không có disable trực tiếp trong pass
            // này (scissor luôn áp khi set). Ghi nhận để pass sau không mang state cũ:
            // flush shadow (encoder hiện tại đã qua draw không scissor nên đúng).
            c.pendingScissorEnabled = false;
            c.pendingScissorValid = false;
        }
    }
    // Cull / fillMode / blendColor / depthState — dirty-check từng món.
    {
        bool cullOn = c.state.IsEnabled(0x0B44);
        uint32_t cullM = c.state.CullMode(), frontF = c.state.FrontFace();
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
    // Vertex buffers theo binding
    for (auto& kv : bindMap) enc->setVertexBuffer(kv.second.first.get(), kv.second.second, kv.first);
    // Uniforms tách VS/FS — IR staging cache: nếu bytes giống lần trước cùng program
    // thì tái dùng buffer cũ (0 Metal alloc), tránh newBufferWithBytes mỗi draw.
    // Trước đây: mỗi draw = 2× newBufferWithBytes (vs+fs) dù uniforms không đổi.
    std::shared_ptr<metal::IBuffer> vsUbuf, fsUbuf;
    {
        size_t vsSize = pr.vsUBSize ? pr.vsUBSize : 16;
        size_t fsSize = pr.fsUBSize ? pr.fsUBSize : 16;
        std::vector<uint8_t> vsb(vsSize, 0), fsb(fsSize, 0);
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
            vsUbuf = uc.vsBuf;
            fsUbuf = uc.fsBuf;
            c.appleStats.uniformReused++;
        } else {
            // Chỉ upload stage đã đổi (stage còn lại tái dùng nếu giống).
            if (sameVS) {
                vsUbuf = uc.vsBuf;
                c.appleStats.uniformReused++;
            } else {
                vsUbuf = TempUpload(c, vsb.data(), vsb.size());
                if (vsUbuf) { uc.vsBytes = vsb; uc.vsBuf = vsUbuf; }
            }
            if (sameFS) {
                fsUbuf = uc.fsBuf;
                c.appleStats.uniformReused++;
            } else {
                fsUbuf = TempUpload(c, fsb.data(), fsb.size());
                if (fsUbuf) { uc.fsBytes = fsb; uc.fsBuf = fsUbuf; }
            }
            uc.prog = prog;
            uc.valid = (vsUbuf && fsUbuf);
            // Giữ buffers sống đến flush (encoder deferred).
            if (vsUbuf) c.pendingKeep.push_back(vsUbuf);
            if (fsUbuf && fsUbuf != vsUbuf) c.pendingKeep.push_back(fsUbuf);
        }
        if (!vsUbuf || !fsUbuf) { c.appleStats.miscFail++; return false; }
        enc->setVertexBuffer(vsUbuf.get(), 0, 16);
        enc->setFragmentBuffer(fsUbuf.get(), 0, 16);
    }
    // UBO read-only (vanilla 1.17+/Sodium): bind PER-STAGE theo thứ tự khai báo
    // của stage đó (khớp [[buffer(17+bi)]] trong MSL: bi = index trong stage).
    // Block → (tên → bindingPoint từ glUniformBlockBinding) → GL buffer
    // (BindBufferBase/Range). Gộp thứ tự vs-trước bind chung cả 2 stage sẽ lệch
    // khi vs/fs khai báo khác nhau (post blur) → đọc nhầm buffer → treo GPU.
    // Block thiếu buffer → bind zero fallback (đúng hơn fault GPU).
    std::vector<std::shared_ptr<metal::IBuffer>> uboKeep;
    {
        auto bindZeroV = [&](int s) {
            auto z = FallbackZeroBuf(c);
            if (!z) return;
            uboKeep.push_back(z);
            enc->setVertexBuffer(z.get(), 0, (uint32_t)s);
        };
        auto bindZeroF = [&](int s) {
            auto z = FallbackZeroBuf(c);
            if (!z) return;
            uboKeep.push_back(z);
            enc->setFragmentBuffer(z.get(), 0, (uint32_t)s);
        };
        auto uploadBlock = [&](const std::string& nm) -> std::shared_ptr<metal::IBuffer> {
            GLuint point = 0;
            size_t need = 0;
            for (auto& b : pr.uniformBlocks)
                if (b.name == nm) { point = b.binding; need = b.minSize; break; }
            auto bit = c.uniformBindPoints.find(point);
            if (bit == c.uniformBindPoints.end() || !bit->second.buffer) {
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " unbound, zero fallback");
                return nullptr;
            }
            auto t = c.buffers.find(bit->second.buffer);
            if (t == c.buffers.end() || t->second.data.empty()) {
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " nodata, zero fallback");
                return nullptr;
            }
            size_t off = (size_t)bit->second.offset;
            if (off >= t->second.data.size()) {
                c.LogDebug(0, 0, 0, 0, "AppleDrawGL: UBO " + nm + " offset vuot, zero fallback");
                return nullptr;
            }
            size_t len = bit->second.size ? (size_t)bit->second.size
                                          : t->second.data.size() - off;
            len = std::min(len, t->second.data.size() - off);
            if (!len) return nullptr;
            // Buffer thiếu so với struct shader cần (misbound: vd Globals đọc
            // nhầm buffer SamplerInfo 16B trong khi struct 56B) → đọc OOB hoặc
            // rác điều khiển loop (MenuBlurRadius khổng lồ → treo GPU → iOS ban
            // submissions, đen + đứng hình). Zero fallback giữ GPU sống (GL coi
            // là undefined, blur thành sharp còn hơn fault).
            if (need && len < need) {
                ++c.appleStats.uboSmall;
                static std::set<std::string> loggedSmall;
                if (loggedSmall.size() < 16 && loggedSmall.insert(nm).second) {
                    char b[160];
                    snprintf(b, sizeof(b),
                             "AppleDrawGL: UBO %s small %zuB < need %zuB, zero fallback",
                             nm.c_str(), len, need);
                    c.LogDebug(0, 0, 0, 0, b);
                }
                return nullptr;
            }
            // Pad 0 lên bội số 16 (std140 pad; A11 TBDR nghiêm OOB).
            size_t padded = (len + 15) & ~((size_t)15);
            std::vector<uint8_t> ubPad(padded, 0);
            memcpy(ubPad.data(), t->second.data.data() + off, len);
            auto ub = TempUpload(c, ubPad.data(), padded);
            if (ub) {
                uboKeep.push_back(ub);
                // IR deferred: encoder chưa commit → giữ buffer đến flush.
                c.pendingKeep.push_back(ub);
            }
            return ub;
        };
        // Tương thích ngược: program link trước khi có vsBlocks/fsBlocks
        // (link cũ) → vsBlocks/fsBlocks rỗng → dùng merged order cho cả 2 stage.
        bool legacy = pr.vsBlocks.empty() && pr.fsBlocks.empty();
        if (legacy) {
            int slot = 17;
            for (auto& b : pr.uniformBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(b.name);
                if (!ub) {
                    auto z = FallbackZeroBuf(c);
                    if (z) {
                        uboKeep.push_back(z);
                        enc->setVertexBuffer(z.get(), 0, (uint32_t)slot);
                        enc->setFragmentBuffer(z.get(), 0, (uint32_t)slot);
                    }
                } else {
                    enc->setVertexBuffer(ub.get(), 0, (uint32_t)slot);
                    enc->setFragmentBuffer(ub.get(), 0, (uint32_t)slot);
                }
                ++slot;
            }
        } else {
            int slot = 17;
            for (auto& nm : pr.vsBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(nm);
                if (ub) enc->setVertexBuffer(ub.get(), 0, (uint32_t)slot);
                else bindZeroV(slot);
                ++slot;
            }
            slot = 17;
            for (auto& nm : pr.fsBlocks) {
                if (slot > 30) break;
                auto ub = uploadBlock(nm);
                if (ub) enc->setFragmentBuffer(ub.get(), 0, (uint32_t)slot);
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
    // Chỉ LOG + đếm (không chặn — desktop GL thường "chạy được").
    auto hazardCheck = [&](GLuint texId) {
        if (drawColorTexId && texId == drawColorTexId) {
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
    for (size_t k = 0; k < pr.vsSamplers.size(); ++k) {
        const std::string& name = pr.vsSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        if (kind == 'A') continue; // array: chưa có fallback đúng loại (giữ skip)
        const TextureObject* txp = nullptr;
        metal::ITexture* gpu = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit);
            auto tit = c.textures.find(texId);
            if (tit != c.textures.end() && tit->second.gpu &&
                kindOk(name, tit->second.target)) {
                txp = &tit->second;
                gpu = txp->gpu.get();
                hazardCheck(texId);
            } else if (tit != c.textures.end()) {
                static int nDenyV = 0;
                if (++nDenyV <= 8) {
                    fprintf(stderr,
                            "[TGLMT] sampdenyV#%d prog@%u vs=%s unit=%u tex#%u tgt=0x%x ifmt=0x%x gpu=%d\n",
                            nDenyV, prog, name.c_str(), unit, texId,
                            tit->second.target, tit->second.internalFormat,
                            (int)(tit->second.gpu != nullptr));
                    fflush(stderr);
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
        if (kind == 'A') continue; // array: chưa có fallback đúng loại (giữ skip)
        const TextureObject* txp = nullptr;
        metal::ITexture* gpu = nullptr;
        if (unit < 32) {
            GLuint texId = c.state.BoundTexture(unit);
            auto tit = c.textures.find(texId);
            if (tit != c.textures.end() && tit->second.gpu &&
                kindOk(name, tit->second.target)) {
                txp = &tit->second;
                gpu = txp->gpu.get();
                hazardCheck(texId);
                // Soi texture GUI (widgets 256x256 / header 256x128): log 1 lần
                // mỗi texture để thấy min/mag + pixel GPU thật + state raster
                // (nút chỉ còn chữ = sample trong suốt → discard; cần biết
                // texture rỗng hay state blend/depth/cull/scissor sai).
                if (txp->w == 256 && (txp->h == 256 || txp->h == 128)) {
                    static std::set<GLuint> loggedGui;
                    if (loggedGui.size() < 12 && loggedGui.insert(texId).second) {
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
                        // Pixel GPU thật (upload-only texture như widgets: readback
                        // chính xác; render-target texture có thể stale vì encoder
                        // chưa commit — ghi chú để khỏi đọc sai).
                        std::string gpx = "nogpu";
                        if (txp->gpu && txp->w <= 512 && txp->h <= 512) {
                            auto wt = c.device->wrapAsTarget(txp->gpu.get(), nullptr);
                            if (wt) {
                                std::vector<uint8_t> tb(4, 0);
                                // đọc 1 texel góc bằng readback hàng đầu
                                std::vector<uint8_t> row((size_t)txp->w * 4, 0);
                                // (readback full nhỏ gọn hơn: 256x256=256KB, 12 lần max)
                                std::vector<uint8_t> full((size_t)txp->w * txp->h * 4, 0);
                                if (wt->readback(full.data(), (size_t)txp->w * 4)) {
                                    char gb[32];
                                    snprintf(gb, sizeof(gb), "gpu%02X%02X%02X%02X",
                                             full[0], full[1], full[2], full[3]);
                                    gpx = gb;
                                } else {
                                    gpx = "rbFail";
                                }
                            }
                        }
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
                }
            } else if (tit != c.textures.end()) {
                // kindOk từ chối hoặc thiếu GPU: sample đen → quad trong suốt →
                // discard (nghi phạm nút mất nền). Log stderr để thấy trên máy.
                static int nDeny = 0;
                if (++nDeny <= 16) {
                    fprintf(stderr,
                            "[TGLMT] sampdeny#%d prog@%u fs=%s unit=%u tex#%u %ux%u "
                            "tgt=0x%x gpu=%d\n",
                            nDeny, prog, name.c_str(), unit, texId, tit->second.w,
                            tit->second.h, tit->second.target,
                            (int)(tit->second.gpu != nullptr));
                    fflush(stderr);
                }
                c.LogDebug(0, 0, 0, 0,
                           "AppleDrawGL: FS sampler " + name + " thieu/khop, fallback den");
            } else {
                // texId 0/unknown: sampler không bind gì (GL incomplete = đen).
                static int nMiss = 0;
                if (++nMiss <= 16) {
                    fprintf(stderr, "[TGLMT] sampmiss#%d prog@%u fs=%s unit=%u tex#%u\n",
                            nMiss, prog, name.c_str(), unit, texId);
                    fflush(stderr);
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
            enc->setFragmentTexture(tit->second.gpu.get(), unit);
            auto ss = SamplerForUnit(c, unit, tit->second);
            if (ss) enc->setFragmentSamplerState(ss.get(), unit);
        }
    }
    // Index buffer. Metal drawIndexed KHÔNG có baseVertex → emulate bằng cách cộng
    // baseVertex vào từng giá trị index (đúng GL). baseVertex âm làm index âm → lỗi.
    std::shared_ptr<metal::IBuffer> ib;
    size_t ioff = 0;
    metal::IndexType ity = metal::IndexType::UInt32;
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
        if (baseVertex == 0) {
            if (bit != c.buffers.end() && bit->second.gpu && srcBytes &&
                indexByteOff + (size_t)count * elem <= bit->second.gpu->length()) {
                ib = bit->second.gpu; // fast path: dùng thẳng EBO GPU
                ioff = indexByteOff;
            } else if (srcBytes) {
                ib = TempUpload(c, srcBytes, (size_t)count * elem);
                ioff = 0;
            } else {
                return false;
            }
        } else {
            if (!srcBytes) return false;
            // Viết lại indices + baseVertex vào buffer tạm
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
            ib = TempUpload(c, rewritten.data(), rewritten.size());
            ioff = 0;
        }
        if (!ib) { c.appleStats.miscFail++; return false; }
        // Validator index max vs sức chứa đỉnh (TBDR fault khi index trỏ ra
        // ngoài buffer): quét tối đa 32k index đầu, chỉ LOG + đếm.
        if (srcBytes) {
            static std::set<GLuint> warnedIdx;
            uint64_t mx = 0;
            GLsizei scan = count > 32768 ? 32768 : count;
            for (GLsizei k = 0; k < scan; ++k)
                mx = std::max(mx, (elem == 2)
                                        ? (uint64_t)(srcBytes[2 * k] | (srcBytes[2 * k + 1] << 8))
                                        : (uint64_t)(srcBytes[4 * k] | (srcBytes[4 * k + 1] << 8) |
                                                     (srcBytes[4 * k + 2] << 16) |
                                                     (srcBytes[4 * k + 3] << 24)));
            int64_t want = (int64_t)mx + (int64_t)baseVertex;
            if (want >= 0) {
                for (auto& ca : cas) {
                    auto bbit = bindMap.find(ca.bufferIndex);
                    if (bbit == bindMap.end() || !bbit->second.first || !ca.stride) continue;
                    size_t gpuLen = bbit->second.first->length();
                    size_t elemBytes = (size_t)ca.size * GLTypeSize(ca.type);
                    size_t cap = 0;
                    if (ca.divisor > 0) {
                        cap = gpuLen; // per-instance: fetch nhỏ, bỏ qua
                    } else if (gpuLen > bbit->second.second + ca.offset + elemBytes) {
                        cap = (gpuLen - bbit->second.second - ca.offset - elemBytes) / ca.stride +
                              1;
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
    // IR deferred: giữ index temp sống đến flush (encoder chưa commit).
    if (indexed && ib) c.pendingKeep.push_back(ib);
    if (indexed) enc->drawIndexed(prim, (uint32_t)count, ity, ib.get(), ioff, (uint32_t)inst);
    else enc->drawPrimitives(prim, (uint32_t)first, (uint32_t)count, (uint32_t)inst);
    // IR: KHÔNG commit mỗi draw. Encoder giữ mở để batch N draw → 1 commit ở flush
    // (ReadPixels/Blit/present/EndFrame/target đổi/glClear). Trước đây mỗi draw =
    // 1 commandBuffer + commit (1 GL → 3 Metal). Giờ N draw cùng pass = 1 encoder.
    // Ngữ cảnh cho FIRST-fault handler bất đồng bộ (A11 ban sau fault hàng loạt).
    {
        char b[160];
        snprintf(b, sizeof(b), "prog@%u vao@%u mode=0x%x count=%d idx=%d fbo=%u",
                 prog, vao, mode, count, (int)indexed, c.state.BoundDrawFBO());
        c.device->noteDrawContext(b);
    }
    c.appleStats.drawsEncoded++;
    c.appleStats.progEncoded[prog]++;
    // Chẩn đoán đen màn hình: dump TOÀN BỘ draw-state lần đầu mỗi
    // (program,VAO,texture0) — texture0 vì cùng prog/vao nhưng texture khác
    // nhau (logo vs widgets) là 2 case khác nhau (nút mất nền mà dump trùng
    // logo thì mù). Cap 96 combo, mỗi combo 1 lần nên không phình.
    {
        static std::set<std::tuple<GLuint, GLuint, GLuint>> loggedDraws;
        GLuint tex0 = c.state.BoundTexture(0);
        auto key = std::make_tuple(prog, vao, tex0);
        if (loggedDraws.size() < 96 && !loggedDraws.count(key)) {
            loggedDraws.insert(key);
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
                        idxs += std::to_string(v + (int64_t)bv) + " ";
                    }
                    fprintf(stderr, "[TGLMT]   idx first6(base applied)=[%s]\n", idxs.c_str());
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
                }
                fprintf(stderr,
                        "[TGLMT]   attr slot%d=%s %dx0x%x%s bind=%u rel=%zu stride=%d "
                        "buf=%u base=%zu v0=[%s]\n",
                        i, nm.c_str(), a.size, a.type, a.normalized ? "N" : "", a.binding,
                        a.relativeOffset, a.stride, a.buffer, base, bytes.c_str());
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
                GLuint texId = c.state.BoundTexture(unit);
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
                        }
                    }
                }
                fprintf(stderr,
                        "[TGLMT]   samp %s (%s): unit=%u tex=%u %ux%u fmt=0x%x gpu=%d "
                        "min=0x%x mag=0x%x lv=%d tgt=0x%x px0=%s\n",
                        name.c_str(), isVS ? "vs" : "fs", unit, texId, tx.w, tx.h,
                        tx.internalFormat, (int)(tx.gpu != nullptr), minF, magF, tx.levels,
                        tx.target, px0.c_str());
            };
            for (auto& s : pr.vsSamplers) dumpSamp(s, true);
            for (auto& s : pr.fsSamplers) dumpSamp(s, false);
            // State raster.
            {
                float bc[4];
                c.state.GetBlendColor(bc);
                auto sc = c.state.GetScissor();
                fprintf(stderr,
                        "[TGLMT]   state blend=%d cull=%d/%d depth=%d scissor=%d[%d,%d,%d,%d] "
                        "clear=(%.2f,%.2f,%.2f,%.2f)\n",
                        (int)c.state.IsEnabled(0x0BE2), (int)c.state.IsEnabled(0x0B44),
                        c.state.CullMode(), (int)c.state.IsEnabled(0x0B71),
                        (int)c.state.IsEnabled(0x0C11), sc.x, sc.y, sc.w, sc.h, c.clearColor[0],
                        c.clearColor[1], c.clearColor[2], c.clearColor[3]);
            }
            fflush(stderr);
        }
    }
    return true;
}

} // namespace tglmt
