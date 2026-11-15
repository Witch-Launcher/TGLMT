#!/bin/sh
# Build TGLMT (macOS dev, Null backend mặc định; thêm -DAPPLE=1 để thử Apple bridge).
set -e
GEN=1
if [ "$1" = "--no-gen" ]; then GEN=0; fi
if [ $GEN -eq 1 ]; then
  python3 tools/gen_gl_headers.py
  python3 tools/gen_version_inventory.py
  python3 tools/gen_version_gate.py
  python3 tools/gen_c_exports.py
  python3 tools/gen_stubs.py
  python3 tools/gen_compat_tables.py
  python3 tools/gen_state_pnames.py
fi
# DÙNG CHUNG build-work (KHÔNG tạo thêm build dir: mỗi build dir ~2 GB và đã
# làm đầy phân vùng nhiều lần — xem docs/DECISIONS.md D-010).
BUILD_DIR=build-work
[ -d "$BUILD_DIR" ] || BUILD_DIR=build
cmake -S . -B "$BUILD_DIR" ${APPLE:+-DTGLMT_APPLE_METAL=ON}
cmake --build "$BUILD_DIR" -j8
ctest --test-dir "$BUILD_DIR" --output-on-failure
# Sinh lại báo cáo coverage (số đo, không viết tay) — cần log ctest -V.
ctest --test-dir "$BUILD_DIR" -V -j8 > /tmp/tglmt_ctest_v.log 2>&1 || true
python3 tools/gen_coverage_report.py --ctest-log /tmp/tglmt_ctest_v.log
