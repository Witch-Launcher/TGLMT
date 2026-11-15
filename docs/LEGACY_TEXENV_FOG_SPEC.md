# LEGACY_TEXENV_FOG_SPEC.md — Texture Environment, Texture Generation & Fog

**Authoritative implementation reference for TGLMT's OpenGL→Metal compatibility layer.**

Scope: `glTexEnv*`, `glGetTexEnv*`, `glTexGen*`, `glGetTexGen*`, `glFog*`, and the fixed-function
fragment stages they drive (texture environment → texture application → color sum → fog).

This document covers **Compatibility Profile** semantics. In the **Core Profile** all of
`glTexEnv*`, `glTexGen*` and `glFog*` are *removed* (see §0.3), so every statement here is
COMPAT-only.

---

## 0. Sources, citation policy, and a major correction to the task brief

### 0.1 Sources actually consulted

| Ref | Document | How obtained | Status |
|---|---|---|---|
| **[CORE46]** | OpenGL 4.6 **Core Profile** Specification (May 5, 2022), 851 pages | **In-repo**: `docs/khronos/glspec46.core.pdf` | ✅ VERIFIED locally |
| **[C46]** | OpenGL 4.6 **Compatibility Profile** Specification (May 5, 2022), 1027 pages | Khronos GitHub registry mirror `KhronosGroup/OpenGL-Registry` → `specs/gl/glspec46.compatibility.pdf` (registry.khronos.org returns HTTP 403 to this environment) | ✅ VERIFIED from the official PDF |
| **[G21]** | OpenGL 2.1 Specification (Dec 1, 2006), 394 pages | Same GitHub mirror → `specs/gl/glspec21.pdf` | ✅ VERIFIED from the official PDF |
| **[G13]** | OpenGL 1.3 Specification (Aug 14, 2001), 296 pages | Same GitHub mirror → `specs/gl/glspec13.pdf` | ✅ VERIFIED |
| **[G14]** | OpenGL 1.4 Specification (Jul 24, 2002), 312 pages | Same GitHub mirror → `specs/gl/glspec14.pdf` | ✅ VERIFIED |
| **[G121]** | OpenGL 1.2.1 Specification (Apr 1, 1999), 278 pages | Same GitHub mirror → `specs/gl/glspec121.pdf` | ✅ VERIFIED (OCR is poor; used only for the ARB_multitexture appendix) |
| **[G11]** | OpenGL 1.1 Specification (Mar 4, 1997), 194 pages | Same GitHub mirror → `specs/gl/glspec11.pdf` | ✅ VERIFIED (font-encoding-garbled; used only for the classic texenv/fog tables) |
| **[ARB-COMB]** | `ARB_texture_env_combine` extension spec | `extensions/ARB/ARB_texture_env_combine.txt` (GitHub mirror) | ✅ VERIFIED |
| **[ARB-DOT3]** | `ARB_texture_env_dot3` extension spec | `extensions/ARB/ARB_texture_env_dot3.txt` | ✅ VERIFIED |
| **[ARB-XBAR]** | `ARB_texture_env_crossbar` extension spec | `extensions/ARB/ARB_texture_env_crossbar.txt` | ✅ VERIFIED |
| **[ARB-ADD]** | `ARB_texture_env_add` extension spec | `extensions/ARB/ARB_texture_env_add.txt` | ✅ VERIFIED |
| **[EXT-COMB]** | `EXT_texture_env_combine` extension spec | `extensions/EXT/EXT_texture_env_combine.txt` | ✅ VERIFIED |
| **[NV-COMB4]** | `NV_texture_env_combine4` extension spec | `extensions/NV/NV_texture_env_combine4.txt` | ✅ VERIFIED |
| **[REG]** | OpenGL Registry `gl.xml` (token registry) | **In-repo**: `docs/khronos/gl.xml` | ✅ VERIFIED locally |

Citation convention used throughout:

* `§X.Y` = section number in the named spec.
* `p.N` = **printed page number** in the named spec (the number in the page header/footer, *not*
  the PDF page index). E.g. `[C46] §16.1 p.599`.
* PDF page index is given only as `PDF p.N` when it is the only unambiguous locator.

### 0.2 The Compatibility Profile PDF *was* obtainable

The task brief stated the Compatibility Profile PDF "could NOT be downloaded (403 from
registry.khronos.org)". That is true for `registry.khronos.org` and `www.khronos.org` (both 403
from this environment), **but** the Khronos GitHub mirror
`https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/specs/gl/` serves the same
official PDFs without a 403. Every claim below marked **[C46]** is therefore quoted from the
actual Compatibility Profile PDF, not from memory.

### 0.3 Core Profile removes all of this

`[CORE46] §E.2.2 "Deprecated and Removed Features"` (PDF p.701–705, printed p.679–683) removes:

* `TexEnv` target `TEXTURE_ENV` **and all associated parameters**;
* `TexEnv` target `TEXTURE_FILTER_CONTROL` and parameter `TEXTURE_LOD_BIAS`;
* `TexGen*` and `Enable`/`Disable` targets `TEXTURE_GEN_*`;
* `Enable` target `FOG`, `Fog`, and all associated parameters;
* the `FOG_HINT` target to `Hint`.

`[REG]` confirms this structurally: the `glTexEnv*`, `glTexGen*`, `glFog*`, `glFogCoord*`
commands appear in `GL_VERSION_1_0` and are re-listed in `GL_VERSION_3_2` **only** because
GL 3.2 restored them for compatibility; each is inside a
`<remove profile="core" comment="Compatibility-only GL 1.0 features removed from GL 3.2">`
block. `GL_TEXTURE_ENV_MODE`, `GL_TEXTURE_ENV_COLOR`, `GL_TEXTURE_GEN_MODE`, `GL_OBJECT_PLANE`,
`GL_EYE_PLANE`, `GL_COMBINE*`, `GL_SRC*`, `GL_OPERAND*` remain in `gl.xml` as *tokens* because
the token registry is shared across profiles — but the *commands* that read/write them are
COMPAT-only.

`[CORE46] Table 23.19` ("Texture Environment and Generation", PDF p.623, printed p.601) is
reduced to a single row (`ACTIVE TEXTURE`) — proof that no texenv/texgen state survives in core.

### 0.4 ⚠️ CORRECTION: the task brief's `GL_COMBINE` token list is not OpenGL

The brief asks for "GL_COMBINE (GL 1.3+): the full set of combine functions (GL_ADD,
GL_SUBTRACT, GL_REVERSE_SUBTRACT, GL_MIN, GL_MAX, GL_MULTIPLY, GL_SRC_ALPHA,
GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA_SATURATE)" and "combine ARGUMENT operands (GL_SRC_COLOR,
GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA,
GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC0_COLOR..GL_SRC2_COLOR)" and
"the SOURCE arguments (GL_SOURCE0_RGB/ALPHA …)" and "expressed in terms of A0/A1/A2 and C0/C1/C2".

**This is a mixture of two different, mutually incompatible APIs and is NOT OpenGL's
`GL_COMBINE`.** Evidence:

1. `[REG]` — `GL_SRC0_COLOR`, `GL_SRC2_COLOR`, `GL_EYE_LINEAR_PLANE`, `GL_TEXTURE_GEN_S_MODE`,
   `GL_TEXTURE_GEN_T_MODE`, `GL_TEXTURE_GEN_R_MODE`, `GL_TEXTURE_GEN_Q_MODE` **do not exist**
   anywhere in the OpenGL token registry (verified by exhaustive grep: 0 hits).
2. `GL_MIN` (0x8007) and `GL_MAX` (0x8008) exist in `[REG]` but with group
   `BlendEquationModeEXT` — they are **blend equations** (`EXT_blend_minmax`), not texture
   combine functions. `GL_MULTIPLY` (0x9294) is an `EXT_blend_equation`-era token.
   `GL_SRC_ALPHA_SATURATE` (0x0308) is a **blending factor** (`EXT_blend_equation_separate`).
   `GL_REVERSE_SUBTRACT` does not exist under that name at all — the registry token is
   `GL_FUNC_REVERSE_SUBTRACT` (0x800B), an `EXT_blend_subtract` **blend equation**.
3. `GL_DST_COLOR` / `GL_ONE_MINUS_DST_COLOR` / `GL_DST_ALPHA` / `GL_ONE_MINUS_DST_ALPHA` are
   **blending factors**, valid for `glBlendFunc`, never for `glTexEnv`.
4. The "A0/A1/A2 / C0/C1/C2" formulation with `GL_SRC0_COLOR` style operands is the
   **3dfx-style `COMBINE`** extension (`3DFX_texture_env_combine`), a vendor extension that was
   never in the Khronos registry under that name in a form retrievable here, and is **not** in
   OpenGL 1.3, 2.1 or 4.6.

**Real OpenGL `GL_COMBINE`** (GL 1.3+, `[ARB-COMB]`, `[G13] §3.8.12`, `[G21] §3.8.13`,
`[C46] §16.1`) uses:

* **Combine functions** (`COMBINE_RGB` / `COMBINE_ALPHA`): `GL_REPLACE`, `GL_MODULATE`,
  `GL_ADD`, `GL_ADD_SIGNED`, `GL_INTERPOLATE`, `GL_SUBTRACT`, plus `GL_DOT3_RGB` /
  `GL_DOT3_RGBA` from `ARB_texture_env_dot3`.
* **Source arguments** (`SOURCE0_RGB`…`SOURCE2_ALPHA`): `GL_TEXTURE`, `GL_CONSTANT`,
  `GL_PRIMARY_COLOR`, `GL_PREVIOUS` (+ `GL_TEXTURE0..GL_TEXTURE<n>` from
  `ARB_texture_env_crossbar`, + `GL_ZERO` / `TEXTURE<n>_ARB` from `NV_texture_env_combine4`).
* **Operand arguments** (`OPERAND0_RGB`…`OPERAND2_ALPHA`): `GL_SRC_COLOR`,
  `GL_ONE_MINUS_SRC_COLOR`, `GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA`.

This document specifies the **real** OpenGL semantics. Where the brief's phrasing reflects the
3dfx-style model, that is called out explicitly in §A.3.9.

### 0.5 ⚠️ CORRECTION: `GL_TEXTURE_GEN_*_MODE` / `GL_EYE_LINEAR_PLANE` do not exist

The brief's items A.6 and A.7 ask for "`GL_TEXTURE_GEN_MODE` (GL 1.4): GL_OBJECT_LINEAR,
GL_OBJECT_PLANE, GL_EYE_LINEAR, GL_EYE_PLANE, GL_SPHERE_MAP" and "GL_TEXTURE_GEN_R/Q_MODE with
GL_EYE_LINEAR_PLANE".

Reality, per `[G21] §2.11.4`, `[G14] §2.10.4`, `[G21] §3.8.13` and `[REG]`:

* `glTexGen*` takes a **`coord`** argument (`GL_S`, `GL_T`, `GL_R`, `GL_Q`) and a **`pname`**
  argument. `pname` is one of **`GL_TEXTURE_GEN_MODE`**, **`GL_OBJECT_PLANE`**,
  **`GL_EYE_PLANE`** — that is the complete list, in every OpenGL version 1.1 → 4.6 compat.
* The **value** written to `GL_TEXTURE_GEN_MODE` is one of `GL_OBJECT_LINEAR`,
  `GL_EYE_LINEAR`, `GL_SPHERE_MAP`, `GL_REFLECTION_MAP`, `GL_NORMAL_MAP`.
* There is **no** `GL_TEXTURE_GEN_S_MODE`/`_T_MODE`/`_R_MODE`/`_Q_MODE` and **no**
  `GL_EYE_LINEAR_PLANE` / `GL_OBJECT_PLANE`-as-a-mode. The *per-coordinate* selection is done by
  the `coord` argument, not by separate pnames.
* `GL_OBJECT_PLANE` / `GL_EYE_PLANE` are the **pnames** that carry the 4 plane coefficients,
  *not* modes.

The "EYE_LINEAR_PLANE" concept in the brief does correspond to real behaviour, but only
indirectly: `GL_EYE_LINEAR` mode uses the plane set by `GL_EYE_PLANE`, and the eye-space plane
coefficients are derived from the object-space plane by the model-view inverse **at the time the
plane was specified**. See §A.7.

### 0.6 Deterministic choices TGLMT must make

| # | Ambiguity | TGLMT's deterministic choice | Rationale |
|---|---|---|---|
| D1 | `GL_ADD` texture-env mode: saturate to 1.0 or not? | **Saturate**: `min(1, Cp + Cs)` | `[G21]`/`[C46]` tables show plain `Cv = Cp + Cs`, but `[ARB-ADD]` explicitly states `Cv = min(1, Cf + Ct)` and its revision history records "The addition should saturate to 1." `[G21]` also states "All of these color values are in the range [0,1]". Saturation is the only reading consistent with both, and matches every real driver. |
| D2 | `GL_DECAL` on non-RGB/RGBA base internal formats | **Undefined → return the incoming fragment colour unchanged** (no-op) | `[G21] Table 3.21`/`[C46] Table 16.2` print "undefined" for ALPHA/LUMINANCE/LUMINANCE_ALPHA/INTENSITY. A deterministic no-op is the least surprising. |
| D3 | `GL_SPHERE_MAP` when the reflection vector is degenerate | Clamp `m` to a small positive epsilon; if `r = (0,0,-1)` output `s = t = 0.5` | See §A.6.5. |
| D4 | Fog coordinate source `GL_FRAGMENT_DEPTH`: radial distance or `\|z_eye\|`? | **Radial distance** `sqrt(x²+y²+z²)` in eye space by default | Spec says "the eye-coordinate distance from the eye … to the fragment center" = radial. `\|z_e\|` is only an explicitly *permitted approximation*. See §B.3. |
| D5 | `GL_FOG_HINT` | **Ignore it** (compute fog per-fragment) | `[C46] §21.5`: "An implementation may ignore them entirely." |
| D6 | Fog `GL_LINEAR` with `s == e` | Return `f = 0.0` (fully fogged) | Spec: "if, when 16.3 is selected, e = s, results are undefined". Fully-fogged is the safest. |
| D7 | `GL_COMBINE` `GL_DOT3_RGB`/`GL_DOT3_RGBA` | Implement both (they are core in 4.6 compat) | `[C46] Table 16.4` includes them. |
| D8 | `GL_FOG_COORD` source requires `glFogCoord*` vertex data | Implement; default the fog coordinate to 0 if unset | `[G21] §3.10` + `[C46] §16.4` |
| D9 | `GL_TEXTURE_ENV_COLOR` clamping | Clamp each component to [0,1] on set | `[G21] §3.8.13`: "values outside this range are clamped to it". |
| D10 | `GL_RGB_SCALE` / `GL_ALPHA_SCALE` invalid values | `GL_INVALID_VALUE` unless 1.0/2.0/4.0 | `[ARB-COMB]` "Errors". |

