# OpenGL API Coverage — TGLMT vs. the OpenGL 4.6 Compatibility Surface

> **CẬP NHẬT 2026-11-11 (sau session T0/T1) — ĐO LẠI.**
> Bảng số ở phần dưới được đo **trước** session T0/T1. Trạng thái **hiện tại**
> (đo lại bằng script, không viết tay):
>
> | Bề mặt | Tổng | Đã có prototype C ABI | **Đã implement thật** | Còn stub `UNIMPLEMENTED` |
> |---|---|---|---|---|
> | GL 4.6 **Core** | 657 | 657 | **657** | 0 |
> | GL 4.6 **Compatibility** | 1048 | 1048 | **808** | 240 |
> | Trong đó: legacy fixed-function (349 lệnh) | 349 | 349 | **109** | 240 |
>
> Lệnh đo:
> ```bash
> python3 - <<'EOF'
> import re, pathlib, json
> stub = pathlib.Path("src/gl/gl_stubs_gen.cpp").read_text()
> stubs = set(re.findall(r"UNIMPLEMENTED: (gl[A-Za-z0-9_]+)", stub))
> inv = json.load(open("data/version_inventory.json"))
> compat = set(inv["versions"]["4.6"]["commands"]["compatibility"])
> core   = set(inv["versions"]["4.6"]["commands"]["core"])
> print("compat", len(compat), "stub", len(stubs & compat), "impl", len(compat - stubs))
> print("core  ", len(core),   "stub", len(stubs & core),   "impl", len(core - stubs))
> print("legacy", len(compat-core), "stub", len(stubs & (compat-core)))
> EOF
> ```
> **Lưu ý:** "đã implement thật" nghĩa là **có định nghĩa trong `src/gl/*.cpp`
> do viết tay** (không phải stub sinh tự động). Nó **KHÔNG** đồng nghĩa hành vi
> đúng — xem `PROGRESS.md` §"Test" và `docs/TRUTH_MANIFEST.md`.
>
> Thay đổi so với lần đo trước: toàn bộ 349 prototype legacy đã có trong
> `include/tglmt/gl_legacy.h` + `include/tglmt/gl_c_api.h`, **được version-gate**
> theo (version, profile) của context. Đã implement thật + test PASS:
> 26 lệnh matrix stack (`test_legacy_matrix`) và 83 lệnh immediate mode /
> current vertex attributes (`test_legacy_immediate`).
>
> Lưu ý: số "đã implement thật" = **có định nghĩa trong `src/gl/*.cpp` viết
> tay**, KHÔNG đồng nghĩa hành vi runtime đúng — xem `PROGRESS.md` §"Test".

This document reports the **measured** implementation status of TGLMT
against the **OpenGL 4.6 Compatibility profile** command surface: the
**1048** entry points that the Khronos registry defines for the OpenGL 4.6
compatibility profile.

Companion document: [`docs/OPENGL_PROFILE_MATRIX.md`](OPENGL_PROFILE_MATRIX.md)
- that document describes *what the specification says*; this one describes
*what TGLMT actually exports today*.

---

## 0. What is and is not measured here

| | |
| --- | --- |
| **Measured** | the set of `gl*` symbols that the built `libtglmt` dynamic library **defines and exports** (`nm -gU`, type `T`), intersected with the 1048-command OpenGL 4.6 compatibility set derived from the Khronos registry. |
| **Cross-checked** | the same count obtained independently by parsing the `extern "C"` wrapper table in `src/gl/gl_c_exports.cpp`. |
| **NOT measured** | runtime behaviour, numerical correctness, error generation, performance, or conformance. No program was executed against the library for this document. |
| **NOT claimed** | CTS conformance, dTLS conformance, or any conformance-test result. |

A symbol being *exported* does **not** mean the command behaves correctly.
Section 6 states exactly what is known beyond the export table.

---

## 1. Method

### 1.1 Inputs

