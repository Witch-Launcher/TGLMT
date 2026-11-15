# CONFORMANCE_TESTING.md — Kế hoạch kiểm thử tuân thủ OpenGL (Khronos CTS)

> **Mục đích:** làm rõ TGLMT đứng ở đâu so với bộ kiểm thử tuân thủ chính
> thức của Khronos, **kế hoạch** cách chạy nó, và **quy tắc báo cáo** kết quả
> mà không được tự nhận "đã chạy đầy đủ CTS".
>
> **Trạng thái hiện tại (đo được 2026-11-11): `NOT_RUN`.**
> TGLMT **chưa** chạy một test case CTS nào. Không có kết quả CTS nào được
> tuyên bố ở bất kỳ đâu trong repo này.
>
> **Nhãn môi trường của mọi số đo trong tài liệu này:** `NONE` (chưa xác minh
> ở đâu cả) — vì chưa có lần chạy CTS nào.

---

## 0. Tóm tắt điều được biết và điều chưa biết

| Câu hỏi | Trả lời | Bằng chứng |
|---|---|---|
| TGLMT có **conform** OpenGL không? | **Chưa biết — không được tuyên bố.** | Không có lần chạy CTS nào (`NOT_RUN`) |
| CTS có tồn tại cho **GL 1.0–2.1** không? | **Không.** CTS chỉ có package từ `KHR-GL30` trở lên | §3.2 |
| CTS có hỗ trợ **Compatibility Profile** không? | Có, nhưng **chỉ một package**: `KHR-COMPAT-GL42` | §3.1 |
| CTS có chạy được với TGLMT không? | **Chưa** — thiếu platform layer (EGL/GLX) | §4 |
| TGLMT đã "pass CTS" chưa? | **KHÔNG.** Không có dữ liệu | `NOT_RUN` |

---

## 1. Nguồn chân lý

| Nguồn | Vai trò | Nơi lưu trong repo |
|---|---|---|
| **Khronos OpenGL Registry** (`gl.xml`) | Nguồn chân lý **duy nhất** cho prototype/enum/version/profile | `docs/khronos/gl.xml` (vendored) |
| **GL 4.6 Core Profile Specification** (PDF, May 5 2022) | Ngữ nghĩa, công thức, điều kiện lỗi | `docs/khronos/glspec46.core.pdf` |
| **GLSL 4.60 Specification** (PDF) | Ngữ nghĩa shader | `docs/khronos/GLSLangSpec.4.60.pdf` |
| **Khronos OpenGL CTS** | Bộ test tuân thủ chính thức | **CHƯA có** trong repo (§2) |
| **Extension specs** (863 extension) | Ngữ nghĩa extension | `docs/khronos/extensions/` |

> **Quy tắc:** mọi prototype/enum trong TGLMT phải sinh từ `gl.xml`
> (`tools/gen_gl_headers.py`). Không hardcode tay. Mọi test phải dẫn được
> về một mục của registry hoặc một mục của đặc tả PDF.

---

## 2. Khronos OpenGL CTS — tình trạng trong repo

### 2.1 Đo thật: CTS **không** có trong cây

```bash
$ cd /Volumes/project/TGLMT
$ find . -path ./build-work -prune -o -path ./.git -prune -o \
       \( -iname '*VK-GL-CTS*' -o -iname '*dEQP*' -o -iname '*gl_cts*' \
       -o -iname 'cts-runner*' \) -print
(không có kết quả)

$ grep -rniE "vk-gl-cts|dEQP|glcts" CMakeLists.txt tools/ scripts/
(không có kết quả)
```

⇒ **Không có mã nguồn CTS, không có runner, không có target CMake nào chạy
CTS.** Trạng thái: `MISSING`.

### 2.2 Chỗ đặt CTS khi vendor

```
third_party/VK-GL-CTS/          # submodule hoặc tarball đã pin
third_party/VK-GL-CTS/CMakePresets.json
```

* **Pin** một commit cụ thể (không theo `main`) để kết quả tái lập được.
* Baseline tham chiếu khi lập kế hoạch: `VK-GL-CTS` `main` =
  `a17b2ea469b1192fefc82ac856e8296cb409f68f` (2026-11-09). Đây là mốc tham
  chiếu để **đọc** cấu trúc; khi vendor phải chọn và ghi rõ commit đã pin.
