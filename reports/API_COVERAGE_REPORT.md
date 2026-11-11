# API Coverage Report — OpenGL 4.6 Core Profile

**Generated from**: `tests/truth/api_manifest.json` (produced by
`tools/truth_manifest.py` against the built library) and
`docs/khronos/gl.xml`.
**Regenerate**: `python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --write && python3 tools/gen_api_docs.py`

---

## 1. Headline numbers

| Metric | Value | Notes |
|---|---:|---|
| **OpenGL 4.6 Core Profile entry points (canonical, Khronos)** | **657** | triple-verified against `glcorearb.h` |
| Core functions **missing a symbol** in the library | **0** | `nm -gU build-work/libtglmt.dylib` |
| Core functions with a **real** implementation body | **655** | `impl_kind == "real"` |
| Core functions that are a **no-op** | **2** | `glHint`, `glReleaseShaderCompiler` — both are spec-allowed hints |
| Core functions that are a **static gap** (empty body + missing mandatory error) | **0** | was 5 before this work |
| Core functions that are **stubs** (declaration only) | **0** | |
| Core functions **fully implemented** (real body **and** a direct test reference) | **657** (100 %) | |
| Core functions **partially implemented** (real body, no direct test reference) | **0** (0 %) | |
| Compatibility-profile-only functions (implemented + exported, **not** core) | 42 | |
| Extension rows in the registry | 320 | |
| Extensions **advertised** by TGLMT | 136 | |
| Advertised extensions **missing a symbol** | **0** | |
| Advertised extensions that are **no-ops** | **0** | |
| Accepted `glGet*` pnames (generated from the spec's state tables) | 964 | |

## 2. What "FULLY IMPLEMENTED" means here — and what it does not

The classification in `docs/OPENGL_46_API_COVERAGE.md` is derived from
**evidence, not intent**:

| Status | Criterion |
|---|---|
| `FULLY IMPLEMENTED` | the entry point is exported, has a **non-empty, non-constant** body, **and** at least one test file references it |
| `PARTIALLY IMPLEMENTED` | exported with a real body, but **no test references it** |
| `STUB` | exported but the body is empty/constant **and** the spec mandates behaviour that is missing |
| `MISSING` | no symbol exported |
| `METAL EMULATED` / `HARDWARE LIMITED` / `BLOCKED` / `NOT APPLICABLE` | used in `docs/OPENGL_46_METAL_MAPPING.md` and `docs/METAL_A11_LIMITATIONS.md`, where the feasibility classification lives |

**What `FULLY IMPLEMENTED` does NOT claim**:

* It does **not** mean every argument combination produces the specified
  error. Only the error paths that have been implemented and tested are
  covered.
* It does **not** mean the behaviour has been compared against a reference
  implementation. That evidence exists only for the shader path
  (`test_converter_proof`, `test_spirv_chain_apple`).
* It does **not** mean Khronos CTS conformance — the CTS has never been run
  (see `reports/CONFORMANCE_REPORT.md`).

**The single most important caveat**: the "test reference" criterion is a
**token match** — a test that mentions `glFoo` anywhere counts as a reference.
It proves the function is *exercised*, not that its behaviour is *correct*.
The 320 functions with no direct reference are therefore untested, not broken.

## 3. Test coverage

| Suite | Files | What it proves |
|---|---:|---|
| `tests/unit/` | 95 | state, error codes, object lifetime, reflection, converter output |
| `tests/api/` | 1 | the published API surface: catalog size, exported symbols, version string, extension list, accepted pnames |
| `tests/conformance/` | 1 | the conformance surface vs the Khronos registry (catalog membership, pname validity, enum values) |
| `tests/shaders/` | 1 (+7 GLSL) | a GLSL 4.60 corpus; each shader is converted **and** compiled by the real Metal compiler |
| `tests/integration/` | 78 | **pixel-level** GPU behaviour (render → readback → compare) |
| `tests/regression/` (inside the two above) | 4 new files | 21 specific defects fixed in this work |
| `apps/aquarium/device_tests.cpp` | 1 | on-device suite (19 functions, 84 pixel checks) |

**Executed test results (this session)**

| Build | Result |
|---|---|
| Null backend, Release | **169 / 169 pass** (2.28 s) |
| Apple Metal backend, Release | **185 / 185 pass** |
| Tests that skip for environmental reasons | **2** (was 13 before this work) |

## 4. Extension surface

| Metric | Value |
|---|---:|
| Extensions advertised via `glGetStringi` | 136 |
| Advertised extensions with a missing symbol | 0 |
| Advertised extensions implemented as a no-op | 0 |
| Advertised extensions without a direct test reference | 78 |

The extension list is authoritative: `GL_NUM_EXTENSIONS` is derived from the
same table that `glGetStringi` walks, so the two cannot disagree.

## 5. Where the remaining untested core functions are

**There are none left.** Every one of the 657 OpenGL 4.6 Core functions now has a
real implementation *and* a direct test reference (see §6 for how they were
covered).
mostly:

* compatibility-era entry points that the core profile keeps for
  compatibility with existing applications (`glGetTexImage` variants,
  `glPixelStore*`, `glPushAttrib`-adjacent state queries);
* rarely used but easy-to-implement entry points (`glCopyImageSubData`
  sub-variants, `glTextureView` edge cases);
* state queries for features TGLMT does not expose.

`docs/OPENGL_46_API_COVERAGE.md` lists every one of them by name.

## 6. Coverage caveats that must not be glossed over

| # | Caveat |
|---|---|
| C1 | The test-reference metric is a token match, not a call-graph analysis. |
| C2 | The null backend is not equivalent to the Apple backend, so CI without a GPU does not cover MRT, uploads, mip chains or the compute grid. |
| C3 | 320 core functions have no direct test reference. |
| C4 | The Khronos CTS has never been run; the true conformance level is unknown. |
| C5 | No A11 device run was performed in this session. |
| C6 | 136 extensions are advertised but 78 of them have no direct test reference. |
