# FEATURE_INVENTORY.md — TGLMT

Ma trận tính năng OpenGL 1.0–4.6 × Core/Compatibility. Dữ liệu lệnh sinh tự động từ
`data/version_inventory.json` (Khronos gl.xml). Trạng thái implementation theo source
xem xét (không tự ýDeclare). Nhãn: NATIVE / EMULATED / PARTIAL / HARDWARE_LIMITED /
UNSUPPORTED / NOT_TESTED (theo AGENTS.md §6).

## 1. Tổng quan lệnh (tự động từ gl.xml)

| Profile | Tổng lệnh | Đã prototype+gate | Thực sự implement + test | Stub (UNIMPLEMENTED) |
|---------|-----------|-------------------|--------------------------|----------------------|
| Core 4.6 | 657 | 657 | Chưa kê chi tiết từng lệnh | Chưa kê |
| Compatibility 4.6 | 1048 | 1048 | 109 (matrix 26 + immediate 83) | 240 |

- Lệnh introduced theo version: 1.0:306, 1.1:30, 1.2:4, 1.3:46, 1.4:47, 1.5:19, 2.0:93, 2.1:6, 3.0:84, 3.1:12, 3.2:19, 3.3:58, 4.0:46, 4.1:88, 4.2:12, 4.3:43, 4.4:9, 4.5:122, 4.6:4.
- Core-only: 657; Compat-only: 391; cả hai: 657.

## 2. Ma trận version × profile (theo registry)

| Version | Core available | Compat available | Nhắc nhở |
|---------|----------------|------------------|----------|
| 1.0 | 0 | 306 | Legacy (chưa có profile) |
| 1.1 | 0 | 30 | |
| 1.2 | 0 | 4 | |
| 1.3 | 0 | 46 | |
| 1.4 | 0 | 47 | |
| 1.5 | 0 | 19 | |
| 2.0 | 0 | 93 | Shader đầu tiên |
| 2.1 | 0 | 6 | |
| 3.0 | 84 | 84 | VAO, FBO, integer attributes, instancing, UBO, sampler, sync, sRGB |
| 3.1 | 12 | 12 | |
| 3.2 | 19 | 19 | Profile split bắt đầu ở 3.2 |
| 3.3 | 58 | 58 | |
| 4.0 | 46 | 46 | Tessellation |
| 4.1 | 88 | 88 | Separable shader objects |
| 4.2 | 12 | 12 | |
| 4.3 | 43 | 43 | Compute, shader storage, images, atomic counters |
| 4.4 | 9 | 9 | |
| 4.5 | 122 | 122 | DSA, robustness |
| 4.6 | 4 | 4 | SPIR-V-related |

## 3. Trạng thái theo nhóm chức năng

