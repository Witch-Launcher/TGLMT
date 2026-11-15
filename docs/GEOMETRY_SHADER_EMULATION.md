# GEOMETRY_SHADER_EMULATION — Emulation Geometry Shader trên Metal (TGLMT)

> Quyết định kiến trúc liên quan: **A6** (`docs/DECISIONS.md`, `AGENTS.md` §2) —
> *"Geometry shader BẮT BUỘC phải emulation. Không được kết luận `HARDWARE_LIMITED`
> cho GS chỉ vì Metal không có GS stage."*
>
> Trạng thái: **`EMULATED`** (chạy thật trên Metal, có pixel thật kiểm chứng).
> Nhãn môi trường kiểm chứng: `OTHER_GPU` (Mac có Metal device) — **chưa** chạy
> trên `A11_DEVICE`.

---

## 1. Bối cảnh

Metal **không có** stage geometry shader. Trước khi có file này, TGLMT:

* `src/gl/gl_program.cpp` gắn cờ `pr.hasGeometryStage = true` khi program có GS
  ⇒ **link OK**;
* `src/gl/gl_apple_draw.cpp` **từ chối vẽ** ⇒ draw im lặng không sinh pixel.

Việc "link OK rồi vẽ câm" vi phạm nếp P6f của repo (fail rõ tại link thay vì
link OK nhưng vẽ sai/câm). Bản này thay bằng emulation thật: **GS → MSL compute
kernel + draw buffer mở rộng**, đúng hình dạng tiền lệ của đường tessellation
(TCS → compute kernel, `src/gl/gl_tess_xfb.cpp`).

---

## 2. Kiến trúc: 3 pass

```
 (1) VS capture prepass            (2) GS kernel                 (3) Draw
 ─────────────────────────         ────────────────────         ─────────────────
 compute "TGLMT_vs_capture"        compute "TGLMT_gs"            vertex "TGLMT_gs_vs"
 1 thread / 1 vertex               1 thread / 1 input primitive   đọc buffer mở rộng
 đọc attribute qua descriptor      đọc buffer capture             → varying cho FS
 → ghi buffer capture              → ghi buffer MỞ RỘNG + index   fragment "TGLMT_fs"
   (gl_Position, gl_PointSize,        (mỗi field 1 buffer)        drawIndexed
    VS output varyings)
```

Vì sao phải có pass (1)? Metal không cho vertex function ghi kết quả sang stage
kế tiếp (vertex → fragment), và cũng không có "geometry stage". Cách chuẩn (giống
MoltenVK cho tessellation): biến VS thành **compute kernel** để ghi output của
VS vào buffer, rồi đọc buffer đó trong kernel tiếp theo. Repo đã có sẵn cơ chế
này cho TCS (`ConvertGLSLtoMSL(src, GL_VERTEX_SHADER, /*vertexCapture=*/true)`).

Pass (3) dùng **passthrough vertex**: một vertex function không có `stage_in`
attribute, chỉ đọc buffer mở rộng bằng `[[vertex_id]]` + `[[buffer(n)]]`.

Code:

| Thành phần | File:dòng |
|---|---|
| Translator GLSL→MSL + runtime | `src/gl/gl_geometry_emulation.cpp` |
| Header (API nội bộ) | `include/tglmt/GeometryEmulation.h` |
| Link path (dịch + fail rõ) | `src/gl/gl_program.cpp` (`PrepareGeometryProgram`, `FinishGeometryProgram`) |
| Draw path (gọi emulation) | `src/gl/gl_apple_draw.cpp` (`AppleDrawGL` → `GeometryStageDraw`) |
| Lượt vẽ cuối (passthrough + FS) | `src/gl/gl_apple_draw.cpp` (`AppleDrawGeometryExpanded`) |
| Test | `tests/unit/test_geometry_emulation.cpp` |

---

## 3. Bảng ánh xạ GLSL → MSL compute