* CTS dùng license Apache-2.0 ⇒ được vendor nhưng **phải giữ nguyên**
  `LICENSE`/`NOTICE` và không sửa file nào của CTS (sửa làm mất ý nghĩa
  "chạy CTS").

### 2.3 Bố cục module CTS (đọc từ commit tham chiếu)

```
VK-GL-CTS/
├── modules/egl/            # EGL test package (dEQP-EGL)
├── modules/glshared/
├── external/openglcts/     # ← module OpenGL DESKTOP
│   ├── modules/gl/         #   gl3c* (GL 3.x) + gl4c* (GL 4.x)
│   ├── modules/common/     #   dùng chung
│   ├── data/gl_cts/        #   dữ liệu test (.test, .bin)
│   └── docs/
└── framework/, executor/, targets/
```

> Module OpenGL **desktop** nằm ở `external/openglcts/modules/gl` (không phải
> `modules/gl` như ở các bản CTS cũ). Số file trong module này:
> **131** (đo bằng GitHub trees API, recursive=1, `truncated: false`).

---

## 3. Bảng nhóm test CTS × version/profile

### 3.1 Các test package của CTS cho OpenGL desktop

Nguồn: `external/openglcts/modules/gl/glcTestPackageRegistry.cpp`
(hàm `registerPackages`). Tên package **là** prefix của case name, và CTS
**quyết định version/profile của context theo prefix đó**
(`external/openglcts/README.md`: *"the GL context version is determined by the
case name"*).

| Package (case prefix) | Context type mà CTS yêu cầu | Profile | TGLMT version tương ứng |
|---|---|---|---|
| `KHR-GL30` | `ContextType(3, 0, PROFILE_CORE)` | Core | 3.0 (Legacy) |
| `KHR-GL31` | `ContextType(3, 1, PROFILE_CORE, CONTEXT_FORWARD_COMPATIBLE)` | Core + forward-compatible | 3.1 (Legacy) |
| `KHR-GL32` | `ContextType(3, 2, PROFILE_CORE)` | Core | 3.2 Core |
| `KHR-GL33` | `ContextType(3, 3, PROFILE_CORE)` | Core | 3.3 Core |
| `KHR-GL40` | `ContextType(4, 0, PROFILE_CORE)` | Core | 4.0 Core |
| `KHR-GL41` | `ContextType(4, 1, PROFILE_CORE)` | Core | 4.1 Core |
| `KHR-GL42` | `ContextType(4, 2, PROFILE_CORE)` | Core | 4.2 Core |
| **`KHR-COMPAT-GL42`** | `ContextType(4, 2, PROFILE_COMPATIBILITY)` | **Compatibility** | 4.2 Compatibility |
| `KHR-GL43` | `ContextType(4, 3, PROFILE_CORE)` | Core | 4.3 Core |
| `KHR-GL44` | `ContextType(4, 4, PROFILE_CORE)` | Core | 4.4 Core |
| `KHR-GL45` | `ContextType(4, 5, PROFILE_CORE)` | Core | 4.5 Core |
| `KHR-GL46` | `ContextType(4, 6, PROFILE_CORE)` | Core | 4.6 Core |
| `KHR-Single-GL43` … `KHR-Single-GL46` | context **single config** | Core | 4.3–4.6 Core |
| `KHR-NoContext` | không tạo context | — | — |
| `CTS-Configs` | liệt kê config | — | — |

> **Hệ quả quan trọng:** CTS **không** có package nào cho **Compatibility
> Profile** ngoài `KHR-COMPAT-GL42`, và **không** có package nào cho
> GL 1.0–2.1. ⇒ **Phần lớn bề mặt legacy mà TGLMT phải hỗ trợ (L01–L18,
> L20) không thể được CTS kiểm chứng.** Bắt buộc phải tự viết test theo spec
> (§5).

> **GLES không thuộc phạm vi** (A5 trong `AGENTS.md`): các package
> `KHR-GLES2/3/31/32`, `KHR-GLESEXT`, `dEQP-GLES*` **không** dùng. Chỉ dùng
> `KHR-GL*`, `KHR-COMPAT-GL42`, `KHR-Single-GL*`, `KHR-NoContext`.

### 3.2 Các nhóm test trong từng package

Đọc từ `gl3cTestPackages.cpp` / `gl4cTestPackages.cpp` (`init()` của mỗi
package). Số nhóm tính bằng đếm `addChild(new <Group>(...)`).

