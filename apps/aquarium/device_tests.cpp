#include "device_tests.h"
#include "tglmt/Context.h"
#include "tglmt/gl46.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

struct Suite {
    Context& ctx;
    const std::function<void(const char*)>& log;
    int pass = 0, fail = 0, skip = 0;

    void Say(const std::string& s) { log(s.c_str()); }
    void Check(bool ok, const char* name, const std::string& detail = {}) {
        if (ok) { ++pass; Say(std::string("PASS ") + name + (detail.empty() ? "" : "  [" + detail + "]")); }
        else { ++fail; Say(std::string("FAIL ") + name + (detail.empty() ? "" : "  [" + detail + "]")); }
    }
    void Skip(const char* name, const std::string& why) {
        ++skip; Say(std::string("SKIP ") + name + "  [" + why + "]");
    }
};

std::string ErrName(GLenum e) {
    char b[32];
    std::snprintf(b, sizeof(b), "0x%04x", e);
    return b;
}

void ClearErrors() { while (glGetError() != 0) {} }

void Finish(Context& ctx) {
    ctx.FlushPendingEncoder();
    ctx.CommitAndWait();
}

GLuint Shader(Suite& t, GLenum type, const char* src, const char* name) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char info[1024] = {0};
        glGetShaderInfoLog(s, sizeof(info) - 1, nullptr, info);
        t.Check(false, name, info);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

GLuint Program(Suite& t, const char* vs, const char* fs, const char* name) {
    GLuint v = Shader(t, 0x8B31, vs, name);
    GLuint f = Shader(t, 0x8B30, fs, name);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char info[1024] = {0};
        glGetProgramInfoLog(p, sizeof(info) - 1, nullptr, info);
        t.Check(false, name, info);
        glDeleteProgram(p);
        p = 0;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

GLuint QuadVAO() {
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    float verts[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    return vao;
}

GLuint ColorTex(int w, int h, GLenum fmt = 0x8058) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, fmt, w, h, 0, 0x1908, 0x1401, nullptr);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    return tex;
}

GLuint TargetTex(int w, int h, GLuint* fbo) {
    GLuint tex = ColorTex(w, h);
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(0x8D40, *fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    return tex;
}

bool ReadRGBA(Context& ctx, int x, int y, unsigned char* px) {
    Finish(ctx);
    std::memset(px, 0, 4);
    glReadPixels(x, y, 1, 1, 0x1908, 0x1401, px);
    return glGetError() == 0;
}

const char* kQuadVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 a_pos;\n"
    "out vec2 v_uv;\n"
    "void main(){ v_uv = a_pos * 0.5 + 0.5; gl_Position = vec4(a_pos,0,1); }\n";

const char* kSample2DFS =
    "#version 460 core\n"
    "in vec2 v_uv; out vec4 o; uniform sampler2D uTex; uniform float uLod;\n"
    "void main(){ o = textureLod(uTex, v_uv, uLod); }\n";

const char* kArrayFS =
    "#version 460 core\n"
    "in vec2 v_uv; out vec4 o;\n"
    "uniform sampler2DArray uArr; uniform sampler2DArray uDepth;\n"
    "uniform float uLayer; uniform float uUseDepth;\n"
    "void main(){\n"
    "  vec4 a = texture(uArr, vec3(v_uv, uLayer));\n"
    "  ivec3 s = textureSize(uArr, 0);\n"
    "  float v = (uUseDepth > 0.5) ? texture(uDepth, vec3(v_uv, uLayer)).r : a.r;\n"
    "  o = vec4(v, float(s.z)/8.0, a.g, 1.0);\n"
    "}\n";

const char* k3DFS =
    "#version 460 core\n"
    "in vec2 v_uv; out vec4 o; uniform sampler3D uTex; uniform float uZ;\n"
    "void main(){ o = texture(uTex, vec3(v_uv, uZ)); }\n";

void TestInfo(Suite& t) {
    const char* ver = (const char*)glGetString(0x1F02);
    const char* ren = (const char*)glGetString(0x1F01);
    GLint next = 0;
    glGetIntegerv(0x821D, &next);
    t.Check(ver && std::strstr(ver, "4.6"), "GL_VERSION 4.6", ver ? ver : "(null)");
    t.Say(std::string("INFO renderer=") + (ren ? ren : "(null)") +
          " extensions=" + std::to_string(next));
}

void TestFboBasic(Suite& t) {
    ClearErrors();
    GLuint fbo = 0;
    GLuint tex = TargetTex(16, 16, &fbo);
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO complete");
    glViewport(0, 0, 16, 16);
    glDisable(0x0B71); glDisable(0x0C11); glDisable(0x0BE2);
    glClearColor(1, 0, 0, 1);
    glClear(0x4000);
    unsigned char p[4];
    bool ok = ReadRGBA(t.ctx, 8, 8, p);
    t.Check(ok && p[0] > 200 && p[1] < 50, "FBO clear/read red",
            "rgba=" + std::to_string(p[0]) + "," + std::to_string(p[1]) + "," +
            std::to_string(p[2]) + "," + std::to_string(p[3]));
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
}

void TestArray(Suite& t) {
    ClearErrors();
    GLuint arr = 0;
    glGenTextures(1, &arr);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arr);
    std::vector<uint8_t> px(4 * 4 * 3 * 4, 0);
    for (int z = 0; z < 3; ++z)
        for (int i = 0; i < 16; ++i) px[(z * 16 + i) * 4] = (uint8_t)(z * 80);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 3, 0, 0x1908, 0x1401, px.data());
    glTexParameteri(0x8C1A, 0x2800, 0x2600);
    glTexParameteri(0x8C1A, 0x2801, 0x2600);
    t.Check(glGetError() == 0, "glTexImage3D 2D_ARRAY");

    GLuint fbo = 0;
    GLuint color = TargetTex(32, 32, &fbo);
    GLuint prog = Program(t, kQuadVS, kArrayFS, "array shader");
    if (!prog) return;
    GLuint vao = QuadVAO();
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uArr"), 1);
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 2.0f);
    glUniform1f(glGetUniformLocation(prog, "uUseDepth"), 0.0f);
    glViewport(0, 0, 32, 32);
    glDrawArrays(0x0004, 0, 6);
    unsigned char p[4];
    ReadRGBA(t.ctx, 16, 16, p);
    t.Check(p[0] > 150 && p[1] > 80 && p[1] < 112,
            "sampler2DArray layer 2 + textureSize.z",
            "rgba=" + std::to_string(p[0]) + "," + std::to_string(p[1]));

    GLuint depth = 0;
    glGenTextures(1, &depth);
    glActiveTexture(0x84C2);
    glBindTexture(0x8C1A, depth);
    glTexImage3D(0x8C1A, 0, 0x8CAC, 8, 8, 2, 0, 0x1902, 0x1406, nullptr);
    t.Check(glGetError() == 0, "depth array storage");

    GLuint arrRT = 0;
    glGenTextures(1, &arrRT);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arrRT);
    glTexImage3D(0x8C1A, 0, 0x8058, 8, 8, 2, 0, 0x1908, 0x1401, nullptr);
    GLuint fboD = 0;
    glGenFramebuffers(1, &fboD);
    glBindFramebuffer(0x8D40, fboD);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arrRT, 0, 0);
    glFramebufferTextureLayer(0x8D40, 0x8D00, depth, 0, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "depth array FBO complete");
    glViewport(0, 0, 8, 8);
    glClearDepth(0.25);
    glClear(0x0100);
    t.Check(glGetError() == 0, "clear depth array layer");

    glBindFramebuffer(0x8D40, fbo);
    glViewport(0, 0, 32, 32);
    glActiveTexture(0x84C1); glBindTexture(0x8C1A, arrRT);
    glActiveTexture(0x84C2); glBindTexture(0x8C1A, depth);
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uArr"), 1);
    glUniform1i(glGetUniformLocation(prog, "uDepth"), 2);
    glUniform1f(glGetUniformLocation(prog, "uUseDepth"), 1.0f);
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    ReadRGBA(t.ctx, 16, 16, p);
    t.Check(p[0] > 50 && p[0] < 80, "depth array layer clears to 0.25",
            "r=" + std::to_string(p[0]));

    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &fboD);
    glDeleteTextures(1, &color);
    glDeleteTextures(1, &arr);
    glDeleteTextures(1, &arrRT);
    glDeleteTextures(1, &depth);
}

