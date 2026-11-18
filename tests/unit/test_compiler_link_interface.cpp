#include "tglmt/compiler/ShaderIR.h"
#include "test_util.h"

using namespace tglmt::compiler;

static ShaderCompileResult Compile(ShaderStage stage, const std::string& src) {
    auto c = CreateCompiler();
    return c->compileShader(stage, src, GLSLVersion{}, GlslProfile::Core, ExtensionSet{},
                            CompileOptions{}, SourceOrigin{"test", 0});
}

static bool HasCode(const ProgramLinkResult& r, const std::string& code) {
    for (const auto& d : r.diagnostics)
        if (d.code == code) return true;
    return false;
}

int main() {
    std::string vs = R"(#version 330
layout(location=0) in vec3 aPos;
out vec3 vColor;
out vec2 vUv;
void main() { vColor = aPos; vUv = aPos.xy; gl_Position = vec4(aPos, 1.0); }
)";
    std::string fsOk = R"(#version 330
in vec3 vColor;
in vec2 vUv;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, vUv.x); }
)";
    std::string fsBadType = R"(#version 330
in vec2 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 0.0, 1.0); }
)";
    std::string fsMissing = R"(#version 330
in vec4 vMissing;
out vec4 fragColor;
void main() { fragColor = vMissing; }
)";

    ShaderCompileResult rvs = Compile(ShaderStage::Vertex, vs);
    TGLMT_CHECK(rvs.success, "VS phai compile duoc");
    TGLMT_CHECK(rvs.outputs.size() == 2, "VS phai co 2 output interface");

    auto c = CreateCompiler();
    ProgramLinkRequest okReq;
    okReq.stages.push_back(rvs);
    okReq.stages.push_back(Compile(ShaderStage::Fragment, fsOk));
    TGLMT_CHECK(okReq.stages.back().success, "FS ok phai compile duoc");
    ProgramLinkResult ok = c->linkProgram(okReq);
    TGLMT_CHECK(ok.success, "interface khop phai link thanh cong");

    ProgramLinkRequest badTypeReq;
    badTypeReq.stages.push_back(rvs);
    badTypeReq.stages.push_back(Compile(ShaderStage::Fragment, fsBadType));
    TGLMT_CHECK(badTypeReq.stages.back().success, "FS sai kieu van compile duoc");
    ProgramLinkResult badType = c->linkProgram(badTypeReq);
    TGLMT_CHECK(!badType.success, "interface sai kieu phai link that bai");
    TGLMT_CHECK(HasCode(badType, "LNK-002"), "phai bao LNK-002 cho sai kieu");

    ProgramLinkRequest missingReq;
    missingReq.stages.push_back(rvs);
    missingReq.stages.push_back(Compile(ShaderStage::Fragment, fsMissing));
    TGLMT_CHECK(missingReq.stages.back().success, "FS thieu input van compile duoc");
    ProgramLinkResult missing = c->linkProgram(missingReq);
    TGLMT_CHECK(!missing.success, "interface thieu phai link that bai");
    TGLMT_CHECK(HasCode(missing, "LNK-001"), "phai bao LNK-001 cho thieu output");

    printf("test_compiler_link_interface: failures=%d\n", tglmt_test::Failures());
    return TGLMT_RESULT;
}
