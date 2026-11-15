# MODERN_OPENGL_PIPELINE.md — Đường programmable pipeline của TGLMT (OpenGL 2.0 → 4.6)

Tài liệu kỹ thuật cho **Layer 4 (Modern OpenGL Pipeline)** của TGLMT: OpenGL → Metal
trên iOS / Apple A11, tự viết trong một tầng (không Mesa / Zink / MoltenVK / ANGLE —
`docs/DECISIONS.md` A1–A2).

**Nguồn spec:** `docs/khronos/glspec46.core.pdf` (OpenGL 4.6 Core Profile, 5 May 2022)
và `docs/khronos/gl.xml` (Khronos OpenGL-Registry). GLSL: `docs/khronos/GLSLangSpec.4.60.pdf`.

**Nhãn dùng trong tài liệu:**

| Nhãn | Nghĩa |
|---|---|
| **SPEC** | quy tắc theo đặc tả (kèm mục/bảng) |
| **CURRENT** | trạng thái thật của code trong repo lúc viết (kèm `file:line`) |
| **PLAN** | thiết kế đề xuất, chưa có trong code |

**Môi trường đo:** build `build-work/` (Debug), `TGLMT_APPLE_METAL=ON`, chạy trên
**Intel UHD Graphics 630** (macOS, Metal 3) → nhãn môi trường = **`OTHER_GPU`**,
**không phải** `A11_DEVICE`. Kết quả `ctest -j8`: **188/188 PASS** (đo lúc 2026-11-11).
Các test có hậu tố `_apple` chạy đường Metal thật trên GPU này; test không có hậu tố
chạy trên backend Null (`REFERENCE_ONLY` cho state machine).

---

## 0. Phạm vi và cách đo

| Nhóm | Nội dung | § |
|---|---|---|
| M2.x | GL 2.0 – 2.1 (shader/program object, uniform, vertex attrib, MRT) | §2 |
| M3.x | GL 3.0 – 3.3 (VAO, FBO, UBO, XFB, sampler object, sync, instancing) | §3 |
| M4.x | GL 4.0 – 4.6 (tessellation, compute, SSBO, image, subroutines, DSA, SPIR-V) | §4 |
| — | Kiến trúc đường đi (shader → program → link → uniform → VAO → FBO → draw → present) | §5 |
| — | Vertex input path | §6 |
| — | Framebuffer path | §7 |
| — | Compute path | §8 |
| — | Tessellation + geometry shader | §9 |
| — | Giới hạn Metal (A11) vs yêu cầu tối thiểu của GL | §10 |
| — | Điểm chưa verify / blocker | §11 |

### 0.1 Nguồn của mọi con số trong §2–§4

Con số **không** viết tay. Nguồn:

| Số | Cách đo | Script/kết quả |
|---|---|---|
| Entry point nào thuộc version nào | `data/version_inventory.json` → `commands[*].introduced_version` | `tools/gen_version_inventory.py`; tự kiểm tra 657 core / 1048 compat (`self_check.passed = true`) |
| Entry point nào là **stub trung thực** | đếm định nghĩa trong `src/gl/gl_stubs_gen.cpp` (240 hàm, đều `UNIMPLEMENTED:`) | `grep -c UNIMPLEMENTED src/gl/gl_stubs_gen.cpp` → **240** |
| Entry point nào **có code thật** | quét định nghĩa trong `src/gl/*.cpp` (loại `gl_stubs_gen.cpp`, `gl_c_exports.cpp`, `gl_dispatch_gen.cpp`, `gl_version_gate_gen.cpp`, `gl_ext_aliases_gen.cpp`) | phân tích tĩnh, đối chiếu `nm -gU build-work/libtglmt.dylib` → **596/596 symbol modern đã export** |
| Feature nào thuộc nhóm nào | bảng nhóm 81 dòng dưới đây, mỗi dòng là một tập entry point **rút ra từ inventory** | tổng 596 entry point = **đúng bằng** số command `introduced_version ≥ 2.0` trong inventory, **không thừa không thiếu** |

Kết quả đo được (2026-11-11):

```
GL 4.6 Core Profile  : 657 command
GL 4.6 Compatibility : 1048 command
Trong đó ≥ 2.0 (modern): 596 command
  ├─ 2.x: 99   (2.0: 93, 2.1: 6)
  ├─ 3.x: 172  (3.0: 84, 3.1: 12, 3.2: 19, 3.3: 57)
  └─ 4.x: 325  (4.0: 46, 4.1: 88, 4.2: 12, 4.3: 43, 4.4: 9, 4.5: 122, 4.6: 4)
Stub trong src/gl/gl_stubs_gen.cpp: 240 — TẤT CẢ đều là GL 1.0/1.1/1.3/1.4
  (173 + 16 + 17 + 34). Trong tập 596 modern: 0 stub.
Feature group (bảng §2–§4): 81 nhóm, phủ 596/596 entry point.
```

> **Đọc trạng thái.** "Có code thật" ≠ "đúng spec". Mỗi dòng bảng §2–§4 ghi rõ
> **mapping Metal** và **ghi chú hạn chế** nếu có. Ba nhóm bị giới hạn cứng bởi phần
> cứng Metal được nêu ở §9.3 (GS), §10 (viewport, fp64, atomics) và §11 (XFB,
> sync, indirect draw).

---

## 1. Bản đồ kiến trúc một lần đọc

```
                    ┌──────────────────────── Layer 1: C ABI (version-gated) ───────────────┐
                    │  src/gl/gl_c_exports.cpp  (1049 prototype extern "C", sinh tự động)   │
                    └───────────────────────────────────┬───────────────────────────────────┘
                                                        ▼
  ┌──────────────┐   glCreateShader/glShaderSource   ┌──────────────────────────────────┐
  │  app GLSL    │──────────────────────────────────▶│ glCompileShader → ConvertGLSLtoMSL│
  │  source      │                                   │ src/gl/gl_shader.cpp:56           │
  └──────────────┘                                   │ src/gl/glsl_to_msl.cpp:8351       │
                                                       └───────────────┬──────────────────┘
                                                                       │ MSL string
                                                                       ▼
  ┌──────────────┐   glCreateProgram/glAttachShader  ┌──────────────────────────────────┐
  │  program     │──────────────────────────────────▶│ glLinkProgram                    │
  │  object      │                                   │ src/gl/gl_program.cpp:393         │
  └──────────────┘                                   │  · vary/attrib/uniform matching   │
                                                       │  · Metal library compile          │
                                                       │  · PSO creation                   │
                                                       └───────────────┬──────────────────┘
                                                                       │ MTLLibrary + MTLRenderPipelineState
                                                                       ▼
  ┌──────────────┐   glUniform*/glProgramUniform*    ┌──────────────────────────────────┐
  │  uniform     │──────────────────────────────────▶│ glGetUniformLocation             │
  │  values      │                                   │ src/gl/gl_uniform.cpp:394         │
  └──────────────┘                                   │ StoreU → uniform bytes            │
                                                       └───────────────┬──────────────────┘
                                                                       ▼
  ┌──────────────┐   glGen/BindVertexArray           ┌──────────────────────────────────┐
  │  VAO         │──────────────────────────────────▶│ src/gl/gl_vertex.cpp:9/31        │
  │  + VBO/EBO   │   glVertexAttribPointer           │ VertexArrayObject{attribs,bindings}│
  │  + attribs   │──────────────────────────────────▶│ src/gl/gl_vertex.cpp:70          │
  └──────────────┘                                   └───────────────┬──────────────────┘
                                                                       │ Metal vertex descriptor
                                                                       ▼
  ┌──────────────┐   glGen/BindFramebuffer           ┌──────────────────────────────────┐
  │  FBO         │──────────────────────────────────▶│ src/gl/gl_framebuffer.cpp:29/56  │
  │  + renderbuf │   glFramebufferTexture2D          │ AttachToFBO → colorTex/depthTex   │
  │  + texture   │──────────────────────────────────▶│ src/gl/gl_framebuffer.cpp:218    │
  └──────────────┘                                   └───────────────┬──────────────────┘
                                                                       │ IRenderTarget
                                                                       ▼
  ┌──────────────┐   glDrawArrays/glDrawElements     ┌──────────────────────────────────┐
  │  draw        │──────────────────────────────────▶│ EmitDraw → AppleDrawGL            │
  │              │                                   │ src/gl/gl_draw.cpp:131 / 1774    │
  └──────────────┘                                   │  · pipeline-key cache             │
                                                       │  · setVertexBuffer/setSamplerState│
                                                       │  · drawPrimitives                 │
                                                       └───────────────┬──────────────────┘
                                                                       ▼
  ┌──────────────┐                                                        ┌────────────────┐
  │  present     │◀───────────────────────────────────────────────────────│ CAMetalLayer    │
  └──────────────┘   presentTarget (flip triangle)                        │ nextDrawable    │
                       src/metal/AppleMetalBridge.mm:2026                  └────────────────┘
```

