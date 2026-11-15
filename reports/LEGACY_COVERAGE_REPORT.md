# LEGACY_COVERAGE_REPORT.md — Coverage OpenGL 1.0–2.1 / Compatibility fixed-function (TGLMT)

> **Mọi số liệu đo bằng script**, không viết tay. Lệnh đo kèm theo từng bảng.
>
> **Phạm vi:** bề mặt **legacy fixed-function** của OpenGL Desktop 1.0–2.1 và
> **Compatibility Profile** 3.2–4.6 — tức **391** entry point mà registry
> Khronos định nghĩa là *có trong compatibility nhưng không có trong core*
> (`docs/OPENGL_PROFILE_MATRIX.md` §3.3).
>
> **Nhãn môi trường:** `OTHER_GPU` (Intel UHD 630, macOS 15.7.7) và
> `REFERENCE_ONLY` (backend Null). **KHÔNG phải `A11_DEVICE`.** Xem
> `reports/FULL_AUDIT_REPORT.md` §0.

---

## 0. Nguồn chân lý và cách đo

| Input | Vai trò |
|---|---|
| `docs/khronos/gl.xml` (vendored) | nguồn chân lý duy nhất cho API/enum/profile |
| `data/version_inventory.json` | inventory máy đọc được theo (version × profile) |
| `docs/OPENGL_PROFILE_MATRIX.md` §3.4 | bảng rule **L01–L20** (first-match-wins) |
| `src/gl/gl_stubs_gen.cpp` | nguồn của 240 stub trung thực (`UNIMPLEMENTED: glXxx`) |
| `src/gl/*.cpp` (viết tay) | nguồn của định nghĩa thật |
| `tests/**/*.cpp` + `ctest -V` | bằng chứng test |

> **Script đo:** bản script tự chứa được nhúng đầy đủ ở
> `reports/FULL_AUDIT_REPORT.md` §13 — trích ra
> `/tmp/dod_measure.py` rồi chạy như dưới đây. Không cần file ngoài repo.

Lệnh đo chính (dùng lại cho cả 3 báo cáo):

```bash
cd /Volumes/project/TGLMT
(cd build-work && ctest -j8 -V > /tmp/tglmt_ctest_v.log 2>&1)
python3 /tmp/dod_measure.py --ctest-log /tmp/tglmt_ctest_v.log \
        --out /tmp/tglmt_dod_rows.json
```

`dod_full.py` phân loại thân hàm bằng **đúng** bộ của
`tools/truth_manifest.py` (`real` / `noop` / `constant` / `alias` / `missing`),
loại 5 file sinh tự động, và **chỉ tính** một lệnh là "có test" khi test
**gọi thật** (`glXxx(`) **và PASS** **và** có assertion giá trị neo theo spec.

---

## 1. Bảng theo 20 nhóm L01–L20

Định nghĩa nhóm: `docs/OPENGL_PROFILE_MATRIX.md` §3.3 (bảng) và §3.4 (script).

| Nhóm | Nhãn | Lệnh | Implement thật | Stub | Test ID | Đạt §14.1 | Trạng thái |
|---|---|---:|---:|---:|---|---:|---|
| **L01** | Immediate mode (Begin/End + vertex specification) | 173 | **88** | 85 | `test_legacy_immediate` (7 lệnh), `test_context_version` (1) | **7** | `PARTIALLY_IMPLEMENTED` |
| **L02** | Client-side vertex arrays | 12 | **0** | 12 | — | 0 | `MISSING` |
| **L03** | Fixed-function transform + matrix stack | 20 | **20** | 0 | `test_legacy_matrix` (9 lệnh) | **9** | `PARTIALLY_IMPLEMENTED` |
| **L04** | Lighting, materials, shading model | 18 | **0** | 18 | — | 0 | `MISSING` |
| **L05** | Fog | 4 | **0** | 4 | — | 0 | `MISSING` |
| **L06** | Texture env + texture-coordinate generation | 15 | **0** | 15 | — | 0 | `MISSING` |
| **L07** | Display lists | 8 | **0** | 8 | — | 0 | `MISSING` |
| **L08** | Evaluators | 23 | **0** | 23 | — | 0 | `MISSING` |
| **L09** | Selection and feedback modes | 8 | **0** | 8 | — | 0 | `MISSING` |
| **L10** | Accumulation buffer | 2 | **0** | 2 | — | 0 | `MISSING` |
| **L11** | Pixel transfer modes + pixel drawing | 12 | **0** | 12 | — | 0 | `MISSING` |
| **L12** | Raster position + window position | 40 | **0** | 40 | — | 0 | `MISSING` |
| **L13** | Stipple (polygon + line) | 3 | **0** | 2 | — | 0 | `STUB` (`glLineStipple` là no-op) |
| **L14** | Alpha test | 1 | **0** | 1 | — | 0 | `MISSING` |
| **L15** | Texture residency + priority | 2 | **0** | 2 | — | 0 | `MISSING` |
| **L16** | User clipping planes | 2 | **0** | 2 | — | 0 | `MISSING` |
| **L17** | Attribute stacks | 4 | **0** | 4 | — | 0 | `MISSING` |
| **L18** | Colour-index mode state | 2 | **0** | 2 | — | 0 | `MISSING` |
| **L19** | Compat-only vertex-type entry points (GL 3.3) | 30 | **30** | 0 | `test_packed_vertex_types` (12), `test_readpixels_packed` (3) | **12** | `PARTIALLY_IMPLEMENTED` |
| **L20** | Compat-only n-suffixed queries (GL 4.5) | 12 | **12** | 0 | — | **0** | `PARTIALLY_IMPLEMENTED` (code có, **không test nào gọi**) |
| | **TỔNG** | **391** | **150** | **240** | 3 test file | **28** | |

