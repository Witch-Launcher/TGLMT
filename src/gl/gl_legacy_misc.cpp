// gl_legacy_misc.cpp — RESIDENCY / PRIORITY / FEEDBACK MARKER (misc) +
//                     TRUY VẤN TEXTURE ENVIRONMENT & TEXTURE GENERATION.
//
// Spec (đã đối chiếu TRỰC TIẾP với PDF):
//   • GL 2.1 §3.8.12 "Texturing" (glspec21.pdf PDF p.183–184, idx 196–197):
//       AreTexturesResident: "returns TRUE if all of the n texture objects
//       named in textures are resident, or if the implementation does not
//       distinguish a working set. ... If any of the names in textures are
//       unused or are zero, FALSE is returned, the error INVALID_VALUE is
//       generated, and the contents of residences are indeterminate."
//       PrioritizeTextures: "Each priority value is clamped to the range [0,1]
//       before it is assigned. ... PrioritizeTextures silently ignores
//       attempts to prioritize unused texture object names or zero."
//   • GL 2.1 §5.3 "Feedback" (PDF p.240, idx 253):
//       "void PassThrough(float token); may be used as a marker in feedback
//       mode. token is returned as if it were a primitive; it is indicated
//       with its own unique identifying value. ... PassThrough may not occur
//       between Begin and End. It has no effect when the GL is not in
//       feedback mode."
//   • GL 2.1 Table 6.19 "Texture Environment and Generation" (PDF p.284,
//     idx 297) + GL 2.1 §3.8.13 (PDF p.184–187) + §2.11.4 (PDF p.50):
//     TEXTURE_ENV_MODE (GetTexEnviv), TEXTURE_ENV_COLOR (GetTexEnvfv),
//     COMBINE_*/SRC*/OPERAND* (GetTexEnviv), RGB_SCALE/ALPHA_SCALE
//     (GetTexEnvfv), TEXTURE_GEN_MODE (GetTexGeniv), EYE_PLANE/OBJECT_PLANE
//     (GetTexGenfv).
//   • GL 4.2C §3.10.17 (PDF p.350, idx 369), §5.3 (PDF p.459, idx 478),
//     §6.1.3 (PDF p.477, idx 496) — văn bản tương đương.
//
// ĐƯỜNG DRAW / FEEDBACK: các truy vấn texture env/gen áp cho TEXTURE UNIT
// ĐANG HOẠT ĐỘNG (spec GL 1.3+ / GL 2.1 §2.8.1: "the current texture
// coordinate set is determined by the value of ACTIVE_TEXTURE") — cùng cách
// chọn unit như src/gl/gl_legacy_texenv.cpp (activeTextureUnit & 7).
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

