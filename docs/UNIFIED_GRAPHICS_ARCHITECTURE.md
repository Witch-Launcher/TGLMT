# UNIFIED GRAPHICS ARCHITECTURE — TGLMT

> How the OpenGL 1.0–2.1 **legacy fixed-function pipeline** and the OpenGL
> 2.0–4.6 **modern programmable pipeline** live in **one** GL context, on
> **one** Metal device, behind **one** draw path.
>
> Scope: PHASE 5 of `other/prompt.md` (§7 "REQUIRED ARCHITECTURE", §12
> "PHASE 5 — Architecture Design"). Companion documents:
> `docs/OPENGL_TO_METAL_ARCHITECTURE.md` (overview for new readers),
> `docs/IMPLEMENTATION_ROADMAP.md` (what to build, in what order),
> `docs/LEGACY_TO_METAL_MAPPING.md` (per-mechanism Metal strategy),
> `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` (per-stage entry-point plan).

---

## 0. How to read this document

### 0.1 Labels

Every claim carries one of three labels. **Do not mix them.**

| Label | Meaning |
|---|---|
| **SPEC** | What the OpenGL specification requires, with the section cited. |
| **CURRENT** | What the code in this repository does *today*, with `file:line`. |
| **PLAN** | What this document proposes to build. Not implemented. |

### 0.2 Evidence rules

* Every **CURRENT** claim cites `file:line`. Line numbers refer to the working
  tree as of this document; they were verified by reading the files.
* Numbers are **measured**, not estimated. Where a number comes from
  `data/version_inventory.json` or from counting generated code, the source is
  named.
* No claim of "conformant" is made anywhere. A feature is only *implemented*
  when §14.1 of `other/prompt.md` is satisfied (see §9).

### 0.3 Environment label

All current behaviour was verified on the **Null backend**
(`src/metal/NullMetalBackend.cpp`), i.e. environment label
`REFERENCE_ONLY` — **not** `A11_DEVICE`. See §8.3.

---

## 1. The problem this document solves

A Compatibility-profile context must be able to run **both**:

* a legacy program — `glBegin(GL_QUADS)` + `glVertex3f` + `glColor3f` +
  `glLightfv` + `glEnable(GL_LIGHTING)` + `glFogf` + `glTexEnvf`, and
* a modern program — `glUseProgram(p)` + `glBindVertexArray` +
  `glDrawElements`.

…**in the same context, at the same time, interleaved**, with the correct
result for each draw.

### 1.1 SPEC — the dispatch rule

OpenGL 2.1 Specification, §2.15 "Fixed-Function Vertex Transformations" (the
compatibility-profile rule):

> "If the program object has no vertex shader, or no program object is
> currently in use, the fixed-function method for processing vertices is used
> instead."

OpenGL 4.6 Specification §1.2.4 (Compatibility Profile):

> "…the compatibility profile of OpenGL continues to support the legacy
> OpenGL commands developed for such fixed-function hardware, although they are
> typically implemented by writing shaders which reproduce the operation of
> such hardware."

Two consequences that drive the whole design:

1. **There is no "legacy mode".** There is *one* draw path with *two* program
   sources. A context is not "legacy" or "modern"; a **draw** is.
2. The legacy behaviour is legally implemented **by writing a shader**. This is
   the licence for the generated-MSL approach in §4.

### 1.2 CURRENT — where the dispatch would happen

The dispatch point is the program lookup at the top of the draw path:

* `src/gl/gl_apple_draw.cpp:1774` — `bool AppleDrawGL(GLenum mode, GLsizei count, …)`
* `src/gl/gl_apple_draw.cpp:1787` — `GLuint prog = c.state.BoundProgram();`
* `src/gl/gl_apple_draw.cpp:1799-1804` — the lookup into `c.programs`:

```cpp
    } else {
        auto pit = c.programs.find(prog);
        if (pit == c.programs.end() || !pit->second.linked) {
            c.appleStats.noProgram++; return false;
        }
        prp = &pit->second;
    }
```

**CURRENT behaviour:** when no program is bound (`prog == 0`) the draw is
**dropped** and counted in `appleStats.noProgram`. This is the single branch
that §4 replaces. Everything below it — target selection, the pipeline-key
cache, encoder reuse, uniform staging, the ring allocator, depth/blend/stencil
encoding — is **program-agnostic** and is reused unchanged by the legacy path.

### 1.3 CURRENT — the legacy state that already exists

`Context::legacy` (`include/tglmt/Context.h:685`) holds the fixed-function
state that has been implemented so far:

* `LegacyState` (`include/tglmt/LegacyState.h:126`)
* `GLMatrix` — column-major 4×4 (`include/tglmt/LegacyState.h:20-75`)
* `MatrixStack` — depth 32 (`include/tglmt/LegacyState.h:79-103`)
* `LegacyVertex` — position/colour/normal/texcoord/texcoord1/secondary/index/
  edge flag (`include/tglmt/LegacyState.h:106-115`)
* `LegacyPrimitive` — mode + up to 4 vertex indices (`include/tglmt/LegacyState.h:119-123`)
* `immediateVertices` / `immediatePrimitives`
  (`include/tglmt/LegacyState.h:147-149`)

Implementation: `src/gl/gl_legacy_matrix.cpp` (26 entry points) and
`src/gl/gl_legacy_vertex.cpp` (83 entry points). Primitive assembly is
`AssemblePrimitives` at `src/gl/gl_legacy_vertex.cpp:100-162`.

**Gap:** the assembled primitives are never handed to Metal. `glEnd` only
calls `AssemblePrimitives` (`src/gl/gl_legacy_vertex.cpp:89-95`); the comment
at `src/gl/gl_legacy_vertex.cpp:93` says so explicitly. Status of the legacy
draw path today: **PARTIALLY_IMPLEMENTED**.

---

## 2. Layer map — the spec's Layer 1..8 against the real repository

