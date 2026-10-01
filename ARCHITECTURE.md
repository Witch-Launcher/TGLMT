# TGLMT Architecture — C++ lõi + metal-cpp + Objective-C++ cầu

```
┌────────────────────────────────────────────────────────────┐
│App / Test (C++ gọi gl* như OpenGL 4.6 core)                │
│include/tglmt/gl46.h (sinh từ gl.xml — 698 decl, không đoán)│
└──────────────────────┬─────────────────────────────────────┘
                       ▼
┌────────────────────────────────────────────────────────────┐
│ TGLMT Core (C++17, platform-agnostic)  src/core/           │
│  Context: current state per-thread (như GL context)        │
│  ObjectRegistry<T>: Gen/Create/Delete/Is cho mọi loại      │
│  ErrorTracker: glGetError queue + KHR_no_error bypass      │
│  StateTracker: cap/blend/depth/stencil/raster/viewport/... │
│   shadow copies cho mọi glGet* (Metal không cho query GPU) │
└──────────────────────┬─────────────────────────────────────┘
                       ▼
┌────────────────────────────────────────────────────────────┐
│ TGLMT GL dispatch  src/gl/  (mỗi file 1 nhóm spec)         │
│  gl_buffer.cpp  gl_vertex.cpp  gl_draw.cpp  gl_texture.cpp │
│  gl_sampler.cpp gl_shader.cpp gl_program.cpp gl_uniform.cpp│
│  gl_framebuffer.cpp gl_raster.cpp gl_compute.cpp           │
│  gl_sync_query.cpp gl_tess.cpp gl_xfb.cpp gl_dsa.cpp       │
│  gl_get.cpp gl_debug.cpp gl_ext.cpp gl_robust.cpp          │
│  Mỗi hàm: validate enum theo gl.xml → cập nhật shadow →    │
│   ghi IMetalEncoder / PipelineCache request                │
└──────────────────────┬─────────────────────────────────────┘
                       ▼
┌────────────────────────────────────────────────────────────┐
│ IMetal abstraction (C++ thuần)  src/metal/MetalInterface.h │
│  IMetalDevice / IMetalBuffer / IMetalTexture /             │
│  IMetalLibrary / IMetalRenderPipeline / IMetalComputePipe/ │
│  IMetalCommandQueue,Buffer,Encoder(Render/Blit/Compute)    │
│  Hai backend:                                              │
│   (a) AppleMetalBridge.mm — ObjC++ gọi <Metal/Metal.h>     │
│       (tương đương 1-1 metal-cpp MTL::Device::*).          │
│   (b) NullMetalBackend.cpp — CPU shadow, chạy trên Linux/CI│
│       không GPU, vẫn pass logic/state tests.               │
│  third_party/metal-cpp/ — header gốc Apple (fetch script), │
│   khi có sẽ typedef MTL::Device* sang bridge (không fork). │
└────────────────────────────────────────────────────────────┘
```

## Ánh xạ tọa độ /isz (đọc từ spec, không đoán)

- GL NDC z ∈ [-1,1], Metal NDC z ∈ [0,1] → vertex shader TGLMT chèn
  `out.pos.z = (in.pos.z * 0.5 + 0.5)` trừ khi `glClipControl(GL_ZERO_TO_ONE)`.
  Tài liệu: `glspec46.core.pdf` §13 (Viewport) + MSL spec §2 (coordinates).
- GL origin bottom-left, Metal top-left → `glViewport` được flip-y trong
  `MTLViewport` + `MTLRenderPassDescriptor` load/store; `glClipControl`
  `GL_CLIP_ORIGIN` được emulate (ghi rõ trong log nếu app đổi).
- `glPolygonOffsetClamp` (GL 4.6) → `setDepthBias:slopeScale:clamp:` (Metal 3+ `depthBiasClamp`).

## Không có tương đương 1-1 (ghi rõ giới hạn)

