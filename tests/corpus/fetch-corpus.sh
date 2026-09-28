#!/bin/sh
# Tai lai shader corpus vanilla de test converter (nhe, khong commit jar).
# Chay tu repo root: sh tests/corpus/fetch-corpus.sh
set -e
VER_JSON_URL="https://piston-meta.mojang.com/mc/game/version_manifest_v2.json"
OUT="tests/corpus"
mkdir -p "$OUT"
VER_JSON="$OUT/.version.json"
CLIENT_JAR="$OUT/.client.jar"
curl -sL "$VER_JSON_URL" -o "$VER_JSON"
PKG_URL=$(python3 -c "import json;d=json.load(open('$VER_JSON'));print([v for v in d['versions'] if v['id']=='26.1.2'][0]['url'])")
curl -sL "$PKG_URL" -o "$OUT/.pkg.json"
CLIENT_URL=$(python3 -c "import json;d=json.load(open('$OUT/.pkg.json'));print(d['downloads']['client']['url'])")
curl -sL "$CLIENT_URL" -o "$CLIENT_JAR"
rm -rf "$OUT/mc-26.1.2-shaders"
mkdir -p "$OUT/mc-26.1.2-shaders"
unzip -o -q "$CLIENT_JAR" "assets/minecraft/shaders/*" -d "$OUT/.extract"
mv "$OUT/.extract/assets/minecraft/shaders" "$OUT/mc-26.1.2-shaders"
rm -rf "$OUT/.extract" "$CLIENT_JAR" "$VER_JSON" "$OUT/.pkg.json"
echo "corpus -> $OUT/mc-26.1.2-shaders"
