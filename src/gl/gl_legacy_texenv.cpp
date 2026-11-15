// gl_legacy_texenv.cpp — OpenGL fixed-function TEXTURE ENVIRONMENT & GENERATION.
//
// Spec: OpenGL 1.3 §3.8.12 (Texture Environments and Texture Functions) —
// PDF p.146–151; OpenGL 4.2 Compatibility Profile §3.10.17 — PDF p.347–352.
//
// Đã đối chiếu trực tiếp:
//  • Table 3.21 (GL1.3 p.148) / Table 3.25 (4.2C p.348): base color Cs/As từ
//    filtered texel.
//  • Table 3.22 (GL1.3 p.148) / 3.26 (4.2C p.349): REPLACE/MODULATE/DECAL.
//  • Table 3.23 (GL1.3 p.149) / 3.27 (4.2C p.349): BLEND/ADD.
//  • Table 3.24 (GL1.3 p.150) / 3.28 (4.2C p.351): COMBINE_RGB/COMBINE_ALPHA.
//  • Table 3.25 (GL1.3 p.151) / 3.29 (4.2C p.352): arguments cho COMBINE_RGB.
//  • Table 3.26 (GL1.3 p.151) / 3.30 (4.2C p.352): arguments cho COMBINE_ALPHA.
//  • GL 1.3 §2.11 (PDF p.38–39): phương trình texture coordinate generation.
//
// CÔNG THỨC (viết đúng theo spec):
//   OBJECT_LINEAR:  g = p1*xo + p2*yo + p3*zo + p4*wo   (toạ độ OBJECT)
//   EYE_LINEAR:    g = p1'*xe + p2'*ye + p3'*ze + p4'*we, với (p') = (p)·M⁻¹
//                  (M = modelview TẠI LÚC gọi glTexGen với pname=EYE_PLANE)
//   SPHERE_MAP:    u = normalize(vertex eye coords);  r = u − 2·n'·(n'·u);
//                  m = 2/sqrt(rx²+ry²+(rz+1)²);  s = rx/m + 0.5;  t = ry/m + 0.5
//
// MẶC ĐỊNH (spec GL 1.3 §3.8.12 p.147):
//   texture function = MODULATE; combiner RGB/ALPHA = MODULATE;
//   source 0/1/2 = TEXTURE/PREVIOUS/CONSTANT;
//   operand RGB 0/1 = SRC_COLOR, operand RGB 2 = SRC_ALPHA;
//   operand ALPHA 0/1/2 = SRC_ALPHA;
//   env color = (0,0,0,0).
// TexGen mặc định (spec GL 1.3 §2.11 p.39): tất cả tọa độ EYE_LINEAR, không
// bật; p của S = (1,0,0,0), của T = (0,1,0,0), của R/Q = (0,0,0,0).
#include "tglmt/gl46.h"
#include "tglmt/LegacyDisplayList.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

