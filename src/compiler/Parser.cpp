#include "tglmt/compiler/AST.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace tglmt::compiler {

Parser::Parser(std::vector<Token> tokens, std::string fileName)
    : tokens_(std::move(tokens)), fileName_(std::move(fileName)) {}

Token Parser::peek(int ahead) const {
    size_t p = pos_ + static_cast<size_t>(ahead);
    if (p >= tokens_.size()) return tokens_.back();
    return tokens_[p];
}

Token Parser::advance() {
    if (!atEnd()) ++pos_;
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenType t) const {
    if (atEnd()) return false;
    return tokens_[pos_].type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

bool Parser::matchAny(std::initializer_list<TokenType> types) {
    for (auto t : types) {
        if (match(t)) return true;
    }
    return false;
}

SourceLocation Parser::locOf(const Token& t) const { return t.location; }

void Parser::expect(TokenType t, const std::string& msg) {
    if (!match(t)) {
        Token tok = peek();
        errorAt(tok, msg);
    }
}

void Parser::errorAt(const Token& t, const std::string& msg, const std::string& code) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.code = code;
    d.message = msg + " (token='" + t.lexeme + "', line=" + std::to_string(t.location.line) + ")";
    d.span = SourceSpan{locOf(t), locOf(t)};
    d.span->begin = t.location;
    d.span->end = t.location;
    diagnostics_.push_back(d);
}

bool Parser::atEnd() const {
    return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::EOF_Token;
}

AstTranslationUnit Parser::parse() {
    AstTranslationUnit tu;
    tu.loc = locOf(peek());
    tu.success = true;

    
    while (!atEnd()) {
        if (check(TokenType::Pp_Version) || check(TokenType::Pp_Extension) ||
            check(TokenType::Pp_Line) || check(TokenType::Pp_Pragma) ||
            check(TokenType::Pp_Import) || check(TokenType::Pp_Define) ||
            check(TokenType::Pp_Undef)) {
            
            advance();
            continue;
        }
        AstDeclaration decl = parseDeclaration();
        tu.declarations.push_back(std::move(decl));
    }

    tu.diagnostics = std::move(diagnostics_);
    tu.success = tu.diagnostics.empty() ||
        std::none_of(tu.diagnostics.begin(), tu.diagnostics.end(),
                     [](const Diagnostic& d) { return d.severity == DiagnosticSeverity::Error; });
    return tu;
}

bool Parser::isTypeKeyword(TokenType t) const {
    switch (t) {
        case TokenType::Kw_Void: case TokenType::Kw_Float: case TokenType::Kw_Double:
        case TokenType::Kw_Int: case TokenType::Kw_Uint: case TokenType::Kw_Bool:
        case TokenType::Kw_Vec2: case TokenType::Kw_Vec3: case TokenType::Kw_Vec4:
        case TokenType::Kw_Ivec2: case TokenType::Kw_Ivec3: case TokenType::Kw_Ivec4:
        case TokenType::Kw_Uvec2: case TokenType::Kw_Uvec3: case TokenType::Kw_Uvec4:
        case TokenType::Kw_Bvec2: case TokenType::Kw_Bvec3: case TokenType::Kw_Bvec4:
        case TokenType::Kw_Mat2: case TokenType::Kw_Mat3: case TokenType::Kw_Mat4:
        case TokenType::Kw_Dmat2: case TokenType::Kw_Dmat3: case TokenType::Kw_Dmat4:
        case TokenType::Kw_Mat2x2: case TokenType::Kw_Mat2x3: case TokenType::Kw_Mat2x4:
        case TokenType::Kw_Mat3x2: case TokenType::Kw_Mat3x3: case TokenType::Kw_Mat3x4:
        case TokenType::Kw_Mat4x2: case TokenType::Kw_Mat4x3: case TokenType::Kw_Mat4x4:
        case TokenType::Kw_Sampler1D: case TokenType::Kw_Sampler2D: case TokenType::Kw_Sampler3D:
        case TokenType::Kw_SamplerCube: case TokenType::Kw_Sampler1DShadow:
        case TokenType::Kw_Sampler2DShadow: case TokenType::Kw_Sampler2DRect:
        case TokenType::Kw_Sampler2DRectShadow: case TokenType::Kw_Sampler1DArray:
        case TokenType::Kw_Sampler2DArray: case TokenType::Kw_SamplerBuffer:
        case TokenType::Kw_Sampler2DMS: case TokenType::Kw_Sampler2DMSArray:
        case TokenType::Kw_SamplerCubeShadow:
        case TokenType::Kw_Isampler1D: case TokenType::Kw_Isampler2D: case TokenType::Kw_Isampler3D:
        case TokenType::Kw_IsamplerCube: case TokenType::Kw_Isampler2DRect:
        case TokenType::Kw_Isampler1DArray: case TokenType::Kw_Isampler2DArray:
        case TokenType::Kw_IsamplerBuffer:
        case TokenType::Kw_Usampler1D: case TokenType::Kw_Usampler2D: case TokenType::Kw_Usampler3D:
        case TokenType::Kw_UsamplerCube: case TokenType::Kw_Usampler2DRect:
        case TokenType::Kw_Usampler1DArray: case TokenType::Kw_Usampler2DArray:
        case TokenType::Kw_UsamplerBuffer:
        case TokenType::Kw_Image1D: case TokenType::Kw_Image2D: case TokenType::Kw_Image3D:
        case TokenType::Kw_Image2DRect: case TokenType::Kw_ImageCube:
        case TokenType::Kw_Image2DArray: case TokenType::Kw_ImageBuffer:
        case TokenType::Kw_AtomicUint:
        case TokenType::Kw_Struct:
            return true;
        default:
            return false;
    }
}

