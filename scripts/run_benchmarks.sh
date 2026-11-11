#!/usr/bin/env bash
# scripts/run_benchmarks.sh — chạy toàn bộ benchmark và lưu kết quả JSON.
#
# Dùng:
#   scripts/run_benchmarks.sh <phase>          # phase = baseline | optimized
#   scripts/run_benchmarks.sh baseline -j 8
#
# Kết quả ghi vào benchmarks/<phase>/:
#   compilation.json
#   runtime.json
#   meta.json          (máy, OS, build config, git state)
set -euo pipefail

PHASE="${1:-baseline}"
shift || true
JOBS=""
if [ "${1:-}" = "-j" ]; then JOBS="-j${2:-8}"; shift 2; fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/benchmarks/$PHASE"
mkdir -p "$OUT"

BUILD="$ROOT/build-bench"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON \
      >/dev/null
cmake --build "$BUILD" $JOBS >/dev/null

echo "[bench] compilation → $OUT/compilation.json"
"$BUILD/bench_compile" --repeats 30 --warmup 5 2>/dev/null > "$OUT/compilation.json"

echo "[bench] runtime → $OUT/runtime.json"
"$BUILD/bench_runtime" --iters 3000 --warmup 300 2>/dev/null > "$OUT/runtime.json"

{
  echo "{"
  echo "  \"phase\": \"$PHASE\","
  echo "  \"host\": \"$(uname -srm)\","
  echo "  \"os\": \"$(sw_vers -productVersion 2>/dev/null || echo unknown)\","
  echo "  \"build\": \"Release (-O3 -DNDEBUG), TGLMT_APPLE_METAL=ON\","
  echo "  \"git\": \"$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)$(git -C "$ROOT" status --porcelain | grep -q . && echo '+dirty' || echo '')\","
  echo "  \"date\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\""
  echo "}"
} > "$OUT/meta.json"

echo "[bench] xong: $OUT"
