# Conformance Report

**Scope**: how far TGLMT can be shown to conform to the OpenGL 4.6 Core
Profile specification, and — stated plainly — what is **not** known.

---

## 1. Summary

| Conformance evidence | Status |
|---|---|
| Khronos OpenGL CTS (dEQP `KHR-GL46*`) | **NOT RUN** — no CTS source in the tree, and no platform layer to host it |
| Differential testing against a reference GL implementation | **NOT POSSIBLE** on this host — no reference GL 4.6 context available |
| Differential testing against `glslang` + `spirv-cross` (shader path only) | **DONE** — `tests/integration/test_converter_proof.cpp`, `test_spirv_chain_apple.cpp` |
| Specification-derived API/state coverage | **DONE** — the API catalog and the accepted-pname list are both derived from the official Khronos sources |
| Pixel-level GPU behaviour | **DONE** — 78 integration tests read pixels back and compare |
| A11 device behaviour | **NOT VERIFIED in this session** |

**There is no CTS result to report. Any statement that TGLMT "passes the
OpenGL 4.6 CTS" would be false.**

---

## 2. Why the CTS cannot be run today

### 2.1 The CTS source is not in the tree

```
$ find . -iname '*VK-GL-CTS*' -o -iname '*dEQP*' -o -iname '*deqp*'
(no results)
$ grep -riE "cts|deqp" --include='*' . | grep -v '^./docs/khronos' | head
(no results outside a docstring in tools/truth_manifest.py)
```

The only Khronos artefacts vendored are the **specification inputs**:
`docs/khronos/gl.xml`, `glspec46.core.pdf`, `GLSLangSpec.4.60.pdf`,
`opengl46-quick-reference-card.pdf` and `docs/khronos/extensions/`.

### 2.2 TGLMT has no EGL/GLX platform layer

The CTS's `dEQP-GL` module needs a platform that can create a context,
make it current and resolve entry points:

| CTS requirement | TGLMT status |
|---|---|
| `eglGetDisplay` / `eglCreateContext` / `eglMakeCurrent` | **absent** — TGLMT has no EGL (see the comment in `src/core/Context.cpp:46`) |
| `eglGetProcAddress` | **absent** — TGLMT resolves symbols by `dlsym` on the dylib, which is close but not the EGL contract |
| Surface / pbuffer creation | **absent** — TGLMT renders into FBOs and a `CAMetalLayer` |
| `GL_CONTEXT_PROFILE_MASK` / `GL_CONTEXT_FLAGS` reporting | present (`glGetIntegerv`) |
| `glGetString(GL_VERSION)` in the CTS-expected format | reports `4.6 (TGLMT Metal port, build …)` — the CTS parses the leading version, but the vendor string is non-standard |

A CTS port therefore needs **three** pieces of work, not one:

1. Vendor and pin `VK-GL-CTS`.
2. Write an **offscreen EGL-like platform** that creates a TGLMT context,
   provides a pbuffer-like FBO and implements `eglGetProcAddress`.
3. Fix the conformance gaps the CTS would then expose (see §5).

The project's own roadmap listed this as a future phase; it has not been done.

### 2.3 What the CTS would immediately expose

Running `KHR-GL46` "mustpass" would, at minimum, exercise:

* the **error-generation** paths that were missing before this work (the CTS
  checks `glGetError` after every invalid call) — those are now largely fixed
  and are covered by `tests/regression/test_regression_gl_errors.cpp`;
* the **state table** — 341 state variables, of which the implementation
  tracks a subset; the CTS would find every unmodelled one;
* the **texture completeness** rules, which TGLMT does not implement at all;
* the **framebuffer completeness** rules, of which two status codes are never
  returned;
* the **GLSL conformance suite** (`glsl-*` tests), which would exercise a
  translator that is a rewriter, not a compiler.

## 3. What conformance evidence *does* exist

### 3.1 Specification-derived catalogs

| Artefact | Source | How it is verified |
|---|---|---|
| The 657-entry core API catalog | `docs/khronos/gl.xml` | set-equal to the official Khronos `glcorearb.h` |
| The 42-entry compatibility-only catalog | `gl.xml`, `profile="compatibility"` | none of them appear in `glcorearb.h` |
| The 965 accepted `glGet*` pnames | `glspec46.core.pdf` Ch. 23 (341 state variables) + the enums of the 136 advertised extensions + the pnames the implementation handles | extracted by `tools/gen_state_pnames.py` from the PDF and cross-checked against the registry |
| The `GL_*` enum values | `gl.xml` | generated into `include/tglmt/gl46_types.h` |

The accepted-pname list is a **specification-derived** conformance artefact
that did not exist before this work.

### 3.2 Pixel-level GPU verification

78 integration tests render into an FBO, read the pixels back and compare
against an expected value with an explicit tolerance. They cover blending,
depth, stencil, MSAA + resolve, MRT, rasterisation modes, the ROPs, polygon
offset, viewport orientation, texture upload/sampling, tessellation
(quads/triangles, spacing, winding, instance ID, patch, capture), transform
feedback, indirect draw, compute + SSBO readback, sample locations, sample
mask, shadow comparison, clip distance, conservative depth, multi-draw, base
instance, and the device suite.

### 3.3 Differential shader testing

`tests/integration/test_converter_proof.cpp` renders the same geometry
through **TGLMT's converter** and through **`glslangValidator -G` +
`spirv-cross`**, then compares the pixels within ±5 **and** checks the
physically expected value (r ≈ 216). This is the strongest available
evidence that the shader translation preserves semantics.

