# Limits (honest)

Decl coverage is 100% (698 core + 3 compat/extension extras, verified via
symbol table). Behavior coverage is not: entries below are no-ops,
trace-only, or CPU approximations. Anything outside the converter subset
fails compile with a log instead of generating guessed code.

## Renders correctly

VAO/VBO/EBO, `DrawArrays/Elements` (+`BaseVertex`, instanced, indirect,
multi), `FAN`/`LOOP` expansion, primitive restart split, UBYTE indices,
depth test, per-attachment blend and `glColorMaski` (verified by
`test_mrt_apple`), polygon offset via `setDepthBias`, cull, wireframe
fill, scissor, samplers + anisotropy, sRGB/R8/RG8 formats, MSAA resolve on
the direct path, `BlitFramebuffer` (same-size NEAREST on GPU, scaled on
CPU), mipmap generation, `ReadPixels`/`GetTexImage` roundtrip,
`ARB_conservative_depth` (`depth_greater`/`depth_less` map to Metal
`[[depth(...)]]`; `depth_unchanged` uses `any`).

## Stubbed or approximated

| Area | Behavior |
|---|---|
| Tessellation | GPU subset: triangle/quad domains, TCS/TES, spacing/cw, MRT/MSAA/depth, `gl_ClipDistance[0]`. Isolines, point mode, factors >16 on A11, multi-index clip and TCS clip distance are rejected/unsupported. |
| Geometry stage | Compile fails honestly; Metal has no classic geometry stage and no compute fallback is wired (see HONESTLY_ABSENT.md) |
| Compute stage | GPU: kernel, SSBO, image, dispatch — `ARB_compute_shader` (verified by `test_compute_dispatch`) |
| Transform feedback | Real capture for the vertex stage (compute prepass): interleaved/separate, pause/resume, instancing, arrays/matrices, overflow query. GS/TES capture and fp64 varyings are not implemented. |
| `glLogicOp` | Shadow only on the GL draw path; the native compute-ROP kernel is tested by `test_rop_apple` but not wired to `glDraw*` |
| Stencil test | GPU: encoded in depth-stencil state; a single Metal reference value is shared by front/back faces |
| `glColorMask` / `glColorMaski` | GPU: per-attachment write mask baked into the MRT pipeline; verified by `test_mrt_apple` |
| Per-attachment blend, `ClearBuffer` on MRT > 0 | GPU for the tested MRT paths; verified by `test_mrt_apple` (blend, mask, clear, read buffer) |
| Compressed / 3D / array / multisample texture storage | Compressed rejected with a GL error; MSAA 2D and 2D array, 3D textures, 2D array textures and mip chains implemented. OpenGL has no 3D multisample texture target; `glTexImage3DMultisample`/`glTexStorage3DMultisample` take `GL_TEXTURE_2D_MULTISAMPLE_ARRAY`. FBO layer attach works for 2D arrays (per-layer view), 3D slices (render pass `depthPlane`), and MSAA 2D-array layers (multisample slice + resolve slice). |
| `glInvalidateFramebuffer` | Partial: named attachments become `contentUndefined` and use `DontCare`; `glInvalidateTexImage`/`glInvalidateTexSubImage` remain no-ops |
| `glClearTexImage` / `glClearTexSubImage` | Real level/region clear on GPU plus shadow; verified by `test_clear_tex_3d`, `test_clear_tex_cube`, `test_enum_behavior` |
| `ReadPixels` | RGBA/BGRA + `UNSIGNED_BYTE` only |
| Dual-source blend factors | Fall back to ONE/ADD |
| `baseInstance != 0` | GPU via `drawIndexedBaseInstance` / `drawPrimitivesBaseInstance` |
| `UNSIGNED_INT_10F_11F_11F_REV` packed vertex | Normalized approximation, logged |

## Device notes (A11 baseline)

- Mesh shaders need Apple7+ (A17/M3); vanilla does not use them.
- iOS forbids `Depth24Unorm_Stencil8`; TGLMT maps it to
  `Depth32Float_Stencil8` automatically.
- `generateMipmaps` needs a mipmapped texture; single-level textures keep
  the base level (renders, may shimmer).
- Deferred command queue is ON: N draws on the same target share 1
  `MTLRenderCommandEncoder` (see `docs/en/perf.md` + `test_perf_deferred_apple`).
  Budget still matters on A11 — FBO thrash or feedback loops split passes.
  Set `TGLMT_DIAG=1` only for debugging (release stays quiet for 60fps).

## Roadmap pointer

Sodium/Iris stage: isolines/point mode tessellation, geometry via mesh
prepass (Apple7+) or CPU fallback, XFB after GS/TES, fp64 strategy,
parallel share-lists with fencing, UBO `std140` layout audit. See
`ROADMAP.md` for the full plan and `COVERAGE.md` for reproduction commands.
