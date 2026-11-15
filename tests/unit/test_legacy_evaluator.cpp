// test_legacy_evaluator.cpp — EVALUATOR (Compatibility Profile).
//
// Spec (đã đọc trực tiếp từ PDF):
//   • OpenGL 2.1 §5.1 "Evaluators" — glspec21.pdf PDF p.244–250 (1-based;
//     trang in 230–236). Công thức Bernstein (5.1) ở PDF p.244.
//   • OpenGL 4.2 (Compatibility Profile) §5.1 — glspec42.compatibility.pdf
//     PDF p.471–477 (1-based; trang in 451–457). Nội dung GIỐNG HỆT GL 2.1.
//   • OpenGL 2.1 §6.1.3 (GetMap) — PDF p.264; Table 6.28 (state table) —
//     PDF p.307.
//
// Kiểm chứng:
//   1. glMap1/glMap2: target phải là GL_MAP1_*/GL_MAP2_*; order/stride/domain;
//      ACTIVE_TEXTURE ⇒ lỗi đúng (INVALID_ENUM / INVALID_VALUE /
//      INVALID_OPERATION).
//   2. CÔNG THỨC đánh giá (Bernstein): order 2 = nội suy tuyến tính; order 4
//      = đa thức bậc 3 (trọng số 1/8, 3/8, 3/8, 1/8 tại u = 0.5) — expected
//      tính ĐỘC LẬP bằng công thức binomial, không dùng lại code impl.
//   3. Pre-image: p'(u') = p((u'−u1)/(u2−u1)).
//   4. glEvalCoord1/2 sinh đúng vị trí / màu / normal / texcoord / index và
//      KHÔNG đổi current color/normal/texcoord.
//   5. glMap2: bilinear trên 4 góc (VERTEX_4).
//   6. AUTO_NORMAL: normal giải tích của mặt phẳng z = 0.
//   7. glEvalMesh1/2 + glEvalPoint1/2 sinh hình học theo lưới, và hình học
//      đó đi tới đường draw fixed-function (vertex/index buffer được dựng).
//   8. glGetMap: ORDER / COEFF / DOMAIN; thứ tự 2-D (u chạy nhanh hơn v) và
//      round-trip với mảng points đầu vào.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "tglmt/gl_legacy_types.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

const LegacyState& Legacy() { return Context::CurrentPtr()->legacy; }

// C(n,i) — chính xác bằng số nguyên (dùng để tính expected độc lập).
double Binom(int n, int i) {
    double r = 1.0;
    for (int k = 0; k < i; ++k) r = r * (double)(n - k) / (double)(k + 1);
    return r;
}

// Đánh giá độc lập theo công thức Bernstein của spec §5.1 (PDF p.244):
//   p(u) = Σ_i C(n,i)·u^i·(1−u)^(n−i)·R_i
// (KHÔNG dùng de Casteljau như implementation ⇒ 2 đường code độc lập).
double Bernstein1(const double* ctrl, int n, int k, double u, int comp) {
    double sum = 0.0;
    for (int i = 0; i <= n; ++i) {
        double w = Binom(n, i) * std::pow(u, (double)i) *
                   std::pow(1.0 - u, (double)(n - i));
        sum += w * ctrl[(size_t)i * (size_t)k + (size_t)comp];
    }
    return sum;
}

// Đánh giá 1 điểm 1-D và trả về đỉnh sinh ra (đỉnh đầu của batch).
bool EvalOne1(GLfloat u, LegacyVertex* out) {
    glBegin(GL_POINTS);
    glEvalCoord1f(u);
    glEnd();
    const LegacyState& L = Legacy();
    if (L.immediateVertices.empty()) return false;
    *out = L.immediateVertices[0];
    return true;
}

bool EvalOne2(GLfloat u, GLfloat v, LegacyVertex* out) {
    glBegin(GL_POINTS);
    glEvalCoord2f(u, v);
    glEnd();
    const LegacyState& L = Legacy();
    if (L.immediateVertices.empty()) return false;
    *out = L.immediateVertices[0];
    return true;
}

}  // namespace

