# GLSL Version Matrix (1.10 - 4.60)

Generated file -- do not edit by hand.

Regenerate with:

```
python3 tools/gen_version_inventory.py --out data/version_inventory.json
python3 tools/gen_version_inventory.py --docs-from data/version_inventory.json \
        --gl-doc docs/OPENGL_VERSION_MATRIX.md \
        --glsl-doc docs/GLSL_VERSION_MATRIX.md
```

## Provenance

The per-version GLSL data lives in the `glsl` section of
`data/version_inventory.json`, which is written by
`tools/gen_version_inventory.py`.

Each feature bullet carries a provenance marker:

- **[PDF]** -- verified against the vendored specification PDF
  `docs/khronos/GLSLangSpec.4.60.pdf` (OpenGL Shading Language 4.60.8,
  2023-08-14). The section/page reference is given in parentheses.
- **[KNOWLEDGE]** -- **not** verifiable from the vendored PDF. The
  vendored PDF documents GLSL 4.60 only; its "Changes" section
  (sec. 1.1) reaches back to GLSL 4.50 revision 7. Everything about
  earlier GLSL versions in this document is historical knowledge and
  has **not** been checked against a primary source in this repository.

This document is a **specification inventory**. It makes **no claim
about CTS conformance and no claim about runtime behaviour** of any
implementation.

## GL <-> GLSL anchor table

| GLSL version | provided by OpenGL version |
| --- | --- |
| 1.10 | 2.0 |
| 1.20 | 2.1 |
| 1.30 | 3.0 |
| 1.40 | 3.1 |
| 1.50 | 3.2 |
| 3.30 | 3.3 |
| 4.00 | 4.0 |
| 4.10 | 4.1 |
| 4.20 | 4.2 |
| 4.30 | 4.3 |
| 4.40 | 4.4 |
| 4.50 | 4.5 |
| 4.60 | 4.6 |

OpenGL 1.0 - 1.5 have no GLSL version of their own; GLSL for those
releases exists only as extensions.

## Per-version detail

### GLSL 1.10

- **Provided by OpenGL 2.0**
- **Profiles**: none (pre-profile language)
- **Shader stages available**: vertex, fragment
- **Major language features**:

  - **[KNOWLEDGE]** First release of the OpenGL Shading Language; replaces the GL 1.1 fixed-function vertex/fragment program model.
  - **[KNOWLEDGE]** Two stages only: vertex and fragment.
  - **[KNOWLEDGE]** Qualifiers const, uniform, attribute, varying; types float, vec2-4, mat2-4, bool, sampler2D.
  - **[KNOWLEDGE]** Built-ins gl_Position, gl_FragColor, gl_FragCoord, gl_PointSize, gl_Normal, gl_Color, gl_TexCoord.
  - **[PDF]** No #version directive is required; a shader without one is treated as targeting version 1.10. (GLSL 4.60 spec, sec. 3.3, PDF p.19)
  - **[KNOWLEDGE]** No integer types and no geometry, tessellation, compute, image or buffer types.

### GLSL 1.20

- **Provided by OpenGL 2.1**
- **Profiles**: none (pre-profile language)
- **Shader stages available**: vertex, fragment
- **Major language features**:

  - **[KNOWLEDGE]** Integer scalar and vector types (int, uint, ivec2-4, uvec2-4) and integer uniforms.
  - **[KNOWLEDGE]** Non-square matrix types (mat2x3, mat3x2, mat2x4, mat4x2, ...).
  - **[KNOWLEDGE]** Array constructors, for example float[3](1.0, 2.0, 3.0).
  - **[KNOWLEDGE]** Stage set unchanged: vertex and fragment only.

### GLSL 1.30

- **Provided by OpenGL 3.0**
- **Profiles**: none (pre-profile language)
- **Shader stages available**: vertex, fragment
- **Major language features**:

  - **[KNOWLEDGE]** The #version directive becomes mandatory and the declared version must match the language in use.
  - **[KNOWLEDGE]** Bitwise operators on integers (& | ^ ~ << >>), ++/--, compound assignment, switch and do-while statements.
  - **[KNOWLEDGE]** Explicit-LOD and texel-fetch texture functions (textureLod, texelFetch).
  - **[KNOWLEDGE]** gl_FragDepth and gl_FrontFacing built-ins.
  - **[KNOWLEDGE]** Multiple render targets through gl_FragData[].
  - **[KNOWLEDGE]** Integer texture formats and their sampler types (isampler*/usampler*, ARB_texture_integer).
  - **[KNOWLEDGE]** Precision qualifiers lowp, mediump, highp.
  - **[KNOWLEDGE]** Stage set unchanged: vertex and fragment only.

