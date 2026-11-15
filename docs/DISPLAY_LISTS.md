# DISPLAY_LISTS.md — thiết kế display list cho TGLMT

Tài liệu kỹ thuật dùng để **implement** display list của OpenGL Compatibility
Profile trên TGLMT (OpenGL → Metal, iOS / Apple A11).

**Nguồn spec:** OpenGL 2.1 Specification (`glspec21.pdf`, 01-Dec-2006) —
**§5.4 "Display Lists"** (trang 240-244 của PDF) và §5.5 (Flush/Finish).
Bản PDF không nằm trong repo; đối chiếu cục bộ: `docs/khronos/glspec46.core.pdf`
(App. E.2.2 xác nhận display list đã bị gỡ khỏi Core Profile 3.2 ⇒ mọi entry
point trong tài liệu này là **COMPAT-ONLY**).

**Nhãn:** `SPEC` (theo đặc tả) · `CURRENT` (code thật trong repo, kèm
`file:line`) · `PLAN` (thiết kế đề xuất).
Trạng thái theo bộ `AGENTS.md` §6.

**Môi trường đo:** Null backend, build Debug (`build-work/`), nhãn `NONE`.

---

## 0. Tóm tắt trạng thái (CURRENT, đo được)

| Entry point | Nhóm | Trạng thái | Bằng chứng |
|---|---|---|---|
| `glGenLists` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:134-136` |
| `glNewList` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:349-351` |
| `glEndList` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:65-67` |
| `glCallList` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:23-25` |
| `glCallLists` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:26-28` |
| `glDeleteLists` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:50-52` |
| `glIsList` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:231-233` |
| `glListBase` | L07 | **STUB** | `src/gl/gl_stubs_gen.cpp:259-261` |

⇒ **8/8 entry point display list chưa implement** (0 `FULLY_IMPLEMENTED`).
Tổng số entry point legacy còn stub: xem `docs/FIXED_FUNCTION_PIPELINE.md` §2.

Đo thêm: `glGetIntegerv(GL_DISPLAY_LIST /*0x82E7*/)` trả `0` **không lỗi**
(`include/tglmt/gl46_pnames.h:368`) — 0x82E7 **không phải** pname trong GL
(enum chết), xem `docs/FIXED_FUNCTION_PIPELINE.md` §6 L-7.

---

## 1. SPEC — semantics đầy đủ

### 1.1 Mô hình (GL 2.1 §5.4)

> "A display list is simply a group of GL commands and arguments that has been
> stored for subsequent execution. … Doing so causes the commands within the list
> to be executed **just as if they were given normally**. The only exception
> pertains to commands that rely upon **client state**. When such a command is
> accumulated into the display list (that is, **when issued, not when executed**),
> the client state in effect at that time applies to the command. **Only server
> state is affected when the command is executed.** As always, pointers which are
> passed as arguments to commands are dereferenced when the command is issued."

Ba quy tắc đây là **xương sống** của toàn bộ thiết kế:

| # | Quy tắc | Hệ quả thiết kế |
|---|---|---|
| R1 | Lệnh trong list được thực thi **như thể gọi trực tiếp** | Replay phải gọi lại **chính** hàm GL đó, không phải một "draw" rút gọn |
| R2 | **Client state** được chụp lúc **ghi** (compile) | Record phải mang theo giá trị client tại thời điểm record (current color, current texcoord, con trỏ array…) |
| R3 | **Server state** được đọc lúc **thực thi** (replay) | Matrix, lighting, texture env, viewport… **không** được bake vào record |

R2/R3 là lý do **không thể** biến display list thành "cache draw call" — xem §4.

### 1.2 Vòng đời

| Hàm | Semantics (SPEC) |
|---|---|
| `glNewList(GLuint n, GLenum mode)` | `n` là số nguyên dương; `mode ∈ {GL_COMPILE, GL_COMPILE_AND_EXECUTE}`. `GL_COMPILE` ⇒ **không** thực thi lệnh; `GL_COMPILE_AND_EXECUTE` ⇒ thực thi **và** ghi vào list. `n == 0` ⇒ `GL_INVALID_VALUE`. Sau `glNewList`, **mọi** lệnh GL kế tiếp được ghi vào list theo thứ tự phát ra, cho tới khi `glEndList`. |
| `glEndList(void)` | Kết thúc list, GL trở lại trạng thái thực thi bình thường. **Chỉ tại `glEndList`** list mới thực sự được gắn với chỉ số `n`. `GL_INVALID_OPERATION` nếu `glEndList` không có `glNewList` tương ứng, hoặc `glNewList` được gọi lần thứ hai trước `glEndList`. `GL_OUT_OF_MEMORY` nếu không lưu được — **khi đó nội dung cũ của list và mọi state khác của GL đều không đổi**, trừ state do việc thực thi ở chế độ `GL_COMPILE_AND_EXECUTE`. |
| `glCallList(GLuint n)` | Thực thi list `n`, **theo thứ tự lệnh**, "just as if they were issued without using a display list". `n == 0` ⇒ `GL_INVALID_VALUE`. Chỉ số không tồn tại ⇒ không có tác dụng. |
| `glCallLists(GLsizei n, GLenum type, const void* lists)` | Thực thi `n` list. Mỗi phần tử của `lists` được đọc theo kiểu `type` rồi **cộng** với `glListBase` để ra chỉ số list; các list được thực thi **theo thứ tự**. `n == 0` ⇒ không làm gì. |
| `glGenLists(GLsizei s)` | Trả `n` sao cho `n … n+s−1` là `s` chỉ số **chưa dùng liên tiếp**. **Đồng thời tạo một list rỗng cho mỗi chỉ số** đó (⇒ các chỉ số trở thành "đã dùng"). Trả `0` nếu không có `s` chỉ số chưa dùng **liên tiếp**, hoặc `s == 0`. |
| `glIsList(GLuint list)` | `GL_TRUE` nếu `list` là chỉ số của một display list. |
| `glDeleteLists(GLuint list, GLsizei range)` | Xoá `range` list liên tiếp bắt đầu từ `list`; mọi thông tin bị mất, chỉ số trở lại "chưa dùng". Chỉ số không tương ứng list nào bị **bỏ qua**. `range == 0` ⇒ không làm gì. |
| `glListBase(GLuint base)` | Đặt offset dùng cho `glCallLists`. Ban đầu = 0. |

### 1.3 Kiểu dữ liệu của `glCallLists` (bảng offset)

`type` có thể là (SPEC §5.4):

| `type` | Cách đọc mỗi offset |
|---|---|
| `GL_BYTE` | 1 byte có dấu → int (cắt phần thập phân nếu là float) |
| `GL_UNSIGNED_BYTE` | 1 byte không dấu |
| `GL_SHORT` | 2 byte có dấu |
| `GL_UNSIGNED_SHORT` | 2 byte không dấu |
| `GL_INT` | 4 byte có dấu |
| `GL_UNSIGNED_INT` | 4 byte không dấu |
| `GL_FLOAT` | 4 byte float → **cắt** (truncate) về int |
| `GL_2_BYTES` | 2 byte không dấu, ghép big-endian: `offset = (b0 << 8) + b1` |
| `GL_3_BYTES` | 3 byte không dấu, ghép big-endian |
| `GL_4_BYTES` | 4 byte không dấu, ghép big-endian |

Thuật toán cho `b ∈ {2,3,4}` (nguyên văn spec):

```
offset ← 0
for i = 1 to b:
    offset ← offset shifted left 8 bits
    offset ← offset + byte
    advance to next byte in the array
