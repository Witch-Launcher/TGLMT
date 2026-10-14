// gl_program.cpp — Program link → MTLRender/ComputePipelineState.
// Spec §7 (Programs). Metal: newLibraryWithSource(MSL) + newRenderPipelineState.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
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
    pr.appleCS.reset(); pr.appleCSPipeline.reset(); pr.computeMSL.clear();
    pr.computeBlockSlots.clear(); pr.computeSamplers.clear(); pr.computeUBSize = 0;
    pr.vsImages.clear(); pr.fsImages.clear(); pr.computeImages.clear();
    pr.computeAtomics.clear(); pr.vsAtomics.clear(); pr.fsAtomics.clear();
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
        if (its->second.type == 0x91B9) { // COMPUTE_SHADER
            pr.computeMSL = its->second.msl;
            pr.computeLocalSize[0] = its->second.conv.localSize[0];
            pr.computeLocalSize[1] = its->second.conv.localSize[1];
            pr.computeLocalSize[2] = its->second.conv.localSize[2];
            pr.computeBlockSlots = its->second.conv.blockSlots;
            // P6f: compute samplers (thứ tự khai báo = texture slot) + uniforms.
            auto kindOf = [](const GLSLVar& s) {
                return s.isBuffer ? 'B' : s.isCube ? 'C' : s.isArray ? 'A' : s.isShadow ? 'S' : '2';
            };
            for (auto& s : its->second.conv.samplers) {
                pr.computeSamplers.push_back(s.name);
                pr.samplerKind[s.name] = kindOf(s);
                if (!pr.samplerUnits.count(s.name)) pr.samplerUnits[s.name] = 0;
            }
            // Uniforms: 1 stage (isVS=true), offset/size từ converter.
            for (auto& u : its->second.conv.uniforms) {
                if (u.isSampler) continue;
                pr.uniformLayout.push_back({u.name, u.uniformOffset, u.uniformSize, -1, true});
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
        return;
    }
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
            addBlock(b.name, b.binding, true);
            pr.vsBlocks.push_back(b.name);
        }
        for (auto& b : fsC.blocks) {
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
