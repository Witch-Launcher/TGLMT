# OpenGL 4.6 Core → Metal — Mapping & Feasibility Matrix

> Scope: **OpenGL 4.6 Core Profile** (657 entry points, per
> `tools/gl46_catalog.py`) on **Apple Metal** targeting **iOS / Apple A11**.
>
> The per-function mapping table lives in `docs/mapping/GL46_to_Metal.md`
> (657 rows, one per core entry point). This document is the *feasibility*
> view: for every **capability** of OpenGL 4.6 Core, how it is realised on
> Metal, what the semantic differences are, what the cost is, and what cannot
> be done on A11.

---

## 1. Classification scheme

| Class | Meaning |
|---|---|
| **A — Direct mapping** | A Metal API with equivalent semantics. The GL call becomes a Metal call (possibly with a state read). |
| **B — Software emulation** | No Metal equivalent; the behaviour is reproduced on the CPU (or by several Metal calls). |
| **C — Hybrid** | Metal + CPU/subsystem cooperation is needed to keep the semantics. |
| **D — Hardware/platform limited** | Metal on A11 genuinely cannot do it. The limit is stated, with its consequence. |
| **E — Not feasible / unverified** | No implementation exists that is known to be correct. |

**Class D and E are never reported as "fully implemented".**

---

## 2. Feasibility matrix

### 2.1 Rendering pipeline

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| Primitive assembly (`POINTS`…`TRIANGLE_FAN`) | A | `drawPrimitives:type:` | `TRIANGLE_FAN` is expanded **on the CPU** (`gl_apple_draw.cpp:1913`) because Metal has no fan primitive. Cost: O(n) CPU + an index upload per draw. `LINE_LOOP` likewise. |
| `LINE_LOOP` / `TRIANGLE_FAN` with `baseVertex` | **D** | — | The CPU expansion bails out for `baseVertex != 0` (`gl_apple_draw.cpp:1945`); the whole draw is dropped. |
| Primitive restart (`GL_PRIMITIVE_RESTART_INDEX`) | B | Split the draw into runs on the CPU | Reads every index via a CPU readback per draw (`gl_draw.cpp:259`). Cost: O(n) per draw. |
| Indexed draws (`GL_UNSIGNED_BYTE/SHORT/INT`) | A | `drawIndexedPrimitives:type:` | `GL_UNSIGNED_BYTE` is expanded to `UInt16` on the CPU. |
| `baseVertex` | B | Index rewriting on the CPU (`gl_apple_draw.cpp:3540`) | Metal has no base-vertex. Cost: O(n) CPU per draw; cached by `(ebo, offset, count, baseVertex, elemType, contentGen)`. |
| `baseInstance` / `gl_BaseInstance` | A | `drawPrimitives:...:baseInstance:` | Direct. |
| Indirect draws (`glDrawArraysIndirect`, `glDrawElementsIndirect`) | **B (partial)** | The command struct is read back on the CPU and re-issued as a normal draw (`gl_draw.cpp:359`) | **Semantic difference**: GL reads the command buffer *at execution time*. A compute shader that generates draw commands is therefore read one frame early. `glDispatchComputeIndirect` *is* real (`dispatchThreadgroupsWithIndirectBuffer:`). |
| `glMultiDrawArrays` / `glMultiDrawElements` | A | N Metal draws | N pipeline lookups. |
| `glDrawTransformFeedback` | B | Replay of the captured vertex count | No attribute replay; the transform-feedback buffer is not re-bound as attributes. |
| Tessellation (TCS/TES) | A (C) | `MTLTriangleTessellationFactorsHalf`, `drawPatches` | Implemented via a compute-style TCS kernel plus a factor buffer. `quads` and `triangles` domains supported; **`isolines` and `point_mode` are rejected** (`glsl_to_msl.cpp:8533`). |
| Geometry shaders | **D** | — | Metal has no geometry shader. The converter **rejects GS with a clear log** (`gl_shader.cpp:44`). A mesh-shader path exists behind `MTLGPUFamilyApple7` (A14+), **not** on A11. |
| Primitive ordering | A | — | — |

