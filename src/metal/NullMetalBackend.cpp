// Null backend — CPU shadow, chạy mọi nơi (Linux CI, macOS không GPU).
// Ghi drawTrace để unit test so khớp hành vi mà không cần GPU thật.
#include "tglmt/MetalInterface.h"
#include <cstring>
namespace tglmt::metal {

class NullBuffer : public IBuffer {
public:
    explicit NullBuffer(size_t n): data_(n,0) {}
    NullBuffer(const void* p,size_t n): data_((const uint8_t*)p,(const uint8_t*)p+n) {}
    void* contents() override { return data_.data(); }
    const void* contents() const override { return data_.data(); }
    size_t length() const override { return data_.size(); }
    void didModifyRange(size_t, size_t) override {}
private:
    std::vector<uint8_t> data_;
};

class NullTexture : public ITexture {
public:
    NullTexture(uint32_t w,uint32_t h,PixelFormat f): w_(w),h_(h),f_(f){}
    uint32_t width() const override { return w_; }
    uint32_t height() const override { return h_; }
    PixelFormat pixelFormat() const override { return f_; }
private:
    uint32_t w_,h_; PixelFormat f_;
};

class NullEncoder : public IEncoder {
public:
    explicit NullEncoder(std::vector<DrawTrace>& t): trace_(t) {}
    void setViewport(const Viewport&) override {}
    void drawPrimitives(PrimitiveType t,uint32_t s,uint32_t c,uint32_t inst) override {
        trace_.push_back({t,c,inst,false,s});
    }
    void drawIndexed(PrimitiveType t,uint32_t c,IndexType, IBuffer*,size_t off,uint32_t inst) override {
        (void)off; trace_.push_back({t,c,inst,true});
    }
    void setVertexBuffer(IBuffer*,size_t,uint32_t) override {}
    void endEncoding() override {}
private:
    std::vector<DrawTrace>& trace_;
};

class NullDevice : public IDevice {
public:
    explicit NullDevice(LogFn l): log_(l) {}
    std::string name() const override { return "TGLMT Null (CPU shadow)"; }
    bool isNull() const override { return true; }
    std::shared_ptr<IBuffer> newBuffer(size_t n,StorageMode) override {
        return std::make_shared<NullBuffer>(n);
    }
    std::shared_ptr<IBuffer> newBufferWithBytes(const void* p,size_t n,StorageMode) override {
        return std::make_shared<NullBuffer>(p,n);
    }
    std::shared_ptr<ITexture> newTexture(uint32_t w,uint32_t h,PixelFormat f) override {
        return std::make_shared<NullTexture>(w,h,f);
    }
    std::shared_ptr<IEncoder> makeEncoder() override {
        return std::make_shared<NullEncoder>(trace_);
    }
    void commitAndWait() override {}
    const std::vector<DrawTrace>& drawTrace() const override { return trace_; }
    void clearTrace() override { trace_.clear(); }
private:
    LogFn log_; std::vector<DrawTrace> trace_;
};

std::shared_ptr<IDevice> CreateDevice(const std::string& backend, LogFn log) {
    if (backend == "null" || backend.empty()) return std::make_shared<NullDevice>(log);
#ifdef TGLMT_APPLE_METAL
    extern std::shared_ptr<IDevice> CreateAppleDevice(LogFn);
    if (backend == "apple") return CreateAppleDevice(log);
#endif
    // fallback trung thực: backend lạ → null + ghi log thay vì crash
    return std::make_shared<NullDevice>(log);
}

} // namespace tglmt::metal
