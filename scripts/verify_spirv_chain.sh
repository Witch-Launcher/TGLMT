#!/bin/sh
# Chứng thực chuỗi GLSL460 -> SPIR-V -> MSL -> AIR (ARB_gl_spirv, M4).
# Yêu cầu: glslangValidator, spirv-cross, Xcode metal. FAIL rõ ràng nếu thiếu.
# Dùng -G (OpenGL semantics), KHÔNG dùng -V (Vulkan): TGLMT chạy GLSL 4.6 với
# GL binding model; -V sinh descriptor set/binding khác và SPIRV-Cross MSL sẽ
# map sai uniform/attribute. Đây là gate tĩnh cho chuỗi shader runtime P1.
set -e
command -v glslangValidator >/dev/null || { echo "THIEU glslangValidator (brew install glslang)"; exit 2; }
command -v spirv-cross >/dev/null || { echo "THIEU spirv-cross (brew install spirv-cross)"; exit 2; }
OUT="${1:-build/spirv-chain}"
mkdir -p "$OUT"
glslangValidator -G shaders/glsl/tri.vert -o "$OUT/tri.vert.spv"
glslangValidator -G shaders/glsl/tri.frag -o "$OUT/tri.frag.spv"
spirv-cross --msl "$OUT/tri.vert.spv" --output "$OUT/tri.msl"
spirv-cross --msl "$OUT/tri.frag.spv" --output "$OUT/tri.frag.msl"
xcrun -sdk macosx metal -c "$OUT/tri.msl" -o "$OUT/tri.air"
xcrun -sdk macosx metal -c "$OUT/tri.frag.msl" -o "$OUT/tri.frag.air"
echo "SPIRV-CHAIN OK: $OUT"
