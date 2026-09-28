# API reference

Two surfaces. C++ for apps linked against the static lib, C for loaders
that `dlsym` by plain name (LWJGL) and for the launcher bridge.

## C++ API (`tglmt::gl`, `tglmt::Renderer`)

Headers: `include/tglmt/gl46.h` (698 decls, generated from `gl.xml`),
`include/tglmt/Renderer.h`.

```cpp
tglmt::Renderer r;
bool ok = r.Init(backend, w, h, depth, fmt);
// backend: "apple" (real GPU) or "null" (CPU trace). depth: attach depth
// buffer. fmt MUST equal the CAMetalLayer pixelFormat.
bool frame = r.BeginFrame();   // MakeCurrent + default target (FBO 0)
bool shown = r.EndFrame(layer); // CAMetalLayer*; nullptr = headless
void r.Resize(w, h);           // rebuild target on next BeginFrame
bool px = r.ReadPixels(x, y, w, h, out); // RGBA8, bottom-left origin
bool gpu = r.hasRealGPU();
```

Call every `gl::` function only after `BeginFrame` on the calling thread.
Each thread needs its own `MakeCurrent`; phase 1 shares one `Context`
serially (see `launcher.md`).

### Context and state (headers under `include/tglmt/`)

| Header | Owns |
|---|---|
| `Context.h` | Per-thread GL state: buffers, VAOs, textures, shaders, programs, FBOs, UBO bindings, clear values, stats |
| `StateTracker.h` | Capability/blend/depth/stencil/cull/viewport/scissor/clip-control shadow used to bake Metal state |
| `MetalInterface.h` | `IDevice`/`IBuffer`/`ITexture`/`IRenderPipeline`/`IRenderEncoder` + `CustomAttrib` (VAO descriptor) + `DrawTrace` |
| `GLSLConverter.h` | `ConvertGLSLtoMSL(src, stage)` result: MSL + varyings/uniforms/samplers/UBO blocks |
| `GLFWShim.h` | C++ `GLFWShim` singleton behind the C API below |

## C API (stable for `dlsym`)

Plain OpenGL entry points: every `gl*` from `gl46.h` is exported unmangled
from `libtglmt.dylib` (`src/gl/gl_c_exports.cpp`, generated — do not edit).
They forward 1:1 to `tglmt::gl::*`.

Window API (`include/tglmt/GLFWShim.h`, `extern "C"`):

```c
TGLMT_Window* TGLMT_CreateWindow(uint32_t w, uint32_t h, const char* backend, int depth);
int  TGLMT_MakeCurrent(TGLMT_Window* w);
void TGLMT_ClearCurrent(void);
int  TGLMT_SwapBuffers(TGLMT_Window* w, void* metalLayer); // CAMetalLayer*
void TGLMT_SwapInterval(int interval);
void TGLMT_ResizeWindow(TGLMT_Window* w, uint32_t width, uint32_t height);
void* TGLMT_GetProcAddress(const char* name); // "glDrawElements", NULL if unknown
int  TGLMT_HasRealGPU(void);
void TGLMT_DestroyWindow(TGLMT_Window* w);
```

Behavior notes: `SwapBuffers` flushes pending draws, presents to the layer,
and starts the next frame. `GetProcAddress` covers the vanilla set (draw,
buffer, VAO, texture, shader, program, FBO, sync); anything else returns
NULL — link the dylib directly if you need all 701 symbols.
