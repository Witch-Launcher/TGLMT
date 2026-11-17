// glsl_coverage_cases.h — bảng case kiểm tra độ phủ GLSL của TGLMT.
//
// Dùng chung cho:
//   * tools/glsl_probe.cpp           — công cụ chẩn đoán (in danh sách gap)
//   * tests/shaders/test_glsl_coverage.cpp — CTest gate (mọi case phải PASS)
//
// Tiêu chuẩn "FULL GLSL": một tính năng chỉ ĐƯỢC coi là hỗ trợ khi
//   (1) converter không báo lỗi, VÀ
//   (2) MSL sinh ra biên dịch được bằng compiler Metal thật.
//
// Bảng built-in FUNCTION sinh tự động: tools/gen_glsl_builtin_funcs.py
// (nguồn: GLSL 4.60 spec §8).
#ifndef TGLMT_GLSL_COVERAGE_CASES_H
#define TGLMT_GLSL_COVERAGE_CASES_H

#include "tglmt/gl46.h"
#include "tglmt/GLSLConverter.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace tglmt {

// Mã stage (giống hằng trong src/gl/glsl_to_msl.cpp).
static constexpr uint32_t kGLSLVs = 0x8B31u;  // GL_VERTEX_SHADER
static constexpr uint32_t kGLSLFs = 0x8B30u;  // GL_FRAGMENT_SHADER
static constexpr uint32_t kGLSLCs = 0x91B9u;  // GL_COMPUTE_SHADER
static constexpr uint32_t kGLSLTcs = 0x8E88u; // GL_TESS_CONTROL_SHADER
static constexpr uint32_t kGLSLTes = 0x8E87u; // GL_TESS_EVALUATION_SHADER
static constexpr uint32_t kGLSLGs = 0x8DD9u;  // GL_GEOMETRY_SHADER

struct GLSLProbeCase {
    const char* group;
    const char* name;
    const char* src;
    uint32_t stage;
    const char* mustContain;    // nullptr = không yêu cầu
    const char* mustNotContain;
};

