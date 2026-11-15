# FIXED_FUNCTION_PIPELINE.md — kiến trúc fixed-function của TGLMT

Tài liệu kỹ thuật dùng để **implement** phần fixed-function (Compatibility
Profile) của TGLMT: OpenGL → Metal trên iOS / Apple A11.

**Nguồn spec chính:** OpenGL 2.1 Specification (`glspec21.pdf`, 01-Dec-2006) —
mục 2.6, 2.7, 2.10, 2.11, 2.13, 2.14, 3.8.13, 3.10, 5.4, 6.2.
Bản PDF **không** nằm trong repo; bản đối chiếu cục bộ là
`docs/khronos/glspec46.core.pdf` (Core Profile 4.6) — dùng để đối chiếu phần
**Core đã gỡ** (App. E.2) và phần state/enum.
Nguồn enum duy nhất trong repo: `docs/khronos/gl.xml` (Khronos OpenGL-Registry).

**Nhãn** dùng trong tài liệu:

| Nhãn | Nghĩa |
|---|---|
| **SPEC** | quy tắc theo đặc tả (kèm mục) |
| **CURRENT** | trạng thái thật của code trong repo lúc viết (kèm `file:line`) |
| **PLAN** | thiết kế đề xuất, chưa có trong code |

Trạng thái tính năng dùng đúng bộ trong `AGENTS.md` §6:
`FULLY_IMPLEMENTED`, `PARTIALLY_IMPLEMENTED`, `INCORRECT`, `STUB`, `MISSING`,
`EMULATED`, `HARDWARE_LIMITED`, `BLOCKED`, `NOT_VERIFIED`, `NOT APPLICABLE`.

**Môi trường đo:** tất cả số đo dưới đây chạy trên **Null backend** (CPU) của
TGLMT (`tglmtCreateContextAttribs(..., backend="null")`), build Debug
(`build-work/`). Nhãn môi trường = `NONE` (chưa chạy trên `A11_DEVICE`).
Mọi phép đo được sinh bằng probe C++ link vào `build-work/libtglmt.dylib`
(xem §6).

---

## 0. Phạm vi

| Nhóm | Mục | § |
|---|---|---|
| L03 | Matrix stack + transform | §2.1 |
| L01 | Current vertex attributes (immediate mode) | §2.2 |
| L04 | Lighting / material / shade model | §2.3 |
| L05 | Fog | §2.4 |
| L06 | Texture environment + texture generation | §2.5 |
| L13/L14/L16 | Rasterization legacy (alpha test, clip plane, stipple, smooth) | §2.6 |

Chi tiết immediate mode → `docs/IMMEDIATE_MODE.md`.
Chi tiết lighting + fog + texenv/texgen → `docs/LEGACY_TEXTURE_AND_LIGHTING.md`.
Chi tiết display lists → `docs/DISPLAY_LISTS.md`.

---

## 1. Mô hình state: `Context::legacy` vs `StateTracker`

### 1.1 Vì sao tách (SPEC của kiến trúc, đã chốt)

**CURRENT.** Quyết định **D-004** (`docs/DECISIONS.md:92-103`):

> State fixed-function (matrix stack, current vertex attributes, immediate mode)
> nằm trong `Context::legacy` (`include/tglmt/LegacyState.h`), **không** nhét vào
> `StateTracker`.

Lý do đã ghi: `StateTracker` là state machine của đường **programmable pipeline**
đang chạy production (Minecraft). Thêm state legacy vào đó là rủi ro hồi quy và
làm pipeline-key cache phải mang theo state không liên quan.

**Hệ quả bắt buộc (dùng để review code mới):**

1. Đường modern **không được** đọc `Context::legacy`. Nếu một draw path modern
   đọc `c.legacy.*` thì vi phạm D-004.
2. Đường legacy **được** đọc `Context::state` cho phần state *dùng chung* với
   core (blend, depth, cull, viewport, texture binding) — vì các phần đó đã
   bake thẳng vào Metal descriptor.
3. Mọi state **chỉ có ở Compatibility Profile** phải nằm trong `Context::legacy`
   hoặc trong `Context::ext` (extension state), **không** thêm vào `StateTracker`.

### 1.2 Cấu trúc hiện tại (CURRENT, đọc từ file)

```
include/tglmt/Context.h:643     StateTracker state;      ← đường modern (core)
include/tglmt/Context.h:681     ExtState ext;             ← state của extension
include/tglmt/Context.h:685     LegacyState legacy;      ← state fixed-function
```

`include/tglmt/LegacyState.h`:

| Thành phần | file:line | Ghi chú |
|---|---|---|
| `struct GLMatrix` (column-major, `m[0..3]` = cột 0) | `LegacyState.h:20-75` | đúng layout OpenGL |
| `GLMatrix::Mult` (this = this · other ⇒ bên phải biến đổi trước) | `LegacyState.h:40-50` | đúng `glMultMatrix` |
| `GLMatrix::LoadTranspose` / `MultTranspose` | `LegacyState.h:34-37`, `:51-56` | transpose khi nạp |
| `GLMatrix::Translate/Scale` | `LegacyState.h:57-64` | post-multiply |
| `GLMatrix::Rotate/Ortho/Frustum` (decl) | `LegacyState.h:66-68` | định nghĩa ở `src/gl/gl_legacy_matrix.cpp:25-78` |
| `kMatrixStackDepth = 32` | `LegacyState.h:79` | spec tối thiểu 32 cho modelview |
| `struct MatrixStack` (vector, `stack[0]` = phần tử đang dùng) | `LegacyState.h:81-103` | `Push()` → false khi đầy (GL_STACK_OVERFLOW), `Pop()` → false khi còn 1 (GL_STACK_UNDERFLOW) |
| `struct LegacyVertex` (pos/color/normal/texCoord/texCoord1/secondaryColor/index/edgeFlag) | `LegacyState.h:106-115` | 1 đỉnh đã "resolve" |
| `struct LegacyPrimitive` (mode + `idx[4]` + count) | `LegacyState.h:119-123` | triangulate sẵn |
| `struct LegacyState` | `LegacyState.h:126-163` | toàn bộ state fixed-function |
| `matrixMode` (mặc định `GL_MODELVIEW` 0x1700) | `LegacyState.h:128` | |
| 4 stack: `modelview, projection, texture, color` | `LegacyState.h:129` | **texture chỉ có 1 stack dù chung**, xem §2.1 |
| `currentColor[4]` = (1,1,1,1) | `LegacyState.h:132` | đúng spec |
| `currentNormal[3]` = (0,0,1) | `LegacyState.h:133` | đúng spec |
| `currentSecondaryColor[3]` = (0,0,0) | `LegacyState.h:134` | spec là (0,0,0,**1**) → xem §6 L-2 |
| `currentTexCoord[8][4]` | `LegacyState.h:136` | 8 unit; mặc định `q=1` (ctor `:156`) |
| `activeTextureUnit` | `LegacyState.h:138` | đồng bộ 2 chiều với `glActiveTexture` |
| `inBeginEnd`, `beginMode`, `edgeFlag`, `currentIndex` | `LegacyState.h:141-144` | |
| `immediateVertices`, `immediatePrimitives` | `LegacyState.h:147-149` | output của assembler |

**Đồng bộ 2 chiều `activeTextureUnit`:**

* `src/gl/gl_texture.cpp:596-604` — `glActiveTexture` (đường modern) ghi
  `c.legacy.activeTextureUnit = tex - 0x84C0`.
* `src/gl/gl_legacy_vertex.cpp:36-45` — `LegacyActiveTexture` (gọi nội bộ từ
  `glMultiTexCoord*`) ghi ngược lại `c.legacy.activeTextureUnit` **và** gọi
  `glActiveTexture` của đường modern.

**Gate theo profile (CURRENT).**

