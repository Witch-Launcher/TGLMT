// gl_ext_dsa_state.cpp — EXT_direct_state_access legacy/state entries.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>
#include <cmath>

using namespace tglmt;

namespace {
static bool ValidUnit(GLenum texunit, GLuint& u) {
    if (texunit < 0x84C0 || texunit >= 0x84C0 + 32) {
        Context::Current().errors.Record(0x0500); return false;
    }
    u = texunit - 0x84C0; return true;
}
static uint64_t Key(GLuint unit, GLenum pname) { return ((uint64_t)unit << 32) | pname; }
static VertexArrayObject* VAO(Context& c, GLuint id) {
    auto it = c.vaos.find(id);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
static void SetPtr(GLuint va, GLuint buffer, GLuint attrib, GLint size, GLenum type,
                   GLboolean normalized, GLsizei stride, GLintptr offset) {
    Context& c = Context::Current();
    VertexArrayObject* v = VAO(c, va);
    if (!v) return;
    if (attrib >= 16 || size < 0 || stride < 0 || offset < 0) { c.errors.Record(0x0501); return; }
    auto& a = v->attribs[attrib];
    a.buffer = buffer; a.size = size; a.type = type; a.normalized = normalized;
    a.stride = stride; a.binding = 0; a.relativeOffset = 0; a.offset = (size_t)offset;
    a.isInt = false; a.isLong = false;
    v->bindings[0].buffer = buffer; v->bindings[0].offset = offset; v->bindings[0].stride = stride;
    VAOSyncAttribOffset(*v, attrib);
}
static GLuint AttribForArray(GLenum array, GLuint index) {
    switch (array) {
        case 0x8074: return 0; // VERTEX_ARRAY
        case 0x8075: return 2; // NORMAL_ARRAY
        case 0x8076: return 3; // COLOR_ARRAY
        case 0x8077: return 1; // INDEX_ARRAY
        case 0x8078: return 8 + index; // TEXTURE_COORD_ARRAY
        case 0x8079: return 1; // EDGE_FLAG_ARRAY
        case 0x845E: return 4; // SECONDARY_COLOR_ARRAY
        case 0x8457: return 5; // FOG_COORD_ARRAY
        default:
            if (array >= 0x8500 && array < 0x8510) return array - 0x8500;
            return 0xFFFFFFFFu;
    }
}
static std::array<GLdouble,16> Identity() {
    std::array<GLdouble,16> m{};
    m[0]=m[5]=m[10]=m[15]=1;
    return m;
}
static std::array<GLdouble,16>* Matrix(Context& c, GLenum mode) {
    if (mode != 0x1700 && mode != 0x1701 && mode != 0x1702 && mode != 0x1800) {
        c.errors.Record(0x0500); return nullptr;
    }
    auto& st = c.ext.matrixStacks[mode];
    if (st.empty()) st.push_back(Identity());
    return &st.back();
}
static void Mul(std::array<GLdouble,16>& a, const std::array<GLdouble,16>& b) {
    std::array<GLdouble,16> r{};
    for (int col=0; col<4; ++col)
        for (int row=0; row<4; ++row) {
            double s=0;
            for (int k=0; k<4; ++k) s += a[k*4+row]*b[col*4+k];
            r[col*4+row]=s;
        }
    a=r;
}
static void Load(Context& c, GLenum mode, const GLdouble* src, bool transpose) {
    auto* m=Matrix(c,mode); if(!m||!src) return;
    if (!transpose) std::memcpy(m->data(),src,16*sizeof(double));
    else for(int col=0;col<4;++col) for(int row=0;row<4;++row) (*m)[col*4+row]=src[row*4+col];
}
} // namespace

extern "C" {

void glFramebufferDrawBufferEXT(GLuint framebuffer, GLenum mode) {
    tglmt::gl::glNamedFramebufferDrawBuffer(framebuffer, mode);
}
void glFramebufferDrawBuffersEXT(GLuint framebuffer, GLsizei n, const GLenum* bufs) {
    tglmt::gl::glNamedFramebufferDrawBuffers(framebuffer, n, bufs);
}
void glFramebufferReadBufferEXT(GLuint framebuffer, GLenum mode) {
    tglmt::gl::glNamedFramebufferReadBuffer(framebuffer, mode);
}
void glGetFramebufferParameterivEXT(GLuint framebuffer, GLenum pname, GLint* params) {
    tglmt::gl::glGetNamedFramebufferParameteriv(framebuffer, pname, params);
}
void glNamedCopyBufferSubDataEXT(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset,
        GLintptr writeOffset, GLsizeiptr size) {
    tglmt::gl::glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
}
void glNamedFramebufferTexture1DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level) {
    (void)textarget; tglmt::gl::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTexture2DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level) {
    (void)textarget; tglmt::gl::glNamedFramebufferTexture(framebuffer, attachment, texture, level);
}
void glNamedFramebufferTexture3DEXT(GLuint framebuffer, GLenum attachment, GLenum textarget,
        GLuint texture, GLint level, GLint zoffset) {
    (void)textarget; tglmt::gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, zoffset);
}
void glNamedFramebufferTextureFaceEXT(GLuint framebuffer, GLenum attachment, GLuint texture,
        GLint level, GLenum face) {
    tglmt::gl::glNamedFramebufferTextureLayer(framebuffer, attachment, texture, level, (GLint)face);
}
void glNamedRenderbufferStorageMultisampleCoverageEXT(GLuint renderbuffer, GLsizei coverageSamples,
        GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height) {
    (void)coverageSamples;
    tglmt::gl::glNamedRenderbufferStorageMultisample(renderbuffer, colorSamples, internalformat, width, height);
}

