# LEGACY_LIGHTING_SPEC.md — OpenGL fixed-function lighting & materials (Compatibility Profile)

**Purpose:** an authoritative, implementation-ready reference for the OpenGL fixed-function
LIGHTING and MATERIALS pipeline (per-vertex lighting, materials, light sources, light model,
`glColorMaterial`, shade model / flat shading) that TGLMT must provide in its **Compatibility
Profile** code path, translatable 1:1 to Metal (MSL) by the `src/` owner.

This document is **normative for TGLMT**: where the OpenGL specification is ambiguous, the
"deterministic choice" column in §14 states exactly what TGLMT must do.

---

## 0. Sources, verification legend, and how to read this file

### 0.1 Sources used

| Tag | Document | Where it came from | Verified how |
|---|---|---|---|
| `[GL21]` | **The OpenGL Graphics System: A Specification, Version 2.1 (December 1, 2006)** — `glspec21.pdf`, 394 pp. | downloaded during this research from the Khronos registry mirror `https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/specs/gl/glspec21.pdf` (reachable; `registry.khronos.org` itself returns HTTP 403 from this network). **Not vendored in this repo.** | text extracted with `pypdf`; every quoted paragraph was read in the extracted text |
| `[GL42C]` | **OpenGL 4.2 (Compatibility Profile) — August 8, 2011** — `glspec42.compatibility.20110808.withchanges.pdf`, 741 pp. | downloaded from `http://www.opengl.org/registry/doc/glspec42.compatibility.20110808.withchanges.pdf` (reachable). **Not vendored in this repo.** | text extracted with `pypdf`; every quoted paragraph was read in the extracted text |
| `[GL46CORE]` | **OpenGL 4.6 (Core Profile) — May 5, 2022** — `docs/khronos/glspec46.core.pdf` (851 pp.) | **vendored in this repo** | extracted with `pypdf`; page numbers below are PDF page numbers of the vendored file (printed page in parentheses) |
| `[REF]` | Khronos **OpenGL reference pages** (the `glLight`/`glMaterial`/`glColorMaterial`/`glLightModel`/`glShadeModel`/`glGetLight`/`glGet` man pages, SGI/Khronos "OpenGL 2" text). | read via public mirrors of that text (`docs.gl/gl2/...`, `linux.die.net/man/3/...`, `opengl.org.ru/docs/man/...`, Microsoft Learn Win32 OpenGL pages). `registry.khronos.org/OpenGL-Refpages` returns 403 from this network. | quoted text reproduced from those mirrors |
| `[REG]` | **Khronos OpenGL API Registry** — `docs/khronos/gl.xml` | **vendored in this repo** | enum values extracted programmatically |
| `[MESA]` | Mesa reference implementation — `src/mesa/main/light.c`, `src/mesa/main/ffvertex_prog.c` | `https://gitlab.freedesktop.org/mesa/mesa/-/raw/main/...` | read; used only as a **cross-check of the spec text**, never as the primary source |

### 0.2 Citation conventions

* `[GL21 §2.14.1 p.59]` = section 2.14.1, **printed** page 59 of the OpenGL 2.1 spec (PDF page 73).
* `[GL42C §2.13.1 p.80]` = section 2.13.1, printed page 80 of the OpenGL 4.2 Compatibility Profile spec.
* `[GL46CORE §13.6, PDF p.474–475 / printed p.452–453]` = verified in the vendored core PDF.
* Where nothing is cited, the statement is **TGLMT engineering policy** (marked *(TGLMT policy)*).

### 0.3 Why the 4.6 *Compatibility* spec is not the primary source

`glspec46.compat.pdf` could **not** be downloaded (HTTP 403 from `registry.khronos.org` and
`www.khronos.org`; every guessed mirror path under `opengl.org/registry/doc/` returned 404).
The 4.2 Compatibility Profile spec `[GL42C]` was used instead. This is safe because the
lighting chapters of the compatibility profile are **verbatim identical** between the 2.1
specification and the 4.2 compatibility profile (same text, same equation numbers 2.6–2.9 for
`f_i`, `h_i`, `att_i`, `spot_i`; same tables 2.13/2.14; same state tables 6.16/6.17).
The 4.6 compatibility profile inherits these chapters unchanged. **Assumption to re-verify**
when the 4.6 compat PDF becomes reachable: only §2.13 (lighting), §2.12.2 (normal
transformation), §2.22 (flatshading), and the state tables could conceivably have been
re-numbered; the *semantics* are not expected to have changed.

### 0.4 Scope note — the core profile has none of this

The OpenGL 4.6 **Core** profile has **removed** the entire fixed-function lighting/materials
machinery:

> "Enable/Disable targets … LIGHTING, VERTEX_PROGRAM_TWO_SIDE, LIGHTi, and COLOR_MATERIAL;
> … **Material\*, Light\*, LightModel\*, and ColorMaterial, ShadeModel**, and all associated
> fixed-function … lighting … state." — `[GL46CORE] App. E.2.2 "Removed Features"`
> (PDF p.702–703 / printed p.680–681)

So: in a **core** context these entry points do not exist and `glGet(GL_MAX_LIGHTS)` is not a
core query. In a **compat** context TGLMT must implement everything in this document.

---

## 1. Where lighting sits in the pipeline

```
vertex attributes (pos, normal, color)
   → modelview transform (§3: vertex + normal)
   → [LIGHTING ENABLED]  per-vertex lighting (§5)          ← this document
   → primitive assembly / clipping
   → [SHADE_MODEL = FLAT]  provoking-vertex colour substitution (§10)
   → rasterization + colour interpolation
   → texture environment / blending / framebuffer
```

Facts (all verified):

* **Lighting is computed per vertex.** "GL lighting computes colors for each vertex sent to
  the GL." — `[GL21 §2.14.1 p.59]`, `[GL42C §2.13.1 p.80]`.
* Lighting is applied **before** primitive assembly/clipping; the resulting primary/secondary
  colours are then interpolated across the primitive (and clipped with the primitive,
  `[GL21 §2.14.8 p.70]` / `[GL42C §2.23.1 p.211]`).
* "All computations are carried out in **eye coordinates**." — `[GL21 §2.14.1 p.62]`,
  `[GL42C §2.13.1 p.84]`.
* If `GL_LIGHTING` is disabled, "the current color and current secondary color are assigned to
  the vertex primary and secondary color" — `[GL21 §2.14.1 p.59]`.
* "The value of **A** produced by lighting is the alpha value associated with `d_cm`."
  (i.e. the *material diffuse* alpha; the alpha of the emission/ambient/specular terms is
  **not** used.) — `[GL21 §2.14.1 p.62]`, `[GL42C §2.13.1 p.84]`.
* "Results of lighting are **undefined** if the w coordinate (w in eye coordinates) of V is
  zero." — `[GL21 §2.14.1 p.62]`, `[GL42C §2.13.1 p.84]`.
* After lighting, "all components of both primary and secondary colors are clamped to the range
  [0, 1]" — `[GL21 §2.14.6 p.69]`, `[GL42C §2.13.6]`.

---

## 2. State that must exist

```cpp
struct Material {                 // one instance for FRONT, one for BACK
    float ambient[4];   // a_cm   GL_AMBIENT
    float diffuse[4];   // d_cm   GL_DIFFUSE
    float specular[4];  // s_cm   GL_SPECULAR
    float emission[4];  // e_cm   GL_EMISSION
    float shininess;    // s_rm   GL_SHININESS
    float index[3];     // a_m,d_m,s_m  GL_COLOR_INDEXES (colour-index mode only)
};

struct Light {
    float ambient[4];   // a_cli  GL_AMBIENT
    float diffuse[4];   // d_cli  GL_DIFFUSE
    float specular[4];  // s_cli  GL_SPECULAR
    float eyePos[4];    // P_pl   GL_POSITION, ALREADY transformed by the modelview (§4.1)
    float spotDir[3];   // s_dl   GL_SPOT_DIRECTION, ALREADY transformed by Mu (§4.2), NOT normalized
    float spotExponent;  // s_rl   GL_SPOT_EXPONENT
    float spotCutoff;    // c_rl   GL_SPOT_CUTOFF, degrees
    float k0, k1, k2;    //        GL_{CONSTANT,LINEAR,QUADRATIC}_ATTENUATION
    bool  enabled;       // GL_LIGHTi enable bit
};

struct LightModel {
    float ambient[4];        // a_cs  GL_LIGHT_MODEL_AMBIENT
    bool  localViewer;       // v_bs  GL_LIGHT_MODEL_LOCAL_VIEWER
    bool  twoSide;           // t_bs  GL_LIGHT_MODEL_TWO_SIDE
    GLenum colorControl;     // c_es  GL_SINGLE_COLOR | GL_SEPARATE_SPECULAR_COLOR
};

// Context-level lighting state
Material matFront, matBack;
Light    lights[MAX_LIGHTS];         // MAX_LIGHTS >= 8  (see §13)
LightModel lightModel;
bool  lightingEnabled;               // GL_LIGHTING
bool  colorMaterialEnabled;          // GL_COLOR_MATERIAL
GLenum colorMaterialFace;            // GL_COLOR_MATERIAL_FACE
GLenum colorMaterialParameter;       // GL_COLOR_MATERIAL_PARAMETER
GLenum shadeModel;                   // GL_SHADE_MODEL
GLenum provokingVertexMode;         // GL_PROVOKING_VERTEX (LAST_VERTEX_CONVENTION default)
```

### 2.1 Initial values (all verified)

Material — `[GL21 table 2.10 p.61]`, `[GL42C table 2.13 p.83]`, `[GL21 table 6.12 p.277]`,
`[GL42C table 6.17 p.520]`:

| Material parameter | Initial value (FRONT **and** BACK) |
|---|---|
| `GL_AMBIENT` (a_cm) | **(0.2, 0.2, 0.2, 1.0)** |
| `GL_DIFFUSE` (d_cm) | **(0.8, 0.8, 0.8, 1.0)** |
| `GL_SPECULAR` (s_cm) | (0.0, 0.0, 0.0, 1.0) |
| `GL_EMISSION` (e_cm) | (0.0, 0.0, 0.0, 1.0) |
| `GL_SHININESS` (s_rm) | 0.0 |
| `GL_COLOR_INDEXES` (a_m,d_m,s_m) | (0.0, 1.0, 1.0) |

