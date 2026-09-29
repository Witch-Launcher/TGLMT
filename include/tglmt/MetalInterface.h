#pragma once
// IMetal abstraction — C++ thuần, 2 backend: Apple (ObjC++) và Null (CPU/CI).
// Tương đương 1-1 với metal-cpp (MTL::Device, MTL::Buffer, ...). Khi vendor
// third_party/metal-cpp, các struct này map trực tiếp sang MTL::* mà không đổi API.
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace tglmt::metal {

enum class PixelFormat : uint32_t {
    Invalid = 0, RGBA8Unorm = 70, RGBA8Unorm_sRGB = 71, RGBA8Uint = 73,
    BGRA8Unorm = 80, BGRA8Unorm_sRGB = 81,
    Depth32Float = 252, Depth24Stencil8 = 255, Stencil8 = 53,
    R8Unorm = 10, RG8Unorm = 30, RGBA16Float = 115, RGBA32Float = 125,
    R32Float = 55, R32Sint = 56, R32Uint = 57, // buffer textures (CloudFaces)
};
// GL enum thô (giá trị từ gl.xml) cho blend/sampler — bridge ánh xạ sang MTL*.
// Không include GL header ở đây để giữ IMetal độc lập platform.
struct AttachmentBlend {
    bool enabled = false;
    uint32_t srcRGB = 1;    // GL_ONE
    uint32_t dstRGB = 0;    // GL_ZERO
    uint32_t srcAlpha = 1;  // GL_ONE
    uint32_t dstAlpha = 0;  // GL_ZERO
    uint32_t rgbOp = 0x8006;   // GL_FUNC_ADD
    uint32_t alphaOp = 0x8006; // GL_FUNC_ADD
};
struct SamplerDesc {
    uint32_t minFilter = 0x2601; // GL_LINEAR
    uint32_t magFilter = 0x2601; // GL_LINEAR
    uint32_t sWrap = 0x2901;     // GL_REPEAT
    uint32_t tWrap = 0x2901;     // GL_REPEAT
    float maxAniso = 1.0f;
};
struct ScissorRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;
    uint32_t h = 0;
}; // hệ Metal (origin top-left)
// Attribute cho vertex descriptor tùy biến (M5b VAO→descriptor dùng chính struct này).
// size/type/normalized theo GL enum (size 1..4, type 0x1400..0x1406...), offset byte.
// divisor: 0 = per-vertex, >0 = per-instance (glVertexBindingDivisor/AttribDivisor).
// A11: mọi divisor đều hỗ trợ qua MTLVertexStepFunctionPerInstance + stepRate.
struct CustomAttrib {
    uint32_t loc = 0;
    uint32_t size = 4;
    uint32_t type = 0x1406; // GL_FLOAT
    bool normalized = false;
    uint32_t offset = 0;
    uint32_t bufferIndex = 0; // VAO binding index
    uint32_t stride = 0;      // stride của buffer này (lấy từ attrib đầu tiên dùng nó)
    uint32_t divisor = 0;     // 0 = PerVertex, >0 = PerInstance stepRate
};
// Tùy chọn pipeline cho M5b GL-driven render (depth/blend bake cùng descriptor).
struct PipelineOpts {
    bool depth = false;              // target có depth → depthAttachmentPixelFormat
    bool blend = false;              // bật blending attachment 0
    AttachmentBlend blend0;          // cấu hình blend khi blend=true
};

class ISamplerState {
public:
    virtual ~ISamplerState() = default;
};
class IDepthStencilState {
public:
    virtual ~IDepthStencilState() = default;
};
enum class PrimitiveType : uint8_t {
    Point=0, Line=1, LineStrip=2, Triangle=3, TriangleStrip=4,
    Fan=5,    // GL_TRIANGLE_FAN — Metal không có fan; trace giữ đúng, GPU expand ở EmitDraw (M5b)
    Patches=6 // GL_PATCHES — đường tessellation drawPatches (xem gl_tess_xfb)
};
enum class IndexType : uint8_t { UInt16=0, UInt32=1 };
enum class StorageMode : uint8_t { Shared=0, Managed=1, Private=2, Memoryless=3 };
enum class LoadAction : uint8_t { DontCare=0, Load=1, Clear=2 };
enum class StoreAction : uint8_t { DontCare=0, Store=1, MultisampleResolve=2 };

struct Viewport { double x,y,w,h,n,f; };
struct ClearColor { double r,g,b,a; };
// DrawTrace: first = vertexStart (non-indexed) hoặc 0 (indexed).
// A11/vanilla: chunk mesh dùng glDrawArrays(first!=0) nhiều → phải trace đúng.
struct DrawTrace { PrimitiveType prim; uint32_t count; uint32_t instanceCount; bool indexed; uint32_t first = 0;};
enum class LoadOp : uint8_t { Load = 0, Clear = 1, DontCare = 2 };

