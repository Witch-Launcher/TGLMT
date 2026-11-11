# Baseline Test Report

**Scope**: the test infrastructure and the behaviour of the implementation
**before** the correctness work of this session, and the same measurements
**after**.

**Machine**: `Darwin 24.6.0 x86_64`, macOS 15.7.7, Intel iGPU
(`Intel(R) KBL Unknown`), 16 GB.
**Build**: Release (`-O3 -DNDEBUG`), `TGLMT_APPLE_METAL=ON`.

---

## 1. Baseline (start of session, commit `10ff95f` + working tree)

### 1.1 Build

| Configuration | Wall time | Result |
|---|---:|---|
| Null, Release, `-j8` | 1 m 47 s | clean, no project warnings |
| Apple Metal, Release, `-j8` | 1 m 54 s | clean |

### 1.2 CTest

| Configuration | Result | Wall time |
|---|---|---:|
| Null, Release | 169 / 169 "passed" | 2.28 s |
| Apple Metal, Release | 169 / 169 "passed" | 4.64 s |

### 1.3 What that "169 / 169 passed" actually meant

Classifying the 169 executables by their own output revealed that a large
part of the suite was inert:

| Category | Count | Meaning |
|---|---:|---|
| **Executed and asserting** | 86 | the test really checked something |
| **Silently skipped** (null backend: "no MTL device") | 80 | printed `SKIP` and returned 0 ⇒ CTest counted it as a **pass** |
| **Silently skipped** (CWD-dependent fixture paths) | 10 | printed `SKIP: shader not found` ⇒ counted as a **pass** |
| Genuinely skipped | 2 | `test_gs_mesh_apple` (needs Apple7+), `test_mc_terrain_ubo_real` (false positive) |

**Two independent defects made the suite look green while testing almost
nothing:**

**(a) `assert()` is compiled out in Release.** 13 test files used `assert()`
as their only failure mechanism. Under `-DNDEBUG` the assertion *expression is
not evaluated at all*. Proof (run in `/tmp`, the repository was not modified):

```
$ sed 's/count == 6/count == 999/' tests/unit/test_elements_offset0.cpp > /tmp/broken.cpp
=== flags: '-DNDEBUG=1' ===
test_elements_offset0 PASS         <-- rc=0   (silently green)
=== flags: 'none' ===
Assertion failed: (tr[0].indexed && tr[0].count == 999), function main, line 30.
                                       <-- rc=134 (SIGABRT)
```

The worst case was `tests/unit/test_renderer.cpp` — the only test of the
public `Renderer` façade. In a Release build it printed
`SKIP phần Apple (không có GPU / build null)` followed by `PASS`
**on a machine that did have a Metal device**.

**(b) The Metal-compiler gate never fired.**
`tests/unit/test_converter_aquarium.cpp` wrote the generated MSL to
`<name>.metal` but ran `metal -c <name>.msl`. Metal treats a `.msl` argument
as a **linker input**: it exits 0, prints a warning and produces **no `.air`
file**. The "does the Metal compiler accept our MSL?" check was therefore a
no-op, and on a clean machine the test *failed* (because `metal -c` on a
missing file exits 1).

### 1.4 Baseline benchmark

| Metric | median | p95 |
|---|---:|---:|
| GLSL → MSL (vertex, complex) | 0.22 ms | 0.26 ms |
| GLSL → MSL (fragment, complex) | 0.70 ms | 0.92 ms |
| GLSL → MSL (compute) | 0.26 ms | 0.36 ms |
| MSL → Metal library | 0.055–0.068 ms | up to **175 ms** |
| Pipeline creation | 0.004–0.007 ms | 0.02 ms |
| `glBindBuffer` | 0.062 µs | 0.063 µs |
| `glGetIntegerv` | 0.042 µs | 0.043 µs |
| `glUniform1f` | 0.050 µs | 0.051 µs |
| `glDrawArrays` (3 verts) | 0.95 µs | 1.13 µs |
| `glClear` | 7.3 µs | 114 µs |
| 100 draws | 127 µs | — |

Raw data: `benchmarks/baseline/`.

---

## 2. Changes made to the test system

