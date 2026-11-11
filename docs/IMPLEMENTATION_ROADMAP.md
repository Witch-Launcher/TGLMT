# Implementation Roadmap

Derived from the audit (`reports/AUDIT_REPORT.md`), the coverage report and
the conformance report. Priorities reflect **risk to correctness**, not
effort: a silent wrong answer is worse than a missing feature.

Status legend: **DONE** (this session) · **OPEN** (not started) ·
**PARTIAL** (started).

---

## Phase 0 — Project discovery — **DONE**

Repository-wide investigation of the source, headers, generators, tools,
tests, benchmarks and build system. Result: `docs/PROJECT_ARCHITECTURE.md`.

## Phase 1 — API audit — **DONE**

The catalog was verified against the Khronos registry and the "698" figure
was found to be wrong (the real core surface is 657). The derivation was
fixed and made single-sourced. Result:
`docs/OPENGL_46_API_INVENTORY.md`, `docs/OPENGL_46_API_COVERAGE.md`.

## Phase 2 — Metal feasibility — **DONE**

Every OpenGL 4.6 Core capability was classified A–E against Metal and the
A11 feature set. Result: `docs/OPENGL_46_METAL_MAPPING.md`,
`docs/METAL_A11_LIMITATIONS.md`.

## Phase 3 — Baseline test & benchmark — **DONE**

Baseline captured before any change: build times, CTest results, the number
of tests that were silently inert, and the benchmark numbers. Result:
`reports/BASELINE_TEST_REPORT.md`, `benchmarks/baseline/`.

## Phase 4 — Implementation plan — **DONE**

This document.

## Phase 5 — Implementation — **DONE (correctness) / OPEN (features)**

### 5.1 Correctness fixes — **DONE**

21 defects fixed, each with a regression test:

| Area | Fixes |
|---|---|
| API catalog | 657 core / 42 compatibility-only; one shared derivation; generated artefacts regenerated |
| Crash safety | `glShaderSource(NULL)`, `glUniform*fv(count<0)`, `glBufferData` of a mapped buffer (use-after-free) |
| Error generation | `glGet*` INVALID_ENUM (965 accepted pnames), `glBindTexture` target, draw mode/type, the five `Invalidate*` entry points |
| Object lifetime | buffers, programs, VAOs, samplers unbound on deletion; `glDeleteShader` respects attachment |
| Shader semantics | texture bias, `flat`/`noperspective`/`centroid`, fragment output location, `textureQueryLod` |
| Uniform data | `transpose` honoured; `mat3` packed into the MSL column layout |
| Metal backend | pipeline-cache identity (`ILibrary::identity()` + `linkGen`); default-framebuffer depth test enabled |
| Test system | `assert()` → always-on checks; CTest working directory; Metal-compiler gate; `docs/` tracked |

### 5.2 Remaining correctness work — **OPEN**, ranked