* Entry point fixed-function bị chặn ở Core Profile **ở lớp C ABI**:
  `src/gl/gl_c_exports.cpp:4680-4684` (`glBegin` → `tglmt::GateCheck(7)`),
  `:5240-5244` (`glLightf` → `GateCheck(465)`).
  Bảng gate: `src/gl/gl_version_gate_gen.cpp` (sinh bởi `tools/gen_version_gate.py`),
  ngữ nghĩa bit: `include/tglmt/VersionGate.h:38-65`.
* Pname fixed-function bị chặn ở Core Profile:
  `src/gl/gl_get.cpp:294-311` (`CompatOnlyPname`) → `GL_INVALID_ENUM`
  (đo: `glGetFloatv(GL_CURRENT_COLOR)` ở GL 4.6 Core ⇒ `0x0500` ✓).

---

## 2. Bảng trạng thái fixed-function đầy đủ

Cột **"mặc định theo spec"** lấy từ OpenGL 2.1 spec: bảng 2.10 (lighting),
bảng 3.10/3.20–3.25 (texenv), §3.10 (fog), bảng 6.10–6.19 (state),
§2.11.3 (normal), §2.11.4 (texgen).

Cột **"nơi lưu trong TGLMT"** dùng đường dẫn + số dòng thật.
Cột **"trạng thái"** theo bộ `AGENTS.md` §6.
Cột **"test ID"** là tên test CTest (stem file trong `tests/`), theo
`docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` §0.3.

### 2.1 Transform — matrix stack (GL 2.1 §2.11)

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Current matrix mode | `GL_MATRIX_MODE` 0x0BA0 | `GL_MODELVIEW` | `LegacyState.h:128` | **PARTIALLY_IMPLEMENTED** | `test_legacy_matrix` | State đúng, **nhưng query trả `GL_INVALID_ENUM`** (đo) — xem §6 L-1; enum `GL_COLOR` bị dùng sai — §6 L-9 |
| Modelview matrix | `GL_MODELVIEW_MATRIX` 0x0BA6 | identity | `LegacyState.h:129` (`modelview.top()`) | **PARTIALLY_IMPLEMENTED** | `test_legacy_matrix` | Query đúng; **chưa có đường GPU** |
| Projection matrix | `GL_PROJECTION_MATRIX` 0x0BA7 | identity | `LegacyState.h:129` (`projection.top()`) | **PARTIALLY_IMPLEMENTED** | `test_legacy_matrix` | như trên |
| Texture matrix | `GL_TEXTURE_MATRIX` 0x0BA8 | identity | `LegacyState.h:129` (`texture.top()`) | **PARTIALLY_IMPLEMENTED** | `test_legacy_matrix` | **1 stack dùng chung**, spec §2.11.2 yêu cầu *mỗi texture unit một stack* → sai khi dùng multi-texture |
| Color matrix stack depth | `GL_COLOR_MATRIX_STACK_DEPTH` 0x80B2 | ≥2 | `LegacyState.h:129` (`color`) | **MISSING** | — | Query trả `GL_INVALID_ENUM` (đo); stack **không truy cập được** vì matrix mode sai — xem §6 L-9 |
| Modelview stack depth | `GL_MODELVIEW_STACK_DEPTH` 0x0BA3 | 1 | `LegacyState.h:129` | **FULLY_IMPLEMENTED** | `test_legacy_matrix` | query đúng (đo: 1) |
| Projection stack depth | `GL_PROJECTION_STACK_DEPTH` 0x0BA4 | 1 | `LegacyState.h:129` | **FULLY_IMPLEMENTED** | `test_legacy_matrix` | đo: 1 |
| Texture stack depth | `GL_TEXTURE_STACK_DEPTH` 0x0BA5 | 1 | `LegacyState.h:129` | **FULLY_IMPLEMENTED** | `test_legacy_matrix` | đo: 1 |
| Max modelview stack depth | `GL_MAX_MODELVIEW_STACK_DEPTH` 0x0D36 | ≥32 | `LegacyState.h:79` | **FULLY_IMPLEMENTED** | `test_legacy_matrix` | đo: 32 |
| Max projection stack depth | `GL_MAX_PROJECTION_STACK_DEPTH` 0x0D38 | ≥2 | `LegacyState.h:79` | **PARTIALLY_IMPLEMENTED** | — | Trả **32** — giá trị **đúng**, nhưng do `case 0x0D38` bị gán nhãn `MAX_TEXTURE_STACK_DEPTH`; đồng thời `case 0x0D37` (enum đã gỡ `GL_MAX_NAME_STACK_DEPTH`) cũng trả 32 **không lỗi** → xem §6 L-3 |
| Max texture stack depth | `GL_MAX_TEXTURE_STACK_DEPTH` 0x0D39 | ≥2 | `LegacyState.h:79` | **MISSING** | — | Query đúng 0x0D39 ⇒ `GL_INVALID_ENUM` (đo) → xem §6 L-3 |
| Viewport | `GL_VIEWPORT` 0x0BA2 | theo window | `StateTracker.h:115-124` | **FULLY_IMPLEMENTED** | `test_viewport_array` | dùng chung core/compat |
| Depth range | `GL_DEPTH_RANGE` 0x0B70 | (0,1) | `StateTracker.h:125-131` | **FULLY_IMPLEMENTED** | `test_viewport_array` | dùng chung |
| Normal rescale | `GL_RESCALE_NORMAL` 0x803A | FALSE | — | **MISSING** | — | không có state; `glEnable` vẫn "ok" → §6 L-4 |
| Normal normalize | `GL_NORMALIZE` 0x0BA1 | FALSE | — | **MISSING** | — | như trên |

### 2.2 Current vertex attributes (GL 2.1 §2.7)

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Current color | `GL_CURRENT_COLOR` 0x0B00 | (1,1,1,1) | `LegacyState.h:132` | **PARTIALLY_IMPLEMENTED** | `test_legacy_immediate` | Query đúng (đo: 1,1,1,1); **chưa có đường GPU** |
| Current index | `GL_CURRENT_INDEX` 0x0B01 | 0 | `LegacyState.h:144` | **PARTIALLY_IMPLEMENTED** | — | color-index mode đã bị gỡ ở GL 4.6 Core (App. E.2.2) |
| Current normal | `GL_CURRENT_NORMAL` 0x0B02 | (0,0,1) | `LegacyState.h:133` | **PARTIALLY_IMPLEMENTED** | `test_legacy_immediate` | Query đúng (đo: 0,0,1) |
| Current secondary color | `GL_CURRENT_SECONDARY_COLOR` 0x8459 | (0,0,0,1) | `LegacyState.h:134` (`float[3]`) | **INCORRECT** | — | Query pname đúng 0x8459 ⇒ `GL_INVALID_ENUM` (đo); state chỉ có 3 thành phần → xem §6 L-2 |
| Current texture coords | `GL_CURRENT_TEXTURE_COORDS` 0x0B03 | (0,0,0,1) | `LegacyState.h:136` | **INCORRECT** | `test_legacy_immediate` | Query pname đúng 0x0B03 trả **màu secondary** (đo); test hiện dùng **sai** enum 0x0B04 → xem §6 L-2 |
| Active texture unit | `GL_ACTIVE_TEXTURE` 0x84E0 | `GL_TEXTURE0` | `StateTracker.h:200` + `LegacyState.h:138` | **FULLY_IMPLEMENTED** | `test_legacy_immediate` | đồng bộ 2 chiều |
| Edge flag (hiện tại) | — (`glEdgeFlag`) | TRUE | `LegacyState.h:143` | **PARTIALLY_IMPLEMENTED** | — | state có, chưa dùng để rasterize |
| Raster position | `GL_CURRENT_RASTER_POSITION` 0x0B07 | (0,0,0,1) | — | **MISSING** | — | `glRasterPos*` là stub |
| Raster position valid | `GL_CURRENT_RASTER_POSITION_VALID` 0x0B08 | FALSE | — | **MISSING** | — | |
| Raster position distance | `GL_RASTER_POSITION_DISTANCE` (không còn enum ở GL 4.6) | 0 | — | **NOT APPLICABLE** | — | đã bị gỡ |

