// test_lexer.cpp — Unit test cho lexer GLSL (src/compiler/Lexer.cpp).
// Mục tiêu: xác minh tokenization, source location, numeric literal, comment,
// từ khóa và các trường hợp lỗi lex.
// Chạy trên backend Null (không cần Metal).
#include "tglmt/compiler/Lexer.h"
#include "test_util.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt::compiler;

static void ShowTokens(const std::vector<Token>& tokens) {
    for (const auto& t : tokens) {
        printf("  [%d] %-20s '%s' @ %s:%d:%d\n",
               (int)t.type, t.lexeme.c_str(),
               t.location.file.c_str(), t.location.line, t.location.column);
    }
}

int main() {
    int fails = 0;
    #define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++fails; } } while(0)

    // 1) Identifiers + keywords + punctuation
    {
        std::string src = "uniform vec3 iris_Entity;\nvoid main() { discard; }\n";
        Lexer lex(src, "test.glsl");
        auto tokens = lex.tokenize();
        CHECK(!lex.hasError(), "không có lỗi lex");
        // uniform, vec3, iris_Entity, ;
        // Tìm token iris_Entity
        bool foundIris = false;
        for (const auto& t : tokens) {
            if (t.lexeme == "iris_Entity" && t.type == TokenType::Ident) {
                foundIris = true;
                CHECK(t.location.line == 1, "iris_Entity ở dòng 1");
            }
        }
        CHECK(foundIris, "có token iris_Entity");
    }

    // 2) Numeric literals (int, uint, float, hex)
    {
        struct { std::string src; TokenType type; } cases[] = {
            {"42", TokenType::IntLiteral},
            {"42u", TokenType::UintLiteral},
            {"3.14", TokenType::FloatLiteral},
            {"0xFF", TokenType::IntLiteral},
            {"1.0e-2", TokenType::FloatLiteral},
        };
        for (auto& c : cases) {
            Lexer lex(c.src, "t.glsl");
            auto tokens = lex.tokenize();
            CHECK(!lex.hasError(), ("literal: " + c.src).c_str());
            CHECK(tokens.size() >= 2, "có ít nhất literal + EOF");
            CHECK(tokens[0].type == c.type, ("literal type: " + c.src).c_str());
        }
    }

    // 3) Comments (line + block) bị bỏ qua, không sinh token
    {
        std::string src = "int a; // comment\nint /* block */ b;\n";
        Lexer lex(src, "t.glsl");
        auto tokens = lex.tokenize();
        CHECK(!lex.hasError(), "không lỗi comment");
        int idents = 0;
        for (const auto& t : tokens) {
            if (t.lexeme == "int") ++idents;
            if (t.lexeme == "a") ++idents;
            if (t.lexeme == "b") ++idents;
        }
        CHECK(idents == 4, "2 int + a + b (comment bị bỏ qua)");
    }

    // 4) Unterminated string literal → lỗi
    {
        std::string src = "\"unterminated";
        Lexer lex(src, "t.glsl");
        auto tokens = lex.tokenize();
        CHECK(lex.hasError(), "phát hiện chuỗi chưa kết thúc");
    }

    // 5) Source location: dòng/cột chính xác
    {
        std::string src = "vec3 iris_Entity;\n";
        Lexer lex(src, "shader.glsl");
        auto tokens = lex.tokenize();
        for (const auto& t : tokens) {
            if (t.lexeme == "iris_Entity") {
                CHECK(t.location.file == "shader.glsl", "tên file đúng");
                CHECK(t.location.line == 1, "dòng đúng");
                // "vec3 " = 5 ký tự, iris_Entity bắt đầu cột 6
                CHECK(t.location.column == 6, "cột đúng (sau 'vec3 ')");
            }
        }
    }

    // 6) EOF token cuối cùng
    {
        std::string src = "int a;";
        Lexer lex(src, "t.glsl");
        auto tokens = lex.tokenize();
        CHECK(!tokens.empty(), "có ít nhất 1 token");
        CHECK(tokens.back().type == TokenType::EOF_Token, "cuối là EOF");
    }

    #undef CHECK
    printf("test_lexer: %d fail(s)\n", fails);
    return fails == 0 ? 0 : 1;
}