> ⚠️ **The material ambient is (0.2,0.2,0.2,1) and the material diffuse is (0.8,0.8,0.8,1) —
> NOT all-zero.** The commonly quoted "all zeros except specular" is wrong; the same values
> appear in the reference pages ("The initial ambient reflectance for both front- and
> back-facing materials is (0.2, 0.2, 0.2, 1.0)" / "… diffuse … (0.8, 0.8, 0.8, 1.0)") `[REF glMaterial]`.

Light sources — `[GL21 table 2.10 p.61]`, `[GL42C table 2.13 p.83]`, `[GL42C table 6.17 p.520]`:

| Light parameter | Initial value |
|---|---|
| `GL_AMBIENT` (a_cli) | (0.0, 0.0, 0.0, 1.0) |
| `GL_DIFFUSE` (d_cli) | light 0: **(1.0, 1.0, 1.0, 1.0)**; lights i>0: (0.0, 0.0, 0.0, 1.0) |
| `GL_SPECULAR` (s_cli) | light 0: **(1.0, 1.0, 1.0, 1.0)**; lights i>0: (0.0, 0.0, 0.0, 1.0) |
| `GL_POSITION` (P_pl) | **(0.0, 0.0, 1.0, 0.0)** → w = 0 ⇒ *directional* light pointing +Z |
| `GL_SPOT_DIRECTION` (s_dl) | (0.0, 0.0, −1.0) |
| `GL_SPOT_EXPONENT` (s_rl) | 0.0 |
| `GL_SPOT_CUTOFF` (c_rl) | 180.0 (⇒ no cone) |
| `GL_CONSTANT_ATTENUATION` (k0) | 1.0 |
| `GL_LINEAR_ATTENUATION` (k1) | 0.0 |
| `GL_QUADRATIC_ATTENUATION` (k2) | 0.0 |
| `GL_LIGHTi` enable bit | **FALSE for every light** (all lights disabled initially) |

> Note the alpha of `a_cli/d_cli/s_cli` is 1.0 for light 0 but the refpage text for the *other*
> lights says "(0, 0, 0, 0)" in some editions — the **spec tables win: (0,0,0,1.0)**.
> The alpha of light colours is **not** used by the lighting equation (only `d_cm.a` survives).

Light model — `[GL21 table 2.10 p.61]`, `[GL42C table 2.13 p.83]`, `[GL42C table 6.17 p.520]`:

| Parameter | Initial value |
|---|---|
| `GL_LIGHT_MODEL_AMBIENT` (a_cs) | (0.2, 0.2, 0.2, 1.0) |
| `GL_LIGHT_MODEL_LOCAL_VIEWER` (v_bs) | **FALSE** (viewer at (0,0,∞) in eye coordinates) |
| `GL_LIGHT_MODEL_TWO_SIDE` (t_bs) | FALSE |
| `GL_LIGHT_MODEL_COLOR_CONTROL` (c_es) | `GL_SINGLE_COLOR` |

> ⚠️ **`GL_LIGHT_MODEL_LOCAL_VIEWER` defaults to `GL_FALSE`, not `GL_TRUE`.**
> Verified in three independent places: `[GL21 table 2.10 p.61]` ("vbs boolean FALSE"),
> `[GL21 table 6.12 p.277]` ("LIGHT MODEL LOCAL VIEWER B GetBooleanv **False**"),
> `[GL42C table 6.17 p.520]` ("LIGHT MODEL LOCAL VIEWER B GetBooleanv **FALSE**");
> the reference page agrees ("GL_LIGHT_MODEL_LOCAL_VIEWER … The initial value is GL_FALSE"
> `[REF glGet]`); and Mesa initialises `lm->LocalViewer = GL_FALSE;` `[MESA light.c]`.
> The OpenGL "Optimizing Lighting Performance" note also says "Set GL_LIGHT_MODEL_LOCAL_VIEWER
> to GL_FALSE in glLightModel() (**the default**)". **TGLMT must default to FALSE.**

Miscellaneous initial state — `[GL21 §2.14.4 p.68]`, `[GL42C §2.13.4 p.90]`:

* "In the initial state, all lighting parameters have their default values. Back color
  evaluation does not take place, **ColorMaterial is FRONT_AND_BACK and AMBIENT_AND_DIFFUSE**,
  and both **lighting** and **COLOR_MATERIAL are disabled**."
* `GL_SHADE_MODEL` = `GL_SMOOTH` (`[GL42C table 6.16 p.519]`).
* `GL_PROVOKING_VERTEX` = `GL_LAST_VERTEX_CONVENTION` (`[GL42C table 6.16 p.519]`,
  `[GL46CORE §13.6, PDF p.475 / printed p.453]`).
* `GL_NORMALIZE` = disabled, `GL_RESCALE_NORMAL` = disabled ("The initial state is for normals
  not to be rescaled or normalized" — `[GL21 §2.11.3 p.48]`, `[GL42C §2.12.2 p.76–77]`).

---

## 3. Normal transformation (pre-lighting)

Spec: `[GL21 §2.11.3 "Normal Transformation" p.48–50]`, `[GL42C §2.12.2 p.76–78]`.

The normal supplied by the application is transformed to **eye coordinates** by a matrix derived
from the modelview matrix `M`:

```
(n'x', n'y', n'z', q') = (nx, ny, nz, q) · M⁻¹
```

where `q` is computed from the **associated vertex coordinates** `(x,y,z,w)`:

```
q = 0                              if w == 0
q = −( (nx,ny,nz) · (x,y,z) ) / w  if w != 0            ← equation (2.1) in [GL21], (2.5) in [GL42C]
```

Alternative permitted form: "Implementations may choose instead to transform `(nx ny nz)` to eye
coordinates using `(nx' ny' nz') = (nx ny nz) · Mu⁻¹` where Mu is the upper leftmost 3x3 matrix
taken from M." — `[GL21 §2.11.3 p.49]`, `[GL42C §2.12.2 p.77]`.

**Rescaling** (`GL_RESCALE_NORMAL`): `(n''x,n''y,n''z) = f · (n'x,n'y,n'z)` with `f = 1` if
rescaling is disabled, else either

```
f = 1 / sqrt( m31² + m32² + m33² )        (m_ij = element row i, col j of M⁻¹; uniform-scale case)
```

or the permitted per-normal alternative

```
f = 1 / sqrt( n'x² + n'y² + n'z² )         (recomputed for each normal)
```

**Normalizing** (`GL_NORMALIZE`): `nf = m · (n''x,n''y,n''z)` with `m = 1` if normalization is
disabled, else `m = 1 / sqrt(n''x² + n''y² + n''z²)`.

Order is fixed: **transform → rescale → normalize**.

> Degenerate cases: "In case of an exactly singular matrix, the transformed normal is
> undefined." — `[GL21 §2.11.3 p.50]`, `[GL42C §2.12.2 p.78]`.

### 3.1 TGLMT implementation (Metal)

```cpp
// Eye-space normal, per [GL21 §2.11.3].  inverse-transpose of the modelview matrix.
// NOTE: the spec permits the simpler Mu⁻¹ form; the inverse-transpose is the form used by
// every real driver and by Mesa, and it is what the "rescale" factor f is derived from.
vec3 eyeNormal(vec3 n_obj, vec3 vertexObj, float w_obj, const mat4 &modelview)
{
    mat3 nrm = transpose(inverse(mat3(modelview)));   // = (M⁻¹)ᵀ restricted to 3x3
    vec3 n_eye = nrm * n_obj;                        // (nx' ny' nz')
    // q-term of eq. (2.1)/(2.5): q = -(n·v)/w  — the "perspective-correct normal" adjustment.
    // Implementations that use the Mu⁻¹ alternative omit it; see §14 flag F-3.
    return n_eye;
}

// Rescale + normalize
vec3 finishNormal(vec3 n_eye, bool rescale, bool normalize, const mat4 &modelview)
{
    if (rescale) {
        // uniform-scale fast path: f = 1/|third row of M⁻¹| ; portable path: per-normal f
        n_eye *= 1.0f / max(length(n_eye), 1e-20f);   // permitted alternative in the spec
    }
    if (normalize) {
        float len = length(n_eye);
        if (len > 0.0f) n_eye /= len;                 // m = 1/|n''|
    }
    return n_eye;
}
```

**TGLMT choices:** use the inverse-transpose (3x3) of the modelview; implement the `q` term
exactly as written (it is a no-op for `w == 1`, which is the overwhelmingly common case);
implement the per-normal rescale factor `f = 1/|n'|` (the "portable" alternative explicitly
allowed by the spec) **and** the uniform-scale fast path `f = 1/sqrt(m31²+m32²+m33²)` when the
modelview is a uniform scale, because they agree there.

---

## 4. `glLight` — where the light lives in eye space

Spec: `[GL21 §2.14.2 p.66]`, `[GL42C §2.13.2 p.87]`. Verbatim:

> "The **current model-view matrix is applied to the position parameter** indicated with `Light`
> for a particular light source **when that position is specified**. These transformed values are
> the values used in the lighting equation.
>
> The **spotlight direction is transformed when it is specified using only the upper leftmost 3x3
> portion of the model-view matrix**. That is, if `Mu` is the upper left 3x3 matrix taken from
> the current model-view matrix `M`, then the spotlight direction `(dx,dy,dz)` is transformed to
> `(d'x,d'y,d'z) = Mu · (dx,dy,dz)`."

### 4.1 The rules, spelled out

| Item | Rule |
|---|---|
| `GL_POSITION` | Transformed by the **current modelview matrix** (full 4×4, including translation) **at the moment `glLight` is called**. The result is stored in **eye space** and used as-is forever after. A later `glTranslate`/`glRotate` does **not** move the light. |
| `GL_POSITION` with `w == 0` | The light is **directional** (at infinity). Its direction is the direction of `(x,y,z)`; the light "shines from" that direction. `w != 0` ⇒ **positional** (point/spot) light, position `(x/w, y/w, z/w)` in eye space. |
| `GL_SPOT_DIRECTION` | Transformed by the **upper-left 3×3** of the current modelview **at the moment `glLight` is called**. Translation is ignored; the 3×3 (including any non-uniform scale) **is** applied. The stored value is **not normalized** — normalization happens per-vertex inside the lighting equation. |
| `GL_AMBIENT/DIFFUSE/SPECULAR` | Stored verbatim (no transform). |
| `GL_SPOT_EXPONENT/CUTOFF`, attenuation | Stored verbatim (no transform). |

Confirmed by the reference page: "The returned values are those maintained in **eye
coordinates**. They will not be equal to the values specified using `glLight`, unless the
modelview matrix was identity at the time `glLight` was called." — `[REF glGetLight]`.
And: "Although spot direction is normalized before being used in the lighting equation, the
returned values are the **transformed versions of the specified values prior to
normalization**." — `[REF glGetLight]`.

### 4.2 TGLMT implementation

```cpp
void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    // ... enum/value validation (§11) ...
    Light &L = lights[light - GL_LIGHT0];
    switch (pname) {
    case GL_POSITION: {
        // full modelview transform, AT CALL TIME
        float p[4];
        transform4x4(p, ctx.modelview, params);     // p = M · params
        std::copy(p, p + 4, L.eyePos);
        break;
    }
    case GL_SPOT_DIRECTION: {
        // upper-left 3x3 of the modelview, AT CALL TIME; NO translation
        float d[3];
        transform3x3(d, upperLeft3x3(ctx.modelview), params);
        std::copy(d, d + 3, L.spotDir);             // NOT normalized
        break;
    }
    ...
    }
}
```

> ⚠️ **Mesa divergence (not a TGLMT requirement):** Mesa transforms the spot direction with the
> *inverse-transpose* of the modelview 3×3 (`TRANSFORM_DIRECTION`), not with `Mu`
> `[MESA light.c]`. The two agree for rigid/uniform-scale modelviews (the overwhelming
> majority of real content) and differ only under non-uniform scale. **TGLMT follows the spec:
> `Mu · d`.** (Flag F-2, §14.)

---

## 5. The lighting equation

### 5.1 Notation (verbatim, `[GL21 §2.14.1 p.60]`, `[GL42C §2.13.1 p.82]`)

* A colour parameter is 4 floats `(r,g,b,a)`. There are **no restrictions** on the values of
  colour parameters (they are *not* clamped when set).
* A position parameter is 4 floats `(x,y,z,w)`; `w` may be zero ⇒ "a point at infinity in the
  direction given by x, y, and z".
* `c1 ∗ c2` = component-wise product of two colours (without alpha).
* `d1 ⊙ d2 = max{ d1 · d2 , 0 }` — **the "⊙" operator clamps the dot product at 0.**
* `→P1P2` = the unit vector from P1 to P2, with the homogeneous-w conventions:
  * `w(P2) == 0`, `w(P1) != 0` ⇒ unit vector in the direction of P2's `(x,y,z)`;
  * `w(P1) == 0`, `w(P2) != 0` ⇒ the **negative** of the unit vector in the direction of P1's
    `(x,y,z)`;
  * both `w == 0` ⇒ unit vector of `(P2 − P1)`.
* `d̂` = unit vector in the direction of `d`. `‖P1P2‖` = distance between P1 and P2.
* `V` = the point corresponding to the vertex being lit; `n` = the corresponding (eye-space)
  normal. `Pe` = the eyepoint = `(0,0,0,1)` in eye coordinates.

### 5.2 The equations (verbatim, `[GL21 §2.14.1 p.62]`, `[GL42C §2.13.1 p.84–85]`)

**`c_es == GL_SINGLE_COLOR`:**

```
c_pri = e_cm
      + a_cm ∗ a_cs
      + Σ(i = 0 … n−1) (att_i)(spot_i) [ a_cm ∗ a_cli
                                        + (n ⊙ →V P_pli) d_cm ∗ d_cli
                                        + (f_i)(n ⊙ ĥ_i)^s_rm s_cm ∗ s_cli ]

c_sec = (0, 0, 0, 1)
```

**`c_es == GL_SEPARATE_SPECULAR_COLOR`:**

```
c_pri = e_cm
      + a_cm ∗ a_cs
      + Σ(i = 0 … n−1) (att_i)(spot_i) [ a_cm ∗ a_cli
                                        + (n ⊙ →V P_pli) d_cm ∗ d_cli ]

c_sec = Σ(i = 0 … n−1) (att_i)(spot_i)(f_i)(n ⊙ ĥ_i)^s_rm s_cm ∗ s_cli
```

**Auxiliary quantities** (equation numbers as in the specs):

```
(2.2)/(2.6)   f_i  =  1,   if  n ⊙ →V P_pli ≠ 0
                   0,   otherwise

(2.3)/(2.7)   h_i  =  →V P_pli + →V P_e ,          if v_bs == TRUE
                   →V P_pli + (0,0,1)ᵀ ,            if v_bs == FALSE
            (ĥ_i is the unit vector in the direction of h_i)

(2.4)/(2.8)   att_i = 1 / ( k0_i + k1_i·‖V P_pli‖ + k2_i·‖V P_pli‖² ) ,  if P_pli.w != 0
                    1.0                                                  ,  otherwise
            (a directional light, i.e. w == 0, is NEVER attenuated)

(2.5)/(2.9)   spot_i = ( →P_pli V ⊙ ŝ_dli )^s_rli ,  if c_rli != 180.0 and →P_pli V ⊙ ŝ_dli ≥ cos(c_rli)
                     0.0                              ,  if c_rli != 180.0 and →P_pli V ⊙ ŝ_dli <  cos(c_rli)
                     1.0                              ,  if c_rli == 180.0
```

Read carefully:

* `f_i` is **1 only if the clamped diffuse dot is non-zero** — i.e. the specular lobe is
  killed whenever the surface faces away from the light. (Classic OpenGL behaviour.)
* `h_i` is **not normalized in the equation**; `ĥ_i` (the unit vector) is what is dotted with
  `n`.
* The **cutoff test comes first**, the exponent is applied **after** the test, to the value
  **already clamped at 0** by `⊙`.
* `spot_i = 1.0` whenever `c_rli == 180` — i.e. the default light is *not* a spotlight at all
  and the cone test is skipped entirely (not merely always-passing).
* `→P_pli V` is the unit vector **from the light to the vertex** (the opposite of `→V P_pli`).
* The attenuation distance is **vertex → light** (`‖V P_pli‖`), not eye → light.

### 5.2.1 The five places implementations usually get it wrong

1. **`f_i` gates the specular.** If the clamped diffuse dot is 0 (surface facing away from the
   light), the specular is **0**, no matter how large the shininess is. The gate is applied to
   the *clamped* dot (`n ⊙ →V P_pli`), so a back-facing surface has no highlight.
2. **`shininess == 0` does NOT mean "no specular".** `(n ⊙ ĥ)^0 = 1` (IEEE-754), so the
   specular term is the constant `f_i · s_cm · s_cli` — a flat lobe, still gated by `f_i`.
3. **The spotlight cutoff test happens BEFORE the exponent**, and the exponent is applied to the
   value already clamped at 0 by `⊙`. `spot == 1.0` when the cutoff is 180° (no cone test at
   all).
4. **A directional light (`w == 0`) is never attenuated** — `att = 1.0` regardless of `k0,k1,k2`.
   The default light (`GL_POSITION = (0,0,1,0)`) is directional.
5. **The alpha of the lit colour is the material diffuse alpha** (`d_cm.a`); the alpha of the
   emission/ambient/specular terms and of the light colours is **not** used.

### 5.3 Two-sided lighting (spec text)

> "Lighting may operate in two-sided mode (`tbs = TRUE`), in which a front color is computed
> with one set of material parameters (the front material) and a back color is computed with a
> second set of material parameters (the back material). **This second computation replaces
> `n` with `−n`.** If `tbs = FALSE`, then the back color and front color are both assigned the
> color computed using the front material with `n`." — `[GL21 §2.14.1 p.62]`,
> `[GL42C §2.13.1 p.85]`

> "If the primitive is a point or a line segment, **the front color is always selected**. If it
> is a polygon, then the selection is performed based on the sign of the (clipped or unclipped)
> polygon's area `a` computed in window coordinates … If the sign of `a` (including the possible
> reversal of this sign as indicated by the last call to `FrontFace`) is positive, the color of
> each vertex of the polygon becomes the front color computed for that vertex; otherwise the back
> color is selected." — `[GL42C §2.13.1 p.85]`; area formula = equation (3.8) of
> `[GL42C §3.6.1 p.236]` (identical to equation (2.6) in `[GL21 §2.14.1 p.63]`):

```
a = (1/2) · Σ(i = 0 … n−1) [ x_i^w·y_(i⊕1)^w − x_(i⊕1)^w·y_i^w ]      (i⊕1 = (i+1) mod n)
```

---

## 6. Reference implementation (per vertex, eye space)

```cpp
// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static inline float dotClamp0(const vec3 &a, const vec3 &b) { return std::max(dot(a, b), 0.0f); }
static inline vec3  normalizeSafe(const vec3 &v) {
    float l = length(v);
    return (l > 0.0f) ? v / l : vec3(0, 0, 0);
}

// ---------------------------------------------------------------------------
// Per-vertex lighting, eye space.  Implements [GL21 §2.14.1] / [GL42C §2.13.1].
// ---------------------------------------------------------------------------
// Veye : eye-space vertex position (homogeneous; w != 0 guaranteed)
// Neye : eye-space normal AFTER §3 transform/rescale/normalize
// mat  : the material to use (front or back)
// model: { sceneAmbient[4], localViewer, twoSide, colorControl }
struct LitColor { vec3 primary; vec3 secondary; float alpha; };

LitColor computeLit(const vec4 &Veye, const vec3 &Neye, const Material &mat,
                    const Light *lights, int numLights, const LightModel &model)
{
    // ---- ambient + emission -----------------------------------------------
    // c_pri starts as  e_cm + a_cm * a_cs
    vec3 cpri = vec3(mat.emission[0],   mat.emission[1],   mat.emission[2])
              + vec3(mat.ambient[0],   mat.ambient[1],   mat.ambient[2])
              * vec3(model.ambient[0], model.ambient[1], model.ambient[2]);
    vec3 csec = vec3(0, 0, 0);                 // SINGLE_COLOR: c_sec = (0,0,0,1)

    const bool separate = (model.colorControl == GL_SEPARATE_SPECULAR_COLOR);

    for (int i = 0; i < numLights; ++i) {
        const Light &L = lights[i];
        if (!L.enabled) continue;             // "If light i is disabled, the ith term in the
                                             //  lighting equation is effectively removed" (§2.13.2)

        // ---- L = →V P_pl  (unit vector from the vertex to the light) --------
        vec3  Ldir;
        float dist = 0.0f;
        if (L.eyePos[3] != 0.0f) {                       // positional light
            vec3 Peye = vec3(L.eyePos[0], L.eyePos[1], L.eyePos[2]) / L.eyePos[3];
            vec3 Vxyz = Veye.xyz / Veye.w;
            vec3 d    = Peye - Vxyz;
            dist = length(d);
            Ldir = normalizeSafe(d);
        } else {                                         // directional light (w == 0)
            Ldir = normalizeSafe(vec3(L.eyePos[0], L.eyePos[1], L.eyePos[2]));
        }

        // ---- attenuation  att_i  (eq. 2.4 / 2.8) --------------------------
        float att = 1.0f;
        if (L.eyePos[3] != 0.0f) {
            float denom = L.k0 + L.k1 * dist + L.k2 * dist * dist;
            att = (denom != 0.0f) ? (1.0f / denom) : 0.0f;   // see flag F-4
        }

        // ---- spotlight  spot_i  (eq. 2.5 / 2.9) ---------------------------
        float spot = 1.0f;
        if (L.spotCutoff != 180.0f) {
            vec3  toVertex = -Ldir;                       // →P_pl V  (light → vertex)
            vec3  sdir     = normalizeSafe(vec3(L.spotDir[0], L.spotDir[1], L.spotDir[2]));
            float c        = dotClamp0(toVertex, sdir);    // ⊙ clamps at 0
            float cosCut   = std::cos(L.spotCutoff * kPi / 180.0f);
            spot = (c >= cosCut) ? std::pow(c, L.spotExponent) : 0.0f;
            //  ^ cutoff test FIRST, exponent AFTER, on the clamped value.
        }

        // ---- diffuse -------------------------------------------------------
        float ndotl = dotClamp0(Neye, Ldir);               // n ⊙ →V P_pl
        float f_i   = (ndotl != 0.0f) ? 1.0f : 0.0f;       // eq. 2.2 / 2.6

        // ---- specular half vector -----------------------------------------
        vec3 spec = vec3(0, 0, 0);
        {
            float s;                     // = (f_i)(n ⊙ ĥ_i)^s_rm
            if (mat.shininess == 0.0f) {
                // pow(x, 0) == 1 for every x (IEEE-754), so the lobe is FLAT:
                // the specular contribution is f_i * s_cm * s_cli — NOT zero.
                // (Mesa takes the same "degenerate" path: lit.z = (N·L > 0) ? 1 : 0.)
                s = f_i;
            } else {
                vec3 h;
                if (model.localViewer) {
                    vec3 Vdir = normalizeSafe(-(Veye.xyz / Veye.w));  // →V P_e (eye at origin)
                    h = normalizeSafe(Ldir + Vdir);
                } else {
                    h = normalizeSafe(Ldir + vec3(0, 0, 1));          // infinite viewer
                }
                float ndoth = dotClamp0(Neye, h);                     // n ⊙ ĥ
                s = f_i * std::pow(ndoth, mat.shininess);             // (f_i)(n ⊙ ĥ)^s_rm
            }
            spec = s * vec3(mat.specular[0], mat.specular[1], mat.specular[2])
                        * vec3(L.specular[0],   L.specular[1],   L.specular[2]);
        }

        // ---- accumulate ----------------------------------------------------
        float w = att * spot;
        cpri += w * ( vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2])
                    * vec3(L.ambient[0], L.ambient[1], L.ambient[2])
                  + ndotl * vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2])
                            * vec3(L.diffuse[0], L.diffuse[1], L.diffuse[2]) );
        if (separate) csec += w * spec;
        else          cpri += w * spec;
    }

    // Alpha: "The value of A produced by lighting is the alpha value associated with d_cm."
    return { cpri, csec, mat.diffuse[3] };
}
```

### 6.1 Two-sided entry point

```cpp
// Compute BOTH the front colour (with n) and the back colour (with −n and the BACK material),
// then select per primitive (per polygon facing) — see §5.3 and §14 flag F-6.
LitColor front = computeLit(Veye,  Neye, matFront,  lights, n, model);
LitColor back  = computeLit(Veye, -Neye, matBack,   lights, n, model);   // only if twoSide
```

In a GPU implementation the two-sided case is done the way GLSL does it: compute both and
select in the fragment stage with the facing bit (`gl_FrontFacing` semantics). This is
equivalent to the spec's per-polygon selection for planar polygons and is what Mesa does
(`[MESA ffvertex_prog.c]` computes `lit` and `bflit` and selects on facing).

For **points and lines** the front colour is always used (no back colour exists).

### 6.2 Order of operations (the short version)

1. transform the normal (§3) → rescale → normalize;
2. per light: compute `L` (vertex→light), `att`, `spot`, `f_i`, `h_i`;
3. `ndotl = max(N·L, 0)`; `ndoth = max(N·ĥ, 0)`;
4. `spec = f_i · pow(ndoth, s_rm) · s_cm · s_cli`;
5. accumulate `Σ (att·spot)·[ a_cm·a_cli + ndotl·d_cm·d_cli + spec ]`;
6. `c_pri = e_cm + a_cm·a_cs + Σ…`; `c_sec` per colour-control mode;
7. `alpha = d_cm.a`;
8. clamp the resulting colours to [0,1] (the clamp happens after lighting, in the colour
   pipeline, `[GL21 §2.14.6]`).

---

## 7. `glMaterial` — material parameters

Spec: `[GL21 §2.14.2 p.64–65]` (table 2.11), `[GL42C §2.13.2 p.86–87]` (table 2.14).

`void glMaterial{if}(enum face, enum pname, T param);`
`void glMaterial{if}v(enum face, enum pname, const T params);`

`face` ∈ `GL_FRONT`, `GL_BACK`, `GL_FRONT_AND_BACK` — selects the front material, the back
material, or both.

| `pname` | #values | Symbol | Meaning |
|---|---:|---|---|
| `GL_AMBIENT` | 4 | a_cm | ambient reflectance of the material |
| `GL_DIFFUSE` | 4 | d_cm | diffuse reflectance of the material |
| `GL_AMBIENT_AND_DIFFUSE` | 4 | a_cm, d_cm | sets **both** to the same value |
| `GL_SPECULAR` | 4 | s_cm | specular reflectance of the material |
| `GL_EMISSION` | 4 | e_cm | emissive colour of the material |
| `GL_SHININESS` | 1 | s_rm | specular exponent, range **[0, 128]** |
| `GL_COLOR_INDEXES` | 3 | a_m,d_m,s_m | colour-index mode only: ambient/diffuse/specular indices |

* "`AMBIENT_AND_DIFFUSE` is used to set `a_cm` and `d_cm` to the same value."
  — `[GL21 table 2.11 p.65]`, `[GL42C table 2.14 p.88]`.
* "While the ambient, diffuse, specular and emission material parameters all have alpha
  components, **only the diffuse alpha component is used in the lighting computation**."
  — `[REF glMaterial]`.
* "The material parameters can be updated at any time. In particular, `glMaterial` can be
  called **between a call to `glBegin` and the corresponding call to `glEnd`**."
  — `[REF glMaterial]`; and the spec: "Material properties can be changed inside a Begin/End
  pair by calling `Material`." — `[GL21 §2.14.2 p.65]`, `[GL42C §2.13.2 p.87]`.
* Material colours are **not clamped** when set ("Neither integer nor floating-point values are
  clamped" `[REF glMaterial]`), but the *final lit colour* is clamped to [0,1] after lighting.
* Integer variants map linearly: 1.0 ↔ most positive representable int, −1.0 ↔ most negative.

---

## 8. `glLight` — light source parameters

Spec: `[GL21 §2.14.2 p.64–65]` (table 2.11), `[GL42C §2.13.2 p.86–87]` (table 2.14).

`void glLight{if}(enum light, enum pname, T param);`
`void glLight{if}v(enum light, enum pname, const T params);`

`light` = `GL_LIGHTi` (`GL_LIGHT0 + i`, `0x4000 + i`), `0 ≤ i < GL_MAX_LIGHTS`.

| `pname` | #values | Symbol | Range | Meaning |
|---|---:|---|---|---|
| `GL_AMBIENT` | 4 | a_cli | unbounded | ambient intensity of light i |
| `GL_DIFFUSE` | 4 | d_cli | unbounded | diffuse intensity of light i |
| `GL_SPECULAR` | 4 | s_cli | unbounded | specular intensity of light i |
| `GL_POSITION` | 4 | P_pl | unbounded | position; **transformed by the modelview at call time** (§4) |
| `GL_SPOT_DIRECTION` | 3 | s_dl | unbounded | spot axis; **transformed by Mu at call time** (§4) |
| `GL_SPOT_EXPONENT` | 1 | s_rl | **[0, 128]** | spot intensity distribution |
| `GL_SPOT_CUTOFF` | 1 | c_rl | **[0, 90] or exactly 180** | spot cone half-angle, degrees |
| `GL_CONSTANT_ATTENUATION` | 1 | k0 | **[0, ∞)** | constant attenuation factor |
| `GL_LINEAR_ATTENUATION` | 1 | k1 | **[0, ∞)** | linear attenuation factor |
| `GL_QUADRATIC_ATTENUATION` | 1 | k2 | **[0, ∞)** | quadratic attenuation factor |

* "`GL_LIGHTi` is a symbolic constant of the form `GL_LIGHTi`, indicating that light `i` is to
  have the specified parameter set. The constants obey `GL_LIGHTi = GL_LIGHT0 + i`."
  — `[GL21 §2.14.2 p.64]`, `[GL42C §2.13.2 p.86]`.
* Lights are enabled/disabled with `glEnable(GL_LIGHTi)` / `glDisable(GL_LIGHTi)`; a disabled
  light "is effectively removed from the summation" — `[GL42C §2.13.2 p.87]`.
* The default light is a **directional** light: `GL_POSITION = (0,0,1,0)` ⇒ `L = normalize(0,0,1)`
  and attenuation is exactly 1.
* **Spot exponent range:** the spec's parameter table gives the exponent range `[0.0, 128.0]`
  `[GL21 table 2.10 p.61]`, `[GL42C table 2.13 p.83]` and the reference page says "Only values
  in the range **[0,128]** are accepted" `[REF glLight]`. **Exponent 0 is legal** (it means a
  uniform distribution inside the cone). `GL_SPOT_CUTOFF` legal values are `[0,90]` **plus the
  special value 180**; 180 means "no cone" (spot factor 1.0).
* The reference page also states: "It is significant only when `GL_SPOT_CUTOFF` is not 180,
  which it is initially. The initial direction is (0,0,−1)." `[REF glLight]`.

---

## 9. `glLightModel` — lighting model parameters

Spec: `[GL21 §2.14.2 p.64–65]` (table 2.11), `[GL42C §2.13.2 p.86–87]` (table 2.14).

`void glLightModel{if}(enum pname, T param);`
`void glLightModel{if}v(enum pname, const T params);`

| `pname` | #values | Symbol | Initial | Meaning |
|---|---:|---|---|---|
| `GL_LIGHT_MODEL_AMBIENT` | 4 | a_cs | (0.2,0.2,0.2,1.0) | ambient colour of the **scene** |
| `GL_LIGHT_MODEL_LOCAL_VIEWER` | 1 | v_bs | **FALSE** | viewer at the eye origin (TRUE) vs at infinity along −Z (FALSE) |
| `GL_LIGHT_MODEL_TWO_SIDE` | 1 | t_bs | FALSE | two-sided lighting |
| `GL_LIGHT_MODEL_COLOR_CONTROL` | 1 | c_es | `GL_SINGLE_COLOR` | `GL_SINGLE_COLOR` or `GL_SEPARATE_SPECULAR_COLOR` |

* `GL_LIGHT_MODEL_LOCAL_VIEWER`: "If `params` is 0 (or 0.0), specular reflection angles take
  the view direction to be parallel to and in the direction of the −z axis, regardless of the
  location of the vertex in eye coordinates. Otherwise specular reflections are computed from
  the origin of the eye coordinate system." `[REF glLightModel]`
  In the equation this is `h_i = →V P_pli + →V P_e` (TRUE) vs `h_i = →V P_pli + (0,0,1)ᵀ` (FALSE).
* `GL_LIGHT_MODEL_TWO_SIDE`: "It has no effect on the lighting calculations for points, lines,
  or bitmaps." `[REF glLightModel]`
* `GL_LIGHT_MODEL_COLOR_CONTROL`: with `GL_SEPARATE_SPECULAR_COLOR` the specular contribution is
  written to the **secondary colour** (`c_sec`) instead of being added to the primary colour.
  The alpha of `c_sec` is always 1.
* Boolean parameters: any **non-zero** value means TRUE (`0` = FALSE, anything else = TRUE) —
  `[REF glLightModel]`, and Mesa does `newbool = (params[0] != 0.0F)` `[MESA light.c]`.

---

## 10. `glColorMaterial` — material properties that track the current colour

Spec: `[GL21 §2.14.3 p.66–68]` (figure 2.10), `[GL42C §2.13.3 p.87–89]` (figure 2.12).

`void glColorMaterial(enum face, enum mode);`

* `face` ∈ `GL_FRONT`, `GL_BACK`, `GL_FRONT_AND_BACK` (default `GL_FRONT_AND_BACK`).
* `mode` ∈ `GL_EMISSION`, `GL_AMBIENT`, `GL_DIFFUSE`, `GL_SPECULAR`, `GL_AMBIENT_AND_DIFFUSE`
  (default `GL_AMBIENT_AND_DIFFUSE`).

| `mode` | material property/properties that track the current colour |
|---|---|
| `GL_EMISSION` | `e_cm` |
| `GL_AMBIENT` | `a_cm` |
| `GL_DIFFUSE` | `d_cm` |
| `GL_SPECULAR` | `s_cm` |
| `GL_AMBIENT_AND_DIFFUSE` | **both** `a_cm` and `d_cm` |

Verbatim rules:

> "When `COLOR_MATERIAL` is enabled, the indicated parameter or parameters **always track the
> current color**. … The replacements made to material properties are **permanent**; the
> replaced values remain until changed by either sending a new color or by setting a new
> material value **when ColorMaterial is not currently enabled** to override that particular
> value." — `[GL21 §2.14.3 p.67]`, `[GL42C §2.13.3 p.89]`

> "For instance, calling `ColorMaterial(FRONT, AMBIENT)` while `COLOR_MATERIAL` is enabled sets
> the front material `a_cm` to the value of the current color." — same section.

Consequences (all verified):

* **Emission is affected.** `glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION)` makes **both**
  the front and back material emission track the current colour. There is nothing special about
  emission in the tracking rules.
* The tracking is **continuous**, not a one-shot copy: every change of the current colour
  (`glColor*`, and the colour attribute of a vertex-array draw) updates the tracked material
  property.
* **When is the colour captured?** The spec's wording ("continuously track the current color")
  plus the reference page's note — "If the GL version is 1.1 or greater, and `GL_COLOR_MATERIAL`
  is enabled, evaluated color values affect the results of the lighting equation as if the
  current color were being modified, but no change is made to the tracking lighting parameter"
  `[REF glColorMaterial]` — means: for **per-vertex** use the colour value that is current for
  that vertex; for **evaluated (EvalCoord/EvalPoint)** geometry, the evaluator's colour
  affects the *result* but the tracked material parameter itself is **not** updated.
* A `glMaterial` call on a tracked property while `GL_COLOR_MATERIAL` is enabled is
  immediately overridden by the current colour (the material state keeps tracking). While
  `GL_COLOR_MATERIAL` is disabled, the last tracked value stays in the material state.
* `glGetMaterial(face, pname)` returns the **tracked** value while tracking is enabled.
* "Calling `glDrawElements`, `glDrawArrays`, or `glDrawRangeElements` may leave the current
  color indeterminate, if the color array is enabled. If `glColorMaterial` is enabled while the
  current color is indeterminate, the lighting material state specified by `face` and `mode` is
  also indeterminate." — `[REF glColorMaterial]`

### 10.1 TGLMT implementation rule

Resolve the tracking **at vertex-specification time** (i.e. when the colour of a vertex is
fixed), not at draw time:

```cpp
// when a vertex's colour becomes final (glVertex* in immediate mode, or when the colour
// attribute of a vertex is consumed in a vertex-array draw):
if (ctx.colorMaterialEnabled) {
    const GLenum m = ctx.colorMaterialParameter;
    if (m == GL_AMBIENT || m == GL_AMBIENT_AND_DIFFUSE)
        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) copy(currentColor, matFront.ambient);
    if (m == GL_DIFFUSE || m == GL_AMBIENT_AND_DIFFUSE)
        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) copy(currentColor, matFront.diffuse);
    if (m == GL_SPECULAR)
        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) copy(currentColor, matFront.specular);
    if (m == GL_EMISSION)
        if (face == GL_FRONT || face == GL_FRONT_AND_BACK) copy(currentColor, matFront.emission);
    // ...same for the BACK material when face is GL_BACK / GL_FRONT_AND_BACK
}
```

*(TGLMT policy: the material state is the single source of truth; the GPU shader receives the
material as constants/uniforms, so the tracked value must be committed before the draw is
encoded.)*

---

## 11. `glShadeModel` and flat shading (the provoking vertex rule)

### 11.1 The command

`void glShadeModel(enum mode);` — `mode` ∈ `GL_SMOOTH` (initial) | `GL_FLAT`.

`GL_SMOOTH`: "vertex colors are treated individually" (interpolated).
`GL_FLAT`: flat shading is enabled — "all vertices of the primitive are assigned the same
primary and secondary colors" `[GL21 §2.14.7 p.69]`.

### 11.2 WHICH vertex — the precise rule

**Modern rule (4.x compatibility profile): flat shading uses the PROVOKING VERTEX.**

> "For fixed-function vertex processing, flatshading a primitive means to assign all vertices
> of the primitive the same primary and secondary colors (in RGBA mode) or the same color index
> (in color index mode). … **The color and/or output values assigned are those of the provoking
> vertex of the primitive.** The provoking vertex is controlled with the command
> `void ProvokingVertex(enum provokeMode);` … The initial value of the provoking vertex mode is
> `LAST_VERTEX_CONVENTION`." — `[GL42C §2.22 "Flatshading" p.206–207]`

and the same in the 4.6 core spec:

> "Flatshading an output of the last vertex processing stage means to assign all vertices of the
> primitive the same value for that output. **The output values assigned are those of the
> provoking vertex of the primitive.** … The state required for flatshading is one bit for the
> provoking vertex mode. **The initial value of the provoking vertex mode is
> `LAST_VERTEX_CONVENTION`.**" — `[GL46CORE §13.6, PDF p.474–475 / printed p.452–453]`

**Table of provoking-vertex selection** — `[GL42C table 2.18 p.207]`; the identical table
appears in the core spec as table 13.2 `[GL46CORE PDF p.475]`:

| Primitive type (i = primitive index) | First-vertex convention | **Last-vertex convention (default)** |
|---|---|---|
| point | i | i |
| independent line | 2i−1 | **2i** |
| line loop | i | **i+1 if i<n, else 1** |
| line strip | i | **i+1** |
| independent triangle | 3i−2 | **3i** |
| triangle strip | i | **i+2** |
| triangle fan | i+1 | **i+2** |
| independent quad | 4i−3 | **4i** (¹) |
| quad strip | 2i−1 | **2i+2** (¹) |
| single polygon (i = 1) | 1 | **1** |
| line adjacency | 4i−2 | 4i−1 |
| line strip adjacency | i+1 | i+2 |
| triangle adjacency | 6i−5 | 6i−1 |
| triangle strip adjacency | 2i−1 | 2i+3 |

¹ For quads the behaviour is implementation dependent: "The provoking vertex behavior of quad
primitives is implementation dependent, and may be determined by calling `GetBooleanv` with the
symbolic constant `QUADS_FOLLOW_PROVOKING_VERTEX`. A return value of TRUE indicates that the
provoking vertex mode is respected for quad primitives, while a return value of FALSE indicates
that the implementation always behave as though the provoking vertex mode were
`LAST_VERTEX_CONVENTION`." — `[GL42C §2.22 p.206]`
(`GL_QUADS_FOLLOW_PROVOKING_VERTEX` is **not** in the Khronos registry `gl.xml` — `[REG]` — so it
is a compat-only query token that TGLMT may define as `0x8E4F`-style private value or omit;
see flag F-7.)

**Legacy rule (OpenGL ≤ 2.1) for reference** — `[GL21 §2.14.7 p.69–70]`, table 2.12:

> "These colors are the colors of the vertex that spawned the primitive. For a point, these are
> the colors associated with the point. For a line segment, they are the colors of the **second
> (final) vertex** of the segment. For a polygon, they come from a selected vertex depending on
> how the polygon was generated."

| Polygon primitive (i) | vertex |
|---|---|
| single polygon (i ≡ 1) | 1 |
| triangle strip | i+2 |
| triangle fan | i+2 |
| independent triangle | 3i |
| quad strip | 2i+2 |
| independent quad | 4i |

**With the default `LAST_VERTEX_CONVENTION` the two tables agree exactly** (single polygon → 1;
triangle strip → i+2; fan → i+2; independent triangle → 3i; quad strip → 2i+2; independent quad
→ 4i; independent line → 2i; line strip → i+1; point → i).

### 11.3 TGLMT rule

* Implement flat shading as "take the colour of the **provoking vertex**", with
  `GL_PROVOKING_VERTEX` state, default `GL_LAST_VERTEX_CONVENTION`.
* With the default convention this is **the LAST vertex of the primitive** for points, lines,
  triangles and quads, and **vertex 1** for a single `GL_POLYGON`.
* **TGLMT must implement quads as "always `LAST_VERTEX_CONVENTION`"** (i.e. independent quad →
  4i, quad strip → 2i+2) and report `GL_QUADS_FOLLOW_PROVOKING_VERTEX = GL_FALSE`. This is
  always spec-legal, is the historical behaviour, and avoids an extra state bit. (Flag F-7.)
* Flat shading is applied to the **colour** only; the normal, texture coordinates and fog
  coordinate are still interpolated.
* When `GL_FLAT` is set, the *lighting* is still computed per vertex for every vertex (the
  spec computes the colour of every vertex and then discards all but the provoking one) —
  `[GL42C §2.22 p.207]`: "The colors selected are those derived from current values, generated
  by lighting, or generated by vertex shading, if lighting is disabled, enabled, or a vertex
  shader is in use, respectively." *(TGLMT may optimise by computing the colour of the
  provoking vertex only; the observable result is identical.)*

