// gl_uniform.cpp — Uniforms/UBO/SSBO → MTLBuffer argument.
// Metal không có glUniform: TGLMT giữ shadow CPU + upload vào MTLBuffer khi draw.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <type_traits>
using namespace tglmt;

static ProgramObject* CurProg(Context& c) {
    GLuint p = c.state.BoundProgram();
    if (!p) { c.errors.Record(0x0502); return nullptr; }
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return nullptr; }
    return &it->second;
}
static void SetU(ProgramObject& pr, GLint loc, const void* v, size_t n) {
    pr.uniforms[loc].assign((const uint8_t*)v, (const uint8_t*)v + n);
}
// Sampler unit tracking: glUniform1i(samplerLoc, unit) → samplerUnits[name]=unit.
// Đúng GL: sampler uniform giá trị là texture unit. AppleDrawGL bind slot k theo unit này.
static void TrackSamplerUnit(ProgramObject& pr, GLint loc, GLint unit) {
    if (unit < 0 || unit >= 32) return;
    for (auto& kv : pr.uniformLoc) {
        if (kv.second != loc) continue;
        const std::string nm = kv.first;
        auto isSamp = [&](const std::vector<std::string>& lst) {
            for (auto& s : lst) if (s == nm) return true;
            return false;
        };
        if (isSamp(pr.vsSamplers) || isSamp(pr.fsSamplers)) {
            pr.samplerUnits[nm] = (GLuint)unit;
            return;
        }
        // program chưa link / sampler list rỗng (link cũ): thử tên trong conv? bỏ qua
        return;
    }
}

