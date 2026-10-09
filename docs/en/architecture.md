# Architecture

This page describes the runtime path of a single OpenGL call, from the entry point to
Metal work, and the deferred-execution model that batches it.

## Layers

```
  Application  (LWJGL / GLFW / Blaze3D / direct gl:: calls)
      |
      |  698 gl* entry points              include/tglmt/gl46.h
      |  10 TGLMT_* window functions        include/tglmt/GLFWShim.h
      v
  +---------------------------------------------------------------------+
  | Dispatch        src/gl/*.cpp                                         |
  |   argument validation, GL error recording, shadow-state updates      |
  |   gl_shader.cpp      glCompileShader  -> GLSL->MSL                   |
  |   gl_program.cpp     glLinkProgram    -> link checks + MSL compile   |
  |   gl_apple_draw.cpp  glDraw*          -> deferred Metal encode       |
  |   gl_texture.cpp / gl_buffer.cpp / gl_framebuffer.cpp / gl_raster.cpp|
  +---------------------------------------------------------------------+
      |
      |  include/tglmt/
      |    StateTracker.h    CPU shadow of GL state, dirty flags
      |    ObjectRegistry.h  id -> object tables (mutex protected)
      |    ErrorTracker.h    FIFO GL error queue
      |    Context.h         per-context everything (buffers, textures,
      |                      programs, FBOs, staging maps, AppleStats)
      v
  +---------------------------------------------------------------------+
  | GLSL -> MSL     src/gl/glsl_to_msl.cpp                               |
  |   vertex + fragment only; emits TGLMT_vs / TGLMT_fs                 |
  +---------------------------------------------------------------------+
      |
      v
  Deferred draw path     src/gl/gl_apple_draw.cpp
      |    state shadowing and redundant-state suppression
      |    encoder reuse across consecutive draws
      |    pipeline-key cache, uniform cache
      |    buffer/texture staging with range coalescing
      |    ring-buffer uploads, conditional flushes
      v
  Metal abstraction      include/tglmt/MetalInterface.h
      |
      +--> src/metal/AppleMetalBridge.mm   Apple backend, real GPU
      +--> src/metal/NullMetalBackend.cpp  Null backend, trace + shadow
      v
  presentTarget(CAMetalLayer)     or     readback from the shadow buffer
```

Two backends implement the same `metal::IDevice` interface. The backend is chosen when
the Context is constructed from the string `"apple"` or `"null"`; a request for `"apple"`
that cannot create a Metal device falls back to the Null device and reports itself as
`apple(null-fallback,no-MTL-device)`.

## Context and state

`Context` owns every object table (buffers, textures, samplers, programs, FBOs, VAOs,
transform-feedback objects), both staging maps, the ring allocator, the pipeline caches
and `AppleStats`.

`StateTracker` holds the CPU shadow of GL state. Its groups are:

| Group | Contents |
|---|---|
| Capabilities | `glEnable` / `glEnablei` per target |
| Blend and colour mask | per-attachment blend state and 4-bit colour masks |
| Depth and stencil | depth func/enable/write, front and back stencil |
| Raster | viewports, cull mode, front face, polygon mode, scissor, polygon offset, line width, clip control |
| Bindings | buffer binding points, per-unit texture slots and sampler bindings, active texture unit, bound VAO, program, XFB, read/draw FBO, patch vertices |
| Pixel store | pack/unpack row length, skip, alignment |
| Get-shadow | `(pname, index)` values returned by `glGet*` |

Every setter compares before assigning. `SetEnabled`, `SetBlendColor`, `SetViewport`,
`SetScissor`, `SetLineWidth`, `SetDepthRangef`, `SetPatchVertices` and `BindXFB` all
return early when the value is unchanged, which also means `stateSeq_` does not advance.
`stateSeq_` is therefore a direct measurement of "N identical GL calls produced zero state
changes".

All `glGet*` calls read the shadow; none of them round-trips to the GPU
(`src/gl/gl_get.cpp`).

## Object registry

