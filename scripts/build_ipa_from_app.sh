#!/bin/sh
# build_ipa_from_app.sh — Đóng IPA cho TrollStore từ một bundle .app có sẵn.
# Ưu điểm so với build_ipa_trollstore.sh: .app do chính Xcode/toolchain Apple
# sinh ra (Info.plist binary, PkgInfo, DTXcode, icon convert chuẩn) → giảm rủi ro
# bị LaunchServices từ chối (lỗi 181) so với tự dựng từng file bằng tay.
#   sh scripts/build_ipa_from_app.sh <đường-dẫn/Some.app> [output.ipa]
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_APP="${1:-}"
OUT="${2:-$ROOT/apps/aquarium/AquariumBench-xcode.ipa}"

if [ -z "$SRC_APP" ] || [ ! -d "$SRC_APP" ]; then
  echo "Dùng: sh scripts/build_ipa_from_app.sh <đường-dẫn/Some.app> [out.ipa]"
  echo "VD:  sh scripts/build_ipa_from_app.sh build/xcode-ios/Release-iphoneos/AquariumBench.app"
  exit 1
fi
SRC_APP="$(cd "$SRC_APP" && pwd)"
APP_NAME="$(basename "$SRC_APP" .app)"
# OUT phải là absolute: zip chạy trong subshell đã cd sang staging.
mkdir -p "$(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"

TMP="$(mktemp -d)"
DEST="$TMP/Payload/$APP_NAME.app"
mkdir -p "$TMP/Payload"

echo "== copy .app từ toolchain Apple =="
cp -R "$SRC_APP" "$DEST"

echo "== bỏ chữ ký / provisioning (TrollStore tự ldid lại trên máy) =="
rm -rf "$DEST/_CodeSignature" "$DEST/_EmbeddedSignature" "$DEST/CodeResources"
rm -f "$DEST/embedded.mobileprovision" "$DEST/_MASReceipt" 2>/dev/null || true
find "$DEST" -name ".DS_Store" -delete || true
find "$DEST" -name "*.lproj" -type d -exec rm -rf {} + 2>/dev/null || true

echo "== chuẩn hóa perms (TrollStore fix lại sau, nhưng đóng gói cho sạch) =="
find "$TMP" -type d -exec chmod 755 {} + || true
find "$TMP" -type f -exec chmod 644 {} + || true
chmod 755 "$DEST/$APP_NAME"
test -f "$DEST/PkgInfo" || printf 'APPL????' > "$DEST/PkgInfo"
printf 'APPL????' > "$DEST/PkgInfo"

echo "== đồng bộ MinimumOSVersion với minos thật của binary =="
BIN_MINOS="$(otool -l "$DEST/$APP_NAME" | awk '/LC_BUILD_VERSION/{f=1} f&&/minos/{print $2; exit}')"
BIN_PLATFORM="$(otool -l "$DEST/$APP_NAME" | awk '/LC_BUILD_VERSION/{f=1} f&&/platform/{print $2; exit}')"
if [ "$BIN_PLATFORM" != "2" ]; then
  echo "LỖI: binary platform=$BIN_PLATFORM, phải là 2 (iOS)"
  rm -rf "$TMP"; exit 1
fi
echo "  binary: platform=2 minos=$BIN_MINOS"
/usr/libexec/PlistBuddy -c "Set :MinimumOSVersion $BIN_MINOS" "$DEST/Info.plist"
echo "  MinimumOSVersion = $(/usr/libexec/PlistBuddy -c 'Print :MinimumOSVersion' "$DEST/Info.plist")"

# Bundle ID: giá trị trong Info.plist THẮNG build setting PRODUCT_BUNDLE_IDENTIFIER,
# nên nếu không ghi đè ở đây thì BUNDLE_ID truyền vào sẽ bị bỏ qua âm thầm.
# ios_Info.plist cố ý không khai key này → Xcode tự sinh từ PRODUCT_BUNDLE_IDENTIFIER.
if [ -n "${BUNDLE_ID:-}" ]; then
  /usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string $BUNDLE_ID" "$DEST/Info.plist" >/dev/null 2>&1 \
    || /usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $BUNDLE_ID" "$DEST/Info.plist" >/dev/null
fi
if ! /usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$DEST/Info.plist" >/dev/null 2>&1; then
  echo "LỖI: .app không có CFBundleIdentifier (Xcode không sinh ra?)"
  rm -rf "$TMP"; exit 1
fi
echo "  CFBundleIdentifier = $(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$DEST/Info.plist")"
chmod 644 "$DEST/Info.plist"

echo "== zip IPA =="
rm -f "$OUT"
(cd "$TMP" && zip -qr "$OUT" Payload)
rm -rf "$TMP"

echo "OK: $OUT"
unzip -l "$OUT" | head -n 8

if [ "${VERIFY:-1}" = "1" ]; then
  echo "== verify TrollStore =="
  python3 "$ROOT/scripts/verify_ipa_trollstore.py" "$OUT" --minos "${MAX_MINOS:-16.0}" \
    ${BUNDLE_ID:+--bundle-id "$BUNDLE_ID"}
fi
