# Legacy Pipeline Audit — OpenGL 1.0–2.1 fixed-function & compatibility behaviour

> Scope: the OpenGL 1.0 → 2.1 **legacy fixed-function pipeline** plus the
> compatibility-profile-only behaviour that a Compatibility-profile context
> (GL 3.2 → 4.6) must still provide.
>
> Companion documents:
> [`docs/LEGACY_TO_METAL_MAPPING.md`](LEGACY_TO_METAL_MAPPING.md) — how each
> mechanism maps onto Metal on iOS/A11.
> [`docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md`](COMPATIBILITY_IMPLEMENTATION_PLAN.md) —
> the staged build order.
>
> **Nothing in this document describes implemented behaviour.** Every section is
> split into three labelled blocks:
>
> * **SPEC** — what the OpenGL specification requires (quoted/paraphrased from
>   the vendored sources listed in §0.2, with a section reference).
> * **CURRENT** — what TGLMT actually does today, with `file:line` evidence, or
>   an explicit "absent" statement naming the exact `grep` that proved it.
> * **PLAN** — what will be built (this is *not* implemented; see the plan doc).
>
> The **STATUS** line under each group is one of `MISSING` / `PARTIAL` /
> `PRESENT` and always cites the evidence it is based on.

---

## 0. Method, sources and vocabulary

### 0.1 How the entry-point census was produced

The census is **not** hand-written. The command set comes from the vendored
Khronos registry and the machine-generated inventory:

| Input | Path | Role |
|---|---|---|
| Khronos OpenGL registry | `docs/khronos/gl.xml` (sha256 `b9ca2cfa…94ad3`) | authoritative list of every command, enum and profile attribute |
| Generated inventory | `data/version_inventory.json` | per-version × profile command sets (schema 1), produced by `tools/gen_version_inventory.py` |
| Profile grouping | `docs/OPENGL_PROFILE_MATRIX.md` §3.3/§3.4 | the `L01`–`L20` functional groups and the script that assigns commands to them |
| Built library | `build-work/libtglmt.dylib` | the measured export surface |

The census command actually run for this audit:

```sh
cd /Volumes/project/TGLMT
nm -gU build-work/libtglmt.dylib | awk '$2=="T"{print $3}' | sed 's/^_//' | sort > /tmp/exported.txt
python3 - <<'PY'
import json
d = json.load(open('data/version_inventory.json'))
v = d['versions']['4.6']['commands']['compatibility']
exp = set(open('/tmp/exported.txt').read().split())
print("compat46 total", len(v), "missing", len([n for n in v if n not in exp]))
PY
# → compat46 total 1048 missing 348
```

and the per-group breakdown, using the script embedded in
`docs/OPENGL_PROFILE_MATRIX.md` §3.4 (extracted verbatim to `/tmp/pg.py`):

```sh
python3 /tmp/pg.py --dylib build-work/libtglmt.dylib
```

Result (18 of the 20 legacy groups are entirely missing):

| Group | Label | GL 4.6 compat | implemented | missing |
|---|---|---:|---:|---:|
| L01 | Immediate mode (Begin/End and vertex specification) | 173 | 0 | 173 |
| L02 | Client-side vertex arrays | 12 | 0 | 12 |
| L03 | Fixed-function transform and matrix stack | 20 | 0 | 20 |
| L04 | Lighting, materials and shading model | 18 | 0 | 18 |
| L05 | Fog | 4 | 0 | 4 |
| L06 | Texture environment and texture coordinate generation | 15 | 0 | 15 |
| L07 | Display lists | 8 | 0 | 8 |
| L08 | Evaluators | 23 | 0 | 23 |
| L09 | Selection and feedback modes | 8 | 0 | 8 |
| L10 | Accumulation buffer | 2 | 0 | 2 |
| L11 | Pixel transfer modes and pixel drawing | 12 | 0 | 12 |
| L12 | Raster position and window position | 40 | 0 | 40 |
| L13 | Stipple (polygon and line stipple) | 3 | 1 | 2 |
| L14 | Alpha test | 1 | 0 | 1 |
| L15 | Texture residency and priority | 2 | 0 | 2 |
| L16 | User clipping planes | 2 | 0 | 2 |
| L17 | Attribute stacks | 4 | 0 | 4 |
| L18 | Colour-index mode state | 2 | 0 | 2 |
| L19 | Compatibility-only vertex-type entry points (GL 3.3) | 30 | 30 | 0 |
| L20 | Compatibility-only n-suffixed pixel/history queries (GL 4.5) | 12 | 12 | 0 |

`L19`/`L20` are **not** fixed-function: they are the GL 3.3 packed-vertex-type
family and the GL 4.5 `Getn*` robustness queries, both already implemented
(`src/gl/gl_packed_robust.cpp`). The legacy fixed-function surface is therefore
**349 commands, of which 348 are absent** (`glLineStipple` is exported but is an
empty stub — see §7).

### 0.2 Specification sources

| Source | Path | Used for |
|---|---|---|
| OpenGL 2.1 Specification (Dec 1 2006) | fetched to `/tmp/glspec21.pdf` for this audit; **not vendored in the repo** | all GL 1.0–2.1 semantics (immediate mode, matrix stack, lighting, fog, texenv, texgen, evaluators, display lists, selection/feedback, accumulation, attribute stack, pixel ops) |
| OpenGL 4.6 Core Profile Specification (May 5 2022) | `docs/khronos/glspec46.core.pdf` | Appendix E.2 "Removed Features" — the authoritative list of what the Compatibility profile retains |
| Khronos OpenGL registry | `docs/khronos/gl.xml` | enum values, version of introduction, profile attributes |
| Metal Feature Set Tables (May 21 2026) | `docs/apple/Metal-Feature-Set-Tables.pdf` | A11 = `MTLGPUFamilyApple4` limits |
| Metal Shading Language Specification (2026-06-04) | `docs/apple/Metal-Shading-Language-Specification.pdf` | MSL attribute/interpolation rules |

> **Citation convention.** `glspec21 §X.Y` = the GL 2.1 specification section;
> `glspec46 App. E.2` = the 4.6 Core Profile PDF, Appendix E.2;
> `gl.xml:NNNN` = a line in the vendored registry;
> `file:line` = a location in the TGLMT source tree.

### 0.3 The single most important structural fact

The GL 4.6 Core Profile specification states the relationship between the two
pipelines explicitly:

> "Older generations of graphics hardware were not programmable using shaders,
> although they were configurable by setting state controlling specific details
> of their operation. The compatibility profile of OpenGL continues to support
> the legacy OpenGL commands developed for such fixed-function hardware,
> although they are typically implemented by writing shaders which reproduce
> the operation of such hardware."
> — glspec46 §1.2.4

and, for the draw path:

> "When the linked program object is used as the current program object, the
> executable code for the vertex shaders it contains is used to process
> vertices. … **If the program object has no vertex shader, or no program object
> is currently in use, the fixed-function method for processing vertices is used
> instead.**"
> — glspec21 §2.15

This is the load-bearing rule for the whole compatibility profile: **a
compatibility-profile context contains both pipelines, and the selection between
them is per-draw, decided by whether a program object is in use.** Every design
decision in the mapping document follows from it.

### 0.4 Vocabulary (repo glossary)

`project/docs/GLOSSARY.md` already defines the mapping vocabulary used here:
`DIRECT` / `GENERATED_SHADER` / `GPU_EMU` / `CPU_EMU` / `HYBRID` / `LIMITED`
(= `HARDWARE_LIMITED`) / `BLOCKED`, and the rule that `HARDWARE_LIMITED` needs
**(a)** a capability query or Metal documentation and **(b)** a list of
emulation options considered with the reason for rejection. That rule is honoured
in the mapping document.

### 0.5 A note on scope

GLU (the tessellator/quadric library) is in the project scope (D-015) but is a
*library*, not part of the OpenGL fixed-function pipeline; it is scheduled
separately (`project/TODO.md` T-019) and is **not** audited here.

---

## 1. Group L01 — Immediate mode (Begin/End, vertex specification)

### SPEC

**Semantics.** `glBegin(mode)` / `glEnd()` bracket a run of vertex-specification
commands. `glBegin` selects one of ten primitive modes; the vertices between
`Begin` and `End` are assembled into primitives:

| mode | assembly rule (glspec21 §2.6.1) |
|---|---|
| `GL_POINTS` | each vertex is one point |
| `GL_LINES` | vertex pairs; an odd trailing vertex is ignored |
| `GL_LINE_STRIP` | consecutive segments; <2 vertices ⇒ no primitive |
| `GL_LINE_LOOP` | as strip plus a closing segment from last to first |
| `GL_TRIANGLES` | vertex triples; the trailing `k` (0…2) vertices are ignored |
| `GL_TRIANGLE_STRIP` | overlapping triangles (A, B, current); <3 vertices ⇒ no primitive |
| `GL_TRIANGLE_FAN` | triangles (v0, vi, vi+1) |
| `GL_QUADS` | groups of 4 → one quad each; trailing `k` (0…3) ignored |
| `GL_QUAD_STRIP` | quad *i* has vertices (v2i, v2i+1, v2i+3, v2i+2) — note the **reversed** last two; <4 vertices ⇒ no primitive; odd count ⇒ last ignored |
| `GL_POLYGON` | one boundary polygon; **fewer than 3 vertices ⇒ no primitive**; only convex polygons are guaranteed correct, otherwise the result need only lie within the convex hull of the projected vertices |

**Current attribute state.** `glColor*`, `glNormal3*`, `glTexCoord[1-4]*`,
`glMultiTexCoord[1-4]*`, `glSecondaryColor3*`, `glIndex*`, `glFogCoord*`,
`glEdgeFlag*` set *current values* that are copied onto the vertex when the
vertex is specified (glspec21 §2.7). `glVertex{234}{sifd}[v]` supplies x, y, z, w
(`Vertex2` ⇒ z=0, w=1; `Vertex3` ⇒ w=1).

**Edge flags.** `glEdgeFlag`/`glEdgeFlagv` set one current bit (initially
`TRUE`). When the mode is `POLYGON`, `TRIANGLES` or `QUADS`, each vertex begins an
edge; a `TRUE` flag marks it a *boundary* edge, `FALSE` marks it *non-boundary*
(glspec21 §2.6.2). Edge classification is used by polygon rasterization
(§3.5.4).

**Illegal commands inside Begin/End.** glspec21 §2.6.3 is exhaustive:

> "The only GL commands that are allowed within any Begin/End pairs are the
> commands for specifying vertex coordinates, vertex colors, normal coordinates,
> texture coordinates, generic vertex attributes, and fog coordinates (Vertex,
> Color, SecondaryColor, Index, Normal, TexCoord and MultiTexCoord, VertexAttrib,
> FogCoord), the ArrayElement command …, the EvalCoord and EvalPoint commands
> …, commands for specifying lighting material parameters (Material commands),
> display list invocation commands (CallList and CallLists), and the EdgeFlag
> command. **Executing any other GL command between the execution of Begin and
> the corresponding execution of End results in the error INVALID_OPERATION.**
> Executing Begin after Begin has already been executed but before an End is
> executed generates the INVALID_OPERATION error, as does executing End without a
> previous corresponding Begin."