```

Ghi chú: `GL_FLOAT` ⇒ "floating point values are truncated" — **không** làm
round-to-nearest.

### 1.4 Lồng nhau

* `glCallList` / `glCallLists` **có thể** nằm trong một display list.
* Nếu `glNewList` dùng `GL_COMPILE_AND_EXECUTE` ⇒ các list lồng nhau **được thực
  thi**, nhưng **chính lệnh `glCallList`/`glCallLists`** (không phải các lệnh
  thành phần của list được gọi) mới được ghi vào list đang xây dựng.
* Có **giới hạn độ sâu lồng nhau** (implementation-dependent) để tránh đệ quy
  vô hạn. **Giới hạn này phải ≥ 64.**

### 1.5 Lệnh **KHÔNG** được ghi vào list (thực thi tức thì)

SPEC §5.4 liệt kê tường minh các lệnh "executed immediately" khi đang biên dịch
display list. Đây là danh sách **chuẩn (normative)** — bảng dưới đây được
trích nguyên văn từ `glspec21.pdf`:

| Nhóm | Lệnh |
|---|---|
| Display lists | `GenLists`, `DeleteLists` |
| Render modes | `FeedbackBuffer`, `SelectBuffer`, `RenderMode` |
| Vertex arrays | `ClientActiveTexture`, `ColorPointer`, `EdgeFlagPointer`, `FogCoordPointer`, `IndexPointer`, `InterleavedArrays`, `NormalPointer`, `SecondaryColorPointer`, `TexCoordPointer`, `VertexAttribPointer`, `VertexPointer` |
| Client state | `EnableClientState`, `DisableClientState`, `EnableVertexAttribArray`, `DisableVertexAttribArray`, `PushClientAttrib`, `PopClientAttrib` |
| Pixels và textures | `PixelStore`, `ReadPixels`, `GenTextures`, `DeleteTextures`, `AreTexturesResident` |
| Occlusion queries | `GenQueries`, `DeleteQueries` |
| Vertex buffer objects | `GenBuffers`, `DeleteBuffers`, `BindBuffer`, `BufferData`, `BufferSubData`, `MapBuffer`, `UnmapBuffer` |
| Program và shader objects | `CreateProgram`, `CreateShader`, `DeleteProgram`, `DeleteShader`, `AttachShader`, `DetachShader`, `BindAttribLocation`, `CompileShader`, `ShaderSource`, `LinkProgram`, `ValidateProgram` |
| GL command stream management | `Finish`, `Flush` |
| Truy vấn khác | **mọi** lệnh query có tên bắt đầu bằng `Get` hoặc `Is` |

Ngoài ra (SPEC §5.5, đoạn cuối): `TexImage3D`, `TexImage2D`, `TexImage1D`,
`Histogram`, `ColorTable` được thực thi tức thì **khi được gọi với tham số
proxy** (`GL_PROXY_TEXTURE_3D`, `GL_PROXY_TEXTURE_2D`, `GL_PROXY_TEXTURE_CUBE_MAP`,
`GL_PROXY_TEXTURE_1D`, `GL_PROXY_HISTOGRAM`, `GL_PROXY_COLOR_TABLE`,
`GL_PROXY_POST_CONVOLUTION_COLOR_TABLE`, `GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE`).

**Quy tắc quan trọng đi kèm (SPEC §5.4):**

> "GL commands that source data from buffer objects **dereference the buffer
> object data in question at display list compile time**, rather than encoding
> the buffer ID and buffer offset into the display list. Only GL commands that
> are executed immediately, rather than being compiled into a display list, are
> permitted to use a buffer object as a data sink."

Tức là: `glDrawElements` với buffer ⇒ khi **ghi** list phải **chụp nội dung
index**, không phải id buffer + offset. Cùng nguyên tắc cho con trỏ client
(R1/R2).

### 1.6 State cần cho display list (SPEC §5.4, đoạn cuối)

Spec liệt kê chính xác:

| State | Kiểu | Ghi chú |
|---|---|---|
| "display list creation" bit | 1 bit | ban đầu = **thực thi tức thì** |
| chỉ số list hiện đang định nghĩa | 1 số nguyên | |
| bit "có thực thi khi biên dịch không" | 1 bit | tương ứng `GL_COMPILE` vs `GL_COMPILE_AND_EXECUTE` |
| `ListBase` | 1 số nguyên | ban đầu = 0 |
| tập các chỉ số đang được dùng | tập | ban đầu = rỗng |

### 1.7 Tương tác với display list và pipeline

SPEC §5.4, đoạn cuối:

> "When a program object is in use, a display list may be executed whose vertex
> attribute calls do not match up exactly with what is expected by the vertex
> shader contained in that program object. Handling of this mismatch is described
> in section 2.15.3."

⇒ Replay phải **late-bound**: lệnh "vẽ dải đỉnh này với mode này" được quyết
định **lúc replay** bởi program object đang bind, chứ không phải lúc compile.

---

## 2. CURRENT — hiện trạng trong TGLMT

### 2.1 Không có bất kỳ cơ chế display list nào

* `ObjectRegistry::ObjectKind` (`include/tglmt/ObjectRegistry.h:13-16`) liệt kê:
  `Buffer, Texture, Sampler, VertexArray, Framebuffer, Renderbuffer, Program,
  Shader, ProgramPipeline, Query, Sync, TransformFeedback` — **không có**
  `DisplayList`.
* `ObjectRegistry` chỉ là **bộ cấp ID** (`Gen`/`Create`/`Delete`/`Is`/`NextId`,
  `include/tglmt/ObjectRegistry.h:20-25`), không phải kho lệnh.
* 8 entry point đều là stub trung thực (§0).

### 2.2 Rủi ro kỹ thuật đã biết khi implement

| # | Rủi ro | Bằng chứng | Xử lý PLAN |
|---|---|---|---|
| D-1 | Bề mặt GL là **1049 prototype `extern "C"`** (`include/tglmt/gl_c_api.h`); bắt lệnh "ghi thay vì thực thi" ở **từng** entry point là không bảo trì được | `docs/LEGACY_TO_METAL_MAPPING.md` §9.2 | Chuyển switch nằm ở **tầng dispatcher** (xem §3.2) |
| D-2 | `src/gl/gl_c_exports.cpp` là **file sinh tự động** (`tools/gen_c_exports.py`) — sửa tay sẽ bị ghi đè | header file dòng 1 | Sửa **generator**, không sửa file sinh |
| D-3 | `tools/gen_stubs.py` coi một hàm là "đã implement" nếu thấy **định nghĩa** `RET glXxx(...) {` trong `src/gl/*.cpp` (trừ `gl_stubs_gen.cpp` và `gl_c_exports.cpp`) | `tools/gen_stubs.py:20-33` | Hàm display list phải được định nghĩa **tay** trong file mới, không dùng macro sinh |
| D-4 | Version-gate đang chạy ở lớp C ABI; dispatcher mới phải **không** phá gate | `docs/DECISIONS.md` D-003 | Gate check **trước**, dispatch check **sau** |

### 2.3 Điểm cần lưu ý: `glCallList` bị gate

`glCallList` là COMPAT-ONLY (`include/tglmt/gl_legacy.h:17-18`; mask
`0x0000003FFFF803FF` tại `src/gl/gl_version_gate_gen.cpp`). Ở GL 4.6 **Core**
⇒ `GL_INVALID_OPERATION` và lời gọi **không** được thực thi (đo ✓).

---

## 3. PLAN — thiết kế command-stream

### 3.1 Vì sao là command stream (không phải draw cache)

Xem §4.

### 3.2 Kiến trúc 3 tầng

```
┌──────────────────────────────────────────────────────────────┐
│ Tầng 1 — C ABI (src/gl/gl_c_exports.cpp, file SINH)          │
│   extern "C" glXxx(...) { GateCheck(idx); gl::glXxx(...); }  │
│   → thêm: nếu đang capture ⇒ Record(cmdId, args) thay vì gọi│
└──────────────────────────────────────────────────────────────┘
                              │
┌──────────────────────────────────────────────────────────────┐
│ Tầng 2 — Dispatcher (src/gl/gl_display_list.cpp — MỚI)        │
│   DisplayListRecorder::ShouldRecord(cmdId) → bool            │
│   DisplayListRecorder::Record(cmdId, args…)                  │
│   DisplayListRecorder::ExecuteNow(cmdId, args…)              │
└──────────────────────────────────────────────────────────────┘
                              │
┌──────────────────────────────────────────────────────────────┐
│ Tầng 3 — State (Context::legacy.displayLists — MỚI)           │
│   map<GLuint, DisplayList>  + allocator liên tục + listBase  │
└──────────────────────────────────────────────────────────────┘
```

**Vì sao chuyển switch xuống tầng dispatcher (thay vì nhét vào mỗi entry
point):**

1. Bề mặt GL có 1049 entry point (`include/tglmt/gl_c_api.h`); một `if` trong
   mỗi hàm là 1049 chỗ để quên.
2. `src/gl/gl_c_exports.cpp` là **file sinh** ⇒ có thể sửa **generator**
   (`tools/gen_c_exports.py`) để chèn dispatcher call **một cách máy hóa**.
3. Tập lệnh "thực thi tức thì" (§1.5) là **danh sách tên lệnh** ⇒ có thể sinh
   ra từ `docs/khronos/gl.xml` + bảng whitelist ⇒ không hardcode tay.

**Vấn đề cần giải quyết trước khi code (PLAN, quyết định kiến trúc):**

`gl_c_exports.cpp` là wrapper **phẳng**: `gl::glXxx(args…)` — mỗi entry point
gọi đúng một hàm C++ với đúng bộ tham số. Để dispatcher chặn được, wrapper phải
biết **argc/argv** theo cách **type-safe**. Ba phương án:

| PA | Cách | Đánh giá |
|---|---|---|
| **PA-1** | Sinh thêm một lớp `gl::` có biến thể `Record` cho từng lệnh | Rất lớn (1049 hàm × 2) |
| **PA-2** *(chọn)* | Dispatcher nằm **trong** `tglmt::gl::`, tức mỗi hàm `tglmt::gl::glXxx` tự hỏi "có đang capture không" qua một **helper chung** `DL::Capture(cmdId)` | Vẫn cần chạm vào nhiều hàm, nhưng chỉ **những hàm có thể nằm trong display list** |
| **PA-3** | Dùng **X-macro / bảng mô tả lệnh** (tên, số tham số, kiểu) sinh từ `gl.xml`; dispatcher dùng bảng để **marshal** tham số vào một `std::vector<uint8_t>` | Sạch nhất, tự sinh, nhưng phải viết marshaller cho ~15 kiểu tham số |

**Khuyến nghị (PLAN):** **PA-3 cho Stage 2**, **PA-2 cho Stage 1**
(xem lộ trình §5). Bảng mô tả lệnh sinh từ `docs/khronos/gl.xml` đã có sẵn
(`tools/gl46_catalog.py::parse_command_prototypes`).

### 3.3 Cấu trúc lệnh (command record)

```cpp
// include/tglmt/DisplayList.h  (MỚI)
namespace tglmt {

// Một lệnh đã ghi vào display list.
struct DLCommand {
    GLenum     op;        // opcode nội bộ (xem OpCode)
    GLuint     argsOffset;// offset vào payload bytes của list
    GLuint     argsSize;  // số byte tham số đã marshal
    GLuint     aux;       // dữ liệu phụ (vd số phần tử của mảng tham số)
};

// Một display list.
struct DisplayList {
    std::vector<DLCommand> cmds;        // thứ tự lệnh
    std::vector<uint8_t>     payload;    // tham số đã marshal (append-only)
    bool        executable = false;      // false ⇒ list rỗng (do glGenLists tạo)
    uint32_t    nestDepth = 0;           // dùng khi replay để chặn đệ quy
};

// Toàn bộ state display list của một context.
struct DisplayLists {
    std::map<GLuint, DisplayList> lists;  // index → list
    std::set<GLuint>                used;  // chỉ số đã dùng (glGenLists đánh dấu)
    GLuint  listBase       = 0;          // glListBase
    GLuint  nextFree       = 1;          // allocator: chỉ số chưa dùng kế tiếp
    bool    capturing      = false;      // đang trong glNewList..glEndList
    bool    compileAndExec = false;      // GL_COMPILE_AND_EXECUTE
    GLuint  captureIndex   = 0;          // list đang định nghĩa
    GLuint  captureDepth   = 0;          // độ sâu lồng nhau khi REPLAY
    GLuint  replayDepth    = 0;          // (≥64 ⇒ GL_STACK_OVERFLOW, xem §1.4)
    std::vector<DLCommand> pending;      // buffer ghi tạm (transactional, xem §3.6)
    std::vector<uint8_t>     pendingPayload;
};
```

`DisplayLists` đặt **trong `Context::legacy`** (không phải `StateTracker` — D-004)
và `ObjectRegistry` **không** cần thêm kind mới: `glGenLists`/`glIsList`/
`glDeleteLists` có bộ quản lý riêng (allocator liên tục + tập chỉ số đã dùng),
khác hẳn `ObjectRegistry` (cấp ID đơn lẻ, không cần "contiguous block").

### 3.4 Ghi (record) — quy tắc

| Tình huống | Hành vi (SPEC §5.4) |
|---|---|
| `glNewList(n, GL_COMPILE)` | `capturing = true`, `compileAndExec = false`, `captureIndex = n`. **Bắt đầu buffer `pending` rỗng.** List `n` cũ (nếu có) **chưa** bị đụng tới. |
| `glNewList(n, GL_COMPILE_AND_EXECUTE)` | như trên nhưng `compileAndExec = true`: mỗi lệnh **vừa** thực thi **vừa** ghi. |
| `glNewList` khi đã `capturing` | `GL_INVALID_OPERATION` (SPEC §5.4). List đang mở **không** bị đóng. |
| `glNewList(0, …)` | `GL_INVALID_VALUE`. |
| Lệnh GL khi `capturing` | Nếu lệnh nằm trong danh sách "thực thi tức thì" (§1.5) ⇒ **thực thi ngay**, **không** ghi. Ngược lại ⇒ **ghi** vào `pending`; nếu `compileAndExec` thì **thực thi luôn**. |
| `glEndList()` | Nếu `!capturing` ⇒ `GL_INVALID_OPERATION`. Nếu `captureIndex == 0` ⇒ `GL_INVALID_OPERATION` (spec: `glNewList(0)` đã lỗi, không có list hợp lệ để đóng). Nếu thành công ⇒ **commit** `pending` → `lists[captureIndex]`, đánh dấu `used.insert(captureIndex)`, `capturing = false`. |
| `glEndList()` không đủ bộ nhớ | `GL_OUT_OF_MEMORY` và **không** đổi nội dung cũ của list, **không** đổi state khác (trừ state do `COMPILE_AND_EXECUTE` đã đổi). ⇒ **commit phải transactional**: dựng vào `pending`, chỉ chuyển vào `lists[]` khi thành công. |

**Điểm dễ sai (phải test):**

* `glEndList` khi `glNewList` **chưa** gọi ⇒ `GL_INVALID_OPERATION` (đo được sau khi
  implement).
* `glNewList(0, …)` ⇒ `GL_INVALID_VALUE`, và **không** được mở capture.
* `glNewList` lần 2 trước `glEndList` ⇒ `GL_INVALID_OPERATION`.

### 3.5 Phát lại (replay)

```
glCallList(n):
    if (n == 0) → GL_INVALID_VALUE; return
    Replay(n, depth = 0)

Replay(n, depth):
    if (depth > kMaxNestDepth /* ≥64 */) → GL_STACK_OVERFLOW; return
    it = lists.find(n); if (it == end) → return;      // chỉ số không tồn tại: không tác dụng
    for (cmd : it->second.cmds):
        if (cmd.op == OP_CALL_LIST):
            Replay(target, depth+1)                   // ← late-bound, xem §3.7
        else:
            Execute(cmd)                               // gọi lại hàm GL tương ứng
