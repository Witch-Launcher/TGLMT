# OpenGL Desktop Version Matrix (1.0 - 4.6, Core + Compatibility)

Generated file -- do not edit by hand.

Regenerate with:

```
python3 tools/gen_version_inventory.py --out data/version_inventory.json
python3 tools/gen_version_inventory.py --docs-from data/version_inventory.json \
        --gl-doc docs/OPENGL_VERSION_MATRIX.md \
        --glsl-doc docs/GLSL_VERSION_MATRIX.md
```

Source of truth: `docs/khronos/gl.xml` (vendored Khronos OpenGL-Registry, sha256 `b9ca2cfa5c676e901c20d34af3407f1687cde0f1336a5ff7a8974d04c7494ad3`).

Every number below is read from `data/version_inventory.json`, which is
produced by `tools/gen_version_inventory.py`. No count in this document is
typed by hand.

This document is a **specification inventory**. It says which entry
points and tokens the Khronos registry defines for each OpenGL version
and profile. It makes **no claim about CTS conformance and no claim
about what this implementation actually does at runtime**.

## Derivation rules

| Rule | Meaning |
| --- | --- |
| `compat_not_core_definition` | present in the compatibility profile but absent from the core profile (includes the fixed-function entry points dropped from core in GL 3.2) |
| `compat_only_definition` | required by a `profile="compatibility"` require block and never a core entry point (this is the definition used by tools/gl46_catalog.py and by the self-check) |
| `compatibility_is_superset_of_core` | True |
| `extensions_are_separate` | True |
| `feature_order` | ascending GL_VERSION_x_y, api=gl only, <= 4.6 |
| `last_write_wins` | True |
| `profile_model_legacy_versions` | GL 1.0 - 3.1 carry no profile attribute in gl.xml and are modelled as a single unsplit surface (core == compatibility) |
| `remove_no_profile` | removed everywhere from that version onward |
| `remove_profile_compatibility` | removed from the compatibility profile only |
| `remove_profile_core` | removed from the core profile only |
| `require_no_profile` | available in every profile |
| `require_profile_compatibility` | compatibility profile only |
| `require_profile_core` | core profile only |
| `within_feature_order` | remove blocks applied before require blocks |

## Self-check

| Check | Value | Expected |
| --- | --- | --- |
| GL 4.6 core commands | 657 | 657 |
| GL 4.6 compatibility-only commands | 42 | 42 |
| Matches `tools/gl46_catalog.parse_core_commands()` | yes | true |
| Matches `tools/gl46_catalog.parse_compat_only_commands()` | yes | true |

For reference, the number of entry points that are in the OpenGL 4.6
**compatibility profile but not the core profile** is **391** -- the 42
compatibility-only entry points plus the 349 fixed-function entry points
that GL 3.2 removed from core and that were never restored. That is a
different question from "which entry points does the registry mark
`profile='compatibility'`", which is what the 42 above counts.

## GL <-> GLSL anchor table

| OpenGL version | GLSL version |
| --- | --- |
| 2.0 | 1.10 |
| 2.1 | 1.20 |
| 3.0 | 1.30 |
| 3.1 | 1.40 |
| 3.2 | 1.50 |
| 3.3 | 3.30 |
| 4.0 | 4.00 |
| 4.1 | 4.10 |
| 4.2 | 4.20 |
| 4.3 | 4.30 |
| 4.4 | 4.40 |
| 4.5 | 4.50 |
| 4.6 | 4.60 |

OpenGL 1.0 - 1.5 have **no** GLSL: shaders are only reachable through
extensions (for example `GL_ARB_shader_objects`, `GL_ARB_vertex_shader`,
`GL_ARB_fragment_shader`) and are outside the version matrix below.

## Per-version summary

`core` = entry points in the OpenGL core profile at that version;
`compat` = entry points in the OpenGL compatibility profile at that
version; `compat-only` = required with `profile="compatibility"` and
never core. `new` = entry points first required by that version.
`rm-core` = entry points removed from the core profile by that version.
`total` = union of the core and compatibility sets.

| GL | model | total | core | compat | compat-only | new | rm-core | core enums | compat enums | GLSL |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 1.0 | legacy | 306 | 306 | 306 | 0 | 306 | 0 | 424 | 424 | -- (none) |
| 1.1 | legacy | 336 | 336 | 336 | 0 | 30 | 0 | 528 | 528 | -- (none) |
| 1.2 | legacy | 340 | 340 | 340 | 0 | 4 | 0 | 569 | 569 | -- (none) |
| 1.3 | legacy | 386 | 386 | 386 | 0 | 46 | 0 | 665 | 665 | -- (none) |
| 1.4 | legacy | 433 | 433 | 433 | 0 | 47 | 0 | 715 | 715 | -- (none) |
| 1.5 | legacy | 452 | 452 | 452 | 0 | 19 | 0 | 765 | 765 | -- (none) |
| 2.0 | legacy | 545 | 545 | 545 | 0 | 93 | 0 | 849 | 849 | 1.10 |
| 2.1 | legacy | 551 | 551 | 551 | 0 | 6 | 0 | 872 | 872 | 1.20 |
| 3.0 | legacy | 635 | 635 | 635 | 0 | 84 | 0 | 1109 | 1109 | 1.30 |
| 3.1 | legacy | 647 | 647 | 647 | 0 | 12 | 0 | 1170 | 1170 | 1.40 |
| 3.2 | split | 666 | 316 | 666 | 0 | 19 | 350 | 802 | 1234 | 1.50 |
| 3.3 | split | 724 | 344 | 724 | 30 | 58 | 0 | 818 | 1250 | 3.30 |
| 4.0 | split | 770 | 390 | 770 | 30 | 46 | 0 | 896 | 1327 | 4.00 |
| 4.1 | split | 858 | 478 | 858 | 30 | 88 | 0 | 931 | 1362 | 4.10 |
| 4.2 | split | 870 | 490 | 870 | 30 | 12 | 0 | 1043 | 1474 | 4.20 |
| 4.3 | split | 913 | 534 | 913 | 30 | 43 | 0 | 1304 | 1733 | 4.30 |
| 4.4 | split | 922 | 543 | 922 | 30 | 9 | 0 | 1323 | 1752 | 4.40 |
| 4.5 | split | 1044 | 653 | 1044 | 42 | 122 | 0 | 1345 | 1786 | 4.50 |
| 4.6 | split | 1048 | 657 | 1048 | 42 | 4 | 0 | 1367 | 1808 | 4.60 |

### Reading the `core` column before GL 3.2

The registry carries no `profile` attribute before `GL_VERSION_3_2`, so
GL 1.0 - 3.1 are a single unsplit surface: their `core` and `compat`
sets are identical by construction. The drop from 647 commands at 3.1 to
316 at 3.2 is the `GL_VERSION_3_2` `<remove profile="core">` blocks
removing the fixed-function API from the core profile -- the
compatibility profile keeps all of it.

## Per-version detail

### OpenGL 1.0 (`GL_VERSION_1_0`, legacy profile model)

- core commands: **306**, compatibility commands: **306**, compatibility-only commands: **0**
- core enums: **424**, compatibility enums: **424**
- first required in this version: **306** commands, **424** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.0 (306)</summary>

- `glAccum`
- `glAlphaFunc`
- `glBegin`
- `glBitmap`
- `glBlendFunc`
- `glCallList`
- `glCallLists`
- `glClear`
- `glClearAccum`
- `glClearColor`
- `glClearDepth`
- `glClearIndex`
- `glClearStencil`
- `glClipPlane`
- `glColor3b`
- `glColor3bv`
- `glColor3d`
- `glColor3dv`
- `glColor3f`
- `glColor3fv`
- `glColor3i`
- `glColor3iv`
- `glColor3s`
- `glColor3sv`
- `glColor3ub`
- `glColor3ubv`
- `glColor3ui`
- `glColor3uiv`
- `glColor3us`
- `glColor3usv`
- `glColor4b`
- `glColor4bv`
- `glColor4d`
- `glColor4dv`
- `glColor4f`
- `glColor4fv`
- `glColor4i`
- `glColor4iv`
- `glColor4s`
- `glColor4sv`
- `glColor4ub`
- `glColor4ubv`
- `glColor4ui`
- `glColor4uiv`
- `glColor4us`
- `glColor4usv`
- `glColorMask`
- `glColorMaterial`
- `glCopyPixels`
- `glCullFace`
- `glDeleteLists`
- `glDepthFunc`
- `glDepthMask`
- `glDepthRange`
- `glDisable`
- `glDrawBuffer`
- `glDrawPixels`
- `glEdgeFlag`
- `glEdgeFlagv`
- `glEnable`
- `glEnd`
- `glEndList`
- `glEvalCoord1d`
- `glEvalCoord1dv`
- `glEvalCoord1f`
- `glEvalCoord1fv`
- `glEvalCoord2d`
- `glEvalCoord2dv`
- `glEvalCoord2f`
- `glEvalCoord2fv`
- `glEvalMesh1`
- `glEvalMesh2`
- `glEvalPoint1`
- `glEvalPoint2`
- `glFeedbackBuffer`
- `glFinish`
- `glFlush`
- `glFogf`
- `glFogfv`
- `glFogi`
- `glFogiv`
- `glFrontFace`
- `glFrustum`
- `glGenLists`
- `glGetBooleanv`
- `glGetClipPlane`
- `glGetDoublev`
- `glGetError`
- `glGetFloatv`
- `glGetIntegerv`
- `glGetLightfv`
- `glGetLightiv`
- `glGetMapdv`
- `glGetMapfv`
- `glGetMapiv`
- `glGetMaterialfv`
- `glGetMaterialiv`
- `glGetPixelMapfv`
- `glGetPixelMapuiv`
- `glGetPixelMapusv`
- `glGetPolygonStipple`
- `glGetString`
- `glGetTexEnvfv`
- `glGetTexEnviv`
- `glGetTexGendv`
- `glGetTexGenfv`
- `glGetTexGeniv`
- `glGetTexImage`
- `glGetTexLevelParameterfv`
- `glGetTexLevelParameteriv`
- `glGetTexParameterfv`
- `glGetTexParameteriv`
- `glHint`
- `glIndexMask`
- `glIndexd`
- `glIndexdv`
- `glIndexf`
- `glIndexfv`
- `glIndexi`
- `glIndexiv`
- `glIndexs`
- `glIndexsv`
- `glInitNames`
- `glIsEnabled`
- `glIsList`
- `glLightModelf`
- `glLightModelfv`
- `glLightModeli`
- `glLightModeliv`
- `glLightf`
- `glLightfv`
- `glLighti`
- `glLightiv`
- `glLineStipple`
- `glLineWidth`
- `glListBase`
- `glLoadIdentity`
- `glLoadMatrixd`
- `glLoadMatrixf`
- `glLoadName`
- `glLogicOp`
- `glMap1d`
- `glMap1f`
- `glMap2d`
- `glMap2f`
- `glMapGrid1d`
- `glMapGrid1f`
- `glMapGrid2d`
- `glMapGrid2f`
- `glMaterialf`
- `glMaterialfv`
- `glMateriali`
- `glMaterialiv`
- `glMatrixMode`
- `glMultMatrixd`
- `glMultMatrixf`
- `glNewList`
- `glNormal3b`
- `glNormal3bv`
- `glNormal3d`
- `glNormal3dv`
- `glNormal3f`
- `glNormal3fv`
- `glNormal3i`
- `glNormal3iv`
- `glNormal3s`
- `glNormal3sv`
- `glOrtho`
- `glPassThrough`
- `glPixelMapfv`
- `glPixelMapuiv`
- `glPixelMapusv`
- `glPixelStoref`
- `glPixelStorei`
- `glPixelTransferf`
- `glPixelTransferi`
- `glPixelZoom`
- `glPointSize`
- `glPolygonMode`
- `glPolygonStipple`
- `glPopAttrib`
- `glPopMatrix`
- `glPopName`
- `glPushAttrib`
- `glPushMatrix`
- `glPushName`
- `glRasterPos2d`
- `glRasterPos2dv`
- `glRasterPos2f`
- `glRasterPos2fv`
- `glRasterPos2i`
- `glRasterPos2iv`
- `glRasterPos2s`
- `glRasterPos2sv`
- `glRasterPos3d`
- `glRasterPos3dv`
- `glRasterPos3f`
- `glRasterPos3fv`
- `glRasterPos3i`
- `glRasterPos3iv`
- `glRasterPos3s`
- `glRasterPos3sv`
- `glRasterPos4d`
- `glRasterPos4dv`
- `glRasterPos4f`
- `glRasterPos4fv`
- `glRasterPos4i`
- `glRasterPos4iv`
- `glRasterPos4s`
- `glRasterPos4sv`
- `glReadBuffer`
- `glReadPixels`
- `glRectd`
- `glRectdv`
- `glRectf`
- `glRectfv`
- `glRecti`
- `glRectiv`
- `glRects`
- `glRectsv`
- `glRenderMode`
- `glRotated`
- `glRotatef`
- `glScaled`
- `glScalef`
- `glScissor`
- `glSelectBuffer`
- `glShadeModel`
- `glStencilFunc`
- `glStencilMask`
- `glStencilOp`
- `glTexCoord1d`
- `glTexCoord1dv`
- `glTexCoord1f`
- `glTexCoord1fv`
- `glTexCoord1i`
- `glTexCoord1iv`
- `glTexCoord1s`
- `glTexCoord1sv`
- `glTexCoord2d`
- `glTexCoord2dv`
- `glTexCoord2f`
- `glTexCoord2fv`
- `glTexCoord2i`
- `glTexCoord2iv`
- `glTexCoord2s`
- `glTexCoord2sv`
- `glTexCoord3d`
- `glTexCoord3dv`
- `glTexCoord3f`
- `glTexCoord3fv`
- `glTexCoord3i`
- `glTexCoord3iv`
- `glTexCoord3s`
- `glTexCoord3sv`
- `glTexCoord4d`
- `glTexCoord4dv`
- `glTexCoord4f`
- `glTexCoord4fv`
- `glTexCoord4i`
- `glTexCoord4iv`
- `glTexCoord4s`
- `glTexCoord4sv`
- `glTexEnvf`
- `glTexEnvfv`
- `glTexEnvi`
- `glTexEnviv`
- `glTexGend`
- `glTexGendv`
- `glTexGenf`
- `glTexGenfv`
- `glTexGeni`
- `glTexGeniv`
- `glTexImage1D`
- `glTexImage2D`
- `glTexParameterf`
- `glTexParameterfv`
- `glTexParameteri`
- `glTexParameteriv`
- `glTranslated`
- `glTranslatef`
- `glVertex2d`
- `glVertex2dv`
- `glVertex2f`
- `glVertex2fv`
- `glVertex2i`
- `glVertex2iv`
- `glVertex2s`
- `glVertex2sv`
- `glVertex3d`
- `glVertex3dv`
- `glVertex3f`
- `glVertex3fv`
- `glVertex3i`
- `glVertex3iv`
- `glVertex3s`
- `glVertex3sv`
- `glVertex4d`
- `glVertex4dv`
- `glVertex4f`
- `glVertex4fv`
- `glVertex4i`
- `glVertex4iv`
- `glVertex4s`
- `glVertex4sv`
- `glViewport`

</details>

### OpenGL 1.1 (`GL_VERSION_1_1`, legacy profile model)

- core commands: **336**, compatibility commands: **336**, compatibility-only commands: **0**
- core enums: **528**, compatibility enums: **528**
- first required in this version: **30** commands, **104** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.1 (30)</summary>

- `glAreTexturesResident`
- `glArrayElement`
- `glBindTexture`
- `glColorPointer`
- `glCopyTexImage1D`
- `glCopyTexImage2D`
- `glCopyTexSubImage1D`
- `glCopyTexSubImage2D`
- `glDeleteTextures`
- `glDisableClientState`
- `glDrawArrays`
- `glDrawElements`
- `glEdgeFlagPointer`
- `glEnableClientState`
- `glGenTextures`
- `glGetPointerv`
- `glIndexPointer`
- `glIndexub`
- `glIndexubv`
- `glInterleavedArrays`
- `glIsTexture`
- `glNormalPointer`
- `glPolygonOffset`
- `glPopClientAttrib`
- `glPrioritizeTextures`
- `glPushClientAttrib`
- `glTexCoordPointer`
- `glTexSubImage1D`
- `glTexSubImage2D`
- `glVertexPointer`

