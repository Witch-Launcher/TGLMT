// test_mc_copytobuffer_real.cpp — Tái hiện ĐÚNG sequence vanilla 26.1.2
// (không đoán, không tự bịa test):
//
// Provenance (bằng chứng thật, không copy source Mojang — EULA cấm đăng source):
// - MC 26.1.2 piston-meta:
//   https://piston-meta.mojang.com/v1/packages/ee044d53f31fdcbfa31be3684ad321d44c81c3a5/26.1.2.json
// - client.jar SHA1 4e618f09a0c649dde3fdf829df443ce0b8831e65, URL:
//   https://piston-data.mojang.com/v1/objects/4e618f09a0c649dde3fdf829df443ce0b8831e65/client.jar
//   (tải về .tmp/mc26/ trên ổ D, đã gitignore — KHÔNG commit jar/source MC).
// - Crash thật: crash-2026-09-30_20.56.04-client.txt:
//   "IllegalStateException: Couldn't perform copyTobuffer for texture 41:
//    GL error 1282" tại GlCommandEncoder.copyTextureToBuffer:249
//   <- Screenshot.takeScreenshot:72 <- GameRenderer.takeAutoScreenshot:632
//   (vào world >10 sections là chụp auto-screenshot → crash).
// - GL error 1282 = 0x0502 = GL_INVALID_OPERATION.
// - Phương pháp: `javap -p -c -classpath .tmp/mc26/mc26-client.jar <class>`
//   cho GlCommandEncoder / DirectStateAccess$Core|Emulated / GlConst /
//   Screenshot / GameRenderer / CommandEncoder. Chỉ rút ra SEQUENCE GL API
//   (facts để tương thích — không copy code Mojang):
//   * Screenshot.takeScreenshot: w/h từ RenderTarget, getColorTexture (RGBA8),
//     createBuffer(w*h*4), createCommandEncoder,
//     copyTextureToBuffer(tex, buf, offset=0, runnable, mip=0).
//   * GlCommandEncoder.copyTextureToBuffer 9-arg:
//     clearGlErrors()
//     + DirectStateAccess.bindFrameBufferTextures(readFbo, glId, 0, mip, READ=36008)
//     + _glBindBuffer(PIXEL_PACK=35051, pbo)
//     + _pixelStore(PACK_ROW_LENGTH=3330, w)
//     + _readPixels(x=0,y=0,w,h, ext=6408 RGBA, type=5121 UBYTE, offset=0)
//     + queueFencedTask(runnable)
//     + _glFramebufferTexture2D(READ, COLOR0, TEXTURE_2D, 0, mip) // detach
//     + _glBindFramebuffer(READ, 0) + _glBindBuffer(PACK, 0)
//     + _getError() !=0 → throw copyTobuffer.
//   * DirectStateAccess$Emulated.bindFrameBufferTextures:
//     target = (p5==0 ? DRAW : p5); _glBindFramebuffer(target,fbo);
//     _glFramebufferTexture2D(target, COLOR0, TEX2D, color, level);
//     _glFramebufferTexture2D(target, DEPTH, TEX2D, depth, level).
//   * TGLMT ép Emulated path: glGetStringi KHÔNG quảng cáo
//     GL_ARB_direct_state_access + GetProcAddress trả null cho DSA
//     (glNamedFramebufferTexture/glCreateBuffers/...) → LWJGL capability false
//     → DirectStateAccess.create chọn Emulated. Đã đối chiếu
//     src/gl/glfw_shim.cpp GetProcAddress + gl_error_string_debug.cpp.
// - Bug TGLMT (đã fix): glFramebufferTexture2D luôn attach vào BoundDrawFBO,
//   bỏ qua target → attach READ rơi nhầm sang DRAW → READ rỗng → glReadPixels
//   0x0502 (crash) + detach READ nhầm sang DRAW (xóa DRAW → mất nút/blur sau
//   screenshot). Fix: tôn trọng READ vs DRAW vs FRAMEBUFFER.
//
// Test này chạy Null backend (không cần GPU) vì lỗi là logic FBO/error-queue,
// quan sát được trên CPU shadow. Apple GPU path dùng chung attach/error logic.