| Package | # nhóm | Nhóm (tên lớp) |
|---|---:|---|
| `KHR-GL30` | 18 | `InfoTests`, `ClipDistance::Tests`, `GLSLnoperspectiveTests`, `TransformFeedback::Tests`, `TransformFeedback3Tests`, `TextureRepeatModeTests`, `TextureLodBasicTests`, `GL30ShaderTests`, `TextureShadowLodTest`, `FramebufferBlitTests`, `TextureLodBiasTests`, `BufferObjectsTests`, `TransformFeedbackTests`, `ApiCoverageTests`, `GetUniformTests`, `FragCoordConventionsTests`, `TextureStorageTests`, `DrawBuffersTests` |
| `KHR-GL31` | 3 | `CommonBugsTests`, `TextureSizePromotion::Tests`, `PrimitiveRestartTests` |
| `KHR-GL32` | 4 | `GPUShader5Tests`, `TransformFeedbackOverflowQueryTests`, `PackedPixelsTests`, `PackedDepthStencilTests` |
| `KHR-GL33` | 7 | `GL33ShaderTests`, `PipelineStatisticsQueryTests`, `CullDistance::Tests`, `TextureSwizzleTests`, `NearestEdgeCases`, `PixelStorageModesTests`, `TextureFilterMinmax` |
| `KHR-GL40` | 10 | `DrawElementsBaseVertexTests`, `InternalformatTests`, `GPUShaderFP64Tests`, `TextureGatherTests`, `DrawIndirectTestsGL40`, `ClipControlTests`, `ShaderSubroutineTests`, `TextureBarrierTests`, `ExposedExtensionsTests`, `FragmentShadingRateTests` |
| `KHR-GL41` | 2 | `VertexAttrib64BitTests`, `ViewportArrayTests` |
| `KHR-GL42` | 7 | `MapBufferAlignmentTests`, `ShaderAtomicCountersTests`, `ShaderImageLoadStoreTests`, `ShadingLanguage420PackTests`, `TextureViewTests`, `BindImageTextureTests`, `GL42ShaderTests` |
| `KHR-COMPAT-GL42` | 1 | `GL42CompatShaderTests` |
| `KHR-GL43` | 21 | `CopyImageTests`, `DrawIndirectTestsGL43`, `ProgramInterfaceQueryTests`, `ComputeShaderTests`, `ShaderStorageBufferObjectTests`, `VertexAttribBindingTests`, `ShaderImageSizeTests`, `ExplicitUniformLocationGLTests`, `BlendEquationAdvancedTests`, `ShaderBitfieldOperationTests`, `StencilTexturingTests`, `SparseBufferTests`, `SparseTextureTests`, `IndirectParametersTests`, `ShaderBallotTests`, `ShaderConstExprTests`, `ClearTextureImageTestCases`, `AggressiveShaderOptimizationsTests`, `TextureQueryLevelsTests`, `TextureQueryLodTests`, `TextureBufferTests` |
| `KHR-GL44` | 18 | `GL44ShaderTests`, `BufferStorageTests`, `LayoutBindingTests`, `MultiBindTests`, `SeparateShaderObjsTests`, `SampleShadingTests`, `SampleVariablesTests`, `ShaderMultisampleInterpolationTests`, `ShaderTextureImageSamplesTests`, `TextureFilterAnisotropicTests`, `TextureStencil8Tests`, `GeometryShaderTests`, `GPUShader5Tests`, `TessellationShaderTests`, `TextureCubeMapArrayTests`, `TextureBorderClampTests`, `TextureBufferTests`, `ContextFlushControl::Tests` |
| `KHR-GL45` | 24 | `GL45ShaderTests`, `ClipControlTests`, `DirectStateAccess::Tests`, `GetTextureSubImage::Tests`, `TextureBarrierTests`, `ConditionalRenderInverted::Tests`, `Sync::Tests`, `IncompleteTextureAccess::Tests`, `ParallelShaderCompileTests`, `PostDepthCoverage`, `SparseTexture2Tests`, `SparseTextureClampTests`, `ShaderAtomicCounterOps`, `ShaderDrawParametersTests`, `ShaderViewportLayerArray`, `LimitsTests`, `ShaderGroupVote`, `PolygonOffsetClamp`, `SeparableProgramsTransformFeedbackTests`, `LayoutLocationTests`, `SpirvExtensionsTests`, `GlSpirvTests`, `es31compatibility::Tests`, `NegativeTextureLookupFunctionsBiasTests` |
| `KHR-GL46` | 0 mới | `KHR-GL46` kế thừa nguyên `KHR-GL45` (`GL46TestPackage::init()` chỉ gọi `GL45TestPackage::init()`) |

