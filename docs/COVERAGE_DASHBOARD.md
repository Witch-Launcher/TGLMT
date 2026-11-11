# Coverage Dashboard (TGLMT Extension)

**Advertised: 133** | **Missing (có hàm): 86**

## Theo nhóm (advertised)

| Nhóm | Số lượng | Extensions |
|---|---|---|
| blend | 7 | GL_EXT_blend_color, GL_EXT_blend_equation_separate, GL_EXT_blend_func_separate, GL_EXT_blend_minmax, GL_OES_blend_equation_separate... |
| buffer/storage | 30 | GL_ARB_buffer_storage, GL_ARB_color_buffer_float, GL_ARB_copy_buffer, GL_ARB_draw_buffers, GL_ARB_draw_buffers_blend... |
| compute | 1 | GL_ARB_compute_variable_group_size |
| debug | 3 | GL_EXT_debug_label, GL_EXT_debug_marker, GL_KHR_debug |
| depth/stencil | 6 | GL_ARB_clip_control, GL_ARB_conservative_depth, GL_ARB_depth_clamp, GL_ATI_separate_stencil, GL_EXT_clip_control... |
| image | 3 | GL_EXT_copy_image, GL_NV_copy_image, GL_OES_copy_image |
| multisample | 5 | GL_ARB_sample_locations, GL_ARB_sample_shading, GL_ARB_sampler_objects, GL_MESA_sampler_objects, GL_OES_sample_shading |
| other | 23 | GL_AMD_multi_draw_indirect, GL_ARB_ES3_1_compatibility, GL_ARB_ES3_2_compatibility, GL_ARB_base_instance, GL_ARB_draw_indirect... |
| query | 8 | GL_ARB_internalformat_query, GL_ARB_internalformat_query2, GL_ARB_occlusion_query, GL_ARB_occlusion_query2, GL_ARB_timer_query... |
| robustness | 4 | GL_ARB_robustness, GL_EXT_robustness, GL_KHR_no_error, GL_KHR_robustness |
| shader/program | 9 | GL_ARB_compute_shader, GL_ARB_explicit_uniform_location, GL_ARB_get_program_binary, GL_ARB_parallel_shader_compile, GL_ARB_program_interface_query... |
| sync | 2 | GL_APPLE_sync, GL_ARB_sync |
| texture | 15 | GL_ARB_clear_texture, GL_ARB_get_texture_sub_image, GL_ARB_texture_filter_anisotropic, GL_ARB_texture_mirror_clamp_to_edge, GL_ARB_texture_swizzle... |
| vertex | 17 | GL_APPLE_vertex_array_object, GL_ARB_draw_elements_base_vertex, GL_ARB_half_float_vertex, GL_ARB_instanced_arrays, GL_ARB_vertex_array_bgra... |

## Theo nhóm (missing, có hàm thật)

| Nhóm | Số lượng | Ví dụ |
|---|---|---|
| buffer/storage | 11 | GL_ANGLE_framebuffer_blit(1), GL_ANGLE_framebuffer_multisample(1), GL_APPLE_flush_buffer_range(1)... |
| depth/stencil | 3 | GL_EXT_depth_bounds_test(1), GL_EXT_stencil_clear_tag(1), GL_IMG_user_clip_plane(2) |
| image | 3 | GL_HP_image_transform(6), GL_OES_EGL_image(2), GL_OES_EGL_image_external(1) |
| other | 16 | GL_ARB_matrix_palette(5), GL_ARB_transpose_matrix(4), GL_ARB_window_pos(16)... |
| pixel-transfer | 4 | GL_EXT_pixel_transform(6), GL_EXT_semaphore(9), GL_EXT_semaphore_fd(1)... |
| query | 1 | GL_OES_query_matrix(1) |
| shader/program | 8 | GL_ANGLE_translated_shader_source(1), GL_APPLE_vertex_program_evaluators(7), GL_ARB_fragment_program(17)... |
| sync | 2 | GL_APPLE_fence(8), GL_EXT_x11_sync_object(1) |
| texture | 14 | GL_APPLE_copy_texture_levels(1), GL_APPLE_texture_range(2), GL_ARB_multitexture(33)... |
| vendor-specific | 12 | GL_APPLE_object_purgeable(3), GL_QCOM_alpha_test(1), GL_QCOM_driver_control(4)... |
| vertex | 12 | GL_ANGLE_instanced_arrays(3), GL_APPLE_element_array(5), GL_APPLE_vertex_array_range(3)... |

