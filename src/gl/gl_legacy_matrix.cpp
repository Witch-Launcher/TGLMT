// gl_legacy_matrix.cpp — OpenGL fixed-function matrix stack (Compatibility).
//
// Spec: OpenGL 1.0–2.1 §"Transformations" + GL 4.6 Core spec App. E.2
// (Compatibility Profile giữ lại toàn bộ phần này). Ở Core Profile 3.2+ các
// entry point này KHÔNG tồn tại → version-gate trả GL_INVALID_OPERATION.
//
// Quy tắc quan trọng:
//  - Ma trận column-major 4x4 (m[0..3] = cột 0).
//  - glMultMatrix: current = current * M ⇒ M biến đổi TRƯỚC.
//  - glLoadTransposeMatrix: đọc theo hàng (row-major) ⇒ transpose khi nạp.
//  - glOrtho/glFrustum chỉ hợp lệ ở matrix mode = GL_PROJECTION, nếu không
//    ⇒ GL_INVALID_OPERATION (spec).
//  - Push/Pop vượt độ sâu ⇒ GL_STACK_OVERFLOW / GL_STACK_UNDERFLOW.
#include "tglmt/gl46.h"
#include "tglmt/LegacyDisplayList.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt {
// ---- Triển khai GLMatrix::Rotate / Ortho / Frustum ------------------------
void GLMatrix::Rotate(float angleDeg, float x, float y, float z) {
    // Trục không chuẩn hoá: độ dài 0 ⇒ phép xoay là identity (spec: giá trị
    // của góc bị bỏ qua nếu vector chỉ định có độ dài 0).
    float len = std::sqrt(x * x + y * y + z * z);
    if (len == 0.0f) return;
    float nx = x / len, ny = y / len, nz = z / len;
    float rad = angleDeg * 3.14159265358979323846f / 180.0f;
    float s = std::sin(rad), co = std::cos(rad);
    float t = 1.0f - co;
    // Ma trận xoay column-major: cột 0 = (co + nx*nx*t, ny*nx*t + s*nz, ...)
    float r[16];
    r[0]  = co + nx * nx * t;
    r[1]  = ny * nx * t + s * nz;
    r[2]  = nz * nx * t - s * ny;
    r[3]  = 0.0f;
    r[4]  = nx * ny * t - s * nz;
    r[5]  = co + ny * ny * t;
    r[6]  = nz * ny * t + s * nx;
    r[7]  = 0.0f;
    r[8]  = nx * nz * t + s * ny;
    r[9]  = ny * nz * t - s * nx;
    r[10] = co + nz * nz * t;
    r[11] = 0.0f;
    r[12] = 0.0f; r[13] = 0.0f; r[14] = 0.0f; r[15] = 1.0f;
    Mult(r);
}

void GLMatrix::Ortho(double l, double r, double b, double t, double n, double f) {
    float rl = (float)(r - l), tb = (float)(t - b), fn = (float)(f - n);
    float m[16];
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0]  = 2.0f / rl;
    m[5]  = 2.0f / tb;
    m[10] = -2.0f / fn;
    m[12] = (float)(-(r + l) / rl);
    m[13] = (float)(-(t + b) / tb);
    m[14] = (float)(-(f + n) / fn);
    m[15] = 1.0f;
    Mult(m);
}

void GLMatrix::Frustum(double l, double r, double b, double t, double n, double f) {
    float rl = (float)(r - l), tb = (float)(t - b), nf = (float)(n - f);
    float m[16];
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0]  = (float)(2.0 * n / rl);
    m[5]  = (float)(2.0 * n / tb);
    m[8]  = (float)((r + l) / rl);
    m[9]  = (float)((t + b) / tb);
    m[10] = (float)((f + n) / nf);
    m[14] = (float)(2.0 * f * n / nf);
    m[15] = 1.0f;
    Mult(m);
}

}  // namespace tglmt

namespace tglmt::gl {

// ---- Trợ giúp -------------------------------------------------------------
static MatrixStack* StackForMode(GLenum mode) {
    Context& c = Context::Current();
    switch (mode) {
        case 0x1700 /*GL_MODELVIEW*/:  return &c.legacy.modelview;
        case 0x1701 /*GL_PROJECTION*/: return &c.legacy.projection;
        case 0x1702 /*GL_TEXTURE*/:     return &c.legacy.texture;
        case 0x180D /*GL_COLOR*/:       return &c.legacy.color;
        default: return nullptr;
    }
}

static bool ValidMatrixMode(GLenum mode) {
    return mode == 0x1700 || mode == 0x1701 || mode == 0x1702 || mode == 0x180D;
}

// ---- glMatrixMode ---------------------------------------------------------
void glMatrixMode(GLenum mode) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMatrixMode")) return;
    if (!ValidMatrixMode(mode)) { c.errors.Record(0x0500); return; }  // INVALID_ENUM
    if (LegacyRecordMatrixMode(mode)) return;
    c.legacy.matrixMode = mode;
}

// ---- glLoadIdentity -------------------------------------------------------
void glLoadIdentity() {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadIdentity")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordLoadIdentity()) return;
    s->top().LoadIdentity();
}