Đối chiếu với §3.3 của `docs/OPENGL_PROFILE_MATRIX.md` (bảng gốc):
`L01–L18` = 349 lệnh legacy fixed-function, `L19` = 30, `L20` = 12 ⇒ **391**.
Script tự kiểm: `legacy_assigned == compat_not_core` (assert trong script §3.4
của profile matrix). Tổng khớp: **391 / 391**.

### 1.1 Chi tiết "implement thật" của các nhóm có code

**L03 — 20/20 lệnh, file `src/gl/gl_legacy_matrix.cpp`:**
`glFrustum` (:269), `glLoadIdentity` (:108), `glLoadMatrixd/f`,
`glLoadTransposeMatrixd/f` (:134), `glMatrixMode`, `glMultMatrixd/f`,
`glMultTransposeMatrixd/f`, `glOrtho`, `glPopMatrix`, `glPushMatrix`,
`glRotated`, `glRotatef`, `glScaled`, `glScalef`, `glTranslated`,
`glTranslatef`.
**Không còn stub nào trong L03** — nhưng chỉ **9/20** đạt §14.1 (11 lệnh không
được test gọi: `glLoadMatrixd/f`, `glMultMatrixd/f`, `glMultTransposeMatrixd/f`,
`glRotated`, `glScaled`, `glTranslated`).

**L01 — 88/173 lệnh, file `src/gl/gl_legacy_vertex.cpp`:**
Phân bố theo họ (đo bằng script, không suy đoán):

| Họ | Kiểu **float/double** | Kiểu **int/short/byte/ubyte** |
|---|---|---|
| `glVertex<n>` | `2d 2f 3d 3f 4d 4f` + bản `v` → **REAL** | `2i 2s 3i 3s 4i 4s` + bản `v` → **STUB** (12) |
| `glColor<n>` | **REAL** | **REAL** (tất cả `b s i f d ub us ui` + bản `v`) |
| `glNormal3` | `3d 3f` + `v` → **REAL** | `3b 3i 3s` + `v` → **STUB** (6) |
| `glTexCoord<n>` | `1d 1f 2d 2f 3d 3f 4d 4f` + `v` → **REAL** | `1i 1s 2i 2s 3i 3s 4i 4s` + `v` → **STUB** (16) |
| `glMultiTexCoord<n>` | `1d 1f 2d 2f 3d 3f 4d 4f` + `v` → **REAL** | `1i 1s 2i 2s 3i 3s 4i 4s` + `v` → **STUB** (16) |
| `glSecondaryColor3` | `3d 3f` + `v` → **REAL** | `3b 3i 3s 3ub 3ui 3us` + `v` → **STUB** (12) |
| `glEdgeFlag` / `glEdgeFlagv` | **REAL** | — |
| `glIndex<n>` (mọi kiểu) | — | **STUB** (10 lệnh) |
| `glFogCoord<n>` | — | **STUB** (4 lệnh) |
| `glRect<n>` | — | **STUB** (8 lệnh) |
| `glArrayElement` | — | **STUB** (1 lệnh) |

⇒ **62 / 85** lệnh L01 còn stub **chỉ vì thiếu biến thể kiểu số nguyên**
(`glVertex*`, `glTexCoord*`, `glMultiTexCoord*`, `glNormal*`,
`glSecondaryColor*`), dù cơ chế đã có sẵn cho kiểu float/double. Đây là
khoảng trống **rẻ nhất** để đóng: chỉ cần thêm overload chuyển đổi kiểu.

