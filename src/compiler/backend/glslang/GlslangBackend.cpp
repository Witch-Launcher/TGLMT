#include "tglmt/compiler/backend/glslang/GlslangBackend.h"

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_msl.hpp>

#include <algorithm>
#include <map>
#include <regex>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tglmt::compiler::backend::glslang {

namespace {

using namespace tglmt::compiler;
namespace glslang_api = ::glslang;

void EnsureGlslang() {
    static std::once_flag once;
    std::call_once(once, [] { glslang_api::InitializeProcess(); });
}

EShLanguage ToGlslangStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex: return EShLangVertex;
        case ShaderStage::Fragment: return EShLangFragment;
        case ShaderStage::Compute: return EShLangCompute;
        case ShaderStage::Geometry: return EShLangGeometry;
        case ShaderStage::TessControl: return EShLangTessControl;
        case ShaderStage::TessEval: return EShLangTessEvaluation;
    }
    return EShLangVertex;
}

ScalarType ScalarFromSpv(spirv_cross::SPIRType::BaseType b) {
    switch (b) {
        case spirv_cross::SPIRType::Int: return ScalarType::Int;
        case spirv_cross::SPIRType::UInt: return ScalarType::UInt;
        case spirv_cross::SPIRType::Boolean: return ScalarType::Bool;
        case spirv_cross::SPIRType::Double: return ScalarType::Double;
        default: return ScalarType::Float;
    }
}

Type TypeFromSpv(const spirv_cross::SPIRType& t) {
    Type out;
    if (t.basetype == spirv_cross::SPIRType::Struct) {
        out.kind = Type::Kind::Struct;
        return out;
    }
    out.scalar = ScalarFromSpv(t.basetype);
    if (t.columns > 1) {
        out.kind = Type::Kind::Matrix;
        out.matrixCols = (int)t.columns;
        out.matrixRows = (int)t.vecsize;
    } else if (t.vecsize > 1) {
        out.kind = Type::Kind::Vector;
        out.vectorSize = (int)t.vecsize;
    } else {
        out.kind = Type::Kind::Scalar;
    }
    for (int i = (int)t.array.size() - 1; i >= 0; --i) {
        Type arr;
        arr.kind = Type::Kind::Array;
        arr.arrayDimensions.push_back(Type{});
        arr.arrayDimensions.back().scalar = ScalarType::Int;
        arr.arrayDimensions.back().vectorSize = (int)t.array[i];
        out.arrayDimensions.insert(out.arrayDimensions.begin(), arr.arrayDimensions.begin(),
                                   arr.arrayDimensions.end());
    }
    return out;
}

void FillResource(const spirv_cross::CompilerMSL& msl, const spirv_cross::Resource& r,
                  std::vector<Variable>& dst, bool sampler, bool image, bool buffer) {
    Variable v;
    v.name = msl.get_name(r.id);
    if (v.name.empty()) v.name = msl.get_fallback_name(r.id);
    const auto& t = msl.get_type(r.type_id);
    v.type = TypeFromSpv(t);
    if (sampler) {
        v.type.kind = Type::Kind::Sampler;
        v.type.scalar = ScalarFromSpv(msl.get_type(t.image.type).basetype);
        v.type.isShadow = t.image.depth != 0;
        v.type.isArrayTex = t.image.arrayed != 0;
        v.type.isMS = t.image.ms != 0;
        v.type.isBuffer = t.image.dim == spv::DimBuffer;
        v.isSampler = true;
    } else if (image) {
        v.type.kind = Type::Kind::Image;
        v.isImage = true;
    }
    v.isBuffer = buffer;
    if (msl.has_decoration(r.id, spv::DecorationLocation))
        v.location = (int)msl.get_decoration(r.id, spv::DecorationLocation);
    if (msl.has_decoration(r.id, spv::DecorationBinding))
        v.binding = (int)msl.get_decoration(r.id, spv::DecorationBinding);
    dst.push_back(v);
}

const char* FirstLog(glslang_api::TShader& shader) {
    const char* log = shader.getInfoLog();
    if (log && log[0]) return log;
    log = shader.getInfoDebugLog();
    return log ? log : "";
}

bool IsUserVarying(const std::string& name) {
    return !name.empty() && name.rfind("gl_", 0) != 0;
}

