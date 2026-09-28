#pragma once
// GLConvert.h — hàm chuyển đổi tọa độ GL→Metal thuần CPU (không cần GPU, unit-test được).
// Căn cứ: glspec46.core.pdf §13 (viewport, GL origin bottom-left, depth [-1,1])
// + Metal (origin top-left, depth [0,1]) + glClipControl.
#include "tglmt/MetalInterface.h"

namespace tglmt {

// glViewport(x,y,w,h) + glDepthRange(n,f) → MTLViewport.
// targetHeight: chiều cao render target (để flip-y). clipOriginUpperLeft=true khi
// glClipControl(GL_CLIP_ORIGIN... = UPPER_LEFT); zeroToOneDepth=true khi
// glClipControl depth = ZERO_TO_ONE (GL 4.5+), ngược lại map [-1,1]→[0,1].
inline metal::Viewport GLViewportToMetal(float x, float y, float w, float h,
        double n, double f, float targetHeight,
        bool clipOriginUpperLeft = false, bool zeroToOneDepth = false) {
    metal::Viewport vp;
    vp.x = x;
    vp.y = clipOriginUpperLeft ? (double)y : (double)(targetHeight - (y + h));
    vp.w = w;
    vp.h = h;
    if (zeroToOneDepth) {
        vp.n = n;
        vp.f = f;
    } else {
        vp.n = n * 0.5 + 0.5; // GL NDC z [-1,1] → Metal [0,1] (làm trong shader/projection;
        vp.f = f * 0.5 + 0.5; // viewport depth ở đây chỉ ghi nhận, driver Metal dùng trực tiếp)
    }
    return vp;
}

// glScissor(x,y,w,h) GL coords → Metal ScissorRect (flip-y như viewport).
inline metal::ScissorRect GLScissorToMetal(int x, int y, int w, int h, int targetHeight,
        bool clipOriginUpperLeft = false) {
    metal::ScissorRect r;
    r.x = (uint32_t)(x < 0 ? 0 : x);
    r.y = (uint32_t)(clipOriginUpperLeft ? (y < 0 ? 0 : y)
                                         : (targetHeight - (y + h) < 0 ? 0 : targetHeight - (y + h)));
    r.w = (uint32_t)(w < 0 ? 0 : w);
    r.h = (uint32_t)(h < 0 ? 0 : h);
    return r;
}

} // namespace tglmt
