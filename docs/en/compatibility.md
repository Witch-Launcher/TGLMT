# Compatibility & Performance

This page states what TGLMT does, what it emulates, what it only records, and what it
does not do — plus the counters and commands you can use to verify any of those claims
against the current tree.

## How to read a capability claim

Each row below carries one of five classes. They are deliberately distinct; "supported"
alone would hide the difference between real GPU work and a CPU fallback.

| Class | Meaning |
|---|---|
| **GPU** | the GL call is validated, state is tracked, and Metal work is encoded on the Apple backend |
| **CPU emulated** | the GL call produces a correct result, computed on the CPU rather than by the GPU |
| **Trace only** | the call is accepted and recorded, but no GPU work results; observable through counters or on the Null backend |
| **No-op accepted** | the call is consumed without effect and without raising a GL error |
| **Rejected** | the call reports a GL error, a compile failure, or a link failure with an explicit message |

## Raster and framebuffer functionality

| Feature | Class | Notes |
|---|---|---|
| Depth test, depth write, depth function | GPU | encoded as `MTLDepthStencilState`, cached (16 states) |
| Stencil test and stencil ops | Trace only | state is tracked and returned by `glGet*`; the depth-stencil descriptor carries depth only, so stencil never reaches the GPU |
| Blending, blend colour, all 15 factors and 5 equations | GPU | attachment 0 on the draw path |
| `glColorMask` / `glColorMaski` | GPU | baked into the pipeline key as `colorWriteMask` |
| Face culling, front-face orientation | GPU | verified by a dedicated orientation test |
| Scissor test | GPU | scissor rectangle is passed through unmodified |
| Viewport, multiple viewports | GPU | converted for GL's lower-left origin |
| `glClipControl` | GPU | affects both viewport depth conversion and the appended shader Z conversion |
| Polygon mode (fill / line) | GPU | mapped to `setTriangleFillModeLines` |
| Polygon offset (`glPolygonOffset`, `glPolygonOffsetClamp`) | Trace only | tracked and it dirties the pipeline key, but no depth bias is passed to Metal |
| Line width | Trace only | recorded in the state shadow; `MTLRenderCommandEncoder` has no line-width control and Metal lines are always 1 pixel |
| `glLogicOp` | Trace only | stored in the shadow; there is no logic-op stage in the render path |
| Primitive restart | GPU | fixed and custom restart indices split the draw correctly |
| MSAA via the `metal::` interface | GPU | `makeMSAATarget`, `makeMSAAPipeline`, resolve — verified by test |
| MSAA via the GL API (`glRenderbufferStorageMultisample`, `glTexImage2DMultisample`, `glTexStorage*Multisample`) | No-op accepted | no multisample storage is allocated |
| Renderbuffers (`glRenderbufferStorage*`, `glFramebufferRenderbuffer`) | No-op accepted | FBOs must attach textures with `glFramebufferTexture2D` |
| `glInvalidateFramebuffer` and friends | No-op accepted | explicitly documented as not implemented |
| `glBlitFramebuffer` | GPU, CPU fallback | GPU blit when formats match; CPU copy when scaling, filtering, or format differs |
| FBO completeness, attachment with mip level | GPU | target size is `base >> level` |
| `glClear` | GPU | immediate clear, or deferred to the next draw as `loadAction = Clear` |
| `glClearTexImage` / `glClearTexSubImage` | No-op accepted | |

## Texture and buffer functionality