`ObjectRegistry` is a mutex-protected `id -> object` table with a monotonically increasing
next id, used for buffers, textures, samplers, programs, shaders, framebuffers, VAOs and
transform-feedback objects. Deletion removes the table entry; the per-context maps
(`c.buffers`, `c.textures`, …) hold the actual payload.

## Shader and program system

`glCompileShader` routes vertex and fragment shaders to `ConvertGLSLtoMSL`. Any other
stage — tessellation control, tessellation evaluation, geometry, compute — is rejected at
compile time with `COMPILE_STATUS = false` and an explicit log
(`src/gl/gl_shader.cpp`). Nothing is silently approximated at this level.

`glLinkProgram` then:

1. checks varying name/location/type agreement between vertex outputs and fragment inputs;
2. checks that every fragment input is written by the vertex stage;
3. computes `trueSize` (largest member end offset) and `minSize` (that value rounded up to
   16) for each uniform block;
4. assigns uniform block bindings from `layout(binding=N)` or from a previous
   `glUniformBlockBinding`;
5. on a real device, compiles the generated MSL to `MTLLibrary` objects and stores them on
   the program. On a Null device this step is skipped entirely.

A program with tessellation or geometry stages, or a vertex-only program, links successfully
because the OpenGL specification requires it, but the draw path then declines to encode and
counts the attempt as trace-only.

Pipeline state objects are created lazily on first use and pre-warmed by
`Context::PrewarmPipelineForProgram` before the first frame.

## Draw-call lifecycle

`glDrawArrays` / `glDrawElements` and their variants converge on `AppleDrawGL`
(`src/gl/gl_apple_draw.cpp`). In order:

1. **Reject early** — no current target, `baseInstance != 0`, a program that is not linked,
   a program with tessellation/geometry stages, `GL_PATCHES`, non-zero `baseVertex` on a
   non-indexed draw. Each rejection increments a dedicated counter rather than silently
   dropping the work.
2. **Resolve resources** — vertex array, element array, bound program, colour and depth
   attachments of the currently bound draw FBO.
3. **Hazard pre-scan** — if the program samples the very texture it renders into, the open
   encoder is ended first and `hazardSplits` increments. This is a TBDR requirement: Metal
   has no automatic read-write hazard tracking for the same resource in one render pass.
4. **Choose or create the render encoder** — reuse when the open encoder exists and the
   draw FBO, colour attachment, depth presence and pending clear all match; otherwise flush
   the old encoder and create a new one.
5. **Clear** — an immediate clear is issued when possible; if not, the clear is recorded as
   `applePendingClear` and consumed by the next draw as `loadAction = Clear`.
6. **Resolve the pipeline** — a `Context::PipelineKey` (vertex/fragment libraries, format,
   vertex stride, depth, blend, colour write mask, and the full vertex attribute layout) is
   compared against a cache. A hit skips the bridge call entirely; a miss calls
   `makeCustomPipeline`.
7. **Upload uniforms** — the per-stage uniform byte buffer is compared against
   `uniformCache` and only a changed stage is re-uploaded. Uniform data comes from the
   ring allocator.
8. **Encode state** — viewport, scissor, cull, fill mode, blend, depth, colour mask are
   written to the encoder only when the pending-encoder shadow says the value changed.
9. **Encode the draw** — FAN and LOOP primitives are expanded into triangles on the CPU
   first, because Metal has neither.
10. **Record usage** — the buffers and textures the draw consumed are appended to the
    pending-usage lists that gate later conditional flushes.

## Deferred execution

TGLMT does not issue one Metal command per OpenGL call. Three independent mechanisms
defer work.

### Encoder lifetime

A single `MTLRenderCommandEncoder` stays open across consecutive draws that belong to the
same render pass. It is ended only when something invalidates the pass:

| Trigger | Site |
|---|---|
| Draw FBO, colour attachment or depth presence changes | `gl_apple_draw.cpp` |
| Feedback hazard detected by the pre-scan | `gl_apple_draw.cpp` |
| Immediate clear | `gl_apple_draw.cpp` |
| Framebuffer blit, `glReadPixels`, `glGetTexImage`, `glCopyTexSubImage2D` | `gl_framebuffer.cpp`, `gl_readpixels_dsa.cpp`, `gl_get_extra.cpp`, `gl_texture.cpp` |
| A texture that is the current render target is re-specified | `gl_texture.cpp` |
| Staging of a resource already used in the open pass | `gl_buffer.cpp`, `gl_texture.cpp` |
| `glFlush` / `glFinish` | `gl_sync_query.cpp` |
| Frame boundary — `SwapBuffers`, `EndFrame`, `Resize` | `glfw_shim.cpp`, `Renderer.cpp` |

`encodersCreated` versus `encoderReused` in the frame summary is the direct measurement of
batching: a healthy frame has `encReuse` far larger than `encNew`.

### Resource staging

`glBufferSubData` and `glTexSubImage2D` do not touch the GPU immediately. They append to
`pendingBufRanges` / `pendingTexRegions`. Adjacent ranges are merged (buffer gap up to 64
bytes; texture regions into a bounding box) before one `memcpy` and one
`didModifyRange` / `replaceRegion`.

Staging only breaks batching when the resource has *already* been used by the open pass.
`MustFlushForBufferStage` and `MustFlushForTextureStage` check the pending-usage lists, and
a declined flush increments `flushAvoided`. This is what lets thousands of uploads coexist
with encoder batching.

In-flight protection uses a three-slot buffer pool; a slot the GPU is still reading is not
written. If no slot is free, TGLMT commits and waits (`bufWaitStall`) instead of corrupting
data. Textures equivalent: `texWasRTFlush` when the only copy lives on the GPU.

### Frame boundary

`SwapBuffers` and `EndFrame` flush the encoder and all staging, then call `ThrottleGpu()`,
which blocks only when the number of command buffers in flight exceeds
`kMaxFramesInFlight` (2). There is no `commitAndWait` per frame: Metal executes command
buffers on one queue in submission order, so frame N+1 cannot observe frame N's writes
early.

## Pipeline caching

Three layers avoid redundant work:

| Layer | Key | Counter |
|---|---|---|
| IR pipeline key | libraries, format, stride, depth, blend, colour mask, attribute layout | `pipelineLookupSkipped` on hit, `pipelineLookups` on miss |
| Bridge PSO cache | descriptor identity inside `AppleMetalBridge.mm` | `pipelineLookups` |
| Encoder-local reuse | identical `nativeHandle()` within one pass | `pipelineReused` |

Depth-stencil states are cached the same way and counted as `depthReused`.

## Ring-buffer uploads

`Context` owns three 4 MB shared `MTLBuffer` objects rotated per frame
(`kRingFrames = 3`, `kRingSize = 4 MB`, 256-byte alignment). Uniform data, temporary index
buffers and FAN/LOOP expansions are bump-allocated from the current slot, which keeps
per-frame `MTLBuffer` allocations at zero (`ringAllocs`); the fallback count is
`tempAllocs`.

## Synchronization boundaries

| Mechanism | Meaning |
|---|---|
| `cbCommitted` / `cbCompleted` | atomics bumped per command-buffer commit and per completion handler |
| `GpuBusy()` | `cbCommitted > cbCompleted` |
| `ThrottleGpu()` | blocks only when in-flight work exceeds `kMaxFramesInFlight`; counted as `framesThrottled` |
| `Context::CommitAndWait()` | the only permitted path to a blocking commit-and-wait; bumps `waitGen` |
| `FlushPendingEncoder()` | ends the open encoder and commits without waiting |
| `glFinish()` | encoder + all staging + `CommitAndWait()` |
| `glFlush()` | encoder + all staging, no wait |

`CommitAndWait` must be used for every blocking wait so that `waitGen` stays accurate;
bypassing it makes the buffer pool orphan unnecessarily.

## Coordinate conversion

