

#include "tglmt/gl46.h"
#include "tglmt/GeometryEmulation.h"
#include "tglmt/Context.h"
#include "tglmt/GLSLConverter.h"
#include "tglmt/compiler/ShaderIR.h"
#include "tglmt/ShadowCompiler.h"
#include "tglmt/compiler/backend/glslang/GlslangBackend.h"
#include <cstdio>
#include <cstdlib>
using namespace tglmt;

namespace tglmt::gl {

namespace {

const char* StageName(GLenum stage) {
    switch (stage) {
        case 0x8B31: return "vertex";
        case 0x8B30: return "fragment";
        case 0x91B9: return "compute";
        case 0x8E88: return "tess-control";
        case 0x8E87: return "tess-eval";
        default: return "unknown";
    }
}

bool StageFromGL(unsigned stage, tglmt::compiler::ShaderStage& st) {
    using namespace tglmt::compiler;
    switch (stage) {
        case 0x8B31: st = ShaderStage::Vertex; return true;
        case 0x8B30: st = ShaderStage::Fragment; return true;
        case 0x91B9: st = ShaderStage::Compute; return true;
        case 0x8E88: st = ShaderStage::TessControl; return true;
        case 0x8E87: st = ShaderStage::TessEval; return true;
        default: return false;
    }
}

std::unique_ptr<tglmt::compiler::Compiler> MakeShadowCompiler() {
    static const bool useGlslang = [] {
        const char* v = std::getenv("TGLMT_GLSLANG_SHADOW");
        return v && v[0] && v[0] != '0';
    }();
    if (useGlslang) return tglmt::compiler::backend::glslang::CreateGlslangCompiler();
    return tglmt::compiler::CreateCompiler();
}

} 

void ShadowValidateCompile(const std::string& source, unsigned stage, unsigned shaderId) {
    using namespace tglmt::compiler;
    ShaderStage st;
    if (!StageFromGL(stage, st)) return;
    auto compiler = MakeShadowCompiler();
    ShaderCompileResult res = compiler->compileShader(
        st, source, GLSLVersion{}, GlslProfile::Core, ExtensionSet{}, CompileOptions{},
        SourceOrigin{"glCompileShader", (int)shaderId});
    if (res.success) return;
    std::fprintf(stderr, "[TGLMT shadow] new compiler FAILED %s shader %u (%zu diagnostics)\n",
                 StageName(stage), shaderId, res.diagnostics.size());
    for (const auto& d : res.diagnostics)
        std::fprintf(stderr, "[TGLMT shadow]   %s: %s\n", d.code.c_str(), d.message.c_str());
}

void ShadowValidateLink(const std::vector<std::pair<unsigned, std::string>>& stages) {
    using namespace tglmt::compiler;
    ProgramLinkRequest req;
    for (const auto& [type, source] : stages) {
        ShaderStage st;
        if (!StageFromGL(type, st)) continue;
        auto compiler = MakeShadowCompiler();
        req.stages.push_back(compiler->compileShader(st, source, GLSLVersion{}, GlslProfile::Core,
                                                     ExtensionSet{}, CompileOptions{},
                                                     SourceOrigin{"glLinkProgram", 0}));
    }
    if (req.stages.empty()) return;
    auto compiler = MakeShadowCompiler();
    ProgramLinkResult res = compiler->linkProgram(req);
    if (res.success) return;
    std::fprintf(stderr, "[TGLMT shadow link] FAILED (%zu diagnostics)\n", res.diagnostics.size());
    for (const auto& d : res.diagnostics)
        std::fprintf(stderr, "[TGLMT shadow link]   %s: %s\n", d.code.c_str(), d.message.c_str());
}

bool ShadowCompilerEnabled() {
    static const bool enabled = [] {
        const char* v = std::getenv("TGLMT_NEW_COMPILER_SHADOW");
        return v && v[0] && v[0] != '0';
    }();
    return enabled;
}

GLuint glCreateShader(GLenum type) {
    Context& c = Context::Current();
    GLuint id; c.registry.Create(ObjectKind::Shader, 1, &id);
    ShaderObject s; s.id = id; s.type = type;
    c.shaders[id] = s;
    return id;
}
void glDeleteShader(GLuint s) {
    Context& c = Context::Current();
    
    
    
    
    
    
    
    
    bool attached = false;
    for (const auto& kv : c.programs) {
        for (GLuint sh : kv.second.shaders)
            if (sh == s) { attached = true; break; }
        if (attached) break;
    }
    c.registry.Delete(ObjectKind::Shader, 1, &s);
    if (!attached) c.shaders.erase(s);
}
GLboolean glIsShader(GLuint s) {
    return Context::Current().registry.Is(ObjectKind::Shader, s) ? 1 : 0;
}
void glShaderSource(GLuint s, GLsizei n, const GLchar* const* str, const GLint* len) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    
    
    
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (n > 0 && !str) { c.errors.Record(0x0501); return; }
    std::string src;
    for (GLsizei i = 0; i < n; ++i) {
        if (!str[i]) { c.errors.Record(0x0501); return; }
        if (len && len[i] >= 0) src.append(str[i], (size_t)len[i]);
        else src.append(str[i]);
    }
    it->second.source = src;
}
void glCompileShader(GLuint s) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    if (it->second.source.empty()) { it->second.compiled = false; it->second.infoLog = "error: empty source"; return; }
    uint32_t stage = it->second.type;
    
    
    if (stage == 0x8DD9) {
        
        
        
        
        GsTranslation gt = TranslateGeometryShader(it->second.source, "", {}, false);
        it->second.conv = GLSLConvertResult{};
        it->second.msl.clear();
        it->second.compiled = gt.ok;
        it->second.infoLog = gt.ok ? "" : ("error: " + gt.log);
        return;
    }
    if (stage != 0x8B31 && stage != 0x8B30 && stage != 0x91B9 &&
        stage != 0x8E88 && stage != 0x8E87) {
        
        
        it->second.conv = GLSLConvertResult{};
        it->second.msl.clear();
        it->second.compiled = false;
        const char* nm = (stage == 0x8DD9) ? "geometry" : "unknown-stage";
        it->second.infoLog = std::string("error: ") + nm +
            " shader ngoài subset (GS cần đường mesh riêng)";
        return;
    }
    GLSLConvertResult r = ConvertGLSLtoMSL(it->second.source, stage);
    it->second.conv = r;
    it->second.msl = r.ok ? r.msl : "";
    it->second.compiled = r.ok;
    it->second.infoLog = r.ok ? "" : ("error: " + r.log);
    if (r.ok && ShadowCompilerEnabled()) ShadowValidateCompile(it->second.source, stage, s);
}
void glGetShaderSource(GLuint s, GLsizei n, GLsizei* l, GLchar* src) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    size_t k = std::min((size_t)n - 1, it->second.source.size());
    memcpy(src, it->second.source.c_str(), k); src[k] = 0;
    if (l) *l = (GLsizei)k;
}
void glSpecializeShader(GLuint s, const GLchar* pEntry, GLuint n, const GLuint* idx, const GLuint* val) {
    
    (void)pEntry; (void)n; (void)idx; (void)val;
    glCompileShader(s);
}

} 