| Feature | Class | Notes |
|---|---|---|
| `glTexImage2D` / `glTexSubImage2D` | GPU | staged, coalesced, flushed as one `replaceRegion` |
| Mip levels greater than 0 | CPU emulated | level 0 stays on the GPU; higher levels are written to the shadow only (`mipStaged`) |
| `glGenerateMipmap` | GPU | uses the Metal mip-generation path; requires a mipmapped texture |
| Mip filtering on a single-level texture | CPU emulated | falls back to the base level and counts `mipBase` |
| Drawing or clearing into mip level greater than 0 of a single-level texture | Trace only | skipped and counted as `mipLevelSkipped` |
| Cubemaps, `samplerCube` | GPU | faces are uploaded immediately rather than staged |
| `sampler2DArray` | GPU | mapped to `texture2d_array` |
| `glTexBuffer` / `samplerBuffer` | CPU emulated, GPU read | `glTexBuffer` materialises the buffer into a texture when it is bound (`BindBufTex`); the shader then reads it with `.read()` on the GPU |
| Compressed texture upload (`glCompressedTexImage*`) | No-op accepted | no decompression, no error |
| `glCopyTexSubImage2D` / `glCopyTextureSubImage2D` | GPU | read framebuffer → texture copy, then the shadow is refreshed from the GPU |
| `glCopyTexImage*`, `glCopyTexSubImage1D`, `glCopyTexSubImage3D` | No-op accepted | |
| `glPixelStorei` pack/unpack row length, skip, alignment | CPU emulated | exact spec enums; applied at upload and readback |
| PBO unpack and pack as byte offsets | CPU emulated | `GL_PIXEL_UNPACK_BUFFER` / `GL_PIXEL_PACK_BUFFER` offsets are honoured |
| `glBufferSubData` | CPU emulated | staged and merged; flushed conditionally |
| `glMapBufferRange` / `glUnmapBuffer` | GPU, shared memory | staged ranges are flushed first, then the pointer addresses the shared `MTLBuffer` directly; `glUnmapBuffer` calls `didModifyRange` and copies the region back into the shadow so later reads see it |
| Named buffers (DSA) | GPU | named-buffer updates and maps synchronise with the GPU so uniforms are not stale |
| Ring-buffer upload of uniforms, indices and temporary data | GPU | three 4 MB shared buffers rotated per frame; zero `MTLBuffer` allocations per frame under budget |
| `glGetTexImage`, `glGetTextureSubImage`, `glReadPixels` | CPU emulated | level 0 is flushed from the GPU and returned in GL order; mip levels greater than 0 are read from the shadow. Only `GL_RGBA`/`GL_BGRA` with `GL_UNSIGNED_BYTE` are accepted — anything else raises `GL_INVALID_OPERATION` (`glGetTexImage` additionally accepts the packed `GL_RGBA8`/`GL_RGBA8_REV` types) |

## Draw functionality

| Feature | Class | Notes |
|---|---|---|
| `glDrawArrays`, `glDrawElements`, `glDrawRangeElements` | GPU | including `idx == nullptr` meaning offset 0 into the bound element buffer |
| Instancing | GPU | `glDraw*Instanced` |
| `baseVertex` on an indexed draw | CPU emulated | Metal indexed draws have no `baseVertex`, so each index is rewritten with `baseVertex` added into a temporary ring buffer; a negative or overflowing resulting index raises `GL_INVALID_OPERATION` |
| `baseVertex != 0` on a non-indexed draw | Trace only | the draw path returns before encoding |
| `baseInstance != 0` | Trace only | rejected with an explicit log |
| Indirect draws | CPU emulated | the command is read from `GL_DRAW_INDIRECT_BUFFER` (or from the host pointer) on the CPU, then issued as an ordinary draw; `glMultiDraw*Indirect` loops over that |
| `glMultiDraw*` | CPU emulated | expanded into sub-draws that share one open encoder; each sub-draw increments `multidrawBatched` (tracked in the context, not printed in the frame summary) |
| `GL_TRIANGLE_FAN`, `GL_LINE_LOOP` | CPU emulated | expanded to triangles / closed strips on the CPU; combined with `baseVertex != 0` these become trace-only |
| `GL_PATCHES` | Trace only | the draw path returns before encoding |
| Transform feedback | Trace only | object creation, binding, begin/end, pause/resume, buffer binding, varyings query and active/paused queries are implemented; nothing is captured because no program executes, and `glGetTransformFeedbackVarying` reports type `GL_FLOAT` and size 1 regardless of the real declaration |
| Geometry shader draws | Trace only | programs with a geometry stage link, then the draw path declines to encode |
| Tessellation draws | Trace only | as above for tessellation stages |
| Compute dispatch | No-op accepted | `glDispatchCompute` logs and returns; no compute encoder is created from the GL path |

## Shader converter boundaries

Translation happens in `src/gl/glsl_to_msl.cpp` at `glCompileShader`. The converter is
strict: anything it does not understand produces `ok = false` and a message, which
`glCompileShader` turns into `COMPILE_STATUS = false` with `"error: "` prefixed.

### Accepted

