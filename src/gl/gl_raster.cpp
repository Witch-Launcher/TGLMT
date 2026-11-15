// gl_raster.cpp — Raster state: enable/cap, blend, depth/stencil, cull, viewport,
// scissor, polygon offset/clamp (GL 4.6), clip control, line/point.
// Metal: bake vào MTLRenderPipelineDescriptor.colorAttachments / MTLDepthStencilDescriptor.
#include "tglmt/gl46.h"
#include "tglmt/LegacyDisplayList.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/VersionGate.h"
#include "tglmt/LegacyState.h"
#include <vector>
using namespace tglmt;

namespace tglmt::gl {

// T0.9 — cap enable chỉ tồn tại ở Compatibility Profile (GL_LIGHTING, GL_FOG,
// GL_ALPHA_TEST, GL_LIGHT0..7, GL_COLOR_MATERIAL, GL_TEXTURE_GEN_*, ...).
// Ở Core Profile, GL 4.6 Core spec §17.1 yêu cầu:
//   "An INVALID_ENUM error is generated if cap is not one of the listed values."
// Trước đây TGLMT chấp nhận im lặng ⇒ state nói dối app (app tưởng lighting
// đang bật trong khi không có gì được bật).
//
// Bảng cap sinh tự động từ docs/khronos/gl.xml (tools/gen_compat_caps.py).
static bool RejectCompatOnlyCapInCore(Context& c, GLenum cap) {
    if (c.glProfile != GlProfile::Core) return false;
    if (!IsCompatOnlyEnableCap((unsigned int)cap)) return false;
    c.errors.Record(0x0500);  // GL_INVALID_ENUM
    const char* nm = CompatOnlyEnableCapName((unsigned int)cap);
    c.LogDebug(0, 0, 0, 0, nm ? (std::string("glEnable/disable cap không tồn tại ở Core Profile: ") + nm).c_str()
                               : "glEnable/disable cap không tồn tại ở Core Profile");
    return true;
}

// GL_LIGHTi bật/tắt cần cập nhật CẢ state chung (để glIsEnabled trả về đúng)
// VÀ legacy light state (để shader fixed-function biết light nào được bật).
// StateTracker giữ caps_ theo enum ⇒ glIsEnabled(GL_LIGHTi) đã đúng; legacy
// giữ cờ riêng để đường fixed-function đọc nhanh.
static void SyncLegacyLightEnable(Context& c, GLenum cap, bool on) {
    if (cap < 0x4000 || cap > 0x4007) return;  // GL_LIGHT0..GL_LIGHT7
    int i = (int)(cap - 0x4000);
    if (i < 0 || i >= kMaxLights) return;
    c.legacy.lights[i].enabled = on;
}

void glEnable(GLenum cap) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glEnable")) return;
    if (RejectCompatOnlyCapInCore(c, cap)) return;
    if (LegacyRecordEnable(cap)) return;
    c.state.SetEnabled(cap, true);
    if (cap == 0x0B50 /*GL_LIGHTING*/) c.legacy.lightingEnabled = true;
    if (cap == 0x0B57 /*GL_COLOR_MATERIAL*/) c.legacy.colorMaterialEnabled = true;
    SyncLegacyLightEnable(c, cap, true);
}
void glDisable(GLenum cap) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glDisable")) return;
    if (RejectCompatOnlyCapInCore(c, cap)) return;
    if (LegacyRecordDisable(cap)) return;
    c.state.SetEnabled(cap, false);
    if (cap == 0x0B50 /*GL_LIGHTING*/) c.legacy.lightingEnabled = false;
    if (cap == 0x0B57 /*GL_COLOR_MATERIAL*/) c.legacy.colorMaterialEnabled = false;
    SyncLegacyLightEnable(c, cap, false);
}
void glEnablei(GLenum t, GLuint i) { Context::Current().state.SetEnabledI(t, true, i); }
void glDisablei(GLenum t, GLuint i) { Context::Current().state.SetEnabledI(t, false, i); }
GLboolean glIsEnabled(GLenum cap) {
    Context& c = Context::Current();
    // glIsEnabled với cap không hợp lệ: GL 4.6 Core §17.1 yêu cầu
    // GL_INVALID_ENUM — kể cả ở Compatibility Profile (cap không có trong
    // danh sách của profile hiện tại).
    if (RejectCompatOnlyCapInCore(c, cap)) return 0;
    return c.state.IsEnabled(cap) ? 1 : 0;
}

