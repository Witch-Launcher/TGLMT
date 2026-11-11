// test_internalformat_query.cpp — Unit: ARB_internalformat_query +
// ARB_internalformat_query2 (glspec46.core.pdf §22.3) cho glGetInternalformativ.
// Trước đây: target bị bỏ qua, NUM_SAMPLE_COUNTS ghi 1 vào MỌI phần tử, các pname
// kích thước component dùng SAI enum (0x8274 bị gọi là RED_SIZE trong khi gl.xml
// gọi 0x8271 là RED_SIZE) → trả sai số.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_internalformat_query FAIL: %s\n", msg); ++gFails; }
}

// pname (đối chiếu gl.xml group InternalFormatPName)
static const GLenum P_SUPPORTED   = 0x826F; // GL_INTERNALFORMAT_SUPPORTED
static const GLenum P_PREFERRED   = 0x8270; // GL_INTERNALFORMAT_PREFERRED
static const GLenum P_RED_SIZE    = 0x8271; // GL_INTERNALFORMAT_RED_SIZE
static const GLenum P_GREEN_SIZE  = 0x8272; // GL_INTERNALFORMAT_GREEN_SIZE
static const GLenum P_BLUE_SIZE   = 0x8273; // GL_INTERNALFORMAT_BLUE_SIZE
static const GLenum P_ALPHA_SIZE  = 0x8274; // GL_INTERNALFORMAT_ALPHA_SIZE
static const GLenum P_DEPTH_SIZE  = 0x8275; // GL_INTERNALFORMAT_DEPTH_SIZE
static const GLenum P_STENCIL_SZ  = 0x8276; // GL_INTERNALFORMAT_STENCIL_SIZE
static const GLenum P_COLOR_REND  = 0x8286; // GL_COLOR_RENDERABLE
static const GLenum P_DEPTH_REND  = 0x8287; // GL_DEPTH_RENDERABLE
static const GLenum P_STENCIL_RND = 0x8288; // GL_STENCIL_RENDERABLE
static const GLenum P_FB_REND     = 0x8289; // GL_FRAMEBUFFER_RENDERABLE
static const GLenum P_SAMPLES     = 0x80A9; // GL_SAMPLES
static const GLenum P_NUM_SAMPLES = 0x9380; // GL_NUM_SAMPLE_COUNTS

static GLint Query(GLenum target, GLenum fmt, GLenum pname, GLsizei n = 1) {
    GLint v = -1;
    glGetInternalformativ(target, fmt, pname, n, &v);
    return v;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // --- Target hợp lệ / format hỗ trợ ---
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_SUPPORTED) == 1, "RGBA8 supported");
    Check(Query(GL_RENDERBUFFER, 0x1234 /*format lạ*/, P_SUPPORTED) == 0,
          "format lạ → INTERNALFORMAT_SUPPORTED = 0 (không lỗi, core §22.3)");
    Check(glGetError() == GL_NO_ERROR, "format lạ không sinh lỗi");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_PREFERRED) == (GLint)GL_RGBA8,
          "INTERNALFORMAT_PREFERRED = chính format");

    // --- Kích thước component (enum đúng theo gl.xml) ---
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_RED_SIZE) == 8, "RED_SIZE(RGBA8) = 8");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_GREEN_SIZE) == 8, "GREEN_SIZE(RGBA8) = 8");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_BLUE_SIZE) == 8, "BLUE_SIZE(RGBA8) = 8");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_ALPHA_SIZE) == 8, "ALPHA_SIZE(RGBA8) = 8");
    Check(Query(GL_RENDERBUFFER, GL_R8, P_RED_SIZE) == 8, "RED_SIZE(R8) = 8");
    Check(Query(GL_RENDERBUFFER, GL_R8, P_GREEN_SIZE) == 0, "GREEN_SIZE(R8) = 0");
    Check(Query(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, P_DEPTH_SIZE) == 24, "DEPTH_SIZE = 24");
    Check(Query(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, P_STENCIL_SZ) == 8, "STENCIL_SIZE = 8");

    // --- Renderable ---
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_COLOR_REND) == 1, "COLOR_RENDERABLE(RGBA8) = 1");
    Check(Query(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, P_COLOR_REND) == 0,
          "COLOR_RENDERABLE(depth) = 0");
    Check(Query(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, P_DEPTH_REND) == 1,
          "DEPTH_RENDERABLE(DEPTH24_STENCIL8) = 1");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_DEPTH_REND) == 0, "DEPTH_RENDERABLE(RGBA8) = 0");
    Check(Query(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, P_STENCIL_RND) == 1,
          "STENCIL_RENDERABLE(DEPTH24_STENCIL8) = 1");
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_FB_REND) == 2 /*FULL_SUPPORT*/,
          "FRAMEBUFFER_RENDERABLE(RGBA8) = FULL_SUPPORT");

    // --- Sample counts: phân biệt theo target ---
    Check(Query(GL_RENDERBUFFER, GL_RGBA8, P_NUM_SAMPLES) == 2,
          "NUM_SAMPLE_COUNTS(RENDERBUFFER) = 2");
    Check(Query(GL_TEXTURE_2D_MULTISAMPLE, GL_RGBA8, P_NUM_SAMPLES) == 2,
          "NUM_SAMPLE_COUNTS(TEXTURE_2D_MULTISAMPLE) = 2");
    Check(Query(GL_TEXTURE_2D, GL_RGBA8, P_NUM_SAMPLES) == 0,
          "NUM_SAMPLE_COUNTS(TEXTURE_2D) = 0 (target không hỗ trợ multisample)");
    Check(Query(GL_TEXTURE_2D, GL_RGBA8, P_NUM_SAMPLES, 2) == 0,
          "NUM_SAMPLE_COUNTS chỉ ghi 1 giá trị");
    // SAMPLES: danh sách sample count giảm dần (spec §22.3)
    GLint samples[2] = {0, 0};
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, P_SAMPLES, 2, samples);
    Check(glGetError() == GL_NO_ERROR, "glGetInternalformativ(SAMPLES) không lỗi");
    Check(samples[0] == 4 && samples[1] == 1, "SAMPLES(RGBA8) = {4,1} giảm dần");
    GLint one[1] = {0};
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, P_SAMPLES, 1, one);
    Check(one[0] == 4, "SAMPLES với bufSize=1 → max sample count");
    GLint none[2] = {7, 7};
    glGetInternalformativ(GL_TEXTURE_2D, GL_RGBA8, P_SAMPLES, 2, none);
    Check(none[0] == 7 && none[1] == 7, "SAMPLES target không hỗ trợ → không ghi gì");

    // --- Pname scalar chỉ ghi đúng 1 giá trị (không đổ params còn lại) ---
    GLint box[4] = {-1, -1, -1, -1};
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, P_RED_SIZE, 4, box);
    Check(box[0] == 8, "RED_SIZE ghi params[0]");
    Check(box[1] == -1 && box[2] == -1 && box[3] == -1, "pname scalar không ghi thừa params");

    // --- Lỗi theo spec ---
    glGetInternalformativ(0x1234 /*target sai*/, GL_RGBA8, P_SUPPORTED, 1, box);
    Check(glGetError() == GL_INVALID_ENUM, "target sai → INVALID_ENUM");
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, 0x1234 /*pname sai*/, 1, box);
    Check(glGetError() == GL_INVALID_ENUM, "pname sai → INVALID_ENUM");
    glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, P_SUPPORTED, -1, box);
    Check(glGetError() == GL_INVALID_VALUE, "count < 0 → INVALID_VALUE");

    return gFails ? 1 : 0;
}
