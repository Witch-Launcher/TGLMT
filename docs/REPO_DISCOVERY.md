# REPO DISCOVERY — TGLMT (Phase 0, full repository)

> **Scope of this document**: what the TGLMT repository *actually is* on
> 2026-11-11, measured from the working tree. Every claim carries a
> `file:line` citation. Anything that could not be verified from the tree is
> marked **NOT_VERIFIED** explicitly.
>
> **Measured totals**: 66 code files / **54 195 LOC** in `src/**` + `include/**`
> (`wc -l`); 185 CTest targets; 18 public headers (6 947 LOC), 5 core files
> (969), 41 GL files (42 706), 2 Metal backend files (3 573).

---

## 1. Executive summary

TGLMT today is a **single-layer, OpenGL 4.6 Core-only translator**. It is a
faithful, well-instrumented implementation of the modern programmable pipeline
mapped onto Metal, with an honest inventory/audit toolchain — but it has **no
legacy fixed-function engine, no compatibility-profile context model, no
versioned API surface, and no separate IR or validation layer**.

The eight layers named in the mission brief map onto the tree as follows
(evidence in §2):

| # | Layer | Exists? | Where |
|---|---|---|---|
| 1 | **API frontend** (`extern "C"` surface) | **YES** | `src/gl/gl_c_exports.cpp` (generated, 3 533 LOC, 701 slots) |
| 2 | **Context + state** | **YES** | `include/tglmt/Context.h` (1 302) + `StateTracker.h` (304) + `ErrorTracker.h` (24) + `ObjectRegistry.h` (32) + `src/core/` (5 files, 969 LOC) |
| 3 | **Legacy compatibility engine** (fixed-function) | **NO — does not exist at all** | §2.4 |
| 4 | **Modern pipeline** (2.0–4.6 programmable) | **YES** | `src/gl/gl_{program,shader,uniform,vertex,draw,texture,sampler,framebuffer,compute,sync_query,tess_xfb,readpixels_dsa}.cpp` + `src/gl/gl_apple_draw.cpp` |
| 5 | **Internal IR** | **PARTIAL — a naming convention, not a module** | No IR type/file exists. "IR" in the tree means the *deferred encoder + pipeline-key cache* inside `src/gl/gl_apple_draw.cpp` (`:2352`, `:2409`, `:2614`, `:2817`, `:3719`). |
| 6 | **Shader translation** | **YES** | `src/gl/glsl_to_msl.cpp` (12 627 LOC) + `include/tglmt/GLSLConverter.h` |
| 7 | **Metal backend** | **YES** | `include/tglmt/MetalInterface.h` (926 LOC, `IDevice` with 83 virtuals) + `src/metal/AppleMetalBridge.mm` (3 494) + `src/metal/NullMetalBackend.cpp` (79) |
| 8 | **Validation** | **NO as a separate layer** | Validation is inlined in every entry point (`c.errors.Record(0x0500)` …). The only reusable validation artefact is the generated pname whitelist `include/tglmt/gl46_pnames.h`. |

---

## 2. The layering as implemented

### 2.1 Call path (measured)

```
application
  └─ dlsym("glDrawElements")
       └─ src/gl/gl_c_exports.cpp:…        extern "C" wrapper + GlProfileScope timer
            └─ tglmt::gl::glDrawElements  src/gl/gl_draw.cpp:…
                 ├─ validate (ErrorTracker::Record, src/core/ErrorTracker.cpp:8)
                 ├─ mutate StateTracker (include/tglmt/StateTracker.h:60)
                 ├─ mutate Context object maps (include/tglmt/Context.h:644-673)
                 └─ AppleDrawGL()  include/tglmt/GLAppleDraw.h:7 → src/gl/gl_apple_draw.cpp
                      └─ metal::IDevice / IRenderEncoder (include/tglmt/MetalInterface.h:510, :371)
                           └─ src/metal/AppleMetalBridge.mm → id<MTLRenderCommandEncoder>
```

There is **no function-pointer dispatch table** at runtime. `src/gl/gl_dispatch_gen.cpp`
is a *name manifest* consumed only by the audit tooling — `docs/PROJECT_ARCHITECTURE.md:92-96`
states this, and it is consistent with the code: the generated file defines
`GetCoreFunctionCount()`, `GetCoreFunctionName()`, `GetCoreFunctionTableSize()`,
`GetCompatOnlyFunctionCount()`, `GetCompatOnlyFunctionTableSize()`, and **no file
under `src/` calls any of them** — their only consumers are the tests
(`tests/api/test_api_surface.cpp:30-52`, `tests/regression/test_regression_catalog.cpp:31-44`,
`tests/conformance/test_conformance_surface.cpp:32-47`). The extension count is
*not* taken from the generated file: `tglmt_num_extensions()` is defined by hand at
`src/gl/gl_error_string_debug.cpp:340` and consumed at `src/gl/gl_get.cpp:142`.

### 2.2 API frontend — EXISTS

* `src/gl/gl_c_exports.cpp` is **generated** (`tools/gen_c_exports.py`, banner at
  `src/gl/gl_c_exports.cpp:1`). It emits 701 `extern "C"` forwarders, one per
  catalogued entry point plus 2 bonus symbols:
  * `include/tglmt/gl46.h` declares **657 core + 42 compatibility-only = 699**
    prototypes (`include/tglmt/gl46.h:2-3`, compat block at `:666-711`).
  * `tools/gen_c_exports.py:37` adds `BONUS = ["glGetPointerv", "glLineStipple",
    "glDepthRangeArrayfvNV"]`. `glGetPointerv` is already a core entry point
    (re-required by `GL_VERSION_4_3`), so only `glLineStipple` and
    `glDepthRangeArrayfvNV` are extra → **699 + 2 = 701**, matching
    `src/gl/gl_c_exports.cpp:3532` (`int gGlProfileN = 701;`).
* Each wrapper is timed by `tglmt::GlProfileScope` (`include/tglmt/GlProfile.h:25-38`),
  which feeds the `gltop#` diagnostics. `include/tglmt/GlProfile.h:12` hard-caps the
  table at `kGlProfileMax = 768`; `tools/gen_c_exports.py:107-108` fails the build
  if the count exceeds it — **adding ~67 more entry points for GL 1.0–2.1 will
  consume roughly half of that headroom.**
* `src/gl/glfw_shim.cpp:367-406` holds a **separate, hand-maintained** `GetProcAddress`
  table with ~70 entries. It is *not* generated and is not the full surface — a
  legacy app calling `TGLMT_GetProcAddress("glBegin")` would get `nullptr`
  (`src/gl/glfw_shim.cpp:404-405` returns `nullptr` for unknown names).

### 2.3 Context + state — EXISTS

