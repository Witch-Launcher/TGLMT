# Legacy fixed-function → Metal (iOS / Apple A11) — mapping strategy

> Companion to [`docs/LEGACY_PIPELINE_AUDIT.md`](LEGACY_PIPELINE_AUDIT.md) (the
> spec-level audit) and
> [`docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md`](COMPATIBILITY_IMPLEMENTATION_PLAN.md)
> (the build order).
>
> **Every strategy in this document is a PLAN, not an implementation.** Nothing
> described here exists in the TGLMT source tree today; the audit measured the
> current state as *349 legacy entry points, 348 absent*. Where a claim is about
> TGLMT's existing code it is cited `file:line`; where it is about Metal it is
> cited to the vendored Apple PDFs or to the iOS SDK headers; where it is about
> the OpenGL specification it is cited to the spec section.

---

## 0. Classification scheme

| Class | Meaning |
|---|---|
| **DIRECT MAPPING** | A Metal API with equivalent semantics; the GL call becomes a Metal call. |
| **GENERATED_SHADER** | The fixed-function stage is reproduced by **generating MSL** that implements the spec's equations, driven by uniforms. |
| **GPU EMULATION** | Reproduced with additional GPU passes (extra render/compute passes, extra attachments). |
| **CPU EMULATION** | Reproduced on the CPU; the result is uploaded or fed back into the GL pipeline. |
| **HYBRID** | Partly GPU, part CPU; the split is stated per mechanism. |
| **HARDWARE LIMITED** | Metal on A11 cannot do it natively. Per `project/AGENTS.md` §6 this requires **(a)** a capability query or Apple documentation and **(b)** the emulation options considered with the reason for rejection. |
| **BLOCKED** | No viable path identified. |

The `HARDWARE_LIMITED` bar is not "Metal has no such API" — every such row below
carries a capability citation *and* a considered-options list.

---

## 1. The A11 / Metal constraints that drive every decision

### 1.1 What the device is

| Property | Value | Source |
|---|---|---|
| GPU | `MTLGPUFamilyApple4` (A11 Bionic) | `docs/apple/Metal-Feature-Set-Tables.pdf` p. 2 ("A11 Bionic · Metal · Apple4") |
| Metal programming model | **Metal 2.x** — *not* Metal 3/4 | same table, p. 2: Metal 3 & 4 starts at "A14 Bionic · Metal 3 & 4 · Apple7" |
| Max colour render targets / pass | 8 | feature tables p. 8 |
| Max MSAA samples | 4 | feature tables p. 9 |
| Max texture units per stage (sampler state table) | 16 | feature tables p. 7 ("Maximum number of entries in the sampler state argument table, per graphics or kernel functions": Apple4 = 16) |
| Max textures per stage (texture argument table) | 96 | feature tables p. 7 ("…texture argument table…": Apple4 = 96) |
| Max vertex attributes per vertex descriptor | 31 | feature tables p. 7 |
| Max constant-buffer arguments per stage | 31 | feature tables p. 7 |
| Max fragment `[[stage_in]]` components | 124 | feature tables p. 7 |
| Max `[[clip_distance]]` outputs | **1** | see §1.3 |
| Max point size | 511 px | feature tables p. 8 |
| Max viewports / scissor rects per vertex function | **1** on Apple4 (A11); 16 from Apple5 (A12) — a *hardware-family* limit, not a Metal-version one | feature tables p. 7, row "Maximum number of viewports and scissor rectangles, per vertex function": Apple2/3/4 = 1, Apple5…Apple10 = 16 |
| Programmable sample positions | supported (Apple2+) | feature tables p. 3 |
| Counting occlusion query | supported (Apple3+) | feature tables p. 3 |
| **Programmable blending** | **Metal 3 & 4 only** | feature tables p. 3, row "Programmable blending · Metal 3 & 4 · Apple2" — the *Metal* column says 3 & 4, the *hardware* column says the feature is available from Apple2 hardware, but the A11 runs Metal 2, so the API is not reachable |

### 1.2 The primitive-type gap

Metal's entire primitive vocabulary is five types
(`MTLRenderCommandEncoder.h`, iOS SDK):

```objc
typedef NS_ENUM(NSUInteger, MTLPrimitiveType) {
    MTLPrimitiveTypePoint = 0,
    MTLPrimitiveTypeLine = 1,
    MTLPrimitiveTypeLineStrip = 2,
    MTLPrimitiveTypeTriangle = 3,
    MTLPrimitiveTypeTriangleStrip = 4,
} API_AVAILABLE(macos(10.11), ios(8.0));
```

There is **no** quad primitive, **no** polygon primitive and **no** triangle
fan. TGLMT's `metal::PrimitiveType` mirrors this and adds two software-only
values with a comment saying exactly that
(`include/tglmt/MetalInterface.h:229-233`):

```cpp
enum class PrimitiveType : uint8_t {
    Point=0, Line=1, LineStrip=2, Triangle=3, TriangleStrip=4,
    Fan=5,    // GL_TRIANGLE_FAN — Metal không có fan; trace giữ đúng, GPU expand ở EmitDraw (M5b)
    Patches=6 // GL_PATCHES — đường tessellation drawPatches (xem gl_tess_xfb)
};
```

and `ToMTLPrim` (`src/metal/AppleMetalBridge.mm:240-251`) maps anything it does
not recognise to `MTLPrimitiveTypeTriangle`.

**Consequence:** `GL_QUADS`, `GL_QUAD_STRIP` and `GL_POLYGON` *must* be
triangulated on the CPU. `GL_TRIANGLE_FAN` and `GL_LINE_LOOP` must be
expanded. The modern path already does exactly this for fan and line-loop
(`src/gl/gl_apple_draw.cpp:1937-2000`); the legacy path reuses that pattern.

### 1.3 The clip-distance gap (the one hard limit that bites)

The MSL specification allows `[[clip_distance]]` to be declared as `float` or
`float[n]` (MSL spec table 5.4, "Attributes for vertex function return type"),
with `n` known at compile time. **The Metal compiler rejects the array form.**
TGLMT measured this and hard-fails on it
(`src/gl/glsl_to_msl.cpp:6024-6027` and `:6072`):

```cpp
// Metal [[clip_distance]] chỉ nhận MỘT scalar (mảng/ nhiều field bị compiler
// từ chối). ...
    if (idx != 0) {
        err = "gl_ClipDistance[" + std::to_string(idx) +
              "] chưa hỗ trợ (Metal chỉ 1 clip_distance)";
        return false;
    }
```

TGLMT's own A11 limitations document records the same conclusion
(`docs/METAL_A11_LIMITATIONS.md` §2.6) and notes that
`GL_MAX_CLIP_DISTANCES` is **over-reported** as 8 by the modern path
(`src/gl/gl_get.cpp:303`).

This matters for the legacy `GL_CLIP_PLANE0..5` (six user clip planes): only
**one** can be expressed natively. See §7.3 for the full options analysis.

### 1.4 The clip-space z and viewport conventions TGLMT already uses

The modern path bakes the GL→Metal clip-space z conversion into every vertex
shader it generates (`src/gl/glsl_to_msl.cpp:10356`):

```cpp
const std::string kZConv = " _out.tglmt_position.z = _out.tglmt_position.z * 0.5 + _out.tglmt_position.w * 0.5;";
```

which maps GL's clip `z ∈ [−w, w]` onto Metal's `z ∈ [0, w]`. The viewport's
`znear`/`zfar` are then passed through unchanged, because the conversion has
already been done (`include/tglmt/GLConvert.h:16-20`, `:36-37`):

```cpp
// Depth n/f: VS converter ĐÃ bake kZConv (GL clip z [-w,w] → Metal NDC [0,w]) nên
// z ra rasterizer = GL window depth chuẩn hóa [0,1]. ...
    vp.n = n; // glDepthRange n/f passthrough (shader đã convert z → [0,1]) — xem trên
    vp.f = f;
```

**The legacy path must use exactly the same convention**, otherwise
`glDepthRange`, `glReadPixels` depth values and the accumulation buffer's
depth semantics all shift.

The y-flip is also already established: TGLMT keeps framebuffer memory in
**GL order** (row 0 = bottom), so a lower-left-origin GL viewport is expressed
as a **negative-height** Metal viewport (`include/tglmt/GLConvert.h:24-34`):

```cpp
    } else {
        vp.y = (double)y + (double)h; // NDC y=-1 → hàng memory y (đáy GL)
        vp.h = -(double)h;            // negative viewport height = lật y
    }
```

Because that flip mirrors the winding, the modern path inverts the front-face
convention when the clip origin is not upper-left
(`src/gl/gl_apple_draw.cpp:2689-2690`):

```cpp
        if (c.state.ClipOrigin() != 0x8CA2)
            frontF = (frontF == 0x0900) ? 0x0901u : 0x0900u;
```

**The legacy path inherits all of this for free** if it reuses the same
viewport state and the same `GLViewportToMetal` conversion. That is a large
part of why the legacy path is a shader-generation problem and not a
coordinate-system problem.

### 1.5 Provoking vertex / flat shading — a genuine divergence

| Concept | GL 2.1 compatibility | Metal |
|---|---|---|
| Which vertex supplies the flat value | **depends on the primitive type** — glspec21 table 2.12: single polygon → vertex 1; triangle strip *i* → vertex *i*+2; triangle fan *i* → vertex *i*+2; independent triangle → vertex 3*i*; quad strip *i* → vertex 2*i*+2; independent quad → vertex 4*i* | MSL `[[flat]]` takes the **provoking vertex**, which for Metal is the **last** vertex of the primitive |
| The modern core profile | `GL_LAST_VERTEX_CONVENTION` is the default (`src/gl/gl_get.cpp:334` reports `0x8E4E`) — i.e. GL 4.x agrees with Metal | — |

So GL 2.1's `glShadeModel(GL_FLAT)` **cannot** be expressed with Metal's
`[[flat]]` interpolation: for a triangle strip GL wants vertex *i*+2 while
Metal gives the last vertex of the triangle, which for a strip is a different
vertex depending on the parity of *i*.

**Options considered:**
1. *Use `[[flat]]` and accept the difference.* Rejected: the divergence is
   visible (a strip's quads would alternate between two colours instead of a
   consistent one) and it is exactly the kind of silent wrongness this project
   forbids.
2. *Duplicate the flat source vertex's colour to every vertex of the
   primitive.* **Selected.** The legacy path already assembles primitives on
   the CPU, so at assembly time it knows which vertex is the flat source for
   each primitive. It writes that vertex's colour into the *colour* attribute
   of all the primitive's vertices; the shader then interpolates a constant.
   Cost: zero GPU work, one CPU write per vertex. It is exact for the
   "colour" case. The lighting case is subtler — see §6.4 — and is handled by
   resolving the flat colour on the CPU when lighting is enabled.

---

## 2. The architecture: one legacy state block, one generated shader, one pipeline key

This is the central design decision of the compatibility work and it answers
the question "how does the legacy fixed-function pipeline coexist with the
modern programmable pipeline in the same context?".

### 2.1 The rule that makes it possible

glspec21 §2.15 (quoted in the audit §0.3):

> "If the program object has no vertex shader, or no program object is currently
> in use, the fixed-function method for processing vertices is used instead."

And glspec46 §1.2.4:

> "…the compatibility profile of OpenGL continues to support the legacy OpenGL
> commands developed for such fixed-function hardware, although they are
> typically implemented by writing shaders which reproduce the operation of
> such hardware."

So a compatibility-profile context holds **both** pipelines and picks between
them **per draw**, based on whether a program object is in use. There is no
"legacy mode" and "modern mode"; there is one draw path with two program
sources.

