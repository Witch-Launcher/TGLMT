# Limits (honest)

Decl coverage is 100% (698 core + 3 compat/extension extras, verified via
symbol table). Behavior coverage is not: entries below are no-ops,
trace-only, or CPU approximations. Anything outside the converter subset
fails compile with a log instead of generating guessed code.

## Renders correctly

VAO/VBO/EBO, `DrawArrays/Elements` (+`BaseVertex`, instanced, indirect,
multi), `FAN`/`LOOP` expansion, primitive restart split, UBYTE indices,
depth test, per-target blend (attachment 0 on the GL path), cull, wireframe
fill, scissor, samplers + anisotropy, sRGB/R8/RG8 formats, MSAA resolve on
the direct path, `BlitFramebuffer` (same-size NEAREST on GPU, scaled on
CPU), mipmap generation, `ReadPixels`/`GetTexImage` roundtrip.

## Stubbed or approximated

| Area | Behavior |
|---|---|
| Tessellation / geometry / compute stages | Compile fails honestly; no GPU encode |
| Transform feedback | State machine + draw count only, no varying capture |
| `glLogicOp` | Shadow only (compute-ROP shader exists but is not wired to draws) |
| Stencil test | Tracked, not encoded |
| `glColorMask` | No-op (no write-mask bake) |
| Per-attachment blend beyond 0, `ClearBuffer` on MRT > 0 | Single-target only |
| Compressed / 3D / multisample texture storage | No-op or RGBA8 fallback with log |
| `ReadPixels` | RGBA/BGRA + `UNSIGNED_BYTE` only |
| Dual-source blend factors | Fall back to ONE/ADD |
| `baseInstance != 0` | Trace-only |
| `UNSIGNED_INT_10F_11F_11F_REV` packed vertex | Normalized approximation, logged |

## Device notes (A11 baseline)

- Mesh shaders need Apple7+ (A17/M3); vanilla does not use them.
- iOS forbids `Depth24Unorm_Stencil8`; TGLMT maps it to
  `Depth32Float_Stencil8` automatically.
- `generateMipmaps` needs a mipmapped texture; single-level textures keep
  the base level (renders, may shimmer).
- One command buffer per draw: correct, not yet batched. Budget draw calls
  on A11 accordingly.

## Roadmap pointer

Sodium/Iris stage: tessellation factors from TCS, geometry via mesh
prepass (Apple7+) or CPU fallback, real compute dispatch, parallel
share-lists with fencing, UBO `std140` layout audit. See `ROADMAP.md` for
the full plan and `COVERAGE.md` for reproduction commands.