| Component | Header | Impl | Relationship |
|---|---|---|---|
| `Context` | `include/tglmt/Context.h:632-1300` | `src/core/Context.cpp` (602 LOC) | Owns everything: `ErrorTracker errors` (`:637`), `StateTracker state` (`:638`), `ObjectRegistry registry` (`:639`), the Metal `device` (`:641`), 12 object maps (`:644-661`), the deferred-encoder shadow (`:883-896`), the pipeline MRU cache (`:1087-1097`), the staging queues (`:1104-1125`) and the wrap-target cache (`:1171-1189`). |
| `StateTracker` | `include/tglmt/StateTracker.h:60-302` | `src/core/StateTracker.cpp` (150 LOC) | The GL state machine: capability enables (`:65-69`), blend per draw buffer (`:72-89`), depth (`:92-101`), stencil (`:94-100`), viewport/scissor ×16 (`:115-164`), raster state (`:104-142`), pixel-store (`:51-58`, `:250-251`), and every binding point (`:174-249`). Plus a generic byte store `SetShadow/GetShadow` (`:262-263`) used to answer `glGet*`. |
| `ErrorTracker` | `include/tglmt/ErrorTracker.h:10-22` | `src/core/ErrorTracker.cpp` (61 LOC) | FIFO error queue, **capped at 32 entries** (`src/core/ErrorTracker.cpp:44`), mutex-protected, `glGetError` pops the front (`:46-52`). `KHR_no_error` bypass: `ErrorTracker.h:12` + `src/core/Context.cpp:49-55` (`TGLMT_NO_ERROR=1` env var). |
| `ObjectRegistry` | `include/tglmt/ObjectRegistry.h:18-30` | `src/core/ObjectRegistry.cpp` (26 LOC) | One monotonic name space shared by **12 object kinds** (`include/tglmt/ObjectRegistry.h:13-16`), matching the 12 maps in `Context` (`Context.h:644-656`, `:661`). |

**How they relate:** `Context` *contains* the other three by value
(`Context.h:637-639`); `StateTracker` and `ErrorTracker` are leaf-ish (no
back-pointer to `Context`), `ObjectRegistry` only holds ids and a `live_` set
(`ObjectRegistry.h:29`) — it does **not** own the object payloads, `Context`'s maps do.
So `ObjectRegistry` answers `glIsX` (`ObjectRegistry.h:23`) while the payload
lives in `Context`.

**Key state-modelling convention:** Metal bakes state into immutable pipeline
objects, so every GL state change must be *shadowed* on the CPU and re-baked
when a draw happens. `StateTracker::SetShadow/GetShadow`
(`StateTracker.h:262-263`) is the mechanism for all state that has no
dedicated field — this is the single biggest structural fact for the legacy
work, because fixed-function state (lighting, fog, texenv, texgen, current
texcoord, matrix) is *large* and mostly has no dedicated field.

### 2.4 Legacy compatibility engine — **DOES NOT EXIST** (verified)

**Claim:** there is no legacy fixed-function / compatibility engine in this
repository — not a partially implemented one, not a stub, not even a skeleton.

**Evidence (all reproducible):**

1. **No directory, no file.**
   `find src -type d` → exactly `src/core`, `src/gl`, `src/metal`. There is no
   `compat/`, `legacy/`, `fixed_function/`, or equivalent.
   `grep -rn "legacy\|Legacy" src/` returns only 25 hits, all of which are
   unrelated (legacy *texture* mip handling in `src/gl/gl_texture.cpp:622,653,824,1014,1148,1766`,
   legacy *program* fallback in `src/gl/gl_apple_draw.cpp:3113-3115`, legacy
   `gl_FragColor` support in the shader converter `src/gl/glsl_to_msl.cpp:9660-12608`,
   and the `EXT_direct_state_access` "legacy/state entries" comment in
   `src/gl/gl_ext_dsa_state.cpp:1`).

2. **No immediate mode.** `grep -rn "glBegin\b" src/` → no `glBegin`/`glEnd`
   entry point. The only `glBegin*` symbols are `glBeginConditionalRender`,
   `glBeginQuery`, `glBeginQueryIndexed`, `glBeginTransformFeedback`
   (`src/gl/gl_c_exports.cpp:30,34,38,42`). Similarly there is no `glVertex2/3/4*`,
   `glColor3/4*`, `glNormal3*`, `glTexCoord1..4*`, `glEdgeFlag`, `glMultiTexCoord*`
   (non-`P` variants), `glRect*`, `glArrayElement`, `glPointSize` exists but as a
   1.0-era *core* function (`include/tglmt/gl46.h:382`), not as immediate mode.

3. **No legacy matrix stack.** `grep -rn "glMatrixMode\|glLoadIdentity\|glLoadMatrix\|glPushMatrix\|glPopMatrix\|glMultMatrix\|glTranslated\|glRotated\|glScaled\|glOrtho\|glFrustum" src/` → **0 matches**.
   The only matrix code in the tree is the **named-matrix** (`EXT_direct_state_access`)
   variant: `src/gl/gl_ext_dsa_state.cpp:56-78` (`Matrix()`, `Mul()`, `Load()`) and
   `:256-321` (`glMatrixLoadfEXT`, `glMatrixMultfEXT`, `glMatrixPushEXT`,
   `glMatrixPopEXT`, `glMatrixTranslatefEXT`, `glMatrixScalefEXT`,
   `glMatrixRotatefEXT`, `glMatrixOrthoEXT` `:308`, `glMatrixFrustumEXT` `:315`, …).
   Those operate on `Context::ext.matrixStacks`
   (`include/tglmt/Context.h:581`) and are **not** the legacy global
   `glMatrixMode`/`glPushMatrix` stack: there is no global *current matrix mode*,
   no `glPushMatrix`/`glPopMatrix` on a global stack, and **the named matrices are
   never applied to a draw** — `src/gl/gl_apple_draw.cpp` does not read
   `Context::ext.matrixStacks` at all.

4. **No lighting / material / fog / texture-env / texture-gen.**
   `grep -rn "GL_LIGHTING\|GL_FOG\|GL_TEXTURE_GEN\|GL_COLOR_MATERIAL\|glLightf\|glMaterialfv\|glFogfv\|glTexEnvf\|glTexGenf" src/gl/*.cpp src/core/*.cpp include/tglmt/Context.h include/tglmt/StateTracker.h`
   → **exactly 1 hit**, and it is an unrelated comment
   (`src/gl/gl_get.cpp:518`, mentioning the enum `GL_FOG_COLOR`).
   (Widening the search to the generated headers only finds enum *constants*:
   `include/tglmt/gl46_types.h:1605`, `include/tglmt/gl46_pnames.h:453` — never a
   state field or an implementation.)
   `Context::ExtState` (`include/tglmt/Context.h:570-631`) does have
   `texEnvF/texEnvI/texGenF/texGenI` maps (`:582-585`) and a `matrixStacks` map
   (`:581`) — but these are **bookkeeping maps for the extension entry points
   only**; nothing in the draw path (`src/gl/gl_apple_draw.cpp`) reads them.
   **Note:** this is also *not* the legacy `GL_TEXTURE_ENV` fixed-function
   pipeline; the modern `GL_TEXTURE_ENV` core state is not implemented at all.

