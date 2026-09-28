# TGLMT — OpenGL 4.6 Core sang Metal (iOS)

[English version](README.md) | Giấy phép: MIT | Mục tiêu: iPhone A11 trở lên

TGLMT cho bạn viết code render một lần bằng **OpenGL 4.6 Core** và chạy trên
**Metal** (iOS/macOS). Không MoltenVK, không ANGLE. Làm cho đường boot vanilla
của launcher Minecraft Java trên iOS.

## Có gì

- Đủ mặt API OpenGL 4.6 Core: 698 hàm + exports C trần, loader có sẵn
  (`dlsym("glDrawElements")`) chạy không cần sửa.
- Vẽ GPU thật: VAO/VBO/EBO, indexed + `baseVertex`, instancing, indirect từ
  `DRAW_INDIRECT_BUFFER`, bung `TRIANGLE_FAN`/`LINE_LOOP`.
- Converter GLSL sang MSL, fail trung thực (không đoán code): uniform, mảng
  uniform, MRT tới 8 target, UBO read-only, nhiều sampler.
- Framebuffer + `BlitFramebuffer`, sinh mipmap, depth/blend/cull,
  `ReadPixels` đúng chiều.
- C API cửa sổ (`TGLMT_CreateWindow` / `SwapBuffers` / `GetProcAddress`) để
  nhúng vào launcher; có present lên `CAMetalLayer`.

## Build

```sh
# Logic + test, không cần GPU
cmake -S . -B build && cmake --build build -j8
ctest --test-dir build

# Metal thật trên macOS
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8

# Dylib cho máy iOS (arm64)
cmake -S . -B build-ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path) \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DTGLMT_APPLE_METAL=ON -DTGLMT_BUILD_TESTS=OFF
cmake --build build-ios
```

## Ví dụ tối thiểu

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"

tglmt::Renderer r;
r.Init("apple", 960, 600, true, tglmt::metal::PixelFormat::RGBA8Unorm);
r.BeginFrame();
tglmt::gl::glViewport(0, 0, 960, 600);
tglmt::gl::glClearColor(0, 0, 0, 1);
tglmt::gl::glClear(0x00004000);
// ... compile shader, bind VAO, glDrawArrays ...
r.EndFrame(metalLayer); // CAMetalLayer*; nullptr = headless
```

Quy tắc quan trọng: chỉ gọi `gl::` sau `BeginFrame`; pixel format của layer
phải bằng `fmt` truyền vào `Init`; chi tiết xem `docs/vi/`.

## Tài liệu

- `docs/vi/getting-started.md` — build, test, frame đầu tiên
- `docs/vi/api.md` — tham chiếu API C++ và C
- `docs/vi/architecture.md` — pipeline, luồng draw, ánh xạ tọa độ
- `docs/vi/launcher.md` — nhúng vào launcher iOS
- `docs/vi/limits.md` — phần stub và lý do

## Trạng thái

Render scene vanilla chạy được (kiểm chứng bằng pixel test GPU).
Tessellation, geometry shader và compute encode đang stub trung thực, để dành
cho giai đoạn Sodium/Iris. Xem `docs/vi/limits.md`.