| Nhóm | Lệnh | Trạng thái | Nhãn | Bằng chứng file:line | Test ID |
|------|------|------------|------|----------------------|---------|
| Context/version/gate | ~15 | Implement thật | NATIVE | `src/gl/gl_context_api.cpp`, `src/core/VersionGate.cpp`, `src/gl/gl_version_gate_gen.cpp` | test_context_version |
| Error queue | glGetError | Implement thật | NATIVE | `src/core/ErrorTracker.cpp` | test_regression_gl_errors |
| Object lifecycle | glGen/Delete*, glIs* | Implement thật | NATIVE | `src/core/ObjectRegistry.cpp` | — |
| Buffer | glBindBuffer, glBufferData/Storage, glMapBuffer, ... | Một phần | PARTIAL | `src/gl/gl_buffer.cpp` | test_core_buffer |
| Vertex attrib / VAO | glVertexAttrib*, glBindVertexArray | Một phần | PARTIAL | `src/gl/gl_vertex.cpp` | test_core_vertex_attrib |
| Shader compile | glCreateShader, glShaderSource, glCompileShader | Converter text | PARTIAL | `src/gl/gl_shader.cpp`, `src/gl/glsl_to_msl.cpp` | test_glsl_coverage |
| Program link | glLinkProgram, glUseProgram, glAttach/DetachShader | Link + MTL compile | PARTIAL | `src/gl/gl_program.cpp` | test_shader_program |
| Program binary | glGetProgramBinary, glProgramBinary | GLSL source binary | NATIVE | `src/gl/gl_program.cpp` | — |
| Uniform | glUniform*, glGetUniform* | Một phần | PARTIAL | `src/gl/gl_uniform.cpp` | test_core_uniform_entrypoints |
| UBO | glBindBufferBase, uniform block | Một phần | PARTIAL | `src/gl/gl_uniform.cpp` | test_ubo_layout_binding |
| SSBO | shader storage buffer | Một phần | PARTIAL | `src/gl/gl_uniform.cpp` | test_ssbo_graphics_apple |
| Texture | glTexImage*, glTexParameter*, sampler | Một phần | PARTIAL | `src/gl/gl_texture.cpp`, `src/gl/gl_sampler.cpp` | test_texture_sampler |
| FBO | glBindFramebuffer, glFramebufferTexture*, blit | Một phần | PARTIAL | `src/gl/gl_framebuffer.cpp` | test_fbo_completeness |
| Draw | glDrawArrays, glDrawElements, MultiDraw | Một phần | PARTIAL | `src/gl/gl_draw.cpp` | test_core_draw |
| Compute | glDispatchCompute, compute shader | Kernel MSL | PARTIAL | `src/gl/gl_compute.cpp` | test_compute_dispatch |
| Geometry shader | GS emulation compute kernel | EMULATED (subset) | EMULATED | `src/gl/gl_geometry_emulation.cpp`, `docs/GEOMETRY_SHADER_EMULATION.md` | test_geometry_emulation |
| Tessellation | TCS/TES | PARTIAL (triangles/quads subset) | PARTIAL | `src/gl/gl_tess_xfb.cpp` | test_shader_tess |
| Transform feedback | XFB | PARTIAL (capture) | PARTIAL | `src/gl/gl_tess_xfb.cpp` | test_xfb_capture |
| Fixed-function matrix | glMatrixMode, glLoadMatrix, glRotate, ... | Implement thật | NATIVE | `src/gl/gl_legacy_matrix.cpp` | test_legacy_matrix |
| Fixed-function immediate | glBegin/glEnd, glVertex*, ... | Implement thật | NATIVE | `src/gl/gl_legacy_vertex.cpp` | test_legacy_immediate |
| Fixed-function lighting | glLight*, glMaterial*, ... | Implement thật | NATIVE | `src/gl/gl_legacy_light.cpp` | test_legacy_lighting |
| Fixed-function fog | glFog* | Implement thật | NATIVE | `src/gl/gl_legacy_fog.cpp` | test_legacy_fog |
| Fixed-function texenv | glTexEnv*, glTexGen* | Implement thật | NATIVE | `src/gl/gl_legacy_texenv.cpp` | test_legacy_texenv |
| Fixed-function raster | alpha test, clip plane, stipple, polygon mode | Implement thật | NATIVE | `src/gl/gl_legacy_raster.cpp`, `gl_legacy_alpha.cpp` | test_legacy_raster |
| Fixed-function draw path | MSL sinh fixed-function, LegacyDraw | PARTIAL (pixel chưa verify) | PARTIAL | `src/gl/gl_legacy_draw.cpp`, `src/metal/AppleMetalBridge.mm` | test_legacy_draw |
| Display list | glGenLists, glCallList, ... | Implement thật | NATIVE | `src/gl/gl_legacy_displaylist.cpp` | test_legacy_displaylist |
| Evaluator | glMap*, glEval* | Implement thật | NATIVE | `src/gl/gl_legacy_evaluator.cpp` | test_legacy_evaluator |
| Selection/feedback | glSelectBuffer, glRenderMode, ... | Implement thật | NATIVE | `src/gl/gl_legacy_selection.cpp` | test_legacy_selection |
| Client vertex array | glVertexPointer, glDrawArrays client | Implement thật | NATIVE | `src/gl/gl_legacy_clientarrays.cpp` | test_legacy_clientarrays |
| Pixel ops | glBitmap, glDrawPixels, glPixelTransfer* | Implement thật | NATIVE | `src/gl/gl_legacy_pixelops.cpp` | test_legacy_pixelops |
| Color index | glIndex*, material color-index | State only | PARTIAL | `src/gl/gl_legacy_colorindex.cpp` | test_legacy_colorindex |
| Attrib stack | glPushAttrib/PopAttrib | Implement thật | NATIVE | `src/gl/gl_legacy_attribstack.cpp` | test_legacy_attribstack |
| Accumulation buffer | glAccum, glClearAccum | State only (Metal không có) | HARDWARE_LIMITED | `src/gl/gl_legacy_colorindex.cpp` | test_legacy_colorindex |
| GLU | glu* | MISSING | MISSING | — | — |
| Debug (KHR_debug) | glDebugMessageControl, ... | Một phần | PARTIAL | `src/gl/gl_debug_p4.cpp` | test_debug_p4 |
| Synchronization | glFenceSync, glClientWaitSync | Một phần | PARTIAL | `src/gl/gl_sync_query.cpp` | test_core_query_sync |
| Query | glGenQuery, glBeginQuery, ... | Một phần | PARTIAL | `src/gl/gl_sync_query.cpp` | test_query_objects |
| Sampler objects | glGenSamplers, glSamplerParameter* | Một phần | PARTIAL | `src/gl/gl_sampler.cpp` | test_texture_sampler |
| Program interface query | glGetProgramInterface, ... | Một phần | PARTIAL | `src/gl/gl_program_interface_p10.cpp` | test_uniform_block_query |
| Separable program | glUseProgramStages, program pipeline | Một phần | PARTIAL | `src/gl/gl_program.cpp` | test_pipeline_sso |
| Subroutines | glUniformSubroutinesuiv | Một phần | PARTIAL | `src/gl/gl_program.cpp` | test_subroutine_api |
| SPIR-V ingestion | GL_ARB_gl_spirv | UNSUPPORTED (đường GLSL→MSL) | UNSUPPORTED | — | test_spirv_chain_apple |
| Robustness | GL_KHR_robustness | Chưa advertise đầy đủ | NOT_TESTED | — | — |
| Parallel shader compile | GL_ARB_parallel_shader_compile | Stub (số thread) | STUB | `src/gl/gl_get.cpp` | test_runtime_options |
| Bindless | GL_ARB_bindless_texture | Một phần | PARTIAL | `src/gl/gl_ext_bindless.cpp` | — |
| DSA | GL_ARB_direct_state_access | Một phần | PARTIAL | `src/gl/gl_ext_dsa_state.cpp`, `gl_ext_dsa_texture.cpp` | test_buffer_dsa_p5 |
| Imaging | glConvolutionFilter, glHistogram, ... | Một phần | PARTIAL | `src/gl/gl_ext_imaging.cpp` | test_compressed_contract |
| Path rendering | GL_NV_path_rendering | Một phần | PARTIAL | `src/gl/gl_ext_path_command.cpp` | — |
| Query performance | GL_ARB_perf ... | Một phần | PARTIAL | `src/gl/gl_ext_perf.cpp` | — |

