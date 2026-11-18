#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>
#include <set>

#include "tglmt/compiler/ShaderIR.h"

namespace tglmt::compiler::ir {

enum class IRTypeKind {
    Scalar, Vector, Matrix, Array, Struct, Sampler, Image, AtomicCounter,
    Void, Bool, Opaque
};

enum class IRScalar {
    Float, Double, Int, UInt, Bool, Void
};

struct IRType {
    IRTypeKind kind = IRTypeKind::Scalar;
    IRScalar scalar = IRScalar::Float;
    int vectorSize = 0;
    int matrixCols = 0, matrixRows = 0;
    bool matrixRowMajor = false;
    std::shared_ptr<IRType> arrayElement;
    int arraySize = 0;
    std::string structName;
    bool isSampler = false;
    bool isImage = false;
    bool isShadow = false;
    int imageDim = 0;
    bool isBufferTex = false;
    int samplerDim = 0;
    std::vector<std::pair<std::string, IRType>> fields;

    bool isNumeric() const;
    bool isInteger() const;
    bool isFloat() const;
    bool isBool() const;
    bool isScalar() const { return kind == IRTypeKind::Scalar; }
    bool isVector() const { return kind == IRTypeKind::Vector; }
    bool isMatrix() const { return kind == IRTypeKind::Matrix; }
    bool isOpaque() const;
    std::string toString() const;
    bool operator==(const IRType& o) const;
};

enum class IRStorage {
    Local, Uniform, Buffer, In, Out, InOut, Patch, Constant, PushConstant
};

enum class IRInterpolation {
    Smooth, Flat, NoPerspective, Centroid, Sample, None
};

struct IRConstant {
    bool isInt = true;
    long long intValue = 0;
    double floatValue = 0.0;
    bool boolValue = false;
};

struct IRValue {
    enum class Kind { Constant, Parameter, Instruction, Undef };
    Kind kind = Kind::Undef;
    IRType type;
    std::string name;
    int id = -1;
    IRConstant constant;
};

enum class IROp {
    Param, Load, Store,
    Add, Sub, Mul, Div, Mod,
    Neg, Not,
    Eq, Ne, Lt, Le, Gt, Ge,
    And, Or,
    FToI, FToU, IToF, UToF, IToU, UToI, Bitcast,
    ExtractElement, InsertElement, Shuffle, Swizzle,
    MatrixTimesVector, MatrixTimesMatrix, Transpose, OuterProduct,
    Call,
    Branch, CondBranch, Return, Discard, Phi,
    TextureSample, TextureLod, TextureGrad, TextureFetch, TextureSize,
    TextureQueryLod, TextureQueryLevels, TextureGather,
    ImageLoad, ImageStore, ImageSize,
    AtomicAdd, AtomicSub, AtomicInc, AtomicDec, AtomicMin, AtomicMax,
    AtomicAnd, AtomicOr, AtomicXor, AtomicExchange, AtomicCompareExchange,
    Barrier, MemoryBarrier,
    GetElementPtr,
};

struct IRInstruction {
    IROp op = IROp::Param;
    IRType resultType;
    int resultId = -1;
    std::vector<int> operandIds;
    IRConstant constant;
    std::string name;
    int binding = -1;
    int location = -1;
    int set = 0;
    IRStorage storage = IRStorage::Local;
    IRInterpolation interpolation = IRInterpolation::None;
    int sourceLine = -1;
    int sourceColumn = -1;
    std::string sourceFile;
};

struct IRBasicBlock {
    std::string label;
    std::vector<IRInstruction> instructions;
    std::vector<int> successors;
    int condBranchTargetTrue = -1;
    int condBranchTargetFalse = -1;
    int branchTarget = -1;
};

struct IRExpr {
    enum class Kind {
        Constant, Variable, Call, Constructor, Unary, Binary, Assign, Ternary,
        Member, Index, Swizzle, PostfixInc, PostfixDec, PrefixInc, PrefixDec
    };
    Kind kind = Kind::Variable;
    IRType type;
    IRConstant constant;
    std::string text;
    std::vector<std::shared_ptr<IRExpr>> args;
};

struct IRStmt {
    enum class Kind {
        Declaration, Expression, Block, If, For, While, DoWhile, Return,
        Discard, Break, Continue
    };
    Kind kind = Kind::Block;
    IRType declType;
    std::string declName;
    std::shared_ptr<IRExpr> init;
    std::shared_ptr<IRExpr> expr;
    std::shared_ptr<IRExpr> cond;
    std::shared_ptr<IRExpr> step;
    std::shared_ptr<IRStmt> forInit;
    std::vector<std::shared_ptr<IRStmt>> body;
    std::shared_ptr<IRStmt> thenS;
    std::shared_ptr<IRStmt> elseS;
};

struct IRFunction {
    std::string name;
    IRType returnType;
    std::vector<IRType> paramTypes;
    std::vector<std::string> paramNames;
    std::vector<IRStorage> paramStorages;
    std::vector<IRBasicBlock> blocks;
    std::vector<std::shared_ptr<IRStmt>> body;
    bool isEntryPoint = false;
    bool isBuiltin = false;
    bool usesDiscard = false;
};

struct IRModule {
    std::string name;
    ShaderStage stage = ShaderStage::Vertex;
    GLSLVersion version{};
    GlslProfile profile = GlslProfile::Core;
    std::vector<IRFunction> functions;
    struct IRResource {
        std::string name;
        IRType type;
        IRStorage storage;
        int binding = -1;
        int location = -1;
        int set = 0;
        bool isArray = false;
        int arraySize = 0;
        bool isAnonymousBlock = false;
    };
    struct IRGlobal {
        std::string name;
        IRType type;
        IRStorage storage = IRStorage::Constant;
        std::shared_ptr<IRExpr> init;
    };
    std::vector<std::pair<std::string, IRType>> structs;
    std::vector<IRGlobal> globals;
    std::vector<IRResource> uniforms;
    std::vector<IRResource> samplers;
    std::vector<IRResource> images;
    std::vector<IRResource> atomicCounters;
    std::vector<IRResource> storageBlocks;
    std::vector<IRResource> inputs;
    std::vector<IRResource> outputs;
    bool isCompute = false;
    int localSizeX = 1, localSizeY = 1, localSizeZ = 1;
    bool isTessControl = false;
    bool isTessEval = false;
    int tessVertices = 0;
    int tessPatchType = 0;
    int tessSpacing = 0;
    int tessVertexOrder = 0;
    std::vector<Diagnostic> diagnostics;
    bool success = false;
    std::unordered_map<int, IRConstant> constantValues;
};

struct IRVerifyResult {
    bool valid = false;
    std::vector<Diagnostic> diagnostics;
};

IRVerifyResult verifyModule(const IRModule& module);
IRVerifyResult verifyFunction(const IRFunction& func);
IRVerifyResult verifyBlock(const IRBasicBlock& block, const IRFunction& func);

} 
