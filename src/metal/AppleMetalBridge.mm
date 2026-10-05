// AppleMetalBridge.mm — Objective-C++ cầu sang <Metal/Metal.h> gốc Apple (M5: encode thật).
// Tương đương 1-1 với metal-cpp (MTL::Device::newBuffer, newLibraryWithSource, ...).
// Phạm vi M5 (ghi rõ): offscreen render target + 1 pipeline layout chuẩn TGLMT
// (pos float2 @0 + color float4 @1, stride 24) + readback. Map đầy đủ
// VAO->MTLVertexDescriptor từ StateTracker là M5b (chưa làm, không giả vờ).
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h> // presentTarget (cả iOS lẫn macOS đều có QuartzCore)
#import <TargetConditionals.h> // phân biệt iOS/macOS: synchronizeResource chỉ tồn tại trên macOS
#include "tglmt/MetalInterface.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>

namespace tglmt::metal {
#ifdef TGLMT_APPLE_METAL
std::shared_ptr<IDevice> CreateAppleDevice(LogFn log);
#endif
} // namespace tglmt::metal

#ifdef TGLMT_APPLE_METAL
namespace tglmt::metal {

static MTLPixelFormat ToMTL(PixelFormat f) {
    switch (f) {
        case PixelFormat::RGBA8Unorm: return MTLPixelFormatRGBA8Unorm;
        case PixelFormat::RGBA8Unorm_sRGB: return MTLPixelFormatRGBA8Unorm_sRGB;
        case PixelFormat::RGBA8Uint: return MTLPixelFormatRGBA8Uint;
        case PixelFormat::BGRA8Unorm: return MTLPixelFormatBGRA8Unorm;
        case PixelFormat::BGRA8Unorm_sRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
        case PixelFormat::R8Unorm: return MTLPixelFormatR8Unorm;
        case PixelFormat::RG8Unorm: return MTLPixelFormatRG8Unorm;
        case PixelFormat::R32Float: return MTLPixelFormatR32Float;
        case PixelFormat::R32Sint: return MTLPixelFormatR32Sint;
        case PixelFormat::R32Uint: return MTLPixelFormatR32Uint;
        case PixelFormat::R8Sint: return MTLPixelFormatR8Sint;
        case PixelFormat::R8Uint: return MTLPixelFormatR8Uint;
        case PixelFormat::R16Sint: return MTLPixelFormatR16Sint;
        case PixelFormat::R16Uint: return MTLPixelFormatR16Uint;
        case PixelFormat::R16Float: return MTLPixelFormatR16Float;
        case PixelFormat::Depth32Float: return MTLPixelFormatDepth32Float;
#if TARGET_OS_OSX
        case PixelFormat::Depth24Stencil8: return MTLPixelFormatDepth24Unorm_Stencil8;
#else
        // iOS cấm Depth24Unorm_Stencil8 → Depth32Float_Stencil8 (chính xác hơn, vẫn pass depth test)
        case PixelFormat::Depth24Stencil8: return MTLPixelFormatDepth32Float_Stencil8;
#endif
        default: return MTLPixelFormatInvalid;
    }
}
// Bảng ánh xạ GL enum (giá trị đã đối chiếu gl46_types.h) → MTL. Không đoán.
static MTLBlendFactor ToMTLBlend(uint32_t g) {
    switch (g) {
        case 0: return MTLBlendFactorZero;
        case 1: return MTLBlendFactorOne;
        case 0x0300: return MTLBlendFactorSourceColor;
        case 0x0301: return MTLBlendFactorOneMinusSourceColor;
        case 0x0302: return MTLBlendFactorSourceAlpha;
        case 0x0303: return MTLBlendFactorOneMinusSourceAlpha;
        case 0x0304: return MTLBlendFactorDestinationAlpha;
        case 0x0305: return MTLBlendFactorOneMinusDestinationAlpha;
        case 0x0306: return MTLBlendFactorDestinationColor;
        case 0x0307: return MTLBlendFactorOneMinusDestinationColor;
        case 0x0308: return MTLBlendFactorSourceAlphaSaturated;
        case 0x8001: return MTLBlendFactorBlendColor;
        case 0x8002: return MTLBlendFactorOneMinusBlendColor;
        case 0x8003: return MTLBlendFactorBlendAlpha;
        case 0x8004: return MTLBlendFactorOneMinusBlendAlpha;
        default: return MTLBlendFactorOne;
    }
}
static MTLBlendOperation ToMTLBlendOp(uint32_t g) {
    switch (g) {
        case 0x8006: return MTLBlendOperationAdd;
        case 0x800A: return MTLBlendOperationSubtract;
        case 0x800B: return MTLBlendOperationReverseSubtract;
        case 0x8007: return MTLBlendOperationMin;
        case 0x8008: return MTLBlendOperationMax;
        default: return MTLBlendOperationAdd;
    }
}
static MTLCompareFunction ToMTLCompare(uint32_t g) {
    switch (g) {
        case 0x0200: return MTLCompareFunctionNever;
        case 0x0201: return MTLCompareFunctionLess;
        case 0x0202: return MTLCompareFunctionEqual;
        case 0x0203: return MTLCompareFunctionLessEqual;
        case 0x0204: return MTLCompareFunctionGreater;
        case 0x0205: return MTLCompareFunctionNotEqual;
        case 0x0206: return MTLCompareFunctionGreaterEqual;
        default: return MTLCompareFunctionAlways;
    }
}
static MTLSamplerMinMagFilter ToMTLFilter(uint32_t g) {
    return (g == 0x2600) ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
}
static MTLSamplerAddressMode ToMTLWrap(uint32_t g) {
    switch (g) {
        case 0x2901: return MTLSamplerAddressModeRepeat;
        case 0x8370: return MTLSamplerAddressModeMirrorRepeat;
        case 0x812D: return MTLSamplerAddressModeClampToBorderColor;
        default: return MTLSamplerAddressModeClampToEdge; // 0x812F CLAMP_TO_EDGE
    }
}
static MTLPrimitiveType ToMTLPrim(PrimitiveType t) {
    switch (t) {
        case PrimitiveType::Point: return MTLPrimitiveTypePoint;
        case PrimitiveType::Line: return MTLPrimitiveTypeLine;
        case PrimitiveType::LineStrip: return MTLPrimitiveTypeLineStrip;
        case PrimitiveType::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        // Metal không có Fan/Patches trong drawPrimitives thường:
        // Fan được expand ở EmitDraw khi đi đường GPU (M5b); Patches đi drawPatches.
        // Default ở đây chỉ là lưới an toàn cho trace-encoder (không encode GPU).
        default: return MTLPrimitiveTypeTriangle;
    }
}

class AppleBuffer : public IBuffer {
public:
    explicit AppleBuffer(id<MTLBuffer> b) : buf_(b) {}
    void* contents() override { return [buf_ contents]; }
    const void* contents() const override { return [buf_ contents]; }
    size_t length() const override { return [buf_ length]; }
    void didModifyRange(size_t off, size_t len) override {
#if TARGET_OS_OSX
        // MTLStorageModeManaged + didModifyRange chỉ tồn tại trên macOS.
        // iOS unified memory: Shared luôn đồng bộ, không cần gọi.
        if ([buf_ storageMode] == MTLStorageModeManaged)
            [buf_ didModifyRange:NSMakeRange(off, len)];
#else
        (void)off; (void)len;
#endif
    }
    id<MTLBuffer> get() const { return buf_; }
private:
    id<MTLBuffer> buf_;
};

// Tập depth texture đã qua first-clear (per-texture, sống theo device +
// mọi AppleTexture depth còn giữ ref → không UAF, không leak cross-test).
struct DepthInitSet {
    std::mutex mu;
    std::set<void*> inited;
};

class AppleTexture : public ITexture {
public:
    AppleTexture(id<MTLTexture> t, uint32_t w, uint32_t h, PixelFormat f)
        : tex_(t), w_(w), h_(h), f_(f) {}
    // Depth Private khởi đầu rác: device nhớ texture nào đã init (per-texture,
    // không per-wrapper — wrapper tạo mới mỗi draw nên flag trên wrapper sai
    // thành force-clear mọi encoder mới = mất depth giữa frame = X-RAY).
    // Giữ shared_ptr tới set để an toàn teardown (device có thể chết trước).
    ~AppleTexture() {
        if (depthInitSet_ && tex_) {
            std::lock_guard<std::mutex> l(depthInitSet_->mu);
            depthInitSet_->inited.erase((__bridge void*)tex_);
        }
    }
    void trackDepthInit(std::shared_ptr<struct DepthInitSet> s) { depthInitSet_ = s; }
    uint32_t width() const override { return w_; }
    uint32_t height() const override { return h_; }
    PixelFormat pixelFormat() const override { return f_; }
    uint32_t levelCount() const override {
        @try { return (uint32_t)[tex_ mipmapLevelCount]; } @catch (NSException*) { return 1; }
    }
    id<MTLTexture> get() const { return tex_; }
private:
    id<MTLTexture> tex_;
    uint32_t w_, h_;
    PixelFormat f_;
    std::shared_ptr<struct DepthInitSet> depthInitSet_;
};

class AppleLibrary : public ILibrary {
public:
    explicit AppleLibrary(id<MTLLibrary> l) : lib_(l) {}
    id<MTLLibrary> get() const { return lib_; }
private:
    id<MTLLibrary> lib_;
};

class ApplePipeline : public IRenderPipeline {
public:
    explicit ApplePipeline(id<MTLRenderPipelineState> p, bool depth = false)
        : pso_(p), hasDepth_(depth) {}
    id<MTLRenderPipelineState> get() const { return pso_; }
    // Pipeline có depthAttachmentPixelFormat không? Pass phải khớp (TBDR strict).
    bool hasDepth() const { return hasDepth_; }
    uint64_t nativeHandle() const override { return (uint64_t)(__bridge void*)pso_; }
private:
    id<MTLRenderPipelineState> pso_;
    bool hasDepth_;
};

class AppleSampler : public ISamplerState {
public:
    explicit AppleSampler(id<MTLSamplerState> s) : samp_(s) {}
    id<MTLSamplerState> get() const { return samp_; }
private:
    id<MTLSamplerState> samp_;
};

class AppleDepthStencil : public IDepthStencilState {
public:
    explicit AppleDepthStencil(id<MTLDepthStencilState> s) : ds_(s) {}
    id<MTLDepthStencilState> get() const { return ds_; }
private:
    id<MTLDepthStencilState> ds_;
};

class AppleComputePipeline : public IComputePipeline {
public:
    explicit AppleComputePipeline(id<MTLComputePipelineState> p) : pso_(p) {}
    id<MTLComputePipelineState> get() const { return pso_; }
private:
    id<MTLComputePipelineState> pso_;
};

class AppleComputeEncoder : public IComputeEncoder {
public:
    explicit AppleComputeEncoder(id<MTLCommandBuffer> cb) : cb_(cb) {
        enc_ = cb ? [cb computeCommandEncoder] : nil;
    }
    void setPipeline(IComputePipeline* p) override {
        if (!enc_ || !p) return;
        AppleComputePipeline* ap = dynamic_cast<AppleComputePipeline*>(p);
        if (!ap) return;
        [enc_ setComputePipelineState:ap->get()];
    }
    void setTexture(ITexture* t, uint32_t idx) override {
        if (!enc_ || !t) return;
        AppleTexture* at = dynamic_cast<AppleTexture*>(t);
        if (!at) return;
        [enc_ setTexture:at->get() atIndex:idx];
    }
    void setBuffer(IBuffer* b, uint32_t idx) override {
        if (!enc_ || !b) return;
        AppleBuffer* ab = dynamic_cast<AppleBuffer*>(b);
        if (!ab) return;
        [enc_ setBuffer:ab->get() offset:0 atIndex:idx];
    }
    void dispatch2D(uint32_t w, uint32_t h) override {
        if (!enc_) return;
        MTLSize grid = {(NSUInteger)(w ? w : 1), (NSUInteger)(h ? h : 1), 1};
        MTLSize tg = {8, 8, 1};
        [enc_ dispatchThreads:grid threadsPerThreadgroup:tg];
    }
    bool endAndCommit() override {
        if (!enc_ || !cb_) return false;
        [enc_ endEncoding];
        enc_ = nil;
        [cb_ commit];
        [cb_ waitUntilCompleted];
        return [cb_ status] == MTLCommandBufferStatusCompleted;
    }
private:
    id<MTLCommandBuffer> cb_;
    id<MTLComputeCommandEncoder> enc_;
};

// Render target đầy đủ: color (+resolve khi MSAA) + depth tùy chọn.
// readback luôn đọc từ resolve/color Shared (không đọc MSAA/Private trực tiếp).
class AppleTarget : public IRenderTarget {
public:
    AppleTarget(id<MTLTexture> color, PixelFormat fmt, id<MTLTexture> resolve,
                id<MTLTexture> depth, uint32_t w, uint32_t h, bool msaa,
                bool depthFirstClear)
        : color_(color), fmt_(fmt), resolve_(resolve ? resolve : color), depth_(depth),
          w_(w), h_(h), msaa_(msaa), depthFirstClear_(depthFirstClear) {}
    PixelFormat pixelFormat() const override { return fmt_; }
    uint32_t width() const override { return w_; }
    uint32_t height() const override { return h_; }
    bool readback(void* dst, size_t bpr) override {
        if (!dst || !resolve_) return false;
        @try {
            [resolve_ getBytes:dst bytesPerRow:bpr
                fromRegion:MTLRegionMake2D(0, 0, w_, h_) mipmapLevel:0];
        } @catch (NSException*) { return false; }
        return true;
    }
    id<MTLTexture> color() const { return color_; }
    id<MTLTexture> resolve() const { return resolve_; }
    id<MTLTexture> depth() const { return depth_; }
    bool isMSAA() const { return msaa_; }
    bool hasDepth() const override { return depth_ != nil; }
    // Depth Private khởi đầu rác → lần đầu dùng phải Clear (sau đó theo loadAction).
    bool takeDepthFirstClear() {
        bool r = depth_ && depthFirstClear_;
        depthFirstClear_ = false;
        return r;
    }
private:
    id<MTLTexture> color_, resolve_, depth_;
    PixelFormat fmt_;
    uint32_t w_, h_;
    bool msaa_;
    bool depthFirstClear_ = true;
};

// Tracer cho đường GL dispatch cũ (giữ trace như Null, không encode GPU).
class TraceEncoder : public IEncoder {
public:
    explicit TraceEncoder(std::vector<DrawTrace>& t) : trace_(t) {}
    void setViewport(const Viewport&) override {}
    void drawPrimitives(PrimitiveType t, uint32_t s, uint32_t c, uint32_t inst) override {
        trace_.push_back({t, c, inst, false, s});
    }
    void drawIndexed(PrimitiveType t, uint32_t c, IndexType, IBuffer*, size_t, uint32_t inst) override {
        trace_.push_back({t, c, inst, true, 0});
    }
    void setVertexBuffer(IBuffer*, size_t, uint32_t) override {}
    void endEncoding() override {}
private:
    std::vector<DrawTrace>& trace_;
};

class AppleRenderEncoder : public IRenderEncoder {
public:
    struct SharedDiag {
        std::mutex mu;
        std::string lastCtx;
        bool firstErrLogged = false;
        int faultLogs = 0;
    };
    AppleRenderEncoder(id<MTLCommandBuffer> cb, id<MTLRenderCommandEncoder> enc,
                       id<MTLTexture> target, LogFn log = nullptr,
                       std::shared_ptr<SharedDiag> diag = nullptr,
                       std::atomic<uint64_t>* committed = nullptr,
                       std::atomic<uint64_t>* completed = nullptr)
        : cb_(cb), enc_(enc), target_(target), log_(log), diag_(diag),
          countCommitted_(committed), countCompleted_(completed), ok_(cb && enc) {}
    void setViewport(const Viewport& vp) override {
        if (!ok_) return;
        MTLViewport m = {vp.x, vp.y, vp.w, vp.h, vp.n, vp.f};
        [enc_ setViewport:m];
    }
    void setVertexBuffer(IBuffer* b, size_t off, uint32_t idx) override {
        if (!ok_ || !b) return;
        AppleBuffer* ab = dynamic_cast<AppleBuffer*>(b);
        if (!ab) return;
        [enc_ setVertexBuffer:ab->get() offset:off atIndex:idx];
    }
    void setPipeline(IRenderPipeline* p) override {
        if (!ok_ || !p) return;
        ApplePipeline* ap = dynamic_cast<ApplePipeline*>(p);
        if (!ap || !ap->get()) return;
        [enc_ setRenderPipelineState:ap->get()];
    }
    void drawPrimitives(PrimitiveType t, uint32_t start, uint32_t count, uint32_t inst) override {
        if (!ok_) { ok_ = false; return; }
        if (inst <= 1) [enc_ drawPrimitives:ToMTLPrim(t) vertexStart:start vertexCount:count];
        else [enc_ drawPrimitives:ToMTLPrim(t) vertexStart:start vertexCount:count instanceCount:inst];
    }
    void drawIndexed(PrimitiveType t, uint32_t count, IndexType it, IBuffer* ib, size_t off, uint32_t inst) override {
        if (!ok_ || !ib) { ok_ = false; return; }
        AppleBuffer* ab = dynamic_cast<AppleBuffer*>(ib);
        if (!ab) { ok_ = false; return; }
        MTLIndexType m = (it == IndexType::UInt16) ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32;
        if (inst <= 1) [enc_ drawIndexedPrimitives:ToMTLPrim(t) indexCount:count indexType:m
                        indexBuffer:ab->get() indexBufferOffset:off];
        else [enc_ drawIndexedPrimitives:ToMTLPrim(t) indexCount:count indexType:m
              indexBuffer:ab->get() indexBufferOffset:off instanceCount:inst];
    }
    void setTessellationFactorBuffer(IBuffer* b, size_t off, size_t stride) override {
        if (!ok_ || !b) return;
        AppleBuffer* ab = dynamic_cast<AppleBuffer*>(b);
        if (!ab) return;
        [enc_ setTessellationFactorBuffer:ab->get() offset:off instanceStride:stride];
    }
    void drawPatches(uint32_t controlPoints, uint32_t patchStart, uint32_t patchCount) override {
        if (!ok_) return;
        [enc_ drawPatches:(NSUInteger)controlPoints patchStart:(NSUInteger)patchStart
               patchCount:(NSUInteger)patchCount patchIndexBuffer:nil patchIndexBufferOffset:0
               instanceCount:1 baseInstance:0];
    }
    void drawMesh(uint32_t groups) override {
        if (!ok_) return;
        // drawMeshThreadgroups cần macOS 13+/iOS 16+ — gate runtime, OS cũ fail mềm.
        SEL sel = @selector(drawMeshThreadgroups:threadsPerObjectThreadgroup:threadsPerMeshThreadgroup:);
        if (![enc_ respondsToSelector:sel]) { ok_ = false; return; }
        MTLSize g = {(NSUInteger)(groups ? groups : 1), 1, 1};
        MTLSize t = {1, 1, 1};
        [enc_ drawMeshThreadgroups:g threadsPerObjectThreadgroup:t threadsPerMeshThreadgroup:t];
    }
    void setTriangleFillModeLines(bool lines) override {
        if (!ok_) return;
        [enc_ setTriangleFillMode:lines ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
    }
    void setScissorRect(const ScissorRect& r) override {
        if (!ok_) return;
        MTLScissorRect m = {(NSUInteger)r.x, (NSUInteger)r.y, (NSUInteger)r.w, (NSUInteger)r.h};
        [enc_ setScissorRect:m];
    }
    void setDepthStencilState(IDepthStencilState* s) override {
        if (!ok_ || !s) return;
        AppleDepthStencil* ad = dynamic_cast<AppleDepthStencil*>(s);
        if (!ad) return;
        [enc_ setDepthStencilState:ad->get()];
    }
    void setFragmentTexture(ITexture* t, uint32_t idx) override {
        if (!ok_ || !t) return;
        AppleTexture* at = dynamic_cast<AppleTexture*>(t);
        if (!at) return;
        [enc_ setFragmentTexture:at->get() atIndex:idx];
    }
    void setFragmentSamplerState(ISamplerState* s, uint32_t idx) override {
        if (!ok_ || !s) return;
        AppleSampler* as = dynamic_cast<AppleSampler*>(s);
        if (!as) return;
        [enc_ setFragmentSamplerState:as->get() atIndex:idx];
    }
    void setVertexTexture(ITexture* t, uint32_t idx) override {
        if (!ok_ || !t) return;
        AppleTexture* at = dynamic_cast<AppleTexture*>(t);
        if (!at) return;
        [enc_ setVertexTexture:at->get() atIndex:idx];
    }
    void setVertexSamplerState(ISamplerState* s, uint32_t idx) override {
        if (!ok_ || !s) return;
        AppleSampler* as = dynamic_cast<AppleSampler*>(s);
        if (!as) return;
        [enc_ setVertexSamplerState:as->get() atIndex:idx];
    }
    void setFragmentBuffer(IBuffer* b, size_t off, uint32_t idx) override {
        if (!ok_ || !b) return;
        AppleBuffer* ab = dynamic_cast<AppleBuffer*>(b);
        if (!ab) return;
        [enc_ setFragmentBuffer:ab->get() offset:off atIndex:idx];
    }
    void setCullMode(bool enabled, uint32_t cullModeGL, uint32_t frontFaceGL) override {
        if (!ok_) return;
        if (!enabled) {
            [enc_ setCullMode:MTLCullModeNone];
            return;
        }
        [enc_ setCullMode:(cullModeGL == 0x0404) ? MTLCullModeFront : MTLCullModeBack];
        [enc_ setFrontFacingWinding:(frontFaceGL == 0x0900) ? MTLWindingClockwise
                                                           : MTLWindingCounterClockwise];
    }
    void setBlendColor(float r, float g, float b, float a) override {
        if (!ok_) return;
        [enc_ setBlendColorRed:r green:g blue:b alpha:a];
    }
    bool endAndCommitNoWait() override {
        if (!enc_ || !cb_) return false;
        [enc_ endEncoding];
        enc_ = nil;
        // Đếm command buffer để Context biết còn frame nào đang bay (xem
        // setCommandBufferCounters). Handler gắn cho MỌI commit: một queue Metal
        // thực thi in-order nên completed là "đã xong tới đây". Không handler
        // thì không biết khi nào an toàn ghi lại shared memory → phải
        // commitAndWait mỗi frame (mất pipeline, +latency).
        if (countCompleted_) {
            std::atomic<uint64_t>* done = countCompleted_;
            [cb_ addCompletedHandler:^(id<MTLCommandBuffer>) {
                done->fetch_add(1, std::memory_order_relaxed);
            }];
        }
        {
            // Chẩn đoán đen màn hình: GPU có thể error CB mà CPU không hay (A11).
            // Bắt fault ĐẦU TIÊN kèm ngữ cảnh draw (prog/vao).
            // (Ban submissions của iOS làm mọi CB sau đều status=5 "prior errors",
            // che mất nguyên nhân gốc.)
            // Bản cũ sample 1/120 → fault của bake encoder (hiếm, đúng cái cần)
            // gần như luôn lọt. Block rẻ (~13 commit/frame); chỉ log khi fault:
            // FIRST luôn log, sau đó cap 16 + 1/120 để không spam khi bị ban.
            if (log_) {
                LogFn log = log_;
                std::shared_ptr<SharedDiag> diag = diag_;
                [cb_ addCompletedHandler:^(id<MTLCommandBuffer> b) {
                  if ([b status] != MTLCommandBufferStatusCompleted && log) {
                      NSString* e = [[b error] localizedDescription];
                      std::string ctx;
                      bool first = false;
                      int n = 0;
                      if (diag) {
                          std::lock_guard<std::mutex> l(diag->mu);
                          ctx = diag->lastCtx;
                          first = !diag->firstErrLogged;
                          if (first) diag->firstErrLogged = true;
                          n = ++diag->faultLogs;
                      }
                      if (first || n <= 16 || (n % 120) == 0) {
                          std::string msg = std::string(first ? "[TGLMT] FIRST drawCB fault ctx={" : "[TGLMT] drawCB status=") +
                              (first ? ctx + "} status=" : "") +
                              std::to_string((long)[b status]) + " err=" +
                              (e ? [e UTF8String] : "?");
                          log(msg);
                      }
                  }
                }];
            }
        }
        NoteCommit();
        [cb_ commit];
        return ok_;
    }
    void setCompletedCounter(std::atomic<uint64_t>* c) { countCompleted_ = c; }
    void noteCommit() { NoteCommit(); }
    bool endAndCommit() override {
        if (!enc_ || !cb_) return false;
        [enc_ endEncoding];
        enc_ = nil;
#if TARGET_OS_OSX
        // macOS discrete GPU: bắt buộc synchronize trước khi CPU đọc Shared texture.
        // iOS/TVDR unified memory không có API này (không cần) → guard để biên dịch được cho iOS.
        id<MTLBlitCommandEncoder> blit = [cb_ blitCommandEncoder];
        if (blit && target_) { [blit synchronizeResource:target_]; [blit endEncoding]; }
#endif
        [cb_ commit];
        [cb_ waitUntilCompleted];
        return ok_ && [cb_ status] == MTLCommandBufferStatusCompleted;
    }
private:
    void NoteCommit() {
        if (countCommitted_) countCommitted_->fetch_add(1, std::memory_order_relaxed);
    }
    id<MTLCommandBuffer> cb_;
    id<MTLRenderCommandEncoder> enc_;
    id<MTLTexture> target_;
    LogFn log_;
    std::shared_ptr<SharedDiag> diag_;
    std::atomic<uint64_t>* countCommitted_ = nullptr;
    std::atomic<uint64_t>* countCompleted_ = nullptr;
    bool ok_;
};

class AppleDevice : public IDevice {
public:
    explicit AppleDevice(id<MTLDevice> d, LogFn log)
        : dev_(d), queue_([d newCommandQueue]), log_(log),
          diag_(std::make_shared<AppleRenderEncoder::SharedDiag>()) {}
    void noteDrawContext(const std::string& s) override {
        if (!diag_) return;
        std::lock_guard<std::mutex> l(diag_->mu);
        diag_->lastCtx = s;
    }
    std::string name() const override {
        return std::string("TGLMT Apple (") +
            ([dev_.name UTF8String] ? [dev_.name UTF8String] : "MTLDevice") + ")";
    }
    bool isNull() const override { return false; }
    std::shared_ptr<IBuffer> newBuffer(size_t n, StorageMode) override {
        id<MTLBuffer> b = [dev_ newBufferWithLength:(NSUInteger)(n ? n : 1)
                            options:MTLResourceStorageModeShared];
        return b ? std::make_shared<AppleBuffer>(b) : nullptr;
    }
    std::shared_ptr<IBuffer> newBufferWithBytes(const void* p, size_t n, StorageMode) override {
        if (!p || !n) return newBuffer(n, StorageMode::Shared);
        id<MTLBuffer> b = [dev_ newBufferWithBytes:p length:(NSUInteger)n
                            options:MTLResourceStorageModeShared];
        return b ? std::make_shared<AppleBuffer>(b) : nullptr;
    }
    std::shared_ptr<ITexture> newTexture(uint32_t w, uint32_t h, PixelFormat f) override {
        // Guard: validateWithDevice THIẾU width/height quá max là abort không
        // catch được (đã crash thật: buffer texture width 710178 > 16384).
        // MTLDevice không expose maxTextureDimension2D → mọi Apple GPU A9+ = 16384.
        const NSUInteger maxD = 16384;
        if (w == 0 || h == 0 || w > maxD || h > maxD) {
            if (log_)
                log_("TGLMT: newTexture " + std::to_string(w) + "x" + std::to_string(h) +
                     " vuot Metal max (" + std::to_string(maxD) + ") → bỏ qua");
            return nullptr;
        }
        MTLTextureDescriptor* d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:ToMTL(f)
                                            width:w height:h mipmapped:NO];
        // Integer/float-buffer textures chỉ đọc (.read), không render target
        // (R32Sint làm RT có thể fail validation trên TBDR).
        if (f == PixelFormat::R32Sint || f == PixelFormat::R32Uint || f == PixelFormat::R32Float)
            d.usage = MTLTextureUsageShaderRead;
        else
            d.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        // Depth/Stencil CẤM Shared/Managed (validateWithDevice abort — đã crash
        // thật khi glTexImage2D DEPTH_COMPONENT32F). Giống newDepthTexture: Private.
        if (f == PixelFormat::Depth32Float || f == PixelFormat::Depth24Stencil8)
            d.storageMode = MTLStorageModePrivate;
        else
            d.storageMode = MTLStorageModeShared;
        id<MTLTexture> t = [dev_ newTextureWithDescriptor:d];
        if (!t) return nullptr;
        auto tex = std::make_shared<AppleTexture>(t, w, h, f);
        if (f == PixelFormat::Depth32Float || f == PixelFormat::Depth24Stencil8)
            tex->trackDepthInit(depthInit_);
        return tex;
    }
    std::shared_ptr<IEncoder> makeEncoder() override {
        return std::make_shared<TraceEncoder>(trace_);
    }
    void setCommandBufferCounters(std::atomic<uint64_t>* committed,
                                  std::atomic<uint64_t>* completed) override {
        cbCommitted_ = committed;
        cbCompleted_ = completed;
    }
    void commitAndWait() override {
        // Barrier thật: commit rỗng + đợi → xả hết draws NoWait trước đó (cho
        // glFinish/glReadPixels). Không ảnh hưởng throughput frame thường.
        if (!queue_) return;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return;
        if (cbCompleted_)
            [cb addCompletedHandler:^(id<MTLCommandBuffer>) {
                cbCompleted_->fetch_add(1, std::memory_order_relaxed);
            }];
        NoteCommit(cb);
        [cb commit];
        [cb waitUntilCompleted];
        // Chẩn đoán đen màn hình: CPU-side đếm encode đủ mà GPU không chạy gì
        // (A11) thì status ở đây lộ ra (error/timeout/device-removed...).
        if ([cb status] != MTLCommandBufferStatusCompleted && log_) {
            static long lastLogged = 0;
            long st = (long)[cb status];
            if (st != lastLogged) {
                lastLogged = st;
                std::string msg = "[TGLMT] commitAndWait status=" + std::to_string(st);
                if ([cb error]) msg += " err=" + std::string([[[cb error] localizedDescription] UTF8String] ? [[[cb error] localizedDescription] UTF8String] : "?");
                log_(msg);
            }
        }
    }
    const std::vector<DrawTrace>& drawTrace() const override { return trace_; }
    void clearTrace() override { trace_.clear(); }