#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_copytobuffer_real FAIL: %s (glErr=0x%x)\n", msg, glGetError());
        // Xả error queue để case sau không bị dính lỗi cũ.
        while (glGetError() != 0) {}
        ++gFails;
    }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const int W = 32, H = 16;
    const size_t NEED = (size_t)W * H * 4;

    // Dựng texture RGBA8 pattern không đen (đỏ/xanh xen kẽ theo x) để phân biệt.
    std::vector<uint8_t> pattern(NEED);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            uint8_t* p = pattern.data() + ((size_t)y * W + x) * 4;
            p[0] = (uint8_t)(x * 8); p[1] = (uint8_t)(y * 16); p[2] = 64; p[3] = 255;
        }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    glPixelStorei(0x0CF5, 1); // UNPACK_ALIGNMENT=1 cho upload tight
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, pattern.data());
    Check(glGetError() == 0, "teximage");

    GLuint texDraw = 0;
    glGenTextures(1, &texDraw);
    glBindTexture(0x0DE1, texDraw);
    std::vector<uint8_t> blue(NEED, 0);
    for (size_t i = 0; i < (size_t)W * H; ++i) {
        blue[i * 4 + 2] = 255; blue[i * 4 + 3] = 255;
    }
    glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, blue.data());
    Check(glGetError() == 0, "teximage draw");

    // ---- 1. READ/DRAW isolation (Emulated attach đúng target) ----
    GLuint readFbo = 0, drawFbo = 0;
    glGenFramebuffers(1, &readFbo);
    glGenFramebuffers(1, &drawFbo);
    Check(glGetError() == 0, "gen fbo");

    glBindFramebuffer(0x8CA8, readFbo); // READ
    glFramebufferTexture2D(0x8CA8, 0x8CE0, 0x0DE1, tex, 0); // READ←tex
    Check(glGetError() == 0, "attach READ");
    glBindFramebuffer(0x8CA9, drawFbo); // DRAW
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, texDraw, 0); // DRAW←texDraw
    Check(glGetError() == 0, "attach DRAW");

    // Đọc lại mapping nội bộ (không phải GL API, nhưng là oracle trung thực
    // cho isolation — bug cũ: READ rỗng, DRAW bị ghi đè).
    {
        auto rit = ctx.fbos.find(readFbo);
        auto wit = ctx.fbos.find(drawFbo);
        Check(rit != ctx.fbos.end() && wit != ctx.fbos.end(), "fbos exist");
        if (rit != ctx.fbos.end() && wit != ctx.fbos.end()) {
            auto rc = rit->second.colorTex.find(0);
            auto wc = wit->second.colorTex.find(0);
            Check(rc != rit->second.colorTex.end() && rc->second == tex,
                  "READ fbo giữ đúng tex (bug cũ: READ rỗng)");
            Check(wc != wit->second.colorTex.end() && wc->second == texDraw,
                  "DRAW fbo giữ đúng texDraw (bug cũ: bị ghi đè)");
        }
    }

    // ---- 2. Full copyTextureToBuffer vanilla (Screenshot path, PBO offset 0) ----
    GLuint pbo = 0;
    glGenBuffers(1, &pbo);
    glBindBuffer(0x88EB /*PIXEL_PACK*/, pbo);
    std::vector<uint8_t> zero(NEED, 0);
    glBufferData(0x88EB, (GLsizeiptr)NEED, zero.data(), 0x88E8 /*STREAM_READ*/);
    Check(glGetError() == 0, "pbo data");

    // Đúng thứ tự vanilla: bind READ → attach đã làm ở trên, set PACK_ROW_LENGTH
    // rồi readPixels offset 0.
    glBindFramebuffer(0x8CA8, readFbo);
    glPixelStorei(0x0D02 /*PACK_ROW_LENGTH*/, W);
    glPixelStorei(0x0D05 /*PACK_ALIGNMENT*/, 4);
    glReadPixels(0, 0, W, H, 0x1908 /*RGBA*/, 0x1401 /*UBYTE*/, (void*)0);
    {
        GLenum e = glGetError();
        if (e != 0) {
            printf("test_mc_copytobuffer_real FAIL: vanilla readPixels GL error 0x%x "
                   "(want 0; bug cũ 0x502/1282)\n", e);
            ++gFails;
        }
    }
    // PBO phải chứa pattern (không đen) — Null backend copy shadow tight.
    {
        auto bit = ctx.buffers.find(pbo);
        Check(bit != ctx.buffers.end() && bit->second.data.size() >= NEED, "pbo size");
        if (bit != ctx.buffers.end() && bit->second.data.size() >= NEED) {
            const uint8_t* d = bit->second.data.data();
            // Giữa ảnh phải ≈ pattern, không đen.
            const uint8_t* mid = d + ((size_t)(H / 2) * W + W / 2) * 4;
            const uint8_t* exp = pattern.data() + ((size_t)(H / 2) * W + W / 2) * 4;
            printf("pbo mid=(%u,%u,%u,%u) want=(%u,%u,%u,%u)\n",
                   mid[0], mid[1], mid[2], mid[3], exp[0], exp[1], exp[2], exp[3]);
            Check(mid[0] == exp[0] && mid[1] == exp[1] && mid[2] == exp[2],
                  "pbo pixels khớp texture (bug cũ: đen hoặc 1282)");
        }
    }

    // Detach đúng READ (vanilla): DRAW phải còn nguyên (hồi quy mất nút).
    glFramebufferTexture2D(0x8CA8, 0x8CE0, 0x0DE1, 0, 0); // detach READ
    glBindFramebuffer(0x8CA8, 0);
    glBindBuffer(0x88EB, 0);
    Check(glGetError() == 0, "detach");
    {
        auto wit = ctx.fbos.find(drawFbo);
        Check(wit != ctx.fbos.end(), "draw fbo còn sau detach READ");
        if (wit != ctx.fbos.end()) {
            auto wc = wit->second.colorTex.find(0);
            Check(wc != wit->second.colorTex.end() && wc->second == texDraw,
                  "DRAW còn texDraw sau detach READ (bug cũ: DRAW bị xóa → mất nút)");
        }
        auto rit = ctx.fbos.find(readFbo);
        if (rit != ctx.fbos.end()) {
            auto rc = rit->second.colorTex.find(0);
            // Sau detach READ phải là 0 hoặc absent — không được còn tex cũ
            // theo cách làm rò sang DRAW.
            bool detached = (rc == rit->second.colorTex.end() || rc->second == 0);
            Check(detached, "READ đã detach về 0");
        }
    }

    // ---- 3. FRAMEBUFFER target (menu/post dùng 0x8D40) vẫn về DRAW ----
    {
        GLuint fbo3 = 0;
        glGenFramebuffers(1, &fbo3);
        glBindFramebuffer(0x8D40, fbo3); // FRAMEBUFFER → cả READ+DRAW
        glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, tex, 0);
        Check(glGetError() == 0, "attach FRAMEBUFFER");
        auto it = ctx.fbos.find(fbo3);
        Check(it != ctx.fbos.end(), "fbo3 exist");
        if (it != ctx.fbos.end()) {
            auto cc = it->second.colorTex.find(0);
            Check(cc != it->second.colorTex.end() && cc->second == tex,
                  "FRAMEBUFFER attach thấy được");
        }
    }

    // ---- 4. Named path (Core DSA) không hồi quy ----
    {
        GLuint fboN = 0;
        glGenFramebuffers(1, &fboN);
        glNamedFramebufferTexture(fboN, 0x8CE0, tex, 0);
        Check(glGetError() == 0, "named attach");
        auto it = ctx.fbos.find(fboN);
        Check(it != ctx.fbos.end(), "fboN exist");
        if (it != ctx.fbos.end()) {
            auto cc = it->second.colorTex.find(0);
            Check(cc != it->second.colorTex.end() && cc->second == tex,
                  "named attach giữ tex");
        }
    }

    if (gFails) {
        printf("test_mc_copytobuffer_real FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_mc_copytobuffer_real PASS (vanilla 26.1.2 copyTobuffer sequence, "
           "READ/DRAW isolation, PBO offset 0)\n");
    return 0;
}
