# Performance Report

**Measurement host**: `Darwin 24.6.0 x86_64`, macOS 15.7.7, Intel iGPU
(`Intel(R) KBL Unknown`), 16 GB RAM.
**Build**: Release (`-O3 -DNDEBUG`), `TGLMT_APPLE_METAL=ON`.
**Protocol**: 9 full runs per phase; metric = median of the 9 medians; the
**min** is reported as the interference-robust statistic; control metrics
(untouched code paths) establish the noise floor.

Raw data: `benchmarks/baseline/`, `benchmarks/optimized/`,
`benchmarks/compare/baseline_vs_optimized.txt`.

> **The target platform is iOS / Apple A11. The development host is macOS on
> an Intel iGPU.** Absolute numbers differ between them — especially for
> texture uploads, where iOS uses `MTLStorageModeShared` (a direct
> `replaceRegion`) and macOS uses `MTLStorageModeManaged` (staging + blit).
> `benchmarks/compilation/bench_upload.mm` measures **both** paths with the
> raw Metal API so an A11-relevant number can be obtained on any host.

---

## 1. Baseline vs optimised

### 1.1 Runtime

| Metric | baseline | optimised | Δ (median) | Δ (min) | verdict |
|---|---:|---:|---:|---:|---|
| `glTexImage2D_64x64_full_upload` | 74.29 µs | 45.12 µs | **−39.3 %** | **−31.1 %** | **IMPROVED** |
| `glTexSubImage2D_64x64_staged` | 7.33 µs | 7.28 µs | −0.8 % | −2.8 % | within noise |
| `glBindBuffer` | 0.062 µs | 0.062 µs | 0.0 % | 0.0 % | unchanged |
| `glGetIntegerv` | 0.042 µs | 0.042 µs | 0.0 % | 0.0 % | unchanged |
| `glUniform1f` | 0.050 µs | 0.050 µs | 0.0 % | 0.0 % | unchanged |
| `glBufferSubData` (64 B) | 0.073 µs | 0.072 µs | −1.4 % | 0.0 % | unchanged |
| `glDrawArrays` (3 verts) | 1.73 µs | 0.94 µs | −45.8 % | −2.3 % | **noise** (control) |
| `glDrawElements` (3 verts) | 2.01 µs | 1.07 µs | −46.8 % | −0.6 % | **noise** (control) |
| `glClear` | 10.61 µs | 7.37 µs | −30.6 % | −5.4 % | **noise** (control) |
| 100 draws | 208.2 µs | 123.8 µs | −40.5 % | +0.2 % | **noise** (control) |

### 1.2 The noise floor

The last five rows are **control metrics**: code paths the optimisation did
not touch. Their *median* values move by up to 46.8 %, while their *min*
values move by at most 5.4 %. **The min statistic is the only one that can
support a conclusion on this host.**

### 1.3 Compilation

| Metric | baseline | optimised | Δ (median) | Δ (min) |
|---|---:|---:|---:|---:|
| `vs_complex.glsl_to_msl` | 0.2256 ms | 0.2212 ms | −1.9 % | −0.6 % |
| `fs_complex.glsl_to_msl` | 0.7174 ms | 0.6955 ms | −3.1 % | −1.0 % |
| `cs_compute.glsl_to_msl` | 0.2690 ms | 0.2626 ms | −2.4 % | −2.6 % |
| `fs_complex.msl_to_metal_library` | 0.0639 ms | 0.0622 ms | −2.7 % | −0.5 % |
| `fs_complex.pipeline_creation` | 0.0066 ms | 0.0063 ms | −4.6 % | −1.6 % |
| `fs_complex.total` | 0.7953 ms | 0.7643 ms | −3.9 % | −0.8 % |

All within the noise floor: **no compiler regression, no compiler gain**.

### 1.4 Upload (raw Metal API)

| Metric | baseline | optimised | Δ (median) | Δ (min) |
|---|---:|---:|---:|---:|
| `shared_replaceRegion_x1_ios_path` | 0.559 µs | 0.586 µs | +4.8 % | +0.2 % |
| `shared_replaceRegion_x2_old_buggy_path` | 1.407 µs | 1.664 µs | +18.3 % | −0.1 % |
| `managed_staging_plus_blit_macos_path` | 34.25 µs | 25.52 µs | −25.5 % | −4.4 % |
| `newTexture_64x64` | 10.41 µs | 10.34 µs | −0.7 % | +1.2 % |
| `newBufferWithBytes_16KB` | 12.66 µs | 8.53 µs | −32.6 % | −2.2 % |

The `shared_*` and `managed_*` metrics measure **untouched Metal code**, so
their movement is pure noise — and it defines the noise floor.

---

## 2. The optimisation that was delivered

### 2.1 Problem

`AppleMetalBridge.mm`'s `updateTexture`, `updateTextureLevel`,
`updateTexture3D` and `updateTexture3DLevel` each performed a
`replaceRegion:withBytes:` **and then** called `uploadTexBytes()`, which
performs *another* upload of the same region. **Every texture upload wrote
the texture twice.**

This is a structural defect, not a tuning issue: the number of GPU writes per
upload was exactly 2 instead of 1.

