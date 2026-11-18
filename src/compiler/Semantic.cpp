#include "tglmt/compiler/Semantic.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace tglmt::compiler {

bool ResolvedType::isNumeric() const {
    return scalar == ResolvedScalar::Float || scalar == ResolvedScalar::Double ||
           scalar == ResolvedScalar::Int || scalar == ResolvedScalar::UInt;
}
bool ResolvedType::isInteger() const {
    return scalar == ResolvedScalar::Int || scalar == ResolvedScalar::UInt;
}
bool ResolvedType::isUnsigned() const { return scalar == ResolvedScalar::UInt; }
bool ResolvedType::isFloat() const { return scalar == ResolvedScalar::Float || scalar == ResolvedScalar::Double; }
bool ResolvedType::isBool() const { return scalar == ResolvedScalar::Bool; }

std::string ResolvedType::toString() const {
    std::string s;
    switch (scalar) {
        case ResolvedScalar::Float: s = "float"; break;
        case ResolvedScalar::Double: s = "double"; break;
        case ResolvedScalar::Int: s = "int"; break;
        case ResolvedScalar::UInt: s = "uint"; break;
        case ResolvedScalar::Bool: s = "bool"; break;
        case ResolvedScalar::Void: s = "void"; break;
    }
    if (isVector()) s = "vec" + std::to_string(vectorSize);
    else if (isMatrix()) s = "mat" + std::to_string(matrixCols) + "x" + std::to_string(matrixRows);
    else if (isArray()) s = "array<" + (arrayElement ? arrayElement->toString() : "?") + ">";
    else if (isStruct()) s = "struct " + structName;
    else if (isSampler) s = "sampler";
    else if (isImage) s = "image";
    return s;
}

bool ResolvedType::operator==(const ResolvedType& o) const {
    if (kind != o.kind) return false;
    if (scalar != o.scalar) return false;
    if (isVector() && vectorSize != o.vectorSize) return false;
    if (isMatrix() && (matrixCols != o.matrixCols || matrixRows != o.matrixRows)) return false;
    if (isArray() && arraySize != o.arraySize) return false;
    if (kind == ResolvedKind::Sampler) {
        if (samplerDim != o.samplerDim) return false;
        if (samplerScalar != o.samplerScalar) return false;
        if (isShadow != o.isShadow) return false;
    }
    return true;
}

ConversionRank RankConversion(const ResolvedType& from, const ResolvedType& to) {
    if (from == to) return ConversionRank::Exact;
    
    if (from.isOpaque() || to.isOpaque()) return ConversionRank::None;
    
    if (from.scalar == ResolvedScalar::Void || to.scalar == ResolvedScalar::Void)
        return ConversionRank::None;
    
    
    if (from.isScalar() && to.isVector() && from.scalar == to.scalar)
        return ConversionRank::Promotion;

    auto implicitlyConvertible = [](ResolvedScalar f, ResolvedScalar t) {
        if (f == t) return true;
        if (f == ResolvedScalar::Int && (t == ResolvedScalar::UInt || t == ResolvedScalar::Float || t == ResolvedScalar::Double)) return true;
        if (f == ResolvedScalar::UInt && (t == ResolvedScalar::Float || t == ResolvedScalar::Double)) return true;
        if (f == ResolvedScalar::Float && t == ResolvedScalar::Double) return true;
        return false;
    };
    if (from.isScalar() && to.isScalar() && implicitlyConvertible(from.scalar, to.scalar))
        return ConversionRank::Conversion;
    if (from.isVector() && to.isVector() && from.vectorSize == to.vectorSize &&
        implicitlyConvertible(from.scalar, to.scalar))
        return ConversionRank::Conversion;
    if (from.isMatrix() && to.isMatrix() && from.matrixCols == to.matrixCols &&
        from.matrixRows == to.matrixRows && implicitlyConvertible(from.scalar, to.scalar))
        return ConversionRank::Conversion;
    if (from.isScalar() && to.isVector() &&
        implicitlyConvertible(from.scalar, to.scalar))
        return ConversionRank::Promotion;
    
    if (from.isVector() || from.isMatrix() || to.isVector() || to.isMatrix())
        return ConversionRank::None;
    
    if (from.isArray() || from.isStruct() || to.isArray() || to.isStruct())
        return ConversionRank::None;
    
    if (from.isScalar() && to.isScalar()) {
        
        if ((from.scalar == ResolvedScalar::Int && to.scalar == ResolvedScalar::Float) ||
            (from.scalar == ResolvedScalar::Float && to.scalar == ResolvedScalar::Double))
            return ConversionRank::Promotion;
        
        if ((from.isBool() && to.isNumeric()) || (from.isNumeric() && to.isBool()))
            return ConversionRank::Conversion;
        
        if ((from.scalar == ResolvedScalar::Int && to.scalar == ResolvedScalar::UInt) ||
            (from.scalar == ResolvedScalar::UInt && to.scalar == ResolvedScalar::Int))
            return ConversionRank::Conversion;
    }
    return ConversionRank::None;
}

void SymbolTable::pushScope() {
    scopes_.emplace_back();
}

void SymbolTable::popScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