GLboolean glIsEnabledi(GLenum t, GLuint i) { return Context::Current().state.IsEnabled(t, i) ? 1 : 0; }
void glBlendColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { Context::Current().state.SetBlendColor(r, g, b, a); }
void glBlendEquation(GLenum m) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.rgbEq = m; b.alphaEq = m; }
}
void glBlendEquationSeparate(GLenum r, GLenum a) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.rgbEq = r; b.alphaEq = a; }
}
void glBlendEquationi(GLuint i, GLenum m) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].rgbEq = m; B[i].alphaEq = m; }
}
void glBlendEquationSeparatei(GLuint i, GLenum r, GLenum a) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].rgbEq = r; B[i].alphaEq = a; }
}
void glBlendFunc(GLenum s, GLenum d) {
    if (LegacyRecordBlendFunc(s, d)) return;
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.srcRGB = s; b.dstRGB = d; b.srcAlpha = s; b.dstAlpha = d; }
}
void glBlendFuncSeparate(GLenum srgb, GLenum drgb, GLenum sa, GLenum da) {
    auto& B = Context::Current().state.Blend();
    for (auto& b : B) { b.srcRGB = srgb; b.dstRGB = drgb; b.srcAlpha = sa; b.dstAlpha = da; }
}
void glBlendFunci(GLuint i, GLenum s, GLenum d) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].srcRGB = s; B[i].dstRGB = d; B[i].srcAlpha = s; B[i].dstAlpha = d; }
}
void glBlendFuncSeparatei(GLuint i, GLenum srgb, GLenum drgb, GLenum sa, GLenum da) {
    auto& B = Context::Current().state.Blend();
    if (i < B.size()) { B[i].srcRGB = srgb; B[i].dstRGB = drgb; B[i].srcAlpha = sa; B[i].dstAlpha = da; }
}
void glDepthFunc(GLenum f) { Context::Current().state.Depth().func = f; }
void glDepthMask(GLboolean m) {
    if (LegacyRecordDepthMask(m)) return;
    Context::Current().state.Depth().writeMask = m ? true : false;
}
void glDepthRange(GLdouble n, GLdouble f) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glDepthRange")) return;
    c.state.SetDepthRange(n, f);
}
void glDepthRangef(GLfloat n, GLfloat f) {
    // glDepthRangef và glDepthRange set CÙNG một state → giá trị phải đọc lại
    // được bằng glGetFloatv/glGetDoublev(GL_DEPTH_RANGE). Trước đây hàm inline
    // SetDepthRangef chỉ cập nhật viewports_ mà KHÔNG ghi shadow ⇒ query trả
    // giá trị CŨ (sai).
    Context::Current().state.SetDepthRange((double)n, (double)f, 0);
}
void glDepthRangeArrayv(GLuint f, GLsizei n, const GLdouble* v) {
    Context& c = Context::Current();
    // GL 4.6 §14.3: INVALID_VALUE nếu count âm; con trỏ NULL khi count>0 →
    // INVALID_VALUE (trước đây deref NULL ⇒ SEGFAULT).
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (n > 0 && !v) { c.errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) c.state.SetDepthRange(v[2*i], v[2*i+1], f + i);
}
void glDepthRangeIndexed(GLuint i, GLdouble n, GLdouble f) { Context::Current().state.SetDepthRange(n, f, i); }
// P9 viewport_array (ARB/OES/NV forwards sang core DepthRangeArray/Indexed).
// ARB_viewport_array gl.xml liệt kê NV-suffixed (quirk) — forward về core.
void glDepthRangeArraydvNV(GLuint f, GLsizei n, const GLdouble* v) { glDepthRangeArrayv(f, n, v); }
void glDepthRangeIndexeddNV(GLuint i, GLdouble n, GLdouble f) { glDepthRangeIndexed(i, n, f); }
// OES float versions → convert sang double, forward core.
void glDepthRangeArrayfvOES(GLuint f, GLsizei n, const GLfloat* v) {
    // Convert float→double (depth range cần precision — dùng thread_local buf).
    thread_local std::vector<double> tmp;
    tmp.resize((size_t)n * 2);
    for (GLsizei i = 0; i < n * 2; ++i) tmp[(size_t)i] = (double)v[i];
    glDepthRangeArrayv(f, n, tmp.data());
}
void glDepthRangeIndexedfOES(GLuint i, GLfloat n, GLfloat f) { glDepthRangeIndexed(i, (GLdouble)n, (GLdouble)f); }
void glStencilFunc(GLenum f, GLint r, GLuint m) {
    // P9: tôn trọng ActiveStencilFace (EXT_stencil_two_side). Mặc định BOTH → cả 2.
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 /*FRONT*/ || face == 0x0408 /*BOTH*/) {
        c.state.StencilFront().func = f;
        c.state.StencilFront().ref = r;
        c.state.StencilFront().valueMask = m;
    }
    if (face == 0x0405 /*BACK*/ || face == 0x0408 /*BOTH*/) {
        c.state.StencilBack().func = f;
        c.state.StencilBack().ref = r;
        c.state.StencilBack().valueMask = m;
    }
}
void glStencilFuncSeparate(GLenum face, GLenum f, GLint r, GLuint m) {
    Context& c = Context::Current();
    // Sửa bug: FRONT=0x0404, BACK=0x0405, BOTH=0x0408 (trước đây check 0x0409 sai).
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().func = f, c.state.StencilFront().ref = r, c.state.StencilFront().valueMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().func = f, c.state.StencilBack().ref = r, c.state.StencilBack().valueMask = m;
}
void glStencilOp(GLenum sf, GLenum dpf, GLenum dpp) {
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 || face == 0x0408) { auto& fr = c.state.StencilFront(); fr.sfail = sf; fr.dpfail = dpf; fr.dppass = dpp; }
    if (face == 0x0405 || face == 0x0408) { auto& bk = c.state.StencilBack(); bk.sfail = sf; bk.dpfail = dpf; bk.dppass = dpp; }
}
void glStencilOpSeparate(GLenum face, GLenum sf, GLenum dpf, GLenum dpp) {
    Context& c = Context::Current();
    // Sửa bug face check (như FuncSeparate).
    if (face == 0x0404 || face == 0x0408) { auto& fr = c.state.StencilFront(); fr.sfail = sf; fr.dpfail = dpf; fr.dppass = dpp; }
    if (face == 0x0405 || face == 0x0408) { auto& bk = c.state.StencilBack(); bk.sfail = sf; bk.dpfail = dpf; bk.dppass = dpp; }
}
void glStencilMask(GLuint m) {
    Context& c = Context::Current();
    GLenum face = c.state.ActiveStencilFace();
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().writeMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().writeMask = m;
}
void glStencilMaskSeparate(GLenum face, GLuint m) {
    Context& c = Context::Current();
    if (face == 0x0404 || face == 0x0408) c.state.StencilFront().writeMask = m;
    if (face == 0x0405 || face == 0x0408) c.state.StencilBack().writeMask = m;
}
// P9 EXT_stencil_two_side: chọn face cho StencilFunc/Op/Mask không-Separate.
void glActiveStencilFaceEXT(GLenum face) {
    Context& c = Context::Current();
    if (face != 0x0404 && face != 0x0405 && face != 0x0408) {
        c.errors.Record(0x0500); return; // INVALID_ENUM
    }
    c.state.SetActiveStencilFace(face);
}
void glCullFace(GLenum m) {
    if (LegacyRecordCullFace(m)) return;
    Context::Current().state.SetCullFace(m);
}
void glFrontFace(GLenum m) {
    if (LegacyRecordFrontFace(m)) return;
    Context::Current().state.SetFrontFace(m);
}
void glPolygonMode(GLenum f, GLenum m) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPolygonMode")) return;
    c.state.SetPolygonMode(f, m);
}
// glPolygonOffset không có tham số clamp: theo convention Vulkan/Metal, clamp==0
// nghĩa là KHÔNG clamp bias. (Trước đây truyền 1e30f → giá trị rác trong shadow
// glGet + không bao giờ tới Metal vì depth bias chẳng được encode.)
void glPolygonOffset(GLfloat f, GLfloat u) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPolygonOffset")) return;
    if (LegacyRecordPolygonOffset(f, u)) return;
    c.state.SetPolygonOffset(f, u, 0.0f);
}
void glPolygonOffsetClamp(GLfloat f, GLfloat u, GLfloat cl) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPolygonOffsetClamp")) return;
    c.state.SetPolygonOffset(f, u, cl);
}
void glLineWidth(GLfloat w) {
    if (LegacyRecordLineWidth(w)) return;
    Context::Current().state.SetLineWidth(w);
}
void glPointSize(GLfloat s) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPointSize")) return;
    // GL 4.6 §14.4: "An INVALID_VALUE error is generated if size is less than
    // or equal to zero."
    if (s <= 0.0f) { c.errors.Record(0x0501); return; }
    if (LegacyRecordPointSize(s)) return;
    c.state.SetShadow(0x0B11 /*POINT_SIZE*/, &s, 4);
}
// POINT_FADE_THRESHOLD_SIZE (0x8128) — float; POINT_SPRITE_COORD_ORIGIN
// (0x8CA0) — enum int. Cả hai đều phải validate pname theo §14.4.
static bool PointPnameOk(GLenum p, bool* isFloat) {
    if (p == 0x8128) { *isFloat = true; return true; }
    if (p == 0x8CA0) { *isFloat = false; return true; }
    return false;
}
void glPointParameterf(GLenum p, GLfloat v) {
    Context& c = Context::Current();
    bool isF = false;
    if (!PointPnameOk(p, &isF) || !isF) { c.errors.Record(0x0500); return; }
    if (v < 0.0f) { c.errors.Record(0x0501); return; }
    c.state.SetShadow(p, &v, 4);
}
void glPointParameterfv(GLenum p, const GLfloat* v) {
    Context& c = Context::Current();
    bool isF = false;
    if (!PointPnameOk(p, &isF) || !isF || !v) { c.errors.Record(0x0500); return; }
    if (v[0] < 0.0f) { c.errors.Record(0x0501); return; }
    c.state.SetShadow(p, v, 4);
}
void glPointParameteri(GLenum p, GLint v) {
    Context& c = Context::Current();
    bool isF = false;
    if (!PointPnameOk(p, &isF) || isF) { c.errors.Record(0x0500); return; }
    // POINT_SPRITE_COORD_ORIGIN: chỉ UPPER_LEFT (0x8CA1) / LOWER_LEFT (0x8CA2).
    if (v != 0x8CA1 && v != 0x8CA2) { c.errors.Record(0x0501); return; }
    c.state.SetShadow(p, &v, 4);
}
void glPointParameteriv(GLenum p, const GLint* v) {
    Context& c = Context::Current();
    bool isF = false;
    if (!PointPnameOk(p, &isF) || isF || !v) { c.errors.Record(0x0500); return; }
    if (v[0] != 0x8CA1 && v[0] != 0x8CA2) { c.errors.Record(0x0501); return; }
    c.state.SetShadow(p, v, 4);
}
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glViewport")) return;
    if (LegacyRecordViewport(x, y, w, h)) return;
    c.state.SetViewport(0, (float)x, (float)y, (float)w, (float)h);
}
void glViewportArrayv(GLuint f, GLsizei n, const GLfloat* v) {
    Context& c = Context::Current();
    // INVALID_VALUE nếu count âm hoặc con trỏ NULL (trước đây deref NULL).
    if (n < 0) { c.errors.Record(0x0501); return; }
    if (n > 0 && !v) { c.errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) c.state.SetViewport(f + i, v[4*i], v[4*i+1], v[4*i+2], v[4*i+3]);
}
void glViewportIndexedf(GLuint i, GLfloat x, GLfloat y, GLfloat w, GLfloat h) { Context::Current().state.SetViewport(i, x, y, w, h); }
void glViewportIndexedfv(GLuint i, const GLfloat* v) {
    Context& c = Context::Current();
    if (!v) { c.errors.Record(0x0501); return; }  // NULL → INVALID_VALUE
    c.state.SetViewport(i, v[0], v[1], v[2], v[3]);
}
// --- Scissor (ARB_viewport_array / GL 4.6 §14.9.2) ---
// Mỗi viewport index có một scissor box riêng; lệnh indexed ghi đúng box của
// index đó (trước đây mọi lệnh đều ghi vào box của viewport 0).
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    if (LegacyRecordScissor(x, y, w, h)) return;
    // glScissor đặt box cho TẤT CẢ viewport (spec §14.9.2).
    c.state.SetScissorAll(x, y, w, h);
}
void glScissorArrayv(GLuint f, GLsizei n, const GLint* v) {
    Context& c = Context::Current();
    if (n < 0) { c.errors.Record(0x0501); return; } // INVALID_VALUE (count < 0)
    if (n > 0 && !v) { c.errors.Record(0x0501); return; } // NULL → INVALID_VALUE
    if (!v) return;
    // first+count vượt MAX_VIEWPORTS → INVALID_VALUE. Dùng giới hạn nội bộ
    // (kMaxViewports) vì gl_get.cpp vẫn báo GL_MAX_VIEWPORTS = 1 (xem report).
    if ((GLuint)n + f > (GLuint)kMaxViewports) { c.errors.Record(0x0501); return; }
    for (GLsizei i = 0; i < n; ++i) {
        const GLint* b = v + 4 * i;
        if (b[2] < 0 || b[3] < 0) { c.errors.Record(0x0501); return; } // INVALID_VALUE
        c.state.SetScissor((int)(f + i), b[0], b[1], b[2], b[3]);
    }
}
void glScissorIndexed(GLuint i, GLint x, GLint y, GLsizei w, GLsizei h) {
    Context& c = Context::Current();
    if (w < 0 || h < 0) { c.errors.Record(0x0501); return; } // INVALID_VALUE
    if (i >= (GLuint)kMaxViewports) { c.errors.Record(0x0501); return; }
    c.state.SetScissor((int)i, x, y, w, h);
}
void glScissorIndexedv(GLuint i, const GLint* v) {
    if (!v) return;
    glScissorIndexed(i, v[0], v[1], v[2], v[3]);
}
void glClipControl(GLenum o, GLenum d) { Context::Current().state.SetClipControl(o, d); }
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    uint8_t m = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
    Context& c = Context::Current();
    for (int i = 0; i < 8; ++i) c.state.SetColorMask((GLuint)i, m);
}
void glColorMaski(GLuint i, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    uint8_t m = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
    if ((int)i < 8) Context::Current().state.SetColorMask(i, m);
}
void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c_) { (void)a;(void)b;(void)c_; }
void glLogicOp(GLenum o) {
    Context& c = Context::Current();
    // GL 4.6 §17.3.9: "An INVALID_VALUE error is generated if op is not one of
    // the logicop modes in table 17.3" (0x1500..0x150F liên tục).
    if (o < 0x1500 || o > 0x150F) { c.errors.Record(0x0501); return; }
    // Shadow khoá theo pname Core (GL_LOGIC_OP_MODE = 0x0BF0) để cả
    // GL_LOGIC_OP_MODE (Core) và GL_LOGIC_OP (compat-only) đọc cùng state.
    c.state.SetShadow(0x0BF0, &o, 4);
}
void glSampleCoverage(GLfloat v, GLboolean inv) {
    Context& c = Context::Current();
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    c.state.SetShadow(0x80AA /*SAMPLE_COVERAGE_VALUE*/, &v, sizeof(v));
    GLint iv = inv ? 1 : 0;
    c.state.SetShadow(0x80AB /*SAMPLE_COVERAGE_INVERT*/, &iv, sizeof(iv));
}
void glSampleMaski(GLuint m, GLbitfield mask) {
    Context& c = Context::Current();
    if (m >= 1) { c.errors.Record(0x0501); return; } // MAX_SAMPLE_MASK_WORDS = 1
    c.state.SetShadow(0x8E52 /*SAMPLE_MASK_VALUE*/, &mask, sizeof(mask), m);
}
void glMinSampleShading(GLfloat v) {
    // GL 4.6 §14.3.1.1: "value is clamped to [0, 1] when specified".
    // Enum đúng: GL_MIN_SAMPLE_SHADING_VALUE = 0x8C37 (trước đây ghi 0x8C51 —
    // không phải enum GL nào → query luôn trả 0).
    // GL_SAMPLE_SHADING (0x8C36) là enable cap: glEnable/glDisable generic đã
    // track qua StateTracker::SetEnabled → glIsEnabled + glGetBooleanv đúng.
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    Context::Current().state.SetShadow(0x8C37 /*MIN_SAMPLE_SHADING_VALUE*/, &v, 4);
}
void glHint(GLenum t, GLenum m) { (void)t;(void)m; }
void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }
} // namespace tglmt::gl
