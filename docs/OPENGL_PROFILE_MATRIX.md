# OpenGL Core / Compatibility Profile Matrix (Desktop GL 1.0 - 4.6)

Scope: how the **Desktop OpenGL** API surface splits into the **Core profile**
and the **Compatibility profile**, version by version, for OpenGL 1.0 through
OpenGL 4.6.

This document is a **specification inventory**. Every count and every list
below is derived from the Khronos OpenGL registry (`docs/khronos/gl.xml`) and
the vendored OpenGL 4.6 Core Profile specification PDF. It makes **no claim
about CTS conformance** and **no claim about runtime behaviour** of any
implementation.

The measured implementation status of TGLMT against this inventory is a
separate document: [`docs/OPENGL_API_COVERAGE.md`](OPENGL_API_COVERAGE.md).

---

## 0. Provenance and reproduction

| Input | What it is | How it is used |
| --- | --- | --- |
| `docs/khronos/gl.xml` | Khronos OpenGL-Registry, vendored byte-identical to `KhronosGroup/OpenGL-Registry@main`, sha256 `b9ca2cfa5c676e901c20d34af3407f1687cde0f1336a5ff7a8974d04c7494ad3` (2 774 722 bytes) | authoritative source for every command / enum / profile attribute |
| `data/version_inventory.json` | machine-generated per-version x profile inventory (schema 1) | every number in this document |
| `docs/khronos/glspec46.core.pdf` | *The OpenGL Graphics System: A Specification, Version 4.6 (Core Profile) - May 5, 2022*, 851 PDF pages | citable specification text (Appendix E, Appendix K, §22.2) |
| `docs/khronos/GLSLangSpec.4.60.pdf` | *The OpenGL Shading Language Specification, version 4.60.8* | GLSL profile behaviour |
| `docs/khronos/extensions/**` | vendored extension specs (ARB/KHR/EXT/…) | extension-handling rules |
| `tools/gl46_catalog.py` | the project's canonical registry derivation | cross-check of the 657 / 42 figures |

Regenerate the inventory that every table in this document is derived from:

```
python3 tools/gen_version_inventory.py --out data/version_inventory.json
```

The generator's self-check (run automatically) asserts:

| Check | Value | Expected |
| --- | --- | --- |
| GL 4.6 core commands | 657 | 657 |
| GL 4.6 compatibility-only commands | 42 | 42 |
| Matches `tools/gl46_catalog.parse_core_commands()` | yes | true |
| Matches `tools/gl46_catalog.parse_compat_only_commands()` | yes | true |

Citation convention used below:

* **`gl.xml:LINE`** - a line number in the vendored registry (stable for the
  vendored sha256 above).
* **spec §X.Y / App. X** - a section of the 4.6 Core Profile PDF. Where a PDF
  page is given it is the *physical PDF page*; the printed page number is
  PDF page − 22.
* **"Registry says X" / "Spec says Y"** - the two are kept strictly apart.

---

## 1. Where the profile split starts

### 1.1 Derived from the registry (authoritative)

Scanning every `<feature api="gl" name="GL_VERSION_x_y">` block of `gl.xml`
for `profile=` attributes yields exactly one boundary:

| GL feature | `profile=` attributes present in the registry | Profile model |
| --- | --- | --- |
| `GL_VERSION_1_0` … `GL_VERSION_3_1` | **none** | **legacy (unsplit)** |
| `GL_VERSION_3_2` (`gl.xml:31629`) | `<remove profile="core">` ×9 (`gl.xml:31735, 31995, 32342, 32349, 32425, 32488, 32512, 32518, 32527`) | **split begins** |
| `GL_VERSION_3_3` (`gl.xml:32543`) | `<require profile="compatibility">` ×1 (`gl.xml:32608`) | split |
| `GL_VERSION_4_0` … `GL_VERSION_4_2` | none | split |
| `GL_VERSION_4_3` (`gl.xml:33125`) | `<require profile="compatibility">` (`gl.xml:33519`), `<require profile="core">` (`gl.xml:33522`) | split |
| `GL_VERSION_4_4` | none | split |
| `GL_VERSION_4_5` (`gl.xml:33588`) | `<require profile="compatibility">` ×2 (`gl.xml:33750, 33764`) | split |
| `GL_VERSION_4_6` (`gl.xml:33787`) | none | split |

The registry states the boundary in its own words, in the comment that
precedes the first `<remove profile="core">` block (`gl.xml:31730-31734`):

> ```
> <!-- OpenGL 3.2 is where core and compatibility profiles were first
>      introduced, so many, many things were removed from the core
>      profile in this version. A few were reintroduced later (e.g.
>      GetPointerv / STACK_{UNDER,OVER}FLOW by OpenGL 4.3 for debug
>      functionality). -->
> ```

**Consequence.** OpenGL 1.0 – 3.1 carry **no** profile attribute at all. The
registry therefore models them as a single, unsplit API surface: for those
versions `core == compatibility` *by construction*, and there is no such
thing as "the core profile of OpenGL 1.4". The split boundary is **OpenGL
3.2**.

### 1.2 Confirmed by the specification

| Spec statement | Citation |
| --- | --- |
| "Starting with OpenGL 3.2, API profiles are defined. Profiled context creation extends the versioned context creation interface to specify a profile which must be implemented by the context. The name strings for the GLX and WGL profiled context creation interfaces are `GLX_ARB_create_context_profile` and `WGL_ARB_create_context_profile` respectively." | spec App. K §K.3.3.66 "Profiled Context Creation" (PDF p. 781, printed p. 759) |
| "Starting with OpenGL 3.0, a new context creation interface is required in the window system integration layer. This interface specifies the context version required as well as other attributes of the context." (→ `GLX_ARB_create_context` / `WGL_ARB_create_context`) | spec App. K §K.3.3.48 "Versioned Context Creation" (PDF p. 778, printed p. 756) |
| "Profiles define subsets of OpenGL functionality targeted to specific application domains. Starting with OpenGL 3.2, two profiles are defined (see below)." | spec App. E (PDF p. 699, printed p. 677) |

The registry and the specification agree: **the core/compatibility split
starts at OpenGL 3.2.**

---

## 2. Per-version command counts, core vs compatibility

`core` = entry points in the OpenGL core profile at that version;
`compat` = entry points in the OpenGL compatibility profile at that version;
`compat-only` = required with `profile="compatibility"` and never core;
`removed from core` = entry points the version removed from the core profile;
`removed from compat` = entry points the version removed from the
compatibility profile.

| GL | profile model | core cmds | compat cmds | compat-only | removed from core | removed from compat | newly required (core) | GLSL |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1.0 | legacy | 306 | 306 | 0 | 0 | 0 | 306 | -- (none) |
| 1.1 | legacy | 336 | 336 | 0 | 0 | 0 | 30 | -- (none) |
| 1.2 | legacy | 340 | 340 | 0 | 0 | 0 | 4 | -- (none) |
| 1.3 | legacy | 386 | 386 | 0 | 0 | 0 | 46 | -- (none) |
| 1.4 | legacy | 433 | 433 | 0 | 0 | 0 | 47 | -- (none) |
| 1.5 | legacy | 452 | 452 | 0 | 0 | 0 | 19 | -- (none) |
| 2.0 | legacy | 545 | 545 | 0 | 0 | 0 | 93 | 1.10 |
| 2.1 | legacy | 551 | 551 | 0 | 0 | 0 | 6 | 1.20 |
| 3.0 | legacy | 635 | 635 | 0 | 0 | 0 | 84 | 1.30 |
| 3.1 | legacy | 647 | 647 | 0 | 0 | 0 | 12 | 1.40 |
| 3.2 | **split** | **316** | **666** | 0 | **350** | 0 | 19 | 1.50 |
| 3.3 | split | 344 | 724 | 30 | 0 | 0 | 28 | 3.30 |
| 4.0 | split | 390 | 770 | 30 | 0 | 0 | 46 | 4.00 |
| 4.1 | split | 478 | 858 | 30 | 0 | 0 | 88 | 4.10 |
| 4.2 | split | 490 | 870 | 30 | 0 | 0 | 12 | 4.20 |
| 4.3 | split | 534 | 913 | 30 | 0 | 0 | 44 | 4.30 |
| 4.4 | split | 543 | 922 | 30 | 0 | 0 | 9 | 4.40 |
| 4.5 | split | 653 | 1044 | 42 | 0 | 0 | 110 | 4.50 |
| 4.6 | split | **657** | **1048** | **42** | 0 | 0 | 4 | 4.60 |

Two facts fall straight out of the table and are worth stating explicitly:

* **Exactly one version in the whole range removes anything from either
  profile: OpenGL 3.2.** It removes **350** commands *from the core profile
  only* and removes **0** commands from the compatibility profile. Every
  other version in 1.0 - 4.6 has `removed from core = 0` and
  `removed from compat = 0`.
* The compatibility command set is monotonically non-decreasing over the
  whole range (306 → 1048). The core command set is monotonically
  non-decreasing **except** for the single 3.2 step (647 → 316).

### 2.1 The commands removed from the core profile in OpenGL 3.2 (350)

**Registry reason category for all 350: `removed-by-profile`.**
Every one of them is removed by a `<remove profile="core">` block inside
`<feature api="gl" name="GL_VERSION_3_2">` (`gl.xml:31735 … 32527`), with the
registry comments naming the originating GL version
(`"Compatibility-only GL 1.0 features removed from GL 3.2"` …
`"Compatibility-only GL 3.0 features removed from GL 3.2"`). No entry point
is removed from the compatibility profile by any version.