| GLSL (GS) | MSL trong kernel `TGLMT_gs` | Ghi chú |
|---|---|---|
| `layout(triangles) in;` | `tglmt_p.inVerts = 3` + mode ghép primitive | `points`=1, `lines`=2, `triangles`=3 || `layout(triangle_strip, max_vertices=N) out;` | `tglmt_p.maxVerts = N`; output type quyết định cách sinh index | chỉ `points`/`line_strip`/`triangle_strip` (GL 4.2C §2.16.2) |
| `gl_in[i].gl_Position` | `tglmt_cap_gl_Position[tglmt_base + tglmt_slotof(tglmt_prim, (i), …)]` | `gl_in[]` đọc từ buffer capture của VS |
| `gl_in[i].gl_PointSize` | `tglmt_cap_gl_PointSize[…]` | |
| `gl_in[i].gl_ClipDistance[]` | **không hỗ trợ** | link fail |
| `vColor[i]` (input của GS) | `tglmt_cap_vColor[tglmt_base + tglmt_slotof(…)]` | `vColor` là output của VS, capture vào buffer riêng |
| `gl_Position` (ghi) | `tglmt_cur_pos` → copy vào buffer mở rộng khi `EmitVertex()` | |
| `gl_PointSize` (ghi) | `tglmt_cur_psize` | |
| `gl_Layer` (ghi) | `tglmt_cur_layer` → buffer riêng (chưa dùng cho layered rendering) | |
| `gl_ViewportIndex` (ghi) | `tglmt_cur_vpidx` → buffer riêng | |
| `gl_PrimitiveID` (ghi) | **không hỗ trợ** | link fail |
| `gl_PrimitiveIDIn` (đọc) | `tglmt_primid` = `inst*primsPerInst + prim` | GL 4.2C §2.16.4 |
| `gl_InvocationID` (đọc) | `tglmt_inv` = 0 | `layout(invocations=N)` (N>1) ⇒ link fail |
| `uniform float u;` | `tglmt_u.u` (struct `TGLMTUniforms` **của VS**) | GS phải dùng chung uniform với VS |
| `uniform sampler2D t;` | **không hỗ trợ** | link fail |
| `EmitVertex()` | `{ if (tglmt_n < tglmt_p.outVertsCap) { ghi tất cả field output; tglmt_n++; } }` | giới hạn bởi `max_vertices` |
| `EndPrimitive()` | `{ dựng index của primitive output từ các vertex đã emit; ghi vào buffer index; tglmt_pstart = tglmt_n; }` | |
| `EmitStreamVertex()` / `EndStreamPrimitive()` | **không hỗ trợ** | link fail |
| `vec3`/`ivec2`/… (kiểu cục bộ) | `float3`/`int2`/… | đổi tên kiểu GLSL→MSL trong thân `main` |
| hàm phụ (helper function) | **không hỗ trợ** | link fail (MSL không có hàm đó) |
| `#define`/`#include` | **không hỗ trợ** | link fail (chỉ `#version` được chấp nhận) |

### 3.1. Ghép primitive input (mode của lệnh draw)

`gl_in[k]` không phải luôn là `vertex thứ k` của một danh sách liên tiếp: với
strip/fan, các primitive dùng chung đỉnh. Hàm `tglmt_slotof()` (file-scope,
sinh ra trong MSL) ánh xạ `(primitive p, đỉnh k)` → *slot* trong buffer capture:

| mode draw | slot của đỉnh k của primitive p |
|---|---|
| `GL_POINTS` | `p` |
| `GL_LINES` | `2p + k` |
| `GL_LINE_STRIP` | `p + k` |
| `GL_LINE_LOOP` | `(p + k) % n` |
| `GL_TRIANGLES` | `3p + k` |
| `GL_TRIANGLE_STRIP` | `p + k` |
| `GL_TRIANGLE_FAN` | `k == 0 ? 0 : p + k` |

Slot được cộng thêm `tglmt_base = inst * vertsPerInst` (offset instance).

### 3.2. Sinh index cho primitive output

