#include "tglmt/compiler/ShaderIR.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace tglmt::compiler;

#ifndef TGLMT_CORPUS_DIR
#define TGLMT_CORPUS_DIR "tests/corpus/mc-26.1.2-shaders"
#endif

static std::string ReadFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

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
            fs::path ip = incDir / name;
            if (!fs::exists(ip)) {
                out += "#error missing import " + ref + "\n";
                continue;
            }
            out += "#line 1 0\n" + ReadFile(ip) + "\n#line " + std::to_string(lineNo + 1) + " 1\n";
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

static bool CompileOne(const std::string& path, const std::string& source,
                       ShaderStage stage, const std::string& variantName) {
    auto compiler = CreateCompiler();
    ShaderCompileResult r = compiler->compileShader(
        stage, source, GLSLVersion{}, GlslProfile::Core, ExtensionSet{}, CompileOptions{},
        SourceOrigin{path, 0});
    if (r.success) return true;
    fprintf(stderr, "FAIL %s (%s): %zu diagnostics\n", path.c_str(), variantName.c_str(),
            r.diagnostics.size());
    for (auto& d : r.diagnostics)
        fprintf(stderr, "  [%s] %s (line=%d)\n", d.code.c_str(), d.message.c_str(),
                d.span ? d.span->begin.line : 0);
    return false;
}

int main() {
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_compiler_new_pipeline_corpus SKIP (thieu corpus %s)\n", root.string().c_str());
        return 0;
    }
    fs::path incDir = root / "include";

    const std::string runtimeDefs = "#define PORTAL_LAYERS 16\n";
    int files = 0, variants = 0;
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
            std::string base = InlineImports(ReadFile(e.path()), incDir);
            std::set<std::string> macros = CollectMacros(base);
            macros.erase("PORTAL_LAYERS");
            std::string allOn = DefinesPrefix(macros) + base;
            ++files;
            TGLMT_CHECK(CompileOne(path, runtimeDefs + base, stage, "base"),
                        ("base variant: " + path).c_str());
            ++variants;
            TGLMT_CHECK(CompileOne(path, runtimeDefs + allOn, stage, "all-on"),
                        ("all-on variant: " + path).c_str());
            ++variants;
        }
    }
    TGLMT_CHECK(files >= 70, "corpus phai co it nhat 70 shader");
    printf("test_compiler_new_pipeline_corpus: files=%d variants=%d failures=%d\n", files, variants,
           tglmt_test::Failures());
    return TGLMT_RESULT;
}
