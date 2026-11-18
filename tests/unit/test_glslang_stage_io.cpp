#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/compiler/backend/glslang/GlslangBackend.h"

#include "test_util.h"

#include <string>

using namespace tglmt::compiler;
using namespace tglmt::compiler::backend::glslang;

static const Variable* FindVar(const std::vector<Variable>& vars, const std::string& name) {
    for (const auto& v : vars)
        if (v.name == name) return &v;
    return nullptr;
}

static ShaderCompileResult Compile(Compiler* c, ShaderStage stage, const std::string& src) {
    return c->compileShader(stage, src, GLSLVersion{}, GlslProfile::Core, ExtensionSet{},
                            CompileOptions{}, SourceOrigin{"test", 0});
}

static void CheckPair(const char* name, const std::vector<Variable>& vsOut,
                      const std::vector<Variable>& fsIn) {
    for (const char* varying : {"vUV", "vColor"}) {
        const Variable* a = FindVar(vsOut, varying);
        const Variable* b = FindVar(fsIn, varying);
        TGLMT_CHECK(a != nullptr, varying);
        TGLMT_CHECK(b != nullptr, varying);
        if (a && b) {
            TGLMT_CHECK(a->location >= 0, "vertex output location assigned");
            TGLMT_CHECK(b->location >= 0, "fragment input location assigned");
            TGLMT_CHECK(a->location == b->location, "matching location across stages");
        }
    }
    (void)name;
}

int main() {
    auto compiler = CreateGlslangCompiler();
    auto vr = Compile(compiler.get(), ShaderStage::Vertex,
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec2 aUV;\n"
        "out vec2 vUV;\n"
        "out vec3 vColor;\n"
        "out float aUnused;\n"
        "void main() { vUV = aUV; vColor = aPos; aUnused = 0.0;"
        " gl_Position = vec4(aPos, 1.0); }\n");
    TGLMT_CHECK(vr.success, "vertex compiles");
    auto fr = Compile(compiler.get(), ShaderStage::Fragment,
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "in vec2 vUV;\n"
        "out vec4 oColor;\n"
        "void main() { oColor = vec4(vColor, vUV.x); }\n");
    TGLMT_CHECK(fr.success, "fragment compiles");

    ProgramLinkRequest req;
    req.stages.push_back(vr);
    req.stages.push_back(fr);
    auto linked = compiler->linkProgram(req);
    TGLMT_CHECK(linked.success, "link succeeds");
    TGLMT_CHECK(linked.stages.size() == 2, "link returns stages");
    const ShaderCompileResult* lvs = nullptr;
    const ShaderCompileResult* lfs = nullptr;
    for (const auto& s : linked.stages) {
        if (s.stage == ShaderStage::Vertex) lvs = &s;
        else if (s.stage == ShaderStage::Fragment) lfs = &s;
    }
    TGLMT_CHECK(lvs != nullptr, "linked vertex stage");
    TGLMT_CHECK(lfs != nullptr, "linked fragment stage");
    if (lvs && lfs) {
        CheckPair("link", lvs->outputs, lfs->inputs);
        TGLMT_CHECK(lvs->moduleId.find("user(locn") != std::string::npos,
                    "vertex MSL has varyings");
        TGLMT_CHECK(lfs->moduleId.find("user(locn") != std::string::npos,
                    "fragment MSL has varyings");
    }
    return TGLMT_RESULT;
}
