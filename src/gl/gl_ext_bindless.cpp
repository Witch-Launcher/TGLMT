// gl_ext_bindless.cpp — ARB/NV bindless texture + shader buffer load + unified memory.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;
extern "C" {
GLuint64 glGetTextureHandleARB(GLuint texture);
GLuint64 glGetTextureHandleNV(GLuint texture);
GLuint64 glGetTextureSamplerHandleARB(GLuint texture, GLuint sampler);
GLuint64 glGetImageHandleARB(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
GLboolean glIsTextureHandleResidentARB(GLuint64 handle);
GLboolean glIsImageHandleResidentARB(GLuint64 handle);
void glMakeTextureHandleResidentARB(GLuint64 handle);
void glMakeTextureHandleNonResidentARB(GLuint64 handle);
void glMakeImageHandleResidentARB(GLuint64 handle, GLenum access);
void glMakeImageHandleNonResidentARB(GLuint64 handle);
void glGetVertexAttribLui64vARB(GLuint index, GLenum pname, GLuint64* params);
void glUniformHandleui64ARB(GLint location, GLuint64 value);
void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64* value);
void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value);
void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64* values);
void glVertexAttribL1ui64ARB(GLuint index, GLuint64 x);
void glVertexAttribL1ui64vARB(GLuint index, const GLuint64* v);
void glVertexAttribL1ui64NV(GLuint index, GLuint64 x);
void glGetBufferParameterui64vNV(GLenum target, GLenum pname, GLuint64* params);
void glGetNamedBufferParameterui64vNV(GLuint buffer, GLenum pname, GLuint64* params);
void glGetIntegerui64vNV(GLenum value, GLuint64* result);
void glMakeBufferResidentNV(GLenum target, GLenum access);
void glMakeBufferNonResidentNV(GLenum target);
void glMakeNamedBufferResidentNV(GLuint buffer, GLenum access);
void glMakeNamedBufferNonResidentNV(GLuint buffer);
GLboolean glIsBufferResidentNV(GLenum target);
GLboolean glIsNamedBufferResidentNV(GLuint buffer);
void glUniformui64NV(GLint location, GLuint64 value);
void glUniformui64vNV(GLint location, GLsizei count, const GLuint64* value);
void glProgramUniformui64NV(GLuint program, GLint location, GLuint64 value);
void glProgramUniformui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64* value);
void glBufferAddressRangeNV(GLenum pname, GLuint index, GLuint64 address, GLsizeiptr length);
void glGetIntegerui64i_vNV(GLenum value, GLuint index, GLuint64* result);
void glVertexAttribFormatNV(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);
void glVertexAttribIFormatNV(GLuint index, GLint size, GLenum type, GLsizei stride);
void glVertexFormatNV(GLint size, GLenum type, GLsizei stride);
void glNormalFormatNV(GLenum type, GLsizei stride);
void glColorFormatNV(GLint size, GLenum type, GLsizei stride);
void glSecondaryColorFormatNV(GLint size, GLenum type, GLsizei stride);
void glFogCoordFormatNV(GLenum type, GLsizei stride);
void glIndexFormatNV(GLenum type, GLsizei stride);
void glEdgeFlagFormatNV(GLsizei stride);
void glTexCoordFormatNV(GLint size, GLenum type, GLsizei stride);
}

namespace {
static GLuint64 NewHandle(Context& c, GLuint tex) {
    GLuint64 h = c.ext.nextBindlessHandle++;
    if (h == 0) h = c.ext.nextBindlessHandle++;
    c.ext.bindlessHandles[h] = tex;
    c.ext.bindlessResident[h] = false;
    return h;
}
static bool HasHandle(Context& c, GLuint64 h) {
    return c.ext.bindlessHandles.count(h) != 0;
}
static ProgramObject* Prog(GLuint p) {
    Context& c=Context::Current(); auto it=c.programs.find(p);
    if(it==c.programs.end()){c.errors.Record(0x0502);return nullptr;} return &it->second;
}
static void PutU64(ProgramObject* p, GLint loc, const GLuint64* v, GLsizei n) {
    if(!p||!v||n<0)return;
    p->uniforms[loc].assign((const uint8_t*)v,(const uint8_t*)v+(size_t)n*8);
}
} // namespace

