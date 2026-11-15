# HARDWARE_LIMITATIONS.md — the honest limit list for TGLMT on Apple A11

> **What this document is.** The list of things TGLMT **cannot** do on an
> Apple A11 (iPhone 8 / 8 Plus / X, iOS 11–16) because of the **hardware or
> the Metal API surface on that hardware** — as opposed to things TGLMT simply
> has not implemented yet.
>
> **The distinction is the point of this document.** `project/AGENTS.md` §6
> requires it, and it changes the work item: a *hardware limit* cannot be
> closed by writing code; a *missing implementation* can. Mixing the two
> produces the two failure modes this project must avoid — claiming a feature
> works when it does not, and writing "impossible on A11" for something that
> was merely never finished.
>
> **Status vocabulary** (`project/AGENTS.md` §6):
> `FULLY_IMPLEMENTED` · `PARTIAL` · `INCORRECT` · `STUB` · `MISSING` ·
> `EMULATED` · `HARDWARE_LIMITED` · `BLOCKED` · `NOT_VERIFIED` ·
> `NOT_APPLICABLE`.
>
> **OPEN / CLOSED:**
> * **CLOSED** = the limit is *emulated* and the emulation is implemented **and
>   tested**. (A hardware limit that has been worked around is closed.)
> * **OPEN** = the limit is known, the remedy is designed or not yet started,
>   and it is **not** covered by a passing test.
>
> **Environment labels:** `A11_DEVICE` · `SIMULATOR` · `OTHER_GPU` ·
> `REFERENCE_ONLY` · `NONE`.
>
> > **No A11 device was available while writing this document.** Every
> > device-dependent statement below is `REFERENCE_ONLY` (Apple documentation)
> > or `NOT_VERIFIED`. **No item below is closed on the strength of a device
> > measurement**, because none was made. Items marked CLOSED are closed
> > *logically* (the mechanism is in the code path and the GL-level behaviour
> > is specified) but carry the note "device test pending" — see §5.
>
> **Evidence shortcuts:** `docs/METAL_A11_CAPABILITIES.md` (capability matrix
> + the runtime probe), `docs/METAL_FEATURE_MAPPING.md` (the full
> option-by-option analysis for each HARDWARE LIMITED conclusion).
> `FST` = `docs/apple/Metal-Feature-Set-Tables.pdf` (May 21, 2026);
> **FST-2017** = the 2017-09-12 edition, which has a literal
> `iOS_GPUFamily4_v1` (= A11) column.

---

## 0. How to read a row

Each row gives:

| Column | Meaning |
|---|---|
| **Limit** | The precise, bounded statement of what the hardware/API cannot do. |
| **Runtime query** | The exact, runnable check that detects the limit on the device — a `MTLDevice` property, a `supportsFamily:` call, a table lookup, or a probe. |
| **Affected GL** | The OpenGL feature(s) hit. |
| **Fallback** | What TGLMT does instead, today. |
| **Status** | OPEN / CLOSED, plus the project status token. |
| **Class** | `HARDWARE_LIMITED` (true hardware/API limit) vs `MISSING` (implementation gap). |

---

## 1. Summary table

| # | Limit | Class | Status | Fallback today |
|---|---|---|---|---|
| L1 | No line-width API in Metal at all | `HARDWARE_LIMITED` | **CLOSED** (core) / OPEN (compatibility) | Report `GL_ALIASED_LINE_WIDTH_RANGE` = (1,1); reject `>1` |
| L2 | Metal's flat interpolation takes the **first** vertex; GL 4.x wants the **last** | `HARDWARE_LIMITED` | **OPEN** | None — flat-shaded output is silently wrong |
| L3 | One viewport + one scissor per render pass on A11 | `HARDWARE_LIMITED` | **OPEN** | Only viewport 0 applied; `GL_MAX_VIEWPORTS` over-reported as 16 |
| L4 | `[[clip_distance]]` accepts only one scalar in practice | `HARDWARE_LIMITED` | **OPEN** | `GL_MAX_CLIP_DISTANCES` over-reported as 8; only `[0]` used |
| L5 | No bitwise blend operation | `HARDWARE_LIMITED` | **OPEN** | `GL_COLOR_LOGIC_OP` bypasses the blend stage and **does nothing** |
| L6 | No per-sample shading rate | `HARDWARE_LIMITED` | **CLOSED** (by conformance, not by code) | Shade every sample; GL permits a higher rate |
| L7 | Border-clamp sampling needs hardware family Apple7 | `HARDWARE_LIMITED` | **OPEN** | Uses `MTLSamplerAddressModeClampToBorderColor` unconditionally → latent validation risk |
| L8 | No sampler LOD bias before iOS 26 | `HARDWARE_LIMITED` | **OPEN** | `GL_TEXTURE_LOD_BIAS` parsed but never reaches Metal |
| L9 | No BC1–BC7 decoder in A11 | `HARDWARE_LIMITED` | **OPEN** | `newCompressedTexture` returns `nullptr`; CPU shadow kept; no transcode |
| L10 | `GL_COMPRESSED_TEXTURE_FORMATS` advertises BC regardless of device | `MISSING` | **OPEN** | Advertises formats the device cannot use |
| L11 | `MTLPixelFormatDepth24Unorm_Stencil8` does not exist on iOS | `HARDWARE_LIMITED` | **CLOSED** | Map D24S8 → `Depth32Float_Stencil8` |
| L12 | No fp64 in MSL | `HARDWARE_LIMITED` | **OPEN** | `double` in GLSL is not emulated |
| L13 | No 64-bit atomics (Apple9+) | `HARDWARE_LIMITED` | **OPEN** | — |
| L14 | No floating-point atomics (Apple7+) | `HARDWARE_LIMITED` | **OPEN** | — |
| L15 | No texture (image) atomics (Apple6+; API iOS 17) | `HARDWARE_LIMITED` | **OPEN** | — |
| L16 | Buffer argument table = 31 entries per stage | `HARDWARE_LIMITED` | **OPEN** | `GL_MAX_UNIFORM_BUFFER_BINDINGS` = 36 over-reports; the converter **fails at compile time** with a diagnostic |
| L17 | Texture argument table: 31 (FST-2017) vs 96 (FST-2026) | `NOT_VERIFIED` | **OPEN** | `GL_MAX_TEXTURE_IMAGE_UNITS` = 32 — unsafe under either table |
| L18 | Sampler argument table = 16 per stage | `HARDWARE_LIMITED` | **OPEN** | Unused samplers pruned; no hard count check |
| L19 | No layered rendering (Apple5+) | `HARDWARE_LIMITED` | **OPEN** | `makeMSAATargetLayered` uses `MTLTextureType2DMultisampleArray` (Apple7 for layered MSAA) |
| L20 | No Metal 4 tier-2 barriers (`MTLAccessor`) | `HARDWARE_LIMITED` | **CLOSED** | Encoder-boundary ordering |
| L21 | No geometry-shader stage in Metal; no mesh shader on A11 | `MISSING` (D-018 forbids HARDWARE_LIMITED) | **OPEN** | GS programs draw nothing |
| L22 | No `MTLEvent`-based fence wait in TGLMT | `MISSING` (the API exists on A11) | **OPEN** | `glClientWaitSync` always returns `ALREADY_SIGNALED` |
| L23 | Dual-source blending mapped to a second draw buffer instead of `index(1)` | `MISSING` (hardware supports it) | **OPEN** | Wrong blend result |
| L24 | Metal's stencil has one reference value in TGLMT (Metal has two since iOS 9) | `MISSING` | **OPEN** | Front/back reference collapsed |
| L25 | No alpha-to-coverage wiring | `MISSING` (hardware supports it) | **OPEN** | `GL_SAMPLE_ALPHA_TO_COVERAGE` ignored |
| L26 | `GL_SAMPLE_MASK_VALUE` emulated with `discard` instead of `[[sample_mask]]` | `MISSING` | **OPEN** | Wrong sample-write semantics |
| L27 | `GL_POINT_SIZE` not plumbed to `[[point_size]]` in the raster path | `MISSING` | **OPEN** | All points 1 px |
| L28 | `glPolygonOffset` units vs Metal's 2⁻²⁴ bias | `MISSING` (approximation) | **OPEN** | ~1 part in 2²⁴ difference |
| L29 | `glClipControl` depth-mode half not implemented | `MISSING` | **OPEN** | `ZERO_TO_ONE` silently wrong |
| L30 | `GL_POLYGON_MODE` face argument (FRONT/BACK) collapsed | `MISSING` | **OPEN** | — |
| L31 | `glBlitFramebuffer` `GL_LINEAR` implemented as NEAREST | `MISSING` | **OPEN** | Visible divergence |
| L32 | `glReadPixels` supports RGBA8 sources only | `MISSING` | **OPEN** | Float attachments read back wrong |
| L33 | `glDrawTransformFeedback` replay does not restore attribute bindings | `MISSING` | **OPEN** | — |
| L34 | `GL_MAX_VIEWPORTS` / `GL_MAX_CLIP_DISTANCES` / `GL_MAX_TEXTURE_IMAGE_UNITS` over-reported | `MISSING` | **OPEN** | App can be led into a Metal validation abort |
| L35 | `GL_MAX_TEXTURE_SIZE` etc. under-reported (8192 vs 16384) | `MISSING` (conservative) | **OPEN** | — |

