#include <metal_stdlib>
using namespace metal;
struct VIn { float2 pos [[attribute(0)]]; float4 col [[attribute(1)]]; };
struct VOut { float4 pos [[position]]; float4 col; };
// TGLMT M4 mau: GL y-up -> Metal y-down (flip), NDC z [-1,1] -> [0,1] (z=0 -> 0.5)
vertex VOut triVS(VIn in [[stage_in]]) {
  VOut o; o.pos = float4(in.pos.x, -in.pos.y, 0.5, 1.0); o.col = in.col; return o;
}
fragment float4 triFS(VOut in [[stage_in]]) { return in.col; }
