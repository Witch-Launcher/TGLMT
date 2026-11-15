// bench_translate.cpp — đo THỜI GIAN DỊCH GLSL → MSL (không Metal).
//
// Mục tiêu: biết chính xác phần nào của pipeline "compile shader" tốn thời
// gian, để tối ưu mà KHÔNG đổi hành vi OpenGL Desktop sau khi compile.
//
// Khác với bench_compile (đo GLSL→MSL→Metal library→pipeline), benchmark này
// chỉ đo ConvertGLSLtoMSL() ⇒ phần Metal (newLibraryWithSource) bị loại khỏi
// phép đo.
//
// Corpus: tests/corpus/mc-26.1.2-shaders (79 shader .vsh/.fsh thật của
// Minecraft) + tests/shaders/glsl/*.vert|frag (shader nhỏ của repo).
// #moj_import được inline trước khi đo (giống tools/dump_msl_corpus.cpp).
//
// Protocol (benchmarks/README.md): warm-up bỏ, ≥9 lần chạy, báo cáo median
// của các median + min.
//
// Build (dùng build-bench có sẵn, KHÔNG tạo build dir mới — mỗi build dir
// ~2 GB và đã làm đầy phân vùng):
//   cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON
//   cmake --build build-bench -j8 --target bench_translate
//   ./build-bench/bench_translate --repeats 9 --warmup 2
//
// Output: JSON trên stdout (cùng schema với bench_compile để diff được).
#include "tglmt/gl46.h"
#include "tglmt/GLSLConverter.h"
#include <algorithm>
#include <chrono>
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

static std::string ReadFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

struct Shader {
    std::string name;
    std::string src;
    uint32_t stage;
};

// Nội tuyến #moj_import <...> (giống tools/dump_msl_corpus.cpp) để shader
// dịch được độc lập.
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
            if (a != std::string::npos && b != std::string::npos) {
                std::string ref = t.substr(a + 1, b - a - 1);
                size_t colon = ref.find(':');
                std::string name = (colon == std::string::npos) ? ref : ref.substr(colon + 1);
                fs::path p = incDir / name;
                if (fs::exists(p)) {
                    out += "#line 1 0\n" + ReadFile(p) + "\n#line " + std::to_string(n + 1) + " 1\n";
                    continue;
                }
            }
        }
        out += line + "\n";
    }
    return out;
}

static void CollectCorpus(std::vector<Shader>& out) {
    fs::path root = "tests/corpus/mc-26.1.2-shaders";
    fs::path incDir = root / "include";
    if (fs::exists(root)) {
        std::vector<fs::path> files;
        for (auto& e : fs::recursive_directory_iterator(root)) {
            if (!e.is_regular_file()) continue;
            fs::path p = e.path();
            std::string ext = p.extension().string();
            if (ext == ".vsh" || ext == ".fsh") files.push_back(p);
        }
        std::sort(files.begin(), files.end());
        for (auto& p : files) {
            Shader s;
            s.name = p.filename().string();
            s.src = InlineImports(ReadFile(p), incDir);
            s.stage = (p.extension().string() == ".vsh") ? 0x8B31u : 0x8B30u;
            if (s.src.empty()) continue;
            out.push_back(std::move(s));
        }
    }
    // Shader nhỏ của repo (tests/shaders/glsl) — biên dịch được không cần import.
    fs::path gdir = "tests/shaders/glsl";
    if (fs::exists(gdir)) {
        std::vector<fs::path> files;
        for (auto& e : fs::recursive_directory_iterator(gdir)) {
            if (e.is_regular_file()) files.push_back(e.path());
        }
        std::sort(files.begin(), files.end());
        for (auto& p : files) {
            std::string ext = p.extension().string();
            uint32_t stage = 0;
            if (ext == ".vert") stage = 0x8B31u;
            else if (ext == ".frag") stage = 0x8B30u;
            else if (ext == ".comp") stage = 0x91B9u;
            else continue;
            Shader s;
            s.name = "repo/" + p.filename().string();
            s.src = ReadFile(p);
            s.stage = stage;
            out.push_back(std::move(s));
        }
    }
}