5. **No display lists / evaluators / selection / feedback / accumulation /
   pixel ops / client-side vertex arrays.**
   `grep -rn "glNewList\|glCallList\|glEvalPoint\|glEvalMesh\|glSelectBuffer\|glFeedbackBuffer\|glAccum\|glPixelMap\|glRasterPos\|glDrawPixels\|glCopyPixels\|glEnableClientState\|glVertexPointer\|glInterleavedArrays" src/gl/*.cpp`
   → **2 hits, both `EXT_direct_state_access` variants**, not the GL 1.0 API:
   `src/gl/gl_ext_dsa_state.cpp:328` `glEnableClientStateIndexedEXT` and
   `:334` `glEnableClientStateiEXT`. They only flip a bookkeeping flag
   (`src/gl/gl_ext_dsa_state.cpp:329`,
   `Context::ext.clientEnabled[{array,index}]`, declared
   `include/tglmt/Context.h:587`) that **no draw path ever reads**.
   Their siblings `glClientAttribDefaultEXT` (`:323`) and
   `glPushClientAttribDefaultEXT` (`:327`) only clear that map.
   There is **no** `glNewList`/`glCallList`/`glEvalPoint`/`glSelectBuffer`/
   `glFeedbackBuffer`/`glAccum`/`glPixelMap*`/`glRasterPos*`/`glDrawPixels`/
   `glCopyPixels`/`glVertexPointer`/`glInterleavedArrays` anywhere.
   (`glReadPixels` — a *core* 1.0 function — exists at
   `src/gl/gl_readpixels_dsa.cpp:89` and is unrelated to legacy pixel ops.)
   `src/gl/gl_ext_imaging.cpp` implements `ARB_imaging` **state**
   (colour table / convolution / histogram parameters, `:1-30`) — the *fixed-function
   pixel pipeline* behind them is not implemented.

6. **The 42 "compatibility-only" entry points are NOT legacy.**
   `python3 tools/gl46_catalog.py` prints
   `OpenGL 4.6 Core Profile commands: 657 (expected 657)` and
   `Compatibility-profile-only commands: 42 (expected 42)`.
   Listing them shows they are exclusively:
   * 30 packed-attribute functions from `GL_VERSION_3_3`
     (`glVertexP2ui`…`glSecondaryColorP3uiv`), and
   * 12 `glGetn*` robustness getters from `GL_VERSION_4_5`
     (`glGetnColorTable`, `glGetnMapdv`, `glGetnPixelMap*`, …).
   They are implemented in `src/gl/gl_packed_robust.cpp:1-3`.
   **Not one** of them is a GL 1.0–2.1 fixed-function entry point.

7. **The context reports Core, not Compatibility.**
   `src/gl/gl_get.cpp:148` → `case 0x9126: *v = 0x00000001;` i.e.
   `GL_CONTEXT_CORE_PROFILE_BIT` only; the compatibility bit is never set.
   `src/gl/gl_error_string_debug.cpp:17` →
   `GL_VERSION` = `"4.6 (TGLMT Metal port, build …)"`; there is no
   version/profile negotiation at all.
   `tests/api/test_api_surface.cpp:79-80` actively asserts that the
   compatibility bit is **absent**.

**Quantified size of the legacy hole.** Computing the set of commands required by
`GL_VERSION_1_0` … `GL_VERSION_2_1` in `docs/khronos/gl.xml` and comparing it with
the 657 + 42 currently declared:

| Metric | Count |
|---|---:|
| Commands required by `GL_VERSION_1_0`..`GL_VERSION_2_1` (any profile) in `gl.xml` | 551 |
| Of those, already declared/exported by TGLMT | 202 |
| **Missing (no declaration in `gl46.h`, no exported symbol)** | **349** |

Representative missing families: `glBegin`/`glEnd`, all `glVertex*`/`glColor*`/
`glNormal*`/`glTexCoord*`/`glMultiTexCoord*`/`glSecondaryColor*`/`glIndex*`
immediate-mode entry points, the whole matrix stack
(`glMatrixMode`, `glLoadIdentity`, `glPushMatrix`, `glPopMatrix`,
`glMultMatrix*`, `glLoadMatrix*`, `glTranslated`, `glRotated`, `glScaled`,
`glOrtho`, `glFrustum`), lighting and materials (`glLight*`, `glMaterial*`,
`glLightModel*`), fog (`glFog*`), texture environment and generation
(`glTexEnv*`, `glTexGen*`), display lists (`glNewList`, `glCallList`,
`glEndList`, `glDeleteLists`, `glIsList`, `glListBase`, `glLoadName`,
`glPushName`, `glPopName`, `glInitNames`), evaluators (`glMap1*`, `glMap2*`,
`glMapGrid1*`, `glMapGrid2*`, `glEvalCoord1*`, `glEvalCoord2*`,
`glEvalPoint1/2`, `glEvalMesh1/2`), selection & feedback
(`glSelectBuffer`, `glFeedbackBuffer`), accumulation (`glAccum`, `glClearAccum`),
pixel transfer (`glPixelMap*`, `glPixelTransfer*`, `glPixelZoom`),
`glRasterPos*`, `glWindowPos*`, `glDrawPixels`, `glCopyPixels`,
client-side vertex arrays (`glEnableClientState`, `glDisableClientState`,
`glVertexPointer`, `glColorPointer`, `glNormalPointer`, `glTexCoordPointer`,
`glIndexPointer`, `glEdgeFlagPointer`, `glSecondaryColorPointer`,
`glFogCoordPointer`, `glInterleavedArrays`), `glPolygonStipple`,
`glGetPolygonStipple`, `glShadeModel`, `glAlphaFunc`, `glClipPlane`,
`glGetClipPlane`, `glAreTexturesResident`, `glPrioritizeTextures`,
`glClientActiveTexture`, `glPassThrough`, `glGetMap*`, `glGetLight*`,
`glGetMaterial*`, `glGetTexEnv*`, `glGetTexGen*`.

One partial exception, stated precisely: **`glLineStipple` exists as a no-op**
at `src/gl/gl_raster.cpp:274` (`void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }`)
and is exported through the bonus path at `src/gl/gl_c_exports.cpp:2810`, but it is
**not declared** in `include/tglmt/gl46.h` and it does nothing.

### 2.5 Modern pipeline — EXISTS

Implemented in `src/gl/`:
`gl_program.cpp` (1 795), `gl_shader.cpp` (96), `gl_uniform.cpp` (2 083),
`gl_vertex.cpp` (389), `gl_draw.cpp` (596), `gl_apple_draw.cpp` (4 649),
`gl_texture.cpp` (3 506), `gl_sampler.cpp` (75), `gl_framebuffer.cpp` (1 217),
`gl_raster.cpp` (275), `gl_compute.cpp` (489), `gl_sync_query.cpp` (404),
`gl_tess_xfb.cpp` (291), `gl_readpixels_dsa.cpp` (644), `gl_get.cpp` (1 108),
`gl_get_extra.cpp` (1 427), `gl_buffer.cpp` (1 030), plus 9 `gl_ext_*.cpp`
extension files and `gl_packed_robust.cpp` (206).

### 2.6 Internal IR — **PARTIAL / NAMING ONLY**

There is no IR module, no IR type, no IR file. The word "IR" in the source is a
label for the **deferred materialisation** strategy inside
`src/gl/gl_apple_draw.cpp`:
* `:2352` "IR pipeline-key cache" → `Context::PipelineKey` (`include/tglmt/Context.h:1015-1083`).
* `:2409` "Encoder IR (deferred materialization): 1 encoder cho N draw liên tiếp" →
  `Context::pendingEncoder/pendingTarget/pendingPipeline` (`Context.h:883-885`).