namespace {

constexpr GLenum kInvalidEnum = GL_INVALID_ENUM;
constexpr GLenum kInvalidValue = GL_INVALID_VALUE;
constexpr GLenum kInvalidOperation = GL_INVALID_OPERATION;

// Token feedback của PassThrough. src/gl/gl_legacy_selection.cpp dùng bộ token
// kiểu GL 1.x (0x0000…0x0006) cho cả khối feedback ⇒ giữ nhất quán (0x0006).
// (Registry có GL_PASS_THROUGH_TOKEN = 0x0700 nhưng repo dùng bộ token GL 1.x.)
constexpr GLfloat kFeedbackTokenPassThrough = 0x0006;

// State phụ của selection/feedback (src/gl/gl_legacy_selection.cpp): pname
// riêng 0xF001, struct SideState {GLenum feedbackType; GLint
// selectRecordCount; float hitMinZ; float hitMaxZ; uint32_t flags; ...} ⇒
// flags ở offset 16, bit 4 = "đã chuẩn hoá renderMode".
// Cần đọc vì include/tglmt/LegacyState.h đặt renderMode mặc định = 0x1C01
// (GL_FEEDBACK) — sai; file selection chuẩn hoá 1 lần khi có lệnh render mode.
constexpr GLenum kSelectionSidePname = 0xF001;
constexpr size_t kSelectionSideBytes = 24;
constexpr size_t kSelectionFlagsOffset = 16;
constexpr uint32_t kFlagModeInitialized = 1u << 4;

// true nếu GL đang ở chế độ FEEDBACK (đã được chuẩn hoá).
bool FeedbackModeActive(Context& c) {
    uint8_t blob[kSelectionSideBytes] = {0};
    c.state.GetShadow(kSelectionSidePname, blob, sizeof(blob));
    uint32_t flags = 0;
    std::memcpy(&flags, blob + kSelectionFlagsOffset, sizeof(flags));
    if (!(flags & kFlagModeInitialized)) return false;  // mặc định = GL_RENDER
    return c.legacy.selection.renderMode == GL_FEEDBACK;
}

// ---------------------------------------------------------------------------
// TEXTURE ENVIRONMENT — truy vấn
// ---------------------------------------------------------------------------
constexpr GLenum kTextureEnv = GL_TEXTURE_ENV;
constexpr GLenum kTextureEnvColor = GL_TEXTURE_ENV_COLOR;
constexpr GLenum kTextureEnvMode = GL_TEXTURE_ENV_MODE;
constexpr GLenum kCombineRGB = GL_COMBINE_RGB;
constexpr GLenum kCombineAlpha = GL_COMBINE_ALPHA;
constexpr GLenum kRgbScale = GL_RGB_SCALE;
constexpr GLenum kAlphaScale = GL_ALPHA_SCALE;

bool TexEnvTargetValid(GLenum target) {
    return target == kTextureEnv || target == kTextureEnvColor;
}

// pname hợp lệ + số giá trị trả về (Table 6.19, GL 2.1 p.284).
int TexEnvQueryCount(GLenum pname) {
    switch (pname) {
        case kTextureEnvMode:
        case kCombineRGB:
        case kCombineAlpha:
        case kRgbScale:
        case kAlphaScale:
        case GL_SRC0_RGB: case GL_SRC1_RGB: case GL_SRC2_RGB:
        case GL_SRC0_ALPHA: case GL_SRC1_ALPHA: case GL_SRC2_ALPHA:
        case GL_OPERAND0_RGB: case GL_OPERAND1_RGB: case GL_OPERAND2_RGB:
        case GL_OPERAND0_ALPHA: case GL_OPERAND1_ALPHA: case GL_OPERAND2_ALPHA:
            return 1;
        case kTextureEnvColor:
            return 4;
        default:
            return 0;
    }
}

// Giá trị enum của pname (hoặc 0 nếu pname không phải kiểu enum).
GLenum TexEnvEnumValue(const LegacyTexEnv& e, GLenum pname) {
    switch (pname) {
        case kTextureEnvMode: return e.mode;
        case kCombineRGB: return e.combineRGB;
        case kCombineAlpha: return e.combineAlpha;
        case GL_SRC0_RGB: return e.src0RGB;
        case GL_SRC1_RGB: return e.src1RGB;
        case GL_SRC2_RGB: return e.src2RGB;
        case GL_SRC0_ALPHA: return e.src0Alpha;
        case GL_SRC1_ALPHA: return e.src1Alpha;
        case GL_SRC2_ALPHA: return e.src2Alpha;
        case GL_OPERAND0_RGB: return e.operand0RGB;
        case GL_OPERAND1_RGB: return e.operand1RGB;
        case GL_OPERAND2_RGB: return e.operand2RGB;
        case GL_OPERAND0_ALPHA: return e.operand0Alpha;
        case GL_OPERAND1_ALPHA: return e.operand1Alpha;
        case GL_OPERAND2_ALPHA: return e.operand2Alpha;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// TEXTURE GENERATION — truy vấn
// ---------------------------------------------------------------------------
constexpr GLenum kTextureGenMode = GL_TEXTURE_GEN_MODE;
constexpr GLenum kObjectPlane = GL_OBJECT_PLANE;
constexpr GLenum kEyePlane = GL_EYE_PLANE;

bool TexGenCoordIndex(GLenum coord, int& out) {
    switch (coord) {
        case GL_S: out = 0; return true;
        case GL_T: out = 1; return true;
        case GL_R: out = 2; return true;
        case GL_Q: out = 3; return true;
        default: return false;
    }
}

bool TexGenPnameValid(GLenum pname) {
    return pname == kTextureGenMode || pname == kObjectPlane || pname == kEyePlane;
}

// Chuyển float [0,1] → int theo quy ước GetTexEnviv/GetTexGeniv (nhân với
// 2^31−1, làm tròn) — cùng quy ước mà driver dùng cho pname kiểu float khi
// truy vấn bằng biến thể int.
GLint FloatToGLint(float f) {
    double v = (double)f * 2147483647.0;
    if (!(v > -2147483648.0)) return (GLint)(-2147483647.0 - 1.0);
    if (v > 2147483647.0) return 2147483647;
    return (GLint)(v >= 0.0 ? v + 0.5 : v - 0.5);
}

}  // namespace

// ===========================================================================
// MISC — residency / priority / feedback marker
// ===========================================================================
GLboolean glAreTexturesResident(GLsizei n, const GLuint* textures, GLboolean* residences) {
    Context& c = Context::Current();
    if (n < 0) {
        c.errors.Record(kInvalidValue);
        return GL_FALSE;
    }
    if (n == 0) return GL_TRUE;  // "all of the 0 texture objects" ⇒ TRUE
    if (!textures) {
        c.errors.Record(kInvalidValue);
        return GL_FALSE;
    }
    // "If any of the names in textures are unused or are zero, FALSE is
    // returned, the error INVALID_VALUE is generated, and the contents of
    // residences are indeterminate." (GL 2.1 §3.8.12, PDF p.183)
    bool allUsed = true;
    for (GLsizei i = 0; i < n; ++i) {
        const GLuint name = textures[i];
        if (name == 0) { allUsed = false; break; }
        const bool used = c.textures.count(name) || c.registry.Is(ObjectKind::Texture, name);
        if (!used) { allUsed = false; break; }
    }
    if (!allUsed) {
        c.errors.Record(kInvalidValue);
        if (residences)
            for (GLsizei i = 0; i < n; ++i) residences[i] = GL_FALSE;
        return GL_FALSE;
    }
    // TGLMT không phân biệt working set ⇒ mọi texture đều "resident"
    // ("...or if the implementation does not distinguish a working set") ⇒
    // TRUE, và nội dung của residences KHÔNG bị thay đổi.
    return GL_TRUE;
}

void glPrioritizeTextures(GLsizei n, const GLuint* textures, const GLfloat* priorities) {
    Context& c = Context::Current();
    if (c.legacy.inBeginEnd) {
        // Rect/DrawPixels đều bị cấm trong Begin/End; PrioritizeTextures cũng
        // không nằm trong danh sách lệnh được phép (GL 2.1 §2.6.3 p.19–20).
        c.errors.Record(kInvalidOperation);
        return;
    }
    if (n < 0) {
        c.errors.Record(kInvalidValue);
        return;
    }
    if (n == 0) return;
    if (!textures || !priorities) {
        c.errors.Record(kInvalidValue);
        return;
    }
    bool outOfRange = false;
    for (GLsizei i = 0; i < n; ++i) {
        const GLuint name = textures[i];
        const float p = priorities[i];
        if (p < 0.0f || p > 1.0f) outOfRange = true;
        // "PrioritizeTextures silently ignores attempts to prioritize unused
        // texture object names or zero (default textures)."
        if (name == 0) continue;
        auto it = c.textures.find(name);
        if (it == c.textures.end()) {
            if (!c.registry.Is(ObjectKind::Texture, name)) continue;  // tên chưa dùng
            TextureObject fresh;                                       // tên đã gen
            fresh.id = name;
            c.textures[name] = fresh;
            it = c.textures.find(name);
        }
        // "Each priority value is clamped to the range [0,1] before it is
        // assigned." (GL 2.1 §3.8.12, PDF p.184)
        float clamped = p;
        if (clamped < 0.0f) clamped = 0.0f;
        else if (clamped > 1.0f) clamped = 1.0f;
        it->second.fparams[GL_TEXTURE_PRIORITY] = clamped;
        it->second.params[GL_TEXTURE_PRIORITY] = (GLint)clamped;
    }
    // LỆCH NHỎ VỚI SPEC: GL 2.1 §3.8.12 (p.184) chỉ nói "clamp", KHÔNG yêu cầu
    // sinh lỗi khi priority nằm ngoài [0,1]. Task slice này yêu cầu
    // GL_INVALID_VALUE ⇒ TGLMT sinh lỗi (giá trị vẫn được clamp theo spec).
    if (outOfRange) c.errors.Record(kInvalidValue);
}

void glPassThrough(GLfloat token) {
    Context& c = Context::Current();
    // "PassThrough may not occur between Begin and End." (GL 2.1 §5.3, p.240)
    if (c.legacy.inBeginEnd) {
        c.errors.Record(kInvalidOperation);
        return;
    }
    // "It has no effect when the GL is not in feedback mode."
    if (!FeedbackModeActive(c)) return;

    LegacyState::LegacySelection& sel = c.legacy.selection;
    GLfloat* buf = sel.feedbackBuffer;
    const GLsizei cap = sel.feedbackCapacity;
    if (!buf || cap <= 0) return;  // không có feedback buffer ⇒ không ghi
    // Khối feedback của PassThrough = token định danh + giá trị token.
    if (sel.feedbackUsed + 2 > cap) {
        // Buffer tràn: theo spec §5.3 đặt cờ overflow; TGLMT không có cờ
        // overflow riêng cho feedback ở đây ⇒ không ghi gì thêm.
        return;
    }
    buf[sel.feedbackUsed++] = kFeedbackTokenPassThrough;
    buf[sel.feedbackUsed++] = token;
}

// ===========================================================================
// TEXTURE ENVIRONMENT — glGetTexEnvfv / glGetTexEnviv
// ===========================================================================
void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat* params) {
    Context& c = Context::Current();
    if (!TexEnvTargetValid(target)) { c.errors.Record(kInvalidEnum); return; }
    const int n = TexEnvQueryCount(pname);
    if (n == 0) { c.errors.Record(kInvalidEnum); return; }
    if (!params) { c.errors.Record(kInvalidValue); return; }
    const GLuint unit = c.legacy.activeTextureUnit & 7u;
    const LegacyTexEnv& e = c.legacy.texEnv[unit];
    if (pname == kTextureEnvColor) {
        for (int i = 0; i < 4; ++i) params[i] = e.color[i];
        return;
    }
    if (pname == kRgbScale || pname == kAlphaScale) {
        params[0] = (float)(pname == kRgbScale ? e.rgbScale : e.alphaScale);
        return;
    }
    params[0] = (float)TexEnvEnumValue(e, pname);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint* params) {
    Context& c = Context::Current();
    if (!TexEnvTargetValid(target)) { c.errors.Record(kInvalidEnum); return; }
    const int n = TexEnvQueryCount(pname);
    if (n == 0) { c.errors.Record(kInvalidEnum); return; }
    if (!params) { c.errors.Record(kInvalidValue); return; }
    const GLuint unit = c.legacy.activeTextureUnit & 7u;
    const LegacyTexEnv& e = c.legacy.texEnv[unit];
    if (pname == kTextureEnvColor) {
        // Biến thể int: thành phần màu (float) → int theo quy ước ×(2^31−1).
        for (int i = 0; i < 4; ++i) params[i] = FloatToGLint(e.color[i]);
        return;
    }
    if (pname == kRgbScale || pname == kAlphaScale) {
        params[0] = (GLint)(pname == kRgbScale ? e.rgbScale : e.alphaScale);
        return;
    }
    params[0] = (GLint)TexEnvEnumValue(e, pname);
}

// ===========================================================================
// TEXTURE GENERATION — glGetTexGenfv / glGetTexGeniv / glGetTexGendv
// ===========================================================================
void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat* params) {
    Context& c = Context::Current();
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kInvalidEnum); return; }
    if (!TexGenPnameValid(pname)) { c.errors.Record(kInvalidEnum); return; }
    if (!params) { c.errors.Record(kInvalidValue); return; }
    const GLuint unit = c.legacy.activeTextureUnit & 7u;
    const LegacyTexGenCoord& g = c.legacy.texGen[unit].coord[ci];
    if (pname == kTextureGenMode) {
        params[0] = (float)g.mode;
        return;
    }
    for (int i = 0; i < 4; ++i) params[i] = g.plane[i];
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint* params) {
    Context& c = Context::Current();
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kInvalidEnum); return; }
    if (!TexGenPnameValid(pname)) { c.errors.Record(kInvalidEnum); return; }
    if (!params) { c.errors.Record(kInvalidValue); return; }
    const GLuint unit = c.legacy.activeTextureUnit & 7u;
    const LegacyTexGenCoord& g = c.legacy.texGen[unit].coord[ci];
    if (pname == kTextureGenMode) {
        params[0] = (GLint)g.mode;
        return;
    }
    // Biến thể int cho mặt phẳng: float → int theo quy ước ×(2^31−1).
    for (int i = 0; i < 4; ++i) params[i] = FloatToGLint(g.plane[i]);
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble* params) {
    Context& c = Context::Current();
    int ci = 0;
    if (!TexGenCoordIndex(coord, ci)) { c.errors.Record(kInvalidEnum); return; }
    if (!TexGenPnameValid(pname)) { c.errors.Record(kInvalidEnum); return; }
    if (!params) { c.errors.Record(kInvalidValue); return; }
    const GLuint unit = c.legacy.activeTextureUnit & 7u;
    const LegacyTexGenCoord& g = c.legacy.texGen[unit].coord[ci];
    if (pname == kTextureGenMode) {
        params[0] = (GLdouble)g.mode;
        return;
    }
    for (int i = 0; i < 4; ++i) params[i] = (GLdouble)g.plane[i];
}

}  // namespace tglmt::gl