| Area | Supported |
|---|---|
| Stages | vertex and fragment only |
| Versions | `100`, `120`, `130`, `140`, `150`, `300` (including `300 es`), `330`, `400`, `410`, `440`, `450`, `460` |
| Preprocessor | `#version`, object-like `#define` / `#undef`, `#ifdef` / `#ifndef` / `#if` / `#elif` / `#else` / `#endif` with `defined()`, integer literals and numeric single-token macros; `#line`, `#pragma`, `#extension`, `precision` are discarded |
| Declarations | `in` / `out` / `uniform`, with or without `layout(location=N)`; multiple declarators per statement; interpolation and precision qualifiers parsed and dropped |
| Types | `float`, `vec2`–`vec4`, `mat2`–`mat4`, `int`, `ivec2`–`ivec4`, `uint`, `uvec2`–`uvec4`, `bool`; `struct`; arrays on uniforms, uniform-block members (size 1–1024), and `const` arrays written as `T[] name = T[](...)` |
| Uniform blocks | `layout(std140) uniform Block { … }` with optional `layout(binding=N)`; offsets computed by the converter; reported through `GL_ACTIVE_UNIFORM_BLOCKS` |
| Samplers | `sampler2D`, `samplerCube`, `sampler2DArray`, `samplerBuffer` (+ `i`/`u` variants), `sampler2DShadow` |
| Texture built-ins | `texture` (with optional bias), `texelFetch`, `textureGrad`, `textureLod`, `textureProj` |
| Built-in variables | `gl_Position`, `gl_PointSize`, `gl_PointCoord`, `gl_FragCoord`, `gl_FrontFacing`, `gl_VertexID`, `gl_InstanceID`, `gl_FragColor`, `gl_FragData[0]` |
| Fragment outputs | 1 to 8 `vec4` outputs, explicit or assigned locations |
| Other built-ins | `discard` becomes `discard_fragment()`; `dFdx` / `dFdy` become `dfdx` / `dfdy`; `radians` / `degrees` become helper functions because MSL has neither |
| Control flow | function and `struct` bodies are passed through whole, with GLSL type names rewritten to their MSL equivalents; `double` / `dvec*` anywhere is an error |

### Rejected

| Construct | Reason |
|---|---|
| Tessellation control, tessellation evaluation, geometry, compute stages | compile fails with an explicit stage name; these have no GLSL translation path |
| Unknown or missing `#version` | explicit message |
| Function-like macros, unparseable `#if`, stray `#elif` / `#else` / `#endif`, other directives | explicit message |
| Types outside the list above, including `double`, `dvec*`, `mat2x3` and friends, `isampler2D`, `usampler2D`, `sampler1D`, `sampler2DMS` | explicit message |
| Arrays on `in` / `out` declarations | explicit message |
| Unsized array declarators (`float a[];`) and sized `const` arrays (`const float a[4] = …`) | explicit message; only `T[] name = T[](...)` is accepted for `const` arrays |
| Uniform-block instances declared as arrays (`} inst[4];`) or containing struct members | the block parse returns false, which surfaces as a generic declaration error |
| Matrix-typed vertex attributes | Metal attributes are at most `float4` |
| `gl_FragData[N]` for `N > 0`, zero or more than 8 fragment outputs, non-`vec4` outputs, duplicate output locations | explicit message |
| `layout(binding=N)` on a plain (non-block) uniform | explicit message |
| Uniform block redeclared with different members | explicit message |
| `layout(push_constant)` / other non-`location` layouts on plain declarations, `shared` variables | explicit message |
| `buffer { … }` (SSBO) blocks | not recognised: they are passed through to MSL and surface as an MSL compile error on a real device, not as a converter error |
| Subroutines | not parsed |
| Declarations that appear after a function body | explicit message |

### Approximated rather than rejected

These compile successfully but are not equivalent to the OpenGL behaviour:

| Construct | Approximation |
|---|---|
| `sampler2DShadow` | mapped to an ordinary `texture2d<float>`; the compare reference is dropped |
| `textureGrad`, `textureLod`, `textureProj` | restricted to `sampler2D`; other sampler types are rejected |
| Default-block uniform layout | laid out with MSL natural alignment, not strict std140 |
| `flat` / `smooth` / `noperspective` | parsed and discarded; flatness is inferred from the type |
| Unknown `gl_*` identifiers | passed through verbatim into MSL, so a genuinely unrewritten built-in surfaces as an MSL compile error rather than a converter error |

### Failure tiers

This distinction matters when reading a test result:

1. **Converter rejects** — `glCompileShader` reports failure with an explicit message on
   any backend.
2. **MSL compile fails** — the converter succeeded, but Metal rejects the generated MSL.
   This is only detected when a real device is present, because MSL compilation is skipped
   on the Null backend. A Null-backend build can therefore link a program that would fail
   on the Apple backend.
