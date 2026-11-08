// test_tess_cull_apple.cpp — GL_CULL_FACE + layout(cw/ccw) trên tess.
// Hai chương trình chỉ khác winding qualifier; với cull bật, đúng một chương
// trình phải sống. Nếu encoder tess bỏ qua cull, cả hai cùng sống (fail).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS = "#version 460 core\nvoid main(){ gl_Position = vec4(0.0); }\n";
static const char* kFS =
    "#version 460 core\nin vec4 vColor;\nlayout(location=0) out vec4 c;\nvoid main(){ c = vColor; }\n";

static tglmt::GLuint Compile(tglmt::GLenum t, const char* s) {
    using namespace tglmt::gl;
    tglmt::GLuint h = glCreateShader(t);
    glShaderSource(h, 1, &s, nullptr);
    glCompileShader(h);
    tglmt::GLint ok = 0;
    glGetShaderiv(h, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(h, sizeof(log), nullptr, log); printf("compile: %s\n", log); }
    return ok ? h : 0;
}

static tglmt::GLuint Build(const char* order) {
    using namespace tglmt::gl;
    std::string tcs =
        "#version 460 core\nlayout(vertices=3) out;\n"
        "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
        "void main(){\n"
        "  vec2 c[3]=vec2[3](vec2(-0.9,-0.9),vec2(0.9,-0.9),vec2(0.0,0.9));\n"
        "  gl_out[gl_InvocationID].gl_Position=vec4(c[gl_InvocationID],0.0,1.0);\n"
        "  gl_TessLevelOuter[0]=1.0; gl_TessLevelOuter[1]=1.0; gl_TessLevelOuter[2]=1.0;\n"
        "  gl_TessLevelInner[0]=1.0;\n}\n";
    std::string tes =
        "#version 460 core\nlayout(triangles, equal_spacing, " + std::string(order) + ") in;\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "out vec4 vColor;\n"
        "void main(){\n"
        "  vec3 uv=gl_TessCoord;\n"
        "  vec2 p=gl_in[0].gl_Position.xy*uv.x+gl_in[1].gl_Position.xy*uv.y+gl_in[2].gl_Position.xy*uv.z;\n"
        "  vColor=vec4(1.0,0.0,0.0,1.0); gl_Position=vec4(p,0.0,1.0);\n}\n";
    auto vs = Compile(0x8B31, kVS);
    auto tc = Compile(0x8E88, tcs.c_str());
    auto te = Compile(0x8E87, tes.c_str());
    auto fs = Compile(0x8B30, kFS);
    if (!vs || !tc || !te || !fs) return 0;
    auto p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log); printf("link: %s\n", log); return 0; }
    return p;
}

static bool Rendered(tglmt::GLuint p, unsigned char* mid) {
    using namespace tglmt::gl;
    glUseProgram(p);
    glPatchParameteri(0x8E72, 3);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E, 0, 3);
    if (glGetError() != 0) return false;
    unsigned char px[4] = {};
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, px);
    memcpy(mid, px, 4);
    return px[0] > 200 && px[1] < 50 && px[2] < 50;
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_cull_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);
    tglmt::GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glEnable(0x0B44 /*CULL_FACE*/);
    glCullFace(0x0405 /*BACK*/);
    glFrontFace(0x0901 /*CCW*/);
    glDisable(0x0B71); glDisable(0x0BE2); glDisable(0x0B90);
    tglmt::GLuint pccw = Build("ccw"), pcw = Build("cw");
    if (!pccw || !pcw) return 1;
    unsigned char a[4] = {}, b[4] = {};
    bool accw = Rendered(pccw, a);
    bool acw = Rendered(pcw, b);
    printf("ccw=%d(%u,%u,%u) cw=%d(%u,%u,%u)\n", accw, a[0], a[1], a[2], acw, b[0], b[1], b[2]);
    if (accw == acw) {
        printf("FAIL cull/winding tess: cả hai cùng %s\n", accw ? "sống" : "bị cull");
        return 1;
    }
    printf("test_tess_cull_apple PASS (cull + cw/ccw đúng)\n");
    return 0;
}
