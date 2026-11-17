#!/usr/bin/env python3
"""Sinh `tools/glsl_builtin_funcs.inc` — bảng case probe cho GLSL built-in FUNCTIONS.

Mỗi dòng: (tên, mã GLSL gọi hàm, kiểu kết quả mong đợi).
Kết quả mong đợi dùng để sinh phép gán hợp lệ trong shader probe.

Nguồn: OpenGL Shading Language 4.60 spec §8 "Built-in Functions"
(docs/khronos/GLSLangSpec.4.60.pdf) — §8.1 … §8.13.
"""
import pathlib
import sys

# (tên, lời gọi, kiểu kết quả)
#   "float" | "vec2".."vec4" | "int" | "uint" | "bool" | "mat4" | "void"
#   "sampler" (giữ nguyên biến sampler) | "i/u/b" biến thể vector
F = [
    # ---- §8.1 Angle and Trigonometric ----
    ("radians", "radians(1.0)", "float"),
    ("degrees", "degrees(1.0)", "float"),
    ("sin", "sin(1.0)", "float"),
    ("cos", "cos(1.0)", "float"),
    ("tan", "tan(1.0)", "float"),
    ("asin", "asin(0.5)", "float"),
    ("acos", "acos(0.5)", "float"),
    ("atan", "atan(1.0)", "float"),
    ("atan(y,x)", "atan(1.0, 2.0)", "float"),
    ("sinh", "sinh(0.5)", "float"),
    ("cosh", "cosh(0.5)", "float"),
    ("tanh", "tanh(0.5)", "float"),
    ("asinh", "asinh(0.5)", "float"),
    ("acosh", "acosh(1.5)", "float"),
    ("atanh", "atanh(0.5)", "float"),
    # ---- §8.2 Exponential ----
    ("pow", "pow(2.0, 3.0)", "float"),
    ("exp", "exp(1.0)", "float"),
    ("log", "log(2.0)", "float"),
    ("exp2", "exp2(1.0)", "float"),
    ("log2", "log2(2.0)", "float"),
    ("sqrt", "sqrt(4.0)", "float"),
    ("inversesqrt", "inversesqrt(4.0)", "float"),
    # ---- §8.3 Common ----
    ("abs", "abs(-1.0)", "float"),
    ("sign", "sign(-1.0)", "float"),
    ("floor", "floor(1.5)", "float"),
    ("trunc", "trunc(1.5)", "float"),
    ("round", "round(1.5)", "float"),
    ("roundEven", "roundEven(1.5)", "float"),
    ("ceil", "ceil(1.5)", "float"),
    ("fract", "fract(1.5)", "float"),
    ("mod", "mod(5.0, 3.0)", "float"),
    ("modf", "modf(1.5, out float ip)", "float"),
    ("min", "min(1.0, 2.0)", "float"),
    ("max", "max(1.0, 2.0)", "float"),
    ("clamp", "clamp(1.0, 0.0, 2.0)", "float"),
    ("mix", "mix(0.0, 1.0, 0.5)", "float"),
    ("step", "step(0.5, 1.0)", "float"),
    ("smoothstep", "smoothstep(0.0, 1.0, 0.5)", "float"),
    ("isnan", "isnan(1.0)", "bool"),
    ("isinf", "isinf(1.0)", "bool"),
    ("floatBitsToInt", "floatBitsToInt(1.0)", "int"),
    ("floatBitsToUint", "floatBitsToUint(1.0)", "uint"),
    ("intBitsToFloat", "intBitsToFloat(1)", "float"),
    ("uintBitsToFloat", "uintBitsToFloat(1u)", "float"),
    ("fma", "fma(1.0, 2.0, 3.0)", "float"),
    ("frexp", "frexp(1.5, out int e)", "float"),
    ("ldexp", "ldexp(1.5, 2)", "float"),
    ("packUnorm2x16", "packUnorm2x16(vec2(0.5))", "uint"),
    ("packSnorm2x16", "packSnorm2x16(vec2(0.5))", "uint"),
    ("packUnorm4x8", "packUnorm4x8(vec4(0.5))", "uint"),
    ("packSnorm4x8", "packSnorm4x8(vec4(0.5))", "uint"),
    ("unpackUnorm2x16", "unpackUnorm2x16(1u)", "vec2"),
    ("unpackSnorm2x16", "unpackSnorm2x16(1u)", "vec2"),
    ("unpackUnorm4x8", "unpackUnorm4x8(1u)", "vec4"),
    ("unpackSnorm4x8", "unpackSnorm4x8(1u)", "vec4"),
    ("packHalf2x16", "packHalf2x16(vec2(0.5))", "uint"),
    ("unpackHalf2x16", "unpackHalf2x16(1u)", "vec2"),
    ("packDouble2x32", "packDouble2x32(uvec2(1u))", "double"),
    ("unpackDouble2x32", "unpackDouble2x32(1.0L)", "uvec2"),
    # ---- §8.4 Geometric ----
    ("length", "length(vec3(1.0))", "float"),
    ("distance", "distance(vec3(1.0), vec3(2.0))", "float"),
    ("dot", "dot(vec3(1.0), vec3(2.0))", "float"),
    ("cross", "cross(vec3(1.0), vec3(2.0))", "vec3"),
    ("normalize", "normalize(vec3(1.0))", "vec3"),
    ("faceforward", "faceforward(vec3(1.0), vec3(1.0), vec3(1.0))", "vec3"),
    ("reflect", "reflect(vec3(1.0), vec3(0.0, 1.0, 0.0))", "vec3"),
    ("refract", "refract(vec3(1.0), vec3(0.0, 1.0, 0.0), 0.5)", "vec3"),
    # ---- §8.5 Matrix ----
    ("matrixCompMult", "matrixCompMult(mat4(1.0), mat4(2.0))", "mat4"),
    ("outerProduct", "outerProduct(vec3(1.0), vec3(2.0))", "mat3"),
    ("transpose", "transpose(mat3(1.0))", "mat3"),
    ("determinant", "determinant(mat3(1.0))", "float"),
    ("inverse", "inverse(mat3(1.0))", "mat3"),
    # ---- §8.6 Vector Relational ----
    ("lessThan", "lessThan(vec2(1.0), vec2(2.0))", "bvec2"),
    ("lessThanEqual", "lessThanEqual(vec2(1.0), vec2(2.0))", "bvec2"),
    ("greaterThan", "greaterThan(vec2(1.0), vec2(2.0))", "bvec2"),
    ("greaterThanEqual", "greaterThanEqual(vec2(1.0), vec2(2.0))", "bvec2"),
    ("equal", "equal(vec2(1.0), vec2(2.0))", "bvec2"),
    ("notEqual", "notEqual(vec2(1.0), vec2(2.0))", "bvec2"),
    ("any", "any(bvec2(true))", "bool"),
    ("all", "all(bvec2(true))", "bool"),
    ("not", "not(bvec2(true))", "bvec2"),
    # ---- §8.7 Integer ----
    ("uaddCarry", "uaddCarry(1u, 2u, out uint c)", "uint"),
    ("usubBorrow", "usubBorrow(2u, 1u, out uint b)", "uint"),
    ("umulExtended", "umulExtended(2u, 3u, out uint hi, out uint lo)", "void"),
    ("imulExtended", "imulExtended(2, 3, out int hi, out int lo)", "void"),
    ("bitfieldExtract", "bitfieldExtract(5, 0, 2)", "int"),
    ("bitfieldInsert", "bitfieldInsert(5, 1, 2, 3)", "int"),
    ("bitfieldReverse", "bitfieldReverse(5)", "int"),
    ("bitCount", "bitCount(5)", "int"),
    ("findLSB", "findLSB(5)", "int"),
    ("findMSB", "findMSB(5)", "int"),
    # ---- §8.8 Texture ----
    ("texture", "texture(t_s, vec2(0.0))", "vec4"),
    ("textureProj", "textureProj(t_s, vec3(0.0))", "vec4"),
    ("textureLod", "textureLod(t_s, vec2(0.0), 0.0)", "vec4"),
    ("textureOffset", "textureOffset(t_s, vec2(0.0), ivec2(1))", "vec4"),
    ("texelFetch", "texelFetch(t_s, ivec2(0), 0)", "vec4"),
    ("textureProjOffset", "textureProjOffset(t_s, vec3(0.0), ivec2(1))", "vec4"),
    ("textureLodOffset", "textureLodOffset(t_s, vec2(0.0), 0.0, ivec2(1))", "vec4"),
    ("textureProjLod", "textureProjLod(t_s, vec3(0.0), 0.0)", "vec4"),
    ("textureProjLodOffset", "textureProjLodOffset(t_s, vec3(0.0), 0.0, ivec2(1))", "vec4"),
    ("textureGrad", "textureGrad(t_s, vec2(0.0), vec2(1.0), vec2(1.0))", "vec4"),
    ("textureGradOffset", "textureGradOffset(t_s, vec2(0.0), vec2(1.0), vec2(1.0), ivec2(1))", "vec4"),
    ("textureSize", "textureSize(t_s, 0)", "ivec2"),
    ("textureQueryLod", "textureQueryLod(t_s, vec2(0.0))", "vec2"),
    ("textureQueryLevels", "textureQueryLevels(t_s)", "int"),
    ("textureGather", "textureGather(t_s, vec2(0.0))", "vec4"),
    ("textureGatherOffset", "textureGatherOffset(t_s, vec2(0.0), ivec2(1))", "vec4"),
    ("textureGatherOffsets", "textureGatherOffsets(t_s, vec2(0.0), ivec2(1), ivec2(2), ivec2(3))", "vec4"),
    # ---- §8.9 Atomic ----
    ("atomicAdd", "atomicAdd(t_ac, 1u)", "uint"),
    ("atomicMin", "atomicMin(t_ac, 1u)", "uint"),
    ("atomicMax", "atomicMax(t_ac, 1u)", "uint"),
    ("atomicAnd", "atomicAnd(t_ac, 1u)", "uint"),
    ("atomicOr", "atomicOr(t_ac, 1u)", "uint"),
    ("atomicXor", "atomicXor(t_ac, 1u)", "uint"),
    ("atomicExchange", "atomicExchange(t_ac, 1u)", "uint"),
    ("atomicCompSwap", "atomicCompSwap(t_ac, 1u, 2u)", "uint"),
    ("atomicCounter", "atomicCounter(t_ac)", "uint"),
    ("atomicCounterIncrement", "atomicCounterIncrement(t_ac)", "uint"),
    ("atomicCounterDecrement", "atomicCounterDecrement(t_ac)", "uint"),
    ("atomicLoad", "atomicLoad(t_ac)", "uint"),
    ("atomicStore", "atomicStore(t_ac, 1u)", "void"),
    # ---- §8.11 Fragment processing ----
    ("dFdx", "dFdx(1.0)", "float"),
    ("dFdy", "dFdy(1.0)", "float"),
    ("fwidth", "fwidth(1.0)", "float"),
    ("interpolateAtCentroid", "interpolateAtCentroid(1.0)", "float"),
    ("interpolateAtSample", "interpolateAtSample(1.0, 0)", "float"),
    ("interpolateAtOffset", "interpolateAtOffset(1.0, vec2(0.0))", "float"),
    # ---- §8.12 Noise (deprecated) ----
    ("noise1", "noise1(vec3(0.5))", "float"),
    ("noise2", "noise2(vec3(0.5))", "vec2"),
    ("noise3", "noise3(vec3(0.5))", "vec3"),
    ("noise4", "noise4(vec3(0.5))", "vec4"),
    # ---- §8.13 Shader invocation control ----
    ("barrier", "barrier()", "void"),
    # ---- GLSL 1.10–1.30 legacy texture (vẫn hợp lệ ở profile compatibility) ----
    ("texture2D", "texture2D(t_s, vec2(0.0))", "vec4"),
    ("texture2DProj", "texture2DProj(t_s, vec3(0.0))", "vec4"),
    ("texture2DLod", "texture2DLod(t_s, vec2(0.0), 0.0)", "vec4"),
    ("texture2DProjLod", "texture2DProjLod(t_s, vec3(0.0), 0.0)", "vec4"),
    ("texture2DGrad", "texture2DGrad(t_s, vec2(0.0), vec2(1.0), vec2(1.0))", "vec4"),
    ("texture2DProjGrad", "texture2DProjGrad(t_s, vec3(0.0), vec2(1.0), vec2(1.0))", "vec4"),
    ("texture2DLodOffset", "texture2DLodOffset(t_s, vec2(0.0), 0.0, ivec2(1))", "vec4"),
    ("textureCube", "textureCube(t_c, vec3(0.0))", "vec4"),
    ("textureCubeLod", "textureCubeLod(t_c, vec3(0.0), 0.0)", "vec4"),
    ("texture3D", "texture3D(t_3d, vec3(0.0))", "vec4"),
    ("texture3DProj", "texture3DProj(t_3d, vec3(0.0))", "vec4"),
    ("texture3DLod", "texture3DLod(t_3d, vec3(0.0), 0.0)", "vec4"),
    # ---- GLSL 4.5: mix với bool ----
    ("mix(bool)", "mix(true, false, true)", "bool"),
    # ---- GLSL 4.00+: clamp/others với vector ----
    ("clamp(vec)", "clamp(vec3(0.5), vec3(0.0), vec3(1.0))", "vec3"),
    ("min(vec)", "min(vec3(1.0), vec3(2.0))", "vec3"),
    ("max(vec)", "max(vec3(1.0), vec3(2.0))", "vec3"),
    ("mix(vec)", "mix(vec3(0.0), vec3(1.0), vec3(0.5))", "vec3"),
    ("step(vec)", "step(vec3(0.5), vec3(1.0))", "vec3"),
    ("smoothstep(vec)", "smoothstep(vec3(0.0), vec3(1.0), vec3(0.5))", "vec3"),
    ("fract(vec)", "fract(vec3(1.5))", "vec3"),
    ("sqrt(vec)", "sqrt(vec3(4.0))", "vec3"),
    ("normalize(vec2)", "normalize(vec2(1.0))", "vec2"),
    ("dFdx(vec)", "dFdx(vec3(1.0))", "vec3"),
]

