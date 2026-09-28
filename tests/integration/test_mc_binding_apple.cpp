// test_mc_binding_apple.cpp — Hồi quy đen màn hình 26.1.2 trên máy.
// Game (VertexArrayCache$Separate, kiểm chứng bằng javap client.jar 26.1.2)
// setup VAO bằng Separate path theo thứ tự: Format(rel) → Binding →
// BindVertexBuffer. glBindVertexBuffer cũ ghi đè relativeOffset bằng binding
// offset → mọi attribute đọc từ đầu buffer → đỉnh rác → đen màn hình (không
// error, không crash nên test cũ không bắt được).
// Test có 2 lớp (lớp state chạy mọi backend kể cả null/CI, lớp pixel cần GPU):
//  1. State: sau chuỗi Format/Binding/BindVertexBuffer, effective offset phải
//     = binding.offset + relativeOffset, relativeOffset giữ nguyên.
//  2. Pixel (Apple backend): shader gui vanilla THẬT vẽ tam giác đỏ qua
//     Separate path, offset 0 (như game) và offset 16 (khóa phép cộng địa chỉ).
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

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_binding_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

// Chuỗi setup đúng thứ tự game, trả VAO id. padBytes byte rác đầu buffer.
static GLuint SetupSeparateVAO(GLintptr bindingOffset, int padBytes, GLuint& outVbo) {
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    uint8_t raw[16 + 3 * 28];
    memset(raw, 0x7F, sizeof(raw));
    float tri[3 * 7] = {-1, -1, 0, 1, 0, 0, 1, //
                        3,  -1, 0, 1, 0, 0, 1, //
                        -1, 3,  0, 1, 0, 0, 1};
    memcpy(raw + padBytes, tri, sizeof(tri));
    glBufferData(0x8892, sizeof(raw), raw, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribFormat(0, 3, 0x1406, 0, 0);
    glVertexAttribFormat(1, 4, 0x1406, 0, 12);
    glVertexAttribBinding(0, 0);
    glVertexAttribBinding(1, 0);
    glBindVertexBuffer(0, vbo, bindingOffset, 28);
    outVbo = vbo;
    return vao;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    bool hasGPU = !ctx.device->isNull();

    // ---- Lớp 1: state (mọi backend) ----
    {
        GLuint vbo = 0;
        GLuint vao = SetupSeparateVAO(0, 0, vbo);
        auto& V = ctx.vaos[vao];
        Check(V.attribs[0].relativeOffset == 0, "rel0 giu");
        Check(V.attribs[1].relativeOffset == 12, "rel1 giu (khong bi BindVertexBuffer xoa)");
        Check(V.attribs[0].offset == 0, "eff0 = 0 + 0");
        Check(V.attribs[1].offset == 12, "eff1 = 0 + 12");
        Check(V.bindings[0].offset == 0 && V.bindings[0].buffer == vbo, "binding0");
        if (gFails) return 1;
        printf("test_mc_binding_apple state OK (backend %s)\n", hasGPU ? "apple" : "null");
    }
    // Binding offset != 0 (state thuần, không cần GPU).
    {
        GLuint vbo = 0;
        GLuint vao = SetupSeparateVAO(16, 16, vbo);
        auto& V = ctx.vaos[vao];
        Check(V.attribs[0].relativeOffset == 0, "rel0 giu (off16)");
        Check(V.attribs[1].relativeOffset == 12, "rel1 giu (off16)");
        Check(V.attribs[0].offset == 16, "eff0 = 16 + 0");
        Check(V.attribs[1].offset == 28, "eff1 = 16 + 12");
        if (gFails) return 1;
    }

    if (!hasGPU) {
        printf("test_mc_binding_apple SKIP pixel (no MTL device)\n");
        return 0;
    }

    // ---- Lớp 2: pixel (Apple backend, shader gui vanilla thật) ----
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_binding_apple SKIP: thieu corpus %s\n", root.string().c_str());
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_binding_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

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
    // UBO identity (std140 == MSL natural cho 2 struct này).
    GLuint uboDT, uboProj;
    glGenBuffers(1, &uboDT);
    glGenBuffers(1, &uboProj);
    uint8_t dt[160];
    memset(dt, 0, sizeof(dt));
    for (int i = 0; i < 16; ++i) ((float*)dt)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    float white[4] = {1, 1, 1, 1};
    memcpy(dt + 64, white, 16);
    for (int i = 0; i < 16; ++i) ((float*)(dt + 96))[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    uint8_t pm[64];
    for (int i = 0; i < 16; ++i) ((float*)pm)[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    glBindBuffer(0x8A11, uboDT);
    glBufferData(0x8A11, sizeof(dt), dt, 0x88E4);
    glBindBuffer(0x8A11, uboProj);
    glBufferData(0x8A11, sizeof(pm), pm, 0x88E4);
    glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, "DynamicTransforms"), 0);
    glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, "Projection"), 1);
    glBindBufferBase(0x8A11, 0, uboDT);
    glBindBufferBase(0x8A11, 1, uboProj);
    if (gFails) return 1;

    auto drawCheck = [&](GLintptr bindingOffset, int pad, const char* tag) {
        GLuint vbo = 0;
        GLuint vao = SetupSeparateVAO(bindingOffset, pad, vbo);
        (void)vao;
        glUseProgram(prog);
        glViewport(0, 0, 256, 256);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "gl error sau draw");
        unsigned char px[256 * 256 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
        unsigned char* mid = px + (128 * 256 + 128) * 4;
        printf("[%s] mid=(%u,%u,%u,%u)\n", tag, mid[0], mid[1], mid[2], mid[3]);
        Check(mid[0] > 200 && mid[1] < 50 && mid[2] < 50, "center do");
    };
    drawCheck(0, 0, "offset0");
    drawCheck(16, 16, "offset16");
    if (gFails) return 1;
    if (ctx.appleStats.drawsEncoded == 0) {
        printf("test_mc_binding_apple FAIL: draw not encoded\n");
        return 1;
    }
    printf("test_mc_binding_apple PASS (Separate path state + pixels exact)\n");
    return 0;
}