---

## 2. True hardware / Metal-API limits

Each row: the limit, the **runtime query** that detects it, the affected GL
feature, the fallback, and the status.

---

### L1 — Metal has no line-width API

| | |
|---|---|
| **Limit** | Metal rasterises lines at exactly **1 pixel** width. There is no API to change it: `grep -rn "lineWidth\|LineWidth" $SDK/System/Library/Frameworks/Metal.framework/Headers/` returns **0 hits**, and `MTLRenderPipelineDescriptor` has no line-width property. |
| **Runtime query** | There is no direct query. The detection is: `glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, r)` → TGLMT returns `(1,1)`; and any `glLineWidth(w>1)` is rejected with `GL_INVALID_VALUE`. On the device, confirm the absence by grepping the SDK (above) — Metal offers no positive query. |
| **Affected GL** | `glLineWidth`, `GL_ALIASED_LINE_WIDTH_RANGE`, `GL_ALIASED_LINE_WIDTH_GRANULARITY`, `GL_SMOOTH_LINE_WIDTH_RANGE` |
| **Fallback** | TGLMT reports `GL_ALIASED_LINE_WIDTH_RANGE` = `(1,1)` (`src/gl/gl_get.cpp:353`, `:527`, `:593`) and rejects widths > 1. |
| **Status** | **CLOSED** for the GL **core** profile — GL 4.6 App. E.2 mandates `GL_INVALID_VALUE` for `glLineWidth > 1`, so the current behaviour is exactly conformant. **OPEN** for the **compatibility** profile, where wide lines are legal: the fix is CPU expansion of each segment into a quad with mitred joins (`docs/LEGACY_TO_METAL_MAPPING.md` §7.5). |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · SDK header scan + `MTLRenderPipeline.h:179-307` (full property list) + no FST row for line width. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §3.2, §10 H1/H2. |

---

### L2 — Provoking vertex: Metal = first, GL 4.x = last

| | |
|---|---|
| **Limit** | MSL's `[[flat]]` interpolation takes the value from the **first** vertex of the primitive. GL 4.x's default is `GL_LAST_VERTEX_CONVENTION` (the *last* vertex). Metal offers no way to change this: `grep -rn "ProvokingVertex" $SDK/.../Metal.framework/Headers/` → **0 hits** (there is no `setProvokingVertexMode:` in the iOS SDK). |
| **Runtime query** | Draw a triangle with three different flat-sourced colours and read back the pixel: Metal returns vertex 0's colour, GL requires vertex 2's. The *detection* is a pixel test, not a property. |
| **Affected GL** | `glProvokingVertex`, `GL_PROVOKING_VERTEX`, all `flat`-qualified varyings, and the GL 1.x `glShadeModel(GL_FLAT)` |
| **Fallback** | **None today.** TGLMT stores the state (`src/gl/gl_readpixels_dsa.cpp:260-266`) and reports `GL_PROVOKING_VERTEX` = `0x8E4E` (LAST) (`src/gl/gl_get.cpp:293-295`), but the raster path cannot honour it — the flat value still comes from the first vertex. The app is told LAST and gets FIRST: a **silent** divergence. |
| **Status** | **OPEN** — `PARTIAL` / `INCORRECT` for flat-shaded draws. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · Metal Programming Guide, "Graphics Rendering: Render Command Encoder": *"When rendering a triangle primitive with flat shading, the attributes of the **first** vertex (also known as the provoking vertex) are used for the whole triangle."* <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Render-Ctx/Render-Ctx.html>. Corroborated by `metal2vulkan` `docs/API.md` ("Xbox uses 'last vertex', Metal uses 'first vertex'") and by ANGLE's Metal backend, which rewrites the index buffer specifically to impose GL's last-vertex rule. **This corrects `docs/LEGACY_TO_METAL_MAPPING.md` §1.5, which states Metal uses the last vertex.** Fix: CPU index rotation (swap first/last per triangle; rotate for strips; swap for lines) — the same technique ANGLE uses. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §3.2, §10 H15. |

---

### L3 — One viewport and one scissor rectangle per render pass

