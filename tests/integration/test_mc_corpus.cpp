// test_mc_corpus.cpp — Chạy TOÀN BỘ shader vanilla 26.1.2 qua converter+link,
// đúng như Blaze3D làm: inline #moj_import, prepend defines biến thể.
// Corpus: tests/corpus/mc-26.1.2-shaders (trích từ client.jar, xem fetch-corpus.sh).
// Mục tiêu: 0 fail. Mỗi fail in tên shader + lý do để fix có đích.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
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

// Inline #moj_import <minecraft:name.glsl> như Blaze3D (kèm #line để trung thực).
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
                out += "// TGLMT-CORPUS: missing import " + ref + "\n";
                continue;
            }
            out += "#line 1 0\n" + Read(ip) + "\n#line " + std::to_string(lineNo + 1) + " 1\n";
            continue;
        }
        out += line + "\n";
    }
    return out;
}

int main() {
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_corpus SKIP (thieu corpus %s)\n", root.string().c_str());
        return 0;
    }
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    fs::path core = root / "core";
    fs::path incDir = root / "include";

    // Thu thập VS/FS theo basename
    std::map<std::string, std::string> vsMap, fsMap;
    for (auto& e : fs::directory_iterator(core)) {
        std::string fn = e.path().filename().string();
        if (fn.size() > 4 && fn.substr(fn.size() - 4) == ".vsh")
            vsMap[fn.substr(0, fn.size() - 4)] = Read(e.path());
        if (fn.size() > 4 && fn.substr(fn.size() - 4) == ".fsh")
            fsMap[fn.substr(0, fn.size() - 4)] = Read(e.path());
    }

    // Cặp link: cùng basename + cặp đặc biệt của Blaze3D (screenquad/animate_sprite
    // không có VS riêng, dùng chung screenquad.vsh / animate_sprite.vsh).
    std::vector<std::pair<std::string, std::string>> pairs;
    for (auto& [name, _] : vsMap) {
        if (name == "screenquad") continue; // pair riêng bên dưới
        auto it = fsMap.find(name);
        if (it != fsMap.end()) pairs.emplace_back(name, name);
    }
    const char* extra[][2] = {
        {"screenquad", "lightmap"}, {"screenquad", "blit_screen"},
        {"animate_sprite", "animate_sprite_blit"}, {"animate_sprite", "animate_sprite_interpolate"},
    };
    for (auto& pr : extra)
        if (vsMap.count(pr[0]) && fsMap.count(pr[1])) pairs.emplace_back(pr[0], pr[1]);

    // Defines biến thể Blaze3D prepend (quan sát trên máy qua log lỗi).
    const std::string kDefines = "#define ALPHA_CUTOUT 0.5\n#define EMISSIVE\n";

    // Compile mỗi shader 1 lần, share qua programs (đúng Blaze3D, test shared-shader).
    std::map<std::string, GLuint> vsCache, fsCache;
    int convertFail = 0, linkFail = 0, okPairs = 0;
    auto getVS = [&](const std::string& name) -> GLuint {
        auto it = vsCache.find(name);
        if (it != vsCache.end()) return it->second;
        std::string src = InlineImports(vsMap[name], incDir);
        // 2 biến thể: trần + kèm defines (Blaze3D prepend theo program)
        GLuint sh = glCreateShader(0x8B31);
        const char* p = src.c_str();
        glShaderSource(sh, 1, &p, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, 0x8B81, &ok);
        if (!ok) {
            GLchar log[1024] = {0};
            GLsizei l = 0;
            glGetShaderInfoLog(sh, 1024, &l, log);
            printf("VS-FAIL %s: %s\n", name.c_str(), log);
            // thử biến thể defines
            std::string src2 = kDefines + src;
            GLuint sh2 = glCreateShader(0x8B31);
            const char* p2 = src2.c_str();
            glShaderSource(sh2, 1, &p2, nullptr);
            glCompileShader(sh2);
            glGetShaderiv(sh2, 0x8B81, &ok);
            if (!ok) {
                glGetShaderInfoLog(sh2, 1024, &l, log);
                printf("VS-FAIL %s (+defines): %s\n", name.c_str(), log);
                ++convertFail;
                return 0;
            }
            printf("VS-NOTE %s: can defines moi compile (tran thi fail)\n", name.c_str());
            vsCache[name + "+defines"] = sh2;
            vsCache[name] = sh2;
            return sh2;
        }
        vsCache[name] = sh;
        return sh;
    };
    auto getFS = [&](const std::string& name) -> GLuint {
        auto it = fsCache.find(name);
        if (it != fsCache.end()) return it->second;
        std::string src = InlineImports(fsMap[name], incDir);
        GLuint sh = glCreateShader(0x8B30);
        const char* p = src.c_str();
        glShaderSource(sh, 1, &p, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, 0x8B81, &ok);
        if (!ok) {
            GLchar log[1024] = {0};
            GLsizei l = 0;
            glGetShaderInfoLog(sh, 1024, &l, log);
            printf("FS-FAIL %s: %s\n", name.c_str(), log);
            ++convertFail;
            return 0;
        }
        fsCache[name] = sh;
        return sh;
    };

    for (auto& [vn, fn] : pairs) {
        GLuint vs = getVS(vn);
        GLuint fs = getFS(fn);
        if (!vs || !fs) continue; // đã đếm ở trên
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, 0x8B82, &ok);
        if (!ok) {
            GLchar log[1024] = {0};
            GLsizei l = 0;
            glGetProgramInfoLog(p, 1024, &l, log);
            printf("LINK-FAIL %s+%s: %s\n", vn.c_str(), fn.c_str(), log);
            ++linkFail;
            continue;
        }
        ++okPairs;
    }

    // post/*.fsh compile-only (pair runtime, không link ở đây)
    int postFail = 0, postOk = 0;
    for (auto& e : fs::directory_iterator(root / "post")) {
        std::string fn = e.path().filename().string();
        if (fn.size() < 5 || fn.substr(fn.size() - 4) != ".fsh") continue;
        std::string src = InlineImports(Read(e.path()), incDir);
        GLuint sh = glCreateShader(0x8B30);
        const char* p = src.c_str();
        glShaderSource(sh, 1, &p, nullptr);
        glCompileShader(sh);
        GLint ok = 0;
        glGetShaderiv(sh, 0x8B81, &ok);
        if (!ok) {
            GLchar log[1024] = {0};
            GLsizei l = 0;
            glGetShaderInfoLog(sh, 1024, &l, log);
            printf("POST-FAIL %s: %s\n", fn.c_str(), log);
            ++postFail;
        } else {
            ++postOk;
        }
    }

    printf("test_mc_corpus: pairs=%d ok=%d convertFail=%d linkFail=%d post=%d/%d\n",
           (int)pairs.size(), okPairs, convertFail, linkFail, postOk, postOk + postFail);
    if (convertFail || linkFail || postFail) {
        printf("test_mc_corpus FAIL\n");
        return 1;
    }
    printf("test_mc_corpus PASS\n");
    return 0;
}
