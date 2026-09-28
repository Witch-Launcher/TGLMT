#!/bin/sh
# configure_ios.sh — Sinh Xcode project cho app iOS (mở bằng Xcode → chọn Team →
# chọn device → Run). Makefile gọi target này; có thể gọi tay.
#   sh scripts/configure_ios.sh [deployment_target] [bundle_id]
#   IOS_DEPLOYMENT_TARGET=15.0 sh scripts/configure_ios.sh
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIB_IPHONEOS="$ROOT/build-iphoneos/libtglmt.a"
DEPLOY_TARGET="${1:-${IOS_DEPLOYMENT_TARGET:-16.0}}"
BUNDLE_ID="${2:-com.tglmt.aquarium}"

if [ ! -f "$LIB_IPHONEOS" ]; then
  echo "THIẾU $LIB_IPHONEOS"
  echo "Build lib cho iOS trước: make lib-ios DEPLOY_TARGET=$DEPLOY_TARGET"
  echo "  cmake -S \"$ROOT\" -B \"$ROOT/build-iphoneos\" -DTGLMT_APPLE_METAL=ON \\"
  echo "    -DCMAKE_TOOLCHAIN_FILE=\"$ROOT/apps/aquarium/ios_toolchain.cmake\" \\"
  echo "    -DIOS_DEPLOYMENT_TARGET=$DEPLOY_TARGET -DTGLMT_BUILD_TESTS=OFF"
  echo "  cmake --build \"$ROOT/build-iphoneos\" -j8"
  exit 1
fi

echo "== configure iOS: minos=$DEPLOY_TARGET bundle=$BUNDLE_ID =="
cmake -S "$ROOT/apps/aquarium" -B "$ROOT/apps/aquarium/build-ios" \
  -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE="$ROOT/apps/aquarium/ios_toolchain.cmake" \
  -DIOS_DEPLOYMENT_TARGET="$DEPLOY_TARGET" \
  -DAQUA_BUNDLE_ID="$BUNDLE_ID" \
  -DAQUA_IOS_MIN_VERSION="$DEPLOY_TARGET" \
  -DTGLMT_INCLUDE_DIR="$ROOT/include" \
  -DTGLMT_LIB="$LIB_IPHONEOS" \
  -DIOS=ON
echo "OK: $ROOT/apps/aquarium/build-ios/AquariumBench.xcodeproj"
echo "Tiếp: make xcode-app   (build unsigned)  |  mở bằng Xcode để ký + deploy (DEPLOY_IOS.md)"