### 2.2 Rasterisation

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| Viewport / scissor (×16) | A (partial) | `setViewport:`, `setScissorRect:` | Metal has **one** viewport per render pass. `GL_MAX_VIEWPORTS` is reported as 16 but only index 0 is applied (`gl_apple_draw.cpp:2598`). **D**. |
| `GL_POLYGON_MODE` | A | `setTriangleFillMode:` | — |
| Face culling / `glFrontFace` | A | `setCullMode:` | — |
| Depth test / write mask | A | `MTLDepthStencilState` | — |
| Stencil test (front/back, 8 ops) | A | `MTLDepthStencilState` + `setStencilReference:` | — |
| Polygon offset | A | `setDepthBias:slopeScale:clamp:` | — |
| `GL_SAMPLE_SHADING` / `GL_MIN_SAMPLE_SHADING_VALUE` | **D** | — | Metal has no per-sample shading rate. The state is tracked and reported but has no rasterisation effect (`gl_apple_draw.cpp:2579`). |
| `GL_COLOR_LOGIC_OP` | **D** | — | Metal has no logic op. The blend stage is bypassed (`gl_apple_draw.cpp:2299`), but the logic op itself is **not** performed. |
| Line width > 1 | **D** | — | Metal draws 1-pixel lines only. `GL_ALIASED_LINE_WIDTH_RANGE` is reported as (1, 1). |
| Point size | A | `[[point_size]]` | — |
| Clip distances (`gl_ClipDistance`, ≤8) | A (partial) | `[[clip_distance]]` | Metal has **one** clip distance; `GL_MAX_CLIP_DISTANCES` is reported as 8 but only `[0]` is implemented (`glsl_to_msl.cpp:5938`). **D**. |
| `glProvokingVertex` | A | provoking-vertex convention is fixed in Metal | Only the Metal default (last vertex) is available. |

### 2.3 Blending and colour

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| Per-draw-buffer blend (`glBlendFunci`, `glBlendEquationi`) | A | `MTLRenderPipelineColorAttachmentDescriptor.blending` | — |
| `GL_COLOR_WRITEMASK` per buffer | A | `writeMask` | — |
| `GL_BLEND_COLOR` | A | `setBlendColor:` | — |
| Dual-source blending (`layout(index=1)`) | **D** | — | Metal 3.0 has `[[dual_source_color]]`, but that requires **Apple9/A14+**. On A11 it is impossible. TGLMT maps `index=1` to `[[color(1)]]` (a second draw buffer) — a **documented divergence**, not GL-conformant dual-source blending. |
| sRGB framebuffers | A | `MTLPixelFormatRGBA8Unorm_sRGB` | — |

### 2.4 Framebuffers and MRT

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| FBO / renderbuffer objects | A | `MTLRenderPassDescriptor` + attachments | — |
| Multiple render targets | A | `colorAttachments[0..7]` | A `GL_NONE` draw buffer is a **hole**: verified against the real Metal compiler that `MTLPixelFormatInvalid` for that slot is accepted (`benchmarks`/audit note in `reports/AUDIT_REPORT.md`). |
| `glReadBuffer` / `glDrawBuffers` | A | per-FBO state | — |
| Framebuffer completeness | C | `CheckFramebufferComplete` (`gl_framebuffer.cpp:441`) | `INCOMPLETE_MULTISAMPLE` and `FRAMEBUFFER_UNSUPPORTED` are **not** detected. Size-mismatched attachments return `GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS` — a token **removed in GL 3.0**; in GL 4.6 a size mismatch is legal (effective size = intersection). **Divergence.** |
| MSAA (`glRenderbufferStorageMultisample`) | A | `MTLTextureType2DMultisample` + `MTLStoreActionMultisampleResolve` | MSAA **depth** is an internal Metal attachment, not the application's depth texture ⇒ depth readback from an MSAA FBO is wrong. |
| `glBlitFramebuffer` | B | `MTLBlitCommandEncoder` (GPU) or CPU fallback | `GL_LINEAR` + scaling is implemented as **nearest** (`gl_framebuffer.cpp:695`). MRT blits only handle attachment 0. |
| `glReadPixels` | C | GPU readback of the whole target, then crop + convert | Only RGBA8 sources are supported; float attachments return quantised or zero data. Out-of-bounds reads are clamped (legal — the spec leaves them undefined). |