### 2.3 Lighting / material / shade model (GL 2.1 §2.14)

**Toàn bộ nhóm này hiện KHÔNG có state.** Các entry point `glLight*`,
`glLightModel*`, `glMaterial*`, `glColorMaterial`, `glShadeModel` đều là **stub
trung thực** trong `src/gl/gl_stubs_gen.cpp` (`:41-43`, `:235-258`, `:289-300`,
`:560-562`) — chúng chỉ ghi một dòng debug `KHR_debug` rồi trả về giá trị an toàn.
*Không* sinh `GL_INVALID_OPERATION` (đúng spec: lệnh hợp lệ không được sinh lỗi).

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Lighting on/off | `GL_LIGHTING` 0x0B50 | FALSE | — | **MISSING** | — | `glEnable(GL_LIGHTING)` **không lỗi** nhưng vô tác dụng → §6 L-4 |
| Color material on/off | `GL_COLOR_MATERIAL` 0x0B57 | FALSE | — | **MISSING** | — | |
| Color material parameter | `GL_COLOR_MATERIAL_PARAMETER` 0x0B56 | `GL_AMBIENT_AND_DIFFUSE` | — | **MISSING** | — | |
| Color material face | `GL_COLOR_MATERIAL_FACE` 0x0B55 | `GL_FRONT_AND_BACK` | — | **MISSING** | — | |
| Shade model | `GL_SHADE_MODEL` 0x0B54 | `GL_SMOOTH` | — | **MISSING** | — | query ⇒ `GL_INVALID_ENUM` (đo) |
| Material ambient (front/back) | `GL_AMBIENT` 0x1200 | (0.2,0.2,0.2,1) | — | **MISSING** | — | qua `glGetMaterialfv` |
| Material diffuse | `GL_DIFFUSE` 0x1201 | (0.8,0.8,0.8,1) | — | **MISSING** | — | |
| Material specular | `GL_SPECULAR` 0x1202 | (0,0,0,1) | — | **MISSING** | — | |
| Material emission | `GL_EMISSION` 0x1600 | (0,0,0,1) | — | **MISSING** | — | |
| Material shininess | `GL_SHININESS` 0x1601 | 0.0 | — | **MISSING** | — | khoảng `[0,128]` |
| Material color indexes | `GL_COLOR_INDEXES` 0x1603 | (0,1,1) | — | **NOT APPLICABLE** | — | color-index mode đã gỡ (GL 4.6 App. E.2.2) |
| Light model ambient | `GL_LIGHT_MODEL_AMBIENT` 0x0B53 | (0.2,0.2,0.2,1) | — | **MISSING** | — | |
| Light model local viewer | `GL_LIGHT_MODEL_LOCAL_VIEWER` 0x0B51 | FALSE | — | **MISSING** | — | |
| Light model two side | `GL_LIGHT_MODEL_TWO_SIDE` 0x0B52 | FALSE | — | **MISSING** | — | |
| Light model color control | `GL_LIGHT_MODEL_COLOR_CONTROL` 0x81F8 | `GL_SINGLE_COLOR` | — | **MISSING** | — | |
| Light *i* enabled | `GL_LIGHT0`..`GL_LIGHT7` 0x4000-0x4007 | FALSE | — | **MISSING** | — | `glEnable(GL_LIGHT0)` vô tác dụng |
| Light ambient | `GL_AMBIENT` (glGetLightfv) | (0,0,0,1) | — | **MISSING** | — | |
| Light diffuse | `GL_DIFFUSE` (glGetLightfv) | i=0: (1,1,1,1); i>0: (0,0,0,1) | — | **MISSING** | — | |
| Light specular | `GL_SPECULAR` (glGetLightfv) | i=0: (1,1,1,1); i>0: (0,0,0,1) | — | **MISSING** | — | |
| Light position | `GL_POSITION` (glGetLightfv) | (0,0,1,0) | — | **MISSING** | — | |
| Light spot direction | `GL_SPOT_DIRECTION` | (0,0,−1) | — | **MISSING** | — | |
| Light spot exponent | `GL_SPOT_EXPONENT` | 0.0 | — | **MISSING** | — | `[0,128]` |
| Light spot cutoff | `GL_SPOT_CUTOFF` | 180.0 | — | **MISSING** | — | `[0,90] ∪ {180}` |
| Attenuation k0/k1/k2 | `GL_CONSTANT_/LINEAR_/QUADRATIC_ATTENUATION` | 1.0 / 0.0 / 0.0 | — | **MISSING** | — | `[0,∞)` |

### 2.4 Fog (GL 2.1 §3.10)

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Fog on/off | `GL_FOG` 0x0B60 | FALSE | — | **MISSING** | — | `glEnable(GL_FOG)` vô tác dụng |
| Fog mode | `GL_FOG_MODE` 0x0B65 | `GL_EXP` | — | **MISSING** | — | |
| Fog density | `GL_FOG_DENSITY` 0x0B62 | 1.0 | — | **MISSING** | — | `<0` ⇒ `GL_INVALID_VALUE` |
| Fog start | `GL_FOG_START` 0x0B63 | 0.0 | — | **MISSING** | — | |
| Fog end | `GL_FOG_END` 0x0B64 | 1.0 | — | **MISSING** | — | |
| Fog color | `GL_FOG_COLOR` 0x0B66 | (0,0,0,0) | — | **MISSING** | — | kẹp về [0,1] |
| Fog coord source | `GL_FOG_COORD_SRC` 0x8450 | `GL_FRAGMENT_DEPTH` | — | **MISSING** | — | |
| Fog index | `GL_FOG_INDEX` 0x0B61 | 0 | — | **NOT APPLICABLE** | — | color-index mode đã gỡ |
| Fog hint | `GL_FOG_HINT` 0x0C54 | `GL_DONT_CARE` | `glHint` là no-op (`src/gl/gl_raster.cpp:273`) | **MISSING** | — | |

### 2.5 Texture environment + texture generation (GL 2.1 §3.8.13, §2.11.4)

