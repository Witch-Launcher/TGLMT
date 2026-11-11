# GLSL 4.60 → MSL — Compiler Architecture, Coverage & Divergences

> The TGLMT shader compiler is a **token/string rewriter**, not a compiler
> front end. This document states exactly what it supports, what it does not,
> and — most importantly — where the produced MSL **differs semantically**
> from the GLSL that was submitted.

---

## 1. Architecture

`src/gl/glsl_to_msl.cpp` (12 564 LOC). The pipeline:

| # | Stage | Entry point | Algorithm |
|---|---|---|---|
| 1 | Comment strip | `StripComments` | character scan |
| 2 | C++ keyword rename | `RenameCxxOnlyKeywords` | token list swap (`new`, `class`, `thread`, `device`, … → `tglmt_kw_*`) |
| 3 | Preprocessor | (inline) | **line-oriented**; `#version` whitelist, `#define` (object- and function-like), `#undef`, `#ifdef/#ifndef`, `#if/#elif` via a small recursive-descent integer evaluator, `#else/#endif`. Any other directive in an active branch → **hard fail**. |
| 4 | Struct split | `SplitStructDeclarators`, `CollectStructInfos` | brace matching + a member type/offset table |
| 5 | Declaration scan | `ParseDecls` and friends | whitespace tokenisation; recognises `uniform`, `in`, `out`, `layout(...)`, uniform blocks, SSBOs, images, atomic counters, subroutines |
| 6 | Precision qualifiers | — | **dropped** (correct for desktop GL 4.60: precision qualifiers have no effect there, and MSL has no equivalent) |
| 7 | Layout | `MSLAlign` / `MSLSize` | **MSL natural alignment**, *not* std140/std430 |
| 8 | Body rewriting | ~60 passes | see §3 |
| 9 | MSL emission | — | fixed prelude (~70 helper templates) + stage structs + entry point |

There is **no AST, no type system, no overload resolution and no dataflow
analysis**. Every semantic decision is made by substring matching on the
source text. Consequences are listed in §5.

## 2. Design principle: fail loudly

Anything the translator cannot express exactly produces
`GLSLConvertResult::ok == false` plus a human-readable log, and
`GL_COMPILE_STATUS` becomes `GL_FALSE`. The translator never emits a
"best guess" shader. Examples of honest failures:

* `double` / `dvec` types → `fail("kiểu không hỗ trợ: double")`
* `#include` → `fail("directive không hỗ trợ: #include ...")`
* geometry shader stage → rejected at `glCompileShader`
* `isolines` / `point_mode` tessellation domains → rejected
* SSBO with an unsized array **and** an instance name → rejected

---

## 3. Feature coverage

Legend: ✅ correct · ⚠️ supported with a semantic caveat · ❌ rejected with a
clear error.

### 3.1 Versions and preprocessor

| Feature | | Notes |
|---|---|---|
| `#version 460/450/440/430/410/400/330/300/150/140/130/120/100` | ✅ | the version token is validated against a whitelist |
| `#define` / `#undef` | ✅ | object-like and function-like |
| `#ifdef` / `#ifndef` / `#else` / `#endif` | ✅ | |
| `#if` / `#elif` | ⚠️ | only `defined`, `!`, `&&`, `\|\|`, parentheses and integer literals — **no arithmetic or relational operators** (`#if X > 3` fails) |
| `#include` | ❌ | hard fail |
| `#line` / `#pragma` / `#extension` | ✅ | dropped (no capability reporting) |

### 3.2 Declarations and layout

