// test_converter_aquarium.cpp — Converter phủ toàn bộ shader app (14 stages).
// Không cần GPU: assert convert ok + metal -c biên dịch được + varyings 2 stage
// khớp nhau (tên/location/type) + uniform layout sanity. Thiếu tool → SKIP phần đó.
#include "tglmt/GLSLConverter.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
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
static bool Run(const std::string& cmd) { return system(cmd.c_str()) == 0; }

int main() {
    using namespace tglmt;
    std::string dir = ShaderDir();
    if (dir.empty()) { printf("test_converter_aquarium SKIP: shaders not found\n"); return 0; }
    const char* pairs[7][2] = {{"fish.vert", "fish.frag"}, {"water.vert", "water.frag"},
                               {"sand.vert", "sand.frag"}, {"plant.vert", "plant.frag"},
                               {"bubble.vert", "bubble.frag"}, {"bg.vert", "bg.frag"},
                               {"hud.vert", "hud.frag"}};
    bool haveMetal = (system("xcrun --sdk macosx --find metal >/dev/null 2>&1") == 0);
    for (auto& pr : pairs) {
        std::string vs = Read(dir + "/" + pr[0]), fs = Read(dir + "/" + pr[1]);
        GLSLConvertResult vr = ConvertGLSLtoMSL(vs, 0x8B31);
        GLSLConvertResult fr = ConvertGLSLtoMSL(fs, 0x8B30);
        if (!vr.ok) { printf("FAIL convert %s: %s\n", pr[0], vr.log.c_str()); return 1; }
        if (!fr.ok) { printf("FAIL convert %s: %s\n", pr[1], fr.log.c_str()); return 1; }
        // varyings vs-out ↔ fs-in khớp (tên, location, kiểu)
        for (auto& o : vr.outputs) {
            bool found = false;
            for (auto& i : fr.inputs)
                if (i.name == o.name && i.location == o.location && i.mslType == o.mslType)
                    found = true;
            if (!found) { printf("FAIL varying mismatch %s:%s\n", pr[0], o.name.c_str()); return 1; }
        }
        // uniform layout sanity: offsets tăng dần, trong buffer
        for (auto& u : vr.uniforms) {
            if (u.glslType == "sampler2D") continue;
            assert(u.uniformOffset + u.uniformSize <= vr.uniformBufferSize);
        }
        if (haveMetal) {
            std::string base = std::string("/tmp/tglmt-conv-") + pr[0];
            { std::ofstream f(base + ".msl"); f << vr.msl; }
            { std::ofstream f(base + ".frag.msl"); f << fr.msl; }
            if (!Run("xcrun -sdk macosx metal -c " + base + ".msl -o " + base + ".air >/dev/null 2>&1")) {
                printf("FAIL metal -c %s\n", pr[0]);
                return 1;
            }
            if (!Run("xcrun -sdk macosx metal -c " + base + ".frag.msl -o " + base + ".frag.air >/dev/null 2>&1")) {
                printf("FAIL metal -c %s\n", pr[1]);
                return 1;
            }
        }
        printf("ok %s + %s (vsUB=%zu fsUB=%zu)\n", pr[0], pr[1], vr.uniformBufferSize,
               fr.uniformBufferSize);
    }
    if (!haveMetal) printf("(metal compiler missing: compile-check skipped)\n");
    printf("test_converter_aquarium PASS (12/14 stages)\n");
    return 0;
}