> **Lưu ý:** `KHR-GL46` **không** thêm nhóm mới ⇒ độ phủ CTS cho GL 4.6 =
> độ phủ CTS cho GL 4.5.

### 3.3 GL 1.0 – 2.1: **không có CTS** ⇒ phải tự viết test theo spec

Bằng chứng (không suy đoán):

* Registry của CTS (`glcTestPackageRegistry.cpp`) **không** đăng ký package
  nào dưới `KHR-GL30`.
* Module `external/openglcts/modules/gl` ở commit tham chiếu chỉ chứa file
  `gl3c*` và `gl4c*` (GL 3.0+); **không** có module `dEQP-GL` legacy.
* Không có `KHR-GL20`, `KHR-GL21`, `KHR-GL10`, …

⇒ Theo §9.8 đặc tả: *"GL 1.x không có CTS tương ứng. Dùng Minecraft, game
OpenGL thật, ví dụ Red Book, và test do AI viết theo spec. Không phụ thuộc
Mesa/Piglit."*

**Kế hoạch thay thế cho GL 1.0–2.1 (và cho phần legacy của Compatibility):**

| Nguồn tham chiếu | Dùng cho | Trạng thái |
|---|---|---|
| **Red Book** (OpenGL Programming Guide) | Ngữ nghĩa fixed-function, immediate mode, matrix stack | Chưa vendor vào repo (`MISSING`) |
| **Minecraft (LWJGL)** | Chứng minh ứng dụng OpenGL **thật** chạy được | Đã có corpus: `tests/corpus/mc-26.1.2-shaders/` (3 file) + `tests/unit/test_mc_*.cpp` |
| **Test do AI viết theo spec** | Chủ yếu nguồn kiểm chứng cho GL 1.x | `tests/unit/test_legacy_matrix.cpp`, `test_legacy_immediate.cpp` (đã có) |
| **Không dùng** Mesa / Piglit / Zink / MoltenVK / ANGLE | A1 (dịch thẳng) + A9 (không cần Mesa) | — |

---

## 4. Shim context tối thiểu để chạy CTS

### 4.1 Vì sao cần shim

CTS không tự biết "Metal". Nó cần một **platform layer** để:

1. tạo context GL với (version, profile, flags) xác định,
2. làm context đó current cho một thread,
3. resolve entry point (`eglGetProcAddress` hoặc GLX),
4. cấp một **surface/pbuffer offscreen** để render.

TGLMT **không** có EGL/GLX. Bằng chứng: `src/core/Context.cpp:45` ghi rõ
*"TGLMT không có EGL"*; `include/tglmt/gl_context_api.h` chỉ cung cấp API
context của riêng TGLMT (không có lớp platform nào khác).

### 4.2 Danh sách API tối thiểu của shim

| API | Loại | TGLMT có sẵn thứ tương đương? | Vị trí |
|---|---|---|---|
| `eglGetDisplay` / `eglInitialize` / `eglTerminate` | lifecycle | ❌ phải viết | — |
| `eglChooseConfig` | config | ❌ phải viết (trả 1 config tổng hợp) | — |
| `eglCreateContext` (+ `EGL_CONTEXT_MAJOR_VERSION`, `EGL_CONTEXT_MINOR_VERSION`, `EGL_CONTEXT_OPENGL_PROFILE_MASK`, `EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE`, `EGL_CONTEXT_OPENGL_DEBUG`) | context | ✅ có `tglmtCreateContextAttribs` — cần bọc | `include/tglmt/gl_context_api.h:52` |
| `eglMakeCurrent` | context | ✅ có `tglmtMakeCurrent` — cần bọc | `include/tglmt/gl_context_api.h:58` |
| `eglGetProcAddress` | symbol | ✅ có `tglmtGetProcAddress` — cần bọc (CTS yêu cầu trả `NULL` cho symbol không hợp lệ) | `include/tglmt/gl_context_api.h:66` |
| `eglCreatePbufferSurface` (hoặc dùng FBO) | surface | ⚠️ TGLMT render vào FBO / `CAMetalLayer`; CTS chấp nhận pbuffer offscreen ⇒ cần FBO shim | — |
| `eglDestroyContext` / `eglDestroySurface` | lifecycle | ❌ phải viết | — |
| `eglGetError` | error | ❌ phải viết | — |
| `eglBindAPI(EGL_OPENGL_API)` | API binding | ❌ phải viết (no-op) | — |