Output của GS được **mở rộng thành list** (Metal không có "strip sinh từ
compute"): `points` → 1 index/vertex, `line_strip` → 2 index/cặp,
`triangle_strip` → 3 index/tam giác (đảo winding ở tam giác lẻ theo quy tắc
strip). Hình học kết quả giống hệt strip tương ứng (GL 4.2C §2.16.2: "The
vertices output by the geometry shader are assembled into points, lines, or
triangles based on the output primitive type in the manner described in
section 2.6.1").

---

## 4. Layout buffer

### 4.1. Buffer capture (output của VS)

| Buffer | Nội dung | Ghi bởi | Đọc bởi |
|---|---|---|---|
| `cap_<varying>` | output của VS, **1 phần tử / vertex** | capture kernel | GS kernel |
| `cap_gl_Position` | `gl_Position` của VS (clip space) | capture kernel | GS kernel (`gl_in[]`) |
| `cap_gl_PointSize` | `gl_PointSize` của VS | capture kernel | GS kernel (`gl_in[]`) |

Chỉ số = *vertex slot* = `instance * count + lvid` (lvid = chỉ số trong draw,
không phải chỉ số vertex khi draw có index — capture kernel tự tra index).

### 4.2. Buffer output mở rộng (output của GS)

**Mỗi field 1 buffer riêng** (SoA) — cố ý *không* dùng một struct MSL: struct
MSL có padding (`float3` = 12 byte nhưng align 16) ⇒ phải tính layout thủ công,
lệch 1 byte là hỏng hình. SoA loại bỏ hoàn toàn rủi ro layout.

| Buffer | Kiểu | Ghi |
|---|---|---|
| `out_pos` | `float4` | `gl_Position` |
| `out_psize` | `float` | `gl_PointSize` |
| `out_layer` | `uint` | `gl_Layer` |
| `out_vpidx` | `uint` | `gl_ViewportIndex` |
| `out_<user>` | kiểu GLSL tương ứng | output do user khai báo |

Kích thước: `totalPrims * outVertsCap * sizeof(field)`, trong đó
`outVertsCap = vertsPerInput` (emission tĩnh) hoặc `max_vertices` (emission
động).

### 4.3. Buffer index

`uint` mỗi index trỏ tới **vertex slot tuyệt đối** trong buffer output
(`tglmt_obase + i`, `tglmt_obase = tid * outVertsCap`).

* **Emission tĩnh** (mọi `EmitVertex`/`EndPrimitive` nằm trên đường thẳng):
  layout index **tĩnh** — `EndPrimitive` thứ i ghi tại `tglmt_ibase + offset_i`
  ⇒ **không cần atomic**, số index biết trước (`totalPrims * idxPerPrim`).
* **Emission động** (`EmitVertex` trong `if`/`for`/…): dùng
  `metal::atomic_fetch_add_explicit` để cấp vùng index và đếm primitive; số
  index thật đọc từ counter **sau khi GPU chạy xong** (`CommitAndWait`).

### 4.4. Buffer counter (trường hợp động)

`atomic_uint[4]`: `[0]` = số index, `[1]` = số vertex, `[2]` = số primitive.

### 4.5. Buffer tham số

```c
struct TGLMT_GsParams {
  uint primsPerInst;  // số primitive input mỗi instance
  uint vertsPerInst;  // số vertex của draw (count)
  uint inVerts;       // số vertex / primitive input (1/2/3)
  uint maxVerts;      // max_vertices
  uint outVertsCap;   // sức chứa vertex output / primitive
  uint idxPerPrim;    // số index / primitive input (layout tĩnh)
  uint instances;     // số instance
  uint mode;          // mã ghép primitive (bảng §3.1)
};
```

### 4.6. Slot buffer Metal

| Kernel | Slot dùng |
|---|---|
| `TGLMT_vs_capture` | `0..15` vertex buffer, `16` uniform, `17..` capture varying, `28` index, `29` draw info, `30` attribute descriptor (quy ước có sẵn của repo) |
| `TGLMT_gs` | `0..` capture buffer, `+1` params, `+2..` buffer output mở rộng, `+1` index, `+1` counter, `28` uniform (struct `TGLMTUniforms` của VS) |
| `TGLMT_gs_vs` (vertex) | `0` = `out_pos`, `1..` = output user mà FS đọc |

---

## 5. Dispatch

| Pass | Số thread | Threadgroup | Dispatch |
|---|---|---|---|
| capture (VS) | `count * instances` | 64 | `ceil(verts/64)` group × 64 thread |
| GS | `prims * instances` (1 thread / 1 input primitive) | 64 | `ceil(prims/64)` group × 64 thread |
| draw | — | — | `drawIndexed(Point|Line|Triangle, indexCount, UInt32)` |

Mỗi pass là một compute encoder riêng, `endAndCommit()` trước pass sau ⇒ thứ tự
đúng theo hàng GL. Trước khi encode, `FlushPendingEncoder()` +
`FlushAllBufferStaging()` + `FlushAllTextureStaging()` để dữ liệu vertex buffer
mới nhất được thấy.

---

## 6. Đường link (khi nào FAIL)

Link **fail kèm info log** (không bao giờ "link OK rồi vẽ câm"):

| Điều kiện | Message (rút gọn) |
|---|---|
| Program có GS mà thiếu VS hoặc FS | `program có geometry shader cần đủ VS + GS + FS` |
| GS thiếu `layout(<prim>) in;` | `GS thiếu input layout qualifier` |
| GS thiếu `layout(<prim>, max_vertices=N) out;` | `GS thiếu output layout qualifier` / `GS thiếu max_vertices` |
| Output primitive type ≠ `points`/`line_strip`/`triangle_strip` | `output primitive type của GS chỉ được points/line_strip/triangle_strip` |
| `max_vertices > 32` | `max_vertices = N vượt ngưỡng emulation GS (32)` |
| `layout(invocations = N)`, N > 1 | `GS nhiều invocation ... chưa hỗ trợ` |
| Input adjacency (`lines_adjacency`, `triangles_adjacency`) | `input primitive adjacency của GS chưa hỗ trợ` |
| GS dùng `sampler`/`image`/`mat*` | `uniform của GS có kiểu chưa hỗ trợ` |
| GS dùng `gl_ClipDistance`/`gl_CullDistance` | `gl_ClipDistance chưa hỗ trợ trong emulation GS` |
| GS ghi `gl_PrimitiveID` | `gl_PrimitiveID (output của GS) chưa hỗ trợ` |
| GS dùng `EmitStreamVertex`/`EndStreamPrimitive` | `vertex stream ... chưa hỗ trợ` |
| GS có hàm phụ | `GS chỉ hỗ trợ thân main trực tiếp` |
| GS có `#define`/`#include` | `preprocessor directive chưa hỗ trợ trong GS` |
| Input của GS không có output tương ứng ở VS | `GS input `X` không có vertex shader output tương ứng` |
| Kiểu input GS ≠ kiểu output VS | `type mismatch GS input `X`` |
| Uniform của GS không có (hoặc khác kiểu) ở VS | `GS uniform `X` phải khai báo giống vertex shader` |
| FS đọc input không do GS ghi | `fragment input không do geometry shader ghi: X` |
| GS emit nhiều hơn `max_vertices` (đường tĩnh) | `GS emit N vertex/lần gọi vượt max_vertices` |
| VS attribute có location > 15 | `attribute `X` có location N ngoài 0..15` |
| XFB (`glTransformFeedbackVaryings`) + GS | `XFB + geometry shader chưa hỗ trợ capture` |
| MSL compile lỗi (Metal compiler) | `MSL GS kernel compile: <lỗi Metal>` + excerpt MSL |

Ở **draw time**: mode draw không tương thích với input primitive của GS ⇒
`GL_INVALID_OPERATION` (GL 4.2C §2.16.1) — ví dụ GS `layout(triangles) in` mà
app gọi `glDrawArrays(GL_LINES, …)`.

---

## 7. Query counter

| Query | Nguồn | Ghi chú |
|---|---|---|
| `GL_GEOMETRY_SHADER_INVOCATIONS` (0x887F) | `Context::pipeStats.gsInvocations` | = số primitive input × instance |
| `GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED` (0x82F3) | `Context::pipeStats.gsPrimsWritten` | emission tĩnh: tính trên CPU; emission động: đọc counter GPU |

Counter được cập nhật **cả trên backend Null** (đường draw gọi
`GeometryStageDraw` trước khi kiểm tra `device->isNull()`).

---

## 8. Giới hạn đã biết (nói thẳng)

1. **`glCompileShader(GL_GEOMETRY_SHADER)` vẫn báo COMPILE_STATUS = FALSE.**
   `src/gl/gl_shader.cpp` (ngoài write-scope của task này) chặn mọi stage ngoài
   VS/FS/CS/TCS/TES ⇒ `infoLog = "error: geometry shader ngoài subset"`.
   Link path của TGLMT **bỏ qua** cờ đó và dịch GS bằng translator riêng, nên
   *sau khi link thành công* `glGetShaderiv(GL_COMPILE_STATUS)` được đặt lại
   thành TRUE (trong `PrepareGeometryProgram`). **Cần sửa**:
   `src/gl/gl_shader.cpp` — khi `stage == GL_GEOMETRY_SHADER`, gọi
   `TranslateGeometryShader()` và đặt `compiled`/`infoLog` theo kết quả.
   Đến lúc đó, app kiểm tra COMPILE_STATUS *trước* link vẫn thấy FALSE ⇒
   app đó sẽ không link (hành vi lệch spec, đã ghi ở đây).
2. **`glGetProgramiv(GL_GEOMETRY_VERTICES_OUT / _INPUT_TYPE / _OUTPUT_TYPE)` chưa
   trả giá trị** (đường query nằm ở `src/gl/gl_get.cpp`, ngoài write-scope).
   Cần sửa: đọc từ side-table GS (`gl::GetGeometryProgramInfo`) và trả
   `max_vertices` / `GL_POINTS|GL_LINES|GL_TRIANGLES` /
   `GL_POINTS|GL_LINE_STRIP|GL_TRIANGLE_STRIP`.
3. **`gl_Layer` / `gl_ViewportIndex`** được ghi vào buffer nhưng **không** dùng
   cho layered rendering (Metal không có `[[render_target_array_index]]` ở
   đường này).
4. **Vertex stream** (`EmitStreamVertex`) không hỗ trợ.
5. **Input adjacency** (`lines_adjacency`, `triangles_adjacency`) không hỗ trợ.
6. **`gl_PrimitiveID` là output của GS** không hỗ trợ (chưa nối tới
   `gl_PrimitiveID` của FS).
7. **Hàm phụ, `#define`, `#include`, `double`, mảng trong GS** không hỗ trợ.
8. **Sampler trong GS** không hỗ trợ (GS dùng chung uniform buffer của VS).
9. **`max_vertices` ≤ 32** (giới hạn implementation-defined; giá trị này là
   lựa chọn của TGLMT, không phải yêu cầu của spec).
10. **XFB + GS** không hỗ trợ.
11. Với **emission động** phải `CommitAndWait()` để đọc counter ⇒ stall CPU
    (chấp nhận được: đường tĩnh — trường hợp phổ biến — không cần).
12. `gl_PrimitiveIDIn` đếm liên tiếp qua các instance của **cùng lệnh draw**
    (không đếm qua các lệnh draw trước — đúng "Begin ngầm" của GL).

---

## 9. Trích dẫn spec

| Nội dung | Nguồn |
|---|---|
| Geometry shader: stage, input/output primitive, `max_vertices`, query | OpenGL 4.2 Compatibility Profile, **§2.16** (bản GL 4.6 Core tương ứng §11.3) |
| Input primitive type ↔ draw mode, `INVALID_OPERATION` | OpenGL 4.2C **§2.16.1** |
| Output primitive type chỉ `points`/`line_strip`/`triangle_strip` | OpenGL 4.2C **§2.16.2** |
| Ghép primitive output (theo §2.6.1) | OpenGL 4.2C **§2.16.2** |
| `gl_in[]`, `gl_PrimitiveIDIn`, uniform của GS | OpenGL 4.2C **§2.16.3**, **§2.16.4** |
| Input layout qualifier (`points`/`lines`/…), bảng kích thước mảng | GLSL **§4.3.8.1** (Input Layout Qualifiers) |
| Output layout qualifier (`points`/`line_strip`/`triangle_strip`, `max_vertices`) | GLSL **§4.3.8.2** (Output Layout Qualifiers) |
| `EmitVertex()`, `EndPrimitive()` | GLSL **§8.10** (Geometry Shader Functions) |
| Built-in của GS (`gl_in[]`, `gl_PrimitiveIDIn`, `gl_PrimitiveID`, `gl_Layer`) | GLSL **§7** (Built-in Variables) |

> **Ghi chú về nguồn GLSL:** bản `GLSLangSpec.3.20.pdf` **không** được Khronos
> publish riêng (registry chỉ có 3.30 trở lên cho nhánh này; GLSL 3.20 là phiên
> bản đi kèm OpenGL 3.2). Văn bản §4.3.8.1/§4.3.8.2/§8.10 của GLSL 3.30 được
> đối chiếu và **giống hệt** GLSL 3.20 cho phần geometry shader (3.30 chỉ bổ
> sung `gl_PrimitiveID`/`gl_ViewportIndex` ở §7).

---

## 10. Test

`tests/unit/test_geometry_emulation.cpp` (đăng ký tự động qua `file(GLOB)`):

1. **Translator**: layout parse, phân tích emission tĩnh/động, nội tuyến
   `EmitVertex`/`EndPrimitive`, mapping `gl_in[]`/input/output, sinh kernel MSL.
2. **Plan**: số primitive input / invocation / vertex mở rộng / primitive output,
   instancing, mode mismatch.
3. **Link**: VS+GS+FS link OK; thiếu VS / thiếu FS ⇒ fail + log; 7 ca GS ngoài
   subset ⇒ fail + log; `GL_ATTACHED_SHADERS`, `GL_SHADER_TYPE`,
   `GL_COMPILE_STATUS` sau link.
4. **Counter**: `GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED` = 3 (9 đỉnh / 3 tam
   giác), `GL_GEOMETRY_SHADER_INVOCATIONS` = 3, instancing ⇒ 6.
5. **Mode mismatch**: `glDrawArrays(GL_LINES)` với GS `triangles` ⇒
   `GL_INVALID_OPERATION`.
6. **Side-table**: program GS phải có trong bảng sau link, có field cho FS.
7. **MSL thật**: biên dịch kernel + passthrough bằng compiler Metal thật.
8. **Pixel thật** (backend Metal): 5 ca — GS biến 1 tam giác nhỏ thành tam giác
   lớn (A), thành hình vuông 2 tam giác (B), GS có biến cục bộ + uniform dùng
   chung với VS (C), GS `flat out` → FS `flat in` (D), GS `points`→`line_strip`
   với draw **indexed** (E). Pixel trong vùng mở rộng phải có đúng màu của GS,
   ngoài vùng đó là màu clear.

**Non-vacuity**: đã cố tình làm hỏng phần emit (bỏ `tglmt_n++`) ⇒ **5 ca pixel
FAIL** với thông báo cụ thể (`pixel(32,32) = 0,0,0`), sau đó khôi phục ⇒ PASS.

Chạy:

```bash
cmake --build build-work -j8 --target test_geometry_emulation
./build-work/test_geometry_emulation
```
