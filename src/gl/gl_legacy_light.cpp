// gl_legacy_light.cpp — OpenGL fixed-function LIGHTING & MATERIALS.
//
// Spec: OpenGL 2.1 §2.14 (Fixed-Function Vertex Lighting and Coloring) —
// văn bản giữ nguyên trong OpenGL 4.2/4.6 Compatibility Profile.
// Đã đối chiếu trực tiếp với glspec21.pdf (PDF p.73–85) và
// glspec42.compatibility PDF p.106–108 + bảng state 6.17/6.18 (p.520–521).
//
// Mọi công thức bên dưới ghi theo ký hiệu của spec:
//   acm/dcm/scm/ecm/srm = material (front|back)
//   acli/dcli/scli      = light i
//   Ppli/sdli/srli/crli/k0i/k1i/k2i = light i
//   acs = scene ambient, vbs = local viewer, tbs = two-side, ces = color control
//
// LƯU Ý QUAN TRỌNG về mặc định (spec Table 2.10, glspec21 PDF p.61):
//   • material ambient = (0.2,0.2,0.2,1), diffuse = (0.8,0.8,0.8,1)
//   • dcli/scli của LIGHT0 = (1,1,1,1), của LIGHTi (i>0) = (0,0,0,1)
//   • Ppli mặc định = (0,0,1,0)  (w=0 ⇒ nguồn sáng ở vô cùng)
//   • vbs (LOCAL_VIEWER) mặc định = FALSE
//   • acs (scene ambient) mặc định = (0.2,0.2,0.2,1)
#include "tglmt/gl46.h"
#include "tglmt/LegacyDisplayList.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

// GLenum hằng số dùng nhiều — định nghĩa tại chỗ để file độc lập với
// gl_legacy_types.h (nơi chúng có thể đã đổi tên theo gen_gl_headers.py).
namespace {
constexpr GLenum kLight0 = GL_LIGHT0;         // GL_LIGHT0
constexpr GLenum kLightMax = GL_LIGHT7;       // GL_LIGHT7
constexpr GLenum kSpotDirection = GL_SPOT_DIRECTION;  // GL_SPOT_DIRECTION
constexpr GLenum kSpotExponent = GL_SPOT_EXPONENT;   // GL_SPOT_EXPONENT
constexpr GLenum kSpotCutoff = GL_SPOT_CUTOFF;     // GL_SPOT_CUTOFF
constexpr GLenum kPosition = GL_POSITION;       // GL_POSITION
constexpr GLenum kAmbient = GL_AMBIENT;        // GL_AMBIENT
constexpr GLenum kDiffuse = GL_DIFFUSE;        // GL_DIFFUSE
constexpr GLenum kSpecular = GL_SPECULAR;       // GL_SPECULAR
constexpr GLenum kEmission = GL_EMISSION;       // GL_EMISSION
constexpr GLenum kShininess = GL_SHININESS;      // GL_SHININESS
constexpr GLenum kAmbientAndDiffuse = GL_AMBIENT_AND_DIFFUSE;
constexpr GLenum kColorIndexes = GL_COLOR_INDEXES;   // GL_COLOR_INDEXES
constexpr GLenum kFront = GL_FRONT;          // GL_FRONT
constexpr GLenum kBack = GL_BACK;           // GL_BACK
constexpr GLenum kFrontAndBack = GL_FRONT_AND_BACK;   // GL_FRONT_AND_BACK
constexpr GLenum kLightModelAmbient = GL_LIGHT_MODEL_AMBIENT;        // GL_LIGHT_MODEL_AMBIENT
constexpr GLenum kLightModelLocalViewer = GL_LIGHT_MODEL_LOCAL_VIEWER;   // GL_LIGHT_MODEL_LOCAL_VIEWER
constexpr GLenum kLightModelTwoSide = GL_LIGHT_MODEL_TWO_SIDE;       // GL_LIGHT_MODEL_TWO_SIDE
constexpr GLenum kLightModelColorControl = GL_LIGHT_MODEL_COLOR_CONTROL;  // GL_LIGHT_MODEL_COLOR_CONTROL
constexpr GLenum kSingleColor = GL_SINGLE_COLOR;             // GL_SINGLE_COLOR
constexpr GLenum kSeparateSpecular = GL_SEPARATE_SPECULAR_COLOR;        // GL_SEPARATE_SPECULAR_COLOR
constexpr GLenum kSmooth = GL_SMOOTH;                  // GL_SMOOTH
constexpr GLenum kFlat = GL_FLAT;                    // GL_FLAT

constexpr GLenum kErrEnum = GL_INVALID_ENUM;      // GL_INVALID_ENUM
constexpr GLenum kErrValue = GL_INVALID_VALUE;     // GL_INVALID_VALUE
constexpr GLenum kErrOperation = GL_INVALID_OPERATION; // GL_INVALID_OPERATION

// Chuyển đổi kiểu số nguyên → float cho THAM SỐ MÀU theo spec GL 2.1 Table 2.9
// (PDF p.59): signed int → (2c+1)/(2^31−1).
//
// LƯU Ý: Table 2.9 CHỈ áp cho COLOR parameter. Spec §2.14.2 (PDF p.64):
//   "Color parameters specified with Material and Light are converted to
//    floating-point values (if specified as integers) as indicated in table 2.9"
// Vị trí (POSITION), hướng (SPOT_DIRECTION) và các real (SHININESS,
// SPOT_EXPONENT, attenuation) chuyển TRỰC TIẾP (float)c — nếu dùng
// Table 2.9 cho chúng thì vị trí (1,0,0,1) sẽ ra 1e-9 ⇒ sai hoàn toàn.
float IntToFloat(GLint v) {
    return (float)((2.0 * (double)v + 1.0) / 2147483647.0);
}
void CopyIntsToFloats(const GLint* src, float* dst, int n) {
    for (int i = 0; i < n; ++i) dst[i] = IntToFloat(src[i]);
}
// Bản chuyển trực tiếp cho tham số không phải màu.
void CopyIntsDirect(const GLint* src, float* dst, int n) {
    for (int i = 0; i < n; ++i) dst[i] = (float)src[i];
}
}  // namespace

