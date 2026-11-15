# IMMEDIATE_MODE.md — immediate mode của TGLMT

Tài liệu kỹ thuật dùng để **implement** immediate mode (Begin/End paradigm) của
OpenGL Compatibility Profile trên TGLMT (OpenGL → Metal, iOS / Apple A11).

**Nguồn spec:** OpenGL 2.1 Specification (`glspec21.pdf`, 01-Dec-2006) —
**§2.6 "Begin/End Paradigm"** (trang 13-20), **§2.7 "Vertex Specification"**
(trang 20-21), **§2.10 "Rectangles"** (trang 40), **§2.13 "Current Raster
Position"** (trang 54-56), §2.3 (bảng lỗi 2.3).
Đối chiếu cục bộ: `docs/khronos/glspec46.core.pdf` (Core Profile 4.6, App. E.2.2
xác nhận Begin/End đã bị gỡ khỏi Core Profile 3.2 ⇒ **mọi** entry point trong tài
liệu này là COMPAT-ONLY).

**Nhãn:** `SPEC` · `CURRENT` (code thật, kèm `file:line`) · `PLAN`.
Trạng thái theo bộ `AGENTS.md` §6.

**Môi trường đo:** Null backend, build Debug (`build-work/`), nhãn `NONE`.

---

## 0. Tóm tắt trạng thái (CURRENT, đo được)

Census bằng script (đọc `include/tglmt/gl_legacy.h` × định nghĩa thật trong
`src/gl/*.cpp`, trừ `gl_stubs_gen.cpp` / `gl_c_exports.cpp`):

| Nhóm | Khai báo | Đã implement | Stub |
|---|---:|---:|---:|
| `glBegin` / `glEnd` | 2 | **2** | 0 |
| `glVertex{234}{sifd}[v]` | 24 | **12** (chỉ `f`/`d`) | 12 |
| `glColor{34}{sifdub}[v]` | 32 | **32** | 0 |
| `glNormal3{sifd}[v]` | 10 | **4** (chỉ `f`/`d`) | 6 |
| `glTexCoord{1234}{sifd}[v]` | 32 | **16** (chỉ `f`/`d`) | 16 |
| `glMultiTexCoord{1234}{sifd}[v]` | 32 | **16** (chỉ `f`/`d`) | 16 |
| `glSecondaryColor3{sifdub}[v]` | 16 | **4** (chỉ `f`/`d`) | 12 |
| `glEdgeFlag` / `glEdgeFlagv` | 2 | **2** | 0 |
| `glFogCoord{f,d}[v]` | 4 | 0 | 4 |
| `glIndex{sifdub}[v]` | 10 | 0 | 10 |
| `glArrayElement` | 1 | 0 | 1 |
| `glRect{sifd}[v]` | 8 | 0 | 8 |
| **TỔNG immediate** | **173** | **88** | **85** |

Tổng legacy: **349** khai báo / **109** implement / **240** stub
(xem `docs/FIXED_FUNCTION_PIPELINE.md`).

**Điểm cần lưu ý (CURRENT):** `src/gl/gl_legacy_vertex.cpp:539-591` implement
**18 hàm `glIndex1*` / `glIndex2*` / `glIndex3*` không tồn tại trong OpenGL**
(đối chiếu `docs/khronos/gl.xml`: `glIndex1i`, `glIndex2f`, `glIndex3uiv`… **không
có** trong registry). Chúng **không** được export ra C ABI
(`src/gl/gl_c_exports.cpp` không có mục nào tên `glIndex1i`/`glIndex3f`) ⇒
**dead code**, không app nào gọi được. Ngược lại, **10 entry point `glIndex*`
thật** vẫn là stub. Xem §6 L-2.

**Test hiện có:** `tests/unit/test_legacy_immediate.cpp` (CTest name
`test_legacy_immediate`) — PASS theo `PROGRESS.md:130`.

---

## 1. SPEC — semantics

### 1.1 Mô hình (GL 2.1 §2.6)

> "In the GL, most geometric objects are drawn by enclosing a series of coordinate
> sets that specify vertices and optionally normals, texture coordinates, and
> colors between Begin/End pairs. There are **ten** geometric objects that are
> drawn this way."

| Mode | Enum | Hex |
|---|---|---|
| `GL_POINTS` | points | 0x0000 |
| `GL_LINES` | separated line segments | 0x0001 |
| `GL_LINE_LOOP` | line segment loops | 0x0002 |
| `GL_LINE_STRIP` | line segment strips | 0x0003 |
| `GL_TRIANGLES` | separated triangles | 0x0004 |
| `GL_TRIANGLE_STRIP` | triangle strips | 0x0005 |
| `GL_TRIANGLE_FAN` | triangle fans | 0x0006 |
| `GL_QUADS` | separated quadrilaterals | 0x0007 |
| `GL_QUAD_STRIP` | quadrilateral strips | 0x0008 |
| `GL_POLYGON` | polygons | 0x0009 |

Có **10 mode**, không hơn không kém. `glBegin` với mode khác ⇒ lỗi (xem §1.4).

### 1.2 Quy tắc gom đỉnh (GL 2.1 §2.7)

> "A call to any Vertex command specifies four coordinates: x, y, z, and w. …
> A call to Vertex2 sets the x and y coordinates; the z coordinate is implicitly
> set to zero and the w coordinate to one. Vertex3 sets x, y, and z to the
> provided values and w to one. Vertex4 sets all four coordinates…"

> "Invoking a Vertex command outside of a Begin/End pair results in **undefined
> behavior**."

**Current values** (màu, normal, texcoord, secondary color, index, edge flag) được
**chụp lại vào đỉnh tại thời điểm `glVertex*` được gọi** — chúng là *client
state* theo nghĩa display list (xem `docs/DISPLAY_LISTS.md` §3.6).

Quy tắc texture coordinate (GL 2.1 §2.7):