bool SymbolTable::insert(const Symbol& sym) {
    if (scopes_.empty()) return false;
    auto& cur = scopes_.back();
    
    cur[sym.name].push_back(std::make_shared<Symbol>(sym));
    return true;
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string& name) const {
    for (int i = (int)scopes_.size() - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end() && !it->second.empty()) return it->second.front();
    }
    return nullptr;
}

std::shared_ptr<Symbol> SymbolTable::lookupCurrentScope(const std::string& name) const {
    if (scopes_.empty()) return nullptr;
    auto it = scopes_.back().find(name);
    if (it != scopes_.back().end() && !it->second.empty()) return it->second.front();
    return nullptr;
}

std::vector<std::shared_ptr<Symbol>> SymbolTable::lookupAll(const std::string& name) const {
    std::vector<std::shared_ptr<Symbol>> out;
    
    for (int i = (int)scopes_.size() - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) {
            for (auto& s : it->second) out.push_back(s);
        }
    }
    return out;
}

SemanticAnalyzer::SemanticAnalyzer(ShaderStage stage, const GLSLVersion& version,
                                   GlslProfile profile, const ExtensionSet& extensions)
    : stage_(stage), version_(version), profile_(profile), extensions_(extensions) {}

void SemanticAnalyzer::diagError(int offset, const std::string& code, const std::string& msg) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.code = code;
    d.message = msg;
    d.span = SourceSpan{};
    d.span->begin.line = 1; d.span->begin.column = 1; d.span->begin.offset = offset;
    d.span->end = d.span->begin;
    diagnostics_.push_back(d);
    ++errorCount_;
}

void SemanticAnalyzer::diagError(const SourceLocation& loc, const std::string& code,
                                 const std::string& msg) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::Error;
    d.code = code;
    d.message = msg;
    d.span = SourceSpan{};
    d.span->begin = loc;
    d.span->end = loc;
    diagnostics_.push_back(d);
    ++errorCount_;
}

ResolvedType SemanticAnalyzer::resolveAstType(const AstType& ty) {
    ResolvedType rt;
    auto toScalar = [](const std::string& b) -> ResolvedScalar {
        if (b == "float") return ResolvedScalar::Float;
        if (b == "double") return ResolvedScalar::Double;
        if (b == "int") return ResolvedScalar::Int;
        if (b == "uint") return ResolvedScalar::UInt;
        if (b == "bool") return ResolvedScalar::Bool;
        return ResolvedScalar::Void;
    };
    std::string base = ty.baseType;
    
    if (base.rfind("struct ", 0) == 0) {
        rt.kind = ResolvedKind::Struct;
        rt.structName = base.substr(7);
        rt.scalar = ResolvedScalar::Void;
    } else {
        rt.scalar = toScalar(base);
        if (base == "void") rt.kind = ResolvedKind::Scalar;
        else if (base.rfind("vec", 0) == 0) { rt.kind = ResolvedKind::Vector; rt.vectorSize = base[3]-'0'; rt.scalar = ResolvedScalar::Float; }
        else if (base.rfind("ivec", 0) == 0) { rt.kind = ResolvedKind::Vector; rt.scalar = ResolvedScalar::Int; rt.vectorSize = base[4]-'0'; }
        else if (base.rfind("uvec", 0) == 0) { rt.kind = ResolvedKind::Vector; rt.scalar = ResolvedScalar::UInt; rt.vectorSize = base[4]-'0'; }
        else if (base.rfind("bvec", 0) == 0) { rt.kind = ResolvedKind::Vector; rt.scalar = ResolvedScalar::Bool; rt.vectorSize = base[4]-'0'; }
        else if (base.rfind("mat", 0) == 0) { rt.kind = ResolvedKind::Matrix; rt.scalar = ResolvedScalar::Float; }
        else if (base.rfind("dmat", 0) == 0) { rt.kind = ResolvedKind::Matrix; rt.scalar = ResolvedScalar::Double; }
        else if (base.rfind("sampler", 0) == 0 || base.rfind("isampler", 0) == 0 || base.rfind("usampler", 0) == 0) {
            rt.kind = ResolvedKind::Sampler; rt.isSampler = true;
            std::string rest;
            if (base.rfind("isampler", 0) == 0) { rest = base.substr(8); rt.samplerScalar = ResolvedScalar::Int; }
            else if (base.rfind("usampler", 0) == 0) { rest = base.substr(8); rt.samplerScalar = ResolvedScalar::UInt; }
            else { rest = base.substr(7); rt.samplerScalar = ResolvedScalar::Float; }
            if (rest == "1D") rt.samplerDim = 1;
            else if (rest == "2D") rt.samplerDim = 2;
            else if (rest == "3D") rt.samplerDim = 3;
            else if (rest == "Cube") rt.samplerDim = 4;
            else if (rest == "2DRect") rt.samplerDim = 5;
            else if (rest == "1DArray") rt.samplerDim = 6;
            else if (rest == "2DArray") rt.samplerDim = 7;
            else if (rest == "CubeArray") rt.samplerDim = 8;
            else if (rest == "Buffer") rt.samplerDim = 9;
            else if (rest == "2DMS") rt.samplerDim = 10;
            else if (rest == "2DMSArray") rt.samplerDim = 11;
            else if (rest == "1DShadow") { rt.samplerDim = 1; rt.isShadow = true; }
            else if (rest == "2DShadow") { rt.samplerDim = 2; rt.isShadow = true; }
            else if (rest == "2DRectShadow") { rt.samplerDim = 5; rt.isShadow = true; }
            else if (rest == "CubeShadow") { rt.samplerDim = 4; rt.isShadow = true; }
            else if (rest == "1DArrayShadow") { rt.samplerDim = 6; rt.isShadow = true; }
            else if (rest == "2DArrayShadow") { rt.samplerDim = 7; rt.isShadow = true; }
        }
        else if (base.rfind("image", 0) == 0) { rt.kind = ResolvedKind::Image; rt.isImage = true; }
        else if (base == "atomic_uint") { rt.kind = ResolvedKind::Opaque; }
        else rt.kind = ResolvedKind::Scalar;
    }

    if (rt.kind == ResolvedKind::Matrix) {
        std::string dims = base;
        if (dims.rfind("dmat", 0) == 0) dims = dims.substr(4);
        else if (dims.rfind("mat", 0) == 0) dims = dims.substr(3);
        if (dims.size() == 1) {
            rt.matrixCols = rt.matrixRows = dims[0] - '0';
        } else if (dims.size() == 3 && dims[1] == 'x') {
            rt.matrixCols = dims[0] - '0';
            rt.matrixRows = dims[2] - '0';
        } else {
            rt.matrixCols = rt.matrixRows = 4;
        }
    }
    
    if (ty.isArray) {
        ResolvedType baseType = rt;
        rt.kind = ResolvedKind::Array;
        rt.arraySize = ty.arraySizes.empty() ? -1 : ty.arraySizes[0];
        rt.arrayElement = std::make_shared<ResolvedType>(baseType);
        
        rt.scalar = ResolvedScalar::Void;
        rt.vectorSize = 0;
        rt.matrixCols = 0; rt.matrixRows = 0;
    }
    return rt;
}

