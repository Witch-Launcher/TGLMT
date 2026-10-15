// test_sodium_chunk.cpp — Regression crash 2026-10-15 (Sodium block_layer_opaque):
// `directive không hỗ trợ: #error "Vertex compression must be enabled`.
// Root cause: preprocessor fail directive lạ NGAY CẢ trong nhánh #ifdef TẮT
// (trái spec) + attribute đọc trong helper bị strip + bvec/empty-args/uint-promotion
// + const float[N]. Test cả shader Sodium 0.9.2 THẬT (vs+fs, link + Metal compile).
// Chạy cả 2 backend (Null: bỏ qua Metal compile, vẫn check convert/link).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_sodium_chunk FAIL: %s\n", msg); ++gFails; }
}

static std::shared_ptr<metal::IDevice> MakeDev() {
    auto d = metal::CreateDevice("apple", nullptr);
    if (!d || d->isNull()) return nullptr;
    return d;
}

// Biên dịch MSL (khi có device thật), trả false + in lỗi nếu fail.
static bool CheckMSL(const std::shared_ptr<metal::IDevice>& dev,
                     const std::string& msl, const char* tag) {
    if (!dev) return true; // Null backend: bỏ qua
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) {
        printf("test_sodium_chunk FAIL: MSL %s: %.300s\n", tag, err.c_str());
        return false;
    }
    return true;
}

