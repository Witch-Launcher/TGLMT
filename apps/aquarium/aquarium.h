#pragma once
// Aquarium — benchmark cá 3D (GFXBench-style): nước, cá boids, rong, bọt khí, cát.
// Render HOÀN TOÀN bằng OpenGL 4.6 (tglmt::gl). Không chạm Metal trực tiếp:
// shell nền tảng (macOS/iOS) chỉ tạo layer + gọi present. Vật lý CPU: boids,
// buoyancy/bobbing, va chạm thành bể, bọt khí nổi.
#include <cstdint>
#include <string>

namespace aquarium {

// Khởi tạo 1 lần: shaderDir chứa *.vert/*.frag; fishCount, bubbleCount cấu hình bench.
bool Init(const std::string& shaderDir, int fishCount = 48, int bubbleCount = 60);
// Render 1 frame: timeSec (giây), dt (giây, đã kẹp), viewW/H (pixel drawable).
// Trả false khi lỗi GL nghiêm trọng.
bool RenderFrame(double timeSec, double dt, int viewW, int viewH);
// FPS trung bình từ đầu (điểm benchmark) + số frame đã render.
double AverageFPS();
long RenderedFrames();
// Lỗi cuối cùng (Init/program) để shell ghi log/gửi chẩn đoán. Rỗng = không lỗi mới.
const char* LastError();
// Mask bật/tắt object (bit0 bg,1 sand,2 plants,3 fish,4 water,5 bubbles,6 hud).
// Mặc định 0xFF (tất cả). Dùng file aqua.cfg trên máy để bisection từ xa.
void SetObjectMask(unsigned mask);
// Self-test tối giản qua đúng đường GL (program đỏ, không uniform): true = PASS.
// Điền r/g/b đọc được ở giữa màn hình (kỳ vọng đỏ) để shell log.
bool RunSelfTest(int viewW, int viewH, int* r, int* g, int* b);
// Chuỗi "fish:59 sand:59 ..." đếm draw đã encode theo program (chẩn đoán).
const char* ProgStatsString();
// Program GL id theo index 0..6 (fish,water,sand,plant,bubble,bg,hud). 0 = chưa Init.
unsigned ProgramId(int index);
// Ghi MSL đã convert của program ra dir (đối chiếu mac/iOS). Trả false + LastError.
bool DumpProgramMSL(unsigned prog, const std::string& dir);

} // namespace aquarium
