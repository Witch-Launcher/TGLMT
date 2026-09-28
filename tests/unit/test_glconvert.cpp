// test_glconvert.cpp — CPU-only: GL→Metal viewport/scissor conversion (GLConvert.h).
#include "tglmt/GLConvert.h"
#include <cassert>
#include <cstdio>
int main() {
    // glViewport(10,20,100,50) trên target cao 200, depth mặc định 0..1 (GL gọi DepthRange(0,1)? mặc định GL là n=0? GL spec: DepthRange mặc định near=0? — thực tế GL mặc định (0,1)? glspec: initial depth range (0,1)? Ta test mapping tuyến tính chung.
    auto vp = tglmt::GLViewportToMetal(10, 20, 100, 50, -1.0, 1.0, 200.0f);
    assert(vp.x == 10 && vp.w == 100 && vp.h == 50);
    assert(vp.y == 200 - (20 + 50)); // flip-y: 130
    assert(vp.n == 0.0 && vp.f == 1.0); // [-1,1] → [0,1]
    // ZERO_TO_ONE: giữ nguyên
    auto vp2 = tglmt::GLViewportToMetal(0, 0, 64, 64, 0.0, 1.0, 64.0f, false, true);
    assert(vp2.y == 0 && vp2.n == 0.0 && vp2.f == 1.0);
    // UPPER_LEFT: không flip
    auto vp3 = tglmt::GLViewportToMetal(10, 20, 100, 50, 0.0, 1.0, 200.0f, true, true);
    assert(vp3.y == 20);
    // scissor flip
    auto sc = tglmt::GLScissorToMetal(10, 20, 100, 50, 200);
    assert(sc.x == 10 && sc.y == 130 && sc.w == 100 && sc.h == 50);
    printf("test_glconvert PASS\n");
    return 0;
}
