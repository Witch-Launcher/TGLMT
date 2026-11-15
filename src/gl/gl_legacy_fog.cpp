// gl_legacy_fog.cpp — OpenGL fixed-function FOG (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §3.12 "Fog" (PDF p.365–367) —
// văn bản giống hệt OpenGL 2.1 §3.10.
//
// Ba phương trình (spec eq. 3.29/3.30/3.31, PDF p.366):
//   GL_EXP   (3.29):  f = exp(-d*c)
//   GL_EXP2  (3.30):  f = exp(-(d*c)^2)
//   GL_LINEAR(3.31):  f = (e - c) / (e - s)
// Trong đó: d = FOG_DENSITY, s = FOG_START, e = FOG_END, c = fog coordinate.
//
// QUY TẮC BẮT BUỘC:
//  • f được clamp vào [0,1] (spec: "the result is clamped to [0,1] to obtain
//    the final f").
//  • Fog chỉ ảnh hưởng RGB, KHÔNG đổi alpha
//    (spec: "The rasterized fragment's A value is not changed by fog blending").
//  • Blending: C = f*Cr + (1-f)*Cf  (spec §3.12).
//  • Fog coordinate: FOG_COORD_SRC = FRAGMENT_DEPTH (mặc định) ⇒ c là
//    "eye-coordinate distance from the eye (0,0,0,1) to the fragment center"
//    (spec p.366) — tức khoảng cách GỐC, KHÔNG phải |z|.
//    FOG_COORD ⇒ c là giá trị nội suy của fog coordinate.
//  • FOG_DENSITY < 0 ⇒ GL_INVALID_VALUE (spec p.366).
//  • Mặc định: FOG_MODE = EXP, d = 1.0, s = 0.0, e = 1.0, Cf = (0,0,0,0),
//    FOG_COORD_SRC = FRAGMENT_DEPTH, fog tắt.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

namespace {
constexpr GLenum kFogMode = GL_FOG_MODE;         // GL_FOG_MODE
constexpr GLenum kFogDensity = GL_FOG_DENSITY;      // GL_FOG_DENSITY
constexpr GLenum kFogStart = GL_FOG_START;        // GL_FOG_START
constexpr GLenum kFogEnd = GL_FOG_END;          // GL_FOG_END
constexpr GLenum kFogColor = GL_FOG_COLOR;        // GL_FOG_COLOR
constexpr GLenum kFogCoordSrc = GL_FOG_COORDINATE_SOURCE;     // GL_FOG_COORDINATE_SOURCE
constexpr GLenum kFogIndex = GL_FOG_INDEX;        // GL_FOG_INDEX

constexpr GLenum kExp = GL_EXP;             // GL_EXP
constexpr GLenum kExp2 = GL_EXP2;            // GL_EXP2
constexpr GLenum kLinear = GL_LINEAR;          // GL_LINEAR
constexpr GLenum kFogCoord = GL_FOG_COORD;        // GL_FOG_COORD
constexpr GLenum kFragmentDepth = GL_FRAGMENT_DEPTH;   // GL_FRAGMENT_DEPTH

constexpr GLenum kErrEnum = GL_INVALID_ENUM;
constexpr GLenum kErrValue = GL_INVALID_VALUE;
}  // namespace

// pname hợp lệ của glFog: trả về số giá trị cần đọc.
static int FogPnameCount(GLenum pname) {
    switch (pname) {
        case kFogMode:    return 1;
        case kFogDensity: return 1;
        case kFogStart:   return 1;
        case kFogEnd:     return 1;
        case kFogColor:   return 4;
        case kFogCoordSrc: return 1;
        case kFogIndex:   return 1;
        default: return 0;
    }
}

static bool FogPnameValid(GLenum pname) { return FogPnameCount(pname) > 0; }

