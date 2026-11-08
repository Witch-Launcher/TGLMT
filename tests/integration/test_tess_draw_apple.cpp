// test_tess_draw_apple.cpp — GL 4.6 tessellation E2E trên Metal (P-Tess T4a).
// VS+TCS+TES+FS qua API GL; TCS = compute pass, TES = post-tess vertex,
// glDrawArrays(GL_PATCHES). Subset T4a: TCS không đọc input, không uniform.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "void main() { gl_Position = vec4(0.0); }\n";
static const char* kTCS =
    "#version 460 core\n"
    "layout(vertices = 3) out;\n"
    "out vec2 vPos[];\n"
    "patch out vec4 pColor;\n"
    "void main() {\n"
    "  vec2 c[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));\n"
    "  vPos[gl_InvocationID] = c[gl_InvocationID];\n"
    "  pColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "  gl_TessLevelOuter[0] = 1.0; gl_TessLevelOuter[1] = 1.0; gl_TessLevelOuter[2] = 1.0;\n"
    "  gl_TessLevelInner[0] = 1.0;\n"
    "}\n";
static const char* kTES =
    "#version 460 core\n"
    "layout(triangles) in;\n"
    "in vec2 vPos[];\n"
    "patch in vec4 pColor;\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "  vec2 p = vPos[0] * gl_TessCoord.x + vPos[1] * gl_TessCoord.y + vPos[2] * gl_TessCoord.z;\n"
    "  vColor = pColor;\n"
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
    if (ctx.device->isNull()) { printf("test_tess_draw_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

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
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E /*GL_PATCHES*/, 0, 3);
    if (glGetError() != 0) { printf("FAIL gl error\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("FAIL center pixel not red\n");
        return 1;
    }
    printf("test_tess_draw_apple PASS (GL tessellation via Metal drawPatches)\n");
    return 0;
}