3. **Silently approximated** — the rows in the previous table.

There is no SPIR-V in the runtime path. `glSpecializeShader` simply re-runs the GLSL
conversion. SPIR-V appears only as an offline verification oracle:
`scripts/verify_spirv_chain.sh` and `tests/integration/test_converter_proof.cpp` compare
TGLMT's output against `glslangValidator` → `spirv-cross` → `metal` on the same input.

### P1 spike: glslang + SPIRV-Cross (deferred)

A spike was run to decide whether to replace the hand-written converter
(`glsl_to_msl.cpp`) with `glslang` (GLSL→SPIR-V) + `SPIRV-Cross` (SPIR-V→MSL).

**Finding (measured on device, build `p0-honesty`, iPhone 8 Plus / A11):**

| Stage | Time | Note |
|---|---|---|
| `glCompileShader` (hand-written converter) | **1.05 ms/shader** | 105 ms / 100 calls |
| `mslLib` (Metal `newLibraryWithSource`, MSL→AIR) | **73–106 ms/shader** | cache miss; cache hit = 0 ms |
| `glLinkProgram` (total) | **53 ms/link** | 4935 ms / 93 calls |

The Metal MSL→AIR compile dominates and is **independent of how the MSL was
produced** (hand-written converter vs SPIRV-Cross). Replacing the converter would
therefore save ≈1 ms/shader — negligible.

**Cost:** `glslang` ≈1.6 MB + `SPIRV-Cross` ≈0.4 MB static → `libtglmt.dylib`
grows ≈1.29 MB → ≈3.3 MB (under the 6 MB budget, but not free).

**Decision (2026-10-12):** defer. The hand-written converter is fast and correct on
the vanilla/Minecraft corpus. glslang + SPIRV-Cross becomes worth the +2 MB when
P3–P5 needs geometry / tessellation / compute translation, or when a real shader
fails the subset. The hand-written path stays the runtime default; the offline
`verify_spirv_chain.sh` oracle already covers correctness checking.

## Metal architectural mismatches

These are properties of Metal itself, not gaps in a code path.

| OpenGL concept | Metal reality | TGLMT behaviour |
|---|---|---|
| Geometry shaders | none | a hand-written mesh-shader path exists (`shaders/gs_mesh.metal`, `makeMeshPipeline`, `drawMesh`) but is reachable only from native Metal code and tests, gated on `supportsMesh()` (Apple7 and newer). GLSL geometry shaders do not compile. |
| Tessellation stages | no GL-style TCS/TES | a hand-written path exists (`shaders/tess_triangle.metal`, `makeTessPipeline`, `drawPatches`) reachable only from native Metal code and tests. GLSL tessellation shaders do not compile, and `GL_PATCHES` draws are not encoded. |
| Compute shaders with `image` load/store | supported by Metal | not reachable from the GL path; `glDispatchCompute` logs only |
| Transform feedback | none | state plumbing only, no capture |
| Logic operations | none in the render path | shadow only |
| Stencil | fully supported by Metal | tracked, but not encoded |
| Read/write hazard within one render pass | not tracked automatically | TGLMT pre-scans each draw and splits the pass (`hazardSplits`) when a program samples the texture it renders into |
| Render command buffer with mixed load/store semantics | explicit | one encoder stays open across draws of the same pass; it ends on any pass-identity change |

## Device and platform constraints

| Constraint | Value |
|---|---|
| iOS architecture | `arm64` only |
| Deployment target | `16.0` by default (`DEPLOY_TARGET`, `IOS_DEPLOYMENT_TARGET`) |
| Design baseline | iPhone A11 |
| `MAX_SAMPLES` reported | 4 |
| Depth-stencil format on iOS | `Depth24Stencil8` requests are mapped to `Depth32Float_Stencil8` |
| Layer pixel format | must be `MTLPixelFormatRGBA8Unorm` |
| Extension string | fixed at six entries: `GL_ARB_gl_spirv`, `GL_ARB_indirect_parameters`, `GL_ARB_pipeline_statistics_query`, `GL_ARB_polygon_offset_clamp`, `GL_ARB_texture_filter_anisotropic`, `GL_KHR_no_error` |

The extension list is intentionally frozen. LWJGL builds its capability set from
`glGetString(GL_EXTENSIONS)`, so advertising an extension whose path is not exercised end
to end causes the application to take that path. Adding untested entries previously made
Minecraft hang shortly after startup.

### Reported limits

