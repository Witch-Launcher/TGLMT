#include "tglmt/ErrorTracker.h"
#include <cstdio>
#if defined(__APPLE__)
#include <execinfo.h>
#endif
namespace tglmt {
void ErrorTracker::Record(GLenum err) {
    if (noError_) return; // KHR_no_error: UB, bỏ queue để tối ưu
    {
        // Bẫy lỗi GL oan trên máy (vanilla quy lỗi pending cho call gần nhất,
        // vd copyTextureToBuffer crash 1282 trong khi call đó vô tội):
        // log 30 lỗi đầu KÈM caller (backtrace, symbol export của dylib).
        static int nErr = 0;
        if (++nErr <= 30) {
#if defined(__APPLE__)
            if (nErr <= 3) {
                // 3 lỗi đầu kèm caller để phân biệt nguồn (probe boot vs sau).
                void* bt[7];
                int nb = backtrace(bt, 7);
                char** sym = backtrace_symbols(bt, nb);
                fprintf(stderr, "[TGLMT] glerr#%d code=0x%x at %s\n", nErr, err,
                        (sym && nb > 3) ? sym[3] : "?");
                // (không free(sym): 3 lần duy nhất, giữ đơn giản)
            } else {
                fprintf(stderr, "[TGLMT] glerr#%d code=0x%x\n", nErr, err);
            }
#else
            fprintf(stderr, "[TGLMT] glerr#%d code=0x%x\n", nErr, err);
#endif
            fflush(stderr);
        }
    }
    std::lock_guard<std::mutex> l(mu_);
    if (queue_.size() < 32) queue_.push_back(err);
}
GLenum ErrorTracker::GetError() {
    std::lock_guard<std::mutex> l(mu_);
    if (queue_.empty()) return 0; // GL_NO_ERROR
    GLenum e = queue_.front();
    queue_.erase(queue_.begin());
    return e;
}
bool ErrorTracker::HasPending() const {
    std::lock_guard<std::mutex> l(mu_);
    return !queue_.empty();
}
void ErrorTracker::Clear() {
    std::lock_guard<std::mutex> l(mu_);
    queue_.clear();
}
} // namespace tglmt