void SemanticAnalyzer::declareBuiltins() {
    
    auto addFunc = [&](const std::string& name, ResolvedType ret, std::vector<ResolvedType> params) {
        auto existing = symTab_.lookupAll(name);
        for (auto& e : existing)
            if (e->kind == SymbolKind::BuiltinFunction && e->funcParamTypes == params) return;
        Symbol s;
        s.name = name; s.kind = SymbolKind::BuiltinFunction;
        s.type = ret; s.funcParamTypes = std::move(params);
        s.isBuiltin = true; s.declLoc = SourceLocation{};
        symTab_.insert(s);
    };
    
    addFunc("pow2", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}});
    addFunc("pow2", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("pow2", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}});
    addFunc("pow2", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    addFunc("pow2", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    
    addFunc("dot", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    
    addFunc("mix", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("mix", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("mix", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    
    addFunc("max", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("max", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("max", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    addFunc("max", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}});
    addFunc("min", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("min", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("min", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    addFunc("min", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}});
    
    addFunc("length", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    addFunc("length", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("abs", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("abs", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("clamp", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("clamp", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("clamp", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}});
    
    addFunc("dot", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    
    addFunc("mix", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    addFunc("length", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}, {ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}});
    
    addFunc("texture", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2}});
    addFunc("texture", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
    addFunc("texture", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
    addFunc("texelFetch", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, 2}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}});
    addFunc("texelFetch", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, 3}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}});
    
    {
        ResolvedType f{ResolvedKind::Scalar, ResolvedScalar::Float};
        ResolvedType v2{ResolvedKind::Vector, ResolvedScalar::Float, 2};
        ResolvedType v3{ResolvedKind::Vector, ResolvedScalar::Float, 3};
        ResolvedType v4{ResolvedKind::Vector, ResolvedScalar::Float, 4};
        std::vector<ResolvedType> gens = {f, v2, v3, v4};
        const char* gen1[] = {"radians", "degrees", "sin", "cos", "tan", "asin", "acos",
                              "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
                              "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",
                              "abs", "sign", "floor", "trunc", "round", "roundEven",
                              "ceil", "fract", "dFdx", "dFdy", "fwidth"};
        for (const char* fn : gen1)
            for (auto& g : gens) addFunc(fn, g, {g});
        for (auto& g : gens) addFunc("atan", g, {g});
        const char* gen2[] = {"pow", "mod", "min", "max", "step", "reflect"};
        for (const char* fn : gen2)
            for (auto& g : gens) addFunc(fn, g, {g, g});
        for (auto& g : gens) addFunc("atan", g, {g, g});
        for (auto& g : gens) addFunc("distance", f, {g, g});
        for (auto& g : gens) addFunc("dot", f, {g, g});
        for (auto& g : gens) addFunc("length", f, {g});
        for (auto& g : gens) addFunc("normalize", g, {g});
        addFunc("cross", v3, {v3, v3});
        for (auto& g : gens) addFunc("refract", g, {g, g, f});
        for (auto& g : gens) {
            addFunc("mix", g, {g, g, f});
            addFunc("mix", g, {g, g, g});
            addFunc("clamp", g, {g, g, g});
            addFunc("smoothstep", g, {g, g, g});
            addFunc("smoothstep", g, {f, f, g});
            addFunc("faceforward", g, {g, g, g});
        }
        addFunc("textureLod", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, v2, f});
        addFunc("textureLod", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, v3, f});
        addFunc("textureGrad", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, v2, v2, v2});
        addFunc("textureGrad", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, v3, v3, v3});
        addFunc("textureSize", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, 2}, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}});
        addFunc("texelFetch", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, 2}});
        addFunc("texelFetch", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, 3}});
        addFunc("texelFetch", v4, {ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}, ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}});
        ResolvedType ivec2{ResolvedKind::Vector, ResolvedScalar::Int, 2};
        ResolvedType ivec3{ResolvedKind::Vector, ResolvedScalar::Int, 3};
        ResolvedType ivec4{ResolvedKind::Vector, ResolvedScalar::Int, 4};
        ResolvedType uvec4{ResolvedKind::Vector, ResolvedScalar::UInt, 4};
        ResolvedType si{ResolvedKind::Scalar, ResolvedScalar::Int};
        auto makeSampler = [](int dim, ResolvedScalar sc) {
            ResolvedType t{ResolvedKind::Sampler, ResolvedScalar::Void};
            t.isSampler = true; t.samplerDim = dim; t.samplerScalar = sc;
            return t;
        };
        addFunc("texelFetch", ivec4, {makeSampler(9, ResolvedScalar::Int), si});
        addFunc("texelFetch", v4, {makeSampler(9, ResolvedScalar::Float), si});
        addFunc("texelFetch", uvec4, {makeSampler(9, ResolvedScalar::UInt), si});
        addFunc("texelFetch", v4, {makeSampler(2, ResolvedScalar::Float), ivec2, si});
        addFunc("texelFetch", ivec4, {makeSampler(2, ResolvedScalar::Int), ivec2, si});
        addFunc("texelFetch", uvec4, {makeSampler(2, ResolvedScalar::UInt), ivec2, si});
        addFunc("texelFetch", v4, {makeSampler(7, ResolvedScalar::Float), ivec3, si});
        addFunc("texelFetch", ivec4, {makeSampler(7, ResolvedScalar::Int), ivec3, si});
        addFunc("texelFetch", uvec4, {makeSampler(7, ResolvedScalar::UInt), ivec3, si});
        addFunc("textureProj", v4, {makeSampler(2, ResolvedScalar::Float), ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 3}});
        addFunc("textureProj", v4, {makeSampler(2, ResolvedScalar::Float), ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4}});
        addFunc("texture", v4, {makeSampler(4, ResolvedScalar::Float), v3});
        addFunc("texture", v4, {makeSampler(3, ResolvedScalar::Float), v3});
        addFunc("texture", v4, {makeSampler(7, ResolvedScalar::Float), v3});
        addFunc("texture", v4, {makeSampler(2, ResolvedScalar::Float), v2});
        addFunc("textureLod", v4, {makeSampler(2, ResolvedScalar::Float), v2, f});
        addFunc("textureLod", v4, {makeSampler(4, ResolvedScalar::Float), v3, f});
        addFunc("textureGrad", v4, {makeSampler(2, ResolvedScalar::Float), v2, v2, v2});
        addFunc("textureGrad", v4, {makeSampler(4, ResolvedScalar::Float), v3, v3, v3});
    }
    auto addVar = [&](const std::string& name, ResolvedType ty) {
        Symbol s;
        s.name = name; s.kind = SymbolKind::BuiltinVariable;
        s.type = ty; s.isBuiltin = true; s.isLValue = true;
        s.declLoc = SourceLocation{};
        symTab_.insert(s);
    };
    addVar("gl_Position", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4});
    addVar("gl_PointSize", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float});
    addVar("gl_FragCoord", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 4});
    addVar("gl_FrontFacing", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool});
    addVar("gl_PointCoord", ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, 2});
    addVar("gl_VertexID", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int});
    addVar("gl_InstanceID", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int});
    addVar("gl_PrimitiveID", ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int});
}