</details>

### OpenGL 1.2 (`GL_VERSION_1_2`, legacy profile model)

- core commands: **340**, compatibility commands: **340**, compatibility-only commands: **0**
- core enums: **569**, compatibility enums: **569**
- first required in this version: **4** commands, **41** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.2 (4)</summary>

- `glCopyTexSubImage3D`
- `glDrawRangeElements`
- `glTexImage3D`
- `glTexSubImage3D`

</details>

### OpenGL 1.3 (`GL_VERSION_1_3`, legacy profile model)

- core commands: **386**, compatibility commands: **386**, compatibility-only commands: **0**
- core enums: **665**, compatibility enums: **665**
- first required in this version: **46** commands, **96** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.3 (46)</summary>

- `glActiveTexture`
- `glClientActiveTexture`
- `glCompressedTexImage1D`
- `glCompressedTexImage2D`
- `glCompressedTexImage3D`
- `glCompressedTexSubImage1D`
- `glCompressedTexSubImage2D`
- `glCompressedTexSubImage3D`
- `glGetCompressedTexImage`
- `glLoadTransposeMatrixd`
- `glLoadTransposeMatrixf`
- `glMultTransposeMatrixd`
- `glMultTransposeMatrixf`
- `glMultiTexCoord1d`
- `glMultiTexCoord1dv`
- `glMultiTexCoord1f`
- `glMultiTexCoord1fv`
- `glMultiTexCoord1i`
- `glMultiTexCoord1iv`
- `glMultiTexCoord1s`
- `glMultiTexCoord1sv`
- `glMultiTexCoord2d`
- `glMultiTexCoord2dv`
- `glMultiTexCoord2f`
- `glMultiTexCoord2fv`
- `glMultiTexCoord2i`
- `glMultiTexCoord2iv`
- `glMultiTexCoord2s`
- `glMultiTexCoord2sv`
- `glMultiTexCoord3d`
- `glMultiTexCoord3dv`
- `glMultiTexCoord3f`
- `glMultiTexCoord3fv`
- `glMultiTexCoord3i`
- `glMultiTexCoord3iv`
- `glMultiTexCoord3s`
- `glMultiTexCoord3sv`
- `glMultiTexCoord4d`
- `glMultiTexCoord4dv`
- `glMultiTexCoord4f`
- `glMultiTexCoord4fv`
- `glMultiTexCoord4i`
- `glMultiTexCoord4iv`
- `glMultiTexCoord4s`
- `glMultiTexCoord4sv`
- `glSampleCoverage`

</details>

### OpenGL 1.4 (`GL_VERSION_1_4`, legacy profile model)

- core commands: **433**, compatibility commands: **433**, compatibility-only commands: **0**
- core enums: **715**, compatibility enums: **715**
- first required in this version: **47** commands, **50** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.4 (47)</summary>

- `glBlendColor`
- `glBlendEquation`
- `glBlendFuncSeparate`
- `glFogCoordPointer`
- `glFogCoordd`
- `glFogCoorddv`
- `glFogCoordf`
- `glFogCoordfv`
- `glMultiDrawArrays`
- `glMultiDrawElements`
- `glPointParameterf`
- `glPointParameterfv`
- `glPointParameteri`
- `glPointParameteriv`
- `glSecondaryColor3b`
- `glSecondaryColor3bv`
- `glSecondaryColor3d`
- `glSecondaryColor3dv`
- `glSecondaryColor3f`
- `glSecondaryColor3fv`
- `glSecondaryColor3i`
- `glSecondaryColor3iv`
- `glSecondaryColor3s`
- `glSecondaryColor3sv`
- `glSecondaryColor3ub`
- `glSecondaryColor3ubv`
- `glSecondaryColor3ui`
- `glSecondaryColor3uiv`
- `glSecondaryColor3us`
- `glSecondaryColor3usv`
- `glSecondaryColorPointer`
- `glWindowPos2d`
- `glWindowPos2dv`
- `glWindowPos2f`
- `glWindowPos2fv`
- `glWindowPos2i`
- `glWindowPos2iv`
- `glWindowPos2s`
- `glWindowPos2sv`
- `glWindowPos3d`
- `glWindowPos3dv`
- `glWindowPos3f`
- `glWindowPos3fv`
- `glWindowPos3i`
- `glWindowPos3iv`
- `glWindowPos3s`
- `glWindowPos3sv`

</details>

### OpenGL 1.5 (`GL_VERSION_1_5`, legacy profile model)

- core commands: **452**, compatibility commands: **452**, compatibility-only commands: **0**
- core enums: **765**, compatibility enums: **765**
- first required in this version: **19** commands, **50** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **none (pre-GLSL; extensions only)**

<details><summary>Commands first required in 1.5 (19)</summary>

- `glBeginQuery`
- `glBindBuffer`
- `glBufferData`
- `glBufferSubData`
- `glDeleteBuffers`
- `glDeleteQueries`
- `glEndQuery`
- `glGenBuffers`
- `glGenQueries`
- `glGetBufferParameteriv`
- `glGetBufferPointerv`
- `glGetBufferSubData`
- `glGetQueryObjectiv`
- `glGetQueryObjectuiv`
- `glGetQueryiv`
- `glIsBuffer`
- `glIsQuery`
- `glMapBuffer`
- `glUnmapBuffer`

</details>

### OpenGL 2.0 (`GL_VERSION_2_0`, legacy profile model)

- core commands: **545**, compatibility commands: **545**, compatibility-only commands: **0**
- core enums: **849**, compatibility enums: **849**
- first required in this version: **93** commands, **84** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **1.10**

<details><summary>Commands first required in 2.0 (93)</summary>

- `glAttachShader`
- `glBindAttribLocation`
- `glBlendEquationSeparate`
- `glCompileShader`
- `glCreateProgram`
- `glCreateShader`
- `glDeleteProgram`
- `glDeleteShader`
- `glDetachShader`
- `glDisableVertexAttribArray`
- `glDrawBuffers`
- `glEnableVertexAttribArray`
- `glGetActiveAttrib`
- `glGetActiveUniform`
- `glGetAttachedShaders`
- `glGetAttribLocation`
- `glGetProgramInfoLog`
- `glGetProgramiv`
- `glGetShaderInfoLog`
- `glGetShaderSource`
- `glGetShaderiv`
- `glGetUniformLocation`
- `glGetUniformfv`
- `glGetUniformiv`
- `glGetVertexAttribPointerv`
- `glGetVertexAttribdv`
- `glGetVertexAttribfv`
- `glGetVertexAttribiv`
- `glIsProgram`
- `glIsShader`
- `glLinkProgram`
- `glShaderSource`
- `glStencilFuncSeparate`
- `glStencilMaskSeparate`
- `glStencilOpSeparate`
- `glUniform1f`
- `glUniform1fv`
- `glUniform1i`
- `glUniform1iv`
- `glUniform2f`
- `glUniform2fv`
- `glUniform2i`
- `glUniform2iv`
- `glUniform3f`
- `glUniform3fv`
- `glUniform3i`
- `glUniform3iv`
- `glUniform4f`
- `glUniform4fv`
- `glUniform4i`
- `glUniform4iv`
- `glUniformMatrix2fv`
- `glUniformMatrix3fv`
- `glUniformMatrix4fv`
- `glUseProgram`
- `glValidateProgram`
- `glVertexAttrib1d`
- `glVertexAttrib1dv`
- `glVertexAttrib1f`
- `glVertexAttrib1fv`
- `glVertexAttrib1s`
- `glVertexAttrib1sv`
- `glVertexAttrib2d`
- `glVertexAttrib2dv`
- `glVertexAttrib2f`
- `glVertexAttrib2fv`
- `glVertexAttrib2s`
- `glVertexAttrib2sv`
- `glVertexAttrib3d`
- `glVertexAttrib3dv`
- `glVertexAttrib3f`
- `glVertexAttrib3fv`
- `glVertexAttrib3s`
- `glVertexAttrib3sv`
- `glVertexAttrib4Nbv`
- `glVertexAttrib4Niv`
- `glVertexAttrib4Nsv`
- `glVertexAttrib4Nub`
- `glVertexAttrib4Nubv`
- `glVertexAttrib4Nuiv`
- `glVertexAttrib4Nusv`
- `glVertexAttrib4bv`
- `glVertexAttrib4d`
- `glVertexAttrib4dv`
- `glVertexAttrib4f`
- `glVertexAttrib4fv`
- `glVertexAttrib4iv`
- `glVertexAttrib4s`
- `glVertexAttrib4sv`
- `glVertexAttrib4ubv`
- `glVertexAttrib4uiv`
- `glVertexAttrib4usv`
- `glVertexAttribPointer`

</details>

### OpenGL 2.1 (`GL_VERSION_2_1`, legacy profile model)

- core commands: **551**, compatibility commands: **551**, compatibility-only commands: **0**
- core enums: **872**, compatibility enums: **872**
- first required in this version: **6** commands, **23** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **1.20**

<details><summary>Commands first required in 2.1 (6)</summary>

- `glUniformMatrix2x3fv`
- `glUniformMatrix2x4fv`
- `glUniformMatrix3x2fv`
- `glUniformMatrix3x4fv`
- `glUniformMatrix4x2fv`
- `glUniformMatrix4x3fv`

</details>

### OpenGL 3.0 (`GL_VERSION_3_0`, legacy profile model)

- core commands: **635**, compatibility commands: **635**, compatibility-only commands: **0**
- core enums: **1109**, compatibility enums: **1109**
- first required in this version: **84** commands, **237** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **1.30**

<details><summary>Commands first required in 3.0 (84)</summary>

- `glBeginConditionalRender`
- `glBeginTransformFeedback`
- `glBindBufferBase`
- `glBindBufferRange`
- `glBindFragDataLocation`
- `glBindFramebuffer`
- `glBindRenderbuffer`
- `glBindVertexArray`
- `glBlitFramebuffer`
- `glCheckFramebufferStatus`
- `glClampColor`
- `glClearBufferfi`
- `glClearBufferfv`
- `glClearBufferiv`
- `glClearBufferuiv`
- `glColorMaski`
- `glDeleteFramebuffers`
- `glDeleteRenderbuffers`
- `glDeleteVertexArrays`
- `glDisablei`
- `glEnablei`
- `glEndConditionalRender`
- `glEndTransformFeedback`
- `glFlushMappedBufferRange`
- `glFramebufferRenderbuffer`
- `glFramebufferTexture1D`
- `glFramebufferTexture2D`
- `glFramebufferTexture3D`
- `glFramebufferTextureLayer`
- `glGenFramebuffers`
- `glGenRenderbuffers`
- `glGenVertexArrays`
- `glGenerateMipmap`
- `glGetBooleani_v`
- `glGetFragDataLocation`
- `glGetFramebufferAttachmentParameteriv`
- `glGetIntegeri_v`
- `glGetRenderbufferParameteriv`
- `glGetStringi`
- `glGetTexParameterIiv`
- `glGetTexParameterIuiv`
- `glGetTransformFeedbackVarying`
- `glGetUniformuiv`
- `glGetVertexAttribIiv`
- `glGetVertexAttribIuiv`
- `glIsEnabledi`
- `glIsFramebuffer`
- `glIsRenderbuffer`
- `glIsVertexArray`
- `glMapBufferRange`
- `glRenderbufferStorage`
- `glRenderbufferStorageMultisample`
- `glTexParameterIiv`
- `glTexParameterIuiv`
- `glTransformFeedbackVaryings`
- `glUniform1ui`
- `glUniform1uiv`
- `glUniform2ui`
- `glUniform2uiv`
- `glUniform3ui`
- `glUniform3uiv`
- `glUniform4ui`
- `glUniform4uiv`
- `glVertexAttribI1i`
- `glVertexAttribI1iv`
- `glVertexAttribI1ui`
- `glVertexAttribI1uiv`
- `glVertexAttribI2i`
- `glVertexAttribI2iv`
- `glVertexAttribI2ui`
- `glVertexAttribI2uiv`
- `glVertexAttribI3i`
- `glVertexAttribI3iv`
- `glVertexAttribI3ui`
- `glVertexAttribI3uiv`
- `glVertexAttribI4bv`
- `glVertexAttribI4i`
- `glVertexAttribI4iv`
- `glVertexAttribI4sv`
- `glVertexAttribI4ubv`
- `glVertexAttribI4ui`
- `glVertexAttribI4uiv`
- `glVertexAttribI4usv`
- `glVertexAttribIPointer`

</details>

### OpenGL 3.1 (`GL_VERSION_3_1`, legacy profile model)

- core commands: **647**, compatibility commands: **647**, compatibility-only commands: **0**
- core enums: **1170**, compatibility enums: **1170**
- first required in this version: **12** commands, **61** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **1.40**

<details><summary>Commands first required in 3.1 (12)</summary>

- `glCopyBufferSubData`
- `glDrawArraysInstanced`
- `glDrawElementsInstanced`
- `glGetActiveUniformBlockName`
- `glGetActiveUniformBlockiv`
- `glGetActiveUniformName`
- `glGetActiveUniformsiv`
- `glGetUniformBlockIndex`
- `glGetUniformIndices`
- `glPrimitiveRestartIndex`
- `glTexBuffer`
- `glUniformBlockBinding`

</details>

### OpenGL 3.2 (`GL_VERSION_3_2`, split profile model)

- core commands: **316**, compatibility commands: **666**, compatibility-only commands: **0**
- core enums: **802**, compatibility enums: **1234**
- first required in this version: **19** commands, **64** enums
- removed from the core profile in this version: **350** commands, **432** enums
- GLSL that goes with it: **1.50**

<details><summary>Commands first required in 3.2 (19)</summary>

- `glClientWaitSync`
- `glDeleteSync`
- `glDrawElementsBaseVertex`
- `glDrawElementsInstancedBaseVertex`
- `glDrawRangeElementsBaseVertex`
- `glFenceSync`
- `glFramebufferTexture`
- `glGetBufferParameteri64v`
- `glGetInteger64i_v`
- `glGetInteger64v`
- `glGetMultisamplefv`
- `glGetSynciv`
- `glIsSync`
- `glMultiDrawElementsBaseVertex`
- `glProvokingVertex`
- `glSampleMaski`
- `glTexImage2DMultisample`
- `glTexImage3DMultisample`
- `glWaitSync`

</details>

<details><summary>Commands removed from the core profile in 3.2 (350)</summary>

