// tests/shaders/test_glsl_coverage.cpp — CỔNG độ phủ GLSL của TGLMT.
//
// Mục tiêu: **FULL GLSL** — mọi tính năng GLSL (built-in variable theo stage,
// built-in function §8, và ngôn ngữ) đều phải:
//   (1) converter không báo lỗi, VÀ
//   (2) MSL sinh ra BIÊN DỊCH ĐƯỢC bằng compiler Metal thật
//       (`xcrun -sdk macosx metal -c`).
//
// (2) là điều kiện bắt buộc: converter là bộ viết lại văn bản, nên một
// built-in GLSL chưa được ánh xạ vẫn "convert OK" nhưng MSL đó **không biên
// dịch được** ⇒ hỏng âm thầm.
//
// Bảng case dùng chung với tools/glsl_probe.cpp:
// tests/shaders/glsl_coverage_cases.h (built-in function sinh tự động bằng
// tools/gen_glsl_builtin_funcs.py).
#include "glsl_coverage_cases.h"
#include "test_util.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt;

int main() {
    std::vector<GLSLProbeCase> all;
    all.reserve(sizeof(kCases) / sizeof(kCases[0]) +
                sizeof(kBuiltins) / sizeof(kBuiltins[0]) +
                sizeof(kFnCases) / sizeof(kFnCases[0]));
    for (const GLSLProbeCase& c : kCases) all.push_back(c);
    for (const GLSLProbeCase& c : kBuiltins) all.push_back(c);
    for (const GLSLProbeCase& c : kFnCases) all.push_back(c);

    std::vector<std::string> msl, log;
    std::vector<bool> convOk, metalOk;
    int bad = GLSLProbeRunAll(all, msl, convOk, metalOk, log);

    int ok = 0, na = 0;
    for (size_t i = 0; i < all.size(); ++i) {
        if (convOk[i] && metalOk[i]) { ++ok; continue; }
        if (log[i].rfind("N/A - ", 0) == 0) { ++na; continue; }
        std::string why = log[i].empty() ? std::string("?") : log[i];
        printf("  FAIL [%s] %-38s %s\n", all[i].group, all[i].name, why.c_str());
    }
    printf("test_glsl_coverage: %d/%d GLSL feature hoat dong (%d N/A - xem test_geometry_emulation)\n",
           ok, (int)all.size() - na, na);
    TGLMT_CHECK(bad == 0, "FULL GLSL: moi tinh nang GLSL phai convert + Metal-compile duoc");
    return TGLMT_RESULT;
}