# Kiểu → phép gán hợp lệ trong shader probe.
ASSIGN = {
    "float": "float _r = {expr};",
    "double": "double _r = {expr};",
    "int": "int _r = {expr};",
    "uint": "uint _r = {expr};",
    "bool": "bool _r = {expr};",
    "vec2": "vec2 _r = {expr};",
    "vec3": "vec3 _r = {expr};",
    "vec4": "vec4 _r = {expr};",
    "uvec2": "uvec2 _r = {expr};",
    "bvec2": "bvec2 _r = {expr};",
    "ivec2": "ivec2 _r = {expr};",
    "ivec3": "ivec3 _r = {expr};",
    "ivec4": "ivec4 _r = {expr};",
    "mat3": "mat3 _r = {expr};",
    "mat4": "mat4 _r = {expr};",
    "void": "{expr};",
}

# Kiểu → fragment/vertex; texture cần fragment, atomic/barrier cần compute.
COMPUTE_ONLY = {"atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor",
                "atomicExchange", "atomicCompSwap", "atomicCounter", "atomicCounterIncrement",
                "atomicCounterDecrement", "atomicLoad", "atomicStore", "barrier"}


def main():
    out = ["// SINH TỰ ĐỘNG bằng tools/gen_glsl_builtin_funcs.py — KHÔNG sửa tay.",
           "// Nguồn: GLSL 4.60 spec §8 \"Built-in Functions\".",
           ""]
    for name, call, ret in F:
        if ret not in ASSIGN:
            print(f"kiểu kết quả chưa biết: {ret}", file=sys.stderr)
            return 2
        body = ASSIGN[ret].format(expr=call)
        if name in COMPUTE_ONLY:
            src = ("#version 460\nlayout(local_size_x=1) in;\n"
                   "layout(binding=0) uniform atomic_uint t_ac;\n"
                   "void main(){ " + body + " }\n")
            stage = "kGLSLCs"
        elif (name.startswith("texture") or name.startswith("texelFetch")
              or name.startswith("dFdx") or name.startswith("dFdy")
              or name.startswith("fwidth") or name.startswith("interpolateAt")
              or name.startswith("noise")):
            src = ("#version 460\nuniform sampler2D t_s;\n"
                   "uniform samplerCube t_c;\n"
                   "uniform sampler3D t_3d;\n"
                   "void main(){ " + body + " gl_FragColor = vec4(1.0); }\n")
            stage = "kGLSLFs"
        else:
            src = ("#version 460\nvoid main(){ " + body + " gl_Position = vec4(1.0); }\n")
            stage = "kGLSLVs"
        esc = src.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n')
        out.append(f'    {{"fn", "{name}", "{esc}", {stage}, nullptr, nullptr}},')
    p = (pathlib.Path(__file__).resolve().parent.parent / "tests" / "shaders" /
         "glsl_builtin_funcs.inc")
    p.write_text("\n".join(out) + "\n")
    print(f"[gen_glsl_builtin_funcs] wrote {p} ({len(F)} hàm)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