- `glAccum`
- `glAlphaFunc`
- `glAreTexturesResident`
- `glArrayElement`
- `glBegin`
- `glBitmap`
- `glCallList`
- `glCallLists`
- `glClearAccum`
- `glClearIndex`
- `glClientActiveTexture`
- `glClipPlane`
- `glColor3b`
- `glColor3bv`
- `glColor3d`
- `glColor3dv`
- `glColor3f`
- `glColor3fv`
- `glColor3i`
- `glColor3iv`
- `glColor3s`
- `glColor3sv`
- `glColor3ub`
- `glColor3ubv`
- `glColor3ui`
- `glColor3uiv`
- `glColor3us`
- `glColor3usv`
- `glColor4b`
- `glColor4bv`
- `glColor4d`
- `glColor4dv`
- `glColor4f`
- `glColor4fv`
- `glColor4i`
- `glColor4iv`
- `glColor4s`
- `glColor4sv`
- `glColor4ub`
- `glColor4ubv`
- `glColor4ui`
- `glColor4uiv`
- `glColor4us`
- `glColor4usv`
- `glColorMaterial`
- `glColorPointer`
- `glCopyPixels`
- `glDeleteLists`
- `glDisableClientState`
- `glDrawPixels`
- `glEdgeFlag`
- `glEdgeFlagPointer`
- `glEdgeFlagv`
- `glEnableClientState`
- `glEnd`
- `glEndList`
- `glEvalCoord1d`
- `glEvalCoord1dv`
- `glEvalCoord1f`
- `glEvalCoord1fv`
- `glEvalCoord2d`
- `glEvalCoord2dv`
- `glEvalCoord2f`
- `glEvalCoord2fv`
- `glEvalMesh1`
- `glEvalMesh2`
- `glEvalPoint1`
- `glEvalPoint2`
- `glFeedbackBuffer`
- `glFogCoordPointer`
- `glFogCoordd`
- `glFogCoorddv`
- `glFogCoordf`
- `glFogCoordfv`
- `glFogf`
- `glFogfv`
- `glFogi`
- `glFogiv`
- `glFrustum`
- `glGenLists`
- `glGetClipPlane`
- `glGetLightfv`
- `glGetLightiv`
- `glGetMapdv`
- `glGetMapfv`
- `glGetMapiv`
- `glGetMaterialfv`
- `glGetMaterialiv`
- `glGetPixelMapfv`
- `glGetPixelMapuiv`
- `glGetPixelMapusv`
- `glGetPointerv`
- `glGetPolygonStipple`
- `glGetTexEnvfv`
- `glGetTexEnviv`
- `glGetTexGendv`
- `glGetTexGenfv`
- `glGetTexGeniv`
- `glIndexMask`
- `glIndexPointer`
- `glIndexd`
- `glIndexdv`
- `glIndexf`
- `glIndexfv`
- `glIndexi`
- `glIndexiv`
- `glIndexs`
- `glIndexsv`
- `glIndexub`
- `glIndexubv`
- `glInitNames`
- `glInterleavedArrays`
- `glIsList`
- `glLightModelf`
- `glLightModelfv`
- `glLightModeli`
- `glLightModeliv`
- `glLightf`
- `glLightfv`
- `glLighti`
- `glLightiv`
- `glLineStipple`
- `glListBase`
- `glLoadIdentity`
- `glLoadMatrixd`
- `glLoadMatrixf`
- `glLoadName`
- `glLoadTransposeMatrixd`
- `glLoadTransposeMatrixf`
- `glMap1d`
- `glMap1f`
- `glMap2d`
- `glMap2f`
- `glMapGrid1d`
- `glMapGrid1f`
- `glMapGrid2d`
- `glMapGrid2f`
- `glMaterialf`
- `glMaterialfv`
- `glMateriali`
- `glMaterialiv`
- `glMatrixMode`
- `glMultMatrixd`
- `glMultMatrixf`
- `glMultTransposeMatrixd`
- `glMultTransposeMatrixf`
- `glMultiTexCoord1d`
- `glMultiTexCoord1dv`
- `glMultiTexCoord1f`
- `glMultiTexCoord1fv`
- `glMultiTexCoord1i`
- `glMultiTexCoord1iv`
- `glMultiTexCoord1s`
- `glMultiTexCoord1sv`
- `glMultiTexCoord2d`
- `glMultiTexCoord2dv`
- `glMultiTexCoord2f`
- `glMultiTexCoord2fv`
- `glMultiTexCoord2i`
- `glMultiTexCoord2iv`
- `glMultiTexCoord2s`
- `glMultiTexCoord2sv`
- `glMultiTexCoord3d`
- `glMultiTexCoord3dv`
- `glMultiTexCoord3f`
- `glMultiTexCoord3fv`
- `glMultiTexCoord3i`
- `glMultiTexCoord3iv`
- `glMultiTexCoord3s`
- `glMultiTexCoord3sv`
- `glMultiTexCoord4d`
- `glMultiTexCoord4dv`
- `glMultiTexCoord4f`
- `glMultiTexCoord4fv`
- `glMultiTexCoord4i`
- `glMultiTexCoord4iv`
- `glMultiTexCoord4s`
- `glMultiTexCoord4sv`
- `glNewList`
- `glNormal3b`
- `glNormal3bv`
- `glNormal3d`
- `glNormal3dv`
- `glNormal3f`
- `glNormal3fv`
- `glNormal3i`
- `glNormal3iv`
- `glNormal3s`
- `glNormal3sv`
- `glNormalPointer`
- `glOrtho`
- `glPassThrough`
- `glPixelMapfv`
- `glPixelMapuiv`
- `glPixelMapusv`
- `glPixelTransferf`
- `glPixelTransferi`
- `glPixelZoom`
- `glPolygonStipple`
- `glPopAttrib`
- `glPopClientAttrib`
- `glPopMatrix`
- `glPopName`
- `glPrioritizeTextures`
- `glPushAttrib`
- `glPushClientAttrib`
- `glPushMatrix`
- `glPushName`
- `glRasterPos2d`
- `glRasterPos2dv`
- `glRasterPos2f`
- `glRasterPos2fv`
- `glRasterPos2i`
- `glRasterPos2iv`
- `glRasterPos2s`
- `glRasterPos2sv`
- `glRasterPos3d`
- `glRasterPos3dv`
- `glRasterPos3f`
- `glRasterPos3fv`
- `glRasterPos3i`
- `glRasterPos3iv`
- `glRasterPos3s`
- `glRasterPos3sv`
- `glRasterPos4d`
- `glRasterPos4dv`
- `glRasterPos4f`
- `glRasterPos4fv`
- `glRasterPos4i`
- `glRasterPos4iv`
- `glRasterPos4s`
- `glRasterPos4sv`
- `glRectd`
- `glRectdv`
- `glRectf`
- `glRectfv`
- `glRecti`
- `glRectiv`
- `glRects`
- `glRectsv`
- `glRenderMode`
- `glRotated`
- `glRotatef`
- `glScaled`
- `glScalef`
- `glSecondaryColor3b`
- `glSecondaryColor3bv`
- `glSecondaryColor3d`
- `glSecondaryColor3dv`
- `glSecondaryColor3f`
- `glSecondaryColor3fv`
- `glSecondaryColor3i`
- `glSecondaryColor3iv`
- `glSecondaryColor3s`
- `glSecondaryColor3sv`
- `glSecondaryColor3ub`
- `glSecondaryColor3ubv`
- `glSecondaryColor3ui`
- `glSecondaryColor3uiv`
- `glSecondaryColor3us`
- `glSecondaryColor3usv`
- `glSecondaryColorPointer`
- `glSelectBuffer`
- `glShadeModel`
- `glTexCoord1d`
- `glTexCoord1dv`
- `glTexCoord1f`
- `glTexCoord1fv`
- `glTexCoord1i`
- `glTexCoord1iv`
- `glTexCoord1s`
- `glTexCoord1sv`
- `glTexCoord2d`
- `glTexCoord2dv`
- `glTexCoord2f`
- `glTexCoord2fv`
- `glTexCoord2i`
- `glTexCoord2iv`
- `glTexCoord2s`
- `glTexCoord2sv`
- `glTexCoord3d`
- `glTexCoord3dv`
- `glTexCoord3f`
- `glTexCoord3fv`
- `glTexCoord3i`
- `glTexCoord3iv`
- `glTexCoord3s`
- `glTexCoord3sv`
- `glTexCoord4d`
- `glTexCoord4dv`
- `glTexCoord4f`
- `glTexCoord4fv`
- `glTexCoord4i`
- `glTexCoord4iv`
- `glTexCoord4s`
- `glTexCoord4sv`
- `glTexCoordPointer`
- `glTexEnvf`
- `glTexEnvfv`
- `glTexEnvi`
- `glTexEnviv`
- `glTexGend`
- `glTexGendv`
- `glTexGenf`
- `glTexGenfv`
- `glTexGeni`
- `glTexGeniv`
- `glTranslated`
- `glTranslatef`
- `glVertex2d`
- `glVertex2dv`
- `glVertex2f`
- `glVertex2fv`
- `glVertex2i`
- `glVertex2iv`
- `glVertex2s`
- `glVertex2sv`
- `glVertex3d`
- `glVertex3dv`
- `glVertex3f`
- `glVertex3fv`
- `glVertex3i`
- `glVertex3iv`
- `glVertex3s`
- `glVertex3sv`
- `glVertex4d`
- `glVertex4dv`
- `glVertex4f`
- `glVertex4fv`
- `glVertex4i`
- `glVertex4iv`
- `glVertex4s`
- `glVertex4sv`
- `glVertexPointer`
- `glWindowPos2d`
- `glWindowPos2dv`
- `glWindowPos2f`
- `glWindowPos2fv`
- `glWindowPos2i`
- `glWindowPos2iv`
- `glWindowPos2s`
- `glWindowPos2sv`
- `glWindowPos3d`
- `glWindowPos3dv`
- `glWindowPos3f`
- `glWindowPos3fv`
- `glWindowPos3i`
- `glWindowPos3iv`
- `glWindowPos3s`
- `glWindowPos3sv`

</details>

### OpenGL 3.3 (`GL_VERSION_3_3`, split profile model)

- core commands: **344**, compatibility commands: **724**, compatibility-only commands: **30**
- core enums: **818**, compatibility enums: **1250**
- first required in this version: **58** commands, **16** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **3.30**

<details><summary>Commands first required in 3.3 (58)</summary>

