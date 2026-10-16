// test_sodium_fulldraw_apple.cpp — Bug 2: draws encode (dAtt==dEnc, misc=0)
// nhưng terrain vô hình trên máy (cừu+mây render, đất mất).
// Vẽ 1 tam giác bằng Sodium 0.9.2 block_layer_opaque: VS nguyên bản,
// FS rút gọn (bỏ RGSS/discard, giữ varyings/UBO/fog/sampleNearest) — rồi readback.
// compact (uvec2 IPointer), UBO u_Globals, light texture, section-time
// buffer texture — rồi readback. Nếu local ra trắng: đường shader/VAO/UBO
// vô tội, nguyên nhân nằm ở game-state (matrix/camera/cull/depth).
// Null backend: chỉ check link (convert đã có ở test_sodium_chunk).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool c, const char* m) {
    if (!c) { printf("test_sodium_fulldraw_apple FAIL: %s\n", m); ++gFails; }
}

// Đóng gói 3×20bit (Sodium _deinterleave_u20x3 ngược): value=v điền hi/lo 10bit.
static void Pack20(uint32_t v, uint32_t& hi, uint32_t& lo) {
    hi = (v >> 10) & 0x3FFu;
    lo = v & 0x3FFu;
}
static void PackPos(float x, float y, float z, uint32_t out[2]) {
    uint32_t vx = (uint32_t)((x + 8.0f) * 32768.0f);
    uint32_t vy = (uint32_t)((y + 8.0f) * 32768.0f);
    uint32_t vz = (uint32_t)((z + 8.0f) * 32768.0f);
    uint32_t hx, lx, hy, ly, hz, lz;
    Pack20(vx, hx, lx);
    Pack20(vy, hy, ly);
    Pack20(vz, hz, lz);
    out[0] = hx | (hy << 10) | (hz << 20);
    out[1] = lx | (ly << 10) | (lz << 20);
}

static const char* kVS = R"GLSL(
#version 330 core
#define USE_VERTEX_COMPRESSION
#define USE_FOG
float linear_fog_value(float vertexDistance, float fogStart, float fogEnd) {
    if (vertexDistance <= fogStart) { return 0.0; }
    else if (vertexDistance >= fogEnd) { return 1.0; }
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
vec3 _vert_position;
vec2 _vert_tex_diffuse_coord;
vec2 _vert_tex_diffuse_coord_bias;
vec2 _vert_tex_light_coord;
vec4 _vert_color;
uint _draw_id;
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
uniform sampler2D u_LightTex;
uniform isamplerBuffer u_SectionTimeInfo;
uniform vec3 u_RegionOffset;
uniform int u_CurrentTime;
uniform uint u_RegionID;
uvec3 _get_relative_chunk_coord(uint pos) {
    return uvec3(pos) >> uvec3(5u, 0u, 2u) & uvec3(7u, 3u, 7u);
}
vec3 _get_draw_translation(uint pos) {
    return _get_relative_chunk_coord(pos) * vec3(16.0);
}
void main() {
    _vert_init();
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
    gl_Position = u_ProjectionMatrix * u_ModelViewMatrix * vec4(position, 1.0);
    v_Color = _vert_color * texture(u_LightTex, _vert_tex_light_coord);
    v_TexCoord = (_vert_tex_diffuse_coord_bias * u_TexCoordShrink) + _vert_tex_diffuse_coord;
    v_Material = _material_params;
}
)GLSL";