std::optional<size_t> SemanticAnalyzer::resolveOverload(
    const std::string& name,
    const std::vector<CheckedExpr>& args,
    const std::vector<std::shared_ptr<Symbol>>& candidates,
    SourceLocation callLoc) {
    std::vector<size_t> viable;
    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& cand = candidates[i];
        if (cand->funcParamTypes.size() != args.size()) continue;
        bool ok = true;
        for (size_t j = 0; j < args.size(); ++j) {
            auto r = RankConversion(args[j].type, cand->funcParamTypes[j]);
            if (r == ConversionRank::None) { ok = false; break; }
        }
        if (ok) viable.push_back(i);
    }
    if (viable.empty()) return std::nullopt;
    if (viable.size() == 1) return viable[0];
    
    
    bool allExact = true;
    for (auto i : viable) {
        bool exact = true;
        for (size_t j = 0; j < args.size(); ++j)
            if (RankConversion(args[j].type, candidates[i]->funcParamTypes[j]) != ConversionRank::Exact) exact = false;
        if (!exact) { allExact = false; break; }
    }
    if (allExact) {
        
        
        diagError(callLoc, "SEM-001", "goi qua tai `" + name + "` co nhieu overload khop cung cap");
        return std::nullopt;
    }
    return viable[0];
}

