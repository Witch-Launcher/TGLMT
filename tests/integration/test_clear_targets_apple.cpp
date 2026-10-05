// test_clear_targets_apple.cpp — glClear immediate trên FBO đang bind (Fix #2).
//
// Deferred-clear (cũ): glClear chỉ ghi applePendingClear → tiêu thụ bởi DRAW
// KẾ TIẾP bất kể FBO:
//   (a) 2 glClear liên tiếp → clear đầu MẤT (mask gộp, color bị đè);
//   (b) clear fbo A áp lên encoder của draw fbo B (clrmiss# trong log máy) →
//       sky pass clear (sky-blue) bị mất / clear đen áp sai → world đen,
//       sprite/item atlas bị wipe (mất mặt nút GUI).
// Immediate (mới): AppleClearNow → makeClearEncoder NGAY trên fbo đang bind.
//
// Lớp pixel (cần MTL device): đọc lại texture của 2 FBO — clear độc lập không
// đè nhau; clear depth không wipe color; scissor probe (GuiItemAtlas region).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_clear_targets_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

// FBO 32x32 color RGBA8 (data=0) [+ depth DEPTH_COMPONENT32F nếu wantDepth].
static GLuint MakeFBO(bool wantDepth) {
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    std::vector<unsigned char> zeros(32 * 32 * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058, 32, 32, 0, 0x1908, 0x1401, zeros.data());
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9 /*DRAW*/, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
    if (wantDepth) {
        GLuint dep = 0;
        glGenTextures(1, &dep);
        glBindTexture(0x0DE1, dep);
        glTexImage2D(0x0DE1, 0, 0x8CAC /*DEPTH_COMPONENT32F*/, 32, 32, 0, 0x1902, 0x1406,
                     nullptr);
        glFramebufferTexture2D(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x0DE1, dep, 0);
    }
    return fbo;
}

static std::array<unsigned char, 4> ReadPx(GLuint fbo, int x, int y) {
    unsigned char px[4] = {0, 0, 0, 0};
    glBindFramebuffer(0x8CA8 /*READ*/, fbo);
    glReadPixels(x, y, 1, 1, 0x1908, 0x1401, px);
    return {px[0], px[1], px[2], px[3]};
}

static bool Near(unsigned char v, unsigned char expect) {
    return v + 3 >= expect && v <= expect + 3;
}
static bool IsColor(const std::array<unsigned char, 4>& p, unsigned char r, unsigned char g,
                    unsigned char b, unsigned char a = 255) {
    return Near(p[0], r) && Near(p[1], g) && Near(p[2], b) && Near(p[3], a);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    bool hasGPU = !ctx.device->isNull();
    if (!hasGPU) {
        printf("test_clear_targets_apple SKIP: no MTL device\n");
        return 0;
    }

    GLuint fboA = MakeFBO(/*wantDepth=*/true);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "fboA complete");
    GLuint fboB = MakeFBO(false);
    Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "fboB complete");
    Check(glGetError() == 0, "setup gl error");

    // --- 1. Clear liên tiếp 2 FBO khác nhau: không đè nhau, không mất ---
    glBindFramebuffer(0x8CA9, fboA);
    glClearColor(1, 0, 0, 1);
    glClear(0x00004000);
    glBindFramebuffer(0x8CA9, fboB);
    glClearColor(0, 1, 0, 1);
    glClear(0x00004000);

    auto pxA = ReadPx(fboA, 16, 16);
    auto pxB = ReadPx(fboB, 16, 16);
    printf("[clear] A=(%u,%u,%u,%u) B=(%u,%u,%u,%u)\n", pxA[0], pxA[1], pxA[2], pxA[3], pxB[0],
           pxB[1], pxB[2], pxB[3]);
    Check(IsColor(pxA, 255, 0, 0, 255), "A = đỏ (clear A không bị clear B đè/mất)");
    Check(IsColor(pxB, 0, 255, 0, 255), "B = xanh lá (clear B không phụ thuộc draw)");

    // --- 2. Clear depth trên FBO có depth: color giữ nguyên ---
    glBindFramebuffer(0x8CA9, fboA);
    glClearDepth(0.5);
    glClear(0x00000100 /*DEPTH_BUFFER_BIT*/);
    pxA = ReadPx(fboA, 16, 16);
    Check(IsColor(pxA, 255, 0, 0, 255), "depth clear không wipe color");

    // --- 3. 2 clear liên tiếp CÙNG FBO: clear cuối thắng ---
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    glClearColor(1, 1, 0, 1);
    glClear(0x00004000);
    pxA = ReadPx(fboA, 16, 16);
    Check(IsColor(pxA, 255, 255, 0, 255), "clear cuối cùng thắng trên cùng fbo");

    // --- 4. Scissor probe (GuiItemAtlas clear vùng slot): scissor box (8,8,16,16) GL ---
    glClearColor(0, 0, 255 / 255.0f, 1); // xanh dương = màu trong vùng
    glEnable(0x0C11);                     // SCISSOR_TEST
    glScissor(8, 8, 16, 16);
    glClear(0x00004000);
    glDisable(0x0C11);
    auto inPx = ReadPx(fboA, 16, 16);  // trong vùng scissor (GL bottom-left coords)
    auto outPx = ReadPx(fboA, 2, 2);   // ngoài vùng
    printf("[scissor] in=(%u,%u,%u,%u) out=(%u,%u,%u,%u)\n", inPx[0], inPx[1], inPx[2], inPx[3],
           outPx[0], outPx[1], outPx[2], outPx[3]);
    Check(IsColor(inPx, 0, 0, 255, 255), "scissor: pixel TRONG vùng được clear");
    // Metal loadAction có clip theo scissor hay không? GL bắt buộc clip.
    Check(IsColor(outPx, 255, 255, 0, 255), "scissor: pixel NGOÀI vùng giữ màu cũ (GL parity)");

    // --- 5. Deferred state sạch: draw kế tiếp không clear lại lần nữa ---
    Check(!ctx.applePendingClear || ctx.appleClearMask == 0,
          "immediate clear tiêu thụ deferred treo");

    if (gFails) return 1;
    printf("test_clear_targets_apple PASS\n");
    return 0;
}
