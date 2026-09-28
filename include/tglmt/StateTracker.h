#pragma once
// StateTracker — toàn bộ GL state machine trên CPU, bake thành descriptor Metal.
// Vì Metal bake state vào pipeline (không glEnable động), mọi thay đổi đánh dirty
// và PipelineCache sẽ tạo lại MTLRenderPipelineState/MTLDepthStencilState/MTLSamplerState.
// Bao phủ: blend (per-target), depth/stencil, cull/frontface, viewport/scissor,
// clipControl, polygonOffset, sampler units, buffer bindings, FBO bindings.
#include "tglmt/gl46_types.h"
#include <array>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstring>

namespace tglmt {

constexpr int kMaxDrawBuffers = 8;
constexpr int kMaxTextureUnits = 32;
constexpr int kMaxVertexAttribs = 16;
constexpr int kMaxViewports = 16;

struct BlendState {
    bool enabled = false;
    GLenum rgbEq = 0x8006; // GL_FUNC_ADD
    GLenum alphaEq = 0x8006;
    GLenum srcRGB = 1;     // GL_ONE
    GLenum dstRGB = 0;     // GL_ZERO
    GLenum srcAlpha = 1;
    GLenum dstAlpha = 0;
};

struct DepthState {
    bool test = false;
    GLenum func = 0x0203; // GL_LESS
    bool writeMask = true;
};

struct StencilFace {
    GLenum func = 0x0207; // GL_ALWAYS
    GLint ref = 0;
    GLuint valueMask = 0xFFFFFFFFu;
    GLuint writeMask = 0xFFFFFFFFu;
    GLenum sfail = 0x1E00;  // KEEP
    GLenum dpfail = 0x1E00;
    GLenum dppass = 0x1E00;
};

struct ViewportState {
    float x = 0, y = 0, w = 0, h = 0, n = 0, f = 1; // GL depth [-1,1] mặc định; Metal convert 0..1
};

struct PixelStoreState {
    GLint packAlignment = 4, unpackAlignment = 4;
    GLint packRowLength = 0, unpackRowLength = 0;
    GLint packSkipPixels = 0, packSkipRows = 0;
    GLint unpackSkipPixels = 0, unpackSkipRows = 0;
    GLint packImageHeight = 0, unpackImageHeight = 0;
    GLint packSkipImages = 0, unpackSkipImages = 0;
};

class StateTracker {
public:
    StateTracker();

    // --- capability (glEnable/glDisable/glEnablei) ---
    void SetEnabled(GLenum cap, bool on, GLuint index = 0);
    bool IsEnabled(GLenum cap, GLuint index = 0) const;

    // --- blend ---
    std::array<BlendState, kMaxDrawBuffers>& Blend() { dirtyPipeline_ = true; return blend_; }
    const std::array<BlendState, kMaxDrawBuffers>& Blend() const { return blend_; }
    void SetBlendColor(float r, float g, float b, float a);
    void GetBlendColor(float out[4]) const {
        out[0] = blendColor_[0]; out[1] = blendColor_[1];
        out[2] = blendColor_[2]; out[3] = blendColor_[3];
    }

    // --- depth/stencil ---
    DepthState& Depth() { dirtyDepth_ = true; return depth_; }
    const DepthState& Depth() const { return depth_; }
    StencilFace& StencilFront() { dirtyDepth_ = true; return stenFront_; }
    StencilFace& StencilBack() { dirtyDepth_ = true; return stenBack_; }
    const StencilFace& StencilFront() const { return stenFront_; }
    const StencilFace& StencilBack() const { return stenBack_; }
    void SetDepthRange(double n, double f, GLuint idx = 0);

    // --- raster ---
    void SetCullFace(GLenum mode);
    void SetFrontFace(GLenum mode);
    GLenum CullMode() const { return cullMode_; }
    GLenum FrontFace() const { return frontFace_; }
    void SetPolygonOffset(float factor, float units, float clamp);
    void SetLineWidth(float w);
    void SetViewport(int idx, float x, float y, float w, float h);
    ViewportState GetViewport(int idx) const {
        return (idx >= 0 && idx < kMaxViewports) ? viewports_[idx] : ViewportState{};
    }
    void SetDepthRangef(float n, float f, int idx);
    void SetClipControl(GLenum origin, GLenum depth);
    GLenum ClipOrigin() const { return clipOrigin_; }
    GLenum ClipDepth() const { return clipDepth_; }
    void SetPolygonMode(GLenum face, GLenum mode);
    GLenum PolygonMode() const { return polyMode_; } // FRONT_AND_BACK dùng chung (M5b)
    // Scissor (glScissor*): Metal setScissorRect — trước đây no-op, giờ lưu thật.
    void SetScissor(int x, int y, int w, int h) { scissor_ = {x, y, w, h}; }
    struct ScissorBox { int x = 0, y = 0, w = 0, h = 0; };
    ScissorBox GetScissor() const { return scissor_; }

