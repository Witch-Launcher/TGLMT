// test_viewport_array.cpp — Unit: ARB_viewport_array / OES_viewport_array (GL 4.6
// §13.8.1 viewport + §14.9.2 scissor) — mỗi viewport index có viewport và scissor
// box RIÊNG. Trước đây glScissorArrayv/glScissorIndexed chỉ ghi vào box của
// viewport 0 (và bỏ qua tham số `first`), nên viewport 1..N bị cắt nhầm.
// Spec: docs/khronos/extensions/ARB/ARB_viewport_array.txt + glspec46.core.pdf.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_viewport_array FAIL: %s\n", msg); ++gFails; }
}

static const GLenum SCISSOR_BOX = 0x0C10; // GL_SCISSOR_BOX

static bool BoxEq(const StateTracker::ScissorBox& b, int x, int y, int w, int h) {
    return b.x == x && b.y == y && b.w == w && b.h == h;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // --- glScissorArrayv(first, count, v): mỗi viewport một box ---
    // Box của viewport 0 trước đó: glScissor đặt cho MỌI viewport (spec §14.9.2).
    glScissor(1, 2, 3, 4);
    Check(glGetError() == GL_NO_ERROR, "glScissor hợp lệ");
    Check(BoxEq(ctx.state.GetScissor(0), 1, 2, 3, 4), "glScissor đặt box viewport 0");

    const GLint boxes[8] = {10, 20, 30, 40,   // viewport 1
                            50, 60, 70, 80};  // viewport 2
    glScissorArrayv(1, 2, boxes);
    Check(glGetError() == GL_NO_ERROR, "glScissorArrayv hợp lệ");
    Check(BoxEq(ctx.state.GetScissor(1), 10, 20, 30, 40), "ScissorArrayv ghi box viewport 1");
    Check(BoxEq(ctx.state.GetScissor(2), 50, 60, 70, 80), "ScissorArrayv ghi box viewport 2");
    Check(BoxEq(ctx.state.GetScissor(0), 1, 2, 3, 4), "ScissorArrayv không đụng viewport 0");

    // --- glScissorIndexed: đúng index ---
    glScissorIndexed(3, 5, 6, 7, 8);
    Check(glGetError() == GL_NO_ERROR, "glScissorIndexed hợp lệ");
    Check(BoxEq(ctx.state.GetScissor(3), 5, 6, 7, 8), "ScissorIndexed ghi box viewport 3");
    Check(BoxEq(ctx.state.GetScissor(1), 10, 20, 30, 40), "ScissorIndexed không đụng viewport 1");

    const GLint v3[4] = {90, 91, 92, 93};
    glScissorIndexedv(4, v3);
    Check(glGetError() == GL_NO_ERROR, "glScissorIndexedv hợp lệ");
    Check(BoxEq(ctx.state.GetScissor(4), 90, 91, 92, 93), "ScissorIndexedv ghi box viewport 4");

    // --- glScissor (legacy) đặt box cho MỌI viewport ---
    glScissor(0, 0, 64, 64);
    Check(BoxEq(ctx.state.GetScissor(0), 0, 0, 64, 64), "glScissor box viewport 0");
    Check(BoxEq(ctx.state.GetScissor(2), 0, 0, 64, 64), "glScissor áp cho mọi viewport");

    // --- Query: GL_SCISSOR_BOX của index 0 (glGetIntegerv) ---
    GLint box0[4] = {-1, -1, -1, -1};
    glGetIntegerv(SCISSOR_BOX, box0);
    Check(box0[0] == 0 && box0[1] == 0 && box0[2] == 64 && box0[3] == 64,
          "glGetIntegerv(GL_SCISSOR_BOX) = box viewport 0");

    // --- Query indexed: glGetIntegeri_v(GL_SCISSOR_BOX, i) ---
    // LƯU Ý: gl_get.cpp (OutI) chỉ chuyển tiếp 4 byte đầu của shadow nên GL trả
    // được phần tử đầu (x) của box theo index; 3 phần tử còn lại phải đọc qua
    // state.GetScissor(i) (StateTracker giữ nguồn sự thật).
    glScissorArrayv(1, 2, boxes);   // lại đặt box 1 và 2
    GLint qi[4] = {-1, -1, -1, -1};
    glGetIntegeri_v(SCISSOR_BOX, 1, qi);
    Check(qi[0] == 10, "glGetIntegeri_v(GL_SCISSOR_BOX, 1) → x của viewport 1");
    Check(glGetError() == GL_NO_ERROR, "glGetIntegeri_v(SCISSOR_BOX, 1) không lỗi");
    GLint qi2[4] = {-1, -1, -1, -1};
    glGetIntegeri_v(SCISSOR_BOX, 2, qi2);
    Check(qi2[0] == 50, "glGetIntegeri_v(GL_SCISSOR_BOX, 2) → x của viewport 2");
    GLint qi0[4] = {-1, -1, -1, -1};
    glGetIntegeri_v(SCISSOR_BOX, 0, qi0);
    Check(qi0[0] == 0, "glGetIntegeri_v(GL_SCISSOR_BOX, 0) → x của viewport 0");

    // --- Lỗi theo spec ---
    glScissorArrayv(15, 4, boxes);   // first+count > MAX_VIEWPORTS
    Check(glGetError() == GL_INVALID_VALUE, "ScissorArrayv vượt MAX_VIEWPORTS → INVALID_VALUE");
    glScissorArrayv(0, -1, boxes);    // count < 0
    Check(glGetError() == GL_INVALID_VALUE, "ScissorArrayv count < 0 → INVALID_VALUE");
    glScissorIndexed(99, 0, 0, 4, 4); // index >= MAX_VIEWPORTS
    Check(glGetError() == GL_INVALID_VALUE, "ScissorIndexed index vượt MAX_VIEWPORTS → INVALID_VALUE");
    glScissorIndexed(2, 0, 0, -4, 4);  // width < 0
    Check(glGetError() == GL_INVALID_VALUE, "ScissorIndexed width < 0 → INVALID_VALUE");
    glScissor(0, 0, -1, 4);
    Check(glGetError() == GL_INVALID_VALUE, "glScissor width < 0 → INVALID_VALUE");

    // --- Viewport array: mỗi index một viewport (đã xử lý đúng, giữ nguyên) ---
    const GLfloat vps[8] = {0, 0, 16, 16,   // viewport 1
                            32, 32, 8, 8};  // viewport 2
    glViewportArrayv(1, 2, vps);
    Check(glGetError() == GL_NO_ERROR, "glViewportArrayv hợp lệ");
    auto vp1 = ctx.state.GetViewport(1);
    auto vp2 = ctx.state.GetViewport(2);
    Check(vp1.x == 0 && vp1.y == 0 && vp1.w == 16 && vp1.h == 16, "ViewportArrayv viewport 1");
    Check(vp2.x == 32 && vp2.y == 32 && vp2.w == 8 && vp2.h == 8, "ViewportArrayv viewport 2");

    // --- Draw path dùng viewport index hiện tại (Metal chỉ 1 viewport/pass) ---
    // GL không có state CPU cho viewport index (đến từ gl_ViewportIndex của GS);
    // TGLMT để index 0, draw path đọc state.GetScissor(index) tương ứng.
    Check(ctx.state.CurrentViewportIndex() == 0, "viewport index mặc định = 0");
    ctx.state.SetCurrentViewportIndex(2);
    Check(ctx.state.CurrentViewportIndex() == 2, "viewport index hiện tại đổi được");
    Check(BoxEq(ctx.state.GetScissor((int)ctx.state.CurrentViewportIndex()), 50, 60, 70, 80),
          "draw path đọc scissor box theo viewport index hiện tại");
    ctx.state.SetCurrentViewportIndex(0);

    return gFails ? 1 : 0;
}