`glTexEnv*` / `glTexGen*` / `glGetTexEnv*` / `glGetTexGen*` là **stub**
(`src/gl/gl_stubs_gen.cpp:614-643`).
Có một **bóng DSA** (`EXT_direct_state_access`) lưu texenv/texgen theo unit:
`src/gl/gl_ext_dsa_state.cpp:358-435` (`glMultiTexEnv*EXT`, `glMultiTexGen*EXT`,
`glGetMultiTexEnv*EXT`, `glGetMultiTexGen*EXT`), lưu ở
`Context::ext.texEnvF/texEnvI/texGenF/texGenI` (`include/tglmt/Context.h:584-587`).
Bóng này **không** nối vào bất kỳ đường render nào và **không** phản ánh
`glTexEnv*` của GL 1.0 (tách unit theo `target` chứ không theo `texunit`).

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Texture env mode | `GL_TEXTURE_ENV_MODE` 0x2200 | `GL_MODULATE` | — (stub) | **MISSING** | — | query ⇒ `GL_INVALID_ENUM` (đo) |
| Texture env color | `GL_TEXTURE_ENV_COLOR` 0x2201 | (0,0,0,0) | — (stub) | **MISSING** | — | |
| Combine RGB | `GL_COMBINE_RGB` 0x8571 | `GL_MODULATE` | — | **MISSING** | — | |
| Combine alpha | `GL_COMBINE_ALPHA` 0x8572 | `GL_MODULATE` | — | **MISSING** | — | |
| Source 0/1/2 RGB | `GL_SOURCE0_RGB` 0x8580 / `0x8581` / `0x8582` | TEXTURE / PREVIOUS / CONSTANT | — | **MISSING** | — | |
| Source 0/1/2 alpha | `GL_SOURCE0_ALPHA` 0x8588 / `0x8589` / `0x858A` | TEXTURE / PREVIOUS / CONSTANT | — | **MISSING** | — | |
| Operand 0/1/2 RGB | `GL_OPERAND0_RGB` 0x8590 / `0x8591` / `0x8592` | SRC_COLOR / SRC_COLOR / SRC_ALPHA | — | **MISSING** | — | |
| Operand 0/1/2 alpha | `GL_OPERAND0_ALPHA` 0x8598 / `0x8599` / `0x859A` | SRC_ALPHA (cả 3) | — | **MISSING** | — | |
| RGB scale | `GL_RGB_SCALE` 0x8573 | 1.0 | — | **MISSING** | — | |
| Alpha scale | `GL_ALPHA_SCALE` 0x0D1C | 1.0 | — | **MISSING** | — | |
| Texture LOD bias | `GL_TEXTURE_LOD_BIAS` 0x8501 | 0.0 | — | **MISSING** | — | target `GL_TEXTURE_FILTER_CONTROL` |
| Point-sprite coord replace | `GL_COORD_REPLACE` 0x8862 | FALSE | — | **MISSING** | — | |
| Texgen mode (S/T/R/Q) | `GL_TEXTURE_GEN_MODE` 0x2500 | `GL_EYE_LINEAR` | — (stub) | **MISSING** | — | |
| Texgen object plane | `GL_OBJECT_PLANE` 0x2501 | s:(1,0,0,0) t:(0,1,0,0) r,q:(0,0,0,0) | — | **MISSING** | — | |
| Texgen eye plane | `GL_EYE_PLANE` 0x2502 | như trên | — | **MISSING** | — | |
| Texgen enable S/T/R/Q | `GL_TEXTURE_GEN_S` 0x0C60 … `GL_TEXTURE_GEN_Q` 0x0C63 | FALSE | — | **MISSING** | — | query ⇒ `GL_INVALID_ENUM` (đo) |

### 2.6 Rasterization legacy (GL 2.1 §3.x, §4)

| State | `glGet` pname | Mặc định spec | Nơi lưu trong TGLMT | Trạng thái | Test ID | Ghi chú |
|---|---|---|---|---|---|---|
| Polygon mode | `GL_POLYGON_MODE` 0x0B40 | `GL_FILL` | `StateTracker.h:280` | **PARTIALLY_IMPLEMENTED** | `test_core_raster_state` | chỉ FILL/LINE/POINT ở Metal; FRONT/BACK tách bị gỡ |
| Line width | `GL_LINE_WIDTH` 0x0B21 | 1.0 | `StateTracker.h:283` | **PARTIALLY_IMPLEMENTED** | `test_core_raster_state` | Metal chỉ 1.0 px ⇒ >1 phải `GL_INVALID_VALUE` |
| Point size | `GL_POINT_SIZE` 0x0B11 | 1.0 | `StateTracker.h:147` (`gl_raster.cpp:142-148`) | **FULLY_IMPLEMENTED** | `test_sample_state` | |
| Alpha test | `GL_ALPHA_TEST` 0x0BC0 | FALSE | — (`glAlphaFunc` stub `gl_stubs_gen.cpp:10`) | **MISSING** | — | |
| Alpha func / ref | `GL_ALPHA_TEST_FUNC` / `GL_ALPHA_TEST_REF` | `GL_ALWAYS` / 0 | — | **MISSING** | — | |
| Clip plane *i* | `GL_CLIP_PLANE0`..`GL_CLIP_PLANE5` 0x3000-0x3005 | FALSE, hệ số (0,0,0,0) | — (`glClipPlane` stub) | **MISSING** | — | xem `docs/LEGACY_TO_METAL_MAPPING.md` §7.3 |
| Polygon stipple | `GL_POLYGON_STIPPLE` 0x0B42 | mặc định 0xFFFFFFFF/row | — | **MISSING** | — | |
| Line stipple | `GL_LINE_STIPPLE` 0x0B24 | pattern 1, factor 1 | `src/gl/gl_raster.cpp:274` (no-op) | **STUB** | — | hàm đã export nhưng vô tác dụng |
| Point smooth | `GL_POINT_SMOOTH` 0x0B10 | FALSE | `StateTracker::caps_` | **INCORRECT** | — | `glIsEnabled` trả đúng nhưng rasterizer không làm gì |
| Line smooth | `GL_LINE_SMOOTH` 0x0B20 | FALSE | `StateTracker::caps_` | **INCORRECT** | — | query trả đúng (đo: 0) |
| Polygon smooth | `GL_POLYGON_SMOOTH` 0x0B41 | FALSE | `StateTracker::caps_` | **INCORRECT** | — | query trả đúng (đo: 0) |
| Color sum | `GL_COLOR_SUM` 0x0BE8 | FALSE | — | **MISSING** | — | |

---

## 3. Đường đi state → GPU

Đây là bảng quyết định **state nào thành uniform, state nào thành hằng số biên
dịch (compile-time constant), state nào ảnh hưởng pipeline key**.
Nền tảng: `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.1–4.4,
`docs/LEGACY_TO_METAL_MAPPING.md` §2.2–2.6.

### 3.1 Ba lớp

| Lớp | Ý nghĩa | Hậu quả khi đổi |
|---|---|---|
| **Uniform** (Metal constant buffer, slot 16) | giá trị đổi theo draw, **không** đổi hình dạng shader | upload lại buffer, **không** tạo pipeline mới |
| **Hằng số biên dịch** (compile-time constant trong MSL sinh) | quyết định *cấu trúc* shader (có/không block lighting, có/không texgen sphere-map…) | **sinh lại MSL → `ILibrary` mới → pipeline key mới** |
| **Pipeline key** | phần state mà Metal bake vào `MTLRenderPipelineState` (blend, depth, cull, format, vertex layout…) | `setRenderPipeline` mới |

**Quy tắc vàng (PLAN):** *giá trị* của state fixed-function **không bao giờ** được
đưa vào pipeline key. Chỉ *hình dạng* mới được. Lý do: `Context::PipelineKey`
(`include/tglmt/Context.h:1041-1108`) **đã** phân biệt được legacy program với
modern program bằng `vsLib`/`fsLib` (serial đơn điệu của `ILibrary::identity()`)
nên **không cần** thêm field nào — xem `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md`
§4.4.

### 3.2 Bảng phân loại (PLAN — state → nơi đi)

| State fixed-function | Điểm đến | Lý do |
|---|---|---|
| Modelview / Projection / Texture matrix | **uniform** (`float4x4`) | đổi mỗi draw mà không đổi hình dạng |
| Normal matrix (inverse-transpose 3×3) | **uniform** (`float4x4`, lấy 3×3 trên) | tránh bẫy padding `float3x3` của MSL (48B vs 36B) |
| Tham số ánh sáng (ambient/diffuse/specular/position/spot/attenuation) | **uniform** | đổi giá trị không đổi shape |
| Material (front/back) + light model | **uniform** | |
| Shade model SMOOTH/FLAT | **hằng số biên dịch** → `LegacyProgramKey.shadeModel` | flat path khác hẳn (xem §5.4) |
| `GL_LIGHTING` bật/tắt | **hằng số biên dịch** → `key.lighting` | có/không khối lighting |
| `GL_LIGHT_MODEL_TWO_SIDE` | **hằng số biên dịch** → `key.twoSided` | fragment stage cần `[[front_facing]]` |
| `GL_LIGHT_MODEL_COLOR_CONTROL` | **hằng số biên dịch** → `key.separateSpecular` | thêm 1 varying + 1 phép cộng ở fragment |
| Số đèn (`GL_LIGHT0..7`) | **uniform** (`numLights`) — **không** đưa vào key | vòng lặp `for(i<u.numLights)` ⇒ bật đèn thứ 9 không đổi shader |
| Fog mode / density / start / end / color | **uniform** | |
| `GL_FOG_COORD_SRC` | **hằng số biên dịch** → `key.fogCoordSrc` | `GL_FOG_COORD` cần thêm 1 vertex attribute ⇒ đổi vertex layout ⇒ đổi `PipelineKey::attribs[]`/`stride` |
| `GL_FOG` bật/tắt | **hằng số biên dịch** → `key.fog` | |
| Texenv mode / color / combine args / scale | **uniform** | |
| Texgen mode (OBJECT_LINEAR/EYE_LINEAR/SPHERE_MAP/…) | **hằng số biên dịch** → `key.texGen` | sphere/reflection map cần code riêng trong VS |
| Số texture unit dùng | **hằng số biên dịch** → `key.numTexUnits` | mỗi unit 1 khối sample |
| `GL_COLOR_MATERIAL` + face + parameter | **hằng số biên dịch** → `key.colorMaterialMask` | nhưng *giá trị* material theo current color phải resolve ở CPU (xem §5.3) |
| `GL_NORMALIZE` / `GL_RESCALE_NORMAL` | **hằng số biên dịch** → `key.normalize` / `key.rescaleNormal` | |
| `GL_ALPHA_TEST` + func + ref | func/ref là **uniform**; bật/tắt là **hằng số biên dịch** | `discard_fragment()` |
| `GL_CLIP_PLANE0..5` | số plane là **hằng số biên dịch**; hệ số là **uniform** | xem giới hạn Metal ở `docs/LEGACY_TO_METAL_MAPPING.md` §7.3 |
| `GL_POLYGON_STIPPLE` | mask là **uniform** (32×uint); bật/tắt là **hằng số biên dịch** | |
| Blend / depth / stencil / cull / front-face / viewport / scissor / color-mask / polygon-offset | **pipeline key** (đã có sẵn trong `StateTracker`) | dùng chung với core |
| Vertex layout legacy (position, normal, color, texcoord, fogCoord, flatColor) | **pipeline key** (`attribs[]`, `stride`) | hình dạng layout phụ thuộc `LegacyProgramKey` |

### 3.3 Bảng `LegacyProgramKey` (PLAN — cấu trúc)

Theo `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.3:

