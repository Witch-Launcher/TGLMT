# LEGACY_TEXTURE_AND_LIGHTING.md — lighting, fog, texture env & texgen

Tài liệu kỹ thuật dùng để **implement** phần fixed-function *lighting /
materials*, *fog*, *texture environment* và *texture coordinate generation* của
OpenGL Compatibility Profile trên TGLMT (OpenGL → Metal, iOS / Apple A11).

**Nguồn spec:** OpenGL 2.1 Specification (`glspec21.pdf`, 01-Dec-2006) —
**§2.14 "Colors and Coloring"** (§2.14.1 Lighting, §2.14.2 Lighting Parameter
Specification, §2.14.3 ColorMaterial, §2.14.4 Lighting State, §2.14.6 Clamping,
§2.14.7 Flatshading, bảng 2.10/2.11/2.12), **§2.11.3 "Normal Transformation"**,
**§2.11.4 "Generating Texture Coordinates"**, **§3.8.13 "Texture Environments and
Texture Functions"** (bảng 3.20-3.25), **§3.9 "Color Sum"**, **§3.10 "Fog"**
(phương trình 3.31-3.33), bảng state 6.10-6.13 & 6.19.
Đối chiếu cục bộ: `docs/khronos/glspec46.core.pdf` (App. E.2.2 xác nhận toàn bộ
nhóm này đã bị gỡ khỏi Core Profile 3.2 ⇒ **COMPAT-ONLY**).

**Nhãn:** `SPEC` · `CURRENT` (code thật, kèm `file:line`) · `PLAN`.
Trạng thái theo bộ `AGENTS.md` §6.

**Môi trường đo:** Null backend, build Debug (`build-work/`), nhãn `NONE`.

> **Cảnh báo về công thức:** mọi công thức dưới đây được trích **nguyên văn**
> từ `glspec21.pdf` (đã tải về từ
> `https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/specs/gl/glspec21.pdf`
> và đối chiếu). Có **một** công thức trong tài liệu cùng repo
> (`docs/LEGACY_TO_METAL_MAPPING.md` §8.2) **bị thiếu căn bậc hai** — xem §5.4.

---

## 0. Tóm tắt trạng thái (CURRENT, đo được)

Census: đọc `include/tglmt/gl_legacy.h` × định nghứa thật trong `src/gl/*.cpp`
(trừ `gl_stubs_gen.cpp`, `gl_c_exports.cpp`).

| Nhóm | Entry point khai báo | Đã implement | Stub | File stub |
|---|---:|---:|---:|---|
| **Lighting** (`glLight*`, `glLightModel*`, `glMaterial*`, `glGetLight*`, `glGetMaterial*`, `glColorMaterial`, `glShadeModel`) | **18** | **0** | **18** | `src/gl/gl_stubs_gen.cpp:41-43` (glColorMaterial), `:141-147` (glGetLight*), `:156-161` (glGetMaterial*), `:235-258` (glLight*), `:289-300` (glMaterial*), `:560-562` (glShadeModel) |
| **Fog** (`glFog{f,i,v,fv}`) | **4** | **0** | **4** | `src/gl/gl_stubs_gen.cpp:122-133` |
| **Fog coord** (`glFogCoord{f,d,fv,dv}`) | **4** | **0** | **4** | `src/gl/gl_stubs_gen.cpp:107-121` |
| **Texenv** (`glTexEnv{f,i,v,fv}`, `glGetTexEnv{fv,iv}`) | **6** | **0** | **6** | `src/gl/gl_stubs_gen.cpp:174-179` (glGetTexEnv*), `:614-625` (glTexEnv*) |
| **Texgen** (`glTexGen{f,i,d,v,fv,iv}`, `glGetTexGen{fv,iv}`, `glGetTexGendv`) | **9** | **0** | **9** | `src/gl/gl_stubs_gen.cpp:180-188` (glGetTexGen*), `:626-643` (glTexGen*) |
| **TỔNG (lighting + fog + texenv + texgen)** | **41** | **0** | **41** | |

**Bóng extension (EXT_direct_state_access)** — lưu texenv/texgen theo unit nhưng
**không** nối vào đường render:

| Nhóm | Số lệnh | File | Trạng thái |
|---|---:|---|---|
| `glMultiTexEnv{f,fv,i,iv}EXT` | 4 | `src/gl/gl_ext_dsa_state.cpp:358-375` | **PARTIALLY_IMPLEMENTED** — lưu vào `Context::ext.texEnvF/texEnvI`, không ảnh hưởng render |
| `glMultiTexGen{f,fv,i,iv}EXT`, `glMultiTexGen{d,dv}EXT` | 6 | `src/gl/gl_ext_dsa_state.cpp:376-403` | **PARTIALLY_IMPLEMENTED** — `Context::ext.texGenF/texGenI` |
| `glGetMultiTexEnv{fv,iv}EXT` | 2 | `src/gl/gl_ext_dsa_state.cpp:404-417` | **PARTIALLY_IMPLEMENTED** |
| `glGetMultiTexGen{fv,iv}EXT`, `glGetMultiTexGendvEXT` | 3 | `src/gl/gl_ext_dsa_state.cpp:418-435` | **PARTIALLY_IMPLEMENTED** |
| **TỔNG bóng EXT** | **15** | | |

⇒ **Không có** entry point lighting/fog/texenv/texgen nào của GL 1.0–2.1 được
implement thật. **0/41**.

**Test:** chưa có test nào cho nhóm này (`TODO.md` T1-S5/S6/S7 đều `[ ]`).

---

## 1. SPEC — công thức lighting (GL 2.1 §2.14.1)

### 1.1 Ký hiệu

Theo §2.14.1 (nguyên văn):

> "If c1 and c2 are colors without alpha where c1 = (r1, g1, b1) and c2 = (r2, g2,
> b2), then define **c1 ∗ c2 = (r1r2, g1g2, b1b2)**. Addition of colors is
> accomplished by addition of the components. Multiplication of colors by a scalar
> means multiplying each component by that scalar. If d1 and d2 are directions,
> then define **d1 ⊙ d2 = max{d1 · d2, 0}**. … If P1 and P2 are (homogeneous, with
> four coordinates) points then let **→P1P2** be the unit vector that points from P1
> to P2. … If d is an arbitrary direction, then let **d̂** be the unit vector in d's
> direction. Let ‖P1P2‖ be the distance between P1 and P2. Finally, let **V** be the
> point corresponding to the vertex being lit, and **n** be the corresponding
> normal. Let **Pe** be the eyepoint ((0, 0, 0, 1) in eye coordinates)."

Tên ký hiệu (bảng 2.10):