## Sẵn sàng implement (≤3 hàm thật)

- GL_ANGLE_framebuffer_blit (1 hàm, 0 alias)
- GL_ANGLE_framebuffer_multisample (1 hàm, 0 alias)
- GL_ANGLE_instanced_arrays (3 hàm, 0 alias)
- GL_ANGLE_translated_shader_source (1 hàm, 0 alias)
- GL_APPLE_copy_texture_levels (1 hàm, 0 alias)
- GL_APPLE_flush_buffer_range (1 hàm, 0 alias)
- GL_APPLE_framebuffer_multisample (1 hàm, 0 alias)
- GL_APPLE_object_purgeable (3 hàm, 0 alias)
- GL_APPLE_texture_range (2 hàm, 0 alias)
- GL_APPLE_vertex_array_range (3 hàm, 0 alias)
- GL_ARM_shader_core_properties (1 hàm, 0 alias)
- GL_EXT_bindable_uniform (3 hàm, 0 alias)
- GL_EXT_compiled_vertex_array (2 hàm, 0 alias)
- GL_EXT_cull_vertex (2 hàm, 0 alias)
- GL_EXT_depth_bounds_test (1 hàm, 0 alias)
- GL_EXT_external_buffer (2 hàm, 0 alias)
- GL_EXT_gpu_program_parameters (2 hàm, 0 alias)
- GL_EXT_index_func (1 hàm, 0 alias)
- GL_EXT_index_material (1 hàm, 0 alias)
- GL_EXT_light_texture (3 hàm, 0 alias)
- GL_EXT_memory_object_fd (1 hàm, 0 alias)
- GL_EXT_memory_object_win32 (2 hàm, 0 alias)
- GL_EXT_multiview_draw_buffers (2 hàm, 0 alias)
- GL_EXT_semaphore_fd (1 hàm, 0 alias)
- GL_EXT_semaphore_win32 (2 hàm, 0 alias)
- GL_EXT_shader_pixel_local_storage2 (3 hàm, 0 alias)
- GL_EXT_sparse_texture (1 hàm, 0 alias)
- GL_EXT_stencil_clear_tag (1 hàm, 0 alias)
- GL_EXT_texture_object (2 hàm, 0 alias)
- GL_EXT_texture_perturb_normal (1 hàm, 0 alias)
- GL_EXT_texture_storage_compression (2 hàm, 0 alias)
- GL_EXT_vertex_weighting (3 hàm, 0 alias)
- GL_EXT_win32_keyed_mutex (2 hàm, 0 alias)
- GL_EXT_x11_sync_object (1 hàm, 0 alias)
- GL_IMG_framebuffer_downsample (2 hàm, 0 alias)
- GL_IMG_multisampled_render_to_texture (1 hàm, 1 alias)
- GL_IMG_user_clip_plane (2 hàm, 0 alias)
- GL_INTEL_map_texture (3 hàm, 0 alias)
- GL_OES_EGL_image (2 hàm, 0 alias)
- GL_OES_EGL_image_external (1 hàm, 0 alias)
- GL_OES_point_size_array (1 hàm, 0 alias)
- GL_OES_query_matrix (1 hàm, 0 alias)
- GL_OVR_multiview_multisampled_render_to_texture (1 hàm, 0 alias)
- GL_QCOM_alpha_test (1 hàm, 0 alias)
- GL_QCOM_frame_extrapolation (1 hàm, 0 alias)
- GL_QCOM_framebuffer_foveated (2 hàm, 0 alias)
- GL_QCOM_motion_estimation (2 hàm, 0 alias)
- GL_QCOM_shader_framebuffer_fetch_noncoherent (1 hàm, 0 alias)
- GL_QCOM_shading_rate (1 hàm, 0 alias)
- GL_QCOM_texture_foveated (1 hàm, 0 alias)
- GL_QCOM_tiled_rendering (2 hàm, 0 alias)
- GL_SUNX_constant_data (1 hàm, 0 alias)
- GL_SUN_mesh_array (1 hàm, 0 alias)

---
Xem `docs/HONESTLY_ABSENT.md` để biết ext nào KHÔNG làm và vì sao.
Quy tắc vàng: advertise ⇔ implement ⇔ test.