A second, softer rule covers the client-state array commands
(`EnableClientState`, `DisableClientState`, `PushClientAttrib`,
`PopClientAttrib`, `ColorPointer`, `FogCoordPointer`, `EdgeFlagPointer`,
`IndexPointer`, `NormalPointer`, `TexCoordPointer`, `SecondaryColorPointer`,
`VertexPointer`, `VertexAttribPointer`, `ClientActiveTexture`,
`InterleavedArrays`, `PixelStore`): they are "not allowed within any Begin/End
pair, but an error may or may not be generated if such execution occurs. If an
error is not generated, GL operation is undefined."

**Rectangles.** `glRect*` is specified in §2.10 as a *separate* command that does
**not** use Begin/End: it is "equivalent to a Begin/End pair with a rectangle
primitive", and the current texture coordinates and colour of the rectangle's
vertices are those in effect when the command is issued.

**Raster position.** `glRasterPos*` (§2.13) sets the current raster position; the
coordinates are "treated as if they were specified in a Vertex command", i.e.
transformed by the current model-view and projection matrices, clipped, and
projected to window coordinates, together with the associated colour/texcoord
data.

**Window position.** `glWindowPos*` (GL 1.4, §2.13 / App. G §G.15) sets the
current raster position directly in window coordinates, bypassing the transform.

### CURRENT

**STATUS: MISSING.** No immediate-mode entry point exists.

*Absence evidence (exact commands):*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglBegin\b|\bglEnd\b|\bglVertex[234][sifd]" src/ include/ \
  | grep -vE "glBeginQuery|glBeginTransformFeedback|glBeginConditionalRender|glBeginPerf"
```

The only three hits are English/Vietnamese prose inside comments that mention
`glBegin/glEnd` in the sense of "the query's Begin/End pair"
(`src/gl/gl_error_string_debug.cpp:63`, `include/tglmt/Context.h:541`,
`include/tglmt/MetalInterface.h:550`). No declaration, no implementation.

* Export census: all 173 L01 commands are absent from
  `build-work/libtglmt.dylib` (see §0.1).

* The *modern* draw path cannot be reused as-is: `AppleDrawGL`
  (`src/gl/gl_apple_draw.cpp:1774`) begins by resolving a **linked program
  object** and bails out with `noProgram++` when there is none
  (`src/gl/gl_apple_draw.cpp:1799-1802`). There is no "no program ⇒ fixed
  function" fallback, which is exactly the rule glspec21 §2.15 requires.

* Primitive types `GL_QUADS` (0x0007), `GL_QUAD_STRIP` (0x0008) and
  `GL_POLYGON` (0x0009) do not exist in the modern path either:
  `IsValidPrimitiveMode` (`src/gl/gl_draw.cpp:9-27`) rejects them with
  `GL_INVALID_ENUM`, and the primitive-type switch in
  `src/gl/gl_apple_draw.cpp:2006-2014` has no case for them. `MetalInterface.h:229-233`
  lists the whole `metal::PrimitiveType` enum: `Point`, `Line`, `LineStrip`,
  `Triangle`, `TriangleStrip`, `Fan`, `Patches` — there is **no** quad or polygon
  primitive in Metal, and `AppleMetalBridge.mm:240-251` (`ToMTLPrim`) maps Metal's
  five real primitive types only.

* The two existing CPU-side primitive expansions in the modern path are the
  pattern the legacy path will reuse: `GL_TRIANGLE_FAN` is expanded to an indexed
  triangle list at `src/gl/gl_apple_draw.cpp:1937-1974`, and `GL_LINE_LOOP` is
  expanded to a closed line strip at `src/gl/gl_apple_draw.cpp:1975-2000`.

### PLAN

* Implement `glBegin`/`glEnd` as a **client-side assembler** in a new
  `src/gl/gl_legacy_immediate.cpp`: `glBegin` opens a primitive block, the
  `glVertex*` family appends a fully-resolved vertex (position + current colour +
  normal + texcoords + edge flag) to a CPU staging vector, `glEnd` closes the
  block and hands the assembled vertex range to the draw path.
* Enforce the §2.6.3 legality rule with a per-context "in Begin/End" flag:
  every GL entry point that is not on the allowed list must raise
  `GL_INVALID_OPERATION` while the flag is set. This needs a **central guard**,
  not a per-file check, because there are ~700 entry points; the natural place is
  the `ErrorTracker`/dispatch layer.
* `GL_POLYGON` is triangulated as a fan from vertex 0 (legal for the guaranteed
  convex case, and it satisfies "need only lie within the convex hull").
  `GL_QUADS` → two triangles each; `GL_QUAD_STRIP` → two triangles each using
  the spec's (v2i, v2i+1, v2i+3, v2i+2) ordering.
* `glRect*` is implemented as a direct two-triangle submission, not through the
  Begin/End assembler.

---

## 2. Group L03 — Fixed-function transform and matrix stack

### SPEC

**Matrix modes.** `glMatrixMode` selects one of `GL_MODELVIEW`, `GL_PROJECTION`,
`GL_TEXTURE`, `GL_COLOR` (glspec21 §2.11.2). `GL_TEXTURE` applies to the
**active texture unit's** texture matrix stack.

**Stack depths.** "For MODELVIEW mode, the stack depth is at least 32 … For the
other modes, the depth is at least 2. Texture matrix stacks for all texture units
have the same depth." The current matrix is the top of the stack; initially every
stack holds one identity matrix and the mode is `MODELVIEW`.

**Push/pop errors.** "Popping a matrix off a stack with only one entry generates
the error **STACK_UNDERFLOW**; pushing a matrix onto a full stack generates
**STACK_OVERFLOW**." (glspec21 §2.11.2; error codes 0x0504 / 0x0503, confirmed in
`docs/khronos/gl.xml:628` and `:630`.)

**Composition order.** `glMultMatrix` computes `C' = C · M` — the argument is
**post-multiplied** (column-major storage, glspec21 §2.11.2). `glTranslate`,
`glRotate`, `glScale`, `glFrustum`, `glOrtho` each build a matrix and invoke
`MultMatrix` with it, so they also post-multiply.

**Transpose variants.** `glLoadTransposeMatrix`/`glMultTransposeMatrix` take
row-major data and behave exactly like the non-transposed call with the
transpose of the argument.

**Viewport / depth range.** `glViewport` sets the viewport rectangle; `n`/`f` of
`glDepthRange` are clamped to `[0,1]` and map window z as
`zw = [(f−n)/2]·zd + (n+f)/2` (glspec21 §2.11.1). Both `INVALID_VALUE` when
width or height is negative.

**Normal transformation.** Normals are transformed by the model-view matrix
`(nx′, ny′, nz′, q′) = (nx, ny, nz, q)·M⁻¹` and then rescaled/normalised as
controlled by `GL_RESCALE_NORMAL` / `GL_NORMALIZE` (glspec21 §2.11.3). The
inverse-transpose is therefore the operation, not the plain model-view matrix.

**`GL_TEXTURE` matrix and texgen interaction.** The texture matrix stack
multiplies the *generated* texture coordinates (§3 below).

### CURRENT