void Test3D(Suite& t) {
    ClearErrors();
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x806F, tex);
    std::vector<uint8_t> px(4 * 4 * 2 * 4, 0);
    for (int z = 0; z < 2; ++z)
        for (int i = 0; i < 16; ++i) px[(z * 16 + i) * 4] = (uint8_t)(z ? 200 : 40);
    glTexImage3D(0x806F, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, px.data());
    t.Check(glGetError() == 0, "3D texture storage");
    GLuint fbo = 0;
    GLuint color = TargetTex(16, 16, &fbo);
    GLuint prog = Program(t, kQuadVS, k3DFS, "3D shader");
    if (!prog) return;
    GLuint vao = QuadVAO();
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.75f);
    glActiveTexture(0x84C0);
    glBindTexture(0x806F, tex);
    glViewport(0, 0, 16, 16);
    glDrawArrays(0x0004, 0, 6);
    unsigned char p[4];
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[0] > 150, "sampler3D slice 1", "r=" + std::to_string(p[0]));
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    glDeleteTextures(1, &tex);
}

void TestMipCopy(Suite& t) {
    ClearErrors();
    GLuint srcFbo = 0;
    GLuint src = TargetTex(8, 8, &srcFbo);
    glViewport(0, 0, 8, 8);
    glClearColor(1, 0, 0, 1); glClear(0x4000);

    GLuint dst = 0;
    glGenTextures(1, &dst);
    glBindTexture(0x0DE1, dst);
    glTexStorage2D(0x0DE1, 4, 0x8058, 8, 8);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, dst);
    glCopyTexSubImage2D(0x0DE1, 1, 0, 0, 0, 0, 4, 4);
    t.Check(glGetError() == 0, "glCopyTexSubImage2D mip level");

    GLuint fbo = 0;
    GLuint color = TargetTex(16, 16, &fbo);
    GLuint prog = Program(t, kQuadVS, kSample2DFS, "mip sample shader");
    if (!prog) return;
    GLuint vao = QuadVAO();
    glUseProgram(prog);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, dst);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glUniform1f(glGetUniformLocation(prog, "uLod"), 1.0f);
    glViewport(0, 0, 16, 16);
    glDrawArrays(0x0004, 0, 6);
    unsigned char p[4];
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[0] > 200 && p[1] < 50, "sample copied mip level 1",
            "rgba=" + std::to_string(p[0]) + "," + std::to_string(p[1]));
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteFramebuffers(1, &srcFbo);
    glDeleteTextures(1, &color);
    glDeleteTextures(1, &src);
    glDeleteTextures(1, &dst);
}

void TestMsaa(Suite& t) {
    ClearErrors();
    GLuint ms = 0, msDepth = 0, msFbo = 0;
    glGenTextures(1, &ms);
    glBindTexture(0x9100, ms);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 32, 32, 1);
    glGenTextures(1, &msDepth);
    glBindTexture(0x0DE1, msDepth);
    glTexImage2D(0x0DE1, 0, 0x81A5, 32, 32, 0, 0x1902, 0x1406, nullptr);
    glGenFramebuffers(1, &msFbo);
    glBindFramebuffer(0x8D40, msFbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x9100, ms, 0);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, msDepth, 0);
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "MSAA FBO complete");
    glViewport(0, 0, 32, 32);
    glClearColor(0, 1, 0, 1);
    glClear(0x4000);
    unsigned char pm[4];
    ReadRGBA(t.ctx, 16, 16, pm);

    GLuint resolve = 0, fbo = 0;
    resolve = TargetTex(32, 32, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glBindFramebuffer(0x8CA8, msFbo);
    glBindFramebuffer(0x8CA9, fbo);
    glBlitFramebuffer(0, 0, 32, 32, 0, 0, 32, 32, 0x4000, 0x2600);
    t.Check(glGetError() == 0, "MSAA resolve blit");
    glBindFramebuffer(0x8D40, fbo);
    unsigned char p[4];
    ReadRGBA(t.ctx, 16, 16, p);
    t.Check(p[1] > 200 && p[0] < 50, "MSAA resolved green",
            "rgba=" + std::to_string(p[0]) + "," + std::to_string(p[1]) +
            " raw=" + std::to_string(pm[1]));
    glDeleteFramebuffers(1, &msFbo);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &ms);
    glDeleteTextures(1, &msDepth);
    glDeleteTextures(1, &resolve);
}

void TestStencil(Suite& t) {
    ClearErrors();
    GLuint color = ColorTex(16, 16);
    GLuint ds = 0;
    glGenTextures(1, &ds);
    glBindTexture(0x0DE1, ds);
    glTexImage2D(0x0DE1, 0, 0x88F0, 16, 16, 0, 0x84F9, 0x84FA, nullptr);
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
    glFramebufferTexture2D(0x8D40, 0x84F9, 0x0DE1, ds, 0);
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "stencil FBO complete");
    const char* fsRed = "#version 460 core\nout vec4 o;void main(){o=vec4(1,0,0,1);}\n";
    const char* fsGreen = "#version 460 core\nout vec4 o;void main(){o=vec4(0,1,0,1);}\n";
    GLuint red = Program(t, kQuadVS, fsRed, "stencil red");
    GLuint green = Program(t, kQuadVS, fsGreen, "stencil green");
    GLuint vao = QuadVAO();
    glViewport(0, 0, 16, 16);
    glDisable(0x0B71); glEnable(0x0B90);
    glClearColor(0, 0, 1, 1); glClearStencil(0);
    glClear(0x4000 | 0x0400);
    glUseProgram(red);
    glStencilMask(0xFF);
    glStencilFunc(0x0207, 1, 0xFF);
    glStencilOp(0x1E00, 0x1E00, 0x1E01);
    glDrawArrays(0x0004, 0, 6);
    unsigned char p[4];
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[0] > 200 && p[1] < 50, "stencil write red");
    glUseProgram(green);
    glStencilFunc(0x0202, 0, 0xFF);
    glStencilOp(0x1E00, 0x1E00, 0x1E00);
    glDrawArrays(0x0004, 0, 6);
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[0] > 200 && p[1] < 50, "stencil EQUAL 0 rejects green");
    glStencilFunc(0x0202, 1, 0xFF);
    glDrawArrays(0x0004, 0, 6);
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[1] > 200 && p[0] < 50, "stencil EQUAL 1 passes green");
    glDisable(0x0B90);
    glDeleteProgram(red); glDeleteProgram(green);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    glDeleteTextures(1, &ds);
}

void TestCompressedContract(Suite& t) {
    ClearErrors();
    GLint n = -1;
    glGetIntegerv(0x86A2, &n);
    unsigned char data[64] = {};
    glCompressedTexImage2D(0x0DE1, 0, 0x83F0, 4, 4, 0, 32, data);
    GLenum e = glGetError();
    if (n == 0)
        t.Check(e == 0x0500, "compressed unsupported rejects DXT1", "formats=0 err=" + ErrName(e));
    else
        t.Check(e == 0, "compressed DXT1 upload", "formats=" + std::to_string(n) + " err=" + ErrName(e));
    ClearErrors();
}

void TestPipelineContract(Suite& t) {
    ClearErrors();
    const char* gs =
        "#version 460 core\n"
        "layout(triangles) in; layout(triangle_strip, max_vertices=3) out;\n"
        "void main(){ for(int i=0;i<3;i++){ gl_Position=gl_in[i].gl_Position; EmitVertex(); } EndPrimitive(); }\n";
    const char* vs =
        "#version 460 core\nlayout(location=0) in vec2 p;void main(){gl_Position=vec4(p,0,1);}\n";
    const char* fs = "#version 460 core\nout vec4 o;void main(){o=vec4(1);}\n";
    GLuint g = glCreateShader(0x8DD9);
    glShaderSource(g, 1, &gs, nullptr);
    glCompileShader(g);
    GLint gok = 0;
    glGetShaderiv(g, 0x8B81, &gok);
    char ginfo[512] = {0};
    glGetShaderInfoLog(g, sizeof(ginfo) - 1, nullptr, ginfo);
    if (!gok) {
        t.Check(true, "geometry shader rejected honestly", std::string(ginfo).substr(0, 120));
        glDeleteShader(g);
        ClearErrors();
        return;
    }
    GLuint v = Shader(t, 0x8B31, vs, "GS contract VS");
    GLuint f = Shader(t, 0x8B30, fs, "GS contract FS");
    GLuint p = glCreateProgram();
    if (v) glAttachShader(p, v);
    if (f) glAttachShader(p, f);
    glAttachShader(p, g);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    t.Check(ok == 0, "geometry shader link rejected", ok ? "linked unexpectedly" : "link failed");
    glDeleteProgram(p);
    if (v) glDeleteShader(v);
    if (f) glDeleteShader(f);
    glDeleteShader(g);
    ClearErrors();
}