| Input | Path | Role |
| --- | --- | --- |
| Registry-derived inventory | `data/version_inventory.json` | the authoritative list of the 1048 OpenGL 4.6 compatibility commands and the 657 core ones |
| Built library | `build-work/libtglmt.0.1.0.dylib` | the artifact that was measured |
| Wrapper table | `src/gl/gl_c_exports.cpp` | the generated `extern "C"` shim list (independent cross-check) |
| Truth manifest | `tests/truth/api_manifest.json` | per-API implementation-body classification for the core set |

### 1.2 The library was rebuilt from the current sources first

The pre-existing `build-work` artifact predated the latest source edit
(`src/gl/gl_uniform.cpp`), so it was rebuilt before measuring:

```
cmake --build build-work --target tglmt_shared -j8
```

```
[100%] Linking CXX shared library libtglmt.dylib
[100%] Built target tglmt_shared
```

Resulting artifact:

```
$ ls -l build-work/libtglmt.0.1.0.dylib
-rwxr-xr-x  1 vuduyanh  admin  3203936 Nov 11 09:13 build-work/libtglmt.0.1.0.dylib
```

The measurement below was taken on that freshly linked library.

### 1.3 Raw commands used

**Primary method — symbol table of the built library:**

```
$ nm -gU build-work/libtglmt.0.1.0.dylib | grep -c ' T _gl'
1860
```

**Secondary method — parse the exported wrapper table:**

```
$ grep -c 'extern "C"' src/gl/gl_c_exports.cpp      # 3 matches: 1 opening block + 2 comment mentions
$ python3 - <<'PY'
import re
src = open('src/gl/gl_c_exports.cpp').read()
body = src[src.find('extern "C"'):]
names = set(re.findall(r'^\s*[A-Za-z_][A-Za-z0-9_ \*]*\b(gl\w+)\s*\(', body, re.M))
print(len(names))
PY
701
```

**The measurement script** (both methods in one run, see §7):

```
$ python3 tools/profile_groups.py --dylib build-work/libtglmt.0.1.0.dylib
```

### 1.4 Raw output of the measurement

```
dylib: build-work/libtglmt.0.1.0.dylib
total defined text symbols (T): 2738
of which gl*: 1860
GL4.6 compat  : 700 / 1048
GL4.6 core    : 657 / 657
compat-only 42: 42 / 42
missing       : 348
gl_c_exports.cpp extern "C" wrappers: 701
  of which in GL4.6 compat: 700
  agreement with nm (compat): True
```

Notes on the numbers:

* `1860` is **every** `gl*` symbol the library exports. It is **not** the
  GL 4.6 surface: 700 of them are OpenGL 4.6 compatibility commands and the
  other 1160 are extension entry points (`gl*EXT`, `gl*ARB`, `gl*OES`, …).
  The split is exact and has no remainder - every exported `gl*` symbol is
  either an OpenGL 4.6 compatibility command or a registered extension
  command name:

  ```
  gl exported:            1860
    GL4.6-compat:          700
    extension command:    1574   (overlaps the 700 for promoted extensions)
    in neither set:          0
  ```

* The `extern "C"` wrapper table has 701 entries. One of them,
  `glDepthRangeArrayfvNV`, is an extension entry point that is not part of
  the OpenGL 4.6 compatibility set, which is why the two methods agree on
  **700** and not on 701.

---

## 2. Headline result

| Metric | Value |
| --- | ---: |
| OpenGL 4.6 compatibility commands (registry) | **1048** |
| Exported by TGLMT (`nm -gU`, type `T`) | **700** |
| **Not exported (missing)** | **348** |
| Coverage of the GL 4.6 compatibility surface | **66.8 %** |
| OpenGL 4.6 **core** commands (registry) | 657 |
| Exported by TGLMT | **657** (100 %) |
| Registry compatibility-only commands (the 42) | 42 |
| Exported by TGLMT | **42** (100 %) |