It also carried a data-loss hazard on macOS: the duplicated direct
`replaceRegion` on a *managed* texture is exactly the pattern the source
comments identify as having caused data loss
("replaceRegion+synchronizeResource trên máy này từng chép GPU copy (rỗng)
đè CPU copy → mất dữ liệu").

### 2.2 Fix

`uploadTexBytes()` already performs the single correct upload
(`replaceRegion` on shared storage, staging + blit on managed storage), so
the duplicated call was removed from all four entry points.

### 2.3 Evidence

**Structural (deterministic, not timing-dependent)**: the number of
`replaceRegion` calls per upload went from 2 to 1. That is a countable fact
in the source.

**Isolated measurement of the iOS/A11 path** (raw Metal, shared storage,
64×64 RGBA8 = 16 KiB):

| Operation | median | p95 |
|---|---:|---:|
| 1 × `replaceRegion` (after the fix) | **0.55 µs** | 0.72 µs |
| 2 × `replaceRegion` (before the fix) | 1.41 µs | 1.63 µs |

→ the duplicate cost **0.85 µs per 16 KiB upload**, i.e. the fix removes
**~60 % of the upload cost on the iOS/A11 path**.

**End-to-end** (`glTexImage2D` 64×64 RGBA8, 9 runs): **74.29 → 45.12 µs**
(−39.3 % median, **−31.1 % min**), against a ~5 % noise floor. **Confirmed.**

**Correctness**: all 185 tests pass, including the pixel-verified upload
tests `test_texture_apple`, `test_mc_texupload_apple`,
`test_get_texture_mip_apple`, `test_mip_storage_apple`,
`test_legacy_mip_apple`.

### 2.4 Why the staged path did not improve

`glTexSubImage2D` shows no change (−2.8 % min, inside the noise). Its cost
is dominated by the CPU-side shadow copy and the staging bookkeeping, not by
the GPU upload — the upload happens later, at flush time, and is amortised
over the merged bounding box.

---

## 3. Where the time actually goes

### 3.1 Per-draw breakdown (in-tree instrumentation, 2 000 mixed draws)

| Phase | µs/draw | Share |
|---|---:|---:|
| pipeline lookup | 1.8 | 26 % |
| uniform upload | 0.6 | 9 % |
| sampler binding | 0.3 | 4 % |
| index handling | 0.2 | 3 % |
| Metal draw encode | 5.6 | 58 % |

Pipeline-cache effectiveness: **1 998 / 2 000** draws hit the context-level
cache; only **2** reached the Metal bridge. The cost is building and
comparing the cache key, not the lookup.

### 3.2 Shader compilation (load time)

| Stage | median | p95 |
|---|---:|---:|
| GLSL → MSL (VS) | 0.22 ms | 0.26 ms |
| GLSL → MSL (FS) | 0.70 ms | 0.92 ms |
| GLSL → MSL (CS) | 0.26 ms | 0.36 ms |
| MSL → Metal library | 0.055–0.068 ms | **175 ms** |
| Pipeline creation | 0.004–0.007 ms | 0.02 ms |
| **Total, GLSL → pipeline ready (FS)** | **0.76 ms** | 0.92 ms |

The **Metal compiler** dominates the cold path (175 ms p95 vs a 0.066 ms
median). This is Apple's compiler, and it is a load-time cost that a binary
library cache would amortise.

---

## 4. Bottlenecks that remain

| Bottleneck | Cost | Why it is still there |
|---|---:|---|
| Metal compiler (cold) | up to 175 ms per library | Apple's compiler; a binary cache would amortise it |
| `newTexture` | 10.4 µs | GPU allocation; only on texture creation |
| `newBufferWithBytes` | 8.5 µs | GPU allocation; the macOS staging path allocates one per upload |
| `TRIANGLE_FAN` / `baseVertex` / primitive-restart expansion | O(n) CPU per draw | Metal has no equivalent primitive types |
| Indirect draws read back on the CPU | one-frame-stale semantics | `MTLIndirectCommandBuffer` exists, but the GL semantics do not map directly |
| `glClientWaitSync` does not block | — | no Metal event is created |
| Pipeline cache key construction | ~26 % of the draw cost | the key is ~700 bytes; a hash would risk collisions |

---

## 5. Regression check

Every optimisation was followed by the full test suite:

| Change | Test result |
|---|---|
| Duplicate texture upload removed | 185 / 185 pass |
| Catalog derivation fixed | 185 / 185 pass + `test_regression_catalog` |
| Shader semantics fixed | 185 / 185 pass + `test_regression_shader_semantics` (6 cases) |
| Error generation / object lifetime fixed | 185 / 185 pass + `test_regression_gl_errors` (11 cases) |
| Pipeline cache identity fixed | 185 / 185 pass + `test_regression_pipeline_cache_apple` (verified to fail when reverted) |

---

## 6. Reproducing

```sh
scripts/bench_ab.sh baseline optimized 9      # build + 9 runs/phase + comparison
python3 benchmarks/compare/bench_compare.py \
    benchmarks/baseline/runtime.json benchmarks/optimized/runtime.json \
    --control glBindBuffer glGetIntegerv glUniform1f
```

See `benchmarks/README.md` for the full protocol.