* `:2614` "IR dirty-check", `:2817` "IR staging cache", `:3719` "IR deferred: ring buffers".

So the "IR" today is **an encoder-reuse + pipeline-cache strategy**, not an
intermediate representation. There is no place where GL semantics are lowered
into a data structure that is then lowered into Metal — the lowering is direct
and inline, exactly as decision D-014 requires ("dịch thẳng OpenGL → Metal
trong một tầng"). That is *consistent* with the current architecture decision,
but it means there is **no natural home** for a legacy fixed-function engine
that would need to lower legacy state into something the Metal path can consume.

### 2.7 Shader translation — EXISTS

`src/gl/glsl_to_msl.cpp` (12 627 LOC) implements `ConvertGLSLtoMSL()`
(declared `include/tglmt/GLSLConverter.h:187-188`). It is a **token/string
rewriter**, not a compiler front end — `docs/PROJECT_ARCHITECTURE.md:164-183`
documents the pass pipeline and `include/tglmt/GLSLConverter.h:2-25` documents
the supported GLSL subset. Anything outside the subset fails loudly
(`GLSLConvertResult::ok = false`, `GLSLConverter.h:100`).

**Legacy-relevant limitation:** the converter targets **GLSL 4.60** only.
`include/tglmt/GLSLConverter.h:2` says "GLSL ES 3.0/GL 4.60 (subset có tài liệu)";
there is **no GLSL 1.10 / 1.20 / 1.30 / 1.40 / 1.50 / 3.30 support** — i.e. none
of the shader languages required for GL 1.0–3.3 (`project/docs/SPEC.md:79-90`
lists the required GL↔GLSL mapping). Also there is **no `gl_FragColor`/`gl_FragData`
compatibility path for the legacy fragment model** — the converter only maps
`gl_FragColor`/`gl_FragData` as a special case of the modern pipeline
(`src/gl/glsl_to_msl.cpp:9660-9691`).

### 2.8 Metal backend — EXISTS

* **Abstraction**: `include/tglmt/MetalInterface.h` — pure C++17, no GL types
  (`:128-129` explains the deliberate absence of GL headers), namespace
  `tglmt::metal` (`:13`). `IDevice` (`:510`) has **83 virtual methods**
  (measured: `awk '/^class IDevice \{/,/^\};/' … | grep -c virtual` → 83).
  Interfaces: `ISamplerState` (`:221`), `IDepthStencilState` (`:225`),
  `IBuffer` (`:246`), `ITexture` (`:255`), `IEncoder` (`:301`), `ILibrary` (`:313`),
  `IRenderPipeline` (`:326`), `IRenderTarget` (`:334`), `IRenderEncoder` (`:371`),
  `IComputePipeline` (`:462`), `IComputeEncoder` (`:466`).
  Value types: `PixelFormat` (`:15-50`), `CompressedBlockInfo` (`:55`),
  `AttachmentBlend` (`:130`), `SamplerDesc` (`:140`), `ScissorRect` (`:165`),
  `CustomAttrib` (`:175`), `StencilFaceDesc` (`:186`), `StencilDesc` (:`195`),
  `PipelineOpts` (`:199`), `Viewport` (`:239`), `ClearColor` (`:240`),
  `DrawTrace` (`:243`).
  Factory: `std::shared_ptr<IDevice> CreateDevice(const std::string& backend, LogFn log)` (`:924`).
* **Apple backend**: `src/metal/AppleMetalBridge.mm` (3 494 LOC), Objective-C++
  with ARC (`CMakeLists.txt:61` sets `-fobjc-arc`), implements every `IDevice`
  virtual against `id<MTL*>`; entry point `CreateAppleDevice(LogFn)` at
  `src/metal/AppleMetalBridge.mm:3472` (declared `:28`).
* **Null backend**: `src/metal/NullMetalBackend.cpp` (79 LOC) — `NullBuffer`
  (`:7`), `NullTexture` (`:19`), `NullEncoder` (`:29`), `NullDevice` (`:45`),
  and the factory `CreateDevice` (`:69-77`) which resolves `"null"`/empty to the
  null device, `"apple"` to `CreateAppleDevice` only when `TGLMT_APPLE_METAL` is
  defined (`:71-74`), and falls back to null for anything unknown (`:75-76`).
* **Selection**: `src/core/Context.cpp:56` `device = metal::CreateDevice(backend, CtxLog);`
  with an explicit null fallback at `:57-65` and the diagnostic tag
  `"apple(null-fallback)"` at `:64`.

### 2.9 Validation — **NO separate layer**

Error generation is inlined in each entry point, e.g.
`src/gl/gl_get.cpp:474` `if (!IsAcceptedPname(p)) c.errors.Record(0x0500);`.
The **only** shared validation artefact is the generated pname whitelist
`include/tglmt/gl46_pnames.h` (993 LOC), produced by `tools/gen_state_pnames.py`,
which merges (a) the Ch. 23 state table of `glspec46.core.pdf`
(cached at `docs/khronos/state_pnames_46.txt`), (b) the enums of the extensions
TGLMT advertises, and (c) the pnames that have a `case 0x…:` branch in
`src/gl/gl_get.cpp` / `src/gl/gl_get_extra.cpp`
(`tools/gen_state_pnames.py:11-27`).
**Implication for legacy:** there is no shared argument-validation helper
(e.g. no `ValidateEnum(GLenum, const GLenum*)`, no per-entry-point spec table).
A legacy engine must re-implement validation per entry point, exactly like the
modern layer does.

---

## 3. Thread / current-context model

| Fact | Evidence |
|---|---|
| The current context is a `thread_local` pointer | `src/core/Context.cpp:10` `thread_local Context* Context::tCurrent_ = nullptr;` (declared `include/tglmt/Context.h:1297`) |
| `MakeCurrent` sets it | `src/core/Context.cpp:120` `void Context::MakeCurrent(Context* ctx) { tCurrent_ = ctx; }` (declared `Context.h:635`) |
| `Current()` returns the thread's context, else a process-global fallback | `src/core/Context.cpp:114-119`: `if (tCurrent_) return *tCurrent_;` → `std::lock_guard<std::mutex> l(sMu_);` → `if (!sFallback_) sFallback_ = new Context("null");` → `return *sFallback_;` |
| The fallback context is created lazily, once, under a mutex | `src/core/Context.cpp:11-12` (`sFallback_`, `sMu_`) and `:116-118` |
| Every `gl*` reaches the context through `Context::Current()` | `include/tglmt/Context.h:5` ("mọi gl* đều đi qua Context::Current()") |
| The `Renderer` façade calls `MakeCurrent` itself | `src/core/Renderer.cpp:53` `Context::MakeCurrent(impl_->ctx.get());` (inside `BeginFrame`; also `:81` and `:97`) |
| The GLFW shim keeps one `Renderer` per window and re-`MakeCurrent`s on the calling thread | `src/gl/glfw_shim.cpp:25` `std::unique_ptr<Renderer> renderer;`, `:59`, `:77`, `:93`, `:330` |
| Threading contract | "1 context / 1 thread tại 1 thời điểm" (`include/tglmt/GLFWShim.h:9`); the context is **not** internally synchronised — `docs/PROJECT_ARCHITECTURE.md:271` lists this as limitation L5. |

**Consequence for the mission:** there is **no context-share group**
(shared display lists / shared textures), no `Context` creation API beyond the
C++ constructor (`include/tglmt/Context.h:1291`), and **no way to ask a context
which version/profile it is** — the version is a compile-time constant
(`src/gl/gl_get.cpp:140-141`, `src/gl/gl_error_string_debug.cpp:17`).
For a 1.0–4.6 mission this is the single most disruptive structural change:
a context must become able to *be* a 1.5-compat context or a 4.6-core context.

---

## 4. The generated-file pipeline

### 4.1 Which files are auto-generated, and by which tool

| Generated artefact | Generator | Marker |
|---|---|---|
| `include/tglmt/gl46_types.h` (1 871 LOC) | `tools/gen_gl_headers.py` | `:1` `// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.` |
| `include/tglmt/gl46.h` (716 LOC) | `tools/gen_gl_headers.py` | `:1` same banner; `:2-3` state 657 core + 42 compat-only |
| `src/gl/gl_dispatch_gen.cpp` (714 LOC) | `tools/gen_gl_headers.py` | `:1` `// AUTO-GENERATED từ docs/khronos/gl.xml.` |
| `src/gl/gl_c_exports.cpp` (3 533 LOC) | `tools/gen_c_exports.py` | `:1` `// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.` |
| `src/gl/gl_stubs_gen.cpp` (6 LOC) | `tools/gen_stubs.py` | `:1` `// AUTO-GENERATED bởi tools/gen_stubs.py …` (currently only the banner → **zero stubs**) |
| `src/gl/gl_ext_aliases_gen.cpp` (1 391 LOC) | `tools/gen_ext_aliases.py` | `:1` `// AUTO-GENERATED bởi tools/gen_ext_aliases.py — KHÔNG SỬA TAY.` |
| `include/tglmt/gl46_pnames.h` (993 LOC) | `tools/gen_state_pnames.py` | `:1` `// AUTO-GENERATED bởi tools/gen_state_pnames.py — KHÔNG SỬA TAY.` |
| `tests/truth/api_manifest.json` | `tools/truth_manifest.py` | — (JSON, no banner) |
| `docs/OPENGL_46_API_INVENTORY.md`, `docs/OPENGL_46_API_COVERAGE.md` | `tools/gen_api_docs.py` | — |
| `docs/TRUTH_MANIFEST.md`, `docs/EXTENSION_MATRIX.md` | `tools/truth_manifest.py --write`, `tools/ext_matrix.py --write` | — |
| `cmake/tglmt_buildinfo.h.in` → **generated** `${CMAKE_BINARY_DIR}/generated/tglmt_buildinfo.h` (not in the source tree) | CMake `configure_file` | `CMakeLists.txt:48-49`; the template's own header says "SINH TỰ ĐỘNG bởi CMake … KHÔNG sửa tay" |

The generator inputs are: `docs/khronos/gl.xml` (the vendored Khronos registry),
`docs/khronos/glspec46.core.pdf` (state table, cached in
`docs/khronos/state_pnames_46.txt`), `src/gl/gl_error_string_debug.cpp` (the
advertised-extension list `kExts[]`), and `src/gl/*.cpp` (to detect which
entry points are already implemented).

### 4.2 The documented invariants

| Invariant | Where stated | Current value (verified) |
|---|---|---|
| `CORE_FUNCTION_COUNT = 657` | `tools/gl46_catalog.py:57` | `python3 tools/gl46_catalog.py` → `OpenGL 4.6 Core Profile commands: 657 (expected 657)` ✔ |
| `COMPAT_ONLY_FUNCTION_COUNT = 42` | `tools/gl46_catalog.py:63` | → `Compatibility-profile-only commands: 42 (expected 42)` ✔ |
| The two counts are asserted by the tool itself | `tools/gl46_catalog.py:189-190` | `assert len(core) == CORE_FUNCTION_COUNT`, `assert len(compat) == COMPAT_ONLY_FUNCTION_COUNT` |
| `gen_gl_headers.py --check` re-asserts both | `tools/gen_gl_headers.py:111-114` | same asserts |
| The derivation follows Khronos `reg.py` PASS-1 semantics (profile-aware `<require>`/`<remove>`, version-ordered) | `tools/gl46_catalog.py:33-46` | cross-checked against the official `glcorearb.h` (`:45-46`) |
| The catalog is the **single** source of truth for all three former copies | `tools/gl46_catalog.py:9-17` | `tools/gen_gl_headers.py:14`, `tools/gen_stubs.py:10`, `tools/truth_manifest.py:34` all `import gl46_catalog` |
| `gl46.h` declares core + compat-only, and the counts are exposed at runtime | `include/tglmt/gl46.h:711-715` | `GetCoreFunctionCount()`, `GetCompatOnlyFunctionCount()`, `GetCoreFunctionName()`, `GetCoreFunctionTableSize()`, `tglmt_num_extensions()` |
| The C-export table size is bounded | `include/tglmt/GlProfile.h:12` `kGlProfileMax = 768`; `tools/gen_c_exports.py:107-108` raises if exceeded | current 701 → **67 slots of headroom** |
| The stub generator must report zero missing | `tools/gen_stubs.py:41-43` | `src/gl/gl_stubs_gen.cpp` is 6 lines → **0 stubs** |
| The manifest joins registry ⇄ advertised ⇄ exported symbol ⇄ implementation ⇄ test | `tools/truth_manifest.py:6-16` | `tests/truth/api_manifest.json` |
| Generated wrappers are not "the implementation of record" | `tools/truth_manifest.py:51-58` | `GENERATED = {gl_c_exports.cpp, gl_ext_aliases_gen.cpp, gl_stubs_gen.cpp, gl_dispatch_gen.cpp}` |

**Where the invariants live in the tree today:** `tools/gl46_catalog.py` is the
only derivation; `tools/gen_gl_headers.py`, `tools/gen_stubs.py` and
`tools/truth_manifest.py` all delegate to it. `docs/OPENGL_46_API_INVENTORY.md`
and `docs/OPENGL_46_API_COVERAGE.md` are generated *from* the manifest by
`tools/gen_api_docs.py` (`:21-24`).

**Important consequence for the mission:** every one of these generators is
hard-wired to "OpenGL 4.6 Core" as the *only* axis. `tools/gl46_catalog.py:82-83`
filters features to `GL_VERSION_*` up to `(4,6)`; `parse_core_commands` returns
one flat `{name: version}` map with **no profile and no version-range axis**.
Producing a version × profile matrix (GL 1.0 → 4.6, Core + Compatibility)
requires replacing this flat map with a *feature model*, i.e. the derivation in
`project/docs/SPEC.md:61-77`.

---

## 5. Build system

### 5.1 CMake (`CMakeLists.txt`, 160 lines)

| Item | Evidence |
|---|---|
| Language / standard | `:1-5` `cmake_minimum_required(VERSION 3.20)`, `project(TGLMT VERSION 0.1.0 LANGUAGES C CXX)`, `CMAKE_CXX_STANDARD 17`, `CMAKE_CXX_STANDARD_REQUIRED ON` |
| **Option** `TGLMT_APPLE_METAL` (default OFF) | `:8`; effect `:56-62` — enables `OBJCXX`, appends `src/metal/AppleMetalBridge.mm`, defines `TGLMT_APPLE_METAL=1`, sets `-fobjc-arc`; `:83-85` and `:100-102` link `Metal`/`Foundation`/`QuartzCore` |
| **Option** `TGLMT_BUILD_TESTS` (default ON) | `:9`; effect `:105-159` |
| **Option** `TGLMT_BUILD_SHARED` (default ON) | `:10`; effect `:89-103` → target `tglmt_shared` (output name `tglmt`, `VERSION 0.1.0`, `SOVERSION 0`, default visibility) |
| **Option** `TGLMT_FORCE_DIAG` (default OFF) | `:11-14` → `add_compile_definitions(TGLMT_FORCE_DIAG=1)` |
| Linker acceleration | `:20-24` uses `ld.lld` when found |
| Build identity embedded in `GL_VERSION` | `:30-48` — `git rev-list --count HEAD` + `git rev-parse --short HEAD` + dirty flag → the template `cmake/tglmt_buildinfo.h.in` → the **generated** `${CMAKE_BINARY_DIR}/generated/tglmt_buildinfo.h`; `:49` adds the generated dir to the include path |
| Source globs | `:51-53` `src/core/*.cpp`, `src/gl/*.cpp` (with `CONFIGURE_DEPENDS`) |
| Object library (compile once, link twice) | `:68-70` `add_library(tglmt_objs OBJECT …)`, `POSITION_INDEPENDENT_CODE ON`; `:80` `add_library(tglmt STATIC $<TARGET_OBJECTS:tglmt_objs>)`; `:90` the shared twin. Rationale in `:64-67`. |
| `glsl_to_msl.cpp` compiled at `-O1` | `:78` (rationale and measured cost in `:72-77`) |
| **`install()` / export rules** | **absent** — `grep -n "install" CMakeLists.txt` → no match |
| XCFramework target | **absent** — no `xcodebuild -create-xcframework` anywhere in `CMakeLists.txt` or `scripts/` (D-016 in `project/docs/SPEC.md:150` is still open) |

### 5.2 CTest registration

`CMakeLists.txt:105-133`:

```
:107-109  file(GLOB TGLMT_TESTS CONFIGURE_DEPENDS
              tests/unit/*.cpp tests/integration/*.cpp tests/regression/*.cpp
              tests/api/*.cpp tests/conformance/*.cpp tests/shaders/*.cpp)
:116-118  add_library(tglmt_test_pch OBJECT EXCLUDE_FROM_ALL tests/pch_stub.cpp)
          target_precompile_headers(tglmt_test_pch PRIVATE <tglmt/gl46.h> <tglmt/Context.h>)
:119-133  foreach(T ${TGLMT_TESTS})
              get_filename_component(NAME ${T} NAME_WE)
              add_executable(${NAME} ${T})
              target_link_libraries(${NAME} PRIVATE tglmt)
              target_precompile_headers(${NAME} REUSE_FROM tglmt_test_pch)
              target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/tests)
              target_compile_definitions(${NAME} PRIVATE
                  TGLMT_CORPUS_DIR="…" TGLMT_ROOT="…")
              add_test(NAME ${NAME} COMMAND ${NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
          endforeach()
```

* **185 CTest targets** = 92 unit + 77 integration + 13 regression + 1 api + 1 conformance + 1 shaders.
  Verified directly against an existing build tree: `cd build-work && ctest -N` → `Total Tests: 185`.
  (The *pass* status of those 185 was **not** re-measured here — see §8.)
* The `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` at `:132` is deliberate (comment `:128-131`):
  10 tests use relative fixture paths and would otherwise silently SKIP.
* `truth_probe` is added as an executable **without** `add_test`
  (`:136-137`, comment `:135` "not a CTest gate").
* The shared PCH (`:116-118`) is why 185 tests still build in reasonable time;
  `include/tglmt/Context.h` is in it, which is why the `Context.h → GLSLConverter.h`
  coupling (§ `SUBSYSTEM_DEPENDENCY_GRAPH.md` §6.3) is paid for by every test.

### 5.3 Benchmark targets

`CMakeLists.txt:139-159`: glob `benchmarks/compilation/*.cpp`, `benchmarks/compilation/*.mm`,
`benchmarks/runtime/*.cpp` → one executable per file, linked to `tglmt`;
`.mm` files are compiled as OBJCXX with `-fobjc-arc` and link `Metal`/`Foundation`
(`:150-158`). They are **not** CTest gates (comment `:140-141`).
Measured: `benchmarks/compilation/bench_compile.cpp` 208 LOC,
`benchmarks/compilation/bench_upload.mm` 132 LOC,
`benchmarks/runtime/bench_runtime.cpp` 240 LOC, plus
`benchmarks/compare/bench_compare.py` 185 LOC and `bench_median.py` 98 LOC,
and JSON baselines in `benchmarks/{baseline,optimized}/`.

### 5.4 Makefile (277 lines)

`Makefile` is the **iOS packaging** entry point, not a general build system.
Targets: `ipa` (default, `:227-234`), `ipa-xcode` (`:196`), `ipa-manual` (`:219`),
`ipa-ios15` (`:237`), `app-binary` (`:209`), `xcode-project` (`:176`), `xcode-app` (`:179`),
`lib-ios` (`:127`), `lib-mac` (`:137`), `app-mac` (`:141`), `test` (`:144`),
`audit` (`:149`), `truth` (`:167`), `probe` (`:158`), `scan-gaps` (`:163`),
`check-tools` (`:102`), `icons` (`:121`), `verify` (`:242`), `diagnose` (`:251`).
Variables: `DEPLOY_TARGET ?= 16.0` (`:29`), `BUNDLE_ID ?= com.tglmt.aquarium` (`:30`),
`BUILD_METHOD ?= xcode` (`:32`), `CONFIG ?= Release` (`:33`).

---

## 6. iOS packaging path

```
Makefile:ipa  (Makefile:227)
  └─ BUILD_METHOD=xcode → Makefile:ipa-xcode (:196)
       ├─ xcode-app (:179)  ─ requires lib-ios (:127) + xcode-project (:176) + icons (:121)
       │    ├─ lib-ios (:127-135)
       │    │    cmake -S . -B build-iphoneos -DTGLMT_APPLE_METAL=ON
       │    │          -DCMAKE_TOOLCHAIN_FILE=apps/aquarium/ios_toolchain.cmake
       │    │          -DIOS_DEPLOYMENT_TARGET=$(DEPLOY_TARGET) -DTGLMT_BUILD_TESTS=OFF
       │    │    → build-iphoneos/libtglmt.a
       │    ├─ xcode-project (:176-177) → scripts/configure_ios.sh
       │    │    → cmake -G Xcode on apps/aquarium (standalone project,
       │    │      apps/aquarium/CMakeLists.txt:18-30 requires -DTGLMT_INCLUDE_DIR + -DTGLMT_LIB)
       │    │    → apps/aquarium/build-ios/AquariumBench.xcodeproj
       │    ├─ icons (:121-123) → scripts/gen_ios_icons.py → apps/aquarium/ios_assets
       │    └─ xcodebuild -project … -sdk iphoneos -configuration Release
       │         CODE_SIGNING_ALLOWED=NO …  (Makefile:182-193)
       └─ scripts/build_ipa_from_app.sh (Makefile:200) → apps/aquarium/AquariumBench.ipa
  └─ BUILD_METHOD=manual → Makefile:ipa-manual (:219)
       ├─ app-binary (:209) → clang++ -arch arm64 -miphoneos-version-min=…
       │    apps/aquarium/aquarium.cpp + apps/aquarium/ios_shell.mm + libtglmt.a
       └─ scripts/build_ipa_trollstore.sh (Makefile:222)
```

Supporting pieces:

| Path | Role |
|---|---|
| `apps/aquarium/ios_toolchain.cmake` | `CMAKE_SYSTEM_NAME iOS`, `CMAKE_OSX_SYSROOT iphoneos`, `CMAKE_OSX_ARCHITECTURES arm64`, `CMAKE_OSX_DEPLOYMENT_TARGET` from `-DIOS_DEPLOYMENT_TARGET`, `-miphoneos-version-min` in every flag set (comment: without it the binary records `minos=SDK` and iOS refuses to register the app → TrollStore error 181). |
| `apps/aquarium/ios_shell.mm` (454 LOC) | UIKit shell + `CAMetalLayer` present. |
| `apps/aquarium/macos_shell.mm` (167 LOC) | AppKit shell. |
| `apps/aquarium/ios_Info.plist` / `macos_Info.plist` | Bundle metadata; `MinimumOSVersion` must match the binary (`Makefile:243-247` runs `scripts/verify_ipa_trollstore.py`; that script reads `platform`/`minos`/`sdk` with `otool -l` — `scripts/verify_ipa_trollstore.py:83`, and `scripts/build_ipa_trollstore.sh:29-31` does the same for the hand-built bundle). |
| `scripts/verify_ipa_trollstore.py` | Post-build IPA verification (bundle id, minos, icons). |
| `scripts/build_ipa_from_app.sh`, `scripts/build_ipa_trollstore.sh` | Two IPA packers (Xcode-made `.app` preferred; hand-built fallback). |
| `scripts/configure_ios.sh` | Generates the Xcode project; fails early if `build-iphoneos/libtglmt.a` is missing. |
| `scripts/gen_ios_icons.py` | Icon generation (missing icon ⇒ LaunchServices error 181). |
| `scripts/fetch_metal_cpp.sh` | Fetches Apple's `metal-cpp` headers (not yet vendored — `third_party/metal-cpp/` does **not** exist; `ARCHITECTURE.md:39-40` describes it as "thư mục chờ"). |
| `build-ios-pkg/` | A **pre-existing iOS device build tree**: `CMakeCache.txt` shows `CMAKE_SYSTEM_NAME=iOS`, `CMAKE_OSX_ARCHITECTURES=arm64`, `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`, `TGLMT_APPLE_METAL=ON`, `TGLMT_BUILD_TESTS=OFF`, `TGLMT_BUILD_SHARED=ON`; it contains `libtglmt.a` (4 454 120 B) and `libtglmt.dylib` (2 909 824 B). Note the deployment target here is **14.0**, while `Makefile:29` uses **16.0** — two different baselines coexist in the tree. |
| `apps/aquarium/DEPLOY_IOS.md` | Step-by-step device deployment. |

**Gap for the mission:** the iOS path produces a **dylib + static lib**, not an
**XCFramework** (`project/docs/SPEC.md:150`, D-016). There is no
`xcodebuild -create-xcframework` invocation anywhere in the repo. For a
1.0–4.6 SDK this is a packaging deliverable that does not yet exist.

---

## 7. Risks and gaps for the 1.0–4.6 mission

Ordered by how much they block the legacy (1.0–2.1) work.

### R1 — The legacy fixed-function engine does not exist (349 missing entry points)

*Evidence*: §2.4 (7 independent checks) + the computed 349-entry-point gap.
*Impact*: GL 1.0–2.1 support is not "a compatibility profile of the existing
core" — it is a **new subsystem** (immediate mode, matrix stack, lighting,
materials, fog, texenv, texgen, display lists, evaluators, selection/feedback,
accumulation, pixel transfer, client-side vertex arrays).
*Needed*: a new source directory (e.g. `src/gl/legacy/`), a new state container
(matrix stacks + per-vertex current attributes + lighting/material/fog/texenv
tables), a lowering path from that state into the modern draw path, and ~349
entry points with per-call validation.

