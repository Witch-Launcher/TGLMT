#pragma once







#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/compiler/Lexer.h"

namespace tglmt::compiler {

enum class DirectiveAction {
    Keep,
    Drop,
    Error,
};


struct Macro {
    std::string name;
    bool isObjectLike = true;
    std::vector<std::string> params;
    std::string replacement;
    SourceLocation defLoc;
};


struct PreprocessResult {
    bool success = false;
    std::string source;
    std::vector<Diagnostic> diagnostics;
    GLSLVersion version{};
    GlslProfile profile = GlslProfile::Core;
    ExtensionSet extensions;
};


class Preprocessor {
public:
    Preprocessor(std::string source, std::string fileName);

    PreprocessResult run();

private:
    std::string source_;
    std::string fileName_;
    std::vector<bool> condActive_;
    std::vector<bool> condSeenTrue_;
    std::vector<bool> condInElse_;
    std::unordered_map<std::string, Macro> macros_;

    std::vector<Diagnostic> diagnostics_;

    int lineOf(int offset) const;
    int colOf(int offset) const;
    SourceLocation locAt(int offset) const;

    bool isDirectiveStart(int offset) const;
    DirectiveAction classifyDirective(const std::string& name);


    struct ExprResult { bool ok = false; long long value = 0; };
    ExprResult evalIfExpression(const std::string& expr, int startOffset);


    void defineMacro(int pos, PreprocessResult& result);
    void undefMacro(int pos);
    bool expandMacros(const std::string& line, std::string& out);


    std::string readSourceIdent(int& pos) const;
    std::string trim(const std::string& s);
    void addDiag(DiagnosticSeverity sev, const std::string& code, int offset, const std::string& msg);
    bool currentActive() const;
    bool enclosingActive() const;
};

}
