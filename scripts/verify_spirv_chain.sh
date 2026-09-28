#!/bin/sh
# Chứng thực chuỗi GLSL460 -> SPIR-V -> MSL -> AIR (ARB_gl_spirv, M4).
# Yêu cầu: glslangValidator, spirv-cross, Xcode metal. FAIL rõ ràng nếu thiếu.
set -e
command -v glslangValidator >/dev/null || { echo "THIEU glslangValidator (brew install glslang)"; exit 2; }
command -v spirv-cross >/dev/null || { echo "THIEU spirv-cross (brew install spirv-cross)"; exit 2; }
OUT="${1:-build/spirv-chain}"
mkdir -p "$OUT"
glslangValidator -V shaders/glsl/tri.vert -o "$OUT/tri.vert.spv"
glslangValidator -V shaders/glsl/tri.frag -o "$OUT/tri.frag.spv"
spirv-cross --msl "$OUT/tri.vert.spv" --output "$OUT/tri.msl"
spirv-cross --msl "$OUT/tri.frag.spv" --output "$OUT/tri.frag.msl"
xcrun -sdk macosx metal -c "$OUT/tri.msl" -o "$OUT/tri.air"
xcrun -sdk macosx metal -c "$OUT/tri.frag.msl" -o "$OUT/tri.frag.air"
echo "SPIRV-CHAIN OK: $OUT"
