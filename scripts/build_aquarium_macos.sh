#!/bin/sh
# Build TGLMT (Apple backend) rồi build app Aquarium macOS, chạy benchmark.
# App CHỈ link lib đã build — không biên dịch lại sources TGLMT.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
echo "== 1. Build TGLMT =="
cmake -S "$ROOT" -B "$ROOT/build-apple" -DTGLMT_APPLE_METAL=ON
cmake --build "$ROOT/build-apple" -j8
echo "== 2. Build Aquarium (link lib đã build) =="
cmake -S "$ROOT/apps/aquarium" -B "$ROOT/apps/aquarium/build" \
  -DTGLMT_INCLUDE_DIR="$ROOT/include" \
  -DTGLMT_LIB="$ROOT/build-apple/libtglmt.a"
cmake --build "$ROOT/apps/aquarium/build" -j8
echo "== 3. Benchmark 300 frames =="
"$ROOT/apps/aquarium/build/AquariumBench.app/Contents/MacOS/AquariumBench" \
  --bench-frames 300 --shaders "$ROOT/apps/aquarium/shaders" 2>&1 | tail -8
