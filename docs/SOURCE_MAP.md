# SOURCE MAP — TGLMT (Phase 0 full repository discovery)

> Scope: every file under `src/**` and `include/**` (66 files, 54 195 LOC measured with
> `wc -l`). Nothing outside `src/`+`include/` is listed here except the test table in §3.
>
> LOC measured 2026-11-11 with `wc -l` over the working tree (not the build dirs).
> Verified totals: `wc -l $(find src include -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.mm' \))`
> → **54195 total**, split **6947** (headers) + **969** (core) + **42706** (gl) + **3573** (metal).
>
> **Layer legend**
>
> | Tag | Layer | Reality in this repo |
> |---|---|---|
> | `L0-HDR` | Public headers, `include/tglmt/` | Exists (18 files). Public by *location* only: `CMakeLists.txt:16` `include_directories(include)` and `:81` `target_include_directories(tglmt PUBLIC include)`. **There are no `install()` rules anywhere** (`grep -n "install" CMakeLists.txt apps/aquarium/CMakeLists.txt` → no match), so nothing is installed/exported as an SDK. |
> | `L0-GEN` | Public headers, machine-generated | Exists (3 files), banner `// AUTO-GENERATED … KHÔNG SỬA TAY` on line 1 of each. |
> | `L1-CTX` | Context + state core (`src/core/`) | Exists (5 files). |
> | `L2-API` | OpenGL entry-point implementation (`src/gl/gl_*.cpp`) | Exists (37 hand-written files, 24 435 LOC). |
| `L2-GEN` | Machine-generated entry-point artefacts (`src/gl/*_gen.cpp`, `gl_c_exports.cpp`) | Exists (4 files, 5 644 LOC). All carry a `// AUTO-GENERATED …` banner on line 1. |
> | `L3-SH` | Shader translation (GLSL→MSL) | Exists, **1 file** (`glsl_to_msl.cpp`). |
> | `L4-MI` | Metal abstraction (`MetalInterface.h`) | Exists (1 header, 83 virtuals on `IDevice`). |
> | `L5-BE` | Metal backend implementation (`src/metal/`) | Exists (2 files). |
> | `L6-LEGACY` | Legacy fixed-function / compatibility engine | **DOES NOT EXIST.** No file, no directory, no symbol. Evidence in `docs/REPO_DISCOVERY.md` §2.4. |
> | `L7-VAL` | Standalone validation layer | **DOES NOT EXIST as a separate layer.** Validation is inlined per entry point (`c.errors.Record(0x0500)` etc.). Only the *pname* whitelist is a separate artefact (`gl46_pnames.h`, generated). |

---

## 1. `include/tglmt/` — public headers (18 files, 6 947 LOC)

