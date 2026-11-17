# PROGRESS.md — TGLMT

Trạng thái công việc. **Cập nhật cuối mỗi session** (đọc `AGENTS.md` §3).
Mọi khẳng định phải có bằng chứng `file:line`, test ID, output thật.

---

## Session 2026-11-16 — FULL GLSL

Mục tiêu: **GLSL đầy đủ** (OpenGL Desktop 1.0–4.6 ⇒ GLSL 110 → 460), giữ nguyên
behavior OpenGL Desktop sau khi compile, đo được bằng **cổng convert + biên dịch
Metal thật**.

### Đã làm

**1. Hạ tầng đo + cổng (verify được):**
- `tests/shaders/glsl_coverage_cases.h` — bảng case dùng chung; mỗi case yêu cầu
  converter không báo lỗi **và** MSL biên dịch được bằng `xcrun -sdk macosx metal -c`
  (000+ case).
- `tools/glsl_probe.cpp` — công cụ chẩn đoán in danh sách gap.
- `tools/gen_glsl_builtin_funcs.py` → `tests/shaders/glsl_builtin_funcs.inc`
  — 155 case built-in **function** (§8 GLSL 4.60), sinh tự động.
- `tests/shaders/test_glsl_coverage.cpp` — **CTest gate** (PASS).

**2. Kiến trúc "built-in pass tự đăng ký"** (để nhiều nhóm làm song song, không
sửa `glsl_to_msl.cpp` — file ~13k dòng):
- `include/tglmt/GLSLBuiltins.h` (+ `GLSLTextUtil.h`)
- `src/gl/glsl_builtins_registry.cpp` — registry tự đăng ký qua static init
  (id sort ⇒ deterministic).
- `glsl_to_msl.cpp`: 1 điểm gắn `RunGLSLBuiltinPasses()` (sau các pass sẵn có)
  + recompute lại uniform layout (uniform được thêm bởi pass) + `RunGLSLSourcePasses()`
  trước parse.

**3. Các nhóm built-in đã implement:**

| Nhóm | File | Nội dung |
|---|---|---|
| fn_math | `glsl_builtins_fn_math.cpp` | modf/frexp/roundEven, pack/unpack, matrixCompMult, outerProduct, not, uaddCarry/usubBorrow/umulExtended/imulExtended, mix(bool), bitcast |
| fn_texture | `glsl_builtins_fn_texture.cpp` | textureProj, textureProjOffset, textureProjLod, textureGatherOffsets, noise1..4, interpolateAt*, atomic counter + SSBO/UBO atomicLoad/Store, legacy texture2D*/Cube/3D |
| modern_io | `glsl_builtins_modern_io.cpp` | gl_SampleMask/MaskIn, gl_FragDepth, gl_Layer, gl_ViewportIndex, gl_PrimitiveID |
| legacy | `glsl_builtins_legacy.cpp` | GLSL 1.20 built-ins: gl_FrontColor/BackColor/SecondaryColor, gl_FogFragCoord, gl_ClipVertex, gl_TexCoord[N] (vec4), gl_ModelViewMatrix/ProjectionMatrix/ModelViewProjectionMatrix/NormalMatrix/TextureMatrix[8], gl_Fog, gl_ClipPlane |
| lang | `glsl_builtins_lang.cpp` | half/f16vec, `flat` varying int, uniform array của struct (source pass), invariant (source pass), SSBO unsized-array (source pass), vec2→vec4 assignment |
| final | `glsl_builtins_final.cpp` | gl_BaseVertex ([[base_vertex]]), gl_DrawID (CPU uniform), gl_NumSamples (CPU uniform), gl_ClipDistance[1] (honest error), TCS `gl_out` block |

**4. Kết quả đo:**
- **GLSL coverage probe: 0 → 324/324** (convert + Metal compile thật).
  4 N/A: 3 GS case (kiểm tra ở `tests/unit/test_geometry_emulation.cpp` — có ca pixel)
  + 1 giới hạn phần cứng (Metal chỉ 1 `[[clip_distance]]`).
- **CTest: 214/214 PASS** (trước: 208 — thêm test coverage + builtin tests).
- **Minecraft corpus (79 shader): MSL byte-identical** so với khi bắt đầu
  tối ưu dịch shader.
- `tools/gen_coverage_report.py` sinh `reports/API_COVERAGE_REPORT.md`:
  core 5830/5830, compatibility 12446/12446 (**100%, 0 stub**).

**5. CMake:** tests link `tglmt_objs` (OBJECT library) thay vì archive tĩnh
`tglmt` — lld loại các object chỉ có static-init registration của builtin pass ⇒
pass là no-op trong binary test. Runtime library không bị ảnh hưởng.

**6. Phụ thuộc backend còn lại (chỉ directive — chưa làm):**
- `gl_DrawID`: CPU phải ghi uniform `tglmt_DrawID` theo MultiDraw ordinal
  (`src/gl/gl_draw.cpp:482-493` thread `order` qua `EmitDraw` → `AppleDrawGL`;
  vòng nạp VS uniform trong `AppleDrawGL` ghi `e.loc >= 0` cho tên
  `tglmt_DrawID`; draw đơn = 0).
- `gl_NumSamples`: vòng nạp uniform FS: nếu `e.name == "tglmt_NumSamples"`
  ghi `target->sampleCount()`.
- `gl_BaseVertex`: `AppleMetalBridge.mm` truyền `baseVertex:` thật vào
  `drawIndexedPrimitives` (thay `baseVertex:0`), và nếu program dùng
  `[[base_vertex]]` thì đi qua đường không-idxRewrite.

### NOT_VERIFIED
- Chưa chạy trên Apple A11 thật (toàn bộ test là `REFERENCE_ONLY`/Null).
- Corpus Minecraft MSL: byte-identical bằng cổng thủ công (dump diff), không phải
  bằng pixel thật trên thiết bị.
- gl_DrawID/gl_NumSamples: converter path compile OK; giá trị chưa đúng khi chạy
  MultiDraw/fbo không đủ samples (chờ backend).

