# ADR-001 — GLSL frontend: custom AST/preprocessor (không dùng glslang làm frontend chính)

## Trạng thái
SUPERSEDED ngày 2026-11-18 bởi `ADR-007-glslang-spirv-cross.md` (user directive).
Nội dung dưới đây giữ làm lịch sử quyết định.

## Context
TGLMT hiện dùng `src/gl/glsl_to_msl.cpp` (~13k LOC), một **token/string rewriter** không có AST, type system, overload resolution, hay dataflow analysis (docs/GLSL_460_COMPILER.md §1). Nguyên nhân gốc ISS-001/ISS-002/ISS-003/ISS-004. Cần thay bằng frontend có cấu trúc để hỗ trợ GLSL 1.10→4.60 đúng version/profile/extension.

Hai ứng cử viên:
- **A — glslang (Khronos) + SPIR-V**: parser/semantic/type-check đã trưởng thành, typed IR (SPIR-V), có SPIRV-Tools validator/optimizer và SPIRV-Cross MSL backend.
- **B — GLSL AST frontend + typed IR nội bộ + MSL backend riêng**: viết lexer/preprocessor/parser/type-checker tự TGLMT, IR riêng, lower sang MSL.

## Ràng buộc từ AGENTS.md / DECISIONS.md
- A1: dịch thẳng OpenGL→Metal trong một tầng, KHÔNG dùng Mesa/Zink/MoltenVK/ANGLE/state tracker / GLSL→NIR của bên thứ ba.
- A2: IR/ABI nội bộ được phép; không chèn OpenGL-implementation khác vào đường chạy.
- A5: chỉ OpenGL Desktop (không phải ESSL). GLSL desktop semantics khác Vulkan/SPIR-V (default uniform binding, layout qualifiers, built-in biến, clip-space, depth range, matrix-major, derivatives, primitive ordering).
- C++17. Public API C ABI, không exception/RTTI/STL qua ranh giới thư viện.
- Không tự viết compiler "hoàn toàn từ zero" nếu frontend hiện có thể đáp ứng — nhưng tránh phụ thuộc sâu vào internal headers/API không ổn định.

## Đánh giá ứng cử viên A (glslang + SPIR-V + SPIRV-Cross)

Ưu điểm (giả thuyết, chưa đo):
- Parser/type-check/validation GLSL đã thành熟, xử lý version/profile/extension, built-in, overload.
- Typed IR (SPIR-V) + SPIRV-Tools validation/optimization/disassembly.
- SPIRV-Cross có backend MSL và reflection/resource remapping.

Rủi ro / chi phí (cần kiểm chứng trước khi nhận):
1. **GL desktop vs Vulkan semantics**: glslang sinh SPIR-V theo Vulkan convention (binding set, storage buffer, clip-space Y-flat, depth 0..1, matrix row-major, no default uniform block binding). TGLMT phải lower về GL convention (default UBO binding 0, std140, clip-space Y-flip, depth -1..1, column-major). Việc "unsugar" SPIR-V về GL thêm pass phức tạp và dễ lỗi ngữ nghĩa.
2. **Built-in biến & hàm**: gl_InstanceID, gl_VertexID, gl_DrawID, gl_BaseVertex, gl_FragCoord, gl_PointCoord, derivative, texture*, image*, atomic — mapping SPIR-V built-in → MSL/GL khác nhau, phải kiểm tra từng case.
3. **Specialization constants / entry point**: ARB_gl_spirv + glSpecializeShader phải map đúng.
4. **Tessellation/geometry**: SPIR-V tessellation model khác GL; Geometry shader emulation (A6 bắt buộc) phải build từ SPIR-V tess/geometry — phức tạp.
5. **Build complexity**: glslang + SPIRV-Tools + SPIRV-Cross là 3 dự án lớn C++, cần vendor hoặc package manager, tăng thời gian build và kích thước dependency. Vendor trên iOS (arm64, iOS 16) cần xác nhận toolchain.
6. **License**: glslang/SPIRV-Cross Apache 2.0 — có thể dùng, nhưng cần ghi rõ.
7. **Source location / diagnostic**: glslang diagnostic có source location, nhưng mapping ngược qua SPIR-V → MSL → GLSL source cho debug shaderpack cần source-map riêng.

## Đánh giá ứng cử viên B (custom AST frontend + typed IR + MSL backend)

Ưu điểm:
- Toàn quyền kiểm soát GLSL desktop semantics (version/profile/extension/built-in/layout/precision), không cần "unsugar" Vulkan convention.
- IR nội bộ thiết kế theo Metal ABI (buffer slot, attribute location, varying, interpolation, sample mask) — lower trực tiếp, ít lớp trung gian.
- Build không nặng (chỉ C++17 STL, không vendor C++ lớn).
- Diagnostic source location chính xác (file/line/column) do frontend tự sở hữu AST.
- Dễ thêm pass chuyênMetal (UBO std140 lowering, sampler/texture separation, tess/GS emulation, double strategy).

