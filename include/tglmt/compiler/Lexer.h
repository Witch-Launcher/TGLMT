#pragma once




#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "tglmt/compiler/ShaderIR.h"

namespace tglmt::compiler {

enum class TokenType {

    LeftParen, RightParen, LeftBracket, RightBracket, LeftBrace, RightBrace,
    Dot, Comma, Colon, Semicolon, Equal,

    Plus, Minus, Star, Slash, Percent,
    Increment, Decrement,
    PlusEqual, MinusEqual, StarEqual, SlashEqual, PercentEqual,
    Ampersand, Bar, Caret, Tilde, Bang,
    AmpAmp, BarBar, CaretCaret,
    AmpersandEqual, BarEqual, CaretEqual,
    LeftShift, RightShift,
    LeftShiftEqual, RightShiftEqual,
    EqualEqual, NotEqual,
    Less, LessEqual, Greater, GreaterEqual,
    Arrow,
    Question,

    IntLiteral, UintLiteral, FloatLiteral, DoubleLiteral, BoolLiteral,
    StringLiteral,

    Ident,

    Kw_Attribute, Kw_Const, Kw_Uniform, Kw_Buffer, Kw_Varying,
    Kw_In, Kw_Out, Kw_InOut, Kw_Patch,
    Kw_Centroid, Kw_Sample, Kw_Flat, Kw_Smooth, Kw_NoPerspective,
    Kw_Invariant, Kw_Precise,
    Kw_Shared, Kw_ReadOnly, Kw_WriteOnly, Kw_Coherent, Kw_Volatile, Kw_Restrict,
    Kw_Void, Kw_Float, Kw_Double, Kw_Int, Kw_Uint, Kw_Bool,
    Kw_Vec2, Kw_Vec3, Kw_Vec4, Kw_Ivec2, Kw_Ivec3, Kw_Ivec4,
    Kw_Uvec2, Kw_Uvec3, Kw_Uvec4, Kw_Bvec2, Kw_Bvec3, Kw_Bvec4,
    Kw_Mat2, Kw_Mat3, Kw_Mat4, Kw_Dmat2, Kw_Dmat3, Kw_Dmat4,
    Kw_Mat2x2, Kw_Mat2x3, Kw_Mat2x4, Kw_Mat3x2, Kw_Mat3x3, Kw_Mat3x4,
    Kw_Mat4x2, Kw_Mat4x3, Kw_Mat4x4,
    Kw_Sampler1D, Kw_Sampler2D, Kw_Sampler3D, Kw_SamplerCube,
    Kw_Sampler1DShadow, Kw_Sampler2DShadow, Kw_Sampler2DRect,
    Kw_Sampler2DRectShadow, Kw_Sampler1DArray, Kw_Sampler2DArray,
    Kw_SamplerBuffer, Kw_Sampler2DMS, Kw_Sampler2DMSArray,
    Kw_SamplerCubeShadow,
    Kw_Isampler1D, Kw_Isampler2D, Kw_Isampler3D, Kw_IsamplerCube,
    Kw_Isampler2DRect, Kw_Isampler1DArray, Kw_Isampler2DArray,
    Kw_IsamplerBuffer,
    Kw_Usampler1D, Kw_Usampler2D, Kw_Usampler3D, Kw_UsamplerCube,
    Kw_Usampler2DRect, Kw_Usampler1DArray, Kw_Usampler2DArray,
    Kw_UsamplerBuffer,
    Kw_Image1D, Kw_Image2D, Kw_Image3D, Kw_Image2DRect, Kw_ImageCube,
    Kw_Image2DArray, Kw_ImageBuffer,
    Kw_AtomicUint,
    Kw_Struct, Kw_If, Kw_Else, Kw_For, Kw_While, Kw_Do, Kw_Switch, Kw_Case,
    Kw_Default, Kw_Break, Kw_Continue, Kw_Return, Kw_Discard,
    Kw_True, Kw_False,
    Kw_Subroutine,
    Kw_Layout,

    Kw_Gl_Position, Kw_Gl_PointSize, Kw_Gl_FragCoord, Kw_Gl_FrontFacing,
    Kw_Gl_PointCoord, Kw_Gl_VertexID, Kw_Gl_InstanceID, Kw_Gl_DrawID,
    Kw_Gl_BaseVertex, Kw_Gl_BaseInstance, Kw_Gl_ClipDistance,
    Kw_Gl_PrimitiveID, Kw_Gl_InvocationID, Kw_Gl_PrimitiveIDIn,
    Kw_Gl_Layer, Kw_Gl_ViewportIndex, Kw_Gl_TessLevelOuter,
    Kw_Gl_TessLevelInner, Kw_Gl_TessCoord, Kw_Gl_PatchVerticesIn,
    Kw_Gl_PatchSize, Kw_Gl_PatchSizeIn, Kw_Gl_In, Kw_Gl_Out,

    Pp_Version, Pp_Extension, Pp_Define, Pp_Undef, Pp_If, Pp_Ifdef,
    Pp_Ifndef, Pp_Else, Pp_Elif, Pp_Endif, Pp_Line, Pp_Pragma,
    Pp_Import,
    Pp_Defined, Pp_Include,

    EOF_Token,
    Error,
};

struct Token {
    TokenType type = TokenType::Error;
    std::string lexeme;
    SourceLocation location;
    int offset = 0;
};


inline constexpr int kMaxTokenLength = 4096;


class Lexer {
public:

    Lexer(std::string source, std::string fileName);



    std::vector<Token> tokenize();
    bool hasError() const { return hasError_; }
    std::string errorMessage() const { return errorMessage_; }

private:
    std::string source_;
    std::string fileName_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
    int offset_ = 0;
    bool hasError_ = false;
    std::string errorMessage_;
    mutable std::vector<int> lineStarts_;
    mutable bool lineIndexBuilt_ = false;

    char peek(int ahead = 0) const;
    char advance();
    void skipWhitespace();
    void skipComment();
    Token makeToken(TokenType type, int start, int end) const;
    SourceLocation locationAt(int offset) const;
    bool isIdentStart(char c) const;
    bool isIdentChar(char c) const;
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
    Token scanNumber(int start);
    Token scanString(int start);
    Token scanIdentOrKeyword(int start);
    Token scanPunctOrOperator(int start);
    Token scanDirective(int start);
    void recordError(int start, const std::string& msg);
};

}