### 4.3 Yêu cầu về chính TGLMT (đã có / còn thiếu)

| Yêu cầu của CTS | TGLMT hiện tại | Bằng chứng |
|---|---|---|
| Báo cáo `GL_CONTEXT_PROFILE_MASK` đúng | ✅ có | `src/gl/gl_get.cpp` (pname gated) — test `test_context_version` PASS |
| Báo cáo `GL_CONTEXT_FLAGS` đúng | ✅ có | `src/gl/gl_get.cpp` — test `test_context_version` PASS |
| `glGetString(GL_VERSION)` đúng định dạng | ✅ `"4.6 (TGLMT Metal port, build …)"` | `src/gl/gl_error_string_debug.cpp:20-30` |
| `GL_VENDOR` / `GL_RENDERER` | ✅ `"TGLMT"` / `"Apple GPU (Metal)"` | `src/gl/gl_error_string_debug.cpp:12-13` |
| `GL_NUM_EXTENSIONS` khớp `glGetStringi` | ✅ | `tests/conformance/test_conformance_surface.cpp` (C3) |
| Từ chối attribs mâu thuẫn | ✅ trả `NULL` | `src/core/Context.cpp` (`SetVersionProfile`) |
| Context **đúng version** (không phải "4.6 giả") | ✅ gate theo `(version, profile)` | `src/gl/gl_version_gate_gen.cpp` (1048 entry) |
| **Offscreen FBO dùng được như pbuffer** | ⚠️ cần kiểm | `src/gl/gl_framebuffer.cpp` |
| **KHR-COMPAT-GL42**: profile Compatibility thật | ⚠️ version-gate có, nhưng **chưa có đường draw fixed-function** | `PROGRESS.md` (T1-S4) |
| `KHR-Single-GL4x`: một config duy nhất | ⚠️ cần cấu hình CTS | — |

### 4.4 Cấu trúc shim đề xuất (chưa implement)

```
tools/cts_shim/
├── CMakeLists.txt          # target `tglmt-cts-platform`
├── egl_shim.cpp            # bọc EGL tối thiểu (§4.2)
├── glu_platform.cpp        # subclass `glu::Platform` → trả ContextType → egl_shim
└── README.md
```

> Trạng thái: **`MISSING`**. Đây là kế hoạch, không phải bằng chứng.

---

## 5. Quy tắc báo cáo kết quả CTS

### 5.1 Bốn trạng thái, tách biệt tuyệt đối

| Trạng thái | Định nghĩa |
|---|---|
| **PASS** | Case chạy hết và CTS báo `PASS`. |
| **FAIL** | Case chạy và CTS báo `FAIL` (kèm log). |
| **SKIP** | CTS tự báo skip (vd: `NotSupported` do capability query) — **kèm lý do do CTS đưa ra**. |
| **NOT_RUN** | Case **chưa** được chạy (chưa có shim, chưa build, chưa có GPU, bị loại khỏi danh sách lọc). |

### 5.2 Luật bắt buộc

1. **Không được gộp `SKIP` và `NOT_RUN` vào `PASS`.** Báo cáo phải có 4 dòng
   riêng, tổng = tổng số case của các package đã chạy.
2. **Phải ghi rõ package nào đã chạy.** Không được viết "CTS: PASS" nếu chỉ
   chạy `KHR-GL46` — phải viết `KHR-GL46: 812/840 PASS` (ví dụ minh hoạ, **số
   thật phải đo**).
3. **Cấm tuyên bố "đã chạy đầy đủ CTS".** Cụm được phép là:
   *"đã chạy N/M case của package K, trên nền tảng P, nhãn môi trường E"* —
   và phải kèm danh sách package.
4. **Mỗi FAIL phải được phân loại nguyên nhân** (xem §5.3) và **không bị
   bỏ khỏi báo cáo**.
