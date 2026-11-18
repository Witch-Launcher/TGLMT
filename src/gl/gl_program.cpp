

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/ShadowCompiler.h"
#include "tglmt/glsl_type.h"
#include "tglmt/GeometryEmulation.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>
using namespace tglmt;

static void BuildUniformSpecMap(ProgramObject& pr) {
    

    auto isSamplerName = [&](const std::string& nm) {
        auto in = [&](const std::vector<std::string>& v) {
            for (const auto& s : v)
                if (s == nm) return true;
            return false;
        };
        auto inStage = [&](const std::vector<ProgramObject::StageSampler>& v) {
            for (const auto& s : v)
                if (s.name == nm) return true;
            return false;
        };
        if (in(pr.vsSamplers) || in(pr.fsSamplers) || in(pr.computeSamplers)) return true;
        if (inStage(pr.vsCaptureSamplers) || inStage(pr.tcsSamplers) || inStage(pr.tesSamplers) ||
            inStage(pr.fsTessSamplers))
            return true;
        return false;
    };
    auto isImageName = [&](const std::string& nm) {
        auto in = [&](const std::vector<ProgramObject::ImageSlot>& v) {
            for (const auto& im : v)
                if (im.name == nm) return true;
            return false;
        };
        return in(pr.vsImages) || in(pr.fsImages) || in(pr.computeImages) ||
               in(pr.tcsImages) || in(pr.tesImages);
    };

    for (const auto& kv : pr.uniformLoc) {
        const std::string& nm = kv.first;
        const GLint loc = kv.second;
        ProgramObject::UniformSlot::Spec sp;
        
        for (const auto& au : pr.activeUniforms) {
            if (au.name == nm || au.name == nm + "[0]") {
                sp.type = au.type;
                sp.arraySize = au.size > 0 ? au.size : 1;
                break;
            }
        }
        
        if (!sp.type) {
            for (const auto& e : pr.uniformLayout) {
                if (e.name != nm) continue;
                sp.type = e.glType;
                sp.arraySize = e.arraySize > 0 ? e.arraySize : 1;
                break;
            }
        }
        
        sp.isSampler = isSamplerName(nm) || UniformTypeIsSampler(sp.type);
        sp.isImage = isImageName(nm) || UniformTypeIsImage(sp.type);
        pr.uniforms[loc].spec = sp;
    }
}

namespace tglmt::gl {
static bool IsIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
}

static GLenum GlslTypeToGLEnum(const std::string& t) {
    if (t == "float") return 0x1406;
    if (t == "vec2") return 0x8B50;
    if (t == "vec3") return 0x8B51;
    if (t == "vec4") return 0x8B52;
    if (t == "int") return 0x1404;
    if (t == "ivec2") return 0x8B53;
    if (t == "ivec3") return 0x8B54;
    if (t == "ivec4") return 0x8B55;
    if (t == "uint") return 0x1405;
    if (t == "uvec2") return 0x8DC6;
    if (t == "uvec3") return 0x8DC7;
    if (t == "uvec4") return 0x8DC8;
    if (t == "bool") return 0x8B56;
    if (t == "bvec2") return 0x8B57;
    if (t == "bvec3") return 0x8B58;
    if (t == "bvec4") return 0x8B59;
    if (t == "mat2") return 0x8B5A;
    if (t == "mat3") return 0x8B5B;
    if (t == "mat4") return 0x8B5C;
    
    
    if (t == "sampler1D") return 0x8B5D;
    if (t == "sampler2D") return 0x8B5E;
    if (t == "sampler3D") return 0x8B5F;
    if (t == "samplerCube") return 0x8B60;
    if (t == "sampler1DShadow") return 0x8B61;
    if (t == "sampler2DShadow") return 0x8B62;
    if (t == "sampler2DRect") return 0x8B63;
    if (t == "sampler2DRectShadow") return 0x8B64;
    if (t == "sampler1DArray") return 0x8DC0;
    if (t == "sampler2DArray") return 0x8DC1;
    if (t == "samplerBuffer") return 0x8DC2;
    if (t == "sampler1DArrayShadow") return 0x8DC3;
    if (t == "sampler2DArrayShadow") return 0x8DC4;
    if (t == "samplerCubeShadow") return 0x8DC5;
    if (t == "sampler2DMS") return 0x9108;
    if (t == "sampler2DMSArray") return 0x9109;
    if (t == "isampler1D") return 0x8DC9;
    if (t == "isampler2D") return 0x8DCA;
    if (t == "isampler3D") return 0x8DCB;
    if (t == "isamplerCube") return 0x8DCC;
    if (t == "isampler2DRect") return 0x8DCD;
    if (t == "isampler1DArray") return 0x8DCE;
    if (t == "isampler2DArray") return 0x8DCF;
    if (t == "isamplerBuffer") return 0x8DD0;
    if (t == "usampler1D") return 0x8DD1;
    if (t == "usampler2D") return 0x8DD2;
    if (t == "usampler3D") return 0x8DD3;
    if (t == "usamplerCube") return 0x8DD4;
    if (t == "usampler2DRect") return 0x8DD5;
    if (t == "usampler1DArray") return 0x8DD6;
    if (t == "usampler2DArray") return 0x8DD7;
    if (t == "usamplerBuffer") return 0x8DD8;
    return 0; 
}
GLuint glCreateProgram() {
    Context& c = Context::Current();
    GLuint id; c.registry.Create(ObjectKind::Program, 1, &id);
    c.programs[id] = ProgramObject{id};
    return id;
}
GLuint glCreateShaderProgramv(GLenum type, GLsizei n, const GLchar* const* s) {
    GLuint p = glCreateProgram();
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, n, s, nullptr);
    glCompileShader(sh);
    glAttachShader(p, sh);
    glLinkProgram(p);
    return p;
}
void glDeleteProgram(GLuint p) {
    Context& c = Context::Current();
    GLuint a[1]={p}; c.registry.Delete(ObjectKind::Program, 1, a);
    
    
    
    if (c.state.BoundProgram() == p) c.state.BindProgram(0);
    for (auto& kv : c.pipelines)
        if (kv.second.activeProgram == p) kv.second.activeProgram = 0;
    
    DropGeometryProgramInfo(p);
    c.programs.erase(p);
}
GLboolean glIsProgram(GLuint p) {
    return Context::Current().registry.Is(ObjectKind::Program, p) ? 1 : 0;
}
void glAttachShader(GLuint p, GLuint s) {
    Context& c = Context::Current();
    auto itp = c.programs.find(p);
    auto its = c.shaders.find(s);
    if (itp == c.programs.end() || its == c.shaders.end()) { c.errors.Record(0x0502); return; }
    itp->second.shaders.push_back(s);
}
void glDetachShader(GLuint p, GLuint s) {
    Context& c = Context::Current();
    auto itp = c.programs.find(p);
    if (itp == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto& v = itp->second.shaders;
    v.erase(std::remove(v.begin(), v.end(), s), v.end());
}
static std::string RewriteColorIndex(std::string msl, int from, int to) {
    if (from == to) return msl;
    std::string a = "[[color(" + std::to_string(from) + ")]]";
    std::string b = "[[color(" + std::to_string(to) + ")]]";
    size_t p = 0;
    while ((p = msl.find(a, p)) != std::string::npos) { msl.replace(p, a.size(), b); p += b.size(); }
    
    std::string fa = "mrt" + std::to_string(from), fb = "mrt" + std::to_string(to);
    p = 0;
    while ((p = msl.find(fa, p)) != std::string::npos) {
        bool left = p > 0 && IsIdentChar(msl[p - 1], false);
        bool right = p + fa.size() < msl.size() && IsIdentChar(msl[p + fa.size()], false);
        if (!left && !right) { msl.replace(p, fa.size(), fb); p += fb.size(); }
        else p += fa.size();
    }
    return msl;
}

static std::string InjectSampleMask(std::string msl) {
    if (msl.find("tglmt_sampleMask") != std::string::npos) return msl;
    
    
    if (msl.find("buffer(30)") != std::string::npos) return msl;
    size_t fn = msl.find(" TGLMT_fs(");
    if (fn == std::string::npos) return msl;
    size_t open = msl.find('(', fn);
    if (open == std::string::npos) return msl;
    int depth = 1;
    size_t close = open + 1;
    for (; close < msl.size() && depth > 0; ++close) {
        if (msl[close] == '(') ++depth;
        else if (msl[close] == ')') --depth;
    }
    if (depth != 0) return msl;
    --close; 

    msl.insert(close,
               ",\n    constant uint& tglmt_sampleMask [[buffer(30)]],"
               "\n    uint tglmt_sampleID [[sample_id]]");
    size_t body = msl.find('{', close);
    if (body == std::string::npos) return msl;
    msl.insert(body + 1,
               "\n    if (!((tglmt_sampleMask >> (tglmt_sampleID & 31u)) & 1u))"
               " { discard_fragment(); }\n");
    return msl;
}
static std::string WrapSingleFragColor(std::string msl, const std::string& type, int loc) {
    if (loc <= 0 || msl.find("struct TGLMT_FOut") != std::string::npos) return msl;
    std::string sig = "fragment " + type + " TGLMT_fs(";
    size_t p = msl.find(sig);
    if (p == std::string::npos) return msl;
    std::string rep = "struct TGLMT_FOut { " + type + " c [[color(" +
                      std::to_string(loc) + ")]]; };\nfragment TGLMT_FOut TGLMT_fs(";
    msl.replace(p, sig.size(), rep);
    size_t r = msl.find("return tglmt_fragColor;", p);
    if (r != std::string::npos)
        msl.replace(r, strlen("return tglmt_fragColor;"),
                    "TGLMT_FOut _out = {}; _out.c = tglmt_fragColor; return _out;");
    return msl;
}
static std::string RewriteLocns(std::string msl, bool isAttrib, int from, int to) {
    
    std::string a, b;
    if (isAttrib) {
        a = "[[attribute(" + std::to_string(from) + ")]]";
        b = "[[attribute(" + std::to_string(to) + ")]]";
    } else {
        a = "[[user(locn" + std::to_string(from) + ")]]";
        b = "[[user(locn" + std::to_string(to) + ")]]";
    }
    size_t p = 0;
    while ((p = msl.find(a, p)) != std::string::npos) {
        msl.replace(p, a.size(), b);
        p += b.size();
    }
    return msl;
}

static bool IsSubIdent(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}
static std::string SubStripComments(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size();) {
        if (i + 1 < in.size() && in[i] == '/' && in[i + 1] == '/') {
            while (i < in.size() && in[i] != '\n') { out.push_back(' '); ++i; }
        } else if (i + 1 < in.size() && in[i] == '/' && in[i + 1] == '*') {
            i += 2;
            while (i + 1 < in.size() && !(in[i] == '*' && in[i + 1] == '/')) {
                out.push_back(in[i] == '\n' ? '\n' : ' '); ++i;
            }
            if (i + 1 < in.size()) { out += "  "; i += 2; }
        } else {
            out.push_back(in[i++]);
        }
    }
    return out;
}
static void SubSkipSpace(const std::string& s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
}
static std::string SubIdent(const std::string& s, size_t& i) {
    size_t b = i;
    while (i < s.size() && IsSubIdent(s[i])) ++i;
    return s.substr(b, i - b);
}
static bool SubWord(const std::string& s, size_t i, const char* w) {
    size_t n = strlen(w);
    if (i + n > s.size() || s.compare(i, n, w) != 0) return false;
    if (i > 0 && IsSubIdent(s[i - 1])) return false;
    if (i + n < s.size() && IsSubIdent(s[i + n])) return false;
    return true;
}
static void RefreshSubroutines(Context& c, ProgramObject& pr) {
    pr.subroutines.clear();
    for (GLuint sid : pr.shaders) {
        auto sit = c.shaders.find(sid);
        if (sit == c.shaders.end()) continue;
        GLenum st = sit->second.type;
        if (st != 0x8B31 && st != 0x8B30 && st != 0x8DD9 &&
            st != 0x8E88 && st != 0x8E87) continue;
        std::string src = SubStripComments(sit->second.source);
        auto& info = pr.subroutines[st];
        size_t i = 0;
        while (i < src.size()) {
            if (!SubWord(src, i, "subroutine")) { ++i; continue; }
            size_t j = i + 10;
            SubSkipSpace(src, j);
            if (SubWord(src, j, "uniform")) {
                j += 7; SubSkipSpace(src, j);
                std::string ty = SubIdent(src, j);
                SubSkipSpace(src, j);
                std::string nm = SubIdent(src, j);
                if (!ty.empty() && !nm.empty()) {
                    bool dup = false;
                    for (auto& u : info.uniforms) if (u.first == nm) { dup = true; break; }
                    if (!dup) info.uniforms.emplace_back(nm, ty);
                }
                size_t semi = src.find(';', j);
                i = (semi == std::string::npos) ? src.size() : semi + 1;
                continue;
            }
            if (j < src.size() && src[j] == '(') {
                size_t rp = src.find(')', j + 1);
                if (rp == std::string::npos) { i = src.size(); continue; }
                std::string ty = src.substr(j + 1, rp - j - 1);
                
                size_t b = ty.find_first_not_of(" \t\r\n");
                size_t e = ty.find_last_not_of(" \t\r\n");
                ty = (b == std::string::npos) ? "" : ty.substr(b, e - b + 1);
                size_t lp = src.find('(', rp + 1);
                size_t semi = src.find(';', rp + 1);
                if (lp == std::string::npos || (semi != std::string::npos && semi < lp)) {
                    i = rp + 1; continue;
                }
                std::string before = src.substr(rp + 1, lp - rp - 1);
                
                size_t q = before.size();
                while (q > 0 && !IsSubIdent(before[q - 1])) --q;
                size_t qe = q;
                while (q > 0 && IsSubIdent(before[q - 1])) --q;
                std::string fn = before.substr(q, qe - q);
                if (!ty.empty() && !fn.empty() && info.implIndex.find(fn) == info.implIndex.end()) {
                    GLuint idx = (GLuint)info.impls.size();
                    info.impls.push_back(fn);
                    info.implIndex[fn] = idx;
                    info.typeImpls[ty].push_back(fn);
                }
                i = lp + 1;
                continue;
            }
            ++i;
        }
    }
    
    
    for (auto& [st, info] : pr.subroutines) {
        (void)st;
        GLint next = 0;
        for (auto& kv : pr.uniformLoc) next = std::max(next, kv.second + 1);
        for (auto& u : info.uniforms) {
            auto it = pr.uniformLoc.find(u.first);
            if (it == pr.uniformLoc.end()) {
                pr.uniformLoc[u.first] = next++;
            }
            for (auto& e : pr.uniformLayout)
                if (e.name == u.first) { e.loc = pr.uniformLoc[u.first]; break; }
        }
    }
}

