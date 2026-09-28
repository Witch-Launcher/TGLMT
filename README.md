# TGLMT — OpenGL 4.6 Core to Metal (iOS)

[Vietnamese version](README.vi.md) | License: MIT | Target: iPhone A11 and newer

TGLMT lets you write rendering code once in **OpenGL 4.6 Core** and run it on
**Metal** (iOS/macOS). No MoltenVK, no ANGLE. Built for the vanilla boot path
of Minecraft Java launchers on iOS.

## What you get

- Full OpenGL 4.6 Core API surface: 698 functions + plain-C exports, so
  existing loaders (`dlsym("glDrawElements")`) work unchanged.
- Real GPU draws: VAO/VBO/EBO, indexed + `baseVertex`, instancing, indirect
  from `DRAW_INDIRECT_BUFFER`, `TRIANGLE_FAN`/`LINE_LOOP` expansion.
- GLSL to MSL converter with honest failures (no guessed code): uniforms,
  uniform arrays, MRT up to 8 targets, read-only UBO blocks, multi-sampler.
- Framebuffer + `BlitFramebuffer`, mipmap generation, depth/blend/cull,
  `ReadPixels` with correct orientation.
- C window API (`TGLMT_CreateWindow` / `SwapBuffers` / `GetProcAddress`) for
  embedding in a launcher; `CAMetalLayer` present included.

## Build

```sh
# Logic + tests, no GPU needed
cmake -S . -B build && cmake --build build -j8
ctest --test-dir build

# Real Metal on macOS
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8

# iOS device dylib (arm64)
cmake -S . -B build-ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=$(xcrun --sdk iphoneos --show-sdk-path) \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DTGLMT_APPLE_METAL=ON -DTGLMT_BUILD_TESTS=OFF
cmake --build build-ios
```

## Minimal example

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"

tglmt::Renderer r;
r.Init("apple", 960, 600, true, tglmt::metal::PixelFormat::RGBA8Unorm);
r.BeginFrame();
tglmt::gl::glViewport(0, 0, 960, 600);
tglmt::gl::glClearColor(0, 0, 0, 1);
tglmt::gl::glClear(0x00004000);
// ... compile shaders, bind VAO, glDrawArrays ...
r.EndFrame(metalLayer); // CAMetalLayer*; nullptr = headless
```

Rules that matter: call `gl::` only after `BeginFrame`; the layer pixel
format must equal the `fmt` passed to `Init`; see `docs/en/` for details.

## Docs

- `docs/en/getting-started.md` — build, test, first frame
- `docs/en/api.md` — C++ and C API reference
- `docs/en/architecture.md` — pipeline, draw flow, coordinate mapping
- `docs/en/launcher.md` — embedding in an iOS launcher
- `docs/en/limits.md` — what is stubbed and why

## Status

Vanilla scene rendering works (verified by GPU pixel tests). Tessellation,
geometry shaders and compute encode are honestly stubbed and planned for the
Sodium/Iris stage. See `docs/en/limits.md`.