AstType Parser::parseTypeSpecifier() {
    AstType ty;
    Token tok = peek();
    if (!isTypeKeyword(tok.type)) {
        errorAt(tok, "kieu du lieu mong doi");
        return ty;
    }
    ty.baseType = tok.lexeme;
    advance();

    
    if (tok.type == TokenType::Kw_Struct) {
        if (check(TokenType::Ident)) {
            ty.baseType = tok.lexeme + " " + peek().lexeme;
            advance();
        }
    }

    
    if (match(TokenType::LeftBracket)) {
        ty.isArray = true;
        if (!check(TokenType::RightBracket)) {
            Token sizeTok = peek();
            if (sizeTok.type == TokenType::IntLiteral) {
                ty.arraySizes.push_back(std::stoll(sizeTok.lexeme));
                advance();
            } else {
                ty.arraySizes.push_back(-1); 
                errorAt(sizeTok, "kich thuoc mang mong doi");
            }
        } else {
            ty.arraySizes.push_back(-1);
        }
        expect(TokenType::RightBracket, "']' mong doi sau kieu mang");
    }
    return ty;
}

AstType Parser::parseType() {
    AstType ty = parseTypeSpecifier();

    
    
    return ty;
}

void Parser::consumeSemicolon(const std::string& ctx) {
    if (!match(TokenType::Semicolon)) {
        errorAt(peek(), "; mong doi " + ctx);
    }
}