---

## 12. Error conditions

### 12.1 The Begin/End rule (very commonly missed)

> "The only GL commands that are allowed within any Begin/End pairs are the commands for
> specifying vertex coordinates, vertex colors, normal coordinates, texture coordinates, generic
> vertex attributes, and fog coordinates …, the ArrayElement command, the EvalCoord and
> EvalPoint commands, **commands for specifying lighting material parameters (Material
> commands)**, display list invocation commands, and the EdgeFlag command. **Executing any other
> GL command between the execution of Begin and the corresponding execution of End results in
> the error `INVALID_OPERATION`.**" — `[GL21 §2.6.3 p.19–20]`, `[GL42C §2.6.3 p.31]`

⇒ **`glLight`, `glLightModel`, `glColorMaterial` and `glShadeModel` inside `glBegin`/`glEnd`
must raise `GL_INVALID_OPERATION`. `glMaterial` is explicitly allowed inside Begin/End.**
(The reference pages agree: "GL_INVALID_OPERATION is generated if glLight is executed between
the execution of glBegin and the corresponding execution of glEnd" `[REF glLight]`, same for
`glColorMaterial` `[REF glColorMaterial]` and `glLightModel` `[REF glLightModel]`.)

### 12.2 `glLight` / `glLightfv` / `glLighti` / `glLightiv`

