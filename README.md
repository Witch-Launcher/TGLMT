# TGLMT

[Vietnamese version](README.vi.md)

TGLMT is a translation layer that runs **OpenGL 4.6 Core** rendering on **Metal**.
Application code keeps issuing OpenGL calls; TGLMT validates them, tracks GL state on
the CPU, translates GLSL to Metal Shading Language when a program links, and batches
the resulting work into Metal command buffers.

There is no Vulkan driver in the path: no MoltenVK, no ANGLE, no SPIR-V in the runtime.

| | |
|---|---|
| API surface | OpenGL 4.6 Core — 698 declared functions in `include/tglmt/gl46.h`, 701 exported C symbols |
| Shader input | GLSL vertex + fragment, converted directly to MSL at `glCompileShader` |
| Platforms | iOS arm64 device (designed against an A11 baseline), macOS host |
| Backends | `apple` (Metal, real GPU) and `null` (CPU trace/shadow, no GPU needed) |
| Outputs | `libtglmt.a` (static) and `libtglmt.dylib` (shared, for `dlopen`/`dlsym`) |
| License | MIT |

The primary use case is the vanilla boot path of a Minecraft Java launcher on iOS:
LWJGL/GLFW resolve `gl*` symbols at runtime and the renderer issues ordinary
OpenGL calls.

## Architecture

```
  OpenGL 4.6 application (LWJGL / GLFW / Blaze3D / any GL caller)
        |
        |  698 gl* entry points + TGLMT_* window API
        v
  +------------------------------------------------------------------+
  | Dispatch layer      src/gl/*.cpp                                   |
  |   validates arguments, records GL state, shadows glGet*            |
  +------------------------------------------------------------------+
        |
        +--> StateTracker / ObjectRegistry / ErrorTracker   (include/tglmt)
        |
        +--> GLSL -> MSL converter  src/gl/glsl_to_msl.cpp
        |        (vertex + fragment only; other stages rejected honestly)
        v
  Deferred draw path   src/gl/gl_apple_draw.cpp
        |  state shadowing, redundant-state suppression
        |  encoder reuse across draws, pipeline-key cache
        |  buffer/texture staging with coalescing, ring-buffer uploads
        v
  Metal abstraction    include/tglmt/MetalInterface.h
        |
        +--> Apple backend   src/metal/AppleMetalBridge.mm  (real GPU)
        +--> Null backend    src/metal/NullMetalBackend.cpp (trace/shadow)
        v
  CAMetalLayer present   or   headless readback from the shadow buffer
```

A full description of every stage is in [Architecture](docs/en/architecture.md).

## What to read next

| Document | Contents |
|---|---|
| [Getting Started](docs/en/getting-started.md) | Build matrix, tests, first frame, verifying the GPU backend is live |
| [Architecture](docs/en/architecture.md) | Draw-call lifecycle, deferred execution, coordinate conversion |
| [API Reference](docs/en/api.md) | `Renderer`, `tglmt::gl`, the C exports, the GLFW shim |
| [Integration](docs/en/integration.md) | Embedding the dylib, `CAMetalLayer`, threading, black-screen diagnosis |
| [Compatibility & Performance](docs/en/compatibility.md) | What is implemented, partial, trace-only or unsupported; counters; reproducible tests |

## Build and test

```sh
cmake -S . -B build              # Null backend, builds on any host
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON   # macOS, real Metal
cmake --build build-apple -j8
ctest --test-dir build-apple --output-on-failure
```

Tests that need a GPU print `SKIP` and exit 0 when no GPU is present, so both
configurations are safe to run in CI. See
[Getting Started](docs/en/getting-started.md) for the iOS cross-compile and the
full build matrix.

## Documentation site

`docs/index.html` is the bilingual documentation site (English/Vietnamese, dark/light,
search). It is generated from the Markdown in this repository:

```sh
python3 tools/gen_docs_site.py
```

The generator owns the page manifest; `docs/index.html` is only the site template.
See [Getting Started](docs/en/getting-started.md) for the regeneration workflow.