The spec's eight layers (`other/prompt.md` §7) are a *reference* architecture,
not a mandate ("Đây là kiến trúc tham khảo, không phải yêu cầu áp dụng máy
móc"). This section maps them onto the files that exist, and marks what does
**not** exist as `MISSING`.

### 2.1 The map

| Layer | Spec name | Files that implement it (CURRENT) | Status |
|---|---|---|---|
| **L1** | OpenGL API Frontend | `include/tglmt/gl_c_api.h` (generated public C ABI, 1049 prototypes) · `src/gl/gl_c_exports.cpp` (generated 1:1 forwarders) · `src/gl/gl_ext_aliases_gen.cpp` (extension aliases) · `src/gl/gl_version_gate_gen.cpp` (gate table, 1048 entries) · `src/gl/gl_dispatch_gen.cpp` (name→version manifest, audit-only) · `src/gl/gl_context_api.cpp:17` `tglmtCreateContextAttribs`, `:55` `tglmtMakeCurrent`, `:78` `tglmtGetProcAddress` · `include/tglmt/VersionGate.h:58` `VersionGateAllows` · `src/core/VersionGate.cpp` `GateCheck` | **EXISTS** |
| **L2** | Context and State Machine | `include/tglmt/Context.h:634` `class Context` · `include/tglmt/StateTracker.h` + `src/core/StateTracker.cpp` (caps, blend×8, depth/stencil, viewport×16, scissor×16, raster, pixel-store, bindings) · `include/tglmt/ErrorTracker.h` + `src/core/ErrorTracker.cpp` (FIFO error queue) · `include/tglmt/ObjectRegistry.h` + `src/core/ObjectRegistry.cpp` (12 object kinds) · `src/core/Context.cpp:10` `thread_local Context* Context::tCurrent_`, `:115-120` `Current()`/`MakeCurrent()` | **EXISTS** |
| **L3** | Legacy Compatibility Engine | `include/tglmt/LegacyState.h:126` · `src/gl/gl_legacy_matrix.cpp` (26) · `src/gl/gl_legacy_vertex.cpp` (83) | **PARTIAL** — matrix stack, current vertex attributes, immediate-mode assembly only. **MISSING**: lighting/materials/shade model, fog, texture environment + texture generation, legacy rasterisation (alpha test, stipple, clip planes), raster/window position, pixel transfer + `glDrawPixels`/`glBitmap`, display lists, evaluators, selection + feedback, accumulation buffer, client-side vertex arrays, attribute stacks, and the **legacy draw path** itself. |
| **L4** | Modern OpenGL Pipeline | `src/gl/gl_program.cpp` (program/link/reflection) · `src/gl/gl_shader.cpp` · `src/gl/gl_vertex.cpp` · `src/gl/gl_draw.cpp` · `src/gl/gl_framebuffer.cpp` · `src/gl/gl_texture.cpp` · `src/gl/gl_buffer.cpp` · `src/gl/gl_compute.cpp` · `src/gl/gl_tess_xfb.cpp` (tessellation + XFB) | **EXISTS** (core surface complete — all 657 GL 4.6 Core commands have a non-stub implementation, measured in §7.2) |
| **L5** | Unified Intermediate Representation | **Partial, and it is not a *unified* IR.** What exists is a *deferred-materialisation* shadow of Metal state, not a command IR: `Context::PipelineKey` (`include/tglmt/Context.h:1041`) · `Context::pendingEncoder` / `pendingTarget` / `pendingPipeline` (`include/tglmt/Context.h:909-911`) · `Context::pending*` per-state shadows (`include/tglmt/Context.h:997-1023`) · `Context::UniformCache` (`include/tglmt/Context.h:1029-1036`) · `metal::DrawTrace` (`include/tglmt/MetalInterface.h:243`) | **PARTIAL**. **MISSING**: a command-stream representation; there is nothing today that can record a sequence of GL commands and replay it (needed by display lists — §6). `src/gl/gl_dispatch_gen.cpp` is a *manifest*, not a runtime IR. |
| **L6** | Shader Translation and Compilation | `include/tglmt/GLSLConverter.h` (the contract) · `src/gl/glsl_to_msl.cpp` (12 627 lines — a token/string rewriter with ~60 passes, **not** a compiler front end) · `src/gl/gl_program.cpp:770-806` `c.device->compileLibrary(...)` for VS/TCS/TES/FS/CS · `src/metal/AppleMetalBridge.mm:1783` `compileLibrary` | **PARTIAL**. **MISSING**: the **fixed-function MSL generator** (§4.2). GLSL version coverage is also partial — `src/gl/glsl_to_msl.cpp:8480-8488` accepts only the version strings `300 460 450 440 410 400 430 330 150 140 130 120 100`; **`110` (GLSL 1.10) is not in the list**, so GLSL 1.10 is rejected with "phiên bản GLSL không nhận diện". |
| **L7** | Metal Backend | `include/tglmt/MetalInterface.h` (`IDevice` + `IBuffer`/`ITexture`/`ILibrary`/`IRenderPipeline`/`IRenderTarget`/`IRenderEncoder`/`IComputeEncoder`) · `src/metal/AppleMetalBridge.mm` (3 494 lines, real Metal) · `src/metal/NullMetalBackend.cpp` (79 lines, trace-only) · `src/metal/AppleMetalBridge.mm:2661` `makeCustomPipeline` | **EXISTS** |
| **L7.1** | Window-system layer (sub-item of L7) | `apps/aquarium/ios_shell.mm:6,22,79-89` (real `CAMetalLayer` + `CADisplayLink`) · `include/tglmt/GLFWShim.h` + `src/gl/glfw_shim.cpp:91` `SwapBuffers` → `src/core/Renderer.cpp:77-92` `EndFrame` → `presentTarget` | **EXISTS but is app-local**, not part of the library. There is **no** library-level "config/framebuffer" selection API: `tglmtCreateContextAttribs` (`src/gl/gl_context_api.cpp:17`) takes only version/profile/flags, no pixel format, no depth/stencil bits, no double-buffer choice. **MISSING**: a real `TGLMTConfig`/framebuffer-description API. |
| **L8** | Validation and Compatibility Layer | Validation is **inlined per entry point** (`Context::Current().errors.Record(0x0500)` etc.) — there is no standalone validator object. Reusable artefacts that do exist: `include/tglmt/gl46_pnames.h` (generated pname whitelist) · `src/gl/gl_get.cpp:294` `CompatOnlyPname` (Core-profile pname rejection) · `src/gl/gl_version_gate_gen.cpp` (version/profile gate) · `tests/conformance/test_conformance_surface.cpp` (surface-level checks C1–C4) | **PARTIAL**. **MISSING**: a cross-layer consistency checker; a spec-reference differential harness inside the repo; pname-level version gating for the fixed-function pnames that are not yet implemented. |

### 2.2 The one structural gap that matters

Layers 1, 2, 4, 6, 7 and 7.1 are real and in production use (Minecraft runs on
this path). Layers 3 and 5 are the two that are thin, and they are exactly the
two the legacy programme needs:

* **L3** exists as a *state block* (`Context::legacy`) but has **no execution
  path**. It records; it never draws.
* **L5** exists as a *deferred-state shadow*, not as a command IR. That is
  sufficient for the modern path (which needs "did anything change?") and
  **insufficient** for display lists (which need "replay these commands").

The PLAN in §4 and §6 is to add the missing *execution* path without touching
the working modern path.

---

## 3. Design principles

| # | Principle | Why |
|---|---|---|
| P1 | **One draw path, two program sources.** The legacy path must not fork `AppleDrawGL`. | A fork duplicates the target/pipeline/encoder/hazard logic — the part of this codebase that has the most hard-won bug fixes (see the long comment history at `src/gl/gl_apple_draw.cpp:2022-2031`). |
| P2 | **The legacy program is a real `ILibrary` pair.** Not a special case in the pipeline builder. | `Context::PipelineKey` already keys on library identity; a special case would need a *new* key field and would risk cache collisions (§4.4). |
| P3 | **Compile-time shape vs run-time values.** Features that change the *shape* of the generated shader go in the program key; everything else is a uniform. | A light colour change must not recompile a shader. |
| P4 | **Fixed-function semantics are computed per vertex where the spec says so.** Lighting is per-vertex (GL 2.1 §2.14.1), fog is per-fragment (§3.10), the texture environment is per-fragment. | Computing them per-fragment would be a *different* result, not an approximation. |
| P5 | **Do not rewrite the project.** Every change is additive and locally reversible. | `other/prompt.md` §7: "Không được viết lại toàn bộ dự án chỉ vì muốn thay đổi cấu trúc." |
| P6 | **The legacy path reuses the modern coordinate conventions.** Same `kZConv`, same viewport conversion, same front-face inversion. | Otherwise `glDepthRange`, `glReadPixels` depth values and `glClipControl` semantics all shift (§4.8). |

---

## 4. The legacy draw path — design

This is the core of the document.

### 4.1 State → uniform

**CURRENT.** Fixed-function state lives in `Context::legacy`
(`include/tglmt/Context.h:685`). It is *not* in `StateTracker` — that is a
deliberate, recorded decision (`docs/DECISIONS.md` D-004): `StateTracker` is
the modern path's state machine and is in production use; adding legacy state
to it would risk regression and would force the pipeline-key cache to carry
state it does not need.

**PLAN.** The legacy state is uploaded as a **Metal constant buffer at slot
16**, the same slot the modern path uses for the program uniform block
(`src/gl/glsl_to_msl.cpp:12371` emits `constant TGLMTUniforms& tglmt_u
[[buffer(16)]]`; the modern draw path binds it at
`src/gl/gl_apple_draw.cpp:2939-2941`).

Reusing slot 16 is safe because a draw uses **either** the legacy program
**or** a modern program, never both. The uniform staging path
(`Context::UniformCache`, `include/tglmt/Context.h:1029-1036`) already
implements "upload only when the bytes changed", so the legacy path gets the
same optimisation for free.

**State that is a uniform value** (changes per draw, does not change the
shader shape):

* the three matrices: modelview, projection, texture (per unit);
* the normal matrix (inverse-transpose of the modelview 3×3);
* per-light: ambient/diffuse/specular colour, position, spot direction,
  spot exponent, spot cutoff, the three attenuation coefficients;
* front and back materials (ambient/diffuse/specular/emission/shininess);
* the light model: scene ambient, local-viewer, two-side, colour-control;
* fog: mode, colour, start/end/density, coordinate source;
* per texture unit: the texture-environment mode and colour, the combine
  arguments, the RGB/alpha scale factors.

**State that is a shader-shape change** (§4.3).

### 4.2 The generated fixed-function MSL

**PLAN.** One generated MSL program, regenerated only when the *shape* changes.
The MSL below is the design; it is **not** in the tree yet.

#### 4.2.1 Uniform block

```metal
// ---- generated by TGLMT; layout must match the CPU-side packer exactly ----
struct TGLMTLegacyLight {
    float4 ambient;         // GL_AMBIENT
    float4 diffuse;         // GL_DIFFUSE
    float4 specular;        // GL_SPECULAR
    float4 position;        // GL_POSITION (eye space; w == 0 => directional)
    float4 spotDirection;   // GL_SPOT_DIRECTION (eye space, xyz used)
    float  spotExponent;    // GL_SPOT_EXPONENT
    float  spotCutoff;      // GL_SPOT_CUTOFF (degrees; 180 => no cutoff)
    float  k0;              // GL_CONSTANT_ATTENUATION
    float  k1;              // GL_LINEAR_ATTENUATION
    float  k2;              // GL_QUADRATIC_ATTENUATION
    uint   enabled;         // 0 => this light contributes nothing
};

struct TGLMTLegacyUniforms {
    float4x4 modelView;
    float4x4 projection;
    float4x4 textureMatrix;    // GL_TEXTURE_MATRIX of the bound unit
    float4x4 normalMatrix4;    // inverse-transpose of modelView 3x3, in a 4x4
                               // (avoids the float3x3 column-padding trap —
                               //  see the note on std140 vs MSL below)

    uint   numLights;
    TGLMTLegacyLight lights[8];

    float4 matAmbientFront,  matDiffuseFront,  matSpecularFront,  matEmissionFront;
    float4 matAmbientBack,   matDiffuseBack,   matSpecularBack,   matEmissionBack;
    float  matShininessFront, matShininessBack;

    float4 sceneAmbient;      // GL_LIGHT_MODEL_AMBIENT
    uint   localViewer;       // GL_LIGHT_MODEL_LOCAL_VIEWER
    uint   twoSided;          // GL_LIGHT_MODEL_TWO_SIDE
    uint   separateSpecular;  // GL_LIGHT_MODEL_COLOR_CONTROL == GL_SEPARATE_SPECULAR_COLOR
    uint   colorMaterialMask; // bit0 AMBIENT, bit1 DIFFUSE, bit2 SPECULAR, bit3 EMISSION
    uint   normalize;         // GL_NORMALIZE
    uint   rescaleNormal;     // GL_RESCALE_NORMAL

    uint   fogMode;           // 0 none, 1 LINEAR, 2 EXP, 3 EXP2
    float  fogStart, fogEnd, fogDensity;
    float4 fogColor;
    uint   fogCoordSrc;       // 0 FRAGMENT_DEPTH, 1 FOG_COORD

    uint   numTexUnits;
    uint   texEnvMode[8];     // GL_MODULATE / REPLACE / DECAL / BLEND / ADD / COMBINE
    float4 texEnvColor[8];
    float4 texEnvArg[8];      // packed GL_COMBINE arguments (see 4.2.4)
    float4 texEnvArg2[8];
    float  texEnvRgbScale[8];
    float  texEnvAlphaScale[8];

    uint   alphaTestEnabled;
    uint   alphaFunc;         // GL_NEVER .. GL_ALWAYS
    float  alphaRef;

    uint   clipPlaneCount;
    float4 clipPlanes[6];     // eye-space planes (a,b,c,d)

    uint   polygonStipple[32];// 32 rows x 8 bits
};
```

**Layout rules that must be respected by the CPU packer:**

* MSL constant-buffer layout: every member is 16-byte aligned; a `float3` is
  padded to 16 bytes; a `float3x3` has 16-byte column strides.
* GL's `mat3` is 9 tight floats (36 bytes) — **different** from MSL's
  `float3x3` (48 bytes). The existing modern path already handles this exact
  trap for user uniform blocks (`src/gl/glsl_to_msl.cpp` records it in
  `ProgramObject::UniformEntry::glslType`, `include/tglmt/Context.h:332-335`:
  *"MSL float3x3 = 3 cột float3 stride 16 (=48B) còn GL mat3 = 9 float
  column-major chặt (=36B) → phải nới cột"*). The legacy block avoids the
  problem entirely by using `float4x4 normalMatrix4` and taking the upper 3×3.

#### 4.2.2 Vertex stage — transforms

```metal
struct TGLMTLegacyVSOut {
    float4 position   [[position]];
    float4 color;
    float4 texcoord;
    float3 eyePos;
    float  fogCoord;
    float3 eyeNormal;
    float4 specular;        // only emitted when separateSpecular is on
};

vertex TGLMTLegacyVSOut TGLMT_legacy_vs(
    float4 inPos      [[attribute(0)]],
    float3 inNormal   [[attribute(1)]],
    float4 inColor    [[attribute(2)]],
    float4 inTex0     [[attribute(3)]],
    constant TGLMTLegacyUniforms& u [[buffer(16)]])
{
    TGLMTLegacyVSOut o;

    // GL matrices are column-major; uploading the GL bytes directly as an
    // MSL float4x4 and using M * v reproduces GL's "M applied first"
    // semantics exactly.
    float4 eye = u.modelView * inPos;

    // GL_TEXTURE_MATRIX is applied to the texture coordinates *before* the
    // modelview/projection (GL 2.1 §2.10.1).
    float4 tc = u.textureMatrix * inTex0;

    float4 clip = u.projection * eye;
    // GL clip-space z is [-w, w]; Metal NDC z is [0, w].
    // This is the SAME conversion the modern path bakes in
    // (src/gl/glsl_to_msl.cpp:10356, kZConv) — do not diverge.
    clip.z = clip.z * 0.5 + clip.w * 0.5;

    o.position  = clip;
    o.texcoord  = tc;
    o.eyePos    = eye.xyz;
    o.fogCoord  = length(eye.xyz);        // GL_FRAGMENT_DEPTH source
    o.eyeNormal = float3x3(u.normalMatrix4[0].xyz,
                           u.normalMatrix4[1].xyz,
                           u.normalMatrix4[2].xyz) * inNormal;
    o.color     = inColor;
    return o;
}
```

`[[attribute(n)]]` (not `[[stage_in]]`) is correct here because the legacy
path supplies its **own** `MTLVertexDescriptor` (built by
`makeCustomPipeline`, `src/metal/AppleMetalBridge.mm:2661-2703`) rather than
relying on the modern converter's `TGLMT_VIn` struct.

**Normal transform.** GL 2.1 §2.11.3: the normal is transformed by the
inverse-transpose of the upper-left 3×3 of the modelview matrix. `GL_NORMALIZE`
rescales to unit length; `GL_RESCALE_NORMAL` scales by the magnitude of the
*original* normal.

#### 4.2.3 Vertex stage — lighting (per vertex, GL 2.1 §2.14.1)

**SPEC.** "GL lighting computes colors **for each vertex** sent to the GL."
This is not an approximation choice — computing it per fragment is a
*different* result.

```metal
    // ---- normal handling (GL 2.1 §2.11.3) ----
    float3 nEye = o.eyeNormal;
    if (u.normalize) {
        nEye = normalize(nEye);
    } else if (u.rescaleNormal) {
        nEye *= length(inNormal) / max(length(nEye), 1e-20);
    }

    // ---- per-vertex lighting accumulation ----
    float3 diffuseAcc  = float3(0.0);
    float3 specularAcc = float3(0.0);
    for (uint i = 0u; i < u.numLights; ++i) {
        TGLMTLegacyLight L = u.lights[i];
        if (L.enabled == 0u) { continue; }

        // L = unit vector from the vertex towards the light.
        float3 Ldir;
        float  dist;
        if (L.position.w == 0.0) {
            // Directional light: GL_POSITION holds the direction, w == 0.
            Ldir = normalize(-L.position.xyz);
            dist = 0.0;
        } else {
            float3 d = L.position.xyz - o.eyePos;
            dist = length(d);
            Ldir = d / max(dist, 1e-20);
        }

        // V = unit vector from the vertex towards the viewer.
        //   local viewer  : V = normalize(eye_point - P) = normalize(-eyePos)
        //   infinite viewer: V = (0, 0, 1)   (constant — position independent)
        float3 V = u.localViewer ? normalize(-o.eyePos) : float3(0.0, 0.0, 1.0);
        float3 H = normalize(Ldir + V);

        // ---- spotlight (GL 2.1 §2.14.1) ----
        float spot = 1.0;
        if (L.spotCutoff < 180.0) {
            float cd = dot(normalize(-Ldir), normalize(L.spotDirection.xyz));
            if (cd < cos(L.spotCutoff * 0.01745329252)) {
                spot = 0.0;
            } else {
                spot = pow(cd, L.spotExponent);
            }
        }

        // ---- attenuation (GL 2.1 §2.14.1): 1 / (k0 + k1*d + k2*d^2) ----
        float att = 1.0 / max(L.k0 + L.k1 * dist + L.k2 * dist * dist, 1e-20);
        att *= spot;

        float ndotl = max(dot(nEye, Ldir), 0.0);
        diffuseAcc  += L.diffuse.rgb  * ndotl * att;
        specularAcc += L.specular.rgb * pow(max(dot(nEye, H), 0.0),
                                            max(u.matShininessFront, 1e-4)) * att;
    }
```

> **Correction to an existing document.** `docs/LEGACY_TO_METAL_MAPPING.md:730`
> writes `float3 V = u.localViewer ? normalize(-eye.xyz) : normalize(P);`
> where `P` is the **light** position. That is wrong: the infinite-viewer
> vector is the constant `(0, 0, 1)`, independent of both the vertex and the
> light (GL 2.1 §2.14.1: *"if it's not true, the vector (0, 0, 1) is used as
> the second vector in the sum"*). The local-viewer branch
> `normalize(-eye.xyz)` in that document is correct. **This document uses the
> corrected form.**

**Light-position transform (a classic trap, must be CPU-side).** GL 2.1
§2.14.2: *"the current model-view matrix is applied to the position parameter
… when that position is specified"*. So `glLightfv(GL_POSITION, …)` stores the
**eye-space** position, computed with the modelview matrix that is current at
that moment. A later `glTranslate` does **not** move an already-specified
light. The spot direction is transformed by the upper-left 3×3 of the
modelview at specification time. Both are CPU responsibilities at
`glLightfv` time, not shader work.

**Two-sided lighting** selects front or back material by facing. Facing is a
per-fragment property but lighting is per-vertex, so exactly reproducing
"back material on back faces" requires per-fragment lighting. This is a
documented divergence (see `docs/LEGACY_TO_METAL_MAPPING.md` §5.2) and must
be listed in the compatibility caveats; `u.twoSided` is therefore a
**shape-changing** flag (the fragment stage needs `[[front_facing]]`).

**`GL_SEPARATE_SPECULAR_COLOR`** (`GL_LIGHT_MODEL_COLOR_CONTROL`) emits the
specular term into a separate varying and adds it in the fragment stage. This
is exact, and `separateSpecular` is part of the program key.

**`glColorMaterial`** is resolved on the CPU: when `GL_COLOR_MATERIAL` is
enabled for `AMBIENT`/`DIFFUSE`/`SPECULAR`/`EMISSION`, that material
parameter follows the **current colour** at the time the vertex is specified.
The CPU assembler resolves it per vertex (§4.5).

#### 4.2.4 Fragment stage — texture environment, fog, alpha test

Fog is applied to the **post-texturing** colour (GL 2.1 §3.10) and before the
per-fragment operations (alpha test, blending).

```metal
fragment float4 TGLMT_legacy_fs(
    TGLMTLegacyVSOut in [[stage_in]],
    texture2d<float> tex0 [[texture(0)]],
    sampler samp0        [[sampler(0)]],
    texture2d<float> tex1 [[texture(1)]],
    sampler samp1        [[sampler(1)]],
    constant TGLMTLegacyUniforms& u [[buffer(16)]])
{
    float4 color = in.color;

    // ---- texture environment, one block per unit (GL 2.1 tables 3.21/3.22) ----
    for (uint i = 0u; i < u.numTexUnits; ++i) {
        float4 src;
        if (i == 0u) {
            src = tex0.sample(samp0, in.texcoord.xy);
        } else {
            src = tex1.sample(samp1, in.texcoord.xy);
        }
        float4 prev = color;                 // "Cp, Ap"
        float4 env  = u.texEnvColor[i];      // "Cf, Af"

        switch (u.texEnvMode[i]) {
            case 0:  // GL_MODULATE
                color = prev * src;
                break;
            case 1:  // GL_REPLACE
                color = src;
                break;
            case 2:  // GL_DECAL
                color.rgb = prev.rgb * (1.0 - src.a) + src.rgb * src.a;
                color.a   = prev.a;
                break;
            case 3:  // GL_BLEND
                color.rgb = prev.rgb * (1.0 - src.rgb) + env.rgb * src.rgb;
                color.a   = prev.a;
                break;
            case 4:  // GL_ADD
                color.rgb = prev.rgb + src.rgb;
                color.a   = prev.a + src.a;
                break;
            default: // GL_COMBINE — see 4.2.5
                color = tglmt_combine(prev, src, env, u.texEnvArg[i],
                                      u.texEnvArg2[i],
                                      u.texEnvRgbScale[i], u.texEnvAlphaScale[i]);
                break;
        }
    }

    // ---- fog (GL 2.1 §3.10): applied to the post-texturing colour ----
    if (u.fogMode != 0u) {
        float c = (u.fogCoordSrc == 1u) ? in.fogCoord : length(in.eyePos);
        float f;
        if (u.fogMode == 1u) {                       // GL_LINEAR  (eq. 3.33)
            f = (u.fogEnd - c) / (u.fogEnd - u.fogStart);
        } else if (u.fogMode == 2u) {                // GL_EXP     (eq. 3.31)
            f = exp(-u.fogDensity * c);
        } else {                                    // GL_EXP2    (eq. 3.32)
            f = exp(-(u.fogDensity * c) * (u.fogDensity * c));
        }
        f = clamp(f, 0.0, 1.0);
        color.rgb = f * color.rgb + (1.0 - f) * u.fogColor.rgb;   // alpha unchanged
    }

    // ---- alpha test (GL 2.1 §4.1.1) ----
    if (u.alphaTestEnabled) {
        float a = color.a;
        bool pass = false;
        switch (u.alphaFunc) {
            case 0:  pass = false;                  break;  // GL_NEVER
            case 1:  pass = (a <  u.alphaRef);     break;  // GL_LESS
            case 2:  pass = (a == u.alphaRef);     break;  // GL_EQUAL
            case 3:  pass = (a <= u.alphaRef);     break;  // GL_LEQUAL
            case 4:  pass = (a >  u.alphaRef);     break;  // GL_GREATER
            case 5:  pass = (a != u.alphaRef);     break;  // GL_NOTEQUAL
            case 6:  pass = (a >= u.alphaRef);     break;  // GL_GEQUAL
            default: pass = true;                  break;  // GL_ALWAYS
        }
        if (!pass) { discard_fragment(); }
    }

    return color;
}
```

Fog semantics that must not be missed: the result `f` is clamped to `[0,1]`;
**alpha is not changed** by fog; `GL_LINEAR` with `start == end` is undefined
in GL and needs a deterministic TGLMT choice (proposed: a hard step
`f = (c >= end) ? 0 : 1`) which must be recorded as a divergence.

#### 4.2.5 `GL_COMBINE` (GL 1.3)

The general combiner is mechanical but large. It takes three sources
(`SRC0_RGB`, `SRC1_RGB`, `SRC2_RGB`) and an alpha source, each with an
operand applied to RGB and to alpha separately, combined by `COMBINE_RGB` /
`COMBINE_ALPHA` (`REPLACE`, `MODULATE`, `ADD`, `ADD_SIGNED`, `INTERPOLATE`,
`SUBTRACT`, `DOT3_RGB`, `DOT3_RGBA`), then scaled by `RGB_SCALE` /
`ALPHA_SCALE`. `ADD_SIGNED` clamps its operand to `[-1,1]` before adding;
`INTERPOLATE` is `a + 2b + c − 1`; the `DOT3` modes produce a 4-component dot
product that is replicated.

**PLAN:** emit `tglmt_combine` as a helper function into the generated MSL
when any unit uses `GL_COMBINE`; otherwise omit it (so the common case does
not pay for it).

### 4.3 Program shape → program key

The **program key** (`LegacyProgramKey`) is the *shape* of the generated
shader. It is the input to the legacy-program cache; a change in the key
regenerates the MSL, compiles a **new** `ILibrary` pair, and bumps
`ProgramObject::linkGen`.

| Key field | Changes the shader because… |
|---|---|
| `lighting` | lighting code present/absent |
| `shadeModel` (`GL_SMOOTH`/`GL_FLAT`) | the flat-shading path differs (§4.7) |
| `separateSpecular` | an extra varying + fragment add |
| `twoSided` | the fragment stage needs `[[front_facing]]` |
| `fog` | fog block present/absent |
| `fogCoordSrc` | `GL_FOG_COORD` needs an extra vertex attribute |
| `alphaTest` | alpha-test block present/absent |
| `clipPlanes` | clip-plane code present/absent (and how many) |
| `polygonStipple` | stipple code present/absent |
| `texEnvCombine` | the `tglmt_combine` helper present/absent |
| `numTexUnits` | number of texture blocks emitted |
| `texGen` | which texture-generation modes are compiled in |
| `pointSprite` | `GL_POINT_SPRITE` origin handling |
| `numLights` | **uniform**, *not* a key field — the loop bound is a uniform |

`numLights` is deliberately **not** in the key: the loop is `for (i = 0; i <
u.numLights; ++i)`, so enabling a 9th light does not change the shader.

### 4.4 The pipeline key — proof that a legacy program does not break the cache

**This is the requirement: "prove, with code, that adding a legacy program as
one new key does not break the cache; name the fields that would have to be
added and why."**

The answer: **no field has to be added.** Here is the proof.

#### 4.4.1 What the key is

`Context::PipelineKey` — `include/tglmt/Context.h:1041-1108`:

| Field | Line | Meaning |
|---|---|---|
| `vsLib` | `:1045` | `ILibrary::identity()` of the vertex library |
| `fsLib` | `:1046` | `ILibrary::identity()` of the fragment library |
| `programGen` | `:1049` | `ProgramObject::linkGen` — link generation counter |
| `fmt` | `:1050` | colour pixel format |
| `stride` | `:1051` | vertex stride |
| `depth`, `samples`, `stencil`, `stencilDesc` | `:1052-1055` | depth/stencil configuration |
| `colorCount`, `colorFmts[8]`, `blends[8]` | `:1057-1059` | MRT configuration |
| `nAttribs`, `attribs[16]` | `:1062-1063` | the vertex descriptor |

`operator==` (`:1064-1107`) compares every one of those fields. The MRU cache
is `Context::pipeCache[8]` (`:1117-1123`), and the lookup is at
`src/gl/gl_apple_draw.cpp:2374-2378`.

#### 4.4.2 Why the existing fields are already sufficient

`ILibrary::identity()` is a **monotonic serial**, not a pointer:

* `include/tglmt/MetalInterface.h:313-324` — the contract, with the explicit
  warning that a pointer identity is unsafe (the allocator may return the same
  address for a new object, which would hand back a stale pipeline);
* `src/metal/AppleMetalBridge.mm:438-445` — the implementation:

```cpp
static std::atomic<uint64_t> gLibrarySerial{1};
...
    explicit AppleLibrary(id<MTLLibrary> l)
        : lib_(l), serial_(gLibrarySerial.fetch_add(1, std::memory_order_relaxed)) {}
    uint64_t identity() const override { return serial_; }
```

Therefore, for a legacy program compiled through the same
`IDevice::compileLibrary` (`src/metal/AppleMetalBridge.mm:1783`):

| Requirement | How the existing key satisfies it |
|---|---|
| legacy ≠ any modern program | different `vsLib`/`fsLib` serials |
| legacy generation *n* ≠ legacy generation *n+1* | new serials **and** a bumped `programGen` |
| two different legacy shapes ≠ each other | different serials (different MSL ⇒ different library) |
| legacy vs modern, same draw config | different serials ⇒ different key entries ⇒ no collision |
| the legacy vertex layout | already covered by `attribs[16]`/`nAttribs`/`stride` — `operator==` compares loc, size, type, normalized, offset, bufferIndex, stride, divisor (`:1097-1105`) |
| legacy uniform *values* | **must not** be in the key — and they are not; they are uploaded per draw through `Context::UniformCache` |

#### 4.4.3 The two subtleties that *would* break the cache if mishandled

1. **`linkGen` must be bumped on every regeneration.** `ProgramObject::linkGen`
   is documented at `include/tglmt/Context.h:220-224` as exactly the guard
   against "the same `ILibrary` identity reused". The legacy program cache
   must therefore increment it on every regeneration, not just on first
   creation. **If this is missed, the cache can return the pipeline of the
   previous legacy program.**
2. **The legacy program must not be stored in `Context::programs`.** It is not
   a GL program object: it must not appear in `glGetIntegerv(GL_CURRENT_PROGRAM)`,
   must not be listed by `glGetProgramiv`, and `glUseProgram` must not be able
   to select it. It lives in a dedicated member of the legacy state and is only
   *referenced* by the draw path. Storing it in `Context::programs` would make
   it observable through the public API — a spec violation.

#### 4.4.4 What the bridge does with the key

`makeCustomPipeline` (`src/metal/AppleMetalBridge.mm:2661`) builds its own
string key from the same inputs (`:2705-2712`): the two library identities, the
entry-point names, the pixel format, the stride, and the per-attribute
`loc,size,type,offset,bufferIndex,divisor,stride`. The depth/stencil/blend
parts are appended at `:2764-2790`. **A legacy program participates in this
key identically to a modern program** — again no new field.

#### 4.4.5 Conclusion

**No new field is required in `Context::PipelineKey`.** The legacy program is
one more `(vsLib, fsLib, programGen)` triple. The *only* thing that must be
enforced is P2 (§3): the legacy program must be a **real `ILibrary` pair**
compiled through `IDevice::compileLibrary`, not a special-cased pipeline.

> If the legacy program were implemented as a special case inside
> `makeCustomPipeline` (e.g. a flag "this is the legacy pipeline"), then
> **yes**, a new field would be needed — a `bool legacy` or a
> `LegacyProgramKey` hash — because two different legacy shapes could then
> collide on the same `(vsLib=0, fsLib=0)`. That design is rejected.

### 4.5 The legacy vertex layout

**PLAN.** The legacy path needs a **fixed** vertex layout, because the
generated shader has fixed `[[attribute(n)]]` slots. Metal supports 31 vertex
attributes (A11 feature tables p. 7); the backend enforces that limit at
`src/metal/AppleMetalBridge.mm:2677` (`attribs[i].loc >= 31 → nullptr`). The
GL legacy minimum is 2 texture units and modern drivers expose far more.

| location | attribute | type | bytes | source |
|---|---|---|---|---|
| 0 | `position` | float4 | 16 | `glVertex*` x,y,z,w |
| 1 | `normal` | float3 | 12 | `glNormal*` |
| 2 | `color` | float4 | 16 | `glColor*` (or the resolved `glColorMaterial` value) |
| 3 | `texcoord` | float4 | 16 | `glTexCoord*` of the bound unit, after the texture matrix |
| 4 | `fogCoord` | float1 | 4 | only when `GL_FOG_COORD_SRC == GL_FOG_COORD` |
| 5 | `flatColor` | float4 | 16 | only when `shadeModel == GL_FLAT` (§4.7) |

**Stride** = the sum of the enabled attributes' sizes, tightly packed
(4-byte alignment for `float3`/`float1` inside a vertex buffer is *not*
required on Metal — `MTLVertexFormat` is a per-attribute format and the
offset is arbitrary; but the offset must be 4-byte aligned for `Float3`).

The layout is a function of the `LegacyProgramKey`, and `LegacyProgramKey`
implies the layout ⇒ the layout is stable for a given key ⇒ the pipeline
key's `stride` field (`include/tglmt/Context.h:1051`) and `attribs[]` are
stable ⇒ the cache stays correct.

**Secondary colour** (`glSecondaryColor*`) and **colour index** (`glIndex*`)
are not in the base layout. `GL_SECONDARY_COLOR` is not in the core GL 2.1
Compatibility feature set that `glLightModel` supports; `glIndex`/colour-index
rendering is a colour-index buffer mode, which TGLMT does not implement
(status `MISSING`, §2.1). When those modes are implemented they add one
attribute each and therefore one more program-key field.

#### 4.5.1 Where the vertices are assembled

**CURRENT.** `AssemblePrimitives` (`src/gl/gl_legacy_vertex.cpp:100-162`)
already assembles the primitive topology correctly:

* `GL_QUADS` → `(v0,v1,v2)` + `(v0,v2,v3)` (`:144-149`);
* `GL_QUAD_STRIP` → quad *i* = `(v2i, v2i+1, v2i+3, v2i+2)` → `(v2i,v2i+1,v2i+3)`
  + `(v2i,v2i+3,v2i+2)` (`:150-155`).

The `GL_QUAD_STRIP` ordering is the spec's (GL 1.4 §2.6.1) and **must not**
be "simplified" to `(2i, 2i+1, 2i+2, 2i+3)` — that flips the winding of
every quad in the strip. The file's own header comment
(`src/gl/gl_legacy_vertex.cpp:15-16`) warns about this.

**PLAN.** The assembler additionally: resolves `glColorMaterial` against the
current colour; evaluates `GL_OBJECT_LINEAR` / `GL_EYE_LINEAR` texture
generation on the CPU; computes the flat-source colour for `GL_FLAT`; and
writes the result into a flat, tightly-packed vertex array.

#### 4.5.2 Batching

**PLAN.** The legacy path batches by primitive type and flushes the batch when
something that could reorder primitives changes. The existing ring allocator
(`Context::RingAlloc`, `include/tglmt/Context.h:1235`, 4 MB × 5 frames at
`:1224-1230`) is exactly the right destination for the vertex array — no new
allocation path, no new lifetime hazard.

A batch must be broken by: a change of primitive mode; a change of any
pipeline state (blend, depth func, cull, front face, viewport, scissor, depth
bias, colour mask); a change of any *legacy* shading state (lights, material,
fog, texenv, texgen, the matrices, shade model); a change of render target; a
framebuffer read (`glReadPixels`, `glCopyPixels`, `glAccum` with `RETURN`);
entering/leaving selection or feedback mode; `glFlush`/`glFinish`/present.

The first three of those are already detected by the modern path's dirty-check
machinery (`include/tglmt/Context.h:997-1023`), so the legacy path reuses it
rather than re-implementing the comparison.

### 4.6 Dispatch rules

**SPEC.** GL 2.1 §2.15 (quoted in §1.1).

**PLAN — the exact dispatch, per draw:**

```
                        (no program object currently in use)
                                    │
                    ┌───────────────┴───────────────┐
                    ▼                               ▼
          legacy fixed-function              programmable pipeline
          (generated MSL program)            (user GLSL program)
                    │                               │
                    └───────────────┬───────────────┘
                                    ▼
                       shared deferred draw path
                       src/gl/gl_apple_draw.cpp:1774 AppleDrawGL
```

Concretely, `src/gl/gl_apple_draw.cpp:1799-1804` becomes:

```cpp
    } else {
        auto pit = c.programs.find(prog);
        if (pit == c.programs.end() || !pit->second.linked) {
            if (prog == 0 && c.legacy.enabled) {
                prp = &c.legacy.program;   // the generated legacy program
            } else {
                c.appleStats.noProgram++; return false;
            }
        } else {
            prp = &pit->second;
        }
    }
```

The full dispatch table:

| Condition | Result | Authority |
|---|---|---|
| `prog != 0`, program linked | modern pipeline | current behaviour |
| `prog != 0`, program **not** linked | draw is an error (`GL_INVALID_OPERATION`) | GL 4.6 §7.3 |
| `prog == 0`, context is Core Profile | draw is an error (`GL_INVALID_OPERATION`) | GL 4.6 §2.15 / Core Profile removes fixed-function entirely |
| `prog == 0`, context is Legacy or Compatibility | **legacy fixed-function** | GL 2.1 §2.15 |
| `prog == 0`, context is Compatibility but a *program pipeline* is bound | the pipeline's VS/FS, if both present | GL 4.6 §7.3 — this is the `pipeBound` branch at `src/gl/gl_apple_draw.cpp:1791-1797`, which is already handled and takes priority |

Note the ordering: the `PROGRAM_PIPELINE_BINDING` check
(`src/gl/gl_apple_draw.cpp:1790-1797`) happens **before** the program lookup,
so a bound pipeline wins over "no program in use". That is correct.

**Compatibility-profile detection.** A context is Legacy (`< 3.2`) or
Compatibility (`≥ 3.2`) per `Context::glProfile`
(`include/tglmt/Context.h:652`) and `GlProfile` (`include/tglmt/VersionGate.h:24-31`).
The C ABI already gates legacy entry points by `(version, profile)`
(`include/tglmt/VersionGate.h:58-65`), so `glBegin` in a Core-profile context
never reaches the dispatcher — it is rejected with `GL_INVALID_OPERATION`
(D-002/D-003).

### 4.7 Winding and the provoking vertex

#### 4.7.1 The divergence

| Concept | OpenGL | Metal |
|---|---|---|
| Which vertex supplies a `flat` value | **last vertex** by default in GL 3.2+ (`GL_LAST_VERTEX_CONVENTION`); in GL 2.1 compatibility the provoking vertex **depends on the primitive type** (GL 2.1 table 2.12) | **first** vertex of the primitive |
| `glShadeModel(GL_FLAT)` in GL 2.1 | provoking vertex per primitive type | — |

The Metal Programming Guide ("Graphics Rendering: Render Command Encoder")
states: *"When rendering a triangle primitive with flat shading, the
attributes of the **first** vertex (also known as the provoking vertex) are
used for the whole triangle."* This is recorded in
`docs/HARDWARE_LIMITATIONS.md:122-131` (L2) and
`docs/METAL_FEATURE_MAPPING.md` F4 / H15, both of which also note that
`setProvokingVertexMode:` **does not exist** in the iOS SDK.

> **Inconsistency found.** `docs/LEGACY_TO_METAL_MAPPING.md:171` still states
> Metal uses the **last** vertex. `docs/HARDWARE_LIMITATIONS.md:131` and
> `docs/METAL_FEATURE_MAPPING.md` (F4, H15) state **first** and explicitly
> flag §1.5 of the mapping document as wrong. **This document follows
> "first".** The mapping document should be corrected.

#### 4.7.2 Consequence for `glShadeModel(GL_FLAT)`

GL 2.1's provoking vertex per primitive type (table 2.12):

| Primitive | GL 2.1 flat source |
|---|---|
| single polygon | vertex 1 (the *second*) |
| triangle strip, triangle *i* | vertex *i*+2 |
| triangle fan, triangle *i* | vertex *i*+2 |
| independent triangle *i* | vertex 3*i* |
| quad strip, quad *i* | vertex 2*i*+2 |
| independent quad *i* | vertex 4*i* |

**PLAN.** Because the legacy path already assembles primitives on the CPU
(`src/gl/gl_legacy_vertex.cpp:100-162`), it knows the flat-source vertex for
each primitive *at assembly time*. Two cases:

* **Lighting off** (the common case): the flat colour is the vertex's current
  colour. The assembler writes that colour into the colour attribute of
  **every** vertex of the primitive; the shader then interpolates a constant.
  **Exact**, zero GPU cost.
* **Lighting on**: the flat colour is the *lit* colour of the flat-source
  vertex, which depends on that vertex's position and normal — values the
  other vertices do not have. Exact reproduction needs the flat-source
  vertex's lighting inputs. **PLAN:** the assembler emits the flat-source
  vertex's (position, normal, colour) as a second attribute set
  (`flatPos`, `flatNormal`, `flatColor`); the vertex shader evaluates lighting
  a second time on those and the fragment stage selects the flat result.
  Cost: one extra lighting evaluation and three extra attributes — but it is
  exact.

This is why `shadeModel == GL_FLAT` is part of the program key: the shader
has a different shape.

#### 4.7.3 Consequence for `GL_QUADS` / `GL_QUAD_STRIP`

Quad **winding** and quad **flat shading** are two different things and must
not be confused.

* **Winding.** The CPU triangulation of `GL_QUADS` → `(v0,v1,v2)+(v0,v2,v3)` and
  `GL_QUAD_STRIP` → `(v2i,v2i+1,v2i+3)+(v2i,v2i+3,v2i+2)` preserves the
  original winding, so the front-face determination is unchanged. This is
  already implemented (`src/gl/gl_legacy_vertex.cpp:144-155`) and is
  correct.
* **Flat shading.** For `GL_QUADS`, GL 2.1's flat source is vertex
  **4*i**+1 (the second vertex of the quad) and for `GL_QUAD_STRIP` it is
  vertex 2*i*+2. After triangulation into triangles, the *triangle's* first
  vertex is not the quad's flat source, so the "duplicate the flat colour into
  every vertex of the primitive" trick must be applied **per quad**, not per
  triangle. The assembler must therefore compute the flat source at the
  **quad** level and apply it to both triangles of that quad. Doing it per
  triangle would produce two different colours in one quad — a visible,
  wrong result.
* **Orientation.** The modern path already compensates for the y-flip (§4.8)
  by inverting the front-face convention when the clip origin is not
  upper-left (`src/gl/gl_apple_draw.cpp:2688-2690`). The legacy path inherits
  this for free because it uses the same viewport state and the same
  `setCullMode` call.

#### 4.7.4 The modern path's own flat-shading divergence

The same first-vs-last provoking-vertex divergence affects the **modern**
path's `flat`-qualified varyings (`GL_ARB_provoking_vertex`). The fix chosen
in `docs/METAL_FEATURE_MAPPING.md` H15 is **CPU index rotation** (the
technique ANGLE's Metal backend uses): for triangles, swap the first and last
vertex of each triangle; for strips, rotate; for lines, swap the pair.
**PLAN:** the legacy path and the modern path should share **one** index
rotation helper, so the fix is written once. Status of the modern path today:
`HARDWARE_LIMITED` natively, fix available but **not implemented**.

### 4.8 Clip-space z and the viewport y-flip

**These conventions already exist in TGLMT. The legacy path must reuse them
exactly.**

#### 4.8.1 Clip-space z

**CURRENT.** The modern path bakes the GL→Metal z conversion into every
generated vertex shader:

* `src/gl/glsl_to_msl.cpp:10356`

```cpp
const std::string kZConv = " _out.tglmt_position.z = _out.tglmt_position.z * 0.5 + _out.tglmt_position.w * 0.5;";
```

which maps GL's clip `z ∈ [−w, w]` onto Metal's `z ∈ [0, w]`. The viewport's
`znear`/`zfar` are then passed through unchanged, because the conversion has
already happened (`include/tglmt/GLConvert.h:36-37`).

**PLAN.** The legacy vertex shader uses the identical line (§4.2.2). This is
why `glDepthRange`, `glReadPixels` depth values and the accumulation buffer's
depth semantics do not shift between the two pipelines.

**Decision point:** the conversion lives in the **shader**, not in the
viewport. Alternative (converting in the viewport by halving `znear`/`zfar`)
would double-convert for the modern path, which already bakes it. **Chosen:
bake it in the shader, exactly like the modern path.**

#### 4.8.2 The viewport y-flip

**CURRENT.** TGLMT keeps framebuffer memory in **GL order** (row 0 = the
bottom of the GL window). A lower-left-origin GL viewport is therefore
expressed as a **negative-height** Metal viewport
(`include/tglmt/GLConvert.h:21-45`, the flip at `:36-39`):

```cpp
    } else {
        vp.y = (double)y + (double)h; // NDC y=-1 → hàng memory y (đáy GL)
        vp.h = -(double)h;            // negative viewport height = lật y
    }
```

Because that flip mirrors the winding, the modern path inverts the
front-face convention when the clip origin is not upper-left
(`src/gl/gl_apple_draw.cpp:2688-2690`).

**PLAN.** The legacy path uses the same `GLViewportToMetal`
(`include/tglmt/GLConvert.h:21-45`) and therefore inherits both the flip and
the front-face inversion. **Decision point:** the flip is in the *viewport*,
not in the shader — because the shader cannot know the viewport. **Chosen:
reuse `GLViewportToMetal` unchanged.**

#### 4.8.3 The one thing that *is* legacy-specific

`GL_CLIP_PLANE0..5` (six user clip planes). Metal offers exactly **one**
`[[clip_distance]]` scalar — the array form is rejected by the Metal
compiler, which TGLMT already detects and reports
(`src/gl/glsl_to_msl.cpp:6024-6027` for the comment, the check at `:6070-6072`):

```cpp
// Metal [[clip_distance]] chỉ nhận MỘT scalar (mảng/ nhiều field bị compiler
// từ đối) ...
    if (idx != 0) {
        err = "gl_ClipDistance[" + std::to_string(idx) +
              "] chưa hỗ trợ (Metal chỉ 1 clip_distance)";
        return false;
    }
```

So at most **one** legacy clip plane can be expressed natively; the remaining
five must be emulated in the fragment shader (discard) or on the CPU
(clip the primitives). This is a **PLAN** item, not part of the draw-path
core, and it is a **shape-changing** program-key field (`clipPlaneCount`).

### 4.9 Where the legacy state is uploaded

| Item | Mechanism | Evidence |
|---|---|---|
| Legacy uniform block | Metal constant buffer, **slot 16** | the modern path uses the same slot: `src/gl/glsl_to_msl.cpp:12371`; bound at `src/gl/gl_apple_draw.cpp:2939-2941` |
| Vertex array | per-frame ring allocator | `include/tglmt/Context.h:1224-1235`; `Context::RingAlloc` |
| Index buffer (for indexed legacy draws) | same ring / same path as the modern path | `src/gl/gl_apple_draw.cpp:664-702` (index rewrite cache) |
| Textures | the modern texture-binding path — unchanged | `src/gl/gl_apple_draw.cpp:207-373` (`SamplerForUnit`) |
| Pipeline | `makeCustomPipeline` with the legacy `CustomAttrib` array | `src/metal/AppleMetalBridge.mm:2661` |

**Buffer budget check.** The legacy path needs: 1 constant buffer (slot 16),
at most 16 fragment textures + 16 samplers (the A11 limit), 1 vertex buffer.
It does **not** need slots 17+ (UBO/SSBO), 24+ (atomics) or 30 (sample mask).
Because a draw uses one program or the other, never both, there is no slot
conflict.

---

## 5. The legacy draw path — implementation shape

### 5.1 New files (PLAN)

| File | Contents |
|---|---|
| `include/tglmt/LegacyState.h` (extend) | the lighting / fog / texenv / raster / pixel-op state blocks; the legacy program cache; the `LegacyProgramKey` |
| `src/gl/gl_legacy_program.cpp` (new) | `LegacyProgramKey` → MSL text → `compileLibrary` → the cached `ProgramObject` |
| `src/gl/gl_legacy_draw.cpp` (new) | the vertex assembly + the batch flush that calls `AppleDrawGL` with the legacy program |
| `src/gl/gl_legacy_fixed.msl` (new, or an inline string) | the MSL template pieces |

### 5.2 Modified files (PLAN) — and nothing else

| File | Change | Risk |
|---|---|---|
| `src/gl/gl_apple_draw.cpp:1799-1804` | the dispatch branch (§4.6) | **LOW** — one branch, guarded by `prog == 0` |
| `src/gl/gl_legacy_vertex.cpp:89-95` | `glEnd` → assemble **and** submit the batch | **MED** — new code path, but the assembly itself is unchanged and already tested |
| `include/tglmt/Context.h` | add the legacy program cache + the legacy state blocks | **LOW** — additive members |

Nothing in `src/core/`, nothing in `src/metal/`, nothing in the generated
files is touched. **The modern path's code is not modified at all** except for
the one dispatch branch.

### 5.3 How the legacy program reaches the existing uniform machinery

`ProgramObject` is reused **as-is** for the legacy program. It already carries
everything the existing uniform-upload code needs:

* `appleVS`, `appleFS` (`include/tglmt/Context.h:379`)
* `linkGen` (`:224`)
* `uniformLayout`, `vsUBSize`, `fsUBSize` (`:339`, `:378`)
* `vsSamplers`, `fsSamplers`, `samplerUnits`, `samplerKind` (`:416-421`)
* `attribFill` — *not* used by the legacy path (the legacy shader reads only
  the attributes it declares)

The existing uniform upload path
(`src/gl/gl_apple_draw.cpp:2817-2941`) therefore works unchanged. The legacy
uniform block is laid out so that the CPU packer fills `uniformLayout` exactly
as it would for a translated GLSL program.

---

## 6. Layer 5 — do we need a unified IR?

**Answer: a *command* IR is needed, but only for display lists, and it does
not need to be a general-purpose IR over both pipelines.**

### 6.1 What the modern path already has (and it is not an IR)

`Context`'s `pending*` members (`include/tglmt/Context.h:909-1023`) are a
**deferred-materialisation shadow of Metal state**: "have I already set this
viewport / this blend state / this pipeline?" The modern path does not need
more than that, because the modern path never replays anything.

### 6.2 What display lists need — and why it is unavoidable

**SPEC.** GL 2.1 §5.4: *"the commands within the list to be executed just as
if they were given normally. The only exception pertains to commands that rely
upon client state."*

Two consequences:

1. **Server state is evaluated at execution time.** A list containing
   `glTranslate(1,0,0)` followed by a `glBegin/glEnd` block must translate by
   the matrix current when the list is *called*, not when it was compiled.
2. **Client state is captured at compile time.** A recorded `glDrawElements`
   must capture the index *data*, not the pointer; a recorded
   `glDrawElements` from a buffer must capture the index bytes.

A **cached draw** (compile the list into a vertex buffer, re-issue one draw)
therefore violates the spec. The only representation that preserves the
semantics exactly is a **command stream**.

### 6.3 PLAN — a tagged command stream, scoped to display lists

```cpp
// include/tglmt/LegacyState.h (PLAN)
enum class LegacyCmd : uint16_t { /* ... */ };

struct LegacyCmdRecord {
    LegacyCmd op;
    uint32_t  a, b;        // op-specific scalars
    float     f[4];        // op-specific floats
    // payload: a small tagged byte blob for the commands that need one
    std::vector<uint8_t> blob;
};
```

The replay path calls **the same internal functions** the direct path calls,
so there is no second implementation of any semantics.

**The recording switch must live in the dispatcher, not in each entry
point.** The public C ABI is a generated flat wrapper
(`src/gl/gl_c_exports.cpp`, generated by `tools/gen_c_exports.py`) that
forwards 1:1 into `tglmt::gl::`. A `Context::displayListCapture` flag consulted
by the wrapper is the natural place — but that is a change to a **generated**
file, which is the single biggest risk in the compatibility programme
(`docs/LEGACY_TO_METAL_MAPPING.md` §9.2). **PLAN: split it.**

* **Stage 1:** record only the commands legacy applications actually use
  (the immediate-mode block, state changes, draws); everything else executes
  immediately.
* **Stage 2:** full dispatcher interception honouring the spec's
  "execute immediately" list exactly.

### 6.4 Performance implications of a command stream

* A command-stream display list replays at **GL-command speed**, not at
  Metal-draw speed. This is inherent — the whole point is that server state is
  re-evaluated.
* The optimisation that makes this acceptable: a list containing only state
  changes plus one immediate-mode block collapses to "apply the state
  changes, then one vertex-array draw". For the common legacy pattern (build
  a list once, call it every frame with a changing modelview), the replay is
  **one** `drawPrimitives` — as fast as the equivalent modern code.
* **Uniform / pipeline cache impact:** replaying a list re-enters the normal
  draw path, so the pipeline cache and the uniform cache apply exactly as
  they do for a direct draw. **No new caching layer is needed.**

### 6.5 Should the IR be unified across both pipelines?

**No — and this is a deliberate decision, not an omission.**

* The modern path is a *stream* of draws against a *current program*. Its
  natural representation is "the current program + the current state + a
  draw".
* The legacy path is the same, except the "current program" is the generated
  legacy program.
* Unifying them into one IR would mean rewriting the modern path — which
  `other/prompt.md` §7 explicitly forbids ("Không được viết lại toàn bộ dự
  án") and which would put the working Minecraft path at risk for no
  functional gain.

**The unification is at the *draw path*, not at the *command path*.** Both
pipelines converge on `AppleDrawGL` (`src/gl/gl_apple_draw.cpp:1774`) with a
`ProgramObject` in hand. That is the right place for the seam, and it is
already there.

---

## 7. Baseline measurements

### 7.1 Command coverage (measured)

Source: `data/version_inventory.json` (generated by
`tools/gen_version_inventory.py` from `docs/khronos/gl.xml`), fields
`totals` and `self_check`:

| Metric | Value |
|---|---|
| GL 4.6 **Core** commands | **657** |
| GL 4.6 **Compatibility** commands | **1048** |
| In Compatibility but not in Core | **391** |
| "Compatibility-only" (removed from Core by the registry) | **42** |
| Enums | **1808** |
| Extensions | **863** |
| Version slots | **19** |

### 7.2 Legacy stub count (measured)

Source: `src/gl/gl_stubs_gen.cpp` (generated by `tools/gen_stubs.py`). Each
stub is a function whose body logs `UNIMPLEMENTED: <name>` and returns a safe
value.

| Measurement | Value | How |
|---|---|---|
| Stub functions in `src/gl/gl_stubs_gen.cpp` | **240** | `grep -c "UNIMPLEMENTED: gl" src/gl/gl_stubs_gen.cpp` |
| Of those, introduced in GL 1.0 | 173 | joined against the inventory |
| … GL 1.1 | 16 | " |
| … GL 1.3 | 17 | " |
| … GL 1.4 | 34 | " |
| **Of those, in the T1 tier (GL 1.0–2.1)** | **240 (100 %)** | " |
| GL 1.0–2.1 fixed-function commands (349) **implemented** | **109** | 349 − 240 |
| — `src/gl/gl_legacy_matrix.cpp` (26 functions, 20 compatibility-only) | 20 | counted in the file |
| — `src/gl/gl_legacy_vertex.cpp` (98 functions, 88 compatibility-only, incl. 8 macro-generated) | 88 | " |
| — `src/gl/gl_raster.cpp` (`glLineStipple` — an exported **no-op**, not behaviour) | 1 | `src/gl/gl_raster.cpp:274` |

**Consequence:** every remaining stub is a GL 1.0–2.1 fixed-function entry
point. **No stub exists in the modern core surface.** The remaining work is
entirely the T1 tier.

(The T0–T4 tier definitions are in `docs/IMPLEMENTATION_ROADMAP.md` §3.)

---

## 8. Low-risk refactor plan

### 8.1 Order (PLAN)

Each step is independently testable and independently revertible.

| # | Step | Files | Test to add | Revert |
|---|---|---|---|---|
| 0 | **Baseline** | — | run the existing suite; record the pass count | — |
| 1 | Extend `LegacyState` with the lighting / fog / texenv / raster state blocks (state only, no behaviour) | `include/tglmt/LegacyState.h` | `test_legacy_state.cpp` — set/get round-trip | `git revert` of the header |
| 2 | Implement the **state-setting entry points** (`glLight*`, `glMaterial*`, `glLightModel*`, `glShadeModel`, `glColorMaterial`, `glFog*`, `glTexEnv*`, `glTexGen*`, `glAlphaFunc`, `glPolygonStipple`, `glClipPlane`) — state only | new `src/gl/gl_legacy_light.cpp`, `src/gl/gl_legacy_raster.cpp` | `test_legacy_light_state.cpp` | delete the new file + the stub entries |
| 3 | Implement the **generated MSL emitter** + the program cache (no draw yet) | new `src/gl/gl_legacy_program.cpp` | `test_legacy_program.cpp` — the MSL text contains the expected blocks; the key changes ⇒ a new library identity | delete the new file |
| 4 | Wire the **dispatch branch** (§4.6) — legacy program selected when no program is in use; the draw still fails later, but the branch is exercised | `src/gl/gl_apple_draw.cpp:1799-1804` | `test_legacy_dispatch.cpp` | revert one branch |
| 5 | Implement the **vertex assembly → Metal draw** for the immediate-mode path | new `src/gl/gl_legacy_draw.cpp`; `src/gl/gl_legacy_vertex.cpp:89-95` | `test_legacy_draw.cpp` (Null backend: trace comparison) + `test_legacy_draw_apple.cpp` (pixel comparison) | revert the `glEnd` hook |
| 6 | Implement **lighting / fog / texenv** in the generated shader and validate against reference images | `src/gl/gl_legacy_program.cpp` | pixel-comparison tests | regenerate the old program text |
| 7 | Implement the **remaining legacy entry points** in the tier order of `docs/IMPLEMENTATION_ROADMAP.md` | per stage | per stage | per stage |

**Invariant for every step:** the modern path's behaviour must not change.
The measurable check is the existing test suite plus the Minecraft boot path
(`tests/unit/test_mc_boot_path.cpp`).

### 8.2 Rollback

* Every step above adds **new files** or adds **new members**; only step 4
  modifies an existing function, and it modifies **one branch**.
* Steps 1–3 and 5–7 are revertible by deleting the new file(s) and restoring
  the `LegacyState` header from git. The generated stub file
  (`src/gl/gl_stubs_gen.cpp`) is regenerated by `tools/gen_stubs.py`, never
  edited by hand (`AGENTS.md` §5), so removing an implementation is a matter
  of deleting the implementation file and re-running the generator.
* Step 4 is revertible by reverting one `if` statement.

### 8.3 What is **not** being proposed

* **No** rewrite of `src/gl/gl_apple_draw.cpp`. The dispatch branch is 5 lines.
* **No** change to `StateTracker`. Legacy state stays in `Context::legacy`
  (D-004).
* **No** change to `Context::PipelineKey`. §4.4 proves it is not needed.
* **No** change to the Metal abstraction (`include/tglmt/MetalInterface.h`).
  The legacy path uses `makeCustomPipeline`, `compileLibrary`,
  `setVertexBuffer`, `setFragmentBuffer` — all already there.
* **No** change to any generated file except through its generator.

---

## 9. Risk matrix

| # | Risk | Impact | Mitigation | How to verify |
|---|---|---|---|---|
| R1 | The legacy program is implemented as a special case in `makeCustomPipeline` instead of as a real `ILibrary` pair | **HIGH** — pipeline cache returns the wrong program; two legacy shapes collide | P2 (§3): compile the legacy MSL through `IDevice::compileLibrary` and install it in `ProgramObject::appleVS`/`appleFS` | A test that builds two different legacy shapes and asserts `pipelineKey` differs; plus the existing `tests/regression/test_regression_pipeline_cache_apple.cpp` |
| R2 | `linkGen` is not bumped when the legacy program is regenerated | **HIGH** — a stale pipeline is returned for a new legacy shape | Bump `linkGen` on every regeneration (not just the first) | Regenerate the legacy program with a changed key and assert `linkGen` increased and the pipeline identity changed |
| R3 | The legacy program leaks into the public program namespace | **MED** — `glGetIntegerv(GL_CURRENT_PROGRAM)` / `glGetProgramiv` would show a non-GL object | Store it outside `Context::programs` (§4.4.3) | `test_legacy_dispatch.cpp`: after a legacy draw, `glGetIntegerv(GL_CURRENT_PROGRAM)` must still be 0 and `glGetProgramiv` must not list it |
| R4 | The legacy path diverges on the clip-space z convention | **HIGH** — every depth value is wrong; `glReadPixels` depth, `glDepthRange`, the accumulation buffer all wrong | Reuse `kZConv` verbatim (`src/gl/glsl_to_msl.cpp:10356`) | Pixel-comparison test: a legacy triangle and a modern triangle with the same transform must produce identical depth |
| R5 | The legacy path diverges on the viewport y-flip / front-face convention | **HIGH** — the image is mirrored and culling is inverted | Reuse `GLViewportToMetal` (`include/tglmt/GLConvert.h:21-45`) and the existing front-face inversion (`src/gl/gl_apple_draw.cpp:2688-2690`) | `tests/integration/test_cull_orientation_apple.cpp` and a legacy cull test |
| R6 | The `GL_QUAD_STRIP` vertex order is "simplified" to the naive order | **MED** — the winding of every quad flips; half the quads become back-facing | Keep the existing order (`src/gl/gl_legacy_vertex.cpp:150-155`); the header comment already warns about it | `tests/unit/test_legacy_immediate.cpp` already asserts the order; add a culling test |
| R7 | `GL_FLAT` is implemented with Metal's `[[flat]]` | **MED** — flat-shaded strips alternate colours | Duplicate the flat-source colour per primitive on the CPU (§4.7.2) | Pixel comparison of a `GL_FLAT` triangle strip against a reference |
| R8 | `GL_FLAT` with `GL_QUADS`/`GL_QUAD_STRIP` is applied per triangle instead of per quad | **MED** — one quad shows two colours | Compute the flat source at the **quad** level (§4.7.3) | Pixel comparison of a flat-shaded quad |
| R9 | The infinite-viewer vector is computed from the light position | **MED** — specular highlights are wrong for directional lights | Use the constant `(0,0,1)` (§4.2.3); note the correction to `docs/LEGACY_TO_METAL_MAPPING.md:730` | Pixel comparison of a specular highlight with `GL_LIGHT_MODEL_LOCAL_VIEWER` off and on |
| R10 | The light position is transformed by the modelview at *draw* time instead of at `glLightfv` time | **MED** — a later `glTranslate` moves the light (spec §2.14.2 says it must not) | Transform on the CPU in `glLightfv` | A test: set a light, then translate, then draw — the light must not have moved |
| R11 | The legacy vertex layout changes with the program key, invalidating the pipeline cache | **LOW** — extra pipeline compiles, not wrong output | The layout is a pure function of the `LegacyProgramKey` (§4.5) | Assert that the same key ⇒ the same `stride` and the same `attribs[]` |
| R12 | The generated MSL does not compile on the Metal compiler | **MED** — the legacy path silently produces nothing | Compile the MSL in a unit test through the device (the Apple backend can compile without a GPU — `compileLibrary` at `src/metal/AppleMetalBridge.mm:1783`) | `test_legacy_program_apple.cpp`: compile the generated MSL and assert no error |
| R13 | The legacy vertex buffer is uploaded per draw and costs too much | **MED** — the legacy path is 10× slower than it needs to be | Batch by primitive type and use the ring allocator (§4.5.2) | A benchmark comparing 10 000 quads in one `glBegin` block vs 10 000 `glBegin` blocks |
| R14 | The display-list recording switch is put into the **generated** `gl_c_exports.cpp` | **HIGH** — a generator change of 3533 lines; high regression risk | Two-stage plan (§6.3); the generator change is a separate, reviewable commit | `tools/gen_c_exports.py` round-trip: regenerate and diff ⇒ zero unexpected diff |
| R15 | The legacy path adds a state field to `StateTracker` | **MED** — regresses the modern path; makes the pipeline key carry irrelevant state | D-004: keep it in `Context::legacy` | The existing modern test suite stays green |
| R16 | `GL_CLIP_PLANE` is treated as natively supported | **MED** — 6 planes are requested, Metal gives 1 | Treat `clipPlaneCount > 1` as an emulation problem (fragment discard or CPU clipping) and say so in the docs | A test that clips with 6 planes and compares against a reference image |

---

## 10. Definition of Done

Per `other/prompt.md` §14.1, a legacy feature is only `FULLY_IMPLEMENTED`
when **all four** hold:

1. **A real code path** (`file:line`).
2. **A test ID that passes.**
3. **Comparison against the expected result** per the spec or a reference
   implementation.
4. **An environment label**: `A11_DEVICE` | `SIMULATOR` | `OTHER_GPU` |
   `REFERENCE_ONLY` | `NONE`.

**Current status of the legacy draw path:** `PARTIALLY_IMPLEMENTED`.
Reason: the state exists (`include/tglmt/LegacyState.h`), the assembly is
correct and tested (`src/gl/gl_legacy_vertex.cpp:100-162`,
`tests/unit/test_legacy_immediate.cpp`), but **there is no code path from the
assembled primitives to Metal**, and every test to date runs on the Null
backend → environment label `REFERENCE_ONLY`.

---

## 11. Not verified / open points

These are the things this document could **not** establish from the repository
and that must be checked before the corresponding code is written.

| # | Point | Why it is open |
|---|---|---|
| N1 | The exact Metal-side behaviour of `[[flat]]` on the **A11** device | The Metal Programming Guide statement ("first vertex") is the authority used here; the repo records it at `docs/HARDWARE_LIMITATIONS.md:122-131`, but no TGLMT test measures it on hardware. Needs an `A11_DEVICE` test. |
| N2 | The exact `MTLSamplerDescriptor` LOD-bias behaviour needed for `GL_TEXTURE_FILTER_CONTROL` | `docs/LEGACY_TO_METAL_MAPPING.md` §8.1 says Metal has no lod-bias in the sampler and proposes computing the bias in the shader; this has not been implemented or measured. |
| N3 | Whether the Metal compiler accepts the generated legacy MSL with the `tglmt_combine` helper and the 8-light array | Metal's uniform-buffer/array limits on A11 are documented in `docs/METAL_A11_CAPABILITIES.md`, but the *generated* program has never been compiled. |
| N4 | The A11 fragment-stage `[[stage_in]]` limit (31 vs 96; 60 vs 124 — the two Apple editions disagree) | Recorded as an open measurement in `PROGRESS.md`; the legacy program adds varyings on top of the modern ones. |
| N5 | Whether `glShadeModel(GL_FLAT)` with lighting on can be made exact without a second lighting evaluation | The design in §4.7.2 is plausible but unmeasured. |
| N6 | The behaviour of `GL_LINEAR` fog with `start == end` | Undefined in GL; the proposed deterministic behaviour is a TGLMT choice that must be recorded as a divergence. |
| N7 | The A11 cost of the legacy fragment shader (8 tex units × combine) | Needs a device benchmark; §4.2's design assumes it is acceptable. |

---

## 12. Cross-references

| Document | What it answers |
|---|---|
| `docs/OPENGL_TO_METAL_ARCHITECTURE.md` | How a GL call flows through TGLMT (overview for new readers) |
| `docs/IMPLEMENTATION_ROADMAP.md` | What to build, in what order, with the measured entry-point counts |
| `docs/LEGACY_TO_METAL_MAPPING.md` | Per-mechanism Metal strategy and MSL sketches |
| `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` | Per-stage entry-point plan, dependency graph, parallelisation |
| `docs/LEGACY_PIPELINE_AUDIT.md` | The 20 fixed-function groups, each with a status and evidence |
| `docs/METAL_FEATURE_MAPPING.md` | GL feature → Metal strategy (DIRECT / GENERATED_SHADER / GPU_EMU / CPU_EMU / HYBRID / LIMITED / BLOCKED) |
| `docs/METAL_A11_CAPABILITIES.md` | The A11 capability queries behind the `HARDWARE_LIMITED` verdicts |
| `docs/HARDWARE_LIMITATIONS.md` | Real hardware limits vs missing implementation |
| `docs/REPO_DISCOVERY.md`, `docs/SOURCE_MAP.md`, `docs/SUBSYSTEM_DEPENDENCY_GRAPH.md` | The measured repository structure this document maps onto |
| `docs/DECISIONS.md` | The locked architecture decisions (A1–A10, D-001…D-005) |