### R2 — The context has no version/profile model

*Evidence*: `src/gl/gl_get.cpp:140-148` hard-codes `MAJOR_VERSION=4`,
`MINOR_VERSION=6`, `CONTEXT_PROFILE_MASK=0x00000001` (core bit only);
`src/gl/gl_error_string_debug.cpp:17` hard-codes the `GL_VERSION` string;
`tests/api/test_api_surface.cpp:79-80` asserts the compatibility bit is **absent**.
*Impact*: a legacy app that checks `GL_CONTEXT_PROFILE_MASK` (or the version
string) to choose the compatibility path will take the wrong branch, and any
test written against the current behaviour will break the moment a
compatibility profile is added.
*Needed*: a version/profile field on `Context`, a creation API that takes
`{major, minor, profile}`, per-profile `GL_VERSION` / `GL_SHADING_LANGUAGE_VERSION`
strings, and per-profile behaviour gates on every entry point.

### R3 — The catalog derivation is single-axis (version only, no profile range)

*Evidence*: `tools/gl46_catalog.py:90-118` `parse_core_commands()` returns a
flat `{name: introduced_version}` map; `:121-146` `parse_compat_only_commands()`
returns a second flat map; there is no notion of "this command exists in
versions 1.0–3.1 but not 3.2+".
*Impact*: the whole generator family (`gen_gl_headers.py`, `gen_stubs.py`,
`gen_c_exports.py`, `gen_ext_aliases.py`, `gen_state_pnames.py`,
`truth_manifest.py`, `gen_api_docs.py`) consumes that flat map. Adding
GL 1.0–4.6 × {core, compatibility} requires a **feature model** (per version:
introduced / inherited / deprecated / removed / core-only / compat-only), exactly
as `project/docs/SPEC.md:61-77` requires. This is the single largest
tooling change of the mission and it is *upstream* of every other change.

