#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/compiler/backend/glslang/GlslangBackend.h"
#include "test_util.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace tglmt::compiler;
using namespace tglmt::compiler::backend::glslang;

#ifndef TGLMT_CORPUS_DIR
#define TGLMT_CORPUS_DIR "tests/corpus/mc-26.1.2-shaders"
#endif

static std::string StripVersionLines(const std::string& src) {
    std::istringstream iss(src);
    std::string line, out;
    while (std::getline(iss, line)) {
        size_t s = line.find_first_not_of(" \t\r");
        if (s != std::string::npos && line.compare(s, 8, "#version") == 0) continue;
        out += line + "\n";
    }
    return out;
}

static std::string ReadFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string InlineImports(const std::string& src, const fs::path& incDir,
                                 std::set<std::string>& included) {
    std::istringstream iss(src);
    std::string line, out;
    while (std::getline(iss, line)) {
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
            fs::path ip = incDir / name;
            if (!fs::exists(ip)) {
                out += "#error missing import " + ref + "\n";
                continue;
            }
            if (included.insert(ip.string()).second)
                out += StripVersionLines(ReadFile(ip)) + "\n";
            continue;
        }
        out += line + "\n";
    }
    return out;
}

static std::set<std::string> CollectMacros(const std::string& src) {
    std::set<std::string> out;
    std::istringstream iss(src);
    std::string line;
    auto isIdent = [](char c, bool first) {
        return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
    };
    while (std::getline(iss, line)) {
        size_t s = line.find_first_not_of(" \t\r");
        if (s == std::string::npos || line[s] != '#') continue;
        size_t k = s + 1;
        while (k < line.size() && isspace((unsigned char)line[k])) ++k;
        size_t e = k;
        while (e < line.size() && isIdent(line[e], e == k)) ++e;
        std::string dir = line.substr(k, e - k);
        std::string rest = line.substr(e);
        auto addWord = [&](size_t p) {
            while (p < rest.size() && !isIdent(rest[p], true)) ++p;
            size_t q = p;
            while (q < rest.size() && isIdent(rest[q], q == p)) ++q;
            if (q > p) out.insert(rest.substr(p, q - p));
        };
        if (dir == "ifdef" || dir == "ifndef") {
            addWord(0);
        } else if (dir == "if" || dir == "elif") {
            size_t p = 0;
            while ((p = rest.find("defined", p)) != std::string::npos) {
                p += 7;
                addWord(p);
            }
        }
    }
    return out;
}

static std::string DefinesPrefix(const std::set<std::string>& ms) {
    std::string out;
    for (auto& m : ms) {
        if (m == "ALPHA_CUTOUT") out += "#define ALPHA_CUTOUT 0.5\n";
        else out += "#define " + m + " 1\n";
    }
    return out;
}

static std::string InjectAfterVersion(const std::string& src, const std::string& defs) {
    std::istringstream iss(src);
    std::string line, out;
    bool injected = false;
    while (std::getline(iss, line)) {
        out += line + "\n";
        if (!injected) {
            size_t s = line.find_first_not_of(" \t\r");
            if (s != std::string::npos && line.compare(s, 8, "#version") == 0) {
                out += defs;
                injected = true;
            }
        }
    }
    return injected ? out : defs + out;
}

static bool MetalAvailable() {
    return std::system("xcrun -find metal >/dev/null 2>&1") == 0;
}

static bool CompileMSL(const std::string& msl, const std::string& tag) {
    fs::path tmp = fs::temp_directory_path() /
                   ("tglmt_msl_" + std::to_string((long)getpid()) + "_" +
                    std::to_string(std::hash<std::string>{}(tag)) + ".metal");
    {
        std::ofstream f(tmp);
        f << msl;
    }
    std::string cmd = "xcrun -sdk macosx metal -c \"" + tmp.string() + "\" -o \"" +
                      tmp.string() + ".metallib\" 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    std::string log;
    char buf[1024];
    while (p && fgets(buf, sizeof buf, p)) log += buf;
    int rc = p ? pclose(p) : -1;
    fs::remove(tmp);
    fs::remove(tmp.string() + ".metallib");
    if (rc == 0) return true;
    fprintf(stderr, "METAL FAIL %s\n%s\n", tag.c_str(), log.c_str());
    return false;
}

int main() {
    if (!MetalAvailable()) {
        printf("test_glslang_spirv_cross_corpus SKIP (khong co xcrun metal)\n");
        return 0;
    }
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_glslang_spirv_cross_corpus SKIP (thieu corpus %s)\n", root.string().c_str());
        return 0;
    }
    fs::path incDir = root / "include";
    struct Job {
        std::string path;
        ShaderStage stage;
        std::string source;
    };
    std::vector<Job> jobs;
    for (const char* dir : {"core", "post"}) {
        fs::path d = root / dir;
        if (!fs::exists(d)) continue;
        for (auto& e : fs::directory_iterator(d)) {
            std::string fn = e.path().filename().string();
            ShaderStage stage;
            if (fn.size() > 4 && fn.substr(fn.size() - 4) == ".vsh") stage = ShaderStage::Vertex;
            else if (fn.size() > 4 && fn.substr(fn.size() - 4) == ".fsh") stage = ShaderStage::Fragment;
            else continue;
            std::string path = (fs::path(dir) / fn).generic_string();
            std::set<std::string> included;
            std::string base = InlineImports(ReadFile(e.path()), incDir, included);
            std::set<std::string> macros = CollectMacros(base);
            macros.erase("PORTAL_LAYERS");
            jobs.push_back({path, stage,
                            InjectAfterVersion(base, "#define PORTAL_LAYERS 16\n" +
                                                          DefinesPrefix(macros))});
        }
    }
    int files = (int)jobs.size();
    std::atomic<int> next{0};
    std::atomic<int> passed{0};
    unsigned hw = std::thread::hardware_concurrency();
    int workers = (int)std::min<unsigned>(8, hw == 0 ? 4 : hw);
    auto run = [&] {
        for (;;) {
            int i = next.fetch_add(1);
            if (i >= (int)jobs.size()) break;
            auto& j = jobs[i];
            auto compiler = CreateGlslangCompiler();
            ShaderCompileResult r = compiler->compileShader(
                j.stage, j.source, GLSLVersion{}, GlslProfile::Core, ExtensionSet{},
                CompileOptions{}, SourceOrigin{j.path, 0});
            if (!r.success) {
                fprintf(stderr, "COMPILE FAIL %s (%zu diagnostics)\n", j.path.c_str(),
                        r.diagnostics.size());
                for (const auto& d : r.diagnostics) {
                    std::string msg = d.message;
                    if (msg.size() > 1200) msg.resize(1200);
                    fprintf(stderr, "  [%s] %s\n", d.code.c_str(), msg.c_str());
                }
                continue;
            }
            if (r.moduleId.empty()) {
                fprintf(stderr, "EMPTY MSL %s\n", j.path.c_str());
                continue;
            }
            if (CompileMSL(r.moduleId, j.path)) passed.fetch_add(1);
        }
    };
    std::vector<std::thread> threads;
    for (int i = 0; i < workers; ++i) threads.emplace_back(run);
    for (auto& t : threads) t.join();
    int ok = passed.load();
    TGLMT_CHECK(files >= 70, "corpus phai co it nhat 70 shader");
    TGLMT_CHECK(ok == files, "moi shader corpus phai emit MSL ma xcrun metal -c chap nhan");
    printf("test_glslang_spirv_cross_corpus: files=%d metal_ok=%d failures=%d\n", files, ok,
           tglmt_test::Failures());
    return TGLMT_RESULT;
}