```
LegacyProgramKey {
    bool  lighting;          // GL_LIGHTING
    GLenum shadeModel;       // SMOOTH | FLAT
    bool  separateSpecular;  // GL_LIGHT_MODEL_COLOR_CONTROL == SEPARATE_SPECULAR_COLOR
    bool  twoSided;          // GL_LIGHT_MODEL_TWO_SIDE
    bool  fog;               // GL_FOG
    bool  fogCoordSrc;       // GL_FOG_COORD
    bool  alphaTest;
    uint8_t numClipPlanes;
    bool  polygonStipple;
    bool  texEnvCombine;     // có unit nào dùng GL_COMBINE
    uint8_t numTexUnits;
    uint8_t texGenMask;      // bit per mode đã bật
    uint8_t colorMaterialMask;// bit0 AMBIENT, bit1 DIFFUSE, bit2 SPECULAR, bit3 EMISSION
    bool  normalize, rescaleNormal;
    bool  pointSprite;
};
```

**Quy tắc bất biến:** `LegacyProgramKey` ⇒ *vertex layout* ⇒ `PipelineKey::stride`
và `PipelineKey::attribs[]` là ổn định. Không có đường nào trong đó được để
"rơi" vào uniform rồi lại đổi layout.

---

## 4. Quy tắc `glEnable` / `glDisable` cho enum fixed-function

**SPEC (GL 2.1 §2.11.3, §2.11.4, §2.14.1, §2.14.3, §3.10, §4.1.4, §3.5.2):
`glEnable`/`glDisable` nhận một *enable target*.** Target nào chỉ tồn tại ở
Compatibility Profile thì **phải** sinh `GL_INVALID_ENUM` ở Core Profile
(OpenGL 4.6 Core spec §22.1 + App. E.2.2, danh sách bị gỡ in trong
`docs/khronos/glspec46.core.pdf` trang 680-681).

### 4.1 Bảng enum

| Enum | Hex | Hợp lệ ở profile | Spec | Trạng thái TGLMT |
|---|---|---|---|---|
| `GL_LIGHTING` | 0x0B50 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.14.1 | **INCORRECT** — `glEnable` nhận, `glIsEnabled` trả TRUE, không có tác dụng |
| `GL_COLOR_MATERIAL` | 0x0B57 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.14.3 | **INCORRECT** — như trên |
| `GL_LIGHT0`…`GL_LIGHT7` | 0x4000-0x4007 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.14.2 | **INCORRECT** — như trên |
| `GL_FOG` | 0x0B60 | Compat (gỡ ở Core 3.2) | GL 2.1 §3.10 | **INCORRECT** — như trên |
| `GL_NORMALIZE` | 0x0BA1 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.3 | **INCORRECT** — như trên |
| `GL_RESCALE_NORMAL` | 0x803A | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.3 | **INCORRECT** — như trên |
| `GL_TEXTURE_GEN_S` | 0x0C60 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.4 | **INCORRECT** — như trên |
| `GL_TEXTURE_GEN_T` | 0x0C61 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.4 | **INCORRECT** — như trên |
| `GL_TEXTURE_GEN_R` | 0x0C62 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.4 | **INCORRECT** — như trên |
| `GL_TEXTURE_GEN_Q` | 0x0C63 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.11.4 | **INCORRECT** — như trên |
| `GL_COLOR_SUM` | 0x0BE8 | Compat (gỡ ở Core 3.2) | GL 2.1 §3.9 | **INCORRECT** |
| `GL_ALPHA_TEST` | 0x0BC0 | Compat (gỡ ở Core 3.2) | GL 2.1 §4.1.4 | **INCORRECT** |
| `GL_CLIP_PLANE0`…`GL_CLIP_PLANE5` | 0x3000-0x3005 | Compat (gỡ ở Core 3.2) | GL 2.1 §2.12 | **INCORRECT** |
| `GL_POLYGON_STIPPLE` | 0x0B42 | Compat (gỡ ở Core 3.2) | GL 2.1 §3.5.2 | **INCORRECT** |
| `GL_LINE_STIPPLE` | 0x0B24 | Compat (gỡ ở Core 3.2) | GL 2.1 §3.6 | **INCORRECT** |
| `GL_POINT_SPRITE` | 0x8861 | Compat; ở Core 3.2 bị gỡ (App. E.2.2 "Non-sprite points") | GL 1.4 §3.3 | **INCORRECT** |
| `GL_COORD_REPLACE` | 0x8862 | Compat | GL 1.4 §3.3 | **INCORRECT** |
| `GL_VERTEX_PROGRAM_TWO_SIDE` | 0x8643 | Compat | GL 2.0/GLSL 1.20 §2.14.1 | **INCORRECT** |
| `GL_LINE_SMOOTH` | 0x0B20 | **Core + Compat** (vẫn còn ở GL 4.6 Core §14) | GL 2.1 §3.6 | **INCORRECT** — state đúng, tác dụng không có |
| `GL_POLYGON_SMOOTH` | 0x0B41 | **Core + Compat** (vẫn còn ở GL 4.6 Core §14) | GL 2.1 §3.5 | **INCORRECT** — state đúng, tác dụng không có |
| `GL_POINT_SMOOTH` | 0x0B10 | Compat; Core 3.2 gỡ | GL 2.1 §3.3 | **INCORRECT** |
| `GL_TEXTURE_1D/2D/3D/CUBE_MAP/1D_ARRAY/2D_ARRAY/RECTANGLE/2D_MULTISAMPLE/2D_MULTISAMPLE_ARRAY` | 0x0DE0, 0x0DE1, 0x806F, 0x8513, 0x8C18, 0x8C1A, 0x84F5, 0x9100, 0x9102 | **Core + Compat** | GL 1.1–3.0 | **FULLY_IMPLEMENTED** (đường modern) |