5. **Case bị loại bằng `--deqp-case` / waiver phải được liệt kê**, không được
   im lặng giấu.
6. **Mọi kết quả phải mang nhãn môi trường** (`AGENTS.md` §7). Chạy CTS trên
   Intel Mac ⇒ `OTHER_GPU`; **không** được dùng để tuyên bố `A11_DEVICE`.
7. **CTS là bằng chứng tuân thủ, không phải bằng chứng đúng tuyệt đối.** Một
   case FAIL có thể do lỗi CTS/waiver; phải đối chiếu spec trước khi kết
   luận `INCORRECT`.

### 5.3 Phân loại nguyên nhân FAIL (bắt buộc)

| Mã | Nguyên nhân | Hành động |
|---|---|---|
| `F-IMPL` | Implementation sai so với spec | Sửa implementation; trạng thái `INCORRECT` cho lát tương ứng |
| `F-MISSING` | Entry point / feature chưa có | `MISSING` |
| `F-STUB` | Còn stub `UNIMPLEMENTED` | `STUB` |
| `F-GATE` | Version/profile gate sai | Sửa `src/gl/gl_version_gate_gen.cpp` (sinh lại) |
| `F-SHIM` | Shim/platform layer (không phải lỗi TGLMT) | Sửa shim; **không** tính vào lỗi implementation |
| `F-HW` | Giới hạn phần cứng thật | `HARDWARE_LIMITED` — **phải kèm** capability query + các phương án emulation đã xét |
| `F-CTS` | Lỗi/waiver của CTS | Ghi nhận, không sửa TGLMT |
| `F-ENV` | Môi trường (thiếu GPU, timeout, hết bộ nhớ) | Ghi `SKIP` kèm lý do, **không** tính FAIL của implementation |

---

## 6. Trạng thái hiện tại của TGLMT với CTS (đo thật)

| Hạng mục | Trạng thái | Bằng chứng / lệnh |
|---|---|---|
| Mã nguồn CTS trong repo | `MISSING` | `find . -iname '*VK-GL-CTS*' -o -iname '*dEQP*'` ⇒ không có kết quả (§2.1) |
| Target CMake chạy CTS | `MISSING` | `grep -rniE "vk-gl-cts\|dEQP\|glcts" CMakeLists.txt tools/ scripts/` ⇒ không có kết quả |
| Platform layer (EGL/GLX) | `MISSING` | `src/core/Context.cpp` không có lớp platform; `include/tglmt/gl_context_api.h` chỉ có API context TGLMT |
| Shim CTS | `MISSING` | `tools/cts_shim/` chưa tồn tại |
| Số case CTS đã chạy | **0** | — |
| Số case CTS PASS | **0** | — |
| Số case CTS FAIL | **0** | — |
| Số case CTS SKIP | **0** | — |
| Số case CTS NOT_RUN | **toàn bộ** | — |
| Kết luận tuân thủ | **Không có** | Không được viết "TGLMT pass OpenGL CTS" |

### 6.1 Bằng chứng tuân thủ **khác** CTS đang có (đo được)

Các bằng chứng này **không** thay thế CTS. Chúng được liệt kê để báo cáo
không trống rỗng:

| Bằng chứng | Lệnh | Kết quả | Nhãn môi trường |
|---|---|---|---|
| Bề mặt API khớp registry | `python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict` | exit 0; core 657/657 có symbol, 0 thiếu; compat-only 42/42; 0 advertised no-op | `OTHER_GPU` |
| Ma trận extension khớp | `python3 tools/ext_matrix.py --lib build-work/libtglmt.dylib --strict` | exit 0; 246 glcore extension, 136 advertise, 0 missing | `OTHER_GPU` |
| Mọi symbol advertise dlsym được | `python3 tools/ext_coverage.py --dlsym build-work/libtglmt.dylib` | "136 advertised, 0 thiếu symbol dlsym" | `OTHER_GPU` |
| Bề mặt conformance (state + pname) | `ctest -R test_conformance_surface` | PASS | `REFERENCE_ONLY` (backend Null) |
| So sánh shader với `glslang` + `spirv-cross` | `ctest -R test_converter_proof` | PASS | `OTHER_GPU` (reference = `REFERENCE_ONLY`) |
| Toàn bộ suite CTest | `ctest -j8` | 188/188 PASS | `OTHER_GPU` |