void TestClipDistance(Suite& t) {
    ClearErrors();
    GLuint fbo = 0, color = TargetTex(64, 64, &fbo);
    const char* vs =
        "#version 460 core\n"
        "layout(location=0) in vec2 a_pos;\n"
        "void main(){ gl_Position=vec4(a_pos,0,1); gl_ClipDistance[0]=a_pos.x; }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 o; void main(){ o=vec4(1,0,0,1); }\n";
    GLuint v = Shader(t, 0x8B31, vs, "clip VS compile");
    GLuint f = Shader(t, 0x8B30, fs, "clip FS compile");
    GLuint p = 0;
    if (v && f) {
        p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f);
        glLinkProgram(p);
        GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
        t.Check(ok != 0, "clip distance link", ok ? "" : "link failed");
        if (!ok) { glDeleteProgram(p); p = 0; }
    } else {
        t.Check(false, "clip distance shaders", "compile failed");
    }
    if (p) {
        GLuint vao = QuadVAO();
        glUseProgram(p);
        glViewport(0, 0, 64, 64);
        glDisable(0x0B71); glDisable(0x0C11); glDisable(0x0BE2);
        glEnable(0x3000 /*GL_CLIP_DISTANCE0*/);
        glClearColor(0, 0, 1, 1); glClear(0x4000);
        glDrawArrays(0x0004, 0, 6);
        unsigned char l[4] = {}, r[4] = {};
        bool lok = ReadRGBA(t.ctx, 8, 32, l);
        bool rok = ReadRGBA(t.ctx, 56, 32, r);
        t.Check(lok && l[0] < 50 && l[2] > 200, "clip distance left clipped",
                "rgba=" + std::to_string(l[0]) + "," + std::to_string(l[2]));
        t.Check(rok && r[0] > 200 && r[2] < 50, "clip distance right kept",
                "rgba=" + std::to_string(r[0]) + "," + std::to_string(r[2]));
        glDisable(0x3000);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(p);
    }
    if (v) glDeleteShader(v);
    if (f) glDeleteShader(f);
    glBindFramebuffer(0x8D40, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
    ClearErrors();
}

void TestTransformFeedback(Suite& t) {
    ClearErrors();
    GLuint x = 0;
    glGenTransformFeedbacks(1, &x);
    glBindTransformFeedback(0x8E22, x);
    glBeginTransformFeedback(0x0004);
    GLint active = 0;
    glGetTransformFeedbackiv(x, 0x8E24, &active);
    glEndTransformFeedback();
    t.Check(active == 1, "transform feedback state machine", "active=" + std::to_string(active));

    // VS-only capture thật: compute prepass + pack vào GL XFB buffer.
    GLuint fbo = 0, color = TargetTex(16, 16, &fbo);
    GLuint v = Shader(t, 0x8B31, kQuadVS, "xfb vs");
    GLuint f = Shader(t, 0x8B30, kSample2DFS, "xfb fs");
    GLuint p = 0;
    if (v && f) {
        p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        const char* vars[2] = {"v_uv", "gl_Position"};
        glTransformFeedbackVaryings(p, 2, vars, 0x8C8C);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, 0x8B82, &ok);
        t.Check(ok != 0, "transform feedback capture link", ok ? "" : "link failed");
        if (!ok) { glDeleteProgram(p); p = 0; }
    } else {
        t.Check(false, "transform feedback capture shaders", "compile failed");
    }
    if (p) {
        const int stride = 24; // vec2 v_uv (8) + vec4 gl_Position (16)
        GLuint xb = 0;
        glGenBuffers(1, &xb);
        glBindBuffer(0x8892, xb);
        glBufferData(0x8892, 3 * stride, nullptr, 0x88E4);
        glBindTransformFeedback(0x8E22, x);
        glBindBufferBase(0x8C8E, 0, xb);
        GLuint vao = QuadVAO();
        glUseProgram(p);
        glViewport(0, 0, 16, 16);
        glBeginTransformFeedback(0x0004);
        glDrawArrays(0x0004, 0, 3);
        glEndTransformFeedback();
        Finish(t.ctx);
        float raw[18] = {};
        glBindBuffer(0x8892, xb);
        glGetBufferSubData(0x8892, 0, sizeof(raw), raw);
        bool uvOk = std::fabs(raw[0]) < 0.001f && std::fabs(raw[1]) < 0.001f &&
                    std::fabs(raw[6] - 1.0f) < 0.001f && std::fabs(raw[7]) < 0.001f &&
                    std::fabs(raw[12] - 1.0f) < 0.001f && std::fabs(raw[13] - 1.0f) < 0.001f;
        bool posOk = std::fabs(raw[2] + 1.0f) < 0.001f && std::fabs(raw[3] + 1.0f) < 0.001f &&
                     std::fabs(raw[8] - 1.0f) < 0.001f && std::fabs(raw[9] + 1.0f) < 0.001f &&
                     std::fabs(raw[14] - 1.0f) < 0.001f && std::fabs(raw[15] - 1.0f) < 0.001f;
        t.Check(uvOk, "transform feedback capture v_uv", uvOk ? "" : "wrong packed layout");
        t.Check(posOk, "transform feedback capture gl_Position", posOk ? "" : "wrong packed layout");
        glDeleteBuffers(1, &xb);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(p);
    }

    // XFB array/matrix reflection + capture, XFB-only (không FS): VS out mảng/
    // matrix không phải stage-out hợp lệ của Metal nên chỉ capture kernel chạy.
    {
        const char* vsArr =
            "#version 460 core\n"
            "layout(location=0) in vec2 aPos;\n"
            "out float vArr[2];\n"
            "out mat2 vMat;\n"
            "void main(){ vArr[0]=1.0; vArr[1]=2.0;\n"
            "  vMat=mat2(3.0,4.0,5.0,6.0); gl_Position=vec4(aPos,0,1); }\n";
        GLuint va = Shader(t, 0x8B31, vsArr, "xfb array VS compile");
        GLuint pa = 0;
        if (va) {
            pa = glCreateProgram(); glAttachShader(pa, va);
            const char* vars[3] = {"vArr[0]", "vArr[1]", "vMat"};
            glTransformFeedbackVaryings(pa, 3, vars, 0x8C8C);
            glLinkProgram(pa);
            GLint ok = 0; glGetProgramiv(pa, 0x8B82, &ok);
            t.Check(ok != 0, "xfb array/matrix link", ok ? "" : "link failed");
            if (!ok) { glDeleteProgram(pa); pa = 0; }
        } else {
            t.Check(false, "xfb array/matrix VS", "compile failed");
        }
        if (pa) {
            GLint s0=0,s1=0,s2=0; GLenum ty0=0,ty1=0,ty2=0;
            char n0[64]={},n1[64]={},n2[64]={};
            glGetTransformFeedbackVarying(pa, 0, sizeof(n0), nullptr, &s0, &ty0, n0);
            glGetTransformFeedbackVarying(pa, 1, sizeof(n1), nullptr, &s1, &ty1, n1);
            glGetTransformFeedbackVarying(pa, 2, sizeof(n2), nullptr, &s2, &ty2, n2);
            bool refl = ty0 == 0x1406 && s0 == 1 && ty1 == 0x1406 && s1 == 1 &&
                        ty2 == 0x8B5A && s2 == 1;
            t.Check(refl, "xfb array/matrix reflection",
                    std::string(n0)+","+std::to_string(ty0)+","+std::to_string(s0)+" | "+
                    n2+","+std::to_string(ty2)+","+std::to_string(s2));
            const float verts[] = {-1,-1, 1,-1, 0,1};
            GLuint vao = 0, vbo = 0, xb = 0;
            glGenVertexArrays(1, &vao); glBindVertexArray(vao);
            glGenBuffers(1, &vbo); glBindBuffer(0x8892, vbo);
            glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
            glVertexAttribPointer(0, 2, 0x1406, 0, 0, nullptr);
            glEnableVertexAttribArray(0);
            glGenBuffers(1, &xb); glBindBuffer(0x8892, xb);
            glBufferData(0x8892, 3 * 24, nullptr, 0x88E4);
            glBindTransformFeedback(0x8E22, 0);
            glBindBufferBase(0x8C8E, 0, xb);
            glBindBuffer(0x8892, vbo);
            glUseProgram(pa);
            glViewport(0, 0, 16, 16);
            glEnable(0x8C89 /*RASTERIZER_DISCARD*/);
            glBeginTransformFeedback(0x0004);
            glDrawArrays(0x0004, 0, 3);
            glEndTransformFeedback();
            Finish(t.ctx);
            float raw[18] = {};
            glBindBuffer(0x8892, xb);
            glGetBufferSubData(0x8892, 0, sizeof(raw), raw);
            bool cap = true;
            const float want[6] = {1,2,3,4,5,6};
            for (int i = 0; i < 3 && cap; ++i)
                for (int k = 0; k < 6; ++k)
                    if (std::fabs(raw[i*6+k] - want[k]) > 0.001f) cap = false;
            t.Check(cap, "xfb array/matrix capture", cap ? "" : "wrong packed values");

            // Buffer range 8B → mọi record vượt; overflow query phải set, không GL error.
            ClearErrors();
            glBindBufferRange(0x8C8E, 0, xb, 0, 8);
            GLuint qO = 0, qS = 0;
            glGenQueries(1, &qO); glGenQueries(1, &qS);
            glBeginQuery(0x82EC /*TRANSFORM_FEEDBACK_OVERFLOW*/, qO);
            glBeginQueryIndexed(0x82ED /*STREAM_OVERFLOW*/, 0, qS);
            glBeginTransformFeedback(0x0004);
            glDrawArrays(0x0004, 0, 3);
            glEndTransformFeedback();
            glEndQuery(0x82EC); glEndQueryIndexed(0x82ED, 0);
            Finish(t.ctx);
            GLuint ovf = 0, stm = 0;
            glGetQueryObjectuiv(qO, 0x8866, &ovf);
            glGetQueryObjectuiv(qS, 0x8866, &stm);
            t.Check(glGetError() == 0 && ovf != 0 && stm != 0,
                    "xfb overflow query stream0", "ovf=" + std::to_string(ovf) +
                    " stream=" + std::to_string(stm));
            glDeleteQueries(1, &qO); glDeleteQueries(1, &qS);
            glDisable(0x8C89);
            glDeleteBuffers(1, &xb);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
            glBindTransformFeedback(0x8E22, 0);
            glDeleteProgram(pa);
        }
        if (va) glDeleteShader(va);
    }
    if (v) glDeleteShader(v);
    if (f) glDeleteShader(f);
    glUseProgram(0);
    glBindTransformFeedback(0x8E22, 0);
    glDeleteTransformFeedbacks(1, &x);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
}

