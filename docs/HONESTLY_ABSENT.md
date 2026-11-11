# Honestly Absent — Extensions KHÔNG advertise và vì sao

**Quy tắc vàng:** advertise ⇔ implement ⇔ test. Extension nào dưới đây KHÔNG có
trong `glGetStringi(GL_EXTENSIONS)` vì chưa đủ 3 điều kiện. Không "nói dối" —
app check extension string sẽ thấy thiếu và dùng đường fallback.

(Cập nhật 2026-11-02. Nguồn số liệu hiện tại: `docs/EXTENSION_MATRIX.md`,
`docs/EXTENSION_BEHAVIOR_AUDIT.md`; `tools/ext_coverage.py --missing` liệt kê
chi tiết từng hàm thiếu.)

## 1. Metal-impossible (kiến trúc Metal không có — không thể implement đúng)

| Extension | Vì sao không thể |
|---|---|
| **Geometry shader** (ARB_geometry_shader4, EXT_geometry_shader4, OES_geometry_shader) | Metal không có geometry stage cổ điển; Apple7+ có mesh shaders nhưng TGLMT chưa map, và compute fallback chưa có. TGLMT fail trung thực tại `glCompileShader` thay vì im lặng. |
| **Tessellation** (ARB_tessellation_shader, OES_tessellation_shader) | Metal **có** tessellation (Apple3+: post-tessellation vertex + tessellation factors). TGLMT đã chạy triangles + quads, output CP 3..32, input/output patch count runtime, TCS `gl_in`/`gl_out`, TES `gl_TessCoord`/`gl_TessLevel*`, `gl_InstanceID`/local `gl_PrimitiveID` trong TCS/TES, default levels; đã có `gl_ClipDistance[0]` cho VS/TES, nhưng chưa có isolines/point_mode, triangle domain factor >16 trên A11, `gl_CullDistance`, multi-index clip, hay TCS `gl_ClipDistance`. Chưa advertise vì chưa đủ contract/device matrix. |
| **Bindless textures** (ARB_bindless_texture, NV_bindless_texture) | GL bindless dùng 64-bit handles trỏ trực tiếp GPU memory. Metal dùng argument buffers + resource heaps — mô hình khác hẳn, không map 1:1 được mà không viết lại toàn bộ texture binding. Ngoài phạm vi (MC/Sodium không dùng bindless). |
| **Sparse textures/buffers** (ARB_sparse_texture, ARB_sparse_buffer) | Metal 3 có sparse resources nhưng API khác (sparse texture heap, tile mapping). GL sparse (commitment, residency) không map trực tiếp. MC không dùng sparse. |
| **Shader clock / ballot nguyên thủy** (ARB_shader_clock, NV_shader_thread_group) | Metal không expose shader clock hay subgroup ballot nguyên thủy cho fragment/compute theo cách GL yêu cầu (subgroup đã emulate qua simd_* cho các op cơ bản — xem P6c). |
| **Fragment shading rate** (EXT_fragment_shading_rate, NV_shading_rate_image) | Metal có variable rasterization rate (Apple6+); TGLMT chưa map sang GL API nên chưa advertise. |

## 2. Legacy fixed-function (GL cổ — không liên quan Core 4.6 / Minecraft)

Core 4.6 đã bỏ fixed-function. Các ext này chỉ có ý nghĩa với GL 1.x-2.x:

- **ARB_imaging** (color table, convolution, histogram — 32 hàm): fixed-function imaging, Core đã bỏ.
- **ARB_matrix_palette, ARB_vertex_blend**: skinning fixed-function, Core bỏ (dùng shader).
- **ARB_vertex_program / ARB_fragment_program** (assembly shaders): bị GLSL thay thế từ 2004.
- **ARB_multitexture, ARB_texture_env_***: fixed-function texture units.
- **ARB_transpose_matrix, ARB_window_pos**: immediate-mode helpers.
- **EXT_fog_coord, EXT_coordinate_frame, EXT_cull_vertex**: fixed-function.
- **EXT_compiled_vertex_array, EXT_convolution**: legacy.
- **ARB_shader_objects**: ĐÃ IMPLEMENT + ADVERTISED (P8; `glValidateProgram` thật, ARB entry points forward core).

*MC (Core 3.2+/GLSL) không dùng bất kỳ ext nào trong nhóm này.*

## 3. Vendor-specific / không áp dụng iOS-Metal

