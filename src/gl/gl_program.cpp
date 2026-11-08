// gl_program.cpp — Program link → MTLRender/ComputePipelineState.
// Spec §7 (Programs). Metal: newLibraryWithSource(MSL) + newRenderPipelineState.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>
using namespace tglmt;

namespace tglmt::gl {
static bool IsIdentChar(char c, bool first) {
    return c == '_' || isalpha((unsigned char)c) || (!first && isdigit((unsigned char)c));
}
// P10: GLSL type string → GL enum (cho program_interface_query TYPE property).
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
    // Sampler (GL_ACTIVE_UNIFORMS reflection — OpenGL spec xóa phần "Shadow"
    // khỏi tên type nhưng pack khai đúng GLSL).
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
    return 0; // unknown
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
    // MRT field name `mrtN` phải đổi cùng lúc với attribute/copy.
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
// Fragment đơn output do converter phát `fragment T TGLMT_fs(...) { ... return
// tglmt_fragColor; }` (KHÔNG có [[color]] — Metal mặc định color(0)). Khi
// glBindFragDataLocation đổi sang attachment khác, bọc return thành struct có
// [[color(loc)]] để output đi đúng draw buffer.
// Fixed-function sample mask (glSampleMaski/glSampleCoverage): MSL không có
// state tương đương. Inject một fragment input [[sample_id]] + buffer mask và
// discard sample bị tắt trước khi shader chạy. Chỉ compile bản này cho MSAA.
static std::string InjectSampleMask(std::string msl) {
    if (msl.find("tglmt_sampleMask") != std::string::npos) return msl;
    // buffer(30) là slot cuối của graphics (SSBO/UBO có thể chiếm); nếu shader
    // đã dùng thì không inject để tránh đè binding.
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
    --close; // index of matching ')' */

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
    // Viết lại [[user(locnFROM)]] hoặc [[attribute(FROM)]] → số thật (token duy nhất).
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

// --- ARB_separate_shader_objects: subroutine reflection ---------------------
// Converter đã hỗ trợ subroutine bằng cách hạ mỗi uniform subroutine thành
// `uniform int` + hàm dispatch switch. Ở đây chỉ cần đọc tên impl/uniform từ
// source GLSL để trả index/location đúng GL; việc chọn hàm do uniform int đảm
// nhiệm (glUniformSubroutinesuiv ghi ordinal của type tương ứng).
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
                // trim type
                size_t b = ty.find_first_not_of(" \t\r\n");
                size_t e = ty.find_last_not_of(" \t\r\n");
                ty = (b == std::string::npos) ? "" : ty.substr(b, e - b + 1);
                size_t lp = src.find('(', rp + 1);
                size_t semi = src.find(';', rp + 1);
                if (lp == std::string::npos || (semi != std::string::npos && semi < lp)) {
                    i = rp + 1; continue;
                }
                std::string before = src.substr(rp + 1, lp - rp - 1);
                // tên hàm = identifier cuối cùng trước '('
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
    // Gán location cho subroutine uniform theo thứ tự khai báo để
    // glUniformSubroutinesuiv(count) và glGetSubroutineUniformLocation khớp nhau.
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

void glLinkProgram(GLuint p) {
    Context& c = Context::Current();
    auto itp = c.programs.find(p);
    if (itp == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto& pr = itp->second;
    pr.linked = false; pr.infoLog.clear();
    pr.reflectionComplete = false;
    pr.uniformLayout.clear();
    // Bytes uniform cũ (loc → data) thuộc layout CŨ; giữ lại sau relink sẽ
    // upload nhầm giá trị cho loc trùng số nhưng khác nghĩa. `uniformLoc`
    // (name → loc) VẪN giữ: app cache loc trước link là hành vi được hỗ trợ.
    pr.uniforms.clear();
    pr.attribLoc.clear(); pr.appleVS.reset(); pr.appleFS.reset();
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
    ShaderObject *vs = nullptr, *fs = nullptr, *tcs = nullptr, *tes = nullptr;
    bool hasV = false, hasF = false, hasTC = false, hasTE = false;
    pr.hasTessStages = false; pr.hasGeometryStage = false;
    for (GLuint s : pr.shaders) {
        auto its = c.shaders.find(s);
        if (its == c.shaders.end()) { pr.infoLog = "error: missing shader"; return; }
        if (!its->second.compiled) { pr.infoLog = "error: shader not compiled"; return; }
        if (its->second.type == 0x8B31) { hasV = true; vs = &its->second; pr.vertexMSL = vs->msl; }
        if (its->second.type == 0x8B30) { hasF = true; fs = &its->second; pr.fragmentMSL = fs->msl; }
        if (its->second.type == 0x8E88) { hasTC = true; tcs = &its->second; pr.tessControlMSL = tcs->msl; }
        if (its->second.type == 0x8E87) { hasTE = true; tes = &its->second; pr.tessEvalMSL = tes->msl; }
        if (its->second.type == 0x91B9) { // COMPUTE_SHADER
            pr.computeMSL = its->second.msl;
            pr.computeLocalSize[0] = its->second.conv.localSize[0];
            pr.computeLocalSize[1] = its->second.conv.localSize[1];
            pr.computeLocalSize[2] = its->second.conv.localSize[2];
            pr.computeBlockSlots = its->second.conv.blockSlots;
            // P4: SSBO program-scope cho glShaderStorageBlockBinding + query 0x92E6.
            for (auto& b : its->second.conv.blocks)
                if (b.isStorage)
                    pr.storageBlocks.push_back({b.name, b.binding, b.bufferSize,
                                                (int)b.members.size(), 4u});
            // P6f: compute samplers (thứ tự khai báo = texture slot) + uniforms.
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
            // Uniforms: 1 stage (isVS=true), offset/size từ converter.
            for (auto& u : its->second.conv.uniforms) {
                if (u.isSampler) continue;
                pr.uniformLayout.push_back({u.name, u.uniformOffset, u.uniformSize, -1, true, GlslTypeToGLEnum(u.glslType)});
            }
            pr.computeUBSize = its->second.conv.uniformBufferSize;
            // P9 atomic counters: copy slots (binding→slot+offset) cho dispatch bind.
            for (auto& as : its->second.conv.atomicSlots)
                pr.computeAtomics.push_back({as.name, as.binding, as.offset, as.slot});
            // P6g images: slot = samplerCount + index, binding = layout hoặc units.
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
        if (its->second.type == 0x8E88 || its->second.type == 0x8E87) // TCS/TES
            pr.hasTessStages = true;
        if (its->second.type == 0x8DD9) // GEOMETRY_SHADER: Metal không native → cờ fallback
            pr.hasGeometryStage = true;
    }
    if (pr.shaders.empty()) { pr.infoLog = "error: no shaders attached"; return; }
    // P-Tess T1: TCS compile được (kernel MSL hợp lệ) nhưng link/draw cần T3/T4
    // (pipeline tess descriptor + TCS compute pre-pass + drawPatches). Fail rõ ở
    // link thay vì link OK nhưng vẽ sai/câm (trung thực, đúng nếp P6f).
    // Draw path cũng đã từ chối hasTessStages + PATCHES (trace-only, xem AppleDrawGL).
    if (pr.hasTessStages) {
        // P-Tess T3/T4a: triangles, TCS=compute, TES=post-tess vertex. Subset đầu
        // tiên chưa có VS prepass (VS attribute → TCS in) và chưa bind uniform/
        // sampler trong tess path — fail RÕ tại link thay vì vẽ sai/câm.
        if (!hasV || !hasF || !hasTC || !hasTE) {
            pr.infoLog = "error: tess program cần đủ VS+TCS+TES+FS";
            return;
        }
        if (!tcs || !tes || (tes->conv.tessPatchType != 3 && tes->conv.tessPatchType != 4)) {
            pr.infoLog = "error: tessellation chỉ hỗ trợ domain triangles/quads (T3/T4)";
            return;
        }
        // TCS không tự khai báo domain. Nếu compile đơn lẻ đã chọn sai factor
        // struct (heuristic N>=4) so với TES, convert lại với domain thật.
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
        // T4b: VS prepass. Link theo tên (GLSL varying match), không dựa
        // location của Metal interstage vì các stage giao tiếp qua buffer.
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
        // T4b stage 2d: atomic counters bind theo binding point.
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
                                                    loc, isVS, ty});
                        std::string nm = u.name;
                        GLint sz = (u.arraySize > 0) ? (GLint)u.arraySize : 1;
                        if (u.arraySize > 0) nm += "[0]";
                        pr.activeUniforms.push_back({nm, ty, sz});
                    }
                    out.push_back({u.name, u.uniformOffset, u.uniformSize});
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
        // T4b stage 2c: image units (texture slot = sampler count + index).
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
        // T4b stage 2b: UBO (block read-only) theo từng stage. Giữ binding từ
        // lần link trước; layout(binding=N) là mặc định đúng GL.
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
        // T4b stage 2b: SSBO program-scope (index/dedupe cho override + query).
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
                // Mọi gl_in/TCS in phải có capture slot (gl_Position/PointSize luôn có).
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
        // attribFill cho capture descriptor (cùng quy ước class/comps với
        // AppleDrawGL: 0=float, 1=int, 2=uint).
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
        }
        for (auto& s : tcv.tessSlots)
            pr.tessSlots.push_back({s.name, s.slot, s.isInput, s.isPatch, s.isGlIn,
                                    s.mslType, s.count, true});
        for (auto& s : tes->conv.tessSlots)
            pr.tessSlots.push_back({s.name, s.slot, s.isInput, s.isPatch, s.isGlIn,
                                    s.mslType, s.count, false});
        pr.linked = true;
        pr.reflectionComplete = true;
        return;
    }
    if (!pr.computeMSL.empty()) {
        // P6f: compute program — biên dịch MTLLibrary + tạo compute pipeline ngay
        // tại link (lỗi → link fail thật, đúng cam kết advertise ⇔ implement).
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
        pr.linked = true;
        pr.reflectionComplete = true;
        return;
    }
    if (!hasV && !hasF) { pr.infoLog = "error: graphics program needs a shader"; return; }
    if (!pr.separable && hasF && !hasV) {
        pr.infoLog = "error: non-separable fragment program needs vertex shader"; return;
    }
    // Snapshot conv: Blaze3D gắn CÙNG ShaderObject vào NHIỀU program (và có thể
    // relink). Link cũ mutate vs->conv/fs->conv tại chỗ (900→0) trong khi MSL
    // copy lại từ vs->msl gốc mỗi lần → link thứ 2 thấy location đã 0 nên bỏ
    // rewrite, Metal nhận attribute(900) out-of-bounds → 1282 → crash game.
    // Từ đây chỉ làm việc trên bản copy, link idempotent, shader gốc giữ nguyên.
    GLSLConvertResult vsC = vs ? vs->conv : GLSLConvertResult{};
    GLSLConvertResult fsC = fs ? fs->conv : GLSLConvertResult{};
    // Không có fragment: GL cho phép (chỉ depth/raster discard)? — yêu cầu FS để đơn giản
    // và trung thực với M5b (pipeline cần fragment fn). Ghi rõ thay vì im lặng.
    if (vs && !fs && !pr.separable && pr.xfbVaryings.empty()) {
        // Vertex-only không XFB: GL chỉ hợp lệ khi rasterizer discard; TGLMT
        // chưa có pipeline fragment nên vẫn giữ đường trace-only cũ.
        // reflectionComplete giữ false: reflection rỗng (không điền) nên
        // glGetUniformLocation vẫn nới lỏng — không có draw thật để dùng loc.
        pr.linked = true;
        return;
    }
    // 1. Varying matching: mọi fs.in phải có vs.out cùng tên (đúng GL: thiếu → link lỗi).
    //    vs.out thừa không ai đọc: cho phép (GL cho phép). FS-only separable thì
    //    link độc lập, việc ghép với VS xảy ra ở pipeline draw/validate.
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
    // 2. Gán locn thật cho varying tạm (>=900). Ưu tiên: explicit khớp nhau;
    //    explicit vs tạm → theo explicit; cả 2 tạm → số mới. Explicit khác nhau → link lỗi.
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
    // 3. Attribute locations: glBindAttribLocation/explicit giữ, còn lại gán index
    // TRỐNG nhỏ nhất (driver desktop cũng vậy — không đụng index đã chiếm bởi
    // explicit location LẪN bởi giá trị đã bind).
    // Người bind ở đây là Iris MixinVertexFormat.bindAttributesIris (gọi trong
    // ShaderCreator.link trước glLinkProgram): bind theo THỨ TỰ element của format
    // của draw — core names ("iris_" + tên), element mở rộng giữ tên gốc
    // (mc_Entity/at_tangent...). Hệ quả thực chiến: attribute shader có mà format
    // của draw KHÔNG có — vd iris_Entity do EntityPatcher inject ("in ivec3
    // iris_Entity") nhưng draw POSITION_TEX của TEXTURED không có element Entity —
    // không được bind; gán "next từ 0" như trước sẽ đè iris_Position@0 rồi tự báo
    // aliasing giả (crash pipeline CR+DH). Index đã chiếm = explicit ∪ bind.
    // Attribute inactive đã bị strip ở converter nên còn lại đây đều ACTIVE; nếu 2
    // tên chung 1 index sau tất cả (aliasing thật — app bind 2 tên cùng index) thì
    // Metal không biểu diễn được → fail rõ ở link thay vì Metal error khó đọc.
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
                assigned = (int)bit->second; // BindAttribLocation (phải gọi trước link)
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
        }
        // Kiểu attribute (cho fill-zero ở AppleDrawGL khi GL array tắt): chỉ các
        // kiểu scalar/vector float/int/uint — mat/bool/double hiếm, không bù.
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
            // iris_Entity: MSL khai uintN (format UShort4 unsigned) → zero-fill
            // phải dùng format UINT, không phải INT (lệch dấu = Metal từ chối).
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
    // 4. Uniform layout gộp (vs trước, fs sau, mỗi stage align 16 đầu khối).
    // Lưu ý: offset gộp để tương thích cũ; AppleDrawGL chuyển về stage-local khi upload.
    {
        size_t off = 0;
        for (auto& u : vsC.uniforms) {
            if (u.isSampler) continue;
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, true, GlslTypeToGLEnum(u.glslType)});
        }
        pr.vsUBSize = vsC.uniformBufferSize;
        off = (pr.vsUBSize + 15) & ~((size_t)15);
        for (auto& u : fsC.uniforms) {
            if (u.isSampler) continue;
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, false, GlslTypeToGLEnum(u.glslType)});
        }
        pr.fsUBSize = fsC.uniformBufferSize;
    }
    RefreshSubroutines(c, pr);
    // 4c. GL_ACTIVE_UNIFORMS reflection — glGetActiveUniform phải trả type/size
    // thật. Iris ProgramUniforms.buildUniforms so type Iris cấp với type chương
    // trình đọc: stub cũ luôn FLOAT nên `iris_LightmapTextureMatrix` (mat4, do
    // Sodium/Vanilla transformer chèn) bị "Disabling that uniform" → lightmap
    // UV = 0 → terrain/entity đen (CR r5.9.3, iPhone 8 Plus build 107).
    // Name mảng theo spec GL: "name[0]" + size = số phần tử.
    pr.activeUniforms.clear();
    pr.fragOutputs.clear();
    for (size_t i = 0; i < fsC.outputs.size(); ++i) {
        auto& o = fsC.outputs[i];
        // layout(location=) thắng; nếu không có thì glBindFragDataLocation(Indexed)
        // quyết định, cuối cùng mới theo thứ tự khai báo (đúng GL).
        GLint loc;
        GLuint outIdx = 0;
        auto fb = pr.fragDataBind.find(o.name);
        if (o.location >= 0 && o.location < kTempLocBase) loc = o.location;
        else if (fb != pr.fragDataBind.end()) loc = (GLint)fb->second.color;
        else loc = (GLint)i;
        if (fb != pr.fragDataBind.end()) outIdx = fb->second.index;
        // Single-output converter luôn phát [[color(0)]] (bất kể layout/binding);
        // MRT dùng [[color(location tạm 900+)]]. Viết lại đúng nguồn.
        int srcColor = (fsC.outputs.size() > 1) ? o.location : 0;
        pr.fragmentMSL = RewriteColorIndex(pr.fragmentMSL, srcColor, loc);
        pr.fragOutputs.push_back({o.name, loc, GlslTypeToGLEnum(o.glslType), outIdx});
        o.location = loc; // để debug/reflection phía sau
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
            if (!ty) return; // type lạ (image/atomic...) — không thêm để khỏi sai
            pr.activeUniforms.push_back({nm, ty, sz});
        };
        for (auto& u : vsC.uniforms) add(u);
        for (auto& u : fsC.uniforms) add(u);
    }
    // 4b. Sampler lists + units mặc định 0 (đúng GL) cho AppleDrawGL slot mapping.
    {
        pr.vsSamplers.clear(); pr.fsSamplers.clear();
        auto kindOf = [](const GLSLVar& s) -> char {
            // P3: đủ kind sampler (1/2/S/3/C/A/B/I/U/i/u/x/y) cho draw path KindTarget.
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
        // P6g images: slot = samplerCount + index (chung texture index space).
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
        // P9 graphics atomics: copy slots từ converter (vs/fs riêng).
        pr.vsAtomics.clear(); pr.fsAtomics.clear();
        for (auto& as : vsC.atomicSlots) pr.vsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
        for (auto& as : fsC.atomicSlots) pr.fsAtomics.push_back({as.name, as.binding, as.offset, as.slot});
    }
    // 4c. Uniform blocks (UBO read-only): gộp vs+fs theo thứ tự khai báo, index ổn định.
    // Đồng thời giữ thứ tự RIÊNG mỗi stage (vsBlocks/fsBlocks) khớp
    // [[buffer(17+bi)]] mà converter gán trong MSL từng stage (bi = thứ tự khai
    // báo TRONG stage đó). Gộp thứ tự (vs trước) bind chung cho cả 2 stage sẽ
    // lệch slot khi vs/fs khai báo khác nhau (vd post blur: vs dùng
    // SamplerInfo+RotScale, fs dùng Globals+SamplerInfo+BlurConfig) → fs đọc
    // nhầm buffer (Radius rác → loop treo GPU → iOS ban submissions, đen màn).
    {
        // Giữ binding từ lần link trước (app gọi glUniformBlockBinding sau link
        // lần 1, không gọi lại sau relink) — xem addBlock bên dưới.
        std::unordered_map<std::string, GLuint> oldBinds;
        for (auto& b : pr.uniformBlocks) oldBinds[b.name] = b.binding;
        pr.uniformBlocks.clear();
        pr.vsBlocks.clear();
        pr.fsBlocks.clear();
        // P4: slot thật per-stage từ converter (UBO 17+k, SSBO xen kẽ cùng dải) —
        // AppleDrawGL bind theo list này; vsBlocks/fsBlocks (UBO names) chỉ còn
        // cho stage-flag query + diag.
        pr.vsShaderBlocks.clear();
        pr.fsShaderBlocks.clear();
        for (auto& bs : vsC.blockSlots)
            pr.vsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
        for (auto& bs : fsC.blockSlots)
            pr.fsShaderBlocks.push_back({bs.name, bs.slot, bs.isStorage});
        // P4: SSBO program-scope — index cho glShaderStorageBlockBinding (vs rồi fs,
        // dedupe tên). Override binding giữ qua relink (ssboBindOverride không clear).
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
            // Ưu tiên: layout(binding=N) trong GLSL → binding giữ từ link trước → 0.
            // layout binding là giá trị mặc định đúng theo GL (glLinkProgram gán
            // từ qualifier); glUniformBlockBinding sau link vẫn override bình thường.
            if (layoutBind >= 0) ub.binding = (GLuint)layoutBind;
            else {
                auto ob = oldBinds.find(nm);
                if (ob != oldBinds.end()) ub.binding = ob->second;
            }
            pr.uniformBlocks.push_back(ub);
        };
        for (auto& b : vsC.blocks) {
            if (b.isStorage) continue; // SSBO: không phải UBO (query riêng)
            addBlock(b.name, b.binding, true);
            pr.vsBlocks.push_back(b.name);
        }
        for (auto& b : fsC.blocks) {
            if (b.isStorage) continue;
            addBlock(b.name, b.binding, false);
            pr.fsBlocks.push_back(b.name);
        }
        // Kích thước struct thật mỗi block (max end-offset members, không pad
        // cuối) để AppleDrawGL từ chối buffer thiếu (misbound → OOB fault A11).
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
                // std140: kích thước block luôn là bội số 16 (Minecraft dùng
                // Std140SizeCalculator + roundToward(uboSize, alignment)). Bản cũ
                // dùng max end-offset (không pad) nên dễ hơn slice thật 1..15
                // byte → tưởng buffer thiếu → bind zero → ma trận toàn 0 →
                // geometry suy biến (ô atlas trống / model vô hình).
                ub.trueSize = need;
                ub.minSize = (need + 15) & ~(size_t)15;
            }
        }
        // Diag: binding sau link — đối chiếu glUniformBlockBinding/ubobind trên máy.
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
        // ZERO_TO_ONE cảnh báo: shader đã bake z-convert mặc định; app đổi ClipControl
        // depth cần relink (hiện log, M5c recompile tự động).
        if (c.state.ClipDepth() == 0x935F)
            c.LogDebug(0, 0, 0, 0, "glLinkProgram: ZERO_TO_ONE nhưng VS đã bake z-convert mặc định (cần relink)");
    }
    // Tag entity_shadow (Fix #6): DT+Fog+Proj, không Lighting/Lightmap, không vs
    // sampler, đúng 1 fs 2D sampler (Sampler0 = shadow.png). AppleDrawGL soi draw
    // của program này: bind sai id / shadow.png bị ghi đè nội dung → shadowmis#
    // + dump texring (bắt chuỗi bind làm unit trỏ sai, không cần debugger).
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
    // Chẩn đoán: shader khai báo attribute bao nhiêu (inputs) và có dùng
    // gl_VertexID không — so với casN=0 (VAO tắt mọi attrib) ở draw time.
    // inputs>0 + casN=0 = pipeline có stage_in nhưng vertex descriptor rỗng
    // → Metal đọc (0,0,0,1) → quad degenerate → atlas bake rỗng.
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
    // Tint 1.21 (standalone uniform): log layout ColorModulator/Fog* để đối
    // chiếu với giá trị game set (uni4f) và bytes thực upload (tintdraw).
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
    pr.linked = true;
    pr.reflectionComplete = true;
    (void)hasF;
    // 5. Apple backend: biên dịch MTLLibrary ngay tại link (lỗi biên dịch → link fail thật).
    // Kèm excerpt MSL để log MC hiện đủ nguyên nhân từ xa (không cần DumpProgramMSL).
    if (c.device && !c.device->isNull()) {
        std::string err;
        // XFB-only: capture kernel thay VS; VS thường không cần và array output
        // không phải stage-out hợp lệ của Metal.
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
        // XFB: Metal không có transform feedback native. Chương trình có
        // glTransformFeedbackVaryings được capture bằng compute prepass chạy
        // đúng VS (attributes/uniforms) rồi pack kết quả vào buffer XFB.
        // Giai đoạn này chỉ nhận VS-only; XFB + GS/tess phải capture sau stage
        // tương ứng (sẽ mở khi đường GS/TES hoàn chỉnh).
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
            // Bản có sample-mask phải tồn tại để MSAA draw phản ánh glSampleMaski;
            // nếu Metal từ chối (shader lạ), fallback bản thường thay vì fail link.
            std::string maskErr;
            pr.appleFSMasked = c.device->compileLibrary(
                InjectSampleMask(pr.fragmentMSL), maskErr);
        }
    }
    // Msluse (diag): số lần MSL tham chiếu `ubo_<Block>.` — trả lời trực tiếp
    // "shader có ĐỌC block này không?". fs=0/vs=0 → block chết (vanilla strip
    // → app không bind → zero-fallback vô hại); fs>0 mà binding sai (uboSmall
    // #) → dữ liệu sai ĐANG được đọc = nguyên nhân đen sáng.
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
    if (n == 0x8258 && v) it->second.separable = true; // PROGRAM_SEPARABLE
}
void glBindAttribLocation(GLuint p, GLuint i, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (n) it->second.attribBind[n] = i; // có hiệu lực ở LinkProgram (đúng GL)
}
void glBindFragDataLocation(GLuint p, GLuint col, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; } // INVALID_OPERATION
    if (col >= 8) { c.errors.Record(0x0501); return; }                // INVALID_VALUE
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
// --- GL_ARB_get_program_binary (get_program_binary) ---------------------------
// Binary = GLSL GỐC của các shader (không serialize được state Metal). Round-trip
// trung thực: glGetProgramBinary ghi (type, source) từng shader; glProgramBinary
// parse → tạo shader mới → compile + link lại → program dùng được y hệt.
// Lưu MSL làm binary cũng được nhưng reload sẽ không tái tạo được uniform/attrib
// location (link mới sinh location) → GLSL + relink là đường trung thực duy nhất.
static const uint32_t kBinMagic = 0x54474D54u; // 'TGMT'
static const GLenum kBinFmt = 0x54474D54u;
static inline void Put32(unsigned char* o, uint32_t v) {
    o[0] = (unsigned char)(v & 0xFF); o[1] = (unsigned char)((v >> 8) & 0xFF);
    o[2] = (unsigned char)((v >> 16) & 0xFF); o[3] = (unsigned char)((v >> 24) & 0xFF);
}
static inline uint32_t Get32(const unsigned char* o) {
    return (uint32_t)o[0] | ((uint32_t)o[1] << 8) | ((uint32_t)o[2] << 16) |
           ((uint32_t)o[3] << 24);
}
// Thu shader còn nguồn GLSL (thiếu nguồn → không serialize được shader đó).
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
    size_t n = 8; // magic + count
    for (auto* s : sh) n += 8 + s->source.size();
    return n;
}
// glGetProgramiv(GL_PROGRAM_BINARY_LENGTH) (gl_get.cpp dùng qua decl extern).
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
    std::vector<const ShaderObject*> sh = BinShaders(c, pr);
    // Chưa link / không có nguồn → không có binary (length=0 = "không có", đúng
    // spec — caller đọc length trước khi cấp buffer).
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
    if ((size_t)n < total) { c.errors.Record(0x0501); return; } // buffer quá nhỏ
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
    // Format khác → trung thực báo lỗi (không đoán nội dung của driver khác).
    if (f != kBinFmt || !b || l < 8) {
        pr.infoLog = "error: unsupported program binary format";
        c.errors.Record(0x0500 /*INVALID_ENUM*/);
        return;
    }
    const unsigned char* base = (const unsigned char*)b;
    const unsigned char* o = base;
    uint32_t magic = Get32(o), count = Get32(o + 4);
    o += 8; // qua header (magic + count) trước khi đọc từng shader
    if (magic != kBinMagic || count == 0 || count > 64) {
        pr.infoLog = "error: bad program binary";
        c.errors.Record(0x0501);
        return;
    }
    // Xóa shader cũ rồi tạo lại từ binary (compile + attach + link).
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
    c.state.SetShadow(0x8254 /*PROGRAM_PIPELINE_BINDING*/, &p, 4);
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
    c.state.GetShadow(0x8254, &bound, 4);
    bool unbind = false;
    for (GLsizei i = 0; i < n; ++i) {
        if (bound && p[i] == bound) unbind = true;
        c.pipelines.erase(p[i]);
    }
    if (unbind) { GLuint z = 0; c.state.SetShadow(0x8254, &z, 4); }
    c.registry.Delete(ObjectKind::ProgramPipeline, n, p);
}
GLboolean glIsProgramPipeline(GLuint p) { return Context::Current().registry.Is(ObjectKind::ProgramPipeline, p) ? 1 : 0; }
void glUseProgramStages(GLuint a, GLbitfield b, GLuint c_) {
    Context& c = Context::Current();
    auto it = c.pipelines.find(a);
    if (it == c.pipelines.end()) { c.errors.Record(0x0502); return; }
    const GLbitfield kAll = 0x00000001u | 0x00000002u | 0x00000004u | 0x00000008u |
                            0x00000010u | 0x00000020u;
    // GL_ALL_SHADER_BITS (0xFFFFFFFF) là bitmask hợp lệ, không phải bit lạ.
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
} // namespace tglmt::gl

namespace tglmt {
// Ghi MSL đã convert của program ra file để đối chiếu trên máy (chẩn đoán từ xa).
bool DumpProgramMSL(GLuint program, const std::string& dir, std::string& err) {
    Context& c = Context::Current();
    auto it = c.programs.find(program);
    if (it == c.programs.end()) { err = "no program"; return false; }
    if (!it->second.linked) { err = "not linked"; return false; }
    auto write = [&](const std::string& name, const std::string& src) {
        // Ghi file tối giản (không dùng <fstream> để khỏi thêm include).
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
} // namespace tglmt
