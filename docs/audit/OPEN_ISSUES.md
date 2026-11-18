# OPEN_ISSUES.md — TGLMT

Danh sách vấn đề đã xác định từ Phase 0 reconnaissance + log shaderpack. Mỗi issue: ID, severity, file/symbol, bằng chứng, cách tái hiện, ảnh hưởng, nguyên nhân gốc (đã chứng minh/hypothèse), test cần thêm, trạng thái.

Severity: P0 (chặn release / sai ngữ nghĩa shader), P1 (sai API/state), P2 (thiếu cobertura / không verify), P3 (docs/perf).

## ISS-001 — GLSL compiler là string rewriter, không có AST/type system (Nguyên nhân gốc chung)

- Severity: P0
- File/symbol: `src/gl/glsl_to_msl.cpp` (~13 057 LOC), `include/tglmt/GLSLConverter.h`
- Bằng chứng: docs/GLSL_460_COMPILER.md §1 — pipeline là comment strip → rename keyword → preprocessor line-oriented → split struct → scan déclaration → ~60 pass rewrite văn bản → emit MSL. "There is no AST, no type system, no overload resolution and no dataflow analysis."
- Cách tái hiện: Bất kỳ shader GLSL hợp lệ nhưng ngoài subset (struct lồng, overload phức tạp, std140, double, array struct, ...) sẽ được convert OK nhưng sinh MSL sai hoặc Metal compile fail ngầm.
- Ảnh hưởng: Nguyên nhân gốc ISS-002/ISS-003/ISS-004; không thể mở rộng GLSL 4.60 an toàn.
- Nguyên nhân gốc: Đã chứng minh (tài liệu + source). Kiến trúc hiện tại dùng substring match cho mọi quyết định ngữ nghĩa.
- Test cần thêm: Compiler frontend unit test (lexer/parser/type-check) độc lập; differential test GLSL→MSL vs reference.
- Trạng thái: MỚI (chưa sửa). Thiết kế ADR tại docs/compiler/ADR/.

## ISS-002 — Complementary `pow2`: overload cùng tên/cùng arity khác kiểu bị sai kiểu trả về

- Severity: P0
- File/symbol: `src/gl/glsl_to_msl.cpp` (helper-call analysis, `RewriteIdents`, pass built-in)
- Bằng chứng: latestlog 2.txt:1957 — Metal từ chối MSL: hàm trả `int` nhưng `return pow2(x) * float3(x)` trả `float3`. test_helper_overloads.cpp đã覆盖 phần overload cùng arity khác kiểu (PASS trên unit test đơn lẻ) nhưng chưa khai thác đường pipeline đầy đủ (convert → link → MSL compile → Metal compile thật).
- Cách tái hiện: Shader GLSL định nghĩa 5 overload `pow2(int/float/vec2/vec3/vec4)`, gọi `pow2(vec3)` gán vào `float3`, convert + link + Metal compile.
- Ảnh hưởng: deferred1.vsh của Complementary Reimagined R5.9.3 bị Metal từ chối → pack chết.
- Nguyên nhân gốc: Hàm phân tích helper arity/resolution dựa trên văn bản, không có type system ⇒ suy luận lại kiểu gọi sai.
- Test cần thêm: `test_regression_shaderpack_return_type` — convert + link + `xcrun metal -c` + kiểm tra kiểu biểu thức trả về trong MSL/AST.
- Trạng thái: Chưa có regression test đầy đủPipeline; unit test overload có PASS nhưng chưa khâu Metal compile thật đầy đủ.

## ISS-003 — BSL `iris_Entity` thiếu field trong `TGLMT_VIn` (stage input không nhất quán)

- Severity: P0
- File/symbol: `src/gl/glsl_to_msl.cpp` (lọc input bằng quét text, lines 11500-11573), `src/gl/gl_program.cpp` (varying/attribute match, attribLoc), `include/tglmt/GLSLConverter.h` (`GLSLVar.forceUnsignedAttr`)
- Bằng chứng: latestlog 2.txt:2294/2812 — MSL gọi `_in.iris_Entity` trong khi struct `TGLMT_VIn` không khai báo field đó. attribute được inject bởi Iris EntityPatcher (`in ivec3 iris_Entity`) nhưng draw format của TEXTURED không có element Entity ⇒ không bind ⇒ link gán "từ 0" ⇒ field bị strip / location lệch.
- Cách tái hiện: Shader vertex có `in ivec3 iris_Entity` dùng trong helper, link với fragment, sinh MSL, kiểm tra `TGLMT_VIn` có field `iris_Entity` với kiểu/location đúng.
- Ảnh hưởng: BSL 10.1.8 TEXTURED (và pack khác dùng iris_Entity) pipeline lỗi.
- Nguyên nhân gốc: Converter lọc/strip input bằng quét văn bản; linker gán location attribute không đồng bộ với MSL đã sinh (attribute active/inactive, macro, struct/interface block).
- Test cần thêm: `test_regression_shaderpack_iris_entity` — attribute active qua helper, location/reflection khớp, Metal compile/link pass.
- Trạng thái: Chưa có test tái hiện đầy đủ.