void glEnableVertexArrayEXT(GLuint vaobj, GLenum array) {
    GLuint idx=AttribForArray(array,0);
    if(idx==0xFFFFFFFFu){Context::Current().errors.Record(0x0500);return;}
    tglmt::gl::glEnableVertexArrayAttrib(vaobj,idx);
}
void glDisableVertexArrayEXT(GLuint vaobj, GLenum array) {
    GLuint idx=AttribForArray(array,0);
    if(idx==0xFFFFFFFFu){Context::Current().errors.Record(0x0500);return;}
    tglmt::gl::glDisableVertexArrayAttrib(vaobj,idx);
}
void glVertexArrayBindVertexBufferEXT(GLuint vaobj, GLuint bindingindex, GLuint buffer,
        GLintptr offset, GLsizei stride) {
    tglmt::gl::glVertexArrayVertexBuffer(vaobj,bindingindex,buffer,offset,stride);
}
void glVertexArrayVertexAttribBindingEXT(GLuint vaobj, GLuint attribindex, GLuint bindingindex) {
    tglmt::gl::glVertexArrayAttribBinding(vaobj,attribindex,bindingindex);
}
void glVertexArrayVertexAttribDivisorEXT(GLuint vaobj, GLuint index, GLuint divisor) {
    tglmt::gl::glVertexArrayBindingDivisor(vaobj,index,divisor);
}
void glVertexArrayVertexBindingDivisorEXT(GLuint vaobj, GLuint bindingindex, GLuint divisor) {
    tglmt::gl::glVertexArrayBindingDivisor(vaobj,bindingindex,divisor);
}
void glVertexArrayVertexAttribFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLboolean normalized, GLuint relativeoffset) {
    tglmt::gl::glVertexArrayAttribFormat(vaobj,attribindex,size,type,normalized,relativeoffset);
}
void glVertexArrayVertexAttribIFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLuint relativeoffset) {
    tglmt::gl::glVertexArrayAttribIFormat(vaobj,attribindex,size,type,relativeoffset);
}
void glVertexArrayVertexAttribLFormatEXT(GLuint vaobj, GLuint attribindex, GLint size, GLenum type,
        GLuint relativeoffset) {
    tglmt::gl::glVertexArrayAttribLFormat(vaobj,attribindex,size,type,relativeoffset);
}
void glVertexArrayVertexAttribOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,index,size,type,normalized,stride,offset);
}
void glVertexArrayVertexAttribIOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,index,size,type,0,stride,offset);
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(v&&index<16) v->attribs[index].isInt=true;
}
void glVertexArrayVertexAttribLOffsetEXT(GLuint vaobj, GLuint buffer, GLuint index, GLint size,
        GLenum type, GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,index,size,type,0,stride,offset);
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(v&&index<16) v->attribs[index].isLong=true;
}
void glVertexArrayVertexOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,0,size,type,0,stride,offset);
}
void glVertexArrayColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,3,size,type,0,stride,offset);
}
void glVertexArraySecondaryColorOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,4,size,type,0,stride,offset);
}
void glVertexArrayNormalOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset) {
    SetPtr(vaobj,buffer,2,3,type,0,stride,offset);
}
void glVertexArrayFogCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset) {
    SetPtr(vaobj,buffer,5,1,type,0,stride,offset);
}
void glVertexArrayEdgeFlagOffsetEXT(GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,1,1,0x1401,0,stride,offset);
}
void glVertexArrayIndexOffsetEXT(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride,
        GLintptr offset) {
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(!v){return;} if(offset<0){c.errors.Record(0x0501);return;}
    v->elementBuffer=buffer;
    (void)type;(void)stride;(void)offset;
}
void glVertexArrayTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLint size, GLenum type,
        GLsizei stride, GLintptr offset) {
    SetPtr(vaobj,buffer,8,size,type,0,stride,offset);
}
void glVertexArrayMultiTexCoordOffsetEXT(GLuint vaobj, GLuint buffer, GLenum texunit, GLint size,
        GLenum type, GLsizei stride, GLintptr offset) {
    GLuint u=0; if(!ValidUnit(texunit,u)) return;
    SetPtr(vaobj,buffer,8+u,size,type,0,stride,offset);
}

