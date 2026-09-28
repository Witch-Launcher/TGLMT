# Gắn TGLMT làm render backend (OpenGL → Metal iOS)

TGLMT là lớp dịch OpenGL 4.6 Core sang Metal. App của bạn viết render
**hoàn toàn bằng OpenGL 4.6** (`tglmt::gl`), TGLMT dịch runtime sang Metal
trên iOS/macOS. Class `tglmt::Renderer` (`include/tglmt/Renderer.h`) gói toàn
bộ việc này thành 5 lệnh gọi.

## 1. Link

- Headers: `include/` — app chỉ cần `tglmt/Renderer.h` + `tglmt/gl46.h`.
- Lib đã build:
  - iOS device (arm64): `build-iphoneos/libtglmt.a` + `-framework Metal
    -framework Foundation -framework QuartzCore`
  - macOS: `build-apple/libtglmt.a` + các framework trên + `-framework Cocoa`
    (cho shell/AppKit của bạn).
- App **không bao giờ** biên dịch lại sources TGLMT — chỉ link artifact.

## 2. Vòng đời tối giản

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"

tglmt::Renderer renderer;
// fmt PHẢI khớp CAMetalLayer.pixelFormat của bạn, ngược lại EndFrame(layer)
// trả false trung thực (màn hình sẽ đen nếu bạn bỏ qua giá trị trả về!).
renderer.Init("apple", width, height, /*depth=*/true,
              tglmt::metal::PixelFormat::RGBA8Unorm);

// Mỗi frame (SAU Init/BeginFrame mới được gọi lệnh gl:: — xem §4):
renderer.BeginFrame();          // MakeCurrent + default target (FBO 0)
gl::glViewport(0, 0, width, height);
gl::glClearColor(...); gl::glClear(...);
gl::glUseProgram(prog);         // program GLSL đã compile/link như OpenGL thật
gl::glBindVertexArray(vao);
gl::glDrawArrays(...);          // / glDrawElements... — dịch sang Metal tại đây
renderer.EndFrame(metalLayer);  // CAMetalLayer*: blit + present; nullptr = headless

// Khi xoay/resize màn hình:
renderer.Resize(newW, newH);

// Đọc pixel / chụp màn hình (gốc bottom-left, chuẩn GL):
renderer.ReadPixels(x, y, w, h, buffer);

// Chẩn đoán (nên log ra console/file, như AquariumBench làm):
tglmt::RendererStats s = renderer.stats(); // drawsAttempted/drawsEncoded/...
if (!renderer.hasRealGPU()) { /* đang trace-only: CI không GPU hoặc build null */ }
```

## 3. Shell nền tảng (bạn tự viết, ~100 dòng)

TGLMT không tạo cửa sổ. Shell của bạn chỉ làm:
1. Tạo `CAMetalLayer` (`pixelFormat` phải trùng `fmt` truyền vào `Init` —
   **đọc lại `layer.pixelFormat` sau khi gán** vì iOS có thể ép sang format
   khác; mismatch = `EndFrame` false = đen màn hình).
2. `framebufferOnly = NO` (present đi bằng blit copy, TBDR mới nhận).
3. Vòng lặp: `BeginFrame → lệnh gl:: → EndFrame(layer)`.
4. Resize → `Renderer::Resize`.

Xem mẫu đầy đủ: `apps/aquarium/macos_shell.mm` (AppKit) và
`apps/aquarium/ios_shell.mm` (UIKit + CADisplayLink).

## 4. Quy ước bắt buộc (vi phạm là lỗi khó tìm)

- **Mọi lệnh `gl::` phải chạy SAU `BeginFrame`** (nó `MakeCurrent` nội bộ).
  Gọi trước đó sẽ rơi vào Context fallback và object (VAO/program/buffer)
  lạc sang chỗ khác — draw sau đó im lặng không hiện gì.
- Shader là **GLSL 460 core**, trong subset của `GLSLConverter`
  (xem `include/tglmt/GLSLConverter.h`): `layout(location=)` cho attribute,
  varying, uniform, fragment-out; `uniform` float/vec/mat + `sampler2D`;
  `texture()`, `discard`, `gl_PointSize/Coord`, `gl_FragCoord` được hỗ trợ.
  Ngoài subset → compile fail trung thực (log trong `glGetShaderInfoLog`),
  không sinh code đoán mò.
- Varying `out` bên vertex phải có `in` cùng tên bên fragment, không là
  link fail (đúng semantics OpenGL).
- `glDrawElements(..., nullptr)` với EBO đang bind = offset 0 (đúng spec),
  không phải client pointer.

## 5. Backend Null (CI không GPU)

`Init("null", ...)` luôn thành công: draw trace-only, `ReadPixels` đọc từ
shadow (ví dụ clear đỏ → đọc ra đỏ), `hasRealGPU()` false, stats cân bằng
(`drawsAttempted == drawsEncoded`, vì không có gì để fail). Dùng để test
logic app trên Linux/CI.

## 6. Giới hạn đã biết (ghi rõ, không giấu)

- Tessellation/Geometry stage: trace-only, chưa encode GPU (cờ
  `hasTessStages`/`hasGeometryStage` trong program).
- `baseInstance != 0`: trace-only (Metal draw non-indirect không có khái niệm này).
- Stencil: đã lưu state nhưng chưa encode (depth đã đủ cho đa số scene 3D).
- Mỗi `glDraw*` = 1 command buffer riêng (đúng trước, chưa tối ưu batching).
  Throughput vẫn >100 FPS với ~60 draws/frame trên Intel KBL đo thực tế.
- Xem thêm: `ARCHITECTURE.md` (bảng ánh xạ chi tiết), `COVERAGE.md` (số liệu).