static bool ExtractUniformStruct(const std::string& msl, std::string* out) {
    size_t p = msl.find("struct TGLMTUniforms");
    if (p == std::string::npos) return false;
    size_t b = msl.find('{', p);
    if (b == std::string::npos) return false;
    int depth = 0;
    size_t e = b;
    for (; e < msl.size(); ++e) {
        if (msl[e] == '{') ++depth;
        else if (msl[e] == '}') { --depth; if (depth == 0) { ++e; break; } }
    }
    if (depth != 0 || e > msl.size()) return false;
    *out = msl.substr(p, e - p) + "\n";
    return true;
}

static bool PrepareGeometryProgram(Context& c, ProgramObject& pr, ShaderObject* vs,
                                   ShaderObject* fs, ShaderObject* gs, GsProgramInfo* info,
                                   GLSLConvertResult* vsC, GLSLConvertResult* fsC) {
    if (!vs || !fs || !gs) {
        pr.infoLog = "error: program có geometry shader cần đủ VS + GS + FS";
        return false;
    }
    
    
    std::string uniformStruct;
    if (!ExtractUniformStruct(vs->msl, &uniformStruct)) {
        uniformStruct.clear();
    }
    
    std::vector<GsVar> vsOuts;
    for (const auto& o : vsC->outputs) {
        GsVar v;
        v.name = o.name;
        v.glslType = o.glslType;
        v.mslType = o.mslType;
        vsOuts.push_back(v);
    }
    GsTranslation t = TranslateGeometryShader(gs->source, uniformStruct, vsOuts);
    if (!t.ok) {
        pr.infoLog = "error: geometry shader emulation: " + t.log;
        return false;
    }
    
    
    for (const auto& u : t.uniforms) {
        bool found = false;
        for (const auto& uv : vs->conv.uniforms) {
            if (uv.name != u.name) continue;
            found = true;
            if (uv.mslType != u.mslType) {
                pr.infoLog = "error: GS uniform `" + u.name + "` khác kiểu với VS (" +
                             u.glslType + " vs " + uv.glslType + ")";
                return false;
            }
            break;
        }
        if (!found) {
            pr.infoLog = "error: GS uniform `" + u.name +
                         "` phải khai báo giống vertex shader (emulation GS dùng chung "
                         "buffer uniform của VS)";
            return false;
        }
        
        
        
        if (uniformStruct.find(u.name) == std::string::npos) {
            pr.infoLog = "error: GS uniform `" + u.name +
                         "` không có trong uniform buffer của vertex shader "
                         "(vertex shader phải dùng uniform này để emulation GS "
                         "dùng chung được)";
            return false;
        }
    }
    
    for (const auto& fi : fsC->inputs) {
        bool found = false;
        for (const auto& go : t.outputs) {
            if (go.name != fi.name) continue;
            found = true;
            if (go.mslType != fi.mslType) {
                pr.infoLog = "error: varying type mismatch GS→FS: " + fi.name;
                return false;
            }
            
            
            auto norm = [](const std::string& s) {
                return (s == "smooth" || s.empty()) ? std::string() : s;
            };
            if (norm(go.interp) != norm(fi.interpolation)) {
                pr.infoLog = "error: interpolation qualifier mismatch GS→FS: " + fi.name +
                             " (GS='" + go.interp + "', FS='" + fi.interpolation + "')";
                return false;
            }
            break;
        }
        if (!found) {
            pr.infoLog = "error: fragment input không do geometry shader ghi: " + fi.name;
            return false;
        }
        if (fi.arraySize > 0) {
            pr.infoLog = "error: fragment input dạng mảng chưa hỗ trợ với geometry "
                         "shader: " + fi.name;
            return false;
        }
    }
    
    std::vector<GLSLVar> gsOuts;
    for (size_t i = 0; i < t.outputs.size(); ++i) {
        GLSLVar v;
        v.name = t.outputs[i].name;
        v.glslType = t.outputs[i].glslType;
        v.mslType = t.outputs[i].mslType;
        v.location = kTempLocBase + (int)i;   
        gsOuts.push_back(v);
    }
    vsC->outputs = gsOuts;
    
    pr.vsUniforms.clear();
    pr.fsUniforms.clear();
    for (auto& u : vs->conv.uniforms) {
        if (u.isSampler) continue;
        pr.vsUniforms.push_back({u.name, u.uniformOffset, u.uniformSize, u.glslType,
                                  u.arraySize});
    }
    pr.vsUniformBytes = vs->conv.uniformBufferSize;
    for (auto& u : fs->conv.uniforms) {
        if (u.isSampler) continue;
        pr.fsUniforms.push_back({u.name, u.uniformOffset, u.uniformSize, u.glslType,
                                  u.arraySize});
    }
    pr.fsUniformBytes = fs->conv.uniformBufferSize;
    
    info->gs = t;
    
    
    gs->compiled = true;
    gs->infoLog.clear();
    return true;
}

