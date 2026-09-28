# Getting started

## Requirements

- CMake 3.20+, C++17 compiler.
- macOS with Xcode for `build-apple` (real Metal encode).
- Xcode iOS SDK for device builds. The Null backend builds anywhere,
  including Linux CI.

## Build matrix

| Target | Command | Result |
|---|---|---|
| Logic + tests (no GPU) | `cmake -S . -B build && cmake --build build -j8` | `libtglmt.a`, 23 tests |
| Real Metal (macOS) | `cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8` | GPU pixel tests run |
| iOS device lib | `cmake -S . -B build-ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=<iphoneos sdk> -DCMAKE_OSX_ARCHITECTURES=arm64 -DTGLMT_APPLE_METAL=ON -DTGLMT_BUILD_TESTS=OFF && cmake --build build-ios` | `libtglmt.dylib` (arm64) |

Options: `TGLMT_APPLE_METAL` (default OFF), `TGLMT_BUILD_TESTS` (default ON),
`TGLMT_BUILD_SHARED` (default ON, produces `libtglmt.dylib`).

## Run tests

```sh
ctest --test-dir build --output-on-failure                 # null backend
ctest --test-dir build-apple --output-on-failure           # real GPU
python3 tools/gen_gl_headers.py   # expect: core46 functions: 698
python3 tools/gen_stubs.py        # expect: implemented: 701, missing: 0
python3 tools/gen_c_exports.py    # regenerates src/gl/gl_c_exports.cpp
```

Tests that need a GPU print `SKIP` and exit 0 when none is present.

## First frame

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
// shaders/VAO/draws here
r.EndFrame(metalLayer);
if (!r.hasRealGPU()) { /* trace-only fallback, check stats */ }
tglmt::RendererStats s = r.stats(); // drawsAttempted/drawsEncoded/noProgram/...
```

Verifylist: `hasRealGPU()` true on device; `drawsEncoded == drawsAttempted`
means every draw reached Metal; otherwise read `noProgram` / `noTarget` /
`noPipeline` / `miscFail` to find the failing stage.