void glGetVertexArrayIntegervEXT(GLuint vaobj, GLenum pname, GLint* param) {
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(!v||!param){c.errors.Record(0x0501);return;}
    if(pname==0x8895) *param=(GLint)v->elementBuffer;
    else if(pname==0x85B5) *param=(GLint)vaobj;
    else {c.errors.Record(0x0500);}
}
void glGetVertexArrayIntegeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, GLint* param) {
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(!v||!param||index>=16){c.errors.Record(0x0501);return;}
    auto& a=v->attribs[index];
    switch(pname) {
        case 0x8622: *param=a.enabled; break;
        case 0x8623: *param=a.size; break;
        case 0x8624: *param=a.stride; break;
        case 0x8625: *param=(GLint)a.type; break;
        case 0x886A: *param=a.normalized; break;
        case 0x889F: *param=(GLint)a.buffer; break;
        case 0x234B: *param=(GLint)a.relativeOffset; break;
        case 0x88FE: *param=(GLint)a.divisor; break;
        default: c.errors.Record(0x0500);
    }
}
void glGetVertexArrayPointervEXT(GLuint vaobj, GLenum pname, void** param) {
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(!v||!param){c.errors.Record(0x0501);return;}
    if(pname==0x8895) *param=(void*)(uintptr_t)v->elementBuffer;
    else c.errors.Record(0x0500);
}
void glGetVertexArrayPointeri_vEXT(GLuint vaobj, GLuint index, GLenum pname, void** param) {
    Context& c=Context::Current(); VertexArrayObject* v=VAO(c,vaobj);
    if(!v||!param||index>=16){c.errors.Record(0x0501);return;}
    if(pname==0x889F) *param=(void*)(uintptr_t)v->attribs[index].buffer;
    else if(pname==0x864F) *param=(void*)v->attribs[index].offset;
    else c.errors.Record(0x0500);
}

