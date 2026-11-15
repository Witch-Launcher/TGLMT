// gl_legacy_raster.cpp — CURRENT RASTER POSITION, WINDOW POSITION, CLIP PLANES,
//                        STIPPLE (polygon + line), POINT PARAMETERS.
//
// Spec (đã đối chiếu trực tiếp với PDF, không suy đoán):
//   • OpenGL 1.0  §2.10 Clipping, §2.11 Current Raster Position,
//                 §3.4.2 Line Stipple, §3.5.2 Stippling (polygon)
//                 → glspec10.pdf PDF p.41–44, 74–75.
//   • OpenGL 2.1  §2.12 Clipping (PDF p.52–54), §2.13 Current Raster Position
//                 (PDF p.54–56), §3.4.2 Line Stipple (PDF p.104–105),
//                 §3.5.2 Stippling (PDF p.110), §6.1.3 Enumerated Queries
//                 (GetClipPlane, PDF p.249), §6.1.5 Stipple Query (PDF p.253).
//   • OpenGL 4.2 Compatibility Profile §2.23 Clipping, §2.25 Current Raster
//                 Position (PDF p.214–217), §3.5.2 Line Stipple (PDF p.235),
//                 §3.6.2 Stippling (PDF p.241), §6.1.3 (PDF p.477),
//                 §6.1.6 Stipple Query (PDF p.485).
//   • OpenGL 1.4  §3.3 Points (PointParameters, PDF p.79–80).
//   • OpenGL 4.6 Compatibility Profile §3.5.2 (LineStipple: factor clamp).
//
// BAO GIỜ ĐÃ CÓ, THIẾU GÌ:
//   • State: LegacyState::raster, ::clipPlanes[6], ::stipple, ::pointParam
//     (include/tglmt/LegacyState.h) — đủ field, KHÔNG sửa file đó.
//   • Entry point glLineStipple / glPointParameter{f,v,i,iv} ĐÃ CÓ trong
//     src/gl/gl_raster.cpp (230–258, 354) nhưng (a) glLineStipple là no-op,
//     (b) glPointParameter* chỉ nhận 2 pname của Core Profile
//     (GL_POINT_FADE_THRESHOLD_SIZE / GL_POINT_SPRITE_COORD_ORIGIN) nên các
//     pname Compatibility (GL_POINT_SIZE_MIN/MAX, GL_POINT_DISTANCE_ATTENUATION)
//     chưa chạy được. Vì src/gl/gl_raster.cpp NGOÀI write-scope, toàn bộ logic
//     đúng đặt ở đây trong các hàm LegacySet* (xem cuối file) để agent-main
//     chỉ cần gọi lại — KHÔNG định nghĩa trùng tên (tránh duplicate symbol).
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