static bool FinishGeometryProgram(Context& c, ProgramObject& pr, ShaderObject* vs,
                                  ShaderObject* fs, GsProgramInfo* info,
                                  const GLSLConvertResult& fsC) {
    
    info->fsFields.clear();
    for (const auto& fi : fsC.inputs) {
        if (fi.location < 0) continue;
        GsFsField f;
        f.name = fi.name;
        f.mslType = fi.mslType;
        f.interp = fi.interpolation;
        f.location = fi.location;
        info->fsFields.push_back(f);
    }
    std::string passMsl = BuildGeometryPassthroughMsl(info->gs, info->fsFields);
    if (c.device && !c.device->isNull()) {
        std::string err;
        
        GLSLConvertResult capConv = ConvertGLSLtoMSL(vs->source, 0x8B31, true);
        if (!capConv.ok) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: MSL VS capture convert: " + capConv.log;
            return false;
        }
        pr.vertexCaptureMSL = capConv.msl;
        pr.vsCaptureIndexSlot = capConv.captureIndexSlot;
        pr.vsCaptureDrawSlot = capConv.captureDrawSlot;
        pr.vsCaptureDescSlot = capConv.captureDescSlot;
        pr.vsCaptureSlots.clear();
        for (auto& s : capConv.captureSlots)
            pr.vsCaptureSlots.push_back({s.name, s.slot, s.mslType, s.isGlIn, s.arraySize});
        
        
        for (auto& a : vs->conv.inputs) {
            auto it = pr.attribLoc.find(a.name);
            if (it == pr.attribLoc.end()) continue;
            int to = it->second;
            if (to < 0 || to > 15) {
                pr.linked = false;
                pr.infoLog = "error: GS emulation: attribute `" + a.name +
                             "` có location " + std::to_string(to) +
                             " ngoài 0..15 (descriptor capture chỉ có 16 slot)";
                return false;
            }
            int from = a.location;
            if (from == to || from < 0) continue;
            std::string pat = "tglmt_desc[" + std::to_string(from) + "]";
            std::string rep = "tglmt_desc[" + std::to_string(to) + "]";
            size_t p = 0;
            while ((p = pr.vertexCaptureMSL.find(pat, p)) != std::string::npos) {
                pr.vertexCaptureMSL.replace(p, pat.size(), rep);
                p += rep.size();
            }
        }
        info->capLib = c.device->compileLibrary(pr.vertexCaptureMSL, err);
        if (!info->capLib) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: MSL VS capture compile: " + err +
                         "\n--- MSL head ---\n" + pr.vertexCaptureMSL.substr(0, 1200);
            return false;
        }
        info->capPipe = c.device->makeComputePipeline(info->capLib.get(), "TGLMT_vs_capture");
        if (!info->capPipe) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: VS capture compute pipeline create failed";
            return false;
        }
        
        info->gsLib = c.device->compileLibrary(info->gs.msl, err);
        if (!info->gsLib) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: MSL GS kernel compile: " + err +
                         "\n--- MSL head ---\n" + info->gs.msl.substr(0, 1200);
            return false;
        }
        info->gsPipe = c.device->makeComputePipeline(info->gsLib.get(), "TGLMT_gs");
        if (!info->gsPipe) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: GS compute pipeline create failed (TGLMT_gs)";
            return false;
        }
        
        info->passLib = c.device->compileLibrary(passMsl, err);
        if (!info->passLib) {
            pr.linked = false;
            pr.infoLog = "error: GS emulation: MSL passthrough vertex compile: " + err +
                         "\n--- MSL head ---\n" + passMsl.substr(0, 1200);
            return false;
        }
        
        info->fsLib = pr.appleFS;
    }
    info->program = pr.id;
    info->linkGen = pr.linkGen;
    StoreGeometryProgramInfo(*info);
    return true;
}

