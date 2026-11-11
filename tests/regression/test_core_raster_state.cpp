// Test nhóm Raster/State của GL 4.6 Core — 21 entry point:
//   glBlendColor, glBlendEquation, glBlendEquationSeparate, glBlendEquationi,
//   glBlendEquationSeparatei, glBlendFuncSeparatei,
//   glDepthRange, glDepthRangef, glDepthRangeArrayv, glDepthRangeIndexed,
//   glIsEnabledi, glLineWidth, glPointSize,
//   glPointParameterf/fv/i/iv,
//   glStencilMaskSeparate, glStencilOpSeparate,
//   glViewportIndexedf, glViewportIndexedfv.
//
// Kèm các entry point còn thiếu test ở nhóm khác:
//   glVertexAttrib{1,2,3,4}{s,sv} (8), glVertexAttrib4{bv,iv,s,sv,ubv,uiv,usv} (7)
//   glVertexAttrib4{Nbv,Niv,Nsv,Nub,Nubv,Nuiv,Nusv} (7)
//   glUniform3d/3dv/4d/4dv (4), glProgramUniform3d/3dv/4d/4dv (4)
//   glClampColor, glProvokingVertex, glBindImageTextures, glGetGraphicsResetStatus
//
// Kiểm chứng: set → get phải khớp; lỗi theo spec (INVALID_ENUM/INVALID_VALUE).

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }
bool Near(float a, float b) { return fabsf(a - b) < 1e-5f; }
// Tolerance 1e-6: giá trị float (vd 0.1f) khi nâng lên double lệch ~1.5e-9.
bool NearD(double a, double b) { return fabs(a - b) < 1e-6; }

// ===========================================================================
// Blend
// ===========================================================================
bool TestBlendState() {
    // glBlendColor → glGetFloatv(GL_BLEND_COLOR)
    glBlendColor(0.25f, 0.5f, 0.75f, 1.0f);
    GLfloat bc[4] = {0};
    glGetFloatv(GL_BLEND_COLOR, bc);
    TGLMT_CHECK(Near(bc[0], 0.25f) && Near(bc[3], 1.0f),
                "glBlendColor phải lưu đúng 4 thành phần");

    // glBlendEquation → cả RGB và alpha
    glBlendEquation(GL_MAX);
    GLint eq = 0;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &eq);
    TGLMT_CHECK(eq == GL_MAX, "glBlendEquation phải set cả RGB");
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eq);
    TGLMT_CHECK(eq == GL_MAX, "glBlendEquation phải set cả alpha");

    // glBlendEquationSeparate
    glBlendEquationSeparate(GL_FUNC_SUBTRACT, GL_FUNC_ADD);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &eq);
    TGLMT_CHECK(eq == GL_FUNC_SUBTRACT, "glBlendEquationSeparate phải set RGB riêng");
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &eq);
    TGLMT_CHECK(eq == GL_FUNC_ADD, "glBlendEquationSeparate phải set alpha riêng");

    // glBlendEquationi / glBlendEquationSeparatei (theo draw buffer)
    glBlendEquationi(0, GL_MIN);
    glGetIntegeri_v(GL_BLEND_EQUATION_RGB, 0, &eq);
    TGLMT_CHECK(eq == GL_MIN, "glBlendEquationi phải set equation của buffer 0");
    glBlendEquationSeparatei(0, GL_MAX, GL_FUNC_ADD);
    glGetIntegeri_v(GL_BLEND_EQUATION_RGB, 0, &eq);
    TGLMT_CHECK(eq == GL_MAX, "glBlendEquationSeparatei phải set RGB riêng");
    glGetIntegeri_v(GL_BLEND_EQUATION_ALPHA, 0, &eq);
    TGLMT_CHECK(eq == GL_FUNC_ADD, "glBlendEquationSeparatei phải set alpha riêng");

    // glBlendFuncSeparatei
    DrainErrors();
    glBlendFuncSeparatei(0, GL_ONE, GL_ZERO, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBlendFuncSeparatei không lỗi");
    GLint sf = 0, df = 0;
    glGetIntegeri_v(GL_BLEND_SRC_RGB, 0, &sf);
    glGetIntegeri_v(GL_BLEND_DST_RGB, 0, &df);
    TGLMT_CHECK(sf == GL_ONE && df == GL_ZERO,
                "glBlendFuncSeparatei phải set src/dst RGB của buffer 0");
    glGetIntegeri_v(GL_BLEND_SRC_ALPHA, 0, &sf);
    glGetIntegeri_v(GL_BLEND_DST_ALPHA, 0, &df);
    TGLMT_CHECK(sf == GL_SRC_ALPHA && df == GL_ONE_MINUS_SRC_ALPHA,
                "glBlendFuncSeparatei phải set src/dst alpha của buffer 0");
    return true;
}