| GL | Metal thực tế | TGLMT làm gì |
|---|---|---|
| Geometry shader | Không có; spirv-cross cho ra MSL **không biên dịch được** (`unknown main0`, `EmitVertex` không tồn tại — đã chứng thực) | Đường mesh shader Metal 3+ (`gs_mesh.metal`, `makeMeshPipeline`, `drawMesh`): compile-verified, gate thực thi Apple7+ (`supportsMesh()`; Intel tạo pipeline OK nhưng vẽ đen — đã quan sát; Metal3 family KHÔNG đủ). GS tổng quát (multi-stream/layer) cần M5b dịch GLSL→mesh |
| glLogicOp (16 ROPs) | Không có ROP phần cứng | Composite compute-ROP (`rop.metal` + compute API): đúng Table 17.3 bitwise từng component cho target integer (spec: no effect trên float/sRGB). Test 16/16 ops khớp CPU |
| glPolygonMode LINE | Có (`setTriangleFillMode:`) | `setTriangleFillModeLines` — test wireframe: giữa đen, biên đỏ |
| glScissor | Có (`setScissorRect:`) | `setScissorRect` + `GLScissorToMetal` flip-y — test loại center |
| Depth test | Có (`MTLDepthStencilState` + depth attachment) | `makeDepthStencilState` (map đủ 8 `MTLCompareFunction`) + `makeRenderTargetWithDepth` + `makeDepthPipeline` (depth format khớp pass) — test near thắng far |
| Blending | Có (per-attachment factors/ops) | `makeBlendPipeline` (map đủ 15 factors + 5 ops) — test công thức SRC_ALPHA ra (127,0,127) |
| Sampler/aniso | Có (`MTLSamplerDescriptor.maxAnisotropy`) | `makeSampler` + bind texture/sampler — test textured-quad 4 góc đúng texel |
| Texture format | Có (sRGB/depth pixel formats; lưu ý `Depth24Unorm_Stencil8` cấm trên iOS → dùng `Depth32Float_Stencil8`) | `ToMetalFormat` theo internalFormat GL — test mapping trên mọi backend |
| MSAA resolve | Có (`storeActionMultisampleResolve`; pipeline `rasterSampleCount` PHẢI khớp — sai cho pixel tối, đã quan sát) | `makeMSAATarget` + `makeMSAAPipeline` — test resolve center đỏ |
| Viewport/scissor coords | Khác hệ (GL bottom-left/z -1..1 vs Metal top-left/z 0..1) | `GLViewportToMetal`/`GLScissorToMetal` (+`glClipControl` variants) — unit test CPU |
| Transform feedback | Không có | Emulate đầy đủ plumbing: varyings (`glTransformFeedbackVaryings`→`glGetTransformFeedbackVarying`), bind points, state machine Begin/Pause/Resume/End đúng spec, `capturedCount` từ draw trong phiên active, `glDrawTransformFeedback*` vẽ đúng count. Tính varying thật cần M5b thực thi program |
| Primitive restart | Không có | Emulate CPU: tách strip/fan tại restart index (cả `PRIMITIVE_RESTART` custom index và `FIXED_INDEX`) thành nhiều `drawIndexed` — raster hệt nhau |
| Tessellation | Có (`drawPatches`, factor buffer, post-tess vertex fn) | Đường Metal thật: pipeline Integer/Half/PerPatch + `[[patch(triangle)]]` fn + factors 1.0 (= passthrough đúng tess level 1); TCS tính factors động cần M5b |
| TRIANGLE_FAN | Không có | Trace giữ đúng `Fan`; đường GPU expand ở M5b |
| ELEMENT_ARRAY_BUFFER binding | N/A | Đúng spec: là state của VAO, không phải global |
| Texture target cmds | N/A | Resolve qua active unit + kiểm tra target khớp (sửa lỗi first-match cũ) |
| `glGetError` | Không có | Hàng đợi lỗi CPU + validation layer; `KHR_no_error` → bỏ queue |
| SPIR-V (`glSpecializeShader`, `ARB_gl_spirv`) | Metal chỉ nhận MSL/DXBC | Bắt buộc `spirv-cross`/`spirv-val` ngoài, TGLMT không tự biên dịch SPIR-V |
| `glLogicOp`, `glPolygonMode(FILL/LINE/POINT)` đầy đủ | Hạn chế | Shadow + bake được phần nào, còn lại báo `UNSUPPORTED` trung thực |

## Luồng lệnh (ví dụ `glDrawElements` — deferred full)

```
glDrawElements(mode,count,type,indices)
 → validate (ErrorTracker, enum từ gl.xml)
 → EmitDraw: Apple path BỎ trace-encoder (Null giữ trace cho CI),
    UBYTE expand CPU, restart-split, Multidraw đếm batched
 → AppleDrawGL:
    VAO→descriptor → target resolve → hazard pre-scan (feedback? split pass)
    → FlushAll staging (memcpy Shared, rẻ) → PipelineKey cache
      (trùng? reuse cachedPipe, BỎ bridge lookup)
    → pendingEncoder reuse (cùng target/FBO/depth, không clear)
      + dirty-check viewport/cull/fill/blend/depth (đổi mới encode)
    → uniforms/UBO/index qua ring 3×4MB triple-buffer (steady-state 0 alloc)
    → drawIndexed/drawPrimitives vào encoder MỞ, KHÔNG commit
 → Flush (commitNoWait, 1 commit cho N draws): target đổi / hazard /
    stage-used / ReadPixels / Blit / present / EndFrame
```

Counters chứng minh ở `Context::AppleStats`: `encodersCreated<<drawsEncoded`,
`traceSkipped`, `ringAllocs`/`tempAllocs==0`, `flushAvoided`, `hazardSplits`,
`multidrawBatched`, `diagSkipped`, `psoPrewarmed`. Xem `docs/en/perf.md`.