namespace {
constexpr GLenum kTextureEnv = GL_TEXTURE_ENV;       // GL_TEXTURE_ENV
constexpr GLenum kTextureEnvMode = GL_TEXTURE_ENV_MODE;   // GL_TEXTURE_ENV_MODE
constexpr GLenum kTextureEnvColor = GL_TEXTURE_ENV_COLOR;  // GL_TEXTURE_ENV_COLOR
constexpr GLenum kCombineRGB = GL_COMBINE_RGB;       // GL_COMBINE_RGB
constexpr GLenum kCombineAlpha = GL_COMBINE_ALPHA;     // GL_COMBINE_ALPHA
constexpr GLenum kRgbScale = GL_RGB_SCALE;         // GL_RGB_SCALE
constexpr GLenum kAlphaScale = GL_ALPHA_SCALE;       // GL_ALPHA_SCALE

constexpr GLenum kSrc0RGB = GL_SRC0_RGB, kSrc1RGB = GL_SRC1_RGB, kSrc2RGB = GL_SRC2_RGB;
constexpr GLenum kSrc0Alpha = GL_SRC0_ALPHA, kSrc1Alpha = GL_SRC1_ALPHA, kSrc2Alpha = GL_SRC2_ALPHA;
constexpr GLenum kOperand0RGB = GL_OPERAND0_RGB, kOperand1RGB = GL_OPERAND1_RGB, kOperand2RGB = GL_OPERAND2_RGB;
constexpr GLenum kOperand0Alpha = GL_OPERAND0_ALPHA, kOperand1Alpha = GL_OPERAND1_ALPHA, kOperand2Alpha = GL_OPERAND2_ALPHA;

// Nguồn (SOURCE) của combiner: TEXTURE / PREVIOUS / CONSTANT / PRIMARY_COLOR.
constexpr GLenum kSrcTexture = GL_TEXTURE;       // GL_TEXTURE
constexpr GLenum kSrcPrevious = GL_PREVIOUS;      // GL_PREVIOUS
constexpr GLenum kSrcConstant = GL_CONSTANT;      // GL_CONSTANT
constexpr GLenum kSrcPrimaryColor = GL_PRIMARY_COLOR;  // GL_PRIMARY_COLOR

// Operand của combiner.
constexpr GLenum kOpSrcColor = GL_SRC_COLOR;          // GL_SRC_COLOR
constexpr GLenum kOpOneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR;  // GL_ONE_MINUS_SRC_COLOR
constexpr GLenum kOpSrcAlpha = GL_SRC_ALPHA;          // GL_SRC_ALPHA
constexpr GLenum kOpOneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA;  // GL_ONE_MINUS_SRC_ALPHA

// Combiner functions.
constexpr GLenum kFuncReplace = GL_REPLACE;      // GL_REPLACE
constexpr GLenum kFuncModulate = GL_MODULATE;     // GL_MODULATE
constexpr GLenum kFuncAdd = GL_ADD;          // GL_ADD
constexpr GLenum kFuncAddSigned = GL_ADD_SIGNED;   // GL_ADD_SIGNED
constexpr GLenum kFuncInterpolate = GL_INTERPOLATE;  // GL_INTERPOLATE
constexpr GLenum kFuncSubtract = GL_SUBTRACT;     // GL_SUBTRACT
constexpr GLenum kFuncDot3RGB = GL_DOT3_RGB;      // GL_DOT3_RGB
constexpr GLenum kFuncDot3RGBA = GL_DOT3_RGBA;     // GL_DOT3_RGBA

// TexGen.
constexpr GLenum kCoordS = GL_S, kCoordT = GL_T, kCoordR = GL_R, kCoordQ = GL_Q;
constexpr GLenum kTextureGenMode = GL_TEXTURE_GEN_MODE;  // GL_TEXTURE_GEN_MODE
constexpr GLenum kObjectPlane = GL_OBJECT_PLANE;     // GL_OBJECT_PLANE
constexpr GLenum kEyePlane = GL_EYE_PLANE;        // GL_EYE_PLANE
constexpr GLenum kObjectLinear = GL_OBJECT_LINEAR;    // GL_OBJECT_LINEAR
constexpr GLenum kEyeLinear = GL_EYE_LINEAR;       // GL_EYE_LINEAR
constexpr GLenum kSphereMap = GL_SPHERE_MAP;       // GL_SPHERE_MAP
constexpr GLenum kReflectionMap = GL_REFLECTION_MAP;   // GL_REFLECTION_MAP
constexpr GLenum kNormalMap = GL_NORMAL_MAP;       // GL_NORMAL_MAP

constexpr GLenum kErrEnum = GL_INVALID_ENUM;
constexpr GLenum kErrValue = GL_INVALID_VALUE;

constexpr int kNumUnits = 8;
constexpr int kNumCoords = 4;
}  // namespace

