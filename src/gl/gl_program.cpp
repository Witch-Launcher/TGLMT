// gl_program.cpp — Program link → MTLRender/ComputePipelineState.
// Spec §7 (Programs). Metal: newLibraryWithSource(MSL) + newRenderPipelineState.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
using namespace tglmt;

namespace tglmt::gl {
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
void glLinkProgram(GLuint p) {
    Context& c = Context::Current();
    auto itp = c.programs.find(p);
    if (itp == c.programs.end()) { c.errors.Record(0x0502); return; }
    auto& pr = itp->second;
    pr.linked = false; pr.infoLog.clear();
    pr.uniformLayout.clear();
    pr.attribLoc.clear(); pr.appleVS.reset(); pr.appleFS.reset();
    pr.vsUBSize = pr.fsUBSize = 0;
    ShaderObject *vs = nullptr, *fs = nullptr;
    bool hasV = false, hasF = false;
    pr.hasTessStages = false; pr.hasGeometryStage = false;
    for (GLuint s : pr.shaders) {
        auto its = c.shaders.find(s);
        if (its == c.shaders.end()) { pr.infoLog = "error: missing shader"; return; }
        if (!its->second.compiled) { pr.infoLog = "error: shader not compiled"; return; }
        if (its->second.type == 0x8B31) { hasV = true; vs = &its->second; pr.vertexMSL = vs->msl; }
        if (its->second.type == 0x8B30) { hasF = true; fs = &its->second; pr.fragmentMSL = fs->msl; }
        if (its->second.type == 0x91B9) pr.computeMSL = its->second.msl; // COMPUTE_SHADER
        if (its->second.type == 0x8E88 || its->second.type == 0x8E87) // TCS/TES
            pr.hasTessStages = true;
        if (its->second.type == 0x8DD9) // GEOMETRY_SHADER: Metal không native → cờ fallback
            pr.hasGeometryStage = true;
    }
    if (pr.shaders.empty()) { pr.infoLog = "error: no shaders attached"; return; }
    if (!pr.computeMSL.empty()) { pr.linked = true; return; } // compute program
    if (!hasV) { pr.infoLog = "error: graphics program needs vertex shader"; return; }
    // Snapshot conv: Blaze3D gắn CÙNG ShaderObject vào NHIỀU program (và có thể
    // relink). Link cũ mutate vs->conv/fs->conv tại chỗ (900→0) trong khi MSL
    // copy lại từ vs->msl gốc mỗi lần → link thứ 2 thấy location đã 0 nên bỏ
    // rewrite, Metal nhận attribute(900) out-of-bounds → 1282 → crash game.
    // Từ đây chỉ làm việc trên bản copy, link idempotent, shader gốc giữ nguyên.
    GLSLConvertResult vsC = vs->conv, fsC = fs ? fs->conv : GLSLConvertResult{};
    // Không có fragment: GL cho phép (chỉ depth/raster discard)? — yêu cầu FS để đơn giản
    // và trung thực với M5b (pipeline cần fragment fn). Ghi rõ thay vì im lặng.
    if (vs && !fs) {
        // Vertex-only: hợp lệ trong GL khi không cần màu (rasterizer discard/XFB).
        // TGLMT M5b cần fragment fn → link OK nhưng AppleDraw trả về trace-only.
        pr.linked = true;
        return;
    }
    // 1. Varying matching: mọi fs.in phải có vs.out cùng tên (đúng GL: thiếu → link lỗi).
    //    vs.out thừa không ai đọc: cho phép (GL cho phép).
    for (auto& fi : fsC.inputs) {
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
    // 3. Attribute locations: glBindAttribLocation honoured, explicit giữ, còn lại gán tiếp.
    // Blaze3D bind attribute theo tên element của VertexFormat cho mọi program
    // (javap GlProgram.link, 26.1.2) — kể cả attribute shader không đọc. Attribute
    // inactive đã bị strip ở converter nên còn lại đây đều ACTIVE; nếu 2 tên khác
    // nhau chung 1 index (aliasing thật) thì Metal không biểu diễn được
    // (`attribute used more than once`) → fail rõ ở link thay vì Metal error khó đọc.
    {
        int next = 0;
        for (auto& a : vsC.inputs)
            if (a.location < kTempLocBase) next = std::max(next, a.location + 1);
        for (auto& a : vsC.inputs) {
            auto bit = pr.attribBind.find(a.name);
            int assigned = a.location;
            if (bit != pr.attribBind.end()) {
                assigned = (int)bit->second; // BindAttribLocation (phải gọi trước link)
            } else if (assigned >= kTempLocBase) {
                assigned = next++;
            }
            if (assigned != a.location) {
                pr.vertexMSL = RewriteLocns(pr.vertexMSL, true, a.location, assigned);
                a.location = assigned;
            }
            pr.attribLoc[a.name] = assigned;
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
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, true});
        }
        pr.vsUBSize = vsC.uniformBufferSize;
        off = (pr.vsUBSize + 15) & ~((size_t)15);
        for (auto& u : fsC.uniforms) {
            if (u.isSampler) continue;
            pr.uniformLayout.push_back({u.name, off + u.uniformOffset, u.uniformSize, -1, false});
        }
        pr.fsUBSize = fsC.uniformBufferSize;
    }
    // 4b. Sampler lists + units mặc định 0 (đúng GL) cho AppleDrawGL slot mapping.
    {
        pr.vsSamplers.clear(); pr.fsSamplers.clear();
        auto kindOf = [](const GLSLVar& s) {
            return s.isBuffer ? 'B' : s.isCube ? 'C' : s.isArray ? 'A' : s.isShadow ? 'S' : '2';
        };
        for (auto& s : vsC.samplers) {
            pr.vsSamplers.push_back(s.name);
            pr.samplerKind[s.name] = kindOf(s);
            if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
        }
        for (auto& s : fsC.samplers) {
            pr.fsSamplers.push_back(s.name);
            pr.samplerKind[s.name] = kindOf(s);
            if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
        }
    }
    // 4c. Uniform blocks (UBO read-only): gộp vs+fs theo thứ tự khai báo, index ổn định.
    // Đồng thời giữ thứ tự RIÊNG mỗi stage (vsBlocks/fsBlocks) khớp
    // [[buffer(17+bi)]] mà converter gán trong MSL từng stage (bi = thứ tự khai
    // báo TRONG stage đó). Gộp thứ tự (vs trước) bind chung cho cả 2 stage sẽ
    // lệch slot khi vs/fs khai báo khác nhau (vd post blur: vs dùng
    // SamplerInfo+RotScale, fs dùng Globals+SamplerInfo+BlurConfig) → fs đọc
    // nhầm buffer (Radius rác → loop treo GPU → iOS ban submissions, đen màn).
    {
        pr.uniformBlocks.clear();
        pr.vsBlocks.clear();
        pr.fsBlocks.clear();
        GLuint idx = 0;
        auto addBlock = [&](const std::string& nm, bool isVS) {
            for (auto& b : pr.uniformBlocks) if (b.name == nm) return;
            ProgramObject::UniformBlock ub;
            ub.name = nm; ub.index = idx++; ub.binding = 0; ub.isVS = isVS;
            // giữ binding đã gọi glUniformBlockBinding trước link (nếu có)
            pr.uniformBlocks.push_back(ub);
        };
        for (auto& b : vsC.blocks) {
            addBlock(b.name, true);
            pr.vsBlocks.push_back(b.name);
        }
        for (auto& b : fsC.blocks) {
            addBlock(b.name, false);
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
                ub.minSize = need;
            }
        }
        // ZERO_TO_ONE cảnh báo: shader đã bake z-convert mặc định; app đổi ClipControl
        // depth cần relink (hiện log, M5c recompile tự động).
        if (c.state.ClipDepth() == 0x935F)
            c.LogDebug(0, 0, 0, 0, "glLinkProgram: ZERO_TO_ONE nhưng VS đã bake z-convert mặc định (cần relink)");
    }
    pr.linked = true;
    (void)hasF;
    // 5. Apple backend: biên dịch MTLLibrary ngay tại link (lỗi biên dịch → link fail thật).
    // Kèm excerpt MSL để log MC hiện đủ nguyên nhân từ xa (không cần DumpProgramMSL).
    if (c.device && !c.device->isNull()) {
        std::string err;
        pr.appleVS = c.device->compileLibrary(pr.vertexMSL, err);
        if (!pr.appleVS) {
            pr.linked = false;
            pr.infoLog = "error: MSL vertex compile: " + err + "\n--- MSL head ---\n" +
                         pr.vertexMSL.substr(0, 1200);
            return;
        }
        pr.appleFS = c.device->compileLibrary(pr.fragmentMSL, err);
        if (!pr.appleFS) {
            pr.linked = false;
            pr.infoLog = "error: MSL fragment compile: " + err + "\n--- MSL head ---\n" +
                         pr.fragmentMSL.substr(0, 1200);
            return;
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
    if (n == 0x825B && v) it->second.separable = true; // PROGRAM_SEPARABLE
}
void glBindAttribLocation(GLuint p, GLuint i, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    if (n) it->second.attribBind[n] = i; // có hiệu lực ở LinkProgram (đúng GL)
}
void glBindFragDataLocation(GLuint p, GLuint col, const GLchar* n) { (void)p;(void)col;(void)n; }
void glBindFragDataLocationIndexed(GLuint p, GLuint col, GLuint idx, const GLchar* n) { (void)p;(void)col;(void)idx;(void)n; }
void glGetAttachedShaders(GLuint p, GLsizei m, GLsizei* c_, GLuint* s) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    GLsizei k = std::min(m, (GLsizei)it->second.shaders.size());
    for (GLsizei i = 0; i < k; ++i) s[i] = it->second.shaders[i];
    if (c_) *c_ = k;
}
void glProgramBinary(GLuint p, GLenum f, const void* b, GLsizei l) { (void)p;(void)f;(void)b;(void)l; }
void glGetProgramBinary(GLuint p, GLsizei n, GLsizei* l, GLenum* f, void* b) {
    (void)p;(void)n;(void)l;(void)f;(void)b;
}
void glValidateProgram(GLuint p) { (void)p; }
void glActiveShaderProgram(GLuint pl, GLuint pr) { (void)pl;(void)pr; }
void glBindProgramPipeline(GLuint p) {
    Context& c = Context::Current();
    c.state.SetShadow(0x8254 /*PROGRAM_PIPELINE_BINDING*/, &p, 4);
}
void glGenProgramPipelines(GLsizei n, GLuint* p) { Context::Current().registry.Gen(ObjectKind::ProgramPipeline, n, p); }
void glCreateProgramPipelines(GLsizei n, GLuint* p) { Context::Current().registry.Create(ObjectKind::ProgramPipeline, n, p); }
void glDeleteProgramPipelines(GLsizei n, const GLuint* p) { Context::Current().registry.Delete(ObjectKind::ProgramPipeline, n, p); }
GLboolean glIsProgramPipeline(GLuint p) { return Context::Current().registry.Is(ObjectKind::ProgramPipeline, p) ? 1 : 0; }
void glUseProgramStages(GLuint a, GLbitfield b, GLuint c_) { (void)a;(void)b;(void)c_; }
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
