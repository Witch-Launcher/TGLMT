// gl_ext_perf.cpp — AMD/INTEL performance monitor objects + NV memory attachment.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cstring>

using namespace tglmt;
extern "C" {
void glGenPerfMonitorsAMD(GLsizei n, GLuint* monitors);
void glDeletePerfMonitorsAMD(GLsizei n, GLuint* monitors);
void glBeginPerfMonitorAMD(GLuint monitor);
void glEndPerfMonitorAMD(GLuint monitor);
void glSelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* counterList);
void glGetPerfMonitorGroupsAMD(GLint* numGroups, GLsizei groupsSize, GLuint* groups);
void glGetPerfMonitorCountersAMD(GLuint group, GLint* numCounters, GLint* maxActiveCounters, GLsizei counterSize, GLuint* counters);
void glGetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize, GLsizei* length, GLchar* groupString);
void glGetPerfMonitorCounterStringAMD(GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, GLchar* counterString);
void glGetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname, void* data);
void glGetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten);
void glCreatePerfQueryINTEL(GLuint queryId, GLuint* queryHandle);
void glDeletePerfQueryINTEL(GLuint queryHandle);
void glBeginPerfQueryINTEL(GLuint queryHandle);
void glEndPerfQueryINTEL(GLuint queryHandle);
void glGetFirstPerfQueryIdINTEL(GLuint* queryId);
void glGetNextPerfQueryIdINTEL(GLuint queryId, GLuint* nextQueryId);
void glGetPerfQueryIdByNameINTEL(GLchar* queryName, GLuint* queryId);
void glGetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength, GLchar* queryName, GLuint* dataSize,
        GLuint* noCounters, GLuint* noInstances, GLuint* capsMask);
void glGetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar* counterName,
        GLuint counterDescLength, GLchar* counterDesc, GLuint* counterOffset, GLuint* counterDataSize,
        GLuint* counterTypeEnum, GLuint* counterDataTypeEnum, GLuint64* rawCounterMaxValue);
void glGetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags, GLsizei dataSize, void* data, GLuint* bytesWritten);
void glBufferAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset);
void glNamedBufferAttachMemoryNV(GLuint buffer, GLuint memory, GLuint64 offset);
void glTexAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset);
void glTextureAttachMemoryNV(GLuint texture, GLuint memory, GLuint64 offset);
void glGetMemoryObjectDetachedResourcesuivNV(GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint* params);
void glResetMemoryObjectParameterNV(GLuint memory, GLenum pname);
}

namespace {
static bool Monitor(Context& c, GLuint id) {
    auto it=c.ext.perfMonitors.find(id);
    if(it==c.ext.perfMonitors.end()){c.errors.Record(0x0502);return false;}
    return true;
}
static bool Query(Context& c, GLuint id) {
    auto it=c.ext.perfQueries.find(id);
    if(it==c.ext.perfQueries.end()){c.errors.Record(0x0502);return false;}
    return true;
}
static void NameOut(GLsizei bufSize, GLsizei* length, GLchar* out, const char* s) {
    size_t n=s?strlen(s):0;
    if(length)*length=(GLsizei)n;
    if(out&&bufSize>0){size_t k=std::min(n,(size_t)(bufSize-1));std::memcpy(out,s,k);out[k]=0;}
}
static void MemAttach(Context& c, GLuint memory, GLuint64 offset) {
    (void)offset;
    if(memory==0){c.errors.Record(0x0501);return;}
    c.errors.Record(0x0502); // no external memory objects
}
}

extern "C" {

void glGenPerfMonitorsAMD(GLsizei n, GLuint* monitors) {
    Context& c=Context::Current(); if(n<0||(n&&!monitors)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i){GLuint id=c.ext.nextPerfObject++;c.ext.perfMonitors[id]=false;monitors[i]=id;}
}
void glDeletePerfMonitorsAMD(GLsizei n, GLuint* monitors) {
    Context& c=Context::Current(); if(n<0||(n&&!monitors)){c.errors.Record(0x0501);return;}
    for(GLsizei i=0;i<n;++i)c.ext.perfMonitors.erase(monitors[i]);
}
void glBeginPerfMonitorAMD(GLuint monitor) {
    Context& c=Context::Current(); if(!Monitor(c,monitor))return; c.ext.perfMonitors[monitor]=true;
}
void glEndPerfMonitorAMD(GLuint monitor) {
    Context& c=Context::Current(); if(!Monitor(c,monitor))return; c.ext.perfMonitors[monitor]=false;
}
void glSelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint* counterList) {
    (void)enable;(void)group;(void)numCounters;(void)counterList;
    Context& c=Context::Current(); Monitor(c,monitor);
}
void glGetPerfMonitorGroupsAMD(GLint* numGroups, GLsizei groupsSize, GLuint* groups) {
    Context& c=Context::Current(); if(groupsSize<0){c.errors.Record(0x0501);return;}
    if(groups&&groupsSize>0)groups[0]=0; if(numGroups)*numGroups=0;
}
void glGetPerfMonitorCountersAMD(GLuint group, GLint* numCounters, GLint* maxActiveCounters,
        GLsizei counterSize, GLuint* counters) {
    (void)group;(void)counterSize;(void)counters;
    if(numCounters)*numCounters=0; if(maxActiveCounters)*maxActiveCounters=0;
}
void glGetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize, GLsizei* length, GLchar* groupString) {
    (void)group; NameOut(bufSize,length,groupString,"");
}
void glGetPerfMonitorCounterStringAMD(GLuint group, GLuint counter, GLsizei bufSize, GLsizei* length, GLchar* counterString) {
    (void)group;(void)counter; NameOut(bufSize,length,counterString,"");
}
void glGetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname, void* data) {
    (void)group;(void)counter;(void)pname; if(data)std::memset(data,0,16);
}
void glGetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint* data, GLint* bytesWritten) {
    (void)pname; Context& c=Context::Current(); if(!Monitor(c,monitor))return;
    if(dataSize<0){c.errors.Record(0x0501);return;}
    if(data&&dataSize>0)std::memset(data,0,(size_t)dataSize*sizeof(GLuint));
    if(bytesWritten)*bytesWritten=0;
}

