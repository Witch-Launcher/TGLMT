// gl_ext_dsa_texture.cpp — EXT_direct_state_access texture entries.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

using namespace tglmt;

namespace {
struct TexGuard {
    Context& c;
    GLenum target;
    GLuint oldActive;
    GLuint oldTex;
    TexGuard(GLuint unit, GLenum t, GLuint tex) : c(Context::Current()), target(t) {
        oldActive = c.state.ActiveTexture();
        c.state.SetActiveTexture(unit);
        oldTex = c.state.BoundTexture(unit, t);
        tglmt::gl::glBindTexture(t, tex);
    }
    ~TexGuard() {
        tglmt::gl::glBindTexture(target, oldTex);
        c.state.SetActiveTexture(oldActive);
    }
};
static bool Unit(GLenum texunit, GLuint& u) {
    if (texunit < 0x84C0 || texunit >= 0x84C0 + 32) {
        Context::Current().errors.Record(0x0500); return false;
    }
    u = texunit - 0x84C0;
    return true;
}
} // namespace

extern "C" {

void glBindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture) {
    GLuint u = 0;
    if (!Unit(texunit, u)) return;
    Context& c = Context::Current();
    GLuint old = c.state.ActiveTexture();
    c.state.SetActiveTexture(u);
    tglmt::gl::glBindTexture(target, texture);
    c.state.SetActiveTexture(old);
}

void glTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
void glTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void glTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}
void glMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) {
    GLuint u = 0; if (!Unit(texunit, u)) return;
    TexGuard g(u, target, Context::Current().state.BoundTexture(u, target));
    tglmt::gl::glTexImage1D(target, level, internalformat, width, border, format, type, pixels);
}
void glMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
    GLuint u = 0; if (!Unit(texunit, u)) return;
    TexGuard g(u, target, Context::Current().state.BoundTexture(u, target));
    tglmt::gl::glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}
void glMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) {
    GLuint u = 0; if (!Unit(texunit, u)) return;
    TexGuard g(u, target, Context::Current().state.BoundTexture(u, target));
    tglmt::gl::glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
}

void glTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLenum type, const void* pixels) {
    (void)target; tglmt::gl::glTextureSubImage1D(texture, level, xoffset, width, format, type, pixels);
}
void glTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
    (void)target; tglmt::gl::glTextureSubImage2D(texture, level, xoffset, yoffset, width, height, format, type, pixels);
}
void glTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    (void)target; tglmt::gl::glTextureSubImage3D(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}
void glMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLenum type, const void* pixels) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexSubImage1D(target,level,xoffset,width,format,type,pixels);
}
void glMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,pixels);
}
void glMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void* pixels) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexSubImage3D(target,level,xoffset,yoffset,zoffset,width,height,depth,format,type,pixels);
}

void glCompressedTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLint border, GLsizei imageSize, const void* bits) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glCompressedTexImage1D(target, level, internalformat, width, border, imageSize, bits);
}
void glCompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* bits) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, bits);
}
void glCompressedTextureImage3DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* bits) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, bits);
}
void glCompressedMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLint border, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexImage1D(target,level,internalformat,width,border,imageSize,bits);
}
void glCompressedMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexImage2D(target,level,internalformat,width,height,border,imageSize,bits);
}
void glCompressedMultiTexImage3DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexImage3D(target,level,internalformat,width,height,depth,border,imageSize,bits);
}
void glCompressedTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLsizei imageSize, const void* bits) {
    (void)target; tglmt::gl::glCompressedTextureSubImage1D(texture,level,xoffset,width,format,imageSize,bits);
}
void glCompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* bits) {
    (void)target; tglmt::gl::glCompressedTextureSubImage2D(texture,level,xoffset,yoffset,width,height,format,imageSize,bits);
}
void glCompressedTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* bits) {
    (void)target; tglmt::gl::glCompressedTextureSubImage3D(texture,level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,bits);
}
void glCompressedMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLsizei width, GLenum format, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexSubImage1D(target,level,xoffset,width,format,imageSize,bits);
}
void glCompressedMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexSubImage2D(target,level,xoffset,yoffset,width,height,format,imageSize,bits);
}
void glCompressedMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* bits) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCompressedTexSubImage3D(target,level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,bits);
}