| Ký hiệu | Nghĩa | Mặc định |
|---|---|---|
| `acm` | ambient color of material | (0.2, 0.2, 0.2, 1.0) |
| `dcm` | diffuse color of material | (0.8, 0.8, 0.8, 1.0) |
| `scm` | specular color of material | (0.0, 0.0, 0.0, 1.0) |
| `ecm` | emissive color of material | (0.0, 0.0, 0.0, 1.0) |
| `srm` | specular exponent (range [0.0, 128.0]) | 0.0 |
| `acli` | ambient intensity of light *i* | (0.0, 0.0, 0.0, 1.0) |
| `dcli` | diffuse intensity of light *i* | i=0: (1,1,1,1); i>0: (0,0,0,1) |
| `scli` | specular intensity of light *i* | i=0: (1,1,1,1); i>0: (0,0,0,1) |
| `Ppli` | position of light *i* (object coords, w=0 ⇒ ở vô cùng) | (0,0,1,0) |
| `sdli` | direction of spotlight for light *i* | (0,0,−1) |
| `srli` | spotlight exponent (range [0.0, 128.0]) | 0.0 |
| `crli` | spotlight cutoff angle (range [0.0, 90.0] ∪ {180.0}) | 180.0 |
| `k0i`, `k1i`, `k2i` | attenuation (range [0.0, ∞)) | 1.0 / 0.0 / 0.0 |
| `acs` | ambient color of scene | (0.2, 0.2, 0.2, 1.0) |
| `vbs` | local viewer (TRUE = eye tại (0,0,0); FALSE = (0,0,∞)) | FALSE |
| `ces` | color control: `SINGLE_COLOR` / `SEPARATE_SPECULAR_COLOR` | SINGLE_COLOR |
| `tbs` | two-sided lighting | FALSE |

### 1.2 Khi nào lighting áp dụng

> "Lighting is turned on or off using the generic Enable or Disable commands with
> the symbolic value LIGHTING. **If lighting is off**, the current color and
> current secondary color are assigned to the vertex primary and secondary color,
> respectively. **If lighting is on**, colors computed from the current lighting
> parameters are assigned to the vertex primary and secondary colors."

⇒ Lighting **per-vertex**: "GL lighting computes colors **for each vertex** sent to
the GL" (§2.14.1).

### 1.3 Tổng màu (nguyên văn, `SINGLE_COLOR`)

```
c_pri = e_cm
      + a_cm ∗ a_cs
      + Σ_{i=0}^{n−1} (att_i)(spot_i) [ a_cm ∗ a_cli
                                      + (n ⊙ →V P_li) d_cm ∗ d_cli
                                      + (f_i)(n ⊙ ĥ_i)^{s_rm} s_cm ∗ s_cli ]

c_sec = (0, 0, 0, 1)
```

**Tổng màu (`SEPARATE_SPECULAR_COLOR`):**

```
c_pri = e_cm
      + a_cm ∗ a_cs
      + Σ_{i=0}^{n−1} (att_i)(spot_i) [ a_cm ∗ a_cli
                                      + (n ⊙ →V P_li) d_cm ∗ d_cli ]

c_sec = Σ_{i=0}^{n−1} (att_i)(spot_i)(f_i)(n ⊙ ĥ_i)^{s_rm} s_cm ∗ s_cli
```

### 1.4 Các phụ trợ (nguyên văn, phương trình 2.2 – 2.5)

```
(2.2)   f_i  =  1,        n ⊙ →V P_li ≠ 0
               0,        otherwise

(2.3)   h_i  =  →V P_li + →V P_e ,   v_bs = TRUE
               →V P_li + (0 0 1)^T ,  v_bs = FALSE
        (hi là vector; ĥ_i là vector đơn vị theo hướng của hi)

(2.4)   att_i =  1 / (k0_i + k1_i·‖V P_li‖ + k2_i·‖V P_li‖²) ,  nếu P_li's w ≠ 0
                1.0 ,  otherwise
        (light có hướng, w = 0 ⇒ không suy giảm theo khoảng cách)

(2.5)   spot_i = ( →P_li V ⊙ ŝ_dli )^{sr_li} ,  nếu cr_li ≠ 180.0 và →P_li V ⊙ ŝ_dli ≥ cos(cr_li)
                 0.0 ,              nếu cr_li ≠ 180.0 và →P_li V ⊙ ŝ_dli <  cos(cr_li)
                 1.0 ,              nếu cr_li = 180.0
```

### 1.5 Đọc kỹ 4 công thức then chốt

**(a) `f_i` (2.2) — "specular chỉ sáng khi diffuse sáng".**
`f_i = 1` khi `n ⊙ →V P_li ≠ 0`, tức là khi **diffuse term khác 0**. Đây là quy
tắc cổ điển của fixed-function lighting: nếu mặt quay lưng về nguồn sáng thì
specular cũng tắt. Lưu ý điều kiện dùng **⊙** (clamp ở 0), **không** phải dot
thuần.

**(b) `h_i` (2.3) — vector phản chiếu về nguồn sáng.**
* **Local viewer** (`v_bs = TRUE`): `h_i = →V P_li + →V P_e` = hướng từ đỉnh
  về nguồn sáng **cộng** hướng từ đỉnh về mắt người quan sát. `→V P_e` là hướng
  từ đỉnh về eyepoint.
* **Infinite viewer** (`v_bs = FALSE`): `h_i = →V P_li + (0 0 1)^T` — hướng về
  người quan sát là **hằng** `(0,0,1)` trong eye space (camera nhìn theo −Z).
* `ĥ_i` = **vector đơn vị** theo hướng `h_i` ⇒ đây là "half vector" kinh điển
  (phản chiếu về nguồn sáng).

**Suy ra công thức hiện dụng (tương đương):**

```
L̂ = normalize( P_eye − V_eye )             // hướng từ đỉnh về nguồn sáng
V̂ = localViewer ? normalize( −V_eye )      // hướng từ đỉnh về mắt (local)
              : (0, 0, 1)                  // hướng từ đỉnh về mắt (infinite)
Ĥ = normalize( L̂ + V̂ )                    // half vector
n̂ · Ĥ  =  (n ⊙ ĥ_i)   (vì ĥ_i = normalize(L̂ + V̂))
```

**(c) `att_i` (2.4) — suy giảm theo khoảng cách.**
`1 / (k0 + k1·d + k2·d²)` với `d = ‖V P_li‖` = khoảng cách từ đỉnh tới vị trí
nguồn sáng (eye space). **Nếu `w = 0`** (nguồn sáng ở vô cùng / directional) thì
`att_i = 1.0` — không suy giảm.

**(d) `spot_i` (2.5) — hình nón.**
Vector dùng là **`→P_li V`** = hướng **từ nguồn sáng tới đỉnh** (ngược với
`→V P_li`). Dot với `ŝ_dli` (hướng chiếu của nguồn sáng, đã transform) rồi **clamp
⊙ ở 0**, lũy thừa `sr_li`, và so với `cos(cr_li)`.

* `cr_li = 180.0` ⇒ `spot_i = 1.0` (không có hình nón).
* Ngoài hình nón (`dot < cos(cr_li)`) ⇒ `spot_i = 0`.
* Vì `⊙` clamp ở 0, `dot` âm (đỉnh nằm sau nguồn sáng) ⇒ 0 ⇒ ngoài hình nón.

**Đặc biệt:** `cr_li ∈ [0, 90] ∪ {180}` ⇒ `cos(cr_li) ≥ 0` ⇒ vùng "trong hình nón"
luôn có dot ≥ 0, nhất quán với clamp `⊙`.

### 1.6 Các điều kiện khác của §2.14.1 (nguyên văn)

* "All computations are carried out in **eye coordinates**."
* "The value of A produced by lighting is the alpha value associated with `dcm`.
  A is always associated with the primary color `c_pri`; the alpha component of
  `c_sec` is always 1."
* "Results of lighting are undefined if the w coordinate (w in eye coordinates) of
  V is zero."