| Extension | Vì sao bỏ qua |
|---|---|
| **ANGLE_*** (framebuffer_blit, instanced_arrays, translated_shader_source...) | ANGLE là translation layer (GL→Metal/D3D). TGLMT đã là native Metal — không cần ANGLE, không implement API của translation layer khác. |
| **APPLE_fence, APPLE_flush_buffer_range, APPLE_object_purgeable, APPLE_texture_range, APPLE_vertex_array_range** | APPLE ext đời cũ (GL 2.x era). Chức năng đã có qua ARB_sync (advertised), ARB_map_buffer_range (advertised), KHR_debug (advertised). Không implement API cũ khi đã có đường mới tốt hơn. |
| **ARM_*, QCOM_*, IMG_*, NV_*, AMD_*, ATI_*** | GPU vendor khác (Adreno/Mali/PowerVR/NVIDIA/AMD). TGLMT chạy Apple GPU — không implement vendor ext của hãng khác. |
| **EGL / CL / X11** (EXT_EGL_image_*, ARB_cl_event, EXT_x11_sync_object, OES_EGL_image) | EGL (Android/Linux), OpenCL interop, X11 — không tồn tại trên iOS. Ngoài phạm vi tuyệt đối. |
| **OES_*** (trừ cái đã advertise) | OES là OpenGL ES. TGLMT là GL 4.6 Core (desktop). Một số OES đã advertise vì alias với core (OES_mapbuffer, OES_vertex_array_object...). OES còn lại (query_matrix, EGL_image...) không liên quan desktop Core. |

## 4. Chưa implement (làm được, nhưng ngoài phạm vi P0–P6 — ứng viên P8+)

Nhóm này **không phải Metal-impossible** — có thể implement nếu cần. Sắp xếp theo giá trị:

### Giá trị cao (MC/mods có thể dùng)
| Extension | Hàm thiếu | Ghi chú |
|---|---|---|
| **ARB_shader_atomic_counters** | 0 | **ĐÃ IMPLEMENT + ADVERTISED (P9)** — atomic_uint → MSL atomics, compute + graphics. |
| **ARB_compute_variable_group_size** | 1 (glDispatchComputeGroupSizeARB) | **ĐÃ IMPLEMENT + ADVERTISED (P4).** |
| **EXT_debug_label** | 1 (glLabelObjectEXT) | **ĐÃ IMPLEMENT + ADVERTISED (P4).** |
| **KHR_blend_equation_advanced** | ~5 | Advanced blend (multiply/overlay...). Metal có một số blend op nhưng không đủ hết. Ít dùng. |
| **EXT_depth_bounds_test** | 1 (glDepthBoundsEXT) | Depth bounds — Metal không có trực tiếp (phải emulate bằng shader clip). Ít dùng. |
| **EXT_stencil_two_side / EXT_stencil_clear_tag** | 0 / 1 | **EXT_stencil_two_side ĐÃ IMPLEMENT + ADVERTISED (P9); GL stencil core đã encode vào Metal depth-stencil**; stencil_clear_tag chưa có. |

### Giá trị thấp (hiếm, chuyên dụng)
- **ARB_ES3_2_compatibility**: **ĐÃ IMPLEMENT + ADVERTISED (P4)** — chỉ 1 hàm (PrimitiveBoundingBox, hint no-op).
- **EXT_raster_multisample, EXT_multisample**: Metal có MSAA; TGLMT đã consume sample mask (inject `[[sample_id]]` trong MSAA fragment shader). `ARB_sample_locations` đã implement + advertise (default positions, programmable table, FBO params, MSAA draw); EXT_raster_multisample vẫn chưa map rasterization samples riêng.
- **ARB_viewport_array, OES_viewport_array**: **State ĐÃ implement đầy đủ** (mỗi viewport index có viewport box + scissor box riêng, `glViewportIndexed*`/`glScissorIndexed*`/`glScissorArrayv` ghi đúng index, `glGetIntegeri_v(GL_SCISSOR_BOX, i)` trả đúng box theo index) và `GL_MAX_VIEWPORTS` trả **16** (mức tối thiểu của GL 4.6 core).
  **Giới hạn còn lại (giới hạn Metal, không phải thiếu state):** Metal không render nhiều viewport trong một draw, và `gl_ViewportIndex` đến từ geometry shader (TGLMT emulate GS bằng compute-prepass nên không mang được index) → **viewport index luôn = 0**, tức `glViewportIndexed(1, …)` sẽ bị bỏ qua khi rasterize. App dùng viewport array để render nhiều lớp trong 1 draw sẽ ra hình sai (không phải crash). MC chỉ dùng 1 viewport nên không ảnh hưởng.
- **EXT_multiview_draw_buffers**: Indexed draw/read buffer cho multiview — ít dùng (đã có draw_buffers2 cho MRT indexed).
- **EXT_framebuffer_blit_layers, EXT_multisampled_render_to_texture**: Layered/MSAA framebuffer nâng cao — ít dùng.
- **INTEL_performance_query, AMD_performance_monitor**: Vendor perf counters — không Apple.
- **EXT_semaphore, EXT_memory_object, EXT_external_buffer**: Vulkan interop — không iOS.

## 4.5 Implemented nhưng CHƯA advertise (P8 — hàm có, ext chưa claim vì cần verify thêm)

