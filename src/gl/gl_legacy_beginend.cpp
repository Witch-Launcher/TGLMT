// gl_legacy_beginend.cpp — bảng lệnh BỊ CẤM trong glBegin/glEnd.
//
// Spec: OpenGL 2.1 §2.6.3 "GL Commands within Begin/End" (glspec21.pdf
// PDF p.19–20) — văn bản giữ nguyên trong OpenGL 4.2/4.6 Compatibility.
//
// Xem include/tglmt/LegacyBeginEnd.h để mô tả đầy đủ.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;

namespace tglmt {

namespace {

// Hậu tố kiểu dữ liệu của một entry point OpenGL (vd "glLightfv" → base
// "glLight", hậu tố "fv"). Dùng để so khớp TÊN LỆNH chứ không so tiền tố:
// nếu so tiền tố thì "glColorMaterial" sẽ khớp base "glColor" (lệnh được
// phép) ⇒ sai.
bool IsTypeSuffix(const char* s) {
    static const char* kSuffixes[] = {
        // vector: "2fv" "3fv" "4fv" "2dv" "3dv" "4dv" "1fv" "1dv" ...
        "2fv", "3fv", "4fv", "2dv", "3dv", "4dv",
        "1fv", "1dv", "1iv", "1uiv", "1sv", "1ubv", "1usv", "1bv",
        "2f", "3f", "4f", "2d", "3d", "4d", "1f", "1d",
        "1i", "1ui", "1s", "1us", "1b", "1ub",
        "fv", "iv", "uiv", "ubv", "usv", "sv", "bv", "dv",
        "ui", "ub", "us",
        "f", "i", "v", "d", "s", "b",
        // glEvalMesh1 / glEvalMesh2 — tham số kiểu GLint, không phải kiểu GL.
        "1", "2",
        nullptr};
    for (const char** p = kSuffixes; *p; ++p)
        if (std::strcmp(s, *p) == 0) return true;
    return false;
}

// true nếu `name` là entry point có base `base` (vd name="glLightfv",
// base="glLight").
bool NameIs(const char* name, const char* base) {
    const size_t bl = std::strlen(base);
    if (std::strncmp(name, base, bl) != 0) return false;
    const char* rest = name + bl;
    if (*rest == '\0') return true;
    return IsTypeSuffix(rest);
}

// Lệnh ĐƯỢC PHÉP trong Begin/End (spec §2.6.3, PDF p.19):
//   "The only GL commands that are allowed within any Begin/End pairs are the
//    commands for specifying vertex coordinates, vertex colors, normal
//    coordinates, texture coordinates, generic vertex attributes, and fog
//    coordinates (Vertex, Color, SecondaryColor, Index, Normal, TexCoord and
//    MultiTexCoord, VertexAttrib, FogCoord), the ArrayElement command, the
//    EvalCoord and EvalPoint commands, commands for specifying lighting
//    material parameters (Material commands), display list invocation commands
//    (CallList and CallLists), and the EdgeFlag command."
const char* const kAllowed[] = {
    "glVertex", "glColor", "glSecondaryColor", "glIndex", "glNormal",
    "glTexCoord", "glMultiTexCoord", "glVertexAttrib", "glFogCoord",
    "glArrayElement", "glEvalCoord", "glEvalPoint",
    "glMaterial", "glCallList", "glCallLists", "glEdgeFlag",
    nullptr};

// Lệnh BỊ CẤM. Nhóm "may or may not be generated" của spec (client-state:
// EnableClientState, DisableClientState, PushClientAttrib, PopClientAttrib,
// ColorPointer, FogCoordPointer, EdgeFlagPointer, IndexPointer, NormalPointer,
// TexCoordPointer, SecondaryColorPointer, VertexPointer, VertexAttribPointer,
// ClientActiveTexture, InterleavedArrays, PixelStore) cũng nằm ở đây: TGLMT
// sinh INVALID_OPECTION ⇒ an toàn hơn "GL operation is undefined".
const char* const kForbidden[] = {
    // Ma trận / biến đổi
    "glPushMatrix", "glPopMatrix", "glLoadIdentity", "glLoadMatrix",
    "glLoadTransposeMatrix", "glMultMatrix", "glMultTransposeMatrix",
    "glTranslate", "glRotate", "glScale", "glOrtho", "glFrustum",
    // Spec GL 4.2C §2.6.3 (PDF p.51): danh sách lệnh ĐƯỢC PHÉP trong
    // Begin/End không có glRasterPos lẫn glWindowPos ⇒ cả hai đều bị cấm.
    "glMatrixMode", "glViewport", "glDepthRange", "glRasterPos", "glWindowPos",
    // Lighting / material / fog / texture env
    "glLight", "glLightModel", "glColorMaterial", "glShadeModel",
    "glFog", "glTexEnv", "glTexGen",
    // Enable/Disable
    "glEnable", "glDisable",
    // Draw
    "glDrawArrays", "glDrawElements", "glDrawRangeElements",
    // Pixel ops
    "glReadPixels", "glDrawPixels", "glCopyPixels", "glPixelStore",
    "glPixelTransfer", "glBitmap",
    // Selection / feedback / render mode
    "glSelectBuffer", "glFeedbackBuffer", "glRenderMode",
    // Evaluator
    "glEvalMesh", "glMap1", "glMap2",
    // Raster
    "glClipPlane", "glAlphaFunc", "glPolygonMode", "glPolygonOffset",
    "glPointSize", "glLineStipple", "glPolygonStipple",
    // Client state (spec: error optional — TGLMT sinh lỗi cho an toàn)
    "glEnableClientState", "glDisableClientState",
    "glPushClientAttrib", "glPopClientAttrib",
    "glColorPointer", "glFogCoordPointer", "glEdgeFlagPointer",
    "glIndexPointer", "glNormalPointer", "glSecondaryColorPointer",
    "glTexCoordPointer", "glVertexPointer", "glVertexAttribPointer",
    "glClientActiveTexture", "glInterleavedArrays",
    nullptr};

}  // namespace

// true nếu lệnh `name` bị cấm trong glBegin/glEnd.
bool LegacyCommandForbiddenInBeginEnd(const char* name) {
    if (!name) return false;
    // Kiểm tra DANH SÁCH CẤM TRƯỚC: "glColorMaterial" phải khớp base cấm
    // "glColorMaterial", KHÔNG được coi là lệnh được phép "glColor".
    for (const char* const* p = kForbidden; *p; ++p)
        if (NameIs(name, *p)) return true;
    for (const char* const* p = kAllowed; *p; ++p)
        if (NameIs(name, *p)) return false;
    return false;
}

// Ghi GL_INVALID_OPERATION nếu lệnh `name` bị cấm trong glBegin/glEnd.
// Trả true nếu lệnh bị chặn ⇒ caller phải return ngay (không thực thi).
bool LegacyCheckBeginEndAllowed(Context& c, const char* name) {
    if (!c.legacy.inBeginEnd) return false;
    if (!LegacyCommandForbiddenInBeginEnd(name)) return false;
    c.errors.Record(0x0502);  // GL_INVALID_OPERATION
    return true;
}

}  // namespace tglmt