    std::shared_ptr<ILibrary> compileLibrary(const std::string& msl, std::string& err) override {
        NSError* e = nil;
        NSString* src = [NSString stringWithUTF8String:msl.c_str()];
        auto t0 = std::chrono::steady_clock::now();
        id<MTLLibrary> lib = [dev_ newLibraryWithSource:src options:nil error:&e];
        long ms =
            (long)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
        // MSL vanilla lớn (terrain/entity) compile hàng giây trên A11 — log để
        // biết render thread có đang kẹt ở đây (đen màn hình + ít swap).
        static int nLib = 0;
        if (log_ && (++nLib <= 80 || ms > 500))
            log_("[TGLMT] mslLib #" + std::to_string(nLib) + " " + std::to_string(ms) +
                 "ms bytes=" + std::to_string(msl.size()) + (lib ? "" : " NIL"));
        if (!lib) {
            err = e ? [[e localizedDescription] UTF8String] : "unknown metal compile error";
            return nullptr;
        }
        err.clear();
        return std::make_shared<AppleLibrary>(lib);
    }
    std::shared_ptr<IRenderPipeline> makeRenderPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !vsFn || !fsFn) return nullptr;
        // Key bằng std::string (không dùng buffer cố định: 2 con trỏ + tên hàm
        // dài có thể vượt 64 byte gây cắt key và đụng cache sai pipeline).
        std::string key = std::to_string((uintptr_t)alVs) + "|" + vsFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt);
        {   // pipeline cache (M5): cùng key không biên dịch lại
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:vsFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        // Layout chuẩn TGLMT M5: pos float2 @0, color float4 @1 (xem triangle.metal).
        MTLRenderPipelineDescriptor* d = basePipelineDesc(vs, fs, fmt, false);
        NSError* e = nil;
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pso) {
            if (log_ && e) log_("pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso);
    }
    // Helper dựng pipeline descriptor chung (non-tess + tess chỉ khác 4 dòng).
    MTLRenderPipelineDescriptor* basePipelineDesc(id<MTLFunction> vs, id<MTLFunction> fs,
                                                 PixelFormat fmt, bool depth) {
        MTLVertexDescriptor* vd = [MTLVertexDescriptor vertexDescriptor];
        vd.attributes[0].format = MTLVertexFormatFloat2;
        vd.attributes[0].offset = 0; vd.attributes[0].bufferIndex = 0;
        vd.attributes[1].format = MTLVertexFormatFloat4;
        vd.attributes[1].offset = 8; vd.attributes[1].bufferIndex = 0;
        vd.layouts[0].stride = 24; vd.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
        d.vertexFunction = vs; d.fragmentFunction = fs;
        d.vertexDescriptor = vd;
        d.colorAttachments[0].pixelFormat = ToMTL(fmt);
        // depthAttachmentPixelFormat phải khớp render pass có depth (validation Metal).
        if (depth) d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        return d;
    }
    std::shared_ptr<IRenderPipeline> makeDepthPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !vsFn || !fsFn) return nullptr;
        std::string key = "depth|" + std::to_string((uintptr_t)alVs) + "|" + vsFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt);
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second, true);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:vsFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        MTLRenderPipelineDescriptor* d = basePipelineDesc(vs, fs, fmt, true);
        NSError* e = nil;
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pso) {
            if (log_ && e) log_("depth pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso, true);
    }
    std::shared_ptr<IRenderPipeline> makeTessPipeline(ILibrary* vsLib, const char* postTessFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt, uint32_t controlPoints) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !postTessFn || !fsFn) return nullptr;
        if (controlPoints == 0 || controlPoints > 32) return nullptr; // MSL spec: N 0..32
        std::string key = "tess|" + std::to_string((uintptr_t)alVs) + "|" + postTessFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt) + "|" + std::to_string(controlPoints);
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:postTessFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        MTLRenderPipelineDescriptor* d = basePipelineDesc(vs, fs, fmt, false);
        // Tessellation: control points bước theo PerPatchControlPoint (PerVertex bị
        // validation từ chối với post-tessellation vertex function).
        d.vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerPatchControlPoint;
        d.tessellationPartitionMode = MTLTessellationPartitionModeInteger;
        d.maxTessellationFactor = 64;
        d.tessellationFactorScaleEnabled = NO;
        d.tessellationFactorFormat = MTLTessellationFactorFormatHalf;
        d.tessellationControlPointIndexType = MTLTessellationControlPointIndexTypeNone;
        d.tessellationFactorStepFunction = MTLTessellationFactorStepFunctionPerPatch;
        NSError* e = nil;
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pso) {
            if (log_ && e) log_("tess pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso);
    }
    std::shared_ptr<IRenderTarget> makeRenderTarget(uint32_t w, uint32_t h, PixelFormat f) override {
        auto t = std::dynamic_pointer_cast<AppleTexture>(newTexture(w, h, f));
        if (!t) return nullptr;
        return std::make_shared<AppleTarget>(t->get(), f, nil, nil, w, h, false, false);
    }
    // Depth first-clear PER-TEXTURE (không per-wrapper): wrapper tạo mới mỗi
    // draw mà flag ở wrapper thì mọi encoder mới đều force-clear depth → mất
    // occlusion giữa frame (mây/nước/entity nhìn xuyên tường = X-RAY).
    bool takeDepthInit(id<MTLTexture> t) {
        if (!t) return false;
        std::lock_guard<std::mutex> l(depthInit_->mu);
        void* key = (__bridge void*)t;
        if (depthInit_->inited.count(key)) return false;
        depthInit_->inited.insert(key);
        return true;
    }
    std::shared_ptr<IRenderTarget> wrapAsTarget(ITexture* color, ITexture* depth) override {
        AppleTexture* c = dynamic_cast<AppleTexture*>(color);
        AppleTexture* d = dynamic_cast<AppleTexture*>(depth);
        if (!c && !d) return nullptr;
        // Depth-only FBO (clearDepthTexture: glFramebufferTexture2D color=0) →
        // target không color; makeClearEncoder gắn depth-attachment không color.
        if (!c) return std::make_shared<AppleTarget>(nil, d->pixelFormat(), nil, d->get(),
                                                     d->width(), d->height(), false,
                                                     takeDepthInit(d->get()));
        return std::make_shared<AppleTarget>(c->get(), c->pixelFormat(), nil,
                                             d ? d->get() : nil, c->width(), c->height(), false,
                                             d ? takeDepthInit(d->get()) : false);
    }
    void setDefaultRenderTarget(std::shared_ptr<IRenderTarget> t) override { defaultTarget_ = t; }
    std::shared_ptr<IRenderTarget> defaultRenderTarget() override { return defaultTarget_; }
    bool presentTarget(IRenderTarget* target, void* metalLayer) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        if (!at || !metalLayer || !queue_) return false;
        CAMetalLayer* layer = (__bridge CAMetalLayer*)metalLayer;
        @try {
            id<CAMetalDrawable> drawable = [layer nextDrawable];
            if (!drawable) {
                if (log_) log_("presentTarget: nextDrawable nil (layer chưa vào window?)");
                return false;
            }
            id<MTLCommandBuffer> cb = [queue_ commandBuffer];
            if (!cb) {
                if (log_) log_("presentTarget: commandBuffer nil");
                return false;
            }
            // Copy min(target, drawable) — lệch size (xoay màn hình) thì letterbox
            // phần còn lại thay vì overrun validation. Cùng RGBA8Unorm.
            NSUInteger dw = drawable.texture.width, dh = drawable.texture.height;
            NSUInteger cw = at->width() < dw ? at->width() : dw;
            NSUInteger ch = at->height() < dh ? at->height() : dh;
            // Chẩn đoán tỉ lệ màn hình: log size drawable vs target (3 lần đầu +
            // khi lệch — lệch là zoom/crop toàn màn hình ở khâu present).
            {
                static int nPres = 0;
                if ((++nPres <= 3 || at->width() != dw || at->height() != dh) && log_) {
                    char b[128];
                    snprintf(b, sizeof(b),
                             "[TGLMT] present #%d drawable=%lux%lu target=%ux%u copy=%lux%lu",
                             nPres, (unsigned long)dw, (unsigned long)dh, at->width(),
                             at->height(), (unsigned long)cw, (unsigned long)ch);
                    log_(b);
                }
            }
            if (cw == 0 || ch == 0) return false;
            if (drawable.texture.pixelFormat != at->resolve().pixelFormat) {
                if (log_) log_("presentTarget: drawable/target format lệch");
                fprintf(stderr, "[TGLMT] presentTarget FORMAT MISMATCH drawable=%u target=%u\n",
                        (unsigned)drawable.texture.pixelFormat,
                        (unsigned)at->resolve().pixelFormat);
                fflush(stderr);
                return false;
            }
            // Memory target = GL-order (hàng 0 = đáy GL); drawable hàng 0 = trên
            // màn hình → present phải LẬT DỌC.
            // Bản cũ: loop từng hàng (1242 lệnh copyFromTexture / frame ở
            // 2208x1242) — mỗi lệnh là 1 blit command có setup cost riêng nên
            // nuốt phần lớn frame time trên A11 (đo được ~90ms/frame).
            // Nay: 1 render pass + 1 fullscreen triangle lấy texture với v đảo —
            // ~1 draw thay vì ~1242 blit.
            if (!EnsureFlipPipeline(at->pixelFormat())) {
                // Fallback: blit từng hàng (chậm hơn nhiều nhưng LUÔN đúng hình).
                // Chỉ xảy ra nếu Metal từ chối shader flip — không bao giờ màn đen.
                if (log_) log_("presentTarget: flip pipeline unavailable → row blit");
                return PresentRowBlit(cb, at, drawable, cw, ch);
            }
            MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
            rp.colorAttachments[0].texture = drawable.texture;
            rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
            id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
            if (!enc) return false;
            [enc setRenderPipelineState:flipPipe_];
            MTLViewport vp = {0.0, 0.0, (double)dw, (double)dh, 0.0, 1.0};
            [enc setViewport:vp];
            [enc setFragmentTexture:at->resolve() atIndex:0];
            if (flipSampler_) [enc setFragmentSamplerState:flipSampler_ atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
            // Đọc texture TRƯỚC present: sau present, drawable.texture là
            // không hợp lệ (log "should not be called after already presenting").
            id<MTLTexture> dtex = drawable.texture;
            [cb presentDrawable:drawable];
            // Handler + đếm cho MỌI frame present (không chỉ 3 frame probe):
            // thiếu → cbCommitted - cbCompleted âm/dương sai, ThrottleGpu phải
            // CommitAndWait mỗi frame.
            if (cbCompleted_)
                [cb addCompletedHandler:^(id<MTLCommandBuffer>) {
                    cbCompleted_->fetch_add(1, std::memory_order_relaxed);
                }];
            NoteCommit(cb);
            // PHẢI commit ở đây cho mọi frame. Nhánh probe trước đây nuốt
            // commit → frame ≥ 4 không bao giờ present (màn đóng băng đúng
            // loading frame 3) và drawable giữ bởi CB không commit làm
            // nextDrawable chặn vô hạn → treo hình.
            [cb commit];
            // Tự kiểm: 3 frame đầu, đọc ngược 1 pixel giữa drawable sau khi
            // GPU xong và so với pixel nguồn. Nếu lệch (hoặc đen) thì flip
            // hỏng trên máy này → log rõ thay vì chỉ hiện "màn đen" giống
            // nhau. Chỉ 3 lần, không ảnh hưởng throughput.
            const bool probe = (presentProbe_ < 3);
            if (probe) {
                [cb waitUntilCompleted];
                ++presentProbe_;
                uint8_t got[4] = {0, 0, 0, 0}, want[4] = {0, 0, 0, 0}, notWant[4] = {0, 0, 0, 0};
                // Drawable row 0 = trên; nguồn row 0 = đáy GL ⇒ điểm ở giữa
                // drawable phải khớp pixel ở hàng ĐẢO của nguồn.
                NSUInteger hSrc = (NSUInteger)at->height(), wSrc = (NSUInteger)at->width();
                NSUInteger sy = (hSrc > 1) ? (hSrc - 1 - (NSUInteger)(dh / 2)) : 0;
                NSUInteger sx = (wSrc > 1) ? (NSUInteger)(dw / 2) : 0;
                if (sx >= wSrc) sx = wSrc - 1;
                if (sy >= hSrc) sy = hSrc - 1;
                @try {
                    [dtex getBytes:got bytesPerRow:4
                         fromRegion:MTLRegionMake2D((NSUInteger)(dw / 2), (NSUInteger)(dh / 2), 1, 1)
                      mipmapLevel:0];
                    [at->resolve() getBytes:want bytesPerRow:4
                                fromRegion:MTLRegionMake2D(sx, sy, 1, 1) mipmapLevel:0];
                    // cùng cách nhưng KHÔNG lật → phải KHÁC (nếu bằng nhau thì
                    // flip không làm gì).
                    NSUInteger ny = (hSrc > 1) ? (NSUInteger)(dh / 2) : 0;
                    if (ny >= hSrc) ny = hSrc - 1;
                    [at->resolve() getBytes:notWant bytesPerRow:4
                                fromRegion:MTLRegionMake2D(sx, ny, 1, 1) mipmapLevel:0];
                } @catch (NSException*) {}
                // Chỉ kết luận "flip hỏng" khi drawable ĐEN trong khi nguồn có
                // màu: đó đúng là triệu chứng màn đen. Không so trung tâm ảnh
                // (gradient nên ±1 mức xanh là bình thường và gây báo động giả
                // → tự ép về row-blit chậm).
                const bool drawableBlack = (got[0] | got[1] | got[2] | got[3]) == 0;
                const bool srcHasColor = (want[0] | want[1] | want[2] | want[3]) != 0;
                const bool ok = !(drawableBlack && srcHasColor);
                if (log_) {
                    char b[224];
                    snprintf(b, sizeof(b),
                             "[TGLMT] presentflipcheck drawable=(%u,%u,%u,%u) "
                             "src-mirror=(%u,%u,%u,%u) src-plain=(%u,%u,%u,%u) -> %s",
                             got[0], got[1], got[2], got[3], want[0], want[1], want[2],
                             want[3], notWant[0], notWant[1], notWant[2], notWant[3],
                             ok ? "OK (flip dung)"
                                : (got[0] == notWant[0] && got[1] == notWant[1])
                                      ? "KHONG FLIP -> row blit"
                                      : "MA THAY -> row blit");
                    log_(b);
                }
                if (!ok) {
                    flipFmt_ = 0; // ép fallback ở các frame sau
                    flipPipe_ = nil;
                }
                return true;
            }
            return true;
        } @catch (NSException*) { return false; }
    }
    std::shared_ptr<ITexture> newTextureWithBytes(uint32_t w, uint32_t h, PixelFormat f,
            const void* data, size_t bytesPerRow) override {
        auto t = std::dynamic_pointer_cast<AppleTexture>(newTexture(w, h, f));
        if (!t || !data) return t;
        @try {
            [t->get() replaceRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0
                          withBytes:data bytesPerRow:bytesPerRow];
        } @catch (NSException*) { return nullptr; }
        return t;
    }
    std::shared_ptr<ITexture> newDepthTexture(uint32_t w, uint32_t h, PixelFormat f) override {
        MTLPixelFormat m = ToMTL(f);
        if (m == MTLPixelFormatInvalid) return nullptr;
        MTLTextureDescriptor* d = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:m
                                                width:w height:h mipmapped:NO];
        d.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        d.storageMode = MTLStorageModePrivate; // depth không readback trực tiếp
        id<MTLTexture> t = [dev_ newTextureWithDescriptor:d];
        if (!t) return nullptr;
        auto tex = std::make_shared<AppleTexture>(t, w, h, f);
        tex->trackDepthInit(depthInit_);
        return tex;
    }
    std::shared_ptr<IRenderTarget> makeRenderTargetWithDepth(uint32_t w, uint32_t h,
            PixelFormat colorFmt) override {
        auto c = std::dynamic_pointer_cast<AppleTexture>(newTexture(w, h, colorFmt));
        auto dep = std::dynamic_pointer_cast<AppleTexture>(
            newDepthTexture(w, h, PixelFormat::Depth32Float));
        if (!c || !dep) return nullptr;
        return std::make_shared<AppleTarget>(c->get(), colorFmt, nil, dep->get(), w, h, false,
                                             true /* fresh depth */);
    }
    std::shared_ptr<IRenderTarget> makeMSAATarget(uint32_t w, uint32_t h, uint32_t samples) override {
        if (samples < 2) return makeRenderTarget(w, h, PixelFormat::RGBA8Unorm);
        MTLTextureDescriptor* md = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:
                                    MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];
        md.textureType = MTLTextureType2DMultisample;
        md.sampleCount = samples;
        md.usage = MTLTextureUsageRenderTarget;
        md.storageMode = MTLStorageModePrivate;
        id<MTLTexture> msaa = [dev_ newTextureWithDescriptor:md];
        auto res = std::dynamic_pointer_cast<AppleTexture>(
            newTexture(w, h, PixelFormat::RGBA8Unorm));
        if (!msaa || !res) return nullptr;
        return std::make_shared<AppleTarget>(msaa, PixelFormat::RGBA8Unorm, res->get(), nil, w, h, true,
                                             false /* MSAA không depth */);
    }
    std::shared_ptr<IRenderPipeline> makeMSAAPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt, uint32_t samples) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !vsFn || !fsFn || samples < 2) return nullptr;
        std::string key = "msaa|" + std::to_string(samples) + "|" +
                          std::to_string((uintptr_t)alVs) + "|" + vsFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt);
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:vsFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        MTLRenderPipelineDescriptor* d = basePipelineDesc(vs, fs, fmt, false);
        d.rasterSampleCount = samples;
        NSError* e = nil;
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pso) {
            if (log_ && e) log_("msaa pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso);
    }
    bool supportsMesh() override {
        // supportsFamily: có từ macOS 10.15; family lạ với OS cũ trả NO an toàn.
        // A11 (Apple4) → false đúng (mesh cần Apple7+ = A17/M3). Vanilla không cần mesh.
        return [dev_ supportsFamily:MTLGPUFamilyApple7];
    }
    bool blitCopy(ITexture* src, ITexture* dst,
            uint32_t sx, uint32_t sy, uint32_t w, uint32_t h,
            uint32_t dx, uint32_t dy) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(src);
        AppleTexture* b = dynamic_cast<AppleTexture*>(dst);
        if (!a || !b || !w || !h) return false;
        if (a->pixelFormat() != b->pixelFormat()) return false; // khác format → caller fallback CPU
        return blitTex(a->get(), b->get(), sx, sy, w, h, dx, dy);
    }
    // Helper dùng chung cho blitCopy + blitTo/FromTarget (target cũng là texture).
    bool blitTex(id<MTLTexture> s, id<MTLTexture> d, uint32_t sx, uint32_t sy, uint32_t w,
                 uint32_t h, uint32_t dx, uint32_t dy) {
        if (!s || !d || !queue_ || !w || !h) return false;
        if (s.pixelFormat != d.pixelFormat) return false;
        @try {
            id<MTLCommandBuffer> cb = [queue_ commandBuffer];
            if (!cb) return false;
            id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
            if (!blit) return false;
            MTLOrigin so = {(NSUInteger)sx, (NSUInteger)sy, 0};
            MTLSize ss = {(NSUInteger)w, (NSUInteger)h, 1};
            MTLOrigin dd = {(NSUInteger)dx, (NSUInteger)dy, 0};
            [blit copyFromTexture:s sourceSlice:0 sourceLevel:0 sourceOrigin:so sourceSize:ss
                        toTexture:d destinationSlice:0 destinationLevel:0 destinationOrigin:dd];
            [blit endEncoding];
            if (cbCompleted_)
                [cb addCompletedHandler:^(id<MTLCommandBuffer>) {
                    cbCompleted_->fetch_add(1, std::memory_order_relaxed);
                }];
            NoteCommit(cb);
            [cb commit];
            [cb waitUntilCompleted];
            return [cb status] == MTLCommandBufferStatusCompleted;
        } @catch (NSException*) { return false; }
    }
    bool blitToTarget(ITexture* src, IRenderTarget* dst, uint32_t sx, uint32_t sy, uint32_t w,
                      uint32_t h, uint32_t dx, uint32_t dy) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(src);
        AppleTarget* t = dynamic_cast<AppleTarget*>(dst);
        if (!a || !t || !w || !h) return false;
        if (a->pixelFormat() != t->pixelFormat()) return false;
        return blitTex(a->get(), t->color(), sx, sy, w, h, dx, dy);
    }
    bool blitFromTarget(IRenderTarget* src, ITexture* dst, uint32_t sx, uint32_t sy, uint32_t w,
                        uint32_t h, uint32_t dx, uint32_t dy) override {
        AppleTarget* t = dynamic_cast<AppleTarget*>(src);
        AppleTexture* b = dynamic_cast<AppleTexture*>(dst);
        if (!t || !b || !w || !h) return false;
        if (t->pixelFormat() != b->pixelFormat()) return false;
        return blitTex(t->resolve(), b->get(), sx, sy, w, h, dx, dy);
    }
    bool generateMipmaps(ITexture* tex) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(tex);
        if (!a || !queue_) return false;
        @try {
            // Chỉ texture mipmapped mới sinh được; non-mipmapped → false để GL fallback base-level.
            if ([a->get() mipmapLevelCount] <= 1) return false;
            id<MTLCommandBuffer> cb = [queue_ commandBuffer];
            if (!cb) return false;
            id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
            if (!blit) return false;
            [blit generateMipmapsForTexture:a->get()];
            [blit endEncoding];
            [cb commit];
            [cb waitUntilCompleted];
            return [cb status] == MTLCommandBufferStatusCompleted;
        } @catch (NSException*) { return false; }
    }
    bool updateTexture(ITexture* tex, uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, const void* data, size_t bytesPerRow) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(tex);
        if (!a || !data || !w || !h) return false;
        @try {
            [a->get() replaceRegion:MTLRegionMake2D(x, y, w, h) mipmapLevel:0
                          withBytes:data bytesPerRow:bytesPerRow];
            return true;
        } @catch (NSException*) { return false; }
    }
    std::shared_ptr<ITexture> newCubeTexture(uint32_t size, PixelFormat f) override {
        if (!size) return nullptr;
        MTLPixelFormat m = ToMTL(f);
        if (m == MTLPixelFormatInvalid) return nullptr;
        // Chỉ RGBA8/R8 cubemap (panorama vanilla); depth/cube không hỗ trợ.
        if (m != MTLPixelFormatRGBA8Unorm && m != MTLPixelFormatRGBA8Unorm_sRGB &&
            m != MTLPixelFormatR8Unorm)
            return nullptr;
        MTLTextureDescriptor* d = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:m
                                                 size:size mipmapped:NO];
        d.usage = MTLTextureUsageShaderRead;
        d.storageMode = MTLStorageModeShared;
        id<MTLTexture> t = nil;
        @try { t = [dev_ newTextureWithDescriptor:d]; } @catch (NSException*) { return nullptr; }
        if (!t) return nullptr;
        return std::make_shared<AppleTexture>(t, size, size, f);
    }
    bool updateCubeFace(ITexture* tex, uint32_t face, const void* data,
            size_t bytesPerRow) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(tex);
        if (!a || !data || face >= 6) return false;
        @try {
            if ([a->get() textureType] != MTLTextureTypeCube) return false;
            uint32_t s = a->width();
            [a->get() replaceRegion:MTLRegionMake2D(0, 0, s, s) mipmapLevel:0 slice:face
                          withBytes:data bytesPerRow:bytesPerRow bytesPerImage:bytesPerRow * s];
            return true;
        } @catch (NSException*) { return false; }
    }
    std::shared_ptr<IRenderPipeline> makeMeshPipeline(ILibrary* meshLib, const char* meshFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt) override {
        AppleLibrary* alMesh = dynamic_cast<AppleLibrary*>(meshLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alMesh || !alFs || !meshFn || !fsFn) return nullptr;
        // Gate runtime: OS cũ không có class/API mesh (trả nil → test SKIP, không crash).
        Class cls = NSClassFromString(@"MTLMeshRenderPipelineDescriptor");
        if (!cls) return nullptr;
        SEL sel = @selector(newRenderPipelineStateWithMeshDescriptor:options:reflection:error:);
        if (![dev_ respondsToSelector:sel]) return nullptr;
        std::string key = "mesh|" + std::to_string((uintptr_t)alMesh) + "|" + meshFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt);
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second);
        }
        id<MTLFunction> mf = [alMesh->get() newFunctionWithName:[NSString stringWithUTF8String:meshFn]];
        id<MTLFunction> ff = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!mf || !ff) return nullptr;
        MTLMeshRenderPipelineDescriptor* desc = [[cls alloc] init];
        desc.meshFunction = mf;
        desc.fragmentFunction = ff;
        desc.colorAttachments[0].pixelFormat = ToMTL(fmt);
        NSError* e = nil;
        id<MTLRenderPipelineState> pso =
            [dev_ newRenderPipelineStateWithMeshDescriptor:desc
                                                  options:MTLPipelineOptionNone
                                               reflection:nil
                                                    error:&e];
        if (!pso) {
            if (log_ && e) log_("mesh pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso);
    }
    std::shared_ptr<IComputePipeline> makeComputePipeline(ILibrary* lib, const char* fn) override {
        AppleLibrary* al = dynamic_cast<AppleLibrary*>(lib);
        if (!al || !fn) return nullptr;
        id<MTLFunction> f = [al->get() newFunctionWithName:[NSString stringWithUTF8String:fn]];
        if (!f) return nullptr;
        NSError* e = nil;
        id<MTLComputePipelineState> pso = [dev_ newComputePipelineStateWithFunction:f error:&e];
        if (!pso) {
            if (log_ && e) log_("compute pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        return std::make_shared<AppleComputePipeline>(pso);
    }
    // (size,type,norm) GL → MTLVertexFormat. Combo ngoài bảng → Invalid (báo nil).
    // A11: bao phủ mọi combo vanilla/Sodium hay dùng (UBYTE/USHORT/SHORT/HALF/UINT/INT).
    MTLVertexFormat ToMTLVertexFormat(uint32_t size, uint32_t type, bool norm) {
        if (type == 0x1406) { // FLOAT
            switch (size) {
                case 1: return MTLVertexFormatFloat;
                case 2: return MTLVertexFormatFloat2;
                case 3: return MTLVertexFormatFloat3;
                case 4: return MTLVertexFormatFloat4;
                default: break;
            }
        } else if (type == 0x1401) { // UNSIGNED_BYTE
            switch (size) {
                case 2: return norm ? MTLVertexFormatUChar2Normalized : MTLVertexFormatUChar2;
                case 3: return norm ? MTLVertexFormatUChar3Normalized : MTLVertexFormatUChar3;
                case 4: return norm ? MTLVertexFormatUChar4Normalized : MTLVertexFormatUChar4;
                default: break;
            }
        } else if (type == 0x1400) { // BYTE
            switch (size) {
                case 2: return norm ? MTLVertexFormatChar2Normalized : MTLVertexFormatChar2;
                case 3: return norm ? MTLVertexFormatChar3Normalized : MTLVertexFormatChar3;
                case 4: return norm ? MTLVertexFormatChar4Normalized : MTLVertexFormatChar4;
                default: break;
            }
        } else if (type == 0x1403) { // UNSIGNED_SHORT
            switch (size) {
                case 2: return norm ? MTLVertexFormatUShort2Normalized : MTLVertexFormatUShort2;
                case 3: return norm ? MTLVertexFormatUShort3Normalized : MTLVertexFormatUShort3;
                case 4: return norm ? MTLVertexFormatUShort4Normalized : MTLVertexFormatUShort4;
                default: break;
            }
        } else if (type == 0x1402) { // SHORT
            switch (size) {
                case 2: return norm ? MTLVertexFormatShort2Normalized : MTLVertexFormatShort2;
                case 3: return norm ? MTLVertexFormatShort3Normalized : MTLVertexFormatShort3;
                case 4: return norm ? MTLVertexFormatShort4Normalized : MTLVertexFormatShort4;
                default: break;
            }
        } else if (type == 0x140B) { // HALF_FLOAT
            switch (size) {
                case 2: return MTLVertexFormatHalf2;
                case 3: return MTLVertexFormatHalf3;
                case 4: return MTLVertexFormatHalf4;
                default: break;
            }
        } else if (type == 0x1405) { // UNSIGNED_INT
            switch (size) {
                case 1: return MTLVertexFormatUInt;
                case 2: return MTLVertexFormatUInt2;
                case 3: return MTLVertexFormatUInt3;
                case 4: return MTLVertexFormatUInt4;
                default: break;
            }
        } else if (type == 0x1404) { // INT
            switch (size) {
                case 1: return MTLVertexFormatInt;
                case 2: return MTLVertexFormatInt2;
                case 3: return MTLVertexFormatInt3;
                case 4: return MTLVertexFormatInt4;
                default: break;
            }
        }
        return MTLVertexFormatInvalid;
    }
    std::shared_ptr<IRenderPipeline> makeCustomPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt,
            const CustomAttrib* attribs, uint32_t nAttribs, uint32_t stride,
            const PipelineOpts* opts = nullptr) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !vsFn || !fsFn) return nullptr;
        // nAttribs==0: draw suy đỉnh từ vertex_id (screenquad), descriptor rỗng hợp lệ.
        if (nAttribs > 0 && !attribs) return nullptr;
        MTLVertexDescriptor* vd = [MTLVertexDescriptor vertexDescriptor];
        bool layoutSeen[31] = {false};
        uint32_t layoutDivisor[31] = {0};
        for (uint32_t i = 0; i < nAttribs && i < 16; ++i) {
            MTLVertexFormat f = ToMTLVertexFormat(attribs[i].size, attribs[i].type,
                                                 attribs[i].normalized);
            if (f == MTLVertexFormatInvalid) return nullptr;
            if (attribs[i].loc >= 31 || attribs[i].bufferIndex >= 31) return nullptr;
            vd.attributes[attribs[i].loc].format = f;
            vd.attributes[attribs[i].loc].offset = attribs[i].offset;
            vd.attributes[attribs[i].loc].bufferIndex = attribs[i].bufferIndex;
            if (!layoutSeen[attribs[i].bufferIndex]) {
                layoutSeen[attribs[i].bufferIndex] = true;
                vd.layouts[attribs[i].bufferIndex].stride =
                    attribs[i].stride ? attribs[i].stride : stride;
                layoutDivisor[attribs[i].bufferIndex] = attribs[i].divisor;
            } else {
                // cùng binding khác divisor → Metal không hỗ trợ (1 stepFunction/layout).
                // Giữ divisor đầu, log để chẩn đoán (vanilla không gặp).
                if (layoutDivisor[attribs[i].bufferIndex] != attribs[i].divisor && log_)
                    log_("makeCustomPipeline: binding divisor mismatch, giữ cái đầu");
            }
        }
        // Áp stepFunction PerInstance cho binding có divisor>0 (instancing chunk/entity).
        for (int b = 0; b < 31; ++b) if (layoutSeen[b]) {
            if (layoutDivisor[b] > 0) {
                vd.layouts[b].stepFunction = MTLVertexStepFunctionPerInstance;
                vd.layouts[b].stepRate = layoutDivisor[b];
            } else {
                vd.layouts[b].stepFunction = MTLVertexStepFunctionPerVertex;
                vd.layouts[b].stepRate = 1;
            }
        }
        std::string key = "custom|" + std::to_string((uintptr_t)alVs) + "|" + vsFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt) + "|" + std::to_string(stride) + "|";
        for (uint32_t i = 0; i < nAttribs; ++i)
            key += std::to_string(attribs[i].loc) + "," + std::to_string(attribs[i].size) +
                   "," + std::to_string(attribs[i].type) + "," +
                   std::to_string(attribs[i].offset) + "," +
                   std::to_string(attribs[i].bufferIndex) + "," +
                   std::to_string(attribs[i].divisor) + "," +
                   std::to_string(attribs[i].stride) + ";";
        bool useDepth = opts && opts->depth;
        bool useBlend = opts && opts->blend;
        uint32_t writeMask = opts ? opts->colorWriteMask : 0xFu;
        key += useDepth ? "D" : "-";
        key += "W" + std::to_string(writeMask);
        if (useBlend) {
            const AttachmentBlend& b = opts->blend0;
            key += "B" + std::to_string(b.srcRGB) + "," + std::to_string(b.dstRGB) + "," +
                   std::to_string(b.srcAlpha) + "," + std::to_string(b.dstAlpha) + "," +
                   std::to_string(b.rgbOp) + "," + std::to_string(b.alphaOp);
        }
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second, useDepth);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:vsFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
        d.vertexFunction = vs; d.fragmentFunction = fs;
        d.vertexDescriptor = vd;
        d.colorAttachments[0].pixelFormat = ToMTL(fmt);
        if (useDepth) d.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
        // glColorMask bits R=1 G=2 B=4 A=8 khớp MTLColorWriteMask (Red=1 ...).
        d.colorAttachments[0].writeMask = (MTLColorWriteMask)(writeMask & 0xFu);
        if (useBlend) {
            const AttachmentBlend& b = opts->blend0;
            d.colorAttachments[0].blendingEnabled = YES;
            d.colorAttachments[0].sourceRGBBlendFactor = ToMTLBlend(b.srcRGB);
            d.colorAttachments[0].destinationRGBBlendFactor = ToMTLBlend(b.dstRGB);
            d.colorAttachments[0].sourceAlphaBlendFactor = ToMTLBlend(b.srcAlpha);
            d.colorAttachments[0].destinationAlphaBlendFactor = ToMTLBlend(b.dstAlpha);
            d.colorAttachments[0].rgbBlendOperation = ToMTLBlendOp(b.rgbOp);
            d.colorAttachments[0].alphaBlendOperation = ToMTLBlendOp(b.alphaOp);
        }
        NSError* e = nil;
        // Chẩn đoán đen màn hình: PSO compile đồng bộ trên render thread, A11 có
        // thể mất hàng giây/shader đầu. Log mỗi lần miss để latestlog thấy được
        // game có đang kẹt ở compile hay không.
        auto t0 = std::chrono::steady_clock::now();
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        long ms =
            (long)std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();
        static int nCustomPSO = 0;
        if (log_ && (++nCustomPSO <= 60 || ms > 200))
            log_("[TGLMT] customPSO #" + std::to_string(nCustomPSO) + " " + vsFn + "+" +
                 fsFn + " attrs=" + std::to_string(nAttribs) + " " + std::to_string(ms) +
                 "ms" + (pso ? "" : " NIL"));
        if (!pso) {
            if (log_ && e) log_("custom pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso, useDepth);
    }
    std::shared_ptr<IComputeEncoder> makeComputeEncoder() override {
        if (!queue_) return nullptr;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return nullptr;
        return std::make_shared<AppleComputeEncoder>(cb);
    }
    std::shared_ptr<IDepthStencilState> makeDepthStencilState(uint32_t func, bool write) override {
        // IR cache: chỉ 8 compare func × 2 writeMask = 16 trạng thái. Trước đây mỗi draw
        // tạo mới 1 MTLDepthStencilState (1 GL → 1 Metal alloc). Giờ cache vĩnh viễn.
        uint64_t key = ((uint64_t)func << 1) | (write ? 1u : 0u);
        {
            std::lock_guard<std::mutex> l(dmu_);
            auto it = dcache_.find(key);
            if (it != dcache_.end()) return std::make_shared<AppleDepthStencil>(it->second);
        }
        MTLDepthStencilDescriptor* d = [[MTLDepthStencilDescriptor alloc] init];
        d.depthCompareFunction = ToMTLCompare(func);
        d.depthWriteEnabled = write ? YES : NO;
        id<MTLDepthStencilState> s = [dev_ newDepthStencilStateWithDescriptor:d];
        if (!s) return nullptr;
        { std::lock_guard<std::mutex> l(dmu_); dcache_[key] = s; }
        return std::make_shared<AppleDepthStencil>(s);
    }
    std::shared_ptr<ISamplerState> makeSampler(const SamplerDesc& sd) override {
        MTLSamplerDescriptor* d = [[MTLSamplerDescriptor alloc] init];
        d.minFilter = ToMTLFilter(sd.minFilter);
        d.magFilter = ToMTLFilter(sd.magFilter);
        d.sAddressMode = ToMTLWrap(sd.sWrap);
        d.tAddressMode = ToMTLWrap(sd.tWrap);
        // Mipmap linear khi đủ levels; texture 1 level thì tắt lọc mip để
        // A11 không fetch LOD>0 (fault/đen với minfilter mipmap).
        d.mipFilter = sd.noMip ? MTLSamplerMipFilterNotMipmapped : MTLSamplerMipFilterLinear;
        d.maxAnisotropy = (NSUInteger)(sd.maxAniso >= 1.0f ? sd.maxAniso : 1);
        // LOD clamp theo GL_TEXTURE_MIN_LOD/MAX_LOD (sampler) + BASE/MAX_LEVEL
        // (texture). Minecraft set MAX_LOD=0 cho sampler không mipmap và
        // BASE/MAX_LEVEL=0 cho texture mỗi draw; bỏ qua thì Metal clamp 0..1000.
        d.lodMinClamp = sd.lodMin < 0.0f ? 0.0f : sd.lodMin;
        d.lodMaxClamp = sd.lodMax < d.lodMinClamp ? d.lodMinClamp : sd.lodMax;
        if (sd.noMip) { d.lodMinClamp = 0.0f; d.lodMaxClamp = 0.0f; }
        id<MTLSamplerState> s = [dev_ newSamplerStateWithDescriptor:d];
        return s ? std::make_shared<AppleSampler>(s) : nullptr;
    }
    std::shared_ptr<IRenderPipeline> makeBlendPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat fmt,
            const AttachmentBlend* blends, uint32_t blendCount) override {
        AppleLibrary* alVs = dynamic_cast<AppleLibrary*>(vsLib);
        AppleLibrary* alFs = dynamic_cast<AppleLibrary*>(fsLib);
        if (!alVs || !alFs || !vsFn || !fsFn) return nullptr;
        std::string key = "blend|" + std::to_string((uintptr_t)alVs) + "|" + vsFn + "|" +
                          std::to_string((uintptr_t)alFs) + "|" + fsFn + "|" +
                          std::to_string((uint32_t)fmt) + "|";
        for (uint32_t i = 0; i < blendCount && i < 8; ++i) {
            const AttachmentBlend& b = blends[i];
            key += b.enabled ? "1" : "0";
            key += "," + std::to_string(b.srcRGB) + "," + std::to_string(b.dstRGB) + "," +
                   std::to_string(b.srcAlpha) + "," + std::to_string(b.dstAlpha) + "," +
                   std::to_string(b.rgbOp) + "," + std::to_string(b.alphaOp) + ";";
        }
        {
            std::lock_guard<std::mutex> l(pmu_);
            auto it = pcache_.find(key);
            if (it != pcache_.end()) return std::make_shared<ApplePipeline>(it->second);
        }
        id<MTLFunction> vs = [alVs->get() newFunctionWithName:[NSString stringWithUTF8String:vsFn]];
        id<MTLFunction> fs = [alFs->get() newFunctionWithName:[NSString stringWithUTF8String:fsFn]];
        if (!vs || !fs) return nullptr;
        MTLRenderPipelineDescriptor* d = basePipelineDesc(vs, fs, fmt, false);
        for (uint32_t i = 0; i < blendCount && i < 8; ++i) {
            const AttachmentBlend& b = blends[i];
            d.colorAttachments[i].blendingEnabled = b.enabled ? YES : NO;
            d.colorAttachments[i].sourceRGBBlendFactor = ToMTLBlend(b.srcRGB);
            d.colorAttachments[i].destinationRGBBlendFactor = ToMTLBlend(b.dstRGB);
            d.colorAttachments[i].sourceAlphaBlendFactor = ToMTLBlend(b.srcAlpha);
            d.colorAttachments[i].destinationAlphaBlendFactor = ToMTLBlend(b.dstAlpha);
            d.colorAttachments[i].rgbBlendOperation = ToMTLBlendOp(b.rgbOp);
            d.colorAttachments[i].alphaBlendOperation = ToMTLBlendOp(b.alphaOp);
        }
        NSError* e = nil;
        id<MTLRenderPipelineState> pso = [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
        if (!pso) {
            if (log_ && e) log_("blend pipeline error: " + std::string([[e localizedDescription] UTF8String]));
            return nullptr;
        }
        { std::lock_guard<std::mutex> l(pmu_); pcache_[key] = pso; }
        return std::make_shared<ApplePipeline>(pso);
    }
    std::shared_ptr<IRenderEncoder> makeRenderEncoder(IRenderTarget* target,
            IRenderPipeline* pipeline, const ClearColor& clear) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        ApplePipeline* ap = dynamic_cast<ApplePipeline*>(pipeline);
        if (!at || !ap || !queue_) return nullptr;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return nullptr;
        MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = at->color();
        rp.colorAttachments[0].loadAction = MTLLoadActionClear;
        if (at->isMSAA()) {
            // MSAA: render vào multisample texture, resolve về Shared texture để readback.
            rp.colorAttachments[0].resolveTexture = at->resolve();
            rp.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        } else {
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        rp.colorAttachments[0].clearColor = MTLClearColorMake(clear.r, clear.g, clear.b, clear.a);
        // Depth chỉ attach khi pipeline có depth format (TBDR strict sẽ fail nếu lệch).
        // Store (không DontCare) để pass sau Load được trên TBDR.
        if (at->depth() && ap->hasDepth()) {
            rp.depthAttachment.texture = at->depth();
            rp.depthAttachment.loadAction = MTLLoadActionClear;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
            rp.depthAttachment.clearDepth = 1.0;
        }
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        if (!enc) return nullptr;
        [enc setRenderPipelineState:ap->get()];
        return std::make_shared<AppleRenderEncoder>(cb, enc, at->resolve(), log_, diag_,
                                                        cbCommitted_, cbCompleted_);
    }
    static MTLLoadAction ToMTLLoad(LoadOp o) {
        switch (o) {
            case LoadOp::Load: return MTLLoadActionLoad;
            case LoadOp::DontCare: return MTLLoadActionDontCare;
            default: return MTLLoadActionClear;
        }
    }
    std::shared_ptr<IRenderEncoder> makeRenderEncoderActions(IRenderTarget* target,
            IRenderPipeline* pipeline, const ClearColor& clear, double clearDepth,
            LoadOp colorLoad, LoadOp depthLoad) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        ApplePipeline* ap = dynamic_cast<ApplePipeline*>(pipeline);
        if (!at || !ap || !queue_) return nullptr;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return nullptr;
        MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = at->color();
        rp.colorAttachments[0].loadAction = ToMTLLoad(colorLoad);
        if (at->isMSAA()) {
            rp.colorAttachments[0].resolveTexture = at->resolve();
            rp.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        } else {
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        rp.colorAttachments[0].clearColor = MTLClearColorMake(clear.r, clear.g, clear.b, clear.a);
        if (at->depth() && ap->hasDepth()) {
            MTLLoadAction dl = ToMTLLoad(depthLoad);
            if (at->takeDepthFirstClear()) dl = MTLLoadActionClear; // depth Private khởi đầu rác
            rp.depthAttachment.texture = at->depth();
            rp.depthAttachment.loadAction = dl;
            rp.depthAttachment.storeAction = MTLStoreActionStore; // TBDR: Load pass sau cần Store
            rp.depthAttachment.clearDepth = clearDepth;
        }
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        if (!enc) return nullptr;
        [enc setRenderPipelineState:ap->get()];
        return std::make_shared<AppleRenderEncoder>(cb, enc, at->resolve(), log_, diag_,
                                                        cbCommitted_, cbCompleted_);
    }
    std::shared_ptr<IRenderEncoder> makeRenderEncoderLoad(IRenderTarget* target,
            IRenderPipeline* pipeline) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        ApplePipeline* ap = dynamic_cast<ApplePipeline*>(pipeline);
        if (!at || !ap || !queue_) return nullptr;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return nullptr;
        MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
        rp.colorAttachments[0].texture = at->color();
        rp.colorAttachments[0].loadAction = MTLLoadActionLoad;
        if (at->isMSAA()) {
            rp.colorAttachments[0].resolveTexture = at->resolve();
            rp.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
        } else {
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
        }
        if (at->depth() && ap->hasDepth()) {
            MTLLoadAction dl = MTLLoadActionLoad;
            if (at->takeDepthFirstClear()) dl = MTLLoadActionClear;
            rp.depthAttachment.texture = at->depth();
            rp.depthAttachment.loadAction = dl;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
        }
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        if (!enc) return nullptr;
        [enc setRenderPipelineState:ap->get()];
        return std::make_shared<AppleRenderEncoder>(cb, enc, at->resolve(), log_, diag_,
                                                        cbCommitted_, cbCompleted_);
    }
    // glClear immediate: clear-ONLY pass (không pipeline, không draw). Tôn trọng
    // FBO đang bind; scissor truyền vào để test xác nhận loadAction có clip theo
    // scissor_rect hay không (Metal spec mơ hồ — integration test tự kiểm).
    std::shared_ptr<IRenderEncoder> makeClearEncoder(IRenderTarget* target,
            const ClearColor& clear, double clearDepth, LoadOp colorLoad, LoadOp depthLoad,
            const ScissorRect* scissor) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        if (!at || !queue_) return nullptr;
        bool doColor = (colorLoad == LoadOp::Clear) && at->color();
        bool doDepth = (depthLoad == LoadOp::Clear) && at->depth();
        if (!doColor && !doDepth) return nullptr;
        id<MTLCommandBuffer> cb = [queue_ commandBuffer];
        if (!cb) return nullptr;
        MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
        if (at->color()) {
            rp.colorAttachments[0].texture = at->color();
            rp.colorAttachments[0].loadAction =
                doColor ? MTLLoadActionClear : MTLLoadActionLoad; // depth-only: giữ color
            if (at->isMSAA()) {
                rp.colorAttachments[0].resolveTexture = at->resolve();
                rp.colorAttachments[0].storeAction = MTLStoreActionMultisampleResolve;
            } else {
                rp.colorAttachments[0].storeAction = MTLStoreActionStore;
            }
            rp.colorAttachments[0].clearColor =
                MTLClearColorMake(clear.r, clear.g, clear.b, clear.a);
        }
        if (doDepth) {
            rp.depthAttachment.texture = at->depth();
            rp.depthAttachment.loadAction = MTLLoadActionClear;
            rp.depthAttachment.storeAction = MTLStoreActionStore;
            rp.depthAttachment.clearDepth = clearDepth;
        }
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
        if (!enc) return nullptr;
        if (scissor && scissor->w > 0 && scissor->h > 0) {
            MTLScissorRect s = {(NSUInteger)scissor->x, (NSUInteger)scissor->y,
                                (NSUInteger)scissor->w, (NSUInteger)scissor->h};
            [enc setScissorRect:s];
        }
        return std::make_shared<AppleRenderEncoder>(cb, enc, at->resolve(), log_, diag_,
                                                        cbCommitted_, cbCompleted_);
    }
    // Fallback chặy: blit từng hàng (đồi để lẟt dắng độ để, đảm hình nhắng).
    bool flipCopy(ITexture* src, ITexture* dst) override {
        AppleTexture* a = dynamic_cast<AppleTexture*>(src);
        AppleTexture* b = dynamic_cast<AppleTexture*>(dst);
        if (!a || !b || !queue_) return false;
        if (a->pixelFormat() != b->pixelFormat()) return false;
        if (!EnsureFlipPipeline(a->pixelFormat())) return false;
        @try {
            id<MTLCommandBuffer> cb = [queue_ commandBuffer];
            if (!cb) return false;
            MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
            rp.colorAttachments[0].texture = b->get();
            rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
            rp.colorAttachments[0].storeAction = MTLStoreActionStore;
            id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];
            if (!enc) return false;
            [enc setRenderPipelineState:flipPipe_];
            MTLViewport vp = {0.0, 0.0, (double)b->width(), (double)b->height(), 0.0, 1.0};
            [enc setViewport:vp];
            [enc setFragmentTexture:a->get() atIndex:0];
            if (flipSampler_) [enc setFragmentSamplerState:flipSampler_ atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
            [enc endEncoding];
            NoteCommit(cb);
            [cb commit];
            [cb waitUntilCompleted];
            return [cb status] == MTLCommandBufferStatusCompleted;
        } @catch (NSException*) { return false; }
    }
    bool PresentRowBlit(id<MTLCommandBuffer> cb, AppleTarget* at, id<CAMetalDrawable> drawable,
                       NSUInteger cw, NSUInteger ch) {
        id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
        if (!blit) return false;
        for (NSUInteger i = 0; i < ch; ++i) {
            MTLOrigin so = {0, ch - 1 - i, 0};
            MTLOrigin dob = {0, i, 0};
            MTLSize sz = {cw, 1, 1};
            [blit copyFromTexture:at->resolve() sourceSlice:0 sourceLevel:0 sourceOrigin:so
                      sourceSize:sz toTexture:drawable.texture destinationSlice:0
                 destinationLevel:0 destinationOrigin:dob];
        }
        [blit endEncoding];
        [cb presentDrawable:drawable];
        if (cbCompleted_)
            [cb addCompletedHandler:^(id<MTLCommandBuffer>) {
                cbCompleted_->fetch_add(1, std::memory_order_relaxed);
            }];
        NoteCommit(cb);
        [cb commit];
        return true;
    }
    // ---- Present flip: 1 render pass thay v~1242 blit/h\u00e0ng ----------------
    // MSL: fullscreen triangle, uv.y đảo để đổi GL-order (row 0 = đáy) sang
    // drawable-order (row 0 = trên).
    static const char* kFlipMSL() {
        return
            "#include <metal_stdlib>\n"
            "using namespace metal;\n"
            // Sampler hằng số: dạng sampler(tex, filter::…, address::…) bị MSL
            // từ chối ("no matching constructor") vì đối số lọc/địa chỉ mơ hồ.
            "constexpr sampler kFlipSampler(filter::nearest, address::clamp_to_edge);\n"
            "struct VOut { float4 pos [[position]]; float2 uv; };\n"
            "vertex VOut tglmt_flip_vs(uint vid [[vertex_id]]) {\n"
            "  float2 p = float2(float((vid << 1) & 2), float(vid & 2));\n"
            "  VOut o;\n"
            "  o.pos = float4(p * 2.0 - 1.0, 0.0, 1.0);\n"
            // Metal: đỉnh đáy màn hình (p.y=0) phải lấy hàng ĐÁY của texture
            // (uv.y=1). Texture lưu theo thứ tự ảnh (hàng 0 = trên) nên cần
            // uv.y = p.y. Dùng 1.0-p.y sẽ úp ngược toàn bộ hình.
            "  o.uv = float2(p.x, p.y);\n"
            "  return o;\n"
            "}\n"
            "fragment float4 tglmt_flip_fs(VOut in [[stage_in]],\n"
            "                             texture2d<float> src [[texture(0)]]) {\n"
            "  return src.sample(kFlipSampler, in.uv);\n"
            "}\n";
    }
    bool EnsureFlipPipeline(PixelFormat fmt) {
        if (flipPipe_ && flipFmt_ == (uint32_t)fmt) return true;
        @try {
            NSError* e = nil;
            NSString* src = [NSString stringWithUTF8String:kFlipMSL()];
            id<MTLLibrary> lib = [dev_ newLibraryWithSource:src options:nil error:&e];
            if (!lib) {
                if (log_) log_("flipLib: " + std::string(e ? [[e localizedDescription] UTF8String]
                                                            : "nil"));
                return false;
            }
            id<MTLFunction> vs = [lib newFunctionWithName:@"tglmt_flip_vs"];
            id<MTLFunction> fs = [lib newFunctionWithName:@"tglmt_flip_fs"];
            if (!vs || !fs) {
                if (log_) log_("flipFn: vs/fs nil");
                return false;
            }
            MTLRenderPipelineDescriptor* d = [[MTLRenderPipelineDescriptor alloc] init];
            d.vertexFunction = vs; d.fragmentFunction = fs;
            d.colorAttachments[0].pixelFormat = ToMTL(fmt);
            id<MTLRenderPipelineState> pso =
                [dev_ newRenderPipelineStateWithDescriptor:d error:&e];
            if (!pso) {
                if (log_) log_("flipPSO: " + std::string(e ? [[e localizedDescription] UTF8String]
                                                            : "nil"));
                return false;
            }
            if (!flipSampler_) {
                MTLSamplerDescriptor* sd = [[MTLSamplerDescriptor alloc] init];
                sd.minFilter = MTLSamplerMinMagFilterNearest;
                sd.magFilter = MTLSamplerMinMagFilterNearest;
                sd.mipFilter = MTLSamplerMipFilterNotMipmapped;
                sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
                sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
                flipSampler_ = [dev_ newSamplerStateWithDescriptor:sd];
            }
            flipPipe_ = pso;
            flipFmt_ = (uint32_t)fmt;
            return true;
        } @catch (NSException* ex) {
            if (log_) log_(std::string("flipEXC: ") + (ex ? [[ex reason] UTF8String] : "?"));
            return false;
        }
    }
    void NoteCommit(id<MTLCommandBuffer>) {
        if (cbCommitted_) cbCommitted_->fetch_add(1, std::memory_order_relaxed);
    }

    // Scissor-region clear (GL parity): blit buffer→texture ghi ĐÚNG vùng —
    // loadAction clear không clip theo scissor (kiểm chứng test_clear_targets).
    // CB retain buffer đã encode → không cần giữ sau commit.
    bool fillRegionColor(IRenderTarget* target, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                         const ClearColor& c) override {
        return fillRegionColorAndDepth(target, x, y, w, h, c, 2.0 /*no depth clear*/);
    }
    bool fillRegionColorAndDepth(IRenderTarget* target, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                                 const ClearColor& c, double clearDepth) override {
        AppleTarget* at = dynamic_cast<AppleTarget*>(target);
        if (!at || !queue_ || !w || !h) return false;
        if (at->isMSAA()) return false; // blit buffer→texture không ghi được MSAA
        PixelFormat f = at->pixelFormat();
        bool bgra = (f == PixelFormat::BGRA8Unorm);
        if (!bgra && f != PixelFormat::RGBA8Unorm) return false; // sRGB/float/int → fallback
        id<MTLTexture> t = at->resolve();
        if (!t) return false;
        const bool wantDepth = (clearDepth <= 1.0) && (at->depth() != nil);
        // Kẹp vùng vào kích thước texture.
        uint32_t tw = (uint32_t)t.width, th = (uint32_t)t.height;
        if (x >= tw || y >= th) return false;
        if (x + w > tw) w = tw - x;
        if (y + h > th) h = th - y;
        auto to8 = [](double v) -> uint8_t {
            double s = v < 0 ? 0 : (v > 1 ? 1 : v);
            return (uint8_t)(s * 255.0 + 0.5);
        };
        uint8_t px[4] = {to8(c.r), to8(c.g), to8(c.b), to8(c.a)};
        if (bgra) std::swap(px[0], px[2]);
        // stride 256 (rule blit buffer copy); row thật w*4 byte đầu.
        size_t stride = (((size_t)w * 4) + 255) & ~(size_t)255;
        size_t need = stride * h;
        // Buffer clear TÁI SỬ DỤNG (grow-on-demand). GuiItemAtlas clear 1 slot /
        // item / frame (~300 lần) → newBufferWithBytes mỗi lần là ~300
        // MTLBuffer + commit mỗi frame: vừa nặng vừa dễ chạm giới hạn
        // submissions của iOS (mọi CB sau bị bỏ → mọi ô atlas đổ trống).
        if (!clearBuf_ || clearBufSize_ < need) {
            clearBuf_ = [dev_ newBufferWithLength:(NSUInteger)need
                                          options:MTLResourceStorageModeShared];
            if (!clearBuf_) return false;
            clearBufSize_ = need;
        }
        uint8_t* dst = (uint8_t*)[clearBuf_ contents];
        if (!dst) return false;
        for (uint32_t r = 0; r < h; ++r)
            for (uint32_t x2 = 0; x2 < w; ++x2)
                memcpy(dst + (size_t)r * stride + (size_t)x2 * 4, px, 4);
        @try {
            id<MTLCommandBuffer> cb = [queue_ commandBuffer];
            if (!cb) return false;
            id<MTLBlitCommandEncoder> blit = [cb blitCommandEncoder];
            if (!blit) return false;
            [blit copyFromBuffer:clearBuf_ sourceOffset:0 sourceBytesPerRow:stride
                 sourceBytesPerImage:stride * h
                       sourceSize:MTLSizeMake(w, h, 1)
                       toTexture:t destinationSlice:0 destinationLevel:0
               destinationOrigin:MTLOriginMake(x, y, 0)];
            [blit endEncoding];
            // Depth clear (nếu có) trong CÙNG command buffer.
            if (wantDepth) {
                MTLRenderPassDescriptor* rp = [MTLRenderPassDescriptor renderPassDescriptor];
                rp.colorAttachments[0].loadAction = MTLLoadActionDontCare;
                rp.colorAttachments[0].storeAction = MTLStoreActionDontCare;
                rp.depthAttachment.texture = at->depth();
                rp.depthAttachment.loadAction = MTLLoadActionClear;
                rp.depthAttachment.storeAction = MTLStoreActionStore;
                rp.depthAttachment.clearDepth = clearDepth;
                id<MTLRenderCommandEncoder> de = [cb renderCommandEncoderWithDescriptor:rp];
                if (de) [de endEncoding];
            }
            // Handler + đếm: command buffer này cũng phải được tính vào
            // in-flight tracking, nếu không CPU tưởng GPU rảnh và ghi đè
            // shared memory đang bay.
            if (cbCompleted_)
                [cb addCompletedHandler:^(id<MTLCommandBuffer>) {
                    cbCompleted_->fetch_add(1, std::memory_order_relaxed);
                }];
            NoteCommit(cb);
            [cb commit]; // không wait: thứ tự đảm bảo cùng queue
            return true;
        } @catch (NSException*) {
            return false;
        }
    }
