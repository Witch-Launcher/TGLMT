# DECISIONS — TGLMT

Quyết định kiến trúc đã **chốt**. Không mở lại trừ khi có bằng chứng kỹ thuật
mới (kèm đo đạc). Mỗi quyết định ghi: ngày, nội dung, lý do, bằng chứng.

---

## A1–A10 — Chiến lược tổng thể (chủ sở hữu chốt 2026-11-11)

| # | Quyết định | Bằng chứng / ghi chú |
|---|---|---|
| A1 | **Dịch thẳng OpenGL 1.0–4.6 → Metal trong MỘT tầng.** Không dùng Mesa, Zink, MoltenVK, ANGLE, hay state tracker / GLSL→NIR của bên thứ ba. | TGLMT tự viết toàn bộ. Nguồn: `other/prompt.md` §1.2 (LỊCH SỬ SỬA 2026-11-11). |
| A2 | **Không có lớp trung gian bên ngoài.** IR/ABI nội bộ của TGLMT được phép (chi tiết hiện thực), nhưng không được chèn một OpenGL-implementation khác vào đường chạy. | `other/prompt.md` §1.2, §7 Layer 5. |
| A3 | **GLU: CÓ** trong phạm vi. **GLUT: KHÔNG** — GLUT là thư viện tiện ích, không phải OpenGL API. | `other/prompt.md` §1.2, §7 Layer 7.1. |
| A4 | **Đóng gói CẢ `.dylib` LẪN `.xcframework`.** Không phân phối qua App Store. | `other/prompt.md` §1.2, §15.2. |
| A5 | **Phạm vi chỉ là OpenGL Desktop.** OpenGL ES / ESSL không thuộc phạm vi, không phải phương án thay thế. | `other/prompt.md` §1.2, §3. |
| A6 | **Geometry shader BẮT BUỘC phải emulation.** Không được kết luận `HARDWARE_LIMITED` cho GS chỉ vì Metal không có GS stage. | `other/prompt.md` §1.2, §8.3, §11.2. Thiết kế emulation: `docs/LEGACY_TO_METAL_MAPPING.md` / `docs/METAL_FEATURE_MAPPING.md` (H19). |
| A7 | **Port Metal layer sang metal-cpp nếu hiệu năng tương đương hoặc tốt hơn — phải đo được.** | Chưa đo ⇒ chưa chốt. Hiện dùng Objective-C++ (`src/metal/AppleMetalBridge.mm`, ARC). |
| A8 | **Shader compile: runtime MSL vs precompile+cache — chọn theo số đo benchmark.** | Chưa đo ⇒ chưa chốt. Hiện: runtime compile (`src/gl/glsl_to_msl.cpp`). |
| A9 | **Reference cho differential testing: Minecraft, game OpenGL thật, và test do AI tự viết theo spec.** Không cần Mesa. | `other/prompt.md` §9.5. |
| A10 | **Chỉ những gì thực sự là OpenGL mới vào phạm vi (kể cả GLU).** | `other/prompt.md` §1.2. |

---

## D-001 — Registry Khronos là nguồn chân lý duy nhất cho API/enum/state-token

**Ngày:** 2026-11-11. **Trạng thái:** chốt.

Toàn bộ prototype, enum, và ranh giới (version, profile) của entry point được
sinh tự động từ `docs/khronos/gl.xml` (vendored Khronos OpenGL-Registry), theo
semantics chính thức của `xml/reg.py` PASS 1: `<require>`/`<remove>` có hoặc
không thuộc `profile="core"`, xử lý theo thứ tự version tăng dần, last-write-wins.

**Lý do:** trước đây dự án mang ba bản sao độc lập của cùng một phép tính, mỗi
bản có một lỗi riêng ⇒ con số "698 hàm core" thực chất là *core ∪
compatibility-only*. Số đúng cho GL 4.6 **Core Profile** là **657**; số cho
**Compatibility Profile** là **1048**.

**Bằng chứng:**
- `tools/gl46_catalog.py` (đơn vị suy luận dùng chung) — mô tả lỗi B1/B2.
- `tools/gen_version_inventory.py` — sinh `data/version_inventory.json`,
  tự kiểm tra 657 / 42 / 1048.
- `tools/gen_version_gate.py` — sinh `src/gl/gl_version_gate_gen.cpp`
  (1048 entry, mỗi entry một mask 38 bit = 19 version × 2 profile).
