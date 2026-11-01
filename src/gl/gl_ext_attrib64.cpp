// gl_ext_attrib64.cpp — NV_vertex_attrib_integer_64bit.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;
extern "C" {

static GLint64* Cur64(Context& c, GLuint i) {
    if (i >= 16) { c.errors.Record(0x0501); return nullptr; }
    auto it = c.attrib64.find(i);
    if (it == c.attrib64.end()) {
        std::array<GLint64, 4> init = {0, 0, 0, 1};
        it = c.attrib64.emplace(i, init).first;
    }
    return it->second.data();
}
static GLint64* V64(const GLint64* v) { return (GLint64*)v; }

void glVertexAttribL1i64NV(GLuint i, GLint64 x) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=x; d[1]=0; d[2]=0; d[3]=1;
}
void glVertexAttribL1i64vNV(GLuint i, const GLint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=v[0]; d[1]=0; d[2]=0; d[3]=1;
}
void glVertexAttribL2i64NV(GLuint i, GLint64 x, GLint64 y) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=x; d[1]=y; d[2]=0; d[3]=1;
}
void glVertexAttribL2i64vNV(GLuint i, const GLint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=v[0]; d[1]=v[1]; d[2]=0; d[3]=1;
}
void glVertexAttribL3i64NV(GLuint i, GLint64 x, GLint64 y, GLint64 z) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=x; d[1]=y; d[2]=z; d[3]=1;
}
void glVertexAttribL3i64vNV(GLuint i, const GLint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=v[0]; d[1]=v[1]; d[2]=v[2]; d[3]=1;
}
void glVertexAttribL4i64NV(GLuint i, GLint64 x, GLint64 y, GLint64 z, GLint64 w) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=x; d[1]=y; d[2]=z; d[3]=w;
}
void glVertexAttribL4i64vNV(GLuint i, const GLint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    std::memcpy(d,v,4*sizeof(GLint64));
}
void glVertexAttribL1ui64NV(GLuint i, GLuint64 x) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=(GLint64)x; d[1]=0; d[2]=0; d[3]=1;
}
void glVertexAttribL1ui64vNV(GLuint i, const GLuint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=(GLint64)v[0]; d[1]=0; d[2]=0; d[3]=1;
}
void glVertexAttribL2ui64NV(GLuint i, GLuint64 x, GLuint64 y) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=(GLint64)x; d[1]=(GLint64)y; d[2]=0; d[3]=1;
}
void glVertexAttribL2ui64vNV(GLuint i, const GLuint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=(GLint64)v[0]; d[1]=(GLint64)v[1]; d[2]=0; d[3]=1;
}
void glVertexAttribL3ui64NV(GLuint i, GLuint64 x, GLuint64 y, GLuint64 z) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=(GLint64)x; d[1]=(GLint64)y; d[2]=(GLint64)z; d[3]=1;
}
void glVertexAttribL3ui64vNV(GLuint i, const GLuint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    d[0]=(GLint64)v[0]; d[1]=(GLint64)v[1]; d[2]=(GLint64)v[2]; d[3]=1;
}
void glVertexAttribL4ui64NV(GLuint i, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d) return;
    d[0]=(GLint64)x; d[1]=(GLint64)y; d[2]=(GLint64)z; d[3]=(GLint64)w;
}
void glVertexAttribL4ui64vNV(GLuint i, const GLuint64* v) {
    Context& c = Context::Current(); GLint64* d = Cur64(c,i); if (!d||!v) return;
    std::memcpy(d,v,4*sizeof(GLuint64));
}

static void Get64(GLuint i, GLenum p, GLint64* out, bool asUnsigned) {
    (void)asUnsigned;
    Context& c = Context::Current();
    if (i >= 16 || !out) { c.errors.Record(0x0501); return; }
    if (p == 0x8626) { // CURRENT_VERTEX_ATTRIB
        auto it = c.attrib64.find(i);
        if (it != c.attrib64.end()) {
            std::memcpy(out, it->second.data(), 4*sizeof(GLint64));
        } else {
            double d[4] = {0,0,0,1};
            c.state.GetShadow(0x8626, d, sizeof(d), i);
            for (int k=0;k<4;++k) out[k] = (GLint64)d[k];
        }
        return;
    }
    GLint x = 0;
    tglmt::gl::glGetVertexAttribiv(i, p, &x);
    out[0] = (GLint64)x;
}
void glGetVertexAttribLi64vNV(GLuint i, GLenum p, GLint64* v) { Get64(i,p,v,false); }
void glGetVertexAttribLui64vNV(GLuint i, GLenum p, GLuint64* v) { Get64(i,p,(GLint64*)v,true); }

void glVertexAttribLFormatNV(GLuint i, GLint size, GLenum type, GLsizei stride) {
    Context& c = Context::Current();
    if (i >= 16) { c.errors.Record(0x0501); return; }
    if (size < 1 || size > 4 || (type != 0x140E && type != 0x140F) || stride < 0) {
        c.errors.Record(0x0500); return;
    }
    GLuint va = c.state.BoundVAO();
    auto it = c.vaos.find(va);
    if (it == c.vaos.end()) { c.errors.Record(0x0502); return; }
    auto& a = it->second.attribs[i];
    a.size=size; a.type=type; a.stride=stride; a.offset=0; a.relativeOffset=0;
    a.buffer=c.state.BoundBuffer(0x8892); a.normalized=0; a.isInt=false; a.isLong=true;
    if (a.binding < it->second.bindings.size()) {
        it->second.bindings[a.binding].buffer = a.buffer;
        it->second.bindings[a.binding].offset = 0;
        it->second.bindings[a.binding].stride = stride;
    }
    VAOSyncAttribOffset(it->second, i);
}

} // extern "C"