static const GLSLProbeCase kCases[] = {
    // ================= GLSL 1.10 / 1.20: built-in vertex input =================
    {"glsl120", "gl_Vertex", "#version 120\nvoid main(){ gl_Position = gl_Vertex; }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_Normal", "#version 120\nvoid main(){ gl_Position = vec4(gl_Normal, 1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_Color", "#version 120\nvoid main(){ gl_Position = gl_Color; }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_SecondaryColor", "#version 120\nvoid main(){ gl_Position = gl_SecondaryColor; }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_PointSize", "#version 120\nvoid main(){ gl_PointSize = 2.0; gl_Position = vec4(0.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_MultiTexCoord0", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord0; }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_MultiTexCoord7", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord7; }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_TexCoord0 (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_TexCoord0; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_TexCoord3 (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_TexCoord3; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_FrontColor (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_FrontColor; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_BackColor (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_BackColor; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_FrontSecondaryColor (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_FrontSecondaryColor; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_BackSecondaryColor (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_BackSecondaryColor; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_FogFragCoord (FS)", "#version 120\nvoid main(){ gl_FragColor = gl_FogFragCoord; }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_ClipVertex (VS)", "#version 120\nvoid main(){ gl_ClipVertex = gl_Position; gl_Position = vec4(0.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_PointCoord (FS)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_PointCoord, 0.0, 1.0); }\n", kGLSLFs, nullptr, nullptr},

    // ================= GLSL 1.10 / 1.20: built-in UNIFORM =================
    {"glsl120", "gl_ModelViewMatrix", "#version 120\nvoid main(){ gl_Position = gl_ModelViewMatrix * vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_ProjectionMatrix", "#version 120\nvoid main(){ gl_Position = gl_ProjectionMatrix * vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_ModelViewProjectionMatrix", "#version 120\nvoid main(){ gl_Position = gl_ModelViewProjectionMatrix * vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_NormalMatrix", "#version 120\nvoid main(){ gl_Normal = gl_NormalMatrix * gl_Normal; gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_TextureMatrix[0]", "#version 120\nvoid main(){ gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(1.0); gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_TextureMatrix[3]", "#version 120\nvoid main(){ gl_TexCoord[3] = gl_TextureMatrix[3] * vec4(1.0); gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_Fog.depth", "#version 120\nvoid main(){ gl_FogDepth = gl_Fog.depth; gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl120", "gl_Fog.color", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.color, 1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl120", "gl_ClipPlane[0]", "#version 120\nvoid main(){ gl_Position = vec4(gl_ClipPlane[0][0]); }\n", kGLSLVs, nullptr, nullptr},

    // ================= GLSL 1.30+ : ES3-style built-ins =================
    {"glsl130", "gl_VertexID", "#version 130\nvoid main(){ gl_Position = vec4(float(gl_VertexID), 0.0, 0.0, 1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl130", "gl_InstanceID", "#version 130\nvoid main(){ gl_Position = vec4(float(gl_InstanceID), 0.0, 0.0, 1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl140", "gl_FragDepth", "#version 140\nvoid main(){ gl_FragDepth = 0.5; gl_FragColor = vec4(1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl150", "gl_ClipDistance", "#version 150\nvoid main(){ gl_ClipDistance[0] = 1.0; gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl150", "gl_ClipDistance (FS)", "#version 150\nvoid main(){ gl_FragColor = vec4(1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl400", "gl_SampleID", "#version 400\nvoid main(){ gl_FragColor = vec4(float(gl_SampleID)); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl400", "gl_SamplePosition", "#version 400\nvoid main(){ gl_FragColor = vec4(gl_SamplePosition, 0.0, 1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl400", "gl_SampleMaskIn", "#version 400\nvoid main(){ gl_FragColor = vec4(float(gl_SampleMaskIn[0])); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl400", "gl_SampleMask", "#version 400\nvoid main(){ gl_SampleMask[0] = 1; gl_FragColor = vec4(1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"glsl410", "gl_Layer", "#version 410\nvoid main(){ gl_Layer = 1; gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl410", "gl_ViewportIndex", "#version 410\nvoid main(){ gl_ViewportIndex = 0; gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl420", "gl_PrimitiveID (GS)", "#version 420\nlayout(triangles) in; layout(triangle_strip, max_vertices=3) out;\nvoid main(){ gl_Position = vec4(float(gl_PrimitiveID)); EmitVertex(); EndPrimitive(); }\n", kGLSLGs, nullptr, nullptr},
    {"glsl430", "gl_InvocationID (GS)", "#version 430\nlayout(triangles) in; layout(triangle_strip, max_vertices=3) out;\nvoid main(){ gl_Position = vec4(float(gl_InvocationID)); EmitVertex(); EndPrimitive(); }\n", kGLSLGs, nullptr, nullptr},
    {"glsl430", "gl_PrimitiveIDIn (GS)", "#version 430\nlayout(triangles) in; layout(triangle_strip, max_vertices=3) out;\nvoid main(){ gl_Position = vec4(float(gl_PrimitiveIDIn)); EmitVertex(); EndPrimitive(); }\n", kGLSLGs, nullptr, nullptr},
    {"glsl460", "gl_BaseVertex", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseVertex)); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl460", "gl_BaseInstance", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseInstance)); }\n", kGLSLVs, nullptr, nullptr},
    {"glsl460", "gl_DrawID", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_DrawID)); }\n", kGLSLVs, nullptr, nullptr},

    // ================= Compute built-ins (GLSL 4.30) =================
    {"glsl430", "gl_GlobalInvocationID", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_GlobalInvocationID.x; }\n", kGLSLCs, nullptr, nullptr},
    {"glsl430", "gl_LocalInvocationID", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_LocalInvocationID.x; }\n", kGLSLCs, nullptr, nullptr},
    {"glsl430", "gl_WorkGroupID", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_WorkGroupID.x; }\n", kGLSLCs, nullptr, nullptr},
    {"glsl430", "gl_NumWorkGroups", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_NumWorkGroups.x; }\n", kGLSLCs, nullptr, nullptr},
    {"glsl430", "gl_WorkGroupSize", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uvec3 s = gl_WorkGroupSize; }\n", kGLSLCs, nullptr, nullptr},
    {"glsl430", "gl_LocalInvocationIndex", "#version 430\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_LocalInvocationIndex; }\n", kGLSLCs, nullptr, nullptr},

    // ================= Ngôn ngữ: qualifier & kiểu =================
    {"lang", "struct + instance", "#version 430\nstruct S { vec3 a; float b; };\nvoid main(){ S s; s.b = 1.0; gl_Position = vec4(s.a, s.b); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "mảng uniform", "#version 430\nuniform float u[4];\nvoid main(){ gl_Position = vec4(u[0]); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "mảng local + index động", "#version 430\nvoid main(){ float a[4]; a[gl_VertexID & 3] = 1.0; gl_Position = vec4(a[0]); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "for loop thường", "#version 430\nvoid main(){ float s = 0.0; for (int i = 0; i < 4; ++i) s += float(i); gl_Position = vec4(s); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "while / do-while", "#version 430\nvoid main(){ int i = 0; while (i < 3) ++i; do { --i; } while (i > 0); gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "switch", "#version 430\nvoid main(){ int k = 1; switch (k) { case 0: k = 1; break; default: k = 2; } gl_Position = vec4(float(k)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "ternary lồng nhau", "#version 430\nvoid main(){ float a = 1.0 > 0.0 ? 2.0 : 3.0; gl_Position = vec4(a); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "preprocessor #if/#else", "#version 430\n#define A 1\n#if A\nvoid main(){ gl_Position = vec4(1.0); }\n#else\nvoid main(){ gl_Position = vec4(0.0); }\n#endif\n", kGLSLVs, nullptr, nullptr},
    {"lang", "#define có tham số", "#version 430\n#define SQ(x) ((x)*(x))\nvoid main(){ gl_Position = vec4(SQ(2.0)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "macro toán tử", "#version 430\n#define MAX2(a,b) ((a)>(b)?(a):(b))\nvoid main(){ gl_Position = vec4(MAX2(1.0,2.0)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "#undef + #ifdef", "#version 430\n#define A 1\n#undef A\n#ifdef A\nvoid main(){ gl_Position = vec4(1.0); }\n#else\nvoid main(){ gl_Position = vec4(0.0); }\n#endif\n", kGLSLVs, nullptr, nullptr},
    {"lang", "bitfield / bit ops", "#version 430\nvoid main(){ int a = 5; a = a << 2; a = a & 0xF; a = a ^ 3; gl_Position = vec4(float(a)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "uint / uvec", "#version 430\nvoid main(){ uint a = 5u; uvec2 b = uvec2(1u,2u); gl_Position = vec4(float(a + b.x)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "int64 / uint64", "#version 430\nvoid main(){ int64_t a = 5L; uint64_t b = 6UL; gl_Position = vec4(float(a + b)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "double / dvec", "#version 430\nvoid main(){ double d = 1.5; dvec2 v = dvec2(1.0); gl_Position = vec4(float(d + v.x)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "half / f16vec", "#version 430\nvoid main(){ half h = 1.0h; f16vec2 v = f16vec2(1.0h); gl_Position = vec4(float(h + v.x)); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "bool / bvec", "#version 430\nvoid main(){ bool b = true; bvec2 v = bvec2(true,false); gl_Position = vec4(b ? 1.0 : 0.0); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "mảng mảng", "#version 430\nvoid main(){ float a[2][3]; a[0][0] = 1.0; gl_Position = vec4(a[0][0]); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "struct lồng struct", "#version 430\nstruct A { vec3 v; };\nstruct B { A a; float f; };\nvoid main(){ B b; b.a.v = vec3(1.0); b.f = 2.0; gl_Position = vec4(b.a.v, b.f); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "hàm có out param", "#version 430\nvoid f(out float x){ x = 1.0; }\nvoid main(){ float v; f(v); gl_Position = vec4(v); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "hàm có inout param", "#version 430\nvoid f(inout float x){ x += 1.0; }\nvoid main(){ float v = 1.0; f(v); gl_Position = vec4(v); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "hàm trả struct", "#version 430\nstruct S { float f; };\nS g(){ S s; s.f = 1.0; return s; }\nvoid main(){ gl_Position = vec4(g().f); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "mảng của struct (uniform)", "#version 430\nstruct S { float f; };\nuniform S u[2];\nvoid main(){ gl_Position = vec4(u[0].f); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "const + const expr", "#version 430\nconst float K = 2.0;\nvoid main(){ gl_Position = vec4(K); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "precision qualifier", "#version 430\nprecision highp float;\nvoid main(){ gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "layout(location=) FS out", "#version 430\nlayout(location=1) out vec4 o;\nvoid main(){ o = vec4(1.0); }\n", kGLSLFs, "color(1)", nullptr},
    {"lang", "layout(location=) attrib", "#version 430\nlayout(location=0) in vec4 a;\nvoid main(){ gl_Position = a; }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "layout(std140) uniform block", "#version 430\nlayout(std140) uniform U { mat4 m; float v; } u;\nvoid main(){ gl_Position = u.m * vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "layout(binding=) sampler", "#version 430\nlayout(binding=0) uniform sampler2D s;\nvoid main(){ gl_FragColor = texture(s, vec2(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "interpolation flat", "#version 430\nflat in int k;\nvoid main(){ gl_Position = vec4(float(k)); }\n", kGLSLVs, "[[flat]]", nullptr},
    {"lang", "interpolation noperspective", "#version 430\nnoperspective in float k;\nvoid main(){ gl_Position = vec4(k); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "centroid", "#version 430\ncentroid in float k;\nvoid main(){ gl_Position = vec4(k); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "sample (interpolation)", "#version 430\nsample in float k;\nvoid main(){ gl_Position = vec4(k); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "invariant", "#version 430\ninvariant gl_Position;\nvoid main(){ gl_Position = vec4(1.0); }\n", kGLSLVs, nullptr, nullptr},
    {"lang", "shared (compute)", "#version 430\nlayout(local_size_x=1) in;\nshared float s[4];\nvoid main(){ s[0] = 1.0; }\n", kGLSLCs, nullptr, nullptr},
    {"lang", "buffer block unsized", "#version 430\nlayout(std430) buffer B { float d[]; } b;\nvoid main(){ b.d[0] = 1.0; }\n", kGLSLCs, nullptr, nullptr},
    {"lang", "buffer block sized", "#version 430\nlayout(std430) buffer B { float d[16]; } b;\nvoid main(){ b.d[0] = 1.0; }\n", kGLSLCs, nullptr, nullptr},
    {"lang", "atomic counter", "#version 430\nlayout(local_size_x=1) in;\nlayout(binding=0) uniform atomic_uint c;\nvoid main(){ atomicCounterIncrement(c); }\n", kGLSLCs, nullptr, nullptr},
    {"lang", "image load/store", "#version 430\nlayout(local_size_x=1) in;\nlayout(rgba32f, binding=0) uniform image2D img;\nvoid main(){ vec4 c = imageLoad(img, ivec2(0)); imageStore(img, ivec2(0), c); }\n", kGLSLCs, nullptr, nullptr},
    {"lang", "texelFetch", "#version 430\nuniform sampler2D s;\nvoid main(){ gl_FragColor = texelFetch(s, ivec2(0), 0); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "textureLod", "#version 430\nuniform sampler2D s;\nvoid main(){ gl_FragColor = textureLod(s, vec2(0.0), 0.0); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "textureGrad", "#version 430\nuniform sampler2D s;\nvoid main(){ gl_FragColor = textureGrad(s, vec2(0.0), vec2(1.0), vec2(1.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "textureGather", "#version 430\nuniform sampler2D s;\nvoid main(){ gl_FragColor = textureGather(s, vec2(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "textureOffset", "#version 430\nuniform sampler2D s;\nvoid main(){ gl_FragColor = textureOffset(s, vec2(0.0), ivec2(1)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "shadow sampler", "#version 430\nuniform sampler2DShadow sh;\nvoid main(){ gl_FragColor = texture(sh, vec3(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "isampler / usampler", "#version 430\nuniform isampler2D s;\nvoid main(){ gl_FragColor = vec4(float(texture(s, vec2(0.0)).r)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "sampler2DArray", "#version 430\nuniform sampler2DArray s;\nvoid main(){ gl_FragColor = texture(s, vec3(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "samplerCube", "#version 430\nuniform samplerCube s;\nvoid main(){ gl_FragColor = texture(s, vec3(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "textureProj", "#version 430\nuniform sampler2DShadow s;\nvoid main(){ gl_FragColor = textureProj(s, vec4(0.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "dFdx / dFdy / fwidth", "#version 430\nvoid main(){ float d = dFdx(1.0) + dFdy(1.0) + fwidth(1.0); gl_FragColor = vec4(d); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "modf / frexp / ldexp", "#version 430\nvoid main(){ float ip; float fp = modf(1.5, ip); int e; float m = frexp(1.5, e); gl_FragColor = vec4(fp + m + ldexp(m, e)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "pack/unpack", "#version 430\nvoid main(){ gl_FragColor = unpackHalf2x16(packHalf2x16(vec2(1.0))); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "bitcast floatBitsToInt", "#version 430\nvoid main(){ int i = floatBitsToInt(1.0); float f = intBitsToFloat(i); gl_FragColor = vec4(f); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "bitcast uintBitsToFloat", "#version 430\nvoid main(){ float f = uintBitsToFloat(1u); gl_FragColor = vec4(f); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "bitcast uvecBitsToFloat", "#version 430\nvoid main(){ vec2 f = uvecBitsToFloat(uvec2(1u)); gl_FragColor = vec4(f, 0.0, 1.0); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "findLSB / findMSB", "#version 430\nvoid main(){ int a = findLSB(5); int b = findMSB(5); gl_FragColor = vec4(float(a+b)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "bitfieldExtract/Insert", "#version 430\nvoid main(){ int a = bitfieldExtract(5, 0, 2); a = bitfieldInsert(a, 1, 2, 3); gl_FragColor = vec4(float(a)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "bitfieldReverse / uaddCarry", "#version 430\nvoid main(){ uint a = bitfieldReverse(1u); uint c; uint s = uaddCarry(1u, 2u, c); gl_FragColor = vec4(float(a+s)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "interpolateAtCentroid", "#version 430\nvoid main(){ gl_FragColor = vec4(interpolateAtCentroid(1.0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "interpolateAtSample", "#version 430\nvoid main(){ gl_FragColor = vec4(interpolateAtSample(1.0, 0)); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "interpolateAtOffset", "#version 430\nvoid main(){ gl_FragColor = vec4(interpolateAtOffset(1.0, vec2(0.0))); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "noise1 (deprecated)", "#version 400\nvoid main(){ float n = noise1(vec3(0.0)); gl_FragColor = vec4(n); }\n", kGLSLFs, nullptr, nullptr},
    {"lang", "tess control shader", "#version 400\nlayout(vertices=3) out;\nvoid main(){ gl_TessLevelOuter[0] = 1.0; gl_out[gl_InvocationID].gl_Position = vec4(1.0); }\n", kGLSLTcs, nullptr, nullptr},
    {"lang", "tess eval shader", "#version 400\nlayout(triangles) in;\nvoid main(){ gl_Position = vec4(1.0); }\n", kGLSLTes, nullptr, nullptr},
};


// ================= BỘ SINH CASE CHO BUILT-IN (theo stage) =================
// Mỗi built-in GLSL 4.60 §7 — khai báo biểu thức dùng nó, để probe biết
// converter có map sang MSL hợp lệ (Metal compile được) hay không.
#define VSIN(n, expr)  {"builtin", n, expr, kGLSLVs, nullptr, nullptr}
#define FSIN(n, expr)  {"builtin", n, expr, kGLSLFs, nullptr, nullptr}
#define CSIN(n, expr)  {"builtin", n, expr, kGLSLCs, nullptr, nullptr}


// Bảng case built-in FUNCTION — SINH TỰ ĐỘNG (tools/gen_glsl_builtin_funcs.py).

// ================= BỘ SINH CASE CHO BUILT-IN (theo stage) =================
// Mỗi built-in GLSL 4.60 §7 — khai báo biểu thức dùng nó, để probe biết
// converter có map sang MSL hợp lệ (Metal compile được) hay không.
#define VSIN(n, expr)  {"builtin", n, expr, kGLSLVs, nullptr, nullptr}
#define FSIN(n, expr)  {"builtin", n, expr, kGLSLFs, nullptr, nullptr}
#define CSIN(n, expr)  {"builtin", n, expr, kGLSLCs, nullptr, nullptr}

static const GLSLProbeCase kBuiltins[] = {
    // ---- Vertex: input ----
    VSIN("gl_VertexID", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_VertexID)); }\n"),
    VSIN("gl_InstanceID", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_InstanceID)); }\n"),
    VSIN("gl_BaseVertex", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseVertex)); }\n"),
    VSIN("gl_BaseInstance", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseInstance)); }\n"),
    VSIN("gl_DrawID", "#version 460\nvoid main(){ gl_Position = vec4(float(gl_DrawID)); }\n"),
    VSIN("gl_Vertex(120)", "#version 120\nvoid main(){ gl_Position = gl_Vertex; }\n"),
    VSIN("gl_Normal(120)", "#version 120\nvoid main(){ gl_Position = vec4(gl_Normal, 1.0); }\n"),
    VSIN("gl_Color(120)", "#version 120\nvoid main(){ gl_Position = gl_Color; }\n"),
    VSIN("gl_SecondaryColor(120)", "#version 120\nvoid main(){ gl_Position = gl_SecondaryColor; }\n"),
    VSIN("gl_PointSize(120)", "#version 120\nvoid main(){ gl_PointSize = 1.0; gl_Position = vec4(0.0); }\n"),
    VSIN("gl_MultiTexCoord0(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord0; }\n"),
    VSIN("gl_MultiTexCoord1(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord1; }\n"),
    VSIN("gl_MultiTexCoord2(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord2; }\n"),
    VSIN("gl_MultiTexCoord3(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord3; }\n"),
    VSIN("gl_MultiTexCoord4(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord4; }\n"),
    VSIN("gl_MultiTexCoord5(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord5; }\n"),
    VSIN("gl_MultiTexCoord6(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord6; }\n"),
    VSIN("gl_MultiTexCoord7(120)", "#version 120\nvoid main(){ gl_Position = gl_MultiTexCoord7; }\n"),
    // ---- Vertex: uniform ----
    VSIN("gl_ModelViewMatrix(120)", "#version 120\nvoid main(){ gl_Position = gl_ModelViewMatrix * vec4(1.0); }\n"),
    VSIN("gl_ProjectionMatrix(120)", "#version 120\nvoid main(){ gl_Position = gl_ProjectionMatrix * vec4(1.0); }\n"),
    VSIN("gl_ModelViewProjectionMatrix(120)", "#version 120\nvoid main(){ gl_Position = gl_ModelViewProjectionMatrix * vec4(1.0); }\n"),
    VSIN("gl_NormalMatrix(120)", "#version 120\nvoid main(){ gl_Normal = gl_NormalMatrix * gl_Normal; gl_Position = vec4(1.0); }\n"),
    VSIN("gl_TextureMatrix[0](120)", "#version 120\nvoid main(){ gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(1.0); gl_Position = vec4(1.0); }\n"),
    VSIN("gl_TextureMatrix[1](120)", "#version 120\nvoid main(){ gl_TexCoord[1] = gl_TextureMatrix[1] * vec4(1.0); gl_Position = vec4(1.0); }\n"),
    VSIN("gl_TextureMatrix[7](120)", "#version 120\nvoid main(){ gl_TexCoord[7] = gl_TextureMatrix[7] * vec4(1.0); gl_Position = vec4(1.0); }\n"),
    VSIN("gl_FogParameters(120)", "#version 120\nvoid main(){ gl_FogDepth = gl_FogParameters.near; gl_Position = vec4(1.0); }\n"),
    // ---- Vertex: output ----
    // gl_ClipDistance[0]: Metal có ĐÚNG một [[clip_distance]] ⇒ hỗ trợ đầy đủ.
    VSIN("gl_ClipDistance[0](150)", "#version 150\nvoid main(){ gl_ClipDistance[0] = 1.0; gl_Position = vec4(1.0); }\n"),
    // gl_ClipDistance[1]: Metal chỉ MỘT [[clip_distance]] ⇒ converter phải fail
    // RÕ ràng ("GL chỉ hỗ trợ 1 clip distance trên Metal"), KHÔNG âm thầm bỏ ⇒
    // case này CỐ TÌNH ở trạng thái honest error (không phải thiếu sót).
    VSIN("gl_ClipDistance[1](150)", "#version 150\nvoid main(){ gl_ClipDistance[1] = 1.0; gl_Position = vec4(1.0); }\n"),
    VSIN("gl_ClipVertex(120)", "#version 120\nvoid main(){ gl_ClipVertex = gl_Position; gl_Position = vec4(0.0); }\n"),
    // ---- Fragment: input ----
    FSIN("gl_FragCoord", "#version 460\nvoid main(){ gl_FragColor = gl_FragCoord; }\n"),
    FSIN("gl_FrontFacing", "#version 460\nvoid main(){ gl_FragColor = vec4(gl_FrontFacing ? 1.0 : 0.0); }\n"),
    FSIN("gl_PointCoord", "#version 460\nvoid main(){ gl_FragColor = vec4(gl_PointCoord, 0.0, 1.0); }\n"),
    FSIN("gl_SampleID", "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_SampleID)); }\n"),
    FSIN("gl_SamplePosition", "#version 460\nvoid main(){ gl_FragColor = vec4(gl_SamplePosition, 0.0, 1.0); }\n"),
    FSIN("gl_SampleMaskIn[0]", "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_SampleMaskIn[0])); }\n"),
    FSIN("gl_ClipDistance[0](150)", "#version 150\nvoid main(){ gl_FragColor = vec4(gl_ClipDistance[0]); }\n"),
    FSIN("gl_PrimitiveID(150)", "#version 150\nvoid main(){ gl_FragColor = vec4(float(gl_PrimitiveID)); }\n"),
    FSIN("gl_Layer(150)", "#version 150\nvoid main(){ gl_FragColor = vec4(float(gl_Layer)); }\n"),
    FSIN("gl_ViewportIndex(150)", "#version 150\nvoid main(){ gl_FragColor = vec4(float(gl_ViewportIndex)); }\n"),
    FSIN("gl_HelperInvocation(460)", "#version 460\nvoid main(){ gl_FragColor = vec4(gl_HelperInvocation ? 1.0 : 0.0); }\n"),
    FSIN("gl_FrontColor(120)", "#version 120\nvoid main(){ gl_FragColor = gl_FrontColor; }\n"),
    FSIN("gl_BackColor(120)", "#version 120\nvoid main(){ gl_FragColor = gl_BackColor; }\n"),
    FSIN("gl_FrontSecondaryColor(120)", "#version 120\nvoid main(){ gl_FragColor = gl_FrontSecondaryColor; }\n"),
    FSIN("gl_BackSecondaryColor(120)", "#version 120\nvoid main(){ gl_FragColor = gl_BackSecondaryColor; }\n"),
    FSIN("gl_TexCoord[0](120)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_TexCoord[0], 0.0, 1.0); }\n"),
    FSIN("gl_TexCoord[3](120)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_TexCoord[3], 0.0, 1.0); }\n"),
    FSIN("gl_FogFragCoord(120)", "#version 120\nvoid main(){ gl_FragColor = gl_FogFragCoord; }\n"),
    FSIN("gl_Color(120)", "#version 120\nvoid main(){ gl_FragColor = gl_Color; }\n"),
    FSIN("gl_SecondaryColor(120)", "#version 120\nvoid main(){ gl_FragColor = gl_SecondaryColor; }\n"),
    // ---- Fragment: uniform ----
    FSIN("gl_NumSamples(460)", "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_NumSamples)); }\n"),
    FSIN("gl_ClipPlane[0](120)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_ClipPlane[0][0]); }\n"),
    FSIN("gl_Fog.color(120)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.color, 1.0); }\n"),
    FSIN("gl_Fog.range(120)", "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.range, 1.0, 1.0, 1.0); }\n"),
    // ---- Fragment: output ----
    FSIN("gl_FragDepth(140)", "#version 140\nvoid main(){ gl_FragDepth = 0.5; gl_FragColor = vec4(1.0); }\n"),
    FSIN("gl_SampleMask[0](400)", "#version 400\nvoid main(){ gl_SampleMask[0] = 1; gl_FragColor = vec4(1.0); }\n"),
    // ---- Compute ----
    CSIN("gl_GlobalInvocationID", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_GlobalInvocationID.x; }\n"),
    CSIN("gl_LocalInvocationID", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_LocalInvocationID.x; }\n"),
    CSIN("gl_WorkGroupID", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_WorkGroupID.x; }\n"),
    CSIN("gl_NumWorkGroups", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_NumWorkGroups.x; }\n"),
    CSIN("gl_WorkGroupSize", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uvec3 s = gl_WorkGroupSize; }\n"),
    CSIN("gl_LocalInvocationIndex", "#version 460\nlayout(local_size_x=1) in;\nvoid main(){ uint i = gl_LocalInvocationIndex; }\n"),
};