| # | Change | Effect |
|---|---|---|
| T1 | `tests/test_util.h` with `TGLMT_CHECK` (always evaluates) and `TGLMT_RESULT`; 13 files converted | A Release build now **fails** when the behaviour is wrong |
| T2 | `add_test(... WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})` + `TGLMT_ROOT` define | 10 GPU tests that silently skipped now **run** |
| T3 | `test_converter_aquarium` writes and compiles the same path, and asserts the `.air` file exists | The Metal-compiler gate now **actually gates** |
| T4 | `tests/regression/test_regression_shader_semantics.cpp` (6 cases) | Locks the 6 shader-semantics fixes |
| T5 | `tests/regression/test_regression_gl_errors.cpp` (11 cases) | Locks the 11 error-generation / lifetime fixes |
| T6 | `tests/regression/test_regression_catalog.cpp` | Locks the catalog at 657 + 42 and checks every core symbol is exported |
| T7 | `tests/regression/test_regression_pipeline_cache_apple.cpp` | Locks the pipeline-cache fix; **verified to fail when the fix is reverted** |

## 3. After

### 3.1 Build and test

| Configuration | Result | Wall time |
|---|---|---:|
| Null, Release, `-j8` | clean | — |
| Apple Metal, Release, `-j8` | clean, no project warnings | — |
| CTest, Null | **169 / 169 pass** | 2.28 s |
| CTest, Apple Metal | **185 / 185 pass** | 9.2 s |

### 3.2 Skips

| | before | after |
|---|---:|---:|
| Tests that skip | 92 | **2** |

The 10 previously-skipped GPU tests now run and pass, including the
strongest pixel-level tests in the repository:

```
test_blend_apple             PASS (blend formula exact)
test_converter_proof         PASS (converter == spirv-cross on GPU)
test_msaa_apple              PASS (MSAA resolve exact)
test_raster_apple            PASS (fill/wire/scissor/depth)
test_rop_apple               PASS (16/16 ROPs exact)
test_spirv_chain_apple       PASS (GLSL->SPV->MSL->GPU->pixels)
test_tess_patch_apple        PASS (real GPU tessellation)
test_texture_apple           PASS (upload+sampler+sample exact)
test_gpu_triangle_apple      PASS (real GPU render + readback)
test_converter_aquarium      PASS (12/14 stages)
```

### 3.3 Behavioural baseline before vs after

| Property | Before | After |
|---|---|---|
| Core catalog | 698 (wrong) | **657** (canonical) + 42 compat-only tracked separately |
| Core functions without a symbol | 0 | 0 |
| Core functions that are static gaps | 5 | **0** |
| Core functions with a `glGet*` INVALID_ENUM path | 0 | **964** accepted pnames validated |
| Crash on `glShaderSource(NULL)` | yes | **no** |
| Abort on `glUniform*fv(count<0)` | yes | **no** |
| Use-after-free on `glBufferData` of a mapped buffer | yes | **no** |
| Stale pipeline after `glLinkProgram` | yes | **no** |
| `transpose` honoured in `glUniformMatrix*fv` | no | **yes** |
| `mat3` uniform contents | corrupt | **correct** |
| `texture(s,uv,bias)` LOD bias | lost | **preserved** |
| `flat` / `noperspective` varyings | interpolated | **correct** |
| Single fragment output at `location≠0` | attachment 0 | **correct attachment** |
| `textureQueryLod` | uncompilable MSL | **compiles and runs** |
| Deleted buffer/program/VAO/sampler | stayed bound | **unbound** |
| Texture upload writes | **twice** | **once** |

## 4. Reproducing

```sh
# Null (không cần GPU)
cmake -S . -B build-null -DCMAKE_BUILD_TYPE=Release -DTGLMT_BUILD_TESTS=ON
cmake --build build-null -j8
(cd build-null && ctest -j4 --output-on-failure)

# Apple Metal
cmake -S . -B build-metal -DCMAKE_BUILD_TYPE=Release -DTGLMT_BUILD_TESTS=ON \
      -DTGLMT_APPLE_METAL=ON
cmake --build build-metal -j8
(cd build-metal && ctest -j4 --output-on-failure)

# Audit
python3 tools/truth_manifest.py --lib build-metal/libtglmt.dylib --strict
python3 tools/gen_api_docs.py
```

## 5. Known gaps in the test baseline

| # | Gap |
|---|---|
| G1 | The Khronos CTS has never been run (see `reports/CONFORMANCE_REPORT.md`) |
| G2 | No A11 device run was performed in this session |
| G3 | The null backend is not equivalent to the Apple backend, so CI without a GPU does not cover MRT/uploads/mips/compute-grid |
| G4 | 320 core functions have no direct test reference |
| G5 | No fuzzing, no ASan/LSan in CI |
| G6 | The "test reference" metric is a token match, not a call-graph analysis |
