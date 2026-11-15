# TODO.md — TGLMT

Ưu tiên theo tier (§11.2 của `other/prompt.md`). Mỗi mục ghi rõ:
**vì sao**, **file dự kiến**, **test ID**, **exit criteria**.

Trạng thái: `[ ]` chưa làm · `[~]` đang làm · `[x]` xong (có test PASS).

---

## T0 — Nền tảng (exit gate: tạo được context cho mọi version/profile; inventory tự động; harness chạy được)

- [x] **T0.1** Inventory tự động (version × profile) từ `gl.xml`
  → `tools/gen_version_inventory.py`, `data/version_inventory.json`
  → self-check 657 / 42 / 1048 PASS.
- [x] **T0.2** Bảng version-gate runtime (1048 entry, mask 38 bit)
  → `tools/gen_version_gate.py`, `src/gl/gl_version_gate_gen.cpp`.
- [x] **T0.3** Context versioned + profile + flags (public C ABI)
  → `include/tglmt/gl_context_api.h`, `src/gl/gl_context_api.cpp`
  → test `test_context_version` PASS.
- [x] **T0.4** Version-gate mọi entry point ở lớp C ABI
  → `src/gl/gl_c_exports.cpp` (sinh), `src/core/VersionGate.cpp`
  → test `test_context_version` PASS.
- [x] **T0.5** Public C ABI header (`gl_c_api.h`) + `getProcAddress` đầy đủ,
  gate theo version/profile.
- [x] **T0.6** `glGetString(GL_VERSION)` / `GL_MAJOR_VERSION` / `GL_MINOR_VERSION` /
  `GL_CONTEXT_FLAGS` / `GL_CONTEXT_PROFILE_MASK` / `GL_SHADING_LANGUAGE_VERSION`
  theo context thật.
- [ ] **T0.7** `tglmtGetProcAddress` phải trả `NULL` cho entry point của
  extension **chưa** advertise (hiện trả con trỏ cho mọi entry point trong bảng).
  *Vì sao:* app dựa vào kết quả để quyết định dùng extension; trả con trỏ cho
  extension không có là nói dối.
  *File:* `src/gl/gl_context_api.cpp`. *Test:* `test_context_version`.
- [x] **T0.8** `glGetIntegerv`/`glGetFloatv` gate **pname** theo version+profile cho
  **toàn bộ** pname fixed-function.
  → Bảng **sinh tự động** từ `docs/khronos/gl.xml` (nhóm `GetPName` + nhóm bổ
  sung): `tools/gen_compat_tables.py` → `src/gl/gl_compat_tables_gen.cpp`
  (189 pname compat-only). `src/gl/gl_get.cpp` (`CompatOnlyPname`) dùng bảng này.
  *Test:* `test_compat_profile_gates` PASS.
- [x] **T0.9** `glEnable`/`glDisable`/`glIsEnabled` phải **từ chối** enum
  fixed-function ở Core Profile.
  → Bảng **sinh tự động** từ registry (48 cap compat-only, có xử lý ALIAS
  `0x3000` = `GL_CLIP_PLANE0`/`GL_CLIP_DISTANCE0`): `tools/gen_compat_tables.py`
  → `src/gl/gl_compat_tables_gen.cpp`; gate tại `src/gl/gl_raster.cpp`
  (`RejectCompatOnlyCapInCore`).
  *Test:* `test_compat_profile_gates` PASS.
- [ ] **T0.10** Chia sẻ context (share group) + vòng đời resource khi hủy context
  (GL 4.6 spec §2.6.2). *Vì sao:* app dùng nhiều context share texture.
- [ ] **T0.11** `GL_EXTENSIONS`/`glGetStringi` phải phản ánh đúng extension khả
  dụng theo (version, profile).

---

## T1 — GL 1.0–2.1 + Compatibility (exit gate: chương trình legacy thực tế đạt
ngưỡng differential test)

- [x] **T1-S1** Matrix stack (26 entry point) — state + semantics + test.
  → `src/gl/gl_legacy_matrix.cpp`, test `test_legacy_matrix` PASS.
- [x] **T1-S2** Current vertex attributes + current texture coords theo unit +
  `glActiveTexture` đồng bộ đường legacy.
  → `src/gl/gl_legacy_vertex.cpp`, `src/gl/gl_texture.cpp`.
  *Test:* `test_legacy_immediate` PASS.
