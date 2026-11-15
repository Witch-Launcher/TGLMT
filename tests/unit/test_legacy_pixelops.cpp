// test_legacy_pixelops.cpp — PIXEL TRANSFER / PIXEL MAPS / PIXEL ZOOM
//                          (Compatibility Profile).
//
// Spec: OpenGL 2.1 §3.6.3 "Pixel Transfer Modes" (glspec21.pdf PDF p.116–118)
//       — Table 3.2 (PixelTransfer), Table 3.3 (PixelMap), quy tắc lỗi.
//       OpenGL 4.2C §3.7.3 (PDF p.247) — văn bản tương đương.
//
// Kiểm tra:
//  1. Pixel transfer: pname sai ⇒ INVALID_ENUM; pname kiểu integer nhận giá
//     trị float không nguyên ⇒ INVALID_VALUE; state scale/bias lưu đúng.
//  2. Pixel map: set/get round-trip cho CẢ 3 biến thể (fv/uiv/usv).
//  3. Map enum sai ⇒ INVALID_ENUM (cả set lẫn get).
//  4. size > GL_MAX_PIXEL_MAP_TABLE (256) ⇒ INVALID_VALUE; size < 1 ⇒
//     INVALID_VALUE.
//  5. glPixelZoom(2,3) ⇒ state zoom = (2,3).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

// API công khai do src/gl/gl_legacy_pixelops.cpp cung cấp (state pixel zoom
// lưu trong shadow state vì LegacyState không có field cho nó).
namespace tglmt {
namespace gl {
bool LegacyPixelZoomState(const Context& c, float* xfactor, float* yfactor);
}
}  // namespace tglmt

namespace {

constexpr GLenum kRedScale = GL_RED_SCALE;
constexpr GLenum kGreenScale = GL_GREEN_SCALE;
constexpr GLenum kBlueBias = GL_BLUE_BIAS;
constexpr GLenum kAlphaScale = GL_ALPHA_SCALE;
constexpr GLenum kDepthScale = GL_DEPTH_SCALE;
constexpr GLenum kIndexShift = GL_INDEX_SHIFT;
constexpr GLenum kIndexOffset = GL_INDEX_OFFSET;
constexpr GLenum kMapColor = GL_MAP_COLOR;
constexpr GLenum kPixelMapIToR = GL_PIXEL_MAP_I_TO_R;
constexpr GLenum kPixelMapIToI = GL_PIXEL_MAP_I_TO_I;
constexpr GLenum kBadPname = 0x8123;  // không phải pname pixel transfer nào

constexpr GLsizei kMaxPixelMapTable = 256;  // GL_MAX_PIXEL_MAP_TABLE của TGLMT

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

}  // namespace

// ---------------------------------------------------------------------------
// 1. Pixel transfer — pname validation + state
// ---------------------------------------------------------------------------
static void TestPixelTransfer() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // --- glPixelTransferf với pname hợp lệ ---
    glPixelTransferf(kRedScale, 2.0f);
    glPixelTransferf(kGreenScale, 0.5f);
    glPixelTransferf(kBlueBias, 0.25f);
    glPixelTransferf(kAlphaScale, 1.5f);
    glPixelTransferf(kDepthScale, 3.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pixel transfer pname hợp lệ ⇒ không lỗi");
    TGLMT_CHECK(Near(c.legacy.pixelTransfer.redScale, 2.0f), "RED_SCALE = 2.0");
    TGLMT_CHECK(Near(c.legacy.pixelTransfer.greenScale, 0.5f), "GREEN_SCALE = 0.5");
    TGLMT_CHECK(Near(c.legacy.pixelTransfer.blueBias, 0.25f), "BLUE_BIAS = 0.25");
    TGLMT_CHECK(Near(c.legacy.pixelTransfer.alphaScale, 1.5f), "ALPHA_SCALE = 1.5");
    TGLMT_CHECK(Near(c.legacy.pixelTransfer.depthScale, 3.0f), "DEPTH_SCALE = 3.0");

    // --- glPixelTransferi với pname kiểu integer ---
    glPixelTransferi(kIndexOffset, 4);
    glPixelTransferi(kIndexShift, 2);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "pixel transfer integer pname ⇒ không lỗi");
    TGLMT_CHECK(c.legacy.pixelTransfer.indexOffset == 4u, "INDEX_OFFSET = 4");
    TGLMT_CHECK(c.legacy.pixelTransfer.indexScale == 2u, "INDEX_SHIFT lưu vào indexScale");

    // --- MAP_COLOR (boolean) ---
    glPixelTransferf(kMapColor, 1.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "MAP_COLOR hợp lệ ⇒ không lỗi");
    TGLMT_CHECK(c.legacy.pixelTransfer.mapColor, "MAP_COLOR = TRUE");

    // --- pname sai ⇒ INVALID_ENUM ---
    DrainErrors();
    glPixelTransferf(kBadPname, 1.0f);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glPixelTransferf pname sai ⇒ INVALID_ENUM");
    DrainErrors();
    glPixelTransferi(kBadPname, 1);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glPixelTransferi pname sai ⇒ INVALID_ENUM");

    // --- pname kiểu integer nhận float không nguyên ⇒ INVALID_VALUE ---
    DrainErrors();
    glPixelTransferf(kIndexOffset, 2.5f);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glPixelTransferf(INDEX_OFFSET, 2.5) ⇒ INVALID_VALUE");
    // Lệnh bị từ chối ⇒ state KHÔNG đổi.
    TGLMT_CHECK(c.legacy.pixelTransfer.indexOffset == 4u,
                "lệnh lỗi KHÔNG được thực thi (INDEX_OFFSET giữ nguyên)");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2+3+4. Pixel map — round-trip 3 biến thể, enum sai, size sai
