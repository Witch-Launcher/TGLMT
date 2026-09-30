#include "tglmt/ErrorTracker.h"
#include <cstdio>
#if defined(__APPLE__)
#include <execinfo.h>
#include <dlfcn.h>
#endif
namespace tglmt {
void ErrorTracker::Record(GLenum err) {
    if (noError_) return; // KHR_no_error: UB, bỏ queue để tối ưu
    {
        // Bẫy lỗi GL oan trên máy (vanilla quy lỗi pending cho call gần nhất,
        // vd copyTextureToBuffer crash 1282 trong khi call đó vô tội):
        // log 10 lỗi đầu KÈM TÊN HÀM gl gọi (dladdr qua dynamic symbol table —
        // dylib strip nhưng symbol export gl* vẫn còn, backtrace_symbols thì câm).
        static int nErr = 0;
        if (++nErr <= 10) {
#if defined(__APPLE__)
            void* bt[14];
            int nb = backtrace(bt, 14);
            // Bỏ 2 frame đầu (Record + hàm gl gọi trực tiếp sẽ hiện tên ở [1..]).
            // In tên symbol cho từng frame để thấy gl... nào ghi lỗi.
            // (dladdr có thể câm trên máy arm64e do PAC — fallback in raw PC.)
            char line[768];
            int w = snprintf(line, sizeof(line), "[TGLMT] glerr#%d code=0x%x", nErr, err);
            for (int i = 1; i < nb && w < (int)sizeof(line) - 40; ++i) {
                Dl_info info;
                if (dladdr(bt[i], &info) && info.dli_sname) {
                    w += snprintf(line + w, sizeof(line) - w, " <- %s", info.dli_sname);
                    if (i >= 4) break; // 4 frames gần nhất là đủ (gl* + caller GL)
                } else {
                    w += snprintf(line + w, sizeof(line) - w, " <- %p", bt[i]);
                    if (i >= 4) break;
                }
            }
            fprintf(stderr, "%s\n", line);
            fflush(stderr);
#else
            fprintf(stderr, "[TGLMT] glerr#%d code=0x%x\n", nErr, err);
            fflush(stderr);
#endif
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
