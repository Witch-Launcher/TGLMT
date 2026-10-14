// test_shader_subroutine.cpp — validate P6e: subroutine → switch-dispatch MSL.
// Trên backend Metal thật: Convert OK + MSL compile thật bằng Metal.
// Trên backend Null (device isNull): chỉ check Convert OK (bỏ qua MSL compile).
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt;

namespace {
std::shared_ptr<metal::IDevice> MakeDev() {
    auto d = metal::CreateDevice("apple", nullptr);
    if (!d || d->isNull()) return nullptr;
    return d;
}
#define FS "#version 460 core\n"
} // namespace

int main() {
    auto dev = MakeDev();
    const uint32_t Frag = 0x8B30;
    struct Case { const char* name; const char* glsl; bool expectOk; };
    std::vector<Case> cases = {
        {"sub_basic", FS "subroutine vec4 ft(vec4 v);\n"
                         "subroutine(ft) vec4 f1(vec4 v){ return v*2.0; }\n"
                         "subroutine(ft) vec4 f2(vec4 v){ return v+1.0; }\n"
                         "subroutine uniform ft myF;\nout vec4 c;\n"
                         "void main(){ c = myF(vec4(1.0)); }", true},
        {"sub_2params", FS "subroutine float ft2(float a, float b);\n"
                           "subroutine(ft2) float g1(float a, float b){ return a+b; }\n"
                           "subroutine(ft2) float g2(float a, float b){ return a*b; }\n"
                           "subroutine uniform ft2 myG;\nout vec4 c;\n"
                           "void main(){ c = vec4(myG(1.0, 2.0)); }", true},
        {"sub_void", FS "subroutine void ftv(float x);\n"
                        "subroutine(ftv) void h1(float x){ }\n"
                        "subroutine uniform ftv myH;\nout vec4 c;\n"
                        "void main(){ myH(1.0); c = vec4(1); }", true},
        {"sub_2vars", FS "subroutine vec4 ft(vec4 v);\n"
                         "subroutine(ft) vec4 f1(vec4 v){ return v; }\n"
                         "subroutine(ft) vec4 f2(vec4 v){ return -v; }\n"
                         "subroutine uniform ft a;\nsubroutine uniform ft b;\nout vec4 c;\n"
                         "void main(){ c = a(vec4(1)) + b(vec4(2)); }", true},
        {"sub_notype_neg", FS "subroutine uniform ft myF;\nout vec4 c;\n"
                              "void main(){ c = myF(vec4(1)); }", false},
    };

    int fail = 0;
    for (auto& cs : cases) {
        auto r = ConvertGLSLtoMSL(cs.glsl, Frag);
        if (!r.ok) {
            if (cs.expectOk) { printf("CONV-FAIL %-16s %s\n", cs.name, r.log.c_str()); ++fail; }
            continue;
        }
        if (!cs.expectOk) { printf("UNEXPECTED-OK %-16s\n", cs.name); ++fail; continue; }
        if (dev) {
            std::string err;
            auto lib = dev->compileLibrary(r.msl, err);
            if (!lib) { printf("MSL-FAIL %-16s %s\n", cs.name, err.c_str()); ++fail; }
        }
    }
    if (fail == 0) { printf("test_shader_subroutine: %zu cases OK\n", cases.size()); return 0; }
    printf("test_shader_subroutine: %d FAIL\n", fail);
    return 1;
}
