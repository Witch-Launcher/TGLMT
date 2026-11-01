// gl_ext_include_shading.cpp — ARB_shading_language_include, NV/EXT fragment shading rate.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>

using namespace tglmt;
extern "C" {
void glNamedStringARB(GLenum type, GLint namelen, const GLchar* name, GLint stringlen, const GLchar* string);
void glDeleteNamedStringARB(GLint namelen, const GLchar* name);
void glCompileShaderIncludeARB(GLuint shader, GLsizei count, const GLchar* const* path, const GLint* length);
GLboolean glIsNamedStringARB(GLint namelen, const GLchar* name);
void glGetNamedStringARB(GLint namelen, const GLchar* name, GLsizei bufSize, GLint* stringlen, GLchar* string);
void glGetNamedStringivARB(GLint namelen, const GLchar* name, GLenum pname, GLint* params);
void glBindShadingRateImageNV(GLuint texture);
void glShadingRateImagePaletteNV(GLuint viewport, GLuint first, GLsizei count, const GLenum* rates);
void glGetShadingRateImagePaletteNV(GLuint viewport, GLuint entry, GLenum* rate);
void glShadingRateImageBarrierNV(GLboolean synchronize);
void glShadingRateSampleOrderNV(GLenum order);
void glShadingRateSampleOrderCustomNV(GLenum rate, GLuint samples, const GLint* locations);
void glGetShadingRateSampleLocationivNV(GLenum rate, GLuint samples, GLuint index, GLint* location);
void glShadingRateEXT(GLenum rate);
void glShadingRateCombinerOpsEXT(GLenum op0, GLenum op1);
void glFramebufferShadingRateEXT(GLenum target, GLenum attachment, GLuint texture,
        GLint baseLayer, GLsizei numLayers, GLsizei texelWidth, GLsizei texelHeight);
void glGetFragmentShadingRatesEXT(GLsizei samples, GLsizei maxCount, GLsizei* count, GLenum* shadingRates);
}

namespace {
static std::string Name(GLint len, const GLchar* name) {
    if (!name) return {};
    if (len < 0) return std::string(name);
    return std::string(name, (size_t)len);
}
static bool ValidName(const std::string& name) {
    return !name.empty() && name[0] == '/';
}
static bool Rate(GLenum r) { return r >= 0x96A6 && r <= 0x96AE; }
static bool Combiner(GLenum r) { return r >= 0x96D2 && r <= 0x96D6; }
static uint64_t PalKey(GLuint viewport, GLuint first) { return ((uint64_t)viewport << 32) | first; }
}

