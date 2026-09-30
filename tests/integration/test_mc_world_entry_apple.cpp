// test_mc_world_entry_apple.cpp — tái hiện sequence vào world của MC 26.1.2:
// chunk VBO streaming (10 SubData kề) → draw vào FBO color (NULL như MC) →
// PBO screenshot full (copyTobuffer) → overlay multiply xám D2 (đúng tex#15173
// trên máy: blend ZERO/ONE_MINUS_SRC_COLOR). Đỏ ở bất kỳ bước nào = bug thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

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

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec4 col;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "void main() {\n"
    "  vCol = col;\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_mc_world_entry SKIP: no MTL device\n"); return 0; }

    const int W = 256, H = 128;
    // Default target cho overlay; FBO riêng cho chunk (đúng MC: world vào FBO).
    auto defTarget = ctx.device->makeRenderTarget(W, H, metal::PixelFormat::RGBA8Unorm);
    if (!defTarget) { printf("FAIL deftarget\n"); return 1; }
    ctx.device->setDefaultRenderTarget(defTarget);

    // ---- Chunk FBO color, NULL data (y MC texture 41) ----
    GLuint fboTex = 0;
    glGenTextures(1, &fboTex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, fboTex);
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, nullptr);
    if (glGetError() != 0) { printf("FAIL fbo teximage\n"); return 1; }
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fboTex, 0);
    if (glGetError() != 0) { printf("FAIL fbo attach\n"); return 1; }

    // ---- Chunk program + mesh streaming (2 quads trong 1 VBO) ----
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    // 2 quads TRIANGLES (12 đỉnh): trái đỏ (x -1..0), phải xanh lá (x 0..1).
    float mesh[12 * 6];
    float LV[6][2] = {{-1, -1}, {0, -1}, {0, 1}, {-1, -1}, {0, 1}, {-1, 1}};
    float RV[6][2] = {{0, -1}, {1, -1}, {1, 1}, {0, -1}, {1, 1}, {0, 1}};
    for (int i = 0; i < 6; ++i) {
        mesh[i * 6] = LV[i][0];
        mesh[i * 6 + 1] = LV[i][1];
        mesh[i * 6 + 2] = 1;
        mesh[i * 6 + 3] = 0;
        mesh[i * 6 + 4] = 0;
        mesh[i * 6 + 5] = 1;
        mesh[(6 + i) * 6] = RV[i][0];
        mesh[(6 + i) * 6 + 1] = RV[i][1];
        mesh[(6 + i) * 6 + 2] = 0;
        mesh[(6 + i) * 6 + 3] = 1;
        mesh[(6 + i) * 6 + 4] = 0;
        mesh[(6 + i) * 6 + 5] = 1;
    }
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    std::vector<uint8_t> zeros(sizeof(mesh), 0);
    glBufferData(0x8892, (GLsizeiptr)sizeof(mesh), zeros.data(), 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, 0x1406, 0, 24, (void*)8);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, 0x8B82, &ok);
    if (!ok) { printf("FAIL link chunk\n"); return 1; }
    glUseProgram(prog);

    // Streaming kiểu chunk: 6 SubData kề nhau 48B đổ đầy VBO (coalesce).
    for (int i = 0; i < 6; ++i) {
        glNamedBufferSubData(vbo, (GLintptr)(i * 48), 48,
                             (const uint8_t*)mesh + i * 48);
    }
    if (glGetError() != 0) { printf("FAIL stream\n"); return 1; }

    glViewport(0, 0, W, H);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    // 2 draws cùng VBO khác first (chunk mesh style, batching phải gộp encoder).
    uint64_t enc0 = ctx.appleStats.encodersCreated;
    glDrawArrays(0x0004, 0, 6); // quad trái đỏ
    if (glGetError() != 0) { printf("FAIL drawL\n"); return 1; }
    glDrawArrays(0x0004, 6, 6); // quad phải xanh lá
    if (glGetError() != 0) { printf("FAIL drawR\n"); return 1; }
    uint64_t encNew = ctx.appleStats.encodersCreated - enc0;
    printf("chunk batch: encoders=%llu (want 1)\n", (unsigned long long)encNew);
    if (encNew != 1) { printf("FAIL batch\n"); return 1; }

    // ---- PBO screenshot full FBO (copyTobuffer) ----
    GLuint pbo = 0;
    glGenBuffers(1, &pbo);
    glBindBuffer(0x88EB, pbo);
    size_t need = (size_t)W * H * 4;
    std::vector<uint8_t> zero(need, 0);
    glBufferData(0x88EB, (GLsizeiptr)need, zero.data(), 0x88E8);
    glGetTextureSubImage(fboTex, 0, 0, 0, 0, W, H, 0, 0x1908, 0x1401, (GLsizei)need,
                         (void*)0);
    GLenum e = glGetError();
    if (e != 0) {
        printf("FAIL world screenshot path: GL error 0x%x\n", e);
        return 1;
    }
    // PBO phải chứa trái đỏ / phải xanh lá (chunk pixels lên đúng).
    {
        auto it = ctx.buffers.find(pbo);
        if (it == ctx.buffers.end() || it->second.data.size() < need) {
            printf("FAIL pbo size\n");
            return 1;
        }
        const uint8_t* d = it->second.data.data();
        const uint8_t* L = d + ((size_t)(H / 2) * W + W / 4) * 4;
        const uint8_t* R = d + ((size_t)(H / 2) * W + 3 * W / 4) * 4;
        printf("shot L=(%u,%u,%u) R=(%u,%u,%u)\n", L[0], L[1], L[2], R[0], R[1], R[2]);
        if (!(L[0] > 200 && L[1] < 50) || !(R[1] > 200 && R[0] < 50)) {
            printf("FAIL chunk pixels in screenshot\n");
            return 1;
        }
    }
    glBindBuffer(0x88EB, 0);

    // ---- Overlay multiply xám D2 lên default target (đúng tex#15173) ----
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_world_entry SKIP: thieu corpus\n");
        return 0;
    }
    GLuint vs2 = glCreateShader(0x8B31), fs2 = glCreateShader(0x8B30);
    std::string vsSrc = InlineImports(Read(root / "core" / "position_tex.vsh"), root / "include");
    std::string fsSrc = InlineImports(Read(root / "core" / "position_tex.fsh"), root / "include");
    const char* pv = vsSrc.c_str();
    const char* pf = fsSrc.c_str();
    glShaderSource(vs2, 1, &pv, nullptr);
    glShaderSource(fs2, 1, &pf, nullptr);
    glCompileShader(vs2);
    glCompileShader(fs2);
    glGetShaderiv(vs2, 0x8B81, &ok);
    if (!ok) { printf("FAIL vs2\n"); return 1; }
    glGetShaderiv(fs2, 0x8B81, &ok);
    if (!ok) { printf("FAIL fs2\n"); return 1; }
    GLuint prog2 = glCreateProgram();
    glAttachShader(prog2, vs2);
    glAttachShader(prog2, fs2);
    glBindAttribLocation(prog2, 0, "Position");
    glBindAttribLocation(prog2, 1, "UV0");
    glLinkProgram(prog2);
    glGetProgramiv(prog2, 0x8B82, &ok);
    if (!ok) { printf("FAIL link overlay\n"); return 1; }
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
    glUniformBlockBinding(prog2, glGetUniformBlockIndex(prog2, "DynamicTransforms"), 0);
    glUniformBlockBinding(prog2, glGetUniformBlockIndex(prog2, "Projection"), 1);
    glBindBufferBase(0x8A11, 0, uboDT);
    glBindBufferBase(0x8A11, 1, uboProj);

    // Texture xám D2D2D2FF 64x64 (đúng tex#15173 trên máy).
    GLuint grayTex = 0;
    glGenTextures(1, &grayTex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, grayTex);
    std::vector<uint8_t> gray(64 * 64 * 4);
    for (int i = 0; i < 64 * 64; ++i) {
        gray[i * 4] = 0xD2; gray[i * 4 + 1] = 0xD2; gray[i * 4 + 2] = 0xD2; gray[i * 4 + 3] = 0xFF;
    }
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, gray.data());
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);

    GLuint vao2 = 0, vbo2 = 0, ebo2 = 0;
    glGenVertexArrays(1, &vao2);
    glBindVertexArray(vao2);
    float quad[4 * 5] = {-1, -1, 0, 0, 0, 1, -1, 0, 1, 0, 1, 1, 0, 1, 1, -1, 1, 0, 0, 1};
    glGenBuffers(1, &vbo2);
    glBindBuffer(0x8892, vbo2);
    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo2);
    glBindBuffer(0x8893, ebo2);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribFormat(0, 3, 0x1406, 0, 0);
    glVertexAttribFormat(1, 2, 0x1406, 0, 12);
    glVertexAttribBinding(0, 0);
    glVertexAttribBinding(1, 0);
    glBindVertexBuffer(0, vbo2, 0, 20);

    // Về default FB, nền xanh dương, overlay multiply.
    glBindFramebuffer(0x8CA9, 0);
    glUseProgram(prog2);
    GLint locS = glGetUniformLocation(prog2, "Sampler0");
    glUniform1i(locS, 0);
    glEnable(0x0BE2);
    glBlendFuncSeparate(0x0000 /*ZERO*/, 0x0301 /*ONE_MINUS_SRC_COLOR*/, 0x0001 /*ONE*/,
                        0x0303 /*ONE_MINUS_SRC_ALPHA*/);
    glViewport(0, 0, W, H);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    if (glGetError() != 0) { printf("FAIL overlay draw\n"); return 1; }
    unsigned char px[256 * 128 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, W, H, 0x1908, 0x1401, px);
    unsigned char* mid = px + ((H / 2) * W + W / 2) * 4;
    // Multiply: out = src*0 + dst*(1-src). dst=blue(0,0,255), src gray 0xD2:
    // B = 255*(1-210/255) = 255*45/255 = 45. R=G=0.
    printf("overlay mid=(%u,%u,%u) want ~(0,0,45)\n", mid[0], mid[1], mid[2]);
    if (std::abs((int)mid[2] - 45) > 6 || mid[0] > 6 || mid[1] > 6) {
        printf("FAIL multiply blend formula\n");
        return 1;
    }
    printf("test_mc_world_entry PASS (stream+FBO+PBOshot+multiply exact)\n");
    return 0;
}
