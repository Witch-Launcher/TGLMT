# TGLMT — Báo cáo sự thật (Phase 0 + contract tests)

**Trạng thái:** snapshot tĩnh sau Phase 1/2 contract; chưa phải kết luận conformance.
**Ngày:** 2026-11-02
**Commit:** `d546de4+dirty` (bao gồm thay đổi chưa commit)
**Môi trường đo:** macOS 15.7.7 (24G720), x86_64, build `build/`, `build/libtglmt.dylib`
**Phạm vi:** API surface, symbol export, advertised extensions, static source shape,
test suite hiện có. Chưa chạy CTS/Piglit/Mesa/device matrix.

---

## 1. Kết luận điều hành

TGLMT hiện **API/symbol-complete** nhưng **behavior-complete thì chưa**.

- 698/698 core function có symbol export; 0 missing.
- 133/133 advertised extension có đủ symbol `dlsym`; 0 missing.
- 246/246 `glcore` extension đủ symbol; 74 extension symbol-complete nhưng chưa advertise.
- **0 core function** còn thân no-op/constant không được spec cho phép (static scan).
- **383/698 core function** không có tham chiếu trực tiếp trong test hiện có.
- **80/133 advertised extension** không có tham chiếu test trực tiếp.
- Test suite hiện tại: **156/156 pass**; probe runtime **6 pass / 0 gap**.
- Tài liệu đang mâu thuẫn với code (xem §5). Không được dùng prose làm bằng chứng.

Kết luận thẳng: với dữ liệu hiện tại, **chưa đủ căn cứ để tuyên bố "hỗ trợ hoàn toàn OpenGL 4.6"**.
Cần Phase 2–5 (differential, CTS, device, report tự động) trước khi đưa ra tuyên bố đó.

---

## 2. Phương pháp và thang bằng chứng

| Mức | Ý nghĩa | Trạng thái hiện tại |
|---|---|---|
| **S1 Symbol** | `nm`/`dlsym` thấy symbol | Đạt cho 698 core + 133 advertised ext |
| **S2 Contract** | validate/error/state round-trip đúng | Một phần; chưa phủ hết |
| **S3 CPU correctness** | shadow/readback/CPU emulation đúng spec | Một phần; 383 function không có test trực tiếp |
| **S4 GPU correctness** | hiệu ứng GPU thật, pixel/buffer verify trên device | Chưa đo hệ thống |
| **S5 Conformance** | pass Khronos CTS/Piglit case | Chưa chạy |
| **S6 Performance** | đạt ngân sách frame trên thiết bị | Có vài perf test, chưa có gate |
| **X Rejected** | từ chối trung thực | Có ở một số đường (tess/GS), chưa audit hết |
| **M Missing** | thiếu symbol/hành vi | 0 symbol; behavior gap xem §4 |

Quy tắc đang áp dụng: `advertise ⇔ implement ⇔ test`. Phase 0 bổ sung điều kiện
**test phải là behavior test**, không chỉ là symbol/dlsym.

---

## 3. Baseline đo được

### 3.1 API surface

| Hạng mục | Số lượng |
|---|---:|
| Core functions (GL 4.6, `gl.xml`) | 698 |
| Exported `gl*` symbols | 1.855 (gồm alias) |
| Advertised extensions (`kExts`) | 133 |
| `glcore` extension rows | 246 |
| Advertised ext thiếu symbol | 0 |
| Core thiếu symbol | 0 |
| Core no-op/constant bodies | 7 |
| — trong đó static gap | **0** |
| — spec-allowed (hint/invalidate) | 7 |

### 3.2 Test suite

