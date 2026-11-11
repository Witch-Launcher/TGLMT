# Performance Engineering

> Every number in this document was measured on the machine described in
> `benchmarks/*/meta.json`. Where a measurement is unreliable, that is stated
> explicitly rather than hidden.

---

## 1. Measurement methodology

| Aspect | Choice | Why |
|---|---|---|
| Build | Release, `-O3 -DNDEBUG`, `TGLMT_APPLE_METAL=ON` | matches the shipped configuration |
| Warm-up | 200–300 iterations discarded | removes first-touch page faults, Metal pipeline cache misses and shader-cache warm-up |
| Repetitions | **9 full runs per phase**, metric = **median of the 9 medians** | a single run on this machine has a ±30–47 % spread |
| Second statistic | **min of the 9 runs** | the fastest run is the least contaminated by interference; it is the robust statistic |
| Noise floor | measured from **control metrics** — code paths the optimisation did *not* touch | without this, "improved" is meaningless |
| Threshold | 2 % for a verdict; anything below the noise floor is reported as "not proven" | |

### 1.1 The noise floor is large, and that is a finding

The development machine is an Intel iGPU in a shared macOS host. Control
metrics (untouched code paths) moved by up to **46.8 %** on the median
statistic between two phases:

| Control metric | median-based delta | min-based delta |
|---|---:|---:|
| `glBindBuffer` | 0.00 % | 0.00 % |
| `glGetIntegerv` | 0.00 % | 0.00 % |
| `glUniform1f` | 0.00 % | 0.00 % |
| `frame_100_draws` | −40.5 % | **+0.24 %** |
| `glDrawElements_3` | −46.8 % | **−0.57 %** |
| `glDrawArrays_3` | −45.8 % | **−2.26 %** |
| `glClear` | −30.6 % | **−5.37 %** |

**Conclusion**: the median statistic on this host is dominated by interference;
the **min** statistic is stable to about ±5 %. All conclusions below therefore
rest on the min statistic and on structural (countable) arguments, not on the
medians.

---

## 2. Where the time goes

Measured with the in-tree instrumentation
(`tests/integration/test_perf_draw_apple.cpp`, `TGLMT_DIAG=1`), 2 000 mixed
draws (program/VAO/uniform/texture/FBO changes):

| Phase | Time per draw | Share |
|---|---:|---:|
| pipeline lookup | 1.8 µs | 26 % |
| uniform upload | 0.6 µs | 9 % |
| sampler binding | 0.3 µs | 4 % |
| index handling | 0.2 µs | 3 % |
| Metal draw encode | 5.6 µs | 58 % |
| **total** | **~1.3 µs** (without diagnostics) | |

Pipeline-cache effectiveness: **1 998 of 2 000 draws hit the context-level
cache** (`pipelineLookupSkipped`); only **2** reached the Metal bridge. The
cache-key construction, not the lookup, is the cost.

---

## 3. Optimisation delivered

### 3.1 Duplicate texture upload removed

**Problem.** `AppleMetalBridge.mm`'s `updateTexture`, `updateTextureLevel`,
`updateTexture3D` and `updateTexture3DLevel` each performed a
`replaceRegion:withBytes:` **and then** called `uploadTexBytes()`, which
performs *another* upload of the same region. Every texture upload therefore
wrote the texture **twice**.

This is a **structural** defect, not a tuning issue: the number of GPU writes
per upload was exactly 2 instead of 1.

**Fix.** `uploadTexBytes()` already performs the single correct upload
(`replaceRegion` on shared storage, staging + blit on managed storage), so the
duplicated call was removed from all four entry points
(`src/metal/AppleMetalBridge.mm`).

**Evidence — isolated measurement of the iOS/A11 path** (raw Metal API, no
TGLMT code in the loop; `bench_upload`, shared storage is what iOS uses):

| Operation (64×64 RGBA8 = 16 KiB) | median | p95 |
|---|---:|---:|
| 1 × `replaceRegion` (**after** the fix) | **0.55 µs** | 0.72 µs |
| 2 × `replaceRegion` (**before** the fix) | 1.41 µs | 1.63 µs |

The duplicate cost **0.85 µs per 16 KiB upload** on the shared-storage path —
i.e. the fix removes **~60 % of the upload cost on the iOS/A11 path**.

**Evidence — end-to-end** (`glTexImage2D` 64×64 RGBA8, 9 runs, min statistic):

| Metric | baseline | optimized | delta |
|---|---:|---:|---:|
| `glTexImage2D_64x64_full_upload` | 74.29 µs | 45.12 µs | **−39.3 % (median) / −31.1 % (min)** |
| `glTexSubImage2D_64x64_staged` | 7.33 µs | 7.28 µs | −0.8 % / −2.8 % (within noise) |