int main() {
    auto dev = MakeDev();
    // ===== 1. #error trong nhánh TẮT → bỏ qua (đúng spec); nhánh ACTIVE → fail giữ message.
    {
        const char* withDef = "#version 330 core\n"
            "#define USE_VERTEX_COMPRESSION\n"
            "#ifdef USE_VERTEX_COMPRESSION\n"
            "uniform float x;\n"
            "#else\n"
            "#error \"Vertex compression must be enabled\n"
            "#endif\n"
            "void main(){ gl_Position = vec4(x); }\n";
        auto r = ConvertGLSLtoMSL(withDef, 0x8B31);
        Check(r.ok, "ifdef-active #error phải bỏ qua");
        if (r.ok && !CheckMSL(dev, r.msl, "ifdef-active")) ++gFails;
        const char* noDef = "#version 330 core\n"
            "#ifdef USE_VERTEX_COMPRESSION\n"
            "uniform float x;\n"
            "#else\n"
            "#error \"Vertex compression must be enabled\n"
            "#endif\n"
            "void main(){ gl_Position = vec4(0.0); }\n";
        auto r2 = ConvertGLSLtoMSL(noDef, 0x8B31);
        Check(!r2.ok && r2.log.find("Vertex compression") != std::string::npos,
              "ifdef-inactive #error phải fail giữ message");
    }
    // ===== 2. attribute đọc trong helper → thread param (không strip, không treo).
    {
        const char* src = "#version 330 core\n"
            "layout(location=0) in vec2 a_TC;\n"
            "out vec2 v_TC;\n"
            "vec2 dup() { return a_TC + a_TC; }\n"
            "void main(){ v_TC = dup(); gl_Position = vec4(0.0); }\n";
        auto r = ConvertGLSLtoMSL(src, 0x8B31);
        Check(r.ok, "helper attr convert fail");
        if (r.ok) {
            Check(r.msl.find("a_TC") != std::string::npos, "attr mất khỏi MSL");
            if (!CheckMSL(dev, r.msl, "helper-attr")) ++gFails;
        }
    }
    // ===== 3. uint*float implicit + bvec mix (Sodium patterns, minimal).
    {
        const char* src2 = "#version 330 core\n"
            "layout(location=0) in uvec2 a_Pos;\n"
            "out vec4 c;\n"
            "uvec3 f(uvec2 d) { return uvec3(d.x, d.y, 1u); }\n"
            "void main(){ c = vec4(f(a_Pos) * 2.0, 1.0); }\n";
        auto r = ConvertGLSLtoMSL(src2, 0x8B31);
        Check(r.ok, "uint-promote convert fail");
        if (r.ok && !CheckMSL(dev, r.msl, "uint-promote")) ++gFails;
        const char* src3 = "#version 330 core\n"
            "layout(location=0) in uvec2 a_TC;\n"
            "out vec4 c;\n"
            "void main(){ c = vec4(mix(vec2(0.0), vec2(1.0), bool2(a_TC > 1u)), 0.0, 1.0); }\n";
        auto r3 = ConvertGLSLtoMSL(src3, 0x8B31);
        Check(r3.ok, "bvec-mix convert fail");
        if (r3.ok && !CheckMSL(dev, r3.msl, "bvec-mix")) ++gFails;
    }
static const char* kSodiumVS = R"GLSL(
#version 330 core
#define USE_VERTEX_COMPRESSION
#define USE_FOG

const int FOG_SHAPE_SPHERICAL = 0;
const int FOG_SHAPE_CYLINDRICAL = 1;

float linear_fog_value(float vertexDistance, float fogStart, float fogEnd) {
    if (vertexDistance <= fogStart) {
        return 0.0;
    } else if (vertexDistance >= fogEnd) {
        return 1.0;
    }

    return (vertexDistance - fogStart) / (fogEnd - fogStart);
}

float total_fog_value(float sphericalVertexDistance, float cylindricalVertexDistance, float environmentalStart, float environmantalEnd, float renderDistanceStart, float renderDistanceEnd) {
    return max(linear_fog_value(sphericalVertexDistance, environmentalStart, environmantalEnd), linear_fog_value(cylindricalVertexDistance, renderDistanceStart, renderDistanceEnd));
}

vec4 _linearFog(vec4 fragColor, vec2 fragDistance, vec4 fogColor, vec2 environmentFog, vec2 renderFog, float fadeFactor) {
#ifdef USE_FOG
    float fogValue = max(1.0 - fadeFactor, total_fog_value(fragDistance.y, fragDistance.x, environmentFog.x, environmentFog.y, renderFog.x, renderFog.y));
    return vec4(mix(fragColor.rgb, fogColor.rgb, fogValue * fogColor.a), fragColor.a);
#else
    return fragColor;
#endif
}

vec2 getFragDistance(vec3 position) {
    return vec2(max(length(position.xz), abs(position.y)), length(position));
}
layout(std140) uniform u_Globals {
    mat4 u_ProjectionMatrix;
    mat4 u_ModelViewMatrix;

    vec4 u_FogColor;
    vec2 u_EnvironmentFog;
    vec2 u_RenderFog;

    vec2 u_TexelSize;
    vec2 u_TexCoordShrink;

    float u_FadePeriodInv;
    bool u_UseRGSS;
};
// The position of the vertex around the model origin
vec3 _vert_position;

// The block texture coordinate of the vertex
vec2 _vert_tex_diffuse_coord;
vec2 _vert_tex_diffuse_coord_bias;

// The light texture coordinate of the vertex
vec2 _vert_tex_light_coord;

// The color of the vertex
vec4 _vert_color;

// The index of the draw command which this vertex belongs to
uint _draw_id;

// The material bits for the primitive
uint _material_params;

#ifdef USE_VERTEX_COMPRESSION
const uint POSITION_BITS        = 20u;
const uint POSITION_MAX_COORD   = 1u << POSITION_BITS;
const uint POSITION_MAX_VALUE   = POSITION_MAX_COORD - 1u;

const uint TEXTURE_BITS         = 15u;
const uint TEXTURE_MAX_COORD    = 1u << TEXTURE_BITS;
const uint TEXTURE_MAX_VALUE    = TEXTURE_MAX_COORD - 1u;

const float VERTEX_SCALE = 32.0 / float(POSITION_MAX_COORD);
const float VERTEX_OFFSET = -8.0;

in uvec2 a_Position;
in vec4 a_Color;
in uvec2 a_TexCoord;
in uvec4 a_LightAndData;

uvec3 _deinterleave_u20x3(uvec2 data) {
    uvec3 hi = (uvec3(data.x) >> uvec3(0u, 10u, 20u)) & 0x3FFu;
    uvec3 lo = (uvec3(data.y) >> uvec3(0u, 10u, 20u)) & 0x3FFu;

    return (hi << 10u) | lo;
}

vec2 _get_texcoord() {
    return vec2(a_TexCoord & TEXTURE_MAX_VALUE) / float(TEXTURE_MAX_COORD);
}

vec2 _get_texcoord_bias() {
    return mix(vec2(-1.0), vec2(1.0), bvec2(a_TexCoord >> TEXTURE_BITS));
}

void _vert_init() {
    _vert_position = (_deinterleave_u20x3(a_Position) * VERTEX_SCALE) + VERTEX_OFFSET;
    _vert_color = a_Color;
    _vert_tex_diffuse_coord = _get_texcoord();
    _vert_tex_diffuse_coord_bias = _get_texcoord_bias();

    _vert_tex_light_coord = vec2(a_LightAndData.xy) / vec2(256.0);

    _material_params = a_LightAndData[2];
    _draw_id = a_LightAndData[3];
}

#else
#error "Vertex compression must be enabled"
#endif

out vec4 v_Color;
out vec2 v_TexCoord;

flat out uint v_Material;

#ifdef USE_FOG
out vec2 v_FragDistance;
out float fadeFactor;
#endif
uniform sampler2D u_LightTex; // The light map texture sampler

uniform isamplerBuffer u_SectionTimeInfo;

uniform vec3 u_RegionOffset;
uniform int u_CurrentTime;
uniform uint u_RegionID;

uvec3 _get_relative_chunk_coord(uint pos) {
    // Packing scheme is defined by LocalSectionIndex
    return uvec3(pos) >> uvec3(5u, 0u, 2u) & uvec3(7u, 3u, 7u);
}

vec3 _get_draw_translation(uint pos) {
    return _get_relative_chunk_coord(pos) * vec3(16.0);
}

void main() {
    _vert_init();

    // Transform the chunk-local vertex position into world model space
    vec3 translation = u_RegionOffset + _get_draw_translation(_draw_id);
    vec3 position = _vert_position + translation;

#ifdef USE_FOG
    v_FragDistance = getFragDistance(position);

    int chunkId = int(_draw_id);
    int chunkFade = texelFetch(u_SectionTimeInfo, int((u_RegionID * 256u) + uint(chunkId))).r;
    int fadeTime = u_CurrentTime - chunkFade;
    float elapsed = float(fadeTime);
    float fade = clamp(float(u_CurrentTime - chunkFade) * u_FadePeriodInv, 0.0, 1.0);
    fadeFactor = (chunkFade < 0) ? 1.0 : fade;
#endif

    // Transform the vertex position into model-view-projection space
    gl_Position = u_ProjectionMatrix * u_ModelViewMatrix * vec4(position, 1.0);

    // Add the light color to the vertex color, and pass the texture coordinates to the fragment shader
    v_Color = _vert_color * texture(u_LightTex, _vert_tex_light_coord);
    v_TexCoord = (_vert_tex_diffuse_coord_bias * u_TexCoordShrink) + _vert_tex_diffuse_coord; // FMA for precision

    v_Material = _material_params;
}

)GLSL";
static const char* kSodiumFS = R"GLSL(
#version 330 core
#define USE_VERTEX_COMPRESSION
#define USE_FOG