// ===========================================================================
// Depth range
// ===========================================================================
bool TestDepthRange() {
    glDepthRange(0.25, 0.75);
    GLfloat dr[2] = {0};
    glGetFloatv(GL_DEPTH_RANGE, dr);
    TGLMT_CHECK(Near(dr[0], 0.25f) && Near(dr[1], 0.75f),
                "glDepthRange phải lưu đúng (n, f)");

    glDepthRangef(0.1f, 0.9f);
    glGetFloatv(GL_DEPTH_RANGE, dr);
    TGLMT_CHECK(Near(dr[0], 0.1f) && Near(dr[1], 0.9f), "glDepthRangef phải lưu đúng");

    // glDepthRangeIndexed — giá trị riêng cho từng viewport index.
    glDepthRangeIndexed(2, 0.0, 0.5);
    GLdouble dri[2] = {0};
    glGetDoublei_v(GL_DEPTH_RANGE, 2, dri);
    TGLMT_CHECK(NearD(dri[0], 0.0) && NearD(dri[1], 0.5),
                "glDepthRangeIndexed phải lưu theo viewport index");
    GLdouble dr0[2] = {0};
    glGetDoublei_v(GL_DEPTH_RANGE, 0, dr0);
    TGLMT_CHECK(NearD(dr0[0], 0.1) && NearD(dr0[1], 0.9),
                "glDepthRangeIndexed không được đụng viewport 0");

    // glDepthRangeArrayv — mảng (n,f) cho nhiều viewport.
    const GLdouble arr[4] = {0.2, 0.4, 0.6, 0.8};
    glDepthRangeArrayv(0, 2, arr);
    glGetDoublei_v(GL_DEPTH_RANGE, 0, dr0);
    TGLMT_CHECK(NearD(dr0[0], 0.2) && NearD(dr0[1], 0.4),
                "glDepthRangeArrayv phải lưu theo từng viewport");
    glGetDoublei_v(GL_DEPTH_RANGE, 1, dr0);
    TGLMT_CHECK(NearD(dr0[0], 0.6) && NearD(dr0[1], 0.8),
                "glDepthRangeArrayv phải lưu viewport thứ 2");

    // NULL → INVALID_VALUE (trước đây deref ⇒ CRASH).
    DrainErrors();
    glDepthRangeArrayv(0, 2, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glDepthRangeArrayv NULL → INVALID_VALUE");
    // count âm → INVALID_VALUE.
    DrainErrors();
    glDepthRangeArrayv(0, -1, arr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glDepthRangeArrayv count<0 → INVALID_VALUE");
    return true;
}

// ===========================================================================
// Line/point/stencil/viewport
// ===========================================================================
bool TestLinePointStencilViewport() {
    // glLineWidth
    glLineWidth(3.0f);
    GLfloat lw = 0;
    glGetFloatv(GL_LINE_WIDTH, &lw);
    TGLMT_CHECK(Near(lw, 3.0f), "glLineWidth phải lưu đúng");

    // glPointSize (phải > 0)
    glPointSize(4.0f);
    GLfloat ps = 0;
    glGetFloatv(GL_POINT_SIZE, &ps);
    TGLMT_CHECK(Near(ps, 4.0f), "glPointSize phải lưu đúng");
    DrainErrors();
    glPointSize(0.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glPointSize(0) → INVALID_VALUE");
    DrainErrors();
    glPointSize(-1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glPointSize âm → INVALID_VALUE");

    // glPointParameterf/fv (float pnames)
    glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, 2.0f);
    GLfloat pt = 0;
    glGetFloatv(GL_POINT_FADE_THRESHOLD_SIZE, &pt);
    TGLMT_CHECK(Near(pt, 2.0f), "glPointParameterf phải lưu đúng");
    glPointParameterfv(GL_POINT_FADE_THRESHOLD_SIZE, (const GLfloat[]){5.0f});
    glGetFloatv(GL_POINT_FADE_THRESHOLD_SIZE, &pt);
    TGLMT_CHECK(Near(pt, 5.0f), "glPointParameterfv phải lưu đúng");
    // pname sai → INVALID_ENUM.
    DrainErrors();
    glPointParameterf(0xDEAD, 1.0f);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "glPointParameterf pname sai → INVALID_ENUM");

    // glPointParameteri/iv (enum pnames)
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT);
    GLint po = 0;
    glGetIntegerv(GL_POINT_SPRITE_COORD_ORIGIN, &po);
    TGLMT_CHECK(po == GL_UPPER_LEFT, "glPointParameteri phải lưu đúng");
    glPointParameteriv(GL_POINT_SPRITE_COORD_ORIGIN, (const GLint[]){GL_LOWER_LEFT});
    glGetIntegerv(GL_POINT_SPRITE_COORD_ORIGIN, &po);
    TGLMT_CHECK(po == GL_LOWER_LEFT, "glPointParameteriv phải lưu đúng");
    // Giá trị không hợp lệ → INVALID_VALUE.
    DrainErrors();
    glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, 0x1234);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glPointParameteri với giá trị sai → INVALID_VALUE");

    // glStencilMaskSeparate / glStencilOpSeparate
    GLint smBackBefore = 0;
    glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &smBackBefore);
    glStencilMaskSeparate(GL_FRONT, 0x0Fu);
    GLint sm = 0;
    glGetIntegerv(GL_STENCIL_WRITEMASK, &sm);
    TGLMT_CHECK(sm == 0x0F, "glStencilMaskSeparate FRONT phải set front write mask");
    glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &sm);
    TGLMT_CHECK(sm == smBackBefore,
                "glStencilMaskSeparate FRONT không được đụng back write mask");
    glStencilMaskSeparate(GL_BACK, 0xF0u);
    glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &sm);
    TGLMT_CHECK(sm == 0xF0, "glStencilMaskSeparate BACK phải set back write mask");

    glStencilOpSeparate(GL_FRONT, GL_INCR, GL_DECR, GL_INVERT);
    GLint op = 0;
    glGetIntegerv(GL_STENCIL_FAIL, &op);
    TGLMT_CHECK(op == GL_INCR, "glStencilOpSeparate FRONT phải set stencil fail");
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &op);
    TGLMT_CHECK(op == GL_DECR, "glStencilOpSeparate FRONT phải set depth fail");
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &op);
    TGLMT_CHECK(op == GL_INVERT, "glStencilOpSeparate FRONT phải set depth pass");

    // glViewportIndexedf / fv
    glViewportIndexedf(1, 1.0f, 2.0f, 30.0f, 40.0f);
    GLfloat vp[4] = {0};
    glGetFloati_v(GL_VIEWPORT, 1, vp);
    TGLMT_CHECK(Near(vp[0], 1.0f) && Near(vp[2], 30.0f) && Near(vp[3], 40.0f),
                "glViewportIndexedf phải lưu theo viewport index");
    glViewportIndexedfv(2, (const GLfloat[]){5.0f, 6.0f, 70.0f, 80.0f});
    glGetFloati_v(GL_VIEWPORT, 2, vp);
    TGLMT_CHECK(Near(vp[0], 5.0f) && Near(vp[3], 80.0f),
                "glViewportIndexedfv phải lưu theo viewport index");
    // NULL → INVALID_VALUE.
    DrainErrors();
    glViewportIndexedfv(2, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glViewportIndexedfv NULL → INVALID_VALUE");

    // glIsEnabledi — trạng thái enable theo draw buffer.
    glEnablei(GL_BLEND, 0);
    TGLMT_CHECK(glIsEnabledi(GL_BLEND, 0) == GL_TRUE,
                "glIsEnabledi phải trả TRUE cho buffer đang enable");
    glDisablei(GL_BLEND, 0);
    TGLMT_CHECK(glIsEnabledi(GL_BLEND, 0) == GL_FALSE,
                "glIsEnabledi phải trả FALSE sau khi disable");
    return true;
}