| | |
|---|---|
| **Limit** | A11 supports **exactly 1** viewport and 1 scissor rectangle per render pass. |
| **Runtime query** | FST p.7, limits row *"Maximum number of viewports and scissor rectangles, per vertex function"*: Apple2 = 1, Apple3 = 1, **Apple4 = 1**, Apple5…Apple10 = 16. FST-2017 p.3 carries the identical row with `iOS_GPUFamily4_v1` = **1**. FST p.3 feature table: "Multiple viewports \| Metal 3 & 4 \| **Apple5**". The Metal API `setViewports:count:` exists (`MTLRenderCommandEncoder.h:287`, iOS 12.0) but the *device* limit is 1. |
| **Affected GL** | `glViewportIndexed`, `glScissorIndexed`, `glViewportIndexedfv`, `GL_MAX_VIEWPORTS`, `GL_VIEWPORT_INDEX_PROVOKING_VERTEX`, and per-viewport depth ranges |
| **Fallback** | Only viewport index 0 is applied (`src/metal/AppleMetalBridge.mm:873-877`). `GL_MAX_VIEWPORTS` is reported as **16** (`src/gl/gl_get.cpp:302`, `kMaxViewports` = 16 in `include/tglmt/StateTracker.h:19`) — an **over-report**. The comment at `src/gl/gl_get.cpp:299-301` says the 16 is deliberate so "apps that divide by ≤ this value are safe" — but a Metal validation abort is the opposite of safe. |
| **Status** | **OPEN** — `INCORRECT` (the limit query lies). Fix: report `GL_MAX_VIEWPORTS` = 1 and raise `GL_INVALID_OPERATION` for `glViewportIndexed(idx > 0)`; optionally provide a multi-pass emulation for the common 2-viewport case. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.7 + FST-2017 p.3 + FST p.3. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §1, §10 H5. |

---

### L4 — One `[[clip_distance]]` in practice