**Specification reason.** The GL 4.6 Core Profile specification groups the
same removals by feature in Appendix E.2.2 "Removed Features" (PDF pp. 701-705,
printed pp. 679-683). The table below is the 350 commands grouped by the
spec bullet that removes them.

| GL 4.6 Core spec, Appendix E.2.2 "Removed Features" bullet | count | commands |
| --- | ---: | --- |
| Begin / End primitive specification | 165 | `glArrayElement`, `glBegin`, `glColor3b`, `glColor3bv`, `glColor3d`, `glColor3dv`, `glColor3f`, `glColor3fv`, `glColor3i`, `glColor3iv`, `glColor3s`, `glColor3sv`, `glColor3ub`, `glColor3ubv`, `glColor3ui`, `glColor3uiv`, `glColor3us`, `glColor3usv`, `glColor4b`, `glColor4bv`, `glColor4d`, `glColor4dv`, `glColor4f`, `glColor4fv`, `glColor4i`, `glColor4iv`, `glColor4s`, `glColor4sv`, `glColor4ub`, `glColor4ubv`, `glColor4ui`, `glColor4uiv`, `glColor4us`, `glColor4usv`, `glEdgeFlag`, `glEdgeFlagv`, `glEnd`, `glFogCoordd`, `glFogCoorddv`, `glFogCoordf`, `glFogCoordfv`, `glIndexd`, `glIndexdv`, `glIndexf`, `glIndexfv`, `glIndexi`, `glIndexiv`, `glIndexs`, `glIndexsv`, `glIndexub`, `glIndexubv`, `glMultiTexCoord1d`, `glMultiTexCoord1dv`, `glMultiTexCoord1f`, `glMultiTexCoord1fv`, `glMultiTexCoord1i`, `glMultiTexCoord1iv`, `glMultiTexCoord1s`, `glMultiTexCoord1sv`, `glMultiTexCoord2d`, `glMultiTexCoord2dv`, `glMultiTexCoord2f`, `glMultiTexCoord2fv`, `glMultiTexCoord2i`, `glMultiTexCoord2iv`, `glMultiTexCoord2s`, `glMultiTexCoord2sv`, `glMultiTexCoord3d`, `glMultiTexCoord3dv`, `glMultiTexCoord3f`, `glMultiTexCoord3fv`, `glMultiTexCoord3i`, `glMultiTexCoord3iv`, `glMultiTexCoord3s`, `glMultiTexCoord3sv`, `glMultiTexCoord4d`, `glMultiTexCoord4dv`, `glMultiTexCoord4f`, `glMultiTexCoord4fv`, `glMultiTexCoord4i`, `glMultiTexCoord4iv`, `glMultiTexCoord4s`, `glMultiTexCoord4sv`, `glNormal3b`, `glNormal3bv`, `glNormal3d`, `glNormal3dv`, `glNormal3f`, `glNormal3fv`, `glNormal3i`, `glNormal3iv`, `glNormal3s`, `glNormal3sv`, `glRectd`, `glRectdv`, `glRectf`, `glRectfv`, `glRecti`, `glRectiv`, `glRects`, `glRectsv`, `glSecondaryColor3b`, `glSecondaryColor3bv`, `glSecondaryColor3d`, `glSecondaryColor3dv`, `glSecondaryColor3f`, `glSecondaryColor3fv`, `glSecondaryColor3i`, `glSecondaryColor3iv`, `glSecondaryColor3s`, `glSecondaryColor3sv`, `glSecondaryColor3ub`, `glSecondaryColor3ubv`, `glSecondaryColor3ui`, `glSecondaryColor3uiv`, `glSecondaryColor3us`, `glSecondaryColor3usv`, `glTexCoord1d`, `glTexCoord1dv`, `glTexCoord1f`, `glTexCoord1fv`, `glTexCoord1i`, `glTexCoord1iv`, `glTexCoord1s`, `glTexCoord1sv`, `glTexCoord2d`, `glTexCoord2dv`, `glTexCoord2f`, `glTexCoord2fv`, `glTexCoord2i`, `glTexCoord2iv`, `glTexCoord2s`, `glTexCoord2sv`, `glTexCoord3d`, `glTexCoord3dv`, `glTexCoord3f`, `glTexCoord3fv`, `glTexCoord3i`, `glTexCoord3iv`, `glTexCoord3s`, `glTexCoord3sv`, `glTexCoord4d`, `glTexCoord4dv`, `glTexCoord4f`, `glTexCoord4fv`, `glTexCoord4i`, `glTexCoord4iv`, `glTexCoord4s`, `glTexCoord4sv`, `glVertex2d`, `glVertex2dv`, `glVertex2f`, `glVertex2fv`, `glVertex2i`, `glVertex2iv`, `glVertex2s`, `glVertex2sv`, `glVertex3d`, `glVertex3dv`, `glVertex3f`, `glVertex3fv`, `glVertex3i`, `glVertex3iv`, `glVertex3s`, `glVertex3sv`, `glVertex4d`, `glVertex4dv`, `glVertex4f`, `glVertex4fv`, `glVertex4i`, `glVertex4iv`, `glVertex4s`, `glVertex4sv` |
| Edge flags and fixed-function vertex processing (vertex pointers / client state) | 12 | `glClientActiveTexture`, `glColorPointer`, `glDisableClientState`, `glEdgeFlagPointer`, `glEnableClientState`, `glFogCoordPointer`, `glIndexPointer`, `glInterleavedArrays`, `glNormalPointer`, `glSecondaryColorPointer`, `glTexCoordPointer`, `glVertexPointer` |
| Edge flags and fixed-function vertex processing (matrix stack) | 20 | `glFrustum`, `glLoadIdentity`, `glLoadMatrixd`, `glLoadMatrixf`, `glLoadTransposeMatrixd`, `glLoadTransposeMatrixf`, `glMatrixMode`, `glMultMatrixd`, `glMultMatrixf`, `glMultTransposeMatrixd`, `glMultTransposeMatrixf`, `glOrtho`, `glPopMatrix`, `glPushMatrix`, `glRotated`, `glRotatef`, `glScaled`, `glScalef`, `glTranslated`, `glTranslatef` |
| Edge flags and fixed-function vertex processing (lighting / materials / texgen / clip planes) | 29 | `glClipPlane`, `glColorMaterial`, `glGetClipPlane`, `glGetLightfv`, `glGetLightiv`, `glGetMaterialfv`, `glGetMaterialiv`, `glGetTexGendv`, `glGetTexGenfv`, `glGetTexGeniv`, `glLightModelf`, `glLightModelfv`, `glLightModeli`, `glLightModeliv`, `glLightf`, `glLightfv`, `glLighti`, `glLightiv`, `glMaterialf`, `glMaterialfv`, `glMateriali`, `glMaterialiv`, `glShadeModel`, `glTexGend`, `glTexGendv`, `glTexGenf`, `glTexGenfv`, `glTexGeni`, `glTexGeniv` |
| Evaluators | 23 | `glEvalCoord1d`, `glEvalCoord1dv`, `glEvalCoord1f`, `glEvalCoord1fv`, `glEvalCoord2d`, `glEvalCoord2dv`, `glEvalCoord2f`, `glEvalCoord2fv`, `glEvalMesh1`, `glEvalMesh2`, `glEvalPoint1`, `glEvalPoint2`, `glGetMapdv`, `glGetMapfv`, `glGetMapiv`, `glMap1d`, `glMap1f`, `glMap2d`, `glMap2f`, `glMapGrid1d`, `glMapGrid1f`, `glMapGrid2d`, `glMapGrid2f` |
| Current raster position | 40 | `glRasterPos2d`, `glRasterPos2dv`, `glRasterPos2f`, `glRasterPos2fv`, `glRasterPos2i`, `glRasterPos2iv`, `glRasterPos2s`, `glRasterPos2sv`, `glRasterPos3d`, `glRasterPos3dv`, `glRasterPos3f`, `glRasterPos3fv`, `glRasterPos3i`, `glRasterPos3iv`, `glRasterPos3s`, `glRasterPos3sv`, `glRasterPos4d`, `glRasterPos4dv`, `glRasterPos4f`, `glRasterPos4fv`, `glRasterPos4i`, `glRasterPos4iv`, `glRasterPos4s`, `glRasterPos4sv`, `glWindowPos2d`, `glWindowPos2dv`, `glWindowPos2f`, `glWindowPos2fv`, `glWindowPos2i`, `glWindowPos2iv`, `glWindowPos2s`, `glWindowPos2sv`, `glWindowPos3d`, `glWindowPos3dv`, `glWindowPos3f`, `glWindowPos3fv`, `glWindowPos3i`, `glWindowPos3iv`, `glWindowPos3s`, `glWindowPos3sv` |
| Pixel transfer modes and operations | 8 | `glGetPixelMapfv`, `glGetPixelMapuiv`, `glGetPixelMapusv`, `glPixelMapfv`, `glPixelMapuiv`, `glPixelMapusv`, `glPixelTransferf`, `glPixelTransferi` |
| Fixed-function fragment processing (texture environment) | 6 | `glGetTexEnvfv`, `glGetTexEnviv`, `glTexEnvf`, `glTexEnvfv`, `glTexEnvi`, `glTexEnviv` |
| Fixed-function fragment processing (fog) | 4 | `glFogf`, `glFogfv`, `glFogi`, `glFogiv` |
| Fixed-function fragment processing (texture residency and priority) | 2 | `glAreTexturesResident`, `glPrioritizeTextures` |
| Display lists | 8 | `glCallList`, `glCallLists`, `glDeleteLists`, `glEndList`, `glGenLists`, `glIsList`, `glListBase`, `glNewList` |
| Selection and feedback modes | 8 | `glFeedbackBuffer`, `glInitNames`, `glLoadName`, `glPassThrough`, `glPopName`, `glPushName`, `glRenderMode`, `glSelectBuffer` |
| Attribute stacks | 4 | `glPopAttrib`, `glPopClientAttrib`, `glPushAttrib`, `glPushClientAttrib` |
| Accumulation buffers | 2 | `glAccum`, `glClearAccum` |
| Polygon Stipple | 2 | `glGetPolygonStipple`, `glPolygonStipple` |
| Color index mode | 2 | `glClearIndex`, `glIndexMask` |
| User clipping planes | 2 | `glClipPlane`, `glGetClipPlane` |
| Alpha test | 1 | `glAlphaFunc` |
| Bitmaps | 1 | `glBitmap` |
| Pixel drawing | 2 | `glDrawPixels`, `glPixelZoom` |
| Pixel copying | 1 | `glCopyPixels` |
| Wide lines and line stipple | 1 | `glLineStipple` |
| **sub-total (all bullets above)** | **349** | |
| *special case - removed from core in 3.2, **restored to core in 4.3*** | 1 | `glGetPointerv` |
| **TOTAL removed from the core profile in GL 3.2** | **350** | |

