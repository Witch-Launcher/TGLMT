// gl_legacy_rect.cpp — glRect* (OpenGL Compatibility Profile).
//
// Spec (đã đối chiếu TRỰC TIẾP với PDF):
//   • GL 2.1 §2.10 "Rectangles" (glspec21.pdf PDF p.40, idx 53):
//       "void Rect{sifd}( T x1, T y1, T x2, T y2 );
//        void Rect{sifd}v( const T v1[2], const T v2[2] );
//        Each command takes either four arguments organized as two consecutive
//        pairs of (x, y) coordinates, or two pointers to arrays each of which
//        contains an x value followed by a y value. The effect of the Rect
//        command
//        Rect(x1, y1, x2, y2);
//        is exactly the same as the following sequence of commands:
//        Begin(POLYGON);
//        Vertex2(x1, y1);
//        Vertex2(x2, y1);
//        Vertex2(x2, y2);
//        Vertex2(x1, y2);
//        End();
//        The appropriate Vertex2 command would be invoked depending on which of
//        the Rect commands is issued."
//   • GL 4.2C §2.11 "RECTANGLES" (glspec42.compatibility.pdf PDF p.69–70,
//     idx 88–89) — văn bản giữ nguyên trong Compatibility Profile.
//   • GL 2.1 §2.7 "Vertex Specification" (PDF p.20, idx 33) + Table 2.9
//     (PDF p.59, idx 72) — quy tắc chuyển đổi kiểu của toạ độ đỉnh.
//
// WINDING: thứ tự đỉnh (x1,y1) → (x2,y1) → (x2,y2) → (x1,y2) là NGƯỜI THUẬN
// (counter-clockwise) khi x2 > x1 và y2 > y1; ngược lại winding bị đảo — đúng
// như GL_POLYGON/GL_QUADS nói chung (spec không quy định hướng cho Rect, nó
// quy định đúng chuỗi lệnh Begin/Vertex/End ở trên).
//
// CHUYỂN ĐỔI KIỂU (biến thể int/short): Table 2.9 (GL 2.1 p.59) CHỈ áp cho
// "Color, normal, and depth components" ⇒ toạ độ đỉnh của biến thể
// integer/short chuyển THẲNG sang float (không dùng (2c+1)/(2^n−1)).
// Vì vậy glRecti/glRects KHÔNG gọi glVertex2i (src/gl/gl_legacy_typevariants.cpp:99
// áp Table 2.9 cho toạ độ — lệch spec, xem báo cáo phiên) mà chuyển trực tiếp.
//
// CURRENT RASTER POSITION: GL 2.1/4.2C KHÔNG yêu cầu Rect cập nhật current
// raster position (raster position chỉ được đặt bởi RasterPos/WindowPos —
// GL 2.1 §2.13 p.54, GL 4.2C §2.25 p.214). TGLMT vẫn cập nhật raster
// position về GÓC TRÊN-TRÁI của hình chữ nhật (yêu cầu của task slice này) —
// hành vi MỞ RỘNG, được ghi rõ ở đây và trong báo cáo phiên.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