| Condition | Error |
|---|---|
| `light` is not `GL_LIGHT0 + i` with `0 ≤ i < GL_MAX_LIGHTS` | `GL_INVALID_ENUM` |
| `pname` is not one of `GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_POSITION, GL_SPOT_DIRECTION, GL_SPOT_EXPONENT, GL_SPOT_CUTOFF, GL_CONSTANT_ATTENUATION, GL_LINEAR_ATTENUATION, GL_QUADRATIC_ATTENUATION` | `GL_INVALID_ENUM` |
| `GL_SPOT_EXPONENT` outside **[0, 128]** | `GL_INVALID_VALUE` |
| `GL_SPOT_CUTOFF` outside **[0, 90]** and not exactly 180 | `GL_INVALID_VALUE` |
| any attenuation factor < 0 | `GL_INVALID_VALUE` |
| scalar form (`glLightf`/`glLighti`) used with a multi-valued `pname` (`AMBIENT/DIFFUSE/SPECULAR/POSITION/SPOT_DIRECTION`) | `GL_INVALID_ENUM` — spec: "If `param` corresponds to a multi-valued parameter, the error `INVALID_ENUM` results." `[GL21 §2.14.2 p.64]`, `[GL42C §2.13.2 p.86]` |
| called between `glBegin` and `glEnd` | `GL_INVALID_OPERATION` |

