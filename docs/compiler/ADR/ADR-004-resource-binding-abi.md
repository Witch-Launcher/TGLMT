# ADR-004 — Resource binding ABI giữa GL locations/bindings và Metal slots

## Trạng thái
Chốt.

## Context
OpenGL dùng location (attribute/varying/uniform/fragment output) và binding point (UBO/SSBO/image/atomic/sampler unit). Metal dùng buffer slot `[[buffer(k)]]`, texture slot `[[texture(k)]]`, sampler slot `[[sampler(k)]]`, attribute index. Cần mapping rõ ràng, không xung đột, ổn định qua relink.

## Quyết định
- **Attribute**: location = Metal attribute index (0..15, `[[attribute(N)]]`). Nếu thiếu layout ⇒ gán chỉ số trống nhỏ nhất từ 0 (không đụng explicit/bind đã chiếm). 2 attribute active cùng index ⇒ fail rõ tại link.
- **Varying (VS→FS)**: `[[user(locnN)]]`; location gán ở link (explicit khớp nhau wins; cả 2 tạm ⇒ số mới từ 0; explicit khác nhau ⇒ link lỗi).
- **Fragment output**: `[[color(N)]]`; layout(location=) thắng, rồi glBindFragDataLocation(Indexed), rồi thứ tự khai báo. Dual-source index+color.
- **Uniform block (UBO)**: `constant TGLMTUniforms& [[buffer(16)]]`; block slot theo thứ tự khai báo trong từng stage, `[[buffer(17+k)]]`; backend bind `uniformBindPoints[binding]` → buffer(slot). Giữ binding từ lần link trước (glUniformBlockBinding sau link vẫn override).
- **SSBO**: `device T* [[buffer(slot)]]`, slot `17+k` xen kẽ cùng dải với UBO mỗi stage; `glShaderStorageBlockBinding` override binding theo tên, giữ qua relink.
- **Image**: `texture2d<T, access::...> [[texture(slot)]]`, slot = samplerCount + imageIndex (chung index space texture); binding = layout binding hoặc sampler unit.
- **Atomic counter**: `device atomic_uint* [[buffer(slot)]]`, slot = ssboCnt + index (sau SSBO), binding + offset từ layout.
- **Sampler**: texture slot `[[texture(k)]]`, sampler slot `[[sampler(k)]]` (-1 = dùng chung index với texture). `samplerUnits[tên] = 0` mặc định (đúng GL).
- **Uniform (non-block)**: gộp vs trước fs sau, mỗi stage align 16 đầu khối; offset/size từ converter; `uniformLayout` tra O(1).
- **Compute**: UBO/SSBO/sampler/image/atomic slots riêng của compute stage; `dispatch3D` với `threadsPerThreadgroup` từ `layout(local_size)`.

## Nguyên tắc
- Không tự tính slot từ binding point (tránh lệch slot khi vs/fs khai báo khác nhau ⇒ fs đọc nhầm buffer ⇒ GPU hang).
- Giữ binding qua relink (oldBinds) ⇒ app gọi glUniformBlockBinding sau link lần 1 không cần gọi lại.
- Attribute inactive đã bị strip converter ⇒ remaining đều ACTIVE.
- Pipeline key phải bao gồm linkGen ⇒ không bao giờ trả nhầm pipeline shader cũ.

## Trade-offs
- Gộp uniform vs+fs (tương thích cũ) nhưng AppleDrawGL chuyển về stage-local khi upload ⇒ phức tạp 중간; thay vào đó có thể tách hoàn toàn per-stage. Giữ gộp để không phá pipeline hiện có; tối ưu sau.

## Test plan
- UBO/SSBO byte-level golden test (std140/std430, struct lồng, vec3 padding, mat2/3/4, row/column-major, explicit offset/align/binding/location).
- Binding override + relink giữ binding.
- Sampler unit default 0; texture/sampler slot không xung đột.
- Compute dispatch slots.
- Regression: pow2/iris_Entity/dayBlend không liên quan trực tiếp nhưng resource binding ảnh hưởng uniform.

## Owner
Agent 3 (backend) + Agent 5 (lifecycle/link). Review Lead.
