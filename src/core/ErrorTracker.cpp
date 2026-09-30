#include "tglmt/ErrorTracker.h"
#include <cstdio>
namespace tglmt {
void ErrorTracker::Record(GLenum err) {
    if (noError_) return; // KHR_no_error: UB, bỏ queue để tối ưu
    {
        // Bẫy lỗi GL oan trên máy (vanilla quy lỗi pending cho call gần nhất,
        // vd copyTextureToBuffer crash 1282 trong khi call đó vô tội):
        // log 30 lỗi đầu để thấy nguồn thật.
        static int nErr = 0;
        if (++nErr <= 30) {
            fprintf(stderr, "[TGLMT] glerr#%d code=0x%x\n", nErr, err);
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