Lệnh đo (cho bảng trên):

```bash
python3 - <<'EOF'
import json, re
rows = json.load(open('/tmp/tglmt_dod_rows.json'))
l01 = [r for r in rows if r['group'] == 'L01']
st = {r['name'] for r in l01 if r['stub']}
fams = {'glVertex': r'^glVertex[234](i|s)(v?)$',
        'glTexCoord': r'^glTexCoord[1234](i|s)(v?)$',
        'glMultiTexCoord': r'^glMultiTexCoord[1234](i|s)(v?)$',
        'glNormal': r'^glNormal3(b|i|s)(v?)$',
        'glSecondaryColor': r'^glSecondaryColor3(b|i|s|ub|us|ui)(v?)$'}
tot = 0
for f, rx in fams.items():
    sel = [n for n in st if re.match(rx, n)]
    tot += len(sel); print("%-18s %d" % (f, len(sel)))
print("TOTAL int-variant stubs:", tot)
EOF
```

Kết quả: `glVertex 12`, `glTexCoord 16`, `glMultiTexCoord 16`,
`glNormal 6`, `glSecondaryColor 12` ⇒ **62**.

**L19 — 30/30 lệnh, file `src/gl/gl_packed_robust.cpp:124-138`** (sinh bằng
macro `PK_N` / `MTP_N`): `glVertexP2ui…P4uiv`, `glColorP3ui…P4uiv`,
`glNormalP3ui/uiv`, `glTexCoordP1ui…P4uiv`,
`glMultiTexCoordP1ui…P4uiv`, `glSecondaryColorP3ui/uiv`.
**12/30** đạt §14.1: 18 bản mảng (`…uiv`) **không có test gọi**.

**L20 — 12/12 lệnh, file `src/gl/gl_packed_robust.cpp`:**
`glGetnColorTable`, `glGetnConvolutionFilter`, `glGetnHistogram`,
`glGetnMapdv/fv/iv`, `glGetnMinmax`, `glGetnPixelMapfv/uiv/usv`,
`glGetnPolygonStipple`, `glGetnSeparableFilter`.
**0/12** đạt §14.1 — có code thật nhưng **không test nào gọi tới**.

---

## 2. Bảng theo 19 version (tập legacy khác nhau theo version)

Tập legacy của một version = `compatibility(version) − core(version)`.
Với GL 1.0–3.1 registry **không tách profile** ⇒ tập legacy **rỗng** ở 10
version đầu (đó là lý do "legacy set khác nhau theo version").

```bash
python3 /tmp/dod_measure.py   # mục "LEGACY per-version"
```

