# TESTING_STRATEGY.md — Chiến lược kiểm thử TGLMT

> **Phạm vi:** toàn bộ Desktop OpenGL **1.0 → 4.6**, **Core Profile** và
> **Compatibility Profile** (legacy fixed-function pipeline bao gồm).
>
> **Nguyên tắc số 1 của tài liệu này:** *định nghĩa tiêu chí trước khi chạy*.
> Mọi ngưỡng, metric, nhãn môi trường và quy tắc PASS/SKIP/NOT_RUN đều được
> viết ra **trước** khi test chạy, để một kết quả đỏ không thể bị "giải thích
> sau".
>
> **Nguyên tắc số 2:** *mọi con số trong tài liệu này phải do lệnh sinh ra*.
> Lệnh đo được ghi kèm. Tài liệu này **không** chứa số ước lượng.
>
> **Nguồn đặc tả:** `docs/khronos/glspec46.core.pdf` (OpenGL 4.6 Core Profile
> Specification, May 5 2022), `docs/khronos/GLSLangSpec.4.60.pdf`,
> `docs/khronos/gl.xml` (Khronos OpenGL-Registry, vendored). Mọi trích dẫn
> "§x.y" bên dưới là số mục trong **bảng mục lục của chính file PDF đó**.

---

## 0. Trạng thái tài liệu

| Mục | Giá trị |
|---|---|
| Phiên bản tài liệu | 2026-11-11 (PHASE 4 — Full Baseline Testing) |
| Cây được đo | `git rev-parse --short HEAD` = `a0347e0` (+ cây bẩn) |
| Môi trường đo | macOS 15.7.7 (Darwin 24.6.0), **x86_64**, GPU **Intel UHD Graphics 630** |
| **Nhãn môi trường** | **`OTHER_GPU`** (KHÔNG phải `A11_DEVICE`) |
| Build | `build-work/` — `CMAKE_BUILD_TYPE=Debug`, `TGLMT_APPLE_METAL=ON` |
| Backend | Metal thật (`src/metal/AppleMetalBridge.mm`), device `Intel(R) KBL Unknown` |
| Số đo tham chiếu | `reports/BASELINE_TEST_REPORT.md` |

> ⚠️ **Không được dùng số đo của tài liệu này để khẳng định hành vi trên
> Apple A11.** Mọi kết luận về A11 phải chờ số đo nhãn `A11_DEVICE`
> (§7).

---

## 1. Sáu nguyên tắc bất di bất dịch

| # | Nguyên tắc | Hệ quả thực thi |
|---|---|---|
| **P1** | **Test phải tự kiểm tra giá trị.** | Một test chỉ gọi API rồi in `PASS` là **không có giá trị chứng minh**. Mỗi test phải đọc lại trạng thái/pixel/buffer và so với **giá trị kỳ vọng viết tay** hoặc **giá trị suy ra từ spec**. |
| **P2** | **`SKIP` ≠ `PASS`, `NOT_RUN` ≠ `PASS`.** | Ba trạng thái tách biệt: `PASS`, `FAIL`, `SKIP` (có lý do), `NOT_RUN` (chưa chạy). Xem §4. |
| **P3** | **Không dùng `assert()` làm cơ chế fail.** | `NDEBUG` (Release) xoá biểu thức ⇒ "xanh giả". Bắt buộc dùng `TGLMT_CHECK` / `Check` trong `tests/test_util.h`. |
| **P4** | **Một lỗi trong một API ⇒ FAIL cả test.** | Không "bỏ qua" test để giảm thời gian chạy. |
| **P5** | **Baseline theo lát (version × subsystem).** | Không đợi baseline toàn bộ mới bắt đầu làm. Mỗi lát có trạng thái riêng. |
| **P6** | **Không claim thứ chưa đo.** | `FULLY_IMPLEMENTED` cần đủ 4 mục: code path `file:line` + test ID PASS + đối chiếu spec + nhãn môi trường. |

### 1.1 Định nghĩa "giá trị kỳ vọng"

Một giá trị kỳ vọng phải thuộc **một** trong ba loại, và test phải ghi rõ
loại nào:

| Loại | Nguồn | Ví dụ |
|---|---|---|
| **HẰNG SỐ** | Viết tay trong test, suy ra trực tiếp từ công thức trong spec | `glClearColor(1,0,0,1)` rồi `glReadPixels` ⇒ `(255,0,0,255)` |
| **SUY RA TỪ SPEC** | Tính bằng công thức của spec tại thời điểm chạy | Công thức blend `src*a + dst*(1-a)` (§17.3.6) |
| **ĐỐI CHIẾU THAM CHIẾU** | So với một implementation tham chiếu đã ghi nguồn/phiên bản | `glslangValidator` + `spirv-cross` (`test_converter_proof`) |

Nếu không thuộc loại nào ⇒ test **không đạt P1**.

---

## 2. Ma trận version × profile × subsystem

### 2.1 Chiều 1 — Version (19) × Profile (28 context)

Theo §9.1 đặc tả và `docs/OPENGL_PROFILE_MATRIX.md` §1: **profile chỉ bắt đầu
từ GL 3.2**. GL 1.0–3.1 không có khái niệm profile trong registry ⇒ mô hình
**một bộ API duy nhất** (`TGLMT_PROFILE_LEGACY`).

| Nhóm | Version | Số context | Profile |
|---|---|---:|---|
| **Legacy** (không tách profile) | 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 2.0, 2.1, 3.0, 3.1 | 10 | `Legacy` (1 bộ API) |
| **Có profile** | 3.2, 3.3, 4.0, 4.1, 4.2, 4.3, 4.4, 4.5, 4.6 | 9 × 2 = 18 | `Core` + `Compatibility` |
| | | **28 context** | |

Ràng buộc bắt buộc (đã kiểm bằng `tests/unit/test_context_version.cpp`):

