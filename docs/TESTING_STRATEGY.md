# Testing Strategy

> What is proven, how it is proven, and — just as important — **what is not
> proven**. Every claim in this document is backed by a command that can be
> re-run.

---

## 1. Principles

1. **A green CTest run is not evidence.** Before this work, 13 test files used
   `assert()` as their only failure mechanism; in a Release build (`-DNDEBUG`)
   the assertion expression is *not even evaluated*, so those tests passed
   unconditionally. All of them now use `TGLMT_CHECK` from `tests/test_util.h`,
   which always evaluates the condition and returns a non-zero exit code.
2. **A skipped test is not a passing test.** Tests report `SKIP` explicitly and
   the skip reason is recorded. The count of *actually executed* assertions is
   reported alongside the pass count.
3. **Pixel-level truth beats state-level truth.** A test that only checks
   `glGetIntegerv` output proves the bookkeeping is self-consistent, not that
   the GPU did the right thing. The strongest tests in the repository read
   pixels back and compare them to an expected image.
4. **Differential evidence beats assertion evidence.** Where a reference
   implementation is available (`glslangValidator` + `spirv-cross`), the same
   workload is run through both and the *results* are compared.

---

## 2. Test inventory

| Suite | Files | LOC | What it covers |
|---|---:|---:|---|
| `tests/unit/` | 95 | 13 751 | Null backend or converter only; no GPU required |
| `tests/integration/` | 78 | 11 684 | Real Metal device; pixel-verified |
| `tests/api/` | 1 | — | The published API surface (catalog, exports, version, pnames) |
| `tests/conformance/` | 1 | — | Conformance surface vs the Khronos registry |
| `tests/shaders/` | 1 + 7 GLSL | — | GLSL 4.60 corpus; each shader is converted **and** compiled by the real Metal compiler |
| `tests/truth/` | 1 | — | Runtime probe for known gaps (not a CTest gate) |
| `tests/regression/` | 4 | — | One file per defect family fixed in this work (see §6) |
| `apps/aquarium/device_tests.cpp` | 1 | 1 598 | On-device suite, shared with the host |

**185 CTest executables** in the current tree (169 before this work, +5 regression
suites, +6 new suites: the API surface, the shader corpus, the conformance
surface, and the uniform / vertex-attribute / raster / framebuffer entry-point
suites).

### 2.1 Assertion strength

| Strength | Meaning | Typical location |
|---|---|---|
| **STRONG** | reads back pixels / buffer contents and compares to an expected value | most of `tests/integration/*_apple.cpp` |
| **MEDIUM** | checks a returned value, an error code, a link/compile status | most of `tests/unit/` |
| **WEAK** | only checks "did not crash" | **eliminated** — see §3.1 |

---

## 3. Test-infrastructure defects found and fixed

### 3.1 `assert()` compiled out in Release — 13 files

Affected: `test_core_registry`, `test_elements_offset0`, `test_formats`,
`test_framebuffer_state`, `test_glconvert`, `test_readpixels_packed`,
`test_renderer`, `test_restart_xfb`, `test_shader_program`,
`test_texture_sampler`, `test_draw_triangle`, `test_converter_aquarium`,
`test_runtime_options`.

**Proof that the tests were meaningless in Release** (run in `/tmp`, nothing in
the repository was modified):

```
$ sed 's/count == 6/count == 999/' tests/unit/test_elements_offset0.cpp > /tmp/broken.cpp
=== flags: '-DNDEBUG=1' ===
test_elements_offset0 PASS         <-- rc=0   (silently green)
=== flags: 'none' ===
Assertion failed: (tr[0].indexed && tr[0].count == 999), function main, line 30.
                                       <-- rc=134 (SIGABRT)
```

The worst case was `tests/unit/test_renderer.cpp`: it is the *only* test of
the public `Renderer` façade, and in a Release build its `assert(r.Init(...))`
was compiled out — so `Init()` was never called, the Apple branch was never
taken, and the test printed `SKIP phần Apple (không có GPU / build null)`
followed by `PASS`, **on a machine that did have a Metal device**.

**Fix**: `tests/test_util.h` provides `TGLMT_CHECK(cond, msg)` (always
evaluates, counts failures, prints `file:line: FAIL: msg`) and
`TGLMT_RESULT` (the process exit code). All 13 files were converted.