Nguồn phân loại Core/Compat cho từng enum: `include/tglmt/gl46_types.h`
(1376 enum Core) vs `include/tglmt/gl_legacy_types.h` (428 enum chỉ-Compat),
sinh bởi `tools/gen_gl_headers.py` từ `docs/khronos/gl.xml` (D-005).

### 4.2 Vấn đề "state nói dối" (CURRENT, đã đo)

**SPEC.** Một enable target mà GL không hỗ trợ phải sinh `GL_INVALID_ENUM`
(GL 4.6 Core spec §22.1). Ở Core Profile, enable target chỉ tồn tại ở Compat
phải sinh lỗi (App. E.2.2).

**CURRENT.** `glEnable` hiện **nhận mọi enum**:
`src/gl/gl_raster.cpp:10` → `Context::Current().state.SetEnabled(cap, true)` →
`src/core/StateTracker.cpp:6-20` (`caps_[cap] = on`), không validate gì.
`glIsEnabled` (`src/gl/gl_raster.cpp:14`) trả đúng theo `caps_`.

**Đo được (probe, GL 4.6 Core):**

```
glEnable(GL_LIGHTING)   → err = 0x000 (không lỗi)
glIsEnabled(GL_LIGHTING) → 1
glGetIntegerv(GL_LIGHTING) → err = 0x500   (pname compat-only ⇒ INVALID_ENUM)
```

⇒ App ở Core Profile **tưởng** lighting đã bật. Đây là loại lỗi tệ nhất:
"no-op accepted". Đã ghi ở `TODO.md` **T0.9** (`docs` này không sửa TODO).

**PLAN.** Sửa ở `src/gl/gl_raster.cpp`:
1. Thêm bảng `kCompatOnlyEnableCaps` (20 enum ở §4.1) — khi
   `Context::Current().glProfile == GlProfile::Core` ⇒ `Record(0x0500)` và **không**
   ghi state.
2. Với enum lạ (không phải của GL) ⇒ `Record(0x0500)`.
3. Với profile Compat: enum chưa implement (lighting/fog/texenv/texgen/alpha
   test/clip plane/stipple) ⇒ **vẫn phải nhận** nhưng phải báo rõ cho app rằng
   chưa có tác dụng. Vì spec **không** cho phép báo lỗi ở đây, cơ chế đúng là:
   triển khai thật (mục tiêu), hoặc tối thiểu phát `KHR_debug` khi bật.

---

## 5. Công thức: shade model, color material, two-sided, normal rescaling

Nguồn: **OpenGL 2.1 spec §2.11.3** (normal transformation), **§2.14.1**
(lighting), **§2.14.3** (ColorMaterial), **§2.14.4** (lighting state),
**§2.14.7** (flatshading, bảng 2.12).

Ký hiệu (theo spec §2.14.1):
`c₁ ∗ c₂` = nhân từng thành phần RGB; `d₁ ⊙ d₂ = max(d₁·d₂, 0)`;
`V→P` = vector đơn vị từ V tới P.

### 5.1 Biến đổi normal (SPEC §2.11.3)

Gọi `M` là modelview matrix hiện tại.

**(a) Chuyển normal sang toạ độ eye:**

```
(nx', ny', nz', q') = (nx, ny, nz, q) · M⁻¹
```

với `q` tính từ toạ độ vertex:

```
        ⎧ 0                                  nếu w = 0
   q =  ⎨
        ⎩ −(nx·x + ny·y + nz·z)/w            nếu w ≠ 0
```

> Lưu ý: đây là **phép nhân vector hàng với nghịch đảo** — tức là
> `n' = (M⁻¹)ᵀ · n` khi xét 3 thành phần đầu. Với `M` vuông ⇒ bằng
> **inverse-transpose** của 3×3 trên trái.

**Biến thể được phép:** spec cho phép dùng `(nx',ny',nz') = (nx,ny,nz)·M_u⁻¹`
với `M_u` = 3×3 trên trái của `M`. (Đây là cách TGLMT nên làm: `normalMatrix`.)

**(b) Rescale (khi `GL_RESCALE_NORMAL` bật):**

```
(nx'', ny'', nz'') = f · (nx', ny', nz')
```

`f = 1` nếu rescale tắt. Nếu bật, spec cho **hai** cách:

```
f = 1 / √( m31² + m32² + m33² )        (m = M⁻¹, đánh số hàng/cột từ 1)
```

hoặc (được phép, khuyến nghị cho implementation):

```
f = 1 / √( nx'² + ny'² + nz'² )        ← tính lại cho từng normal
```

Cách thứ hai làm **mọi normal khác 0 trở thành đơn vị** bất kể độ dài đầu
vào và bản chất của `M`.

**(c) Normalize (sau rescale):**

```
n_f = m · (nx'', ny'', nz'')

        ⎧ 1                                    nếu normalize tắt
   m =  ⎨
        ⎩ 1 / √( nx''² + ny''² + nz''² )      nếu normalize bật
```

**Thứ tự bắt buộc:** transform → rescale → normalize. Không được đảo.

**Quy tắc dùng khi cả hai bật:** `GL_RESCALE_NORMAL` và `GL_NORMALIZE` là hai
bit độc lập; nếu cả hai bật thì normalize cuối cùng **xoá** tác dụng của
rescale về mặt độ dài (nhưng rescale vẫn ảnh hưởng nếu `f ≠ 1` và normal sau
transform khác 0 ⇒ `n_f` vẫn đơn vị; tức là normalize thắng về độ dài).

**Ghi chú spec (quan trọng):** ma trận modelview **không nghịch đảo được**
⇒ kết quả normal **undefined**; ma trận **sng** ⇒ các phần tử của nghịch đảo
**tùy ý**; nhưng **không** được làm GL ngắt/dừng.

