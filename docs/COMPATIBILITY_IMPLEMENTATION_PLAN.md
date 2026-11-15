# Compatibility Implementation Plan — OpenGL 1.0–2.1 legacy pipeline

> Derived from [`docs/LEGACY_PIPELINE_AUDIT.md`](LEGACY_PIPELINE_AUDIT.md) (the
> spec-level audit) and
> [`docs/LEGACY_TO_METAL_MAPPING.md`](LEGACY_TO_METAL_MAPPING.md) (the
> Metal mapping strategy).
>
> **Nothing in this document is implemented.** Every stage below is a plan.
> Stage 0 is the only stage that touches code paths the existing 185 tests
> exercise; every later stage is additive.
>
> Ordering rule: **state model → matrix stack → immediate mode → lighting →
> fog → texenv → display lists → evaluators → selection/feedback → pixel
> ops**, with the rasterisation and attribute-stack work folded in where it
> depends on the state it reads.

---

## 0. How to read this plan

### 0.1 Stage anatomy

Each stage specifies:

| Field | Meaning |
|---|---|
| **API entry points** | the exact `gl*` names to add, with the count |
| **Files** | files to create and files to modify, with the reason |
| **New context state** | the state the stage introduces and where it lives |
| **Tests** | the test IDs to write, and where they live |
| **Exit criteria** | the observable conditions that must hold for the stage to be called done |
| **Depends on** | the stages that must land first, and what breaks if they do not |

### 0.2 Exit criteria vocabulary

Per `project/AGENTS.md` §6, a stage is done when **all four** hold:

1. the code path is real (`file:line` exists);
2. a test exists and **passes**;
3. the expected value is checked against the specification or a reference
   implementation;
4. the environment label is recorded (`A11_DEVICE` / `SIMULATOR` /
   `OTHER_GPU` / `REFERENCE_ONLY` / `NONE`).

`SKIP` and `NOT_RUN` never count as a pass. A GPU pixel test that skips on the
null backend is `NOT_VERIFIED`, not done.

### 0.3 Test conventions

* Test files live in `tests/unit/` (no GPU needed) or `tests/integration/`
  (real GPU, pixel-verified) and are picked up automatically by the
  `file(GLOB …)` in `CMakeLists.txt:107-109`; the CTest name is the file
  stem.
* `TGLMT_CHECK` / `TGLMT_RESULT` from `tests/test_util.h` — never `assert()`
  (D-010: `assert()` is compiled out in Release and the test silently passes).
* A test must assert something real: a pixel value, a matrix element, an
  error code, a query result. "Does not crash" is not a pass.
* GPU pixel tests follow the pattern of the existing ones, e.g.
  `tests/integration/test_blend_apple.cpp`: render, `glReadPixels`, compare.
* Legacy-specific test names are prefixed `test_legacy_` so they can be run as
  a group:
  ```sh
  (cd build-work && ctest -R 'test_legacy_' --output-on-failure)
  ```

### 0.4 The inventory obligation

Every stage must update the machine-readable inventory. The schema for that is
`project/TODO.md` T-002 (`kind, deprecated, removed_in_core, in_compat, glsl_dep,
status, env_tag, mapping, notes`), and the generator is
`tools/gen_version_inventory.py`. The coverage table that consumes it is
`docs/OPENGL_API_COVERAGE.md`. Until T-002 lands, the *minimum* is for each
new entry point to appear in `tests/truth/api_manifest.json` with a `tests`
reference — `tools/truth_manifest.py --write` produces that file and
`tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict` is the gate.

---

## 1. Stage overview and dependency graph

| # | Stage | Entry points | Depends on | Parallel with |
|---|---|---:|---|---|
| S0 | Context version/profile + state skeleton | 0 new (6 modified) | — | — |
| S1 | Matrix stack + transform state | 20 | S0 | — |
| S2 | Legacy current-vertex state + `glBegin`/`glEnd` guard | 0 new (2 modified) | S0 | S1 |
| S3 | Immediate mode: vertex specification | 165 | S2 | — |
| S4 | Lighting, materials, shading model | 18 | S1, S3 | — |
| S5 | Fog | 4 | S4 | — |
| S6 | Texture environment + texture generation | 15 | S5 | — |
| S7 | Legacy rasterisation (alpha test, stipple, clip planes, texture residency, colour index) | 9 | S4 | — |
| S8 | Raster position + window position | 40 | S1, S3 | S4, S5, S6, S7 |
| S9 | Pixel transfer modes + pixel drawing | 12 | S8 | S7 |
| S10 | Display lists | 8 | S3, S8 | S9 |
| S11 | Evaluators | 23 | S3 | S9, S10 |
| S12 | Selection + feedback | 8 | S8, S10 | S11 |
| S13 | Accumulation buffer | 2 | S9 | S12 |
| S14 | Client-side vertex arrays | 12 | S3 | S13 |
| S15 | Attribute stacks | 4 | S4, S5, S6, S14 | — |
| S16 | GLU (tessellator, quadric, `gluBuild2DMipmaps`) | TBD (T-019) | S3 | — |

**Total new entry points: 348** (the measured missing count from the audit
§0.1) + the 4 that need fixing (`glLineStipple` is exported but inert; see
S7).

### 1.1 Dependency graph

```
                    S0 context/profile + state skeleton
                     │
        ┌────────────┼────────────┐
        ▼            ▼            ▼
  S1 matrix     S2 current     (independent:
    stack        vertex state    S7 needs S4)
        │            │
        │            ▼
        │      S3 immediate mode
        │            │
        ├────────────┼──────────────┐
        ▼            ▼              ▼
  S4 lighting   S8 raster pos   S11 evaluators
        │            │
        ▼            ▼
  S5 fog       S9 pixel ops ──────┐
        │                          │
        ▼                          ▼
  S6 texenv/texgen            S10 display lists
                                   │
                                   ▼
                              S12 selection/feedback
                                   │
  S14 client arrays ──► S15 attribute stacks
                                   │
                                   ▼
                              S13 accumulation buffer
```

### 1.2 What can be parallelised

* **S0 is the only hard serialisation point.** Everything else can start once
  the state skeleton exists.
* **S1 (matrix stack) and S2 (current vertex state) are independent of each
  other** and can be done in parallel by two agents: they touch disjoint parts
  of the legacy state block.
* **S8 (raster position) only needs S1 and S3** — it is independent of
  lighting/fog/texenv, so it can proceed in parallel with S4–S7.
* **S7 (rasterisation) needs S4** only because the alpha test lives in the
  generated fragment shader; the *state* half (clip planes, stipple patterns,
  texture priority) can be built earlier and in parallel.
* **S9 (pixel ops) depends on S8**, not on lighting. **S10 (display lists)
  depends on S3 and S8** (a recorded list can contain a raster-position
  command).
* **S11 (evaluators) depends only on S3** — it produces vertices.
* **S14 (client arrays) depends only on S3** — it produces vertices.
* **S12 (selection/feedback) depends on S8 and S10**: it needs the raster
  position semantics and it must interoperate with the display-list replay
  (a list can contain `glRenderMode`).
* **S13 (accumulation) depends on S9** (it reads and writes the framebuffer).
* **S15 (attribute stacks) is last** — it snapshots the state that S4/S5/S6/
  S14 create. Building it earlier would guarantee it saves nothing.

**Practical parallelism recommendation for three agents:**

* Agent A: S0 → S1 → S4 → S5 → S6 → S15
* Agent B: S0 → S2 → S3 → S11 → S14
* Agent C: S0 → S8 → S9 → S10 → S12 → S13 → S7

with S7 split so that the *state* half lands with Agent A (which owns the
legacy state block) and the *shader* half lands with Agent C.

---

## 2. S0 — Context version/profile and the legacy state skeleton

**Entry points:** 0 new. 6 existing entry points change behaviour:
`glGetIntegerv` (`GL_CONTEXT_PROFILE_MASK`, `GL_MAJOR_VERSION`,
`GL_MINOR_VERSION`), `glGetString` (`GL_VERSION`), `glGetStringi`
(`GL_SHADING_LANGUAGE_VERSION`), `glEnable`/`glDisable` (reject legacy caps
in a core context).

### 2.1 Files

| Action | File | Reason |
|---|---|---|
| create | `include/tglmt/LegacyState.h` | the fixed-function state block |
| create | `src/core/LegacyState.cpp` | its initialisers and helpers |
| create | `src/gl/gl_legacy_program.cpp` | the generated-MSL program builder + cache |
| create | `include/tglmt/LegacyProgram.h` | the `LegacyProgramKey` and the program wrapper |
| modify | `include/tglmt/Context.h` | add `LegacyState legacy;`, `GLenum profile`, `int profileVersion`, and the legacy program cache |
| modify | `src/core/Context.cpp` | initialise the new members |
| modify | `src/gl/gl_get.cpp` | report the profile mask / version from the context |
| modify | `src/gl/gl_error_string_debug.cpp` | report the version string from the context |
| modify | `src/gl/gl_raster.cpp` | `glEnable`/`glDisable` reject a legacy cap in a core context |
| modify | `include/tglmt/ObjectRegistry.h` | (deferred to S10) add the `DisplayList` object kind |

### 2.2 New context state

```cpp
// include/tglmt/Context.h
    // --- profile / version (T-010) ---
    enum class GLProfile : uint8_t { Core, Compatibility };
    GLProfile profile = GLProfile::Core;
    uint8_t glMajor = 4, glMinor = 6;

    // --- legacy fixed-function state (all fixed-function state lives here,
    //     NOT in StateTracker, so the core path cannot be affected) ---
    LegacyState legacy;

    // --- the generated legacy program + its cache ---
    struct LegacyProgramCacheEntry {
        LegacyProgramKey key;
        std::shared_ptr<metal::ILibrary> vs, fs;
        uint64_t linkGen = 0;
        bool valid = false;
    };
    static constexpr size_t kLegacyProgramCacheN = 4;
    LegacyProgramCacheEntry legacyProgramCache[kLegacyProgramCacheN];
```

`LegacyState` (in `include/tglmt/LegacyState.h`) starts **empty**: S1 fills in
the matrix stacks, S2 the current-vertex state, S4 the lighting, S5 the fog,
S6 the texenv/texgen, S7 the rasterisation state, S8 the raster position, S9
the pixel transfer state, S10 the display lists, S11 the evaluators, S12 the
selection/feedback, S13 the accumulation buffer, S14 the client arrays, S15
the attribute stacks. Every field is added by the stage that owns it.

### 2.3 Behavioural requirements

1. **Profile selection.** The context constructor takes an optional profile.
   `Context::Context(backend, profile, major, minor)`. The default stays
   `Core` / 4.6 so **every existing test keeps its current behaviour**.
2. **`GL_CONTEXT_PROFILE_MASK`** returns `GL_CONTEXT_COMPATIBILITY_PROFILE_BIT`
   (0x2) in a compatibility context and `GL_CONTEXT_CORE_PROFILE_BIT` (0x1) in
   a core context. Today it is hard-coded to core
   (`src/gl/gl_get.cpp:148`).
3. **`GL_VERSION`** reports `"4.6 (TGLMT Metal port, build …)"` for core and
   `"4.6 compatibility (TGLMT Metal port, build …)"` for compatibility. The
   core string must stay **byte-identical** to today's
   (`src/gl/gl_error_string_debug.cpp:17`) so that no existing test breaks.
4. **`glGetStringi(GL_SHADING_LANGUAGE_VERSION, i)`** reports
   `"460 compatibility"` in a compatibility context, `"460 core"` in core.