- [x] **T1-S3** Immediate mode: `glBegin`/`glEnd` + `glVertex*`/`glColor*`/
  `glNormal*`/`glTexCoord*`/`glMultiTexCoord*`/`glEdgeFlag*`/`glIndex*` +
  **ghép primitive**.
  *Đã xong:* 83 lệnh + ghép primitive cho 9 mode + **bảng lệnh bị cấm
  trong glBegin/glEnd** (spec GL 2.1 §2.6.3, `src/gl/gl_legacy_beginend.cpp`,
  guard trong 69 entry point) + `glEnd` **render ngay** (spec §2.6).
  *Còn lại:* `glArrayElement` (evaluator, thuộc T1-S12).
  *Test:* `test_legacy_immediate` + `test_legacy_begin_end` PASS.
- [x] **T1-S4** Đường draw legacy: MSL sinh cho fixed-function + nối vào
  `AppleDrawGL` mà **không** phá pipeline key.
  → `src/gl/gl_legacy_draw.cpp` (1223 dòng) + `include/tglmt/LegacyDraw.h`.
  Chương trình legacy = cặp `metal::ILibrary` nằm NGOÀI `Context::programs`;
  **không sửa `Context::PipelineKey`** (`vsLib`/`fsLib`/`programGen` đã đủ);
  shape baked vào MSL, giá trị đi qua uniform staging sẵn có.
  *Exit (ĐÃ ĐẠT):* vẽ được tam giác fixed-function qua đường draw chung.
  **CÒN LẠI:** đối chiếu **pixel** trên Metal thật (backend Null chỉ verify
  được state/trace/uniform ⇒ `PARTIALLY_IMPLEMENTED`).
  *Test:* `test_legacy_draw` PASS.
- [x] **T1-S5** Lighting & materials (18 lệnh: `glLight*`, `glLightModel*`,
  `glMaterial*`, `glColorMaterial`) + `glGetLight*`/`glGetMaterial*`.
  → `src/gl/gl_legacy_light.cpp`; state `LegacyLight`/`LegacyMaterial`/
  `LegacyLightModel`/`LegacyColorMaterial` trong `include/tglmt/LegacyState.h`.
  *Test:* `test_legacy_lighting` PASS.
- [x] **T1-S6** Fog (4 lệnh: `glFog*`) + `glFogCoord*`.
  → `src/gl/gl_legacy_fog.cpp`; state `LegacyFog`.
  *Test:* `test_legacy_fog` PASS.
- [x] **T1-S7** Texture environment & generation (15 lệnh).
  → `src/gl/gl_legacy_texenv.cpp`; state `LegacyTexEnv`/`LegacyTexGen`.
  *Test:* `test_legacy_texenv` PASS.
- [x] **T1-S8** Legacy rasterization: alpha test (`src/gl/gl_legacy_alpha.cpp`),
  clip plane, stipple, polygon mode, polygon offset, point size, color mask.
  → `src/gl/gl_legacy_raster.cpp` (clip plane, polygon/line stipple, point
  params) + `src/gl/gl_legacy_alpha.cpp`.
  *Test:* `test_legacy_raster` PASS (44 lệnh).
- [x] **T1-S9** Raster position / window position (48 lệnh).
  → `src/gl/gl_legacy_raster.cpp` (`glRasterPos*`, `glWindowPos*`).
  *Test:* `test_legacy_raster` PASS.
- [x] **T1-S10** Pixel ops: `glBitmap`, `glDrawPixels`, `glCopyPixels`,
  `glPixelTransfer*`, pixel maps, zoom.
  → `src/gl/gl_legacy_pixelops.cpp`. *Test:* `test_legacy_pixelops` PASS.
  *Còn lại:* `glPixelStore*` vẫn stub (xem T0.10).
- [x] **T1-S11** Display lists (8 lệnh) — command stream, `GL_COMPILE` /
  `GL_COMPILE_AND_EXECUTE`, nesting.
  → `src/gl/gl_legacy_displaylist.cpp` (+ `include/tglmt/LegacyDisplayList.h`
  cho recorder API). *Test:* `test_legacy_displaylist` PASS.
  *Ghi chú:* record được geometry + nhóm state (enable/disable, blend, depth,
  cull, front-face, polygon-offset, viewport, scissor, line/point width,
  matrix, shade-model, light, material, texenv, texgen).