### GLSL 1.40

- **Provided by OpenGL 3.1**
- **Profiles**: none (pre-profile language)
- **Shader stages available**: vertex, fragment, geometry
- **Major language features**:

  - **[KNOWLEDGE]** Geometry shader stage introduced (ARB_geometry_shader4, OpenGL 3.1).
  - **[KNOWLEDGE]** Uniform blocks with an explicit std140 layout.
  - **[KNOWLEDGE]** textureGather family and the textureOffset variants.
  - **[KNOWLEDGE]** samplerBuffer, for texture buffer objects (ARB_texture_buffer_object, OpenGL 3.1).

### GLSL 1.50

- **Provided by OpenGL 3.2**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry
- **Major language features**:

  - **[PDF]** First version that carries a profile argument: '#version 150 core' or '#version 150 compatibility'. A profile argument can only be used with version 150 or greater, and if no profile is given the default is core. (GLSL 4.60 spec, sec. 3.3, PDF p.19)
  - **[KNOWLEDGE]** 'in'/'out' replace 'attribute'/'varying'; the latter are deprecated but remain available in the compatibility profile.
  - **[KNOWLEDGE]** layout qualifiers, including layout(location=) for fragment outputs.
  - **[KNOWLEDGE]** Separate sampler objects are distinguished from combined sampler/image uniform slots.
  - **[PDF]** gl_FragColor and gl_FragData exist only in the compatibility profile. (GLSL 4.60 spec, sec. 7.1/7.2, PDF p.156)
  - **[KNOWLEDGE]** Tessellation shaders are NOT part of this version; they arrive with GLSL 4.00.

### GLSL 3.30

- **Provided by OpenGL 3.3**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry
- **Major language features**:

  - **[KNOWLEDGE]** Version 330: the language version required by the OpenGL 3.3 core profile.
  - **[KNOWLEDGE]** 'in'/'out' everywhere; 'attribute'/'varying' are no longer part of the core language.
  - **[KNOWLEDGE]** layout(location=) extended to vertex inputs and outputs (ARB_enhanced_layouts).
  - **[KNOWLEDGE]** gl_VertexID and gl_InstanceID built-ins.
  - **[KNOWLEDGE]** Stage set unchanged from 1.50: vertex, fragment and geometry. Tessellation shaders arrive with GLSL 4.00.

### GLSL 4.00

- **Provided by OpenGL 4.0**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation
- **Major language features**:

  - **[KNOWLEDGE]** Tessellation control and tessellation evaluation shader stages introduced (ARB_tessellation_shader, OpenGL 4.0).
  - **[KNOWLEDGE]** 64-bit integer types int64_t and uint64_t (ARB_gpu_shader_int64).
  - **[KNOWLEDGE]** Double-precision floating point: double, dvec2-4, dmat2-4 (ARB_gpu_shader_fp64).
  - **[KNOWLEDGE]** Texture LOD query functions (textureQueryLod, textureQueryLevels).
  - **[KNOWLEDGE]** Transform feedback 3.0: multiple streams and interleaved queries.

### GLSL 4.10

- **Provided by OpenGL 4.1**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation
- **Major language features**:

  - **[KNOWLEDGE]** gl_ViewportIndex built-in (ARB_viewport_array).
  - **[KNOWLEDGE]** Separate sampler objects: 'sampler' is a distinct opaque type from the texture sampler types.
  - **[KNOWLEDGE]** No new shader stage.

### GLSL 4.20

- **Provided by OpenGL 4.2**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation
- **Major language features**:

  - **[KNOWLEDGE]** Image types (image1D..imageBuffer, iimage*, uimage*) with imageLoad/imageStore/imageAtomic* (ARB_shader_image_load_store).
  - **[KNOWLEDGE]** Atomic counters (atomic_uint) with atomicCounterIncrement, atomicCounterDecrement and friends (ARB_shader_atomic_counters).
  - **[KNOWLEDGE]** Memory qualifiers coherent, volatile, restrict, readonly, writeonly on image and buffer variables.
  - **[KNOWLEDGE]** No new shader stage.

### GLSL 4.30

