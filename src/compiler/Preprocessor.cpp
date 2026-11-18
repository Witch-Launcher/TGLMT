#include "tglmt/compiler/Preprocessor.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <unordered_map>

namespace tglmt::compiler {
namespace {

bool isIdentChar(char c) {
    return c == '_' || std::isalpha(static_cast<unsigned char>(c)) ||
           std::isdigit(static_cast<unsigned char>(c));
}
bool isIdentStart(char c) {
    return c == '_' || std::isalpha(static_cast<unsigned char>(c));
}
std::string trimStr(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r')) ++b;
    size_t e = s.size();
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

struct IfExprParser {
    const std::string& s;
    int pos = 0;
    bool ok = true;
    std::string err;
    const std::unordered_map<std::string, Macro>* macros = nullptr;

    explicit IfExprParser(const std::string& str) : s(str) {}

    void skipWS() {
        while (pos < static_cast<int>(s.size()) &&
               (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r')) ++pos;
    }
    bool matchTok(const char* tok) {
        skipWS();
        size_t n = strlen(tok);
        if (pos + (int)n <= static_cast<int>(s.size()) && s.compare(pos, n, tok) == 0) {
            pos += (int)n;
            return true;
        }
        return false;
    }
    long long readInt() {
        skipWS();
        int b = pos;
        bool hex = false;
        if (pos < static_cast<int>(s.size()) && s[pos] == '0' && pos + 1 < static_cast<int>(s.size()) &&
            (s[pos + 1] == 'x' || s[pos + 1] == 'X')) { hex = true; pos += 2; }
        while (pos < static_cast<int>(s.size())) {
            char c = s[pos];
            if (hex ? std::isxdigit(static_cast<unsigned char>(c))
                    : std::isdigit(static_cast<unsigned char>(c))) ++pos;
            else break;
        }
        if (pos == b) { ok = false; err = "so nguyen muon doi"; return 0; }
        return std::strtoll(s.substr(b, pos - b).c_str(), nullptr, hex ? 16 : 10);
    }
    std::string readIdent() {
        skipWS();
        int b = pos;
        while (pos < static_cast<int>(s.size()) &&
               (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_')) ++pos;
        if (pos == b) { ok = false; err = "identifier muon doi"; return ""; }
        return s.substr(b, pos - b);
    }
    long long parseOr();
    long long parseAnd() {
        long long v = parseEquality();
        while (ok && matchTok("&&")) { v = v && parseEquality(); }
        return v;
    }
    long long parseEquality() {
        long long v = parseRelational();
        while (ok) {
            if (matchTok("==")) v = (v == parseRelational());
            else if (matchTok("!=")) v = (v != parseRelational());
            else break;
        }
        return v;
    }
    long long parseRelational() {
        long long v = parseAdditive();
        while (ok) {
            if (matchTok("<=")) v = (v <= parseAdditive());
            else if (matchTok(">=")) v = (v >= parseAdditive());
            else if (matchTok("<")) v = (v < parseAdditive());
            else if (matchTok(">")) v = (v > parseAdditive());
            else break;
        }
        return v;
    }
    long long parseAdditive() {
        long long v = parseMultiplic();
        while (ok) {
            if (matchTok("+")) v = v + parseMultiplic();
            else if (matchTok("-")) v = v - parseMultiplic();
            else break;
        }
        return v;
    }
    long long parseMultiplic() {
        long long v = parseUnary();
        while (ok) {
            if (matchTok("*")) v = v * parseUnary();
            else if (matchTok("/")) v = v / parseUnary();
            else if (matchTok("%")) v = v % parseUnary();
            else break;
        }
        return v;
    }
    long long parseUnary() {
        if (matchTok("!")) return !parseUnary();
        if (matchTok("-")) return -parseUnary();
        if (matchTok("+")) return parseUnary();
        if (matchTok("~")) return ~parseUnary();
        return parsePrimary();
    }
    long long parsePrimary() {
        skipWS();
        if (pos >= static_cast<int>(s.size())) { ok = false; err = "bieu thuc chua ket thuc"; return 0; }
        if (s[pos] == '(') {
            ++pos;
            long long v = parseOr();
            if (!ok) return 0;
            skipWS();
            if (pos >= static_cast<int>(s.size()) || s[pos] != ')') {
                ok = false; err = "')' muon doi"; return 0;
            }
            ++pos;
            return v;
        }
        if (matchTok("defined")) {
            skipWS();
            bool paren = false;
            if (pos < static_cast<int>(s.size()) && s[pos] == '(') { paren = true; ++pos; }
            std::string name = readIdent();
            if (!ok) return 0;
            long long v = 0;
            if (macros && macros->count(name)) v = 1;
            if (paren) { skipWS(); if (pos < static_cast<int>(s.size()) && s[pos] == ')') ++pos; }
            return v;
        }
        if (isIdentStart(s[pos])) {
            readIdent();
            return 0; 
        }
        return readInt();
    }
    long long evaluate() { long long v = parseOr(); if (!ok) return 0; return v; }
};

long long IfExprParser::parseOr() {
    long long v = parseAnd();
    while (ok && matchTok("||")) { v = v || parseAnd(); }
    return v;
}
} 

Preprocessor::Preprocessor(std::string source, std::string fileName)
    : source_(std::move(source)), fileName_(std::move(fileName)) {
    condActive_.push_back(true);
    condSeenTrue_.push_back(false);
    condInElse_.push_back(false);
}

int Preprocessor::lineOf(int offset) const {
    int line = 1;
    for (int i = 0; i < offset && i < static_cast<int>(source_.size()); ++i)
        if (source_[i] == '\n') ++line;
    return line;
}

int Preprocessor::colOf(int offset) const {
    int col = 1;
    for (int i = 0; i < offset && i < static_cast<int>(source_.size()); ++i) {
        if (source_[i] == '\n') col = 1;
        else ++col;
    }
    return col;
}

SourceLocation Preprocessor::locAt(int offset) const {
    SourceLocation loc;
    loc.file = fileName_;
    loc.line = lineOf(offset);
    loc.column = colOf(offset);
    loc.offset = offset;
    return loc;
}

void Preprocessor::addDiag(DiagnosticSeverity sev, const std::string& code,
                           int offset, const std::string& msg) {
    Diagnostic d;
    d.severity = sev;
    d.code = code;
    d.message = msg;
    d.span = SourceSpan{locAt(offset), locAt(offset)};
    d.span->begin = locAt(offset);
    d.span->end = locAt(offset);
    diagnostics_.push_back(d);
}

bool Preprocessor::currentActive() const {
    if (condActive_.empty()) return true;
    return condActive_.back();
}

bool Preprocessor::enclosingActive() const {
    if (condActive_.size() <= 1) return true; 
    return condActive_[condActive_.size() - 2];
}

bool Preprocessor::isDirectiveStart(int offset) const {
    if (offset >= static_cast<int>(source_.size()) || source_[offset] != '#')
        return false;
    if (offset == 0) return true;
    char prev = source_[offset - 1];
    return prev == '\n' || prev == '\r';
}

DirectiveAction Preprocessor::classifyDirective(const std::string& name) {
    if (name == "version" || name == "extension" || name == "line" ||
        name == "pragma" || name == "import")
        return DirectiveAction::Keep;
    if (name == "define" || name == "undef" || name == "if" || name == "ifdef" ||
        name == "ifndef" || name == "else" || name == "elif" || name == "endif")
        return DirectiveAction::Drop;
    return DirectiveAction::Error;
}

Preprocessor::ExprResult Preprocessor::evalIfExpression(const std::string& expr, int startOffset) {
    ExprResult er;
    if (expr.empty()) { er.ok = false; return er; }
    IfExprParser parser(expr);
    parser.macros = &macros_;
    long long v = parser.evaluate();
    er.ok = parser.ok;
    er.value = v;
    if (!parser.ok) {
        addDiag(DiagnosticSeverity::Error, "PP-006", startOffset,
                "#if bieu thuc khong hop le: " + parser.err);
    }
    return er;
}

std::string Preprocessor::readSourceIdent(int& pos) const {
    int b = pos;
    while (pos < static_cast<int>(source_.size()) && isIdentChar(source_[pos])) ++pos;
    return source_.substr(b, pos - b);
}

void Preprocessor::defineMacro(int pos, PreprocessResult& result) {
    int p = pos;
    while (p < static_cast<int>(source_.size()) &&
           (source_[p] == ' ' || source_[p] == '\t')) ++p;
    std::string name = readSourceIdent(p);
    if (name.empty()) {
        addDiag(DiagnosticSeverity::Error, "PP-007", pos, "#define thieu ten macro");
        result.success = false;
        return;
    }
    Macro m;
    m.name = name;
    m.defLoc = locAt(pos);
    
    
    int nameEnd = static_cast<int>(p);
    bool isFunctionLike = (nameEnd < static_cast<int>(source_.size()) &&
                           source_[nameEnd] == '(');
    while (p < static_cast<int>(source_.size()) &&
           (source_[p] == ' ' || source_[p] == '\t')) ++p;
    if (isFunctionLike) {
        m.isObjectLike = false;
        ++p; 
        while (p < static_cast<int>(source_.size()) && source_[p] != ')') {
            while (p < static_cast<int>(source_.size()) &&
                   (source_[p] == ' ' || source_[p] == '\t')) ++p;
            std::string param = readSourceIdent(p);
            if (!param.empty()) m.params.push_back(param);
            while (p < static_cast<int>(source_.size()) &&
                   (source_[p] == ' ' || source_[p] == '\t')) ++p;
            if (p < static_cast<int>(source_.size()) && source_[p] == ',') ++p;
        }
        if (p < static_cast<int>(source_.size()) && source_[p] == ')') ++p;
    }
    while (p < static_cast<int>(source_.size()) &&
           (source_[p] == ' ' || source_[p] == '\t')) ++p;
    int eol = p;
    while (eol < static_cast<int>(source_.size()) && source_[eol] != '\n') ++eol;
    m.replacement = source_.substr(p, eol - p);
    macros_[name] = m;
}

void Preprocessor::undefMacro(int pos) {
    int p = pos;
    while (p < static_cast<int>(source_.size()) &&
           (source_[p] == ' ' || source_[p] == '\t')) ++p;
    std::string name = readSourceIdent(p);
    macros_.erase(name);
}

bool Preprocessor::expandMacros(const std::string& line, std::string& out) {
    out = line;
    for (auto& kv : macros_) {
        if (kv.second.isObjectLike) {
            std::string token = kv.second.name;
            size_t p = 0;
            while ((p = out.find(token, p)) != std::string::npos) {
                bool leftOk = (p == 0 || !isIdentChar(out[p - 1]));
                bool rightOk = (p + token.size() >= out.size() || !isIdentChar(out[p + token.size()]));
                if (leftOk && rightOk) {
                    out.replace(p, token.size(), kv.second.replacement);
                    p += kv.second.replacement.size();
                } else {
                    p += token.size();
                }
            }
        }
    }
    return true;
}

PreprocessResult Preprocessor::run() {
    PreprocessResult result;
    result.success = true;

    std::ostringstream out;
    int pos = 0;

    while (pos < static_cast<int>(source_.size())) {
        int lineEnd = pos;
        while (lineEnd < static_cast<int>(source_.size()) && source_[lineEnd] != '\n') ++lineEnd;
        bool hasNewline = (lineEnd < static_cast<int>(source_.size()));

        std::string line = source_.substr(pos, lineEnd - pos);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        int d = 0;
        while (d < static_cast<int>(line.size()) && (line[d] == ' ' || line[d] == '\t')) ++d;
        bool atDirective = (d < static_cast<int>(line.size()) && line[d] == '#');

        if (atDirective) {
            int directiveStart = pos + d;
            int p = directiveStart + 1;
            while (p < static_cast<int>(source_.size()) &&
                   (source_[p] == ' ' || source_[p] == '\t')) ++p;
            std::string name = readSourceIdent(p);

            int eol = p;
            while (eol < static_cast<int>(source_.size()) && source_[eol] != '\n') ++eol;
            std::string rest = trimStr(source_.substr(p, eol - p));

            DirectiveAction action = classifyDirective(name);
            bool parentActive = enclosingActive();

            if (name == "if") {
                ExprResult er = evalIfExpression(rest, directiveStart);
                bool active = parentActive && er.ok && er.value != 0;
                condActive_.push_back(active);
                condSeenTrue_.push_back(er.ok && er.value != 0);
                condInElse_.push_back(false);
            } else if (name == "ifdef") {
                bool defined = macros_.count(rest) > 0;
                bool active = parentActive && defined;
                condActive_.push_back(active);
                condSeenTrue_.push_back(defined);
                condInElse_.push_back(false);
            } else if (name == "ifndef") {
                bool defined = macros_.count(rest) > 0;
                bool active = parentActive && !defined;
                condActive_.push_back(active);
                condSeenTrue_.push_back(!defined);
                condInElse_.push_back(false);
            } else if (name == "else") {
                if (condActive_.size() > 1) {
                    if (condInElse_.back()) {
                        addDiag(DiagnosticSeverity::Error, "PP-003", directiveStart, "#else lap lai");
                        result.success = false;
                    } else {
                        condInElse_.back() = true;
                        bool seen = condSeenTrue_.back();
                        condActive_.back() = parentActive && !seen;
                    }
                } else {
                    addDiag(DiagnosticSeverity::Error, "PP-008", directiveStart, "#else khong co #if");
                    result.success = false;
                }
            } else if (name == "elif") {
                if (condActive_.size() > 1) {
                    if (condInElse_.back()) {
                        addDiag(DiagnosticSeverity::Error, "PP-004", directiveStart, "#elif sau #else");
                        result.success = false;
                    } else {
                        ExprResult er = evalIfExpression(rest, directiveStart);
                        bool matched = er.ok && er.value != 0;
                        bool anyBefore = condSeenTrue_.back();
                        condSeenTrue_.back() = anyBefore || matched;
                        condActive_.back() = parentActive && matched && !anyBefore;
                    }
                } else {
                    addDiag(DiagnosticSeverity::Error, "PP-009", directiveStart, "#elif khong co #if");
                    result.success = false;
                }
            } else if (name == "endif") {
                if (condActive_.size() > 1) {
                    condActive_.pop_back();
                    condSeenTrue_.pop_back();
                    condInElse_.pop_back();
                } else {
                    addDiag(DiagnosticSeverity::Error, "PP-005", directiveStart, "#endif khong co #if");
                    result.success = false;
                }
            } else if (name == "define") {
                if (parentActive) defineMacro(p, result);
            } else if (name == "undef") {
                if (parentActive) undefMacro(p);
            } else if (name == "version") {
                int sp = 0;
                while (sp < static_cast<int>(rest.size()) && (rest[sp] == ' ' || rest[sp] == '\t')) ++sp;
                std::string verTok;
                while (sp < static_cast<int>(rest.size()) &&
                       (std::isdigit(static_cast<unsigned char>(rest[sp])) || rest[sp] == '.')) {
                    verTok += rest[sp++];
                }
                int ver = verTok.empty() ? 0 : std::atoi(verTok.c_str());
                result.version.major = ver / 100;
                result.version.minor = ver % 100;
                while (sp < static_cast<int>(rest.size()) && (rest[sp] == ' ' || rest[sp] == '\t')) ++sp;
                std::string prof;
                while (sp < static_cast<int>(rest.size()) &&
                       std::isalpha(static_cast<unsigned char>(rest[sp]))) { prof += rest[sp++]; }
                if (prof == "compatibility") result.profile = GlslProfile::Compatibility;
                else if (prof == "core") result.profile = GlslProfile::Core;
                else result.profile = GlslProfile::Legacy;
                if (parentActive) out << line << "\n";
            } else if (name == "extension") {
                int sp = 0;
                while (sp < static_cast<int>(rest.size()) && (rest[sp] == ' ' || rest[sp] == '\t')) ++sp;
                std::string extName;
                while (sp < static_cast<int>(rest.size()) && rest[sp] != ':') { extName += rest[sp++]; }
                extName = trimStr(extName);
                while (sp < static_cast<int>(rest.size()) && rest[sp] != ':') ++sp;
                if (sp < static_cast<int>(rest.size()) && rest[sp] == ':') ++sp;
                while (sp < static_cast<int>(rest.size()) && (rest[sp] == ' ' || rest[sp] == '\t')) ++sp;
                std::string behavior;
                while (sp < static_cast<int>(rest.size()) &&
                       std::isalpha(static_cast<unsigned char>(rest[sp]))) { behavior += rest[sp++]; }
                if (!extName.empty()) {
                    bool enable = (behavior == "enable" || behavior == "required");
                    result.extensions.enabled[extName] = enable;
                }
                if (parentActive) out << line << "\n";
            } else if (name == "line" || name == "pragma" || name == "import") {
                if (parentActive) out << line << "\n";
            } else {
                addDiag(DiagnosticSeverity::Error, "PP-002", directiveStart,
                        "directive khong ho tro: #" + name);
                result.success = false;
            }

            pos = eol;
            if (hasNewline) ++pos;
        } else {
            if (currentActive()) {
                std::string expanded;
                expandMacros(line, expanded);
                out << expanded << "\n";
            }
            pos = lineEnd;
            if (hasNewline) ++pos;
        }
    }

    if (condActive_.size() > 1) {
        addDiag(DiagnosticSeverity::Error, "PP-001", 0,
                "#if/#ifdef/#ifndef khong co #endif tuong ung");
        result.success = false;
    }

    result.source = out.str();
    result.diagnostics = diagnostics_;
    return result;
}

} 