// TexGen: mặc định theo spec GL 1.3 §2.11 p.39 — mode = EYE_LINEAR cho mọi
// coord, plane p của S = (1,0,0,0), của T = (0,1,0,0), của R/Q = (0,0,0,0).
// Hiện LegacyTexEnvCoord::mode mặc định GL_NONE (0x2400 = GL_OBJECT_LINEAR!)
// — sai. Sửa ở đây bằng hàm khởi tạo lại state texGen khi cần.
void LegacyResetTexGenDefaults(tglmt::LegacyTexGen& tg) {
    for (int i = 0; i < kNumCoords; ++i) {
        tg.coord[i].mode = kEyeLinear;
        tg.coord[i].plane[0] = tg.coord[i].plane[1] =
            tg.coord[i].plane[2] = tg.coord[i].plane[3] = 0.0f;
    }
    tg.coord[0].plane[0] = 1.0f;  // S: p1 = 1
    tg.coord[1].plane[1] = 1.0f;  // T: p2 = 1
}

// ---------------------------------------------------------------------------
// glTexEnv — texture environment của texture unit ĐANG HOẠT ĐỘNG
// ---------------------------------------------------------------------------
// target: GL_TEXTURE_ENV (GL 1.0–1.2) hoặc GL_TEXTURE_ENV (là texture unit
// đang hoạt động từ GL 1.3 trở lại). GL_TEXTURE_ENV_COLOR /
// GL_TEXTURE_FILTER_CONTROL / GL_POINT_SPRITE không phải "target" của
// glTexEnv trong GL desktop (chúng là pname/target riêng của glTexEnv2).
static bool TexEnvTargetValid(GLenum target) {
    return target == kTextureEnv;
}

// pname hợp lệ + số giá trị.
static int TexEnvPnameCount(GLenum pname) {
    switch (pname) {
        case kTextureEnvMode: return 1;
        case kTextureEnvColor: return 4;
        case kCombineRGB: return 1;
        case kCombineAlpha: return 1;
        case kRgbScale: return 1;
        case kAlphaScale: return 1;
        case kSrc0RGB: case kSrc1RGB: case kSrc2RGB: return 1;
        case kSrc0Alpha: case kSrc1Alpha: case kSrc2Alpha: return 1;
        case kOperand0RGB: case kOperand1RGB: case kOperand2RGB: return 1;
        case kOperand0Alpha: case kOperand1Alpha: case kOperand2Alpha: return 1;
        default: return 0;
    }
}

// Kiểm tra giá trị hợp lệ cho từng pname (spec 4.2C §3.10.17 p.347).
static bool TexEnvValueValid(GLenum pname, GLenum v) {
    switch (pname) {
        case kTextureEnvMode:
            return v == kFuncReplace || v == kFuncModulate || v == GL_DECAL ||
                   v == GL_BLEND || v == kFuncAdd || v == GL_COMBINE;
        case kCombineRGB:
            return v == kFuncReplace || v == kFuncModulate || v == kFuncAdd ||
                   v == kFuncAddSigned || v == kFuncInterpolate || v == kFuncSubtract ||
                   v == kFuncDot3RGB || v == kFuncDot3RGBA;
        case kCombineAlpha:
            return v == kFuncReplace || v == kFuncModulate || v == kFuncAdd ||
                   v == kFuncAddSigned || v == kFuncInterpolate || v == kFuncSubtract;
        case kRgbScale: case kAlphaScale:
            // GL 1.3: 1, 2 hoặc 4 (giá trị float).
            return v == 1.0f || v == 2.0f || v == 4.0f;
        case kSrc0RGB: case kSrc1RGB: case kSrc2RGB:
        case kSrc0Alpha: case kSrc1Alpha: case kSrc2Alpha:
            return v == kSrcTexture || v == kSrcPrevious || v == kSrcConstant ||
                   v == kSrcPrimaryColor;
        case kOperand0RGB: case kOperand1RGB: case kOperand2RGB:
        case kOperand0Alpha: case kOperand1Alpha: case kOperand2Alpha:
            return v == kOpSrcColor || v == kOpOneMinusSrcColor || v == kOpSrcAlpha ||
                   v == kOpOneMinusSrcAlpha;
        default:
            return true;
    }
}