### 2.2 The state block

A single owner for all fixed-function state, living next to (not inside) the
existing `StateTracker`:

```
include/tglmt/LegacyState.h        (new)
    struct LegacyState {
        // --- transform (L03) ---
        MatrixStack  modelview, projection, color;      // + per-unit texture stacks
        GLenum       matrixMode;
        // --- current vertex state (L01) ---
        float4       curColor, curSecondaryColor;
        float3       curNormal;
        float4       curTexCoord[kMaxTextureUnits];
        float        curFogCoord;
        GLuint       curIndex;
        bool         curEdgeFlag;
        // --- lighting (L04) ---
        Light        lights[kMaxLights];                // 10 params each
        Material     matFront, matBack;                // 5 params each
        LightModel   lightModel;                       // ambient, localViewer, twoSide, colorControl
        bool         lightingEnabled, colorMaterialEnabled;
        GLenum       shadeModel;                       // GL_SMOOTH / GL_FLAT
        // --- fog (L05) ---
        Fog          fog;                              // mode, density, start, end, color, coordSrc
        // --- texture environment (L06) ---
        TexEnv       texEnv[kMaxTextureUnits];          // mode, color, combine args
        TexGen       texGen[kMaxTextureUnits][4];        // mode, objectPlane, eyePlane
        // --- rasterisation (L13/L14/L16) ---
        AlphaTest    alphaTest;
        ClipPlane    clipPlanes[6];
        GLuint       polygonStipple[32];               // 32 rows x 8 bits
        // --- pixel ops (L11/L12) ---
        PixelTransfer pixelTransfer;
        GLuint       pixelMaps[...];
        float4       rasterPos; bool rasterPosValid;
        // --- special functions (L07/L08/L09/L10) ---
        DisplayLists displayLists;
        Evaluators   evaluators;
        Selection    selection;                         // name stack, select buffer
        Feedback     feedback;
        AccumBuffer  accum;
        // --- stacks (L17) ---
        AttribStack  serverAttribStack, clientAttribStack;
    };
```

`Context` gains one `LegacyState legacy;` member next to `StateTracker state`
(`include/tglmt/Context.h:638`). Keeping it separate from `StateTracker` is
deliberate: `StateTracker` is the *core-profile* state machine and the modern
path must not change behaviour (backward-compatibility requirement). A separate
block means the modern draw path never reads it, so there is zero risk of
regressing the 185 existing tests.

### 2.3 The generated shader

The legacy pipeline is **one generated MSL program**, not a family of them. A
single program with all the fixed-function stages present, controlled by
uniforms and by compile-time constants chosen when the program is generated.

Because the shader must be generated *after* the legacy state is known (at
least: which features are enabled), the program is generated **lazily on first
use** and cached. The cache key is the *shape* of the legacy state, not its
values:

```
LegacyProgramKey {
    bool  lighting;        // GL_LIGHTING enabled
    GLenum shadeModel;     // SMOOTH / FLAT
    bool  fog;
    bool  alphaTest;
    bool  clipPlanes;      // any of the 6 enabled
    bool  polygonStipple;
    bool  texEnvCombine;   // any unit uses GL_COMBINE
    uint8_t numTexUnits;   // highest active unit + 1
    uint8_t numTexGen;     // number of enabled texgen coords
    bool  separateSpecular;// GL_SEPARATE_SPECULAR_COLOR
    bool  twoSided;
};
```

Values that change *per draw* (light intensities, material, fog parameters,
matrices, texenv colours, the environment mode) live in a **uniform block**, not
in the program. So changing a light's colour does not regenerate the program
and does not invalidate the pipeline.

### 2.4 The pipeline key

This is the part that must be exactly right or the pipeline cache will hand
back the wrong program.

TGLMT's pipeline cache is two-level:

* `Context::PipelineKey` (`include/tglmt/Context.h:1015-1082`) — the CPU-side
  key, compared field by field in `operator==`, used by the 8-entry MRU cache
  `pipeCache[]` (`include/tglmt/Context.h:1091-1097`) to skip the bridge
  lookup.
* The bridge's own string key in `makeCustomPipeline`
  (`src/metal/AppleMetalBridge.mm:2705-2782`), which concatenates the two
  library identities, the entry-point names, the pixel format, the stride, the
  attribute list, the depth/stencil description and the per-attachment blend
  descriptors.

**The existing key already contains everything a second program needs.** It has
`vsLib` and `fsLib` — the `ILibrary::identity()` of the two shader libraries
(`include/tglmt/Context.h:1019-1020`, `include/tglmt/MetalInterface.h:316-323`)
— and `programGen`, a per-program link generation counter
(`include/tglmt/Context.h:222`). The library identity is a monotonic
serial assigned at compile time (`src/metal/AppleMetalBridge.mm:437-448`):

```cpp
static std::atomic<uint64_t> gLibrarySerial{1};
class AppleLibrary : public ILibrary {
    explicit AppleLibrary(id<MTLLibrary> l)
        : lib_(l), serial_(gLibrarySerial.fetch_add(1, std::memory_order_relaxed)) {}
    uint64_t identity() const override { return serial_; }
```

**Therefore: a legacy program is just another pair of libraries.** The legacy
MSL is compiled through the *same* `IDevice::compileLibrary`
(`src/metal/AppleMetalBridge.mm:1783-1800`) and installed into the *same*
`PipelineKey.vsLib` / `PipelineKey.fsLib` fields. **No change to the pipeline
key structure is required at all.** The key's existing fields then:

* separate the legacy program from every modern program (different library
  serial ⇒ different `vsLib`/`fsLib`);
* separate two *different* legacy programs from each other (different serials);
* separate two *different generations* of the same legacy program (the
  `programGen` counter, incremented whenever the legacy program is regenerated);
* keep the legacy program and the modern program for the *same* draw
  configuration distinct.

The one thing the key does **not** cover, and must not have to: the legacy
program's *uniform contents*. Those are uploaded per draw through the uniform
staging path and are not part of pipeline identity — exactly like a modern
program's uniform values.

**Where the key does need a new field:** the *vertex descriptor*. The legacy
path uses a different attribute layout than the modern path (position, normal,
colour, texcoords, … at fixed locations), so the legacy draw must present its
own `CustomAttrib` array to `makeCustomPipeline`. The existing
`PipelineKey::attribs[16]` / `nAttribs` fields
(`include/tglmt/Context.h:1036-1037`, `:1070-1079`) already carry that, since
the key compares each attribute's location, size, type, normalisation, offset,
buffer index, stride and divisor. **So the existing key is sufficient here
too**, provided the legacy path always uses the *same* attribute layout for a
given `LegacyProgramKey`.

**Conclusion: no change to `Context::PipelineKey` is needed.** That is the
answer to "a pipeline key so the pipeline cache stays correct": the existing
key is already correct *provided* the legacy program is a real `ILibrary` pair
and the legacy path fills in the existing attribute fields. This is a
significant simplification and it is why the legacy work can reuse the modern
draw path wholesale.

### 2.5 How the two pipelines coexist in one context

```
                        glUseProgram(0) / no program in use
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
                       · same encoder reuse / batching
                       · same pipeline cache + key
                       · same uniform staging + ring allocator
                       · same viewport / depth / blend / stencil encoding
```

The dispatch point is the program lookup at
`src/gl/gl_apple_draw.cpp:1786-1805`:

```cpp
    ProgramObject pipelineProg;
    GLuint prog = c.state.BoundProgram();
    ...
    } else {
        auto pit = c.programs.find(prog);
        if (pit == c.programs.end() || !pit->second.linked) {
            c.appleStats.noProgram++; return false;
        }
        prp = &pit->second;
    }
    ProgramObject& pr = *prp;
```

Today `prog == 0` (no program) falls into the `noProgram` bail-out. The
compatibility work replaces that one branch with:

```cpp
    if (pit == c.programs.end() || !pit->second.linked) {
        if (c.legacy.active && prog == 0) {
            prp = &c.legacy.program;          // the generated legacy program
        } else {
            c.appleStats.noProgram++; return false;
        }
    }
```

The `ProgramObject` struct is reused as-is: the legacy program fills in
`appleVS`, `appleFS`, `linkGen`, `uniformLayout`, `vsUBSize`/`fsUBSize` and the
sampler lists, and the existing uniform-upload code
(`src/gl/gl_apple_draw.cpp:2817-2941`) then works unchanged.

**One subtlety:** `ProgramObject::linkGen` must change whenever the legacy
program is regenerated, because the pipeline cache uses it to avoid handing
back a stale pipeline for a relinked program. The legacy program cache must
therefore bump `linkGen` on every regeneration.

**A second subtlety:** the legacy program is *not* a GL program object. It must
not appear in `glGetIntegerv(GL_CURRENT_PROGRAM)`, must not be listed by
`glGetProgramiv`, and `glUseProgram` must not be able to select it. It is
therefore stored **outside** `Context::programs` — as a dedicated member of the
legacy state — and only *referenced* by the draw path.

### 2.6 Where the legacy state is uploaded

The legacy uniform block is a Metal **constant buffer**. The modern path
already reserves `buffer(16)` for the program's uniform block
(`src/gl/glsl_to_msl.cpp:12371`, `:12579`):

```cpp
    msl_out << "vertex TGLMT_VOut TGLMT_vs(TGLMT_VIn _in [[stage_in]],\n"
               "    constant TGLMTUniforms& tglmt_u [[buffer(16)]]";
```

and the modern draw path binds it with
`enc->setVertexBuffer(vsUbuf.get(), vsUoff, 16)` /
`enc->setFragmentBuffer(fsUbuf.get(), fsUoff, 16)`
(`src/gl/gl_apple_draw.cpp:2940-2941`).

The legacy path uses the **same slot 16** with a different struct type
(`TGLMTLegacyState`), bound to a different buffer. Since a draw uses either
the legacy program or a modern program — never both — the slot is free to be
reinterpreted. The uniform staging path already handles the "uniforms changed
→ re-upload" logic through the `Context::UniformCache`
(`include/tglmt/Context.h:1003-1010`, used at `src/gl/gl_apple_draw.cpp:2889-2938`),
comparing the staged bytes with the previous ones and skipping the upload when
they are identical. The legacy path uses the same mechanism.

**Buffer budget check:** the legacy program uses one constant buffer (slot 16),
at most 4 texture samplers per stage (16 sampler slots available on A11 — see
§1.1) and no vertex buffers beyond the attribute buffer. The modern path's
budget (16 = uniforms, 17+ = UBO/SSBO, 24+ = atomics, 30 = sample mask) is
untouched because the legacy path never has a modern program bound at the same
time.

---

## 3. Immediate mode → Metal (vertex batching and primitive expansion)

### 3.1 The batching strategy

**The problem.** `glBegin`/`glEnd` is a *client-side* construct: Metal has no
equivalent. Naïvely, one `glBegin`/`glEnd` pair = one Metal draw call, and an
application that draws 10 000 quads issues 10 000 `drawPrimitives` calls. On a
TBDR that is catastrophic: each draw carries the full vertex-processing and
rasterisation setup.

**The strategy: assemble on the CPU, batch by primitive type.**

```
glBegin(mode)
   → open a CPU assembly buffer (a std::vector<LegacyVertex>)

glVertex*(x,y,z,w)
   → append one fully-resolved vertex:
       position   (x,y,z,w)
       colour     = current colour (already resolved against glColorMaterial)
       normal     = current normal
       texcoord   = current texcoord for the active unit (and for every unit
                    whose texgen is enabled → the texgen is evaluated here,
                    on the CPU, because it is a function of the *object*
                    coordinates and the eye-plane coefficients)
       edgeFlag   = current edge flag

glEnd()
   → expand the assembled vertices into Metal primitives (below)
   → append the resulting (vertices, primitive-range) to the *frame's*
     pending-primitive batch, keyed by Metal primitive type
   → do NOT draw yet
```