const int FOG_SHAPE_SPHERICAL = 0;
const int FOG_SHAPE_CYLINDRICAL = 1;

float linear_fog_value(float vertexDistance, float fogStart, float fogEnd) {
    if (vertexDistance <= fogStart) {
        return 0.0;
    } else if (vertexDistance >= fogEnd) {
        return 1.0;
    }

    return (vertexDistance - fogStart) / (fogEnd - fogStart);
}

float total_fog_value(float sphericalVertexDistance, float cylindricalVertexDistance, float environmentalStart, float environmantalEnd, float renderDistanceStart, float renderDistanceEnd) {
    return max(linear_fog_value(sphericalVertexDistance, environmentalStart, environmantalEnd), linear_fog_value(cylindricalVertexDistance, renderDistanceStart, renderDistanceEnd));
}

vec4 _linearFog(vec4 fragColor, vec2 fragDistance, vec4 fogColor, vec2 environmentFog, vec2 renderFog, float fadeFactor) {
#ifdef USE_FOG
    float fogValue = max(1.0 - fadeFactor, total_fog_value(fragDistance.y, fragDistance.x, environmentFog.x, environmentFog.y, renderFog.x, renderFog.y));
    return vec4(mix(fragColor.rgb, fogColor.rgb, fogValue * fogColor.a), fragColor.a);
#else
    return fragColor;
#endif
}

