#pragma once










#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <memory>

namespace tglmt::compiler {





struct SourceLocation {
    std::string file;
    int line = 1;
    int column = 1;
    int offset = 0;
};

struct SourceSpan {
    SourceLocation begin;
    SourceLocation end;
};

enum class DiagnosticSeverity {
    Remark,
    Warning,
    Error,
    InternalError,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string message;
    std::optional<SourceSpan> span;
    std::vector<std::string> notes;
};





enum class ScalarType {
    Void,
    Float,
    Double,
    Int,
    UInt,
    Bool,
};

enum class StorageQualifier {
    None,
    Const,
    Uniform,
    Buffer,
    In,
    Out,
    InOut,
    Patch,
    Centroid,
    Sample,
    Flat,
    Smooth,
    NoPerspective,
    Invariant,
    Precise,
    Shared,
    ReadOnly,
    WriteOnly,
    Coherent,
    Volatile,
    Restrict,
    BufferAlignOffset,
};

enum class LayoutQualifier {
    None,
    Location,
    Binding,
    Offset,
    Align,
    Rows,
    Cols,
    Stride,
    MatrixMajorRow,
    MatrixMajorColumn,
    Set,
    Stream,
    BindingIndex,
    LocalSizeX, LocalSizeY, LocalSizeZ,
    Vertices,
    Domain,
    Spacing,
    VertexOrder,
    PointMode,
    DepthGreater, DepthAny,
    Color, Index,
};



struct Type {
    enum class Kind {
        Scalar,
        Vector,
        Matrix,
        Array,
        Struct,
        Sampler,
        Image,
        AtomicCounter,
        Subroutine,
    };
    Kind kind = Kind::Scalar;
    ScalarType scalar = ScalarType::Void;
    int vectorSize = 0;
    int matrixCols = 0, matrixRows = 0;
    bool matrixRowMajor = false;
    std::vector<Type> arrayDimensions;

    struct Member {
        std::string name;
        std::unique_ptr<Type> type;
        std::vector<LayoutQualifier> qualifiers;
        size_t offsetBytes = 0;
    };
    std::vector<Member> members;
    std::string structName;

    bool isShadow = false;
    bool isArrayTex = false;
    bool isMS = false;
    bool isBuffer = false;
    int imageDim = 0;
    bool imageArray = false;

    Type() = default;
    Type(const Type& o) { copyFrom(o); }
    Type& operator=(const Type& o) {
        if (this != &o) copyFrom(o);
        return *this;
    }
    Type(Type&&) = default;
    Type& operator=(Type&&) = default;

