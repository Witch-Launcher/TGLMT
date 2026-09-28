#!/bin/sh
# Tải header metal-cpp chính chủ Apple vào third_party/metal-cpp (chỉ khi build iOS thật).
# Nguồn: https://developer.apple.com/metal/cpp/ — không vendor lậu, tải tại build-time.
set -e
URL="https://developer.apple.com/metal/cpp/files/metal-cpp_macOS15_iOS18.zip"
DST="third_party/metal-cpp"
mkdir -p "$DST"
if [ -d "$DST/metal" ]; then echo "metal-cpp đã có."; exit 0; fi
echo "Tải metal-cpp từ Apple..."
curl -L "$URL" -o /tmp/metal-cpp.zip && unzip -o /tmp/metal-cpp.zip -d "$DST" && echo OK