| Path | LOC | Layer | Responsibility (one line) | Public header status |
|---|---:|---|---|---|
| `include/tglmt/gl46_types.h` | 1871 | `L0-GEN` | Generated GL scalar typedefs + every core/compat enum value harvested from `docs/khronos/gl.xml`. | Public, **generated** (`tools/gen_gl_headers.py`) |
| `include/tglmt/gl46_pnames.h` | 993 | `L0-GEN` | Generated sorted array + binary-search `IsAcceptedPname()` — the only reusable validation table in the tree. | Public, **generated** (`tools/gen_state_pnames.py`) |
| `include/tglmt/MetalInterface.h` | 926 | `L4-MI` | Pure-C++ abstract Metal API: `IDevice` (83 virtuals) plus `IBuffer/ITexture/ILibrary/IRenderPipeline/IRenderTarget/IRenderEncoder/IComputeEncoder`. | Public (backend abstraction) |
| `include/tglmt/gl46.h` | 716 | `L0-GEN` | Generated prototypes: **657 core** + **42 compatibility-only** entry points in `namespace tglmt::gl`, plus `GetCoreFunctionCount()`/`GetCompatOnlyFunctionCount()`. | Public, **generated** (`tools/gen_gl_headers.py`) |
| `include/tglmt/StateTracker.h` | 304 | `L0-HDR` | The whole GL state machine (caps, blend×8, depth/stencil, viewport×16, scissor×16, raster, pixel-store, bindings) + generic `SetShadow/GetShadow` store for `glGet*`. | Public (core) |
| `include/tglmt/GLSLConverter.h` | 190 | `L0-HDR` | Shader-translation contract: `GLSLVar/GLSLBlock/GLSLConvertResult` + `ConvertGLSLtoMSL()`. | Public (shader layer) |
| `include/tglmt/glsl_type.h` | 172 | `L0-HDR` | GLSL type-name ↔ GL type enum mapping used by uniform packing and `glGetActiveUniform`. | Public, GL-layer helper |
| `include/tglmt/uniform_pack.h` | 107 | `L0-HDR` | std140 layout/packing rules for uniform blocks (shared by `gl_uniform.cpp` and `glsl_to_msl.cpp`). | Public, GL-layer helper |
| `include/tglmt/Renderer.h` | 79 | `L0-HDR` | Embedding façade: `Init/Resize/BeginFrame/EndFrame/ReadPixels/hasRealGPU` over one `Context`. | Public (app façade) |
| `include/tglmt/GLFWShim.h` | 65 | `L0-HDR` | Minimal GLFW-shaped shim for the Minecraft-Java-on-iOS launcher + `extern "C"` `TGLMT_*` C API. | Public (app façade) |
| `include/tglmt/GLConvert.h` | 60 | `L0-HDR` | Pure-CPU GL→Metal coordinate conversion (`GLViewportToMetal`, `GLScissorToMetal`). | Public, GL-layer helper |
| `include/tglmt/GlProfile.h` | 39 | `L0-HDR` | Per-`gl*` call-count/timing RAII probe (`GlProfileScope`) feeding the `gltop#` diagnostics. | Public, diagnostic |
| `include/tglmt/UniformQuery.h` | 29 | `L0-HDR` | Shared typed uniform reader for `glGetUniform*` and `glGetnUniform*`. | Public, GL-layer helper |
| `include/tglmt/ObjectRegistry.h` | 32 | `L0-HDR` | Name-space generator for the 12 GL object kinds (`Gen/Create/Delete/Is`). | Public (core) |
| `include/tglmt/ErrorTracker.h` | 24 | `L0-HDR` | Per-context FIFO error queue + `KHR_no_error` bypass switch. | Public (core) |
| `include/tglmt/GLAppleDraw.h` | 20 | `L0-HDR` | Declarations of the deferred draw path (`AppleDrawGL`, `AppleClearNow`, `AppleClearBufferImage`, `DumpProgramMSL`). | Public, GL-layer helper |
| `include/tglmt/limits.h` | 18 | `L0-HDR` | The two implementation limits shared between `glGet*` and validation (`kMaxCombinedTextureImageUnits`, `kMaxImageUnits`). | Public, GL-layer helper |

### 1.1 Sub-totals for `include/tglmt/`

| Metric | Value |
|---|---|
| Files | 18 |
| LOC | 6 947 |
| Generated | 3 files (`gl46_types.h`, `gl46_pnames.h`, `gl46.h`) = 3 580 LOC |
| Hand-written | 15 files = 3 367 LOC |

---

## 2. `src/` — implementation (48 files, 47 248 LOC)

### 2.1 `src/core/` — context + state core (5 files, 969 LOC)

| Path | LOC | Layer | Responsibility (one line) | Public header status |
|---|---:|---|---|---|
| `src/core/Context.cpp` | 602 | `L1-CTX` | Implements `Context::Current()/MakeCurrent()` (thread-local + global fallback), options loader, encoder flushing, ring allocator, staging flush, wrap-target cache. | n/a (impl) |
| `src/core/StateTracker.cpp` | 150 | `L1-CTX` | Implements the state machine mutators (blend/depth/stencil/cull/viewport/scissor/pixel-store + shadow store). | n/a (impl) |
| `src/core/Renderer.cpp` | 130 | `L1-CTX` | Implements the `Renderer` façade: owns a `Context`, creates the default render target, `BeginFrame/EndFrame` (present), `ReadPixels`. | n/a (impl) |
| `src/core/ErrorTracker.cpp` | 61 | `L1-CTX` | Implements the FIFO error queue (pop on `glGetError`) and `KHR_no_error` bypass. | n/a (impl) |
| `src/core/ObjectRegistry.cpp` | 26 | `L1-CTX` | Implements the monotonic id generator shared by all 12 object kinds. | n/a (impl) |