vec2 getFragDistance(vec3 position) {
    return vec2(max(length(position.xz), abs(position.y)), length(position));
}
layout(std140) uniform u_Globals {
    mat4 u_ProjectionMatrix;
    mat4 u_ModelViewMatrix;

    vec4 u_FogColor;
    vec2 u_EnvironmentFog;
    vec2 u_RenderFog;

    vec2 u_TexelSize;
    vec2 u_TexCoordShrink;

    float u_FadePeriodInv;
    bool u_UseRGSS;
};
const uint MATERIAL_USE_MIP_OFFSET = 0u;
const uint MATERIAL_ALPHA_CUTOFF_OFFSET = 1u;

const float[4] ALPHA_CUTOFF = float[4](0.0, 0.01, 0.5, 1.0);

bool _material_use_mips(uint material) {
    return ((material >> MATERIAL_USE_MIP_OFFSET) & 1u) != 0u;
}

float _material_alpha_cutoff(uint material) {
    return ALPHA_CUTOFF[(material >> MATERIAL_ALPHA_CUTOFF_OFFSET) & 3u];
}

in vec4 v_Color; // The interpolated vertex color
in vec2 v_TexCoord; // The interpolated block texture coordinates
in vec2 v_FragDistance; // The fragment's distance from the camera (cylindrical and spherical)
in float fadeFactor;

flat in uint v_Material;

uniform sampler2D u_BlockTex; // The block texture

out vec4 fragColor; // The output fragment for the color framebuffer

vec4 sampleNearest(sampler2D sampler, vec2 uv, vec2 pixelSize, vec2 du, vec2 dv, vec2 texelScreenSize) {
    // Convert our UV back up to texel coordinates and find out how far over we are from the center of each pixel
    vec2 uvTexelCoords = uv / pixelSize;
    vec2 texelCenter = round(uvTexelCoords) - 0.5f;
    vec2 texelOffset = uvTexelCoords - texelCenter;

    // Move our offset closer to the texel center based on texel size on screen
    texelOffset = (texelOffset - 0.5f) * pixelSize / texelScreenSize + 0.5f;
    texelOffset = clamp(texelOffset, 0.0f, 1.0f);

    uv = (texelCenter + texelOffset) * pixelSize;
    return textureGrad(sampler, uv, du, dv);
}

vec4 sampleNearest(sampler2D source, vec2 uv, vec2 pixelSize) {
    vec2 du = dFdx(uv);
    vec2 dv = dFdy(uv);
    vec2 texelScreenSize = sqrt(du * du + dv * dv);
    return sampleNearest(source, uv, pixelSize, du, dv, texelScreenSize);
}

// Rotated Grid Super-Sampling
vec4 sampleRGSS(sampler2D source, vec2 uv, vec2 pixelSize) {
    vec2 du = dFdx(uv);
    vec2 dv = dFdy(uv);

    vec2 texelScreenSize = sqrt(du * du + dv * dv);
    float maxTexelSize = max(texelScreenSize.x, texelScreenSize.y);

    float minPixelSize = min(pixelSize.x, pixelSize.y);

    float transitionStart = minPixelSize * 1.0;
    float transitionEnd = minPixelSize * 2.0;
    float blendFactor = smoothstep(transitionStart, transitionEnd, maxTexelSize);

    float duLength = length(du);
    float dvLength = length(dv);
    float minDerivative = min(duLength, dvLength);
    float maxDerivative = max(duLength, dvLength);

    float effectiveDerivative = sqrt(minDerivative * maxDerivative);

    float mipLevelExact = max(0.0, log2(effectiveDerivative / minPixelSize));

    const vec2 offsets[4] = vec2[](
    vec2(0.125, 0.375),
    vec2(-0.125, -0.375),
    vec2(0.375, -0.125),
    vec2(-0.375, 0.125)
    );

    vec4 rgssColor = vec4(0.0);
    for (int i = 0; i < 4; ++i) {
        vec2 sampleUV = uv + offsets[i] * pixelSize;
        rgssColor += textureLod(source, sampleUV, mipLevelExact);
    }
    rgssColor *= 0.25;

    vec4 nearestColor = sampleNearest(source, uv, pixelSize, du, dv, texelScreenSize);

    return mix(nearestColor, rgssColor, blendFactor);
}