Rủi ro:
- Viết parser/type-checker/overload resolver từ zero là công sức lớn; cần test đặc biệt để không gây regression so với converter cũ (byte-identical MSL cho corpus 86 shader đã verify).
- Phải tự build SPIR-V-like validation (hoặc dùng SPIRV-Tools chỉ cho IR nội bộ nếu quyết định hỗ trợ SPIR-V ingestion).

## Quyết định
**Chọn Ứng cử viên B** — custom GLSL AST frontend + typed IR nội bộ + MSL backend riêng.

Lý do:
- Tuân thủ A1/A2 (không chèn GLSL→NIR của bên thứ ba vào đường chạy GLSL; IR nội bộ được phép).
- GLSL desktop semantics khác Vulkan/SPIR-V đủ lớn để "unsugar" SPIR-V là điểm yếu, không phải điểm mạnh.
- Build/iOS toolchain đơn giản hơn (không vendor glslang/SPIRV-Tools/SPIRV-Cross).
- Có kiểm soát đầy đủ diagnostic + source mapping, cần cho shaderpack debugging.

## Điều kiện dùng glslang/SPIR-V sau này (tùy chọn, không phải đường chính)
- Chỉ dùng làm **cross-check / reference** trong differential test (generate SPIR-V từ reference glslang, so với IR TGLMT), không dùng trong đường compile production.
- SPIR-V ingestion (`GL_ARB_gl_spirv`) vẫn được cho phép làm **tùy chọn** (A1 không cấm SPIR-V làm input định dạng, chỉ cấm dùng implementation OpenGL của bên thứ ba); nếu làm thì qua IR nội bộ, không qua glslang trong đường main.

## Trade-offs
| Tiêu chí | A (glslang+SPIR-V+Cross) | B (custom AST+IR+MSL) |
|----------|--------------------------|-----------------------|
| GLSL semantic độ chính xác (desktop) | Cần lower Vulkan→GL (rủi ro) | Thiết kế trực tiếp GL (ưu điểm) |
| Build/iOS dependency | Nặng (3 dự án C++ lớn) | Nhẹ (C++17 STL) |
| Diagnostic/source location | Cần source-map qua SPIR-V | Chính xác ngay tại frontend |
| Overload/type-check | Đã có | Viết mới (công sức lớn, cần test) |
| MSL backend | SPIRV-Cross (Vulkan-oriented) | Viết mới (Metal-oriented) |
| Tess/GS emulation | Phức tạp từ SPIR-V | Dễ hơn từ IR nội bộ |
| Regression so với converter cũ | Không tự động | Cần cổng byte-identical (D-009) |

## Rủi ro và cách giảm thiểu
- Rủi ro: viết type-checker/overload resolver có lỗi ⇒ regression shader. Giảm: cổng `test_glsl_coverage` (convert + `xcrun metal -c`) + regression test pow2/iris_Entity/dayBlend + differential corpus Minecraft.
- Rủi ro: MSL backend mới sinh code khác converter cũ ⇒ Byte-identical gate (D-009) sẽ fail. Giảm: chạy converter cũ song song trong giai đoạn migration, so sánh output; chỉ cắt mới khi byte-identical trên corpus 86 shader.
- Rủi ro: thiếu built-in GL. Giảm: giữ kiến trúc "built-in pass tự đăng ký" (D-011) làm compatibility adapter trong migration.

## Test plan
- Lexer: token, numeric literal, suffix, comment, source location.
- Preprocessor: `#define`/object/function macro, expansion, token pasting/stringification, `#if/#elif` (toán tử quan hệ + arithmetic), `defined`, nested, `#version`, `#extension`, `#line`, include/import.
- Parser/AST: TU, declaration, struct, interface block, layout qualifier, function overload, parameter qualifiers, expression, statement, array, constructor, swizzle, control flow.
- Symbol table: global/local, shadowing, overload set, built-in.
- Type system: scalar/vector/matrix/sampler/texture/image/atomic/array/struct, implicit conversion, constructor.
- Overload resolution: argument type, conversion ranking, ambiguity, built-in vs user, in/out/inout.
- L-value/r-value: assignment target, swizzle write mask, array index, in/out/inout restriction.
- Stage semantics: vertex/fragment/geometry/tessellation/compute interface + built-in.
- Diagnostic: file/line/column, token span, error code ổn định.
- Regression: pow2 (overload cùng arity khác kiểu), iris_Entity (input activity qua helper), dayBlend (arity sau preprocess).

## Migration / rollback
- Giữ `glsl_to_msl.cpp` làm **compatibility fallback** (Candidate C, D-011) trong migration. Shader chọn fallback phải có diagnostics rõ, test riêng, telemetry. Không silently fallback nếu đường mới fail rồi trả compile success.
- Cắt converter cũ khi corpus 86 shader byte-identical + test shaderpack pass trên Metal.

## Reviewer / owner
Lead (Agent 0). Agent 1 thực hiện frontend.
