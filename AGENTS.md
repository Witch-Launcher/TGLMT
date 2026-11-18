# AGENTS.md — TGLMT (OpenGL 1.0–4.6 → Metal)

Hướng dẫn bắt buộc cho mọi agent làm việc trên repo này.
Đọc file này + `PROGRESS.md` + `TODO.md` **trước khi** đọc source.

---

## 1. Dự án là gì

TGLMT = một implementation OpenGL → Metal tự viết, chạy trên iOS (target phần
cứng: **Apple A11**, iPhone 8/X, iOS tối đa 16). Mục tiêu hiện tại:

> **FULL DESKTOP OPENGL 1.0 → 4.6** — Core Profile **và** Compatibility Profile,
> gồm legacy fixed-function pipeline, chạy thật trên Metal.

Artifact: `libtglmt.dylib` (dùng hiện tại qua `dlopen`/`dlsym`) + `.xcframework`
(sẽ làm ở §15.2 của đặc tả). Không phân phối qua App Store.

## 2. Quy tắc làm việc

Viết test trước rồi mới bắt đầu làm.
Làm thật kĩ, không qua loa, không fix để qua test mà phải fix thật, hoạt động được thật.
Làm việc một cách chuyên nghiệp, có quy tắc, không đoán mò, cái nào không biết hãy tìm trên Internet và đọc docs thực tế.
Phải giữ nguyên hành vi OpenGL, GLSL,... Đã thiết kế sau khi biên dịch sang Metal iOS.
Sau khi làm xong toàn bộ những gì được giao hãy chạy test xem thời gian biên dịch và tối ưu chúng xuống mức thấp nhất có thể đạt được. Sau khi tối ưu vẫn phải giữ đùng hành vi của OpenGL, GLSL,... Đã thiết kế.
Làm phải trắc trắn nếu không trắc trắn hãy đi tìm kĩ điểm tạo nên cái không trắc trắn ấy đến khi trắc trắn, tránh sai xót

## 3. Quy tắc kiến trúc đã CHỐT (không tự ý đổi)

| # | Quyết định |
|---|---|
| A1 | Dịch thẳng OpenGL 1.0–4.6 → Metal trong **một tầng**. KHÔNG dùng Mesa / Zink / MoltenVK / ANGLE / state tracker / GLSL→NIR của bên thứ ba. |
| A2 | Không có lớp trung gian OpenGL-implementation khác. IR/ABI nội bộ của TGLMT được phép (chi tiết hiện thực nội bộ). |
| A3 | **GLU: CÓ** trong phạm vi. **GLUT: CÓ** (thư viện tiện ích). |
| A4 | Đóng gói CẢ `.dylib` LẪN `.xcframework`. |
| A5 | Phạm vi chỉ là **OpenGL Desktop**. OpenGL ES / ESSL **không** thuộc phạm vi, không phải phương án thay thế. |
| A6 | Geometry shader **BẮT BUỘC phải emulation**. Không được kết luận `HARDWARE_LIMITED` cho GS chỉ vì Metal không có GS stage. |
| A7 | Port Metal layer sang metal-cpp **nếu** hiệu năng tương đương hoặc tốt hơn — phải **đo được**. |
| A8 | Shader compile: runtime MSL vs precompile+cache — quyết định theo **số đo benchmark**, không theo giả định. |
| A9 | Reference cho differential testing: Minecraft, game OpenGL thật, và test do AI tự viết theo spec. **Không cần** Mesa. |
| A10 | Chỉ những gì thực sự là OpenGL mới vào phạm vi (kể cả GLU). |
| A11 | (User override 2026-11-18) GLSL→MSL dùng **glslang + SPIRV-Cross** (git submodule) thay cho frontend tự viết. A1/A2 vẫn cấm Mesa/Zink/MoltenVK/ANGLE; glslang/SPIRV-Cross là ngoại lệ duy nhất cho đường dịch GLSL. Chi tiết: `docs/DECISIONS.md` D-013. |

Chi tiết + bằng chứng: `docs/DECISIONS.md`.

## 4. Quy trình nhiều session

**Đầu session**
1. Đọc `AGENTS.md`, `PROGRESS.md`, `TODO.md`.
2. Đọc `docs/DECISIONS.md` (quyết định kiến trúc đã chốt).
3. Chỉ sau đó mới đọc source liên quan.

**Cuối session**
1. Cập nhật `PROGRESS.md`: đã làm gì (kèm `file:line`, test ID, output thật),
   test còn FAIL/SKIP/NOT_RUN, mục NOT_VERIFIED, blocker, bước tiếp theo.
2. Cập nhật `TODO.md`.
3. Cập nhật `docs/DECISIONS.md` nếu có quyết định kiến trúc mới.

