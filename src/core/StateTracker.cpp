#include "tglmt/StateTracker.h"
namespace tglmt {
StateTracker::StateTracker() {
    viewports_[0] = {0, 0, 0, 0, 0, 1};
}
void StateTracker::SetEnabled(GLenum cap, bool on, GLuint index) {
    if (index == 0) {
        auto it = caps_.find(cap);
        if (it != caps_.end() && it->second == on) return; // IR shadowing: 0 Metal khi không đổi
        caps_[cap] = on;
    } else {
        auto& m = capsI_[cap];
        auto it = m.find(index);
        if (it != m.end() && it->second == on) return;
        m[index] = on;
    }
    // Mọi cap raster đều bake vào pipeline/depth → dirty
    dirtyPipeline_ = true; dirtyDepth_ = true;
    ++stateSeq_;
}
bool StateTracker::IsEnabled(GLenum cap, GLuint index) const {
    if (index == 0) {
        auto it = caps_.find(cap);
        return it != caps_.end() && it->second;
    }
    auto it = capsI_.find(cap);
    if (it == capsI_.end()) return false;
    auto j = it->second.find(index);
    return j != it->second.end() && j->second;
}
void StateTracker::SetBlendColor(float r, float g, float b, float a) {
    if (blendColor_[0]==r && blendColor_[1]==g && blendColor_[2]==b && blendColor_[3]==a)
        return; // shadowing: glBlendColor trùng → 0 Metal
    blendColor_[0]=r; blendColor_[1]=g; blendColor_[2]=b; blendColor_[3]=a;
    dirtyPipeline_ = true;
    ++stateSeq_;
    uint8_t buf[16]; float v[4]={r,g,b,a}; memcpy(buf,v,16);
    SetShadow(0x8003 /*GL_BLEND_COLOR*/, buf, 16); // shadow cho glGet
}
void StateTracker::SetDepthRange(double n, double f, GLuint idx) {
    if (idx < kMaxViewports) {
        auto& v = viewports_[idx];
        if (v.n==(float)n && v.f==(float)f) {
            // vẫn cập nhật shadow cho glGet nhưng không tăng seq (không đổi Metal state)
            uint8_t buf[16]; double vv[2]={n,f}; memcpy(buf,vv,16);
            SetShadow(0x0B70 /*GL_DEPTH_RANGE*/, buf, 16, idx);
            return;
        }
        v.n = (float)n; v.f = (float)f; ++stateSeq_;
    }
    uint8_t buf[16]; double v[2]={n,f}; memcpy(buf,v,16);
    SetShadow(0x0B70 /*GL_DEPTH_RANGE*/, buf, 16, idx);
}
// SetLineWidth/SetViewport/SetDepthRangef đã inline trong header (shadowing).
void StateTracker::SetCullFace(GLenum m){ if (cullMode_==m) return; cullMode_=m; dirtyPipeline_=true; ++stateSeq_; SetShadow(0x0B45,&m,4); }
void StateTracker::SetFrontFace(GLenum m){ if (frontFace_==m) return; frontFace_=m; dirtyPipeline_=true; ++stateSeq_; SetShadow(0x0B46,&m,4); }
void StateTracker::SetPolygonOffset(float f, float u, float c){ if (polyFactor_==f&&polyUnits_==u&&polyClamp_==c) return; polyFactor_=f; polyUnits_=u; polyClamp_=c; dirtyPipeline_=true; ++stateSeq_; }
// SetLineWidth/SetViewport/SetDepthRangef đã inline trong header (shadowing).
void StateTracker::SetClipControl(GLenum o, GLenum d){ if (clipOrigin_==o&&clipDepth_==d) return; clipOrigin_=o; clipDepth_=d; dirtyPipeline_=true; ++stateSeq_; }
void StateTracker::SetPolygonMode(GLenum face, GLenum mode){
    // Metal: FILL trực tiếp; LINE → setTriangleFillModeLines (M5b); POINT → expand ở M5b.
    GLenum want = polyMode_;
    if (face == 0x0404 /*FRONT_AND_BACK*/ || face == 0x0408 /*FRONT*/) want = mode;
    if (want == polyMode_) return;
    polyMode_ = want;
    dirtyPipeline_=true;
    ++stateSeq_;
}
void StateTracker::BindBuffer(GLenum t, GLuint b){
    auto it = bufferBindings_.find(t);
    if (it != bufferBindings_.end() && it->second == b) return; // shadowing
    bufferBindings_[t]=b;
    ++stateSeq_;
}
GLuint StateTracker::BoundBuffer(GLenum t) const {
    auto it=bufferBindings_.find(t); return it==bufferBindings_.end()?0:it->second;
}
void StateTracker::BindTextureUnit(GLuint u, GLuint t, GLenum tgt){
    if(u<kMaxTextureUnits){
        if (texBound_[u]==t && (t==0 || texTarget_[u]==tgt || tgt==0)) {
            // tgt==0 (DSA bind không target): giữ target cũ, coi như không đổi nếu id trùng
            if (t==0 || tgt==0 || texTarget_[u]==tgt) return;
        }
        texBound_[u]=t;
        if (tgt) texTarget_[u]=tgt;
        else if (t==0) texTarget_[u]=0;
        ++stateSeq_;
    }
}
void StateTracker::SetActiveTexture(GLuint u){ if (activeTex_==u) return; activeTex_=u; ++stateSeq_; }
void StateTracker::BindSampler(GLuint u, GLuint s){ if(u<kMaxTextureUnits) { if (samplerBound_[u]==s) return; samplerBound_[u]=s; ++stateSeq_; } }
void StateTracker::BindVAO(GLuint v){ if (boundVAO_==v) return; boundVAO_=v; ++stateSeq_; }
void StateTracker::BindFBO(GLenum t, GLuint f){
    GLuint oldR = boundReadFBO_, oldD = boundDrawFBO_;
    if(t==0x8CA8) boundReadFBO_=f; // GL_READ_FRAMEBUFFER
    else if(t==0x8CA9) boundDrawFBO_=f; // GL_DRAW_FRAMEBUFFER
    else { boundReadFBO_=boundDrawFBO_=f; } // GL_FRAMEBUFFER
    if (oldR!=boundReadFBO_ || oldD!=boundDrawFBO_) ++stateSeq_;
}
void StateTracker::BindProgram(GLuint p){ if (boundProgram_==p) return; boundProgram_=p; ++stateSeq_; }
void StateTracker::SetShadow(GLenum p, const void* d, size_t n, GLuint i){
    std::vector<uint8_t> v((const uint8_t*)d,(const uint8_t*)d+n);
    shadow_[{p,i}]=std::move(v);
}
bool StateTracker::GetShadow(GLenum p, void* o, size_t n, GLuint i) const {
    auto it=shadow_.find({p,i});
    if(it==shadow_.end()||it->second.size()<n) return false;
    memcpy(o,it->second.data(),n);
    return true;
}
} // namespace tglmt