int main() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context GL 4.6 Compatibility");
    if (!ctx) return 1;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // =====================================================================
    // 0. TRẠNG THÁI ĐẦU (spec §5.1, glspec21.pdf PDF p.250):
    //    "Initially, all maps have order 1 (making them constant maps). All
    //     vertex coordinate maps produce the coordinates (0, 0, 0, 1) (or the
    //     appropriate subset); all normal coordinate maps produce (0, 0, 1);
    //     RGBA maps produce (1, 1, 1, 1); color index maps produce 1.0; and
    //     texture coordinate maps produce (0, 0, 0, 1). In the initial state,
    //     all maps are disabled."
    // =====================================================================
    {
        GLint ord = 0;
        glGetMapiv(GL_MAP1_VERTEX_3, GL_ORDER, &ord);
        TGLMT_CHECK(ord == 1, "trạng thái đầu: order của map = 1 (map hằng)");

        GLfloat v3[3] = {9, 9, 9};
        glGetMapfv(GL_MAP1_VERTEX_3, GL_COEFF, v3);
        TGLMT_CHECK(Near(v3[0], 0.0f) && Near(v3[1], 0.0f) && Near(v3[2], 0.0f),
                    "trạng thái đầu: map vertex = (0,0,0)");

        GLfloat v4[4] = {9, 9, 9, 9};
        glGetMapfv(GL_MAP1_VERTEX_4, GL_COEFF, v4);
        TGLMT_CHECK(Near(v4[0], 0.0f) && Near(v4[1], 0.0f) && Near(v4[2], 0.0f) &&
                        Near(v4[3], 1.0f),
                    "trạng thái đầu: map vertex 4 = (0,0,0,1)");

        GLfloat nrm[3] = {9, 9, 9};
        glGetMapfv(GL_MAP1_NORMAL, GL_COEFF, nrm);
        TGLMT_CHECK(Near(nrm[0], 0.0f) && Near(nrm[1], 0.0f) && Near(nrm[2], 1.0f),
                    "trạng thái đầu: map normal = (0,0,1)");

        GLfloat rgba[4] = {9, 9, 9, 9};
        glGetMapfv(GL_MAP1_COLOR_4, GL_COEFF, rgba);
        TGLMT_CHECK(Near(rgba[0], 1.0f) && Near(rgba[1], 1.0f) && Near(rgba[2], 1.0f) &&
                        Near(rgba[3], 1.0f),
                    "trạng thái đầu: map RGBA = (1,1,1,1)");

        GLfloat idx[1] = {9};
        glGetMapfv(GL_MAP1_INDEX, GL_COEFF, idx);
        TGLMT_CHECK(Near(idx[0], 1.0f), "trạng thái đầu: map color index = 1.0");

        // MAP1_TEXTURE_COORD_1 có k = 1 ⇒ COEFF trả đúng 1 giá trị (s).
        GLfloat tc1[4] = {9, 9, 9, 9};
        glGetMapfv(GL_MAP1_TEXTURE_COORD_1, GL_COEFF, tc1);
        TGLMT_CHECK(Near(tc1[0], 0.0f),
                    "trạng thái đầu: map texture coord 1 = (0)");

        // MAP1_TEXTURE_COORD_4 có k = 4 ⇒ (0,0,0,1).
        GLfloat tc[4] = {9, 9, 9, 9};
        glGetMapfv(GL_MAP1_TEXTURE_COORD_4, GL_COEFF, tc);
        TGLMT_CHECK(Near(tc[0], 0.0f) && Near(tc[1], 0.0f) && Near(tc[2], 0.0f) &&
                        Near(tc[3], 1.0f),
                    "trạng thái đầu: map texture coord 4 = (0,0,0,1)");

        GLfloat dom[2] = {9, 9};
        glGetMapfv(GL_MAP1_VERTEX_3, GL_DOMAIN, dom);
        TGLMT_CHECK(Near(dom[0], 0.0f) && Near(dom[1], 1.0f),
                    "trạng thái đầu: domain 1-D = (0,1)");

        TGLMT_CHECK(glIsEnabled(GL_MAP1_VERTEX_3) == GL_FALSE,
                    "trạng thái đầu: mọi map đều tắt");
        TGLMT_CHECK(glIsEnabled(GL_MAP2_VERTEX_4) == GL_FALSE,
                    "trạng thái đầu: map 2-D cũng tắt");
    }

    // =====================================================================
    // 1. glMap1 + glEvalCoord1: nội suy TUYẾN TÍNH (order 2)
    //    Spec §5.1: p(u) = Σ B_i^n(u) R_i, n = order − 1 = 1
    //             ⇒ p(0.5) = 0.5·R_0 + 0.5·R_1.
    // =====================================================================
    {
        const GLfloat pts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMap1f hợp lệ ⇒ không lỗi");
        glEnable(GL_MAP1_VERTEX_3);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnable(GL_MAP1_VERTEX_3) ⇒ không lỗi");

        LegacyVertex v{};
        TGLMT_CHECK(EvalOne1(0.5f, &v), "glEvalCoord1f sinh ra 1 đỉnh");
        TGLMT_CHECK(Near(v.pos[0], 0.5f) && Near(v.pos[1], 0.5f) && Near(v.pos[2], 0.5f),
                    "order 2: đỉnh = nội suy tuyến tính (0.5,0.5,0.5)");
        TGLMT_CHECK(Near(v.pos[3], 1.0f), "VERTEX_3 ⇒ w = 1");

        TGLMT_CHECK(EvalOne1(0.0f, &v), "đánh giá tại u=0 sinh đỉnh");
        TGLMT_CHECK(Near(v.pos[0], 0.0f) && Near(v.pos[1], 0.0f) && Near(v.pos[2], 0.0f),
                    "u=0 ⇒ R_0");
        TGLMT_CHECK(EvalOne1(1.0f, &v), "đánh giá tại u=1 sinh đỉnh");
        TGLMT_CHECK(Near(v.pos[0], 1.0f) && Near(v.pos[1], 1.0f) && Near(v.pos[2], 1.0f),
                    "u=1 ⇒ R_1");
        glDisable(GL_MAP1_VERTEX_3);
    }

    // =====================================================================
    // 2. order 4: đa thức Bernstein bậc 3 (spec §5.1, PDF p.244)
    //    Trọng số tại u = 0.5: C(3,i)·0.5^i·0.5^(3−i) = 1/8, 3/8, 3/8, 1/8.
    //    Expected tính độc lập bằng binomial + cả giá trị lý thuyết.
    // =====================================================================
    {
        const GLfloat pts[12] = {0.0f, 0.0f, 0.0f,
                                 1.0f, 2.0f, 3.0f,
                                 4.0f, 5.0f, 6.0f,
                                 7.0f, 8.0f, 9.0f};
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 4, pts);
        glEnable(GL_MAP1_VERTEX_3);
        LegacyVertex v{};
        TGLMT_CHECK(EvalOne1(0.5f, &v), "map order 4 sinh đỉnh");
        const double ctrl[12] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        for (int comp = 0; comp < 3; ++comp) {
            const double want = Bernstein1(ctrl, 3, 3, 0.5, comp);
            TGLMT_CHECK(Near(v.pos[comp], (float)want),
                        "order 4: thành phần khớp Bernstein tính độc lập");
        }
        // Giá trị lý thuyết: x = 22/8 = 2.75, y = 29/8 = 3.625, z = 36/8 = 4.5
        TGLMT_CHECK(Near(v.pos[0], 2.75f), "order 4 tại u=0.5: x = 2.75");
        TGLMT_CHECK(Near(v.pos[1], 3.625f), "order 4 tại u=0.5: y = 3.625");
        TGLMT_CHECK(Near(v.pos[2], 4.5f), "order 4 tại u=0.5: z = 4.5");

        // u = 0.25: trọng số C(3,i)·0.25^i·0.75^(3−i) = 27/64, 27/64, 9/64, 1/64
        TGLMT_CHECK(EvalOne1(0.25f, &v), "map order 4 đánh giá tại u=0.25");
        for (int comp = 0; comp < 3; ++comp) {
            const double want = Bernstein1(ctrl, 3, 3, 0.25, comp);
            TGLMT_CHECK(Near(v.pos[comp], (float)want),
                        "order 4 tại u=0.25 khớp Bernstein độc lập");
        }
        glDisable(GL_MAP1_VERTEX_3);
    }

    // =====================================================================
    // 3. Pre-image: p'(u') = p((u'−u1)/(u2−u1))  (spec §5.1, PDF p.245)
    // =====================================================================
    {
        const GLfloat pts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
        glMap1f(GL_MAP1_VERTEX_3, 2.0f, 6.0f, 3, 2, pts);
        glEnable(GL_MAP1_VERTEX_3);
        LegacyVertex v{};
        TGLMT_CHECK(EvalOne1(4.0f, &v), "đánh giá trong pre-image [2,6]");
        TGLMT_CHECK(Near(v.pos[0], 0.5f) && Near(v.pos[1], 0.5f) && Near(v.pos[2], 0.5f),
                    "pre-image: u'=4 ⇒ (4−2)/(6−2) = 0.5");
        TGLMT_CHECK(EvalOne1(2.0f, &v), "đánh giá tại u'=u1");
        TGLMT_CHECK(Near(v.pos[0], 0.0f), "u'=u1 ⇒ R_0");
        TGLMT_CHECK(EvalOne1(6.0f, &v), "đánh giá tại u'=u2");
        TGLMT_CHECK(Near(v.pos[0], 1.0f), "u'=u2 ⇒ R_1");
        glDisable(GL_MAP1_VERTEX_3);
    }

    // =====================================================================
    // 4. glMap2: bilinear trên 4 góc (VERTEX_4)
    //    Layout client: R_ij ở points[ustride·i + vstride·j]
    //    (spec §5.1, PDF p.246) ⇒ ustride = 4 (k), vstride = 8 (uorder·k).
    // =====================================================================
    {
        const GLfloat pts[16] = {
            0.0f, 0.0f, 0.0f, 1.0f,   // R_00
            1.0f, 0.0f, 0.0f, 1.0f,   // R_10  (+ustride = +4)
            0.0f, 1.0f, 0.0f, 1.0f,   // R_01  (+vstride = +8)
            1.0f, 1.0f, 0.0f, 1.0f    // R_11  (+12)
        };
        glMap2f(GL_MAP2_VERTEX_4, 0.0f, 1.0f, 4, 2, 0.0f, 1.0f, 8, 2, pts);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glMap2f hợp lệ ⇒ không lỗi");
        glEnable(GL_MAP2_VERTEX_4);

        LegacyVertex v{};
        TGLMT_CHECK(EvalOne2(0.5f, 0.5f, &v), "glEvalCoord2f sinh đỉnh");
        TGLMT_CHECK(Near(v.pos[0], 0.5f) && Near(v.pos[1], 0.5f) &&
                        Near(v.pos[2], 0.0f) && Near(v.pos[3], 1.0f),
                    "glMap2 bilinear: (0.5,0.5,0,1) = trung bình 4 góc");

        // Góc: u,v = (0,0) ⇒ R_00 ; (1,1) ⇒ R_11.
        TGLMT_CHECK(EvalOne2(0.0f, 0.0f, &v), "đánh giá tại góc (0,0)");
        TGLMT_CHECK(Near(v.pos[0], 0.0f) && Near(v.pos[1], 0.0f),
                    "(u,v)=(0,0) ⇒ R_00");
        TGLMT_CHECK(EvalOne2(1.0f, 1.0f, &v), "đánh giá tại góc (1,1)");
        TGLMT_CHECK(Near(v.pos[0], 1.0f) && Near(v.pos[1], 1.0f),
                    "(u,v)=(1,1) ⇒ R_11");
        glDisable(GL_MAP2_VERTEX_4);
    }

    // =====================================================================
    // 5. Map màu + map texture + map index: giá trị ĐÁNH GIÁ được dùng cho
    //    đỉnh, nhưng current state KHÔNG bị đổi (spec §5.1, PDF p.247:
    //    "Use of evaluators has no effect on the current color, normal, or
    //     texture coordinates").
    // =====================================================================
    {
        const GLfloat cpts[8] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        const GLfloat tpts[4] = {0.25f, 0.75f, 0.5f, 0.5f};
        const GLfloat ipts[2] = {3.0f, 7.0f};
        glMap1f(GL_MAP1_COLOR_4, 0.0f, 1.0f, 4, 2, cpts);
        glMap1f(GL_MAP1_TEXTURE_COORD_2, 0.0f, 1.0f, 2, 2, tpts);
        glMap1f(GL_MAP1_INDEX, 0.0f, 1.0f, 1, 2, ipts);
        glEnable(GL_MAP1_COLOR_4);
        glEnable(GL_MAP1_TEXTURE_COORD_2);
        glEnable(GL_MAP1_INDEX);
        glEnable(GL_MAP1_VERTEX_3);
        const GLfloat vpts[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, vpts);

        glColor4f(0.25f, 0.5f, 0.75f, 1.0f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.1f, 0.2f);

        LegacyVertex v{};
        TGLMT_CHECK(EvalOne1(0.5f, &v), "đánh giá với map màu/texture/index");
        TGLMT_CHECK(Near(v.color[0], 0.5f) && Near(v.color[1], 0.5f) &&
                        Near(v.color[2], 0.5f) && Near(v.color[3], 1.0f),
                    "map COLOR_4: màu đỉnh = (0.5,0.5,0.5,1)");
        TGLMT_CHECK(Near(v.texCoord[0], 0.375f) && Near(v.texCoord[1], 0.625f) &&
                        Near(v.texCoord[2], 0.0f) && Near(v.texCoord[3], 1.0f),
                    "map TEXTURE_COORD_2: (s,t) = (0.375,0.625), r=0, q=1");
        TGLMT_CHECK(v.index == 5, "map INDEX: color index = floor(5.0) = 5");

        // Current state KHÔNG đổi.
        const LegacyState& L = Legacy();
        TGLMT_CHECK(Near(L.currentColor[0], 0.25f) && Near(L.currentColor[1], 0.5f) &&
                        Near(L.currentColor[2], 0.75f),
                    "evaluator KHÔNG đổi current color");
        TGLMT_CHECK(Near(L.currentTexCoord[0][0], 0.1f) &&
                        Near(L.currentTexCoord[0][1], 0.2f),
                    "evaluator KHÔNG đổi current texture coord");

        glDisable(GL_MAP1_COLOR_4);
        glDisable(GL_MAP1_TEXTURE_COORD_2);
        glDisable(GL_MAP1_INDEX);
        glDisable(GL_MAP1_VERTEX_3);
    }

    // =====================================================================
    // 6. AUTO_NORMAL: normal giải tích của mặt phẳng z = 0 (spec §5.1,
    //    PDF p.247: m = ∂q/∂u × ∂q/∂v, n = m/‖m‖).
    // =====================================================================
    {
        const GLfloat npts[12] = {0.0f, 0.0f, 0.0f,   // R_00
                                  1.0f, 0.0f, 0.0f,   // R_10
                                  0.0f, 1.0f, 0.0f,   // R_01
                                  1.0f, 1.0f, 0.0f};  // R_11
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 6, 2, npts);
        glEnable(GL_MAP2_VERTEX_3);
        glEnable(GL_AUTO_NORMAL);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnable(GL_AUTO_NORMAL) ⇒ không lỗi");

        LegacyVertex v{};
        TGLMT_CHECK(EvalOne2(0.5f, 0.5f, &v), "đánh giá với AUTO_NORMAL");
        TGLMT_CHECK(Near(v.normal[0], 0.0f) && Near(v.normal[1], 0.0f) &&
                        Near(v.normal[2], 1.0f),
                    "AUTO_NORMAL: mặt z=0 ⇒ normal = (0,0,1)");

        // Tắt AUTO_NORMAL ⇒ normal lấy từ map normal (nếu bật) hoặc current.
        glDisable(GL_AUTO_NORMAL);
        glDisable(GL_MAP2_VERTEX_3);
    }

    // =====================================================================
    // 7. Lỗi: target / order / stride / domain / ACTIVE_TEXTURE.
    //    Spec §5.1 (glspec21.pdf PDF p.245–246):
    //      • target không hợp lệ            ⇒ INVALID_ENUM
    //      • order < 1 hoặc > MAX_EVAL_ORDER ⇒ INVALID_VALUE
    //      • stride < k                     ⇒ INVALID_VALUE
    //      • u1 = u2 (hoặc v1 = v2)         ⇒ INVALID_VALUE
    //      • ACTIVE_TEXTURE ≠ TEXTURE0      ⇒ INVALID_OPERATION
    // =====================================================================
    {
        const GLfloat pts[12] = {0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        DrainErrors();
        glMap1f(GL_FOG_COORD, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glMap1f target không phải map ⇒ INVALID_ENUM");

        DrainErrors();
        glMap1f(GL_MAX_EVAL_ORDER, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glMap1f với enum không phải map ⇒ INVALID_ENUM");

        DrainErrors();
        glMap1f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glMap1f với target 2-D ⇒ INVALID_ENUM");

        DrainErrors();
        glMap2f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glMap2f với target 1-D ⇒ INVALID_ENUM");

        DrainErrors();
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 0, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "glMap1f order = 0 (< 1) ⇒ INVALID_VALUE");

        DrainErrors();
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 257, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "glMap1f order > MAX_EVAL_ORDER ⇒ INVALID_VALUE");

        DrainErrors();
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 2, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "glMap1f stride (2) < k (3) ⇒ INVALID_VALUE");

        DrainErrors();
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 0, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "glMap1f stride = 0 (< k) ⇒ INVALID_VALUE (spec §5.1)");

        DrainErrors();
        glMap1f(GL_MAP1_VERTEX_3, 1.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMap1f u1 = u2 ⇒ INVALID_VALUE");

        DrainErrors();
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 0, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMap2f uorder = 0 ⇒ INVALID_VALUE");

        DrainErrors();
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 2, 2, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "glMap2f ustride < k ⇒ INVALID_VALUE");

        DrainErrors();
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 3, 0, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMap2f vorder = 0 ⇒ INVALID_VALUE");

        DrainErrors();
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 2.0f, 2.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMap2f v1 = v2 ⇒ INVALID_VALUE");

        // ACTIVE_TEXTURE ≠ TEXTURE0 ⇒ INVALID_OPERATION (spec §5.1 PDF p.246).
        DrainErrors();
        glActiveTexture(GL_TEXTURE1);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glActiveTexture(GL_TEXTURE1) ⇒ không lỗi");
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glMap1f khi ACTIVE_TEXTURE = TEXTURE1 ⇒ INVALID_OPERATION");
        glMap2f(GL_MAP2_VERTEX_3, 0.0f, 1.0f, 3, 2, 0.0f, 1.0f, 6, 2, pts);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glMap2f khi ACTIVE_TEXTURE = TEXTURE1 ⇒ INVALID_OPERATION");
        glActiveTexture(GL_TEXTURE0);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glActiveTexture(GL_TEXTURE0) ⇒ không lỗi");
    }

    // =====================================================================
    // 8. Lỗi lưới + EvalMesh: mode sai ⇒ INVALID_ENUM; grid ≤ 0 ⇒
    //    INVALID_VALUE (spec §5.1, PDF p.248).
    // =====================================================================
    {
        DrainErrors();
        glMapGrid1d(0, 0.0, 1.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMapGrid1d n = 0 ⇒ INVALID_VALUE");
        DrainErrors();
        glMapGrid1d(-1, 0.0, 1.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMapGrid1d n < 0 ⇒ INVALID_VALUE");
        DrainErrors();
        glMapGrid2d(1, 0.0, 1.0, 0, 0.0, 1.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMapGrid2d nv = 0 ⇒ INVALID_VALUE");
        DrainErrors();
        glMapGrid2d(0, 0.0, 1.0, 1, 0.0, 1.0);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glMapGrid2d nu = 0 ⇒ INVALID_VALUE");

        DrainErrors();
        glEvalMesh1(GL_TRIANGLES, 0, 1);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glEvalMesh1(GL_TRIANGLES) ⇒ INVALID_ENUM (mode ∈ {POINT,LINE})");
        DrainErrors();
        glEvalMesh1(GL_LINE_STRIP, 0, 1);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glEvalMesh1(GL_LINE_STRIP) ⇒ INVALID_ENUM");
        DrainErrors();
        glEvalMesh2(GL_TRIANGLES, 0, 1, 0, 1);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glEvalMesh2(GL_TRIANGLES) ⇒ INVALID_ENUM (mode ∈ {FILL,LINE,POINT})");
        DrainErrors();
        glEvalMesh2(GL_QUAD_STRIP, 0, 1, 0, 1);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glEvalMesh2(GL_QUAD_STRIP) ⇒ INVALID_ENUM");
    }

    // =====================================================================
    // 9. glEvalCoord ngoài glBegin/glEnd ⇒ INVALID_OPERATION; không có map
    //    vertex bật ⇒ "nothing happens" (spec §5.1, PDF p.250).
    // =====================================================================
    {
        const GLfloat pts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, pts);
        glEnable(GL_MAP1_VERTEX_3);

        DrainErrors();
        glEvalCoord1f(0.5f);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glEvalCoord1f ngoài glBegin/glEnd ⇒ INVALID_OPERATION");

        // Không bật map vertex ⇒ KHÔNG sinh đỉnh và KHÔNG sinh lỗi.
        glDisable(GL_MAP1_VERTEX_3);
        DrainErrors();
        glBegin(GL_POINTS);
        glEvalCoord1f(0.5f);
        glEnd();
        TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                    "EvalCoord khi không có map vertex ⇒ không lỗi");
        TGLMT_CHECK(Legacy().immediateVertices.empty(),
                    "EvalCoord khi không có map vertex ⇒ không sinh đỉnh");
    }

    // =====================================================================
    // 10. glGetMap: ORDER / COEFF / DOMAIN + thứ tự 2-D (u nhanh hơn v).
    //     Spec §6.1.3 (PDF p.264) + Table 6.28 (PDF p.307).
    // =====================================================================
    {
        const GLfloat pts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
        glMap1f(GL_MAP1_VERTEX_3, 2.0f, 6.0f, 3, 2, pts);

        DrainErrors();
        GLfloat coeff[6] = {0};
        glGetMapfv(GL_MAP1_VERTEX_3, GL_COEFF, coeff);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetMapfv(GL_COEFF) ⇒ không lỗi");
        bool same = true;
        for (int i = 0; i < 6; ++i) same = same && Near(coeff[i], pts[i]);
        TGLMT_CHECK(same, "glGetMapfv(GL_COEFF) trả lại đúng control point");

        GLint ord = 0;
        glGetMapiv(GL_MAP1_VERTEX_3, GL_ORDER, &ord);
        TGLMT_CHECK(ord == 2, "glGetMapiv(GL_ORDER) = 2");

        GLfloat dom[2] = {0};
        glGetMapfv(GL_MAP1_VERTEX_3, GL_DOMAIN, dom);
        TGLMT_CHECK(Near(dom[0], 2.0f) && Near(dom[1], 6.0f),
                    "glGetMapfv(GL_DOMAIN) = (u1,u2) = (2,6)");

        GLdouble dcoeff[6] = {0};
        glGetMapdv(GL_MAP1_VERTEX_3, GL_COEFF, dcoeff);
        bool dsame = true;
        for (int i = 0; i < 6; ++i) dsame = dsame && (dcoeff[i] == (GLdouble)pts[i]);
        TGLMT_CHECK(dsame, "glGetMapdv(GL_COEFF) trả control point dạng double");

        GLint icoeff[6] = {0};
        glGetMapiv(GL_MAP1_VERTEX_3, GL_COEFF, icoeff);
        bool isame = true;
        for (int i = 0; i < 6; ++i) isame = isame && (icoeff[i] == (GLint)pts[i]);
        TGLMT_CHECK(isame, "glGetMapiv(GL_COEFF) trả control point dạng int");

        // ---- map 2-D: thứ tự R_ij = u nhanh hơn v (KHỚP layout client
        //      points[ustride·i + vstride·j]) ----
        const GLfloat pts2[16] = {
            0.0f, 0.0f, 0.0f, 1.0f,   // R_00
            1.0f, 0.0f, 0.0f, 1.0f,   // R_10
            0.0f, 1.0f, 0.0f, 1.0f,   // R_01
            1.0f, 1.0f, 0.0f, 1.0f    // R_11
        };
        glMap2f(GL_MAP2_VERTEX_4, 0.0f, 1.0f, 4, 2, 0.0f, 1.0f, 8, 2, pts2);

        GLfloat coeff2[16] = {0};
        glGetMapfv(GL_MAP2_VERTEX_4, GL_COEFF, coeff2);
        bool same2 = true;
        for (int i = 0; i < 16; ++i) same2 = same2 && Near(coeff2[i], pts2[i]);
        TGLMT_CHECK(same2,
                    "glGetMapfv 2-D: COEFF theo thứ tự u-nhanh-hơn-v (round-trip)");

        GLint ord2[2] = {0, 0};
        glGetMapiv(GL_MAP2_VERTEX_4, GL_ORDER, ord2);
        TGLMT_CHECK(ord2[0] == 2 && ord2[1] == 2,
                    "glGetMapiv 2-D: ORDER = (uorder, vorder) = (2,2)");

        GLfloat dom2[4] = {0};
        glGetMapfv(GL_MAP2_VERTEX_4, GL_DOMAIN, dom2);
        TGLMT_CHECK(Near(dom2[0], 0.0f) && Near(dom2[1], 1.0f) &&
                        Near(dom2[2], 0.0f) && Near(dom2[3], 1.0f),
                    "glGetMapfv 2-D: DOMAIN = (u1,u2,v1,v2)");

        // Query sai ⇒ INVALID_ENUM.
        DrainErrors();
        glGetMapfv(GL_MAP1_VERTEX_3, 0x1234, coeff);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glGetMapfv query không hợp lệ ⇒ INVALID_ENUM");
        DrainErrors();
        glGetMapfv(GL_FOG_COORD, GL_COEFF, coeff);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "glGetMapfv target không phải map ⇒ INVALID_ENUM");
    }

    // =====================================================================
    // 11. glEvalMesh1: sinh hình học theo lưới, ĐI TỚI đường draw
    //     fixed-function.
    // =====================================================================
    {
        const GLfloat pts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};  // x = u
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, pts);
        glEnable(GL_MAP1_VERTEX_3);
        glMapGrid1d(4, 0.0f, 1.0f);

        DrainErrors();
        glEvalMesh1(GL_LINE, 0, 4);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEvalMesh1(GL_LINE) ⇒ không lỗi");
        const LegacyState& L = Legacy();
        TGLMT_CHECK(L.immediateVertices.size() == 5,
                    "glEvalMesh1(GL_LINE, 0, 4) sinh 5 đỉnh (i = 0..4)");
        TGLMT_CHECK(L.immediatePrimitives.size() == 4,
                    "glEvalMesh1(GL_LINE) ⇒ LINE_STRIP ⇒ 4 đoạn");
        if (L.immediateVertices.size() == 5) {
            const float want[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            bool ok = true;
            for (int i = 0; i < 5; ++i) ok = ok && Near(L.immediateVertices[i].pos[0], want[i]);
            TGLMT_CHECK(ok, "lưới 4 phân đoạn: u = 0, 0.25, 0.5, 0.75, 1");
        }
        // Hình học đã đi tới đường draw (vertex/index buffer được dựng).
        TGLMT_CHECK(c.legacy.vertexCount == 5, "5 đỉnh được đẩy lên vertex buffer");
        TGLMT_CHECK(c.legacy.indexCount == 8, "8 index (4 đoạn × 2 đỉnh)");

        DrainErrors();
        glEvalMesh1(GL_POINT, 0, 4);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEvalMesh1(GL_POINT) ⇒ không lỗi");
        TGLMT_CHECK(Legacy().immediateVertices.size() == 5,
                    "glEvalMesh1(GL_POINT) sinh 5 đỉnh");
        TGLMT_CHECK(Legacy().immediatePrimitives.size() == 5,
                    "glEvalMesh1(GL_POINT) ⇒ 5 điểm");
        glDisable(GL_MAP1_VERTEX_3);
    }

    // =====================================================================
    // 12. glEvalMesh2 (FILL) + glEvalPoint1/2.
    // =====================================================================
    {
        const GLfloat pts2[16] = {
            0.0f, 0.0f, 0.0f, 1.0f,   // R_00
            1.0f, 0.0f, 0.0f, 1.0f,   // R_10
            0.0f, 1.0f, 0.0f, 1.0f,   // R_01
            1.0f, 1.0f, 0.0f, 1.0f    // R_11
        };
        glMap2f(GL_MAP2_VERTEX_4, 0.0f, 1.0f, 4, 2, 0.0f, 1.0f, 8, 2, pts2);
        glEnable(GL_MAP2_VERTEX_4);
        glMapGrid2d(1, 0.0f, 1.0f, 1, 0.0f, 1.0f);

        DrainErrors();
        glEvalMesh2(GL_FILL, 0, 1, 0, 1);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEvalMesh2(GL_FILL) ⇒ không lỗi");
        const LegacyState& L = Legacy();
        TGLMT_CHECK(L.immediateVertices.size() == 4,
                    "glEvalMesh2(GL_FILL, 0,1, 0,1) sinh 4 đỉnh (1 quad strip)");
        TGLMT_CHECK(L.immediatePrimitives.size() == 2,
                    "1 quad strip ⇒ 2 tam giác");
        if (L.immediateVertices.size() == 4) {
            const float wu[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            const float wv[4] = {0.0f, 1.0f, 0.0f, 1.0f};
            bool ok = true;
            for (int i = 0; i < 4; ++i)
                ok = ok && Near(L.immediateVertices[i].pos[0], wu[i]) &&
                     Near(L.immediateVertices[i].pos[1], wv[i]);
            TGLMT_CHECK(ok, "quad strip: (u,v) = (0,0),(0,1),(1,0),(1,1)");
        }

        // glEvalPoint1/2 = EvalCoord tại điểm lưới.
        const GLfloat lpts[6] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};  // x = u
        glMap1f(GL_MAP1_VERTEX_3, 0.0f, 1.0f, 3, 2, lpts);
        glEnable(GL_MAP1_VERTEX_3);
        glMapGrid1d(4, 0.0f, 1.0f);
        LegacyVertex v{};
        glBegin(GL_POINTS);
        glEvalPoint1(2);
        glEnd();
        TGLMT_CHECK(Legacy().immediateVertices.size() == 1, "glEvalPoint1 sinh 1 đỉnh");
        if (!Legacy().immediateVertices.empty()) {
            v = Legacy().immediateVertices[0];
            TGLMT_CHECK(Near(v.pos[0], 0.5f),
                        "glEvalPoint1(2) với lưới 4 phân đoạn ⇒ u = 0.5");
        }

        glMapGrid2d(2, 0.0f, 1.0f, 4, 0.0f, 1.0f);
        glBegin(GL_POINTS);
        glEvalPoint2(1, 2);
        glEnd();
        TGLMT_CHECK(Legacy().immediateVertices.size() == 1, "glEvalPoint2 sinh 1 đỉnh");
        if (!Legacy().immediateVertices.empty()) {
            v = Legacy().immediateVertices[0];
            TGLMT_CHECK(Near(v.pos[0], 0.5f) && Near(v.pos[1], 0.5f),
                        "glEvalPoint2(1,2): u = 1/2, v = 2/4");
        }
        glDisable(GL_MAP1_VERTEX_3);
        glDisable(GL_MAP2_VERTEX_4);
    }

    // =====================================================================
    // 13. glMapGrid bị cấm trong glBegin/glEnd (spec §2.6.3: chỉ Vertex,
    //     Color, Normal, TexCoord, EvalCoord, EvalPoint, … được phép).
    // =====================================================================
    {
        DrainErrors();
        glBegin(GL_POINTS);
        glMapGrid1d(2, 0.0f, 1.0f);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glMapGrid1d trong glBegin/glEnd ⇒ INVALID_OPERATION");
        glEnd();
    }

    tglmtDestroyContext(ctx);
    return TGLMT_RESULT;
}