### Blocker còn lại (phần khác — xem prompt cho phiên kế)
- GLU chưa có (`glu*`).
- Extensions: 76 extension có hàm thật chưa implement.
- T0.7 `tglmtGetProcAddress` trả NULL cho extension chưa advertise.
- Accumulation buffer chưa có implementation thật (chỉ có state).
- GS còn: layered rendering (`gl_Layer` trong GS), XFB+GS, `invocations > 1`.
- gl_FogCoord per-vertex; gl_FrontColor… trong helper fn.
- gl_ClipDistance[1]: giới hạn phần cứng (documented, không fix được).

### Bước tiếp theo
1. Viết prompt cho phiên kế (GLU + tất cả phần còn lại) — xem báo cáo cuối.
2. Commit trạng thái GLSL đầy đủ.

---

## Session 2026-11-15 — Compatibility Profile 100 % + GS emulation + GLSL cũ + tối ưu dịch shader

### Bối cảnh

Session trước còn **203 stub** của Compatibility Profile. Session này đóng hết,
**không stub nào còn lại**, rồi làm tiếp 3 việc còn treo: GLSL ≤1.20,
**emulation geometry shader** (A6 bắt buộc), và **đo + tối ưu tốc độ dịch
shader**.

**Lưu ý vận hành (quan trọng cho session sau):** máy đã đầy phân vùng vì
mỗi agent tạo một build dir riêng (~2 GB). Đã dọn phân vùng chính
(4,1 GB → 9,1 GB trống) và **đổi quy trình**: agent **không** tạo build dir nữa
— dùng `tools/agent_syntax_check.sh` (`-fsyntax-only`) để tự bắt lỗi, còn
build + ctest chạy tập trung ở main agent.

---

### Đã làm

#### 1. Compatibility Profile: **203 → 0 stub**

State mở rộng trước (`include/tglmt/LegacyState.h`: clip plane, raster
position, stipple, point params, color index, accum, client vertex array,
pixel transfer/maps, evaluator maps, display list, selection/feedback, attrib
stack), rồi 11 nhóm lệnh:

| Nhóm | File | Lệnh | Test |
|---|---|---|---|
| Type-variant (i/s/ub) | `src/gl/gl_legacy_typevariants.cpp` | 62 | `test_legacy_typevariants` |
| Raster / window pos / clip / stipple | `src/gl/gl_legacy_raster.cpp` | 44 | `test_legacy_raster` |
| Evaluator | `src/gl/gl_legacy_evaluator.cpp` | 23 | `test_legacy_evaluator` |
| Display list | `src/gl/gl_legacy_displaylist.cpp` | 8 | `test_legacy_displaylist` |
| Selection & feedback | `src/gl/gl_legacy_selection.cpp` | 7 | `test_legacy_selection` |
| Client vertex array | `src/gl/gl_legacy_clientarrays.cpp` | 13 | `test_legacy_clientarrays` |
| Color index / accum / attrib stack | `src/gl/gl_legacy_colorindex.cpp`, `gl_legacy_attribstack.cpp` | 18 | `test_legacy_colorindex`, `test_legacy_attribstack` |
| Pixel ops / rect / misc / texenv query | `src/gl/gl_legacy_pixelops.cpp`, `gl_legacy_rect.cpp`, `gl_legacy_misc.cpp` | 28 | `test_legacy_pixelops`, `test_legacy_rect`, `test_legacy_misc` |

**Coverage đo được** (từ `data/version_inventory.json`): **mọi version 1.0 → 4.6,
cả Core lẫn Compatibility: 100 %**, 0 stub.

#### 2. Sửa lỗi spec thật phát hiện khi đối chiếu PDF

| Lỗi | Sửa |
|---|---|
| **Table 2.9 áp sai phạm vi**: agent áp `(2c+1)/(2^n−1)` cho **vertex/texture coord**; GL 2.1 §2.14 (PDF p.73) nói rõ chỉ *"Color, normal, and depth components"* | Sửa `gl_legacy_typevariants.cpp` (vertex/texcoord/multitexcoord → **cast thẳng**), cập nhật test |
| `LegacyState::renderMode` mặc định = `0x1C01` (GL_FEEDBACK) thay vì GL_RENDER | Sửa → `GL_RENDER` |
| `gl_get.cpp` map sai `0x0D37` (MAX_NAME_STACK_DEPTH bị ghi nhãn MAX_PROJECTION) + thiếu 7 pname render-mode/select/feedback | Sửa + thêm pname |
| `glWindowPos*` chưa nằm trong bảng lệnh cấm trong `glBegin/glEnd` (spec 4.2C §2.6.3 p.51) | Thêm vào `src/gl/gl_legacy_beginend.cpp` |
| `glGetProgramiv(GEOMETRY_*)` không trả giá trị | Sửa `src/gl/gl_get.cpp` |

#### 3. GLSL ≤ 1.20: `attribute` / `varying`

`src/gl/glsl_to_msl.cpp` nay hiểu `attribute`/`varying` (map sang `in`/`out` +
`[[attribute(N)]]` / `[[user(locnN)]]`), kèm built-in GLSL 1.20
(`gl_Vertex`, `gl_Normal`, `gl_Color`, `gl_MultiTexCoord0..7`, `gl_TexCoord0..7`).
Gate **đúng theo profile**: ≤1.20 OK · 1.30/1.40 OK (deprecated, spec 1.30
§1.2.1) · ≥1.50 **chỉ** profile `compatibility`.
→ `tests/shaders/test_glsl_legacy_syntax.cpp` PASS.

#### 4. Geometry shader **emulation** (A6)

Trước đó: GS **link OK** nhưng draw im lặng không vẽ (`gl_apple_draw.cpp:1936`)
⇒ vi phạm chính sách "fail rõ tại link" của repo. Nay: GS dịch thành **MSL
compute kernel** + draw buffer mở rộng (`EmitVertex` → ghi vào buffer, kèm
index + ranh giới primitive), có **ca pixel thật** trong test.
→ `include/tglmt/GeometryEmulation.h`, `src/gl/gl_geometry_emulation.cpp`,
`src/gl/gl_program.cpp` (link), `src/gl/gl_apple_draw.cpp` (draw),
`docs/GEOMETRY_SHADER_EMULATION.md`, `tests/unit/test_geometry_emulation.cpp`.
Phần chưa hỗ trợ **fail rõ tại link** (không link-OK-rồi-vẽ-câm).