| Hạng mục | Kết quả |
|---|---|
| CTest configured | 156 |
| Pass trước Phase 0 | 100/102 |
| Pass sau Phase 0 | 102/102 |
| Pass sau Phase 1 | 103/103 |
| Pass sau Phase 2 (contract + stencil/MSAA depth/sample-mask/mip/legacy-mip/2D-array) | **134/134** |
| Pass sau Tess T4b stage 2i + XFB capture VS-only (compute prepass, interleaved/separate, pause/resume, instancing, VS-only + rasterizer discard, TF-primitives-written query) | **137/137** |
| Pass sau Tess quad + triangle N>3 + default levels + `gl_InstanceID` TCS/TES (quad 2 patch input CP 3/output 5; triangle output 5; TCS `gl_in.length()` = input CP; `glPatchParameterfv` default levels) | **140/140** |
| Pass sau Tess spacing/cw + MRT/MSAA/depth (trước clip/XFB array) | **143/143** |
| Pass sau XFB array element/mat2/mat2x3 reflection + overflow query + VS clip | **147/147** (`-j1` và `-j8`) |
| Pass sau TCS `gl_in` + `glDrawElementsBaseVertex` (interpolate control points) | **148/148** (`-j1` và `-j8`) |
| Pass sau XFB array-of-matrices (`mat2[2]` element + whole-array) | **149/149** (`-j1` và `-j8`) |
| Pass sau `ARB_base_instance` + `ARB_ES3_1_compatibility` (3 base-instance entry points + barrier-by-region) | **150/150** (`-j6`) |
| Pass sau `ARB_sample_locations` (default positions + programmable table + MSAA draw) | **152/152** (`-j6`) |
| Pass sau FBO completeness + sRGB MSAA target | **153/153** (`-j4`) |
| Pass sau FBO completeness + sRGB MSAA + 3D slice FBO | **154/154** (`-j4`) |
| Pass sau MSAA 2D array + `ARB_texture_storage_multisample` | **155/155** (`-j4`) |
| Pass sau `ARB_conservative_depth` | **156/156** (`-j4`) |
| Metal validation (host, `MTL_DEBUG_LAYER=1` + `MTL_SHADER_VALIDATION=1`, `ctest -j4`) | **156/156 pass** (2026-11-02; ~7.9s warm) |
| Device suite (host Apple Metal) | **73/73 pass**, gồm tess T4a/T4b + triangle N=5/default levels + quad + TCS/TES `gl_InstanceID` + atomic + FBO/MSAA/MRT + TCS gl_out/TES gl_in + instancing + XFB capture/array/matrix/overflow + VS `gl_ClipDistance[0]` |
| Device A11 (iPhone, iOS 16.7.16) | **73/73 pass**, 0 fail, 0 skip; log 2026-11-02 04:49, IPA SHA-256 `db8530380d4de84410fa20b91705cffde8a7f481ca931bcca8f7684eee3256a3`; gồm VS clip + XFB array/matrix/overflow |
| Thời gian full suite | ~5–20 giây (Metal shader cache đã ấm; cold chậm hơn) |

Hai lỗi baseline đã xử lý:

1. `test_mc_validate_apple` abort `Command encoder released without endEncoding`.
   Nguyên nhân: `Context` bị hủy khi còn `MTLRenderCommandEncoder` mở. Đã thêm
   `Context::~Context()` đóng encoder/commit/wait trước khi member bị hủy.
2. `test_helper_overloads` fail không ổn định khi `ctest -j8`.
   Nguyên nhân: dùng đường dẫn `/tmp/tglmt-overload-*` cố định. Đã đổi sang path
   theo PID, có dọn file.
3. Metal API/GPU validation sạch trên host: full suite **156/156** với
   `MTL_DEBUG_LAYER=1 MTL_SHADER_VALIDATION=1`. Đã sửa storage mode texture
   macOS (Managed; iOS Shared), `vertexDescriptor` cho GL TES, factor buffer
   `instanceStride=0`, texture usage `ShaderWrite`/`RenderTarget`, fallback
   array/3D, UBO `vec3` padding, và cache `depth` flag của tess pipeline.