Chi tiết compiler: `docs/SHADER_COMPILER.md`.

---

## 2. Bảng feature — GL 2.x (2.0 – 2.1)

| Feature | Version GL | Số EP | file:line (đại diện) | Test ID | Mapping Metal | Ghi chú |
|---|---|---|---|---|---|---|
| Shader objects (`glCreateShader` … `glGetShaderInfoLog`) | 2.0 | 11 | `src/gl/gl_shader.cpp`:56; `src/gl/gl_program.cpp`:175 | test_shader_program, test_core_registry | DIRECT | `glDeleteShader` chỉ xoá thật khi không còn program gắn (GL 4.6 §7.3) — `gl_shader.cpp`:17 |
| Program objects, link, reflection | 2.0 | 15 | `src/gl/gl_program.cpp`:146; `src/gl/gl_get.cpp`:884 | test_shader_program, test_validate_program | DIRECT | `glGetActiveUniform` trả type đúng (sửa bug 0x8B86→0x8B85) — `gl_get.cpp`:815 |
| Uniform API `glUniform*` | 2.0 | 19 | `src/gl/gl_uniform.cpp`:551 | test_core_uniform_entrypoints, test_regression_uniform_transpose | DIRECT | `transpose` được tôn trọng + đóng gói MSL (std140≠MSL) — `include/tglmt/uniform_pack.h` |
| Vertex attributes `glVertexAttrib*` | 2.0 | 36 | `src/gl/gl_vertex.cpp`:215; `src/gl/gl_readpixels_dsa.cpp`:564 | test_core_vertex_attrib | DIRECT | generic attribute đọc bằng `SetAttribValue` → shadow `GL_CURRENT_VERTEX_ATTRIB` (`gl_vertex.cpp`:202) |
| Vertex attrib pointer / enable / query | 2.0 | 7 | `src/gl/gl_vertex.cpp`:50; `src/gl/gl_get_extra.cpp`:891 | test_core_vertex_attrib, test_draw_triangle | DIRECT | xem §6 |
| Multiple render targets (`glDrawBuffers`) | 2.0 | 1 | `src/gl/gl_framebuffer.cpp`:562 | test_mrt_apple, test_fbo_completeness | DIRECT | 8 attachment Metal (`AppleMetalBridge.mm`:765) |
| Non-square matrix uniforms (`glUniformMatrixNxMfv`) | 2.1 | 6 | `src/gl/gl_uniform.cpp`:897 | test_core_uniform_entrypoints | DIRECT | pack theo cột MSL — `uniform_pack.h` |
| Separate stencil/blend state | 2.0 | 4 | `src/gl/gl_raster.cpp`:21 | test_core_raster_state, test_p9_extras | DIRECT | stencil 2 mặt: Metal chỉ set 1 reference → `PARTIAL` (xem §10) |

**Tổng 2.x: 8 feature, 99 entry point, 0 stub.**

---

## 3. Bảng feature — GL 3.x (3.0 – 3.3)

| Feature | Version GL | Số EP | file:line (đại diện) | Test ID | Mapping Metal | Ghi chú |
|---|---|---|---|---|---|---|
| Vertex Array Objects | 3.0 | 4 | `src/gl/gl_vertex.cpp`:31 | test_core_vertex_attrib, test_draw_triangle | DIRECT | xoá VAO đang bind → binding về 0 (`gl_vertex.cpp`:24) |
| Framebuffer Objects + renderbuffers | 3.0 | 18 | `src/gl/gl_framebuffer.cpp`:56; `src/gl/gl_get_extra.cpp`:759 | test_fbo_completeness, test_renderbuffer | DIRECT | completeness rules xem §7.2 |
| Indexed buffer binding (`BindBufferBase/Range`) | 3.0 | 2 | `src/gl/gl_buffer.cpp`:393 | test_core_buffer, test_mc_ubostage_apple | DIRECT | binding point riêng cho UBO/SSBO/atomic (`gl_buffer.cpp`:409) |
| Mapped buffer range | 3.0 | 2 | `src/gl/gl_buffer.cpp`:877 | test_core_buffer, test_buffer_dsa_p5 | HYBRID | shared storage + barrier; `glUnmapBuffer` luôn `GL_TRUE` |
| Integer vertex attributes (`glVertexAttribI*`) | 3.0 | 23 | `src/gl/gl_vertex.cpp`:223; `src/gl/gl_get_extra.cpp`:876 | test_core_vertex_attrib, test_sodium_vertex_apple | DIRECT | `isInt`/`isLong` flag tách khỏi `normalized` |
| Integer texture parameters (`glTexParameterI*`) | 3.0 | 4 | `src/gl/gl_ext_dsa_texture.cpp`:268 | test_core_texture_sampler | DIRECT | |
| Unsigned integer uniforms + `glGetUniformuiv` | 3.0 | 9 | `src/gl/gl_uniform.cpp`:572; `src/gl/gl_get.cpp`:1204 | test_core_uniform_entrypoints | DIRECT | |
| Transform feedback (entry point 3.0) | 3.0 | 4 | `src/gl/gl_tess_xfb.cpp`:55; `src/gl/gl_readpixels_dsa.cpp`:480 | test_restart_xfb, test_xfb_capture_apple | GPU_EMU | capture VS-only bằng compute prepass; xem §9.2 |
| FBO blit, clear buffer, fragment data location | 3.0 | 7 | `src/gl/gl_framebuffer.cpp`:819; `src/gl/gl_get.cpp`:1074 | test_mrt_apple, test_frag_bind | DIRECT | `GL_LINEAR` trong blit → NEAREST (divergence, xem `METAL_FEATURE_MAPPING.md` §8) |
| Indexed per-target state (`glEnablei`/`glColorMaski`) | 3.0 | 6 | `src/gl/gl_draw_buffers2_p8.cpp`:11; `src/gl/gl_get.cpp`:619 | test_core_raster_state, test_mrt_apple | DIRECT | |
| Conditional render + `glClampColor` | 3.0 | 3 | `src/gl/gl_sync_query.cpp`:383; `src/gl/gl_readpixels_dsa.cpp`:242 | test_conditional_render | DIRECT | conditional render = hint, bỏ qua hợp lệ |
| Mipmap generation + `glGetStringi` | 3.0 | 2 | `src/gl/gl_ext_dsa_texture.cpp`:349; `src/gl/gl_error_string_debug.cpp`:347 | test_api_surface, test_texture_sampler | DIRECT | `generateMipmapsForTexture:` (`AppleMetalBridge.mm`:2411) |
| Instanced draw | 3.1 | 2 | `src/gl/gl_draw.cpp`:365 | test_core_draw, test_tess_instance_id_apple | DIRECT | `MTLVertexStepFunctionPerInstance` (`AppleMetalBridge.mm`:2694) |
| Uniform buffer objects | 3.1 | 7 | `src/gl/gl_uniform.cpp`:465; `src/gl/gl_get.cpp`:934 | test_ubo_layout_binding, test_uniform_block_query | DIRECT | pack std140 → MSL tại bind time; block binding ghi đè `layout(binding=)` |
| Copy buffer sub data + texture buffer | 3.1 | 2 | `src/gl/gl_buffer.cpp`:667; `src/gl/gl_ext_dsa_texture.cpp`:361 | test_readpixels_packed, test_enum_exts | CPU_EMU | `glCopyBufferSubData` = `memmove` + staged upload |
| Primitive restart index | 3.1 | 1 | `src/gl/gl_draw.cpp`:575 | test_restart_xfb | LIMITED | Metal không có restart index; state được lưu nhưng **không** tách run trên GPU path → divergence |
| Sync objects | 3.2 | 8 | `src/gl/gl_sync_query.cpp`:9; `src/gl/gl_get_extra.cpp`:978 | test_core_query_sync, test_query_gaps_p2 | LIMITED | `glClientWaitSync` luôn trả `ALREADY_SIGNALED`, **không** dùng `MTLSharedEvent` → xem §11 |
| Base vertex draw | 3.2 | 4 | `src/gl/gl_draw.cpp`:369 | test_core_draw, test_sodium_vertex_apple | CPU_EMU | index được rewrite trên CPU (`AppleMetalBridge.mm`:917 `baseVertex` hard-code 0) |
| Provoking vertex + sample mask | 3.2 | 2 | `src/gl/gl_readpixels_dsa.cpp`:260; `src/gl/gl_multisample_p8.cpp`:11 | test_sample_mask_apple, test_sample_state | LIMITED | Metal lấy vertex **đầu**; GL core mặc định **cuối** → `HARDWARE_LIMITED` |
| Multisample texture storage | 3.2 | 2 | `src/gl/gl_texture.cpp`:3300 | test_msaa_gl_apple, test_msaa_array_apple | DIRECT | `MTLTextureType2DMultisample` |
| Untyped framebuffer texture + multisample query | 3.2 | 2 | `src/gl/gl_framebuffer.cpp`:196; `src/gl/gl_get_extra.cpp`:1378 | test_fbo_completeness, test_sample_locations_apple | DIRECT | |
| Sampler objects | 3.3 | 14 | `src/gl/gl_sampler.cpp`:8; `src/gl/gl_get_extra.cpp`:682 | test_texture_sampler, test_core_texture_sampler | DIRECT | `MTLSamplerState` per unit |
| Dual-source blending / `glBindFragDataLocationIndexed` | 3.3 | 2 | `src/gl/gl_program.cpp`:1561; `src/gl/gl_get.cpp`:1083 | test_dual_source, test_frag_bind | LIMITED | mapping hiện tại là `[[color(location+index)]]` **không phải** `index(1)` → sai blend source (xem §10) |
| Packed vertex attributes `gl*P*ui` | 3.3 | 38 | `src/gl/gl_packed_robust.cpp`:132; `src/gl/gl_vertex.cpp`:381 | test_packed_vertex_types, test_vertex_packed | DIRECT | unpack CPU theo `glspec46.core.pdf` Table 10.2 |
| Per-attribute divisor | 3.3 | 1 | `src/gl/gl_vertex.cpp`:192 | test_core_vertex_attrib | DIRECT | `MTLVertexStepFunctionPerInstance` |
| Query counter + 64-bit query result | 3.3 | 3 | `src/gl/gl_sync_query.cpp`:201 | test_query_samples_passed, test_core_query_sync | DIRECT | `GL_TIMESTAMP` là CPU clock; `GL_GPU_DISJOINT` báo `TRUE` |

