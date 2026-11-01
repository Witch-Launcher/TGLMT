// test_mdi_count_baseinstance_apple.cpp — Voxy MDIC regression:
//  - draw count + DrawElementsIndirectCommand do compute shader ghi;
//    CPU đọc phải chờ compute (SyncComputeWritesTo) thay vì thấy 0/stale.
//  - baseInstance != 0 phải tới Metal base_instance (quads3.vert dùng
//    positionBuffer[gl_BaseInstance]).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "#extension GL_ARB_shader_draw_parameters : require\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() {\n"
    "  float x = (gl_BaseInstance == 0u) ? -0.5 : 0.5;\n"
    "  gl_Position = vec4(pos + vec2(x, 0.0), 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) out vec4 c;\n"
    "void main() { c = vec4(1.0, 0.0, 0.0, 1.0); }\n";
static const char* kCS =
    "#version 460 core\n"
    "layout(local_size_x = 1) in;\n"
    "layout(std430, binding = 0) buffer Cmd { uint cmd[]; };\n"
    "layout(std430, binding = 1) buffer Cnt { uint cnt[]; };\n"
    "void main() {\n"
    "  // 2 draw commands: 3 indices, 1 instance, firstIndex 0, baseVertex 0\n"
    "  cmd[0]=3u; cmd[1]=1u; cmd[2]=0u; cmd[3]=0u; cmd[4]=0u;\n"
    "  cmd[5]=3u; cmd[6]=1u; cmd[7]=0u; cmd[8]=0u; cmd[9]=1u;\n"
    "  cnt[0]=2u;\n"
    "  uint x=0u;\n"
    "  for (uint i=0u; i<1000000000u; ++i) x = x*1664525u + 1013904223u;\n"
    "  cnt[1]=x; // giữ vòng lặp, đồng thời trì hoãn compute so với CPU read\n"
    "}\n";

static GLuint Shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[512]; GLsizei n = 0;
        glGetShaderInfoLog(s, sizeof(log), &n, log);
        printf("compile 0x%x FAIL: %.400s\n", type, log);
        return 0;
    }
    return s;
}
static GLuint Program(const char* vs, const char* fs) {
    GLuint v = Shader(0x8B31, vs), f = Shader(0x8B30, fs);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glBindAttribLocation(p, 0, "pos");
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { printf("link FAIL\n"); return 0; }
    return p;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mdi_count_baseinstance_apple SKIP: no MTL device\n");
        return 0;
    }

    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glTexImage2D(0x0DE1, 0, 0x8058 /*RGBA8*/, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5) {
        printf("test_mdi_count_baseinstance_apple SKIP: FBO incomplete\n");
        return 0;
    }

    GLuint prog = Program(kVS, kFS);
    GLuint cprog = glCreateProgram();
    {
        GLuint cs = Shader(0x91B9, kCS);
        glAttachShader(cprog, cs);
        glLinkProgram(cprog);
        GLint ok = 0; glGetProgramiv(cprog, 0x8B82, &ok);
        if (!prog || !ok) { printf("test_mdi_count_baseinstance_apple FAIL: program\n"); return 1; }
    }

    GLuint vao = 0, vbo = 0, ebo = 0, cmdbuf = 0, cntbuf = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    const float verts[] = {-0.25f, -0.5f, 0.25f, -0.5f, 0.0f, 0.5f};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    const unsigned short idx[3] = {0, 1, 2};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glGenBuffers(1, &cmdbuf);
    glBindBuffer(0x90D2 /*SHADER_STORAGE_BUFFER*/, cmdbuf);
    glBufferData(0x90D2, 10 * 4, nullptr, 0x88E4);
    glGenBuffers(1, &cntbuf);
    glBindBuffer(0x90D2, cntbuf);
    glBufferData(0x90D2, 2 * 4, nullptr, 0x88E4);

    glViewport(0, 0, 64, 64);
    glDisable(0x0B71);
    glClearColor(0, 0, 0, 1);
    glClear(0x4000);

    glUseProgram(cprog);
    glBindBufferBase(0x90D2, 0, cmdbuf);
    glBindBufferBase(0x90D2, 1, cntbuf);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(0x00000400 | 0x00002000); // COMMAND | SHADER_STORAGE

    glUseProgram(prog);
    glBindVertexArray(vao);
    glBindBuffer(0x8893, ebo);
    glBindBuffer(0x8F3F /*DRAW_INDIRECT_BUFFER*/, cmdbuf);
    glBindBuffer(0x80EE /*PARAMETER_BUFFER*/, cntbuf);
    glMultiDrawElementsIndirectCount(0x0004, 0x1403, (const void*)0,
                                     (GLintptr)0, 2, 0);
    glFinish();

    unsigned char px[64 * 64 * 4] = {};
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    auto at = [&](int x, int y, int ch) { return px[(y * 64 + x) * 4 + ch]; };
    bool left = at(16, 32, 0) > 200 && at(16, 32, 1) < 50;
    bool right = at(48, 32, 0) > 200 && at(48, 32, 1) < 50;
    GLenum err = glGetError();
    if (err) printf("gl error 0x%x\n", err);
    if (!left || !right || err) {
        printf("test_mdi_count_baseinstance_apple FAIL: left=%d right=%d pxL=(%u,%u,%u) pxR=(%u,%u,%u) err=0x%x\n",
               (int)left, (int)right, at(16,32,0), at(16,32,1), at(16,32,2),
               at(48,32,0), at(48,32,1), at(48,32,2), err);
        return 1;
    }
    printf("test_mdi_count_baseinstance_apple PASS\n");
    return 0;
}
