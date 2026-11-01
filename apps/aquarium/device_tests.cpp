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
    t.Say("INFO transform feedback: state only, no varying capture on Metal");
    glBindTransformFeedback(0x8E22, 0);
    glDeleteTransformFeedbacks(1, &x);
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
    TestTransformFeedback(t);
    TestIndirect(t);
    TestCompute(t);
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