* Core/Compatibility **chỉ** hợp lệ từ 3.2 trở lên; `< 3.2` ⇒ `Legacy`.
* `Compatibility` **không** được `FORWARD_COMPATIBLE`.
* `NO_ERROR` chỉ hợp lệ với context `DEBUG`.
* Mâu thuẫn ⇒ `tglmtCreateContextAttribs` trả `NULL` (không phải lỗi runtime).

| Version | Profile | Lệnh (registry) | Còn stub `UNIMPLEMENTED` | Có định nghĩa thật |
|---|---|---:|---:|---:|
| 1.0 | Legacy | 306 | 173 | 133 |
| 1.1 | Legacy | 336 | 189 | 147 |
| 1.2 | Legacy | 340 | 189 | 151 |
| 1.3 | Legacy | 386 | 206 | 180 |
| 1.4 | Legacy | 433 | 240 | 193 |
| 1.5 | Legacy | 452 | 240 | 212 |
| 2.0 | Legacy | 545 | 240 | 305 |
| 2.1 | Legacy | 551 | 240 | 311 |
| 3.0 | Legacy | 635 | 240 | 395 |
| 3.1 | Legacy | 647 | 240 | 407 |
| 3.2 | Core | 316 | 0 | 316 |
| 3.2 | Compatibility | 666 | 240 | 426 |
| 3.3 | Core | 344 | 0 | 344 |
| 3.3 | Compatibility | 724 | 240 | 484 |
| 4.0 | Core | 390 | 0 | 390 |
| 4.0 | Compatibility | 770 | 240 | 530 |
| 4.1 | Core | 478 | 0 | 478 |
| 4.1 | Compatibility | 858 | 240 | 618 |
| 4.2 | Core | 490 | 0 | 490 |
| 4.2 | Compatibility | 870 | 240 | 630 |
| 4.3 | Core | 534 | 0 | 534 |
| 4.3 | Compatibility | 913 | 240 | 673 |
| 4.4 | Core | 543 | 0 | 543 |
| 4.4 | Compatibility | 922 | 240 | 682 |
| 4.5 | Core | 653 | 0 | 653 |
| 4.5 | Compatibility | 1044 | 240 | 804 |
| 4.6 | Core | 657 | 0 | 657 |
| 4.6 | Compatibility | 1048 | 240 | 808 |

*Lệnh đo (giống hệt `docs/OPENGL_API_COVERAGE.md` §0):*

```bash
python3 - <<'EOF'
import re, pathlib, json
stub = pathlib.Path("src/gl/gl_stubs_gen.cpp").read_text()
stubs = set(re.findall(r"UNIMPLEMENTED: (gl[A-Za-z0-9_]+)", stub))
inv = json.load(open("data/version_inventory.json"))
compat = set(inv["versions"]["4.6"]["commands"]["compatibility"])
core   = set(inv["versions"]["4.6"]["commands"]["core"])
print("compat", len(compat), "stub", len(stubs & compat), "impl", len(compat - stubs))
print("core  ", len(core),   "stub", len(stubs & core),   "impl", len(core - stubs))
print("legacy", len(compat-core), "stub", len(stubs & (compat-core)))
EOF
```

> **Cảnh báo diễn giải.** "Có định nghĩa thật" = *có định nghĩa viết tay trong
> `src/gl/*.cpp`*, KHÔNG phải "hành vi đúng". Số hành vi đúng = số có test
> PASS + đối chiếu spec.

### 2.2 Chiều 2 — Subsystem (39 nhóm)

Phân loại lấy **nguyên văn** từ `docs/OPENGL_PROFILE_MATRIX.md` §3.4 (script
tự sinh từ `data/version_inventory.json`, không hardcode):

* **L01–L18** — fixed-function legacy bị Core Profile 3.2 loại bỏ (349 lệnh).
* **L19** — entry point vertex kiểu compatibility thêm ở GL 3.3 (30 lệnh).
* **L20** — truy vấn hậu kỳ `glGetn*` thêm ở GL 4.5 (12 lệnh).
* **M01–M19** — bề mặt hiện đại, theo chương của GL 4.6 Core spec.

Bảng trạng thái baseline cho **GL 4.6 Compatibility (1048 lệnh)**:

| # | Subsystem | Lệnh | stub `UNIMPLEMENTED` | định nghĩa thật |
|---|---|---:|---:|---:|
| L01 | Immediate mode (Begin/End + vertex specification) | 173 | 85 | 88 |
| L02 | Client-side vertex arrays | 12 | 12 | 0 |
| L03 | Fixed-function transform and matrix stack | 20 | 0 | 20 |
| L04 | Lighting, materials and shading model | 18 | 18 | 0 |
| L05 | Fog | 4 | 4 | 0 |
| L06 | Texture environment + texture coordinate generation | 15 | 15 | 0 |
| L07 | Display lists | 8 | 8 | 0 |
| L08 | Evaluators | 23 | 23 | 0 |
| L09 | Selection and feedback modes | 8 | 8 | 0 |
| L10 | Accumulation buffer | 2 | 2 | 0 |
| L11 | Pixel transfer modes + pixel drawing | 12 | 12 | 0 |
| L12 | Raster position + window position | 40 | 40 | 0 |
| L13 | Stipple (polygon + line) | 3 | 2 | 1 |
| L14 | Alpha test | 1 | 1 | 0 |
| L15 | Texture residency + priority | 2 | 2 | 0 |
| L16 | User clipping planes | 2 | 2 | 0 |
| L17 | Attribute stacks | 4 | 4 | 0 |
| L18 | Colour-index mode state | 2 | 2 | 0 |
| L19 | Compatibility-only vertex-type entry points (GL 3.3) | 30 | 0 | 30 |
| L20 | Compatibility-only n-suffixed queries (GL 4.5) | 12 | 0 | 12 |
| M01 | Buffer objects (§6) | 37 | 0 | 37 |
| M02 | Shaders, programs, program pipelines (§7) | 49 | 0 | 49 |
| M03 | Program interfaces, uniforms, uniform blocks (§7.3, §7.6) | 131 | 0 | 131 |
| M04 | Textures + texture objects (§8) | 90 | 0 | 90 |
| M05 | Sampler objects (§8.2) | 16 | 0 | 16 |
| M06 | Pixel storage modes + pixel rectangles (§8.4) | 3 | 0 | 3 |
| M07 | Framebuffer objects + renderbuffers (§9) | 46 | 0 | 46 |
| M08 | Vertex arrays, vertex attribs, VAO (§10.3) | 108 | 0 | 108 |
| M09 | Drawing commands + conditional rendering (§10.4, §10.9) | 33 | 0 | 33 |
| M10 | Rasterization + multisample state (§14) | 31 | 0 | 31 |
| M11 | Transform feedback (§13.3) | 16 | 0 | 16 |
| M12 | Per-fragment ops, blending, stencil, enable (§17) | 35 | 0 | 35 |
| M13 | Pixel read + copy (§18) | 3 | 0 | 3 |
| M14 | Compute shaders, images, memory barriers (§19) | 4 | 0 | 4 |
| M15 | Asynchronous queries (§4.2) | 19 | 0 | 19 |
| M16 | Sync objects + fences (§4.1) | 6 | 0 | 6 |
| M17 | Debug API + object labels (§20) | 11 | 0 | 11 |
| M18 | Context queries, strings, general state (§21, §22) | 16 | 0 | 16 |
| M19 | Miscellaneous general GL (§2, §17.4) | 4 | 0 | 4 |
| | **TỔNG (compat 4.6)** | **1048** | **240** | **808** |

Script sinh bảng (trích từ `docs/OPENGL_PROFILE_MATRIX.md` §3.4, chạy từ repo
root, đọc **chỉ** `data/version_inventory.json` + `src/gl/gl_stubs_gen.cpp`):
phân loại first-match-wins theo bảng rule L01–L20 + M01–M19; tổng khớp
1048/240/808 và **0 lệnh không phân loại được**.

### 2.3 Chiều 3 — Lát (slice) = version × profile × subsystem

Một lát là *đơn vị baseline nhỏ nhất có nghĩa*. Trạng thái của một lát:

| Trạng thái | Nghĩa là |
|---|---|
| `FULLY_IMPLEMENTED` | code path thật `file:line` + test ID PASS + đối chiếu spec + nhãn môi trường |
| `PARTIALLY_IMPLEMENTED` | có code nhưng chưa đủ điều kiện trên |
| `INCORRECT` | code có, hành vi sai (có ví dụ lỗi) |
| `STUB` | chỉ có stub `UNIMPLEMENTED` |
| `MISSING` | chưa có entry point |
| `EMULATED` | đường thay thế đã viết, đang dùng để mô phỏng feature không có phần cứng |
| `HARDWARE_LIMITED` | phần cứng thật không có ⇒ **phải kèm** capability query + các phương án emulation đã xét |
| `BLOCKED` | bị chặn bởi việc khác |
| `NOT_VERIFIED` | chưa đo được ở môi trường nào ⇒ **không phải PASS, không phải FAIL** |
| `NOT_APPLICABLE` | không áp dụng cho profile/version đó |

**Quy tắc lát (P5):** lát nào chưa có test thì lát đó là `NOT_VERIFIED`, và
**không được** chuyển sang `FULLY_IMPLEMENTED` chỉ vì "đã có prototype".

**Bảng lát hiện tại (đo được):** xem `reports/BASELINE_TEST_REPORT.md` §4.

---

## 3. Phân loại test

7 lớp. Mỗi lớp có **mục đích** và **tiêu chí đạt** khác nhau; không được trộn.

| # | Lớp | Thư mục | Mục đích | Tiêu chí đạt (P1) |
|---|---|---|---|---|
| **T1** | **unit** | `tests/unit/` | State machine, error code, đối chiếu state sau lệnh. Không cần GPU. | So sánh giá trị trả về / `glGetError` với **giá trị kỳ vọng viết tay**. |
| **T2** | **integration** | `tests/integration/` | Hành vi thật trên GPU: render → `glReadPixels` → so ảnh. | So **pixel** với giá trị kỳ vọng, dùng metric + ngưỡng §5. |
| **T3** | **regression** | `tests/regression/` | Mỗi lỗi đã sửa có 1 test hồi quy. | Test **phải FAIL khi revert fix** (đã kiểm với `test_regression_pipeline_cache_apple`). |
| **T4** | **api-surface** | `tests/api/` | Bề mặt API: đúng số hàm, đúng symbol export, đúng pname, đúng profile mask. | So với registry `gl.xml` (sinh tự động). |
| **T5** | **conformance** | `tests/conformance/` | Bề mặt tuân thủ *so với nguồn Khronos* (chưa phải CTS). | So với registry + catalog sinh tự động. |
| **T6** | **shader** | `tests/shaders/` | Biên dịch + **thực thi** GLSL, không chỉ compile. | Shader phải compile **và** tạo pixel đúng. |
| **T7** | **image-regression** | `tests/reference/` + `tools/compare_images.py` | So ảnh với bộ ảnh tham chiếu đã commit. | Chạy metric §5; vượt ngưỡng ⇒ FAIL, trừ khi có ngoại lệ đã đăng ký §5.3. |

Ngoài ra: `tests/truth/probe_gaps.cpp` là **runtime probe** để dò khoảng
trống đã biết — **không** phải CTest gate.

### 3.1 Inventory đo được (baseline PHASE 4)

Đo bằng `ctest -N` trong `build-work` + đếm file trong `tests/`:

| Lớp | Số test CTest | Ghi chú |
|---|---:|---|
| T1 unit | 95 | gồm `test_context_version`, `test_legacy_matrix`, `test_legacy_immediate`; **7** file hậu tố `_apple` |
| T2 integration | 77 | **73** file hậu tố `_apple` (cần Metal device) |
| T3 regression | 13 | **1** file hậu tố `_apple` (`test_regression_pipeline_cache_apple`) |
| T4 api-surface | 1 | `test_api_surface` |
| T5 conformance | 1 | `test_conformance_surface` |
| T6 shader | 1 | `test_shader_corpus` (+ 7 file GLSL trong `tests/shaders/glsl/`) |
| **Tổng** | **188** | khớp `ctest -N` ⇒ `Total Tests: 188`; **81** file hậu tố `_apple` |

* **188 / 188** file test có ít nhất một cơ chế fail (không có file "chỉ gọi
  API"): đo bằng cách đếm `TGLMT_CHECK` / `Check` / `CHECK` / `FAIL` + `return 1`.
* **64 / 188** file test đọc pixel (`glReadPixels`) — đây là nhóm bằng chứng
  mạnh nhất.
* **1 / 188** file còn dùng `assert()` thật: `tests/unit/test_converter_aquarium.cpp:69`.
  → Đây là **defect còn lại** của P3: trong build Release (`-DNDEBUG`) biểu thức
  bị xoá ⇒ test đó **không thể fail**. Phải chuyển sang `TGLMT_CHECK`.
  Trạng thái: `INCORRECT` (cơ chế kiểm thử), không phải lỗi implementation.

---

## 4. Quy tắc báo cáo PASS / FAIL / SKIP / NOT_RUN

### 4.1 Bốn trạng thái, định nghĩa chặt

| Trạng thái | Định nghĩa | Được tính vào "đã chạy" không? |
|---|---|---|
| **PASS** | Test chạy hết, **không** có assertion nào fail, **không** có nhánh SKIP nào được kích hoạt. | ✅ Có |
| **FAIL** | Có ≥ 1 assertion fail, hoặc test trả về mã ≠ 0. | ✅ Có |
| **SKIP** | Test **không chạy phần kiểm chứng** vì thiếu tài nguyên; **phải** in lý do cụ thể lên stdout và **phải** ghi ở nơi đếm được. | ⚠️ **KHÔNG** — tính riêng |
| **NOT_RUN** | Test chưa được chạy (chưa có GPU, chưa build, chưa có fixture). | ❌ **KHÔNG** |

### 4.2 Quy tắc bắt buộc

1. **`SKIP` không bao giờ được đếm là `PASS`.** Trong báo cáo, phải có 3 dòng
   riêng: `PASS = N`, `FAIL = M`, `SKIP = K`, `NOT_RUN = J`, và tổng phải bằng
   `ctest -N`.
2. **`SKIP` phải kèm lý do cụ thể**, không được là "không có GPU" chung chung.
   Mẫu chuẩn: `test_xxx SKIP: <lý do cụ thể> [điều kiện cần]`.
3. **Một test in `SKIP` nhưng trả mã 0 ⇒ CTest báo `Passed`.** Đây là tình
   huống đang tồn tại trong repo (xem §10, T7 gap). Khi báo cáo phải **đếm
   lại** các test có nhánh SKIP bằng cách đọc output, không chỉ tin `ctest`.
4. **`NOT_RUN` là trạng thái mặc định của bất kỳ thứ gì chưa đo.** Nguồn cần
   tài nguyên đang **không có** ⇒ `NOT_VERIFIED` (không phải PASS, không phải
   FAIL).
5. **Cấm bỏ test.** Một test bị xoá khỏi danh sách CTest là regression của
   chính quy trình kiểm thử và phải được ghi lại trong báo cáo.
6. **Cấm "bỏ qua" lỗi.** Không được `#if 0` hóa một assertion để làm xanh.

### 4.3 Kiểm tra "test có tự kiểm tra giá trị" (P1)

Một test được coi là **hợp lệ** khi thỏa **cả ba**:

1. Có ít nhất một **assertion so sánh giá trị** (`TGLMT_CHECK` / `Check` /
   `CHECK`), **không** phải chỉ `assert()`.
2. Giá trị kỳ vọng thuộc một trong 3 loại §1.1.
3. Test **không** chỉ kiểm tra "không crash" (không được có tổng số assertion
   = 0).

Lệnh đếm (dùng để tái lập số liệu §3.1):

```bash
python3 - <<'EOF'
import os, re
n_check = n_px = n_assert = 0
for d in ("unit","integration","regression","api","conformance","shaders"):
    for f in sorted(os.listdir(f"tests/{d}")):
        if not f.endswith(".cpp"): continue
        s = open(f"tests/{d}/{f}").read()
        n_check += len(re.findall(r'(?<![A-Za-z_])(?:TGLMT_CHECK|CHECK|Check|REQUIRE|expect|EXPECT)\s*\(', s))
        n_px   += int("glReadPixels" in s)
        n_assert += len([l for l in s.splitlines()
                         if re.search(r'(?<![A-Za-z_])assert\s*\(', l)
                         and not l.strip().startswith('//')
                         and '//' not in l.split('assert')[0]])
print("check-calls:", n_check, "| files reading pixels:", n_px, "| raw assert():", n_assert)
EOF
```

---

## 5. Metric so ảnh và ngưỡng theo nhóm

> **Định nghĩa TRƯỚC khi chạy** (theo §9.8 đặc tả). Ngưỡng dưới đây là **chính
> sách của TGLMT**; mỗi dòng đều trích **mục đặc tả** làm căn cứ và ghi rõ
> chỗ nào spec **bắt buộc chính xác** và chỗ nào spec **cho phép sai số**.

### 5.1 Bốn metric chuẩn

Với ảnh tham chiếu `R` (reference) và ảnh thực `T` (test), mỗi pixel có
`N = w × h × 4` mẫu (RGBA8, giá trị 0..255):

| Metric | Định nghĩa | Ký hiệu |
|---|---|---|
| **Max channel diff** | `max |R_i − T_i|` trên mọi kênh mọi pixel | `MAXD` |
| **% pixel lệch** | `%` pixel mà `max_c |R_c − T_c| > tol_channel` | `PCT` |
| **MAE** | `(1/N) · Σ |R_i − T_i|` | `MAE` |
| **SSIM** | Structural Similarity Index trên ảnh xám (Luma), cửa sổ 8×8, `C1 = (0.01·255)²`, `C2 = (0.03·255)²` | `SSIM` |

Bổ sung (bắt buộc cho ảnh có alpha): **Alpha coverage diff** = `|Σ α_R − Σ α_T| / (w·h)`
(đo hình dạng, không đo màu).

> **Quy tắc:** một ảnh **PASS** khi **mọi** metric áp dụng cho nhóm đều nằm
> trong ngưỡng. **Một metric vượt ngưỡng ⇒ FAIL**, kể cả khi metric khác rất
> tốt.

### 5.2 Ngưỡng theo nhóm (bảng chuẩn)

| Nhóm | Metric áp dụng | Ngưỡng PASS | Căn cứ đặc tả (GL 4.6 Core Profile) | Loại sai số |
|---|---|---|---|---|
| **rasterization** (coverage, edge, polygon, line, MSAA resolve) | `MAXD`, `PCT`, `MAE` | Tọa độ nguyên / trục căn: `MAXD = 0`, `PCT = 0 %`, `MAE = 0`. Cạnh phân số: `MAXD ≤ 1/255`, `PCT ≤ 0.05 %`, `MAE ≤ 0.5/255`. MSAA resolve: `MAXD ≤ 1/255`, `PCT ≤ 0.1 %` | §14.6.1 *Basic Polygon Rasterization* (quy tắc lấy mẫu tại tâm pixel ⇒ **tập fragment là duy nhất**); §14.9.1 *Pixel Ownership Test*; §14.6.3 *Antialiasing* + §14.6.6 (độ phân giải mẫu) | **Exact** cho tập fragment; **implementation-defined** cho giá trị mẫu MSAA ⇒ cho phép ±1 LSB |
| **shading / interpolation** (FS/VS output, varyings, gl_FragCoord) | `MAXD`, `PCT`, `MAE`, `SSIM` | `MAXD ≤ 2/255`, `PCT ≤ 0.5 %`, `MAE ≤ 1/255`, `SSIM ≥ 0.99` | §15.2.3 *Shader Outputs*; Appendix C.5/C.6 (độ chính xác lệnh `GLSL.std.450`: `OpFDiv` 2.5 ULP, `exp`/`log` 3 ULP ⇒ sai số ULP rồi lượng tử hoá xuống target UNORM8) | **Implementation-defined (ULP)** ⇒ biểu diễn bằng ngân sách LSB |
| **depth** | `MAXD` (đơn vị: giá trị depth đã lưu) | ≤ **1 bước lượng tử** của định dạng depth buffer: `DEPTH_COMPONENT24` ⇒ `5.96e-8`; `DEPTH_COMPONENT16` ⇒ `1.53e-5`; `DEPTH32F` ⇒ `0` (exact) | §17.3.4 *Depth Buffer Test* — giá trị depth được ghi vào buffer đúng định dạng ⇒ sai số **chỉ** do lượng tử hoá; §14.6.5 *Depth Offset* (offset tính theo công thức spec ⇒ **exact**) | **Exact** sau lượng tử hoá |
| **blend** | `MAXD`, `PCT`, `MAE` | Target **fixed-point** (UNORM/SNORM): `MAXD ≤ 1/255` (1 ULP), `PCT ≤ 0.1 %`, `MAE ≤ 0.5/255`. Target **float32**: sai số tương đối `≤ 2⁻²⁰` | §17.3.6 *Blending* — trích nguyên văn: *"If the color buffer is fixed-point, the components of the source and destination values and blend factors are each clamped to [0,1] or [−1,1] respectively for an unsigned normalized or signed normalized color buffer prior to evaluating the blend equation. If the color buffer is floating-point, no clamping occurs."* ⇒ công thức blend là **duy nhất**; sai số chỉ do lượng tử hoá 1 ULP. Với float32: IEEE-754 f32 (`2⁻²³ ≈ 1.2e-7`), cho phép `2⁻²⁰` để tính cả sai số tích luỹ | **Exact** (fixed-point, sau clamp+quantize) / **IEEE-754** (float) |
| **compute** | `MAXD` theo phần tử buffer | Output **integer**: `MAXD = 0` (exact). Output **float**: sai số tương đối `≤ 2⁻²⁰` (= 8 ULP của f32, vì 1 ULP = 2⁻²³) | §19 *Compute Shaders*; Appendix C.5/C.6 (ngân sách ULP từng lệnh `GLSL.std.450`) ⇒ cho phép `N · ULP` với `N` = số phép cộng/tích trong shader; ngân sách chọn `2⁻²⁰` tương ứng ~8 ULP | **Exact** (integer) / **ULP** (float) |
| **texture** (sampling, filtering, mipmap, completeness) | `MAXD`, `PCT`, `MAE` | `GL_NEAREST`: `MAXD = 0` (exact). `GL_LINEAR`: `MAXD ≤ 2/255`, `PCT ≤ 0.5 %`. Mipmap: chọn mức **exact** ⇒ `MAXD ≤ 2/255`. Texture **không hoàn chỉnh**: sample phải bằng `(0,0,0,1)` ⇒ `MAXD = 0` | §8.15 *Texture Magnification* + §8.14 minification (trọng số lọc **implementation-dependent** ⇒ cho phép ±1–2 LSB); §8.17 *Texture Completeness* + §8.17.2 *Effects of Completeness on Texture Application* (texture không hoàn chỉnh ⇒ giá trị lấy mẫu cố định ⇒ **exact**); §9.6/§9.7 (chuyển đổi sang định dạng framebuffer) | **Exact** cho NEAREST + incomplete; **implementation-defined** cho trọng số lọc |
| **framebuffer** (completeness, clear, blit, copy, storage) | `MAXD`, `PCT` + **enum chính xác** | `glClear`: `MAXD = 0` (exact). Blit `GL_NEAREST`: `MAXD = 0`. Blit `GL_LINEAR`: `MAXD ≤ 2/255`. `glCheckFramebufferStatus` trả **đúng enum** (byte-exact) | §17.4.3 *Clearing the Buffers* (giá trị ghi là **duy nhất** ⇒ exact); §18.3.1 *Blitting Pixel Rectangles* (bộ lọc); §9.4 *Framebuffer Completeness* + §9.4.2 *Whole Framebuffer Completeness* (enum trạng thái phải khớp tuyệt đối) | **Exact** (clear, enum) / **implementation-defined** (blit LINEAR) |

### 5.3 Sổ ngoại lệ (exception registry)

Mọi ngoại lệ so với ngưỡng §5.2 **phải** được ghi lại, không được "giải thích
miệng". Mỗi mục gồm: id case, nhóm, metric vượt, giá trị đo, lý do, spec
mục, người ký, ngày, điều kiện hết hạn.

| Exception ID | Case | Nhóm | Vượt | Lý do | Căn cứ | Ký | Hết hạn |
|---|---|---|---|---|---|---|---|
| _(chưa có mục nào)_ | | | | | | | |

Sổ nằm ở `tests/reference/exceptions.yaml` (chưa tạo — `MISSING`).

### 5.4 Ảnh tham chiếu chỉ dùng được ở version/profile nó hỗ trợ

Một ảnh tham chiếu chỉ có ý nghĩa khi được **sinh** ở đúng (version, profile)
mà ta định so sánh. Quy tắc:

* Ảnh tham chiếu **Core** không dùng để đánh giá **Compatibility** và ngược lại
  (bề mặt state khác nhau).
* Ảnh tham chiếu GL 4.6 **không** được dùng cho GL 3.0 (ngữ nghĩa raster/shading
  khác).
* Với (version, profile) **chưa có** ảnh tham chiếu ⇒ lát đó là
  **`NOT_VERIFIED`**, hoặc so **trực tiếp với giá trị suy ra từ spec** (§1.1
  loại 2) — và khi đó phải ghi rõ trong báo cáo là "so với spec", không phải
  "so với tham chiếu".

---

## 6. Bộ ảnh tham chiếu (reference image set)

> **Trạng thái hiện tại: `MISSING`.** Chưa có thư mục `tests/reference/`,
> chưa có công cụ sinh/so ảnh. Số đo "image regression" trong
> `reports/BASELINE_TEST_REPORT.md` là **`NOT_VERIFIED`**. Mục §6 là **đặc
> tả thiết kế** để agent sau implement, không phải bằng chứng.

### 6.1 Nơi lưu

```
tests/reference/
└── <version>_<profile>/              # ví dụ 4.6_core, 4.6_compatibility, 1.0_legacy
    └── <case_id>/
        ├── ref.png                   # ảnh tham chiếu (PNG)
        ├── meta.json                 # tham số render + hash
        └── diff.png                  # ảnh sai lệch (sinh khi FAIL, KHÔNG commit mặc định)
```

* `<version>_<profile>` dùng đúng nhãn của `data/version_inventory.json`
  (`1.0`…`4.6`, `core`/`compatibility`), nối bằng `_`.
* **Không** trộn ảnh giữa các profile: mỗi profile một thư mục riêng.

### 6.2 Format

| Trường | Yêu cầu |
|---|---|
| Định dạng ảnh | **PNG**, 8 bit/channel, RGBA, **không** interlace, không gamma/ICC chunk (để so sánh byte-stable) |
| Kích thước | cố định theo `meta.json`; so sánh phải cùng kích thước (không resize) |
| Sắp xếp | PNG không có thứ tự dòng ⇒ so sánh **phải** giải mã trước (row-major, top-left origin) |
| `meta.json` | `{ "version": "4.6", "profile": "core", "case": "...", "width": 64, "height": 64, "format": "rgba8", "viewport": [...], "color_mask": [...], "depth": {...}, "clear": [...], "backend": "apple", "device": "...", "source": "TGLMT <build>", "sha256": "<hash của ref.png>" }` |

### 6.3 Cách sinh

* Công cụ dự kiến: `tools/gen_reference.py` — **chưa có** (`MISSING`).
  Lệnh dự kiến: `python3 tools/gen_reference.py --case <case_id> --version <v> --profile <p> --out tests/reference/`.
* Ảnh **chỉ được commit** khi:
  1. Giá trị kỳ vọng là **HẰNG SỐ** hoặc **SUY RA TỪ SPEC** (không được chỉ vì
     "TGLMT vẽ ra thế này thì lưu lại" — đó là **bằng chứng giả**, tự tham chiếu).
  2. Có `meta.json` đầy đủ.
  3. Được review.
* Ảnh sinh trên GPU **khác** (ví dụ Intel) **không** được dùng làm tham chiếu
  cho A11 ⇒ ảnh tham chiếu phải mang **nhãn môi trường** trong `meta.json`
  (`device`, `backend`) và phải có **bộ riêng** cho từng nhãn mà ta tuyên bố.

### 6.4 Cách so

* Công cụ dự kiến: `tools/compare_images.py` — **chưa có** (`MISSING`).
  Lệnh dự kiến:
  `python3 tools/compare_images.py ref.png out.png --group shading --json out.json`.
* Công cụ phải:
  1. Kiểm tra kích thước khớp (không resize ⇒ FAIL nếu lệch).
  2. Tính đủ 4 metric §5.1 + alpha coverage nếu có alpha.
  3. Chọn ngưỡng theo **nhóm** trong §5.2 (nhóm lấy từ `meta.json`).
  4. Trả **exit code ≠ 0** nếu vượt ngưỡng (để dùng làm CI gate).
  5. Sinh `diff.png` (highlight pixel lệch) để review.
  6. In **mọi ngoại lệ** đã đăng ký trong `exceptions.yaml` kèm lý do.

### 6.5 Một test hình ảnh bắt buộc phải có

| Loại ảnh | Bắt buộc cho nhóm nào |
|---|---|
| Fixed-function rendering | rasterization, depth, blend, texture |
| Legacy compatibility rendering | rasterization, shading (fixed-function ≈ shading), texture env |
| Modern programmable rendering | shading, compute |
| Mixed legacy + modern resources | shading, texture |
| Offscreen rendering (FBO) | framebuffer |
| Multisample rendering | rasterization |
| Texture operations | texture |
| Framebuffer operations | framebuffer |
| Complex scenes | tất cả |

*(theo §9.7 đặc tả)*

---

## 7. Nhãn môi trường chạy

Mỗi kết quả đo **phải** mang một nhãn. Không nhãn ⇒ `NONE`.

| Nhãn | Nghĩa | Điều kiện xác minh |
|---|---|---|
| `A11_DEVICE` | chạy trên iPhone 8/X (Apple A11) thật | log thiết bị + `MTLDevice.name` + `supportsFamily(MTLGPUFamily4_v1)` |
| `SIMULATOR` | iOS Simulator | log `SIMULATOR_*` biến môi trường |
| `OTHER_GPU` | GPU khác (ví dụ Mac Intel) | `system_profiler SPDisplaysDataType` |
| `REFERENCE_ONLY` | chỉ chạy trên reference backend (Null / glslang+spirv-cross), chưa chạy Metal | log backend |
| `NONE` | chưa xác minh ở đâu cả | — |

### 7.1 Quy tắc

1. **Không được** lấy kết quả `OTHER_GPU` để tuyên bố `A11_DEVICE`.
2. Một bảng kết quả **phải** ghi nhãn ngay trên bảng, không ở chân trang.
3. Nếu một kết quả chỉ chạy trên Null backend ⇒ `REFERENCE_ONLY`.
4. Kết quả **differential** (so với `glslangValidator`/`spirv-cross`) ⇒ nhãn
   là của **device chạy TGLMT**, kèm ghi chú reference là `REFERENCE_ONLY`.
5. **Baseline PHASE 4** (tài liệu này + `reports/BASELINE_TEST_REPORT.md`) mang
   nhãn **`OTHER_GPU`** — máy đo là Intel iMac, GPU Intel UHD Graphics 630.

---

## 8. Baseline theo lát — quy trình

Theo §9.8: *"Baseline theo lát (version × subsystem), không chờ baseline toàn bộ
mới bắt đầu làm."*

### 8.1 Thứ tự lát

1. **Lát API surface** (T4/T5) — rẻ, chạy nhanh, bắt lỗi cấu trúc ngay.
2. **Lát state/error** (T1) theo subsystem M01→M19.
3. **Lát pixel** (T2) theo nhóm §5.2 — rasterization → shading → depth → blend →
   texture → framebuffer → compute.
4. **Lát legacy** (L01–L20) — sau khi có đường draw fixed-function.
5. **Lát shader** (T6) theo GLSL version (1.10 → 4.60).

### 8.2 Điều kiện đóng lát

Một lát được coi là **đóng** khi:

* có ≥ 1 test **PASS** cho mỗi entry point thuộc lát,
* test đó **tự kiểm tra giá trị** (P1),
* có **đối chiếu spec** ghi trong tên/biến của case,
* có **nhãn môi trường**,
* và **không còn** entry point nào của lát ở trạng thái `STUB`/`MISSING` —
  trừ khi lát được ghi nhận là `BLOCKED` với lý do.

### 8.3 Trạng thái lát hiện tại

Xem `reports/BASELINE_TEST_REPORT.md` §4–§6. Tóm tắt đo được:

* **Lát API surface**: đóng cho GL 4.6 (657 core / 42 compat-only / 1048 compat).
* **Lát GL 1.0–4.6 version-gate**: đóng (`test_context_version`, 28 context).
* **Lát legacy draw path**: **chưa đóng** — matrix stack (L03, 20 lệnh) và
  immediate mode (L01, 88/173) có state + primitive assembly nhưng **chưa nối
  vào Metal draw path** ⇒ `PARTIALLY_IMPLEMENTED`.
* **Lát image regression**: **`NOT_VERIFIED`** (chưa có bộ ảnh tham chiếu).

---

## 9. Pipeline CI

### 9.1 Các bước (đúng thứ tự)

```bash
# --- 0. build -------------------------------------------------------
cmake -S . -B build-work -DCMAKE_BUILD_TYPE=Debug \
      -DTGLMT_APPLE_METAL=ON -DTGLMT_BUILD_TESTS=ON
cmake --build build-work -j8

# --- 1. test: đếm trước, rồi chạy -----------------------------------
(cd build-work && ctest -N)                       # kỳ vọng: Total Tests: 188
(cd build-work && ctest -j8 --output-on-failure) # kỳ vọng: 100% tests passed out of 188

# --- 2. audit bề mặt API so với Khronos registry --------------------
python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
python3 tools/ext_matrix.py     --lib build-work/libtglmt.dylib --strict
python3 tools/ext_coverage.py    --dlsym build-work/libtglmt.dylib
```

### 9.2 Điều kiện gate (CI **đỏ** nếu vi phạm)

| # | Gate | Lệnh kiểm tra | Điều kiện đỏ |
|---|---|---|---|
| G1 | `ctest` | `ctest -j8 --output-on-failure` | exit ≠ 0 **hoặc** `tests passed` ≠ `Total Tests` từ `ctest -N` |
| G2 | Không test "xanh giả" do `assert()` | đếm `assert(` thật trong `tests/` | > 0 |
| G3 | Truth manifest | `tools/truth_manifest.py --strict` | exit ≠ 0 (thiếu symbol, hoặc advertise no-op) |
| G4 | Extension matrix | `tools/ext_matrix.py --strict` | exit ≠ 0 (dòng "advertised-lie") |
| G5 | Extension dlsym | `tools/ext_coverage.py --dlsym <lib>` | "thiếu symbol dlsym" > 0 |
| G6 | Coverage surface | `tools/coverage_dashboard.py` | không được giảm so với baseline đã commit |
| G7 | Số test | `ctest -N` | ≠ số baseline đã commit (188) mà không có patch ghi rõ |

### 9.3 Job theo nhãn môi trường

| Job | Backend | Nhãn | Ghi chú |
|---|---|---|---|
| `null` | `TGLMT_APPLE_METAL=OFF` | `REFERENCE_ONLY` | nhanh, chạy mọi commit |
| `apple-x86` | `TGLMT_APPLE_METAL=ON` | `OTHER_GPU` | chạy khi có GPU Intel/AMD |
| `ios-device` | `build-iphoneos` + thiết bị | `A11_DEVICE` | **chưa có** trong CI hiện tại (`NOT_VERIFIED`) |
| `ios-simulator` | Simulator | `SIMULATOR` | **chưa có** (`NOT_VERIFIED`) |

> ⚠️ Job `ios-device` là nơi duy nhất được phép gắn nhãn `A11_DEVICE`.
> Không job nào khác được gắn nhãn đó.

### 9.4 Benchmark (không phải gate)

`benchmarks/` **không** phải CTest gate (số phụ thuộc máy). Chạy tay theo
protocol 9 lần → median của median + min (xem `benchmarks/README.md`):

```bash
cd build-work
for i in $(seq 1 9); do ./bench_compile --repeats 30 --warmup 5 > /tmp/c_$i.json; done
python3 benchmarks/compare/bench_median.py /tmp/c_*.json -o /tmp/compile_merged.json
```

Baseline PHASE 4 đã đo: xem `reports/BASELINE_TEST_REPORT.md` §6.

---

## 10. Khoảng trống đã biết của chiến lược (danh sách trung thực)

| # | Khoảng trống | Hệ quả | Vì sao chưa đóng |
|---|---|---|---|
| **T1** | **Chưa chạy Khronos CTS.** | Lớn không đối chiếu tuân thủ lớn nhất còn lại | Không có EGL/GLX platform layer; xem `docs/CONFORMANCE_TESTING.md` |
| **T2** | **Chưa có số đo trên Apple A11.** | Hành vi trên GPU đích **chưa** xác minh | Không có thiết bị trong phiên đo; nhãn hiện tại là `OTHER_GPU` |
| **T3** | **Chưa có bộ ảnh tham chiếu + công cụ so ảnh.** | Lớp test T7 (image-regression) **không tồn tại** | `tests/reference/` chưa được tạo; `tools/gen_reference.py`, `tools/compare_images.py` chưa có |
| **T4** | **Null backend ≠ Metal backend.** | MRT, upload texture, mip chain, compute grid chưa được kiểm trên CI | Null backend là CPU shadow |
| **T5** | **Metric "test reference" chỉ là token match.** | Test chỉ *nhắc* `glFoo` cũng được tính là coverage cho `glFoo` | Cần phân tích call-graph |
| **T6** | **Chưa có fuzzing / property testing.** | Lỗi validation có thể lọt | Chưa có libFuzzer harness |
| **T7** | **Chưa có kiểm tra rò rỉ bộ nhớ (ASan/LSan) trong CI.** | Rò rỉ sẽ vô hình | Chưa nối vào build (có `build-asan/` cục bộ nhưng không phải gate) |
| **T8** | **1 file test còn dùng `assert()` thật.** | `tests/unit/test_converter_aquarium.cpp:69` **không thể fail** trong build Release | Chưa chuyển sang `TGLMT_CHECK` |
| **T9** | **Không có test nào cover 18/42 entry point compat-only.** | `glColorP3uiv`, `glVertexP4ui`, `glGetnPixelMapfv`… không có test trực tiếp | Đo bằng `truth_manifest.py`; xem `reports/BASELINE_TEST_REPORT.md` §5 |
| **T10** | **Không có test ở profile Compatibility trên GPU.** | Bề mặt legacy **chưa** được kiểm bằng hành vi | Đường draw fixed-function chưa tồn tại |

---

## 11. Chính sách hồi quy

1. Mỗi lỗi đã sửa trong implementation ⇒ có 1 test trong
   `tests/regression/test_regression_*.cpp`.
2. Test phải **FAIL khi revert fix**. Đã kiểm với
   `test_regression_pipeline_cache_apple` (báo
   `khoá cache pipeline phải chứa linkGen hiện tại của program`).
3. Test không chạy được phải **in lý do** và **không** được tính là pass.
4. `tools/truth_manifest.py --strict` chạy ở bước audit; symbol thiếu hoặc
   no-op được advertise ⇒ đỏ.
5. Không được xoá test, không được `#if 0` một assertion, không được hạ
   precision để làm xanh một test.
6. Mọi thay đổi hành vi phải kèm cập nhật `reports/BASELINE_TEST_REPORT.md`
   và lát tương ứng trong §8.3.