**Tổng 3.x: 26 feature, 172 entry point, 0 stub.**

---

## 4. Bảng feature — GL 4.x (4.0 – 4.6)

| Feature | Version GL | Số EP | file:line (đại diện) | Test ID | Mapping Metal | Ghi chú |
|---|---|---|---|---|---|---|
| Tessellation control (`glPatchParameter*`) | 4.0 | 2 | `src/gl/gl_draw.cpp`:589 | test_tess_patch_apple, test_tess_quad_apple | HYBRID | TCS→compute kernel, TES→post-tess vertex (§9.1) |
| Subroutines | 4.0 | 8 | `src/gl/gl_get_extra.cpp`:1094; `src/gl/gl_program.cpp`:377 | test_subroutine_api, test_shader_subroutine | GENERATED_SHADER | MSL không có fn pointer → dispatch bằng `switch` trên uniform int |
| Transform feedback 4.0 object model | 4.0 | 6 | `src/gl/gl_tess_xfb.cpp`:17 | test_restart_xfb, test_xfb_overflow_apple | GPU_EMU | xem §9.2 |
| Indirect draw + `glDrawTransformFeedback` | 4.0 | 4 | `src/gl/gl_draw.cpp`:434 | test_core_draw, test_mdi_count_baseinstance_apple | CPU_EMU | indirect command **đọc trên CPU** (`gl_draw.cpp`:393) chứ không dùng `MTLIndirectCommandBuffer` |
| Indexed query + per-attachment blend | 4.0 | 7 | `src/gl/gl_sync_query.cpp`:186; `src/gl/gl_raster.cpp`:25 | test_core_query_sync, test_mrt_apple | DIRECT | |
| Double-precision uniforms + query | 4.0 | 18 | `src/gl/gl_uniform.cpp`:582; `src/gl/gl_get.cpp`:1210 | test_core_uniform_entrypoints | LIMITED | MSL **không có** `double` → hạ xuống float (`glsl_to_msl.cpp`:90); xem §10 |
| Per-sample shading | 4.0 | 1 | `src/gl/gl_raster.cpp`:263 | test_sample_shading_state | LIMITED | Metal không có API tần số shading → `HARDWARE_LIMITED` (chấp nhận theo spec §14.3.1.1) |
| Program pipeline objects | 4.1 | 9 | `src/gl/gl_program.cpp`:1729; `src/gl/gl_readpixels_dsa.cpp`:299 | test_pipeline_draw_apple, test_pipeline_sso | DIRECT | `glUseProgramStages` yêu cầu program separable (`gl_program.cpp`:1763) |
| Separate shader objects + program binary | 4.1 | 6 | `src/gl/gl_program.cpp`:152; `src/gl/gl_readpixels_dsa.cpp`:276 | test_core41_contract, test_pipeline_sso | DIRECT | `glShaderBinary` → `INVALID_ENUM` (không có binary format) |
| `glProgramUniform*` (DSA uniform) | 4.1 | 50 | `src/gl/gl_uniform.cpp`:1349 | test_core_uniform_entrypoints, test_regression_uniform_transpose | DIRECT | |
| Viewport/scissor/depth-range arrays | 4.1 | 10 | `src/gl/gl_raster.cpp`:186; `src/gl/gl_ext_state.cpp`:75 | test_viewport_array, test_core_raster_state | LIMITED | A11 chỉ **1** viewport/pass → TGLMT báo 16 nhưng chỉ index 0 tới Metal |
| 64-bit vertex attributes `glVertexAttribL*` | 4.1 | 10 | `src/gl/gl_vertex.cpp`:243; `src/gl/gl_get_extra.cpp`:887 | test_core_vertex_attrib | DIRECT | `double` → float (`isLong`) |
| `glGetShaderPrecisionFormat` | 4.1 | 1 | `src/gl/gl_get_extra.cpp`:1407 | test_shader_precision | DIRECT | Metal: float32 / int32 |
| Image load/store + `glMemoryBarrier` | 4.2 | 2 | `src/gl/gl_compute.cpp`:469 | test_ssbo_graphics_apple, test_compute_dispatch | DIRECT | `texture2d<T, access::read_write>`; **image atomics** → buffer atomics |
| Base instance draw | 4.2 | 3 | `src/gl/gl_draw.cpp`:366 | test_base_instance_apple | DIRECT | `drawPrimitivesBaseInstance` (`AppleMetalBridge.mm`:905) |
| Immutable texture storage + internal format query | 4.2 | 4 | `src/gl/gl_texture.cpp`:3329; `src/gl/gl_get_extra.cpp`:1340 | test_mip_storage_apple, test_internalformat_query | DIRECT | `glTexStorage2D/3D` cấp đủ mip chain |
| Compute shaders | 4.3 | 2 | `src/gl/gl_compute.cpp`:117 | test_compute_dispatch, test_compute_query_indirect | DIRECT | `dispatchThreads` (thread grid) + `dispatchThreadgroupsWithIndirectBuffer` |
| Program interface query | 4.3 | 6 | `src/gl/gl_program_interface_p10.cpp`:44 | test_p10_sodium_iris, test_explicit_uniform_loc | DIRECT | |
| Shader storage block binding | 4.3 | 1 | `src/gl/gl_program.cpp`:1274 | test_ssbo_graphics_apple | DIRECT | `glShaderStorageBlockBinding` override `layout(binding=)` |
| Vertex attribute format/binding | 4.3 | 6 | `src/gl/gl_vertex.cpp`:123 | test_core_vertex_attrib, test_mc_binding_apple | DIRECT | separate path: Format/Binding + BindVertexBuffer |
| Texture views | 4.3 | 1 | `src/gl/gl_texture.cpp`:3472 | test_p10_sodium_iris | DIRECT | `newTextureViewWithPixelFormat:` (iOS 9.0) |
| Copy image sub data | 4.3 | 1 | `src/gl/gl_texture.cpp`:2244 | test_copy_image_3d, test_copy_image_cube | DIRECT | `MTLBlitCommandEncoder.copyFromTexture:` |
| Debug output (KHR_debug) | 4.3 | 10 | `src/gl/gl_debug_p4.cpp`:85 | test_debug_p4 | DIRECT | callback + group stack |
| Framebuffer invalidation + params | 4.3 | 6 | `src/gl/gl_apple_draw.cpp`:2493; `src/gl/gl_framebuffer.cpp`:1022 | test_p10_sodium_iris, test_regression_gl_errors | DIRECT | |
| Buffer invalidation + clear | 4.3 | 4 | `src/gl/gl_buffer.cpp`:1019 | test_core41_contract, test_core_buffer | DIRECT | |
| Multi-draw indirect | 4.3 | 2 | `src/gl/gl_draw.cpp`:469 | test_enum_behavior, test_sodium_vertex_apple | CPU_EMU | loop `glDrawArraysIndirect` |
| Multisample texture storage 3D | 4.3 | 2 | `src/gl/gl_texture.cpp`:3347 | test_msaa_array_apple, test_texture_target_validation | LIMITED | `MTLTextureType2DMultisampleArray` là **Apple7** → không dùng trên A11 |
| Texture buffer range | 4.3 | 1 | `src/gl/gl_readpixels_dsa.cpp`:448 | test_enum_behavior | DIRECT | texture buffer |
| Buffer storage (immutable) | 4.4 | 1 | `src/gl/gl_buffer.cpp`:563 | test_buffer_dsa_p5, test_buffer_storage_flags | DIRECT | `GL_BUFFER_IMMUTABLE_STORAGE` |
| Bindless-style binding entry points | 4.4 | 6 | `src/gl/gl_buffer.cpp`:482; `src/gl/gl_compute.cpp`:481 | test_core_buffer, test_texture_per_target_bind | DIRECT | |
| Clear texture image | 4.4 | 2 | `src/gl/gl_texture.cpp`:3045 | test_clear_tex_3d, test_clear_tex_cube | DIRECT | |
| Direct State Access: named objects | 4.5 | 52 | `src/gl/gl_buffer.cpp`:337; `src/gl/gl_framebuffer.cpp`:34 | test_buffer_dsa_p5, test_modern_ext_objects | DIRECT | `glCreate*` dùng `registry.Create` (non-zero name) |
| Clip control | 4.5 | 1 | `src/gl/gl_raster.cpp`:232 | test_framebuffer_state, test_enum_behavior | LIMITED | origin → viewport flip (đúng); depth mode `ZERO_TO_ONE` **chưa** xử lý trong shader (`gl_program.cpp`:1350 cảnh báo) |
| Robustness `Getn*` queries | 4.5 | 18 | `src/gl/gl_packed_robust.cpp`:201; `src/gl/gl_get_extra.cpp`:483 | test_readpixels_packed, test_get_texture_formats | DIRECT | 6 hàm evaluator/pixel-map trả 0 (compat-only, không có nội dung thật) |
| Texture barrier + `glReadnPixels` + region barrier | 4.5 | 3 | `src/gl/gl_readpixels_dsa.cpp`:267; `src/gl/gl_compute.cpp`:468 | test_readpixels_packed, test_readpixels_region | HYBRID | `glMemoryBarrierByRegion` = full barrier |
| Transform feedback buffer binding + queries | 4.5 | 5 | `src/gl/gl_tess_xfb.cpp`:90 | test_restart_xfb, test_xfb_capture_apple | GPU_EMU | |
| Query buffer objects | 4.5 | 4 | `src/gl/gl_sync_query.cpp`:348 | test_query_objects, test_query_samples_passed | DIRECT | |
| Named buffer clear (DSA) | 4.5 | 2 | `src/gl/gl_buffer.cpp`:967 | test_buffer_dsa_p5 | DIRECT | |
| Named framebuffer invalidation (DSA) | 4.5 | 2 | `src/gl/gl_framebuffer.cpp`:1050 | test_regression_gl_errors | DIRECT | |
| `glPolygonOffsetClamp` | 4.6 | 1 | `src/gl/gl_raster.cpp`:140 | test_framebuffer_state, test_p0_honesty | DIRECT | `setDepthBias:slopeScale:clamp:` |
| Multi-draw indirect count | 4.6 | 2 | `src/gl/gl_draw.cpp`:496 | test_core_draw, test_mdi_count_baseinstance_apple | CPU_EMU | draw count đọc từ `PARAMETER_BUFFER` trên CPU |
| SPIR-V specialization (`ARB_gl_spirv`) | 4.6 | 1 | `src/gl/gl_shader.cpp`:90 | test_core_program_framebuffer_query | STUB | `glSpecializeShader` bỏ qua toàn bộ tham số và **recompile** shader; **không** có SPIR-V ingestion → xem `docs/SHADER_COMPILER.md` §7 |
| 64-bit buffer size query | 3.2 | 1 | `src/gl/gl_get.cpp`:756 | test_core_program_framebuffer_query | DIRECT | |
| Double/enum indexed query | 4.1 | 2 | `src/gl/gl_get.cpp`:732 | test_core_program_framebuffer_query, test_core_raster_state | DIRECT | |
| Atomic counter buffer query + instanced XFB replay | 4.2 | 3 | `src/gl/gl_get.cpp`:1027; `src/gl/gl_draw.cpp`:559 | test_core_draw, test_core_program_framebuffer_query | DIRECT / GPU_EMU | `glDrawTransformFeedbackStreamInstanced` hiện bỏ qua `stream`/`n` (`gl_draw.cpp`:559) |
| 64-bit internal format query | 4.3 | 1 | `src/gl/gl_get_extra.cpp`:1352 | test_core_program_framebuffer_query | DIRECT | |
| Named texture/framebuffer/renderbuffer queries + copy + compressed sub-image | 4.5 | 35 | `src/gl/gl_framebuffer.cpp`:941; `src/gl/gl_buffer.cpp`:686 | test_compressed_contract, test_get_texture_3d | DIRECT | `glCompressedTextureSubImage1D` trả `INVALID_ENUM` (`gl_texture.cpp`:2838) |