class IBuffer {
public:
    virtual ~IBuffer() = default;
    virtual void* contents() = 0;
    virtual const void* contents() const = 0;
    virtual size_t length() const = 0;
    virtual void didModifyRange(size_t off, size_t len) = 0;
};

class ITexture {
public:
    virtual ~ITexture() = default;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual PixelFormat pixelFormat() const = 0;
};

using LogFn = std::function<void(const std::string&)>;

class IEncoder {
public:
    virtual ~IEncoder() = default;
    virtual void setViewport(const Viewport& vp) = 0;
    virtual void drawPrimitives(PrimitiveType t, uint32_t start, uint32_t count, uint32_t instances=1) = 0;
    virtual void drawIndexed(PrimitiveType t, uint32_t count, IndexType it, IBuffer* ib, size_t off, uint32_t instances=1) = 0;
    virtual void setVertexBuffer(IBuffer* b, size_t off, uint32_t idx) = 0;
    virtual void endEncoding() = 0;
};

// --- M5: render pipeline API (offscreen render + readback cho pixel-compare) ---
// Thêm không phá vỡ: Null backend dùng default (trả nullptr), Apple backend cài thật.
class ILibrary {
public:
    virtual ~ILibrary() = default;
};

class IRenderPipeline {
public:
    virtual ~IRenderPipeline() = default;
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    // Đọc pixel RGBA8. Trả false khi backend không hỗ trợ.
    virtual bool readback(void* dst, size_t bytesPerRow) = 0;
    // Format color thật của target (cho pipeline khớp + present kiểm tra).
    virtual PixelFormat pixelFormat() const { return PixelFormat::RGBA8Unorm; }
};

class IRenderEncoder {
public:
    virtual ~IRenderEncoder() = default;
    virtual void setViewport(const Viewport& vp) = 0;
    virtual void setVertexBuffer(IBuffer* b, size_t off, uint32_t idx) = 0;
    virtual void drawPrimitives(PrimitiveType t, uint32_t start, uint32_t count, uint32_t instances = 1) = 0;
    virtual void drawIndexed(PrimitiveType t, uint32_t count, IndexType it, IBuffer* ib, size_t off, uint32_t instances = 1) = 0;
    // Kết thúc encode + commit + chờ GPU xong (kèm blit-synchronize để readback an toàn).
    virtual bool endAndCommit() = 0;
    // Kết thúc + commit KHÔNG đợi (cho nhiều draw/frame; thứ tự đảm bảo bởi cùng
    // queue; readback/present ở cuối frame vẫn đúng). Default = endAndCommit.
    virtual bool endAndCommitNoWait() { return endAndCommit(); }
    // Tessellation: factor buffer (setTessellationFactorBuffer) + drawPatches
    // (patchIndexBuffer nil, instanceCount 1). Default no-op (Null backend).
    virtual void setTessellationFactorBuffer(IBuffer* b, size_t off, size_t stride) {
        (void)b; (void)off; (void)stride;
    }
    virtual void drawPatches(uint32_t controlPoints, uint32_t patchStart, uint32_t patchCount) {}
    // Raster thay thế đúng hành vi (Metal có sẵn, GL chỉ cần bake khác cách):
    virtual void setTriangleFillModeLines(bool lines) { (void)lines; } // glPolygonMode LINE
    virtual void setScissorRect(const ScissorRect& r) { (void)r; }     // glScissor
    virtual void setDepthStencilState(IDepthStencilState* s) { (void)s; } // glDepthFunc/Mask
    virtual void setFragmentTexture(ITexture* t, uint32_t idx) { (void)t; (void)idx; }
    virtual void setFragmentSamplerState(ISamplerState* s, uint32_t idx) { (void)s; (void)idx; }
    virtual void setVertexTexture(ITexture* t, uint32_t idx) { (void)t; (void)idx; }
    virtual void setVertexSamplerState(ISamplerState* s, uint32_t idx) { (void)s; (void)idx; }
    virtual void setFragmentBuffer(IBuffer* b, size_t off, uint32_t idx) {
        (void)b; (void)off; (void)idx;
    }
    // Cull (glCullFace/glFrontFace): enabled=false → MTLCullModeNone.
    // cullModeGL: GL_BACK/GL_FRONT; frontFaceGL: GL_CCW/GL_CW.
    virtual void setCullMode(bool enabled, uint32_t cullModeGL, uint32_t frontFaceGL) {
        (void)enabled; (void)cullModeGL; (void)frontFaceGL;
    }
    // Blend color (glBlendColor) cho factors CONSTANT_*.
    virtual void setBlendColor(float r, float g, float b, float a) {
        (void)r; (void)g; (void)b; (void)a;
    }
    // Mesh draw: 1D grid `groups` threadgroup (mỗi group 1×1×1 thread cho gsMesh).
    // Chỉ gọi khi pipeline là mesh; API yếu (respondsToSelector) trên OS cũ.
    virtual void drawMesh(uint32_t groups) { (void)groups; }
};

