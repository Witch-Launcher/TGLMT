#pragma once
// Renderer — mặt tiền gọn để gắn TGLMT làm render backend cho app khác.
// App chỉ cần: Renderer + tglmt/gl46.h (OpenGL 4.6) + 1 CAMetalLayer cho present.
// Mọi lệnh vẽ vẫn là OpenGL 4.6 thuần, dịch runtime sang Metal (Apple backend)
// hoặc chạy trace/shadow trên CPU (Null backend, cho CI không GPU).
//
// QUY ƯỚC QUAN TRỌNG: mọi lệnh gl:: chỉ có hiệu lực trên Context hiện tại.
// Renderer::BeginFrame tự MakeCurrent, nên gọi mọi gl:: SAU BeginFrame
// (hoặc sau MakeCurrent thủ công). Gọi gl:: trước Init/BeginFrame sẽ rơi vào
// Context fallback và object lạc sang chỗ khác.
#include "tglmt/MetalInterface.h"

#include <cstdint>
#include <memory>
#include <string>

namespace tglmt {

class Context;

struct RendererStats {
    uint64_t drawsAttempted = 0;
    uint64_t drawsEncoded = 0; // encode GPU thật (Apple backend)
    uint64_t noProgram = 0;
    uint64_t noTarget = 0;
    uint64_t noPipeline = 0;
    uint64_t miscFail = 0;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    // backend: "apple" (GPU thật) hoặc "null" (CPU trace/shadow, cho CI/test).
    // depth=true: target kèm depth buffer (cho 3D có depth test).
    // fmt: format color — PHẢI khớp CAMetalLayer.pixelFormat khi present,
    // ngược lại presentTarget từ chối trung thực (màn hình đen).
    // Trả false khi backend lạ hoặc (Apple mà) tạo target thất bại.
    bool Init(const std::string& backend, uint32_t w, uint32_t h, bool depth = true,
              metal::PixelFormat fmt = metal::PixelFormat::RGBA8Unorm);

    // Đổi kích thước: dựng lại target ở BeginFrame kế tiếp (đánh dấu clear lại).
    void Resize(uint32_t w, uint32_t h);

    // Bắt đầu frame: MakeCurrent + đảm bảo default target + bind FBO 0.
    // Trả false khi chưa Init hoặc (Apple mà) không có target.
    //
    // newFrame=false: chỉ gắn context (MakeCurrent được gọi giữa frame bởi
    // chunk-uploader thread). TUYỆT ĐỐI không xoay target/ring ở chế độ này —
    // nếu xoay theo mỗi lần MakeCurrent thì target mặc định đổi giữa chừng
    // frame và ring bị ghi đè khi GPU còn đọc (vỡ hình).
    bool BeginFrame(bool newFrame = true);

    // Kết thúc frame. layer=nullptr: headless (không present, vẫn true).
    // layer=CAMetalLayer*: blit target lên drawable. Trả false khi present lỗi.
    // Đơn luồng (như mọi GL context).
    bool EndFrame(void* metalLayer);

    // Đọc pixel RGBA8, gốc bottom-left (chuẩn GL). Trả false khi lỗi GL/tham số.
    bool ReadPixels(int x, int y, int w, int h, void* out);

    Context& context();
    // True khi device là GPU thật (encode Metal). False: Null backend,
    // fallback thiếu GPU, hoặc build không có Apple backend — mọi draw
    // trace-only, readback từ shadow. Kiểm tra cái này trước khi kỳ vọng pixel GPU.
    bool hasRealGPU() const;
    RendererStats stats() const;
    std::string backendName() const;
    uint32_t width() const;
    uint32_t height() const;
    metal::PixelFormat colorFormat() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tglmt