CheckedExpr SemanticAnalyzer::checkLiteral(const AstExprPtr& e) {
    CheckedExpr c;
    c.isConstant = true; c.isLValue = false;
    switch (e->kind) {
        case AstExprKind::Integer: c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int}; c.intValue = e->intValue; break;
        case AstExprKind::Float: c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}; c.floatValue = e->floatValue; break;
        case AstExprKind::Bool: c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool}; c.boolValue = e->boolValue; break;
        case AstExprKind::Uint: c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::UInt}; c.intValue = e->intValue; break;
        default: c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Void};
    }
    return c;
}

CheckedExpr SemanticAnalyzer::checkIdentifier(const AstExprPtr& e) {
    CheckedExpr c;
    c.isLValue = true;
    auto sym = symTab_.lookup(e->name);
    if (!sym) {
        diagError(e->loc, "SEM-002", "bien khong duoc khai bao: " + e->name);
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Void};
        return c;
    }
    c.type = sym->type;
    c.isLValue = sym->isLValue && (sym->kind == SymbolKind::Variable || sym->kind == SymbolKind::BuiltinVariable);
    return c;
}

CheckedExpr SemanticAnalyzer::checkMember(const AstExprPtr& e) {
    auto base = checkExpression(e->object);
    if (!base.type.isStruct() && !base.type.isVector() && !base.type.isMatrix()) {
        diagError(e->loc, "SEM-003", "khong truy duoc thanh vien `" + e->member + "` tren kieu " + base.type.toString());
        return {{ResolvedKind::Scalar, ResolvedScalar::Void}};
    }
    CheckedExpr c;
    const std::string& m = e->member;
    if (base.type.isStruct()) {
        auto it = structFields_.find(base.type.structName);
        if (it != structFields_.end()) {
            auto mi = it->second.find(m);
            if (mi != it->second.end()) {
                c.type = mi->second;
                c.isLValue = base.isLValue;
                return c;
            }
        }
    }
    if (base.type.isVector()) {
        bool allSwizzle = !m.empty() && m.size() <= 4;
        int maxComp = base.type.vectorSize;
        for (char ch : m) {
            int idx = -1;
            if (ch == 'x' || ch == 'r' || ch == 's') idx = 0;
            else if (ch == 'y' || ch == 'g' || ch == 't') idx = 1;
            else if (ch == 'z' || ch == 'b' || ch == 'p') idx = 2;
            else if (ch == 'w' || ch == 'a' || ch == 'q') idx = 3;
            if (idx < 0 || idx >= maxComp) { allSwizzle = false; break; }
        }
        if (allSwizzle) {
            if (m.size() == 1)
                c.type = ResolvedType{ResolvedKind::Scalar, base.type.scalar};
            else
                c.type = ResolvedType{ResolvedKind::Vector, base.type.scalar, (int)m.size()};
            c.isLValue = base.isLValue;
            return c;
        }
    }
    if (base.type.isMatrix() && m.size() == 1) {
        int idx = -1;
        if (m[0] == 'x' || m[0] == 'r' || m[0] == 's') idx = 0;
        else if (m[0] == 'y' || m[0] == 'g' || m[0] == 't') idx = 1;
        else if (m[0] == 'z' || m[0] == 'b' || m[0] == 'p') idx = 2;
        else if (m[0] == 'w' || m[0] == 'a' || m[0] == 'q') idx = 3;
        if (idx >= 0 && idx < base.type.matrixCols) {
            c.type = ResolvedType{ResolvedKind::Vector, base.type.scalar,
                                  base.type.matrixRows > 0 ? base.type.matrixRows : 4};
            c.isLValue = base.isLValue;
            return c;
        }
    }
    c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float};
    c.isLValue = base.isLValue;
    return c;
}

CheckedExpr SemanticAnalyzer::checkIndex(const AstExprPtr& e) {
    auto base = checkExpression(e->object);
    auto idx = checkExpression(e->indices[0]);
    if (!base.type.isArray() && !base.type.isVector() && !base.type.isMatrix()) {
        diagError(e->loc, "SEM-004", "chi so mang tren kieu khong hop le");
        return {{ResolvedKind::Scalar, ResolvedScalar::Void}};
    }
    CheckedExpr c;
    if (base.type.isArray()) c.type = *base.type.arrayElement;
    else if (base.type.isMatrix())
        c.type = ResolvedType{ResolvedKind::Vector, base.type.scalar,
                              base.type.matrixRows > 0 ? base.type.matrixRows : 4};
    else c.type = ResolvedType{ResolvedKind::Scalar, base.type.scalar};
    c.isLValue = base.isLValue;
    return c;
}

CheckedExpr SemanticAnalyzer::checkSwizzle(const AstExprPtr& e) {
    auto base = checkExpression(e->object);
    if (!base.type.isVector()) {
        diagError(e->loc, "SEM-005", "swizzle tren kieu khong phai vector");
        return {{ResolvedKind::Scalar, ResolvedScalar::Void}};
    }
    CheckedExpr c;
    
    if (e->swizzle.size() == 1) {
        c.type = ResolvedType{base.type.kind == ResolvedKind::Vector ? ResolvedKind::Scalar : ResolvedKind::Scalar,
                              base.type.scalar};
    } else {
        c.type = base.type;
    }
    c.isLValue = base.isLValue;
    return c;
}