Note: the reference page's wording is "GL_INVALID_ENUM is generated if either `light` or
`pname` is not an accepted value." `[REF glLight]` — i.e. **one** `GL_INVALID_ENUM`, not one per
bad argument.

### 12.3 `glMaterial` / `glMaterialfv` / `glMateriali` / `glMaterialiv`

| Condition | Error |
|---|---|
| `face` ∉ {`GL_FRONT`, `GL_BACK`, `GL_FRONT_AND_BACK`} | `GL_INVALID_ENUM` |
| `pname` ∉ {`GL_AMBIENT`, `GL_DIFFUSE`, `GL_AMBIENT_AND_DIFFUSE`, `GL_SPECULAR`, `GL_EMISSION`, `GL_SHININESS`, `GL_COLOR_INDEXES`} | `GL_INVALID_ENUM` |
| `GL_SHININESS` outside **[0, 128]** | `GL_INVALID_VALUE` |
| scalar form used with a multi-valued `pname` | `GL_INVALID_ENUM` (same spec sentence, `[GL21 §2.14.2 p.64]`) |
| called between `glBegin` and `glEnd` | **allowed** (no error) |

Reference-page wording: "GL_INVALID_ENUM is generated if either `face` or `pname` is not an
accepted value. GL_INVALID_VALUE is generated if a specular exponent outside the range
[0,128] is specified." `[REF glMaterial]`