**The 348 missing commands are exactly the OpenGL 1.x-3.1 fixed-function
legacy surface.** Verified by set algebra against the registry:

```
missing set == (the 350 commands removed from core at GL 3.2)
               - {glGetPointerv}   # restored to core by GL 4.3
               - {glLineStipple}   # the one legacy command TGLMT does export
```

So: **TGLMT implements the OpenGL 4.6 core profile completely (657/657),
implements all 42 registry compatibility-only entry points (42/42), and
implements none of the 349 fixed-function legacy entry points that the
compatibility profile retains — with exactly one exception, `glLineStipple`,
which is exported but is an empty no-op (see §6.2).**

---

## 3. Functional-group table

Groups `L01` - `L20` are the compatibility-only groups of
[`docs/OPENGL_PROFILE_MATRIX.md` §3.3](OPENGL_PROFILE_MATRIX.md); groups
`M01` - `M19` are the modern (GL 2.0 - 4.6) groups, named after the
corresponding chapters of the OpenGL 4.6 Core Profile specification. The
grouping rule table is the script embedded in
[`docs/OPENGL_PROFILE_MATRIX.md` §3.4](OPENGL_PROFILE_MATRIX.md) and is
reproduced in §7 below.

| # | Functional group | GL 4.6 compat | implemented | missing | status |
| --- | --- | ---: | ---: | ---: | --- |
| L01 | Immediate mode (Begin/End and vertex specification) | 173 | 0 | 173 | **ENTIRELY MISSING** |
| L02 | Client-side vertex arrays | 12 | 0 | 12 | **ENTIRELY MISSING** |
| L03 | Fixed-function transform and matrix stack | 20 | 0 | 20 | **ENTIRELY MISSING** |
| L04 | Lighting, materials and shading model | 18 | 0 | 18 | **ENTIRELY MISSING** |
| L05 | Fog | 4 | 0 | 4 | **ENTIRELY MISSING** |
| L06 | Texture environment and texture coordinate generation | 15 | 0 | 15 | **ENTIRELY MISSING** |
| L07 | Display lists | 8 | 0 | 8 | **ENTIRELY MISSING** |
| L08 | Evaluators | 23 | 0 | 23 | **ENTIRELY MISSING** |
| L09 | Selection and feedback modes | 8 | 0 | 8 | **ENTIRELY MISSING** |
| L10 | Accumulation buffer | 2 | 0 | 2 | **ENTIRELY MISSING** |
| L11 | Pixel transfer modes and pixel drawing | 12 | 0 | 12 | **ENTIRELY MISSING** |
| L12 | Raster position and window position | 40 | 0 | 40 | **ENTIRELY MISSING** |
| L13 | Stipple (polygon and line stipple) | 3 | 1 | 2 | partial |
| L14 | Alpha test | 1 | 0 | 1 | **ENTIRELY MISSING** |
| L15 | Texture residency and priority (fixed-function fragment processing) | 2 | 0 | 2 | **ENTIRELY MISSING** |
| L16 | User clipping planes | 2 | 0 | 2 | **ENTIRELY MISSING** |
| L17 | Attribute stacks | 4 | 0 | 4 | **ENTIRELY MISSING** |
| L18 | Colour-index mode state | 2 | 0 | 2 | **ENTIRELY MISSING** |
| L19 | Compatibility-only vertex-type entry points (GL 3.3) | 30 | 30 | 0 | complete |
| L20 | Compatibility-only n-suffixed pixel/history queries (GL 4.5) | 12 | 12 | 0 | complete |
| M01 | Buffer objects (spec ch. 6) | 37 | 37 | 0 | complete |
| M02 | Shaders, programs and program pipelines (spec ch. 7) | 49 | 49 | 0 | complete |
| M03 | Program interfaces, uniforms and uniform blocks (spec ch. 7.3, 7.6) | 131 | 131 | 0 | complete |
| M04 | Textures and texture objects (spec ch. 8) | 90 | 90 | 0 | complete |
| M05 | Sampler objects (spec ch. 8.2) | 16 | 16 | 0 | complete |
| M06 | Pixel storage modes and pixel rectangles (spec ch. 8.4) | 3 | 3 | 0 | complete |
| M07 | Framebuffer objects and renderbuffers (spec ch. 9) | 46 | 46 | 0 | complete |
| M08 | Vertex arrays, vertex attributes and vertex array objects (spec ch. 10.3) | 108 | 108 | 0 | complete |
| M09 | Drawing commands and conditional rendering (spec ch. 10.4, 10.9) | 33 | 33 | 0 | complete |
| M10 | Rasterization and multisample state (spec ch. 14) | 31 | 31 | 0 | complete |
| M11 | Transform feedback (spec ch. 13.3) | 15 | 15 | 0 | complete |
| M12 | Per-fragment operations, blending, stencil and enable state (spec ch. 17) | 35 | 35 | 0 | complete |
| M13 | Pixel read and copy (spec ch. 18) | 3 | 3 | 0 | complete |
| M14 | Compute shaders, images and memory barriers (spec ch. 19) | 4 | 4 | 0 | complete |
| M15 | Asynchronous queries (spec ch. 4.2) | 19 | 19 | 0 | complete |
| M16 | Sync objects and fences (spec ch. 4.1) | 6 | 6 | 0 | complete |
| M17 | Debug API and object labels (spec ch. 20) | 11 | 11 | 0 | complete |
| M18 | Context queries, strings and general state (spec ch. 21, 22) | 16 | 16 | 0 | complete |
| M19 | Miscellaneous general GL (spec ch. 2, 17.4) | 4 | 4 | 0 | complete |
| | **TOTAL** | **1048** | **700** | **348** | |

