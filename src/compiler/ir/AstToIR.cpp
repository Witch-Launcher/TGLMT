#include "tglmt/compiler/ir/AstToIR.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace tglmt::compiler::ir {

namespace {

IRType irTypeFromBase(const std::string& base) {
    IRType t;
    if (base == "float") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::Float; }
    else if (base == "int") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::Int; }
    else if (base == "uint") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::UInt; }
    else if (base == "bool") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::Bool; }
    else if (base == "void") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::Void; }
    else if (base == "double") { t.kind = IRTypeKind::Scalar; t.scalar = IRScalar::Double; }
    else if (base == "vec2") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Float; t.vectorSize = 2; }
    else if (base == "vec3") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Float; t.vectorSize = 3; }
    else if (base == "vec4") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Float; t.vectorSize = 4; }
    else if (base == "ivec2") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Int; t.vectorSize = 2; }
    else if (base == "ivec3") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Int; t.vectorSize = 3; }
    else if (base == "ivec4") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Int; t.vectorSize = 4; }
    else if (base == "uvec2") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::UInt; t.vectorSize = 2; }
    else if (base == "uvec3") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::UInt; t.vectorSize = 3; }
    else if (base == "uvec4") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::UInt; t.vectorSize = 4; }
    else if (base == "bvec2") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Bool; t.vectorSize = 2; }
    else if (base == "bvec3") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Bool; t.vectorSize = 3; }
    else if (base == "bvec4") { t.kind = IRTypeKind::Vector; t.scalar = IRScalar::Bool; t.vectorSize = 4; }
    else if (base == "mat2") { t.kind = IRTypeKind::Matrix; t.scalar = IRScalar::Float; t.matrixCols = 2; t.matrixRows = 2; }
    else if (base == "mat3") { t.kind = IRTypeKind::Matrix; t.scalar = IRScalar::Float; t.matrixCols = 3; t.matrixRows = 3; }
    else if (base == "mat4") { t.kind = IRTypeKind::Matrix; t.scalar = IRScalar::Float; t.matrixCols = 4; t.matrixRows = 4; }
    else if (base == "mat2x2" || base == "mat2x3" || base == "mat2x4" ||
             base == "mat3x2" || base == "mat3x3" || base == "mat3x4" ||
             base == "mat4x2" || base == "mat4x3" || base == "mat4x4") {
        t.kind = IRTypeKind::Matrix; t.scalar = IRScalar::Float;
        t.matrixCols = base[3] - '0';
        t.matrixRows = base[5] - '0';
    }
    else if (base.rfind("sampler", 0) == 0 || base.rfind("isampler", 0) == 0 ||
             base.rfind("usampler", 0) == 0) {
        t.kind = IRTypeKind::Sampler; t.isSampler = true;
        std::string rest;
        if (base.rfind("isampler", 0) == 0) { rest = base.substr(8); t.scalar = IRScalar::Int; }
        else if (base.rfind("usampler", 0) == 0) { rest = base.substr(8); t.scalar = IRScalar::UInt; }
        else { rest = base.substr(7); t.scalar = IRScalar::Float; }
        if (rest == "1D") t.samplerDim = 1;
        else if (rest == "2D") t.samplerDim = 2;
        else if (rest == "3D") t.samplerDim = 3;
        else if (rest == "Cube") t.samplerDim = 4;
        else if (rest == "2DRect") t.samplerDim = 5;
        else if (rest == "1DArray") t.samplerDim = 6;
        else if (rest == "2DArray") t.samplerDim = 7;
        else if (rest == "CubeArray") t.samplerDim = 8;
        else if (rest == "Buffer") { t.samplerDim = 9; t.isBufferTex = true; }
        else if (rest == "2DMS") t.samplerDim = 10;
        else if (rest == "2DMSArray") t.samplerDim = 11;
        else if (rest == "1DShadow") { t.samplerDim = 1; t.isShadow = true; }
        else if (rest == "2DShadow") { t.samplerDim = 2; t.isShadow = true; }
        else if (rest == "2DRectShadow") { t.samplerDim = 5; t.isShadow = true; }
        else if (rest == "CubeShadow") { t.samplerDim = 4; t.isShadow = true; }
        else if (rest == "1DArrayShadow") { t.samplerDim = 6; t.isShadow = true; }
        else if (rest == "2DArrayShadow") { t.samplerDim = 7; t.isShadow = true; }
    }
    else if (base.rfind("image", 0) == 0 || base.rfind("iimage", 0) == 0 ||
             base.rfind("uimage", 0) == 0) {
        t.kind = IRTypeKind::Image; t.isImage = true;
        if (base.rfind("iimage", 0) == 0) t.scalar = IRScalar::Int;
        else if (base.rfind("uimage", 0) == 0) t.scalar = IRScalar::UInt;
        else t.scalar = IRScalar::Float;
        if (base.find("2DArray") != std::string::npos) t.imageDim = 7;
        else if (base.find("2D") != std::string::npos) t.imageDim = 2;
        else if (base.find("3D") != std::string::npos) t.imageDim = 3;
        else if (base.find("Cube") != std::string::npos) t.imageDim = 4;
        else if (base.find("Buffer") != std::string::npos) t.imageDim = 9;
    }
    else {
        t.kind = IRTypeKind::Struct;
        t.structName = base;
    }
    return t;
}