void glLinkProgram(GLuint p) {
    Context& c = Context::Current();
    auto itp = c.programs.find(p);
    if (itp == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto& pr = itp->second;
    pr.linked = false; pr.infoLog.clear();
    pr.reflectionComplete = false;
    pr.uniformLayout.clear();
    
    
    
    if (ShadowCompilerEnabled()) {
        std::vector<std::pair<unsigned, std::string>> shadowStages;
        for (GLuint sid : pr.shaders) {
            auto its = c.shaders.find(sid);
            if (its == c.shaders.end()) continue;
            shadowStages.push_back({(unsigned)its->second.type, its->second.source});
        }
        ShadowValidateLink(shadowStages);
    }
    pr.uniforms.clear();
    pr.attribLoc.clear(); pr.appleVS.reset(); pr.appleFS.reset();
    pr.activeAttribs.clear();
    pr.appleCS.reset(); pr.appleCSPipeline.reset(); pr.computeMSL.clear();
    pr.computeBlockSlots.clear(); pr.computeSamplers.clear(); pr.computeUBSize = 0;
    pr.vsShaderBlocks.clear(); pr.fsShaderBlocks.clear();
    pr.tcsShaderBlocks.clear(); pr.tesShaderBlocks.clear();
    pr.storageBlocks.clear();
    pr.vsImages.clear(); pr.fsImages.clear(); pr.computeImages.clear();
    pr.computeAtomics.clear(); pr.vsAtomics.clear(); pr.fsAtomics.clear();
    pr.vsUBSize = pr.fsUBSize = 0;
    pr.pipelineVS = nullptr; pr.pipelineFS = nullptr;
    pr.appleTCS.reset(); pr.appleTES.reset(); pr.appleTCSPipeline.reset();
    pr.appleVSCapture.reset(); pr.appleVSCapturePipeline.reset();
    pr.vsCaptureIndexSlot = pr.vsCaptureDrawSlot = pr.vsCaptureDescSlot = -1;
    pr.vsCaptureSlots.clear(); pr.vertexCaptureMSL.clear();
    pr.vsUniforms.clear(); pr.tcsUniforms.clear(); pr.tesUniforms.clear(); pr.fsUniforms.clear();
    pr.vsUniformBytes = pr.tcsUniformBytes = pr.tesUniformBytes = pr.fsUniformBytes = 0;
    pr.vsCaptureSamplers.clear(); pr.tcsSamplers.clear(); pr.tesSamplers.clear();
    pr.fsTessSamplers.clear(); pr.vsSamplers.clear(); pr.fsSamplers.clear();
    pr.tcsImages.clear(); pr.tesImages.clear();
    pr.tcsAtomics.clear(); pr.tesAtomics.clear();
    pr.tessControlPoints = 0; pr.tessPatchType = 0; pr.tessSpacing = 0;
    pr.tessVertexOrder = 0;
    pr.tessFactorSlot = -1; pr.tesFactorSlot = -1;
    pr.tessControlMSL.clear(); pr.tessEvalMSL.clear(); pr.tessSlots.clear();
    ShaderObject *vs = nullptr, *fs = nullptr, *tcs = nullptr, *tes = nullptr, *gsh = nullptr;
    bool hasV = false, hasF = false, hasTC = false, hasTE = false;
    pr.hasTessStages = false; pr.hasGeometryStage = false;
    for (GLuint s : pr.shaders) {
        auto its = c.shaders.find(s);
        if (its == c.shaders.end()) { pr.infoLog = "error: missing shader"; return; }
        if (its->second.type == 0x8DD9) {
            
            
            
            
            gsh = &its->second;
            pr.hasGeometryStage = true;
            continue;
        }
        if (!its->second.compiled) { pr.infoLog = "error: shader not compiled"; return; }
        if (its->second.type == 0x8B31) { hasV = true; vs = &its->second; pr.vertexMSL = vs->msl; }
        if (its->second.type == 0x8B30) { hasF = true; fs = &its->second; pr.fragmentMSL = fs->msl; }
        if (its->second.type == 0x8E88) { hasTC = true; tcs = &its->second; pr.tessControlMSL = tcs->msl; }
        if (its->second.type == 0x8E87) { hasTE = true; tes = &its->second; pr.tessEvalMSL = tes->msl; }
        if (its->second.type == 0x91B9) { 
            pr.computeMSL = its->second.msl;
            pr.computeLocalSize[0] = its->second.conv.localSize[0];
            pr.computeLocalSize[1] = its->second.conv.localSize[1];
            pr.computeLocalSize[2] = its->second.conv.localSize[2];
            pr.computeBlockSlots = its->second.conv.blockSlots;
            
            for (auto& b : its->second.conv.blocks)
                if (b.isStorage)
                    pr.storageBlocks.push_back({b.name, b.binding, b.bufferSize,
                                                (int)b.members.size(), 4u});
            
            auto kindOf = [](const GLSLVar& s) -> char {
                if (s.isBuffer) return s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B';
                if (s.isCube) return 'C';
                if (s.isArray) return 'A';
                if (s.isShadow) return 'S';
                if (s.is1D) return '1';
                if (s.is3D) return s.sampleType == "int" ? 'x' : s.sampleType == "uint" ? 'y' : '3';
                if (s.sampleType == "int") return 'i';
                if (s.sampleType == "uint") return 'u';
                return '2';
            };
            for (auto& s : its->second.conv.samplers) {
                pr.computeSamplers.push_back(s.name);
                pr.samplerKind[s.name] = kindOf(s);
                if (s.samplerSlot >= 0) pr.samplerSlots[s.name] = s.samplerSlot;
                if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
            }
            
            for (auto& u : its->second.conv.uniforms) {
                if (u.isSampler) continue;
                pr.uniformLayout.push_back({u.name, u.uniformOffset, u.uniformSize, -1, true,
                                            GlslTypeToGLEnum(u.glslType), u.glslType, u.arraySize});
            }
            pr.computeUBSize = its->second.conv.uniformBufferSize;
            
            for (auto& as : its->second.conv.atomicSlots)
                pr.computeAtomics.push_back({as.name, as.binding, as.offset, as.slot});
            
            for (size_t k = 0; k < its->second.conv.images.size(); ++k) {
                auto& im = its->second.conv.images[k];
                int slot = (int)pr.computeSamplers.size() + (int)k;
                int binding = im.imageBinding;
                if (binding < 0) {
                    auto uit = pr.samplerUnits.find(im.name);
                    binding = (uit == pr.samplerUnits.end()) ? 0 : (int)uit->second;
                }
                pr.computeImages.push_back({im.name, slot, binding});
                if (!pr.samplerUnits.count(im.name)) pr.samplerUnits[im.name] = (GLuint)binding;
            }
        }
        if (its->second.type == 0x8E88 || its->second.type == 0x8E87) 
            pr.hasTessStages = true;
    }
    if (pr.shaders.empty()) { pr.infoLog = "error: no shaders attached"; return; }
    
    
    
    if (pr.hasTessStages) {
        
        
        
        if (!hasV || !hasF || !hasTC || !hasTE) {
            pr.infoLog = "error: tess program cần đủ VS+TCS+TES+FS";
            return;
        }
        if (!tcs || !tes || (tes->conv.tessPatchType != 3 && tes->conv.tessPatchType != 4)) {
            pr.infoLog = "error: tessellation chỉ hỗ trợ domain triangles/quads (T3/T4)";
            return;
        }
        
        
        GLSLConvertResult tcsConv = tcs->conv;
        if (tcs->conv.tessPatchType != tes->conv.tessPatchType) {
            tcsConv = ConvertGLSLtoMSL(tcs->source, 0x8E88, false, tes->conv.tessPatchType);
            if (!tcsConv.ok) {
                pr.infoLog = "error: MSL TCS convert theo domain TES: " + tcsConv.log;
                return;
            }
            pr.tessControlMSL = tcsConv.msl;
        }
        const GLSLConvertResult& tcv = tcsConv;
        const int minCP = (tes->conv.tessPatchType == 4) ? 4 : 3;
        if (tcv.tessVertices < minCP || tcv.tessVertices > 32) {
            pr.infoLog = "error: tessellation domain " +
                         std::string(tes->conv.tessPatchType == 4 ? "quads" : "triangles") +
                         " yêu cầu 3..32 control point (layout(vertices=N), N>=" +
                         std::to_string(minCP) + "; tcs=" +
                         std::to_string(tcv.tessVertices) + ")";
            return;
        }
        
        
        bool needCapture = false;
        for (auto& a : vs->conv.inputs) {
            if (a.location < 0 || a.location >= 16) {
                pr.infoLog = "error: T4b yêu cầu VS attribute có layout(location=0..15) "
                             "tường minh (attribute `" + a.name + "`)";
                return;
            }
        }
        for (auto& s : tcv.tessSlots) {
            if (!s.isInput) continue;
            needCapture = true;
            if (s.isGlIn) {
                if (s.name != "gl_Position" && s.name != "gl_PointSize") {
                    pr.infoLog = "error: T4b gl_in." + s.name + " chưa hỗ trợ";
                    return;
                }
                continue;
            }
            bool found = false;
            for (auto& o : vs->conv.outputs)
                if (o.name == s.name) {
                    if (o.mslType != s.mslType) {
                        pr.infoLog = "error: tess varying type mismatch VS→TCS: " + s.name;
                        return;
                    }
                    found = true;
                    break;
                }
            if (!found) {
                pr.infoLog = "error: TCS in `" + s.name + "` không có VS out tương ứng";
                return;
            }
        }
        
        pr.vsAtomics.clear(); pr.fsAtomics.clear();
        for (auto& as : vs->conv.atomicSlots) pr.vsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        for (auto& as : tcv.atomicSlots) pr.tcsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        for (auto& as : tes->conv.atomicSlots) pr.tesAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        for (auto& as : fs->conv.atomicSlots) pr.fsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        {
            GLint nextLoc = 0;
            for (auto& kv : pr.uniformLoc) nextLoc = std::max(nextLoc, kv.second + 1);
            std::unordered_map<std::string, GLenum> seenType;
            auto addStage = [&](const ShaderObject& sh, std::vector<ProgramObject::StageUniform>& out,
                                size_t& bytes, bool isVS) -> bool {
                for (auto& u : sh.conv.uniforms) {
                    if (u.isSampler) continue;
                    GLenum ty = GlslTypeToGLEnum(u.glslType);
                    if (!ty) {
                        pr.infoLog = "error: tess uniform `" + u.name + "` type chưa hỗ trợ: " +
                                     u.glslType;
                        return false;
                    }
                    auto st = seenType.find(u.name);
                    if (st != seenType.end() && st->second != ty) {
                        pr.infoLog = "error: tess uniform `" + u.name +
                                     "` khác kiểu giữa các stage";
                        return false;
                    }
                    bool firstSeen = (st == seenType.end());
                    seenType[u.name] = ty;
                    auto lit = pr.uniformLoc.find(u.name);
                    GLint loc = (lit != pr.uniformLoc.end()) ? lit->second : nextLoc++;
                    pr.uniformLoc[u.name] = loc;
                    if (firstSeen) {
                        pr.uniformLayout.push_back({u.name, u.uniformOffset, u.uniformSize,
                                                    loc, isVS, ty, u.glslType, u.arraySize});
                        std::string nm = u.name;
                        GLint sz = (u.arraySize > 0) ? (GLint)u.arraySize : 1;
                        if (u.arraySize > 0) nm += "[0]";
                        pr.activeUniforms.push_back({nm, ty, sz});
                    }
                    out.push_back({u.name, u.uniformOffset, u.uniformSize, u.glslType,
                                   u.arraySize});
                }
                bytes = sh.conv.uniformBufferSize;
                return true;
            };
            if (!addStage(*vs, pr.vsUniforms, pr.vsUniformBytes, true) ||
                !addStage(*tcs, pr.tcsUniforms, pr.tcsUniformBytes, false) ||
                !addStage(*tes, pr.tesUniforms, pr.tesUniformBytes, false) ||
                !addStage(*fs, pr.fsUniforms, pr.fsUniformBytes, false))
                return;
        }
        {
            auto kindOf = [](const GLSLVar& s) -> char {
                if (s.isBuffer) return s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B';
                if (s.isCube) return 'C';
                if (s.isArray) return 'A';
                if (s.isShadow) return 'S';
                if (s.is1D) return '1';
                if (s.is3D) return s.sampleType == "int" ? 'x' : s.sampleType == "uint" ? 'y' : '3';
                if (s.sampleType == "int") return 'i';
                if (s.sampleType == "uint") return 'u';
                return '2';
            };
            std::set<std::string> activeSamplerNames;
            auto addStage = [&](const ShaderObject& sh, std::vector<ProgramObject::StageSampler>& out,
                                std::vector<std::string>* names) {
                for (size_t k = 0; k < sh.conv.samplers.size(); ++k) {
                    const auto& s = sh.conv.samplers[k];
                    char kind = kindOf(s);
                    out.push_back({s.name, (int)k, s.samplerSlot, kind});
                    if (names) names->push_back(s.name);
                    pr.samplerKind[s.name] = kind;
                    if (s.samplerSlot >= 0) pr.samplerSlots[s.name] = s.samplerSlot;
                    if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
                    if (activeSamplerNames.insert(s.name).second) {
                        GLenum ty = GlslTypeToGLEnum(s.glslType);
                        if (ty) pr.activeUniforms.push_back({s.name, ty, 1});
                    }
                }
            };
            addStage(*vs, pr.vsCaptureSamplers, &pr.vsSamplers);
            addStage(*tcs, pr.tcsSamplers, nullptr);
            addStage(*tes, pr.tesSamplers, nullptr);
            addStage(*fs, pr.fsTessSamplers, &pr.fsSamplers);
        }
        
        {
            auto addImages = [&](const ShaderObject& sh,
                                 std::vector<ProgramObject::ImageSlot>& out,
                                 size_t samplerCount) {
                for (size_t k = 0; k < sh.conv.images.size(); ++k) {
                    const auto& im = sh.conv.images[k];
                    int slot = (int)samplerCount + (int)k;
                    int binding = im.imageBinding;
                    if (binding < 0) {
                        auto uit = pr.samplerUnits.find(im.name);
                        binding = (uit == pr.samplerUnits.end()) ? 0 : (int)uit->second;
                    }
                    out.push_back({im.name, slot, binding});
                    if (!pr.samplerUnits.count(im.name)) pr.samplerUnits[im.name] = (GLuint)binding;
                }
            };
            addImages(*vs, pr.vsImages, pr.vsCaptureSamplers.size());
            addImages(*tcs, pr.tcsImages, pr.tcsSamplers.size());
            addImages(*tes, pr.tesImages, pr.tesSamplers.size());
            addImages(*fs, pr.fsImages, pr.fsTessSamplers.size());
        }
        
        
        {
            std::unordered_map<std::string, GLuint> oldBinds;
            for (auto& b : pr.uniformBlocks) oldBinds[b.name] = b.binding;
            pr.uniformBlocks.clear();
            pr.vsBlocks.clear(); pr.fsBlocks.clear();
            for (auto& bs : vs->conv.blockSlots) pr.vsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
            for (auto& bs : tcv.blockSlots) pr.tcsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
            for (auto& bs : tes->conv.blockSlots) pr.tesShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
            for (auto& bs : fs->conv.blockSlots) pr.fsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
            GLuint idx = 0;
            auto addBlock = [&](const GLSLBlock& b, bool isVS) {
                for (auto& ex : pr.uniformBlocks)
                    if (ex.name == b.name) return;
                ProgramObject::UniformBlock ub;
                ub.name = b.name; ub.index = idx++; ub.isVS = isVS;
                ub.layoutBinding = b.binding;
                if (b.binding >= 0) ub.binding = (GLuint)b.binding;
                else {
                    auto ob = oldBinds.find(b.name);
                    if (ob != oldBinds.end()) ub.binding = ob->second;
                }
                ub.minSize = b.bufferSize;
                size_t end = 0;
                for (auto& m : b.members)
                    end = std::max(end, m.uniformOffset + m.uniformSize);
                ub.trueSize = end;
                pr.uniformBlocks.push_back(ub);
            };
            for (auto& b : vs->conv.blocks) if (!b.isStorage) { addBlock(b, true); pr.vsBlocks.push_back(b.name); }
            for (auto& b : tcv.blocks) if (!b.isStorage) addBlock(b, false);
            for (auto& b : tes->conv.blocks) if (!b.isStorage) addBlock(b, false);
            for (auto& b : fs->conv.blocks) if (!b.isStorage) { addBlock(b, false); pr.fsBlocks.push_back(b.name); }
        }
        
        pr.storageBlocks.clear();
        {
            auto addStorage = [&](const GLSLBlock& b, unsigned mask) {
                for (auto& s : pr.storageBlocks)
                    if (s.name == b.name) { s.stageMask |= mask; return; }
                pr.storageBlocks.push_back({b.name, b.binding, b.bufferSize,
                                            (int)b.members.size(), mask});
            };
            for (auto& b : vs->conv.blocks) if (b.isStorage) addStorage(b, 1u);
            for (auto& b : tcv.blocks) if (b.isStorage) addStorage(b, 8u);
            for (auto& b : tes->conv.blocks) if (b.isStorage) addStorage(b, 16u);
            for (auto& b : fs->conv.blocks) if (b.isStorage) addStorage(b, 2u);
        }
        for (auto& s : tes->conv.tessSlots) {
            if (s.isGlIn) {
                if (s.name != "gl_Position" && s.name != "gl_PointSize") {
                    pr.infoLog = "error: TES gl_in." + s.name + " chưa hỗ trợ";
                    return;
                }
                needCapture = true;
                continue;
            }
            if (!s.isInput) continue;
            bool found = false;
            for (auto& o : tcv.tessSlots)
                if (!o.isInput && !o.isGlIn && o.name == s.name) { found = true; break; }
            if (!found) {
                pr.infoLog = "error: TES in `" + s.name + "` không có TCS out tương ứng";
                return;
            }
        }
        if (c.device && !c.device->isNull()) {
            std::string err;
            if (needCapture) {
                GLSLConvertResult capConv = ConvertGLSLtoMSL(vs->source, 0x8B31, true);
                if (!capConv.ok) {
                    pr.infoLog = "error: MSL VS capture convert: " + capConv.log;
                    return;
                }
                pr.vertexCaptureMSL = capConv.msl;
                pr.vsCaptureIndexSlot = capConv.captureIndexSlot;
                pr.vsCaptureDrawSlot = capConv.captureDrawSlot;
                pr.vsCaptureDescSlot = capConv.captureDescSlot;
                for (auto& s : capConv.captureSlots)
                    pr.vsCaptureSlots.push_back({s.name, s.slot, s.mslType, s.isGlIn, s.arraySize});
                
                for (auto& s : tcv.tessSlots) {
                    if (!s.isInput) continue;
                    bool got = false;
                    for (auto& cslot : pr.vsCaptureSlots)
                        if (cslot.name == s.name) { got = true; break; }
                    if (!got) {
                        pr.infoLog = "error: VS capture thiếu slot `" + s.name + "`";
                        return;
                    }
                }
                pr.appleVSCapture = c.device->compileLibrary(pr.vertexCaptureMSL, err);
                if (!pr.appleVSCapture) {
                    pr.infoLog = "error: MSL VS capture compile: " + err +
                                 "\n--- MSL head ---\n" + pr.vertexCaptureMSL.substr(0, 1200);
                    return;
                }
                pr.appleVSCapturePipeline =
                    c.device->makeComputePipeline(pr.appleVSCapture.get(), "TGLMT_vs_capture");
                if (!pr.appleVSCapturePipeline) {
                    pr.infoLog = "error: VS capture compute pipeline create failed";
                    return;
                }
            }
            pr.appleTCS = c.device->compileLibrary(pr.tessControlMSL, err);
            if (!pr.appleTCS) {
                pr.infoLog = "error: MSL TCS compile: " + err + "\n--- MSL head ---\n" +
                             pr.tessControlMSL.substr(0, 1200);
                return;
            }
            pr.appleTCSPipeline = c.device->makeComputePipeline(pr.appleTCS.get(), "TGLMT_tcs");
            if (!pr.appleTCSPipeline) {
                pr.infoLog = "error: TCS compute pipeline create failed (TGLMT_tcs)";
                return;
            }
            pr.appleTES = c.device->compileLibrary(pr.tessEvalMSL, err);
            if (!pr.appleTES) {
                pr.infoLog = "error: MSL TES compile: " + err + "\n--- MSL head ---\n" +
                             pr.tessEvalMSL.substr(0, 1200);
                return;
            }
            pr.appleFS = c.device->compileLibrary(pr.fragmentMSL, err);
            if (!pr.appleFS) {
                pr.infoLog = "error: MSL FS compile: " + err + "\n--- MSL head ---\n" +
                             pr.fragmentMSL.substr(0, 1200);
                return;
            }
            pr.appleVS = c.device->compileLibrary(pr.vertexMSL, err);
            if (!pr.appleVS) {
                pr.infoLog = "error: MSL VS compile: " + err + "\n--- MSL head ---\n" +
                             pr.vertexMSL.substr(0, 1200);
                return;
            }
        }
        pr.tessControlPoints = tcv.tessVertices;
        pr.tessPatchType = tes->conv.tessPatchType;
        pr.tessSpacing = tes->conv.tessSpacing;
        pr.tessVertexOrder = tes->conv.tessVertexOrder;
        pr.tessFactorSlot = tcv.tessFactorSlot;
        pr.tesFactorSlot = tes->conv.tesFactorSlot;
        
        
        pr.attribFill.clear();
        auto ClsComps = [](const std::string& t) -> std::pair<int, int> {
            if (t == "float") return {0, 1};
            if (t == "vec2") return {0, 2};
            if (t == "vec3") return {0, 3};
            if (t == "vec4") return {0, 4};
            if (t == "int") return {1, 1};
            if (t == "ivec2") return {1, 2};
            if (t == "ivec3") return {1, 3};
            if (t == "ivec4") return {1, 4};
            if (t == "uint") return {2, 1};
            if (t == "uvec2") return {2, 2};
            if (t == "uvec3") return {2, 3};
            if (t == "uvec4") return {2, 4};
            return {-1, 0};
        };
        for (auto& a : vs->conv.inputs) {
            auto cc = ClsComps(a.glslType);
            if (cc.first < 0) continue;
            if (a.forceUnsignedAttr) cc.first = 2;
            pr.attribFill.push_back({a.location, (uint8_t)cc.second, (uint8_t)cc.first});
            pr.attribLoc[a.name] = a.location;
            GLenum aty = GlslTypeToGLEnum(a.glslType);
            if (aty) pr.activeAttribs.push_back({a.name, aty, 1});
        }
        for (auto& s : tcv.tessSlots)
            pr.tessSlots.push_back({s.name, s.slot, s.isInput, s.isPatch, s.isGlIn,
                                    s.mslType, s.count, true});
        for (auto& s : tes->conv.tessSlots)
            pr.tessSlots.push_back({s.name, s.slot, s.isInput, s.isPatch, s.isGlIn,
                                    s.mslType, s.count, false});
        pr.linked = true; ++pr.linkGen;
        pr.reflectionComplete = true;
        BuildUniformSpecMap(pr);
        return;
    }
    if (!pr.computeMSL.empty()) {
        
        
        if (c.device && !c.device->isNull()) {
            std::string err;
            pr.appleCS = c.device->compileLibrary(pr.computeMSL, err);
            if (!pr.appleCS) {
                pr.linked = false;
                pr.infoLog = "error: MSL compute compile: " + err + "\n--- MSL head ---\n" +
                             pr.computeMSL.substr(0, 1200);
                return;
            }
            pr.appleCSPipeline = c.device->makeComputePipeline(pr.appleCS.get(), "TGLMT_cs");
            if (!pr.appleCSPipeline) {
                pr.linked = false;
                pr.infoLog = "error: compute pipeline create failed (TGLMT_cs)";
                return;
            }
        }
        pr.linked = true; ++pr.linkGen;
        pr.reflectionComplete = true;
        BuildUniformSpecMap(pr);
        return;
    }
    if (!hasV && !hasF) { pr.infoLog = "error: graphics program needs a shader"; return; }
    if (!pr.separable && hasF && !hasV) {
        pr.infoLog = "error: non-separable fragment program needs vertex shader"; return;
    }
    
    
    
    
    
    GLSLConvertResult vsC = vs ? vs->conv : GLSLConvertResult{};
    GLSLConvertResult fsC = fs ? fs->conv : GLSLConvertResult{};
    
    
    
    
    
    
    bool hasGeometry = false;
    GsProgramInfo gsInfo;
    if (pr.hasGeometryStage) {
        
        
        DropGeometryProgramInfo(p);
        if (!pr.xfbVaryings.empty()) {
            pr.infoLog = "error: XFB + geometry shader chưa hỗ trợ capture";
            return;
        }
        if (!gsh->compiled) {
            pr.infoLog = "error: geometry shader chưa compile được: " + gsh->infoLog;
            return;
        }
        if (!PrepareGeometryProgram(c, pr, vs, fs, gsh, &gsInfo, &vsC, &fsC)) return;
        hasGeometry = true;
    }
    
    
    if (vs && !fs && !pr.separable && pr.xfbVaryings.empty()) {
        
        
        
        
        pr.linked = true; ++pr.linkGen;
        return;
    }
    
    
    
    for (auto& fi : fsC.inputs) {
        if (!hasV) break;
        bool found = false;
        for (auto& vo : vsC.outputs)
            if (vo.name == fi.name) {
                if (vo.mslType != fi.mslType) {
                    pr.infoLog = "error: varying type mismatch: " + fi.name;
                    return;
                }
                found = true;
                break;
            }
        if (!found) { pr.infoLog = "error: fragment input not written by vertex: " + fi.name; return; }
    }
    
    
    {
        int next = 0;
        for (auto& vo : vsC.outputs)
            if (vo.location < kTempLocBase) next = std::max(next, vo.location + 1);
        for (auto& fi : fsC.inputs)
            if (fi.location < kTempLocBase) next = std::max(next, fi.location + 1);
        for (auto& vo : vsC.outputs) {
            GLSLVar* fi = nullptr;
            for (auto& f : fsC.inputs)
                if (f.name == vo.name) { fi = &f; break; }
            bool voExp = vo.location < kTempLocBase;
            bool fiExp = fi && fi->location < kTempLocBase;
            if (voExp && fiExp && vo.location != fi->location) {
                pr.infoLog = "error: varying location mismatch: " + vo.name;
                return;
            }
            int assigned = voExp ? vo.location : (fiExp ? fi->location : next++);
            if (vo.location != assigned) {
                pr.vertexMSL = RewriteLocns(pr.vertexMSL, false, vo.location, assigned);
                vo.location = assigned;
            }
            if (fi && fi->location != assigned) {
                pr.fragmentMSL = RewriteLocns(pr.fragmentMSL, false, fi->location, assigned);
                fi->location = assigned;
            }
        }
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    {
        std::set<int> taken;
        for (auto& a : vsC.inputs) {
            if (a.location < kTempLocBase) taken.insert((int)a.location);
            auto bit = pr.attribBind.find(a.name);
            if (bit != pr.attribBind.end()) taken.insert((int)bit->second);
        }
        int next = 0;
        for (auto& a : vsC.inputs) {
            auto bit = pr.attribBind.find(a.name);
            int assigned = a.location;
            if (bit != pr.attribBind.end()) {
                assigned = (int)bit->second; 
            } else if (assigned >= kTempLocBase) {
                while (taken.count(next)) next++;
                assigned = next;
                taken.insert(next);
            }
            if (assigned != a.location) {
                pr.vertexMSL = RewriteLocns(pr.vertexMSL, true, a.location, assigned);
                a.location = assigned;
            }
            pr.attribLoc[a.name] = assigned;
            {
                GLenum aty = GlslTypeToGLEnum(a.glslType);
                if (aty) pr.activeAttribs.push_back({a.name, aty, 1});
            }
        }
        
        
        pr.attribFill.clear();
        auto ClsComps = [](const std::string& t) -> std::pair<int, int> {
            if (t == "float") return {0, 1};
            if (t == "vec2") return {0, 2};
            if (t == "vec3") return {0, 3};
            if (t == "vec4") return {0, 4};
            if (t == "int") return {1, 1};
            if (t == "ivec2") return {1, 2};
            if (t == "ivec3") return {1, 3};
            if (t == "ivec4") return {1, 4};
            if (t == "uint") return {2, 1};
            if (t == "uvec2") return {2, 2};
            if (t == "uvec3") return {2, 3};
            if (t == "uvec4") return {2, 4};
            return {-1, 0};
        };
        for (auto& a : vsC.inputs) {
            auto cc = ClsComps(a.glslType);
            if (cc.first < 0) continue;
            
            
            if (a.forceUnsignedAttr) cc.first = 2;
            pr.attribFill.push_back({a.location, (uint8_t)cc.second, (uint8_t)cc.first});
        }
        for (size_t i = 0; i < vsC.inputs.size(); ++i)
            for (size_t j = i + 1; j < vsC.inputs.size(); ++j)
                if (vsC.inputs[i].location == vsC.inputs[j].location) {
                    pr.infoLog = "error: attribute location aliasing: '" + vsC.inputs[i].name +
                                 "' and '" + vsC.inputs[j].name + "' both use index " +
                                 std::to_string(vsC.inputs[i].location) +
                                 " (bind chung index cho 2 attribute ACTIVE không biểu diễn được trên Metal)";
                    return;
                }
    }
    
    
    {
        size_t off = 0;
        for (auto& u : vsC.uniforms) {
            if (u.isSampler) continue;
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, true,
                                        GlslTypeToGLEnum(u.glslType), u.glslType, u.arraySize});
        }
        pr.vsUBSize = vsC.uniformBufferSize;
        off = (pr.vsUBSize + 15) & ~((size_t)15);
        for (auto& u : fsC.uniforms) {
            if (u.isSampler) continue;
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, false,
                                        GlslTypeToGLEnum(u.glslType), u.glslType, u.arraySize});
        }
        pr.fsUBSize = fsC.uniformBufferSize;
    }
    
    
    
    
    
    
    {
        
        std::unordered_map<std::string, GLint> explicitLoc;
        std::set<GLint> taken;
        std::unordered_map<GLint, std::string> locOwner; 
        auto collect = [&](const std::vector<GLSLVar>& us) {
            for (auto& u : us) {
                if (u.isSampler) continue;
                if (u.location < 0 || u.location >= kTempLocBase) continue; 
                auto it = explicitLoc.find(u.name);
                if (it != explicitLoc.end()) {
                    if (it->second != u.location) {
                        pr.infoLog = "error: uniform `" + u.name +
                                     "` khác location explicit giữa các stage (" +
                                     std::to_string(it->second) + " vs " +
                                     std::to_string(u.location) + ")";
                        return false;
                    }
                    continue;
                }
                
                
                
                auto own = locOwner.find(u.location);
                if (own != locOwner.end()) {
                    pr.infoLog = "error: uniform `" + u.name + "` và `" + own->second +
                                 "` cùng location explicit " + std::to_string(u.location);
                    return false;
                }
                explicitLoc[u.name] = u.location;
                locOwner[u.location] = u.name;
                taken.insert(u.location);
            }
            return true;
        };
        if (!collect(vsC.uniforms) || !collect(fsC.uniforms)) {
            pr.linked = false;   
            return;
        }
        
        
        GLint next = 0;
        for (auto& kv : pr.uniformLoc) {
            if (explicitLoc.count(kv.first)) continue; 
            next = std::max(next, kv.second + 1);
        }
        for (auto& [name, loc] : explicitLoc) {
            pr.uniformLoc[name] = loc;
        }
        
        
        
        
        std::unordered_map<std::string, GLint> assigned;
        for (auto& e : pr.uniformLayout) {
            if (e.loc >= 0) continue;               
            auto ex = explicitLoc.find(e.name);
            if (ex != explicitLoc.end()) {
                e.loc = ex->second;
                assigned[e.name] = ex->second;
                continue;
            }
            auto as = assigned.find(e.name);
            if (as != assigned.end()) { e.loc = as->second; continue; }
            auto old = pr.uniformLoc.find(e.name);
            if (old != pr.uniformLoc.end() && !taken.count(old->second)) {
                e.loc = old->second;
                taken.insert(old->second);
                assigned[e.name] = old->second;
                continue;
            }
            while (taken.count(next)) ++next;
            e.loc = next;
            taken.insert(next);
            assigned[e.name] = next;
            pr.uniformLoc[e.name] = next;
            ++next;
        }
    }
    RefreshSubroutines(c, pr);
    
    
    
    
    
    
    pr.activeUniforms.clear();
    pr.activeAttribs.clear();
    {
        std::set<std::string> seenA;
        for (auto& a : vsC.inputs) {
            if (a.isSampler || a.isImage || a.isAtomicCounter) continue;
            if (!seenA.insert(a.name).second) continue;
            GLenum aty = GlslTypeToGLEnum(a.glslType);
            if (!aty) continue;
            pr.activeAttribs.push_back({a.name, aty, 1});
        }
    }
    pr.fragOutputs.clear();
    for (size_t i = 0; i < fsC.outputs.size(); ++i) {
        auto& o = fsC.outputs[i];
        
        
        GLint loc;
        GLuint outIdx = 0;
        auto fb = pr.fragDataBind.find(o.name);
        if (o.location >= 0 && o.location < kTempLocBase) loc = o.location;
        else if (fb != pr.fragDataBind.end()) loc = (GLint)fb->second.color;
        else loc = (GLint)i;
        if (fb != pr.fragDataBind.end()) outIdx = fb->second.index;
        else outIdx = (GLuint)o.index; 
        
        
        
        
        
        
        int srcColor;
        if (fsC.outputs.size() > 1) srcColor = o.location + o.index;
        else if (fsC.fragStructOutputs)
            srcColor = o.hasExplicitLocation ? (o.location + o.index) : 0;
        else srcColor = -1; 
        int dstColor = loc + (int)outIdx;
        if (srcColor >= 0) pr.fragmentMSL = RewriteColorIndex(pr.fragmentMSL, srcColor, dstColor);
        pr.fragOutputs.push_back({o.name, loc, GlslTypeToGLEnum(o.glslType), outIdx});
        o.location = loc; 
    }
    if (fsC.outputs.size() == 1 && !pr.fragOutputs.empty() && pr.fragOutputs[0].location > 0)
        pr.fragmentMSL = WrapSingleFragColor(pr.fragmentMSL, fsC.outputs[0].mslType,
                                             pr.fragOutputs[0].location);
    {
        std::set<std::string> seen;
        auto add = [&](const GLSLVar& u) {
            std::string nm = u.name;
            GLint sz = (u.arraySize > 0) ? (GLint)u.arraySize : 1;
            if (u.arraySize > 0) nm += "[0]";
            if (!seen.insert(nm).second) return;
            GLenum ty = GlslTypeToGLEnum(u.glslType);
            if (!ty) return; 
            pr.activeUniforms.push_back({nm, ty, sz});
        };
        for (auto& u : vsC.uniforms) add(u);
        for (auto& u : fsC.uniforms) add(u);
    }
    
    {
        pr.vsSamplers.clear(); pr.fsSamplers.clear();
        auto kindOf = [](const GLSLVar& s) -> char {
            
            if (s.isBuffer) return s.sampleType == "int" ? 'I' : s.sampleType == "uint" ? 'U' : 'B';
            if (s.isCube) return 'C';
            if (s.isArray) return 'A';
            if (s.isShadow) return 'S';
            if (s.is1D) return '1';
            if (s.is3D) return s.sampleType == "int" ? 'x' : s.sampleType == "uint" ? 'y' : '3';
            if (s.sampleType == "int") return 'i';
            if (s.sampleType == "uint") return 'u';
            return '2';
        };
        for (auto& s : vsC.samplers) {
            pr.vsSamplers.push_back(s.name);
            pr.samplerKind[s.name] = kindOf(s);
            if (s.samplerSlot >= 0) pr.samplerSlots[s.name] = s.samplerSlot;
            if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
        }
        for (auto& s : fsC.samplers) {
            pr.fsSamplers.push_back(s.name);
            pr.samplerKind[s.name] = kindOf(s);
            if (s.samplerSlot >= 0) pr.samplerSlots[s.name] = s.samplerSlot;
            if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
        }
        
        pr.vsImages.clear(); pr.fsImages.clear();
        for (size_t k = 0; k < vsC.images.size(); ++k) {
            auto& im = vsC.images[k];
            int slot = (int)pr.vsSamplers.size() + (int)k;
            int binding = im.imageBinding;
            if (binding < 0) {
                auto uit = pr.samplerUnits.find(im.name);
                binding = (uit == pr.samplerUnits.end()) ? 0 : (int)uit->second;
            }
            pr.vsImages.push_back({im.name, slot, binding});
            if (!pr.samplerUnits.count(im.name)) pr.samplerUnits[im.name] = (GLuint)binding;
        }
        for (size_t k = 0; k < fsC.images.size(); ++k) {
            auto& im = fsC.images[k];
            int slot = (int)pr.fsSamplers.size() + (int)k;
            int binding = im.imageBinding;
            if (binding < 0) {
                auto uit = pr.samplerUnits.find(im.name);
                binding = (uit == pr.samplerUnits.end()) ? 0 : (int)uit->second;
            }
            pr.fsImages.push_back({im.name, slot, binding});
            if (!pr.samplerUnits.count(im.name)) pr.samplerUnits[im.name] = (GLuint)binding;
        }
        
        pr.vsAtomics.clear(); pr.fsAtomics.clear();
        for (auto& as : vsC.atomicSlots) pr.vsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        for (auto& as : fsC.atomicSlots) pr.fsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
    }
    
    
    
    
    
    
    
    {
        
        
        std::unordered_map<std::string, GLuint> oldBinds;
        for (auto& b : pr.uniformBlocks) oldBinds[b.name] = b.binding;
        pr.uniformBlocks.clear();
        pr.vsBlocks.clear();
        pr.fsBlocks.clear();
        
        
        
        pr.vsShaderBlocks.clear();
        pr.fsShaderBlocks.clear();
        for (auto& bs : vsC.blockSlots)
            pr.vsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
        for (auto& bs : fsC.blockSlots)
            pr.fsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
        
        
        pr.storageBlocks.clear();
        auto addStorage = [&](const GLSLBlock& b, unsigned mask) {
            for (auto& s : pr.storageBlocks)
                if (s.name == b.name) { s.stageMask |= mask; return; }
            pr.storageBlocks.push_back({b.name, b.binding, b.bufferSize,
                                        (int)b.members.size(), mask});
        };
        for (auto& b : vsC.blocks) if (b.isStorage) addStorage(b, 1u);
        for (auto& b : fsC.blocks) if (b.isStorage) addStorage(b, 2u);
        GLuint idx = 0;
        auto addBlock = [&](const std::string& nm, int layoutBind, bool isVS) {
            for (auto& b : pr.uniformBlocks) if (b.name == nm) return;
            ProgramObject::UniformBlock ub;
            ub.name = nm; ub.index = idx++; ub.binding = 0; ub.isVS = isVS;
            ub.layoutBinding = layoutBind;
            
            
            
            if (layoutBind >= 0) ub.binding = (GLuint)layoutBind;
            else {
                auto ob = oldBinds.find(nm);
                if (ob != oldBinds.end()) ub.binding = ob->second;
            }
            pr.uniformBlocks.push_back(ub);
        };
        for (auto& b : vsC.blocks) {
            if (b.isStorage) continue; 
            addBlock(b.name, b.binding, true);
            pr.vsBlocks.push_back(b.name);
        }
        for (auto& b : fsC.blocks) {
            if (b.isStorage) continue;
            addBlock(b.name, b.binding, false);
            pr.fsBlocks.push_back(b.name);
        }
        
        
        {
            auto trueSize = [](const std::vector<GLSLVar>& members) {
                size_t end = 0;
                for (auto& m : members)
                    end = std::max(end, m.uniformOffset + m.uniformSize);
                return end;
            };
            for (auto& ub : pr.uniformBlocks) {
                size_t need = 0;
                for (auto& b : vsC.blocks)
                    if (b.name == ub.name) need = std::max(need, trueSize(b.members));
                for (auto& b : fsC.blocks)
                    if (b.name == ub.name) need = std::max(need, trueSize(b.members));
                
                
                
                
                
                ub.trueSize = need;
                ub.minSize = (need + 15) & ~(size_t)15;
            }
        }
        
        if (c.DiagOn()) {
            static int nLb = 0;
            if (++nLb <= 120) {
                std::string all;
                for (auto& ub : pr.uniformBlocks) {
                    char b[96];
                    snprintf(b, sizeof(b), "%s#%u->b%u(lay%d,%zuB) ", ub.name.c_str(),
                             ub.index, ub.binding, ub.layoutBinding, ub.minSize);
                    all += b;
                }
                fprintf(stderr, "[TGLMT] linkblk#%d prog@%u %s\n", nLb, p, all.c_str());
                fflush(stderr);
            }
        }
        
        
        if (c.state.ClipDepth() == 0x935F)
            c.LogDebug(0, 0, 0, 0, "glLinkProgram: ZERO_TO_ONE nhưng VS đã bake z-convert mặc định (cần relink)");
    }
    
    
    
    
    {
        bool hasDT = false, hasFog = false, hasProj = false, hasLight = false;
        for (auto& ub : pr.uniformBlocks) {
            if (ub.name == "DynamicTransforms") hasDT = true;
            else if (ub.name == "Fog") hasFog = true;
            else if (ub.name == "Projection") hasProj = true;
            else if (ub.name == "Lighting" || ub.name == "Lightmap") hasLight = true;
        }
        char k0 = 0;
        if (pr.fsSamplers.size() == 1) {
            auto kit = pr.samplerKind.find(pr.fsSamplers[0]);
            if (kit != pr.samplerKind.end()) k0 = kit->second;
        }
        pr.shadowLike = hasDT && hasFog && hasProj && !hasLight && pr.vsSamplers.empty() &&
                        pr.fsSamplers.size() == 1 && k0 == '2';
    }
    
    
    
    
    if (c.DiagOn()) {
        static int nLink = 0;
        if (++nLink <= 128)
            fprintf(stderr,
                    "[TGLMT] link#%d prog=%u vsInputs=%zu usesVertexID=%d usesInstID=%d "
                    "vsOut=%zu fsIn=%zu blocks=%zu\n",
                    nLink, p, vsC.inputs.size(), (int)vsC.usesVertexID,
                    (int)vsC.usesInstanceID, vsC.outputs.size(), fsC.inputs.size(),
                    vsC.blocks.size());
    }
    
    
    if (c.DiagOn()) {
        static int nTintProg = 0;
        bool any = false;
        for (auto& e : pr.uniformLayout)
            if (e.name == "ColorModulator" || e.name == "FogColor" ||
                e.name == "FogStart" || e.name == "FogEnd") any = true;
        if (any && ++nTintProg <= 64) {
            for (auto& e : pr.uniformLayout)
                if (e.name == "ColorModulator" || e.name == "FogColor" ||
                    e.name == "FogStart" || e.name == "FogEnd")
                    fprintf(stderr,
                            "[TGLMT] tintprog#%d prog=%u %s off=%zu size=%zu loc=%d vs=%d\n",
                            nTintProg, p, e.name.c_str(), e.offset, e.size, e.loc, (int)e.isVS);
        }
    }
    pr.linked = true; ++pr.linkGen;
    pr.reflectionComplete = true;
    BuildUniformSpecMap(pr);
    (void)hasF;
    
    
    if (c.device && !c.device->isNull()) {
        std::string err;
        
        
        const bool xfbOnly = !pr.xfbVaryings.empty() && !hasF;
        if (!pr.vertexMSL.empty() && !xfbOnly) {
            pr.appleVS = c.device->compileLibrary(pr.vertexMSL, err);
            if (!pr.appleVS) {
                pr.linked = false;
                pr.infoLog = "error: MSL vertex compile: " + err + "\n--- MSL head ---\n" +
                             pr.vertexMSL.substr(0, 1200);
                return;
            }
        }
        
        
        
        
        
        if (!pr.xfbVaryings.empty()) {
            if (hasTC || hasTE) {
                pr.linked = false;
                pr.infoLog = "error: XFB + tessellation chưa hỗ trợ capture";
                return;
            }
            if (pr.xfbBufferMode == 0x8C8D && pr.xfbVaryings.size() > 4) {
                pr.linked = false;
                pr.infoLog = "error: XFB separate attribs vượt quá 4 buffer";
                return;
            }
            GLSLConvertResult capConv = ConvertGLSLtoMSL(vs->source, 0x8B31, true);
            if (!capConv.ok) {
                pr.linked = false;
                pr.infoLog = "error: MSL VS capture convert (XFB): " + capConv.log;
                return;
            }
            for (auto& nm : pr.xfbVaryings) {
                std::string base = nm;
                size_t lb = nm.rfind('[');
                int idx = -1;
                if (lb != std::string::npos && !nm.empty() && nm.back() == ']') {
                    std::string ns = nm.substr(lb + 1, nm.size() - lb - 2);
                    bool digits = !ns.empty();
                    for (char ch : ns) if (!isdigit((unsigned char)ch)) digits = false;
                    if (digits) { base = nm.substr(0, lb); idx = atoi(ns.c_str()); }
                }
                bool found = false;
                for (auto& cs : capConv.captureSlots) {
                    if (cs.name != base) continue;
                    if (idx >= 0 && (cs.arraySize <= idx)) break;
                    found = true; break;
                }
                if (!found) {
                    pr.linked = false;
                    pr.infoLog = "error: XFB varying `" + nm + "` không có trong VS output";
                    return;
                }
            }
            pr.vertexCaptureMSL = capConv.msl;
            pr.vsCaptureIndexSlot = capConv.captureIndexSlot;
            pr.vsCaptureDrawSlot = capConv.captureDrawSlot;
            pr.vsCaptureDescSlot = capConv.captureDescSlot;
            for (auto& cs : capConv.captureSlots)
                pr.vsCaptureSlots.push_back({cs.name, cs.slot, cs.mslType, cs.isGlIn, cs.arraySize});
            pr.appleVSCapture = c.device->compileLibrary(pr.vertexCaptureMSL, err);
            if (!pr.appleVSCapture) {
                pr.linked = false;
                pr.infoLog = "error: MSL VS capture compile (XFB): " + err;
                return;
            }
            pr.appleVSCapturePipeline =
                c.device->makeComputePipeline(pr.appleVSCapture.get(), "TGLMT_vs_capture");
            if (!pr.appleVSCapturePipeline) {
                pr.linked = false;
                pr.infoLog = "error: VS capture compute pipeline create failed (XFB)";
                return;
            }
        }
        if (!pr.fragmentMSL.empty()) {
            pr.appleFS = c.device->compileLibrary(pr.fragmentMSL, err);
            if (!pr.appleFS) {
                pr.linked = false;
                pr.infoLog = "error: MSL fragment compile: " + err + "\n--- MSL head ---\n" +
                             pr.fragmentMSL.substr(0, 1200);
                return;
            }
            
            
            std::string maskErr;
            pr.appleFSMasked = c.device->compileLibrary(
                InjectSampleMask(pr.fragmentMSL), maskErr);
        }
    }
    
    if (hasGeometry) {
        if (!pr.linked) return;   
        if (!FinishGeometryProgram(c, pr, vs, fs, &gsInfo, fsC)) return;
    }
    
    
    
    
    if (c.DiagOn()) {
        auto refs = [](const std::string& s, const std::string& nm) {
            size_t n = 0, pos = 0;
            const std::string key = "ubo_" + nm + ".";
            while ((pos = s.find(key, pos)) != std::string::npos) {
                ++n;
                pos += key.size();
            }
            return n;
        };
        for (auto& ub : pr.uniformBlocks) {
            fprintf(stderr, "[TGLMT] msluse# prog@%u %s vs=%zu fs=%zu\n", p, ub.name.c_str(),
                    refs(pr.vertexMSL, ub.name), refs(pr.fragmentMSL, ub.name));
        }
    }
}
void glUseProgram(GLuint p) {
    Context& c = Context::Current();
    if (p && !c.programs.count(p)) { c.errors.Record(0x0502); return; }
    c.state.BindProgram(p);
}
void glProgramParameteri(GLuint p, GLenum n, GLint v) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (n == 0x8258) {
        
        if (v != 0 && v != 1) { c.errors.Record(0x0501); return; } 
        it->second.separable = (v != 0);
    } else if (n == 0x8257) {
        
        
        if (v != 0 && v != 1) { c.errors.Record(0x0501); return; }
        it->second.binaryRetrievable = (v != 0);
    } else {
        c.errors.Record(0x0500); 
    }
}
void glBindAttribLocation(GLuint p, GLuint i, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (n) it->second.attribBind[n] = i; 
}
void glBindFragDataLocation(GLuint p, GLuint col, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; } 
    if (col >= 8) { c.errors.Record(0x0501); return; }                
    if (!n) { c.errors.Record(0x0501); return; }
    it->second.fragDataBind[n] = {col, 0};
}
void glBindFragDataLocationIndexed(GLuint p, GLuint col, GLuint idx, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (col >= 8 || idx > 1) { c.errors.Record(0x0501); return; }
    if (!n) { c.errors.Record(0x0501); return; }
    it->second.fragDataBind[n] = {col, idx};
}
void glGetAttachedShaders(GLuint p, GLsizei m, GLsizei* c_, GLuint* s) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    GLsizei k = std::min(m, (GLsizei)it->second.shaders.size());
    for (GLsizei i = 0; i < k; ++i) s[i] = it->second.shaders[i];
    if (c_) *c_ = k;
}