- [x] **T1-S12** Evaluators (23 lệnh) — `glMap1*`, `glMap2*`, `glEvalCoord*`,
  `glEvalMesh*`, `glEvalPoint*`.
  → `src/gl/gl_legacy_evaluator.cpp`. *Test:* `test_legacy_evaluator` PASS.
  *Ghi chú:* đánh giá theo **Bernstein** (spec §5.1 eq. 5.1), `glGetMap*`
  theo signature thật `(map, value, data)` (ORDER/COEFF/DOMAIN).
- [x] **T1-S13** Selection & feedback (8 lệnh) — `glSelectBuffer`,
  `glRenderMode(GL_SELECT/GL_FEEDBACK)`, name stack, hit records.
  → `src/gl/gl_legacy_selection.cpp` (+ hook vào `src/gl/gl_legacy_draw.cpp`).
  *Test:* `test_legacy_selection` PASS.
- [x] **T1-S14** Accumulation buffer (2 lệnh) — **state only** (Metal không
  có accumulation buffer; KHÔNG giả vờ có).
  → `src/gl/gl_legacy_colorindex.cpp`. *Test:* `test_legacy_colorindex` PASS.
  *Còn lại:* buffer tích luỹ thật cần texture riêng (xem `docs/LEGACY_TO_METAL_MAPPING.md` §12).
- [x] **T1-S15** Client-side vertex arrays (13 lệnh) + client attribute state.
  → `src/gl/gl_legacy_clientarrays.cpp` + hook trong `src/gl/gl_draw.cpp`
  (`LegacyDrawClientArrays` → đọc client memory **hoặc** VBO theo offset).
  *Test:* `test_legacy_clientarrays` PASS.
  *Ghi chú:* quy tắc chuyển kiểu theo bảng "Integer Handling" của spec 4.2C
  p.60 (Vertex/TexCoord/Index: **cast thẳng**; Normal/Color: **normalize**).
- [x] **T1-S16** Attribute stack (4 lệnh) — `glPushAttrib`/`glPopAttrib`/
  `glPushClientAttrib`/`glPopClientAttrib`.
  → `src/gl/gl_legacy_attribstack.cpp`. *Test:* `test_legacy_attribstack` PASS.
- [ ] **T1-S17** GLU (tessellator, quadrics, `gluBuild2DMipmaps`) — A3 nói CÓ.

---

## T1.9 — Hiệu năng dịch shader (đo được, tối ưu được)

- [x] **T1.9.1** Benchmark **riêng phần dịch GLSL→MSL** (không Metal) trên
  corpus 86 shader thật (79 Minecraft + 7 của repo), theo protocol
  `benchmarks/README.md`.
  → `benchmarks/compilation/bench_translate.cpp` (target `bench_translate`),
  `--dump <dir>` cho cổng đối chiếu byte-identical.
- [x] **T1.9.2** Tối ưu translator **không đổi 1 byte MSL**: tra cứu tên O(1),
  `string_view` thay `std::string` tạm, copy theo lô thay push_back từng ký tự,
  guard "token kích hoạt không có ⇒ pass không thể đổi gì".
  → `src/gl/glsl_to_msl.cpp`. *Kết quả:* **−15,8 %** (736,6 → 620,3 µs/shader).
  *Cổng:* corpus 86/86 **byte-identical** + `ctest` 207/207 PASS.
  *Tài liệu:* `docs/SHADER_TRANSLATION_PERFORMANCE.md`,
  `reports/SHADER_TRANSLATION_BENCHMARK.md`.

---

## T2 — GL 3.0–3.3 (Core + Compat)

- [x] **T2.1** GLSL cũ (`attribute`/`varying` của ≤1.20) trong
  `glsl_to_msl.cpp` — map sang `in`/`out` + `[[attribute(N)]]`/`[[user(locnN)]]`.
  Gate đúng theo profile: ≤1.20 OK, 1.30/1.40 OK (deprecated), ≥1.50 **chỉ**
  profile `compatibility`.
  *Test:* `tests/shaders/test_glsl_legacy_syntax.cpp` PASS.
  *Còn lại:* `gl_FrontColor`/`gl_BackColor`/`gl_FogFragCoord`/`gl_ClipVertex`,
  built-in uniform (`gl_ModelViewMatrix`…) chưa hỗ trợ.