## ISS-004 — E-LITE `dayBlend` báo sai arity (co 3)

- Severity: P0
- File/symbol: `src/gl/glsl_to_msl.cpp` (helper-call graph / arity check, lines 7486-7490, 7736-7740)
- Bằng chứng: latestlog 2.txt:2508 — Converter từ chối `helper dayBlend sai arity (co 3)`. Chưa có source sau preprocess/import ⇒ chưa kết luận false positive.
- Cách tái hiện: Lấy source `prepare.vsh` sau preprocess/import (kèm `#moj_import` / include), phân tích function declaration/overload/call-site, kiểm tra arity thực tế.
- Ảnh hưởng: E-LITE 5.1.3 shader từ chối convert.
- Nguyên nhân gốc: Giả thuyết — phân tích helper dựa trên văn bản trước preprocess ⇒ đếm sai tham số do macro / nested call / default / array / qualifier.
- Test cần thêm: `test_regression_shaderpack_helper_overload` — source gốc + source sau preprocess, xác nhận parser arity/overload đúng; GLSL hợp lệ convert, GLSL sai arity có compile status/log đúng.
- Trạng thái: Chưa có source pack ⇒ test chưa viết; cần lấy source hoặc minimal reproduction.

## ISS-005 — MakeUp `fixedTaaCorrection`: warn Iris expression resolver (ngoài TGLMT) + `iris_Entity` pipeline khác

- Severity: P1
- File/symbol: Iris expression resolver (ngoài repo) + `iris_Entity` (xem ISS-003)
- Bằng chứng: latestlog — Iris cảnh báo không resolve custom uniform expression `fixedTaaCorrection` của MakeUp UltraFast; sau đó lỗi `iris_Entity` ở pipeline khác.
- Cách tái hiện: Phân loại cảnh báo Iris (fatal vs không) và lỗi MSL thực sự làm pipeline disable.
- Ảnh hưởng: Có thể không phải lỗi TGLMT; cần tách để không bổ nhiệm sai.
- Nguyên nhân gốc: Chưa xác định (hypothèse).
- Test cần thêm: Nếu TGLMT có liên quan, test adapter uniform expression; nếu không, ghi rõ không phải lỗi TGLMT.
- Trạng thái: Chưa phân loại.

## ISS-006 — Version reporting không nhất quán (Iris thấy GL 4.5, TGLMT context string 4.6)

- Severity: P1
- File/symbol: `src/gl/gl_get.cpp`, `src/gl/gl_error_string_debug.cpp` (`glGetString(GL_VERSION)`), `src/core/Context.cpp` (`SetVersionProfile`), `src/gl/gl_context_api.cpp`
- Bằng chứng: Log Iris `OpenGL 4.5 detected, enabling DSA`; logger khác ghi context string `OpenGL 4.6`. Chưa đủ kết luận version reporting sai; phải điều tra mọi nguồn truy vấn version, profile, extension, GLSL version, capabilities, DSA function availability, thứ tự khởi tạo context.
- Cáchéré hiện: Gọi `glGetString(GL_VERSION)`, `glGetIntegerv(GL_MAJOR/MINOR_VERSION)`, `GL_SHADING_LANGUAGE_VERSION`, `glGetStringi(GL_EXTENSIONS)`, extension function pointers, `GL_MAX_*`, GLSL `#version`/`#extension` acceptance trên nhiều context version/profile.
- Ảnh hưởng: Iris chọn nhánh code 4.5 (DSA) trong khi TGLMT báo 4.6 ⇒ hành vi không đồng nhất.
- Nguyên nhân gốc: Chưa xác định (hypothèse). Có thể do nhiều source version (context state vs build string vs GLSL version).
- Test cần thêm: Test truy vấn version đầy đủ trên các context 1.0–4.6 × Core/Compat; so sánh output giữa các nguồn.
- Trạng thái: Chưa có test.

## ISS-007 — Preprocessor GLSL không đủ ngữ pháp chuẩn