**STATUS: MISSING** for the GL 1.x matrix stack API, with one **PARTIAL**
shadow that must not be confused with it.

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglMatrixMode\b|\bglPushMatrix\b|\bglPopMatrix\b|\bglLoadIdentity\b|\bglLoadMatrixf\b|\bglMultMatrixf\b|\bglOrtho\b|\bglFrustum\b" src/ include/
# → no output (exit status 1)
```

All 20 L03 commands are absent from the exported surface.

* **PARTIAL shadow — EXT matrix API.** `Context::ExtState::matrixStacks`
  (`include/tglmt/Context.h:581`) stores a `map<GLenum, vector<array<GLdouble,16>>>`
  and is driven by the *extension* entry points
  `glMatrixPushEXT` / `glMatrixPopEXT` / `glMatrixLoadfEXT` / `glMatrixMultfEXT`
  … in `src/gl/gl_ext_dsa_state.cpp:249-281`, with helpers
  `Matrix()`/`Mul()`/`Load()` at `src/gl/gl_ext_dsa_state.cpp:56-78`. This is
  **not** the GL 1.x matrix stack: it has no *current matrix mode*, no per-texture-unit
  texture stacks, no `GL_COLOR` stack, a fixed depth of 32
  (`src/gl/gl_ext_dsa_state.cpp:276`) for all modes, and it is not reachable
  through `glMatrixMode`/`glPushMatrix`. It does already produce the right
  `GL_STACK_OVERFLOW` (0x0503) / `GL_STACK_UNDERFLOW` (0x0504) errors
  (`src/gl/gl_ext_dsa_state.cpp:276` and `:280`), which is the error behaviour
  the legacy stack also needs.

* **No matrix queries.** `GL_MODELVIEW_MATRIX` (0x0BA6), `GL_PROJECTION_MATRIX`
  (0x0BA7), `GL_TEXTURE_MATRIX` (0x0BA8) are **not** in the accepted-pname list
  `include/tglmt/gl46_pnames.h` (verified: `grep -c "0x0BA6u"
  include/tglmt/gl46_pnames.h` → 0), so `glGetIntegerv(GL_MODELVIEW_MATRIX)`
  today raises `GL_INVALID_ENUM` and returns 0
  (`src/gl/gl_get.cpp:474`, `src/gl/gl_get.cpp:131`).

* **Viewport and depth range do exist** and are wired to Metal, but as *modern*
  state: `glViewport` → `StateTracker::SetViewport`
  (`src/gl/gl_raster.cpp:185`), `glDepthRange` → `SetDepthRange`
  (`src/gl/gl_raster.cpp:51`), converted to a Metal viewport by
  `GLViewportToMetal` (`include/tglmt/GLConvert.h:21-39`) and applied at
  `src/gl/gl_apple_draw.cpp:2626-2635`. The conversion is a *lower-left-origin*
  flip (negative viewport height), see §9.

### PLAN

* Add a `LegacyMatrixState` block owning four stacks per mode
  (`MODELVIEW`, `PROJECTION`, `COLOR`, `TEXTURE`-per-unit), a current-mode
  enum, and the stack-depth limits (modelview ≥ 32, others ≥ 2).
* The transform result is fed to the GPU as a **uniform**, not baked into the
  pipeline: the fixed-function vertex stage multiplies by the current
  model-view and projection matrices. This makes the matrix stack a pure
  CPU-side state object, and it keeps the matrix changes free of pipeline
  recompiles.
* The normal matrix (inverse-transpose of the model-view 3×3) is computed on the
  CPU when the model-view changes and uploaded alongside it.
* `GL_TEXTURE` mode targets the **active** texture unit's stack, so the stack
  must be per texture unit; `glActiveTexture` already exists
  (`StateTracker::SetActiveTexture`, `src/core/StateTracker.cpp:122`) and
  `StateTracker::activeTex_` (`include/tglmt/StateTracker.h:290`) gives the unit
  index.

---

## 3. Group L06 — Texture environment and texture-coordinate generation

### SPEC

**Texture environment.** `glTexEnv{target, pname, param}` sets the texture
environment of the **active texture unit** (glspec21 §3.8.13). `target` is
`GL_TEXTURE_ENV`, `GL_TEXTURE_FILTER_CONTROL` (only `TEXTURE_LOD_BIAS`) or
`GL_POINT_SPRITE` (point-sprite coordinate replacement).

`GL_TEXTURE_ENV_MODE` selects the texture function:

| mode | RGB | alpha |
|---|---|---|
| `GL_REPLACE` | `Cs` | `As` |
| `GL_MODULATE` | `Cp·Cs` | `Ap·As` |
| `GL_DECAL` | `Cp(1−As) + Cs·As` | `Ap` |
| `GL_BLEND` | `Cp(1−Cs) + Cc·Cs` | `Ap` |
| `GL_ADD` | `Cp + Cs` | `Ap + As` |
| `GL_COMBINE` | per the 3-argument combiner | per the combiner |

where `Cp`/`Ap` are the *previous* result (for unit 0 the incoming fragment
colour), `Cs`/`As` the texture source colour, `Cc`/`Ac` the texture environment
colour, and the mapping from the texture's base internal format to `(Cs, As)` is
table 3.20.

`GL_COMBINE` (GL 1.3) takes `COMBINE_RGB`/`COMBINE_ALPHA` (each
`REPLACE`/`MODULATE`/`ADD`/`ADD_SIGNED`/`INTERPOLATE`/`SUBTRACT`/`DOT3_RGB`/
`DOT3_RGBA`), `SRC0/1/2_RGB`, `SRC0/1/2_ALPHA`, `OPERAND0/1/2_RGB`,
`OPERAND0/1/2_ALPHA` and the `RGB_SCALE`/`ALPHA_SCALE` multipliers. The combiner
results are clamped to `[0,1]`.

Initial state: texture function and combiner functions `MODULATE`; combiner
sources `TEXTURE`, `PREVIOUS`, `CONSTANT` for 0/1/2; combiner RGB operands for
sources 0 and 1 `SRC_COLOR`, source 2 and all alpha operands `SRC_ALPHA`;
environment colour `(0,0,0,0)`.

**Texture coordinate generation.** `glTexGen{coord, pname, param}` (glspec21
§2.11.4) with `coord ∈ {S,T,R,Q}` and `pname ∈ {TEXTURE_GEN_MODE, OBJECT_PLANE,
EYE_PLANE}`:

| `TEXTURE_GEN_MODE` | computation |
|---|---|
| `GL_OBJECT_LINEAR` | `g = p1·xo + p2·yo + p3·zo + p4·wo` (object coordinates) |
| `GL_EYE_LINEAR` | `g = p1′·xe + p2′·ye + p3′·ze + p4′·we` with `(p1′…p4′) = (p1…p4)·M⁻¹` |
| `GL_SPHERE_MAP` | reflection vector `r = u − 2·nf·(nf·u)`, `m = 2rx² + ry² + (rz+1)²`, `s = rx/m + ½`, `t = ry/m + ½`; `R`/`Q` coords ⇒ `INVALID_ENUM` |
| `GL_REFLECTION_MAP` | same `r`, but `s = rx`, `t = ry`, `r = rz`; `Q` coord ⇒ `INVALID_ENUM` |
| `GL_NORMAL_MAP` | `s = nfx`, `t = nfy`, `r = nfz`; `Q` coord ⇒ `INVALID_ENUM` |

Enabled per coordinate with `GL_TEXTURE_GEN_S/T/R/Q`. Initial state: all
generation disabled; `EYE_LINEAR` mode with `p1 = 1` for s and `p2 = 1` for t,
all other `pi` zero.

### CURRENT

**STATUS: MISSING** for the core `glTexEnv*`/`glTexGen*` API, with one
**PARTIAL** extension shadow.

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglTexEnv(f|i|v|fv|iv)\b|\bglTexGen(f|i|d|v|fv|iv|dv)\b" src/ include/
# → no output (exit status 1)
```

All 15 L06 commands are absent from the exported surface. `glGetTexEnvfv`,
`glGetTexEnviv`, `glGetTexGenfv`, `glGetTexGeniv`, `glGetTexGendv` are equally
absent.

* **PARTIAL shadow — EXT multitexture state.** `Context::ExtState` carries
  `texEnvF` / `texEnvI` / `texGenF` / `texGenI`
  (`include/tglmt/Context.h:582-585`), driven by `glMultiTexEnvfEXT` and
  friends in `src/gl/gl_ext_dsa_state.cpp:358-435`. The key function is
  `static uint64_t Key(GLuint unit, GLenum pname)`
  (`src/gl/gl_ext_dsa_state.cpp:17`) = `(unit << 32) | pname`. That key **does
  not include the `target`** and, for texgen, **does not include the
  coordinate** — so `GL_TEXTURE_ENV` and `GL_TEXTURE_FILTER_CONTROL`, and the
  four texgen coordinates, all collide in one slot. The state is therefore
  lossy and cannot be reused verbatim for the legacy per-unit, per-target,
  per-coordinate model; it is however a useful proof that the extension path
  compiles and that the shadow plumbing works.

* **Texture-unit count.** The modern path already has 32 texture units
  (`kMaxTextureUnits = 32`, `include/tglmt/StateTracker.h:17`) with a per-unit,
  per-target binding table (`include/tglmt/StateTracker.h:287`), which is
  strictly more than the legacy minimum (2 units in GL 1.3, `MAX_TEXTURE_UNITS`
  ≥ 2 per glspec21 table 6.36). The legacy texture environment state must be
  added **per unit**.

### PLAN

* Per-unit texture-environment state: mode, environment colour, and the full
  `GL_COMBINE` argument set, plus the LOD bias (`TEXTURE_FILTER_CONTROL`).
* Per-unit, per-coordinate texgen state: mode, object plane, eye plane.
* Both are uploaded to the fixed-function fragment stage as a uniform block
  (see the mapping document for the MSL).
* `GL_COMBINE`'s `DOT3_RGB`/`DOT3_RGBA` and `ADD_SIGNED`/`INTERPOLATE` are
  the only modes that need care: `ADD_SIGNED` clamps the operand to `[-1,1]`
  before adding, `INTERPOLATE` is `a + 2b + c − 1`, and the DOT3 modes produce a
  4-component result that is replicated.

---

## 4. Group L04 — Fixed-function lighting, materials and shading model

### SPEC

**When lighting applies.** "Lighting is turned on or off using the generic
Enable or Disable commands with the symbolic value `LIGHTING`. If lighting is
off, the current color and current secondary color are assigned to the vertex
primary and secondary colors… If lighting is on, colors computed from the
current lighting parameters are assigned to the vertex primary and secondary
colors." (glspec21 §2.14.1.)

**Light count.** "There are n light sources … (n is an implementation dependent
maximum that **must be at least 8**)." Each light is enabled/disabled with
`GL_LIGHT0`…`GL_LIGHTn`.

**Material parameters** (`glMaterial`): `AMBIENT`, `DIFFUSE`, `SPECULAR`,
`EMISSION`, `SHININESS`, and `COLOR_INDEXES`. `AMBIENT_AND_DIFFUSE` sets both
`acm` and `dcm`. `face` is `FRONT`, `BACK` or `FRONT_AND_BACK`.

**Light source parameters** (`glLight`): `AMBIENT`, `DIFFUSE`, `SPECULAR`,
`POSITION` (object coordinates, transformed by the model-view matrix **at the
time the position is specified**), `SPOT_DIRECTION` (transformed by the upper
leftmost 3×3 of the model-view matrix at the time it is specified),
`SPOT_EXPONENT`, `SPOT_CUTOFF`, `CONSTANT_ATTENUATION`, `LINEAR_ATTENUATION`,
`QUADRATIC_ATTENUATION`.

**Light model parameters** (`glLightModel`): `AMBIENT` (scene ambient),
`LOCAL_VIEWER`, `TWO_SIDE`, `COLOR_CONTROL` (`SINGLE_COLOR` or
`SEPARATE_SPECULAR_COLOR`).

**Attenuation** is `1 / (k0 + k1·d + k2·d²)` where `d` is the distance from the
light to the vertex.

**Spotlight** uses the angle between the eye-to-light vector and the spot
direction, with `SPOT_CUTOFF` = 180° meaning "no cutoff".

**Default values** (glspec21 table 2.10): material ambient `(0.2,0.2,0.2,1)`,
diffuse `(0.8,0.8,0.8,1)`, specular `(0,0,0,1)`, emission `(0,0,0,1)`,
shininess `0`; light 0 diffuse/specular `(1,1,1,1)`, lights *i*>0
`(0,0,0,1)`, position `(0,0,1,0)`, spot direction `(0,0,−1)`, spot exponent `0`,
spot cutoff `180°`, attenuation `k0=1, k1=0, k2=0`; light model ambient
`(0.2,0.2,0.2,1)`, local viewer `FALSE`, two-side `FALSE`, colour control
`SINGLE_COLOR`.

**Errors.** `glLight`/`glMaterial`/`glLightModel` raise `INVALID_ENUM` for an
unknown `pname`/`light`/`face`; `INVALID_VALUE` if a parameter is outside its
allowable range (e.g. shininess outside `[0,128]`, spot cutoff outside
`[0,90]` ∪ {180}, spot exponent outside `[0,128]`, negative attenuation).

**`glColorMaterial`.** Attaches one or more material properties to the current
colour so they track it continuously (glspec21 §2.14.3). `face` is
`FRONT`/`BACK`/`FRONT_AND_BACK`; `pname` is `AMBIENT`/`DIFFUSE`/`SPECULAR`/
`EMISSION`/`AMBIENT_AND_DIFFUSE`.

**`glShadeModel`.** `GL_SMOOTH` (initial) or `GL_FLAT`. Flatshading assigns the
primitive the colour of "the vertex that spawned the primitive", and **which**
vertex that is depends on how the primitive was generated (glspec21 table 2.12):

| primitive | flat-shade source vertex |
|---|---|
| single polygon (*i* ≡ 1) | vertex 1 |
| triangle strip *i* | vertex *i*+2 |
| triangle fan *i* | vertex *i*+2 |
| independent triangle | vertex 3*i* |
| quad strip *i* | vertex 2*i*+2 |
| independent quad | vertex 4*i* |

Note that this is **not** Metal's provoking-vertex convention (see the mapping
document).

**Evaluation point.** Fixed-function lighting is evaluated **per vertex**
(the lighting equation runs on vertex data), and the resulting colour is
interpolated across the primitive — except under `GL_FLAT`.

### CURRENT

