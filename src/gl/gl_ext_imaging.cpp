// gl_ext_imaging.cpp — ARB_imaging color table / convolution / histogram state.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>

using namespace tglmt;
extern "C" {
void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* table);
void glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void* data);
void glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
void glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void glColorTableParameteriv(GLenum target, GLenum pname, const GLint* params);
void glGetColorTable(GLenum target, GLenum format, GLenum type, void* table);
void glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetColorTableParameteriv(GLenum target, GLenum pname, GLint* params);
void glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* image);
void glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* image);
void glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
void glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* row, const void* column);
void glConvolutionParameterf(GLenum target, GLenum pname, GLfloat param);
void glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat* params);
void glConvolutionParameteri(GLenum target, GLenum pname, GLint param);
void glConvolutionParameteriv(GLenum target, GLenum pname, const GLint* params);
void glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, void* image);
void glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint* params);
void glGetSeparableFilter(GLenum target, GLenum format, GLenum type, void* row, void* column, void* span);
void glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
void glResetHistogram(GLenum target);
void glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values);
void glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetHistogramParameteriv(GLenum target, GLenum pname, GLint* params);
void glMinmax(GLenum target, GLenum internalformat, GLboolean sink);
void glResetMinmax(GLenum target);
void glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values);
void glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat* params);
void glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint* params);
}

namespace {
static bool ColorTarget(GLenum t) {
    return t==0x80D0||t==0x80D1||t==0x80D2||t==0x80D3||t==0x80D4||t==0x80D5;
}
static bool ConvTarget(GLenum t) {
    return t==0x8010||t==0x8011||t==0x8012||t==0x8016||t==0x8017||t==0x8018;
}
static bool HistTarget(GLenum t) { return t==0x8024||t==0x8025; }
static bool MinmaxTarget(GLenum t) { return t==0x802E; }
static int Components(GLenum f) {
    switch(f){case 0x1908:case 0x1903:case 0x1906:return 4;case 0x1907:return 3;
        case 0x1909:case 0x190A:case 0x1904:case 0x1905:return 2;default:return 1;}
}
static int TypeSize(GLenum t) {
    switch(t){case 0x1406:case 0x1404:case 0x1405:case 0x140B:return 4;
        case 0x1403:case 0x1402:return 2;case 0x1401:case 0x1400:return 1;default:return 1;}
}
static int PixBytes(GLenum f, GLenum t) { return Components(f)*TypeSize(t); }
static uint64_t Key(GLenum target, GLenum pname) { return ((uint64_t)target<<32)|pname; }
static int TableWidth(Context& c, GLenum t) {
    auto it=c.ext.colorTableWidths.find(t); return it==c.ext.colorTableWidths.end()?0:it->second;
}
static int ConvWidth(Context& c, GLenum t) {
    auto it=c.ext.convolutionSizes.find(t); return it==c.ext.convolutionSizes.end()?0:it->second[0];
}
static int ConvHeight(Context& c, GLenum t) {
    auto it=c.ext.convolutionSizes.find(t); return it==c.ext.convolutionSizes.end()?0:it->second[1];
}
static void OutF(const std::array<GLfloat,4>& a, GLfloat* p) { if(p)for(int i=0;i<4;++i)p[i]=a[i]; }
static void OutI(const std::array<GLint,4>& a, GLint* p) { if(p)for(int i=0;i<4;++i)p[i]=a[i]; }
}