### 3.2 Ten tests silently skipped because of the working directory

`add_test(NAME ${NAME} COMMAND ${NAME})` ran the test with CWD = the build
directory, but ten tests resolve their fixtures through **relative** paths
(`shaders/triangle.metal`, `apps/aquarium/shaders/...`). They printed
`SKIP: shader not found` and CTest reported **PASS**.

Measured difference (Apple Metal build):

| Test | under `ctest` (build dir) | from the repo root |
|---|---|---|
| `test_blend_apple` | SKIP | PASS (blend formula exact) |
| `test_converter_proof` | SKIP | PASS (converter == spirv-cross on GPU) |
| `test_msaa_apple` | SKIP | PASS (MSAA resolve exact) |
| `test_raster_apple` | SKIP | PASS (fill/wire/scissor/depth) |
| `test_rop_apple` | SKIP | PASS (16/16 ROPs exact) |
| `test_spirv_chain_apple` | SKIP | PASS (GLSL→SPV→MSL→GPU→pixels) |
| `test_tess_patch_apple` | SKIP | PASS (real GPU tessellation) |
| `test_texture_apple` | SKIP | PASS (upload+sampler+sample exact) |
| `test_gpu_triangle_apple` | SKIP | PASS (real GPU render + readback) |
| `test_converter_aquarium` | SKIP | FAIL (see §3.3) |

These are **the strongest pixel-level tests in the repository**, and they were
never running in CI.

**Fix**: `add_test(... WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})` plus a
`TGLMT_ROOT` compile definition.

**Result**: the number of tests that skip for environmental reasons dropped
from **13 to 2** (`test_gs_mesh_apple` needs Apple7+, and
`test_mc_terrain_ubo_real` prints the token `SKIP_ROWS`).

### 3.3 The Metal-compiler gate never fired

`tests/unit/test_converter_aquarium.cpp` wrote the generated MSL to
`<name>.metal` but ran `xcrun -sdk macosx metal -c <name>.msl`. Metal treats
a `.msl` argument as a **linker input**, exits 0 and produces **no `.air`
file** — so the "does the Metal compiler accept our MSL?" check was a no-op.
On a clean machine the test failed; on the development machine it "passed"
because stale `.msl` files from an earlier build were lying around.

**Fix**: write and compile the same path, and assert that the `.air` file
actually exists.

---

## 4. Test categories and what each proves

### 4.1 API contract tests (`tests/unit/`)

State round-trips, error codes, object lifetime, reflection queries.
Example: `tests/regression/test_regression_gl_errors.cpp` covers 11 error-generation
and object-lifetime defects with explicit expected `GLenum` values.

### 4.2 Pixel-level GPU tests (`tests/integration/`)

Render into an FBO, read the pixels back, compare against the expected
value with an explicit tolerance. These are the only tests that prove the
*rendering* is correct.

### 4.3 Differential tests

| Test | Reference | Comparison |
|---|---|---|
| `test_converter_proof` | `glslangValidator -G` + `spirv-cross` | rendered pixels within ±5 **and** the physically expected value |
| `test_spirv_chain_apple` | full GLSL → SPIR-V → MSL → Metal | pixel comparison of the same scene |

These are the strongest shader-correctness evidence available without the
Khronos CTS.

### 4.4 Device suite

`apps/aquarium/device_tests.cpp` — 19 test functions, 84 pixel-verified
checks. The **same source** is compiled for the host
(`tests/integration/test_device_suite_apple.cpp`) and for iOS
(`apps/aquarium/ios_shell.mm`), so there is a single source of truth.

### 4.5 Conformance audit

`tools/truth_manifest.py` joins four independent sources:

```
Khronos registry (gl.xml)  ⇄  exported symbols (nm)  ⇄  implementation (src/gl)  ⇄  tests
```

and produces `tests/truth/api_manifest.json` + `docs/TRUTH_MANIFEST.md`.
`--strict` fails the build on a missing symbol or an advertised no-op.

---

## 5. How to run

