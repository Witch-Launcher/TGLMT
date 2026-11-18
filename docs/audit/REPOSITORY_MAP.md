# REPOSITORY_MAP.md — TGLMT

Bản đồ module theo subsistema. Đường dẫn tương đối gốc repo `/Volumes/project/TGLMT`.

## 1. Cấu trúc cao nhất

```
/apps/                 # Ứng dụng demo / launcher integration (Aquarium, v.v.)
/benchmarks/           # Microbenchmark dịch shader + runtime
/build-work/           # Build directory hiện tại (Debug, Metal ON)
/cmake/                # CMake modules (tglmt_buildinfo.h.in, v.v.)
/data/                 # Dữ liệu sinh: version_inventory.json
/docs/                 # Tài liệu (xem §3)
/include/tglmt/        # Header công khai + nội bộ
/scripts/              # Helper script
/shaders/              # Shader fixture
/src/                  # Mã nguồn C++ / Objective-C++
/tests/                # Test CTest (xem §4)
/tools/                # Code generator, probe, audit tool
```

## 2. Include (`include/tglmt/`)

| File | Vai trò |
|------|---------|
| gl46.h | Include tổng hợp OpenGL 4.6 Core (sinh tự động) |
| gl46_types.h | Enum/type Core 4.6 (sinh từ gl.xml) |
| gl_legacy.h | Prototype Compatibility-only (sinh tự động) |
| gl_legacy_types.h | Enum Compatibility-only (sinh tự động) |
| gl_c_api.h | **C ABI công khai** 1049 prototype `extern "C"` (sinh tự động) |
| gl_context_api.h | C ABI tạo/quản lý context versioned |
| gl46_pnames.h | Danh sách pname (sinh tự động) |
| GlProfile.h | `GlProfile` enum, `kGlProfileMax` |
| Context.h | `Context` (trái心), object store, ShaderObject, ProgramObject, LegacyState |
| VersionGate.h | `GlProfile`, `VersionGateAllows`, bảng gate |
| ErrorTracker.h | Hàng đợi lỗi FIFO |
| ObjectRegistry.h | Quản lý tên object (gen/delete/is) |
| StateTracker.h | State shadow moderne (blend, depth, viewport, v.v.) |
| LegacyState.h | State fixed-function tách riêng (light, fog, texenv, matrix, ...) |
| LegacyDraw.h | Đường draw fixed-function |
| LegacyBeginEnd.h | Bảng lệnh cấm trong glBegin/glEnd |
| LegacyDisplayList.h | Display list recorder |
| MetalInterface.h | IMetal abstraction (IDevice, IBuffer, ITexture, ILibrary, IRenderPipeline, IRenderEncoder, IComputeEncoder, ...) |
| GLSLConverter.h | **Converter GLSL→MSL** (string rewriter), `GLSLConvertResult`, `ConvertGLSLtoMSL` |
| GLSLBuiltins.h | Registry built-in pass tự đăng ký |
| GLSLTextUtil.h | Tiện ích văn bản cho built-in pass |
| uniform_pack.h | Tính toán layout uniform |
| UniformQuery.h | Query uniform |
| GeometryEmulation.h | `GsVar`, `GsTranslation`, `TranslateGeometryShader` |
| Renderer.h | Renderer shell |
| limits.h | Giới hạn nội bộ |

## 3. Source core (`src/core/`)

| File | Vai trò |
|------|---------|
| Context.cpp | Constructor Context (xây dựng version-gate mask mặc định), Current/MakeCurrent |
| ErrorTracker.h/.cpp | ErrorTracker (queue FIFO) |
| ObjectRegistry.h/.cpp | Đăng ký/tên object |
| StateTracker.h/.cpp | State tracker modern |
| VersionGate.cpp | `GateCheck`, `FindVersionGateIndex`, `VersionGateAllows` |
| Renderer.cpp | Renderer (shell) |

## 4. Source OpenGL (`src/gl/`, ~71 file)

### 4.1 Shader / program / compiler (vùng trọng tâm của đợt đại tu)