`include/tglmt/GLConvert.h` contains the pure-CPU conversions:

- **Viewport** — OpenGL's origin is the lower left and Metal's is the upper left, so a
  `GL_LOWER_LEFT` viewport becomes `y = targetHeight - (y + h)`. Under
  `GL_UPPER_LEFT` (`glClipControl`) the values pass through unchanged.
- **Depth range** — `GL_NEGATIVE_ONE_TO_ONE` maps `z` from `[-1,1]` to `[0,1]`;
  `GL_ZERO_TO_ONE` passes through.
- **Scissor** — identity, with only negative-coordinate clamping.
- **Shader Z** — the converter appends a depth conversion to the vertex stage unless the
  program uses `GL_ZERO_TO_ONE`.
- **Present** — memory is GL-order (row 0 is the bottom). The present pass draws a
  full-screen triangle that samples the target with a flipped V, so one draw produces the
  vertical flip. If that pipeline cannot be created, present falls back to a per-row blit
  that is always correct but much slower.
- **Readback** — `glReadPixels` flushes the GPU and returns GL-order rows.

## Framebuffer and texture handling

FBOs are resolved to a concrete colour texture (plus optional depth texture) at draw time.
Attachment with a mip level greater than 0 records the level on the attachment; the render
target is sized `base >> level`. Drawing or clearing into mip level greater than 0 of a
single-level texture is skipped and counted as `mipLevelSkipped`.

Texture uploads honour `glPixelStorei` pack/unpack row length, skip and alignment. Pixel
formats are mapped from GL internal format to `metal::PixelFormat` at texture creation; a
re-specification of a texture that is currently the render target flushes the pending
encoder first and discards its staged regions.

`glBlitFramebuffer`, `glCopyTexSubImage2D` and `glGetTexImage` are implemented as
GPU-side copies where formats match and fall back to a CPU copy otherwise.

## Error handling

`ErrorTracker` is a mutex-protected FIFO of GL error codes. `glGetError` pops from the
front, matching the specification. The first ten recorded errors are logged with a
backtrace so the offending caller can be located.

`glGetString` reports vendor `TGLMT`, renderer `Apple GPU (Metal)`, version
`4.6 (TGLMT Metal port)` and shading-language version `4.60 (TGLMT MSL)`. The extension
list is deliberately fixed at six entries; the returned list is what LWJGL builds its
capability set from.

Diagnostic logging is gated by `Context::DiagOn()`: the environment variable `TGLMT_DIAG=1`
for a normal build, or configure with `-DTGLMT_FORCE_DIAG=ON` when the process cannot be
given environment variables. Diagnostics skipped while the gate is closed are counted
(`diagSkipped`), not silently discarded.

## Where each subsystem lives

| Subsystem | Files |
|---|---|
| Renderer façade | `src/core/Renderer.cpp`, `include/tglmt/Renderer.h` |
| Context, staging, caches, counters | `src/core/Context.cpp`, `include/tglmt/Context.h` |
| Shadow state | `src/core/StateTracker.cpp`, `include/tglmt/StateTracker.h` |
| Object tables, error queue | `src/core/ObjectRegistry.cpp`, `src/core/ErrorTracker.cpp` |
| GL dispatch | `src/gl/gl_*.cpp` |
| GLSL to MSL | `src/gl/glsl_to_msl.cpp` |
| Deferred draw path | `src/gl/gl_apple_draw.cpp` |
| C exports, window shim | `src/gl/gl_c_exports.cpp`, `src/gl/glfw_shim.cpp` |
| Metal abstraction | `include/tglmt/MetalInterface.h` |
| Apple backend | `src/metal/AppleMetalBridge.mm` |
| Null backend | `src/metal/NullMetalBackend.cpp` |
| Coordinate conversion | `include/tglmt/GLConvert.h` |
| Hand-written Metal shaders | `shaders/*.metal` |
| Header generators | `tools/gen_gl_headers.py`, `tools/gen_c_exports.py`, `tools/gen_stubs.py` |