The draw is issued **not** at `glEnd` but at the next point where the
ordering of the primitives could otherwise be violated — i.e. when a command
that changes the pipeline state, the render target, the viewport or the
primitive type arrives, or at flush/finish/present time.

**Why batching by primitive type is safe.** Within one `glBegin`/`glEnd` block
the primitive mode is fixed. Across blocks it may change. Two consecutive
blocks with the *same* mode and the *same* pipeline state produce the same
result whether they are drawn as one draw call or two — **provided** the
vertex attributes are carried in the vertex buffer rather than taken from
uniforms, which they are: position, colour, normal, texcoord are all
per-vertex. The one thing that is *not* per-vertex is the **current vertex
state** (colour, texcoord, …) — but that is resolved at `glVertex*` time
into the per-vertex attributes, so it is safe too.

**What must break a batch:**
* a change of primitive mode (different `MetalPrimitiveType`);
* a change of pipeline state (blend, depth func, cull mode, front face,
  viewport, scissor, depth bias, colour mask, …);
* a change of the *legacy* state that affects shading (lighting parameters,
  material, fog, texenv, texgen, current matrices, shade model) — because the
  generated program is the same but the *uniforms* differ, and a batched draw
  would apply the newest uniform values to all the batched vertices;
* a change of the render target / framebuffer binding;
* any command that reads the framebuffer (`glReadPixels`, `glCopyPixels`,
  `glAccum` with `RETURN`, …);
* entering/leaving selection or feedback mode;
* `glFlush` / `glFinish` / present.

**Result:** an application that draws 10 000 quads in one state gets **one**
`drawPrimitives(MTLPrimitiveTypeTriangle, …)` call with 60 000 vertices, not
10 000 calls. The CPU cost is one memcpy of the vertex array into the staging
buffer per batch.

**The staging buffer.** TGLMT already has exactly the right facility: the
per-frame ring allocator (`Context::RingAlloc`, `src/core/Context.cpp:539-561`)
with a 4 MB × 5-frame ring (`include/tglmt/Context.h:1194-1210`), already used
for uniform staging, index rewriting and the fan/line-loop expansion. The legacy
vertex array goes into the same ring, so **no new allocation path and no new
buffer-lifetime hazard** is introduced.

**The attribute layout.** The legacy path needs a fixed vertex layout. A
sensible one, matching what Metal can express in 31 attributes
(feature tables p. 7):

| location | attribute | type |
|---|---|---|
| 0 | `position` | float4 |
| 1 | `normal` | float3 |
| 2 | `color` | float4 |
| 3 | `secondaryColor` | float4 |
| 4 | `texcoord0` | float4 |
| 5 | `texcoord1` | float4 |
| … | … | … |
| 5+`n` | `texcoordN` | float4 |

With `GL_MAX_TEXTURE_UNITS` ≤ 32 and the legacy minimum of 2, the layout fits
comfortably. The `fogCoord` is derived in the shader from the eye-space
position, so it does not need its own attribute (except in the
`GL_FOG_COORD_SRC = GL_FOG_COORD` mode, where the client `glFogCoord*` value
is needed — that mode adds one more attribute, folded into the program key).

The stride is fixed for a given `LegacyProgramKey`, so the pipeline key's
existing `stride` field (`include/tglmt/Context.h:1025`) is stable.

### 3.2 Primitive expansion

| GL mode | Metal primitive | Expansion |
|---|---|---|
| `GL_POINTS` | `MTLPrimitiveTypePoint` | none |
| `GL_LINES` | `MTLPrimitiveTypeLine` | none (odd trailing vertex dropped) |
| `GL_LINE_STRIP` | `MTLPrimitiveTypeLineStrip` | none |
| `GL_LINE_LOOP` | `MTLPrimitiveTypeLineStrip` | **CPU**: append a copy of the first vertex at the end — the pattern already implemented at `src/gl/gl_apple_draw.cpp:1975-2000` |
| `GL_TRIANGLES` | `MTLPrimitiveTypeTriangle` | none (trailing `k` dropped) |
| `GL_TRIANGLE_STRIP` | `MTLPrimitiveTypeTriangleStrip` | none |
| `GL_TRIANGLE_FAN` | `MTLPrimitiveTypeTriangle` | **CPU**: fan → triangle list — the pattern already implemented at `src/gl/gl_apple_draw.cpp:1937-1974` |
| `GL_QUADS` | `MTLPrimitiveTypeTriangle` | **CPU**: quad (v0,v1,v2,v3) → triangles (v0,v1,v2) + (v0,v2,v3) |
| `GL_QUAD_STRIP` | `MTLPrimitiveTypeTriangle` | **CPU**: quad *i* = (v2i, v2i+1, v2i+3, v2i+2) → triangles (v2i, v2i+1, v2i+3) + (v2i, v2i+3, v2i+2). **The odd ordering is the spec's** (glspec21 §2.6.1) and must be preserved or the winding flips |
| `GL_POLYGON` | `MTLPrimitiveTypeTriangle` | **CPU**: fan from vertex 0 → (v0,vi,vi+1) for i = 1..n−2. Legal: the spec only guarantees convex polygons, and for non-convex ones the result "need only lie within the convex hull of the projected vertices" — a fan satisfies that |

The existing fan and line-loop expansions in
`src/gl/gl_apple_draw.cpp:1937-2000` are the template; the legacy path needs
the same two plus three new ones (quad, quad-strip, polygon).

**Winding consequence.** The fan/quad triangulations must preserve the
**original** winding, because the front-face determination happens on the
triangles. The spec's `GL_QUAD_STRIP` ordering (v2i, v2i+1, v2i+3, v2i+2) is
*not* the naive (v2i, v2i+1, v2i+2, v2i+3) — using the naive order flips the
winding of every quad in the strip and would make half of them back-facing.
This is a real, easy-to-get-wrong detail; it is called out here and in the
audit.

**Primitive counts and `gl_raster` statistics.** The modern path counts
primitives for `GL_ARB_pipeline_statistics_query` with `PrimCount`
(`src/gl/gl_draw.cpp:95-116`). The legacy path must count the *GL* primitives
(quads count as one quad, not two triangles) so the statistics match the GL
semantics.

---

## 4. Matrix stack → Metal

**Strategy: GENERATED_SHADER + CPU state.**

The matrix stack is pure CPU state (`LegacyState`), and its *result* reaches
the GPU as uniforms in the generated vertex shader:

```metal
struct TGLMTLegacyVSOut {
    float4 position [[position]];
    float4 color;
    float4 texcoord;
    float3 eyePos;      // for fog (FRAGMENT_DEPTH) and sphere map
    float  fogCoord;    // for FOG_COORD source
};

vertex TGLMTLegacyVSOut TGLMT_legacy_vs(
    float4 inPos     [[attribute(0)]],
    float3 inNormal  [[attribute(1)]],
    float4 inColor   [[attribute(2)]],
    float4 inTex0    [[attribute(4)]],
    constant TGLMTLegacyUniforms& u [[buffer(16)]])
{
    TGLMTLegacyVSOut o;
    float4 eye = u.modelView * inPos;
    o.position  = u.projection * eye;
    // GL clip z is [-w, w]; Metal wants [0, w] — the SAME conversion the
    // modern path bakes into its shaders (glsl_to_msl.cpp:10356 kZConv).
    o.position.z = o.position.z * 0.5 + o.position.w * 0.5;
    o.eyePos    = eye.xyz;
    o.fogCoord  = length(eye.xyz);          // GL_FRAGMENT_DEPTH source
    o.color     = inColor;
    o.texcoord  = inTex0;
    return o;
}
```

**The normal matrix** is the inverse-transpose of the model-view matrix's
upper-left 3×3 (glspec21 §2.11.3), computed on the CPU when the model-view
matrix changes and uploaded as a `float3x3`/`float4x4` uniform.

**`GL_RESCALE_NORMAL` / `GL_NORMALIZE`** are per-vertex operations applied
after the normal transform. `GL_NORMALIZE` is a unit-length normalisation;
`GL_RESCALE_NORMAL` scales by the magnitude of the original normal.

**Cost:** negligible. The matrices change at most once per draw and are
uploaded through the existing uniform staging path.

**Pipeline-key impact:** none — the matrices are uniform *values*, not program
shape. The vertex layout does not change.

---

## 5. Lighting, materials and shading model → Metal

**Strategy: GENERATED_SHADER (per-vertex lighting), with a CPU resolution of
the flat-shading case.**

### 5.1 Why per-vertex

glspec21 §2.14.1: "GL lighting computes colors **for each vertex** sent to the
GL." The lighting equation is evaluated per vertex and the result is
interpolated. So the lighting belongs in the **vertex** shader of the generated
program — not the fragment shader. This is not a simplification; it is the
spec, and doing it per-fragment would be a different (and wrong) result.

### 5.2 The generated lighting code

```metal
// --- material / light state, from the uniform block ---
struct TGLMTLegacyLight {
    float4 ambient;      // acli
    float4 diffuse;      // dcli
    float4 specular;     // scli
    float4 position;     // Ppli  (w = 0 → directional light at infinity)
    float3 spotDir;      // sdli  (already transformed by the model-view 3x3)
    float  spotExponent; // srli
    float  spotCutoff;   // crli  (degrees; 180 → no cutoff)
    float  k0, k1, k2;   // attenuation
};
struct TGLMTLegacyUniforms {
    float4x4 modelView;
    float4x4 projection;
    float3x3 normalMatrix;
    TGLMTLegacyLight lights[8];
    float4 matAmbientFront, matDiffuseFront, matSpecularFront, matEmissionFront;
    float4 matAmbientBack,  matDiffuseBack,  matSpecularBack,  matEmissionBack;
    float  matShininessFront, matShininessBack;
    float4 sceneAmbient;      // LIGHT_MODEL_AMBIENT
    bool   localViewer;       // LIGHT_MODEL_LOCAL_VIEWER
    bool   twoSided;          // LIGHT_MODEL_TWO_SIDE
    bool   separateSpecular;  // LIGHT_MODEL_COLOR_CONTROL
    uint   numLights;
    ...
};
```

The per-vertex lighting evaluation, following glspec21 §2.14.1 exactly:

```metal
    // --- normal transform (spec 2.11.3): n' = (n · M^-1), then rescale/normalize
    float3 nEye = u.normalMatrix * inNormal;
    if (u.normalize)  nEye = normalize(nEye);
    else if (u.rescaleNormal) nEye *= length(inNormal) / max(length(nEye), 1e-20);

    // --- per-face material selection (two-sided lighting uses the back material
    //     for back-facing primitives; the facing is known per fragment, so the
    //     material choice is deferred to the fragment stage when twoSided is on)
    float4 matA = u.matAmbientFront,  matD = u.matDiffuseFront;
    float4 matS = u.matSpecularFront, matE = u.matEmissionFront;
    float  shin = u.matShininessFront;

    // --- accumulated lighting (spec 2.14.1)
    float3 diffuseAcc  = float3(0.0);
    float3 specularAcc = float3(0.0);
    for (uint i = 0u; i < u.numLights; ++i) {
        TGLMTLegacyLight L = u.lights[i];
        float3 P = L.position.xyz;                  // already in eye space
        float3 Ldir = normalize(P - eye.xyz);
        float  dist = length(P - eye.xyz);
        float3 V = u.localViewer ? normalize(-eye.xyz) : normalize(P);  // local vs infinite viewer
        float3 H = normalize(Ldir + V);

        // spotlight (spec 2.14.1): angle between -Ldir and the spot direction
        float spot = 1.0;
        if (L.spotCutoff < 180.0) {
            float cd = dot(normalize(-Ldir), normalize(L.spotDir));
            if (cd < cos(L.spotCutoff * 0.01745329252)) spot = 0.0;
            else spot = pow(cd, L.spotExponent);
        }
        // attenuation (spec 2.14.1): 1/(k0 + k1*d + k2*d^2)
        float att = 1.0 / (L.k0 + L.k1 * dist + L.k2 * dist * dist);
        att *= spot;

        float ndotl = max(dot(nEye, Ldir), 0.0);
        diffuseAcc  += L.diffuse.rgb  * ndotl * att;
        specularAcc += L.specular.rgb * pow(max(dot(nEye, H), 0.0), shin) * att;
    }
    float4 lit = matD * diffuseAcc
               + matS * specularAcc
               + matE                       // emission is added directly
               + u.sceneAmbient * matA;     // scene ambient × material ambient
    o.color = lit * inColor.rgb;             // material × primary colour
```

**Design notes that matter:**

* **The light position and spot direction are transformed on the CPU at the
  time `glLight` is called** (glspec21 §2.14.2: "The current model-view matrix
  is applied to the position parameter … when that position is specified", and
  the spot direction by the upper-left 3×3 only). The shader therefore works
  entirely in eye space, and a later `glTranslate` does not move an already
  specified light. This is a *CPU-side* responsibility and is easy to get
  wrong.
* **At least 8 lights** (glspec21 §2.14.1). The uniform array is sized 8.
  A `GL_LIGHT0`…`GL_LIGHT7` enable selects which of the eight participate;
  the enabled ones are packed to the front of the array and `numLights` is set
  accordingly, so the shader loop is bounded by the *enabled* count.
* **Local vs infinite viewer**: `GL_LIGHT_MODEL_LOCAL_VIEWER` chooses between
  `V = normalize(eye − P)` and `V = normalize(P)` (the viewer at infinity).
  This is a branch on a uniform, i.e. free.
* **Two-sided lighting** selects the front or back material by the facing.
  The facing is a *per-fragment* property, but the lighting is per-vertex, so
  two-sided lighting with the *correct* material per face is only exactly
  reproducible if the facing is known per vertex — which it is not, in general.
  The pragmatic resolution used by real implementations (and the one planned
  here) is to select the material in the fragment stage using
  `[[front_facing]]`, which requires the *lighting* to be done per fragment
  for the two-sided case. **This is a documented, deliberate divergence**:
  under `GL_LIGHT_MODEL_TWO_SIDE`, TGLMT evaluates lighting per fragment
  instead of per vertex. It is called out in the audit and must be listed in
  the compatibility caveats. The alternative — evaluating lighting per vertex
  twice (once per face) and selecting in the fragment stage — doubles the
  vertex cost and still cannot be exact when a triangle spans the facing
  boundary.
* **`GL_SEPARATE_SPECULAR_COLOR`** (`GL_LIGHT_MODEL_COLOR_CONTROL`) is
  implemented by emitting the specular term into a separate varying and
  adding it in the fragment stage. This *is* exact, and it is why the
  `separateSpecular` flag is part of the program key.
* **`glColorMaterial`** is resolved on the CPU: when
  `GL_COLOR_MATERIAL` is enabled for `AMBIENT`/`DIFFUSE`/`SPECULAR`/`EMISSION`,
  the corresponding material parameter follows the current colour. The CPU
  assembler resolves it per vertex.

### 5.3 `glShadeModel(GL_FLAT)`

As established in §1.5, the flat source vertex depends on the primitive type
and does **not** match Metal's provoking vertex. The resolution:

* **Lighting off** (the common case for flat shading): the flat colour is the
  vertex's current colour. The CPU assembler knows the flat-source index for
  the primitive it is building and writes that colour into *every* vertex of
  the primitive. The shader then interpolates a constant. **Exact.**
* **Lighting on**: the flat colour is the *lit* colour of the flat-source
  vertex, which depends on that vertex's position and normal — values the
  other vertices do not have. Exact reproduction therefore requires either
  (a) evaluating lighting per primitive on the CPU, or (b) a per-primitive
  colour attribute.
  Option (b) is the good one: the CPU assembler already knows the flat-source
  vertex; it can compute the flat colour's *inputs* (position, normal,
  colour) and pass them as a **second set of attributes** for the flat source.
  The vertex shader evaluates lighting a second time on those and the
  fragment stage selects it. Cost: one extra lighting evaluation per vertex
  and three extra attributes — but it is exact.
  This is the design chosen; it is also why `shadeModel == GL_FLAT` is part of
  the program key (the shader has a different shape).

### 5.4 Pipeline-key impact

`lighting`, `shadeModel`, `separateSpecular`, `twoSided` and `numLights > 0`
all change the *shape* of the generated program and therefore produce a new
library identity, hence a new pipeline key entry. `numLights` itself does
**not** need to be in the key (it is a uniform; the loop bound is a uniform).

---

## 6. Fog → Metal

**Strategy: GENERATED_SHADER (fragment stage), after texturing.**

Fog is applied to the **post-texturing** colour (glspec21 §3.10), so in the
generated fragment shader it comes *after* the texture-environment step and
*before* the per-fragment operations (alpha test, blending, …).

```metal
    // --- after the texture environment step ---
    float c = (u.fogCoordSrc == GL_FOG_COORD) ? in.fogCoord : length(in.eyePos);
    float f;
    if (u.fogMode == GL_LINEAR) {
        f = (u.fogEnd - c) / (u.fogEnd - u.fogStart);
    } else if (u.fogMode == GL_EXP) {
        f = exp(-u.fogDensity * c);
    } else {                       // GL_EXP2
        f = exp(-(u.fogDensity * c) * (u.fogDensity * c));
    }
    f = clamp(f, 0.0, 1.0);         // spec 3.10: the result is clamped to [0,1]
    color.rgb = f * color.rgb + (1.0 - f) * u.fogColor.rgb;   // alpha unchanged
```

**Semantic points:**

* `f` is clamped to `[0,1]` (glspec21 §3.10).
* **Alpha is not changed** by fog.
* The three modes are `GL_LINEAR` (eq. 3.33), `GL_EXP` (eq. 3.31) and
  `GL_EXP2` (eq. 3.32).
* `GL_LINEAR` with `start == end` is undefined in GL — TGLMT must pick a
  deterministic behaviour (division by zero). The plan: if `end == start`, use
  `f = (c >= end) ? 0 : 1` (a hard step), which is what the undefined case
  degenerates to in practice, and record it as a divergence.
* `GL_FOG_COORD_SRC` selects between the interpolated fog coordinate (from
  `glFogCoord*`, a client attribute) and the eye-space distance. The
  `GL_FOG_COORD` source needs the client attribute, so it changes the vertex
  layout → it is part of the program key.
* `GL_FOG_HINT` chooses between the exact exponential and a cheaper
  approximation; because both are available in the same shader, the hint is a
  uniform branch, not a program-shape change.

**Cost:** one `exp`/`pow` and one lerp per fragment. On A11 that is cheap
relative to the texture fetch that precedes it.

---

## 7. Legacy rasterisation → Metal

### 7.1 Alpha test — GENERATED_SHADER (exact)

The alpha test is a fragment discard:

```metal
    if (u.alphaTestEnabled) {
        float a = color.a;
        bool pass = false;
        switch (u.alphaFunc) {
            case 0: pass = false; break;                       // GL_NEVER
            case 1: pass = true;  break;                       // GL_ALWAYS
            case 2: pass = (a <  u.alphaRef); break;           // GL_LESS
            case 3: pass = (a <= u.alphaRef); break;           // GL_LEQUAL
            case 4: pass = (a == u.alphaRef); break;           // GL_EQUAL
            case 5: pass = (a >= u.alphaRef); break;           // GL_GEQUAL
            case 6: pass = (a >  u.alphaRef); break;           // GL_GREATER
            case 7: pass = (a != u.alphaRef); break;           // GL_NOTEQUAL
        }
        if (!pass) discard_fragment();
    }
```

`ref` is clamped to `[0,1]` and the fragment's alpha is rounded to the
nearest integer before the comparison (glspec21 §4.1.4). This is exact, not
an approximation. Note the modern core profile has no alpha test at all
(it was removed in GL 3.2), so this is purely additive.

### 7.2 Polygon stipple — GENERATED_SHADER (exact)

Polygon stipple is a 32×32 bit mask in window coordinates (glspec21 §3.5.2).
The mask is uploaded as 32 × `uint8` and tested in the fragment shader against
`gl_FragCoord`:

```metal
    if (u.polygonStippleEnabled) {
        uint row = uint(gl_FragCoord.y) & 31u;
        uint col = uint(gl_FragCoord.x) & 31u;
        uint bits = u.polygonStipple[row];
        if (((bits >> col) & 1u) == 0u) discard_fragment();
    }
```

The spec's coordinate convention is the *window* coordinate with the origin in
the lower left; `gl_FragCoord` in Metal is upper-left origin, so the row index
must be flipped (`31 - row`) to match. This is a detail that must be
verified with a pixel test, not assumed.

### 7.3 User clip planes — HARDWARE LIMITED (6 planes, Metal gives 1)

**Evidence.**
* (a) *Capability:* Metal exposes exactly one `[[clip_distance]]` output. The
  MSL specification's table 5.4 permits `float[n]`, but **the Metal compiler
  rejects the array form**; TGLMT measured this and hard-fails with the
  message "Metal chỉ nhận MỘT scalar" (`src/gl/glsl_to_msl.cpp:6024-6027`,
  `:6072`). TGLMT's A11 limitations document records the same
  (`docs/METAL_A11_LIMITATIONS.md` §2.6) and the modern path over-reports
  `GL_MAX_CLIP_DISTANCES` as 8 (`src/gl/gl_get.cpp:303`).
* The legacy API needs **six** user clip planes: `GL_CLIP_PLANE0`…
  `GL_CLIP_PLANE5`, and glspec21 table 6.33 sets the minimum number of user
  clipping planes to **6**.

**Emulation options considered:**

1. *Use `[[clip_distance]]` for plane 0 and implement the remaining five in the
   fragment shader* (compute the eye-space signed distance per fragment and
   `discard_fragment()`). — **Rejected as not spec-exact**: GL's user clip
   planes are part of the *clip volume* (glspec21 §2.12) and clip *primitives*,
   not fragments. A fragment-level discard produces the same result only for
   fragments, not for the clipped primitive geometry; the difference shows at
   the clip boundary (the primitive is cut, not the pixels). It also cannot
   reproduce the fact that a fully-clipped primitive contributes nothing at all.
2. *Clip the vertices on the CPU before the draw* — i.e. implement
   Sutherland–Hodgman clipping of the assembled polygon against each enabled
   plane, in eye space, exactly as the spec's clip volume requires. — **This is
   the selected option.** It is exact, it happens on the CPU where the legacy
   assembler already works, and it costs nothing on the GPU. It does mean the
   legacy path cannot batch across a clip-plane change, and that a clipped
   polygon may become a fan rather than the original triangle list — which is
   correct, because the spec's clip volume produces new geometry anyway.
   The trade-off: the *depth* values of the clipped fragments are interpolated
   over the clipped polygon, which is what the spec requires.
3. *Reject `GL_CLIP_PLANE0..5` on A11.* — Rejected: it removes six
   compatibility entry points that are perfectly implementable.