extern "C" {

GLuint64 glGetTextureHandleARB(GLuint texture) {
    Context& c=Context::Current(); if(!c.textures.count(texture)){c.errors.Record(0x0502);return 0;}
    return NewHandle(c,texture);
}
GLuint64 glGetTextureHandleNV(GLuint texture) { return glGetTextureHandleARB(texture); }
GLuint64 glGetTextureSamplerHandleARB(GLuint texture, GLuint sampler) {
    Context& c=Context::Current();
    if(!c.textures.count(texture)||(sampler&&!c.samplers.count(sampler))){c.errors.Record(0x0502);return 0;}
    return NewHandle(c,texture);
}
GLuint64 glGetTextureSamplerHandleNV(GLuint texture, GLuint sampler) { return glGetTextureSamplerHandleARB(texture,sampler); }
GLuint64 glGetImageHandleARB(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) {
    (void)level;(void)layered;(void)layer;(void)format;
    return glGetTextureHandleARB(texture);
}
GLuint64 glGetImageHandleNV(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) {
    return glGetImageHandleARB(texture,level,layered,layer,format);
}
GLboolean glIsTextureHandleResidentARB(GLuint64 handle) {
    Context& c=Context::Current(); auto it=c.ext.bindlessResident.find(handle);
    return it!=c.ext.bindlessResident.end()&&it->second;
}
GLboolean glIsImageHandleResidentARB(GLuint64 handle) { return glIsTextureHandleResidentARB(handle); }
GLboolean glIsTextureHandleResidentNV(GLuint64 handle) { return glIsTextureHandleResidentARB(handle); }
GLboolean glIsImageHandleResidentNV(GLuint64 handle) { return glIsImageHandleResidentARB(handle); }
void glMakeTextureHandleResidentARB(GLuint64 handle) {
    Context& c=Context::Current(); if(!HasHandle(c,handle)){c.errors.Record(0x0502);return;}
    c.ext.bindlessResident[handle]=true;
}
void glMakeTextureHandleNonResidentARB(GLuint64 handle) {
    Context& c=Context::Current(); if(!HasHandle(c,handle)){c.errors.Record(0x0502);return;}
    c.ext.bindlessResident[handle]=false;
}
void glMakeImageHandleResidentARB(GLuint64 handle, GLenum access) {
    (void)access; glMakeTextureHandleResidentARB(handle);
}
void glMakeImageHandleNonResidentARB(GLuint64 handle) { glMakeTextureHandleNonResidentARB(handle); }
void glMakeTextureHandleResidentNV(GLuint64 handle) { glMakeTextureHandleResidentARB(handle); }
void glMakeTextureHandleNonResidentNV(GLuint64 handle) { glMakeTextureHandleNonResidentARB(handle); }
void glMakeImageHandleResidentNV(GLuint64 handle, GLenum access) { glMakeImageHandleResidentARB(handle,access); }
void glMakeImageHandleNonResidentNV(GLuint64 handle) { glMakeImageHandleNonResidentARB(handle); }

void glGetVertexAttribLui64vARB(GLuint index, GLenum pname, GLuint64* params) {
    Context& c=Context::Current(); if(!params||index>=16){c.errors.Record(0x0501);return;}
    if(pname==0x8626) {
        auto it=c.attrib64.find(index);
        if(it!=c.attrib64.end()) { for(int i=0;i<4;++i)params[i]=(GLuint64)it->second[i]; }
        else { for(int i=0;i<3;++i)params[i]=0; params[3]=1; }
    } else {
        GLint v=0; tglmt::gl::glGetVertexAttribiv(index,pname,&v); params[0]=(GLuint64)v;
    }
}

void glUniformHandleui64ARB(GLint location, GLuint64 value) {
    auto* p=Prog(Context::Current().state.BoundProgram()); if(p)PutU64(p,location,&value,1);
}
void glUniformHandleui64vARB(GLint location, GLsizei count, const GLuint64* value) {
    PutU64(Prog(Context::Current().state.BoundProgram()),location,value,count);
}
void glUniformHandleui64NV(GLint location, GLuint64 value) { glUniformHandleui64ARB(location,value); }
void glUniformHandleui64vNV(GLint location, GLsizei count, const GLuint64* value) { glUniformHandleui64vARB(location,count,value); }
void glProgramUniformHandleui64ARB(GLuint program, GLint location, GLuint64 value) {
    PutU64(Prog(program),location,&value,1);
}
void glProgramUniformHandleui64vARB(GLuint program, GLint location, GLsizei count, const GLuint64* values) {
    PutU64(Prog(program),location,values,count);
}
void glProgramUniformHandleui64NV(GLuint program, GLint location, GLuint64 value) { glProgramUniformHandleui64ARB(program,location,value); }
void glProgramUniformHandleui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64* values) { glProgramUniformHandleui64vARB(program,location,count,values); }
void glVertexAttribL1ui64ARB(GLuint index, GLuint64 x) { glVertexAttribL1ui64NV(index,x); }
void glVertexAttribL1ui64vARB(GLuint index, const GLuint64* v) { if(v)glVertexAttribL1ui64NV(index,v[0]); }

