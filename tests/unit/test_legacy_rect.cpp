// test_legacy_rect.cpp — glRect* (OpenGL Compatibility Profile).
//
// Spec: OpenGL 2.1 §2.10 "Rectangles" (glspec21.pdf PDF p.40, idx 53):
//       "The effect of the Rect command Rect(x1,y1,x2,y2) is exactly the same
//        as the following sequence of commands:
//        Begin(POLYGON); Vertex2(x1,y1); Vertex2(x2,y1); Vertex2(x2,y2);
//        Vertex2(x1,y2); End();"
//       OpenGL 4.2C §2.11 "RECTANGLES" (PDF p.69–70) — văn bản giữ nguyên.
//       Quy tắc chuyển đổi kiểu: GL 2.1 §2.7 (PDF p.20) + Table 2.9
//       (PDF p.59) — Table 2.9 CHỈ áp cho color/normal/depth ⇒ toạ độ đỉnh
//       của biến thể integer/short chuyển THẲNG sang float.
//
// Kiểm tra:
//  1. glRectf(0,0,1,1) ⇒ 4 đỉnh đúng vị trí, 2 tam giác (1 polygon), và 1
//     draw trong DrawTrace của backend Null.
//  2. Current raster position được cập nhật về góc trên-trái.
//  3. Biến thể int/short/double cho KẾT QUẢ GIỐNG HỆT biến thể float.
//  4. Lỗi: Rect trong glBegin/glEnd ⇒ INVALID_OPERATION; con trỏ NULL ⇒
//     INVALID_VALUE.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {

constexpr GLenum kPolygon = GL_POLYGON;

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }
void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

