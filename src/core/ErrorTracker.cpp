#include "tglmt/ErrorTracker.h"
namespace tglmt {
void ErrorTracker::Record(GLenum err) {
    if (noError_) return; // KHR_no_error: UB, bỏ queue để tối ưu
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
