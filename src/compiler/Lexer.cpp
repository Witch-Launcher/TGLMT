#include "tglmt/compiler/Lexer.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <cstring>

namespace tglmt::compiler {

Lexer::Lexer(std::string source, std::string fileName)
    : source_(std::move(source)), fileName_(std::move(fileName)) {}

char Lexer::peek(int ahead) const {
    size_t p = pos_ + static_cast<size_t>(ahead);
    if (p >= source_.size()) return '\0';
    return source_[p];
}

char Lexer::advance() {
    char c = peek();
    if (c == '\0') return '\0';
    ++pos_;
    ++offset_;
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Lexer::skipComment() {
    char c1 = peek();
    char c2 = peek(1);
    if (c1 == '/' && c2 == '/') {
        while (true) {
            char c = peek();
            if (c == '\0' || c == '\n') break;
            advance();
        }
    } else if (c1 == '/' && c2 == '*') {
        advance(); advance();
        int depth = 1;
        while (true) {
            char c = peek();
            if (c == '\0') {
                recordError(static_cast<int>(pos_), "unterminated block comment");
                return;
            }
            if (c == '/' && peek(1) == '*') { advance(); advance(); ++depth; }
            else if (c == '*' && peek(1) == '/') { advance(); advance(); --depth; if (depth == 0) break; }
            else advance();
        }
    }
}

Token Lexer::makeToken(TokenType type, int start, int end) const {
    Token tok;
    tok.type = type;
    tok.lexeme = source_.substr(static_cast<size_t>(start),
                                static_cast<size_t>(end - start));
    tok.location = locationAt(start);
    tok.offset = start;
    return tok;
}

SourceLocation Lexer::locationAt(int offset) const {
    if (!lineIndexBuilt_) {
        lineStarts_.clear();
        lineStarts_.push_back(0);
        for (size_t i = 0; i < source_.size(); ++i)
            if (source_[i] == '\n') lineStarts_.push_back(static_cast<int>(i) + 1);
        lineIndexBuilt_ = true;
    }
    auto it = std::upper_bound(lineStarts_.begin(), lineStarts_.end(), offset);
    size_t idx = static_cast<size_t>(it - lineStarts_.begin());
    if (idx == 0) idx = 1;
    SourceLocation loc;
    loc.file = fileName_;
    loc.line = static_cast<int>(idx);
    loc.column = offset - lineStarts_[idx - 1] + 1;
    loc.offset = offset;
    return loc;
}

bool Lexer::isIdentStart(char c) const {
    return c == '_' || std::isalpha(static_cast<unsigned char>(c));
}

bool Lexer::isIdentChar(char c) const {
    return c == '_' || std::isalnum(static_cast<unsigned char>(c));
}

bool Lexer::isDigit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool Lexer::isHexDigit(char c) const {
    return std::isxdigit(static_cast<unsigned char>(c));
}

void Lexer::recordError(int start, const std::string& msg) {
    if (hasError_) return;
    hasError_ = true;
    errorMessage_ = msg;

}

static bool IsKeywordIdent(std::string_view s, TokenType& out) {
    static const struct { const char* word; TokenType type; } kKeywords[] = {
        {"attribute", TokenType::Kw_Attribute},
        {"const", TokenType::Kw_Const},
        {"uniform", TokenType::Kw_Uniform},
        {"buffer", TokenType::Kw_Buffer},
        {"varying", TokenType::Kw_Varying},
        {"in", TokenType::Kw_In},
        {"out", TokenType::Kw_Out},
        {"inout", TokenType::Kw_InOut},
        {"patch", TokenType::Kw_Patch},
        {"centroid", TokenType::Kw_Centroid},
        {"sample", TokenType::Kw_Sample},
        {"flat", TokenType::Kw_Flat},
        {"smooth", TokenType::Kw_Smooth},
        {"noperspective", TokenType::Kw_NoPerspective},
        {"invariant", TokenType::Kw_Invariant},
        {"precise", TokenType::Kw_Precise},
        {"shared", TokenType::Kw_Shared},
        {"readonly", TokenType::Kw_ReadOnly},
        {"writeonly", TokenType::Kw_WriteOnly},
        {"coherent", TokenType::Kw_Coherent},
        {"volatile", TokenType::Kw_Volatile},
        {"restrict", TokenType::Kw_Restrict},
        {"void", TokenType::Kw_Void},
        {"float", TokenType::Kw_Float},
        {"double", TokenType::Kw_Double},
        {"int", TokenType::Kw_Int},
        {"uint", TokenType::Kw_Uint},
        {"bool", TokenType::Kw_Bool},
        {"vec2", TokenType::Kw_Vec2},
        {"vec3", TokenType::Kw_Vec3},
        {"vec4", TokenType::Kw_Vec4},
        {"ivec2", TokenType::Kw_Ivec2},
        {"ivec3", TokenType::Kw_Ivec3},
        {"ivec4", TokenType::Kw_Ivec4},
        {"uvec2", TokenType::Kw_Uvec2},
        {"uvec3", TokenType::Kw_Uvec3},
        {"uvec4", TokenType::Kw_Uvec4},
        {"bvec2", TokenType::Kw_Bvec2},
        {"bvec3", TokenType::Kw_Bvec3},
        {"bvec4", TokenType::Kw_Bvec4},
        {"mat2", TokenType::Kw_Mat2},
        {"mat3", TokenType::Kw_Mat3},
        {"mat4", TokenType::Kw_Mat4},
        {"dmat2", TokenType::Kw_Dmat2},
        {"dmat3", TokenType::Kw_Dmat3},
        {"dmat4", TokenType::Kw_Dmat4},
        {"mat2x2", TokenType::Kw_Mat2x2},
        {"mat2x3", TokenType::Kw_Mat2x3},
        {"mat2x4", TokenType::Kw_Mat2x4},
        {"mat3x2", TokenType::Kw_Mat3x2},
        {"mat3x3", TokenType::Kw_Mat3x3},
        {"mat3x4", TokenType::Kw_Mat3x4},
        {"mat4x2", TokenType::Kw_Mat4x2},
        {"mat4x3", TokenType::Kw_Mat4x3},
        {"mat4x4", TokenType::Kw_Mat4x4},
        {"sampler1D", TokenType::Kw_Sampler1D},
        {"sampler2D", TokenType::Kw_Sampler2D},
        {"sampler3D", TokenType::Kw_Sampler3D},
        {"samplerCube", TokenType::Kw_SamplerCube},
        {"sampler1DShadow", TokenType::Kw_Sampler1DShadow},
        {"sampler2DShadow", TokenType::Kw_Sampler2DShadow},
        {"sampler2DRect", TokenType::Kw_Sampler2DRect},
        {"sampler2DRectShadow", TokenType::Kw_Sampler2DRectShadow},
        {"sampler1DArray", TokenType::Kw_Sampler1DArray},
        {"sampler2DArray", TokenType::Kw_Sampler2DArray},
        {"samplerBuffer", TokenType::Kw_SamplerBuffer},
        {"sampler2DMS", TokenType::Kw_Sampler2DMS},
        {"sampler2DMSArray", TokenType::Kw_Sampler2DMSArray},
        {"samplerCubeShadow", TokenType::Kw_SamplerCubeShadow},
        {"isampler1D", TokenType::Kw_Isampler1D},
        {"isampler2D", TokenType::Kw_Isampler2D},
        {"isampler3D", TokenType::Kw_Isampler3D},
        {"isamplerCube", TokenType::Kw_IsamplerCube},
        {"isampler2DRect", TokenType::Kw_Isampler2DRect},
        {"isampler1DArray", TokenType::Kw_Isampler1DArray},
        {"isampler2DArray", TokenType::Kw_Isampler2DArray},
        {"isamplerBuffer", TokenType::Kw_IsamplerBuffer},
        {"usampler1D", TokenType::Kw_Usampler1D},
        {"usampler2D", TokenType::Kw_Usampler2D},
        {"usampler3D", TokenType::Kw_Usampler3D},
        {"usamplerCube", TokenType::Kw_UsamplerCube},
        {"usampler2DRect", TokenType::Kw_Usampler2DRect},
        {"usampler1DArray", TokenType::Kw_Usampler1DArray},
        {"usampler2DArray", TokenType::Kw_Usampler2DArray},
        {"usamplerBuffer", TokenType::Kw_UsamplerBuffer},
        {"image1D", TokenType::Kw_Image1D},
        {"image2D", TokenType::Kw_Image2D},
        {"image3D", TokenType::Kw_Image3D},
        {"image2DRect", TokenType::Kw_Image2DRect},
        {"imageCube", TokenType::Kw_ImageCube},
        {"image2DArray", TokenType::Kw_Image2DArray},
        {"imageBuffer", TokenType::Kw_ImageBuffer},
        {"atomic_uint", TokenType::Kw_AtomicUint},
        {"struct", TokenType::Kw_Struct},
        {"if", TokenType::Kw_If},
        {"else", TokenType::Kw_Else},
        {"for", TokenType::Kw_For},
        {"while", TokenType::Kw_While},
        {"do", TokenType::Kw_Do},
        {"switch", TokenType::Kw_Switch},
        {"case", TokenType::Kw_Case},
        {"default", TokenType::Kw_Default},
        {"break", TokenType::Kw_Break},
        {"continue", TokenType::Kw_Continue},
        {"return", TokenType::Kw_Return},
        {"discard", TokenType::Kw_Discard},
        {"subroutine", TokenType::Kw_Subroutine},
        {"layout", TokenType::Kw_Layout},
        {"true", TokenType::Kw_True},
        {"false", TokenType::Kw_False},

        {"gl_Position", TokenType::Kw_Gl_Position},
        {"gl_PointSize", TokenType::Kw_Gl_PointSize},
        {"gl_FragCoord", TokenType::Kw_Gl_FragCoord},
        {"gl_FrontFacing", TokenType::Kw_Gl_FrontFacing},
        {"gl_PointCoord", TokenType::Kw_Gl_PointCoord},
        {"gl_VertexID", TokenType::Kw_Gl_VertexID},
        {"gl_InstanceID", TokenType::Kw_Gl_InstanceID},
        {"gl_DrawID", TokenType::Kw_Gl_DrawID},
        {"gl_BaseVertex", TokenType::Kw_Gl_BaseVertex},
        {"gl_BaseInstance", TokenType::Kw_Gl_BaseInstance},
        {"gl_ClipDistance", TokenType::Kw_Gl_ClipDistance},
        {"gl_PrimitiveID", TokenType::Kw_Gl_PrimitiveID},
        {"gl_InvocationID", TokenType::Kw_Gl_InvocationID},
        {"gl_PrimitiveIDIn", TokenType::Kw_Gl_PrimitiveIDIn},
        {"gl_Layer", TokenType::Kw_Gl_Layer},
        {"gl_ViewportIndex", TokenType::Kw_Gl_ViewportIndex},
        {"gl_TessLevelOuter", TokenType::Kw_Gl_TessLevelOuter},
        {"gl_TessLevelInner", TokenType::Kw_Gl_TessLevelInner},
        {"gl_TessCoord", TokenType::Kw_Gl_TessCoord},
        {"gl_PatchVerticesIn", TokenType::Kw_Gl_PatchVerticesIn},
        {"gl_PatchSize", TokenType::Kw_Gl_PatchSize},
        {"gl_PatchSizeIn", TokenType::Kw_Gl_PatchSizeIn},
        {"gl_in", TokenType::Kw_Gl_In},
        {"gl_out", TokenType::Kw_Gl_Out},
        {nullptr, TokenType::Error},
    };
    for (const auto& kw : kKeywords) {
        if (kw.word == nullptr) break;
        if (s == kw.word) { out = kw.type; return true; }
    }
    return false;
}

Token Lexer::scanIdentOrKeyword(int start) {
    while (isIdentChar(peek())) advance();
    int end = static_cast<int>(pos_);
    std::string_view word(source_.data() + start, static_cast<size_t>(end - start));
    TokenType type;
    if (IsKeywordIdent(word, type)) return makeToken(type, start, end);
    return makeToken(TokenType::Ident, start, end);
}

Token Lexer::scanString(int start) {

    advance();
    while (true) {
        char c = peek();
        if (c == '\0') {
            recordError(start, "unterminated string literal");
            return makeToken(TokenType::Error, start, static_cast<int>(pos_));
        }
        if (c == '\n') {
            recordError(start, "unterminated string literal");
            return makeToken(TokenType::Error, start, static_cast<int>(pos_));
        }
        if (c == '\\') { advance(); if (peek() != '\0') advance(); continue; }
        if (c == '"') { advance(); break; }
        advance();
    }
    int end = static_cast<int>(pos_);
    return makeToken(TokenType::StringLiteral, start, end);
}

Token Lexer::scanNumber(int start) {
    bool isFloat = false;
    bool isHex = false;
    bool isUnsigned = false;
    bool isDouble = false;

    if (source_[start] == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
        isHex = true;
        advance(); advance();
        while (isHexDigit(peek())) advance();
    } else {
        while (isDigit(peek())) advance();
        if (peek() == '.') { isFloat = true; advance(); while (isDigit(peek())) advance(); }
        if (peek() == 'e' || peek() == 'E') { isFloat = true; advance();
            if (peek() == '+' || peek() == '-') advance();
            while (isDigit(peek())) advance(); }
    }

    char c = peek();
    if (c == 'u' || c == 'U') { isUnsigned = true; advance(); c = peek(); }
    if (c == 'l' || c == 'L') { advance(); c = peek(); }
    if (c == 'f' || c == 'F') { isFloat = true; advance(); c = peek(); }
    if (c == 'd' || c == 'D') { isDouble = true; advance(); }
    if (isDouble) return makeToken(TokenType::DoubleLiteral, start, static_cast<int>(pos_));
    if (isFloat) return makeToken(TokenType::FloatLiteral, start, static_cast<int>(pos_));
    return isUnsigned ? makeToken(TokenType::UintLiteral, start, static_cast<int>(pos_))
                      : makeToken(TokenType::IntLiteral, start, static_cast<int>(pos_));
}

Token Lexer::scanPunctOrOperator(int start) {
    char c = peek();

    char c2 = peek(1);
    std::string two = std::string(1, c) + c2;
    if (two == "++") { advance(); advance(); return makeToken(TokenType::Increment, start, static_cast<int>(pos_)); }
    if (two == "--") { advance(); advance(); return makeToken(TokenType::Decrement, start, static_cast<int>(pos_)); }
    if (two == "+=") { advance(); advance(); return makeToken(TokenType::PlusEqual, start, static_cast<int>(pos_)); }
    if (two == "-=") { advance(); advance(); return makeToken(TokenType::MinusEqual, start, static_cast<int>(pos_)); }
    if (two == "*=") { advance(); advance(); return makeToken(TokenType::StarEqual, start, static_cast<int>(pos_)); }
    if (two == "/=") { advance(); advance(); return makeToken(TokenType::SlashEqual, start, static_cast<int>(pos_)); }
    if (two == "%=") { advance(); advance(); return makeToken(TokenType::PercentEqual, start, static_cast<int>(pos_)); }
    if (two == "&&") { advance(); advance(); return makeToken(TokenType::AmpAmp, start, static_cast<int>(pos_)); }
    if (two == "||") { advance(); advance(); return makeToken(TokenType::BarBar, start, static_cast<int>(pos_)); }
    if (two == "^^") { advance(); advance(); return makeToken(TokenType::CaretCaret, start, static_cast<int>(pos_)); }
    if (two == "&=") { advance(); advance(); return makeToken(TokenType::AmpersandEqual, start, static_cast<int>(pos_)); }
    if (two == "|=") { advance(); advance(); return makeToken(TokenType::BarEqual, start, static_cast<int>(pos_)); }
    if (two == "^=") { advance(); advance(); return makeToken(TokenType::CaretEqual, start, static_cast<int>(pos_)); }
    if (two == "<<") { advance(); advance(); return makeToken(TokenType::LeftShift, start, static_cast<int>(pos_)); }
    if (two == ">>") { advance(); advance(); return makeToken(TokenType::RightShift, start, static_cast<int>(pos_)); }
    if (two == "<<=") { advance(); advance(); advance(); return makeToken(TokenType::LeftShiftEqual, start, static_cast<int>(pos_)); }
    if (two == ">>=") { advance(); advance(); advance(); return makeToken(TokenType::RightShiftEqual, start, static_cast<int>(pos_)); }
    if (two == "==") { advance(); advance(); return makeToken(TokenType::EqualEqual, start, static_cast<int>(pos_)); }
    if (two == "!=") { advance(); advance(); return makeToken(TokenType::NotEqual, start, static_cast<int>(pos_)); }
    if (two == "<=") { advance(); advance(); return makeToken(TokenType::LessEqual, start, static_cast<int>(pos_)); }
    if (two == ">=") { advance(); advance(); return makeToken(TokenType::GreaterEqual, start, static_cast<int>(pos_)); }
    if (two == "->") { advance(); advance(); return makeToken(TokenType::Arrow, start, static_cast<int>(pos_)); }

    advance();
    int end = static_cast<int>(pos_);
    switch (c) {
        case '(': return makeToken(TokenType::LeftParen, start, end);
        case ')': return makeToken(TokenType::RightParen, start, end);
        case '[': return makeToken(TokenType::LeftBracket, start, end);
        case ']': return makeToken(TokenType::RightBracket, start, end);
        case '{': return makeToken(TokenType::LeftBrace, start, end);
        case '}': return makeToken(TokenType::RightBrace, start, end);
        case '.': return makeToken(TokenType::Dot, start, end);
        case ',': return makeToken(TokenType::Comma, start, end);
        case ':': return makeToken(TokenType::Colon, start, end);
        case ';': return makeToken(TokenType::Semicolon, start, end);
        case '=': return makeToken(TokenType::Equal, start, end);
        case '+': return makeToken(TokenType::Plus, start, end);
        case '-': return makeToken(TokenType::Minus, start, end);
        case '*': return makeToken(TokenType::Star, start, end);
        case '/': return makeToken(TokenType::Slash, start, end);
        case '%': return makeToken(TokenType::Percent, start, end);
        case '&': return makeToken(TokenType::Ampersand, start, end);
        case '|': return makeToken(TokenType::Bar, start, end);
        case '^': return makeToken(TokenType::Caret, start, end);
        case '~': return makeToken(TokenType::Tilde, start, end);
        case '!': return makeToken(TokenType::Bang, start, end);
        case '<': return makeToken(TokenType::Less, start, end);
        case '>': return makeToken(TokenType::Greater, start, end);
        case '?': return makeToken(TokenType::Question, start, end);
        default: recordError(start, std::string("không nhận ký tự: '") + c + "'");
                 return makeToken(TokenType::Error, start, end);
    }
}

Token Lexer::scanDirective(int start) {
    advance();

    while (peek() == ' ' || peek() == '\t') advance();
    int nameStart = static_cast<int>(pos_);
    while (isIdentChar(peek())) advance();
    std::string name = source_.substr(nameStart, pos_ - nameStart);
    TokenType type = TokenType::Error;
    if (name == "version") type = TokenType::Pp_Version;
    else if (name == "extension") type = TokenType::Pp_Extension;
    else if (name == "line") type = TokenType::Pp_Line;
    else if (name == "pragma") type = TokenType::Pp_Pragma;
    else if (name == "import") type = TokenType::Pp_Import;
    else if (name == "define") type = TokenType::Pp_Define;
    else if (name == "undef") type = TokenType::Pp_Undef;
    else if (name == "if") type = TokenType::Pp_If;
    else if (name == "ifdef") type = TokenType::Pp_Ifdef;
    else if (name == "ifndef") type = TokenType::Pp_Ifndef;
    else if (name == "else") type = TokenType::Pp_Else;
    else if (name == "elif") type = TokenType::Pp_Elif;
    else if (name == "endif") type = TokenType::Pp_Endif;

    while (peek() != '\n' && peek() != '\0') advance();
    return makeToken(type, start, static_cast<int>(pos_));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(source_.size() / 3 + 16);
    while (true) {
        for (;;) {
            skipWhitespace();
            if (peek() == '/' && (peek(1) == '/' || peek(1) == '*')) { skipComment(); continue; }
            break;
        }
        char c = peek();
        if (c == '\0') break;
        int start = static_cast<int>(pos_);
        if (c == '#') {
            tokens.push_back(scanDirective(start));
            continue;
        }
        if (c == '"') { tokens.push_back(scanString(start)); continue; }
        if (isDigit(c) || (c == '.' && isDigit(peek(1)))) { tokens.push_back(scanNumber(start)); continue; }
        if (isIdentStart(c)) { tokens.push_back(scanIdentOrKeyword(start)); continue; }
        tokens.push_back(scanPunctOrOperator(start));
    }
    tokens.push_back(makeToken(TokenType::EOF_Token, static_cast<int>(pos_), static_cast<int>(pos_)));
    return tokens;
}

}