5. **Removed-function rule.** In a **core** context, a legacy entry point must
   raise `GL_INVALID_OPERATION` (glspec46 App. E: "Functions which have been
   removed will generate an INVALID_OPERATION error if called in the core
   profile"). In a **compatibility** context they execute.
   This requires a **central guard**, because there are ~350 of them: the
   guard lives in the generated C-export shim (`src/gl/gl_c_exports.cpp`,
   generated by `tools/gen_c_exports.py`) or, better, in a thin dispatch layer
   that the generator emits.
   > **Risk.** The export shim is generated. Modifying the generator is
   > mandatory (`project/AGENTS.md` §5: "File generated → sửa generator, không
   > sửa tay"). The generator change must keep the 1860 currently-exported
   > symbols exported and must not change the timing instrumentation.
6. **Legacy enables must not lie.** `glEnable(GL_LIGHTING)` in a *core*
   context must raise `GL_INVALID_OPERATION` (it is a removed feature). In a
   *compatibility* context it must succeed **and do something** — which is
   only true once S4 lands. Therefore: **the compatibility context must not be
   creatable until S4 is done**, or the enable must be rejected until then.
   The simplest correct rule: the compatibility profile is gated per-feature —
   a legacy enable is accepted only if the corresponding legacy state is
   implemented. This is the "no silent inert enable" rule from the audit
   §15.2.
   > Until S4/S5/S6 land, the compatibility context rejects
   > `GL_LIGHTING`, `GL_FOG`, `GL_ALPHA_TEST`, `GL_TEXTURE_GEN_*`,
   > `GL_CLIP_PLANE*`, `GL_LIGHT*`, `GL_COLOR_MATERIAL`, `GL_RESCALE_NORMAL`,
   > `GL_NORMALIZE` with `GL_INVALID_OPERATION`.

### 2.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_profile` | `tests/unit/test_legacy_profile.cpp` | a core context reports `GL_CONTEXT_CORE_PROFILE_BIT` and the current version string; a compatibility context reports `GL_CONTEXT_COMPATIBILITY_PROFILE_BIT`, the compatibility version string, and `"460 compatibility"` from `glGetStringi` |
| `test_legacy_profile_removed_error` | `tests/unit/test_legacy_profile_removed_error.cpp` | in a **core** context, `glEnable(GL_LIGHTING)` raises `GL_INVALID_OPERATION`; in a compatibility context it does not raise |
| `test_legacy_profile_no_regression` | `tests/unit/test_legacy_profile_no_regression.cpp` | a core context's `GL_VERSION` string is byte-identical to the pre-change string; `glGetIntegerv(GL_CONTEXT_PROFILE_MASK)` is still `0x1` |

### 2.5 Exit criteria

* A compatibility context can be created and reports the correct profile mask,
  version string and shading-language string.
* Every legacy entry point raises `GL_INVALID_OPERATION` in a core context.
* No legacy enable is accepted-and-inert anywhere.
* **All 185 existing tests still pass** — this is the gate that proves the
  core path is untouched:
  ```sh
  (cd build-work && ctest -j4 --output-on-failure)     # 185/185
  python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
  python3 tools/ext_matrix.py --lib build-work/libtglmt.dylib --strict
  ```
* `tools/gen_gl_headers.py --check` and `tools/gen_c_exports.py` still pass.

### 2.6 Depends on

Nothing. **This stage gates everything else.**

---

## 3. S1 — Matrix stack and fixed-function transform

**Entry points: 20** (all of L03).

`glFrustum`, `glLoadIdentity`, `glLoadMatrixd`, `glLoadMatrixf`,
`glLoadTransposeMatrixd`, `glLoadTransposeMatrixf`, `glMatrixMode`,
`glMultMatrixd`, `glMultMatrixf`, `glMultTransposeMatrixd`,
`glMultTransposeMatrixf`, `glOrtho`, `glPopMatrix`, `glPushMatrix`,
`glRotated`, `glRotatef`, `glScaled`, `glScalef`, `glTranslated`,
`glTranslatef`.

### 3.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_matrix.cpp` | the 20 entry points |
| create | `include/tglmt/LegacyMatrix.h` | `MatrixStack` and the 4×4 helpers (column-major, `double` precision) |
| modify | `include/tglmt/LegacyState.h` | add the four stacks + the current mode |
| modify | `src/gl/gl_get.cpp` | `GL_MODELVIEW_MATRIX` (0x0BA6), `GL_PROJECTION_MATRIX` (0x0BA7), `GL_TEXTURE_MATRIX` (0x0BA8), `GL_MATRIX_MODE` (0x0BA0), `GL_MODELVIEW_STACK_DEPTH` (0x0BA3), `GL_PROJECTION_STACK_DEPTH` (0x0BA4), `GL_TEXTURE_STACK_DEPTH` (0x0BA5) |
| modify | `tools/gen_state_pnames.py` | add the 7 new pnames (the file is generated) |
| modify | `src/gl/gl_apple_draw.cpp` | upload the legacy matrices as uniforms in the legacy draw path |

### 3.2 New context state

```cpp
struct LegacyMatrixStack {
    static constexpr size_t kDepth = 32;   // ≥ 32 for MODELVIEW; ≥ 2 elsewhere
    std::vector<std::array<GLdouble,16>> stack;   // column-major
    size_t depth() const { return stack.size(); }
};
struct LegacyState {
    LegacyMatrixStack modelview, projection, color;
    std::vector<LegacyMatrixStack> texture;        // one per texture unit
    GLenum  matrixMode = GL_MODELVIEW;
    // cached for the shader upload
    std::array<GLdouble,16> modelviewFlat{}, projectionFlat{};
    std::array<GLdouble,9>  normalMatrix{};        // inverse-transpose of MV 3x3
};
```

The per-unit texture stacks mean `texture` must be sized lazily to the number
of texture units in use (32 max, `kMaxTextureUnits` in
`include/tglmt/StateTracker.h:17`).

### 3.3 Semantics to implement (from the audit §2)

* `glMatrixMode` accepts `GL_MODELVIEW`, `GL_PROJECTION`, `GL_TEXTURE`,
  `GL_COLOR`; anything else ⇒ `GL_INVALID_ENUM`.
* `glPushMatrix` past the depth ⇒ `GL_STACK_OVERFLOW` (0x0503);
  `glPopMatrix` on an empty stack ⇒ `GL_STACK_UNDERFLOW` (0x0504).
* `glMultMatrix` computes `C' = C · M` (post-multiply) — the argument is
  **post**-multiplied.
* `glLoadTransposeMatrix` / `glMultTransposeMatrix` take row-major data.
* `glTranslate`/`glRotate`/`glScale`/`glFrustum`/`glOrtho` each build a matrix
  and post-multiply it.
* `glRotate` rotates **counter-clockwise** about the axis when the axis points
  up, with the angle in **degrees**.
* `GL_TEXTURE` mode targets the **active** texture unit's stack.
* The model-view matrix's inverse-transpose is maintained for the normal
  transform.

### 3.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_matrix_stack` | `tests/unit/test_legacy_matrix_stack.cpp` | `glPushMatrix`/`glPopMatrix` restore the previous matrix; `glLoadIdentity` resets; the stack depth limit is honoured and the overflow/underflow errors are the right code |
| `test_legacy_matrix_order` | `tests/unit/test_legacy_matrix_order.cpp` | `glMultMatrix` **post**-multiplies: `C' = C·M`, verified element-by-element against a hand-computed matrix; `glTranslate` after `glScale` gives a different result from the reverse order |
| `test_legacy_matrix_transpose` | `tests/unit/test_legacy_matrix_transpose.cpp` | `glLoadTransposeMatrix` with row-major data equals `glLoadMatrix` with the transposed data; same for the `Mult` variants |
| `test_legacy_matrix_modes` | `tests/unit/test_legacy_matrix_modes.cpp` | each of the four modes has its own stack; `GL_TEXTURE` follows `glActiveTexture`; `glMatrixMode` with a bad enum ⇒ `GL_INVALID_ENUM` |
| `test_legacy_matrix_query` | `tests/unit/test_legacy_matrix_query.cpp` | `glGetFloatv(GL_MODELVIEW_MATRIX)` returns the stack top; the 3 matrix pnames and the 3 stack-depth pnames are accepted (no `GL_INVALID_ENUM`) |
| `test_legacy_matrix_texture_stack` | `tests/unit/test_legacy_matrix_texture_stack.cpp` | two texture units have independent texture matrix stacks |

### 3.5 Exit criteria

* All 20 entry points implemented with the spec's error behaviour.
* The 7 new pnames are accepted and return the correct values.
* The upload path sets the legacy uniform block's matrices and the normal
  matrix.
* `test_legacy_matrix_*` all pass on the null backend (`env_tag = NONE`).
* The 185 existing tests still pass.

### 3.6 Depends on

S0 (the state block and the profile guard). Independent of S2–S16.

---

## 4. S2 — Legacy current-vertex state and the `glBegin`/`glEnd` guard

**Entry points: 0 new.** 2 existing entry points change:
`glEnable`/`glDisable` (the legacy-cap list) and the central dispatcher (the
Begin/End legality guard).

### 4.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_state.cpp` | the current-vertex block + the Begin/End guard |
| modify | `include/tglmt/LegacyState.h` | add the current-vertex fields |
| modify | `src/gl/gl_c_exports.cpp` | **via `tools/gen_c_exports.py`** — the guard hook |
| modify | `tools/gen_c_exports.py` | emit the guard call in the wrapper |

### 4.2 New context state

```cpp
struct LegacyCurrent {
    float4 color{1,1,1,1};
    float4 secondaryColor{0,0,0,1};
    float3 normal{0,0,1};
    float4 texCoord[kMaxTextureUnits]{};
    float  fogCoord{0.0f};
    GLuint index{0};
    bool   edgeFlag{true};
};
struct LegacyState {
    LegacyCurrent cur;
    // Begin/End assembly
    bool     inBeginEnd{false};
    GLenum   beginMode{0};
    size_t   beginVertexBase{0};
};
```

### 4.3 The Begin/End guard — the critical design decision

The spec (glspec21 §2.6.3) says:

> "Executing any other GL command between the execution of Begin and the
> corresponding execution of End results in the error INVALID_OPERATION."

That list of "any other GL command" covers ~700 entry points, so the check
**cannot** be implemented per entry point. It must be centralised.

**Options considered:**

1. *A flag checked in each entry point.* — Rejected: 700 places to remember,
   and one omission is a silent correctness hole.
2. *A check in the generated C-export shim.* — **Selected.** The shim
   (`src/gl/gl_c_exports.cpp`) is the single funnel through which every
   public `gl*` call passes. The generator (`tools/gen_c_exports.py`) emits a
   guard call at the top of each wrapper. The cost is one branch per GL call
   in the hot path — measurable but small, and it can be made
   near-zero-cost by having the guard be a single predictable branch on a
   `bool` that is false almost always.

   The guard needs to know **which** commands are allowed inside Begin/End:
   the vertex-specification family (`glVertex*`, `glColor*`, `glNormal3*`,
   `glTexCoord*`, `glMultiTexCoord*`, `glSecondaryColor3*`, `glIndex*`,
   `glFogCoord*`, `glEdgeFlag*`), `glVertexAttrib*`, `glArrayElement`,
   `glEvalCoord*`, `glEvalPoint*`, `glMaterial*`, `glCallList`, `glCallLists`
   and `glEnd`. The generator therefore needs the allowed-set; the natural
   source of truth is `docs/khronos/gl.xml` (the registry knows which commands
   exist in which version, but not which are Begin/End-legal — that is a
   spec-text fact). The set is therefore **hand-written in the generator**,
   with a comment citing glspec21 §2.6.3.

   > **Alternative for the generator:** the guard could be driven by a table
   > in `LegacyState.h` (`IsBeginEndLegal(const char* fn)`) with a
   > `std::unordered_set<std::string_view>` of the allowed names. The
   > generator then emits `if (c.legacy.inBeginEnd && !c.legacy.isLegal("glVertex3f")) { c.errors.Record(0x0502); return; }`.
   > A string comparison per call is too slow for the hot path, so the set
   > should be a **bitmask indexed by the dispatch table index**
   > (`GetCoreFunctionName(i)` already provides a stable index,
   > `include/tglmt/gl46.h`). The generator emits the *index* as a constant,
   > and `LegacyState` holds a `std::bitset<1024>` of legal indices.
   > This is fast (one bit test) and exact.

3. *A check in the C++ implementations only (not the C shim).* — Rejected:
   the C++ layer is not the public API surface (`extern "C"` is), and the
   two would drift.

### 4.4 Semantics to implement

* `glBegin` while already in Begin/End ⇒ `GL_INVALID_OPERATION`.
* `glEnd` without a matching `glBegin` ⇒ `GL_INVALID_OPERATION`.
* Any command outside the allowed set, while in Begin/End ⇒
  `GL_INVALID_OPERATION`.
* The soft-rule commands (`glEnableClientState`, `glColorPointer`,
  `glPixelStore`, …) "may or may not" generate an error — TGLMT **may**
  generate one; choosing to generate `GL_INVALID_OPERATION` is conformant and
  simpler. The choice must be documented.

### 4.5 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_begin_guard` | `tests/unit/test_legacy_begin_guard.cpp` | `glBegin` inside `glBegin` ⇒ `INVALID_OPERATION`; `glEnd` without `glBegin` ⇒ `INVALID_OPERATION`; a legal command inside Begin/End (`glVertex3f`, `glColor3f`, `glMaterialfv`, `glEvalCoord1f`, `glCallList`) does **not** raise; an illegal one (`glEnable`, `glBindTexture`, `glDrawArrays`, `glViewport`, `glClear`) does raise |
| `test_legacy_current_state` | `tests/unit/test_legacy_current_state.cpp` | each current-value setter updates the block; `glColorMaterial` resolution is deferred to S4 |

### 4.6 Exit criteria

* The guard is enforced for **every** exported entry point, verified by a test
  that iterates the dispatch table and calls a representative sample of
  commands inside a Begin/End block, asserting that exactly the allowed set
  survives.
* The 185 existing tests still pass — the guard must not fire when no Begin/End
  is active (the hot path).

### 4.7 Depends on

S0. Independent of S1.

---

## 5. S3 — Immediate mode: vertex specification and primitive assembly

**Entry points: 165** of the 173 in L01. The remaining 8 (`glRect*`) are in
S8, because a rectangle is a raster-position-relative operation.

`glBegin`, `glEnd`, `glVertex{2,3,4}{d,f,i,s}[v]` (24), `glColor{3,4}{u?[bdfis]}v?`
(32), `glNormal3{u?[bdfis]}v?` (10), `glTexCoord{1,2,3,4}{u?[dfis]}v?` (32),
`glMultiTexCoord{1,2,3,4}{u?[dfis]}v?` (32), `glSecondaryColor3{u?[bdfis]}v?`
(16), `glIndex{u?[bdfis]}v?` (10), `glFogCoord{1,2}{u?[dfi]}v?` (4),
`glEdgeFlag`, `glEdgeFlagv`, `glArrayElement`.

### 5.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_immediate.cpp` | the 165 entry points + the assembler |
| create | `include/tglmt/LegacyVertex.h` | the assembled-vertex struct and the primitive batch |
| modify | `include/tglmt/LegacyState.h` | add the assembler and the pending-batch state |
| modify | `src/gl/gl_apple_draw.cpp` | the legacy draw path: the branch that selects the legacy program, and the batched draw submission |
| modify | `include/tglmt/Context.h` | the legacy program cache (§2.2) and the pending-batch list |

### 5.2 New context state

```cpp
struct LegacyVertex {
    float4 position;
    float4 color;
    float4 secondaryColor;
    float3 normal;
    float4 texCoord[kMaxTextureUnits];   // per-unit, resolved at vertex time
    float  fogCoord;
    uint   edgeFlag;
};
struct LegacyBatch {
    metal::PrimitiveType type;           // Point / Line / LineStrip / Triangle / TriangleStrip
    uint32_t first, count;               // range in the staging buffer
};
struct LegacyState {
    // assembler
    std::vector<LegacyVertex> vertices;  // the whole frame's assembled vertices
    std::vector<LegacyBatch>  batches;   // pending, not yet submitted
    // staging
    size_t stagingVertexBase = 0;
};
```

The staging buffer is TGLMT's existing ring allocator
(`Context::RingAlloc`, `src/core/Context.cpp:539`), so no new allocation path
is introduced. The per-frame reset happens in `Context::NextFrame`
(`src/core/Context.cpp:562`).

### 5.3 Semantics to implement

**Assembly.** `glVertex*` appends a fully-resolved vertex: position, current
colour, current normal, current texcoords, current fog coordinate, current
edge flag, current colour index.

**Primitive assembly at `glEnd`** (glspec21 §2.6.1):

| mode | rule |
|---|---|
| `GL_POINTS` | one point per vertex |
| `GL_LINES` | vertex pairs; an odd trailing vertex is dropped |
| `GL_LINE_STRIP` | n−1 segments; <2 vertices ⇒ no primitive |
| `GL_LINE_LOOP` | as strip + a closing segment (v_last → v_first) |
| `GL_TRIANGLES` | vertex triples; trailing `k` (0…2) dropped |
| `GL_TRIANGLE_STRIP` | overlapping triangles (A, B, current) |
| `GL_TRIANGLE_FAN` | triangles (v0, vi, vi+1) |
| `GL_QUADS` | groups of 4; trailing `k` (0…3) dropped |
| `GL_QUAD_STRIP` | quad *i* = (v2i, v2i+1, v2i+3, **v2i+2**) — the spec's order; <4 vertices ⇒ no primitive; odd count ⇒ last dropped |
| `GL_POLYGON` | one polygon; <3 vertices ⇒ no primitive; fan-triangulated from v0 |

**Metal expansion.** `GL_TRIANGLE_FAN` → triangle list; `GL_LINE_LOOP` →
closed line strip; `GL_QUADS`/`GL_QUAD_STRIP`/`GL_POLYGON` → triangle list.
The CPU expansions for fan and line-loop already exist in the modern path
(`src/gl/gl_apple_draw.cpp:1937-2000`) and are the template; the three
quad/polygon expansions are new.

**Batching.** Consecutive primitives of the same Metal primitive type, with
unchanged pipeline and legacy state, are submitted as **one**
`drawPrimitives`. The batch is broken by:
* a primitive-type change;
* a pipeline-state change (blend, depth, cull, front face, viewport, scissor,
  depth bias, colour mask, …);
* a legacy-state change that affects shading (matrices, lights, material,
  fog, texenv, texgen, shade model);
* a framebuffer/target change;
* a framebuffer-reading command (`glReadPixels`, `glCopyPixels`, `glAccum`
  with `RETURN`);
* entering/leaving selection or feedback mode;
* `glFlush` / `glFinish` / present.

**Legality.** The S2 guard covers the "illegal inside Begin/End" rule.

**Edge flags.** `glEdgeFlag`/`glEdgeFlagv` set the current bit; it is copied
onto each vertex. Its *effect* (boundary vs non-boundary edge) matters only for
polygon rasterisation, which is a no-op difference on Metal (there are no
boundary edges in a triangle rasteriser) — except for polygon stipple, where
the spec's boundary-edge rule affects the pattern reset. The edge flag is
therefore carried but has no rasterisation effect in the first iteration; this
is a documented divergence.

### 5.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_immediate_assembly` | `tests/unit/test_legacy_immediate_assembly.cpp` | the assembler produces the right primitive count and the right vertex ranges for every mode, including the drop rules (odd `GL_LINES` tail, `k` tails for triangles/quads, `GL_LINE_STRIP` with 1 vertex, `GL_POLYGON` with 2 vertices) |
| `test_legacy_immediate_quadstrip_order` | `tests/unit/test_legacy_immediate_quadstrip_order.cpp` | the `GL_QUAD_STRIP` triangle order is (v2i, v2i+1, v2i+3) + (v2i, v2i+3, v2i+2) — verified against the spec's ordering, **not** the naive one |
| `test_legacy_immediate_current` | `tests/unit/test_legacy_immediate_current.cpp` | the current colour/normal/texcoord are captured per vertex, not per primitive |
| `test_legacy_immediate_errors` | `tests/unit/test_legacy_immediate_errors.cpp` | `glBegin`/`glEnd` nesting errors, `glBegin` with a bad mode ⇒ `GL_INVALID_ENUM` |
| `test_legacy_immediate_gpu` | `tests/integration/test_legacy_immediate_gpu.cpp` | on a real GPU: a triangle, a quad, a quad strip and a polygon drawn in immediate mode produce the expected pixels; the quad strip's winding is correct (both halves visible, not one culled) |
| `test_legacy_immediate_batching` | `tests/integration/test_legacy_immediate_batching.cpp` | 1000 quads in one state produce **one** `drawPrimitives` (counted via a new `Context::AppleStats` counter) and the correct pixels |

### 5.5 Exit criteria

* All 165 entry points implemented.
* The 10 primitive modes assemble correctly, verified against a hand-written
  reference for the vertex ranges.
* One `drawPrimitives` per batch, verified by a counter.
* The GPU test renders the expected pixels for all 10 modes.
* The 185 existing tests still pass.

### 5.6 Depends on

S2 (the guard) and S0. The matrix stack (S1) is needed for the *rendering* to
be correct, but the assembly logic can be written against S2 alone; the GPU
pixel test needs S1.

---

## 6. S4 — Fixed-function lighting, materials and shading model

**Entry points: 18** (all of L04).

`glColorMaterial`, `glGetLightfv`, `glGetLightiv`, `glGetMaterialfv`,
`glGetMaterialiv`, `glLightModelf`, `glLightModelfv`, `glLightModeli`,
`glLightModeliv`, `glLightf`, `glLightfv`, `glLighti`, `glLightiv`,
`glMaterialf`, `glMaterialfv`, `glMateriali`, `glMaterialiv`, `glShadeModel`.

### 6.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_lighting.cpp` | the 18 entry points |
| create | `src/gl/gl_legacy_shader_gen.cpp` | the MSL generator for the legacy program (lighting stage) |
| modify | `include/tglmt/LegacyState.h` | the light/material/light-model state |
| modify | `src/gl/gl_apple_draw.cpp` | the legacy program's vertex stage uses the lighting uniforms |
| modify | `src/gl/gl_get.cpp` | the lighting pnames (`GL_LIGHT_MODEL_AMBIENT` 0x0B53, `GL_LIGHT_MODEL_LOCAL_VIEWER` 0x0B51, `GL_LIGHT_MODEL_TWO_SIDE` 0x0B52, `GL_LIGHT_MODEL_COLOR_CONTROL` 0x81F8, `GL_SHADE_MODEL` 0x0B54, `GL_COLOR_MATERIAL` 0x0B57, `GL_COLOR_MATERIAL_FACE` 0x0B58, `GL_COLOR_MATERIAL_PARAMETER` 0x0B59, `GL_MATERIAL_AMBIENT` 0x0B00, `GL_MATERIAL_DIFFUSE` 0x0B01, `GL_MATERIAL_SPECULAR` 0x0B02, `GL_MATERIAL_EMISSION` 0x0B03, `GL_MATERIAL_SHININESS` 0x0B26) |
| modify | `tools/gen_state_pnames.py` | add the new pnames |

### 6.2 New context state

```cpp
struct LegacyLight {
    float4 ambient{0,0,0,1}, diffuse{1,1,1,1}, specular{1,1,1,1};
    float4 position{0,0,1,0};
    float3 spotDirection{0,0,-1};
    float  spotExponent{0}, spotCutoff{180};
    float  k0{1}, k1{0}, k2{0};
    bool   enabled{false};
};
struct LegacyMaterial {
    float4 ambient{0.2f,0.2f,0.2f,1}, diffuse{0.8f,0.8f,0.8f,1};
    float4 specular{0,0,0,1}, emission{0,0,0,1};
    float  shininess{0};
};
struct LegacyState {
    LegacyLight  lights[8];                 // ≥ 8 required (glspec21 2.14.1)
    LegacyMaterial matFront, matBack;
    float4 sceneAmbient{0.2f,0.2f,0.2f,1};
    bool  localViewer{false}, twoSided{false}, separateSpecular{false};
    GLenum shadeModel{GL_SMOOTH};
    bool  lightingEnabled{false};
    bool  colorMaterialEnabled{false};
    GLenum colorMaterialFace{GL_FRONT_AND_BACK};
    GLenum colorMaterialParam{GL_AMBIENT_AND_DIFFUSE};
    uint8_t numLightsEnabled{0};
};
```

### 6.3 Semantics to implement

* **Defaults** exactly as glspec21 table 2.10 (light 0 diffuse/specular
  `(1,1,1,1)`, lights *i*>0 `(0,0,0,1)`, position `(0,0,1,0)`, spot
  direction `(0,0,−1)`, spot cutoff 180°, attenuation `k0=1,k1=0,k2=0`;
  material ambient `(0.2,…)`, diffuse `(0.8,…)`, specular/emission `(0,…)`,
  shininess 0; scene ambient `(0.2,…)`).
* **Light position** is transformed by the model-view matrix **at the time
  `glLight` is called**; the **spot direction** by the upper-left 3×3 only.
  Both are therefore CPU-side operations at `glLight` time.
* **`glLight`/`glMaterial`/`glLightModel` validation**: `INVALID_ENUM` for an
  unknown pname/light/face; `INVALID_VALUE` for an out-of-range value
  (shininess outside `[0,128]`, spot exponent outside `[0,128]`, spot cutoff
  outside `[0,90] ∪ {180}`, negative attenuation).
* **`glMaterial` face** is `FRONT`, `BACK` or `FRONT_AND_BACK`.
* **`glLightModel`** accepts `AMBIENT`, `LOCAL_VIEWER`, `TWO_SIDE`,
  `COLOR_CONTROL` (`SINGLE_COLOR` / `SEPARATE_SPECULAR_COLOR`).
* **`glColorMaterial`** attaches material parameters to the current colour.
* **`glShadeModel`** accepts `GL_SMOOTH` (initial) and `GL_FLAT`.

**Generated shader.** The lighting is evaluated **per vertex** in the legacy
vertex shader, per glspec21 §2.14.1. The MSL is given in the mapping
document §5.2.

**Two-sided lighting** is the one deliberate divergence: because the facing is
a per-fragment property and the lighting is per-vertex, the two-sided case
evaluates lighting per fragment. This must be:
* documented in the compatibility caveats;
* flagged in the generated program (`twoSided` in the program key) so that
  the two paths are visibly different in the shader.

**Flat shading** requires the CPU to resolve the flat-source vertex per
primitive (mapping §1.5, §5.3).

### 6.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_lighting_state` | `tests/unit/test_legacy_lighting_state.cpp` | the light/material/light-model setters and getters round-trip; the defaults match glspec21 table 2.10; the error codes are right |
| `test_legacy_lighting_position_transform` | `tests/unit/test_legacy_lighting_position_transform.cpp` | the light position is transformed by the model-view matrix **at set time**: setting a light, then translating, then querying/using the light gives the *set-time* position |
| `test_legacy_lighting_spot` | `tests/unit/test_legacy_lighting_spot.cpp` | the spot cone maths (cutoff angle, exponent) matches a CPU reference |
| `test_legacy_lighting_attenuation` | `tests/unit/test_legacy_lighting_attenuation.cpp` | `1/(k0+k1·d+k2·d²)` matches a CPU reference |
| `test_legacy_lighting_gpu` | `tests/integration/test_legacy_lighting_gpu.cpp` | on a real GPU: a lit quad with one directional light, one point light and one spotlight produces pixels matching a CPU reference implementation of glspec21 §2.14.1 within a tolerance; two-sided lighting picks the front/back material by facing |
| `test_legacy_lighting_flat` | `tests/integration/test_legacy_lighting_flat.cpp` | `glShadeModel(GL_FLAT)` gives one colour per primitive, using the **spec's** source vertex for a triangle strip (vertex *i*+2) and for a quad strip (vertex 2*i*+2) |
| `test_legacy_lighting_colormaterial` | `tests/integration/test_legacy_lighting_colormaterial.cpp` | `glColorMaterial(GL_DIFFUSE, GL_FRONT, GL_AMBIENT_AND_DIFFUSE)` makes the diffuse material track the current colour |

### 6.5 Exit criteria

* All 18 entry points implemented with the spec's defaults and errors.
* The GPU test matches a CPU reference of the spec's lighting equation.
* The flat-shading source-vertex rule is verified against glspec21 table 2.12.
* The two-sided-lighting divergence is documented and appears in the
  compatibility caveats.
* The 185 existing tests still pass.

### 6.6 Depends on

S1 (the model-view matrix, for the position/spot-direction transform) and S3
(the assembler, for the flat-shading source vertex).

---

## 7. S5 — Fog

**Entry points: 4** (all of L05): `glFogf`, `glFogfv`, `glFogi`, `glFogiv`.

### 7.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_fog.cpp` | the 4 entry points |
| modify | `include/tglmt/LegacyState.h` | the fog state |
| modify | `src/gl/gl_legacy_shader_gen.cpp` | the fog stage in the generated fragment shader |
| modify | `src/gl/gl_get.cpp` | the fog pnames (`GL_FOG_COLOR` 0x0B66, `GL_FOG_DENSITY` 0x0B62, `GL_FOG_END` 0x0B64, `GL_FOG_MODE` 0x0B65, `GL_FOG_START` 0x0B63, `GL_FOG_COORD_SRC` 0x0B65-conflict → 0x8450) |
| modify | `tools/gen_state_pnames.py` | add the new pnames |

### 7.2 New context state

```cpp
struct LegacyState {
    GLenum fogMode{GL_EXP};        // GL_LINEAR / GL_EXP / GL_EXP2
    float  fogDensity{1.0f};
    float  fogStart{0.0f}, fogEnd{1.0f};
    float4 fogColor{0,0,0,0};
    GLenum fogCoordSrc{GL_FRAGMENT_DEPTH};
    bool   fogEnabled{false};
    GLenum fogHint{GL_FOG_HINT};   // the hint value is stored
};
```

### 7.3 Semantics to implement

* The three equations (glspec21 eq. 3.31–3.33) exactly, with `f` clamped to
  `[0,1]`.
* Fog is applied to the **post-texturing** colour, before the per-fragment
  operations; **alpha is unchanged**.
* `GL_FOG_COORD_SRC` selects `GL_FRAGMENT_DEPTH` (default) or `GL_FOG_COORD`.
* `d < 0` ⇒ `GL_INVALID_VALUE`.
* `GL_FOG_HINT` is stored. The hint selects between the exact exponential and
  a cheaper approximation in the shader — it is a uniform branch, not a
  program-shape change.
* `GL_LINEAR` with `start == end` is **undefined** in GL; TGLMT must pick a
  deterministic rule (hard step) and document it.

### 7.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_fog_state` | `tests/unit/test_legacy_fog_state.cpp` | the setters/getters round-trip; the defaults match the spec; `d < 0` ⇒ `INVALID_VALUE`; a bad mode ⇒ `INVALID_ENUM` |
| `test_legacy_fog_formula` | `tests/unit/test_legacy_fog_formula.cpp` | the three equations match a CPU reference for a range of inputs, including the `[0,1]` clamp |
| `test_legacy_fog_gpu` | `tests/integration/test_legacy_fog_gpu.cpp` | on a real GPU: a fogged gradient matches a CPU reference within a tolerance, for all three modes; alpha is unchanged by fog |

### 7.5 Exit criteria

* The 4 entry points implemented; the GPU output matches the CPU reference.
* The `GL_LINEAR` degenerate case is documented.

### 7.6 Depends on

S4 (the fog lives in the same generated fragment shader).

---

## 8. S6 — Texture environment and texture generation

**Entry points: 15** (all of L06): `glTexEnvf`, `glTexEnvfv`, `glTexEnvi`,
`glTexEnviv`, `glGetTexEnvfv`, `glGetTexEnviv`, `glTexGend`, `glTexGendv`,
`glTexGenf`, `glTexGenfv`, `glTexGeni`, `glTexGeniv`, `glGetTexGendv`,
`glGetTexGenfv`, `glGetTexGeniv`.

### 8.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_texenv.cpp` | the 15 entry points |
| modify | `include/tglmt/LegacyState.h` | the per-unit texenv and texgen state |
| modify | `src/gl/gl_legacy_shader_gen.cpp` | the texenv chain and the texgen code |
| modify | `src/gl/gl_get.cpp` | the texenv/texgen pnames (`GL_TEXTURE_ENV_MODE` 0x2200, `GL_TEXTURE_ENV_COLOR` 0x2201, `GL_COMBINE_RGB` 0x8571, `GL_COMBINE_ALPHA` 0x8572, `GL_RGB_SCALE` 0x8573, `GL_ALPHA_SCALE` 0x8574, `GL_SRC0_RGB` 0x8580 … `GL_OPERAND2_ALPHA` 0x8598, `GL_TEXTURE_GEN_MODE` 0x2500, `GL_OBJECT_PLANE` 0x2501, `GL_EYE_PLANE` 0x2502) |
| modify | `tools/gen_state_pnames.py` | add the new pnames |

### 8.2 New context state

```cpp
struct LegacyTexEnv {
    GLenum mode{GL_MODULATE};          // REPLACE/MODULATE/DECAL/BLEND/ADD/COMBINE
    float4 color{0,0,0,0};
    // GL_COMBINE arguments
    GLenum combineRGB{GL_MODULATE}, combineAlpha{GL_MODULATE};
    GLenum srcRGB[3]{GL_TEXTURE,GL_PREVIOUS,GL_CONSTANT};
    GLenum srcAlpha[3]{GL_TEXTURE,GL_PREVIOUS,GL_CONSTANT};
    GLenum operandRGB[3]{GL_SRC_COLOR,GL_SRC_COLOR,GL_SRC_ALPHA};
    GLenum operandAlpha[3]{GL_SRC_ALPHA,GL_SRC_ALPHA,GL_SRC_ALPHA};
    float  rgbScale{1.0f}, alphaScale{1.0f};
    float  lodBias{0.0f};              // GL_TEXTURE_FILTER_CONTROL
};
struct LegacyTexGen {
    GLenum mode{GL_EYE_LINEAR};        // OBJECT_LINEAR / EYE_LINEAR / SPHERE_MAP /
                                       // REFLECTION_MAP / NORMAL_MAP
    float4 objectPlane{1,0,0,0}, eyePlane{1,0,0,0};
    bool   enabled{false};             // per coordinate
};
struct LegacyState {
    LegacyTexEnv texEnv[kMaxTextureUnits];
    LegacyTexGen texGen[kMaxTextureUnits][4];   // S, T, R, Q
};
```

### 8.3 Semantics to implement

* `glTexEnv` target is `GL_TEXTURE_ENV`, `GL_TEXTURE_FILTER_CONTROL` (only
  `TEXTURE_LOD_BIAS`) or `GL_POINT_SPRITE` (only
  `POINT_SPRITE_COORD_ORIGIN`). Other target ⇒ `INVALID_ENUM`.
* The five texture functions (glspec21 tables 3.21/3.22) exactly, plus
  `GL_COMBINE` (glspec21 table 3.23) with its three sources, six operands,
  two combine functions and the two scale factors.
* `TEXTURE_ENV_COLOR` components outside `[0,1]` are clamped.
* The initial state (glspec21 §3.8.13): mode and combiner functions `MODULATE`;
  sources `TEXTURE`, `PREVIOUS`, `CONSTANT`; RGB operands for sources 0/1
  `SRC_COLOR`, source 2 and all alpha operands `SRC_ALPHA`; environment colour
  `(0,0,0,0)`.
* `glTexGen` coord is `S`/`T`/`R`/`Q`; `SPHERE_MAP` and `REFLECTION_MAP` with
  coord `R`/`Q` ⇒ `INVALID_ENUM` (per the mode), `NORMAL_MAP` with `Q` ⇒
  `INVALID_ENUM`.
* The generation defaults: all disabled; `EYE_LINEAR` with `p1 = 1` for s and
  `p2 = 1` for t, everything else zero.
* `OBJECT_LINEAR` and `EYE_LINEAR` are evaluated on the CPU at vertex time;
  `SPHERE_MAP`, `REFLECTION_MAP` and `NORMAL_MAP` in the generated vertex
  shader.

### 8.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_texenv_state` | `tests/unit/test_legacy_texenv_state.cpp` | the setters/getters round-trip per unit; the defaults match the spec; a bad target/pname ⇒ `INVALID_ENUM`; the env colour is clamped |
| `test_legacy_texenv_modes` | `tests/integration/test_legacy_texenv_modes.cpp` | on a real GPU: `GL_REPLACE`, `GL_MODULATE`, `GL_DECAL`, `GL_BLEND`, `GL_ADD` each produce the spec's result for an RGBA texture; the alpha behaviour of `DECAL` (alpha unchanged) and `ADD` (alpha added) is checked |
| `test_legacy_texenv_combine` | `tests/integration/test_legacy_texenv_combine.cpp` | `GL_COMBINE` with the full argument set: `SRC0/1/2_RGB`, the operands, `COMBINE_RGB`/`COMBINE_ALPHA`, `RGB_SCALE`/`ALPHA_SCALE` — checked against a CPU reference for a representative matrix of combinations, including `ADD_SIGNED`, `INTERPOLATE`, `SUBTRACT` and `DOT3_RGB`/`DOT3_RGBA` |
| `test_legacy_texgen` | `tests/integration/test_legacy_texgen.cpp` | `GL_OBJECT_LINEAR`, `GL_EYE_LINEAR`, `GL_SPHERE_MAP`, `GL_REFLECTION_MAP`, `GL_NORMAL_MAP` each produce the spec's coordinates for a known camera and geometry, compared against a CPU reference |
| `test_legacy_texenv_multitexture` | `tests/integration/test_legacy_texenv_multitexture.cpp` | two units chain correctly (unit 1 sees unit 0's result as `PREVIOUS`) |

### 8.5 Exit criteria

* The 15 entry points implemented; all six environment modes and all five
  generation modes verified against a CPU reference on the GPU.
* `GL_COMBINE`'s full argument matrix is covered.

### 8.6 Depends on

S5 (the texenv runs before the fog in the fragment stage).

---

## 9. S7 — Legacy rasterisation: alpha test, stipple, clip planes, texture residency, colour index

**Entry points: 9 new** (2 of the 3 L13 commands are new; `glLineStipple` exists
but is inert and must be reimplemented).

`glAlphaFunc` (L14), `glClipPlane`, `glGetClipPlane` (L16),
`glPolygonStipple`, `glGetPolygonStipple` (L13), `glLineStipple` (reimplement —
currently an empty stub at `src/gl/gl_raster.cpp:274`),
`glAreTexturesResident`, `glPrioritizeTextures` (L15), `glClearIndex`,
`glIndexMask` (L18).

### 9.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_raster.cpp` | the 9 new entry points + the `glLineStipple` reimplementation |
| modify | `include/tglmt/LegacyState.h` | the alpha test, clip planes, stipple patterns, texture priorities, colour-index state |
| modify | `src/gl/gl_legacy_shader_gen.cpp` | the alpha test and polygon-stipple fragment code |
| modify | `src/gl/gl_get.cpp` | the pnames (`GL_ALPHA_TEST_FUNC` 0x0BC1, `GL_ALPHA_TEST_REF` 0x0BC0, `GL_CLIP_PLANE0..5` 0x3000–0x3005, `GL_POLYGON_STIPPLE_PATTERN` 0x0A13) |
| modify | `tools/gen_state_pnames.py` | add the new pnames |
| modify | `src/gl/gl_raster.cpp` | replace the `glLineStipple` stub |

### 9.2 New context state

```cpp
struct LegacyState {
    GLenum  alphaFunc{GL_ALWAYS};
    float   alphaRef{0.0f};
    bool    alphaTestEnabled{false};
    float   clipPlanes[6][4]{};        // A,B,C,D in eye coordinates
    bool    clipPlaneEnabled[6]{};
    uint8_t polygonStipple[32]{};      // 32 rows x 8 bits
    bool    polygonStippleEnabled{false};
    uint16_t lineStipplePattern{0xffff};
    GLuint  lineStippleFactor{1};
    bool    lineStippleEnabled{false};
    std::vector<float> texturePriorities;
    GLuint  colorIndex{0};
    GLbitfield indexMask{0xffffffff};
};
```

### 9.3 Semantics to implement

* **Alpha test** in the generated fragment shader as a `discard_fragment()`
  with the eight comparison functions; `ref` clamped to `[0,1]` and the
  fragment alpha rounded to the nearest integer before the comparison.
* **Polygon stipple** as a 32×32 mask tested against the window coordinate
  (the row index must be flipped because `gl_FragCoord` is upper-left origin
  while GL's window origin is lower-left).
* **Line stipple** needs a CPU expansion of the line into a strip with a
  per-vertex line distance, plus a fragment-stage pattern test.
* **User clip planes**: the CPU clipper (shared with the selection path, see
  S12) clips the assembled polygon against each enabled plane in eye space.
  Plane 0 may additionally use the native `[[clip_distance]]` — the plan
  starts with the CPU clipper for all six (exact), and the native path is an
  optimisation to be measured.
* **`glAreTexturesResident`** answers from the texture objects: a texture is
  resident when it has a realised GPU handle.
* **`glPrioritizeTextures`** stores the priorities, which feed the legacy
  LOD-bias path of the texture environment.
* **`glClearIndex` / `glIndexMask`** store the state. On iOS/CAMetalLayer there
  are no colour-index visuals, so the state has no effect — the same reason
  the core profile removed colour-index mode (glspec46 App. E.2). This is
  `NOT_APPLICABLE` behaviour with the commands present, and it must be
  documented.

### 9.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_alpha_test` | `tests/integration/test_legacy_alpha_test.cpp` | on a real GPU: all eight `glAlphaFunc` comparisons behave as the spec's table, with `ref` clamping and the alpha rounding |
| `test_legacy_polygon_stipple` | `tests/integration/test_legacy_polygon_stipple.cpp` | a known 32×32 pattern produces exactly the expected masked pixels; the row flip is verified (a pattern that is not vertically symmetric catches the error) |
| `test_legacy_line_stipple` | `tests/integration/test_legacy_line_stipple.cpp` | the pattern repeats along the line, the `factor` alternation is right, and the pattern resets at each segment |
| `test_legacy_clip_plane` | `tests/integration/test_legacy_clip_plane.cpp` | a polygon straddling a clip plane is cut exactly on the plane, with the correct depth interpolation on the surviving part; a fully-outside polygon produces no fragments |
| `test_legacy_tex_priority` | `tests/unit/test_legacy_tex_priority.cpp` | `glPrioritizeTextures` stores the priorities and `glAreTexturesResident` reports the residency of the bound textures |
| `test_legacy_colorindex` | `tests/unit/test_legacy_colorindex.cpp` | `glClearIndex`/`glIndexMask` store the state and are queryable; the commands do not error |

### 9.5 Exit criteria

* All 9 entry points implemented; `glLineStipple` is no longer an empty stub.
* The GPU pixel tests match the spec for the alpha test and the stipples.
* The clip-plane test verifies the exact cut line and the depth interpolation.
* The two-sided-lighting / flat-shading / wide-line / logic-op divergences are
  listed in the compatibility caveats.

### 9.6 Depends on

S4 (the alpha test lives in the same generated fragment shader). The *state*
half can land earlier and in parallel.

---

## 10. S8 — Raster position and window position

**Entry points: 40** (all of L12) + the 8 `glRect*` commands (which are part of
L01 but depend on the raster position).

`glRasterPos{2,3,4}{d,f,i,s}[v]` (20), `glWindowPos{2,3,4}{d,f,i,s}[v]` (20),
`glRect{d,f,i,s}[v]` (8).

### 10.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_rasterpos.cpp` | the 40 entry points |
| modify | `src/gl/gl_legacy_immediate.cpp` | the 8 `glRect*` entry points |
| modify | `include/tglmt/LegacyState.h` | the raster position state |
| modify | `src/gl/gl_get.cpp` | the pnames (`GL_CURRENT_RASTER_POSITION` 0x0B01, `GL_CURRENT_RASTER_TEXTURE_COORDS` 0x0B02, `GL_CURRENT_RASTER_COLOR` 0x0B04, `GL_CURRENT_RASTER_DEPTH` 0x0B05) |
| modify | `tools/gen_state_pnames.py` | add the new pnames |

### 10.2 New context state

```cpp
struct LegacyState {
    float4 rasterPos{0,0,0,1};
    bool   rasterPosValid{false};
    float4 rasterColor{0,0,0,1};
    float4 rasterTexCoord{0,0,0,1};
    float  rasterDepth{0.0f};
};
```

### 10.3 Semantics to implement

* `glRasterPos*` transforms the coordinate exactly as a vertex (model-view,
  projection, clip, viewport), captures the associated colour/texcoord/depth,
  and sets the valid flag. If the point is clipped, the position becomes
  indeterminate and the valid flag is cleared (glspec21 §2.13).
* `glWindowPos*` sets the position directly in window coordinates, bypassing
  the transform.
* `glRect*` is equivalent to a Begin/End pair with a rectangle primitive
  (glspec21 §2.10), using the current texture coordinates and colour.

### 10.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_rasterpos` | `tests/unit/test_legacy_rasterpos.cpp` | the raster position equals the vertex transform of the same coordinate; the valid flag clears when the point is outside the clip volume; `glWindowPos` bypasses the transform |
| `test_legacy_rect` | `tests/integration/test_legacy_rect.cpp` | on a real GPU: `glRectf` draws a filled rectangle at the current raster position with the current colour and texture coordinates |

### 10.5 Exit criteria

* The 40 raster/window-position commands and the 8 rectangle commands
  implemented and queryable.

### 10.6 Depends on

S1 (the matrices) and S3 (the assembler, for the rectangle).

---

## 11. S9 — Pixel transfer modes and pixel drawing

**Entry points: 12** (all of L11).

`glBitmap`, `glCopyPixels`, `glDrawPixels`, `glGetPixelMapfv`,
`glGetPixelMapuiv`, `glGetPixelMapusv`, `glPixelMapfv`, `glPixelMapuiv`,
`glPixelMapusv`, `glPixelTransferf`, `glPixelTransferi`, `glPixelZoom`.

### 11.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_pixelops.cpp` | the 12 entry points |
| modify | `include/tglmt/LegacyState.h` | the pixel transfer and pixel map state |
| modify | `src/gl/gl_texture.cpp` | `glPixelStorei` must accept `GL_SWAP_BYTES` / `GL_LSB_FIRST` (currently `INVALID_ENUM`, `src/gl/gl_texture.cpp:1711-1717`) — and `glGetIntegerv` already reports them as 0 (`src/gl/gl_get.cpp:373-374`), which is an inconsistency this stage removes |
| modify | `src/gl/gl_get.cpp` | the pixel-map pnames |

### 11.2 New context state

```cpp
struct LegacyPixelTransfer {
    float redScale{1}, greenScale{1}, blueScale{1}, alphaScale{1};
    float redBias{0}, greenBias{0}, blueBias{0}, alphaBias{0};
    float indexShift{0}, indexOffset{0};
    bool  mapColor{false}, mapStencil{false};
    std::array<std::array<float,256>,4> maps;   // R,G,B,A maps
    std::array<std::array<float,256>,2> indexMaps;
    std::array<std::array<float,256>,2> depthMaps;
};
struct LegacyState {
    LegacyPixelTransfer pixelTransfer;
    float  pixelZoomX{1}, pixelZoomY{1};
};
```

### 11.3 Semantics to implement

* `glPixelTransfer*` sets the 12 transfer parameters; the scale/bias apply to
  the pack and unpack paths.
* `glPixelMap*` installs up to `MAX_PIXEL_MAP_TABLE` (≥ 32) entries per target;
  the colour matrix target is a 4×4 matrix applied as
  `dst = Σ Mᵢⱼ · srcⱼ`.
* `glDrawPixels` writes a rectangle of pixels at the raster position, applying
  the transfer operations and the pixel maps.
* `glBitmap` writes a 1-bit mask at the raster position.
* `glCopyPixels` copies from the read buffer to the write buffer at the raster
  position, with format conversion and the transfer operations.
* `glPixelZoom` scales the written rectangle.

### 11.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_pixeltransfer` | `tests/unit/test_legacy_pixeltransfer.cpp` | the transfer parameters round-trip; `glPixelMap*` round-trips; the maps are applied on the pack path (verified through `glReadPixels` of a known image) |
| `test_legacy_drawpixels` | `tests/integration/test_legacy_drawpixels.cpp` | on a real GPU: `glDrawPixels` writes the expected pixels at the raster position, in `GL_RGB`/`GL_RGBA`/`GL_BGR`/`GL_BGRA` and with `GL_UNSIGNED_BYTE`/`GL_FLOAT` |
| `test_legacy_copypixels` | `tests/integration/test_legacy_copypixels.cpp` | `glCopyPixels` copies a rectangle with the correct format conversion |
| `test_legacy_bitmap` | `tests/integration/test_legacy_bitmap.cpp` | `glBitmap` writes the expected 1-bit mask |
| `test_legacy_pixelstore_swap` | `tests/unit/test_legacy_pixelstore_swap.cpp` | `glPixelStorei(GL_UNPACK_SWAP_BYTES, 1)` is accepted (no `INVALID_ENUM`) and round-trips through `glGetIntegerv` |

### 11.5 Exit criteria

* The 12 entry points implemented.
* The `glPixelStorei` inconsistency (accepted-on-read, rejected-on-write) is
  removed.
* The pixel tests match a CPU reference.

### 11.6 Depends on

S8 (the raster position).

---

## 12. S10 — Display lists

**Entry points: 8** (all of L07).

`glCallList`, `glCallLists`, `glDeleteLists`, `glEndList`, `glGenLists`,
`glIsList`, `glListBase`, `glNewList`.

### 12.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_displaylist.cpp` | the 8 entry points |
| create | `include/tglmt/LegacyDisplayList.h` | the list object and the recorded-command stream |
| modify | `include/tglmt/ObjectRegistry.h` | add the `DisplayList` object kind |
| modify | `include/tglmt/LegacyState.h` | the list store and the capture state |
| modify | `src/gl/gl_c_exports.cpp` | **via `tools/gen_c_exports.py`** — the capture/replay hook |
| modify | `tools/gen_c_exports.py` | emit the capture/replay dispatch |

### 12.2 New context state

```cpp
struct LegacyDisplayList {
    GLuint  name;
    std::vector<LegacyRecordedCommand> commands;   // the command stream
    bool    valid{false};
};
struct LegacyState {
    std::map<GLuint, LegacyDisplayList> lists;
    GLuint  listBase{0};
    GLuint  captureList{0};         // 0 = not capturing
    bool    captureExecute{false};  // GL_COMPILE_AND_EXECUTE
    std::vector<LegacyRecordedCommand> captureBuffer;
    uint32_t nestingDepth{0};
    static constexpr uint32_t kMaxNesting = 64;   // spec: ≥ 64
};
```

### 12.3 Semantics to implement

This is the highest-risk stage. The rules (glspec21 §5.4, listed in the audit
§8) are:

* **Recording.** While a list is open, commands are recorded instead of
  executed — **except** for the long list of commands the spec says are
  executed immediately (display-list management, render modes, the vertex-array
  and client-state commands, `PixelStore`, `ReadPixels`, texture/query/buffer/
  program/shader object management, `Finish`, `Flush`, and every `Get*`/`Is*`
  query).
* **Client state at compile time, server state at execution time.** A recorded
  command must carry the client values captured when it was issued.
* **Pointer arguments are dereferenced at issue time.**
* **Buffer-object data is dereferenced at compile time.**
* **`GL_COMPILE` vs `GL_COMPILE_AND_EXECUTE`.**
* **`glNewList(0)` ⇒ `INVALID_VALUE`; `glEndList` without `glNewList` ⇒
  `INVALID_OPERATION`; `glNewList` twice ⇒ `INVALID_OPERATION`; `glCallList(0)`
  ⇒ `INVALID_VALUE`.**
* **`glGenLists`** returns a *contiguous* block and creates an empty list for
  each; 0 if no such block exists or `range = 0`.
* **`glIsList`**, **`glDeleteLists(list, 0)`** does nothing.
* **`glCallLists`** offset encoding by `type` plus `glListBase`.
* **Nesting limit ≥ 64.**
* **`glEndList` failure ⇒ `OUT_OF_MEMORY`** with the previous list contents
  and all other state unchanged — the recording must be transactional
  (build into a temporary, commit on `glEndList`).
* **Under `GL_COMPILE_AND_EXECUTE`, a nested `glCallList` executes the nested
  list but records the *call*.**

**Implementation approach.** The recording must intercept the whole command
stream, so the hook is in the same place as the S2 Begin/End guard: the
generated C-export shim. The generator emits, for each entry point, a call to
a dispatcher that either executes or records.

The dispatch table already has a stable index per command
(`GetCoreFunctionName(i)`, `include/tglmt/gl46.h`), so the recording can be a
switch on the index.

**The staged approach** (from the mapping document §9.2):

* **S10a (minimum viable):** record the immediate-mode block, the
  state-changing commands and the draw commands. Everything else executes
  immediately. This covers the common legacy pattern.
* **S10b (complete):** implement the full "execute immediately" list.

Both stages must be tested; S10b is the one that makes the stage "done".

### 12.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_displaylist_basic` | `tests/unit/test_legacy_displaylist_basic.cpp` | `glNewList`/`glEndList`/`glCallList`/`glIsList`/`glDeleteLists`/`glGenLists` behave as the spec says; the error cases raise the right codes; `glGenLists` returns a contiguous block |
| `test_legacy_displaylist_compile` | `tests/unit/test_legacy_displaylist_compile.cpp` | `GL_COMPILE` does not execute; `GL_COMPILE_AND_EXECUTE` does; the "execute immediately" list is honoured (e.g. `glGetIntegerv` inside a list returns the live value and is not recorded) |
| `test_legacy_displaylist_state` | `tests/integration/test_legacy_displaylist_state.cpp` | **the key semantic test**: a list that contains `glTranslate` is replayed with the matrix current at *call* time, not at compile time — the GPU pixels differ when the matrix changes between compile and call |
| `test_legacy_displaylist_nesting` | `tests/unit/test_legacy_displaylist_nesting.cpp` | nested lists replay in order; the nesting limit is ≥ 64 and raises the right error |
| `test_legacy_displaylist_gpu` | `tests/integration/test_legacy_displaylist_gpu.cpp` | on a real GPU: a list built once and called every frame with a changing matrix produces the correct moving image |

### 12.5 Exit criteria

* All 8 entry points implemented with the spec's errors.
* The "server state at execution time" semantic is proven by the GPU test.
* The "execute immediately" list is honoured in full (S10b).
* The nesting limit and the transactional `glEndList` are implemented.

### 12.6 Depends on

S3 (the recorded commands include the immediate-mode block) and S8 (a list can
contain a raster-position command).

---

## 13. S11 — Evaluators

**Entry points: 23** (all of L08).

`glEvalCoord1d`, `glEvalCoord1dv`, `glEvalCoord1f`, `glEvalCoord1fv`,
`glEvalCoord2d`, `glEvalCoord2dv`, `glEvalCoord2f`, `glEvalCoord2fv`,
`glEvalMesh1`, `glEvalMesh2`, `glEvalPoint1`, `glEvalPoint2`, `glGetMapdv`,
`glGetMapfv`, `glGetMapiv`, `glMap1d`, `glMap1f`, `glMap2d`, `glMap2f`,
`glMapGrid1d`, `glMapGrid1f`, `glMapGrid2d`, `glMapGrid2f`.

### 13.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_evaluator.cpp` | the 23 entry points |
| create | `include/tglmt/LegacyEvaluator.h` | the map state and the Bernstein evaluation |
| modify | `include/tglmt/LegacyState.h` | the evaluator state |
| modify | `src/gl/gl_get.cpp` | the evaluator pnames (`GL_MAX_EVAL_ORDER` 0x0D30) |

### 13.2 New context state

```cpp
struct LegacyMap1 {
    GLenum target{0};
    double u1{0}, u2{1};
    int    order{1}, stride{0};
    std::vector<double> points;
    bool   valid{false};
};
struct LegacyMap2 {
    GLenum target{0};
    double u1{0}, u2{1}, v1{0}, v2{1};
    int    uorder{1}, vorder{1}, ustride{0}, vstride{0};
    std::vector<double> points;
    bool   valid{false};
};
struct LegacyState {
    LegacyMap1 map1;
    LegacyMap2 map2;
    double grid1{0}, grid2{1}, grid3{0}, grid4{1};   // glMapGrid1/2
    int    grid2n{1};                                    // grid subdivision counts
    static constexpr int kMaxEvalOrder = 8;             // spec minimum
};
```

### 13.3 Semantics to implement

* `glMap1`: `order < 1` or `order > MAX_EVAL_ORDER` ⇒ `INVALID_VALUE`;
  `stride < k` (the component count of the target) ⇒ `INVALID_VALUE`;
  `u1 == u2` ⇒ `INVALID_VALUE`.
* `glMap2`: the same checks on `uorder`/`vorder`; `u1 == u2` or `v1 == v2` ⇒
  `INVALID_VALUE`.
* Evaluation: `p′(u′) = p((u′−u1)/(u2−u1))` with `p` the Bernstein
  polynomial of degree `order−1`; the 2-D form is the double sum.
* `glEvalCoord1/2` produce a vertex that goes through the **same** legacy
  assembler as `glVertex*`.
* `glEvalPoint1/2` produce a single point.
* `glEvalMesh1` produces `n1` points or `n2` line segments over `[u1,u2]`;
  `glEvalMesh2` produces an `n1`×`n2` grid of points or an `n3`×`n4` grid of
  line segments.
* `glMapGrid1/2` define the uniform sampling grid.

### 13.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_evaluator_map` | `tests/unit/test_legacy_evaluator_map.cpp` | `glMap1`/`glMap2` store the control points and the domain; `glGetMapdv/fv/iv` return them; the error cases raise `INVALID_VALUE` |
| `test_legacy_evaluator_eval` | `tests/unit/test_legacy_evaluator_eval.cpp` | the Bernstein evaluation matches a CPU reference for `order` 1..4; the domain remapping `(u′−u1)/(u2−u1)` is applied; the 2-D form matches |
| `test_legacy_evaluator_mesh` | `tests/unit/test_legacy_evaluator_mesh.cpp` | `glEvalMesh1`/`glEvalMesh2` produce the right number of vertices and the right values |
| `test_legacy_evaluator_gpu` | `tests/integration/test_legacy_evaluator_gpu.cpp` | on a real GPU: a surface evaluated with `glEvalMesh2` renders the expected pixels; the normal map produces the expected shading |

### 13.5 Exit criteria

* The 23 entry points implemented; the evaluation matches a CPU reference.
* The generated vertices flow through the normal pipeline (transform,
  lighting, rasterisation) — proven by the GPU test.

### 13.6 Depends on

S3 (the assembler). Independent of the lighting and texenv stages.

---

## 14. S12 — Selection and feedback

**Entry points: 8** (all of L09).

`glFeedbackBuffer`, `glInitNames`, `glLoadName`, `glPassThrough`, `glPopName`,
`glPushName`, `glRenderMode`, `glSelectBuffer`.

### 14.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_selection.cpp` | the 8 entry points |
| modify | `include/tglmt/LegacyState.h` | the name stack, the select/feedback buffers, the render mode |
| modify | `src/gl/gl_legacy_immediate.cpp` | the assembly hook that produces the hit records / feedback blocks |

### 14.2 New context state

```cpp
struct LegacyState {
    GLenum renderMode{GL_RENDER};           // GL_RENDER / GL_SELECT / GL_FEEDBACK
    // selection
    GLuint* selectBuffer{nullptr}; GLsizei selectBufferSize{0};
    GLuint  nameStack[64]{};  size_t nameStackDepth{0};
    bool    hitSinceLastRecord{false};
    bool    selectOverflow{false};
    // feedback
    GLfloat* feedbackBuffer{nullptr}; GLsizei feedbackBufferSize{0};
    GLenum  feedbackType{GL_2D};
    bool    feedbackOverflow{false};
};
```

The name stack depth is at least 64 (glspec21 table 6.33).

### 14.3 Semantics to implement

* `glRenderMode(GL_SELECT)` before `glSelectBuffer` ⇒ `INVALID_OPERATION`;
  `glSelectBuffer` while in selection mode ⇒ `INVALID_OPERATION`. The same
  pair for feedback.
* `glPushName` on a full stack ⇒ `STACK_OVERFLOW`; `glPopName` on an empty
  stack ⇒ `STACK_UNDERFLOW`; `glLoadName` on an empty stack ⇒
  `INVALID_OPERATION`.
* In selection/feedback mode **no framebuffer updates happen** — the legacy
  draw path suppresses the rasterisation entirely.
* **Selection**: a hit occurs when a point/line/polygon (or a valid
  `glRasterPos`) intersects the clip volume; a culled polygon does not hit;
  `glPolygonMode` does not affect the test; `glWindowPos` always hits. The hit
  record is (name-stack depth, min z, max z, the name stack bottom-first),
  with the depths multiplied by `2³²−1` and rounded.
* **Feedback**: each primitive that would have been rasterised produces a
  block (primitive token + the vertex values per glspec21 table 5.2), with
  window coordinates and no depth-offset arithmetic. Feedback occurs after
  culling and after the polygon-mode interpretation.
* `glPassThrough` inserts a marker token and may not occur between Begin and
  End.
* `glRenderMode` returns the number of records/values copied, or −1 on
  overflow, and clears the name stack.

### 14.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_selection_basic` | `tests/unit/test_legacy_selection_basic.cpp` | the errors are right; the name stack's overflow/underflow codes are right; `glRenderMode` returns the record count |
| `test_legacy_selection_hit` | `tests/integration/test_legacy_selection_hit.cpp` | on a real GPU: a quad under the cursor produces one hit record with the right name stack and a z range that matches a CPU computation of the quad's window-space z extent; a quad outside the clip volume produces no hit; a back-facing (culled) quad produces no hit |
| `test_legacy_feedback` | `tests/integration/test_legacy_feedback.cpp` | the feedback buffer contains the right token and the right per-vertex value count for `GL_2D`/`GL_3D`/`GL_3D_COLOR`/`GL_3D_COLOR_TEXTURE`/`GL_4D_COLOR_TEXTURE`; the coordinates are window coordinates |
| `test_legacy_feedback_passthrough` | `tests/unit/test_legacy_feedback_passthrough.cpp` | `glPassThrough` inserts the token and raises an error inside Begin/End |

### 14.5 Exit criteria

* The 8 entry points implemented.
* The hit record's z range matches a CPU computation.
* The feedback block layout matches the spec's grammar.

### 14.6 Depends on

S8 (the raster-position semantics) and S10 (a list can contain
`glRenderMode`).

---

## 15. S13 — Accumulation buffer

**Entry points: 2** (all of L10): `glAccum`, `glClearAccum`.

### 15.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_accum.cpp` | the 2 entry points |
| modify | `include/tglmt/LegacyState.h` | the accumulation state |
| modify | `src/gl/gl_apple_draw.cpp` | the full-screen pass that performs the accumulation operations |
| modify | `src/metal/AppleMetalBridge.mm` | the blend operation needed for `GL_ACCUM`/`GL_ADD` |

### 15.2 New context state

```cpp
struct LegacyState {
    std::shared_ptr<metal::ITexture> accumTexture;   // the accumulation buffer
    uint32_t accumWidth{0}, accumHeight{0};
    float4  accumClear{0,0,0,0};
};
```

### 15.3 Semantics to implement

* The five operations (glspec21 §4.2.4): `GL_ACCUM`, `GL_LOAD`, `GL_RETURN`,
  `GL_MULT`, `GL_ADD`.
* The values are **signed in `[−1,1]`** and are clamped **only** on
  `GL_RETURN`.
* The scissor test restricts which pixels an operation affects.
* `glClearAccum` sets the clear value.
* The accumulation texture must be a **floating-point** target (RGBA16Float at
  minimum; the RGBA32Float renderability of the A11 must be probed on device
  before relying on it).

### 15.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_accum_basic` | `tests/unit/test_legacy_accum_basic.cpp` | `glClearAccum` sets the value; the operation enum is validated; the "no accumulation buffer" case raises `INVALID_OPERATION` (only reachable if the context was created without one) |
| `test_legacy_accum_gpu` | `tests/integration/test_legacy_accum_gpu.cpp` | on a real GPU: N jittered frames accumulated and returned produce the expected average, within a tolerance; the scissor box restricts the affected pixels; the `[−1,1]` signed semantics are respected |

### 15.5 Exit criteria

* The 2 entry points implemented.
* The accumulation round-trip matches a CPU reference.

### 15.6 Depends on

S9 (it reads and writes the framebuffer).

---

## 16. S14 — Client-side vertex arrays

**Entry points: 12** (all of L02).

`glClientActiveTexture`, `glColorPointer`, `glDisableClientState`,
`glEdgeFlagPointer`, `glEnableClientState`, `glFogCoordPointer`,
`glIndexPointer`, `glInterleavedArrays`, `glNormalPointer`,
`glSecondaryColorPointer`, `glTexCoordPointer`, `glVertexPointer`.

### 16.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_clientarray.cpp` | the 12 entry points |
| modify | `include/tglmt/LegacyState.h` | the per-array descriptors |

### 16.2 New context state

```cpp
struct LegacyClientArray {
    bool     enabled{false};
    GLint    size{4};
    GLenum   type{GL_FLOAT};
    GLsizei  stride{0};
    const void* pointer{nullptr};
};
struct LegacyState {
    LegacyClientArray arrays[8];   // VERTEX, COLOR, NORMAL, TEXTURE_COORD,
                                    // SECONDARY_COLOR, INDEX, EDGE_FLAG, FOG_COORD
    GLuint clientActiveUnit{0};
    LegacyClientArray texCoordArrays[kMaxTextureUnits];
};
```

### 16.3 Semantics to implement

* The array is enabled/disabled by name (`GL_VERTEX_ARRAY`, `GL_COLOR_ARRAY`,
  `GL_NORMAL_ARRAY`, `GL_TEXTURE_COORD_ARRAY`, `GL_SECONDARY_COLOR_ARRAY`,
  `GL_INDEX_ARRAY`, `GL_EDGE_FLAG_ARRAY`, `GL_FOG_COORD_ARRAY`).
* The data are read **when the drawing command is issued**, not when the
  pointer is set.
* `glInterleavedArrays(format, stride, pointer)` sets up several arrays from
  one interleaved buffer (the 17 formats).
* Validation: `size` outside `[1,4]` or an unsupported `type` ⇒
  `INVALID_VALUE`; an unknown array name ⇒ `INVALID_ENUM`.
* `glClientActiveTexture` selects the texture unit for the texcoord arrays and
  interacts with `glActiveTexture`.

**Implementation.** The arrays are gathered on the CPU at draw time into the
same staging buffer the immediate-mode path uses, so both paths share one
vertex layout and one pipeline.

### 16.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_clientarray_state` | `tests/unit/test_legacy_clientarray_state.cpp` | the setters store the descriptors; the enable/disable names are validated; the error cases raise the right codes |
| `test_legacy_clientarray_gpu` | `tests/integration/test_legacy_clientarray_gpu.cpp` | on a real GPU: a quad drawn from a client array produces the same pixels as the same quad drawn from a buffer object and from immediate mode; the array data are read at *draw* time (mutate the client memory after `glVertexPointer` and before the draw ⇒ the new data are used) |
| `test_legacy_interleaved` | `tests/integration/test_legacy_interleaved.cpp` | every `glInterleavedArrays` format produces the same pixels as the equivalent separate arrays |

### 16.5 Exit criteria

* The 12 entry points implemented.
* The client-array, buffer-object and immediate-mode paths produce identical
  pixels for the same geometry.

### 16.6 Depends on

S3 (the shared assembler).

---

## 17. S15 — Attribute stacks

**Entry points: 4** (all of L17): `glPopAttrib`, `glPopClientAttrib`,
`glPushAttrib`, `glPushClientAttrib`.

### 17.1 Files

| Action | File | Reason |
|---|---|---|
| create | `src/gl/gl_legacy_attribstack.cpp` | the 4 entry points |
| modify | `include/tglmt/LegacyState.h` | the two stacks |

### 17.2 New context state

```cpp
struct LegacyState {
    std::vector<LegacyAttribSnapshot> serverStack;   // depth ≥ 16
    std::vector<LegacyAttribSnapshot> clientStack;   // depth ≥ 16
};
```

The snapshot is a struct copy of the groups selected by the mask.

### 17.3 Semantics to implement

* The server stack saves the server groups; the client stack the client
  groups (glspec21 table 6.2).
* `STACK_OVERFLOW` when the stack is at `MAX_ATTRIB_STACK_DEPTH` /
  `MAX_CLIENT_ATTRIB_STACK_DEPTH`; `STACK_UNDERFLOW` on an empty pop. The
  depth is at least 16.
* Bits set in the mask that do not correspond to a group are ignored.
* **Matrix stacks are never pushed or popped.**
* The texture-group ordering: ascending on push, descending on pop.

### 17.4 Tests

| Test ID | File | What it checks |
|---|---|---|
| `test_legacy_attribstack` | `tests/unit/test_legacy_attribstack.cpp` | a push/pop round-trips the selected groups and leaves the others alone; the matrix stacks are unaffected by a push/pop; the texture units are restored in the reverse order; the overflow/underflow codes are right |
| `test_legacy_clientattribstack` | `tests/unit/test_legacy_clientattribstack.cpp` | the client stack saves the vertex-array and pixel-store state |

### 17.5 Exit criteria

* The 4 entry points implemented; the group semantics are verified per group.
* The matrix-stack exclusion is verified.

### 17.6 Depends on

S4, S5, S6 (the state it snapshots) and S14 (the client state).

---

## 18. S16 — GLU

**Entry points: TBD.** The scope is fixed by `project/docs/DECISIONS.md` D-015
and `project/TODO.md` T-019: the tessellator, the quadric objects,
`gluBuild2DMipmaps` and `gluErrorString`. The open question Q11 (write a
subset ourselves vs. use a vendor library) is recorded in
`project/docs/PLAN.md` §"Q11" and is **not** decided here.

This stage is listed for completeness of the roadmap; its API list is not
fixed and it is out of the legacy fixed-function scope of this document.

---

## 19. Cross-cutting implementation requirements

These apply to **every** stage and are not repeated per stage.

### 19.1 The generated files

Per `project/AGENTS.md` §5, the following are generated and must be changed
through their generator, never by hand:

| File | Generator | What changes |
|---|---|---|
| `include/tglmt/gl46.h`, `include/tglmt/gl46_types.h`, `src/gl/gl_dispatch_gen.cpp` | `tools/gen_gl_headers.py` | the new legacy entry-point declarations (the header generator only emits the core 657 + the 42 compat-only; it must be extended to emit the legacy set) |
| `src/gl/gl_c_exports.cpp` | `tools/gen_c_exports.py` | the `extern "C"` wrappers for the new entry points, plus the Begin/End guard and the display-list capture hooks |
| `src/gl/gl_stubs_gen.cpp` | `tools/gen_stubs.py` | nothing — the new entry points must be *implemented*, not stubbed |
| `include/tglmt/gl46_pnames.h` | `tools/gen_state_pnames.py` | the new state pnames |

`tools/gl46_catalog.py` is the single source of truth for the core command
set (D-002). The legacy commands need a **separate** derivation — the
registry's `<remove profile="core">` blocks at GL 3.2 already encode exactly
which commands are compatibility-only, so the legacy set can be derived from
the same registry rather than hand-listed. The generator change should do
that, so that the header and the export table stay consistent with the
registry.

### 19.2 The state-ownership rule

**All** fixed-function state lives in `LegacyState`, never in `StateTracker`.
`StateTracker` is the core-profile state machine and the 185 existing tests
exercise it. Mixing the two would risk the modern path for no benefit.

The one exception is state that is *already* correct in the modern path and
is shared by both profiles: the viewport, the scissor box, the depth range,
the blend state, the depth/stencil state, the texture bindings and the active
texture unit. Those stay in `StateTracker` and the legacy path reads them
from there. The audit lists them in §14.

### 19.3 The uniform-block convention

The legacy program uses the same `buffer(16)` convention as the modern path
(`src/gl/glsl_to_msl.cpp:12371`) and the same uniform-staging path
(`src/gl/gl_apple_draw.cpp:2817-2941`). The legacy uniform struct is
`TGLMTLegacyUniforms`; its layout is the generator's responsibility and must
match the MSL struct exactly.

### 19.4 The "no silent inert state" rule

Every legacy enable cap (`GL_LIGHTING`, `GL_FOG`, `GL_ALPHA_TEST`,
`GL_TEXTURE_GEN_S/T/R/Q`, `GL_CLIP_PLANE0..5`, `GL_LIGHT0..7`,
`GL_COLOR_MATERIAL`, `GL_RESCALE_NORMAL`, `GL_NORMALIZE`,
`GL_POINT_SMOOTH`, `GL_POINT_SPRITE`, `GL_POLYGON_STIPPLE`, `GL_LINE_STIPPLE`,
`GL_INDEX_LOGIC_OP`, `GL_COLOR_LOGIC_OP`, `GL_POLYGON_SMOOTH`,
`GL_LINE_SMOOTH`) must do something in a compatibility context. Until the
stage that implements it lands, the compatibility context must reject the
enable with `GL_INVALID_OPERATION`. This is tracked per cap in the stage
table below.

| Cap | Implemented by | Until then |
|---|---|---|
| `GL_LIGHTING`, `GL_LIGHT0..7` | S4 | reject |
| `GL_COLOR_MATERIAL`, `GL_RESCALE_NORMAL`, `GL_NORMALIZE` | S4 | reject |
| `GL_FOG` | S5 | reject |
| `GL_TEXTURE_GEN_S/T/R/Q` | S6 | reject |
| `GL_ALPHA_TEST` | S7 | reject |
| `GL_CLIP_PLANE0..5` | S7 | reject |
| `GL_POLYGON_STIPPLE`, `GL_LINE_STIPPLE` | S7 | reject |
| `GL_POINT_SPRITE` | S6 | reject |
| `GL_COLOR_LOGIC_OP`, `GL_INDEX_LOGIC_OP` | S7 | reject |
| `GL_LINE_SMOOTH`, `GL_POLYGON_SMOOTH`, `GL_POINT_SMOOTH` | S7 (documented as no-ops) | accept, documented |

`GL_LINE_SMOOTH` / `GL_POLYGON_SMOOTH` / `GL_POINT_SMOOTH` are the exception:
Metal has no such rasterisation modes, so they are accepted and documented
as no-ops. That must be stated in the compatibility caveats.

### 19.5 The pipeline-cache correctness rule

The legacy program is regenerated whenever its `LegacyProgramKey` changes.
On regeneration:

* a **new** `ILibrary` pair is compiled (a new serial ⇒ a new pipeline key
  entry, automatically);
* `linkGen` is bumped (the legacy program uses the same
  `ProgramObject::linkGen` mechanism, `include/tglmt/Context.h:222`);
* the old libraries are released only after the pipeline cache no longer
  references them (the existing `pipeCache[]` holds `shared_ptr`, so this is
  handled by the existing lifetime rules).

**The test that proves it:** a test that changes the legacy program's shape
(the number of active texture units, for instance), draws, changes it back,
draws, and verifies that the two draws produce different pixels — i.e. that
the cache did not hand back the wrong pipeline.

### 19.6 The error-generation rule

Per `project/AGENTS.md` §5: "Mọi entry point phải validate và sinh lỗi GL đúng
spec". Every new entry point must:
* validate its enums and ranges;
* raise the right error code;
* leave the state unchanged when it errors (the modern path has a convention
  for this: the DSA state setters validate before mutating, e.g.
  `src/gl/gl_ext_dsa_state.cpp`).

### 19.7 The inventory obligation

Per stage, the newly implemented entry points must appear in
`tests/truth/api_manifest.json` with a `tests` reference. The gate is:

```sh
python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --write
python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
```

`--strict` fails if an advertised symbol is missing or is a no-op. Once the
legacy commands are exported, `--strict` becomes the gate that keeps
`glLineStipple`-style stubs from being counted as implemented.

---

## 20. Entry-point count per stage (summary)

| Stage | New entry points | Cumulative |
|---|---:|---:|
| S0 Context/profile | 0 | 0 |
| S1 Matrix stack | 20 | 20 |
| S2 Current vertex + guard | 0 | 20 |
| S3 Immediate mode | 165 | 185 |
| S4 Lighting | 18 | 203 |
| S5 Fog | 4 | 207 |
| S6 Texenv / texgen | 15 | 222 |
| S7 Legacy rasterisation | 9 (+1 reimplemented) | 231 |
| S8 Raster / window position + rect | 48 | 279 |
| S9 Pixel ops | 12 | 291 |
| S10 Display lists | 8 | 299 |
| S11 Evaluators | 23 | 322 |
| S12 Selection / feedback | 8 | 330 |
| S13 Accumulation buffer | 2 | 332 |
| S14 Client-side vertex arrays | 12 | 344 |
| S15 Attribute stacks | 4 | **348** |

**348 = the measured missing count** from the audit §0.1 (the 349 legacy
commands minus `glLineStipple`, which is exported but inert and is reimplemented
in S7).

---

## 21. Milestones and the gates between them

| Milestone | Stages | Gate |
|---|---|---|
| **M1 — the context can be a compatibility context** | S0 | a compatibility context exists, the profile mask/version are right, the removed-function rule works, and the 185 existing tests still pass |
| **M2 — the legacy pipeline renders** | S1–S6 | an immediate-mode triangle renders the correct pixels with a model-view transform, lighting, fog and a texture environment; the batching works (one draw for many primitives) |
| **M3 — the legacy rasterisation is right** | S7 | the alpha test, the stipples, the clip planes and the logic op match the spec's pixel output |
| **M4 — the legacy specials work** | S8–S13 | the raster position, the pixel ops, the display lists, the evaluators, the selection/feedback and the accumulation buffer all behave per the spec |
| **M5 — the whole legacy surface is present** | S14–S15 | the client arrays and the attribute stacks work; **348/348** legacy entry points are implemented and the export census agrees with the inventory |
| **M6 — GLU** | S16 | the GLU subset is implemented and tested (T-019) |

### 21.1 The gate that must never be skipped

At **every** milestone:

```sh
cd /Volumes/project/TGLMT
cmake --build build-work -j8
(cd build-work && ctest -j4 --output-on-failure)          # 185/185 must pass
python3 tools/truth_manifest.py --lib build-work/libtglmt.dylib --strict
python3 tools/ext_matrix.py --lib build-work/libtglmt.dylib --strict
python3 tools/gen_gl_headers.py --check
python3 tools/ext_coverage.py --dlsym build-work/libtglmt.dylib
```

A milestone is not done if any of these fails. The 185 existing tests are the
regression gate for the whole programme: the compatibility work must not
change the core profile's behaviour.

### 21.2 The differential-test obligation

Per D-021, the reference for the legacy behaviour is a *real* OpenGL
implementation (Minecraft, a real GL game) or a test written by an AI from the
spec — **not** Mesa. The legacy stages should therefore be validated against:

* a **CPU reference implementation of each equation**, written from the spec
  and living in the test file. This is the primary reference for lighting,
  fog, texenv, texgen, the evaluators and the selection z-ranges; it is
  exact and needs no external system.
* a **real GL application** for the end-to-end behaviour (Minecraft is the
  project's canonical workload; it is a modern GL application though, so the
  *legacy* workload must come from elsewhere — the plan should identify a
  legacy GL 1.x/2.1 application to pin as the differential reference).

> **Open item.** The differential reference for the *legacy* workload is not
> yet identified. This is recorded in `project/TODO.md` as part of T-019/T-020
> and must be closed before M2 can claim conformance-grade evidence. Until
> then, the CPU-reference tests are the only evidence, and the status is
> `PARTIAL` / `NOT_VERIFIED` on device.

### 21.3 The device-verification obligation

The GPU pixel tests run on the development host (macOS, Apple GPU) and on the
A11 device. Per `project/AGENTS.md` §6, a result is only `FULLY_IMPLEMENTED`
with the environment label `A11_DEVICE`. The A11 device log is
`Documents/tglmt-device-tests.log` (per D-022, the device exists; the log
capture is tracked separately).

**The A11-specific risks that must be measured on the device:**

1. The **RGBA32Float render-target** support for the accumulation buffer
   (S13) — RGBA16Float is the fallback.
2. The **wide-line** and **line-stipple** expansion performance (S7) — the
   CPU expansion cost per segment.
3. The **logic-op** two-pass cost (S7) — an application that enables it for
   every draw will see a large slowdown.
4. The **clip-plane** CPU clipping cost (S7) — it breaks batching across a
   clip-plane change.
5. The **display-list replay** cost (S10) — the per-call overhead of the
   command-stream replay.
6. The **batching** effectiveness (S3) — the number of `drawPrimitives` calls
   for a representative legacy workload.

---

## 22. Risks and their mitigations

| # | Risk | Impact | Mitigation |
|---|---|---|---|
| R1 | The **display-list recording** needs a hook in the generated export shim, and the shim is generated | the highest-risk item; a mistake affects all 1860 exports | the guard and the capture hook are designed together and land in the same generator change; the hot-path cost is one predictable branch, measured with `bench_runtime` |
| R2 | The **Begin/End guard** must be exactly right over ~700 entry points | a missing entry point means a silently-accepted illegal command | the guard is driven by a **bitset indexed by the dispatch-table index**, generated from a hand-written allowed-set with a spec citation; a test iterates the dispatch table |
| R3 | The **legacy program regeneration** invalidates pipelines | a stale pipeline produces wrong pixels | `linkGen` is bumped on every regeneration (mapping §2.4); the cache-correctness test is mandatory |
| R4 | The **flat-shading source vertex** and the **two-sided lighting** diverge from the simplest reading of the spec | visible differences in legacy applications | both are documented divergences with an explicit spec citation; the flat-shading rule is implemented exactly (the spec's table 2.12) |
| R5 | The **logic op** and the **accumulation buffer** need extra passes | a large slowdown for the applications that use them | measured on the device; the cost is reported honestly in the compatibility caveats |
| R6 | The **six user clip planes** exceed Metal's one `[[clip_distance]]` | a divergence if the CPU clipper is used | the CPU clipper is exact, so there is **no** divergence; the cost is CPU time and a batching break |
| R7 | **Client-side vertex arrays** force a CPU copy | a slowdown versus a buffer-object path | inherent — Metal has no client arrays; the copy is a plain memcpy |
| R8 | The **export census over-reports** `glLineStipple` | a false "implemented" claim | the `--strict` truth gate plus the S7 reimplementation |
| R9 | The **legacy state block grows large** and the attribute stack snapshots become expensive | a memory/perf regression | the block is a few kB; 16 stack levels is tens of kB; measured in S15 |
| R10 | The **no differential reference** for the legacy workload | the evidence is CPU-reference-only | identified as an open item (§21.2); the CPU references are exact for the equations, but the end-to-end behaviour needs a real legacy application |

---

## 23. What this plan does **not** cover

* **GLU** (S16) — the scope is fixed by D-015 but the API list is open (Q11).
* **The GLSL 1.10 / 1.20 compatibility profile** — that is
  `project/TODO.md` T-037, a *shader-language* item, not a fixed-function one.
  The legacy GLSL built-ins (`gl_Vertex`, `gl_Color`, `gl_Normal`,
  `gl_MultiTexCoord*`, `gl_FogCoord`, `gl_ModelViewMatrix`,
  `gl_ProjectionMatrix`, `gl_NormalMatrix`, `gl_ClipPlane*`) are a separate
  work item and are **not** part of the fixed-function pipeline this plan
  covers.
* **The geometry-shader emulation** required by D-018 — a modern-tier item;
  the legacy pipeline has no geometry-shader stage and does not help with it.
* **The window-system layer** (T-011) — creating and selecting contexts is a
  separate item; this plan assumes a context exists.
* **The modern-profile API completion** (T-040 … T-045) — separate items.
* **Performance optimisation beyond the cost model** — Phase 10 of the plan
  (`project/docs/PLAN.md`) covers the general performance work; this plan
  only ensures the legacy path is not *worse* than an equivalent GL 1.x
  implementation.

---

## 24. Cross-references

| Topic | Where |
|---|---|
| Spec-level audit, per-group status and semantics | `docs/LEGACY_PIPELINE_AUDIT.md` |
| Metal mapping strategy, the MSL sketches, the cost model | `docs/LEGACY_TO_METAL_MAPPING.md` |
| Profile split, `L01`–`L20` grouping | `docs/OPENGL_PROFILE_MATRIX.md` |
| Measured export status of the compat surface | `docs/OPENGL_API_COVERAGE.md` |
| A11 hardware limits | `docs/METAL_A11_LIMITATIONS.md` |
| Project vocabulary, the status tokens, the `HARDWARE_LIMITED` bar | `project/docs/GLOSSARY.md`, `project/AGENTS.md` §6 |
| Project decisions (D-014 one-tier, D-015 GLU, D-018 GS, D-021 reference) | `project/docs/DECISIONS.md` |
| Open work items | `project/TODO.md` T-010, T-019 … T-038 |
| Overall phase plan | `project/docs/PLAN.md` |