## 4. GLSL version/profile matrix

| GLSL version | OpenGL version | Profile | Trạng thái converter | Nhãn | Test |
|--------------|----------------|---------|----------------------|------|------|
| 1.10 | 1.4 | — | Không | NOT_TESTED | — |
| 1.20 | 1.4 | Compat | Một phần (attribute/varying, built-in 1.20) | PARTIAL | test_glsl_legacy_syntax |
| 1.30 | 1.5 | Compat | Không | NOT_TESTED | — |
| 1.40 | 2.0 | Compat | Không | NOT_TESTED | — |
| 1.50 | 2.1 | Compat | Không | NOT_TESTED | — |
| 3.30 | 3.3 | Core+Compat | Không (converter chỉ hiểu 4.60 subset) | NOT_TESTED | test_glsl_legacy_syntax (chủ yếu ≤1.20) |
| 4.00–4.60 | 4.0–4.6 | Core | Subset (docs/GLSL_460_COMPILER.md) | PARTIAL | test_glsl_coverage |

## 5. Extension

- 1066 entry point trong bảng `getProcAddress` (TODO.md T0.5).
- 76 extension có hàm thật chưa implement (PROGRESS.md).
- Extension advertisement chưa đồng bộ (ISS-016).

## 6. Ghi chú nhãn

- "Implement thật" trong bảng trên = có code + test PASS trên backend Null. Chưa có nhãn `A11_DEVICE` vì máy xây dựng không có thiết bị iOS/A11.
- Đường draw fixed-function: code/state/trace đúng nhưng **pixel chưa verify trên Metal thật** ⇒ `PARTIAL`, không được ghi là hoàn chỉnh (ISS-B.3 trong TODO.md).
- Converter GLSL: convert OK ≠ ngữ nghĩa đúng. `test_glsl_coverage` yêu cầu MSL compile bằng `xcrun metal -c` nhưng KHÔNG chứng minh pixel equivalence.

---
*Tiếp theo: docs/compiler/ADR/ (kiến trúc compiler mới).*