```

* `OP_CALL_LIST` / `OP_CALL_LISTS` là **opcode nội bộ**: khi record lệnh
  `glCallList(x)`, ta ghi **lệnh call** chứ không ghi nội dung của list `x`
  (SPEC §5.4: "the CallList or CallLists, rather than those lists' constituent
  commands, is placed in the list under construction").
* `GL_COMPILE_AND_EXECUTE` + lệnh call: list được **thực thi** (đi qua
  `Replay`) **và** lệnh call **được ghi**.

### 3.6 Quy tắc "resource dependency" — điểm dễ sai nhất

Đây là phần quyết định list có đúng semantics hay không.

| Loại dependency | Quy tắc SPEC | Hiện thực PLAN |
|---|---|---|
| **Client state** (current color, current texcoord, current normal, current secondary color, current index, edge flag) | Chụp lúc **ghi** (R2) | Vì `glVertex*` **đã** resolve current state thành dữ liệu đỉnh cục bộ (`src/gl/gl_legacy_vertex.cpp:165-180`), ta ghi **đỉnh đã resolve** ⇒ R2 thoả mãn *tự nhiên*. Tương tự `glColor*` đứng một mình trong list: `glColor` **là** client state nhưng cũng là **server state** cho màu đỉnh — theo R2 ta **chụp giá trị tại thời điểm ghi**. |
| **Server state** (matrix, lighting, material, fog, texenv, texgen, viewport, blend, depth, bind texture) | Đọc lúc **thực thi** (R3) | **Không** ghi giá trị vào list. Chỉ ghi *lệnh*. Vì vậy `glTranslate` trong list phải được thực thi lại mỗi lần `glCallList` với matrix hiện hành. |
| **Con trỏ client** (`glVertexPointer`, `glColorPointer`, …) | Dereference lúc **ghi** | Chép **nội dung** mảng vào `payload` của list. |
| **Buffer object** (`glDrawElements` với buffer) | Dereference lúc **ghi** (SPEC §5.4, đoạn cuối) | Chép **bytes index** vào `payload`; **không** ghi buffer id + offset. |
| **Texture đã bind** | Đây là *server* state ⇒ bind lúc replay | `glBindTexture` trong list phải được thực thi lại ⇒ texture đổi giữa lúc compile và lúc replay sẽ cho kết quả khác — **đúng spec**. |
| **Đối tượng bị xoá** (`glDeleteTextures`, `glDeleteBuffers` giữa compile và replay) | — | Vì texture/buffer là server state và lệnh delete **thực thi tức thì** (§1.5), list vẫn hợp lệ; nếu lệnh delete nằm **trong** list thì nó bị ghi? **Không** — `DeleteTextures`/`DeleteBuffers` thuộc nhóm "thực thi tức thì" ⇒ không được ghi. Hệ quả: xoá texture **bên ngoài** list sẽ làm list vẽ sai ⇒ đúng theo spec. |

**Quy tắc thiết kế rút ra (PLAN):**

> **Ràng buộc:** mọi thứ mà spec coi là *client state* phải được **chụp** (copy
> giá trị) vào list. Mọi thứ spec coi là *server state* phải được **ghi lệnh**,
> không ghi giá trị.

Phân loại chuẩn của "client state" theo GL 2.1 §6.1.3 (bảng 6.1):
*vertex array pointers*, *client pixel store*, *client attribute enable*, và
*current vertex attributes* (current color / normal / texcoord / secondary
color / index / raster position). Mọi thứ khác là server state.

### 3.7 Late-bound: list chứa draw lệnh của đường modern

Một display list có thể chứa `glDrawArrays`/`glDrawElements` (đường modern) và
cũng có thể chứa `glBegin/glEnd` (đường legacy). Khi replay, **program object
đang bind quyết định** đường nào thực thi (SPEC §5.4 cuối §2.15.3).

⇒ Lệnh draw phải được ghi **không kèm** thông tin "program nào". Chỉ ghi
`(mode, first, count)` / `(mode, count, type, offsetBytes, indexBytes)`.

### 3.8 `glCallLists` — bảng offset

Hàm `glCallLists(n, type, lists)`:

1. Nếu `n == 0` ⇒ không làm gì.
2. Với `type ∈ {GL_2_BYTES, GL_3_BYTES, GL_4_BYTES}`: đọc `n * b` byte từ
   `lists`, ghép **big-endian** theo thuật toán §1.3.
3. Với `type` kiểu số: đọc `n` phần tử theo kiểu, chuyển về int
   (`GL_FLOAT` ⇒ **truncate**).
4. Với mỗi offset `o`: chỉ số list = `o + listBase`.
5. Thực thi lần lượt theo thứ tự.

**Lỗi (SPEC §5.4):** không nêu rõ lỗi cho `type` sai; nhưng theo GL 4.6 §22.1
⇒ `GL_INVALID_ENUM` cho `type` không hợp lệ, `GL_INVALID_VALUE` cho `n < 0`.

### 3.9 `glGenLists` — allocator liên tục

Spec: trả `n` sao cho `n … n+s−1` là `s` chỉ số **chưa dùng liên tiếp**, **và**
tạo một list rỗng cho mỗi chỉ số đó.

Thuật toán (PLAN):

```
if (s <= 0) return 0;
run = 0; start = nextFree;
for (i = nextFree; ; ++i) {
    if (!used.count(i)) { run++; if (run == s) { start = i - s + 1; break; } }
    else run = 0;
    if (i - nextFree > kMaxScan) return 0;    // không đủ block liên tiếp
}
nextFree = start + s;
for (i = start; i < start + s; ++i) { used.insert(i); lists[i] = DisplayList{}; }
return start;
```

`glDeleteLists(list, range)`: xoá `range` list từ `list`; bỏ qua chỉ số không
tồn tại; `range == 0` ⇒ không làm gì; giải phóng chỉ số về "chưa dùng".

`glIsList(list)` ⇒ `lists.count(list) && lists[list].executable`.
Lưu ý: list do `glGenLists` tạo là **rỗng nhưng vẫn là display list** ⇒
`glIsList` phải trả `GL_TRUE` (spec: "GenLists also has the effect of creating
an empty display list for each of the indices").

---

## 4. Vì sao KHÔNG được biến thành "cache draw call đơn giản"

| # | Semantics sẽ sai nếu cache draw | SPEC | Hậu quả thực tế |
|---|---|---|---|
| 1 | **Server state đọc lúc replay** | §5.4 ("Only server state is affected when the command is executed") | List chứa `glTranslate(1,0,0)` + `glBegin/glEnd` sẽ bake ma trận lúc compile. App dịch chuyển model sau khi compile ⇒ **hình sai mà không có lỗi nào**. Đây là lỗi "silent wrongness" mà app legacy không bao giờ phát hiện. |
| 2 | **Client state phải chụp lúc compile** | §5.4 | Cache draw sẽ dùng current color **tại lúc replay** thay vì lúc compile. |
| 3 | **Lệnh thay đổi state bên trong list** | §5.4 | `glBegin/…/glEnd` + `glColor` xen kẽ: cache draw sẽ gộp thành 1 draw, mất thứ tự lệnh. |
| 4 | **`GL_COMPILE` vs `GL_COMPILE_AND_EXECUTE`** | §5.4 | Cache draw không phân biệt được: side effect của `COMPILE_AND_EXECUTE` (state bị đổi) là quan sát được. |
| 5 | **Lệnh "thực thi tức thì"** | §5.4 | `glGenLists`, `glReadPixels`, `glBindBuffer`, `Finish`… phải **không** được ghi. Cache draw ghi tất cả ⇒ sai. |
| 6 | **Lồng nhau** | §5.4 | `glCallList` trong list phải thực thi list con **lúc replay**, không nội tuyến. Cache draw inline nội dung ⇒ sai về thứ tự và về giới hạn độ sâu ≥64. |
| 7 | **Con trỏ & buffer dereference lúc compile** | §5.4 | `glDrawElements` với con trỏ client: cache draw giữ con trỏ ⇒ **dangling pointer**. Với buffer: phải chụp **bytes**, không phải id+offset. |
| 8 | **Late-bound program** | §5.4 (cuối) | List chứa draw phải chạy qua **program đang bind lúc replay**. Cache draw gắn program lúc compile ⇒ sai khi app đổi program. |
| 9 | **`glListBase` + `glCallLists`** | §5.4 | Offsets cộng với `listBase`; cache draw không có khái niệm "nhiều list". |
| 10 | **`GL_OUT_OF_MEMORY` phải transactional** | §5.4 | Commit phải là một bước; cache draw dễ ghi nửa chừng. |

> **Kết luận:** command stream là lựa chọn **duy nhất** bảo toàn semantics.
> Xác nhận cùng kết luận: `docs/LEGACY_TO_METAL_MAPPING.md` §9.1,
> `docs/LEGACY_PIPELINE_AUDIT.md` §8 (PLAN), `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md`
> §6.2-6.3.

---

## 5. Lộ trình implement theo stage

| Stage | Nội dung | File | Entry point | Test ID | Exit criteria |
|---|---|---|---|---|---|
| **D0** | State + allocator + query cơ bản (chưa record lệnh nào) | `include/tglmt/DisplayList.h` (mới), `src/gl/gl_display_list.cpp` (mới) | `glGenLists`, `glIsList`, `glDeleteLists`, `glListBase` | `test_legacy_displaylist` (mới) | `glGenLists(4)` trả block **liên tiếp**; `glIsList` đúng cho index trong/ngoài block; `glDeleteLists(x,0)` không làm gì; index không tồn tại bị bỏ qua; `glListBase` round-trip |
| **D1** | Vòng đời list + buffer `pending` transactional | `src/gl/gl_display_list.cpp` | `glNewList`, `glEndList` | `test_legacy_displaylist` | `glNewList(0,…)` ⇒ `INVALID_VALUE`; `glNewList` 2× ⇒ `INVALID_OPERATION`; `glEndList` không `glNewList` ⇒ `INVALID_OPERATION`; `glEndList` commit 1 lần; `COMPILE` **không** thực thi, `COMPILE_AND_EXECUTE` **có** |
| **D2** | Ghi lệnh: dispatcher + bảng opcode | `tools/gen_c_exports.py` (sửa generator) + `src/gl/gl_c_exports.cpp` (sinh lại) + `src/gl/gl_display_list.cpp` | tất cả lệnh ghi được | `test_legacy_displaylist` | `glTranslate` trong list được **ghi**, không chạy (mode COMPILE); lệnh "thực thi tức thì" (§1.5) **không** được ghi |
| **D3** | Replay: `glCallList` + lồng nhau + giới hạn ≥64 | `src/gl/gl_display_list.cpp` | `glCallList` | `test_legacy_displaylist` | List chứa `glTranslate` ⇒ dịch đúng theo matrix **lúc replay**; list lồng nhau thực thi đúng thứ tự; độ sâu > 64 ⇒ `GL_STACK_OVERFLOW` |
| **D4** | `glCallLists` + bảng offset 10 kiểu | `src/gl/gl_display_list.cpp` | `glCallLists` | `test_legacy_displaylist` | 10 kiểu `type` cho offset đúng (kể cả `GL_2/3/4_BYTES` big-endian và `GL_FLOAT` truncate) |
| **D5** | Client/buffer dependency: chụp con trỏ & bytes index | `src/gl/gl_display_list.cpp` | các lệnh có con trỏ | `test_legacy_displaylist_buffer` (mới) | `glDrawElements` với buffer: sửa buffer **sau khi compile** ⇒ replay vẫn dùng **bytes cũ** (SPEC §5.4) |
| **D6** | Tương tác với đường legacy + modern (late-bound) | `src/gl/gl_apple_draw.cpp`, `src/gl/gl_display_list.cpp` | — | `test_legacy_displaylist_apple` (mới, GPU) | List chứa `glBegin/glEnd` replay qua đường legacy; list chứa `glDrawArrays` replay theo **program đang bind lúc replay** |

**Ràng buộc chung cho mọi stage:**

1. State phải nằm trong `Context::legacy` (D-004), **không** trong `StateTracker`.
2. `src/gl/gl_c_exports.cpp` và `src/gl/gl_stubs_gen.cpp` là **file sinh** ⇒ mọi
   thay đổi phải qua `tools/gen_c_exports.py` / `tools/gen_stubs.py`
   (`AGENTS.md` §5). Thứ tự regenerate: `gen_gl_headers.py` →
   `gen_version_inventory.py` → `gen_version_gate.py` → `gen_c_exports.py` →
   `gen_stubs.py`.
3. `glNewList`/`glEndList`/`glCallList`/`glCallLists` là COMPAT-ONLY ⇒ đã có
   version-gate; **không** được phá gate (D-003).
4. Mọi test phải dùng `TGLMT_CHECK` (không `assert()`), và phải kiểm tra **giá
   trị** chứ không chỉ "không crash" (`AGENTS.md` §8).
5. Test phải tạo context **Compatibility** (`TGLMT_PROFILE_COMPATIBILITY`) —
   ở Core, mọi lệnh display list trả `GL_INVALID_OPERATION`.

---

## 6. Điểm chưa verify

| # | Điểm | Vì sao chưa verify |
|---|---|---|
| V-1 | Chi phí marshalling tham số (PA-3) | Chưa có bảng mô tả lệnh sinh tự động; chưa đo |
| V-2 | Ảnh hưởng của `glPushAttrib`/`glPopAttrib` lên display list | `glPushAttrib`/`glPopAttrib` cũng là stub; tương tác chưa rõ |
| V-3 | `GL_OUT_OF_MEMORY` path | Chưa có cơ chế cấp phát thất bại để test |
| V-4 | Tương tác với `glRenderMode(GL_SELECT)` / `GL_FEEDBACK` | `glRenderMode` là stub (nhóm L09/L10) |
| V-5 | Hành vi với `ARB_direct_state_access` khi display list đang mở | Chưa quyết định |

---

## 7. Tham chiếu chéo

* `docs/FIXED_FUNCTION_PIPELINE.md` — bảng state fixed-function, quy tắc `glEnable`, lỗi đã đo.
* `docs/IMMEDIATE_MODE.md` — lệnh được phép trong `glBegin/glEnd` (liên quan trực tiếp §1.5: `Material`, `CallList`, `EdgeFlag` **được** phép trong Begin/End).
* `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` §6.2-6.3 — vì sao cần command stream, IR có cần thiết không.
* `docs/LEGACY_TO_METAL_MAPPING.md` §9 — chiến lược Metal cho display list.
* `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` §1 (S10), §0.3 — quy ước stage & test.
* `docs/DECISIONS.md` D-003, D-004.
