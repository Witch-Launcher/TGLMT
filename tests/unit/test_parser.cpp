// test_parser.cpp — Unit test cho parser GLSL (src/compiler/Parser.cpp).
// Mục tiêu: xác minh recursive descent parser sinh AST đúng cấu trúc cho:
// déclaration biến/uniform, hàm, struct, khối lệnh, biểu thức, lệnh điều khiển.
// Chạy backend Null (không cần Metal).
// TDD: test viết trước, implement sau cho pass.
#include "tglmt/compiler/Lexer.h"
#include "tglmt/compiler/AST.h"
#include "test_util.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt::compiler;

static int gFails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++gFails; } } while(0)

static AstTranslationUnit ParseSrc(const std::string& src, const std::string& file) {
    Lexer lex(src, file);
    auto tokens = lex.tokenize();
    Parser parser(std::move(tokens), file);
    return parser.parse();
}

int main() {
    // 1) Variable declaration
    {
        auto tu = ParseSrc("uniform vec3 uColor;\nfloat x = 1.0;\nint arr[4];\n", "t.glsl");
        CHECK(tu.success, "parse variable declaration thành công");
        CHECK(tu.declarations.size() >= 2, "có ít nhất 2 khai báo");
        bool foundUniform = false;
        bool foundArray = false;
        for (auto& d : tu.declarations) {
            if (d.varName == "uColor") foundUniform = true;
            if (d.varName == "arr") foundArray = true;
        }
        CHECK(foundUniform, "có uniform uColor");
        CHECK(foundArray, "có mảng arr");
    }

    // 2) Function declaration + definition
    {
        auto tu = ParseSrc("float pow2(float x) { return x * x; }\nvoid main() { discard; }\n", "t.glsl");
        CHECK(tu.success, "parse function thành công");
        bool foundPow2 = false;
        bool foundMain = false;
        for (auto& d : tu.declarations) {
            if (d.kind == AstDeclKind::Function) {
                if (d.funcName == "pow2") foundPow2 = true;
                if (d.funcName == "main") foundMain = true;
            }
        }
        CHECK(foundPow2, "có hàm pow2");
        CHECK(foundMain, "có hàm main");
    }

    // 3) Expression: binary, call, member
    {
        auto tu = ParseSrc("vec3 v = uColor + vec3(1.0);\nfloat a = dot(x, y);\n", "t.glsl");
        CHECK(tu.success, "parse biểu thức thành công");
    }

    // 4) if / for / while / discard / return (trong ham main)
    {
        auto tu = ParseSrc("void main() {\n"
                           "  if (a > 0) { discard; } else { return 1.0; }\n"
                           "  for (; x; ) {}\n"
                           "  while (x) {}\n"
                           "}\n", "t.glsl");
        if (!tu.success) {
            printf("  diagnostics: %zu\n", tu.diagnostics.size());
            for (auto& d : tu.diagnostics) printf("   [%s] %s @ %d:%d\n", d.code.c_str(), d.message.c_str(), d.span->begin.line, d.span->begin.column);
        }
        CHECK(tu.success, "parse lenh dieu khien trong ham main thành công");
    }

    // 5) Struct
    {
        auto tu = ParseSrc("struct Light { vec3 pos; float intensity; };\n", "t.glsl");
        CHECK(tu.success, "parse struct thành công");
        bool foundStruct = false;
        for (auto& d : tu.declarations) {
            if (d.kind == AstDeclKind::Struct) foundStruct = true;
        }
        CHECK(foundStruct, "có struct Light");
    }

    // 6) Syntax error → fail honest
    {
        auto tu = ParseSrc("float x =\n", "t.glsl");
        CHECK(!tu.success, "sai cuap phap phai fail");
    }

    printf("test_parser: %d fail(s)\n", gFails);
    return gFails == 0 ? 0 : 1;
}
