// test_tess_quad_apple.cpp — GL 4.6 tessellation quad domain trên Metal.
// VS+TCS(vertices=4)+TES(quads)+FS; TCS compute pass + MTLQuadTessellationFactorsHalf.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "out vec2 vIn;\n"
    "void main() { vIn = vec2(0.5, 0.25); gl_Position = vec4(0.0); }\n";
static const char* kTCS =
    "#version 460 core\n"
    "layout(vertices = 5) out;\n"
    "in vec2 vIn[];\n"
    "out vec2 vPos[];\n"
    "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
    "patch out vec4 pColor;\n"
    "void main() {\n"
    "  vec2 c[5] = vec2[5](vec2(-1.0,-1.0), vec2(0.0,-1.0), vec2(0.0,1.0), vec2(-1.0,1.0), vec2(-0.5,0.0));\n"
    "  int last = gl_PatchVerticesIn - 1;\n"
    "  vec2 base = vIn[last];\n"
    "  vPos[gl_InvocationID] = c[gl_InvocationID] + vec2(float(gl_PrimitiveID), 0.0) + base * 0.0;\n"
    "  gl_out[gl_InvocationID].gl_Position = vec4(vPos[gl_InvocationID], 0.0, 1.0);\n"
    "  bool ok = (gl_PatchVerticesIn == 3 && base.x > 0.4);\n"
    "  pColor = !ok ? vec4(0.0,0.0,1.0,1.0)\n"
    "               : (gl_PrimitiveID == 0 ? vec4(1.0,0.0,0.0,1.0)\n"
    "                                      : vec4(0.0,1.0,0.0,1.0));\n"
    "  gl_TessLevelOuter[0] = 1.0; gl_TessLevelOuter[1] = 1.0;\n"
    "  gl_TessLevelOuter[2] = 1.0;\n"
    "  gl_TessLevelInner[0] = 1.0;\n"
    "}\n";
static const char* kTES =
    "#version 460 core\n"
    "layout(quads) in;\n"
    "in vec2 vPos[];\n"
    "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "patch in vec4 pColor;\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "  vec2 uv = gl_TessCoord.xy;\n"
    "  vec2 p0 = gl_in[0].gl_Position.xy;\n"
    "  vec2 p1 = gl_in[1].gl_Position.xy;\n"
    "  vec2 p2 = gl_in[2].gl_Position.xy;\n"
    "  vec2 p3 = gl_in[3].gl_Position.xy;\n"
    "  vec2 p = p0*(1.0-uv.x)*(1.0-uv.y) + p1*uv.x*(1.0-uv.y)\n"
    "         + p2*uv.x*uv.y + p3*(1.0-uv.x)*uv.y;\n"
    "  vec4 col = pColor;\n"
    "  if (gl_PatchVerticesIn != 5 || gl_in.length() != 5\n"
    "      || gl_TessLevelOuter[0] > 1.5 || gl_TessLevelInner[0] > 1.5\n"
    "      || gl_TessLevelOuter[3] < 1.5 || gl_TessLevelInner[1] < 1.5\n"
    "      || gl_PrimitiveID > 1)\n"
    "    col = vec4(0.0, 0.0, 1.0, 1.0);\n"
    "  vColor = col;\n"
    "  gl_Position = vec4(p, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec4 vColor;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vColor; }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src, const char* nm) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    tglmt::GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("FAIL %s compile: %s\n", nm, log);
        return 0;
    }
    return s;
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_quad_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    tglmt::GLuint vs = Compile(0x8B31, kVS, "vs");
    tglmt::GLuint tc = Compile(0x8E88, kTCS, "tcs");
    tglmt::GLuint te = Compile(0x8E87, kTES, "tes");
    tglmt::GLuint fs = Compile(0x8B30, kFS, "fs");
    if (!vs || !tc || !te || !fs) return 1;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL link: %s\n", log);
        return 1;
    }
    glUseProgram(p);
    // Input patch 3 CP, TCS output 5 CP: GL_PATCH_VERTICES != layout(vertices).
    tglmt::GLint maxPatch = 0;
    glGetIntegerv(0x8E7D /*GL_MAX_PATCH_VERTICES*/, &maxPatch);
    if (maxPatch < 32) { printf("FAIL max patch vertices=%d\n", (int)maxPatch); return 1; }
    glPatchParameteri(0x8E72 /*GL_PATCH_VERTICES*/, 0);
    if (glGetError() != 0x0501) { printf("FAIL patch vertices 0 không INVALID_VALUE\n"); return 1; }
    glPatchParameteri(0x8E72 /*GL_PATCH_VERTICES*/, 33);
    if (glGetError() != 0x0501) { printf("FAIL patch vertices 33 không INVALID_VALUE\n"); return 1; }
    tglmt::GLint maxTessLevel = 0;
    glGetIntegerv(0x8E7E /*GL_MAX_TESS_GEN_LEVEL*/, &maxTessLevel);
    if (maxTessLevel < 16) { printf("FAIL max tess level=%d\n", (int)maxTessLevel); return 1; }
    glPatchParameteri(0x8E72 /*GL_PATCH_VERTICES*/, 3);
    float qOuter[4] = {};
    float qInner[2] = {};
    glGetFloatv(0x8E74 /*GL_PATCH_DEFAULT_OUTER_LEVEL*/, qOuter);
    glGetFloatv(0x8E73 /*GL_PATCH_DEFAULT_INNER_LEVEL*/, qInner);
    if (qOuter[0] != 1.0f || qOuter[3] != 1.0f || qInner[0] != 1.0f || qInner[1] != 1.0f) {
        printf("FAIL default tess levels khác 1.0\n");
        return 1;
    }
    float dOuter[4] = {2.0f, 2.0f, 2.0f, 2.0f};
    float dInner[2] = {2.0f, 2.0f};
    glPatchParameterfv(0x8E74 /*GL_PATCH_DEFAULT_OUTER_LEVEL*/, dOuter);
    glPatchParameterfv(0x8E73 /*GL_PATCH_DEFAULT_INNER_LEVEL*/, dInner);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E /*GL_PATCHES*/, 0, 6);
    if (glGetError() != 0) { printf("FAIL gl error\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* left = px + (32 * 64 + 16) * 4;
    unsigned char* right = px + (32 * 64 + 48) * 4;
    printf("left=(%u,%u,%u,%u) right=(%u,%u,%u,%u)\n",
           left[0], left[1], left[2], left[3], right[0], right[1], right[2], right[3]);
    if (!(left[0] > 200 && left[1] < 50 && left[2] < 50)) {
        printf("FAIL left patch not red\n");
        return 1;
    }
    if (!(right[1] > 200 && right[0] < 50 && right[2] < 50)) {
        printf("FAIL right patch not green\n");
        return 1;
    }
    printf("test_tess_quad_apple PASS (quad domain, 2 patches, input CP 3 != output CP 5, runtime counts)\n");
    return 0;
}
