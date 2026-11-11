// tests/shaders/test_shader_corpus.cpp — Biên dịch corpus GLSL 4.60 và kiểm
// tra MSL sinh ra.
//
// Mỗi shader trong `tests/shaders/glsl/` được dịch GLSL→MSL bằng converter
// thật, rồi MSL được **kiểm tra nội dung** (không chỉ "không lỗi"):
//   • các mẫu MSL bắt buộc phải có (ví dụ `[[flat]]` cho varying flat,
//     `bias(...)` cho texture bias, `[[color(1)]]` cho output location=1,
//     `calculate_unclamped_lod` cho textureQueryLod);
//   • MSL phải **không** chứa các mẫu cấm (tên hàm GLSL chưa được dịch).
//
// Nếu có Metal compiler, MSL còn được biên dịch thật để chứng minh nó
// *compile được* — không chỉ rằng converter không báo lỗi.
#include "tglmt/GLSLConverter.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace tglmt;

static std::string Dir() {
#ifdef TGLMT_ROOT
    return std::string(TGLMT_ROOT) + "/tests/shaders/glsl";
#else
    return "tests/shaders/glsl";
#endif
}

static bool ReadFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.good()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static bool HasMetalCompiler() {
    return std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") == 0;
}

struct Case {
    const char* file;
    uint32_t stage;
    const char* mustContain;   // mẫu BẮT BUỘC có trong MSL (nullptr = không yêu cầu)
    const char* mustNotContain; // mẫu CẤM có trong MSL
};

int main() {
    static const Case kCases[] = {
        {"basic.vert", 0x8B31, nullptr, nullptr},
        {"basic.frag", 0x8B30, "bias(", "textureQueryLod"},
        {"flat_varying.vert", 0x8B31, "[[flat]]", nullptr},
        {"flat_varying.frag", 0x8B30, "[[flat]]", nullptr},
        {"single_output_loc1.frag", 0x8B30, "[[color(1)]]", "[[color(0)]]"},
        {"compute.comp", 0x91B9, "threadgroup", nullptr},
        {"query_lod.frag", 0x8B30, "calculate_unclamped_lod", "textureQueryLod"},
    };

    const std::string dir = Dir();
    const bool metal = HasMetalCompiler();
    int converted = 0;
    int metalCompiled = 0;

    for (const Case& c : kCases) {
        std::string src;
        const std::string path = dir + "/" + c.file;
        if (!ReadFile(path, src)) {
            printf("test_shader_corpus SKIP: không đọc được %s\n", path.c_str());
            continue;
        }
        GLSLConvertResult r = ConvertGLSLtoMSL(src, c.stage);
        TGLMT_CHECK(r.ok, (std::string("convert thất bại: ") + c.file).c_str());
        if (!r.ok) {
            fprintf(stderr, "  log: %s\n", r.log.c_str());
            continue;
        }
        ++converted;
        if (c.mustContain) {
            TGLMT_CHECK(r.msl.find(c.mustContain) != std::string::npos,
                        (std::string("MSL phải chứa '") + c.mustContain + "' (" + c.file + ")")
                            .c_str());
        }
        if (c.mustNotContain) {
            TGLMT_CHECK(r.msl.find(c.mustNotContain) == std::string::npos,
                        (std::string("MSL KHÔNG được chứa '") + c.mustNotContain + "' (" +
                         c.file + ")")
                            .c_str());
        }
        if (metal) {
            // Biên dịch thật bằng Metal compiler và kiểm tra file .air được
            // tạo (metal -c có thể exit 0 mà không sinh file nếu input bị coi
            // là linker input).
            const std::string base = std::string("/tmp/tglmt-corpus-") + c.file;
            {
                std::ofstream f(base + ".metal");
                f << r.msl;
            }
            const std::string air = base + ".air";
            const std::string cmd = "xcrun -sdk macosx metal -c " + base + ".metal -o " + air +
                                    " >/dev/null 2>&1";
            if (std::system(cmd.c_str()) != 0) {
                TGLMT_CHECK(false, (std::string("Metal compiler từ chối: ") + c.file).c_str());
                continue;
            }
            std::ifstream probe(air, std::ios::binary);
            TGLMT_CHECK(probe.good(),
                        (std::string("Metal compiler không sinh .air: ") + c.file).c_str());
            if (probe.good()) ++metalCompiled;
        }
    }

    printf("test_shader_corpus: %d shader convert, %d Metal-compiled\n", converted,
           metalCompiled);
    TGLMT_CHECK(converted == (int)(sizeof(kCases) / sizeof(kCases[0])),
                "mọi shader trong corpus phải dịch được");
    return TGLMT_RESULT;
}