| Version | GLSL | Lệnh legacy (compat − core) | Stub | Implement thật | Đạt §14.1 | Trạng thái |
|---|---|---:|---:|---:|---:|---|
| 1.0 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` (chưa có profile) |
| 1.1 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 1.2 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 1.3 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 1.4 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 1.5 | — | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 2.0 | 1.10 | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 2.1 | 1.20 | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 3.0 | 1.30 | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 3.1 | 1.40 | 0 | 0 | 0 | 0 | `NOT APPLICABLE` |
| 3.2 | 1.50 | **350** | 240 | 109 | 17 | `MISSING` |
| 3.3 | 3.30 | **380** | 240 | 139 | 29 | `PARTIALLY_IMPLEMENTED` |
| 4.0 | 4.00 | 380 | 240 | 139 | 29 | `PARTIALLY_IMPLEMENTED` |
| 4.1 | 4.10 | 380 | 240 | 139 | 29 | `PARTIALLY_IMPLEMENTED` |
| 4.2 | 4.20 | 380 | 240 | 139 | 29 | `PARTIALLY_IMPLEMENTED` |
| 4.3 | 4.30 | **379** | 240 | 138 | 28 | `PARTIALLY_IMPLEMENTED` |
| 4.4 | 4.40 | 379 | 240 | 138 | 28 | `PARTIALLY_IMPLEMENTED` |
| 4.5 | 4.50 | **391** | 240 | 150 | 28 | `PARTIALLY_IMPLEMENTED` |
| 4.6 | 4.60 | **391** | 240 | 150 | 28 | `PARTIALLY_IMPLEMENTED` |

**Đọc bảng:**

- **10 version đầu (1.0–3.1):** `NOT APPLICABLE` cho *danh nghĩa "legacy
  profile"*, **không phải** "đã làm xong". Ở các version này, API fixed-function
  nằm **trong** core surface và đã bị tính ở bảng coverage modern của
  `reports/MODERN_API_COVERAGE_REPORT.md`. Ví dụ GL 2.0: 545 lệnh, 240 stub
  (44.0 %) — 240 stub **đều là fixed-function**.
- **3.2:** lần đầu GL tách profile ⇒ 350 lệnh bị loại khỏi core. TGLMT giữ
  **240** stub + **109** có code ⇒ **17** đạt §14.1.
- **3.3:** +30 lệnh `L19` (`glVertexP*`…) ⇒ 380 lệnh, 29 đạt §14.1.
- **4.3:** `glGetPointerv` được **khôi phục vào core** (spec GL 4.3) ⇒
  legacy **giảm** 380 → 379.
- **4.5:** +12 lệnh `L20` (`glGetn*`) ⇒ 391 lệnh.
- **4.6:** không đổi so với 4.5 ⇒ 391 lệnh.

**Quan sát quan trọng:** từ 3.2 đến 4.6, tỉ lệ stub của tập legacy **đứng yên
ở 240 lệnh tuyệt đối** — tức **toàn bộ** phần legacy đã triển khai là phần
đã có sẵn từ các phiên trước (matrix stack + immediate mode + packed vertex),
**không có** phần legacy nào mới được thêm kể từ khi tách profile.

---

## 3. Phân tích: vì sao mỗi nhóm chưa đạt

### 3.1 Nhóm `BLOCKED` vì **thiếu implementation** — 148 lệnh

Các nhóm có **0 entry point triển khai thật**, tất cả là stub `UNIMPLEMENTED`
trong `src/gl/gl_stubs_gen.cpp`:

| Nhóm | Lệnh | Vì sao chưa làm |
|---|---:|---|
| L02 Client-side vertex arrays | 12 | Cần client-side vertex array + `glEnableClientState`; phụ thuộc draw path (xem 3.3) |
| L04 Lighting, materials, shading model | 18 | Cần shader fixed-function sinh MSL + state light/material. Metal **không** có fixed-function lighting ⇒ phải sinh shader ⇒ **phụ thuộc 3.3** |
| L05 Fog | 4 | Cùng lý do — fog phải được mã hoá trong fragment shader ⇒ **phụ thuộc 3.3** |
| L06 Texture env + texcoord generation | 15 | Cùng lý do — texenv phải vào fragment shader ⇒ **phụ thuộc 3.3** |
| L07 Display lists | 8 | Cần command-stream recorder; độc lập về mặt kiến trúc, nhưng cần hạ tầng command stream mới |
| L08 Evaluators | 23 | Cần bộ đánh giá đa thức 1D/2D + mesh tessellation CPU; **độc lập** với draw path ở mức state, nhưng output phải đi qua draw path |
| L09 Selection and feedback | 8 | Cần name stack + hit-record buffer + `GL_SELECT` render mode; **độc lập** về state, nhưng `glRenderMode` phải can thiệp vào draw path |
| L10 Accumulation buffer | 2 | Cần buffer accumulation riêng (Metal không có accumulation buffer) ⇒ **CPU/GPU emulation** |
| L11 Pixel transfer modes + pixel drawing | 12 | `glDrawPixels`/`glBitmap`/`glCopyPixels` cần đường raster 2D; `glCopyPixels` có thể dùng `MTLBlitCommandEncoder`; pixel maps cần CPU |
| L12 Raster position + window position | 40 | Cần theo dõi vị trí raster sau mỗi draw + `glWindowPos*`; **phụ thuộc draw path** để biết vị trí raster thật |
| L14 Alpha test | 1 | Alpha test phải vào fragment shader ⇒ **phụ thuộc 3.3** |
| L15 Texture residency + priority | 2 | Cần mô hình residency texture (Metal không có) ⇒ emulation CPU |
| L16 User clipping planes | 2 | Metal **không** có user clip plane ⇒ phải cắt bằng shader/discard hoặc CPU ⇒ cần pipeline cố định |
| L17 Attribute stacks | 4 | Cần snapshot state stack; **độc lập** với draw path |
| L18 Colour-index mode state | 2 | Cần chế độ colour-index; phụ thuộc pipeline cố định |

**Tổng: 148 lệnh — `MISSING`/`STUB`.** 148/391 = **37.9 %** của bề mặt legacy.

### 3.2 Nhóm **chỉ còn thiếu draw path** — 122 lệnh

Đây là phần **không phải** thiếu implementation logic, mà thiếu **đường draw
fixed-function**.

| Nhóm | Lệnh | Đã có | Còn thiếu |
|---|---:|---|---|
| L01 Immediate mode | 173 | 88 lệnh + ghép primitive đúng (kiểm thử được thứ tự đỉnh) | **nối `immediatePrimitives` vào Metal draw path** |
| L03 Matrix stack | 20 | 20 lệnh, semantics đã kiểm chứng đầy đủ | **nối ma trận modelview/projection vào vertex transform** |
| L13 Stipple | 3 | `glLineStipple` tồn tại nhưng **thân no-op** (`src/gl/gl_raster.cpp:274`) | triển khai stipple + draw path |
| L19/L20 | 42 | 42 lệnh có code thật | **không cần draw path** (là attribute setter / query) — thiếu **test**, không thiếu code |

**Bằng chứng "1 chỗ ghi, 0 chỗ đọc":**

```bash
grep -rn "immediatePrimitives" src/ include/
```
```
src/gl/gl_legacy_vertex.cpp:98    // Primitive đã ghép (sẽ đưa vào Metal draw path).
src/gl/gl_legacy_vertex.cpp:107   st.immediatePrimitives.push_back(p);
include/tglmt/LegacyState.h:149   std::vector<LegacyPrimitive> immediatePrimitives;
include/tglmt/LegacyState.h:161   immediatePrimitives.clear();
```
⇒ Không có chỗ nào **đọc** `immediatePrimitives` ⇒ **không render được hình
fixed-function**. Trạng thái của hạng mục này: **`BLOCKED`**.

Tác động: dù L01+L03 có **108** lệnh triển khai thật và 2 test PASS
(`test_legacy_immediate`, `test_legacy_matrix`), **không** có **ảnh** legacy
nào trong repo ⇒ benchmark legacy (`reports/BASELINE_TEST_REPORT.md` §5.6)
và `NV-06` (baseline ảnh legacy) đều là `NOT_VERIFIED`.

### 3.3 Nhóm là **vấn đề thiết kế**, không phải thiếu code

| Vấn đề | Bằng chứng | Hệ quả |
|---|---|---|
| **Metal không có fixed-function pipeline** | `docs/METAL_A11_CAPABILITIES.md`, `docs/METAL_FEATURE_MAPPING.md` | Lighting / fog / texenv / alpha test / stipple **không thể** ánh xạ 1:1 — phải **sinh MSL sinh fixed-function** cho từng tổ hợp trạng thái. Đây là lý do 6 nhóm (L04, L05, L06, L13, L14, L16 = 41 lệnh) là `BLOCKED` **cho tới khi** có pipeline sinh đó. |
| **Metal không có display list / evaluator / selection buffer** | `docs/HARDWARE_LIMITATIONS.md` | L07, L08, L09 (39 lệnh) cần **emulation CPU**: recorder command stream, evaluator polynomial, name stack + hit buffer. Đây là *thiết kế* mới, không phải port. |
| **Metal không có accumulation buffer / texture residency / user clip plane** | `docs/METAL_A11_CAPABILITIES.md` §giới hạn | L10, L15, L16 (6 lệnh) buộc emulation. |
| **24 entry point legacy nằm ngoài bề mặt registry** | `reports/BASELINE_TEST_REPORT.md` §3.5 | `glTranslatefv`, `glRotatefv`, `glScalefv`, `glRotatedv`, `glScaledv`, `glTranslatedv` + 18 form `glIndex<n><type>` có **định nghĩa** trong `src/gl/` nhưng **không export** và **không** khai báo trong `gl_c_api.h` / `gl_legacy.h` ⇒ **mã chết**, không client nào gọi được. Trạng thái: `PARTIALLY_IMPLEMENTED`. Không tính vào cột coverage nào (bề mặt đo là registry). |
| **`glEnable(GL_LIGHTING)` ở Core Profile bị im lặng** | `TODO.md` T0.9 | `src/gl/gl_raster.cpp` chấp nhận enum fixed-function ở Core mà không lỗi ⇒ *"no-op accepted"* = state nói dối app. Trạng thái: `INCORRECT`. |
| **`glGetIntegerv` gate pname mới phủ 12 pname** | `src/gl/gl_get.cpp` (`CompatOnlyPname`), `TODO.md` T0.8 | Pname fixed-function (light/material/fog/texenv) chưa gate ⇒ ở Core vẫn trả về giá trị thay vì `GL_INVALID_ENUM`. |
| **`GL_EXTENSIONS` là bảng tĩnh** | `src/gl/gl_error_string_debug.cpp:92` | Không lọc theo (version, profile) ⇒ một extension thuộc compatibility có thể bị advertise ở core context. `TODO.md` T0.11. |

---

## 4. Bảng "ưu tiên PHASE 6" (14 mục của `other/prompt.md` §PHASE 6)

Nguồn: `other/prompt.md` dòng 1286–1301 (danh sách ưu tiên, **không** phải
danh sách giới hạn).

| # | Hạng mục ưu tiên | Nhóm | Lệnh | Stub | Implement thật | Đạt §14.1 | Trạng thái | Bước tiếp theo (`TODO.md`) |
|---:|---|---|---:|---:|---:|---:|---|---|
| 1 | State management | M18 + M12 | 51 | 0 | 51 | **46** | `PARTIALLY_IMPLEMENTED` | T0.8 (gate pname), T0.9 (gate `glEnable`) |
| 2 | Matrix stacks | L03 | 20 | 0 | 20 | **9** | `PARTIALLY_IMPLEMENTED` | T1-S4 (nối draw path) + test cho 11 lệnh còn lại |
| 3 | Immediate mode | L01 | 173 | 85 | 88 | **7** | `PARTIALLY_IMPLEMENTED` | T1-S3 (bảng lệnh cấm trong `glBegin`) + T1-S4 + 62 lệnh biến thể kiểu số nguyên |
| 4 | Vertex arrays | L02 | 12 | 12 | 0 | **0** | `MISSING` | T1-S15 (12 lệnh) |
| 5 | Fixed-function transformations | L03 | 20 | 0 | 20 | **9** | `PARTIALLY_IMPLEMENTED` | T1-S4 |
| 6 | Lighting and materials | L04 | 18 | 18 | 0 | **0** | `MISSING` | T1-S5 (18 lệnh) — cần pipeline sinh fixed-function |
| 7 | Texture environment | L06 | 15 | 15 | 0 | **0** | `MISSING` | T1-S7 (15 lệnh) |
| 8 | Fog | L05 | 4 | 4 | 0 | **0** | `MISSING` | T1-S6 (4 lệnh) |
| 9 | Legacy rasterization | L13, L14, L16, L10, L15 | 10 | 9 | 0 | **0** | `MISSING` | T1-S8 (alpha test, clip plane, stipple, polygon mode, polygon offset, point size, color mask) |
| 10 | Display lists | L07 | 8 | 8 | 0 | **0** | `MISSING` | T1-S11 (8 lệnh) |
| 11 | Evaluators | L08 | 23 | 23 | 0 | **0** | `MISSING` | T1-S12 (23 lệnh) |
| 12 | Pixel operations | L11 + L12 | 52 | 52 | 0 | **0** | `MISSING` | T1-S9 (48 lệnh raster/window pos) + T1-S10 (12 lệnh pixel) |
| 13 | Selection and feedback | L09 | 8 | 8 | 0 | **0** | `MISSING` | T1-S13 (8 lệnh) |
| 14 | Các chức năng còn lại theo inventory | L17, L18, L19, L20 | 48 | 6 | 42 | **12** | `PARTIALLY_IMPLEMENTED` | T1-S16 (attribute stack, 4) + T1-S14 (accumulation, 2) + test cho L19/L20 (30) |
| — | *(ngoài danh sách)* | GLU | — | — | — | — | `MISSING` | T1-S17 — **A3 nói GLU CÓ trong phạm vi**; chưa có `src/glu/` |

Lệnh đo bảng này:

```bash
python3 - <<'EOF'
import json, collections
rows = json.load(open('/tmp/tglmt_dod_rows.json'))
g = collections.defaultdict(list)
for r in rows: g[r['group']].append(r)
phase6 = [("1. State management", ["M18","M12"]),
           ("2. Matrix stacks", ["L03"]), ("3. Immediate mode", ["L01"]),
           ("4. Vertex arrays", ["L02"]),
           ("5. Fixed-function transformations", ["L03"]),
           ("6. Lighting and materials", ["L04"]), ("7. Texture environment", ["L06"]),
           ("8. Fog", ["L05"]),
           ("9. Legacy rasterization", ["L13","L14","L16","L10","L15"]),
           ("10. Display lists", ["L07"]), ("11. Evaluators", ["L08"]),
           ("12. Pixel operations", ["L11","L12"]),
           ("13. Selection and feedback", ["L09"]),
           ("14. Remaining per inventory", ["L17","L18","L19","L20"])]