namespace {

// ---------------------------------------------------------------------------
// Hằng spec (lấy từ header SINH — không hardcode hex).
// ---------------------------------------------------------------------------
constexpr GLenum kClipPlane0 = GL_CLIP_PLANE0;
constexpr GLenum kClipPlaneMax = GL_CLIP_PLANE5;
constexpr GLenum kPointSizeMin = GL_POINT_SIZE_MIN;
constexpr GLenum kPointSizeMax = GL_POINT_SIZE_MAX;
constexpr GLenum kPointDistanceAttenuation = GL_POINT_DISTANCE_ATTENUATION;
constexpr GLenum kPointFadeThresholdSize = GL_POINT_FADE_THRESHOLD_SIZE;
constexpr GLenum kPointSpriteCoordOrigin = GL_POINT_SPRITE_COORD_ORIGIN;
constexpr GLenum kFogCoord = GL_FOG_COORD;
constexpr GLenum kDepthClamp = GL_DEPTH_CLAMP;

// Pattern polygon stipple: 32×32 BIT (spec GL 2.1 §3.5.2 PDF p.110:
// "a 32×32 pattern ... as if the height and width passed to that command were
// both equal to 32, the type were BITMAP, and the format were COLOR INDEX")
// ⇒ đóng gói 4 byte/hàng × 32 hàng = 128 byte.
constexpr int kStippleCols = 32;
constexpr int kStippleRows = 32;
constexpr int kStippleRowBytes = kStippleCols / 8;   // 4
constexpr int kStippleBytes = kStippleRows * kStippleRowBytes;  // 128

// Con trỏ NULL cho các biến thể vector: spec Desktop GL không liệt kê lỗi
// cho trường hợp này (man page glRasterPos chỉ liệt kê INVALID_OPERATION
// trong Begin/End) — ta ghi GL_INVALID_VALUE để không deref NULL (an toàn).
inline void NullPointerError(Context& c) { c.errors.Record(GL_INVALID_VALUE); }

// Nghịch đảo ma trận 4×4 (column-major: m[col*4 + row]). Gauss-Jordan có
// pivot. Trả false nếu ma trận singular (spec GL 2.1 §2.12: "the resulting
// plane equation is undefined if M is singular").
bool Invert4x4(const float* in, float* out) {
    double a[4][8];
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) a[r][c] = (double)in[c * 4 + r];
        for (int c = 0; c < 4; ++c) a[r][4 + c] = (r == c) ? 1.0 : 0.0;
    }
    for (int col = 0; col < 4; ++col) {
        int piv = col;
        for (int r = col + 1; r < 4; ++r)
            if (std::fabs(a[r][col]) > std::fabs(a[piv][col])) piv = r;
        if (std::fabs(a[piv][col]) < 1e-12) return false;
        if (piv != col)
            for (int c = 0; c < 8; ++c) { double t = a[col][c]; a[col][c] = a[piv][c]; a[piv][c] = t; }
        const double d = a[col][col];
        for (int c = 0; c < 8; ++c) a[col][c] /= d;
        for (int r = 0; r < 4; ++r) {
            if (r == col) continue;
            const double f = a[r][col];
            if (f == 0.0) continue;
            for (int c = 0; c < 8; ++c) a[r][c] -= f * a[col][c];
        }
    }
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) out[c * 4 + r] = (float)a[r][4 + c];
    return true;
}

// ---------------------------------------------------------------------------
// CLIP VOLUME (spec GL 2.1 §2.12 PDF p.52)
// ---------------------------------------------------------------------------
// View volume trong clip coords: -wc ≤ xc ≤ wc, -wc ≤ yc ≤ wc, -wc ≤ zc ≤ wc.
// Với GL 4.x (clip control): origin LOWER_LEFT ⇒ [-1,1] cho x/y, UPPER_LEFT ⇒
// [0,1]; depth NEGATIVE_ONE_TO_ONE ⇒ [-1,1], ZERO_TO_ONE ⇒ [0,1].
bool InsideClipVolume(Context& c, const float clip[4]) {
    const float wc = clip[3];
    if (!(wc > 0.0f)) return false;  // wc ≤ 0 ⇒ ngoài clip volume
    const float xd = clip[0] / wc, yd = clip[1] / wc, zd = clip[2] / wc;
    const bool upper = (c.state.ClipOrigin() == GL_UPPER_LEFT);
    if (upper) {
        if (xd < 0.0f || xd > 1.0f || yd < 0.0f || yd > 1.0f) return false;
    } else {
        if (xd < -1.0f || xd > 1.0f || yd < -1.0f || yd > 1.0f) return false;
    }
    if (c.state.ClipDepth() == GL_ZERO_TO_ONE) {
        if (zd < 0.0f || zd > 1.0f) return false;
    } else {
        if (zd < -1.0f || zd > 1.0f) return false;
    }
    return true;
}