Reason-category legend used in this document:

| Category | Meaning | Where it applies here |
| --- | --- | --- |
| `deprecated` | the feature belonged to the GL 3.0 deprecation model: deprecated by GL 3.0, removed from the core profile by GL 3.1, and optionally restored in GL 3.1 through the `GL_ARB_compatibility` extension | the whole 350-set (spec App. E §E.2: "OpenGL 3.0 defined a set of deprecated features. OpenGL 3.1 removed most of the deprecated features and moved them into the optional `GL_ARB_compatibility` extension.") |
| `replaced` | the entry point's function is provided by a programmable-pipeline replacement (vertex attributes, shaders, buffer objects) | e.g. `glBegin`/`glVertex*` → `glVertexAttrib*` + `glDrawArrays*`; `glMatrixMode`/`glPushMatrix` → `glUniformMatrix*`; `glLight*`/`glMaterial*` → shader uniforms |
| `moved to extension` | the functionality left the core profile but was reachable through an extension (the GL 3.1 deprecation model) | the 350 at GL 3.2 were reachable in GL 3.1 through `GL_ARB_compatibility` (spec App. K §K.3.3.50) |
| `removed-by-profile` | the registry removed the entry point from one profile only | all 350 at GL 3.2 (`<remove profile="core">`) |

All 350 sit in the same sequence: deprecated by GL 3.0 → removed from the
core profile by GL 3.1 → removed from the core profile again (as a group) by
GL 3.2, reachable in GL 3.1 through `GL_ARB_compatibility` → permanently
retained only in the compatibility profile from GL 3.2 onwards.

### 2.2 The one removal that was undone: `glGetPointerv`

`glGetPointerv` is in the 350 removed at GL 3.2 but is **restored to the core
profile in GL 4.3**, by the plain (unprofiled) `<require>` block of
`GL_VERSION_4_3` (`gl.xml:33517`). The registry comment on the *sibling*
`<require profile="core">` block (`gl.xml:33522`) states the intent:

> ```
> <require profile="core" comment="Restore functionality removed in GL 3.2 core to GL 4.3. Needed for debug interface.">
>     <enum name="GL_STACK_UNDERFLOW"/>
>     <enum name="GL_STACK_OVERFLOW"/>
> </require>
> ```

The registry comment at `gl.xml:31730` says the same thing: *"A few were
reintroduced later (e.g. GetPointerv / STACK_{UNDER,OVER}FLOW by OpenGL 4.3
for debug functionality)."*

The specification says exactly the same in its own words - spec App. G §G.3
"Deprecation Model" (PDF p. 715, printed p. 693):

> "The following features which were previously deprecated have been
> re-introduced to the OpenGL 4.3 core profile:
> • The `GetPointerv` command (see section 22.2) and the `STACK_OVERFLOW`
> and `STACK_UNDERFLOW` errors (see table 2.3). These features are used by
> the debug functionality in chapter 20."

Consequence for the counts: the *net* core removals that survived to GL 4.6
are **349**, not 350, and the compatibility profile keeps all 350.

### 2.3 Commands removed from the *compatibility* profile

**None, in any version 1.0 - 4.6.** The registry contains no
`<remove profile="compatibility">` block for `api="gl"`, and the inventory
reports `removed_in_compatibility_commands = 0` for all 19 versions. The
specification states the same in stronger terms (spec App. E §E.2, PDF p. 700):

> "Deprecated and removed features are summarized below in two groups:
> features which are marked deprecated by the core profile, but have not yet
> been removed, and features actually removed from the core profile of the
> current version of OpenGL (**no features have been removed from or
> deprecated in the compatibility profile**)."

---

## 3. The two different "compatibility only" sets

These two numbers are routinely conflated. They are **not** the same question
and they are **not** the same set.

| | **Registry compatibility-only (42)** | **Compatibility-but-not-core (391)** |
| --- | --- | --- |
| Definition | the entry point is required by a `<require profile="compatibility">` block and is **never** a core entry point | the entry point is present in the compatibility profile and **absent** from the core profile |
| Registry mechanism | `profile="compatibility"` attribute present | `<remove profile="core">` at GL 3.2 **plus** the `profile="compatibility"` requires |
| Size at GL 4.6 | **42** | **391** |
| Composition | 30 `gl*P*ui`/`gl*P*uiv` vertex-type entry points (GL 3.3) + 12 `glGetn*` pixel/history queries (GL 4.5) | 349 fixed-function entry points removed from core at GL 3.2 and never restored **+** the same 42 |
| Meaning | "the registry added this *new* entry point for the compatibility profile only" | "the compatibility profile has this *old* entry point that core dropped" |
| Cross-check | `tools/gl46_catalog.COMPAT_ONLY_FUNCTION_COUNT = 42`, asserted by the generator's self-check | `data/version_inventory.json` → `versions["4.6"]["counts"]["compat_not_core_commands"] = 391` |

### 3.1 The 42 registry compatibility-only commands

Registry source: `gl.xml:32608` (GL 3.3, comment *"Reuse
ARB_vertex_type_2_10_10_10_rev compatibility profile"*) and `gl.xml:33750`
(GL 4.5, comment *"Reuse GL_ARB_robustness"*).

| Command | introduced in |
| --- | --- |
| `glColorP3ui` | 3.3 |
| `glColorP3uiv` | 3.3 |
| `glColorP4ui` | 3.3 |
| `glColorP4uiv` | 3.3 |
| `glMultiTexCoordP1ui` | 3.3 |
| `glMultiTexCoordP1uiv` | 3.3 |
| `glMultiTexCoordP2ui` | 3.3 |
| `glMultiTexCoordP2uiv` | 3.3 |
| `glMultiTexCoordP3ui` | 3.3 |
| `glMultiTexCoordP3uiv` | 3.3 |
| `glMultiTexCoordP4ui` | 3.3 |
| `glMultiTexCoordP4uiv` | 3.3 |
| `glNormalP3ui` | 3.3 |
| `glNormalP3uiv` | 3.3 |
| `glSecondaryColorP3ui` | 3.3 |
| `glSecondaryColorP3uiv` | 3.3 |
| `glTexCoordP1ui` | 3.3 |
| `glTexCoordP1uiv` | 3.3 |
| `glTexCoordP2ui` | 3.3 |
| `glTexCoordP2uiv` | 3.3 |
| `glTexCoordP3ui` | 3.3 |
| `glTexCoordP3uiv` | 3.3 |
| `glTexCoordP4ui` | 3.3 |
| `glTexCoordP4uiv` | 3.3 |
| `glVertexP2ui` | 3.3 |
| `glVertexP2uiv` | 3.3 |
| `glVertexP3ui` | 3.3 |
| `glVertexP3uiv` | 3.3 |
| `glVertexP4ui` | 3.3 |
| `glVertexP4uiv` | 3.3 |
| `glGetnColorTable` | 4.5 |
| `glGetnConvolutionFilter` | 4.5 |
| `glGetnHistogram` | 4.5 |
| `glGetnMapdv` | 4.5 |
| `glGetnMapfv` | 4.5 |
| `glGetnMapiv` | 4.5 |
| `glGetnMinmax` | 4.5 |
| `glGetnPixelMapfv` | 4.5 |
| `glGetnPixelMapuiv` | 4.5 |
| `glGetnPixelMapusv` | 4.5 |
| `glGetnPolygonStipple` | 4.5 |
| `glGetnSeparableFilter` | 4.5 |

Why these two families are compatibility-only:

* The **30 `gl*P*ui` / `gl*P*uiv`** entry points are the *unsigned integer*
  typed variants of the immediate-mode vertex/colour/texcoord entry points
  (the `GL_ARB_vertex_type_2_10_10_10_rev` family). They only make sense as
  part of the compatibility profile's immediate-mode vertex specification
  (`glBegin`/`glEnd` + `glVertex*`/`glColor*`/`glTexCoord*`); the core
  profile has no immediate mode at all, so there is nothing for them to
  serve. The registry comment (`gl.xml:32608`) says exactly this.
* The **12 `glGetn*`** entry points are the bounded-size ("n") variants of
  the pixel-map, histogram, convolution, colour-table, min/max and polygon
  stipple queries. Every one of them queries *fixed-function* state that the
  core profile removed (pixel transfer modes and operations, histogram,
  convolution, colour tables, polygon stipple - spec App. E §E.2.2). The
  registry reuses the `GL_ARB_robustness` text for them (`gl.xml:33750`).
  Note that the *other* `glGetn*` family (`glGetnUniform*`, `glGetnTexImage`,
  `glGetnCompressedTexImage`, `glReadnPixels`) is **core** - it is required
  without a profile attribute in `GL_VERSION_4_5`.

### 3.2 The 391 "compatibility but not core" commands

This is the larger set: **everything the compatibility profile has that the
core profile does not**. It is the fixed-function legacy surface that
OpenGL 3.2 core dropped and the compatibility profile kept.

Derivation (all three terms are script-checked):

```
391 = 349  (the 350 removed from core at GL 3.2, minus glGetPointerv which
             GL 4.3 restored to core)
    +  30  (GL 3.3 <require profile="compatibility">, gl.xml:32608)
    +  12  (GL 4.5 <require profile="compatibility">, gl.xml:33750)
```

The same identity holds at every version from 3.3 on; the intermediate
numbers are `compat_not_core_commands` = 350 (3.2), 380 (3.3 - 4.2),
379 (4.3 - 4.4, `glGetPointerv` restored), 391 (4.5 - 4.6).

### 3.3 Categorized breakdown of the 391-set

The grouping below is **generated from `data/version_inventory.json`** by the
script in §3.4. Each of the 391 commands lands in exactly one functional
group; the script asserts that the legacy groups together reproduce the
`compatibility - core` set exactly, so the table cannot drift from the
registry.

| # | Functional group | commands | share of 391 |
| --- | --- | ---: | ---: |
| L01 | Immediate mode (Begin/End and vertex specification) | 173 | 44.2 % |
| L02 | Client-side vertex arrays | 12 | 3.1 % |
| L03 | Fixed-function transform and matrix stack | 20 | 5.1 % |
| L04 | Lighting, materials and shading model | 18 | 4.6 % |
| L05 | Fog | 4 | 1.0 % |
| L06 | Texture environment and texture coordinate generation | 15 | 3.8 % |
| L07 | Display lists | 8 | 2.0 % |
| L08 | Evaluators | 23 | 5.9 % |
| L09 | Selection and feedback modes | 8 | 2.0 % |
| L10 | Accumulation buffer | 2 | 0.5 % |
| L11 | Pixel transfer modes and pixel drawing | 12 | 3.1 % |
| L12 | Raster position and window position | 40 | 10.2 % |
| L13 | Stipple (polygon and line stipple) | 3 | 0.8 % |
| L14 | Alpha test | 1 | 0.3 % |
| L15 | Texture residency and priority (fixed-function fragment processing) | 2 | 0.5 % |
| L16 | User clipping planes | 2 | 0.5 % |
| L17 | Attribute stacks | 4 | 1.0 % |
| L18 | Colour-index mode state | 2 | 0.5 % |
| L19 | Compatibility-only vertex-type entry points (GL 3.3) | 30 | 7.7 % |
| L20 | Compatibility-only n-suffixed pixel/history queries (GL 4.5) | 12 | 3.1 % |
| | **TOTAL** | **391** | **100 %** |

Reading of the table:

* **L01 - L18** (349 commands) are the *legacy fixed-function* surface that
  OpenGL 3.2 removed from the core profile and that the compatibility
  profile still carries. Each group maps onto a bullet of the GL 4.6 Core
  Profile specification's Appendix E.2.2 "Removed Features" list (the
  per-command mapping is in §2.1).