void glCreatePerfQueryINTEL(GLuint queryId, GLuint* queryHandle) {
    Context& c=Context::Current(); if(!queryHandle){c.errors.Record(0x0501);return;}
    (void)queryId; GLuint h=c.ext.nextPerfObject++; c.ext.perfQueries[h]=false; c.ext.perfQueryNames[h]="TGLMT"; *queryHandle=h;
}
void glDeletePerfQueryINTEL(GLuint queryHandle) {
    Context& c=Context::Current(); c.ext.perfQueries.erase(queryHandle); c.ext.perfQueryNames.erase(queryHandle);
}
void glBeginPerfQueryINTEL(GLuint queryHandle) {
    Context& c=Context::Current(); if(!Query(c,queryHandle))return; c.ext.perfQueries[queryHandle]=true;
}
void glEndPerfQueryINTEL(GLuint queryHandle) {
    Context& c=Context::Current(); if(!Query(c,queryHandle))return; c.ext.perfQueries[queryHandle]=false;
}
void glGetFirstPerfQueryIdINTEL(GLuint* queryId) { if(queryId)*queryId=1; }
void glGetNextPerfQueryIdINTEL(GLuint queryId, GLuint* nextQueryId) { (void)queryId; if(nextQueryId)*nextQueryId=0; }
void glGetPerfQueryIdByNameINTEL(GLchar* queryName, GLuint* queryId) {
    (void)queryName; if(queryId)*queryId=1;
}
void glGetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength, GLchar* queryName, GLuint* dataSize,
        GLuint* noCounters, GLuint* noInstances, GLuint* capsMask) {
    (void)queryId; if(queryName&&queryNameLength>0){const char* s="TGLMT";size_t n=std::min<size_t>(strlen(s),queryNameLength-1);
        std::memcpy(queryName,s,n);queryName[n]=0;}
    if(dataSize)*dataSize=0; if(noCounters)*noCounters=0; if(noInstances)*noInstances=0; if(capsMask)*capsMask=0;
}
void glGetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar* counterName,
        GLuint counterDescLength, GLchar* counterDesc, GLuint* counterOffset, GLuint* counterDataSize,
        GLuint* counterTypeEnum, GLuint* counterDataTypeEnum, GLuint64* rawCounterMaxValue) {
    (void)queryId;(void)counterId;
    if(counterName&&counterNameLength>0)counterName[0]=0;
    if(counterDesc&&counterDescLength>0)counterDesc[0]=0;
    if(counterOffset)*counterOffset=0; if(counterDataSize)*counterDataSize=0;
    if(counterTypeEnum)*counterTypeEnum=0; if(counterDataTypeEnum)*counterDataTypeEnum=0;
    if(rawCounterMaxValue)*rawCounterMaxValue=0;
}
void glGetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags, GLsizei dataSize, void* data, GLuint* bytesWritten) {
    (void)flags; Context& c=Context::Current(); if(!Query(c,queryHandle))return;
    if(dataSize<0){c.errors.Record(0x0501);return;}
    if(data&&dataSize>0)std::memset(data,0,(size_t)dataSize);
    if(bytesWritten)*bytesWritten=0;
}

void glBufferAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset) {
    Context& c=Context::Current(); (void)target; MemAttach(c,memory,offset);
}
void glNamedBufferAttachMemoryNV(GLuint buffer, GLuint memory, GLuint64 offset) {
    Context& c=Context::Current(); (void)buffer; MemAttach(c,memory,offset);
}
void glTexAttachMemoryNV(GLenum target, GLuint memory, GLuint64 offset) {
    Context& c=Context::Current(); (void)target; MemAttach(c,memory,offset);
}
void glTextureAttachMemoryNV(GLuint texture, GLuint memory, GLuint64 offset) {
    Context& c=Context::Current(); (void)texture; MemAttach(c,memory,offset);
}
void glGetMemoryObjectDetachedResourcesuivNV(GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint* params) {
    (void)memory;(void)pname;(void)first;
    Context& c=Context::Current();
    if(count<0||(count&&!params)){c.errors.Record(0x0501);return;}
    if(params&&count>0)std::memset(params,0,(size_t)count*sizeof(GLuint));
}
void glResetMemoryObjectParameterNV(GLuint memory, GLenum pname) {
    (void)memory;(void)pname; Context::Current().errors.Record(0x0502);
}

} // extern "C"
