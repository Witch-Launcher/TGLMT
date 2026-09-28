#include <metal_stdlib>
using namespace metal;
// TGLMT GS-emulation qua mesh shader (Metal 3+, Apple7+/M1+, iOS 16+).
// Căn cứ MSL spec §2.20.2 (mesh<V,P,NV,NP,triangle>), §5.1.8/Table 5.13,
// set_vertex/set_primitive_count/set_index.
// Đây là đường thay thế GS triangle-copy (1 primitive vào → 1 triangle ra,
// tương đương glDrawArrays(TRIANGLES) qua GS passthrough max_vertices=3).
// GS tổng quát (multi-stream, layer, topology khác, amplification) cần M5b
// dịch GLSL GS → mesh (ghi rõ, không giả).
struct GVOut { float4 pos [[position]]; float4 col; };
struct GPOut { float glow; };
[[mesh]] void gsMesh(mesh<GVOut, GPOut, 3, 1, topology::triangle> m,
                     constant float4 *inPos [[buffer(0)]],
                     constant float4 *inCol [[buffer(1)]],
                     uint tid [[thread_index_in_threadgroup]]) {
  // 1 threadgroup × 1 thread phát 1 triangle (GS invocation đơn).
  if (tid == 0) {
    for (uint i = 0; i < 3; ++i) {
      GVOut v; v.pos = inPos[i]; v.col = inCol[i];
      m.set_vertex(i, v);
    }
    GPOut p; p.glow = 0.0;
    m.set_primitive(0, p);
    m.set_index(0, (uchar)0);
    m.set_index(1, (uchar)1);
    m.set_index(2, (uchar)2);
    m.set_primitive_count(1);
  }
}
fragment float4 gsFS(GVOut in [[stage_in]]) { return in.col; }