void glGetBufferParameterui64vNV(GLenum target, GLenum pname, GLuint64* params) {
    Context& c=Context::Current();
    GLuint b=c.state.BoundBuffer(target);
    if(b==0||!c.buffers.count(b)){c.errors.Record(0x0502);return;}
    if(pname!=0x8F1D){c.errors.Record(0x0500);return;}
    if(params)*params=c.ext.bufferAddresses.count(b)?c.ext.bufferAddresses[b]:0;
}
void glGetNamedBufferParameterui64vNV(GLuint buffer, GLenum pname, GLuint64* params) {
    Context& c=Context::Current();
    if(!c.buffers.count(buffer)){c.errors.Record(0x0502);return;}
    if(pname!=0x8F1D){c.errors.Record(0x0500);return;}
    if(params)*params=c.ext.bufferAddresses.count(buffer)?c.ext.bufferAddresses[buffer]:0;
}
void glGetIntegerui64vNV(GLenum value, GLuint64* result) {
    Context& c=Context::Current(); if(!result){c.errors.Record(0x0501);return;}
    if(value==0x8F1D) { *result=0; return; }
    *result=0;
}
static uint64_t FakeAddress(Context& c, GLuint buf) {
    auto it=c.ext.bufferAddresses.find(buf);
    if(it!=c.ext.bufferAddresses.end()) return it->second;
    uint64_t a=0x100000000ULL + (uint64_t)buf*0x10000ULL;
    c.ext.bufferAddresses[buf]=a; return a;
}
void glMakeBufferResidentNV(GLenum target, GLenum access) {
    Context& c=Context::Current(); (void)access;
    GLuint b=c.state.BoundBuffer(target);
    if(!b||!c.buffers.count(b)){c.errors.Record(0x0502);return;}
    FakeAddress(c,b);
}
void glMakeBufferNonResidentNV(GLenum target) {
    Context& c=Context::Current(); GLuint b=c.state.BoundBuffer(target);
    if(b) c.ext.bufferAddresses.erase(b);
}
void glMakeNamedBufferResidentNV(GLuint buffer, GLenum access) {
    Context& c=Context::Current(); (void)access;
    if(!c.buffers.count(buffer)){c.errors.Record(0x0502);return;}
    FakeAddress(c,buffer);
}
void glMakeNamedBufferNonResidentNV(GLuint buffer) { Context::Current().ext.bufferAddresses.erase(buffer); }
GLboolean glIsBufferResidentNV(GLenum target) {
    Context& c=Context::Current(); return c.ext.bufferAddresses.count(c.state.BoundBuffer(target))?1:0;
}
GLboolean glIsNamedBufferResidentNV(GLuint buffer) {
    return Context::Current().ext.bufferAddresses.count(buffer)?1:0;
}
void glUniformui64NV(GLint location, GLuint64 value) {
    auto* p=Prog(Context::Current().state.BoundProgram()); PutU64(p,location,&value,1);
}
void glUniformui64vNV(GLint location, GLsizei count, const GLuint64* value) {
    PutU64(Prog(Context::Current().state.BoundProgram()),location,value,count);
}
void glProgramUniformui64NV(GLuint program, GLint location, GLuint64 value) { PutU64(Prog(program),location,&value,1); }
void glProgramUniformui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64* value) { PutU64(Prog(program),location,value,count); }

void glBufferAddressRangeNV(GLenum pname, GLuint index, GLuint64 address, GLsizeiptr length) {
    Context& c=Context::Current(); (void)length;
    c.ext.indexedAddresses[pname][index]=address;
}
void glGetIntegerui64i_vNV(GLenum value, GLuint index, GLuint64* result) {
    Context& c=Context::Current(); if(!result){c.errors.Record(0x0501);return;}
    auto it=c.ext.indexedAddresses.find(value);
    *result=(it!=c.ext.indexedAddresses.end()&&it->second.count(index))?it->second[index]:0;
}
static void SetNVFormat(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride) {
    Context& c=Context::Current();
    if(index>=16||size<0||stride<0){c.errors.Record(0x0501);return;}
    VertexArrayObject* v=nullptr; auto it=c.vaos.find(c.state.BoundVAO());
    if(it!=c.vaos.end())v=&it->second; if(!v){c.errors.Record(0x0502);return;}
    auto& a=v->attribs[index]; a.size=size; a.type=type; a.normalized=normalized;
    a.stride=stride; a.isInt=false;
}
void glVertexAttribFormatNV(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride) {
    SetNVFormat(index,size,type,normalized,stride);
}
void glVertexAttribIFormatNV(GLuint index, GLint size, GLenum type, GLsizei stride) {
    SetNVFormat(index,size,type,0,stride); Context& c=Context::Current();
    auto it=c.vaos.find(c.state.BoundVAO()); if(it!=c.vaos.end()&&index<16)it->second.attribs[index].isInt=true;
}
void glVertexFormatNV(GLint size, GLenum type, GLsizei stride) { SetNVFormat(0,size,type,0,stride); }
void glNormalFormatNV(GLenum type, GLsizei stride) { SetNVFormat(2,3,type,0,stride); }
void glColorFormatNV(GLint size, GLenum type, GLsizei stride) { SetNVFormat(3,size,type,0,stride); }
void glSecondaryColorFormatNV(GLint size, GLenum type, GLsizei stride) { SetNVFormat(4,size,type,0,stride); }
void glFogCoordFormatNV(GLenum type, GLsizei stride) { SetNVFormat(5,1,type,0,stride); }
void glIndexFormatNV(GLenum type, GLsizei stride) { SetNVFormat(1,1,type,0,stride); }
void glEdgeFlagFormatNV(GLsizei stride) { SetNVFormat(1,1,0x1401,0,stride); }
void glTexCoordFormatNV(GLint size, GLenum type, GLsizei stride) { SetNVFormat(8,size,type,0,stride); }

} // extern "C"