### 3.3 Artifact mới

- `tests/truth/api_manifest.json`: manifest từng core function/extension.
- `docs/TRUTH_MANIFEST.md`: bản tóm tắt sinh tự động.
- `make audit`: gate symbol/dlsym/no-op/manifest (chưa gate behavior).
- `make truth`: sinh lại matrix + manifest.
- `make probe`: chạy `truth_probe` (runtime diagnostic cho gap; GAP không làm
  command fail). Kết quả hiện tại: **6 pass / 0 gap** (pipeline, frag data,
  compressed reject, MSAA 2D, DSA 1D, barrier).

Lưu ý: `impl_kind = real` chỉ có nghĩa "thân hàm không rỗng/không constant/không alias".
Nó **không** chứng minh đúng spec.

---

## 4. Static behavior gaps — trạng thái sau contract tests

Static scan hiện báo **0 static gap**: 7 hàm còn no-op/constant đều thuộc diện
hint/invalidate được spec cho phép. Các nhóm từng là gap đã có implementation +
test contract; giới hạn còn lại được ghi rõ thay vì ẩn.

### 4.1 Program pipeline / separate shader objects (core 4.1) — đã xử lý

- `glActiveShaderProgram`, `glUseProgramStages`, `glValidateProgramPipeline`,
  `glBindProgramPipeline` đã validate object/stage và lưu state thật.
- `glGetProgramPipelineiv`, `glGetProgramPipelineInfoLog` trả state/log thật.
- `glProgramParameteri(GL_PROGRAM_SEPARABLE)` đã sửa đúng enum `0x8258`.
- Draw path `BuildPipelineProgram` ghép VS+FS linked, remap FS uniform location/
  offset, và ưu tiên pipeline binding hơn `glUseProgram`.
- Test: `test_pipeline_sso`, probe `program pipeline state` PASS.
- Test GPU: `test_pipeline_draw_apple` (VS-only + FS-only separable, draw + readback).
- Giới hạn: pipeline draw mới chứng minh VS+FS (không GS/tess); chưa có test GPU
  riêng cho uniform/sampler merge. Tess có đường riêng T4a/T4b, chưa đi qua SSO.

### 4.2 Frag data location (core 3.0) — đã xử lý

- `glBindFragDataLocation`/`Indexed` lưu binding, validate program/color/index/name.
- Link áp dụng: `layout(location=)` thắng, sau đó binding, cuối cùng thứ tự khai báo;
  MSL được rewrite `[[color(N)]]`/`mrtN` sang location thật.
- `glGetFragDataLocation`/`glGetFragDataIndex` trả location/index thật.
- Test: `test_frag_bind`, `test_frag_bind_apple` (GPU attachment 1), probe PASS.

### 4.3 Compressed textures — đã chuyển sang reject trung thực

- `GL_NUM_COMPRESSED_TEXTURE_FORMATS = 0`; TGLMT chưa map ASTC/ETC2/BPTC.
- `glCompressedTexImage*` / `glCompressedTexSubImage*` / DSA variants →
  `GL_INVALID_ENUM`; `glGetCompressed*` → `GL_INVALID_OPERATION`.
- Có test `test_compressed_contract`; probe `compressed texture contract` PASS.
- Đây **không phải hỗ trợ nén**; chỉ là từ chối đúng spec thay vì no-op im lặng.
- 10 mục compressed đã ra khỏi danh sách static gap.

### 4.4 MSAA + stencil + sample mask (core 3.2+) — đã hỗ trợ 2D

- `glTexImage2DMultisample` + `glTextureStorage2DMultisample` tạo MSAA target
  thật (multisample color + resolve + **multisample depth**), draw path chọn
  target/pipeline `rasterSampleCount`; FBO có depth dùng depth đa mẫu riêng.
