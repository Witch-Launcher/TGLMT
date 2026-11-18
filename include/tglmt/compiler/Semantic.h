#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>
#include <set>

#include "tglmt/compiler/AST.h"

namespace tglmt::compiler {

enum class ResolvedScalar {
    Void, Float, Double, Int, UInt, Bool
};

enum class ResolvedKind {
    Scalar, Vector, Matrix, Array, Struct, Sampler, Image, AtomicCounter, Opaque
};

struct ResolvedType {
    ResolvedKind kind = ResolvedKind::Scalar;
    ResolvedScalar scalar = ResolvedScalar::Void;
    int vectorSize = 0;          
    int matrixCols = 0, matrixRows = 0;
    bool matrixRowMajor = false;
    std::shared_ptr<ResolvedType> arrayElement; 
    int arraySize = 0;           
    std::string structName;      
    bool isSampler = false;
    bool isImage = false;
    bool isShadow = false;
    int imageDim = 0;
    int samplerDim = 0;
    ResolvedScalar samplerScalar = ResolvedScalar::Float;
    bool isBufferTex = false;    

    ResolvedType() = default;
    ResolvedType(ResolvedKind k, ResolvedScalar s, int vs=0, int mc=0, int mr=0)
        : kind(k), scalar(s), vectorSize(vs), matrixCols(mc), matrixRows(mr) {}

    bool isNumeric() const;
    bool isInteger() const;
    bool isUnsigned() const;
    bool isFloat() const;
    bool isBool() const;
    bool isScalar() const { return kind == ResolvedKind::Scalar; }
    bool isVector() const { return kind == ResolvedKind::Vector; }
    bool isMatrix() const { return kind == ResolvedKind::Matrix; }
    bool isArray() const { return kind == ResolvedKind::Array; }
    bool isStruct() const { return kind == ResolvedKind::Struct; }
    bool isOpaque() const { return isSampler || isImage || kind == ResolvedKind::AtomicCounter; }
    std::string toString() const;
    bool operator==(const ResolvedType& o) const;
    bool operator!=(const ResolvedType& o) const { return !(*this == o); }
};

enum class ConversionRank {
    Exact,          
    Promotion,      
    Conversion,     
    None            
};
ConversionRank RankConversion(const ResolvedType& from, const ResolvedType& to);

enum class SymbolKind {
    Variable, Function, Struct, InterfaceBlock, BuiltinFunction, BuiltinVariable
};

struct Symbol {
    std::string name;
    ResolvedType type;
    SymbolKind kind = SymbolKind::Variable;
    std::vector<ResolvedType> funcParamTypes; 
    std::vector<StorageQualifier> qualifiers;
    std::vector<LayoutQualifier> layout;
    int location = -1;
    int binding = -1;
    SourceLocation declLoc;
    bool isBuiltin = false;
    bool isLValue = true; 
};

class SymbolTable {
public:
    void pushScope();
    void popScope();
    bool insert(const Symbol& sym);
    std::shared_ptr<Symbol> lookup(const std::string& name) const; 
    std::shared_ptr<Symbol> lookupCurrentScope(const std::string& name) const;
    std::vector<std::shared_ptr<Symbol>> lookupAll(const std::string& name) const; 
    int scopeDepth() const { return (int)scopes_.size(); }

private:
    std::vector<std::unordered_map<std::string, std::vector<std::shared_ptr<Symbol>>>> scopes_;
};

struct SemanticResult {
    bool success = false;
    std::vector<Diagnostic> diagnostics;
    
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::vector<Variable> uniforms;
    std::vector<Variable> samplers;
    size_t uniformBufferSize = 0;
    std::unordered_map<const AstExpression*, ResolvedType> exprTypes;
};

struct CheckedExpr {
    ResolvedType type;
    bool isConstant = false;
    bool isLValue = false;
    long long intValue = 0;
    double floatValue = 0.0;
    bool boolValue = false;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(ShaderStage stage, const GLSLVersion& version,
                     GlslProfile profile, const ExtensionSet& extensions);

    
    SemanticResult analyze(const AstTranslationUnit& tu);

private:
    ShaderStage stage_;
    GLSLVersion version_;
    GlslProfile profile_;
    ExtensionSet extensions_;
    SymbolTable symTab_;
    std::unordered_map<std::string, std::unordered_map<std::string, ResolvedType>> structFields_;
    std::unordered_map<const AstExpression*, ResolvedType> exprTypes_;
    std::vector<Diagnostic> diagnostics_;
    int errorCount_ = 0;

    void diagError(int offset, const std::string& code, const std::string& msg);
    void diagError(const SourceLocation& loc, const std::string& code, const std::string& msg);

    
    void checkDeclaration(const AstDeclaration& d);
    void checkVariableDeclaration(const AstDeclaration& d);
    void checkFunctionDeclaration(const AstDeclaration& d);
    void checkStructDeclaration(const AstDeclaration& d);
    void checkInterfaceBlock(const AstDeclaration& d);

    
    void checkStatement(const AstStmtPtr& s);
    void checkBlock(const AstStmtPtr& s);

    
    CheckedExpr checkExpression(const AstExprPtr& e);
    CheckedExpr checkExpressionImpl(const AstExprPtr& e);
    CheckedExpr checkBinary(const AstExprPtr& e);
    CheckedExpr checkUnary(const AstExprPtr& e);
    CheckedExpr checkCall(const AstExprPtr& e);
    CheckedExpr checkConstructor(const AstExprPtr& e);
    CheckedExpr checkMember(const AstExprPtr& e);
    CheckedExpr checkIndex(const AstExprPtr& e);
    CheckedExpr checkSwizzle(const AstExprPtr& e);
    CheckedExpr checkTernary(const AstExprPtr& e);
    CheckedExpr checkLiteral(const AstExprPtr& e);
    CheckedExpr checkIdentifier(const AstExprPtr& e);

    
    std::optional<size_t> resolveOverload(
        const std::string& name,
        const std::vector<CheckedExpr>& args,
        const std::vector<std::shared_ptr<Symbol>>& candidates,
        SourceLocation callLoc);

    
    ResolvedType resolveAstType(const AstType& ty);
    bool isAssignableTo(const ResolvedType& dst, const ResolvedType& src) const;
    bool isLValueAllowed(const ResolvedType& ty) const;

    
    void declareBuiltins();
    bool isValidBuiltinForStage(const std::string& name) const;
};

} 