int main(int argc, char** argv) {
    int repeats = 9, warmup = 2;
    std::string dumpDir;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--repeats") && i + 1 < argc) repeats = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warmup") && i + 1 < argc) warmup = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dump") && i + 1 < argc) dumpDir = argv[++i];
    }
    if (repeats < 1) repeats = 1;
    if (warmup < 0) warmup = 0;

    // Converter không cần Context (giống tests/shaders/test_shader_corpus.cpp).
    // Không tạo Context để tránh phụ thuộc backend.

    std::vector<Shader> corpus;
    CollectCorpus(corpus);
    if (corpus.empty()) {
        fprintf(stderr, "bench_translate: corpus rỗng (chạy từ gốc repo)\n");
        return 2;
    }

    // --dump <dir>: ghi MSL của MỖI shader ra file ⇒ cổng đối chiếu
    // "byte-identical" cho mọi tối ưu (không được đổi 1 byte MSL).
    if (!dumpDir.empty()) {
        fs::create_directories(dumpDir);
        size_t n = 0;
        for (size_t i = 0; i < corpus.size(); ++i) {
            GLSLConvertResult r = ConvertGLSLtoMSL(corpus[i].src, corpus[i].stage);
            std::string fn = corpus[i].name;
            for (char& ch : fn)
                if (ch == '/') ch = '_';
            std::ofstream o(fs::path(dumpDir) / (fn + ".msl"), std::ios::binary);
            o << r.msl;
            ++n;
        }
        printf("{\n  \"dumped\": %zu,\n  \"dir\": \"%s\"\n}\n", n, dumpDir.c_str());
        return 0;
    }

    // Baseline: chạy 1 lần mỗi shader để biết cái nào dịch được / lỗi.
    size_t okCount = 0, srcBytes = 0, outBytes = 0;
    std::vector<double> perShaderUs(corpus.size(), 0.0);
    std::vector<char> ok(corpus.size(), 0);
    for (size_t i = 0; i < corpus.size(); ++i) {
        GLSLConvertResult r = ConvertGLSLtoMSL(corpus[i].src, corpus[i].stage);
        ok[i] = r.ok ? 1 : 0;
        if (r.ok) {
            ++okCount;
            srcBytes += corpus[i].src.size();
            outBytes += r.msl.size();
        }
    }

    // Warm-up (bỏ kết quả).
    for (int w = 0; w < warmup; ++w) {
        for (auto& s : corpus) {
            GLSLConvertResult r = ConvertGLSLtoMSL(s.src, s.stage);
            (void)r;
        }
    }

    // Mỗi lần chạy: đo TỔNG thời gian dịch toàn bộ corpus, và thời gian từng
    // shader (để biết shader nào nặng).
    std::vector<double> runUs;
    std::vector<double> totals(repeats, 0.0);
    for (int rep = 0; rep < repeats; ++rep) {
        auto t0 = std::chrono::steady_clock::now();
        for (size_t i = 0; i < corpus.size(); ++i) {
            auto s0 = std::chrono::steady_clock::now();
            GLSLConvertResult r = ConvertGLSLtoMSL(corpus[i].src, corpus[i].stage);
            auto s1 = std::chrono::steady_clock::now();
            if (rep == 0)
                perShaderUs[i] = std::chrono::duration<double, std::micro>(s1 - s0).count();
            (void)r;
        }
        auto t1 = std::chrono::steady_clock::now();
        totals[rep] = std::chrono::duration<double, std::micro>(t1 - t0).count();
    }

    std::vector<double> sorted = totals;
    std::sort(sorted.begin(), sorted.end());
    double median = sorted[sorted.size() / 2];
    double mn = sorted.front();
    double mx = sorted.back();

    double perShaderUsMed = (double)corpus.size() ? median / (double)corpus.size() : 0.0;
    double mbPerSec = (mn > 0.0) ? ((double)srcBytes / 1048576.0) / (mn / 1e6) : 0.0;

    // Thứ tự theo thời gian (nặng nhất trước).
    std::vector<size_t> idx(corpus.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(),
              [&](size_t a, size_t b) { return perShaderUs[a] > perShaderUs[b]; });

    printf("{\n");
    printf("  \"benchmark\": \"translate\",\n");
    printf("  \"scope\": \"GLSL->MSL only (no Metal)\",\n");
    printf("  \"shaders\": %zu,\n", corpus.size());
    printf("  \"shaders_ok\": %zu,\n", okCount);
    printf("  \"repeats\": %d,\n", repeats);
    printf("  \"warmup\": %d,\n", warmup);
    printf("  \"total_us_median\": %.3f,\n", median);
    printf("  \"total_us_min\": %.3f,\n", mn);
    printf("  \"total_us_max\": %.3f,\n", mx);
    printf("  \"per_shader_us_median\": %.3f,\n", perShaderUsMed);
    printf("  \"source_bytes\": %zu,\n", srcBytes);
    printf("  \"msl_bytes\": %zu,\n", outBytes);
    printf("  \"src_mb_per_sec\": %.3f,\n", mbPerSec);
    printf("  \"top_slowest\": [\n");
    for (size_t k = 0; k < idx.size() && k < 10; ++k) {
        printf("    {\"name\": \"%s\", \"us\": %.3f, \"stage\": \"%s\", \"ok\": %s}%s\n",
               corpus[idx[k]].name.c_str(), perShaderUs[idx[k]],
               corpus[idx[k]].stage == 0x8B31u ? "vertex" : (corpus[idx[k]].stage == 0x8B30u ? "fragment" : "other"),
               ok[idx[k]] ? "true" : "false", k + 1 < idx.size() && k < 9 ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");
    return 0;
}
