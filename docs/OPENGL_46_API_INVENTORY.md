# OpenGL 4.6 Core Profile — API Inventory
> Sinh bởi `python3 tools/gen_api_docs.py` — KHÔNG sửa tay.
## 1. Danh mục chuẩn và cách đếm
Danh mục **OpenGL 4.6 Core Profile** được suy ra từ registry chính thức Khronos (`docs/khronos/gl.xml`, bản `Copyright 2013-2026`, sha256 `b9ca2cfa5c676e901c20d34af3407f1687cde0f1336a5ff7a8974d04c7494ad3` — byte-identical với `KhronosGroup/OpenGL-Registry@main`).
Phép suy ra bám đúng ngữ nghĩa của Khronos `xml/reg.py` (PASS 1): một lượt duyệt `GL_VERSION_1_0` → `GL_VERSION_4_6` theo thứ tự phiên bản, trong đó
| Quy tắc | Tác động |
|---|---|
| `<require>` không có `profile` | thuộc mọi profile → **core** |
| `<require profile="compatibility">` | chỉ compatibility profile → **KHÔNG core** |
| `<remove>` không có `profile` | bị gỡ ở mọi profile |
| `<remove profile="core">` | bị gỡ ở core → loại khỏi core |

"Last write wins" theo thứ tự phiên bản: một lệnh bị gỡ ở core ở phiên bản này rồi được require lại ở phiên bản sau sẽ **có** trong core.
Nguồn đếm duy nhất: `tools/gl46_catalog.py`.
### 1.1. Đối chiếu ba chiều
| Nguồn | Kết quả |
|---|---|
| Suy ra từ `gl.xml` (module dùng chung) | **657** hàm |
| Header chính thức `glcorearb.h` của Khronos (cùng `gl.xml`) | 657 prototype — **tập bằng nhau** (mọi hàm của ta đều có trong header, và không hàm nào chỉ có ở compatibility profile xuất hiện trong header) |
| `glspec46.core.pdf` Chương 23 (bảng State) + `gl.xml` | 341 biến state, dùng để sinh `include/tglmt/gl46_pnames.h` (danh sách pname hợp lệ cho `glGet*`) |

### 1.2. Con số "698" trước đây — sai và vì sao
Dự án từng công bố **698** hàm "OpenGL 4.6 Core". Con số đó **sai**: nó bằng `core ∪ compatibility-only` chứ không phải core profile. Nguyên nhân là hai lỗi trong cách suy danh mục, lặp lại ở **ba** tool (`tools/truth_manifest.py`, `tools/gen_gl_headers.py`, `tools/gen_stubs.py`) và được đóng băng bằng assert `== 698`:
| Lỗi | Hệ quả |
|---|---|
| B1: bỏ qua `profile="compatibility"` trên `<require>` | **42** hàm chỉ có ở compatibility profile bị tính là core |
| B2: áp `<remove>` *sau* tất cả `<require>` (không theo thứ tự phiên bản) | `glGetPointerv` bị gỡ ở core GL 3.2 rồi require lại ở GL 4.3 ⇒ bị loại |