| File | Vai trò |
|------|---------|
| glsl_to_msl.cpp | **GLSL→MSL string rewriter (~13k LOC)** — cần thay bằng compiler pipeline có AST |
| GLSLConverter.h | Header converter (xem include) |
| GLSLBuiltins.h / GLSLTextUtil.h | Built-in pass registry |
| glsl_builtins_registry.cpp | Registry built-in pass |
| glsl_builtins_fn_math.cpp | Built-in nhóm toán học |
| glsl_builtins_fn_texture.cpp | Built-in nhóm texture |
| glsl_builtins_modern_io.cpp | Built-in moderno IO |
| glsl_builtins_legacy.cpp | Built-in GLSL 1.20 legacy |
| glsl_builtins_lang.cpp | Built-in ngôn ngữ |
| glsl_builtins_final.cpp | Built-in cuối (gl_BaseVertex, gl_DrawID, ...) |
| gl_shader.cpp | Shader object: `glCreateShader`/`glShaderSource`/`glCompileShader`/`glGetShader*`/`glSpecializeShader` |
| gl_program.cpp | Program link → MTLRender/ComputePipelineState (2081 LOC), varying match, attrib loc, uniform layout, GS/tess emulation |
| gl_geometry_emulation.cpp | GS emulation compute kernel |
| gl_tess_xfb.cpp | Tessellation + transform feedback |
| gl_uniform.cpp | `glUniform*`, `glGetUniform*`, `glShaderStorageBlockBinding` |
| gl_vertex.cpp | Vertex attribute (`glVertexAttrib*`, `glEnableVertexAttribArray`, ...) |
| gl_buffer.cpp | Buffer object |
| gl_framebuffer.cpp | FBO |
| gl_texture.cpp | Texture |
| gl_sampler.cpp | Sampler object |
| gl_draw.cpp | Draw dispatch |
| gl_compute.cpp | Compute dispatch |
| gl_get.cpp | Mọi glGet* (1276 LOC) |
| gl_error_string_debug.cpp | `glGetString`, debug string |
| gl_context_api.cpp | C ABI context (versioned) |
| gl_c_exports.cpp | **Wrapper C ABI** 1049 entry point (sinh tự động), version-gate |
| gl_dispatch_gen.cpp | Bảng dispatch sinh tự động |
| gl_version_gate_gen.cpp | Bảng gate sinh tự động (1048 entry) |
| gl_stubs_gen.cpp | Stub sinh tự động (7 LOC) |
| gl_compat_tables_gen.cpp | Bảng cap/pname compat-only sinh tự động |
| gl_raster.cpp | Raster state |
| gl_sync_query.cpp | Sync/query |
| gl_debug_p4.cpp | KHR_DEBUG |
| gl_draw_buffers2_p8.cpp | Draw buffers blend (indexed) |
| gl_multisample_p8.cpp | Multisample |
| gl_packed_robust.cpp | Packed/robust |
| gl_program_interface_p10.cpp | Program interface query |
| gl_readpixels_dsa.cpp | ReadPixels DSA |
| gl_shader_objects_p8.cpp | Shader objects p8 |
| gl_ext_*.cpp | Các extension (imaging, dsa, bindless, perf, state, misc, path_command, include_shading, uniform64, attrib64) |
| gl_legacy_*.cpp | Fixed-function (matrix, vertex, immediate, lighting, fog, texenv, raster, pixelops, evaluator, selection, displaylist, clientarrays, attribstack, colorindex, misc, rect, typevariants, alpha, beginend) |
| gl_apple_draw.cpp | Apple Metal draw path |
| glfw_shim.cpp | GLFW shim / GetProcAddress fallback |

## 5. Source Metal (`src/metal/`)

| File | Vai trò |
|------|---------|
| AppleMetalBridge.mm | Backend Metal thật (Objective-C++, ARC), compile library, pipeline, encoder, draw |
| NullMetalBackend.cpp | Backend Null (CPU/CI), đủ test state machine |

## 6. Test (`tests/`)

