# LEGACY_DRAW_PATH — đường draw fixed-function (OpenGL Compatibility Profile)

> Tài liệu này mô tả **đường draw fixed-function** đã dựng trong TGLMT: một
> context Compatibility/Legacy **thực sự rasterize** hình học fixed-function
> trên Metal thay vì chỉ gom dữ liệu.
>
> Ngày: 2026-11-12 · Backend dùng để kiểm chứng: **Null** (không GPU) +
> **trình biên dịch Apple Metal 3.2** (`xcrun -sdk macosx metal`) để kiểm tra
> MSL sinh ra có hợp lệ hay không.

---

## 1. Vấn đề đã giải quyết

Trước đây `glEnd()` chỉ **ghép primitive** vào `Context::legacy.immediatePrimitives`
(`src/gl/gl_legacy_vertex.cpp:100 AssemblePrimitives`) — **KHÔNG có gì được
vẽ**. Đây chính là blocker B.1 / T1-S4 ghi trong `PROGRESS.md`.

Giờ: `glEnd()` → batch immediate-mode → **buffer đỉnh + index buffer** →
**chương trình MSL sinh ra (VS+FS)** → **đường draw dùng chung `AppleDrawGL`**
→ Metal.

---

## 2. Kiến trúc

Bám sát `docs/LEGACY_TO_METAL_MAPPING.md` §2 (state block / generated shader /
pipeline key / coexistence / uniform upload), §3 (immediate mode), §5 (lighting),
§6 (fog), §8 (texture env + texgen).

### 2.1 Chương trình fixed-function nằm NGOÀI `Context::programs`

| Thành phần | Vị trí | Ghi chú |
|---|---|---|
| Bản sao do Context sở hữu | `Context::legacyProgram` (`include/tglmt/Context.h:690`) | `ProgramObject` đầy đủ (MSL, uniform layout, `linkGen`) |
| Con trỏ từ state | `LegacyState::program` (`include/tglmt/LegacyState.h:312`) | trỏ vào `Context::legacyProgram` |

⇒ `glGetProgramiv` / `glUseProgram` **không** thấy chương trình này (nó không
nằm trong `Context::programs`) — đúng yêu cầu "legacy program không được xuất
hiện như GL program object".

**Kiểm chứng**: `tests/unit/test_legacy_draw.cpp` → `TestImmediateTriangle`
kiểm `c.programs.empty()` và `GL_CURRENT_PROGRAM == 0` sau khi vẽ.

### 2.2 PipelineKey — KHÔNG cần sửa

`Context::PipelineKey` (`include/tglmt/Context.h:1046`) đã có
`vsLib` / `fsLib` / `programGen`. Chương trình legacy có library riêng và
`linkGen` riêng ⇒ **tự tách khỏi mọi program modern** mà không phải sửa
`PipelineKey`.

`linkGen` tăng mỗi lần sinh lại chương trình (`src/gl/gl_legacy_draw.cpp:897`).

### 2.3 Dispatch: chỉ khi KHÔNG có program object trong use

`src/gl/gl_apple_draw.cpp:1830` (trong `AppleDrawGL`):

```cpp
if (gl::LegacySelectProgram(c, prog, prp)) { legacyActive = true; }
else { c.appleStats.noProgram++; return false; }
```

