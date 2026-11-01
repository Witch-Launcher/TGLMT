// gl_ext_uniform64.cpp — ARB_gpu_shader_int64 + NV_gpu_shader5 uniform entry points.
// TGLMT stores uniform payload bytes; these entries preserve 64-bit values.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;

namespace {
static ProgramObject* Prog(GLuint p) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
static ProgramObject* CurProg() {
    Context& c = Context::Current();
    GLuint p = c.state.BoundProgram();
    if (!p) { c.errors.Record(0x0502); return nullptr; }
    return Prog(p);
}
static void Put(GLint loc, const void* v, size_t n) {
    if (!v) return;
    ProgramObject* p = CurProg();
    if (!p) return;
    p->uniforms[loc].assign((const uint8_t*)v, (const uint8_t*)v + n);
}
static void PutP(GLuint prog, GLint loc, const void* v, size_t n) {
    if (!v) return;
    ProgramObject* p = Prog(prog);
    if (!p) return;
    p->uniforms[loc].assign((const uint8_t*)v, (const uint8_t*)v + n);
}
static void Get64(GLuint prog, GLint loc, GLsizei bufSize, GLint64* out) {
    Context& c = Context::Current();
    ProgramObject* p = Prog(prog);
    if (!p) return;
    auto it = p->uniforms.find(loc);
    if (it == p->uniforms.end()) { if (out) out[0] = 0; return; }
    if (bufSize < (GLsizei)it->second.size()) { c.errors.Record(0x0502); return; }
    if (out) std::memcpy(out, it->second.data(), it->second.size());
}
} // namespace