**STATUS: MISSING.**

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglLight(f|i|v|fv|iv|Model|Modelf|Modelfv|Modeli|Modeliv)\b|\bglMaterial(f|i|v|fv|iv)\b|\bglColorMaterial\b|\bglShadeModel\b" src/ include/
# → no output (exit status 1)
```

All 18 L04 commands are absent from the exported surface.

* `GL_LIGHTING` (0x0B50), `GL_COLOR_MATERIAL` (0x0B57), `GL_LIGHT0` (0x4000),
  `GL_RESCALE_NORMAL` (0x803A) and `GL_NORMALIZE` (0x0BA1) are **not** in the
  accepted-pname list (`grep -c "0x0B50u" include/tglmt/gl46_pnames.h` → 0), so
  `glGetIntegerv(GL_LIGHTING)` currently raises `GL_INVALID_ENUM`.

* `glEnable`/`glDisable` do accept *any* enum value and store it in
  `StateTracker::caps_` (`include/tglmt/StateTracker.h:266`,
  `src/core/StateTracker.cpp:6-20`). Consequence: `glEnable(GL_LIGHTING)`
  today **succeeds and is queryable** (`glIsEnabled(GL_LIGHTING)` returns
  `GL_TRUE`) while doing nothing at all. That is a silent-wrong-answer hazard
  for legacy applications and must be fixed as part of the compatibility work —
  either by implementing the pipeline or by making the compat context reject the
  call.

* The modern path has no lighting concept whatsoever; the shader pipeline
  computes colour in the fragment shader. There is no place where a light count,
  attenuation or spot cone could be hooked in.

### PLAN

* Per-context lighting state: up to 8 (spec minimum) lights with their ten
  parameters each, front and back material with five parameters each, the four
  light-model parameters, the shade model, and the `glColorMaterial` binding.
* Lighting is computed **in the generated fixed-function vertex shader**, per
  vertex, exactly as the spec describes; the colour is then interpolated. The
  `GL_FLAT` case is special (§3/§4 of the mapping document).
* `glColorMaterial` must be resolved at *vertex specification time* (the
  material tracks the current colour continuously), so it is applied on the CPU
  while the legacy assembler fills the vertex, not in the shader.

---

## 5. Group L05 — Fog

### SPEC

**Where fog sits.** "If enabled, fog blends a fog color with a rasterized
fragment's **post-texturing** color using a blending factor f." (glspec21 §3.10.)
Fog is therefore applied **after** texturing and **before** the per-fragment
operations of chapter 4.

**The three equations** (glspec21 eq. 3.31–3.33):

```
GL_LINEAR :  f = (e − c) / (e − s)
GL_EXP    :  f = exp(−d·c)
GL_EXP2   :  f = exp(−(d·c)²)
```

with `c` the fog coordinate, `d` the density, `s` the start and `e` the end.
"If `e` is selected, `s = e`, results are undefined."

**Fog coordinate source.** `GL_FOG_COORD_SRC` selects `GL_FRAGMENT_DEPTH`
(default) or `GL_FOG_COORD`. With `FRAGMENT_DEPTH`, `c` is the eye-coordinate
distance from the eye to the fragment centre; with `FOG_COORD`, `c` is the
interpolated fog coordinate.

**Clamping.** "No matter which equation and approximation is used to compute f,
the result is clamped to [0, 1]."

**Colour.** `C = f·Cr + (1−f)·Cf` per RGB component; **alpha is not changed**
by fog.

**Initial state.** Fog disabled, `FOG_COORD_SRC = FRAGMENT_DEPTH`,
`FOG_MODE = EXP`, `d = 1.0`, `e = 1.0`, `s = 0.0`, `Cf = (0,0,0,0)`.

**Legal range.** `d` specified less than zero ⇒ `INVALID_VALUE`.

**`GL_FOG_HINT`.** A hint (GL 1.0 hints) that the app may use to select the
desired fog quality; the GL specification explicitly allows a hint to be
ignored, but for a *compatibility* context it is the only knob an application
has.

### CURRENT

**STATUS: MISSING** for the fog API; the *enable* half is silently accepted.

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglFog(f|i|v|fv|iv)\b" src/ include/
# → no output (exit status 1)
```

All 4 L05 commands are absent from the exported surface.
`glFogCoord*` (8 commands) live in L01 and are equally absent.

* `GL_FOG` (0x0B60) is not in the accepted-pname list
  (`grep -c "0x0B60u" include/tglmt/gl46_pnames.h` → 0), so
  `glGetIntegerv(GL_FOG)` raises `GL_INVALID_ENUM`. But
  `glEnable(GL_FOG)` is accepted and stored in `StateTracker::caps_`
  (same mechanism as `GL_LIGHTING` above) and is therefore *queryable as
  enabled* while having no effect.

* `glHint` is a **no-op** (`src/gl/gl_raster.cpp:273`:
  `void glHint(GLenum t, GLenum m) { (void)t;(void)m; }`). For the core profile
  this is spec-legal (hints may be ignored), but `GL_FOG_HINT` is meaningless
  in the compatibility profile if fog itself is implemented and the hint is
  ignored.

### PLAN

* Fog state: mode, density, start, end, colour, coordinate source, plus the
  `GL_FOG_HINT` value.
* The fragment stage of the generated fixed-function shader applies fog after
  the texture-environment step, using the eye-space distance interpolated from
  the vertex stage (the `GL_FRAGMENT_DEPTH` source) or the interpolated fog
  coordinate (the `GL_FOG_COORD` source, which requires the `glFogCoord*`
  client attribute from L01).
* `GL_FOG_HINT` is stored and honoured by choosing between the exact
  exponential and a linear approximation in the generated shader — the hint
  mechanism the spec intends.

---

## 6. Groups L09/L10 — Selection, feedback and the accumulation buffer

### SPEC

**Selection (`GL_SELECT`).** `glSelectBuffer(n, buffer)` supplies the array that
receives hit records; `glRenderMode(GL_SELECT)` switches the GL out of normal
rendering — "In selection mode, framebuffer updates as described in chapter 4
are not performed."

A **hit record** (glspec21 §5.2) contains, in order: the number of elements on
the name stack at the time of the hit, a minimum depth, a maximum depth, and the
name stack bottom-first. The depths are the min/max over the **window-coordinate
z** of each post-clipping vertex of the primitive, each multiplied by `2³²−1`
and rounded; "No depth offset arithmetic is performed on these values."

A hit occurs when a point, line, polygon — or the valid coordinates produced by
a `RasterPos` command — intersects the clip volume. `WindowPos` **always**
generates a hit. A culled polygon does **not** hit.

Errors: `glRenderMode` to `GL_SELECT` before `glSelectBuffer` ⇒
`INVALID_OPERATION`; `glSelectBuffer` while in selection mode ⇒
`INVALID_OPERATION`. `glPushName` on a full stack ⇒ `STACK_OVERFLOW`;
`glPopName` on an empty stack ⇒ `STACK_UNDERFLOW`; `glLoadName` on an empty
stack ⇒ `INVALID_OPERATION`. The name stack depth is at least 64.

`glRenderMode(GL_SELECT)` **returns** the number of hit records copied, or −1 if
the selection array overflowed.

**Feedback (`GL_FEEDBACK`).** `glFeedbackBuffer(n, type, buffer)` supplies the
array; `glRenderMode(GL_FEEDBACK)` switches to feedback mode. Each primitive
that *would have been* rasterized produces a block: a primitive-type token
followed by the vertex data in the requested layout. Table 5.2 gives the
per-vertex value counts:

| type | values per vertex |
|---|---|
| `GL_2D` | 2 |
| `GL_3D` | 3 |
| `GL_3D_COLOR` | 3 + k |
| `GL_3D_COLOR_TEXTURE` | 7 + k |
| `GL_4D_COLOR_TEXTURE` | 8 + k |

with `k = 1` in colour-index mode and `k = 4` in RGBA mode. The coordinates
returned are **window** coordinates; `w` is in clip coordinates. Feedback
occurs after polygon culling and after `PolygonMode` interpretation.
`glPassThrough(token)` inserts a marker token and "may not occur between Begin
and End".

Errors: entering feedback mode before `glFeedbackBuffer` ⇒
`INVALID_OPERATION`; `glFeedbackBuffer` while in feedback mode ⇒
`INVALID_OPERATION`. `glRenderMode` returns the number of values placed in the
buffer, or −1 on overflow.

**Accumulation buffer (`glAccum`).** "Each portion of a pixel in the
accumulation buffer consists of four values: one for each of R, G, B, and A."
Operations: `ACCUM` (read the framebuffer, multiply by `value`, add to the
accumulation buffer), `LOAD` (like ACCUM but replace), `RETURN` (multiply the
accumulation buffer by `value`, clamp to `[0,1]`, write to the enabled colour
buffers), `MULT` (multiply the accumulation buffer by `value`), `ADD` (multiply
then add `value`).

Accumulation values are signed in `[−1,1]` and are clamped only on `RETURN`.
The scissor test restricts which pixels an `Accum` operation affects. If there
is no accumulation buffer, `glAccum` raises `INVALID_OPERATION`.
`glClearAccum` sets the initial accumulation values.

### CURRENT

**STATUS: MISSING** for both groups, and for the accumulation buffer.

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(SelectBuffer|FeedbackBuffer|RenderMode|InitNames|PushName|PopName|LoadName|PassThrough)\b" src/ include/
# → no output (exit status 1)