- [x] **T2.2** Geometry shader **emulation** (A6: bắt buộc, không `HARDWARE_LIMITED`).
  → GS dịch thành **MSL compute kernel** + draw buffer mở rộng:
  `include/tglmt/GeometryEmulation.h`, `src/gl/gl_geometry_emulation.cpp`,
  link path `src/gl/gl_program.cpp`, draw path `src/gl/gl_apple_draw.cpp`.
  *Test:* `test_geometry_emulation` PASS (có **ca pixel thật**).
  *Thiết kế:* `docs/GEOMETRY_SHADER_EMULATION.md`.
  *Còn lại:* `gl_Layer`/layered rendering, XFB + GS, `invocations > 1`,
  input adjacency (đều **fail rõ tại link**, không link-OK-rồi-vẽ-câm).
- [ ] **T2.3** Core/Compat behaviour difference còn sót (state, defaults,
  `glGetString` theo profile).

---

## T3 — GL 4.0–4.6

- [ ] **T3.1** GLSL 4.00 → 4.60 (chi tiết khác 4.60).
- [ ] **T3.2** Tessellation (TCS/TES ↔ compute + post-tessellation vertex) — đã có
  một đường tess cơ bản, cần kiểm chứng theo spec.
- [ ] **T3.3** Geometry shader emulation (nếu chưa xong ở T2).
- [ ] **T3.4** SPIR-V ingestion (`GL_ARB_gl_spirv`) — A1 cho phép, nhưng KHÔNG
  đưa SPIR-V vào đường chạy của GLSL→MSL (§6.3).
- [ ] **T3.5** Program pipeline objects + separate shader objects — đã có một
  phần, cần kiểm chứng đầy đủ.

---

## T4 — Extensions

- [ ] **T4.1** Kiểm kê extension riêng (đã có `docs/EXTENSION_MATRIX.md` +
  `docs/ext_matrix.json`) — chỉ công bố extension khi implementation thực sự
  đạt spec của extension đó.

---

## Hạ tầng / công cụ (làm song song được)

- [ ] **I.1** `docs/TESTING_STRATEGY.md`: định nghĩa metric so ảnh (max channel
  diff, % pixel lệch, SSIM) + ngưỡng theo nhóm **trước khi** chạy.
- [ ] **I.2** Bộ test hình ảnh (reference images) cho fixed-function, legacy
  compat, modern, offscreen, MSAA, texture/framebuffer ops.
- [ ] **I.3** Harness chạy test trên **thiết bị A11** + xuất log (đã có
  `apps/`, cần chạy lại cho 2 test mới).
- [ ] **I.4** Benchmark harness có baseline/warm-cache/cold-cache/median/p95
  (§10.4) cho cả legacy lẫn modern.
- [ ] **I.5** `xcodebuild -create-xcframework` + kiểm tra `lipo -info`,
  `otool -L`, `nm -gU` (A4).
- [ ] **I.6** Đo để chốt A7 (metal-cpp) và A8 (runtime vs precompile shader).
- [ ] **I.7** Symbol export policy: `OpenGLES.framework` của iOS có symbol `gl*`
  trùng tên ⇒ phải có export list rõ ràng + test app link đồng thời cả hai.

---

## Việc cần làm ngay (chặn các mục trên)

- [ ] **B.3** Đối chiếu **pixel** của đường draw fixed-function trên Metal thật
  (A11). Hiện mới verify được state/uniform/trace trên backend Null ⇒ trạng thái
  đường draw là `PARTIALLY_IMPLEMENTED`, **không** được ghi là hoàn chỉnh.
  *Cần:* chạy `apps/aquarium` (hoặc test tương tự) trên iPhone 8/X, chụp
  framebuffer, so với ảnh reference; gắn nhãn `A11_DEVICE`.
- [ ] **B.4** `GL_FOG_COORD` per-vertex + verify `GL_FLAT` bằng pixel.
  `GL_FOG_COORD_SRC == GL_FOG_COORD` hiện chỉ truyền uniform `currentCoord`
  (không theo từng đỉnh) ⇒ sai spec cho client-attribute fog coord.
- [ ] **B.5** `tglmtGetProcAddress` phải trả `NULL` cho entry point của
  extension **chưa** advertise (T0.7) — hiện vẫn trả con trỏ cho mọi entry
  point trong bảng ⇒ app tưởng extension có sẵn.
- [ ] **B.6** Kiểm tra lại enum của `gen_gl_headers.py` — session 2026-11-12 đã
  phát hiện 8 enum sai trong code legacy vì viết tay thay vì dùng header sinh
  tự động; nay code legacy đã dùng header ⇒ **không còn số viết tay**. Vẫn
  nên kiểm tra định kỳ bằng script đối chiếu `gl.xml` ↔ header.
