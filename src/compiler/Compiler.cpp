#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/compiler/Preprocessor.h"
#include "tglmt/compiler/Lexer.h"
#include "tglmt/compiler/AST.h"
#include "tglmt/compiler/Semantic.h"
#include "tglmt/compiler/ir/IR.h"
#include "tglmt/compiler/ir/AstToIR.h"
#include "tglmt/compiler/passes/Pass.h"
#include "tglmt/compiler/passes/ConstantFolding.h"
#include "tglmt/compiler/passes/DCE.h"
#include "tglmt/compiler/passes/Canonicalization.h"
#include "tglmt/compiler/passes/LayoutLowering.h"
#include "tglmt/compiler/backend/metal/MSLBackend.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tglmt::compiler {

namespace {

static Type convertIRType(const ir::IRType& t) {
    Type out;
    switch (t.kind) {
        case ir::IRTypeKind::Scalar: {
            out.kind = Type::Kind::Scalar;
            switch (t.scalar) {
                case ir::IRScalar::Float: out.scalar = ScalarType::Float; break;
                case ir::IRScalar::Double: out.scalar = ScalarType::Double; break;
                case ir::IRScalar::Int: out.scalar = ScalarType::Int; break;
                case ir::IRScalar::UInt: out.scalar = ScalarType::UInt; break;
                case ir::IRScalar::Bool: out.scalar = ScalarType::Bool; break;
                default: out.scalar = ScalarType::Void; break;
            }
            break;
        }
        case ir::IRTypeKind::Vector:
            out.kind = Type::Kind::Vector;
            out.scalar = t.scalar == ir::IRScalar::Int ? ScalarType::Int
                       : t.scalar == ir::IRScalar::UInt ? ScalarType::UInt
                       : t.scalar == ir::IRScalar::Bool ? ScalarType::Bool
                       : ScalarType::Float;
            out.vectorSize = t.vectorSize;
            break;
        case ir::IRTypeKind::Matrix:
            out.kind = Type::Kind::Matrix;
            out.scalar = ScalarType::Float;
            out.matrixCols = t.matrixCols;
            out.matrixRows = t.matrixRows;
            out.matrixRowMajor = t.matrixRowMajor;
            break;
        case ir::IRTypeKind::Array:
            if (t.arrayElement) out = convertIRType(*t.arrayElement);
            out.kind = Type::Kind::Array;
            out.arrayDimensions.push_back(Type{});
            out.arrayDimensions.back().scalar = ScalarType::Int;
            out.arrayDimensions.back().vectorSize = t.arraySize > 0 ? t.arraySize : 0;
            break;
        case ir::IRTypeKind::Struct:
            out.kind = Type::Kind::Struct;
            out.structName = t.structName;
            break;
        case ir::IRTypeKind::Sampler:
            out.kind = Type::Kind::Sampler;
            out.scalar = t.scalar == ir::IRScalar::Int ? ScalarType::Int
                       : t.scalar == ir::IRScalar::UInt ? ScalarType::UInt
                       : ScalarType::Float;
            out.isShadow = t.isShadow;
            out.isArrayTex = t.isBufferTex;
            out.isMS = t.samplerDim == 10 || t.samplerDim == 11;
            out.isBuffer = t.samplerDim == 9;
            break;
        case ir::IRTypeKind::Image:
            out.kind = Type::Kind::Image;
            out.imageDim = t.imageDim;
            out.imageArray = t.samplerDim == 7 || t.samplerDim == 11;
            break;
        default:
            out.kind = Type::Kind::Scalar;
            out.scalar = ScalarType::Float;
            break;
    }
    return out;
}

static void fillVariables(std::vector<Variable>& dst, const std::vector<ir::IRModule::IRResource>& src) {
    for (const auto& r : src) {
        Variable v;
        v.name = r.name;
        v.type = convertIRType(r.type);
        v.location = r.location;
        v.binding = r.binding;
        v.isSampler = r.type.isSampler;
        v.isImage = r.type.isImage;
        dst.push_back(v);
    }
}

class RealCompiler : public Compiler {
public:
    ShaderCompileResult compileShader(ShaderStage stage,
        const std::string& source,
        const GLSLVersion& version,
        GlslProfile profile,
        const ExtensionSet& extensions,
        const CompileOptions& options,
        const SourceOrigin& origin) override {
        (void)options;
        ShaderCompileResult r;
        r.moduleId = origin.name;
        r.isCompute = (stage == ShaderStage::Compute);
        r.stage = stage;

        
        Preprocessor pp(source, origin.name);
        PreprocessResult pres = pp.run();
        r.diagnostics.insert(r.diagnostics.end(), pres.diagnostics.begin(), pres.diagnostics.end());
        if (!pres.success) {
            r.success = false;
            return r;
        }

        
        Lexer lex(pres.source, origin.name);
        std::vector<Token> tokens = lex.tokenize();

        
        Parser parser(std::move(tokens), origin.name);
        AstTranslationUnit tu = parser.parse();
        r.diagnostics.insert(r.diagnostics.end(), tu.diagnostics.begin(), tu.diagnostics.end());
        if (!tu.success) {
            r.success = false;
            return r;
        }

        
        SemanticAnalyzer sema(stage, pres.version, pres.profile, pres.extensions);
        SemanticResult sem = sema.analyze(tu);
        r.diagnostics.insert(r.diagnostics.end(), sem.diagnostics.begin(), sem.diagnostics.end());
        if (!sem.success) {
            r.success = false;
            return r;
        }

        
        ir::IRModule module = ir::astToModule(tu, sem, stage);
        module.success = sem.success;
        module.diagnostics = sem.diagnostics;

        
        std::vector<std::unique_ptr<passes::Pass>> passes;
        passes.push_back(std::make_unique<passes::ConstantFolding>());
        passes.push_back(std::make_unique<passes::DCE>());
        passes.push_back(std::make_unique<passes::Canonicalization>());
        passes.push_back(std::make_unique<passes::LayoutLowering>());

        for (auto& p : passes) {
            if (!p->applicable(module)) continue;
            auto pres2 = p->run(module);
            if (pres2 == passes::PassResult::Error) {
                Diagnostic d;
                d.severity = DiagnosticSeverity::Error;
                d.code = "IR-003";
                d.message = "pass '" + p->info().name + "' that bai";
                module.diagnostics.push_back(d);
                r.diagnostics.push_back(d);
                r.success = false;
                return r;
            }
            if (p->info().requiresVerifier) {
                auto vr = ir::verifyModule(module);
                if (!vr.valid) {
                    module.diagnostics.insert(module.diagnostics.end(), vr.diagnostics.begin(), vr.diagnostics.end());
                    r.diagnostics.insert(r.diagnostics.end(), vr.diagnostics.begin(), vr.diagnostics.end());
                    r.success = false;
                    return r;
                }
            }
        }

        
        backend::metal::MSLBackend msl;
        backend::metal::MSLCompileOptions opts;
        opts.emitSourceOnly = true; 
        backend::metal::MSLCompileResult mres = msl.build(module, opts);
        r.diagnostics.insert(r.diagnostics.end(), mres.diagnostics.begin(), mres.diagnostics.end());
        if (!mres.success) {
            r.success = false;
            return r;
        }

        fillVariables(r.inputs, module.inputs);
        fillVariables(r.outputs, module.outputs);
        fillVariables(r.uniforms, module.uniforms);
        fillVariables(r.samplers, module.samplers);
        fillVariables(r.images, module.images);
        for (const auto& sb : module.storageBlocks) {
            Variable v;
            v.name = sb.name;
            v.type = convertIRType(sb.type);
            v.binding = sb.binding;
            v.isBuffer = true;
            r.uniforms.push_back(v);
        }
        r.isCompute = module.isCompute;
        r.localSize[0] = module.localSizeX;
        r.localSize[1] = module.localSizeY;
        r.localSize[2] = module.localSizeZ;
        r.success = true;
        
        r.moduleId = mres.mslSource; 
        return r;
    }