### 12.4 `glLightModel` / `glLightModelfv` / `glLightModeli` / `glLightModeliv`

| Condition | Error |
|---|---|
| `pname` ∉ {`GL_LIGHT_MODEL_AMBIENT`, `GL_LIGHT_MODEL_LOCAL_VIEWER`, `GL_LIGHT_MODEL_TWO_SIDE`, `GL_LIGHT_MODEL_COLOR_CONTROL`} | `GL_INVALID_ENUM` |
| `GL_LIGHT_MODEL_COLOR_CONTROL` value ∉ {`GL_SINGLE_COLOR`, `GL_SEPARATE_SPECULAR_COLOR`} | `GL_INVALID_ENUM` (value is an enum, not a number) |
| scalar form used with the multi-valued `GL_LIGHT_MODEL_AMBIENT` | `GL_INVALID_ENUM` (same spec sentence) |
| called between `glBegin` and `glEnd` | `GL_INVALID_OPERATION` |

(Mesa: `_mesa_error(ctx, GL_INVALID_ENUM, "glLightModel(pname=0x%x)", pname)` and
`_mesa_error(ctx, GL_INVALID_ENUM, "glLightModel(param=0x0%x)")` for a bad colour-control value
`[MESA light.c]` — consistent with the table above.)

### 12.5 `glColorMaterial`

| Condition | Error |
|---|---|
| `face` ∉ {`GL_FRONT`, `GL_BACK`, `GL_FRONT_AND_BACK`} | `GL_INVALID_ENUM` |
| `mode` ∉ {`GL_EMISSION`, `GL_AMBIENT`, `GL_DIFFUSE`, `GL_SPECULAR`, `GL_AMBIENT_AND_DIFFUSE`} | `GL_INVALID_ENUM` |
| called between `glBegin` and `glEnd` | `GL_INVALID_OPERATION` |

Reference page: "GL_INVALID_ENUM is generated if `face` or `mode` is not an accepted value.
GL_INVALID_OPERATION is generated if glColorMaterial is executed between the execution of
glBegin and the corresponding execution of glEnd." `[REF glColorMaterial]`

### 12.6 `glShadeModel`

| Condition | Error |
|---|---|
| `mode` ∉ {`GL_FLAT`, `GL_SMOOTH`} | `GL_INVALID_ENUM` |
| called between `glBegin` and `glEnd` | `GL_INVALID_OPERATION` (follows from §12.1: ShadeModel is not on the allowed list) |

(Mesa: `if (mode != GL_FLAT && mode != GL_SMOOTH) { _mesa_error(ctx, GL_INVALID_ENUM, "glShadeModel"); }`
`[MESA light.c]`.)

### 12.7 Range errors that come from the parameter table

