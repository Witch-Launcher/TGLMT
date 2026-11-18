# ADR-006 — Geometry/tessellation emulation (Metal không có GS stage)

## Trạng thái
Chốt (giữ D-008, bổ sung gap chưa hỗ trợ).

## Context
Metal không có geometry shader stage. A6 (AGENTS.md) bắt buộc: GS phải emulation, KHÔNG kết luận HARDWARE_LIMITED chỉ vì Metal không có GS stage. Tessellation (TCS/TES) Metal có post-tess vertex (`[[patch]]`, `[[control_point]]`, `MTLTriangleTessellationFactorsHalf`) nhưng TGLMT hiện chưa có đường tess đầy đủ.

## Quyết định hiện tại (đã implement)
- **GS**: dịch thành **MSL compute kernel** + draw buffer mở rộng (`docs/GEOMETRY_SHADER_EMULATION.md`).
  - `gl_in[]` ← capture buffer (đường tess capture).
  - `EmitVertex()` → ghi vertex hiện tại vào buffer mở rộng (bộ đếm atomic).
  - `EndPrimitive()` → ghi index + ranh giới primitive (có EndPrimitive ngầm cuối shader theo GLSL 3.20 §8.10).
  - Vertex shader passthrough đọc buffer mở rộng → FS.
- Link: GS compile được (syntax/format) lúc compile shader; lỗi phụ thuộc interface (VS output, uniform dùng chung) biết lúc link. Fail rõ tại link, KHÔNG link-OK-rồi-vẽ-câm.
- `gl_ClipDistance`: chỉ 1 clip_distance trên Metal (giới hạn phần cứng, documented).

## Chưa hỗ trợ (phải fail rõ tại link, không link-OK-vẽ-câm)
- `gl_Layer` / layered rendering.
- XFB + GS.
- `invocations > 1`.
- Input adjacency.
- `gl_ClipDistance` > 1.

## Tessellation (TCS/TES)
- Hiện hỗ trợ subset triangles/quads, TCS=compute kernel, TES=post-tess vertex, VS prepass capture (T4b).
- `isolines` / `point_mode` rejected.
- Chưa bind uniform/sampler đầy đủ trong tess path ⇒ fail rõ tại link.

## Test plan
- `test_geometry_emulation.cpp` (có ca pixel thật trên Null backend).
- Mỗi tính năng chưa hỗ trợ ⇒ link fail rõ + log.
- Pixel verify trên A11 (NOT_VERIFIED, chưa có thiết bị).

## Owner
Agent 5 (lifecycle/link) + Agent 3 (backend tess pipeline). Review Lead.
