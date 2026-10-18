// glsl2msl_cli.cpp — chạy ConvertGLSLtoMSL trên 1 file (debug/harness pack).
// Build: clang++ -std=c++17 -O1 tools/glsl2msl_cli.cpp src/gl/glsl_to_msl.cpp \
//            -Iinclude -o /tmp/glsl2msl
#include "tglmt/GLSLConverter.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s file.glsl [stage=vsh|fsh|gsh|tcs|tes|csh] [-o out.msl]\n", argv[0]);
        return 2;
    }
    uint32_t stage = 0x8B30;
    std::string outPath;
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) { outPath = argv[++i]; continue; }
        if (a == "vsh") stage = 0x8B31;
        else if (a == "fsh") stage = 0x8B30;
        else if (a == "gsh") stage = 0x8DD9;
        else if (a == "tcs") stage = 0x8E88;
        else if (a == "tes") stage = 0x8E87;
        else if (a == "csh") stage = 0x91B9;
    }
    std::ifstream f(argv[1], std::ios::binary);
    if (!f) { fprintf(stderr, "không mở được %s\n", argv[1]); return 2; }
    std::ostringstream ss; ss << f.rdbuf();
    tglmt::GLSLConvertResult r = tglmt::ConvertGLSLtoMSL(ss.str(), stage);
    if (!r.ok) {
        printf("FAIL %s: %s\n", argv[1], r.log.c_str());
        return 1;
    }
    if (!outPath.empty()) {
        std::ofstream o(outPath, std::ios::binary);
        o << r.msl;
    } else {
        fputs(r.msl.c_str(), stdout);
    }
    return 0;
}
