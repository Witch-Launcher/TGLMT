#pragma once
// Lõi error tracker — tương đương glGetError (Metal không có, phải emulate trên CPU).
// Spec: glspec46.core.pdf §2.3 (Errors) + KHR_no_error.txt (UB khi bật, bỏ check).
#include "tglmt/gl46_types.h"
#include <vector>
#include <mutex>

namespace tglmt {

class ErrorTracker {
public:
    void SetNoError(bool on) { noError_ = on; if (on) queue_.clear(); }
    bool NoError() const { return noError_; }
    void Record(GLenum err);          // GL_INVALID_ENUM/VALUE/OPERATION/OUT_OF_MEMORY/...
    GLenum GetError();                // pop như glGetError
    bool HasPending() const;
    void Clear();
private:
    bool noError_ = false;
    std::vector<GLenum> queue_;
    mutable std::mutex mu_;
};

} // namespace tglmt
