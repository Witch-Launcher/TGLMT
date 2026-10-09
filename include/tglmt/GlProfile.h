#pragma once
// GlProfile — đo thời gian và số lần gọi tại MỖI điểm vào extern "C" của GL.
// Câu hỏi đang mở từ log p2-phase-time: frame ~37ms nhưng draw+clear+present+
// stall chỉ chiếm 13–34% → thời gian còn lại nằm ở đâu? Tool này trả lời theo
// từng hàm gl* (LWJGL dlsym thẳng vào gl_c_exports.cpp nên mọi call đều qua đây).
// Chi phí: 2 steady_clock::now()/call (~50ns) — không đáng kể so với 1 frame.
#include <chrono>
#include <cstdint>

namespace tglmt {
// Phải >= số hàm sinh được (hiện 701). tools/gen_c_exports.py báo nếu vượt.
constexpr int kGlProfileMax = 768;

struct GlCallStat {
    uint64_t n = 0;  // số lần gọi
    uint64_t ns = 0; // tổng thời gian CPU (nanoseconds)
};

// Định nghĩa trong src/gl/gl_c_exports.cpp (file tự sinh biết danh sách tên).
extern GlCallStat gGlProfile[kGlProfileMax];
extern const char* const gGlProfileName[kGlProfileMax];
extern int gGlProfileN;

// RAII: khai báo biến ở đầu wrapper, huỷ (kể cả khi return sớm) tự cộng dồn.
struct GlProfileScope {
    int id;
    std::chrono::steady_clock::time_point t0;
    explicit GlProfileScope(int i) : id(i), t0(std::chrono::steady_clock::now()) {}
    ~GlProfileScope() {
        long long ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::steady_clock::now() - t0)
                           .count();
        if (id >= 0 && id < kGlProfileMax) {
            gGlProfile[id].n++;
            gGlProfile[id].ns += (uint64_t)ns;
        }
    }
};
} // namespace tglmt