Chi tiết: `reports/BASELINE_TEST_REPORT.md`.

---

## 7. Kế hoạch thực thi theo nhóm

Thứ tự chọn theo **giá trị bằng chứng / công sức**: mỗi nhóm là một lát
(version × subsystem), theo `docs/TESTING_STRATEGY.md` §2.3.

### G0 — Nền tảng (bắt buộc trước mọi nhóm khác)

| Việc | Công sức ước lượng | Phụ thuộc |
|---|---|---|
| Vendor + pin `VK-GL-CTS` (commit cụ thể, giữ nguyên file) | 0.5 ngày | mạng |
| Build CTS (`glcts` + `executor`) với `TGLMT_BUILD_SHARED=ON` | 0.5–1 ngày | G0.1 |
| `tools/cts_shim/` — EGL tối thiểu + `glu::Platform` | 3–5 ngày | G0.2 |
| `eglCreatePbufferSurface` → FBO shim, đọc pixel qua `glReadPixels` | 1–2 ngày | G0.3 |
| `eglGetProcAddress` → `tglmtGetProcAddress` + kiểm tra `NULL` đúng contract | 0.5 ngày | G0.3 |
| Harness: chạy 1 package, parse log CTS, sinh báo cáo 4 trạng thái | 1–2 ngày | G0.3–G0.5 |

**Tổng G0 ≈ 6–11 ngày công.**

### G1 — Core Profile hiện đại (giá trị cao nhất)

| Nhóm CTS | Bản đồ subsystem TGLMT | Kỳ vọng sơ bộ | Công sức |
|---|---|---|---|
| `KHR-GL46` (kế thừa GL45, 24 nhóm) | M01–M19 + M03 (shader) | Nhiều FAIL ở state/error/shading | 5–10 ngày sửa lỗi |
| `KHR-GL45` | DSA (`gl4cDirectStateAccess*`), sparse, sync, limits | Fail ở DSA + sparse | 5–10 ngày |
| `KHR-GL44` | GS, tess, sample shading, multi-bind, separate shader obj | **GS phải emulation** (A6) | 8–15 ngày |
| `KHR-GL43` | compute, SSBO, copy image, sparse, indirect params | Compute + SSBO | 5–8 ngày |
| `KHR-GL42` | atomic counters, image load/store, texture view, 420 pack | GLSL 4.20 | 4–8 ngày |
| `KHR-GL41` | vertex attrib 64-bit, viewport array | | 1–2 ngày |
| `KHR-GL40` | FP64 shader, texture gather, clip control, fragment shading rate | **FP64**: Metal hỗ trợ `f64`? cần đo | 3–6 ngày |
| `KHR-GL30`–`KHR-GL33` | buffer objects, XFB, packed pixels/stencil, texture swizzle | | 3–6 ngày |

**Tổng G1 (chạy + sửa) ≈ 34–65 ngày công.**

### G2 — Compatibility Profile

| Nhóm CTS | Ghi chú | Công sức |
|---|---|---|
| `KHR-COMPAT-GL42` (1 nhóm: `GL42CompatShaderTests`) | Package **duy nhất** kiểm tra Compatibility. Yêu cầu đường draw fixed-function (L01–L18) + `GL42CompatShaderTests` (GLSL 420 pack cho compat) | 10–20 ngày (kèm T1-S4) |

### G3 — GL 1.0–2.1 (không có CTS ⇒ tự viết theo spec)

| Việc | Công sức |
|---|---|
| Vendor **Red Book** (bản PDF hợp lệ) vào `docs/khronos/` để trích dẫn | 0.5 ngày (+ pháp lý) |
| Bộ test fixed-function theo 13 nhóm §9.3 đặc tả: immediate mode, matrix stack, lighting, texture env, fog, display list, evaluator, pixel draw, bitmap, client array, blend/depth legacy, selection/feedback, compat shader | 20–40 ngày |
| Chạy Minecraft (LWJGL) trên TGLMT ở version ≤ 2.1 và ghi log | 3–5 ngày |
| Kịch bản Red Book tối thiểu (chương 2–9) chạy được | 5–10 ngày |

**Tổng G3 ≈ 29–56 ngày công.**

### G4 — Single-config + NoContext + quản lý waiver

| Việc | Công sức |
|---|---|
| `KHR-Single-GL43..46` (1 config duy nhất) | 1–2 ngày |
| `KHR-NoContext` | 0.5 ngày |
| Sổ waiver của TGLMT (không được sửa CTS) + quy trình duyệt | 1–2 ngày |