### 2.2 `src/gl/` — OpenGL entry points + shader translation (41 files, 42 706 LOC)

Split: 36 hand-written `L2-API` files (24 435 LOC) + 4 generated `L2-GEN` files (5 644 LOC) + 1 `L3-SH` shader file (12 627 LOC) = 41 files / 42 706 LOC.

| Path | LOC | Layer | Responsibility (one line) | Public header status |
|---|---:|---|---|---|
| `src/gl/glsl_to_msl.cpp` | 12627 | `L3-SH` | The GLSL→MSL translator: a token/string rewriter with ~60 rewrite passes, **not** a compiler front end. | n/a (impl) |
| `src/gl/gl_apple_draw.cpp` | 4649 | `L2-API` | Deferred GL→Metal draw path: VAO→vertex descriptor, pipeline-key cache, encoder reuse, buffer/texture staging, uniform ring upload. | n/a (impl) |
| `src/gl/gl_texture.cpp` | 3506 | `L2-API` | Texture objects: `MTLTextureDescriptor` + `replaceRegion`, pixel-store alignment, mip chains, compressed formats. | n/a (impl) |
| `src/gl/gl_program.cpp` | 1795 | `L2-API` | Program objects: GLSL→MSL at compile, `newLibraryWithSource` + `newRenderPipelineState` at link, uniform/attrib reflection. | n/a (impl) |
| `src/gl/gl_c_exports.cpp` | 3533 | `L2-GEN` | **Generated** `extern "C"` 1:1 forwarders (701 slots) so LWJGL `dlsym("glXxx")` resolves. | n/a (generated impl) |
| `src/gl/gl_get_extra.cpp` | 1427 | `L2-API` | Object getters (texture/sampler/FBO/RBO/VAO/sync/program resources) that read back from GPU when the CPU shadow is stale. | n/a (impl) |
| `src/gl/gl_uniform.cpp` | 2083 | `L2-API` | Uniforms/UBO/SSBO: CPU shadow + std140 packing + upload into Metal buffers at draw time. | n/a (impl) |
| `src/gl/gl_ext_aliases_gen.cpp` | 1391 | `L2-GEN` | **Generated** `extern "C"` aliases for extension entry points that are 1:1 with an already-implemented core function. | n/a (generated impl) |
| `src/gl/gl_framebuffer.cpp` | 1217 | `L2-API` | FBO/RBO objects → `MTLRenderPassDescriptor` (colour attachments, depth/stencil, load/store actions). | n/a (impl) |
| `src/gl/gl_get.cpp` | 1108 | `L2-API` | All `glGet*` state getters, served from the CPU shadow; owns the `IsAcceptedPname` → `INVALID_ENUM` rule. | n/a (impl) |
| `src/gl/gl_buffer.cpp` | 1030 | `L2-API` | Buffer objects: `MTLBuffer` + CPU shadow, `glBufferStorage` immutability, map/unmap, orphaning. | n/a (impl) |
| `src/gl/gl_dispatch_gen.cpp` | 714 | `L2-GEN` | **Generated** name→introduced-version manifest (core + compat-only) used only by audit tooling; **no runtime consumer**. | n/a (generated impl) |
| `src/gl/gl_draw.cpp` | 596 | `L2-API` | Draw entry points: mode/type/count validation, primitive-restart splitting, fan expansion, then hand-off to `AppleDrawGL`. | n/a (impl) |
| `src/gl/gl_readpixels_dsa.cpp` | 644 | `L2-API` | `glReadPixels` readback (FBO→blit/getTexture) plus the DSA vertex/attrib/transform-feedback-varyings entries. | n/a (impl) |
| `src/gl/gl_ext_dsa_state.cpp` | 523 | `L2-API` | `EXT_direct_state_access` state entries — **including the only matrix code in the tree** (`glMatrix*EXT`, i.e. *named* matrices, not the legacy global matrix stack). | n/a (impl) |
| `src/gl/gl_sync_query.cpp` | 404 | `L2-API` | Sync objects (`MTLSharedEvent`) and queries (timer / occlusion / pipeline-statistics). | n/a (impl) |
| `src/gl/gl_ext_dsa_texture.cpp` | 391 | `L2-API` | `EXT_direct_state_access` texture entries. | n/a (impl) |
| `src/gl/gl_vertex.cpp` | 389 | `L2-API` | VAO + vertex attributes → `MTLVertexDescriptor` + `setVertexBuffer`; `ARB_vertex_attrib_binding` format/binding path. | n/a (impl) |
| `src/gl/glfw_shim.cpp` | 435 | `L2-API` | GLFW shim implementation: window, `MakeCurrent` (thread-local), `SwapBuffers` (present), and the `GetProcAddress` table. | n/a (impl) |
| `src/gl/gl_error_string_debug.cpp` | 341 | `L2-API` | `glGetError`, `glGetString`/`glGetStringi`, the advertised-extension list `kExts[]`, and the debug callback. | n/a (impl) |
| `src/gl/gl_ext_imaging.cpp` | 307 | `L2-API` | `ARB_imaging` colour-table / convolution / histogram **state** (state-only, not the legacy GL 1.x pixel pipeline). | n/a (impl) |
| `src/gl/gl_tess_xfb.cpp` | 291 | `L2-API` | Tessellation (Metal `drawPatches`) + transform-feedback emulation (capture buffers, begin/pause/resume/end). | n/a (impl) |
| `src/gl/gl_raster.cpp` | 275 | `L2-API` | Raster state: blend, depth/stencil, cull, viewport, scissor, polygon offset/clamp, clip control, line/point. | n/a (impl) |
| `src/gl/gl_ext_path_command.cpp` | 255 | `L2-API` | `NV_path_rendering` / `NV_command_list` object lifecycle + parameters; rendering entry points fail explicitly. | n/a (impl) |
| `src/gl/gl_ext_bindless.cpp` | 236 | `L2-API` | ARB/NV bindless texture handle bookkeeping + shader buffer load. | n/a (impl) |
| `src/gl/gl_ext_state.cpp` | 230 | `L2-API` | Stateful entries for small glcore extensions. | n/a (impl) |
| `src/gl/gl_compute.cpp` | 489 | `L2-API` | Compute dispatch → `MTLComputeCommandEncoder.dispatchThreads` + memory barrier. | n/a (impl) |
| `src/gl/gl_program_interface_p10.cpp` | 224 | `L2-API` | `ARB_program_interface_query` introspection (uniforms, blocks, program inputs/outputs). | n/a (impl) |
| `src/gl/gl_debug_p4.cpp` | 220 | `L2-API` | `KHR_debug` + `EXT_debug_marker` + `EXT_discard_framebuffer` + `ARB_parallel_shader_compile`. | n/a (impl) |
| `src/gl/gl_ext_perf.cpp` | 172 | `L2-API` | AMD/INTEL performance-monitor object bookkeeping (not advertised). | n/a (impl) |
| `src/gl/gl_ext_misc.cpp` | 155 | `L2-API` | Small unsupported/stateful glcore extension entries. | n/a (impl) |
| `src/gl/gl_ext_include_shading.cpp` | 151 | `L2-API` | `ARB_shading_language_include` + NV/EXT fragment shading-rate entries. | n/a (impl) |
| `src/gl/gl_ext_attrib64.cpp` | 127 | `L2-API` | `NV_vertex_attrib_integer_64bit` current-value storage. | n/a (impl) |
| `src/gl/gl_ext_uniform64.cpp` | 105 | `L2-API` | `ARB_gpu_shader_int64` / `NV_gpu_shader5` uniform entry points. | n/a (impl) |
| `src/gl/gl_shader.cpp` | 96 | `L2-API` | Shader objects: calls `ConvertGLSLtoMSL` at `glCompileShader`; fails loudly outside the supported subset. | n/a (impl) |
| `src/gl/gl_shader_objects_p8.cpp` | 174 | `L2-API` | `ARB_shader_objects` entry points forwarding to the core shader/program functions. | n/a (impl) |
| `src/gl/gl_sampler.cpp` | 75 | `L2-API` | Sampler objects → `MTLSamplerDescriptor` / `MTLSamplerState` (+ anisotropy). | n/a (impl) |
| `src/gl/gl_multisample_p8.cpp` | 65 | `L2-API` | `EXT_multisample` / multisample-render-to-texture state (API present, **not advertised**). | n/a (impl) |
| `src/gl/gl_packed_robust.cpp` | 206 | `L2-API` | The 42 compatibility-only entry points: 30 packed-attribute `gl*P*ui(v)` + 12 `glGetn*` robustness getters. | n/a (impl) |
| `src/gl/gl_draw_buffers2_p8.cpp` | 44 | `L2-API` | `EXT_draw_buffers2` indexed per-target colour mask / enable / query. | n/a (impl) |
| `src/gl/gl_stubs_gen.cpp` | 6 | `L2-GEN` | **Generated** honest stubs for any catalogued entry point not hand-implemented — currently **empty** (only the file banner). | n/a (generated impl) |

