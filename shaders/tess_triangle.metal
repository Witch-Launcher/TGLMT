#include <metal_stdlib>
using namespace metal;
// TGLMT tessellation test: 1 triangle patch, 3 control points.
// Căn cứ MSL spec §5.1.1.1 (post-tess vertex fn, [[patch(triangle)]]),
// §5.2.3.2/Table 5.3 (patch_control_point input, position_in_patch float3 cho triangle).
// Với factors 1.0, nội suy barycentric cho đúng tam giác gốc (passthrough đúng
// hành vi GL tess level 1; TCS tính factors động cần M5b).
struct CP { float2 pos [[attribute(0)]]; float4 col [[attribute(1)]]; };
struct VOut { float4 pos [[position]]; float4 col; };
[[patch(triangle)]] [[vertex]] VOut tessVS(patch_control_point<CP> cps [[stage_in]],
                                           float3 bp [[position_in_patch]]) {
  float2 p = cps[0].pos * bp.x + cps[1].pos * bp.y + cps[2].pos * bp.z;
  float4 c = cps[0].col * bp.x + cps[1].col * bp.y + cps[2].col * bp.z;
  VOut o; o.pos = float4(p.x, -p.y, 0.5, 1.0); o.col = c; return o;
}
fragment float4 tessFS(VOut in [[stage_in]]) { return in.col; }
