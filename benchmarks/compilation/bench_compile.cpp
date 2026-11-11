// benchmarks/compilation/bench_compile.cpp — Đo thời gian biên dịch shader.
//
// Đo từng giai đoạn của đường đi GLSL → MSL → Metal library → pipeline:
//   1. GLSL → MSL (converter trong src/gl/glsl_to_msl.cpp)
//   2. MSL → MTLLibrary (newLibraryWithSource)
//   3. Tạo MTLRenderPipelineState (makeCustomPipeline)
//   4. Tổng thời gian từ GLSL source → pipeline sẵn sàng
//
// Quy trình đo (có thể tái lập):
//   • warm-up N lần (không tính vào kết quả)
//   • R lần lặp, báo cáo median / p95 / min / max
//   • chế độ COLD: xoá cache giữa các lần đo
//   • chế độ WARM: giữ cache (đo đường cache hit)
//
// Xuất JSON ra stdout để tools/so sánh được.
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace tglmt;
using Clock = std::chrono::steady_clock;

struct Stat {
    std::vector<double> ms;
    void add(double v) { ms.push_back(v); }
    double median() const {
        if (ms.empty()) return 0;
        std::vector<double> s = ms;
        std::sort(s.begin(), s.end());
        return s[s.size() / 2];
    }
    double p95() const {
        if (ms.empty()) return 0;
        std::vector<double> s = ms;
        std::sort(s.begin(), s.end());
        size_t i = (size_t)(s.size() * 0.95);
        if (i >= s.size()) i = s.size() - 1;
        return s[i];
    }
    double min() const {
        if (ms.empty()) return 0;
        return *std::min_element(ms.begin(), ms.end());
    }
    double max() const {
        if (ms.empty()) return 0;
        return *std::max_element(ms.begin(), ms.end());
    }
    double mean() const {
        if (ms.empty()) return 0;
        double t = 0;
        for (double v : ms) t += v;
        return t / (double)ms.size();
    }
};

// Shader mẫu: tương độ phức tạp thực tế (UBO + SSBO + sampler + nhiều phép).
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec3 aPos;\n"
    "layout(location=1) in vec3 aNrm;\n"
    "layout(location=2) in vec2 aUV;\n"
    "layout(location=0) out vec2 vUV;\n"
    "layout(location=1) out vec3 vNrm;\n"
    "layout(std140, binding=0) uniform Camera { mat4 uMVP; mat4 uModel; vec4 uTint; };\n"
    "void main(){\n"
    "  vUV = aUV;\n"
    "  vNrm = aNrm;\n"
    "  gl_Position = uMVP * uModel * vec4(aPos,1.0);\n"
    "}\n";

static const char* kFS =
    "#version 460 core\n"
    "layout(location=0) in vec2 vUV;\n"
    "layout(location=1) in vec3 vNrm;\n"
    "layout(location=0) out vec4 oColor;\n"
    "layout(location=1) out vec4 oNormal;\n"
    "uniform sampler2D uAlbedo;\n"
    "uniform sampler2D uNormalMap;\n"
    "layout(std140, binding=1) uniform Material { vec4 uBase; float uRough; float uMetal; };\n"
    "layout(std140, binding=2) uniform Post { vec4 uTint; };\n"
    "layout(std430, binding=0) buffer Nodes { vec4 nodeData[]; };\n"
    "void main(){\n"
    "  vec3 n = normalize(vNrm);\n"
    "  vec4 alb = texture(uAlbedo, vUV) * uBase;\n"
    "  vec3 nn = texture(uNormalMap, vUV).xyz * 2.0 - 1.0;\n"
    "  n = normalize(n + vec3(nn.xy, 0.0));\n"
    "  float spec = pow(max(dot(n, normalize(vec3(1,1,1))), 0.0), mix(64.0, 4.0, uRough));\n"
    "  vec4 nd = nodeData[0];\n"
    "  oColor = alb * uTint + vec4(spec * uMetal);\n"
    "  oNormal = vec4(n * 0.5 + 0.5, 1.0) + nd * 0.0;\n"
    "}\n";

static const char* kCS =
    "#version 460 core\n"
    "layout(local_size_x=64) in;\n"
    "layout(std430, binding=0) buffer Src { float inData[]; };\n"
    "layout(std430, binding=1) buffer Dst { float outData[]; };\n"
    "shared float tile[64];\n"
    "void main(){\n"
    "  uint i = gl_GlobalInvocationID.x;\n"
    "  tile[gl_LocalInvocationID.x] = inData[i] * 2.0;\n"
    "  barrier();\n"
    "  outData[i] = tile[gl_LocalInvocationID.x] + 1.0;\n"
    "}\n";