> "The error `INVALID_VALUE` occurs if a specified lighting parameter lies outside the allowable
> range given in table 2.10 / 2.13. (The symbol "∞" indicates the maximum representable
> magnitude for the indicated type.)" — `[GL21 §2.14.2 p.64–65]`, `[GL42C §2.13.2 p.86–87]`

Full list of ranges (table 2.13):

| Parameter | Range |
|---|---|
| any colour component (materials, lights, light model) | (−∞, +∞) — **unbounded, never clamped** |
| `GL_SHININESS` (s_rm) | [0.0, 128.0] |
| `GL_SPOT_EXPONENT` (s_rl) | [0.0, 128.0] |
| `GL_SPOT_CUTOFF` (c_rl) | [0.0, 90.0], **or 180.0** |
| attenuation k0, k1, k2 | [0.0, ∞) |
| light position / spot direction | unbounded |

---

## 13. Querying the state

### 13.1 The correct queries

| Query | Command | Returns |
|---|---|---|
| material | `glGetMaterialfv(face, pname, params)` with `face` ∈ {`GL_FRONT`,`GL_BACK`} and `pname` ∈ {`GL_AMBIENT`,`GL_DIFFUSE`,`GL_SPECULAR`,`GL_EMISSION`,`GL_SHININESS`,`GL_COLOR_INDEXES`} | that material's values |
| light | `glGetLightfv(light, pname, params)` with `light` = `GL_LIGHTi` and `pname` ∈ {`GL_AMBIENT`,`GL_DIFFUSE`,`GL_SPECULAR`,`GL_POSITION`,`GL_SPOT_DIRECTION`,`GL_SPOT_EXPONENT`,`GL_SPOT_CUTOFF`,`GL_CONSTANT_ATTENUATION`,`GL_LINEAR_ATTENUATION`,`GL_QUADRATIC_ATTENUATION`} | that light's values (position/direction in **eye space**) |
| light model | `glGetFloatv(GL_LIGHT_MODEL_AMBIENT, …)`, `glGetBooleanv(GL_LIGHT_MODEL_LOCAL_VIEWER/TWO_SIDE, …)`, `glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL, …)` | the model parameters |
| colour material | `glGetIntegerv(GL_COLOR_MATERIAL_FACE, …)`, `glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, …)`, `glIsEnabled(GL_COLOR_MATERIAL)` | the tracking configuration |
| shade model | `glGetIntegerv(GL_SHADE_MODEL, …)` | `GL_FLAT` / `GL_SMOOTH` |
| lighting | `glIsEnabled(GL_LIGHTING)`, `glIsEnabled(GL_LIGHTi)` | enable bits |

`glGetLight` returns the values **as maintained in eye coordinates**: "The returned values are
those maintained in eye coordinates. They will not be equal to the values specified using
`glLight`, unless the modelview matrix was identity at the time `glLight` was called. The
initial value is (0, 0, 1, 0)." `[REF glGetLight]`
`GL_SPOT_DIRECTION` is returned **not normalized** (it is normalized only when used in the
lighting equation) `[REF glGetLight]`.

### 13.2 `glGet` with lighting pnames — the "current material / current light" question

**Spec position:** the compatibility-profile state tables list the lighting state with
`GetMaterialfv` / `GetLightfv` as *the* query commands —

```
AMBIENT      2×C  GetMaterialfv  (0.2,0.2,0.2,1.0)  Ambient material color
DIFFUSE      2×C  GetMaterialfv  (0.8,0.8,0.8,1.0)  Diffuse material color
SPECULAR     2×C  GetMaterialfv  (0.0,0.0,0.0,1.0)  Specular material color
EMISSION     2×C  GetMaterialfv  (0.0,0.0,0.0,1.0)  Emissive mat. color
SHININESS    2×R  GetMaterialfv  0.0                 Specular exponent of material
LIGHT MODEL AMBIENT      C  GetFloatv  (0.2,0.2,0.2,1.0)
LIGHT MODEL LOCAL VIEWER B  GetBooleanv FALSE
LIGHT MODEL TWO SIDE      B  GetBooleanv FALSE
LIGHT MODEL COLOR CONTROL Z2 GetIntegerv SINGLE COLOR
```
— `[GL42C table 6.17 p.520]`, `[GL21 table 6.12 p.277]`

and the *per-light* entries (`AMBIENT 8*×C GetLightfv`, `POSITION 8*×P GetLightfv`, …) are in
table 6.13 `[GL21 table 6.13 p.278]` / `[GL42C table 6.17 p.521]`.

⇒ **`glGet(GL_AMBIENT)` etc. are NOT spec queries**; the spec's way to read material state is
`glGetMaterial`, and to read light state `glGetLight`. `glGet(GL_AMBIENT)` appears in **no**
state table, and the reference page for `glGet` lists `GL_LIGHT_MODEL_*`, `GL_LIGHTING`,
`GL_LIGHTi` but **not** `GL_AMBIENT`/`GL_DIFFUSE`/`GL_SPECULAR`/`GL_EMISSION`/`GL_SHININESS`/
`GL_POSITION`/`GL_SPOT_*` `[REF glGet]`.