IRType irTypeFromAst(const AstType& ty) {
    IRType t = irTypeFromBase(ty.baseType);
    if (ty.isArray) {
        IRType elem = t;
        t.kind = IRTypeKind::Array;
        t.arrayElement = std::make_shared<IRType>(elem);
        t.arraySize = ty.arraySizes.empty() ? -1 : ty.arraySizes[0];
    }
    return t;
}

IRType irTypeFromResolved(const ResolvedType& rt) {
    IRType t;
    switch (rt.kind) {
        case ResolvedKind::Scalar:
        case ResolvedKind::Vector:
        case ResolvedKind::Matrix:
            t.kind = rt.kind == ResolvedKind::Scalar ? IRTypeKind::Scalar
                   : rt.kind == ResolvedKind::Vector ? IRTypeKind::Vector
                                                     : IRTypeKind::Matrix;
            t.vectorSize = rt.vectorSize;
            t.matrixCols = rt.matrixCols;
            t.matrixRows = rt.matrixRows;
            switch (rt.scalar) {
                case ResolvedScalar::Float: t.scalar = IRScalar::Float; break;
                case ResolvedScalar::Double: t.scalar = IRScalar::Double; break;
                case ResolvedScalar::Int: t.scalar = IRScalar::Int; break;
                case ResolvedScalar::UInt: t.scalar = IRScalar::UInt; break;
                case ResolvedScalar::Bool: t.scalar = IRScalar::Bool; break;
                case ResolvedScalar::Void: t.scalar = IRScalar::Void; break;
            }
            break;
        case ResolvedKind::Array:
            t.kind = IRTypeKind::Array;
            if (rt.arrayElement) t.arrayElement = std::make_shared<IRType>(irTypeFromResolved(*rt.arrayElement));
            t.arraySize = rt.arraySize;
            break;
        case ResolvedKind::Struct:
            t.kind = IRTypeKind::Struct;
            t.structName = rt.structName;
            break;
        case ResolvedKind::Sampler:
            t.kind = IRTypeKind::Sampler; t.isSampler = true;
            t.isShadow = rt.isShadow; t.samplerDim = rt.samplerDim;
            t.isBufferTex = rt.isBufferTex;
            switch (rt.samplerScalar) {
                case ResolvedScalar::Int: t.scalar = IRScalar::Int; break;
                case ResolvedScalar::UInt: t.scalar = IRScalar::UInt; break;
                default: t.scalar = IRScalar::Float; break;
            }
            break;
        case ResolvedKind::Image:
            t.kind = IRTypeKind::Image; t.isImage = true; t.imageDim = rt.imageDim;
            break;
        case ResolvedKind::AtomicCounter:
            t.kind = IRTypeKind::AtomicCounter;
            break;
        default:
            t.kind = IRTypeKind::Opaque;
            break;
    }
    return t;
}

struct ModuleBuilder {
    IRModule& module;
    const SemanticResult& sem;
    std::unordered_map<std::string, IRType> structTypes;

    bool isSamplerType(const IRType& t) const { return t.isSampler; }