grep -rnE "\bgl(Accum|ClearAccum|GetAccum)\b" src/ include/
# → no output (exit status 1)
```

All 8 L09 commands and both L10 commands are absent from the exported surface.

* `GL_RENDER_MODE` (0x0C00) is not an accepted pname, so
  `glGetIntegerv(GL_RENDER_MODE)` raises `GL_INVALID_ENUM`.

* There is no second colour buffer in TGLMT's model. The framebuffer model is
  `FramebufferObject` with `colorTex` attachments plus depth/stencil
  (`include/tglmt/Context.h:493-524`); the default framebuffer is a single
  `IRenderTarget` (`Context::defaultRenderTarget`, `src/core/Context.cpp:56`
  path). **There is no accumulation buffer anywhere**, and `glClear`'s
  `ACCUM_BUFFER_BIT` is not a thing.

* Metal has no accumulation-buffer concept either: the only accumulation-like
  feature is `MTLStoreActionMultisampleResolve` (MSAA resolve), which is a
  different operation.

### PLAN

* Selection and feedback are **client-side** operations in the sense that their
  results are written to *client* memory, but they still need per-primitive
  geometric information that only the rasterizer has. The design chosen here is
  a dedicated **selection/feedback pass** driven by the legacy path (see the
  mapping document): the legacy draw path, when the render mode is
  `GL_SELECT`/`GL_FEEDBACK`, does not rasterize into the framebuffer at all but
  instead evaluates the primitive geometry on the CPU (the legacy path already
  knows the client vertices, the current matrices and the clip volume) and
  writes hit records / feedback blocks directly into the user buffers. This
  avoids needing a second Metal pass and gives exact spec semantics for the
  z-range rule.
* The accumulation buffer is emulated with a dedicated off-screen RGBA float
  texture owned by the context, plus the four `glAccum` operations as
  render-pass operations (accumulate/add/multiply are expressible as a
  full-screen quad blend; `RETURN` is a copy back into the colour buffer).
  `glClearAccum` sets the texture's clear colour.

---

## 7. Groups L13/L14/L16 and the rest of legacy rasterisation

### SPEC

**Alpha test (L14).** "The alpha test discards a fragment conditional on the
outcome of a comparison between the incoming fragment's alpha value and a
constant value… The comparison is enabled or disabled with the generic Enable
and Disable commands using the symbolic constant `ALPHA_TEST`. When disabled, it
is as if the comparison always passes." `glAlphaFunc(func, ref)` with `func` ∈
`{NEVER, ALWAYS, LESS, LEQUAL, EQUAL, GEQUAL, GREATER, NOTEQUAL}`; `ref` is
clamped to `[0,1]`. Initial state: disabled, func `ALWAYS`, ref `0`.
(glspec21 §4.1.4.)

**Stipple (L13).**

* *Polygon stipple* (`glPolygonStipple`, GL 1.0): a 32×32 bit mask in window
  coordinates; fragments whose corresponding mask bit is 0 are discarded. The
  pattern is a *fixed* 32×32 bitmap, not a user-supplied pattern (that is
  `glPolygonStipple`'s pattern argument: a pointer to 32 rows of 8 bytes each,
  the low 8 bits of each byte used).
* *Line stipple* (`glLineStipple(factor, pattern)`, GL 1.0): `factor` is the
  number of consecutive fragments that are passed, alternating with the number
  that are discarded; `pattern` is a 16-bit mask used from the least significant
  bit upwards, repeating. `factor` must be in `[1,256]`, else `INVALID_VALUE`.
  The pattern is reset at the start of each line segment.

**User clip planes (L16).** `glClipPlane(plane, equation)` sets one of six
user-defined clipping half-spaces, expressed in eye coordinates as
`A·x + B·y + C·z + D ≥ 0`. `plane` is `GL_CLIP_PLANE0`…`GL_CLIP_PLANE5`;
an unknown plane ⇒ `INVALID_ENUM`. `glGetClipPlane` returns the four
coefficients. The planes are enabled with `GL_CLIP_PLANE0`…`GL_CLIP_PLANE5` and
are intersected with the clip volume (glspec21 §2.12). Minimum number of planes
is 6 (table 6.33).

**Legacy rasterisation state that already exists in the modern core profile.**
The following are *not* legacy-only and are already implemented in TGLMT, but
they interact with the legacy pipeline and are listed for completeness:

| feature | GL semantics | TGLMT status |
|---|---|---|
| `glPolygonMode` | `GL_FILL`, `GL_LINE`, `GL_POINT`; the face argument may be `FRONT`, `BACK` or `FRONT_AND_BACK` in the compatibility profile | `PARTIAL` — `StateTracker::SetPolygonMode` (`src/core/StateTracker.cpp:79-87`) collapses `FRONT`/`BACK` into `FRONT_AND_BACK` and only `FILL`/`LINE` are distinguished; `GL_POINT` is not implemented (the modern path has no point-mode polygon expansion) |
| `glPolygonOffset` | offset = `factor × dz + r × units`, `r` = minimum resolvable depth difference | `PRESENT` — `StateTracker::SetPolygonOffset` (`src/core/StateTracker.cpp:64-76`) → `enc->setDepthBias` (`src/gl/gl_apple_draw.cpp:2793-2801`) |
| `glPointSize` | size of rasterized points | `PARTIAL` — stored in the shadow (`src/gl/gl_raster.cpp:142-148`) and queryable, but the modern path never applies it: the Metal point size comes from the shader's `[[point_size]]` output (`src/gl/glsl_to_msl.cpp:11819`), and `glPointSize` is not connected to it |
| `glScissor` | box restricting writes | `PRESENT` — `SetScissor` (`include/tglmt/StateTracker.h:148`) → `enc->setScissorRect` (`src/gl/gl_apple_draw.cpp:2660`) |
| `glColorMask` | per-channel write mask | `PRESENT` — `StateTracker::SetColorMask` (`include/tglmt/StateTracker.h:76`) → `AttachmentBlend::writeMask` (`src/gl/gl_apple_draw.cpp:2326`) |
| `glLogicOp` | 16 bitwise logical operations between fragment and framebuffer colour, applied **instead of** blending | `PARTIAL` — the opcode is stored (`src/gl/gl_raster.cpp:243-249`) and blending is correctly bypassed (`src/gl/gl_apple_draw.cpp:2321`), but **the operation itself is never performed**: Metal has no logic-op stage |
| `glHint` | quality hints (`GL_FOG_HINT`, `GL_LINE_SMOOTH_HINT`, `GL_POLYGON_SMOOTH_HINT`, `GL_PERSPECTIVE_CORRECTION_HINT`) | `PARTIAL` — accepted and ignored (`src/gl/gl_raster.cpp:273`) |

**Colour-index mode (L18).** `glClearIndex` and `glIndexMask` control the
colour-index clear value and write mask. The compatibility profile allows a
window system to supply colour-index visuals; the 4.6 core profile states "no
color index visuals are supplied by the window system-binding APIs such as GLX
and WGL, so the default framebuffer is always in RGBA mode" (glspec46 App. E.2).
On iOS/CAMetalLayer there are no colour-index visuals, so the sensible
compatibility behaviour is: accept the commands, keep the state, and have no
effect (RGBA-only default framebuffer). `glIndexMask` affects only colour-index
buffers.

**Texture residency and priority (L15).** `glAreTexturesResident(n, ids,
residency)` and `glPrioritizeTextures(n, ids, priorities)`. The priorities
feed the texture-environment LOD bias mechanism in the fixed-function pipeline.
`glAreTexturesResident` must report residency for the *texture image units*
actually bound in the current context.

### CURRENT

**STATUS: MISSING** for L13 (2 of 3), L14, L15 and L16; the third L13 command
is a **stub**.

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bglAlphaFunc\b|\bglClipPlane\b|\bglPolygonStipple\b|\bglGetPolygonStipple\b" src/ include/
# → no output (exit status 1)

grep -rnE "\bglAreTexturesResident\b|\bglPrioritizeTextures\b|\bglClearIndex\b|\bglIndexMask\b" src/ include/
# → no output (exit status 1)
```

* **`glLineStipple` is exported but is an empty stub.**
  `src/gl/gl_raster.cpp:274`:
  `void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }`.
  It is registered in the export table (`src/gl/gl_c_exports.cpp:2810-2812`)
  and in `src/gl/gl_ext_aliases_gen.cpp`, and it is counted as "implemented" by
  the group script, but it has **no effect whatsoever**. This is the one place
  where the export census over-reports: the honest state is *exported, no-op*.
  (`docs/OPENGL_API_COVERAGE.md` §6.2 already calls this out for the core
  surface; for the compat surface the same caveat applies.)

* `GL_ALPHA_TEST` (0x0BC0) and `GL_CLIP_PLANE0` (0x3000) are not accepted
  pnames, so querying them raises `GL_INVALID_ENUM`; enabling them through
  `glEnable` succeeds silently and does nothing.

* `glPolygonStipple` has no Metal counterpart and no CPU counterpart.

* `glAreTexturesResident` has no implementation; the modern path's texture
  residency is implicit (a texture with a null `gpu` handle samples as the
  black fallback texture, `src/gl/gl_apple_draw.cpp:112-119`).

### PLAN

* **Alpha test** becomes a `discard_fragment()` in the generated fixed-function
  fragment shader, gated by a uniform compare function and reference. This is
  exact, not an approximation.
* **Polygon stipple** becomes a 32×32 bit mask uploaded as a uniform/constant
  buffer and tested in the fragment shader against `gl_FragCoord` — again exact.
* **Line stipple** is harder: Metal's line rasteriser gives no fragment
  coverage information and lines are 1 pixel wide, so a stipple pattern along a
  line cannot be reproduced by the fragment stage alone. Options are analysed in
  the mapping document; the leading candidate is to expand lines into
  triangle strips on the CPU with a per-vertex line coordinate, so the fragment
  stage can apply the pattern exactly.
* **User clip planes** are the one genuinely hardware-limited item: Metal
  exposes exactly one `[[clip_distance]]` output
  (`include/tglmt/MetalInterface.h` does not wrap it; the MSL spec table 5.4
  allows `float[n]`, but the Metal compiler rejects arrays — see
  `src/gl/glsl_to_msl.cpp:6024-6027` and `:6072`, where index ≠ 0 is an
  explicit "Metal chỉ nhận MỘT scalar" failure). Six user planes therefore
  cannot be expressed natively; the emulation options are analysed in the
  mapping document.
* `glAreTexturesResident` is answered from the texture objects themselves
  (a texture is resident when it has a realised GPU handle); `glPrioritizeTextures`
  feeds the legacy LOD-bias path of the texture environment.

---

## 8. Group L07 — Display lists

### SPEC

**Model.** "A display list is simply a group of GL commands and arguments that
has been stored for subsequent execution… This causes the commands within the
list to be executed just as if they were given normally. The only exception
pertains to commands that rely upon client state. When such a command is
accumulated into the display list (that is, when issued, not when executed), the
client state in effect at that time applies to the command."

**Creation.** `glNewList(list, mode)` begins a list; `mode` is `GL_COMPILE` (do
not execute) or `GL_COMPILE_AND_EXECUTE` (execute *and* store). `list` must be
a positive integer; `list = 0` ⇒ `INVALID_VALUE`. `glEndList()` ends it and is
the point at which the list is actually associated with the index.

**Errors.** `glEndList` without a matching `glNewList` ⇒ `INVALID_OPERATION`;
`glNewList` a second time before `glEndList` ⇒ `INVALID_OPERATION`;
`glEndList` when the list cannot be stored ⇒ `OUT_OF_MEMORY` (in which case the
previous contents of the list and all other GL state are unchanged, except for
state changed by execution under `COMPILE_AND_EXECUTE`).

**Execution.** `glCallList(list)`; `list = 0` ⇒ `INVALID_VALUE`.
`glCallLists(n, type, lists)` executes *n* lists, each offset by the current
`glListBase`; the offset encoding depends on `type` (`BYTE`, `UNSIGNED_BYTE`,
`SHORT`, `UNSIGNED_SHORT`, `INT`, `UNSIGNED_INT`, `FLOAT`, `2_BYTES`,
`3_BYTES`, `4_BYTES`); `n = 0` does nothing.

**Names.** `glGenLists(range)` returns a contiguous block of previously unused
indices and *creates* an empty list for each, or 0 if no such block exists or
`range = 0`. `glIsList(list)` reports whether the index is a display list.
`glDeleteLists(list, range)` frees them; `range = 0` does nothing; indices that
do not correspond to a list are ignored.

**Nesting.** `glCallList`/`glCallLists` may appear inside a display list.
Under `COMPILE_AND_EXECUTE` the nested list is executed but the *call* is what
is stored. An implementation-dependent nesting limit applies, and it **must be
at least 64**.

**Commands that are executed immediately rather than compiled into the list**
(glspec21 §5.4, an explicit list):