| Feature | | Notes |
|---|---|---|
| `layout(location=N)` on `in`/`out`/fragment outputs | ✅ | |
| `layout(binding=N)` | ⚠️ | honoured for uniform blocks, images and atomic counters; **ignored** on a plain uniform/sampler |
| `layout(set=N)` | ❌ | not parsed (Vulkan-style GLSL is out of scope) |
| `layout(offset=M)` | ⚠️ | honoured only for `atomic_uint` |
| `layout(std140)` / `std430` / `packed` / `shared` | ❌ | **ignored** — the layout is always MSL natural alignment. See §5.1 |
| `layout(column_major)` / `row_major` | ❌ | ignored; `row_major` therefore produces the wrong layout |
| `layout(vertices=N)` (TCS) | ✅ | validated 1..32 |
| `layout(triangles/quads)` (TES) | ✅ | `isolines` and `point_mode` rejected |
| `layout(local_size_x/y/z)` | ✅ | validated 1..1024 |
| `layout(index=I)` (dual source) | ⚠️ | mapped to a second draw buffer, not dual-source blending — impossible on A11 |
| `layout(depth_greater/depth_any)` | ✅ | → `[[depth(greater)]]` / `[[depth(any)]]` |

### 3.3 Interpolation qualifiers

| Feature | | Notes |
|---|---|---|
| `smooth` | ✅ | MSL default is `center_perspective` |
| `flat` | ✅ | → `[[flat]]` (fixed in this work; see §5.2) |
| `noperspective` | ✅ | → `[[center_no_perspective]]` (fixed in this work) |
| `centroid` | ✅ | → `[[center]]` (fixed in this work) |
| `sample` | ⚠️ | stripped; MSL `[[sample]]` is not emitted |
| `invariant`, `precise` | ⚠️ | stripped (mostly harmless) |

### 3.4 Types

| Feature | | Notes |
|---|---|---|
| `float/vec2..4`, `int/ivec*`, `uint/uvec*` | ✅ | |
| `bool/bvec2..4` | ⚠️ | `bvec` exists as a type; `!=`/`==` on vectors rely on the relational-rewrite heuristics |
| `double/dvec*` | ❌ | rejected (no `double` in MSL) |
| `mat2/mat3/mat4` | ✅ | |
| `mat2x3`, `mat3x2`, `mat2x4`, `mat4x2`, `mat3x4`, `mat4x3` | ⚠️ | only some reshaping combinations are implemented |
| `struct` | ✅ | |
| `const` global / `const` array | ✅ | → `constant` |
| unsized local arrays | ⚠️ | the size is inferred from the initialiser only |

### 3.5 Built-in variables

| Feature | | Notes |
|---|---|---|
| `gl_Position`, `gl_PointSize` | ✅ | |
| `gl_FragCoord`, `gl_FrontFacing`, `gl_PointCoord`, `gl_FragDepth` | ✅ | |
| `gl_VertexID`, `gl_InstanceID`, `gl_PrimitiveID` | ✅ | |
| `gl_SampleID`, `gl_SamplePosition`, `gl_SampleMask` | ✅ | |
| `gl_WorkGroupSize` | ⚠️ | `local_size` is captured, but `gl_WorkGroupSize[]` is not rewritten |
| `gl_LocalInvocationID/Index`, `gl_GlobalInvocationID/Index`, `gl_NumWorkGroups`, `gl_WorkGroupID` | ✅ | |
| `gl_Layer`, `gl_ViewportIndex` | ❌ | not supported |
| `gl_InvocationID` | ⚠️ | only the `gl_in[]`/`gl_out[]` members are supported |
| `gl_TessLevelOuter/Inner` | ⚠️ | read from a factor buffer |
| `gl_ClipDistance[0]` | ✅ | only `[0]` (Metal has one clip distance) |

### 3.6 Built-in functions

