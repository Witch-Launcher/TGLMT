// test_converter_proof.cpp — CHỨNG MINH converter đúng hành vi:
// fish.vert/fish.frag qua 2 đường (TGLMT converter vs glslang+spirv-cross),
// render cùng geometry/uniforms trên GPU thật, so pixel (tolerance ±5).
// FAIL nếu lệch (converter sai hoặc giả định layout spirv sai — đều phải điều tra).
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static std::string ShaderDir() {
    const char* cands[] = {"apps/aquarium/shaders", "../apps/aquarium/shaders",
                           "../../apps/aquarium/shaders"};
    if (const char* root = std::getenv("TGLMT_ROOT")) {
        std::string p = std::string(root) + "/apps/aquarium/shaders";
        if (std::ifstream(p + "/fish.vert").good()) return p;
    }
    for (auto c : cands)
        if (std::ifstream(std::string(c) + "/fish.vert").good()) return c;
    return "";
}
static std::string Read(const std::string& p) {
    std::ifstream f(p);
    std::ostringstream s;
    s << f.rdbuf();
    return s.str();
}
static bool Run(const std::string& cmd, std::string& out) {
    out.clear();
    FILE* p = popen((cmd + " 2>&1").c_str(), "r");
    if (!p) return false;
    char buf[1024];
    while (fgets(buf, sizeof(buf), p)) out += buf;
    return pclose(p) == 0;
}
// Tìm entry `vertex|fragment ... NAME(` trong MSL (cho spirv-cross main0).
static std::string FindEntry(const std::string& msl, const std::string& kind) {
    size_t p = 0;
    while ((p = msl.find(kind, p)) != std::string::npos) {
        size_t q = p + kind.size();
        while (q < msl.size() && isspace((unsigned char)msl[q])) ++q;
        size_t r = q;
        while (r < msl.size() && (isalnum((unsigned char)msl[r]) || msl[r] == '_')) ++r;
        std::string ret = msl.substr(q, r - q); // kiểu trả về, bỏ qua
        (void)ret;
        while (r < msl.size() && isspace((unsigned char)msl[r])) ++r;
        size_t n0 = r;
        while (r < msl.size() && (isalnum((unsigned char)msl[r]) || msl[r] == '_')) ++r;
        std::string name = msl.substr(n0, r - n0);
        while (r < msl.size() && isspace((unsigned char)msl[r])) ++r;
        if (r < msl.size() && msl[r] == '(' && !name.empty()) return name;
        p = q;
    }
    return "";
}