    bool operator==(const Type& o) const {
        if (kind != o.kind || scalar != o.scalar || vectorSize != o.vectorSize ||
            matrixCols != o.matrixCols || matrixRows != o.matrixRows ||
            matrixRowMajor != o.matrixRowMajor || structName != o.structName ||
            isShadow != o.isShadow || isArrayTex != o.isArrayTex || isMS != o.isMS ||
            isBuffer != o.isBuffer || imageDim != o.imageDim || imageArray != o.imageArray ||
            arrayDimensions != o.arrayDimensions || members.size() != o.members.size())
            return false;
        for (size_t i = 0; i < members.size(); ++i) {
            if (members[i].name != o.members[i].name) return false;
            if ((members[i].type == nullptr) != (o.members[i].type == nullptr)) return false;
            if (members[i].type && !(*members[i].type == *o.members[i].type)) return false;
        }
        return true;
    }

private:
    void copyFrom(const Type& o) {
        kind = o.kind;
        scalar = o.scalar;
        vectorSize = o.vectorSize;
        matrixCols = o.matrixCols;
        matrixRows = o.matrixRows;
        matrixRowMajor = o.matrixRowMajor;
        arrayDimensions = o.arrayDimensions;
        members.clear();
        members.reserve(o.members.size());
        for (const auto& m : o.members) {
            Member c;
            c.name = m.name;
            if (m.type) c.type = std::make_unique<Type>(*m.type);
            c.qualifiers = m.qualifiers;
            c.offsetBytes = m.offsetBytes;
            members.push_back(std::move(c));
        }
        structName = o.structName;
        isShadow = o.isShadow;
        isArrayTex = o.isArrayTex;
        isMS = o.isMS;
        isBuffer = o.isBuffer;
        imageDim = o.imageDim;
        imageArray = o.imageArray;
    }
};





struct Variable {
    std::string name;
    Type type;
    std::vector<StorageQualifier> qualifiers;
    std::vector<LayoutQualifier> layoutQualifiers;
    SourceLocation declLocation;
    int location = -1;
    int binding = -1;
    int offset = -1;
    int arraySize = 0;
    bool isPatch = false;
    bool isSampler = false;
    bool isImage = false;
    bool isAtomicCounter = false;
    bool isBuffer = false;
    bool isVertexID = false;
    bool isInstanceID = false;
    bool isBaseVertex = false;
    bool isBaseInstance = false;
    bool isDrawID = false;
    bool isFrontFacing = false;
    bool isPointCoord = false;
    bool isFragCoord = false;
    bool isClipDistance = false;
    bool usesPointSize = false;
};

struct FunctionParam {
    std::string name;
    Type type;
    StorageQualifier qualifier = StorageQualifier::None;
};

struct Function {
    std::string name;
    Type returnType;
    std::vector<FunctionParam> params;
    bool isBuiltin = false;
    bool isPrototypeOnly = false;
    SourceLocation declLocation;

};





enum class ShaderStage {
    Vertex = 0x8B31,
    Fragment = 0x8B30,
    Geometry = 0x8DD9,
    TessControl = 0x8E88,
    TessEval = 0x8E87,
    Compute = 0x91B9,
};

enum class GlslProfile {
    Core,
    Compatibility,
    Legacy,
};

struct GLSLVersion {
    int major = 4;
    int minor = 60;
    bool operator==(const GLSLVersion& o) const {
        return major == o.major && minor == o.minor;
    }
    bool operator!=(const GLSLVersion& o) const { return !(*this == o); }
};

struct ExtensionSet {
    std::unordered_map<std::string, bool> enabled;
};

struct CompileOptions {
    bool defaultTessellationOuterInner = true;
    bool forceUnsignedVertexAttribute = false;

};


struct ShaderCompileResult {
    bool success = false;



    std::string moduleId;

    std::vector<uint32_t> spirv;
    std::vector<Diagnostic> diagnostics;

    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::vector<Variable> uniforms;
    std::vector<Variable> samplers;
    std::vector<Variable> images;
    std::vector<Variable> atomicCounters;
    size_t uniformBufferSize = 0;
    bool usesVertexID = false;
    bool usesInstanceID = false;
    bool isCompute = false;
    ShaderStage stage = ShaderStage::Vertex;
    int localSize[3] = {1, 1, 1};
};





struct ProgramLinkRequest {

    std::vector<ShaderCompileResult> stages;

    std::unordered_map<std::string, int> attribBindings;

    std::unordered_map<std::string, std::pair<int,int>> fragDataBindings;

    std::vector<std::string> xfbVaryings;
    bool separable = false;
    bool binaryRetrievable = true;
};

struct ProgramLinkResult {
    bool success = false;
    std::vector<Diagnostic> diagnostics;

    std::vector<ShaderCompileResult> stages;

    std::vector<Variable> activeUniforms;
    std::vector<Variable> activeAttribs;
    struct FragOutput { std::string name; int location; int index; };
    std::vector<FragOutput> fragOutputs;

    struct UniformLayoutEntry {
        std::string name;
        size_t offsetBytes = 0;
        size_t sizeBytes = 0;
        int location = -1;
        bool isVertexStage = true;
        Type type;
        int arraySize = 0;
    };
    std::vector<UniformLayoutEntry> uniformLayout;
    size_t vsUniformBytes = 0, fsUniformBytes = 0;

    struct StageBinding { std::string name; int slot; bool isStorage = false; };
    std::vector<StageBinding> vsShaderBlocks, fsShaderBlocks;

};

struct SourceOrigin {
    std::string name;
    int shaderId = 0;
};





struct Compiler {
    virtual ~Compiler() = default;

    virtual ShaderCompileResult compileShader(ShaderStage stage,
        const std::string& source,
        const GLSLVersion& version,
        GlslProfile profile,
        const ExtensionSet& extensions,
        const CompileOptions& options,
        const SourceOrigin& origin) = 0;


    virtual ProgramLinkResult linkProgram(const ProgramLinkRequest& request) = 0;
};


std::unique_ptr<Compiler> CreateCompiler();

}