// ---------------------------------------------------------------------------
// Helper: light index từ enum GL_LIGHTi
// ---------------------------------------------------------------------------
static bool LightIndex(GLenum light, int& out) {
    if (light < kLight0 || light > kLightMax) return false;
    out = (int)(light - kLight0);
    return true;
}

// Transform vị trí nguồn sáng bằng modelview HIỆN TẠI (spec §2.14.2):
// "The current model-view matrix is applied to the position parameter
//  indicated with Light for a particular light source when that position is
//  specified. These transformed values are the values used in the lighting
//  equation."
// ⇒ biến đổi tại thời điểm gọi glLight, không phải tại thời điểm vẽ.
static void TransformLightPosition(const GLMatrix& mv, float pos[4]) {
    float out[4];
    mv.Transform4(pos[0], pos[1], pos[2], pos[3], out);
    for (int i = 0; i < 4; ++i) pos[i] = out[i];
}

// Spot direction: chỉ 3x3 trên-trái của modelview (spec §2.14.2):
//   d' = Mu * d   (Mu = upper-left 3x3 của modelview), KHÔNG chuẩn hoá.
static void TransformSpotDirection(const GLMatrix& mv, float dir[3]) {
    const float* m = mv.m;
    // column-major: cột 0 = (m0,m1,m2), cột 1 = (m4,m5,m6), cột 2 = (m8,m9,m10)
    float x = m[0] * dir[0] + m[4] * dir[1] + m[8] * dir[2];
    float y = m[1] * dir[0] + m[5] * dir[1] + m[9] * dir[2];
    float z = m[2] * dir[0] + m[6] * dir[1] + m[10] * dir[2];
    dir[0] = x; dir[1] = y; dir[2] = z;
}

// ---------------------------------------------------------------------------
// glLight — đặt tham số của một nguồn sáng
// ---------------------------------------------------------------------------
// pname → (số giá trị, con trỏ tới mảng float trong LegacyLight).
// Dùng con trỏ thẳng vào struct (thay vì pointer-to-member) vì phần tử là
// MẢNG — kiểu `float (T::*)[4]` không gán được cho `float T::*`.
static bool LightPnameSlot(const LegacyLight& L, GLenum pname, int& count, const float*& out) {
    switch (pname) {
        case kAmbient:       count = 4; out = L.ambient; return true;
        case kDiffuse:       count = 4; out = L.diffuse; return true;
        case kSpecular:      count = 4; out = L.specular; return true;
        case kPosition:      count = 4; out = L.position; return true;
        case kSpotDirection: count = 3; out = L.spotDir; return true;
        case kSpotExponent:  count = 1; out = &L.spotExponent; return true;
        case kSpotCutoff:    count = 1; out = &L.spotCutoff; return true;
        case 0x1207 /*GL_CONSTANT_ATTENUATION*/: count = 1; out = &L.attConst; return true;
        case 0x1208 /*GL_LINEAR_ATTENUATION*/:    count = 1; out = &L.attLinear; return true;
        case 0x1209 /*GL_QUADRATIC_ATTENUATION*/: count = 1; out = &L.attQuad; return true;
        default: return false;
    }
}