* `TexCoord1` ⇒ `s` = tham số, `t = r = 0`, `q = 1`.
* `TexCoord2` ⇒ `s,t`; `r = 0`, `q = 1`.
* `TexCoord3` ⇒ `s,t,r`; `q = 1`.
* `TexCoord4` ⇒ `s,t,r,q`.
* "The TexCoord commands are exactly equivalent to the corresponding MultiTexCoord
  commands with texture set to TEXTURE0."
* "Gets of CURRENT_TEXTURE_COORDS return the texture coordinate set defined by the
  value of ACTIVE_TEXTURE."
* "Implementations must support at least **two** sets of texture coordinates."

### 1.3 Edge flag (GL 2.1 §2.6.2)

> "Each edge of each primitive generated from a polygon, triangle strip, triangle
> fan, separate triangle set, quadrilateral strip, or separate quadrilateral set,
> is flagged as either boundary or non-boundary. … By default, all edges are
> boundary edges."

> "When Begin is supplied with one of the argument values **POLYGON, TRIANGLES, or
> QUADS**, each vertex specified within a Begin and End pair begins an edge."

State: **một** bit hiện tại, ban đầu `TRUE`.

### 1.4 Lỗi (GL 2.1 §2.6.3 + bảng lỗi 2.3)

| Tình huống | Lỗi | Nguồn |
|---|---|---|
| `glBegin` sau `glBegin` mà chưa `glEnd` | `GL_INVALID_OPERATION` | §2.6.3 (nguyên văn) |
| `glEnd` không có `glBegin` tương ứng | `GL_INVALID_OPERATION` | §2.6.3 (nguyên văn) |
| `glBegin(mode)` với `mode` ngoài 10 mode trên | **`GL_INVALID_ENUM`** | Bảng 2.3 + bullet §2: "If a command that requires an enumerated value is passed a symbolic constant that is not one of those specified as allowable for that command, the error INVALID_ENUM is generated." (xem §6 L-1) |
| Bất kỳ lệnh GL nào khác ngoài danh sách cho phép, giữa `Begin` và `End` | `GL_INVALID_OPERATION` | §2.6.3 (nguyên văn) |
| `glVertex*` ngoài Begin/End | **undefined** (TGLMT chọn `GL_INVALID_OPERATION` — chặt hơn spec, hợp lệ) | §2.7 |

---

## 2. Bảng lệnh bị cấm trong `glBegin` / `glEnd`

### 2.1 Nguồn spec (nguyên văn, GL 2.1 §2.6.3)

> "The only GL commands that are allowed within any Begin/End pairs are the
> commands for specifying vertex coordinates, vertex colors, normal coordinates,
> texture coordinates, generic vertex attributes, and fog coordinates
> (**Vertex, Color, SecondaryColor, Index, Normal, TexCoord and MultiTexCoord,
> VertexAttrib, FogCoord**), the **ArrayElement** command (see section 2.8), the
> **EvalCoord** and **EvalPoint** commands (see section 5.1), commands for
> specifying lighting material parameters (**Material** commands; see section
> 2.14.2), display list invocation commands (**CallList** and **CallLists**; see
> section 5.4), and the **EdgeFlag** command. **Executing any other GL command
> between the execution of Begin and the corresponding execution of End results in
> the error INVALID_OPERATION.** Executing Begin after Begin has already been
> executed but before an End is executed generates the INVALID_OPERATION error, as
> does executing End without a previous corresponding Begin."

> "Execution of the commands **EnableClientState, DisableClientState,
> PushClientAttrib, PopClientAttrib, ColorPointer, FogCoordPointer,
> EdgeFlagPointer, IndexPointer, NormalPointer, TexCoordPointer,
> SecondaryColorPointer, VertexPointer, VertexAttribPointer, ClientActiveTexture,
> InterleavedArrays, and PixelStore** is not allowed within any Begin/End pair,
> **but an error may or may not be generated** if such execution occurs. **If an
> error is not generated, GL operation is undefined.**"

### 2.2 Bảng A — lệnh **ĐƯỢC PHÉP** trong Begin/End (16 họ)

| # | Họ lệnh | Entry point cụ thể | Vì sao được phép |
|---|---|---|---|
| 1 | Vertex | `glVertex{234}{sifd}[v]` | chỉ định toạ độ đỉnh |
| 2 | Color | `glColor{34}{sifdub}[v]` | current color của đỉnh |
| 3 | SecondaryColor | `glSecondaryColor3{sifdub}[v]` | current secondary color |
| 4 | Index | `glIndex{sifdub}[v]` | color index của đỉnh |
| 5 | Normal | `glNormal3{sifd}[v]` | current normal |
| 6 | TexCoord | `glTexCoord{1234}{sifd}[v]` | current texture coordinate |
| 7 | MultiTexCoord | `glMultiTexCoord{1234}{sifd}[v]` | current texcoord theo unit |
| 8 | VertexAttrib | `glVertexAttrib{1234}{f,d,ub,us,ui}` | generic vertex attribute |
| 9 | FogCoord | `glFogCoord{f,d}[v]` | current fog coordinate |
| 10 | ArrayElement | `glArrayElement` | lấy đỉnh từ array client |
| 11 | EvalCoord | `glEvalCoord{1,2}{f,d}[v]` | evaluator sinh đỉnh |
| 12 | EvalPoint | `glEvalPoint{1,2}` | evaluator sinh đỉnh |
| 13 | Material | `glMaterial{f,i}[v]` | đổi material **trong** Begin/End (spec §2.14.2 cho phép) |
| 14 | CallList | `glCallList` | display list invocation |
| 15 | CallLists | `glCallLists` | display list invocation |
| 16 | EdgeFlag | `glEdgeFlag`, `glEdgeFlagv` | cờ cạnh |

⇒ **16 họ lệnh được phép.**

### 2.3 Bảng B — lệnh **BỊ CẤM**, lỗi **bắt buộc** `GL_INVALID_OPERATION`