| | |
|---|---|
| **Limit** | The MSL spec permits `[[clip_distance]]` to be `float` or `float[n]`, but the **Metal compiler rejects the array form** — TGLMT measured this and hard-fails with *"Metal chỉ nhận MỘT scalar"*. |
| **Runtime query** | Compile a vertex function returning `float2 cd [[clip_distance]]` — the Metal compiler's error message states the limit. TGLMT's own failure path: `src/gl/glsl_to_msl.cpp:6024-6027`, `:6072`. |
| **Affected GL** | `gl_ClipDistance`, `gl_ClipPlane`, `GL_MAX_CLIP_DISTANCES` (GL 4.6 core minimum **8**; the GL 1.x `GL_CLIP_PLANE0..5` set is **6**) |
| **Fallback** | Only `gl_ClipDistance[0]` is usable. `GL_MAX_CLIP_DISTANCES` is reported as **8** (`src/gl/gl_get.cpp:303`) — an over-report; `gl_ClipDistance[1..7]` is accepted by the GL layer and then fails at shader-compile time. |
| **Status** | **OPEN** — `PARTIAL`. Fix: report `GL_MAX_CLIP_DISTANCES` = 1, **or** implement the CPU Sutherland–Hodgman clip of the assembled primitive (the option `docs/LEGACY_TO_METAL_MAPPING.md` §7.3 already selects, and which this document endorses). |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` (measured by TGLMT on the Metal compiler; the MSL spec's Table 5.4 allows `float[n]`, so the *spec* permits it and the *compiler* does not) · MSL spec p.143 + `src/gl/glsl_to_msl.cpp:6024-6027`. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §1, §9, §10. |

---

### L5 — No bitwise blend operation (logic op)

| | |
|---|---|
| **Limit** | `MTLBlendOperation` has exactly six values — `Add, Subtract, ReverseSubtract, Min, Max, Unspecialized` (`MTLRenderPipeline.h:53-64`). None of GL's 16 logic ops (`CLEAR, AND, AND_REVERSE, COPY, AND_INVERTED, NOOP, XOR, OR, NOR, EQUIV, INVERT, OR_REVERSE, COPY_INVERTED, OR_INVERTED, NAND, SET`) is expressible. |
| **Runtime query** | `grep -rn "logicOp" $SDK/.../Metal.framework/` → 0 hits; read `MTLBlendOperation` in `MTLRenderPipeline.h:53-64`. Behavioural check: enable `GL_COLOR_LOGIC_OP`, draw, read back — the destination is unchanged. |
| **Affected GL** | `glLogicOp`, `GL_COLOR_LOGIC_OP` (GL 1.0 core; removed from the GL 3.2+ core profile, so it is a **compatibility-profile** feature) |
| **Fallback** | TGLMT **bypasses the blend stage** when `GL_COLOR_LOGIC_OP` is enabled (`src/gl/gl_apple_draw.cpp:2320-2322`) — which is the correct *structure* (the logic op replaces blending) — but **never performs the operation**, so the framebuffer keeps its old value. |
| **Status** | **OPEN** — `INCORRECT` (silently wrong pixels). |
| **Class / evidence** | `HARDWARE_LIMITED` for the blend stage; the **feature** is `GPU EMULATION`-able · `REFERENCE_ONLY` · `MTLRenderPipeline.h:53-64`. **Crucially, A11 has programmable blending (framebuffer fetch)** — FST p.3 "Programmable blending \| Metal 3 & 4 \| **Apple2**" and FST-2017 p.2 shows ✓ in the `iOS_GPUFamily4_v1` column — so the single-pass emulation (read the destination via a `[[color(0)]]` fragment **input**, apply the bitwise op, write the result) is available on A11. The two-pass form (source + destination textures, combine in a full-screen pass) is the universal fallback. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §2, §10 H4. |

---

### L6 — No per-sample shading rate

| | |
|---|---|
| **Limit** | Metal has no per-sample shading-rate control. `MTLRenderPipelineDescriptor` has no such property; the fragment shader is invoked per-pixel by default. Metal's `rasterizationRateMap` is a **different** feature (pixel coarsening) and is hardware family **Apple6+**, so it is not available on A11 either. |
| **Runtime query** | Property-based: none exists. Behavioural: enable `GL_SAMPLE_SHADING` with `GL_MIN_SAMPLE_SHADING_VALUE` = 0.5 on a 4× MSAA target and count the fragment invocations — Metal will invoke once per pixel (rate 1.0), never once per two samples. |
| **Affected GL** | `glEnable(GL_SAMPLE_SHADING)`, `GL_MIN_SAMPLE_SHADING_VALUE` |
| **Fallback** | TGLMT tracks and reports the state but does not apply it; it logs once per draw that the state is being ignored (`src/gl/gl_apple_draw.cpp:2602-2612`). |
| **Status** | **CLOSED** — and the closure is by *conformance*, not by code: GL 4.6 §14.3.1.1 states the implementation may shade at a rate **higher** than the requested minimum, so shading every sample is conformant. The only divergence is performance, and TGLMT says so in the log. `NOT_APPLICABLE` for correctness. |
| **Class / evidence** | `HARDWARE_LIMITED` (performance only) · `REFERENCE_ONLY` · no property in `MTLRenderPipeline.h:179-307`; FST p.4 "Variable rasterization rate \| Metal 3 & 4 \| **Apple6**". Options analysis: `docs/METAL_FEATURE_MAPPING.md` §3.2, §10 H3. |

---

### L7 — Border-clamp sampling requires hardware family Apple7

| | |
|---|---|
| **Limit** | `MTLSamplerAddressModeClampToBorderColor` is declared from iOS 14.0, but the **feature** is hardware family **Apple7** (A14+). A11 does not have it. Metal's border colour is also only an **enum of three values** (transparent black / opaque black / opaque white), not GL's arbitrary `float4`. |
| **Runtime query** | Create a sampler with `addressMode = .clampToBorderColor` and sample outside `[0,1]`; on hardware without the feature the sampler creation or the draw fails Metal validation. There is **no** positive capability property for it. FST p.3 puts "Border color" at **Apple7**; FST-2017 p.2 shows "Border color" ✓ **only** on macOS 10.12/10.13 — on **no** iOS column at all. |
| **Affected GL** | `GL_CLAMP_TO_BORDER`, `GL_TEXTURE_BORDER_COLOR`, `GL_ARB_texture_border_clamp` |
| **Fallback** | TGLMT maps `GL_CLAMP_TO_BORDER` → `MTLSamplerAddressModeClampToBorderColor` **unconditionally** (`src/metal/AppleMetalBridge.mm:218`) and sets `d.borderColor` (`:2945`), approximating GL's arbitrary border colour by the nearest of Metal's three enum values (`:229-238`). |
| **Status** | **OPEN** — this is a **latent crash/abort risk on A11**: the sampler may be rejected by Metal validation, and even if accepted, the border colour is wrong. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.3 + FST-2017 p.2; `MTLSampler.h:70` (API availability) vs `MTLSampler.h:86-89` (3-value enum) — note the **API/feature mismatch**: the API exists on iOS 14+ but the hardware does not. Fix: emulate in the shader (clamp manually; `discard_fragment()` for the transparent case). Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H6. |

---

### L8 — No sampler LOD bias

| | |
|---|---|
| **Limit** | `MTLSamplerDescriptor.lodBias` is `API_AVAILABLE(macos(26.0), ios(26.0))` (`MTLSampler.h:204`) and FST p.5 puts "Sampler LOD bias" at hardware family **Apple10**. A11 (max iOS 16) has no sampler LOD bias. |
| **Runtime query** | `grep -rn "lodBias" $SDK/.../Metal.framework/Headers/MTLSampler.h` → the only hit carries `API_AVAILABLE(macos(26.0), ios(26.0))`. No runtime property. |
| **Affected GL** | `GL_TEXTURE_LOD_BIAS` (GL 1.4), `GL_MAX_TEXTURE_LOD_BIAS` |
| **Fallback** | TGLMT reports `GL_MAX_TEXTURE_LOD_BIAS` = **15** (`src/gl/gl_get.cpp:339`) — the GL 1.4 minimum — and parses the texture parameter, but the bias never reaches Metal (`src/gl/gl_texture.cpp:1534`: *"mất độ chính xác (vd GL_TEXTURE_LOD_BIAS 0.5 → 0)"*). |
| **Status** | **OPEN** — `MISSING` (the parameter is accepted and silently ignored → wrong mip selection). Fix: bake the bias into the generated MSL's LOD computation. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · `MTLSampler.h:204` + FST p.5. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H7. |

---

### L9 — No BC1–BC7 decoder in A11

| | |
|---|---|
| **Limit** | A11 cannot sample BC-compressed (S3TC/RGTC/BPTC) textures. FST p.3 puts "BC pixel formats" at hardware family **Apple9** and footnote 2 says *"As of Apple9 all GPUs have support"* (some Apple7/Apple8 **iPadOS** devices support it, which is why Apple added a runtime property). |
| **Runtime query** | `MTLDevice.supportsBCTextureCompression` — `API_AVAILABLE(macos(11.0), ios(16.4))` (`MTLDevice.h:510`). On A11 it returns **false**. TGLMT already wraps this in `deviceSupportsBC()` (`src/metal/AppleMetalBridge.mm:1449-1455`). |
| **Affected GL** | `GL_COMPRESSED_S3RGB_S3TC_DXT1_EXT`, `…DXT3/5`, `GL_COMPRESSED_RGTC_RED/…`, `GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT`, and `glCompressedTexImage2D` with those internal formats |
| **Fallback** | `newCompressedTexture` returns `nullptr` for BC formats (`src/metal/AppleMetalBridge.mm:1502-1508`) and the GL layer keeps a **CPU shadow**. There is **no** CPU decode/transcode path — `docs/METAL_A11_LIMITATIONS.md` §2.5 already records that `docs/EXTENSION_MATRIX.md`'s claim of a "CPU decode fallback" is **not** implemented. |
| **Status** | **OPEN** — the texture renders black/empty. The remedy (CPU transcode BC→ASTC, or CPU decompress BC→RGBA8, at load time) is designed but not built; ASTC *is* available on A11. |
| **Class / evidence** | `HARDWARE_LIMITED` natively, **but reachable via CPU transcode** — so **not** `BLOCKED` · `REFERENCE_ONLY` · FST p.3 + footnote 2, `MTLDevice.h:510`. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H8. |

---

### L10 — `GL_COMPRESSED_TEXTURE_FORMATS` advertises formats the device cannot use

| | |
|---|---|
| **Limit** | The advertised list is a **compile-time constant** (`kCompressedFmts`, consumed by `CompressedTextureFormats()`, `src/gl/gl_texture.cpp:2642-2653`) and includes BC1–BC7 regardless of the device. |
| **Runtime query** | `glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, …)` → compare against `deviceSupportsBC()`; or read `GL_COMPRESSED_TEXTURE_FORMATS` on the device and check that BC is absent. |
| **Affected GL** | `GL_COMPRESSED_TEXTURE_FORMATS`, `GL_NUM_COMPRESSED_TEXTURE_FORMATS` |
| **Fallback** | None — the list is static. |
| **Status** | **OPEN** — `MISSING` (an *implementation* bug, not a hardware limit). Fix: filter the advertised list by the device's compressed-format capability. |
| **Class / evidence** | `MISSING` · `REFERENCE_ONLY` for the device capability · `src/gl/gl_texture.cpp:2642`, `src/gl/gl_get.cpp:345-347`, `src/metal/AppleMetalBridge.mm:1449-1455`. |

---

### L11 — `MTLPixelFormatDepth24Unorm_Stencil8` does not exist on iOS

| | |
|---|---|
| **Limit** | The D24S8 pixel format is **unavailable on the entire iOS platform**, not just on A11. |
| **Runtime query** | `grep -n "Depth24Unorm_Stencil8" $SDK/.../MTLPixelFormat.h` → the constant carries `API_UNAVAILABLE(ios)`. Likewise `MTLDevice.depth24Stencil8PixelFormatSupported` is `API_UNAVAILABLE(ios)` (`MTLDevice.h:461`). |
| **Affected GL** | `GL_DEPTH24_STENCIL8` |
| **Fallback** | TGLMT maps `Depth24Stencil8` → `MTLPixelFormatDepth32Float_Stencil8` (`src/metal/AppleMetalBridge.mm:105`). The depth buffer then carries 32-bit float precision instead of 24-bit fixed point — a **superset**, so no GL-visible behaviour changes. |
| **Status** | **CLOSED** — `FULLY_IMPLEMENTED` for GL semantics. |
| **Class / evidence** | `HARDWARE_LIMITED` (platform-wide) but fully mitigated · `REFERENCE_ONLY` · `MTLPixelFormat.h:219`, `MTLDevice.h:461`, `src/metal/AppleMetalBridge.mm:105`. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H9. |

---

### L12 — No fp64 in MSL

| | |
|---|---|
| **Limit** | The MSL specification states: *"Metal does not support the `double`, `long long`, `unsigned long long`, and `long double` data types."* MSL's `long`/`ulong` are 64-bit **integers**, not floats. |
| **Runtime query** | Compile MSL containing `double` — the Metal compiler rejects it. TGLMT's GLSL→MSL converter must therefore reject or emulate `double`. |
| **Affected GL** | GLSL `double`, `dvec2/3/4`, `dmat*`, and the `GL_ARB_gpu_shader_fp64` feature that GL 4.0+ core **requires** |
| **Fallback** | None — the converter has no double path. |
| **Status** | **OPEN** — a genuine GL 4.6 **core-profile** conformance gap. Remedy: two-float (double-single) emulation in generated MSL, with a documented precision loss (~48-bit mantissa, not bit-exact). |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · MSL spec §2.1, p.25 (`docs/apple/Metal-Shading-Language-Specification.pdf`). Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H10. |

---

### L13 — No 64-bit atomics

| | |
|---|---|
| **Limit** | FST p.5: "64-bit atomics \| Metal 3 & 4 \| **Apple9**". Footnote 7: Apple8 supports only min/max on macOS; the full set starts at Apple9. |
| **Runtime query** | Compile MSL using `atomic_fetch_add_explicit` on a `device atomic<long>` — the Metal compiler reports the unsupported type on non-Apple9 hardware. |
| **Affected GL** | GLSL `atomicAdd`/`atomicMin`/`atomicMax`/`atomicAnd`/… on `int64_t`/`uint64_t`; `GL_MAX_COMPUTE_ATOMIC_COUNTERS` semantics |
| **Fallback** | None. |
| **Status** | **OPEN** — `MISSING`. Remedy: a compare-exchange loop on two 32-bit atomics, or a threadgroup mutex. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.5 + footnote 7. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H11. |

---

### L14 — No floating-point atomics

| | |
|---|---|
| **Limit** | FST p.5: "Floating-point atomics \| Metal 3 & 4 \| **Apple7**". |
| **Runtime query** | Compile MSL using `atomic_fetch_add_explicit` on a `device atomic<float>` — the Metal compiler reports the unsupported type. |
| **Affected GL** | GLSL float `atomicAdd`/`atomicMin`/`atomicMax`/`atomicExchange` on `float` |
| **Fallback** | None. |
| **Status** | **OPEN** — `MISSING`. Remedy: a compare-exchange loop on the integer bit pattern (the classic lock-free float add). |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.5. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H12. |

---

### L15 — No texture (image) atomics

| | |
|---|---|
| **Limit** | FST p.5: "Texture atomics \| Metal 3 & 4 \| **Apple6**". Independently, the SDK's `MTLTextureUsageShaderAtomic` is `API_AVAILABLE(macos(14.0), **ios(17.0)**)` (`MTLTexture.h:104`) — i.e. **beyond the A11's maximum OS** even if the hardware allowed it. Note that *read/write textures* (non-atomic) **are** available: FST p.4 "Read/write textures in functions \| Metal 3 & 4 \| **Apple4**" — exactly at the A11 floor. |
| **Runtime query** | `grep -n "ShaderAtomic" $SDK/.../MTLTexture.h` → the availability macro. On the device: creating a texture with `MTLTextureUsageShaderAtomic` fails on iOS ≤ 16 (the usage flag is not even declared). |
| **Affected GL** | `GL_ARB_shader_image_load_store` atomic variants (`imageAtomicAdd`, `imageAtomicMin`, `imageAtomicMax`, `imageAtomicAnd`, `imageAtomicOr`, `imageAtomicXor`, `imageAtomicExchange`, `imageAtomicCompSwap`) |
| **Fallback** | None — the plain (non-atomic) image load/store path works. |
| **Status** | **OPEN** — `MISSING`. Remedy: back the GL "image" with a **device buffer** (SSBO) and use buffer atomics, which A11 has (FST p.4 "Read/write buffers in functions \| Apple3"). |
| **Class / evidence** | `HARDWARE_LIMITED` natively, emulable via SSBO-backed images · `REFERENCE_ONLY` · FST p.4/p.5, `MTLTexture.h:104`. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §5, §10 H13. |

---

### L16 — Buffer argument table = 31 entries per stage

| | |
|---|---|
| **Limit** | Metal's **buffer** argument table holds **31 entries per graphics or kernel function** on every Apple family, A11 included. FST p.7: "Maximum number of entries in the buffer argument table, per graphics or kernel function" = 31 for Apple2…Apple10; FST-2017 p.3 gives the same 31 for `iOS_GPUFamily4_v1`. GL 4.6 requires `GL_MAX_UNIFORM_BUFFER_BINDINGS ≥ 36`. |
| **Runtime query** | Bind buffer index 31 to a stage and draw — Metal raises a validation error ("index out of range"). The FST row is the documented limit. |
| **Affected GL** | `glBindBufferBase`/`glBindBufferRange` for `GL_UNIFORM_BUFFER` and `GL_SHADER_STORAGE_BUFFER`; `GL_MAX_UNIFORM_BUFFER_BINDINGS`, `GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS` |
| **Fallback** | The GLSL→MSL converter **fails at compile time** with a diagnostic when a graphics program needs a slot > 30: `fail("UBO graphics vượt slot buffer(30)")` (`src/gl/glsl_to_msl.cpp:10736`), `fail("SSBO graphics vượt slot buffer(30)")` (`:10694`), `fail("atomic counter graphics vượt slot buffer(30)")` (`:11888`). The failure surfaces as `GL_COMPILE_STATUS = false` with the message in the info log (`src/gl/gl_shader.cpp:79-80`) — **a clean, diagnosable GL error, not a runtime Metal abort**. The draw-time guards (`if (sb.slot < 0 || sb.slot > 30) continue;`, `src/gl/gl_apple_draw.cpp:1051, 1060, 1069, 1666, 3161, 3168`) are then unreachable for converter-produced slots. Only the **legacy** bind path can still silently drop: `if (slot > 30) break;` (`src/gl/gl_apple_draw.cpp:3118`). |
| **Status** | **OPEN** — but the failure mode is **safe** (compile-time error), so the defect is limited to the *advertised* limit: TGLMT reports `GL_MAX_UNIFORM_BUFFER_BINDINGS` = **36** (`src/gl/gl_get.cpp:183`) and `GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS` = **16** (`src/gl/gl_get.cpp:216`), while the converter's effective graphics budget is **14 UBO+SSBO blocks per stage** (slots 17…30; `gfxNext` starts at 17, `src/gl/glsl_to_msl.cpp:10688`). An app that trusts the query and uses 20 blocks gets a link error it cannot explain. **Correction to `docs/METAL_A11_LIMITATIONS.md` §3**, which calls the >30 case a "Metal validation abort" and a "crash risk": the converter refuses the program first, so there is no abort. |
| **Class / evidence** | `HARDWARE_LIMITED` (the 31-slot table is a hardware fact) · `REFERENCE_ONLY` for the table · the enforcement evidence is `REFERENCE_ONLY` (code read, not a device run) · FST p.7 + FST-2017 p.3. Fix: report the limit TGLMT can actually honour (14 graphics blocks/stage) or widen the slot allocation to use 0…30, and keep the compile-time diagnostic. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §4, §10 H14. |

---

### L17 — Texture argument table: 31 or 96? (`NOT_VERIFIED`)

| | |
|---|---|
| **Limit** | Apple's two Feature Set Table editions **disagree** about the A11: FST-2017 p.3 gives **31** for `iOS_GPUFamily4_v1`; FST p.7 gives **96** for Apple4. |
| **Runtime query** | Compile a fragment function with N `[[texture(i)]]` bindings and find the largest N that compiles (the probe's `probeTextureBindingLimit()` in `docs/METAL_A11_CAPABILITIES.md` §4). The Metal compiler's error states the limit. |
| **Affected GL** | `GL_MAX_TEXTURE_IMAGE_UNITS`, `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`, and any shader with many texture units |
| **Fallback** | TGLMT reports `GL_MAX_TEXTURE_IMAGE_UNITS` = **32** (`src/gl/gl_get.cpp:159`) and `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` = **48** (`include/tglmt/limits.h:13`). Under the 2017 table, 32 > 31 → a 32-texture fragment shader aborts the Metal command buffer. Under the 2026 table, 32 ≤ 96 → fine. |
| **Status** | **OPEN** — `NOT_VERIFIED`. Until the device probe runs, the safe value is **16** (the GL 4.6 minimum for `GL_MAX_TEXTURE_IMAGE_UNITS`, safe under either table). `docs/METAL_A11_LIMITATIONS.md` §3 asserts the 31 figure as fact; **this document does not** — it is `NOT_VERIFIED` because Apple's own current documentation contradicts it. |
| **Class / evidence** | `NOT_VERIFIED` (not `HARDWARE_LIMITED` — the hardware limit is real, its *value* is unknown) · evidence: FST-2017 p.3 vs FST p.7. |

---

### L18 — Sampler argument table = 16 per stage

| | |
|---|---|
| **Limit** | Metal's **sampler state** argument table holds **16 entries per graphics or kernel function** on every Apple family. FST p.7: 16 for Apple2…Apple10, with footnote 3 adding that inline `constexpr` samplers declared in MSL **also count** against the same 16. FST-2017 p.3 gives 16 for `iOS_GPUFamily4_v1`. |
| **Runtime query** | Compile a fragment function with N `[[sampler(i)]]` arguments and find the largest N that compiles (same probe technique as L17). |
| **Affected GL** | `GL_MAX_TEXTURE_IMAGE_UNITS` (each GL texture unit carries its own sampler), `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` |
| **Fallback** | The GLSL→MSL converter **prunes samplers that the shader does not actually reference** (`src/gl/glsl_to_msl.cpp:11907-11930`) so that only the *used* ones consume a `[[sampler(i)]]` slot; the texture slot is the sampler's index and the sampler slot is a separate counter (`sampSlot++`, `:11968-11970`). TGLMT's `kMaxTextureUnits` = **32** (`include/tglmt/StateTracker.h:17`) and the bridge's binding cache is `kBindSlots = 32` (`src/metal/AppleMetalBridge.mm:1121`). **There is no hard check that the used-sampler count ≤ 16** — a shader that genuinely uses more than 16 samplers will produce MSL with `[[sampler(16)]]`… and the **Metal library compiler** rejects it. That rejection is a clean compile error (`compileLibrary` returns `nullptr`, `src/metal/AppleMetalBridge.mm:1797-1800`) which TGLMT turns into `pr.infoLog = "error: MSL FS compile: …"` and `GL_LINK_STATUS = false` (`src/gl/gl_program.cpp:800-805`) — again **a diagnosable GL error, not a runtime abort**. |
| **Status** | **OPEN** — `MISSING` (no pre-flight count check, no corrected limit query). The failure is safe (compile-time), but the app gets a link error instead of a clear "too many texture units". |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.7 + footnote 3, FST-2017 p.3. |

---

### L19 — No layered rendering

| | |
|---|---|
| **Limit** | FST p.3: "Layered rendering \| Metal 3 & 4 \| **Apple5**" and "Layered rendering to multisample textures \| **Apple7**". FST-2017 p.2 shows "Layered rendering" ✓ only on macOS 10.12/10.13 — on no iOS column. A11 therefore cannot render to *different layers of an array* within one pass via a per-primitive layer index. |
| **Runtime query** | A vertex function that writes `[[render_target_array_index]]` — the attribute exists from iOS 14 in the MSL spec, but the **hardware** floor is Apple5. On A11 the draw is rejected. |
| **Affected GL** | `gl_Layer` / layered rendering (GL 4.3 core), `glFramebufferTextureLayer` to an array with multiple layers, layered FBO rendering |
| **Fallback** | TGLMT's `makeMSAATargetLayered` creates an `MTLTextureType2DMultisampleArray` (`src/metal/AppleMetalBridge.mm:2248`) — the *layered MSAA* feature is **Apple7**, so this path is expected to fail on A11. Single-layer array rendering (`MTLTextureType2DArray` with a fixed `slice`) is a different mechanism and works. |
| **Status** | **OPEN** — `MISSING` / latent validation risk on A11 for the layered-MSAA path. |
| **Class / evidence** | `HARDWARE_LIMITED` · `REFERENCE_ONLY` · FST p.3, FST-2017 p.2. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §8, §10 H17. |

---

### L20 — No Metal 4 tier-2 resource barriers

| | |
|---|---|
| **Limit** | Metal 4's `MTLAccessor` / `MTLBarrierEncoder` tier-2 barrier API does not exist on A11 (or in the iOS 16 SDK at all): `grep -rn "MTLAccessor" $SDK/.../Metal.framework/Headers/` → **0 hits**; `MTLCommandEncoder.h:125` `barrierAfterQueueStages:` is `API_AVAILABLE(macos(26.0), ios(26.0))`. FST p.5 lists "Command barriers \| **Metal 4** \| Apple7". |
| **Runtime query** | The absence is structural (no symbol to query). The *substitute* is available and measurable: `MTLResourceStateCommandEncoder` exists from iOS 13.0 (`MTLCommandBuffer.h:424`), but FST p.3 **footnote 3** restricts it on Apple3…Apple10: memory barriers may **not** include the `.fragment`/`.tile` stages in `after`, nor `MTLBarrierScope.renderTargets`. |
| **Affected GL** | `glMemoryBarrier`, `glMemoryBarrierByRegion` (per-bit semantics) |
| **Fallback** | Encoder-boundary ordering: end the current encoder and start a new one. Within a command buffer, Metal executes encoders in order with automatic hazard tracking — **stronger** than GL's requirements for the cases TGLMT uses, so no information is lost. The divergence is that GL's *per-bit* semantics are not reproduced individually: every barrier bit behaves like a full barrier. |
| **Status** | **CLOSED** — `EMULATED`. The per-bit granularity is a documented divergence, not a correctness loss. |
| **Class / evidence** | `HARDWARE_LIMITED` (tier-2 API) with a working tier-1 substitute · `REFERENCE_ONLY` · FST p.5 + p.3 footnote 3, `MTLCommandBuffer.h:424`, `MTLCommandEncoder.h:125`. Options analysis: `docs/METAL_FEATURE_MAPPING.md` §4, §10 H16. |

---

## 3. Missing implementations (NOT hardware limits)

These are **not** hardware limits. The hardware can do them; TGLMT has not.
Listing them here is the point of the document — they must not be filed under
"A11 cannot".

| # | Item | Why it is *not* a hardware limit | Current behaviour | Fix | Status |
|---|---|---|---|---|---|
| L21 | **Geometry shaders** | D-018 forbids concluding `HARDWARE_LIMITED`. Metal has no GS *and* no mesh shader on A11 (mesh shading is Apple7, FST p.5) — but Metal has everything needed to **emulate** one: a compute kernel per input primitive, buffer atomics for the output ticket, and a second kernel to build the index buffer. | GS programs draw **nothing**: `src/gl/gl_apple_draw.cpp:1901-1904` returns `false` with the log *"AppleDrawGL: GS trace-only (M5b-future)"*. | The compute-based emulation specified in `docs/METAL_FEATURE_MAPPING.md` §3.5. | **OPEN** — `MISSING` |
| L22 | **`glClientWaitSync` / `glFenceSync` blocking** | `MTLEvent` and `MTLSharedEvent` exist on A11 from **iOS 12.0** (`MTLDevice.h:910, 916`); `MTLSharedEvent.waitUntilSignaledValue:timeoutMS:` is iOS 15.0 (`MTLEvent.h:62`). | `glClientWaitSync` always returns `ALREADY_SIGNALED` and never blocks (`src/gl/gl_sync_query.cpp:26-31`). **`docs/METAL_A11_LIMITATIONS.md` §6's parenthetical "(no Metal event)" is wrong.** | `MTLSharedEvent` + `MTLSharedEventListener`, signalled from a command-buffer completion handler. | **OPEN** — `MISSING` |
| L23 | **Dual-source blending mapping** | The hardware supports it: `MTLBlendFactorSource1*` are `API_AVAILABLE(macos(10.12), **ios(11.0)**)` (`MTLRenderPipeline.h:41-44`); MSL `color(m), index(i)` is "Metal 1.2 and later"; FST p.3 puts it at hardware family **Apple2**; FST-2017 p.2 shows ✓ on `iOS_GPUFamily4_v1`. | `src/gl/glsl_to_msl.cpp:12556` maps `index=1` to `[[color(1)]]` — a **second draw buffer**, not a second blend source. The blend result is wrong. | Emit `[[color(0)]] index(1)` and use `MTLBlendFactorSource1*`. | **OPEN** — `INCORRECT` |
| L24 | **Separate front/back stencil reference** | Metal has had `setStencilFrontReferenceValue:backReferenceValue:` since **iOS 9.0** (`MTLRenderCommandEncoder.h:468`). | TGLMT sets only `setStencilReferenceValue:` with the comment *"Metal 1 ref cho cả 2 mặt"* (`src/metal/AppleMetalBridge.mm:957-960`). | Call the two-value form. | **OPEN** — `PARTIAL` |
| L25 | **Alpha-to-coverage** | `MTLRenderPipelineDescriptor.alphaToCoverageEnabled` is `API_AVAILABLE(macos(10.11), ios(8.0))` (`MTLRenderPipeline.h:192`) — available on every Metal device including A11. | Not wired up: `grep alphaToCoverage src/metal/AppleMetalBridge.mm` → 0 hits. `GL_SAMPLE_ALPHA_TO_COVERAGE` is ignored. | Set the descriptor flag when the GL enable is on. | **OPEN** — `MISSING` |
| L26 | **Sample mask semantics** | MSL's `[[sample_mask]]` fragment **output** is "Metal 1 and later" (MSL spec Table 5.7, p.151). | TGLMT emulates `glSampleMaski` by injecting `[[sample_id]]` + a mask buffer + `discard_fragment()` (`src/gl/gl_program.cpp:210-236`). That kills the whole invocation rather than suppressing only the masked samples' writes. | Emit a real `[[sample_mask]]` output. | **OPEN** — `PARTIAL` |
| L27 | **`GL_POINT_SIZE` → `[[point_size]]`** | MSL's `[[point_size]]` is a Metal 1 vertex output; the A11 device limit is 511 px (FST p.8). | The raster path does not plumb `GL_POINT_SIZE`; only the TCS/TES paths emit `[[point_size]]` (`src/gl/glsl_to_msl.cpp:4717, 4774`). All points are 1 px. | Emit `[[point_size]]` in the vertex stage. | **OPEN** — `MISSING` |
| L28 | **Polygon-offset units** | The mapping is *nearly* right; the residual is a fixed-point convention difference (~1 part in 2²⁴), not a missing capability. | `units → depthBias` unchanged, `factor → slopeScale` (`src/gl/gl_apple_draw.cpp:2771-2796`). The GL 24-bit `r` is 1/(2²⁴−1); Metal's bias unit is 1/2²⁴. | Fold the depth range into the bias, or accept and document the 1-part-in-16 777 216 difference. | **OPEN** — `PARTIAL` |
| L29 | **`glClipControl` depth mode** | Nothing about the depth mode is a Metal limitation — it is a shader rewrite TGLMT has not finished. | The origin half works (viewport Y-flip, `src/gl/gl_apple_draw.cpp:2624-2633`); the depth half is acknowledged as incomplete (`src/gl/gl_program.cpp:1350`: *"shader đã bake z-convert mặc định; app đổi ClipControl"*). `GL_ZERO_TO_ONE` is silently wrong. | Make the z-conversion depend on the tracked clip-depth mode. | **OPEN** — `PARTIAL` |
| L30 | **`GL_POLYGON_MODE` face argument** | Metal has no notion of a polygon-mode face; this is a state-mapping gap. | FRONT/BACK are collapsed (`src/core/StateTracker.cpp:80-86`). | Keep the distinction in the GL state block and apply it in the CPU polygon expansion. | **OPEN** — `MISSING` |
| L31 | **`glBlitFramebuffer` filtering** | `MTLBlitCommandEncoder` performs a straight copy; scaling + filtering needs a render pass — entirely doable on A11. | `GL_LINEAR` + scaling is implemented as **NEAREST**; MRT blits only handle attachment 0 (`src/metal/AppleMetalBridge.mm:2301-2314`; `docs/OPENGL_46_METAL_MAPPING.md` §2.4). | Do the scaled blit as a full-screen textured quad. | **OPEN** — `INCORRECT` |
| L32 | **`glReadPixels` format coverage** | A GPU readback of any format is possible. | Only RGBA8 sources are supported; float attachments return quantised or zero data (`docs/OPENGL_46_METAL_MAPPING.md` §2.4). | Add a conversion pass. | **OPEN** — `PARTIAL` |
| L33 | **`glDrawTransformFeedback` replay** | Purely a TGLMT implementation detail. | The replay does not restore the original attribute bindings (`docs/OPENGL_46_METAL_MAPPING.md` §2.5). | Rebuild the vertex descriptor from the captured varyings. | **OPEN** — `PARTIAL` |
| L34 | **Over-reported limits** | These are TGLMT's own numbers, not the device's. | `GL_MAX_VIEWPORTS` = 16 (real: 1, L3); `GL_MAX_CLIP_DISTANCES` = 8 (real: 1, L4); `GL_MAX_TEXTURE_IMAGE_UNITS` = 32 (real: ≤ 31 or 96, L17); `GL_MAX_UNIFORM_BUFFER_BINDINGS` = 36 (real: 31, L16); `GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS` = 16 (real: 31 total across kinds). | Report the true value, or advertise the GL minimum **and** enforce the Metal limit so the failure is a GL error rather than a Metal abort. | **OPEN** — `INCORRECT` |
| L35 | **Under-reported limits** | Conservative, not wrong — but it makes apps give up capability they could have. | `GL_MAX_TEXTURE_SIZE` = 8192 (real 16 384), `GL_MAX_CUBE_MAP_TEXTURE_SIZE` = 8192, `GL_MAX_RENDERBUFFER_SIZE` = 8192, `GL_MAX_VIEWPORT_DIMS` = 8192², `GL_MAX_FRAMEBUFFER_WIDTH/HEIGHT` = 8192 (`src/gl/gl_get.cpp:149-153, 244, 401-402`). FST p.8 gives 16 384 for all of these on Apple4. | Query the device (or use the documented family values) instead of hard-coding 8192. | **OPEN** — `PARTIAL` |

---

## 4. Limits that are *not* on A11 but are on Metal in general

For completeness, so nobody files them under "A11":

| Feature | Where it becomes available | Evidence |
|---|---|---|
| Mesh shaders | Apple7 (A14) | FST p.5; `MTLDevice.h:830` (iOS 16.0) |
| Ray tracing | Apple6 (A13) | FST p.5; `MTLDevice.h:1145` (iOS 14.0) |
| BC texture compression | Apple9 (A14+, plus some Apple7/8 iPadOS) | FST p.3 + footnote 2; `MTLDevice.h:510` (iOS 16.4) |
| Layered rendering | Apple5 (A12) | FST p.3 |
| Multiple viewports | Apple5 (A12) | FST p.3, FST p.7 |
| Border colour | Apple7 (A14) | FST p.3 |
| Texture atomics | Apple6 (A13) **and** the API is iOS 17.0 | FST p.5; `MTLTexture.h:104` |
| 64-bit atomics | Apple9 | FST p.5 |
| Floating-point atomics | Apple7 | FST p.5 |
| Sparse textures | Apple6 | FST p.4 |
| Variable rasterization rate | Apple6 | FST p.4 |
| Vertex amplification | Apple6 | FST p.4 |
| Function pointers | Apple6 | FST p.5 |
| Barycentric coordinates / primitive ID | Apple7 | FST p.5 |
| Lossy texture compression | Apple8 | FST p.4 |
| Query texture LOD | Apple8 | FST p.5 + footnote 8 |
| Sampler LOD bias | Apple10 (and the API is iOS 26.0) | FST p.5; `MTLSampler.h:204` |
| Metal 4 (argument tables, command allocators/barriers, decoupled queues, tensors, ML encoding) | Apple7 | FST p.5-6 |

---

## 5. What must happen to close the OPEN items

| Priority | Action | Closes |
|---|---|---|
| **P0** | Run the capability probe (`docs/METAL_A11_CAPABILITIES.md` §4) on the physical A11 and commit the JSON report with `environment_label = "A11_DEVICE"`. | L17, and turns every `REFERENCE_ONLY` number into a measured one. |
| **P0** | Correct the **over-reported** limits: `GL_MAX_VIEWPORTS` → 1, `GL_MAX_CLIP_DISTANCES` → 1, and enforce the 31-slot buffer table in the bind path. | L3, L4, L16, L34 |
| **P0** | Stop advertising BC in `GL_COMPRESSED_TEXTURE_FORMATS` when `supportsBCTextureCompression` is false. | L10 |
| **P0** | Guard `MTLSamplerAddressModeClampToBorderColor` behind a device check, or emulate border clamp in the shader. | L7 |
| **P1** | Implement the dual-source `[[color(0)]] index(1)` mapping. | L23 |
| **P1** | Implement the `MTLSharedEvent`-based `glClientWaitSync`. | L22 |
| **P1** | Implement the provoking-vertex index rotation for `flat` varyings. | L2 |
| **P1** | Wire up alpha-to-coverage and the two-value stencil reference. | L24, L25 |
| **P2** | Implement the geometry-shader emulation (compute GS + index builder). | L21 |
| **P2** | CPU transcode/decompress of BC textures at load time. | L9 |
| **P2** | Bake `GL_TEXTURE_LOD_BIAS` into the generated MSL. | L8 |
| **P2** | Two-float fp64 emulation in the GLSL→MSL converter. | L12 |
| **P3** | Scaled/filtered `glBlitFramebuffer`, `glReadPixels` format coverage, `glDrawTransformFeedback` attribute restore, `glClipControl` depth mode, `GL_POLYGON_MODE` face argument. | L29–L33 |
| **P3** | Raise the under-reported texture-size limits to the documented 16 384. | L35 |

---

## 6. Honest summary

* **The A11 is not a weak device.** It has, at the family floor: tessellation,
  programmable sample positions, programmable blending (framebuffer fetch),
  dual-source blending, argument buffers tier 1, cube-map texture arrays,
  imageblocks, tile shaders, raster order groups, quad permutes, read-write
  textures, counting occlusion queries, 16 384-px textures, 8 MRTs, 4× MSAA,
  1 024-thread compute with 32 KB threadgroup memory, and 64-bit integers.
  Every one of those is documented in Apple's Feature Set Tables for
  `MTLGPUFamilyApple4` (or the literal `iOS_GPUFamily4_v1` column of the 2017
  edition).
* **The genuinely hard limits are few**: wide lines, bitwise logic op, >1
  viewport, >1 clip distance, border-clamp sampling, sampler LOD bias, BC
  textures, fp64, 64-bit/float/texture atomics, the 31-slot buffer table,
  layered rendering, and Metal 4 tier-2 barriers. Each has a designed
  remedy in `docs/METAL_FEATURE_MAPPING.md` §10.
* **The bigger risk to this project is not the hardware.** Ten of the
  OPEN items in §3 are plain **missing implementations** — including two
  that the previous documentation wrongly blamed on the hardware
  (dual-source blending, and the fence-sync event). Those are the cheapest
  wins available and they are currently mis-filed.
* **Nothing here has been verified on the device.** The single highest-value
  action is to run `docs/METAL_A11_CAPABILITIES.md` §4's probe on an iPhone 8/X
  and commit the JSON — after which this document's `REFERENCE_ONLY` and
  `NOT_VERIFIED` rows can be closed with measurements.