* **Two-sided** (`tbs = TRUE`): "a front color is computed with one set of material
  parameters (the front material) and a back color is computed with a second set of
  material parameters (the back material). **This second computation replaces n
  with −n.** If `tbs = FALSE`, then the back color and front color are both
  assigned the color computed using the front material with n."
* "An individual light is enabled or disabled by calling Enable or Disable with
  the symbolic value LIGHTi. **If light i is disabled, the ith term in the lighting
  equation is effectively removed from the summation.**"
* **Clamping** (§2.14.6): "After lighting (whether enabled or not), all components
  of both primary and secondary colors are clamped to the range [0, 1]."

### 1.7 Biến đổi normal (GL 2.1 §2.11.3)

Xem `docs/FIXED_FUNCTION_PIPELINE.md` §5.1 (đầy đủ công thức transform →
rescale → normalize).

**Tóm tắt phần liên quan lighting:**

```
n_eye = normalize(M_u⁻¹ · n_object)      // biến đổi, rồi rescale/normalize
```

* `GL_NORMALIZE` ⇒ `n_f` đơn vị.
* `GL_RESCALE_NORMAL` ⇒ nhân `f` (xem §5.1 của FIXED_FUNCTION_PIPELINE.md).
* **Thứ tự bắt buộc:** transform → rescale → normalize.
* "In case of an exactly singular matrix, the transformed normal is undefined. If
  the GL implementation determines that the model-view matrix is uninvertible, then
  the entries in the inverted matrix are arbitrary. In any case, neither normal
  transformation nor use of the transformed normal may lead to GL interruption or
  termination."

### 1.8 Transform vị trí nguồn sáng (GL 2.1 §2.14.2, nguyên văn)

> "The current **model-view matrix is applied to the position parameter** indicated
> with Light for a particular light source **when that position is specified**.
> These transformed values are the values used in the lighting equation. The
> **spotlight direction is transformed when it is specified using only the upper
> leftmost 3x3 portion of the model-view matrix**. That is, if Mu is the upper
> left 3x3 matrix taken from the current model-view matrix M, then the spotlight
> direction … is transformed to … = **Mu · d**."

⇒ **Hệ quả thiết kế (rất dễ sai):** `glLight(GL_LIGHT0, GL_POSITION, …)` phải
**transform ngay lúc gọi** bằng modelview **hiện tại**, và **lưu kết quả** (eye
space). Một `glTranslate` **sau** đó **không** di chuyển nguồn sáng đã đặt.
Tương tự, spot direction transform bằng **3×3 trên trái** (bỏ qua phần
translate/scale).

### 1.9 Số nguồn sáng

> "There are **n** light sources, indexed by i = 0, …, n−1. (**n is an
> implementation dependent maximum that must be at least 8.**)"

⇒ TGLMT phải hỗ trợ **≥ 8** nguồn sáng. `GL_LIGHT0`…`GL_LIGHT7` = 0x4000-0x4007.

---

## 2. SPEC — bảng state

### 2.1 State của nguồn sáng (bảng 2.11, §2.14.2)

| `glLight` pname | Ký hiệu | Số giá trị | Mặc định | Ghi chú |
|---|---|---:|---|---|
| `GL_AMBIENT` | `acli` | 4 | (0,0,0,1) | |
| `GL_DIFFUSE` | `dcli` | 4 | i=0: (1,1,1,1); i>0: (0,0,0,1) | |
| `GL_SPECULAR` | `scli` | 4 | i=0: (1,1,1,1); i>0: (0,0,0,1) | |
| `GL_POSITION` | `Ppli` | 4 | (0,0,1,0) | **object coords**; w=0 ⇒ directional; transform bằng modelview **lúc gọi** |
| `GL_SPOT_DIRECTION` | `sdli` | 3 | (0,0,−1) | transform bằng **3×3 trên trái** của modelview lúc gọi |
| `GL_SPOT_EXPONENT` | `srli` | 1 | 0.0 | range [0,128] |
| `GL_SPOT_CUTOFF` | `crli` | 1 | 180.0 | range [0,90] ∪ {180} |
| `GL_CONSTANT_ATTENUATION` | `k0i` | 1 | 1.0 | range [0,∞) |
| `GL_LINEAR_ATTENUATION` | `k1i` | 1 | 0.0 | range [0,∞) |
| `GL_QUADRATIC_ATTENUATION` | `k2i` | 1 | 0.0 | range [0,∞) |

### 2.2 State của material (bảng 2.11)

| `glMaterial` pname | Ký hiệu | Số giá trị | Mặc định (front & back) |
|---|---|---:|---|
| `GL_AMBIENT` | `acm` | 4 | (0.2, 0.2, 0.2, 1.0) |
| `GL_DIFFUSE` | `dcm` | 4 | (0.8, 0.8, 0.8, 1.0) |
| `GL_AMBIENT_AND_DIFFUSE` | `acm` + `dcm` | 4 | đặt cả hai |
| `GL_SPECULAR` | `scm` | 4 | (0.0, 0.0, 0.0, 1.0) |
| `GL_EMISSION` | `ecm` | 4 | (0.0, 0.0, 0.0, 1.0) |
| `GL_SHININESS` | `srm` | 1 | 0.0, range [0,128] |
| `GL_COLOR_INDEXES` | `am`,`dm`,`sm` | 3 | (0,1,1) — color-index mode đã gỡ ở 4.6 Core |

`face ∈ {GL_FRONT, GL_BACK, GL_FRONT_AND_BACK}`.

### 2.3 State của light model (`glLightModel`)

| pname | Ký hiệu | Số giá trị | Mặc định | Ghi chú |
|---|---|---:|---|---|
| `GL_LIGHT_MODEL_AMBIENT` | `acs` | 4 | (0.2,0.2,0.2,1) | màu ambient của cảnh |
| `GL_LIGHT_MODEL_LOCAL_VIEWER` | `vbs` | 1 | FALSE | TRUE ⇒ mắt ở (0,0,0) eye space |
| `GL_LIGHT_MODEL_TWO_SIDE` | `tbs` | 1 | FALSE | xem §1.6 |
| `GL_LIGHT_MODEL_COLOR_CONTROL` | `ces` | 1 | `GL_SINGLE_COLOR` | `GL_SEPARATE_SPECULAR_COLOR` |

### 2.4 Color material (GL 2.1 §2.14.3)

| `glColorMaterial` mode | Thành phần material theo current color |
|---|---|
| `GL_EMISSION` | `ecm` |
| `GL_AMBIENT` | `acm` |
| `GL_DIFFUSE` | `dcm` |
| `GL_SPECULAR` | `scm` |
| `GL_AMBIENT_AND_DIFFUSE` | `acm` **và** `dcm` |

`face ∈ {GL_FRONT, GL_BACK, GL_FRONT_AND_BACK}`; mặc định
`COLOR_MATERIAL_FACE = GL_FRONT_AND_BACK`, `COLOR_MATERIAL_PARAMETER =
GL_AMBIENT_AND_DIFFUSE`.

**Ghi chú quan trọng (spec §2.14.3):** "The replacements made to material
properties are **permanent**; the replaced values remain until changed by either
sending a new color or by setting a new material value when ColorMaterial is not
currently enabled to override that particular value."

