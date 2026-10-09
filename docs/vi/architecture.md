# Kiến trúc

Trang này mô tả đường chạy runtime của một lệnh OpenGL, từ entry point tới việc tạo lệnh
Metal, cùng mô hình thực thi trễ (deferred execution) gộp các lệnh đó lại với nhau.

## Các lớp

```
  Ứng dụng  (LWJGL / GLFW / Blaze3D / gọi gl:: trực tiếp)
      |
      |  698 entry point gl*                include/tglmt/gl46.h
      |  10 hàm cửa sổ TGLMT_*              include/tglmt/GLFWShim.h
      v
  +---------------------------------------------------------------------+
  | Dispatch        src/gl/*.cpp                                         |
  |   kiểm tra tham số, ghi lỗi GL, cập nhật shadow-state                |
  |   gl_shader.cpp      glCompileShader  -> GLSL->MSL                   |
  |   gl_program.cpp     glLinkProgram    -> kiểm tra link + biên dịch MSL|
  |   gl_apple_draw.cpp  glDraw*          -> encode Metal trễ            |
  |   gl_texture.cpp / gl_buffer.cpp / gl_framebuffer.cpp / gl_raster.cpp|
  +---------------------------------------------------------------------+
      |
      |  include/tglmt/
      |    StateTracker.h    shadow CPU của trạng thái GL, cờ dirty
      |    ObjectRegistry.h  bảng id -> đối tượng (bảo vệ bằng mutex)
      |    ErrorTracker.h    hàng đợi lỗi GL dạng FIFO
      |    Context.h         mọi thứ theo Context (buffer, texture,
      |                      program, FBO, staging map, AppleStats)
      v
  +---------------------------------------------------------------------+
  | GLSL -> MSL     src/gl/glsl_to_msl.cpp                               |
  |   chỉ vertex + fragment; xuất TGLMT_vs / TGLMT_fs                   |
  +---------------------------------------------------------------------+
      |
      v
  Đường draw trễ            src/gl/gl_apple_draw.cpp
      |    shadow trạng thái và bỏ qua trạng thái thừa
      |    tái dùng encoder giữa các draw liên tiếp
      |    cache pipeline-key, cache uniform
      |    staging buffer/texture với gộp range
      |    upload ring-buffer, flush có điều kiện
      v
  Trừu tượng Metal          include/tglmt/MetalInterface.h
      |
      +--> src/metal/AppleMetalBridge.mm   backend Apple, GPU thật
      +--> src/metal/NullMetalBackend.cpp  backend Null, trace + shadow
      v
  presentTarget(CAMetalLayer)     hoặc     đọc lại từ buffer shadow
```

Hai backend cùng triển khai interface `metal::IDevice`. Backend được chọn khi dựng
Context từ chuỗi `"apple"` hoặc `"null"`; yêu cầu `"apple"` nhưng không tạo được Metal
device thì rơi về Null device và tự báo là
`apple(null-fallback,no-MTL-device)`.

## Context và trạng thái

`Context` sở hữu mọi bảng đối tượng (buffer, texture, sampler, program, FBO, VAO, đối
tượng transform feedback), cả hai staging map, ring allocator, các cache pipeline và
`AppleStats`.

`StateTracker` giữ shadow CPU của trạng thái GL. Các nhóm của nó:

| Nhóm | Nội dung |
|---|---|
| Khả năng | `glEnable` / `glEnablei` theo target |
| Blend và mask màu | trạng thái blend theo attachment và mask màu 4 bit |
| Depth và stencil | depth func/enable/write, stencil trước và sau |
| Raster | viewport, cull mode, front face, polygon mode, scissor, polygon offset, line width, clip control |
| Bindings | điểm gắn buffer, slot texture theo unit và gắn sampler, đơn vị texture active, VAO, program, XFB, FBO đọc/ghi, số đỉnh patch |
| Pixel store | row length, skip, alignment của pack/unpack |
| Shadow cho Get | giá trị `(pname, index)` mà `glGet*` trả về |

Mọi setter đều so sánh trước khi gán. `SetEnabled`, `SetBlendColor`, `SetViewport`,
`SetScissor`, `SetLineWidth`, `SetDepthRangef`, `SetPatchVertices` và `BindXFB` đều trả
về ngay khi giá trị không đổi, tức là `stateSeq_` không tăng. Do đó `stateSeq_` là phép
đo trực tiếp của "N lệnh GL giống hệt nhau tạo ra 0 thay đổi trạng thái".

Mọi lệnh `glGet*` đều đọc shadow; không lệnh nào round-trip sang GPU
(`src/gl/gl_get.cpp`).

## Registry đối tượng