Toàn bộ GL command còn lại (≈ 1049 − 16 họ ở Bảng A − 16 lệnh ở Bảng C) phải
sinh `GL_INVALID_OPERATION` khi thực thi giữa `Begin` và `End`.

| Nhóm | Ví dụ đại diện | Vì sao cấm |
|---|---|---|
| Matrix stack | `glPushMatrix`, `glPopMatrix`, `glTranslate*`, `glRotate*`, `glScale*`, `glLoadMatrix*`, `glMultMatrix*`, `glOrtho`, `glFrustum`, `glMatrixMode` | biến đổi trong lúc gom đỉnh làm đỉnh không nhất quán |
| Viewport / raster state | `glViewport`, `glDepthRange`, `glScissor`, `glEnable`, `glDisable`, `glBlendFunc`, `glDepthFunc`, `glCullFace`, `glFrontFace`, `glPolygonMode`, `glPolygonOffset`, `glHint` | đổi state giữa chừng |
| Lighting / material (trừ `Material`) | `glLight*`, `glLightModel*`, `glColorMaterial`, `glShadeModel` | đặc biệt `glColorMaterial` + `glColor` **được** phép (xem §2.4) |
| Fog | `glFog*` | |
| Texenv / texgen | `glTexEnv*`, `glTexGen*` | |
| Texture binding | `glBindTexture`, `glTexParameter*`, `glGenTextures`, `glDeleteTextures` | texture đổi giữa các đỉnh |
| Buffer / VAO / program | `glBindBuffer`, `glBufferData`, `glBindVertexArray`, `glUseProgram`, `glLinkProgram`, `glDrawArrays`, `glDrawElements` | lệnh draw trong Begin/End là **undefined** |
| Pixel ops | `glReadPixels`, `glDrawPixels`, `glCopyPixels`, `glPixelStore*` | |
| Display list mgmt | `glNewList`, `glEndList`, `glGenLists`, `glDeleteLists` | |
| Query | mọi `glGet*` / `glIs*` | |
| Selection / feedback | `glRenderMode`, `glSelectBuffer`, `glFeedbackBuffer`, `glPushName`, `glPopName` | |
| Evaluator setup | `glMap1*`, `glMap2*`, `glMapGrid*` | |
| Accumulation | `glAccum`, `glClearAccum` | |