**Tổng 4.x: 47 feature, 325 entry point, 0 stub** (trong đó `glSpecializeShader` là
STUB về *hành vi*, không phải về *entry point*).

---

## 5. Kiến trúc đường đi (chi tiết từng bước)

### 5.1 Shader object → program object → link

| Bước | Hàm | file:line | Ghi chú |
|---|---|---|---|
| Tạo shader | `glCreateShader` | `src/gl/gl_shader.cpp`:10 | `registry.Create(ObjectKind::Shader)` → tên khác 0 |
| Nạp source | `glShaderSource` | `src/gl/gl_shader.cpp`:39 | `count<0` / `str[i]==NULL` → `GL_INVALID_VALUE` (bug crash cũ đã sửa) |
| Compile | `glCompileShader` | `src/gl/gl_shader.cpp`:56 | Gọi `ConvertGLSLtoMSL` (`src/gl/glsl_to_msl.cpp`:8351); lỗi → `COMPILE_STATUS=false` + `infoLog` |
| Tạo program | `glCreateProgram` | `src/gl/gl_program.cpp`:146 | |
| Gắn shader | `glAttachShader` / `glDetachShader` | `src/gl/gl_program.cpp`:175 | |
| Link | `glLinkProgram` | `src/gl/gl_program.cpp`:393 | 6 nhóm kiểm tra, xem §5.2 |
| Dùng | `glUseProgram` | `src/gl/gl_program.cpp`:1525 | `GL_INVALID_OPERATION` nếu program không tồn tại |

### 5.2 Link: những gì TGLMT kiểm tra (CURRENT)

Theo thứ tự trong `glLinkProgram`:

1. **Shader tồn tại & đã compile** — `gl_program.cpp`:435-436 → `infoLog = "error: missing shader"` / `"error: shader not compiled"`.
2. **Varying matching** — mỗi `fs.in` phải có `vs.out` cùng tên **và cùng kiểu**; thiếu → `"error: fragment input not written by vertex: <name>"` (`gl_program.cpp`:917), sai kiểu → `"error: varying type mismatch: <name>"` (`gl_program.cpp`:911).
3. **Gán location thật cho varying** — explicit của VS/FS phải khớp nhau, nếu lệch → `"error: varying location mismatch"` (`gl_program.cpp`:934). Varying không có explicit → gán số trống kế tiếp (≥900 là "tạm", `include/tglmt/GLSLConverter.h`:97) và **viết lại MSL** bằng `RewriteLocns` (`gl_program.cpp`:939-945`).
4. **Gán attribute location** — `glBindAttribLocation` + `layout(location=)` được giữ; phần còn lại gán index trống nhỏ nhất (`gl_program.cpp`:962-989). **Aliasing 2 attribute ACTIVE cùng index → link fail** (`gl_program.cpp`:1019`).
5. **Uniform layout** — VS trước, FS sau, mỗi stage align 16 (`gl_program.cpp`:1028-1043`). `layout(location=N)` explicit được giữ (`gl_program.cpp`:1044-1069`).
6. **Biên dịch Metal** — `compileLibrary(MSL)` cho VS/FS/TCS/TES/CS; lỗi Metal ⇒ `LINK_STATUS=false` + excerpt 1200 ký tự MSL trong `infoLog` (`gl_program.cpp`:1418-1503`).

> **Quan trọng:** link fail vì **lỗi Metal** cũng là link fail đúng GL — nhưng
> `infoLog` chứa thông báo Metal, không phải thông báo GLSL. Đây là hệ quả của
> kiến trúc "token rewriter" (không có front-end GLSL thật) — xem
> `docs/SHADER_COMPILER.md` §5.

### 5.3 Uniform → GPU

| Bước | file:line | Ghi chú |
|---|---|---|
| Tra cứu location | `src/gl/gl_uniform.cpp`:394 | Sau khi link + reflection đầy đủ, tên **không** có trong program ⇒ `-1` (`gl_uniform.cpp`:417). Trước link → nới lỏng (app cache loc sớm). |
| Ghi giá trị | `src/gl/gl_uniform.cpp`:551 (`glUniform1f` … ) | `UniformSlotOk` kiểm tra type/count → `GL_INVALID_OPERATION`/`GL_INVALID_VALUE` |
| Đóng gói ma trận | `include/tglmt/uniform_pack.h` | GL `mat3` = 36 B chặt; MSL `float3x3` trong `constant` struct = 48 B → phải pack theo cột |
| Upload draw path | `src/gl/gl_apple_draw.cpp`:2940 (`setVertexBuffer`) | buffer uniform VS riêng, FS riêng; scratch `thread_local` tái dùng (0 alloc khi đổi) |
| Upload compute path | `src/gl/gl_compute.cpp`:149-171 | `buffer(16)` = uniforms |

### 5.4 Vertex input → GPU

Xem §6.

### 5.5 Framebuffer → GPU

Xem §7.

### 5.6 Draw → present

| Bước | file:line | Ghi chú |
|---|---|---|
| Validate mode/type | `src/gl/gl_draw.cpp`:142-143 | `IsValidPrimitiveMode` / `IsValidIndexType`; sai ⇒ `GL_INVALID_ENUM` và **không** draw |
| XFB mode compatibility | `src/gl/gl_draw.cpp`:147-155 | `glBeginTransformFeedback` mode phải tương thích với draw mode |
| Conditional render | `src/gl/gl_draw.cpp`:156-165 | query chưa sẵn sàng ⇒ skip draw |
| Pipeline statistics | `src/gl/gl_draw.cpp`:170-173 | đếm software theo spec §18.4.2 |
| Emit | `src/gl/gl_draw.cpp`:184-207 | non-indexed / indexed (offset 0 vào EBO vẫn là indexed) |
| **Metal draw** | `src/gl/gl_apple_draw.cpp`:1774 (`AppleDrawGL`) | chọn target, dựng pipeline key, bind buffer/uniform/sampler, `drawPrimitives` |
| Present | `src/metal/AppleMetalBridge.mm`:2026 (`presentTarget`) | `CAMetalLayer.nextDrawable` + full-screen flip triangle |

---

## 6. Vertex input path (CURRENT)

### 6.1 Object model

`VertexArrayObject` (`include/tglmt/Context.h`) giữ:

- `attribs[16]`: mỗi attrib có `enabled, size, type, normalized, stride, relativeOffset, buffer, binding, divisor, isInt, isLong`.
- `bindings[]`: mỗi binding có `buffer, offset, stride, divisor`.
- `elementBuffer`: EBO cho draw indexed.

### 6.2 Các entry point chính

| Hàm | file:line | Ghi chú |
|---|---|---|
| `glGenVertexArrays` / `glCreateVertexArrays` | `src/gl/gl_vertex.cpp`:9 / :14 | `registry.Gen` vs `registry.Create` (tên khác 0) |
| `glBindVertexArray` | `src/gl/gl_vertex.cpp`:31 | id không tồn tại ⇒ `GL_INVALID_OPERATION` |
| `glDeleteVertexArrays` | `src/gl/gl_vertex.cpp`:19 | VAO đang bind bị xoá ⇒ binding về 0 |
| `glEnableVertexAttribArray` | `src/gl/gl_vertex.cpp`:43 | `i >= 16` ⇒ `GL_INVALID_VALUE` (giới hạn 16 attrib) |
| `glVertexAttribPointer` | `src/gl/gl_vertex.cpp`:70 | size/type/normalized/stride/offset; buffer = `GL_ARRAY_BUFFER` **tại thời điểm gọi** (spec §10.3) |
| `glVertexAttribIPointer` | `src/gl/gl_vertex.cpp`:88 | format integer: `normalized` bị reset |
| `glVertexAttribLPointer` | `src/gl/gl_vertex.cpp`:106 | format 64-bit (`isLong`) |
| `glVertexAttribFormat` / `IFormat` / `LFormat` | `src/gl/gl_vertex.cpp`:123 / :132 / :143 | separate path (GL 4.3) |
| `glVertexAttribBinding` | `src/gl/gl_vertex.cpp`:183 | gắn attrib → binding index |
| `glBindVertexBuffer` | `src/gl/gl_vertex.cpp`:152 | binding → (buffer, offset, stride) |
| `glVertexBindingDivisor` | `src/gl/gl_vertex.cpp`:175 | divisor là state **của binding** |
| `glVertexAttribDivisor` | `src/gl/gl_vertex.cpp`:192 | đồng bộ cả hai |

### 6.3 Tính effective offset (spec §10.3.1)

`src/gl/gl_apple_draw.cpp`:2058-2081:

```
effective_address = bindings[binding].offset (base) + attribs[i].relativeOffset
```

`VAOSyncAttribOffset` cập nhật `relativeOffset` khi binding đổi. **Bug cũ đã sửa:**
trước đây dùng chung `a.offset` cho cả base lẫn relative ⇒ base+relative bị nhân
đôi (mất hình trong Separate path của Minecraft).

### 6.4 Metal mapping

`makeCustomPipeline` (`src/metal/AppleMetalBridge.mm`:2662) dựng `MTLVertexDescriptor`:

- `vd.attributes[loc].format` ← `ToMTLVertexFormat(size, type, normalized)` (`:2674`).
- `vd.attributes[loc].offset` ← `attribs[i].relativeOffset` (`:2679`).
- `vd.attributes[loc].bufferIndex` ← `attribs[i].binding` (`:2680`).
- `vd.layouts[bufferIndex].stride` ← stride của binding (`:2683`).
- `stepFunction = PerInstance` + `stepRate = divisor` nếu `divisor > 0` (`:2694-2699`).
- `loc >= 31` hoặc `bufferIndex >= 31` ⇒ `nullptr` (Metal chỉ 31 attribute, FST p.7).

**Giới hạn:** `MAX_VERTEX_ATTRIBS` = 16 (Metal cho 31); `stride = 0` ⇒ TGLMT tự tính
`size * sizeof(type)` (đóng gói chặt) tại `gl_apple_draw.cpp`:2092.

**Fill-zero attribute:** nếu shader đọc attribute mà GL array **tắt**, TGLMT bind
zero-buffer vào slot trống (`gl_apple_draw.cpp`:2099-2120`) — đúng trường hợp Iris
inject `iris_Entity` nhưng format draw không có element đó.

---

## 7. Framebuffer path (CURRENT)

### 7.1 Object model

`FramebufferObject` giữ `colorTex/colorLevel/colorLayer` (theo attachment index),
`depthTex/depthLevel/depthLayer`, `stencilTex`, `depthStencilTex`, `drawBuffers`,
`readBuffer`.

| Hàm | file:line |
|---|---|
| `glGenFramebuffers` / `glCreateFramebuffers` | `src/gl/gl_framebuffer.cpp`:29 / :34 |
| `glBindFramebuffer` | `src/gl/gl_framebuffer.cpp`:56 |
| `glGenRenderbuffers` / `glBindRenderbuffer` | `src/gl/gl_framebuffer.cpp`:124 / :118 |
| `glRenderbufferStorage` | `src/gl/gl_framebuffer.cpp`:147 |
| `glFramebufferTexture2D` | `src/gl/gl_framebuffer.cpp`:218 |
| `glFramebufferRenderbuffer` | `src/gl/gl_framebuffer.cpp`:189 |
| `glCheckFramebufferStatus` | `src/gl/gl_framebuffer.cpp`:536 |
| `glDrawBuffers` / `glReadBuffer` | `src/gl/gl_framebuffer.cpp`:562 / :589 |
| `glBlitFramebuffer` | `src/gl/gl_framebuffer.cpp`:819 |
| `glClearBufferfv` | `src/gl/gl_framebuffer.cpp`:1101 |

### 7.2 Quy tắc completeness (SPEC GL 4.6 §9.4.2)

