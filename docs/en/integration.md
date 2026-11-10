# Integration

This page covers embedding TGLMT into an iOS application: which artefact to ship, how to
load it, what the layer must look like, and how to verify a first render.

## Choosing an artefact

| Artefact | Produced by | Load with | Choose when |
|---|---|---|---|
| `libtglmt.a` | any host or iOS build | link at build time | you control the whole link, or you ship a static binary |
| `libtglmt.dylib` | `TGLMT_BUILD_SHARED=ON` (default) | `dlopen` + `dlsym` | a separate loader resolves `gl*` at runtime, as LWJGL does |

Both are built from the same sources, so behaviour is identical. The shared library is
built with default symbol visibility (`CXX_VISIBILITY_PRESET default`,
`VISIBILITY_INLINES_HIDDEN OFF`) specifically so `dlsym(handle, "glDrawElements")`
resolves.

### iOS cross-compile

```sh
cmake -S . -B build-ios \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT="$(xcrun --sdk iphoneos --show-sdk-path)" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DTGLMT_APPLE_METAL=ON \
  -DTGLMT_BUILD_SHARED=ON \
  -DTGLMT_BUILD_TESTS=OFF
cmake --build build-ios -j8
```

Ship `build-ios/libtglmt.dylib` inside the application bundle, for example under
`Frameworks/`.

## Loading at runtime

```c
void* lib = dlopen("libtglmt.dylib", RTLD_NOW | RTLD_LOCAL);
if (!lib) { /* dlerror() */ }

typedef void* (*getproc_t)(const char*);
getproc_t getproc = (getproc_t)dlsym(lib, "TGLMT_GetProcAddress");

/* Standard loader flow: resolve each gl* name once, cache the pointer. */
glDrawElements_t p = (glDrawElements_t)getproc("glDrawElements");
```

`TGLMT_GetProcAddress` resolves names through an in-process table generated from
`docs/khronos/gl.xml` and returns `NULL` for anything it does not know. It resolves the
same 701 symbols that `libtglmt.dylib` exports directly.

If you do not need dynamic resolution, skip `dlopen` and link the library, then call the
C functions directly:

```c
extern TGLMT_Window* TGLMT_CreateWindow(uint32_t, uint32_t, const char*, int);
extern int            TGLMT_MakeCurrent(TGLMT_Window*);
extern int            TGLMT_SwapBuffers(TGLMT_Window*, void*);
```

The complete list of ten `TGLMT_*` functions is in
[API Reference](api.md).

## CAMetalLayer

TGLMT presents by asking the layer for a drawable and writing into it.

| Requirement | Why |
|---|---|
| `layer.pixelFormat = MTLPixelFormatRGBA8Unorm` | `presentTarget` compares the drawable's format against the target's and returns `false` on a mismatch |
| The layer must be attached to a window and on screen | `nextDrawable` returns `nil` otherwise |
| Keep `drawableSize` in step with the view | TGLMT letterboxes a mismatch instead of overrunning, which shows up as a cropped or offset image |
| `framebufferOnly = NO` recommended | the first three presents read one pixel back from the drawable as a flip self-check; with `framebufferOnly = YES` that read is rejected, the check concludes the flip failed, and TGLMT falls back to the per-row blit present path for the rest of the session |

### drawableSize

`drawableSize` is the layer's own property; TGLMT does not set it. The application owns
the relationship between `view.bounds * contentScaleFactor` and `drawableSize`. What TGLMT
does guarantee is that the render target is created at the size passed to `Init` or
`Resize`, and that present copies `min(target, drawable)` in each dimension.

On rotation or any size change, call `TGLMT_ResizeWindow` (or `Renderer::Resize`). The
target is destroyed immediately and rebuilt at the next `BeginFrame`, and the following
frame is forced to clear. The frame that observes the new size therefore never draws into
a stale target.

### Present path

`presentTarget` renders one full-screen triangle that samples the render target with a
flipped V, writing into the drawable in a single render pass. Only the first three
presents perform the pixel self-check. If the flip pipeline cannot be created, or the
self-check reports that the drawable is black while the source has colour, TGLMT switches
to a per-row blit for all subsequent frames: correct, but with a per-row setup cost that
is visible on A11-class hardware.

## Threading

| Rule | Detail |
|---|---|
| One Context, one thread at a time | the current Context is `thread_local`; each thread calls `MakeCurrent` itself |
| Serialise externally | an application that interleaves a chunk-upload thread with the render thread must hold its own mutex around `MakeCurrent` and the GL work |
| No share lists | objects created on one Context are invisible to another |
| `CreateWindow`, `SwapBuffers` | internally locked; the rest of the GL surface is not |

