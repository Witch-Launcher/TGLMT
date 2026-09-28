# Bắt đầu

## Yêu cầu

- CMake 3.20+, compiler C++17.
- macOS + Xcode cho `build-apple` (encode Metal thật).
- iOS SDK cho build máy thật. Backend Null build ở đâu cũng được, kể cả
  Linux CI.

## Ma trận build

| Mục tiêu | Lệnh | Kết quả |
|---|---|---|
| Logic + test (không GPU) | `cmake -S . -B build && cmake --build build -j8` | `libtglmt.a`, 23 test |
| Metal thật (macOS) | `cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8` | chạy pixel test GPU |
| Lib máy iOS | `cmake -S . -B build-ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=<iphoneos sdk> -DCMAKE_OSX_ARCHITECTURES=arm64 -DTGLMT_APPLE_METAL=ON -DTGLMT_BUILD_TESTS=OFF && cmake --build build-ios` | `libtglmt.dylib` (arm64) |

Options: `TGLMT_APPLE_METAL` (mặc định OFF), `TGLMT_BUILD_TESTS` (mặc định
ON), `TGLMT_BUILD_SHARED` (mặc định ON, ra `libtglmt.dylib`).

## Chạy test

```sh
ctest --test-dir build --output-on-failure                 # backend null
ctest --test-dir build-apple --output-on-failure           # GPU thật
python3 tools/gen_gl_headers.py   # kỳ vọng: core46 functions: 698
python3 tools/gen_stubs.py        # kỳ vọng: implemented: 701, missing: 0
python3 tools/gen_c_exports.py    # sinh lại src/gl/gl_c_exports.cpp
```

Test cần GPU sẽ in `SKIP` và exit 0 khi không có GPU.

## Frame đầu tiên

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"
namespace gl = tglmt::gl;

tglmt::Renderer r;
r.Init("apple", 960, 600, true, tglmt::metal::PixelFormat::RGBA8Unorm);
r.BeginFrame();
gl::glViewport(0, 0, 960, 600);
gl::glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
gl::glClear(0x00004000); // COLOR_BUFFER_BIT
// shader/VAO/draw ở đây
r.EndFrame(metalLayer);
if (!r.hasRealGPU()) { /* đang trace-only, kiểm tra stats */ }
tglmt::RendererStats s = r.stats(); // drawsAttempted/drawsEncoded/noProgram/...
```

Checklist xác minh: `hasRealGPU()` true trên máy thật;
`drawsEncoded == drawsAttempted` nghĩa là mọi draw đã tới Metal; nếu lệch,
đọc `noProgram` / `noTarget` / `noPipeline` / `miscFail` để biết stage nào
hỏng.