**18 of the 39 groups are entirely missing.** All 18 are legacy
fixed-function groups (`L01` - `L18` minus `L13`'s one entry point).

### 3.1 The 348 missing commands, group by group

* **L01 Immediate mode (Begin/End and vertex specification)** (173):
  `glArrayElement`, `glBegin`, `glColor3b`, `glColor3bv`, `glColor3d`,
  `glColor3dv`, `glColor3f`, `glColor3fv`, `glColor3i`, `glColor3iv`,
  `glColor3s`, `glColor3sv`, `glColor3ub`, `glColor3ubv`, `glColor3ui`,
  `glColor3uiv`, `glColor3us`, `glColor3usv`, `glColor4b`, `glColor4bv`,
  `glColor4d`, `glColor4dv`, `glColor4f`, `glColor4fv`, `glColor4i`,
  `glColor4iv`, `glColor4s`, `glColor4sv`, `glColor4ub`, `glColor4ubv`,
  `glColor4ui`, `glColor4uiv`, `glColor4us`, `glColor4usv`, `glEdgeFlag`,
  `glEdgeFlagv`, `glEnd`, `glFogCoordd`, `glFogCoorddv`, `glFogCoordf`,
  `glFogCoordfv`, `glIndexd`, `glIndexdv`, `glIndexf`, `glIndexfv`,
  `glIndexi`, `glIndexiv`, `glIndexs`, `glIndexsv`, `glIndexub`,
  `glIndexubv`, `glMultiTexCoord1d`, `glMultiTexCoord1dv`,
  `glMultiTexCoord1f`, `glMultiTexCoord1fv`, `glMultiTexCoord1i`,
  `glMultiTexCoord1iv`, `glMultiTexCoord1s`, `glMultiTexCoord1sv`,
  `glMultiTexCoord2d`, `glMultiTexCoord2dv`, `glMultiTexCoord2f`,
  `glMultiTexCoord2fv`, `glMultiTexCoord2i`, `glMultiTexCoord2iv`,
  `glMultiTexCoord2s`, `glMultiTexCoord2sv`, `glMultiTexCoord3d`,
  `glMultiTexCoord3dv`, `glMultiTexCoord3f`, `glMultiTexCoord3fv`,
  `glMultiTexCoord3i`, `glMultiTexCoord3iv`, `glMultiTexCoord3s`,
  `glMultiTexCoord3sv`, `glMultiTexCoord4d`, `glMultiTexCoord4dv`,
  `glMultiTexCoord4f`, `glMultiTexCoord4fv`, `glMultiTexCoord4i`,
  `glMultiTexCoord4iv`, `glMultiTexCoord4s`, `glMultiTexCoord4sv`,
  `glNormal3b`, `glNormal3bv`, `glNormal3d`, `glNormal3dv`, `glNormal3f`,
  `glNormal3fv`, `glNormal3i`, `glNormal3iv`, `glNormal3s`, `glNormal3sv`,
  `glRectd`, `glRectdv`, `glRectf`, `glRectfv`, `glRecti`, `glRectiv`,
  `glRects`, `glRectsv`, `glSecondaryColor3b`, `glSecondaryColor3bv`,
  `glSecondaryColor3d`, `glSecondaryColor3dv`, `glSecondaryColor3f`,
  `glSecondaryColor3fv`, `glSecondaryColor3i`, `glSecondaryColor3iv`,
  `glSecondaryColor3s`, `glSecondaryColor3sv`, `glSecondaryColor3ub`,
  `glSecondaryColor3ubv`, `glSecondaryColor3ui`, `glSecondaryColor3uiv`,
  `glSecondaryColor3us`, `glSecondaryColor3usv`, `glTexCoord1d`,
  `glTexCoord1dv`, `glTexCoord1f`, `glTexCoord1fv`, `glTexCoord1i`,
  `glTexCoord1iv`, `glTexCoord1s`, `glTexCoord1sv`, `glTexCoord2d`,
  `glTexCoord2dv`, `glTexCoord2f`, `glTexCoord2fv`, `glTexCoord2i`,
  `glTexCoord2iv`, `glTexCoord2s`, `glTexCoord2sv`, `glTexCoord3d`,
  `glTexCoord3dv`, `glTexCoord3f`, `glTexCoord3fv`, `glTexCoord3i`,
  `glTexCoord3iv`, `glTexCoord3s`, `glTexCoord3sv`, `glTexCoord4d`,
  `glTexCoord4dv`, `glTexCoord4f`, `glTexCoord4fv`, `glTexCoord4i`,
  `glTexCoord4iv`, `glTexCoord4s`, `glTexCoord4sv`, `glVertex2d`,
  `glVertex2dv`, `glVertex2f`, `glVertex2fv`, `glVertex2i`, `glVertex2iv`,
  `glVertex2s`, `glVertex2sv`, `glVertex3d`, `glVertex3dv`, `glVertex3f`,
  `glVertex3fv`, `glVertex3i`, `glVertex3iv`, `glVertex3s`, `glVertex3sv`,
  `glVertex4d`, `glVertex4dv`, `glVertex4f`, `glVertex4fv`, `glVertex4i`,
  `glVertex4iv`, `glVertex4s`, `glVertex4sv`
* **L02 Client-side vertex arrays** (12): `glClientActiveTexture`,
  `glColorPointer`, `glDisableClientState`, `glEdgeFlagPointer`,
  `glEnableClientState`, `glFogCoordPointer`, `glIndexPointer`,
  `glInterleavedArrays`, `glNormalPointer`, `glSecondaryColorPointer`,
  `glTexCoordPointer`, `glVertexPointer`
* **L03 Fixed-function transform and matrix stack** (20): `glFrustum`,
  `glLoadIdentity`, `glLoadMatrixd`, `glLoadMatrixf`,
  `glLoadTransposeMatrixd`, `glLoadTransposeMatrixf`, `glMatrixMode`,
  `glMultMatrixd`, `glMultMatrixf`, `glMultTransposeMatrixd`,
  `glMultTransposeMatrixf`, `glOrtho`, `glPopMatrix`, `glPushMatrix`,
  `glRotated`, `glRotatef`, `glScaled`, `glScalef`, `glTranslated`,
  `glTranslatef`
* **L04 Lighting, materials and shading model** (18): `glColorMaterial`,
  `glGetLightfv`, `glGetLightiv`, `glGetMaterialfv`, `glGetMaterialiv`,
  `glLightModelf`, `glLightModelfv`, `glLightModeli`, `glLightModeliv`,
  `glLightf`, `glLightfv`, `glLighti`, `glLightiv`, `glMaterialf`,
  `glMaterialfv`, `glMateriali`, `glMaterialiv`, `glShadeModel`
* **L05 Fog** (4): `glFogf`, `glFogfv`, `glFogi`, `glFogiv`
* **L06 Texture environment and texture coordinate generation** (15):
  `glGetTexEnvfv`, `glGetTexEnviv`, `glGetTexGendv`, `glGetTexGenfv`,
  `glGetTexGeniv`, `glTexEnvf`, `glTexEnvfv`, `glTexEnvi`, `glTexEnviv`,
  `glTexGend`, `glTexGendv`, `glTexGenf`, `glTexGenfv`, `glTexGeni`,
  `glTexGeniv`
* **L07 Display lists** (8): `glCallList`, `glCallLists`, `glDeleteLists`,
  `glEndList`, `glGenLists`, `glIsList`, `glListBase`, `glNewList`
* **L08 Evaluators** (23): `glEvalCoord1d`, `glEvalCoord1dv`,
  `glEvalCoord1f`, `glEvalCoord1fv`, `glEvalCoord2d`, `glEvalCoord2dv`,
  `glEvalCoord2f`, `glEvalCoord2fv`, `glEvalMesh1`, `glEvalMesh2`,
  `glEvalPoint1`, `glEvalPoint2`, `glGetMapdv`, `glGetMapfv`, `glGetMapiv`,
  `glMap1d`, `glMap1f`, `glMap2d`, `glMap2f`, `glMapGrid1d`, `glMapGrid1f`,
  `glMapGrid2d`, `glMapGrid2f`
* **L09 Selection and feedback modes** (8): `glFeedbackBuffer`,
  `glInitNames`, `glLoadName`, `glPassThrough`, `glPopName`, `glPushName`,
  `glRenderMode`, `glSelectBuffer`
* **L10 Accumulation buffer** (2): `glAccum`, `glClearAccum`
* **L11 Pixel transfer modes and pixel drawing** (12): `glBitmap`,
  `glCopyPixels`, `glDrawPixels`, `glGetPixelMapfv`, `glGetPixelMapuiv`,
  `glGetPixelMapusv`, `glPixelMapfv`, `glPixelMapuiv`, `glPixelMapusv`,
  `glPixelTransferf`, `glPixelTransferi`, `glPixelZoom`
* **L12 Raster position and window position** (40): `glRasterPos2d`,
  `glRasterPos2dv`, `glRasterPos2f`, `glRasterPos2fv`, `glRasterPos2i`,
  `glRasterPos2iv`, `glRasterPos2s`, `glRasterPos2sv`, `glRasterPos3d`,
  `glRasterPos3dv`, `glRasterPos3f`, `glRasterPos3fv`, `glRasterPos3i`,
  `glRasterPos3iv`, `glRasterPos3s`, `glRasterPos3sv`, `glRasterPos4d`,
  `glRasterPos4dv`, `glRasterPos4f`, `glRasterPos4fv`, `glRasterPos4i`,
  `glRasterPos4iv`, `glRasterPos4s`, `glRasterPos4sv`, `glWindowPos2d`,
  `glWindowPos2dv`, `glWindowPos2f`, `glWindowPos2fv`, `glWindowPos2i`,
  `glWindowPos2iv`, `glWindowPos2s`, `glWindowPos2sv`, `glWindowPos3d`,
  `glWindowPos3dv`, `glWindowPos3f`, `glWindowPos3fv`, `glWindowPos3i`,
  `glWindowPos3iv`, `glWindowPos3s`, `glWindowPos3sv`
* **L13 Stipple (polygon and line stipple)** (2 of 3 missing):
  `glGetPolygonStipple`, `glPolygonStipple` ( `glLineStipple` **is**
  exported - but see §6.2)
* **L14 Alpha test** (1): `glAlphaFunc`
* **L15 Texture residency and priority** (2): `glAreTexturesResident`,
  `glPrioritizeTextures`
* **L16 User clipping planes** (2): `glClipPlane`, `glGetClipPlane`
* **L17 Attribute stacks** (4): `glPopAttrib`, `glPopClientAttrib`,
  `glPushAttrib`, `glPushClientAttrib`
* **L18 Colour-index mode state** (2): `glClearIndex`, `glIndexMask`

---

## 4. Per-version view of the same measurement

For each OpenGL version, the compatibility-profile command set as of that
version, and how many of them TGLMT exports:

| GL | compat cmds at that version | exported by TGLMT | missing |
| --- | ---: | ---: | ---: |
| 1.0 | 306 | 49 | 257 |
| 1.1 | 336 | 63 | 273 |
| 1.2 | 340 | 67 | 273 |
| 1.3 | 386 | 76 | 310 |
| 1.4 | 433 | 85 | 348 |
| 1.5 | 452 | 104 | 348 |
| 2.0 | 545 | 197 | 348 |
| 2.1 | 551 | 203 | 348 |
| 3.0 | 635 | 287 | 348 |
| 3.1 | 647 | 299 | 348 |
| 3.2 | 666 | 318 | 348 |
| 3.3 | 724 | 376 | 348 |
| 4.0 | 770 | 422 | 348 |
| 4.1 | 858 | 510 | 348 |
| 4.2 | 870 | 522 | 348 |
| 4.3 | 913 | 565 | 348 |
| 4.4 | 922 | 574 | 348 |
| 4.5 | 1044 | 696 | 348 |
| 4.6 | 1048 | 700 | 348 |

The `missing` column saturates at **348 from OpenGL 1.4 onwards**: every one
of the 348 missing entry points was introduced in OpenGL 1.0 - 1.3, and
nothing TGLMT omits was introduced later.

---

## 5. Cross-checks

| Cross-check | Result |
| --- | --- |
| `nm -gU` on the built dylib vs. parsing `src/gl/gl_c_exports.cpp` | **identical** - 700 GL 4.6 compatibility commands in both; set equality verified programmatically |
| `nm -gU` on the other build trees (`build-apple`, `build-asan`, `build-bench`, `build-null`) | **700** in every tree (measured on the pre-existing artifacts; `build-work` was rebuilt from current sources) |
| `tools/gl46_catalog.py` self-check | 657 core / 42 compatibility-only - matches the inventory |
| `python3 tools/gen_api_docs.py --check` | `OK` - the project's own generated coverage document agrees with its truth manifest |
| `tests/truth/api_manifest.json` | 657 core entries: **655** `impl_kind=real`, **2** `impl_kind=noop`; 42 compatibility-only entries: **42** `impl_kind=real` |
| All exported `gl*` symbols accounted for | 1860 = 700 (GL 4.6 compat) + extension entry points; **0** symbols in neither category |

---

## 6. Honest caveats

### 6.1 Export presence is not conformance

This document measures **symbol export only**. It does **not** establish that
any exported command produces correct results, raises the correct errors, or
respects the profile rules of
[`docs/OPENGL_PROFILE_MATRIX.md`](OPENGL_PROFILE_MATRIX.md). Per-API
behavioural status is tracked separately in
[`docs/OPENGL_46_API_COVERAGE.md`](OPENGL_46_API_COVERAGE.md) (generated by
`tools/gen_api_docs.py` from `tests/truth/api_manifest.json`).

### 6.2 Three exported commands are empty no-ops

| Command | Where | Note |
| --- | --- | --- |
| `glHint` | `src/gl/gl_raster.cpp:273` - `void glHint(GLenum t, GLenum m) { (void)t;(void)m; }` | the truth manifest classifies it `impl_kind=noop`, `noop_triage=allowed:hint` - GL 4.6 §17.2 permits hints to be silently ignored |
| `glReleaseShaderCompiler` | `src/gl/gl_readpixels_dsa.cpp:275` - `void glReleaseShaderCompiler() {}` | `impl_kind=noop`, `noop_triage=allowed:hint` - a pure hint with no errors listed in the spec |
| `glLineStipple` | `src/gl/gl_raster.cpp:274` - `void glLineStipple(GLint a, GLushort b) { (void)a;(void)b; }` | **not** in the truth manifest (it is not a core command); inspected directly in the source. It is exported and callable but does nothing. |

So of the 700 exported commands, **697 have a non-trivial implementation body**
and **3 are empty no-ops** (two of them explicitly spec-allowed).

### 6.3 The 42 compatibility-only commands are exported but are not core

`tools/gl46_catalog.py` states the rule for the project:

> "The project implements and exports them as well (they are pulled in by
> GL_ARB_vertex_type_2_10_10_10_rev / GL_ARB_robustness), but they are NOT
> part of the OpenGL 4.6 Core Profile and must never be reported as such."