static const char* kFS = R"GLSL(
#version 330 core
#define USE_VERTEX_COMPRESSION
#define USE_FOG
float linear_fog_value(float vertexDistance, float fogStart, float fogEnd) {
    if (vertexDistance <= fogStart) { return 0.0; }
    else if (vertexDistance >= fogEnd) { return 1.0; }
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
in vec4 v_Color;
in vec2 v_TexCoord;
in vec2 v_FragDistance;
in float fadeFactor;
flat in uint v_Material;
uniform sampler2D u_BlockTex;
out vec4 fragColor;
vec4 sampleNearest(sampler2D sampler, vec2 uv, vec2 pixelSize, vec2 du, vec2 dv, vec2 texelScreenSize) {
    vec2 uvTexelCoords = uv / pixelSize;
    vec2 texelCenter = round(uvTexelCoords) - 0.5f;
    vec2 texelOffset = uvTexelCoords - texelCenter;
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
void main() {
    vec4 color = u_UseRGSS ? vec4(1.0) : sampleNearest(u_BlockTex, v_TexCoord, u_TexelSize);
    color *= v_Color;
    fragColor = _linearFog(color, v_FragDistance, u_FogColor, u_EnvironmentFog, u_RenderFog, fadeFactor);
}
)GLSL";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);

    GLuint shv = glCreateShader(0x8B31), shf = glCreateShader(0x8B30);
    glShaderSource(shv, 1, &kVS, nullptr);
    glShaderSource(shf, 1, &kFS, nullptr);
    glCompileShader(shv);
    glCompileShader(shf);
    GLint okv = 0, okf = 0;
    glGetShaderiv(shv, 0x8B81, &okv);
    glGetShaderiv(shf, 0x8B81, &okf);
    Check(okv == 1 && okf == 1, "sodium vs/fs compile");
    GLuint prog = glCreateProgram();
    glAttachShader(prog, shv);
    glAttachShader(prog, shf);
    glBindAttribLocation(prog, 0, "a_Position");
    glBindAttribLocation(prog, 1, "a_Color");
    glBindAttribLocation(prog, 2, "a_TexCoord");
    glBindAttribLocation(prog, 3, "a_LightAndData");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok == 1, "sodium link");
    if (gFails || ctx.device->isNull()) {
        if (gFails == 0) printf("test_sodium_fulldraw_apple PASS (null: link OK)\n");
        else printf("test_sodium_fulldraw_apple: %d FAIL\n", gFails);
        return gFails ? 1 : 0;
    }

    auto target =
        ctx.device->makeRenderTargetWithDepth(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_sodium_fulldraw_apple FAIL: target nil\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    // UBO u_Globals 192B: identity proj/mv, fog alpha 0 (tắt fog), texel 1/4.
    GLuint ubo;
    glGenBuffers(1, &ubo);
    uint8_t gb[192];
    memset(gb, 0, sizeof(gb));
    for (int i = 0; i < 16; ++i) ((float*)gb)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    for (int i = 0; i < 16; ++i) ((float*)(gb + 64))[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    // fogColor (128) giữ 0 → alpha 0 → không fog.
    ((float*)(gb + 160))[0] = 0.25f;
    ((float*)(gb + 160))[1] = 0.25f; // u_TexelSize
    glBindBuffer(0x8A11, ubo);
    glBufferData(0x8A11, sizeof(gb), gb, 0x88E4);
    glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, "u_Globals"), 0);
    glBindBufferBase(0x8A11, 0, ubo);

    // Block texture trắng 4x4 + light texture trắng 16x16.
    GLuint blockTex, lightTex;
    glGenTextures(1, &blockTex);
    glActiveTexture(0x84C0 + 2);
    glBindTexture(0x0DE1, blockTex);
    unsigned char white4[4 * 4 * 4];
    memset(white4, 255, sizeof(white4));
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, white4);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glGenTextures(1, &lightTex);
    glActiveTexture(0x84C0 + 0);
    glBindTexture(0x0DE1, lightTex);
    unsigned char white16[16 * 16 * 4];
    memset(white16, 255, sizeof(white16));
    glTexImage2D(0x0DE1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, white16);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);

    // Section-time buffer texture R32I 256 texel = 0.
    GLuint sectBuf, sectTex;
    glGenBuffers(1, &sectBuf);
    int32_t times[256];
    memset(times, 0, sizeof(times));
    glBindBuffer(0x8892, sectBuf);
    glBufferData(0x8892, sizeof(times), times, 0x88E4);
    glGenTextures(1, &sectTex);
    glActiveTexture(0x84C0 + 1);
    glBindTexture(0x8C2A, sectTex); // TEXTURE_BUFFER
    glTexBuffer(0x8C2A, 0x8235, sectBuf); // R32I

    // VAO compact interleaved 48B: pos uvec2 | color vec4f | tc uvec2 | light uvec4.
    struct Vert {
        uint32_t pos[2];
        float color[4];
        uint32_t tc[2];
        uint32_t light[4];
    };
    Vert tri[3];
    PackPos(-0.5f, -0.5f, 0.0f, tri[0].pos);
    PackPos(0.5f, -0.5f, 0.0f, tri[1].pos);
    PackPos(0.0f, 0.5f, 0.0f, tri[2].pos);
    for (int i = 0; i < 3; ++i) {
        tri[i].color[0] = tri[i].color[1] = tri[i].color[2] = tri[i].color[3] = 1.0f;
        tri[i].tc[0] = tri[i].tc[1] = 16384u;
        tri[i].light[0] = tri[i].light[1] = 128u;
        tri[i].light[2] = 0u;
        tri[i].light[3] = 0u;
    }
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(0, 2, 0x1405, 48, (void*)0);
    glVertexAttribPointer(1, 4, 0x1406, 0, 48, (void*)8);
    glVertexAttribIPointer(2, 2, 0x1405, 48, (void*)24);
    glVertexAttribIPointer(3, 4, 0x1405, 48, (void*)32);

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "u_LightTex"), 0);
    glUniform1i(glGetUniformLocation(prog, "u_SectionTimeInfo"), 1);
    glUniform1i(glGetUniformLocation(prog, "u_BlockTex"), 2);
    glUniform3f(glGetUniformLocation(prog, "u_RegionOffset"), 0, 0, 0);
    glUniform1i(glGetUniformLocation(prog, "u_CurrentTime"), 0);
    glUniform1ui(glGetUniformLocation(prog, "u_RegionID"), 0);
    glViewport(0, 0, 256, 256);
    glDisable(0x0BE2);
    glDisable(0x0B44);
    glEnable(0x0B71); // DEPTH như world thật
    glClearColor(0, 0, 0, 1);
    glClearDepthf(1.0f);
    glClear(0x00004000 | 0x00000100);
    while (glGetError() != 0) {}
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    glDrawArrays(0x0004, 0, 3);
    GLenum e = glGetError();
    printf("fulldraw err=0x%x encoded=%d\n", e, ctx.appleStats.drawsEncoded > encBefore);
    Check(e == 0, "fulldraw gl error");
    Check(ctx.appleStats.drawsEncoded > encBefore, "fulldraw not encoded");
    unsigned char px[256 * 256 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    unsigned char* mid = px + (128 * 256 + 128) * 4;
    printf("fulldraw mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[0] > 200 && mid[1] > 200 && mid[2] > 200, "fulldraw pixels den (shader/VAO/UBO?)");

    // Draw 2: section id != 0 (chunk xa, per-chunk translation thật).
    // draw_id=5 → rel=(0,1,1)*16=(0,16,16); regionOffset=(0,-16,-16) bù lại → tam giác về giữa.
    // Trắng = byte .w + uniform vec3/uint/int đúng với giá trị thật (không chỉ số 0).
    for (int i = 0; i < 3; ++i) tri[i].light[3] = 5u;
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glUniform3f(glGetUniformLocation(prog, "u_RegionOffset"), 0, -16, -16);
    glClear(0x00004000 | 0x00000100);
    while (glGetError() != 0) {}
    encBefore = ctx.appleStats.drawsEncoded;
    glDrawArrays(0x0004, 0, 3);
    e = glGetError();
    printf("fulldraw2 err=0x%x encoded=%d\n", e, ctx.appleStats.drawsEncoded > encBefore);
    Check(e == 0, "fulldraw2 gl error");
    Check(ctx.appleStats.drawsEncoded > encBefore, "fulldraw2 not encoded");
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    mid = px + (128 * 256 + 128) * 4;
    printf("fulldraw2 mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    Check(mid[0] > 200 && mid[1] > 200 && mid[2] > 200, "fulldraw2 den (draw_id/uniform sai?)");

    // Draw 3a/3b: xác thực uniform time/region/FOG có tác dụng thật.
    // Fog ĐỎ alpha 1, fog range xa (fog≈0 ở gốc). Trắng = fade đúng, ĐỎ = time/region kẹt.
    // 3a (RegionID): times[256]=-1000 → RegionID=1 đọc idx 256 → fadeFactor=1 → trắng.
    //   RegionID kẹt 0 → đọc idx 0 (time 0), inv=0 → fade 0 → ĐỎ.
    // 3b (CurrentTime): RegionID=0, times[0]=50, now=100, inv=1 → fade=1 → trắng.
    //   Time kẹt 0 → fade=clamp(-50)=0 → ĐỎ.
    {
        uint8_t gb2[192];
        memset(gb2, 0, sizeof(gb2));
        for (int i = 0; i < 16; ++i) ((float*)gb2)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        for (int i = 0; i < 16; ++i) ((float*)(gb2 + 64))[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        ((float*)(gb2 + 128))[0] = 1.0f; // fogColor ĐỎ alpha 1
        ((float*)(gb2 + 128))[3] = 1.0f;
        ((float*)(gb2 + 144))[0] = 100000.0f; // envFog xa
        ((float*)(gb2 + 144))[1] = 200000.0f;
        ((float*)(gb2 + 152))[0] = 100000.0f; // renderFog xa
        ((float*)(gb2 + 152))[1] = 200000.0f;
        ((float*)(gb2 + 160))[0] = 0.25f;
        ((float*)(gb2 + 160))[1] = 0.25f;
        glBindBuffer(0x8A11, ubo);
        glBufferData(0x8A11, sizeof(gb2), gb2, 0x88E4);
        // 3a
        int32_t times2[512];
        memset(times2, 0, sizeof(times2));
        times2[256] = -1000;
        glBindBuffer(0x8892, sectBuf);
        glBufferData(0x8892, sizeof(times2), times2, 0x88E4);
        glTexBuffer(0x8C2A, 0x8235, sectBuf); // MC rebind mỗi frame → refresh cache
        for (int i = 0; i < 3; ++i) tri[i].light[3] = 0u;
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glUniform3f(glGetUniformLocation(prog, "u_RegionOffset"), 0, 0, 0);
        glUniform1i(glGetUniformLocation(prog, "u_CurrentTime"), 100);
        glUniform1ui(glGetUniformLocation(prog, "u_RegionID"), 1);
        // fadeInv=0 qua UBO? gb2 offset 176 = 0 → inv 0.
        glClear(0x00004000 | 0x00000100);
        while (glGetError() != 0) {}
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "fulldraw3a gl error");
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
        mid = px + (128 * 256 + 128) * 4;
        printf("fulldraw3a mid=(%u,%u,%u) (trang=RegionID dung, do=ket)\n", mid[0], mid[1], mid[2]);
        Check(mid[0] > 200 && mid[1] > 200 && mid[2] > 200, "fulldraw3a DO (RegionID/time ket?)");
        // 3b
        times2[0] = 50;
        times2[256] = 0;
        glBindBuffer(0x8892, sectBuf);
        glBufferData(0x8892, sizeof(times2), times2, 0x88E4);
        glTexBuffer(0x8C2A, 0x8235, sectBuf); // refresh cache
        ((float*)gb2)[176 / 4] = 1.0f; // fadeInv=1
        glBindBuffer(0x8A11, ubo);
        glBufferData(0x8A11, sizeof(gb2), gb2, 0x88E4);
        glUniform1ui(glGetUniformLocation(prog, "u_RegionID"), 0);
        glClear(0x00004000 | 0x00000100);
        while (glGetError() != 0) {}
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "fulldraw3b gl error");
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
        mid = px + (128 * 256 + 128) * 4;
        printf("fulldraw3b mid=(%u,%u,%u) (trang=time dung, do=ket)\n", mid[0], mid[1], mid[2]);
        Check(mid[0] > 200 && mid[1] > 200 && mid[2] > 200, "fulldraw3b DO (CurrentTime ket?)");
    }

    glDeleteProgram(prog);
    if (gFails == 0) { printf("test_sodium_fulldraw_apple PASS\n"); return 0; }
    printf("test_sodium_fulldraw_apple: %d FAIL\n", gFails);
    return 1;
}