**Task agents**
- Mỗi agent có **write-scope riêng** (thư mục/module). Không cho hai agent sửa
  cùng một file.
- **Chỉ agent chính** được ghi vào inventory (`data/version_inventory.json`) và
  `PROGRESS.md`.
- Agent phải trả về: bằng chứng `file:line`, test ID, output thật. Báo cáo chung
  chung bị từ chối.

## 5. Chuẩn C++ / build

- C++17 (`set(CMAKE_CXX_STANDARD 17)`). Không dùng C++20.
- Metal layer: Objective-C++ (`.mm`), ARC (`-fobjc-arc`), hoặc metal-cpp (nếu A7
  được chốt theo đo đo).
- Public API là **C ABI** (`extern "C"`): không exception / RTTI / STL đi qua
  ranh giới thư viện.
- Current context lưu theo **thread** (`thread_local`).
- Deployment target tối thiểu: **iOS 16.0** (A11 không chạy được iOS 17+).
- Build (clone mới phải init submodule glslang + SPIRV-Cross):
  ```bash
  git submodule update --init --depth 1
  cmake -S . -B build-work -DCMAKE_BUILD_TYPE=Debug
  cmake --build build-work -j8
  cd build-work && ctest -j8
  ```
- `TGLMT_APPLE_METAL=ON` mới bật backend Metal thật (mặc định OFF ⇒ Null
  backend, đủ cho test state machine).

## 6. File sinh tự động — KHÔNG SỬA TAY

| File sinh | Bằng |
|---|---|
| `include/tglmt/gl46_types.h`, `include/tglmt/gl46.h`, `src/gl/gl_dispatch_gen.cpp` | `tools/gen_gl_headers.py` |
| `include/tglmt/gl_legacy_types.h`, `include/tglmt/gl_legacy.h` | `tools/gen_gl_headers.py` |
| `include/tglmt/gl_c_api.h` (public C ABI), `src/gl/gl_c_exports.cpp` | `tools/gen_c_exports.py` |
| `src/gl/gl_stubs_gen.cpp` | `tools/gen_stubs.py` |
| `src/gl/gl_version_gate_gen.cpp` | `tools/gen_version_gate.py` |
| `data/version_inventory.json`, `docs/OPENGL_VERSION_MATRIX.md`, `docs/GLSL_VERSION_MATRIX.md` | `tools/gen_version_inventory.py` |

Thứ tự regenerate: `gen_gl_headers.py` → `gen_version_inventory.py` →
`gen_version_gate.py` → `gen_c_exports.py` → `gen_stubs.py` → build.

**Nguồn chân lý duy nhất cho API/enum:** `docs/khronos/gl.xml` (Khronos
OpenGL-Registry, vendored). Không hardcode prototype/enum trong tay.

## 7. Quy tắc báo cáo (bắt buộc)

- Mọi con số trong báo cáo phải do **script sinh ra** từ inventory/kết quả
  test, không viết tay.
- Trạng thái tính năng dùng đúng bộ: `FULLY_IMPLEMENTED`,
  `PARTIALLY_IMPLEMENTED`, `INCORRECT`, `STUB`, `MISSING`, `EMULATED`,
  `HARDWARE_LIMITED`, `BLOCKED`, `NOT_VERIFIED`, `NOT APPLICABLE`.
- `FULLY_IMPLEMENTED` cần đủ **4** mục:
  1. code path thật (`file:line`),
  2. test ID đã PASS,
  3. đối chiếu với expected theo spec hoặc reference,
  4. nhãn môi trường: `A11_DEVICE` | `SIMULATOR` | `OTHER_GPU` |
     `REFERENCE_ONLY` | `NONE`.
- Không dùng chữ "conformant". Chỉ báo "đã chạy N/M test của suite X".
- Không tính SKIP / NOT RUN là PASS. Không bỏ test thất bại.
- Nguồn cần tài nguyên đang **KHÔNG có** ⇒ ghi `NOT_VERIFIED` (không phải PASS,
  cũng không phải FAIL).
- Không dùng "Metal không có API tương ứng" làm lý do để kết luận
  `HARDWARE_LIMITED`. Phải kèm capability query + các phương án emulation đã
  xét.

## 8. Nhãn môi trường chạy

| Nhãn | Nghĩa |
|---|---|
| `A11_DEVICE` | chạy trên iPhone 8/X (A11) thật |
| `SIMULATOR` | iOS Simulator |
| `OTHER_GPU` | GPU khác (ví dụ Mac Intel) — KHÔNG được dùng để khẳng định A11 |
| `REFERENCE_ONLY` | chỉ chạy trên reference backend, chưa chạy Metal |
| `NONE` | chưa xác minh ở đâu cả |