// Ghi 1 giá trị đã kiểm tra cho pname của texture unit `unit`.
static void TexEnvSetUnit(LegacyTexEnv& e, GLenum pname, GLenum v) {
    switch (pname) {
        case kTextureEnvMode: e.mode = v; return;
        case kCombineRGB: e.combineRGB = v; return;
        case kCombineAlpha: e.combineAlpha = v; return;
        case kRgbScale: e.rgbScale = v; return;
        case kAlphaScale: e.alphaScale = v; return;
        case kSrc0RGB: e.src0RGB = v; return;
        case kSrc1RGB: e.src1RGB = v; return;
        case kSrc2RGB: e.src2RGB = v; return;
        case kSrc0Alpha: e.src0Alpha = v; return;
        case kSrc1Alpha: e.src1Alpha = v; return;
        case kSrc2Alpha: e.src2Alpha = v; return;
        case kOperand0RGB: e.operand0RGB = v; return;
        case kOperand1RGB: e.operand1RGB = v; return;
        case kOperand2RGB: e.operand2RGB = v; return;
        case kOperand0Alpha: e.operand0Alpha = v; return;
        case kOperand1Alpha: e.operand1Alpha = v; return;
        case kOperand2Alpha: e.operand2Alpha = v; return;
        default: return;
    }
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexEnvi")) return;
    {   // Ghi display list (xem docs/DECISIONS.md: lệnh ghi ở đầu, lỗi chuyển
        // sang thời điểm replay — giới hạn đã biết, xem PROGRESS.md).
        GLfloat rec = (GLfloat)param;
        if (LegacyRecordTexEnv(target, pname, &rec, 1)) return;
    }
    if (!TexEnvTargetValid(target)) { c.errors.Record(kErrEnum); return; }
    int n = TexEnvPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (n != 1) { c.errors.Record(kErrEnum); return; }
    if (!TexEnvValueValid(pname, (GLenum)param)) { c.errors.Record(kErrEnum); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    if (pname == kTextureEnvColor) { c.errors.Record(kErrEnum); return; }
    TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)param);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexEnvf")) return;
    {
        GLfloat rec = param;
        if (LegacyRecordTexEnv(target, pname, &rec, 1)) return;
    }
    if (!TexEnvTargetValid(target)) { c.errors.Record(kErrEnum); return; }
    int n = TexEnvPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (n != 1) { c.errors.Record(kErrEnum); return; }
    // Các pname enum-like cần giá trị nguyên gốc.
    if (pname != kRgbScale && pname != kAlphaScale) {
        GLint iv = (GLint)param;
        if ((float)iv != param) { c.errors.Record(kErrEnum); return; }
        if (!TexEnvValueValid(pname, (GLenum)iv)) { c.errors.Record(kErrEnum); return; }
        GLuint unit = c.legacy.activeTextureUnit & 7u;
        TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)iv);
        return;
    }
    // RGB_SCALE / ALPHA_SCALE: float 1/2/4.
    if (!TexEnvValueValid(pname, param)) { c.errors.Record(kErrEnum); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)param);
}

