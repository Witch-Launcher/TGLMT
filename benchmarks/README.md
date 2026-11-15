# TGLMT Benchmarks

Two benchmark families, both emitting JSON on stdout so results can be
diffed and aggregated automatically.

| Benchmark | Source | What it measures |
|---|---|---|
| `bench_compile` | `benchmarks/compilation/bench_compile.cpp` | GLSL → MSL → Metal library → pipeline, per stage |
| `bench_upload` | `benchmarks/compilation/bench_upload.mm` | Raw Metal upload costs — **including the iOS/A11 shared-storage path**, so an A11-relevant number can be obtained on any host |
| `bench_runtime` | `benchmarks/runtime/bench_runtime.cpp` | Per-call API overhead, uploads, draw calls, frame cost |
| `bench_translate` | `benchmarks/compilation/bench_translate.cpp` | **GLSL → MSL translation only** (no Metal) — isolates the converter; `--dump <dir>` writes each shader's MSL for byte-identical diffing |

## Building

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON
cmake --build build-bench -j8
```

The benchmarks are **not** CTest gates: their numbers depend on the machine.

## Running

```sh
./build-bench/bench_compile --repeats 30 --warmup 5
./build-bench/bench_translate --repeats 9 --warmup 2
./build-bench/bench_runtime  --iters 2000 --warmup 300
./build-bench/bench_upload   --iters 300
```

Each benchmark prints a JSON document. The `bench_upload` and `bench_runtime`
tools print a `[TGLMT]` diagnostic line on stdout before the JSON, so extract
the JSON with `sed -n '/^{/,$p'`.

## Protocol

| Aspect | Rule |
|---|---|
| Warm-up | discard the first N iterations (200–300) — removes first-touch page faults and Metal pipeline-cache misses |
| Repetitions | **9 full runs** per phase; the reported value is the **median of the 9 medians** |
| Second statistic | the **min** of the 9 runs — robust to interference on a shared host |
| Noise floor | measured from **control metrics** (code paths the change did not touch); on the development host it is ≈ 5 % (min statistic) but up to 47 % (median statistic) |
| Verdict threshold | 2 %; anything inside the noise floor is reported as *not proven* |

## Aggregating and comparing

```sh
# Gộp N lần chạy thành 1 tài liệu (median của median)
python3 benchmarks/compare/bench_median.py run*.json -o merged.json

# So sánh hai phase, kèm báo cáo nhiễu nền từ các metric CONTROL
python3 benchmarks/compare/bench_compare.py \
    benchmarks/baseline/runtime.json benchmarks/optimized/runtime.json \
    --control glBindBuffer glGetIntegerv glUniform1f

# Chạy trọn quy trình A/B (build + 9 lần chạy mỗi phase + so sánh)
scripts/bench_ab.sh baseline optimized 9
```

`bench_compare.py` exits non-zero when any metric regresses beyond the
threshold, so it can be used as a CI gate.

## Directory layout

```
benchmarks/
  compilation/     bench_compile.cpp, bench_upload.mm
  runtime/         bench_runtime.cpp
  compare/         bench_compare.py, bench_median.py
  baseline/        committed baseline results + meta.json (machine, OS, git state)
  optimized/       committed results after the optimisation pass
  compare/         generated A/B tables
```

`meta.json` records the host, OS, build configuration and git state so a
result set can be traced back to the exact tree that produced it.

## What the numbers mean

* `bench_compile` → **load-time** cost. The `msl_to_metal_library` p95
  (up to ~175 ms) is Apple's Metal compiler, not TGLMT.
* `bench_upload` → the `shared_*` metrics are the **iOS/A11** path
  (`MTLStorageModeShared` ⇒ a direct `replaceRegion`); the `managed_*` metric
  is the macOS path (staging buffer + blit + command buffer). The two differ
  by ~70×, which is why the target platform matters.
* `bench_runtime` → **per-frame** cost. `frame_100_draws` is the most
  representative number for an application.
