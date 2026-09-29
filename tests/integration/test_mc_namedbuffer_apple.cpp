// test_mc_namedbuffer_apple.cpp — Hồi quy đen màn hình 26.1.2 (UBO luôn 0).
// Game update buffer per-frame qua DirectStateAccess: writeToBuffer →
// glNamedBufferSubData, map/unmap qua glMapNamedBufferRange/glUnmapNamedBuffer
// (javap GlCommandEncoder/GlBuffer). Các hàm Named cũ chỉ chép shadow CPU,
// không sync GPU → matrices 0 → clip toàn bộ → đen mà không error nào.
// Test: (1) VBO null + NamedSubData rồi vẽ; (2) MapNamedRange/Unmap rồi vẽ;
// (3) UBO gui identity qua NamedSubData rồi vẽ shader gui vanilla THẬT.
// Thiếu GPU → SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace tglmt;
using namespace tglmt::gl;

#ifndef TGLMT_CORPUS_DIR
#define TGLMT_CORPUS_DIR "tests/corpus/mc-26.1.2-shaders"
#endif

static std::string Read(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "layout(location = 0) uniform mat4 uMVP;\n"
    "void main() {\n"
    "  vCol = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "  gl_Position = uMVP * vec4(pos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_namedbuffer_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static GLuint MakeTriProgram() {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) printf("test_mc_namedbuffer_apple FAIL: link tri\n");
    return ok ? p : 0;
}

static void SetupTriVAO(GLuint vbo) {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(0x8892, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
}

static bool ReadCenterRed() {
    unsigned char px[256 * 256 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    unsigned char* mid = px + (128 * 256 + 128) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    return mid[0] > 200 && mid[1] < 50 && mid[2] < 50;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_namedbuffer_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_namedbuffer_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);
    float ident[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

    // Case 1: VBO null + glNamedBufferSubData (cơ chế update UBO của game).
    {
        GLuint prog = MakeTriProgram();
        Check(prog != 0, "tri program");
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, 3 * 2 * 4, nullptr, 0x88E4); // zeros
        float tri[6] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};
        glNamedBufferSubData(vbo, 0, sizeof(tri), tri);
        SetupTriVAO(vbo);
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, 0, ident);
        glViewport(0, 0, 256, 256);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "gl error case1");
        Check(ReadCenterRed(), "NamedSubData VBO ve do");
    }
    // Case 2: glMapNamedBufferRange + memcpy + glUnmapNamedBuffer.
    {
        GLuint prog = MakeTriProgram();
        Check(prog != 0, "tri program 2");
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, 3 * 2 * 4, nullptr, 0x88E4);
        float tri[6] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};
        void* m = glMapNamedBufferRange(vbo, 0, sizeof(tri), 0x0001);
        Check(m != nullptr, "map named range");
        if (m) memcpy(m, tri, sizeof(tri));
        Check(glUnmapNamedBuffer(vbo) != 0, "unmap named");
        SetupTriVAO(vbo);
        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, 0, ident);
        glViewport(0, 0, 256, 256);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "gl error case2");
        Check(ReadCenterRed(), "MapNamedRange VBO ve do");
    }
    // Case 3: UBO shader gui vanilla, matrices qua NamedSubData (đúng game).
    {
        fs::path root = TGLMT_CORPUS_DIR;
        if (!fs::exists(root)) {
            printf("test_mc_namedbuffer_apple SKIP: thieu corpus\n");
            return gFails ? 1 : 0;
        }
        GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
        std::string vsSrc = Read(root / "core" / "gui.vsh");
        std::string fsSrc = Read(root / "core" / "gui.fsh");
        const char* pv = vsSrc.c_str();
        const char* pf = fsSrc.c_str();
        glShaderSource(vs, 1, &pv, nullptr);
        glShaderSource(fs, 1, &pf, nullptr);
        glCompileShader(vs);
        glCompileShader(fs);
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glBindAttribLocation(prog, 0, "Position");
        glBindAttribLocation(prog, 1, "Color");
        glLinkProgram(prog);
        GLint ok = 0;
        glGetProgramiv(prog, 0x8B82, &ok);
        Check(ok != 0, "link gui");
        GLuint uboDT, uboProj;
        glGenBuffers(1, &uboDT);
        glGenBuffers(1, &uboProj);
        glBindBuffer(0x8A11, uboDT);
        glBufferData(0x8A11, 160, nullptr, 0x88E4);
        glBindBuffer(0x8A11, uboProj);
        glBufferData(0x8A11, 64, nullptr, 0x88E4);
        // Ghi matrices qua Named path (game writeToBuffer).
        uint8_t dt[160];
        memset(dt, 0, sizeof(dt));
        for (int i = 0; i < 16; ++i) ((float*)dt)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        float white[4] = {1, 1, 1, 1};
        memcpy(dt + 64, white, 16);
        for (int i = 0; i < 16; ++i) ((float*)(dt + 96))[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        uint8_t pm[64];
        for (int i = 0; i < 16; ++i) ((float*)pm)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        glNamedBufferSubData(uboDT, 0, sizeof(dt), dt);
        glNamedBufferSubData(uboProj, 0, sizeof(pm), pm);
        glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, "DynamicTransforms"), 0);
        glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, "Projection"), 1);
        glBindBufferBase(0x8A11, 0, uboDT);
        glBindBufferBase(0x8A11, 1, uboProj);
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float verts[3 * 7] = {-1, -1, 0, 1, 0, 0, 1, //
                              3,  -1, 0, 1, 0, 0, 1, //
                              -1, 3,  0, 1, 0, 0, 1};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, 0x1406, 0, 28, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, 0x1406, 0, 28, (void*)12);
        glUseProgram(prog);
        glViewport(0, 0, 256, 256);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "gl error case3");
        Check(ReadCenterRed(), "NamedSubData UBO ve do");
    }
    if (gFails) return 1;
    printf("test_mc_namedbuffer_apple PASS (Named write path sync GPU)\n");
    return 0;
}