#### 5. Đo + tối ưu tốc độ dịch GLSL → MSL

Benchmark riêng phần dịch (không Metal) trên corpus 86 shader thật:
`benchmarks/compilation/bench_translate.cpp` (+ `--dump <dir>` cho cổng đối
chiếu byte-identical).

Tối ưu trong `src/gl/glsl_to_msl.cpp`: tra cứu tên O(1) (hash map thay quét
tuyến tính) · `string_view` thay `std::string` tạm trong scanner · copy **theo
lô** thay push_back từng ký tự · guard *"token kích hoạt không có ⇒ pass không
thể đổi gì"* cho 5 pass.

**Kết quả đo (best-of-7, chạy đan xen baseline/optimized):**

| Chỉ số | Trước | Sau | Cải thiện |
|---|---|---|---|
| median của median (86 shader) | 63 350,9 µs | 53 347,7 µs | **−15,8 %** |
| µs / shader | 736,6 µs | 620,3 µs | **−15,8 %** |
| throughput | 2,013 MB/s | 2,380 MB/s | **+18,2 %** |

**Cổng đúng đắn:** corpus 86/86 **byte-identical** (`diff -rq` rỗng) +
`ctest` **207/207 PASS**. Cổng này đã **bắt được 1 bug thật** (nhân bản text
trong `RewriteIdents`) và `test_pack_regressions` bắt được 1 guard sai.
→ `docs/SHADER_TRANSLATION_PERFORMANCE.md`, `reports/SHADER_TRANSLATION_BENCHMARK.md`.

### Test

`ctest -j8` → **207/207 PASS** (baseline 194 → 207: +13 file test mới).

### NOT_VERIFIED

- **Chưa chạy trên Apple A11 thật** — mọi test chạy backend Null ⇒ nhãn môi
  trường `REFERENCE_ONLY` (không phải `A11_DEVICE`). Đường draw fixed-function,
  GS emulation: **có case pixel** nhưng mới trên Null backend.
- **Accumulation buffer**: chỉ có **state** (Metal không có accumulation
  buffer — không giả vờ có).
- **GS**: `gl_Layer`/layered rendering, XFB + GS, `invocations > 1`, input
  adjacency chưa hỗ trợ (fail rõ tại link).
- **GLSL 1.20**: `gl_FrontColor`/`gl_BackColor`/`gl_FogFragCoord`/`gl_ClipVertex`
  và built-in uniform (`gl_ModelViewMatrix`…) chưa hỗ trợ.
- **Color-index rendering**: Metal không có color-index framebuffer ⇒ chỉ có
  state + query.

### Blocker (còn lại)

1. **Chạy test trên iPhone 8/X (A11)** ⇒ nhãn `A11_DEVICE` + pixel thật.
2. `tglmtGetProcAddress` trả con trỏ cho entry point extension **chưa advertise**
   (T0.7).
3. Accumulation buffer thật (cần texture riêng).
4. GS: layered rendering + XFB + `invocations > 1`.
5. GLSL 1.20: built-in uniform + `gl_FrontColor`/`gl_FogFragCoord`.

### Bước tiếp theo

1. Chạy `apps/aquarium` trên iPhone 8/X (A11) — đối chiếu pixel của draw
   fixed-function + GS emulation.
2. T0.7 — `tglmtGetProcAddress` trả `NULL` cho extension chưa advertise.
3. GS: layered rendering (`gl_Layer`) + XFB.
4. Accumulation buffer thật (texture RGBA16F + full-screen pass).
5. GLSL 1.20: built-in uniform matrix + `gl_FrontColor`/`gl_FogFragCoord`.

---

## Session 2026-11-11 — Mở rộng từ "GL 4.6 Core" sang "GL 1.0–4.6 Core + Compatibility"

### Bối cảnh

Nhiệm vụ mới (`other/prompt.md`): TGLMT phải trở thành implementation **toàn bộ
Desktop OpenGL 1.0–4.6**, gồm Core Profile **và** Compatibility Profile, legacy
fixed-function pipeline, chạy thật trên Metal, target Apple A11.

Trạng thái trước session: repo có **54 195 LOC** (`src/**` + `include/**`),
**185** test CTest, implementation **OpenGL 4.6 Core Profile** — **không có**
bất kỳ entry point fixed-function nào (đo được: 348/1048 lệnh của bề mặt
GL 4.6 Compatibility chưa có; 18/39 nhóm chức năng legacy hoàn toàn vắng mặt).

---

### Đã làm

#### PHASE 0 — Full Repository Discovery

| Tài liệu | Nội dung |
|---|---|
| `docs/REPO_DISCOVERY.md` | Báo cáo kiến trúc thực tế + tầng nào tồn tại / không tồn tại (715 dòng, 470 trích dẫn `file:line`) |
| `docs/SOURCE_MAP.md` | Bảng từng file source + LOC + subsystem + test groups (197 dòng) |
| `docs/SUBSYSTEM_DEPENDENCY_GRAPH.md` | Đồ thị phụ thuộc thật (phân tích `#include`) + 8 vòng phụ thuộc (424 dòng) |

Kết luận quan trọng (có bằng chứng): **không tồn tại** "legacy compatibility
engine" trong code; state fixed-function **không có chỗ để ở** (`StateTracker`
không có field nào cho light/material/fog/texenv/matrix); `GetProcAddress` là
bảng ~70 entry viết tay ⇒ app legacy resolve `glBegin` sẽ crash lúc load.

#### PHASE 1 — Specification Inventory (tự động, theo version × profile)

| Tài liệu / file | Nội dung |
|---|---|
| `tools/gen_version_inventory.py` (1257 dòng) | Parse `docs/khronos/gl.xml` theo semantics `reg.py` PASS 1 → JSON theo (version × profile) |
| `data/version_inventory.json` (4.2 MB) | Inventory máy đọc được: 1048 command + 1808 enum + extension tách riêng |
| `tools/gen_version_gate.py` | Sinh bảng runtime gate (1048 entry, mask 38 bit) |
| `src/gl/gl_version_gate_gen.cpp` | Bảng gate sinh tự động |
| `docs/OPENGL_VERSION_MATRIX.md` | Ma trận 19 version × 2 profile, sinh từ JSON |
| `docs/GLSL_VERSION_MATRIX.md` | 13 version GLSL (1.10 → 4.60) + provenance `[PDF]`/`[KNOWLEDGE]` |
| `docs/OPENGL_PROFILE_MATRIX.md` | Ranh giới profile (bắt đầu ở **3.2**), 391 lệnh "có trong compat nhưng không có trong core" phân loại L01–L20 |
| `docs/OPENGL_API_COVERAGE.md` | Đo coverage hiện tại bằng `nm -gU` (700/1048 = 66.8 %) |