4. *Use the `GL_CLIP_DISTANCE` extension (GL 3.0) with 8 planes.* — Rejected:
   it does not exist in the 1.x/2.1 API surface this work targets, and it
   would not help anyway: the Metal limit is 1 clip distance regardless.

**Conclusion: HYBRID.** Plane 0 can use the native `[[clip_distance]]` for
free, but for uniformity and exactness the plan is to clip all six planes on
the CPU (option 2). The native path is kept as an optimisation for the
single-plane case if measurement shows the CPU clip is a bottleneck.

### 7.4 Line stipple — GENERATED_SHADER via CPU line expansion

**Problem.** Line stipple is a *pattern along the line*, alternating
`factor` fragments on and off, with a 16-bit pattern, reset at each line
segment (glspec21 §3.4.2). Metal's line rasteriser gives the fragment shader
no information about the position *along* the line, so the fragment stage
cannot evaluate the pattern.

**Options considered:**

1. *Ignore the pattern and draw solid lines.* Rejected: visibly wrong.
2. *Use `glLineStipple`'s factor to select a subset of segments.* Rejected:
   that is not what stippling does.
3. *Expand lines into triangle strips on the CPU, with a per-vertex "distance
   along the line" attribute, and evaluate the pattern in the fragment
   shader.* — **Selected.** The legacy assembler already expands lines; adding
   a line-distance attribute and a width-1 quad per segment is a natural
   extension. The fragment shader computes
   `pattern[(uint)dist] & (1 << (uint)(dist / factor))` and discards. This is
   exact, including the per-segment pattern reset (the distance restarts at
   each segment).

   The cost is that stippled lines become triangle strips rather than Metal
   lines: 4 vertices per segment instead of 2, and no hardware line
   rasterisation. For a legacy application that uses stippling, this is a
   correctness-for-performance trade that is worth it. Unstippled lines keep
   the hardware path.
4. *Use MSAA to fake the pattern.* Rejected: MSAA is spatial, the pattern is
   temporal-along-the-line.

**Conclusion: GENERATED_SHADER + CPU expansion**, with the "stipple" flag in
the program key because the fragment stage differs.

### 7.5 Wide lines — HARDWARE LIMITED

**Evidence.**
* (a) *Capability:* Metal's line rasterisation is fixed at one pixel wide.
  There is no line-width API: the iOS SDK's `MTLRenderCommandEncoder` has
  `setViewport`, `setScissorRects`, `setDepthBias`, `setCullMode` … and
  **no** `setLineWidth`; `grep -rn "LineWidth" $SDK/Metal.framework/Headers/*.h`
  returns nothing. `MTLRenderPipelineDescriptor` likewise has no line-width
  property. Apple's own Metal feature tables list no line-width capability at
  all (the relevant rows are "Maximum size of a point primitive" and the
  primitive-type list), and the MSL specification has no line-width
  qualifier.
* TGLMT's own documentation records the same conclusion with the same kind
  of evidence (`docs/METAL_A11_LIMITATIONS.md` §2.4), and the modern path
  accordingly reports `GL_ALIASED_LINE_WIDTH_RANGE` as `(1,1)`
  (`src/gl/gl_get.cpp:527`, `:593`) and clamps `glLineWidth` accordingly.
* The **legacy** profile, unlike core, permits `glLineWidth > 1`
  (glspec46 App. E.2: "Wide lines and line stipple — LineWidth is not
  deprecated, but values greater than 1.0 will generate an INVALID_VALUE
  error" — that rule is the *core* profile's; the compatibility profile keeps
  wide lines legal).

**Emulation options considered:**

1. *Reject `glLineWidth > 1` with `GL_INVALID_VALUE`.* — Rejected as the
   primary answer: it is exactly the core profile's rule and would make the
   compatibility profile lie about its own capability.
2. *Expand wide lines into triangle strips on the CPU.* — **Selected.**
   Together with the line-stipple expansion (§7.4) this is the same
   machinery: a segment of width *w* becomes a quad, and the fragment stage
   (or the CPU) discards fragments outside the segment's span and applies the
   stipple. The classic caveat is **line joins and caps**: GL's wide-line
   rasterisation has defined join/cap behaviour that a naive quad-per-segment
   does not reproduce (gaps at convex joins, overdraw at concave joins).
   TGLMT must therefore either implement joins (adding the two extra
   triangles per join) or document the divergence. The plan is: implement
   mitred joins for the `GL_` default and document the cap behaviour.
3. *Use MSAA to approximate the width.* Rejected: MSAA is not a width
   mechanism.

**Conclusion: HARDWARE LIMITED natively, HYBRID in practice** — wide lines are
emulated by CPU expansion with a documented join/cap divergence, and
`GL_ALIASED_LINE_WIDTH_RANGE` must report the *emulated* range (which is
unbounded up to the implementation limit) rather than `(1,1)`.

### 7.6 Polygon mode `GL_POINT` / `GL_LINE` — CPU EMULATION

* `GL_LINE`: Metal's `setTriangleFillModeLines` (`include/tglmt/MetalInterface.h:408`,
  used at `src/gl/gl_apple_draw.cpp:2721-2727`) draws the *outline* of the
  triangles, not the polygon boundary. For a `GL_POLYGON` with more than three
  vertices the outline is wrong, so the legacy path expands the polygon into
  its boundary line loop on the CPU.
* `GL_POINT`: Metal has no point-mode polygon. The legacy path emits the
  polygon's vertices as points.
* `GL_POLYGON_MODE` face argument `FRONT`/`BACK`: the modern path collapses
  them (`src/core/StateTracker.cpp:80-86`); the compatibility profile needs
  them distinguished.

### 7.7 Logic op — HARDWARE LIMITED

