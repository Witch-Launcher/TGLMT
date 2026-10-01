# Performance (deferred full — honest numbers)

Target: Minecraft Java vanilla on iPhone A11+ (TBDR) and MacIntel (IMR/discrete),
both real Metal. Null backend stays green on CI (logic only).

## What was slow (1 GL → ~10-20 Metal, measured before fix)

- `EmitDraw` created a trace `IEncoder` AND encoded GPU work per draw
  (double-encoder on Apple).
- Every draw did 2× `newBufferWithBytes` (uniforms) + N× UBO uploads +
  index-rewrite alloc + `makeCustomPipeline` string-build + mutex, even when
  nothing changed.
- `StageBufferRange`/`StageTexRegion` flushed the open encoder on EVERY
  `SubData`, even for buffers not used in the pass → `encodersCreated ≈ draws`.
- Hot-path diagnostics: `fprintf` per unique combo, 256×256 GUI-texture
  `wrapAsTarget+readback` INSIDE draw (sync stall), 32k-index scan per draw,
  full draw-state dump with GPU readback.
- `blitCopy`/`generateMipmaps`/`CopyFB` used `commit+waitUntilCompleted`
  inside frame (CPU idle, GPU starved — WWDC19 anti-pattern).

## What changed (this patch)

| Fix | File | Counter |
|---|---|---|
| Skip trace encoder on Apple (`traceSkipped`) | `src/gl/gl_draw.cpp` | `traceSkipped` |
| Conditional-flush staging (`flushAvoided`) | `gl_buffer/texture.cpp`, `Context` used-sets | `flushAvoided` |
| Triple-buffer ring 3×4MB, 0 alloc steady-state | `Context::RingAlloc`, `gl_apple_draw` uniforms/UBO/index | `ringAllocs`, `tempAllocs==0` |
| Pipeline-key cache skips bridge lookup | `Context::pendingPipeKey` | `pipelineLookupSkipped` |
| Uniform bytes cache + dirty-check state | `uniformCache`, `pending*Valid` | `uniformReused`, `stateSkipped`, `depthReused` |
| Feedback-hazard split (TBDR correct) | pre-scan + `hazardSplits` | `hazardSplits`, `hazardWarn` |
| MultiDraw batch counting | `glMultiDraw*` | `multidrawBatched` |
| Diag gate `TGLMT_DIAG=1` (release quiet) | all `fprintf`/`readback` hot paths | `diagSkipped` |
| Validator skip-after-warn (4k scan cap) | index/vertex range checks | `rangeWarn` |
| Depth-state prewarm (16 states) | `PrewarmPipelineForProgram` | `psoPrewarmed` |

## How to reproduce

```sh
# Logic + perf proof, real Metal on MacIntel
cmake -S . -B build-apple -DTGLMT_APPLE_METAL=ON && cmake --build build-apple -j8
./build-apple/test_perf_deferred_apple   # want: 100 draws -> 1 encoder, ring>0, tempFallback=0
./build-apple/test_ir_batch_apple        # 3 draws -> 1 encoder
./build-apple/test_ir_full_apple         # pipeline 1+1, buffer 10->1, tex 4->1
ctest --test-dir build-apple             # 42/42 green

# Quiet release (Minecraft): no stderr spam, no readback stall
unset TGLMT_DIAG
# Debug on device: full dumps + GUI texture soi
TGLMT_DIAG=1 ./build-ios-pkg/run.sh 2> latestlog.txt
```

Expected `test_perf_deferred_apple` output (MacIntel, May 2026):

```
batch100: enc=1 (want 1) reuse=99 traceSkip=100 ring=2 tempFallback=0 us=~2000-3000
cond-flush: enc=1 flushAvoided>=5
used-flush: enc=2 (split on used-buffer stage)
multidraw: enc=1 batched=5
mid=(255,0,0)
```

`ring=2` = first draw uploads VS+FS once, 99 draws reuse (steady-state 0 alloc).
`us` varies by machine; assert is on counters, not wall-clock.

## Budgets (A11 baseline, vanilla 64×64 → fullscreen extrapolates linearly)

- Encoders per frame: `encodersCreated << drawsEncoded` (target < draws/50).
- Temp allocs per frame steady-state: `tempAllocs == 0` (ring covers).
- Sync waits per frame: 0 except `ReadPixels`/`present` boundaries.
- First-frame hitch: MSL compile at `glLinkProgram` (loading screen), PSO
  descriptor build at first draw (ms). Call `PrewarmPipelineForProgram` per
  program during loading to warm depth cache.
- If `encodersCreated ≈ drawsEncoded`, check: FBO thrash per draw, hazard
  split storm (`hazardSplits` high = feedback loop in shaders), or staging
  of used buffers per draw (chunk uploader writing VBO being drawn — needs
  double-buffering in launcher).

## What is still NOT deferred (honest)

- `glBufferData`/`glTexImage2D` allocate immediately (`newBuffer`/`newTexture`).
  Only `SubData`/`TexSubImage` are staged. Cold-path allocs stay (correct).
- `glCopyTexSubImage`/`glBlitFramebuffer` scaled path uses CPU fallback +
  `commitAndWait` (rare: blur/post chain). Same-size NEAREST stays GPU-only.
- PSO compile is synchronous at first draw per unique key (ms). Async compile
  with fallback PSO is roadmap (Metal4 suspend/resume, ICB for indirect).
- Fan/loop CPU expansion allocates `std::vector` per draw (rare: vanilla uses
  triangles; menu logo only).
- `ReadPixels`/`GetTexImage` flush + wait (correct per spec §18.2).

## Counters (`Context::AppleStats`, also via `Renderer::stats` subset)

`drawsAttempted/drawsEncoded/noProgram/noTarget/noPipeline/miscFail`,
`encodersCreated/encoderReused/pipelineReused/stateSkipped/uniformReused/
depthReused/pipelineLookups/pipelineLookupSkipped/bufferCoalesced/
bufferFlushes/texCoalesced/texFlushes/traceSkipped/diagSkipped/ringAllocs/
ringWraps/tempAllocs/flushAvoided/hazardSplits/multidrawBatched/psoPrewarmed`.
