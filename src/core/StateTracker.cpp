#include "tglmt/StateTracker.h"
namespace tglmt {
StateTracker::StateTracker() {
    viewports_[0] = {0, 0, 0, 0, 0, 1};
}
void StateTracker::SetEnabled(GLenum cap, bool on, GLuint index) {
    if (index == 0) caps_[cap] = on;
    else capsI_[cap][index] = on;
    // Mọi cap raster đều bake vào pipeline/depth → dirty
    dirtyPipeline_ = true; dirtyDepth_ = true;
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
    blendColor_[0]=r; blendColor_[1]=g; blendColor_[2]=b; blendColor_[3]=a;
    dirtyPipeline_ = true;
    uint8_t buf[16]; float v[4]={r,g,b,a}; memcpy(buf,v,16);
    SetShadow(0x8003 /*GL_BLEND_COLOR*/, buf, 16); // shadow cho glGet
}
void StateTracker::SetDepthRange(double n, double f, GLuint idx) {
    if (idx < kMaxViewports) { viewports_[idx].n = (float)n; viewports_[idx].f = (float)f; }
    uint8_t buf[16]; double v[2]={n,f}; memcpy(buf,v,16);
    SetShadow(0x0B70 /*GL_DEPTH_RANGE*/, buf, 16, idx);
}
void StateTracker::SetCullFace(GLenum m){ cullMode_=m; dirtyPipeline_=true; SetShadow(0x0B45,&m,4); }
void StateTracker::SetFrontFace(GLenum m){ frontFace_=m; dirtyPipeline_=true; SetShadow(0x0B46,&m,4); }
void StateTracker::SetPolygonOffset(float f, float u, float c){ polyFactor_=f; polyUnits_=u; polyClamp_=c; dirtyPipeline_=true; }
void StateTracker::SetLineWidth(float w){ lineWidth_=w; }
void StateTracker::SetViewport(int i, float x, float y, float w, float h){
    if(i>=0&&i<kMaxViewports){viewports_[i].x=x;viewports_[i].y=y;viewports_[i].w=w;viewports_[i].h=h;}
}
void StateTracker::SetDepthRangef(float n, float f, int i){
    if(i>=0&&i<kMaxViewports){viewports_[i].n=n;viewports_[i].f=f;}
}
void StateTracker::SetClipControl(GLenum o, GLenum d){ clipOrigin_=o; clipDepth_=d; dirtyPipeline_=true; }
void StateTracker::SetPolygonMode(GLenum face, GLenum mode){
    // Metal: FILL trực tiếp; LINE → setTriangleFillModeLines (M5b); POINT → expand ở M5b.
    if (face == 0x0404 /*FRONT_AND_BACK*/ || face == 0x0408 /*FRONT*/) polyMode_ = mode;
    dirtyPipeline_=true;
}
void StateTracker::BindBuffer(GLenum t, GLuint b){ bufferBindings_[t]=b; }
GLuint StateTracker::BoundBuffer(GLenum t) const {
    auto it=bufferBindings_.find(t); return it==bufferBindings_.end()?0:it->second;
}
void StateTracker::BindTextureUnit(GLuint u, GLuint t, GLenum tgt){
    if(u<kMaxTextureUnits){texBound_[u]=t; texTarget_[u]=tgt;}
}
void StateTracker::SetActiveTexture(GLuint u){ activeTex_=u; }
void StateTracker::BindSampler(GLuint u, GLuint s){ if(u<kMaxTextureUnits) samplerBound_[u]=s; }
void StateTracker::BindVAO(GLuint v){ boundVAO_=v; }
void StateTracker::BindFBO(GLenum t, GLuint f){
    if(t==0x8CA8) boundReadFBO_=f; // GL_READ_FRAMEBUFFER
    else if(t==0x8CA9) boundDrawFBO_=f; // GL_DRAW_FRAMEBUFFER
    else { boundReadFBO_=boundDrawFBO_=f; } // GL_FRAMEBUFFER
}
void StateTracker::BindProgram(GLuint p){ boundProgram_=p; }
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