    ProgramLinkResult linkProgram(const ProgramLinkRequest& request) override {
        ProgramLinkResult r;
        r.success = true;
        const ShaderCompileResult* vs = nullptr;
        const ShaderCompileResult* fs = nullptr;
        for (const auto& s : request.stages) {
            r.diagnostics.insert(r.diagnostics.end(), s.diagnostics.begin(), s.diagnostics.end());
            if (!s.success) r.success = false;
            if (s.isCompute) continue;
            const std::string& name = s.moduleId;
            (void)name;
        }
        for (const auto& s : request.stages) {
            if (!s.success) continue;
            if (s.stage == ShaderStage::Vertex) vs = &s;
            else if (s.stage == ShaderStage::Fragment) fs = &s;
        }
        if (vs && fs) {
            for (const auto& fi : fs->inputs) {
                if (fi.name.empty() || fi.name.rfind("gl_", 0) == 0) continue;
                const Variable* match = nullptr;
                for (const auto& vo : vs->outputs)
                    if (vo.name == fi.name) { match = &vo; break; }
                if (!match) {
                    Diagnostic d;
                    d.severity = DiagnosticSeverity::Error;
                    d.code = "LNK-001";
                    d.message = "fragment input `" + fi.name +
                                "` khong co vertex output tuong ung";
                    r.diagnostics.push_back(d);
                    r.success = false;
                    continue;
                }
                bool same = match->type == fi.type;
                if (!same) {
                    Diagnostic d;
                    d.severity = DiagnosticSeverity::Error;
                    d.code = "LNK-002";
                    d.message = "kieu interface `" + fi.name +
                                "` khong khop giua vertex output va fragment input";
                    r.diagnostics.push_back(d);
                    r.success = false;
                }
            }
        }
        for (const auto& s : request.stages)
            for (const auto& u : s.uniforms) r.activeUniforms.push_back(u);
        if (vs)
            for (const auto& a : vs->inputs) r.activeAttribs.push_back(a);
        return r;
    }
};

} 

std::unique_ptr<Compiler> CreateCompiler() {
    return std::make_unique<RealCompiler>();
}

} 
