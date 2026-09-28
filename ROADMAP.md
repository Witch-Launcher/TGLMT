# TGLMT — OpenGL 4.6 Core → Metal (iOS) : Roadmap thực thi

> Nguồn chân lý duy nhất: `docs/khronos/gl.xml`, `docs/khronos/glspec46.core.pdf`,
> `docs/khronos/GLSLangSpec.4.60.pdf`, `docs/khronos/extensions/ARB/*.txt`,
> `docs/khronos/extensions/KHR/KHR_no_error.txt`,
> `docs/apple/Metal-Shading-Language-Specification.pdf`,
> `docs/mapping/GL46_to_Metal.md`.
> Không suy đoán ngoài tài liệu. Mọi phát hiện lệch phải ghi nhận trung thực.

## 0. Phát hiện kiểm chứng (26/09/2026, đã chạy script trên `gl.xml`)

- `gl.xml` có **3301** `<command>` (mọi version + extension).
- Lọc `GL_VERSION_1_0..4_6` trừ 350 hàm `remove` (compat: `glBegin/glEnd`, `glAccum`, …)
  → **OpenGL 4.6 Core Profile = 698 hàm** (không phải 657).
- `GL_VERSION_4_6` mới đúng 4 hàm (khớp spec 31/07/2017):
  `glSpecializeShader`, `glMultiDrawArraysIndirectCount`,
  `glMultiDrawElementsIndirectCount`, `glPolygonOffsetClamp`.
- `docs/mapping/GL46_to_Metal.md` có **658 dòng dữ liệu** (tiêu đề ghi 657 — lệch +1),
  **thiếu 42 hàm core** so với `gl.xml`:
  - 30 hàm packed-attribute `GL_VERSION_3_3`: `glVertexP*/glTexCoordP*/glMultiTexCoordP*/glNormalP*/glColorP*/glSecondaryColorP*`
    → chiến lược TGLMT: unpack trên CPU (theo spec Table 10.x) rồi đưa vào `MTLBuffer`.
  - 12 hàm robustness `GL_VERSION_4_5` (`glGetn*`: `glGetnMapdv/fv/iv`, `glGetnPixelMap*`, `glGetnColorTable`, `glGetnConvolutionFilter`, `glGetnHistogram`, `glGetnMinmax`, `glGetnPolygonStipple`, `glGetnSeparableFilter`)
    → chiến lược: shadow-state CPU + đọc có kiểm tra `bufSize` (spec KHR_robustness).
- `ARB_gl_spirv` (ratified 22/07/2016, rev 46): yêu cầu GL ≥ 3.3, SPIR-V 1.00 → TGLMT dùng `spirv-cross` GLSL→MSL, không tự chế compiler.
- `KHR_no_error` (v6, 25/02/2015): **không có hàm mới**, bật là UB khi có lỗi → TGLMT map thành `TGLMT_NO_ERROR=1` bỏ `glGetError`/validation để tối ưu, mặc định tắt.

## 1. Ngôn ngữ & tầng (theo yêu cầu)

- **C++17 lõi**: mọi state, registry, dispatch GL. Không phụ thuộc UI.
- **metal-cpp + Objective-C++ cầu**: `src/metal/` gồm interface C++ thuần (`IMetal*`)
  + cài đặt `MetalBridge.mm` dùng `<Metal/Metal.h>` gốc Apple.
  `third_party/metal-cpp/` là thư mục chờ — script `scripts/fetch_metal_cpp.sh` tải header chính chủ Apple khi build iOS thật.
  Trên macOS dev hiện tại dùng trực tiếp `MTLDevice` qua ObjC++ (tương đương 1-1 với metal-cpp `MTL::Device`).
- **Python orchestrator**: `tools/orchestrator/pipeline.py` điều phối Translator→Tester→Reviewer→BugFixer, retry ≤ 3, log provenance JSONL.

## 2. Milestones (ánh xạ 11 ưu tiên trong đề bài)

| Giai đoạn | Tuần | Đầu ra kiểm chứng được |
|---|---|---|
| M0 Chuẩn bị | W1–W2 | Repo scaffold, `gen_gl_headers.py` sinh đủ 698 decl từ `gl.xml`, build xanh macOS + Null backend Linux |
| M1 Lõi state | W3–W4 | Context/Registry/Error/StateTracker + 139 `glGet*`/`glIs*` shadow pass unit test |
| M2 Draw cơ bản | W5–W8 | Buffer/VAO/VertexAttrib/draw (`drawPrimitives/drawIndexedPrimitives`), viewport/y-flip/depth 0..1, test tam giác đỏ |
| M3 Texture/Sampler | W9–W11 | `MTLTexture`+`replaceRegion`, `MTLSamplerState`, PixelStore alignment, blit/mipmap, test upload/readback |
| M4 Shader/Uniform | W12–W15 | GLSL→MSL (offline `metal -c` + runtime lib), Pipeline cache, UBO/SSBO/ProgramUniform DSA, atomic counter doc |
| M5 FBO/Raster | W16–W18 | RenderPassDescriptor MRT/depth-stencil/blend/clear/polygonOffset/clipControl emulate |
| M6 Compute/Sync/Query | W19–W21 | Compute encoder, `MTLSharedEvent` fence, timer/occlusion/statistics-query, barrier |
| M7 Tess/Geom/TF/Indirect | W22–W25 | Tessellation pipeline, geometry→mesh/tess fallback (ghi rõ giới hạn), TF emulate bằng buffer+compute, indirect/ICB/multidraw |
| M8 DSA/Debug/Ext | W26–W27 | Toàn bộ `glNamed*/glCreate*`, `KHR_no_error`, debug callback → `os_log`/validation, ARB_aniso/clamp/SPIR-V/statistics/overflow/draw_params/group_vote |
| M9 Harden/Test/Perf | W28–W30 | Stress, GPU counters, HUD, coverage ≥ 698/698 decl + ≥ 80% behavior pass, báo cáo dashboard |

## 3. Chính sách retry / CI-like

`Translator → Tester → Reviewer → BugFixer → Tester…`, tối đa **3 vòng**.
Fail sau 3 vòng → `status=needs_human`, giữ toàn bộ log + GPU capture.
Mọi bước ghi `out/provenance.jsonl` (`task_id, agent, model, input_hash, spec_ref, result, ts`).

## 4. Sandbox & bảo mật

- Build/test trong `sandbox/` (Docker hoặc `sandbox-exec`), không net, chỉ đọc `docs/`, chỉ ghi `out/`.
- Bật Metal validation layer ở Debug (`METAL_DEVICE_WRAPPER_TYPE=1`, `MTL_DEBUG_LAYER=1`).
- Quét mã sinh bởi AI trước khi biên dịch (deny `system/exec/socket`).

## 5. Độ chính xác & hiệu năng

- Chính: `tests_passed / 698`, bao phủ decl 100% (do sinh từ `gl.xml`).
- Phụ: FPS/GPU time qua `GPUStartTime/GPUEndTime` + `MTLSharedEvent`, token usage qua orchestrator stats.
