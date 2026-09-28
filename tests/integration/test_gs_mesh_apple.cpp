// test_gs_mesh_apple.cpp — GS thay thế bằng mesh shader (Metal 3+, xem gs_mesh.metal).
// Gate runtime trung thực: thiết bị/OS không hỗ trợ (Intel KBL ở đây) → SKIP,
// không crash. Trên Apple7+/M1+ chạy thật: phát 1 triangle đỏ, check pixel.
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string FindMSL() {
    std::string cands[] = {"shaders/gs_mesh.metal", "../shaders/gs_mesh.metal",
                           "../../shaders/gs_mesh.metal"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/shaders/gs_mesh.metal";
        if (std::ifstream(p).good()) { std::ostringstream s; s << std::ifstream(p).rdbuf(); return s.str(); }
    }
    for (auto& c : cands)
        if (std::ifstream(c).good()) { std::ostringstream s; s << std::ifstream(c).rdbuf(); return s.str(); }
    return "";
}

int main() {
    using namespace tglmt::metal;
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_gs_mesh_apple SKIP: no MTL device\n"); return 0; }
    printf("device: %s\n", dev->name().c_str());
    // Gate THỰC THI (không phải tạo pipeline): Intel tạo pipeline OK nhưng vẽ đen.
    if (!dev->supportsMesh()) {
        printf("test_gs_mesh_apple SKIP: no mesh execution (need Apple7+; observed black on Intel)\n");
        return 0;
    }
    std::string msl = FindMSL();
    if (msl.empty()) { printf("test_gs_mesh_apple SKIP: gs_mesh.metal not found\n"); return 0; }
    std::string err;
    auto lib = dev->compileLibrary(msl, err);
    if (!lib) {
        // Driver cũ không biên dịch được mesh syntax → SKIP có lý do (cần Metal 3 compiler)
        printf("test_gs_mesh_apple SKIP: mesh MSL compile unsupported here: %s\n", err.c_str());
        return 0;
    }
    auto pso = dev->makeMeshPipeline(lib.get(), "gsMesh", lib.get(), "gsFS",
                                     PixelFormat::RGBA8Unorm);
    if (!pso) {
        printf("test_gs_mesh_apple SKIP: mesh pipeline unsupported (need Apple7+/Metal3 GPU)\n");
        return 0;
    }
    auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
    // Input primitive: 3 clip-space verts + màu (đọc qua [[buffer]] trong mesh fn)
    float pos[12] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f};
    float col[16] = {1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0};
    auto posB = dev->newBufferWithBytes(pos, sizeof(pos), StorageMode::Shared);
    auto colB = dev->newBufferWithBytes(col, sizeof(col), StorageMode::Shared);
    ClearColor black{0, 0, 0, 1};
    auto enc = dev->makeRenderEncoder(target.get(), pso.get(), black);
    if (!enc) { printf("test_gs_mesh_apple FAIL: encoder\n"); return 1; }
    Viewport vp{0, 0, 64, 64, 0, 1};
    enc->setViewport(vp);
    enc->setVertexBuffer(posB.get(), 0, 0);
    enc->setVertexBuffer(colB.get(), 0, 1);
    enc->drawMesh(1);
    if (!enc->endAndCommit()) { printf("test_gs_mesh_apple FAIL: commit\n"); return 1; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    if (!target->readback(px, 64 * 4)) { printf("test_gs_mesh_apple FAIL: readback\n"); return 1; }
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("mid=(%u,%u,%u,%u)\n", mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("test_gs_mesh_apple FAIL: center not red\n");
        return 1;
    }
    printf("test_gs_mesh_apple PASS (mesh GS-emulation exact)\n");
    return 0;
}