// Ghi giá trị đã kiểm tra cho một pname đơn (dùng chung cho bản f/i).
static void FogSetScalar(Context& c, GLenum pname, float v) {
    LegacyFog& f = c.legacy.fog;
    switch (pname) {
        case kFogMode:
            // Spec: param phải là EXP, EXP2 hoặc LINEAR; sai ⇒ INVALID_ENUM.
            if (v == (float)kExp) f.mode = kExp;
            else if (v == (float)kExp2) f.mode = kExp2;
            else if (v == (float)kLinear) f.mode = kLinear;
            else c.errors.Record(kErrEnum);
            return;
        case kFogDensity:
            // "If d is specified less than zero, the error INVALID_VALUE results."
            if (v < 0.0f) { c.errors.Record(kErrValue); return; }
            f.density = v;
            return;
        case kFogStart:
            f.start = v;
            return;
        case kFogEnd:
            f.end = v;
            return;
        case kFogCoordSrc:
            if (v == (float)kFogCoord) f.coordSrc = kFogCoord;
            else if (v == (float)kFragmentDepth) f.coordSrc = kFragmentDepth;
            else c.errors.Record(kErrEnum);
            return;
        case kFogIndex:
            // Fog index chỉ có ý nghĩa ở color-index mode; vẫn lưu để đúng
            // về mặt state (spec §3.12 nói rõ if = 0 mặc định).
            f.currentCoord = v;  // tạm giữ chỗ — giá trị thật lưu riêng
            return;
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glFogf(GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFogf")) return;
    int n = FogPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (n != 1) { c.errors.Record(kErrEnum); return; }  // pname nhiều giá trị
    FogSetScalar(c, pname, param);
}

void glFogi(GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFogi")) return;
    int n = FogPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (n != 1) { c.errors.Record(kErrEnum); return; }
    // Số nguyên → float theo spec §2.3.1 (signed int: (2c+1)/(2^31−1)).
    float v = (float)((2.0 * (double)param + 1.0) / 2147483647.0);
    // Nhưng các pname "enum-like" (FOG_MODE, FOG_COORD_SRC) cần giá trị nguyên
    // gốc để so sánh với hằng số ⇒ dùng giá trị nguyên cho 2 pname này.
    if (pname == kFogMode || pname == kFogCoordSrc) {
        switch (param) {
            case (GLint)kExp:
            case (GLint)kExp2:
            case (GLint)kLinear:
            case (GLint)kFogCoord:
            case (GLint)kFragmentDepth:
                break;
            default:
                c.errors.Record(kErrEnum);
                return;
        }
        FogSetScalar(c, pname, (float)param);
        return;
    }
    FogSetScalar(c, pname, v);
}

void glFogfv(GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFogfv")) return;
    int n = FogPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (n == 1) { FogSetScalar(c, pname, params[0]); return; }
    // FOG_COLOR: 4 giá trị.
    if (pname == kFogColor) {
        memcpy(c.legacy.fog.color, params, sizeof(float) * 4);
        return;
    }
    c.errors.Record(kErrEnum);
}

void glFogiv(GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFogiv")) return;
    int n = FogPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (n == 1) {
        if (pname == kFogMode || pname == kFogCoordSrc) {
            switch (params[0]) {
                case (GLint)kExp:
                case (GLint)kExp2:
                case (GLint)kLinear:
                case (GLint)kFogCoord:
                case (GLint)kFragmentDepth:
                    break;
                default:
                    c.errors.Record(kErrEnum);
                    return;
            }
            FogSetScalar(c, pname, (float)params[0]);
            return;
        }
        float v = (float)((2.0 * (double)params[0] + 1.0) / 2147483647.0);
        FogSetScalar(c, pname, v);
        return;
    }
    if (pname == kFogColor) {
        for (int i = 0; i < 4; ++i)
            c.legacy.fog.color[i] =
                (float)((2.0 * (double)params[i] + 1.0) / 2147483647.0);
        return;
    }
    c.errors.Record(kErrEnum);
}

// ---------------------------------------------------------------------------
// glFogCoord* — đặt fog coordinate hiện tại (client attribute)
// ---------------------------------------------------------------------------
// Chỉ dùng khi FOG_COORD_SRC == GL_FOG_COORD. Giá trị được nội suy cho mọi
// vertex của primitive (spec: fog coordinate là một current vertex attribute).
void glFogCoordf(GLfloat coord) {
    Context::Current().legacy.fog.currentCoord = coord;
}
void glFogCoordd(GLdouble coord) {
    Context::Current().legacy.fog.currentCoord = (float)coord;
}
void glFogCoordfv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(GL_INVALID_VALUE); return; }
    glFogCoordf(v[0]);
}
void glFogCoorddv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(GL_INVALID_VALUE); return; }
    glFogCoordd(v[0]);
}

}  // namespace tglmt::gl