**PLAN cho TGLMT:** normal matrix tính trên CPU khi modelview đổi, upload dạng
`float4x4` (lấy 3×3 trên) — tránh bẫy padding `float3x3` của MSL
(`docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.2.1).

### 5.2 Color material (SPEC §2.14.3)

`glColorMaterial(face, mode)`:
`face ∈ {GL_FRONT, GL_BACK, GL_FRONT_AND_BACK}`;
`mode ∈ {GL_EMISSION, GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_AMBIENT_AND_DIFFUSE}`.

| `mode` | Thành phần material theo current color |
|---|---|
| `GL_EMISSION` | `ecm` (emissive) |
| `GL_AMBIENT` | `acm` (ambient) |
| `GL_DIFFUSE` | `dcm` (diffuse) |
| `GL_SPECULAR` | `scm` (specular) |
| `GL_AMBIENT_AND_DIFFUSE` | **cả** `acm` và `dcm` |

Điều kiện để thành phần material *theo dõi* current color (hình 2.10 của spec):
`GL_COLOR_MATERIAL` **đang bật** **và** `face` khớp **và** `mode` khớp.

Đặc điểm quan trọng (spec §2.14.3):
- "The replacements made to material properties are **permanent**" — giá trị đã
  ghi đè vẫn còn cho tới khi có color mới hoặc `glMaterial` mới (khi
  `GL_COLOR_MATERIAL` không còn override).
- Material **có thể** đổi trong `glBegin/glEnd` (spec §2.14.2 cho phép
  `glMaterial` trong Begin/End; và `glColorMaterial` + `glColor` cũng vậy).
- Khi **vertex shader** đang hoạt động, thay đổi material trong Begin/End không
  được bảo đảm có hiệu lực cho tới `End` (spec §2.14.2, §2.14.3).

**Vị trí xử lý (PLAN):** resolve ở **CPU** trong assembler immediate mode
(`src/gl/gl_legacy_vertex.cpp` `AppendVertex`, `:165-180`) — không phải trong
shader. Vì current color là *client state* theo định nghĩa vertex, và giá trị
material phải "chảy" theo current color tại thời điểm `glVertex*` được gọi.

**Suy ra:** `LegacyProgramKey` **không** cần `colorMaterialMask` nếu resolve hết
ở CPU — nhưng nếu material được upload dưới dạng uniform thì vẫn cần mask để
shader biết phần nào bị override (xem `docs/LEGACY_TO_METAL_MAPPING.md` §5.2,
ghi chú cuối: "glColorMaterial must be resolved at *vertex specification time*").

### 5.3 Two-sided lighting (SPEC §2.14.1)

`GL_LIGHT_MODEL_TWO_SIDE` (`tbs`, mặc định FALSE):

- `tbs = TRUE`: tính **hai** màu.
  - Màu **front**: dùng **material front** với normal `n`.
  - Màu **back**: dùng **material back** với normal **−n**.
- `tbs = FALSE`: cả front và back đều dùng **material front** với normal `n`.

Ngoài ra, nếu `tbs = FALSE` nhưng `VERTEX_PROGRAM_TWO_SIDE` bật (chỉ khi vertex
shader hoạt động) thì GL chọn front/back color theo *facing* của primitive
(spec §2.14.1: "The selection between back and front colors depends on the
primitive… If it is a point or a line segment, the front color is always
selected. If it is a polygon, then the selection is based on the sign of the
(clipped or unclipped) polygon's signed area computed in window coordinates.").

**Vấn đề cần quyết định (PLAN):** lighting theo spec là **per-vertex**
("GL lighting computes colors **for each vertex** sent to the GL", §2.14.1), còn
facing là **per-fragment**. Không thể vừa per-vertex vừa chọn material theo
facing một cách chính xác khi tam giác cắt qua ranh giới facing.
`docs/LEGACY_TO_METAL_MAPPING.md` §5.2 chọn: dưới `GL_LIGHT_MODEL_TWO_SIDE`,
đánh giá lighting **per-fragment** bằng `[[front_facing]]` — đây là **sai lệch
có chủ đích**, phải ghi vào danh sách caveat của compatibility.

### 5.4 Flat shading (SPEC §2.14.7, bảng 2.12)

`glShadeModel(GL_FLAT)`: mọi đỉnh của primitive được gán **cùng** màu — màu của
"the vertex that spawned the primitive".

**Bảng 2.12 — vertex nguồn (đánh số 1..n, n = số đỉnh trong Begin/End):**

| Kiểu primitive | Vertex nguồn |
|---|---|
| single polygon (i ≡ 1) | **1** |
| triangle strip *i* | **i + 2** |
| triangle fan *i* | **i + 2** |
| independent triangle | **3i** |
| quad strip *i* | **2i + 2** |
| independent quad | **4i** |

Ngoài ra: với **point** → màu của chính point; với **line segment** → màu của
**đỉnh thứ hai** (đỉnh cuối) của segment.

Màu dùng là "the current color (if lighting is disabled) in effect when the
indicated vertex is specified"; nếu lighting bật thì là màu **đã được chiếu
sáng** của đỉnh đó.

**Quan trọng:** bảng này **không** trùng với quy ước provoking vertex của
Metal (`docs/LEGACY_TO_METAL_MAPPING.md` §1.5). Vì vậy `GL_FLAT` **không** thể
dựa vào `MTLRenderPipelineDescriptor`/flat-interpolation của Metal.

**PLAN (theo `docs/LEGACY_TO_METAL_MAPPING.md` §5.3):**
- Lighting **tắt**: assembler CPU biết chỉ số đỉnh nguồn flat ⇒ ghi màu đó vào
  **mọi** đỉnh của primitive ⇒ shader nội suy hằng số ⇒ **chính xác**.
- Lighting **bật**: màu flat phụ thuộc position/normal của đỉnh nguồn mà các
  đỉnh khác không có ⇒ phải truyền thêm một bộ attribute cho đỉnh nguồn flat
  (`flatColor`) ⇒ cũng chính xác, nhưng tốn 1 lighting evaluation + 3 attribute.

---

## 6. Lỗi đã đo được (CURRENT, cần sửa)

Tất cả lỗi dưới đây **đã đo bằng probe** trên `build-work/libtglmt.dylib`
(GL 4.6 Compatibility, Null backend). Không sửa file nguồn ở tài liệu này —
mỗi lỗi đã ghi vào `TODO.md` hoặc là phần của stage tương ứng ở §7.

| # | Hiện tượng | file:line | Bằng chứng đo | Mức |
|---|---|---|---|---|
| **L-1** | `glGetIntegerv(GL_MATRIX_MODE /*0x0BA0*/)` → `GL_INVALID_ENUM` | `src/gl/gl_get.cpp:294-311` (`CompatOnlyPname` không có 0x0BA0) và `:324-580` (không có case nào) | `0x0BA0 -> 0 err=500` | Trung bình |
| **L-2** | Pname fixed-function dùng **sai enum**: `0x0B03` bị ghi là CURRENT_SECONDARY_COLOR (thật là CURRENT_TEXTURE_COORDS), `0x0B04` bị ghi là CURRENT_TEXTURE_COORDS (thật là `GL_CURRENT_RASTER_COLOR`), `0x0B05` bị ghi là alias (thật là `GL_CURRENT_RASTER_INDEX`), `0x8849` bị ghi là alias TEXTURE_MATRIX (thật là `GL_MATRIX_INDEX_ARRAY_POINTER_ARB`) | `src/gl/gl_get.cpp:278` (`case 0x0B03`), `:281` (`case 0x0B04`), `:260` (`case 0x8849`), `:304-307` (`CompatOnlyPname`) | `glGetFloatv(0x0B03)` trả `(0,0,0,-1)` — tức **màu secondary**, không phải texcoord; `glGetFloatv(0x8459)` → `err=500` | **Cao** |
| **L-3** | `GL_MAX_PROJECTION_STACK_DEPTH` (0x0D38) và `GL_MAX_TEXTURE_STACK_DEPTH` (0x0D39) bị gán sai case: code dùng 0x0D37 (= `GL_MAX_NAME_STACK_DEPTH`, enum đã gỡ) và 0x0D38 | `src/gl/gl_get.cpp:248-249` | `glGetIntegerv(0x0D37)` → 32 **không lỗi** (phải là INVALID_ENUM); `glGetIntegerv(0x0D39)` → **0 + err=500** (phải là 32) | Trung bình |
| **L-4** | `glEnable`/`glDisable` chấp nhận mọi enum kể cả ở Core Profile | `src/gl/gl_raster.cpp:10-11`; `src/core/StateTracker.cpp:6-20` | Core: `glEnable(GL_LIGHTING)` → err=0, `glIsEnabled`=1 | **Cao** (TODO T0.9) |
| **L-5** | `GL_CURRENT_SECONDARY_COLOR` state chỉ có 3 thành phần (`float[3]`), spec là RGBA | `include/tglmt/LegacyState.h:134` | — | Thấp (chưa có đường render) |
| **L-6** | `GL_TEXTURE_MATRIX` chỉ có **một** stack dùng chung cho mọi texture unit; spec §2.11.2 yêu cầu mỗi unit một stack | `include/tglmt/LegacyState.h:129` | — | Thấp (chỉ ảnh hưởng multi-texture) |
| **L-7** | `glGetIntegerv(GL_DISPLAY_LIST /*0x82E7*/)` trả 0 **không lỗi**; 0x82E7 không phải là pname trong GL (enum chết) | `include/tglmt/gl46_pnames.h:368` | `0x82E7 -> 0 err=0` | Thấp |
| **L-8** | `glLightfv(GL_LIGHT0, GL_DIFFUSE, …)` **không** sinh lỗi, chỉ ghi debug `KHR_debug` | `src/gl/gl_stubs_gen.cpp:247-258` | `glLightfv err=0` | Trung bình (đúng spec là lệnh hợp lệ không được báo lỗi ⇒ phải implement) |
| **L-9** | **Matrix mode `GL_COLOR` dùng sai enum**: code dùng `0x180D` (không phải enum GL nào — `docs/khronos/gl.xml` có **0** enum 0x180D) thay vì `GL_COLOR` = **0x1800** | `src/gl/gl_legacy_matrix.cpp:91` (`case 0x180D`), `:97` (`ValidMatrixMode`) | `glMatrixMode(0x1800 /*GL_COLOR*/)` ⇒ **`err=0x500`** (spec §2.11.2: COLOR là mode hợp lệ ⇒ phải chấp nhận); `glMatrixMode(0x180D)` ⇒ **không lỗi** (enum không tồn tại) | **Cao** |

---

## 7. Lộ trình implement

Bám theo `TODO.md` (T1) và `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` (S0-S16).
Mỗi dòng: **entry point + file** → **test ID** → **exit criteria**.

| Stage | Nội dung | Entry point / file | Test ID | Exit criteria |
|---|---|---|---|---|
| **F0** *(= TODO T0.8/T0.9)* | Sửa pname fixed-function + chặn cap ở Core + sửa enum matrix mode | `src/gl/gl_get.cpp`, `src/gl/gl_raster.cpp`, `src/gl/gl_legacy_matrix.cpp` | `test_legacy_matrix` (mở rộng), `test_context_version` | `glGetIntegerv(GL_MATRIX_MODE)` trả đúng ở Compat; `glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH)` = 32; `glGetFloatv(GL_CURRENT_TEXTURE_COORDS)` trả texcoord; `glEnable(GL_LIGHTING)` ở Core ⇒ `GL_INVALID_ENUM`; `glMatrixMode(GL_COLOR /*0x1800*/)` **không** lỗi |
| **F1** *(= TODO T1-S4 / B.1)* | **Chương trình MSL sinh cho fixed-function** + vertex layout legacy + nối vào `AppleDrawGL` | `src/gl/gl_legacy_program.cpp` (mới), `src/gl/gl_apple_draw.cpp` (dispatch), `include/tglmt/LegacyProgram.h` (mới) | `test_legacy_draw_apple` (mới, GPU) | Vẽ được tam giác fixed-function trên `A11_DEVICE`; ảnh khớp reference; `PipelineKey` **không** cần field mới |
| **F2** *(= TODO T1-S5)* | Lighting + material + shade model + color material | 18 lệnh: `glLight{f,i,v,fv,iv}`, `glLightModel{f,i,v,fv,iv}`, `glMaterial{f,i,v,fv,iv}`, `glColorMaterial`, `glShadeModel` | `test_legacy_lighting` (mới) | `glGetLightfv`/`glGetMaterialfv` trả đúng mặc định; công thức §5 khớp spec trên 1 case tính tay; `glShadeModel(GL_FLAT)` đúng bảng 2.12 |
| **F3** *(= TODO T1-S6)* | Fog | 4 lệnh: `glFog{f,i,v,fv,iv}` | `test_legacy_fog` (mới) | 3 mode đúng công thức §3.10 của `docs/LEGACY_TEXTURE_AND_LIGHTING.md`; fog áp **sau** texturing, **trước** alpha test |
| **F4** *(= TODO T1-S7)* | Texenv + texgen | 15 lệnh `glTexEnv*`/`glTexGen*` + `glGetTexEnv*`/`glGetTexGen*` | `test_legacy_texenv` (mới) | 5 mode cơ bản + COMBINE đúng bảng 3.21-3.25; texgen 4 mode đúng §2.11.4 |
| **F5** *(= TODO T1-S8)* | Rasterization legacy | `glAlphaFunc`, `glClipPlane`, `glPolygonStipple`, `glLineStipple` (đã export nhưng no-op), point/line/polygon smooth | `test_legacy_raster` (mới) | Alpha test/stipple đúng bằng `discard_fragment`; clip plane đúng số plane Metal cho phép (xem `docs/LEGACY_TO_METAL_MAPPING.md` §7.3) |
| **F6** *(= TODO T1-S9)* | Raster position / window position | 40 lệnh `glRasterPos*`/`glWindowPos*` | `test_legacy_rasterpos` (mới) | Raster position transform đúng §2.13; window position bypass transform |

**Ràng buộc cho mọi stage F1-F6:**

1. Không sửa `include/tglmt/Context.h` để nhét state legacy vào `StateTracker`
   (vi phạm D-004). State mới của lighting/fog/texenv/texgen đặt trong
   `Context::legacy` (mở rộng `LegacyState`) hoặc `Context::ext`.
2. Mọi pname mới phải thêm vào `tools/gen_state_pnames.py` (sinh
   `include/tglmt/gl46_pnames.h`) — **không** hardcode danh sách trong tay.
   Hiện trong 964 pname được chấp nhận **không** có pname nào thuộc nhóm
   *fixed-function-only* (matrix stack, current vertex attributes, lighting, fog,
   texenv/texgen) — đo: **0/964**; mọi query nhóm đó phải đi qua `case` riêng
   (`LegacyPnameI` / `LegacyPnameF` trong `src/gl/gl_get.cpp:242-288`).
3. Mọi enum dùng trong test phải lấy từ `include/tglmt/gl_legacy_types.h`
   (compat-only) hoặc `gl46_types.h` (core) — **không** literal.

---

## 8. Điểm chưa verify

| # | Điểm | Vì sao chưa verify |
|---|---|---|
| V-1 | Mọi kết luận "ảnh khớp reference" của F1-F6 | Chưa có đường draw legacy ⇒ chưa có ảnh nào để so |
| V-2 | Nhãn môi trường `A11_DEVICE` | Toàn bộ phép đo chạy Null backend trên máy dev, **không** phải iPhone 8/X |
| V-3 | `GL_MAX_PROJECTION_STACK_DEPTH`/`GL_MAX_TEXTURE_STACK_DEPTH` có đúng giá trị sau khi sửa | Đo trước khi sửa, giá trị 32 là giá trị *tình cờ* đúng qua case nhầm |
| V-4 | Số texture unit legacy tối đa (8 trong `LegacyState.h:136`) | Spec GL 1.3 yêu cầu ≥2; giá trị 8 là lựa chọn của TGLMT, chưa đối chiếu với `GL_MAX_TEXTURE_UNITS` mà TGLMT báo |
| V-5 | Tương thích với `EXT_direct_state_access` (bóng texenv/texgen) | Chưa quyết định: giữ bóng hay bỏ khi implement `glTexEnv*` thật |
| V-6 | Công thức lighting §5 khi chạy trên GPU | Chưa có chương trình sinh ⇒ mới là công thức lý thuyết đối chiếu `glspec21.pdf` |

---

## 9. Tham chiếu chéo

* `docs/IMMEDIATE_MODE.md` — assembler, primitive assembly, lệnh bị cấm trong `glBegin/glEnd`.
* `docs/LEGACY_TEXTURE_AND_LIGHTING.md` — công thức lighting/fog/texenv/texgen đầy đủ.
* `docs/DISPLAY_LISTS.md` — command stream, resource dependency.
* `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` — program key, pipeline key, vertex layout.
* `docs/LEGACY_TO_METAL_MAPPING.md` — chiến lược Metal cho từng cơ chế.
* `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` — stage S0-S16, exit criteria.
* `docs/DECISIONS.md` — D-002 (version-gate), D-003 (gate ở C ABI), D-004 (tách state), D-005 (enum).
* `docs/DECISIONS.md` D-001 — `docs/khronos/gl.xml` là nguồn chân lý cho enum.
