# Metal on iOS / Apple A11 — Limitations, Evidence and Consequences

> This document states, with evidence, what OpenGL 4.6 Core behaviour **cannot**
> be reproduced on an Apple A11 GPU (or on iOS in general) using Metal, and
> what TGLMT does instead.
>
> Sources: Apple's *Metal Feature Set Tables* (iOS GPU family 4 =
> `MTLGPUFamilyApple4` = A11), the Metal Shading Language Specification, and
> the Metal API availability macros in the iOS SDK.

---

## 1. The A11 in one table

| Property | Value |
|---|---|
| SoC | Apple A11 (A11 Bionic) |
| GPU family | `MTLGPUFamilyApple4` (also Apple3, Apple2, Apple1) |
| Architecture | Tile-based deferred renderer (TBDR) |
| Metal feature set | Metal 2.x (iOS 11 SDK and later) |
| Max 2D texture | 16 384 px |
| Max 3D texture | 2 048 px |
| Max 2D array layers | 2 048 |
| Max colour render targets | 8 |
| Max MSAA samples | 4 |
| Max buffers / textures per stage (tier 1) | 31 |
| Max threadgroup memory | 32 KB |
| Max threads per threadgroup | 1 024 |
| Max threadgroups per kernel | 65 535 |
| Max tessellation factor | 16 |
| BC1–BC7 compression | **not supported** |
| ETC2 / EAC / ASTC | supported |
| Programmable sample positions | supported |
| Visibility-buffer occlusion queries | supported (Apple3+) |
| Tier-2 resource barriers (`MTLAccessor`) | **not supported** (Apple9/M1 only) |
| Metal 3 features (mesh shaders, dual-source blend, ray tracing) | **not supported** |

---

## 2. Hard limits — impossible on A11, by design of Metal

These are **not** implementation gaps in TGLMT. They are properties of the
Metal API surface available to the device.

### 2.1 Geometry shaders

*Metal has no geometry shader stage.* The closest equivalents are the mesh
shader pipeline (`MTLMeshRenderPipelineDescriptor`, Metal 3 / Apple7) and
`drawMeshThreadgroups:` — both require **A14 or newer**.

**TGLMT behaviour**: `glCreateShader(GL_GEOMETRY_SHADER)` is accepted, but
`glCompileShader` fails with an explicit log:
`error: geometry shader ngoài subset (GS cần đường mesh riêng)`.
The draw path refuses geometry programs rather than rendering something
different.

**Consequence for GL 4.6 conformance**: geometry shaders are part of the core
profile, so this is a genuine, hardware-caused conformance gap.

### 2.2 Dual-source blending

`layout(location=0, index=1)` is the *second blend source of draw buffer 0*.
Metal 3 offers `[[dual_source_color(n)]]`, which requires **Apple9 / A14+**.

**TGLMT behaviour**: an output with `index=1` is emitted as `[[color(1)]]` —
i.e. a second draw buffer, not a second blend source. `GL_MAX_DUAL_SOURCE_DRAW_BUFFERS`
is reported as 1 (the correct GL 3.3 minimum).

**Consequence**: dual-source blending is not reproduced. Apps that use it need
two draw buffers and blend manually.

### 2.3 Per-sample shading rate (`GL_SAMPLE_SHADING`)

Metal has no equivalent of `GL_MIN_SAMPLE_SHADING_VALUE`. The state is
tracked and reported correctly, but it does not change rasterisation.

### 2.4 Lines wider than 1 pixel

Metal's line rasterisation is always 1 pixel wide. `GL_ALIASED_LINE_WIDTH_RANGE`
is reported as `(1, 1)`, and the implementation clamps `glLineWidth` to 1
with an `INVALID_VALUE` for anything else (per GL 4.6 §14.6.4 the range for
`ALIASED_LINE_WIDTH` is implementation-dependent, so 1.0 is conformant).

### 2.5 BC1–BC7 / S3TC compressed textures

