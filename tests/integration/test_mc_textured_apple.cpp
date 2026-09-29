// test_mc_textured_apple.cpp — Mô phỏng 1 draw GUI textured của menu MC.
// position_tex.vsh/fsh THẬT (Sampler0 + UBO), VAO Separate path, VBO+EBO indexed,
// texture RGBA upload qua glTexImage2D, blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA như GUI.
// Đen màn hình trên máy mà test này đỏ => lỗi ở indexed/texture/blend.
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
static std::string InlineImports(const std::string& src, const fs::path& incDir) {
    std::istringstream iss(src);
    std::string line, out;
    int n = 0;
    while (std::getline(iss, line)) {
        ++n;
        std::string t = line;
        size_t s = t.find_first_not_of(" \t\r");
        if (s != std::string::npos) t = t.substr(s);
        if (t.rfind("#moj_import", 0) == 0) {
            size_t a = t.find('<'), b = t.find('>');
            std::string ref =
                (a != std::string::npos && b != std::string::npos && b > a)
                    ? t.substr(a + 1, b - a - 1)
                    : "";
            size_t colon = ref.find(':');
            std::string name = (colon == std::string::npos) ? ref : ref.substr(colon + 1);
            out += "#line 1 0\n" + Read(incDir / name) + "\n#line " + std::to_string(n + 1) +
                   " 1\n";
            continue;
        }
        out += line + "\n";
    }
    return out;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_textured_apple SKIP: no MTL device\n");
        return 0;
    }
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_textured_apple SKIP: thieu corpus\n");
        return 0;
    }
    // Target CÓ depth như máy thật (CreateWindow depth=true) + clear cả
    // COLOR|DEPTH như game boot. Từng không có test nào bao phủ combo này.
    auto target =
        ctx.device->makeRenderTargetWithDepth(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_textured_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    std::string vsSrc = InlineImports(Read(root / "core" / "position_tex.vsh"), root / "include");
    std::string fsSrc = InlineImports(Read(root / "core" / "position_tex.fsh"), root / "include");
    const char* pv = vsSrc.c_str();
    const char* pf = fsSrc.c_str();
    glShaderSource(vs, 1, &pv, nullptr);
    glShaderSource(fs, 1, &pf, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    if (!ok) {
        printf("test_mc_textured_apple FAIL: vs\n");
        return 1;
    }
    glGetShaderiv(fs, 0x8B81, &ok);
    if (!ok) {
        printf("test_mc_textured_apple FAIL: fs\n");
        return 1;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "Position");
    glBindAttribLocation(prog, 1, "UV0");
    glLinkProgram(prog);
    glGetProgramiv(prog, 0x8B82, &ok);
    if (!ok) {
        GLchar log[1024] = {0};
        glGetProgramInfoLog(prog, 1024, nullptr, log);
        printf("test_mc_textured_apple FAIL: link: %s\n", log);
        return 1;
    }
    // UBO identity.
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

    // Texture 4x4: 4 góc màu (đỏ/xanh lá/xanh dương/trắng), alpha 255.
    // Hàng 0-1 (dưới GL): đỏ,đỏ,xanh lá,xanh lá; hàng 2-3: xanh dương ×2, trắng ×2.
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    unsigned char img[4 * 4 * 4];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            unsigned char* p = img + (y * 4 + x) * 4;
            p[0] = ((x < 2) == (y < 2)) ? 255 : 0;
            p[1] = (x >= 2) ? 255 : 0;
            p[2] = (y >= 2) ? 255 : 0;
            p[3] = 255;
        }
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, img);
    glTexParameteri(0x0DE1, 0x2801, 0x2600); // MIN NEAREST
    glTexParameteri(0x0DE1, 0x2800, 0x2600); // MAG NEAREST
    glTexParameteri(0x0DE1, 0x2802, 0x812F); // WRAP_S CLAMP
    glTexParameteri(0x0DE1, 0x2803, 0x812F); // WRAP_T CLAMP

    // Quad fullscreen indexed (2 tam giác), pos + uv interleaved.
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float quad[4 * 5] = {-1, -1, 0, 0, 0, //
                         1,  -1, 0, 1, 0, //
                         1,  1,  0, 1, 1, //
                         -1, 1,  0, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo); // ELEMENT_ARRAY_BUFFER
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribFormat(0, 3, 0x1406, 0, 0);
    glVertexAttribFormat(1, 2, 0x1406, 0, 12);
    glVertexAttribBinding(0, 0);
    glVertexAttribBinding(1, 0);
    glBindVertexBuffer(0, vbo, 0, 20);

    glUseProgram(prog);
    GLint locS = glGetUniformLocation(prog, "Sampler0");
    glUniform1i(locS, 0);
    // Blend như GUI: SRC_ALPHA, ONE_MINUS_SRC_ALPHA.
    glEnable(0x0BE2);
    glBlendFuncSeparate(0x0302, 0x0303, 0x0001, 0x0303);
    glViewport(0, 0, 256, 256);
    glClearColor(0, 0, 1, 1);
    glClearDepthf(1.0f);
    glClear(0x00004000 | 0x00000100); // COLOR|DEPTH như game
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    if (glGetError() != 0) {
        printf("test_mc_textured_apple FAIL: gl error after draw\n");
        return 1;
    }
    printf("appleStats drawsAttempted=%llu drawsEncoded=%llu noProgram=%llu noTarget=%llu "
           "noPipeline=%llu miscFail=%llu\n",
           (unsigned long long)ctx.appleStats.drawsAttempted,
           (unsigned long long)ctx.appleStats.drawsEncoded,
           (unsigned long long)ctx.appleStats.noProgram,
           (unsigned long long)ctx.appleStats.noTarget,
           (unsigned long long)ctx.appleStats.noPipeline,
           (unsigned long long)ctx.appleStats.miscFail);
    if (ctx.appleStats.drawsEncoded == 0) {
        printf("test_mc_textured_apple FAIL: draw not encoded\n");
        return 1;
    }
    unsigned char px[256 * 256 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    auto at = [&](int x, int y) -> unsigned char* { return px + (y * 256 + x) * 4; };
    // Lưới chẩn đoán: in 4x4 mẫu (R/G/B dominant) để thấy mẫu không gian.
    for (int gy = 3; gy >= 0; --gy) {
        std::string row;
        for (int gx = 0; gx < 4; ++gx) {
            unsigned char* q = at(32 + gx * 64, 32 + gy * 64);
            char c = (q[0] > 200 && q[1] < 50 && q[2] < 50)    ? 'R'
                     : (q[1] > 200 && q[0] < 50 && q[2] < 50)  ? 'G'
                     : (q[2] > 200 && q[0] < 50 && q[1] < 50)  ? 'B'
                     : (q[0] > 200 && q[1] > 200 && q[2] > 200) ? 'W'
                     : (q[2] > 200)                             ? 'b'
                                                               : '.';
            row += c;
        }
        printf("grid y=%d: %s\n", gy, row.c_str());
    }
    // UV mapping: v=0 đáy GL. Quad uv (0,0)-(1,1); readback y=0 là đáy.
    // Đáy-trái uv≈(0.25,0.25) → img hàng 0-1 (đỏ); đỉnh-trái uv≈(0.25,0.75) → hàng 2-3 cột 0-1 (xanh dương).
    unsigned char* bl = at(64, 64);
    unsigned char* tl = at(64, 192);
    unsigned char* tr = at(192, 192);
    printf("bl=(%u,%u,%u) tl=(%u,%u,%u) tr=(%u,%u,%u)\n", bl[0], bl[1], bl[2], tl[0],
           tl[1], tl[2], tr[0], tr[1], tr[2]);
    bool red = bl[0] > 200 && bl[1] < 50 && bl[2] < 50;
    bool blue = tl[2] > 200 && tl[0] < 50 && tl[1] < 50;
    bool whitePx = tr[0] > 200 && tr[1] > 200 && tr[2] > 200;
    if (!(red && blue && whitePx)) {
        printf("test_mc_textured_apple FAIL: texels wrong (texture/blend/indexed?)\n");
        return 1;
    }
    printf("test_mc_textured_apple PASS (indexed+texture+blend exact)\n");
    return 0;
}