- **Provided by OpenGL 4.3**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation, compute
- **Major language features**:

  - **[KNOWLEDGE]** Compute shader stage introduced (OpenGL 4.3) with local_size_x/y/z, shared variables, memoryBarrier and group-shared storage.
  - **[KNOWLEDGE]** Shader storage buffer objects (layout(std430) buffer variables).
  - **[KNOWLEDGE]** Subroutines: the 'subroutine' keyword, subroutine types and subroutine uniforms.
  - **[KNOWLEDGE]** Program interface queries: named uniforms and uniform blocks with resource locations.
  - **[KNOWLEDGE]** Image load/store extended to the vertex, tessellation and geometry stages.

### GLSL 4.40

- **Provided by OpenGL 4.4**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation, compute
- **Major language features**:

  - **[KNOWLEDGE]** Extended layout qualifier set on uniforms and blocks (binding, offset, align, ...).
  - **[KNOWLEDGE]** Dynamic indexing of arrays of samplers and of uniform arrays.
  - **[KNOWLEDGE]** ARB_gpu_shader5 arithmetic and layout extensions.
  - **[KNOWLEDGE]** No new shader stage.

### GLSL 4.50

- **Provided by OpenGL 4.5**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation, compute
- **Major language features**:

  - **[KNOWLEDGE]** Cull distance built-ins (gl_CullDistance, ARB_clip_cull_distance).
  - **[PDF]** The 4.50 revision does not yet contain ARB_shader_atomic_counter_ops, ARB_shader_draw_parameters, ARB_shader_group_vote or ARB_gl_spirv: the 4.60 revision summary lists all four as newly incorporated. (GLSL 4.60 spec, sec. 1.1.6, PDF p.8)
  - **[KNOWLEDGE]** No new shader stage.

### GLSL 4.60

- **Provided by OpenGL 4.6**
- **Profiles**: core and compatibility
- **Shader stages available**: vertex, fragment, geometry, tessellation control, tessellation evaluation, compute
- **Major language features**:

  - **[PDF]** '#version 460' is required in any shader that uses version 4.60 of the language, and __VERSION__ substitutes the decimal integer 460. (GLSL 4.60 spec, sec. 3.3, PDF p.17 and p.19)
  - **[PDF]** Incorporates GL_ARB_shader_atomic_counter_ops, GL_ARB_shader_draw_parameters, GL_ARB_shader_group_vote and GL_ARB_gl_spirv. (GLSL 4.60 spec, sec. 1.1.6, PDF p.8)
  - **[PDF]** Six stages: vertex, tessellation control, tessellation evaluation, geometry, fragment and compute. (GLSL 4.60 spec, Chapter 2, PDF p.11)
  - **[PDF]** The compatibility profile adds the fixed-function fragment outputs gl_FragColor and gl_FragData and the fixed-function vertex inputs gl_Color, gl_SecondaryColor, gl_Normal, gl_Vertex and gl_MultiTexCoord0-7. (GLSL 4.60 spec, sec. 7.1/7.2, PDF p.156)
  - **[PDF]** When compiling for OpenGL SPIR-V the macro GL_SPIRV is predefined as 100. (GLSL 4.60 spec, PDF p.22)
  - **[PDF]** Compatibility-profile features are not available when generating SPIR-V. (GLSL 4.60 spec, sec. 3.3, PDF p.19)

## Stage availability at a glance

| GLSL version | vertex | fragment | geometry | tessellation control | tessellation evaluation | compute |
| --- | --- | --- | --- | --- | --- | --- |
| 1.10 | yes | yes | -- | -- | -- | -- |
| 1.20 | yes | yes | -- | -- | -- | -- |
| 1.30 | yes | yes | -- | -- | -- | -- |
| 1.40 | yes | yes | yes | -- | -- | -- |
| 1.50 | yes | yes | yes | -- | -- | -- |
| 3.30 | yes | yes | yes | -- | -- | -- |
| 4.00 | yes | yes | yes | yes | yes | -- |
| 4.10 | yes | yes | yes | yes | yes | -- |
| 4.20 | yes | yes | yes | yes | yes | -- |
| 4.30 | yes | yes | yes | yes | yes | yes |
| 4.40 | yes | yes | yes | yes | yes | yes |
| 4.50 | yes | yes | yes | yes | yes | yes |
| 4.60 | yes | yes | yes | yes | yes | yes |

Stage list and the six-stage pipeline wording are **[PDF]**-verified
(`docs/khronos/GLSLangSpec.4.60.pdf`, Chapter 2, p.11): "Currently,
these processors are the vertex, tessellation control, tessellation
evaluation, geometry, fragment, and compute processors."