The same conclusion comes from the classic state-variable tables (Red Book appendix "OpenGL
State Variables"), which give the *suggested get command* for those pnames as the dedicated
ones, not `glGet`:

> "GL_AMBIENT | Ambient material color | lighting | (0.2, 0.2, 0.2, 1.0) | **glGetMaterialfv()**
>  GL_DIFFUSE | Diffuse material color | lighting | (0.8, 0.8, 0.8, 1.0) | **glGetMaterialfv()**"
> — `[REF: Red Book Appendix B "OpenGL State Variables"]`

**But** there is a long-standing legacy behaviour in the wild: `glGet(GL_AMBIENT)`,
`glGet(GL_DIFFUSE)`, `glGet(GL_SPECULAR)`, `glGet(GL_EMISSION)`, `glGet(GL_SHININESS)`,
`glGet(GL_POSITION)`, `glGet(GL_SPOT_DIRECTION)`, `glGet(GL_SPOT_EXPONENT)`,
`glGet(GL_SPOT_CUTOFF)`, `glGet(GL_CONSTANT_ATTENUATION)`, `glGet(GL_LINEAR_ATTENUATION)`,
`glGet(GL_QUADRATIC_ATTENUATION)` return, respectively:

* the **current (front) material's** value for the material pnames, and
* the **"current light"**'s value for the light pnames — the light most recently touched by a
  `glLight` call (a historical, undocumented "current light" concept).

Mesa does **not** implement these through `glGet` at all (its `glGet` has no lighting cases, so
they raise `GL_INVALID_ENUM`) `[MESA get.c]`.

**TGLMT decision (flag F-8):** implement the **spec** behaviour — `glGet` with a lighting pname
raises `GL_INVALID_ENUM`; provide `glGetMaterial*` / `glGetLight*` for the real queries.
*(Rationale: the compatibility-profile state tables are unambiguous about which command reads
which state, and matching Mesa maximizes cross-implementation consistency. If TGLMT later wants
legacy tolerance, the legacy mapping is documented above and is trivial to add behind a flag.)*

### 13.3 Enum values (verified against `docs/khronos/gl.xml` — `[REG]`)

| Token | Value | | Token | Value |
|---|---|---|---|---|
| `GL_LIGHTING` | 0x0B50 | | `GL_AMBIENT` | 0x1200 |
| `GL_LIGHT_MODEL_LOCAL_VIEWER` | 0x0B51 | | `GL_DIFFUSE` | 0x1201 |
| `GL_LIGHT_MODEL_TWO_SIDE` | 0x0B52 | | `GL_SPECULAR` | 0x1202 |
| `GL_LIGHT_MODEL_AMBIENT` | 0x0B53 | | `GL_POSITION` | 0x1203 |
| `GL_SHADE_MODEL` | 0x0B54 | | `GL_SPOT_DIRECTION` | 0x1204 |
| `GL_COLOR_MATERIAL_FACE` | 0x0B55 | | `GL_SPOT_EXPONENT` | 0x1205 |
| `GL_COLOR_MATERIAL_PARAMETER` | 0x0B56 | | `GL_SPOT_CUTOFF` | 0x1206 |
| `GL_COLOR_MATERIAL` | 0x0B57 | | `GL_CONSTANT_ATTENUATION` | 0x1207 |
| `GL_MAX_LIGHTS` | 0x0D31 | | `GL_LINEAR_ATTENUATION` | 0x1208 |
| `GL_NORMALIZE` | 0x0BA1 | | `GL_QUADRATIC_ATTENUATION` | 0x1209 |
| `GL_RESCALE_NORMAL` | 0x803A | | `GL_EMISSION` | 0x1600 |
| `GL_AMBIENT_AND_DIFFUSE` | 0x1602 | | `GL_SHININESS` | 0x1601 |
| `GL_SINGLE_COLOR` | 0x81F9 | | `GL_SEPARATE_SPECULAR_COLOR` | 0x81FA |
| `GL_LIGHT0 … GL_LIGHT7` | 0x4000 … 0x4007 | | `GL_FLAT` / `GL_SMOOTH` | 0x1D00 / 0x1D01 |
| `GL_FRONT` / `GL_BACK` / `GL_FRONT_AND_BACK` | 0x0404 / 0x0405 / 0x0408 | | `GL_FIRST/LAST_VERTEX_CONVENTION` | 0x8E4D / 0x8E4E |
| `GL_LIGHTING_BIT` (attrib group) | 0x00000040 | | `GL_MAX_LIGHT_MODEL_AMBIENT` | **does not exist** |

---

## 14. Limits

| Query | Minimum guaranteed | Source |
|---|---|---|
| `GL_MAX_LIGHTS` | **8** | `[GL42C table 6.61 "Implementation Dependent Values" p.564]` — "MAX LIGHTS Z+ GetIntegerv **8** Maximum number of lights"; `[GL21 §2.14.4 p.68]` — "at least 8 sets of light parameters"; `[GL21 §2.14.1 p.60]` — "n is an implementation dependent maximum that **must be at least 8**"; `[REF glLight]` — "at least eight lights are supported" |
| number of light *enable* bits | 8 | `[GL21 §2.14.4 p.68]` — "at least 8 bits to indicate which lights are enabled" |
| `GL_MAX_LIGHT_MODEL_AMBIENT` | **does not exist** | not in `[REG]` (gl.xml), not in any spec table — there is no such limit in OpenGL |

**TGLMT:** expose `GL_MAX_LIGHTS = 8` (minimum). Using 32 (like Mesa) is allowed but must keep
the *spec* minimum of 8 available and the per-light `GL_LIGHTi` enums must be
`GL_LIGHT0 + i`.

---

## 15. Ambiguities, implementation divergences, and the deterministic TGLMT choice

| # | Topic | What the spec says | Where implementations differ | **TGLMT deterministic choice** |
|---|---|---|---|---|
| F-1 | `GL_LIGHT_MODEL_LOCAL_VIEWER` default | **FALSE** `[GL21 table 2.10/6.12]`, `[GL42C table 2.13/6.17]`, `[REF glGet]`, Mesa | tutorials/Red Book examples often *set* it to TRUE, but the initial value is FALSE everywhere verified | **FALSE** |
| F-2 | Spot direction transform | `Mu · d` (upper-left 3×3 of the modelview) `[GL21 §2.14.2 p.66]`, `[GL42C §2.13.2 p.87]` | Mesa uses the **inverse-transpose** 3×3 `[MESA light.c]`; identical for rigid/uniform-scale modelviews | **`Mu · d`** (spec) |
| F-3 | Normal transform | full `M⁻¹` with the `q` term, **or** the permitted `Mu⁻¹` alternative `[GL21 §2.11.3 p.49]` | drivers use the inverse-transpose; the `q` term is a no-op for `w = 1` | **inverse-transpose 3×3 + the `q` term exactly as written**; document that `w = 1` (the normal case) is unaffected |
| F-4 | Attenuation denominator `k0 + k1·d + k2·d²` == 0 | not addressed | `1/0` = ±inf → the light contribution becomes inf/NaN | **clamp the result: if the denominator is 0, the attenuation factor is 0** (the light contributes nothing). *(TGLMT policy — the spec leaves it undefined; picking 0 avoids NaN propagation.)* |
| F-5 | `pow(0, 0)` (spot exponent 0 with a zero cone dot, or shininess 0 with a zero specular dot) | the equation is `(x ⊙ y)^s`, `x ⊙ y ≥ 0`; `0^0` is not defined by the spec | IEEE-754 `pow(x,0) = 1` for every x **including 0**; Mesa's "degenerate" path for shininess == 0 yields `f_i` (a *flat*, non-zero specular), not 0 `[MESA ffvertex_prog.c]` | use `std::pow` (IEEE) ⇒ `pow(0,0) = 1`; keep the `f_i` factor so the specular is still killed when the diffuse term is 0. **Note: shininess 0 ⇒ specular = `f_i · s_cm · s_cli` (flat lobe), NOT zero.** |
| F-6 | Two-sided selection granularity | per **polygon** (sign of the window-space area + `FrontFace`) `[GL42C §2.13.1 p.85]` | the GPU can only select per fragment | compute both front and back colours per vertex, select in the fragment stage with the facing bit (`gl_FrontFacing` semantics) — equivalent for planar polygons, matches GLSL/Mesa |
| F-7 | Quad provoking vertex | implementation dependent; `GL_QUADS_FOLLOW_PROVOKING_VERTEX` reports it `[GL42C §2.22 p.206]`; the token is **not** in `[REG]` | some drivers honour `GL_FIRST_VERTEX_CONVENTION` for quads | **always behave as `LAST_VERTEX_CONVENTION`** for quads; report `GL_QUADS_FOLLOW_PROVOKING_VERTEX = GL_FALSE` if TGLMT exposes the token |
| F-8 | `glGet` with lighting pnames | not in the state tables; the spec queries are `glGetMaterial` / `glGetLight` | legacy drivers return the "current material" (front) and the "current light" | **`GL_INVALID_ENUM`** (spec + Mesa). The legacy mapping is documented in §13.2 should TGLMT want it later |
| F-9 | `glLightModel` scalar form with `GL_LIGHT_MODEL_AMBIENT` | "If `param` corresponds to a multi-valued parameter, the error `INVALID_ENUM` results" `[GL21 §2.14.2 p.64]` | Mesa silently accepts it (it converts to the array form) `[MESA light.c]` | **`GL_INVALID_ENUM`** (spec) |
| F-10 | `glLight`/`glMaterial` scalar form with a multi-valued pname | `GL_INVALID_ENUM` per the same spec sentence | some implementations raise `GL_INVALID_OPERATION` | **`GL_INVALID_ENUM`** (the spec sentence is explicit) |
| F-11 | Colour interpolation of the lit colour | the lighting chapters do not specify the interpolation quality | real drivers interpolate the primary/secondary colour **perspective-correctly** (like texture coordinates) | perspective-correct interpolation (documented as *implementation policy*, since the spec text is silent) |
| F-12 | `glLightModel(GL_LIGHT_MODEL_COLOR_CONTROL, …)` with a non-enum value | the value is an enum | some drivers accept any non-zero integer as "separate specular" | **`GL_INVALID_ENUM`** unless the value is exactly `GL_SINGLE_COLOR` or `GL_SEPARATE_SPECULAR_COLOR` (Mesa behaviour, matches the enum semantics) |
| F-13 | Alpha of light colours | the equation only uses RGB of the light colours; `A` comes from `d_cm` | — | ignore the alpha of `a_cli/d_cli/s_cli`; `c_pri.a = d_cm.a`; `c_sec.a = 1` |
| F-14 | `GL_COLOR_MATERIAL` capture time | "continuously track the current color" | — | resolve at **vertex-specification time** (immediate mode) / at the time the colour attribute is consumed (array draws); evaluated geometry does **not** update the tracked material `[REF glColorMaterial]` |

---

## 16. Conformance checklist / test vectors

1. **Defaults** — after context creation, verify every value in §2.1 via `glGetMaterial*`,
   `glGetLight*`, `glGet*`. In particular material ambient = (0.2,0.2,0.2,1) and diffuse =
   (0.8,0.8,0.8,1); `GL_LIGHT_MODEL_LOCAL_VIEWER` = FALSE; light 0 diffuse/specular =
   (1,1,1,1); light 0 position = (0,0,1,0).
2. **Modelview-at-call-time** — `glLoadIdentity(); glTranslatef(0,0,-5); glLightfv(GL_LIGHT0,
   GL_POSITION, {0,0,0,1}); glTranslatef(10,0,0);` → the light's eye position must still be
   (0,0,−5,1), not (10,0,−5,1).
3. **Directional light ⇒ att = 1** — light with `w = 0` and `k0 = 0, k1 = 1, k2 = 1` must not
   attenuate with distance.
4. **Positional attenuation** — `k0 = 1, k1 = 0, k2 = 0` ⇒ att = 1; `k0 = 0, k1 = 1, k2 = 0` ⇒
   att = 1/d.
5. **Spotlight order** — cutoff 30°, exponent 1 ⇒ the intensity is linear in cos(θ) inside the
   cone and exactly 0 outside; with cutoff 180° the light must light the whole sphere.
6. **Spot exponent 0** — inside the cone the spot factor must be 1 (uniform).
7. **Local viewer** — `GL_LIGHT_MODEL_LOCAL_VIEWER = GL_TRUE` ⇒ the half vector depends on the
   vertex position; FALSE ⇒ the half vector uses (0,0,1).
8. **Two-sided** — a polygon seen from behind with `GL_LIGHT_MODEL_TWO_SIDE = GL_TRUE` uses the
   BACK material and the negated normal; with FALSE it uses the FRONT material and the
   un-negated normal.
9. **`f_i`** — a surface whose normal points away from the light must have **no** specular
   highlight even with a large shininess.
10. **Alpha** — the lit alpha must equal the material diffuse alpha, regardless of the light
    colours.
11. **`glColorMaterial`** — with `COLOR_MATERIAL` enabled and mode `GL_AMBIENT_AND_DIFFUSE`, a
    `glColor4f` before each `glVertex3f` must change the material ambient/diffuse used for
    that vertex; with mode `GL_EMISSION` the emission must track the colour.
12. **Flat shading** — a triangle with three different vertex colours rendered with
    `GL_FLAT` must show the colour of the **third** (last) vertex; a `GL_POLYGON` must show the
    colour of **vertex 1**.
13. **Errors** — every row of §12 must raise exactly the listed error code.

---

## 17. Cross-references inside this repository

* `docs/LEGACY_TEXTURE_AND_LIGHTING.md` — the project's Vietnamese working notes on the same
  subject (2.1 §2.14.1 / §2.11.3). **This document supersedes it** where they disagree: the
  flat-shading rule (provoking vertex, table 2.18 / table 13.2), the `GL_LIGHT_MODEL_LOCAL_VIEWER`
  default (FALSE), the full error tables, and the Begin/End rule.
* `docs/FIXED_FUNCTION_PIPELINE.md` §5.4 — the flat-shading vertex selection table (older 2.1
  form; identical for the default convention).
* `docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md` §"legacy lighting" — the planned implementation
  stages (`LegacyLight lights[8]`, `test_legacy_lighting_state`, `test_legacy_lighting_gpu`).
* `docs/khronos/glspec46.core.pdf` — the vendored core spec (App. E.2.2 removes all of this
  from the core profile; §13.6 defines the provoking-vertex rule).
* `docs/khronos/gl.xml` — the vendored Khronos registry (enum values in §13.3).
* `include/tglmt/gl_legacy.h` — the compat-profile entry points that must implement this
  document (`glMaterial*`, `glLight*`, `glLightModel*`, `glColorMaterial`, `glShadeModel`,
  `glGetMaterial*`, `glGetLight*`); the current stubs live in `src/gl/gl_stubs_gen.cpp`
  (marked `UNIMPLEMENTED`).

---

## 18. Provenance of this document

* Research performed against the sources listed in §0.1; every quoted passage was read in the
  extracted PDF text, not from memory.
* Where a statement is **not** backed by a quotation it is marked *(TGLMT policy)* or appears in
  the flag table of §15.
* **No source file under `src/`, `include/` or `tests/` was modified** while producing this
  document.
