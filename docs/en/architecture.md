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

## Deferred command queue (gom lệnh)

Three queues, zero Metal calls until flush:

1. **State shadow** (`StateTracker` + `Context::stateSeq`): redundant
   `glBind*/glEnable/glUniform*` are dropped by shadowing. `StateSeq` only
   grows on real change — proof `N redundant GL calls → 0 Metal calls`.
2. **Resource staging** (`pendingBufRanges`/`pendingTexRegions`): `SubData`/
   `TexSubImage` only `memcpy` shadow + record range. Ranges merge
   (adjacent/coalesced). GPU `memcpy+didModify`/`replaceRegion` deferred to
   flush. Conditional-flush: staging a buffer/texture NOT used in the open
   pass does NOT split batching (`flushAvoided` counter).
3. **Draw batching** (`pendingEncoder` + `pendingPipeKey` + `uniformCache` +
   ring): consecutive draws on the same target share one
   `MTLRenderCommandEncoder`. Pipeline/PSO looked up once per unique key,
   then reused by native handle. Uniform bytes compared; unchanged stages
   reuse the same ring slice. Temp uploads (uniform/UBO/index/baseVertex)
   come from a 3×4MB triple-buffered ring (`RingAlloc`) — steady-state
   zero `MTLBuffer` allocs per draw.

Diagnostics (`fprintf`, GUI-texture GPU readback, full draw-state dump) are
gated behind `TGLMT_DIAG=1`. Release/Minecraft keeps 60fps with `LogDebug`
callback only. Validators that prevent A11 TBDR faults (range/hazard) stay
always-on but skip after first warn per program (steady-state 0 scan).

See `docs/en/perf.md` for counters, budgets, and how to reproduce.

## Draw call flow (deferred full — 1 GL → 0-1 Metal when unchanged)

```
glDrawElements(mode, count, type, offset)
 -> EmitDraw: resolve EBO (VAO state), expand UBYTE->U16 on CPU, split primitive
    restart runs, read DRAW_INDIRECT_BUFFER when bound.
    Apple backend: SKIP trace encoder (Null keeps trace for CI tests).
    Non-Apple: trace encoder only.
 -> AppleDrawGL (Apple backend only):
      program linked? -> VAO to MTLVertexDescriptor (divisor supported)
      -> FBO 0 or wrapped texture target -> feedback-hazard pre-scan:
         sample-while-rendering? flush pending pass first (TBDR split)
      -> pipeline-key cache (Context): same inputs? reuse cachedPipe,
         skip bridge string-build + mutex entirely
      -> pendingEncoder reuse: same target+FBO+depth+no-clear? reuse open
         encoder, setPipeline only when nativeHandle differs
      -> dirty-check viewport/cull/fill/blend/depth: encode only on change
      -> vertex buffers (note used) + uniforms via ring (reuse bytes? 0 alloc)
      -> UBO blocks via ring + textures (note used, fallback black on incomplete)
      -> index: EBO-GPU fast path or ring upload (baseVertex rewrite in ring)
      -> drawPrimitives/drawIndexed into OPEN encoder, NO commit per draw
Flush (commitNoWait, 1 commit for N draws): target change, FBO change with
different texture, hazard split, staged-resource used in pass, ReadPixels/Blit/
CopyTex (readback), presentTarget, EndFrame, glClear pending on next draw.
```

`FAN` becomes indexed triangles `(0,i,i+1)`, `LINE_LOOP` appends the first
index. Non-indexed variants are converted to indexed with generated indices,
so `first` (vertexStart) is preserved. Fan/loop CPU expansion stays on CPU
(no MTL alloc); the resulting indices go through the same ring path.

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