    std::shared_ptr<IRExpr> expr(const AstExprPtr& e) {
        if (!e) return nullptr;
        auto out = std::make_shared<IRExpr>();
        auto typeIt = sem.exprTypes.find(e.get());
        if (typeIt != sem.exprTypes.end()) out->type = irTypeFromResolved(typeIt->second);
        switch (e->kind) {
            case AstExprKind::Integer:
                out->kind = IRExpr::Kind::Constant;
                out->constant.isInt = true; out->constant.intValue = e->intValue;
                if (out->type.scalar == IRScalar::Void) { out->type.kind = IRTypeKind::Scalar; out->type.scalar = IRScalar::Int; }
                break;
            case AstExprKind::Uint:
                out->kind = IRExpr::Kind::Constant;
                out->constant.isInt = true; out->constant.intValue = e->intValue;
                if (out->type.scalar == IRScalar::Void) { out->type.kind = IRTypeKind::Scalar; out->type.scalar = IRScalar::UInt; }
                break;
            case AstExprKind::Float:
                out->kind = IRExpr::Kind::Constant;
                out->constant.isInt = false; out->constant.floatValue = e->floatValue;
                if (out->type.scalar == IRScalar::Void) { out->type.kind = IRTypeKind::Scalar; out->type.scalar = IRScalar::Float; }
                break;
            case AstExprKind::Double:
                out->kind = IRExpr::Kind::Constant;
                out->constant.isInt = false; out->constant.floatValue = e->floatValue;
                out->type.kind = IRTypeKind::Scalar; out->type.scalar = IRScalar::Double;
                break;
            case AstExprKind::Bool:
                out->kind = IRExpr::Kind::Constant;
                out->constant.isInt = true; out->constant.intValue = e->boolValue ? 1 : 0;
                out->constant.boolValue = e->boolValue;
                if (out->type.scalar == IRScalar::Void) { out->type.kind = IRTypeKind::Scalar; out->type.scalar = IRScalar::Bool; }
                break;
            case AstExprKind::Identifier:
                out->kind = IRExpr::Kind::Variable;
                out->text = e->name;
                break;
            case AstExprKind::Binary: {
                out->text = e->op;
                out->args.push_back(expr(e->left));
                out->args.push_back(expr(e->right));
                if (e->op == "=" || e->op == "+=" || e->op == "-=" || e->op == "*=" ||
                    e->op == "/=" || e->op == "%=")
                    out->kind = IRExpr::Kind::Assign;
                else
                    out->kind = IRExpr::Kind::Binary;
                break;
            }
            case AstExprKind::Unary: {
                out->text = e->op;
                out->args.push_back(expr(e->operand));
                if (e->op == "++")
                    out->kind = e->isPostfix ? IRExpr::Kind::PostfixInc : IRExpr::Kind::PrefixInc;
                else if (e->op == "--")
                    out->kind = e->isPostfix ? IRExpr::Kind::PostfixDec : IRExpr::Kind::PrefixDec;
                else
                    out->kind = IRExpr::Kind::Unary;
                break;
            }
            case AstExprKind::Call: {
                out->kind = IRExpr::Kind::Call;
                out->text = e->callee;
                for (auto& a : e->args) out->args.push_back(expr(a));
                break;
            }
            case AstExprKind::Constructor: {
                out->kind = IRExpr::Kind::Constructor;
                out->text = e->ctorType;
                for (auto& a : e->ctorArgs) out->args.push_back(expr(a));
                if (e->ctorIsArray) {
                    if (out->type.kind != IRTypeKind::Array) {
                        out->type.kind = IRTypeKind::Array;
                        out->type.arrayElement = std::make_shared<IRType>(irTypeFromBase(e->ctorType));
                    }
                    out->type.arraySize = e->ctorArraySize > 0 ? e->ctorArraySize
                                                               : (int)e->ctorArgs.size();
                }
                break;
            }
            case AstExprKind::Member: {
                out->kind = IRExpr::Kind::Member;
                out->text = e->member;
                out->args.push_back(expr(e->object));
                break;
            }
            case AstExprKind::Swizzle: {
                out->kind = IRExpr::Kind::Swizzle;
                out->text = e->swizzle;
                out->args.push_back(expr(e->object));
                break;
            }
            case AstExprKind::Index: {
                out->kind = IRExpr::Kind::Index;
                out->args.push_back(expr(e->object));
                out->args.push_back(expr(e->indices.empty() ? nullptr : e->indices[0]));
                break;
            }
            case AstExprKind::Ternary: {
                out->kind = IRExpr::Kind::Ternary;
                out->args.push_back(expr(e->cond));
                out->args.push_back(expr(e->thenExpr));
                out->args.push_back(expr(e->elseExpr));
                break;
            }
            case AstExprKind::Paren:
                return expr(e->left);
            default:
                out->kind = IRExpr::Kind::Constant;
                out->type.kind = IRTypeKind::Scalar;
                out->type.scalar = IRScalar::Int;
                out->constant.intValue = 0;
                break;
        }
        return out;
    }