uint32_t LocationSlots(const Type& t) {
    if (t.kind == Type::Kind::Matrix) return (uint32_t)std::max(1, t.matrixCols);
    uint32_t n = 1;
    if (t.kind == Type::Kind::Array) {
        for (const auto& d : t.arrayDimensions) n *= (uint32_t)std::max(1, d.vectorSize);
    }
    return n;
}

std::unordered_map<std::string, int> UnionLocations(const std::vector<Variable>& vsOutputs,
                                                    const std::vector<Variable>& fsInputs) {
    struct Slot {
        int location = 0;
        uint32_t count = 1;
    };
    std::map<std::string, Slot> slots;
    for (const auto& v : vsOutputs) {
        if (!IsUserVarying(v.name)) continue;
        Slot& s = slots[v.name];
        s.count = std::max(s.count, LocationSlots(v.type));
    }
    for (const auto& v : fsInputs) {
        if (!IsUserVarying(v.name)) continue;
        Slot& s = slots[v.name];
        s.count = std::max(s.count, LocationSlots(v.type));
    }
    int loc = 0;
    std::unordered_map<std::string, int> out;
    for (auto& kv : slots) {
        kv.second.location = loc;
        out.emplace(kv.first, loc);
        loc += (int)kv.second.count;
    }
    return out;
}

std::string DedupeBlocks(const std::string& src) {
    static const std::regex kBlock(
        R"((?:layout\s*\([^)]*\)\s*)?(uniform|buffer)\s+([A-Za-z_]\w*)\s*\{)");
    std::map<std::string, std::string> seen;
    std::string out;
    size_t pos = 0;
    while (pos < src.size()) {
        std::smatch m;
        std::string rest = src.substr(pos);
        if (!std::regex_search(rest, m, kBlock)) {
            out += rest;
            break;
        }
        size_t start = pos + (size_t)m.position(0);
        out += src.substr(pos, start - pos);
        size_t i = start + (size_t)m.length(0);
        int depth = 1;
        while (i < src.size() && depth > 0) {
            if (src[i] == '{') ++depth;
            else if (src[i] == '}') --depth;
            ++i;
        }
        while (i < src.size() && (src[i] == ';' || src[i] == ' ' || src[i] == '\t' ||
                                  src[i] == '\r'))
            ++i;
        if (i < src.size() && src[i] == '\n') ++i;
        std::string block = src.substr(start, i - start);
        std::string name = m[2];
        auto it = seen.find(name);
        if (it == seen.end()) {
            seen.emplace(name, block);
            out += block;
        } else if (it->second != block) {
            out += block;
        }
        pos = i;
    }
    return out;
}

std::string DedupeFunctions(const std::string& src) {
    static const std::regex kFunction(
        R"(([A-Za-z_]\w*)\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{)");
    std::map<std::string, std::string> seen;
    std::string out;
    size_t pos = 0;
    while (pos < src.size()) {
        std::smatch m;
        std::string rest = src.substr(pos);
        if (!std::regex_search(rest, m, kFunction)) {
            out += rest;
            break;
        }
        size_t start = pos + (size_t)m.position(0);
        out += src.substr(pos, start - pos);
        size_t i = start + (size_t)m.length(0);
        int depth = 1;
        while (i < src.size() && depth > 0) {
            if (src[i] == '{') ++depth;
            else if (src[i] == '}') --depth;
            ++i;
        }
        if (i < src.size() && src[i] == '\n') ++i;
        std::string body = src.substr(start, i - start);
        std::string name = m[2];
        auto it = seen.find(name);
        if (it == seen.end()) {
            seen.emplace(name, body);
            out += body;
        } else if (it->second != body) {
            out += body;
        }
        pos = i;
    }
    return out;
}