// ---- glLoadMatrixf/d ------------------------------------------------------
void glLoadMatrixf(const GLfloat* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadMatrixf")) return;
    if (!m) { c.errors.Record(0x0500); return; }  // NULL pointer → INVALID_VALUE
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordLoadMatrix(m, false)) return;
    s->top().Load(m);
}
void glLoadMatrixd(const GLdouble* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadMatrixd")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    float tmp[16];
    for (int i = 0; i < 16; ++i) tmp[i] = (float)m[i];
    if (LegacyRecordLoadMatrix(tmp, false)) return;
    s->top().Load(tmp);
}

// ---- glLoadTransposeMatrixf/d ---------------------------------------------
void glLoadTransposeMatrixf(const GLfloat* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadTransposeMatrixf")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordLoadMatrix(m, true)) return;
    s->top().LoadTranspose(m);
}
void glLoadTransposeMatrixd(const GLdouble* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadTransposeMatrixd")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    float tmp[16];
    for (int i = 0; i < 16; ++i) tmp[i] = (float)m[i];
    if (LegacyRecordLoadMatrix(tmp, true)) return;
    s->top().LoadTranspose(tmp);
}

// ---- glMultMatrixf/d ------------------------------------------------------
void glMultMatrixf(const GLfloat* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMultMatrixf")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordMultMatrix(m, false)) return;
    s->top().Mult(m);
}
void glMultMatrixd(const GLdouble* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMultMatrixd")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    float tmp[16];
    for (int i = 0; i < 16; ++i) tmp[i] = (float)m[i];
    if (LegacyRecordMultMatrix(tmp, false)) return;
    s->top().Mult(tmp);
}

// ---- glMultTransposeMatrixf/d ---------------------------------------------
void glMultTransposeMatrixf(const GLfloat* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMultTransposeMatrixf")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordMultMatrix(m, true)) return;
    s->top().MultTranspose(m);
}
void glMultTransposeMatrixd(const GLdouble* m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMultTransposeMatrixd")) return;
    if (!m) { c.errors.Record(0x0500); return; }
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    float tmp[16];
    for (int i = 0; i < 16; ++i) tmp[i] = (float)m[i];
    if (LegacyRecordMultMatrix(tmp, true)) return;
    s->top().MultTranspose(tmp);
}

// ---- glPushMatrix / glPopMatrix ------------------------------------------
void glPushMatrix() {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPushMatrix")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordPushMatrix()) return;
    if (!s->Push()) c.errors.Record(0x0503);  // GL_STACK_OVERFLOW
}
void glPopMatrix() {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPopMatrix")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    if (LegacyRecordPopMatrix()) return;
    if (!s->Pop()) c.errors.Record(0x0504);  // GL_STACK_UNDERFLOW
}

// ---- glTranslatef/d / glTranslatefv/dv ------------------------------------
void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTranslatef")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    s->top().Translate(x, y, z);
}
void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
    glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glTranslatefv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTranslatef(v[0], v[1], v[2]);
}
void glTranslatedv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTranslatef((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

// ---- glScalef/d / glScalefv/dv --------------------------------------------
void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glScalef")) return;
    if (LegacyCheckBeginEndAllowed(c, "glTranslatedv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glTranslatefv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glTranslated")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    s->top().Scale(x, y, z);
}
void glScaled(GLdouble x, GLdouble y, GLdouble z) {
    glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glScalefv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glScalef(v[0], v[1], v[2]);
}
void glScaledv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glScalef((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

// ---- glRotatef/d / glRotatefv/dv -----------------------------------------
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glRotatef")) return;
    if (LegacyCheckBeginEndAllowed(c, "glScaledv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glScalefv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glScaled")) return;
    MatrixStack* s = StackForMode(c.legacy.matrixMode);
    if (!s) { c.errors.Record(0x0500); return; }
    s->top().Rotate(angle, x, y, z);
}
void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glRotatefv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glRotatef(v[0], v[1], v[2], v[3]);
}
void glRotatedv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glRotatef((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
}

// ---- glOrtho / glFrustum --------------------------------------------------
// Chỉ hợp lệ khi matrix mode = GL_PROJECTION (spec GL 1.x §2.9).
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glOrtho")) return;
    if (LegacyCheckBeginEndAllowed(c, "glRotatedv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glRotatefv")) return;
    if (LegacyCheckBeginEndAllowed(c, "glRotated")) return;
    if (c.legacy.matrixMode != 0x1701 /*GL_PROJECTION*/) {
        c.errors.Record(0x0502);  // INVALID_OPERATION
        return;
    }
    if (l == r || b == t || n == f) { c.errors.Record(0x0501); return; }  // INVALID_VALUE
    c.legacy.projection.top().Ortho(l, r, b, t, n, f);
}
void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFrustum")) return;
    if (c.legacy.matrixMode != 0x1701 /*GL_PROJECTION*/) {
        c.errors.Record(0x0502);
        return;
    }
    if (l == r || b == t || n == f) { c.errors.Record(0x0501); return; }
    if (n <= 0.0 || f <= 0.0) { c.errors.Record(0x0501); return; }
    c.legacy.projection.top().Frustum(l, r, b, t, n, f);
}

}  // namespace tglmt::gl