    std::shared_ptr<IRStmt> stmt(const AstStmtPtr& s) {
        if (!s) return nullptr;
        auto out = std::make_shared<IRStmt>();
        switch (s->kind) {
            case AstStmtKind::Block: {
                out->kind = IRStmt::Kind::Block;
                for (auto& bs : s->blockStmts) out->body.push_back(stmt(bs));
                break;
            }
            case AstStmtKind::Declaration: {
                if (!s->decl || s->decl->kind != AstDeclKind::Variable) break;
                out->kind = IRStmt::Kind::Declaration;
                out->declName = s->decl->varName;
                out->declType = irTypeFromAst(s->decl->varType);
                out->init = expr(s->decl->varInit);
                break;
            }
            case AstStmtKind::Expression:
                out->kind = IRStmt::Kind::Expression;
                out->expr = expr(s->exprStmt);
                break;
            case AstStmtKind::Return:
                out->kind = IRStmt::Kind::Return;
                out->expr = expr(s->returnExpr);
                break;
            case AstStmtKind::Discard:
                out->kind = IRStmt::Kind::Discard;
                break;
            case AstStmtKind::If:
                out->kind = IRStmt::Kind::If;
                out->cond = expr(s->ifCond);
                out->thenS = stmt(s->ifThen);
                out->elseS = stmt(s->ifElse);
                break;
            case AstStmtKind::For:
                out->kind = IRStmt::Kind::For;
                if (s->loopInitDecl) {
                    auto init = std::make_shared<IRStmt>();
                    init->kind = IRStmt::Kind::Declaration;
                    init->declName = s->loopInitDecl->varName;
                    init->declType = irTypeFromAst(s->loopInitDecl->varType);
                    init->init = expr(s->loopInitDecl->varInit);
                    out->forInit = init;
                } else if (s->loopInit) {
                    auto init = std::make_shared<IRStmt>();
                    init->kind = IRStmt::Kind::Expression;
                    init->expr = expr(s->loopInit);
                    out->forInit = init;
                }
                out->cond = expr(s->loopCond);
                out->step = expr(s->loopInc);
                if (s->loopBody) out->body.push_back(stmt(s->loopBody));
                break;
            case AstStmtKind::While:
                out->kind = IRStmt::Kind::While;
                out->cond = expr(s->loopCond);
                if (s->loopBody) out->body.push_back(stmt(s->loopBody));
                break;
            case AstStmtKind::DoWhile:
                out->kind = IRStmt::Kind::DoWhile;
                out->cond = expr(s->loopCond);
                if (s->loopBody) out->body.push_back(stmt(s->loopBody));
                break;
            case AstStmtKind::Break:
                out->kind = IRStmt::Kind::Break;
                break;
            case AstStmtKind::Continue:
                out->kind = IRStmt::Kind::Continue;
                break;
        }
        return out;
    }

    IRModule::IRResource resourceFrom(const AstDeclaration& d) {
        IRModule::IRResource res;
        res.name = d.varName;
        res.type = irTypeFromAst(d.varType);
        if (res.type.kind == IRTypeKind::Struct) {
            auto it = structTypes.find(res.type.structName);
            if (it != structTypes.end()) res.type.fields = it->second.fields;
        }
        if (d.varBinding > 0) res.binding = d.varBinding;
        if (d.varLocation >= 0) res.location = d.varLocation;
        return res;
    }
};

} 

