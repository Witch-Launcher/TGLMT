// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.
// 349 entry point OpenGL 1.0–2.1 chỉ có ở COMPATIBILITY Profile
// (fixed-function: immediate mode, matrix stack, lighting, fog, texenv,
// display lists, evaluators, selection/feedback, pixel ops).
// Core Profile KHÔNG có các entry point này ⇒ version-gate chặn ở 4.6 Core.
#pragma once
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy_types.h"

namespace tglmt { namespace gl {
void glAccum(GLenum op, GLfloat value); // COMPAT-ONLY, introduced 1.0
void glAlphaFunc(GLenum func, GLfloat ref); // COMPAT-ONLY, introduced 1.0
GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences); // COMPAT-ONLY, introduced 1.1
void glArrayElement(GLint i); // COMPAT-ONLY, introduced 1.1
void glBegin(GLenum mode); // COMPAT-ONLY, introduced 1.0
void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap); // COMPAT-ONLY, introduced 1.0
void glCallList(GLuint list); // COMPAT-ONLY, introduced 1.0
void glCallLists(GLsizei n, GLenum type, const void *lists); // COMPAT-ONLY, introduced 1.0
void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // COMPAT-ONLY, introduced 1.0
void glClearIndex(GLfloat c); // COMPAT-ONLY, introduced 1.0
void glClientActiveTexture(GLenum texture); // COMPAT-ONLY, introduced 1.3
void glClipPlane(GLenum plane, const GLdouble *equation); // COMPAT-ONLY, introduced 1.0
void glColor3b(GLbyte red, GLbyte green, GLbyte blue); // COMPAT-ONLY, introduced 1.0
void glColor3bv(const GLbyte *v); // COMPAT-ONLY, introduced 1.0
void glColor3d(GLdouble red, GLdouble green, GLdouble blue); // COMPAT-ONLY, introduced 1.0
void glColor3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glColor3f(GLfloat red, GLfloat green, GLfloat blue); // COMPAT-ONLY, introduced 1.0
void glColor3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glColor3i(GLint red, GLint green, GLint blue); // COMPAT-ONLY, introduced 1.0
void glColor3iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glColor3s(GLshort red, GLshort green, GLshort blue); // COMPAT-ONLY, introduced 1.0
void glColor3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glColor3ub(GLubyte red, GLubyte green, GLubyte blue); // COMPAT-ONLY, introduced 1.0
void glColor3ubv(const GLubyte *v); // COMPAT-ONLY, introduced 1.0
void glColor3ui(GLuint red, GLuint green, GLuint blue); // COMPAT-ONLY, introduced 1.0
void glColor3uiv(const GLuint *v); // COMPAT-ONLY, introduced 1.0
void glColor3us(GLushort red, GLushort green, GLushort blue); // COMPAT-ONLY, introduced 1.0
void glColor3usv(const GLushort *v); // COMPAT-ONLY, introduced 1.0
void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha); // COMPAT-ONLY, introduced 1.0
void glColor4bv(const GLbyte *v); // COMPAT-ONLY, introduced 1.0
void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha); // COMPAT-ONLY, introduced 1.0
void glColor4dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // COMPAT-ONLY, introduced 1.0
void glColor4fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glColor4i(GLint red, GLint green, GLint blue, GLint alpha); // COMPAT-ONLY, introduced 1.0
void glColor4iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha); // COMPAT-ONLY, introduced 1.0
void glColor4sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha); // COMPAT-ONLY, introduced 1.0
void glColor4ubv(const GLubyte *v); // COMPAT-ONLY, introduced 1.0
void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha); // COMPAT-ONLY, introduced 1.0
void glColor4uiv(const GLuint *v); // COMPAT-ONLY, introduced 1.0
void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha); // COMPAT-ONLY, introduced 1.0
void glColor4usv(const GLushort *v); // COMPAT-ONLY, introduced 1.0
void glColorMaterial(GLenum face, GLenum mode); // COMPAT-ONLY, introduced 1.0
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type); // COMPAT-ONLY, introduced 1.0
void glDeleteLists(GLuint list, GLsizei range); // COMPAT-ONLY, introduced 1.0
void glDisableClientState(GLenum array); // COMPAT-ONLY, introduced 1.1
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels); // COMPAT-ONLY, introduced 1.0
void glEdgeFlag(GLboolean flag); // COMPAT-ONLY, introduced 1.0
void glEdgeFlagPointer(GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glEdgeFlagv(const GLboolean *flag); // COMPAT-ONLY, introduced 1.0
void glEnableClientState(GLenum array); // COMPAT-ONLY, introduced 1.1
void glEnd(void); // COMPAT-ONLY, introduced 1.0
void glEndList(void); // COMPAT-ONLY, introduced 1.0
void glEvalCoord1d(GLdouble u); // COMPAT-ONLY, introduced 1.0
void glEvalCoord1dv(const GLdouble *u); // COMPAT-ONLY, introduced 1.0
void glEvalCoord1f(GLfloat u); // COMPAT-ONLY, introduced 1.0
void glEvalCoord1fv(const GLfloat *u); // COMPAT-ONLY, introduced 1.0
void glEvalCoord2d(GLdouble u, GLdouble v); // COMPAT-ONLY, introduced 1.0
void glEvalCoord2dv(const GLdouble *u); // COMPAT-ONLY, introduced 1.0
void glEvalCoord2f(GLfloat u, GLfloat v); // COMPAT-ONLY, introduced 1.0
void glEvalCoord2fv(const GLfloat *u); // COMPAT-ONLY, introduced 1.0
void glEvalMesh1(GLenum mode, GLint i1, GLint i2); // COMPAT-ONLY, introduced 1.0
void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2); // COMPAT-ONLY, introduced 1.0
void glEvalPoint1(GLint i); // COMPAT-ONLY, introduced 1.0
void glEvalPoint2(GLint i, GLint j); // COMPAT-ONLY, introduced 1.0
void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer); // COMPAT-ONLY, introduced 1.0
void glFogCoordPointer(GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.4
void glFogCoordd(GLdouble coord); // COMPAT-ONLY, introduced 1.4
void glFogCoorddv(const GLdouble *coord); // COMPAT-ONLY, introduced 1.4
void glFogCoordf(GLfloat coord); // COMPAT-ONLY, introduced 1.4
void glFogCoordfv(const GLfloat *coord); // COMPAT-ONLY, introduced 1.4
void glFogf(GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glFogfv(GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glFogi(GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glFogiv(GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); // COMPAT-ONLY, introduced 1.0
GLuint glGenLists(GLsizei range); // COMPAT-ONLY, introduced 1.0
void glGetClipPlane(GLenum plane, GLdouble *equation); // COMPAT-ONLY, introduced 1.0
void glGetLightfv(GLenum light, GLenum pname, GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glGetLightiv(GLenum light, GLenum pname, GLint *params); // COMPAT-ONLY, introduced 1.0
void glGetMapdv(GLenum target, GLenum query, GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glGetMapfv(GLenum target, GLenum query, GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glGetMapiv(GLenum target, GLenum query, GLint *v); // COMPAT-ONLY, introduced 1.0
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glGetMaterialiv(GLenum face, GLenum pname, GLint *params); // COMPAT-ONLY, introduced 1.0
void glGetPixelMapfv(GLenum map, GLfloat *values); // COMPAT-ONLY, introduced 1.0
void glGetPixelMapuiv(GLenum map, GLuint *values); // COMPAT-ONLY, introduced 1.0
void glGetPixelMapusv(GLenum map, GLushort *values); // COMPAT-ONLY, introduced 1.0
void glGetPolygonStipple(GLubyte *mask); // COMPAT-ONLY, introduced 1.0
void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glGetTexEnviv(GLenum target, GLenum pname, GLint *params); // COMPAT-ONLY, introduced 1.0
void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params); // COMPAT-ONLY, introduced 1.0
void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params); // COMPAT-ONLY, introduced 1.0
void glIndexMask(GLuint mask); // COMPAT-ONLY, introduced 1.0
void glIndexPointer(GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glIndexd(GLdouble c); // COMPAT-ONLY, introduced 1.0
void glIndexdv(const GLdouble *c); // COMPAT-ONLY, introduced 1.0
void glIndexf(GLfloat c); // COMPAT-ONLY, introduced 1.0
void glIndexfv(const GLfloat *c); // COMPAT-ONLY, introduced 1.0
void glIndexi(GLint c); // COMPAT-ONLY, introduced 1.0
void glIndexiv(const GLint *c); // COMPAT-ONLY, introduced 1.0
void glIndexs(GLshort c); // COMPAT-ONLY, introduced 1.0
void glIndexsv(const GLshort *c); // COMPAT-ONLY, introduced 1.0
void glIndexub(GLubyte c); // COMPAT-ONLY, introduced 1.1
void glIndexubv(const GLubyte *c); // COMPAT-ONLY, introduced 1.1
void glInitNames(void); // COMPAT-ONLY, introduced 1.0
void glInterleavedArrays(GLenum format, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
GLboolean glIsList(GLuint list); // COMPAT-ONLY, introduced 1.0
void glLightModelf(GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glLightModelfv(GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glLightModeli(GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glLightModeliv(GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glLightf(GLenum light, GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glLightfv(GLenum light, GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glLighti(GLenum light, GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glLightiv(GLenum light, GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glLineStipple(GLint factor, GLushort pattern); // COMPAT-ONLY, introduced 1.0
void glListBase(GLuint base); // COMPAT-ONLY, introduced 1.0
void glLoadIdentity(void); // COMPAT-ONLY, introduced 1.0
void glLoadMatrixd(const GLdouble *m); // COMPAT-ONLY, introduced 1.0
void glLoadMatrixf(const GLfloat *m); // COMPAT-ONLY, introduced 1.0
void glLoadName(GLuint name); // COMPAT-ONLY, introduced 1.0
void glLoadTransposeMatrixd(const GLdouble *m); // COMPAT-ONLY, introduced 1.3
void glLoadTransposeMatrixf(const GLfloat *m); // COMPAT-ONLY, introduced 1.3
void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points); // COMPAT-ONLY, introduced 1.0
void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points); // COMPAT-ONLY, introduced 1.0
void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points); // COMPAT-ONLY, introduced 1.0
void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points); // COMPAT-ONLY, introduced 1.0
void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2); // COMPAT-ONLY, introduced 1.0
void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2); // COMPAT-ONLY, introduced 1.0
void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2); // COMPAT-ONLY, introduced 1.0
void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2); // COMPAT-ONLY, introduced 1.0
void glMaterialf(GLenum face, GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glMateriali(GLenum face, GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glMaterialiv(GLenum face, GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glMatrixMode(GLenum mode); // COMPAT-ONLY, introduced 1.0
void glMultMatrixd(const GLdouble *m); // COMPAT-ONLY, introduced 1.0
void glMultMatrixf(const GLfloat *m); // COMPAT-ONLY, introduced 1.0
void glMultTransposeMatrixd(const GLdouble *m); // COMPAT-ONLY, introduced 1.3
void glMultTransposeMatrixf(const GLfloat *m); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1d(GLenum target, GLdouble s); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1dv(GLenum target, const GLdouble *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1f(GLenum target, GLfloat s); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1fv(GLenum target, const GLfloat *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1i(GLenum target, GLint s); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1iv(GLenum target, const GLint *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1s(GLenum target, GLshort s); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord1sv(GLenum target, const GLshort *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2d(GLenum target, GLdouble s, GLdouble t); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2dv(GLenum target, const GLdouble *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2fv(GLenum target, const GLfloat *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2i(GLenum target, GLint s, GLint t); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2iv(GLenum target, const GLint *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2s(GLenum target, GLshort s, GLshort t); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord2sv(GLenum target, const GLshort *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3d(GLenum target, GLdouble s, GLdouble t, GLdouble r); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3dv(GLenum target, const GLdouble *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3f(GLenum target, GLfloat s, GLfloat t, GLfloat r); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3fv(GLenum target, const GLfloat *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3i(GLenum target, GLint s, GLint t, GLint r); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3iv(GLenum target, const GLint *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3s(GLenum target, GLshort s, GLshort t, GLshort r); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord3sv(GLenum target, const GLshort *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4d(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4dv(GLenum target, const GLdouble *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4fv(GLenum target, const GLfloat *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4i(GLenum target, GLint s, GLint t, GLint r, GLint q); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4iv(GLenum target, const GLint *v); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4s(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q); // COMPAT-ONLY, introduced 1.3
void glMultiTexCoord4sv(GLenum target, const GLshort *v); // COMPAT-ONLY, introduced 1.3
void glNewList(GLuint list, GLenum mode); // COMPAT-ONLY, introduced 1.0
void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz); // COMPAT-ONLY, introduced 1.0
void glNormal3bv(const GLbyte *v); // COMPAT-ONLY, introduced 1.0
void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz); // COMPAT-ONLY, introduced 1.0
void glNormal3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz); // COMPAT-ONLY, introduced 1.0
void glNormal3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glNormal3i(GLint nx, GLint ny, GLint nz); // COMPAT-ONLY, introduced 1.0
void glNormal3iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glNormal3s(GLshort nx, GLshort ny, GLshort nz); // COMPAT-ONLY, introduced 1.0
void glNormal3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glNormalPointer(GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); // COMPAT-ONLY, introduced 1.0
void glPassThrough(GLfloat token); // COMPAT-ONLY, introduced 1.0
void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values); // COMPAT-ONLY, introduced 1.0
void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values); // COMPAT-ONLY, introduced 1.0
void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values); // COMPAT-ONLY, introduced 1.0
void glPixelTransferf(GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glPixelTransferi(GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glPixelZoom(GLfloat xfactor, GLfloat yfactor); // COMPAT-ONLY, introduced 1.0
void glPolygonStipple(const GLubyte *mask); // COMPAT-ONLY, introduced 1.0
void glPopAttrib(void); // COMPAT-ONLY, introduced 1.0
void glPopClientAttrib(void); // COMPAT-ONLY, introduced 1.1
void glPopMatrix(void); // COMPAT-ONLY, introduced 1.0
void glPopName(void); // COMPAT-ONLY, introduced 1.0
void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLfloat *priorities); // COMPAT-ONLY, introduced 1.1
void glPushAttrib(GLbitfield mask); // COMPAT-ONLY, introduced 1.0
void glPushClientAttrib(GLbitfield mask); // COMPAT-ONLY, introduced 1.1
void glPushMatrix(void); // COMPAT-ONLY, introduced 1.0
void glPushName(GLuint name); // COMPAT-ONLY, introduced 1.0
void glRasterPos2d(GLdouble x, GLdouble y); // COMPAT-ONLY, introduced 1.0
void glRasterPos2dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos2f(GLfloat x, GLfloat y); // COMPAT-ONLY, introduced 1.0
void glRasterPos2fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos2i(GLint x, GLint y); // COMPAT-ONLY, introduced 1.0
void glRasterPos2iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos2s(GLshort x, GLshort y); // COMPAT-ONLY, introduced 1.0
void glRasterPos2sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.0
void glRasterPos3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.0
void glRasterPos3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos3i(GLint x, GLint y, GLint z); // COMPAT-ONLY, introduced 1.0
void glRasterPos3iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos3s(GLshort x, GLshort y, GLshort z); // COMPAT-ONLY, introduced 1.0
void glRasterPos3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w); // COMPAT-ONLY, introduced 1.0
void glRasterPos4dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w); // COMPAT-ONLY, introduced 1.0
void glRasterPos4fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos4i(GLint x, GLint y, GLint z, GLint w); // COMPAT-ONLY, introduced 1.0
void glRasterPos4iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w); // COMPAT-ONLY, introduced 1.0
void glRasterPos4sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2); // COMPAT-ONLY, introduced 1.0
void glRectdv(const GLdouble *v1, const GLdouble *v2); // COMPAT-ONLY, introduced 1.0
void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2); // COMPAT-ONLY, introduced 1.0
void glRectfv(const GLfloat *v1, const GLfloat *v2); // COMPAT-ONLY, introduced 1.0
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2); // COMPAT-ONLY, introduced 1.0
void glRectiv(const GLint *v1, const GLint *v2); // COMPAT-ONLY, introduced 1.0
void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2); // COMPAT-ONLY, introduced 1.0
void glRectsv(const GLshort *v1, const GLshort *v2); // COMPAT-ONLY, introduced 1.0
GLint glRenderMode(GLenum mode); // COMPAT-ONLY, introduced 1.0
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.0
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.0
void glScaled(GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.0
void glScalef(GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.0
void glSecondaryColor3b(GLbyte red, GLbyte green, GLbyte blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3bv(const GLbyte *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3d(GLdouble red, GLdouble green, GLdouble blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3f(GLfloat red, GLfloat green, GLfloat blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3i(GLint red, GLint green, GLint blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3iv(const GLint *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3s(GLshort red, GLshort green, GLshort blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3ub(GLubyte red, GLubyte green, GLubyte blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3ubv(const GLubyte *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3ui(GLuint red, GLuint green, GLuint blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3uiv(const GLuint *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3us(GLushort red, GLushort green, GLushort blue); // COMPAT-ONLY, introduced 1.4
void glSecondaryColor3usv(const GLushort *v); // COMPAT-ONLY, introduced 1.4
void glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.4
void glSelectBuffer(GLsizei size, GLuint *buffer); // COMPAT-ONLY, introduced 1.0
void glShadeModel(GLenum mode); // COMPAT-ONLY, introduced 1.0
void glTexCoord1d(GLdouble s); // COMPAT-ONLY, introduced 1.0
void glTexCoord1dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord1f(GLfloat s); // COMPAT-ONLY, introduced 1.0
void glTexCoord1fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord1i(GLint s); // COMPAT-ONLY, introduced 1.0
void glTexCoord1iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord1s(GLshort s); // COMPAT-ONLY, introduced 1.0
void glTexCoord1sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord2d(GLdouble s, GLdouble t); // COMPAT-ONLY, introduced 1.0
void glTexCoord2dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord2f(GLfloat s, GLfloat t); // COMPAT-ONLY, introduced 1.0
void glTexCoord2fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord2i(GLint s, GLint t); // COMPAT-ONLY, introduced 1.0
void glTexCoord2iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord2s(GLshort s, GLshort t); // COMPAT-ONLY, introduced 1.0
void glTexCoord2sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r); // COMPAT-ONLY, introduced 1.0
void glTexCoord3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r); // COMPAT-ONLY, introduced 1.0
void glTexCoord3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord3i(GLint s, GLint t, GLint r); // COMPAT-ONLY, introduced 1.0
void glTexCoord3iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord3s(GLshort s, GLshort t, GLshort r); // COMPAT-ONLY, introduced 1.0
void glTexCoord3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q); // COMPAT-ONLY, introduced 1.0
void glTexCoord4dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q); // COMPAT-ONLY, introduced 1.0
void glTexCoord4fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord4i(GLint s, GLint t, GLint r, GLint q); // COMPAT-ONLY, introduced 1.0
void glTexCoord4iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q); // COMPAT-ONLY, introduced 1.0
void glTexCoord4sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glTexEnvf(GLenum target, GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glTexEnvi(GLenum target, GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glTexEnviv(GLenum target, GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glTexGend(GLenum coord, GLenum pname, GLdouble param); // COMPAT-ONLY, introduced 1.0
void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params); // COMPAT-ONLY, introduced 1.0
void glTexGenf(GLenum coord, GLenum pname, GLfloat param); // COMPAT-ONLY, introduced 1.0
void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params); // COMPAT-ONLY, introduced 1.0
void glTexGeni(GLenum coord, GLenum pname, GLint param); // COMPAT-ONLY, introduced 1.0
void glTexGeniv(GLenum coord, GLenum pname, const GLint *params); // COMPAT-ONLY, introduced 1.0
void glTranslated(GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.0
void glTranslatef(GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.0
void glVertex2d(GLdouble x, GLdouble y); // COMPAT-ONLY, introduced 1.0
void glVertex2dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glVertex2f(GLfloat x, GLfloat y); // COMPAT-ONLY, introduced 1.0
void glVertex2fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glVertex2i(GLint x, GLint y); // COMPAT-ONLY, introduced 1.0
void glVertex2iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glVertex2s(GLshort x, GLshort y); // COMPAT-ONLY, introduced 1.0
void glVertex2sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glVertex3d(GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.0
void glVertex3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glVertex3f(GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.0
void glVertex3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glVertex3i(GLint x, GLint y, GLint z); // COMPAT-ONLY, introduced 1.0
void glVertex3iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glVertex3s(GLshort x, GLshort y, GLshort z); // COMPAT-ONLY, introduced 1.0
void glVertex3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w); // COMPAT-ONLY, introduced 1.0
void glVertex4dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.0
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w); // COMPAT-ONLY, introduced 1.0
void glVertex4fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.0
void glVertex4i(GLint x, GLint y, GLint z, GLint w); // COMPAT-ONLY, introduced 1.0
void glVertex4iv(const GLint *v); // COMPAT-ONLY, introduced 1.0
void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w); // COMPAT-ONLY, introduced 1.0
void glVertex4sv(const GLshort *v); // COMPAT-ONLY, introduced 1.0
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer); // COMPAT-ONLY, introduced 1.1
void glWindowPos2d(GLdouble x, GLdouble y); // COMPAT-ONLY, introduced 1.4
void glWindowPos2dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos2f(GLfloat x, GLfloat y); // COMPAT-ONLY, introduced 1.4
void glWindowPos2fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos2i(GLint x, GLint y); // COMPAT-ONLY, introduced 1.4
void glWindowPos2iv(const GLint *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos2s(GLshort x, GLshort y); // COMPAT-ONLY, introduced 1.4
void glWindowPos2sv(const GLshort *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos3d(GLdouble x, GLdouble y, GLdouble z); // COMPAT-ONLY, introduced 1.4
void glWindowPos3dv(const GLdouble *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos3f(GLfloat x, GLfloat y, GLfloat z); // COMPAT-ONLY, introduced 1.4
void glWindowPos3fv(const GLfloat *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos3i(GLint x, GLint y, GLint z); // COMPAT-ONLY, introduced 1.4
void glWindowPos3iv(const GLint *v); // COMPAT-ONLY, introduced 1.4
void glWindowPos3s(GLshort x, GLshort y, GLshort z); // COMPAT-ONLY, introduced 1.4
void glWindowPos3sv(const GLshort *v); // COMPAT-ONLY, introduced 1.4
} // namespace gl
} // namespace tglmt