**Evidence.**
* (a) *Capability:* Metal has **no logic-op stage**. `MTLBlendOperation`
  (iOS SDK `MTLRenderPipeline.h:53-64`) offers exactly
  `Add, Subtract, ReverseSubtract, Min, Max, Unspecialized` — the 16 bitwise
  operations of glspec21 table 4.3 (`CLEAR`, `AND`, `AND_REVERSE`, `COPY`,
  `AND_INVERTED`, `NOOP`, `XOR`, `OR`, `NOR`, `EQUIV`, `INVERT`, `OR_REVERSE`,
  `COPY_INVERTED`, `OR_INVERTED`, `NAND`, `SET`) are simply not expressible.
  Metal 3's *programmable blending* is the only extension point, and it
  requires the Metal 3 programming model, which the A11 does not have
  (feature tables p. 2: A11 = "Metal", i.e. Metal 2.x; p. 3: "Programmable
  blending · Metal 3 & 4"). Apple's own feature tables therefore say the
  feature is not reachable on A11.
* TGLMT's modern path already documents and *partially* implements this:
  `glLogicOp` stores the opcode (`src/gl/gl_raster.cpp:243-249`) and the blend
  stage is correctly **bypassed** when `GL_COLOR_LOGIC_OP` is enabled
  (`src/gl/gl_apple_draw.cpp:2320-2322`):

  ```cpp
        // GL 4.6 §17.3.9: COLOR_LOGIC_OP on ⇒ blend stage bypassed (logic op thay blend).
        ab.enabled = c.state.IsEnabled(0x0BE2, s) && !c.state.IsEnabled(0x0BF2) && ...
  ```

  but **the operation itself is never performed** — the framebuffer keeps its
  old value. `docs/METAL_A11_LIMITATIONS.md` §2.7 and
  `docs/OPENGL_46_METAL_MAPPING.md` both record this.

**Emulation options considered:**

1. *Leave it unimplemented (the modern path's current behaviour).* Rejected
   for the compatibility profile: `GL_COLOR_LOGIC_OP` is a *core* GL 1.0
   feature and legacy applications (early arcade/particle systems) use it.
2. *Perform the operation in the fragment shader.* — **Not possible**: the
   fragment shader cannot read the destination colour. Metal has no
   framebuffer fetch in a render pass (there is `shaderFramebufferFetch` in
   Metal 3 for tile shaders only, and it is Metal 3 / Apple7+ — not A11).
3. *Two-pass approach: render to an offscreen texture, then a full-screen pass
   that reads both the source and the destination and writes the logical
   result.* — **Selected.** Cost: one extra full-screen pass and one extra
   texture per logic-op draw, plus a copy of the destination. This is
   correct but expensive, and it breaks the "one draw per batch" property
   (the logic-op draws must be isolated). Because it is expensive, the legacy
   path should only take this route when `GL_COLOR_LOGIC_OP` is actually
   enabled — which is the normal case for the applications that need it, and
   never the case for the applications that don't.
4. *CPU implementation.* Rejected: the framebuffer is not CPU-readable at
   draw time (and reading it back would serialise the pipeline completely).

**Conclusion: HARDWARE LIMITED, with GPU EMULATION (two-pass) as the
implementation.** The capability is real and the emulation is expensive; the
plan must therefore (a) implement the two-pass path, (b) report
`GL_COLOR_LOGIC_OP` honestly, and (c) measure the cost, because an
application that enables logic op for every draw will see a large slowdown.

### 7.8 Dithering — GENERATED_SHADER

Dithering selects between two neighbouring colour values based on the
fragment's screen coordinates (glspec21 §4.1.9). It is expressible in the
fragment shader with an ordered-dither matrix:

```metal
    if (u.ditherEnabled) {
        // 4x4 ordered Bayer matrix, standard GL-compatible approximation
        static const uint bayer[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };
        uint d = bayer[(uint(gl_FragCoord.y) & 3u) * 4u + (uint(gl_FragCoord.x) & 3u)];
        float threshold = (float(d) + 0.5) / 16.0;
        color.rgb = floor(color.rgb * 255.0 + threshold) / 255.0;
    }
```

This is the *typical* implementation of GL dithering; the spec leaves the
dithering function implementation-defined, so an ordered dither is conformant.
The dither is applied after blending, before the logic op (glspec21 figure 4.1).

### 7.9 Stipple / alpha test / dither / logic op — pipeline-key impact

All four are uniform-driven *except*:
* `polygonStipple` needs a 32-byte constant buffer ⇒ a different buffer
  binding, not a different program. It can live in the same uniform block as a
  `uint8[32]` array.
* `logicOp` needs a whole different rendering strategy (two-pass) ⇒ not a
  program-shape change but a *path* change.

So none of them changes the pipeline key. They change the **uniform contents**
and, in the logic-op case, the **draw strategy**.

---

## 8. Texture environment and texture generation → Metal

### 8.1 Texture environment — GENERATED_SHADER

The texture environment is a per-unit function of the source colour and the
previous result. In the generated fragment shader, one block per texture unit:

```metal
    float4 texColor = texture2d<float>(tex0, samp0, in.texcoord.xy);   // or the unit's own coords

    // --- GL_REPLACE / GL_MODULATE / GL_DECAL / GL_BLEND / GL_ADD (glspec21 tables 3.21/3.22)
    float4 prev = color;      // "Cp, Ap" — the previous unit's result
    float4 src  = texColor;   // "Cs, As" — the filtered texture colour
    float4 env  = u.texEnvColor[0];

    switch (u.texEnvMode[0]) {
        case GL_REPLACE:  color = src;                                  break;
        case GL_MODULATE: color = prev * src;                           break;
        case GL_DECAL:
            color.rgb = prev.rgb * (1.0 - src.a) + src.rgb * src.a;
            color.a   = prev.a;
            break;
        case GL_BLEND:
            color.rgb = prev.rgb * (1.0 - src.rgb) + env.rgb * src.rgb;
            color.a   = prev.a;
            break;
        case GL_ADD:
            color.rgb = prev.rgb + src.rgb;
            color.a   = prev.a + src.a;
            break;
        case GL_COMBINE: color = combineRGB(prev, src, env, u.combineArgs[0]); break;
    }
```

The `GL_COMBINE` mode (GL 1.3) is the general form: three sources, each with
an RGB source (`SRC0_RGB`, `SRC1_RGB`, `SRC2_RGB`), an alpha source, and an
operand (`SRC_COLOR`, `ONE_MINUS_SRC_COLOR`, `SRC_ALPHA`,
`ONE_MINUS_SRC_ALPHA`, `DST_COLOR`, `ONE_MINUS_DST_COLOR`, `DST_ALPHA`,
`ONE_MINUS_DST_ALPHA`, `CONSTANT_COLOR`, `ONE_MINUS_CONSTANT_COLOR`,
`CONSTANT_ALPHA`, `ONE_MINUS_CONSTANT_ALPHA`) applied to both the RGB and the
alpha channel, then combined by `COMBINE_RGB` / `COMBINE_ALPHA`
(`REPLACE`, `MODULATE`, `ADD`, `ADD_SIGNED`, `INTERPOLATE`, `SUBTRACT`,
`DOT3_RGB`, `DOT3_RGBA`) and finally scaled by `RGB_SCALE` / `ALPHA_SCALE`.
`ADD_SIGNED` clamps the operand to `[−1,1]` before adding; `INTERPOLATE` is
`a + 2b + c − 1`; the `DOT3` modes produce a 4-component dot product that is
replicated.

The full combiner is a large but entirely mechanical function. It is emitted
into the generated fragment shader; the *arguments* are uniforms.

**Multi-texturing.** Each unit contributes one block, chained: unit 0's result
becomes `prev` for unit 1, etc. The number of units is a uniform-driven loop
bound (the shader has a fixed maximum of 16 samplers — the A11 sampler-table
limit, feature tables p. 7 — and the loop is bounded by the highest active
unit).

**`GL_TEXTURE_FILTER_CONTROL` / `TEXTURE_LOD_BIAS`** biases the level of
detail for the unit. Metal's sampler has a `lodMin`/`lodMax` pair
(`include/tglmt/MetalInterface.h:161-162`) but **no** lod-bias parameter in
`MTLSamplerDescriptor` — the bias is applied in the shader by offsetting the
computed LOD. MSL's `sample()` takes an explicit `level`/`gradient` overload
(`texture2d.sample(s, uv, level(...))`), so the legacy path must compute the
bias-adjusted LOD itself: `level = log2(max(dot(dx,dx), dot(dy,dy))) + bias`.
This requires the fragment derivatives, which MSL provides via `dpdx`/`dpdy`.

**Cost:** one texture fetch and ~10 ALU per active unit per fragment. For the
common single-unit case, negligible over the fetch.

### 8.2 Texture coordinate generation — HYBRID (CPU for object/eye-linear, GPU for sphere/reflection)

The generation functions (glspec21 §2.11.4) split cleanly:

* `GL_OBJECT_LINEAR` and `GL_EYE_LINEAR` are **linear functions of the vertex
  coordinates**. They are evaluated per vertex and depend only on the plane
  coefficients and (for eye-linear) the model-view matrix. **Evaluated on the
  CPU** in the legacy assembler, and written into the per-vertex texcoord
  attribute. This is exact, free, and avoids a per-vertex branch.
* `GL_SPHERE_MAP`, `GL_REFLECTION_MAP` and `GL_NORMAL_MAP` need the eye-space
  position and the eye-space normal, both of which the vertex shader has.
  **Evaluated in the generated vertex shader**:

  ```metal
      // sphere map (glspec21 2.11.4)
      float3 u = normalize(eye.xyz);
      float3 nf = normalize(nEye);
      float3 r = u - 2.0 * nf * dot(nf, u);
      float m = 2.0 * r.x * r.x + r.y * r.y + (r.z + 1.0) * (r.z + 1.0);
      o.texcoord = float4(r.x / m + 0.5, r.y / m + 0.5, 0.0, 1.0);
  ```

  Note that the *eye* coordinates used are the ones **after** the model-view
  transform, which is why these three modes cannot be done on the CPU in the
  assembler (the assembler has the object coordinates and the matrix, so it
  *could* — but doing it in the shader keeps the attribute count down).

**Which modes are enabled changes the program shape** (the shader needs the
sphere-map code only if a unit uses it) ⇒ `texGen` is part of the program key.

**`GL_NORMAL_MAP` is GL 1.4**; `GL_SPHERE_MAP` and `GL_REFLECTION_MAP` are
GL 1.0.

### 8.3 `GL_POINT_SPRITE` texture-env target

`glTexEnv(GL_POINT_SPRITE, GL_POINT_SPRITE_COORD_ORIGIN, …)` controls whether
point-sprite texture coordinates run top-left or bottom-left. TGLMT's modern
path already accepts `GL_POINT_SPRITE_COORD_ORIGIN`
(`src/gl/gl_raster.cpp:170-184`) and validates the two legal values. The
legacy path must additionally *apply* it in the fragment shader when
`GL_POINT_SPRITE` is enabled. Metal's `[[point_coord]]` is available (MSL
spec table 5.4), and its origin convention is top-left — so the two cases
differ by a `1 - y` flip.

---

## 9. Display lists → Metal

**Strategy: CPU EMULATION — a command stream, not a cached draw.**

### 9.1 Why a command stream and not a cached draw

The alternatives are:

1. **Cached draw** (compile the list into a vertex buffer and re-issue one
   draw). — **Rejected.** It breaks the spec's central rule: "the commands
   within the list to be executed just as if they were given normally. The only
   exception pertains to commands that rely upon client state" — i.e. **server**
   state (the matrices, the lighting, the texture environment) is evaluated at
   *execution* time. A list containing `glTranslate(1,0,0)` followed by a
   `glBegin/glEnd` block must translate by the matrix that is current when the
   list is *called*, not when it was compiled. A cached draw bakes the compile
   time matrix. This is exactly the sort of silent wrongness that a legacy
   application would never detect.
2. **Command stream** (record the commands and their arguments, replay them on
   `glCallList`). — **Selected.** It is the only representation that preserves
   the spec's semantics exactly.

### 9.2 What must be recorded and what must not

The spec has an explicit list of commands that are **executed immediately**
rather than compiled into the list (glspec21 §5.4, reproduced in the audit
§8). That list is long and it is normative. It includes:

* display-list management (`GenLists`, `DeleteLists`);
* render modes (`RenderMode`, `SelectBuffer`, `FeedbackBuffer`);
* all the vertex-array / client-state commands;
* `PixelStore`, `ReadPixels`;
* texture and query object management;
* all buffer-object commands;
* all program/shader object commands;
* `Finish`, `Flush`;
* **every `Get*` and `Is*` query**.

The implementation therefore needs a **command-recording mode** in the GL
front-end: while a list is open, the dispatcher either records the command or
executes it immediately, according to the spec's list.

Because the GL surface is ~700 entry points, the recording switch must live in
the **dispatcher**, not in each entry point. TGLMT's public C ABI is a
generated flat wrapper file (`src/gl/gl_c_exports.cpp`, generated by
`tools/gen_c_exports.py`) that forwards 1:1 into `tglmt::gl::`. The
dispatcher is the natural place: a `Context::displayListCapture` flag that
the wrapper consults.

**However** — this is a large and invasive change to a generated file, and it
is the single biggest risk in the whole compatibility programme. The plan
therefore splits it:

* **Stage 1 (minimum viable):** record only the commands the legacy
  applications actually use — the immediate-mode block (`glBegin`/`glVertex*`/
  `glEnd`), the state-changing commands, and the draw commands. Everything
  else falls back to "execute immediately", which is wrong only for the
  commands the spec also says to execute immediately.
* **Stage 2 (complete):** implement the full dispatcher interception so the
  spec's "execute immediately" list is honoured exactly.

### 9.3 Semantics that must be preserved

| Rule | glspec21 | Consequence for the implementation |
|---|---|---|
| Client state at compile time, server state at execution time | §5.4 | the recorded command must carry the *client* values (the current colour, the current texcoord, the array pointers) captured at record time, and re-read the *server* state at replay time |
| Pointer arguments are dereferenced when the command is issued | §5.4 | a recorded `glDrawElements` must capture the *index data*, not the pointer |
| Buffer-object data is dereferenced at compile time | §5.4 | a recorded `glDrawElements` from a buffer must capture the index bytes, not the buffer id |
| `GL_COMPILE` does not execute; `GL_COMPILE_AND_EXECUTE` does | §5.4 | two modes, and the difference is observable through side effects |
| `glNewList(0)` ⇒ `INVALID_VALUE`; `glEndList` without `glNewList` ⇒ `INVALID_OPERATION`; `glNewList` twice ⇒ `INVALID_OPERATION`; `glCallList(0)` ⇒ `INVALID_VALUE` | §5.4 | explicit validation |
| Nesting limit ≥ 64 | §5.4 | a depth counter with a limit of at least 64 |
| `glGenLists` returns a *contiguous* block and creates an empty list for each | §5.4 | the allocator must find a contiguous run, not just N free ids |
| `glIsList` | §5.4 | a set membership test |
| `glDeleteLists(list, 0)` does nothing | §5.4 | explicit |
| `glCallLists` offset encoding by `type` (`BYTE`…`4_BYTES`) plus `glListBase` | §5.4 | the encoding table must be implemented exactly |
| Under `GL_COMPILE_AND_EXECUTE`, a nested `glCallList` executes the nested list but records the *call* | §5.4 | the recording must capture the call, not the expansion |
| `glEndList` failure ⇒ `OUT_OF_MEMORY`, and the previous list contents and all other state are unchanged | §5.4 | the recording must be transactional: build into a temporary and commit on `glEndList` |

### 9.4 Interaction with the legacy pipeline

A display list containing a `glBegin`/`glEnd` block replays through the
legacy path; a display list containing a modern draw command replays through
the modern path. Both are possible in the same compatibility context, and the
replay must therefore be *late-bound*: the recorded command says "draw this
vertex range with this primitive type", and at replay time the current program
binding decides which pipeline executes it.

### 9.5 Performance

A command-stream display list replays at GL-command speed, not at
Metal-draw speed. That is inherent to the semantics: the whole point of a
display list is that the *server state* is re-evaluated. The optimisation
available is to make the replay cheap:

* the recorded commands are stored as a compact tagged byte stream, not as
  C++ objects;
* the replay path is the same fast path as the direct GL path (the recorded
  commands call the same internal functions);
* a list that contains only state changes and one immediate-mode block
  collapses to: apply the state changes, then one vertex-array draw.

That last point is worth stating explicitly: for the overwhelmingly common
legacy pattern — build a list once, call it every frame with a changing
model-view matrix — the replay does the state application and **one**
`drawPrimitives`, which is exactly as fast as the equivalent modern code.

---

## 10. Evaluators → Metal

**Strategy: CPU EMULATION (Bernstein polynomial evaluation on the CPU).**

The evaluation is pure arithmetic on the control points; there is no Metal
analogue and no reason to involve the GPU. The result — a vertex — is fed into
the **same** legacy assembler that `glVertex*` uses, so the rest of the
pipeline (transform, lighting, rasterisation) is completely untouched, exactly
as the spec requires: "Transformations, lighting, primitive assembly,
rasterization, and per-pixel operations are not affected by the use of
evaluators" (glspec21 §5.1).

Implementation:

```
p′(u′) = Σᵢ Bⁿᵢ( (u′−u1)/(u2−u1) ) · Rᵢ
p′(u′,v′) = ΣᵢΣⱼ Bⁿᵢ(…u…) · Bᵐⱼ(…v…) · Rᵢⱼ
```

with `n = order − 1`, `m = vorder − 1`. The 1-D form is the special case
`m = 0`.

`glEvalMesh2` produces an `n1`×`n2` grid; `glEvalMesh1` produces `n1` points
or `n2` line segments. The generated vertices are appended to the legacy
assembler. The grid size is unbounded by the spec, so the assembler must grow
dynamically — it does, being a `std::vector`.

**`glMapGrid1/2`** defines the uniform grid in the `u`/`v` domain that
`glEvalMesh` uses. It changes only the sampling of the domain, not the
polynomial.

**Auto-generated normals.** The GL does not synthesise normals from a position
map; an application that wants normals supplies a `GL_MAP2_NORMAL` map and
the grid evaluates it. The plan implements `GL_MAP2_NORMAL` and
`GL_MAP2_TEXTURE_COORD_*` as first-class map targets, which gives the
application the same capability.

---

## 11. Selection and feedback → Metal

**Strategy: CPU EMULATION, evaluated from the assembled primitive geometry.**

### 11.1 Why CPU, not GPU

Both modes need per-primitive *geometric* information:

* selection needs, per intersecting primitive, the minimum and maximum
  **window-coordinate z** over its post-clipping vertices, plus the name stack
  at the time of the hit;
* feedback needs, per primitive, the window coordinates of its vertices and
  the associated colour/texture data.

Neither is available from a Metal draw call. A GPU approach would need an
"ID buffer" pass (render each primitive with a unique colour, read it back,
resolve the hits) — that is the classic technique, and it has two problems
here:

1. It only reports *fragments*, so it cannot report the **z range of the
   primitive's vertices**, which selection explicitly requires (glspec21 §5.2:
   "The minimum and maximum depth values are the minimum and maximum taken
   over all the window coordinate z values of each (post-clipping) vertex of
   each primitive").
2. It cannot produce the feedback *token stream* (the primitive-type tokens
   and the per-vertex layout of table 5.2) without a separate transform pass.

The legacy path, however, **already has the geometry on the CPU**: the
assembler knows every vertex of every primitive, and the legacy state knows
the matrices, the clip volume and the viewport. So the exact spec semantics
are computable directly.

### 11.2 Selection

For each primitive assembled while the render mode is `GL_SELECT`:

1. Transform its vertices to clip space (model-view × projection).
2. Clip against the clip volume — the same CPU clipper used for the user clip
   planes (§7.3) — and collect the window-space z of the surviving vertices.
3. If any vertex survives (i.e. the primitive intersects the clip volume),
   record a hit.
4. The hit record is: the current name-stack depth, the minimum z, the
   maximum z, and the name stack bottom-first. The depths are multiplied by
   `2³²−1` and rounded to the nearest unsigned integer (glspec21 §5.2).
5. The record is written into the user's select buffer; if it would overflow,
   as much as fits is written and the overflow flag is set.

The exact rules that must be honoured:

* a **culled** polygon does *not* hit (glspec21 §5.2);
* `glPolygonMode` does *not* affect the hit test — selection is "based on the
  polygon itself" (glspec21 §5.2);
* `glWindowPos` **always** generates a hit;
* a valid `glRasterPos` generates a hit;
* the name stack is cleared and the stack pointer reset whenever `glRenderMode`
  is called.

`glRenderMode(GL_SELECT)` returns the number of hit records copied, or −1 if
the selection array overflowed.

### 11.3 Feedback

For each primitive assembled while the render mode is `GL_FEEDBACK`, write a
block into the user's feedback buffer:

```
<primitive token> <vertex values>…
```

with the per-vertex value count from glspec21 table 5.2 (`GL_2D` → 2,
`GL_3D` → 3, `GL_3D_COLOR` → 3+k, `GL_3D_COLOR_TEXTURE` → 7+k,
`GL_4D_COLOR_TEXTURE` → 8+k, with k = 4 in RGBA mode). The coordinates are
**window** coordinates; `w` is in clip coordinates; no depth-offset
arithmetic is applied to z.

Feedback occurs *after* polygon culling and after `PolygonMode` interpretation
(glspec21 §5.3) — so the legacy path performs culling and the polygon-mode
interpretation before emitting the feedback block.

`glPassThrough(token)` inserts a marker token; it "may not occur between Begin
and End" (glspec21 §5.3).

`glRenderMode(GL_FEEDBACK)` returns the number of values written, or −1 on
overflow.

### 11.4 The "no framebuffer updates" rule

"In selection mode, framebuffer updates as described in chapter 4 are not
performed" (glspec21 §5.2), and the same for feedback mode. The legacy path
therefore **suppresses the draw entirely** when the render mode is not
`GL_RENDER` — no rasterisation happens at all. This is also why the CPU
approach is natural: there is nothing to rasterise.

---

## 12. Accumulation buffer → Metal

**Strategy: GPU EMULATION with a dedicated off-screen float target.**

Metal has no accumulation buffer (no such framebuffer attachment; the only
accumulation-like feature is `MTLStoreActionMultisampleResolve`, which is a
different operation). The four operations (glspec21 §4.2.4) map onto a
dedicated RGBA-float texture:

| Operation | Emulation |
|---|---|
| `GL_ACCUM` | read the current colour buffer, multiply by `value`, add to the accumulation texture. A full-screen quad with `MTLBlendOperationAdd` and the source pre-multiplied by `value`; or a compute pass |
| `GL_LOAD` | as `GL_ACCUM` but the accumulation texture is replaced rather than added to |
| `GL_RETURN` | multiply the accumulation texture by `value`, clamp to `[0,1]`, and write it into the enabled colour buffers — a full-screen textured quad with blending disabled |
| `GL_MULT` | multiply the accumulation texture by `value` in place |
| `GL_ADD` | multiply by `value` then add `value` |

The accumulation values are **signed in `[−1,1]`** and are clamped **only** on
`GL_RETURN` (glspec21 §4.2.4), so the accumulation texture must be a
floating-point format (RGBA16Float at minimum; RGBA32Float if the A11's
floating-point render-target support allows — the A11 supports RGBA16Float
render targets; RGBA32Float renderability is a separate question that must be
probed on device, `docs/METAL_A11_LIMITATIONS.md` and the existing
`metal::PixelFormat` list in `include/tglmt/MetalInterface.h:15-49`).

The scissor test restricts which pixels an `Accum` operation affects
(glspec21 §4.2.4) — the emulation quad must be scissored identically.

`glClearAccum` sets the accumulation texture's clear colour.

**Cost:** one full-screen pass per `glAccum` call. Legacy applications use
`glAccum` for multi-sample anti-aliasing (accumulate *N* jittered frames and
return the average), so the cost is *N* extra passes per frame. That is the
honest cost of the feature and it is what the alternative (CPU) would be far
worse at.

---

## 13. Client-side vertex arrays → Metal

**Strategy: CPU EMULATION (gather at draw time into the shared staging
buffer).**

Metal has no client-memory vertex arrays: the vertex data must live in an
`MTLBuffer`. The legacy path therefore **gathers** the client arrays on the
CPU at draw time, exactly as it gathers immediate-mode vertices, and uploads
them in the same batched draw.

This is not a workaround, it is the only option: the client arrays are read
"when the drawing command is issued" (glspec21 §2.8 semantics, preserved in
the compatibility profile), and the pointer is a raw host address that is
valid only at that moment.

**`glInterleavedArrays`** is expanded on the CPU into the equivalent
per-attribute descriptors. The interleaved formats are numerous
(`GL_V2F`, `GL_V3F`, `GL_C4UB_V2F`, … `GL_T4F_C4F_N3F_C4F_V2F`) but each is a
simple fixed stride and offset table.

**The interaction with the legacy assembler** is the important design point:
client arrays and immediate mode produce the *same* vertex struct, so they
share one vertex layout, one pipeline and one batching path. An application
that mixes them (draws some quads from client arrays and some in immediate
mode) gets correct ordering, because both feed the same batch.

**Cost:** a CPU copy of the vertex data per draw. This is the price of
client-memory arrays; there is no alternative on Metal.

---

## 14. Attribute stacks and state restoration → Metal

**Strategy: CPU EMULATION (snapshot copies of the legacy state block).**

Because every piece of fixed-function state is CPU-side in the legacy block
(§2.2), a push/pop is a **struct copy**. The mask selects which groups are
copied.

* The server stack copies the server groups: accum, colour-buffer, current,
  depth-buffer, enable, eval, fog, hint, lighting, line, list, multisample,
  pixel-mode, point, polygon, polygon-stipple, scissor, stencil-buffer,
  texture, transform, viewport.
* The client stack copies the client groups: vertex-array and pixel-store.
* **Matrix stacks are excluded** — "Matrix stacks are never pushed or popped
  with PushAttrib, PushClientAttrib, PopAttrib, or PopClientAttrib"
  (glspec21 §6.1.15).
* The texture-group ordering rule is honoured: push iterates the texture units
  ascending, pop iterates descending (glspec21 §6.1.15).
* Depth ≥ 16, with `STACK_OVERFLOW` / `STACK_UNDERFLOW` errors
  (glspec21 table 6.34).

**Cost:** one struct copy per push. The legacy state block is a few kilobytes;
16 levels of it is tens of kilobytes of context memory — negligible.

**Note on the modern path:** the modern core profile has no attribute stack
at all (`glPushAttrib` was removed from core in GL 3.2 and never restored), so
this is purely additive and cannot regress the existing behaviour.

---

## 15. Pixel operations, raster position and window position → Metal

### 15.1 Raster / window position — CPU EMULATION

The raster position is computed on the CPU by the legacy path:

* `glRasterPos*` transforms the coordinate exactly as a vertex (model-view,
  projection, clip, viewport), keeps the associated colour/texcoord/depth, and
  sets a *valid* flag. If the point is clipped, the position becomes
  indeterminate and the valid flag is cleared (glspec21 §2.13).
* `glWindowPos*` sets the position directly in window coordinates; the
  associated data is the current colour/texcoord.

The raster position is consumed by `glDrawPixels`, `glBitmap` and
`glCopyPixels`.

### 15.2 `glDrawPixels` / `glBitmap` — HYBRID (CPU conversion, GPU blit)

The pixel data comes from client memory in one of the legacy formats
(`GL_COLOR_INDEX`, `GL_RGB`, `GL_RGBA`, `GL_BGR`, `GL_BGRA`, `GL_LUMINANCE`, …)
with a legacy type (`GL_UNSIGNED_BYTE`, `GL_BYTE`, `GL_SHORT`, `GL_UNSIGNED_SHORT`,
`GL_INT`, `GL_UNSIGNED_INT`, `GL_FLOAT`, …).

The pipeline is:

1. **CPU:** convert the client pixels into an RGBA8 (or RGBA16F) texture,
   applying the pixel transfer operations (scale/bias) and the pixel maps.
   This is the same kind of conversion the existing `glReadPixels` path does
   (`src/gl/gl_readpixels_dsa.cpp:44-72`, `StoreReadPixels`).
2. **GPU:** blit that texture into the framebuffer at the raster position.
   TGLMT already has the machinery: `IDevice::blitCopy`
   (`include/tglmt/MetalInterface.h:813`) and `blitToTarget`
   (`:833`).

`glBitmap` is the same, with a 1-bit-per-pixel mask expanded on the CPU.

**`glPixelZoom`** scales the written rectangle; applied as a scaled blit or as
a CPU-side nearest-neighbour expansion.

**`glCopyPixels`** copies from the read buffer to the write buffer at the
raster position, with format conversion and the pixel transfer operations
applied. The existing `glBlitFramebuffer` / `IDevice::blitCopy` path is the
building block; the legacy semantics (source = read buffer, destination =
write buffer, pixel transfer applied) need their own wrapper.

**`glPixelTransfer*` and `glPixelMap*`** are CPU-side transforms applied on the
pack and unpack paths. They compose with the existing conversion code. Note
that `GL_SWAP_BYTES` and `GL_LSB_FIRST` are currently *rejected* by
`glPixelStorei` (`src/gl/gl_texture.cpp:1711-1717`) even though
`glGetIntegerv` reports them as 0 (`src/gl/gl_get.cpp:373-374`) — an
inconsistency the legacy work fixes by accepting and honouring them.

### 15.3 The pixel-map family

`glPixelMapfv/uiv/usv` installs up to `MAX_PIXEL_MAP_TABLE` (≥ 32) entries per
target. The targets are the 4×4 colour matrix, the depth map and the index
map. The colour matrix is applied as `dst = Σ Mᵢⱼ · srcⱼ` (glspec21 §3.6.3),
i.e. a 4×4 matrix multiply per pixel — a CPU transform.

---

## 16. Group-by-group summary table

| Group | Strategy | Metal mechanism | Semantic gap / cost |
|---|---|---|---|
| L01 Immediate mode | **HYBRID** — CPU assembler + generated shader | CPU vertex assembly into the ring buffer; one batched `drawPrimitives` per primitive-type run | CPU cost of assembly; `glBegin` legality needs a central guard |
| L02 Client-side vertex arrays | **CPU EMULATION** | gather into the shared staging buffer at draw time | CPU copy per draw; no Metal equivalent exists |
| L03 Matrix stack | **GENERATED_SHADER** + CPU state | matrices as uniforms in the generated VS | none; matrices are re-evaluated per draw as the spec requires |
| L04 Lighting / materials | **GENERATED_SHADER** (per vertex) | generated lighting code in the legacy VS | **two-sided lighting** evaluated per fragment instead of per vertex — documented divergence; flat shading needs a second attribute set |
| L05 Fog | **GENERATED_SHADER** | fog in the generated FS, after texturing | `GL_LINEAR` with `start == end` is undefined in GL — TGLMT must pick a rule |
| L06 Texenv / texgen | **GENERATED_SHADER** (+ CPU for object/eye-linear) | generated per-unit texenv chain in the FS; texgen in the VS/CPU | `TEXTURE_LOD_BIAS` needs a manual LOD computation (Metal's sampler has no bias) |
| L07 Display lists | **CPU EMULATION** — command stream | record/replay at the GL command level | replay is not a single draw; the spec's "execute immediately" list must be honoured |
| L08 Evaluators | **CPU EMULATION** | Bernstein evaluation on the CPU, feeding the legacy assembler | none; the spec explicitly excludes the rest of the pipeline |
| L09 Selection / feedback | **CPU EMULATION** | computed from the assembled geometry + the CPU clipper | none, but it needs a CPU clipper and a z-range per primitive |
| L10 Accumulation buffer | **GPU EMULATION** | dedicated RGBA-float texture + full-screen passes | one extra pass per `glAccum`; signed `[−1,1]` semantics need a float target |
| L11 Pixel transfer / pixel drawing | **HYBRID** — CPU conversion + GPU blit | existing blit machinery + CPU format conversion | `SWAP_BYTES` / `LSB_FIRST` must be added to `glPixelStorei` |
| L12 Raster / window position | **CPU EMULATION** | computed from the legacy matrices and the viewport | none |
| L13 Stipple | polygon: **GENERATED_SHADER** (exact); line: **HYBRID** (CPU expansion + shader pattern) | `gl_FragCoord` mask test / per-vertex line distance | line stipple loses hardware line rasterisation; polygon stipple needs a coordinate-flip check |
| L14 Alpha test | **GENERATED_SHADER** (exact) | `discard_fragment()` | none |
| L15 Texture residency / priority | **CPU EMULATION** | answered from the texture objects | priority feeds the legacy LOD-bias path |
| L16 User clip planes | **HARDWARE LIMITED** natively; **HYBRID** in practice | CPU clipping of the assembled polygon (Metal gives 1 `[[clip_distance]]`) | CPU clip cost; breaks batching across a clip-plane change |
| L17 Attribute stacks | **CPU EMULATION** | struct copy of the legacy state | none |
| L18 Colour-index mode | **NOT APPLICABLE** | no colour-index visuals on iOS/CAMetalLayer | the commands must exist and keep state but have no effect (the core profile removed colour-index mode for the same reason) |

---

## 17. The cross-cutting design decisions, summarised

1. **One state block, one program, one draw path.**
   `LegacyState` (new) owns all fixed-function state; one generated MSL
   program implements the fixed-function stages; the existing deferred draw
   path (`src/gl/gl_apple_draw.cpp:1774`) executes both the legacy and the
   modern program. The legacy program is stored *outside* `Context::programs`
   so it is invisible to `glGetIntegerv(GL_CURRENT_PROGRAM)` and to
   `glUseProgram`.

2. **The pipeline cache needs no structural change.**
   `Context::PipelineKey` already keys on `ILibrary::identity()`
   (`include/tglmt/Context.h:1019-1020`) and on `programGen`
   (`:1023`). A legacy program is just another library pair, so it gets its
   own key entries for free. The vertex layout is carried by the existing
   `attribs[]` / `stride` fields. **The one thing that must be right:** the
   legacy program must be *regenerated* (and `linkGen` bumped) whenever its
   shape changes, or the cache would hand back a stale pipeline.

3. **Uniforms carry the values; the program carries the shape.**
   Everything that changes per draw (matrices, lights, materials, fog, texenv
   arguments) is a uniform. Only the *shape* (which features exist) is
   compiled in, and the shape is part of the library identity.

4. **The spec's own rule is the dispatch rule.**
   glspec21 §2.15: no program in use ⇒ fixed function. The legacy path is
   selected by "no program bound", not by a mode flag.

5. **Batch by primitive type, break on state change.**
   One `drawPrimitives` per run of same-type primitives in the same state.
   The ring allocator (`Context::RingAlloc`) already provides the staging
   memory, so no new allocation path is introduced.

6. **Everything the GPU cannot do exactly, the CPU does.**
   Evaluators, selection, feedback, raster position, client arrays, display
   lists and the attribute stacks are all CPU-side, and the CPU already holds
   the geometry and the state. The one genuinely hardware-limited item that
   matters — six user clip planes against Metal's one `[[clip_distance]]` — is
   resolved by CPU clipping, which is also the exact behaviour.

7. **The two remaining hardware limits are documented, not hidden.**
   Wide lines (Metal rasterises 1 px) and the logic op (Metal has no logic-op
   stage, and programmable blending needs Metal 3) are emulated with a
   measurable cost. Both must be reported honestly in
   `GL_ALIASED_LINE_WIDTH_RANGE` and in the compatibility caveats.

8. **The legacy path must not be able to lie.**
   `glEnable(GL_LIGHTING)` currently returns "enabled" and does nothing
   (`src/core/StateTracker.cpp:6-20`). Before the compatibility work lands,
   the compatibility context must either implement the feature or make the
   enable call fail — a silently-inert enable is the worst possible outcome.

---

## 18. Cost model

| Operation | Cost | Dominant term |
|---|---|---|
| Matrix change | one uniform upload (256 B) | negligible |
| Light change | one uniform upload (~400 B) | negligible |
| Immediate-mode primitive | CPU append + batched draw | CPU: ~20 ns/vertex |
| `glBegin`/`glEnd` pair | CPU assembly | CPU: ~20 ns/vertex, 1 draw per batch |
| Client-array draw | CPU gather + upload | CPU: memcpy-bound, ~1 GB/s |
| Texture fetch + texenv | 1 fetch + ~10 ALU per unit per fragment | GPU |
| Lighting | ~40 ALU per vertex per light | GPU |
| Fog | 1 `exp` + 1 lerp per fragment | GPU |
| Polygon stipple | 1 bit test per fragment | GPU |
| Line stipple | 4 vertices/segment instead of 2, plus a bit test per fragment | GPU + CPU |
| Wide line | 4–6 vertices/segment, CPU expansion | GPU + CPU |
| Logic op | one extra full-screen pass + a destination copy | GPU: ~1 full-screen pass per draw |
| `glAccum` | one full-screen pass | GPU |
| Evaluator mesh | CPU: O(order²) per grid point | CPU |
| Selection hit | CPU: O(vertices) per primitive | CPU |
| Display-list replay | GL-command speed | CPU |

The performance-relevant comparison is against the **modern** path: a legacy
application rewritten to use VBOs and a shader would be faster. The legacy
path's goal is *correctness*, with a performance profile that is not worse
than the equivalent GL 1.x implementation on the same hardware. The two
places where that is hard are the logic op and the accumulation buffer, both
of which are inherently multi-pass in any implementation.

---

## 19. What this document does **not** claim

* No strategy here has been implemented or measured. Every "cost" entry is an
  estimate from the algorithm, not a measurement.
* The A11 capability figures come from Apple's vendored feature tables and
  the iOS SDK headers on the development machine; they have **not** been
  re-verified on the physical device (`project/TODO.md` T-017 is blocked on
  device logs).
* The MSL snippets are sketches of the required computation, not the final
  generated source. The generator's exact output will differ in layout.
* The `[[flat]]`/provoking-vertex divergence (§1.5) and the two-sided-lighting
  divergence (§5.2) are **known, accepted** divergences that must be listed in
  the compatibility caveats. They are not claims of conformance.
* Geometry shaders are explicitly out of scope here (they are a GL 3.2+
  feature, i.e. the modern tier). The project rule D-018 nevertheless
  requires that a geometry shader have a **working emulation** rather than be
  declared `HARDWARE_LIMITED`; TGLMT's existing behaviour is a compile-time
  refusal (`src/gl/gl_apple_draw.cpp:1901-1903`:
  `if (pr.hasGeometryStage) { c.LogDebug(0, 0, 0, 0, "AppleDrawGL: GS trace-only (M5b-future)"); return false; }`).
  That is a *modern-tier* obligation and is **not** discharged by this
  document; it is listed in `project/TODO.md` as a separate item. It is
  mentioned here only so that the reader does not mistake the legacy
  fixed-function work for a solution to it: the legacy pipeline has no
  geometry-shader stage, and the two are independent.
* GLU (D-015 / T-019) is out of scope.

---

## 20. Cross-references

| Topic | Where |
|---|---|
| Spec-level audit, per-group status | `docs/LEGACY_PIPELINE_AUDIT.md` |
| Staged implementation order, test IDs, exit criteria | `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` |
| Profile split and the `L01`–`L20` grouping | `docs/OPENGL_PROFILE_MATRIX.md` |
| Measured export status | `docs/OPENGL_API_COVERAGE.md` |
| 4.6-Core → Metal feasibility (the modern surface) | `docs/OPENGL_46_METAL_MAPPING.md` |
| A11 hardware limits and their evidence | `docs/METAL_A11_LIMITATIONS.md` |
| Apple's capability tables | `docs/apple/Metal-Feature-Set-Tables.pdf` |
| The MSL specification | `docs/apple/Metal-Shading-Language-Specification.pdf` |
| Mapping vocabulary, `HARDWARE_LIMITED` bar | `project/docs/GLOSSARY.md`, `project/AGENTS.md` §6 |
| Project decisions (D-014 one-tier, D-018 GS emulation) | `project/docs/DECISIONS.md` |
