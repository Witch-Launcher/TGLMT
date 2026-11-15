#!/bin/bash
# agent_syntax_check.sh — kiểm tra cú pháp nhanh cho agent, KHÔNG tạo build dir.
#
# Lý do tồn tại: mỗi build dir riêng của agent tốn ~2 GB, làm đầy phân vùng.
# Agent dùng script này để tự bắt lỗi cú pháp/link-reference ngay trong file
# mình viết; build + ctest đầy đủ do main agent chạy tập trung.
#
# Dùng:
#   tools/agent_syntax_check.sh src/gl/gl_legacy_xxx.cpp [file2.cpp ...]
#   tools/agent_syntax_check.sh --test tests/unit/test_xxx.cpp
#
# Exit 0 = sạch, exit 1 = có lỗi.
set -uo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GEN="$ROOT/build-work/generated"
[ -d "$GEN" ] || GEN="$ROOT/build-work"

MODE="lib"
FILES=()
for a in "$@"; do
  case "$a" in
    --test) MODE="test" ;;
    --lib)  MODE="lib" ;;
    *)      FILES+=("$a") ;;
  esac
done

[ ${#FILES[@]} -eq 0 ] && { echo "usage: $0 [--test] <file.cpp> [...]" >&2; exit 2; }

CXXFLAGS=(-std=gnu++17 -fsyntax-only -DTGLMT_APPLE_METAL=1
          -I"$ROOT/include" -I"$GEN" -I"$ROOT/src")

if [ "$MODE" = "test" ]; then
  # Test dùng test_util.h + GL headers, cần thêm include của tests.
  CXXFLAGS+=(-I"$ROOT/tests" -I"$ROOT/tests/unit")
fi

rc=0
for f in "${FILES[@]}"; do
  if [ ! -f "$f" ]; then
    echo "MISSING: $f"; rc=1; continue
  fi
  # Test file: dùng macro TU_* nếu có để tránh trùng main().
  if [ "$MODE" = "test" ]; then
    out=$(/usr/bin/c++ "${CXXFLAGS[@]}" -Dmain=tglmt_test_main "$f" 2>&1)
  else
    out=$(/usr/bin/c++ "${CXXFLAGS[@]}" "$f" 2>&1)
  fi
  if [ -n "$out" ]; then
    echo "=== $f ==="
    echo "$out" | head -40
    rc=1
  else
    echo "OK  $f"
  fi
done
exit $rc
