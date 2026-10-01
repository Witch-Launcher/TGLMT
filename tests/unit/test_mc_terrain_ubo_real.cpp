// test_mc_terrain_ubo_real.cpp — UBO terrain vanilla 26.1.2, không đoán.
//
// Provenance (EULA: không copy source Mojang, chỉ facts API):
// - client.jar SHA1 4e618f09a0c649dde3fdf829df443ce0b8831e65 (.tmp/mc26, gitignore).
// - Shader thật: tests/corpus/mc-26.1.2-shaders/core/terrain.{vsh,fsh} +
//   include/{fog,globals,chunksection,projection,dynamictransforms}.glsl
//   (trích bằng fetch-corpus.sh, xem test_mc_corpus).
// - javap GlCommandEncoder.writeToTexture: UNPACK_ROW_LENGTH(3314)=img.w,
//   SKIP_ROWS(3316)/SKIP_PIXELS(3315), ALIGNMENT(3317)=components,
//   rồi _texSubImage2D — atlas upload qua nhiều sub-region nhỏ.
// - Log máy thật (build b4-crash1): uboSmall tăng 0→64k, mipBase→239k,
//   world đen nhưng panorama/menu 3D lên, nút mất nền, inventory rỗng.
//   → Atlas-textured draws (terrain/blocks/gui) đen, non-textured (text,
//   container nền) lên. Nghi ngờ UBO zero-fallback hoặc atlas upload.
// - Test này khóa: minSize các block terrain (Fog/Globals/ChunkSection/
//   Projection) phải <= kích thước std140 vanilla cung cấp (không false
//   uboSmall), và upload atlas kiểu vanilla (ROW_LENGTH+SKIP) phải giữ pixels.

#include "tglmt/gl46.h"
#include "tglmt/Context.h"

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

#ifndef TGLMT_CORPUS_DIR
#define TGLMT_CORPUS_DIR "tests/corpus/mc-26.1.2-shaders"
#endif