AstDeclaration Parser::parseDeclaration() {
    AstDeclaration decl;
    decl.loc = locOf(peek());

    
    std::vector<StorageQualifier> qualifiers;
    auto consumeQualifier = [&]() -> bool {
        Token t = peek();
        if (t.type == TokenType::Kw_Uniform) { qualifiers.push_back(StorageQualifier::Uniform); advance(); }
        else if (t.type == TokenType::Kw_Buffer) { qualifiers.push_back(StorageQualifier::Buffer); advance(); }
        else if (t.type == TokenType::Kw_In) { qualifiers.push_back(StorageQualifier::In); advance(); }
        else if (t.type == TokenType::Kw_Out) { qualifiers.push_back(StorageQualifier::Out); advance(); }
        else if (t.type == TokenType::Kw_InOut) { qualifiers.push_back(StorageQualifier::InOut); advance(); }
        else if (t.type == TokenType::Kw_Attribute) { qualifiers.push_back(StorageQualifier::In); advance(); }
        else if (t.type == TokenType::Kw_Varying) { qualifiers.push_back(StorageQualifier::Out); advance(); }
        else if (t.type == TokenType::Kw_Const) { qualifiers.push_back(StorageQualifier::Const); advance(); }
        else if (t.type == TokenType::Kw_Patch) { qualifiers.push_back(StorageQualifier::Patch); advance(); }
        else if (t.type == TokenType::Kw_Centroid) { qualifiers.push_back(StorageQualifier::Centroid); advance(); }
        else if (t.type == TokenType::Kw_Sample) { qualifiers.push_back(StorageQualifier::Sample); advance(); }
        else if (t.type == TokenType::Kw_Flat) { qualifiers.push_back(StorageQualifier::Flat); advance(); }
        else if (t.type == TokenType::Kw_Smooth) { qualifiers.push_back(StorageQualifier::Smooth); advance(); }
        else if (t.type == TokenType::Kw_NoPerspective) { qualifiers.push_back(StorageQualifier::NoPerspective); advance(); }
        else if (t.type == TokenType::Kw_Invariant) { qualifiers.push_back(StorageQualifier::Invariant); advance(); }
        else if (t.type == TokenType::Kw_Precise) { qualifiers.push_back(StorageQualifier::Precise); advance(); }
        else if (t.type == TokenType::Kw_Shared) { qualifiers.push_back(StorageQualifier::Shared); advance(); }
        else if (t.type == TokenType::Kw_ReadOnly) { qualifiers.push_back(StorageQualifier::ReadOnly); advance(); }
        else if (t.type == TokenType::Kw_WriteOnly) { qualifiers.push_back(StorageQualifier::WriteOnly); advance(); }
        else if (t.type == TokenType::Kw_Coherent) { qualifiers.push_back(StorageQualifier::Coherent); advance(); }
        else if (t.type == TokenType::Kw_Volatile) { qualifiers.push_back(StorageQualifier::Volatile); advance(); }
        else if (t.type == TokenType::Kw_Restrict) { qualifiers.push_back(StorageQualifier::Restrict); advance(); }
        else return false;
        return true;
    };
    while (consumeQualifier()) {}

    
    std::vector<LayoutQualifier> layout;
    if (match(TokenType::Kw_Layout)) {
        expect(TokenType::LeftParen, "'(' mong doi sau layout");
        while (true) {
            Token t = peek();
            if (t.type == TokenType::Ident) {
                std::string name = t.lexeme; advance();
                LayoutQualifier lq = LayoutQualifier::None;
                if (name == "location") lq = LayoutQualifier::Location;
                else if (name == "binding") lq = LayoutQualifier::Binding;
                else if (name == "offset") lq = LayoutQualifier::Offset;
                else if (name == "align") lq = LayoutQualifier::Align;
                else if (name == "rows") lq = LayoutQualifier::Rows;
                else if (name == "cols") lq = LayoutQualifier::Cols;
                else if (name == "column_major") lq = LayoutQualifier::MatrixMajorColumn;
                else if (name == "row_major") lq = LayoutQualifier::MatrixMajorRow;
                else if (name == "binding") lq = LayoutQualifier::Binding;
                else lq = LayoutQualifier::None; 
                layout.push_back(lq);
                if (match(TokenType::Equal)) {
                    
                    if (check(TokenType::IntLiteral)) advance();
                    else errorAt(peek(), "gia tri layout mong doi");
                }
            } else break;
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::RightParen, "')' mong doi sau layout");
    }

    
    
    while (consumeQualifier()) {}

    if (peek().type == TokenType::Ident && peek(1).type == TokenType::LeftBrace) {
        AstDeclaration blockDecl;
        blockDecl.kind = AstDeclKind::InterfaceBlock;
        blockDecl.loc = decl.loc;
        blockDecl.blockName = peek().lexeme;
        blockDecl.varQualifiers = qualifiers;
        blockDecl.varLayout = layout;
        
        for (auto& lq : layout) {
            if (lq == LayoutQualifier::Binding) blockDecl.blockBinding = 0; 
        }
        advance(); 
        expect(TokenType::LeftBrace, "'{' mong doi sau ten block");
        while (!check(TokenType::RightBrace) && !atEnd()) {
            AstDeclaration::StructMember m;
            m.loc = locOf(peek());
            if (isTypeKeyword(peek().type)) {
                m.type.baseType = peek().lexeme;
                advance();
            } else {
                errorAt(peek(), "kieu thanh vien mong doi");
                break;
            }
            if (check(TokenType::Ident)) {
                m.name = peek().lexeme;
                m.loc = locOf(peek());
                advance();
            } else {
                errorAt(peek(), "ten thanh vien mong doi");
                break;
            }
            if (match(TokenType::LeftBracket)) {
                m.type.isArray = true;
                if (!check(TokenType::RightBracket)) {
                    m.type.arraySizes.push_back(-1);
                }
                expect(TokenType::RightBracket, "']' mong doi");
            }
            expect(TokenType::Semicolon, "; mong doi sau thanh vien block");
            blockDecl.blockMembers.push_back(AstParameter{m.name, m.type});
        }
        expect(TokenType::RightBrace, "'}' mong doi ket thuc block");
        if (check(TokenType::Ident)) { blockDecl.blockInstanceName = peek().lexeme; advance(); }
        consumeSemicolon("block");
        return blockDecl;
    }

    
    AstType baseType = parseType();
    if (baseType.baseType.empty() && !diagnostics_.empty()) {
        
        while (!atEnd() && peek().type != TokenType::Semicolon) advance();
        if (!atEnd()) advance();
        return decl;
    }

    
    if (baseType.baseType.rfind("struct", 0) == 0 && check(TokenType::LeftBrace)) {
        AstDeclaration structDecl;
        structDecl.kind = AstDeclKind::Struct;
        structDecl.loc = decl.loc;
        
        size_t sp = baseType.baseType.find(' ');
        if (sp != std::string::npos) structDecl.structName = baseType.baseType.substr(sp + 1);
        advance(); 
        while (!check(TokenType::RightBrace) && !atEnd()) {
            AstDeclaration::StructMember m;
            m.loc = locOf(peek());
            
            if (isTypeKeyword(peek().type)) {
                m.type.baseType = peek().lexeme;
                advance();
            } else {
                errorAt(peek(), "kieu thanh vien mong doi");
                break;
            }
            
            if (check(TokenType::Ident)) {
                m.name = peek().lexeme;
                m.loc = locOf(peek());
                advance();
            } else {
                errorAt(peek(), "ten thanh vien mong doi");
                break;
            }
            
            if (match(TokenType::LeftBracket)) {
                m.type.isArray = true;
                if (!check(TokenType::RightBracket)) {
                    m.type.arraySizes.push_back(-1);
                }
                expect(TokenType::RightBracket, "']' mong doi");
            }
            expect(TokenType::Semicolon, "; mong doi sau thanh vien struct");
            structDecl.structMembers.push_back(std::move(m));
        }
        expect(TokenType::RightBrace, "'}' mong doi ket thuc struct");
        consumeSemicolon("struct");
        return structDecl;
    }

    
    Token nextTok = peek();
    if (nextTok.type == TokenType::LeftParen) {
        
        std::string funcName = baseType.baseType; 
        
        
        
        
        
    }

    
    
    

    if (check(TokenType::Ident) && peek(1).type == TokenType::LeftParen) {
        
        std::string funcName = peek().lexeme;
        advance(); 
        expect(TokenType::LeftParen, "'(' mong doi sau ten ham");
        AstDeclaration funcDecl;
        funcDecl.kind = AstDeclKind::Function;
        funcDecl.loc = decl.loc;
        funcDecl.funcName = funcName;
        funcDecl.funcReturnType = baseType;
        
        while (!check(TokenType::RightParen) && !atEnd()) {
            AstParameter param;
            param.loc = locOf(peek());
            
            Token pt = peek();
            if (pt.type == TokenType::Kw_In || pt.type == TokenType::Kw_Out ||
                pt.type == TokenType::Kw_InOut) {
                if (pt.type == TokenType::Kw_In) param.qualifier = StorageQualifier::In;
                else if (pt.type == TokenType::Kw_Out) param.qualifier = StorageQualifier::Out;
                else param.qualifier = StorageQualifier::InOut;
                advance();
            }
            param.type = parseType();
            if (check(TokenType::Ident)) {
                param.name = peek().lexeme;
                param.loc = locOf(peek());
                advance();
            }
            funcDecl.funcParams.push_back(param);
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::RightParen, "')' mong doi khoi chung cac tham so ham");
        
        if (match(TokenType::LeftBrace)) {
            funcDecl.funcBody.push_back(parseStatementList());
            expect(TokenType::RightBrace, "'}' mong doi ket thuc ham");
        } else {
            consumeSemicolon("prototype");
        }
        return funcDecl;
    }

    
    while (true) {
        if (!check(TokenType::Ident)) {
            errorAt(peek(), "ten bien mong doi");
            break;
        }
        std::string varName = peek().lexeme;
        SourceLocation varLoc = locOf(peek());
        advance();

        AstType varType = baseType;
        
        if (match(TokenType::LeftBracket)) {
            varType.isArray = true;
            if (!check(TokenType::RightBracket)) {
                Token sz = peek();
                if (sz.type == TokenType::IntLiteral) {
                    varType.arraySizes.push_back(std::stoll(sz.lexeme));
                    advance();
                } else {
                    varType.arraySizes.push_back(-1);
                }
            } else {
                varType.arraySizes.push_back(-1);
            }
            expect(TokenType::RightBracket, "']' mong doi");
        }

        AstDeclaration varDecl;
        varDecl.kind = AstDeclKind::Variable;
        varDecl.loc = varLoc;
        varDecl.varType = varType;
        varDecl.varName = varName;
        varDecl.varQualifiers = qualifiers;
        varDecl.varLayout = layout;
        varDecl.varIsUniform = std::find(qualifiers.begin(), qualifiers.end(), StorageQualifier::Uniform) != qualifiers.end();
        varDecl.varIsBuffer = std::find(qualifiers.begin(), qualifiers.end(), StorageQualifier::Buffer) != qualifiers.end();

        
        for (auto lq : layout) {
            if (lq == LayoutQualifier::Location) {  }
        }
        
        

        if (match(TokenType::Equal)) {
            varDecl.varInit = parseExpression();
        }

        consumeSemicolon("khai bao bien");

        
        
        
        
        
        
        return varDecl;
    }

    
    while (!atEnd() && peek().type != TokenType::Semicolon) advance();
    if (!atEnd()) advance();
    return decl;
}