| # | Item | Severity | Effort | Notes |
|---|---|---|---|---|
| R1 | **Texture completeness model** (GL 4.6 §8.14) | HIGH | small | a mipmap-filtered 1-level texture must sample black, not level 0 |
| R2 | `GL_MAX_TEXTURE_IMAGE_UNITS` 32 → **31**; `GL_MAX_UNIFORM_BUFFER_BINDINGS` 36 → **31** | HIGH | minutes | a 32-sampler shader currently aborts Metal validation |
| R3 | Filter `GL_COMPRESSED_TEXTURE_FORMATS` by device capability | HIGH | small | BC is advertised on devices that have no BC decoder |
| R4 | `glBeginQuery` must flush the pending render encoder for occlusion targets | HIGH | small | otherwise the visibility buffer is missing from the pass descriptor |
| R5 | `INCOMPLETE_MULTISAMPLE` / `FRAMEBUFFER_UNSUPPORTED` | MED | small | a mixed-sample FBO currently produces a mismatched Metal pass |
| R6 | Size-mismatched attachments must be **legal** (effective size = intersection) | MED | small | the current code returns `INCOMPLETE_DIMENSIONS`, a token removed in GL 3.0 |
| R7 | `GL_TEXTURE_MIN_LOD`, `GL_TEXTURE_LOD_BIAS`, `GL_TEXTURE_COMPARE_MODE` on textures | MED | medium | silently ignored today |
| R8 | `GL_UNPACK_IMAGE_HEIGHT`, `GL_UNPACK_SWAP_BYTES`, `GL_UNPACK_LSB_FIRST` | MED | medium | stored but never applied; 3D uploads read the wrong slices |
| R9 | `glUniform*` with a non-existent location must raise `INVALID_OPERATION` | MED | small | currently writes into the uniform store |
| R10 | `glMapBufferRange`: validate the access flags against the storage flags; reject `length == 0` and a concurrent mapping | MED | small | |
| R11 | `glUnmapBuffer` must return `GL_FALSE` when the mapping was lost | MED | small | |
| R12 | `glGetBufferSubData` must wait for non-compute GPU writers | MED | small | |
| R13 | `glGetFramebufferAttachmentParameteriv` must use the dense attachment index | MED | small | every colour attachment query currently returns `OBJECT_TYPE = GL_NONE` |
| R14 | `glGetQueryiv(GL_QUERY_COUNTER_BITS)` must return ≥ 8 | LOW | tiny | |
| R15 | `glBindBufferRange` must check `offset + size <= bufferSize` | LOW | tiny | |
| R16 | `glDeleteQueries` must end an active query | LOW | tiny | |
| R17 | `glGetBufferParameteri64v` must not truncate through `GLint` | LOW | tiny | breaks sizes above 2 GiB |
| R18 | Sampler-state cache is unbounded | MED | small | one `MTLSamplerState` per texture id, forever |
| R19 | Wrap cache: key on a per-texture serial, not the raw `ITexture*` | MED | small | ABA hazard on in-place reallocation |
| R20 | Index-rewrite cache must be per-context | MED | small | cross-context index corruption |
| R21 | `gInflightCB` needs a mutex | MED | tiny | data race on the command-buffer deque |
| R22 | Fallback context must be per-thread | MED | small | two threads share one `Context` today |
| R23 | `glMemoryBarrier` should honour the individual barrier bits | LOW | medium | currently a full barrier for every bit |
| R24 | `glTextureBarrierByRegion` is not implemented | LOW | small | |
| R25 | The draw path must raise `INVALID_OPERATION` when no program is current | LOW | tiny | currently the draw is silently dropped |
| R26 | `glUniform*fv` with `count > 0` and a NULL pointer must raise `INVALID_VALUE` | LOW | tiny | |

### 5.3 Feature work — **OPEN**

| # | Item | Class | Notes |
|---|---|---|---|
| F1 | `glClientWaitSync` must actually block | C | needs `MTLSharedEvent` + `MTLCommandBufferEncoder signalEvent:`; currently always `ALREADY_SIGNALED` |
| F2 | `std140` / `std430` / `packed` / `row_major` layout in the translator | B | the single biggest GLSL-conformance gap |
| F3 | GPU-executed indirect draws | B | `MTLIndirectCommandBuffer`; the GL semantics (read at execution time) do not map directly |
| F4 | `glReadPixels` from float / half-float / integer attachments | C | only RGBA8 is supported |
| F5 | `glBlitFramebuffer` with `GL_LINEAR` + scaling | B | currently nearest ⇒ visible aliasing |
| F6 | `GL_LINEAR` blit for non-matching format pairs | B | the spec restricts which pairs support LINEAR |
| F7 | `glGenerateMipmap` must be a no-op with `INVALID_OPERATION` for a non-mipmapped texture | C | |
| F8 | `glTexStorage*` must set the immutable flag | C | immutability is not enforced |
| F9 | `glCopyTexSubImage3D` should be a GPU copy, not a CPU round-trip | B | |
| F10 | `glDrawTransformFeedback` must re-bind the captured attributes | B | currently replays the vertex count only |
| F11 | `IComputeEncoder::dispatchGroups3D` must be implemented (both backends) | B | the tessellation-control path needs it |
| F12 | Compute limits (1024 invocations, 32 KB shared memory) must be enforced | C | |
| F13 | Dual-source blending | D | **impossible on A11** — document, do not implement |
| F14 | Geometry shaders | D | **impossible on A11** — currently rejected with a clear log |
| F15 | Per-sample shading rate | D | **impossible on any Metal version** |
| F16 | BC compressed textures | D | **impossible on A11** (ETC2/ASTC work) |

