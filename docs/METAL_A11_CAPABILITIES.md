# METAL_A11_CAPABILITIES.md — Metal capability matrix for the Apple A11 target

> **Scope.** What Metal can actually do on an **Apple A11** (iPhone 8 / 8 Plus / X)
> running **iOS 11 – iOS 16**, expressed as capability queries an app can run on
> the device, with the evidence for each claim.
>
> **This file is self-contained.** It restates (and where necessary corrects)
> `docs/METAL_A11_LIMITATIONS.md`; where the two disagree, the resolution and its
> evidence are given in-line.
>
> **Evidence classes used throughout:**
>
> | Class | Meaning |
> |---|---|
> | `A11_DEVICE` | Measured on a physical A11. |
> | `SIMULATOR` | Measured on the iOS Simulator (host GPU). |
> | `OTHER_GPU` | Measured on a GPU that is **not** an A11. The family name is stated inline. **Never** used to assert A11 behaviour. |
> | `REFERENCE_ONLY` | Read from Apple documentation (SDK header, Feature Set Tables, MSL spec). No device involved. |
> | `NOT_VERIFIED` | Could not be established from the SDK/docs; a runtime query is supplied instead. |
>
> **No number in this document is an A11 measurement.** No A11 device was
> available to the author of this file. Every A11-specific number is either
> (a) a **documented** value from Apple's *Metal Feature Set Tables* — Apple's
> own per-GPU-family table, which is authoritative for hardware limits — or
> (b) marked `NOT_VERIFIED` with a runtime query supplied.
>
> **SDK used for header citations**
> `iPhoneOS26.2.sdk` (`xcrun --sdk iphoneos --show-sdk-path`), Xcode 26.3
> (build 17C529), macOS 15.7.7 host. Header paths below are relative to
> `$SDK/System/Library/Frameworks/Metal.framework/Headers/`.
> Feature-set tables vendored at `docs/apple/Metal-Feature-Set-Tables.pdf`
> (dated *May 21, 2026*) and the 2017-09-12 edition at
> <https://metalbyexample.com/wp-content/uploads/Metal-Feature-Set-Tables-2017.pdf>
> (cited as **FST-2017**).

---

## 1. What "A11" means precisely

| Property | Value | Evidence |
|---|---|---|
| SoC | Apple A11 Bionic (iPhone 8, 8 Plus, X) | — |
| Metal GPU family | **`MTLGPUFamilyApple4`** (raw value `1004`) | FST p.2 "Metal GPUs" table: row `A11 Bionic | Metal | Apple4`. Header const `MTLDevice.h:194` `MTLGPUFamilyApple4 = 1004`. Apple docs: `MTLGPUFamily.apple4` — "Represents the Apple family 4 GPU features that correspond to the Apple A11 GPUs." <https://apple-docs.everest.mt/docs/metal/mtlgpufamily/apple4/> |
| Metal **programming model** reachable | **Metal 1/2/3 API surface**; the *Metal 3* programming model is documented as "the combined Metal 1, 2, and 3 API surface and is available on all Apple GPU families" | FST p.6 footnote 1 |
| Metal **3 hardware features** (mesh shaders, RT, …) | **Not available** — these are gated on Apple7+/Apple6+ hardware, not on the OS | FST p.4-5 |
| `MTLGPUFamilyMetal3` (`5001`) | Query exists (iOS 16.0+) but Apple's docs list Metal 3 as **"iOS: A14 and later"** — so `supportsFamily(.metal3)` is expected **false** on A11 | `MTLDevice.h:212`; <https://apple-docs.everest.mt/docs/metal/mtlgpufamily/metal3/> |
| Max OS on this hardware | iOS 16 (project deployment target `DEPLOY_TARGET=16.0`, `Makefile:29`) | `Makefile:29` |

### 1.1 Which family query to use on the device

`supportsFamily:` is the only modern query, and it is **iOS 13.0+**
(`MTLDevice.h:791`, `API_AVAILABLE(macos(10.15), ios(13.0))`).

* On **iOS 11/12** there is no `supportsFamily:`. The legacy query is
  `supportsFeatureSet:` (`MTLDevice.h:785`, deprecated in iOS 16), and the A11's
  feature set is **`MTLFeatureSet_iOS_GPUFamily4_v1`**
  (`MTLDevice.h:155`, `API_AVAILABLE(ios(11.0))`).
* Apple documents the fallback explicitly:
  "if the GPU Family API isn't available, test for features using feature sets…
  to test for the first release of Metal that supported Apple family 4 GPUs, use
  `MTLFeatureSet.iOS_GPUFamily4_v1`"
  — <https://developer.apple.com/documentation/metal/detecting-gpu-features-and-metal-software-versions.md>

> **Correction to `docs/METAL_A11_LIMITATIONS.md` §2.2 / `docs/OPENGL_46_METAL_MAPPING.md` §2.3.**
> Those documents state that dual-source blending "requires Metal 3 (Apple9/A14+)" and
> is therefore impossible on A11. **That is wrong**, and the FST-2017 table
> (which has a dedicated `iOS_GPUFamily4_v1` column) proves it:
> **"Dual-source blending" is ✓ in the `iOS_GPUFamily4_v1` column** (FST-2017
> p.2, *Feature Availability*, column `iOS 11 / GPU Family 4 / Version 1 /
> iOS_GPUFamily4_v1`).
> The current FST (p.3) also lists "Dual-source blending" with hardware family
> **Apple2** — i.e. from A8 upward — and the *Metal 3* column only records which
> programming model first exposed it.
> Corroborating SDK evidence: `MTLRenderPipeline.h:41-44` defines
> `MTLBlendFactorSource1Color`/`OneMinusSource1Color`/`Source1Alpha`/
> `OneMinusSource1Alpha` as `API_AVAILABLE(macos(10.12), ios(11.0))` — **iOS 11**,
> i.e. available on A11. The MSL spec (Table 5.7, `Metal-Shading-Language-Specification.pdf`
> p.151) documents `color(m), index(i)` as "All OS: Metal 1.2 and later", and the
> Metal Programming Guide documents the whole dual-source mechanism
> (`MTLBlendFactorSource1Color` referenced in the blend equation).
> **Consequence:** A11 supports GL dual-source blending; the mapping should be
> `[[color(0)]] index(1)` + `MTLBlendFactorSource1*`, **not** a second draw buffer.
> TGLMT currently maps `index=1` to `[[color(1)]]` (`src/gl/glsl_to_msl.cpp:12556`
> — `cidx = o.location + o.index`), which is the *wrong* semantics but is an
> **implementation gap, not a hardware limit**. See
> `docs/HARDWARE_LIMITATIONS.md` §3.

### 1.2 A note on the two Feature Set Tables

Apple has published two editions and they **disagree** on one important row. Both
are cited below; the disagreement is flagged rather than hidden.

| Row | FST-2017 (per iOS feature set) | FST-2026 (per GPU family) |
|---|---|---|
| "Dual-source blending" | ✓ on `iOS_GPUFamily4_v1` (A11) | hardware family **Apple2** |
| "Programmable blending" | ✓ on `iOS_GPUFamily4_v1` | hardware family **Apple2**, *Metal 3* column |
| "Border color" | ✓ only on macOS 10.12/10.13 columns | hardware family **Apple7** |
| "Multiple viewports" | ✓ only on macOS 10.13 | hardware family **Apple5** |
| "Texture argument table entries" | **31** for every iOS column | **96** for Apple4 |

For "Border color" and "Multiple viewports" the 2017 and 2026 tables agree that
A11 **does not** have them (the 2017 table has no ✓ in any `iOS_GPUFamily4_v1`
cell for those rows; the 2026 table puts them at Apple7/Apple5).
For "Dual-source blending" the 2026 table's *hardware* column (Apple2) is
consistent with the 2017 table's ✓.

---

## 2. Capability table

Legend for the **Min iOS** column: the minimum OS on which the *API* exists
(`API_AVAILABLE` in the iOS SDK). This is **not** the same as "available on
A11" — the two are separate columns on purpose.

**TGLMT uses it?** = the symbol appears in `src/metal/AppleMetalBridge.mm`
(grep-verified) or is otherwise exercised by the GL layer.

### 2.1 Device, family and program model