`ObjectRegistry` là bảng `id -> đối tượng` bảo vệ bằng mutex, với id kế tiếp tăng đơn
điệu, dùng cho buffer, texture, sampler, program, shader, framebuffer, VAO và đối tượng
transform feedback. Xóa đối tượng chỉ gỡ phần tử bảng; các map theo Context
(`c.buffers`, `c.textures`, …) mới giữ dữ liệu thật.

## Hệ thống shader và program

`glCompileShader` chuyển shader vertex và fragment vào `ConvertGLSLtoMSL`. Bất kỳ stage
nào khác — tessellation control, tessellation evaluation, geometry, compute — đều bị từ
chối ngay lúc biên dịch với `COMPILE_STATUS = false` và log rõ ràng
(`src/gl/gl_shader.cpp`). Ở lớp này không có chỗ nào âm thầm xấp xỉ.

Tiếp đó `glLinkProgram`:

1. kiểm tra sự khớp tên/location/kiểu của varying giữa output vertex và input fragment;
2. kiểm tra mọi input fragment đều được stage vertex ghi;
3. tính `trueSize` (offset kết thúc lớn nhất của thành viên) và `minSize` (giá trị đó
   làm tròn lên 16) cho mỗi uniform block;
4. gán binding uniform block từ `layout(binding=N)` hoặc từ
   `glUniformBlockBinding` trước đó;
5. trên thiết bị thật, biên dịch MSL đã sinh ra thành các `MTLLibrary` và lưu vào program.
   Trên Null device, bước này bị bỏ hoàn toàn.

Program có stage tessellation/geometry, hoặc program chỉ có vertex, vẫn link thành công
vì đặc tả OpenGL yêu cầu như vậy, nhưng đường draw từ chối encode và tính lần thử đó vào
trace.

Pipeline state object được tạo lười ở lần dùng đầu và được
`Context::PrewarmPipelineForProgram`预熱 trước frame đầu tiên.

## Vòng đời một lệnh draw

`glDrawArrays` / `glDrawElements` và các biến thể hội tụ về `AppleDrawGL`
(`src/gl/gl_apple_draw.cpp`). Theo thứ tự:

1. **Từ chối sớm** — không có target hiện hành, `baseInstance != 0`, program chưa link,
   program có stage tessellation/geometry, `GL_PATCHES`, `baseVertex != 0` trên draw không
   có index. Mỗi lần từ chối tăng một counter riêng thay vì bỏ việc đi một cách im lặng.
2. **Giải quyết tài nguyên** — vertex array, element array, program đang gắn, attachment
   màu và depth của draw FBO hiện hành.
3. **Quét nguy cơ trước** — nếu program lấy mẫu chính texture mà nó đang vẽ vào, encoder
   đang mở bị đóng trước và `hazardSplits` tăng. Đây là yêu cầu của TBDR: Metal không tự
   theo dõi nguy cơ đọc-ghi trên cùng tài nguyên trong một render pass.
4. **Chọn hoặc tạo render encoder** — tái dùng khi encoder đang mở còn đó và draw FBO,
   color attachment, sự có mặt của depth, cùng pending clear đều khớp; nếu không thì flush
   encoder cũ và tạo encoder mới.
5. **Clear** — clear ngay khi được; nếu không thì clear được ghi vào
   `applePendingClear` và draw kế tiếp tiêu thụ nó dưới dạng `loadAction = Clear`.
6. **Giải quyết pipeline** — một `Context::PipelineKey` (library vertex/fragment, format,
   stride đỉnh, depth, blend, mask ghi màu, và toàn bộ layout thuộc tính đỉnh) được so với
   cache. Trúng thì bỏ qua hẳn lệnh gọi bridge; không trúng thì gọi `makeCustomPipeline`.
7. **Upload uniform** — buffer byte uniform theo stage được so với `uniformCache`, chỉ
   stage nào thay đổi mới upload lại. Dữ liệu uniform lấy từ ring allocator.
8. **Encode trạng thái** — viewport, scissor, cull, fill mode, blend, depth, mask màu chỉ
   được ghi vào encoder khi shadow của pending-encoder báo giá trị đã đổi.
9. **Encode draw** — primitive FAN và LOOP được mở rộng thành tam giác trên CPU trước,
   vì Metal không có.
10. **Ghi nhận sử dụng** — buffer và texture mà draw tiêu thụ được nối vào danh sách
    pending-usage, vốn điều kiện cho các flush có điều kiện sau đó.

## Thực thi trễ

TGLMT không phát ra một lệnh Metal cho mỗi lệnh OpenGL. Có ba cơ chế độc lập trì hoãn
công việc.

### Thời gian sống của encoder

Một `MTLRenderCommandEncoder` duy nhất được mở xuyên suốt các draw liên tiếp thuộc cùng
một render pass. Nó chỉ bị đóng khi có thứ gì vô hiệu hóa pass:

| Kích hoạt | Nơi |
|---|---|
| Draw FBO, color attachment hoặc sự có mặt của depth thay đổi | `gl_apple_draw.cpp` |
| Phát hiện nguy cơ feedback từ phép quét trước | `gl_apple_draw.cpp` |
| Clear ngay | `gl_apple_draw.cpp` |
| Blit framebuffer, `glReadPixels`, `glGetTexImage`, `glCopyTexSubImage2D` | `gl_framebuffer.cpp`, `gl_readpixels_dsa.cpp`, `gl_get_extra.cpp`, `gl_texture.cpp` |
| Texture đang là render target bị khai báo lại | `gl_texture.cpp` |
| Stage một tài nguyên đã được dùng trong pass đang mở | `gl_buffer.cpp`, `gl_texture.cpp` |
| `glFlush` / `glFinish` | `gl_sync_query.cpp` |
| Biên frame — `SwapBuffers`, `EndFrame`, `Resize` | `glfw_shim.cpp`, `Renderer.cpp` |

`encodersCreated` so với `encoderReused` trong dòng tóm tắt frame là phép đo trực tiếp
của batching: frame khỏe mạnh có `encReuse` lớn hơn hẳn `encNew`.

### Staging tài nguyên

`glBufferSubData` và `glTexSubImage2D` không đụng tới GPU ngay. Chúng nối vào
`pendingBufRanges` / `pendingTexRegions`. Các range kề nhau được gộp (khoảng cách buffer
tối đa 64 byte; vùng texture thành hình hộp bao) trước khi một `memcpy` và một
`didModifyRange` / `replaceRegion` duy nhất.

Staging chỉ phá batching khi tài nguyên *đã* được pass đang mở dùng.
`MustFlushForBufferStage` và `MustFlushForTextureStage` kiểm tra danh sách pending-usage,
và lần flush bị từ chối tăng `flushAvoided`. Chính điều này cho phép hàng nghìn upload
sống chung với batching encoder.

Bảo vệ in-flight dùng pool buffer ba slot; slot mà GPU vẫn đang đọc thì không ghi. Nếu
không có slot nào trống, TGLMT commit và chờ (`bufWaitStall`) thay vì làm hỏng dữ liệu.
Texture tương đương: `texWasRTFlush` khi bản duy nhất nằm trên GPU.

### Biên frame

`SwapBuffers` và `EndFrame` flush encoder cùng mọi staging, rồi gọi `ThrottleGpu()`,
chỉ chặn khi số command buffer đang bay vượt `kMaxFramesInFlight` (2). Không có
`commitAndWait` theo từng frame: Metal thực thi command buffer trên một queue theo thứ tự
submit, nên frame N+1 không thể thấy ghi của frame N sớm hơn.

## Cache pipeline

Ba lớp tránh việc trùng lặp:

| Lớp | Key | Counter |
|---|---|---|
| IR pipeline key | library, format, stride, depth, blend, mask màu, layout thuộc tính | `pipelineLookupSkipped` khi trúng, `pipelineLookups` khi không trúng |
| Cache PSO của bridge | định danh descriptor bên trong `AppleMetalBridge.mm` | `pipelineLookups` |
| Tái dùng trong encoder | `nativeHandle()` giống hệt trong một pass | `pipelineReused` |

Depth-stencil state cũng được cache theo cách đó và tính vào `depthReused`.

## Upload ring-buffer

`Context` sở hữu ba `MTLBuffer` shared 4 MB xoay theo frame
(`kRingFrames = 3`, `kRingSize = 4 MB`, căn lề 256 byte). Dữ liệu uniform, buffer index
tạm và phần mở rộng FAN/LOOP được cấp phát bump từ slot hiện tại, giữ số
`MTLBuffer` cấp phát trong một frame bằng 0 (`ringAllocs`); số lần dự phòng là
`tempAllocs`.

## Biên đồng bộ

| Cơ chế | Ý nghĩa |
|---|---|
| `cbCommitted` / `cbCompleted` | atomic tăng mỗi lần commit command buffer và mỗi completion handler |
| `GpuBusy()` | `cbCommitted > cbCompleted` |
| `ThrottleGpu()` | chỉ chặn khi việc đang bay vượt `kMaxFramesInFlight`; tính vào `framesThrottled` |
| `Context::CommitAndWait()` | đường duy nhất được phép dùng cho commit-and-wait chặn; tăng `waitGen` |
| `FlushPendingEncoder()` | đóng encoder đang mở và commit không chờ |
| `glFinish()` | encoder + mọi staging + `CommitAndWait()` |
| `glFlush()` | encoder + mọi staging, không chờ |

`CommitAndWait` phải dùng cho mọi lần chờ chặn để `waitGen` vẫn chính xác; đi vòng qua
nó sẽ làm pool buffer bị mồ côi không cần thiết.