- Severity: P1
- File/symbol: `src/gl/glsl_to_msl.cpp` (preprocessor inline, line-oriented)
- Bằng chứng: docs/GLSL_460_COMPILER.md §3.1 — `#if` chỉ `defined`/`!`/`&&`/`||`/nguyên số; `#include` hard fail; `#line`/`#pragma`/`#extension` bị bỏ qua.
- Cách tái hiện: Shader có `#if X > 3` hoặc `#include` ⇒ hard fail / sai.
- Ảnh hưởng: Shaderpack dùng preprocessor phức tạp (Minecraft `#moj_import` là custom, Iris defines) bị từ chối.
- Nguyên nhân gốc: Preprocessor line-oriented, không đủ biểu thức `#if` chuẩn.
- Test cần thêm: Preprocessor unit test (`#if` expression, `defined`, nested, `#line`, include/import).
- Trạng thái: Chưa có.

## ISS-008 — Layout std140/std430/packed/shared/row_major không được honor

- Severity: P1
- File/symbol: `src/gl/glsl_to_msl.cpp` (`MSLAlign`, `MSLSize`, `uniform_pack.h`), `include/tglmt/GLSLConverter.h` (`GLSLBlock`)
- Bằng chứng: docs/GLSL_460_COMPILER.md §3.2 — `layout(std140)`/`std430`/`packed`/`shared`/`row_major` bị bỏ qua; layout luôn MSL natural alignment.
- Cách tái hiện: UBO/SSBO `std140` với struct lồng / vec3 padding / mat3 ⇒ byte layout sai so với spec.
- Ảnh hưởng: Uniform block data lệch offset ⇒ geometry/matrix rác.
- Nguyên nhân gốc: Layout calculator dùng MSL natural alignment, không phải std140/std430.
- Test cần thêm: Layout calculator unit test (byte-level golden), UBO/SSBO array stride, struct member, nested struct, matrix major order.
- Trạng thái: Chưa có test byte-level.

## ISS-009 — `double`/`dvec*` hạ xuống float mà không báo sai độ chính xác

- Severity: P1
- File/symbol: `src/gl/glsl_to_msl.cpp` (`MSLType`, lines 46-53)
- Bằng chứng: `MSLType` trả `"float"` cho `double`/`dvec*` với comment "Độ chính xác bị giảm (documented limitation)". Không báo lỗi compile khi shader dùng double.
- Cách tái hiện: Shader dùng `double`/`dmat4` ⇒ convert OK, sinh float, không có cảnh báo sai precision.
- Ảnh hưởng: Shader yêu cầu double precision sẽ chạy sai số nhưng TGLMT báo success.
- Nguyên nhân gốc: Metal không có double primitive; chọn hạ cấp im lặng.
- Test cần thêm: Test detect double trong shader ⇒ báo lỗi rõ hoặc capability; không được silently cast.
- Trạng thái: Chưa có test.

## ISS-010 — Unknown identifiers chỉ phát hiện ở Metal compile/link, không ở GLSL compile

- Severity: P1
- File/symbol: `src/gl/glsl_to_msl.cpp`, `src/gl/gl_shader.cpp` (`glCompileShader`)
- Bằng chứng: Converter pass-through biểu thức; tên không khai báo chỉ bị Metal từ chối lúc compile MSL ⇒ `glCompileShader` trả OK nhưng Metal fail ⇒ status bị nhập nhầm.
- Cách tái hiện: Shader dùng biến chưa khai báo ⇒ convert OK, MSL compile fail.
- Ảnh hưởng: Diagnostics sai giai đoạn, source location không chính xác.
- Nguyên nhân gốc: Không có symbol table/type check ⇒ không phát hiện unknown identifier ở GLSL compile.
- Test cần thêm: Shader lỗi tên biến ⇒ `glCompileShader` trả FALSE + info log có file/line/column.
- Trạng thái: Chưa có.

## ISS-011 — `glSpecializeShader` bỏ qua entry point/specialization constants

- Severity: P1
- File/symbol: `src/gl/gl_shader.cpp` (lines 103-107)
- Bằng chứng: `(void)pEntry; (void)n; (void)idx; (void)val; glCompileShader(s);` — bỏ qua toàn bộ tham số specialization.
- Cách tái hiện: Gọi `glSpecializeShader` với entry point/constants ⇒ không có hiệu lực.
- Ảnh hưởng: ARB_gl_spirv specialization không đúng spec.
- Nguyên nhân gốc: Implementation stub.
- Test cần thêm: Test specialization constants thay đổi shader behavior.
- Trạng thái: Stub.

## ISS-012 — Geometry/tessellation/compute capability matrix không rõ ràng