---

# PART A — TEXTURE ENVIRONMENT

## A.1 Targets and their exact meanings

### A.1.1 The three targets

| Target | Token | Introduced | Meaning |
|---|---|---|---|
| `GL_TEXTURE_ENV` | 0x2300 | GL 1.1 | The **texture environment of the currently active texture unit**. |
| `GL_TEXTURE_FILTER_CONTROL` | 0x8500 | GL 1.4 | Per-texture-unit **LOD bias** only (`GL_TEXTURE_LOD_BIAS` = 0x8501). Not a texture environment. |
| `GL_POINT_SPRITE` | 0x8861 | GL 1.4 / `ARB_point_sprite` | Point-sprite coordinate replacement (`GL_COORD_REPLACE` = 0x8862). Not a texture environment. |

Sources: `[G11] §3.8.5` ("target must currently be the symbolic constant TEXTURE_ENV"),
`[G13] §3.8.12` (same), `[G14] §3.8.13` ("target must be one of TEXTURE_ENV or
TEXTURE_FILTER_CONTROL"), `[G21] §3.8.13` ("target must be one of POINT_SPRITE, TEXTURE_ENV or
TEXTURE_FILTER_CONTROL"), `[C46] §16.1 p.598` (same three).

### A.1.2 `GL_TEXTURE_ENV` in GL 1.4+ — the "active texture unit" semantics

**This is the single most important gotcha in the whole area.** In GL 1.1/1.2 (before
`ARB_multitexture` was core) there was exactly one texture environment and `GL_TEXTURE_ENV`
meant "the one and only". Once `ARB_multitexture` landed (GL 1.2.1 appendix F.2, folded into
OpenGL 1.3), `GL_TEXTURE_ENV` became **"the environment of the texture unit selected by
`glActiveTexture`"**.

Verbatim:

* `[G121] Appendix F.2 (Multitexture)`, "Changes to Section 2.10 (Coordinate Transformations)":
  > "There is also a corresponding texture matrix stack for each texture unit. To change the
  > stack affected by matrix operations, set the active texture unit selector by calling
  > `ActiveTextureARB`… **The selector also affects calls modifying texture environment state,
  > texture coordinate generation state, texture binding state**, and queries of all these state
  > values as well as current texture coordinates and current raster texture coordinates."

* `[G21] §3.8.16 p.192` (Texture Application):
  > "The texture unit selected by ActiveTexture determines which texture unit's environment is
  > modified by TexEnv calls."

* `[C46] §16.2 p.606`:
  > "The texture unit selected by ActiveTexture determines which texture unit's environment is
  > modified by TexEnv calls."

**⇒ Implementation rule for TGLMT:**

```
glTexEnv*(target, pname, param):
    if (target == GL_TEXTURE_ENV):
        unit = ctx->activeTextureUnit;         // NOT a global slot
        write unit.env[pname] = param;
    else if (target == GL_TEXTURE_FILTER_CONTROL): ...
    else if (target == GL_POINT_SPRITE):         ...
```

`glActiveTexture(GL_TEXTUREi)` must therefore switch *which unit's* `GL_TEXTURE_ENV_MODE`,
`GL_TEXTURE_ENV_COLOR`, `GL_COMBINE_*`, `GL_SRC*`, `GL_OPERAND*`, `GL_RGB_SCALE`,
`GL_ALPHA_SCALE` are affected. This is the same slot that `glMultiTexEnv*EXT` writes directly
(see TGLMT `src/gl/gl_ext_dsa_state.cpp`).

### A.1.3 `GL_TEXTURE_ENV_COLOR` and `GL_TEXTURE_ENV_MODE`

These are **pnames**, not targets:

* `GL_TEXTURE_ENV_MODE` (0x2200) — the texture *function* selector for this unit.
* `GL_TEXTURE_ENV_COLOR` (0x2201) — the 4-component RGBA "texture environment color"
  (`Cc`, `Ac` in the spec's notation), used **only** by `GL_BLEND` (and `GL_COMBINE` via
  `GL_CONSTANT` source).

Both are per-texture-unit state. `[C46] Table 23.24` (printed p.752) lists them with the
`2∗×` prefix = per texture unit.

`GL_TEXTURE_ENV_COLOR` semantics:
* Set with 4 floats; **each component clamped to [0,1]** on set ([G21] §3.8.13; [C46] §16.1
  p.599: "TEXTURE_ENV_COLOR is set to an RGBA color by providing four single-precision
  floating-point values in the range [0, 1] (values outside this range are clamped to it)").
* If integers are supplied, they are converted to float per `[G21]` table 2.9 (signed ints) /
  `[C46] §2.2.1` equation 2.2.
* **Initial value: (0, 0, 0, 0)** — `[G21] §3.8.13` and `[C46] Table 23.24`.

`GL_TEXTURE_ENV_MODE` initial value: **`GL_MODULATE`** — `[G21] §3.8.13`; `[C46] Table 23.24`.

### A.1.4 Complete list of legal `GL_TEXTURE_ENV` pnames

`[G21] §3.8.13` p.184 and `[C46] §16.1` p.599:

```
GL_TEXTURE_ENV_MODE
GL_TEXTURE_ENV_COLOR
GL_COMBINE_RGB
GL_COMBINE_ALPHA
GL_RGB_SCALE
GL_ALPHA_SCALE
GL_SRC0_RGB,  GL_SRC1_RGB,  GL_SRC2_RGB
GL_SRC0_ALPHA, GL_SRC1_ALPHA, GL_SRC2_ALPHA
GL_OPERAND0_RGB,  GL_OPERAND1_RGB,  GL_OPERAND2_RGB
GL_OPERAND0_ALPHA, GL_OPERAND1_ALPHA, GL_OPERAND2_ALPHA
```

`GL_SRCn_RGB` / `GL_OPERANDn_RGB` are the *aliases* `GL_SOURCE0_RGB` / `GL_OPERAND0_RGB`
(identical values 0x8580/0x8590; `[REG]` lists `GL_SRC0_RGB` as `alias="GL_SOURCE0_RGB"`).
Both spellings must be accepted. `[C46] Table 23.24` uses the `SRCn_*` spelling;
`[G21] Table 6.19` uses the same.

### A.1.5 Target/pname pairing errors

`[C46] §16.1` "Errors" (printed p.604):

* `GL_INVALID_ENUM` if `target` is not `TEXTURE_FILTER_CONTROL`, `POINT_SPRITE`, or `TEXTURE_ENV`.
* `GL_INVALID_ENUM` if `pname` is not one of the valid parameter names.
* `GL_INVALID_OPERATION` if `pname` is a valid parameter name but **not valid for that target**
  (e.g. `GL_COMBINE_RGB` with `target == GL_POINT_SPRITE`).
* `GL_INVALID_OPERATION` if `param`/`params` is not a valid *value* for `pname`.
* `GL_INVALID_OPERATION` if the active texture unit selected by `ActiveTexture` is
  **≥ `MAX_TEXTURE_UNITS`**.

Note the *value* error is `INVALID_OPERATION` in 4.6 compat (not `INVALID_ENUM`). The
`ARB_texture_env_combine` extension text used `INVALID_ENUM` for bad combine values
([ARB-COMB] "Errors"); the 4.6 compat profile is the later, authoritative wording. **TGLMT should
follow `[C46]`: `GL_INVALID_OPERATION` for an out-of-range enum value passed as `param`.**

`[C46] §16.1.1` (printed p.604) also says `glGetTexEnv*` generates "the same errors generated by
TexEnv* for invalid target and pname parameters … with the corresponding parameters."

---

## A.2 The classic texture functions (REPLACE / MODULATE / DECAL / BLEND / ADD)

### A.2.1 Notation (from `[G21] §3.8.13` p.184)

| Symbol | Meaning |
|---|---|
| `Cf`, `Af` | Primary colour components of the **incoming fragment** (before this unit's texture function). |
| `Cs`, `As` | **Texture source colour** of this unit: derived from the filtered texel values `Rt,Gt,Bt,At,Lt,It` per Table 3.20 ([G21]) / Table 16.1 ([C46]). |
| `Cc`, `Ac` | **Texture environment colour** (`GL_TEXTURE_ENV_COLOR`). |
| `Cp`, `Ap` | Result of the **previous** texture environment. For texture unit 0, `Cp = Cf`, `Ap = Af`. |
| `Cv`, `Av` | The **output** of this unit's texture function. |

`Cx` means "apply the operation to each of R,G,B independently". `Ax` is the alpha, operated on
separately. "[G21] §3.8.13 footnote 3".

**All of these colour values are in the range [0, 1].** ([G21] §3.8.13 p.184) — this sentence is
what makes `GL_ADD`'s saturation mandatory (see D1).

### A.2.2 Texture source colour mapping (Table 3.20 [G21] / Table 16.1 [C46])

| Base internal format | `Cs` | `As` |
|---|---|---|
| `ALPHA` | (0, 0, 0) | `At` |
| `LUMINANCE` | (`Lt`,`Lt`,`Lt`) | 1 |
| `LUMINANCE_ALPHA` | (`Lt`,`Lt`,`Lt`) | `At` |
| `INTENSITY` | (`It`,`It`,`It`) | `It` |
| `RGB` | (`Rt`,`Gt`,`Bt`) | 1 |
| `RGBA` | (`Rt`,`Gt`,`Bt`) | `At` |
| `RED` † | (`Rt`,0,0) | 1 |
| `RG` † | (`Rt`,`Gt`,0) | 1 |

† `RED`/`RG` exist only in the 4.6 compat profile ([C46] Table 16.1 p.600). `LUMINANCE`,
`LUMINANCE_ALPHA`, `ALPHA`, `INTENSITY` are deprecated in core but present in compat.

> ⚠️ The 4.6 compat profile applies **texture swizzle** (`GL_TEXTURE_SWIZZLE_R/G/B/A`) to `Cs`
> *after* this mapping: `[C46] §16.1` p.599–600 — the base colour `Cb`/`Ab` is computed from the
> table, then `Cs`/`As` are swizzled per `GL_TEXTURE_SWIZZLE_*`. TGLMT's legacy path should
> implement the swizzle as an identity unless the swizzle state is non-default.

### A.2.3 REPLACE, MODULATE, DECAL (Table 3.21 [G21] p.185 / Table 16.2 [C46] p.601)

```
                       REPLACE          MODULATE          DECAL
ALPHA                 Cv = Cp          Cv = Cp           undefined
                      Av = As          Av = Ap*As

LUMINANCE             Cv = Cs          Cv = Cp*Cs        undefined
                      Av = Ap          Av = Ap

LUMINANCE_ALPHA       Cv = Cs          Cv = Cp*Cs        undefined
                      Av = As          Av = Ap*As

INTENSITY             Cv = Cs          Cv = Cp*Cs        undefined
                      Av = As          Av = Ap*As

RGB / RG / RED        Cv = Cs          Cv = Cp*Cs        Cv = Cs
                      Av = Ap          Av = Ap           Av = Ap

RGBA                  Cv = Cs          Cv = Cp*Cs        Cv = Cp*(1-As) + Cs*As
                      Av = As          Av = Ap*As        Av = Ap
```

**The exact `GL_DECAL` equation** (RGBA row, the only non-trivial one):

```
Cv = Cp*(1 - As) + Cs*As        (per component R, G, B)
Av = Ap
```

That is: **decal blends the fragment towards the texture using the texture's alpha `As` as the
interpolation weight.** This is a *2-term lerp*, **not** a 3-way `if/else` decision.

**⚠️ The 3-way DECAL equation in the task brief does not exist in OpenGL.** The brief describes:

> "if src.a < 0.5 → src; else if tex.a < 0.5 → tex; else mix"

That is **not** OpenGL's `GL_DECAL`. OpenGL's DECAL (all versions 1.1 → 4.6 compat) is a single
lerp as above, and it is only defined for RGB/RGBA base internal formats; for
ALPHA/LUMINANCE/LUMINANCE_ALPHA/INTENSITY it is literally printed as "**undefined**"
([G21] Table 3.21; [C46] Table 16.2). The 3-way form resembles the 3dfx-vendor-style decal
described in §0.4, not the Khronos specification.

> Note the historical difference in the tables: in **GL 1.1** Table 3.11 the RGBA DECAL row is
> written `Rv = Rf/(1-At) + Rt*At` (an algebraically different but equivalent-to-lerp form; the
> `/` in the OCR is a rendering artefact of the bad PDF font encoding, and the true 1.1 text is
> `Rv = Rf(1-At) + Rt*At`). GL 1.3 onwards writes it as `Cv = Cf(1-As) + Cs*As`. Use the 1.3+
> form.

### A.2.4 BLEND and ADD (Table 3.22 [G21] p.186 / Table 16.3 [C46] p.601)

```
                       BLEND                      ADD
ALPHA                 Cv = Cp                    Cv = Cp
                      Av = Ap*As                 Av = Ap*As

LUMINANCE             Cv = Cp*(1-Cs) + Cc*Cs     Cv = Cp + Cs
                      Av = Ap                    Av = Ap

LUMINANCE_ALPHA       Cv = Cp*(1-Cs) + Cc*Cs     Cv = Cp + Cs
                      Av = Ap*As                 Av = Ap*As

INTENSITY             Cv = Cp*(1-Cs) + Cc*Cs     Cv = Cp + Cs
                      Av = Ap*(1-As) + Ac*As     Av = Ap + As

RGB / RG / RED        Cv = Cp*(1-Cs) + Cc*Cs     Cv = Cp + Cs
                      Av = Ap                    Av = Ap

RGBA                  Cv = Cp*(1-Cs) + Cc*Cs     Cv = Cp + Cs
                      Av = Ap*As                 Av = Ap*As
```

**The brief's stated forms** — `GL_BLEND: dst = (1-src.a)*src + src.a*tex` and
`GL_ADD: dst = src + tex` — are **wrong in their operand roles**. OpenGL's `GL_BLEND` is:

```
Cv = Cp*(1 - Cs) + Cc*Cs
```

i.e. the blend weight is the **texture** colour `Cs`, and the other operand is the **texture
environment colour** `Cc` (`GL_TEXTURE_ENV_COLOR`) — *not* the fragment colour and *not* the
fragment alpha. The fragment colour `Cp` is the *destination*. There is no `src.a` in the
`GL_BLEND` equation at all. (`[G11]` Table 3.11 spells the same thing as
`Rv = Rf*(1-Rt) + Rc*Rt` — fragment, texture, environment colour.)

`GL_ADD` is `Cv = Cp + Cs` and (per D1) **saturates to 1.0**.

### A.2.5 Clamping behaviour per mode — the precise answer

| Mode | Output clamped to [0,1]? | Authority |
|---|---|---|
| `GL_REPLACE` | Values are already in [0,1] by construction. | — |
| `GL_MODULATE` | Products of [0,1] values → already in [0,1]. | — |
| `GL_DECAL` | Lerp of [0,1] values → already in [0,1]. **Undefined** for non-RGB/RGBA formats. | [G21] Table 3.21 |
| `GL_BLEND` | Convex combination of [0,1] values → already in [0,1]. | — |
| `GL_ADD` | **MUST saturate**: `min(1, Cp + Cs)`. | [ARB-ADD] (`Cv = min(1, Cf + Ct)`; revision 0.3 "The addition should saturate to 1."); [G21] "All of these color values are in the range [0, 1]"; [C46] §16.1 p.600 "If fragment color clamping is enabled, all of these color values, including the results, are clamped to the range [0, 1]." |
| `GL_COMBINE` | **Results clamped to [0,1]** after the RGB_SCALE / ALPHA_SCALE multiply. | [G21] §3.8.13 p.184: "The RGB and ALPHA results of the texture function are then multiplied by the values of RGB_SCALE and ALPHA_SCALE, respectively. **The results are clamped to [0, 1].**" |

**Important 4.6-compat nuance:** in `[C46] §16.1` p.600, clamping is conditional on **fragment
color clamping** (`GL_CLAMP_FRAGMENT_COLOR`, `[C46] §13.4.2`):

> "If fragment color clamping is enabled, all of these color values, including the results, are
> clamped to the range [0, 1]. If fragment color clamping is disabled, the values are not
> clamped."

and for `GL_COMBINE` specifically ([C46] p.600):
> "If fragment color clamping is enabled, the arguments and results used in table 16.4 are
> clamped to [0, 1]. Otherwise, the results are unmodified."

`GL_CLAMP_FRAGMENT_COLOR` initial value is **`GL_FIXED_ONLY`** ([C46] Table 23.x) — i.e. clamping
is on for fixed-point colour buffers and off for float buffers. **TGLMT should honour
`GL_CLAMP_FRAGMENT_COLOR` for 4.6-compat behaviour**, but note that GL 1.x/2.1 semantics
(`[G21]`) always clamp, so a compatibility-profile implementation targeting older apps should
clamp by default.

### A.2.6 Implementable pseudocode (classic modes)

```cpp
// Called per texture unit, in unit order 0..N-1.
// in : Cf,Af  (primary fragment colour, already colour-summed? NO - see A.5)
//      Cs,As  (this unit's texture source colour, per Table 3.20/16.1)
//      Cc,Ac  (GL_TEXTURE_ENV_COLOR of this unit)
//      Cp,Ap  (previous unit's output; for unit 0 == Cf,Af)
// out: Cv,Av
void TexFuncClassic(GLenum mode, GLfloat Cf[3], GLfloat Af,
                    GLfloat Cs[3], GLfloat As,
                    GLfloat Cc[3], GLfloat Ac,
                    GLfloat Cp[3], GLfloat Ap,
                    GLfloat Cv[3], GLfloat& Av)
{
    switch (mode) {
    case GL_REPLACE:
        Cv[0..2] = Cs[0..2];
        Av = (baseIsAlphaLike) ? As : Ap;   // ALPHA -> As ; LUMINANCE -> Ap ;
                                          // LUMINANCE_ALPHA -> As ; INTENSITY -> As ;
                                          // RGB -> Ap ; RGBA -> As
        break;
    case GL_MODULATE:
        // ALPHA: Cv = Cp, Av = Ap*As ; LUMINANCE: Cv = Cp*Cs, Av = Ap
        // LUMINANCE_ALPHA: Cv = Cp*Cs, Av = Ap*As ; INTENSITY: Cv = Cp*Cs, Av = Ap*As
        // RGB: Cv = Cp*Cs, Av = Ap ; RGBA: Cv = Cp*Cs, Av = Ap*As
        break;
    case GL_DECAL:
        // Only defined for RGB/RGBA: Cv = Cp*(1-As) + Cs*As ; Av = Ap
        // Otherwise: undefined -> TGLMT choice D2: leave colour unchanged.
        break;
    case GL_BLEND:
        // Cv = Cp*(1-Cs) + Cc*Cs ; alpha per table above
        break;
    case GL_ADD:
        // Cv = min(1, Cp + Cs) ; alpha per table above
        break;
    }
}
```

**Implementable per-format alpha table** (avoids branching on the format in the shader):

| base format | REPLACE Av | MODULATE Av | DECAL Av | BLEND Av | ADD Av |
|---|---|---|---|---|---|
| ALPHA | `As` | `Ap*As` | undefined | `Ap*As` | `Ap*As` |
| LUMINANCE | `Ap` | `Ap` | undefined | `Ap` | `Ap` |
| LUMINANCE_ALPHA | `As` | `Ap*As` | undefined | `Ap*As` | `Ap*As` |
| INTENSITY | `As` | `Ap*As` | undefined | `Ap*(1-As)+Ac*As` | `Ap+As` |
| RGB/RG/RED | `Ap` | `Ap` | `Ap` | `Ap` | `Ap` |
| RGBA | `As` | `Ap*As` | `Ap` | `Ap*As` | `Ap*As` |

RGB-column rule for every mode:
* REPLACE → `Cs`
* MODULATE → `Cp*Cs`
* DECAL → `Cp*(1-As)+Cs*As` (RGB/RGBA only)
* BLEND → `Cp*(1-Cs)+Cc*Cs`
* ADD → `min(1, Cp+Cs)`

### A.2.7 Where the texture function sits in the pipeline

`[C46] §16.1` / `[C46] §16.2` (Texture Application) / `[C46] §16.3` (Color Sum) /
`[C46] §16.4` (Fog) / `[C46] Chapter 17` (Per-Fragment Operations).

```
per-vertex:  lighting → colour sum inputs
per-fragment:
   [texgen] → texture coordinate transform (texture matrix)
   [texturing] texture unit 0 env function → unit 1 env function → ... → unit N-1
   [color sum]  (§16.3)  primary + secondary → post-texturing colour
   [fog]        (§16.4)
   --- chapter 17 ---
   [antialiasing §17.1] → [multisample point fade §17.2] → [per-fragment ops §17.3:
       pixel ownership, scissor, multisample, alpha test, depth test, stencil, blending,
       dithering, logic op, colour mask] → framebuffer
```

`[G21] §3.8.16` p.192 states: "The result of this function replaces the incoming fragment's
primary R, G, and B, and A values. These are the color values passed to subsequent operations."

**Important:** texture environment operates on the fragment's **primary** colour only. The
**secondary** colour (`csec`) is untouched by texturing; it is added later at color sum
([C46] §16.3: "a fragment has two RGBA colors: a primary color cpri (which texturing, if
enabled, may have modified) and a secondary color csec"). If lighting is enabled and no vertex
processing stage is active, color sum is *always* applied regardless of `GL_COLOR_SUM`
([C46] §16.3).

### A.2.8 Multitexture chaining

`[C46] §16.2` p.606 and figure 16.1:

```
Cf  --TE0--> --TE1--> --TE2--> --TE3-->  C'f
```

* Unit *i*'s `Cp,Ap` = unit *i-1*'s `Cv,Av`.
* For unit 0, `Cp,Ap` = `Cf,Af`.
* If texturing is disabled for a unit, the fragment passes **unaltered** to the next unit.
* "If a texture unit is disabled or has an invalid or incomplete texture … bound to it, then
  blending is disabled for that texture unit. If the texture environment for a given enabled
  texture unit references a disabled texture unit, or an invalid or incomplete texture that is
  bound to another unit, then the results of texture blending are undefined."
  ([C46] §16.2; [G21] §3.8.16 — same wording.)

**TGLMT rule:** a unit with no complete texture contributes `Cp,Ap` unchanged (i.e. `Cs`/`As` are
irrelevant for that unit). But if a unit's *environment* references another unit's texture
(e.g. `GL_SRC0_RGB = GL_TEXTURE1`), and that other unit is disabled/incomplete, the result is
**undefined** — TGLMT may substitute `Ct = (0,0,0)`, `At = 1` (the "default texture" values) or
skip the reference; document the choice.

---

## A.3 `GL_COMBINE` — the real OpenGL combiner

### A.3.1 The combine function set (`COMBINE_RGB` / `COMBINE_ALPHA`)

From `[ARB-COMB]` "New Tokens", `[G13] §3.8.12` Table 3.24, `[G21] §3.8.13` Table 3.23,
`[C46] §16.1` Table 16.4 (printed p.602):

| `GL_COMBINE_RGB` / `GL_COMBINE_ALPHA` value | Texture function | Token |
|---|---|---|
| `GL_REPLACE` | `Arg0` | 0x1E01 |
| `GL_MODULATE` | `Arg0 * Arg1` | 0x2100 |
| `GL_ADD` | `Arg0 + Arg1` | 0x0104 |
| `GL_ADD_SIGNED` | `Arg0 + Arg1 - 0.5` | 0x8574 |
| `GL_INTERPOLATE` | `Arg0*Arg2 + Arg1*(1 - Arg2)` | 0x8575 |
| `GL_SUBTRACT` | `Arg0 - Arg1` | 0x84E7 |
| `GL_DOT3_RGB` | `4*((Arg0r-0.5)*(Arg1r-0.5) + (Arg0g-0.5)*(Arg1g-0.5) + (Arg0b-0.5)*(Arg1b-0.5))` | 0x86AE |
| `GL_DOT3_RGBA` | same scalar, into r,g,b,**a**; the `COMBINE_ALPHA` result is ignored | 0x86AF |

Verbatim from `[G21] §3.8.13` Table 3.23 (p.187–188) / `[C46]` Table 16.4 (p.602):

```
COMBINE_RGB                          Texture Function
REPLACE                              Arg0
MODULATE                             Arg0 * Arg1
ADD                                  Arg0 + Arg1
ADD_SIGNED                           Arg0 + Arg1 - 0.5
INTERPOLATE                          Arg0*Arg2 + Arg1*(1 - Arg2)
SUBTRACT                             Arg0 - Arg1
DOT3_RGB   4*((Arg0r-0.5)*(Arg1r-0.5) + (Arg0g-0.5)*(Arg1g-0.5) + (Arg0b-0.5)*(Arg1b-0.5))
DOT3_RGBA  4*((Arg0r-0.5)*(Arg1r-0.5) + (Arg0g-0.5)*(Arg1g-0.5) + (Arg0b-0.5)*(Arg1b-0.5))

COMBINE_ALPHA                        Texture Function
REPLACE                              Arg0
MODULATE                             Arg0 * Arg1
ADD                                  Arg0 + Arg1
ADD_SIGNED                           Arg0 + Arg1 - 0.5
INTERPOLATE                          Arg0*Arg2 + Arg1*(1 - Arg2)
SUBTRACT                             Arg0 - Arg1
```

Notes:
* `DOT3_RGB` places the scalar into **all three** of r,g,b.
* `DOT3_RGBA` places the scalar into **all four** of r,g,b,a — **the `COMBINE_ALPHA` result is
  discarded** ([G21] Table 3.23 caption: "The result generated from COMBINE_ALPHA is ignored for
  DOT3_RGBA"; [ARB-DOT3] "Note that the result generated from COMBINE_ALPHA_ARB function is
  ignored.").
* `DOT3_RGB`/`DOT3_RGBA` are only legal for `COMBINE_RGB`. `ARB_texture_env_dot3` "Errors":
  `INVALID_ENUM` if the `COMBINE_RGB_ARB` value is not one of `REPLACE, MODULATE, ADD,
  ADD_SIGNED_ARB, INTERPOLATE_ARB, SUBTRACT_ARB, DOT3_RGB_ARB or DOT3_RGBA_ARB`.
* `GL_SUBTRACT` came from `ARB_texture_env_combine` (0x84E7). `GL_ADD_SIGNED` = 0x8574.
* `EXT_texture_env_combine` ([EXT-COMB]) is the *older, weaker* variant: no `SUBTRACT`,
  `OPERAND2_RGB`/`OPERAND2_ALPHA` are restricted to `SRC_ALPHA` only, and the source set is
  `TEXTURE / CONSTANT / PRIMARY_COLOR / PREVIOUS`. **OpenGL 1.3+ uses the ARB form.**

### A.3.2 Source arguments (`SRCn_RGB` / `SRCn_ALPHA`)

From `[ARB-COMB]` Table 3.21/3.22, `[G21] §3.8.13` Tables 3.24/3.25, `[C46] §16.1` Tables
16.5/16.6 (printed p.603):

`SRCn_RGB` (n = 0,1,2) may be:

| Value | Meaning |
|---|---|
| `GL_TEXTURE` (0x1702) | This unit's own texture source colour `Cs` (RGB) / `As` (alpha). |
| `GL_CONSTANT` (0x8576) | The texture environment colour `Cc` / `Ac`. |
| `GL_PRIMARY_COLOR` (0x8577) | The **primary** colour of the incoming fragment `Cf` / `Af`. |
| `GL_PREVIOUS` (0x8578) | The result of the previous texture environment `Cp` / `Ap`. On texture unit 0 this maps to `PRIMARY_COLOR` (`Cp,Ap` = `Cf,Af`). |

`SRCn_ALPHA` (n = 0,1,2) may be: `GL_TEXTURE`, `GL_CONSTANT`, `GL_PRIMARY_COLOR`,
`GL_PREVIOUS` — the same four, but always yielding the **alpha** component.

Extensions:
* `ARB_texture_env_crossbar` ([ARB-XBAR]) adds `GL_TEXTURE0 .. GL_TEXTURE<n>` (0x84C0 + n) as
  values for `SRCn_RGB`/`SRCn_ALPHA`, meaning "the texture source colour of texture unit *n*".
  OpenGL 1.4+ supports this; `GL_TEXTURE` remains for backward compatibility.
  ⇒ `[C46] §16.2` p.606: "If TEXTUREn is specified as SRCn_RGB or SRCn_ALPHA, the texture value
  from texture unit n will be used in computing the texture function for this texture unit."
* `NV_texture_env_combine4` ([NV-COMB4]) adds `GL_ZERO` and `GL_TEXTURE<n>_ARB` as source
  values, plus a 4th source (`SOURCE3_RGB_NV`/`SOURCE3_ALPHA_NV`) and 4th operand
  (`OPERAND3_RGB_NV`/`OPERAND3_ALPHA_NV`) with a `GL_COMBINE4_NV` mode
  (`Arg0*Arg1 + Arg2*Arg3`). **Not core; optional for TGLMT.**

### A.3.3 Operand arguments (`OPERANDn_RGB` / `OPERANDn_ALPHA`)

From `[ARB-COMB]`, `[G21] Tables 3.24/3.25`, `[C46] Tables 16.5/16.6`:

`OPERANDn_RGB` (n = 0,1,2) may be:

| Value | Result (RGB) |
|---|---|
| `GL_SRC_COLOR` (0x0300) | the source's RGB |
| `GL_ONE_MINUS_SRC_COLOR` (0x0301) | `1 - source's RGB` |
| `GL_SRC_ALPHA` (0x0302) | the source's alpha (replicated to r,g,b) |
| `GL_ONE_MINUS_SRC_ALPHA` (0x0303) | `1 - source's alpha` (replicated) |

`OPERANDn_ALPHA` (n = 0,1,2) may be: `GL_SRC_ALPHA`, `GL_ONE_MINUS_SRC_ALPHA` only.

> Note: `GL_SRC_ALPHA` in an RGB operand context yields a *scalar* (the source alpha), which is
> then used for all three components. This is the classic way to get a lerp weight.

### A.3.4 The exact argument resolution tables

`[C46] §16.1` Table 16.5 (p.603) — **COMBINE_RGB** arguments:

```
SRCn_RGB      OPERANDn_RGB        Argument
TEXTURE       SRC_COLOR           Cs
              ONE_MINUS_SRC_COLOR (1 - Cs)
              SRC_ALPHA           As
              ONE_MINUS_SRC_ALPHA (1 - As)

CONSTANT      SRC_COLOR           Cc
              ONE_MINUS_SRC_COLOR (1 - Cc)
              SRC_ALPHA           Ac
              ONE_MINUS_SRC_ALPHA (1 - Ac)

PRIMARY_COLOR SRC_COLOR           Cf
              ONE_MINUS_SRC_COLOR (1 - Cf)
              SRC_ALPHA           Af
              ONE_MINUS_SRC_ALPHA (1 - Af)

PREVIOUS      SRC_COLOR           Cp
              ONE_MINUS_SRC_COLOR (1 - Cp)
              SRC_ALPHA           Ap
              ONE_MINUS_SRC_ALPHA (1 - Ap)
```

`[C46] §16.1` Table 16.6 (p.603) — **COMBINE_ALPHA** arguments:

```
SRCn_ALPHA    OPERANDn_ALPHA      Argument
TEXTURE       SRC_ALPHA           As
              ONE_MINUS_SRC_ALPHA (1 - As)

CONSTANT      SRC_ALPHA           Ac
              ONE_MINUS_SRC_ALPHA (1 - Ac)

PRIMARY_COLOR SRC_ALPHA           Af
              ONE_MINUS_SRC_ALPHA (1 - Af)

PREVIOUS      SRC_ALPHA           Ap
              ONE_MINUS_SRC_ALPHA (1 - Ap)
```

Note the difference between `TEXTURE`/`PREVIOUS` in the RGB table: `TEXTURE` maps to the *current
unit's* `Cs`,`As`; `PREVIOUS` maps to `Cp`,`Ap` (previous unit's output).

### A.3.5 The exact GL_COMBINE algorithm — implementable

```cpp
struct CombineArgs {
    GLfloat rgb[3];   // Arg0/1/2 for the RGB combiner
    GLfloat a;        // Arg for the alpha combiner
};

// Resolve one argument (source enum + operand enum) for a given "source triple".
//   sourceRGB[3] and sourceA describe the RGB source (Cs/As, Cc/Ac, Cf/Af, Cp/Ap)
//   sourceAonly describes the alpha source (As, Ac, Af, Ap)
static inline void ResolveArg(GLenum src, GLenum operand,
                              const GLfloat sourceRGB[3], GLfloat sourceA,
                              GLfloat outRGB[3], GLfloat& outA)
{
    switch (operand) {
    case GL_SRC_COLOR:
        outRGB[0]=sourceRGB[0]; outRGB[1]=sourceRGB[1]; outRGB[2]=sourceRGB[2];
        outA = sourceA; break;
    case GL_ONE_MINUS_SRC_COLOR:
        outRGB[0]=1-sourceRGB[0]; outRGB[1]=1-sourceRGB[1]; outRGB[2]=1-sourceRGB[2];
        outA = 1-sourceA; break;
    case GL_SRC_ALPHA:
        outRGB[0]=outRGB[1]=outRGB[2]=sourceA; outA = sourceA; break;
    case GL_ONE_MINUS_SRC_ALPHA:
        outRGB[0]=outRGB[1]=outRGB[2]=1-sourceA; outA = 1-sourceA; break;
    }
}

// The per-source triples (for n = 0,1,2):
//   sourceRGB / sourceA  come from SRCn_RGB / SRCn_ALPHA respectively:
//     GL_TEXTURE       -> (Cs,As)
//     GL_CONSTANT      -> (Cc,Ac)
//     GL_PRIMARY_COLOR -> (Cf,Af)
//     GL_PREVIOUS      -> (Cp,Ap)
//     GL_TEXTUREi      -> (Csi,Asi)          [ARB_texture_env_crossbar]
//   For the ALPHA combiner, only SRCn_ALPHA matters and it resolves to the source's alpha.

static GLfloat CombineEval(GLenum func, GLfloat A0, GLfloat A1, GLfloat A2)
{
    switch (func) {
    case GL_REPLACE:    return A0;
    case GL_MODULATE:   return A0 * A1;
    case GL_ADD:        return A0 + A1;
    case GL_ADD_SIGNED: return A0 + A1 - 0.5f;
    case GL_INTERPOLATE:return A0 * A2 + A1 * (1.0f - A2);
    case GL_SUBTRACT:   return A0 - A1;
    default:            return A0; // undefined
    }
}
```

Full per-unit COMBINE evaluation:

```cpp
void TexFuncCombine(const TexEnvState& env,
                    GLfloat Cf[3], GLfloat Af,
                    GLfloat Cs[3], GLfloat As,
                    GLfloat Cc[3], GLfloat Ac,
                    GLfloat Cp[3], GLfloat Ap,
                    GLfloat Cv[3], GLfloat& Av)
{
    // --- resolve Arg0, Arg1, Arg2 for RGB ---
    GLfloat argR[3][3], argA[3];
    for (int n = 0; n < 3; ++n) {
        const GLfloat* sRGB; GLfloat sA;
        switch (env.srcRGB[n]) {
        case GL_TEXTURE:       sRGB = Cs; sA = As; break;
        case GL_CONSTANT:      sRGB = Cc; sA = Ac; break;
        case GL_PRIMARY_COLOR: sRGB = Cf; sA = Af; break;
        case GL_PREVIOUS:      sRGB = Cp; sA = Ap; break;
        default:               sRGB = Cs; sA = As; break; // TEXTUREi, undefined
        }
        ResolveArg(env.srcRGB[n], env.operandRGB[n], sRGB, sA, argR[n], argA[n]);

        // separate alpha-source triple for the ALPHA combiner
        GLfloat sAonly;
        switch (env.srcAlpha[n]) {
        case GL_TEXTURE:       sAonly = As; break;
        case GL_CONSTANT:      sAonly = Ac; break;
        case GL_PRIMARY_COLOR: sAonly = Af; break;
        case GL_PREVIOUS:      sAonly = Ap; break;
        default:               sAonly = As; break;
        }
        GLfloat dummyRGB[3];
        ResolveArg(env.srcAlpha[n], env.operandAlpha[n], &sAonly /*unused*/, sAonly,
                   dummyRGB, argA[n]);
    }

    // --- DOT3 needs the raw Arg0/Arg1 components, not scalars ---
    if (env.combineRGB == GL_DOT3_RGB || env.combineRGB == GL_DOT3_RGBA) {
        GLfloat d = 4.0f * ((argR[0][0]-0.5f)*(argR[1][0]-0.5f)
                          + (argR[0][1]-0.5f)*(argR[1][1]-0.5f)
                          + (argR[0][2]-0.5f)*(argR[1][2]-0.5f));
        Cv[0]=Cv[1]=Cv[2]=d;
        if (env.combineRGB == GL_DOT3_RGBA) { Av = d; }
        else { Av = CombineEval(env.combineAlpha, argA[0], argA[1], argA[2]); }
        return;
    }

    // --- RGB ---
    for (int c = 0; c < 3; ++c)
        Cv[c] = CombineEval(env.combineRGB, argR[0][c], argR[1][c], argR[2][c]);

    // --- ALPHA ---
    Av = CombineEval(env.combineAlpha, argA[0], argA[1], argA[2]);

    // --- post-combiner scale, then clamp ---
    const GLfloat rgbScale = env.rgbScale;   // 1.0, 2.0 or 4.0
    const GLfloat alphaScale = env.alphaScale;
    for (int c = 0; c < 3; ++c) Cv[c] *= rgbScale;
    Av *= alphaScale;

    // "[G21] §3.8.13: The results are clamped to [0, 1]."
    if (fragmentColorClampingEnabled) {   // 4.6 compat: GL_CLAMP_FRAGMENT_COLOR
        for (int c = 0; c < 3; ++c) Cv[c] = clamp01(Cv[c]);
        Av = clamp01(Av);
    }
}
```

**A subtlety worth getting right:** the `Arg2` used by `GL_INTERPOLATE` comes from the
**RGB** argument table when computing the RGB result, and from the **ALPHA** argument table when
computing the alpha result. i.e. `Arg2` for RGB is `ResolveArg(SRC2_RGB, OPERAND2_RGB, …)` and
`Arg2` for alpha is `ResolveArg(SRC2_ALPHA, OPERAND2_ALPHA, …)`. They are **different values**.

Also: for the alpha combiner, `Arg0`/`Arg1`/`Arg2` are the alpha of the resolved operand. Since
the alpha operand table only offers `SRC_ALPHA` / `ONE_MINUS_SRC_ALPHA`, `Arg0_alpha` is
`As`/`(1-As)` etc. — never a colour.

### A.3.6 `GL_RGB_SCALE` / `GL_ALPHA_SCALE`

From `[ARB-COMB]` "New Tokens": accepted values are **1.0, 2.0, 4.0** only.
`INVALID_VALUE` otherwise. Initial value **1.0** for both ([ARB-COMB] "New State";
`[G21] Table 6.19`; `[C46] Table 23.25`).

They multiply the RGB and ALPHA results **after** the combine function, before clamping.

### A.3.7 Initial state of the texture environment (per texture unit)

`[G21] §3.8.13` p.186–187 and `[C46] §16.1` p.604:

```
TEXTURE_ENV_MODE   = GL_MODULATE
COMBINE_RGB        = GL_MODULATE
COMBINE_ALPHA      = GL_MODULATE
SRC0_RGB           = GL_TEXTURE
SRC1_RGB           = GL_PREVIOUS
SRC2_RGB           = GL_CONSTANT
SRC0_ALPHA         = GL_TEXTURE
SRC1_ALPHA         = GL_PREVIOUS
SRC2_ALPHA         = GL_CONSTANT
OPERAND0_RGB       = GL_SRC_COLOR
OPERAND1_RGB       = GL_SRC_COLOR
OPERAND2_RGB       = GL_SRC_ALPHA
OPERAND0_ALPHA     = GL_SRC_ALPHA
OPERAND1_ALPHA     = GL_SRC_ALPHA
OPERAND2_ALPHA     = GL_SRC_ALPHA
RGB_SCALE          = 1.0
ALPHA_SCALE        = 1.0
TEXTURE_ENV_COLOR  = (0, 0, 0, 0)
TEXTURE_LOD_BIAS   = 0.0
```

(The 2.1 text says "the combiner RGB and ALPHA sources are each TEXTURE, PREVIOUS, and CONSTANT
for sources 0, 1, and 2 respectively".)

### A.3.8 `GL_COMBINE` interaction with `GL_SRC0_RGB = GL_TEXTURE0` etc.

When `SRCn_RGB` is `GL_TEXTUREi` (crossbar), the referenced texture unit's *own* texture source
colour is used — i.e. the unit's `Cs`/`As` computed from **unit i's** bound texture, with unit
i's own `GL_TEXTURE_ENV_COLOR`? No: `Ct<n>`/`At<n>` are "the filtered texture RGB and alpha
values from the texture bound to texture unit <n>" ([ARB-XBAR] Table 3.21 addition). So it is the
**raw filtered texture value** of unit *n*, not unit *n*'s combiner output. `GL_PREVIOUS` is the
only way to reach another unit's *output*.

### A.3.9 ⚠️ Where the brief's A.3/A.4 token list belongs (not OpenGL)

| Brief's requested token | Reality |
|---|---|
| combine fn `GL_REVERSE_SUBTRACT` | Not a texture-env combine fn, and not even a registry token under that name. `GL_FUNC_REVERSE_SUBTRACT` (0x800B) is an **`EXT_blend_subtract` blend equation**. |
| combine fn `GL_MIN` / `GL_MAX` | `EXT_blend_minmax` **blend equations** (0x8007/0x8008). |
| combine fn `GL_MULTIPLY` | `EXT_blend_equation`-era token (0x9294). |
| combine fn `GL_SRC_ALPHA` / `GL_ONE_MINUS_SRC_ALPHA` / `GL_SRC_ALPHA_SATURATE` | **Blending factors**, not combine functions. (`GL_SRC_ALPHA` 0x0302 happens to also be a legal *operand* value — different role.) |
| operand `GL_DST_COLOR` / `GL_ONE_MINUS_DST_COLOR` / `GL_DST_ALPHA` / `GL_ONE_MINUS_DST_ALPHA` | **Blending factors**. Never valid for `glTexEnv`. |
| operand `GL_SRC0_COLOR` .. `GL_SRC2_COLOR` | **Do not exist** in the OpenGL registry ([REG]: 0 hits). |
| source `GL_SOURCE0_RGB` etc. | ✅ These *do* exist (0x8580–0x858A) — but their *values* are `TEXTURE/CONSTANT/PRIMARY_COLOR/PREVIOUS`, and there is no `A0/A1/A2`, `C0/C1/C2` formulation anywhere in OpenGL. |
| "A0/A1/A2 and C0/C1/C2" | 3dfx-style vendor combiner, not OpenGL. |

**TGLMT must implement the OpenGL ARB form above.** A driver that receives
`glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MIN)` must generate `GL_INVALID_OPERATION`
([C46] §16.1: "INVALID_OPERATION error is generated if param or params are not valid values for
pname") — it must **not** silently treat it as a blend equation.

---

## A.4 `glTexGen` — texture coordinate generation

### A.4.1 Command signature and legal arguments

```c
void glTexGenf (GLenum coord, GLenum pname, GLfloat param);
void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
void glTexGend (GLenum coord, GLenum pname, GLdouble param);
void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params);
void glTexGeni (GLenum coord, GLenum pname, GLint param);
void glTexGeniv(GLenum coord, GLenum pname, const GLint *params);
```

* `coord` ∈ { `GL_S`, `GL_T`, `GL_R`, `GL_Q` } — selects **which** texture coordinate.
* `pname` ∈ { `GL_TEXTURE_GEN_MODE`, `GL_OBJECT_PLANE`, `GL_EYE_PLANE` } — **the complete list**
  in every version 1.1 → 4.6 compat.
* `GL_TEXTURE_GEN_MODE` value ∈ { `GL_OBJECT_LINEAR`, `GL_EYE_LINEAR`, `GL_SPHERE_MAP`,
  `GL_REFLECTION_MAP`, `GL_NORMAL_MAP` }.

Sources: `[G11] §2.9.4`; `[G14] §2.10.4`; `[G21] §2.11.4`; `[C46] §12.1.3` p.507–508.

### A.4.2 State layout

`[G21] §2.11.4` p.52 and `[C46] §12.1.3` p.509:

> "The state required for texture coordinate generation **for each texture unit** comprises a
> five-valued integer for each coordinate indicating coordinate generation mode, and a bit for
> each coordinate to indicate whether texture coordinate generation is enabled or disabled. In
> addition, four coefficients are required for the four coordinates for each of EYE_LINEAR and
> OBJECT_LINEAR."

```
struct TexGenState {           // per texture unit
    GLenum  mode[4];           // indexed by {S,T,R,Q} -> GL_OBJECT_LINEAR | GL_EYE_LINEAR |
                                //   GL_SPHERE_MAP | GL_REFLECTION_MAP | GL_NORMAL_MAP
    GLfloat objPlane[4][4];    // p1..p4 per coordinate, for OBJECT_LINEAR
    GLfloat eyePlane[4][4];    // p'1..p'4 per coordinate, for EYE_LINEAR (see A.7)
    GLboolean enabled[4];      // GL_TEXTURE_GEN_S/T/R/Q enables
};
```

**Initial values** ([G21] §2.11.4 p.52; [C46] §12.1.3 p.509):

* All generation **disabled** for all coordinates.
* `p` for **s**: all 0 except `p1 = 1`.
* `p` for **t**: all 0 except `p2 = 1`.
* `p` for **r** and **q**: all 0.
* These apply for both the EYE_LINEAR and OBJECT_LINEAR versions.
* **All texture generation modes are initially `GL_EYE_LINEAR`.**

Enables use `glEnable`/`glDisable` with `GL_TEXTURE_GEN_S` (0x0C60), `GL_TEXTURE_GEN_T` (0x0C61),
`GL_TEXTURE_GEN_R` (0x0C62), `GL_TEXTURE_GEN_Q` (0x0C63) — per texture unit, and they respond to
`glActiveTexture` ([G121] F.2; [C46] §12.1.1).

### A.4.3 `GL_OBJECT_LINEAR` — exact equation

`[G21] §2.11.4` p.50:

```
g = p1*xo + p2*yo + p3*zo + p4*wo
```

where `(xo, yo, zo, wo)` are the **object coordinates** of the vertex, and `p1..p4` are the
coefficients set by `glTexGenfv(GL_S|GL_T|GL_R|GL_Q, GL_OBJECT_PLANE, params)`.

"There is a distinct group of plane equation coefficients for each texture coordinate; `coord`
indicates the coordinate to which the specified coefficients pertain."

### A.4.4 `GL_EYE_LINEAR` — exact equation

`[G21] §2.11.4` p.50–51:

```
g = p'1*xe + p'2*ye + p'3*ze + p'4*we

where (p'1 p'2 p'3 p'4) = (p1 p2 p3 p4) M^-1
```

* `(xe, ye, ze, we)` = **eye coordinates** of the vertex.
* `p1..p4` are the coefficients set by `glTexGenfv(coord, GL_EYE_PLANE, params)` — i.e. the
  **same 4 numbers** you'd pass to `GL_OBJECT_PLANE`; the eye-space version is derived.
* **`M` is the model-view matrix in effect when `p1..p4` are specified** — i.e. the transform is
  performed **at the time of the `glTexGenfv` call**, not at draw time.
* "Computed texture coordinates may be inaccurate or undefined if M is poorly conditioned or
  singular."

### A.4.5 The three remaining modes

`[G21] §2.11.4` p.51; `[C46] §12.1.3` p.508–509:

* **`GL_SPHERE_MAP`** — see §A.6.
* **`GL_REFLECTION_MAP`** — compute the reflection vector `r` exactly as for SPHERE_MAP, then
  `s = rx`, `t = ry`, `r = rz`. (Note: `r` here is the **reflection vector's z**, not the texture
  r coordinate — the generated *texture* r coordinate is set to `rz`.)
* **`GL_NORMAL_MAP`** — compute the eye-space normal `nf` as in the normal-transformation rules
  (`[G21] §2.11.3` / `[C46] §12.1.2`), then `s = nfx`, `t = nfy`, `r = nfz`.

### A.4.6 Errors

`[C46] §12.1.3` "Errors" (printed p.509) — the authoritative list:

* `GL_INVALID_ENUM` if `coord` is not `GL_S`, `GL_T`, `GL_R`, or `GL_Q`.
* `GL_INVALID_ENUM` if `pname` is not `GL_TEXTURE_GEN_MODE`, `GL_OBJECT_PLANE`, or `GL_EYE_PLANE`.
* `GL_INVALID_ENUM` if `coord` is either `R` or `Q`, and the mode is `GL_SPHERE_MAP`.
* `GL_INVALID_ENUM` if `coord` is `Q` and the mode is `GL_REFLECTION_MAP`.
* `GL_INVALID_ENUM` if `coord` is `Q` and the mode is `GL_NORMAL_MAP`.

`[G21]`/`[G14]`/`[G11]` express the same rules in prose ("Calling TexGen with a coord of either R
or Q when pname indicates SPHERE MAP generates the error INVALID_ENUM.").

`glGetTexGen*` generates "the same errors generated by TexGen* for invalid coord and pname
parameters … with the corresponding parameters" ([C46] §12.1.3.1 p.509).

### A.4.7 `glGetTexGen*` semantics — the EYE/OBJECT plane asymmetry

`[C46] §12.1.3.1` (printed p.509) and `[G21] §2.11.4`:

> "EYE_LINEAR coefficients are returned in the **eye coordinates** that were computed when the
> plane was specified; OBJECT_LINEAR coefficients are returned in **object coordinates**."

So:
* `glGetTexGenfv(GL_S, GL_OBJECT_PLANE, p)` returns the raw `p1..p4` you passed to
  `glTexGenfv(GL_S, GL_OBJECT_PLANE, ...)`.
* `glGetTexGenfv(GL_S, GL_EYE_PLANE, p)` returns the **eye-space** coefficients `p'1..p'4`
  (= `p · M^-1` computed at spec time), *not* the raw values you passed.

**This is a real trap.** A round-trip test `set → get` on `GL_EYE_PLANE` will *not* return the
input unless the model-view matrix is the identity at the time of the call. TGLMT must store
**both** the raw object-space plane and the derived eye-space plane.

### A.4.8 Where the generated coordinate goes

`[G21] §2.11.2` p.46 and `[C46] §12.1.1` p.504:

```
[s t r q]' = M_texture * [s t r q]
```

> "The matrix is applied to the coordinates resulting from texture coordinate generation (which
> may simply be the current texture coordinates), and the resulting transformed coordinates
> become the texture coordinates associated with a vertex."

So the order is: **texgen → texture matrix**. If texgen is disabled for a coordinate, the
*current* texture coordinate (from `glTexCoord*` / `glMultiTexCoord*`) is used instead, and is
still multiplied by the texture matrix.

---

## A.5 The texture matrix (GL_TEXTURE_MATRIX) — per-unit, not shared

### A.5.1 The answer

**Per-texture-unit.** Each texture unit has its **own** texture matrix stack, and the stack
affected by matrix operations is the one belonging to the **currently active texture unit**.

Verbatim, `[G21] §2.11.2` p.47:

> "There is a stack of matrices for each of matrix modes MODELVIEW, PROJECTION, and COLOR, **and
> for each texture unit**. For MODELVIEW mode, the stack depth is at least 32 … For the other
> modes, the depth is at least 2. **Texture matrix stacks for all texture units have the same
> depth.** The current matrix in any mode is the matrix on the top of the stack for that mode."

and p.48:

> "When the current matrix mode is TEXTURE, **the texture matrix stack of the active texture unit
> is pushed or popped**."

Identical text in `[C46] §12.1.1` p.505, `[G14] §2.10.2` p.37–38, `[G13] §2.10` p.35, and
`[G121] Appendix F.2` p.245:

> "There is also a corresponding texture matrix stack for each texture unit. To change the stack
> affected by matrix operations, set the active texture unit selector by calling
> `ActiveTextureARB`."

### A.5.2 The historical gotcha, precisely

* **GL 1.1 (pre-multitexture)**: `glMatrixMode(GL_TEXTURE)` selected **one single** TEXTURE
  matrix stack, shared by all texture units — but GL 1.1 had only *one* texture unit anyway
  ([G11] §2.10.2: "There is a stack of matrices for each of the matrix modes … MODELVIEW,
  PROJECTION and TEXTURE").
* **GL 1.2.1 / 1.3+ (with `ARB_multitexture`)**: per-unit texture matrix stacks. This is the
  change that bites people porting GL 1.x code.

**So: the classic gotcha is "is it one shared stack or per-unit?" — the answer in every OpenGL
version with multitexture (1.2.1 onward, including 2.1 and 4.6 compat) is PER-UNIT.**
The pre-multitexture single stack is GL 1.0/1.1 only.

### A.5.3 Subtlety: texture *coordinate sets* vs texture *image units*

In OpenGL 1.3/1.4/2.1 and 4.6 compat, a "texture unit" is conceptually split:

* a **texture coordinate processing unit** = texture matrix stack + texture coordinate generation
  state — bounded by `MAX_TEXTURE_COORDS`;
* a **texture image unit** = all the texture state (bound textures, texenv, texparam, …) —
  bounded by `MAX_COMBINED_TEXTURE_IMAGE_UNITS`.

`[G21] §2.11.2` p.46:

> "Each texture unit contains up to two distinct sub-units: a texture coordinate processing unit
> (consisting of a texture matrix stack and texture coordinate generation state) and a texture
> image unit (consisting of all the texture state defined in section 3.8). In implementations with
> a different number of supported texture coordinate sets and texture image units, some texture
> units may consist of only one of the two sub-units."

`[C46] §8` p.179–180 states the same, and adds:

* "The active texture unit selector **selects the texture image unit** accessed by commands
> involving texture image processing … Such commands include all variants of TexEnv (except for
> those controlling point sprite coordinate replacement), TexParameter, TexImage, BindTexture,
> Enable/Disable for any texture target …"
* "The active texture unit selector **also specifies the texture coordinate set** accessed by
> commands involving texture coordinate processing … Such commands include those accessing the
  current matrix stack (if MATRIX_MODE is TEXTURE), TexEnv commands controlling point sprite
  coordinate replacement, **TexGen**, Enable/Disable (if any texture coordinate generation enum
  is selected) …"

**Error conditions** ([G21] §2.11.2 p.46; [C46] §12.1.1 p.505):
* `GL_INVALID_OPERATION` if the **texture image unit** index ≥ `MAX_COMBINED_TEXTURE_IMAGE_UNITS`
  for image-processing commands (incl. `glTexEnv` with `GL_TEXTURE_ENV`).
* `GL_INVALID_OPERATION` if the **texture coordinate set** index ≥ `MAX_TEXTURE_COORDS` for
  coordinate-processing commands (incl. `glTexGen`, texture-matrix ops).
* `GL_INVALID_ENUM` if `glActiveTexture` is given an invalid texture enum.

**TGLMT practical note:** in practice implementations have `MAX_TEXTURE_COORDS ==
MAX_COMBINED_TEXTURE_IMAGE_UNITS`, so a single per-unit array suffices; but implement the two
limits separately so the `INVALID_OPERATION` behaviour matches.

### A.5.4 Stack depth and errors

* Texture matrix stack depth ≥ 2 (all units have the **same** depth).
* `glPushMatrix` on a full stack → `GL_STACK_OVERFLOW`.
* `glPopMatrix` with one entry → `GL_STACK_UNDERFLOW`.
* Initial: all matrices identity; initial matrix mode `GL_MODELVIEW`; initial active texture
  unit `GL_TEXTURE0`.

---

## A.6 `GL_SPHERE_MAP` — the exact equation

### A.6.1 The spec text

`[G21] §2.11.4` p.51 (identical in `[G14] §2.10.4` p.40, `[G13]`, `[G11] §2.9.4`):

> "When used with a suitably constructed texture image, calling TexGen with TEXTURE_GEN_MODE
> indicating SPHERE_MAP can simulate the reflected image of a spherical environment on a polygon.
> SPHERE_MAP texture coordinates are generated as follows. Denote the **unit vector pointing from
> the origin to the vertex (in eye coordinates)** by `u`. Denote the **current normal, after
> transformation to eye coordinates**, by `nf`. Let `r = (rx ry rz)ᵀ`, the **reflection vector**,
> be given by
>
> ```
> r = u − 2 nfᵀ(nf u)
> ```
>
> and let
>
> ```
> m = 2 / sqrt(rx² + ry² + (rz + 1)²)
> ```
>
> Then the value assigned to an s coordinate is `s = rx/m + 1/2`; the value assigned to a t
> coordinate is `t = ry/m + 1/2`."

### A.6.2 ⚠️ Correction: the brief's SPHERE_MAP formula is not OpenGL's

The brief asks for:

> "s = (1 + x/sqrt(x²+y²+z²))/2, t = (1 + y/sqrt(...))/2"

**That is a different (environment-map / "sphere mapping") formula, not OpenGL's
`GL_SPHERE_MAP`.** OpenGL's `GL_SPHERE_MAP` is the *reflection-vector* formula above. The
difference is fundamental:

| | Brief's formula | OpenGL's `GL_SPHERE_MAP` |
|---|---|---|
| Input vector | the vertex direction itself | the **reflected** direction `r = u − 2 nfᵀ(nf u)` |
| Uses the normal? | **No** | **Yes** — the normal is essential |
| Denominator | `sqrt(x²+y²+z²)` | `sqrt(rx²+ry²+(rz+1)²)` |

OpenGL's version simulates a **reflective** environment (it depends on the surface normal);
the brief's version is the classic **environment-mapped cube/sphere lookup** used for
non-reflective skyboxes.

**TGLMT must implement the OpenGL formula.** Implement:

```cpp
// SPHERE_MAP, per vertex:
vec3 u = normalize(eyePos);                 // unit vector origin -> vertex, in eye coords
vec3 nf = eyeSpaceNormal;                   // current normal, transformed to eye coords
// (already normalized per the normal-transformation rules)
vec3 r = u - 2.0f * nf * dot(nf, u);        // reflection vector
float m = 2.0f / sqrtf(r.x*r.x + r.y*r.y + (r.z+1.0f)*(r.z+1.0f));
s = r.x / m + 0.5f;
t = r.y / m + 0.5f;
// R and Q coordinates are NOT generated by SPHERE_MAP (INVALID_ENUM to select them).
```

Note the `+1` in `(rz+1)`: it is what makes the mapping behave at the "mirror" pole.

### A.6.3 Which coordinates SPHERE_MAP may drive

`[G21] §2.11.4` p.51 and `[C46] §12.1.3` p.508:

> "Calling TexGen with a coord of either R or Q when pname indicates SPHERE_MAP generates the
> error INVALID_ENUM."

⇒ `glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP)` and `glTexGenf(GL_T, …, GL_SPHERE_MAP)`
are legal; `GL_R` and `GL_Q` raise `GL_INVALID_ENUM`.

`GL_REFLECTION_MAP`: legal for S, T, R; **Q is INVALID_ENUM**.
`GL_NORMAL_MAP`: legal for S, T, R; **Q is INVALID_ENUM**.

### A.6.4 The reflection vector `u`

`u` is "the unit vector pointing from the origin to the vertex (in eye coordinates)". The origin
here is the **eye** origin, so `u = normalize(eyePos)` where `eyePos` is the vertex position
after the model-view transform.

The normal `nf` is "the current normal, after transformation to eye coordinates" — i.e. the
normal after the model-view transform (and after the rescale/normalise rules of
`[G21] §2.11.3` / `[C46] §12.1.2`).

### A.6.5 Degenerate cases and TGLMT's deterministic choice (D3)

The formula has a singularity when `rx = ry = 0` and `rz = −1`: the denominator
`sqrt(0 + 0 + 0) = 0`, so `m = ∞` and `s = rx/m + 1/2 = 0/∞ + 1/2 = 1/2`, `t = 1/2`. This is
the case where the reflected ray points exactly away from the viewer.

There is **no** `z <= 0` special case in the OpenGL spec — the brief's premise about "z <= 0"
belongs to the *other* (non-OpenGL) formula it quotes. For OpenGL's formula the only degenerate
case is the one above.

**TGLMT deterministic choice (D3):**
* Compute `d2 = rx*rx + ry*ry + (rz+1)*(rz+1)`.
* If `d2 <= 0` (i.e. `r == (0,0,-1)`): output `s = t = 0.5`.
* Otherwise `m = 2/sqrt(d2)`, `s = rx/m + 0.5`, `t = ry/m + 0.5`.
* Do **not** clamp `s`/`t` to [0,1] — the spec does not, and with a valid normal `s,t` land in
  [0,1] anyway.

---

## A.7 `GL_EYE_LINEAR` with an eye-space plane — answering the brief's A.7

The brief asks: "GL_TEXTURE_GEN_R/Q_MODE with GL_EYE_LINEAR_PLANE: exact equation
(s = dot(E, P), where P is the plane in eye space)".

**As stated this uses non-existent tokens** (see §0.5). The real, equivalent OpenGL semantics:

* `glTexGenfv(GL_R, GL_EYE_PLANE, p)` — `p` = 4 coefficients of a plane given in **object
  coordinates**; the GL immediately computes the eye-space plane
  `p' = (p1 p2 p3 p4) · M⁻¹` using the **current model-view matrix `M`**.
* `glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR)` selects `GL_EYE_LINEAR` for the R
  coordinate.
* At vertex-processing time the R coordinate is generated as

  ```
  g = p'1*xe + p'2*ye + p'3*ze + p'4*we
  ```

  which is exactly `dot((xe,ye,ze,1), p')` — the signed distance (up to the plane's normal
  length) from the eye-space vertex to the eye-space plane.

So the brief's "s = dot(E, P) with P the plane in eye space" is **conceptually right**; the
tokens and the naming are wrong.

**Key detail:** the `M⁻¹` is applied **once, at `glTexGenfv` time**, using the model-view
matrix then in effect. If the app changes the model-view matrix afterwards, the stored eye
plane does **not** change. (`[G21] §2.11.4`: "M is the model-view matrix in effect when
p1, . . . , p4 are specified.")

**TGLMT implementation:**

```cpp
// glTexGenfv(coord, GL_EYE_PLANE, p):
//   1. store raw object plane:  env.texGen[unit].objPlane[coord] = p
//   2. compute eye plane:      env.texGen[unit].eyePlane[coord] = p * inverse(modelView)
//   3. (GetTexGen(GL_EYE_PLANE) returns the eye plane; GetTexGen(GL_OBJECT_PLANE) returns raw p)
```

Matrix convention: `p' = p · M⁻¹` means `p'` is the row vector `p` multiplied by the inverse
model-view. With `M` in column-major 4×4 form, `p'ᵀ = (M⁻¹)ᵀ · pᵀ` = `transpose(inverse(M)) ·
p`. (In GLSL/Metal terms: `p' = transpose(inverse(M)) * p` with `p` as a column.)

---

## A.8 `glTexEnv` / `glTexGen` error conditions (consolidated)

### A.8.1 `glTexEnv*`

From `[C46] §16.1` "Errors" (printed p.604):

| Condition | Error |
|---|---|
| `target` ∉ {`GL_TEXTURE_FILTER_CONTROL`, `GL_POINT_SPRITE`, `GL_TEXTURE_ENV`} | `GL_INVALID_ENUM` |
| `pname` ∉ valid list for `GL_TEXTURE_ENV` (§A.1.4) | `GL_INVALID_ENUM` |
| `pname` valid but not valid for this `target` | `GL_INVALID_OPERATION` |
| `param`/`params` not a valid *value* for `pname` | `GL_INVALID_OPERATION` |
| active texture unit ≥ `MAX_TEXTURE_UNITS` | `GL_INVALID_OPERATION` |

Additional value-level rules:
* `GL_TEXTURE_ENV_MODE` value ∈ {`GL_REPLACE`, `GL_MODULATE`, `GL_DECAL`, `GL_BLEND`, `GL_ADD`,
  `GL_COMBINE`} ([G21] §3.8.13; [C46] §16.1). Otherwise `GL_INVALID_OPERATION`.
* `GL_COMBINE_RGB`/`GL_COMBINE_ALPHA` value ∈ {`GL_REPLACE`, `GL_MODULATE`, `GL_ADD`,
  `GL_ADD_SIGNED`, `GL_INTERPOLATE`, `GL_SUBTRACT`} (+ `GL_DOT3_RGB`/`GL_DOT3_RGBA` for
  `GL_COMBINE_RGB` only) ([ARB-COMB], [ARB-DOT3], [C46] Table 16.4).
* `GL_SRCn_RGB`/`GL_SRCn_ALPHA` value ∈ {`GL_TEXTURE`, `GL_CONSTANT`, `GL_PRIMARY_COLOR`,
  `GL_PREVIOUS`} (+ `GL_TEXTURE0..n` crossbar) ([ARB-COMB], [ARB-XBAR]).
* `GL_OPERANDn_RGB` value ∈ {`GL_SRC_COLOR`, `GL_ONE_MINUS_SRC_COLOR`, `GL_SRC_ALPHA`,
  `GL_ONE_MINUS_SRC_ALPHA`}; `GL_OPERANDn_ALPHA` value ∈ {`GL_SRC_ALPHA`,
  `GL_ONE_MINUS_SRC_ALPHA`} ([ARB-COMB]).
* `GL_RGB_SCALE`/`GL_ALPHA_SCALE` ∈ {1.0, 2.0, 4.0} else `GL_INVALID_VALUE` ([ARB-COMB]).
* `GL_TEXTURE_ENV_COLOR` — 4 floats, clamped to [0,1]; **no error** for out-of-range.

### A.8.2 `glTexGen*`

From `[C46] §12.1.3` "Errors" (printed p.509):

| Condition | Error |
|---|---|
| `coord` ∉ {`GL_S`,`GL_T`,`GL_R`,`GL_Q`} | `GL_INVALID_ENUM` |
| `pname` ∉ {`GL_TEXTURE_GEN_MODE`,`GL_OBJECT_PLANE`,`GL_EYE_PLANE`} | `GL_INVALID_ENUM` |
| `coord` ∈ {`GL_R`,`GL_Q`} and mode = `GL_SPHERE_MAP` | `GL_INVALID_ENUM` |
| `coord` = `GL_Q` and mode = `GL_REFLECTION_MAP` | `GL_INVALID_ENUM` |
| `coord` = `GL_Q` and mode = `GL_NORMAL_MAP` | `GL_INVALID_ENUM` |

Also: if the active texture coordinate set index ≥ `MAX_TEXTURE_COORDS` → `GL_INVALID_OPERATION`
([C46] §12.1.1 p.505).

### A.8.3 `glGetTexEnv*` / `glGetTexGen*`

* `glGetTexEnv*`: same target/pname errors as `glTexEnv*` ([C46] §16.1.1 p.604).
* `glGetTexGen*`: same coord/pname errors as `glTexGen*` ([C46] §12.1.3.1 p.509).
* `glGetTexEnv*` with `target` = `GL_TEXTURE_ENV` reads the **active** texture unit's env.
* `glGetTexGen*` reads the **active** texture coordinate set's texgen state.

---

## A.9 `glGet` pnames for texture environment and generation

### A.9.1 `glGetTexEnv*` pnames

`[C46] Table 23.24` (printed p.752) and `[G21] Table 6.19` (printed p.284–285):

| pname | `glGetTexEnviv`/`fv`? | Type | Initial | Notes |
|---|---|---|---|---|
| `GL_TEXTURE_ENV_MODE` | `iv` | E | `GL_MODULATE` | per texture unit |
| `GL_TEXTURE_ENV_COLOR` | `fv` | C (4 floats) | (0,0,0,0) | per texture unit |
| `GL_COMBINE_RGB` | `iv` | E | `GL_MODULATE` | per texture unit |
| `GL_COMBINE_ALPHA` | `iv` | E | `GL_MODULATE` | per texture unit |
| `GL_SRC0_RGB` | `iv` | E | `GL_TEXTURE` | per texture unit |
| `GL_SRC1_RGB` | `iv` | E | `GL_PREVIOUS` | |
| `GL_SRC2_RGB` | `iv` | E | `GL_CONSTANT` | |
| `GL_SRC0_ALPHA` | `iv` | E | `GL_TEXTURE` | |
| `GL_SRC1_ALPHA` | `iv` | E | `GL_PREVIOUS` | |
| `GL_SRC2_ALPHA` | `iv` | E | `GL_CONSTANT` | |
| `GL_OPERAND0_RGB` | `iv` | E | `GL_SRC_COLOR` | |
| `GL_OPERAND1_RGB` | `iv` | E | `GL_SRC_COLOR` | |
| `GL_OPERAND2_RGB` | `iv` | E | `GL_SRC_ALPHA` | |
| `GL_OPERAND0_ALPHA` | `iv` | E | `GL_SRC_ALPHA` | |
| `GL_OPERAND1_ALPHA` | `iv` | E | `GL_SRC_ALPHA` | |
| `GL_OPERAND2_ALPHA` | `iv` | E | `GL_SRC_ALPHA` | |
| `GL_RGB_SCALE` | `fv` | R | 1.0 | |
| `GL_ALPHA_SCALE` | `fv` | R | 1.0 | |
| `GL_TEXTURE_LOD_BIAS` | `fv` | R | 0.0 | target `GL_TEXTURE_FILTER_CONTROL` |
| `GL_COORD_REPLACE` | `iv` | B | FALSE | target `GL_POINT_SPRITE` |

> ⚠️ **These are `glGetTexEnv*` pnames, not `glGetIntegerv` pnames.** `glGetIntegerv(GL_TEXTURE_ENV_MODE)`
> is **not** legal — the state table lists the *Get command* as `GetTexEnviv`. The brief's item
> A.10 ("glGet pnames: GL_TEXTURE_ENV_MODE, GL_TEXTURE_ENV_COLOR, …") must be read as
> "gettable via `glGetTexEnv*`", not via `glGetIntegerv`.
>
> Exception: `glGetIntegerv(GL_ACTIVE_TEXTURE)` **is** a plain `glGetIntegerv` pname
> ([C46] Table 23.24: `ACTIVE TEXTURE E GetIntegerv TEXTURE0`).

`GL_SRCn_RGB`/`GL_OPERANDn_RGB` and `GL_SOURCE0_RGB`/`GL_OPERAND0_RGB` are the same tokens
(aliases) — both spellings must work.

### A.9.2 `glGetTexGen*` pnames

`[C46] Table 23.24` (printed p.752), `[G21] Table 6.19`:

| pname | `glGetTexGeniv`/`fv`? | Type | Initial | Notes |
|---|---|---|---|---|
| `GL_TEXTURE_GEN_MODE` | `iv` | E | `GL_EYE_LINEAR` | per coordinate (S,T,R,Q), per texture unit |
| `GL_OBJECT_PLANE` | `fv` (4 floats) | R4 | see §A.4.2 | raw object-space plane |
| `GL_EYE_PLANE` | `fv` (4 floats) | R4 | see §A.4.2 | **eye-space** plane (already transformed) |

Plus `glIsEnabled(GL_TEXTURE_GEN_S/T/R/Q)` for the enables (per texture unit).

**There are no `GL_TEXTURE_GEN_S_MODE` / `GL_TEXTURE_GEN_T_MODE` / `GL_TEXTURE_GEN_R_MODE` /
`GL_TEXTURE_GEN_Q_MODE` pnames and no `GL_TEXTURE_GEN_S_PLANE` etc.** — the coordinate is an
*argument* to `glGetTexGen*(coord, pname, params)`, not a pname. (The brief's A.10 list is
therefore wrong on this point; see §0.5.)

---

# PART B — FOG

## B.1 The three fog equations

`[G21] §3.10` p.194 (equations 3.31–3.33) and `[C46] §16.4` p.608 (equations 16.1–16.3):

```
GL_EXP     (16.1):  f = exp(−d · c)
GL_EXP2    (16.2):  f = exp(−(d · c)²)
GL_LINEAR  (16.3):  f = (e − c) / (e − s)
```

| Symbol | Meaning | State pname |
|---|---|---|
| `c` | fog coordinate (§B.3) | — |
| `d` | fog density | `GL_FOG_DENSITY` |
| `s` | linear fog start | `GL_FOG_START` |
| `e` | linear fog end | `GL_FOG_END` |

Verbatim from `[C46] §16.4` p.608:

> "This factor f is computed according to one of three equations: f = exp(−d·c), f = exp(−(d·c)²),
> or f = (e−c)/(e−s)."

**Clamp:** `[C46] §16.4` p.609 and `[G21] §3.10` p.195:

> "No matter which equation and approximation is used to compute f, the result is **clamped to
> [0, 1]** to obtain the final f."

**Linear edge case:** "[C46] §16.4 p.608: (if, when 16.3 is selected, e = s, results are
undefined)". TGLMT choice **D6**: return `f = 0.0`.

**EXP/EXP2 note:** `d ≥ 0` is required (see B.6). `exp()` of a large negative number → 0, which is
already in range.

### B.1.1 Implementable pseudocode

```cpp
GLfloat ComputeFogFactor(GLenum mode, GLfloat d, GLfloat s, GLfloat e, GLfloat c)
{
    GLfloat f;
    switch (mode) {
    case GL_EXP:    f = expf(-d * c);                 break;
    case GL_EXP2:   f = expf(-(d * c) * (d * c));     break;
    case GL_LINEAR:
        if (e == s) return 0.0f;                      // D6: undefined -> fully fogged
        f = (e - c) / (e - s);
        break;
    default:        f = 0.0f; break;
    }
    return clamp01(f);
}
```

---

## B.2 The blend — RGB only, alpha untouched

`[G21] §3.10` p.195 and `[C46] §16.4` p.609:

> "In RGBA mode, if `Cr` represents a rasterized fragment's R, G, or B value, then the
> corresponding value produced by fog is
>
> ```
> C = f·Cr + (1 − f)·Cf
> ```
>
> (**The rasterized fragment's A value is not changed by fog blending.**)"

**Confirmed: fog affects R, G, B only. The fragment's alpha channel is untouched.** The fog
colour's alpha is also not used in the RGBA blend.

`Cf` = `GL_FOG_COLOR` (the *fog* colour), `Cr` = the post-texturing, post-colour-sum fragment
colour.

**4.6 compat nuance** ([C46] §16.4 p.609):

> "If fragment color clamping is enabled, the components of `Cr` and `Cf` and the result `C` are
> clamped to the range [0, 1] before the fog blend is performed."

In GL 2.1 ([G21] §3.10 p.195): "Each component of `Cf` is clamped to [0, 1] when specified." —
i.e. the clamp happens at `glFog` time in 2.1, at blend time in 4.6 compat.

**TGLMT:** clamp `GL_FOG_COLOR` components to [0,1] on `glFog` (matching 2.1 and matching
`[C46]`'s "Cf … clamped to the range [0,1] before the fog blend"), and additionally honour
`GL_CLAMP_FRAGMENT_COLOR` for the `Cr`/`C` clamp in 4.6-compat mode.

**Color index mode** ([G21] §3.10 p.195; [C46] §16.4 p.609) — legacy, removed in core:

```
I = ir + (1 − f)·if
```

`(1−f)·if` is rounded to the nearest fixed-point value with the same number of fractional bits
as `ir`, and the integer part of `I` is masked with `2ⁿ−1` (`n` = bits per colour index).
`if` = `GL_FOG_INDEX` (initial 0).

---

## B.3 The fog coordinate — the exact quantity

### B.3.1 The spec text

`[C46] §16.4` p.608 (and identically `[G21] §3.10` p.194–195, `[G14] §3.10` p.160):

> "If a vertex or geometry shader is active, or if the fog source, as defined below, is
> `FOG_COORD`, then `c` is the **interpolated value of the fog coordinate** for this fragment.
> Otherwise, if the fog source is `FRAGMENT_DEPTH`, then `c` is the **eye-coordinate distance
> from the eye, (0, 0, 0, 1) in eye coordinates, to the fragment center**."

### B.3.2 ⚠️ Radial distance vs |z_eye| — the exact answer

**The spec quantity is the RADIAL eye-space distance to the fragment centre:**

```
c = |P_eye − (0,0,0)| = sqrt(xe² + ye² + ze²)
```

**NOT** `|ze|`.

The `|ze|` form is only an *explicitly permitted approximation*, quoted verbatim from
`[C46] §16.4` p.609 and `[G21] §3.10` p.195:

> "An implementation may choose to approximate the eye-coordinate distance from the eye to each
> fragment center by **|ze|**."

The eye is at `(0,0,0,1)` in eye coordinates — i.e. the origin of the eye coordinate system — so
the distance is just the length of the eye-space position vector.

**Note the historical naming trap:** in **GL 1.1 and 1.3** the spec wrote the equations as
`f = exp(−d·z)` and called the variable `z` ([G11] §3.9 eq. 3.16; [G13] §3.10 eq. 3.24), which
reads as if it were the eye-space z coordinate. It is **not** — the accompanying prose says
"(z is the eye-coordinate distance from the eye, (0, 0, 0, 1) in eye coordinates, to the fragment
center)". GL 1.4 renamed the variable to `c` to remove the confusion. **Do not be misled by the
old `z` symbol.**

**TGLMT deterministic choice (D4): implement the true radial distance** `sqrt(x²+y²+z²)` in eye
space by default. Optionally expose the `|ze|` approximation behind a flag, documented as a
spec-permitted deviation (it is much cheaper on Metal and is what most drivers do), but the
default must be the spec-exact radial distance.

**Cost note for TGLMT:** the radial distance is *not* linear in screen space, so it cannot be
computed per-vertex and interpolated — it must be computed per-fragment from the interpolated
eye-space position. In the Metal shader this means TGLMT must carry the interpolated eye-space
position (or at least `ze` plus the interpolation perspective factor) to the fragment stage.
`c = |P_eye|` requires the full 3-component eye position; `|ze|` needs only `ze`.

### B.3.3 `GL_FOG_COORD_SRC` — the source selector

`[C46] §16.4` p.608–609:

> "If pname is `FOG_COORD_SRC`, then param must be, or params must point to an integer that is one
> of the symbolic constants `FRAGMENT_DEPTH` or `GL_FOG_COORD`."

| Token | Value | Meaning |
|---|---|---|
| `GL_FOG_COORDINATE_SOURCE` / `GL_FOG_COORD_SRC` | 0x8450 | The pname. `[REG]` lists `GL_FOG_COORD_SRC` as an alias of `GL_FOG_COORDINATE_SOURCE`. Both spellings must be accepted. |
| `GL_FRAGMENT_DEPTH` | 0x8452 | `c` = radial eye-space distance to fragment centre. **Default.** |
| `GL_FOG_COORD` / `GL_FOG_COORDINATE` | 0x8451 | `c` = the interpolated per-vertex fog coordinate (`glFogCoord*` / `glFogCoordPointer`). |

**Default value: `GL_FRAGMENT_DEPTH`** — `[G21] §3.10` p.195–196 ("FOG_COORD_SRC is
FRAGMENT_DEPTH"); `[C46] §16.4` p.610; `[C46] Table 23.x` ("FOG_COORD_SRC E GetIntegerv
FRAGMENT_DEPTH").

**Version note:** `GL_FOG_COORD_SRC` / `GL_FOG_COORD` / `GL_FRAGMENT_DEPTH` were introduced by
**`EXT_fog_coord`**, core in **GL 1.4**. In **GL 1.3** fog always used the eye-coordinate
distance — there was no source selector ([G13] §3.10). In **GL 1.1/1.2** likewise.

**The "vertex shader is active" override:** `[C46] §16.4` and `[G21] §3.10` both say the fog
coordinate is the interpolated fog coordinate **if a vertex or geometry shader is active**, even
when `FOG_COORD_SRC` is `FRAGMENT_DEPTH`. (In GL 1.4, before shaders existed, the wording was
just "if the fog source is FRAGMENT_DEPTH … otherwise FOG_COORDINATE".) **TGLMT: when a vertex
or geometry shader is active, use the interpolated fog coordinate.**

In GLSL, the interpolated fog coordinate is the built-in `gl_FogFragCoord` (see `[C46] §7.1`
and the GLSL 4.60 spec). `[C46]` p.739: "The built-in output `gl_FogFragCoord` is used as the c
value described in …".

---

## B.4 `glFog` state, defaults, and `GL_FOG_HINT`

### B.4.1 The command

```c
void glFogf (GLenum pname, GLfloat param);
void glFogfv(GLenum pname, const GLfloat *params);
void glFogi (GLenum pname, GLint param);
void glFogiv(GLenum pname, const GLint *params);
```

### B.4.2 Complete pname table with defaults

| pname | Type | Meaning | Initial value | Authority |
|---|---|---|---|---|
| `GL_FOG_MODE` (0x0B65) | E | `GL_EXP` \| `GL_EXP2` \| `GL_LINEAR` | **`GL_EXP`** | [G21] §3.10; [C46] Table 23.x |
| `GL_FOG_DENSITY` (0x0B62) | R | `d` in EXP/EXP2 | **1.0** | [G21] §3.10; [C46] |
| `GL_FOG_START` (0x0B63) | R | `s` in LINEAR | **0.0** | [G21] §3.10; [C46] |
| `GL_FOG_END` (0x0B64) | R | `e` in LINEAR | **1.0** | [G21] §3.10; [C46] |
| `GL_FOG_COLOR` (0x0B66) | C (4) | `Cf` | **(0,0,0,0)** | [G21] §3.10; [C46] |
| `GL_FOG_COORD_SRC` (0x8450) | E | `GL_FRAGMENT_DEPTH` \| `GL_FOG_COORD` | **`GL_FRAGMENT_DEPTH`** | [G21] §3.10; [C46] |
| `GL_FOG_INDEX` (0x0B61) | CI | `if`, colour-index mode fog index | **0** | [G21] §3.10; [C46] |
| `GL_FOG` enable | B | fog enabled? | **FALSE** (disabled) | [G21] §3.10; [C46] |

**Verification of the brief's defaults:** "density 1.0, start 0, end 1, color (0,0,0,0)" —
✅ **all four confirmed.** Verbatim from `[G21] §3.10` p.196:

> "In the initial state, fog is disabled, FOG_COORD_SRC is FRAGMENT_DEPTH, FOG_MODE is EXP,
> d = 1.0, e = 1.0, and s = 0.0; Cf = (0, 0, 0, 0) and if = 0."

and `[C46] §16.4` p.610: identical.

`GL_FOG_START` / `GL_FOG_END` may be negative (they were changed to allow negatives in GL 1.4 —
"[G21] Appendix A changes: 'Fog start and end values may now be negative.'"). Only the *density*
must be non-negative.

`GL_FOG_COLOR` components are clamped to [0,1] when specified ([G21] §3.10 p.195: "Each
component of Cf is clamped to [0, 1] when specified"; `[C46]` §16.4 p.609 clamps `Cf` before the
blend).

### B.4.3 `GL_FOG_HINT`

`GL_FOG_HINT` (0x0C54) is a **hint target** for `glHint`.

`[C46] §21.5` p.703–704, Table 21.3:

> "FOG_HINT — Fog quality (calculated per-pixel or per-vertex)"

Hint values: `GL_FASTEST` (most efficient option), `GL_NICEST` (highest quality),
`GL_DONT_CARE` (no preference).

> "The interpretation of hints is implementation-dependent. **An implementation may ignore them
> entirely.** The initial value of all hints is `DONT_CARE`."

**Effect:** `GL_FASTEST` permits computing the fog factor **per vertex** and interpolating it;
`GL_NICEST` requires per-fragment evaluation.

**TGLMT deterministic choice (D5):** compute fog per-fragment and ignore the hint. This is
always legal ("may ignore them entirely") and is the higher-quality choice. Note this interacts
with §B.3.2: a per-vertex fog factor would have to be based on `|ze|` (linear in eye space),
which is the permitted approximation.

`GL_FOG_HINT` is **removed in the core profile** (`[CORE46] §E.2.2`: "Hints — The
PERSPECTIVE_CORRECTION_HINT, POINT_SMOOTH_HINT, FOG_HINT, and GENERATE_MIPMAP_HINT targets to
Hint"), but present in the compatibility profile ([C46] Table 23.62).

---

## B.5 Where fog sits in the pipeline

`[C46] §16.4` follows §16.1 (Texture Environments), §16.2 (Texture Application), §16.3 (Color Sum).
`[C46] Chapter 17` (Writing Fragments and Samples to the Framebuffer) follows.

```
[per-fragment]
   texture environment + texture application   (§16.1, §16.2)
   color sum                                    (§16.3)
   >>> FOG <<<                                  (§16.4)
   --- Chapter 17 ---
   antialiasing application                    (§17.1)
   multisample point fade                      (§17.2)
   per-fragment operations                     (§17.3)
       pixel ownership → scissor → multisample → alpha test → depth test → stencil test
       → blending → dithering → logic op → colour mask
   framebuffer
```

`[G21]` has the same ordering: §3.8 texturing → §3.9 color sum → §3.10 fog → Chapter 4
(per-fragment operations: §4.1.1 pixel ownership … §4.1.8 blending … §4.1.9 dithering).

**Fog is applied AFTER texture environment and AFTER color sum, and BEFORE all per-fragment
operations** (alpha test, blending, dithering, logic op).

`[G21] §3.10` p.194: "If enabled, fog blends a fog color with a rasterized fragment's
**post-texturing color** using a blending factor f." — and color sum (§3.9) also runs on the
post-texturing colour, so the fog input is the *post-color-sum* colour.

**Because fog does not change alpha, the fog↔alpha-test ordering is not observable.** But
fog↔**blending** ordering *is*: fog must be applied **before** blending, so the blend function
sees the fogged colour.

**"Fog has no effect if a fragment shader is active."** ([G21] §3.10 p.196; [C46] §16.4 p.610;
`[G21] §3.11.2` bullet list: "Fog as described in section 3.10 is not applied.")

---

## B.6 `glFog*` error conditions

From `[G21] §3.10` p.195 and `[C46] §16.4` p.609:

| Condition | Error |
|---|---|
| `d` (the `GL_FOG_DENSITY` value) is **negative** | **`GL_INVALID_VALUE`** |
| `pname` is not one of the listed fog pnames | `GL_INVALID_ENUM` |
| `GL_FOG_MODE` value ∉ {`GL_EXP`,`GL_EXP2`,`GL_LINEAR`} | `GL_INVALID_ENUM` |
| `GL_FOG_COORD_SRC` value ∉ {`GL_FRAGMENT_DEPTH`,`GL_FOG_COORD`} | `GL_INVALID_ENUM` |

Verbatim: "[G21] §3.10: If d is specified less than zero, the error INVALID VALUE results."
and "[C46] §16.4: An INVALID_VALUE error is generated if d is negative."

Only the **density** is restricted. `GL_FOG_START` / `GL_FOG_END` may be any float, including
negative.

`glGetIntegerv`/`glGetFloatv` with a fog pname: `GL_INVALID_ENUM` if the pname is not a valid
query pname (per the general error rules in `[C46] §2.2`).

---

## B.7 `glGet` pnames for fog

`[C46] Table 23.x` (printed p.739) and `[G21] Table 6.x` (printed p.284):

| pname | Get command | Type | Initial | Notes |
|---|---|---|---|---|
| `GL_FOG_COLOR` | `glGetFloatv` | C | 0.0,0.0,0.0,0.0 | |
| `GL_FOG_INDEX` | `glGetFloatv` | CI | 0 | colour-index mode |
| `GL_FOG_DENSITY` | `glGetFloatv` | R | 1.0 | |
| `GL_FOG_START` | `glGetFloatv` | R | 0.0 | |
| `GL_FOG_END` | `glGetFloatv` | R | 1.0 | |
| `GL_FOG_MODE` | `glGetIntegerv` | E | `GL_EXP` | |
| `GL_FOG_COORD_SRC` | `glGetIntegerv` | E | `GL_FRAGMENT_DEPTH` | |
| `GL_FOG_HINT` | `glGetIntegerv` | E | `GL_DONT_CARE` | hint state, `[C46] Table 23.62` |

**These are plain `glGet*` pnames** (unlike the texenv/texgen ones, which need `glGetTexEnv*` /
`glGetTexGen*`). Note `[C46]` lists `GL_FOG_COLOR`/`GL_FOG_INDEX` with `GetFloatv` but the
general rule ([C46] §22.1) allows a float-typed value to be fetched with `glGetIntegerv` too
(with the standard conversion), and `glGetFloatv` on an enum-typed value converts the same way.

`GL_FOG` enable state is queried with `glIsEnabled(GL_FOG)`.

---

# PART C — Implementation checklist for TGLMT

## C.1 State to store (all per texture unit unless noted)

```cpp
struct TexEnvUnit {
    GLenum  mode;                 // GL_REPLACE|MODULATE|DECAL|BLEND|ADD|COMBINE
    GLfloat envColor[4];          // GL_TEXTURE_ENV_COLOR, clamped [0,1]
    GLenum  combineRGB;           // REPLACE|MODULATE|ADD|ADD_SIGNED|INTERPOLATE|SUBTRACT|DOT3_RGB|DOT3_RGBA
    GLenum  combineAlpha;         // REPLACE|MODULATE|ADD|ADD_SIGNED|INTERPOLATE|SUBTRACT
    GLenum  srcRGB[3];            // TEXTURE|CONSTANT|PRIMARY_COLOR|PREVIOUS|TEXTURE<i>
    GLenum  srcAlpha[3];
    GLenum  operandRGB[3];        // SRC_COLOR|ONE_MINUS_SRC_COLOR|SRC_ALPHA|ONE_MINUS_SRC_ALPHA
    GLenum  operandAlpha[3];      // SRC_ALPHA|ONE_MINUS_SRC_ALPHA
    GLfloat rgbScale;             // 1.0|2.0|4.0
    GLfloat alphaScale;           // 1.0|2.0|4.0
    GLfloat lodBias;              // GL_TEXTURE_LOD_BIAS
};

struct TexGenUnit {
    GLenum   mode[4];             // per coord: OBJECT_LINEAR|EYE_LINEAR|SPHERE_MAP|REFLECTION_MAP|NORMAL_MAP
    GLboolean enabled[4];         // GL_TEXTURE_GEN_S/T/R/Q
    GLfloat  objPlane[4][4];      // raw plane coefficients
    GLfloat  eyePlane[4][4];      // p * M^-1, computed at glTexGenfv time
    GLfloat  texMatrix[4][4];     // this unit's texture matrix (stack depth >= 2)
    int      texMatrixStackDepth;
};

struct FogState {                // global, not per-unit
    GLboolean enabled;
    GLenum   mode;               // EXP|EXP2|LINEAR
    GLfloat  density, start, end;
    GLfloat  color[4];
    GLenum   coordSrc;           // FRAGMENT_DEPTH|FOG_COORD
    GLfloat  index;              // colour-index fog index
    GLenum   hint;               // DONT_CARE|FASTEST|NICEST
};
```

## C.2 Command semantics recap

| Command | Acts on | Notes |
|---|---|---|
| `glTexEnv*(GL_TEXTURE_ENV, …)` | **active** texture unit's env | not a global slot |
| `glTexEnv*(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, v)` | active unit's LOD bias | |
| `glTexEnv*(GL_POINT_SPRITE, GL_COORD_REPLACE, b)` | point-sprite state | |
| `glTexGen*(coord, GL_TEXTURE_GEN_MODE, m)` | active unit, coordinate `coord` | |
| `glTexGen*(coord, GL_OBJECT_PLANE, p)` | active unit, raw object plane | |
| `glTexGen*(coord, GL_EYE_PLANE, p)` | active unit, **derives** eye plane = p·M⁻¹ now | |
| `glMatrixMode(GL_TEXTURE)` + `glLoadMatrixf` etc. | **active** unit's texture matrix | per-unit stack |
| `glPushMatrix/glPopMatrix` in TEXTURE mode | active unit's texture matrix stack | ≥ depth 2 |
| `glFog*(pname, v)` | global fog state | |
| `glHint(GL_FOG_HINT, h)` | global fog hint | may be ignored |

## C.3 Shader-side order (Metal)

```
vertex:
   modelview * pos           -> eyePos
   modelview3x3 * normal     -> eyeNormal (normalized)
   if (texGen enabled)       -> coord = gen(eyePos, eyeNormal)
   coord = texMatrix[unit] * coord

fragment (per texture unit u = 0..N-1):
   Cs,As = sampleTexture(u, coord[u])
   Cv,Av = texFunc(env[u], Cf,Af, Cs,As, Cc,Ac, Cp,Ap)
   Cp,Ap = Cv,Av
   // after all units:
   if (colorSum)  c = cpri + csec.rgb ; c.a = cpri.a
   if (fogEnabled) { c.rgb = f*c.rgb + (1-f)*fogColor.rgb ; /* a untouched */ }
   // chapter 17: alpha test, blend, dither, logic op
```

**For `GL_FOG_COORD_SRC == GL_FRAGMENT_DEPTH` with no vertex/geometry shader:**
`c = length(eyePosInterpolated)` — requires interpolating the eye-space position to the fragment
stage.

**With a vertex/geometry shader active:** `c = interpolated gl_FogFragCoord`.

## C.4 Highest-risk items (re-check these first)

1. **`GL_TEXTURE_ENV` is per active texture unit** — the single most common bug.
2. **The texture matrix stack is per texture unit** (since GL 1.2.1), not one global stack.
3. **DECAL is a lerp with the texture alpha**, not the 3-way if/else; and undefined for
   LUMINANCE/ALPHA/INTENSITY base formats.
4. **BLEND uses `Cc` (the env colour), not the fragment colour/alpha.**
5. **ADD saturates at 1.0.**
6. **Fog affects RGB only.**
7. **Fog's `GL_FRAGMENT_DEPTH` is the radial eye distance** (not `|z|`), though `|z|` is a
   permitted approximation.
8. **`GL_COMBINE` rejects blend tokens** with `GL_INVALID_OPERATION` — do not silently accept
   `GL_MIN`/`GL_MAX`/`GL_REVERSE_SUBTRACT`/`GL_MULTIPLY`/`GL_SRC_ALPHA_SATURATE`/`GL_DST_COLOR`.
9. **`glGetTexGen*(coord, GL_EYE_PLANE, …)` returns the eye-space plane**, not the raw input.
10. **`Arg2` differs between the RGB and alpha combiners.**
11. **`GL_COMBINE` clamps results to [0,1]** after the scale (subject to
    `GL_CLAMP_FRAGMENT_COLOR` in 4.6 compat).

---

## Appendix — quick token reference (values from `docs/khronos/gl.xml`)

| Token | Value |
|---|---|
| `GL_TEXTURE_ENV` | 0x2300 |
| `GL_TEXTURE_ENV_MODE` | 0x2200 |
| `GL_TEXTURE_ENV_COLOR` | 0x2201 |
| `GL_COMBINE` | 0x8570 |
| `GL_COMBINE_RGB` | 0x8571 |
| `GL_COMBINE_ALPHA` | 0x8572 |
| `GL_RGB_SCALE` | 0x8573 |
| `GL_ALPHA_SCALE` | 0x0D1C (⚠️ **not** 0x8573 — the two scales have *different* token values) |
| `GL_ADD_SIGNED` | 0x8574 |
| `GL_INTERPOLATE` | 0x8575 |
| `GL_CONSTANT` | 0x8576 |
| `GL_PRIMARY_COLOR` | 0x8577 |
| `GL_PREVIOUS` | 0x8578 |
| `GL_SOURCE0_RGB` / `GL_SRC0_RGB` | 0x8580 |
| `GL_SOURCE1_RGB` / `GL_SRC1_RGB` | 0x8581 |
| `GL_SOURCE2_RGB` / `GL_SRC2_RGB` | 0x8582 |
| `GL_SOURCE0_ALPHA` / `GL_SRC0_ALPHA` | 0x8588 |
| `GL_SOURCE1_ALPHA` / `GL_SRC1_ALPHA` | 0x8589 |
| `GL_SOURCE2_ALPHA` / `GL_SRC2_ALPHA` | 0x858A |
| `GL_OPERAND0_RGB` | 0x8590 |
| `GL_OPERAND1_RGB` | 0x8591 |
| `GL_OPERAND2_RGB` | 0x8592 |
| `GL_OPERAND0_ALPHA` | 0x8598 |
| `GL_OPERAND1_ALPHA` | 0x8599 |
| `GL_OPERAND2_ALPHA` | 0x859A |
| `GL_SUBTRACT` | 0x84E7 |
| `GL_DOT3_RGB` | 0x86AE |
| `GL_DOT3_RGBA` | 0x86AF |
| `GL_TEXTURE0` (crossbar) | 0x84C0 + n |
| `GL_REPLACE` / `GL_MODULATE` / `GL_DECAL` | 0x1E01 / 0x2100 / 0x2101 |
| `GL_ADD` / `GL_BLEND` | 0x0104 / 0x0BE2 |
| `GL_SRC_COLOR` / `GL_ONE_MINUS_SRC_COLOR` | 0x0300 / 0x0301 |
| `GL_SRC_ALPHA` / `GL_ONE_MINUS_SRC_ALPHA` | 0x0302 / 0x0303 |
| `GL_TEXTURE_GEN_MODE` | 0x2500 |
| `GL_OBJECT_PLANE` | 0x2501 |
| `GL_EYE_PLANE` | 0x2502 |
| `GL_OBJECT_LINEAR` / `GL_EYE_LINEAR` / `GL_SPHERE_MAP` | 0x2401 / 0x2400 / 0x2402 |
| `GL_REFLECTION_MAP` / `GL_NORMAL_MAP` | 0x8512 / 0x8511 |
| `GL_TEXTURE_GEN_S` / `_T` / `_R` / `_Q` | 0x0C60 / 0x0C61 / 0x0C62 / 0x0C63 |
| `GL_FOG` | 0x0B60 |
| `GL_FOG_INDEX` | 0x0B61 |
| `GL_FOG_DENSITY` | 0x0B62 |
| `GL_FOG_START` | 0x0B63 |
| `GL_FOG_END` | 0x0B64 |
| `GL_FOG_MODE` | 0x0B65 |
| `GL_FOG_COLOR` | 0x0B66 |
| `GL_FOG_COORDINATE_SOURCE` / `GL_FOG_COORD_SRC` | 0x8450 |
| `GL_FOG_COORD` / `GL_FOG_COORDINATE` | 0x8451 |
| `GL_FRAGMENT_DEPTH` | 0x8452 |
| `GL_FOG_HINT` | 0x0C54 |
| `GL_EXP` / `GL_EXP2` / `GL_LINEAR` | 0x0800 / 0x0801 / **0x2601** |
| `GL_FASTEST` / `GL_NICEST` / `GL_DONT_CARE` | 0x1101 / 0x1102 / 0x1100 |

*(Token values cross-checked against `docs/khronos/gl.xml`. Note `GL_LINEAR` is 0x2601 — the same
token as the texture min/mag filter, not 0x0802. Note `GL_ALPHA_SCALE` (0x0D1C) and `GL_RGB_SCALE`
(0x8573) are distinct tokens, even though the spec text names them in the same breath.)*