**Số đo thật** (từ `data/version_inventory.json`, tự kiểm tra trong tool):
GL 4.6 **core = 657**, **compat = 1048**; compat-only theo registry = 42;
"trong compat nhưng không có trong core" = 391 (349 legacy fixed-function +
30 `gl*P*ui` + 12 `glGetn*`). Profile split bắt đầu ở **GL 3.2**, **không** ở
2.0 như một số tài liệu cũ nói.

#### PHASE 2 — Legacy Pipeline Audit

| Tài liệu | Nội dung |
|---|---|
| `docs/LEGACY_PIPELINE_AUDIT.md` | Audit 20 nhóm fixed-function, mỗi nhóm có STATUS + bằng chứng grep/file:line |
| `docs/LEGACY_TO_METAL_MAPPING.md` | Chiến lược Metal cho từng cơ chế legacy + MSL sketch + khoảng cách semantic |
| `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` | 17 stage, 348 entry point, thứ tự phụ thuộc + nhóm song song hóa |

#### PHASE 3 — Metal A11 Feasibility

| Tài liệu | Nội dung |
|---|---|
| `docs/METAL_A11_CAPABILITIES.md` | Capability query + 19 kết luận HARDWARE_LIMITED (mỗi cái có capability query + phương án emulation bị loại) |
| `docs/METAL_FEATURE_MAPPING.md` | Ánh xạ từng nhóm feature GL → Metal (DIRECT / GENERATED_SHADER / GPU_EMU / CPU_EMU / HYBRID / LIMITED / BLOCKED) |
| `docs/HARDWARE_LIMITATIONS.md` | Phân biệt "giới hạn phần cứng thật" vs "thiếu implementation", trạng thái OPEN/CLOSED |

**Sửa 2 sai lệch trong tài liệu cũ (do agent phát hiện, có bằng chứng):**
- Dual-source blending **CÓ** trên A11 (`MTLGPUFamily4_v1` ✓, `MTLBlendFactorSource1*` từ iOS 11.0) ⇒ mapping cũ của TGLMT sai, không phải phần cứng thiếu.
- Metal dùng provoking vertex = **vertex ĐẦU**, không phải cuối ⇒ `docs/LEGACY_TO_METAL_MAPPING.md` §1.5 đã sửa.

---

### Code mới (T0 + T1)

#### T0 — Context versioned + profile + version-gate

| File | Vai trò |
|---|---|
| `include/tglmt/VersionGate.h` | Kiểu `GlProfile`, bảng gate, `VersionGateAllows()` |
| `src/core/VersionGate.cpp` | `GlVersionSlotIndex()`, `FindVersionGateIndex()`, `GateCheck()` |
| `src/gl/gl_version_gate_gen.cpp` | **Sinh tự động** — 1048 entry, mask 38 bit |
| `include/tglmt/gl_context_api.h` | **Public C ABI** tạo/quản lý context theo version+profile+flags |
| `src/gl/gl_context_api.cpp` | `tglmtCreateContextAttribs`, `tglmtMakeCurrent`, `tglmtGetProcAddress` (gate theo version/profile) |
| `include/tglmt/gl_c_api.h` | **Sinh tự động** — 1049 prototype `extern "C"` (đây mới là API công khai) |
| `include/tglmt/Context.h` | + `glMajor/glMinor/glProfile/glContextFlags/glVersionSlot/glGateMask`; + `SetVersionProfile()`, `VersionAtLeast()`, `ShadingLanguageVersion()` |
| `src/core/Context.cpp` | `SetVersionProfile()` dựng bitmap gate 1 lần/context; validate profile/flags |
| `src/gl/gl_error_string_debug.cpp` | `glGetString(GL_VERSION)` phản ánh version thật; `GL_SHADING_LANGUAGE_VERSION` theo version; `< 2.0` ⇒ INVALID_ENUM |
| `src/gl/gl_get.cpp` | `GL_MAJOR/MINOR_VERSION`, `GL_CONTEXT_FLAGS`, `GL_CONTEXT_PROFILE_MASK` theo context; pname version-gated; pname compat-only ⇒ INVALID_ENUM ở Core |
| `src/gl/glfw_shim.cpp` | `GetProcAddress` fallback sang bảng đầy đủ (1049 core/legacy + 1066 extension) |

**Quy tắc đã chốt (docs/DECISIONS.md D-002, D-003):**
- Profile hợp lệ: `Legacy` (< 3.2), `Core` / `Compatibility` (≥ 3.2). Yêu cầu
  mâu thuẫn ⇒ `tglmtCreateContextAttribs` trả `NULL`.
- Entry point không có trong (version, profile) ⇒ **GL_INVALID_OPERATION** và
  **không thực thi** (gate ở lớp C ABI).
- `glGetProcAddress` trả `NULL` cho entry point không hợp lệ trong context.

#### T1 — Legacy fixed-function (bước đầu)