A11 has **no BC decoder**. `deviceSupportsBC()` returns false for iOS < 16.4
(`AppleMetalBridge.mm:1436`), so `newCompressedTexture()` returns `nullptr`
and the texture has no GPU representation at all.

**Known inconsistency**: `GL_COMPRESSED_TEXTURE_FORMATS` advertises BC1–BC7
and S3TC regardless of the device, and `docs/EXTENSION_MATRIX.md` claims a
"CPU decode fallback" that **does not exist** in the code.

**Correct behaviour**: the advertised list must be filtered by device
capability. ETC2/EAC/ASTC work correctly (verified by
`tests/unit/test_compressed_texture.cpp` and `tests/unit/test_compressed_contract.cpp`).

### 2.6 Multiple viewports / clip distances

Metal has exactly one viewport and one clip distance per render pass.
`GL_MAX_VIEWPORTS` is reported as 16 and `GL_MAX_CLIP_DISTANCES` as 8, but
only index 0 is applied.

**Correct behaviour**: the limits should be reported as 1, or the
implementation should reject multi-viewport use.

### 2.7 `GL_COLOR_LOGIC_OP`

Metal has no logic-op stage. TGLMT bypasses the blend stage when
`GL_COLOR_LOGIC_OP` is enabled, so the result is the destination colour, not
the logical operation.

### 2.8 Tier-2 resource barriers

`MTLAccessor` / `MTLBarrierEncoder` with resource barriers require Apple9 /
M1. On A11, ordering must come from encoder boundaries.

**TGLMT behaviour**: correct. Every compute pass is flushed before a render
pass opens (`gl_apple_draw.cpp:2401-2404`), and Metal guarantees that
encoders within a command buffer execute in order with automatic hazard
tracking. This is *stronger* than GL's requirements for the cases used, so no
information is lost — but the GL per-bit barrier semantics
(`GL_BUFFER_UPDATE_BARRIER_BIT`, `GL_TEXTURE_FETCH_BARRIER`, …) are not
reproduced individually: all bits behave like a full barrier.

---

## 3. Limits TGLMT reports that do not match A11

| GL limit | Reported | A11 real | Direction | Where |
|---|---:|---:|---|---|
| `GL_MAX_TEXTURE_SIZE` | 8 192 | 16 384 | under-reports | `gl_get.cpp:125` |
| `GL_MAX_CUBE_MAP_TEXTURE_SIZE` | 8 192 | 16 384 | under-reports | `gl_get.cpp:127` |
| `GL_MAX_RENDERBUFFER_SIZE` | 8 192 | 16 384 | under-reports | `gl_get.cpp:129` |
| `GL_MAX_VIEWPORT_DIMS` | 8192×8192 | 16384×16384 | under-reports | `gl_get.cpp:220` |
| `GL_MAX_FRAMEBUFFER_WIDTH/HEIGHT` | 8 192 | 16 384 | under-reports | `gl_get.cpp:374` |
| **`GL_MAX_TEXTURE_IMAGE_UNITS`** | **32** | **31** | **over-reports** | `gl_get.cpp:135` |
| `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS` | 48 | 62 | under-reports | `gl_get.cpp:137` |
| **`GL_MAX_UNIFORM_BUFFER_BINDINGS`** | **36** | **31** | **over-reports** | `gl_get.cpp:159` |
| `GL_MAX_VIEWPORTS` | 16 | 1 | over-reports | `gl_get.cpp:275` |
| `GL_MAX_CLIP_DISTANCES` | 8 | 1 | over-reports | `gl_get.cpp:276` |
| `GL_MAX_DUAL_SOURCE_DRAW_BUFFERS` | 1 | 0 (impossible) | over-reports | `gl_get.cpp:133` |
| `GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS` | 1 024 | 1 024 | correct, **not enforced** | `gl_get.cpp:174` |
| `GL_MAX_COMPUTE_SHARED_MEMORY_SIZE` | 32 768 | 32 768 | correct, **not enforced** | `gl_get.cpp:169` |