### 5.4 Architecture work — **OPEN**

| # | Item | Rationale |
|---|---|---|
| A1 | **A real GLSL front end** (lexer → parser → AST → semantic analysis) | the rewriter cannot diagnose an unknown identifier at compile time, cannot implement `std140`, and cannot report the errors the spec requires |
| A2 | **A table-driven state machine** for the 341 state variables | per-entry-point validation is how 55 % of the entry points ended up with no error check |
| A3 | **Make the null backend equivalent** to the Apple backend for MRT, uploads, mips and the compute grid | lets CI cover GPU semantics without a GPU |
| A4 | **An offscreen EGL-like platform** + vendored `VK-GL-CTS` | the only way to obtain a real conformance number |
| A5 | **Context-level locking** or an explicit "one context per thread" contract | the API is not thread-safe today |
| A6 | **A shader binary cache** (Metal library archives) | the Metal compiler's cold path costs up to 175 ms per library |

## Phase 6 — Conformance & differential testing — **PARTIAL**

Done: the API catalog, the accepted-pname list, 78 pixel-level integration
tests, the differential converter test against `glslang` + `spirv-cross`, and
the on-device suite.

Not done: the Khronos CTS, a reference-GL differential harness, and an A11
device run. Result: `reports/CONFORMANCE_REPORT.md`.

## Phase 7 — Performance — **DONE (one optimisation)**

Delivered: the duplicate texture upload was removed (−31 % on the upload
path, measured against a ~5 % noise floor; ~60 % on the iOS shared-storage
path in isolation).

Rejected with reasons: shared storage on macOS, texture-allocation reuse,
batched staging blits, a hashed pipeline key, and GLSL result caching.

Remaining bottlenecks are listed in `docs/PERFORMANCE_OPTIMIZATION.md` §5.

## Phase 8 — Final validation — **DONE**

Clean builds (zero warnings), 185 / 185 tests pass in both configurations, the
manifest and the API documents regenerated from the built library, and the
benchmark A/B re-run. Result: `reports/FINAL_VALIDATION_REPORT.md`.

## Phase 9 — Documentation — **DONE**

| Document | Contents |
|---|---|
| `docs/PROJECT_ARCHITECTURE.md` | layering, data structures, build system, tooling, known limitations |
| `docs/OPENGL_46_API_INVENTORY.md` | the canonical catalog, how it was derived, the per-function table |
| `docs/OPENGL_46_API_COVERAGE.md` | status per API, generated from the manifest |
| `docs/OPENGL_46_METAL_MAPPING.md` | A–E feasibility matrix, A11 capability check, cost of the emulated paths |
| `docs/GLSL_460_COMPILER.md` | the translator's architecture, feature matrix, semantic divergences |
| `docs/METAL_A11_LIMITATIONS.md` | what A11/Metal genuinely cannot do, and the consequences |
| `docs/TESTING_STRATEGY.md` | what is proven, how, and what is not |
| `docs/PERFORMANCE_OPTIMIZATION.md` | methodology, measurements, rejected optimisations |
| `docs/IMPLEMENTATION_ROADMAP.md` | this document |
| `reports/*.md` | audit, coverage, baseline, conformance, performance, final validation |
| `benchmarks/README.md` | the measurement protocol |

---

## Recommended order for the next session

1. **R2, R3** (minutes; they prevent Metal aborts and a false advertisement).
2. **R1** (texture completeness) — a whole class of conformance failures.
3. **R4, R5, R6, R13** (small, high-value correctness fixes).
4. **F1** (`glClientWaitSync`) — unblocks fence-based applications.
5. **A4** (CTS platform) — the only way to know the real conformance level.
6. **F2** (std140/std430) — the biggest GLSL-conformance gap.
7. **A3** (null-backend parity) — improves CI coverage for everything above.
