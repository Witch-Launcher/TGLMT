#!/bin/sh
# Build TGLMT (macOS dev, Null backend mặc định; thêm -DAPPLE=1 để thử Apple bridge).
set -e
GEN=1
if [ "$1" = "--no-gen" ]; then GEN=0; fi
if [ $GEN -eq 1 ]; then
  python3 tools/gen_gl_headers.py
  python3 tools/gen_stubs.py
fi
cmake -S . -B build ${APPLE:+-DTGLMT_APPLE_METAL=ON}
cmake --build build -j8
ctest --test-dir build --output-on-failure
