#!/bin/sh
# build_ipa_trollstore.sh — Đóng IPA unsigned đúng chuẩn cho TrollStore.
# Fix lỗi 181 "Failed to add app to icon cache" do IPA cũ thiếu icon.
# Dùng binary arm64 đã build (build-ios-tmp/AquariumBench), không cần Xcode/sign.
#   sh scripts/build_ipa_trollstore.sh [binary] [output.ipa]
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${1:-$ROOT/build-ios-tmp/AquariumBench}"
OUT="${2:-$ROOT/apps/aquarium/AquariumBench-trollstore.ipa}"
APP_SRC_DIR="$ROOT/apps/aquarium"

if [ ! -f "$BIN" ]; then
  echo "THIẾU binary: $BIN"
  echo "Build trước (VD theo DEPLOY_IOS.md) hoặc chỉ đường dẫn đúng."
  exit 1
fi

TMP="$(mktemp -d)"
APP="$TMP/Payload/AquariumBench.app"
mkdir -p "$APP/shaders" "$APP/Resources/shaders"

echo "== copy binary =="
cp "$BIN" "$APP/AquariumBench"
chmod 755 "$APP/AquariumBench"

# Đọc minos/platform THẬT của binary — không đoán theo biến build.
# LS chỉ đăng ký app khi Info.plist MinimumOSVersion <= iOS của máy; lệch thì
# TrollStore trả 181 (Unable to register) dù mọi thứ khác đã đúng.
BIN_MINOS="$(otool -l "$APP/AquariumBench" | awk '/LC_BUILD_VERSION/{f=1} f&&/minos/{print $2; exit}')"
BIN_PLATFORM="$(otool -l "$APP/AquariumBench" | awk '/LC_BUILD_VERSION/{f=1} f&&/platform/{print $2; exit}')"
BIN_SDK="$(otool -l "$APP/AquariumBench" | awk '/LC_BUILD_VERSION/{f=1} f&&/sdk/{print $2; exit}')"
if [ -z "$BIN_MINOS" ]; then
  echo "LỖI: không đọc được LC_BUILD_VERSION từ binary (có phải build sai arch?)"
  exit 1
fi
if [ "$BIN_PLATFORM" != "2" ]; then
  echo "LỖI: binary platform=$BIN_PLATFORM, phải là 2 (iOS). Build nhầm SDK macOS?"
  exit 1
fi
echo "  binary: platform=$BIN_PLATFORM minos=$BIN_MINOS sdk=$BIN_SDK"

echo "== Info.plist + PkgInfo (đồng bộ MinimumOSVersion với binary) =="
cp "$APP_SRC_DIR/ios_Info.plist" "$APP/Info.plist"
chmod 644 "$APP/Info.plist"
/usr/libexec/PlistBuddy -c "Set :MinimumOSVersion $BIN_MINOS" "$APP/Info.plist" >/dev/null
if [ -n "$BIN_SDK" ]; then
  /usr/libexec/PlistBuddy -c "Set :DTPlatformVersion $BIN_SDK" "$APP/Info.plist" >/dev/null 2>&1 || true
  /usr/libexec/PlistBuddy -c "Set :DTSDKName iphoneos$BIN_SDK" "$APP/Info.plist" >/dev/null 2>&1 || true
fi
# Bản build tay không qua Xcode nên không có TARGETED_DEVICE_FAMILY sinh key này;
# MobileInstallation cần UIDeviceFamily để biết iPhone hay iPad.
/usr/libexec/PlistBuddy -c "Add :UIDeviceFamily array" "$APP/Info.plist" >/dev/null 2>&1 || true
/usr/libexec/PlistBuddy -c "Add :UIDeviceFamily:0 integer 1" "$APP/Info.plist" >/dev/null 2>&1 || true
/usr/libexec/PlistBuddy -c "Add :UIDeviceFamily:1 integer 2" "$APP/Info.plist" >/dev/null 2>&1 || true
if [ -n "${BUNDLE_ID:-}" ]; then
  /usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string $BUNDLE_ID" "$APP/Info.plist" >/dev/null 2>&1 \
    || /usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier $BUNDLE_ID" "$APP/Info.plist" >/dev/null
else
  /usr/libexec/PlistBuddy -c "Add :CFBundleIdentifier string com.tglmt.aquarium" "$APP/Info.plist" >/dev/null
fi
plutil -lint "$APP/Info.plist" >/dev/null
echo "  MinimumOSVersion = $(/usr/libexec/PlistBuddy -c 'Print :MinimumOSVersion' "$APP/Info.plist")"
echo "  CFBundleIdentifier = $(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP/Info.plist")"
echo "  UIDeviceFamily   = $(/usr/libexec/PlistBuddy -c 'Print :UIDeviceFamily' "$APP/Info.plist" | tr '\n' ' ')"
printf 'APPL????' > "$APP/PkgInfo"
chmod 644 "$APP/PkgInfo"

echo "== icons (bundle root, bắt buộc cho TrollStore icon cache) =="
cp "$APP_SRC_DIR"/ios_assets/*.png "$APP/"
# iTunesArtwork không thuộc bundle runtime — xóa khỏi .app nếu copy lố
rm -f "$APP/iTunesArtwork.png" "$APP/AppIcon1024.png" || true
# Giữ lại AppIcon1024 làm iTunes artwork ngoài? Không cần trong .app.

echo "== shaders (đặt cả 2 vị trí để tương thích cũ/mới) =="
cp "$APP_SRC_DIR"/shaders/*.vert "$APP_SRC_DIR"/shaders/*.frag "$APP/shaders/"
cp "$APP_SRC_DIR"/shaders/*.vert "$APP_SRC_DIR"/shaders/*.frag "$APP/Resources/shaders/"

echo "== dọn rác macOS + chuẩn hóa perms trong staging =="
find "$TMP" -name ".DS_Store" -delete || true
find "$TMP" -type d -exec chmod 755 {} + || true
find "$TMP" -type f -name "*.png" -exec chmod 644 {} + || true
find "$TMP" -type f -name "*.vert" -exec chmod 644 {} + || true
find "$TMP" -type f -name "*.frag" -exec chmod 644 {} + || true
chmod 755 "$APP/AquariumBench"

echo "== zip IPA (unsigned, TrollStore tự ldid) =="
rm -f "$OUT"
(cd "$TMP" && zip -qr "$OUT" Payload)
rm -rf "$TMP"

echo "OK: $OUT"
unzip -l "$OUT" | head -n 12
lipo -info "$BIN" || true

if [ "${VERIFY:-1}" = "1" ]; then
  echo "== verify TrollStore (max minos = ${MAX_MINOS:-16.0}) =="
  python3 "$ROOT/scripts/verify_ipa_trollstore.py" "$OUT" --minos "${MAX_MINOS:-16.0}" \
    ${BUNDLE_ID:+--bundle-id "$BUNDLE_ID"}
fi
