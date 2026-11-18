# ADR-003 — MSL backend: viết mới, Metal-oriented (không dùng SPIRV-Cross làm backend chính)

## Trạng thái
Chốt.

## Context
Backend cần đảm bảo: MSL syntax/type đúng, stage I/O struct đúng ABI, attribute index/varying location/fragment color attachment, interpolation, uniform/resource slot không xung đột, buffer alignment/stride/packing, texture/sampler binding, address spaces (constant/device/thread/threadgroup), atomics/barriers memory order/scope, entry point/pipeline stage, Metal feature set + language version hợp lệ trên thiết bị mục tiêu, source line mapping.

Ứng cử viên:
- **A — SPIRV-Cross MSL backend** (được dùng chung với SPIR-V).
- **B — MSL backend riêng từ typed IR nội bộ**.

## Đánh giá
SPIRV-Cross là Vulkan-oriented: resource binding (set/binding), clip-space Y, depth range, matrix-major, built-in (gl_InstanceID, gl_DrawID, derivative, texture*, image*, atomic) khác GL. Mapping GL→MSL qua SPIR-V thêm lớp trung gian và dễ lỗi ngữ nghĩa. Backend riêng từ IR nội bộ (thiết kế theo Metal ABI) kiểm soát trực tiếp attribute location, varying, interpolation, UBO std140/std430, sampler/texture, tess/GS emulation, double strategy.

Rủi ro backend riêng: viết MSL emitter từ zero, phải compile bằng Metal thật (`xcrun metal -c`) và test pixel để chứng minh ngữ nghĩa.

## Quyết định
**Chọn Ứng cử viên B** — MSL backend riêng từ typed IR nội bộ. SPIRV-Cross chỉ dùng làm **cross-check reference** (differential), không backend chính.

## Backend yêu cầu (checklist)
- MSL syntax/type đúng; compile bằng `xcrun metal -c` trên macOS Metal.
- Stage input/output struct: `[[attribute(N)]]`, `[[user(locnN)]]`, `[[color(N)]]`, `[[sample_id]]`, `[[position]]`, `[[point_coord]]`, `[[front_facing]]`, `[[depth(any/greater)]]`, `[[sample]]`, `[[patch]]`, `[[control_point]]`.
- Interpolation qualifier: `[[center_perspective]]` (default), `[[flat]]`, `[[center_no_perspective]]`, `[[center]]`.
- Uniform buffer `constant TGLMTUniforms& [[buffer(16)]]`; UBO/SSBO std140/std430 stride/packing; block slot bảng `[[buffer(17+k)]]`.
- Texture `texture2d<float, access::sample>` + sampler `sampler`; combined sampler mapping; array/cube/shadow/3D/2DArray/MS.
- Image: `texture2d<T, access::read_write>` etc.; atomic counter: `device atomic_uint*`.
- Address space: `constant` (UBO), `device` (SSBO/image/atomic), `thread`, `threadgroup`.
- Atomics/barriers: memory order + scope phù hợp Metal.
- Entry point + pipeline stage (vertex/fragment/compute).
- Metal language version + feature set hợp lệ A11 (Apple7/A8).
- Source line mapping GLSL→MSL để log Metal quay lại GLSL.

## Không coi "Metal compile thành công" là bằng chứng ngữ nghĩa đúng
Phải có pixel/behavior differential test (differential rendering vs reference) cho mỗi feature.

## Trade-offs
| Tiêu chí | SPIRV-Cross | Backend riêng (chọn) |
|----------|-------------|----------------------|
| GLSL semantic | Cần map Vulkan→GL | Thiết kế trực tiếp GL |
| Attribute/varying location | Cần remapping | Chính xác ngay |
| UBO std140/std430 | Cần lower | Chính xác ngay |
| Build dependency | Nặng (vendor SPIRV-Cross) | Nhẹ |
| Diagnostic | SPIR-V diagnostic | GLSL source location |

## Rủi ro và giảm thiểu
- Rủi ro: MSL emitter sinh code lỗi cú pháp/type ⇒ Metal compile fail. Giảm: cổng `xcrun metal -c` cho mọi case corpus; unit test snapshot MSL chỉ làm debug, không thay semantic test.
- Rủi ro: MSL compile được nhưng ngữ nghĩa sai ⇒ pixel sai. Giảm: differential render test, pixel diff, per-channel tolerance.
- Rủi ro: regression so với converter cũ (byte-identical, D-009). Giảm: chạy song song trong migration, chỉ cắt mới khi byte-identical corpus 86 shader.

## Test plan
- Mỗi feature: compile MSL bằng `xcrun metal -c`; tạo pipeline (vertex descriptor/attachment format/sample count); render pixel diff vs reference.
- Resource binding: byte layout golden test + Metal buffer reflection.
- Regression: pow2, iris_Entity, dayBlend.

## Migration / rollback
- Giao diện IR→MSL tách ⇒ có thể đổi backend sau.
- Giữ converter cũ fallback trong migration.

## Owner
Agent 3 (backend), Review Lead.