* display lists: `GenLists`, `DeleteLists`
* render modes: `FeedbackBuffer`, `SelectBuffer`, `RenderMode`
* vertex arrays: `ClientActiveTexture`, `ColorPointer`, `EdgeFlagPointer`,
  `FogCoordPointer`, `IndexPointer`, `InterleavedArrays`, `NormalPointer`,
  `SecondaryColorPointer`, `TexCoordPointer`, `VertexAttribPointer`,
  `VertexPointer`
* client state: `EnableClientState`, `DisableClientState`,
  `EnableVertexAttribArray`, `DisableVertexAttribArray`, `PushClientAttrib`,
  `PopClientAttrib`
* pixels and textures: `PixelStore`, `ReadPixels`, `GenTextures`,
  `DeleteTextures`, `AreTexturesResident`
* occlusion queries: `GenQueries`, `DeleteQueries`
* vertex buffer objects: `GenBuffers`, `DeleteBuffers`, `BindBuffer`,
  `BufferData`, `BufferSubData`, `MapBuffer`, `UnmapBuffer`
* program and shader objects: `CreateProgram`, `CreateShader`, `DeleteProgram`,
  `DeleteShader`, `AttachShader`, `DetachShader`, `BindAttribLocation`,
  `CompileShader`, `ShaderSource`, `LinkProgram`, `ValidateProgram`
* command stream management: `Finish`, `Flush`
* **all** query commands whose names begin with `Get` and `Is`

**Buffer semantics.** "GL commands that source data from buffer objects
dereference the buffer object data in question at display list compile time,
rather than encoding the buffer ID and buffer offset into the display list."

**`glListBase(base)`** sets the base used by `glCallLists`.

### CURRENT

**STATUS: MISSING.**

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(NewList|GenLists|EndList|CallList|CallLists|DeleteLists|IsList|ListBase)\b" src/ include/
# → no output (exit status 1)
```

All 8 L07 commands are absent from the exported surface. `GL_DISPLAY_LIST`
(0x82E7) is not an accepted pname.

There is **no display-list machinery of any kind** in TGLMT:
`ObjectRegistry::ObjectKind` (`include/tglmt/ObjectRegistry.h:12-15`) lists
`Buffer, Texture, Sampler, VertexArray, Framebuffer, Renderbuffer, Program,
Shader, ProgramPipeline, Query, Sync, TransformFeedback` — there is no
`DisplayList` kind, and `ObjectRegistry` is a name allocator only
(`next_`, `live_`), not a command store.

### PLAN

* Display lists are implemented as a **command stream**, not as a cached draw.
  This is the semantically correct choice and it is also the only one that can
  preserve the "client state at compile time, server state at execution time"
  rule (§8 "The only exception pertains to commands that rely upon client
  state"). A cached-draw representation would break, for example, a list that
  contains `glTranslate` followed by a `glBegin/glEnd` block: the translation
  must be re-evaluated at call time, not baked.
* A `DisplayList` object type is added to `ObjectRegistry` (or a sibling
  container) holding an ordered vector of recorded commands.
* The recorded command set is the full GL command stream **minus** the
  "executed immediately" list of §8 — that list must be honoured exactly, and
  it is long. The implementation therefore needs a mechanism to intercept
  commands while a list is open: either a dispatcher flag or a per-entry-point
  "record instead of execute" hook. Because the command set is ~700 entry
  points, the dispatcher-flag approach is the only maintainable one.
* Nesting depth is tracked and limited to ≥ 64, as the spec requires.

---

## 9. Group L08 — Evaluators

### SPEC

**Model.** "Evaluators provide a means to use a polynomial or rational
polynomial mapping to produce vertex, normal, and texture coordinates, and
colors. The values so produced are sent on to further stages of the GL as if
they had been provided directly by the client. **Transformations, lighting,
primitive assembly, rasterization, and per-pixel operations are not affected by
the use of evaluators.**" (glspec21 §5.1.)

**`glMap1{target, u1, u2, stride, order, points}`.** `target` ∈
`{GL_MAP1_VERTEX, GL_MAP1_COLOR, GL_MAP1_NORMAL, GL_MAP1_INDEX,
GL_MAP1_TEXTURE_COORD_1..4}`; the number of components per control point is
table 5.1 (3 or 4 for vertex, 4 for colour, 3 for normal, 1 for index, 1..4 for
texture coordinates). `order` is the polynomial degree + 1; `INVALID_VALUE` if
`order < 1` or `order > MAX_EVAL_ORDER` (minimum 8, table 6.33). `stride` is
the number of values per control-point block; `INVALID_VALUE` if
`stride < k`. `INVALID_VALUE` if `u1 = u2`. The evaluation is

```
p′(u′) = p( (u′ − u1) / (u2 − u1) )
```

with `p` the Bernstein polynomial of degree `n = order − 1`.

**`glMap2{target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points}`.**
The bivariate form `p(u,v) = ΣᵢΣⱼ Bⁿᵢ(u)·Bᵐⱼ(v)·Rᵢⱼ`, with control point
`Rᵢⱼ` at `(ustride)·i + (vstride)·j` values past `points`. `uorder`/`vorder`
must be in `[1, MAX_EVAL_ORDER]`. Evaluation is
`p′(u′,v′) = p( (u′−u1)/(u2−u1), (v′−v1)/(v2−v1) )`.

**`glEvalCoord1{x,y}` / `glEvalCoord2{x,y}`.** Evaluates the map at the given
coordinate(s) and produces a vertex (plus the associated colour/normal/texcoord
data) which is then processed exactly as a `glVertex` would be.

**`glEvalPoint1/2`.** Evaluates the 1-D/2-D map and produces a single point.

**`glEvalMesh1(mode, n1, n2)` / `glEvalMesh2(mode, n1, n2, n3, n4)`.** Evaluates
the map on a grid. `glEvalMesh1` takes a 1-D map (`GL_LINEAR` or `GL_POINT`)
and generates `n1` points or `n2` line segments over `[u1,u2]`. `glEvalMesh2`
takes a 2-D map and generates an `n1`×`n2` grid of points, or `n3`×`n4` grid of
line segments. The generated vertices are fed into the pipeline exactly as if
they had been specified with `glVertex`.

**`glMapGrid1/2`.** Defines a uniform grid in the `u` (and `v`) domain that
`glEvalMesh` uses instead of the map's own domain.

**Auto-generated normals.** The evaluator can produce normals from the surface
itself; the compatibility profile's `glEvalMesh2` in `GL_LINEAR` mode with a
2-D map of normals is how an application gets a normal for each grid point.
(The "automatic normal generation" is a property of how the application maps a
`GL_MAP2_NORMAL` map onto the grid; the GL does not synthesise normals from a
position map by itself.)

### CURRENT

**STATUS: MISSING.**

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(Map1|Map2|MapGrid|EvalCoord|EvalMesh|EvalPoint|GetMap)[fd]?\b" src/ include/
# → no output (exit status 1)
```

All 23 L08 commands are absent from the exported surface.

* `glEvalCoord*` and `glEvalPoint*` are on the **allowed** list inside
  `glBegin/glEnd` (§1), so their absence means immediate mode cannot support
  the evaluator workflow at all.

* `glGetMapdv/fv/iv` would return the control points; nothing in the codebase
  stores map control points.

### PLAN

* Map state: for each of the 1-D and 2-D maps, the target, the domain
  (`u1,u2` / `v1,v2`), the order(s), the strides and the control-point array.
  `MAX_EVAL_ORDER` is 8 minimum; the implementation should support at least
  that.