void TestIndirect(Suite& t) {
    ClearErrors();
    GLuint fbo = 0;
    GLuint color = TargetTex(16, 16, &fbo);
    GLuint prog = Program(t, kQuadVS, kSample2DFS, "indirect shader");
    if (!prog) return;
    GLuint vao = QuadVAO();
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    GLuint tex = ColorTex(1, 1);
    unsigned char white[4] = {255, 0, 0, 255};
    glTexSubImage2D(0x0DE1, 0, 0, 0, 1, 1, 0x1908, 0x1401, white);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    struct Cmd { uint32_t count, instanceCount, firstVertex, baseInstance; } cmd = {6, 1, 0, 0};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(0x8F3F, ib);
    glBufferData(0x8F3F, sizeof(cmd), &cmd, 0x88E4);
    glViewport(0, 0, 16, 16);
    glDrawArraysIndirect(0x0004, (void*)0);
    unsigned char p[4];
    ReadRGBA(t.ctx, 8, 8, p);
    t.Check(p[0] > 200 && p[1] < 50, "indirect draw CPU fallback",
            "rgba=" + std::to_string(p[0]) + "," + std::to_string(p[1]));
    glDeleteBuffers(1, &ib);
    glDeleteTextures(1, &tex);
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color);
}

void TestCompute(Suite& t) {
    ClearErrors();
    const char* src =
        "#version 460 core\n"
        "layout(local_size_x=8) in;\n"
        "layout(std430,binding=0) buffer B{float v[];};\n"
        "void main(){ uint g=gl_GlobalInvocationID.x; v[g]=float(g)+0.5; }\n";
    GLuint sh = Shader(t, 0x91B9, src, "compute shader");
    if (!sh) return;
    GLuint p = glCreateProgram();
    glAttachShader(p, sh);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char info[512] = {0};
        glGetProgramInfoLog(p, sizeof(info) - 1, nullptr, info);
        t.Check(false, "compute link", info);
        return;
    }
    float z[64] = {0};
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(0x90D2, buf);
    glBufferData(0x90D2, sizeof(z), z, 0x88E4);
    glBindBufferBase(0x90D2, 0, buf);
    glUseProgram(p);
    glDispatchCompute(8, 1, 1);
    glMemoryBarrier(0x0020 | 0x0040);
    glBindBuffer(0x90D2, buf);
    const float* r = (const float*)glMapBufferRange(0x90D2, 0, sizeof(z), 0x0001 | 0x0002 | 0x0010);
    int bad = -1;
    if (r) {
        bad = 0;
        for (int i = 0; i < 64; ++i) if (r[i] != (float)i + 0.5f) ++bad;
        glUnmapBuffer(0x90D2);
    }
    t.Check(bad == 0, "compute dispatch SSBO", bad < 0 ? "map failed" : std::to_string(bad) + "/64 wrong");
    glDeleteBuffers(1, &buf);
    glDeleteProgram(p);
    glDeleteShader(sh);
}

void TestReadbackLayer(Suite& t) {
    ClearErrors();
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x8C1A, tex);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "readback array FBO complete");
    glViewport(0, 0, 4, 4);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char l1[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, l1);
    t.Check(glGetError() == 0 && l1[1] > 200 && l1[0] < 50,
            "glReadPixels FBO layer", "rgba=" + std::to_string(l1[0]) + "," +
            std::to_string(l1[1]));
    glFramebufferTextureLayer(0x8D40, 0x8CE0, tex, 0, 0);
    unsigned char l0[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, l0);
    t.Check(glGetError() == 0 && l0[0] == 0 && l0[1] == 0 && l0[2] == 0,
            "glReadPixels layer 0 không bị clear");
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
}

void TestMipFbo(Suite& t) {
    ClearErrors();
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
    glTexImage2D(0x0DE1, 1, 0x8058, 2, 2, 0, 0x1908, 0x1401, nullptr);
    auto& tx = t.ctx.textures[tex];
    t.Check(tx.gpu && tx.gpu->levelCount() >= 2, "glTexImage2D level>0 promote mip chain",
            "levels=" + std::to_string(tx.gpu ? tx.gpu->levelCount() : 0));
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "mip FBO complete");
    glViewport(0, 0, 2, 2);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    unsigned char p[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, p);
    t.Check(glGetError() == 0 && p[2] > 200 && p[0] < 50,
            "glReadPixels FBO mip level 1", "rgba=" + std::to_string(p[0]) + "," +
            std::to_string(p[2]));
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
}

void TestCopyCube(Suite& t) {
    ClearErrors();
    GLuint src = 0, dst = 0;
    glGenTextures(1, &src);
    glBindTexture(0x8513, src);
    for (int f = 0; f < 6; ++f) {
        unsigned char px[2 * 2 * 4];
        for (int i = 0; i < 4; ++i) {
            px[i * 4 + 0] = (unsigned char)(40 + f * 20);
            px[i * 4 + 1] = (unsigned char)(80 + f * 10);
            px[i * 4 + 2] = (unsigned char)(120 + f * 5);
            px[i * 4 + 3] = 255;
        }
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px);
    }
    glGenTextures(1, &dst);
    glBindTexture(0x8513, dst);
    unsigned char zero[2 * 2 * 4] = {0};
    for (int f = 0; f < 6; ++f)
        glTexImage2D(0x8515 + f, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, zero);
    glCopyImageSubData(src, 0x8513, 0, 0, 0, 2, dst, 0x8513, 0, 0, 0, 4, 2, 2, 2);
    t.Check(glGetError() == 0, "glCopyImageSubData cube faces GPU");
    unsigned char got[2 * 2 * 4] = {0};
    glGetTexImage(0x8515 + 4, 0, 0x1908, 0x1401, got);
    t.Check(glGetError() == 0 && got[0] == 80 && got[1] == 100 && got[2] == 130,
            "glGetTexImage cube face +4", "rgba=" + std::to_string(got[0]) + "," +
            std::to_string(got[1]));
    glDeleteTextures(1, &dst);
    glDeleteTextures(1, &src);
}