## 9. Kiểm thử

- Framework: CTest + `tests/test_util.h` (`TGLMT_CHECK` — KHÔNG dùng `assert()`,
  vì `NDEBUG` xoá biểu thức trong build Release ⇒ test "xanh giả").
- `tests/unit/` — unit theo subsystem; `tests/integration/` — luồng nhiều API;
  `tests/regression/` — hồi quy hành vi; `tests/api/` — bề mặt API;
  `tests/conformance/` — conformance; `tests/shaders/` — biên dịch GLSL.
- Test mới phải đăng ký tự động (CMake `file(GLOB ... CONFIGURE_DEPENDS)`).
- Test phải **tự kiểm tra giá trị**, không chỉ "gọi không crash".
- Mọi lỗi phải được tự phân tích, không đẩy cho người dùng.

## 10. Cạm bẫy đã biết (đừng phát lại)

1. **Error queue là FIFO.** Trước khi kiểm tra "lời gọi này sinh lỗi gì" phải
   xoá hết lỗi cũ (`while (glGetError() != 0) {}`).
2. **`tools/gen_stubs.py` chỉ nhận ĐỊNH NGHĨA hàm, không nhận lời gọi.** Nếu
   không, một file implementation gọi `glXxx()` sẽ khiến generator tưởng đã
   implement ⇒ thiếu stub ⇒ link fail. `gl_c_exports.cpp` cũng phải bị loại
   khỏi danh sách "đã implement" (nó chỉ là wrapper forward).
3. **Version-gate nằm ở lớp C ABI** (`include/tglmt/gl_c_api.h`), không ở lớp
   C++ nội bộ `tglmt::gl::`. Test muốn kiểm tra gate phải gọi qua C ABI.
4. **`glGetString(GL_VERSION)` trả con trỏ tới `thread_local std::string`** —
   đừng giữ con trỏ qua một lời gọi `glGetString` khác.
5. `kGlProfileMax` trong `include/tglmt/GlProfile.h` phải ≥ số hàm sinh được
   (hiện 1049). `tools/gen_c_exports.py` sẽ fail nếu vượt.

## 11. GLSL, shader correctness và hiệu năng

- API inventory chỉ đo bề mặt `gl*`; không chứng minh GLSL compile/link hay pixel
  đúng. Báo cáo API và shader phải tách riêng.
- Với lỗi shader, lần theo đủ chuỗi: source/preprocess → parse/semantic → IR/rewrites
  → MSL → Metal library → GL program link/reflection → pipeline → draw/pixel. Ghi
  chính xác lỗi nằm ở pha nào; không gộp mọi lỗi thành “thiếu API”.
- Đọc shader/log gốc nếu có. Tạo regression tối thiểu trước khi sửa; xác nhận test
  tái hiện FAIL trước fix, PASS sau fix. Test phải kiểm tra compile/link status,
  info log, reflection hoặc kết quả pixel phù hợp, không chỉ gọi API.
- Dùng `TGLMT_CHECK`, đăng ký test trong CTest, chạy đúng test mới và suite liên
  quan. Nếu thiếu shaderpack/device/reference, tiếp tục bằng ca tối thiểu và ghi
  đúng phần còn `NOT_VERIFIED`; không suy diễn là đã sửa lỗi thực tế.
- Phạm vi shader là GLSL Desktop theo version/profile được context hỗ trợ. ESSL
  không thay thế GLSL Desktop. Xử lý `#version`, profile và `#extension` theo spec;
  chỉ advertise extension khi hành vi của nó đã được triển khai và xác minh.
- Ưu tiên đối chiếu với spec và reference/pixel. “MSL biên dịch được” chỉ chứng
  minh cú pháp/backend chấp nhận, không chứng minh hành vi GLSL được giữ nguyên.
- Trước tối ưu, đo baseline riêng cho C++ build và shader runtime (dịch, Metal
  library, link/pipeline), trên corpus ghi rõ. Báo median/p95, cache lạnh/ấm,
  máy/OS/toolchain và lệnh chạy. Đo lại cùng cấu hình sau thay đổi.
- Tối ưu chỉ sau khi regression đúng. Tối ưu thuần tốc độ phải giữ output MSL
  byte-identical; nếu sửa semantics khiến output đổi, phải chứng minh bằng test
  spec/differential/pixel. Không chọn cache hay precompile trước benchmark.
- Không tuyên bố “100%” từ một corpus nhỏ hoặc feature smoke test. Ghi phiên bản,
  stage, profile, extension, test và môi trường đã chạy; phần thiếu là `MISSING`,
  chưa chạy là `NOT_VERIFIED`, lỗi là `INCORRECT`/`FAIL` phù hợp.