| Metal capability | Symbol used to query | Min iOS | On A11? | Evidence | TGLMT uses it? |
|---|---|---|---|---|---|
| GPU family query | `MTLDevice.supportsFamily:` | 13.0 | **Yes** (returns true for `.apple4`) | `MTLDevice.h:791` `API_AVAILABLE(macos(10.15), ios(13.0))`; FST p.2 (`A11 Bionic → Apple4`) | **Yes** — `AppleMetalBridge.mm:862, 1163, 1168, 1475, 1937, 2299` |
| Legacy feature-set query | `MTLDevice.supportsFeatureSet:` (`MTLFeatureSet_iOS_GPUFamily4_v1`) | 8.0 | Yes (A11's set) | `MTLDevice.h:155, 785`; Apple docs "Detecting GPU features…" | **No** — the bridge only uses `supportsFamily:` (`@available(macOS 10.15, iOS 13.0, *)` guards at `:861, :1162`) |
| Metal 3 hardware family | `MTLDevice.supportsFamily:MTLGPUFamilyMetal3` | 16.0 | **No** (A14+) | `MTLDevice.h:212`; Apple `MTLGPUFamily.metal3` page: "iOS \| A14 and later" | **No** — `supportsMesh()` (`:2296-2299`) checks `.apple7` instead |
| Device name / registry id | `MTLDevice.name`, `.registryID` | 8.0 | Yes | `MTLDevice.h:395-406` | **Yes** — `AppleMetalBridge.mm:1177-1180` |
| Unified memory | `MTLDevice.hasUnifiedMemory` | 13.0 | Yes (A11 has unified memory) | `MTLDevice.h:446` | **No** |
| Recommended working set | `MTLDevice.recommendedMaxWorkingSetSize` | 16.0 | Yes (value device-specific) | `MTLDevice.h:454` | **No** |
| Max buffer length | `MTLDevice.maxBufferLength` | 12.0 | Yes (value device-specific) | `MTLDevice.h:1041` | **No** — buffers are created with `MTLResourceStorageModeShared` (`:1206-1216`) |

### 2.2 Feature-set families (FST p.3-5): what A11 has

The FST "Metal feature availability by GPU family" table has two columns:
*Metal* (programming model) and *Apple* (hardware family). The hardware column
is the one that decides A11. "A11?" below = **is the hardware family ≤ Apple4?**

| Feature | FST hardware family | A11? | Notes / evidence | TGLMT uses it? |
|---|---|---|---|---|
| Programmable blending (framebuffer fetch) | Apple2 | **Yes** | FST-2017 p.2 shows ✓ on `iOS_GPUFamily4_v1`; FST-2026 p.3 hardware col = Apple2 | **No** — the bridge never enables a `[[color(0)]]` *input*; see `docs/METAL_FEATURE_MAPPING.md` §3.6 |
| Separate depth and stencil attachments | Apple2 | **Yes** | FST p.3 | **Yes** — `AppleTarget` keeps separate `depth_`/`stencil_` textures (`AppleMetalBridge.mm:770-771`) |
| PVRTC / EAC-ETC / ASTC pixel formats | Apple2 | **Yes** | FST p.3 | **Partly** — ASTC/ETC2/EAC mapped (`AppleMetalBridge.mm:121-144`); PVRTC **not** mapped |
| BC (S3TC/RGTC/BPTC) pixel formats | **Apple9** | **No** | FST p.3 + footnote 2: "As of Apple9 all GPUs have support"; runtime check `MTLDevice.supportsBCTextureCompression` (iOS 16.4+, `MTLDevice.h:510`) | **Yes** — `deviceSupportsBC()` at `AppleMetalBridge.mm:1449-1455`; returns false on A11 |
| Compressed volume texture formats | Apple3 | **Yes** | FST p.3 | No |
| Extended-range pixel formats | Apple3 | **Yes** | FST p.3 | No (`BGRA10_XR` not in `ToMTL`) |
| Wide-colour (P3) pixel format | Apple2 | **Yes** | FST p.3 | No |
| Depth-16 pixel format | Apple2 | **Yes** | FST p.3 | No (`ToMTL` has no `Depth16Unorm`) |
| Linear textures (texture buffers) | Apple2 | **Yes** | FST p.3; `MTLTextureTypeTextureBuffer` is iOS 12.0 (`MTLTexture.h:34`) | **No** — `ToMTL`/texture creation has no texture-buffer path |
| MSAA depth resolve | Apple3 | **Yes** | FST p.3 | **Yes** — `makeMSAATarget` creates `MTLTextureType2DMultisample` depth (`:2224-2230`) |
| Array of textures (read) | Apple3 | **Yes** | FST p.3 | **Yes** — `MTLTextureType2DArray` (`:1595`) |
| Array of textures (**write**) | **Apple6** | **No** | FST p.3 | No |
| Cube-map texture arrays | Apple4 | **Yes** (exactly at the floor) | FST p.3; FST-2017 p.2 shows ✓ **only** on `iOS_GPUFamily4_v1` and macOS | No (`MTLTextureTypeCubeArray` is iOS 11.0, `MTLTexture.h:31`, but unused) |
| Stencil texture views | Apple2 | **Yes** | FST p.3 | No |
| Array of samplers | Apple3 | **Yes** | FST p.3 | **Yes** — `setFragmentSamplerState:atIndex:` (`:967-971`) |
| Sampler maximum anisotropy | Apple2 | **Yes** | FST p.3 | **Yes** — `d.maxAnisotropy` (`:2935`) |
| Sampler LOD clamp | Apple2 | **Yes** | FST p.3 | **Yes** — `d.lodMinClamp/lodMaxClamp` (`:2939-2941`) |
| Sampler **LOD bias** | **Apple10** | **No** | FST p.5 ("Sampler LOD bias \| Metal 3 & 4 \| Apple10"); the SDK exposes `MTLSamplerDescriptor.lodBias` only from **iOS 26.0** (`MTLSampler.h:204`) | **No** — `GL_TEXTURE_LOD_BIAS` is parsed in `src/gl/gl_texture.cpp` but never reaches Metal |
| `MTLSamplerState` comparison functions (shadow) | Apple3 | **Yes** | FST p.3; `MTLSampler.h:210` (iOS 9.0) | **Yes** — `d.compareFunction` (`:2930`) |
| 16-bit unsigned integer coordinates | Apple2 | **Yes** | FST p.3 | No |
| **Border color** (clamp-to-border sampling) | **Apple7** | **No** | FST p.3 (hardware family Apple7); FST-2017 p.2 shows ✓ **only** on macOS 10.12/10.13, **not** on any iOS column | **Partly — and this is a latent bug**: `ToMTLWrap` maps `GL_CLAMP_TO_BORDER` → `MTLSamplerAddressModeClampToBorderColor` unconditionally (`AppleMetalBridge.mm:218`) and `d.borderColor` is set at `:2945`. `MTLSamplerAddressModeClampToBorderColor` is `API_AVAILABLE(macos(10.12), ios(14.0))` (`MTLSampler.h:70`) — **the API exists on iOS 14+ but the A11 hardware lacks the feature** (FST: Apple7). See `docs/HARDWARE_LIMITATIONS.md` §2. |
| Counting occlusion query (`MTLVisibilityResultModeCounting`) | Apple3 | **Yes** | FST p.3; `MTLRenderCommandEncoder.h:41` (iOS 9.0) | **Yes** — `visCounting_` = `supportsFamily:MTLGPUFamilyApple3` (`:862`, `:1163`) |
| Base vertex / instance drawing | Apple3 | **Yes** | FST p.3; `MTLRenderCommandEncoder.h:768` (iOS 9.0) | **Partly** — `drawIndexedPrimitives:...baseVertex:baseInstance:` (`:917-919`) but `baseVertex` is hard-coded `0`; the GL layer rewrites indices on the CPU instead |
| **Layered rendering** (render to array/cube layer) | **Apple5** | **No** | FST p.3 (hardware family Apple5); FST-2017 p.2 shows ✓ only on macOS 10.12/10.13 | **No** — the bridge attaches `MTLTextureType2DArray` slices (`AppleMetalBridge.mm:790-794`) but the *layered* (per-primitive layer index) form is unavailable on A11 |
| Memoryless render targets | Apple2 | **Yes** | FST p.3; `MTLStorageModeMemoryless` | No |
| **Dual-source blending** | **Apple2** | **Yes** | FST p.3 (hardware family Apple2); FST-2017 p.2 ✓ on `iOS_GPUFamily4_v1`; `MTLRenderPipeline.h:41-44` (iOS 11.0) | **Wrongly mapped** — see §1.1 |
| Combined MSAA store-and-resolve | Apple3 | **Yes** | FST p.3 | **Yes** — `MTLStoreActionMultisampleResolve` used for MSAA targets |
| MSAA blits | Apple2 | **Yes** | FST p.3 | **Yes** — `MTLBlitCommandEncoder` copy paths (`:2340`, `:2375`) |
| Programmable sample positions | Apple2 | **Yes** | FST p.3; `MTLDevice.areProgrammableSamplePositionsSupported` iOS 11.0 (`MTLDevice.h:862`); `MTLRenderPassDescriptor.setSamplePositions:count:` iOS 11.0 | **Yes** — `ApplySamplePositions` (`:821-826`), queried at `:1200-1204` |
| Deferred store action | Apple2 | **Yes** | FST p.3 | **Yes** — `MTLStoreActionUnknown` not used, but store actions are set per-pass |
| Memory barriers (Metal 3 style) | Apple3 | **Yes (limited)** | FST p.3 + **footnote 3**: "GPU devices in Apple3 through Apple10 families *don't* support memory barriers that include the `MTLRenderStages.fragment` or `.tile` stages in the `after` argument, or `MTLBarrierScope.renderTargets` in the scope argument" | **No** — TGLMT relies on encoder-boundary ordering (see `docs/METAL_A11_LIMITATIONS.md` §2.8) |
| Indirect command buffers (rendering / compute) | Apple3 | **Yes** | FST p.3; `MTLDevice.newIndirectCommandBufferWithDescriptor:` iOS 12.0 (`MTLDevice.h:904`) | **No** — indirect draws are read back on the CPU (`src/gl/gl_draw.cpp:390-546`) |
| Tessellation | Apple3 | **Yes** | FST p.3; `MTLRenderCommandEncoder.drawPatches:` iOS 10.0 (`MTLRenderCommandEncoder.h:817`) | **Yes** — `makeTessPipeline` + `drawPatches` (`AppleMetalBridge.mm:1882-1952`, `:927-931`) |
| Indirect tessellation arguments | **Apple5** | **No** | FST p.3 | No |
| Argument buffers **tier 1** | Apple2 | **Yes** | FST p.3; `MTLDevice.argumentBuffersSupport` iOS 11.0 (`MTLDevice.h:475`) | **No** — the bridge binds resources by index |
| Argument buffers **tier 2** | **Apple6** | **No** | FST p.3 | No |
| Read/write buffers in functions | Apple3 | **Yes** | FST p.4 | **Yes** — `MTLResourceStorageModeShared` buffers bound to compute (`AppleComputeEncoder`) |
| Read/write textures in functions | Apple4 | **Yes** (exactly at the floor) | FST p.4; FST-2017 p.2 ✓ **only** on `iOS_GPUFamily4_v1` | **Partly** — textures get `MTLTextureUsageShaderWrite` (`:1423`, `:1574`) |
| **Imageblocks / tile shaders / raster order groups / quad permute / non-uniform threadgroup size / post-depth coverage** | **Apple4** | **Yes** (exactly at the floor) | FST p.4; FST-2017 p.2 shows these ✓ **only** on `iOS_GPUFamily4_v1` | **No** — TGLMT uses none of them |
| **Multiple viewports** | **Apple5** | **No** | FST p.3 (hardware family Apple5); FST-2017 p.2 ✓ only on macOS 10.13; FST p.7 limits row "Maximum number of viewports and scissor rectangles, per vertex function": **Apple2/3/4 = 1**, Apple5+ = 16 | **No** — the bridge calls the single `setViewport:` (`:873-877`) |
| **Border color** | **Apple7** | **No** | (see above) | — |
| Sparse textures | **Apple6** | **No** | FST p.4 | No |
| Variable rasterization rate | **Apple6** | **No** | FST p.4 | No |
| Vertex amplification | **Apple6** | **No** | FST p.4 | No |
| 64-bit integer math (`long`/`ulong`) | Apple3 | **Yes** | FST p.4; MSL spec p.25: `long`/`ulong` are "All OS: Metal 2.2 and later" | **Yes** — `gl_ext_uniform64.cpp` exists; MSL `long` type is emitted |
| 64-bit atomics | **Apple9** | **No** | FST p.5 | No |
| Floating-point atomics | **Apple7** | **No** | FST p.5 | No |
| **Texture atomics** | **Apple6** | **No** | FST p.5; the SDK's `MTLTextureUsageShaderAtomic` is **iOS 17.0** (`MTLTexture.h:104`) — beyond the A11's max OS anyway | No |
| Ray tracing (compute / render) | **Apple6** | **No** | FST p.5; `MTLDevice.supportsRaytracing` iOS 14.0 (`MTLDevice.h:1145`) | No |
| Function pointers | **Apple6** | **No** | FST p.5 | No |
| Barycentric coordinates / primitive ID | **Apple7** | **No** | FST p.5 | No |
| **Mesh shading** | **Apple7** | **No** | FST p.5; `MTLDevice.newRenderPipelineStateWithMeshDescriptor:` iOS 16.0 (`MTLDevice.h:830`) | `supportsMesh()` returns `supportsFamily:MTLGPUFamilyApple7` → false on A11 (`:2296-2299`) |
| MetalFX temporal upscaling / frame interpolation | Apple7 / Apple5 | No / No | FST p.5 | No |
| Query texture LOD | **Apple8** | **No** | FST p.5 + footnote 8; runtime `MTLDevice.supportsQueryTextureLOD` iOS 14.0 (`MTLDevice.h:503`) | No |
| Lossy texture compression | **Apple8** | **No** | FST p.4 | No |
| Render/compute dynamic libraries | **Apple6** | **No** | FST p.4 | No |
| Depth sample compare bias and gradient | Apple2 | **Yes** | FST p.5 | No |
| Non-private depth/stencil textures | Apple2 | **Yes** | FST p.5 | **Yes** — depth textures use `MTLStorageModePrivate` (`:1429`, `:1430`) |
| Dynamic stride for attribute buffers | Apple4 | **Yes** (at the floor) | FST p.5 | No |
| `MTLAttributeFormat.floatRGB9E5` / `.floatRG11B10` | Apple5 | **No** | FST p.5 | No |
| Relaxed math | Apple4 | **Yes** (at the floor) | FST p.5 | No |
| Metal 4 features (argument tables, command allocators, command barriers, decoupled queues, tensors, ML encoding, performance counter heaps) | **Metal 4 / Apple7+** | **No** | FST p.5-6 | No |

### 2.3 Rasterisation & pipeline state

| Metal capability | Symbol | Min iOS | On A11? | Evidence | TGLMT uses it? |
|---|---|---|---|---|---|
| Line width | *(no API exists)* | — | **No — no API at all** | `grep -rn "lineWidth\|LineWidth" $SDK/.../Metal.framework/Headers/` → **0 hits**. `MTLRenderPipelineDescriptor` has **no** line-width property (verified by reading the full property list, `MTLRenderPipeline.h:179-307`). | **No** — `glLineWidth` clamped; `GL_ALIASED_LINE_WIDTH_RANGE` reported `(1,1)` (`src/gl/gl_get.cpp:353`) |
| Point size | `[[point_size]]` MSL vertex output; device max **511** | Metal 1 | **Yes, up to 511 px** | MSL spec Table 5.4 (p.143): `point_size float — "Size of a point primitive"`; FST p.8 "Maximum size of a point primitive" = 511 for every family including Apple4 | **Partly** — MSL emits `[[point_size]]` in the TCS/TES paths (`src/gl/glsl_to_msl.cpp:4717, 4774`); the raster path does not plumb `GL_POINT_SIZE` |
| **Point-size limit in the pipeline descriptor** | `MTLRenderPipelineDescriptor.maximumPointSize` | — | **Does not exist** | `grep -rn "maximumPointSize" $SDK/.../Metal.framework/` → **0 hits** in the whole framework. The 511 limit is a *device* limit from FST p.8, not a descriptor property. *(This corrects the assumption in the task brief.)* | — |
| Alpha-to-coverage | `MTLRenderPipelineDescriptor.alphaToCoverageEnabled` | 8.0 | **Yes** | `MTLRenderPipeline.h:192` (`API_AVAILABLE(macos(10.11), ios(8.0))`) | **No** — `grep alphaToCoverage src/metal/AppleMetalBridge.mm` → 0 hits |
| Alpha-to-one | `MTLRenderPipelineDescriptor.alphaToOneEnabled` | 8.0 | **Yes** | `MTLRenderPipeline.h:193` | No |
| Programmable sample positions | `MTLRenderPassDescriptor.setSamplePositions:count:` | 11.0 | **Yes** | `MTLRenderPass.h` (`setSamplePositions`, iOS 11.0) | **Yes** — `ApplySamplePositions` (`:821-826`) |
| Sample mask (fixed-function) | *MSL* `[[sample_mask]]` fragment **output** (Metal 1+) | Metal 1 | **Yes as a shader output** | MSL spec Table 5.7 (p.151): `sample_mask uint "Coverage mask"` — "All OS: Metal 1 and later"; `[[sample_id]]` and `[[sample_mask]]` are fragment *inputs* (Table 5.5, p.146) | **Partly** — TGLMT *emulates* GL `glSampleMaski` by injecting `[[sample_id]]` + `discard_fragment()` (`src/gl/gl_program.cpp:210-236`) instead of emitting a real `[[sample_mask]]` output |
| Sample count | `MTLDevice.supportsTextureSampleCount:` | 9.0 | **Yes, 1/2/4** | `MTLDevice.h:798`; FST p.9 "Maximum sample count in render passes with MSAA" = **4** for Apple4 | **Yes** — `MAX_SAMPLES` = 4 (`src/gl/gl_get.cpp:165`); `d.rasterSampleCount` (`:1914`, `:2286`) |
| MSAA depth resolve | `MTLMultisampleDepthResolveFilter` | 9.0 | **Yes** (Apple3+) | `MTLRenderPass.h:179`; FST p.3 | No explicit resolve filter set |
| Depth bias (polygon offset) | `setDepthBias:slopeScale:clamp:` | 8.0 | **Yes** | `MTLRenderCommandEncoder.h:320` | **Yes** — `:1014-1020`; GL mapping in `src/gl/gl_apple_draw.cpp:2771-2796` |
| Depth clamp | `setDepthClipMode:` (`MTLDepthClipModeClamp`) | 11.0 | **Yes** | `MTLRenderCommandEncoder.h:314` (`API_AVAILABLE(macos(10.11), ios(11.0))`); enum at `:63-66` | **Yes** — `:1005-1009` |
| Stencil | `MTLDepthStencilState` + `setStencilReferenceValue:` / `setStencilFrontReferenceValue:backReferenceValue:` | 8.0 / 9.0 | **Yes** | `MTLDepthStencil.h:35-71`; `MTLRenderCommandEncoder.h:462, 468` | **Yes** — but only the *front* reference is used (`:957-960`, comment: "Metal 1 ref cho cả 2 mặt") |
| Polygon mode LINE | `setTriangleFillMode:` (`MTLTriangleFillModeLines`) | 8.0 | **Yes** (but it outlines *triangles*, not polygons) | `MTLRenderCommandEncoder.h:351`; enum `:68-71` | **Yes** — `:942-945`; used at `src/gl/gl_apple_draw.cpp:2715` |
| Viewport / scissor | `setViewport:`, `setScissorRect:` | 8.0 | **Yes — exactly one** | `MTLRenderCommandEncoder.h:281, 338`; FST p.7 limits row "Maximum number of viewports and scissor rectangles, per vertex function": **Apple4 = 1** (FST-2017 p.3 same row: `iOS_GPUFamily4_v1` = 1) | **One only** — `setViewport:` (`:873-877`), `setScissorRect:` (`:946-949`) |
| Winding / cull | `setFrontFacingWinding:`, `setCullMode:` | 8.0 | **Yes** | `MTLRenderCommandEncoder.h:293, 308` | **Yes** — `:995-1004` |
| Blend equations | `MTLBlendOperation` {Add, Subtract, ReverseSubtract, Min, Max} | 8.0 | **Yes** — arithmetic only | `MTLRenderPipeline.h:53-64`; `ToMTLBlendOp` at `AppleMetalBridge.mm:177-186` | **Yes** — `:177-186` |
| Blend factors incl. `Source1*` | `MTLBlendFactorSource1Color` etc. | 11.0 | **Yes** | `MTLRenderPipeline.h:41-44` | **Yes** — `ToMTLBlend` `:170-173` (but the *shader* side doesn't emit `index(1)`, see §1.1) |
| Logic op | *no blend operation* | — | **No** | `MTLBlendOperation` has no bitwise op (`MTLRenderPipeline.h:53-64`); MSL has no logic-op builtin | **No** — `GL_COLOR_LOGIC_OP` bypasses the blend stage (`src/gl/gl_apple_draw.cpp:2320-2322`) and the operation is **not performed** |
| Primitive types | `MTLPrimitiveType` {Point, Line, LineStrip, Triangle, TriangleStrip} | 8.0 | **Yes — five only** | `MTLRenderCommandEncoder.h:30-36` | **Yes** — `ToMTLPrim` (`:240-251`); fan/loop expanded on the CPU (`src/gl/gl_apple_draw.cpp:1927-2000`) |
| Primitive restart | *no API* | — | **No** | No `MTLPrimitiveRestartIndex`/restart-index in the SDK headers | **No** — `glPrimitiveRestartIndex` stores the value (`src/gl/gl_draw.cpp:575`) but no restart handling on the GPU path |

### 2.4 Resources, formats and bindings

| Metal capability | Symbol | Min iOS | On A11? | Evidence | TGLMT uses it? |
|---|---|---|---|---|---|
| Max 2D texture width/height | FST limits | — | **16 384 px** | FST p.8 "Maximum 2D texture width and height": Apple4 = 16 384 px; FST-2017 p.3 `iOS_GPUFamily4_v1` = 16 384 px (identical) | **Yes** — hard-coded `maxD = 16384` (`AppleMetalBridge.mm:1411`, `:1500`) — matches, but by constant, not by query |
| Max 1D texture width | FST limits | — | **16 384 px** | FST p.8 | **No 1D texture path at all** |
| Max cube-map size | FST limits | — | **16 384 px** | FST p.8 | No cube path in the bridge |
| Max 3D texture | FST limits | — | **2 048 px** | FST p.8; FST-2017 p.3 `iOS_GPUFamily4_v1` = 2048 px | **Yes** — `maxD = 2048` (`:1552`) — matches by constant |
| Max array layers | FST limits | — | **2 048** | FST p.8; FST-2017 p.3 = 2048 | **Yes** — no runtime check in `newTexture2DArray` (`:1591`) |
| Max colour render targets / pass | FST limits | — | **8** | FST p.8; FST-2017 p.3 `iOS_GPUFamily4_v1` = 8 | **Yes** — 8 slots (`AppleMetalBridge.mm:765-766`, `:1998`) |
| Max texture buffer width | FST limits | — | **256 M px** | FST p.8 (Apple3+) | No |
| Max total MRT size per pixel | FST limits | — | **512 bits** | FST-2017 p.3 `iOS_GPUFamily4_v1` = 512 bits (vs 256 bits on Apple2/3) | No |
| Depth/stencil formats | `MTLPixelFormatDepth32Float`, `Depth32Float_Stencil8`, `Stencil8`, `X32_Stencil8` | 9.0 / 10.0 | **Yes** | `MTLPixelFormat.h:211, 215, 220, 222` | **Yes** — `ToMTL` maps `Depth24Stencil8 → MTLPixelFormatDepth32Float_Stencil8` (`:105`) |
| **`MTLPixelFormatDepth24Unorm_Stencil8`** | — | — | **`API_UNAVAILABLE(ios)`** | `MTLPixelFormat.h:219`: `MTLPixelFormatDepth24Unorm_Stencil8 API_AVAILABLE(macos(10.11), macCatalyst(13.0)) API_UNAVAILABLE(ios) = 255` | **Correctly avoided** — the bridge maps D24S8 → D32F+S8 (`:105`) |
| `MTLDevice.depth24Stencil8PixelFormatSupported` | — | — | **Unavailable on iOS entirely** | `MTLDevice.h:461` `API_UNAVAILABLE(ios)` | n/a |
| sRGB formats | `MTLPixelFormatRGBA8Unorm_sRGB`, `BGRA8Unorm_sRGB`, `R8Unorm_sRGB`, `RG8Unorm_sRGB` | 8.0 | **Yes** | `MTLPixelFormat.h:61, 66, 32`; FST p.12/13 texture-capability tables list `RGBA8Unorm_sRGB` as "All" for every family | **Yes** — `ToMTL` (`:80`, `:83`) |
| 16-bit normalised formats | `R16Unorm/Snorm`, `RG16Unorm/Snorm`, `RGBA16Unorm/Snorm` | 8.0 | **Yes — All capabilities** | FST p.12/13: `R16Unorm/R16Snorm` = "All" from Apple2; `RGBA16Unorm/RGBA16Snorm` = "All" | **Partly** — `ToMTL` has `R16Float/R16Uint/R16Sint` but **no** `R16Unorm/RG16Unorm/RGBA16Unorm` |
| Float formats | `R16Float`, `RG16Float`, `RGBA16Float`, `R32Float`, `RGBA32Float` | 8.0 | **Yes** | FST p.12/13 (all "All" for `RGBA16Float`, `RG16Float`; `R32Float`/`RGBA32Float` = Write+Color+MSAA+Blend) | **Yes** — `ToMTL` (`:91-94`) |
| Integer formats | `R32Uint/R32Sint`, `RGBA8Uint/Sint`, `R16Uint/Sint`, `RG16Uint/Sint`, `RGBA16/32Uint/Sint` | 8.0 | **Yes** (Color+MSAA from Apple5 for some) | FST p.12/13 | **Partly** — `ToMTL` covers `R32Sint/R32Uint`, `R8Sint/R8Uint`, `R16Sint/R16Uint` |
| **Luminance / intensity / alpha legacy formats** | `MTLPixelFormatA8Unorm` only | 8.0 | **Only A8 exists** | `MTLPixelFormat.h` has `A8Unorm`; no `Luminance`/`Intensity`/`Alpha` | **No** — GL legacy luminance/intensity/alpha need shader-side expansion |
| Packed formats | `RGB10A2Unorm`, `BGR10A2Unorm`, `RGB10A2Uint`, `RG11B10Float`, `RGB9E5Float` | 8.0 | **Yes** (`RG11B10Float`/`RGB9E5Float` from Apple5 for *vertex* formats; as *pixel* formats they are "Filter/Color/MSAA/Resolve/Blend" from Apple2) | FST p.13 | **Partly** — only `RG11B10Float` mapped (`:91`) |
| ASTC | `MTLPixelFormatASTC_*_LDR` | 8.0 | **Yes** (Filter) | FST p.3 (Apple2); FST p.14 compressed-format table: ASTC = "Filter" for every family | **Yes** — `ToMTL` (`:131-144`) |
| ETC2 / EAC | `MTLPixelFormatETC2_*`, `EAC_*` | 8.0 | **Yes** (Filter) | FST p.3; FST p.14 | **Yes** — `ToMTL` (`:121-130`) |
| PVRTC | `MTLPixelFormatPVRTC_*` | 8.0 | **Yes (Filter)** — but deprecated in iOS 18 | `MTLPixelFormat.h:125-132` `API_DEPRECATED(..., ios(8.0, 18.0))`; FST p.3 | **No** |
| BC1–BC7 (S3TC/RGTC/BPTC) | `MTLPixelFormatBC*` | 16.4 (runtime) | **No** | FST p.3 (Apple9) + footnote 2; `MTLDevice.supportsBCTextureCompression` iOS 16.4 (`MTLDevice.h:510`) | **Yes — correctly gated** — `deviceSupportsBC()` (`:1449-1455`) returns false on A11 → `newCompressedTexture` returns `nullptr` (`:1502-1508`) |
| HDR ASTC | `MTLPixelFormatASTC_*_HDR` | — | **No** (Apple5+) | FST p.14 "HDR ASTC pixel formats: Not available … Not available … Not available" (Apple2/3/4) | No |
| Texture views | `newTextureViewWithPixelFormat:textureType:levels:slices:` | 9.0 | **Yes** | `MTLTexture.h:512` (`API_AVAILABLE(macos(10.11), ios(9.0))`) | **Yes** — `AppleTexture::swizzledView` / `textureView` (`:335`, `:380`, `:418`) |
| Texture views **with swizzle** | `newTextureViewWithPixelFormat:textureType:levels:slices:swizzle:` | 13.0 | **Yes** | `MTLTexture.h:538` (`API_AVAILABLE(macos(10.15), ios(13.0))`) | **Yes** — `:333-336` uses `MTLTextureSwizzleChannels` |
| Cube-map texture arrays | `MTLTextureTypeCubeArray` | 11.0 | **Yes** (Apple4 exactly) | `MTLTexture.h:31`; FST p.3 | No |
| Multisample texture arrays | `MTLTextureType2DMultisampleArray` | 14.0 | **No** (Apple7 for *layered* MSAA) | `MTLTexture.h:33`; FST p.3 "Layered rendering to multisample textures: Apple7" | **Yes** — `makeMSAATargetLayered` uses it (`:2248`) → **will fail validation on A11** |
| Texture buffers | `MTLTextureTypeTextureBuffer` | 12.0 | **Yes** | `MTLTexture.h:34`; FST p.3 "Linear textures: Apple2" | No |
| Texture atomics | `MTLTextureUsageShaderAtomic` | **17.0** | **No** (and beyond A11's max OS) | `MTLTexture.h:104` | No |
| Storage modes | `MTLStorageModeShared` / `Private` / `Memoryless` | 8.0 | Yes | `MTLStorageMode` in `MTLTypes.h` | **Yes** — `DefaultTextureStorage()` (`:69-75`); depth is always Private (`:1429`, `:1567`, `:1577`, `:1604`, `:1632`, `:1650`, `:2189`) |
| Max buffers per stage (argument table) | FST limits | — | **31** | FST p.7 "Maximum number of entries in the buffer argument table, per graphics or kernel function": Apple2–Apple10 = **31**; FST-2017 p.3 same row = 31 for `iOS_GPUFamily4_v1` | **Yes, but TGLMT over-reports** — `GL_MAX_UNIFORM_BUFFER_BINDINGS` = **36** (`src/gl/gl_get.cpp:183`) > 31. The *effective* budget is smaller still: the converter allocates graphics blocks from slot 17 and **refuses to compile** past slot 30 (`src/gl/glsl_to_msl.cpp:10736`, `:10694`) ⇒ **14 UBO+SSBO blocks per graphics stage**. That failure is a clean `GL_COMPILE_STATUS = false` with a diagnostic, **not** a runtime Metal abort — see `docs/HARDWARE_LIMITATIONS.md` L16. |
| Max textures per stage (argument table) | FST limits | — | **31 (2017 table) / 96 (2026 table)** — **disagreement; see §2.5** | FST-2017 p.3 `iOS_GPUFamily4_v1` = 31; FST p.7 Apple4 = 96 | **TGLMT reports `GL_MAX_TEXTURE_IMAGE_UNITS` = 32** (`src/gl/gl_get.cpp:159`) — over 31 if the 2017 figure applies, under 96 if the 2026 figure applies. **Must be resolved on device.** |
| Max samplers per stage (argument table) | FST limits | — | **16** | FST p.7: 16 for every family; FST-2017 p.3 `iOS_GPUFamily4_v1` = 16. Footnote 3: inline `constexpr` samplers count toward the limit | **Yes** — `kBindSlots = 32` cache is a *cache* size, not a limit; the sampler limit is 16 |
| Max vertex attributes per descriptor | FST limits | — | **31** | FST p.7; FST-2017 p.3 = 31 | **Yes** — `MTLVertexDescriptor` (`:1839-1844`) |
| Max constant-buffer args per stage | FST limits | — | **31** | FST p.7 | Yes |
| Max `[[stage_in]]` scalars/vectors per fragment fn | FST limits | — | **60 (2017) / 124 (2026)** — disagreement, see §2.5 | FST-2017 p.3 `iOS_GPUFamily4_v1` = 60; FST p.7 Apple4 = 124 | No explicit check |
| Max function constants | FST limits | — | **65 536** | FST p.7; FST-2017 p.3 `iOS_GPUFamily4_v1` = 65 536 | No |
| Max threads per threadgroup | FST limits / `MTLComputePipelineState.maxTotalThreadsPerThreadgroup` | 12.0 (runtime property) | **1 024** | FST p.7 Apple4 = 1 024; FST-2017 p.3 `iOS_GPUFamily4_v1` = **1024** (vs 512 on A9/A10) — footnote 5: "check the actual maximum by inspecting `MTLComputePipelineState.maxTotalThreadsPerThreadgroup`" | **Reported but not enforced** — `GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS` = 1024 (`src/gl/gl_get.cpp:198`) |
| Max threadgroup memory | FST limits / `MTLDevice.maxThreadgroupMemoryLength` | 11.0 | **32 KB** | FST p.7 Apple4 = 32 KB; FST-2017 p.3 `iOS_GPUFamily4_v1` = 32 KB; `MTLDevice.h:847` `API_AVAILABLE(macos(10.13), ios(11.0))` | **Reported, not enforced** — `GL_MAX_COMPUTE_SHARED_MEMORY_SIZE` = 32768 (`src/gl/gl_get.cpp:193`) |
| Max tile memory (A11-specific) | FST-2017 | — | **32 KB** | FST-2017 p.3 "Maximum total tile memory allocation": `iOS_GPUFamily4_v1` = 32 KB; every other iOS column = "Not accessible" | No |
| Max threadgroups per grid | FST-2026 "Maximum threadgroups per mesh shader grid" = "Not available" for Apple4 (mesh only) | — | **NOT_VERIFIED** | No per-kernel threadgroup-grid limit is published for Apple4 in FST p.9 | `GL_MAX_COMPUTE_WORK_GROUP_COUNT` = 65535 is hard-coded (`src/gl/gl_get.cpp:70-72`) — **not verified** |
| Minimum constant-buffer offset alignment | FST limits | — | **4 B** | FST p.8 | No |
| Minimum stride/attribute-offset alignment | FST limits | — | **4 B** | FST p.7 (Apple4 = 4 B; Apple5+ = 1 B) | No |
| Tile size (no MSAA / 2× / 4×) | FST limits | — | **32×32 / 32×32 / 32×16** | FST p.9 | No |
| Max visibility-query offset | FST limits | — | **65 528 B** | FST p.9; FST-2017 p.3 = 65 528 B | **Yes** — the bridge allocates a 2 KB visibility buffer (`:1223`) with 256 slots → max offset 2 040 B, well inside the limit |
| Max number of I/O commands per buffer | FST limits | — | **8 192** | FST p.9 | No |
| Max fences | FST limits | — | **32 768** | FST p.9 | No |
| Max sample buffers / counter sample buffer length | FST limits | — | **32 / 32 KB** | FST p.8 | No |

### 2.5 The numbers that must be measured on the device

Two FST rows disagree between the 2017 and 2026 editions, and the A11 column is
where they disagree:

| Limit | FST-2017 (`iOS_GPUFamily4_v1`) | FST-2026 (Apple4) | Why it matters |
|---|---|---|---|
| Max entries in the **texture argument table** per graphics/kernel function | **31** | **96** | TGLMT reports `GL_MAX_TEXTURE_IMAGE_UNITS` = **32**. If the 2017 figure is the one the A11 enforces, a 32-texture fragment shader exceeds Metal's table. `docs/METAL_A11_LIMITATIONS.md` §3 asserts exactly this ("Metal tier-1 table of 31") — **but the 2026 table says 96 for Apple4**, so the assertion is not settled by the documentation. |
| Max `[[stage_in]]` inputs (scalars/vectors) **and** input components per fragment function | **60** | **124** | A GLSL fragment shader with >60 varyings (or >60 components) would fail to link on A11 if the 2017 figure applies. |

**Important measurement detail discovered while writing this document** (and
therefore part of the probe design): Metal enforces the fragment-stage limit
as **two independent limits**, which the compiler reports with different
messages:

* `"number of shader varyings (N) exceeds limit (32)"` — a **count** limit on
  the number of individual interpolants.
* `"number of shader varying components (N) exceeds limit (124)"` — a
  **component** limit on the total scalar/vector width.

The FST publishes both ("Maximum number of inputs (scalars or vectors) to a
fragment function" and "Maximum number of input components to a fragment
function"). A probe that only varies the *count* measures the count limit; a
probe that varies the *width* measures the component limit. The probe in §4
therefore measures **both** separately (`max_fragment_stage_in_scalars` with
1-component varyings, `max_fragment_stage_in_float4s` with 4-component
varyings), and the vertex function writes constants so that it consumes no
vertex attributes and does not disturb the vertex-attribute limit.

These **cannot** be resolved from documentation. They require the runtime probe
in §4. Until then they are **`NOT_VERIFIED`** and the corresponding GL limit
queries (`GL_MAX_TEXTURE_IMAGE_UNITS`, `GL_MAX_VARYING_COMPONENTS`,
`GL_MAX_VARYING_VECTORS`) must be treated as unproven.

### 2.6 Compute, sync and barriers

| Metal capability | Symbol | Min iOS | On A11? | Evidence | TGLMT uses it? |
|---|---|---|---|---|---|
| Compute dispatch | `dispatchThreadgroups:threadsPerThreadgroup:` | 8.0 | Yes | `MTLComputeCommandEncoder.h` | **Yes** — `AppleComputeEncoder` (`:556`) |
| Compute dispatch (thread grid) | `dispatchThreads:threadsPerThreadgroup:` | 11.0 | Yes | `MTLComputeCommandEncoder.h:243` | No |
| Indirect compute dispatch | `dispatchThreadgroupsWithIndirectBuffer:` | 9.0 | Yes | `MTLComputeCommandEncoder.h` | **Yes** — `:568-569` (GL `glDispatchComputeIndirect`) |
| Compute pipeline (descriptor form) | `newComputePipelineStateWithDescriptor:options:reflection:error:` | 9.0 | Yes | `MTLDevice.h:767` | **Partly** — the bridge uses the *function* form (`newComputePipelineStateWithFunction:`, `MTLDevice.h:743`) at `:2580` |
| `MTLComputePipelineDescriptor.supportIndirectCommandBuffers` | 13.0 | Yes | `MTLComputePipeline.h:73` | No |
| Command buffer event | `MTLDevice.newEvent` / `newSharedEvent` | 12.0 | **Yes** | `MTLDevice.h:910, 916` (`API_AVAILABLE(macos(10.14), ios(12.0))`) | **No** — `glFenceSync`/`glClientWaitSync` are not implemented on this path (`src/gl/gl_sync_query.cpp`) |
| Fence | `MTLDevice.newFence`, `updateFence:`, `waitForFence:` | 10.0 | Yes | `MTLDevice.h:779`; `MTLRenderCommandEncoder.h:803, 811` | No |
| Resource-state barrier encoder | `MTLResourceStateCommandEncoder` | 13.0 | **Yes (limited — see footnote 3)** | `MTLCommandBuffer.h:424`; FST p.3 footnote 3 | No |
| **Metal 4 command barriers / `MTLAccessor`** | `MTLBarrierEncoder` | 26.0 | **No** | `grep -rn "MTLAccessor" $SDK/.../Headers/` → **0 hits**; `MTLCommandEncoder.h:125` `barrierAfterQueueStages:` is iOS 26.0 | No |
| GPU timestamps | `MTLCommandBuffer.GPUStartTime` / `.GPUEndTime` | 10.3 | Yes | `MTLCommandBuffer.h:289, 294` | **Yes** — `:1346-1349` |
| `MTLDevice.counterSets` / performance counters | 14.0 | Yes (API) but "Not available" for Apple4 in FST p.9 | `MTLDevice.h:1047`; FST p.9 "Maximum performance counter heaps (per process): Not available" for Apple4 | No |

### 2.7 Point, line and primitive coverage summary

| GL primitive | Metal primitive | Primitive count after expansion | TGLMT status |
|---|---|---|---|
| `GL_POINTS` | `MTLPrimitiveTypePoint` | 1:1 | Direct |
| `GL_LINES` | `MTLPrimitiveTypeLine` | 1:1 | Direct |
| `GL_LINE_STRIP` | `MTLPrimitiveTypeLineStrip` | 1:1 | Direct |
| `GL_LINE_LOOP` | `LineStrip` + re-close | n → n+1 vertices | **CPU expansion** (`src/gl/gl_apple_draw.cpp:1986-2004`) |
| `GL_TRIANGLES` | `MTLPrimitiveTypeTriangle` | 1:1 | Direct |
| `GL_TRIANGLE_STRIP` | `MTLPrimitiveTypeTriangleStrip` | 1:1 | Direct |
| `GL_TRIANGLE_FAN` | `Triangle` list | n−2 triangles → 3(n−2) indices | **CPU expansion** (`:1935-1966`) |
| `GL_QUADS` / `GL_QUAD_STRIP` | *none* | — | **No Metal equivalent** — must be triangulated on the CPU |
| `GL_POLYGON` | *none* | — | **No Metal equivalent** — must be triangulated on the CPU |
| `GL_LINES_ADJACENCY`, `GL_LINE_STRIP_ADJACENCY`, `GL_TRIANGLES_ADJACENCY`, `GL_TRIANGLE_STRIP_ADJACENCY` | *none* | — | **No Metal equivalent** — adjacency data must be filtered on the CPU |
| `GL_PATCHES` | `drawPatches:` | 1:1 | Direct (tessellation) |

---

## 3. What this Mac measured (`OTHER_GPU` — **not** an A11)

A probe program was compiled and run on the **development host** to demonstrate
the *shape* of the runtime query and to check the API surface. It is recorded
here only so the numbers are not mistaken for A11 numbers.

| Property | Value | Class |
|---|---|---|
| Device | `Intel(R) KBL Unknown` (registryID 4294968427) | **`OTHER_GPU`** |
| OS | macOS 15.7.7 (24G720) | `OTHER_GPU` |
| `supportsFamily(.apple1 … .apple10)` | **all false** (this is an Intel iGPU, not an Apple GPU) | `OTHER_GPU` |
| `supportsFamily(.common1/.common2/.common3/.mac2/.metal3)` | **all true** | `OTHER_GPU` |
| `maxThreadsPerThreadgroup` | 1024 × 1024 × 1024 | `OTHER_GPU` |
| `maxThreadgroupMemoryLength` | 65 536 | `OTHER_GPU` |
| `maxArgumentBufferSamplerCount` | 2 048 | `OTHER_GPU` |
| `readWriteTextureSupport` / `argumentBuffersSupport` | 1 / 1 (tier 1) | `OTHER_GPU` |
| `areRasterOrderGroupsSupported` / `areProgrammableSamplePositionsSupported` | true / true | `OTHER_GPU` |
| `supports32BitFloatFiltering` / `supports32BitMSAA` / `supportsQueryTextureLOD` | true / true / true | `OTHER_GPU` |
| `supportsBCTextureCompression` | true | `OTHER_GPU` |
| `supportsShaderBarycentricCoordinates` | **false** | `OTHER_GPU` |
| `supportsVertexAmplificationCount(4)` | false | `OTHER_GPU` |
| `supportsRaytracing` / `supportsFunctionPointers` | true / true | `OTHER_GPU` |
| `supportsTextureSampleCount` 1 / 2 / 4 / 8 / 16 | true / true / true / true / **false** | `OTHER_GPU` |
| **Probe output** (the §4 program, run on this host) | `max_texture_bindings_per_fragment_function` = **128**, `max_fragment_stage_in_scalars` = **31**, `max_fragment_stage_in_float4s` = **30**, `max_stage_in_vectors_per_fragment_function` = **30**, `can_create_16384x16384_rgba8` = true, `default_sample_positions_4x` = `0.3750,0.1250;0.8750,0.3750;0.1250,0.6250;0.6250,0.8750` | `OTHER_GPU` — **not** A11 numbers. The 31/30 figures are the **macOS** varying limits (the compiler's own message on this host says `exceeds limit (32)` for the count and `exceeds limit (124)` for the components); the iOS limits differ (macOS counts `[[position]]` toward the limit, iOS does not — the compiler's error text says so explicitly). |

**None of these numbers says anything about the A11.** The A11's
`supportsFamily(.apple4)` will be `true`; on this host it is `false` because
this is not an Apple GPU.

---

## 4. Runtime capability probe (run **on the A11 device**)

The following is a complete, compilable Swift program. It produces the JSON
described in §5 and closes the gaps listed in §2.5.

```swift
// TGLMTMetalProbe.swift
// Build for the device:  xcrun -sdk iphoneos swiftc -target arm64-apple-ios16.0 \
//                          -O TGLMTMetalProbe.swift -o TGLMTMetalProbe
// Run: install on iPhone 8 / X (iOS 11..16), execute, take stdout.
//
// Every number printed below is MEASURED ON THE DEVICE. Nothing is hard-coded.

import Foundation
import Metal

func jsonEscape(_ s: String) -> String {
    var out = ""
    for ch in s.unicodeScalars {
        switch ch {
        case "\"": out += "\\\""
        case "\\": out += "\\\\"
        case "\n": out += "\\n"
        case "\r": out += "\\r"
        case "\t": out += "\\t"
        default:
            if ch.value < 0x20 { out += String(format: "\\u%04x", ch.value) }
            else { out.unicodeScalars.append(ch) }
        }
    }
    return out
}

guard let device = MTLCreateSystemDefaultDevice() else {
    print("{\"schema\":\"tglmt.metal.capabilities.v1\",\"error\":\"no Metal device\"}")
    exit(2)
}

var d: [String] = []
/// Append a raw JSON fragment (already correctly encoded).
func putRaw(_ key: String, _ jsonFragment: String) {
    d.append("\"\(jsonEscape(key))\":\(jsonFragment)")
}
/// Append a JSON string.
func putS(_ key: String, _ value: String) { putRaw(key, "\"\(jsonEscape(value))\"") }
/// Append a JSON number.
func putN(_ key: String, _ value: Int) { putRaw(key, "\(value)") }
/// Append a JSON boolean.
func putB(_ key: String, _ value: Bool) { putRaw(key, value ? "true" : "false") }
/// Append a JSON array of strings.
func putA(_ key: String, _ values: [String]) {
    putRaw(key, "[" + values.map { "\"\(jsonEscape($0))\"" }.joined(separator: ",") + "]")
}

putS("schema", "tglmt.metal.capabilities.v1")
putS("captured_at", ISO8601DateFormatter().string(from: Date()))
putS("os_version", ProcessInfo.processInfo.operatingSystemVersionString)
putS("environment_label", "A11_DEVICE")   // CHANGE to SIMULATOR/OTHER_GPU if not an A11

// ---- identity -------------------------------------------------------------
putS("device_name", device.name)
putN("device_registry_id", Int(device.registryID))
putB("has_unified_memory", device.hasUnifiedMemory)
putB("is_low_power", device.isLowPower)
putN("recommended_max_working_set", Int(device.recommendedMaxWorkingSetSize))
putN("max_buffer_length", Int(device.maxBufferLength))
putN("current_allocated_size", Int(device.currentAllocatedSize))

// ---- GPU family -----------------------------------------------------------
let appleFamilies: [(String, MTLGPUFamily)] = [
    ("apple1", .apple1), ("apple2", .apple2), ("apple3", .apple3),
    ("apple4", .apple4), ("apple5", .apple5), ("apple6", .apple6),
    ("apple7", .apple7), ("apple8", .apple8), ("apple9", .apple9),
    ("apple10", .apple10),
]
putA("supported_apple_families", appleFamilies.filter { device.supportsFamily($0.1) }.map { $0.0 })

let commonFamilies: [(String, MTLGPUFamily)] = [
    ("common1", .common1), ("common2", .common2), ("common3", .common3),
]
putA("supported_common_families", commonFamilies.filter { device.supportsFamily($0.1) }.map { $0.0 })

putB("supports_metal3_family", device.supportsFamily(.metal3))
if #available(macOS 26.0, iOS 26.0, *) {
    putB("supports_metal4_family", device.supportsFamily(.metal4))
} else {
    putB("supports_metal4_family", false)   // family constant does not exist yet
}

// Legacy feature-set probe (iOS 11/12 path, where supportsFamily: does not exist).
// The iOS-only feature-set constants are unavailable on macOS, so guard by OS.
#if os(iOS)
if #available(iOS 12.0, *) {
    putB("supports_feature_set_ios_gpuFamily4_v1",
         device.supportsFeatureSet(.iOS_GPUFamily4_v1))
}
#endif

// ---- per-feature switches -------------------------------------------------
putB("read_write_texture_support_tier1", device.readWriteTextureSupport == .tier1)
putB("read_write_texture_support_tier2", device.readWriteTextureSupport == .tier2)
putB("argument_buffers_tier1", device.argumentBuffersSupport == .tier1)
putB("argument_buffers_tier2", device.argumentBuffersSupport == .tier2)
putB("raster_order_groups_supported", device.areRasterOrderGroupsSupported)
putB("programmable_sample_positions_supported", device.areProgrammableSamplePositionsSupported)
putB("supports_32bit_float_filtering", device.supports32BitFloatFiltering)
putB("supports_32bit_msaa", device.supports32BitMSAA)
putB("supports_query_texture_lod", device.supportsQueryTextureLOD)
putB("supports_bc_texture_compression", device.supportsBCTextureCompression)
putB("supports_pull_model_interpolation", device.supportsPullModelInterpolation)
putB("supports_shader_barycentric_coordinates", device.supportsShaderBarycentricCoordinates)
putB("supports_raytracing", device.supportsRaytracing)
putB("supports_function_pointers", device.supportsFunctionPointers)
putB("supports_dynamic_libraries", device.supportsDynamicLibraries)
putB("supports_primitive_motion_blur", device.supportsPrimitiveMotionBlur)
putN("max_vertex_amplification_count", device.supportsVertexAmplificationCount(2) ? 2 : 0)
putN("max_argument_buffer_sampler_count", Int(device.maxArgumentBufferSamplerCount))

// ---- compute --------------------------------------------------------------
let mts = device.maxThreadsPerThreadgroup
putN("max_threads_per_threadgroup_x", Int(mts.width))
putN("max_threads_per_threadgroup_y", Int(mts.height))
putN("max_threads_per_threadgroup_z", Int(mts.depth))
putN("max_threadgroup_memory_length", Int(device.maxThreadgroupMemoryLength))

// ---- MSAA -----------------------------------------------------------------
putA("supported_sample_counts",
     [1, 2, 4, 8, 16].filter { device.supportsTextureSampleCount($0) }.map { "\($0)" })

// ---- default sample positions (proves programmable positions work) ---------
if device.areProgrammableSamplePositionsSupported {
    let pos = device.getDefaultSamplePositions(sampleCount: 4)
    let p = pos.map { String(format: "%.4f,%.4f", $0.x, $0.y) }.joined(separator: ";")
    putS("default_sample_positions_4x", p)
}

// ---- texture / format probing --------------------------------------------
// MTLDevice exposes NO max-texture-dimension API (grep of the whole Metal
// framework finds none), so the practical probe is: try to create the texture
// and see whether Metal accepts it.
//
// IMPORTANT: Metal *validates the pixel format against the device* and, if the
// format is unsupported, it raises an assertion and **aborts the process** —
// it does NOT return nil. (Verified on the development host: creating an
// `astc_4x4_ldr` texture on an Intel GPU that has no ASTC decoder aborts with
// "[MTLTextureDescriptorInternal validateWithDevice:]: failed assertion
//  Texture Descriptor Validation … has invalid pixelFormat (204)".)
// Therefore the compressed-format probes below are gated behind the
// `--unsafe-formats` flag: run them as a SEPARATE invocation, because one
// abort loses the whole report. The default run only touches formats that
// every Metal device supports.
let unsafeFormats = CommandLine.arguments.contains("--unsafe-formats")

func tryTexture(_ w: Int, _ h: Int, _ fmt: MTLPixelFormat, _ usage: MTLTextureUsage) -> Bool {
    let desc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: fmt,
                                                          width: w, height: h,
                                                          mipmapped: false)
    desc.usage = usage
    desc.storageMode = .private
    return device.makeTexture(descriptor: desc) != nil
}
putB("can_create_16384x16384_rgba8",
     tryTexture(16384, 16384, .rgba8Unorm, [.renderTarget, .shaderRead]))
putB("can_create_2048x2048_rgba8",
     tryTexture(2048, 2048, .rgba8Unorm, [.renderTarget, .shaderRead]))
putB("can_create_rgba16float_2048",
     tryTexture(2048, 2048, .rgba16Float, [.renderTarget, .shaderRead]))
putB("can_create_rgba32float_2048",
     tryTexture(2048, 2048, .rgba32Float, [.renderTarget, .shaderRead]))
putB("can_create_depth32float",
     tryTexture(2048, 2048, .depth32Float, [.renderTarget, .shaderRead]))
putB("can_create_depth32float_stencil8",
     tryTexture(2048, 2048, .depth32Float_stencil8, [.renderTarget, .shaderRead]))
if unsafeFormats {
    putB("can_create_astc_4x4",
         tryTexture(64, 64, .astc_4x4_ldr, [.shaderRead]))
    putB("can_create_etc2_rgb8",
         tryTexture(64, 64, .etc2_rgb8, [.shaderRead]))
    putB("can_create_bc1_rgba",
         tryTexture(64, 64, .bc1_rgba, [.shaderRead]))
    putB("can_create_pvrtc_rgb4bpp",
         tryTexture(64, 64, .pvrtc_rgb_4bpp, [.shaderRead]))
}

// ---- the two UNRESOLVED limits (FST-2017 vs FST-2026) --------------------
// Build a real RENDER PIPELINE with N texture bindings / N varyings and let
// Metal report its own limit. The limit is enforced by the *pipeline* (the
// function's resource signature), not by the library, so the library alone
// always compiles — that is why the probe must build a pipeline.
//
// NOTE: the MSL must be self-contained (`#include <metal_stdlib>` +
// `using namespace metal;`), the vertex input struct must contain only
// `[[attribute(n)]]` members, and a vertex descriptor must be supplied
// (otherwise Metal errors with "Vertex function has input attributes but no
// vertex descriptor was set").
func probeTextureBindingLimit() -> Int {
    var lo = 1, hi = 256
    while lo < hi {
        let mid = (lo + hi + 1) / 2
        var src = "#include <metal_stdlib>\nusing namespace metal;\n"
        src += "fragment float4 f("
        for i in 0..<mid { src += "texture2d<float> t\(i) [[texture(\(i))]], " }
        src += "uint s [[sample_id]]) { return float4(0); }"
        do {
            let lib = try device.makeLibrary(source: src, options: nil)
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = lib.makeFunction(name: "vert")
            d.fragmentFunction = lib.makeFunction(name: "f")
            d.colorAttachments[0].pixelFormat = .rgba8Unorm
            // The vertex function needs no attributes, so no descriptor is needed
            // — but the pipeline still needs a vertex function.
            let vsrc = "#include <metal_stdlib>\nusing namespace metal;\n"
                + "vertex float4 vert(uint i [[vertex_id]]) { return float4(0,0,0,1); }"
            let vlib = try device.makeLibrary(source: vsrc, options: nil)
            d.vertexFunction = vlib.makeFunction(name: "vert")
            _ = try device.makeRenderPipelineState(descriptor: d)
            lo = mid
        } catch {
            hi = mid - 1
        }
    }
    return lo
}
putN("max_texture_bindings_per_fragment_function", probeTextureBindingLimit())

func probeVaryingLimit() -> Int {
    var lo = 1, hi = 256
    while lo < hi {
        let mid = (lo + hi + 1) / 2
        var src = "#include <metal_stdlib>\nusing namespace metal;\n"
        src += "struct V { "
        for i in 0..<mid { src += "float4 a\(i) [[attribute(\(i))]]; " }
        src += "};\nstruct O { float4 p [[position]]; "
        for i in 0..<mid { src += "float4 a\(i); " }
        src += "};\n"
        src += "vertex O v(V in [[stage_in]]) { O o; o.p = float4(0,0,0,1); "
        for i in 0..<mid { src += "o.a\(i) = in.a\(i); " }
        src += "return o; }\n"
        src += "fragment float4 f(O in [[stage_in]]) { return float4(in.a0); }"
        do {
            let lib = try device.makeLibrary(source: src, options: nil)
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = lib.makeFunction(name: "v")
            d.fragmentFunction = lib.makeFunction(name: "f")
            d.colorAttachments[0].pixelFormat = .rgba8Unorm
            let vd = MTLVertexDescriptor()
            let stride = 4 * 4 * mid
            for i in 0..<mid {
                vd.attributes[i].format = .float4
                vd.attributes[i].offset = 4 * 4 * i
                vd.attributes[i].bufferIndex = 0
            }
            vd.layouts[0].stride = stride
            vd.layouts[0].stepFunction = .perVertex
            d.vertexDescriptor = vd
            _ = try device.makeRenderPipelineState(descriptor: d)
            lo = mid
        } catch {
            hi = mid - 1
        }
    }
    return lo
}
putN("max_stage_in_vectors_per_fragment_function", probeVaryingLimit())

// The fragment stage_in limit is actually TWO independent Metal limits, which
// the compiler reports separately:
//   "number of shader varyings (N) exceeds limit (32)"            -> COUNT limit
//   "number of shader varying components (N) exceeds limit (124)"  -> COMPONENT limit
// The FST publishes both ("Maximum number of inputs (scalars or vectors) to a
// fragment function" and "...input components..."), and the two editions of the
// FST disagree about the Apple4 value for BOTH. Measure them separately:
// use 1-component varyings to hit the count limit, and 4-component varyings to
// hit the component limit. The vertex function writes constants, so it consumes
// no vertex attributes and does not disturb the vertex-attribute limit.
func probeFragmentStageIn(_ comps: Int) -> Int {
    var lo = 1, hi = 512
    while lo < hi {
        let mid = (lo + hi + 1) / 2
        var src = "#include <metal_stdlib>\nusing namespace metal;\n"
        src += "struct O { float4 p [[position]]; "
        for i in 0..<mid { src += comps == 1 ? "float a\(i); " : "float4 a\(i); " }
        src += "};\n"
        src += "vertex O v(uint i [[vertex_id]]) { O o; o.p = float4(0,0,0,1); "
        for i in 0..<mid { src += comps == 1 ? "o.a\(i) = 0.0; " : "o.a\(i) = float4(0); " }
        src += "return o; }\n"
        src += "fragment float4 f(O in [[stage_in]]) { return float4(in.a0); }"
        do {
            let lib = try device.makeLibrary(source: src, options: nil)
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = lib.makeFunction(name: "v")
            d.fragmentFunction = lib.makeFunction(name: "f")
            d.colorAttachments[0].pixelFormat = .rgba8Unorm
            _ = try device.makeRenderPipelineState(descriptor: d)
            lo = mid
        } catch {
            hi = mid - 1
        }
    }
    return lo
}
putN("max_fragment_stage_in_scalars", probeFragmentStageIn(1))
putN("max_fragment_stage_in_float4s", probeFragmentStageIn(4))

print("{" + d.joined(separator: ",") + "}")
```

### 4.1 How to run it on an A11

```sh
cd /Volumes/project/TGLMT
xcrun -sdk iphoneos swiftc -target arm64-apple-ios16.0 -O TGLMTMetalProbe.swift -o TGLMTMetalProbe
# install on iPhone 8 / X, run, capture stdout
```

The `environment_label` field **must** be changed to `SIMULATOR` or `OTHER_GPU`
if the program is not run on the physical A11. The label in the output is what
makes a number admissible as `A11_DEVICE` evidence.

---

## 5. Machine-readable schema for the captured report

The probe in §4 emits JSON conforming to the following schema. Save it as
`docs/metal_a11_capabilities.json` (or under `reports/`) when the device run
happens.

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://tglmt.local/schemas/metal-capabilities-v1.json",
  "title": "TGLMT Metal capability report",
  "type": "object",
  "required": [
    "schema", "captured_at", "environment_label", "device_name",
    "supported_apple_families", "supported_common_families",
    "max_threads_per_threadgroup_x", "max_threadgroup_memory_length",
    "supported_sample_counts"
  ],
  "additionalProperties": false,
  "properties": {
    "schema":      { "const": "tglmt.metal.capabilities.v1" },
    "captured_at": { "type": "string", "format": "date-time" },
    "environment_label": {
      "enum": ["A11_DEVICE", "SIMULATOR", "OTHER_GPU", "REFERENCE_ONLY", "NONE"]
    },
    "os_version":  { "type": "string" },
    "error":       { "type": "string" },

    "device_name":              { "type": "string" },
    "device_registry_id":       { "type": "integer" },
    "has_unified_memory":       { "type": "boolean" },
    "is_low_power":             { "type": "boolean" },
    "recommended_max_working_set": { "type": "integer" },
    "max_buffer_length":        { "type": "integer" },
    "current_allocated_size":   { "type": "integer" },

    "supported_apple_families": {
      "type": "array",
      "items": { "enum": ["apple1","apple2","apple3","apple4","apple5",
                          "apple6","apple7","apple8","apple9","apple10"] }
    },
    "supported_common_families": {
      "type": "array",
      "items": { "enum": ["common1","common2","common3"] }
    },
    "supports_metal3_family":   { "type": "boolean" },
    "supports_metal4_family":   { "type": "boolean" },
    "supports_feature_set_ios_gpuFamily4_v1": { "type": "boolean" },

    "read_write_texture_support_tier1": { "type": "boolean" },
    "read_write_texture_support_tier2": { "type": "boolean" },
    "argument_buffers_tier1":    { "type": "boolean" },
    "argument_buffers_tier2":    { "type": "boolean" },
    "raster_order_groups_supported": { "type": "boolean" },
    "programmable_sample_positions_supported": { "type": "boolean" },
    "supports_32bit_float_filtering": { "type": "boolean" },
    "supports_32bit_msaa":       { "type": "boolean" },
    "supports_query_texture_lod": { "type": "boolean" },
    "supports_bc_texture_compression": { "type": "boolean" },
    "supports_pull_model_interpolation": { "type": "boolean" },
    "supports_shader_barycentric_coordinates": { "type": "boolean" },
    "supports_raytracing":       { "type": "boolean" },
    "supports_function_pointers": { "type": "boolean" },
    "supports_dynamic_libraries": { "type": "boolean" },
    "supports_primitive_motion_blur": { "type": "boolean" },
    "max_vertex_amplification_count": { "type": "integer" },
    "max_argument_buffer_sampler_count": { "type": "integer" },

    "max_threads_per_threadgroup_x": { "type": "integer", "minimum": 1 },
    "max_threads_per_threadgroup_y": { "type": "integer", "minimum": 1 },
    "max_threads_per_threadgroup_z": { "type": "integer", "minimum": 1 },
    "max_threadgroup_memory_length":  { "type": "integer", "minimum": 0 },

    "supported_sample_counts": {
      "type": "array", "items": { "type": "integer", "enum": [1,2,4,8,16] }
    },
    "default_sample_positions_4x": { "type": "string" },

    "can_create_16384x16384_rgba8":      { "type": "boolean" },
    "can_create_2048x2048_rgba8":        { "type": "boolean" },
    "can_create_rgba16float_2048":       { "type": "boolean" },
    "can_create_rgba32float_2048":       { "type": "boolean" },
    "can_create_depth32float":           { "type": "boolean" },
    "can_create_depth32float_stencil8":  { "type": "boolean" },
    "can_create_astc_4x4":               { "type": "boolean" },
    "can_create_etc2_rgb8":              { "type": "boolean" },
    "can_create_bc1_rgba":               { "type": "boolean" },
    "can_create_pvrtc_rgb4bpp":          { "type": "boolean" },

    "max_texture_bindings_per_fragment_function": { "type": "integer", "minimum": 0 },
    "max_stage_in_vectors_per_fragment_function": { "type": "integer", "minimum": 0 },
    "max_fragment_stage_in_scalars":   { "type": "integer", "minimum": 0 },
    "max_fragment_stage_in_float4s":   { "type": "integer", "minimum": 0 }
  }
}
```

**Rule for consuming this report:** a number may only be cited as an A11 fact if
`environment_label == "A11_DEVICE"`. Everything else is `OTHER_GPU`,
`SIMULATOR`, or `REFERENCE_ONLY` and must be labelled as such in prose.

---

## 6. Summary — what the A11 *can* do

| Area | Verdict on A11 | Evidence class |
|---|---|---|
| Core rasterisation, depth/stencil, blending, MRT (8), MSAA (4×) | **Yes** | `REFERENCE_ONLY` (FST-2017 `iOS_GPUFamily4_v1` column) |
| Tessellation, max factor 16 | **Yes** | `REFERENCE_ONLY` |
| Programmable sample positions, programmable blending (framebuffer fetch), dual-source blending, argument buffers tier 1, cube-map texture arrays, imageblocks, tile shaders, raster order groups | **Yes** (all at the Apple4 floor) | `REFERENCE_ONLY` |
| 16 384 px 2D textures, 2 048 px 3D, 2 048 layers, 256 M-px texture buffers | **Yes** | `REFERENCE_ONLY` |
| ASTC / ETC2 / EAC / PVRTC | **Yes** | `REFERENCE_ONLY` |
| 16-bit normalised + float colour formats | **Yes** | `REFERENCE_ONLY` |
| Compute: 1 024 threads/threadgroup, 32 KB threadgroup memory, indirect dispatch | **Yes** | `REFERENCE_ONLY` |
| Occlusion query with sample counting | **Yes** | `REFERENCE_ONLY` |
| Dual-source blending (GL `index=1` outputs) | **Yes** — TGLMT's mapping is wrong, not the hardware | `REFERENCE_ONLY` |
| BC1–BC7 compressed textures | **No** | `REFERENCE_ONLY` |
| More than one viewport / scissor per pass | **No** (1 only) | `REFERENCE_ONLY` |
| Border-clamp sampling with a border colour | **No** | `REFERENCE_ONLY` |
| Layered rendering (per-primitive layer index) | **No** | `REFERENCE_ONLY` |
| Lines wider than 1 px | **No API at all** | `REFERENCE_ONLY` |
| Logic op (bitwise) | **No** | `REFERENCE_ONLY` |
| Geometry shader / mesh shader | **No** | `REFERENCE_ONLY` |
| Ray tracing, sparse textures, variable ROP, vertex amplification, 64-bit & float atomics, texture atomics, function pointers, barycentrics | **No** | `REFERENCE_ONLY` |
| Metal 4 (argument tables, command allocators/barriers, decoupled queues, tensors) | **No** | `REFERENCE_ONLY` |
| Texture argument table size (31 vs 96) and `[[stage_in]]` limit (60 vs 124) | **`NOT_VERIFIED`** — the two Apple editions disagree | needs §4 probe |
