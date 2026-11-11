#!/usr/bin/env bash
# scripts/bench_ab.sh — A/B benchmark chuẩn: N lần chạy mỗi phase, gộp median.
#
# Dùng:
#   scripts/bench_ab.sh baseline optimized [runs]
#
# Ghi benchmarks/<phase>/{runtime,compilation,upload}.json (đã gộp) và
# benchmarks/compare/<phase>_vs_<phase>.txt (bảng so sánh).
set -euo pipefail

A="${1:-baseline}"
B="${2:-optimized}"
RUNS="${3:-5}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build-bench"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON >/dev/null
cmake --build "$BUILD" -j8 >/dev/null

run_phase() {
  local phase="$1"; shift
  local tmp; tmp="$(mktemp -d)"
  for i in $(seq 1 "$RUNS"); do
    "$BUILD/bench_runtime" --iters 2000 --warmup 300 2>/dev/null | sed -n '/^{/,$p' > "$tmp/runtime$i.json"
    "$BUILD/bench_compile" --repeats 30 --warmup 5 2>/dev/null | sed -n '/^{/,$p' > "$tmp/compilation$i.json"
    "$BUILD/bench_upload" --iters 300 2>/dev/null | sed -n '/^{/,$p' > "$tmp/upload$i.json"
  done
  mkdir -p "$ROOT/benchmarks/$phase"
  python3 "$ROOT/benchmarks/compare/bench_median.py" "$tmp"/runtime*.json -o "$ROOT/benchmarks/$phase/runtime.json"
  python3 "$ROOT/benchmarks/compare/bench_median.py" "$tmp"/compilation*.json -o "$ROOT/benchmarks/$phase/compilation.json"
  python3 "$ROOT/benchmarks/compare/bench_median.py" "$tmp"/upload*.json -o "$ROOT/benchmarks/$phase/upload.json"
  rm -rf "$tmp"
}

run_phase "$A"
run_phase "$B"

mkdir -p "$ROOT/benchmarks/compare"
{
  echo "=== A/B: $A vs $B ($RUNS runs mỗi phase, median của median) ==="
  echo
  for m in runtime compilation upload; do
    echo "--- $m ---"
    # CONTROL: các metric thuộc đường code KHÔNG bị sửa bởi tối ưu upload
    # texture → chúng đo nhiễu nền của phép đo.
    case "$m" in
      runtime)     CTRL=(glBindBuffer glGetIntegerv glUniform1f glDrawArrays glDrawElements glClear) ;;
      compilation) CTRL=(cs_compute.pipeline_creation) ;;
      upload)      CTRL=(newBufferWithBytes newTexture shared_replaceRegion_x1 managed_staging) ;;
      *)           CTRL=() ;;
    esac
    python3 "$ROOT/benchmarks/compare/bench_compare.py" \
        "$ROOT/benchmarks/$A/$m.json" "$ROOT/benchmarks/$B/$m.json" \
        --control "${CTRL[@]}" || true
    echo
  done
} | tee "$ROOT/benchmarks/compare/${A}_vs_${B}.txt"
