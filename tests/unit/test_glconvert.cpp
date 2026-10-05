// test_glconvert.cpp — CPU-only: GL→Metal viewport/scissor conversion (GLConvert.h).
// Quy ước: memory render target = GL-order (hàng 0 = đáy GL) → viewport lower-left
// = negative height (NDC y=-1 → hàng memory y); scissor = identity.
#include "tglmt/GLConvert.h"
#include <cassert>
#include <cstdio>
int main() {
    // glViewport(10,20,100,50) trên target cao 200, depth mặc định GL (0,1)
    auto vp = tglmt::GLViewportToMetal(10, 20, 100, 50, -1.0, 1.0, 200.0f);
    assert(vp.x == 10 && vp.w == 100);
    assert(vp.y == 20 + 50); // GL-correct: NDC y=-1 → hàng 20 (đáy viewport GL)
    assert(vp.h == -50);     // negative height = lật y sang GL-order memory
    assert(vp.n == 0.0 && vp.f == 1.0); // [-1,1] → [0,1]
    // ZERO_TO_ONE: giữ nguyên dấu
    auto vp2 = tglmt::GLViewportToMetal(0, 0, 64, 64, 0.0, 1.0, 64.0f, false, true);
    assert(vp2.y == 64 && vp2.h == -64);
    assert(vp2.n == 0.0 && vp2.f == 1.0);
    // UPPER_LEFT: không lật (window y-up ↔ fb row-topdown trùng)
    auto vp3 = tglmt::GLViewportToMetal(10, 20, 100, 50, 0.0, 1.0, 200.0f, true, true);
    assert(vp3.y == 20 && vp3.h == 50);
    // scissor: memory = GL-order → identity (chỉ clamp âm)
    auto sc = tglmt::GLScissorToMetal(10, 20, 100, 50, 200);
    assert(sc.x == 10 && sc.y == 20 && sc.w == 100 && sc.h == 50);
    auto scNeg = tglmt::GLScissorToMetal(-5, -3, 10, 10, 200);
    assert(scNeg.x == 0 && scNeg.y == 0);
    printf("test_glconvert PASS\n");
    return 0;
}