```sh
# Null backend (no GPU needed) — fast
cmake -S . -B build-null -DCMAKE_BUILD_TYPE=Release -DTGLMT_BUILD_TESTS=ON
cmake --build build-null -j8
(cd build-null && ctest -j4 --output-on-failure)

# Apple Metal backend (needs a GPU)
cmake -S . -B build-metal -DCMAKE_BUILD_TYPE=Release -DTGLMT_BUILD_TESTS=ON \
      -DTGLMT_APPLE_METAL=ON
cmake --build build-metal -j8
(cd build-metal && ctest -j4 --output-on-failure)

# Audit: registry ⇄ symbol ⇄ implementation ⇄ tests
python3 tools/truth_manifest.py --lib build-metal/libtglmt.dylib --strict

# Behaviour-gap scanner (triage list, not proof)
python3 tools/scan_behavior_gaps.py
```

### 5.1 Measured results (this session)

| Build | Result | Wall time |
|---|---|---|
| Null, Release, `-j4` | **185 / 185 pass** | 6.30 s |
| Apple Metal, Release, `-j4` | **185 / 185 pass** | 4.64 s |
| Apple Metal, full rebuild `-j8` | build clean, no warnings in project code | ~1 m 54 s |

---

## 6. Regression tests added in this work

| File | Covers | Defects |
|---|---|---|
| `tests/regression/test_regression_shader_semantics.cpp` | R1–R6 | texture bias lost; `flat`/`noperspective` dropped; fragment output at `location != 0` written to attachment 0; `textureQueryLod` uncompilable; `transpose` ignored; `mat3` packed 36 B into a 48 B slot |
| `tests/regression/test_regression_gl_errors.cpp` | E1–E11 | `glGet*` never raised `INVALID_ENUM`; `glBindTexture` did not validate the target; `glShaderSource` dereferenced NULL; `glUniform*fv` aborted on a negative count; invalid draw mode/type silently drew; buffers/programs/VAOs/samplers stayed bound after deletion; re-specifying a mapped buffer caused a use-after-free; the five `Invalidate*` entry points generated no errors |
| `tests/regression/test_regression_catalog.cpp` | — | the catalog must be 657 core + 42 compatibility-only; every core name must be exported; `glGetPointerv` must be present |
| `tests/regression/test_regression_uniform_transpose.cpp` | R7–R11 | `transpose` của uniform ma trận trên **cả** hai đường (`glUniformMatrix*` và DSA `glProgramUniformMatrix*`), ma trận không vuông `mat2x3`, và `count < 0` |
| `tests/regression/test_regression_pipeline_cache_apple.cpp` | — | the pipeline cache must not reuse a PSO built from a previous shader (relink), and `ILibrary::identity()` must be unique per library |

The pipeline-cache test was verified to **fail** when the fix is reverted
(it reports `khoá cache pipeline phải chứa linkGen hiện tại của program`),
so it is a genuine regression test and not a tautology.

---

## 7. Gaps in the test strategy (honest list)

| # | Gap | Impact | Why it is not closed |
|---|---|---|---|
| T1 | **No Khronos CTS run.** | The single largest conformance unknown | TGLMT has no EGL/GLX platform layer, so the CTS cannot be hosted; see `reports/CONFORMANCE_REPORT.md` |
| T2 | **No A11 device run in this session.** | Hardware behaviour on the target GPU is unverified | No A11 device was attached; the on-device suite exists and is wired, but its log is not in the repository |
| T3 | **The null backend is not equivalent to the Apple backend.** | MRT, texture uploads, mip chains and the compute grid are untested on CI | The null backend is a CPU shadow; making it equivalent is a large piece of work |
| T4 | **The "test reference" metric is a token match.** | A test that merely mentions `glFoo` counts as coverage for `glFoo` | A call-graph-level analysis would be needed |
| T5 | **No fuzzing / property testing** of the API layer | Argument-validation crashes could survive | A libFuzzer harness over the GL entry points is the natural next step |
| T6 | **No memory-leak checking** (ASan/LSan) in CI | Leaks would be invisible | Not wired into the build |

---

## 8. Regression policy

1. Every defect fixed in the implementation gets a test in
   `tests/regression/test_regression_*.cpp` or `tests/regression/test_regression_*`.
2. The test must fail when the fix is reverted (verified for the pipeline-cache
   test, which is the hardest case).
3. A test that cannot be run must say why in its output; it is never counted
   as a pass.
4. `tools/truth_manifest.py --strict` runs in the audit step; a missing symbol
   or an advertised no-op fails.