**Lý do chung (spec không nói rõ từng lệnh):** trong Begin/End, GL đang **gom
đỉnh**; mọi lệnh thay đổi trạng thái hoặc phát sinh draw giữa chừng sẽ làm
ngữ nghĩa "vertex" không xác định. Spec chọn cách **cấm toàn bộ** thay vì liệt
kê từng lệnh ("Executing **any other** GL command … results in the error
INVALID_OPERATION").

### 2.4 Bảng C — lệnh **BỊ CẤM** nhưng lỗi **tuỳ chọn** (16 lệnh)

Nếu **không** sinh lỗi ⇒ **hành vi undefined** (spec nói rõ). TGLMT nên **sinh
lỗi** (chặt hơn, an toàn).

| # | Lệnh | Vì sao cấm |
|---|---|---|
| 1 | `glEnableClientState` | đổi trạng thái array client giữa lúc gom đỉnh |
| 2 | `glDisableClientState` | như trên |
| 3 | `glPushClientAttrib` | đẩy client state stack giữa lúc gom đỉnh |
| 4 | `glPopClientAttrib` | như trên |
| 5 | `glColorPointer` | đổi con trỏ array màu giữa lúc gom đỉnh |
| 6 | `glFogCoordPointer` | đổi con trỏ array fog coord |
| 7 | `glEdgeFlagPointer` | đổi con trỏ array edge flag |
| 8 | `glIndexPointer` | đổi con trỏ array color index |
| 9 | `glNormalPointer` | đổi con trỏ array normal |
| 10 | `glTexCoordPointer` | đổi con trỏ array texcoord |
| 11 | `glSecondaryColorPointer` | đổi con trỏ array secondary color |
| 12 | `glVertexPointer` | đổi con trỏ array vertex |
| 13 | `glVertexAttribPointer` | đổi con trỏ array generic attribute |
| 14 | `glClientActiveTexture` | đổi texture unit đang hoạt động giữa lúc gom đỉnh |
| 15 | `glInterleavedArrays` | đặt lại toàn bộ layout array client |
| 16 | `glPixelStore` | đổi unpack/pack state giữa lúc gom đỉnh |

### 2.5 Tổng kết

| Nhóm | Số lệnh/họ | Lỗi |
|---|---:|---|
| Được phép trong Begin/End (Bảng A) | **16 họ** | — |
| Bị cấm, lỗi **bắt buộc** `INVALID_OPERATION` (Bảng B) | **mọi lệnh còn lại** | bắt buộc |
| Bị cấm, lỗi **tuỳ chọn**, không lỗi ⇒ undefined (Bảng C) | **16 lệnh** | tuỳ chọn |

**Nguồn:** OpenGL 2.1 spec §2.6.3, `glspec21.pdf` trang 19-20 (2 đoạn trích ở
§2.1). Đối chiếu `docs/khronos/glspec46.core.pdf` App. E.2.2: toàn bộ nhóm này
đã bị gỡ khỏi Core Profile 3.2.

### 2.6 PLAN — cơ chế thực thi

Vì bề mặt GL có ~1049 entry point, cơ chế chặn **bắt buộc** phải ở **tầng
dispatcher**, không phải trong từng hàm (cùng lý do như
`docs/DISPLAY_LISTS.md` §3.2).

**Đề xuất (PLAN):**

1. Thêm cờ `Context::legacy.inBeginEnd` (đã có: `LegacyState.h:141`) là nguồn
   sự thật duy nhất.
2. Tạo bảng **whitelist** `kAllowedInBeginEnd` trong `src/gl/gl_legacy_vertex.cpp`
   (mới) gồm **16 opcode** ở Bảng A + **16 opcode** ở Bảng C (Bảng C phải
   sinh lỗi ⇒ cũng nằm trong bảng "cấm nhưng phải báo lỗi").
3. Dispatcher kiểm tra: `if (inBeginEnd && !Allowed(op)) Record(0x0502)`.
   Vì dispatcher đã có sẵn cho display list (xem `docs/DISPLAY_LISTS.md` §3.2),
   **hai** cơ chế này nên dùng chung một điểm kiểm tra.
4. **Ưu tiên:** xử lý `glBegin/glEnd` **trước** khi implement display list, vì
   nó là điều kiện tiên quyết cho `Material`/`CallList`/`EvalCoord` (Bảng A).

---

## 3. CURRENT — mô hình TGLMT hiện tại

### 3.1 File và hàm chính

`src/gl/gl_legacy_vertex.cpp` (603 dòng):

| Hàm | file:line | Vai trò |
|---|---|---|
| `LegacyActiveTexture` | `:36-45` | đổi active texture unit (hiện là **dead code** — không nơi nào gọi) |
| `LegacyMultiTexCoordUnit` | `:49-58` | tra unit từ target `GL_TEXTURE0+i`, validate 0..7 |
| `ValidBeginMode` | `:61-76` | whitelist 10 mode của `glBegin` |
| `glBegin` | `:79-87` | mở capture |
| `glEnd` | `:89-95` | đóng capture + `AssemblePrimitives` |
| `AssemblePrimitives` | `:100-162` | ghép primitive theo spec |
| `AppendVertex` | `:165-180` | chụp current state vào đỉnh |
| `glVertex*` | `:182-215` | wrapper gọi `AppendVertex` |
| `SetCurrentColor` + `glColor*` | `:218-341` | current color |
| `SetCurrentNormal` + `glNormal*` | `:344-361` | current normal |
| `SetCurrentTexCoordOf` / `SetCurrentTexCoord` + `glTexCoord*` | `:365-423` | current texcoord theo unit |
| `glMultiTexCoord*` | `:427-513` | current texcoord theo target, **không** đổi active unit |
| `glSecondaryColor3*` | `:516-532` | current secondary color |
| `SetCurrentIndex` + `glIndex*` | `:535-591` | current color index (**bản đặt sai tên — xem §6 L-2**) |
| `glEdgeFlag` / `glEdgeFlagv` | `:594-601` | current edge flag |

### 3.2 Current vertex attributes

| Attribute | Nơi lưu | Mặc định | Ghi chú |
|---|---|---|---|
| Color (RGBA) | `LegacyState.h:132` `currentColor[4]` | (1,1,1,1) | `glColor3*` ⇒ alpha = 1 |
| Normal | `LegacyState.h:133` `currentNormal[3]` | (0,0,1) | |
| Secondary color | `LegacyState.h:134` `currentSecondaryColor[3]` | (0,0,0) | spec: RGBA (0,0,0,**1**) |
| Texcoord × 8 unit | `LegacyState.h:136` `currentTexCoord[8][4]` | (0,0,0,1) | `q=1` (ctor `:156`) |
| Color index | `LegacyState.h:144` `currentIndex` | 0 | color-index mode đã gỡ ở 4.6 Core |
| Edge flag | `LegacyState.h:143` `edgeFlag` | TRUE | |

**Quy tắc "chụp tại thời điểm gọi `glVertex*`"** — `AppendVertex`
(`:165-180`) copy **toàn bộ** current state vào `LegacyVertex`:

```cpp
LegacyVertex vtx;
vtx.pos[0..3] = x,y,z,w;
memcpy(vtx.color,           c.legacy.currentColor,           sizeof(vtx.color));
memcpy(vtx.normal,          c.legacy.currentNormal,          sizeof(vtx.normal));
memcpy(vtx.secondaryColor,  c.legacy.currentSecondaryColor,  sizeof(vtx.secondaryColor));
memcpy(vtx.texCoord,        c.legacy.currentTexCoord[0],      sizeof(vtx.texCoord));
memcpy(vtx.texCoord1,       c.legacy.currentTexCoord[1],      sizeof(vtx.texCoord1));
vtx.index    = c.legacy.currentIndex;
vtx.edgeFlag = c.legacy.edgeFlag;
c.legacy.immediateVertices.push_back(vtx);
```

**Hạn chế (CURRENT):** chỉ **2 texture unit** được chụp vào đỉnh
(`texCoord` = unit 0, `texCoord1` = unit 1) — dù `currentTexCoord` có 8 slot.
Spec GL 1.3 yêu cầu ≥2 unit ⇒ **đúng tối thiểu**, nhưng app dùng 3+ unit sẽ
sai. Sửa (PLAN): hoặc mở rộng `LegacyVertex` thành mảng theo unit, hoặc dùng
vertex layout cố định với `n` unit (xem `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md`
§4.5).

### 3.3 Gom đỉnh

* `glBegin` (`:79-87`): nếu đã `inBeginEnd` ⇒ `GL_INVALID_OPERATION`; nếu mode
  không hợp lệ ⇒ `GL_INVALID_VALUE` (⚠ xem §6 L-1); reset `inBeginEnd = true`,
  `beginMode = mode`, `ResetImmediate()` (xoá vector đỉnh + primitive).
* `glVertex*` → `AppendVertex` (`:165`): nếu **không** `inBeginEnd` ⇒
  `GL_INVALID_OPERATION` (spec: undefined ⇒ chặt hơn là hợp lệ).
* `glEnd` (`:89-95`): nếu không `inBeginEnd` ⇒ `GL_INVALID_OPERATION`; sau đó
  `inBeginEnd = false` và gọi `AssemblePrimitives(c.legacy)`.

**⚠ Không có draw.** File header tự thừa nhận (`:12-13`):

> "CHƯA LÀM (xem PROGRESS.md): nối primitive đã ghép vào Metal draw path ⇒ hiện
> `glEnd` chỉ ghép primitive, **chưa rasterize**. Trạng thái:
> **PARTIALLY_IMPLEMENTED**."

### 3.4 Primitive assembly

Kết quả nằm ở `LegacyState::immediatePrimitives` (`LegacyState.h:149`), mỗi
phần tử là `LegacyPrimitive { mode; idx[4]; count; }` (`LegacyState.h:119-123`).
Helper `push` (`gl_legacy_vertex.cpp:102-108`) ghi `idx[3] = idx[2]` (thừa, vô
hại — với `count ≤ 3` chỉ `idx[0..count-1]` được dùng).

---

## 4. Bảng thứ tự ghép primitive cho **tất cả** mode

Ký hiệu: `v0, v1, …` là đỉnh **0-based** trong `immediateVertices` (tức là đỉnh
thứ *i+1* theo đánh số 1-based của spec). Cột "code" là `file:line` trong
`src/gl/gl_legacy_vertex.cpp`.

| Mode | Điều kiện sinh primitive (SPEC) | Chỉ số đỉnh (0-based) | Winding | Code |
|---|---|---|---|---|
| `GL_POINTS` | mỗi đỉnh 1 point | `(v0)`, `(v1)`, … | không áp dụng | `:110-112` |
| `GL_LINES` | cặp; đỉnh lẻ cuối bị **bỏ qua** | `(v0,v1)`, `(v2,v3)`, … | không áp dụng | `:113-116` |
| `GL_LINE_STRIP` | ≥2 đỉnh; nếu chỉ 1 đỉnh ⇒ **không** primitive | `(v0,v1)`, `(v1,v2)`, `(v2,v3)`, … | không áp dụng | `:124-127` |
| `GL_LINE_LOOP` | strip + **1 cạnh khép** từ đỉnh cuối về đỉnh đầu | `(v0,v1)`, `(v1,v2)`, …, `(v_{n-2},v_{n-1})`, **`(v_{n-1},v0)`** | không áp dụng | `:117-123` |
| `GL_TRIANGLES` | bộ 3; `k` đỉnh cuối (0..2) bị bỏ qua | `(v0,v1,v2)`, `(v3,v4,v5)`, … | giữ nguyên | `:128-131` |
| `GL_TRIANGLE_STRIP` | ≥3 đỉnh; tam giác *t* dùng `v_t, v_{t+1}, v_{t+2}` | *t* chẵn: `(v_t, v_{t+1}, v_{t+2})`; *t* lẻ: **`(v_{t+1}, v_t, v_{t+2})`** | **đảo ở tam giác lẻ** | `:132-139` |
| `GL_TRIANGLE_FAN` | ≥3 đỉnh | `(v0,v1,v2)`, `(v0,v2,v3)`, `(v0,v3,v4)`, … | giữ nguyên | `:140-143` |
| `GL_QUADS` | nhóm 4; `k` đỉnh cuối (0..3) bị bỏ qua | `(v0,v1,v2)` + **`(v0,v2,v3)`** | giữ nguyên | `:144-149` |
| `GL_QUAD_STRIP` | ≥4 đỉnh; quad *i* = `(v_{2i}, v_{2i+1}, v_{2i+3}, v_{2i+2})`; số đỉnh lẻ ⇒ đỉnh cuối bị bỏ qua | quad *i*: **`(v_{2i}, v_{2i+1}, v_{2i+3})`** + **`(v_{2i}, v_{2i+3}, v_{2i+2})`** | giữ nguyên (nhờ thứ tự lộn xộn) | `:150-155` |
| `GL_POLYGON` | ≥3 đỉnh; **fan từ v0** | `(v0,v1,v2)`, `(v0,v2,v3)`, `(v0,v3,v4)`, … | giữ nguyên | `:156-159` |

### 4.1 Quy tắc `GL_QUAD_STRIP` — điểm dễ sai nhất

**SPEC (GL 2.1 §2.6.1, trích nguyên văn):**

> "If the *m* vertices between the Begin and End are v1, …, vm, where vj is the
> jth specified vertex, then **quad i has vertices (in order) v2i, v2i+1, v2i+3,
> and v2i+2** with i = 0, …, ⌊m/2⌋."

Chuyển sang chỉ số 0-based: **quad *i* = (v_{2i}, v_{2i+1}, v_{2i+3}, v_{2i+2})**.

**Vì sao sai thứ tự là đảo winding:**

Quad *i* có 4 đỉnh theo thứ tự **vòng**: `v_{2i}` → `v_{2i+1}` → `v_{2i+3}` →
`v_{2i+2}` → (quay lại `v_{2i}`). Nếu dùng thứ tự "naive"
`(v_{2i}, v_{2i+1}, v_{2i+2}, v_{2i+3})` thì vòng trở thành
`v_{2i}` → `v_{2i+1}` → `v_{2i+2}` → `v_{2i+3}` — tức là **đảo ngược** vòng so
với thứ tự spec ⇒ **winding đảo** ⇒ quad bị coi là mặt sau ⇒ bị cull, hoặc
lighting tính sai mặt.

Cụ thể với 6 đỉnh `v0..v5`:

| Quad | Thứ tự **spec** | Triangulate (spec) | Thứ tự **sai** | Triangulate (sai) |
|---|---|---|---|---|
| 0 | (v0, v1, v3, v2) | (v0,v1,v3) + (v0,v3,v2) | (v0, v1, v2, v3) | (v0,v1,v2) + (v0,v2,v3) |
| 1 | (v2, v3, v5, v4) | (v2,v3,v5) + (v2,v5,v4) | (v2, v3, v4, v5) | (v2,v3,v4) + (v2,v4,v5) |

Hai bảng tam giác khác nhau ở **cả 4 tam giác**, và mỗi tam giác có hướng
ngược nhau.

**CURRENT: code đã đúng.** `gl_legacy_vertex.cpp:150-155`:

```cpp
case 0x0008:  // GL_QUAD_STRIP → quad i = (2i, 2i+1, 2i+3, 2i+2)
    for (size_t i = 0; i + 3 < n; i += 2) {
        push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 3), 3);
        push(0x0004, (uint32_t)i, (uint32_t)(i + 3), (uint32_t)(i + 2), 3);
    }
```

Header của file cũng cảnh báo (`:15-16`):

> "Lưu ý về thứ tự ghép GL_QUAD_STRIP (spec GL 1.4 §2.6): quad thứ i dùng 4 đỉnh
> (2i, 2i+1, 2i+3, 2i+2) — nếu dùng (2i, 2i+1, 2i+2, 2i+3) thì **WINDING bị đảo**."

**Test:** `tests/unit/test_legacy_immediate.cpp:160-177` kiểm tra đúng 4 tam
giác `(0,1,3) (0,3,2) (2,3,5) (2,5,4)` ⇒ PASS.

### 4.2 Quy tắc `GL_TRIANGLE_STRIP` — winding luân phiên

**SPEC (GL 2.1 §2.6.1):**

> "Each vertex sent between a Begin/End pair **toggles the pointer**. Therefore, the
> first vertex is stored as vertex A, the second stored as vertex B, the third
> stored as vertex A, and so on. **Any vertex after the second one sent forms a
> triangle from vertex A, vertex B, and the current vertex (in that order).**"

Suy ra (0-based, tam giác *t* dùng `v_t, v_{t+1}, v_{t+2}`):

| *t* | (A, B, current) | Tam giác |
|---:|---|---|
| 0 | (v0, v1, v2) | `(v0, v1, v2)` |
| 1 | (v2, v1, v3) | `(v2, v1, v3)` = `(v_{t+1}, v_t, v_{t+2})` |
| 2 | (v2, v3, v4) | `(v2, v3, v4)` |
| 3 | (v4, v3, v5) | `(v4, v3, v5)` = `(v_{t+1}, v_t, v_{t+2})` |

⇒ **Công thức:** `t` chẵn → `(v_t, v_{t+1}, v_{t+2})`; `t` lẻ →
`(v_{t+1}, v_t, v_{t+2})`.

**CURRENT: code đúng** — `gl_legacy_vertex.cpp:132-139`:

```cpp
case 0x0005:  // GL_TRIANGLE_STRIP — winding đảo ở tam giác chẵn
    for (size_t i = 0; i + 2 < n; ++i) {
        if (i & 1)
            push(0x0004, (uint32_t)(i + 1), (uint32_t)i, (uint32_t)(i + 2), 3);
        else
            push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 2), 3);
    }
```

**Test:** `tests/unit/test_legacy_immediate.cpp:118-131` kiểm tra `(0,1,2)` và
`(2,1,3)` ⇒ PASS.

### 4.3 `GL_POLYGON` — giới hạn do spec đặt ra

**SPEC:** "Only **convex** polygons are guaranteed to be drawn correctly by the
GL. If a specified polygon is nonconvex when projected onto the window, then the
rendered polygon need only lie **within the convex hull** of the projected
vertices defining its boundary."

⇒ Triangulate bằng **fan từ v0** là hợp lệ (nằm trong convex hull). Nhưng
spec chỉ bảo đảm cho **đa giác lồi** ⇒ với đa giác lõm, kết quả có thể khác
driver thật (mọi driver đều làm ear-clipping). **Đây là sai lệch được phép
theo spec** — phải ghi vào caveat.

### 4.4 Đếm primitive

`GL_ARB_pipeline_statistics_query` (`GL_PRIMITIVES_SUBMITTED_ARB`) phải đếm
**primitive theo nghĩa GL** (1 quad = 1, không phải 2 tam giác). Đường modern
dùng `PrimCount` (`src/gl/gl_draw.cpp:95-116`); đường legacy phải đếm riêng.
Xem `docs/LEGACY_TO_METAL_MAPPING.md` §3.2.

---

## 5. Raster position / window position

### 5.1 SPEC — raster position (GL 2.1 §2.13)

> "The coordinates are treated **as if they were specified in a Vertex command**.
> If a vertex shader is active, this vertex shader is executed using the x, y, z,
> and w coordinates as the object coordinates of the vertex. Otherwise, the x, y,
> z, and w coordinates are **transformed by the current model-view and projection
> matrices**. These coordinates, along with current values, are used to generate
> primary and secondary colors and texture coordinates just as is done for a
> vertex."

> "The transformed coordinates are passed to **clipping** as if they represented a
> point. If the 'point' is not culled, then the projection to window coordinates
> is computed (section 2.11) and saved as the current raster position, and the
> **valid bit is set**. If the 'point' is culled, the current raster position and
> its associated data become **indeterminate** and the valid bit is cleared."

> "the current raster distance is set to the distance from the origin of the eye
> coordinate system to the vertex as transformed by **only the current model-view
> matrix**."

State liên quan (bảng 6.x GL 2.1): `GL_CURRENT_RASTER_POSITION` (0x0B07),
`GL_CURRENT_RASTER_POSITION_VALID` (0x0B08),
`GL_CURRENT_RASTER_DISTANCE` (đã bị gỡ ở GL 4.6), `GL_CURRENT_RASTER_COLOR`
(0x0B04, color-index mode — đã gỡ), `GL_CURRENT_RASTER_INDEX` (0x0B05 — đã gỡ).

### 5.2 SPEC — window position (GL 2.1 §2.13)

> "Alternately, the current raster position may be set by one of the WindowPos
> commands: … The current raster position, (xw, yw, zw, wc), is defined by:
> xw = x, yw = y …"

⇒ `glWindowPos*` đặt trực tiếp **toạ độ window**, **bypass** transform.
WindowPos **luôn** tạo hit record khi ở selection mode (spec §5.2).

### 5.3 Trạng thái CURRENT

| Entry point | Số lệnh | Trạng thái | Bằng chứng |
|---|---:|---|---|
| `glRasterPos{234}{sifd}[v]` | 32 | **STUB** | `src/gl/gl_stubs_gen.cpp:418-489` |
| `glWindowPos{23}{sifd}[v]` | 16 | **STUB** | `src/gl/gl_stubs_gen.cpp:683-730` |
| `glRasterPos*` / `glWindowPos*` — state | — | **MISSING** | Không có `rasterPos` trong `LegacyState.h` |

⇒ `glGetIntegerv(GL_CURRENT_RASTER_POSITION /*0x0B07*/)` trả `GL_INVALID_ENUM`
(đo) — xem `docs/FIXED_FUNCTION_PIPELINE.md` §2.2.

**Số lệnh thực tế:** `glRasterPos*` = 4 arity × 4 kiểu × 2 (có `v`/không `v`)
= 32; `glWindowPos*` = 2 arity × 4 kiểu × 2 = 16 ⇒ **48 lệnh** (khớp
`TODO.md` T1-S9).

---

## 6. Lỗi đã đo được (CURRENT)

| # | Hiện tượng | file:line | Bằng chứng / SPEC | Mức |
|---|---|---|---|---|
| **L-1** | `glBegin(mode)` với mode sai sinh `GL_INVALID_VALUE`; spec (bảng 2.3 + §2) yêu cầu **`GL_INVALID_ENUM`** | `src/gl/gl_legacy_vertex.cpp:83` (`Record(0x0501)`) | `tests/unit/test_legacy_immediate.cpp:62-63` **đang khẳng định sai** ⇒ cần sửa cả code lẫn test | Trung bình |
| **L-2** | 18 hàm `glIndex1*/2*/3*` **không tồn tại trong OpenGL** được implement; 10 `glIndex*` thật vẫn là stub | `src/gl/gl_legacy_vertex.cpp:535-591` | Đối chiếu `docs/khronos/gl.xml`: `glIndex1i`, `glIndex2f`, `glIndex3uiv`… **không có** trong registry; `src/gl/gl_c_exports.cpp` **không export** chúng ⇒ dead code | Trung bình |
| **L-3** | `glMultiTexCoord*` với target ngoài `GL_TEXTURE0..7` ⇒ `GL_INVALID_VALUE`; theo GL 4.6 §2.3 phải là `GL_INVALID_ENUM` | `src/gl/gl_legacy_vertex.cpp:52` (`Record(0x0501)`) | Spec GL 2.1 §2.7 nói "undefined behavior" ⇒ `INVALID_VALUE` cũng chấp nhận được, nhưng GL 4.6 §2.3 nói `INVALID_ENUM` | Thấp |
| **L-4** | `glActiveTexture` cho phép **32** unit (`src/gl/gl_texture.cpp:598`) nhưng `currentTexCoord` chỉ có **8** slot và `SetCurrentTexCoordOf` mask `unit & 7` (`src/gl/gl_legacy_vertex.cpp:367`) ⇒ unit 10 **alias** unit 2 | `LegacyState.h:136`, `gl_legacy_vertex.cpp:367` | Spec GL 1.3 yêu cầu ≥2 unit; TGLMT báo `GL_MAX_TEXTURE_UNITS` khác ⇒ lệch | Trung bình |
| **L-5** | Chỉ **2** texture unit được chụp vào đỉnh | `src/gl/gl_legacy_vertex.cpp:175-176` | `LegacyVertex` chỉ có `texCoord` + `texCoord1` (`LegacyState.h:110-111`) | Trung bình |
| **L-6** | `LegacyActiveTexture` là **dead code** (định nghĩa `static` nhưng không nơi nào gọi) | `src/gl/gl_legacy_vertex.cpp:36-45` | `grep` toàn repo: chỉ có dòng định nghĩa | Thấp (vô hại) |
| **L-7** | `currentSecondaryColor` chỉ có 3 thành phần | `include/tglmt/LegacyState.h:134` | Spec: RGBA (0,0,0,1) | Thấp |
| **L-8** | **Chưa có draw path**: `glEnd` chỉ ghép primitive, không rasterize | `src/gl/gl_legacy_vertex.cpp:12-13`, `:89-95` | `TODO.md` B.1 (blocker lớn nhất của T1) | **Blocker** |

---

## 7. Lộ trình nối vào Metal draw path

Nền tảng: `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4 (thiết kế đường legacy),
`docs/LEGACY_TO_METAL_MAPPING.md` §3 (batching + primitive expansion).

### 7.1 Trình tự (PLAN)

| Stage | Nội dung | File | Test ID | Exit criteria |
|---|---|---|---|---|
| **M0** *(= TODO B.2)* | **Bảng lệnh bị cấm trong Begin/End** (16 họ được phép + 16 lệnh lỗi tuỳ chọn) + dispatcher chặn | `src/gl/gl_legacy_vertex.cpp` (bảng whitelist), `tools/gen_c_exports.py` (dispatcher) | `test_legacy_immediate` (mở rộng) | `glEnable(GL_DEPTH_TEST)` trong Begin/End ⇒ `GL_INVALID_OPERATION`; `glMaterialfv` trong Begin/End ⇒ **không** lỗi; `glCallList` trong Begin/End ⇒ không lỗi |
| **M1** | Sửa lỗi L-1 (`INVALID_ENUM` cho `glBegin` mode sai) + sửa test | `src/gl/gl_legacy_vertex.cpp:83`, `tests/unit/test_legacy_immediate.cpp:62` | `test_legacy_immediate` | Test khẳng định `GL_INVALID_ENUM` |
| **M2** | Sửa L-2: xoá 18 hàm `glIndex*` phantom, implement 10 `glIndex*` thật; sửa L-3 (enum), L-4 (số unit), L-5 (nhiều unit/đỉnh), L-7 (secondary color RGBA) | `src/gl/gl_legacy_vertex.cpp`, `include/tglmt/LegacyState.h` | `test_legacy_immediate` (mở rộng) | `glIndex(GL_QUAD_STRIP)` không còn là stub; `glGetFloatv(GL_CURRENT_SECONDARY_COLOR)` trả 4 thành phần |
| **M3** | **Bộ chuyển đổi primitive → Metal**: `GL_LINES`/`GL_LINE_STRIP` → `MTLPrimitiveTypeLine`; `GL_LINE_LOOP` → `LineStrip` + chép đỉnh đầu cuối; `GL_POINTS` → `Point`; tam giác → `Triangle` | `src/gl/gl_apple_draw.cpp` (mẫu có sẵn ở `:1937-2006`) | `test_legacy_draw_apple` (mới, GPU) | 10 mode vẽ ra hình đúng |
| **M4** | **Vertex layout legacy** + **chương trình MSL sinh** + nối vào `AppleDrawGL` | `src/gl/gl_legacy_program.cpp` (mới), `include/tglmt/LegacyProgram.h` (mới), `src/gl/gl_apple_draw.cpp` | `test_legacy_draw_apple` | Tam giác fixed-function vẽ đúng màu; `PipelineKey` không cần field mới |
| **M5** | **Batching**: gom nhiều `glBegin/glEnd` cùng state thành 1 draw | `src/gl/gl_apple_draw.cpp` + ring allocator (`Context::RingAlloc`, `include/tglmt/Context.h:1235`) | `test_legacy_batch_apple` (mới) | 10 000 quad ⇒ **1** `drawPrimitives`; đo bằng `Context::appleStats` |
| **M6** | **Raster position / window position** (48 lệnh) | `src/gl/gl_legacy_rasterpos.cpp` (mới) | `test_legacy_rasterpos` (mới) | Raster position đúng §2.13; window position bypass transform |
| **M7** | **`glRect*`** (8 lệnh) — không dùng Begin/End, tương đương một rectangle primitive | `src/gl/gl_legacy_vertex.cpp` | `test_legacy_immediate` (mở rộng) | `glRectf` vẽ đúng hình chữ nhật, dùng current color/texcoord tại thời điểm gọi |

### 7.2 Quy tắc bắt buộc cho batching (PLAN)

Lấy từ `docs/LEGACY_TO_METAL_MAPPING.md` §3.1 — một batch phải **bị phá** khi:

1. đổi primitive mode;
2. đổi pipeline state (blend, depth func, cull, front face, viewport, scissor,
   depth bias, color mask);
3. đổi **legacy shading state** (lights, material, fog, texenv, texgen, matrix,
   shade model) — vì chương trình sinh giống nhau nhưng **uniform** khác, batch
   sẽ áp giá trị mới cho mọi đỉnh đã gom;
4. đổi render target;
5. có lệnh đọc framebuffer (`glReadPixels`, `glCopyPixels`, `glAccum` với
   `GL_RETURN`);
6. vào/ra selection hoặc feedback mode;
7. `glFlush` / `glFinish` / present.

Các mục 1, 2, 4 đã có cơ chế dirty-check sẵn trong `StateTracker`
(`include/tglmt/StateTracker.h:257-259`) ⇒ **tái sử dụng**, không viết lại.

### 7.3 Layout vertex legacy (PLAN)

Theo `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4.5:

| location | attribute | kiểu | nguồn |
|---|---|---|---|
| 0 | `position` | float4 | `glVertex*` x,y,z,w |
| 1 | `normal` | float3 | `glNormal*` |
| 2 | `color` | float4 | `glColor*` (hoặc giá trị đã resolve `glColorMaterial`) |
| 3 | `texcoord` | float4 | `glTexCoord*` của unit đang bind, **sau** khi nhân texture matrix |
| 4 | `fogCoord` | float1 | chỉ khi `GL_FOG_COORD_SRC == GL_FOG_COORD` |
| 5 | `flatColor` | float4 | chỉ khi `shadeModel == GL_FLAT` |

---

## 8. Điểm chưa verify

| # | Điểm | Vì sao chưa verify |
|---|---|---|
| V-1 | Ảnh output của 10 mode | Chưa có draw path (L-8) ⇒ chưa có ảnh nào để so với reference |
| V-2 | Hành vi `GL_POLYGON` với đa giác **lõm** | Spec chỉ bảo đảm lồi ⇒ khác driver là hợp lệ; cần reference để biết chấp nhận được không |
| V-3 | Tương tác `glBegin/glEnd` với `glEvalCoord*` (được phép ở Bảng A) | `glEvalCoord*` là stub |
| V-4 | Tương tác `glBegin/glEnd` với `glCallList` (được phép ở Bảng A) | `glCallList` là stub |
| V-5 | Số texture unit legacy tối đa nên dùng | L-4; chưa chốt |
| V-6 | Nhãn môi trường `A11_DEVICE` | Toàn bộ đo chạy Null backend |

---

## 9. Tham chiếu chéo

* `docs/FIXED_FUNCTION_PIPELINE.md` — bảng state, lỗi đã đo (L-2, L-7 trùng với §6).
* `docs/DISPLAY_LISTS.md` — `glCallList`/`glCallLists` được phép trong Begin/End.
* `docs/LEGACY_TEXTURE_AND_LIGHTING.md` — `glMaterial` được phép trong Begin/End; `glColorMaterial`.
* `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §4 — thiết kế đường draw legacy.
* `docs/LEGACY_TO_METAL_MAPPING.md` §3 — batching + primitive expansion.
* `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` §1 (S3, S8), §0.3 — quy ước stage & test.
* `docs/LEGACY_PIPELINE_AUDIT.md` §1 — audit gốc của nhóm L01 (lưu ý: audit viết **trước** khi `gl_legacy_vertex.cpp` tồn tại; trạng thái "MISSING" ở đó là **stale** so với code hiện tại).