// 4 đỉnh của hình chữ nhật (0,0)-(1,1) theo thứ tự spec: (x1,y1), (x2,y1),
// (x2,y2), (x1,y2).
bool RectVerticesAre(const Context& c) {
    if (c.legacy.immediateVertices.size() != 4) return false;
    const float ex[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    const float ey[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    for (size_t i = 0; i < 4; ++i) {
        const LegacyVertex& v = c.legacy.immediateVertices[i];
        if (!Near(v.pos[0], ex[i]) || !Near(v.pos[1], ey[i])) return false;
        if (!Near(v.pos[2], 0.0f) || !Near(v.pos[3], 1.0f)) return false;
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. Hình chữ nhật tới đường draw
// ---------------------------------------------------------------------------
static void TestRectDraw() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const size_t traceBefore = c.device->drawTrace().size();
    glRectf(0.0f, 0.0f, 1.0f, 1.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectf ⇒ không lỗi");

    // Spec: Rect ≡ Begin(POLYGON) + 4×Vertex2 + End ⇒ 4 đỉnh, 1 polygon
    // (assemble thành 2 tam giác vì Metal không có polygon).
    TGLMT_CHECK(c.legacy.beginMode == kPolygon, "primitive mode = GL_POLYGON (theo spec)");
    TGLMT_CHECK(RectVerticesAre(c), "4 đỉnh đúng vị trí (x1,y1),(x2,y1),(x2,y2),(x1,y2)");
    TGLMT_CHECK(c.legacy.immediatePrimitives.size() == 2,
                "polygon 4 đỉnh ⇒ 2 tam giác sau khi assemble");
    TGLMT_CHECK(c.legacy.vertexCount == 4, "4 đỉnh được đẩy lên vertex buffer");
    TGLMT_CHECK(c.legacy.indexCount == 6, "6 index (2 tam giác)");
    if (c.legacy.indexData.size() == 6) {
        // Winding: tam giác (0,1,2) rồi (0,2,3) — người thuận khi x2>x1,y2>y1.
        TGLMT_CHECK(c.legacy.indexData[0] == 0 && c.legacy.indexData[1] == 1 &&
                        c.legacy.indexData[2] == 2 && c.legacy.indexData[3] == 0 &&
                        c.legacy.indexData[4] == 2 && c.legacy.indexData[5] == 3,
                    "index = (0,1,2) + (0,2,3) — winding CCW");
    }

    // glEnd phải render ngay (spec GL 2.1 §2.6) ⇒ 1 draw mới trong trace.
    TGLMT_CHECK(c.device->drawTrace().size() == traceBefore + 1,
                "glRectf phát draw ngay (1 draw mới trong DrawTrace)");
    if (c.device->drawTrace().size() == traceBefore + 1) {
        const metal::DrawTrace& t = c.device->drawTrace().back();
        TGLMT_CHECK(t.prim == metal::PrimitiveType::Triangle, "trace: primitive = Triangle");
        TGLMT_CHECK(t.count == 6, "trace: count = 6 index");
        TGLMT_CHECK(t.indexed, "trace: draw có index");
    }

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. Current raster position = góc trên-trái
// ---------------------------------------------------------------------------
static void TestRasterPosition() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Viewport 64×64, modelview/projection identity ⇒ toạ độ window:
    //   xw = (w/2)·xd + (x + w/2) = 32·xd + 32
    //   yw = (h/2)·yd + (y + h/2) = 32·yd + 32
    // Góc trên-trái của hình chữ nhật (0,0)-(1,1) = object (0,1) ⇒ window
    // (32, 64).
    glViewport(0, 0, 64, 64);
    DrainErrors();

    glRectf(0.0f, 0.0f, 1.0f, 1.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectf ⇒ không lỗi");
    TGLMT_CHECK(c.legacy.raster.valid, "current raster position hợp lệ sau glRectf");
    TGLMT_CHECK(Near(c.legacy.raster.x, 32.0f),
                "raster position x = 32 (góc trên-trái theo window coords)");
    TGLMT_CHECK(Near(c.legacy.raster.y, 64.0f),
                "raster position y = 64 (góc trên-trái theo window coords)");

    // Hình chữ nhật "đảo" (x2<x1, y2<y1): góc trên-trái vẫn là góc có
    // x nhỏ nhất, y lớn nhất trong window coords ⇒ (0, 64) (object (0,1)).
    glRectf(1.0f, 1.0f, 0.0f, 0.0f);
    TGLMT_CHECK(c.legacy.raster.valid, "raster position hợp lệ sau rect đảo chiều");
    TGLMT_CHECK(Near(c.legacy.raster.x, 32.0f) && Near(c.legacy.raster.y, 64.0f),
                "rect đảo chiều: góc trên-trái vẫn (32,64)");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. Biến thể int / short / double cho kết quả giống float
// ---------------------------------------------------------------------------
static void TestVariants() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();
    glViewport(0, 0, 64, 64);

    // --- glRectd ---
    glRectd(0.0, 0.0, 1.0, 1.0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectd ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRectd cho 4 đỉnh giống hệt biến thể float");

    // --- glRecti: chuyển THẲNG sang float (KHÔNG Table 2.9) ---
    glRecti(0, 0, 1, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRecti ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c),
                "glRecti chuyển toạ độ thẳng sang float (Table 2.9 KHÔNG áp cho toạ độ)");

    // --- glRects ---
    glRects(0, 0, 1, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRects ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRects cho 4 đỉnh giống hệt biến thể float");

    // --- các biến thể con trỏ ---
    const GLdouble dv1[2] = {0.0, 0.0}, dv2[2] = {1.0, 1.0};
    glRectdv(dv1, dv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectdv ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRectdv cho 4 đỉnh giống hệt");

    const GLfloat fv1[2] = {0.0f, 0.0f}, fv2[2] = {1.0f, 1.0f};
    glRectfv(fv1, fv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectfv ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRectfv cho 4 đỉnh giống hệt");

    const GLint iv1[2] = {0, 0}, iv2[2] = {1, 1};
    glRectiv(iv1, iv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectiv ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRectiv cho 4 đỉnh giống hệt");

    const GLshort sv1[2] = {0, 0}, sv2[2] = {1, 1};
    glRectsv(sv1, sv2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glRectsv ⇒ không lỗi");
    TGLMT_CHECK(RectVerticesAre(c), "glRectsv cho 4 đỉnh giống hệt");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Lỗi
// ---------------------------------------------------------------------------
static void TestErrors() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Rect trong glBegin/glEnd ⇒ INVALID_OPERATION (GL 2.1 §2.6.3 p.19–20).
    glBegin(kPolygon);
    glRectf(0.0f, 0.0f, 1.0f, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glRectf trong glBegin/glEnd ⇒ INVALID_OPERATION");
    glEnd();
    DrainErrors();

    // Con trỏ NULL ⇒ INVALID_VALUE.
    glRectfv(nullptr, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glRectfv(NULL,NULL) ⇒ INVALID_VALUE");
    DrainErrors();
    glRectiv(nullptr, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glRectiv(NULL,NULL) ⇒ INVALID_VALUE");

    tglmtDestroyContext(ctx);
}

int main() {
    TestRectDraw();
    TestRasterPosition();
    TestVariants();
    TestErrors();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_rect: OK\n");
        return 0;
    }
    return 1;
}