* **L19 and L20** (42 commands) are the *registry compatibility-only* set of
  §3.1 - entry points that were **added** to the compatibility profile
  later (GL 3.3 and GL 4.5) and were never core.

### 3.4 The script that generates §3.3

Save as `tools/profile_groups.py` (or run from the repository root with the
script in any location). It reads **only** the generated inventory, so it
cannot drift from the registry.

```python
#!/usr/bin/env python3
"""Functional-group taxonomy for the OpenGL desktop command surface.

Usage:  python3 profile_groups.py [--dylib PATH] [--inventory PATH]

Reads only data/version_inventory.json (generated from the Khronos registry)
and, optionally, a built libtglmt dylib for the *measured* implementation
status.  Prints Markdown tables.

The rule table is ordered; the first matching rule wins.  The script asserts
that every command of the OpenGL 4.6 compatibility profile is assigned to
exactly one group, and that the legacy groups together reproduce exactly the
"compatibility but not core" set.
"""
import argparse, json, os, re, subprocess, sys

ROOT = os.environ.get('TGLMT_ROOT') or os.getcwd()

LEGACY = [
 ("L01", "Immediate mode (Begin/End and vertex specification)",
  r'^gl(Begin|End)$|^glEdgeFlag(v)?$|^glVertex[234][dfis]v?$|^glColor[34](u?[bdfis])v?$|^glNormal3[bdfis]v?$'
  r'|^glSecondaryColor3(u?[bdfis])v?$|^glTexCoord[1234](u?[dfis])v?$|^glMultiTexCoord[1234](u?[dfis])v?$'
  r'|^glIndex([1-4]|u?[bdfis])v?$|^glFogCoord[12]?[dfi]v?$|^glArrayElement$|^glRect[dfis]v?$'),
 ("L02", "Client-side vertex arrays",
  r'^gl(VertexPointer|ColorPointer|NormalPointer|TexCoordPointer|SecondaryColorPointer|IndexPointer'
  r'|EdgeFlagPointer|FogCoordPointer|EnableClientState|DisableClientState|InterleavedArrays|ClientActiveTexture)$'),
 ("L03", "Fixed-function transform and matrix stack",
  r'^gl(MatrixMode|PushMatrix|PopMatrix|LoadIdentity|LoadMatrix[d f]|LoadTransposeMatrix[d f]|MultMatrix[d f]'
  r'|MultTransposeMatrix[d f]|Translat(ed|e[fd]?)|Rotat(ed|ef)|Scal(e[fd]?)|Frustum|Ortho)$'),
 ("L04", "Lighting, materials and shading model",
  r'^gl(Light|LightModel|Material|ColorMaterial|ShadeModel|GetLight|GetMaterial)'),
 ("L05", "Fog", r'^glFog[fi]v?$'),
 ("L06", "Texture environment and texture coordinate generation",
  r'^gl(TexEnv|TexGen|GetTexEnv|GetTexGen)'),
 ("L07", "Display lists",
  r'^gl(NewList|EndList|CallList|CallLists|ListBase|GenLists|IsList|DeleteLists)$'),
 ("L08", "Evaluators", r'^gl(Map[12]|MapGrid|EvalCoord|EvalMesh|EvalPoint|GetMap)'),
 ("L09", "Selection and feedback modes",
  r'^gl(RenderMode|InitNames|PopName|PushName|LoadName|SelectBuffer|FeedbackBuffer|PassThrough)$'),
 ("L10", "Accumulation buffer", r'^gl(Accum|ClearAccum)$'),
 ("L11", "Pixel transfer modes and pixel drawing",
  r'^gl(PixelMap|GetPixelMap|PixelTransfer|GetPixelTransfer|DrawPixels|PixelZoom|Bitmap|CopyPixels)'),
 ("L12", "Raster position and window position", r'^gl(RasterPos|WindowPos)'),
 ("L13", "Stipple (polygon and line stipple)", r'^gl(PolygonStipple|GetPolygonStipple|LineStipple)$'),
 ("L14", "Alpha test", r'^glAlphaFunc$'),
 ("L15", "Texture residency and priority (fixed-function fragment processing)",
  r'^gl(AreTexturesResident|PrioritizeTextures)$'),
 ("L16", "User clipping planes", r'^gl(Get)?ClipPlane$'),
 ("L17", "Attribute stacks", r'^gl(PushAttrib|PopAttrib|PushClientAttrib|PopClientAttrib)$'),
 ("L18", "Colour-index mode state", r'^gl(ClearIndex|IndexMask)$'),
 ("L19", "Compatibility-only vertex-type entry points (GL 3.3)",
  r'^gl(Vertex|Color|Normal|SecondaryColor|TexCoord|MultiTexCoord)P[1-4]'),
 ("L20", "Compatibility-only n-suffixed pixel/history queries (GL 4.5)",
  r'^glGetn(Histogram|ColorTable|Minmax|Map[dif]v?|SeparableFilter|ConvolutionFilter|PixelMap[a-z]*|PolygonStipple)$'),
]

MODERN = [
 ("M01", "Buffer objects (spec ch. 6)",
  r'^gl(BindBuffer|Buffer|CopyBuffer|MapBuffer|UnmapBuffer|InvalidateBuffer|GetBuffer|GetNamedBuffer'
  r'|ClearNamedBuffer|InvalidateNamedBuffer|MapNamedBuffer|UnmapNamedBuffer|CopyNamedBuffer|CreateBuffers'
  r'|IsBuffer|GetVertexBuffer|BindVertexBuffer|GenBuffers|DeleteBuffers|NamedBuffer)'),
 ("M02", "Shaders, programs and program pipelines (spec ch. 7)",
  r'^gl(CreateShader|CompileShader|GetShader|DeleteShader|AttachShader|DetachShader|LinkProgram|GetProgram'
  r'|UseProgram|DeleteProgram|CreateProgram|ProgramParameter|ProgramBinary|ShaderBinary|ReleaseShaderCompiler'
  r'|GetShaderPrecisionFormat|GetProgramPipeline|GenProgramPipeline|CreateProgramPipeline|DeleteProgramPipeline'
  r'|IsProgramPipeline|BindProgramPipeline|UseProgramStages|ActiveShaderProgram|TransformFeedbackVaryings'
  r'|DispatchCompute|GetAttachedShaders|IsProgram|IsShader|ShaderSource|SpecializeShader|ValidateProgram'
  r'|ValidateProgramPipeline|GetActiveAtomicCounterBufferiv|ShaderStorageBlockBinding)'),
 ("M03", "Program interfaces, uniforms and uniform blocks (spec ch. 7.3, 7.6)",
  r'^gl(Get)?Uniform|^glBindAttribLocation$|^glBindFragDataLocation(Indexed)?$|^glGetActiveAttrib$'
  r'|^glGetAttribLocation$|^glGetFragData(Index|Location)$|^glGetActiveUniform|^glGetActiveSubroutine'
  r'|^glGetSubroutine|^glProgramUniform|^glGetnUniform'),
 ("M04", "Textures and texture objects (spec ch. 8)",
  r'^gl(BindTexture|ActiveTexture|TexImage|TexSubImage|CompressedTex|CopyTex|GetTex|GetCompressedTex|GetNamedTex'
  r'|TexParameter|GenerateMipmap|TexStorage|ClearTex|InvalidateTex|Texture[0-9]|GetTexture|CreateTextures'
  r'|IsTexture|TextureView|Texture(Parameter|Storage|SubImage)|TexBuffer|TexBufferRange|TextureBuffer'
  r'|TextureBufferRange|TextureBarrier|GenTextures|DeleteTextures|GenerateTextureMipmap'
  r'|Getn(CompressedTexImage|TexImage))'),
 ("M05", "Sampler objects (spec ch. 8.2)",
  r'^gl(GenSamplers|CreateSamplers|BindSampler|BindSamplers|DeleteSamplers|IsSampler|SamplerParameter'
  r'|GetSamplerParameter|GetNamedSamplerParameter)'),
 ("M06", "Pixel storage modes and pixel rectangles (spec ch. 8.4)",
  r'^gl(PixelStore|ReadBuffer)'),
 ("M07", "Framebuffer objects and renderbuffers (spec ch. 9)",
  r'^gl(GenFramebuffers|CreateFramebuffers|BindFramebuffer|DeleteFramebuffers|IsFramebuffer|CheckFramebufferStatus'
  r'|CheckNamedFramebufferStatus|Framebuffer|GetFramebuffer|GetNamedFramebuffer|BlitFramebuffer|BlitNamedFramebuffer'
  r'|ClearNamedFramebuffer|InvalidateNamedFramebuffer|InvalidateFramebuffer|InvalidateSubFramebuffer'
  r'|GenRenderbuffers|CreateRenderbuffers|BindRenderbuffer|DeleteRenderbuffers|IsRenderbuffer|Renderbuffer'
  r'|GetRenderbuffer|GetNamedRenderbuffer|NamedFramebuffer|NamedRenderbuffer)'),
 ("M08", "Vertex arrays, vertex attributes and vertex array objects (spec ch. 10.3)",
  r'^gl(VertexAttrib|GetVertexAttrib|VertexArray|GenVertexArrays|CreateVertexArrays|DeleteVertexArrays|IsVertexArray'
  r'|GetVertexArray|BindVertexArray|EnableVertexAttribArray|DisableVertexAttribArray|EnableVertexArrayAttrib'
  r'|DisableVertexArrayAttrib|VertexBindingDivisor)'),
 ("M09", "Drawing commands and conditional rendering (spec ch. 10.4, 10.9)",
  r'^gl(Draw|MultiDraw|PrimitiveRestart|ClipControl|ProvokingVertex|PatchParameter|BeginConditionalRender'
  r'|EndConditionalRender)'),
 ("M10", "Rasterization and multisample state (spec ch. 14)",
  r'^gl(PointSize|LineWidth|PolygonMode|PolygonOffset|SampleCoverage|SampleMaski|MinSampleShading|DepthRange'
  r'|DepthFunc|DepthMask|CullFace|FrontFace|Viewport|Scissor|LogicOp|Hint|PointParameter|GetMultisamplefv)'),
 ("M11", "Transform feedback (spec ch. 13.3)",
  r'^gl(TransformFeedback|BindTransformFeedback|GenTransformFeedbacks|CreateTransformFeedbacks'
  r'|DeleteTransformFeedbacks|IsTransformFeedback|GetTransformFeedback|PauseTransformFeedback'
  r'|ResumeTransformFeedback|BeginTransformFeedback|EndTransformFeedback)'),
 ("M12", "Per-fragment operations, blending, stencil and enable state (spec ch. 17)",
  r'^gl(Blend|ColorMask|ClampColor|Stencil|ClearBuffer|ClearDepth|ClearStencil|Clear|Enable|Disable|IsEnabled)'),
 ("M13", "Pixel read and copy (spec ch. 18)",
  r'^gl(ReadPixels|ReadnPixels|CopyImageSubData)'),
 ("M14", "Compute shaders, images and memory barriers (spec ch. 19)",
  r'^gl(BindImageTexture|BindImageTextures|MemoryBarrier|GetAtomicCounterBuffer)'),
 ("M15", "Asynchronous queries (spec ch. 4.2)",
  r'^gl(BeginQuery|EndQuery|GetQuery|QueryCounter|CreateQueries|GenQueries|DeleteQueries|IsQuery)'),
 ("M16", "Sync objects and fences (spec ch. 4.1)",
  r'^gl(FenceSync|ClientWaitSync|WaitSync|DeleteSync|IsSync|GetSynciv)'),
 ("M17", "Debug API and object labels (spec ch. 20)",
  r'^gl(DebugMessage|GetDebugMessageLog|PushDebugGroup|PopDebugGroup|ObjectLabel|ObjectPtrLabel|GetObjectLabel'
  r'|GetObjectPtrLabel|GetGraphicsResetStatus)'),
 ("M18", "Context queries, strings and general state (spec ch. 21, 22)",
  r'^gl(GetString|GetStringi|GetBooleanv|GetBooleani_v|GetIntegerv|GetIntegeri_v|GetInteger64v|GetInteger64i_v'
  r'|GetFloatv|GetFloati_v|GetDoublev|GetDoublei_v|GetPointerv|GetError|GetInternalformat)'),
 ("M19", "Miscellaneous general GL (spec ch. 2, 17.4)",
  r'^gl(Flush|Finish|DrawBuffer|DrawBuffers)'),
]

RULES = LEGACY + MODERN

def classify(name):
    for gid, label, rx in RULES:
        if re.search(rx, name):
            return gid, label
    return None, None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--inventory', default=os.path.join(ROOT, 'data/version_inventory.json'))
    ap.add_argument('--dylib', default=None)
    args = ap.parse_args()

    inv = json.load(open(args.inventory))
    V = inv['versions']
    compat46 = list(V['4.6']['commands']['compatibility'])
    core46 = set(V['4.6']['commands']['core'])
    cnc = set(compat46) - core46

    groups = {}
    for n in compat46:
        gid, label = classify(n)
        assert gid is not None, 'unclassified command: %s' % n
        groups.setdefault(gid, []).append(n)

    legacy_ids = {g[0] for g in LEGACY}
    legacy_assigned = set()
    for gid, items in groups.items():
        if gid in legacy_ids:
            legacy_assigned |= set(items)
    assert legacy_assigned == cnc, 'legacy groups do not reproduce compat-not-core'

    implemented = set()
    if args.dylib:
        out = subprocess.run(['nm', '-gU', args.dylib], capture_output=True, text=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) >= 3 and p[1] == 'T':
                implemented.add(p[2].lstrip('_'))

    print('| Group | Label | GL 4.6 compat commands | implemented | missing |')
    print('| --- | --- | ---: | ---: | ---: |')
    tot = impl = miss = 0
    for gid, label, _ in RULES:
        items = groups.get(gid, [])
        if not items:
            continue
        if implemented:
            i = len(set(items) & implemented)
            m = len(items) - i
        else:
            i = m = 0
        tot += len(items); impl += i; miss += m
        print('| %s | %s | %d | %d | %d |' % (gid, label, len(items), i, m))
    print('| **TOTAL** | | **%d** | **%d** | **%d** |' % (tot, impl, miss))

if __name__ == '__main__':
    main()
```