namespace {

constexpr GLenum kPolygon = GL_POLYGON;
constexpr GLenum kInvalidOperation = GL_INVALID_OPERATION;
constexpr GLenum kInvalidValue = GL_INVALID_VALUE;

// --- Biến đổi object → window (modelview → projection → viewport) ----------
// Cùng công thức với ApplyRasterPos trong src/gl/gl_legacy_raster.cpp
// (spec GL 2.1 §2.13 PDF p.54–55): toạ độ được biến đổi bằng modelview rồi
// projection, clip theo clip volume, rồi chiếu sang toạ độ window.
struct WindowPoint {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    bool inside = false;
};

WindowPoint ObjectToWindow(Context& c, float x, float y, float z) {
    const LegacyState& L = c.legacy;
    const GLMatrix& mv = L.modelview.top();
    const GLMatrix& pj = L.projection.top();
    WindowPoint out;
    float eye[4], clip[4];
    mv.Transform4(x, y, z, 1.0f, eye);
    pj.Transform4(eye[0], eye[1], eye[2], eye[3], clip);
    const float wc = clip[3];
    if (!(wc > 0.0f)) return out;  // bị cull ⇒ raster position không hợp lệ
    const float xd = clip[0] / wc, yd = clip[1] / wc, zd = clip[2] / wc;
    // Clip volume: mặc định LOWER_LEFT ⇒ x,y ∈ [−1,1]; depth [−1,1] (GL 4.x).
    if (xd < -1.0f || xd > 1.0f || yd < -1.0f || yd > 1.0f) return out;
    if (zd < -1.0f || zd > 1.0f) return out;
    const ViewportState vp = c.state.GetViewport(0);
    const float n = vp.n, f = vp.f;
    out.x = (vp.w * 0.5f) * xd + (vp.x + vp.w * 0.5f);
    out.y = (vp.h * 0.5f) * yd + (vp.y + vp.h * 0.5f);
    if (c.state.ClipDepth() == GL_ZERO_TO_ONE) out.z = (f - n) * zd + n;
    else out.z = (f - n) * 0.5f * zd + (n + f) * 0.5f;
    out.w = wc;
    out.inside = true;
    return out;
}

// Đặt current raster position = góc trên-trái của hình chữ nhật (window
// coords: x nhỏ nhất, y lớn nhất) + dữ liệu đi kèm (current color / index /
// texcoord / fog coord) — giống SetRasterAssociatedData của
// src/gl/gl_legacy_raster.cpp.
void SetRasterPosUpperLeft(Context& c, float x1, float y1, float x2, float y2) {
    LegacyState& L = c.legacy;
    const float xs[4] = {x1, x2, x2, x1};
    const float ys[4] = {y1, y1, y2, y2};
    WindowPoint best;
    bool found = false;
    for (int i = 0; i < 4; ++i) {
        const WindowPoint p = ObjectToWindow(c, xs[i], ys[i], 0.0f);
        if (!p.inside) continue;
        if (!found || p.x < best.x || (p.x == best.x && p.y > best.y)) {
            best = p;
            found = true;
        }
    }
    if (!found) {
        L.raster.valid = false;  // "the valid bit is cleared"
        return;
    }
    L.raster.x = best.x;
    L.raster.y = best.y;
    L.raster.z = best.z;
    L.raster.w = best.w;
    L.raster.valid = true;
    L.raster.depth = best.z;
    for (int i = 0; i < 4; ++i) {
        const float v = L.currentColor[i];
        L.raster.color[i] = (v < 0.0f) ? 0.0f : ((v > 1.0f) ? 1.0f : v);
    }
    L.raster.index = L.currentIndex;
    const GLuint unit = L.activeTextureUnit < 8 ? L.activeTextureUnit : 0;
    std::memcpy(L.raster.texCoord, L.currentTexCoord[unit], sizeof(L.raster.texCoord));
    if (L.fog.coordSrc == GL_FOG_COORD) {
        L.raster.fogCoord = L.fog.currentCoord;
    } else {
        const GLMatrix& mv = L.modelview.top();
        float eye[4];
        mv.Transform4(x1, y1, 0.0f, 1.0f, eye);
        L.raster.fogCoord =
            std::sqrt(eye[0] * eye[0] + eye[1] * eye[1] + eye[2] * eye[2]);
    }
}

// Rect ≡ Begin(POLYGON); Vertex2(x1,y1); Vertex2(x2,y1); Vertex2(x2,y2);
//      Vertex2(x1,y2); End();
//
// Dùng đúng chuỗi lệnh của spec (Begin/Vertex/End) ⇒ tự động có: ghi display
// list, selection/feedback, và đường draw fixed-function (glEnd render ngay
// theo spec GL 2.1 §2.6).
void RectCore(float x1, float y1, float x2, float y2) {
    Context& c = Context::Current();
    // Rect KHÔNG được phép trong glBegin/glEnd: GL 2.1 §2.6.3 (PDF p.19–20)
    // chỉ cho phép danh sách lệnh hẹp; "Executing any other GL command
    // between the execution of Begin and the corresponding execution of End
    // results in the error INVALID_OPERATION."
    if (c.legacy.inBeginEnd) {
        c.errors.Record(kInvalidOperation);
        return;
    }
    glBegin(kPolygon);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
    SetRasterPosUpperLeft(c, x1, y1, x2, y2);
}

}  // namespace

// ---------------------------------------------------------------------------
// glRect* — 8 entry point
// ---------------------------------------------------------------------------
void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
    RectCore((float)x1, (float)y1, (float)x2, (float)y2);
}

void glRectdv(const GLdouble* v1, const GLdouble* v2) {
    if (!v1 || !v2) { Context::Current().errors.Record(kInvalidValue); return; }
    glRectd(v1[0], v1[1], v2[0], v2[1]);
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
    RectCore(x1, y1, x2, y2);
}

void glRectfv(const GLfloat* v1, const GLfloat* v2) {
    if (!v1 || !v2) { Context::Current().errors.Record(kInvalidValue); return; }
    glRectf(v1[0], v1[1], v2[0], v2[1]);
}

// Biến thể integer: chuyển THẲNG sang float (Table 2.9 không áp cho toạ độ —
// GL 2.1 §2.7 PDF p.20 + Table 2.9 PDF p.59).
void glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
    RectCore((float)x1, (float)y1, (float)x2, (float)y2);
}

void glRectiv(const GLint* v1, const GLint* v2) {
    if (!v1 || !v2) { Context::Current().errors.Record(kInvalidValue); return; }
    glRecti(v1[0], v1[1], v2[0], v2[1]);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
    RectCore((float)x1, (float)y1, (float)x2, (float)y2);
}

void glRectsv(const GLshort* v1, const GLshort* v2) {
    if (!v1 || !v2) { Context::Current().errors.Record(kInvalidValue); return; }
    glRects(v1[0], v1[1], v2[0], v2[1]);
}

}  // namespace tglmt::gl