// Client-defined clip planes (spec GL 2.1 §2.12 PDF p.53): half-space là
// (p1' p2' p3' p4')·(xe ye ze we) ≥ 0 với hệ số đã transform sang EYE SPACE
// lúc gọi glClipPlane.
// LƯU Ù: glEnable/glDisable chưa đồng bộ legacy.clipPlanes[i].enabled
// (src/gl/gl_raster.cpp ngoài write-scope) ⇒ đọc CẢ state tracker (đúng) và
// cờ legacy (để sau này đồng bộ).
bool InsideClientClipPlanes(Context& c, const float eye[4]) {
    const LegacyState& L = c.legacy;
    for (int i = 0; i < 6; ++i) {
        const bool on = L.clipPlanes[i].enabled || c.state.IsEnabled(kClipPlane0 + (GLenum)i);
        if (!on) continue;
        const float* e = L.clipPlanes[i].eq;
        const float d = e[0] * eye[0] + e[1] * eye[1] + e[2] * eye[2] + e[3] * eye[3];
        if (d < 0.0f) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CURRENT RASTER POSITION — dữ liệu đi kèm (spec GL 2.1 §2.13 PDF p.55)
// ---------------------------------------------------------------------------
// "These coordinates, along with current values, are used to generate primary
//  and secondary colors and texture coordinates just as is done for a vertex.
//  The colors and texture coordinates so produced replace the color and texture
//  coordinates stored in the current raster position's associated data."
// Lighting chưa mô phỏng ở tầng raster position ⇒ dùng current color đã clamp
// (RGBA mode). Current raster distance: nếu fog source = FOG_COORD ⇒ current
// fog coordinate; ngược lại = khoảng cách từ gốc eye tới đỉnh (modelview).
void SetRasterAssociatedData(Context& c, const float eye[4]) {
    LegacyState& L = c.legacy;
    for (int i = 0; i < 4; ++i) {
        const float v = L.currentColor[i];
        L.raster.color[i] = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
    }
    L.raster.index = L.currentIndex;
    const GLuint unit = L.activeTextureUnit < 8 ? L.activeTextureUnit : 0;
    std::memcpy(L.raster.texCoord, L.currentTexCoord[unit], sizeof(L.raster.texCoord));
    if (L.fog.coordSrc == kFogCoord) {
        L.raster.fogCoord = L.fog.currentCoord;
    } else {
        const float d2 = eye[0] * eye[0] + eye[1] * eye[1] + eye[2] * eye[2];
        L.raster.fogCoord = std::sqrt(d2);
    }
}

// ---------------------------------------------------------------------------
// glRasterPos* — spec GL 2.1 §2.13 (PDF p.54–55), GL 4.2C §2.25 (p.214–216)
// ---------------------------------------------------------------------------
// "The coordinates are treated as if they were specified in a Vertex command.
//  ... the x, y, z, and w coordinates are transformed by the current model-view
//  and projection matrices."
// "The transformed coordinates are passed to clipping as if they represented a
//  point. If the 'point' is not culled, then the projection to window
//  coordinates is computed and saved as the current raster position, and the
//  valid bit is set. If the 'point' is culled, the current raster position and
//  its associated data become indeterminate and the valid bit is cleared."
void ApplyRasterPos(float x, float y, float z, float w) {
    Context& c = Context::Current();
    // RasterPos KHÔNG được phép trong glBegin/glEnd (spec GL 4.2C §2.6.3
    // PDF p.51: danh sách lệnh được phép trong Begin/End không có RasterPos;
    // man page glRasterPos: "GL_INVALID_OPERATION is generated if glRasterPos
    // is executed between the execution of glBegin and the corresponding
    // execution of glEnd").
    if (LegacyCheckBeginEndAllowed(c, "glRasterPos")) return;
    LegacyState& L = c.legacy;
    const GLMatrix& mv = L.modelview.top();
    const GLMatrix& pj = L.projection.top();

    float eye[4], clip[4];
    mv.Transform4(x, y, z, w, eye);
    pj.Transform4(eye[0], eye[1], eye[2], eye[3], clip);

    if (!InsideClipVolume(c, clip) || !InsideClientClipPlanes(c, eye)) {
        L.raster.valid = false;  // "the valid bit is cleared"
        return;
    }

    const float wc = clip[3];
    const float xd = clip[0] / wc, yd = clip[1] / wc, zd = clip[2] / wc;
    const ViewportState vp = c.state.GetViewport(0);
    const float ox = vp.x + vp.w * 0.5f, oy = vp.y + vp.h * 0.5f;
    const float n = vp.n, f = vp.f;

    L.raster.x = (vp.w * 0.5f) * xd + ox;
    L.raster.y = (vp.h * 0.5f) * yd + oy;
    if (c.state.ClipDepth() == GL_ZERO_TO_ONE) {
        L.raster.z = (f - n) * zd + n;
    } else {
        L.raster.z = (f - n) * 0.5f * zd + (n + f) * 0.5f;
    }
    // Depth clamp (GL 4.2C §2.25 PDF p.215): "If depth clamping is enabled,
    // then raster position zw is first clamped to the range [min(n,f), max(n,f)]".
    if (c.state.IsEnabled(kDepthClamp)) {
        const float lo = n < f ? n : f, hi = n < f ? f : n;
        if (L.raster.z < lo) L.raster.z = lo;
        if (L.raster.z > hi) L.raster.z = hi;
    }
    L.raster.w = wc;
    L.raster.valid = true;
    L.raster.depth = L.raster.z;  // associated depth value của vertex
    SetRasterAssociatedData(c, eye);
}

// glWindowPos* bị cấm trong Begin/End (spec GL 4.2C §2.6.3 PDF p.51: danh
// sách lệnh ĐƯỢC PHÉP trong Begin/End KHÔNG có RasterPos/WindowPos).
// Bảng lệnh cấm (src/gl/gl_legacy_beginend.cpp) có "glRasterPos" nhưng CHƯA
// có "glWindowPos" ⇒ kiểm tra tay ở đây (file đó ngoài write-scope).
inline bool WindowPosForbiddenInBeginEnd(Context& c) {
    if (!c.legacy.inBeginEnd) return false;
    c.errors.Record(GL_INVALID_OPERATION);
    return true;
}

// ---------------------------------------------------------------------------
// glWindowPos* — spec GL 2.1 §2.13 (PDF p.55–56), GL 4.2C §2.25 (p.216–217)
// ---------------------------------------------------------------------------
// "The current raster position, (xw, yw, zw, wc), is defined by:
//      xw = x
//      yw = y
//      zw = n, z ≤ 0 ; f, z ≥ 1 ; n + z(f−n), otherwise
//      wc = 1
//  where n and f are the values passed to DepthRange."
// "Lighting, texture coordinate generation and transformation, and clipping are
//  NOT performed by the WindowPos functions. ... the current raster texture
//  coordinates are set to the current texture coordinates, and the valid bit is
//  set."  ⇒ WindowPos KHÔNG clip ⇒ valid luôn = TRUE.
void ApplyWindowPos(float x, float y, float z) {
    Context& c = Context::Current();
    LegacyState& L = c.legacy;
    const ViewportState vp = c.state.GetViewport(0);
    const float n = vp.n, f = vp.f;

    L.raster.x = x;
    L.raster.y = y;
    if (z <= 0.0f) L.raster.z = n;
    else if (z >= 1.0f) L.raster.z = f;
    else L.raster.z = n + z * (f - n);
    L.raster.w = 1.0f;
    L.raster.valid = true;  // spec: "the valid bit is set" (không clip)
    L.raster.depth = L.raster.z;
    // Associated data: current color đã clamp, current index, current texcoord.
    for (int i = 0; i < 4; ++i) {
        const float v = L.currentColor[i];
        L.raster.color[i] = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
    }
    L.raster.index = L.currentIndex;
    const GLuint unit = L.activeTextureUnit < 8 ? L.activeTextureUnit : 0;
    std::memcpy(L.raster.texCoord, L.currentTexCoord[unit], sizeof(L.raster.texCoord));
    // "If the value of the fog source is FOG_COORD_SRC, then the current raster
    //  distance is set to the value of the current fog coordinate. Otherwise,
    //  the raster distance is set to 0."
    L.raster.fogCoord = (L.fog.coordSrc == kFogCoord) ? L.fog.currentCoord : 0.0f;
}

// ---------------------------------------------------------------------------
// POLYGON STIPPLE PATTERN (spec GL 2.1 §3.5.2 PDF p.110)
// ---------------------------------------------------------------------------
// GAP (báo cáo): LegacyState::stipple.polygon chỉ có 32 byte (uint8_t[32],
// "32 hàng × 8 bit") trong khi pattern theo spec là 32×32 BIT = 128 byte
// đóng gói. Không sửa được LegacyState.h (ngoài write-scope) ⇒ giữ pattern
// đầy đủ trong buffer thread-local (128 byte) và mirror 32 byte đầu vào
// state field. Agent-main: đổi `uint8_t polygon[32]` → `uint8_t polygon[128]`
// rồi bỏ buffer thread-local.
struct PolygonStippleStore {
    const void* ctx = nullptr;
    uint8_t bytes[kStippleBytes];
    PolygonStippleStore() { std::memset(bytes, 0xFF, sizeof(bytes)); }  // spec: "initial stipple pattern is all ones"
};
thread_local PolygonStippleStore g_polygonStipple;

uint8_t* PolygonStippleBytes(Context& c) {
    if (g_polygonStipple.ctx != (const void*)&c) {
        g_polygonStipple.ctx = (const void*)&c;
        std::memset(g_polygonStipple.bytes, 0xFF, sizeof(g_polygonStipple.bytes));
    }
    return g_polygonStipple.bytes;
}

// Số byte của 1 hàng theo pixel-store (row length + alignment).
size_t RowStride(int rowLength, int alignment) {
    const size_t bytes = (size_t)((rowLength + 7) / 8);
    const size_t a = (size_t)(alignment > 0 ? alignment : 4);
    return ((bytes + a - 1) / a) * a;
}

}  // namespace

// ===========================================================================
// RASTER POSITION — 24 entry point
// ===========================================================================
// Chuyển đổi kiểu số nguyên: KHÔNG dùng Table 2.9. Spec GL 2.1 Table 2.9
// (PDF p.59) ghi rõ phạm vi: "Component conversions. COLOR, NORMAL, and DEPTH
// components ... are converted to an internal floating-point representation".
// Toạ độ raster position là TOẠ ĐỘ (không phải color/normal/depth) ⇒ biến
// thể int/short chuyển thẳng sang float (giống hàm Vertex trong driver thật:
// Mesa cast trực tiếp (GLfloat)x). Spec GL 2.1 §2.13: "The coordinates are
// treated as if they were specified in a Vertex command."
void glRasterPos2d(GLdouble x, GLdouble y) { ApplyRasterPos((float)x, (float)y, 0.0f, 1.0f); }
void glRasterPos2dv(const GLdouble* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos2d(v[0], v[1]);
}
void glRasterPos2f(GLfloat x, GLfloat y) { ApplyRasterPos(x, y, 0.0f, 1.0f); }
void glRasterPos2fv(const GLfloat* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos2f(v[0], v[1]);
}
void glRasterPos2i(GLint x, GLint y) { ApplyRasterPos((float)x, (float)y, 0.0f, 1.0f); }
void glRasterPos2iv(const GLint* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos2i(v[0], v[1]);
}
void glRasterPos2s(GLshort x, GLshort y) { ApplyRasterPos((float)x, (float)y, 0.0f, 1.0f); }
void glRasterPos2sv(const GLshort* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos2s(v[0], v[1]);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) {
    ApplyRasterPos((float)x, (float)y, (float)z, 1.0f);
}
void glRasterPos3dv(const GLdouble* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos3d(v[0], v[1], v[2]);
}
void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) { ApplyRasterPos(x, y, z, 1.0f); }
void glRasterPos3fv(const GLfloat* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos3f(v[0], v[1], v[2]);
}
void glRasterPos3i(GLint x, GLint y, GLint z) { ApplyRasterPos((float)x, (float)y, (float)z, 1.0f); }
void glRasterPos3iv(const GLint* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos3i(v[0], v[1], v[2]);
}
void glRasterPos3s(GLshort x, GLshort y, GLshort z) {
    ApplyRasterPos((float)x, (float)y, (float)z, 1.0f);
}
void glRasterPos3sv(const GLshort* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos3s(v[0], v[1], v[2]);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    ApplyRasterPos((float)x, (float)y, (float)z, (float)w);
}
void glRasterPos4dv(const GLdouble* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos4d(v[0], v[1], v[2], v[3]);
}
void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { ApplyRasterPos(x, y, z, w); }
void glRasterPos4fv(const GLfloat* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos4f(v[0], v[1], v[2], v[3]);
}
void glRasterPos4i(GLint x, GLint y, GLint z, GLint w) {
    ApplyRasterPos((float)x, (float)y, (float)z, (float)w);
}
void glRasterPos4iv(const GLint* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos4i(v[0], v[1], v[2], v[3]);
}
void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    ApplyRasterPos((float)x, (float)y, (float)z, (float)w);
}
void glRasterPos4sv(const GLshort* v) {
    if (!v) { NullPointerError(Context::Current()); return; }
    glRasterPos4s(v[0], v[1], v[2], v[3]);
}