### G5 — Đóng vòng lặp

| Việc | Công sức |
|---|---|
| CI job chạy CTS theo lịch (hàng đêm), lưu log + báo cáo 4 trạng thái vào `reports/` | 2–3 ngày |
| Phân loại FAIL tự động theo §5.3 | 2–3 ngày |

---

## 8. Rủi ro đã biết

| # | Rủi ro | Ảnh hưởng | Giảm thiểu |
|---|---|---|---|
| R1 | CTS yêu cầu `GL_ARB_...` mà TGLMT không advertise | Case bị skip hàng loạt | Kiểm tra `GL_EXTENSIONS` / `GL_NUM_EXTENSIONS` khớp `glGetStringi` (đã có) |
| R2 | `KHR-COMPAT-GL42` là package Compatibility **duy nhất** | Phần lớn bề mặt legacy **không** có CTS ⇒ bắt buộc G3 | Chấp nhận; bù bằng test theo spec |
| R3 | CTS chạy trên GPU Intel, TGLMT hướng tới A11 | Kết quả **không** khẳng định A11 | Gắn nhãn `OTHER_GPU`; chạy lại trên A11 trước khi tuyên bố |
| R4 | Sửa file CTS để "cho qua" | Mất toàn bộ giá trị bằng chứng | Tuyệt đối không sửa CTS; dùng waiver của **phía TGLMT** và ghi rõ |
| R5 | Case CTS rất nhiều, chạy lâu | CI chậm | Chạy theo package theo lịch; `--deqp-case` lọc theo lát |
| R6 | GLSL của TGLMT là **rewriter**, không phải compiler | Nhiều FAIL ở nhóm shader (`GL3xShaderTests`, `GL4xShaderTests`, `ShadingLanguage420PackTests`) | Cần bộ test GLSL riêng (`docs/GLSL_VERSION_MATRIX.md`) |
| R7 | CTS dùng **SPIR-V** (`GlSpirvTests`, `SpirvExtensionsTests`) | A1 cấm dùng glslang/SPIR-V của bên thứ ba | Số case này sẽ fail ⇒ ghi `F-IMPL`/`BLOCKED` với lý do kiến trúc, **không** sửa kiến trúc |

---

## 9. Checklist "được phép nói gì"

| Mệnh đề | Trạng thái hiện tại | Được phép? |
|---|---|---|
| "TGLMT đã chạy CTS" | 0 case | ❌ |
| "TGLMT pass OpenGL CTS" | — | ❌ |
| "TGLMT đạt tuân thủ OpenGL 4.6 Core" | — | ❌ |
| "TGLMT chưa chạy CTS; kế hoạch ở `docs/CONFORMANCE_TESTING.md`" | đúng | ✅ |
| "Bề mặt API của TGLMT khớp registry ở GL 4.6 (657 core / 1048 compat)" | đo được | ✅ (kèm lệnh) |
| "Nhóm CTS `KHR-GL46` chưa chạy" | đúng | ✅ |
| "GL 1.0–2.1 không có CTS; dùng test theo spec" | đúng | ✅ |

---

## 10. Tài liệu liên quan

| Tài liệu | Nội dung |
|---|---|
| `docs/TESTING_STRATEGY.md` | Chiến lược kiểm thử, ma trận version×profile×subsystem, metric so ảnh, nhãn môi trường, CI |
| `reports/BASELINE_TEST_REPORT.md` | Số đo baseline PHASE 4 (CTest, coverage 19×2, benchmark) |
| `docs/OPENGL_PROFILE_MATRIX.md` | Ranh giới profile, phân loại L01–L20, M01–M19 |
| `docs/OPENGL_API_COVERAGE.md` | Đo coverage bề mặt API hiện tại |
| `docs/OPENGL_VERSION_MATRIX.md` | Ma trận 19 version × 2 profile sinh từ registry |
| `docs/GLSL_VERSION_MATRIX.md` | 13 version GLSL + provenance |
| `docs/TRUTH_MANIFEST.md` | Join registry ⇄ symbol ⇄ implementation ⇄ tests |
| `reports/CONFORMANCE_REPORT.md` | Báo cáo tuân thủ phiên trước (cũ, cần cập nhật) |
