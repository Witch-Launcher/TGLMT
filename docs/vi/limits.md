# Giới hạn (trung thực)

Độ phủ decl 100% (698 core + 3 compat/extension lẻ, chứng thực qua symbol
table). Độ phủ hành vi thì không: dưới đây là no-op, trace-only hoặc xấp
xỉ CPU. Ngoài subset converter thì compile fail kèm log chứ không sinh code
đoán mò.

## Render đúng

VAO/VBO/EBO, `DrawArrays/Elements` (+`BaseVertex`, instanced, indirect,
multi), bung `FAN`/`LOOP`, tách primitive restart, index UBYTE, depth test,
blend per-target (attachment 0 trên đường GL), cull, wireframe, scissor,
sampler + anisotropy, format sRGB/R8/RG8, MSAA resolve đường direct,
`BlitFramebuffer` (cùng size NEAREST bằng GPU, scale bằng CPU), sinh
mipmap, roundtrip `ReadPixels`/`GetTexImage`.

## Đang stub hoặc xấp xỉ

| Mục | Hành vi |
|---|---|
| Stage tessellation / geometry / compute | Compile fail trung thực; chưa encode GPU |
| Transform feedback | State machine + đếm draw, không capture varying |
| `glLogicOp` | Shadow only (shader compute-ROP có nhưng chưa nối vào draw) |
| Stencil test | Đã lưu, chưa encode |
| `glColorMask` | No-op (chưa bake write-mask) |
| Blend attachment > 0, `ClearBuffer` MRT > 0 | Chỉ single-target |
| Texture compressed / 3D / multisample storage | No-op hoặc fallback RGBA8 kèm log |
| `ReadPixels` | Chỉ RGBA/BGRA + `UNSIGNED_BYTE` |
| Blend dual-source | Rơi về ONE/ADD |
| `baseInstance != 0` | Trace-only |
| Packed vertex `UNSIGNED_INT_10F_11F_11F_REV` | Xấp xỉ normalized, có log |

## Ghi chú thiết bị (chuẩn A11)

- Mesh shader cần Apple7+ (A17/M3); vanilla không dùng.
- iOS cấm `Depth24Unorm_Stencil8`; TGLMT tự map sang
  `Depth32Float_Stencil8`.
- `generateMipmaps` cần texture mipmapped; texture 1 level giữ base level
  (render được, có thể shimmer).
- Mỗi draw một command buffer: đúng trước, chưa batch. Budget số draw trên
  A11 cho phù hợp.

## Hướng tiếp theo

Giai đoạn Sodium/Iris: tessellation factors từ TCS, geometry qua mesh
prepass (Apple7+) hoặc fallback CPU, dispatch compute thật, share-lists
song song có fence, audit layout UBO `std140`. Xem `ROADMAP.md` cho kế
hoạch đủ và `COVERAGE.md` cho lệnh tái tạo.