* Evaluation is a **Bernstein polynomial** evaluation. It is a CPU-side
  computation with no GPU analogue in Metal, so the natural implementation is
  `CPU_EMU`: evaluate the map, then feed the resulting vertices into the same
  legacy assembler that `glVertex*` uses, so the rest of the pipeline
  (transform, lighting, rasterisation) is untouched — which is exactly what the
  spec demands ("Transformations, lighting, primitive assembly, rasterization,
  and per-pixel operations are not affected by the use of evaluators").
* `glEvalMesh1/2` produce a grid of vertices: `n1`×`n2` for `glEvalMesh2`.
  Because the number of generated vertices is unbounded by the spec, the
  assembler must grow dynamically.

---

## 10. Group L11/L12 — Pixel operations, raster position and window position

### SPEC

**Pixel storage modes.** `glPixelStore*` sets the pack/unpack state:
`SWAP_BYTES`, `LSB_FIRST`, `ROW_LENGTH`, `SKIP_ROWS`, `SKIP_PIXELS`,
`ALIGNMENT`, `SKIP_IMAGES`, `IMAGE_HEIGHT`. The GL 1.1 additions (`SWAP_BYTES`,
`LSB_FIRST`) are the ones TGLMT's modern path does **not** accept.

**Pixel transfer modes (GL 1.0).** `glPixelTransferf/i` sets `RED_SCALE`,
`GREEN_SCALE`, `BLUE_SCALE`, `ALPHA_SCALE`, `RED_BIAS`, `GREEN_BIAS`,
`BLUE_BIAS`, `ALPHA_BIAS`, `INDEX_SHIFT`, `INDEX_OFFSET`, `MAP_COLOR`,
`MAP_STENCIL`, plus `GL_POST_*` image-map variants and the colour-matrix
stacks. These transform pixel values on the way in and out of the framebuffer.

**Pixel maps.** `glPixelMapfv/uiv/usv` installs up to `MAX_PIXEL_MAP_TABLE`
(≥ 32) entries for `GL_PIXEL_MAP_*` targets (`R_TO_R`, `G_TO_G`, …,
`A_TO_A`, `INDEX_TO_INDEX`, `R_TO_B`, …, `DEPTH_TO_DEPTH`).

**`glDrawPixels`.** Writes a rectangle of pixel data into the framebuffer,
starting at the current raster position, using the current raster colour,
depth and stencil. The data goes through the pixel transfer operations and the
pixel maps.

**`glCopyPixels`.** Copies a rectangle of pixels from the read buffer to the
write buffer, at the current raster position, with format conversion and the
pixel transfer operations applied.

**`glBitmap`.** Writes a 1-bit-per-pixel bitmap as a mask, using the current
raster position and the current raster colour.

**`glPixelZoom`.** Magnifies or shrinks the pixel rectangles written by
`glDrawPixels` and `glBitmap`.

**Raster position (L12, 40 commands).** `glRasterPos{234}{sifd}[v]` sets the
current raster position by transforming the coordinate as if it were a vertex
(§1). `glWindowPos{234}{sifd}[v]` (GL 1.4) sets it directly in window
coordinates. The raster position has an associated colour, secondary colour,
texture coordinates and depth, exactly like a vertex.

### CURRENT

**STATUS: PARTIAL** — the modern pixel-storage/pixel-read machinery exists and
is good; every legacy pixel operation is missing.

*Present and reusable:*

* `glPixelStorei` (`src/gl/gl_texture.cpp:1691-1719`) accepts 12 core pnames
  including `ROW_LENGTH`, `SKIP_ROWS`, `SKIP_PIXELS`, `ALIGNMENT`,
  `IMAGE_HEIGHT`, `SKIP_IMAGES` — but **not** `GL_SWAP_BYTES` / `GL_LSB_FIRST`
  (they fall into the `default:` branch and raise `GL_INVALID_ENUM`,
  `src/gl/gl_texture.cpp:1711-1717`; note `glGetIntegerv` nevertheless
  *reports* `GL_UNPACK_SWAP_BYTES = 0` and `GL_UNPACK_LSB_FIRST = 0` at
  `src/gl/gl_get.cpp:373-374` — an inconsistency).
* `glReadPixels` exists and is implemented
  (`src/gl/gl_readpixels_dsa.cpp:89-…`), including PBO packing, region
  clamping, RGBA8 readback and format conversion.
* `glGetTexImage`/`glGetTexSubImage` and the `PACK_ROW_LENGTH`-aware pitch
  helper `PackRowLen` (`src/gl/gl_get_extra.cpp:33-39`) already implement the
  row-length part of the pack state.

*Missing:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(Bitmap|DrawPixels|CopyPixels|PixelMapfv|PixelMapuiv|PixelMapusv|PixelTransferf|PixelTransferi|PixelZoom|GetPixelMapfv|GetPixelMapuiv|GetPixelMapusv)\b" src/ include/
# → no output (exit status 1)

grep -rnE "\bgl(RasterPos|WindowPos)[234][sifd]v?\b" src/ include/
# → no output (exit status 1)
```

All 12 L11 commands and all 40 L12 commands are absent from the exported
surface.

* `glCopyPixels` is a near-miss: the modern path has `glBlitFramebuffer`
  (`src/gl/gl_framebuffer.cpp`) and `IDevice::blitCopy`
  (`include/tglmt/MetalInterface.h:813`) which can copy a rectangle between
  textures, and `glCopyTexImage*` already converts formats. The *legacy*
  semantics differ (source = the read buffer, destination = the write buffer at
  the current raster position, with the pixel transfer operations applied), so
  the modern blit is a building block, not a solution.

### PLAN

* The raster position is **CPU state** in the legacy path: it is produced by
  transforming a coordinate with the current matrices and the current viewport
  (the same arithmetic the legacy vertex path uses), and it carries an
  associated colour/texcoord/depth. `glWindowPos` sets it directly.
* `glDrawPixels`/`glBitmap`/`glCopyPixels` become client→framebuffer writes.
  The pragmatic implementation for a Metal backend is: build a texture from the
  client data (applying the pixel transfer operations and pixel maps on the CPU
  during the upload) and blit it into the framebuffer at the raster position;
  `glCopyPixels` reuses the existing blit path with a format conversion.
* `glPixelZoom` scales the written rectangle.
* `glPixelTransfer*` and `glPixelMap*` are CPU-side transforms applied on the
  pack and unpack paths — they compose with the existing `glReadPixels`
  conversion code and the texture upload path.

---

## 11. Group L02 — Client-side vertex arrays

### SPEC

**Model.** GL 1.1 introduced vertex arrays that live in **client memory**:
`glVertexPointer`, `glColorPointer`, `glNormalPointer`, `glIndexPointer`,
`glTexCoordPointer`, `glSecondaryColorPointer`, `glEdgeFlagPointer`,
`glFogCoordPointer`, plus `glEnableClientState`/`glDisableClientState`,
`glInterleavedArrays` and `glClientActiveTexture` for multitexture. Each array
is enabled/disabled by name (`GL_VERTEX_ARRAY`, `GL_COLOR_ARRAY`, …) and
described by `(size, type, stride, pointer)`.

**Data are read at draw time.** The array contents are read when the drawing
command is issued, not when the pointer is set — the modern core profile
removed this entirely: "all vertex array attribute and element array index
pointers must refer to buffer objects" (glspec46 App. E.2).

**`glInterleavedArrays(format, stride, pointer)`** sets up several arrays at
once from one interleaved buffer: `GL_V2F`, `GL_V3F`, `GL_C4UB_V2F`, `GL_C4UB_V3F`,
`GL_C3F_V3F`, `GL_C4F_N3F_V2F`, `GL_C4F_N3F_C4F_V2F`, `GL_C4F_N3F_C4F_V3F`,
`GL_N3F_V3F`, `GL_N3F_C4F_V3F`, `GL_T2F_V3F`, `GL_T4F_V4F`, `GL_T2F_C4UB_V3F`,
`GL_T2F_C3F_V3F`, `GL_T2F_C3F_C4UB_V2F`, `GL_T2F_C4F_N3F_V2F`,
`GL_T2F_C4F_N3F_C4F_V2F`, `GL_T4F_C4F_N3F_V2F`, `GL_T4F_C4F_N3F_C4F_V2F`.

**Errors.** Enabling an array whose pointer has not been set is an error;
`glVertexPointer` with a `size` outside `[1,4]` or an unsupported `type` ⇒
`INVALID_VALUE`; an unknown array name ⇒ `INVALID_ENUM`.

### CURRENT

**STATUS: MISSING.**

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(VertexPointer|NormalPointer|ColorPointer|IndexPointer|TexCoordPointer|EdgeFlagPointer|FogCoordPointer|SecondaryColorPointer|InterleavedArrays|EnableClientState|DisableClientState|ClientActiveTexture)\b" src/ include/
# → no output (exit status 1)
```

All 12 L02 commands are absent from the exported surface.

* **PARTIAL shadow — EXT client state.** `Context::ExtState::clientPtr` and
  `clientEnabled` (`include/tglmt/Context.h:586-587`) are driven by
  `glEnableClientStateIndexedEXT` / `glDisableClientStateIndexedEXT` /
  `glMultiTexCoordPointerEXT` / `glGetPointerIndexedvEXT`
  (`src/gl/gl_ext_dsa_state.cpp:328-356`). The EXT `glMultiTexCoordPointerEXT`
  stores the pointer and the enabled flag but **discards the size, type and
  stride** (`src/gl/gl_ext_dsa_state.cpp:351-356` — the parameters are named
  and then ignored). There is no element array support at all.

* The modern path is buffer-object-only: `glVertexAttribPointer`
  (`src/gl/gl_vertex.cpp:70-87`) binds to `GL_ARRAY_BUFFER` and stores the
  pointer as a byte offset (`a.relativeOffset = (size_t)ptr`), i.e. the client
  pointer is reinterpreted as an offset into a bound buffer. A true
  client-memory array has no such buffer.

### PLAN

* Client arrays are the **most awkward** part of the legacy surface for a
  Metal backend, because Metal has no client-memory vertex arrays at all: the
  vertex buffer must be an `MTLBuffer`.
* The pragmatic implementation is `CPU_EMU`: at draw time, the client arrays
  are read on the CPU and the vertices are assembled into the same staging
  buffer the immediate-mode path uses, then uploaded. This keeps one vertex
  layout and one pipeline for both legacy paths.
* The alternative — mapping the client pointer to a `newBufferWithBytes:` copy
  per draw — is the same thing with more allocations.
* `glInterleavedArrays` is expanded on the CPU into the equivalent per-attribute
  descriptors.

---

## 12. Group L17 — Attribute stacks and state restoration

### SPEC

**Two stacks.** `glPushAttrib`/`glPopAttrib` use the **server** attribute
stack; `glPushClientAttrib`/`glPopClientAttrib` use the **client** attribute
stack. Each takes a bitmask of attribute groups (glspec21 table 6.2):

| server | client |
|---|---|
| `GL_ACCUM_BUFFER_BIT` | `GL_CLIENT_VERTEX_ARRAY_BIT` |
| `GL_COLOR_BUFFER_BIT` | `GL_CLIENT_PIXEL_STORE_BIT` |
| `GL_CURRENT_BIT` | |
| `GL_DEPTH_BUFFER_BIT` | |
| `GL_ENABLE_BIT` | |
| `GL_EVAL_BIT` | |
| `GL_FOG_BIT` | |
| `GL_HINT_BIT` | |
| `GL_LIGHTING_BIT` | |
| `GL_LINE_BIT` | |
| `GL_LIST_BIT` | |
| `GL_MULTISAMPLE_BIT` | |
| `GL_PIXEL_MODE_BIT` | |
| `GL_POINT_BIT` | |
| `GL_POLYGON_BIT` | |
| `GL_POLYGON_STIPPLE_BIT` | |
| `GL_SCISSOR_BIT` | |
| `GL_STENCIL_BUFFER_BIT` | |
| `GL_TEXTURE_BIT` | |
| `GL_TRANSFORM_BIT` | |
| `GL_VIEWPORT_BIT` | |
| `GL_ALL_ATTRIB_BITS` | `GL_CLIENT_ALL_ATTRIB_BITS` |

"Bits set in mask that do not correspond to an attribute group are ignored."

**Errors.** `glPushAttrib`/`glPushClientAttrib` while the stack depth is
`MAX_ATTRIB_STACK_DEPTH`/`MAX_CLIENT_ATTRIB_STACK_DEPTH` ⇒ `STACK_OVERFLOW`;
`glPopAttrib`/`glPopClientAttrib` on an empty stack ⇒ `STACK_UNDERFLOW`. The
depth is implementation dependent but **must be at least 16** (table 6.34).

**Ordering.** "When state for a group is pushed, all state corresponding to
TEXTURE0 is pushed first, followed by state corresponding to TEXTURE1, and so
on up to and including the state corresponding to TEXTUREk … When state for a
group is popped, texture state is restored in the opposite order."

**Matrices are excluded.** "Matrix stacks are never pushed or popped with
PushAttrib, PushClientAttrib, PopAttrib, or PopClientAttrib."

**Debug commands are excluded in the compatibility profile.** In a
compatibility profile the debug commands are additionally *not recorded into
display lists*.

### CURRENT

**STATUS: MISSING.**

*Absence evidence:*

```sh
cd /Volumes/project/TGLMT
grep -rnE "\bgl(PushAttrib|PopAttrib|PushClientAttrib|PopClientAttrib)\b" src/ include/
# → no output (exit status 1)
```

All 4 L17 commands are absent from the exported surface.
`GL_ATTRIB_STACK_DEPTH` (0x0D33) is not an accepted pname.

There is no attribute-stack machinery, and — importantly for the design — the
**state that would have to be saved does not exist yet** in the form the stack
saves it (the matrix stacks, lighting state, fog state and texture
environments are all missing). The attribute stack is therefore a *late* stage:
implementing it before the state it saves would guarantee it saves nothing.

### PLAN

* The attribute stack is implemented as a **snapshot of the legacy state
  block**, one entry per push, holding a copy of the groups selected by the
  mask. Because the legacy state block is the single owner of all
  fixed-function state (see the mapping document), the push/pop is a struct
  copy — cheap and exactly correct.
* The server/client split maps onto the state block: server groups
  (lighting, fog, texture environment, transform, enable, hint, …) live in the
  server part; client groups (vertex arrays, pixel store) live in the client
  part.
* The texture-group ordering rule (§12 "Ordering") is honoured by iterating the
  texture units in ascending order on push and descending on pop.
* Matrix stacks are deliberately excluded from the snapshot, per the spec.
* Depth limit ≥ 16, with `STACK_OVERFLOW` / `STACK_UNDERFLOW` errors.

---

## 13. Group L01 (part 2) — Current vertex/attribute state and the `glBegin` guard

The "current" values (`glColor*`, `glNormal3*`, `glTexCoord*`,
`glMultiTexCoord*`, `glSecondaryColor3*`, `glIndex*`, `glFogCoord*`,
`glEdgeFlag*`) are a distinct piece of state from the vertex arrays and from
the generic vertex attributes. In the compatibility profile they are *client*
state: the value in effect when `glVertex*` is called is copied into the vertex
(glspec21 §2.7).

### CURRENT

**STATUS: MISSING.**

* The generic vertex attribute current values *do* exist —
  `glVertexAttrib{1,2,3,4}{f,d}[v]` and the packed variants store them in the
  shadow (`src/gl/gl_vertex.cpp:202-250`, `src/gl/gl_vertex.cpp:369-388`) — but
  they are the *modern* generic attributes, not the legacy current values, and
  they are not read by any draw path.
* `glMultiTexCoord*` is in L01 and absent.
* `glEdgeFlag`/`glEdgeFlagv` are in L01 and absent; there is no edge-flag bit
  anywhere in the codebase.

### PLAN

* A `LegacyCurrentState` block holding: colour (RGBA), secondary colour
  (RGBA), normal (3), texture coordinates per unit (4), fog coordinate (1),
  colour index (1) and the edge-flag bit.
* The `glBegin`/`glEnd` legality guard (§1) is enforced centrally so that the
  "illegal inside Begin/End ⇒ `INVALID_OPERATION`" rule is applied uniformly
  to all ~700 entry points, including the ones that exist today.

---

## 14. What already exists that the compatibility work can build on

This section records the parts of the modern implementation that the legacy
pipeline should reuse rather than duplicate. It is the reason the compatibility
work is a matter of weeks rather than months.

| Asset | Location | Reuse |
|---|---|---|
| Deferred draw path, encoder reuse, per-draw pipeline key, uniform staging cache, ring allocator | `src/gl/gl_apple_draw.cpp`, `include/tglmt/Context.h:1011-1097`, `:1194-1210` | the legacy draw path plugs into the *same* encoder and pipeline cache; it must add its own key component (§15) |
| CPU primitive expansion for FAN and LINE_LOOP | `src/gl/gl_apple_draw.cpp:1937-2000` | the pattern for QUADS / QUAD_STRIP / POLYGON expansion |
| GLSL→MSL converter with uniform reflection, varyings, `[[user(locn)]]` layout, `TGLMTUniforms` block at `buffer(16)` | `src/gl/glsl_to_msl.cpp:8351` (entry), `:11843` (uniform struct), `:12370` / `:12578` (VS/FS entry points) | the fixed-function shaders are *generated* MSL, but they reuse the same uniform-block convention and the same vertex-descriptor plumbing |
| MSL post-processing hooks (e.g. `InjectSampleMask`) | `src/gl/gl_program.cpp:213-240` | the pattern for injecting the legacy fragment code into a generated shader |
| Clip-space z conversion `z*0.5 + w*0.5` | `src/gl/glsl_to_msl.cpp:10356` (`kZConv`) | the same conversion must be applied by the generated legacy vertex shader |
| Viewport/scissor conversion | `include/tglmt/GLConvert.h:21-56` | the legacy path uses the same viewport state and the same conversion |
| Front-face inversion for the flipped viewport | `src/gl/gl_apple_draw.cpp:2689-2690` | the legacy path inherits the same winding correction |
| Texture binding tables per unit and per target, 32 units | `include/tglmt/StateTracker.h:183-224`, `:287` | the legacy texture-environment state is added *per unit* next to these |
| Sampler state construction from texture parameters | `src/gl/gl_apple_draw.cpp:207-369` (`SamplerForUnit`) | reused verbatim by the legacy fragment stage |
| `glReadPixels` with region clamping and format conversion | `src/gl/gl_readpixels_dsa.cpp:89-…` | reused by the legacy pixel operations |
| `glPixelStorei` state object | `include/tglmt/StateTracker.h:51-58`, `src/gl/gl_texture.cpp:1691-1719` | extended with `SWAP_BYTES`/`LSB_FIRST` for the legacy pixel transfer modes |
| Framebuffer / render target model | `include/tglmt/Context.h:493-532` | the accumulation buffer is a new off-screen target of the same kind |

---

## 15. Summary of the audit

### 15.1 Status per legacy group

| Group | Entry points | STATUS | Evidence |
|---|---:|---|---|
| L01 Immediate mode | 173 | **MISSING** | grep G1 empty; 0/173 exported |
| L02 Client-side vertex arrays | 12 | **MISSING** | grep G8 empty; 0/12 exported (EXT shadow is lossy: `src/gl/gl_ext_dsa_state.cpp:17`) |
| L03 Matrix stack / transform | 20 | **MISSING** | grep G2 empty; 0/20 exported (EXT matrix shadow exists but is a different API: `src/gl/gl_ext_dsa_state.cpp:249-281`) |
| L04 Lighting / materials | 18 | **MISSING** | grep G3 empty; 0/18 exported; `glEnable(GL_LIGHTING)` is accepted and inert |
| L05 Fog | 4 | **MISSING** | grep G3 empty; 0/4 exported; `glEnable(GL_FOG)` accepted and inert |
| L06 Texenv / texgen | 15 | **MISSING** | grep G3 empty; 0/15 exported (EXT shadow loses target/coord: `src/gl/gl_ext_dsa_state.cpp:17`) |
| L07 Display lists | 8 | **MISSING** | grep G5 empty; 0/8 exported; no `DisplayList` object kind |
| L08 Evaluators | 23 | **MISSING** | grep G5 empty; 0/23 exported |
| L09 Selection / feedback | 8 | **MISSING** | grep G5 empty; 0/8 exported |
| L10 Accumulation buffer | 2 | **MISSING** | grep G6 empty; 0/2 exported; no accumulation target in the model |
| L11 Pixel transfer / pixel drawing | 12 | **PARTIAL** | modern `glPixelStorei`/`glReadPixels` exist; grep G6 empty for the legacy commands; 0/12 exported |
| L12 Raster / window position | 40 | **MISSING** | grep G6 empty; 0/40 exported |
| L13 Stipple | 3 | **PARTIAL** | `glLineStipple` exported but is an empty stub (`src/gl/gl_raster.cpp:274`); 2/3 missing |
| L14 Alpha test | 1 | **MISSING** | grep G4 empty; 0/1 exported |
| L15 Texture residency / priority | 2 | **MISSING** | grep G4 empty; 0/2 exported |
| L16 User clip planes | 2 | **MISSING** | grep G4 empty; 0/2 exported |
| L17 Attribute stacks | 4 | **MISSING** | grep G9 empty; 0/4 exported |
| L18 Colour-index mode | 2 | **MISSING** | grep G4 empty; 0/2 exported (no colour-index visuals on iOS — RGBA-only is the correct behaviour) |
| L19 Compat-only vertex types (GL 3.3) | 30 | **PRESENT** | `src/gl/gl_packed_robust.cpp` |
| L20 Compat-only `Getn*` (GL 4.5) | 12 | **PRESENT** | `src/gl/gl_packed_robust.cpp` |

**Totals: 349 legacy fixed-function entry points, 348 absent, 1 exported-but-inert
(`glLineStipple`).** 18 of the 18 legacy groups are entirely missing except for
two partial groups (L11 and L13) and one complete group of *non-legacy*
compatibility-only commands (L19/L20).

### 15.2 The cross-cutting problems, in order of severity

1. **Silent acceptance of legacy enables.** `glEnable`/`glDisable` accept any
   enum (`src/core/StateTracker.cpp:6-20`) and `glIsEnabled` reports them back.
   A legacy application that enables `GL_LIGHTING`, `GL_FOG`, `GL_ALPHA_TEST`
   or `GL_TEXTURE_GEN_S` gets `GL_TRUE` back and nothing else happens. This is
   the worst kind of failure for the target workload: no error, no crash, wrong
   pixels.
2. **No compatibility profile.** `GL_CONTEXT_PROFILE_MASK` is hard-coded to
   `GL_CONTEXT_CORE_PROFILE_BIT` (`src/gl/gl_get.cpp:148`) and `GL_VERSION`
   reports `"4.6 (TGLMT Metal port, build …)"` (`src/gl/gl_error_string_debug.cpp:17`).
   Until a context can *be* a compatibility context, every legacy entry point
   needs a place to live and a rule for when it is legal.
3. **No fixed-function pipeline.** The draw path requires a linked program
   (`src/gl/gl_apple_draw.cpp:1799-1802`); there is no "no program ⇒ fixed
   function" branch, which is the rule the spec states in glspec21 §2.15.
4. **The legacy state does not exist.** Matrix stacks, lighting, fog, texenv,
   texgen, current vertex state, raster position, the attribute stack, display
   lists, evaluators and the selection/feedback machinery all have to be
   created from scratch. They are, however, all *CPU-side* state, which is the
   good news: they are cheap to model and they map onto uniforms.
5. **Exported-but-inert.** `glLineStipple` is the one place where the export
   census over-reports; it must be counted as absent until it does something.

### 15.3 What this audit does **not** claim

* It does not claim any runtime behaviour was measured: no legacy program was
  executed, because none can be — the entry points do not exist.
* It does not claim CTS/Piglit conformance for any legacy feature.
* It does not claim the compatibility profile can be created: no profile
  mechanism exists yet (`project/TODO.md` T-010 / T-036).
* It does not evaluate Metal feasibility. That is the mapping document's job.
* It does not cover GLU (D-015 / T-019) and does not cover the GL 3.2 → 4.6
  compatibility-profile-only behaviour that is *not* fixed-function (L19/L20 are
  already implemented).

---

## 16. Cross-references

| Topic | Where |
|---|---|
| Profile split, per-version command sets, `L01`–`L20` grouping | `docs/OPENGL_PROFILE_MATRIX.md` |
| Measured export status of the 1048-command compat surface | `docs/OPENGL_API_COVERAGE.md` |
| Version × profile command counts (generated) | `docs/OPENGL_VERSION_MATRIX.md`, `data/version_inventory.json` |
| Existing 4.6-Core → Metal feasibility table | `docs/OPENGL_46_METAL_MAPPING.md` |
| A11 hardware limits | `docs/METAL_A11_LIMITATIONS.md`, `docs/apple/Metal-Feature-Set-Tables.pdf` |
| Mapping strategy per mechanism | `docs/LEGACY_TO_METAL_MAPPING.md` |
| Staged implementation order and test IDs | `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` |
| Project vocabulary and rules | `project/docs/GLOSSARY.md`, `project/AGENTS.md` §6 |
| Open questions this audit raises | `project/TODO.md` T-010, T-019, T-020, T-021, T-022, T-024 … T-038 |