| Feature | | Notes |
|---|---|---|
| `texture(s, uv)` | ✅ | implicit derivatives |
| `texture(s, uv, bias)` | ✅ | **fixed in this work** — see §5.3 |
| `textureLod` / `textureGrad` / `textureProj` / `textureOffset` | ✅ | |
| `textureQueryLod` | ✅ | **fixed in this work** — see §5.4 |
| `textureSize` / `textureGather` / `texelFetch` | ✅ | |
| `imageLoad` / `imageStore` / `imageAtomic*` | ✅ | |
| `atomicAdd/Min/Max/And/Or/Xor/Exchange/CompSwap` | ✅ | |
| `barrier()` / `memoryBarrier*` / `groupMemoryBarrier` | ⚠️ | the strongest Metal barrier is emitted; GL's finer memory model is not reproduced |
| `inverse` / `transpose` / `determinant` / `outerProduct` / `matrixCompMult` | ✅ | native MSL equivalents |
| bit-field ops, `bitCount`, `findLSB/MSB` | ✅ | |
| `pack*` / `unpack*` | ✅ | |
| `floatBitsToInt` and friends | ✅ | → `as_type<>` |
| `dFdx` / `dFdy` / `fwidth` | ✅ | |
| `mod` | ✅ | `x - y*floor(x/y)` — matches GLSL §8.3 exactly |
| integer `/` and `%` | ✅ | both truncate toward zero, identical to GLSL 4.60 §5.9 |
| `lessThan` / `greaterThan` / `<=` / `>=` on vectors | ⚠️ | rewritten to MSL operators, which yield a `boolN`. Using the result as a scalar condition is invalid in GLSL too, so this is not a divergence — but the generated MSL is not accepted by Metal in that case |
| `notEqual` / `equal` | ⚠️ | heuristic (name-based) rewrite |
| `ftransform`, `gl_NormalMatrix` | ❌ | not supported |

### 3.7 Stages

| Stage | | Notes |
|---|---|---|
| Vertex | ✅ | |
| Fragment | ✅ | |
| Compute | ✅ | `shared` → `threadgroup`, `barrier()` → `threadgroup_barrier` |
| Tessellation control / evaluation | ✅ | `triangles`/`quads` only |
| Geometry | ❌ | rejected with a clear log (no Metal equivalent on A11) |

---

## 4. Known **semantic** divergences (post-fix list)

Everything below was verified by inspecting the generated MSL **and** by
running the Metal compiler on it.

| # | Divergence | Severity | Status |
|---|---|---|---|
| D1 | `std140`/`std430`/`packed`/`shared` block layout qualifiers are ignored; MSL natural alignment is used instead | Medium | **Open** — the packer compensates for *uniform* blocks but not for SSBO/std140 array strides or `bool` members |
| D2 | `row_major` layout qualifier is ignored | Low | **Open** (rare in practice) |
| D3 | `GL_TEXTURE_LOD_BIAS`, `GL_TEXTURE_MIN_LOD`, `GL_TEXTURE_COMPARE_MODE` on a *texture* (not a sampler object) are not read | Medium | **Open** |
| D4 | The four GL mipmap filters all map to Metal `Linear` (trilinear) | Low (visual) | **Open** |
| D5 | `gl_Position.w` is not honoured as the clip-space `w` (the depth remap assumes `w == 1`) | Medium | **Open** |
| D6 | Dual-source blending is emulated with a second draw buffer | Low on A11 | **Documented as hardware-limited** |
| D7 | `precision` qualifiers are dropped | None | Correct for desktop GL 4.60 |
| D8 | Unknown identifiers are not diagnosed at GLSL compile time; they fail at Metal compile time (link) | Low | **Open** — would need a real front end |
| D9 | `lessThan` etc. produce a `boolN` that Metal rejects in a scalar condition | None | Matches GLSL's own restriction |

---

## 5. Bugs found and fixed in this work (with the evidence)

### 5.1 Uniform matrix packing — `transpose` ignored and `mat3` corrupted

* **Symptom**: `glUniformMatrix3fv` stores 9 floats (36 bytes, tight
  column-major). MSL's `float3x3` inside a `constant` struct is three `float3`
  columns with a 16-byte stride → **48 bytes**. Copying 36 contiguous bytes
  placed columns 1 and 2 four and eight bytes too early ⇒ the shader read a
  garbage matrix.
* **Second defect**: the `transpose` argument was discarded (`(void)t`), so
  row-major data was stored as if it were column-major.
* **Fix**: `include/tglmt/uniform_pack.h` adds `PackGlslMatrixToMsl` (tight →
  padded column layout) and `TransposeGlslMatrix`; the `glUniformMatrix*fv`
  family now honours `transpose` and rejects a negative count; the packer is
  applied in both the normal draw path and the tessellation path.