### 2.5 Buffers and memory

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| Buffer objects, `glBufferData`, `glBufferSubData` | A | `MTLBuffer` (shared storage) | — |
| `glMapBufferRange` | A (C) | Pointer into `MTLBuffer.contents()` | The pointer is real and stable while mapped. **However** `glUnmapBuffer` always returns `GL_TRUE` (a lost mapping is never reported) and `GL_BUFFER_ACCESS_FLAGS` is not exposed. |
| Persistent mapping (`GL_MAP_PERSISTENT_BIT`) | C | Shared storage, mapping pinned across re-specification | Re-specifying a mapped buffer is now **rejected** with `INVALID_OPERATION` (was a use-after-free). |
| `glCopyBufferSubData` | B | CPU `memmove` + staged upload | Not a GPU copy. |
| `glGetBufferSubData` | C | Read `MTLBuffer.contents()` | **No GPU barrier** for non-compute writers. |
| Uniform/SSBO blocks | A | `constant`/`device` buffer bindings | MSL uses *natural* alignment, not std140 — the packer expands tight GL data into the MSL layout (`include/tglmt/uniform_pack.h`). |
| `glBufferStorage` immutability | A | flags tracked in `BufferObject` | `GL_TEXTURE_IMMUTABLE_FORMAT`/`_LEVELS` are not exposed. |

### 2.6 Shaders and programs

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| GLSL → MSL | B (C) | Custom translator (`glsl_to_msl.cpp`) | No SPIR-V. Not a compiler front end — see `docs/GLSL_460_COMPILER.md`. |
| `glLinkProgram` | A | `newLibraryWithSource` + `newRenderPipelineStateWithDescriptor` | — |
| Uniform location reflection | A | Converter's own reflection | — |
| `glGetProgramBinary` / `glProgramBinary` | A | Serialised MSL | The binary is MSL, not SPIR-V — a driver-specific format. |
| Parallel shader compile | A | Synchronous compile | `GL_MAX_SHADER_COMPILER_THREADS` is reported as 1. |
| Subroutines | B | `uniform int` + a `switch` dispatch | No Metal equivalent of GLSL subroutines; emulated with a dispatch table. |

### 2.7 Compute

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| `glDispatchCompute` | A | `dispatchThreads:` | — |
| `glDispatchComputeIndirect` | A | `dispatchThreadgroupsWithIndirectBuffer:` | — |
| Shared memory (`shared` block) | A | `threadgroup` | — |
| `barrier()` / `memoryBarrier*` | A (C) | `threadgroup_barrier(mem_flags:)` | GL's memory model is finer-grained than Metal's; the implementation emits the strongest Metal barrier. |
| Atomics | A | `atomic_fetch_*_explicit` | — |
| Image load/store | A | `texture2d<T, access::RW>` | — |
| `GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS` (1024) / shared memory (32 KB) | A | Metal tier-1 limits | The limits are **reported but not enforced**; exceeding them aborts the Metal command buffer. |

### 2.8 Synchronisation and queries

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| `glFenceSync` / `glWaitSync` | **E** | — | No `MTLEvent` is created. `glClientWaitSync` always returns `ALREADY_SIGNALED` and never blocks (`gl_sync_query.cpp:26`). **The CPU-blocking contract of GL is not honoured.** |
| `glMemoryBarrier` | A (C) | Commit + encoder ordering | The per-bit GL semantics are not reproduced (all bits are treated identically), but within a command buffer Metal's encoder ordering is stronger than GL requires. |
| `glMemoryBarrierByRegion` | A | aliased to `glMemoryBarrier` | Legal (ByRegion may be stronger than required). |
| `glFinish` / `glFlush` | A | `waitUntilCompleted` / commit | — |
| `GL_SAMPLES_PASSED` occlusion query | A | `MTLVisibilityResultModeCounting` | **Real**, but requires `MTLGPUFamilyApple3` or later. A11 = Apple4 ⇒ available. |
| `GL_ANY_SAMPLES_PASSED` | A | `MTLVisibilityResultModeBoolean` | Real. |
| `GL_TIMESTAMP` / `GL_TIME_ELAPSED` | C | `GPUStartTime`/`GPUEndTime` | `GL_TIMESTAMP` is a CPU clock and reports `GL_GPU_DISJOINT_EXT = TRUE` — honest, not a fake GPU timestamp. `GL_TIME_ELAPSED` is per-command-buffer, not per-query, so it is coarse. |
| Primitive/vertex-shader invocation counters | A | Software counters | Exact. |

### 2.9 Debug, transforms and the rest