void TestStorage3DMip(Suite& t) {
    ClearErrors();
    GLuint arr = 0, fbo = 0;
    glGenTextures(1, &arr);
    glBindTexture(0x8C1A, arr);
    glTexStorage3D(0x8C1A, 3, 0x8058, 4, 4, 2);
    auto& a = t.ctx.textures[arr];
    t.Check(a.gpu && a.gpu->levelCount() == 3, "glTexStorage3D array mip chain",
            "levels=" + std::to_string(a.gpu ? a.gpu->levelCount() : 0));
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arr, 1, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "array mip1 layer1 FBO complete");
    glViewport(0, 0, 2, 2);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    unsigned char p[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, p);
    t.Check(glGetError() == 0 && p[2] > 200 && p[0] < 50,
            "array mip1 clear/read", "rgba=" + std::to_string(p[0]) + "," +
            std::to_string(p[2]));

    GLuint larr = 0;
    glGenTextures(1, &larr);
    glBindTexture(0x8C1A, larr);
    unsigned char ab[4 * 4 * 2 * 4];
    for (int i = 0; i < 4 * 4 * 2 * 4; ++i) ab[i] = (unsigned char)(i & 0xFF);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, ab);
    unsigned char am[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) am[i] = (unsigned char)(50 + i);
    glTexImage3D(0x8C1A, 1, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, am);
    auto& la = t.ctx.textures[larr];
    t.Check(la.gpu && la.gpu->levelCount() == 2, "legacy array level>0 promote",
            "levels=" + std::to_string(la.gpu ? la.gpu->levelCount() : 0));
    glFramebufferTextureLayer(0x8D40, 0x8CE0, larr, 1, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "legacy array mip1 FBO complete");
    glViewport(0, 0, 2, 2);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char lp[4] = {0};
    glReadPixels(0, 0, 1, 1, 0x1908, 0x1401, lp);
    t.Check(glGetError() == 0 && lp[1] > 200 && lp[0] < 50,
            "legacy array mip1 clear/read");
    glDeleteTextures(1, &larr);

    GLuint tex3 = 0;
    glGenTextures(1, &tex3);
    glBindTexture(0x806F, tex3);
    glTexStorage3D(0x806F, 3, 0x8058, 4, 4, 4);
    auto& t3 = t.ctx.textures[tex3];
    t.Check(t3.gpu && t3.gpu->levelCount() == 3, "glTexStorage3D 3D mip chain",
            "levels=" + std::to_string(t3.gpu ? t3.gpu->levelCount() : 0));
    unsigned char px[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) px[i] = (unsigned char)(10 + i);
    glTexSubImage3D(0x806F, 1, 0, 0, 0, 2, 2, 2, 0x1908, 0x1401, px);
    unsigned char got[2 * 2 * 2 * 4] = {0};
    glGetTextureImage(tex3, 1, 0x1908, 0x1401, sizeof(got), got);
    t.Check(glGetError() == 0 && got[0] == 10, "3D storage mip1 readback");

    GLuint legacy = 0;
    glGenTextures(1, &legacy);
    glBindTexture(0x806F, legacy);
    unsigned char base[4 * 4 * 4 * 4];
    for (int i = 0; i < 4 * 4 * 4 * 4; ++i) base[i] = (unsigned char)(i & 0xFF);
    glTexImage3D(0x806F, 0, 0x8058, 4, 4, 4, 0, 0x1908, 0x1401, base);
    unsigned char m1[2 * 2 * 2 * 4];
    for (int i = 0; i < 2 * 2 * 2 * 4; ++i) m1[i] = (unsigned char)(100 + i);
    glTexImage3D(0x806F, 1, 0x8058, 2, 2, 2, 0, 0x1908, 0x1401, m1);
    auto& lg = t.ctx.textures[legacy];
    t.Check(lg.gpu && lg.gpu->levelCount() == 2, "legacy 3D level>0 promote",
            "levels=" + std::to_string(lg.gpu ? lg.gpu->levelCount() : 0));

    GLuint cube = 0;
    glGenTextures(1, &cube);
    glBindTexture(0x8513, cube);
    unsigned char cb[8 * 8 * 4];
    for (int i = 0; i < 8 * 8 * 4; ++i) cb[i] = (unsigned char)(i & 0xFF);
    for (int f = 0; f < 6; ++f)
        glTexImage2D(0x8515 + f, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, cb);
    unsigned char cm[4 * 4 * 4] = {0};
    glTexImage2D(0x8519, 1, 0x8058, 4, 4, 0, 0x1908, 0x1401, cm);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x8519, cube, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "cube mip1 FBO complete");
    glViewport(0, 0, 4, 4);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(0x00004000);
    unsigned char cg[4 * 4 * 4] = {0};
    glGetTexImage(0x8519, 1, 0x1908, 0x1401, cg);
    t.Check(glGetError() == 0 && cg[0] > 200 && cg[1] < 50,
            "cube mip1 clear/get", "rgba=" + std::to_string(cg[0]) + "," +
            std::to_string(cg[1]));
    unsigned char csub[2 * 2 * 4];
    for (int i = 0; i < 2 * 2; ++i) {
        csub[i * 4 + 0] = 0; csub[i * 4 + 1] = 255;
        csub[i * 4 + 2] = 0; csub[i * 4 + 3] = 255;
    }
    glTexSubImage2D(0x8519, 1, 1, 1, 2, 2, 0x1908, 0x1401, csub);
    unsigned char cs[4 * 4 * 4] = {0};
    glGetTexImage(0x8519, 1, 0x1908, 0x1401, cs);
    t.Check(glGetError() == 0 && cs[0] > 200 && cs[((size_t)1 * 4 + 1) * 4 + 1] > 200,
            "cube mip1 subimage giữ nền + ghi vùng");
    glDeleteTextures(1, &cube);
    glDeleteTextures(1, &legacy);
    glDeleteTextures(1, &tex3);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &arr);
}

