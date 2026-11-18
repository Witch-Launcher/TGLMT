# ADR-007 — Dùng glslang + SPIRV-Cross cho GLSL → MSL (supersedes ADR-001)

## Trạng thái
Chốt ngày 2026-11-18 theo user directive. Supersedes `ADR-001-glsl-frontend.md`.

## Context
ADR-001 chọn frontend tự viết (ứng viên B) vì A1/A2 cấm GLSL→NIR của bên thứ ba.
User yêu cầu đổi sang glslang + SPIRV-Cross và dùng git submodule để repo GitHub
kéo được dependency.

## Quyết định
- `third_party/glslang` + `third_party/SPIRV-Cross` là submodule shallow.
- Backend `src/compiler/backend/glslang/GlslangBackend.cpp` dùng
  glslang (OpenGL semantics, SPIR-V 1.0) → SPIRV-Cross MSL 2.0.
- Static archive Apple được merge dependency để `libtglmt.a` self-contained.
- Converter cũ vẫn là đường render đến khi có pixel/differential trên A11
  (D-011); shadow dùng glslang qua env `TGLMT_GLSLANG_SHADOW=1`.

## Hệ quả
- Xem `docs/DECISIONS.md` D-013 cho bằng chứng test và NOT_VERIFIED.
- ADR-001 không còn hiệu lực cho đường dịch GLSL; các lý do desktop-vs-Vulkan
  vẫn là rủi ro cần test (đặc biệt GLSL compatibility 1.x).
