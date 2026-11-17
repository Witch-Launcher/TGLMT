// glsl_probe.cpp — đo gap GLSL của converter TGLMT (GLSL → MSL).
//
// CÔNG CỤ CHẨN ĐOÁN (không phải CTest gate — gate là
// tests/shaders/test_glsl_coverage.cpp). In ra danh sách tính năng GLSL
// đang HỎNG để biết chính xác còn thiếu gì.
//
// Bảng case dùng chung: tests/shaders/glsl_coverage_cases.h
//
// Build (KHÔNG tạo build dir mới — dùng build-work có sẵn):
//   /usr/bin/c++ -std=c++17 -O1 -I include -I build-work/generated \
//       -I tests/shaders tools/glsl_probe.cpp src/gl/glsl_to_msl.cpp -o /tmp/glsl_probe
//   /tmp/glsl_probe
//
// Exit 0 nếu mọi tính năng đều hỗ trợ (sau khi đóng hết gap).
#include "glsl_coverage_cases.h"

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
    std::string curGroup;
    for (size_t i = 0; i < all.size(); ++i) {
        if (convOk[i] && metalOk[i]) { ++ok; continue; }
        if (log[i].rfind("N/A - ", 0) == 0) { ++na; continue; }
        if (curGroup != all[i].group) {
            curGroup = all[i].group;
            printf("\n--- %s ---\n", curGroup.c_str());
        }
        std::string why = log[i].empty() ? std::string("?") : log[i];
        printf("  HONG  %-38s %s\n", all[i].name, why.c_str());
    }
    printf("\n==== GLSL: %d/%d hoat dong (convert + Metal compile), %d hong, %d N/A ====\n",
           ok, (int)all.size() - na, bad, na);
    return bad == 0 ? 0 : 1;
}