### R4 — The generator headroom is finite (67 slots)

*Evidence*: `include/tglmt/GlProfile.h:12` `kGlProfileMax = 768`;
`tools/gen_c_exports.py:107-108` aborts above 768; current size is 701
(`src/gl/gl_c_exports.cpp:3532`).
*Impact*: adding 349 legacy entry points → 1 050 profiled slots > 768 ⇒ the
generator **hard-fails**. The cap must be raised (and the per-call timer cost
re-measured: `include/tglmt/GlProfile.h:6` states ~50 ns/call).
*Also*: `tools/gen_c_exports.py:107-108` and the comment at `include/tglmt/GlProfile.h:11`
("hiện tại 701") both hard-code the current number and will need updating.

### R5 — The shader translator has no legacy GLSL

*Evidence*: `include/tglmt/GLSLConverter.h:2` targets "GLSL ES 3.0/GL 4.60";
the file's documented subset (`:6-25`) covers `layout(location=)`, `uniform`,
`in/out`, `texture()`, `discard` — **not** `attribute`/`varying` (GLSL 1.10–1.30),
not `gl_FragColor` as a normal path, not `texture2D`/`textureCube` (GLSL 1.x
texture functions), not `#version 110/120/130/140/150`.
*Impact*: GL 2.0/2.1 programmable path (GLSL 1.10/1.20) and the GL 1.x
`ARB_vertex_program`/`ARB_fragment_program` assembly path have no translator.
`project/docs/SPEC.md:79-90` requires GL↔GLSL for all versions.
*Needed*: either a compatibility front-end that rewrites legacy GLSL into the
modern subset the converter already accepts (a much cheaper path than teaching
the converter two more language generations), or a second converter.

