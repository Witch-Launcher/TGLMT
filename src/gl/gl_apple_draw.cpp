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
#include <cstring>
#include <map>
#include <vector>

namespace tglmt {

static std::shared_ptr<metal::IBuffer> TempUpload(Context& c, const void* data, size_t n) {
    return c.device->newBufferWithBytes(data, n ? n : 1, metal::StorageMode::Shared);
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
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    metal::SamplerDesc d;
    d.minFilter = minF; d.magFilter = magF; d.sWrap = sW; d.tWrap = tW; d.maxAniso = aniso;
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
        metal::CustomAttrib ca;
        ca.loc = (uint32_t)i; ca.size = (uint32_t)a.size; ca.type = a.type;
        ca.normalized = a.normalized ? true : false;
        ca.offset = (uint32_t)a.offset;
        ca.bufferIndex = a.binding;
        ca.stride = (uint32_t)a.stride;
        ca.divisor = a.divisor;
        cas.push_back(ca);
        if (!bindMap.count(a.binding))
            bindMap[a.binding] = {bit->second.gpu, (size_t)a.offset};
    }
    if (cas.empty()) {
        // Draw không attribute (screenquad suy từ vertex_id): pipeline descriptor
        // rỗng, không bind vertex buffer. Không phải lỗi (trước đây noPipeline oan).
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: attributeless draw (vertex_id)");
    }
    for (auto& ca : cas) {
        if (ca.stride == 0) ca.stride = ca.offset + ca.size * GLTypeSize(ca.type);
    }
    // 3. Target: FBO 0 → default; khác → wrap colorTex[0] (+depth nếu có)
    std::shared_ptr<metal::IRenderTarget> target;
    bool hasDepthTex = false;
    if (c.state.BoundDrawFBO() == 0) {
        target = c.device->defaultRenderTarget();
        if (!target) { c.appleStats.noTarget++; return false; } // app chưa đặt target
    } else {
        auto fit = c.fbos.find(c.state.BoundDrawFBO());
        if (fit == c.fbos.end()) return false;
        auto cit = fit->second.colorTex.find(0);
        if (cit == fit->second.colorTex.end()) return false;
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
    auto pipe = c.device->makeCustomPipeline(pr.appleVS.get(), "TGLMT_vs", pr.appleFS.get(),
                                             "TGLMT_fs", target->pixelFormat(),
                                             cas.empty() ? nullptr : cas.data(), (uint32_t)cas.size(),
                                             cas.empty() ? 0 : cas[0].stride, &opts);
    if (!pipe) {
        c.appleStats.noPipeline++;
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: pipeline nil (format attrib chưa hỗ trợ?)");
        return false;
    }
    // 5. Encoder: clear theo mask 1 lần sau glClear, sau đó LOAD giữ kết quả.
    std::shared_ptr<metal::IRenderEncoder> enc;
    {
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
    }
    if (!enc) { c.appleStats.miscFail++; return false; }
    // Viewport/scissor (GL→Metal convert)
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
        enc->setViewport(mvp);
        if (c.state.IsEnabled(0x0C11)) { // SCISSOR_TEST
            auto sc = c.state.GetScissor();
            metal::ScissorRect mr = GLScissorToMetal(sc.x, sc.y, sc.w, sc.h, (int)target->height(),
                                                   upper);
            // kẹp vào target (Metal_scissor vượt biên → lỗi validation)
            if (mr.x < target->width() && mr.y < target->height()) {
                if (mr.x + mr.w > target->width()) mr.w = target->width() - mr.x;
                if (mr.y + mr.h > target->height()) mr.h = target->height() - mr.y;
                enc->setScissorRect(mr);
            }
        }
    }
    // Cull / fillMode / blendColor / depthState
    enc->setCullMode(c.state.IsEnabled(0x0B44), c.state.CullMode(), c.state.FrontFace());
    if (c.state.PolygonMode() == 0x1B01) enc->setTriangleFillModeLines(true); // LINE
    {
        float bc[4];
        c.state.GetBlendColor(bc);
        enc->setBlendColor(bc[0], bc[1], bc[2], bc[3]);
    }
    std::shared_ptr<metal::IDepthStencilState> dss;
    if (opts.depth) {
        dss = c.device->makeDepthStencilState(c.state.Depth().func,
                                              c.state.Depth().writeMask);
        if (!dss) { c.appleStats.miscFail++; return false; }
        enc->setDepthStencilState(dss.get());
    }
    // Vertex buffers theo binding
    for (auto& kv : bindMap) enc->setVertexBuffer(kv.second.first.get(), kv.second.second, kv.first);
    // Uniforms tách VS/FS (fix bug gộp sai offset FS):
    // Mỗi stage có TGLMTUniforms riêng từ offset 0 (đúng MSL đã sinh).
    // VS uniforms → vertex buffer(16), FS uniforms → fragment buffer(16).
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
        vsUbuf = TempUpload(c, vsb.data(), vsb.size());
        fsUbuf = TempUpload(c, fsb.data(), fsb.size());
        enc->setVertexBuffer(vsUbuf.get(), 0, 16);
        enc->setFragmentBuffer(fsUbuf.get(), 0, 16);
    }
    // UBO read-only (vanilla 1.17+/Sodium): mỗi block bind buffer(17+k) per-stage.
    // Block → bindingPoint (glUniformBlockBinding) → GL buffer (BindBufferBase/Range).
    std::vector<std::shared_ptr<metal::IBuffer>> uboKeep;
    {
        int slot = 17;
        for (auto& b : pr.uniformBlocks) {
            if (slot > 30) break; // Metal tối đa 31 slot, giữ 31 dự phòng
            auto bit = c.uniformBindPoints.find(b.binding);
            if (bit == c.uniformBindPoints.end() || !bit->second.buffer) { ++slot; continue; }
            auto t = c.buffers.find(bit->second.buffer);
            if (t == c.buffers.end() || t->second.data.empty()) { ++slot; continue; }
            size_t off = (size_t)bit->second.offset;
            size_t len = bit->second.size ? (size_t)bit->second.size
                                          : t->second.data.size() - std::min(off, t->second.data.size());
            if (off >= t->second.data.size()) { ++slot; continue; }
            len = std::min(len, t->second.data.size() - off);
            auto ub = TempUpload(c, t->second.data.data() + off, len);
            uboKeep.push_back(ub);
            // UBO dùng chung cả 2 stage (đúng GL: block visible cả vs+fs)
            enc->setVertexBuffer(ub.get(), 0, (uint32_t)slot);
            enc->setFragmentBuffer(ub.get(), 0, (uint32_t)slot);
            ++slot;
        }
    }
    // Sampler/texture theo glUniform1i unit → MSL slot k (fix bug bind theo unit):
    // FS samplers (theo thứ tự khai báo) → fragment texture(k)/sampler(k).
    // VS samplers → vertex texture(k)/sampler(k). Mặc định unit 0 đúng GL.
    // Target không khớp loại sampler (cube/array gắn texture 2D...) → BỎ bind
    // để Metal không abort encoder (đen đúng sampler đó, P1 làm cube/array thật).
    auto kindOk = [&](const std::string& name, GLenum target) {
        auto kit = pr.samplerKind.find(name);
        char kind = (kit == pr.samplerKind.end()) ? '2' : kit->second;
        // Cube/Array: chưa có GPU texture đúng loại (placeholder 2D gắn vào
        // texturecube param sẽ abort encoder) → bỏ bind, đen đúng sampler đó.
        // P1: MTLTextureTypeCube/Array thật.
        if (kind == 'C' || kind == 'A') return false;
        // target==0: DSA bind (glBindTextureUnit) không ghi target → tin tưởng.
        // Chỉ chặn mismatch CHẮC CHẮN (cả hai đã biết mà khác nhau).
        if (target == 0) return true;
        switch (kind) {
            case 'B': return target == 0x8C2A; // TEXTURE_BUFFER
            default: return target == 0x0DE1;  // TEXTURE_2D (+shadow approx)
        }
    };
    for (size_t k = 0; k < pr.vsSamplers.size(); ++k) {
        const std::string& name = pr.vsSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        if (unit >= 32) continue;
        GLuint texId = c.state.BoundTexture(unit);
        if (!texId) continue;
        auto tit = c.textures.find(texId);
        if (tit == c.textures.end() || !tit->second.gpu) continue;
        if (!kindOk(name, tit->second.target)) {
            c.LogDebug(0, 0, 0, 0, "AppleDrawGL: bo bind VS sampler " + name + " (target khong khop)");
            continue;
        }
        enc->setVertexTexture(tit->second.gpu.get(), (uint32_t)k);
        auto ss = SamplerForUnit(c, unit, tit->second);
        if (ss) enc->setVertexSamplerState(ss.get(), (uint32_t)k);
    }
    for (size_t k = 0; k < pr.fsSamplers.size(); ++k) {
        const std::string& name = pr.fsSamplers[k];
        auto uit = pr.samplerUnits.find(name);
        GLuint unit = (uit == pr.samplerUnits.end()) ? 0 : uit->second;
        if (unit >= 32) continue;
        GLuint texId = c.state.BoundTexture(unit);
        if (!texId) continue;
        auto tit = c.textures.find(texId);
        if (tit == c.textures.end() || !tit->second.gpu) continue;
        if (!kindOk(name, tit->second.target)) {
            c.LogDebug(0, 0, 0, 0, "AppleDrawGL: bo bind FS sampler " + name + " (target khong khop)");
            continue;
        }
        enc->setFragmentTexture(tit->second.gpu.get(), (uint32_t)k);
        auto ss = SamplerForUnit(c, unit, tit->second);
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
    }
    if (indexed) enc->drawIndexed(prim, (uint32_t)count, ity, ib.get(), ioff, (uint32_t)inst);
    else enc->drawPrimitives(prim, (uint32_t)first, (uint32_t)count, (uint32_t)inst);
    // Commit không đợi từng draw (throughput benchmark); thứ tự đảm bảo bởi cùng
    // queue; readback/present cuối frame đồng bộ đúng (commitAndWait/present).
    if (!enc->endAndCommitNoWait()) {
        c.appleStats.miscFail++;
        c.LogDebug(0, 0, 0, 0, "AppleDrawGL: commit fail");
        return false;
    }
    c.appleStats.drawsEncoded++;
    c.appleStats.progEncoded[prog]++;
    return true;
}

} // namespace tglmt
