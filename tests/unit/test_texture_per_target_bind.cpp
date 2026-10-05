// test_texture_per_target_bind.cpp — Unit: per-target binding slots (Fix #5).
// Mô phỏng chuỗi bind thật của MC 26.1.2: _bindTexture 2D (guarded, skip khi
// cache đã đúng) xen direct glBindTexture cube/buffer (GlDevice cube create,
// GlCommandEncoder writeToTexture UTB bypass GlStateManager cache).
// Bug cũ: StateTracker gộp 1 slot/unit → cube/buffer PHÁ binding 2D của unit →
// MC guard skip → TGLMT resolve sai texture (sampdeny fallback đen = mất chữ/
// model chớp; upload BoundTex 0x0502 = MẤT glyph/sprite).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static int gFails = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("  FAIL line %d: %s\n", __LINE__, #cond);               \
            ++gFails;                                                      \
        }                                                                  \
    } while (0)

int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt::gl;
    tglmt::GLuint t2d, tcube, tbuf;
    glGenTextures(1, &t2d);
    glGenTextures(1, &tcube);
    glGenTextures(1, &tbuf);

    glActiveTexture(0x84C0); // GL_TEXTURE0

    // 1. 2D bind bình thường.
    glBindTexture(0x0DE1, t2d);
    CHECK(ctx.state.BoundTexture(0) == t2d);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == t2d);

    // 2. Cube bind (direct path như GlDevice.createTexture) — KHÔNG phá slot 2D.
    glBindTexture(0x8513, tcube);
    CHECK(ctx.state.BoundTexture(0, 0x8513) == tcube);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == t2d); // bug cũ: == tcube
    CHECK(ctx.state.BoundTexture(0) == t2d);          // compat getter = slot 2D

    // 3. Buffer bind (UTB CloudFaces unit0 mỗi frame) — KHÔNG phá slot 2D.
    glBindTexture(0x8C2A, tbuf);
    CHECK(ctx.state.BoundTexture(0, 0x8C2A) == tbuf);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == t2d); // bug cũ: == tbuf
    CHECK(ctx.state.BoundTexture(0) == t2d);

    // 4. Guarded skip của MC: _bindTexture(t2d) thấy cache đã == t2d → skip
    //    lệnh GL thật. TGLMT vẫn phải thấy t2d ở slot 2D (chuỗi này từng làm
    //    shadow/GUI sample cube/buffer → fallback đen).
    glBindTexture(0x0DE1, t2d);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == t2d);

    // 5. Upload sau chuỗi cube/buffer: BoundTex per-target → ghi vào t2d,
    //    không 0x0502 (bug cũ: resolve cube id → mismatch → upload MẤT).
    glPixelStorei(0x0CF5, 1); // UNPACK_ALIGNMENT=1
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
    unsigned char px[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glTexSubImage2D(0x0DE1, 0, 0, 0, 4, 1, 0x1908, 0x1401, px);
    CHECK(glGetError() == 0);
    CHECK(ctx.textures[t2d].pixels.size() == 64);
    CHECK(ctx.textures[t2d].pixels[0] == 1 && ctx.textures[t2d].pixels[15] == 16);

    // 6. Face cubemap target → slot cube.
    CHECK(ctx.state.BoundTexture(0, 0x8515) == tcube);

    // 7. glDeleteTextures unbind mọi unit/slot (spec §8.1); slot khác giữ nguyên.
    glDeleteTextures(1, &t2d);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == 0);
    CHECK(ctx.state.BoundTexture(0, 0x8513) == tcube);
    CHECK(ctx.state.BoundTexture(0, 0x8C2A) == tbuf);
    CHECK(glGetError() == 0);

    // 8. Heal: bind id chưa có trong registry (divergence MC cache/TGLMT từng
    //    reject → lệch vĩnh viễn) → tạo object rỗng + bind thật, không lỗi.
    glBindTexture(0x0DE1, 99999);
    CHECK(ctx.textures.count(99999) == 1);
    CHECK(ctx.state.BoundTexture(0, 0x0DE1) == 99999);
    CHECK(glGetError() == 0);

    // 9. AnyBoundAtUnit (hazard check) theo mọi target slot.
    CHECK(ctx.state.AnyBoundAtUnit(0, tcube));
    CHECK(ctx.state.AnyBoundAtUnit(0, tbuf));
    CHECK(!ctx.state.AnyBoundAtUnit(0, t2d)); // đã delete/unbind
    CHECK(!ctx.state.AnyBoundAtUnit(1, tcube));

    // 10. DSA glBindTextureUnit: binding point theo target CỦA OBJECT.
    tglmt::GLuint t2b;
    glGenTextures(1, &t2b);
    glBindTexture(0x0DE1, t2b); // set object target trước
    glBindTextureUnit(1, t2b);
    CHECK(ctx.state.BoundTexture(1, 0x0DE1) == t2b);
    CHECK(ctx.state.BoundTexture(1, 0x8513) == 0); // slot cube unit1 trống

    // 11. glBindTextures (plural): per-element target từ object.
    tglmt::GLuint pair[2] = {t2b, tcube};
    glBindTextures(2, 2, pair);
    CHECK(ctx.state.BoundTexture(2, 0x0DE1) == t2b);
    CHECK(ctx.state.BoundTexture(3, 0x8513) == tcube);

    // 12. TexSlotOf: face → cube, 2D/buffer/rect/3D đúng slot.
    CHECK(tglmt::StateTracker::TexSlotOf(0x0DE1) == 0);
    CHECK(tglmt::StateTracker::TexSlotOf(0x8513) == 1);
    CHECK(tglmt::StateTracker::TexSlotOf(0x851A) == 1);
    CHECK(tglmt::StateTracker::TexSlotOf(0x8C2A) == 2);
    CHECK(tglmt::StateTracker::TexSlotOf(0x8C1A) == 3);
    CHECK(tglmt::StateTracker::TexSlotOf(0x806F) == 4);
    CHECK(tglmt::StateTracker::TexSlotOf(0x8078) == 5);

    if (gFails) {
        printf("test_texture_per_target_bind FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_texture_per_target_bind PASS\n");
    return 0;
}