int main() {
    using namespace tglmt;
    using namespace tglmt::metal;
    std::string dir = ShaderDir();
    if (dir.empty()) { printf("test_converter_proof SKIP: shaders not found\n"); return 0; }
    bool haveTools = (system("command -v glslangValidator >/dev/null 2>&1") == 0 &&
                      system("command -v spirv-cross >/dev/null 2>&1") == 0);
    std::string vsGLSL = Read(dir + "/fish.vert"), fsGLSL = Read(dir + "/fish.frag");
    GLSLConvertResult vr = ConvertGLSLtoMSL(vsGLSL, 0x8B31);
    GLSLConvertResult fr = ConvertGLSLtoMSL(fsGLSL, 0x8B30);
    if (!vr.ok || !fr.ok) { printf("FAIL converter: %s %s\n", vr.log.c_str(), fr.log.c_str()); return 1; }
    auto dev = CreateDevice("apple");
    if (!dev || dev->isNull()) { printf("test_converter_proof SKIP: no MTL device\n"); return 0; }
    if (!haveTools) { printf("test_converter_proof SKIP: missing glslang/spirv-cross\n"); return 0; }

    std::string tmp = "/tmp/tglmt-proof";
    std::string log;
    if (!Run("mkdir -p " + tmp, log) ||
        !Run("glslangValidator -G " + dir + "/fish.vert -o " + tmp + "/f.vert.spv", log) ||
        !Run("glslangValidator -G " + dir + "/fish.frag -o " + tmp + "/f.frag.spv", log) ||
        !Run("spirv-cross --msl " + tmp + "/f.vert.spv --output " + tmp + "/f.msl", log) ||
        !Run("spirv-cross --msl " + tmp + "/f.frag.spv --output " + tmp + "/f.frag.msl", log)) {
        printf("FAIL spirv chain:\n%s\n", log.c_str());
        return 1;
    }
    std::string spvVS = Read(tmp + "/f.msl"), spvFS = Read(tmp + "/f.frag.msl");
    std::string spvVsEntry = FindEntry(spvVS, "vertex"), spvFsEntry = FindEntry(spvFS, "fragment");
    if (spvVsEntry.empty() || spvFsEntry.empty()) {
        printf("FAIL spirv entry not found\n");
        return 1;
    }
    // spirv-cross đặt mỗi uniform vào buffer riêng theo thứ tự của nó (không phải
    // thứ tự khai báo — đã quan sát) → parse index từ MSL, không đoán.
    auto findBuf = [](const std::string& msl, const std::string& name) -> int {
        std::string key = name + " [[buffer(";
        size_t p = msl.find(key);
        if (p == std::string::npos) return -1;
        return atoi(msl.c_str() + p + key.size());
    };
    int bMvp = findBuf(spvVS, "uMVP"), bTime = findBuf(spvVS, "uTime"),
        bWag = findBuf(spvVS, "uWagFreq"), bLight = findBuf(spvFS, "uLightDir");
    if (bMvp < 0 || bTime < 0 || bWag < 0 || bLight < 0) {
        printf("FAIL spirv buffer parse (mvp=%d time=%d wag=%d light=%d)\n", bMvp, bTime, bWag,
               bLight);
        return 1;
    }
    // spirv-cross đặt uniforms ở buffer thấp, đè lên vertex buffer 0 của descriptor
    // → dời sang slot cao (16/17/18, fs 16) theo quy ước TGLMT (vertex data 0..15,
    // uniforms 16+). Chỉ đổi text khai báo buffer, không đổi semantics.
    {
        auto mv = [&](std::string& m, const std::string& name, int from, int to) {
            std::string a = name + " [[buffer(" + std::to_string(from) + ")]]";
            std::string b = name + " [[buffer(" + std::to_string(to) + ")]]";
            size_t p = m.find(a);
            if (p == std::string::npos) {
                printf("FAIL spirv slot rewrite %s\n", name.c_str());
                exit(1);
            }
            m.replace(p, a.size(), b);
        };
        mv(spvVS, "uMVP", bMvp, 16);
        mv(spvVS, "uTime", bTime, 17);
        mv(spvVS, "uWagFreq", bWag, 18);
        mv(spvFS, "uLightDir", bLight, 16);
    }
    std::string err;
    auto tLibV = dev->compileLibrary(vr.msl, err);
    if (!tLibV) { printf("FAIL tgmt-vs compile: %s\n", err.c_str()); return 1; }
    auto tLibF = dev->compileLibrary(fr.msl, err);
    if (!tLibF) { printf("FAIL tgmt-fs compile: %s\n", err.c_str()); return 1; }
    auto sLibV = dev->compileLibrary(spvVS, err);
    if (!sLibV) { printf("FAIL spirv-vs compile: %s\n", err.c_str()); return 1; }
    auto sLibF = dev->compileLibrary(spvFS, err);
    if (!sLibF) { printf("FAIL spirv-fs compile: %s\n", err.c_str()); return 1; }
    CustomAttrib attrs[3] = {{0, 3, 0x1406, false, 0}, {1, 3, 0x1406, false, 12}, {2, 4, 0x1406, false, 24}};
    auto tPso = dev->makeCustomPipeline(tLibV.get(), "TGLMT_vs", tLibF.get(), "TGLMT_fs",
                                        PixelFormat::RGBA8Unorm, attrs, 3, 40);
    auto sPso = dev->makeCustomPipeline(sLibV.get(), spvVsEntry.c_str(), sLibF.get(),
                                        spvFsEntry.c_str(), PixelFormat::RGBA8Unorm, attrs, 3, 40);
    if (!tPso || !sPso) { printf("FAIL pipeline\n"); return 1; }

    // Geometry + uniforms giống hệt 2 đường. MVP=identity, uTime=0 (không wag),
    // normal=(0,0,1), light=(0,0,1) → diff=1, color đỏ a=0 → stripe 0.85 → ~(216,0,0).
    struct V { float px, py, pz, nx, ny, nz, r, g, b, a; };
    V verts[3] = {{-0.5f, -0.5f, 0, 0, 0, 1, 1, 0, 0, 0},
                  {0.5f, -0.5f, 0, 0, 0, 1, 1, 0, 0, 0},
                  {0.0f, 0.5f, 0, 0, 0, 1, 1, 0, 0, 0}};
    auto vb = dev->newBufferWithBytes(verts, sizeof(verts), StorageMode::Shared);
    // Uniform buffer theo offsets converter (đường spirv: cùng thứ tự + MSL layout).
    auto offOf = [](const GLSLConvertResult& r, const char* n) -> size_t {
        for (auto& u : r.uniforms)
            if (u.name == n) return u.uniformOffset;
        return (size_t)-1;
    };
    std::vector<uint8_t> vsUB(vr.uniformBufferSize, 0), fsUB(fr.uniformBufferSize, 0);
    float ident[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    memcpy(vsUB.data() + offOf(vr, "uMVP"), ident, 64);
    float tm0 = 0.0f, wf = 2.0f;
    memcpy(vsUB.data() + offOf(vr, "uTime"), &tm0, 4);
    memcpy(vsUB.data() + offOf(vr, "uWagFreq"), &wf, 4);
    float light[3] = {0, 0, 1};
    memcpy(fsUB.data() + offOf(fr, "uLightDir"), light, 12);
    auto vsBuf = dev->newBufferWithBytes(vsUB.data(), vsUB.size(), StorageMode::Shared);
    auto fsBuf = dev->newBufferWithBytes(fsUB.data(), fsUB.size(), StorageMode::Shared);
    // Đường spirv: explicit-location uniforms → buffer RIÊNG theo location
    // (uMVP@0, uTime@1, uWagFreq@2; fs uLightDir@0) — đúng ABI spirv-cross.
    auto sMvp = dev->newBufferWithBytes(ident, 64, StorageMode::Shared);
    auto sTime = dev->newBufferWithBytes(&tm0, 4, StorageMode::Shared);
    auto sWag = dev->newBufferWithBytes(&wf, 4, StorageMode::Shared);
    auto sLight = dev->newBufferWithBytes(light, 12, StorageMode::Shared);

    auto renderT = [&]() -> std::string {
        auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
        auto enc = dev->makeRenderEncoder(target.get(), tPso.get(), ClearColor{0, 0, 0, 1});
        Viewport vp{0, 0, 64, 64, 0, 1};
        enc->setViewport(vp);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->setVertexBuffer(vsBuf.get(), 0, 16);
        enc->setFragmentBuffer(fsBuf.get(), 0, 16);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) return "commit";
        static unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        if (!target->readback(px, 64 * 4)) return "readback";
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        char b[64];
        snprintf(b, sizeof(b), "%u,%u,%u", mid[0], mid[1], mid[2]);
        return b;
    };
    auto renderS = [&]() -> std::string {
        auto target = dev->makeRenderTarget(64, 64, PixelFormat::RGBA8Unorm);
        auto enc = dev->makeRenderEncoder(target.get(), sPso.get(), ClearColor{0, 0, 0, 1});
        Viewport vp{0, 0, 64, 64, 0, 1};
        enc->setViewport(vp);
        enc->setVertexBuffer(vb.get(), 0, 0);
        enc->setVertexBuffer(sMvp.get(), 0, 16);
        enc->setVertexBuffer(sTime.get(), 0, 17);
        enc->setVertexBuffer(sWag.get(), 0, 18);
        enc->setFragmentBuffer(sLight.get(), 0, 16);
        enc->drawPrimitives(PrimitiveType::Triangle, 0, 3);
        if (!enc->endAndCommit()) return "commit";
        static unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        if (!target->readback(px, 64 * 4)) return "readback";
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        char b[64];
        snprintf(b, sizeof(b), "%u,%u,%u", mid[0], mid[1], mid[2]);
        return b;
    };
    std::string tpx = renderT();
    std::string spx = renderS();
    printf("tgmt=[%s] spirv=[%s]\n", tpx.c_str(), spx.c_str());
    if (tpx == "commit" || tpx == "readback" || spx == "commit" || spx == "readback") {
        printf("FAIL render path\n");
        return 1;
    }
    int r0, r1, r2, s0, s1, s2;
    if (sscanf(tpx.c_str(), "%d,%d,%d", &r0, &r1, &r2) != 3 ||
        sscanf(spx.c_str(), "%d,%d,%d", &s0, &s1, &s2) != 3) {
        printf("FAIL parse\n");
        return 1;
    }
    // Kỳ vọng vật lý: đỏ*0.85 ≈ 216 (diff=1, stripe=0.85+0.15*sin(0)).
    bool physical = std::abs(r0 - 216) <= 6 && r1 <= 4 && r2 <= 4 &&
                    std::abs(s0 - 216) <= 6 && s1 <= 4 && s2 <= 4;
    bool match = std::abs(r0 - s0) <= 5 && std::abs(r1 - s1) <= 5 && std::abs(r2 - s2) <= 5;
    if (!physical) { printf("FAIL physical expectation\n"); return 1; }
    if (!match) { printf("FAIL converter vs spirv-cross mismatch\n"); return 1; }
    printf("test_converter_proof PASS (converter == spirv-cross on GPU)\n");
    return 0;
}