| Thư mục | Mục đích | Số file |
|---------|----------|---------|
| tests/unit | Unit theo subsystem | 113 |
| tests/integration | Luồng nhiều API (Apple Metal) | 77 |
| tests/regression | Hồi quy hành vi (bao gồm shader semantics) | 14 |
| tests/shaders | Biên dịch GLSL (coverage, builtin, corpus) | 12 |
| tests/api | Bề mặt API | 1 |
| tests/conformance | Conformance surface | 1 |
| tests/corpus/ | Corpus shader (Minecraft 26.1.2) | — |
| tests/pch_stub.cpp | PCH stub |
| tests/truth/ | Probe khoảng trống (truth_probe) | — |
| tests/test_util.h | `TGLMT_CHECK`, `TGLMT_RESULT` |

## 7. Tool (`tools/`)

| File | Vai trò |
|------|---------|
| gen_gl_headers.py | Sinh gl46_types.h, gl_legacy_types.h, gl46.h, gl_legacy.h |
| gen_version_inventory.py | Sinh data/version_inventory.json từ gl.xml |
| gen_version_gate.py | Sinh gl_version_gate_gen.cpp |
| gen_c_exports.py | Sinh gl_c_api.h + gl_c_exports.cpp |
| gen_stubs.py | Sinh gl_stubs_gen.cpp |
| gen_compat_tables.py | Sinh gl_compat_tables_gen.cpp |
| gen_glsl_builtin_funcs.py | Sinh glsl_builtin_funcs.inc |
| gen_coverage_report.py | Sinh API_COVERAGE_REPORT.md |
| glsl_probe.cpp | Công cụ chẩn đoán gap GLSL |
| glsl2msl_cli.cpp | CLI convert GLSL→MSL |
| dump_msl_corpus.cpp | Dump MSL corpus |
| ext_*.py | Audit extension |
| pack_conformance.py / pack_glsl_preprocess.py | Xử lý shaderpack |
| verify_msl_corpus.sh | Xác minh MSL corpus |
| agent_syntax_check.sh | `-fsyntax-only` cho agent (không tạo build dir) |

## 8. Docs (`docs/`)

| File | Vai trò |
|------|---------|
| ARCHITECTURE.md | Kiến trúc tổng thể |
| DECISIONS.md | Quyết định kiến trúc đã chốt (A1–A10, D-001..D-012) |
| GLSL_460_COMPILER.md | Mô tả compiler hiện tại (string rewriter) |
| GEOMETRY_SHADER_EMULATION.md | Thiết kế GS emulation |
| LEGACY_TO_METAL_MAPPING.md | Ánh xạ fixed-function → Metal |
| METAL_A11_CAPABILITIES.md | Khả năng A11 |
| OPENGL_VERSION_MATRIX.md | Ma trận version × profile |
| GLSL_VERSION_MATRIX.md | Ma trận GLSL version |
| EXTENSION_MATRIX.md | Ma trận extension |
| IMPLEMENTATION_ROADMAP.md | Lộ trình triển khai |
| SHADER_TRANSLATION_PERFORMANCE.md | Hiệu năng dịch shader |
| REPO_DISCOVERY.md / SOURCE_MAP.md / SUBSYSTEM_DEPENDENCY_GRAPH.md | Khám phá repo (session trước) |
| docs/audit/ | **(mới) Báo cáo audit giai đoạn này** |

## 9. Ranh giới ownership (theo Mục 8 của prompt)

- Agent 0 Lead: kiến trúc, ADR, interface, `docs/audit/`, `docs/compiler/`, `docs/plan/`, PROGRESS.md/TODO.md, data/version_inventory.json
- Agent 1 Frontend: `src/compiler/frontend/`, `include/tglmt/compiler/frontend/`
- Agent 2 IR/passes: `src/compiler/ir/`, `src/compiler/passes/`
- Agent 3 Backend: `src/compiler/backend/metal/`
- Agent 4 API: `src/gl/` (bên ngoài compiler/link do Lead phân quyền), `include/tglmt/gl46*`
- Agent 5 Lifecycle: `src/gl/gl_shader.cpp`, `src/gl/gl_program.cpp`, reflection, Metal integration
- Agent 6 Test: `tests/`
- Agent 7 Doc/Perf: `docs/`, `benchmarks/`

---
*Tiếp theo: docs/audit/OPEN_ISSUES.md, FEATURE_INVENTORY.md*