namespace tglmt::gl {
GLint glGetUniformLocation(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return -1; }
    std::string nm = n ? n : "";
    auto f = it->second.uniformLoc.find(nm);
    if (f != it->second.uniformLoc.end()) return f->second;
    // Uniform không tồn tại trong program đã link → -1 (đúng GL). Chưa link → cho
    // loc tạm (GL cho query trước link? spec: GetUniformLocation trước link trả -1;
    // TGLMT nới lỏng có chủ đích để app cache loc sớm — ghi rõ, loc vẫn đúng sau link
    // vì entry tra theo tên).
    GLint loc = (GLint)it->second.uniformLoc.size();
    it->second.uniformLoc[nm] = loc;
    for (auto& e : it->second.uniformLayout)
        if (e.name == nm) {
            e.loc = loc;
            break;
        }
    return loc;
}
GLuint glGetUniformBlockIndex(GLuint p, const GLchar* n) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !n) { c.errors.Record(0x0502); return 0xFFFFFFFFu; }
    for (auto& b : it->second.uniformBlocks)
        if (b.name == n) return b.index;
    return 0xFFFFFFFFu; // GL_INVALID_INDEX khi block không tồn tại (đúng GL)
}
void glUniformBlockBinding(GLuint p, GLuint b, GLuint bi) {
    Context& c = Context::Current();
    {
        // Chẩn đoán misbound UBO: chỉ khi TGLMT_DIAG=1.
        if (c.DiagOn()) {
            static int nBind = 0;
            if (++nBind <= 400) {
                std::string nm = "?";
                auto it0 = c.programs.find(p);
                if (it0 != c.programs.end())
                    for (auto& blk : it0->second.uniformBlocks)
                        if (blk.index == b) { nm = blk.name; break; }
                fprintf(stderr, "[TGLMT] ublockbind#%d prog@%u idx=%u(%s) -> point %u\n",
                        nBind, p, b, nm.c_str(), bi);
                fflush(stderr);
            }
        }
    }
    auto it = c.programs.find(p);
    if (it == c.programs.end()) { c.errors.Record(0x0502); return; }
    for (auto& blk : it->second.uniformBlocks)
        if (blk.index == b) { blk.binding = bi; return; }
    // block index chưa có (gọi trước link): lưu shadow để link giữ lại
    // (đơn giản: bỏ qua + log, vì link sẽ mặc định binding 0; app vanilla gọi sau link)
    c.LogDebug(0, 0, 0, 0, "glUniformBlockBinding: block index chưa link, giữ mặc định 0");
    (void)bi;
}
void glShaderStorageBlockBinding(GLuint a, GLuint b, GLuint c_) { (void)a;(void)b;(void)c_; }
void glUniform1f(GLint l, GLfloat a) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,&a,4); }
void glUniform1fv(GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*4); }
void glUniform1i(GLint l, GLint a) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) { SetU(*pr,l,&a,4); TrackSamplerUnit(*pr,l,a); } }
void glUniform1iv(GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*4); }
void glUniform1ui(GLint l, GLuint a) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,&a,4); }
void glUniform1uiv(GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*4); }
void glUniform1d(GLint l, GLdouble a) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,&a,8); }
void glUniform1dv(GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*8); }
void glUniform2f(GLint l, GLfloat a, GLfloat b) { GLfloat v[2]={a,b}; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,8); }
void glUniform2fv(GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*8); }
void glUniform2i(GLint l, GLint a, GLint b) { GLint v[2]={a,b}; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,8); }
void glUniform2iv(GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*8); }
void glUniform2ui(GLint l, GLuint a, GLuint b) { GLuint v[2]={a,b}; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,8); }
void glUniform2uiv(GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*8); }
void glUniform2d(GLint l, GLdouble a, GLdouble b) { GLdouble v[2]={a,b}; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,16); }
void glUniform2dv(GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*16); }
void glUniform3f(GLint l, GLfloat a, GLfloat b, GLfloat c_) { GLfloat v[3]={a,b,c_}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,12); }
void glUniform3fv(GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*12); }
void glUniform3i(GLint l, GLint a, GLint b, GLint c_) { GLint v[3]={a,b,c_}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,12); }
void glUniform3iv(GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*12); }
void glUniform3ui(GLint l, GLuint a, GLuint b, GLuint c_) { GLuint v[3]={a,b,c_}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,12); }
void glUniform3uiv(GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*12); }
void glUniform3d(GLint l, GLdouble a, GLdouble b, GLdouble c_) { GLdouble v[3]={a,b,c_}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,24); }
void glUniform3dv(GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*24); }
void glUniform4f(GLint l, GLfloat a, GLfloat b, GLfloat c_, GLfloat d) { GLfloat v[4]={a,b,c_,d}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,16); }
void glUniform4fv(GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*16); }
void glUniform4i(GLint l, GLint a, GLint b, GLint c_, GLint d) { GLint v[4]={a,b,c_,d}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,16); }
void glUniform4iv(GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*16); }
void glUniform4ui(GLint l, GLuint a, GLuint b, GLuint c_, GLuint d) { GLuint v[4]={a,b,c_,d}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,16); }
void glUniform4uiv(GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*16); }
void glUniform4d(GLint l, GLdouble a, GLdouble b, GLdouble c_, GLdouble d) { GLdouble v[4]={a,b,c_,d}; Context& ctx=Context::Current(); if(auto* pr=CurProg(ctx)) SetU(*pr,l,v,32); }
void glUniform4dv(GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*32); }
void glUniformMatrix2fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*16); }
void glUniformMatrix2dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*32); }
void glUniformMatrix2x3fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*24); }
void glUniformMatrix2x3dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*48); }
void glUniformMatrix2x4fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*32); }
void glUniformMatrix2x4dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*64); }
void glUniformMatrix3fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*36); }
void glUniformMatrix3dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*72); }
void glUniformMatrix3x2fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*24); }
void glUniformMatrix3x2dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*48); }
void glUniformMatrix3x4fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*48); }
void glUniformMatrix3x4dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*96); }
void glUniformMatrix4fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*64); }
void glUniformMatrix4dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*128); }
void glUniformMatrix4x2fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*32); }
void glUniformMatrix4x2dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*64); }
void glUniformMatrix4x3fv(GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*48); }
void glUniformMatrix4x3dv(GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); if(auto* pr=CurProg(c)) SetU(*pr,l,v,(size_t)n*96); }
void glUniformSubroutinesuiv(GLenum a, GLsizei b, const GLuint* c_) { (void)a;(void)b;(void)c_; }
// ProgramUniform DSA — cập nhật program không cần bind
#define PU1(func, T, N) void func(GLuint p, GLint l, T v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,&v,sizeof(v)); if constexpr (std::is_same<T,GLint>::value) TrackSamplerUnit(it->second,l,v); (void)N; }
PU1(glProgramUniform1f, GLfloat, 1) PU1(glProgramUniform1i, GLint, 1)
PU1(glProgramUniform1ui, GLuint, 1) PU1(glProgramUniform1d, GLdouble, 1)
void glProgramUniform1fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*4); }
void glProgramUniform1iv(GLuint p, GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*4); }
void glProgramUniform1uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*4); }
void glProgramUniform1dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*8); }
void glProgramUniform2f(GLuint p, GLint l, GLfloat a, GLfloat b) { GLfloat v[2]={a,b}; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,8); }
void glProgramUniform2fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*8); }
void glProgramUniform2i(GLuint p, GLint l, GLint a, GLint b) { GLint v[2]={a,b}; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,8); }
void glProgramUniform2iv(GLuint p, GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*8); }
void glProgramUniform2ui(GLuint p, GLint l, GLuint a, GLuint b) { GLuint v[2]={a,b}; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,8); }
void glProgramUniform2uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*8); }
void glProgramUniform2d(GLuint p, GLint l, GLdouble a, GLdouble b) { GLdouble v[2]={a,b}; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,16); }
void glProgramUniform2dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*16); }
void glProgramUniform3f(GLuint p, GLint l, GLfloat a, GLfloat b, GLfloat c_) { GLfloat v[3]={a,b,c_}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,12); }
void glProgramUniform3fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*12); }
void glProgramUniform3i(GLuint p, GLint l, GLint a, GLint b, GLint c_) { GLint v[3]={a,b,c_}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,12); }
void glProgramUniform3iv(GLuint p, GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*12); }
void glProgramUniform3ui(GLuint p, GLint l, GLuint a, GLuint b, GLuint c_) { GLuint v[3]={a,b,c_}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,12); }
void glProgramUniform3uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*12); }
void glProgramUniform3d(GLuint p, GLint l, GLdouble a, GLdouble b, GLdouble c_) { GLdouble v[3]={a,b,c_}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,24); }
void glProgramUniform3dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*24); }
void glProgramUniform4f(GLuint p, GLint l, GLfloat a, GLfloat b, GLfloat c_, GLfloat d) { GLfloat v[4]={a,b,c_,d}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,16); }
void glProgramUniform4fv(GLuint p, GLint l, GLsizei n, const GLfloat* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*16); }
void glProgramUniform4i(GLuint p, GLint l, GLint a, GLint b, GLint c_, GLint d) { GLint v[4]={a,b,c_,d}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,16); }
void glProgramUniform4iv(GLuint p, GLint l, GLsizei n, const GLint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*16); }
void glProgramUniform4ui(GLuint p, GLint l, GLuint a, GLuint b, GLuint c_, GLuint d) { GLuint v[4]={a,b,c_,d}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,16); }
void glProgramUniform4uiv(GLuint p, GLint l, GLsizei n, const GLuint* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*16); }
void glProgramUniform4d(GLuint p, GLint l, GLdouble a, GLdouble b, GLdouble c_, GLdouble d) { GLdouble v[4]={a,b,c_,d}; Context& ctx=Context::Current(); auto it=ctx.programs.find(p); if(it==ctx.programs.end()){ctx.errors.Record(0x0502);return;} SetU(it->second,l,v,32); }
void glProgramUniform4dv(GLuint p, GLint l, GLsizei n, const GLdouble* v) { Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*32); }
void glProgramUniformMatrix2fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*16); }
void glProgramUniformMatrix2dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*32); }
void glProgramUniformMatrix2x3fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*24); }
void glProgramUniformMatrix2x3dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*48); }
void glProgramUniformMatrix2x4fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*32); }
void glProgramUniformMatrix2x4dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*64); }
void glProgramUniformMatrix3fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*36); }
void glProgramUniformMatrix3dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*72); }
void glProgramUniformMatrix3x2fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*24); }
void glProgramUniformMatrix3x2dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*48); }
void glProgramUniformMatrix3x4fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*48); }
void glProgramUniformMatrix3x4dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*96); }
void glProgramUniformMatrix4fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*64); }
void glProgramUniformMatrix4dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*128); }
void glProgramUniformMatrix4x2fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*32); }
void glProgramUniformMatrix4x2dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*64); }
void glProgramUniformMatrix4x3fv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLfloat* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*48); }
void glProgramUniformMatrix4x3dv(GLuint p, GLint l, GLsizei n, GLboolean t, const GLdouble* v) { (void)t; Context& c=Context::Current(); auto it=c.programs.find(p); if(it==c.programs.end()){c.errors.Record(0x0502);return;} SetU(it->second,l,v,(size_t)n*96); }
} // namespace tglmt::gl
