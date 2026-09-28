// test_mc_gui_apple.cpp — Pixel test đường render VANILLA trên Apple backend.
// Dùng shader gui.vsh/gui.fsh THẬT từ corpus 26.1.2 (UBO DynamicTransforms +
// Projection, attribute không layout), bind attribute + UBO đúng như Blaze3D
// (GlProgram.link bind theo VertexFormat; setupUniforms bind block), vẽ tam giác
// fullscreen qua glDrawArrays, đọc glReadPixels. Đen màn hình trên máy mà test
// này đỏ => lỗi ở draw path (UBO/bind/descriptor), không phải ở shader.
// Thiếu GPU → SKIP trung thực.
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

// Inline #moj_import như Blaze3D (gui không có import nhưng giữ chung).
static std::string InlineImports(const std::string& src, const fs::path& incDir) {
    std::istringstream iss(src);
    std::string line, out;
    int lineNo = 0;
    while (std::getline(iss, line)) {
        ++lineNo;
        std::string t = line;
        size_t s = t.find_first_not_of(" \t\r");
        if (s != std::string::npos) t = t.substr(s);
        if (t.rfind("#moj_import", 0) == 0) {
            size_t a = t.find('<'), b = t.find('>');
            std::string ref = (a != std::string::npos && b != std::string::npos && b > a)
                                  ? t.substr(a + 1, b - a - 1)
                                  : "";
            size_t colon = ref.find(':');
            std::string name = (colon == std::string::npos) ? ref : ref.substr(colon + 1);
            out += "#line 1 0\n" + Read(incDir / name) + "\n#line " +
                   std::to_string(lineNo + 1) + " 1\n";
            continue;
        }
        out += line + "\n";
    }
    return out;
}

static void SetIdentity(float* m) {
    for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_gui_apple SKIP: no MTL device\n");
        return 0;
    }
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_gui_apple SKIP: thieu corpus %s\n", root.string().c_str());
        return 0;
    }
    fs::path core = root / "core", incDir = root / "include";

    auto target = ctx.device->makeRenderTarget(256, 256, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_gui_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    std::string vsSrc = InlineImports(Read(core / "gui.vsh"), incDir);
    std::string fsSrc = InlineImports(Read(core / "gui.fsh"), incDir);
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    const char* pv = vsSrc.c_str();
    const char* pf = fsSrc.c_str();
    glShaderSource(vs, 1, &pv, nullptr);
    glShaderSource(fs, 1, &pf, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    if (!ok) {
        GLchar log[1024] = {0};
        glGetShaderInfoLog(vs, 1024, nullptr, log);
        printf("test_mc_gui_apple FAIL: vs compile: %s\n", log);
        return 1;
    }
    glGetShaderiv(fs, 0x8B81, &ok);
    if (!ok) {
        GLchar log[1024] = {0};
        glGetShaderInfoLog(fs, 1024, nullptr, log);
        printf("test_mc_gui_apple FAIL: fs compile: %s\n", log);
        return 1;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    // Bind attribute như Blaze3D (format POSITION_COLOR: Position→0, Color→1).
    glBindAttribLocation(p, 0, "Position");
    glBindAttribLocation(p, 1, "Color");
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        GLchar log[1024] = {0};
        glGetProgramInfoLog(p, 1024, nullptr, log);
        printf("test_mc_gui_apple FAIL: link: %s\n", log);
        return 1;
    }
    if (glGetAttribLocation(p, "Position") != 0 || glGetAttribLocation(p, "Color") != 1) {
        printf("test_mc_gui_apple FAIL: attrib loc (pos=%d col=%d)\n",
               glGetAttribLocation(p, "Position"), glGetAttribLocation(p, "Color"));
        return 1;
    }
    // UBO: DynamicTransforms (std140 160B) + Projection (64B), identity.
    // Layout std140 == MSL natural cho 2 struct này (vec3 align 16 cả hai).
    GLuint uboDT, uboProj;
    glGenBuffers(1, &uboDT);
    glGenBuffers(1, &uboProj);
    uint8_t dt[160];
    memset(dt, 0, sizeof(dt));
    SetIdentity((float*)dt);                       // ModelViewMat @0
    float white[4] = {1, 1, 1, 1};
    memcpy(dt + 64, white, 16);                    // ColorModulator @64
    SetIdentity((float*)(dt + 96));                // TextureMat @96
    uint8_t pm[64];
    SetIdentity((float*)pm);                       // ProjMat @0
    glBindBuffer(0x8A11, uboDT);                   // GL_UNIFORM_BUFFER
    glBufferData(0x8A11, sizeof(dt), dt, 0x88E4);
    glBindBuffer(0x8A11, uboProj);
    glBufferData(0x8A11, sizeof(pm), pm, 0x88E4);
    GLuint bDT = glGetUniformBlockIndex(p, "DynamicTransforms");
    GLuint bProj = glGetUniformBlockIndex(p, "Projection");
    if (bDT == 0xFFFFFFFFu || bProj == 0xFFFFFFFFu) {
        printf("test_mc_gui_apple FAIL: block index (dt=%u proj=%u)\n", bDT, bProj);
        return 1;
    }
    glUniformBlockBinding(p, bDT, 0);
    glUniformBlockBinding(p, bProj, 1);
    glBindBufferBase(0x8A11, 0, uboDT);
    glBindBufferBase(0x8A11, 1, uboProj);

    // Tam giác fullscreen, màu đỏ.
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

    glUseProgram(p);
    glViewport(0, 0, 256, 256);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) {
        printf("test_mc_gui_apple FAIL: gl error after draw\n");
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
        printf("test_mc_gui_apple FAIL: draw not encoded (trace-only)\n");
        return 1;
    }
    unsigned char px[256 * 256 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 256, 256, 0x1908, 0x1401, px);
    unsigned char* mid = px + (128 * 256 + 128) * 4;
    unsigned char* corner = px + (4 * 256 + 4) * 4;
    printf("mid=(%u,%u,%u,%u) corner=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3],
           corner[0], corner[1], corner[2], corner[3]);
    // Tam giác fullscreen phủ hết → đỏ ở giữa; gui.fsh discard khi alpha==0
    // (alpha=1 ở đây nên giữ).
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("test_mc_gui_apple FAIL: center not red (UBO/bind/descriptor?)\n");
        return 1;
    }
    printf("test_mc_gui_apple PASS (vanilla gui + UBO + binds render exact)\n");
    return 0;
}