// ===========================================================================
// WINDOW POSITION — 16 entry point
// ===========================================================================
void glWindowPos2d(GLdouble x, GLdouble y) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, 0.0f);
}
void glWindowPos2dv(const GLdouble* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], 0.0f);
}
void glWindowPos2f(GLfloat x, GLfloat y) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos(x, y, 0.0f);
}
void glWindowPos2fv(const GLfloat* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos(v[0], v[1], 0.0f);
}
void glWindowPos2i(GLint x, GLint y) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, 0.0f);
}
void glWindowPos2iv(const GLint* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], 0.0f);
}
void glWindowPos2s(GLshort x, GLshort y) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, 0.0f);
}
void glWindowPos2sv(const GLshort* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], 0.0f);
}

void glWindowPos3d(GLdouble x, GLdouble y, GLdouble z) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, (float)z);
}
void glWindowPos3dv(const GLdouble* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], (float)v[2]);
}
void glWindowPos3f(GLfloat x, GLfloat y, GLfloat z) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos(x, y, z);
}
void glWindowPos3fv(const GLfloat* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos(v[0], v[1], v[2]);
}
void glWindowPos3i(GLint x, GLint y, GLint z) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, (float)z);
}
void glWindowPos3iv(const GLint* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], (float)v[2]);
}
void glWindowPos3s(GLshort x, GLshort y, GLshort z) {
    if (WindowPosForbiddenInBeginEnd(Context::Current())) return;
    ApplyWindowPos((float)x, (float)y, (float)z);
}
void glWindowPos3sv(const GLshort* v) {
    Context& c = Context::Current();
    if (WindowPosForbiddenInBeginEnd(c)) return;
    if (!v) { NullPointerError(c); return; }
    ApplyWindowPos((float)v[0], (float)v[1], (float)v[2]);
}

