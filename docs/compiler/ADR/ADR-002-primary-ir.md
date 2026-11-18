# ADR-002 — Primary IR: typed IR nội bộ (SPIR-V chỉ làm cross-check/tùy chọn)

## Trạng thái
Chốt.

## Context
Cần một IR lưu type, storage class, resource binding, location, interpolation, memory qualifier, precision/semantics, control flow và source mapping. Không lấy type bằng đoán từ tên/chuỗi (ISS-010). Hai ứng cử viên:
- **A — SPIR-V làm primary IR** (glslang sinh → SPIRV-Tools validate/opt → SPIRV-Cross lower → MSL).
- **B — Typed IR nội bộ** (cấu trúc do TGLMT sở hữu), SPIR-V chỉ làm cross-check reference hoặc ingestion tùy chọn.

## Ràng buộc
- A1: không dùng GLSL→NIR của bên thứ ba trong đường chạy chính.
- A2: IR nội bộ được phép.
- Cần verifier sau mỗi pass, unit test, semantic test, diagnostic nếu không legalize được.
- Cần lower UBO/SSBO std140/std430, sampler/texture separation, image/atomic, built-in, tess/GS emulation, Metal feature legalization.

## Đánh giá
SPIR-V là typed, structured, có SPIRV-Tools validator/optimizer. Nhưng:
- SPIR-V mang Vulkan convention (binding set, storage image, clip-space, depth range, matrix-major) ⇒ cần lower về GL⇒Metal.
- SPIRV-Cross MSL backend Vulkan-oriented ⇒ mapping GL built-in/layout/binding phức tạp.
- Vendor 3 dự án C++ lớn tăng build/iOS complexity.
- SPIR-V ingestion (`GL_ARB_gl_spirv`) được phép làm tùy chọn, không phải primary.

Typed IR nội bộ cho phép thiết kế trực tiếp theo Metal ABI (buffer slot, attribute location, varying, interpolation, sample mask), ít lớp trung gian, diagnostic source location chính xác, build nhẹ.

## Quyết định
**Chọn Ứng cử viên B** — Typed IR nội bộ làm primary IR. SPIR-V chỉ dùng làm **cross-check reference** (differential test) và **tùy chọn** cho `GL_ARB_gl_spirv` ingestion (qua IR nội bộ, không qua glslang trong đường chính).

## Cấu trúc IR mục tiêu (tiêu chuẩn, có thể đổi tên sau design review)
```cpp
struct ShaderCompileRequest {
    ShaderStage stage;
    std::string source;
    GlslVersion version;
    GlslProfile profile;
    ExtensionSet extensions;
    CompileOptions options;
    SourceOrigin origin;
};
struct ShaderCompileResult {
    bool success;
    IntermediateModule module;
    std::vector<Diagnostic> diagnostics;
    ShaderReflection reflection;
};
struct ProgramLinkRequest { ... };
struct ProgramLinkResult { ... };
```

## Compiler passes (mỗi pass có pre/post-condition, verifier, unit test)
- canonicalization/normalization
- dead code elimination
- constant folding
- specialization/constant propagation
- interface flattening (Metal yêu cầu)
- resource binding remapping
- UBO/SSBO layout lowering (std140/std430)
- sampler/texture separation + combined sampler mapping
- image/atomic lowering
- stage interface linking
- built-in variable lowering
- texture operation lowering
- derivatives + implicit LOD handling
- precision/double strategy
- barrier/memory model lowering
- geometry/tessellation emulation (khi khả thi)
- target feature legalization
- MSL-specific intrinsic lowering

## Trade-offs
| Tiêu chí | SPIR-V primary | Typed IR nội bộ (chọn) |
|----------|----------------|------------------------|
| Typed/structured | Có | Có |
| Validator/optimizer | SPIRV-Tools | Viết verifier/pass riêng |
| GLSL→GL semantic | Cần lower Vulkan→GL | Thiết kế trực tiếp GL |
| Metal backend | SPIRV-Cross (Vulkan) | Viết mới (Metal) |
| Build/iOS dependency | Nặng | Nhẹ |
| Diagnostic/source location | Cần source-map | Chính xác ngay |

## Rủi ro và giảm thiểu
- Rủi ro: viết pass/verifier có lỗi ⇒ IR invalid sinh MSL sai. Giảm: verifier sau mỗi pass, unit test, SPIR-V cross-check cho các case quan trọng.
- Rủi ro: thiếu tối ưu hóa so với SPIRV-Tools. Giảm: thêm pass慢慢 (constant folding, DCE, CSE) sau khi correctness ổn định; đo benchmark trước khi tối ưu.

## Test plan
- IR verifier chạy giữa các transformation; không tạo typed IR invalid.
- Golden SPIR-V disassembly cho các feature quan trọng (dùng làm cross-check).
- Pass unit test (DCE, constant folding, layout lowering, binding remapping).
- Regression: pow2 type resolution, iris_Entity input activity.

## Migration / rollback
- Giao diện frontend↔IR↔backend tách biệt ⇒ có thể đổi IR sau khi ổn định.
- Giữ converter cũ làm fallback trong migration (D-011).

## Owner
Agent 2 (IR/passes), Review Lead.