- `docs/OPENGL_VERSION_MATRIX.md`, `docs/OPENGL_PROFILE_MATRIX.md`.

---

## D-002 — Context có version + profile; entry point được gate theo (version, profile)

**Ngày:** 2026-11-11. **Trạng thái:** chốt (đã implement + test).

Một GL context của TGLMT mang `(major, minor, profile, flags)`. Profile:
`Legacy` (GL 1.0–3.1, registry chưa có khái niệm profile), `Core` (≥ 3.2),
`Compatibility` (≥ 3.2).

Mọi entry point của C ABI đi qua **version-gate**: nếu entry point không tồn
tại trong (version, profile) của context hiện tại ⇒ **GL_INVALID_OPERATION**
(GL 4.6 Core spec §2.3) và lời gọi **không được thực thi**.

**Quy tắc tạo context** (GL 4.6 Core spec App. E, App. K §K.3.3.66, §22.2):
- version ∈ 1.0 … 4.6;
- Core/Compatibility chỉ hợp lệ từ 3.2; `Legacy` chỉ hợp lệ dưới 3.2 ⇒ yêu cầu
  mâu thuẫn bị **từ chối** (trả `NULL`), không tự ý ép về profile khác;
- Compatibility + FORWARD_COMPATIBLE ⇒ mâu thuẫn ⇒ từ chối;
- NO_ERROR chỉ hợp lệ kèm DEBUG.

**Bằng chứng:**
- `include/tglmt/VersionGate.h`, `src/core/VersionGate.cpp` (runtime check).
- `src/gl/gl_version_gate_gen.cpp` (bảng sinh tự động).
- `include/tglmt/gl_context_api.h`, `src/gl/gl_context_api.cpp` (public C ABI).
- Test: `tests/unit/test_context_version.cpp` → PASS (187/187 ctest xanh).

---

## D-003 — Version-gate đặt ở lớp C ABI, không ở lớp C++ nội bộ

**Ngày:** 2026-11-11. **Trạng thái:** chốt.

`include/tglmt/gl_c_api.h` là **API công khai** của thư viện (1049 prototype
`extern "C"`). Lớp `tglmt::gl::` (C++) là lớp nội bộ, không gate.

**Lý do:** app dùng `dlopen`/`dlsym` và link trực tiếp vào symbol `glXxx`
phẳng; gate ở đó mới bao phủ đúng bề mặt API công khai. Đặt gate trong 1049
hàm C++ nội bộ sẽ là thay đổi lớn, rủi ro cao, và không cần thiết.

**Hệ quả bắt buộc:** test kiểm tra version-gate **phải** gọi qua
`gl_c_api.h`, không gọi `tglmt::gl::`.

---

## D-004 — Fixed-function state tách riêng khỏi StateTracker

**Ngày:** 2026-11-11. **Trạng thái:** chốt.

State fixed-function (matrix stack, current vertex attributes, immediate mode)
nằm trong `Context::legacy` (`include/tglmt/LegacyState.h`), **không** nhét vào
`StateTracker`.

**Lý do:** `StateTracker` là state của đường modern programmable pipeline đang
chạy production (Minecraft). Thêm state legacy vào đó làm rủi ro hồi quy và
làm pipeline-key cache phải mang theo state không liên quan. Tách riêng cho
phép đường modern không bị ảnh hưởng khi triển khai legacy.

---

## D-005 — Bảng enum tách `gl46_types.h` (core) và `gl_legacy_types.h` (compat-only)

**Ngày:** 2026-11-11. **Trạng thái:** chốt.

`include/tglmt/gl46_types.h` = enum của GL 4.6 **Core Profile** (1376).
`include/tglmt/gl_legacy_types.h` = enum chỉ có ở Compatibility Profile (428
enum, ví dụ `GL_QUADS`, `GL_ACCUM`, `GL_COMPILE`, `GL_LIGHT0`, `GL_MODULATE`).

**Lý do:** giữ nghĩa gốc của `gl46.h` ("Core 4.6"), đồng thời cho phép
implement legacy mà không làm mờ ranh giới core/compat — vốn là điều bị cấm
trong đặc tả.