## Chuyển đổi toạ độ

`include/tglmt/GLConvert.h` chứa các phép chuyển đổi thuần CPU:

- **Viewport** — gốc toạ độ của OpenGL ở góc dưới trái còn Metal ở góc trên trái, nên
  viewport `GL_LOWER_LEFT` thành `y = targetHeight - (y + h)`. Dưới `GL_UPPER_LEFT`
  (`glClipControl`) giá trị đi qua không đổi.
- **Depth range** — `GL_NEGATIVE_ONE_TO_ONE` ánh xạ `z` từ `[-1,1]` sang `[0,1]`;
  `GL_ZERO_TO_ONE` đi qua.
- **Scissor** — giữ nguyên, chỉ kẹp toạ độ âm.
- **Z trong shader** — converter nối thêm phép chuyển depth vào stage vertex trừ khi
  program dùng `GL_ZERO_TO_ONE`.
- **Present** — bộ nhớ theo thứ tự GL (hàng 0 là hàng dưới). Pass present vẽ một tam
  giác toàn màn hình lấy mẫu target với V lật, nên một draw tạo ra phép lật dọc. Nếu không
  tạo được pipeline đó, present rơi về blit theo từng hàng — luôn đúng nhưng chậm hơn
  nhiều.
- **Đọc lại** — `glReadPixels` flush GPU và trả các hàng theo thứ tự GL.

## Xử lý framebuffer và texture

FBO được giải về một texture màu cụ thể (cộng texture depth tùy chọn) lúc draw. Attach
với mip level lớn hơn 0 ghi level đó vào attachment; render target có kích thước
`base >> level`. Vẽ hoặc clear vào mip level lớn hơn 0 của texture một level thì bị bỏ
và tính vào `mipLevelSkipped`.

Upload texture tôn trọng row length, skip và alignment của `glPixelStorei`. Định dạng
pixel được ánh xạ từ GL internal format sang `metal::PixelFormat` lúc tạo texture; việc
khai báo lại một texture đang là render target sẽ flush encoder pending trước và hủy các
vùng staging của nó.

`glBlitFramebuffer`, `glCopyTexSubImage2D` và `glGetTexImage` được triển khai là copy
trên GPU khi định dạng khớp, và rơi về copy CPU khi không khớp.

## Xử lý lỗi

`ErrorTracker` là hàng đợi FIFO mã lỗi GL bảo vệ bằng mutex. `glGetError` lấy từ đầu
hàng, đúng đặc tả. Mười lỗi đầu tiên được ghi kèm backtrace để định vị người gọi sai.

`glGetString` báo vendor `TGLMT`, renderer `Apple GPU (Metal)`, version
`4.6 (TGLMT Metal port)` và shading-language version `4.60 (TGLMT MSL)`. Danh sách extension
cố ý giữ cố định sáu mục; danh sách trả về chính là thứ mà LWJGL dựng tập khả năng của nó.

Logging chẩn đoán được đóng bởi `Context::DiagOn()`: biến môi trường `TGLMT_DIAG=1` cho
build thường, hoặc configure với `-DTGLMT_FORCE_DIAG=ON` khi không truyền được biến môi
trường cho tiến trình. Các chẩn đoán bị bỏ qua trong khi cổng đang đóng được đếm
(`diagSkipped`), không bị bỏ im lặng.

## Mỗi subsystem nằm ở đâu

| Subsystem | File |
|---|---|
| Lớp phủ Renderer | `src/core/Renderer.cpp`, `include/tglmt/Renderer.h` |
| Context, staging, cache, counter | `src/core/Context.cpp`, `include/tglmt/Context.h` |
| Shadow trạng thái | `src/core/StateTracker.cpp`, `include/tglmt/StateTracker.h` |
| Bảng đối tượng, hàng đợi lỗi | `src/core/ObjectRegistry.cpp`, `src/core/ErrorTracker.cpp` |
| Dispatch GL | `src/gl/gl_*.cpp` |
| GLSL sang MSL | `src/gl/glsl_to_msl.cpp` |
| Đường draw trễ | `src/gl/gl_apple_draw.cpp` |
| Export C, shim cửa sổ | `src/gl/gl_c_exports.cpp`, `src/gl/glfw_shim.cpp` |
| Trừu tượng Metal | `include/tglmt/MetalInterface.h` |
| Backend Apple | `src/metal/AppleMetalBridge.mm` |
| Backend Null | `src/metal/NullMetalBackend.cpp` |
| Chuyển đổi toạ độ | `include/tglmt/GLConvert.h` |
| Shader Metal viết tay | `shaders/*.metal` |
| Trình sinh header | `tools/gen_gl_headers.py`, `tools/gen_c_exports.py`, `tools/gen_stubs.py` |