- `glBindFragDataLocationIndexed`
- `glBindSampler`
- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glDeleteSamplers`
- `glGenSamplers`
- `glGetFragDataIndex`
- `glGetQueryObjecti64v`
- `glGetQueryObjectui64v`
- `glGetSamplerParameterIiv`
- `glGetSamplerParameterIuiv`
- `glGetSamplerParameterfv`
- `glGetSamplerParameteriv`
- `glIsSampler`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glQueryCounter`
- `glSamplerParameterIiv`
- `glSamplerParameterIuiv`
- `glSamplerParameterf`
- `glSamplerParameterfv`
- `glSamplerParameteri`
- `glSamplerParameteriv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexAttribDivisor`
- `glVertexAttribP1ui`
- `glVertexAttribP1uiv`
- `glVertexAttribP2ui`
- `glVertexAttribP2uiv`
- `glVertexAttribP3ui`
- `glVertexAttribP3uiv`
- `glVertexAttribP4ui`
- `glVertexAttribP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

<details><summary>Compatibility-only commands at 3.3 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.0 (`GL_VERSION_4_0`, split profile model)

- core commands: **390**, compatibility commands: **770**, compatibility-only commands: **30**
- core enums: **896**, compatibility enums: **1327**
- first required in this version: **46** commands, **77** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.00**

<details><summary>Commands first required in 4.0 (46)</summary>

- `glBeginQueryIndexed`
- `glBindTransformFeedback`
- `glBlendEquationSeparatei`
- `glBlendEquationi`
- `glBlendFuncSeparatei`
- `glBlendFunci`
- `glDeleteTransformFeedbacks`
- `glDrawArraysIndirect`
- `glDrawElementsIndirect`
- `glDrawTransformFeedback`
- `glDrawTransformFeedbackStream`
- `glEndQueryIndexed`
- `glGenTransformFeedbacks`
- `glGetActiveSubroutineName`
- `glGetActiveSubroutineUniformName`
- `glGetActiveSubroutineUniformiv`
- `glGetProgramStageiv`
- `glGetQueryIndexediv`
- `glGetSubroutineIndex`
- `glGetSubroutineUniformLocation`
- `glGetUniformSubroutineuiv`
- `glGetUniformdv`
- `glIsTransformFeedback`
- `glMinSampleShading`
- `glPatchParameterfv`
- `glPatchParameteri`
- `glPauseTransformFeedback`
- `glResumeTransformFeedback`
- `glUniform1d`
- `glUniform1dv`
- `glUniform2d`
- `glUniform2dv`
- `glUniform3d`
- `glUniform3dv`
- `glUniform4d`
- `glUniform4dv`
- `glUniformMatrix2dv`
- `glUniformMatrix2x3dv`
- `glUniformMatrix2x4dv`
- `glUniformMatrix3dv`
- `glUniformMatrix3x2dv`
- `glUniformMatrix3x4dv`
- `glUniformMatrix4dv`
- `glUniformMatrix4x2dv`
- `glUniformMatrix4x3dv`
- `glUniformSubroutinesuiv`

</details>

<details><summary>Compatibility-only commands at 4.0 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.1 (`GL_VERSION_4_1`, split profile model)

- core commands: **478**, compatibility commands: **858**, compatibility-only commands: **30**
- core enums: **931**, compatibility enums: **1362**
- first required in this version: **88** commands, **35** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.10**

<details><summary>Commands first required in 4.1 (88)</summary>

- `glActiveShaderProgram`
- `glBindProgramPipeline`
- `glClearDepthf`
- `glCreateShaderProgramv`
- `glDeleteProgramPipelines`
- `glDepthRangeArrayv`
- `glDepthRangeIndexed`
- `glDepthRangef`
- `glGenProgramPipelines`
- `glGetDoublei_v`
- `glGetFloati_v`
- `glGetProgramBinary`
- `glGetProgramPipelineInfoLog`
- `glGetProgramPipelineiv`
- `glGetShaderPrecisionFormat`
- `glGetVertexAttribLdv`
- `glIsProgramPipeline`
- `glProgramBinary`
- `glProgramParameteri`
- `glProgramUniform1d`
- `glProgramUniform1dv`
- `glProgramUniform1f`
- `glProgramUniform1fv`
- `glProgramUniform1i`
- `glProgramUniform1iv`
- `glProgramUniform1ui`
- `glProgramUniform1uiv`
- `glProgramUniform2d`
- `glProgramUniform2dv`
- `glProgramUniform2f`
- `glProgramUniform2fv`
- `glProgramUniform2i`
- `glProgramUniform2iv`
- `glProgramUniform2ui`
- `glProgramUniform2uiv`
- `glProgramUniform3d`
- `glProgramUniform3dv`
- `glProgramUniform3f`
- `glProgramUniform3fv`
- `glProgramUniform3i`
- `glProgramUniform3iv`
- `glProgramUniform3ui`
- `glProgramUniform3uiv`
- `glProgramUniform4d`
- `glProgramUniform4dv`
- `glProgramUniform4f`
- `glProgramUniform4fv`
- `glProgramUniform4i`
- `glProgramUniform4iv`
- `glProgramUniform4ui`
- `glProgramUniform4uiv`
- `glProgramUniformMatrix2dv`
- `glProgramUniformMatrix2fv`
- `glProgramUniformMatrix2x3dv`
- `glProgramUniformMatrix2x3fv`
- `glProgramUniformMatrix2x4dv`
- `glProgramUniformMatrix2x4fv`
- `glProgramUniformMatrix3dv`
- `glProgramUniformMatrix3fv`
- `glProgramUniformMatrix3x2dv`
- `glProgramUniformMatrix3x2fv`
- `glProgramUniformMatrix3x4dv`
- `glProgramUniformMatrix3x4fv`
- `glProgramUniformMatrix4dv`
- `glProgramUniformMatrix4fv`
- `glProgramUniformMatrix4x2dv`
- `glProgramUniformMatrix4x2fv`
- `glProgramUniformMatrix4x3dv`
- `glProgramUniformMatrix4x3fv`
- `glReleaseShaderCompiler`
- `glScissorArrayv`
- `glScissorIndexed`
- `glScissorIndexedv`
- `glShaderBinary`
- `glUseProgramStages`
- `glValidateProgramPipeline`
- `glVertexAttribL1d`
- `glVertexAttribL1dv`
- `glVertexAttribL2d`
- `glVertexAttribL2dv`
- `glVertexAttribL3d`
- `glVertexAttribL3dv`
- `glVertexAttribL4d`
- `glVertexAttribL4dv`
- `glVertexAttribLPointer`
- `glViewportArrayv`
- `glViewportIndexedf`
- `glViewportIndexedfv`

</details>

<details><summary>Compatibility-only commands at 4.1 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.2 (`GL_VERSION_4_2`, split profile model)

- core commands: **490**, compatibility commands: **870**, compatibility-only commands: **30**
- core enums: **1043**, compatibility enums: **1474**
- first required in this version: **12** commands, **112** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.20**

<details><summary>Commands first required in 4.2 (12)</summary>

- `glBindImageTexture`
- `glDrawArraysInstancedBaseInstance`
- `glDrawElementsInstancedBaseInstance`
- `glDrawElementsInstancedBaseVertexBaseInstance`
- `glDrawTransformFeedbackInstanced`
- `glDrawTransformFeedbackStreamInstanced`
- `glGetActiveAtomicCounterBufferiv`
- `glGetInternalformativ`
- `glMemoryBarrier`
- `glTexStorage1D`
- `glTexStorage2D`
- `glTexStorage3D`

</details>

<details><summary>Compatibility-only commands at 4.2 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.3 (`GL_VERSION_4_3`, split profile model)

- core commands: **534**, compatibility commands: **913**, compatibility-only commands: **30**
- core enums: **1304**, compatibility enums: **1733**
- first required in this version: **43** commands, **259** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.30**

<details><summary>Commands first required in 4.3 (43)</summary>

- `glBindVertexBuffer`
- `glClearBufferData`
- `glClearBufferSubData`
- `glCopyImageSubData`
- `glDebugMessageCallback`
- `glDebugMessageControl`
- `glDebugMessageInsert`
- `glDispatchCompute`
- `glDispatchComputeIndirect`
- `glFramebufferParameteri`
- `glGetDebugMessageLog`
- `glGetFramebufferParameteriv`
- `glGetInternalformati64v`
- `glGetObjectLabel`
- `glGetObjectPtrLabel`
- `glGetProgramInterfaceiv`
- `glGetProgramResourceIndex`
- `glGetProgramResourceLocation`
- `glGetProgramResourceLocationIndex`
- `glGetProgramResourceName`
- `glGetProgramResourceiv`
- `glInvalidateBufferData`
- `glInvalidateBufferSubData`
- `glInvalidateFramebuffer`
- `glInvalidateSubFramebuffer`
- `glInvalidateTexImage`
- `glInvalidateTexSubImage`
- `glMultiDrawArraysIndirect`
- `glMultiDrawElementsIndirect`
- `glObjectLabel`
- `glObjectPtrLabel`
- `glPopDebugGroup`
- `glPushDebugGroup`
- `glShaderStorageBlockBinding`
- `glTexBufferRange`
- `glTexStorage2DMultisample`
- `glTexStorage3DMultisample`
- `glTextureView`
- `glVertexAttribBinding`
- `glVertexAttribFormat`
- `glVertexAttribIFormat`
- `glVertexAttribLFormat`
- `glVertexBindingDivisor`

</details>

<details><summary>Compatibility-only commands at 4.3 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.4 (`GL_VERSION_4_4`, split profile model)

- core commands: **543**, compatibility commands: **922**, compatibility-only commands: **30**
- core enums: **1323**, compatibility enums: **1752**
- first required in this version: **9** commands, **19** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.40**

<details><summary>Commands first required in 4.4 (9)</summary>

- `glBindBuffersBase`
- `glBindBuffersRange`
- `glBindImageTextures`
- `glBindSamplers`
- `glBindTextures`
- `glBindVertexBuffers`
- `glBufferStorage`
- `glClearTexImage`
- `glClearTexSubImage`

</details>

<details><summary>Compatibility-only commands at 4.4 (30)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.5 (`GL_VERSION_4_5`, split profile model)

- core commands: **653**, compatibility commands: **1044**, compatibility-only commands: **42**
- core enums: **1345**, compatibility enums: **1786**
- first required in this version: **122** commands, **34** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.50**

<details><summary>Commands first required in 4.5 (122)</summary>

- `glBindTextureUnit`
- `glBlitNamedFramebuffer`
- `glCheckNamedFramebufferStatus`
- `glClearNamedBufferData`
- `glClearNamedBufferSubData`
- `glClearNamedFramebufferfi`
- `glClearNamedFramebufferfv`
- `glClearNamedFramebufferiv`
- `glClearNamedFramebufferuiv`
- `glClipControl`
- `glCompressedTextureSubImage1D`
- `glCompressedTextureSubImage2D`
- `glCompressedTextureSubImage3D`
- `glCopyNamedBufferSubData`
- `glCopyTextureSubImage1D`
- `glCopyTextureSubImage2D`
- `glCopyTextureSubImage3D`
- `glCreateBuffers`
- `glCreateFramebuffers`
- `glCreateProgramPipelines`
- `glCreateQueries`
- `glCreateRenderbuffers`
- `glCreateSamplers`
- `glCreateTextures`
- `glCreateTransformFeedbacks`
- `glCreateVertexArrays`
- `glDisableVertexArrayAttrib`
- `glEnableVertexArrayAttrib`
- `glFlushMappedNamedBufferRange`
- `glGenerateTextureMipmap`
- `glGetCompressedTextureImage`
- `glGetCompressedTextureSubImage`
- `glGetGraphicsResetStatus`
- `glGetNamedBufferParameteri64v`
- `glGetNamedBufferParameteriv`
- `glGetNamedBufferPointerv`
- `glGetNamedBufferSubData`
- `glGetNamedFramebufferAttachmentParameteriv`
- `glGetNamedFramebufferParameteriv`
- `glGetNamedRenderbufferParameteriv`
- `glGetQueryBufferObjecti64v`
- `glGetQueryBufferObjectiv`
- `glGetQueryBufferObjectui64v`
- `glGetQueryBufferObjectuiv`
- `glGetTextureImage`
- `glGetTextureLevelParameterfv`
- `glGetTextureLevelParameteriv`
- `glGetTextureParameterIiv`
- `glGetTextureParameterIuiv`
- `glGetTextureParameterfv`
- `glGetTextureParameteriv`
- `glGetTextureSubImage`
- `glGetTransformFeedbacki64_v`
- `glGetTransformFeedbacki_v`
- `glGetTransformFeedbackiv`
- `glGetVertexArrayIndexed64iv`
- `glGetVertexArrayIndexediv`
- `glGetVertexArrayiv`
- `glGetnColorTable`
- `glGetnCompressedTexImage`
- `glGetnConvolutionFilter`
- `glGetnHistogram`
- `glGetnMapdv`
- `glGetnMapfv`
- `glGetnMapiv`
- `glGetnMinmax`
- `glGetnPixelMapfv`
- `glGetnPixelMapuiv`
- `glGetnPixelMapusv`
- `glGetnPolygonStipple`
- `glGetnSeparableFilter`
- `glGetnTexImage`
- `glGetnUniformdv`
- `glGetnUniformfv`
- `glGetnUniformiv`
- `glGetnUniformuiv`
- `glInvalidateNamedFramebufferData`
- `glInvalidateNamedFramebufferSubData`
- `glMapNamedBuffer`
- `glMapNamedBufferRange`
- `glMemoryBarrierByRegion`
- `glNamedBufferData`
- `glNamedBufferStorage`
- `glNamedBufferSubData`
- `glNamedFramebufferDrawBuffer`
- `glNamedFramebufferDrawBuffers`
- `glNamedFramebufferParameteri`
- `glNamedFramebufferReadBuffer`
- `glNamedFramebufferRenderbuffer`
- `glNamedFramebufferTexture`
- `glNamedFramebufferTextureLayer`
- `glNamedRenderbufferStorage`
- `glNamedRenderbufferStorageMultisample`
- `glReadnPixels`
- `glTextureBarrier`
- `glTextureBuffer`
- `glTextureBufferRange`
- `glTextureParameterIiv`
- `glTextureParameterIuiv`
- `glTextureParameterf`
- `glTextureParameterfv`
- `glTextureParameteri`
- `glTextureParameteriv`
- `glTextureStorage1D`
- `glTextureStorage2D`
- `glTextureStorage2DMultisample`
- `glTextureStorage3D`
- `glTextureStorage3DMultisample`
- `glTextureSubImage1D`
- `glTextureSubImage2D`
- `glTextureSubImage3D`
- `glTransformFeedbackBufferBase`
- `glTransformFeedbackBufferRange`
- `glUnmapNamedBuffer`
- `glVertexArrayAttribBinding`
- `glVertexArrayAttribFormat`
- `glVertexArrayAttribIFormat`
- `glVertexArrayAttribLFormat`
- `glVertexArrayBindingDivisor`
- `glVertexArrayElementBuffer`
- `glVertexArrayVertexBuffer`
- `glVertexArrayVertexBuffers`

</details>

<details><summary>Compatibility-only commands at 4.5 (42)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glGetnColorTable`
- `glGetnConvolutionFilter`
- `glGetnHistogram`
- `glGetnMapdv`
- `glGetnMapfv`
- `glGetnMapiv`
- `glGetnMinmax`
- `glGetnPixelMapfv`
- `glGetnPixelMapuiv`
- `glGetnPixelMapusv`
- `glGetnPolygonStipple`
- `glGetnSeparableFilter`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

### OpenGL 4.6 (`GL_VERSION_4_6`, split profile model)

- core commands: **657**, compatibility commands: **1048**, compatibility-only commands: **42**
- core enums: **1367**, compatibility enums: **1808**
- first required in this version: **4** commands, **22** enums
- removed from the core profile in this version: **0** commands, **0** enums
- GLSL that goes with it: **4.60**

<details><summary>Commands first required in 4.6 (4)</summary>

- `glMultiDrawArraysIndirectCount`
- `glMultiDrawElementsIndirectCount`
- `glPolygonOffsetClamp`
- `glSpecializeShader`

</details>

<details><summary>Compatibility-only commands at 4.6 (42)</summary>

- `glColorP3ui`
- `glColorP3uiv`
- `glColorP4ui`
- `glColorP4uiv`
- `glGetnColorTable`
- `glGetnConvolutionFilter`
- `glGetnHistogram`
- `glGetnMapdv`
- `glGetnMapfv`
- `glGetnMapiv`
- `glGetnMinmax`
- `glGetnPixelMapfv`
- `glGetnPixelMapuiv`
- `glGetnPixelMapusv`
- `glGetnPolygonStipple`
- `glGetnSeparableFilter`
- `glMultiTexCoordP1ui`
- `glMultiTexCoordP1uiv`
- `glMultiTexCoordP2ui`
- `glMultiTexCoordP2uiv`
- `glMultiTexCoordP3ui`
- `glMultiTexCoordP3uiv`
- `glMultiTexCoordP4ui`
- `glMultiTexCoordP4uiv`
- `glNormalP3ui`
- `glNormalP3uiv`
- `glSecondaryColorP3ui`
- `glSecondaryColorP3uiv`
- `glTexCoordP1ui`
- `glTexCoordP1uiv`
- `glTexCoordP2ui`
- `glTexCoordP2uiv`
- `glTexCoordP3ui`
- `glTexCoordP3uiv`
- `glTexCoordP4ui`
- `glTexCoordP4uiv`
- `glVertexP2ui`
- `glVertexP2uiv`
- `glVertexP3ui`
- `glVertexP3uiv`
- `glVertexP4ui`
- `glVertexP4uiv`

</details>

## Extensions

Extension commands and enums are **not** included in any count above.
They live in the separate `extensions` section of
`data/version_inventory.json` (863 extensions).

