#!/bin/sh
# Verify MSL that converter sinh ra bang Metal compiler that (macOS).
# Day la bai test THAT: converter bao OK chua du, metal phai compile duoc.
# Chay tu repo root. Yeu cau: build/ da build, xcrun macOS SDK.
# Tra ve 0 neu TAT CA file pass (gom base + all-defines + bind regression).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
test -f build/libtglmt.a || { echo "chay cmake --build build truoc"; exit 2; }
clang++ -std=c++17 -Iinclude tools/dump_msl_corpus.cpp build/libtglmt.a -o /tmp/tglmt_dumpmsl
rm -rf /tmp/tglmt_mslcorpus && /tmp/tglmt_dumpmsl /tmp/tglmt_mslcorpus >/dev/null
pass=0; fail=0; failed=""
for f in /tmp/tglmt_mslcorpus/*.metal; do
  if xcrun -sdk macosx metal -c "$f" -o /tmp/tglmt_x.air >/tmp/tglmt_metalerr.txt 2>&1; then
    pass=$((pass + 1))
  else
    fail=$((fail + 1)); failed="$failed $(basename "$f")"
  fi
done
echo "MSL metal-verify: PASS=$pass FAIL=$fail"
for f in $failed; do echo "  FAIL $f"; done
# Gom nhom loi de triage nhanh
for f in $failed; do
  echo "===== $f"
  xcrun -sdk macosx metal -c "/tmp/tglmt_mslcorpus/$f" -o /tmp/tglmt_x.air 2>&1 | grep -oE "error: .*" | sort -u | head -6
done
test "$fail" -eq 0