AstDeclaration Parser::parseVariableDeclaration(const AstType&, StorageQualifier) {
    AstDeclaration d;
    return d;
}

AstDeclaration Parser::parseFunctionDeclaration(const AstType&, const std::string&) {
    AstDeclaration d;
    return d;
}

AstDeclaration Parser::parseStructDeclaration() {
    AstDeclaration d;
    d.kind = AstDeclKind::Struct;
    return d;
}

AstDeclaration Parser::parseInterfaceBlock() {
    AstDeclaration d;
    d.kind = AstDeclKind::InterfaceBlock;
    return d;
}

AstStmtPtr Parser::parseStatementList() {
    auto block = std::make_shared<AstStatement>();
    block->kind = AstStmtKind::Block;
    while (!atEnd() && peek().type != TokenType::RightBrace) {
        block->blockStmts.push_back(parseStatement());
    }
    return block;
}

AstStmtPtr Parser::parseStatement() {
    Token t = peek();
    switch (t.type) {
        case TokenType::Semicolon: { advance(); auto s = std::make_shared<AstStatement>(); s->kind = AstStmtKind::Block; return s; }
        case TokenType::LeftBrace: return parseBlockStatement();
        case TokenType::Kw_If: return parseIfStatement();
        case TokenType::Kw_For: return parseForStatement();
        case TokenType::Kw_While: return parseWhileStatement();
        case TokenType::Kw_Do: return parseDoWhileStatement();
        case TokenType::Kw_Return: return parseReturnStatement();
        case TokenType::Kw_Discard: return parseDiscardStatement();
        case TokenType::Kw_Break: { advance(); consumeSemicolon("break"); auto s = std::make_shared<AstStatement>(); s->kind = AstStmtKind::Break; s->loc = locOf(t); return s; }
        case TokenType::Kw_Continue: { advance(); consumeSemicolon("continue"); auto s = std::make_shared<AstStatement>(); s->kind = AstStmtKind::Continue; s->loc = locOf(t); return s; }
        case TokenType::Ident:
        case TokenType::Kw_Gl_Position: case TokenType::Kw_Gl_PointSize:
        case TokenType::Kw_Gl_FragCoord: case TokenType::Kw_Gl_FrontFacing:
        case TokenType::Kw_Gl_PointCoord: case TokenType::Kw_Gl_VertexID:
        case TokenType::Kw_Gl_InstanceID: case TokenType::Kw_Gl_DrawID:
        case TokenType::Kw_Gl_BaseVertex: case TokenType::Kw_Gl_BaseInstance:
        case TokenType::Kw_Gl_ClipDistance: case TokenType::Kw_Gl_PrimitiveID:
        case TokenType::Kw_Gl_InvocationID: case TokenType::Kw_Gl_PrimitiveIDIn:
        case TokenType::Kw_Gl_Layer: case TokenType::Kw_Gl_ViewportIndex:
        case TokenType::Kw_Gl_TessLevelOuter: case TokenType::Kw_Gl_TessLevelInner:
        case TokenType::Kw_Gl_TessCoord: case TokenType::Kw_Gl_PatchVerticesIn:
        case TokenType::Kw_Gl_PatchSize: case TokenType::Kw_Gl_PatchSizeIn:
        case TokenType::Kw_Gl_In: case TokenType::Kw_Gl_Out:
            
            
            
            return parseExpressionStatement();
        default:
            return parseDeclarationStmt();
    }
}