**Bằng chứng:** `tools/gen_gl_headers.py` (sinh cả hai), `src/gl/gl_get.cpp`
(`CompatOnlyPname` → `GL_INVALID_ENUM` ở Core Profile).

---

## D-006 — Enum và state phải lấy TỪ HEADER SINH TỰ ĐỘNG, không viết tay

**Ngày:** 2026-11-12. **Trạng thái:** chốt.

Mọi hằng số enum dùng trong code phải lấy từ `include/tglmt/gl46_types.h`
(core) hoặc `include/tglmt/gl_legacy_types.h` (compat-only) — hai header này
do `tools/gen_gl_headers.py` sinh từ `docs/khronos/gl.xml`.

**Lý do:** session 2026-11-12 phát hiện **8 enum sai** trong code fixed-function
mới viết vì đặt số bằng tay:

| Enum | Sai | Đúng (registry) |
|---|---|---|
| `GL_SMOOTH` | 0x8DDA | 0x1D01 |
| `GL_FLAT` | 0x8DDB | 0x1D00 |
| `GL_REPLACE` | 0x8001 | 0x1E01 |
| `GL_SUBTRACT` | 0x800A | 0x84E7 |
| `GL_DOT3_RGB` | 0x8570 | 0x86AE (`0x8570` là `GL_COMBINE`) |
| `GL_EYE_LINEAR` / `GL_OBJECT_LINEAR` | đảo | 0x2400 / 0x2401 |
| `GL_CONSTANT/LINEAR/QUADRATIC_ATTENUATION` | 0x0B47-49 | 0x1207-1209 |
| `GL_COLOR_INDEXES` | 0x8D56 | 0x1603 |

Hậu quả thực tế: `glShadeModel(GL_FLAT)` trả `GL_INVALID_ENUM`;
`glTexEnvi(..., GL_REPLACE)` bị từ chối; attenuation query trả sai giá trị.

**Bằng chứng:** registry `docs/khronos/gl.xml` (giá trị ở cột `value` của
thẻ `<enum>`), đối chiếu bằng script trong `tools/`.

**Hệ quả bắt buộc:** mọi PR mới thêm entry point fixed-function phải dùng
macro từ header; reviewer phải reject code có số hex viết tay cho enum GL.

---

## D-007 — Bảng "state chỉ có ở Compatibility Profile" phải SINH TỰ ĐỘNG, có xử lý ALIAS

**Ngày:** 2026-11-12. **Trạng thái:** chốt.

`tools/gen_compat_tables.py` sinh `src/gl/gl_compat_tables_gen.cpp`:
- **enable cap** compat-only (48) ⇒ ở Core Profile `glEnable`/`glDisable`/
  `glIsEnabled` sinh `GL_INVALID_ENUM` (GL 4.6 Core §17.1 + App. E.2.2).
- **pname** compat-only (189) ⇒ `glGetIntegerv`/`glGetFloatv` sinh
  `GL_INVALID_ENUM` ở Core (GL 4.6 Core §22.1 + App. E.2.2).

**Bằng chứng:** nguồn là `docs/khronos/gl.xml` + bộ phân tích version/profile
dùng chung với `tools/gen_version_inventory.py` (semantics reg.py PASS 1) — không
có bảng viết tay nào ở đây.

**ALIAS — bắt buộc xử lý:** registry dùng CÙNG giá trị enum cho cap core và
cap compat-only:

| Giá trị | Tên compat-only | Tên core |
|---|---|---|
| 0x3000–0x3005 | `GL_CLIP_PLANE0..5` | `GL_CLIP_DISTANCE0..5` |
| 0x0D32 | `GL_MAX_CLIP_PLANES` | `GL_MAX_CLIP_DISTANCES` |

⇒ **mọi giá trị có alias core phải bị LOẠI khỏi bảng reject.** Nếu không,
`glEnable(GL_CLIP_DISTANCE0)` ở Core Profile sẽ trả `GL_INVALID_ENUM` ⇒ hỏng
app dùng shader thật (test `test_device_suite_apple` đã bắt được lỗi này khi
bảng viết tay đầu tiên được thêm).

**Quy ước khớp tên lệnh:** khi so khớp entry point với danh sách cấm/được
phép phải so **TÊN LỆNH** (base + hậu tố kiểu), KHÔNG so tiền tố — nếu so
tiền tố thì `glColorMaterial` sẽ khớp base `glColor` (lệnh được phép) và bị
bỏ qua. Xem `NameIs`/`IsTypeSuffix` trong `src/gl/gl_legacy_beginend.cpp`.