CheckedExpr SemanticAnalyzer::checkConstructor(const AstExprPtr& e) {
    CheckedExpr c;
    c.isConstant = true; c.isLValue = false;
    for (auto& a : e->ctorArgs) checkExpression(a);
    ResolvedType elem;
    const std::string& t = e->ctorType;
    if (t == "float") elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float};
    else if (t == "int") elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int};
    else if (t == "uint") elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::UInt};
    else if (t == "bool") elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool};
    else if (t == "double") elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Double};
    else if (t.size() == 4 && t.compare(0, 3, "vec") == 0) elem = ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, t[3] - '0'};
    else if (t.size() == 5 && t.compare(0, 4, "ivec") == 0) elem = ResolvedType{ResolvedKind::Vector, ResolvedScalar::Int, t[4] - '0'};
    else if (t.size() == 5 && t.compare(0, 4, "uvec") == 0) elem = ResolvedType{ResolvedKind::Vector, ResolvedScalar::UInt, t[4] - '0'};
    else if (t.size() == 5 && t.compare(0, 4, "bvec") == 0) elem = ResolvedType{ResolvedKind::Vector, ResolvedScalar::Bool, t[4] - '0'};
    else if (t == "mat2" || t == "mat3" || t == "mat4")
        elem = ResolvedType{ResolvedKind::Matrix, ResolvedScalar::Float, 0, t[3] - '0', t[3] - '0'};
    else if (t.size() == 6 && t.compare(0, 3, "mat") == 0 && t[4] == 'x')
        elem = ResolvedType{ResolvedKind::Matrix, ResolvedScalar::Float, 0, t[3] - '0', t[5] - '0'};
    else if (t.rfind("sampler", 0) == 0) { elem = ResolvedType{ResolvedKind::Sampler, ResolvedScalar::Void}; elem.isSampler = true; }
    else elem = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Void};
    if (e->ctorIsArray) {
        c.type = ResolvedType{ResolvedKind::Array, ResolvedScalar::Void};
        c.type.arraySize = e->ctorArraySize >= 0 ? e->ctorArraySize : (int)e->ctorArgs.size();
        c.type.arrayElement = std::make_shared<ResolvedType>(elem);
    } else {
        c.type = elem;
    }
    return c;
}

CheckedExpr SemanticAnalyzer::checkUnary(const AstExprPtr& e) {
    auto op = checkExpression(e->operand);
    CheckedExpr c;
    c.isLValue = false;
    c.type = op.type;
    if (e->op == "-") {
        if (!op.type.isNumeric()) diagError(e->loc, "SEM-006", "toan tu `-` can kieu so");
    } else if (e->op == "!") {
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool};
    }
    return c;
}

CheckedExpr SemanticAnalyzer::checkBinary(const AstExprPtr& e) {
    auto left = checkExpression(e->left);
    auto right = checkExpression(e->right);
    CheckedExpr c;
    c.isLValue = false;
    if (e->op == "+" || e->op == "-" || e->op == "*" || e->op == "/" || e->op == "%") {
        if (!left.type.isNumeric() || !right.type.isNumeric()) {
            diagError(e->loc, "SEM-007", "toan tu `" + e->op + "` can kieu so");
        }
        
        auto asFloatVec = [](ResolvedType t) -> ResolvedType {
            if (t.isVector() && t.scalar != ResolvedScalar::Float && t.scalar != ResolvedScalar::Double)
                return ResolvedType{ResolvedKind::Vector, ResolvedScalar::Float, t.vectorSize};
            if (t.isScalar() && t.scalar != ResolvedScalar::Float && t.scalar != ResolvedScalar::Double)
                return ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float};
            return t;
        };
        if (left.type.isFloat() || right.type.isFloat() || left.type.isVector() || right.type.isVector() ||
            left.type.isMatrix() || right.type.isMatrix()) {
            if (!(left.type.isFloat() && right.type.isFloat())) {
                left.type = asFloatVec(left.type);
                right.type = asFloatVec(right.type);
            }
        }
        if (left.type.isMatrix() && right.type.isVector())
            c.type = ResolvedType{ResolvedKind::Vector, left.type.scalar,
                                  left.type.matrixRows > 0 ? left.type.matrixRows : 4};
        else if (left.type.isVector() && right.type.isMatrix())
            c.type = ResolvedType{ResolvedKind::Vector, right.type.scalar,
                                  right.type.matrixCols > 0 ? right.type.matrixCols : 4};
        else if (left.type.isMatrix() && right.type.isMatrix())
            c.type = left.type;
        else if (left.type.isMatrix()) c.type = left.type;
        else if (right.type.isMatrix()) c.type = right.type;
        else if (left.type.isVector()) c.type = left.type;
        else if (right.type.isVector()) c.type = right.type;
        else c.type = (left.type.isFloat() || right.type.isFloat())
            ? ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float}
            : ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int};
    } else if (e->op == "==" || e->op == "!=" || e->op == "<" || e->op == ">" ||
               e->op == "<=" || e->op == ">=") {
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool};
    } else if (e->op == "&&" || e->op == "||" || e->op == "^^") {
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool};
    } else if (e->op == "&" || e->op == "|" || e->op == "^") {
        if (left.type.isVector()) c.type = left.type;
        else if (right.type.isVector()) c.type = right.type;
        else if (left.type.isBool() && right.type.isBool())
            c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Bool};
        else c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Int};
    } else if (e->op == "<<" || e->op == ">>") {
        c.type = left.type;
    } else if (e->op == "=" || e->op == "+=" || e->op == "-=" || e->op == "*=" ||
               e->op == "/=" || e->op == "%=") {
        c.type = left.type;
    }
    return c;
}

