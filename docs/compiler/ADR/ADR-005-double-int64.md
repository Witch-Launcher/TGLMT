# ADR-005 — Double precision và integer widths (int64/uint64) trên Metal

## Trạng thái
Chốt.

## Context
Metal trên A11/iOS 16 không có primitive double (MSL `double` không phải kiểu scalar đầu-class trên iOS family; GPU A11 có FP64 hạn chế/không trực tiếp trong MSL shader). GLSL 4.60 có `double`, `dvec2/3/4`, `dmat2/3/4`, `mat2x3` v.v., và int64/uint64 qua extension. TGLMT không được cast double→float và ghi là đúng (ISS-009, §0.1).

## Quyết định
- **double / dvec / dmat**: **UNSUPPORTED** trong shader compile trên mục tiêu A11 — compiler phải **phát lỗi rõ** khi shader dùng `double`/`dvec*`/`dmat*` và không thể đảm bảo độ chính xác, KHÔNG silently hạ xuống float. `glUniform*d` upload path (CPU) có thể nhận giá trị double nhưng upload xuống buffer lowered thành float với ghi nhận mất precision (đã documented, không phải shader compile success).
- **int64 / uint64**: **UNSUPPORTED** trong shader (parser nhận type nhưng không lower được đúng trên Metal) ⇒ compiler báo lỗi rõ nếu shader dùng int64/uint64 operation ngoài khả năng emulate.
- **Emulation (tùy chọn sau khi có benchmark)**: double-double (pair-of-float) cho tập operation giới hạn, có sai số có giới hạn, tính phí performance/memory. Chỉ bật khi có extension/hardware variant hỗ trợ thật + test equivalence. Nếu không đảm bảo ⇒ fail rõ.

## Lý do
- Không tự suy diễn thông số GPU/Metal feature set (nên §0.2).
- "Parser nhận type ≠ hỗ trợ" (ISS-009, §5.2).
- Không quảng cáo capability chưa implement/test.

## Test plan
- Shader có `double`/`dvec4`/`dmat4` ⇒ `glCompileShader` trả FALSE + info log rõ (không phải Metal compile fail ngầm).
- Shader có `int64_t`/uint64 operation ⇒ FAIL rõ.
- Không có test pixel double (không hỗ trợ).

## Owner
Agent 1 (frontend/type-check) + Agent 3 (backend legalization). Review Lead.