Quy tắc (GL 2.1 §2.15 / GL 4.6 Core §1.2.4 — "if a program object is in use,
the programmable pipeline runs; otherwise fixed-function is used"):

1. `program != 0` ⇒ **KHÔNG** dùng fixed-function.
2. Core Profile ⇒ **KHÔNG** có fixed-function (`LegacyFixedFunctionAllowed`,
   `src/gl/gl_legacy_draw.cpp:753`).
3. Draw **hiện tại** phải là draw fixed-function thật
   (`Context::legacy.drawActive` — batch immediate-mode do `LegacyDrawImmediate`
   phát). Một draw modern (`glDrawArrays`) **không có program** vẫn bị bỏ
   (`noProgram++`) như trước khi có đường legacy: vertex data của nó nằm ở
   vertex array của app, **không** nằm trong layout vertex legacy ⇒ nếu đi
   nhầm sẽ đọc buffer legacy *stale* ⇒ hình sai.

Quy tắc 3 là **sửa lỗi** so với bản đầu tiên của đường này (bản đó cho
`glDrawArrays` không có program đi qua chương trình legacy).

### 2.4 Layout vertex riêng

`include/tglmt/LegacyDraw.h:48` — `LegacyVertexLayout`: **6 × float4 = 96 byte**
(pos, normal, color, tex0, tex1, secColor), cố định ⇒ `PipelineKey::stride`
ổn định, không phân mảnh pipeline cache.

Binding: `src/gl/gl_apple_draw.cpp:2088` (dựng `cas[6]`) và
`src/gl/gl_apple_draw.cpp:2865` (bind `Context::legacy.vertexBuffer` vào
buffer index 0).

### 2.5 Uniform: đi qua ĐÚNG đường staging sẵn có

`LegacyUploadUniforms` (`src/gl/gl_legacy_draw.cpp:905`) đóng gói state vào
`ProgramObject::uniformLayout` + `ProgramObject::uniforms` — **không** nhân
bản cơ chế staging. Đường staging dùng chung: `src/gl/gl_apple_draw.cpp:2890-2946`.

**Bảng kích thước (CPU ↔ MSL phải khớp byte-for-byte)**:

| Stage | Nội dung | Số float4 | Byte |
|---|---|---|---|
| VS | `mvc[4]` `pjc[4]` `nrm[3]` `lights[56]` `matF[5]` `matB[5]` `sceneAmb` `flags` `flags2` | 80 | **1280** |
| FS | `fogParams` `fogColor` `fogCoord` `alphaTest` `envColor[2]` `combA[2]` `combB[2]` `combC[2]` `combD[2]` | 14 | **224** |

(`lights[56]` = 8 nguồn × 7 float4; `comb*` = 2 texture unit.)

Kiểm chứng: `TestUniformBufferSizes` kiểm `vsUBSize == 1280` và
`fsUBSize == 224`.

### 2.6 "Hình dạng" chương trình vs "giá trị" state

`LegacyProgramShape` (`include/tglmt/LegacyDraw.h:60`) = **những gì được BAKED
vào MSL**: lighting bật/tắt, fog, alpha test, separate-specular, two-sided,
normalize, rescale-normal, shade model, số texture unit, mode texture env,
hàm combiner, quy tắc base-format, mask `glColorMaterial`.

`LegacyComputeShape` (`src/gl/gl_legacy_draw.cpp:776`) tính **shape key** (FNV-1a).
`LegacyEnsureProgram` (`src/gl/gl_legacy_draw.cpp:845`):

* shape **không đổi** ⇒ chỉ nạp lại **giá trị** (uniform) ⇒ **không** sinh lại
  chương trình ⇒ **không** phá pipeline cache (đổi màu đèn không làm hỏng
  pipeline).
* shape **đổi** ⇒ sinh MSL mới ⇒ `linkGen++` ⇒ `PipelineKey::programGen` mới ⇒
  pipeline mới (không trả nhầm pipeline cũ).

**Hệ quả về hiệu năng**: đổi *hình dạng* (bật/tắt lighting, đổi mode texture
env, đổi số texture unit) ⇒ **biên dịch lại shader**. Đổi *giá trị* (màu đèn,
vật liệu, tham số fog, ma trận) ⇒ **chỉ upload uniform**, rẻ.

---

## 3. Shader sinh ra

Sinh **MSL trực tiếp** (KHÔNG đi qua `ConvertGLSLtoMSL`). Lý do: converter viết
cho GLSL người dùng (uniform block, varying, sampler GLSL) trong khi chương
trình fixed-function cần (a) **hằng số biên dịch** (mode texture env, hàm
combiner, mask `glColorMaterial`, `[[flat]]`) và (b) **layout uniform struct
float4 thuần** để đóng gói CPU khớp 1-1 với byte MSL. Sinh MSL trực tiếp ngắn
hơn và không rủi ro bị converter bóp méo.

### 3.1 Vertex stage — `src/gl/gl_legacy_draw.cpp:409`

| Phép | Nguồn |
|---|---|
| `eye = modelview · pos` (4 cột của ma trận) | GL 2.1 §2.11 |
| `clip = projection · eye`, `z' = 0.5·z + 0.5·w` (GL clip z → Metal NDC z) | đường modern (`glsl_to_msl.cpp kZConv`) |
| `n_eye = transpose(inverse(modelview3x3)) · n` (ma trận normal, **hàng**) | GL 2.1 §2.11.3 |
| `GL_NORMALIZE` / `GL_RESCALE_NORMAL` | §2.11.3 |
| `glColorMaterial` (mask 4 bit, front/back riêng) | §2.14.1 |
| Ánh sáng **per-vertex** | §2.14.1, `docs/LEGACY_LIGHTING_SPEC.md` §5.2 |
| Toạ độ fog (eye-space) | §3.10 |

Công thức ánh sáng **bám sát** `docs/LEGACY_LIGHTING_SPEC.md` §5.2:

```
c_pri = e_cm + a_cm·a_cs + Σ (att_i·spot_i)[ a_cm·a_cli + (n⊙→V P_pli)·d_cm·d_cli ]
                                                    + f_i·(n⊙ĥ_i)^s_rm·s_cm·s_cli   (SINGLE_COLOR)
c_sec = Σ (att_i·spot_i)·f_i·(n⊙ĥ_i)^s_rm·s_cm·s_cli                                (SEPARATE_SPECULAR)
```

với các điểm dễ sai đã được xử lý đúng:

* `f_i` = 1 **chỉ khi** clamped diffuse dot ≠ 0 (specular bị chặn khi mặt quay
  ngược nguồn sáng);
* `shininess == 0` ⇒ specular phẳng `f_i`, **không** phải 0;
* cutoff của spotlight **kiểm tra trước**, số mũ **sau** (và số mũ áp lên giá
  trị đã clamp ≥ 0);
* `spot_i = 1` khi `c_rli == 180` (bỏ hẳn kiểm tra cone);
* nguồn sáng hướng (`w == 0`) **không** bị suy giảm (`att = 1`);
* alpha của màu primary = alpha của **diffuse hiệu lực** (sau `glColorMaterial`);
* hai mặt: mặt sau dùng normal đảo dấu (`-n`) + vật liệu sau.

### 3.2 Fragment stage — `src/gl/gl_legacy_draw.cpp:503`

| Phép | Nguồn |
|---|---|
| Chọn mặt (`front_facing`) trước texture env | §2.14.1 / §3.8.16 |
| Texture environment: `REPLACE` `MODULATE` `DECAL` `BLEND` `ADD` | `docs/LEGACY_TEXENV_FOG_SPEC.md` §A.2 |
| Texture environment `GL_COMBINE` (RGB + Alpha, `RGB_SCALE`/`ALPHA_SCALE`, operand, DOT3) | §A.3 |
| Ánh xạ texel → (Cs, As) theo base format (RGBA/RGB/LUMINANCE/…/RED/RG) | GL 1.3 Table 3.20 |
| Cộng màu primary + secondary | GL 4.2C §16.3 |
| Fog **per-fragment** (3 mode) | §3.10 |
| Alpha test (`discard_fragment`) | §3.12 |

Quy tắc đã sửa trong lần rà soát này:

* `Cf` (màu primary đi vào texture env) = màu **đã chọn mặt** (front/back).
  Bản đầu dùng `in.color` ⇒ **mất màu mặt sau** khi bật two-sided + texture.
* `GL_DECAL` chỉ định nghĩa cho base format **RGB/RGBA**; base format khác ⇒
  *undefined* ⇒ quy tắc **D2** của `docs/LEGACY_TEXENV_FOG_SPEC.md`: giữ nguyên
  màu đến (no-op).
* Fog `GL_LINEAR` với `start == end` ⇒ `f = 0` (quy tắc **D6**).
* **Mặc định của `GL_COMBINE`** theo spec GL 1.3 §3.8.12 (p.147): hàm combiner
  RGB/Alpha = `MODULATE`; `SRC0_*` = `TEXTURE`, `SRC1_*` = `PREVIOUS`,
  `SRC2_*` = `CONSTANT`; `OPERAND0/1_RGB` = `SRC_COLOR`, `OPERAND2_RGB` =
  `SRC_ALPHA`, `OPERAND*_ALPHA` = `SRC_ALPHA`. State khởi tạo trong
  `LegacyState.h` lưu *pname* (`0x8580`/`0x8588` cho cả 3 slot RGB/Alpha,
  `0x8590`/`0x8592`/`0x8598`…) chứ không phải giá trị ⇒ **đường fixed-function
  map về mặc định của spec** (`MapSource`/`MapOperand`/`MapCombineFunc`,
  `src/gl/gl_legacy_draw.cpp:112-178`), **không** sửa state gốc.

### 3.3 Shade model

`GL_FLAT` ⇒ các varying màu dùng `[[flat, user(locnN)]]` ⇒ Metal lấy giá trị từ
đỉnh provoking (Metal: đỉnh **cuối** của primitive — khớp provoking vertex mặc
định của GL 3.2+ compatibility). BAKED vào shape ⇒ đổi shade model ⇒ sinh lại
chương trình.

---

## 4. Vertex data

`LegacyBuildGeometry` (`src/gl/gl_legacy_draw.cpp:1024`):

1. `immediateVertices` → buffer 6 × float4 (96 byte/đỉnh).
2. **Texture matrix** áp sau texgen (GL 2.1 §2.11.2), tính trên CPU.
3. **TexGen** (`GL_OBJECT_LINEAR`, `GL_EYE_LINEAR`, `GL_SPHERE_MAP`,
   `GL_REFLECTION_MAP`, `GL_NORMAL_MAP`) tính trên CPU, dùng ma trận normal và
   vị trí eye-space.
4. `immediatePrimitives` → index buffer `uint32`.
5. Upload: ring allocator (`Context::RingAlloc`, 0 alloc/frame khi còn chỗ) →
   fallback buffer tạm.

**Số đo thực tế** (backend Null, 1 tam giác):

```
vertexCount = 3, vertexBuffer length = 288 byte (3 × 96), vertexOffset = 0
indexCount  = 3, indexData = {0,1,2}
1 draw call: drawIndexed(Triangle, count = 3, instances = 1, indexed = true)
```

1 quad (`GL_QUADS`) ⇒ 4 đỉnh, 6 index (2 tam giác) ✓.

---

## 5. Khi nào draw fixed-function được phát ra

| Tình huống | Hành vi |
|---|---|
| `glEnd()` xong, **chưa** có draw nào khác | batch **được giữ**; chưa vẽ (xem §7 — follow-up bắt buộc) |
| draw kế tiếp (`glDrawArrays`, …) | batch được flush **trước** draw đó (`src/gl/gl_apple_draw.cpp:1790`) |
| `glClear` | batch được flush **trước** clear (`src/gl/gl_apple_draw.cpp:636`, `:640`) — đúng thứ tự GL (clear xoá hình vừa vẽ) |
| có program object trong use | batch **không** flush bằng đường legacy (GL 2.1 §2.15) |
| gọi `LegacyDrawImmediate` lần 2 với cùng batch | **không** vẽ 2 lần (cờ `drawnPrims`/`drawnVerts`) |

> **Lưu ý về thứ tự trong `DrawTrace` của backend Null**: `src/gl/gl_draw.cpp:186`
> ghi trace của lệnh draw **hiện tại** *trước* khi gọi `AppleDrawGL`, nên trong
> trace của Null, draw modern đứng **trước** draw fixed-function. Trên GPU thật
> thứ tự **NGƯỢC** (batch legacy được encode trước) — đúng thứ tự GL.

---

## 6. Bằng chứng

### 6.1 Test

`tests/unit/test_legacy_draw.cpp` — 21 nhóm kiểm tra, dùng `TGLMT_CHECK`
(không dùng `assert()`):

`TestProfileGate`, `TestImmediateTriangle`, `TestUniformsFollowState`,
`TestShapeRegeneration`, `TestQuadExpansion`, `TestProgramBoundDisablesLegacy`,
`TestAutoFlushOnNextDraw`, `TestErrorSemantics`, `TestTextureEnvAndTexGen`,
`TestTexGenObjectLinear`, `TestAlphaTestShape`, `TestModernDrawNotRoutedToLegacy`,
`TestTwoSidedWithTexture`, `TestDecalOnlyForRgbBaseFormat`,
`TestFogLinearStartEqualsEnd`, `TestPrimaryAlphaFromDiffuse`,
`TestFlatShading`, `TestUniformBufferSizes`, `TestFlushBeforeClear`,
`TestCombineDefaults`.

```
$ cd build-work && ctest -j8
...
100% tests passed out of 190
Total Test time (real) =   5.98 sec
```

`test_legacy_draw` (PASS, exit 0):

```
[TGLMT] symslide libtglmt NOT FOUND in 304 images
[TGLMT] glerr#1 code=0x502 <- _ZN5tglmt2gl5glEndEv <- glEnd <- _ZL18TestErrorSemanticsv <- main
test_legacy_draw: done
EXIT=0
```

Dòng `glerr#1 code=0x502` là **lỗi CỐ Ý** trong `TestErrorSemantics`
(`glEnd` ngoài `glBegin` ⇒ `GL_INVALID_OPERATION`), không phải lỗi của test.

### 6.2 MSL sinh ra **biên dịch thật** bằng trình biên dịch Apple Metal

6 "hình dạng" chương trình × 2 stage = 12 file MSL, biên dịch bằng
`xcrun -sdk macosx metal -c` (Apple Metal 3.2, macOS):

| Shape | VS | FS |
|---|---|---|
| plain (không lighting/fog/texture) | COMPILES | COMPILES |
| lighting + fog + alpha test | COMPILES | COMPILES |
| + texture (MODULATE) | COMPILES | COMPILES |
| + `GL_COMBINE` (ADD_SIGNED/INTERPOLATE + scale 2/4) | COMPILES | COMPILES |
| + DOT3 + separate-specular + two-sided + normalize | COMPILES | COMPILES |
| + `GL_FLAT` (`[[flat, user(locn0)]]`) | COMPILES | COMPILES |

⇒ **12/12 file MSL hợp lệ cú pháp Metal**. (Đây là kiểm tra *hợp lệ cú pháp*,
không phải kiểm tra *hình đúng*.)

### 6.3 Số đo định lượng (backend Null)

```
backend = null, isNull = 1
after glEnd: prims = 1, verts = 3, trace = 0        ← glEnd chỉ gom, chưa vẽ
LegacyDrawImmediate = 1
program ptr == &Context::legacyProgram              ← không phải GL program object
linkGen = 1, shapeKey = 0xdec0ba83ee780ac1
vsUBSize = 1280, fsUBSize = 224
vertexCount = 3, vertexBuffer len = 288, offset = 0
indexCount = 3, indexData = {0,1,2}
Context::programs.size() = 0                        ← legacy KHÔNG lọt vào programs
trace entries = 1
  trace[0]: prim = 3 (Triangle), count = 3, inst = 1, indexed = 1
```

---

## 7. Bảng trạng thái theo tính năng

Nhãn trạng thái: `FULLY_IMPLEMENTED` / `PARTIALLY_IMPLEMENTED` / `STUB` /
`MISSING` / `NOT_VERIFIED`.

**Nhãn môi trường: `NONE` cho mọi mục** (AGENTS.md §7). Lý do: kiểm chứng chỉ
chạy trên backend **Null** — một CPU shadow của API Metal, **không có GPU** và
**không có GL tham chiếu** để so pixel. Vì vậy:

* các mục kiểm chứng được *logic* (chương trình sinh ra, uniform đóng gói,
  vertex data, draw call, trace, ngữ nghĩa lỗi) ⇒ `FULLY_IMPLEMENTED` với
  môi trường `NONE`;
* các mục có phần *"hình"* (kết quả rasterize) ⇒ mã đã có nhưng ghi
  **NOT_VERIFIED (hình)** — xem §9.

Ngoài ra: **12/12 file MSL sinh ra biên dịch được bằng trình biên dịch Apple
Metal 3.2** (`xcrun -sdk macosx metal`) ⇒ MSL *hợp lệ cú pháp*, nhưng đó **không
phải** là đã chạy trên GPU ⇒ vẫn `NONE`.

### 7.1 Hạ tầng

| # | Tính năng | Trạng thái | Bằng chứng | Môi trường |
|---|---|---|---|---|
| 1 | Chương trình fixed-function sinh ra, nằm ngoài `Context::programs` | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:845`, `Context.h:690`; test `TestImmediateTriangle` | `NONE` |
| 2 | Tách pipeline bằng `PipelineKey` sẵn có (không sửa `PipelineKey`) | FULLY_IMPLEMENTED | `Context.h:1046`, `gl_apple_draw.cpp:2417` | `NONE` |
| 3 | Dispatch: fixed-function chỉ khi **không** có program trong use | FULLY_IMPLEMENTED | `gl_apple_draw.cpp:1830`, `gl_legacy_draw.cpp:759`; test `TestModernDrawNotRoutedToLegacy`, `TestProgramBoundDisablesLegacy` | `NONE` |
| 4 | Core Profile ⇒ không fixed-function | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:753`; test `TestProfileGate` | `NONE` |
| 5 | Buffer đỉnh + index buffer, layout 6×float4 cố định | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:1024`; test `TestImmediateTriangle`, `TestQuadExpansion` | `NONE` |
| 6 | Upload uniform qua đường staging dùng chung | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:905`; test `TestUniformsFollowState`, `TestUniformBufferSizes` | `NONE` |
| 7 | Đổi *giá trị* state ⇒ không sinh lại chương trình | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:845`; test `TestShapeRegeneration` | `NONE` |
| 8 | Đổi *hình dạng* ⇒ sinh lại chương trình (`linkGen++`) | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:897`; test `TestShapeRegeneration` | `NONE` |
| 9 | Flush batch trước draw kế tiếp / trước `glClear` | FULLY_IMPLEMENTED | `gl_apple_draw.cpp:1790`, `:636`; test `TestAutoFlushOnNextDraw`, `TestFlushBeforeClear` | `NONE` |
| 10 | `glEnd()` **tự** phát draw | **MISSING** | `src/gl/gl_legacy_vertex.cpp` ngoài write-scope — xem §8 | `NONE` |
| 11 | Draw fixed-function kiểu **vertex array** (`glVertexPointer` + `glDrawArrays`) | **MISSING** | chưa triển khai (nằm ngoài phạm vi task này) | `NONE` |

### 7.2 Vertex stage

| # | Tính năng | Trạng thái | Ghi chú | Môi trường |
|---|---|---|---|---|
| 12 | `modelview` × `projection` | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:409` | `NONE` |
| 13 | Ma trận normal (inverse-transpose 3×3) | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:235` (`NormalMatrixRows`) | `NONE` |
| 14 | `GL_NORMALIZE` / `GL_RESCALE_NORMAL` | FULLY_IMPLEMENTED | baked vào shape | `NONE` |
| 15 | `glColorMaterial` (mask front/back) | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:776` | `NONE` |
| 16 | Ánh sáng **per-vertex** (đủ công thức spec §5.2) | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | `gl_legacy_draw.cpp:331` (`LightingHelper`) | `NONE` |
| 17 | `GL_LIGHT_MODEL_TWO_SIDE` (mặt sau `-n` + vật liệu sau) | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | test `TestTwoSidedWithTexture` | `NONE` |
| 18 | `GL_LIGHT_MODEL_SEPARATE_SPECULAR_COLOR` | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | `flags.z` | `NONE` |
| 19 | `GL_LIGHT_MODEL_LOCAL_VIEWER` | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | `flags.x` | `NONE` |
| 20 | Texture matrix (`GL_TEXTURE`) | FULLY_IMPLEMENTED | `gl_legacy_draw.cpp:1106`; test `TestTextureEnvAndTexGen` | `NONE` |
| 21 | TexGen (OBJECT/EYE_LINEAR, SPHERE/REFLECTION/NORMAL_MAP) | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | `gl_legacy_draw.cpp:1051`; test `TestTexGenObjectLinear` | `NONE` |
| 22 | Shade model `GL_FLAT` | PARTIALLY_IMPLEMENTED | Mã + `[[flat]]` có; **chưa gọi được** vì `glShadeModel` dùng bảng enum sai (xem §8) | `NONE` |
| 23 | `GL_FOG_COORD` (toạ độ fog **per-vertex**) | PARTIALLY_IMPLEMENTED | Chỉ dùng giá trị `currentCoord` (uniform), không theo đỉnh — `glFogCoord` không được gom vào đỉnh (file ngoài scope) | `NONE` |

### 7.3 Fragment stage

| # | Tính năng | Trạng thái | Ghi chú | Môi trường |
|---|---|---|---|---|
| 24 | `GL_REPLACE` / `MODULATE` / `DECAL` / `BLEND` / `ADD` | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | `gl_legacy_draw.cpp:503`; test `TestTextureEnvAndTexGen`, `TestDecalOnlyForRgbBaseFormat`; `GL_REPLACE` **chưa gọi được** từ app (F2) | `NONE` |
| 25 | `GL_COMBINE` (RGB + Alpha, operand, scale) | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | test `TestTextureEnvAndTexGen`, `TestCombineDefaults`; mặc định map về mặc định spec GL 1.3 §3.8.12 | `NONE` |
| 26 | `GL_DOT3_RGB` / `GL_DOT3_RGBA` | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | theo `docs/LEGACY_TEXENV_FOG_SPEC.md` A.3.6; **chưa gọi được** từ app vì bảng enum sai (F2) | `NONE` |
| 27 | Quy tắc base format (texel → Cs/As) | FULLY_IMPLEMENTED (mã) | `SrcRuleOf` (`gl_legacy_draw.cpp:192`) | `NONE` |
| 28 | Cộng màu primary + secondary | FULLY_IMPLEMENTED (mã) | `gl_legacy_draw.cpp:633` | `NONE` |
| 29 | Fog `GL_LINEAR` / `GL_EXP` / `GL_EXP2` | FULLY_IMPLEMENTED (mã) / **NOT_VERIFIED** (hình) | test `TestFogLinearStartEqualsEnd` | `NONE` |
| 30 | Nguồn toạ độ fog `GL_FRAGMENT_DEPTH` (khoảng cách radial eye-space) | FULLY_IMPLEMENTED (mã) | `length(in.eyePos)` | `NONE` |
| 31 | Alpha test (`discard_fragment`) | PARTIALLY_IMPLEMENTED | Shader + uniform có; **`glAlphaFunc`/`glAlphaFuncRef` là STUB** ⇒ func/ref luôn là mặc định `GL_ALWAYS` (xem §8) | `NONE` |

---

## 8. Follow-up bắt buộc (file KHÔNG được đụng trong task này)

Các mục dưới đây **chưa** được sửa vì nằm ngoài write-scope. Mỗi mục ghi rõ
file / hàm / thay đổi cần làm.

| # | File | Hàm / vị trí | Cần làm |
|---|---|---|---|
| **F1** | `src/gl/gl_legacy_vertex.cpp` | `glEnd()` (dòng 89-95) | Thêm `gl::LegacyDrawImmediate(c);` **sau** `AssemblePrimitives(c.legacy);` (kèm `#include "tglmt/LegacyDraw.h"`). Không có lệnh này thì một app **chỉ** dùng immediate mode (không gọi `glDrawArrays`/`glClear` nào sau `glEnd`) **không bao giờ được vẽ**. Hiện batch chỉ được flush khi có draw kế tiếp hoặc `glClear` sau đó. |
| **F2** | `src/gl/gl_legacy_texenv.cpp` | bảng enum dòng 69-76 + `TexEnvValueValid` dòng 139-165 | Bảng enum SAI so với `docs/khronos/gl.xml` (nhóm "TextureEnvMode"): `GL_REPLACE` phải là **`0x1E01`** (đang `0x8001`), `GL_SUBTRACT` phải là **`0x84E7`** (đang `0x800A`), `GL_DOT3_RGB` phải là **`0x86AE`** (đang `0x8570` — trùng `GL_COMBINE`!), `GL_DOT3_RGBA` phải là **`0x86AF`** (đang `0x8E8F`). (Các hằng còn lại trong bảng đó ĐÚNG: `GL_MODULATE` 0x2100, `GL_ADD` 0x0104, `GL_BLEND` 0x0BE2, `GL_ADD_SIGNED` 0x8574, `GL_INTERPOLATE` 0x8575; các *pname* cũng đúng.) Hệ quả: `glTexEnvi(GL_TEXTURE_ENV_MODE, GL_REPLACE)` bị trả `GL_INVALID_ENUM`; `GL_COMBINE` và `GL_DOT3_RGB` trùng nhau trong bảng kiểm tra ⇒ `GL_DOT3_RGB` bị coi là hợp lệ vì `GL_COMBINE` hợp lệ, còn `0x86AE` thì bị từ chối. Đường fixed-function đã **chấp nhận cả hai bảng** (`MapEnvMode`/`MapCombineFunc`, `src/gl/gl_legacy_draw.cpp:112`) nên không vỡ, nhưng API phải sửa. |
| **F3** | `src/gl/gl_legacy_light.cpp` | `kSmooth`/`kFlat` dòng 56-57 | `GL_SMOOTH` phải là `0x1D01` (đang `0x8DDA`), `GL_FLAT` phải là `0x1D00` (đang `0x8DDB`). Hiện `glShadeModel(GL_FLAT)` bị trả `GL_INVALID_ENUM` ⇒ tính năng shade model không gọi được. |
| **F4** | `include/tglmt/LegacyState.h` | `LegacyState::shadeModel` dòng 298 | Mặc định phải là `GL_SMOOTH = 0x1D01` (đang `0x8DDA` — **không phải giá trị GL hợp lệ**). `src/gl/gl_get.cpp:265` trả thẳng giá trị này cho `GL_SHADE_MODEL` ⇒ `glGetIntegerv(GL_SHADE_MODEL)` hiện trả giá trị sai. (Sửa cùng F3.) |
| **F5** | `src/gl/gl_stubs_gen.cpp` (sinh từ `tools/gen_stubs.py`) + file implement mới | `glAlphaFunc` / `glAlphaFuncRef` | Đang là **STUB** (`LogDebug("UNIMPLEMENTED")`) ⇒ `GL_ALPHA_FUNC`/`GL_ALPHA_REF` không bao giờ được ghi ⇒ alpha test luôn là `GL_ALWAYS`. Cần implement (ghi vào `c.state.SetShadow(0x0BC1/0x0BC2, ...)`) rồi **regenerate** `src/gl/gl_stubs_gen.cpp` bằng `tools/gen_stubs.py` (tool ngoài write-scope). |

> Lưu ý: `src/gl/gl_stubs_gen.cpp` được sinh bằng `tools/gen_stubs.py`. Nếu
> implement `glAlphaFunc` trong một file mới (không phải `gl_stubs_gen.cpp`),
> **phải** regenerate lại file stub, nếu không sẽ có **duplicate symbol** khi
> link.

---

## 9. CHƯA xác minh (NOT_VERIFIED)

| Hạng mục | Vì sao chưa xác minh |
|---|---|
| **Hình ảnh thực tế trên Metal** (lighting, fog, texture env, alpha test đúng số hay không) | Backend trong `build-work` là **Null** (không GPU). Test chỉ kiểm được: chương trình sinh ra, uniform đóng gói, vertex data, draw call phát ra, trace, ngữ nghĩa lỗi. |
| **Chạy trên A11 (iPhone 8/X)** | Không có thiết bị trong môi trường build. Nhãn môi trường: `NONE`. |
| **Compile MSL trên toolchain iOS 16 / A11 (Metal 2.x)** | Đã verify bằng **Apple Metal 3.2 trên macOS** (`xcrun metal`) — 12/12 file hợp lệ. Chưa verify trên toolchain iOS cũ hơn. |
| **Hiệu năng** (bao lâu để sinh lại chương trình khi đổi hình dạng; chi phí uniform/frame) | Chưa đo trên GPU. |
| **Đường fixed-function kiểu vertex array** (`glVertexPointer`) | Chưa triển khai (mục 11). |

---

## 10. Danh sách file của đường draw này

| File | Loại | Nội dung | Sửa trong lần rà soát này? |
|---|---|---|---|
| `src/gl/gl_legacy_draw.cpp` | **MỚI** | toàn bộ đường fixed-function: sinh MSL, đóng gói uniform, dựng geometry, phát draw | **có** (sửa 5 lỗi + `GL_FLAT` + mặc định combiner) |
| `include/tglmt/LegacyDraw.h` | **MỚI** | khai báo entry point + shape + layout vertex | **có** (`flatShading`, `LegacySelectProgram`) |
| `src/gl/gl_apple_draw.cpp` | sửa | dispatch (`LegacySelectProgram`), layout vertex legacy, bind buffer legacy, flush trước draw/clear | **có** |
| `include/tglmt/Context.h` | sửa | `Context::legacyProgram` (bản sao `ProgramObject` do Context sở hữu) | không (đã có sẵn từ phần trước) |
| `include/tglmt/LegacyState.h` | sửa | state fixed-function + con trỏ `LegacyState::program` | không (đã có sẵn từ phần trước) |
| `tests/unit/test_legacy_draw.cpp` | **MỚI** | 21 nhóm kiểm tra | **có** (+9 nhóm mới) |
| `docs/LEGACY_DRAW_PATH.md` | **MỚI** | tài liệu này | **có** |

### 10.1 Danh sách lỗi đã sửa trong lần rà soát này

| # | Lỗi | Sửa tại |
|---|---|---|
| 1 | `glDrawArrays` không có program bị đi nhầm qua chương trình legacy (đọc buffer legacy *stale* với layout 6×float4 ⇒ hình sai) | `src/gl/gl_apple_draw.cpp:1830` + `LegacySelectProgram` (`src/gl/gl_legacy_draw.cpp:759`) |
| 2 | Two-sided lighting + texture: texture env nhận `in.color` (mặt trước) nên **mất màu mặt sau** | `src/gl/gl_legacy_draw.cpp:540` (`Cf = primary`) |
| 3 | `GL_DECAL` trên base format ≠ RGB/RGBA: áp công thức ler (undefined) thay vì no-op (quy tắc D2) | `src/gl/gl_legacy_draw.cpp:571` |
| 4 | Fog `GL_LINEAR` với `start == end`: trả 1.0 thay vì 0.0 (quy tắc D6) | `src/gl/gl_legacy_draw.cpp:643` |
| 5 | Alpha của màu primary lấy `matF[1].a` (diffuse gốc) thay vì diffuse **hiệu lực** (sau `glColorMaterial`) | `src/gl/gl_legacy_draw.cpp:477` (`matDf.a`) |
| 6 | Mặc định `GL_COMBINE` sai: hàm combiner mặc định là `GL_COMBINE`(0x8570) ⇒ bị map thành DOT3_RGB thay vì `MODULATE`; nguồn SRC1/SRC2 và operand RGB2 không theo mặc định spec | `src/gl/gl_legacy_draw.cpp:112-178` (`MapCombineFunc`/`MapSource`/`MapOperand`) |
| 7 | `glClear` xoá hình vừa vẽ do `glEnd` (batch chưa flush) | `src/gl/gl_apple_draw.cpp:632-641` |