void glCopyTextureImage1DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLint border) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glCopyTexImage1D(target, level, internalformat, x, y, width, border);
}
void glCopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}
void glCopyMultiTexImage1DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLint border) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCopyTexImage1D(target,level,internalformat,x,y,width,border);
}
void glCopyMultiTexImage2DEXT(GLenum texunit, GLenum target, GLint level, GLenum internalformat,
        GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCopyTexImage2D(target,level,internalformat,x,y,width,height,border);
}
void glCopyTextureSubImage1DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset,
        GLint x, GLint y, GLsizei width) {
    (void)target; tglmt::gl::glCopyTextureSubImage1D(texture,level,xoffset,x,y,width);
}
void glCopyTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)target; tglmt::gl::glCopyTextureSubImage2D(texture,level,xoffset,yoffset,x,y,width,height);
}
void glCopyTextureSubImage3DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLint x, GLint y, GLsizei width, GLsizei height) {
    (void)target; tglmt::gl::glCopyTextureSubImage3D(texture,level,xoffset,yoffset,zoffset,x,y,width,height);
}
void glCopyMultiTexSubImage1DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset,
        GLint x, GLint y, GLsizei width) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCopyTexSubImage1D(target,level,xoffset,x,y,width);
}
void glCopyMultiTexSubImage2DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset,
        GLint x, GLint y, GLsizei width, GLsizei height) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCopyTexSubImage2D(target,level,xoffset,yoffset,x,y,width,height);
}
void glCopyMultiTexSubImage3DEXT(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLint x, GLint y, GLsizei width, GLsizei height) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glCopyTexSubImage3D(target,level,xoffset,yoffset,zoffset,x,y,width,height);
}

void glTextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param) {
    (void)target; tglmt::gl::glTextureParameterf(texture,pname,param);
}
void glTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, const GLfloat* params) {
    (void)target; tglmt::gl::glTextureParameterfv(texture,pname,params);
}
void glTextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param) {
    (void)target; tglmt::gl::glTextureParameteri(texture,pname,param);
}
void glTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* params) {
    (void)target; tglmt::gl::glTextureParameteriv(texture,pname,params);
}
void glTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, const GLint* params) {
    (void)target; tglmt::gl::glTextureParameterIiv(texture,pname,params);
}
void glTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, const GLuint* params) {
    (void)target; tglmt::gl::glTextureParameterIuiv(texture,pname,params);
}
void glMultiTexParameterfEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat param) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameterf(target,pname,param);
}
void glMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, const GLfloat* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameterfv(target,pname,params);
}
void glMultiTexParameteriEXT(GLenum texunit, GLenum target, GLenum pname, GLint param) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameteri(target,pname,param);
}
void glMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameteriv(target,pname,params);
}
void glMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, const GLint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameterIiv(target,pname,params);
}
void glMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, const GLuint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexParameterIuiv(target,pname,params);
}