void TestTessellation(Suite& t) {
    ClearErrors();
    glBindFramebuffer(0x8D40, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    glDisable(0x0C11); // SCISSOR_TEST
    glDisable(0x0B71); // DEPTH_TEST
    glDisable(0x0BE2); // BLEND
    glDisable(0x0B90); // STENCIL_TEST
    glDisable(0x0B44); // CULL_FACE
    glColorMask(1, 1, 1, 1);
    glDepthMask(1);
    Finish(t.ctx);
    ClearErrors();
    auto prevTarget = t.ctx.device->defaultRenderTarget();
    auto tessTarget = t.ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!tessTarget) { t.Skip("tess default target", "makeRenderTarget null"); return; }
    t.ctx.device->setDefaultRenderTarget(tessTarget);
    static const char* vsSrc =
        "#version 460 core\n"
        "void main() { gl_Position = vec4(0.0); }\n";
    static const char* tcsSrc =
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
    static const char* tesSrc =
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
    static const char* fsSrc =
        "#version 460 core\n"
        "in vec4 vColor;\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "void main() { fragColor = vColor; }\n";
    GLuint vs = Shader(t, 0x8B31, vsSrc, "tess VS compile");
    GLuint tc = Shader(t, 0x8E88, tcsSrc, "tess TCS compile");
    GLuint te = Shader(t, 0x8E87, tesSrc, "tess TES compile");
    GLuint fs = Shader(t, 0x8B30, fsSrc, "tess FS compile");
    if (!vs || !tc || !te || !fs) return;
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    t.Check(ok != 0, "tess VS+TCS+TES+FS link");
    if (ok) {
        glUseProgram(p);
        int tw = (int)tessTarget->width(), th = (int)tessTarget->height();
        glViewport(0, 0, tw, th);
        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(0x00004000);
        auto mf0 = t.ctx.appleStats.miscFail;
        auto np0 = t.ctx.appleStats.noPipeline;
        glDrawArrays(0x000E, 0, 3);
        Finish(t.ctx);
        unsigned char px[4] = {0};
        glReadPixels(tw / 2, th / 2, 1, 1, 0x1908, 0x1401, px);
        GLint drawFbo = -1;
        glGetIntegerv(0x8CA6, &drawFbo);
        t.Check(glGetError() == 0 && px[0] > 200 && px[1] < 50 && px[2] < 50,
                "tess drawPatches center đỏ", "rgba=" + std::to_string(px[0]) + "," +
                std::to_string(px[1]) + "," + std::to_string(px[2]) +
                " fbo=" + std::to_string((int)drawFbo) +
                " miscFail+=" + std::to_string(t.ctx.appleStats.miscFail - mf0) +
                " noPipe+=" + std::to_string(t.ctx.appleStats.noPipeline - np0));
    }
    // Triangle domain với output CP 5: TCS không ghi TessLevel → dùng default
    // levels; gl_in.length() của TCS phải là input patch CP (3).
    {
        GLuint vao5 = 0;
        glGenVertexArrays(1, &vao5);
        glBindVertexArray(vao5);
        static const char* vs5 =
            "#version 460 core\n"
            "void main() { gl_Position = vec4(0.0); }\n";
        static const char* tcs5 =
            "#version 460 core\n"
            "layout(vertices = 5) out;\n"
            "out vec2 vPos[];\n"
            "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
            "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
            "patch out vec4 pColor;\n"
            "void main() {\n"
            "  vec2 c[5] = vec2[5](vec2(-0.8,-0.8), vec2(-0.2,-0.8), vec2(-0.2,0.8), vec2(-0.8,0.8), vec2(-0.5,0.0));\n"
            "  float shift = float(gl_PrimitiveID) * 0.9;\n"
            "  vPos[gl_InvocationID] = c[gl_InvocationID] + vec2(shift, 0.0);\n"
            "  gl_out[gl_InvocationID].gl_Position = vec4(vPos[gl_InvocationID], 0.0, 1.0);\n"
            "  bool ok = (gl_PatchVerticesIn == 3 && gl_in.length() == 3);\n"
            "  pColor = !ok ? vec4(0.0,0.0,1.0,1.0)\n"
            "               : (gl_PrimitiveID == 0 ? vec4(1.0,0.0,0.0,1.0)\n"
            "                                      : vec4(0.0,1.0,0.0,1.0));\n"
            "}\n";
        static const char* tes5 =
            "#version 460 core\n"
            "layout(triangles) in;\n"
            "in vec2 vPos[];\n"
            "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
            "patch in vec4 pColor;\n"
            "out vec4 vColor;\n"
            "void main() {\n"
            "  vec3 uv = gl_TessCoord;\n"
            "  vec2 p = vPos[0]*uv.x + vPos[1]*uv.y + vPos[2]*uv.z;\n"
            "  vec4 col = pColor;\n"
            "  if (gl_PatchVerticesIn != 5 || gl_in.length() != 5\n"
            "      || gl_TessLevelOuter[0] < 1.5 || gl_TessLevelInner[0] < 1.5)\n"
            "    col = vec4(0.0,0.0,1.0,1.0);\n"
            "  vColor = col;\n"
            "  gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        GLuint vs5s = Shader(t, 0x8B31, vs5, "tess triN5 VS compile");
        GLuint tc5s = Shader(t, 0x8E88, tcs5, "tess triN5 TCS compile");
        GLuint te5s = Shader(t, 0x8E87, tes5, "tess triN5 TES compile");
        GLuint fs5s = Shader(t, 0x8B30, fsSrc, "tess triN5 FS compile");
        if (vs5s && tc5s && te5s && fs5s) {
            GLuint p5 = glCreateProgram();
            glAttachShader(p5, vs5s); glAttachShader(p5, tc5s);
            glAttachShader(p5, te5s); glAttachShader(p5, fs5s);
            glLinkProgram(p5);
            GLint ok5 = 0;
            glGetProgramiv(p5, 0x8B82, &ok5);
            t.Check(ok5 != 0, "tess triangle N=5 link");
            if (ok5) {
                glUseProgram(p5);
                glPatchParameteri(0x8E72, 3);
                float dO[4] = {2.0f, 2.0f, 2.0f, 2.0f};
                float dI[2] = {2.0f, 2.0f};
                glPatchParameterfv(0x8E74, dO);
                glPatchParameterfv(0x8E73, dI);
                glViewport(0, 0, 64, 64);
                glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
                glClear(0x00004000);
                glDrawArrays(0x000E, 0, 6);
                Finish(t.ctx);
                unsigned char l5[4] = {0}, r5[4] = {0};
                glReadPixels(16, 32, 1, 1, 0x1908, 0x1401, l5);
                glReadPixels(48, 32, 1, 1, 0x1908, 0x1401, r5);
                t.Check(glGetError() == 0 && l5[0] > 200 && l5[1] < 50 && l5[2] < 50 &&
                        r5[1] > 200 && r5[0] < 50 && r5[2] < 50,
                        "tess triangle output CP 5 + default levels",
                        "left=" + std::to_string(l5[0]) + "," + std::to_string(l5[1]) + "," +
                        std::to_string(l5[2]) + " right=" + std::to_string(r5[0]) + "," +
                        std::to_string(r5[1]) + "," + std::to_string(r5[2]));
                float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float one2[2] = {1.0f, 1.0f};
                glPatchParameterfv(0x8E74, one);
                glPatchParameterfv(0x8E73, one2);
            }
            glDeleteProgram(p5);
        }
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao5);
        ClearErrors();
    }

    // gl_InstanceID trong TCS/TES + gl_PrimitiveID local (2 instance × 1 patch).
    {
        GLuint vaoI = 0;
        glGenVertexArrays(1, &vaoI);
        glBindVertexArray(vaoI);
        static const char* vsI =
            "#version 460 core\n"
            "void main() { gl_Position = vec4(0.0); }\n";
        static const char* tcsI =
            "#version 460 core\n"
            "layout(vertices = 3) out;\n"
            "patch out vec4 pColor; patch out vec2 pShift;\n"
            "void main() {\n"
            "  pShift = vec2(float(gl_InstanceID) * 0.9, 0.0);\n"
            "  bool ok = (gl_PatchVerticesIn == 3 && gl_PrimitiveID == 0);\n"
            "  pColor = !ok ? vec4(0.0,0.0,1.0,1.0)\n"
            "               : (gl_InstanceID == 0 ? vec4(1.0,0.0,0.0,1.0)\n"
            "                                     : vec4(0.0,1.0,0.0,1.0));\n"
            "}\n";
        static const char* tesI =
            "#version 460 core\n"
            "layout(triangles) in;\n"
            "patch in vec4 pColor; patch in vec2 pShift;\n"
            "out vec4 vColor;\n"
            "void main() {\n"
            "  vec2 uv = gl_TessCoord.xy;\n"
            "  vec2 p = vec2(-0.8 + 1.6 * uv.x, -0.8 + 1.6 * uv.y) + pShift;\n"
            "  vec4 col = pColor;\n"
            "  if (gl_PatchVerticesIn != 3 || gl_PrimitiveID != 0 || gl_InstanceID > 1)\n"
            "    col = vec4(0.0,0.0,1.0,1.0);\n"
            "  else if (gl_InstanceID == 1) col.b = 0.5;\n"
            "  vColor = col; gl_Position = vec4(p, 0.0, 1.0);\n"
            "}\n";
        GLuint vsIs = Shader(t, 0x8B31, vsI, "tess inst VS compile");
        GLuint tcIs = Shader(t, 0x8E88, tcsI, "tess inst TCS compile");
        GLuint teIs = Shader(t, 0x8E87, tesI, "tess inst TES compile");
        GLuint fsIs = Shader(t, 0x8B30, fsSrc, "tess inst FS compile");
        if (vsIs && tcIs && teIs && fsIs) {
            GLuint pI = glCreateProgram();
            glAttachShader(pI, vsIs); glAttachShader(pI, tcIs);
            glAttachShader(pI, teIs); glAttachShader(pI, fsIs);
            glLinkProgram(pI);
            GLint okI = 0;
            glGetProgramiv(pI, 0x8B82, &okI);
            t.Check(okI != 0, "tess TCS/TES gl_InstanceID link");
            if (okI) {
                glUseProgram(pI);
                glPatchParameteri(0x8E72, 3);
                float dO[4] = {2.0f, 2.0f, 2.0f, 2.0f};
                float dI[2] = {2.0f, 2.0f};
                glPatchParameterfv(0x8E74, dO);
                glPatchParameterfv(0x8E73, dI);
                glViewport(0, 0, 64, 64);
                glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
                glClear(0x00004000);
                glDrawArraysInstanced(0x000E, 0, 3, 2);
                Finish(t.ctx);
                unsigned char li[4] = {0}, ri[4] = {0};
                glReadPixels(16, 32, 1, 1, 0x1908, 0x1401, li);
                glReadPixels(48, 32, 1, 1, 0x1908, 0x1401, ri);
                t.Check(glGetError() == 0 && li[0] > 200 && li[1] < 50 && li[2] < 50 &&
                        ri[1] > 200 && ri[0] < 50 && ri[2] > 90 && ri[2] < 180,
                        "tess TCS/TES gl_InstanceID + primitive local",
                        "left=" + std::to_string(li[0]) + "," + std::to_string(li[1]) + "," +
                        std::to_string(li[2]) + " right=" + std::to_string(ri[0]) + "," +
                        std::to_string(ri[1]) + "," + std::to_string(ri[2]));
                float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                float one2[2] = {1.0f, 1.0f};
                glPatchParameterfv(0x8E74, one);
                glPatchParameterfv(0x8E73, one2);
            }
            glDeleteProgram(pI);
        }
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vaoI);
        ClearErrors();
    }

    // T4b: VS prepass thật (attribute + gl_in + TCS varying passthrough).
    static const char* vsT4bSrc =
        "#version 460 core\n"
        "layout(location = 0) in vec2 aPos;\n"
        "layout(location = 1) in float aBias;\n"
        "layout(location = 2) in vec2 aInst;\n"
        "uniform float uBias;\n"
        "out vec2 vPos; out float vBias;\n"
        "void main() { vPos = aPos + aInst; vBias = aBias + uBias;\n"
        "  gl_Position = vec4(aPos + aInst, 0.0, 1.0); }\n";
    static const char* tcsT4bSrc =
        "#version 460 core\n"
        "layout(vertices = 3) out;\n"
        "in vec2 vPos[]; out vec2 vPos[];\n"
        "in float vBias[]; out float vBias[];\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
        "uniform vec4 uPatchColor;\n"
        "uniform sampler2D uTex;\n"
        "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
        "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
        "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
        "layout(binding = 3, offset = 0) uniform atomic_uint uCounter;\n"
        "patch out vec4 pColor;\n"
        "void main() {\n"
        "  uint cnt = atomicCounterIncrement(uCounter);\n"
        "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
        "  vBias[gl_InvocationID] = vBias[gl_InvocationID];\n"
    "  gl_out[gl_InvocationID].gl_Position = vec4(vPos[gl_InvocationID], 0.25, 1.0);\n"
        "  if (gl_InvocationID == 0) {\n"
        "    pColor = (vBias[0] > 0.5) ? uPatchColor * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)) : vec4(0.0,1.0,0.0,1.0);\n"
        "  }\n"
        "  gl_TessLevelOuter[0]=1.0; gl_TessLevelOuter[1]=1.0; gl_TessLevelOuter[2]=1.0;\n"
        "  gl_TessLevelInner[0]=1.0;\n"
        "}\n";
    static const char* tesT4bSrc =
        "#version 460 core\n"
        "layout(triangles) in;\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "in vec2 vPos[]; patch in vec4 pColor; out vec4 vColor;\n"
        "uniform vec4 uTint;\n"
        "uniform sampler2D uTex;\n"
        "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
        "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
        "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
        "void main() {\n"
        "  vec2 p = vPos[0]*gl_TessCoord.x + vPos[1]*gl_TessCoord.y + vPos[2]*gl_TessCoord.z;\n"
        "  float cpW = (gl_in[0].gl_Position.z > 0.2 ? 1.0 : 0.0) *\n"
    "              (gl_in.length() == 3 ? 1.0 : 0.0);\n"
        "  vColor = pColor * uTint * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)) * cpW; gl_Position = vec4(p, 0.0, 1.0);\n"
        "}\n";
    static const char* fsT4bSrc =
        "#version 460 core\n"
        "in vec4 vColor;\n"
        "uniform float uMul;\n"
        "uniform sampler2D uTex;\n"
        "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
        "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
        "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "layout(location = 1) out vec4 fragSecond;\n"
        "void main() { fragColor = vColor * uMul * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)); fragSecond = vec4(0.0, 1.0, 0.0, 1.0); }\n";
    GLuint vs4 = Shader(t, 0x8B31, vsT4bSrc, "tess T4b VS compile");
    GLuint tc4 = Shader(t, 0x8E88, tcsT4bSrc, "tess T4b TCS compile");
    GLuint te4 = Shader(t, 0x8E87, tesT4bSrc, "tess T4b TES compile");
    GLuint fs4 = Shader(t, 0x8B30, fsT4bSrc, "tess T4b FS compile");
    if (vs4 && tc4 && te4 && fs4) {
        GLuint p2 = glCreateProgram();
        glAttachShader(p2, vs4); glAttachShader(p2, tc4); glAttachShader(p2, te4); glAttachShader(p2, fs4);
        glLinkProgram(p2);
        GLint ok2 = 0;
        glGetProgramiv(p2, 0x8B82, &ok2);
        char plog2[2048] = {};
        if (!ok2) glGetProgramInfoLog(p2, sizeof(plog2), nullptr, plog2);
        t.Check(ok2 != 0, "tess T4b VS attr + TCS in link", plog2);
        if (ok2) {
            static const float verts[] = {
                0,0, 0,0, 0,0,
                -1,-1, 3,-1, -1,3,
            };
            GLuint vao4 = 0, vbo4 = 0;
            glGenVertexArrays(1, &vao4);
            glBindVertexArray(vao4);
            glGenBuffers(1, &vbo4);
            glBindBuffer(0x8892, vbo4);
            glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
            glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttrib1f(1, 0.0f); // array tắt → current generic value
            glVertexAttrib2f(2, 0.0f, 0.0f); // aInst current generic cho draw thường
            glUseProgram(p2);
            GLint uBias = glGetUniformLocation(p2, "uBias");
            GLint uPatch = glGetUniformLocation(p2, "uPatchColor");
            GLint uTint = glGetUniformLocation(p2, "uTint");
            GLint uMul = glGetUniformLocation(p2, "uMul");
            GLint uTex = glGetUniformLocation(p2, "uTex");
            glUniform1f(uBias, 1.0f);
            glUniform4f(uPatch, 1.0f, 0.0f, 0.0f, 1.0f);
            glUniform4f(uTint, 1.0f, 1.0f, 1.0f, 1.0f);
            glUniform1f(uMul, 1.0f);
            glUniform1i(uTex, 0);
            static const unsigned char texel[4] = {255, 0, 0, 255};
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glActiveTexture(0x84C0);
            glBindTexture(0x0DE1, tex);
            glTexParameteri(0x0DE1, 0x2801, 0x2601);
            glTexParameteri(0x0DE1, 0x2800, 0x2601);
            glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, texel);
            const float blockColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            GLuint ubo = 0, ssbo = 0;
            glGenBuffers(1, &ubo);
            glBindBuffer(0x8A11, ubo);
            glBufferData(0x8A11, sizeof(blockColor), blockColor, 0x88E4);
            glBindBufferBase(0x8A11, 0, ubo);
            glGenBuffers(1, &ssbo);
            glBindBuffer(0x90D2, ssbo);
            glBufferData(0x90D2, sizeof(blockColor), blockColor, 0x88E4);
            glBindBufferBase(0x90D2, 1, ssbo);
            glBindImageTexture(2, tex, 0, 0, 0, 0x88B8, 0x8058);
            GLuint acbuf = 0;
            const unsigned int aczero = 0;
            glGenBuffers(1, &acbuf);
            glBindBuffer(0x92C0, acbuf);
            glBufferData(0x92C0, sizeof(aczero), &aczero, 0x88E4);
            glBindBufferBase(0x92C0, 3, acbuf);
            glClear(0x00004000);
            auto mf1 = t.ctx.appleStats.miscFail;
            glDrawArrays(0x000E, 3, 3);
            Finish(t.ctx);
            unsigned char px4[4] = {0};
            glReadPixels((int)tessTarget->width() / 2, (int)tessTarget->height() / 2,
                         1, 1, 0x1908, 0x1401, px4);
            t.Check(glGetError() == 0 && px4[0] > 200 && px4[1] < 50 && px4[2] < 50 &&
                    uBias >= 0 && uPatch >= 0 && uTint >= 0 && uMul >= 0 && uTex >= 0,
                    "tess T4b VS capture + uniform/sampler/UBO/SSBO/image/atomic/gl_out->gl_in đỏ", "rgba=" + std::to_string(px4[0]) + "," +
                    std::to_string(px4[1]) + "," + std::to_string(px4[2]) +
                    " locs=" + std::to_string((int)uBias) + "/" +
                    std::to_string((int)uPatch) + "/" + std::to_string((int)uTint) + "/" +
                    std::to_string((int)uMul) + "/" + std::to_string((int)uTex) +
                    " miscFail+=" + std::to_string(t.ctx.appleStats.miscFail - mf1));
            Finish(t.ctx);
            unsigned int counter = 0;
            glBindBuffer(0x92C0, acbuf);
            glGetBufferSubData(0x92C0, 0, sizeof(counter), &counter);
            t.Check(counter > 0, "tess T4b atomic counter tăng",
                    "counter=" + std::to_string(counter));
            // FBO target thật (không phải default target tự đặt).
            GLuint ftex = 0, gfbo = 0;
            glGenTextures(1, &ftex);
            glBindTexture(0x0DE1, ftex);
            glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
            glGenFramebuffers(1, &gfbo);
            glBindFramebuffer(0x8D40, gfbo);
            glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, ftex, 0);
            glBindTexture(0x0DE1, tex); // unit0 vẫn là texture đỏ cho uTex
            t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
                    "tess T4b FBO complete");
            glViewport(0, 0, 64, 64);
            glClear(0x00004000);
            glDrawArrays(0x000E, 3, 3);
            Finish(t.ctx);
            unsigned char pxF[4] = {0};
            glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, pxF);
            t.Check(glGetError() == 0 && pxF[0] > 200 && pxF[1] < 50 && pxF[2] < 50,
                    "tess T4b FBO center đỏ", "rgba=" + std::to_string(pxF[0]) + "," +
                    std::to_string(pxF[1]) + "," + std::to_string(pxF[2]));
            glBindFramebuffer(0x8D40, 0);
            glDeleteFramebuffers(1, &gfbo);
            glDeleteTextures(1, &ftex);
            // MSAA tess target + resolve blit (pipeline rasterSampleCount=4).
            GLuint mtex = 0, mfbo = 0, rtex = 0, rfbo = 0;
            glGenTextures(1, &mtex);
            glBindTexture(0x9100, mtex);
            glTexImage2DMultisample(0x9100, 4, 0x8058, 64, 64, 1);
            glGenFramebuffers(1, &mfbo);
            glBindFramebuffer(0x8D40, mfbo);
            glFramebufferTexture2D(0x8D40, 0x8CE0, 0x9100, mtex, 0);
            glBindTexture(0x0DE1, tex);
            t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
                    "tess T4b MSAA FBO complete");
            glGenTextures(1, &rtex);
            glBindTexture(0x0DE1, rtex);
            glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
            glGenFramebuffers(1, &rfbo);
            glBindFramebuffer(0x8D40, rfbo);
            glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, rtex, 0);
            glBindTexture(0x0DE1, tex);
            glBindFramebuffer(0x8D40, mfbo);
            glViewport(0, 0, 64, 64);
            glClear(0x00004000);
            glDrawArrays(0x000E, 3, 3);
            Finish(t.ctx);
            glBindFramebuffer(0x8CA8, mfbo);
            glBindFramebuffer(0x8CA9, rfbo);
            glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64, 0x00004000, 0x2600);
            glBindFramebuffer(0x8D40, rfbo);
            unsigned char pxM[4] = {0};
            glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, pxM);
            t.Check(glGetError() == 0 && pxM[0] > 200 && pxM[1] < 50 && pxM[2] < 50,
                    "tess T4b MSAA resolve center đỏ", "rgba=" + std::to_string(pxM[0]) + "," +
                    std::to_string(pxM[1]) + "," + std::to_string(pxM[2]));
            glBindFramebuffer(0x8D40, 0);
            glDeleteFramebuffers(1, &mfbo);
            glDeleteFramebuffers(1, &rfbo);
            glDeleteTextures(1, &mtex);
            glDeleteTextures(1, &rtex);
            // MRT tess: color0 đỏ, color1 xanh lá.
            GLuint mt0 = 0, mt1 = 0, mfbo2 = 0;
            glGenTextures(1, &mt0);
            glBindTexture(0x0DE1, mt0);
            glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
            glGenTextures(1, &mt1);
            glBindTexture(0x0DE1, mt1);
            glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
            glGenFramebuffers(1, &mfbo2);
            glBindFramebuffer(0x8D40, mfbo2);
            glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, mt0, 0);
            glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, mt1, 0);
            glBindTexture(0x0DE1, tex);
            GLenum bufs2[2] = {0x8CE0, 0x8CE1};
            glDrawBuffers(2, bufs2);
            t.Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5,
                    "tess T4b MRT FBO complete");
            glViewport(0, 0, 64, 64);
            glClear(0x00004000);
            glDrawArrays(0x000E, 3, 3);
            Finish(t.ctx);
            unsigned char m0[4] = {0}, m1[4] = {0};
            glReadBuffer(0x8CE0);
            glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, m0);
            glReadBuffer(0x8CE1);
            glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, m1);
            t.Check(glGetError() == 0 && m0[0] > 200 && m0[1] < 50 && m0[2] < 50 &&
                    m1[0] < 50 && m1[1] > 200 && m1[2] < 50,
                    "tess T4b MRT attachments đỏ/xanh",
                    "att0=" + std::to_string(m0[0]) + "," + std::to_string(m0[1]) + "," +
                    std::to_string(m0[2]) + " att1=" + std::to_string(m1[0]) + "," +
                    std::to_string(m1[1]) + "," + std::to_string(m1[2]));
            glBindFramebuffer(0x8D40, 0);
            glDeleteFramebuffers(1, &mfbo2);
            glDeleteTextures(1, &mt0);
            glDeleteTextures(1, &mt1);
            // Instancing tess: 2 instance, per-instance divisor=1; TCS chạy 6
            // invocation và cả hai patch đều đỏ sau khi đọc left/right.
            GLuint ivbo = 0;
            const float instData[] = {-2.0f, 0.0f, 1.0f, 0.0f};
            glGenBuffers(1, &ivbo);
            glBindBuffer(0x8892, ivbo);
            glBufferData(0x8892, sizeof(instData), instData, 0x88E4);
            glBindVertexArray(vao4);
            glBindBuffer(0x8892, ivbo);
            glVertexAttribBinding(2, 1);
            glVertexAttribPointer(2, 2, 0x1406, 0, 8, (void*)0);
            glEnableVertexAttribArray(2);
            glVertexAttribDivisor(2, 1);
            glVertexBindingDivisor(1, 1);
            glViewport(0, 0, 64, 64);
            glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
            glClear(0x00004000);
            unsigned int instBefore = 0;
            glBindBuffer(0x92C0, acbuf);
            glGetBufferSubData(0x92C0, 0, sizeof(instBefore), &instBefore);
            glDrawArraysInstanced(0x000E, 3, 3, 2);
            Finish(t.ctx);
            unsigned int instAfter = 0;
            glBindBuffer(0x92C0, acbuf);
            glGetBufferSubData(0x92C0, 0, sizeof(instAfter), &instAfter);
            unsigned char ipL[4] = {0}, ipR[4] = {0};
            glReadPixels(4, 32, 1, 1, 0x1908, 0x1401, ipL);
            glReadPixels(60, 32, 1, 1, 0x1908, 0x1401, ipR);
            t.Check(glGetError() == 0 && ipL[0] > 200 && ipL[1] < 50 && ipL[2] < 50 &&
                    ipR[0] > 200 && ipR[1] < 50 && ipR[2] < 50 &&
                    instAfter - instBefore == 6,
                    "tess T4b instancing per-instance attr + dispatch",
                    "left=" + std::to_string(ipL[0]) + "," + std::to_string(ipL[1]) + "," +
                    std::to_string(ipL[2]) + " right=" + std::to_string(ipR[0]) + "," +
                    std::to_string(ipR[1]) + "," + std::to_string(ipR[2]) +
                    " counter+=" + std::to_string(instAfter - instBefore));
            glDisableVertexAttribArray(2);
            glVertexAttribDivisor(2, 0);
            glBindVertexArray(0);
            glDeleteBuffers(1, &ivbo);
            glBindVertexArray(vao4);
            glDeleteBuffers(1, &acbuf);
            glDeleteBuffers(1, &ssbo);
            glDeleteBuffers(1, &ubo);
            glDeleteTextures(1, &tex);
            glDeleteBuffers(1, &vbo4);
            glBindVertexArray(0);
            glDeleteVertexArrays(1, &vao4);
        }
        glDeleteProgram(p2);
    }
    if (vs4) glDeleteShader(vs4);
    if (tc4) glDeleteShader(tc4);
    if (te4) glDeleteShader(te4);
    if (fs4) glDeleteShader(fs4);
    // T2 chỉ hỗ trợ gl_in.gl_Position/PointSize → gl_ClipDistance fail rõ.
    static const char* tesGlInSrc =
        "#version 460 core\n"
        "layout(triangles) in;\n"
        "in vec2 vPos[];\n"
        "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
        "patch in vec4 pColor; out vec4 vColor;\n"
        "void main() { vColor = pColor + vec4(gl_in[0].gl_ClipDistance[0]);\n"
        "  gl_Position = vec4(vPos[0], 0.0, 1.0); }\n";
    GLuint teIn = glCreateShader(0x8E87);
    glShaderSource(teIn, 1, &tesGlInSrc, nullptr);
    glCompileShader(teIn);
    GLint okIn = 1;
    glGetShaderiv(teIn, 0x8B81, &okIn);
    char inLog[1024] = {};
    if (!okIn) glGetShaderInfoLog(teIn, sizeof(inLog) - 1, nullptr, inLog);
    t.Check(okIn == 0, "tess TES gl_ClipDistance compile-fail trung thực", inLog);
    glDeleteShader(teIn);
    glDeleteProgram(p);
    glDeleteShader(vs); glDeleteShader(tc); glDeleteShader(te); glDeleteShader(fs);
    glUseProgram(0);
    glBindFramebuffer(0x8D40, 0);
    t.ctx.device->setDefaultRenderTarget(prevTarget);
}