// ---------------------------------------------------------------------------
static void TestPixelMaps() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // --- biến thể float ---
    const GLfloat fv[4] = {0.0f, 0.25f, 0.5f, 1.0f};
    glPixelMapfv(kPixelMapIToR, 4, fv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelMapfv hợp lệ ⇒ không lỗi");
    GLfloat gotF[256] = {0};
    glGetPixelMapfv(kPixelMapIToR, gotF);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetPixelMapfv hợp lệ ⇒ không lỗi");
    bool fOk = true;
    for (int i = 0; i < 4; ++i) fOk = fOk && Near(gotF[i], fv[i]);
    TGLMT_CHECK(fOk, "round-trip pixel map biến thể float (4 phần tử đầu)");
    // Map lưu trong state (LegacyState::pixelMaps).
    bool stOk = true;
    for (int i = 0; i < 4; ++i) stOk = stOk && Near(c.legacy.pixelMaps.iToR[i], fv[i]);
    TGLMT_CHECK(stOk, "state pixelMaps.iToR khớp giá trị đã set");

    // --- biến thể unsigned int ---
    const GLuint uiv[4] = {0u, 65535u, 100000u, 4294967295u};
    glPixelMapuiv(kPixelMapIToR, 4, uiv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelMapuiv hợp lệ ⇒ không lỗi");
    GLuint gotU[256] = {0};
    glGetPixelMapuiv(kPixelMapIToR, gotU);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetPixelMapuiv hợp lệ ⇒ không lỗi");
    bool uOk = true;
    for (int i = 0; i < 4; ++i) uOk = uOk && (gotU[i] == uiv[i]);
    TGLMT_CHECK(uOk, "round-trip pixel map biến thể unsigned int");

    // --- biến thể unsigned short ---
    const GLushort usv[4] = {0u, 1u, 32768u, 65535u};
    glPixelMapusv(kPixelMapIToR, 4, usv);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelMapusv hợp lệ ⇒ không lỗi");
    GLushort gotS[256] = {0};
    glGetPixelMapusv(kPixelMapIToR, gotS);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glGetPixelMapusv hợp lệ ⇒ không lỗi");
    bool sOk = true;
    for (int i = 0; i < 4; ++i) sOk = sOk && (gotS[i] == usv[i]);
    TGLMT_CHECK(sOk, "round-trip pixel map biến thể unsigned short");

    // --- map enum sai ⇒ INVALID_ENUM (set và get) ---
    DrainErrors();
    glPixelMapfv(kBadPname, 4, fv);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glPixelMapfv map sai ⇒ INVALID_ENUM");
    DrainErrors();
    glPixelMapuiv(kBadPname, 4, uiv);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glPixelMapuiv map sai ⇒ INVALID_ENUM");
    DrainErrors();
    glPixelMapusv(kBadPname, 4, usv);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glPixelMapusv map sai ⇒ INVALID_ENUM");
    DrainErrors();
    glGetPixelMapfv(kBadPname, gotF);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetPixelMapfv map sai ⇒ INVALID_ENUM");
    DrainErrors();
    glGetPixelMapuiv(kBadPname, gotU);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetPixelMapuiv map sai ⇒ INVALID_ENUM");
    DrainErrors();
    glGetPixelMapusv(kBadPname, gotS);
    TGLMT_CHECK(glGetError() == GL_INVALID_ENUM, "glGetPixelMapusv map sai ⇒ INVALID_ENUM");

    // --- size > GL_MAX_PIXEL_MAP_TABLE ⇒ INVALID_VALUE ---
    DrainErrors();
    glPixelMapfv(kPixelMapIToR, kMaxPixelMapTable + 1, fv);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "size > MAX_PIXEL_MAP_TABLE ⇒ INVALID_VALUE");
    DrainErrors();
    glPixelMapfv(kPixelMapIToR, 0, fv);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "size < 1 ⇒ INVALID_VALUE");
    DrainErrors();
    glPixelMapuiv(kPixelMapIToR, kMaxPixelMapTable + 1, uiv);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glPixelMapuiv size > MAX ⇒ INVALID_VALUE");

    // --- NULL values ⇒ INVALID_VALUE ---
    DrainErrors();
    glPixelMapfv(kPixelMapIToR, 4, nullptr);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "values NULL ⇒ INVALID_VALUE");

    // --- Bản đồ địa chỉ bằng index (I_TO_I) phải có size = 2^n ---
    DrainErrors();
    const GLfloat one[3] = {0.0f, 0.0f, 0.0f};
    glPixelMapfv(kPixelMapIToI, 3, one);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "map địa chỉ bằng index cần size = 2^n ⇒ INVALID_VALUE");
    DrainErrors();
    glPixelMapfv(kPixelMapIToI, 4, one);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "size = 2^n ⇒ hợp lệ");

    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. Pixel zoom
// ---------------------------------------------------------------------------
static void TestPixelZoom() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    float zx = -1.0f, zy = -1.0f;
    tglmt::gl::LegacyPixelZoomState(c, &zx, &zy);
    TGLMT_CHECK(Near(zx, 1.0f) && Near(zy, 1.0f), "pixel zoom mặc định = (1,1)");

    glPixelZoom(2.0f, 3.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glPixelZoom ⇒ không lỗi");
    zx = -1.0f;
    zy = -1.0f;
    tglmt::gl::LegacyPixelZoomState(c, &zx, &zy);
    TGLMT_CHECK(Near(zx, 2.0f) && Near(zy, 3.0f), "glPixelZoom(2,3) ⇒ state = (2,3)");

    tglmtDestroyContext(ctx);
}

int main() {
    TestPixelTransfer();
    TestPixelMaps();
    TestPixelZoom();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_pixelops: OK\n");
        return 0;
    }
    return 1;
}