`tests/integration/test_spirv_chain_apple.cpp` runs the full
GLSL → SPIR-V → MSL → Metal → pixels chain and compares the result.

### 3.4 Static analysis over the whole source

| Tool | What it proves |
|---|---|
| `tools/truth_manifest.py --strict` | every core entry point is exported; no advertised extension is a no-op |
| `tools/gen_api_docs.py` | the coverage table is derived from the manifest, not hand-written |
| `tools/scan_behavior_gaps.py` | a triage list of "skipped / unimplemented" markers (a heuristic, **not** proof — 2 of its first 3 hits were false positives) |
| `tests/unit/test_core_registry.cpp` | object-name semantics |
| `tests/unit/test_p0_honesty.cpp` | query counters return real values |

## 4. Conformance status by OpenGL 4.6 Core area

| Area | Status | Evidence | Gaps |
|---|---|---|---|
| API surface (657 entry points) | **exported and implemented** | manifest: 0 missing symbols, 0 static gaps | 320 have no direct test |
| Object lifecycle | **implemented** | regression test E6–E10 | context is not thread-safe |
| Error generation | **largely implemented** (was 55 % of entry points with no check) | regression test E1–E5, E11 | 0 core functions untested |
| Uniform ma trận | `transpose` honoured on **both** paths (GL + DSA); non-square matrices (`mat2x3` …) accepted and packed with the Metal column stride | regression test R7–R11 (`test_regression_uniform_transpose`) | — |
| State table (341 variables) | **partial** | `StateTracker` + the 965 accepted pnames | ~23 state variables have no dedicated storage (see the audit) |
| Fixed-function raster state | **implemented** | pixel tests | `GL_COLOR_LOGIC_OP`, per-sample shading, wide lines are impossible on Metal |
| Textures | **implemented** | pixel tests | **no texture-completeness model**; BC compression impossible on A11 |
| Framebuffers | **implemented** | pixel tests | `INCOMPLETE_MULTISAMPLE` / `FRAMEBUFFER_UNSUPPORTED` never returned |
| Blending | **implemented** | `test_blend_apple` | dual-source blending impossible on A11 |
| Shaders / GLSL 4.60 | **partial subset** | differential test | the translator is a rewriter; `std140`/`std430`/`packed`/`row_major` are not honoured; `double` and GS are rejected |
| Compute | **implemented** | `test_compute_dispatch`, `test_ssbo_graphics_apple` | `dispatchGroups3D` not implemented by either backend |
| Synchronisation | **partial** | `glFinish`/`glFlush` correct | `glClientWaitSync` never blocks; `glMemoryBarrier` per-bit semantics not reproduced |
| Queries | **partial** | occlusion queries are **real** (visibility buffer) | `GL_TIMESTAMP` is a CPU clock; `QUERY_COUNTER_BITS` reports 1 |
| Debug output | **implemented** | `test_debug_p4` | — |
| Transform feedback | **partial** | `test_xfb_*` (5 tests) | the replay path does not re-bind attributes |
| Indirect draw | **emulated on the CPU** | `test_mdi_count_baseinstance_apple` | the command buffer is read at submission, not at execution |

## 5. The gaps the CTS would find first (ranked)

1. **Texture completeness** — no model at all. A mipmap-filtered texture
   with only level 0 must sample (0,0,0,1); TGLMT samples level 0.
2. **`std140`/`std430` block layout** — ignored by the translator; only the
   uniform-block packer compensates.
3. **`glClientWaitSync` never blocks** — a fence-based readback races.
4. **Framebuffer completeness status codes** — two codes are never returned,
   and a size mismatch uses a token removed in GL 3.0.
5. **`GL_MAX_TEXTURE_IMAGE_UNITS` over-reported** (32 vs Metal's 31) — a
   32-sampler shader aborts Metal validation.
6. **`GL_COMPRESSED_TEXTURE_FORMATS` over-reported** on devices without BC.
7. **`glUniform*` location validation** — `glUniform*` with a location that
   does not exist still writes into the uniform store.
8. **`glGet*` of an unhandled-but-valid pname** now raises `INVALID_ENUM`,
   which is correct only if the pname list is complete — it is derived from
   the spec, but the extension part is a superset.

## 6. Recommended conformance work (in order of value per effort)

| # | Action | Effort | Value |
|---|---|---|---|
| 1 | Implement the texture-completeness model (§8.14) | small | high — a whole class of CTS failures |
| 2 | Fix `GL_MAX_TEXTURE_IMAGE_UNITS` and `GL_MAX_UNIFORM_BUFFER_BINDINGS` to the real Metal limits | minutes | high — prevents Metal aborts |
| 3 | Filter `GL_COMPRESSED_TEXTURE_FORMATS` by device capability | small | high — removes a false advertisement |
| 4 | Implement `std140`/`std430` layout in the translator | large | high — required for the CTS GLSL suite |
| 5 | Implement `glClientWaitSync` with `MTLSharedEvent` | medium | high — unblocks fence-based apps |
| 6 | Write the offscreen EGL-like platform, vendor the CTS, run `KHR-GL46` mustpass | large | the only way to get a real conformance number |

## 7. Explicit non-claims

* TGLMT has **not** passed the Khronos CTS. It has never been run.
* TGLMT has **not** been validated on Apple A11 hardware in this session.
* "All 657 core entry points are exported and implemented" is a **statement
  about the code**, not a conformance claim. The specification's *behavioural*
  requirements are only partially verified, as tabulated above.