### 2.3 `src/metal/` — backends (2 files, 3 573 LOC)

| Path | LOC | Layer | Responsibility (one line) | Public header status |
|---|---:|---|---|---|
| `src/metal/AppleMetalBridge.mm` | 3494 | `L5-BE` | Objective-C++/ARC bridge onto `<Metal/Metal.h>`: every `IDevice`/`IEncoder` virtual implemented against `id<MTL*>`; owns the bridge-level pipeline cache. | n/a (impl) |
| `src/metal/NullMetalBackend.cpp` | 79 | `L5-BE` | CPU shadow backend: `NullBuffer/NullTexture/NullEncoder/NullDevice` + `CreateDevice()` factory that falls back to null for unknown backend names. | n/a (impl) |

### 2.4 Grand totals

| Directory | Files | LOC | % of LOC |
|---|---:|---:|---:|
| `include/tglmt/` | 18 | 6 947 | 12.8 % |
| `src/core/` | 5 | 969 | 1.8 % |
| `src/gl/` | 41 | 42 706 | 78.8 % |
| `src/metal/` | 2 | 3 573 | 6.6 % |
| **Total** | **66** | **54 195** | 100 % |

Largest single file: `src/gl/glsl_to_msl.cpp` (12 627 LOC, 23 % of the tree).
Second: `src/gl/gl_apple_draw.cpp` (4 649 LOC, 8.6 %).
Smallest: `src/gl/gl_stubs_gen.cpp` (6 LOC, generated and empty).