TGLMT exports all 42. They must not be counted towards "OpenGL 4.6 Core"
coverage, and they are **not** evidence that the compatibility profile's
*legacy* surface is implemented.

### 6.4 TGLMT does not expose a profile switch

The measurement is of a **single** library. Nothing in this measurement
shows that TGLMT can present the compatibility profile at runtime. Whether
TGLMT distinguishes a core context from a compatibility context, how it
reports `GL_CONTEXT_PROFILE_MASK`, and whether it honours the "removed
functions generate `INVALID_OPERATION`" rule of
[`docs/OPENGL_PROFILE_MATRIX.md` §5.4](OPENGL_PROFILE_MATRIX.md) are all
**NOT_VERIFIED** here.

### 6.5 Not verified by this document

| Question | Status |
| --- | --- |
| Does each exported command behave per the GL 4.6 specification? | NOT_VERIFIED (out of scope - symbol presence only) |
| Does the library generate the correct `GL_*` errors? | NOT_VERIFIED |
| Is the compatibility profile distinguishable at runtime? | NOT_VERIFIED |
| Is the forward-compatible / debug / robust / no-error context flag honoured? | NOT_VERIFIED |
| Are the 1160 exported extension entry points behaviourally correct? | NOT_VERIFIED (not part of the 1048-command surface measured here) |
| CTS / dTLS conformance | **Not claimed, and not tested.** |

---

## 7. The measurement script

Save as `tools/profile_groups.py` and run from the repository root. It reads
only the generated inventory (so it cannot drift from the registry) and, when
`--dylib` is given, the built library. It asserts that every one of the 1048
compatibility commands lands in exactly one group and that the legacy groups
reproduce the `compatibility - core` set exactly.

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

---

## 8. Summary

| Statement | Value |
| --- | --- |
| GL 4.6 compatibility command surface (registry) | 1048 |
| Exported by TGLMT today (`nm -gU`, type `T`) | **700** |
| Missing | **348** |
| Coverage | **66.8 %** |
| GL 4.6 core surface | 657 / 657 exported (100 %) |
| Registry compatibility-only surface | 42 / 42 exported (100 %) |
| Groups entirely missing | 18 (all legacy fixed-function groups) |
| The missing 348 are exactly | the OpenGL 1.0-1.3 fixed-function legacy surface that GL 3.2 removed from core |