- Severity: P1
- File/symbol: `src/gl/gl_geometry_emulation.cpp`, `src/gl/gl_tess_xfb.cpp`, `gl_program.cpp`
- Bằng chứng: Comment/tài liệu khác nhau về mức hỗ trợ. GS emulation có; tess fail rõ tại link cho phần chưa hỗ trợ (`gl_Layer`, XFB+GS, `invocations > 1`, adjacency); compute có kernel.
- Cách tái hiện: Liệt kê từng feature GS/tess/compute, kiểm tra có test pixel thật không.
- Ảnh hưởng: Không biết chính xác phần nào NATIVE/EMULATED/PARTIAL/HARDWARE_LIMITED/UNSUPPORTED.
- Nguyên nhân gốc: Không có capability matrix từ implementation + test thật.
- Test cần thêm: Ma trận capability + test cho mỗi feature.
- Trạng thái: Chưa có ma trận.

## ISS-013 — Test Minecraft corpus dùng Context "null", không chứng minh MSL compile trên Metal

- Severity: P2
- File/symbol: `tests/integration/test_mc_corpus.cpp` (`Context ctx("null")`)
- Bằng chứng: Test convert/link nhưng không qua `xcrun metal -c` cho mọi shader corpus.
- Cách tái hiện: Chạy test_mc_corpus, kiểm tra có gate Metal compile không.
- Ảnh hưởng: Có thể bỏ lỗi MSL compile trên shader Minecraft.
- Nguyên nhânรก: Thiếu Metal compile gate trong corpus test.
- Test cần thêm: Thêm gate `xcrun metal -c` cho mỗi shader corpus.
- Trạng thái: Chưa có.

## ISS-014 — Corpus shader nhỏ, chưa đại diện Minecraft/Iris

- Severity: P2
- File/symbol: `tests/shaders/test_shader_corpus.cpp`, `tests/corpus/`
- Bằng chứng: PROGRESS.md — corpus nhỏ, chưa đủ đại diện cho Minecraft/Iris.
- Cáchéen hiện: Thêm shaderpack được phép (Complementary, BSL, E-LITE, MakeUp) theo local path, không commit proprietary.
- Ảnh hưởng: Không phát hiện lỗi shaderpack thật.
- Test cần thêm: Mở rộng corpus, chạy qua các gate (frontend/semantic/SPIR-V/MSL/pipeline/render).
- Trạng thái: Chưa có.

## ISS-015 — Silent stub / no-op advertise như supported

- Severity: P2
- File/symbol: `src/gl/` các file legacy (`gl_legacy_*.cpp`, `gl_ext_*.cpp`)
- Bằng chứng: PROGRESS.md — 240 legacy lệnh là stub trung thực (`UNIMPLEMENTED` + giá trị an toàn);GLU chưa có; accumulation buffer chỉ state.
- Cách tái hiện: Gọi lệnh stub ⇒ không lỗi nhưng không làm gì.
- Ảnh hưởng: App giả tưởng feature có sẵn.
- Nguyên nhân gốc: Stub có giá trị an toàn nhưng được advertise như supported.
- Test cần thêm: Test negative (stub phải trả lỗi/capability rõ, không giả success).
- Trạng thái: Một phần documented.

## ISS-016 — Extension advertisement chưa đồng bộ với implementation

- Severity: P2
- File/symbol: `src/gl/gl_get.cpp` (`GL_EXTENSIONS`/`glGetStringi`), extension files
- Bằng chứng: TODO.md T0.11 — `GL_EXTENSIONS` phải phản ánh đúng extension theo (version, profile); 76 extension có hàm chưa implement.
- Cách tái hiện: Tra cứu extension advertised ⇒ kiểm tra implementation thực sự.
- Ảnh hưởng: App dùng extension ⇒ sai behavior.
- Nguyên nhân gốc: Advertise extension chưa implement/test.
- Test cần thêm: Extension matrix + test behavior.
- Trạng thái: Chưa có.

## ISS-017 — Không có shader dump / source-map tool

- Severity: P3
- File/symbol: Chưa có `tglmt-shader-dump`, `tglmt-shader-validate`, `tglmt-shader-compile`
- Bằng chứng: Mục 8 của prompt yêu cầu công cụ diagnostics.
- Cách tái hiện: Không có CLI để dump GLSL/AST/SPIR-V/MSL.
- Ảnh hưởng: Khó chẩn đoán lỗi shader từ xa.
- Test cần公開: Không.
- Trạng thái: Chưa có.

---
*Tổng: 17 issue (P0: 4, P1: 6, P2: 5, P3: 1). Tiếp theo: docs/audit/FEATURE_INVENTORY.md, sau đó docs/compiler/ADR/.*