// ===========================================================================
// CLIP PLANES — 2 entry point
// ===========================================================================
// glClipPlane (spec GL 2.1 §2.12 PDF p.52–53):
//   "eqn is an array of four double-precision floating-point values. These are
//    the coefficients of a plane equation in OBJECT coordinates: p1, p2, p3,
//    and p4 (in that order). The inverse of the current model-view matrix is
//    applied to these coefficients, at the time they are specified, yielding
//    (p1' p2' p3' p4') = (p1 p2 p3 p4) M^-1 ... to obtain the plane equation
//    coefficients in eye coordinates."
// ⇒ p' = p · M^-1 (hàng nhân ma trận), KHÔNG phải inverse-transpose.
// glGetClipPlane (spec GL 2.1 §6.1.3 PDF p.249):
//   "GetClipPlane always returns four double-precision values in eqn; these are
//    the coefficients of the plane equation of plane in EYE COORDINATES (these
//    coordinates are those that were computed when the plane was specified)."
// ⇒ round-trip trả về HỆ SỐ EYE SPACE (đã transform), không phải hệ số gốc.
void glClipPlane(GLenum plane, const GLdouble* equation) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glClipPlane")) return;
    if (plane < kClipPlane0 || plane > kClipPlaneMax) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (!equation) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    const int idx = (int)(plane - kClipPlane0);
    float inv[16];
    if (!Invert4x4(c.legacy.modelview.top().m, inv)) {
        // M singular ⇒ kết quả undefined theo spec; giữ hệ số gốc để không
        // đoán bừa (vẫn là "indeterminate" theo spec).
        for (int i = 0; i < 4; ++i) c.legacy.clipPlanes[idx].eq[i] = (float)equation[i];
        return;
    }
    for (int j = 0; j < 4; ++j) {
        double s = 0.0;
        for (int i = 0; i < 4; ++i) s += equation[i] * (double)inv[j * 4 + i];
        c.legacy.clipPlanes[idx].eq[j] = (float)s;
    }
}