---

## 3. Tests (185 CTest targets + helpers)

CMake registration: `CMakeLists.txt:107-109` globs
`tests/unit/*.cpp tests/integration/*.cpp tests/regression/*.cpp tests/api/*.cpp tests/conformance/*.cpp tests/shaders/*.cpp`;
`:119-133` creates one executable per file, links `tglmt`, reuses one shared PCH
(`:116-118`, `tglmt_test_pch` over `tests/pch_stub.cpp`) and registers
`add_test(NAME ${NAME} COMMAND ${NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})`.
`:136-137` adds `truth_probe` as an executable that is **not** a CTest gate.

| Directory | `.cpp` files | LOC | Registered with ctest | Purpose |
|---|---:|---:|---|---|
| `tests/unit/` | 92 | 13 074 | yes | CPU-only tests (null backend, converter, state machine, catalog contracts). Never needs a GPU. |
| `tests/integration/` | 77 | 11 489 | yes | Real-Metal / pixel-level tests; they print `SKIP` and exit 0 when no GPU is present. |
| `tests/regression/` | 13 | 5 339 | yes | Defect-regression suite (core buffer/draw/state/program/query, catalog, gl_errors, pipeline cache, shader semantics, uniform transpose). |
| `tests/api/` | 1 | 132 | yes | API-surface contract: core count = 657, compat-only = 42, every core symbol exported by `libtglmt.dylib`, `GL_VERSION`/`GL_CONTEXT_PROFILE_MASK`, `GL_NUM_EXTENSIONS` vs `glGetStringi`, pname validity. |
| `tests/conformance/` | 1 | 163 | yes | Conformance surface vs the vendored Khronos registry. |
| `tests/shaders/` | 1 (+7 GLSL) | 123 | yes | GLSL corpus converted **and** compiled by the real Metal compiler. GLSL inputs: `basic.vert/frag`, `flat_varying.vert/frag`, `compute.comp`, `query_lod.frag`, `single_output_loc1.frag`. |
| `tests/truth/` | 1 (+1 JSON) | 142 | **no** (`truth_probe`, `CMakeLists.txt:136`) | Runtime probe for known static gaps + machine-readable inventory `api_manifest.json` (written by `tools/truth_manifest.py`). |
| `tests/corpus/` | 0 (89 files) | — | — | Vendored Minecraft 26.1.2 shader corpus (`core/ include/ post/`) fetched by `tests/corpus/fetch-corpus.sh`; path injected as `TGLMT_CORPUS_DIR` (`CMakeLists.txt:126`). |
| `tests/test_util.h`, `tests/pch_stub.cpp` | 2 | 67 | n/a | Shared assertion macros (`TGLMT_CHECK`/`TGLMT_RESULT`) and the PCH translation unit. |
| **Total** | **185** | **30 462** | | 185 CTest targets. |

