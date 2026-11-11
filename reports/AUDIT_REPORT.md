# Audit Report — TGLMT OpenGL 4.6 Core on Metal

**Scope**: the whole repository at `10ff95f` (working tree), audited against
the official Khronos OpenGL 4.6 Core Profile specification and Apple's Metal
documentation.

**Method**: five parallel read-only investigations (API catalog, state/object
management, GLSL compiler, Metal backend, synchronisation/framebuffer/texture/
buffer, test & benchmark infrastructure), each producing file:line evidence;
the findings were then cross-checked by the lead engineer and every claim that
survived was re-verified against the source or reproduced at runtime.

---

## 1. Headline: the project did **not** implement 698 OpenGL 4.6 Core APIs

The project advertised **383/698**. Both numbers were wrong, in different ways.

### 1.1 "698" is not the OpenGL 4.6 Core Profile count — it is **657**

The catalog was derived from `docs/khronos/gl.xml` by three tools
(`tools/truth_manifest.py`, `tools/gen_gl_headers.py`, `tools/gen_stubs.py`),
each with its own copy of the derivation and the same two defects:

| Defect | Effect |
|---|---|
| `<require profile="compatibility">` was not honoured | **42** compatibility-profile-only entry points were counted as *core* |
| `<remove>` was applied *after* all `<require>` instead of in version order | `glGetPointerv` (removed for core in GL 3.2, re-required by `GL_VERSION_4_3`) was dropped |

`698 = 657 (real core) + 42 (compatibility-only) − 1 (glGetPointerv)`.

**The correct number is 657.** It was verified three ways:

1. The registry's own version-ordered profile semantics
   (`tools/gl46_catalog.py`, now the single source of truth).
2. The official Khronos-generated `glcorearb.h` from the same `gl.xml`:
   **657 prototypes, set-equal** to our derivation. None of the 42
   compatibility-only names appears in that header.
3. `glspec46.core.pdf` §22.2 documents `GetPointerv` as a core command, and
   the 4.3 change log says "The GetPointerv command … re-introduced to the
   OpenGL 4.3 core profile".

The 42 wrongly-counted functions are the `glVertexP*` / `glTexCoordP*` /
`glMultiTexCoordP*` / `glNormalP*` / `glColorP*` / `glSecondaryColorP*` family
(`GL_VERSION_3_3`) and the 12 `glGetn*` functions (`GL_VERSION_4_5`). All 42
occur **zero times** in the core specification text.

**Status after this work**: the derivation is fixed, the three tools share one
module, the hard-coded `assert len(core46) == 698` is gone, and the manifest
reports core and compatibility-only separately.

### 1.2 "383" was never an implementation count

`383` appeared in exactly two places, both in `docs/TRUTH_REPORT.md` (a file
that was **git-ignored**, so the audit trail was outside version control).
It was a snapshot of the manifest counter `core_without_test_refs` from
commit `51e2da9`. It has nothing to do with how many APIs are implemented.

The current, measured figures are in `reports/API_COVERAGE_REPORT.md`.

---

## 2. Audit findings by severity

### 2.1 CRITICAL

| # | Finding | Location | Status |
|---|---|---|---|
| C1 | The "OpenGL 4.6 Core" catalog included 42 compatibility-profile-only functions and excluded `glGetPointerv` | `tools/truth_manifest.py`, `tools/gen_gl_headers.py`, `tools/gen_stubs.py` | **Fixed** |
| C2 | `glShaderSource` with a NULL string pointer dereferenced NULL ⇒ **SIGSEGV** | `src/gl/gl_shader.cpp:29` | **Fixed** |
| C3 | `glUniform*fv` / `glProgramUniform*fv` with `count < 0` computed a huge byte count ⇒ `std::length_error` ⇒ `std::terminate` (**abort**, not catchable) | `src/gl/gl_uniform.cpp` | **Fixed** |
| C4 | `glBufferData` / `glBufferStorage` on a **mapped** buffer replaced the storage ⇒ the application's persistent-mapping pointer dangled ⇒ **use-after-free** | `src/gl/gl_buffer.cpp:432, 486` | **Fixed** (now `INVALID_OPERATION`) |
| C5 | The pipeline cache keyed on the **raw address** of the `ILibrary`. `glLinkProgram` frees the old library and the allocator frequently hands back the same address ⇒ the cache returned a pipeline compiled from the **previous** shader. The new shader silently never took effect. | `src/metal/AppleMetalBridge.mm:2704`, `include/tglmt/Context.h:982` | **Fixed** (`ILibrary::identity()` + `ProgramObject::linkGen`) |
| C6 | `glUniformMatrix*fv` discarded the `transpose` flag ⇒ matrices were transposed | `src/gl/gl_uniform.cpp:243-260` | **Fixed** |
| C7 | A `mat3` uniform was uploaded as 36 tight bytes into a 48-byte MSL slot (3 columns × 16-byte stride) ⇒ columns 1 and 2 landed 4/8 bytes early ⇒ **garbage matrices** | `src/gl/gl_apple_draw.cpp` upload path | **Fixed** (`include/tglmt/uniform_pack.h`) |