std::string NormalizeSource(const std::string& source) {
    std::istringstream iss(source);
    std::vector<std::string> leadingDefines;
    std::vector<std::string> body;
    std::string line;
    bool seenVersion = false;
    while (std::getline(iss, line)) {
        size_t s = line.find_first_not_of(" \t\r");
        std::string t = s == std::string::npos ? std::string() : line.substr(s);
        if (t.rfind("#version", 0) == 0) {
            if (!seenVersion) {
                body.push_back(line);
                seenVersion = true;
            }
            continue;
        }
        if (!seenVersion && (t.rfind("#define", 0) == 0 || t.rfind("#extension", 0) == 0)) {
            leadingDefines.push_back(line);
            continue;
        }
        body.push_back(line);
    }
    std::string out;
    bool injected = seenVersion;
    for (const auto& l : body) {
        out += l + "\n";
        if (injected) {
            for (const auto& d : leadingDefines) out += d + "\n";
            injected = false;
        }
    }
    if (!seenVersion)
        for (const auto& d : leadingDefines) out = d + "\n" + out;
    return DedupeFunctions(DedupeBlocks(out));
}

template <typename Range>
std::map<std::string, int> RankResources(spirv_cross::CompilerMSL& msl, const Range& resources) {
    std::map<std::string, int> out;
    for (const auto& r : resources) {
        std::string name = msl.get_name(r.id);
        if (name.empty()) name = msl.get_fallback_name(r.id);
        if (!IsUserVarying(name)) continue;
        int slots = (int)LocationSlots(TypeFromSpv(msl.get_type(r.type_id)));
        auto it = out.find(name);
        if (it == out.end()) out.emplace(name, slots);
        else it->second = std::max(it->second, slots);
    }
    int loc = 0;
    for (auto& kv : out) {
        int slots = kv.second;
        kv.second = loc;
        loc += slots;
    }
    return out;
}

bool EmitMsl(const std::vector<uint32_t>& spirv, ShaderStage stage,
             const std::map<std::string, int>* varyingLocations, ShaderCompileResult& r) {
    try {
        spirv_cross::CompilerMSL msl(spirv);
        auto mslOptions = msl.get_msl_options();
        mslOptions.set_msl_version(2, 0);
        msl.set_msl_options(mslOptions);

        auto resources = msl.get_shader_resources();
        if (varyingLocations) {
            const auto& vars = stage == ShaderStage::Vertex ? resources.stage_outputs
                                                            : resources.stage_inputs;
            for (const auto& v : vars) {
                std::string name = msl.get_name(v.id);
                if (name.empty()) name = msl.get_fallback_name(v.id);
                auto it = varyingLocations->find(name);
                if (it != varyingLocations->end())
                    msl.set_decoration(v.id, spv::DecorationLocation, (uint32_t)it->second);
            }
        }

        r.moduleId = msl.compile();
        r.inputs.clear();
        r.outputs.clear();
        r.uniforms.clear();
        r.samplers.clear();
        r.images.clear();
        r.atomicCounters.clear();
        resources = msl.get_shader_resources();
        for (const auto& v : resources.stage_inputs) FillResource(msl, v, r.inputs, false, false, false);
        for (const auto& v : resources.stage_outputs) FillResource(msl, v, r.outputs, false, false, false);
        for (const auto& v : resources.uniform_buffers) FillResource(msl, v, r.uniforms, false, false, true);
        for (const auto& v : resources.push_constant_buffers) FillResource(msl, v, r.uniforms, false, false, true);
        for (const auto& v : resources.separate_images) FillResource(msl, v, r.images, false, true, false);
        for (const auto& v : resources.sampled_images) FillResource(msl, v, r.samplers, true, false, false);
        for (const auto& v : resources.separate_samplers) FillResource(msl, v, r.samplers, true, false, false);
        for (const auto& v : resources.atomic_counters) FillResource(msl, v, r.atomicCounters, false, false, true);
        return true;
    } catch (const std::exception& e) {
        Diagnostic d;
        d.severity = DiagnosticSeverity::Error;
        d.code = "GLS-003";
        d.message = std::string("spirv-cross MSL: ") + e.what();
        r.diagnostics.push_back(d);
        return false;
    }
}