void glMatrixLoadfEXT(GLenum mode, const GLfloat* m) {
    double d[16]; if(!m)return; for(int i=0;i<16;++i)d[i]=m[i]; Load(Context::Current(),mode,d,false);
}
void glMatrixLoaddEXT(GLenum mode, const GLdouble* m) { Load(Context::Current(),mode,m,false); }
void glMatrixLoadTransposefEXT(GLenum mode, const GLfloat* m) {
    double d[16]; if(!m)return; for(int i=0;i<16;++i)d[i]=m[i]; Load(Context::Current(),mode,d,true);
}
void glMatrixLoadTransposedEXT(GLenum mode, const GLdouble* m) { Load(Context::Current(),mode,m,true); }
void glMatrixMultfEXT(GLenum mode, const GLfloat* m) {
    auto* dst=Matrix(Context::Current(),mode); if(!dst||!m)return;
    std::array<double,16> b{}; for(int i=0;i<16;++i)b[i]=m[i]; Mul(*dst,b);
}
void glMatrixMultdEXT(GLenum mode, const GLdouble* m) {
    auto* dst=Matrix(Context::Current(),mode); if(!dst||!m)return;
    std::array<double,16> b{}; std::memcpy(b.data(),m,16*sizeof(double)); Mul(*dst,b);
}
void glMatrixMultTransposefEXT(GLenum mode, const GLfloat* m) {
    auto* dst=Matrix(Context::Current(),mode); if(!dst||!m)return;
    std::array<double,16> b{}; for(int c=0;c<4;++c)for(int r=0;r<4;++r)b[c*4+r]=m[r*4+c]; Mul(*dst,b);
}
void glMatrixMultTransposedEXT(GLenum mode, const GLdouble* m) {
    auto* dst=Matrix(Context::Current(),mode); if(!dst||!m)return;
    std::array<double,16> b{}; for(int c=0;c<4;++c)for(int r=0;r<4;++r)b[c*4+r]=m[r*4+c]; Mul(*dst,b);
}
void glMatrixLoadIdentityEXT(GLenum mode) { auto* m=Matrix(Context::Current(),mode); if(m)*m=Identity(); }
void glMatrixPushEXT(GLenum mode) {
    Context& c=Context::Current(); auto* m=Matrix(c,mode); if(!m)return;
    auto& st=c.ext.matrixStacks[mode]; if(st.size()>=32){c.errors.Record(0x0503);return;} st.push_back(*m);
}
void glMatrixPopEXT(GLenum mode) {
    Context& c=Context::Current(); auto it=c.ext.matrixStacks.find(mode);
    if(it==c.ext.matrixStacks.end()||it->second.size()<=1){c.errors.Record(0x0504);return;}
    it->second.pop_back();
}
void glMatrixTranslatefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z) {
    std::array<double,16> t=Identity(); t[12]=x;t[13]=y;t[14]=z; Mul(*Matrix(Context::Current(),mode),t);
}
void glMatrixTranslatedEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z) {
    std::array<double,16> t=Identity(); t[12]=x;t[13]=y;t[14]=z; auto* m=Matrix(Context::Current(),mode); if(m)Mul(*m,t);
}
void glMatrixScalefEXT(GLenum mode, GLfloat x, GLfloat y, GLfloat z) {
    std::array<double,16> t=Identity(); t[0]=x;t[5]=y;t[10]=z; auto* m=Matrix(Context::Current(),mode); if(m)Mul(*m,t);
}
void glMatrixScaledEXT(GLenum mode, GLdouble x, GLdouble y, GLdouble z) {
    std::array<double,16> t=Identity(); t[0]=x;t[5]=y;t[10]=z; auto* m=Matrix(Context::Current(),mode); if(m)Mul(*m,t);
}
void glMatrixRotatefEXT(GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    double a=angle*3.14159265358979323846/180.0, len=std::sqrt((double)x*x+(double)y*y+(double)z*z);
    if(len==0){Context::Current().errors.Record(0x0501);return;} x/=len;y/=len;z/=len;
    double c=std::cos(a), s=std::sin(a), t=1-c;
    std::array<double,16> r=Identity();
    r[0]=t*x*x+c; r[1]=t*x*y+s*z; r[2]=t*x*z-s*y;
    r[4]=t*x*y-s*z; r[5]=t*y*y+c; r[6]=t*y*z+s*x;
    r[8]=t*x*z+s*y; r[9]=t*y*z-s*x; r[10]=t*z*z+c;
    auto* m=Matrix(Context::Current(),mode); if(m)Mul(*m,r);
}
void glMatrixRotatedEXT(GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    glMatrixRotatefEXT(mode,(GLfloat)angle,(GLfloat)x,(GLfloat)y,(GLfloat)z);
}
void glMatrixOrthoEXT(GLenum mode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    if(r==l||t==b||f==n){Context::Current().errors.Record(0x0501);return;}
    std::array<double,16> m=Identity();
    m[0]=2/(r-l); m[5]=2/(t-b); m[10]=-2/(f-n);
    m[12]=-(r+l)/(r-l); m[13]=-(t+b)/(t-b); m[14]=-(f+n)/(f-n);
    Mul(*Matrix(Context::Current(),mode),m);
}
void glMatrixFrustumEXT(GLenum mode, GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    if(r==l||t==b||f==n||n<=0){Context::Current().errors.Record(0x0501);return;}
    std::array<double,16> m{};
    m[0]=2*n/(r-l); m[5]=2*n/(t-b); m[8]=(r+l)/(r-l); m[9]=(t+b)/(t-b);
    m[10]=-(f+n)/(f-n); m[11]=-1; m[14]=-2*f*n/(f-n);
    Mul(*Matrix(Context::Current(),mode),m);
}

