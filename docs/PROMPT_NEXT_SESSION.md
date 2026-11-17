# Prompt cho phiên kế — TGLMT: tất cả phần còn lại

Bạn làm việc trong repo `/Volumes/project/TGLMT` — implementation **Desktop OpenGL
1.0–4.6 (Core + Compatibility Profile)** chạy trên Metal/iOS, target Apple A11.

**Quy tắc bắt buộc — đọc trước khi làm bất cứ gì:**

1. Đọc `AGENTS.md`, `PROGRESS.md`, `TODO.md`, `docs/DECISIONS.md` (đặc biệt **D-011** —
   kiến trúc builtin pass; **D-012** — CMake link OBJECT; **D-010** — agent không tạo
   build dir; **D-006** — enum từ header sinh tự động).
2. **Không** tạo build dir mới — mỗi cái ~2GB, đã làm đầy đĩa nhiều lần. Self-check:
   `./tools/agent_syntax_check.sh <file.cpp>` → in `OK <file>`. Build + ctest **chạy
   tập trung** bởi agent chính; bạn chỉ được sửa source, không được tự build ngoài
   `build-work` (đã có sẵn).
3. Tests dùng `TGLMT_CHECK(cond,msg)`, **không** dùng `assert()` (bị NDEBUG xoá).
4. Coverage **không** viết tay — dùng `tools/gen_coverage_report.py` (sinh
   `reports/API_COVERAGE_REPORT.md`).
5. **Không commit/push** trừ khi được yêu cầu.

**Trạng thái hiện tại (đã xong — KHÔNG làm lại):**

- **API surface 100%**: core 5830/5830, compatibility 12446/12446 (0 stub), đo bởi
  `tools/gen_coverage_report.py` từ `data/version_inventory.json` (registry Khronos
  `docs/khronos/gl.xml`).
- **GLSL đầy đủ 110→460**: built-in variable + function theo spec 4.60; coverage probe
  **324/324** (convert + biên dịch Metal thật). Cổng: `tools/glsl_probe.cpp` +
  `tests/shaders/test_glsl_coverage.cpp` (CTest PASS).
- **CTest 214/214 PASS.**
- Geometry shader emulation (GS→MSL compute, A6) đã có, có pixel test.
- **GLU chưa có.**

**Your nhiệm vụ — TẤT CẢ phần còn lại, theo thứ tự ưu tiên:**

### A. GLU (T1-S17) — chưa có `glu*` nào
Theo A3 trong AGENTS.md, GLU **có** trong phạm vi (chỉ GLUT không).
Nguồn spec: vendored guides / registry. Khronos không bán spec GLU độc lập —
dùng GL Spec 2.1 Appendix (GLU 1.3) + manpages (gluBuild2DMipmaps, gluNewQuadric,
gluTess*, gluLookAt, gluPerspective, gluOrtho2D, gluProject/gluUnproject,
gluErrorString, gluErrorCode, gluGetString…).

Việc:
1. `include/tglmt/glu.h` (C ABI, không exception/RTTI/STL qua ranh giới) +
   `src/glu/glu.cpp` (+ `glu_tessellator.cpp` nếu cần) + `tests/glu/test_glu*.cpp`
   (mỗi lệnh: giá trị đúng, không chỉ "không crash").
2. GLU tessellator: callback pipeline (begin/vertex/end/combine/error), gluTess* 
   family đầy đủ. Đây là phần khó — có thể tách riêng.
3. `gluBuild2DMipmaps` — dùng lại đường mipmap của repo (`src/gl/gl_texture.cpp`).
4. `gluErrorString` — trả const char*.
5. Đăng ký vào CMake (GLUT không, GLU có).
6. Báo cáo: số lệnh GLU implement / test PASS / không dùng hex enum tay (dùng header).

### B. Backend cho GLSL builtins mới (converter đã implement, backend chưa)
Converter đã thêm các uniform/builtins (`src/gl/glsl_builtins_*.cpp`), nhưng
backend Metal chưa điền giá trị. Đây là việc bắt buộc để shaders GLSL mới chạy
đúng trên thiết bị thật:

1. **`gl_DrawID`** — CPU-uniform `tglmt_DrawID`: thread MultiDraw ordinal qua
   `EmitDraw` (`src/gl/gl_draw.cpp:482-493`) → `AppleDrawGL`; vòng nạp VS uniform
   trong `AppleDrawGL` (~2880-2920): nếu `e.isVS && e.name == "tglmt_DrawID"` ⇒
   `uint32_t v = drawOrdinal` ghi vào `vsb.data() + e.offset`. Draw đơn = 0.
2. **`gl_NumSamples`** — uniform `tglmt_NumSamples`: vòng nạp uniform FS (cùng chỗ,
   nhánh `!e.isVS`): nếu `e.name == "tglmt_NumSamples"` ⇒ `uint32_t(target->sampleCount())`.
3. **`gl_BaseVertex`** — `[[base_vertex]]` là vertex-function param; converter phát
   `uint tglmt_baseVertex [[base_vertex]]`. Backend: `src/metal/AppleMetalBridge.mm:911-921`
   đang hardcode `baseVertex:0` → truyền giá trị thật; trong `AppleDrawGL` nếu program
   chứa `[[base_vertex]]` thì đi qua `drawIndexedPrimitives:… baseVertex:<bv>` (giữ
   nguyên IdxRewrite cho non-baseVertex). **Đừng double-count**: nếu cả hai đều cộng baseVertex ⇒ đỉnh lệch đôi.
4. **`gl_FogFragCoord`** per-fragment (hiện uniform); **`gl_FrontColor`** cho helper function.

### C. tglmtGetProcAddress — T0.7
`src/gl/gl_context_api.cpp` (hoặc `src/core/Context.cpp`): hàm trả NULL cho entry
point của extension **chưa advertise** (hiện trả con trỏ cho mọi entry point trong
bảng). App dựa vào kết quả để quyết định dùng extension. Test `test_context_version`.

### D. Extensions — 76 extension có hàm thật chưa implement
`python3 tools/coverage_dashboard.py` in danh sách 76. Ưu tiên theo usage:
- `GL_APPLE_flush_buffer_range`, `GL_APPLE_framebuffer_multisample`,
  `GL_APPLE_vertex_array_range`, `GL_APPLE_texture_range`, `GL_APPLE_object_purgeable`,
  `GL_APPLE_element_array`, `GL_APPLE_fence`, `GL_APPLE_copy_texture_levels`
- `GL_EXT_depth_bounds_test`, `GL_EXT_stencil_clear_tag`, `GL_APPLE_framebuffer_multisample`
- `GL_ARB_matrix_palette` (compat-only, khó)

Mỗi extension: implement hàm thật + test + **không** implement nếu chưa có Metal
path (phải honest-fail hoặc NOT_VERIFIED).

### E. Accumulation buffer — T1-S14 hoàn thiện phần còn lại
`glAccum`/`glClearAccum`: state đã có; cần buffer thật (RGBA16F texture +
full-screen pass), hoặc nếu sau khi nghiên cứu thấy Metal không khả thi
**phải ghi rõ bằng chứng** (như A6), không giả vờ.

### F. Geometry shader — phần còn lại
- `gl_Layer` trong GS (layered rendering), `gl_ViewportIndex`
- GS + XFB
- `invocations > 1` (hiện fail rõ tại link — giữ nguyên hoặc implement)

### G. A11 verification — nếu có thiết bị
`apps/aquarium` chạy trên iPhone 8/X: pixel thật của đường fixed-function draw +
GS emulation. Nhãn `A11_DEVICE` khi verify xong.

**Protocol khi làm (bắt buộc):**

1. Đầu phiên: đọc `AGENTS.md` + mục mới trong `PROGRESS.md` (cập nhật session cuối).
2. Self-check bằng `./tools/agent_syntax_check.sh`; build + ctest chạy tập trung.
3. Coverage bằng script (`gen_coverage_report.py`) — không viết tay.
4. Test mới: `TGLMT_CHECK`, đăng ký qua CMake `file(GLOB ... CONFIGURE_DEPENDS)`.
5. Hoàn thành → cập nhật `PROGRESS.md` (session mới), `TODO.md`, `docs/DECISIONS.md`
   nếu có quyết định kiến trúc mới; báo cáo theo đúng format của `AGENTS.md §6`
   (số liệu từ script, không từ miệng).
6. Không commit/push trừ khi được yêu cầu.
