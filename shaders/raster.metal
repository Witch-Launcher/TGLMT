#include <metal_stdlib>
using namespace metal;
// TGLMT raster test shaders (layout chuẩn M5: attr0 float2, attr1 float4).
struct VIn { float2 pos [[attribute(0)]]; float4 data [[attribute(1)]]; };
struct VOut { float4 pos [[position]]; float4 col; };
// Depth test: z nằm trong data.x, màu trong data.yzw.
vertex VOut depthVS(VIn in [[stage_in]]) {
  VOut o; o.pos = float4(in.pos.x, -in.pos.y, in.data.x, 1.0);
  o.col = float4(in.data.yzw, 1.0); return o;
}
// Textured quad: uv trong data.xy (z/w pad), màu từ texture.
struct TOut { float4 pos [[position]]; float2 uv; };
vertex TOut texVS(VIn in [[stage_in]]) {
  TOut o; o.pos = float4(in.pos.x, -in.pos.y, 0.5, 1.0); o.uv = in.data.xy; return o;
}
fragment float4 texFS(TOut in [[stage_in]],
                      texture2d<float> tex [[texture(0)]],
                      sampler smp [[sampler(0)]]) {
  return tex.sample(smp, in.uv);
}