class GlslangCompiler final : public Compiler {
public:
    ShaderCompileResult compileShader(ShaderStage stage, const std::string& source,
                                      const GLSLVersion& version, GlslProfile profile,
                                      const ExtensionSet& extensions,
                                      const CompileOptions& options,
                                      const SourceOrigin& origin) override {
        (void)version;
        (void)profile;
        (void)extensions;
        (void)options;
        ShaderCompileResult r;
        r.stage = stage;
        r.isCompute = stage == ShaderStage::Compute;
        EnsureGlslang();

        EShLanguage lang = ToGlslangStage(stage);
        glslang_api::TShader shader(lang);
        std::string text = NormalizeSource(source);
        if (text.empty() || text.back() != '\n') text += '\n';
        const char* strings[1] = {text.c_str()};
        int lengths[1] = {(int)text.size()};
        std::string name = origin.name.empty() ? "shader" : origin.name;
        const char* names[1] = {name.c_str()};
        shader.setStringsWithLengthsAndNames(strings, lengths, names, 1);
        shader.setSourceEntryPoint("main");
        shader.setAutoMapBindings(true);
        shader.setAutoMapLocations(true);
        shader.setEntryPoint("main");
        shader.setEnvInput(glslang_api::EShSourceGlsl, lang, glslang_api::EShClientOpenGL, 460);
        shader.setEnvClient(glslang_api::EShClientOpenGL, glslang_api::EShTargetOpenGL_450);
        shader.setEnvTarget(glslang_api::EShTargetSpv, glslang_api::EShTargetSpv_1_0);
        shader.setPreamble("");

        if (!shader.parse(GetDefaultResources(), 460, false, EShMsgDefault)) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.code = "GLS-001";
            d.message = std::string("glslang parse: ") + FirstLog(shader);
            r.diagnostics.push_back(d);
            return r;
        }

        glslang_api::TProgram program;
        program.addShader(&shader);
        if (!program.link(EShMsgDefault)) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.code = "GLS-002";
            d.message = std::string("glslang link: ") + program.getInfoLog();
            r.diagnostics.push_back(d);
            return r;
        }

        if (!program.mapIO()) {
            Diagnostic d;
            d.severity = DiagnosticSeverity::Error;
            d.code = "GLS-002";
            d.message = std::string("glslang mapIO: ") + program.getInfoLog();
            r.diagnostics.push_back(d);
            return r;
        }

        std::vector<uint32_t> spirv;
        glslang_api::SpvOptions spvOptions;
        spvOptions.disableOptimizer = true;
        spvOptions.optimizeSize = false;
        spvOptions.generateDebugInfo = false;
        glslang_api::GlslangToSpv(*program.getIntermediate(lang), spirv, &spvOptions);
        r.spirv = spirv;

        std::map<std::string, int> varyings;
        if (stage == ShaderStage::Vertex || stage == ShaderStage::Fragment) {
            spirv_cross::CompilerMSL reflect(spirv);
            auto resources = reflect.get_shader_resources();
            const auto& io = stage == ShaderStage::Vertex ? resources.stage_outputs
                                                          : resources.stage_inputs;
            varyings = RankResources(reflect, io);
        }
        if (!EmitMsl(spirv, stage, varyings.empty() ? nullptr : &varyings, r)) return r;

        r.success = true;
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
            if (s.stage == ShaderStage::Vertex) vs = &s;
            else if (s.stage == ShaderStage::Fragment) fs = &s;
            for (const auto& u : s.uniforms) r.activeUniforms.push_back(u);
            if (s.stage == ShaderStage::Vertex)
                for (const auto& a : s.inputs) r.activeAttribs.push_back(a);
        }
        if (vs && fs) {
            for (const auto& fi : fs->inputs) {
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
                if (!(match->type == fi.type)) {
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

        std::unordered_map<std::string, int> unionLocations;
        if (vs && fs) {
            unionLocations = UnionLocations(vs->outputs, fs->inputs);
            std::map<std::string, int> ordered(unionLocations.begin(), unionLocations.end());
            for (const auto& s : request.stages) {
                ShaderCompileResult out = s;
                if (s.success && !s.spirv.empty() &&
                    (s.stage == ShaderStage::Vertex || s.stage == ShaderStage::Fragment)) {
                    if (!EmitMsl(s.spirv, s.stage, &ordered, out)) {
                        r.success = false;
                        r.diagnostics.insert(r.diagnostics.end(), out.diagnostics.begin(),
                                             out.diagnostics.end());
                    }
                }
                r.stages.push_back(std::move(out));
            }
        } else {
            r.stages = request.stages;
        }
        return r;
    }
};

}

std::unique_ptr<Compiler> CreateGlslangCompiler() {
    return std::make_unique<GlslangCompiler>();
}

}