`glGetIntegerv` answers from a static table rather than from the GPU. The values are the
A11 baseline and are chosen to be at least the OpenGL 4.6 minimums:

| Pname | Value |
|---|---|
| `MAJOR_VERSION` / `MINOR_VERSION` | 4 / 6 |
| `MAX_TEXTURE_SIZE`, `MAX_CUBE_MAP_TEXTURE_SIZE`, `MAX_RENDERBUFFER_SIZE` | 8192 |
| `MAX_3D_TEXTURE_SIZE`, `MAX_ARRAY_TEXTURE_LAYERS` | 2048 |
| `MAX_COLOR_ATTACHMENTS`, `MAX_DRAW_BUFFERS` | 8 |
| `MAX_VERTEX_ATTRIBS` | 16 |
| `MAX_TEXTURE_IMAGE_UNITS` / `MAX_VERTEX_TEXTURE_IMAGE_UNITS` / `MAX_COMBINED_TEXTURE_IMAGE_UNITS` | 32 / 16 / 48 |
| `MAX_UNIFORM_BUFFER_BINDINGS` | 36 |
| `MAX_UNIFORM_BLOCK_SIZE` | 65536 |
| `UNIFORM_BUFFER_OFFSET_ALIGNMENT` | 256 |
| `MAX_VERTEX_UNIFORM_BLOCKS` / `MAX_FRAGMENT_UNIFORM_BLOCKS` / `MAX_COMBINED_UNIFORM_BLOCKS` | 14 / 14 / 28 |
| `MAX_SAMPLES` | 4 |
| `MAX_ELEMENTS_VERTICES` / `MAX_ELEMENTS_INDICES` | 1048576 / 1048576 |

Every `glGet*` call reads the CPU shadow; none round-trips to the GPU.

## Deferred execution and batching