- `glRenderbufferStorageMultisample` hỗ trợ RGBA8/sRGB8A8, 1–4 sample.
- `glSampleCoverage`/`glSampleMaski` được **inject vào fragment shader** MSAA
  (`[[sample_id]]` + mask buffer, `discard_fragment()`), không còn state-only.
- Stencil: `glEnable(GL_STENCIL_TEST)`, front/back func/ref/mask/ops, write mask,
  clear stencil và FBO `DEPTH_STENCIL_ATTACHMENT` đã encode vào Metal
  depth-stencil state/render pass (`Depth24Stencil8` → `Depth32Float_Stencil8`).
  Metal chỉ có một stencil reference cho cả hai face; ref front/back khác nhau
  được xấp xỉ (ghi rõ trong code).
- Test: `test_msaa_gl_apple` (draw + resolve + RBO), `test_msaa_depth_apple`
  (MSAA depth che khuất), `test_sample_mask_apple` (mask 0 / all), `test_stencil_apple`
  (clear + ALWAYS/REPLACE + EQUAL pass/reject), `test_renderbuffer`,
  `test_sample_state`, probe MSAA PASS.
- MSAA 2D array (`glTexImage3DMultisample`, `glTextureStorage3DMultisample`) dùng `MTLTextureType2DMultisampleArray` + resolve 2DArray theo layer; `ARB_texture_storage_multisample` đã advertise. Không có MSAA 3D texture trong core GL.

### 4.5 Subroutine uniforms (core 4.0) — đã xử lý

- Parser reflection subroutine từ source; `glGetSubroutineIndex`,
  `glGetSubroutineUniformLocation`, `glGetActiveSubroutine*`,
  `glGetUniformSubroutineuiv` trả dữ liệu thật.
- `glUniformSubroutinesuiv` validate stage/count/type và ghi ordinal vào uniform.
- Test: `test_subroutine_api`; converter subroutine đã có `test_shader_subroutine`.

### 4.6 Texture 1D và barrier — đã xử lý trong Phase 1

- `glTextureSubImage1D` — đã route qua storage 2D (w × 1), giữ/khôi phục binding
  của active unit; có test `test_copy_1d`.
- `glTextureBarrier` — đã đóng encoder hiện tại (end + commitNoWait) trên Apple
  backend; Null backend no-op đúng vì không có hazard GPU; có call trong
  `test_mc_validate_apple`.
- Hai mục này **không còn** trong danh sách 13 static gap.

---

## 5. Tài liệu mâu thuẫn — phải sửa bằng dữ liệu test

| Nguồn | Nói gì | Thực tế/đối chiếu |
|---|---|---|
| `docs/en/compatibility.md` | Compute dispatch là "No-op accepted" | ĐÃ SỬA: `src/gl/gl_compute.cpp` dispatch thật; `test_compute_dispatch` pass trên Metal |
| `docs/en/compatibility.md` | Extension string cố định 6 mục | ĐÃ SỬA: `kExts[]` có 133 mục tại snapshot này |
| `docs/en/limits.md` | `glColorMask` no-op | ĐÃ SỬA: per-attachment write mask baked vào pipeline; `test_mrt_apple` xác nhận attachment 1 |
| `docs/en/limits.md` | Compute "✅ Works" | Khớp code; mâu thuẫn với compatibility.md ở trên đã được sửa |
| `docs/en/limits.md`, `docs/vi/limits.md` | Tessellation/GS "compile fails" chung một dòng; XFB "state machine only" | ĐÃ SỬA: tess triangle/quad chạy GPU; GS compile fail; XFB capture VS-only thật |
| `docs/en/compatibility.md` | Polygon offset "Trace only"; `glClearTex*`/`glInvalidate*` no-op | ĐÃ SỬA: polygon offset gọi `setDepthBias`; `glClearTex*` thật; `glInvalidateFramebuffer` partial |
| `docs/en/compatibility.md` | `sampler2DShadow` bị hạ thành texture thường | ĐÃ SỬA: converter emit `depth2d` + `sample_compare`; native compare sampler có `test_shadow_compare_apple` |
| `docs/en/compatibility.md` | `buffer { }` SSBO không nhận | ĐÃ SỬA: SSBO graphics parse/bind; `test_ssbo_graphics_apple` |
| `tools/ext_matrix.py` (trước Phase 0) | default `build-apple/libtglmt.dylib` | File không tồn tại → báo sai 114 "advertised-lie"; đã sửa auto-detect |
| `docs/HONESTLY_ABSENT.md` | Vertex/Fragment "✅ Full", gồm subroutine | Nay khớp hơn: subroutine reflection + selection đã có test; vẫn cần xác nhận GPU |