void glClientAttribDefaultEXT(GLbitfield mask) {
    Context& c=Context::Current(); (void)mask;
    c.ext.clientPtr.clear(); c.ext.clientEnabled.clear();
}
void glPushClientAttribDefaultEXT(GLbitfield mask) { glClientAttribDefaultEXT(mask); }
void glEnableClientStateIndexedEXT(GLenum array, GLuint index) {
    Context::Current().ext.clientEnabled[{array,index}]=true;
}
void glDisableClientStateIndexedEXT(GLenum array, GLuint index) {
    Context::Current().ext.clientEnabled[{array,index}]=false;
}
void glEnableClientStateiEXT(GLenum array, GLuint index) { glEnableClientStateIndexedEXT(array,index); }
void glDisableClientStateiEXT(GLenum array, GLuint index) { glDisableClientStateIndexedEXT(array,index); }
void glGetFloatIndexedvEXT(GLenum target, GLuint index, GLfloat* data) {
    Context& c=Context::Current(); if(!data){c.errors.Record(0x0501);return;}
    auto it=c.ext.texEnvF.find(Key(index,target));
    if(it!=c.ext.texEnvF.end()){for(int i=0;i<4;++i)data[i]=it->second[i];return;}
    for(int i=0;i<4;++i)data[i]=0;
}
void glGetDoubleIndexedvEXT(GLenum target, GLuint index, GLdouble* data) {
    GLfloat f[4]; glGetFloatIndexedvEXT(target,index,f);
    if(data)for(int i=0;i<4;++i)data[i]=f[i];
}
void glGetPointerIndexedvEXT(GLenum target, GLuint index, void** data) {
    Context& c=Context::Current(); if(!data){c.errors.Record(0x0501);return;}
    auto it=c.ext.clientPtr.find({target,index}); *data=it==c.ext.clientPtr.end()?nullptr:it->second;
}
void glGetPointeri_vEXT(GLenum pname, GLuint index, void** params) { glGetPointerIndexedvEXT(pname,index,params); }
void glMultiTexCoordPointerEXT(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void* pointer) {
    GLuint u=0; if(!ValidUnit(texunit,u)) return;
    Context& c=Context::Current();
    c.ext.clientPtr[{0x8078,u}]=(void*)pointer;
    c.ext.clientEnabled[{0x8078,u}]=true;
    (void)size;(void)type;(void)stride;
}
void glMultiTexEnvfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    auto& a=Context::Current().ext.texEnvF[Key(u,pname)]; a={param,0,0,0}; (void)target;
}
void glMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto& a=Context::Current().ext.texEnvF[Key(u,pname)]; for(int i=0;i<4;++i)a[i]=params[i]; (void)target;
}
void glMultiTexEnviEXT(GLenum texunit, GLenum target, GLenum pname, GLint param) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    auto& a=Context::Current().ext.texEnvI[Key(u,pname)]; a={param,0,0,0}; (void)target;
}
void glMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto& a=Context::Current().ext.texEnvI[Key(u,pname)]; for(int i=0;i<4;++i)a[i]=params[i]; (void)target;
}
void glMultiTexGenfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param);
void glMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params);
void glGetMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat* params);
void glMultiTexGendEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble param) {
    glMultiTexGenfEXT(texunit,coord,pname,(GLfloat)param);
}
void glMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLdouble* params) {
    GLfloat f[4]; if(!params){Context::Current().errors.Record(0x0501);return;}
    for(int i=0;i<4;++i)f[i]=(GLfloat)params[i]; glMultiTexGenfvEXT(texunit,coord,pname,f);
}
void glMultiTexGenfEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat param) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    auto& a=Context::Current().ext.texGenF[Key(u,pname)]; a={param,0,0,0}; (void)coord;
}
void glMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, const GLfloat* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto& a=Context::Current().ext.texGenF[Key(u,pname)]; for(int i=0;i<4;++i)a[i]=params[i]; (void)coord;
}
void glMultiTexGeniEXT(GLenum texunit, GLenum coord, GLenum pname, GLint param) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    auto& a=Context::Current().ext.texGenI[Key(u,pname)]; a={param,0,0,0}; (void)coord;
}
void glMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, const GLint* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto& a=Context::Current().ext.texGenI[Key(u,pname)]; for(int i=0;i<4;++i)a[i]=params[i]; (void)coord;
}
void glGetMultiTexEnvfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto it=Context::Current().ext.texEnvF.find(Key(u,pname));
    for(int i=0;i<4;++i)params[i]=it==Context::Current().ext.texEnvF.end()?0:it->second[i];
    (void)target;
}
void glGetMultiTexEnvivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto it=Context::Current().ext.texEnvI.find(Key(u,pname));
    for(int i=0;i<4;++i)params[i]=it==Context::Current().ext.texEnvI.end()?0:it->second[i];
    (void)target;
}
void glGetMultiTexGendvEXT(GLenum texunit, GLenum coord, GLenum pname, GLdouble* params) {
    GLfloat f[4]; glGetMultiTexGenfvEXT(texunit,coord,pname,f);
    if(params)for(int i=0;i<4;++i)params[i]=f[i];
}
void glGetMultiTexGenfvEXT(GLenum texunit, GLenum coord, GLenum pname, GLfloat* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto it=Context::Current().ext.texGenF.find(Key(u,pname));
    for(int i=0;i<4;++i)params[i]=it==Context::Current().ext.texGenF.end()?0:it->second[i];
    (void)coord;
}
void glGetMultiTexGenivEXT(GLenum texunit, GLenum coord, GLenum pname, GLint* params) {
    GLuint u=0;if(!ValidUnit(texunit,u))return;
    if(!params){Context::Current().errors.Record(0x0501);return;}
    auto it=Context::Current().ext.texGenI.find(Key(u,pname));
    for(int i=0;i<4;++i)params[i]=it==Context::Current().ext.texGenI.end()?0:it->second[i];
    (void)coord;
}