`CheckFramebufferComplete` (`src/gl/gl_framebuffer.cpp`:477-534`):

| Điều kiện | Kết quả | dòng |
|---|---|---|
| Attachment có kích thước 0 | `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT` (0x8CD6) | :486 |
| Kích thước khác attachment đầu tiên | `GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS` (0x8CD9) | :502 |
| Layered-ness / số layer khác nhau | `GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS` (0x8DA8) | :509 |
| FBO không có attachment nào, không có default size | `GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT` (0x8CD7) | :530 |
| **Draw/Read buffer trỏ attachment trống** | **không còn làm FBO incomplete** (GL 4.1+ / ARB_ES2_compatibility) | :470-476, :531-534 |
| FBO có `defaultWidth/Height > 0` mà không attachment | complete (`ARB_framebuffer_no_attachments`) | :529-530 |

`glCheckFramebufferStatus` chỉ nhận target `DRAW`/`READ`/`FRAMEBUFFER`, sai ⇒
`GL_INVALID_ENUM` (`gl_framebuffer.cpp`:538`).

### 7.3 Draw buffer / read buffer validation

`ClassifyBufferConst` (`gl_framebuffer.cpp`:421-434`) theo GL 4.6 table 17.4/17.5/17.6:

- FBO: chỉ `GL_NONE` và `GL_COLOR_ATTACHMENT0..7` hợp lệ; `GL_COLOR_ATTACHMENT8..31` hoặc hằng default-FB ⇒ `GL_INVALID_OPERATION`; còn lại ⇒ `GL_INVALID_ENUM`.
- Default FB: hằng **singular** (table 17.4) khác **plural** (table 17.6) bị phân biệt; `GL_FRONT`/`GL_BACK`/`GL_LEFT`/`GL_RIGHT`/`GL_FRONT_AND_BACK` không hợp lệ trong `glDrawBuffers`.
- `glDrawBuffers` lặp hằng khác `GL_NONE` ⇒ `GL_INVALID_OPERATION` (`gl_framebuffer.cpp`:456-459`).

### 7.4 Metal mapping

`WrapTarget` (`src/core/Context.cpp`:560) gom attachment thành `IRenderTarget`:

- Color ≤ 8 slot, format lấy từ texture thật (`gl_apple_draw.cpp`:2190`).
- Depth tách riêng khỏi stencil (`AppleMetalBridge.mm`:770-771`).
- 3D texture làm color target cần `depthPlane` (`AppleMetalBridge.mm`:790-791`).
- MSAA FBO 1 color slot dùng target multisample+resolve của texture (`gl_apple_draw.cpp`:2207-2225`).
- `GL_NONE` draw buffer ⇒ slot `MTLPixelFormatInvalid` (lỗ hổng hợp lệ).

---

## 8. Compute path (CURRENT)

| Bước | file:line | Ghi chú |
|---|---|---|
| Compile compute shader | `src/gl/gl_shader.cpp`:64 | stage `0x91B9` được chấp nhận |
| Link compute program | `src/gl/gl_program.cpp`:857-879 | `compileLibrary` + `makeComputePipeline("TGLMT_cs")`; lỗi Metal ⇒ link fail |
| `layout(local_size_x/y/z)` | `src/gl/glsl_to_msl.cpp`:8594-8625 | bắt vào `GLSLConvertResult::localSize[3]`, validate 1..1024 |
| Dispatch | `src/gl/gl_compute.cpp`:117 | `glDispatchCompute(x,y,z)` = **số workgroup**; Metal `dispatch3D` nhận **tổng thread** ⇒ `grid = groups * localSize` (`gl_compute.cpp`:313-314) |
| Grid = 0 | `src/gl/gl_compute.cpp`:312 | GL §7.12.1: 0 group ⇒ không làm gì (Metal sẽ fault) |
| Indirect dispatch | `src/gl/gl_compute.cpp`:331 | `dispatchThreadgroupsIndirect(gpu, offset, localSize)` — args đọc **trên GPU** |
| Uniform upload | `src/gl/gl_compute.cpp`:149-171 | buffer slot 16 |
| SSBO bind | `src/gl/gl_compute.cpp`:175-199 | `setBuffer(b, slot, offset)`, tôn trọng `glBindBufferRange` |
| Atomic counter bind | `src/gl/gl_compute.cpp`:201-213 | `device atomic_uint*` |
| Sampler bind | `src/gl/gl_compute.cpp`:257-275 | texture + sampler, sampler mặc định LINEAR/REPEAT |
| Image bind | `src/gl/gl_compute.cpp`:278-290 | texture thật, **không** sampler state |
| `glMemoryBarrier` | `src/gl/gl_compute.cpp`:460 | flush compute encoder đang dồn ⇒ ordering |
| `glBindImageTexture` | `src/gl/gl_compute.cpp`:469 | level/layer ≠ 0 chỉ dùng base texture (log) |
| `glDispatchComputeGroupSize` | `src/gl/gl_compute.cpp`:122 | ARB_compute_variable_group_size |

**Synchronization:** compute dùng **deferred encoder** (1 CB cho N dispatch) trừ khi
`TGLMT_COMPUTE_SYNC=1` (`gl_compute.cpp`:97-110`). Đọc buffer do compute ghi từ CPU
phải đi qua `SyncComputeWritesTo` (`gl_compute.cpp`:398`, `:504`).

**Image atomics:** `docs/METAL_FEATURE_MAPPING.md` §5 — texture atomics là
**Apple6**; TGLMT dùng **buffer atomics** thay thế (emulable trên A11).

---

## 9. Tessellation và geometry shader

### 9.1 Tessellation (TCS/TES) — HYBRID, đã chạy

Metal không có stage TCS/TES riêng: TCS ⇒ **compute kernel**, TES ⇒
**post-tessellation vertex function**.

| Phần | file:line | Ghi chú |
|---|---|---|
| TCS → kernel MSL | `src/gl/glsl_to_msl.cpp`:4547 (`RewriteTessControl`) | thread = (invocation, patch) |
| TES → post-tess vertex | `src/gl/glsl_to_msl.cpp`:4929 (`RewriteTessEval`) | |
| Layout `(vertices=N)` | `src/gl/glsl_to_msl.cpp`:8628-8645 | validate 1..32 |
| Domain `triangles`/`quads` | `src/gl/glsl_to_msl.cpp`:8651-8680 | `isolines` → fail rõ (:8663); `point_mode` → fail rõ (:8665) |
| Partition mode | `src/gl/glsl_to_msl.cpp`:8669-8674 | `equal_spacing` bị map sang `Integer` ⇒ **divergence** (`METAL_FEATURE_MAPPING.md` §3.4) |
| Metal pipeline | `src/metal/AppleMetalBridge.mm`:1882 (`makeTessPipeline`) | `maxTessellationFactor` = 16 nếu không phải Apple5 (:1937) |
| Draw patches | `src/metal/AppleMetalBridge.mm`:927 | `drawPatches:` + factor buffer |
| Link-time validation | `src/gl/gl_program.cpp`:503-535 | cần đủ VS+TCS+TES+FS; domain phải triangles/quads; 3..32 control point |
| VS prepass (T4b) | `src/gl/gl_program.cpp`:536-570 | VS attribute phải có `layout(location=0..15)` tường minh; TCS `in` phải khớp `vs.out` |
| Draw path | `src/gl/gl_apple_draw.cpp`:1827-1900 | bắt buộc `GL_PATCHES` |

**Test:** `test_tess_patch_apple`, `test_tess_quad_apple`, `test_tess_tri_cp_apple`,
`test_tess_spacing_apple`, `test_tess_cull_apple`, `test_tess_instance_id_apple`,
`test_tess_vs_capture_apple`, `test_tess_gl_in_basevertex_apple`,
`test_tess_mrt_msaa_depth_apple`, `test_tess_clip_distance_apple`,
`test_shader_tess` (32 case + capture).

### 9.2 Transform feedback — GPU_EMU, chỉ VS-only

Metal không có transform feedback. TGLMT mô phỏng:

1. `glTransformFeedbackVaryings` (`src/gl/gl_readpixels_dsa.cpp`:480) lưu danh sách varying.
2. Link tạo thêm một **compute kernel** `TGLMT_vs_capture` chạy lại đúng VS, ghi capture ra device buffer (`src/gl/gl_program.cpp`:1442-1488`).
3. `AppleCaptureXFB` (`src/gl/gl_apple_draw.cpp`:1421`) encode compute pass, đọc kết quả về buffer XFB.
4. `glBeginTransformFeedback` chỉ cho phép mode `POINTS`/`LINES`/`TRIANGLES` (`src/gl/gl_tess_xfb.cpp`:61`).
5. Replay `glDrawTransformFeedback` (`src/gl/gl_draw.cpp`:549`).