Các tài liệu khác cần audit tương tự: `docs/en/limits.md`, `docs/HONESTLY_ABSENT.md`,
`docs/EXTENSION_BEHAVIOR_AUDIT.md`. Nguyên tắc: prose chỉ được giữ nếu có test ID
hoặc số liệu sinh tự động tương ứng.

---

## 6. Các gap behavior khác đang được tài liệu/code ghi nhận (chưa verify Phase 0)

Các mục sau chưa được đưa vào manifest vì không phải no-op body, nhưng code/docs ghi
nhận là trace-only hoặc CPU approximation. Cần test S4 để xác nhận:

- Polygon offset: GPU qua `setDepthBias`; state shadow có test, pixel/depth test riêng còn thiếu.
- Logic op: shadow-only trên GL path; kernel compute-ROP native có `test_rop_apple` nhưng chưa nối vào `glDraw*`.
- Line width: shadow-only; Metal line luôn 1 pixel.
- Transform feedback: capture VS-only thật (compute prepass), gồm interleaved/separate, pause/resume, instancing, array/matrix, overflow query; GS/TES và fp64 chưa có.
- Geometry: compile fail trung thực tại `glCompileShader`; Core 4.6 yêu cầu.
- Tessellation: T4a (patch output→input) + T4b stage 1 đã chạy GPU:
  VS capture compute cho attributes/`gl_in`, TCS input match theo tên,
  indexed/`baseVertex`/`first`/`glBindVertexBuffer` offset, multi-patch,
  uniform/sampler/UBO/SSBO/image/atomic counter theo từng stage (VS/TCS/TES/FS),
  FBO color0+depth, FBO MSAA 4x + resolve, MRT 2 color attachment, TCS `gl_out`
  và TCS/TES `gl_in` (Position/PointSize), instancing tess (per-instance divisor,
  draw/capture theo `instanceCount`, `gl_InstanceID` trong VS), kèm texture 1×1 đỏ
  và buffer trắng pixel-proof. TES `gl_in` đọc TCS `gl_out` khi TCS ghi; nếu TCS
  không ghi thì fallback control-point từ VS prepass.
  Đã thêm **quads** (2 patch, input CP 3 → output CP 5), **triangle domain với
  output CP 5** (link re-convert TCS theo domain TES để chọn đúng
  `MTLTriangleTessellationFactorsHalf`), **runtime input/output patch count**
  (`gl_PatchVerticesIn`/`gl_in.length()` khác `layout(vertices=N)`), và
  **`glPatchParameterfv` default outer/inner levels** (TCS ghi một phần level;
  phần còn lại giữ default, `test_tess_quad_apple` + `test_tess_tri_cp_apple`),
  và **`gl_InstanceID` trong TCS/TES** (patch local `gl_PrimitiveID`, instances
  xếp theo `patchCount`; `test_tess_instance_id_apple` + device check).
  Thêm `glGetIntegerv(GL_MAX_PATCH_VERTICES)=32`, `GL_MAX_TESS_GEN_LEVEL` theo
  backend (16 trên A11, 64 trên Mac/Apple5+), validate `glPatchParameteri`.
  Đã thêm spacing (`equal`/`fractional_even`/`fractional_odd`), `layout(cw)` +
  cull tess, MRT+MSAA+depth, patch/index offset (kể cả `gl_in` +
  `glDrawElementsBaseVertex`; integration test interpolates control points), và `gl_ClipDistance[0]` cho VS/TES (FS nhận
  `[[clip_distance]]`). Còn thiếu: isolines, `point_mode`, factor >16 trên A11,
  `gl_CullDistance`, `gl_ClipDistance` index khác 0, và TCS `gl_ClipDistance`.