extern "C" {
#define U64_1(name) void name(GLint l, GLint64 v) { Put(l, &v, 8); }
#define U64_2(name) void name(GLint l, GLint64 x, GLint64 y) { GLint64 v[2]={x,y}; Put(l,v,16); }
#define U64_3(name) void name(GLint l, GLint64 x, GLint64 y, GLint64 z) { GLint64 v[3]={x,y,z}; Put(l,v,24); }
#define U64_4(name) void name(GLint l, GLint64 x, GLint64 y, GLint64 z, GLint64 w) { GLint64 v[4]={x,y,z,w}; Put(l,v,32); }
#define U64V_1(name) void name(GLint l, GLsizei n, const GLint64* v) { if(n>0) Put(l,v,(size_t)n*8); }
#define U64V_2(name) void name(GLint l, GLsizei n, const GLint64* v) { if(n>0) Put(l,v,(size_t)n*16); }
#define U64V_3(name) void name(GLint l, GLsizei n, const GLint64* v) { if(n>0) Put(l,v,(size_t)n*24); }
#define U64V_4(name) void name(GLint l, GLsizei n, const GLint64* v) { if(n>0) Put(l,v,(size_t)n*32); }
#define UU64_1(name) void name(GLint l, GLuint64 v) { Put(l, &v, 8); }
#define UU64_2(name) void name(GLint l, GLuint64 x, GLuint64 y) { GLuint64 v[2]={x,y}; Put(l,v,16); }
#define UU64_3(name) void name(GLint l, GLuint64 x, GLuint64 y, GLuint64 z) { GLuint64 v[3]={x,y,z}; Put(l,v,24); }
#define UU64_4(name) void name(GLint l, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) { GLuint64 v[4]={x,y,z,w}; Put(l,v,32); }
#define UU64V_1(name) void name(GLint l, GLsizei n, const GLuint64* v) { if(n>0) Put(l,v,(size_t)n*8); }
#define UU64V_2(name) void name(GLint l, GLsizei n, const GLuint64* v) { if(n>0) Put(l,v,(size_t)n*16); }
#define UU64V_3(name) void name(GLint l, GLsizei n, const GLuint64* v) { if(n>0) Put(l,v,(size_t)n*24); }
#define UU64V_4(name) void name(GLint l, GLsizei n, const GLuint64* v) { if(n>0) Put(l,v,(size_t)n*32); }

U64_1(glUniform1i64ARB) U64_2(glUniform2i64ARB) U64_3(glUniform3i64ARB) U64_4(glUniform4i64ARB)
U64V_1(glUniform1i64vARB) U64V_2(glUniform2i64vARB) U64V_3(glUniform3i64vARB) U64V_4(glUniform4i64vARB)
UU64_1(glUniform1ui64ARB) UU64_2(glUniform2ui64ARB) UU64_3(glUniform3ui64ARB) UU64_4(glUniform4ui64ARB)
UU64V_1(glUniform1ui64vARB) UU64V_2(glUniform2ui64vARB) UU64V_3(glUniform3ui64vARB) UU64V_4(glUniform4ui64vARB)
U64_1(glUniform1i64NV) U64_2(glUniform2i64NV) U64_3(glUniform3i64NV) U64_4(glUniform4i64NV)
U64V_1(glUniform1i64vNV) U64V_2(glUniform2i64vNV) U64V_3(glUniform3i64vNV) U64V_4(glUniform4i64vNV)
UU64_1(glUniform1ui64NV) UU64_2(glUniform2ui64NV) UU64_3(glUniform3ui64NV) UU64_4(glUniform4ui64NV)
UU64V_1(glUniform1ui64vNV) UU64V_2(glUniform2ui64vNV) UU64V_3(glUniform3ui64vNV) UU64V_4(glUniform4ui64vNV)

void glGetUniformi64vARB(GLuint p, GLint l, GLint64* v) { Get64(p, l, 0x7FFFFFFF, v); }
void glGetUniformui64vARB(GLuint p, GLint l, GLuint64* v) { Get64(p, l, 0x7FFFFFFF, (GLint64*)v); }
void glGetUniformi64vNV(GLuint p, GLint l, GLint64* v) { glGetUniformi64vARB(p, l, v); }
void glGetUniformui64vNV(GLuint p, GLint l, GLuint64* v) { glGetUniformi64vARB(p, l, (GLint64*)v); }
void glGetnUniformi64vARB(GLuint p, GLint l, GLsizei n, GLint64* v) { Get64(p, l, n, v); }
void glGetnUniformui64vARB(GLuint p, GLint l, GLsizei n, GLuint64* v) { Get64(p, l, n, (GLint64*)v); }

#define PU64_1(name) void name(GLuint p, GLint l, GLint64 v) { PutP(p,l,&v,8); }
#define PU64_2(name) void name(GLuint p, GLint l, GLint64 x, GLint64 y) { GLint64 v[2]={x,y}; PutP(p,l,v,16); }
#define PU64_3(name) void name(GLuint p, GLint l, GLint64 x, GLint64 y, GLint64 z) { GLint64 v[3]={x,y,z}; PutP(p,l,v,24); }
#define PU64_4(name) void name(GLuint p, GLint l, GLint64 x, GLint64 y, GLint64 z, GLint64 w) { GLint64 v[4]={x,y,z,w}; PutP(p,l,v,32); }
#define PU64V_1(name) void name(GLuint p, GLint l, GLsizei n, const GLint64* v) { if(n>0) PutP(p,l,v,(size_t)n*8); }
#define PU64V_2(name) void name(GLuint p, GLint l, GLsizei n, const GLint64* v) { if(n>0) PutP(p,l,v,(size_t)n*16); }
#define PU64V_3(name) void name(GLuint p, GLint l, GLsizei n, const GLint64* v) { if(n>0) PutP(p,l,v,(size_t)n*24); }
#define PU64V_4(name) void name(GLuint p, GLint l, GLsizei n, const GLint64* v) { if(n>0) PutP(p,l,v,(size_t)n*32); }
#define PUU64_1(name) void name(GLuint p, GLint l, GLuint64 v) { PutP(p,l,&v,8); }
#define PUU64_2(name) void name(GLuint p, GLint l, GLuint64 x, GLuint64 y) { GLuint64 v[2]={x,y}; PutP(p,l,v,16); }
#define PUU64_3(name) void name(GLuint p, GLint l, GLuint64 x, GLuint64 y, GLuint64 z) { GLuint64 v[3]={x,y,z}; PutP(p,l,v,24); }
#define PUU64_4(name) void name(GLuint p, GLint l, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) { GLuint64 v[4]={x,y,z,w}; PutP(p,l,v,32); }
#define PUU64V_1(name) void name(GLuint p, GLint l, GLsizei n, const GLuint64* v) { if(n>0) PutP(p,l,v,(size_t)n*8); }
#define PUU64V_2(name) void name(GLuint p, GLint l, GLsizei n, const GLuint64* v) { if(n>0) PutP(p,l,v,(size_t)n*16); }
#define PUU64V_3(name) void name(GLuint p, GLint l, GLsizei n, const GLuint64* v) { if(n>0) PutP(p,l,v,(size_t)n*24); }
#define PUU64V_4(name) void name(GLuint p, GLint l, GLsizei n, const GLuint64* v) { if(n>0) PutP(p,l,v,(size_t)n*32); }

PU64_1(glProgramUniform1i64ARB) PU64_2(glProgramUniform2i64ARB) PU64_3(glProgramUniform3i64ARB) PU64_4(glProgramUniform4i64ARB)
PU64V_1(glProgramUniform1i64vARB) PU64V_2(glProgramUniform2i64vARB) PU64V_3(glProgramUniform3i64vARB) PU64V_4(glProgramUniform4i64vARB)
PUU64_1(glProgramUniform1ui64ARB) PUU64_2(glProgramUniform2ui64ARB) PUU64_3(glProgramUniform3ui64ARB) PUU64_4(glProgramUniform4ui64ARB)
PUU64V_1(glProgramUniform1ui64vARB) PUU64V_2(glProgramUniform2ui64vARB) PUU64V_3(glProgramUniform3ui64vARB) PUU64V_4(glProgramUniform4ui64vARB)
PU64_1(glProgramUniform1i64NV) PU64_2(glProgramUniform2i64NV) PU64_3(glProgramUniform3i64NV) PU64_4(glProgramUniform4i64NV)
PU64V_1(glProgramUniform1i64vNV) PU64V_2(glProgramUniform2i64vNV) PU64V_3(glProgramUniform3i64vNV) PU64V_4(glProgramUniform4i64vNV)
PUU64_1(glProgramUniform1ui64NV) PUU64_2(glProgramUniform2ui64NV) PUU64_3(glProgramUniform3ui64NV) PUU64_4(glProgramUniform4ui64NV)
PUU64V_1(glProgramUniform1ui64vNV) PUU64V_2(glProgramUniform2ui64vNV) PUU64V_3(glProgramUniform3ui64vNV) PUU64V_4(glProgramUniform4ui64vNV)

} // extern "C"