### R6 — The state model has no room for fixed-function state

*Evidence*: `include/tglmt/StateTracker.h` models caps, blend, depth, stencil,
viewport/scissor, raster, pixel-store and bindings — there is **no** field for
light sources, material, fog, texture environment, texture generation, or the
current matrix. `Context::ExtState` (`include/tglmt/Context.h:570-631`) has
`texEnvF/texEnvI/texGenF/texGenI` maps (`:582-585`) that are **written by the
extension entry points but never read by the draw path** — they are bookkeeping,
not state.
*Impact*: the legacy engine needs its own state container, and it must be
lowered into the Metal draw path (`src/gl/gl_apple_draw.cpp`) — the modern
pipeline's pipeline-key cache (`include/tglmt/Context.h:1015-1083`) has no slot
for "the legacy program is using these texenv/lighting parameters".
*This is the single hardest architectural problem in the mission*: legacy
fixed-function state is *per-draw* state, and the current pipeline-key model
assumes all per-draw state is expressible as a pipeline descriptor.

### R7 — No separate validation layer; validation is duplicated per entry point

*Evidence*: errors are raised inline (`src/gl/gl_get.cpp:474` etc.); the only
shared artefact is `include/tglmt/gl46_pnames.h` (generated).
*Impact*: 349 new entry points each need their own enum/value/operation checks.
Without a shared validation helper, this is ~349 hand-written validation blocks
with a high chance of divergence from spec.