void glGetClipPlane(GLenum plane, GLdouble* equation) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetClipPlane")) return;
    if (plane < kClipPlane0 || plane > kClipPlaneMax) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (!equation) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    const int idx = (int)(plane - kClipPlane0);
    for (int i = 0; i < 4; ++i) equation[i] = (GLdouble)c.legacy.clipPlanes[idx].eq[i];
}

// ===========================================================================
// POLYGON STIPPLE — 2 entry point
// ===========================================================================
// glPolygonStipple (spec GL 2.1 §3.5.2 PDF p.110):
//   "pattern is a pointer to memory into which a 32×32 pattern is packed. The
//    pattern is unpacked from memory according to the procedure given in
//    section 3.6.4 for DrawPixels; it is as if the height and width passed to
//    that command were both equal to 32, the type were BITMAP, and the format
//    were COLOR INDEX."
//   "If xw and yw are the window coordinates of a rasterized polygon fragment,
//    then that fragment is sent to the next stage of the GL if and only if the
//    bit of the pattern (xw mod 32, yw mod 32) is 1."
// ⇒ pattern = 32×32 BIT đóng gói MSB-first mỗi byte, 4 byte/hàng, 32 hàng
//    = 128 byte. Trong 1 byte, pixel ĐẦU (cột nhỏ nhất) nằm ở bit MSB
//    (UNPACK_LSB_FIRST = FALSE, mặc định).
// LƯU Ù: tầng pixel-store của TGLMT chưa có cờ UNPACK_LSB_FIRST /
//    UNPACK_SWAP_BYTES (src/gl/glPixelStore* còn stub) ⇒ chỉ xử lý mặc định.
void glPolygonStipple(const GLubyte* mask) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPolygonStipple")) return;
    if (!mask) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    uint8_t* pat = PolygonStippleBytes(c);
    const PixelStoreState& ps = c.state.PixelStore();
    const int rowLength = ps.unpackRowLength > 0 ? ps.unpackRowLength : kStippleCols;
    const size_t stride = RowStride(rowLength, ps.unpackAlignment);
    const int skipRows = ps.unpackSkipRows;
    const int skipPixels = ps.unpackSkipPixels;

    for (int y = 0; y < kStippleRows; ++y) {
        const uint8_t* row = mask + (size_t)(skipRows + y) * stride + (size_t)(skipPixels / 8);
        const int bitBase = skipPixels % 8;
        for (int x = 0; x < kStippleCols; ++x) {
            const int bitIndex = x + bitBase;
            const uint8_t src = row[bitIndex / 8];
            const int v = (src >> (7 - (bitIndex % 8))) & 1;  // MSB first
            uint8_t& dst = pat[y * kStippleRowBytes + x / 8];
            const int dstBit = 7 - (x % 8);
            dst = (uint8_t)((dst & ~(1u << dstBit)) | ((unsigned)v << dstBit));
        }
    }
    // Mirror 32 byte đầu vào state field (xem GAP ở PolygonStippleStore).
    std::memcpy(c.legacy.stipple.polygon, pat, sizeof(c.legacy.stipple.polygon));
}

