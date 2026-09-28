// test_spirv_chain_apple.cpp — Nối chuỗi GLSL→SPIR-V→MSL→GPU vào test (M4-runtime).
// Luồng thật, không mock:
//   shaders/glsl/tri.vert/.frag --glslangValidator--> .spv --spirv-cross--> MSL
//   --> MTLDevice.compileLibrary (vs+fs riêng) --> pipeline --> render --> pixel check.
// SKIP trung thực (exit 0 + lý do) khi thiếu tool / thiếu GPU / thiếu file shader.
// FAIL khi tool có mà transpile/compile/render sai.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

static std::string FindFile(const char* rel) {
    const std::string cands[] = {
        std::string("shaders/glsl/") + rel,
        std::string("../shaders/glsl/") + rel,
        std::string("../../shaders/glsl/") + rel,
    };
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/glsl/" + rel;
        if (std::ifstream(p).good()) return p;
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) return c;
    return "";
}

static bool HaveTool(const char* t) {
    std::string cmd = std::string("command -v ") + t + " >/dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

static bool Run(const std::string& cmd, std::string& out) {
    out.clear();
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
    if (!p) return false;
    char buf[1024];
    while (fgets(buf, sizeof(buf), p)) out += buf;
    return pclose(p) == 0;
}

static std::string ReadFile(const std::string& path) {
    std::ifstream f(path);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}

int main() {
    using namespace tglmt::metal;
    std::string vs = FindFile("tri.vert"), fs = FindFile("tri.frag");
    if (vs.empty() || fs.empty()) {
        printf("test_spirv_chain_apple SKIP: shaders/glsl/tri.* not found\n");
        return 0;
    }
    if (!HaveTool("glslangValidator") || !HaveTool("spirv-cross")) {
        printf("test_spirv_chain_apple SKIP: missing glslangValidator/spirv-cross\n");
        return 0;
    }
    std::string tmp = std::string("/tmp/tglmt-spvchain-") + std::to_string(getpid());
    std::string log;
    // 1. GLSL -> SPIR-V
    if (!Run("mkdir -p " + tmp, log) ||
        !Run("glslangValidator -V " + vs + " -o " + tmp + "/tri.vert.spv", log)) {
        printf("test_spirv_chain_apple FAIL: glslang vert:\n%s\n", log.c_str());
        return 1;
    }
    if (!Run("glslangValidator -V " + fs + " -o " + tmp + "/tri.frag.spv", log)) {
        printf("test_spirv_chain_apple FAIL: glslang frag:\n%s\n", log.c_str());
        return 1;
    }
    // 2. SPIR-V -> MSL
    if (!Run("spirv-cross --msl " + tmp + "/tri.vert.spv --output " + tmp + "/tri.msl", log)) {
        printf("test_spirv_chain_apple FAIL: spirv-cross vert:\n%s\n", log.c_str());
        return 1;
    }
    if (!Run("spirv-cross --msl " + tmp + "/tri.frag.spv --output " + tmp + "/tri.frag.msl", log)) {
        printf("test_spirv_chain_apple FAIL: spirv-cross frag:\n%s\n", log.c_str());
        return 1;
    }
    std::string vsMSL = ReadFile(tmp + "/tri.msl");
    std::string fsMSL = ReadFile(tmp + "/tri.frag.msl");
    Run("rm -rf " + tmp, log); // dọn ngay khi đã có MSL trong RAM (mọi đường FAIL/PASS sau đều sạch)
    if (vsMSL.find("vertex") == std::string::npos || fsMSL.find("fragment") == std::string::npos) {
        printf("test_spirv_chain_apple FAIL: MSL missing vertex/fragment entry\n");
        return 1;
    }
    printf("transpile OK (vs %zu bytes, fs %zu bytes)\n", vsMSL.size(), fsMSL.size());

    // 3. MSL -> MTLDevice (cần GPU thật)
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) {
        printf("test_spirv_chain_apple SKIP (transpile OK): no MTL device\n");
        return 0;
    }
    printf("device: %s\n", dev->name().c_str());
    std::string err;
    auto vsLib = dev->compileLibrary(vsMSL, err);
    if (!vsLib) { printf("test_spirv_chain_apple FAIL: vs compile: %s\n", err.c_str()); return 1; }
    auto fsLib = dev->compileLibrary(fsMSL, err);
    if (!fsLib) { printf("test_spirv_chain_apple FAIL: fs compile: %s\n", err.c_str()); return 1; }
    // spirv-cross đặt entry là main0 cho cả 2 stage (đã kiểm chứng trong build/spirv-chain/)
    auto pso = dev->makeRenderPipeline(vsLib.get(), "main0", fsLib.get(), "main0",
                                       PixelFormat::RGBA8Unorm);
    if (!pso) { printf("test_spirv_chain_apple FAIL: pipeline nil\n"); return 1; }

    // 4. Render tam giác đỏ, check pixel (layout spirv: pos attr0 float2, col attr1 float4)
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_spirv_chain_apple FAIL: target nil\n"); return 1; }
    struct V { float x, y, r, g, b, a; };
    V verts[3] = {{-0.5f,-0.5f, 1,0,0,1}, {0.5f,-0.5f, 1,0,0,1}, {0.0f,0.5f, 1,0,0,1}};
    auto vb = dev->newBufferWithBytes(verts, sizeof(verts), StorageMode::Shared);
    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    if (!enc) { printf("test_spirv_chain_apple FAIL: encoder nil\n"); return 1; }
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(vb.get(), 0, 0);
    enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
    if (!enc->endAndCommit()) { printf("test_spirv_chain_apple FAIL: commit\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_spirv_chain_apple FAIL: readback\n"); return 1; }
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("test_spirv_chain_apple FAIL: center pixel not red\n");
        return 1;
    }
    printf("test_spirv_chain_apple PASS (GLSL->SPV->MSL->GPU->pixels)\n");
    return 0;
}