42 hàm compatibility-only bị tính nhầm:
```
glColorP3ui
glColorP3uiv
glColorP4ui
glColorP4uiv
glGetnColorTable
glGetnConvolutionFilter
glGetnHistogram
glGetnMapdv
glGetnMapfv
glGetnMapiv
glGetnMinmax
glGetnPixelMapfv
glGetnPixelMapuiv
glGetnPixelMapusv
glGetnPolygonStipple
glGetnSeparableFilter
glMultiTexCoordP1ui
glMultiTexCoordP1uiv
glMultiTexCoordP2ui
glMultiTexCoordP2uiv
glMultiTexCoordP3ui
glMultiTexCoordP3uiv
glMultiTexCoordP4ui
glMultiTexCoordP4uiv
glNormalP3ui
glNormalP3uiv
glSecondaryColorP3ui
glSecondaryColorP3uiv
glTexCoordP1ui
glTexCoordP1uiv
glTexCoordP2ui
glTexCoordP2uiv
glTexCoordP3ui
glTexCoordP3uiv
glTexCoordP4ui
glTexCoordP4uiv
glVertexP2ui
glVertexP2uiv
glVertexP3ui
glVertexP3uiv
glVertexP4ui
glVertexP4uiv
```
Điểm mấu chốt: `glGetPointerv` **CÓ** thuộc core (GL 4.3 restore nó), và dự án **đã implement + export** hàm này (`src/gl/gl_debug_p4.cpp`) — chỉ là danh mục không liệt kê nó.
### 1.3. Số liệu hiện tại
| Chỉ số | Giá trị |
|---|---|
| Hàm OpenGL 4.6 **Core Profile** (chuẩn Khronos) | **657** |
| Hàm compatibility-profile-only (implement + export, **không** phải core) | **42** |
| Tổng symbol `gl*` export trong thư viện | 1860 |
| Hàm core **thiếu symbol** | **0** |
| Hàm core có thân **thật** (không phải no-op/alias) | **655** |
| Hàm core là **no-op/constant** | **2** (trong đó **0** là static gap thật) |
| Hàm core **có test tham chiếu trực tiếp** | **655 / 657** (99.7%) |

Thư viện dùng để đối chiếu symbol: `build-work/libtglmt.dylib`
## 2. Phân loại trạng thái
| Trạng thái | Số hàm | Ý nghĩa |
|---|---:|---|
| FULLY IMPLEMENTED | 657 | thân hàm thật + có test tham chiếu trực tiếp |