// ===========================================================================
// Vertex attrib dạng short/byte/normalized + uniform 3d/4d
// ===========================================================================
bool TestShortAttribsAndDoubleUniforms() {
    GLfloat v[4] = {0};

    // --- short ---
    glVertexAttrib1s(0, -3);
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], -3.0f), "glVertexAttrib1s phải lưu đúng");
    glVertexAttrib2s(1, 1, 2);
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[1], 2.0f), "glVertexAttrib2s phải lưu đúng");
    glVertexAttrib3s(2, 1, 2, 3);
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 3.0f), "glVertexAttrib3s phải lưu đúng");
    glVertexAttrib4s(3, 1, 2, 3, 4);
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glVertexAttrib4s phải lưu đúng");
    const GLshort s4[4] = {5, 6, 7, 8};
    glVertexAttrib1sv(4, s4);
    glGetVertexAttribfv(4, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 5.0f), "glVertexAttrib1sv phải lưu phần tử đầu");
    glVertexAttrib2sv(5, s4);
    glGetVertexAttribfv(5, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[1], 6.0f), "glVertexAttrib2sv phải lưu đúng");
    glVertexAttrib3sv(6, s4);
    glGetVertexAttribfv(6, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 7.0f), "glVertexAttrib3sv phải lưu đúng");
    glVertexAttrib4sv(7, s4);
    glGetVertexAttribfv(7, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 8.0f), "glVertexAttrib4sv phải lưu đúng");

    // --- byte / int / unsigned ---
    const GLbyte b4[4] = {1, 2, 3, 4};
    const GLint i4[4] = {5, 6, 7, 8};
    const GLubyte ub4[4] = {9, 10, 11, 12};
    const GLuint ui4[4] = {13, 14, 15, 16};
    const GLushort us4[4] = {17, 18, 19, 20};
    glVertexAttrib4bv(8, b4);
    glGetVertexAttribfv(8, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f), "glVertexAttrib4bv phải lưu đúng");
    glVertexAttrib4iv(9, i4);
    glGetVertexAttribfv(9, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 8.0f), "glVertexAttrib4iv phải lưu đúng");
    glVertexAttrib4ubv(10, ub4);
    glGetVertexAttribfv(10, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 9.0f), "glVertexAttrib4ubv phải lưu đúng");
    glVertexAttrib4uiv(11, ui4);
    glGetVertexAttribfv(11, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 16.0f), "glVertexAttrib4uiv phải lưu đúng");
    glVertexAttrib4usv(12, us4);
    glGetVertexAttribfv(12, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 17.0f), "glVertexAttrib4usv phải lưu đúng");

    // --- normalized ---
    glVertexAttrib4Nub(0, 255, 128, 0, 255);
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[2], 0.0f),
                "glVertexAttrib4Nub phải chuẩn hoá về [0,1]");
    glVertexAttrib4Nubv(1, ub4);
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(v[0] <= 1.0f && v[0] >= 0.0f,
                "glVertexAttrib4Nubv phải chuẩn hoá về [0,1]");
    // GL 4.6 core: 4Nui/4Nus (bản scalar) không tồn tại — chỉ có 4Nuiv/4Nusv.
    // Chuẩn hoá uint32: chia cho 2^32-1 (spec §10.3.3).
    const GLuint nui[4] = {4294967295u, 0u, 2147483648u, 1u};
    glVertexAttrib4Nuiv(2, nui);
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[1], 0.0f) && Near(v[2], 0.5f),
                "glVertexAttrib4Nuiv phải chuẩn hoá theo 2^32-1");
    glVertexAttrib4Nusv(3, us4);
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(v[0] <= 1.0f, "glVertexAttrib4Nusv phải chuẩn hoá về [0,1]");
    glVertexAttrib4Nbv(4, b4);
    glGetVertexAttribfv(4, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(v[0] >= -1.0f && v[0] <= 1.0f,
                "glVertexAttrib4Nbv phải chuẩn hoá về [-1,1]");
    glVertexAttrib4Niv(5, i4);
    glGetVertexAttribfv(5, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(v[0] >= -1.0f && v[0] <= 1.0f,
                "glVertexAttrib4Niv phải chuẩn hoá về [-1,1]");
    glVertexAttrib4Nsv(6, s4);
    glGetVertexAttribfv(6, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(v[0] >= -1.0f && v[0] <= 1.0f,
                "glVertexAttrib4Nsv phải chuẩn hoá về [-1,1]");

    // NULL → INVALID_VALUE.
    DrainErrors();
    glVertexAttrib4bv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttrib4bv NULL → INVALID_VALUE");
    DrainErrors();
    glVertexAttrib4sv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttrib4sv NULL → INVALID_VALUE");
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core raster/state entry points (GL 4.6 §13–§15) ===\n");
    static tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    const Case kCases[] = {
        {"blend state", TestBlendState},
        {"depth range", TestDepthRange},
        {"line/point/stencil/viewport", TestLinePointStencilViewport},
        {"vertex attrib short/byte/normalized", TestShortAttribsAndDoubleUniforms},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
