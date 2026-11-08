// test_tess_spacing_apple.cpp — partition mode phải thay đổi tessellation thật.
// Cùng factor 3.5, equal_spacing vs fractional_odd phải cho ảnh khác nhau;
// nếu pipeline luôn dùng Integer thì ảnh giống hệt (test fail).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "void main() { gl_Position = vec4(0.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec4 vColor;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vColor; }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    tglmt::GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("FAIL compile: %s\n", log);
    }
    return ok ? s : 0;
}

static bool Build(const char* spacing, tglmt::GLuint* out) {
    using namespace tglmt::gl;
    std::string tcsSrc =
        "#version 460 core\n"
        "layout(vertices = 3) out;\n"
        "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
        "void main() {\n"
        "  vec2 c[3] = vec2[3](vec2(-0.9,-0.9), vec2(0.9,-0.9), vec2(0.0,0.9));\n"
        "  gl_out[gl_InvocationID].gl_Position = vec4(c[gl_InvocationID], 0.0, 1.0);\n"
        "  gl_TessLevelOuter[0] = 3.5; gl_TessLevelOuter[1] = 3.5; gl_TessLevelOuter[2] = 3.5;\n"
        "  gl_TessLevelInner[0] = 3.5;\n"
        "}\n";
    std::string tesSrc =
        "#version 460 core\n"
        "layout(triangles, " + std::string(spacing) + ", ccw) in;\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "  vec3 uv = gl_TessCoord;\n"
        "  vec2 p = gl_in[0].gl_Position.xy * uv.x + gl_in[1].gl_Position.xy * uv.y\n"
        "         + gl_in[2].gl_Position.xy * uv.z;\n"
        "  vColor = vec4(uv.x, uv.y, uv.z, 1.0);\n"
        "  gl_Position = vec4(p, 0.0, 1.0);\n"
        "}\n";
    tglmt::GLuint vs = Compile(0x8B31, kVS);
    tglmt::GLuint tc = Compile(0x8E88, tcsSrc.c_str());
    tglmt::GLuint te = Compile(0x8E87, tesSrc.c_str());
    tglmt::GLuint fs = Compile(0x8B30, kFS);
    if (!vs || !tc || !te || !fs) return false;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL link %s: %s\n", spacing, log);
        return false;
    }
    *out = p;
    return true;
}

static bool Render(tglmt::GLuint p, unsigned char* px) {
    using namespace tglmt::gl;
    glUseProgram(p);
    glPatchParameteri(0x8E72, 3);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E, 0, 3);
    if (glGetError() != 0) return false;
    memset(px, 0, 64 * 64 * 4);
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    return true;
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_spacing_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    tglmt::GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    tglmt::GLuint pe = 0, pf = 0;
    if (!Build("equal_spacing", &pe) || !Build("fractional_odd", &pf)) return 1;
    unsigned char a[64 * 64 * 4], b[64 * 64 * 4];
    if (!Render(pe, a) || !Render(pf, b)) { printf("FAIL render\n"); return 1; }
    int diff = 0;
    for (int i = 0; i < 64 * 64 * 4; ++i) if (a[i] != b[i]) ++diff;
    int diffPixels = 0;
    for (int i = 0; i < 64 * 64; ++i)
        if (memcmp(a + i * 4, b + i * 4, 4) != 0) ++diffPixels;
    printf("diffBytes=%d diffPixels=%d\n", diff, diffPixels);
    if (diffPixels == 0) {
        printf("FAIL spacing bị bỏ qua: equal và fractional_odd giống hệt\n");
        return 1;
    }
    printf("test_tess_spacing_apple PASS (partition mode thay đổi tessellation)\n");
    return 0;
}