Benchmarks (not CTest gates, `CMakeLists.txt:142-159`):

| Path | LOC | Purpose |
|---|---:|---|
| `benchmarks/compilation/bench_compile.cpp` | 208 | GLSL→MSL compile-time benchmark. |
| `benchmarks/compilation/bench_upload.mm` | 132 | Raw Metal API upload benchmark (Objective-C++/ARC). |
| `benchmarks/runtime/bench_runtime.cpp` | 240 | Runtime GL-call benchmark. |
| `benchmarks/compare/bench_compare.py` | 185 | Baseline vs optimized comparison driver. |
| `benchmarks/compare/bench_median.py` | 98 | Median/p95 post-processing. |

Application (`apps/aquarium/`, standalone CMake project that links the *built* library, `apps/aquarium/CMakeLists.txt:18-30`):

| Path | LOC | Purpose |
|---|---:|---|
| `apps/aquarium/device_tests.cpp` | 1598 | On-device test suite driven from the app. |
| `apps/aquarium/aquarium.cpp` | 759 | The benchmark scene (boids, water, sand, plants, bubbles, HUD) written in plain OpenGL. |
| `apps/aquarium/ios_shell.mm` | 454 | UIKit shell + `CAMetalLayer` present. |
| `apps/aquarium/macos_shell.mm` | 167 | AppKit shell + `CAMetalLayer` present. |
| `apps/aquarium/aquarium.h` | 34 | Scene header. |
| `apps/aquarium/device_tests.h` | 8 | Device-test header. |

---

## 4. Files that do **not** exist (verified absent)

| Expected subsystem | Status | Verification |
|---|---|---|
| `src/compat/`, `src/legacy/`, `src/fixed_function/` | **absent** | `find src -type d` → only `src/core`, `src/gl`, `src/metal`. |
| A legacy fixed-function engine (immediate mode, matrix stack, lighting, fog, texenv/texgen, display lists, evaluators, selection/feedback, accumulation, pixel ops) | **absent** | See `docs/REPO_DISCOVERY.md` §2.4 for the per-symbol grep evidence. |
| A separate internal-IR module | **absent as a module** | The "IR" in this tree is a *naming convention* for the deferred-encoder + pipeline-key cache inside `src/gl/gl_apple_draw.cpp` (`gl_apple_draw.cpp:2352`, `:2409`, `:2614`, `:2817`, `:3719`). There is no IR type/module. |
| A separate validation module | **absent as a module** | Validation is inlined in each entry point; the only shared artefact is the generated pname whitelist `include/tglmt/gl46_pnames.h`. |
| GLU implementation | **absent** | No `glu*` symbol anywhere in `src/` or `include/`. |
| `install()` / export rules for an SDK | **absent** | `grep -n "install" CMakeLists.txt apps/aquarium/CMakeLists.txt` → no match. |