**Giới hạn đã biết:**
- **Chỉ VS-only**: `AppleCaptureXFB` từ chối khi program có tess/GS (`gl_apple_draw.cpp`:1427`); link cũng fail với "XFB + tessellation chưa hỗ trợ capture" (`gl_program.cpp`:1434`).
- `separate attribs > 4 buffer` → link fail (`gl_program.cpp`:1437-1441`).
- Replay **không** khôi phục layout attribute gốc (`METAL_FEATURE_MAPPING.md` §3.3).

**Test:** `test_xfb_capture_apple`, `test_xfb_array_matrix_apple`,
`test_xfb_matrix_array_apple`, `test_xfb_overflow_apple`, `test_restart_xfb`.

### 9.3 Geometry shader — `MISSING`, emulation **bắt buộc** (A6)

**CURRENT.** `glCompileShader` **từ chối** stage `0x8DD9` (GL_GEOMETRY_SHADER):
`src/gl/gl_shader.cpp`:64-75 → `COMPILE_STATUS = false`, `infoLog =
"error: geometry shader ngoài subset (GS cần đường mesh riêng)"`.

`glLinkProgram` vẫn gắn cờ `hasGeometryStage` (`src/gl/gl_program.cpp`:496`) để
đường vẽ chọn fallback; nhưng `AppleDrawGL` từ chối draw:
`src/gl/gl_apple_draw.cpp`:1901-1904` → log `"AppleDrawGL: GS trace-only (M5b-future)"`
và `return false`.

> **Đây là `MISSING`, KHÔNG phải `HARDWARE_LIMITED`.**
> `docs/DECISIONS.md` A6 (và `other/prompt.md` §1.2 A6, §8.3, §11.2) quy định:
> *"Geometry shader BẮT BUỘC phải emulation. Không được kết luận
> `HARDWARE_LIMITED` cho GS chỉ vì Metal không có GS stage."*
> `docs/METAL_FEATURE_MAPPING.md` §3.5 đã đặc tả thiết kế emulation compute-based
> (GS kernel + index-building kernel + draw per primitive type), và §10 H19 ghi
> rõ trạng thái `MISSING` + emulation là đáp án cuối.
>
> **PLAN:** `TODO.md` T2.2 / T3.3.

**Vì sao mesh shader không thay thế được:** mesh shader là **Apple7**
(`src/metal/AppleMetalBridge.mm`:2296-2299 → `supportsFamily:MTLGPUFamilyApple7`),
A11 là Apple4 ⇒ không khả dụng. Test `test_gs_mesh_apple` chỉ chạy được trên
Apple7+/M1+ và **SKIP** trên máy này (nhãn `OTHER_GPU`).

---

## 10. Giới hạn Metal (A11) vs yêu cầu tối thiểu của GL

Nguồn Metal: `docs/METAL_A11_CAPABILITIES.md` (Apple FST + iOS SDK headers).
Nguồn GL: `docs/khronos/glspec46.core.pdf` — Table 23.53–23.73 (Implementation
Dependent Values, cột "Minimum Value").

**Không có thiết bị A11 trong lúc viết.** Mọi dòng bảng Metal là
`REFERENCE_ONLY` (Apple documentation) hoặc `NOT_VERIFIED` (cần probe §4 của
`METAL_A11_CAPABILITIES.md`). Số "TGLMT báo" là **đo trên `OTHER_GPU`** bằng
probe `glGetIntegerv` (2026-11-11) hoặc đọc `src/gl/gl_get.cpp`.

| Pname | GL 4.6 minimum | Metal A11 | TGLMT báo | file:line | Đánh giá |
|---|---|---|---|---|---|
| `MAX_VERTEX_ATTRIBS` | 16 | 31 | 16 | `src/gl/gl_get.cpp`:165 | OK (thấp hơn Metal) |
| `MAX_UNIFORM_BUFFER_BINDINGS` | 84 (Table 23.63) | 31 (buffer argument table) | 36 | `src/gl/gl_get.cpp`:190 | **Dưới minimum GL**; converter còn cấp slot từ 17 và **fail compile** nếu > 30 (`glsl_to_msl.cpp`:10736) |
| `MAX_COMBINED_UNIFORM_BLOCKS` | 70 | — | 28 | `src/gl/gl_get.cpp`:195 | **Dưới minimum GL** |
| `MAX_VARYING_COMPONENTS` | 60 | 60 (FST-2017) / 124 (FST-2026) — mâu thuẫn | 128 | `src/gl/gl_get.cpp`:171 | **NOT_VERIFIED** (2 bảng FST mâu thuẫn) |
| `MAX_VARYING_VECTORS` | 15 | — | 32 | `src/gl/gl_get.cpp`:186 | OK |
| `MAX_COMBINED_TEXTURE_IMAGE_UNITS` | 80 | 31 (FST-2017) / 96 (FST-2026) — mâu thuẫn | 48 | `include/tglmt/limits.h`:13 | **NOT_VERIFIED** |
| `MAX_TEXTURE_IMAGE_UNITS` | 16 | 16 sampler/stage | 32 | `src/gl/gl_get.cpp`:166 | OK về GL; nhưng vượt bảng sampler Metal |
| `MAX_VERTEX_UNIFORM_BLOCKS` / `MAX_FRAGMENT_UNIFORM_BLOCKS` | 14 / 14 | — | 14 / 14 | `src/gl/gl_get.cpp`:193-194 | OK |
| `MAX_VERTEX_UNIFORM_COMPONENTS` / `MAX_FRAGMENT_UNIFORM_COMPONENTS` | 1024 | — | 4096 / 4096 | `src/gl/gl_get.cpp`:169-170 | OK |
| `MAX_FRAGMENT_INPUT_COMPONENTS` | 128 | 60 / 124 (mâu thuẫn) | 128 | `src/gl/gl_get.cpp`:71 | **NOT_VERIFIED** |
| `MAX_UNIFORM_BLOCK_SIZE` | 16384 | — | 65536 | `src/gl/gl_get.cpp`:191 | OK |
| `UNIFORM_BUFFER_OFFSET_ALIGNMENT` | 256 (max cho phép) | 4 B | 256 | `src/gl/gl_get.cpp`:192 | OK |
| `MAX_SHADER_STORAGE_BUFFER_BINDINGS` | 8 | — | 16 | `src/gl/gl_get.cpp`:223 | OK |
| `MAX_SHADER_STORAGE_BLOCK_SIZE` | 227 (basic units) | — | 134217728 | `src/gl/gl_get.cpp`:224 | OK |
| `MAX_COMBINED_SHADER_STORAGE_BLOCKS` | 8 | — | 32 | `src/gl/gl_get.cpp`:233 | OK |
| `MAX_COMPUTE_WORK_GROUP_INVOCATIONS` | 1024 | 1024 | 1024 | `src/gl/gl_get.cpp`:205 | OK (báo nhưng **không enforce**) |
| `MAX_COMPUTE_WORK_GROUP_SIZE` | 1024 (x,y), 64 (z) | 1024 | 1024 | `src/gl/gl_get.cpp`:? | OK; z **không** kiểm |
| `MAX_COMPUTE_SHARED_MEMORY_SIZE` | 32768 | 32 KB | 32768 | `src/gl/gl_get.cpp`:200 | OK (báo, không enforce) |
| `MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS` | 8 | — | 4 | `src/gl/gl_get.cpp`:202 | **Dưới minimum GL** |
| `MAX_COMPUTE_ATOMIC_COUNTERS` | 8 | — | 8 | `src/gl/gl_get.cpp`:203 | OK |
| `MAX_COMPUTE_IMAGE_UNIFORMS` | 8 | — | 8 | `src/gl/gl_get.cpp`:208 | OK |
| `MAX_COMPUTE_SHADER_STORAGE_BLOCKS` | 8 | — | 16 | `src/gl/gl_get.cpp`:232 | OK |
| `MAX_FRAGMENT_SHADER_STORAGE_BLOCKS` | 8 | — | 8 | `src/gl/gl_get.cpp`:231 | OK |
| `MAX_VERTEX_SHADER_STORAGE_BLOCKS` | 0 (min) | — | 8 | `src/gl/gl_get.cpp`:227 | OK (vượt min) |
| `MAX_PATCH_VERTICES` | 32 | max tess factor 16 | 32 | `src/gl/gl_get.cpp`:62 | OK |
| `MAX_TESS_GEN_LEVEL` | 64 | 16 | 64 | `src/gl/gl_get.cpp`:? | **Trên Metal**: factor max 16 |
| `MAX_TRANSFORM_FEEDBACK_BUFFERS` | 4 | — | **0** | (không có case) | **Dưới minimum GL** |
| `MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS` | 4 | — | **0** (`GL_INVALID_ENUM`) | — | **Dưới minimum GL** |
| `MAX_VIEWPORTS` | 16 | **1** | 16 | `src/gl/gl_get.cpp`:407 | **Over-report**: chỉ index 0 tới Metal → divergence |
| `MAX_SAMPLES` | 4 | 4 | 4 | `src/gl/gl_get.cpp`:172 | OK |
| `MAX_SAMPLE_MASK_WORDS` | 1 | — | 1 | `src/gl/gl_get.cpp`:173 | OK |
| `MAX_COLOR_ATTACHMENTS` | 8 | 8 | 8 | `src/gl/gl_get.cpp`:162 | OK |
| `MAX_DRAW_BUFFERS` | 8 | 8 | 8 | `src/gl/gl_get.cpp`:163 | OK |
| `MAX_DUAL_SOURCE_DRAW_BUFFERS` | 1 | 1 (Apple2) | 1 | `src/gl/gl_get.cpp`:164 | OK (nhưng mapping `index=1` sai — xem §4) |
| `MAX_CLIP_DISTANCES` | 8 | **1** | 8 | `src/gl/gl_get.cpp`:408 | **Over-report**; converter chỉ hỗ trợ `gl_ClipDistance[0]` (`glsl_to_msl.cpp`:6025-6071) |
| `MAX_CULL_DISTANCES` | 8 | **không có** | 0 | (không có case) | `gl_CullDistance` → fail rõ (`glsl_to_msl.cpp`:6030) |
| `MAX_TEXTURE_SIZE` | 16384 | 16384 | **8192** | `src/gl/gl_get.cpp`:156 | **Dưới minimum GL** |
| `MAX_3D_TEXTURE_SIZE` | 2048 | 2048 | 2048 | `src/gl/gl_get.cpp`:157 | OK |
| `MAX_ARRAY_TEXTURE_LAYERS` | 2048 | 2048 | 2048 | `src/gl/gl_get.cpp`:159 | OK |
| `MAX_CUBE_MAP_TEXTURE_SIZE` | 16384 | 16384 | **8192** | `src/gl/gl_get.cpp`:158 | **Dưới minimum GL** |
| `MAX_RENDERBUFFER_SIZE` | 16384 | 16384 | **8192** | `src/gl/gl_get.cpp`:160 | **Dưới minimum GL** |
| `MAX_RECTANGLE_TEXTURE_SIZE` | 16384 | — | 8192 | `src/gl/gl_get.cpp`:? | **Dưới minimum GL** |
| `MAX_TEXTURE_BUFFER_SIZE` | 65536 | 256 M-px | 65536 | `src/gl/gl_get.cpp`:196 | OK |
| `MAX_ELEMENT_INDEX` | 2³²−1 | — | 0 | (không có case) | **Dưới minimum GL** |
| `MAX_UNIFORM_LOCATIONS` | 1024 | — | 1024 | `src/gl/gl_get.cpp`:? | OK |
| `MAX_SUBROUTINES` | 256 | — | 0 (`GL_INVALID_ENUM`) | — | **Dưới minimum GL** |

> **Kết luận giới hạn:** 12 pname bị báo **dưới minimum bắt buộc của GL 4.6** và 3
> pname bị **over-report** so với phần cứng A11. Đây là các lỗi *query*, không
> phải lỗi *hành vi draw*, và đều có thể sửa bằng cách đổi hằng số trong
> `src/gl/gl_get.cpp` — nhưng phải sửa **đồng thời** với giới hạn thật của Metal
> (buffer table 31, 1 viewport, 1 clip distance).

---

## 11. Điểm chưa verify / blocker

| # | Hạng mục | Bằng chứng | Nhãn |
|---|---|---|---|
| 1 | **Chưa chạy trên Apple A11 thật** | toàn bộ số đo trong tài liệu này là `OTHER_GPU` (Intel UHD 630) | `NOT_VERIFIED` |
| 2 | **Geometry shader emulation chưa có** | `src/gl/gl_shader.cpp`:64 (compile fail), `src/gl/gl_apple_draw.cpp`:1901 (draw fail) | `MISSING` — bắt buộc theo A6 |
| 3 | **XFB chỉ VS-only**; XFB + tess/GS chưa có | `src/gl/gl_apple_draw.cpp`:1427, `src/gl/gl_program.cpp`:1434 | `PARTIALLY_IMPLEMENTED` |
| 4 | **`glClientWaitSync` không chặn CPU** | `src/gl/gl_sync_query.cpp`:26 | `PARTIALLY_IMPLEMENTED` — Metal **có** `MTLSharedEvent` từ iOS 12 ⇒ là implementation gap, **không** phải giới hạn phần cứng |
| 5 | **Indirect draw đọc trên CPU** | `src/gl/gl_draw.cpp`:393 | `PARTIALLY_IMPLEMENTED` — Metal **có** `MTLIndirectCommandBuffer` (Apple3) |
| 6 | **fp64 không có** | `src/gl/glsl_to_msl.cpp`:90 (`double`→`float`) | `HARDWARE_LIMITED` — MSL không có `double`; có đường two-float emulation (`METAL_FEATURE_MAPPING.md` §5) |
| 7 | **Dual-source blending mapping sai** | `src/gl/glsl_to_msl.cpp`:12556 (`cidx = location + index`) | `INCORRECT` — Metal có `index(1)` từ iOS 11; sửa được trên A11 |
| 8 | **Provoking vertex LAST không được tôn trọng** | `src/gl/gl_readpixels_dsa.cpp`:260 | `HARDWARE_LIMITED` — sửa được bằng CPU index rotation |
| 9 | **Primitive restart không tách run** | `src/gl/gl_draw.cpp`:575 | `HARDWARE_LIMITED` — sửa được bằng CPU index rewrite |
| 10 | **`MAX_VIEWPORTS` = 16 nhưng Metal chỉ 1** | `src/gl/gl_get.cpp`:407 | `INCORRECT` (over-report) |
| 11 | **`GL_MAX_TEXTURE_SIZE` = 8192 < 16384** | `src/gl/gl_get.cpp`:156 | `INCORRECT` (dưới minimum GL) |
| 12 | **12 pname dưới minimum GL** | §10 | `INCORRECT` |
| 13 | **Texture argument table 31 vs 96; `[[stage_in]]` 60 vs 124** | `docs/METAL_A11_CAPABILITIES.md` §2.5 | `NOT_VERIFIED` — cần probe trên A11 |
| 14 | **SPIR-V ingestion (`GL_ARB_gl_spirv`) chưa có** | `src/gl/gl_shader.cpp`:90 | `MISSING` — xem `docs/SHADER_COMPILER.md` §7 |
| 15 | **`glPolygonOffset` đơn vị khác GL** | `src/gl/gl_apple_draw.cpp`:2771-2796 | `PARTIALLY_IMPLEMENTED` — Metal bias tính theo 2⁻²⁴ ULP |
| 16 | **Dual-source + MRT không tương thích Metal** | `docs/METAL_FEATURE_MAPPING.md` §6 | `HARDWARE_LIMITED` (kết hợp) |
| 17 | **`GL_CLIP_CONTROL` depth mode `ZERO_TO_ONE` chưa xử lý** | `src/gl/gl_program.cpp`:1350 (cảnh báo bake z-convert) | `PARTIALLY_IMPLEMENTED` |
| 18 | **12 `Getn*` evaluator/pixel-map trả 0** | `src/gl/gl_packed_robust.cpp`:194-205 | `PARTIALLY_IMPLEMENTED` (dữ liệu evaluator/pixel-map chưa có) |
| 19 | **`glCompressedTextureSubImage1D` trả `INVALID_ENUM`** | `src/gl/gl_texture.cpp`:2838 | `STUB` |
| 20 | **`glDrawTransformFeedbackStreamInstanced` bỏ qua `stream`/`n`** | `src/gl/gl_draw.cpp`:559 | `PARTIALLY_IMPLEMENTED` |

---

## 12. Cross-reference

| Chủ đề | Tài liệu |
|---|---|
| Kiến trúc compiler GLSL→MSL, version GLSL, cache | `docs/SHADER_COMPILER.md` |
| Coverage & divergence của compiler | `docs/GLSL_460_COMPILER.md` |
| Ma trận version GLSL ↔ OpenGL | `docs/GLSL_VERSION_MATRIX.md` |
| Profile Core vs Compatibility | `docs/OPENGL_PROFILE_MATRIX.md` |
| Mapping GL→Metal + bằng chứng `HARDWARE_LIMITED` | `docs/METAL_FEATURE_MAPPING.md` |
| Capability A11 + probe runtime | `docs/METAL_A11_CAPABILITIES.md` |
| Kiến trúc tổng thể (7 tầng) | `docs/UNIFIED_GRAPHICS_ARCHITECTURE.md` |
| Fixed-function (Compatibility) | `docs/FIXED_FUNCTION_PIPELINE.md` |
| Quyết định kiến trúc đã chốt | `docs/DECISIONS.md` |
