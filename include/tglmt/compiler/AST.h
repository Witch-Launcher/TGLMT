#pragma once




#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>

#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/compiler/Lexer.h"

namespace tglmt::compiler {





struct AstType {
    std::string baseType;
    bool isArray = false;
    std::vector<int> arraySizes;
    bool isConst = false;
};





struct AstExpression;
using AstExprPtr = std::shared_ptr<AstExpression>;

enum class AstExprKind {
    Integer, Float, Double, Uint, Bool, String,
    Identifier,
    Binary, Unary, Call, Member, Index, Swizzle, Constructor, Paren,
    Ternary,
};

struct AstExpression {
    AstExprKind kind = AstExprKind::Identifier;
    SourceLocation loc;
    AstType type;

    long long intValue = 0;
    double floatValue = 0.0;
    bool boolValue = false;
    std::string stringValue;

    std::string name;

    std::string op;
    AstExprPtr left, right, operand;

    std::string callee;
    std::vector<AstExprPtr> args;

    AstExprPtr object;
    bool isPostfix = false;
    std::string member;
    std::vector<AstExprPtr> indices;
    std::string swizzle;

    std::string ctorType;
    bool ctorIsArray = false;
    int ctorArraySize = 0;
    std::vector<AstExprPtr> ctorArgs;

    AstExprPtr cond, thenExpr, elseExpr;
};





struct AstDeclaration;

enum class AstStmtKind {
    Declaration, Expression, Return, Discard, If, For, While, DoWhile,
    Block, Break, Continue,
};

struct AstStatement;
using AstStmtPtr = std::shared_ptr<AstStatement>;

struct AstStatement {
    AstStmtKind kind = AstStmtKind::Declaration;
    SourceLocation loc;

    std::unique_ptr<AstDeclaration> decl;
    AstExprPtr declExpr;

    AstExprPtr exprStmt;

    AstExprPtr returnExpr;

    AstExprPtr ifCond;
    AstStmtPtr ifThen, ifElse;

    AstStmtPtr loopBody;
    AstExprPtr loopInit, loopCond, loopInc;
    std::unique_ptr<AstDeclaration> loopInitDecl;

    std::vector<AstStmtPtr> blockStmts;
};





enum class AstDeclKind {
    Variable, Function, Struct, InterfaceBlock,
};

struct AstParameter {
    std::string name;
    AstType type;
    StorageQualifier qualifier = StorageQualifier::None;
    SourceLocation loc;
};

struct AstDeclaration {
    AstDeclKind kind = AstDeclKind::Variable;
    SourceLocation loc;

    AstType varType;
    std::string varName;
    std::vector<StorageQualifier> varQualifiers;
    std::vector<LayoutQualifier> varLayout;
    AstExprPtr varInit;
    bool varIsUniform = false;
    bool varIsBuffer = false;
    int varLocation = -1;
    int varBinding = -1;

    std::string funcName;
    AstType funcReturnType;
    std::vector<AstParameter> funcParams;
    std::vector<AstStmtPtr> funcBody;
    bool funcIsBuiltin = false;

    std::string structName;
    struct StructMember { std::string name; AstType type; std::vector<LayoutQualifier> layout; SourceLocation loc; };
    std::vector<StructMember> structMembers;

    std::string blockName;
    std::string blockInstanceName;
    int blockBinding = -1;
    std::vector<AstParameter> blockMembers;
};





struct AstTranslationUnit {
    SourceLocation loc;
    GLSLVersion version{};
    GlslProfile profile = GlslProfile::Core;
    std::vector<AstDeclaration> declarations;
    std::vector<Diagnostic> diagnostics;
    bool success = false;
};





class Parser {
public:

    Parser(std::vector<Token> tokens, std::string fileName);

    AstTranslationUnit parse();

private:
    std::vector<Token> tokens_;
    std::string fileName_;
    size_t pos_ = 0;
    std::vector<Diagnostic> diagnostics_;

    Token peek(int ahead = 0) const;
    Token advance();
    bool check(TokenType t) const;
    bool match(TokenType t);
    bool matchAny(std::initializer_list<TokenType> types);
    SourceLocation locOf(const Token& t) const;
    void expect(TokenType t, const std::string& msg);
    void errorAt(const Token& t, const std::string& msg, const std::string& code = "PARSE-000");
    bool atEnd() const;


    AstType parseType();
    AstType parseTypeSpecifier();
    bool isTypeKeyword(TokenType t) const;


    AstDeclaration parseDeclaration();
    AstDeclaration parseVariableDeclaration(const AstType& baseType, StorageQualifier qualifier);
    AstDeclaration parseFunctionDeclaration(const AstType& returnType, const std::string& name);
    AstDeclaration parseStructDeclaration();
    AstDeclaration parseInterfaceBlock();


    AstStmtPtr parseStatement();
    AstStmtPtr parseStatementList();
    AstStmtPtr parseDeclarationStmt();
    AstStmtPtr parseExpressionStatement();
    AstStmtPtr parseReturnStatement();
    AstStmtPtr parseDiscardStatement();
    AstStmtPtr parseIfStatement();
    AstStmtPtr parseForStatement();
    AstStmtPtr parseWhileStatement();
    AstStmtPtr parseDoWhileStatement();
    AstStmtPtr parseBlockStatement();
    AstStmtPtr parseBreakContinue();


    AstExprPtr parseExpression();
    AstExprPtr parseAssignment();
    AstExprPtr parseTernary();
    AstExprPtr parseLogicalOr();
    AstExprPtr parseLogicalXor();
    AstExprPtr parseLogicalAnd();
    AstExprPtr parseBitwiseOr();
    AstExprPtr parseBitwiseXor();
    AstExprPtr parseBitwiseAnd();
    AstExprPtr parseEquality();
    AstExprPtr parseRelational();
    AstExprPtr parseShift();
    AstExprPtr parseAdditive();
    AstExprPtr parseMultiplicative();
    AstExprPtr parseUnary();
    AstExprPtr parsePostfix(AstExprPtr base);
    AstExprPtr parsePrimary();


    void consumeSemicolon(const std::string& ctx);
};

}