TGLMT itself never blocks on the GPU at the frame boundary. It commits and waits only
when the number of in-flight command buffers exceeds `kMaxFramesInFlight` (2), which is
counted as `framesThrottled`.

## Launcher integration

A launcher needs to perform this once at startup and once per frame.

```c
/* once */
TGLMT_Window* w = TGLMT_CreateWindow(width, height, "apple", /*depth*/ 1);
TGLMT_MakeCurrent(w);

/* once: give LWJGL the resolver */
register_proc_address(TGLMT_GetProcAddress);

/* per frame */
TGLMT_MakeCurrent(w);              /* if another thread borrowed the Context */
renderFrame();                     /* ordinary gl* calls */
TGLMT_SwapBuffers(w, (__bridge void*)metalLayer);

/* on rotation */
TGLMT_ResizeWindow(w, newWidth, newHeight);
```

Set the layer format before the first present:

```objc
metalLayer.pixelFormat = MTLPixelFormatRGBA8Unorm;
metalLayer.framebufferOnly = NO;
```

Write `stderr` to a file so the frame summary is captured. The first 12 swaps and then
every 600th swap emit two grep-able lines beginning with `[TGLMT] build=` and
`[TGLMT] ir`.

## First-render verification

Work through these in order; each one isolates a different layer.

1. **Is the library loaded?** `TGLMT_BackendName()` must return a non-empty string.
2. **Is a real device active?** `TGLMT_HasRealGPU()` must return `1`. A value of `0` with
   a name of `apple(null-fallback,no-MTL-device)` means the build lacks
   `TGLMT_APPLE_METAL` or device creation failed.
3. **Is anything being drawn?** In the frame summary, `dAtt` and `dEnc` must both be
   non-zero and close to each other.
4. **Is the target present?** The summary prints `target=0x… WxH`; a null pointer or `0x0`
   size means `BeginFrame` never obtained a target.
5. **Is the viewport sane?** `vp=WxH@x,y` must be non-degenerate. An empty viewport means
   no fragment passes.
6. **Is the content correct on the GPU?** `center=(r,g,b,a)` reads the middle pixel of the
   default target. A non-zero value that still shows black on screen points at the present
   path rather than the renderer.
7. **Is present succeeding?** The trailing `present=1` flag, plus `present=#d
   drawable=WxH target=WxH copy=WxH` lines for the first three frames.

## Black-screen diagnosis

Read the two `[TGLMT]` lines first. They separate the failure classes:

| Observation | Interpretation | Next step |
|---|---|---|
| `present=0` | `presentTarget` refused | check `pixelFormat` against `MTLPixelFormatRGBA8Unorm` |
| `dEnc = 0` while `dAtt > 0` | draws are being dropped | check `noProg`, `noTgt`, `noPipe`, `misc` in the same line |
| `target=0x0` or `0x0` size | no render target | `Init`/`BeginFrame` failed, or `Resize` was called with a zero size |
| `vp=0x0` | nothing can rasterise | `glViewport` was never called, or was called with a zero size |
| `dEnc ≈ dAtt` but `center=(0,0,0,0)` | the draw path ran but produced nothing | vertex data, uniforms, textures |
| `center` non-zero but the screen is black | the present path is at fault | compare `drawable=` and `target=` in the present log line; a format mismatch prints `presentTarget FORMAT MISMATCH` |
| No new `[TGLMT]` lines at all | the process is stuck, not slow | a hang rather than a black frame |

The `ir` line gives the batching picture: `encNew` close to `encReuse` means the encoder
is being recreated per draw, which is correct but slow; `encNew` far below `encReuse` is
the intended batching behaviour.

Diagnostic output is gated. Set `TGLMT_DIAG=1` in the environment, or configure with
`-DTGLMT_FORCE_DIAG=ON` when the launcher cannot pass environment variables to the
process.

## AquariumBench

`apps/aquarium/` is a self-contained benchmark application, not the integration guide. It
links `libtglmt.a`, renders a fixed scene and reports frame statistics, and it is the
reference for measuring batching behaviour on real hardware.

Build and deploy instructions specific to that app live in
[`apps/aquarium/DEPLOY_IOS.md`](../../apps/aquarium/DEPLOY_IOS.md). Use it to answer
"how fast is this on a given device"; use this page to answer "how do I embed TGLMT".

## Reference shells

`apps/aquarium/macos_shell.mm` and `apps/aquarium/ios_shell.mm` are minimal platform
shells that create a window, drive the frame loop and own the layer. They are the
smallest working examples of the integration contract on each platform.