void glTexEnviv(GLenum target, GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexEnviv")) return;
    {
        GLfloat rec[4] = {0, 0, 0, 0};
        int nrec = (params && TexEnvPnameCount(pname) > 0 && TexEnvPnameCount(pname) <= 4)
                       ? TexEnvPnameCount(pname) : 1;
        for (int i = 0; i < nrec && params; ++i)
            rec[i] = (float)((2.0 * (double)params[i] + 1.0) / 2147483647.0);
        if (LegacyRecordTexEnv(target, pname, rec, nrec)) return;
    }
    if (!TexEnvTargetValid(target)) { c.errors.Record(kErrEnum); return; }
    int n = TexEnvPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    if (n == 1) {
        if (!TexEnvValueValid(pname, (GLenum)params[0])) { c.errors.Record(kErrEnum); return; }
        TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)params[0]);
        return;
    }
    // TEXTURE_ENV_COLOR: 4 giá trị.
    if (pname == kTextureEnvColor) {
        for (int i = 0; i < 4; ++i)
            c.legacy.texEnv[unit].color[i] =
                (float)((2.0 * (double)params[i] + 1.0) / 2147483647.0);
        return;
    }
    c.errors.Record(kErrEnum);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexEnvfv")) return;
    {
        GLfloat rec[4] = {0, 0, 0, 0};
        int nrec = (params && TexEnvPnameCount(pname) > 0 && TexEnvPnameCount(pname) <= 4)
                       ? TexEnvPnameCount(pname) : 1;
        for (int i = 0; i < nrec && params; ++i) rec[i] = params[i];
        if (LegacyRecordTexEnv(target, pname, rec, nrec)) return;
    }
    if (!TexEnvTargetValid(target)) { c.errors.Record(kErrEnum); return; }
    int n = TexEnvPnameCount(pname);
    if (n == 0) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    if (n == 1) {
        if (pname == kRgbScale || pname == kAlphaScale) {
            if (!TexEnvValueValid(pname, params[0])) { c.errors.Record(kErrEnum); return; }
            TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)params[0]);
            return;
        }
        GLint iv = (GLint)params[0];
        if ((float)iv != params[0]) { c.errors.Record(kErrEnum); return; }
        if (!TexEnvValueValid(pname, (GLenum)iv)) { c.errors.Record(kErrEnum); return; }
        TexEnvSetUnit(c.legacy.texEnv[unit], pname, (GLenum)iv);
        return;
    }
    if (pname == kTextureEnvColor) {
        // Spec GL 1.3 §3.8.12 p.147: giá trị ngoài [0,1] bị CLAMP vào [0,1].
        for (int i = 0; i < 4; ++i) {
            float v = params[i];
            c.legacy.texEnv[unit].color[i] = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        }
        return;
    }
    c.errors.Record(kErrEnum);
}

// ---------------------------------------------------------------------------
// glTexGen — texture coordinate generation
// ---------------------------------------------------------------------------
// coord: GL_S / GL_T / GL_R / GL_Q. pname: GL_TEXTURE_GEN_MODE /
// GL_OBJECT_PLANE / GL_EYE_PLANE.
static bool TexGenCoordIndex(GLenum coord, int& out) {
    switch (coord) {
        case kCoordS: out = 0; return true;
        case kCoordT: out = 1; return true;
        case kCoordR: out = 2; return true;
        case kCoordQ: out = 3; return true;
        default: return false;
    }
}

// Chuyển mode → tên để dùng cho thông báo lỗi.
static const char* TexGenModeName(GLenum m) {
    switch (m) {
        case kObjectLinear: return "GL_OBJECT_LINEAR";
        case kEyeLinear: return "GL_EYE_LINEAR";
        case kSphereMap: return "GL_SPHERE_MAP";
        case kReflectionMap: return "GL_REFLECTION_MAP";
        case kNormalMap: return "GL_NORMAL_MAP";
        default: return "?";
    }
}