CheckedExpr SemanticAnalyzer::checkTernary(const AstExprPtr& e) {
    auto cond = checkExpression(e->cond);
    auto t = checkExpression(e->thenExpr);
    auto f = checkExpression(e->elseExpr);
    CheckedExpr c;
    c.isLValue = t.isLValue || f.isLValue;
    c.type = (t.type == f.type) ? t.type : ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Float};
    return c;
}

CheckedExpr SemanticAnalyzer::checkCall(const AstExprPtr& e) {
    CheckedExpr c;
    c.isLValue = false;
    std::vector<CheckedExpr> checkedArgs;
    for (auto& a : e->args) checkedArgs.push_back(checkExpression(a));
    auto candidates = symTab_.lookupAll(e->callee);
    if (candidates.empty()) {
        diagError(e->loc, "SEM-008", "ham khong duoc khai bao: " + e->callee);
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Void};
        return c;
    }
    
    std::vector<std::shared_ptr<Symbol>> funcs;
    for (auto& s : candidates) if (s->kind == SymbolKind::BuiltinFunction || s->kind == SymbolKind::Function)
        funcs.push_back(s);
    
    auto chosen = resolveOverload(e->callee, checkedArgs, funcs, e->loc);
    if (!chosen) {
        diagError(e->loc, "SEM-008", "khong tim thay overload hop le cho ham `" + e->callee + "`");
        c.type = ResolvedType{ResolvedKind::Scalar, ResolvedScalar::Void};
        return c;
    }
    c.type = funcs[*chosen]->type; 
    return c;
}

CheckedExpr SemanticAnalyzer::checkExpression(const AstExprPtr& e) {
    CheckedExpr r = checkExpressionImpl(e);
    if (e) exprTypes_[e.get()] = r.type;
    return r;
}

CheckedExpr SemanticAnalyzer::checkExpressionImpl(const AstExprPtr& e) {
    if (!e) return {{ResolvedKind::Scalar, ResolvedScalar::Void}};
    switch (e->kind) {
        case AstExprKind::Integer: case AstExprKind::Float: case AstExprKind::Bool:
        case AstExprKind::Uint: return checkLiteral(e);
        case AstExprKind::Identifier: return checkIdentifier(e);
        case AstExprKind::Binary: return checkBinary(e);
        case AstExprKind::Unary: return checkUnary(e);
        case AstExprKind::Call: return checkCall(e);
        case AstExprKind::Constructor: return checkConstructor(e);
        case AstExprKind::Member: return checkMember(e);
        case AstExprKind::Index: return checkIndex(e);
        case AstExprKind::Swizzle: return checkSwizzle(e);
        case AstExprKind::Ternary: return checkTernary(e);
        case AstExprKind::Paren: return checkExpression(e->left);
        default: return {{ResolvedKind::Scalar, ResolvedScalar::Void}};
    }
}

void SemanticAnalyzer::checkVariableDeclaration(const AstDeclaration& d) {
    
    ResolvedType varType = resolveAstType(d.varType);
    if (d.varInit) {
        auto init = checkExpression(d.varInit);
        if (init.type.scalar == ResolvedScalar::Void && errorCount_ == 0) {
            
        } else if (!isAssignableTo(varType, init.type)) {
            diagError(d.loc, "SEM-009", "khoi tao kieu khong hop loi cho `" + d.varName + "`");
        }
    }
    
    Symbol s;
    s.name = d.varName; s.kind = SymbolKind::Variable;
    s.type = varType; s.qualifiers = d.varQualifiers; s.layout = d.varLayout;
    s.location = d.varLocation; s.binding = d.varBinding; s.declLoc = d.loc;
    s.isLValue = true;
    symTab_.insert(s);
}

bool SemanticAnalyzer::isAssignableTo(const ResolvedType& dst, const ResolvedType& src) const {
    if (dst == src) return true;
    if (src.isOpaque() || dst.isOpaque()) return false;
    auto r = RankConversion(src, dst);
    return r != ConversionRank::None;
}