private:
    id<MTLDevice> dev_;
    id<MTLCommandQueue> queue_;
    LogFn log_;
    std::atomic<uint64_t>* cbCommitted_ = nullptr;
    std::atomic<uint64_t>* cbCompleted_ = nullptr;
    id<MTLRenderPipelineState> flipPipe_ = nil; // present flip (1 draw thay vì ~1242 blit)
    id<MTLSamplerState> flipSampler_ = nil;
    uint32_t flipFmt_ = 0;
    int presentProbe_ = 0;
    id<MTLBuffer> clearBuf_ = nil;             // buffer cho fillRegionColor (tái sử dụng)
    size_t clearBufSize_ = 0;
    std::vector<DrawTrace> trace_;
    std::map<std::string, id<MTLRenderPipelineState>> pcache_;
    std::map<uint64_t, id<MTLDepthStencilState>> dcache_; // IR: 16 depth states cache
    std::mutex dmu_;
    std::shared_ptr<IRenderTarget> defaultTarget_;
    std::shared_ptr<AppleRenderEncoder::SharedDiag> diag_;
    std::mutex pmu_;
    // Depth textures đã qua first-clear (per-texture). Xem takeDepthInit.
    std::shared_ptr<DepthInitSet> depthInit_ = std::make_shared<DepthInitSet>();
};

std::shared_ptr<IDevice> CreateAppleDevice(LogFn log) {
    id<MTLDevice> d = MTLCreateSystemDefaultDevice();
    if (!d) {
        // Trên máy thật default device không bao giờ nil — nếu nil (thiết bị
        // jailbreak lạ / race khởi tạo), thử toàn bộ danh sách trước khi bỏ.
        // Log rõ để latestlog phân biệt với dylib build thiếu Apple backend.
        if (log) log("TGLMT: MTLCreateSystemDefaultDevice nil, thu MTLCopyAllDevices");
        NSArray<id<MTLDevice>>* all = MTLCopyAllDevices();
        if (log) log("TGLMT: MTLCopyAllDevices count=" + std::to_string(all.count));
        if (all.count > 0) {
            d = all[0];
            if (log) log(std::string("TGLMT: dung fallback device=") + ([d.name UTF8String] ? [d.name UTF8String] : "?"));
        }
    }
    if (!d) {
        if (log) log("TGLMT: khong co MTLDevice (Null fallback)");
        return nullptr; // headless/CI không GPU → caller fallback Null (trung thực)
    }
    return std::make_shared<AppleDevice>(d, log);
}

} // namespace tglmt::metal
#endif
