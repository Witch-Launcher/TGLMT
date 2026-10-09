// test_runtime_options.cpp — cấu hình runtime đọc từ env (launcher tab Đồ hoạ →
// mục TGLMT). Trước đây frames-in-flight/pool là constexpr → phải build lại dylib
// để đổi; giờ launcher setenv trước khi Context sinh ra.
// Check tường minh thay vì assert (Release defined NDEBUG → assert vô hiệu).
#include "tglmt/Context.h"
#include <cstdio>
#include <cstdlib>

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_runtime_options FAIL: %s\n", msg);
        ++gFails;
    }
}

static const uint32_t kDefFIF = tglmt::Context::kMaxFramesInFlight;
static const size_t kDefPool = 3;

int main() {
    // --- 1. Không set env → mặc định compile-time ---
    unsetenv("TGLMT_FRAMES_IN_FLIGHT");
    unsetenv("TGLMT_BUF_POOL");
    unsetenv("TGLMT_DIAG");
    {
        tglmt::Context ctx("null");
        Check(ctx.opt.framesInFlight == kDefFIF, "default framesInFlight");
        Check(ctx.opt.bufPoolSlots == kDefPool, "default bufPoolSlots");
#if !defined(TGLMT_FORCE_DIAG)
        Check(ctx.opt.diag == false, "default diag off");
        Check(ctx.DiagOn() == false, "DiagOn off khi không env");
#endif
    }
    // --- 2. set hợp lệ → áp dụng, DiagOn phản ánh opt ---
    setenv("TGLMT_FRAMES_IN_FLIGHT", "5", 1);
    setenv("TGLMT_BUF_POOL", "6", 1);
    setenv("TGLMT_DIAG", "1", 1);
    {
        tglmt::Context ctx("null");
        Check(ctx.opt.framesInFlight == 5, "env framesInFlight=5");
        Check(ctx.opt.bufPoolSlots == 6, "env bufPoolSlots=6");
#if !defined(TGLMT_FORCE_DIAG)
        Check(ctx.opt.diag == true, "env TGLMT_DIAG=1");
        Check(ctx.DiagOn() == true, "DiagOn on khi env=1");
#endif
    }
    // --- 3. ngoài [1..8] → bỏ, giữ mặc định (chặn throttle treo / pool vô hạn) ---
    setenv("TGLMT_FRAMES_IN_FLIGHT", "0", 1);
    setenv("TGLMT_BUF_POOL", "99", 1);
    setenv("TGLMT_DIAG", "0", 1);
    {
        tglmt::Context ctx("null");
        Check(ctx.opt.framesInFlight == kDefFIF, "framesInFlight=0 bị bỏ");
        Check(ctx.opt.bufPoolSlots == kDefPool, "bufPool=99 bị bỏ");
#if !defined(TGLMT_FORCE_DIAG)
        Check(ctx.opt.diag == false, "TGLMT_DIAG=0 → off");
        Check(ctx.DiagOn() == false, "DiagOn off khi env=0");
#endif
    }
    // --- 4. chuỗi không phải số → giữ mặc định (atoi trả 0 → nằm ngoài [1..8]) ---
    setenv("TGLMT_FRAMES_IN_FLIGHT", "abc", 1);
    setenv("TGLMT_BUF_POOL", "abc", 1);
    {
        tglmt::Context ctx("null");
        Check(ctx.opt.framesInFlight == kDefFIF, "framesInFlight=abc → default");
        Check(ctx.opt.bufPoolSlots == kDefPool, "bufPool=abc → default");
    }
    unsetenv("TGLMT_FRAMES_IN_FLIGHT");
    unsetenv("TGLMT_BUF_POOL");
    unsetenv("TGLMT_DIAG");
    if (gFails) return 1;
    printf("test_runtime_options PASS (env → Context::opt)\n");
    return 0;
}