// glGetPolygonStipple (spec GL 2.1 §6.1.5 PDF p.253):
//   "The pattern is packed into pixel pack buffer or client memory according to
//    the procedure given in section 4.3.2 for ReadPixels; it is as if the height
//    and width passed to that command were both equal to 32, the type were
//    BITMAP, and the format were COLOR_INDEX."
void glGetPolygonStipple(GLubyte* mask) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetPolygonStipple")) return;
    if (!mask) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    const uint8_t* pat = PolygonStippleBytes(c);
    const PixelStoreState& ps = c.state.PixelStore();
    const int rowLength = ps.packRowLength > 0 ? ps.packRowLength : kStippleCols;
    const size_t stride = RowStride(rowLength, ps.packAlignment);
    const size_t bytesPerRow = (size_t)((rowLength + 7) / 8);
    const int skipRows = ps.packSkipRows;
    const int skipPixels = ps.packSkipPixels;

    for (int y = 0; y < kStippleRows; ++y) {
        uint8_t* row = mask + (size_t)(skipRows + y) * stride + (size_t)(skipPixels / 8);
        std::memset(row, 0, bytesPerRow);
        const int bitBase = skipPixels % 8;
        for (int x = 0; x < kStippleCols; ++x) {
            const int v = (pat[y * kStippleRowBytes + x / 8] >> (7 - (x % 8))) & 1;
            const int bitIndex = x + bitBase;
            row[bitIndex / 8] = (uint8_t)(row[bitIndex / 8] | ((unsigned)v << (7 - (bitIndex % 8))));
        }
    }
}

