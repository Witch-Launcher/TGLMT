// gl_framebuffer.cpp — FBO/RBO → MTLRenderPassDescriptor.
// Spec §9 (Framebuffer). Metal: colorAttachments[i]/depth/stencil, load/store.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLAppleDraw.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>
using namespace tglmt;

namespace tglmt::gl {
// Chẩn đoán clear (diag): log tổ hợp (fbo, mask, rgba) — dedupe cap 64.
// Nguồn đen màn hình: clear sky-blue có bị gọi cho đúng fbo không, hay một
// clear (0,0,0,0) nào đó quét đè world target.
static void ClrProbeLog(Context& c, GLuint fbo, GLbitfield mask) {
    if (!c.DiagOn()) return;
    char key[96];
    auto b = [](float v) { return (int)((v < 0 ? 0 : v > 1 ? 1 : v) * 255.0f + 0.5f); };
    snprintf(key, sizeof(key), "%u|%u|%02X%02X%02X%02X", fbo, mask, b(c.clearColor[0]),
             b(c.clearColor[1]), b(c.clearColor[2]), b(c.clearColor[3]));
    static std::set<std::string> seen;
    if (seen.size() >= 64 || !seen.insert(key).second) return;
    fprintf(stderr, "[TGLMT] clr# fbo=%u mask=0x%x rgba=(%.3f,%.3f,%.3f,%.3f)\n", fbo, mask,
            c.clearColor[0], c.clearColor[1], c.clearColor[2], c.clearColor[3]);
}
void glGenFramebuffers(GLsizei n, GLuint* f) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Framebuffer, n, f);
    for (GLsizei i = 0; i < n; ++i) c.fbos[f[i]] = FramebufferObject{f[i]};
}
void glCreateFramebuffers(GLsizei n, GLuint* f) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Framebuffer, n, f);
    for (GLsizei i = 0; i < n; ++i) c.fbos[f[i]] = FramebufferObject{f[i]};
}
void glDeleteFramebuffers(GLsizei n, const GLuint* f) {
    Context& c = Context::Current();
    c.registry.Delete(ObjectKind::Framebuffer, n, f);
    ++c.objectGen; // cache wrap-target theo FBO không còn hợp lệ
    // GL 4.6 §9.2: xóa FBO đang bind → binding tự về 0 (default FB), không
    // bao giờ để dangling id (mọi glReadBuffer/glDrawBuffer sau đó áp vào
    // default FB như spec, thay vì bị lặng lẽ bỏ qua).
    for (GLsizei i = 0; i < n; ++i) {
        if (f[i] == 0) continue;
        if (c.state.BoundDrawFBO() == f[i]) c.state.BindFBO(0x8CA9 /*DRAW_FRAMEBUFFER*/, 0);
        if (c.state.BoundReadFBO() == f[i]) c.state.BindFBO(0x8CA8 /*READ_FRAMEBUFFER*/, 0);
    }
    for (GLsizei i = 0; i < n; ++i) c.fbos.erase(f[i]);
}
GLboolean glIsFramebuffer(GLuint f) {
    return Context::Current().registry.Is(ObjectKind::Framebuffer, f) ? 1 : 0;
}
void glBindFramebuffer(GLenum t, GLuint f) {
    Context& c = Context::Current();
    if (t != 0x8CA9 /*DRAW*/ && t != 0x8CA8 /*READ*/ && t != 0x8D40 /*FRAMEBUFFER*/) {
        c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    if (f && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    c.state.BindFBO(t, f);
}
static bool AttachToFBO(Context& c, GLuint fbo, GLenum attach, GLuint tex,
                        GLint level, GLint layer = -1);
static GLuint ResolveFBOForTarget(Context& c, GLenum t);

static bool SupportedRenderbufferFormat(GLenum inf) {
    switch (inf) {
        case 0x8058: case 0x8C43: case 0x8051: case 0x8F96: case 0x8F97:
        case 0x8229: case 0x822B: case 0x8C41: case 0x822F:
        case 0x8231: case 0x8232: case 0x8233: case 0x8234:
        case 0x822D: case 0x822E: case 0x8235: case 0x8236:
        case 0x881A: case 0x881B: case 0x8814: case 0x8815:
        case 0x8C3A: case 0x805B:
        case 0x81A5: case 0x81A6: case 0x81A7: case 0x8CAC:
        case 0x88F0:
            return true;
        default:
            return false;
    }
}
static RenderbufferObject* BoundRenderbuffer(Context& c) {
    GLuint id = 0;
    if (!c.state.GetShadow(0x8CA7 /*RENDERBUFFER_BINDING*/, &id, sizeof(id))) return nullptr;
    auto it = c.renderbuffers.find(id);
    return it == c.renderbuffers.end() ? nullptr : &it->second;
}
static bool RenderbufferStorageCore(Context& c, RenderbufferObject& rb, GLsizei samples,
                                    GLenum inf, GLsizei w, GLsizei h) {
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return false; } // INVALID_VALUE
    if (w > 16384 || h > 16384) { c.errors.Record(0x0501); return false; }
    if (!SupportedRenderbufferFormat(inf)) { c.errors.Record(0x0500); return false; }
    // TGLMT stores RBOs as textures. MSAA RBO maps to a 2D-multisample
    // texture + resolve; draw path detects TextureObject::samples.
    if (samples > 1) {
        if (samples > 4 || (inf != 0x8058 && inf != 0x8C43)) {
            c.errors.Record(0x0502); return false; // INVALID_OPERATION
        }
    }
    if (rb.texture) {
        GLuint old = rb.texture;
        rb.texture = 0;
        glDeleteTextures(1, &old);
    }
    rb.w = w; rb.h = h; rb.internalFormat = inf; rb.samples = samples;
    if (w == 0 || h == 0) return true; // zero-sized storage = empty
    if (samples > 1) {
        glCreateTextures(0x9100 /*TEXTURE_2D_MULTISAMPLE*/, 1, &rb.texture);
        glTextureStorage2DMultisample(rb.texture, samples, inf, w, h, GL_TRUE);
    } else {
        glCreateTextures(0x0DE1 /*TEXTURE_2D*/, 1, &rb.texture);
        glTextureStorage2D(rb.texture, 1, inf, w, h);
    }
    return true;
}
void glBindRenderbuffer(GLenum t, GLuint r) {
    Context& c = Context::Current();
    if (t != 0x8D41 /*RENDERBUFFER*/) { c.errors.Record(0x0500); return; }
    if (r && !c.renderbuffers.count(r)) { c.errors.Record(0x0502); return; }
    c.state.SetShadow(0x8CA7 /*RENDERBUFFER_BINDING*/, &r, sizeof(r));
}
void glGenRenderbuffers(GLsizei n, GLuint* r) {
    Context& c = Context::Current();
    c.registry.Gen(ObjectKind::Renderbuffer, n, r);
    for (GLsizei i = 0; i < n; ++i) c.renderbuffers[r[i]] = RenderbufferObject{r[i]};
}
void glCreateRenderbuffers(GLsizei n, GLuint* r) {
    Context& c = Context::Current();
    c.registry.Create(ObjectKind::Renderbuffer, n, r);
    for (GLsizei i = 0; i < n; ++i) c.renderbuffers[r[i]] = RenderbufferObject{r[i]};
}
void glDeleteRenderbuffers(GLsizei n, const GLuint* r) {
    Context& c = Context::Current();
    std::vector<GLuint> textures;
    for (GLsizei i = 0; i < n; ++i) {
        auto it = c.renderbuffers.find(r[i]);
        if (it == c.renderbuffers.end()) continue;
        if (it->second.texture) textures.push_back(it->second.texture);
        c.renderbuffers.erase(it);
    }
    c.registry.Delete(ObjectKind::Renderbuffer, n, r);
    if (!textures.empty()) glDeleteTextures((GLsizei)textures.size(), textures.data());
}
GLboolean glIsRenderbuffer(GLuint r) { return Context::Current().registry.Is(ObjectKind::Renderbuffer, r) ? 1 : 0; }
void glRenderbufferStorage(GLenum t, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (t != 0x8D41) { c.errors.Record(0x0500); return; }
    RenderbufferObject* rb = BoundRenderbuffer(c);
    if (!rb) { c.errors.Record(0x0502); return; }
    RenderbufferStorageCore(c, *rb, 0, inf, w, h);
}
void glRenderbufferStorageMultisample(GLenum t, GLsizei s, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (t != 0x8D41) { c.errors.Record(0x0500); return; }
    if (s < 0) { c.errors.Record(0x0501); return; }
    RenderbufferObject* rb = BoundRenderbuffer(c);
    if (!rb) { c.errors.Record(0x0502); return; }
    RenderbufferStorageCore(c, *rb, s, inf, w, h);
}
void glNamedRenderbufferStorage(GLuint r, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    auto it = c.renderbuffers.find(r);
    if (it == c.renderbuffers.end()) { c.errors.Record(0x0502); return; }
    RenderbufferStorageCore(c, it->second, 0, inf, w, h);
}
void glNamedRenderbufferStorageMultisample(GLuint r, GLsizei s, GLenum inf, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (s < 0) { c.errors.Record(0x0501); return; }
    auto it = c.renderbuffers.find(r);
    if (it == c.renderbuffers.end()) { c.errors.Record(0x0502); return; }
    RenderbufferStorageCore(c, it->second, s, inf, w, h);
}
static bool AttachRenderbuffer(Context& c, GLuint fbo, GLenum attach, GLenum rt, GLuint rbId) {
    if (rt != 0x8D41 /*RENDERBUFFER*/ && rt != 0) {
        c.errors.Record(0x0500); return false;
    }
    if (!rbId) return AttachToFBO(c, fbo, attach, 0, 0); // detach
    auto rit = c.renderbuffers.find(rbId);
    if (rit == c.renderbuffers.end() || !rit->second.texture) {
        c.errors.Record(0x0502); return false;
    }
    return AttachToFBO(c, fbo, attach, rit->second.texture, 0);
}
void glFramebufferRenderbuffer(GLenum t, GLenum a, GLenum rt, GLuint r) {
    Context& c = Context::Current();
    AttachRenderbuffer(c, ResolveFBOForTarget(c, t), a, rt, r);
}
void glNamedFramebufferRenderbuffer(GLuint f, GLenum a, GLenum rt, GLuint r) {
    AttachRenderbuffer(Context::Current(), f, a, rt, r);
}
void glFramebufferTexture(GLenum t, GLenum a, GLuint x, GLint l) {
    // DSA-core không dùng, nhưng Emulated detach/attach qua 2D; để trung thực
    // (không nuốt lệnh) xử lý như 2D TEXTURE_2D.
    glFramebufferTexture2D(t, a, 0x0DE1 /*TEXTURE_2D*/, x, l);
}
void glFramebufferTexture1D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l) {
    // TGLMT FBO model không phân biệt dimension: attach như 2D.
    glFramebufferTexture2D(t, a, tx, x, l);
}
static GLuint ResolveFBOForTarget(Context& c, GLenum t) {
    // Spec §9.2 (đối chiếu client.jar 26.1.2 DirectStateAccess$Emulated):
    // Emulated.bindFrameBufferTextures(fbo, color, depth, level, target) gọi
    // _glBindFramebuffer(target,fbo) + _glFramebufferTexture2D(target,...).
    // copyTextureToBuffer truyền target=READ (36008); detach cũng READ.
    // Bug cũ: luôn dùng BoundDrawFBO → READ rỗng → glReadPixels 0x0502 (1282)
    // + detach READ nhầm sang DRAW (mất nút/blur sau screenshot).
    if (t == 0x8CA8 /*GL_READ_FRAMEBUFFER*/) return c.state.BoundReadFBO();
    // DRAW (0x8CA9) và FRAMEBUFFER (0x8D40) → DRAW (khi bind qua FRAMEBUFFER
    // thì READ==DRAW nên tương đương; khi READ!=DRAW thì FRAMEBUFFER theo spec
    // tác động DRAW).
    return c.state.BoundDrawFBO();
}
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l) {
    Context& c = Context::Current();
    GLint layer = (tx >= 0x8515 && tx <= 0x851A) ? (GLint)(tx - 0x8515) : -1;
    AttachToFBO(c, ResolveFBOForTarget(c, t), a, x, l, layer);
}
void glFramebufferTexture3D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l, GLint z) {
    Context& c = Context::Current();
    AttachToFBO(c, ResolveFBOForTarget(c, t), a, x, l, z);
    (void)tx;
}
void glFramebufferTextureLayer(GLenum t, GLenum a, GLuint x, GLint l, GLint layer) {
    Context& c = Context::Current();
    AttachToFBO(c, ResolveFBOForTarget(c, t), a, x, l, layer);
}
// P8 ARB_geometry_shader4 (base cho glFramebufferTextureFaceARB alias):
// Attach 1 face của cube map (hoặc layer của array) vào FBO.
void glFramebufferTextureFace(GLenum t, GLenum a, GLuint x, GLint l, GLenum face) {
    Context& c = Context::Current();
    GLint layer = (face >= 0x8515 && face <= 0x851A) ? (GLint)(face - 0x8515) : -1;
    AttachToFBO(c, ResolveFBOForTarget(c, t), a, x, l, layer);
}
// Helper attach tôn trọng fbo chỉ định (fix bug bỏ qua `f` ở Named variants).
static bool AttachToFBO(Context& c, GLuint fbo, GLenum attach, GLuint tex,
                        GLint level, GLint layer) {
    if (fbo == 0) { c.errors.Record(0x0502); return false; } // default FB không attach
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return false; }
    if (level < 0) { c.errors.Record(0x0501); return false; }
    auto& f = it->second;
    if (attach == 0x8CE0) { f.colorTex[0] = tex; f.colorLevel[0] = level; f.colorLayer[0] = layer; }
    else if (attach >= 0x8CE0 && attach <= 0x8CE7) {
        f.colorTex[attach - 0x8CE0] = tex; f.colorLevel[attach - 0x8CE0] = level;
        f.colorLayer[attach - 0x8CE0] = layer;
    }
    else if (attach == 0x8D00) { f.depthTex = tex; f.depthLevel = level; f.depthLayer = layer; }
    else if (attach == 0x8D20) { f.stencilTex = tex; f.stencilLevel = level; f.stencilLayer = layer; }
    else if (attach == 0x821A || attach == 0x84F9) { // DEPTH_STENCIL_ATTACHMENT aliases; draw path reads depthTex
        f.depthStencilTex = tex; f.depthStencilLevel = level; f.depthStencilLayer = layer;
        f.depthTex = tex; f.depthLevel = level; f.depthLayer = layer;
        f.stencilTex = tex; f.stencilLevel = level; f.stencilLayer = layer;
    }
    else { c.errors.Record(0x0500); return false; }
    auto txt = c.textures.find(tex);
    if (txt != c.textures.end() && tex) {
        GLint lw = (GLint)txt->second.w >> level, lh = (GLint)txt->second.h >> level;
        f.w = lw > 0 ? lw : 1; f.h = lh > 0 ? lh : 1;
        if (level == 0) txt->second.wasRT = true;
        // Attach mip level>0 → nội dung level do GPU pass sinh ra; shadow level
        // cũ (thường zero từ TexImage NULL) không còn là nguồn đúng. Cube xóa
        // riêng face; texture khác xóa mipData[level].
        if (level > 0 && !c.device->isNull()) {
            if (layer >= 0 && txt->second.isCube)
                txt->second.faceMips[layer].erase(level);
            else
                txt->second.mipData.erase(level);
        }
    }
    return true;
}
void glNamedFramebufferTexture(GLuint f, GLenum a, GLuint x, GLint l) { AttachToFBO(Context::Current(), f, a, x, l); }
void glNamedFramebufferTextureLayer(GLuint f, GLenum a, GLuint x, GLint l, GLint layer) {
    AttachToFBO(Context::Current(), f, a, x, l, layer);
}
void glFramebufferParameteri(GLenum t, GLenum p, GLint v) {
    // P10 ARB_framebuffer_no_attachments + ARB_sample_locations.
    Context& c = Context::Current();
    GLuint fbo = ResolveFBOForTarget(c, t);
    if (fbo == 0) {
        // Default framebuffer chỉ nhận 2 pname sample-location (spec ARB_sample_locations).
        if (p == 0x9342) c.ext.defaultProgrammableSampleLocations = (GLboolean)(v ? 1 : 0);
        else if (p == 0x9343) c.ext.defaultSampleLocationPixelGrid = (GLboolean)(v ? 1 : 0);
        else c.errors.Record(0x0502); // INVALID_OPERATION
        return;
    }
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    auto& f = it->second;
    // Pnames: DEFAULT_WIDTH(0x9310), HEIGHT(0x9311), LAYERS(0x9312),
    // SAMPLES(0x9313), FIXED_SAMPLE_LOCATIONS(0x9314),
    // FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS(0x9342), PIXEL_GRID(0x9343).
    if (p == 0x9310) f.defaultWidth = v;
    else if (p == 0x9311) f.defaultHeight = v;
    else if (p == 0x9312) f.defaultLayers = v;
    else if (p == 0x9313) f.defaultSamples = v;
    else if (p == 0x9314) f.defaultFixedSampleLocations = (GLboolean)(v ? 1 : 0);
    else if (p == 0x9342) f.programmableSampleLocations = (GLboolean)(v ? 1 : 0);
    else if (p == 0x9343) f.sampleLocationPixelGrid = (GLboolean)(v ? 1 : 0);
    else { c.errors.Record(0x0500); return; } // INVALID_ENUM
    if (v < 0 && p >= 0x9310 && p <= 0x9313) { c.errors.Record(0x0501); return; }
}
void glNamedFramebufferParameteri(GLuint f, GLenum p, GLint v) {
    Context& c = Context::Current();
    auto it = c.fbos.find(f);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    // Reuse logic bằng cách bind tạm? Đơn giản: set trực tiếp (giống trên).
    auto& fo = it->second;
    if (p == 0x9310) fo.defaultWidth = v;
    else if (p == 0x9311) fo.defaultHeight = v;
    else if (p == 0x9312) fo.defaultLayers = v;
    else if (p == 0x9313) fo.defaultSamples = v;
    else if (p == 0x9314) fo.defaultFixedSampleLocations = (GLboolean)(v ? 1 : 0);
    else if (p == 0x9342) fo.programmableSampleLocations = (GLboolean)(v ? 1 : 0);
    else if (p == 0x9343) fo.sampleLocationPixelGrid = (GLboolean)(v ? 1 : 0);
    else { c.errors.Record(0x0500); return; }
    if (v < 0 && p >= 0x9310 && p <= 0x9313) { c.errors.Record(0x0501); return; }
}
void glGetFramebufferParameteriv(GLenum t, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (!v) return;
    GLuint fbo = ResolveFBOForTarget(c, t);
    if (fbo == 0) {
        if (p == 0x9342) *v = c.ext.defaultProgrammableSampleLocations;
        else if (p == 0x9343) *v = c.ext.defaultSampleLocationPixelGrid;
        else c.errors.Record(0x0502);
        return;
    }
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    auto& f = it->second;
    if (p == 0x9310) *v = f.defaultWidth;
    else if (p == 0x9311) *v = f.defaultHeight;
    else if (p == 0x9312) *v = f.defaultLayers;
    else if (p == 0x9313) *v = f.defaultSamples;
    else if (p == 0x9314) *v = f.defaultFixedSampleLocations;
    else if (p == 0x9342) *v = f.programmableSampleLocations;
    else if (p == 0x9343) *v = f.sampleLocationPixelGrid;
    else { c.errors.Record(0x0500); return; }
}
void glGetNamedFramebufferParameteriv(GLuint f, GLenum p, GLint* v) {
    Context& c = Context::Current();
    if (!v) return;
    auto it = c.fbos.find(f);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return; }
    auto& fo = it->second;
    if (p == 0x9310) *v = fo.defaultWidth;
    else if (p == 0x9311) *v = fo.defaultHeight;
    else if (p == 0x9312) *v = fo.defaultLayers;
    else if (p == 0x9313) *v = fo.defaultSamples;
    else if (p == 0x9314) *v = fo.defaultFixedSampleLocations;
    else if (p == 0x9342) *v = fo.programmableSampleLocations;
    else if (p == 0x9343) *v = fo.sampleLocationPixelGrid;
    else { c.errors.Record(0x0500); return; }
}
// --- Draw/Read buffer constant validation -----------------------------------
// GL 4.6 core + ARB_direct_state_access (table 17.4/17.5/17.6):
//   • FBO bị ảnh hưởng  → chấp nhận GL_NONE | GL_COLOR_ATTACHMENTi (i <
//     MAX_COLOR_ATTACHMENTS = 8). Hằng số của default FB (table 17.4) →
//     INVALID_OPERATION; COLOR_ATTACHMENT0+i với i >= 8 → INVALID_OPERATION;
//     còn lại → INVALID_ENUM.
//   • Default FB (f = 0): singular → table 17.4 (NONE, FRONT*/BACK*/LEFT/
//     RIGHT/FRONT_AND_BACK, AUXi); plural (DrawBuffers) → table 17.6 (NONE,
//     FRONT_LEFT/RIGHT, BACK_LEFT/RIGHT, AUXi — FRONT/BACK/LEFT/RIGHT/
//     FRONT_AND_BACK KHÔNG hợp lệ). Hằng số attachment → INVALID_OPERATION;
//     còn lại → INVALID_ENUM. (Hằng số table 17.4 trong plural → INVALID_OPERATION.)
//   • glDrawBuffers: mọi hằng số lặp lại (khác GL_NONE) → INVALID_OPERATION.
// Spec không yêu cầu gì về "buffer được cấp phát" cho default FB trên TGLMT
// (không biết cấu hình drawable) → chấp nhận mọi hằng số hợp lệ, không đoán.
enum class BufConstResult { Ok, InvalidOp, InvalidEnum };
static bool IsDefaultFBConst(GLenum v, bool frontBackOk) {
    switch (v) {
        case 0x0000: // GL_NONE
        case 0x0400: case 0x0401: case 0x0402: case 0x0403: // FRONT_LEFT/RIGHT, BACK_LEFT/RIGHT
        case 0x040C: case 0x040D: case 0x040E: case 0x040F: // AUX0..AUX3
            return true;
        case 0x0404: case 0x0405:                           // FRONT, BACK
        case 0x0406: case 0x0407:                           // LEFT, RIGHT
        case 0x0408:                                        // FRONT_AND_BACK
            return frontBackOk;
        default: return false;
    }
}
static BufConstResult ClassifyBufferConst(GLenum v, bool plural, bool fbo) {
    bool defSingul = IsDefaultFBConst(v, true);
    bool defPlural = IsDefaultFBConst(v, false);
    bool attOk = (v >= 0x8CE0 && v <= 0x8CE7);
    bool attAny = (v >= 0x8CE0 && v <= 0x8CFF); // COLOR_ATTACHMENT0..31
    if (fbo) {
        if (v == 0x0000 || attOk) return BufConstResult::Ok;
        if (attAny || defSingul) return BufConstResult::InvalidOp;
        return BufConstResult::InvalidEnum;
    }
    if (plural ? defPlural : defSingul) return BufConstResult::Ok;
    if (defSingul || attAny) return BufConstResult::InvalidOp; // hằng số bảng khác
    return BufConstResult::InvalidEnum;
}
static void ApplyDrawBuffers(Context& c, GLuint fbo, std::vector<GLenum> v) {
    if (fbo == 0) { // default FB: DRAW_BUFFER (0x0C01) + DRAW_BUFFERi (0x8825+i)
        for (GLuint i = 0; i < 8; ++i) {
            GLenum val = i < v.size() ? v[i] : (GLenum)0x0000;
            c.state.SetShadow(0x8825 + i, &val, 4);
        }
        GLenum first = v.empty() ? (GLenum)0x0000 : v[0];
        c.state.SetShadow(0x0C01, &first, 4);
        return;
    }
    auto it = c.fbos.find(fbo);
    if (it != c.fbos.end()) it->second.drawBuffers = std::move(v);
}
static bool ValidateDrawBufferArray(Context& c, const GLenum* b, GLsizei n,
                                    bool fboMode) {
    for (GLsizei i = 0; i < n; ++i) {
        BufConstResult r = ClassifyBufferConst(b[i], /*plural=*/true, fboMode);
        if (r == BufConstResult::InvalidOp) { c.errors.Record(0x0502); return false; }
        if (r == BufConstResult::InvalidEnum) { c.errors.Record(0x0500); return false; }
    }
    for (GLsizei i = 0; i < n; ++i) {
        if (b[i] == 0x0000) continue; // GL_NONE được lặp lại
        for (GLsizei j = i + 1; j < n; ++j)
            if (b[j] == b[i]) { c.errors.Record(0x0502); return false; }
    }
    return true;
}
static void ApplyReadBuffer(Context& c, GLuint fbo, GLenum b) {
    BufConstResult r = ClassifyBufferConst(b, /*plural=*/false, fbo != 0);
    if (r == BufConstResult::InvalidOp) { c.errors.Record(0x0502); return; }
    if (r == BufConstResult::InvalidEnum) { c.errors.Record(0x0500); return; }
    if (fbo == 0) { c.state.SetShadow(0x0C02, &b, 4); return; }
    auto it = c.fbos.find(fbo);
    if (it != c.fbos.end()) it->second.readBuffer = b;
}
// Core completeness: attachment sizes/layers. Spec §9.4.2 — GL 4.1+
// (ARB_ES2_compatibility, core since 4.1) ĐÃ GỠ các clause DRAW_BUFFER/
// READ_BUFFER: draw/read buffer trỏ attachment trống không còn làm FBO
// incomplete — bị bỏ qua như GL_NONE khi ghi/đọc (spec: fragment color không
// được ghi vào buffer đó). Sample-count consistency is enforced by the draw
// path for 2D attachments; MSAA depth is currently an internal Metal
// attachment (see TRUTH_REPORT limitations).
static GLenum CheckFramebufferComplete(Context& c, FramebufferObject& f) {
    bool any = false;
    GLsizei refW = 0, refH = 0;
    bool refLayered = false;
    GLsizei refLayers = 0;
    auto check = [&](GLuint id, GLint level, GLint layer) -> GLenum {
        if (!id) return 0x8CD5; // GL_FRAMEBUFFER_COMPLETE sentinel: attached
        auto it = c.textures.find(id);
        if (it == c.textures.end() || it->second.w == 0 || it->second.h == 0)
            return 0x8CD6; // INCOMPLETE_ATTACHMENT
        const TextureObject& tx = it->second;
        if (!any) {
            any = true;
            refW = tx.w >> level; if (refW < 1) refW = 1;
            refH = tx.h >> level; if (refH < 1) refH = 1;
            bool isLayeredType = tx.target == 0x8C1A /*2D_ARRAY*/ ||
                                 tx.target == 0x806F /*3D*/ ||
                                 tx.target == 0x8513 /*CUBE*/ ||
                                 tx.target == 0x9009 /*CUBE_ARRAY*/;
            refLayered = (layer < 0) && isLayeredType && tx.d > 1;
            refLayers = refLayered ? (GLsizei)(tx.target == 0x806F ? (tx.d >> level) : tx.d) : 1;
            if (refLayers < 1) refLayers = 1;
        }
        GLsizei w = tx.w >> level; if (w < 1) w = 1;
        GLsizei h = tx.h >> level; if (h < 1) h = 1;
        if (w != refW || h != refH) return 0x8CD9; // INCOMPLETE_DIMENSIONS
        bool isLayeredType = tx.target == 0x8C1A || tx.target == 0x806F ||
                             tx.target == 0x8513 || tx.target == 0x9009;
        bool layered = (layer < 0) && isLayeredType && tx.d > 1;
        GLsizei layers = layered
            ? (GLsizei)(tx.target == 0x806F ? (tx.d >> level) : tx.d) : 1;
        if (layers < 1) layers = 1;
        if (layered != refLayered || (layered && layers != refLayers))
            return 0x8DA8; // INCOMPLETE_LAYER_TARGETS
        return 0x8CD5;
    };
    for (auto& kv : f.colorTex) {
        GLint lvl = 0, lyr = -1;
        auto li = f.colorLevel.find(kv.first);
        if (li != f.colorLevel.end()) lvl = li->second;
        auto yi = f.colorLayer.find(kv.first);
        if (yi != f.colorLayer.end()) lyr = yi->second;
        GLenum st = check(kv.second, lvl, lyr);
        if (st != 0x8CD5) return st;
    }
    GLenum st = check(f.depthTex, f.depthLevel, f.depthLayer);
    if (st != 0x8CD5) return st;
    st = check(f.stencilTex, f.stencilLevel, f.stencilLayer);
    if (st != 0x8CD5) return st;
    st = check(f.depthStencilTex, f.depthStencilLevel, f.depthStencilLayer);
    if (st != 0x8CD5) return st;
    // ARB_framebuffer_no_attachments: a sized no-attachment FBO is complete.
    if (!any && f.defaultWidth <= 0 && f.defaultHeight <= 0)
        return 0x8CD7; // MISSING_ATTACHMENT
    // GL 4.1+ (ARB_ES2_compatibility, core): KHÔNG còn clause draw/read
    // buffer trong completeness — draw/read buffer trỏ attachment trống bị
    // bỏ qua như GL_NONE (không ghi/đọc gì), FBO vẫn complete.
    return 0x8CD5;
}
GLenum glCheckFramebufferStatus(GLenum t) {
    Context& c = Context::Current();
    // target phải là DRAW(0x8CA9) / READ(0x8CA8) / FRAMEBUFFER(0x8D40)
    if (t != 0x8CA9 && t != 0x8CA8 && t != 0x8D40) { c.errors.Record(0x0500); return 0; }
    GLuint fbo = ResolveFBOForTarget(c, t);
    if (fbo == 0) return 0x8CD5; // default framebuffer
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return 0; }
    return CheckFramebufferComplete(c, it->second);
}
GLenum glCheckNamedFramebufferStatus(GLuint fbo, GLenum t) {
    Context& c = Context::Current();
    if (t != 0x8CA9 && t != 0x8CA8 && t != 0x8D40) { c.errors.Record(0x0500); return 0; }
    if (fbo == 0) return 0x8CD5;
    auto it = c.fbos.find(fbo);
    if (it == c.fbos.end()) { c.errors.Record(0x0502); return 0; }
    return CheckFramebufferComplete(c, it->second);
}
void glDrawBuffer(GLenum b) {
    Context& c = Context::Current();
    GLuint fbo = c.state.BoundDrawFBO();
    BufConstResult r = ClassifyBufferConst(b, /*plural=*/false, fbo != 0);
    if (r == BufConstResult::InvalidOp) { c.errors.Record(0x0502); return; }
    if (r == BufConstResult::InvalidEnum) { c.errors.Record(0x0500); return; }
    ApplyDrawBuffers(c, fbo, {b});
}
void glDrawBuffers(GLsizei n, const GLenum* b) {
    Context& c = Context::Current();
    if (n < 0 || n > 8 /*MAX_DRAW_BUFFERS*/) { c.errors.Record(0x0501); return; }
    if (n > 0 && !b) { c.errors.Record(0x0501); return; }
    GLuint fbo = c.state.BoundDrawFBO();
    if (!ValidateDrawBufferArray(c, b, n, fbo != 0)) return; // atomic: không đổi state
    ApplyDrawBuffers(c, fbo, b ? std::vector<GLenum>(b, b + n) : std::vector<GLenum>{});
}
void glNamedFramebufferDrawBuffer(GLuint f, GLenum b) {
    // DSA (ARB_direct_state_access): f == 0 → default framebuffer; khác 0 phải
    // là FBO tồn tại (không thì INVALID_OPERATION).
    Context& c = Context::Current();
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    BufConstResult r = ClassifyBufferConst(b, /*plural=*/false, f != 0);
    if (r == BufConstResult::InvalidOp) { c.errors.Record(0x0502); return; }
    if (r == BufConstResult::InvalidEnum) { c.errors.Record(0x0500); return; }
    ApplyDrawBuffers(c, f, {b});
}
void glNamedFramebufferDrawBuffers(GLuint f, GLsizei n, const GLenum* b) {
    // DSA: f == 0 → default framebuffer; khác 0 phải là FBO tồn tại.
    Context& c = Context::Current();
    if (n < 0 || n > 8 /*MAX_DRAW_BUFFERS*/) { c.errors.Record(0x0501); return; }
    if (n > 0 && !b) { c.errors.Record(0x0501); return; }
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    if (!ValidateDrawBufferArray(c, b, n, f != 0)) return; // atomic: không đổi state
    ApplyDrawBuffers(c, f, b ? std::vector<GLenum>(b, b + n) : std::vector<GLenum>{});
}
void glReadBuffer(GLenum b) {
    // Spec §18.2: áp lên READ_FRAMEBUFFER binding (không phải DRAW).
    Context& c = Context::Current();
    GLuint fbo = c.state.BoundReadFBO();
    ApplyReadBuffer(c, fbo, b);
}
void glNamedFramebufferReadBuffer(GLuint f, GLenum b) {
    // DSA: f == 0 → default read framebuffer; khác 0 phải là FBO tồn tại.
    Context& c = Context::Current();
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    ApplyReadBuffer(c, f, b);
}
// Blit có default framebuffer tham gia (composite cuối menu/post chain ra màn
// hình). Toàn bộ pipeline TGLMT lưu pixels theo quy ước GL-order (byte đầu =
// row 0 = đáy GL, cả shadow lẫn GPU — do viewport GL-correct h<0; present
// mới là chỗ lật hàng sang drawable) nên GPU copy thô là đúng hướng — CPU
// fallback cũng copy thô để nhất quán với đường FBO→FBO.
// Nguồn cho CPU fallback blit: Context::ReadTextureGPU trả BẢN TẠM (P1: shadow
// base-level trên Apple có thể rỗng, và đọc ngược vào tx.pixels sẽ hồi sinh bản
// sao CPU mà P1 đã bỏ). Null backend: ReadTextureGPU trả false → dùng shadow
// (với Null shadow LÀ resource).
static const std::vector<uint8_t>* ShadowOrGPU(Context& c, const TextureObject& tx,
                                               std::vector<uint8_t>& own) {
    if (c.ReadTextureGPU(tx, own)) return &own;
    return tx.pixels.size() >= (size_t)tx.w * tx.h * 4 ? &tx.pixels : nullptr;
}
static void BlitWithDefault(Context& c, GLuint readFbo, GLuint drawFbo,
        GLint s0, GLint s1, GLint s2, GLint s3, GLint d0, GLint d1, GLint d2, GLint d3,
        GLenum filter, GLbitfield mask) {
    if (readFbo == 0 && drawFbo == 0) return; // cùng target, không có gì để copy
    int sw = s2 - s0, sh = s3 - s1, dw = d2 - d0, dh = d3 - d1;
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) { c.errors.Record(0x0501); return; }
    bool scaled = (sw != dw || sh != dh);
    bool linear = (filter != 0x2600 /*NEAREST*/);
    // Rect src == dst (cùng offset + cùng size): LINEAR và NEAREST cho ra CÙNG
    // pixel (mẫu trùng tâm texel) → cứ dùng GPU blit. Trước đây điều kiện
    // "!scaled && !linear" khiến mọi blit LINEAR cùng size rơi CPU fallback
    // (readback 11MB + vòng 2.7M pixel + re-upload) ≈ 11–26ms mỗi frame.
    bool identical = !scaled && s0 == d0 && s1 == d1;
    if (!c.device || c.device->isNull()) return; // Null backend: trace-only
    auto def = c.device->defaultRenderTarget();
    if (!def) { c.errors.Record(0x0502); return; }
    if (!(mask & 0x00004000u)) {
        // DEPTH/STENCIL với default FB: default target của Renderer có depth
        // riêng nhưng game không attach depth vào default FB theo nghĩa GL
        // (draw vào default luôn tắt depth test) → bỏ qua trung thực.
        if (mask & (0x00000100u | 0x00000400u))
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: depth/stencil blit với default FB bỏ qua");
        return;
    }
    uint32_t sx = s0 < 0 ? 0 : (uint32_t)s0;
    uint32_t sy = s1 < 0 ? 0 : (uint32_t)s1;
    uint32_t dx = d0 < 0 ? 0 : (uint32_t)d0;
    uint32_t dy = d1 < 0 ? 0 : (uint32_t)d1;
    // FBO→màn hình (composite cuối menu: game render offscreen rồi blit
    // read=N draw=0 mỗi frame — đường này từng bị return sớm = đen chắc).
    if (readFbo != 0 && drawFbo == 0) {
        if (scaled) ++c.appleStats.blitScaled;
        if (linear) ++c.appleStats.blitLinear;
        if (identical) ++c.appleStats.blitIdentical;
        auto rit = c.fbos.find(readFbo);
        if (rit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        GLint rSlot = c.ReadBufferSlot();
        if (rSlot < 0) { c.errors.Record(0x0502); return; } // READ_BUFFER=GL_NONE
        auto cit = rit->second.colorTex.find(rSlot);
        if (cit == rit->second.colorTex.end()) return;
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end() || !tit->second.gpu) return;
        // Blitprobe (diag đen): đọc NGUỒN trước khi blit ra màn hình — nội dung
        // cuối frame thật sự là sky-blue hay đen. Flush+commit để draw hiện hành
        // đã tới card trước getBytes (giống nhánh CPU fallback dưới). Throttle:
        // lần đầu + đổi nguồn + mỗi 60 blit (~2.5s ở 24fps).
        if (c.DiagOn() && tit->second.w > 0 && tit->second.h > 0 &&
            tit->second.gpu &&
            metal::PixelFormatBytes(tit->second.gpu->pixelFormat()) == 4) {
            static uint64_t nBl = 0;
            static GLuint lastSrc = 0;
            ++nBl;
            GLuint srcTex = cit->second;
            if (nBl == 1 || srcTex != lastSrc || nBl % 60 == 0) {
                lastSrc = srcTex;
                c.FlushPendingEncoder();
                c.CommitAndWait();
                auto wt = c.device->wrapAsTarget(tit->second.gpu.get(), nullptr);
                if (wt) {
                    size_t w = tit->second.w, h = tit->second.h;
                    std::vector<uint8_t> fb(w * h * 4, 0);
                    if (wt->readback(fb.data(), w * 4)) {
                        auto px = [&](size_t x, size_t y, unsigned* o) {
                            const uint8_t* p = fb.data() + (y * w + x) * 4;
                            o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = p[3];
                        };
                        unsigned top[4], ctr[4], bot[4];
                        px(w / 2, h / 8, top);
                        px(w / 2, h / 2, ctr);
                        px(w / 2, h * 7 / 8, bot);
                        size_t stride = std::max<size_t>(1, (w * h) / 8192), nz = 0, tot = 0;
                        for (size_t i = 0; i < w * h; i += stride) {
                            const uint8_t* q = fb.data() + i * 4;
                            if (q[0] | q[1] | q[2] | q[3]) ++nz;
                            ++tot;
                        }
                        fprintf(stderr,
                                "[TGLMT] blitprobe# tex=%u %zux%zu top=(%u,%u,%u,%u) "
                                "ctr=(%u,%u,%u,%u) bot=(%u,%u,%u,%u) nz=%zu/%zu\n",
                                srcTex, w, h, top[0], top[1], top[2], top[3], ctr[0],
                                ctr[1], ctr[2], ctr[3], bot[0], bot[1], bot[2], bot[3],
                                nz, tot);
                    }
                }
            }
        }
        uint32_t cw = std::min({(uint32_t)sw, tit->second.w > sx ? tit->second.w - sx : 0,
                                def->width() > dx ? def->width() - dx : 0});
        uint32_t ch = std::min({(uint32_t)sh, tit->second.h > sy ? tit->second.h - sy : 0,
                                def->height() > dy ? def->height() - dy : 0});
        if (!cw || !ch) { c.errors.Record(0x0501); return; }
        if (identical) {
            if (c.device->blitToTarget(tit->second.gpu.get(), def.get(), sx, sy, cw, ch,
                                       dx, dy)) {
                ++c.appleStats.blitFast;
                return; // fast path (rect src==dst: LINEAR ≡ NEAREST, copy GPU thuần)
            }
            ++c.appleStats.blitFastFail;
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: GPU blitToTarget fail, fallback CPU");
        } else {
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: scaled/LINEAR blit ra màn hình, CPU nearest");
        }
        // CPU fallback: nearest-scale shadow nguồn qua texture tạm (định dạng
        // của default target để blitToTarget chấp nhận) rồi GPU copy vào màn hình.
        // Shadow có thể cũ (draws đi GPU-only) → làm tươi từ GPU trước.
        ++c.appleStats.blitCpu;
        auto& tx = tit->second;
        c.FlushPendingEncoder(); // IR: commit batch trước khi readback shadow
        c.CommitAndWait();
        std::vector<uint8_t> srcOwn;
        const std::vector<uint8_t>* srcPx = ShadowOrGPU(c, tx, srcOwn);
        if (!srcPx || !tx.w || !tx.h) {
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: blit ra màn hình thiếu shadow, bỏ qua");
            return;
        }
        std::vector<uint8_t> tmp((size_t)dw * (size_t)dh * 4, 0);
        for (int y = 0; y < dh; ++y)
            for (int x = 0; x < dw; ++x) {
                uint32_t sxp = (uint32_t)s0 + (uint32_t)x * (uint32_t)sw / (uint32_t)dw;
                uint32_t syp = (uint32_t)s1 + (uint32_t)y * (uint32_t)sh / (uint32_t)dh;
                if (sxp >= tx.w || syp >= tx.h) continue;
                memcpy(tmp.data() + ((size_t)y * dw + x) * 4,
                       srcPx->data() + ((size_t)syp * tx.w + sxp) * 4, 4);
            }
        auto ttmp = c.device->newTextureWithBytes((uint32_t)dw, (uint32_t)dh,
                                                  def->pixelFormat(), tmp.data(),
                                                  (size_t)dw * 4);
        if (!ttmp) {
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: không tạo được texture tạm cho blit màn hình");
            return;
        }
        if (!c.device->blitToTarget(ttmp.get(), def.get(), 0, 0, (uint32_t)dw, (uint32_t)dh,
                                    dx, dy))
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: blitToTarget fallback fail");
        return;
    }
    // Màn hình→FBO (screenshot/capture): GPU copy rồi sync shadow thô.
    if (readFbo == 0 && drawFbo != 0) {
        auto wit = c.fbos.find(drawFbo);
        if (wit == c.fbos.end()) { c.errors.Record(0x0502); return; }
        auto cit = wit->second.colorTex.find(0);
        if (cit == wit->second.colorTex.end()) return;
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end() || !tit->second.gpu) return;
        auto& tx = tit->second;
        uint32_t cw = std::min({(uint32_t)sw, def->width() > sx ? def->width() - sx : 0,
                                tx.w > dx ? tx.w - dx : 0});
        uint32_t ch = std::min({(uint32_t)sh, def->height() > sy ? def->height() - sy : 0,
                                tx.h > dy ? tx.h - dy : 0});
        if (!cw || !ch) { c.errors.Record(0x0501); return; }
        if (identical &&
            c.device->blitFromTarget(def.get(), tx.gpu.get(), sx, sy, cw, ch, dx, dy)) {
            ++c.appleStats.blitFast;
            // GetTexImage/ReadPixels sau blit đọc thẳng GPU (GetTexSubImpl level-0
            // refresh) → KHÔNG giữ bản CPU. Chỉ giữ khi format này không đọc lại
            // được từ GPU (P1: không hồi sinh shadow base-level).
            if (!TexReadableFromGPU(tx)) {
                // blitTo/FromTarget giờ là NoWait (p5) → đọc CPU PHẢI barrier
                // trước, nếu không getBytes trả dữ liệu cũ.
                c.FlushPendingEncoder();
                c.CommitAndWait();
                std::vector<uint8_t> full((size_t)tx.w * tx.h * 4, 0);
                auto wrapped = c.device->wrapAsTarget(tx.gpu.get(), nullptr);
                if (wrapped && wrapped->readback(full.data(), (size_t)tx.w * 4))
                    tx.pixels = std::move(full);
            }
            return;
        }
        c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: blit màn hình→FBO scaled/LINEAR/fail, CPU shadow");
        ++c.appleStats.blitCpu;
        // blitNoWait (p5) + batch commit NoWait → barrier trước khi đọc màn hình.
        c.FlushPendingEncoder();
        c.CommitAndWait();
        std::vector<uint8_t> scr((size_t)def->width() * def->height() * 4, 0);
        if (!def->readback(scr.data(), (size_t)def->width() * 4)) {
            c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: readback màn hình fail");
            return;
        }
        // P1: làm việc trên bản tạm — KHÔNG resize tx.pixels (resize = hồi sinh
        // đúng bản sao CPU đã bỏ). Shadow chỉ được giữ lại khi format không đọc
        // lại được từ GPU.
        std::vector<uint8_t> own;
        std::vector<uint8_t>* dstPx = &tx.pixels;
        if (dstPx->size() < (size_t)tx.w * tx.h * 4) {
            own.assign((size_t)tx.w * tx.h * 4, 0);
            if (!dstPx->empty()) memcpy(own.data(), dstPx->data(), dstPx->size());
            dstPx = &own;
        }
        uint32_t scrW = def->width(), scrH = def->height();
        for (int y = 0; y < dh && (uint32_t)(d1 + y) < tx.h; ++y)
            for (int x = 0; x < dw && (uint32_t)(d0 + x) < tx.w; ++x) {
                uint32_t sxp = (uint32_t)s0 + x * (uint32_t)sw / (uint32_t)dw;
                uint32_t syp = (uint32_t)s1 + y * (uint32_t)sh / (uint32_t)dh;
                if (sxp >= scrW || syp >= scrH) continue;
                memcpy(dstPx->data() + (((size_t)(d1 + y) * tx.w + (d0 + x)) * 4),
                       scr.data() + ((size_t)syp * scrW + sxp) * 4, 4);
            }
        if (dstPx->size() >= (size_t)tx.w * tx.h * 4)
            c.device->updateTexture(tx.gpu.get(), 0, 0, tx.w, tx.h, dstPx->data(),
                                    (size_t)tx.w * 4);
        if (dstPx == &own && !TexReadableFromGPU(tx)) tx.pixels = std::move(own);
        return;
    }
}
void glBlitFramebuffer(GLint s0, GLint s1, GLint s2, GLint s3, GLint d0, GLint d1, GLint d2, GLint d3, GLbitfield m, GLenum f) {
    Context& c = Context::Current();
    c.FlushPendingEncoder(); // IR: blit đọc texture GPU → phải commit batch trước
    c.FlushAllBufferStaging();
    c.FlushAllTextureStaging();
    GLuint readFbo = c.state.BoundReadFBO();
    GLuint drawFbo = c.state.BoundDrawFBO();
    // Chẩn đoán đen màn hình: chỉ khi TGLMT_DIAG=1 để giữ throughput.
    {
        static uint64_t n = 0;
        ++c.appleStats.blits;
        if (c.DiagOn() && ++n <= 5) {
            fprintf(stderr,
                    "[TGLMT] blit#%llu read=%u draw=%u src=(%d,%d)-(%d,%d) dst=(%d,%d)-(%d,%d) "
                    "mask=0x%x filter=0x%x\n",
                    (unsigned long long)n, readFbo, drawFbo, s0, s1, s2, s3, d0, d1, d2, d3,
                    m, f);
            fflush(stderr);
        }
    }
    // Composite cuối (menu/post chain ra màn hình): 1 phía là default FB.
    // Từng return sớm ở đây (đen tuyệt đối từ frame đầu dù mọi draw đều khỏe:
    // game render vào FBO rồi blit read=2 draw=0 mỗi frame).
    if (readFbo == 0 || drawFbo == 0) {
        BlitWithDefault(c, readFbo, drawFbo, s0, s1, s2, s3, d0, d1, d2, d3, f, m);
        return;
    }
    auto rit = c.fbos.find(readFbo);
    auto wit = c.fbos.find(drawFbo);
    if (rit == c.fbos.end() || wit == c.fbos.end()) { c.errors.Record(0x0502); return; }
    int sw = s2 - s0, sh = s3 - s1, dw = d2 - d0, dh = d3 - d1;
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) { c.errors.Record(0x0501); return; }
    bool scaled = (sw != dw || sh != dh);
    // Rect src == dst → LINEAR ≡ NEAREST (mẫu trùng tâm texel): dùng GPU blit.
    bool identical = !scaled && s0 == d0 && s1 == d1;
    if (scaled) ++c.appleStats.blitScaled;
    if (f != 0x2600 /*NEAREST*/) ++c.appleStats.blitLinear;
    if (identical) ++c.appleStats.blitIdentical;
    // LINEAR filter với scale cần sampled-quad (M5c); NEAREST scale nguyên có thể blit lặp — hiện fallback CPU.
    if ((m & 0x00004000u) && rit->second.colorTex.count(0) && wit->second.colorTex.count(0)) {
        // Nguồn CPU cho fallback blit FBO→FBO. P1: đọc GPU vào BẢN TẠM (shadow
        // base-level trên Apple có thể rỗng) — không ghi ngược vào tx.pixels.
        std::vector<uint8_t> srcOwn;
        const std::vector<uint8_t>* srcPx = nullptr;
        auto rs = c.textures.find(rit->second.colorTex[0]);
        auto ws = c.textures.find(wit->second.colorTex[0]);
        if (rs != c.textures.end() && ws != c.textures.end() && rs->second.gpu && ws->second.gpu) {
            if (identical) {
                // cùng rect → blitCopy GPU thật (A11 TBDR copy, không resolve scale)
                // Fast path: pure GPU, không readback (giữ 60fps; sampling dùng GPU mới).
                uint32_t sx = s0 < 0 ? 0 : (uint32_t)s0;
                uint32_t sy = s1 < 0 ? 0 : (uint32_t)s1;
                if (c.device->blitCopy(rs->second.gpu.get(), ws->second.gpu.get(),
                                       sx, sy, (uint32_t)sw, (uint32_t)sh, (uint32_t)d0, (uint32_t)d1)) {
                    ++c.appleStats.blitFast;
                    return;
                }
                ++c.appleStats.blitFastFail;
                ++c.appleStats.blitCpu;
                c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: GPU blit fail, fallback CPU shadow");
                // Rơi xuống CPU: cần nguồn tươi (đọc thẳng GPU).
                c.CommitAndWait();
                srcPx = ShadowOrGPU(c, rs->second, srcOwn);
            } else {
                ++c.appleStats.blitCpu;
                c.LogDebug(0, 0, 0, 0, "glBlitFramebuffer: scaled/LINEAR fallback CPU shadow (M5c sampled-quad)");
                // Downsample blur/post: shadow cũ toàn 0 → đen. Đọc thẳng từ GPU.
                c.CommitAndWait();
                srcPx = ShadowOrGPU(c, rs->second, srcOwn);
            }
        }
        // CPU shadow copy (đúng khi cùng format RGBA8; khác size thì nearest đơn giản)
        auto rs2 = c.textures.find(rit->second.colorTex[0]);
        auto ws2 = c.textures.find(wit->second.colorTex[0]);
        if (rs2 != c.textures.end() && ws2 != c.textures.end() &&
            rs2->second.gpu && ws2->second.gpu) {
            if (!srcPx) srcPx = ShadowOrGPU(c, rs2->second, srcOwn);
            uint32_t rw = rs2->second.w, rh = rs2->second.h;
            uint32_t ww = ws2->second.w, wh = ws2->second.h;
            if (srcPx && rw && rh && ww && wh) {
                // Shadow đích còn → ghi thẳng (đường cũ). Rỗng (P1) → bản tạm,
                // chỉ giữ lại khi format này không đọc lại được từ GPU.
                std::vector<uint8_t> own;
                std::vector<uint8_t>* dstPx = &ws2->second.pixels;
                if (dstPx->size() < (size_t)ww * wh * 4) {
                    own.assign((size_t)ww * wh * 4, 0);
                    if (!dstPx->empty()) memcpy(own.data(), dstPx->data(), dstPx->size());
                    dstPx = &own;
                }
                if (!scaled) {
                    memcpy(dstPx->data(), srcPx->data(),
                           std::min(srcPx->size(), dstPx->size()));
                } else {
                    // nearest scale CPU (đủ cho vanilla framebuffer copy khác res khi xoay)
                    for (uint32_t y = 0; y < (uint32_t)dh && (uint32_t)(d1 + y) < wh; ++y)
                        for (uint32_t x = 0; x < (uint32_t)dw && (uint32_t)(d0 + x) < ww; ++x) {
                            uint32_t sxp = (uint32_t)s0 + x * (uint32_t)sw / (uint32_t)dw;
                            uint32_t syp = (uint32_t)s1 + y * (uint32_t)sh / (uint32_t)dh;
                            if (sxp >= rw || syp >= rh) continue;
                            if ((size_t)syp * rw + sxp > srcPx->size() / 4) continue;
                            memcpy(dstPx->data() + (((size_t)(d1 + y) * ww + (d0 + x)) * 4),
                                   srcPx->data() + ((size_t)syp * rw + sxp) * 4, 4);
                        }
                }
                // sync GPU dst từ nguồn (đúng pixels cho draw tiếp theo)
                if (dstPx->size() >= (size_t)ww * wh * 4)
                    c.device->updateTexture(ws2->second.gpu.get(), 0, 0, ww, wh,
                                            dstPx->data(), (size_t)ww * 4);
                if (dstPx == &own && !TexReadableFromGPU(ws2->second))
                    ws2->second.pixels = std::move(own);
            }
        }
    }
    if (m & 0x00000100u) { // DEPTH_BUFFER_BIT
        auto rs = rit->second.depthTex ? c.textures.find(rit->second.depthTex) : c.textures.end();
        auto ws = wit->second.depthTex ? c.textures.find(wit->second.depthTex) : c.textures.end();
        if (rs != c.textures.end() && ws != c.textures.end() && rs->second.gpu && ws->second.gpu && !scaled)
            c.device->blitCopy(rs->second.gpu.get(), ws->second.gpu.get(),
                               (uint32_t)(s0 < 0 ? 0 : s0), (uint32_t)(s1 < 0 ? 0 : s1),
                               (uint32_t)sw, (uint32_t)sh, (uint32_t)d0, (uint32_t)d1);
    }
}
void glBlitNamedFramebuffer(GLuint r, GLuint d, GLint s0, GLint s1, GLint s2, GLint s3, GLint d0, GLint d1, GLint d2, GLint d3, GLbitfield m, GLenum f) {
    // Spec §18.3 + client.jar 26.1.2 DirectStateAccess$Core.blitFrameBuffers:
    // glBlitNamedFramebuffer(readFbo, drawFbo, ...) — bug cũ bỏ qua r/d, dùng
    // binding hiện tại (sai khi Core path bind khác). Tôn trọng r/d bằng bind tạm.
    Context& c = Context::Current();
    // DSA: r/d == 0 → default framebuffer; khác 0 phải là FBO tồn tại
    // (không thì INVALID_OPERATION) — spec §18.3 / ARB_direct_state_access.
    if (r != 0 && !c.fbos.count(r)) { c.errors.Record(0x0502); return; }
    if (d != 0 && !c.fbos.count(d)) { c.errors.Record(0x0502); return; }
    GLuint savedR = c.state.BoundReadFBO();
    GLuint savedD = c.state.BoundDrawFBO();
    c.state.BindFBO(0x8CA8 /*READ*/, r);
    c.state.BindFBO(0x8CA9 /*DRAW*/, d);
    glBlitFramebuffer(s0, s1, s2, s3, d0, d1, d2, d3, m, f);
    c.state.BindFBO(0x8CA8, savedR);
    c.state.BindFBO(0x8CA9, savedD);
}
// glInvalidateFramebuffer (spec §18.4): nội dung attachment được chỉ định trở nên
// KHÔNG XÁC ĐỊNH. Metal không có "invalidate" — nhưng TBDR (A11) thắng băng thông
// thật nếu pass kế tiếp load attachment đó bằng MTLLoadActionDontCare thay vì Load
// (không cần đọc tile từ DRAM). TGLMT đánh dấu TextureObject::contentUndefined;
// AppleDrawGL tiêu thụ cờ khi tạo encoder kế tiếp và chọn DontCare. Chỉ đánh dấu
// đúng attachment liệt kê trong `attachments` (COLOR_ATTACHMENTi / DEPTH / STENCIL),
// KHÔNG đốt cả target — app invalidate depth chứ không invalidate color là bình thường.
// Cờ được tiêu thụ khi encoder dùng attachment đó sinh ra → nội dung mới do chính
// pass đó sinh, không còn undefined.
static void MarkInvalidate(Context& c, GLuint fboId, GLenum attachment) {
    if (!fboId) return; // default FB (swapchain): không đánh dấu (target khác nhau mỗi frame)
    auto fit = c.fbos.find(fboId);
    if (fit == c.fbos.end()) return;
    auto mark = [&](GLuint texId) {
        if (!texId) return;
        auto tit = c.textures.find(texId);
        if (tit != c.textures.end()) tit->second.contentUndefined = true;
    };
    if (attachment == 0x8CE0 /*COLOR_ATTACHMENT0*/ || attachment == 0x8CE1 ||
        attachment == 0x8CE2 || attachment == 0x8CE3 || attachment == 0x8CE4 ||
        attachment == 0x8CE5 || attachment == 0x8CE6 || attachment == 0x8CE7) {
        // colorTex dùng index dense 0..7 (xem glFramebufferTexture2D: colorTex[0]
        // cho ATTACHMENT0, colorTex[a-0x8CE0] cho ATTACHMENTn) — KHÔNG dùng enum
        // 0x8CE0 làm key.
        GLuint idx = attachment - 0x8CE0;
        auto ait = fit->second.colorTex.find(idx);
        if (ait != fit->second.colorTex.end()) mark(ait->second);
    } else if (attachment == 0x8D00 /*DEPTH_ATTACHMENT*/) {
        mark(fit->second.depthTex);
    } else if (attachment == 0x8D20 /*STENCIL_ATTACHMENT*/) {
        mark(fit->second.stencilTex);
        mark(fit->second.depthStencilTex);
    } else if (attachment == 0x821A /*DEPTH_STENCIL_ATTACHMENT*/) {
        mark(fit->second.depthStencilTex);
        mark(fit->second.depthTex);
    } else if (attachment == 0x1902 /*DEPTH*/ || attachment == 0x1903 /*STENCIL*/) {
        mark(fit->second.depthTex);
    } else if (attachment == 0x84F9 /*COLOR*/) {
        for (auto& [k, v] : fit->second.colorTex) mark(v);
    }
}
void glInvalidateFramebuffer(GLenum t, GLsizei n, const GLenum* a) {
    Context& c = Context::Current();
    // Spec §18.4: invalidate áp cho framebuffer target `t` đang bind (DRAW /
    // READ / FRAMEWORK). Resolve binding theo t.
    GLuint fbo = 0;
    if (t == 0x8CA8 /*READ_FRAMEBUFFER*/) fbo = c.state.BoundReadFBO();
    else if (t == 0x8CA9 /*DRAW_FRAMEBUFFER*/) fbo = c.state.BoundDrawFBO();
    else fbo = c.state.BoundDrawFBO(); // GL_FRAMEBUFFER
    // Nếu target này đang có encoder mở → flush để invalidate có thứ tự đúng
    // (content trước invalidate là của draw đã encode, content sau là undefined).
    if (c.pendingEncoder && c.pendingDrawFBO == fbo) c.FlushPendingEncoder();
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (n == 0 || !a) return; // không liệt kê gì = không invalidate gì
    for (GLsizei i = 0; i < n; ++i) MarkInvalidate(c, fbo, a[i]);
}
void glInvalidateSubFramebuffer(GLenum t, GLsizei n, const GLenum* a, GLint x, GLint y, GLsizei w, GLsizei h) { (void)t;(void)n;(void)a;(void)x;(void)y;(void)w;(void)h; }
void glInvalidateNamedFramebufferData(GLuint f, GLsizei n, const GLenum* a) { (void)f;(void)n;(void)a; }
void glInvalidateNamedFramebufferSubData(GLuint f, GLsizei n, const GLenum* a, GLint x, GLint y, GLsizei w, GLsizei h) { (void)f;(void)n;(void)a;(void)x;(void)y;(void)w;(void)h; }
void glClear(GLbitfield m) {
    Context& c = Context::Current();
    Context::PhaseTimer pt(c.appleStats, &Context::AppleStats::nsClear);
    // Metal: renderPass loadAction=Clear + clearColor/Depth/Stencil — Null: ghi shadow để test đọc
    c.state.SetShadow(0x0B00 /*CLEAR_STATE_MARKER*/, &m, 4);
    c.appleStats.glClears++; // chẩn đoán đen màn hình: clear có được gọi không
    GLuint fbo = c.state.BoundDrawFBO();
    if (m & 0x00004000u) ClrProbeLog(c, fbo, m & 0x00004000u);
    // Immediate (đúng GL): clear trên CHÍNH fbo đang bind. Fallback deferred
    // (Null / chưa có target) giữ nguyên hành vi cũ cho tests.
    if (m & 0x00004000u) c.appleClearColorAll = true; // glClear: mọi draw buffer 1 màu
    if (AppleClearNow(m)) return;
    c.applePendingClear = true; // M5b: draw Apple kế tiếp clear, sau đó LOAD
    c.appleClearMask |= m;
    c.appleClearFBO = fbo; // clrmiss#: so với fbo của draw tiêu thụ
}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    Context& c = Context::Current();
    c.clearColor[0]=r; c.clearColor[1]=g; c.clearColor[2]=b; c.clearColor[3]=a;
}
void glClearDepth(GLdouble d) { Context::Current().clearDepth = d; }
void glClearDepthf(GLfloat d) { Context::Current().clearDepth = d; }
void glClearStencil(GLint s) { Context::Current().clearStencil = s; }
void glClearBufferfv(GLenum b, GLint d, const GLfloat* v) {
    Context& c = Context::Current();
    // COLOR (0x1800) drawbuffer d: GL clear ĐÚNG attachment tại slot d (GL_NONE
    // → no-op). Immediate qua AppleClearBufferImage (wrap riêng slot d, không
    // đụng slot khác); fallback deferred (Null) ghi màu riêng slot.
    if (b == 0x1800 /*COLOR*/ && v) {
        if (d < 0 || d >= 8) { c.errors.Record(0x0501); return; } // INVALID_VALUE
        GLuint fbo = c.state.BoundDrawFBO();
        ClrProbeLog(c, fbo, 0x00004000u);
        // d là CHỈ SỐ DRAW BUFFER (spec: clear đúng DRAW_BUFFERd) — d trỏ
        // GL_NONE / không nằm trong danh sách → không clear gì (đúng GL).
        if (fbo != 0) {
            auto fit = c.fbos.find(fbo);
            if (fit == c.fbos.end()) { c.errors.Record(0x0502); return; }
            const auto& dbs = fit->second.drawBuffers;
            if ((size_t)d >= dbs.size() || DrawBufferAttachmentIndex(dbs[(size_t)d]) < 0)
                return; // NONE/ngoài danh sách → no-op
        } else if (c.CurrentDrawBuffer((GLuint)d) == 0x0000) {
            return; // default FB: fragment color d = NONE
        }
        // Metal attachment d ↔ DRAW_BUFFERd → slot d là chỉ số đúng ở cả hai backend.
        if (AppleClearBufferImage(fbo, d, v)) return;
        for (int k = 0; k < 4; ++k) c.appleClearColorSlot[d][k] = v[k];
        c.appleClearColorSlots |= (uint8_t)(1u << d);
        c.applePendingClear = true;
        c.appleClearMask |= 0x00004000u;
        c.appleClearFBO = fbo;
        return;
    }
    if (b == 0x1801 /*DEPTH*/ && v) {
        c.clearDepth = v[0];
        if (AppleClearNow(0x00000100u)) return;
        c.applePendingClear = true;
        c.appleClearMask |= 0x00000100u;
        c.appleClearFBO = c.state.BoundDrawFBO();
        return;
    }
    // STENCIL (0x1802): phải CLEAR stencil thật (trước đây chỉ lưu giá trị,
    // không clear gì) — d bị ignore như các buffer khác.
    if (b == 0x1802 /*STENCIL*/ && v) {
        c.clearStencil = (GLint)v[0];
        if (AppleClearNow(0x00000400u)) return;
        c.applePendingClear = true;
        c.appleClearMask |= 0x00000400u;
        c.appleClearFBO = c.state.BoundDrawFBO();
        return;
    }
    c.errors.Record(0x0500); // INVALID_ENUM: buffer khác COLOR/DEPTH/STENCIL
    (void)d;
}
void glClearBufferiv(GLenum b, GLint d, const GLint* v) {
    GLfloat f[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearBufferfv(b, d, f);
}
void glClearBufferuiv(GLenum b, GLint d, const GLuint* v) {
    GLfloat f[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearBufferfv(b, d, f);
}
void glClearBufferfi(GLenum b, GLint d, GLfloat dep, GLint st) {
    Context& c = Context::Current();
    (void)d; // DEPTH/STENCIL/DEPTH_STENCIL: d không dùng
    GLbitfield m = 0;
    if (b == 0x1801 /*DEPTH*/) { c.clearDepth = dep; m = 0x00000100u; }
    else if (b == 0x1802 /*STENCIL*/) { c.clearStencil = st; m = 0x00000400u; }
    else if (b == 0x84F9 /*GL_DEPTH_STENCIL*/) {
        c.clearDepth = dep; c.clearStencil = st;
        m = 0x00000100u | 0x00000400u;
    } else { c.errors.Record(0x0500); return; } // INVALID_ENUM
    if (AppleClearNow(m)) return;
    c.applePendingClear = true;
    c.appleClearMask |= m;
    c.appleClearFBO = c.state.BoundDrawFBO();
}
void glClearNamedFramebufferfv(GLuint f, GLenum b, GLint d, const GLfloat* v) {
    // DSA: f == 0 → default draw framebuffer; khác 0 phải là FBO tồn tại
    // (không thì INVALID_OPERATION). Clear áp dụng lên framebuffer ĐÍCH,
    // không phụ thuộc binding hiện tại (bind tạm rồi trả lại).
    Context& c = Context::Current();
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    GLuint saved = c.state.BoundDrawFBO();
    if (f != saved) {
        c.state.BindFBO(0x8CA9 /*DRAW_FRAMEBUFFER*/, f);
        glClearBufferfv(b, d, v);
        c.state.BindFBO(0x8CA9, saved);
        return;
    }
    glClearBufferfv(b, d, v);
}
void glClearNamedFramebufferiv(GLuint f, GLenum b, GLint d, const GLint* v) {
    GLfloat fl[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearNamedFramebufferfv(f, b, d, fl);
}
void glClearNamedFramebufferuiv(GLuint f, GLenum b, GLint d, const GLuint* v) {
    GLfloat fl[4] = {(GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]};
    glClearNamedFramebufferfv(f, b, d, fl);
}
void glClearNamedFramebufferfi(GLuint f, GLenum b, GLint d, GLfloat dep, GLint st) {
    // DSA: f == 0 → default draw framebuffer; khác 0 phải là FBO tồn tại.
    Context& c = Context::Current();
    if (f != 0 && !c.fbos.count(f)) { c.errors.Record(0x0502); return; }
    GLuint saved = c.state.BoundDrawFBO();
    if (f != saved) {
        c.state.BindFBO(0x8CA9, f);
        glClearBufferfi(b, d, dep, st);
        c.state.BindFBO(0x8CA9, saved);
        return;
    }
    glClearBufferfi(b, d, dep, st);
}
} // namespace tglmt::gl
