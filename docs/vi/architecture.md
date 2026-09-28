# Kiến trúc

```
App (gọi gl*, OpenGL 4.6 Core)
        |
Dispatch GL của TGLMT (src/gl/, mỗi file một nhóm spec)
  validate enum -> cập nhật shadow CPU -> yêu cầu encode
        |
Core TGLMT (src/core/): Context / StateTracker / ObjectRegistry / ErrorTracker
        |
Interface IMetal C++ (MetalInterface.h)
  +-- AppleMetalBridge.mm (gọi <Metal/Metal.h> thật, ARC)
  +-- NullMetalBackend.cpp (trace CPU cho CI)
```

Shadow chứ không query: Metal không cho đọc state GPU về, nên mọi `glGet*`
đều đọc bản copy CPU mà `glSet*` tương ứng đã ghi.

## Luồng một lệnh draw

```
glDrawElements(mode, count, type, offset)
 -> EmitDraw: resolve EBO (state VAO), bung UBYTE->U16, tách runs
    primitive restart, đọc DRAW_INDIRECT_BUFFER khi bound
 -> trace encoder (luôn ghi, giữ test Null xanh)
 -> AppleDrawGL (chỉ backend Apple):
      program đã link? -> VAO thành MTLVertexDescriptor (có divisor)
      -> FBO 0 hoặc wrap texture -> pipeline từ cache
      -> convert viewport/scissor -> depth/blend/cull/fill
      -> vertex buffer + uniform VS buf(16) + uniform FS buf(16)
      -> UBO buf(17+k) + texture theo sampler unit
      -> viết lại index cho baseVertex -> encode -> commit (không đợi)
```

`FAN` thành triangles indexed `(0,i,i+1)`, `LINE_LOOP` thêm index đầu vào
cuối. Dạng non-indexed được chuyển sang indexed với index sinh thêm nên
`first` (vertexStart) được giữ nguyên.

## Ánh xạ tọa độ

- GL gốc bottom-left, Metal top-left: `glViewport`/`glScissor` flip-y theo
  chiều cao target (`GLConvert.h`).
- GL NDC z [-1,1], Metal [0,1]: converter viết lại output vertex thành
  `z*0.5 + w*0.5`. `glClipControl(ZERO_TO_ONE)` cần relink (có log).
- `ReadPixels` lật hàng lại nên output đúng gốc GL bottom-left.

## Subset converter shader

Nhận: `#version` (100..460, 300 es), `in/out` có hoặc không
`layout(location=)` (linker tự gán), `uniform` scalar/vector/matrix kể cả
mảng, `sampler2D` (+Shadow/Cube/Array map về 2D), UBO
(`layout(std140) uniform B { ... };`), 1..8 fragment out (MRT),
`texture()`, `discard`, `gl_Position/PointSize/PointCoord/FragCoord`.

Từ chối kèm log rõ (không đoán): stage tessellation/geometry/compute,
`texelFetch`, double, mảng không size, attribute ma trận.

Link kiểm tra: mọi input của fragment phải được vertex ghi cùng tên cùng
kiểu; lệch location explicit thì fail. `glBindAttribLocation` được tôn
trọng, còn lại tự gán.

## Mô hình uniform và sampler

Uniform default-block nằm trong `TGLMTUniforms` ở `buffer(16)` mỗi stage
(VS và FS đọc struct riêng từ offset 0). UBO bind ở `buffer(17+k)` theo
thứ tự khai báo, lấy từ GL buffer đang bound vào binding point của block.
Slot sampler `texture(k)/sampler(k)` theo thứ tự khai báo; texture unit GL
lấy từ `glUniform1i` (mặc định 0).