- Metal tess factor: Apple3/4 (A9–A11) tối đa **16**, Apple5+ (A12+) và Mac **64**.
  `makeTessPipeline` chọn theo `supportsFamily:Apple5`; A11 không còn abort
  `maxTessellationFactor(64) must be <= 16`. Device A11 đã PASS `tess drawPatches`
  qua API GL và device suite 73/73; T4a test dùng factor 1, factor >16 trên A11
  chưa được emulate/clamp nên chưa coi là full GL tess.
- `baseInstance != 0` đã encode `drawIndexedBaseInstance`/`drawPrimitivesBaseInstance`;
  `GL_PATCHES` chạy tess triangle/quad qua Metal `drawPatches`; indirect GPU path vẫn trace/CPU fallback.
- Mip level > 0: **đã hỗ trợ** qua `glTexStorage2D` mip chain, legacy
  `glTexImage2D`/`glTexSubImage2D` level>0 (tự nâng GPU lên mip chain khi có
  data), FBO attach level (texture view), `glGenerateMipmap`, và
  `glCopyImageSubData` level>0 (GPU blit view). `glTexImage2D` level>0 NULL
  (chỉ alloc) không nâng chain để tránh sampler mip rỗng.
- `glInvalidateFramebuffer`: partial thật (attachment → `contentUndefined` → `DontCare`); `glInvalidateTexImage`/`glInvalidateTexSubImage` vẫn no-op.
- `glClearTexImage` / `glClearTexSubImage`: thật trên GPU + shadow; test 2D/3D/cube (`test_clear_tex_3d`, `test_clear_tex_cube`, `test_enum_behavior`).
- `glCopyImageSubData`: level 0/level>0 2D và 3D nhiều slice (`blitCopySlices`,
  3D origin.z hoặc array slice) đã có GPU path; `glCopyTexSubImage2D` level>0
  đã wire (read source region → `updateTextureLevel`, test RGBA8 mip xác nhận).
- MSAA 2D + 2D array đã có (color/depth/resolve theo layer); MSAA depth + sample coverage/mask đã có. OpenGL không có target texture multisample 3D; `glTexImage3DMultisample`/`glTexStorage3DMultisample` chỉ nhận `GL_TEXTURE_2D_MULTISAMPLE_ARRAY`, nên "MSAA 3D" không phải blocker hợp lệ.
- Pipeline draw đã có GPU test VS+FS (`test_pipeline_draw_apple`); chưa có GPU test
  cho SSO uniform/sampler merge. Tess có GPU test riêng (`test_tess_draw_apple`),
  chưa chạy qua pipeline SSO.

Danh sách này là đầu vào cho Phase 1/2, không phải kết luận cuối.

---

## 7. Việc đã làm trong Phase 0

1. Sửa `tools/ext_matrix.py` và `tools/ext_coverage.py` để tự tìm library hiện tại,
   tránh false "advertised-lie".
2. Thêm `--strict` cho `ext_matrix.py`; `make audit` fail nếu missing symbol/no-op.
3. Thêm `tools/truth_manifest.py` + `tests/truth/api_manifest.json` +
   `docs/TRUTH_MANIFEST.md`.
4. Sửa teardown `Context` (Metal encoder) và flake test helper overload.
5. Chạy baseline 102 test: 102/102 pass.
6. Phase 1 bước đầu: sửa `glTextureSubImage1D` + `glTextureBarrier`; thêm test
   DSA 1D và barrier; full suite 102/102 pass.