## D-008 — Geometry shader: EMULATE bằng MSL compute kernel, không kết luận HARDWARE_LIMITED

**Ngày:** 2026-11-15. **Trạng thái:** chốt.

**Bối cảnh.** Metal **không có** geometry shader stage. A6 trong `AGENTS.md`
quy định: *"Geometry shader BẮT BUỘC phải emulation. Không được kết luận
HARDWARE_LIMITED cho GS chỉ vì Metal không có GS stage."*

**Trạng thái trước:** GS **link OK** nhưng draw im lặng không vẽ
(`src/gl/gl_apple_draw.cpp` từ chối) ⇒ vi phạm chính sách "fail rõ tại link
thay vì link OK nhưng vẽ sai/câm" (xem phần tessellation cùng file).

**Quyết định.** Dịch GS thành **MSL compute kernel** + draw buffer mở rộng:
- `gl_in[]` ← capture buffer (dùng chung đường tess capture);
- `EmitVertex()` → ghi vertex hiện tại vào buffer mở rộng (bộ đếm atomic);
- `EndPrimitive()` → ghi index + ranh giới primitive (có cả EndPrimitive
  **ngầm** cuối shader theo GLSL 3.20 §8.10);
- vertex shader **passthrough** đọc buffer mở rộng → FS.

**Ranh giới:** phần chưa hỗ trợ (`gl_Layer`/layered rendering, XFB + GS,
`invocations > 1`, input adjacency, `gl_ClipDistance`, …) **phải fail RÕ TẠI
LINK** với log nói rõ — **không** bao giờ link-OK-rồi-vẽ-câm.

**Bằng chứng:** `src/gl/gl_geometry_emulation.cpp`, `src/gl/gl_program.cpp`
(link), `src/gl/gl_apple_draw.cpp` (draw), `docs/GEOMETRY_SHADER_EMULATION.md`,
`tests/unit/test_geometry_emulation.cpp` (có **ca pixel thật**).

**Nhãn môi trường:** `REFERENCE_ONLY` (Null backend) — **chưa** phải `A11_DEVICE`.

---

## D-009 — Tối ưu dịch shader: MSL phải BYTE-IDENTICAL, và cổng đối chiếu bắt buộc

**Ngày:** 2026-11-15. **Trạng thái:** chốt.

**Bối cảnh.** Người dùng yêu cầu *"tối ưu tốc độ dịch shader sao cho giữ nguyên
behavior OpenGL Desktop đã thiết kế sau khi compile"*.

**Quyết định.**
1. **Mọi tối ưu phải giữ MSL byte-identical.** Không "gần đúng", không "cùng
   nghĩa" — cùng **byte**. Lý do: MSL là hợp đồng với app (layout attribute,
   vị trí `user(locnN)`, thứ tự phát biểu, tên helper) và với linker Metal.
2. **Cổng đối chiếu bắt buộc, hai tầng:**
   - `bench_translate --dump <dir>` ⇒ `diff -rq` trên corpus 86 shader;
   - `ctest -j8` toàn bộ (đặc biệt `tests/unit/test_pack_regressions.cpp`).
   Không tầng nào thay được tầng nào: corpus Minecraft **không** phủ hết —
   lỗi "nhân bản text" trong `RewriteIdents` chỉ bị cổng (1) bắt, còn guard
   sai trên `PromoteBoolVecMix` chỉ bị `test_pack_regressions` bắt.
3. **Chỉ tối ưu khi đo được.** Mỗi tối ưu phải kèm số đo trước/sau; hướng đã
   thử mà **chậm hơn** (`unordered_set` trong `RenameLocalShadowUniforms`) thì
   **hoàn tác**, không giữ.

**Bằng chứng:** `docs/SHADER_TRANSLATION_PERFORMANCE.md`,
`reports/SHADER_TRANSLATION_BENCHMARK.md`, `benchmarks/baseline/translate.json`
vs `benchmarks/optimized/translate.json` (−15,8 %).

---

## D-010 — Agent không tạo build dir riêng (chỉ dùng syntax-check)

**Ngày:** 2026-11-15. **Trạng thái:** chốt.

