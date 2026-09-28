#include <metal_stdlib>
using namespace metal;
// glLogicOp composite (glspec46 §17.3.9 Table 17.3): Metal không có ROP phần cứng,
// giả lập bằng compute pass đọc src (màu fragment đã render) + dst (framebuffer),
// áp toán tử bitwise từng component, ghi lại dst. Chỉ đúng cho target integer
// (spec: no effect trên float/sRGB) — kernel dùng RGBA8Uint.
kernel void rop(texture2d<uint, access::read> src [[texture(0)]],
                texture2d<uint, access::read> dst [[texture(1)]],
                constant uint &op [[buffer(0)]],
                device uchar *out [[buffer(1)]],
                uint2 gid [[thread_position_in_grid]]) {
  if (gid.x >= dst.get_width() || gid.y >= dst.get_height()) return;
  uint4 s = src.read(gid);
  uint4 d = dst.read(gid);
  uint4 o = d;
  switch (op) {
    case 0x1500: o = uint4(0); break;                          // CLEAR 0
    case 0x1501: o = s & d; break;                             // AND s∧d
    case 0x1502: o = s & (~d); break;                          // AND_REVERSE s∧¬d
    case 0x1503: o = s; break;                                 // COPY s
    case 0x1504: o = (~s) & d; break;                          // AND_INVERTED ¬s∧d
    case 0x1505: o = d; break;                                 // NOOP d
    case 0x1506: o = s ^ d; break;                             // XOR s xor d
    case 0x1507: o = s | d; break;                             // OR s∨d
    case 0x1508: o = ~(s | d); break;                          // NOR ¬(s∨d)
    case 0x1509: o = ~(s ^ d); break;                          // EQUIV ¬(s xor d)
    case 0x150A: o = ~d; break;                                // INVERT ¬d
    case 0x150B: o = s | (~d); break;                          // OR_REVERSE s∨¬d
    case 0x150C: o = ~s; break;                                // COPY_INVERTED ¬s
    case 0x150D: o = (~s) | d; break;                          // OR_INVERTED ¬s∨d
    case 0x150E: o = ~(s & d); break;                          // NAND ¬(s∧d)
    case 0x150F: o = uint4(0xFFFFFFFFu); break;                // SET all 1's
    default: break;
  }
  // Ghi ra buffer theo byte (đọc được từ CPU qua contents()) thay vì texture.
  uint idx = (gid.y * dst.get_width() + gid.x) * 4;
  out[idx + 0] = (uchar)(o.x & 0xFFu); out[idx + 1] = (uchar)(o.y & 0xFFu);
  out[idx + 2] = (uchar)(o.z & 0xFFu); out[idx + 3] = (uchar)(o.w & 0xFFu);
}