void TestFence(Suite& t) {
    ClearErrors();
    GLsync s = glFenceSync(0x9117, 0);
    GLenum st = glClientWaitSync(s, 0, 1000000000ull);
    t.Check(s != nullptr && (st == 0x911A || st == 0x9119),
            "fence sync", "status=" + ErrName(st));
    if (s) glDeleteSync(s);
}

} // namespace

std::string RunDeviceTests(Context& ctx, const std::function<void(const char*)>& log) {
    Suite t{ctx, log};
    t.Say("=== TGLMT device test suite ===");
    TestInfo(t);
    TestFboBasic(t);
    TestArray(t);
    Test3D(t);
    TestMipCopy(t);
    TestMsaa(t);
    TestStencil(t);
    TestCompressedContract(t);
    TestPipelineContract(t);
    TestClipDistance(t);
    TestTransformFeedback(t);
    TestIndirect(t);
    TestCompute(t);
    TestReadbackLayer(t);
    TestMipFbo(t);
    TestCopyCube(t);
    TestStorage3DMip(t);
    TestTessellation(t);
    TestFence(t);
    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(0x8D40, 0);
    glBindFramebuffer(0x8CA8, 0);
    glBindFramebuffer(0x8CA9, 0);
    ClearErrors();
    std::string summary = "=== SUMMARY: " + std::to_string(t.pass) + " pass, " +
                          std::to_string(t.fail) + " fail, " +
                          std::to_string(t.skip) + " skip ===";
    t.Say(summary);
    return summary;
}