Run it:

```
python3 tools/profile_groups.py --dylib build-work/libtglmt.0.1.0.dylib
```

The `M01` - `M19` groups are the *modern* surface; they are used by
[`docs/OPENGL_API_COVERAGE.md`](OPENGL_API_COVERAGE.md) to report the
measured TGLMT implementation status of the whole 1048-command GL 4.6
compatibility surface.

---

## 4. Extension handling per profile

### 4.1 OpenGL 3.1 and `GL_ARB_compatibility` (the pre-3.2 model)

Before profiles existed, OpenGL 3.0 marked features deprecated and OpenGL
3.1 **removed** them from core while keeping them reachable through an
optional extension. Spec App. K §K.3.3.50 "Restoration of features removed
from OpenGL 3.0" (PDF p. 778, printed p. 756):

> "OpenGL 3.1 removes a large number of features that were marked deprecated
> in OpenGL 3.0. GL implementations needing to maintain these features to
> support existing applications may do so, following the deprecation model,
> by exporting an extension string indicating those features are present.
> Applications written for OpenGL 3.1 should not depend on any of the features
> corresponding to this extension, since they will not be available on all
> platforms with 3.1 implementations.
> The name string for restoration of features deprecated by OpenGL 3.0 is
> `GL_ARB_compatibility`.
> **The profile terminology introduced with OpenGL 3.2 eliminates the
> necessity for evolving this extension.** Instead, interactions between
> features removed by OpenGL 3.1 and new features introduced in later
> OpenGL versions are defined by the compatibility profile corresponding to
> those versions."