extern "C" {

void glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* table) {
    Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0){c.errors.Record(0x0501);return;}
    if(target>=0x80D3)return; // proxy
    size_t n=(size_t)std::max(width,0)*(size_t)PixBytes(format,type);
    c.ext.colorTableWidths[target]=width;c.ext.colorTableFormats[target]=format;
    c.ext.colorTables[target].assign(n,0);
    if(table&&n)std::memcpy(c.ext.colorTables[target].data(),table,n);
    (void)internalformat;
}
void glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void* data) {
    Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(start<0||count<0){c.errors.Record(0x0501);return;}
    if(target>=0x80D3)return;
    size_t bpp=(size_t)PixBytes(format,type), off=(size_t)start*bpp;
    auto& v=c.ext.colorTables[target]; v.resize(std::max(v.size(),off+(size_t)count*bpp),0);
    if(data&&count)std::memcpy(v.data()+off,data,(size_t)count*bpp);
}
void glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
    (void)x;(void)y; Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0){c.errors.Record(0x0501);return;}
    if(target>=0x80D3)return;
    c.ext.colorTableWidths[target]=width;c.ext.colorTables[target].assign((size_t)width*4,0);
    (void)internalformat;
}
void glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width) {
    (void)x;(void)y; Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(start<0||width<0){c.errors.Record(0x0501);return;}
    if(target>=0x80D3)return;
    auto& v=c.ext.colorTables[target];v.resize(std::max(v.size(),(size_t)(start+width)*4),0);
}
void glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(!params){c.errors.Record(0x0501);return;}
    if(pname==0x80D6){for(int i=0;i<4;++i)c.ext.colorTableScale[target][i]=params[i];}
    else if(pname==0x80D7){for(int i=0;i<4;++i)c.ext.colorTableBias[target][i]=params[i];}
    else c.errors.Record(0x0500);
}
void glColorTableParameteriv(GLenum target, GLenum pname, const GLint* params) {
    GLfloat f[4]={1,1,1,1}; if(params)for(int i=0;i<4;++i)f[i]=(GLfloat)params[i]; (void)pname;
    glColorTableParameterfv(target,pname,f);
}
void glGetColorTable(GLenum target, GLenum format, GLenum type, void* table) {
    Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(!table){c.errors.Record(0x0501);return;}
    auto it=c.ext.colorTables.find(target);
    size_t n=it==c.ext.colorTables.end()?0:it->second.size();
    GLsizei w=TableWidth(c,target); size_t want=(size_t)w*(size_t)PixBytes(format,type);
    size_t k=std::min(n,want); if(k)std::memcpy(table,it->second.data(),k);
    if(want>k)std::memset((uint8_t*)table+k,0,want-k);
}
void glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    Context& c=Context::Current();
    if(!ColorTarget(target)){c.errors.Record(0x0500);return;}
    if(!params){c.errors.Record(0x0501);return;}
    if(pname==0x80D6){OutF(c.ext.colorTableScale[target],params);return;}
    if(pname==0x80D7){OutF(c.ext.colorTableBias[target],params);return;}
    GLint v=0;
    if(pname==0x80D9)v=TableWidth(c,target);
    else if(pname==0x80D8)v=(GLint)(c.ext.colorTableFormats.count(target)?c.ext.colorTableFormats[target]:0);
    else if(pname>=0x80DA&&pname<=0x80DD)v=8;
    params[0]=(GLfloat)v;
}
void glGetColorTableParameteriv(GLenum target, GLenum pname, GLint* params) {
    Context& c=Context::Current(); if(!params){c.errors.Record(0x0501);return;}
    if(pname==0x80D6||pname==0x80D7){GLfloat f[4];glGetColorTableParameterfv(target,pname,f);for(int i=0;i<4;++i)params[i]=(GLint)f[i];return;}
    GLfloat f=0;glGetColorTableParameterfv(target,pname,&f);params[0]=(GLint)f;
}

void glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void* image) {
    Context& c=Context::Current();
    if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0){c.errors.Record(0x0501);return;}
    if(target>=0x8016)return;
    size_t n=(size_t)width*(size_t)PixBytes(format,type);
    c.ext.convolutionImages[target].assign(n,0);
    c.ext.convolutionSizes[target]={width,1};
    if(image&&n)std::memcpy(c.ext.convolutionImages[target].data(),image,n);
    (void)internalformat;
}
void glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* image) {
    Context& c=Context::Current();
    if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0||height<0){c.errors.Record(0x0501);return;}
    if(target>=0x8016)return;
    size_t n=(size_t)width*(size_t)height*(size_t)PixBytes(format,type);
    c.ext.convolutionImages[target].assign(n,0);
    c.ext.convolutionSizes[target]={width,height};
    if(image&&n)std::memcpy(c.ext.convolutionImages[target].data(),image,n);
    (void)internalformat;
}
void glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
    (void)x;(void)y; Context& c=Context::Current();
    if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0){c.errors.Record(0x0501);return;}
    if(target>=0x8016)return; c.ext.convolutionSizes[target]={width,1};c.ext.convolutionImages[target].assign((size_t)width*4,0); (void)internalformat;
}
void glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)x;(void)y; Context& c=Context::Current();
    if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0||height<0){c.errors.Record(0x0501);return;}
    if(target>=0x8016)return; c.ext.convolutionSizes[target]={width,height};c.ext.convolutionImages[target].assign((size_t)width*height*4,0); (void)internalformat;
}
void glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* row, const void* column) {
    Context& c=Context::Current();
    if(target!=0x8012){c.errors.Record(0x0500);return;}
    if(width<0||height<0){c.errors.Record(0x0501);return;}
    size_t rb=(size_t)width*(size_t)PixBytes(format,type),cb=(size_t)height*(size_t)PixBytes(format,type);
    c.ext.convolutionImages[target].assign(rb+cb,0);
    if(row&&rb)std::memcpy(c.ext.convolutionImages[target].data(),row,rb);
    if(column&&cb)std::memcpy(c.ext.convolutionImages[target].data()+rb,column,cb);
    c.ext.convolutionSizes[target]={width,height}; (void)internalformat;
}
void glConvolutionParameterf(GLenum target, GLenum pname, GLfloat param) {
    Context& c=Context::Current(); if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    auto& a=c.ext.imagingF[Key(target,pname)]; a={param,0,0,0};
}
void glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat* params) {
    Context& c=Context::Current(); if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(!params){c.errors.Record(0x0501);return;}
    auto& a=c.ext.imagingF[Key(target,pname)]; for(int i=0;i<4;++i)a[i]=params[i];
}
void glConvolutionParameteri(GLenum target, GLenum pname, GLint param) {
    Context& c=Context::Current(); if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    auto& a=c.ext.imagingI[Key(target,pname)]; a={param,0,0,0};
}
void glConvolutionParameteriv(GLenum target, GLenum pname, const GLint* params) {
    Context& c=Context::Current(); if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(!params){c.errors.Record(0x0501);return;}
    auto& a=c.ext.imagingI[Key(target,pname)]; for(int i=0;i<4;++i)a[i]=params[i];
}
void glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, void* image) {
    Context& c=Context::Current(); if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(!image){c.errors.Record(0x0501);return;}
    size_t want=(size_t)ConvWidth(c,target)*(size_t)std::max(1,ConvHeight(c,target))*(size_t)PixBytes(format,type);
    auto it=c.ext.convolutionImages.find(target); size_t n=it==c.ext.convolutionImages.end()?0:it->second.size();
    size_t k=std::min(n,want); if(k)std::memcpy(image,it->second.data(),k); if(want>k)std::memset((uint8_t*)image+k,0,want-k);
}
void glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    Context& c=Context::Current();
    if(!ConvTarget(target)){c.errors.Record(0x0500);return;}
    if(!params){c.errors.Record(0x0501);return;}
    if(pname==0x8018||pname==0x8019){params[0]=(GLfloat)(pname==0x8018?ConvWidth(c,target):ConvHeight(c,target));return;}
    auto it=c.ext.imagingF.find(Key(target,pname)); if(it!=c.ext.imagingF.end()){OutF(it->second,params);return;}
    auto ii=c.ext.imagingI.find(Key(target,pname)); if(ii!=c.ext.imagingI.end()){for(int i=0;i<4;++i)params[i]=(GLfloat)ii->second[i];return;}
    for(int i=0;i<4;++i)params[i]=0;
}
void glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint* params) {
    GLfloat f[4]={};glGetConvolutionParameterfv(target,pname,f);if(params)for(int i=0;i<4;++i)params[i]=(GLint)f[i];
}
void glGetSeparableFilter(GLenum target, GLenum format, GLenum type, void* row, void* column, void* span) {
    (void)format;(void)type;(void)row;(void)column;(void)span;
    Context& c=Context::Current(); if(target!=0x8012)c.errors.Record(0x0500);
}

void glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink) {
    Context& c=Context::Current(); if(!HistTarget(target)){c.errors.Record(0x0500);return;}
    if(width<0){c.errors.Record(0x0501);return;}
    if(target==0x8025)return;
    c.ext.histogramParams[target]={width,(GLint)internalformat,0};c.ext.histogramSink[target]=sink!=0;
}
void glResetHistogram(GLenum target) {
    Context& c=Context::Current(); if(!HistTarget(target)){c.errors.Record(0x0500);return;}
    c.ext.histogramParams.erase(target);c.ext.histogramSink.erase(target);
}
void glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values) {
    Context& c=Context::Current(); if(!HistTarget(target)){c.errors.Record(0x0500);return;}
    (void)format;(void)type; if(values){auto it=c.ext.histogramParams.find(target);GLsizei w=it==c.ext.histogramParams.end()?0:it->second[0];std::memset(values,0,(size_t)w*4);}
    if(reset)glResetHistogram(target);
}
void glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    GLint v[4]={0};glGetHistogramParameteriv(target,pname,v);if(params)params[0]=(GLfloat)v[0];
}
void glGetHistogramParameteriv(GLenum target, GLenum pname, GLint* params) {
    Context& c=Context::Current(); if(!HistTarget(target)||!params){c.errors.Record(0x0501);return;}
    auto it=c.ext.histogramParams.find(target); params[0]=0;
    if(it!=c.ext.histogramParams.end()){
        if(pname==0x8026)params[0]=it->second[0];
        else if(pname==0x8027)params[0]=it->second[1];
        else if(pname>=0x8028&&pname<=0x802B)params[0]=8;
        else if(pname==0x802D)params[0]=c.ext.histogramSink[target]?1:0;
    }
}
void glMinmax(GLenum target, GLenum internalformat, GLboolean sink) {
    Context& c=Context::Current(); if(!MinmaxTarget(target)){c.errors.Record(0x0500);return;}
    c.ext.minmaxParams[target]={(GLint)internalformat,0};c.ext.minmaxSink[target]=sink!=0;
}
void glResetMinmax(GLenum target) {
    Context& c=Context::Current(); if(!MinmaxTarget(target)){c.errors.Record(0x0500);return;}
    c.ext.minmaxParams.erase(target);c.ext.minmaxSink.erase(target);
}
void glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, void* values) {
    (void)format;(void)type; Context& c=Context::Current(); if(!MinmaxTarget(target)){c.errors.Record(0x0500);return;}
    if(values)std::memset(values,0,32); if(reset)glResetMinmax(target);
}
void glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat* params) {
    GLint v[4]={0};glGetMinmaxParameteriv(target,pname,v);if(params)params[0]=(GLfloat)v[0];
}
void glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint* params) {
    Context& c=Context::Current(); if(!MinmaxTarget(target)||!params){c.errors.Record(0x0501);return;}
    auto it=c.ext.minmaxParams.find(target); params[0]=0;
    if(it!=c.ext.minmaxParams.end()){
        if(pname==0x802F)params[0]=it->second[0];
        else if(pname==0x8030)params[0]=c.ext.minmaxSink[target]?1:0;
    }
}

} // extern "C"
