# Giới hạn (trung thực)

Độ phủ decl 100% (698 core + 3 compat/extension lẻ, chứng thực qua symbol
table). Độ phủ hành vi thì không: dưới đây là no-op, trace-only hoặc xấp
xỉ CPU. Ngoài subset converter thì compile fail kèm log chứ không sinh code
đoán mò.

## Render đúng

VAO/VBO/EBO, `DrawArrays/Elements` (+`BaseVertex`, instanced, indirect,
multi), bung `FAN`/`LOOP`, tách primitive restart, index UBYTE, depth test,
blend per-attachment + `glColorMaski` (kiểm bởi `test_mrt_apple`), polygon
offset qua `setDepthBias`, cull, wireframe, scissor, sampler + anisotropy,
format sRGB/R8/RG8, MSAA resolve đường direct, `BlitFramebuffer` (cùng size
NEAREST bằng GPU, scale bằng CPU), sinh mipmap, roundtrip
`ReadPixels`/`GetTexImage`, `ARB_conservative_depth`
(`depth_greater`/`depth_less` map sang Metal `[[depth(...)]]`;
`depth_unchanged` dùng `any`).

## Đang stub hoặc xấp xỉ

| Mục | Hành vi |
|---|---|
| Stage tessellation | GPU subset: domain triangle/quad, TCS/TES, spacing/cw, MRT/MSAA/depth, `gl_ClipDistance[0]`. Isolines, point mode, factor >16 trên A11, multi-index clip và TCS clip distance chưa hỗ trợ. |
| Stage geometry | Compile fail trung thực; Metal không có stage geometry cổ điển và chưa nối compute fallback (xem HONESTLY_ABSENT.md) |
| Stage compute | GPU: kernel, SSBO, image, dispatch — `ARB_compute_shader` (kiểm bởi `test_compute_dispatch`) |
| Transform feedback | Capture thật cho vertex stage (compute prepass): interleaved/separate, pause/resume, instancing, array/matrix, overflow query. GS/TES capture và varying fp64 chưa có. |
| `glLogicOp` | Shadow only trên đường GL; kernel compute-ROP native có test `test_rop_apple` nhưng chưa nối vào `glDraw*` |
| Stencil test | GPU: encode vào depth-stencil state; Metal dùng một ref chung cho hai mặt |
| `glColorMask` / `glColorMaski` | GPU: write-mask per-attachment bake vào pipeline MRT; kiểm bởi `test_mrt_apple` |
| Blend attachment > 0, `ClearBuffer` MRT > 0 | GPU cho các đường MRT đã test; kiểm bởi `test_mrt_apple` (blend, mask, clear, read buffer) |
| Texture compressed / 3D / array / multisample storage | Compressed bị từ chối bằng lỗi GL; MSAA 2D và 2D array, texture 3D, texture 2D array và mip chain đã có. OpenGL không có target texture multisample 3D; `glTexImage3DMultisample`/`glTexStorage3DMultisample` nhận `GL_TEXTURE_2D_MULTISAMPLE_ARRAY`. FBO attach layer chạy cho 2D array (view từng layer), slice 3D (render pass `depthPlane`) và layer MSAA 2D array (multisample slice + resolve slice). |
| `glInvalidateFramebuffer` | Một phần: attachment được đánh dấu `contentUndefined` và dùng `DontCare`; `glInvalidateTexImage`/`glInvalidateTexSubImage` vẫn no-op |
| `glClearTexImage` / `glClearTexSubImage` | Clear level/region thật trên GPU + shadow; kiểm bởi `test_clear_tex_3d`, `test_clear_tex_cube`, `test_enum_behavior` |
| `ReadPixels` | Chỉ RGBA/BGRA + `UNSIGNED_BYTE` |
| Blend dual-source | Rơi về ONE/ADD |
| `baseInstance != 0` | GPU qua `drawIndexedBaseInstance` / `drawPrimitivesBaseInstance` |
| Packed vertex `UNSIGNED_INT_10F_11F_11F_REV` | Xấp xỉ normalized, có log |

## Ghi chú thiết bị (chuẩn A11)

- Mesh shader cần Apple7+ (A17/M3); vanilla không dùng.
- iOS cấm `Depth24Unorm_Stencil8`; TGLMT tự map sang
  `Depth32Float_Stencil8`.
- `generateMipmaps` cần texture mipmapped; texture 1 level giữ base level
  (render được, có thể shimmer).
- Hàng đợi gom lệnh đã BẬT: N draws cùng target chung 1 encoder
  (xem `docs/en/perf.md` + `test_perf_deferred_apple`: 100 draws → 1 encoder,
  ring 0 alloc ổn định). Trên A11 vẫn phải budget — FBO đảo liên tục hoặc
  feedback loop sẽ tách pass. Chỉ bật `TGLMT_DIAG=1` khi debug (release tắt
  để giữ 60fps).

## Hướng tiếp theo

Giai đoạn Sodium/Iris: tessellation isolines/point mode, geometry qua mesh
prepass (Apple7+) hoặc fallback CPU, XFB sau GS/TES, chiến lược fp64,
share-lists song song có fence, audit layout UBO `std140`. Xem `ROADMAP.md`
cho kế hoạch đủ và `COVERAGE.md` cho lệnh tái tạo.