Rules that follow:

| Profile / context | Rule |
| --- | --- |
| GL 3.0 / 3.1 context | deprecated features are still *in* core but deprecated; `GL_ARB_compatibility` may additionally advertise the removed ones |
| GL 3.2 - 4.6 **core** context | `GL_ARB_compatibility` is not a mechanism any more; the features are simply absent. An extension that reintroduces them must not be advertised in a core context |
| GL 3.2 - 4.6 **compatibility** context | the features are present as core API of the compatibility profile, not as an extension |
| Forward-compatible context (any profile) | "Forward compatible contexts cannot restore deprecated functionality through extensions, but they may support additional, non-deprecated functionality through extensions." (spec App. E, PDF p. 699, printed p. 677) |

### 4.2 Extension enumeration differs between the profiles

Spec App. E §E.2.2 (PDF pp. 703-704, printed pp. 681-682) lists, among the
features removed from the core profile:

> "• Unified extension string - `EXTENSIONS` target to `GetString`."

So in the **core** profile the extension list must be obtained with
`GetStringi(GL_EXTENSIONS, index)` plus `GetIntegerv(GL_NUM_EXTENSIONS)`
(spec §22.2, PDF p. 589, printed p. 567); `GetString(GL_EXTENSIONS)` is not
part of the core profile. The **compatibility** profile keeps the unified
string.

### 4.3 Extensions with explicit profile-conditional text (vendored specs)

| Extension spec (vendored) | Profile-conditional content |
| --- | --- |
| `docs/khronos/extensions/KHR/KHR_debug.txt` | "The extension is written against the OpenGL **4.2 Compatibility Profile** specification (April 27, 2012)." The `DISPLAY_LIST` object namespace (`0x82E7`) is marked `[[ Compatibility Profile ]]`; the core profile has no display-list namespace at all. |
| `docs/khronos/extensions/ARB/ARB_debug_output.txt` | "Dependencies on OpenGL 4.0 **Compatibility** specification and any other versions or extensions that already provide the `GetPointerv` entry point ..." and "Dependencies on OpenGL 4.0 **Compatibility** specification and any other versions that support display lists - `DebugMessageControlARB`, `DebugMessageInsertARB`, `DebugMessageCallbackARB`, and `GetDebugMessageLogARB` are **not compiled into display lists**." |
| `docs/khronos/extensions/ARB/ARB_robustness.txt` | "Additions to Chapter 2 of the OpenGL **3.2 (Compatibility Profile)** Specification (OpenGL Operation)"; adds `CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB 0x00000004` to `CONTEXT_FLAGS`. |
| `docs/khronos/extensions/KHR/KHR_no_error.txt` | `CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008`; "only supported if the OpenGL version has support for context flags (as defined in the OpenGL **4.5 core spec**) or an extension supporting equivalent functionality is exposed." |

---

## 5. Deprecated-but-retained APIs

### 5.1 Core profile, OpenGL 4.6

Spec App. E §E.2.1 "Deprecated But Still Supported Features" (PDF pp. 700-701,
printed pp. 678-679) - "The following features are deprecated, but still
present in the core profile. They may be removed from a future version of
OpenGL, and are removed in a forward-compatible context implementing the core
profile.":

| # | Deprecated but still supported in core | Kind |
| --- | --- | --- |
| 1 | Wide lines - `LineWidth` values greater than 1.0 generate an `INVALID_VALUE` error | behavioural restriction |
| 2 | Global component limit query - the implementation-dependent values `MAX_VARYING_COMPONENTS` and `MAX_VARYING_FLOATS` | query tokens |
| 3 | The query targets `NUM_COMPRESSED_TEXTURE_FORMATS` and `COMPRESSED_TEXTURE_FORMATS` (§8.5) | query tokens |
| 4 | Bitmap pack/unpack state for bitmaps - the pixel pack parameters `UNPACK_LSB_FIRST` and `PACK_LSB_FIRST` | pixel-pack tokens |

Additionally, the *removed features* list of §E.2.2 explicitly keeps a few
things alive in core with a narrower scope:

| Kept in core | Restriction | Citation |
| --- | --- | --- |
| `FrontFace` | not deprecated | §E.2.2 "Note that the `FrontFace` and `ClampColor` commands are not deprecated, as they still affect other non-deprecated functionality" |
| `ClampColor` | the command is not deprecated, but the targets `CLAMP_VERTEX_COLOR` and `CLAMP_FRAGMENT_COLOR` are | §E.2.2 |
| `LineWidth` | the command is not deprecated, only values > 1.0 error | §E.2.2 "Wide lines and line stipple" |
| `TEXTURE_INTERNAL_FORMAT` | replaces the deprecated `TEXTURE_COMPONENTS` | §E.2.2 "Legacy OpenGL 1.0 pixel formats" |
| `DEPTH_TEXTURE_MODE` | deprecated, `r` is returned to texture samplers directly | §E.2.2 "Depth texture mode" |

### 5.2 The per-version deprecation model of the core profile

The 4.6 Core Profile specification keeps a "Deprecation Model" section in the
version appendix of every release it documents. Read together, they give the
per-version deprecation history that the registry can only express in
comments (see §8 D2):

| Core profile version | Newly deprecated by that version | Re-introduced / un-deprecated | Citation (PDF page / printed page) |
| --- | --- | --- | --- |
| GL 3.2 core | Wide lines (`LineWidth > 1.0` → `INVALID_VALUE`); global component limit query (`MAX_VARYING_COMPONENTS`, `MAX_VARYING_FLOATS`) | - | §E.2.1 (PDF p. 700-701 / printed pp. 678-679); registry comment `gl.xml:32535` "Deprecated (not removed) in OpenGL 3.2 core" |
| GL 4.1 core | not restated: the 4.6 Core Profile document has no "Version 4.1" appendix, and the registry carries no (even commented-out) `<deprecate>` block for 4.1 | - | **NOT_VERIFIED** from the vendored sources |
| GL 4.2 core | the query targets `NUM_COMPRESSED_TEXTURE_FORMATS` and `COMPRESSED_TEXTURE_FORMATS` | - | App. F §F.2 (PDF p. 707 / printed p. 685); registry comment `gl.xml:33117` "Deprecated in OpenGL 4.2 core" |
| GL 4.3 core | bitmap pack/unpack state `UNPACK_LSB_FIRST`, `PACK_LSB_FIRST` | **re-introduced**: the `GetPointerv` command and the `STACK_OVERFLOW` / `STACK_UNDERFLOW` errors | App. G §G.3 (PDF p. 715 / printed p. 693); registry comment `gl.xml:33526` "Deprecated in OpenGL 4.3 core" |
| GL 4.4 core | none | none | App. H §H.2 (PDF p. 727 / printed p. 705) |
| GL 4.5 core | none | none | App. I §I.2 (PDF p. 741 / printed p. 719) |
| GL 4.6 core | none | none | App. J §J.2 (PDF p. 756 / printed p. 734) |

Quotations:

> **F.2 (GL 4.2)** - "The following features are newly deprecated by the OpenGL
> 4.2 core profile: • The query targets `NUM_COMPRESSED_TEXTURE_FORMATS` and
> `COMPRESSED_TEXTURE_FORMATS` (see section 8.5). Features deprecated by
> OpenGL 4.1 remain deprecated, but have not yet been removed."

> **H.2 (GL 4.4)** - "No new features are deprecated, and no previously
> deprecated features are re-introduced by the OpenGL 4.4 core profile.
> Features deprecated by OpenGL 4.3 remain deprecated, but have not yet been
> removed."

> **J.2 (GL 4.6)** - "No new features are deprecated, and no previously
> deprecated features are re-introduced by the OpenGL 4.6 core profile.
> Features deprecated by OpenGL 4.5 remain deprecated, but have not yet been
> removed."

### 5.3 Compatibility profile, OpenGL 4.6

**Nothing is deprecated and nothing is removed.** Spec App. E §E.2:

> "Deprecated and removed features are summarized below in two groups:
> features which are marked deprecated by the core profile, but have not yet
> been removed, and features actually removed from the core profile of the
> current version of OpenGL (**no features have been removed from or
> deprecated in the compatibility profile**)."

and §E.1:

> "The compatibility profile does not remove any functionality."

So a compatibility context supports the E.2.1 items *without* the
restriction (a `LineWidth` of 2.0 is legal, `MAX_VARYING_COMPONENTS` is
deprecated-but-present, `GL_UNPACK_LSB_FIRST` is honoured), and supports the
whole E.2.2 removal list (fixed function, display lists, evaluators, ...).

### 5.4 Forward-compatible contexts

Spec App. E §E.2 (PDF p. 700, printed p. 678):

> "Functions which have been removed will generate an `INVALID_OPERATION`
> error if called in the core profile **or in a forward-compatible context**.
> Functions which are partially removed ... will generate the errors
> appropriate for any other unrecognized value of that parameter when a
> removed parameter value is passed in the core profile **or a
> forward-compatible context**. Functions which are deprecated but have not
> yet been removed from the core profile continue to operate normally except
> in a forward-compatible context, where they are also removed."

---

## 6. Rules that affect **context creation**

All rules below are derived from the vendored 4.6 Core Profile specification
PDF and the vendored extension specs. The window-system-specific attribute
names (`WGL_CONTEXT_PROFILE_MASK_ARB`, `GLX_CONTEXT_PROFILE_MASK_ARB`,
`WGL_CONTEXT_FLAGS_ARB`, `GLX_CONTEXT_FLAGS_ARB`, …) belong to
`WGL_ARB_create_context_profile` / `GLX_ARB_create_context_profile`, whose
specification text is **not vendored in this repository** - only the
registry's reference to it (spec App. K §K.3.3.66) is available here. The
rules below are therefore stated at the level the vendored sources support.