⇒ **PLAN:** resolve ở CPU tại thời điểm `glVertex*` (xem
`docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.5.1 và
`docs/LEGACY_TO_METAL_MAPPING.md` §5.2 — "must be resolved at *vertex
specification time*").

### 2.5 Flat shading (GL 2.1 §2.14.7, bảng 2.12)

Xem `docs/FIXED_FUNCTION_PIPELINE.md` §5.4 (bảng vertex nguồn theo kiểu
primitive).

### 2.6 Lỗi (GL 2.1 §2.14.2 + bảng 2.3)

| Tình huống | Lỗi |
|---|---|
| `glLight`/`glMaterial`/`glLightModel` với `pname`/`light`/`face` không hợp lệ | `GL_INVALID_ENUM` |
| Tham số nằm ngoài khoảng cho phép (bảng 2.10): shininess ∉ [0,128], spot cutoff ∉ [0,90]∪{180}, spot exponent ∉ [0,128], attenuation < 0 | `GL_INVALID_VALUE` |
| `glLight` với `param` kiểu số cho một pname nhiều giá trị (hoặc ngược lại) | `GL_INVALID_ENUM` (spec: "If param corresponds to a multi-valued parameter, the error INVALID_ENUM results") |

---

## 3. SPEC — fog (GL 2.1 §3.10)

### 3.1 Vị trí áp dụng

> "If enabled, fog blends a fog color with a rasterized fragment's **post-texturing
> color** using a blending factor f."

Thứ tự đường fragment của GL 2.1 (theo thứ tự mục trong §3.11.2):

```
texture environment (§3.8.13) → texture application (§3.8.16)
    → color sum (§3.9) → FOG (§3.10)
    → [chapter 4] alpha test (§4.1.4) → blending (§4.2)
    → dithering (§4.3) → logic op (§4.4)
```

⇒ Fog nằm **sau** texturing, **trước** alpha test / blending.
Vì fog **không** đổi alpha, thứ tự fog ↔ alpha test không ảnh hưởng kết quả.

### 3.2 Ba công thức (nguyên văn)

```
(3.31)   GL_EXP     :  f = exp(−d · c)

(3.32)   GL_EXP2    :  f = exp(−(d · c)²)

(3.33)   GL_LINEAR  :  f = (e − c) / (e − s)
```

* `c` = fog coordinate
* `d` = `GL_FOG_DENSITY`
* `s` = `GL_FOG_START`
* `e` = `GL_FOG_END`

### 3.3 Nguồn của fog coordinate

> "If a vertex shader is active, or if the fog source … is **FOG_COORD**, then `c`
> is the **interpolated value of the fog coordinate** for this fragment. Otherwise,
> if the fog source is **FRAGMENT_DEPTH**, then `c` is the **eye-coordinate
> distance from the eye, (0, 0, 0, 1) in eye coordinates, to the fragment center**."

* `GL_FOG_COORD_SRC = GL_FRAGMENT_DEPTH` (mặc định): `c = |P_eye − (0,0,0)|`.
* `GL_FOG_COORD_SRC = GL_FOG_COORD`: `c` = giá trị `glFogCoord*` nội suy.

### 3.4 Kẹp và phép trộn

> "No matter which equation and approximation is used to compute f, the result is
> **clamped to [0, 1]** to obtain the final f."

> "In RGBA mode, if `Cr` represents a rasterized fragment's R, G, or B value, then
> the corresponding value produced by fog is **C = f·Cr + (1−f)·Cf**.
> (**The rasterized fragment's A value is not changed by fog blending.**)"

⇒ `Cf` = `GL_FOG_COLOR`; mỗi thành phần `Cf` được kẹp về [0,1] khi set.

### 3.5 State & lỗi

| `glFog` pname | Mặc định | Ghi chú |
|---|---|---|
| `GL_FOG_MODE` | `GL_EXP` | `GL_EXP`/`GL_EXP2`/`GL_LINEAR` |
| `GL_FOG_DENSITY` | 1.0 | `d`; **`d < 0` ⇒ `GL_INVALID_VALUE`** |
| `GL_FOG_START` | 0.0 | `s` |
| `GL_FOG_END` | 1.0 | `e` |
| `GL_FOG_COLOR` | (0,0,0,0) | `Cf`, kẹp [0,1] |
| `GL_FOG_COORD_SRC` | `GL_FRAGMENT_DEPTH` | |
| `GL_FOG_INDEX` | 0 | color-index mode — đã gỡ ở 4.6 Core |

* `GL_FOG` enable: mặc định **tắt**.
* `GL_FOG_HINT`: hint, mặc định `GL_DONT_CARE`; "An implementation may ignore them
  entirely."
* "If `e` is selected, `s = e`, results are **undefined**." (GL_LINEAR mà
  start == end).
* "An implementation may choose to approximate the eye-coordinate distance from
  the eye to each fragment center by **|ze|**." — TGLMT có thể dùng cách này
  (rẻ hơn nhiều trên A11) nhưng phải ghi là **sai lệch được phép**.

### 3.6 Vị trí trong shader sinh (PLAN)

Theo `docs/LEGACY_TO_METAL_MAPPING.md` §6:

```metal
    // --- sau bước texture environment ---
    float c = (u.fogCoordSrc == GL_FOG_COORD) ? in.fogCoord : length(in.eyePos);
    float f;
    if      (u.fogMode == 1 /*LINEAR*/) f = (u.fogEnd - c) / (u.fogEnd - u.fogStart);
    else if (u.fogMode == 2 /*EXP*/)   f = exp(-u.fogDensity * c);
    else                               f = exp(-(u.fogDensity * c) * (u.fogDensity * c));
    f = clamp(f, 0.0, 1.0);
    color.rgb = f * color.rgb + (1.0 - f) * u.fogColor.rgb;   // alpha KHÔNG đổi
```

---

## 4. SPEC — texture environment (GL 2.1 §3.8.13)

### 4.1 Tham số

`target ∈ {GL_TEXTURE_ENV, GL_TEXTURE_FILTER_CONTROL, GL_POINT_SPRITE}`.

* `GL_TEXTURE_FILTER_CONTROL` ⇒ chỉ `GL_TEXTURE_LOD_BIAS` (float, mặc định 0.0).
* `GL_POINT_SPRITE` ⇒ `GL_POINT_SPRITE_COORD_ORIGIN` (GL 1.4).
* `GL_TEXTURE_ENV` ⇒ các pname bên dưới.

| pname | Kiểu | Mặc định (bảng 6.19) |
|---|---|---|
| `GL_TEXTURE_ENV_MODE` | enum | `GL_MODULATE` |
| `GL_TEXTURE_ENV_COLOR` | RGBA | (0,0,0,0) |
| `GL_COMBINE_RGB` | enum | `GL_MODULATE` |
| `GL_COMBINE_ALPHA` | enum | `GL_MODULATE` |
| `GL_SOURCE0_RGB` / `GL_SOURCE1_RGB` / `GL_SOURCE2_RGB` | enum | `GL_TEXTURE` / `GL_PREVIOUS` / `GL_CONSTANT` |
| `GL_SOURCE0_ALPHA` / `GL_SOURCE1_ALPHA` / `GL_SOURCE2_ALPHA` | enum | `GL_TEXTURE` / `GL_PREVIOUS` / `GL_CONSTANT` |
| `GL_OPERAND0_RGB` / `GL_OPERAND1_RGB` / `GL_OPERAND2_RGB` | enum | `GL_SRC_COLOR` / `GL_SRC_COLOR` / `GL_SRC_ALPHA` |
| `GL_OPERAND0_ALPHA` / `GL_OPERAND1_ALPHA` / `GL_OPERAND2_ALPHA` | enum | `GL_SRC_ALPHA` (cả 3) |
| `GL_RGB_SCALE` | float | 1.0 (range [1,4]) |
| `GL_ALPHA_SCALE` | float | 1.0 (range [1,4]) |

> **Lưu ý tên enum:** GL 2.1 dùng `GL_SRC0_RGB`/`GL_SRC1_RGB`/`GL_SRC2_RGB`
> (tên GL 1.3) — trong registry Khronos, `GL_SRC0_RGB` và `GL_SOURCE0_RGB` là
> **cùng một giá trị** 0x8580 (`docs/khronos/gl.xml`). TGLMT sinh header từ
> `gl.xml` nên mang tên `GL_SOURCE0_RGB` (`include/tglmt/gl_legacy_types.h`).

### 4.2 Ký hiệu (nguyên văn §3.8.13)

| Ký hiệu | Nghĩa |
|---|---|
| `Cf`, `Af` | primary color của fragment đi vào |
| `Cs`, `As` | texture source color (lấy theo bảng 3.20) |
| `Cc`, `Ac` | texture environment color (`GL_TEXTURE_ENV_COLOR`) |
| `Cp`, `Ap` | kết quả của texture environment **trước đó** (với unit 0: `Cp,Ap = Cf,Af`) |
| `Cv`, `Av` | kết quả của texture function |

### 4.3 Bảng 3.20 — ánh xạ internal format → (Cs, As)

| Base internal format | `Cs` | `As` |
|---|---|---|
| `GL_ALPHA` | (0,0,0) | `At` |
| `GL_LUMINANCE` | (Lt,Lt,Lt) | 1 |
| `GL_LUMINANCE_ALPHA` | (Lt,Lt,Lt) | `At` |
| `GL_INTENSITY` | (It,It,It) | `It` |
| `GL_RGB` | (Rt,Gt,Bt) | 1 |
| `GL_RGBA` | (Rt,Gt,Bt) | `At` |

### 4.4 Bảng 3.21 — `GL_REPLACE`, `GL_MODULATE`, `GL_DECAL`

| Base format | `GL_REPLACE` | `GL_MODULATE` | `GL_DECAL` |
|---|---|---|---|
| `GL_ALPHA` | `Cv = Cp`; `Av = As` | `Cv = Cp`; `Av = Ap·As` | **undefined** |
| `GL_LUMINANCE` | `Cv = Cs`; `Av = Ap` | `Cv = Cp·Cs`; `Av = Ap` | **undefined** |
| `GL_LUMINANCE_ALPHA` | `Cv = Cs`; `Av = As` | `Cv = Cp·Cs`; `Av = Ap·As` | **undefined** |
| `GL_INTENSITY` | `Cv = Cs`; `Av = As` | `Cv = Cp·Cs`; `Av = Ap·As` | **undefined** |
| `GL_RGB` | `Cv = Cs`; `Av = Ap` | `Cv = Cp·Cs`; `Av = Ap` | `Cv = Cs`; `Av = Ap` |
| `GL_RGBA` | `Cv = Cs`; `Av = As` | `Cv = Cp·Cs`; `Av = Ap·As` | `Cv = Cp(1−As) + Cs·As`; `Av = Ap` |

### 4.5 Bảng 3.22 — `GL_BLEND`, `GL_ADD`

| Base format | `GL_BLEND` | `GL_ADD` |
|---|---|---|
| `GL_ALPHA` | `Cv = Cp`; `Av = Ap·As` | `Cv = Cp`; `Av = Ap·As` |
| `GL_LUMINANCE` | `Cv = Cp(1−Cs) + Cc·Cs`; `Av = Ap` | `Cv = Cp + Cs`; `Av = Ap` |
| `GL_LUMINANCE_ALPHA` | `Cv = Cp(1−Cs) + Cc·Cs`; `Av = Ap·As` | `Cv = Cp + Cs`; `Av = Ap·As` |
| `GL_INTENSITY` | `Cv = Cp(1−Cs) + Cc·Cs`; `Av = Ap(1−As) + Ac·As` | `Cv = Cp + Cs`; `Av = Ap + As` |
| `GL_RGB` | `Cv = Cp(1−Cs) + Cc·Cs`; `Av = Ap` | `Cv = Cp + Cs`; `Av = Ap` |
| `GL_RGBA` | `Cv = Cp(1−Cs) + Cc·Cs`; `Av = Ap·As` | `Cv = Cp + Cs`; `Av = Ap + As` |

**Ghi chú:** `GL_DECAL` **undefined** với các base format không có alpha
(`ALPHA`, `LUMINANCE`, `LUMINANCE_ALPHA`, `INTENSITY`). TGLMT nên chọn một hành vi
xác định (ví dụ `GL_MODULATE`) và ghi là **sai lệch**.

### 4.6 Bảng 3.23 — `GL_COMBINE`

**COMBINE_RGB:**

| Combine function | Công thức |
|---|---|
| `GL_REPLACE` | `Arg0` |
| `GL_MODULATE` | `Arg0 · Arg1` |
| `GL_ADD` | `Arg0 + Arg1` |
| `GL_ADD_SIGNED` | `Arg0 + Arg1 − 0.5` |
| `GL_INTERPOLATE` | `Arg0 · Arg2 + Arg1 · (1 − Arg2)` |
| `GL_SUBTRACT` | `Arg0 − Arg1` |
| `GL_DOT3_RGB` | `4 × ((Arg0r−0.5)(Arg1r−0.5) + (Arg0g−0.5)(Arg1g−0.5) + (Arg0b−0.5)(Arg1b−0.5))` |
| `GL_DOT3_RGBA` | như `DOT3_RGB` (dùng 3 thành phần RGB) |

**COMBINE_ALPHA:**

| Combine function | Công thức |
|---|---|
| `GL_REPLACE` | `Arg0` |
| `GL_MODULATE` | `Arg0 · Arg1` |
| `GL_ADD` | `Arg0 + Arg1` |
| `GL_ADD_SIGNED` | `Arg0 + Arg1 − 0.5` |
| `GL_INTERPOLATE` | `Arg0 · Arg2 + Arg1 · (1 − Arg2)` |
| `GL_SUBTRACT` | `Arg0 − Arg1` |

> "The scalar expression computed for the DOT3_RGB and DOT3_RGBA functions is
> placed into each of the 3 (RGB) or 4 (RGBA) components of the output. **The
> result generated from COMBINE_ALPHA is ignored for DOT3_RGBA.**"

### 4.7 Bảng 3.24 — tham số của COMBINE_RGB

| `GL_SRCn_RGB` | `GL_OPERANDn_RGB` | Argument |
|---|---|---|
| `GL_TEXTURE` | `GL_SRC_COLOR` | `Cs` |
| `GL_TEXTURE` | `GL_ONE_MINUS_SRC_COLOR` | `1 − Cs` |
| `GL_TEXTURE` | `GL_SRC_ALPHA` | `As` |
| `GL_TEXTURE` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − As` |
| `GL_TEXTUREn` | `GL_SRC_COLOR` | `Csn` |
| `GL_TEXTUREn` | `GL_ONE_MINUS_SRC_COLOR` | `1 − Csn` |
| `GL_TEXTUREn` | `GL_SRC_ALPHA` | `Asn` |
| `GL_TEXTUREn` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Asn` |
| `GL_CONSTANT` | `GL_SRC_COLOR` | `Cc` |
| `GL_CONSTANT` | `GL_ONE_MINUS_SRC_COLOR` | `1 − Cc` |
| `GL_CONSTANT` | `GL_SRC_ALPHA` | `Ac` |
| `GL_CONSTANT` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Ac` |
| `GL_PRIMARY_COLOR` | `GL_SRC_COLOR` | `Cf` |
| `GL_PRIMARY_COLOR` | `GL_ONE_MINUS_SRC_COLOR` | `1 − Cf` |
| `GL_PRIMARY_COLOR` | `GL_SRC_ALPHA` | `Af` |
| `GL_PRIMARY_COLOR` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Af` |
| `GL_PREVIOUS` | `GL_SRC_COLOR` | `Cp` |
| `GL_PREVIOUS` | `GL_ONE_MINUS_SRC_COLOR` | `1 − Cp` |
| `GL_PREVIOUS` | `GL_SRC_ALPHA` | `Ap` |
| `GL_PREVIOUS` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Ap` |

### 4.8 Bảng 3.25 — tham số của COMBINE_ALPHA

| `GL_SRCn_ALPHA` | `GL_OPERANDn_ALPHA` | Argument |
|---|---|---|
| `GL_TEXTURE` | `GL_SRC_ALPHA` | `As` |
| `GL_TEXTURE` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − As` |
| `GL_TEXTUREn` | `GL_SRC_ALPHA` | `Asn` |
| `GL_TEXTUREn` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Asn` |
| `GL_CONSTANT` | `GL_SRC_ALPHA` | `Ac` |
| `GL_CONSTANT` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Ac` |
| `GL_PRIMARY_COLOR` | `GL_SRC_ALPHA` | `Af` |
| `GL_PRIMARY_COLOR` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Af` |
| `GL_PREVIOUS` | `GL_SRC_ALPHA` | `Ap` |
| `GL_PREVIOUS` | `GL_ONE_MINUS_SRC_ALPHA` | `1 − Ap` |

### 4.9 Quy tắc tổng hợp của texture function

> "If the value of TEXTURE_ENV_MODE is COMBINE, the form of the texture function
> depends on the values of COMBINE_RGB and COMBINE_ALPHA … **The RGB and ALPHA
> results of the texture function are then multiplied by the values of RGB_SCALE
> and ALPHA_SCALE**, respectively. **The results are clamped to [0, 1]**."

⇒ `GL_COMBINE` chỉ dùng cho RGB/A theo bảng 3.23; **không** áp `GL_MODULATE`
mặc định.

**Multi-texturing:** unit 0 dùng `Cf/Af` làm `Cp/Ap`; kết quả của unit *i* trở
thành `Cp/Ap` của unit *i+1* (spec §3.8.13: "for texture environment 0, Cp and Ap
are identical to Cf and Af").

**Điểm dễ sai:** `GL_COMBINE` là **duy nhất** dùng `GL_RGB_SCALE`/`GL_ALPHA_SCALE`;
5 mode cơ bản **không** scale.

---

## 5. SPEC — texture coordinate generation (GL 2.1 §2.11.4)

### 5.1 Tham số

`coord ∈ {GL_S, GL_T, GL_R, GL_Q}`; `pname ∈ {GL_TEXTURE_GEN_MODE, GL_OBJECT_PLANE,
GL_EYE_PLANE}`.

Mode: `GL_OBJECT_LINEAR`, `GL_EYE_LINEAR`, `GL_SPHERE_MAP`, `GL_REFLECTION_MAP`,
`GL_NORMAL_MAP`.

Bật/tắt từng toạ độ: `glEnable/glDisable` với `GL_TEXTURE_GEN_S` (0x0C60),
`GL_TEXTURE_GEN_T` (0x0C61), `GL_TEXTURE_GEN_R` (0x0C62), `GL_TEXTURE_GEN_Q`
(0x0C63).

### 5.2 `GL_OBJECT_LINEAR`

> "g = p1·xo + p2·yo + p3·zo + p4·wo"

`xo,yo,zo,wo` = **object coordinates** của đỉnh. `p1..p4` set bằng
`GL_OBJECT_PLANE`.

### 5.3 `GL_EYE_LINEAR`

> "g = p′1·xe + p′2·ye + p′3·ze + p′4·we
> where **(p′1 p′2 p′3 p′4) = (p1 p2 p3 p4) · M⁻¹**"

`xe,ye,ze,we` = **eye coordinates**. `p1..p4` set bằng `GL_EYE_PLANE`.
**`M` là model-view matrix *tại thời điểm* `p1..p4` được chỉ định.**

⇒ Hệ quả (rất dễ sai): `glTexGen(GL_S, GL_EYE_PLANE, …)` phải **biến đổi hệ số
ngay lúc gọi** bằng `M⁻¹` hiện tại, và **lưu kết quả** (`p′`), **không** giữ
`p` để nhân sau. Một `glTranslate` sau đó không làm đổi texgen.

### 5.4 `GL_SPHERE_MAP`

> "Denote the unit vector pointing from the origin to the vertex (**in eye
> coordinates**) by **u**. Denote the current normal, **after transformation to eye
> coordinates**, by **nf**. Let **r = (rx ry rz)ᵀ**, the reflection vector, be
> given by **r = u − 2·nfᵀ·(nf·u)**, and let **m = 2·√(rx² + ry² + (rz + 1)²)**.
> Then the value assigned to an s coordinate is **s = rx/m + ½**; the value assigned
> to a t coordinate is **t = ry/m + ½**. Calling TexGen with a coord of either R or
> Q when pname indicates SPHERE_MAP generates the error **INVALID_ENUM**."

⚠ **Công thức `m` có căn bậc hai.** `docs/LEGACY_TO_METAL_MAPPING.md` §8.2 viết
`m = 2.0*r.x*r.x + r.y*r.y + (r.z+1.0)*(r.z+1.0)` — **thiếu `√`** ⇒ sai.
Công thức đúng: `m = 2 * sqrt(rx*rx + ry*ry + (rz+1)*(rz+1))`.

### 5.5 `GL_REFLECTION_MAP`

> "compute the reflection vector r as described for the SPHERE_MAP mode. Then the
> value assigned to an s coordinate is **s = rx**; the value assigned to a t
> coordinate is **t = ry**; and the value assigned to an r coordinate is **r = rz**.
> Calling TexGen with a coord of Q when pname indicates REFLECTION_MAP generates
> the error **INVALID_ENUM**."

### 5.6 `GL_NORMAL_MAP`

> "compute the normal vector nf as described in section 2.11.3. Then the value
> assigned to an s coordinate is **s = nfx**; the value assigned to a t coordinate
> is **t = nfy**; and the value assigned to an r coordinate is **r = nfz**.
> Calling TexGen with a coord of Q when pname indicates NORMAL_MAP generates the
> error **INVALID_ENUM**."

(`GL_NORMAL_MAP` là GL 1.4.)

### 5.7 Trạng thái & mặc định (nguyên văn §2.11.4)

> "The initial state has the texture generation function **disabled for all**
> texture coordinates. The initial values of pi for **s** are all 0 except **p1 which
> is one**; for **t** all the pi are zero except **p2, which is 1**. The values of
> pi for **r** and **q** are all 0. These values of pi apply for **both** the EYE
> LINEAR and OBJECT LINEAR versions. **Initially all texture generation modes are
> EYE LINEAR.**"

⇒ Mặc định:

| coord | `p1` | `p2` | `p3` | `p4` | mode mặc định | enable mặc định |
|---|---:|---:|---:|---:|---|---|
| S | **1** | 0 | 0 | 0 | `GL_EYE_LINEAR` | FALSE |
| T | 0 | **1** | 0 | 0 | `GL_EYE_LINEAR` | FALSE |
| R | 0 | 0 | 0 | 0 | `GL_EYE_LINEAR` | FALSE |
| Q | 0 | 0 | 0 | 0 | `GL_EYE_LINEAR` | FALSE |

### 5.8 Thứ tự áp dụng

> "A texture coordinate generation function is enabled or disabled using Enable and
> Disable with an argument of TEXTURE_GEN_S, TEXTURE_GEN_T, TEXTURE_GEN_R, or
> TEXTURE_GEN_Q. When enabled, the specified texture coordinate is computed
> according to the current EYE LINEAR, OBJECT LINEAR or SPHERE_MAP specification,
> depending on the current setting of TEXTURE_GEN_MODE for that coordinate. **When
> disabled, subsequent vertices will take the indicated texture coordinate from the
> current texture coordinates.**"

⇒ Texgen **ghi đè** current texcoord khi bật; khi tắt thì current texcoord được
dùng như bình thường.

### 5.9 Tương tác với texture matrix

Texture matrix (`GL_TEXTURE_MATRIX`) nhân **sau** khi texgen sinh ra toạ độ:
`tc_final = M_texture · tc_generated` (xem
`docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.2.2).

---

## 6. PLAN — thiết kế implement

### 6.1 State mới (trong `Context::legacy`, theo D-004)

```cpp
// include/tglmt/LegacyState.h (mở rộng)
constexpr int kMaxLegacyLights = 8;     // spec tối thiểu 8
constexpr int kMaxLegacyTexUnits = 8;   // theo LegacyState::currentTexCoord[8]

struct LegacyLight {
    float ambient[4];      // acli
    float diffuse[4];      // dcli
    float specular[4];     // scli
    float position[4];     // Ppli  — ĐÃ transform sang eye space lúc glLight
    float spotDir[3];      // sdli  — ĐÃ transform bằng 3x3 lúc glLight
    float spotExponent;    // srli
    float spotCutoff;      // crli  (độ)
    float k0, k1, k2;      // attenuation
    bool  enabled;         // GL_LIGHTi
};

struct LegacyMaterial {
    float ambient[4];   // acm
    float diffuse[4];   // dcm
    float specular[4];  // scm
    float emission[4];  // ecm
    float shininess;    // srm
};

struct LegacyLightModel {
    float   ambient[4];        // acs
    bool    localViewer;       // vbs
    bool    twoSide;           // tbs
    GLenum  colorControl;      // ces
};

struct LegacyFog {
    GLenum  mode;          // GL_EXP | GL_EXP2 | GL_LINEAR
    float   density;       // d
    float   start;         // s
    float   end;           // e
    float   color[4];      // Cf
    GLenum  coordSrc;      // GL_FRAGMENT_DEPTH | GL_FOG_COORD
};

struct LegacyTexEnv {
    GLenum  mode;            // GL_MODULATE|REPLACE|DECAL|BLEND|ADD|COMBINE
    float   color[4];        // Cc
    GLenum  combineRGB;      // GL_REPLACE|MODULATE|ADD|ADD_SIGNED|INTERPOLATE|SUBTRACT|DOT3_RGB|DOT3_RGBA
    GLenum  combineAlpha;    // ...
    GLenum  srcRGB[3], srcAlpha[3];
    GLenum  operandRGB[3], operandAlpha[3];
    float   rgbScale, alphaScale;
    float   lodBias;         // GL_TEXTURE_FILTER_CONTROL
};

struct LegacyTexGen {
    GLenum  mode[4];        // theo coord S,T,R,Q
    float   objectPlane[4][4];  // p1..p4
    float   eyePlane[4][4];    // p′1..p′4 (ĐÃ nhân M⁻¹ lúc glTexGen)
    bool    enabled[4];
};
```

### 6.2 Nơi transform (bắt buộc — dễ sai nhất)

| State | Khi nào transform | Bằng gì | Lưu ở đâu |
|---|---|---|---|
| `GL_POSITION` của light | **ngay lúc `glLight`** | modelview **4×4** đầy đủ | eye space |
| `GL_SPOT_DIRECTION` của light | **ngay lúc `glLight`** | modelview **3×3 trên trái** | eye space |
| `GL_EYE_PLANE` của texgen | **ngay lúc `glTexGen`** | `(p1..p4) · M⁻¹` | eye space (`p′`) |
| `GL_OBJECT_PLANE` của texgen | không transform | — | object space |
| Normal của đỉnh | mỗi đỉnh, trong vertex shader | normal matrix (inverse-transpose 3×3) | eye space |

⇒ `glLight`/`glTexGen` cần **truy cập ma trận hiện tại** của `Context::legacy`
(modelview/projection/texture/color) — tức là phải đặt code trong
`src/gl/gl_legacy_*.cpp` để đọc được `c.legacy.modelview.top()`.

### 6.3 Phân loại state → uniform / hằng số biên dịch / pipeline key

Xem bảng đầy đủ ở `docs/FIXED_FUNCTION_PIPELINE.md` §3.2. Tóm tắt riêng cho nhóm
này:

| State | Điểm đến |
|---|---|
| Tham số light (màu, attenuation, spot) | **uniform** |
| Vị trí/spot direction của light (đã transform) | **uniform** |
| Material front/back | **uniform** |
| Light model (acs, vbs, tbs, ces) | `acs` là **uniform**; `vbs`/`tbs`/`ces` là **hằng số biên dịch** |
| `GL_LIGHTING` | **hằng số biên dịch** (`key.lighting`) |
| `GL_SHADE_MODEL` | **hằng số biên dịch** (`key.shadeModel`) |
| Số đèn bật | **uniform** (`numLights`) — **không** đưa vào key |
| Fog mode/density/start/end/color/coordSrc | **uniform** (trừ `coordSrc` là hằng số biên dịch vì đổi layout) |
| Texenv mode/color/combine args/scale | **uniform** |
| Texgen mode | **hằng số biên dịch** (`key.texGen`) |
| `GL_COLOR_MATERIAL` + face + parameter | resolve ở **CPU**; mask trong **hằng số biên dịch** nếu còn giữ material dạng uniform |

### 6.4 Vị trí trong pipeline sinh

```
vertex stage:  transform → normal transform (rescale/normalize) → lighting (per-vertex)
fragment stage: texture env (per unit) → color sum → FOG → [chapter 4] alpha test → blend
```

Lưu ý (spec §2.14.1): lighting **per-vertex**. Nếu `tbs = TRUE` thì facing là
per-fragment ⇒ phải chọn giữa (a) đánh giá per-fragment bằng `[[front_facing]]`
(sai lệch có chủ đích, `docs/LEGACY_TO_METAL_MAPPING.md` §5.2) hoặc (b) truyền
thêm attribute cho đỉnh nguồn flat.

---

## 7. Lộ trình implement

| Stage | Nội dung | Entry point | File | Test ID | Exit criteria |
|---|---|---|---|---|---|
| **L0** | **State skeleton** (light/material/lightModel/fog/texEnv/texGen) + `glGet*` round-trip | — | `include/tglmt/LegacyState.h`, `src/gl/gl_legacy_lighting.cpp` (mới) | `test_legacy_lighting` (mới) | `glGetLightfv(GL_LIGHT0, GL_POSITION)` trả (0,0,1,0); `glGetMaterialfv(GL_FRONT, GL_AMBIENT)` trả (0.2,0.2,0.2,1); `glGetLightModeliv(GL_LIGHT_MODEL_COLOR_CONTROL)` trả `GL_SINGLE_COLOR` |
| **L1** | **`glLight*` / `glLightModel*`** (8 lệnh) + transform vị trí/spot lúc gọi + enable `GL_LIGHTi` | `glLight{f,i,v,fv,iv}`, `glLightModel{f,i,v,fv,iv}` | `src/gl/gl_legacy_lighting.cpp` | `test_legacy_lighting` | Đặt `GL_POSITION` rồi `glTranslate` ⇒ query trả **giá trị đã transform**; spot direction dùng 3×3 (bỏ translate) |
| **L2** | **`glMaterial*` + `glColorMaterial` + `glShadeModel`** (6 lệnh) | `glMaterial{f,i,v,fv,iv}`, `glColorMaterial`, `glShadeModel` | `src/gl/gl_legacy_lighting.cpp` | `test_legacy_lighting` | Mặc định đúng bảng 2.10; `glColorMaterial(FRONT, AMBIENT)` + `glColor` ⇒ material ambient đuổi theo color; `glShadeModel(GL_FLAT)` lưu đúng |
| **L3** | **Fog** (4 lệnh) + `glFogCoord*` (4 lệnh) | `glFog{f,i,v,fv}`, `glFogCoord{f,d,fv,dv}` | `src/gl/gl_legacy_fog.cpp` (mới) | `test_legacy_fog` (mới) | 3 mode đúng công thức §3.2; `d < 0` ⇒ `INVALID_VALUE`; `glFogCoord` cập nhật current fog coord |
| **L4** | **Texenv** (6 lệnh) + **texgen** (9 lệnh) | `glTexEnv{f,i,v,fv}`, `glGetTexEnv{fv,iv}`, `glTexGen{f,i,d,v,fv,iv}`, `glGetTexGen{fv,iv}`, `glGetTexGendv` | `src/gl/gl_legacy_texenv.cpp` (mới) | `test_legacy_texenv` (mới) | Mặc định đúng bảng 6.19; `GL_SPHERE_MAP` với coord R/Q ⇒ `INVALID_ENUM`; eye plane đã transform lúc gọi |
| **L5** | **Shader sinh** cho lighting + fog + texenv + texgen | — | `src/gl/gl_legacy_program.cpp` (mới) | `test_legacy_lighting_apple` (mới, GPU) | Ảnh khớp case tính tay theo §1.3; fog đúng vị trí §3.1 |
| **L6** | **`GL_COLOR_MATERIAL` resolve ở CPU** + flat shading theo bảng 2.12 | — | `src/gl/gl_legacy_vertex.cpp` (`AppendVertex`) | `test_legacy_immediate` (mở rộng) | Material đuổi theo current color tại thời điểm `glVertex*`; flat color đúng vertex nguồn |
| **L7** | **Hợp nhất bóng EXT_dsa_state** với `glTexEnv*`/`glTexGen*` thật | `glMultiTexEnv*EXT`, `glMultiTexGen*EXT` | `src/gl/gl_ext_dsa_state.cpp` | `test_legacy_texenv` | `glMultiTexEnvfEXT(GL_TEXTURE1, …)` và `glTexEnvf(GL_TEXTURE_ENV, …)` (sau `glActiveTexture(GL_TEXTURE1)`) cho **cùng** kết quả |

**Ràng buộc chung:**

1. State mới nằm trong `Context::legacy` (D-004), **không** trong `StateTracker`.
2. Mọi pname mới phải được thêm vào `tools/gen_state_pnames.py` (sinh
   `include/tglmt/gl46_pnames.h`) — hiện trong 964 pname được chấp nhận **không**
   có pname nào thuộc nhóm *fixed-function-only* (lighting, fog, texenv/texgen):
   đo **0/964**; mọi query nhóm đó phải đi qua `case` riêng.
3. Enum lấy từ `include/tglmt/gl_legacy_types.h`; **không** literal.
4. `glLight*`/`glMaterial*`/`glFog*`/`glTexEnv*`/`glTexGen*` là COMPAT-ONLY ⇒ đã
   có version-gate ở lớp C ABI (`docs/DECISIONS.md` D-003).

---

## 8. Điểm chưa verify

| # | Điểm | Vì sao chưa verify |
|---|---|---|
| V-1 | Công thức lighting trên GPU | Chưa có chương trình sinh ⇒ mới chỉ đối chiếu văn bản `glspec21.pdf` |
| V-2 | Độ chính xác của `f_i` khi normal vuông góc | Ranh giới `n ⊙ →V P_li = 0` ⇒ `f_i = 0`; cần test pixel |
| V-3 | `GL_LINEAR` fog với `start == end` | Spec: **undefined**; cần chọn hành vi xác định (`docs/LEGACY_TO_METAL_MAPPING.md` §6 đề xuất hard step) |
| V-4 | `GL_DECAL` với base format không có alpha | Spec: **undefined**; cần chọn hành vi xác định |
| V-5 | `GL_FOG_COORD_SRC = GL_FOG_COORD` | Cần client attribute `glFogCoord*` ⇒ đổi vertex layout; chưa thiết kế attribute slot |
| V-6 | `GL_NORMAL_MAP` (GL 1.4) | Chưa có trong kế hoạch stage; cần thêm nếu app dùng |
| V-7 | Số texture unit legacy hỗ trợ | 8 (theo `LegacyState.h:136`) vs `GL_MAX_TEXTURE_UNITS` TGLMT báo — lệch |
| V-8 | Nhãn môi trường `A11_DEVICE` | Toàn bộ đo chạy Null backend |
| V-9 | Tương thích với `EXT_direct_state_access` khi `glTexEnv*` thật được implement | Chưa quyết định (stage L7) |

---

## 9. Tham chiếu chéo

* `docs/FIXED_FUNCTION_PIPELINE.md` — bảng state, §5 công thức normal/shade model/color material/two-sided.
* `docs/IMMEDIATE_MODE.md` — `glMaterial` được phép trong `glBegin/glEnd`; assembler.
* `docs/DISPLAY_LISTS.md` — client/server state, resource dependency.
* `docs/LEGACY_TO_METAL_MAPPING.md` §5 (lighting), §6 (fog), §8 (texenv/texgen) — chiến lược Metal; **lưu ý lỗi căn bậc hai ở §8.2**.
* `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.2 — chương trình MSL sinh, uniform block, program key.
* `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` §1 (S4, S5, S6) — quy ước stage & exit criteria.
* `docs/DECISIONS.md` D-004 (tách state), D-005 (enum).
