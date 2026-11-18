# BASELINE.md — TGLMT (Phase 0 reconnaissance)

Sinh từ session 2026-11-17. Mọi con số dưới đây do script/cmake/ctest sinh ra, KHÔNG viết tay.

## 1. Môi trường và toolchain

| Mục | Giá trị |
|-----|---------|
| Hệ điều hành | macOS 15.7.7 (Darwin 24.6.0) |
| Kiến trúc máy xây dựng | x86_64 |
| Xcode | 26.3 (build 17C529) |
| CMake | /usr/local/bin/cmake |
| Clang | /usr/bin/clang |
| Metal toolchain | xcrun metal có sẵn (`xcrun -sdk macosx metal -c`) |
| Thiết bị Metal thực | KHÔNG có trên máy xây dựng (chỉ macOS host GPU, không phải iOS/A11) |
| Build directory | `build-work` (tồn tại trước session, được tái sử dụng) |
| CMAKE_BUILD_TYPE | Debug |
| TGLMT_APPLE_METAL | ON (backend Metal thật, compile MSL qua `xcrun metal`) |
| Backend chạy test | Null (Metal compile gate tách riêng qua `xcrun metal -c` trong test shader) |

## 2. Branch / HEAD / trạng thái thô

| Mục | Giá trị |
|-----|---------|
| Nhánh | `main` (upstream `origin/main`, đồng bộ) |
| HEAD | `bc528d5` Fix crash LWJGL/Minecraft: Context tạo trực tiếp không dựng version-gate mask |
| Lịch sử 5 commit gần nhất | `bc528d5`, `1ed09ea`, `b664b04`, `a0347e0`, `10ff95f` |
| Dirty (working tree) | `AGENTS.md`, `PROGRESS.md`, `TODO.md` (đã sửa trong session trước); `x.h` (untracked, không rõ nguồn) |
| Submodules | Không có |

## 3. Build baseline (sạch, Debug, Metal ON)

Lệnh:
```
cmake -S . -B build-work -DCMAKE_BUILD_TYPE=Debug -DTGLMT_APPLE_METAL=ON
cmake --build build-work -j8
```

Kết quả: **build OK**, 0 lỗi, 0 cảnh báo chặn. Các target chính:
- `libtglmt.dylib` (shared), `libtglmt.a` (static), `tglmt_objs` (object)
- 215 test executables + `truth_probe` + 6 benchmark executables

Kích thước artifact (Debug): chưa đo chính xác (bỏ qua, không phải gate).

## 4. CTest baseline

Lệnh: `cd build-work && ctest -j8`

Kết quả:
```
100% tests passed out of 215
Total Test time (real) = 58.36 sec
```
- Pass: 215, Fail: 0, Skip: 0
- Test chậm nhất: `test_glsl_coverage` (58.34 s — chạy `xcrun metal -c` cho ~324 case GLSL)
- Mọi test chạy **backend Null** cho GL state; shader compile/link qua **Metal thật** (`xcrun metal -c`) trong các test shader/converter.

Nhãn môi trường (theo AGENTS.md §7): toàn bộ test trên máy này = `OTHER_GPU` / `REFERENCE_ONLY` cho phần Metal (không phải A11 device). Chưa có nhãn `A11_DEVICE` nào.

## 5. Test đã đăng ký (theo thư mục)

| Thư mục | Số file test |
|---------|--------------|
| tests/unit | 113 |
| tests/integration | 77 |
| tests/regression | 14 |
| tests/shaders | 12 |
| tests/api | 1 |
| tests/conformance | 1 |
| Tổng | 218 file (215 được đăng ký ctest, 3 không phải test target) |

## 6. Trạng thái kiến trúc hiện tại (tóm tắt từ source)

- **GLSL compiler:** `src/gl/glsl_to_msl.cpp` (~13 057 LOC), **token/string rewriter**, KHÔNG có AST, KHÔNG có type system, KHÔNG có overload resolution đầy đủ, KHÔNG có dataflow analysis. Quyết định ngữ nghĩa bằng substring match trên văn bản nguồn (docs/GLSL_460_COMPILER.md §1). → Đây là rủi ro lớn nhất, nguyên nhân gốc các lỗi shaderpack (pow2, iris_Entity, dayBlend).
- **Preprocessor:** line-oriented, `#if` chỉ hỗ trợ `defined`/`!`/`&&`/`||`/nguyên số nguyên (không có toán tử quan hệ/arithmetic), `#include` hard fail.
- **Layout:** luôn MSL natural alignment; `std140`/`std430`/`packed`/`shared`/`row_major` bị bỏ qua (docs/GLSL_460_COMPILER.md §3.2).
- **double/dvec:** hạ xuống float (documented limitation, không báo sai độ chính xác tại compile).
- **Geometry shader:** emulation bằng MSL compute kernel, fail rõ tại link cho phần chưa hỗ trợ (A6 tuân thủ).
- **Version/profile/gate:** bảng sinh tự động từ `docs/khronos/gl.xml` (D-001..D-007), gate ở lớp C ABI, 1048 entry, mask 38 bit.
- **API coverage (tự đánh giá repo):** Core 657/657, Compatibility 1048/1048 prototype+gate; hành vi runtime từng lệnh chưa đầy đủ (nhiều stub `UNIMPLEMENTED` + giá trị an toàn).
- **C ABI:** `include/tglmt/gl_c_api.h` (1049 prototype `extern "C"`), `src/gl/gl_c_exports.cpp` wrapper forward.

## 7. Lỗi shaderpack đã quan sát (chưa sửa source trong phiên này)

| ID | Shaderpack | Biểu hiện | File nghi ngờ | Trạng thái |
|----|-----------|-----------|---------------|------------|
| ISS-001 | Complementary Reimagined R5.9.3 deferred1 | Metal: hàm trả `int` nhưng `return pow2(x) * float3(x)` trả `float3` | `src/gl/glsl_to_msl.cpp` (helper arity/overload) | Chưa tái hiện test |
| ISS-002 | BSL 10.1.8 TEXTURED | `no member named 'iris_Entity' in 'TGLMT_VIn'` | `src/gl/glsl_to_msl.cpp` (attribute activity), `src/gl/gl_program.cpp` (link/input) | Chưa tái hiện test |
| ISS-003 | E-LITE 5.1.3 prepare.vsh | Converter báo `helper dayBlend sai arity (co 3)` | `src/gl/glsl_to_msl.cpp` (helper-call graph) | Chưa tái hiện test |
| ISS-004 | MakeUp UltraFast 9.6 | Iris warn không resolve `fixedTaaCorrection`; sau đó `iris_Entity` ở pipeline khác | Iris expression resolver (ngoài TGLMT) + MSL iris_Entity | Chưa phân loại |

## 8. NOT_VERIFIED (đã ghi từ PROGRESS.md)

- Chưa chạy trên Apple A11 thật (iPhone 8/X, iOS 16).
- Minecraft corpus MSL byte-identical bằng dump diff, không phải pixel thật trên thiết bị.
- gl_DrawID/gl_NumSamples: converter path compile OK; giá trị chưa đúng khi chạy MultiDraw/fbo.
- TGLMT_APPLE_METAL=ON compile MSL bằng `xcrun metal` trên macOS host (không phải Metal device iOS).

## 9. Build warnings / sanitizer

Chưa bật UBSan/ASan/TSan. CMake không có cờ sanitizer. Chưa có cơ chế bắt warning compiler làm lỗi build (`-Werror` không set).

---
*Tiếp theo: docs/audit/REPOSITORY_MAP.md, OPEN_ISSUES.md, FEATURE_INVENTORY.md*
