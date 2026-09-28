# Architecture

```
App (gl* calls, OpenGL 4.6 Core)
        |
TGLMT GL dispatch (src/gl/, one file per spec group)
  validate enum -> update CPU shadow -> request encode
        |
TGLMT Core (src/core/): Context / StateTracker / ObjectRegistry / ErrorTracker
        |
IMetal C++ interface (MetalInterface.h)
  +-- AppleMetalBridge.mm (real <Metal/Metal.h>, ARC)
  +-- NullMetalBackend.cpp (CPU trace for CI)
```

Shadows, not queries: Metal cannot report GPU state back, so every
`glGet*` reads a CPU copy written by the corresponding `glSet*`.

## Draw call flow

```
glDrawElements(mode, count, type, offset)
 -> EmitDraw: resolve EBO (VAO state), expand UBYTE->U16, split primitive
    restart runs, read DRAW_INDIRECT_BUFFER when bound
 -> trace encoder (always, keeps Null tests green)
 -> AppleDrawGL (Apple backend only):
      program linked? -> VAO to MTLVertexDescriptor (divisor supported)
      -> FBO 0 or wrapped texture target -> pipeline from cache
      -> viewport/scissor convert -> depth/blend/cull/fill
      -> vertex buffers + VS uniforms buf(16) + FS uniforms buf(16)
      -> UBO blocks buf(17+k) + textures by sampler unit
      -> index rewrite for baseVertex -> encode -> commit (no wait)
```

`FAN` becomes indexed triangles `(0,i,i+1)`, `LINE_LOOP` appends the first
index. Non-indexed variants are converted to indexed with generated indices,
so `first` (vertexStart) is preserved.

## Coordinate mapping

- GL origin bottom-left, Metal top-left: `glViewport`/`glScissor` are
  y-flipped against the target height (`GLConvert.h`).
- GL NDC z in [-1,1], Metal in [0,1]: the converter rewrites vertex output
  to `z*0.5 + w*0.5`. `glClipControl(ZERO_TO_ONE)` needs a relink (logged).
- `ReadPixels` flips rows back, so output matches GL bottom-left origin.

## Shader converter subset

Accepted: `#version` (100..460, 300 es), `layout(location=)` or bare
`in/out` (linker assigns), `uniform` scalars/vectors/matrices incl. arrays,
`sampler2D` (+Shadow/Cube/Array mapped to 2D), UBO blocks
(`layout(std140) uniform B { ... };`), 1..8 fragment outs (MRT),
`texture()`, `discard`, `gl_Position/PointSize/PointCoord/FragCoord`.

Rejected with a clear log (never guessed): tessellation/geometry/compute
stages, `texelFetch`, doubles, unsized arrays, matrix vertex attributes.

Link checks: every fragment input must be written by the vertex stage with
a matching type; explicit location mismatches fail. `glBindAttribLocation`
is honored; the rest is auto-assigned.

## Uniform and sampler model

Default-block uniforms live in `TGLMTUniforms` at `buffer(16)` per stage
(VS and FS each read their own struct from offset 0). UBO blocks bind at
`buffer(17+k)` in declaration order from the GL buffer currently bound to
the block's binding point. Sampler `texture(k)/sampler(k)` slots follow
declaration order; the GL texture unit comes from `glUniform1i` (default 0).