### 6.1 Which version + profile combinations are legal

| Requested context | Legal? | Basis |
| --- | --- | --- |
| GL 1.0 - 3.1, no profile requested | **legal** | the registry carries no profile attribute before GL 3.2 (§1.1); this is the single unsplit legacy surface |
| GL 1.0 - 3.1 **with** a profile attribute | **not a meaningful request** | profiles only exist from GL 3.2 (spec App. K §K.3.3.66, PDF p. 781) |
| **Core profile**, GL >= 3.2 | **legal**, and *mandatory to support* | "OpenGL implementations are not required to support all defined profiles, but **must support the core profile** described below." (spec App. E, PDF p. 699, printed p. 677) |
| **Compatibility profile**, GL >= 3.2 | **legal but optional** for the implementation | same quote: only the core profile is mandatory; "OpenGL implementations are not required to support all defined profiles" |
| **Compatibility profile**, GL < 3.2 | **not a meaningful request** | profiles start at 3.2; a pre-3.2 context *is* the unsplit surface that already contains all of the compatibility-only functionality |
| **Core profile + forward-compatible** | **legal** | `CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT` is a context flag of the core profile (spec §22.2) |
| **Compatibility profile + forward-compatible** | **contradictory; the vendored sources cannot describe it as a legal combination** | the compatibility profile "does not remove any functionality" (§E.1) while a forward-compatible context removes *all* deprecated and removed features (§E.2); the compatibility profile contains 349 entry points that are removed features in core, so a forward-compatible compatibility context would have to remove them - i.e. it would not be a compatibility context. The exact window-system attribute semantics live in `WGL_ARB_create_context_profile` / `GLX_ARB_create_context_profile`, whose text is **not vendored here**, so this row is a *derivation from the two vendored specification statements*, not a quotation of the profile-creation extension |
| **Core + compatibility in one context** | **illegal** | "It is not possible to implement both core and compatibility profiles in a single GL context, since the core profile mandates functional restrictions not present in the compatibility profile." (spec App. E §E.1, PDF p. 700, printed p. 678) |
| Requesting a version the implementation does not support | **context creation fails** | "These APIs allow specifying a particular version, profile, and full or forward compatible status, and will either create a context compatible with the request, or fail (if, for example, requesting an OpenGL version or profile not supported by the implementation)." (spec App. E, PDF p. 699) |

### 6.2 Context flags, per profile

All four flags are properties of the *context*, reported through
`GetIntegerv(GL_CONTEXT_FLAGS)` (spec §22.2, PDF p. 589, printed p. 568).

| Flag | Value | Core profile | Compatibility profile | Citation |
| --- | --- | --- | --- | --- |
| `CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT` | `0x00000001` | available: "If `CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT` is set in `CONTEXT_FLAGS`, then the context is a forward-compatible context as defined in appendix E, and the deprecated features described in that appendix are not supported" | see the contradiction in §6.1 | spec §22.2 |
| `CONTEXT_FLAG_DEBUG_BIT` | `0x00000002` | available: "If `CONTEXT_FLAG_DEBUG_BIT` is set in `CONTEXT_FLAGS`, then the context is a debug context, enabling full support for debug output as described in chapter 20." | available; in the compatibility profile the debug commands are additionally *not recorded into display lists* | spec §22.2; `docs/khronos/extensions/ARB/ARB_debug_output.txt` |
| `CONTEXT_FLAG_ROBUST_ACCESS_BIT` | `0x00000004` | available (GL 4.5 feature) | available (GL 4.5 feature) | spec §22.2; `docs/khronos/extensions/ARB/ARB_robustness.txt` (`CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB 0x00000004`) |
| `CONTEXT_FLAG_NO_ERROR_BIT` | `0x00000008` | available (GL 4.5 feature) | available (GL 4.5 feature) | spec §22.2; `docs/khronos/extensions/KHR/KHR_no_error.txt` (`CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008`) |

Note on the robust/no-error flags: `KHR_no_error.txt` conditions the flag on
"the OpenGL version has support for context flags (as defined in the OpenGL
4.5 core spec)". `CONTEXT_FLAGS` itself is a GL 3.0-era token in the registry
(`gl.xml:31216`, required by `GL_VERSION_3_0`), but the *robust access* and
*no error* bits are GL 4.5-era features.

### 6.3 Querying the profile of a live context

| Query | Result | Citation |
| --- | --- | --- |
| `GetIntegerv(GL_CONTEXT_PROFILE_MASK)` | one of `CONTEXT_CORE_PROFILE_BIT` (`0x00000001`) or `CONTEXT_COMPATIBILITY_PROFILE_BIT` (`0x00000002`) | spec §22.2, Table 22.1 (PDF p. 589, printed p. 567) |
| `GetIntegerv(GL_CONTEXT_FLAGS)` | the four flag bits of §6.2 | spec §22.2 |
| `GetIntegerv(GL_MAJOR_VERSION / GL_MINOR_VERSION)` | the context version | spec §22.2 |
| `GetString(GL_VERSION)` | "<version number><space><vendor specific information>" | spec §22.2 |
| `GetStringi(GL_SHADING_LANGUAGE_VERSION, i)` | e.g. `"430 core"` in a 4.30 core context, `"430 compatibility"` in a 4.30 compatibility context | spec §22.2 |

Registry cross-check: `GL_CONTEXT_PROFILE_MASK` (`gl.xml:6124`,
`0x9126`), `GL_CONTEXT_CORE_PROFILE_BIT` (`gl.xml:176`, `0x00000001`) and
`GL_CONTEXT_COMPATIBILITY_PROFILE_BIT` (`gl.xml:177`, `0x00000002`) are all
required by `GL_VERSION_3_2` **without** a profile attribute
(`gl.xml:31631-31632, 31652`) - i.e. the *tokens* exist in both profiles,
while the *value* returned depends on the profile, exactly as spec Table 22.1
says.

---

## 7. Profile-dependent behaviour differences

| Aspect | Core profile | Compatibility profile | Citation |
| --- | --- | --- | --- |
| Available GLSL version | the GLSL version that matches the GL version (1.50 at GL 3.2 … 4.60 at GL 4.6); `#version` **without** a profile argument defaults to `core` | the same modern GLSL versions, **plus** the `compatibility` profile argument (`#version 460 compatibility`), **plus** GLSL 1.10 and 1.20, which the core profile removed (spec §E.2.2) but the compatibility profile retains | GLSL 4.60 §3.3 ("If no profile argument is provided and the version is 150 or greater, the default is core"); spec App. E §E.2.2 ("OpenGL Shading Language versions 1.10 and 1.20") |
| GLSL profile macros | the GLSL compiler advertises the profiles it supports: "All implementations provide the following macro: `#define GL_core_profile 1`"; "Implementations providing the compatibility profile provide the following macro: `#define GL_compatibility_profile 1`". The macro set is a property of the **GLSL implementation**, not of the GL context's profile | same macro set (the GLSL implementation may support both); in a compatibility context shaders may use the `compatibility` profile | GLSL 4.60 §3.3 |
| Default framebuffer contents | always **RGBA**; "no color index visuals are supplied by the window system-binding APIs such as GLX and WGL, so the default framebuffer is always in RGBA mode. All language and state related to color index mode vertex, rasterization, and fragment processing behavior is removed." | colour-index visuals may be supplied by the window system; the colour-index state (`glClearIndex`, `glIndexMask`) exists | spec App. E §E.2.2 "Color index mode" |
| Fixed-function state exists | **no** - transform matrices, lighting, materials, fog, texture environment, texture coordinate generation, alpha test, stipple, accumulation buffer, pixel transfer modes are all removed | **yes** - the whole fixed-function pipeline is present | spec App. E §E.2.2 (24 bullets) |
| Display lists exist | **no** - "Display lists - `NewList`, `EndList`, `CallList`, `CallLists`, `ListBase`, `GenLists`, `IsList`, and `DeleteLists`; all references to display lists ... are removed" | **yes** | spec App. E §E.2.2 "Display lists" |
| Client-side vertex arrays exist | **no** - "all vertex array attribute and element array index pointers must refer to buffer objects"; `VertexAttribPointer` without a bound buffer/VAO is an `INVALID_OPERATION`; drawing without a bound VAO is an `INVALID_OPERATION`; the default VAO (name zero) is deprecated | **yes** - `glVertexPointer`, `glColorPointer`, `glIndexPointer`, `glTexCoordPointer`, `glNormalPointer`, `glSecondaryColorPointer`, `glEdgeFlagPointer`, `glFogCoordPointer`, `glEnableClientState`, `glDisableClientState`, `glInterleavedArrays`, `glClientActiveTexture` are all present | spec App. E §E.2.2 "Client vertex and index arrays"; `gl.xml:31735` etc. |
| `"gl_"` legacy built-ins exist | **no** for the fixed-function ones: `gl_Vertex`, `gl_Normal`, `gl_Color`, `gl_SecondaryColor`, `gl_MultiTexCoord0..7`, `gl_FogCoord` are compatibility-profile vertex-shader *inputs*; `gl_BackColor` / `gl_BackSecondaryColor` are removed from core; GLSL 1.10 and 1.20 are removed | **yes**: compatibility-profile vertex shader built-in inputs `gl_Color`, `gl_SecondaryColor`, `gl_Normal`, `gl_Vertex`, `gl_MultiTexCoord0..7`, `gl_FogCoord`; compatibility-profile outputs `gl_ClipVertex`, `gl_FrontColor`, `gl_BackColor`, `gl_FrontSecondaryColor`, `gl_BackSecondaryColor`, `gl_TexCoord[]`, `gl_FogFragCoord`; compatibility-profile state `gl_ModelViewMatrix`, `gl_ProjectionMatrix`, `gl_NormalMatrix`, `gl_ClipPlane[]`, `gl_PointParameters`, … ; `gl_FrontColor`/`gl_BackColor`/`gl_FrontSecondaryColor`/`gl_BackSecondaryColor`/`gl_Color`/`gl_SecondaryColor` may be redeclared with an interpolation qualifier | GLSL 4.60 §7.2, §7.1.7, §7.4.1, §4.5.1; spec App. E §E.2.2 "Two-sided color selection" |
| Primitive assembly | triangles, lines, points, patches only; `POLYGON`, `QUADS`, `QUAD_STRIP` drawing modes removed; `PolygonMode` face values `FRONT`/`BACK` removed; edge flags are always `TRUE` | full polygon/quad primitive set, edge flags, two-sided colour selection | spec App. E §E.2.2 |
| Wide lines | `LineWidth > 1.0` → `INVALID_VALUE` (deprecated but supported) | legal | spec App. E §E.2.1 / §E.2.2 |
| Line stipple | removed (`glLineStipple`, `LINE_STIPPLE` enable target) | present | spec App. E §E.2.2 "Wide lines and line stipple" |
| Extension enumeration | `GetStringi(GL_EXTENSIONS, i)` + `GL_NUM_EXTENSIONS`; the unified `GetString(GL_EXTENSIONS)` target is removed | unified `GetString(GL_EXTENSIONS)` available | spec App. E §E.2.2 "Unified extension string"; §22.2 |
| Object namespaces | no `DISPLAY_LIST` namespace | `DISPLAY_LIST` namespace (`0x82E7`) exists | `docs/khronos/extensions/KHR/KHR_debug.txt` (`[[ Compatibility Profile ]] DISPLAY_LIST 0x82E7`); `gl.xml:33519` |
| Error behaviour for removed functions | "Functions which have been removed will generate an `INVALID_OPERATION` error if called in the core profile" | the functions exist, so no error | spec App. E §E.2 |