Hàm đã implement + test không crash, nhưng extension string CHƯA advertise vì:
- Cần verify device行為 đầy đủ (MSAA paths, integer-exactness), HOẶC
- Extension ngụ ý feature lớn hơn không hoạt động (GS, blend-eq, XFB sau GS/tess).

| Extension | Hàm đã có | Vì sao chưa advertise |
|---|---|---|
| **ARB_geometry_shader4** | `glFramebufferTextureFace` (attach face như 2D) | Tên ext ngụ ý **geometry shader hoạt động** (không — compile fail). Hàm hữu ích cho layered FBO nên giữ, nhưng không claim ext. |
| **KHR_blend_equation_advanced** | `glBlendBarrierKHR` (barrier no-op) | Ext ngụ ý **advanced blend equations** (multiply/screen/...) hoạt động (không — chỉ có barrier). Giữ hàm, không claim. |
| **EXT_texture_integer** | `glClearColorIi/IuiEXT` (lưu clear color) | Integer FBO clear cần **integer-exact** cho giá trị lớn (float32 không biểu diễn hết int32). Hàm lưu được, nhưng chưa đảm bảo exactness → chưa claim. |
| **EXT_transform_feedback** | `glBindBufferOffsetEXT` (bind XFB/UBO với offset) | XFB capture thật đã có cho **VS-only** (compute prepass, interleaved/separate, pause/resume, instancing, no-FS + rasterizer discard, array element/whole-array, mat2/mat2x3, matrix arrays, TF-primitives-written + TF-overflow queries); double/fp64 và XFB sau GS/TES chưa có. Giữ ext chưa advertise cho tới khi đủ mọi stage. |
| **EXT_multisample** | `glSampleMaskEXT/PatternEXT` (lưu mask state) | Core `glSampleMaski` đã được inject vào fragment shader (`[[sample_id]]` + mask buffer) và có `test_sample_mask_apple`. `glSampleMaskEXT` forward sang core; `glSamplePatternEXT` vẫn no-op. Ext chưa advertise vì sample pattern chưa có, dù mask đã ảnh hưởng render. |
| **EXT_multisampled_render_to_texture** | `glFramebufferTexture2DMultisampleEXT` (attach + samples) | Cần verify device MSAA resolve path với texture samples>0. Hàm attach được, nhưng chưa test MSAA render thật → chưa claim. |
| **EXT_framebuffer_blit_layers** | `glBlitFramebufferLayer(s)EXT` (layer 0 forward, layer>0 fail rõ) | Chỉ layer 0 hoạt động (TGLMT 2D, không layers). Ext ngụ ý full layered → chưa claim. |

*Muốn dùng các hàm trên: gọi trực tiếp (symbol tồn tại), đừng check extension string.*

## 5. Shader stages (converter subset — fail trung thực tại compile)

Không phải extension string, mà là **shader stage** converter từ chối rõ (không sinh code đoán mò):

| Stage | Trạng thái | Chi tiết |
|---|---|---|
| Vertex / Fragment | ✅ Full | P0–P6d: texture, bitcast, subgroup, gl_* builtins, image, subroutine. |
| Compute | ✅ Full | P6f: kernel, local_size, SSBO, image, dispatch. Advertised ARB_compute_shader. |
| Tessellation (TCS/TES) | ⚠️ Partial (triangles/quads) | Đã chạy GPU triangles + quads, output CP 3..32, input/output patch count runtime, TCS/TES `gl_in`/`gl_out`, `gl_TessCoord`, `gl_TessLevel*`, `gl_InstanceID`, local `gl_PrimitiveID`, default levels. Isolines/point_mode, `gl_CullDistance`, multi-index clip, TCS `gl_ClipDistance` và factor >16 trên A11 fail rõ hoặc chưa hỗ trợ; `gl_ClipDistance[0]` ở VS/TES đã chạy. |
| Geometry (GS) | ❌ Native no | Metal không có GS cổ điển; Apple7+ mesh shaders và compute fallback chưa được implement. Fail tại compile (không giả). |
| Mesh/Fragment (NV_mesh_shader) | ❌ Ngoài phạm vi | Mesh shaders (Vulkan-style) — Metal có mesh nhưng API khác hẳn, MC không dùng. |

## Cách đọc dashboard

```
python3 tools/ext_coverage.py --kexts    # advertised có đủ hàm không? (phải 0 nói dối)
python3 tools/ext_coverage.py --missing  # ext nào còn thiếu hàm? (chi tiết)
python3 tools/coverage_dashboard.py       # bảng tổng hợp theo nhóm + sẵn sàng implement
```

**Cam kết:** số "nói dối" luôn = 0. Thêm ext mới ⇔ implement đủ hàm ⇔ test pass ⇔ mới thêm vào `kExts[]`.