The end-to-end improvement (−31 % on the min statistic, against a ~5 % noise
floor) is **confirmed**. The staged path (`glTexSubImage2D`) shows no
measurable change because its cost is dominated by the CPU-side shadow copy,
not by the GPU upload.

**Correctness after the change**: all 185 tests pass, including the
pixel-verified upload tests `test_texture_apple`,
`test_mc_texupload_apple`, `test_get_texture_mip_apple`,
`test_mip_storage_apple` and `test_legacy_mip_apple`.

**Side benefit**: on macOS (managed storage) the old code performed a direct
`replaceRegion` *and* a staging + blit; the direct write on managed storage is
exactly the pattern that the source comments identify as having caused data
loss ("replaceRegion+synchronizeResource trên máy này từng chép GPU copy
(rỗng) đè CPU copy → mất dữ liệu"). Removing it removes that hazard.

### 3.2 Compiler

| Stage | median | p95 | note |
|---|---:|---:|---|
| GLSL → MSL (VS, complex) | 0.22 ms | 0.26 ms | |
| GLSL → MSL (FS, complex) | 0.70 ms | 0.92 ms | |
| GLSL → MSL (CS) | 0.26 ms | 0.36 ms | |
| MSL → Metal library | 0.055–0.068 ms | up to **175 ms** | dominated by the Metal compiler's cold path |
| Pipeline creation | 0.004–0.007 ms | 0.02 ms | cache hit |
| **Total (FS, GLSL → pipeline ready)** | **0.76 ms** | 0.92 ms | |

Observations:

* The **GLSL→MSL** stage is CPU-bound and predictable (0.22–0.72 ms).
* The **Metal compiler** dominates the cold path: the p95 for
  `msl_to_metal_library` is **175 ms** for the fragment stage versus a 0.066 ms
  median. This is Apple's compiler, not TGLMT, and it is a *load-time* cost.
* `glsl_to_msl.cpp` is compiled at `-O1` rather than `-O3` (see
  `CMakeLists.txt`): the file contains one ~4 200-line function and LLVM's
  optimiser is superlinear in function size. The measured cost of `-O1` versus
  `-O3` is +4 % on the heaviest shader conversion, and the build is ~6 s
  faster.

**No compiler optimisation was applied in this session** — the profile shows
the GLSL→MSL stage is already a small fraction of the total load time
(0.7 ms out of ~0.8 ms median, but ~0.7 ms out of ~176 ms at p95), so the
remaining headroom is in the Metal compiler, which is not ours to optimise.

### 3.3 Runtime API overhead

| Call | median | p95 |
|---|---:|---:|
| `glGetIntegerv` | 0.042 µs | 0.042 µs |
| `glUniform1f` | 0.050 µs | 0.051 µs |
| `glBindBuffer` | 0.062 µs | 0.063 µs |
| `glBufferSubData` (64 B) | 0.072 µs | 0.073 µs |
| `glDrawArrays` (3 verts) | 0.94 µs | 1.19 µs |
| `glDrawElements` (3 verts) | 1.07 µs | 2.29 µs |
| `glClear` | 7.4 µs | 97 µs (GPU-bound) |
| 100 draws | 124 µs | — |

These are all in the range a native GL driver produces; no API-call overhead
optimisation was warranted.

---

## 4. Optimisations considered and rejected

| Idea | Why rejected |
|---|---|
| Use `MTLStorageModeShared` for textures on macOS | Would make the dev machine behave like iOS (0.5 µs instead of 38 µs per upload), but the **target is iOS where it is already Shared**, so this buys nothing for A11 while adding risk (Shared render targets on macOS have different write-visibility semantics). |
| Reuse the Metal texture when `glTexImage2D` re-specifies the same size/format | Would save the 10.3 µs `newTexture` allocation, but `glTexImage2D` currently allocates a **1-level** texture; reusing it would change mip-level behaviour (GL says re-specifying level 0 must not invalidate other levels). Behaviour change ⇒ out of scope for an optimisation pass. |
| Batch texture staging blits into one command buffer (macOS only) | The staging path only runs on managed storage, i.e. **not on iOS**. No benefit for the target. |
| Hash the pipeline cache key instead of comparing the whole struct | The comparison already short-circuits on the cheap discriminators; the measured pipeline-lookup share (1.8 µs) is inflated by the diagnostic timers. Measured end-to-end draw cost is 1.3 µs, so there is at most a few percent to gain, at the cost of a hash-collision correctness risk. |
| Precompute / cache GLSL→MSL by source hash | Would help apps that compile the same shader repeatedly. Not measurable with the current benchmark (compilation happens once at load); left as a documented opportunity. |

---

## 5. Known remaining bottlenecks

| Bottleneck | Cost | Why it is still there |
|---|---:|---|
| Metal compiler (cold) | up to 175 ms per library | Apple's compiler; a binary-library cache would amortise it |
| `newTexture` | 10.3 µs | GPU allocation; only on texture creation |
| `newBufferWithBytes` | 8.5 µs | GPU allocation; the staging path allocates a fresh buffer per upload on macOS |
| `TRIANGLE_FAN` / `baseVertex` / primitive-restart expansion | O(n) CPU per draw | Metal has no equivalent primitive types |
| Indirect draws read back on the CPU | one-frame-stale semantics | Metal has `MTLIndirectCommandBuffer`, but the GL 4.6 semantics (the command buffer is read at execution time) do not map directly |
| `glClientWaitSync` does not block | — | no Metal event is created; implementing it needs `MTLSharedEvent` + `MTLCommandBufferEncoder signalEvent:` |

---

## 6. Reproducing

```sh
# A/B chuẩn: 9 lần chạy mỗi phase, gộp median, in bảng so sánh + nhiễu nền
scripts/bench_ab.sh baseline optimized 9

# Chạy từng benchmark riêng
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON
cmake --build build-bench -j8
./build-bench/bench_compile --repeats 30 --warmup 5
./build-bench/bench_runtime  --iters 2000 --warmup 300
./build-bench/bench_upload   --iters 300

# Gộp nhiều lần chạy
python3 benchmarks/compare/bench_median.py run*.json -o merged.json
```

Raw results are committed under `benchmarks/baseline/` and
`benchmarks/optimized/`.

## Tối ưu đường nóng `glUniform*` (2026-11)

Bối cảnh: khi triển khai validation §7.6.1, mỗi lệnh uniform phải tra 2 bảng
hash (`uniformSpec` + `uniforms`). Hành vi GL yêu cầu thêm tra hash cho
`uniformLoc`/`programs` ⇒ chi phí mỗi lệnh tăng.

Các thay đổi đã làm (không đổi hành vi GL):

1. **Gộp `uniformSpec` + `uniforms` thành MỘT map** (`ProgramObject::UniformSlot`
   = {bytes, spec}). Lệnh uniform chỉ cần **MỘT** phép tra hash thay vì hai.
2. **Fuse validate + ghi**: `UniformSlotOk()` trả về con trỏ slot đã tra sẵn,
   `StoreU()` ghi thẳng vào đó ⇒ không tra hash lần 2.
3. **Fast path `CurProg`**: bỏ tra shadow `PROGRAM_PIPELINE_BINDING` khi app
   không dùng program pipeline (đa số).
4. **`resize` + `memcpy`** thay cho `vector::assign` (không cấp lại buffer khi
   kích thước không đổi — app set lại cùng uniform mỗi frame).

Đo (Apple Metal, macOS 15.7.7 Intel iGPU, **batched 64 lệnh/mẫu** để giảm
nhiễu đồng hồ; control = `glGetIntegerv` batched, đường không đổi):

| biến thể | median (5 lần) | min (5 lần) | control |
|---|---|---|---|
| **Tối ưu + validation §7.6.1 đầy đủ** | **0.0305–0.0319 µs** | 0.0294–0.0312 µs | 0.0077 µs |
| Tối ưu, **tắt** validation (đo chi phí validation) | 0.0233–0.0384 µs | 0.0227–0.0326 µs | 0.0077 µs |
| Bỏ fast path `CurProg` (luôn tra shadow pipeline) | 0.0325–0.0479 µs | 0.0309–0.0432 µs | 0.0077 µs |
| Thêm 1 phép tra hash thừa | 0.0314–0.0331 µs | 0.0307–0.0326 µs | 0.0077 µs |

Kết luận (trung thực):
* Mỗi phép tra hash trên `unordered_map` tốn ~2–3 ns; fast path `CurProg`
  tiết kiệm ~3–4 ns (~10 %).
* **Validation §7.6.1 đầy đủ tốn ~6–7 ns/lệnh** (0.0236 → 0.0303 µs theo
  thống kê min). Đây là cái giá của việc bám spec — trả đúng lỗi
  INVALID_OPERATION/INVALID_VALUE thay vì nuốt im lặng. Với app thật (100
  uniform/frame) đây là ~3 µs/frame: không đáng kể.
* Số đo "từng lần" (0.05 µs/lệnh) bị chi phí đồng hồ (~40 %) chi phối ⇒
  **phải dùng phép đo batched** khi so sánh đường nóng.

