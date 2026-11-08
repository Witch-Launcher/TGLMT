// test_tess_mrt_msaa_depth_apple.cpp — Tess pipeline đồng thời:
// 2 color attachment (MRT) + 4x MSAA + depth test/depth attachment.
// Gần đỏ phải thắng xa xanh; att0 đỏ, att1 xanh. Nếu depth bị bỏ qua,
// xa xanh ghi đè att0 → fail. Nếu MRT/MSAA pipeline sai, link/pipeline/FBO fail.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <string>

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("FAIL: %s\n", msg); ++gFails; }
}
static tglmt::GLuint Compile(tglmt::GLenum t, const char* s) {
    using namespace tglmt::gl;
    tglmt::GLuint h = glCreateShader(t);
    glShaderSource(h, 1, &s, nullptr);
    glCompileShader(h);
    tglmt::GLint ok = 0; glGetShaderiv(h, 0x8B81, &ok);
    if (!ok) { char log[2048] = {}; glGetShaderInfoLog(h, sizeof(log), nullptr, log); printf("compile: %s\n", log); return 0; }
    return h;
}
static tglmt::GLuint MakeProgram(float z) {
    using namespace tglmt::gl;
    const char* vs = "#version 460 core\nvoid main(){ gl_Position=vec4(0.0); }\n";
    std::string tcs =
        "#version 460 core\nlayout(vertices=3) out;\n"
        "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
        "void main(){\n"
        "  vec2 c[3]=vec2[3](vec2(-0.9,-0.9),vec2(0.9,-0.9),vec2(0.0,0.9));\n"
        "  gl_out[gl_InvocationID].gl_Position=vec4(c[gl_InvocationID],0.25,1.0);\n"
        "  gl_TessLevelOuter[0]=1.0; gl_TessLevelOuter[1]=1.0; gl_TessLevelOuter[2]=1.0;\n"
        "  gl_TessLevelInner[0]=1.0;\n}\n";
    std::string tes =
        "#version 460 core\nlayout(triangles, equal_spacing, ccw) in;\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "void main(){\n"
        "  vec3 uv=gl_TessCoord;\n"
        "  vec2 p=gl_in[0].gl_Position.xy*uv.x+gl_in[1].gl_Position.xy*uv.y+gl_in[2].gl_Position.xy*uv.z;\n"
        "  gl_Position=vec4(p, " + std::to_string(z) + ", 1.0);\n}\n";
    const char* fs =
        "#version 460 core\n"
        "layout(location=0) out vec4 o0;\nlayout(location=1) out vec4 o1;\n"
        "void main(){ o0=vec4(1,0,0,1); o1=vec4(0,1,0,1); }\n";
    auto v = Compile(0x8B31, vs), tc = Compile(0x8E88, tcs.c_str());
    auto te = Compile(0x8E87, tes.c_str()), f = Compile(0x8B30, fs);
    if (!v || !tc || !te || !f) return 0;
    auto p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, f);
    glLinkProgram(p);
    tglmt::GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { char log[4096] = {}; glGetProgramInfoLog(p, sizeof(log), nullptr, log); printf("link: %s\n", log); return 0; }
    return p;
}
static void Read(tglmt::GLenum slot, unsigned char out[4]) {
    using namespace tglmt::gl;
    glReadBuffer(slot);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, out);
}
int main() {
    tglmt::Context ctx("apple"); tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_mrt_msaa_depth_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    while (glGetError() != 0) {}
    tglmt::GLuint nearP = MakeProgram(0.25f), farP = MakeProgram(0.75f);
    if (!nearP || !farP) return 1;
    tglmt::GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);

    tglmt::GLuint c0 = 0, c1 = 0, depth = 0, fbo = 0;
    glGenTextures(1, &c0); glBindTexture(0x9100, c0);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 64, 64, 0x1401);
    glGenTextures(1, &c1); glBindTexture(0x9100, c1);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 64, 64, 0x1401);
    glGenTextures(1, &depth); glBindTexture(0x0DE1, depth);
    glTexImage2D(0x0DE1, 0, 0x81A6 /*DEPTH_COMPONENT24*/, 64, 64, 0, 0x1902, 0x1406, nullptr);
    glGenFramebuffers(1, &fbo); glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x9100, c0, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x9100, c1, 0);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depth, 0);
    glDrawBuffers(2, (const tglmt::GLenum[]){0x8CE0, 0x8CE1});
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO MRT+MSAA+depth complete");
    Check(glGetError() == 0, "FBO setup no error");

    glViewport(0, 0, 64, 64);
    glEnable(0x0B71 /*DEPTH_TEST*/); glDepthFunc(0x0201 /*LESS*/);
    glDisable(0x0BE2); glDisable(0x0B44);
    glClearColor(0, 0, 1, 1); glClearDepth(1.0); glClear(0x4000 | 0x100);
    glUseProgram(nearP); glDrawArrays(0x000E, 0, 3);
    glUseProgram(farP); glDrawArrays(0x000E, 0, 3);
    Check(glGetError() == 0, "MRT/MSAA/depth tess draws no error");
    ctx.FlushPendingEncoder(); ctx.CommitAndWait();

    unsigned char a0[4] = {}, a1[4] = {};
    Read(0x8CE0, a0); Read(0x8CE1, a1);
    printf("att0=(%u,%u,%u) att1=(%u,%u,%u)\n", a0[0],a0[1],a0[2],a1[0],a1[1],a1[2]);
    Check(a0[0] > 200 && a0[1] < 50, "depth: gần đỏ thắng xa xanh ở att0");
    Check(a1[0] < 50 && a1[1] > 200, "MRT: att1 xanh");
    if (gFails == 0) { printf("test_tess_mrt_msaa_depth_apple PASS\n"); return 0; }
    return 1;
}