// Bản ghi (mutable) của cùng bảng trên.
static bool LightPnameSlotMut(LegacyLight& L, GLenum pname, int& count, float*& out) {
    const float* c = nullptr;
    if (!LightPnameSlot(L, pname, count, c)) return false;
    out = const_cast<float*>(c);
    return true;
}

// Kiểm tra giá trị nằm trong khoảng cho phép (spec Table 2.10):
//   srli ∈ [0,128]; crli ∈ [0,90] ∪ {180}; k0i,k1i,k2i ∈ [0,∞).
static bool LightValueInRange(GLenum pname, float v) {
    switch (pname) {
        case kSpotExponent:
            return v >= 0.0f && v <= 128.0f;
        case kSpotCutoff:
            return (v >= 0.0f && v <= 90.0f) || v == 180.0f;
        case 0x1207: case 0x1208: case 0x1209:
            return v >= 0.0f;
        default:
            return true;  // màu: không ràng buộc (spec: range (−∞,+∞))
    }
}

void glLightf(GLenum light, GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightf")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    float* slot = nullptr;
    if (!LightPnameSlotMut(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    // "If param corresponds to a multi-valued parameter, the error INVALID_ENUM
    //  results." (spec §2.14.2)
    if (count != 1) { c.errors.Record(kErrEnum); return; }
    if (!LightValueInRange(pname, param)) { c.errors.Record(kErrValue); return; }
    if (LegacyRecordLight(light, pname, &param, 1)) return;
    LegacyLight& L = c.legacy.lights[li];
    slot[0] = param;
    // Vị trí & hướng spotlight phải transform bằng modelview tại thời điểm gọi.
    if (pname == kPosition) {
        TransformLightPosition(c.legacy.modelview.top(), L.position);
    } else if (pname == kSpotDirection) {
        TransformSpotDirection(c.legacy.modelview.top(), L.spotDir);
    }
}

void glLighti(GLenum light, GLenum pname, GLint param) {
    // Bản scalar của glLight chỉ dùng được với pname 1 giá trị (real):
    // SPOT_EXPONENT / SPOT_CUTOFF / attenuation. Các pname này chuyển kiểu
    // TRỰC TIẾP (float)param — Table 2.9 chỉ áp cho tham số MÀU.
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLighti")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    float* slot = nullptr;
    if (!LightPnameSlotMut(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (count != 1) { c.errors.Record(kErrEnum); return; }
    float v = (float)param;
    if (!LightValueInRange(pname, v)) { c.errors.Record(kErrValue); return; }
    if (LegacyRecordLight(light, pname, &v, 1)) return;
    slot[0] = v;
}

void glLightfv(GLenum light, GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightfv")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    float* slot = nullptr;
    if (!LightPnameSlotMut(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    // Kiểm tra toàn bộ giá trị trước khi ghi (spec: INVALID_VALUE nếu có giá trị
    // ngoài khoảng — lỗi phải chặn TOÀN BỘ lệnh, không ghi một phần).
    for (int i = 0; i < count; ++i) {
        if (!LightValueInRange(pname, params[i])) { c.errors.Record(kErrValue); return; }
    }
    if (LegacyRecordLight(light, pname, params, count)) return;
    LegacyLight& L = c.legacy.lights[li];
    memcpy(slot, params, sizeof(float) * (size_t)count);
    if (pname == kPosition) {
        TransformLightPosition(c.legacy.modelview.top(), L.position);
    } else if (pname == kSpotDirection) {
        TransformSpotDirection(c.legacy.modelview.top(), L.spotDir);
    }
}

void glLightiv(GLenum light, GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightiv")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    float* slot = nullptr;
    if (!LightPnameSlotMut(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    float tmp[4];
    if (count > 4) { c.errors.Record(kErrEnum); return; }
    // Chỉ tham số MÀU dùng phép (2c+1)/(2^31−1); vị trí/hướng/real thì
    // chuyển trực tiếp (spec §2.14.2).
    const bool isColor = (pname == kAmbient || pname == kDiffuse || pname == kSpecular);
    if (isColor) CopyIntsToFloats(params, tmp, count);
    else CopyIntsDirect(params, tmp, count);
    for (int i = 0; i < count; ++i) {
        if (!LightValueInRange(pname, tmp[i])) { c.errors.Record(kErrValue); return; }
    }
    if (LegacyRecordLight(light, pname, tmp, count)) return;
    LegacyLight& L = c.legacy.lights[li];
    memcpy(slot, tmp, sizeof(float) * (size_t)count);
    if (pname == kPosition) {
        TransformLightPosition(c.legacy.modelview.top(), L.position);
    } else if (pname == kSpotDirection) {
        TransformSpotDirection(c.legacy.modelview.top(), L.spotDir);
    }
}

// ---------------------------------------------------------------------------
// glMaterial — đặt tham số vật liệu
// ---------------------------------------------------------------------------
// Con trỏ thẳng vào struct (mảng float không dùng pointer-to-member được).
static bool MaterialPnameSlot(const LegacyMaterial& M, GLenum pname, int& count,
                               const float*& out) {
    switch (pname) {
        case kAmbient:          count = 4; out = M.ambient; return true;
        case kDiffuse:          count = 4; out = M.diffuse; return true;
        case kSpecular:         count = 4; out = M.specular; return true;
        case kEmission:         count = 4; out = M.emission; return true;
        case kShininess:        count = 1; out = &M.shininess; return true;
        case kAmbientAndDiffuse: count = 4; out = M.ambient; return true;  // xử lý riêng
        case kColorIndexes:     count = 3; out = M.colorIndexes; return true;
        default: return false;
    }
}
static bool MaterialPnameSlotMut(LegacyMaterial& M, GLenum pname, int& count, float*& out) {
    const float* c = nullptr;
    if (!MaterialPnameSlot(M, pname, count, c)) return false;
    out = const_cast<float*>(c);
    return true;
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    // glMaterial ĐƯỢC PHÉP trong Begin/End (spec GL 2.1 §2.6.3: "commands for
    // specifying lighting material parameters (Material commands)").
    Context& c = Context::Current();
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    float* slot = nullptr;
    if (!MaterialPnameSlotMut(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (count != 1) { c.errors.Record(kErrEnum); return; }
    // Shininess ∈ [0,128] (spec Table 2.10).
    if (pname == kShininess && (param < 0.0f || param > 128.0f)) {
        c.errors.Record(kErrValue); return;
    }
    if (LegacyRecordMaterial(face, pname, &param, 1)) return;
    if (pname == kAmbientAndDiffuse) {
        // "AMBIENT_AND_DIFFUSE is used to set acm and dcm to the same value."
        // (spec Table 2.11 note) — nhưng đây là pname 4 giá trị, nên bản "f"
        // (một giá trị) phải sinh INVALID_ENUM (multi-valued).
        c.errors.Record(kErrEnum); return;
    }
    LegacyMaterial* targets[2] = {nullptr, nullptr};
    int n = 0;
    if (face == kFront) targets[n++] = &c.legacy.materialFront;
    else if (face == kBack) targets[n++] = &c.legacy.materialBack;
    else { targets[n++] = &c.legacy.materialFront; targets[n++] = &c.legacy.materialBack; }
    for (int i = 0; i < n; ++i) {
        float* tslot = nullptr;
        int tcount = 0;
        MaterialPnameSlotMut(*targets[i], pname, tcount, tslot);
        tslot[0] = param;
    }
}

void glMateriali(GLenum face, GLenum pname, GLint param) {
    // Bản scalar chỉ dùng được với pname 1 giá trị (real): SHININESS hoặc
    // COLOR_INDEXES. Chuyển kiểu TRỰC TIẾP (Table 2.9 chỉ áp cho màu).
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMateriali")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    float* slot = nullptr;
    if (!MaterialPnameSlotMut(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (count != 1) { c.errors.Record(kErrEnum); return; }
    float v = (float)param;
    if (pname == kShininess && (v < 0.0f || v > 128.0f)) {
        c.errors.Record(kErrValue); return;
    }
    if (LegacyRecordMaterial(face, pname, &v, 1)) return;
    LegacyMaterial* targets[2] = {nullptr, nullptr};
    int n = 0;
    if (face == kFront) targets[n++] = &c.legacy.materialFront;
    else if (face == kBack) targets[n++] = &c.legacy.materialBack;
    else { targets[n++] = &c.legacy.materialFront; targets[n++] = &c.legacy.materialBack; }
    for (int i = 0; i < n; ++i) {
        float* tslot = nullptr;
        int tcount = 0;
        MaterialPnameSlotMut(*targets[i], pname, tcount, tslot);
        tslot[0] = v;
    }
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMaterialfv")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    float* slot = nullptr;
    if (!MaterialPnameSlotMut(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (pname == kShininess && (params[0] < 0.0f || params[0] > 128.0f)) {
        c.errors.Record(kErrValue); return;
    }
    if (LegacyRecordMaterial(face, pname, params, count)) return;
    LegacyMaterial* targets[2] = {nullptr, nullptr};
    int n = 0;
    if (face == kFront) targets[n++] = &c.legacy.materialFront;
    else if (face == kBack) targets[n++] = &c.legacy.materialBack;
    else { targets[n++] = &c.legacy.materialFront; targets[n++] = &c.legacy.materialBack; }
    if (pname == kAmbientAndDiffuse) {
        // Set cả acm và dcm = cùng giá trị (spec Table 2.11 note).
        for (int i = 0; i < n; ++i) {
            memcpy(targets[i]->ambient, params, sizeof(float) * 4);
            memcpy(targets[i]->diffuse, params, sizeof(float) * 4);
        }
        return;
    }
    for (int i = 0; i < n; ++i) {
        float* tslot = nullptr;
        int tcount = 0;
        MaterialPnameSlotMut(*targets[i], pname, tcount, tslot);
        memcpy(tslot, params, sizeof(float) * (size_t)count);
    }
}

void glMaterialiv(GLenum face, GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMaterialiv")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    float* slot = nullptr;
    if (!MaterialPnameSlotMut(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    float tmp[4];
    if (count > 4) { c.errors.Record(kErrEnum); return; }
    // Chỉ tham số MÀU dùng Table 2.9; real (shininess, color indexes) thì
    // chuyển trực tiếp (spec §2.14.2).
    const bool isColor = (pname == kAmbient || pname == kDiffuse ||
                          pname == kSpecular || pname == kEmission ||
                          pname == kAmbientAndDiffuse);
    if (isColor) CopyIntsToFloats(params, tmp, count);
    else CopyIntsDirect(params, tmp, count);
    if (pname == kShininess && (tmp[0] < 0.0f || tmp[0] > 128.0f)) {
        c.errors.Record(kErrValue); return;
    }
    if (LegacyRecordMaterial(face, pname, tmp, count)) return;
    LegacyMaterial* targets[2] = {nullptr, nullptr};
    int n = 0;
    if (face == kFront) targets[n++] = &c.legacy.materialFront;
    else if (face == kBack) targets[n++] = &c.legacy.materialBack;
    else { targets[n++] = &c.legacy.materialFront; targets[n++] = &c.legacy.materialBack; }
    if (pname == kAmbientAndDiffuse) {
        for (int i = 0; i < n; ++i) {
            memcpy(targets[i]->ambient, tmp, sizeof(float) * 4);
            memcpy(targets[i]->diffuse, tmp, sizeof(float) * 4);
        }
        return;
    }
    for (int i = 0; i < n; ++i) {
        float* tslot = nullptr;
        int tcount = 0;
        MaterialPnameSlotMut(*targets[i], pname, tcount, tslot);
        memcpy(tslot, tmp, sizeof(float) * (size_t)count);
    }
}

// ---------------------------------------------------------------------------
// glLightModel — tham số mô hình ánh sáng
// ---------------------------------------------------------------------------
void glLightModelf(GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightModelf")) return;
    switch (pname) {
        case kLightModelAmbient: {
            // 4 giá trị ⇒ bản scalar phải INVALID_ENUM (spec §2.14.2).
            c.errors.Record(kErrEnum); return;
        }
        case kLightModelLocalViewer:
            c.legacy.lightModel.localViewer = (param != 0.0f); return;
        case kLightModelTwoSide:
            c.legacy.lightModel.twoSide = (param != 0.0f); return;
        case kLightModelColorControl:
            if (param == (float)kSingleColor) {
                c.legacy.lightModel.colorControl = kSingleColor;
            } else if (param == (float)kSeparateSpecular) {
                c.legacy.lightModel.colorControl = kSeparateSpecular;
            } else {
                c.errors.Record(kErrEnum);
            }
            return;
        default:
            c.errors.Record(kErrEnum); return;
    }
}

void glLightModeli(GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightModeli")) return;
    switch (pname) {
        case kLightModelAmbient:
            c.errors.Record(kErrEnum); return;
        case kLightModelLocalViewer:
            c.legacy.lightModel.localViewer = (param != 0); return;
        case kLightModelTwoSide:
            c.legacy.lightModel.twoSide = (param != 0); return;
        case kLightModelColorControl:
            if (param == (GLint)kSingleColor) {
                c.legacy.lightModel.colorControl = kSingleColor;
            } else if (param == (GLint)kSeparateSpecular) {
                c.legacy.lightModel.colorControl = kSeparateSpecular;
            } else {
                c.errors.Record(kErrEnum);
            }
            return;
        default:
            c.errors.Record(kErrEnum); return;
    }
}

void glLightModelfv(GLenum pname, const GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightModelfv")) return;
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    switch (pname) {
        case kLightModelAmbient:
            memcpy(c.legacy.lightModel.ambient, params, sizeof(float) * 4);
            return;
        case kLightModelLocalViewer:
            c.legacy.lightModel.localViewer = (params[0] != 0.0f); return;
        case kLightModelTwoSide:
            c.legacy.lightModel.twoSide = (params[0] != 0.0f); return;
        case kLightModelColorControl:
            if (params[0] == (float)kSingleColor) {
                c.legacy.lightModel.colorControl = kSingleColor;
            } else if (params[0] == (float)kSeparateSpecular) {
                c.legacy.lightModel.colorControl = kSeparateSpecular;
            } else {
                c.errors.Record(kErrEnum);
            }
            return;
        default:
            c.errors.Record(kErrEnum); return;
    }
}

void glLightModeliv(GLenum pname, const GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLightModeliv")) return;
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    switch (pname) {
        case kLightModelAmbient: {
            float tmp[4];
            CopyIntsToFloats(params, tmp, 4);
            memcpy(c.legacy.lightModel.ambient, tmp, sizeof(float) * 4);
            return;
        }
        case kLightModelLocalViewer:
            c.legacy.lightModel.localViewer = (params[0] != 0); return;
        case kLightModelTwoSide:
            c.legacy.lightModel.twoSide = (params[0] != 0); return;
        case kLightModelColorControl:
            if (params[0] == (GLint)kSingleColor) {
                c.legacy.lightModel.colorControl = kSingleColor;
            } else if (params[0] == (GLint)kSeparateSpecular) {
                c.legacy.lightModel.colorControl = kSeparateSpecular;
            } else {
                c.errors.Record(kErrEnum);
            }
            return;
        default:
            c.errors.Record(kErrEnum); return;
    }
}

// ---------------------------------------------------------------------------
// glColorMaterial — chọn thuộc tính vật liệu nào theo current color
// ---------------------------------------------------------------------------
void glColorMaterial(GLenum face, GLenum mode) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glColorMaterial")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    switch (mode) {
        case kEmission:
        case kAmbient:
        case kDiffuse:
        case kSpecular:
        case kAmbientAndDiffuse:
            break;
        default:
            c.errors.Record(kErrEnum); return;
    }
    c.legacy.colorMaterial.face = face;
    c.legacy.colorMaterial.mode = mode;
}

// ---------------------------------------------------------------------------
// glShadeModel — FLAT / SMOOTH
// ---------------------------------------------------------------------------
void glShadeModel(GLenum mode) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glShadeModel")) return;
    if (mode != kSmooth && mode != kFlat) {
        c.errors.Record(kErrEnum); return;
    }
    if (LegacyRecordShadeModel(mode)) return;
    c.legacy.shadeModel = mode;
}

// ---------------------------------------------------------------------------
// glGetLightfv / glGetLightiv — truy vấn tham số nguồn sáng
// ---------------------------------------------------------------------------
void glGetLightfv(GLenum light, GLenum pname, GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetLightfv")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    const float* slot = nullptr;
    if (!LightPnameSlot(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    memcpy(params, slot, sizeof(float) * (size_t)count);
}

void glGetLightiv(GLenum light, GLenum pname, GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetLightiv")) return;
    int li = 0;
    if (!LightIndex(light, li)) { c.errors.Record(kErrEnum); return; }
    int count = 0;
    const float* slot = nullptr;
    if (!LightPnameSlot(c.legacy.lights[li], pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    for (int i = 0; i < count; ++i) params[i] = (GLint)slot[i];
}

// ---------------------------------------------------------------------------
// glGetMaterialfv / glGetMaterialiv
// ---------------------------------------------------------------------------
// Spec 4.2C §6.1.3 (PDF p.493): "void GetMaterial{if}v(enum face, enum value, T data);
//   places information about material property value for face in data. face must
//   be either FRONT or BACK, indicating the front or back material, respectively."
// ⇒ với face = FRONT hoặc BACK, chỉ trả về `count` giá trị CỦA MẶT ĐÓ.
// (Bảng state 6.17 ghi "2×C" vì state có HAI bản (front + back), mỗi bản 4
//  giá trị — KHÔNG phải vì một lệnh Get trả cả 8.)
//
// LƯU Ý tương thích: GL_FRONT_AND_BACK được chấp nhận và trả 2×count giá trị
// (front trước, back sau) — khớp hành vi thực tế của Mesa và của các app
// legacy gọi glGetMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, buf8). Đây là
// DIVERGENCE có chủ đích so với câu văn spec 2.1/4.2C ("must be either FRONT
// or BACK") — ghi rõ trong docs/LEGACY_LIGHTING_SPEC.md.
void glGetMaterialfv(GLenum face, GLenum pname, GLfloat* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetMaterialfv")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    const float* slot = nullptr;
    if (!MaterialPnameSlot(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (pname == kAmbientAndDiffuse) {
        // Không phải pname hợp lệ cho GetMaterialfv (spec Table 2.14 chỉ có
        // AMBIENT/DIFFUSE/... riêng). Để tương thích, trả ambient.
        memcpy(params, c.legacy.materialFront.ambient, sizeof(float) * 4);
        return;
    }
    if (face == kFrontAndBack) {
        int bcount = 0;
        const float* bslot = nullptr;
        MaterialPnameSlot(c.legacy.materialBack, pname, bcount, bslot);
        memcpy(params, slot, sizeof(float) * (size_t)count);
        memcpy(params + count, bslot, sizeof(float) * (size_t)count);
        return;
    }
    const LegacyMaterial& m = (face == kBack) ? c.legacy.materialBack
                                              : c.legacy.materialFront;
    int mcount = 0;
    const float* mslot = nullptr;
    MaterialPnameSlot(m, pname, mcount, mslot);
    memcpy(params, mslot, sizeof(float) * (size_t)count);
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint* params) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glGetMaterialiv")) return;
    if (face != kFront && face != kBack && face != kFrontAndBack) {
        c.errors.Record(kErrEnum); return;
    }
    int count = 0;
    const float* slot = nullptr;
    if (!MaterialPnameSlot(c.legacy.materialFront, pname, count, slot)) {
        c.errors.Record(kErrEnum); return;
    }
    if (!params) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (pname == kAmbientAndDiffuse) {
        for (int i = 0; i < 4; ++i) params[i] = (GLint)c.legacy.materialFront.ambient[i];
        return;
    }
    if (face == kFrontAndBack) {
        int bcount = 0;
        const float* bslot = nullptr;
        MaterialPnameSlot(c.legacy.materialBack, pname, bcount, bslot);
        for (int i = 0; i < count; ++i) params[i] = (GLint)slot[i];
        for (int i = 0; i < count; ++i) params[count + i] = (GLint)bslot[i];
        return;
    }
    const LegacyMaterial& m = (face == kBack) ? c.legacy.materialBack
                                              : c.legacy.materialFront;
    int mcount = 0;
    const float* mslot = nullptr;
    MaterialPnameSlot(m, pname, mcount, mslot);
    for (int i = 0; i < count; ++i) params[i] = (GLint)mslot[i];
}

}  // namespace tglmt::gl