void SemanticAnalyzer::checkStatement(const AstStmtPtr& s) {
    switch (s->kind) {
        case AstStmtKind::Block: checkBlock(s); break;
        case AstStmtKind::Declaration:
            if (s->decl) checkDeclaration(*s->decl);
            break;
        case AstStmtKind::Expression: checkExpression(s->exprStmt); break;
        case AstStmtKind::Return: if (s->returnExpr) checkExpression(s->returnExpr); break;
        case AstStmtKind::Discard: break;
        case AstStmtKind::If: {
            checkExpression(s->ifCond);
            checkStatement(s->ifThen);
            if (s->ifElse) checkStatement(s->ifElse);
            break;
        }
        case AstStmtKind::For: {
            symTab_.pushScope();
            if (s->loopInitDecl) checkDeclaration(*s->loopInitDecl);
            if (s->loopInit) checkExpression(s->loopInit);
            if (s->loopCond) checkExpression(s->loopCond);
            if (s->loopInc) checkExpression(s->loopInc);
            checkStatement(s->loopBody);
            symTab_.popScope();
            break;
        }
        case AstStmtKind::While:
        case AstStmtKind::DoWhile:
            if (s->loopCond) checkExpression(s->loopCond);
            checkStatement(s->loopBody);
            break;
        case AstStmtKind::Break:
        case AstStmtKind::Continue:
            break;
        default: break;
    }
}

void SemanticAnalyzer::checkBlock(const AstStmtPtr& s) {
    symTab_.pushScope();
    for (auto& stmt : s->blockStmts) checkStatement(stmt);
    symTab_.popScope();
}

void SemanticAnalyzer::checkDeclaration(const AstDeclaration& d) {
    switch (d.kind) {
        case AstDeclKind::Variable: checkVariableDeclaration(d); break;
        case AstDeclKind::Function: checkFunctionDeclaration(d); break;
        case AstDeclKind::Struct: checkStructDeclaration(d); break;
        case AstDeclKind::InterfaceBlock: checkInterfaceBlock(d); break;
        default: break;
    }
}

void SemanticAnalyzer::checkFunctionDeclaration(const AstDeclaration& d) {
    Symbol probe;
    probe.name = d.funcName; probe.kind = SymbolKind::Function;
    probe.type = resolveAstType(d.funcReturnType);
    for (auto& p : d.funcParams) probe.funcParamTypes.push_back(resolveAstType(p.type));
    auto existing = symTab_.lookupAll(d.funcName);
    bool duplicate = false;
    for (auto& e : existing) {
        if (e->kind == SymbolKind::Function && e->funcParamTypes == probe.funcParamTypes &&
            e->type == probe.type) {
            duplicate = true;
            break;
        }
    }
    if (!duplicate) {
        Symbol s;
        s.name = d.funcName; s.kind = SymbolKind::Function;
        s.type = probe.type;
        s.funcParamTypes = probe.funcParamTypes;
        s.qualifiers = {}; s.declLoc = d.loc; s.isBuiltin = false;
        symTab_.insert(s);
    }
    
    symTab_.pushScope();
    for (auto& p : d.funcParams) {
        Symbol ps; ps.name = p.name; ps.kind = SymbolKind::Variable;
        ps.type = resolveAstType(p.type); ps.qualifiers = {p.qualifier}; ps.isLValue = true;
        symTab_.insert(ps);
    }
    for (auto& stmt : d.funcBody) checkStatement(stmt);
    symTab_.popScope();
}

void SemanticAnalyzer::checkStructDeclaration(const AstDeclaration& d) {
    Symbol s;
    s.name = d.structName; s.kind = SymbolKind::Struct; s.declLoc = d.loc;
    symTab_.insert(s);
    auto& fields = structFields_[d.structName];
    for (auto& m : d.structMembers) fields[m.name] = resolveAstType(m.type);
}

void SemanticAnalyzer::checkInterfaceBlock(const AstDeclaration& d) {
    if (!d.blockInstanceName.empty()) {
        Symbol inst;
        inst.name = d.blockInstanceName; inst.kind = SymbolKind::Struct;
        inst.type.kind = ResolvedKind::Struct;
        inst.type.structName = d.blockInstanceName;
        inst.qualifiers = d.varQualifiers; inst.declLoc = d.loc;
        inst.binding = d.blockBinding;
        symTab_.insert(inst);
        auto& fields = structFields_[d.blockInstanceName];
        for (auto& m : d.blockMembers) fields[m.name] = resolveAstType(m.type);
        return;
    }
    for (auto& m : d.blockMembers) {
        Symbol s;
        s.name = m.name; s.kind = SymbolKind::Variable;
        s.type = resolveAstType(m.type); s.qualifiers = d.varQualifiers;
        s.isLValue = true; s.declLoc = d.loc;
        s.binding = d.blockBinding;
        symTab_.insert(s);
    }
}

SemanticResult SemanticAnalyzer::analyze(const AstTranslationUnit& tu) {
    diagnostics_.clear(); errorCount_ = 0;
    symTab_ = SymbolTable();
    exprTypes_.clear();
    exprTypes_.reserve(512);
    symTab_.pushScope(); 
    declareBuiltins();

    
    if (version_.major == 0) {
        diagError(tu.loc, "SEM-010", "thieu #version");
    }

    for (auto& decl : tu.declarations) checkDeclaration(decl);

    SemanticResult result;
    result.success = (errorCount_ == 0);
    result.diagnostics = std::move(diagnostics_);
    result.exprTypes = std::move(exprTypes_);
    return result;
}

} 