---

## 8. Where `gl.xml` and the GL 4.6 Core Profile specification PDF disagree

The two sources agree on the profile *model*, on the split boundary (GL 3.2),
on the 657 / 42 / 391 counts, and on the fact that the compatibility profile
removes nothing. The divergences below are the ones that actually affect a
tool that wants to derive behaviour from the registry.

### D1 - `glCopyPixels` is removed from core by the registry but is still named in the Core Profile spec's normative body text

* **Registry**: `glCopyPixels` is removed from the core profile by
  `<remove profile="core">` in `GL_VERSION_3_2` (`gl.xml:31735` block,
  "Compatibility-only GL 1.0 features removed from GL 3.2"). It is *not*
  restored, so it is one of the 349 fixed-function commands that exist only
  in the compatibility profile.
* **Specification PDF**: the Core Profile document still names it in
  **normative prose** in §8.6 "Alternate Texture Image Specification
  Commands" (printed p. 218):
  > "The image is taken from the framebuffer exactly as if these arguments
  > were passed to `CopyPixels` (see section 18.3) with argument type set to
  > `COLOR`, `DEPTH`, `DEPTH_STENCIL`, or `STENCIL_INDEX`, depending on
  > `internalformat` ..."
  The document's own Index of Terms lists `CopyPixels, 218, 682` - page 218 is
  that §8.6 reference, page 682 is the Appendix E.2.2 removal bullet
  "• Pixel copying - `CopyPixels`".
* **Impact**: a tool that derives "which commands exist in the core profile"
  from the registry gets the right answer (350 removed, 349 net), but the
  Core Profile PDF's body text still refers to a command that the core
  profile does not have.

  `glCopyPixels` is the **only** command in the 391-set that the Core
  Profile PDF still references in normative body text. The document's Index
  of Terms names **55** of the 391 compatibility-only commands:

  | Where the 55 are referenced | count |
  | --- | ---: |
  | Appendix E.2.2 "Removed Features" only (printed pp. 679-683) | 53 |
  | Appendix H.3 change log only (printed p. 715) | 1 (`glGetPolygonStipple`) |
  | **Body text as well** (§8.6, printed p. 218) | **1 (`glCopyPixels`)** |

### D2 - the registry carries no machine-readable deprecation data at all

The specification's E.2.1 "Deprecated But Still Supported Features" list has
four items. The registry expresses three of them (the six tokens) only inside
**XML comments**, because the `<deprecate>` tag is not defined by the
registry schema yet:

| Registry location | Commented-out `<deprecate profile="core">` block |
| --- | --- |
| `gl.xml:32535-32540` | "Deprecated (not removed) in OpenGL 3.2 core; deprecate tag not defined/supported yet" → `GL_MAX_VARYING_FLOATS`, `GL_MAX_VARYING_COMPONENTS` |
| `gl.xml:33117-33122` | "Deprecated in OpenGL 4.2 core; deprecate tag not defined/supported yet" → `GL_NUM_COMPRESSED_TEXTURE_FORMATS`, `GL_COMPRESSED_TEXTURE_FORMATS` |
| `gl.xml:33526-33531` | "Deprecated in OpenGL 4.3 core; deprecate tag not defined/supported yet" → `GL_UNPACK_LSB_FIRST`, `GL_PACK_LSB_FIRST` |

There is no `<deprecate>` element anywhere in the file outside those
comments. A tool that parses `gl.xml` therefore sees **zero** deprecation
information, and the "wide lines" item (a behavioural restriction on
`LineWidth`, not a token) cannot be expressed in the registry at all.
The *substance* agrees between the two sources; the *encoding* does not.

### D3 - the registry's model is entry-point/token-level, the specification's removal model is feature-level

The specification removes whole *features*, including state, enable targets
and implementation-dependent values, not just entry points:

* "Enable/Disable targets `RESCALE_NORMAL` and `NORMALIZE`"
* "Enable targets of all dimensionalities (`TEXTURE_1D`, `TEXTURE_2D`,
  `TEXTURE_3D`, `TEXTURE_1D_ARRAY`, `TEXTURE_2D_ARRAY`, and `TEXTURE_CUBE_MAP`)"
* "the implementation-dependent values `MAX_TEXTURE_UNITS` and `MAX_TEXTURE_COORDS`"
* "the `PERSPECTIVE_CORRECTION_HINT`, `POINT_SMOOTH_HINT`, `FOG_HINT`, and
  `GENERATE_MIPMAP_HINT` targets to `Hint`"
* "Auxiliary color buffers, including `AUXi` targets of the default framebuffer"
* "Context framebuffer size queries - `RED_BITS`, `GREEN_BITS`, `BLUE_BITS`,
  `ALPHA_BITS`, `DEPTH_BITS`, and `STENCIL_BITS`"

The registry removes the *tokens* for these (`GL_FOG`, `GL_LIGHTING`,
`GL_MAX_TEXTURE_UNITS`, `GL_MAX_TEXTURE_COORDS`, `GL_FOG_HINT`, … all appear
in the `<remove profile="core">` enum lists), but the registry has no way to
express "this token still exists in the header, but a core-profile driver
must reject it as an `Enable`/`Disable`/`Hint`/`GetIntegerv` target". A
registry-driven implementation will accept `glEnable(GL_FOG)` in a core
context unless it implements the behavioural rule by hand.

### D4 - `GL_EXTENSIONS` is a both-profile token in the registry, but the unified string is a removed feature in core

* **Registry**: `GL_EXTENSIONS` is required by `GL_VERSION_1_0` with **no**
  profile attribute (`gl.xml:29905`), i.e. it is available in the core
  profile. `GL_NUM_EXTENSIONS` is likewise required by `GL_VERSION_3_0`
  without a profile attribute (`gl.xml:31215`).
* **Specification**: §E.2.2 removes "Unified extension string - `EXTENSIONS`
  target to `GetString`" from the core profile.

The registry cannot express "remove one *target* of one command", so a
registry-driven core-profile implementation will happily answer
`GetString(GL_EXTENSIONS)` unless it implements the rule by hand.

### D5 - no disagreement on the numbers themselves

For the record, the following were checked and **agree**: the split boundary
(GL 3.2), the core count at 4.6 (657), the registry compatibility-only count
(42), the compatibility-but-not-core count (391), the 350 removed at 3.2 and
the 349 that survive to 4.6, the restoration of `glGetPointerv` /
`GL_STACK_UNDERFLOW` / `GL_STACK_OVERFLOW` to core in 4.3, the fact that
the compatibility profile removes nothing, and the four E.2.1
deprecated-but-still-supported features (they agree in substance; see D2 for
the encoding difference).

---

## 9. What this document does **not** claim

* It does **not** claim that TGLMT implements any of the API described here.
  The measured implementation status is in
  [`docs/OPENGL_API_COVERAGE.md`](OPENGL_API_COVERAGE.md).
* It does **not** claim CTS conformance, dTLS conformance, or any conformance
  test result.
* It does **not** describe runtime behaviour of any implementation. Every
  behavioural statement in this document is a quotation of, or a direct
  paraphrase of, the specification or the registry, with a citation.
* It does **not** cover OpenGL ES, OpenGL ES 2/3, WebGL, or the
  `GL_ARB_*`/`GL_KHR_*` extension entry points (those are counted separately
  in `data/version_inventory.json` under `extensions`).