for name, gs in phase6:
    items = [r for k in gs for r in g.get(k, [])]
    print("%-32s cmds=%-4d stub=%-4d real=%-4d DoD=%d" % (name, len(items),
        sum(1 for r in items if r['stub']),
        sum(1 for r in items if r['kind']=='real' and not r['stub']),
        sum(1 for r in items if r['dod'])))
EOF
```

**Kết luận PHASE 6:** **2 / 14** mục ưu tiên đã có code thật cho **mọi** lệnh
trong nhóm (mục 2 và 5 — cùng là L03), **9 / 14** mục còn **0** lệnh triển khai,
**3 / 14** mục có code nhưng thiếu test. **0 / 14** mục đạt `FULLY_IMPLEMENTED`.

---

## 5. Bảng test legacy

```bash
(cd build-work && ctest -j8 -V > /tmp/tglmt_ctest_v.log 2>&1)
grep -E "test_legacy" /tmp/tglmt_ctest_v.log | grep "Test #"
```

| Test ID | Nhóm | Nội dung | Kết quả | Nhãn |
|---|---|---|---|---|
| `test_legacy_matrix` | L03 | Ma trận: identity, translate, thứ tự nhân (2 chiều), transpose, push/pop, stack overflow/underflow, ortho/frustum + lỗi, matrix mode sai, stack riêng theo mode, đọc lại `GL_PROJECTION_MATRIX`, Core ⇒ `INVALID_OPERATION` + `INVALID_ENUM` | **PASS** | `REFERENCE_ONLY` |
| `test_legacy_immediate` | L01 | Cân bằng `glBegin`/`glEnd`, `glBegin` lồng, mode sai, `glVertex` ngoài `glBegin`, chụp current attributes theo từng đỉnh, ghép primitive cho 9 mode (kể cả `GL_QUAD_STRIP` = `(2i,2i+1,2i+3,2i+2)` và winding đảo của `GL_TRIANGLE_STRIP`), multi-texture current texcoord theo unit, Core ⇒ `INVALID_OPERATION` | **PASS** | `REFERENCE_ONLY` |
| `test_context_version` | L01/L03 (gate) | 28 context (19 version × profile hợp lệ), `GL_VERSION`/`GL_MAJOR`/`GL_MINOR`/`GL_CONTEXT_PROFILE_MASK`/`GL_CONTEXT_FLAGS`/`GL_SHADING_LANGUAGE_VERSION`, pname version-gating, gate entry point 4 ca, `getProcAddress` theo profile, từ chối attribs mâu thuẫn, self-check bảng gate (657/1048) | **PASS** | `REFERENCE_ONLY` |
| `test_packed_vertex_types` | L19 | 12 lệnh `gl*P*ui` | **PASS** | `REFERENCE_ONLY` |
| `test_readpixels_packed` | L19 | `glReadPixels` với packed vertex type | **PASS** | `REFERENCE_ONLY` |

**Test legacy còn thiếu** (mỗi mục là một `NOT_VERIFIED` cụ thể):

| Nhóm | Test cần | Số lệnh chưa có test gọi |
|---|---|---:|
| L01 | test gọi 62 lệnh biến thể kiểu số nguyên + `glIndex*` (10) + `glFogCoord*` (4) + `glRect*` (8) + `glArrayElement` (1) — **85 lệnh stub**; cộng **81 lệnh đã có code** nhưng chưa test gọi | **166 / 173** |
| L03 | test gọi 11 lệnh matrix còn lại | 11 / 20 |
| L19 | test gọi 18 bản mảng `…uiv` | 18 / 30 |
| L20 | test gọi 12 lệnh `glGetn*` | 12 / 12 |
| L02, L04–L18 | chưa có code ⇒ chưa có test | 148 |

---

## 6. Đối chiếu với yêu cầu §14.C của đặc tả

| Bullet §14.C | Nhóm | Kết quả |
|---|---|---|
| Immediate mode | L01 | `PARTIALLY_IMPLEMENTED` — 88/173 code, 7 đạt §14.1, **chưa** render |
| Matrix stacks | L03 | `PARTIALLY_IMPLEMENTED` — 20/20 code, 9 đạt §14.1, **chưa** áp vào draw |
| Lighting | L04 | `MISSING` |
| Materials | L04 | `MISSING` |
| Fog | L05 | `MISSING` |
| Texture environment | L06 | `MISSING` |
| Display lists | L07 | `MISSING` |
| Evaluators | L08 | `MISSING` |
| Pixel operations | L11 + L12 | `MISSING` |
| Selection/feedback | L09 | `MISSING` |
| Các chức năng compatibility khác | L02, L10, L13–L18, L19, L20 | `PARTIALLY_IMPLEMENTED` (L19/L20 có code) / `MISSING` (phần còn lại) |

**Kết luận §14.C: KHÔNG ĐẠT.** 9 / 11 bullet có **0** entry point triển khai thật.

---

## 7. Danh sách `NOT_VERIFIED` / `BLOCKED` kèm lệnh đo

| ID | Hạng mục | Trạng thái | Lệnh đo / điều kiện |
|---|---|---|---|
| **LC-01** | **Đường draw fixed-function** | `BLOCKED` | `grep -rn "immediatePrimitives" src/ include/` — cần **1 chỗ đọc**. Thiết kế: `docs/LEGACY_TO_METAL_MAPPING.md` §coexistence |
| **LC-02** | 240 lệnh legacy còn stub | `MISSING` | Implement theo `TODO.md` T1-S5…T1-S16; đo lại bằng `python3 /tmp/dod_measure.py` |
| **LC-03** | 62 lệnh L01 biến thể kiểu số nguyên | `MISSING` | `grep -c "UNIMPLEMENTED" src/gl/gl_stubs_gen.cpp` giảm 62; thêm case vào `tests/unit/test_legacy_immediate.cpp` |
| **LC-04** | 18 bản mảng `…uiv` của L19 chưa có test | `NOT_VERIFIED` | Thêm case vào `tests/unit/test_packed_vertex_types.cpp`; `ctest -R test_packed_vertex_types` |
| **LC-05** | 12 lệnh `glGetn*` (L20) không test nào gọi | `NOT_VERIFIED` | `grep -rn "glGetn" tests/` — hiện chỉ có `glGetnUniformfvKHR` / `glGetnTexImage`; cần case cho `glGetnHistogram`, `glGetnMinmax`, … |
| **LC-06** | 11 lệnh L03 chưa có test gọi | `NOT_VERIFIED` | `grep -rn "glMultMatrixf\|glRotated\|glScaled\|glTranslated" tests/` — hiện 0 kết quả |
| **LC-07** | **Ảnh legacy** (fixed-function → pixel) | `NOT_VERIFIED` | Sau LC-01: `ctest -R test_legacy_render_apple` đọc pixel, so công thức spec §17.3.6 |
| **LC-08** | **Benchmark legacy** | `NOT_VERIFIED` | Sau LC-01: `cd build-work && for i in 1..9; do ./bench_runtime --iters 2000; done` có metric `glBegin/glEnd overhead` |
| **LC-09** | Hành vi **Compatibility Profile** trên GPU | `NOT_VERIFIED` | Sau LC-01: `ctest -R test_compat_` trên backend `"apple"` |
| **LC-10** | 24 entry point legacy ngoài registry, không export | `PARTIALLY_IMPLEMENTED` | `nm -gU build-work/libtglmt.0.1.0.dylib \| grep -E "_glTranslatefv\|_glIndex1d"` — hiện không có dòng nào ⇒ cần export + gate version |
| **LC-11** | `glEnable(GL_LIGHTING)` im lặng ở Core | `INCORRECT` | `TODO.md` T0.9 — sửa `src/gl/gl_raster.cpp` |
| **LC-12** | `glGetIntegerv` gate pname fixed-function | `NOT_VERIFIED` | `TODO.md` T0.8 — mở rộng `CompatOnlyPname` trong `src/gl/gl_get.cpp` |
| **LC-13** | `GL_EXTENSIONS` lọc theo profile | `NOT_VERIFIED` | `TODO.md` T0.11 — `src/gl/gl_error_string_debug.cpp:92` |
| **LC-14** | **GLU** (A3: CÓ trong phạm vi) | `MISSING` | `TODO.md` T1-S17 — cần `src/glu/` (tessellator, quadrics, `gluBuild2DMipmaps`) |
| **LC-15** | A11 cho bất kỳ mục legacy nào | `NOT_VERIFIED` | `build-iphoneos` + `ctest -j1` trên iPhone 8/X; nhãn → `A11_DEVICE` |

---

## 8. Kết luận

1. **Bề mặt legacy**: 391 entry point (GL 4.6 compatibility − core).
   **150** có định nghĩa thật, **240** còn stub trung thực, **1** là no-op
   (`glLineStipple`, `src/gl/gl_raster.cpp:274`).
2. **Đạt §14.1**: **28 / 391** (7.2 %) — L01: 7, L03: 9, L19: 12, L20: 0.
3. **9 / 20 nhóm** có **0** entry point triển khai thật (L02, L04–L12, L14–L18)
   ⇒ `MISSING`.
4. **Blocker lớn nhất**: không có đường draw fixed-function
   (`immediatePrimitives` 1 chỗ ghi, 0 chỗ đọc) ⇒ không có ảnh legacy nào.
5. **Ưu tiên PHASE 6**: 9 / 14 mục còn 0 lệnh triển khai; **0 / 14** đạt
   `FULLY_IMPLEMENTED`.
6. **Không có claim** về A11, về CTS, hay về "đã render được hình fixed-function".
