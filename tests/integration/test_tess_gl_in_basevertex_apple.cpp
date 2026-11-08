// test_tess_gl_in_basevertex_apple.cpp — TCS gl_in + glDrawElementsBaseVertex.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";
static const char* kTCS =
    "#version 460 core\n"
    "layout(vertices = 3) out;\n"
    "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
    "void main() {\n"
    "  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;\n"
    "  if (gl_InvocationID == 0) {\n"
    "    gl_TessLevelOuter[0] = 1.0; gl_TessLevelOuter[1] = 1.0;\n"
    "    gl_TessLevelOuter[2] = 1.0; gl_TessLevelInner[0] = 1.0;\n"
    "  }\n"
    "}\n";
static const char* kTES =
    "#version 460 core\n"
    "layout(triangles) in;\n"
    "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "  vColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "  vec2 p = gl_in[0].gl_Position.xy * gl_TessCoord.x +\n"
    "           gl_in[1].gl_Position.xy * gl_TessCoord.y +\n"
    "           gl_in[2].gl_Position.xy * gl_TessCoord.z;\n"
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
    glShaderSource(s, 1, &src, nullptr); glCompileShader(s);
    tglmt::GLint ok = 0; glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
               printf("FAIL %s compile: %s\n", nm, log); return 0; }
    return s;
}

int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_gl_in_basevertex_apple SKIP: no MTL device\n"); return 0; }
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
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log);
               printf("FAIL link: %s\n", log); return 1; }
    glUseProgram(p);
    const float verts[] = {
        10, 10, 10, 11, 11, 10,
        -1, -1, 3, -1, -1, 3
    };
    const unsigned short indices[3] = {0, 1, 2};
    tglmt::GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr); glEnableVertexAttribArray(0);
    glGenBuffers(1, &ebo); glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(indices), indices, 0x88E4);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f); glClear(0x00004000);
    glDrawElementsBaseVertex(0x000E /*PATCHES*/, 3, 0x1403, nullptr, 3);
    if (glGetError() != 0) { printf("FAIL gl error\n"); return 1; }
    glFinish();
    unsigned char px[64 * 64 * 4] = {};
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("FAIL center pixel not red\n"); return 1;
    }
    printf("test_tess_gl_in_basevertex_apple PASS\n");
    return 0;
}