| File | Vai trò |
|---|---|
| `include/tglmt/LegacyState.h` | `GLMatrix` (column-major 4x4), `MatrixStack` (depth 32), `LegacyState` |
| `src/gl/gl_legacy_matrix.cpp` | **26 entry point matrix stack** triển khai thật: glMatrixMode, glLoadIdentity, glLoadMatrixf/d, glLoadTransposeMatrixf/d, glMultMatrixf/d, glMultTransposeMatrixf/d, glPushMatrix, glPopMatrix, glTranslate*, glScale*, glRotate*, glOrtho, glFrustum |
| `include/tglmt/gl_legacy_types.h` | **Sinh tự động** — 428 enum compat-only (`GL_QUADS`, `GL_ACCUM`, `GL_LIGHT0`, `GL_MODULATE`, …) |
| `include/tglmt/gl_legacy.h` | **Sinh tự động** — 349 prototype legacy |
| `src/gl/gl_get.cpp` | `GL_MODELVIEW/PROJECTION/TEXTURE_MATRIX`, `*_STACK_DEPTH`, `GL_CURRENT_COLOR/NORMAL/TEXTURE_COORDS` |
| `src/gl/gl_legacy_vertex.cpp` | **83 lệnh** immediate mode + current vertex attributes: `glBegin`/`glEnd`, `glVertex*` (18), `glColor*` (32), `glNormal*` (4), `glTexCoord*` (16), `glMultiTexCoord*` (16), `glSecondaryColor*` (4), `glIndex*` (12), `glEdgeFlag*` (2) + **ghép primitive** theo spec |
| `src/gl/gl_texture.cpp` | `glActiveTexture` đồng bộ `legacy.activeTextureUnit` (spec GL 1.3 §2.8.1) |

**Semantics matrix stack đã kiểm chứng** (spec OpenGL 1.x/2.1):
ma trận column-major; `current = current * M` (M áp trước); transpose khi
nạp `glLoadTransposeMatrix`; `glOrtho`/`glFrustum` **chỉ** hợp lệ ở
`GL_PROJECTION` (nếu không ⇒ `GL_INVALID_OPERATION`); push/pop vượt độ sâu ⇒
`GL_STACK_OVERFLOW` / `GL_STACK_UNDERFLOW`; `r==l` ⇒ `GL_INVALID_VALUE`.

---

### Test

| Test ID | Nội dung | Kết quả |
|---|---|---|
| `test_context_version` | Tạo context cho **mọi** version 1.0–4.6 × profile hợp lệ (28 context), `GL_VERSION`/`GL_MAJOR_VERSION`/`GL_MINOR_VERSION`/`GL_CONTEXT_PROFILE_MASK`/`GL_CONTEXT_FLAGS`/`GL_SHADING_LANGUAGE_VERSION`, pname version-gating, gate entry point theo 4 ca, `getProcAddress` theo profile, từ chối attribs mâu thuẫn, self-check bảng gate (657/1048) | **PASS** |
| `test_legacy_matrix` | Matrix stack: identity, translate, thứ tự nhân (2 chiều), transpose, push/pop, stack overflow/underflow, ortho/frustum + lỗi, matrix mode sai, stack riêng theo mode, `GL_PROJECTION_MATRIX` đọc lại, Core Profile ⇒ INVALID_OPERATION + INVALID_ENUM | **PASS** |
| `test_legacy_immediate` | Immediate mode: cân bằng glBegin/glEnd, glBegin lồng, glBegin mode sai, glVertex ngoài glBegin, chụp current attributes theo từng đỉnh, ghép primitive cho 9 mode (kể cả `GL_QUAD_STRIP` = `(2i,2i+1,2i+3,2i+2)` và winding đảo của `GL_TRIANGLE_STRIP`), multi-texture current texcoord theo unit, Core Profile ⇒ INVALID_OPERATION | **PASS** |

**Output thật:**
```
100% tests passed out of 188
```
(`ctest -j8` trong `build-work`, build Debug, backend Null — nhãn môi trường
`REFERENCE_ONLY` cho state machine, **không** phải `A11_DEVICE`.)

---

### Trạng thái coverage (đo được, `docs/OPENGL_API_COVERAGE.md`)

- GL 4.6 **Core**: 657/657 command có prototype export (**PARTIAL**: mới có
  version-gate, hành vi runtime từng lệnh chưa đổi).
- GL 4.6 **Compatibility**: 1048 lệnh; **349 legacy fixed-function đã có
  prototype + gate**, trong đó **109** đã implement thật + test (26 matrix
  stack + 83 immediate mode / current vertex attributes); **240** còn là stub
  trung thực (`UNIMPLEMENTED` + giá trị an toàn).
- Extension: 1066 entry point trong bảng `getProcAddress`.

### NOT_VERIFIED

- **Chưa chạy trên Apple A11 thật.** Mọi test mới chạy trên backend Null
  (`SIMULATOR`/`REFERENCE_ONLY`). Cần chạy lại `tests/unit/test_context_version`
  và `test_legacy_matrix` trên iPhone 8/X để gắn nhãn `A11_DEVICE`.
- **Đường draw legacy chưa tồn tại**: matrix stack + current vertex attributes +
  immediate mode đã có state, đã gom đỉnh và **đã ghép primitive** (test kiểm
  được thứ tự đỉnh), nhưng **chưa** nối vào Metal draw path ⇒ chưa render được
  hình fixed-function. Trạng thái: `PARTIALLY_IMPLEMENTED`.
- **Lighting / fog / texture env / display list / evaluator / selection-feedback /
  pixel ops / accumulation** chưa implement.
- **GLSL cũ (1.10 → 3.30)** chưa có; `glsl_to_msl.cpp` mới chỉ hiểu GLSL 4.60.
- **Geometry shader emulation** chưa viết (A6 bắt buộc emulation — hiện chưa
  có, không được kết luận `HARDWARE_LIMITED`).
- 2 giới hạn Metal cần đo trên thiết bị: số texture argument table và giới hạn
  `[[stage_in]]` của fragment — tài liệu Apple hai ấn bản mâu thuẫn (31 vs 96;
  60 vs 124). Đã có đoạn đo trong `docs/METAL_A11_CAPABILITIES.md` §4.
- `docs/METAL_A11_LIMITATIONS.md` (bản cũ) **không** được sửa; bản mới ở
  `docs/HARDWARE_LIMITATIONS.md` (đã ghi rõ chỗ nào sai).

### Blocker

1. **Đường draw legacy** cần một chương trình MSL sinh cho fixed-function
   (lighting/fog/texenv) + pipeline key riêng + vertex layout legacy. Thiết kế
   đề xuất: `docs/LEGACY_TO_METAL_MAPPING.md` §"coexistence".
2. **Quy tắc API không hợp lệ bên trong `glBegin`/`glEnd`** cần bảng riêng
   (spec GL 1.x §2.6 liệt kê ~20 lệnh bị cấm).