// Compute (cho composite emulation như LogicOp-ROP, XFB-capture M5b).
class IComputePipeline {
public:
    virtual ~IComputePipeline() = default;
};
class IComputeEncoder {
public:
    virtual ~IComputeEncoder() = default;
    virtual void setPipeline(IComputePipeline* p) { (void)p; }
    virtual void setTexture(ITexture* t, uint32_t idx) { (void)t; (void)idx; }
    virtual void setBuffer(IBuffer* b, uint32_t idx) { (void)b; (void)idx; }
    virtual void dispatch2D(uint32_t w, uint32_t h) { (void)w; (void)h; }
    virtual bool endAndCommit() { return false; }
};

class IDevice {
public:
    virtual ~IDevice() = default;
    virtual std::string name() const = 0;
    virtual bool isNull() const = 0;
    virtual std::shared_ptr<IBuffer> newBuffer(size_t len, StorageMode m) = 0;
    virtual std::shared_ptr<IBuffer> newBufferWithBytes(const void* p, size_t len, StorageMode m) = 0;
    virtual std::shared_ptr<ITexture> newTexture(uint32_t w, uint32_t h, PixelFormat f) = 0;
    virtual std::shared_ptr<IEncoder> makeEncoder() = 0;
    virtual void commitAndWait() = 0;
    virtual const std::vector<DrawTrace>& drawTrace() const = 0;
    virtual void clearTrace() = 0;
    // M5 render API — default nullptr (Null backend không GPU).
    virtual std::shared_ptr<ILibrary> compileLibrary(const std::string& msl, std::string& errLog) {
        (void)msl; errLog = "render pipeline unsupported on this backend"; return nullptr;
    }
    // 6-arg: vs và fs từ 2 library riêng (chuẩn spirv-cross: mỗi stage 1 file MSL).
    virtual std::shared_ptr<IRenderPipeline> makeRenderPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt) {
        (void)vsLib; (void)vsFn; (void)fsLib; (void)fsFn; (void)colorFmt; return nullptr;
    }
    // 4-arg: cùng 1 library cho cả 2 stage (vd triangle.metal) — delegate, giữ tương thích.
    virtual std::shared_ptr<IRenderPipeline> makeRenderPipeline(ILibrary* lib, const char* vsFn,
            const char* fsFn, PixelFormat colorFmt) {
        return makeRenderPipeline(lib, vsFn, lib, fsFn, colorFmt);
    }
    virtual std::shared_ptr<IRenderTarget> makeRenderTarget(uint32_t w, uint32_t h, PixelFormat f) {
        (void)w; (void)h; (void)f; return nullptr;
    }
    // viewport của IRenderEncoder theo hệ Metal (origin top-left, depth 0..1).
    // Caller (sau này là StateTracker M5b) chịu trách nhiệm flip-y/convert từ GL.
    virtual std::shared_ptr<IRenderEncoder> makeRenderEncoder(IRenderTarget* target,
            IRenderPipeline* pipeline, const ClearColor& clear) {
        (void)target; (void)pipeline; (void)clear; return nullptr;
    }
    // Tessellation (MSL spec §5.1.1.1 post-tess vertex fn + §5.2.3.2 patch inputs,
    // MTLTriangleTessellationFactorsHalf; API có từ iOS 10/macOS 10.12):
    // pipeline triangle-domain, partition Integer, factor Half, step PerPatch.
    // factors 1.0 = passthrough đúng hành vi GL tess level 1; TCS tính factors
    // động cần M5b thực thi program (ghi rõ, không giả).
    virtual std::shared_ptr<IRenderPipeline> makeTessPipeline(ILibrary* vsLib, const char* postTessFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt, uint32_t controlPoints) {
        (void)vsLib; (void)postTessFn; (void)fsLib; (void)fsFn; (void)colorFmt;
        (void)controlPoints; return nullptr;
    }
    // Depth/stencil state (glDepthFunc/glDepthMask → MTLCompareFunction + writeMask).
    virtual std::shared_ptr<IDepthStencilState> makeDepthStencilState(uint32_t func, bool writeMask) {
        (void)func; (void)writeMask; return nullptr;
    }
    // Pipeline cho target có depth (depthAttachmentPixelFormat=Depth32Float khớp pass).
    virtual std::shared_ptr<IRenderPipeline> makeDepthPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt) {
        (void)vsLib; (void)vsFn; (void)fsLib; (void)fsFn; (void)colorFmt; return nullptr;
    }
    // Render target kèm depth (color Shared + depth Private, store DontCare).
    virtual std::shared_ptr<IRenderTarget> makeRenderTargetWithDepth(uint32_t w, uint32_t h,
            PixelFormat colorFmt) {
        (void)w; (void)h; (void)colorFmt; return nullptr;
    }
    // Pipeline có blending (glBlendFunc/Equation per-attachment → MTLBlendFactor/Operation).
    virtual std::shared_ptr<IRenderPipeline> makeBlendPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt,
            const AttachmentBlend* blends, uint32_t blendCount) {
        (void)vsLib; (void)vsFn; (void)fsLib; (void)fsFn; (void)colorFmt;
        (void)blends; (void)blendCount; return nullptr;
    }
    // Sampler (glSamplerParameter* + ARB_anisotropy → MTLSamplerDescriptor).
    virtual std::shared_ptr<ISamplerState> makeSampler(const SamplerDesc& d) {
        (void)d; return nullptr;
    }
    // Depth texture riêng (Private) cho mục đích khác readback trực tiếp.
    virtual std::shared_ptr<ITexture> newDepthTexture(uint32_t w, uint32_t h, PixelFormat f) {
        return newTexture(w, h, f);
    }
    // Texture kèm dữ liệu (replaceRegion sau tạo) — cho texturing E2E.
    virtual std::shared_ptr<ITexture> newTextureWithBytes(uint32_t w, uint32_t h, PixelFormat f,
            const void* data, size_t bytesPerRow) {
        (void)data; (void)bytesPerRow;
        return newTexture(w, h, f);
    }
    // MSAA target (multisample Private + resolve Shared, storeActionMultisampleResolve).
    virtual std::shared_ptr<IRenderTarget> makeMSAATarget(uint32_t w, uint32_t h, uint32_t samples) {
        (void)w; (void)h; (void)samples; return nullptr;
    }
    // Mesh pipeline thay thế GS (Metal 3+, MTLMeshRenderPipelineDescriptor).
    // Trả nullptr khi thiết bị/OS không hỗ trợ (test SKIP, không crash).
    virtual std::shared_ptr<IRenderPipeline> makeMeshPipeline(ILibrary* meshLib, const char* meshFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt) {
        (void)meshLib; (void)meshFn; (void)fsLib; (void)fsFn; (void)colorFmt; return nullptr;
    }
    // Khả năng THỰC THI mesh (khác với tạo pipeline thành công!): Intel tạo được
    // pipeline nhưng vẽ ra đen (đã quan sát). Gate đúng: Apple7+ (thực nghiệm:
    // Intel KBL Apple7=0/Metal3=1; Metal3 family KHÔNG đủ).
    virtual bool supportsMesh() { return false; }
    // Compute pipeline/encoder (composite emulation: LogicOp-ROP, XFB-capture M5b).
    virtual std::shared_ptr<IComputePipeline> makeComputePipeline(ILibrary* lib, const char* fn) {
        (void)lib; (void)fn; return nullptr;
    }
    virtual std::shared_ptr<IComputeEncoder> makeComputeEncoder() { return nullptr; }
    // Pipeline với vertex descriptor tùy biến (VAO M5b; test proof dùng trước).
    // opts=nullptr → pipeline thường (tương thích cũ).
    virtual std::shared_ptr<IRenderPipeline> makeCustomPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt,
            const CustomAttrib* attribs, uint32_t nAttribs, uint32_t stride,
            const PipelineOpts* opts = nullptr) {
        (void)vsLib; (void)vsFn; (void)fsLib; (void)fsFn; (void)colorFmt;
        (void)attribs; (void)nAttribs; (void)stride; (void)opts; return nullptr;
    }
    // Bọc texture sẵn có thành render target (FBO attach / ReadPixels GPU).
    // depth=nullptr khi không có depth.
    virtual std::shared_ptr<IRenderTarget> wrapAsTarget(ITexture* color, ITexture* depth) {
        (void)color; (void)depth; return nullptr;
    }
    // Encoder giữ nội dung cũ (LoadActionLoad) — cho draw thứ 2+ trong frame.
    // Default: encoder clear đen (fallback vô hại).
    virtual std::shared_ptr<IRenderEncoder> makeRenderEncoderLoad(IRenderTarget* target,
            IRenderPipeline* pipeline) {
        return makeRenderEncoder(target, pipeline, ClearColor{0, 0, 0, 1});
    }
    // Encoder với load action riêng color/depth (glClear mask). Default: clear cả hai.
    virtual std::shared_ptr<IRenderEncoder> makeRenderEncoderActions(IRenderTarget* target,
            IRenderPipeline* pipeline, const ClearColor& clear, double clearDepth,
            LoadOp colorLoad, LoadOp depthLoad) {
        (void)clearDepth; (void)depthLoad;
        if (colorLoad == LoadOp::Load) return makeRenderEncoderLoad(target, pipeline);
        return makeRenderEncoder(target, pipeline, clear);
    }
    // Default target cho FBO 0 (app/shell đặt; GL render vào đây).
    virtual void setDefaultRenderTarget(std::shared_ptr<IRenderTarget> t) { (void)t; }
    virtual std::shared_ptr<IRenderTarget> defaultRenderTarget() { return nullptr; }
    // Present target lên màn hình qua CAMetalLayer* (void* để giữ header thuần C++).
    // Trả false khi layer/target không hợp lệ.
    virtual bool presentTarget(IRenderTarget* target, void* metalLayer) {
        (void)target; (void)metalLayer; return false;
    }
    // Pipeline cho MSAA: rasterSampleCount phải khớp attachment (validation Metal),
    // ngược lại resolve cho ra pixel tối/sai (đã quan sát thực tế).
    virtual std::shared_ptr<IRenderPipeline> makeMSAAPipeline(ILibrary* vsLib, const char* vsFn,
            ILibrary* fsLib, const char* fsFn, PixelFormat colorFmt, uint32_t samples) {
        (void)vsLib; (void)vsFn; (void)fsLib; (void)fsFn; (void)colorFmt;
        (void)samples; return nullptr;
    }
    // Blit copy rect (glBlitFramebuffer): copy vùng (sx,sy,w,h) sang (dx,dy).
    // A11 TBDR: dùng blitCommandEncoder copyFromTexture (không scale).
    // Scale (srcSize != dstSize) trả false để caller fallback CPU.
    virtual bool blitCopy(ITexture* src, ITexture* dst,
            uint32_t sx, uint32_t sy, uint32_t w, uint32_t h,
            uint32_t dx, uint32_t dy) {
        (void)src; (void)dst; (void)sx; (void)sy; (void)w; (void)h; (void)dx; (void)dy;
        return false;
    }
    // Blit với default framebuffer tham gia (composite cuối menu ra màn hình):
    // FBO→màn hình và màn hình→FBO. Cùng size + COLOR → GPU copy thật;
    // còn lại false để caller giữ hành vi cũ + log rõ.
    virtual bool blitToTarget(ITexture* src, IRenderTarget* dst,
            uint32_t sx, uint32_t sy, uint32_t w, uint32_t h,
            uint32_t dx, uint32_t dy) {
        (void)src; (void)dst; (void)sx; (void)sy; (void)w; (void)h; (void)dx; (void)dy;
        return false;
    }
    virtual bool blitFromTarget(IRenderTarget* src, ITexture* dst,
            uint32_t sx, uint32_t sy, uint32_t w, uint32_t h,
            uint32_t dx, uint32_t dy) {
        (void)src; (void)dst; (void)sx; (void)sy; (void)w; (void)h; (void)dx; (void)dy;
        return false;
    }
    // Sinh mipmap GPU (glGenerateMipmap → generateMipmapsForTexture).
    // Texture phải được tạo với mipmapped=YES + đủ levels, không thì trả false.
    virtual bool generateMipmaps(ITexture* tex) {
        (void)tex; return false;
    }
    // Cập nhật vùng texture GPU từ CPU (glTexSubImage* → replaceRegion).
    // Trả false khi backend không hỗ trợ / vùng sai.
    virtual bool updateTexture(ITexture* tex, uint32_t x, uint32_t y,
            uint32_t w, uint32_t h, const void* data, size_t bytesPerRow) {
        (void)tex; (void)x; (void)y; (void)w; (void)h; (void)data; (void)bytesPerRow;
        return false;
    }
};

// Factory: backend="null" luôn có; backend="apple" chỉ khả dụng trên macOS/iOS (USE_APPLE_METAL=1).
std::shared_ptr<IDevice> CreateDevice(const std::string& backend, LogFn log = nullptr);

} // namespace tglmt::metal