    // --- bindings (shadow, Metal encode dùng) ---
    void BindBuffer(GLenum target, GLuint buf);
    GLuint BoundBuffer(GLenum target) const;
    void BindTextureUnit(GLuint unit, GLuint tex, GLenum target = 0);
    void SetActiveTexture(GLuint unit);
    GLuint ActiveTexture() const { return activeTex_; }
    GLuint BoundTexture(GLuint unit) const {
        return unit < (GLuint)kMaxTextureUnits ? texBound_[unit] : 0;
    }
    GLenum BoundTextureTarget(GLuint unit) const {
        return unit < (GLuint)kMaxTextureUnits ? texTarget_[unit] : 0;
    }
    void BindSampler(GLuint unit, GLuint s);
    GLuint BoundSampler(GLuint unit) const {
        return unit < (GLuint)kMaxTextureUnits ? samplerBound_[unit] : 0;
    }
    void BindVAO(GLuint vao);
    GLuint BoundVAO() const { return boundVAO_; }
    void BindFBO(GLenum target, GLuint fbo);
    GLuint BoundDrawFBO() const { return boundDrawFBO_; }
    GLuint BoundReadFBO() const { return boundReadFBO_; }
    void BindProgram(GLuint p);
    GLuint BoundProgram() const { return boundProgram_; }
    // XFB bind hiện tại (glBindTransformFeedback) — thay vì đoán object đầu tiên
    void BindXFB(GLuint id) { boundXFB_ = id; }
    GLuint BoundXFB() const { return boundXFB_; }
    // Số control point / patch (glPatchParameteri GL_PATCH_VERTICES, mặc định 3)
    void SetPatchVertices(GLint n) { patchVertices_ = n; }
    GLint PatchVertices() const { return patchVertices_; }

    PixelStoreState& PixelStore() { return pixel_; }
    const PixelStoreState& PixelStore() const { return pixel_; }

    bool DirtyPipeline() const { return dirtyPipeline_; }
    bool DirtyDepth() const { return dirtyDepth_; }
    void ClearDirty() { dirtyPipeline_ = false; dirtyDepth_ = false; }

    // Shadow cho glGet*: lưu mọi pname đã Set để Get* trả về (Metal không query GPU)
    void SetShadow(GLenum pname, const void* data, size_t bytes, GLuint index = 0);
    bool GetShadow(GLenum pname, void* out, size_t bytes, GLuint index = 0) const;

private:
    std::unordered_map<GLenum, bool> caps_;
    std::unordered_map<GLenum, std::unordered_map<GLuint, bool>> capsI_;
    std::array<BlendState, kMaxDrawBuffers> blend_;
    float blendColor_[4] = {0, 0, 0, 0};
    DepthState depth_;
    StencilFace stenFront_, stenBack_;
    std::array<ViewportState, kMaxViewports> viewports_;
    GLenum cullMode_ = 0x0405;  // BACK
    GLenum frontFace_ = 0x0901; // CCW
    GLenum polyMode_ = 0x1B02;  // FILL (GL_FILL)
    ScissorBox scissor_ = {0, 0, 0, 0};
    float polyFactor_ = 0, polyUnits_ = 0, polyClamp_ = 0;
    float lineWidth_ = 1.0f;
    GLenum clipOrigin_ = 0x8CA1; // LOWER_LEFT (đã đối chiếu gl.xml; trước đây ghi nhầm 0x8CA0)
    GLenum clipDepth_ = 0x935E;  // NEGATIVE_ONE_TO_ONE (đã đối chiếu; trước đây nhầm 0x8CA1)
    std::unordered_map<GLenum, GLuint> bufferBindings_;
    std::array<GLuint, kMaxTextureUnits> texBound_ = {};
    std::array<GLenum, kMaxTextureUnits> texTarget_ = {};
    std::array<GLuint, kMaxTextureUnits> samplerBound_ = {};
    GLuint activeTex_ = 0;
    GLuint boundVAO_ = 0, boundProgram_ = 0, boundXFB_ = 0;
    GLint patchVertices_ = 3; // GL_PATCH_VERTICES mặc định (spec §10.1.3)
    GLuint boundReadFBO_ = 0, boundDrawFBO_ = 0;
    PixelStoreState pixel_;
    bool dirtyPipeline_ = true, dirtyDepth_ = true;
    struct Key { GLenum p; GLuint i; bool operator==(const Key& o) const { return p==o.p&&i==o.i; } };
    struct KeyH { size_t operator()(const Key& k) const { return (size_t)k.p*1315423911u + k.i; } };
    std::unordered_map<Key, std::vector<uint8_t>, KeyH> shadow_;
};

} // namespace tglmt