| Extension | desktop GL | commands | enums | fully core in |
| --- | --- | ---: | ---: | --- |
| `GL_3DFX_multisample` | yes | 0 | 4 | -- |
| `GL_3DFX_tbuffer` | yes | 1 | 0 | -- |
| `GL_3DFX_texture_compression_FXT1` | yes | 0 | 2 | -- |
| `GL_AMD_blend_minmax_factor` | yes | 0 | 2 | -- |
| `GL_AMD_compressed_3DC_texture` | no | 0 | 2 | -- |
| `GL_AMD_compressed_ATC_texture` | no | 0 | 3 | -- |
| `GL_AMD_conservative_depth` | yes | 0 | 0 | -- |
| `GL_AMD_debug_output` | yes | 4 | 14 | -- |
| `GL_AMD_depth_clamp_separate` | yes | 0 | 2 | -- |
| `GL_AMD_draw_buffers_blend` | yes | 4 | 0 | -- |
| `GL_AMD_framebuffer_multisample_advanced` | yes | 2 | 6 | -- |
| `GL_AMD_framebuffer_sample_positions` | yes | 4 | 4 | -- |
| `GL_AMD_gcn_shader` | yes | 0 | 0 | -- |
| `GL_AMD_gpu_shader_half_float` | yes | 0 | 13 | -- |
| `GL_AMD_gpu_shader_int16` | yes | 0 | 0 | -- |
| `GL_AMD_gpu_shader_int64` | yes | 34 | 28 | -- |
| `GL_AMD_interleaved_elements` | yes | 1 | 9 | -- |
| `GL_AMD_multi_draw_indirect` | yes | 2 | 0 | -- |
| `GL_AMD_name_gen_delete` | yes | 3 | 5 | -- |
| `GL_AMD_occlusion_query_event` | yes | 1 | 6 | -- |
| `GL_AMD_performance_monitor` | yes | 11 | 7 | -- |
| `GL_AMD_pinned_memory` | yes | 0 | 1 | -- |
| `GL_AMD_program_binary_Z400` | no | 0 | 1 | -- |
| `GL_AMD_query_buffer_object` | yes | 0 | 3 | -- |
| `GL_AMD_sample_positions` | yes | 1 | 1 | -- |
| `GL_AMD_seamless_cubemap_per_texture` | yes | 0 | 1 | -- |
| `GL_AMD_shader_atomic_counter_ops` | yes | 0 | 0 | -- |
| `GL_AMD_shader_ballot` | yes | 0 | 0 | -- |
| `GL_AMD_shader_explicit_vertex_parameter` | yes | 0 | 0 | -- |
| `GL_AMD_shader_gpu_shader_half_float_fetch` | yes | 0 | 0 | -- |
| `GL_AMD_shader_image_load_store_lod` | yes | 0 | 0 | -- |
| `GL_AMD_shader_stencil_export` | yes | 0 | 0 | -- |
| `GL_AMD_shader_trinary_minmax` | yes | 0 | 0 | -- |
| `GL_AMD_sparse_texture` | yes | 2 | 9 | -- |
| `GL_AMD_stencil_operation_extended` | yes | 1 | 4 | -- |
| `GL_AMD_texture_gather_bias_lod` | yes | 0 | 0 | -- |
| `GL_AMD_texture_texture4` | yes | 0 | 0 | -- |
| `GL_AMD_transform_feedback3_lines_triangles` | yes | 0 | 0 | -- |
| `GL_AMD_transform_feedback4` | yes | 0 | 1 | -- |
| `GL_AMD_vertex_shader_layer` | yes | 0 | 0 | -- |
| `GL_AMD_vertex_shader_tessellator` | yes | 2 | 7 | -- |
| `GL_AMD_vertex_shader_viewport_index` | yes | 0 | 0 | -- |
| `GL_ANDROID_extension_pack_es31a` | no | 0 | 0 | -- |
| `GL_ANGLE_depth_texture` | no | 0 | 8 | -- |
| `GL_ANGLE_framebuffer_blit` | no | 1 | 4 | -- |
| `GL_ANGLE_framebuffer_multisample` | no | 1 | 3 | -- |
| `GL_ANGLE_instanced_arrays` | no | 3 | 1 | -- |
| `GL_ANGLE_pack_reverse_row_order` | no | 0 | 1 | -- |
| `GL_ANGLE_program_binary` | no | 0 | 1 | -- |
| `GL_ANGLE_texture_compression_dxt3` | no | 0 | 1 | -- |
| `GL_ANGLE_texture_compression_dxt5` | no | 0 | 1 | -- |
| `GL_ANGLE_texture_usage` | no | 0 | 2 | -- |
| `GL_ANGLE_translated_shader_source` | no | 1 | 1 | -- |
| `GL_APPLE_aux_depth_stencil` | yes | 0 | 1 | -- |
| `GL_APPLE_client_storage` | yes | 0 | 1 | -- |
| `GL_APPLE_clip_distance` | no | 0 | 9 | -- |
| `GL_APPLE_color_buffer_packed_float` | no | 0 | 0 | -- |
| `GL_APPLE_copy_texture_levels` | no | 1 | 0 | -- |
| `GL_APPLE_element_array` | yes | 5 | 3 | -- |
| `GL_APPLE_fence` | yes | 8 | 2 | -- |
| `GL_APPLE_float_pixels` | yes | 0 | 14 | -- |
| `GL_APPLE_flush_buffer_range` | yes | 2 | 2 | -- |
| `GL_APPLE_framebuffer_multisample` | no | 2 | 7 | -- |
| `GL_APPLE_object_purgeable` | yes | 3 | 6 | -- |
| `GL_APPLE_rgb_422` | yes | 0 | 4 | -- |
| `GL_APPLE_row_bytes` | yes | 0 | 2 | -- |
| `GL_APPLE_specular_vector` | yes | 0 | 1 | -- |
| `GL_APPLE_sync` | no | 7 | 16 | -- |
| `GL_APPLE_texture_2D_limited_npot` | no | 0 | 0 | -- |
| `GL_APPLE_texture_format_BGRA8888` | no | 0 | 2 | -- |
| `GL_APPLE_texture_max_level` | no | 0 | 1 | -- |
| `GL_APPLE_texture_packed_float` | no | 0 | 4 | -- |
| `GL_APPLE_texture_range` | yes | 2 | 6 | -- |
| `GL_APPLE_transform_hint` | yes | 0 | 1 | -- |
| `GL_APPLE_vertex_array_object` | yes | 4 | 1 | -- |
| `GL_APPLE_vertex_array_range` | yes | 3 | 7 | -- |
| `GL_APPLE_vertex_program_evaluators` | yes | 7 | 10 | -- |
| `GL_APPLE_ycbcr_422` | yes | 0 | 3 | -- |
| `GL_ARB_ES2_compatibility` | yes | 5 | 16 | 4.1 |
| `GL_ARB_ES3_1_compatibility` | yes | 1 | 1 | 4.5 |
| `GL_ARB_ES3_2_compatibility` | yes | 1 | 3 | -- |
| `GL_ARB_ES3_compatibility` | yes | 0 | 13 | -- |
| `GL_ARB_arrays_of_arrays` | yes | 0 | 0 | -- |
| `GL_ARB_base_instance` | yes | 3 | 0 | 4.2 |
| `GL_ARB_bindless_texture` | yes | 16 | 1 | -- |
| `GL_ARB_blend_func_extended` | yes | 2 | 5 | 3.3 |
| `GL_ARB_buffer_storage` | yes | 1 | 9 | 4.4 |
| `GL_ARB_cl_event` | yes | 1 | 2 | -- |
| `GL_ARB_clear_buffer_object` | yes | 2 | 0 | 4.3 |
| `GL_ARB_clear_texture` | yes | 2 | 1 | 4.4 |
| `GL_ARB_clip_control` | yes | 1 | 6 | 4.5 |
| `GL_ARB_color_buffer_float` | yes | 1 | 5 | -- |
| `GL_ARB_compatibility` | yes | 0 | 0 | -- |
| `GL_ARB_compressed_texture_pixel_storage` | yes | 0 | 8 | -- |
| `GL_ARB_compute_shader` | yes | 2 | 18 | 4.3 |
| `GL_ARB_compute_variable_group_size` | yes | 1 | 4 | -- |
| `GL_ARB_conditional_render_inverted` | yes | 0 | 4 | -- |
| `GL_ARB_conservative_depth` | yes | 0 | 0 | -- |
| `GL_ARB_copy_buffer` | yes | 1 | 2 | 3.1 |
| `GL_ARB_copy_image` | yes | 1 | 0 | 4.3 |
| `GL_ARB_cull_distance` | yes | 0 | 2 | -- |
| `GL_ARB_debug_output` | yes | 4 | 22 | -- |
| `GL_ARB_depth_buffer_float` | yes | 0 | 3 | -- |
| `GL_ARB_depth_clamp` | yes | 0 | 1 | -- |
| `GL_ARB_depth_texture` | yes | 0 | 5 | -- |
| `GL_ARB_derivative_control` | yes | 0 | 0 | -- |
| `GL_ARB_direct_state_access` | yes | 97 | 13 | 4.5 |
| `GL_ARB_draw_buffers` | yes | 1 | 17 | -- |
| `GL_ARB_draw_buffers_blend` | yes | 4 | 0 | -- |
| `GL_ARB_draw_elements_base_vertex` | yes | 4 | 0 | 3.2 |
| `GL_ARB_draw_indirect` | yes | 2 | 2 | 4.0 |
| `GL_ARB_draw_instanced` | yes | 2 | 0 | -- |
| `GL_ARB_enhanced_layouts` | yes | 0 | 4 | -- |
| `GL_ARB_explicit_attrib_location` | yes | 0 | 0 | -- |
| `GL_ARB_explicit_uniform_location` | yes | 0 | 1 | -- |
| `GL_ARB_fragment_coord_conventions` | yes | 0 | 0 | -- |
| `GL_ARB_fragment_layer_viewport` | yes | 0 | 0 | -- |
| `GL_ARB_fragment_program` | yes | 19 | 78 | -- |
| `GL_ARB_fragment_program_shadow` | yes | 0 | 0 | -- |
| `GL_ARB_fragment_shader` | yes | 0 | 3 | -- |
| `GL_ARB_fragment_shader_interlock` | yes | 0 | 0 | -- |
| `GL_ARB_framebuffer_no_attachments` | yes | 2 | 9 | 4.3 |
| `GL_ARB_framebuffer_object` | yes | 20 | 73 | 3.0 |
| `GL_ARB_framebuffer_sRGB` | yes | 0 | 1 | -- |
| `GL_ARB_geometry_shader4` | yes | 4 | 20 | -- |
| `GL_ARB_get_program_binary` | yes | 3 | 4 | 4.1 |
| `GL_ARB_get_texture_sub_image` | yes | 2 | 0 | 4.5 |
| `GL_ARB_gl_spirv` | yes | 1 | 2 | -- |
| `GL_ARB_gpu_shader5` | yes | 0 | 6 | -- |
| `GL_ARB_gpu_shader_fp64` | yes | 18 | 13 | 4.0 |
| `GL_ARB_gpu_shader_int64` | yes | 36 | 8 | -- |
| `GL_ARB_half_float_pixel` | yes | 0 | 1 | -- |
| `GL_ARB_half_float_vertex` | yes | 0 | 1 | -- |
| `GL_ARB_imaging` | yes | 34 | 75 | -- |
| `GL_ARB_indirect_parameters` | yes | 2 | 2 | -- |
| `GL_ARB_instanced_arrays` | yes | 1 | 1 | -- |
| `GL_ARB_internalformat_query` | yes | 1 | 1 | 4.2 |
| `GL_ARB_internalformat_query2` | yes | 1 | 134 | 4.3 |
| `GL_ARB_invalidate_subdata` | yes | 6 | 0 | 4.3 |
| `GL_ARB_map_buffer_alignment` | yes | 0 | 1 | -- |
| `GL_ARB_map_buffer_range` | yes | 2 | 6 | 3.0 |
| `GL_ARB_matrix_palette` | yes | 5 | 10 | -- |
| `GL_ARB_multi_bind` | yes | 6 | 0 | 4.4 |
| `GL_ARB_multi_draw_indirect` | yes | 2 | 0 | 4.3 |
| `GL_ARB_multisample` | yes | 1 | 9 | -- |
| `GL_ARB_multitexture` | yes | 34 | 35 | -- |
| `GL_ARB_occlusion_query` | yes | 8 | 5 | -- |
| `GL_ARB_occlusion_query2` | yes | 0 | 1 | -- |
| `GL_ARB_parallel_shader_compile` | yes | 1 | 2 | -- |
| `GL_ARB_pipeline_statistics_query` | yes | 0 | 11 | -- |
| `GL_ARB_pixel_buffer_object` | yes | 0 | 4 | -- |
| `GL_ARB_point_parameters` | yes | 2 | 4 | -- |
| `GL_ARB_point_sprite` | yes | 0 | 2 | -- |
| `GL_ARB_polygon_offset_clamp` | yes | 1 | 1 | 4.6 |
| `GL_ARB_post_depth_coverage` | yes | 0 | 0 | -- |
| `GL_ARB_program_interface_query` | yes | 6 | 50 | 4.3 |
| `GL_ARB_provoking_vertex` | yes | 1 | 4 | 3.2 |
| `GL_ARB_query_buffer_object` | yes | 0 | 4 | -- |
| `GL_ARB_robust_buffer_access_behavior` | yes | 0 | 0 | -- |
| `GL_ARB_robustness` | yes | 20 | 8 | -- |
| `GL_ARB_robustness_isolation` | yes | 0 | 0 | -- |
| `GL_ARB_sample_locations` | yes | 3 | 8 | -- |
| `GL_ARB_sample_shading` | yes | 1 | 2 | -- |
| `GL_ARB_sampler_objects` | yes | 14 | 1 | 3.3 |
| `GL_ARB_seamless_cube_map` | yes | 0 | 1 | -- |
| `GL_ARB_seamless_cubemap_per_texture` | yes | 0 | 1 | -- |
| `GL_ARB_separate_shader_objects` | yes | 61 | 9 | 4.1 |
| `GL_ARB_shader_atomic_counter_ops` | yes | 0 | 0 | -- |
| `GL_ARB_shader_atomic_counters` | yes | 1 | 29 | 4.2 |
| `GL_ARB_shader_ballot` | yes | 0 | 0 | -- |
| `GL_ARB_shader_bit_encoding` | yes | 0 | 0 | -- |
| `GL_ARB_shader_clock` | yes | 0 | 0 | -- |
| `GL_ARB_shader_draw_parameters` | yes | 0 | 0 | -- |
| `GL_ARB_shader_group_vote` | yes | 0 | 0 | -- |
| `GL_ARB_shader_image_load_store` | yes | 2 | 64 | 4.2 |
| `GL_ARB_shader_image_size` | yes | 0 | 0 | -- |
| `GL_ARB_shader_objects` | yes | 39 | 34 | -- |
| `GL_ARB_shader_precision` | yes | 0 | 0 | -- |
| `GL_ARB_shader_stencil_export` | yes | 0 | 0 | -- |
| `GL_ARB_shader_storage_buffer_object` | yes | 1 | 17 | 4.3 |
| `GL_ARB_shader_subroutine` | yes | 8 | 11 | 4.0 |
| `GL_ARB_shader_texture_image_samples` | yes | 0 | 0 | -- |
| `GL_ARB_shader_texture_lod` | yes | 0 | 0 | -- |
| `GL_ARB_shader_viewport_layer_array` | yes | 0 | 0 | -- |
| `GL_ARB_shading_language_100` | yes | 0 | 1 | -- |
| `GL_ARB_shading_language_420pack` | yes | 0 | 0 | -- |
| `GL_ARB_shading_language_include` | yes | 6 | 3 | -- |
| `GL_ARB_shading_language_packing` | yes | 0 | 0 | -- |
| `GL_ARB_shadow` | yes | 0 | 3 | -- |
| `GL_ARB_shadow_ambient` | yes | 0 | 1 | -- |
| `GL_ARB_sparse_buffer` | yes | 3 | 2 | -- |
| `GL_ARB_sparse_texture` | yes | 1 | 11 | -- |
| `GL_ARB_sparse_texture2` | yes | 0 | 0 | -- |
| `GL_ARB_sparse_texture_clamp` | yes | 0 | 0 | -- |
| `GL_ARB_spirv_extensions` | yes | 0 | 2 | -- |
| `GL_ARB_stencil_texturing` | yes | 0 | 1 | -- |
| `GL_ARB_sync` | yes | 7 | 15 | 3.2 |
| `GL_ARB_tessellation_shader` | yes | 2 | 37 | 4.0 |
| `GL_ARB_texture_barrier` | yes | 1 | 0 | 4.5 |
| `GL_ARB_texture_border_clamp` | yes | 0 | 1 | -- |
| `GL_ARB_texture_buffer_object` | yes | 1 | 5 | -- |
| `GL_ARB_texture_buffer_object_rgb32` | yes | 0 | 3 | -- |
| `GL_ARB_texture_buffer_range` | yes | 1 | 3 | 4.3 |
| `GL_ARB_texture_compression` | yes | 7 | 11 | -- |
| `GL_ARB_texture_compression_bptc` | yes | 0 | 4 | -- |
| `GL_ARB_texture_compression_rgtc` | yes | 0 | 4 | -- |
| `GL_ARB_texture_cube_map` | yes | 0 | 12 | -- |
| `GL_ARB_texture_cube_map_array` | yes | 0 | 7 | -- |
| `GL_ARB_texture_env_add` | yes | 0 | 0 | -- |
| `GL_ARB_texture_env_combine` | yes | 0 | 22 | -- |
| `GL_ARB_texture_env_crossbar` | yes | 0 | 0 | -- |
| `GL_ARB_texture_env_dot3` | yes | 0 | 2 | -- |
| `GL_ARB_texture_filter_anisotropic` | yes | 0 | 2 | -- |
| `GL_ARB_texture_filter_minmax` | yes | 0 | 2 | -- |
| `GL_ARB_texture_float` | yes | 0 | 20 | -- |
| `GL_ARB_texture_gather` | yes | 0 | 3 | -- |
| `GL_ARB_texture_mirror_clamp_to_edge` | yes | 0 | 1 | -- |
| `GL_ARB_texture_mirrored_repeat` | yes | 0 | 1 | -- |
| `GL_ARB_texture_multisample` | yes | 4 | 21 | 3.2 |
| `GL_ARB_texture_non_power_of_two` | yes | 0 | 0 | -- |
| `GL_ARB_texture_query_levels` | yes | 0 | 0 | -- |
| `GL_ARB_texture_query_lod` | yes | 0 | 0 | -- |
| `GL_ARB_texture_rectangle` | yes | 0 | 4 | -- |
| `GL_ARB_texture_rg` | yes | 0 | 22 | -- |
| `GL_ARB_texture_rgb10_a2ui` | yes | 0 | 1 | -- |
| `GL_ARB_texture_stencil8` | yes | 0 | 2 | -- |
| `GL_ARB_texture_storage` | yes | 3 | 1 | 4.2 |
| `GL_ARB_texture_storage_multisample` | yes | 2 | 0 | 4.3 |
| `GL_ARB_texture_swizzle` | yes | 0 | 5 | -- |
| `GL_ARB_texture_view` | yes | 1 | 5 | 4.3 |
| `GL_ARB_timer_query` | yes | 3 | 2 | 3.3 |
| `GL_ARB_transform_feedback2` | yes | 7 | 4 | 4.0 |
| `GL_ARB_transform_feedback3` | yes | 4 | 2 | 4.0 |
| `GL_ARB_transform_feedback_instanced` | yes | 2 | 0 | 4.2 |
| `GL_ARB_transform_feedback_overflow_query` | yes | 0 | 2 | -- |
| `GL_ARB_transpose_matrix` | yes | 4 | 4 | -- |
| `GL_ARB_uniform_buffer_object` | yes | 10 | 33 | 3.1 |
| `GL_ARB_vertex_array_bgra` | yes | 0 | 1 | -- |
| `GL_ARB_vertex_array_object` | yes | 4 | 1 | 3.0 |
| `GL_ARB_vertex_attrib_64bit` | yes | 10 | 13 | 4.1 |
| `GL_ARB_vertex_attrib_binding` | yes | 6 | 7 | 4.3 |
| `GL_ARB_vertex_blend` | yes | 10 | 42 | -- |
| `GL_ARB_vertex_buffer_object` | yes | 11 | 31 | -- |
| `GL_ARB_vertex_program` | yes | 62 | 79 | -- |
| `GL_ARB_vertex_shader` | yes | 46 | 26 | -- |
| `GL_ARB_vertex_type_10f_11f_11f_rev` | yes | 0 | 1 | -- |
| `GL_ARB_vertex_type_2_10_10_10_rev` | yes | 38 | 2 | -- |
| `GL_ARB_viewport_array` | yes | 12 | 13 | -- |
| `GL_ARB_window_pos` | yes | 16 | 0 | -- |
| `GL_ARM_mali_program_binary` | no | 0 | 1 | -- |
| `GL_ARM_mali_shader_binary` | no | 0 | 1 | -- |
| `GL_ARM_rgba8` | no | 0 | 0 | -- |
| `GL_ARM_shader_core_properties` | no | 1 | 7 | -- |
| `GL_ARM_shader_framebuffer_fetch` | no | 0 | 2 | -- |
| `GL_ARM_shader_framebuffer_fetch_depth_stencil` | no | 0 | 0 | -- |
| `GL_ARM_texture_unnormalized_coordinates` | no | 0 | 1 | -- |
| `GL_ATI_draw_buffers` | yes | 1 | 17 | -- |
| `GL_ATI_element_array` | yes | 3 | 3 | -- |
| `GL_ATI_envmap_bumpmap` | yes | 4 | 8 | -- |
| `GL_ATI_fragment_shader` | yes | 14 | 104 | -- |
| `GL_ATI_map_object_buffer` | yes | 2 | 0 | -- |
| `GL_ATI_meminfo` | yes | 0 | 3 | -- |
| `GL_ATI_pixel_format_float` | yes | 0 | 2 | -- |
| `GL_ATI_pn_triangles` | yes | 2 | 9 | -- |
| `GL_ATI_separate_stencil` | yes | 2 | 4 | -- |
| `GL_ATI_text_fragment_shader` | yes | 0 | 1 | -- |
| `GL_ATI_texture_env_combine3` | yes | 0 | 3 | -- |
| `GL_ATI_texture_float` | yes | 0 | 12 | -- |
| `GL_ATI_texture_mirror_once` | yes | 0 | 2 | -- |
| `GL_ATI_vertex_array_object` | yes | 12 | 8 | -- |
| `GL_ATI_vertex_attrib_array_object` | yes | 3 | 0 | -- |
| `GL_ATI_vertex_streams` | yes | 45 | 10 | -- |
| `GL_DMP_program_binary` | no | 0 | 3 | -- |
| `GL_DMP_shader_binary` | no | 0 | 1 | -- |
| `GL_EXT_422_pixels` | yes | 0 | 4 | -- |
| `GL_EXT_EGL_image_array` | no | 0 | 0 | -- |
| `GL_EXT_EGL_image_storage` | yes | 2 | 0 | -- |
| `GL_EXT_EGL_image_storage_compression` | no | 0 | 3 | -- |
| `GL_EXT_EGL_sync` | yes | 0 | 0 | -- |
| `GL_EXT_YUV_target` | no | 0 | 4 | -- |
| `GL_EXT_abgr` | yes | 0 | 1 | -- |
| `GL_EXT_base_instance` | no | 3 | 0 | -- |
| `GL_EXT_bgra` | yes | 0 | 2 | -- |
| `GL_EXT_bindable_uniform` | yes | 3 | 6 | -- |
| `GL_EXT_blend_color` | yes | 1 | 5 | -- |
| `GL_EXT_blend_equation_separate` | yes | 1 | 2 | -- |
| `GL_EXT_blend_func_extended` | no | 4 | 7 | -- |
| `GL_EXT_blend_func_separate` | yes | 1 | 4 | -- |
| `GL_EXT_blend_logic_op` | yes | 0 | 0 | -- |
| `GL_EXT_blend_minmax` | yes | 1 | 4 | -- |
| `GL_EXT_blend_subtract` | yes | 0 | 2 | -- |
| `GL_EXT_buffer_storage` | no | 1 | 9 | -- |
| `GL_EXT_clear_texture` | no | 2 | 0 | -- |
| `GL_EXT_clip_control` | no | 1 | 6 | -- |
| `GL_EXT_clip_cull_distance` | no | 0 | 11 | -- |
| `GL_EXT_clip_volume_hint` | yes | 0 | 1 | -- |
| `GL_EXT_cmyka` | yes | 0 | 4 | -- |
| `GL_EXT_color_buffer_float` | no | 0 | 0 | -- |
| `GL_EXT_color_buffer_half_float` | no | 0 | 6 | -- |
| `GL_EXT_color_subtable` | yes | 2 | 0 | -- |
| `GL_EXT_compiled_vertex_array` | yes | 2 | 2 | -- |
| `GL_EXT_conservative_depth` | no | 0 | 0 | -- |
| `GL_EXT_convolution` | yes | 13 | 20 | -- |
| `GL_EXT_coordinate_frame` | yes | 22 | 14 | -- |
| `GL_EXT_copy_image` | no | 1 | 0 | -- |
| `GL_EXT_copy_texture` | yes | 5 | 0 | -- |
| `GL_EXT_cull_vertex` | yes | 2 | 3 | -- |
| `GL_EXT_debug_label` | yes | 2 | 8 | -- |
| `GL_EXT_debug_marker` | yes | 3 | 0 | -- |
| `GL_EXT_depth_bounds_test` | yes | 1 | 2 | -- |
| `GL_EXT_depth_clamp` | no | 0 | 1 | -- |
| `GL_EXT_direct_state_access` | yes | 255 | 3 | -- |
| `GL_EXT_discard_framebuffer` | no | 1 | 3 | -- |
| `GL_EXT_disjoint_timer_query` | no | 12 | 7 | -- |
| `GL_EXT_draw_buffers` | no | 1 | 34 | -- |
| `GL_EXT_draw_buffers2` | yes | 6 | 0 | -- |
| `GL_EXT_draw_buffers_indexed` | no | 8 | 28 | -- |
| `GL_EXT_draw_elements_base_vertex` | no | 4 | 0 | -- |
| `GL_EXT_draw_instanced` | yes | 2 | 0 | -- |
| `GL_EXT_draw_range_elements` | yes | 1 | 2 | -- |
| `GL_EXT_draw_transform_feedback` | no | 2 | 0 | -- |
| `GL_EXT_external_buffer` | yes | 2 | 0 | -- |
| `GL_EXT_float_blend` | no | 0 | 0 | -- |
| `GL_EXT_fog_coord` | yes | 5 | 8 | -- |
| `GL_EXT_fragment_shading_rate` | yes | 4 | 27 | -- |
| `GL_EXT_framebuffer_blit` | yes | 1 | 4 | -- |
| `GL_EXT_framebuffer_blit_layers` | yes | 2 | 0 | -- |
| `GL_EXT_framebuffer_multisample` | yes | 1 | 3 | -- |
| `GL_EXT_framebuffer_multisample_blit_scaled` | yes | 0 | 2 | -- |
| `GL_EXT_framebuffer_object` | yes | 17 | 51 | -- |
| `GL_EXT_framebuffer_sRGB` | yes | 0 | 2 | -- |
| `GL_EXT_geometry_point_size` | no | 0 | 0 | -- |
| `GL_EXT_geometry_shader` | no | 1 | 33 | -- |
| `GL_EXT_geometry_shader4` | yes | 1 | 20 | -- |
| `GL_EXT_gpu_program_parameters` | yes | 2 | 0 | -- |
| `GL_EXT_gpu_shader4` | yes | 34 | 28 | -- |
| `GL_EXT_gpu_shader5` | no | 0 | 0 | -- |
| `GL_EXT_histogram` | yes | 10 | 14 | -- |
| `GL_EXT_index_array_formats` | yes | 0 | 8 | -- |
| `GL_EXT_index_func` | yes | 1 | 3 | -- |
| `GL_EXT_index_material` | yes | 1 | 3 | -- |
| `GL_EXT_index_texture` | yes | 0 | 0 | -- |
| `GL_EXT_instanced_arrays` | no | 3 | 1 | -- |
| `GL_EXT_light_texture` | yes | 3 | 10 | -- |
| `GL_EXT_map_buffer_range` | no | 2 | 6 | -- |
| `GL_EXT_memory_object` | yes | 19 | 11 | -- |
| `GL_EXT_memory_object_fd` | yes | 1 | 1 | -- |
| `GL_EXT_memory_object_win32` | yes | 2 | 9 | -- |
| `GL_EXT_mesh_shader` | yes | 4 | 66 | -- |
| `GL_EXT_misc_attribute` | yes | 0 | 0 | -- |
| `GL_EXT_multi_draw_arrays` | yes | 2 | 0 | -- |
| `GL_EXT_multi_draw_indirect` | no | 2 | 0 | -- |
| `GL_EXT_multisample` | yes | 2 | 17 | -- |
| `GL_EXT_multisampled_compatibility` | no | 0 | 2 | -- |
| `GL_EXT_multisampled_render_to_texture` | no | 2 | 4 | -- |
| `GL_EXT_multisampled_render_to_texture2` | no | 0 | 0 | -- |
| `GL_EXT_multiview_draw_buffers` | no | 3 | 5 | -- |
| `GL_EXT_multiview_tessellation_geometry_shader` | yes | 0 | 0 | -- |
| `GL_EXT_multiview_texture_multisample` | yes | 0 | 0 | -- |
| `GL_EXT_multiview_timer_query` | yes | 0 | 0 | -- |
| `GL_EXT_occlusion_query_boolean` | no | 7 | 5 | -- |
| `GL_EXT_packed_depth_stencil` | yes | 0 | 4 | -- |
| `GL_EXT_packed_float` | yes | 0 | 3 | -- |
| `GL_EXT_packed_pixels` | yes | 0 | 5 | -- |
| `GL_EXT_paletted_texture` | yes | 4 | 7 | -- |
| `GL_EXT_pixel_buffer_object` | yes | 0 | 4 | -- |
| `GL_EXT_pixel_transform` | yes | 6 | 9 | -- |
| `GL_EXT_pixel_transform_color_table` | yes | 0 | 0 | -- |
| `GL_EXT_point_parameters` | yes | 2 | 4 | -- |
| `GL_EXT_polygon_offset` | yes | 1 | 3 | -- |
| `GL_EXT_polygon_offset_clamp` | yes | 1 | 1 | -- |
| `GL_EXT_post_depth_coverage` | yes | 0 | 0 | -- |
| `GL_EXT_primitive_bounding_box` | no | 1 | 1 | -- |
| `GL_EXT_protected_textures` | no | 0 | 2 | -- |
| `GL_EXT_provoking_vertex` | yes | 1 | 4 | -- |
| `GL_EXT_pvrtc_sRGB` | no | 0 | 6 | -- |
| `GL_EXT_raster_multisample` | yes | 1 | 6 | -- |
| `GL_EXT_read_format_bgra` | no | 0 | 3 | -- |
| `GL_EXT_render_snorm` | no | 0 | 8 | -- |
| `GL_EXT_rescale_normal` | yes | 0 | 1 | -- |
| `GL_EXT_robustness` | no | 4 | 8 | -- |
| `GL_EXT_sRGB` | no | 0 | 4 | -- |
| `GL_EXT_sRGB_write_control` | no | 0 | 1 | -- |
| `GL_EXT_secondary_color` | yes | 17 | 7 | -- |
| `GL_EXT_semaphore` | yes | 9 | 13 | -- |
| `GL_EXT_semaphore_fd` | yes | 1 | 1 | -- |
| `GL_EXT_semaphore_win32` | yes | 2 | 7 | -- |
| `GL_EXT_separate_depth_stencil` | no | 0 | 0 | -- |
| `GL_EXT_separate_shader_objects` | yes | 47 | 6 | -- |
| `GL_EXT_separate_specular_color` | yes | 0 | 3 | -- |
| `GL_EXT_shader_framebuffer_fetch` | yes | 0 | 1 | -- |
| `GL_EXT_shader_framebuffer_fetch_non_coherent` | yes | 1 | 1 | -- |
| `GL_EXT_shader_group_vote` | no | 0 | 0 | -- |
| `GL_EXT_shader_image_load_formatted` | yes | 0 | 0 | -- |
| `GL_EXT_shader_image_load_store` | yes | 2 | 55 | -- |
| `GL_EXT_shader_implicit_conversions` | no | 0 | 0 | -- |
| `GL_EXT_shader_integer_mix` | yes | 0 | 0 | -- |
| `GL_EXT_shader_io_blocks` | no | 0 | 0 | -- |
| `GL_EXT_shader_non_constant_global_initializers` | no | 0 | 0 | -- |
| `GL_EXT_shader_pixel_local_storage` | no | 0 | 3 | -- |
| `GL_EXT_shader_pixel_local_storage2` | no | 3 | 3 | -- |
| `GL_EXT_shader_samples_identical` | yes | 0 | 0 | -- |
| `GL_EXT_shader_texture_lod` | no | 0 | 0 | -- |
| `GL_EXT_shader_texture_samples` | no | 0 | 0 | -- |
| `GL_EXT_shadow_funcs` | yes | 0 | 0 | -- |
| `GL_EXT_shadow_samplers` | no | 0 | 4 | -- |
| `GL_EXT_shared_texture_palette` | yes | 0 | 1 | -- |
| `GL_EXT_sparse_texture` | no | 1 | 16 | -- |
| `GL_EXT_sparse_texture2` | yes | 0 | 0 | -- |
| `GL_EXT_stencil_clear_tag` | yes | 1 | 2 | -- |
| `GL_EXT_stencil_two_side` | yes | 1 | 2 | -- |
| `GL_EXT_stencil_wrap` | yes | 0 | 2 | -- |
| `GL_EXT_subtexture` | yes | 2 | 0 | -- |
| `GL_EXT_tessellation_point_size` | no | 0 | 0 | -- |
| `GL_EXT_tessellation_shader` | no | 1 | 47 | -- |
| `GL_EXT_texture` | yes | 0 | 43 | -- |
| `GL_EXT_texture3D` | yes | 2 | 9 | -- |
| `GL_EXT_texture_array` | yes | 1 | 9 | -- |
| `GL_EXT_texture_border_clamp` | no | 8 | 2 | -- |
| `GL_EXT_texture_buffer` | no | 2 | 14 | -- |
| `GL_EXT_texture_buffer_object` | yes | 1 | 5 | -- |
| `GL_EXT_texture_compression_astc_decode_mode` | no | 0 | 1 | -- |
| `GL_EXT_texture_compression_bptc` | no | 0 | 4 | -- |
| `GL_EXT_texture_compression_dxt1` | no | 0 | 2 | -- |
| `GL_EXT_texture_compression_latc` | yes | 0 | 4 | -- |
| `GL_EXT_texture_compression_rgtc` | yes | 0 | 4 | -- |
| `GL_EXT_texture_compression_s3tc` | yes | 0 | 4 | -- |
| `GL_EXT_texture_compression_s3tc_srgb` | no | 0 | 4 | -- |
| `GL_EXT_texture_cube_map` | yes | 0 | 12 | -- |
| `GL_EXT_texture_cube_map_array` | no | 0 | 9 | -- |
| `GL_EXT_texture_env_add` | yes | 0 | 0 | -- |
| `GL_EXT_texture_env_combine` | yes | 0 | 21 | -- |
| `GL_EXT_texture_env_dot3` | yes | 0 | 2 | -- |
| `GL_EXT_texture_filter_anisotropic` | yes | 0 | 2 | -- |
| `GL_EXT_texture_filter_minmax` | yes | 0 | 2 | -- |
| `GL_EXT_texture_format_BGRA8888` | no | 0 | 1 | -- |
| `GL_EXT_texture_format_sRGB_override` | no | 0 | 1 | -- |
| `GL_EXT_texture_integer` | yes | 6 | 47 | -- |
| `GL_EXT_texture_lod_bias` | yes | 0 | 3 | -- |
| `GL_EXT_texture_mirror_clamp` | yes | 0 | 3 | -- |
| `GL_EXT_texture_mirror_clamp_to_edge` | no | 0 | 1 | -- |
| `GL_EXT_texture_norm16` | no | 0 | 8 | -- |
| `GL_EXT_texture_object` | yes | 6 | 5 | -- |
| `GL_EXT_texture_perturb_normal` | yes | 1 | 2 | -- |
| `GL_EXT_texture_query_lod` | no | 0 | 0 | -- |
| `GL_EXT_texture_rg` | no | 0 | 4 | -- |
| `GL_EXT_texture_sRGB` | yes | 0 | 16 | -- |
| `GL_EXT_texture_sRGB_R8` | yes | 0 | 1 | -- |
| `GL_EXT_texture_sRGB_RG8` | yes | 0 | 1 | -- |
| `GL_EXT_texture_sRGB_decode` | yes | 0 | 3 | -- |
| `GL_EXT_texture_shadow_lod` | yes | 0 | 0 | -- |
| `GL_EXT_texture_shared_exponent` | yes | 0 | 3 | -- |
| `GL_EXT_texture_snorm` | yes | 0 | 25 | -- |
| `GL_EXT_texture_storage` | yes | 6 | 23 | -- |
| `GL_EXT_texture_storage_compression` | no | 2 | 16 | -- |
| `GL_EXT_texture_swizzle` | yes | 0 | 5 | -- |
| `GL_EXT_texture_type_2_10_10_10_REV` | no | 0 | 1 | -- |
| `GL_EXT_texture_view` | no | 1 | 5 | -- |
| `GL_EXT_timer_query` | yes | 2 | 1 | -- |
| `GL_EXT_transform_feedback` | yes | 7 | 15 | -- |
| `GL_EXT_unpack_subimage` | no | 0 | 3 | -- |
| `GL_EXT_vertex_array` | yes | 9 | 32 | -- |
| `GL_EXT_vertex_array_bgra` | yes | 0 | 1 | -- |
| `GL_EXT_vertex_attrib_64bit` | yes | 10 | 13 | -- |
| `GL_EXT_vertex_shader` | yes | 42 | 110 | -- |
| `GL_EXT_vertex_weighting` | yes | 3 | 13 | -- |
| `GL_EXT_win32_keyed_mutex` | yes | 2 | 0 | -- |
| `GL_EXT_window_rectangles` | yes | 1 | 6 | -- |
| `GL_EXT_x11_sync_object` | yes | 1 | 1 | -- |
| `GL_FJ_shader_binary_GCCSO` | no | 0 | 1 | -- |
| `GL_GREMEDY_frame_terminator` | yes | 1 | 0 | -- |
| `GL_GREMEDY_string_marker` | yes | 1 | 0 | -- |
| `GL_HP_convolution_border_modes` | yes | 0 | 4 | -- |
| `GL_HP_image_transform` | yes | 6 | 15 | -- |
| `GL_HP_occlusion_test` | yes | 0 | 2 | -- |
| `GL_HP_texture_lighting` | yes | 0 | 3 | -- |
| `GL_HUAWEI_program_binary` | no | 0 | 1 | -- |
| `GL_HUAWEI_shader_binary` | no | 0 | 1 | -- |
| `GL_IBM_cull_vertex` | yes | 0 | 1 | -- |
| `GL_IBM_multimode_draw_arrays` | yes | 2 | 0 | -- |
| `GL_IBM_rasterpos_clip` | yes | 0 | 1 | -- |
| `GL_IBM_static_data` | yes | 1 | 2 | -- |
| `GL_IBM_texture_mirrored_repeat` | yes | 0 | 1 | -- |
| `GL_IBM_vertex_array_lists` | yes | 8 | 16 | -- |
| `GL_IMG_bindless_texture` | no | 6 | 0 | -- |
| `GL_IMG_framebuffer_downsample` | no | 2 | 4 | -- |
| `GL_IMG_multisampled_render_to_texture` | no | 2 | 4 | -- |
| `GL_IMG_program_binary` | no | 0 | 1 | -- |
| `GL_IMG_pvric_end_to_end_signature` | no | 0 | 1 | -- |
| `GL_IMG_read_format` | no | 0 | 2 | -- |
| `GL_IMG_shader_binary` | no | 0 | 1 | -- |
| `GL_IMG_texture_compression_pvrtc` | no | 0 | 4 | -- |
| `GL_IMG_texture_compression_pvrtc2` | no | 0 | 2 | -- |
| `GL_IMG_texture_env_enhanced_fixed_function` | no | 0 | 7 | -- |
| `GL_IMG_texture_filter_cubic` | no | 0 | 3 | -- |
| `GL_IMG_tile_region_protection` | no | 0 | 3 | -- |
| `GL_IMG_user_clip_plane` | no | 2 | 7 | -- |
| `GL_INGR_blend_func_separate` | yes | 1 | 0 | -- |
| `GL_INGR_color_clamp` | yes | 0 | 8 | -- |
| `GL_INGR_interlace_read` | yes | 0 | 1 | -- |
| `GL_INTEL_blackhole_render` | yes | 0 | 1 | -- |
| `GL_INTEL_conservative_rasterization` | yes | 0 | 1 | -- |
| `GL_INTEL_fragment_shader_ordering` | yes | 0 | 0 | -- |
| `GL_INTEL_framebuffer_CMAA` | yes | 1 | 0 | -- |
| `GL_INTEL_map_texture` | yes | 3 | 4 | -- |
| `GL_INTEL_parallel_arrays` | yes | 4 | 5 | -- |
| `GL_INTEL_performance_query` | yes | 10 | 20 | -- |
| `GL_KHR_blend_equation_advanced` | yes | 1 | 15 | -- |
| `GL_KHR_blend_equation_advanced_coherent` | yes | 0 | 1 | -- |
| `GL_KHR_context_flush_control` | yes | 0 | 5 | -- |
| `GL_KHR_debug` | yes | 22 | 81 | -- |
| `GL_KHR_no_error` | yes | 0 | 1 | -- |
| `GL_KHR_parallel_shader_compile` | yes | 1 | 2 | -- |
| `GL_KHR_robust_buffer_access_behavior` | yes | 0 | 0 | -- |
| `GL_KHR_robustness` | yes | 10 | 17 | -- |
| `GL_KHR_shader_subgroup` | yes | 0 | 12 | -- |
| `GL_KHR_texture_compression_astc_hdr` | yes | 0 | 28 | -- |
| `GL_KHR_texture_compression_astc_ldr` | yes | 0 | 28 | -- |
| `GL_KHR_texture_compression_astc_sliced_3d` | yes | 0 | 0 | -- |
| `GL_MESAX_texture_stack` | yes | 0 | 6 | -- |
| `GL_MESA_bgra` | no | 0 | 2 | -- |
| `GL_MESA_framebuffer_flip_x` | yes | 0 | 1 | -- |
| `GL_MESA_framebuffer_flip_y` | yes | 2 | 1 | -- |
| `GL_MESA_framebuffer_swap_xy` | yes | 0 | 1 | -- |
| `GL_MESA_map_buffer_client_pointer` | yes | 2 | 12 | -- |
| `GL_MESA_pack_invert` | yes | 0 | 1 | -- |
| `GL_MESA_program_binary_formats` | yes | 0 | 1 | -- |
| `GL_MESA_resize_buffers` | yes | 1 | 0 | -- |
| `GL_MESA_sampler_objects` | no | 10 | 1 | 3.3 |
| `GL_MESA_shader_integer_functions` | yes | 0 | 0 | -- |
| `GL_MESA_texture_const_bandwidth` | yes | 0 | 1 | -- |
| `GL_MESA_tile_raster_order` | yes | 0 | 3 | -- |
| `GL_MESA_window_pos` | yes | 24 | 0 | -- |
| `GL_MESA_ycbcr_texture` | yes | 0 | 3 | -- |
| `GL_NVX_blend_equation_advanced_multi_draw_buffers` | yes | 0 | 0 | -- |
| `GL_NVX_conditional_render` | yes | 2 | 0 | -- |
| `GL_NVX_cross_process_interop` | no | 0 | 1 | -- |
| `GL_NVX_gpu_memory_info` | yes | 0 | 5 | -- |
| `GL_NVX_gpu_multicast2` | yes | 6 | 1 | -- |
| `GL_NVX_linked_gpu_multicast` | yes | 3 | 2 | -- |
| `GL_NVX_progress_fence` | yes | 4 | 0 | -- |
| `GL_NV_alpha_to_coverage_dither_control` | yes | 1 | 4 | -- |
| `GL_NV_bindless_multi_draw_indirect` | yes | 2 | 0 | -- |
| `GL_NV_bindless_multi_draw_indirect_count` | yes | 2 | 0 | -- |
| `GL_NV_bindless_texture` | yes | 13 | 0 | -- |
| `GL_NV_blend_equation_advanced` | yes | 2 | 51 | -- |
| `GL_NV_blend_equation_advanced_coherent` | yes | 0 | 1 | -- |
| `GL_NV_blend_minmax_factor` | yes | 0 | 2 | -- |
| `GL_NV_blend_square` | yes | 0 | 0 | -- |
| `GL_NV_clip_space_w_scaling` | yes | 1 | 3 | -- |
| `GL_NV_command_list` | yes | 17 | 19 | -- |
| `GL_NV_compute_program5` | yes | 0 | 2 | -- |
| `GL_NV_compute_shader_derivatives` | yes | 0 | 0 | -- |
| `GL_NV_conditional_render` | yes | 2 | 4 | -- |
| `GL_NV_conservative_raster` | yes | 1 | 4 | -- |
| `GL_NV_conservative_raster_dilate` | yes | 1 | 3 | -- |
| `GL_NV_conservative_raster_pre_snap` | yes | 0 | 1 | -- |
| `GL_NV_conservative_raster_pre_snap_triangles` | yes | 1 | 3 | -- |
| `GL_NV_conservative_raster_underestimation` | yes | 0 | 0 | -- |
| `GL_NV_copy_buffer` | no | 1 | 2 | -- |
| `GL_NV_copy_depth_to_color` | yes | 0 | 2 | -- |
| `GL_NV_copy_image` | yes | 1 | 0 | -- |
| `GL_NV_coverage_sample` | no | 2 | 9 | -- |
| `GL_NV_deep_texture3D` | yes | 0 | 2 | -- |
| `GL_NV_depth_buffer_float` | yes | 3 | 4 | -- |
| `GL_NV_depth_clamp` | yes | 0 | 1 | -- |
| `GL_NV_depth_nonlinear` | no | 0 | 1 | -- |
| `GL_NV_draw_buffers` | no | 1 | 33 | -- |
| `GL_NV_draw_instanced` | no | 2 | 0 | -- |
| `GL_NV_draw_texture` | yes | 1 | 0 | -- |
| `GL_NV_draw_vulkan_image` | yes | 5 | 0 | -- |
| `GL_NV_evaluators` | yes | 9 | 24 | -- |
| `GL_NV_explicit_attrib_location` | no | 0 | 0 | -- |
| `GL_NV_explicit_multisample` | yes | 3 | 10 | -- |
| `GL_NV_fbo_color_attachments` | no | 0 | 17 | -- |
| `GL_NV_fence` | yes | 7 | 3 | -- |
| `GL_NV_fill_rectangle` | yes | 0 | 1 | -- |
| `GL_NV_float_buffer` | yes | 0 | 15 | -- |
| `GL_NV_fog_distance` | yes | 0 | 4 | -- |
| `GL_NV_fragment_coverage_to_color` | yes | 1 | 2 | -- |
| `GL_NV_fragment_program` | yes | 6 | 6 | -- |
| `GL_NV_fragment_program2` | yes | 0 | 5 | -- |
| `GL_NV_fragment_program4` | yes | 0 | 0 | -- |
| `GL_NV_fragment_program_option` | yes | 0 | 0 | -- |
| `GL_NV_fragment_shader_barycentric` | yes | 0 | 0 | -- |
| `GL_NV_fragment_shader_interlock` | yes | 0 | 0 | -- |
| `GL_NV_framebuffer_blit` | no | 1 | 4 | -- |
| `GL_NV_framebuffer_mixed_samples` | yes | 4 | 14 | -- |
| `GL_NV_framebuffer_multisample` | no | 1 | 3 | -- |
| `GL_NV_framebuffer_multisample_coverage` | yes | 1 | 4 | -- |
| `GL_NV_generate_mipmap_sRGB` | no | 0 | 0 | -- |
| `GL_NV_geometry_program4` | yes | 4 | 16 | -- |
| `GL_NV_geometry_shader4` | yes | 0 | 0 | -- |
| `GL_NV_geometry_shader_passthrough` | yes | 0 | 0 | -- |
| `GL_NV_gpu_multicast` | yes | 12 | 5 | -- |
| `GL_NV_gpu_program4` | yes | 16 | 8 | -- |
| `GL_NV_gpu_program5` | yes | 2 | 8 | -- |
| `GL_NV_gpu_program5_mem_extended` | yes | 0 | 0 | -- |
| `GL_NV_gpu_shader5` | yes | 33 | 29 | -- |
| `GL_NV_half_float` | yes | 46 | 1 | -- |
| `GL_NV_image_formats` | no | 0 | 0 | -- |
| `GL_NV_instanced_arrays` | no | 1 | 1 | -- |
| `GL_NV_internalformat_sample_query` | yes | 1 | 7 | -- |
| `GL_NV_light_max_exponent` | yes | 0 | 2 | -- |
| `GL_NV_memory_attachment` | yes | 6 | 10 | -- |
| `GL_NV_memory_object_sparse` | yes | 4 | 0 | -- |
| `GL_NV_mesh_shader` | yes | 4 | 48 | -- |
| `GL_NV_multisample_coverage` | yes | 0 | 2 | -- |
| `GL_NV_multisample_filter_hint` | yes | 0 | 1 | -- |
| `GL_NV_non_square_matrices` | no | 6 | 6 | -- |
| `GL_NV_occlusion_query` | yes | 7 | 4 | -- |
| `GL_NV_pack_subimage` | no | 0 | 3 | -- |
| `GL_NV_packed_depth_stencil` | yes | 0 | 2 | -- |
| `GL_NV_parameter_buffer_object` | yes | 3 | 5 | -- |
| `GL_NV_parameter_buffer_object2` | yes | 0 | 0 | -- |
| `GL_NV_path_rendering` | yes | 83 | 164 | -- |
| `GL_NV_path_rendering_shared_edge` | yes | 0 | 1 | -- |
| `GL_NV_pixel_buffer_object` | no | 0 | 4 | -- |
| `GL_NV_pixel_data_range` | yes | 2 | 6 | -- |
| `GL_NV_point_sprite` | yes | 2 | 3 | -- |
| `GL_NV_polygon_mode` | no | 1 | 6 | -- |
| `GL_NV_present_video` | yes | 6 | 6 | -- |
| `GL_NV_primitive_restart` | yes | 2 | 2 | -- |
| `GL_NV_primitive_shading_rate` | yes | 0 | 2 | -- |
| `GL_NV_query_resource` | yes | 1 | 6 | -- |
| `GL_NV_query_resource_tag` | yes | 3 | 0 | -- |
| `GL_NV_read_buffer` | no | 1 | 1 | -- |
| `GL_NV_read_buffer_front` | no | 0 | 0 | -- |
| `GL_NV_read_depth` | no | 0 | 0 | -- |
| `GL_NV_read_depth_stencil` | no | 0 | 0 | -- |
| `GL_NV_read_stencil` | no | 0 | 0 | -- |
| `GL_NV_register_combiners` | yes | 13 | 56 | -- |
| `GL_NV_register_combiners2` | yes | 2 | 1 | -- |
| `GL_NV_representative_fragment_test` | yes | 0 | 1 | -- |
| `GL_NV_robustness_video_memory_purge` | yes | 0 | 1 | -- |
| `GL_NV_sRGB_formats` | no | 0 | 10 | -- |
| `GL_NV_sample_locations` | yes | 3 | 8 | -- |
| `GL_NV_sample_mask_override_coverage` | yes | 0 | 0 | -- |
| `GL_NV_scissor_exclusive` | yes | 2 | 2 | -- |
| `GL_NV_shader_atomic_counters` | yes | 0 | 0 | -- |
| `GL_NV_shader_atomic_float` | yes | 0 | 0 | -- |
| `GL_NV_shader_atomic_float64` | yes | 0 | 0 | -- |
| `GL_NV_shader_atomic_fp16_vector` | yes | 0 | 0 | -- |
| `GL_NV_shader_atomic_int64` | yes | 0 | 0 | -- |
| `GL_NV_shader_buffer_load` | yes | 14 | 3 | -- |
| `GL_NV_shader_buffer_store` | yes | 0 | 3 | -- |
| `GL_NV_shader_noperspective_interpolation` | no | 0 | 0 | -- |
| `GL_NV_shader_storage_buffer_object` | yes | 0 | 0 | -- |
| `GL_NV_shader_subgroup_partitioned` | yes | 0 | 1 | -- |
| `GL_NV_shader_texture_footprint` | yes | 0 | 0 | -- |
| `GL_NV_shader_thread_group` | yes | 0 | 3 | -- |
| `GL_NV_shader_thread_shuffle` | yes | 0 | 0 | -- |
| `GL_NV_shading_rate_image` | yes | 7 | 21 | -- |
| `GL_NV_shadow_samplers_array` | no | 0 | 1 | -- |
| `GL_NV_shadow_samplers_cube` | no | 0 | 1 | -- |
| `GL_NV_stereo_view_rendering` | yes | 0 | 0 | -- |
| `GL_NV_tessellation_program5` | yes | 0 | 5 | -- |
| `GL_NV_texgen_emboss` | yes | 0 | 3 | -- |
| `GL_NV_texgen_reflection` | yes | 0 | 2 | -- |
| `GL_NV_texture_barrier` | yes | 1 | 0 | -- |
| `GL_NV_texture_border_clamp` | no | 0 | 2 | -- |
| `GL_NV_texture_compression_s3tc_update` | no | 0 | 0 | -- |
| `GL_NV_texture_compression_vtc` | yes | 0 | 0 | -- |
| `GL_NV_texture_env_combine4` | yes | 0 | 5 | -- |
| `GL_NV_texture_expand_normal` | yes | 0 | 1 | -- |
| `GL_NV_texture_multisample` | yes | 6 | 2 | -- |
| `GL_NV_texture_npot_2D_mipmap` | no | 0 | 0 | -- |
| `GL_NV_texture_rectangle` | yes | 0 | 4 | -- |
| `GL_NV_texture_rectangle_compressed` | yes | 0 | 0 | -- |
| `GL_NV_texture_shader` | yes | 0 | 73 | -- |
| `GL_NV_texture_shader2` | yes | 0 | 1 | -- |
| `GL_NV_texture_shader3` | yes | 0 | 17 | -- |
| `GL_NV_timeline_semaphore` | yes | 3 | 5 | -- |
| `GL_NV_transform_feedback` | yes | 12 | 31 | -- |
| `GL_NV_transform_feedback2` | yes | 7 | 4 | -- |
| `GL_NV_uniform_buffer_std430_layout` | yes | 0 | 0 | -- |
| `GL_NV_uniform_buffer_unified_memory` | yes | 0 | 3 | -- |
| `GL_NV_vdpau_interop` | yes | 10 | 4 | -- |
| `GL_NV_vdpau_interop2` | yes | 1 | 0 | -- |
| `GL_NV_vertex_array_range` | yes | 2 | 5 | -- |
| `GL_NV_vertex_array_range2` | yes | 0 | 1 | -- |
| `GL_NV_vertex_attrib_integer_64bit` | yes | 19 | 2 | -- |
| `GL_NV_vertex_buffer_unified_memory` | yes | 12 | 25 | -- |
| `GL_NV_vertex_program` | yes | 64 | 83 | -- |
| `GL_NV_vertex_program1_1` | yes | 0 | 0 | -- |
| `GL_NV_vertex_program2` | yes | 0 | 0 | -- |
| `GL_NV_vertex_program2_option` | yes | 0 | 2 | -- |
| `GL_NV_vertex_program3` | yes | 0 | 1 | -- |
| `GL_NV_vertex_program4` | yes | 23 | 1 | -- |
| `GL_NV_video_capture` | yes | 12 | 29 | -- |
| `GL_NV_viewport_array` | no | 12 | 8 | -- |
| `GL_NV_viewport_array2` | yes | 0 | 0 | -- |
| `GL_NV_viewport_swizzle` | yes | 1 | 12 | -- |
| `GL_OES_EGL_image` | no | 2 | 0 | -- |
| `GL_OES_EGL_image_external` | no | 1 | 4 | -- |
| `GL_OES_EGL_image_external_essl3` | no | 0 | 0 | -- |
| `GL_OES_blend_equation_separate` | no | 1 | 2 | -- |
| `GL_OES_blend_func_separate` | no | 1 | 4 | -- |
| `GL_OES_blend_subtract` | no | 1 | 4 | -- |
| `GL_OES_byte_coordinates` | yes | 22 | 1 | -- |
| `GL_OES_compressed_ETC1_RGB8_sub_texture` | no | 0 | 0 | -- |
| `GL_OES_compressed_ETC1_RGB8_texture` | no | 0 | 1 | -- |
| `GL_OES_compressed_paletted_texture` | yes | 0 | 10 | -- |
| `GL_OES_copy_image` | no | 1 | 0 | -- |
| `GL_OES_depth24` | no | 0 | 1 | -- |
| `GL_OES_depth32` | no | 0 | 1 | -- |
| `GL_OES_depth_texture` | no | 0 | 3 | -- |
| `GL_OES_draw_buffers_indexed` | no | 8 | 28 | -- |
| `GL_OES_draw_elements_base_vertex` | no | 4 | 0 | -- |
| `GL_OES_draw_texture` | no | 8 | 1 | -- |
| `GL_OES_element_index_uint` | no | 0 | 1 | -- |
| `GL_OES_extended_matrix_palette` | no | 0 | 0 | -- |
| `GL_OES_fbo_render_mipmap` | no | 0 | 0 | -- |
| `GL_OES_fixed_point` | yes | 107 | 1 | -- |
| `GL_OES_fragment_precision_high` | no | 0 | 0 | -- |
| `GL_OES_framebuffer_object` | no | 15 | 33 | -- |
| `GL_OES_geometry_point_size` | no | 0 | 0 | -- |
| `GL_OES_geometry_shader` | no | 1 | 33 | -- |
| `GL_OES_get_program_binary` | no | 2 | 3 | -- |
| `GL_OES_gpu_shader5` | no | 0 | 0 | -- |
| `GL_OES_mapbuffer` | no | 3 | 4 | -- |
| `GL_OES_matrix_get` | no | 0 | 3 | -- |
| `GL_OES_matrix_palette` | no | 4 | 16 | -- |
| `GL_OES_packed_depth_stencil` | no | 0 | 3 | -- |
| `GL_OES_point_size_array` | no | 1 | 5 | -- |
| `GL_OES_point_sprite` | no | 0 | 2 | -- |
| `GL_OES_primitive_bounding_box` | no | 1 | 1 | -- |
| `GL_OES_query_matrix` | yes | 1 | 0 | -- |
| `GL_OES_read_format` | yes | 0 | 2 | -- |
| `GL_OES_required_internalformat` | no | 0 | 15 | -- |
| `GL_OES_rgb8_rgba8` | no | 0 | 2 | -- |
| `GL_OES_sample_shading` | no | 1 | 2 | -- |
| `GL_OES_sample_variables` | no | 0 | 0 | -- |
| `GL_OES_shader_image_atomic` | no | 0 | 0 | -- |
| `GL_OES_shader_io_blocks` | no | 0 | 0 | -- |
| `GL_OES_shader_multisample_interpolation` | no | 0 | 3 | -- |
| `GL_OES_single_precision` | yes | 6 | 0 | -- |
| `GL_OES_standard_derivatives` | no | 0 | 1 | -- |
| `GL_OES_stencil1` | no | 0 | 1 | -- |
| `GL_OES_stencil4` | no | 0 | 1 | -- |
| `GL_OES_stencil8` | no | 0 | 1 | -- |
| `GL_OES_stencil_wrap` | no | 0 | 2 | -- |
| `GL_OES_surfaceless_context` | no | 0 | 1 | -- |
| `GL_OES_tessellation_point_size` | no | 0 | 0 | -- |
| `GL_OES_tessellation_shader` | no | 1 | 47 | -- |
| `GL_OES_texture_3D` | no | 6 | 6 | -- |
| `GL_OES_texture_border_clamp` | no | 8 | 2 | -- |
| `GL_OES_texture_buffer` | no | 2 | 14 | -- |
| `GL_OES_texture_compression_astc` | no | 0 | 48 | -- |
| `GL_OES_texture_cube_map` | no | 9 | 13 | -- |
| `GL_OES_texture_cube_map_array` | no | 0 | 9 | -- |
| `GL_OES_texture_env_crossbar` | no | 0 | 0 | -- |
| `GL_OES_texture_float` | no | 0 | 1 | -- |
| `GL_OES_texture_float_linear` | no | 0 | 0 | -- |
| `GL_OES_texture_half_float` | no | 0 | 1 | -- |
| `GL_OES_texture_half_float_linear` | no | 0 | 0 | -- |
| `GL_OES_texture_mirrored_repeat` | no | 0 | 1 | -- |
| `GL_OES_texture_npot` | no | 0 | 0 | -- |
| `GL_OES_texture_stencil8` | no | 0 | 2 | -- |
| `GL_OES_texture_storage_multisample_2d_array` | no | 1 | 5 | -- |
| `GL_OES_texture_view` | no | 1 | 5 | -- |
| `GL_OES_vertex_array_object` | no | 4 | 1 | -- |
| `GL_OES_vertex_half_float` | no | 0 | 1 | -- |
| `GL_OES_vertex_type_10_10_10_2` | no | 0 | 2 | -- |
| `GL_OES_viewport_array` | no | 12 | 8 | -- |
| `GL_OML_interlace` | yes | 0 | 2 | -- |
| `GL_OML_resample` | yes | 0 | 6 | -- |
| `GL_OML_subsample` | yes | 0 | 2 | -- |
| `GL_OVR_multiview` | yes | 2 | 4 | -- |
| `GL_OVR_multiview2` | yes | 0 | 0 | -- |
| `GL_OVR_multiview_multisampled_render_to_texture` | no | 1 | 0 | -- |
| `GL_PGI_misc_hints` | yes | 1 | 20 | -- |
| `GL_PGI_vertex_hints` | yes | 0 | 22 | -- |
| `GL_QCOM_YUV_texture_gather` | no | 0 | 0 | -- |
| `GL_QCOM_alpha_test` | no | 1 | 3 | -- |
| `GL_QCOM_binning_control` | no | 0 | 4 | -- |
| `GL_QCOM_driver_control` | no | 4 | 0 | -- |
| `GL_QCOM_extended_get` | no | 8 | 11 | -- |
| `GL_QCOM_extended_get2` | no | 4 | 0 | -- |
| `GL_QCOM_frame_extrapolation` | no | 1 | 0 | -- |
| `GL_QCOM_framebuffer_foveated` | no | 2 | 2 | -- |
| `GL_QCOM_motion_estimation` | no | 2 | 3 | -- |
| `GL_QCOM_perfmon_global_mode` | no | 0 | 1 | -- |
| `GL_QCOM_render_sRGB_R8_RG8` | no | 0 | 0 | -- |
| `GL_QCOM_render_shared_exponent` | no | 0 | 0 | -- |
| `GL_QCOM_shader_framebuffer_fetch_noncoherent` | no | 1 | 1 | -- |
| `GL_QCOM_shader_framebuffer_fetch_rate` | no | 0 | 0 | -- |
| `GL_QCOM_shading_rate` | no | 1 | 8 | -- |
| `GL_QCOM_texture_foveated` | no | 1 | 7 | -- |
| `GL_QCOM_texture_foveated2` | no | 0 | 1 | -- |
| `GL_QCOM_texture_foveated_subsampled_layout` | no | 0 | 2 | -- |
| `GL_QCOM_texture_lod_bias` | no | 0 | 1 | -- |
| `GL_QCOM_tiled_rendering` | no | 2 | 32 | -- |
| `GL_QCOM_writeonly_rendering` | no | 0 | 1 | -- |
| `GL_QCOM_ycbcr_degamma` | no | 0 | 2 | -- |
| `GL_REND_screen_coordinates` | yes | 0 | 2 | -- |
| `GL_S3_s3tc` | yes | 0 | 6 | -- |
| `GL_SGIS_detail_texture` | yes | 2 | 8 | -- |
| `GL_SGIS_fog_function` | yes | 2 | 3 | -- |
| `GL_SGIS_generate_mipmap` | yes | 0 | 2 | -- |
| `GL_SGIS_multisample` | yes | 2 | 16 | -- |
| `GL_SGIS_pixel_texture` | yes | 6 | 4 | -- |
| `GL_SGIS_point_line_texgen` | yes | 0 | 8 | -- |
| `GL_SGIS_point_parameters` | yes | 2 | 4 | -- |
| `GL_SGIS_sharpen_texture` | yes | 2 | 4 | -- |
| `GL_SGIS_texture4D` | yes | 2 | 10 | -- |
| `GL_SGIS_texture_border_clamp` | yes | 0 | 1 | -- |
| `GL_SGIS_texture_color_mask` | yes | 1 | 1 | -- |
| `GL_SGIS_texture_edge_clamp` | yes | 0 | 1 | -- |
| `GL_SGIS_texture_filter4` | yes | 2 | 2 | -- |
| `GL_SGIS_texture_lod` | yes | 0 | 4 | -- |
| `GL_SGIS_texture_select` | yes | 0 | 22 | -- |
| `GL_SGIX_async` | yes | 6 | 1 | -- |
| `GL_SGIX_async_histogram` | yes | 0 | 2 | -- |
| `GL_SGIX_async_pixel` | yes | 0 | 6 | -- |
| `GL_SGIX_blend_alpha_minmax` | yes | 0 | 2 | -- |
| `GL_SGIX_calligraphic_fragment` | yes | 0 | 1 | -- |
| `GL_SGIX_clipmap` | yes | 0 | 12 | -- |
| `GL_SGIX_convolution_accuracy` | yes | 0 | 1 | -- |
| `GL_SGIX_depth_pass_instrument` | yes | 0 | 0 | -- |
| `GL_SGIX_depth_texture` | yes | 0 | 3 | -- |
| `GL_SGIX_flush_raster` | yes | 1 | 0 | -- |
| `GL_SGIX_fog_offset` | yes | 0 | 2 | -- |
| `GL_SGIX_fragment_lighting` | yes | 18 | 20 | -- |
| `GL_SGIX_framezoom` | yes | 1 | 3 | -- |
| `GL_SGIX_igloo_interface` | yes | 1 | 0 | -- |
| `GL_SGIX_instruments` | yes | 6 | 2 | -- |
| `GL_SGIX_interlace` | yes | 0 | 1 | -- |
| `GL_SGIX_ir_instrument1` | yes | 0 | 1 | -- |
| `GL_SGIX_list_priority` | yes | 6 | 1 | -- |
| `GL_SGIX_pixel_texture` | yes | 1 | 2 | -- |
| `GL_SGIX_pixel_tiles` | yes | 0 | 8 | -- |
| `GL_SGIX_polynomial_ffd` | yes | 4 | 6 | -- |
| `GL_SGIX_reference_plane` | yes | 1 | 2 | -- |
| `GL_SGIX_resample` | yes | 0 | 5 | -- |
| `GL_SGIX_scalebias_hint` | yes | 0 | 1 | -- |
| `GL_SGIX_shadow` | yes | 0 | 4 | -- |
| `GL_SGIX_shadow_ambient` | yes | 0 | 1 | -- |
| `GL_SGIX_sprite` | yes | 4 | 7 | -- |
| `GL_SGIX_subsample` | yes | 0 | 5 | -- |
| `GL_SGIX_tag_sample_buffer` | yes | 1 | 0 | -- |
| `GL_SGIX_texture_add_env` | yes | 0 | 1 | -- |
| `GL_SGIX_texture_coordinate_clamp` | yes | 0 | 3 | -- |
| `GL_SGIX_texture_lod_bias` | yes | 0 | 3 | -- |
| `GL_SGIX_texture_multi_buffer` | yes | 0 | 1 | -- |
| `GL_SGIX_texture_scale_bias` | yes | 0 | 4 | -- |
| `GL_SGIX_vertex_preclip` | yes | 0 | 2 | -- |
| `GL_SGIX_ycrcb` | yes | 0 | 2 | -- |
| `GL_SGIX_ycrcb_subsample` | yes | 0 | 0 | -- |
| `GL_SGIX_ycrcba` | yes | 0 | 2 | -- |
| `GL_SGI_color_matrix` | yes | 0 | 11 | -- |
| `GL_SGI_color_table` | yes | 7 | 16 | -- |
| `GL_SGI_texture_color_table` | yes | 0 | 2 | -- |
| `GL_SUNX_constant_data` | yes | 1 | 2 | -- |
| `GL_SUN_convolution_border_modes` | yes | 0 | 1 | -- |
| `GL_SUN_global_alpha` | yes | 8 | 2 | -- |
| `GL_SUN_mesh_array` | yes | 1 | 2 | -- |
| `GL_SUN_slice_accum` | yes | 0 | 1 | -- |
| `GL_SUN_triangle_list` | yes | 7 | 17 | -- |
| `GL_SUN_vertex` | yes | 40 | 0 | -- |
| `GL_VIV_shader_binary` | no | 0 | 1 | -- |
| `GL_WIN_phong_shading` | yes | 0 | 2 | -- |
| `GL_WIN_specular_fog` | yes | 0 | 1 | -- |