3. **`glGetIntegerv` pname-level gate** hiện mới phủ 12 pname compat-only; các
   pname fixed-function còn lại (light/material/fog/texenv) chưa có → sẽ phải
   mở rộng `CompatOnlyPname` khi implement.
---

## Session 2026-11-12 — Fixed-function thật: lighting/material/fog/texenv + đường draw legacy + gate Core↔Compat

### Bối cảnh

Session trước dừng ở "state có, chưa render". Session này đóng lỗ hổng đó:
implement **fixed-function pipeline thật** (lighting/material/fog/texture env) +
**đường draw fixed-function** + **gate Core/Compatibility đúng theo registry**.

**Nguồn spec đã tải và đọc trực tiếp** (registry.khronos.org trả 403, dùng
mirror GitHub chính thức `KhronosGroup/OpenGL-Registry`):
`glspec21.pdf` (394 trang) + `glspec42.compatibility.20110808.withchanges.pdf`
(741 trang) + `glspec13.pdf` (296 trang). Mọi công thức dưới đây đã đối chiếu
với văn bản PDF, không suy đoán.

---

### Đã làm

#### 1. State fixed-function mở rộng (`include/tglmt/LegacyState.h`)

| Struct | Nội dung |
|---|---|
| `LegacyLight` | 8 nguồn sáng (spec §2.14.4 "at least 8 sets of light parameters"): acli/dcli/scli/Ppli/sdli/srli/crli/k0/k1/k2 + cờ enabled |
| `LegacyMaterial` | acm/dcm/scm/ecm/srm + color-index (am,dm,sm) |
| `LegacyLightModel` | acs, vbs (local viewer), tbs (two-side), ces (color control) |
| `LegacyColorMaterial` | face + mode |
| `LegacyFog` | mode, d, s, e, Cf, coordSrc, hint, fog coord hiện tại |
| `LegacyTexEnv` | mode, env colour, đủ tham số GL_COMBINE (RGB/ALPHA combiner, SRCn, OPERANDn, scale) |
| `LegacyTexGen` | 4 coord × (mode + plane) |

**Mặc định đúng spec** (spec GL 2.1 Table 2.10, PDF p.61) — đã ghi trong code:
material ambient (0.2,0.2,0.2,1), diffuse (0.8,0.8,0.8,1); **dcli/scli của
LIGHT0 = (1,1,1,1) nhưng của LIGHTi (i>0) = (0,0,0,1)**; Ppli = (0,0,1,0);
**vbs (LOCAL_VIEWER) mặc định = FALSE**; acs = (0.2,0.2,0.2,1);
ColorMaterial face=FRONT_AND_BACK, mode=AMBIENT_AND_DIFFUSE (spec §2.14.4).

#### 2. Entry point mới (38 lệnh, `src/gl/gl_legacy_{light,fog,texenv,alpha}.cpp`)

- **T1-S5 — Lighting & materials (18)**: `glLight{f,i,fv,iv}`,
  `glMaterial{f,i,fv,iv}`, `glLightModel{f,i,fv,iv}`, `glColorMaterial`,
  `glShadeModel`, `glGetLight{fv,iv}`, `glGetMaterial{fv,iv}`.
- **T1-S6 — Fog (4)**: `glFog{f,i,fv,iv}` + `glFogCoord{f,d,fv,dv}`.
- **T1-S7 — Texture env & gen (15)**: `glTexEnv{f,i,fv,iv}`, `glTexGen{f,i,fv,iv,d,dv}`.
- **Alpha test (1)**: `glAlphaFunc` (trước đây là stub).

**Quy tắc đặc biệt đã implement đúng spec:**

- **`glLight` POSITION/SPOT_DIRECTION transform bằng modelview TẠI LÚC GỌI**
  (spec GL 2.1 §2.14.2, PDF p.66): "The current model-view matrix is applied to
  the position parameter … when that position is specified"; SPOT_DIRECTION chỉ
  bằng 3×3 trên-trái. Sau đó đổi modelview KHÔNG di chuyển light đã đặt.
  → `src/gl/gl_legacy_light.cpp` (`TransformLightPosition`).
- **Table 2.9 (`(2c+1)/(2^31−1)`) CHỈ áp cho tham số MÀU**; position/direction/
  real (shininess, spot exponent, attenuation) chuyển kiểu TRỰC TIẾP
  (spec §2.14.2). Sai ở đây ⇒ vị trí (1,0,0,1) ra 1e-9.
  → `src/gl/gl_legacy_light.cpp` (`IntToFloat` vs `CopyIntsDirect`).
- **`glGetMaterialfv(FRONT|BACK)` trả CHỈ giá trị của mặt đó** (spec 4.2C §6.1.3
  p.493: "face must be either FRONT or BACK"); bản `FRONT_AND_BACK` trả 2×count
  (tương thích Mesa). Trước đó ghi 8 float vào buffer 4 float của caller ⇒
  **buffer overflow** (crash).
- **`glGetLightfv` trong eye space** — spec 4.2C p.493: "POSITION and
  SPOT_DIRECTION return values in eye coordinates".
- Lỗi: enum sai ⇒ INVALID_ENUM; spot exponent ∉[0,128], spot cutoff ∉[0,90]∪{180},
  attenuation < 0, shininess ∉[0,128] ⇒ INVALID_VALUE; lệnh bị từ chối **toàn bộ**
  (không ghi một phần).
- **Fog**: 3 phương trình spec eq. 3.29/3.30/3.31; f clamp [0,1]; chỉ RGB bị ảnh
  hưởng; `FOG_DENSITY < 0` ⇒ INVALID_VALUE (spec 4.2C p.366).
- **Texture env**: mode GL_REPLACE/MODULATE/DECAL/BLEND/ADD/COMBINE; đủ
  GL_COMBINE_RGB/ALPHA + SRCn_RGB/ALPHA + OPERANDn_RGB/ALPHA + scale.
- **TexGen**: OBJECT_LINEAR / EYE_LINEAR / SPHERE_MAP / REFLECTION_MAP /
  NORMAL_MAP; **SPHERE_MAP/REFLECTION_MAP/NORMAL_MAP với R hoặc Q ⇒
  INVALID_ENUM** (spec GL 1.3 §2.11 p.38–39).

#### 3. T0.9 + T0.8 — Gate Core ↔ Compatibility theo REGISTRY (không hardcode)