static const uint32_t kBinMagic = 0x54474D54u; 
static const GLenum kBinFmt = 0x54474D54u;
static inline void Put32(unsigned char* o, uint32_t v) {
    o[0] = (unsigned char)(v & 0xFF); o[1] = (unsigned char)((v >> 8) & 0xFF);
    o[2] = (unsigned char)((v >> 16) & 0xFF); o[3] = (unsigned char)((v >> 24) & 0xFF);
}
static inline uint32_t Get32(const unsigned char* o) {
    return (uint32_t)o[0] | ((uint32_t)o[1] << 8) | ((uint32_t)o[2] << 16) |
           ((uint32_t)o[3] << 24);
}

static std::vector<const ShaderObject*> BinShaders(const Context& c,
                                                   const ProgramObject& pr) {
    std::vector<const ShaderObject*> sh;
    for (GLuint s : pr.shaders) {
        auto si = c.shaders.find(s);
        if (si != c.shaders.end() && !si->second.source.empty())
            sh.push_back(&si->second);
    }
    return sh;
}
static size_t ProgramBinSize(const ProgramObject& pr,
                             const std::vector<const ShaderObject*>& sh) {
    size_t n = 8; 
    for (auto* s : sh) n += 8 + s->source.size();
    return n;
}

GLint ProgramBinaryLength(GLuint p) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) return 0;
    std::vector<const ShaderObject*> sh = BinShaders(c, it->second);
    if (!it->second.linked || sh.empty()) return 0;
    return (GLint)ProgramBinSize(it->second, sh);
}
void glGetProgramBinary(GLuint p, GLsizei n, GLsizei* l, GLenum* f, void* b) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) {
        c.errors.Record(0x0502);
        if (l) *l = 0;
        if (f) *f = 0;
        return;
    }
    auto& pr = it->second;
    
    
    if (!pr.binaryRetrievable) {
        c.errors.Record(0x0502);
        if (l) *l = 0;
        if (f) *f = 0;
        if (b && n > 0) memset(b, 0, (size_t)n);
        return;
    }
    std::vector<const ShaderObject*> sh = BinShaders(c, pr);
    
    
    if (!pr.linked || sh.empty()) {
        if (l) *l = 0;
        if (f) *f = 0;
        if (b && n > 0) memset(b, 0, (size_t)n);
        return;
    }
    size_t total = ProgramBinSize(pr, sh);
    if (l) *l = (GLsizei)total;
    if (f) *f = kBinFmt;
    if (!b) return;
    if ((size_t)n < total) { c.errors.Record(0x0501); return; } 
    unsigned char* o = (unsigned char*)b;
    Put32(o, kBinMagic);
    Put32(o + 4, (uint32_t)sh.size());
    o += 8;
    for (auto* s : sh) {
        Put32(o, s->type);
        Put32(o + 4, (uint32_t)s->source.size());
        memcpy(o + 8, s->source.data(), s->source.size());
        o += 8 + s->source.size();
    }
}
void glProgramBinary(GLuint p, GLenum f, const void* b, GLsizei l) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto& pr = it->second;
    pr.linked = false;
    pr.infoLog.clear();
    
    if (f != kBinFmt || !b || l < 8) {
        pr.infoLog = "error: unsupported program binary format";
        c.errors.Record(0x0500 );
        return;
    }
    const unsigned char* base = (const unsigned char*)b;
    const unsigned char* o = base;
    uint32_t magic = Get32(o), count = Get32(o + 4);
    o += 8; 
    if (magic != kBinMagic || count == 0 || count > 64) {
        pr.infoLog = "error: bad program binary";
        c.errors.Record(0x0501);
        return;
    }
    
    pr.shaders.clear();
    for (uint32_t i = 0; i < count; ++i) {
        if ((size_t)(o - base) + 8 > (size_t)l) {
            pr.infoLog = "error: truncated program binary";
            return;
        }
        uint32_t type = Get32(o), len = Get32(o + 4);
        o += 8;
        if (len == 0 || (size_t)(o - base) + len > (size_t)l || len > (32u << 20)) {
            pr.infoLog = "error: truncated program binary";
            return;
        }
        GLuint sh = glCreateShader(type);
        const char* src = (const char*)o;
        GLint sl = (GLint)len;
        glShaderSource(sh, 1, &src, &sl);
        glCompileShader(sh);
        glAttachShader(p, sh);
        o += len;
    }
    glLinkProgram(p);
}
void glValidateProgram(GLuint p) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    it->second.validateStatus = it->second.linked && !it->second.shaders.empty();
}
void glActiveShaderProgram(GLuint pl, GLuint pr) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(pl);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); return; }
    if (pr != 0) {
        auto pit = c.programs.find(pr);
        if (pit == c.programs.end() || !pit->second.linked) { c.errors.Record(0x0502); return; }
    }
    it->second.activeProgram = pr;
}
void glBindProgramPipeline(GLuint p) {
    Context& c = Context::Current();
    if (p && !c.pipelines.count(p)) { c.errors.Record(0x0502); return; }
    c.state.SetShadow(0x825A , &p, 4);
}
void glGenProgramPipelines(GLsizei n, GLuint* p) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::ProgramPipeline, n, p);
    for (GLsizei i = 0; i < n; ++i) c.pipelines[p[i]] = ProgramPipelineObject{p[i]};
}
void glCreateProgramPipelines(GLsizei n, GLuint* p) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::ProgramPipeline, n, p);
    for (GLsizei i = 0; i < n; ++i) c.pipelines[p[i]] = ProgramPipelineObject{p[i]};
}
void glDeleteProgramPipelines(GLsizei n, const GLuint* p) {
    Context& c = Context::Current();
    GLuint bound = 0;
    c.state.GetShadow(0x825A , &bound, 4);
    bool unbind = false;
    for (GLsizei i = 0; i < n; ++i) {
        if (bound && p[i] == bound) unbind = true;
        c.pipelines.erase(p[i]);
    }
    if (unbind) { GLuint z = 0; c.state.SetShadow(0x825A , &z, 4); }
    c.registry.Delete(ObjectKind::ProgramPipeline, n, p);
}
GLboolean glIsProgramPipeline(GLuint p) { return Context::Current().registry.Is(ObjectKind::ProgramPipeline, p) ? 1 : 0; }
void glUseProgramStages(GLuint a, GLbitfield b, GLuint c_) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(a);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); return; }
    const GLbitfield kAll = 0x00000001u | 0x00000002u | 0x00000004u | 0x00000008u |
                            0x00000010u | 0x00000020u;
    
    if (b != 0xFFFFFFFFu && (b & ~kAll) != 0) { c.errors.Record(0x0501); return; }
    if (c_ != 0) {
        auto pit = c.programs.find(c_);
        if (pit == c.programs.end() || !pit->second.linked) { c.errors.Record(0x0502); return; }
        if (!pit->second.separable) { c.errors.Record(0x0502); return; }
    }
    for (GLbitfield bit = 1; bit <= 0x20; bit <<= 1) {
        if (!(b & bit)) continue;
        if (c_ == 0) it->second.stagePrograms.erase(bit);
        else it->second.stagePrograms[bit] = c_;
    }
}
} 

namespace tglmt {

bool DumpProgramMSL(GLuint program, const std::string& dir, std::string& err) {
    Context& c = Context::Current();
    auto it = c.programs.find(program);
    if (it == c.programs.end()) { err = "no program"; return false; }
    if (!it->second.linked) { err = "not linked"; return false; }
    auto write = [&](const std::string& name, const std::string& src) {
        
        std::string path = dir + "/" + name;
        FILE* f = fopen(path.c_str(), "w");
        if (!f) { err = "fopen " + path; return false; }
        fwrite(src.data(), 1, src.size(), f);
        fclose(f);
        return true;
    };
    char base[32];
    snprintf(base, sizeof(base), "prog%u", program);
    if (!write(std::string(base) + ".vert.msl", it->second.vertexMSL)) return false;
    if (!write(std::string(base) + ".frag.msl", it->second.fragmentMSL)) return false;
    return true;
}
} 