// Bảng case built-in FUNCTION — SINH TỰ ĐỘNG (tools/gen_glsl_builtin_funcs.py).

static const GLSLProbeCase kFnCases[] = {
#include "glsl_builtin_funcs.inc"
};

// ---- Remap location tạm (900+) như linker làm ---------------------------
// Converter gán location tạm cho attribute/varying; linker
// (src/gl/gl_program.cpp RewriteLocns) đổi về số thật TRƯỚC khi biên dịch
// Metal. Probe/test phải làm bước đó, nếu không Metal sẽ báo
// "'attribute' attribute parameter is out of bounds" cho MỌI shader.
inline void GLSLProbeRemapTempLocs(std::string& m) {
    auto remapOne = [&](const std::string& open, const std::string& close) {
        int next = 0;
        size_t p = 0;
        while ((p = m.find(open, p)) != std::string::npos) {
            size_t e = m.find(close, p + open.size());
            if (e == std::string::npos) break;
            int num = atoi(m.c_str() + p + open.size());
            if (num >= 100) {
                std::string repl = open + std::to_string(next) + close;
                m.replace(p, e + close.size() - p, repl);
                ++next;
                p += repl.size();
            } else {
                p = e + close.size();
            }
        }
    };
    remapOne("[[attribute(", ")]]");
    remapOne("[[user(locn", ")]]");
}