`tools/gen_compat_tables.py` (292 dòng) sinh `src/gl/gl_compat_tables_gen.cpp`:
- **48 enable cap compat-only** ⇒ ở Core Profile `glEnable`/`glDisable`/
  `glIsEnabled` sinh **GL_INVALID_ENUM** (trước đây chấp nhận im lặng ⇒
  "state nói dối app").
- **189 pname compat-only** ⇒ `glGetIntegerv`/`glGetFloatv` sinh
  **GL_INVALID_ENUM** ở Core (bảng tay cũ chỉ phủ 12 pname).

**Phát hiện ALIAS quan trọng** — registry dùng CÙNG giá trị cho cap core và
cap compat-only: `0x3000` = `GL_CLIP_PLANE0` (compat) **và** `GL_CLIP_DISTANCE0`
(core). Generator **loại** mọi giá trị có alias core ⇒ nếu không,
`glEnable(GL_CLIP_DISTANCE0)` ở Core sẽ bị từ chối oan ⇒ hỏng app dùng shader
thật. Cùng kiểu: `0x0D32` = `GL_MAX_CLIP_PLANES`/`GL_MAX_CLIP_DISTANCES`.
→ `tools/gen_compat_tables.py` (`build_group`, phát hiện alias).

#### 4. B.2 — Bảng lệnh BỊ CẤM trong glBegin/glEnd

`src/gl/gl_legacy_beginend.cpp` (129 dòng) + `include/tglmt/LegacyBeginEnd.h`.
Danh sách lấy từ spec GL 2.1 §2.6.3 (glspec21.pdf PDF p.19–20) — khớp văn bản spec:
- **Được phép**: Vertex/Color/SecondaryColor/Index/Normal/TexCoord/
  MultiTexCoord/VertexAttrib/FogCoord, ArrayElement, EvalCoord/EvalPoint,
  **Material**, CallList/CallLists, EdgeFlag.
- **Bị cấm** ⇒ INVALID_OPERATION: ma trận, viewport, lighting/lightmodel/
  colormaterial/shademodel, fog, texenv/texgen, enable/disable, draw*, pixel ops,
  selection/feedback/render mode, eval mesh, map1/map2, clip plane, alpha func,
  polygon mode/offset, point size, stipple, và **client-state** (spec cho phép
  "may or may not be generated" — TGLMT sinh lỗi cho an toàn).
- **Khớp TÊN lệnh, không khớp tiền tố**: `glColorMaterial` phải khớp base
  cấm `glColorMaterial`, KHÔNG được coi là lệnh được phép `glColor`
  (`NameIs` + `IsTypeSuffix`).
- Guard được cài vào **69 entry point** (26 matrix + 18 light + 4 fog +
  10 texenv + 1 alpha + 8 raster + 1 draw + glBegin/glEnd).

#### 5. T1-S4 — Đường draw fixed-function (BLOCKER B.1 đã đóng)

`src/gl/gl_legacy_draw.cpp` (1223 dòng) + `include/tglmt/LegacyDraw.h`:
- **Chương trình fixed-function = cặp `metal::ILibrary` (VS+FS) sinh ra**, nằm
  NGOÀI `Context::programs` ⇒ không phải GL program object (không xuất hiện
  trong `glGetProgramiv`, `glUseProgram` không chọn được).
- **Không sửa `Context::PipelineKey`**: `vsLib`/`fsLib`/`programGen` đã đủ tách
  chương trình legacy khỏi mọi program modern (đúng như
  `docs/LEGACY_TO_METAL_MAPPING.md` §2.4 dự đoán).
- **Dispatch**: fixed-function chỉ dùng khi (a) không có program object nào in
  use, (b) profile ≠ Core, (c) đó là draw fixed-function thật.
- **"Hình dạng"** (lighting/fog/alpha test/two-sided/flat/số texture unit) được
  BAKED vào MSL; **giá trị** (light, material, fog, ma trận, texenv) đi qua
  uniform staging sẵn có. Đổi shape ⇒ sinh lại chương trình (`linkGen++`).
- **`glEnd` giờ render NGAY** (spec GL 2.1 §2.6) — trước đó batch chỉ được
  flush khi có draw/clear kế tiếp ⇒ sai thứ tự GL.
- Vertex layout cố định 6×float4 = 96 B ⇒ `PipelineKey::stride` ổn định.

#### 6. Sửa 8 lỗi enum thật (do dùng số viết tay thay vì header sinh tự động)

| Enum | Sai | Đúng | Hậu quả nếu không sửa |
|---|---|---|---|
| `GL_SMOOTH` | 0x8DDA | **0x1D01** | `glShadeModel(GL_FLAT)` trả INVALID_ENUM |
| `GL_FLAT` | 0x8DDB | **0x1D00** | như trên |
| `GL_REPLACE` | 0x8001 | **0x1E01** | `glTexEnvi(..., GL_REPLACE)` bị từ chối |
| `GL_SUBTRACT` | 0x800A | **0x84E7** | combiner sai |
| `GL_DOT3_RGB` | 0x8570 | **0x86AE** | `0x8570` là `GL_COMBINE` — nhầm 2 khái niệm |
| `GL_EYE_LINEAR`/`GL_OBJECT_LINEAR` | đảo | **0x2400 / 0x2401** | texgen dùng sai mode |
| `GL_CONSTANT/LINEAR/QUADRATIC_ATTENUATION` | 0x0B47-49 | **0x1207-1209** | attenuation query trả sai |
| `GL_COLOR_INDEXES` | 0x8D56 | **0x1603** | material color-index sai |

**Sửa triệt để**: mọi hằng số enum trong code legacy giờ lấy từ header **sinh
tự động** (`gl46_types.h` / `gl_legacy_types.h`, từ `docs/khronos/gl.xml`) —
không còn số viết tay.

#### 7. Sửa 3 lỗi hành vi phát hiện qua bảng state registry

- **`glGetMaterialfv` ghi 8 float vào buffer 4 float của caller** ⇒ buffer
  overflow (crash). Đã sửa theo spec 4.2C §6.1.3.
