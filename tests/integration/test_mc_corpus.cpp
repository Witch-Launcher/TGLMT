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
#include <set>
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

    // Defines biến thể Blaze3D prepend theo program (GlProgram.link bind attribute
    // theo VertexFormat; ShaderProgramKeys/Defines kèm macro vào source).
    // Mỗi shader test 2 biến thể: base (không defines → nhánh #else) + all-on
    // (mọi macro file đó nhắc tới → nhánh #ifdef). Thiếu all-on là crash thật
    // 2026-09-28: entity.fsh chỉ pass base, game build với PER_FACE_LIGHTING
    // (8 pipeline entity_*) bật nhánh `gl_FrontFacing` chưa từng được test.
    auto collectMacros = [&](const std::string& src) {
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
                // defined(X) — mọi dạng còn lại converter đã fail rõ nên ở đây
                // chỉ thu thập tên để bật all-on.
                size_t p = 0;
                while ((p = rest.find("defined", p)) != std::string::npos) {
                    p += 7;
                    addWord(p);
                }
            }
        }
        return out;
    };
    auto definesPrefix = [&](const std::set<std::string>& ms) {
        std::string out;
        for (auto& m : ms) {
            // Giá trị đúng runtime: PORTAL_LAYERS=16 (COLORS 16 entries),
            // ALPHA_CUTOUT=0.5 (so sánh alpha); còn lại là cờ → 1.
            if (m == "PORTAL_LAYERS") out += "#define PORTAL_LAYERS 16\n";
            else if (m == "ALPHA_CUTOUT") out += "#define ALPHA_CUTOUT 0.5\n";
            else out += "#define " + m + " 1\n";
        }
        return out;
    };
    // [[attribute(N)]] trùng nhau trong VS MSL = Metal compile fail chắc chắn
    // (crumbling + bind "Normal"→0, crash 2026-09-28). Assert text này chạy được
    // trên backend null (không cần GPU) cho MỌI pair đã link.
    auto dupAttrib = [&](const std::string& msl, std::string& detail) -> bool {
        std::set<int> seen;
        size_t k = 0;
        while ((k = msl.find("[[attribute(", k)) != std::string::npos) {
            int n = atoi(msl.c_str() + k + 12);
            if (seen.count(n)) {
                detail = "attribute(" + std::to_string(n) + ") lap lai";
                return true;
            }
            seen.insert(n);
            ++k;
        }
        return false;
    };

    // Compile mỗi shader theo biến thể, share qua cache (đúng Blaze3D test shared-shader).
    std::map<std::string, GLuint> vsCache, fsCache;
    int convertFail = 0, linkFail = 0, okPairs = 0, okVariants = 0;
    auto getVS = [&](const std::string& name, int variant) -> GLuint {
        std::string key = name + (variant ? "+D" : "");
        auto it = vsCache.find(key);
        if (it != vsCache.end()) return it->second;
        std::string src = InlineImports(vsMap[name], incDir);
        if (variant) src = definesPrefix(collectMacros(src)) + src;
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
            printf("VS-FAIL %s%s: %s\n", name.c_str(), variant ? " (+defines)" : "", log);
            ++convertFail;
            return 0;
        }
        vsCache[key] = sh;
        return sh;
    };
    auto getFS = [&](const std::string& name, int variant) -> GLuint {
        std::string key = name + (variant ? "+D" : "");
        auto it = fsCache.find(key);
        if (it != fsCache.end()) return it->second;
        std::string src = InlineImports(fsMap[name], incDir);
        if (variant) src = definesPrefix(collectMacros(src)) + src;
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
            printf("FS-FAIL %s%s: %s\n", name.c_str(), variant ? " (+defines)" : "", log);
            ++convertFail;
            return 0;
        }
        fsCache[key] = sh;
        return sh;
    };

    for (auto& [vn, fn] : pairs) {
        // Biến thể cần test = hợp macro của cả VS+FS (rỗng → chỉ base).
        std::set<std::string> macros;
        {
            auto a = collectMacros(InlineImports(vsMap[vn], incDir));
            auto b = collectMacros(InlineImports(fsMap[fn], incDir));
            macros.insert(a.begin(), a.end());
            macros.insert(b.begin(), b.end());
        }
        bool pairOk = true;
        for (int variant = 0; variant <= (macros.empty() ? 0 : 1); ++variant) {
            GLuint vs = getVS(vn, variant);
            GLuint fs = getFS(fn, variant);
            if (!vs || !fs) { pairOk = false; continue; } // đã đếm ở trên
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
                printf("LINK-FAIL %s+%s%s: %s\n", vn.c_str(), fn.c_str(),
                       variant ? " (+defines)" : "", log);
                ++linkFail;
                pairOk = false;
                continue;
            }
            std::string detail;
            if (dupAttrib(ctx.programs[p].vertexMSL, detail)) {
                printf("ATTR-FAIL %s+%s%s: %s\n", vn.c_str(), fn.c_str(),
                       variant ? " (+defines)" : "", detail.c_str());
                ++linkFail;
                pairOk = false;
                continue;
            }
            ++okVariants;
        }
        if (pairOk) ++okPairs;
    }
    // Hồi quy crash 2026-09-28: game (GlProgram.link) bind "Normal"→0 cho program
    // crumbling trong khi Normal inactive trong shader. Sau strip phải link OK,
    // attribute duy nhất, và Normal không còn trong VIn.
    {
        GLuint vs = getVS("rendertype_crumbling", 0);
        GLuint fs = getFS("rendertype_crumbling", 0);
        if (vs && fs) {
            GLuint p = glCreateProgram();
            glAttachShader(p, vs);
            glAttachShader(p, fs);
            glBindAttribLocation(p, 0, "Normal");
            glLinkProgram(p);
            GLint ok = 0;
            glGetProgramiv(p, 0x8B82, &ok);
            std::string detail;
            bool dup = ok && dupAttrib(ctx.programs[p].vertexMSL, detail);
            bool normalLeft = ok && ctx.programs[p].vertexMSL.find("Normal") != std::string::npos;
            if (!ok || dup || normalLeft) {
                printf("BIND-FAIL rendertype_crumbling+Normal@0: linked=%d %s normalLeft=%d\n",
                       (int)ok, detail.c_str(), (int)normalLeft);
                ++linkFail;
            } else {
                ++okVariants;
            }
        }
    }

    // post/*.fsh compile-only (pair runtime, không link ở đây), cũng 2 biến thể
    int postFail = 0, postOk = 0;
    for (auto& e : fs::directory_iterator(root / "post")) {
        std::string fn = e.path().filename().string();
        if (fn.size() < 5 || fn.substr(fn.size() - 4) != ".fsh") continue;
        std::string base = InlineImports(Read(e.path()), incDir);
        auto macros = collectMacros(base);
        for (int variant = 0; variant <= (macros.empty() ? 0 : 1); ++variant) {
            std::string src = variant ? definesPrefix(macros) + base : base;
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
                printf("POST-FAIL %s%s: %s\n", fn.c_str(), variant ? " (+defines)" : "", log);
                ++postFail;
            } else {
                ++postOk;
            }
        }
    }

    printf("test_mc_corpus: pairs=%d ok=%d variants=%d convertFail=%d linkFail=%d post=%d/%d\n",
           (int)pairs.size(), okPairs, okVariants, convertFail, linkFail, postOk, postOk + postFail);
    if (convertFail || linkFail || postFail) {
        printf("test_mc_corpus FAIL\n");
        return 1;
    }
    printf("test_mc_corpus PASS\n");
    return 0;
}