The design is described in detail in
[Architecture](architecture.md#deferred-execution). The performance-relevant summary:

- One render command encoder stays open across consecutive draws of the same pass.
- Buffer and texture uploads are staged and coalesced, and only flush the encoder when the
  resource was already used by the open pass.
- Uniforms are compared against a cache and only changed stages are re-uploaded.
- Pipeline state objects are resolved through an in-process key cache before the bridge is
  called.
- Temporary uploads come from a three-slot, three-frame ring allocator.
- The frame boundary flushes everything but does not wait for the GPU; it waits only when
  in-flight command buffers exceed `kMaxFramesInFlight` (2).

Correctness gates, in the order they were chosen: behaviour first, batching second. A
forced `CommitAndWait` (`bufWaitStall`, `texWaitStall`) is always preferred over writing
memory the GPU is still reading.

## Counters

Two lines are printed to stderr for the first 12 swaps and then every 600th swap.

### `[TGLMT] build=…`

| Field | Reading |
|---|---|
| `frame`, `dt` | swap index and time since the previous swap |
| `dAtt` / `dEnc` | draws attempted versus draws encoded in this window. `dEnc` at zero with `dAtt` above zero means the draw path is rejecting work. |
| `att` / `enc` | the same two counters since process start |
| `progs` | distinct programs the draw path has encoded with |
| `noProg`, `noTgt`, `noPipe`, `misc` | which rejection class is responsible |
| `clears`, `blits`, `copyTex` | side-effect call volumes |
| `mipBase` | fragment shader still sampling level 0 because only level 0 exists (`mipWasBase` path) |
| `range`, `hazard` | a draw read past a vertex/index buffer, or sampled the texture it renders into |
| `uboSmall` / `uboPad` | uniform blocks bound smaller than the struct, and blocks bound at the struct size but needing std140 trailing padding |
| `mipSkip` | drawing or clearing into mip level greater than 0 of a single-level texture |
| `texWasRTFl` | flushes caused by uploading to a texture that is currently a render target |
| `target=0x… WxH` | the active render target; a null pointer or zero size means `BeginFrame` never obtained one |
| `vp=WxH@x,y` | the active viewport; a zero size means nothing can rasterise |
| `center=(r,g,b,a)` | a pixel read back from the middle of the default target |
| `present` | whether the present succeeded |

### `[TGLMT] ir …`

| Field | Reading |
|---|---|
| `encNew` / `encReuse` | encoders created versus draws that reused the open encoder. `encReuse` should be far larger than `encNew`. |
| `pipeRe` / `pipeLk` / `pipeSkip` | pipeline reuse inside a pass, bridge lookups, and lookups skipped by the key cache |
| `uniRe` / `depthRe` / `stSkip` | uniform uploads, depth-state objects and `set*` calls skipped by dirty checking |
| `bufCoal` / `bufFl` / `texCoal` / `texFl` | staging coalescing: merges before flush, and flush counts |
| `bufRot` / `bufWait` / `texWait` / `texWasRT` | in-flight protection events |
| `ringAlloc` / `ringWrap` / `tempAlloc` | ring allocator use and fallbacks; `tempAlloc` above zero means an allocation escaped the ring |
| `thr` | frames where the CPU was blocked waiting for the GPU |
| `cbC` / `cbD` | command buffers committed and completed |
| `clrFail` | deferred-clear region fills that failed |
| `pres` | presents completed |

### Diagnostic gate

Heavy diagnostics are gated by `Context::DiagOn()`:

| Build | Enable with |
|---|---|
| Default | `TGLMT_DIAG=1` in the environment |
| Device where the launcher cannot pass environment variables | configure with `-DTGLMT_FORCE_DIAG=ON` |

Diagnostics skipped while the gate is closed are counted as `diagSkipped`, so a quiet log
is distinguishable from a build that has no diagnostics at all.

Deduplicated diagnostic lines (`fboDraw#`, `atlasprobe#`, `link#`, `gpuswap#`, `uboPad#`,
`sampdeny#`, …) each carry their own cap, so a long run will stop emitting repeats rather
than growing without bound.

## Reproducible verification

### Test suite

```sh
cmake -S . -B build
cmake --build build -j8
ctest --test-dir build --output-on-failure          # Null backend, CI-safe
```

```sh
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON
cmake --build build-apple -j8
ctest --test-dir build-apple --output-on-failure    # real GPU
```

`make test` wraps the first block through `scripts/build_and_test.sh`. Tests that need a
GPU, the Minecraft shader corpus, `glslangValidator`, `spirv-cross` or `xcrun metal` print
`SKIP` and exit 0 when the requirement is missing.

### Behavioural tests worth knowing about

| Test | Proves |
|---|---|
| `test_perf_deferred_apple` | 100 same-state draws produce one encoder, no temporary allocations, correct pixels |
| `test_ir_batch_apple` | redundant binds do not advance `stateSeq`; N draws share one encoder and one pipeline set |
| `test_ir_full_apple` | pipeline-key cache hits, buffer and texture staging coalescing, pixels still correct |
| `test_cull_orientation_apple` | front-face handling: `cull BACK` shows a CCW quad, `cull FRONT` hides it |
| `test_viewport_orientation_apple` | the viewport mirror convention maps NDC rows to the correct memory rows |
| `test_present_flip_apple` | present produces `dst[y] == src[H-1-y]` |
| `test_rop_apple` | all 16 logic operations match an independent copy of the specification table |
| `test_converter_proof` | TGLMT output is pixel-identical to `glslangValidator` → `spirv-cross` → `metal` |
| `test_mc_corpus` | every shader in the Minecraft corpus converts and links |
| `test_readpixels_packed`, `test_pbo_unpack` | packed attribute decode and PBO offsets |

### Static checks

These are quick, offline ways to confirm the counts quoted on this page.

```sh
# OpenGL 4.6 Core surface
grep -c 'introduced GL_VERSION' include/tglmt/gl46.h    # → 698 declarations
grep -c 'gl::gl' src/gl/gl_c_exports.cpp                # → 701 exported entry points
grep -c '^{"gl' src/gl/gl_dispatch_gen.cpp              # → 698 dispatch entries

# Regenerate the GL surface from docs/khronos/gl.xml and see if anything is missing.
python3 tools/gen_gl_headers.py
python3 tools/gen_c_exports.py
python3 tools/gen_stubs.py                              # → "[stubs] missing: 0"

# Verify the offline SPIR-V cross-check.
sh scripts/verify_spirv_chain.sh
```

The three exports beyond the 698 core functions are `glGetPointerv`, `glLineStipple` and
`glDepthRangeArrayfvNV`; they are not in the dispatch table.

`tools/gen_stubs.py` reports functions without a hand-written body. An empty
`src/gl/gl_stubs_gen.cpp` means every entry point has an implementation.

### Per-function mapping

`docs/mapping/GL46_to_Metal.md` is a technical reference table of the per-function
strategy. It is not a coverage claim: it lists a subset of the 698 core functions, and
several rows carry a shared generic strategy rather than a specific mapping. Use the test
suite, not the table, to establish current behaviour.
