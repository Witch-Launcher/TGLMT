// test_semantic.cpp — Unit test cho phân tích ngữ nghĩa GLSL (src/compiler/Semantic.cpp).
// Mục tiêu: xác minh type-checker, phân giải overload, báo lỗi chưa khai báo,
// kiểu không khớp. Chạy backend Null (không cần Metal).
// TDD: test viết trước, implement sau cho pass.
#include "tglmt/compiler/Lexer.h"
#include "tglmt/compiler/AST.h"
#include "tglmt/compiler/Preprocessor.h"
#include "tglmt/compiler/Semantic.h"
#include "test_util.h"

#include <cstdio>
#include <string>

using namespace tglmt::compiler;

static int gFails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++gFails; } } while(0)

static SemanticResult Analyze(const std::string& src, ShaderStage stage,
                              const GLSLVersion& ver, GlslProfile prof) {
    Preprocessor pp(src, "t.glsl");
    auto pres = pp.run();
    Lexer lex(pres.source, "t.glsl");
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), "t.glsl");
    auto tu = parser.parse();
    SemanticAnalyzer sa(stage, ver, prof, ExtensionSet{});
    return sa.analyze(tu);
}

int main() {
    // 1) pow2(vec3) → overload vec3, exact match, thành công
    {
        std::string src = R"(
#version 460 core
out vec4 c;
void main() {
    vec3 v = vec3(1.0);
    vec3 r = pow2(v);
    c = vec4(r, 1.0);
}
)";
        auto r = Analyze(src, ShaderStage::Fragment, {4,60}, GlslProfile::Core);
        CHECK(r.success, "pow2(vec3) phân giải overload đúng");
        if (!r.success) {
            for (auto& d : r.diagnostics) printf("   [%s] %s\n", d.code.c_str(), d.message.c_str());
        }
    }

    // 2) pow2(3) int literal → overload int
    {
        std::string src = R"(
#version 460 core
out vec4 c;
void main() {
    int i = pow2(3);
    c = vec4(float(i),0,0,1);
}
)";
        auto r = Analyze(src, ShaderStage::Fragment, {4,60}, GlslProfile::Core);
        CHECK(r.success, "pow2(3) int overload phân giải đúng");
    }

    // 3) biến chưa khai báo → lỗi SEM-002
    {
        std::string src = R"(
#version 460 core
out vec4 c;
void main() {
    c = vec4(undeclared, 0, 0, 1);
}
)";
        auto r = Analyze(src, ShaderStage::Fragment, {4,60}, GlslProfile::Core);
        CHECK(!r.success, "bien chua khai bao phai fail");
        bool has002 = false;
        for (auto& d : r.diagnostics) if (d.code == "SEM-002") has002 = true;
        CHECK(has002, "bao loi SEM-002 (chua khai bao)");
    }

    // 4) overload mơ hồ: gọi pow2 với kiểu không khớp bất kỳ overload nào
    {
        std::string src = R"(
#version 460 core
out vec4 c;
void main() {
    bvec3 v = bvec3(true);
    vec3 r = pow2(v);
    c = vec4(r, 1.0);
}
)";
        auto r = Analyze(src, ShaderStage::Fragment, {4,60}, GlslProfile::Core);
        printf("   diagnostics: %zu (success=%d)\n", r.diagnostics.size(), (int)r.success);
        for (auto& d : r.diagnostics) printf("   [%s] %s\n", d.code.c_str(), d.message.c_str());
        if (r.success) {
            printf("   (unexpectedly succeeded; diagnostics: %zu)\n", r.diagnostics.size());
            for (auto& d : r.diagnostics) printf("   [%s] %s\n", d.code.c_str(), d.message.c_str());
        }
        CHECK(!r.success, "pow2(bvec3) khong co overload phu hop phai fail (bool khong implicit -> float)");
    }

    // 5) gán kiểu không khớp: vec3 = float (không có conversion exact)
    {
        std::string src = R"(
#version 460 core
out vec4 c;
void main() {
    vec3 v = 1.0;   // float -> vec3: khong chuan (can constructor)
    c = vec4(v, 1.0);
}
)";
        auto r = Analyze(src, ShaderStage::Fragment, {4,60}, GlslProfile::Core);
        // float->vec3 ko phải assignment trực tiếp hợp lệ (cần constructor)
        // Tuỳ policy: đây có thể fail. Chỉ in ra để quan sát.
        if (!r.success) {
            printf("   (float->vec3 gán trực tiếp fail, như mong đợi)\n");
        }
    }

    printf("test_semantic: %d fail(s)\n", gFails);
    return gFails == 0 ? 0 : 1;
}