void glGetTextureImageEXT(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void* pixels) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glGetTexImage(target,level,format,type,pixels);
}
void glGetMultiTexImageEXT(GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void* pixels) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexImage(target,level,format,type,pixels);
}
void glGetCompressedTextureImageEXT(GLuint texture, GLenum target, GLint lod, void* img) {
    TexGuard g(Context::Current().state.ActiveTexture(), target, texture);
    tglmt::gl::glGetCompressedTexImage(target,lod,img);
}
void glGetCompressedMultiTexImageEXT(GLenum texunit, GLenum target, GLint lod, void* img) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetCompressedTexImage(target,lod,img);
}
void glGetTextureLevelParameterfvEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat* params) {
    (void)target; tglmt::gl::glGetTextureLevelParameterfv(texture,level,pname,params);
}
void glGetTextureLevelParameterivEXT(GLuint texture, GLenum target, GLint level, GLenum pname, GLint* params) {
    (void)target; tglmt::gl::glGetTextureLevelParameteriv(texture,level,pname,params);
}
void glGetMultiTexLevelParameterfvEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexLevelParameterfv(target,level,pname,params);
}
void glGetMultiTexLevelParameterivEXT(GLenum texunit, GLenum target, GLint level, GLenum pname, GLint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexLevelParameteriv(target,level,pname,params);
}
void glGetTextureParameterfvEXT(GLuint texture, GLenum target, GLenum pname, GLfloat* params) {
    (void)target; tglmt::gl::glGetTextureParameterfv(texture,pname,params);
}
void glGetTextureParameterivEXT(GLuint texture, GLenum target, GLenum pname, GLint* params) {
    (void)target; tglmt::gl::glGetTextureParameteriv(texture,pname,params);
}
void glGetTextureParameterIivEXT(GLuint texture, GLenum target, GLenum pname, GLint* params) {
    (void)target; tglmt::gl::glGetTextureParameterIiv(texture,pname,params);
}
void glGetTextureParameterIuivEXT(GLuint texture, GLenum target, GLenum pname, GLuint* params) {
    (void)target; tglmt::gl::glGetTextureParameterIuiv(texture,pname,params);
}
void glGetMultiTexParameterfvEXT(GLenum texunit, GLenum target, GLenum pname, GLfloat* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexParameterfv(target,pname,params);
}
void glGetMultiTexParameterivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexParameteriv(target,pname,params);
}
void glGetMultiTexParameterIivEXT(GLenum texunit, GLenum target, GLenum pname, GLint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexParameterIiv(target,pname,params);
}
void glGetMultiTexParameterIuivEXT(GLenum texunit, GLenum target, GLenum pname, GLuint* params) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGetTexParameterIuiv(target,pname,params);
}

void glGenerateTextureMipmapEXT(GLuint texture, GLenum target) {
    (void)target; tglmt::gl::glGenerateTextureMipmap(texture);
}
void glGenerateMultiTexMipmapEXT(GLenum texunit, GLenum target) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glGenerateMipmap(target);
}
void glTextureBufferEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer) {
    (void)target; tglmt::gl::glTextureBuffer(texture,internalformat,buffer);
}
void glTextureBufferRangeEXT(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer,
        GLintptr offset, GLsizeiptr size) {
    (void)target; tglmt::gl::glTextureBufferRange(texture,internalformat,buffer,offset,size);
}
void glMultiTexBufferEXT(GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer) {
    GLuint u=0; if(!Unit(texunit,u)) return;
    TexGuard g(u,target,Context::Current().state.BoundTexture(u,target));
    tglmt::gl::glTexBuffer(target,internalformat,buffer);
}
void glTextureStorage2DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    (void)target; tglmt::gl::glTextureStorage2DMultisample(texture,samples,internalformat,width,height,fixedsamplelocations);
}
void glTextureStorage3DMultisampleEXT(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat,
        GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
    (void)target; tglmt::gl::glTextureStorage3DMultisample(texture,samples,internalformat,width,height,depth,fixedsamplelocations);
}
void glTexturePageCommitmentEXT(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
        GLsizei width, GLsizei height, GLsizei depth, GLboolean commit) {
    Context& c=Context::Current();
    auto it=c.textures.find(texture);
    if(it==c.textures.end()){c.errors.Record(0x0502);return;}
    auto&t=it->second;
    if(level<0||xoffset<0||yoffset<0||zoffset<0||width<0||height<0||depth<0||
       (size_t)(xoffset+width)>t.w||(size_t)(yoffset+height)>t.h||
       (size_t)(zoffset+depth)>std::max<uint32_t>(t.d,1)){c.errors.Record(0x0501);return;}
    (void)commit;
}
void glTextureRenderbufferEXT(GLuint texture, GLenum target, GLuint renderbuffer) {
    (void)texture;(void)target;(void)renderbuffer;
    Context::Current().errors.Record(0x0502); // no texture-renderbuffer attachment on Metal
}
void glMultiTexRenderbufferEXT(GLenum texunit, GLenum target, GLuint renderbuffer) {
    (void)texunit;(void)target;(void)renderbuffer;
    Context::Current().errors.Record(0x0502);
}

} // extern "C"