static ProgramObject* P(Context& c, GLuint p) {
    auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return nullptr;} return &it->second;
}
void glNamedProgramLocalParameter4fEXT(GLuint program, GLenum target, GLuint index,
        GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    Context& c=Context::Current(); if(!P(c,program))return;
    float v[4]={x,y,z,w}; auto& b=c.ext.programLocals[((uint64_t)target<<32)|index];
    b.assign((uint8_t*)v,(uint8_t*)v+16);
}
void glNamedProgramLocalParameter4fvEXT(GLuint program, GLenum target, GLuint index, const GLfloat* params) {
    if(!params)return; glNamedProgramLocalParameter4fEXT(program,target,index,params[0],params[1],params[2],params[3]);
}
void glNamedProgramLocalParameter4dEXT(GLuint program, GLenum target, GLuint index,
        GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    glNamedProgramLocalParameter4fEXT(program,target,index,(GLfloat)x,(GLfloat)y,(GLfloat)z,(GLfloat)w);
}
void glNamedProgramLocalParameter4dvEXT(GLuint program, GLenum target, GLuint index, const GLdouble* params) {
    if(!params)return; glNamedProgramLocalParameter4fEXT(program,target,index,(GLfloat)params[0],
        (GLfloat)params[1],(GLfloat)params[2],(GLfloat)params[3]);
}
void glNamedProgramLocalParameterI4iEXT(GLuint program, GLenum target, GLuint index,
        GLint x, GLint y, GLint z, GLint w) {
    Context& c=Context::Current(); if(!P(c,program))return;
    GLint v[4]={x,y,z,w}; auto& b=c.ext.programLocals[((uint64_t)target<<32)|index];
    b.assign((uint8_t*)v,(uint8_t*)v+16);
}
void glNamedProgramLocalParameterI4ivEXT(GLuint program, GLenum target, GLuint index, const GLint* params) {
    if(!params)return; glNamedProgramLocalParameterI4iEXT(program,target,index,params[0],params[1],params[2],params[3]);
}
void glNamedProgramLocalParameterI4uiEXT(GLuint program, GLenum target, GLuint index,
        GLuint x, GLuint y, GLuint z, GLuint w) {
    Context& c=Context::Current(); if(!P(c,program))return;
    GLuint v[4]={x,y,z,w}; auto& b=c.ext.programLocals[((uint64_t)target<<32)|index];
    b.assign((uint8_t*)v,(uint8_t*)v+16);
}
void glNamedProgramLocalParameterI4uivEXT(GLuint program, GLenum target, GLuint index, const GLuint* params) {
    if(!params)return; glNamedProgramLocalParameterI4uiEXT(program,target,index,params[0],params[1],params[2],params[3]);
}
void glNamedProgramLocalParameters4fvEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat* params) {
    if(count<0||!params){Context::Current().errors.Record(0x0501);return;}
    for(GLsizei i=0;i<count;++i) glNamedProgramLocalParameter4fvEXT(program,target,index+i,params+i*4);
}
void glNamedProgramLocalParametersI4ivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLint* params) {
    if(count<0||!params){Context::Current().errors.Record(0x0501);return;}
    for(GLsizei i=0;i<count;++i) glNamedProgramLocalParameterI4ivEXT(program,target,index+i,params+i*4);
}
void glNamedProgramLocalParametersI4uivEXT(GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint* params) {
    if(count<0||!params){Context::Current().errors.Record(0x0501);return;}
    for(GLsizei i=0;i<count;++i) glNamedProgramLocalParameterI4uivEXT(program,target,index+i,params+i*4);
}
static void GetLocal(GLuint program, GLenum target, GLuint index, void* out, size_t n) {
    Context& c=Context::Current(); if(!P(c,program))return;
    auto it=c.ext.programLocals.find(((uint64_t)target<<32)|index);
    if(it==c.ext.programLocals.end()||it->second.size()<n){std::memset(out,0,n);return;}
    std::memcpy(out,it->second.data(),n);
}
void glGetNamedProgramLocalParameterfvEXT(GLuint program, GLenum target, GLuint index, GLfloat* params) {
    GetLocal(program,target,index,params,16);
}
void glGetNamedProgramLocalParameterdvEXT(GLuint program, GLenum target, GLuint index, GLdouble* params) {
    GLfloat f[4]; GetLocal(program,target,index,f,16); if(params)for(int i=0;i<4;++i)params[i]=f[i];
}
void glGetNamedProgramLocalParameterIivEXT(GLuint program, GLenum target, GLuint index, GLint* params) {
    GetLocal(program,target,index,params,16);
}
void glGetNamedProgramLocalParameterIuivEXT(GLuint program, GLenum target, GLuint index, GLuint* params) {
    GetLocal(program,target,index,params,16);
}
void glNamedProgramStringEXT(GLuint program, GLenum target, GLenum format, GLsizei len, const void* string) {
    Context& c=Context::Current(); if(!P(c,program))return;
    if(len<0||!string){c.errors.Record(0x0501);return;}
    c.ext.programStrings[program].assign((const char*)string,(size_t)len);
    (void)target;(void)format; // assembly programs are stored, not assembled
}
void glGetNamedProgramStringEXT(GLuint program, GLenum target, GLenum pname, void* string) {
    Context& c=Context::Current(); if(!P(c,program))return;
    (void)target;(void)pname; auto& s=c.ext.programStrings[program];
    if(string) std::memcpy(string,s.data(),s.size());
}
void glGetNamedProgramivEXT(GLuint program, GLenum target, GLenum pname, GLint* params) {
    Context& c=Context::Current(); if(!P(c,program))return;
    (void)target; if(!params){c.errors.Record(0x0501);return;}
    if(pname==0x8627) *params=(GLint)c.ext.programStrings[program].size();
    else *params=0;
}

} // extern "C"