The two **over-reports that can crash a real app** are
`GL_MAX_TEXTURE_IMAGE_UNITS = 32` (a 32-sampler fragment shader produces
`texture(31)`, which exceeds Metal's tier-1 table of 31 and aborts Metal
validation) and `GL_MAX_UNIFORM_BUFFER_BINDINGS = 36` (bindings beyond slot
30 are silently dropped by `gl_apple_draw.cpp:3078`).

---

## 4. Availability guards in the Metal bridge

The bridge contains exactly **8** runtime availability checks:

| Guard | Gates |
|---|---|
| `@available(macOS 10.13, iOS 11.0, *)` | `setSamplePositions:count:` (programmable sample positions) |
| `@available(macOS 10.15, iOS 13.0, *)` | `MTLVisibilityResultModeCounting` (device-level) and per-draw |
| `@available(macOS 10.13, iOS 11.0, *)` | `getDefaultSamplePositions:count:`, `areProgrammableSamplePositionsSupported` |
| `@available(macOS 10.15, iOS 10.3, *)` | `GPUEndTime` (for `GL_TIME_ELAPSED`) |
| `@available(iOS 16.4, macOS 11.0, tvOS 16.4, *)` | `supportsBCTextureCompression` |
| `@available(macOS 10.13, *)` | ETC2/ASTC on macOS |

The project's minimum deployment target is iOS 16 (`Makefile:29`), so the
Metal-2-era APIs are always available; the guards matter mainly for macOS
and for BC compression.

---

## 5. Device validation status

| Environment | Status | Evidence |
|---|---|---|
| macOS 15.7.7, Intel iGPU | **185 / 185 CTest pass** (Apple Metal build) | `reports/BASELINE_TEST_REPORT.md` |
| iOS device (A11) | **Not executed in this session** | — |

The repository contains an on-device suite (`apps/aquarium/device_tests.cpp`,
19 test functions / 84 pixel-verified checks) that runs on iOS through the
AquariumBench app and writes `Documents/tglmt-device-tests.log`. The same
source is compiled for the host via
`tests/integration/test_device_suite_apple.cpp`.

**Honest statement**: no A11 run was performed in this session, and the
repository does not contain a committed device log. The "73/73 pass" claim
that appears in the (previously untracked) `docs/TRUTH_REPORT.md` refers to an
older revision of the suite (81 checks at HEAD, 84 in the working tree) and
**cannot be reproduced from the repository**. Every device-dependent
statement in these documents is therefore marked as "not verified on A11".

---

## 6. Summary — what "OpenGL 4.6 Core on A11" can and cannot mean

| Aspect | Achievable on A11? |
|---|---|
| 657 core entry points exported and callable | **Yes** — all 657 are exported and implemented; 5 previously-empty Invalidate entry points were implemented in this work |
| GL-level semantics (state, errors, objects, lifetimes) | **Yes**, with the error-generation gaps fixed in this work |
| Shader compilation of the GLSL subset TGLMT supports | **Yes** — measured 0.22 ms (VS) / 0.72 ms (FS) / 0.28 ms (CS) per shader |
| The GLSL *subset* TGLMT supports | **Partial** — see `docs/GLSL_460_COMPILER.md` |
| Geometry shaders | **No** — impossible on A11 |
| Dual-source blending | **No** — impossible on A11 |
| Per-sample shading | **No** — impossible on any Metal version |
| BC compressed textures | **No** — impossible on A11 (ETC2/ASTC work) |
| Multi-viewport / multi-clip-distance rendering | **No** — Metal has one of each |
| CPU-blocking `glClientWaitSync` | **No** — not implemented (no Metal event) |
| GPU-executed indirect draws | **No** — emulated on the CPU (one frame early) |
| Khronos CTS conformance | **Not run** — see `reports/CONFORMANCE_REPORT.md` |