// ===========================================================================
// LINE STIPPLE + POINT PARAMETERS — logic dùng chung
// ===========================================================================
// src/gl/gl_raster.cpp ĐÃ định nghĩa glLineStipple (no-op) và
// glPointParameter{f,v,i,iv} (chỉ pname Core) — file đó NGOÀI write-scope
// nên KHÔNG định nghĩa trùng tên ở đây (duplicate symbol). Hai hàm dưới đây
// chứa TOÀN BỘ logic đúng; agent-main chỉ cần gọi lại chúng từ
// src/gl/gl_raster.cpp (xem báo cáo).
//
// glLineStipple (spec GL 4.6C §3.5.2 + GL 2.1 §3.4.2 PDF p.104–105):
//   "factor is a count that is used to modify the effective line stipple by
//    causing each bit in pattern to be used factor times. factor is clamped to
//    the range [1, 256]."   ⇒ factor ≤ 0 KHÔNG sinh lỗi, được clamp về 1.
//   "The bits of p are numbered with 0 being the least significant and 15 being
//    the most significant."  ⇒ bit 0 (LSB) dùng TRƯỚC.
void LegacySetLineStipple(GLint factor, GLushort pattern) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLineStipple")) return;
    GLint f = factor;
    if (f < 1) f = 1;        // spec: clamp về [1,256]
    if (f > 256) f = 256;
    c.legacy.stipple.linePattern = pattern;
    c.legacy.stipple.lineRepeat = f;
}

// Point parameters (spec GL 1.4 §3.3 PDF p.79–80):
//   "If pname is POINT_SIZE_MIN or POINT_SIZE_MAX, then param specifies, or
//    params points to the lower or upper bound respectively to which the derived
//    point size is clamped. If the lower bound is greater than the upper bound,
//    the point size after clamping is undefined. If pname is
//    POINT_DISTANCE_ATTENUATION, then params points to the coefficients a, b,
//    and c. If pname is POINT_FADE_THRESHOLD_SIZE, then param specifies, or
//    params points to the point fade threshold. Values of POINT_SIZE_MIN,
//    POINT_SIZE_MAX, or POINT_FADE_THRESHOLD_SIZE less than zero result in the
//    error INVALID_VALUE."
// ⇒ min > max KHÔNG sinh lỗi (chỉ "undefined"), pname lạ ⇒ INVALID_ENUM.
void LegacySetPointParameterf(GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    LegacyState& L = c.legacy;
    switch (pname) {
        case kPointSizeMin:
            if (param < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.sizeMin = param;
            return;
        case kPointSizeMax:
            if (param < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.sizeMax = param;
            return;
        case kPointFadeThresholdSize:
            if (param < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.fadeThreshold = param;
            return;
        case kPointDistanceAttenuation:
            // Chỉ có biến thể vector (3 hệ số a,b,c).
            c.errors.Record(GL_INVALID_VALUE);
            return;
        default:
            c.errors.Record(GL_INVALID_ENUM);
            return;
    }
}

void LegacySetPointParameterfv(GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    LegacyState& L = c.legacy;
    if (!params) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    switch (pname) {
        case kPointSizeMin:
            if (params[0] < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.sizeMin = params[0];
            return;
        case kPointSizeMax:
            if (params[0] < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.sizeMax = params[0];
            return;
        case kPointFadeThresholdSize:
            if (params[0] < 0.0f) { c.errors.Record(GL_INVALID_VALUE); return; }
            L.pointParam.fadeThreshold = params[0];
            return;
        case kPointDistanceAttenuation:
            L.pointParam.distAttenuation[0] = params[0];
            L.pointParam.distAttenuation[1] = params[1];
            L.pointParam.distAttenuation[2] = params[2];
            return;
        default:
            c.errors.Record(GL_INVALID_ENUM);
            return;
    }
}

// Biến thể nguyên: chỉ pname kiểu enum (POINT_SPRITE_COORD_ORIGIN) — giống
// driver thật (Mesa _mesa_PointParameteri): pname khác ⇒ INVALID_ENUM.
void LegacySetPointParameteri(GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (pname != kPointSpriteCoordOrigin) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (param != (GLint)GL_UPPER_LEFT && param != (GLint)GL_LOWER_LEFT) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    c.legacy.pointParam.spriteCoordOrigin = (GLenum)param;
}

void LegacySetPointParameteriv(GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (!params) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    if (pname != kPointSpriteCoordOrigin) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (params[0] != (GLint)GL_UPPER_LEFT && params[0] != (GLint)GL_LOWER_LEFT) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    c.legacy.pointParam.spriteCoordOrigin = (GLenum)params[0];
}

}  // namespace tglmt::gl