int main(int argc, char** argv) {
    int repeats = 30;
    int warmup = 5;
    bool withMetal = true;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--repeats") && i + 1 < argc) repeats = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warmup") && i + 1 < argc) warmup = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--no-metal")) withMetal = false;
    }

    auto dev = metal::CreateDevice(withMetal ? "apple" : "null", nullptr);
    bool haveMetal = dev && !dev->isNull();
    if (withMetal && !haveMetal)
        fprintf(stderr, "[bench] không có Metal device — chỉ đo converter\n");

    struct Stage {
        const char* name;
        const char* src;
        uint32_t stage;
    } stages[] = {
        {"vs_complex", kVS, 0x8B31},
        {"fs_complex", kFS, 0x8B30},
        {"cs_compute", kCS, 0x91B9},
    };

    Stat conv[3], lib[3], pipe[3], total[3];

    for (int s = 0; s < 3; ++s) {
        // warm-up
        for (int w = 0; w < warmup; ++w) {
            GLSLConvertResult r = ConvertGLSLtoMSL(stages[s].src, stages[s].stage);
            (void)r;
        }
        for (int it = 0; it < repeats; ++it) {
            auto t0 = Clock::now();
            GLSLConvertResult r = ConvertGLSLtoMSL(stages[s].src, stages[s].stage);
            auto t1 = Clock::now();
            conv[s].add(std::chrono::duration<double, std::milli>(t1 - t0).count());
            if (!r.ok) {
                fprintf(stderr, "[bench] %s convert FAIL: %s\n", stages[s].name, r.log.c_str());
                continue;
            }
            if (!haveMetal) {
                total[s].add(conv[s].ms.back());
                continue;
            }
            std::string err;
            auto t2 = Clock::now();
            auto libObj = dev->compileLibrary(r.msl, err);
            auto t3 = Clock::now();
            lib[s].add(std::chrono::duration<double, std::milli>(t3 - t2).count());
            if (!libObj) {
                fprintf(stderr, "[bench] %s metal compile FAIL: %s\n", stages[s].name,
                        err.c_str());
                continue;
            }
            metal::PipelineOpts opts;
            opts.colorCount = 1;
            opts.colorFmts[0] = metal::PixelFormat::RGBA8Unorm;
            auto t4 = Clock::now();
            auto pipeObj = dev->makeCustomPipeline(libObj.get(), "TGLMT_vs", libObj.get(),
                                                   "TGLMT_fs", metal::PixelFormat::RGBA8Unorm,
                                                   nullptr, 0, 0, &opts);
            auto t5 = Clock::now();
            pipe[s].add(std::chrono::duration<double, std::milli>(t5 - t4).count());
            total[s].add(std::chrono::duration<double, std::milli>(t5 - t0).count());
        }
    }

    // Xuất JSON.
    printf("{\n");
    printf("  \"benchmark\": \"compilation\",\n");
    printf("  \"repeats\": %d,\n", repeats);
    printf("  \"warmup\": %d,\n", warmup);
    printf("  \"metal\": %s,\n", haveMetal ? "true" : "false");
    printf("  \"device\": \"%s\",\n", haveMetal ? dev->name().c_str() : "null");
    printf("  \"stages\": {\n");
    for (int s = 0; s < 3; ++s) {
        printf("    \"%s\": {\n", stages[s].name);
        auto dump = [&](const char* key, const Stat& st) {
            printf("      \"%s\": {\"median_ms\": %.4f, \"p95_ms\": %.4f, \"min_ms\": %.4f, "
                   "\"max_ms\": %.4f, \"mean_ms\": %.4f, \"n\": %zu}%s\n",
                   key, st.median(), st.p95(), st.min(), st.max(), st.mean(), st.ms.size(),
                   (key[0] == 't') ? "" : ",");
        };
        dump("glsl_to_msl", conv[s]);
        dump("msl_to_metal_library", lib[s]);
        dump("pipeline_creation", pipe[s]);
        dump("total", total[s]);
        printf("    }%s\n", (s < 2) ? "," : "");
    }
    printf("  }\n");
    printf("}\n");
    return 0;
}
