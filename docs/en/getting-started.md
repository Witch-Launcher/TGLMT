# Getting Started

This page covers the build matrix, how to run the tests, a minimal first frame, and
how to tell whether the real Metal backend is active.

## Prerequisites

| Requirement | Needed for |
|---|---|
| CMake 3.20+ | every build |
| C++17 compiler | every build |
| Python 3 | regenerating `gl46.h`, `gl46_types.h`, `gl_dispatch_gen.cpp`, `gl_c_exports.cpp`, `gl_stubs_gen.cpp` |
| Xcode command line tools | macOS and iOS builds, Metal framework |
| `glslangValidator` + `spirv-cross` | optional; enables `test_spirv_chain_apple` and `test_converter_proof` |
| `xcrun metal` | optional; enables offline MSL compilation checks in `test_converter_aquarium` |

The Null backend builds on any host, including Linux, because it compiles no Apple
source at all.

## Build matrix

All four configurations are supported. `TGLMT_APPLE_METAL` defaults to `OFF`; turning it
on is what pulls in `src/metal/AppleMetalBridge.mm` and links `-framework Metal`.

| Configuration | Configure | Result |
|---|---|---|
| Host, Null backend (default) | `cmake -S . -B build` | `libtglmt.a`, `libtglmt.dylib`, all tests |
| Host, Apple Metal | `cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON` | same, plus real-GPU tests become active |
| iOS device, arm64 | see below | `build-iphoneos/libtglmt.a` |
| iOS device, shared dylib | see below | `build-ios/libtglmt.dylib` arm64 |

### Host, Null backend

```sh
cmake -S . -B build
cmake --build build -j8
```

### Host, Apple Metal (macOS)

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON
cmake --build build-apple -j8
```

This configures CMake to enable Objective-C++, compile `AppleMetalBridge.mm` with ARC,
define `TGLMT_APPLE_METAL=1`, and link `Metal`, `Foundation` and `QuartzCore`.

### iOS device, arm64

Two equivalent invocations. The toolchain-file form is the one used by the root
`Makefile` and is the recommended one, because it also forces
`-miphoneos-version-min`:

```sh
cmake -S . -B build-iphoneos \
  -DTGLMT_APPLE_METAL=ON \
  -DCMAKE_TOOLCHAIN_FILE=apps/aquarium/ios_toolchain.cmake \
  -DIOS_DEPLOYMENT_TARGET=16.0 \
  -DTGLMT_BUILD_TESTS=OFF
cmake --build build-iphoneos -j8
```

Direct form, without the toolchain file:

```sh
cmake -S . -B build-ios \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk iphoneos --show-sdk-path)" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DTGLMT_APPLE_METAL=ON \
  -DTGLMT_BUILD_TESTS=OFF
cmake --build build-ios -j8
```

Architecture is `arm64` only. The deployment target defaults to `16.0`
(`DEPLOY_TARGET` in the root `Makefile`, `IOS_DEPLOYMENT_TARGET` in
`apps/aquarium/ios_toolchain.cmake`).

For building the Aquarium benchmark app on iOS, including signing, see
[`apps/aquarium/DEPLOY_IOS.md`](../../apps/aquarium/DEPLOY_IOS.md).

## Library outputs

Both targets are built from the same source list.

| Target | Type | Output | Notes |
|---|---|---|---|
| `tglmt` | static | `libtglmt.a` | for apps that link directly |
| `tglmt_shared` | shared | `libtglmt.dylib` → `libtglmt.0.dylib` → `libtglmt.0.1.0.dylib` | for `dlopen`/`dlsym` |

Symbol visibility is deliberately left at the C++ default
(`CXX_VISIBILITY_PRESET default`, `VISIBILITY_INLINES_HIDDEN OFF`), because loaders
resolve `gl*` names through `dlsym`. There are no `install()` rules; consume the
artefacts straight out of the build directory.

## Tests

Every file under `tests/unit/` and `tests/integration/` becomes both an executable and a
CTest test named after the file stem.

```sh
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

`make test` wraps this through `scripts/build_and_test.sh`, which additionally
regenerates the GL headers from `docs/khronos/gl.xml` first (pass `--no-gen` to skip).

Tests that need a GPU print `SKIP` and exit 0 when no GPU is present, so a Null-backend
build and an Apple-backend build both report success on an unattended host.

Relevant behaviours to know before reading the output:

- Tests whose real assertions need a GPU are skipped, not failed, when the device is a
  Null device.
- Tests that read the Minecraft shader corpus check that `TGLMT_CORPUS_DIR` exists and
  skip otherwise. The corpus is not committed; fetch it with
  `tests/corpus/fetch-corpus.sh`.
