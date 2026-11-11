// tests/test_util.h — helper kiểm thử dùng chung cho TGLMT.
//
// VÌ SAO CẦN FILE NÀY
// ------------------
// 13 file test cũ dùng `assert()` làm CƠ CHẾ DUY NHẤT để báo fail.
// `assert()` bị `#define NDEBUG` xoá trong build Release (-DNDEBUG), nên
// biểu thức kiểm tra **không hề được đánh giá** ⇒ test PASS trong khi
// implementation sai. Đây là loại "false green" nguy hiểm nhất: CI báo xanh
// nhưng không kiểm chứng gì.
//
// `TGLMT_CHECK(cond, msg)`:
//   • luôn đánh giá `cond` (không phụ thuộc NDEBUG),
//   • in dòng lỗi kèm file:line,
//   • đếm số lỗi,
//   • `return 1` từ main khi có lỗi (exit code khác 0 ⇒ CTest FAIL).
//
// Dùng:
//   #include "test_util.h"
//   int main() { TGLMT_CHECK(x == 1, "x phải bằng 1"); return TGLMT_RESULT; }
#pragma once

#include <cstdio>

namespace tglmt_test {

inline int& Failures() {
    static int n = 0;
    return n;
}

inline void Check(bool ok, const char* msg, const char* file, int line) {
    if (ok) return;
    ++Failures();
    fprintf(stderr, "%s:%d: FAIL: %s\n", file, line, msg);
}

} // namespace tglmt_test

#define TGLMT_CHECK(cond, msg)                                                    \
    do {                                                                          \
        if (!(cond)) {                                                            \
            tglmt_test::Failures()++;                                             \
            fprintf(stderr, "%s:%d: FAIL: %s\n", __FILE__, __LINE__, (msg));      \
        }                                                                         \
    } while (0)

// Giá trị trả về từ main().
#define TGLMT_RESULT (tglmt_test::Failures() ? 1 : 0)