AstStmtPtr Parser::parseBlockStatement() {
    expect(TokenType::LeftBrace, "'{' mong doi");
    auto block = std::make_shared<AstStatement>();
    block->kind = AstStmtKind::Block;
    block->loc = locOf(peek());
    while (!check(TokenType::RightBrace) && !atEnd()) {
        block->blockStmts.push_back(parseStatement());
    }
    expect(TokenType::RightBrace, "'}' mong doi");
    return block;
}

AstStmtPtr Parser::parseDeclarationStmt() {
    
    AstDeclaration d = parseDeclaration();
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::Declaration;
    s->loc = d.loc;
    s->decl = std::make_unique<AstDeclaration>(std::move(d));
    return s;
}

AstStmtPtr Parser::parseExpressionStatement() {
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::Expression;
    s->loc = locOf(peek());
    s->exprStmt = parseExpression();
    consumeSemicolon("lenh bieu thuc");
    return s;
}

AstStmtPtr Parser::parseReturnStatement() {
    Token t = peek(); expect(TokenType::Kw_Return, "return mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::Return;
    s->loc = locOf(t);
    if (!check(TokenType::Semicolon)) {
        s->returnExpr = parseExpression();
    }
    consumeSemicolon("return");
    return s;
}

AstStmtPtr Parser::parseDiscardStatement() {
    Token t = peek(); expect(TokenType::Kw_Discard, "discard mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::Discard;
    s->loc = locOf(t);
    consumeSemicolon("discard");
    return s;
}

AstStmtPtr Parser::parseIfStatement() {
    Token t = peek(); expect(TokenType::Kw_If, "if mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::If;
    s->loc = locOf(t);
    expect(TokenType::LeftParen, "'(' mong doi sau if");
    s->ifCond = parseExpression();
    expect(TokenType::RightParen, "')' mong doi sau dieu kien if");
    s->ifThen = parseStatement();
    if (match(TokenType::Kw_Else)) {
        s->ifElse = parseStatement();
    }
    return s;
}

AstStmtPtr Parser::parseForStatement() {
    Token t = peek(); expect(TokenType::Kw_For, "for mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::For;
    s->loc = locOf(t);
    expect(TokenType::LeftParen, "'(' mong doi sau for");
    if (isTypeKeyword(peek().type) || peek().type == TokenType::Kw_Const ||
        peek().type == TokenType::Kw_Uniform || peek().type == TokenType::Kw_In ||
        peek().type == TokenType::Kw_Out || peek().type == TokenType::Kw_InOut) {
        s->loopInitDecl = std::make_unique<AstDeclaration>(parseDeclaration());
    } else if (!check(TokenType::Semicolon)) {
        s->loopInit = parseExpression();
        expect(TokenType::Semicolon, "; mong doi trong for");
    } else {
        expect(TokenType::Semicolon, "; mong doi trong for");
    }
    if (!check(TokenType::Semicolon)) s->loopCond = parseExpression();
    expect(TokenType::Semicolon, "; mong doi trong for");
    if (!check(TokenType::RightParen)) s->loopInc = parseExpression();
    expect(TokenType::RightParen, "')' mong doi sau for");
    s->loopBody = parseStatement();
    return s;
}

AstStmtPtr Parser::parseWhileStatement() {
    Token t = peek(); expect(TokenType::Kw_While, "while mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::While;
    s->loc = locOf(t);
    expect(TokenType::LeftParen, "'(' mong doi sau while");
    s->loopCond = parseExpression();
    expect(TokenType::RightParen, "')' mong doi sau dieu kien while");
    s->loopBody = parseStatement();
    return s;
}

AstStmtPtr Parser::parseDoWhileStatement() {
    Token t = peek(); expect(TokenType::Kw_Do, "do mong doi");
    auto s = std::make_shared<AstStatement>();
    s->kind = AstStmtKind::DoWhile;
    s->loc = locOf(t);
    s->loopBody = parseStatement();
    expect(TokenType::Kw_While, "while mong doi sau do");
    expect(TokenType::LeftParen, "'(' mong doi sau while");
    s->loopCond = parseExpression();
    expect(TokenType::RightParen, "')' mong doi");
    consumeSemicolon("do-while");
    return s;
}

AstExprPtr Parser::parseExpression() { return parseAssignment(); }

AstExprPtr Parser::parseAssignment() {
    AstExprPtr left = parseTernary();
    if (!left) return left;
    Token t = peek();
    if (t.type == TokenType::Equal || t.type == TokenType::PlusEqual ||
        t.type == TokenType::MinusEqual || t.type == TokenType::StarEqual ||
        t.type == TokenType::SlashEqual || t.type == TokenType::PercentEqual) {
        advance();
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = t.lexeme;
        e->loc = locOf(t);
        e->left = left;
        e->right = parseAssignment();
        return e;
    }
    return left;
}

AstExprPtr Parser::parseTernary() {
    AstExprPtr cond = parseLogicalOr();
    if (!cond) return cond;
    if (match(TokenType::Question)) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Ternary;
        e->loc = cond->loc;
        e->cond = cond;
        e->thenExpr = parseExpression();
        expect(TokenType::Colon, "':' mong doi trong bieu thuc 3 ngoi");
        e->elseExpr = parseExpression();
        return e;
    }
    return cond;
}

AstExprPtr Parser::parseLogicalOr() {
    AstExprPtr left = parseLogicalXor();
    while (match(TokenType::BarBar)) { 
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "||";
        e->loc = left->loc;
        e->left = left;
        e->right = parseLogicalXor();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseLogicalXor() {
    AstExprPtr left = parseLogicalAnd();
    while (match(TokenType::CaretCaret)) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "^^";
        e->loc = left->loc;
        e->left = left;
        e->right = parseLogicalAnd();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseLogicalAnd() {
    AstExprPtr left = parseBitwiseOr();
    while (match(TokenType::AmpAmp)) { 
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "&&";
        e->loc = left->loc;
        e->left = left;
        e->right = parseBitwiseOr();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseBitwiseOr() {
    AstExprPtr left = parseBitwiseXor();
    while (match(TokenType::Bar)) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "|";
        e->loc = left->loc;
        e->left = left;
        e->right = parseBitwiseXor();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseBitwiseXor() {
    AstExprPtr left = parseBitwiseAnd();
    while (match(TokenType::Caret)) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "^";
        e->loc = left->loc;
        e->left = left;
        e->right = parseBitwiseAnd();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseBitwiseAnd() {
    AstExprPtr left = parseEquality();
    while (match(TokenType::Ampersand)) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = "&";
        e->loc = left->loc;
        e->left = left;
        e->right = parseEquality();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseEquality() {
    AstExprPtr left = parseRelational();
    while (match(TokenType::EqualEqual) || match(TokenType::NotEqual)) {
        Token opTok = tokens_[pos_ - 1];
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = opTok.lexeme;
        e->loc = left->loc;
        e->left = left;
        e->right = parseRelational();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseRelational() {
    AstExprPtr left = parseShift();
    while (match(TokenType::Less) || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        Token opTok = tokens_[pos_ - 1];
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = opTok.lexeme;
        e->loc = left->loc;
        e->left = left;
        e->right = parseShift();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseShift() {
    AstExprPtr left = parseAdditive();
    while (match(TokenType::LeftShift) || match(TokenType::RightShift)) {
        Token opTok = tokens_[pos_ - 1];
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = opTok.lexeme;
        e->loc = left->loc;
        e->left = left;
        e->right = parseAdditive();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseAdditive() {
    AstExprPtr left = parseMultiplicative();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token opTok = tokens_[pos_ - 1];
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = opTok.lexeme;
        e->loc = left->loc;
        e->left = left;
        e->right = parseMultiplicative();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseMultiplicative() {
    AstExprPtr left = parseUnary();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        Token opTok = tokens_[pos_ - 1];
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Binary;
        e->op = opTok.lexeme;
        e->loc = left->loc;
        e->left = left;
        e->right = parseUnary();
        left = e;
    }
    return left;
}

AstExprPtr Parser::parseUnary() {
    Token t = peek();
    if (t.type == TokenType::Plus || t.type == TokenType::Minus ||
        t.type == TokenType::Bang || t.type == TokenType::Tilde ||
        t.type == TokenType::Increment || t.type == TokenType::Decrement) {
        advance();
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Unary;
        e->op = t.lexeme;
        e->loc = locOf(t);
        e->operand = parseUnary();
        return e;
    }
    return parsePostfix(parsePrimary());
}

AstExprPtr Parser::parsePostfix(AstExprPtr base) {
    while (true) {
        Token t = peek();
        if (t.type == TokenType::LeftParen) {
            
            advance();
            auto e = std::make_shared<AstExpression>();
            e->kind = AstExprKind::Call;
            e->loc = base->loc;
            
            if (base->kind == AstExprKind::Identifier) e->callee = base->name;
            while (!check(TokenType::RightParen) && !atEnd()) {
                e->args.push_back(parseExpression());
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RightParen, "')' mong doi sau doi so ham");
            base = e;
        } else if (t.type == TokenType::LeftBracket) {
            
            advance();
            auto e = std::make_shared<AstExpression>();
            e->kind = AstExprKind::Index;
            e->loc = base->loc;
            e->object = base;
            e->indices.push_back(parseExpression());
            expect(TokenType::RightBracket, "']' mong doi");
            base = e;
        } else if (t.type == TokenType::Increment || t.type == TokenType::Decrement) {
            advance();
            auto e = std::make_shared<AstExpression>();
            e->kind = AstExprKind::Unary;
            e->op = t.lexeme;
            e->loc = base->loc;
            e->operand = base;
            e->isPostfix = true;
            base = e;
        } else if (t.type == TokenType::Dot) {
            
            advance();
            if (!check(TokenType::Ident)) { errorAt(peek(), "ten thanh vien mong doi"); break; }
            auto e = std::make_shared<AstExpression>();
            e->kind = AstExprKind::Member;
            e->loc = base->loc;
            e->object = base;
            e->member = peek().lexeme;
            advance();
            base = e;
        } else {
            break;
        }
    }
    return base;
}

AstExprPtr Parser::parsePrimary() {
    Token t = peek();
    if (t.type == TokenType::IntLiteral) {
        advance();
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Integer;
        e->intValue = std::stoll(t.lexeme);
        e->loc = locOf(t);
        return e;
    }
    if (t.type == TokenType::FloatLiteral) {
        advance();
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Float;
        e->floatValue = std::stod(t.lexeme);
        e->loc = locOf(t);
        return e;
    }
    if (t.type == TokenType::Kw_True) { advance(); auto e = std::make_shared<AstExpression>(); e->kind = AstExprKind::Bool; e->boolValue = true; e->loc = locOf(t); return e; }
    if (t.type == TokenType::Kw_False) { advance(); auto e = std::make_shared<AstExpression>(); e->kind = AstExprKind::Bool; e->boolValue = false; e->loc = locOf(t); return e; }
    if (t.type == TokenType::StringLiteral) { advance(); auto e = std::make_shared<AstExpression>(); e->kind = AstExprKind::String; e->stringValue = t.lexeme; e->loc = locOf(t); return e; }
    if (t.type == TokenType::Ident || t.type == TokenType::Kw_Gl_Position ||
        t.type == TokenType::Kw_Gl_PointSize || t.type == TokenType::Kw_Gl_FragCoord ||
        t.type == TokenType::Kw_Gl_FrontFacing || t.type == TokenType::Kw_Gl_PointCoord ||
        t.type == TokenType::Kw_Gl_VertexID || t.type == TokenType::Kw_Gl_InstanceID ||
        t.type == TokenType::Kw_Gl_DrawID || t.type == TokenType::Kw_Gl_BaseVertex ||
        t.type == TokenType::Kw_Gl_BaseInstance || t.type == TokenType::Kw_Gl_ClipDistance ||
        t.type == TokenType::Kw_Gl_PrimitiveID || t.type == TokenType::Kw_Gl_InvocationID ||
        t.type == TokenType::Kw_Gl_PrimitiveIDIn || t.type == TokenType::Kw_Gl_Layer ||
        t.type == TokenType::Kw_Gl_ViewportIndex || t.type == TokenType::Kw_Gl_TessLevelOuter ||
        t.type == TokenType::Kw_Gl_TessLevelInner || t.type == TokenType::Kw_Gl_TessCoord ||
        t.type == TokenType::Kw_Gl_PatchVerticesIn || t.type == TokenType::Kw_Gl_PatchSize ||
        t.type == TokenType::Kw_Gl_PatchSizeIn || t.type == TokenType::Kw_Gl_In ||
        t.type == TokenType::Kw_Gl_Out) {
        advance();
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Identifier;
        e->name = t.lexeme;
        e->loc = locOf(t);
        return e;
    }
    
    if (isTypeKeyword(t.type) && peek(1).type == TokenType::LeftBracket) {
        size_t k = 2;
        int arrSize = -1;
        if (peek(k).type == TokenType::IntLiteral) { arrSize = (int)std::stoll(peek(k).lexeme); k++; }
        if (peek(k).type == TokenType::RightBracket && peek(k + 1).type == TokenType::LeftParen) {
            auto e = std::make_shared<AstExpression>();
            e->kind = AstExprKind::Constructor;
            e->loc = locOf(t);
            e->ctorType = t.lexeme;
            e->ctorIsArray = true;
            e->ctorArraySize = arrSize;
            advance(); advance();
            if (arrSize >= 0 && check(TokenType::IntLiteral)) advance();
            expect(TokenType::RightBracket, "']' mong doi sau kieu mang constructor");
            expect(TokenType::LeftParen, "'(' mong doi sau ten constructor");
            while (!check(TokenType::RightParen) && !atEnd()) {
                e->ctorArgs.push_back(parseExpression());
                if (!match(TokenType::Comma)) break;
            }
            expect(TokenType::RightParen, "')' mong doi sau doi so constructor");
            return e;
        }
    }
    if (isTypeKeyword(t.type) && peek(1).type == TokenType::LeftParen) {
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Constructor;
        e->loc = locOf(t);
        e->ctorType = t.lexeme;
        advance(); 
        expect(TokenType::LeftParen, "'(' mong doi sau ten constructor");
        while (!check(TokenType::RightParen) && !atEnd()) {
            e->ctorArgs.push_back(parseExpression());
            if (!match(TokenType::Comma)) break;
        }
        expect(TokenType::RightParen, "')' mong doi sau doi so constructor");
        return e;
    }
    if (t.type == TokenType::LeftParen) {
        advance();
        auto inner = parseExpression();
        expect(TokenType::RightParen, "')' mong doi");
        auto e = std::make_shared<AstExpression>();
        e->kind = AstExprKind::Paren;
        e->loc = t.location;
        e->left = inner;
        return e;
    }
    errorAt(t, "bieu thuc nguyen tuong khong hop le");
    auto e = std::make_shared<AstExpression>();
    e->kind = AstExprKind::Integer;
    e->intValue = 0;
    return e;
}

} 