| GL capability | Class | Metal mechanism | Semantic difference / cost |
|---|---|---|---|
| `GL_DEBUG_OUTPUT` / `KHR_debug` | A | `LogDebug` → stderr callback | Messages are produced by TGLMT itself, not by a Metal debugger. |
| Transform feedback | A (C) | Vertex shader writes to a `device` buffer | Capture is a VS-only path; the replay path is limited (`gl_draw.cpp:515`). |
| `glGetString` / `glGetStringi` | A | Static tables | The extension list is authoritative and `GL_NUM_EXTENSIONS` matches it exactly. |

---

## 3. A11-specific capability check

Verified against Apple's Metal Feature Set tables for `MTLGPUFamilyApple4`
(A11 = iPhone 8 / 8+ / X).

| Capability | A11 value | TGLMT reports | Correct? |
|---|---:|---:|---|
| Max 2D texture size | 16 384 | 8 192 | Conservative (under-reports) |
| Max 3D texture size | 2 048 | 2 048 | Yes |
| Max 2D array layers | 2 048 | 2 048 | Yes |
| Max colour render targets | 8 | 8 | Yes |
| Max MSAA samples | 4 | 4 | Yes |
| Max buffers per stage (tier 1) | 31 | 36 uniform-buffer bindings | **Over-reports** — slots > 30 are silently dropped |
| Max textures per fragment stage (tier 1) | 31 | 32 (`GL_MAX_TEXTURE_IMAGE_UNITS`) | **Over-reports by 1** — a 32-sampler shader aborts Metal validation |
| Max threadgroup memory | 32 KB | 32 768 | Reported, not enforced |
| Max threads per threadgroup | 1 024 | 1 024 | Reported, not enforced |
| Max threadgroups per kernel | 65 535 | not checked | — |
| Max tessellation factor | 16 (Apple3/4) | 16 (`maxTessellationFactor()`) | Yes — the code queries the device family, it is not hard-coded |
| BC texture compression | **not supported** | `GL_COMPRESSED_TEXTURE_FORMATS` advertises BC1–BC7 and S3TC | **Wrong** — the advertised list does not match the device. BC support requires iOS 16.4+ |
| ETC2 / EAC / ASTC | supported | advertised | Yes |
| Programmable sample positions | supported | queried at runtime | Yes |
| Visibility-buffer counting | Apple3+ | queried at runtime | Yes (A11 = Apple4) |

---

## 4. Cost of the emulated paths

Measured on the development machine (see `benchmarks/baseline/` for the raw
JSON; the A11 numbers will differ in absolute terms but the *shape* of the
cost is the same):

| Emulated operation | Cost per call | Notes |
|---|---|---|
| `TRIANGLE_FAN` expansion | O(vertices) CPU + index upload | Per draw |
| `baseVertex` index rewriting | O(indices) CPU + upload | Cached by content generation |
| Primitive-restart run splitting | O(indices) CPU readback | Per draw, per frame |
| `GL_LINEAR` scaled blit | Falls back to nearest | Visual divergence, not just slow |
| Indirect draw | CPU readback of the command struct | One-frame-stale semantics |
| GLSL → MSL | 0.22 ms (VS) / 0.72 ms (FS) / 0.28 ms (CS) | Once per shader, at load time |
| MSL → Metal library | 0.06 ms median, **175 ms p95** (cold Metal compile) | Dominated by the Metal compiler |
| Pipeline creation | 0.007 ms (cache hit) | |

---

## 5. What is genuinely blocked on A11

These are **not** implementation gaps — they are physical limits of the
Metal API surface available to A11:

1. **Geometry shaders** — Metal has no GS. (A mesh-shader path exists but
   needs Apple7/A14+.)
2. **Dual-source blending** — `[[dual_source_color]]` needs Metal 3.0
   (Apple9/A14+).
3. **Per-sample shading rate** — no Metal equivalent at all.
4. **Wide (> 1 px) lines** — Metal's line rasterisation is 1 px wide.
5. **BC1–BC7 compressed textures** — A11 has no BC decoder; ETC2/EAC/ASTC
   work.
6. **More than one viewport / clip distance per pass** — Metal has one of each.
7. **`GL_COLOR_LOGIC_OP`** — Metal has no logic-op stage.
8. **Tier-2 resource barriers (`MTLAccessor`)** — Apple9/M1 only; A11 uses
   encoder-boundary ordering instead (which is sufficient and correct for
   tier 1).

Everything else in the OpenGL 4.6 Core surface has either a direct Metal
mapping (class A), a working emulation (class B/C), or is documented above as
a limitation with its exact consequence.