void glTexGeni(GLenum coord, GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGeni")) return;
    {
        GLfloat rec = (GLfloat)param;
        if (LegacyRecordTexGen(coord, pname, &rec, 1)) return;
    }
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode: {
            switch (param) {
                case kObjectLinear: case kEyeLinear: case kSphereMap:
                case kReflectionMap: case kNormalMap:
                    break;
                default:
                    c.errors.Record(kErrEnum);
                    return;
            }
            // SPHERE_MAP / REFLECTION_MAP / NORMAL_MAP không hợp lệ với R, Q
            // (spec GL 1.3 §2.11 p.38–39: "Calling TexGen with a coord of either
            //  R or Q when pname indicates SPHERE MAP generates the error
            //  INVALID_ENUM").
            if ((param == kSphereMap || param == kReflectionMap || param == kNormalMap) &&
                (ci == 2 || ci == 3)) {
                c.errors.Record(kErrEnum);
                return;
            }
            tg.coord[ci].mode = (GLenum)param;
            return;
        }
        case kObjectPlane:
        case kEyePlane: {
            // p1..p4 — mặc định S=(1,0,0,0), T=(0,1,0,0), R/Q=(0,0,0,0).
            // Spec: "The initial values of pi for s are all 0 except p1 which is
            // one; for t all the pi are zero except p2, which is 1."
            tg.coord[ci].plane[0] = (float)param;
            tg.coord[ci].plane[1] = 0.0f;
            tg.coord[ci].plane[2] = 0.0f;
            tg.coord[ci].plane[3] = 0.0f;
            return;
        }
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGenf")) return;
    {
        GLfloat rec = param;
        if (LegacyRecordTexGen(coord, pname, &rec, 1)) return;
    }
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode: {
            GLint iv = (GLint)param;
            if ((float)iv != param) { c.errors.Record(kErrEnum); return; }
            glTexGeni(coord, pname, iv);
            return;
        }
        case kObjectPlane:
        case kEyePlane: {
            // Bản scalar chỉ đặt p1; p2..p4 = 0 (xem ghi chú ở glTexGeni).
            tg.coord[ci].plane[0] = param;
            tg.coord[ci].plane[1] = 0.0f;
            tg.coord[ci].plane[2] = 0.0f;
            tg.coord[ci].plane[3] = 0.0f;
            return;
        }
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGeniv")) return;
    {
        GLfloat rec[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4 && params; ++i)
            rec[i] = (float)((2.0 * (double)params[i] + 1.0) / 2147483647.0);
        if (LegacyRecordTexGen(coord, pname, rec, 4)) return;
    }
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode:
            glTexGeni(coord, pname, params[0]);
            return;
        case kObjectPlane:
        case kEyePlane:
            for (int i = 0; i < 4; ++i)
                tg.coord[ci].plane[i] = (float)((2.0 * (double)params[i] + 1.0) / 2147483647.0);
            return;
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glTexGendv(GLenum coord, GLenum pname, const GLdouble* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGendv")) return;
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode: {
            GLint iv = (GLint)params[0];
            if ((double)iv != params[0]) { c.errors.Record(kErrEnum); return; }
            glTexGeni(coord, pname, iv);
            return;
        }
        case kObjectPlane:
        case kEyePlane:
            for (int i = 0; i < 4; ++i) tg.coord[ci].plane[i] = (float)params[i];
            return;
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glTexGend(GLenum coord, GLenum pname, GLdouble param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGend")) return;
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode: {
            GLint iv = (GLint)param;
            if ((double)iv != param) { c.errors.Record(kErrEnum); return; }
            glTexGeni(coord, pname, iv);
            return;
        }
        case kObjectPlane:
        case kEyePlane:
            tg.coord[ci].plane[0] = (float)param;
            tg.coord[ci].plane[1] = 0.0f;
            tg.coord[ci].plane[2] = 0.0f;
            tg.coord[ci].plane[3] = 0.0f;
            return;
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexGenfv")) return;
    {
        GLfloat rec[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4 && params; ++i) rec[i] = params[i];
        if (LegacyRecordTexGen(coord, pname, rec, 4)) return;
    }
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kErrEnum); return; }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    GLuint unit = c.legacy.activeTextureUnit & 7u;
    LegacyTexGen& tg = c.legacy.texGen[unit];
    switch (pname) {
        case kTextureGenMode:
            glTexGeni(coord, pname, (GLint)params[0]);
            return;
        case kObjectPlane:
        case kEyePlane:
            for (int i = 0; i < 4; ++i) tg.coord[ci].plane[i] = params[i];
            return;
        default:
            c.errors.Record(kErrEnum);
            return;
    }
}

}  // namespace tglmt::gl
