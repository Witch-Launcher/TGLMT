#pragma once
// GLConvert.h — hàm chuyển đổi tọa độ GL→Metal thuần CPU (không cần GPU, unit-test được).
// Căn cứ: glspec46.core.pdf §13 (viewport, GL origin bottom-left, depth [-1,1])
// + Metal (origin top-left, depth [0,1]) + glClipControl.
#include "tglmt/MetalInterface.h"

namespace tglmt {

// glViewport(x,y,w,h) + glDepthRange(n,f) → MTLViewport.
// Quy ước memory render target = GL (hàng 0 = đáy GL, texcoord v=0) để GPU
// sample glReadPixels/glCopyTex/glBlit khớp GL. Metal NDC y=+1 → hàng memory 0
// (top-down) nên GL cần viewport h<0 (đã verify: Metal chấp nhận negative
// viewport + GPU validation OK; winding visually lật theo → caller đổi frontFace).
// clipOriginUpperLeft=true khi glClipControl(GL_CLIP_ORIGIN... = UPPER_LEFT);
// zeroToOneDepth=true khi glClipControl depth = ZERO_TO_ONE (GL 4.5+).
// Depth n/f: VS converter ĐÃ bake kZConv (GL clip z [-w,w] → Metal NDC [0,w]) nên
// z ra rasterizer = GL window depth chuẩn hóa [0,1]. glDepthRange(n,f) trong GL:
// window_z = n + z_norm*(f−n) — Metal viewport znear/zfar có đúng công thức đó →
// passthrough n/f (KHÔNG transform 0.5 thêm lần nữa, trước đây double-convert làm
// raster z nén vào [0.5,1] lệch với glClearDepth + gl_FragCoord.z).
inline metal::Viewport GLViewportToMetal(float x, float y, float w, float h,
        double n, double f, float targetHeight,
        bool clipOriginUpperLeft = false, bool zeroToOneDepth = false) {
    (void)targetHeight; // không còn flip theo targetHeight (memory = GL-order)
    (void)zeroToOneDepth; // kZConv xử lý ở VS converter (ZERO_TO_ONE là known-limitation)
    metal::Viewport vp;
    vp.x = x;
    if (clipOriginUpperLeft) {
        vp.y = (double)y;   // UPPER_LEFT: GL y-up window ↔ Metal y-down trùng dấu
        vp.h = (double)h;
    } else {
        vp.y = (double)y + (double)h; // NDC y=-1 → hàng memory y (đáy GL)
        vp.h = -(double)h;            // negative viewport height = lật y
    }
    vp.w = w;
    vp.n = n; // glDepthRange n/f passthrough (shader đã convert z → [0,1]) — xem trên
    vp.f = f;
    return vp;
}

// glScissor(x,y,w,h) GL coords → Metal ScissorRect.
// Memory = GL-order (đo thực tế trên GPU: NDC y=+1 → memory row cuối = GL row
// trên cùng; readback getBytes không flip) và viewport/scissor cùng không
// gian fragment → identity (chỉ clamp âm) là ĐÚNG. ĐÃ THỬ flip (metal_y=H-y-h)
// + test orientation: kết quả cho thấy mapping hiện tại đúng, flip sẽ làm
// lệch scissor. Giữ identity.
// clipOriginUpperLeft giữ tham số cho tương lai (MC luôn LOWER_LEFT).
inline metal::ScissorRect GLScissorToMetal(int x, int y, int w, int h, int targetHeight,
        bool clipOriginUpperLeft = false) {
    (void)targetHeight;
    (void)clipOriginUpperLeft;
    metal::ScissorRect r;
    r.x = (uint32_t)(x < 0 ? 0 : x);
    r.y = (uint32_t)(y < 0 ? 0 : y);
    r.w = (uint32_t)(w < 0 ? 0 : w);
    r.h = (uint32_t)(h < 0 ? 0 : h);
    return r;
}

} // namespace tglmt