- Tests that shell out to `glslangValidator`, `spirv-cross` or `xcrun metal` skip when
  those tools are absent.

To exercise the Apple backend you need a macOS build:

```sh
cmake --build build-apple -j8
ctest --test-dir build-apple --output-on-failure
```

## First frame

```cpp
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"

tglmt::Renderer r;
if (!r.Init("apple", 960, 600, /*depth=*/true)) return 1;

// All gl:: calls must come after Init / BeginFrame: they act on the current
// Context, and BeginFrame is what makes this Renderer's Context current.
r.BeginFrame();

tglmt::gl::glViewport(0, 0, 960, 600);
tglmt::gl::glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
tglmt::gl::glClear(0x4000 /* GL_COLOR_BUFFER_BIT */);

r.EndFrame(metalLayer);   // nullptr = headless, still returns true
```

All rendering commands live in namespace `tglmt::gl` and take the same parameters as
desktop OpenGL 4.6.

## Runtime invariants

Call these out before writing any code against the API:

1. **`gl::` calls act on the current Context.** `Renderer::BeginFrame` makes the
   Renderer's Context current. Calls made before `Init`/`BeginFrame` fall through to the
   fallback Context and their objects land in the wrong place.
2. **One Context, one thread at a time.** The current Context is `thread_local`.
   Applications that interleave a chunk-upload thread with the render thread must
   serialise externally and call `MakeCurrent` on each thread.
3. **`Resize` takes effect at the next `BeginFrame`.** The render target is destroyed
   immediately and rebuilt lazily; the following frame is forced to clear.
4. **`EndFrame` flushes.** The open render encoder and all staged buffer/texture uploads
   are flushed before the present or before a headless return.
5. **Colour format must match the layer.** `Init` takes the pixel format; `presentTarget`
   refuses a `CAMetalLayer` whose `pixelFormat` differs, which shows up as a black screen.
6. **`BeginFrame(false)` must not rotate anything.** That mode only re-binds the Context,
   for callers that call `MakeCurrent` in the middle of a frame. Rotating the ring or the
   target there corrupts the frame.

## Is the real GPU backend active?

There are four independent ways to check, from cheapest to most detailed.

```cpp
r.hasRealGPU();          // C++: true only when the device is a Metal device
r.backendName();         // C++: "apple", "null", or "apple(fallback:null,no-MTL-device)"
```

```c
TGLMT_HasRealGPU();      // C ABI, for JNI / Objective-C launchers
TGLMT_BackendName();     // C ABI
```

On a Null device the backend name is `apple(null-fallback,no-MTL-device)`, which is the
honest signal that the "apple" backend was requested but no Metal device was created —
either the build was configured without `TGLMT_APPLE_METAL`, or device creation failed.
In that case every draw is trace-only and `ReadPixels` returns the CPU shadow, not GPU
output.

At runtime, stderr carries a frame summary for the first 12 swaps and then every 600th
swap:

```
[TGLMT] build=... frame=1 ... dAtt=... dEnc=... target=0x... WxH vp=... center=(r,g,b,a) present=1
[TGLMT] ir frame=1 encNew=... encReuse=... pipeRe=... pipeLk=... uniRe=... stSkip=...
```

`dAtt` is draws attempted, `dEnc` is draws actually encoded. If `dEnc` is 0 while `dAtt`
is not, draws are being dropped — see
[Compatibility & Performance](compatibility.md) for the failure counters.

## Minimal troubleshooting

| Symptom | First check |
|---|---|
| `presentTarget` refuses, screen stays black | `CAMetalLayer.pixelFormat` must be `MTLPixelFormatRGBA8Unorm` |
| Draws drop to zero | `dAtt` vs `dEnc` in the frame summary; then `noProg`, `noTgt`, `noPipe`, `misc` |
| `ReadPixels` returns black or stale data | `hasRealGPU()` — a Null device reads back the shadow buffer |
| Tests fail to configure | CMake 3.20+ and a C++17 compiler |
| `test_converter_*` or `test_spirv_chain_apple` skip | install `glslangValidator`, `spirv-cross`, `xcrun metal` |
| Minecraft-specific tests skip | run `tests/corpus/fetch-corpus.sh` |
| Diagnostics missing from stderr | set `TGLMT_DIAG=1`, or configure with `-DTGLMT_FORCE_DIAG=ON` for a device where the launcher cannot pass environment variables |

## Regenerating the documentation site

```sh
python3 tools/gen_docs_site.py
```

This reads `README.md`, `README.vi.md` and `docs/{en,vi}/*.md`, converts them to HTML,
emits `docs/content-en.js`, `docs/content-vi.js` and `docs/nav.js`, and prints which
files changed. After editing any canonical Markdown, run it and commit the generated
files together with the source.