* **Evidence**: `tests/regression/test_regression_shader_semantics.cpp` (cases R5/R6).

### 5.2 Interpolation qualifiers dropped

* **Symptom**: `flat`/`noperspective`/`centroid` were stripped from the
  declaration without emitting any MSL attribute. MSL's default is
  `center_perspective`, so a GLSL `flat` varying was **smoothly interpolated**.
* **Fix**: `GLSLVar::interpolation` carries the qualifier; `InterpAttr()`
  emits `[[flat]]` / `[[center_no_perspective]]` / `[[center]]`.
* **Evidence**: `tests/regression/test_regression_shader_semantics.cpp` (case R2).

### 5.3 `texture(s, uv, bias)` — the bias was silently discarded

* **Symptom**: the converter emitted `sample(smp, coord, float(bias))`. Metal
  has three overloads that accept a third argument:
  `sample(s, c, int2 offset)`, `sample(s, c, bias options, ...)` and
  `sample(s, c, level options, ...)`. A bare `float` converts to `int2` by
  *standard* conversion, which beats the *user-defined* conversion to `bias`
  ⇒ overload resolution picked the **offset** overload and the LOD bias was
  turned into a texture offset.
* **Proof** (real Metal compiler):
  ```
  warning: implicit conversion from 'float' to 'metal::int2' changes value from 0.5 to 0
  ```
  and with a *runtime* float there is no diagnostic at all — the shader
  compiles clean and samples at LOD 0 with offset (0,0).
* **Fix**: emit the tag type explicitly — `bias(float(x))`.
* **Evidence**: `tests/regression/test_regression_shader_semantics.cpp` (case R1).

### 5.4 `textureQueryLod` produced uncompilable MSL

* **Symptom**: `textureQueryLod` was not in the rewrite list, so the GLSL
  sampler name passed through into MSL, where it is undeclared:
  `error: use of undeclared identifier 'uT'`.
* **Fix**: `RewriteTextureQueryLod` → `calculate_unclamped_lod(smp, coord)`.
* **Evidence**: `tests/regression/test_regression_shader_semantics.cpp` (case R4).

### 5.5 A single fragment output at `location != 0` was written to attachment 0

* **Symptom**: the converter chose between "one plain `float4` return" and
  "a struct with `[[color(N)]]`" based on the *number* of outputs, not on
  their locations. A single output at `layout(location=1)` therefore took the
  plain-return path, and Metal binds a plain fragment return to `color(0)` —
  the app rendered into the wrong attachment.
* **Fix**: the struct path is used whenever an output has an explicit
  location/index other than 0, and the emitted `[[color(N)]]` uses the real
  location. The linker (`gl_program.cpp`) was updated at the same time so
  that `glBindFragDataLocation` still takes precedence for outputs **without**
  an explicit layout (as GL 4.6 §7.3.2 requires).
* **Evidence**: `tests/regression/test_regression_shader_semantics.cpp` (case R3).

---

## 6. How to verify

```sh
# 1. Converter tự nó: dịch một shader và in MSL
cmake --build build -j8
./build/glsl2msl_cli <file.glsl> <vs|fs|cs|tcs|tes>

# 2. MSL phải được Metal compiler chấp nhận
xcrun -sdk macosx metal -c out.metal -o out.air

# 3. Kiểm thử hồi quy ngữ nghĩa
./build/test_regression_shader_semantics
```

`tests/unit/test_converter_aquarium.cpp` compiles the whole
`apps/aquarium/shaders` corpus with the real Metal compiler and checks that
the `.air` file is actually produced (the previous version wrote `.metal` but
compiled `.msl`, so the gate never fired).

`tests/integration/test_converter_proof.cpp` is a **differential** test: the
same geometry is rendered through TGLMT's converter and through
`glslangValidator -G` + `spirv-cross`, and the pixels are compared within
±5. This is the strongest shader-correctness evidence in the repository.