IRModule astToModule(const AstTranslationUnit& tu,
                     const SemanticResult& sem,
                     ShaderStage stage) {
    IRModule module;
    module.stage = stage;
    module.version = tu.version;
    module.profile = tu.profile;
    module.success = sem.success;
    module.diagnostics = sem.diagnostics;
    module.isCompute = stage == ShaderStage::Compute;

    ModuleBuilder builder{module, sem};
    std::set<std::string> seenStructs, seenResources, seenFunctions;

    for (const auto& d : tu.declarations) {
        if (d.kind != AstDeclKind::Struct) continue;
        if (!seenStructs.insert(d.structName).second) continue;
        IRType st;
        st.kind = IRTypeKind::Struct;
        st.structName = d.structName;
        for (const auto& m : d.structMembers)
            st.fields.emplace_back(m.name, irTypeFromAst(m.type));
        builder.structTypes[d.structName] = st;
        module.structs.emplace_back(d.structName, st);
    }

    for (const auto& d : tu.declarations) {
        if (d.kind == AstDeclKind::InterfaceBlock) {
            IRModule::IRResource res;
            res.name = d.blockInstanceName.empty() ? d.blockName : d.blockInstanceName;
            if (!seenResources.insert("block:" + res.name).second) continue;
            res.type.kind = IRTypeKind::Struct;
            res.type.structName = d.blockName;
            for (const auto& m : d.blockMembers)
                res.type.fields.emplace_back(m.name, irTypeFromAst(m.type));
            bool isUniform = std::find(d.varQualifiers.begin(), d.varQualifiers.end(),
                                       StorageQualifier::Uniform) != d.varQualifiers.end();
            res.storage = isUniform ? IRStorage::Uniform : IRStorage::Buffer;
            res.isAnonymousBlock = d.blockInstanceName.empty();
            if (d.blockBinding > 0) res.binding = d.blockBinding;
            module.structs.emplace_back(d.blockName, res.type);
            module.uniforms.push_back(std::move(res));
        } else if (d.kind == AstDeclKind::Variable) {
            if (!seenResources.insert("var:" + d.varName).second) continue;
            IRModule::IRResource res = builder.resourceFrom(d);
            bool isUniform = d.varIsUniform;
            bool isBuffer = d.varIsBuffer;
            bool isIn = std::find(d.varQualifiers.begin(), d.varQualifiers.end(), StorageQualifier::In) != d.varQualifiers.end();
            bool isOut = std::find(d.varQualifiers.begin(), d.varQualifiers.end(), StorageQualifier::Out) != d.varQualifiers.end();
            bool isConst = std::find(d.varQualifiers.begin(), d.varQualifiers.end(), StorageQualifier::Const) != d.varQualifiers.end();
            if (isUniform && res.type.isSampler) {
                res.storage = IRStorage::Uniform;
                module.samplers.push_back(std::move(res));
            } else if (isUniform && res.type.isImage) {
                res.storage = IRStorage::Uniform;
                module.images.push_back(std::move(res));
            } else if (isUniform) {
                res.storage = IRStorage::Uniform;
                module.uniforms.push_back(std::move(res));
            } else if (isBuffer) {
                res.storage = IRStorage::Buffer;
                module.storageBlocks.push_back(std::move(res));
            } else if (isIn) {
                res.storage = IRStorage::In;
                module.inputs.push_back(std::move(res));
            } else if (isOut) {
                res.storage = IRStorage::Out;
                module.outputs.push_back(std::move(res));
            } else {
                IRModule::IRGlobal g;
                g.name = d.varName;
                g.type = res.type;
                g.storage = isConst ? IRStorage::Constant : IRStorage::Local;
                g.init = builder.expr(d.varInit);
                module.globals.push_back(std::move(g));
            }
        }
    }

    for (const auto& d : tu.declarations) {
        if (d.kind != AstDeclKind::Function) continue;
        if (d.funcBody.empty()) continue;
        std::string sig = d.funcName;
        for (const auto& p : d.funcParams) sig += ":" + p.type.baseType;
        if (!seenFunctions.insert(sig).second) continue;
        IRFunction func;
        func.name = d.funcName;
        func.returnType = irTypeFromBase(d.funcReturnType.baseType);
        for (const auto& p : d.funcParams) {
            func.paramTypes.push_back(irTypeFromAst(p.type));
            func.paramNames.push_back(p.name);
            IRStorage st = IRStorage::In;
            if (p.qualifier == StorageQualifier::Out) st = IRStorage::Out;
            else if (p.qualifier == StorageQualifier::InOut) st = IRStorage::InOut;
            func.paramStorages.push_back(st);
        }
        func.isEntryPoint = d.funcName == "main";
        for (const auto& st : d.funcBody) func.body.push_back(builder.stmt(st));
        module.functions.push_back(std::move(func));
    }

    return module;
}

} 