void main() {
    vec4 color = u_UseRGSS ? sampleRGSS(u_BlockTex, v_TexCoord, u_TexelSize) : sampleNearest(u_BlockTex, v_TexCoord, u_TexelSize);
    color *= v_Color; // Apply per-vertex color modulator

#ifdef USE_FRAGMENT_DISCARD
    if (color.a < _material_alpha_cutoff(v_Material)) {
        discard;
    }
#endif

    fragColor = _linearFog(color, v_FragDistance, u_FogColor, u_EnvironmentFog, u_RenderFog, fadeFactor);
}

)GLSL";

    // ===== 4. FULL Sodium 0.9.2 block_layer_opaque (vs+fs, link + Metal) — crash repro.
    {
        Context& c = Context::Current();
        GLuint shv = glCreateShader(0x8B31), shf = glCreateShader(0x8B30);
        const char* pvs = kSodiumVS;
        const char* pfs = kSodiumFS;
        glShaderSource(shv, 1, &pvs, nullptr); glCompileShader(shv);
        glShaderSource(shf, 1, &pfs, nullptr); glCompileShader(shf);
        GLint okv = 0, okf = 0;
        glGetShaderiv(shv, 0x8B81, &okv);
        glGetShaderiv(shf, 0x8B81, &okf);
        Check(okv == 1 && okf == 1, "Sodium vs/fs compile fail");
        if (!(okv == 1 && okf == 1)) { printf("test_sodium_chunk: %d FAIL\n", gFails); return 1; }
        GLuint pr = glCreateProgram();
        glAttachShader(pr, shv); glAttachShader(pr, shf); glLinkProgram(pr);
        GLint ok = 0; glGetProgramiv(pr, 0x8B82, &ok);
        Check(ok == 1, "Sodium program link fail");
        if (ok == 1 && dev) {
            auto itp = c.programs.find(pr);
            Check(itp != c.programs.end(), "program missing");
            if (itp != c.programs.end()) {
                if (!CheckMSL(dev, itp->second.vertexMSL, "sodium-vs-linked")) ++gFails;
                if (!CheckMSL(dev, itp->second.fragmentMSL, "sodium-fs-linked")) ++gFails;
            }
        }
        glDeleteProgram(pr);
    }

    // ===== 4b. FULL Sodium cutout (USE_FRAGMENT_DISCARD: discard + alpha cutoff).
    {
        Context& c = Context::Current();
        std::string fsc(kSodiumFS);
        const std::string anchor = "#define USE_FOG\n";
        size_t ap = fsc.find(anchor);
        if (ap != std::string::npos) fsc.insert(ap + anchor.size(), "#define USE_FRAGMENT_DISCARD\n");
        GLuint shv = glCreateShader(0x8B31), shf = glCreateShader(0x8B30);
        const char* pvs = kSodiumVS;
        const char* pfs = fsc.c_str();
        glShaderSource(shv, 1, &pvs, nullptr); glCompileShader(shv);
        glShaderSource(shf, 1, &pfs, nullptr); glCompileShader(shf);
        GLint okv = 0, okf = 0;
        glGetShaderiv(shv, 0x8B81, &okv);
        glGetShaderiv(shf, 0x8B81, &okf);
        Check(okv == 1 && okf == 1, "Sodium cutout vs/fs compile fail");
        if (!(okv == 1 && okf == 1)) { printf("test_sodium_chunk: %d FAIL\n", gFails); return 1; }
        GLuint pr = glCreateProgram();
        glAttachShader(pr, shv); glAttachShader(pr, shf); glLinkProgram(pr);
        GLint ok = 0; glGetProgramiv(pr, 0x8B82, &ok);
        Check(ok == 1, "Sodium cutout program link fail");
        if (ok == 1 && dev) {
            auto itp = c.programs.find(pr);
            Check(itp != c.programs.end(), "cutout program missing");
            if (itp != c.programs.end()) {
                if (!CheckMSL(dev, itp->second.vertexMSL, "sodium-cutout-vs-linked")) ++gFails;
                if (!CheckMSL(dev, itp->second.fragmentMSL, "sodium-cutout-fs-linked")) ++gFails;
            }
        }
        glDeleteProgram(pr);
    }

    if (gFails == 0) { printf("test_sodium_chunk: all OK\n"); return 0; }
    printf("test_sodium_chunk: %d FAIL\n", gFails);
    return 1;
}