static int gFails = 0;
static void Check(bool c, const char* m) {
    if (!c) {
        printf("test_mc_terrain_ubo_real FAIL: %s (err=0x%x)\n", m, glGetError());
        while (glGetError() != 0) {}
        ++gFails;
    }
}
static std::string Read(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
static std::string Inline(const std::string& src, const fs::path& inc) {
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
            std::string ref = (a != std::string::npos && b != std::string::npos && b > a)
                                  ? t.substr(a + 1, b - a - 1)
                                  : "";
            size_t c = ref.find(':');
            std::string nm = (c == std::string::npos) ? ref : ref.substr(c + 1);
            out += "#line 1 0\n" + Read(inc / nm) + "\n#line " + std::to_string(n + 1) + " 1\n";
            continue;
        }
        out += line + "\n";
    }
    return out;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    fs::path root = TGLMT_CORPUS_DIR;
    if (!fs::exists(root)) {
        printf("test_mc_terrain_ubo_real SKIP (thieu corpus)\n");
        return 0;
    }

    std::string vs = Inline(Read(root / "core" / "terrain.vsh"), root / "include");
    std::string fs2 = Inline(Read(root / "core" / "terrain.fsh"), root / "include");
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    const char* p1 = vs.c_str();
    const char* p2 = fs2.c_str();
    glShaderSource(v, 1, &p1, nullptr);
    glShaderSource(f, 1, &p2, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(v, 0x8B81, &ok);
    Check(ok != 0, "terrain vs compile");
    glGetShaderiv(f, 0x8B81, &ok);
    Check(ok != 0, "terrain fs compile");
    if (gFails) return 1;

    GLuint pr = glCreateProgram();
    glAttachShader(pr, v);
    glAttachShader(pr, f);
    glLinkProgram(pr);
    glGetProgramiv(pr, 0x8B82, &ok);
    Check(ok != 0, "terrain link");
    if (gFails) return 1;

    // minSize theo converter (đã in bằng .tmp/check_ubos: Fog 40, Globals 56,
    // ChunkSection 92, Projection 64). Vanilla cung cấp std140 padded:
    // Fog 48, Globals 64, ChunkSection 96, Projection 64 (suy từ struct +
    // log máy uborange point0=256/point1=64 cho menu — terrain tương tự).
    // Khóa: minSize <= cung cấp (không false uboSmall → zero fallback → đen).
    struct Expect {
        const char* name;
        size_t vanillaPadded;
    };
    Expect ex[] = {
        {"Fog", 48},
        {"Globals", 64},
        {"ChunkSection", 96},
        {"Projection", 64},
    };
    auto it = ctx.programs.find(pr);
    Check(it != ctx.programs.end(), "prog store");
    if (it != ctx.programs.end()) {
        for (auto& e : ex) {
            size_t need = 0;
            for (auto& b : it->second.uniformBlocks)
                if (b.name == e.name) need = b.minSize;
            printf("block %s minSize=%zu vanilla=%zu\n", e.name, need, e.vanillaPadded);
            Check(need != 0, e.name);
            // need là max end-offset (không pad cuối) nên phải <= padded.
            Check(need <= e.vanillaPadded, e.name);
        }
        // Terrain VS trên máy không bind DynamicTransforms (đúng vanilla:
        // ModelViewMat lấy từ ChunkSection) — converter không được đòi block
        // thừa gây misbound.
        bool hasDT = false;
        for (auto& s : it->second.vsBlocks)
            if (s == "DynamicTransforms") hasDT = true;
        printf("vsBlocks has DynamicTransforms=%d (want 0)\n", (int)hasDT);
        Check(!hasDT, "terrain vs không đòi DynamicTransforms");
    }

    // Atlas upload kiểu vanilla: TexImage NULL 64x64 rồi nhiều TexSubImage
    // với ROW_LENGTH = srcW + SKIP_ROWS/PIXELS (tile từ atlas lớn).
    // Dựng src 128x128 pattern, upload tile 32x32 tại (8,8) vào dst (16,16).
    {
        const int SW = 128, SH = 128;
        std::vector<uint8_t> src((size_t)SW * SH * 4);
        for (int y = 0; y < SH; ++y)
            for (int x = 0; x < SW; ++x) {
                uint8_t* p = src.data() + ((size_t)y * SW + x) * 4;
                p[0] = (uint8_t)x; p[1] = (uint8_t)y; p[2] = 128; p[3] = 255;
            }
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1, tex);
        glPixelStorei(0x0CF5, 1);
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
        Check(glGetError() == 0, "atlas alloc");
        // Đúng javap writeToTexture: ROW_LENGTH=srcW, SKIP_ROWS/PIXELS, ALIGN=4.
        glPixelStorei(0x0CF2 /*UNPACK_ROW_LENGTH*/, SW);
        glPixelStorei(0x0CF3 /*UNPACK_SKIP_ROWS*/, 8);
        glPixelStorei(0x0CF4 /*UNPACK_SKIP_PIXELS*/, 8);
        glPixelStorei(0x0CF5 /*UNPACK_ALIGNMENT*/, 4);
        // src tile 32x32 bắt đầu (8,8) trong src → dst (16,16).
        // TGLMT TexSub đọc từ `pixels` + skip: truyền con trỏ đầu src.
        glTexSubImage2D(0x0DE1, 0, 16, 16, 32, 32, 0x1908, 0x1401, src.data());
        Check(glGetError() == 0, "atlas sub");
        auto tit = ctx.textures.find(tex);
        Check(tit != ctx.textures.end(), "tex store");
        if (tit != ctx.textures.end()) {
            // dst(16,16) phải = src(8,8) = (8,8,128,255).
            const uint8_t* d = tit->second.pixels.data() + ((size_t)16 * 64 + 16) * 4;
            printf("atlas dst=(%u,%u,%u,%u) want=(8,8,128,255)\n", d[0], d[1], d[2], d[3]);
            Check(d[0] == 8 && d[1] == 8 && d[2] == 128 && d[3] == 255, "atlas tile khớp");
            // Ngoài tile phải còn 0 (không lem do pitch sai).
            const uint8_t* o = tit->second.pixels.data();
            Check(o[0] == 0 && o[1] == 0, "atlas ngoài tile còn 0");
        }
        // Trả PixelStore về mặc định để test sau không dính.
        glPixelStorei(0x0CF2, 0);
        glPixelStorei(0x0CF3, 0);
        glPixelStorei(0x0CF4, 0);
        glPixelStorei(0x0CF5, 4);
    }

    if (gFails) {
        printf("test_mc_terrain_ubo_real FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_mc_terrain_ubo_real PASS (terrain blocks + atlas ROW_LENGTH/SKIP)\n");
    return 0;
}
