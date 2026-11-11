// pch_stub.cpp — nguồn rỗng cho target PCH dùng chung của các test.
//
// Mọi test đều include "tglmt/gl46.h" + "tglmt/Context.h" (Context.h kéo theo
// MetalInterface.h + GLSLConverter.h) → ~4k dòng header phải parse lại cho
// từng test (~1.2s/test × 167 test ≈ 200s CPU). PCH parse 1 lần rồi REUSE_FROM
// cho mọi test → phần compile chỉ còn code của từng test.
//
// File này KHÔNG chứa logic: chỉ để CMake có một nguồn hợp lệ cho target
// object library sinh PCH.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

namespace {
// Giữ symbol để file không bị loại khỏi đích (một số toolchain cảnh báo
// "empty translation unit" nếu TU không có gì).
const int kTglmtPchAnchor = 0;
} // namespace

extern "C" int tglmt_pch_stub_anchor(void) { return kTglmtPchAnchor; }