extern "C" {

void glNamedStringARB(GLenum type, GLint namelen, const GLchar* name, GLint stringlen, const GLchar* string) {
    Context& c=Context::Current();
    if(type!=0x8DAE && type!=0x8DAE){c.errors.Record(0x0500);return;} // SHADER_INCLUDE_ARB only
    if(namelen<-1||stringlen<-1||!name||!string){c.errors.Record(0x0501);return;}
    std::string n=Name(namelen,name);
    if(!ValidName(n)){c.errors.Record(0x0501);return;}
    c.ext.namedStrings[n]=Name(stringlen,string);
}
void glDeleteNamedStringARB(GLint namelen, const GLchar* name) {
    Context& c=Context::Current(); if(namelen<-1||!name){c.errors.Record(0x0501);return;}
    std::string n=Name(namelen,name);
    if(!ValidName(n)){c.errors.Record(0x0501);return;}
    if(!c.ext.namedStrings.count(n)){c.errors.Record(0x0502);return;}
    c.ext.namedStrings.erase(n);
}
void glCompileShaderIncludeARB(GLuint shader, GLsizei count, const GLchar* const* path, const GLint* length) {
    (void)count;(void)path;(void)length;
    tglmt::gl::glCompileShader(shader);
}
GLboolean glIsNamedStringARB(GLint namelen, const GLchar* name) {
    Context& c=Context::Current(); if(namelen<-1||!name)return 0;
    std::string n=Name(namelen,name);
    if(!ValidName(n))return 0;
    return c.ext.namedStrings.count(n)?1:0;
}
void glGetNamedStringARB(GLint namelen, const GLchar* name, GLsizei bufSize, GLint* stringlen, GLchar* string) {
    Context& c=Context::Current();
    if(namelen<-1||!name||bufSize<0){c.errors.Record(0x0501);return;}
    std::string n=Name(namelen,name);
    if(!ValidName(n)){c.errors.Record(0x0501);return;}
    auto it=c.ext.namedStrings.find(n);
    if(it==c.ext.namedStrings.end()){c.errors.Record(0x0502);return;}
    GLsizei copy=bufSize>0?(GLsizei)std::min<size_t>(it->second.size(),(size_t)(bufSize-1)):0;
    if(string&&bufSize>0){if(copy>0)std::memcpy(string,it->second.data(),(size_t)copy);string[copy]=0;}
    if(stringlen)*stringlen=(GLint)it->second.size();
}
void glGetNamedStringivARB(GLint namelen, const GLchar* name, GLenum pname, GLint* params) {
    Context& c=Context::Current();
    if(namelen<-1||!name||!params){c.errors.Record(0x0501);return;}
    std::string n=Name(namelen,name);
    if(!ValidName(n)){c.errors.Record(0x0501);return;}
    auto it=c.ext.namedStrings.find(n);
    if(it==c.ext.namedStrings.end()){c.errors.Record(0x0502);return;}
    if(pname==0x8DE9)*params=(GLint)it->second.size()+1;
    else if(pname==0x8DEA)*params=0x8DAE;
    else c.errors.Record(0x0500);
}

void glBindShadingRateImageNV(GLuint texture) {
    Context& c=Context::Current();
    if(texture&&!c.textures.count(texture)){c.errors.Record(0x0502);return;}
    c.ext.shadingRateImage=texture;
}
void glShadingRateImagePaletteNV(GLuint viewport, GLuint first, GLsizei count, const GLenum* rates) {
    Context& c=Context::Current();
    if(viewport>=16||first>=16||count<0||first+(GLuint)count>16||(count&&!rates)){c.errors.Record(0x0501);return;}
    std::vector<GLenum> v((size_t)count);
    for(GLsizei i=0;i<count;++i)v[(size_t)i]=rates[i];
    c.ext.shadingRatePalettes[PalKey(viewport,first)]=std::move(v);
}
void glGetShadingRateImagePaletteNV(GLuint viewport, GLuint entry, GLenum* rate) {
    Context& c=Context::Current(); if(!rate||viewport>=16||entry>=16){c.errors.Record(0x0501);return;}
    auto it=c.ext.shadingRatePalettes.find(PalKey(viewport,entry));
    *rate=it==c.ext.shadingRatePalettes.end()?0:it->second[0];
}
void glShadingRateImageBarrierNV(GLboolean synchronize) { (void)synchronize; }
void glShadingRateSampleOrderNV(GLenum order) {
    Context& c=Context::Current();
    if(order!=0x95AE&&order!=0x95AF&&order!=0x95B0){c.errors.Record(0x0500);return;}
    c.ext.shadingRateSampleOrder=order;
}
void glShadingRateSampleOrderCustomNV(GLenum rate, GLuint samples, const GLint* locations) {
    Context& c=Context::Current();
    if(!Rate(rate)||samples==0||samples>32||!locations){c.errors.Record(0x0501);return;}
    c.ext.shadingRateSampleOrder=rate;
}
void glGetShadingRateSampleLocationivNV(GLenum rate, GLuint samples, GLuint index, GLint* location) {
    Context& c=Context::Current();
    if(!Rate(rate)||samples==0||index>=samples||!location){c.errors.Record(0x0501);return;}
    location[0]=0;location[1]=0;
}
void glShadingRateEXT(GLenum rate) {
    Context& c=Context::Current();
    if(!Rate(rate)){c.errors.Record(0x0500);return;}
    c.ext.shadingRate=rate;
}
void glShadingRateCombinerOpsEXT(GLenum op0, GLenum op1) {
    Context& c=Context::Current();
    if(!Combiner(op0)||!Combiner(op1)){c.errors.Record(0x0500);return;}
    c.ext.shadingRateCombiner[0]=op0;c.ext.shadingRateCombiner[1]=op1;
}
void glFramebufferShadingRateEXT(GLenum target, GLenum attachment, GLuint texture,
        GLint baseLayer, GLsizei numLayers, GLsizei texelWidth, GLsizei texelHeight) {
    (void)texelWidth;(void)texelHeight;
    if(numLayers<1){Context::Current().errors.Record(0x0501);return;}
    tglmt::gl::glFramebufferTextureLayer(target,attachment,texture,0,baseLayer);
}
void glGetFragmentShadingRatesEXT(GLsizei samples, GLsizei maxCount, GLsizei* count, GLenum* shadingRates) {
    Context& c=Context::Current();
    if(samples<0||maxCount<0||(maxCount&&!shadingRates)){c.errors.Record(0x0501);return;}
    static const GLenum rates[]={0x96A6,0x96A7,0x96A8,0x96A9,0x96AA,0x96AB,0x96AC,0x96AD,0x96AE};
    GLsizei n=std::min<GLsizei>(maxCount,(GLsizei)(sizeof(rates)/sizeof(rates[0])));
    for(GLsizei i=0;i<n;++i)shadingRates[i]=rates[i];
    if(count)*count=n;
}

} // extern "C"