**Bối cảnh.** Mỗi agent tạo một build dir riêng (~2 GB) ⇒ **đầy phân vùng**,
task bị dừng giữa chừng. Đã dọn phân vùng chính 4,1 GB → 9,1 GB trống.

**Quyết định.** Agent **không** tạo build dir. Thay vào đó:
`tools/agent_syntax_check.sh <file.cpp>` (compile `-fsyntax-only` với đúng cờ
của dự án) để tự bắt lỗi cú pháp ngay trong file mình viết; **build + ctest
chạy tập trung** ở main agent. Benchmark dùng chung `build-bench` (Release)
đã cấu hình sẵn — **không** tạo thêm.

**Bằng chứng:** `tools/agent_syntax_check.sh` (script trong repo),
`docs/SHADER_TRANSLATION_PERFORMANCE.md` §1 (ghi chú đĩa).


## D-011 — Kiến trúc "built-in pass tự đăng ký" cho GLSL (và mọi converter viết lại text)

**Ngày:** 2026-11-16. **Trạng thái:** chốt.

**Bối cảnh.** `src/gl/glsl_to_msl.cpp` là file đơn ~13k dòng. Nhiều nhóm built-in
GLSL (§7 built-in variables, §8 built-in functions) cần được implement song song.
Sửa chung một file ⇒ xung đột (và 12 tool tạo ~13 build dir làm đầy đĩa — xem D-010).

**Quyết định.**
1. Mỗi nhóm built-in nằm ở **file riêng** `src/gl/glsl_builtins_<group>.cpp`,
   **tự đăng ký** qua static initializer (`RegisterGLSLBuiltinPass` / `RegisterGLSLSourcePass`).
   Registry (`src/gl/glsl_builtins_registry.cpp`) sort theo id ⇒ thứ tự chạy ỔN ĐỊNH.
2. `glsl_to_msl.cpp` có **một điểm gắn duy nhất** cho body rewrite (đoạn "PASS DỊCH
   BUILT-IN GLSL", sau §"ConvertGLSLtoMSL") và một điểm cho source-level pass
   (`src = RunGLSLSourcePasses(...)`).
3. Pass có thể **thêm** biến input/output/uniform vào `ctx` (`inputs`/`outputs`/
   `uniforms` + `nextTempLocation`); converter tự sinh lại layout (uniformBufferSize)
   sau khi pass chạy.
4. Tín hiệu của pass: sửa `*ctx.body` / `*ctx.helpers` / `*ctx.mslExtra`.

**Cấm:** không rewrite text của nhóm khác; không đoán mò — phải chứng minh được
bằng biên dịch Metal thật (cổng `tests/shaders/test_glsl_coverage.cpp`).
- PASS = converter ok **và** MSL **biên dịch được** bằng `xcrun -sdk macosx metal -c` —
  convert "không lỗi" mà MSL không biên dịch được = hỏng âm thầm (bug thật phát hiện
  nhờ cổng này).
- Cổng này là **bộ lọc** về Metal `xcrun -sdk macosx metal -c`; mọi case GLSL phải giữ
  được.

**Bằng chứng:** `include/tglmt/GLSLBuiltins.h`, `include/tglmt/GLSLTextUtil.h`,
`src/gl/glsl_builtins_registry.cpp`, `tests/shaders/glsl_coverage_cases.h`,
`tests/shaders/test_glsl_coverage.cpp`, `docs/GLSL_VERSION_MATRIX.md` (pending update).

---

## D-012 — CMakeLists: test phải link OBJECT library, không phải archive tĩnh

**Ngày:** 2026-11-16. **Trạng thái:** chốt.

**Bối cảnh.** Test link `tglmt` (archive tĩnh) + lld ⇒ các object chỉ chứa
static-init registration của builtin pass (`src/gl/glsl_builtins_*.cpp`) bị lld
LOẠI (không symbol nào được tham chiếu) ⇒ mọi pass là NO-OP trong binary test ⇒
test đỏ oan / xanh giả.

**Quyết định.** Test link `tglmt_objs` (OBJECT library) + `-framework Metal
Foundation QuartzCore`. Runtime shared library không bị ảnh hưởng.

**Bằng chứng:** CMakeLists.txt thay đổi; test `test_glsl_coverage` chuyển từ
"đỏ oan" sang PASS thật sau sửa.