### R8 — `GetProcAddress` is a hand-written, incomplete table

*Evidence*: `src/gl/glfw_shim.cpp:367-406` — ~70 entries, hand-maintained,
returns `nullptr` for anything else (`:404-405`).
*Impact*: a legacy application that resolves entry points by name (the normal
pattern for a 1.0-era app, and LWJGL's pattern) will get `nullptr` for
`glBegin`, `glMatrixMode`, `glTexEnvf`, … — i.e. an immediate crash at load
time rather than a graceful error.
*Needed*: the table must become generated from the same catalog as
`gl_c_exports.cpp` (which already has every symbol address).

### R9 — The `include/tglmt/` directory mixes four layers

*Evidence*: see `docs/SUBSYSTEM_DEPENDENCY_GRAPH.md` §6.4.
*Impact*: adding a legacy layer plus a versioned API surface to a directory
that already mixes core / Metal / shader / GL-helper / façade headers will make
the public surface impossible to reason about, and there is **no `install()`
rule** to define what "public" means.
*Needed*: split by layer before publishing an SDK.

### R10 — The module-level cycle `src/core` ↔ `src/gl`

*Evidence*: `docs/SUBSYSTEM_DEPENDENCY_GRAPH.md` §6.2 —
`src/core/Renderer.cpp:5,98,99` → `src/gl/gl_readpixels_dsa.cpp:89` and
`src/gl/gl_error_string_debug.cpp:10` → back into `tglmt/Context.h`.
*Impact*: low today, but a legacy engine that lives in `src/gl/` and whose
state the core must query would turn this into a link-order problem.

### R11 — Documentation and the "4.6 core" claim are baked into generated artefacts

*Evidence*: `README.md:5` "OpenGL 4.6 Core"; `ARCHITECTURE.md:6` "698 decl";
`COVERAGE.md:5-21` "698/698"; `docs/PROJECT_ARCHITECTURE.md:16`; and the
generated `docs/OPENGL_46_API_INVENTORY.md` / `OPENGL_46_API_COVERAGE.md`.
Note that `README.md:14` and `ARCHITECTURE.md:6` still quote **698**, while the
catalog now says **657** — the docs are already partly stale relative to the
code.
*Impact*: any change to the catalog regenerates these files; the mission's
reporting must move to a version × profile matrix.

### R12 — No XCFramework / SDK packaging

*Evidence*: no `install()` in `CMakeLists.txt`; no `xcodebuild -create-xcframework`
in the tree. D-016 (`project/docs/SPEC.md:150`) requires dylib + XCFramework.

### R13 — Build-directory drift (stale artefacts in the tree)

*Evidence*: `build-ios-pkg/CMakeCache.txt` has `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`
while `Makefile:29` uses 16.0; `build-iphoneos/`, `build-iphoneos-work/`,
`build-apple/`, `build-asan/`, `build-bench/`, `build-null/`, `build-work/`
all exist as committed working directories.
*Impact*: a build or a test run can silently use a stale tree. `scripts/build_and_test.sh`
regenerates the generated files before building (`:7-11`), so a build can also
**overwrite** the generated sources in the working tree — `project/AGENTS.md:50-51`
flags this explicitly.

---

## 8. What is NOT_VERIFIED in this document

| Item | Status |
|---|---|
| That the 185 tests currently pass | **NOT_VERIFIED** — no build or `ctest` run was performed in this phase (read-only discovery). `project/AGENTS.md:30` records `185/185 pass, ~44s` as of 2026-11-11; that is a *reported* figure, not re-measured here. |
| Behavioural correctness of any entry point | **NOT_VERIFIED** — this document makes no correctness claim; `docs/OPENGL_46_API_INVENTORY.md` and `reports/AUDIT_REPORT.md` carry the behavioural record. |
| Real-device (A11) behaviour | **NOT_VERIFIED** — no device log exists in the repo (`project/docs/SPEC.md:101`). |
| The `build-ios-pkg/` artefacts being current | **NOT_VERIFIED** — the directory exists with `arm64` iOS outputs, but it was not rebuilt here. |
| `third_party/metal-cpp/` | **absent** (directory does not exist); `scripts/fetch_metal_cpp.sh` exists but the tree has not been vendored. `ARCHITECTURE.md:39-40` describes it as a placeholder. |
| `docs/khronos/gl.xml` being byte-identical to the upstream registry | **NOT_VERIFIED** here; `project/AGENTS.md:73` states a sha256 but it was not recomputed in this phase. |

---

## 9. Where to look next

| Question | File |
|---|---|
| "What is the exact source layout?" | `docs/SOURCE_MAP.md` |
| "What depends on what?" | `docs/SUBSYSTEM_DEPENDENCY_GRAPH.md` |
| "What does the project claim about itself?" | `docs/PROJECT_ARCHITECTURE.md`, `README.md`, `ARCHITECTURE.md` |
| "What is implemented vs not?" | `docs/OPENGL_46_API_INVENTORY.md` (generated), `docs/OPENGL_46_API_COVERAGE.md` (generated) |
| "What are the known defects?" | `reports/AUDIT_REPORT.md` |
| "What is the mission scope?" | `project/docs/SPEC.md`, `project/docs/PLAN.md`, `project/docs/DECISIONS.md` |