### 2.2 HIGH

| # | Finding | Location | Status |
|---|---|---|---|
| H1 | `texture(s, uv, bias)` emitted `sample(s, coord, float(bias))`; MSL overload resolution picked the **`int2 offset`** overload ⇒ the LOD bias became a texture offset and was **silently discarded** | `src/gl/glsl_to_msl.cpp:5512` | **Fixed** (emit `bias(float(x))`) |
| H2 | `flat` / `noperspective` / `centroid` interpolation qualifiers were stripped without emitting the corresponding MSL attribute ⇒ GLSL `flat` varyings were **smoothly interpolated** | `src/gl/glsl_to_msl.cpp:345` | **Fixed** |
| H3 | A single fragment output at `layout(location=N)`, N≠0, was written to **attachment 0** | `src/gl/glsl_to_msl.cpp:10269` | **Fixed** |
| H4 | `textureQueryLod` was not rewritten ⇒ the generated MSL referenced the GLSL sampler name ⇒ **Metal compile failure at link time** | `src/gl/glsl_to_msl.cpp` | **Fixed** |
| H5 | **No state-query entry point ever raised `GL_INVALID_ENUM`** — `glGetIntegerv`, `glGetFloatv`, `glGetBooleanv`, `glGetDoublev`, `glGetInteger64v` and all indexed variants silently returned 0 for an unknown pname | `src/gl/gl_get.cpp` (0 occurrences of `0x0500`) | **Fixed** (965 accepted pnames extracted from the spec's state tables) |
| H6 | `glBindTexture` never validated `target`; an unknown target fell into the 2D slot | `src/gl/gl_texture.cpp:520` | **Fixed** |
| H7 | `glDrawElements` with an invalid index `type` produced **no error on the Metal path** (the type was only validated inside a null-backend-only branch) and the draw was silently dropped; an invalid `mode` still drew triangles | `src/gl/gl_draw.cpp` | **Fixed** |
| H8 | The five `glInvalidate*` entry points were **empty bodies** — no validation, no error generation | `src/gl/gl_texture.cpp:2992`, `src/gl/gl_framebuffer.cpp:978` | **Fixed** (and the misleading "allowed by name prefix" triage was removed) |
| H9 | `glDeleteBuffers` / `glDeleteProgram` / `glDeleteVertexArrays` / `glDeleteSamplers` left the deleted object **bound** | `src/gl/gl_buffer.cpp:319`, `gl_program.cpp:90`, `gl_vertex.cpp:18`, `gl_sampler.cpp:18` | **Fixed** |
| H10 | `glDeleteShader` deleted a shader object **while it was still attached** to a program ⇒ relinking failed with "missing shader" | `src/gl/gl_shader.cpp:17` | **Fixed** |
| H11 | `glClientWaitSync` **always** returned `ALREADY_SIGNALED` and never blocked; `glWaitSync` "completed" immediately ⇒ no CPU-side GPU synchronisation exists | `src/gl/gl_sync_query.cpp:26` | **Open** — documented as a gap (`docs/METAL_A11_LIMITATIONS.md`) |
| H12 | `glBeginQuery` for an occlusion target did **not** flush the open render encoder, so the pass descriptor had no visibility buffer ⇒ the occlusion result was missing or invalid | `src/gl/gl_sync_query.cpp:112` | **Open** |
| H13 | No texture-completeness model: a mipmap-filtered texture with only level 0 samples level 0 instead of black | `src/gl/gl_apple_draw.cpp:331` | **Open** — documented divergence |
| H14 | `GL_COMPRESSED_TEXTURE_FORMATS` advertises BC1–BC7/S3TC on every device, but A11 has **no BC decoder**; `docs/EXTENSION_MATRIX.md` claims a "CPU decode fallback" that does not exist | `src/gl/gl_texture.cpp:2552`, `src/metal/AppleMetalBridge.mm:1436` | **Open** — documented |
| H15 | `GL_MAX_TEXTURE_IMAGE_UNITS` is reported as **32**, but Metal's tier-1 table allows **31** ⇒ a 32-sampler shader aborts Metal validation | `src/gl/gl_get.cpp:135` | **Open** — documented |
| H16 | `GL_LINEAR` + scaling in `glBlitFramebuffer` is implemented as **nearest** ⇒ every scaled blit aliases | `src/gl/gl_framebuffer.cpp:695` | **Open** — documented |
| H17 | `glReadPixels` only supports RGBA8 sources; a float attachment returns quantised or zero data | `src/gl/gl_readpixels_dsa.cpp:44` | **Open** — documented |
| H18 | `glTexStorage2D`/`glTexStorage3D` never set the immutable flag ⇒ immutability is not enforced and `GL_TEXTURE_IMMUTABLE_FORMAT`/`_LEVELS` are unimplemented | `src/gl/gl_texture.cpp:3007` | **Open** — documented |
| H19 | The sampler-state cache is **unbounded** — one `MTLSamplerState` per texture id for the lifetime of the process | `src/gl/gl_apple_draw.cpp:211` | **Open** |
| H20 | The wrap cache keys on raw `ITexture*` ⇒ an in-place texture reallocation can alias (ABA) and route draws into a detached texture | `include/tglmt/Context.h:1118` | **Open** |
| H21 | The index-rewrite cache is **global**, not per-context ⇒ two contexts using the same buffer name share rewritten indices | `src/gl/gl_apple_draw.cpp:653` | **Open** |
| H22 | The fallback context is **process-global** and shared by every thread that never called `MakeCurrent` ⇒ concurrent GL calls from two threads corrupt the same maps | `src/core/Context.cpp:114` | **Open** — documented |
| H23 | `gInflightCB` (a `std::deque`) is mutated without a mutex ⇒ concurrent `commitAndWait` corrupts it | `src/metal/AppleMetalBridge.mm:43` | **Open** |
| H24 | 13 test files used `assert()` as their only failure mechanism ⇒ **compiled out in Release**, so the tests passed unconditionally | 13 files under `tests/` | **Fixed** (`tests/test_util.h`) |
| H25 | 10 tests silently skipped under CTest because they resolve fixtures through relative paths and CTest ran them with CWD = the build directory. These included **the strongest pixel-level tests in the repository**. | `CMakeLists.txt` (`add_test`) | **Fixed** (`WORKING_DIRECTORY`) |
| H26 | The "does the Metal compiler accept our MSL?" gate wrote `.metal` but compiled `.msl` ⇒ Metal treated the input as a linker input, exited 0 and produced no `.air` ⇒ **the gate never fired** | `tests/unit/test_converter_aquarium.cpp:73` | **Fixed** |
| H27 | The whole `docs/` tree was **git-ignored** ⇒ the audit trail was outside version control | `.gitignore:32` | **Fixed** |

### 2.3 MEDIUM

| # | Finding | Location |
|---|---|---|
| M1 | The context pipeline cache key omitted the depth **format** (only a bool), while the Metal bridge's key included it ⇒ the two caches disagree by construction | `include/tglmt/Context.h:974` |
| M2 | The context pipeline cache key recorded `pr.appleFS` while the pipeline was built from `pr.appleFSMasked` when MSAA is on | `src/gl/gl_apple_draw.cpp:2337` |
| M3 | `glGetFramebufferAttachmentParameteriv` looked the colour attachment up by **enum** while the FBO stores **dense indices** ⇒ every colour attachment query returned `OBJECT_TYPE = GL_NONE` | `src/gl/gl_get_extra.cpp:695` |
| M4 | `INCOMPLETE_MULTISAMPLE` and `FRAMEBUFFER_UNSUPPORTED` are never returned | `src/gl/gl_framebuffer.cpp:441` |
| M5 | Size-mismatched attachments are rejected with `GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS` — a token removed in GL 3.0; in GL 4.6 a size mismatch is legal | `src/gl/gl_framebuffer.cpp:466` |
| M6 | `GL_TEXTURE_MIN_LOD`, `GL_TEXTURE_LOD_BIAS` and `GL_TEXTURE_COMPARE_MODE` on a texture (not a sampler object) are never read | `src/gl/gl_apple_draw.cpp:239` |
| M7 | All four GL mipmap filters map to Metal `Linear` (trilinear) | `src/metal/AppleMetalBridge.mm:2931` |
| M8 | `GL_UNPACK_IMAGE_HEIGHT` and `GL_UNPACK_SWAP_BYTES` / `GL_UNPACK_LSB_FIRST` are stored but never applied | `src/gl/gl_texture.cpp:1619` |
| M9 | `glGetBufferParameteri64v` truncates through `GLint` ⇒ sizes above 2 GiB are wrong | `src/gl/gl_get.cpp:579` |
| M10 | `glMapBufferRange` does not validate the access flags against the storage flags, does not reject `length == 0`, and does not reject a concurrent mapping | `src/gl/gl_buffer.cpp:619` |
| M11 | `glUnmapBuffer` always returns `GL_TRUE`; a lost mapping is never reported | `src/gl/gl_buffer.cpp:679` |
| M12 | `glGetBufferSubData` reads the GPU buffer without a GPU barrier for non-compute writers | `src/gl/gl_buffer.cpp:105` |
| M13 | `glMemoryBarrier` treats all bits identically; the per-bit GL semantics are not reproduced | `src/gl/gl_compute.cpp:459` |
| M14 | `GL_TIMESTAMP` is a CPU clock and reports `GL_GPU_DISJOINT_EXT = TRUE` (honest, but not a GPU timestamp) | `src/gl/gl_sync_query.cpp:200` |
| M15 | `glGetQueryiv(GL_QUERY_COUNTER_BITS)` returns 1 for occlusion targets (GL requires ≥ 8) | `src/gl/gl_sync_query.cpp:302` |
| M16 | `IComputeEncoder::dispatchGroups3D` is not overridden by **either** backend ⇒ the tessellation-control path cannot express "one threadgroup per patch" | `include/tglmt/MetalInterface.h:478` |
| M17 | No enforcement of the compute limits (1024 invocations, 32 KB shared memory) that the implementation reports | `src/gl/gl_compute.cpp:293` |
| M18 | Depth testing was unconditionally **disabled** on the default framebuffer, even though the renderer allocates a depth buffer for it | `src/gl/gl_apple_draw.cpp:2280` |
| M19 | MSAA depth is an internal Metal attachment, not the application's depth texture ⇒ depth readback from an MSAA FBO is wrong | `src/metal/AppleMetalBridge.mm:2223` |
| M20 | `glBlitFramebuffer` handles only colour attachment 0 for FBO→FBO | `src/gl/gl_framebuffer.cpp:827` |
| M21 | Every texture upload wrote the texture **twice** (a duplicated `replaceRegion`) | `src/metal/AppleMetalBridge.mm` |
| M22 | `glDrawTransformFeedback` replays the captured vertex count with no attributes | `src/gl/gl_draw.cpp:515` |
| M23 | **All 18 `glProgramUniformMatrix*fv/dv` (DSA) ignored the `transpose` argument** (`(void)t`) ⇒ every matrix uploaded through the DSA path was silently transposed (or mis-laid-out for non-square matrices), while the non-DSA `glUniformMatrix*` honoured it | `src/gl/gl_uniform.cpp` |
| M25 | **All 18 `glProgramUniformMatrix*fv/dv` (DSA) ignored `transpose`** ⇒ ma trận bị đảo chiều âm thầm; đường non-DSA thì đúng ⇒ hai đường cho kết quả khác nhau | `src/gl/gl_uniform.cpp` |
| M26 | `glGetUniformdv` đọc byte của uniform `double` qua bộ đệm **float** ⇒ trả về giá trị rác | `src/gl/gl_get.cpp` |
| M27 | `glGetVertexAttribiv` dùng sai enum `0x88FD` cho `GL_VERTEX_ATTRIB_ARRAY_DIVISOR` (đúng là `0x88FE`) ⇒ luôn trả 0 | `src/gl/gl_get_extra.cpp` |
| M28 | `glGetVertexArrayiv` dùng sai enum `0x889F4` cho `GL_ELEMENT_ARRAY_BUFFER_BINDING` (đúng là `0x8895`) ⇒ luôn trả 0 + INVALID_ENUM | `src/gl/gl_get_extra.cpp` |
| M29 | Tra attachment FBO bằng **enum** trong khi FBO lưu theo **index dày** ⇒ mọi truy vấn attachment trả `OBJECT_TYPE = NONE` | `src/gl/gl_get_extra.cpp` |
| M30 | Attach **renderbuffer** bị flatten thành texture ⇒ `OBJECT_TYPE` báo `GL_TEXTURE` thay vì `GL_RENDERBUFFER` | `src/gl/gl_framebuffer.cpp` |
| M31 | `glClearBufferfi(GL_COLOR, …)` bị từ chối (INVALID_ENUM) dù spec §9.2.3 cho phép clear color bằng giá trị float | `src/gl/gl_framebuffer.cpp` |
| M32 | `glGetConvolutionParameteriv` ghi **4** số nguyên trong khi spec chỉ cho **2** ⇒ **ghi đè stack của app** (ASan) | `src/gl/gl_ext_imaging.cpp` |
| M33 | `glDepthRangeArrayv`/`glViewportArrayv`/`glViewportIndexedfv`/`glTexParameteriv`… deref con trỏ NULL không kiểm tra ⇒ **SEGFAULT** | `src/gl/gl_raster.cpp`, `src/gl/gl_texture.cpp` |
| M34 | Trạng thái blend là **per draw buffer** nhưng `glGetIntegeri_v` luôn đọc buffer 0 | `src/gl/gl_get.cpp` |
| M35 | `glDepthRangef` không ghi shadow ⇒ `glGetFloatv(GL_DEPTH_RANGE)` trả giá trị CŨ | `src/gl/gl_raster.cpp` |
| M36 | GLSL front end **từ chối mọi uniform `double`/`dvec`/`dmat`** dù GLSL 4.60 định nghĩa và Metal có bản tương ứng (float) | `src/gl/glsl_to_msl.cpp` |
| M37 | `glGetTextureParameterfv` đọc qua bản int ⇒ cắt mất phần thập phân của float param | `src/gl/gl_get_extra.cpp` |
| M24 | The GLSL front end rejected **every non-square matrix uniform** (`mat2x3`, `mat3x2`, `mat2x4`, `mat4x2`, `mat3x4`, `mat4x3`) with "uniform kiểu lạ", although GLSL 4.60 defines them and Metal has an exact counterpart (`float2x3`, …) | `src/gl/glsl_to_msl.cpp` |

### 2.4 Findings that turned out to be **false positives**

Recorded because "the audit found it" is not the same as "the audit is right":

| Reported finding | Reality |
|---|---|
| "Metal rejects `MTLPixelFormatInvalid` in a pipeline colour attachment, so MRT with a `GL_NONE` draw buffer fails" | **Verified against the real Metal compiler**: a hole in the colour attachments is **accepted**. No workaround needed. (`/tmp/metaltest/t.m`) |
| "`glLineStipple`/`glDepthRangeArrayfvNV` are missing core functions" | They are extension entry points, correctly listed as *bonus* — but `glGetPointerv`, which was in that list, **is** core since GL 4.3 |
| "`GL_LINE_LOOP` is not a valid primitive mode in core" | **False** — GL 4.6 §10.1.3 documents "A line loop is specified with mode LINE_LOOP". The implementation maps it to a line strip and expands the loop on the CPU |
| "The behaviour-gap scanner's first hit (stencil/baseInstance are trace-only) is accurate" | **False** — both are fully implemented; the scanner's hits are dominated by stale comments |

---

## 3. Structural findings

| # | Finding | Impact |
|---|---|---|
| S1 | The GLSL front end is a **token/string rewriter**, not a compiler: no AST, no type system, no overload resolution | Unknown identifiers are not diagnosed at GLSL compile time; they fail at Metal compile time (link) |
| S2 | There is **no function-pointer dispatch table**; every entry point is a direct function ⇒ validation is per-entry-point and easily missed (which is exactly how 55 % of the core entry points ended up with no error check) | Structural cause of the error-generation gaps |
| S3 | The null backend is **not** behaviourally equivalent to the Apple backend (MRT collapsed, uploads ignored, no mips, fixed 8×8 compute grid) | GPU-only behaviour is untested without a Metal device |
| S4 | Three independent copies of the API-catalog derivation existed | The "698" error was replicated three times and frozen by an `assert` |
| S5 | `glGetString(GL_VERSION)` does not report "Core" | Not required by the spec (the profile is reported by `GL_CONTEXT_PROFILE_MASK`), but the string is not self-describing |

---

## 4. What was done in response

| Area | Result |
|---|---|
| API catalog | Fixed derivation (657 core / 42 compatibility-only), one shared module, generated artefacts regenerated, hard-coded counts removed |
| Error generation | `glGet*` INVALID_ENUM (965 accepted pnames from the official state tables), `glBindTexture` target validation, `glShaderSource` NULL/count, `glUniform*fv` count, draw mode/type, the five `Invalidate*` entry points |
| Object lifetime | Buffers, programs, VAOs and samplers are unbound on deletion; `glDeleteShader` respects attachment; mapped buffers may not be re-specified |
| Shader semantics | texture bias, interpolation qualifiers, fragment output location, `textureQueryLod`, matrix transpose (**both** the `glUniformMatrix*` and the DSA `glProgramUniformMatrix*` paths), matrix packing, non-square matrix uniforms (`mat2x3` …) |
| Metal backend | Pipeline cache identity (`ILibrary::identity()` + `linkGen`), default-framebuffer depth test, single texture upload |
| Test infrastructure | `assert()` → always-on checks, CTest working directory, Metal-compiler gate, `docs/` tracked in git |
| Measurement | New benchmark family (compilation / runtime / upload) with a documented protocol, a noise floor and control metrics |
| API surface | **Toàn bộ 657 hàm core giờ có test trực tiếp** (trước: 339). 11 suite mới trong `tests/regression/` + `tests/api/` + `tests/shaders/` + `tests/conformance/` |
| Conformance | Validation uniform §7.6.1 đầy đủ (sai kiểu/sai kích thước/sampler ngoài dải/location không hợp lệ) — trước đây mọi sai lệch bị nuốt im lặng |
| Bộ nhớ | Toàn bộ suite chạy dưới **AddressSanitizer**: 7 lỗi bộ nhớ thật được phát hiện và sửa (ghi đè stack của app, đọc tràn heap/stack) |
| Hiệu năng | Gộp 2 bảng hash uniform thành 1 + bỏ tra shadow khi không có pipeline ⇒ `glUniform1f` **29.4 ns** (batched 64) *với* validation đầy đủ (trước: 2 tra hash + không validate) |
| Documentation | 9 new/rewritten documents + 6 reports, all generated from real data where possible |

---

## 5. What remains open

See `docs/IMPLEMENTATION_ROADMAP.md` for the full list with priorities. The
most significant:

1. **Khronos CTS has never been run** — the largest conformance unknown.
2. **No A11 device run** was performed in this session.
3. `glClientWaitSync` does not block (no Metal event).
4. No texture-completeness model.
5. The GLSL front end is not a compiler.
6. The context is not thread-safe and the fallback context is process-global.