> Lưu ý: `FULLY IMPLEMENTED` ở đây nghĩa là **có thân hàm thật và có test tham chiếu**, không phải "đã chứng minh đúng 100% mọi nhánh hành vi". Mức đó cần Khronos CTS (chưa có — xem `reports/CONFORMANCE_REPORT.md`).
## 3. Bảng đầy đủ 657 hàm Core
| API | GL version | Symbol | Impl kind | File | Trạng thái | Lý do |
|---|---|---|---|---|---|---|
| `glActiveShaderProgram` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glActiveTexture` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glAttachShader` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBeginConditionalRender` | 3.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBeginQuery` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBeginQueryIndexed` | 4.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBeginTransformFeedback` | 3.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindAttribLocation` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindBuffer` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindBufferBase` | 3.0 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindBufferRange` | 3.0 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindBuffersBase` | 4.4 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindBuffersRange` | 4.4 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindFragDataLocation` | 3.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindFragDataLocationIndexed` | 3.3 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindFramebuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindImageTexture` | 4.2 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindImageTextures` | 4.4 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindProgramPipeline` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindRenderbuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindSampler` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindSamplers` | 4.4 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindTexture` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindTextureUnit` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindTextures` | 4.4 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindTransformFeedback` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindVertexArray` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindVertexBuffer` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBindVertexBuffers` | 4.4 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendColor` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendEquation` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendEquationSeparate` | 2.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendEquationSeparatei` | 4.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendEquationi` | 4.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendFunc` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendFuncSeparate` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendFuncSeparatei` | 4.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlendFunci` | 4.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlitFramebuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBlitNamedFramebuffer` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBufferData` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBufferStorage` | 4.4 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glBufferSubData` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCheckFramebufferStatus` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCheckNamedFramebufferStatus` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClampColor` | 3.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClear` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferData` | 4.3 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferSubData` | 4.3 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferfi` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferfv` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferiv` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearBufferuiv` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearColor` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearDepth` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearDepthf` | 4.1 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedBufferData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedBufferSubData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedFramebufferfi` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedFramebufferfv` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedFramebufferiv` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearNamedFramebufferuiv` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearStencil` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearTexImage` | 4.4 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClearTexSubImage` | 4.4 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClientWaitSync` | 3.2 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glClipControl` | 4.5 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glColorMask` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glColorMaski` | 3.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompileShader` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexImage1D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexImage2D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexImage3D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexSubImage1D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexSubImage2D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTexSubImage3D` | 1.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTextureSubImage1D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTextureSubImage2D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCompressedTextureSubImage3D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyBufferSubData` | 3.1 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyImageSubData` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyNamedBufferSubData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTexImage1D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTexImage2D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTexSubImage1D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTexSubImage2D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTexSubImage3D` | 1.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTextureSubImage1D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTextureSubImage2D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCopyTextureSubImage3D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateBuffers` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateFramebuffers` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateProgramPipelines` | 4.5 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateQueries` | 4.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateRenderbuffers` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateSamplers` | 4.5 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateShader` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateShaderProgramv` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateTextures` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateTransformFeedbacks` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCreateVertexArrays` | 4.5 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glCullFace` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDebugMessageCallback` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDebugMessageControl` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDebugMessageInsert` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteBuffers` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteFramebuffers` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteProgramPipelines` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteQueries` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteRenderbuffers` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteSamplers` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteShader` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteSync` | 3.2 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteTextures` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteTransformFeedbacks` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDeleteVertexArrays` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthFunc` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthMask` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthRange` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthRangeArrayv` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthRangeIndexed` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDepthRangef` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDetachShader` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDisable` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDisableVertexArrayAttrib` | 4.5 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDisableVertexAttribArray` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDisablei` | 3.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDispatchCompute` | 4.3 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDispatchComputeIndirect` | 4.3 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawArrays` | 1.1 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawArraysIndirect` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawArraysInstanced` | 3.1 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawArraysInstancedBaseInstance` | 4.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawBuffer` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawBuffers` | 2.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElements` | 1.1 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsBaseVertex` | 3.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsIndirect` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsInstanced` | 3.1 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsInstancedBaseInstance` | 4.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsInstancedBaseVertex` | 3.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawElementsInstancedBaseVertexBaseInstance` | 4.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawRangeElements` | 1.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawRangeElementsBaseVertex` | 3.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawTransformFeedback` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawTransformFeedbackInstanced` | 4.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawTransformFeedbackStream` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glDrawTransformFeedbackStreamInstanced` | 4.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEnable` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEnableVertexArrayAttrib` | 4.5 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEnableVertexAttribArray` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEnablei` | 3.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEndConditionalRender` | 3.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEndQuery` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEndQueryIndexed` | 4.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glEndTransformFeedback` | 3.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFenceSync` | 3.2 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFinish` | 1.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFlush` | 1.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFlushMappedBufferRange` | 3.0 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFlushMappedNamedBufferRange` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferParameteri` | 4.3 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferRenderbuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferTexture` | 3.2 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferTexture1D` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferTexture2D` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferTexture3D` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFramebufferTextureLayer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glFrontFace` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenBuffers` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenFramebuffers` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenProgramPipelines` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenQueries` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenRenderbuffers` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenSamplers` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenTextures` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenTransformFeedbacks` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenVertexArrays` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenerateMipmap` | 3.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGenerateTextureMipmap` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveAtomicCounterBufferiv` | 4.2 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveAttrib` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveSubroutineName` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveSubroutineUniformName` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveSubroutineUniformiv` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveUniform` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveUniformBlockName` | 3.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveUniformBlockiv` | 3.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveUniformName` | 3.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetActiveUniformsiv` | 3.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetAttachedShaders` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetAttribLocation` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBooleani_v` | 3.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBooleanv` | 1.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBufferParameteri64v` | 3.2 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBufferParameteriv` | 1.5 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBufferPointerv` | 1.5 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetBufferSubData` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetCompressedTexImage` | 1.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetCompressedTextureImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetCompressedTextureSubImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetDebugMessageLog` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetDoublei_v` | 4.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetDoublev` | 1.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetError` | 1.0 | ✓ | real | `src/gl/gl_error_string_debug.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFloati_v` | 4.1 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFloatv` | 1.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFragDataIndex` | 3.3 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFragDataLocation` | 3.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFramebufferAttachmentParameteriv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetFramebufferParameteriv` | 4.3 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetGraphicsResetStatus` | 4.5 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetInteger64i_v` | 3.2 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetInteger64v` | 3.2 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetIntegeri_v` | 3.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetIntegerv` | 1.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetInternalformati64v` | 4.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetInternalformativ` | 4.2 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetMultisamplefv` | 3.2 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedBufferParameteri64v` | 4.5 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedBufferParameteriv` | 4.5 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedBufferPointerv` | 4.5 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedBufferSubData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedFramebufferAttachmentParameteriv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedFramebufferParameteriv` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetNamedRenderbufferParameteriv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetObjectLabel` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetObjectPtrLabel` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetPointerv` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramBinary` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramInfoLog` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramInterfaceiv` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramPipelineInfoLog` | 4.1 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramPipelineiv` | 4.1 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramResourceIndex` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramResourceLocation` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramResourceLocationIndex` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramResourceName` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramResourceiv` | 4.3 | ✓ | real | `src/gl/gl_program_interface_p10.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramStageiv` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetProgramiv` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryBufferObjecti64v` | 4.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryBufferObjectiv` | 4.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryBufferObjectui64v` | 4.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryBufferObjectuiv` | 4.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryIndexediv` | 4.0 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryObjecti64v` | 3.3 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryObjectiv` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryObjectui64v` | 3.3 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryObjectuiv` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetQueryiv` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetRenderbufferParameteriv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSamplerParameterIiv` | 3.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSamplerParameterIuiv` | 3.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSamplerParameterfv` | 3.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSamplerParameteriv` | 3.3 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetShaderInfoLog` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetShaderPrecisionFormat` | 4.1 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetShaderSource` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetShaderiv` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetString` | 1.0 | ✓ | real | `src/gl/gl_error_string_debug.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetStringi` | 3.0 | ✓ | real | `src/gl/gl_error_string_debug.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSubroutineIndex` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSubroutineUniformLocation` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetSynciv` | 3.2 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexImage` | 1.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexLevelParameterfv` | 1.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexLevelParameteriv` | 1.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexParameterIiv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexParameterIuiv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexParameterfv` | 1.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTexParameteriv` | 1.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureLevelParameterfv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureLevelParameteriv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureParameterIiv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureParameterIuiv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureParameterfv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureParameteriv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTextureSubImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTransformFeedbackVarying` | 3.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTransformFeedbacki64_v` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTransformFeedbacki_v` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetTransformFeedbackiv` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformBlockIndex` | 3.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformIndices` | 3.1 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformLocation` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformSubroutineuiv` | 4.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformdv` | 4.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformfv` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformiv` | 2.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetUniformuiv` | 3.0 | ✓ | real | `src/gl/gl_get.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexArrayIndexed64iv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexArrayIndexediv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexArrayiv` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribIiv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribIuiv` | 3.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribLdv` | 4.1 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribPointerv` | 2.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribdv` | 2.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribfv` | 2.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetVertexAttribiv` | 2.0 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnCompressedTexImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnTexImage` | 4.5 | ✓ | real | `src/gl/gl_get_extra.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnUniformdv` | 4.5 | ✓ | real | `src/gl/gl_packed_robust.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnUniformfv` | 4.5 | ✓ | real | `src/gl/gl_packed_robust.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnUniformiv` | 4.5 | ✓ | real | `src/gl/gl_packed_robust.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glGetnUniformuiv` | 4.5 | ✓ | real | `src/gl/gl_packed_robust.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glHint` | 1.0 | ✓ | noop | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | spec cho phép bỏ qua (hint) |
| `glInvalidateBufferData` | 4.3 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateBufferSubData` | 4.3 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateFramebuffer` | 4.3 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateNamedFramebufferData` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateNamedFramebufferSubData` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateSubFramebuffer` | 4.3 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateTexImage` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glInvalidateTexSubImage` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsBuffer` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsEnabled` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsEnabledi` | 3.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsFramebuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsProgramPipeline` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsQuery` | 1.5 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsRenderbuffer` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsSampler` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsShader` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsSync` | 3.2 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsTexture` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsTransformFeedback` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glIsVertexArray` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glLineWidth` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glLinkProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glLogicOp` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMapBuffer` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMapBufferRange` | 3.0 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMapNamedBuffer` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMapNamedBufferRange` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMemoryBarrier` | 4.2 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMemoryBarrierByRegion` | 4.5 | ✓ | real | `src/gl/gl_compute.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMinSampleShading` | 4.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawArrays` | 1.4 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawArraysIndirect` | 4.3 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawArraysIndirectCount` | 4.6 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawElements` | 1.4 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawElementsBaseVertex` | 3.2 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawElementsIndirect` | 4.3 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glMultiDrawElementsIndirectCount` | 4.6 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedBufferData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedBufferStorage` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedBufferSubData` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferDrawBuffer` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferDrawBuffers` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferParameteri` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferReadBuffer` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferRenderbuffer` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferTexture` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedFramebufferTextureLayer` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedRenderbufferStorage` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glNamedRenderbufferStorageMultisample` | 4.5 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glObjectLabel` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glObjectPtrLabel` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPatchParameterfv` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPatchParameteri` | 4.0 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPauseTransformFeedback` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPixelStoref` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPixelStorei` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPointParameterf` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPointParameterfv` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPointParameteri` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPointParameteriv` | 1.4 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPointSize` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPolygonMode` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPolygonOffset` | 1.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPolygonOffsetClamp` | 4.6 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPopDebugGroup` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPrimitiveRestartIndex` | 3.1 | ✓ | real | `src/gl/gl_draw.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramBinary` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramParameteri` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1d` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1f` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1i` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1iv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1ui` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform1uiv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2d` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2f` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2i` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2iv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2ui` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform2uiv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3d` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3f` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3i` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3iv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3ui` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform3uiv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4d` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4f` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4i` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4iv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4ui` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniform4uiv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2x3dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2x3fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2x4dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix2x4fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3x2dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3x2fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3x4dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix3x4fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4x2dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4x2fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4x3dv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProgramUniformMatrix4x3fv` | 4.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glProvokingVertex` | 3.2 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glPushDebugGroup` | 4.3 | ✓ | real | `src/gl/gl_debug_p4.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glQueryCounter` | 3.3 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glReadBuffer` | 1.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glReadPixels` | 1.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glReadnPixels` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glReleaseShaderCompiler` | 4.1 | ✓ | noop | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | spec cho phép bỏ qua (hint) |
| `glRenderbufferStorage` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glRenderbufferStorageMultisample` | 3.0 | ✓ | real | `src/gl/gl_framebuffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glResumeTransformFeedback` | 4.0 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSampleCoverage` | 1.3 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSampleMaski` | 3.2 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameterIiv` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameterIuiv` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameterf` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameterfv` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameteri` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSamplerParameteriv` | 3.3 | ✓ | real | `src/gl/gl_sampler.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glScissor` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glScissorArrayv` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glScissorIndexed` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glScissorIndexedv` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glShaderBinary` | 4.1 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glShaderSource` | 2.0 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glShaderStorageBlockBinding` | 4.3 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glSpecializeShader` | 4.6 | ✓ | real | `src/gl/gl_shader.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilFunc` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilFuncSeparate` | 2.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilMask` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilMaskSeparate` | 2.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilOp` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glStencilOpSeparate` | 2.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexBuffer` | 3.1 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexBufferRange` | 4.3 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexImage1D` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexImage2D` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexImage2DMultisample` | 3.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexImage3D` | 1.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexImage3DMultisample` | 3.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameterIiv` | 3.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameterIuiv` | 3.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameterf` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameterfv` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameteri` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexParameteriv` | 1.0 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexStorage1D` | 4.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexStorage2D` | 4.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexStorage2DMultisample` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexStorage3D` | 4.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexStorage3DMultisample` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexSubImage1D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexSubImage2D` | 1.1 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTexSubImage3D` | 1.2 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureBarrier` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureBuffer` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureBufferRange` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameterIiv` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameterIuiv` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameterf` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameterfv` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameteri` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureParameteriv` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureStorage1D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureStorage2D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureStorage2DMultisample` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureStorage3D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureStorage3DMultisample` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureSubImage1D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureSubImage2D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureSubImage3D` | 4.5 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTextureView` | 4.3 | ✓ | real | `src/gl/gl_texture.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTransformFeedbackBufferBase` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTransformFeedbackBufferRange` | 4.5 | ✓ | real | `src/gl/gl_tess_xfb.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glTransformFeedbackVaryings` | 3.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1d` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1f` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1i` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1iv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1ui` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform1uiv` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2d` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2f` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2i` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2iv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2ui` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform2uiv` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3d` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3f` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3i` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3iv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3ui` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform3uiv` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4d` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4f` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4i` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4iv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4ui` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniform4uiv` | 3.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformBlockBinding` | 3.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2x3dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2x3fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2x4dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix2x4fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3x2dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3x2fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3x4dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix3x4fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4fv` | 2.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4x2dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4x2fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4x3dv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformMatrix4x3fv` | 2.1 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUniformSubroutinesuiv` | 4.0 | ✓ | real | `src/gl/gl_uniform.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUnmapBuffer` | 1.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUnmapNamedBuffer` | 4.5 | ✓ | real | `src/gl/gl_buffer.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUseProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glUseProgramStages` | 4.1 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glValidateProgram` | 2.0 | ✓ | real | `src/gl/gl_program.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glValidateProgramPipeline` | 4.1 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayAttribBinding` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayAttribFormat` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayAttribIFormat` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayAttribLFormat` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayBindingDivisor` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayElementBuffer` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayVertexBuffer` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexArrayVertexBuffers` | 4.5 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1d` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1dv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1f` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1fv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1s` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib1sv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2d` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2dv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2f` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2fv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2s` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib2sv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3d` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3dv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3f` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3fv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3s` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib3sv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nbv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Niv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nsv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nub` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nubv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nuiv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4Nusv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4bv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4d` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4dv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4f` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4fv` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4iv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4s` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4sv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4ubv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4uiv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttrib4usv` | 2.0 | ✓ | real | `src/gl/gl_readpixels_dsa.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribBinding` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribDivisor` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribFormat` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI1i` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI1iv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI1ui` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI1uiv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI2i` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI2iv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI2ui` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI2uiv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI3i` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI3iv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI3ui` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI3uiv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4bv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4i` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4iv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4sv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4ubv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4ui` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4uiv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribI4usv` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribIFormat` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribIPointer` | 3.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL1d` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL1dv` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL2d` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL2dv` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL3d` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL3dv` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL4d` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribL4dv` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribLFormat` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribLPointer` | 4.1 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP1ui` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP1uiv` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP2ui` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP2uiv` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP3ui` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP3uiv` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP4ui` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribP4uiv` | 3.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexAttribPointer` | 2.0 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glVertexBindingDivisor` | 4.3 | ✓ | real | `src/gl/gl_vertex.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glViewport` | 1.0 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glViewportArrayv` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glViewportIndexedf` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glViewportIndexedfv` | 4.1 | ✓ | real | `src/gl/gl_raster.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |
| `glWaitSync` | 3.2 | ✓ | real | `src/gl/gl_sync_query.cpp` | FULLY IMPLEMENTED | có thân thật + test tham chiếu |

## 4. 42 hàm compatibility-profile-only (ngoài phạm vi Core)
| API | GL version | Symbol | File |
|---|---|---|---|
| `glColorP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glColorP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glColorP4ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glColorP4uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnColorTable` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnConvolutionFilter` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnHistogram` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnMapdv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnMapfv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnMapiv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnMinmax` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnPixelMapfv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnPixelMapuiv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnPixelMapusv` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnPolygonStipple` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glGetnSeparableFilter` | 4.5 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP1ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP1uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP2ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP2uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP4ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glMultiTexCoordP4uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glNormalP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glNormalP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glSecondaryColorP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glSecondaryColorP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP1ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP1uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP2ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP2uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP4ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glTexCoordP4uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP2ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP2uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP3ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP3uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP4ui` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |
| `glVertexP4uiv` | 3.3 | ✓ | `src/gl/gl_packed_robust.cpp` |

Các hàm này được implement và export vì `GL_ARB_vertex_type_2_10_10_10_rev` và `GL_ARB_robustness` cần tới, nhưng **không được tính vào OpenGL 4.6 Core**.