- **`glGetIntegerv(GL_LOGIC_OP)`** — `GL_LOGIC_OP` (0x0BF1) đã bị **gỡ khỏi
  Core Profile từ 3.2** (registry); pname Core là `GL_LOGIC_OP_MODE` (0x0BF0).
  Test cũ `test_logic_op_state` assert hành vi KHÔNG đúng theo spec ⇒ đã sửa
  test + dùng khoá shadow 0x0BF0.
- **`include/tglmt/gl46_pnames.h` cũ** (964 pname) ⇒ sinh INVALID_ENUM oan cho
  pname hợp lệ. Đã regenerate (1011 pname).

---

### Test (6 test mới, tất cả PASS)

| Test ID | Nội dung | Kết quả |
|---|---|---|
| `test_legacy_lighting` | Mặc định spec (material/light/light model/color material/shade model), round-trip setter/getter, **transform POSITION theo modelview tại lúc gọi**, ràng buộc giá trị → INVALID_VALUE, enum sai → INVALID_ENUM, lệnh bị từ chối **toàn bộ**, `glGetMaterialfv` chỉ trả mặt được hỏi, Core ⇒ INVALID_OPERATION + pname ⇒ INVALID_ENUM, Compatibility ⇒ chạy thật | **PASS** |
| `test_legacy_fog` | Mặc định (EXP, d=1, s=0, e=1, Cf=(0,0,0,0), FRAGMENT_DEPTH), round-trip 4 biến thể, density<0 ⇒ INVALID_VALUE, mode sai ⇒ INVALID_ENUM, ptr null ⇒ INVALID_VALUE, Core ⇒ gate, Compatibility ⇒ chạy thật | **PASS** |
| `test_legacy_texenv` | Mặc định env + combiner theo spec GL 1.3 p.147, mặc định TexGen theo p.39, round-trip, **state per-texture-unit** (env lẫn texgen), R/Q + SPHERE_MAP ⇒ INVALID_ENUM, Core ⇒ gate | **PASS** |
| `test_legacy_begin_end` | 20 lệnh cấm ⇒ INVALID_OPERATION, 12 lệnh được phép ⇒ không lỗi, **lệnh bị cấm KHÔNG được thực thi** (state không đổi), glBegin lồng/glEnd lạ, Core ⇒ INVALID_OPERATION | **PASS** |
| `test_compat_profile_gates` | Self-check 2 bảng sinh tự động (48 cap / 189 pname), cap/pname core KHÔNG nằm trong bảng, **alias 0x3000 không bị reject**, Core ⇒ reject mọi cap + pname compat-only, Compatibility/Legacy 1.5 ⇒ cho phép | **PASS** |
| `test_legacy_draw` | Chương trình legacy nằm NGOÀI `Context::programs`, `GL_CURRENT_PROGRAM` = 0, geometry (3 đỉnh / 3 index), trace Null (Triangle, 3 index), uniform theo state (model-view, light, material, fog), 6 shape khác nhau, glEnd render ngay | **PASS** |

**Output thật:**
```
100% tests passed out of 194
Total Test time (real):   9.98 sec
```
(`ctest -j8` trong `build-work`, build Debug, backend Null — nhãn môi trường
`REFERENCE_ONLY`, **không** phải `A11_DEVICE`.)

---

### NOT_VERIFIED (quan trọng — chưa được chứng minh)

- **Chưa chạy trên Apple A11 thật.** Mọi test chạy backend Null ⇒ đường draw
  legacy **chưa** được đối chiếu với pixel thật. Trạng thái của đường draw:
  `PARTIALLY_IMPLEMENTED` (code + state + trace đúng; **pixel chưa verify**).
- **12 MSL sinh ra (6 shape × VS+FS) đã biên dịch được bằng compiler Metal
  thật** (`xcrun -sdk macosx metal -c` → 12/12 OK) nhưng **chưa** chạy trên GPU.
- **`GL_FLAT` (shade model)** có code + `[[flat]]` nhưng **chưa verify** pixel.
- **`GL_FOG_COORD`** (fog coord theo client attribute) mới chỉ có uniform
  `currentCoord`, **chưa** per-vertex ⇒ `PARTIALLY_IMPLEMENTED`.
- **Fixed-function kiểu vertex-array** (`glVertexPointer` + `glDrawArrays`
  không qua immediate mode) **chưa** triển khai ⇒ draw đó vẫn bị bỏ.
- **Display list / evaluator / selection+feedback / pixel ops / accumulation**
  chưa implement.
- **GLSL cũ (1.10 → 3.30)** chưa có; `glsl_to_msl.cpp` mới chỉ hiểu GLSL 4.60.
- **Geometry shader emulation** chưa viết (A6 bắt buộc — không được kết
  luận `HARDWARE_LIMITED`).

### Blocker (còn lại)

1. **Chạy test trên iPhone 8/X (A11)** để chuyển nhãn `REFERENCE_ONLY` →
   `A11_DEVICE` và **đối chiếu pixel** của đường draw fixed-function.
2. **Fixed-function vertex-array** (`glVertexPointer`/`glArrayElement`) — hiện
   đường draw chỉ nhận dữ liệu từ immediate mode.
3. **`GL_FOG_COORD` per-vertex** + **`GL_FLAT` verify pixel**.
4. **`tglmtGetProcAddress` trả `NULL` cho entry point extension chưa advertise**
   (T0.7) — hiện vẫn trả con trỏ cho mọi entry point trong bảng.
5. **GLSL 1.10/1.20/1.30/1.40/1.50/3.30** trong `glsl_to_msl.cpp`.

### Bước tiếp theo (đề xuất session kế)

1. **T1-S8 — Legacy rasterization** (clip plane, polygon stipple, line stipple,
   point size, polygon mode) + **T1-S15 — client-side vertex arrays** (mở
   khóa fixed-function vertex-array).
2. **T1-S11 — Display lists** (command stream + `GL_COMPILE`/`GL_COMPILE_AND_EXECUTE`).
3. **T1-S13 — Selection & feedback** (CPU) + **T1-S10 — Pixel ops**.
4. **T2.1 — GLSL 1.30/1.40/1.50/3.30** trong `glsl_to_msl.cpp`.
5. **Chạy `apps/aquarium` trên iPhone 8/X** để lấy pixel thật của đường draw
   fixed-function.