7. Thêm `test_compressed_contract`: compressed upload/read reject đúng spec.
8. Thêm `tests/truth/probe_gaps.cpp` + target `truth_probe`/`make probe`.
9. Phase 2 contract: pipeline/SSO, frag data binding, subroutine, sample state,
   MSAA 2D storage/draw/resolve + RBO, stencil, MSAA depth, sample mask, mip
   storage/render/copy. Kết quả: **140/140 pass** (thêm `test_tess_draw_apple`, `test_tess_vs_capture_apple`, `test_xfb_capture_apple`, `test_tess_quad_apple`, `test_tess_tri_cp_apple`, `test_tess_instance_id_apple`; device suite host 66/66), manifest **0 static gap**,
   399 core chưa có test ref, probe **6 pass / 0 gap**.

---

## 8. Next phases (đề xuất thực thi tiếp)

### Phase 1 — Contract tests cho static gap (đã hoàn tất)
- Đã phủ pipeline/SSO, frag data, subroutine, sample state, compressed, MSAA 2D,
  stencil, MSAA depth, sample mask, mip storage/render/copy.
- Kết quả: 13/13 static gap cũ có implementation/test; 0 static gap còn lại.
- MSAA 2D array đã bổ sung (test_msaa_array_apple); XFB đã capture thật cho VS-only
  (interleaved/separate, pause/resume, instancing, no-FS + rasterizer discard,
  array element/whole-array, mat2/mat2x3 + mat2[2] column-major, TF PRIMITIVES_WRITTEN,
  TF_OVERFLOW/STREAM_OVERFLOW query); XFB với double/fp64 vẫn fail (Metal không
  có fp64), GS và XFB sau GS/TES chưa có; tess đạt subset T4a + T4b stage 2i
  (instancing) + spacing/cw/MRT/MSAA/depth/clip[0].

### Phase 2 — Differential với Mesa
- Harness command stream + PNG/buffer/state dump; chạy TGLMT (null + apple) vs Mesa
  llvmpipe 4.6 trong Docker.
- Bắt đầu với state/format/texture/FBO/draw; sau đó compute/SSBO/image.

### Phase 3 — Khronos CTS
- Port `VK-GL-CTS` (`DEQP_TARGET=null`) qua platform offscreen TGLMT.
- Chạy `KHR-GL46` mustpass trước; lưu case log + waiver.

### Phase 4 — Metal validation + device matrix
- `MTL_DEBUG_LAYER=1`, `MTL_SHADER_VALIDATION=1`, XCTrace/MTLCapture.
- Host macOS: **156/156 pass** với validation layer (2026-11-02); A11 chưa chạy
  lại validation trong phiên này vì device `unavailable`.
- A11 baseline + các đời sau; Minecraft/Sodium/Iris.

### Phase 5 — Report tự động
- Sinh report từ manifest + test results; mỗi claim có test ID/device/build.
- CI gate: advertised ext phải có behavior test; docs stale thì fail.

---

## 9. Lệnh tái lập

```sh
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build --output-on-failure -j8

make audit          # symbol/dlsym/no-op/manifest gates
make truth          # regenerate docs/EXTENSION_MATRIX.md + docs/TRUTH_MANIFEST.md
python3 tools/truth_manifest.py --write
```

---

## 10. Tuyên bố trung thực

- Báo cáo này là **baseline tĩnh**, không thay thế CTS/device test.
- "Không thấy gap" trong scan không có nghĩa là "đúng spec"; nó chỉ có nghĩa là
  scan chưa phát hiện thân no-op/constant.
- Không nên dùng `GL_VERSION = 4.6` như bằng chứng hỗ trợ đầy đủ cho đến khi
  các gap ở §4 và §6 được xử lý hoặc có waiver rõ ràng.