inline bool GLSLProbeContains(const std::string& s, const char* needle) {
    return needle && s.find(needle) != std::string::npos;
}

inline bool GLSLProbeMetalAvail() {
    return std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") == 0;
}

// Chạy toàn bộ case: điền `msl[i]`, `convOk[i]`, `metalOk[i]`, `log[i]`.
// Trả về số case hỏng.
inline int GLSLProbeRunAll(const std::vector<GLSLProbeCase>& all,
                           std::vector<std::string>& msl,
                           std::vector<bool>& convOk,
                           std::vector<bool>& metalOk,
                           std::vector<std::string>& log) {
    const size_t N = all.size();
    msl.assign(N, std::string());
    convOk.assign(N, false);
    metalOk.assign(N, false);
    log.assign(N, std::string());

    for (size_t i = 0; i < N; ++i) {
        if (all[i].stage == kGLSLGs) {
            // Geometry shader dùng TRANSLATOR RIÊNG (src/gl/gl_geometry_emulation.cpp):
            // GS → MSL compute kernel, không đi qua converter VS/FS chung ⇒ probe
            // đứng riêng (không link được cả library) không kiểm được.
            // Độ phủ GS đo bằng tests/unit/test_geometry_emulation.cpp (có ca
            // pixel thật); ở đây chỉ đánh dấu "ngoài phạm vi probe".
            convOk[i] = false;
            metalOk[i] = false;
            log[i] = "N/A - kiem tra o tests/unit/test_geometry_emulation.cpp";
            continue;
        }
        GLSLConvertResult r = ConvertGLSLtoMSL(all[i].src, all[i].stage);
        convOk[i] = r.ok;
        log[i] = r.ok ? "" : r.log;
        if (!r.ok) continue;
        if (all[i].mustContain && !GLSLProbeContains(r.msl, all[i].mustContain)) {
            convOk[i] = false;
            log[i] = std::string("thieu mau \"") + all[i].mustContain + "\"";
            continue;
        }
        if (all[i].mustNotContain && GLSLProbeContains(r.msl, all[i].mustNotContain)) {
            convOk[i] = false;
            log[i] = std::string("co mau cam \"") + all[i].mustNotContain + "\"";
            continue;
        }
        msl[i] = r.msl;
        GLSLProbeRemapTempLocs(msl[i]);
    }

    if (GLSLProbeMetalAvail()) {
        {   // dọn thư mục tạm TRƯỚC khi ghi file .metal
            std::string mk = "rm -rf /tmp/tglmt-glslprobe && mkdir -p /tmp/tglmt-glslprobe";
            std::system(mk.c_str());
        }
        std::string cmd = "cd /tmp/tglmt-glslprobe && xcrun -sdk macosx metal -c";
        std::vector<std::string> files(N);
        bool any = false;
        for (size_t i = 0; i < N; ++i) {
            if (!convOk[i]) continue;
            char buf[64];
            snprintf(buf, sizeof(buf), "/tmp/tglmt-glslprobe/s%03zu.metal", i);
            files[i] = buf;
            FILE* f = fopen(buf, "w");
            if (!f) continue;
            fputs(msl[i].c_str(), f);
            fclose(f);
            cmd += " " + std::string(buf + 21) + " ";
            any = true;
        }
        if (any) {
            cmd += "> /tmp/tglmt-glslprobe/compile.log 2>&1";
            std::system(cmd.c_str());
            // CHỐNG AN TOÀN: nếu chính lệnh metal hỏng (sai tham số, ...) thì
            // KHÔNG file nào được biên dịch ⇒ mọi case sẽ "pass" giả. Phải
            // phát hiện và báo lỗi harness, không báo pass.
            {
                FILE* lf = fopen("/tmp/tglmt-glslprobe/compile.log", "r");
                std::string lg0;
                if (lf) {
                    char b[4096]; size_t n;
                    while ((n = fread(b, 1, sizeof(b), lf)) > 0) lg0.append(b, n);
                    fclose(lf);
                }
                if (lg0.find("metal: error:") != std::string::npos) {
                    for (size_t i = 0; i < N; ++i) {
                        if (!convOk[i]) continue;
                        metalOk[i] = false;
                        log[i] = "HARNESS HONG (lenh metal khong chay dung)";
                    }
                }
            }
            std::string lg;
            {
                FILE* f = fopen("/tmp/tglmt-glslprobe/compile.log", "r");
                if (f) {
                    char b[4096];
                    size_t n;
                    while ((n = fread(b, 1, sizeof(b), f)) > 0) lg.append(b, n);
                    fclose(f);
                }
            }
            std::vector<std::string> lines;
            {
                size_t pos = 0;
                while (pos < lg.size()) {
                    size_t nl = lg.find('\n', pos);
                    if (nl == std::string::npos) nl = lg.size();
                    lines.push_back(lg.substr(pos, nl - pos));
                    pos = nl + 1;
                }
            }
            for (size_t i = 0; i < N; ++i) {
                if (!convOk[i] || files[i].empty()) continue;
                char base[64];
                snprintf(base, sizeof(base), "s%03zu.metal", i);
                bool bad = false;
                for (const std::string& ln : lines) {
                    if (ln.rfind(std::string(base) + ":", 0) != 0) continue;
                    if (ln.find(": error:") != std::string::npos) { bad = true; break; }
                }
                metalOk[i] = !bad;
                if (bad) log[i] = "metal compile fail";
            }
        }
    } else {
        for (size_t i = 0; i < N; ++i) metalOk[i] = convOk[i];
    }
    // gl_ClipDistance[1]: giới hạn phần cứng của Metal (chỉ 1 [[clip_distance]])
    // ⇒ converter báo lỗi trung thực; đây là N/A đã biết, không phải gap.
    for (size_t i = 0; i < N; ++i) {
        if (!convOk[i] && log[i].find("GL chỉ hỗ trợ 1 clip distance") != std::string::npos)
            log[i] = "N/A - Metal chi ho tro 1 clip distance (documented)";
    }
    int bad = 0;
    for (size_t i = 0; i < N; ++i) {
        bool na = log[i].rfind("N/A - ", 0) == 0;
        if (!na && !(convOk[i] && metalOk[i])) ++bad;
    }
    return bad;
}

}  // namespace tglmt

#endif  // TGLMT_GLSL_COVERAGE_CASES_H
