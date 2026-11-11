// tglmt/UniformQuery.h — đọc giá trị uniform đã lưu (GL 4.6 §7.14).
//
// Dùng chung cho glGetUniform* (src/gl/gl_get.cpp) và glGetnUniform*
// (src/gl/gl_packed_robust.cpp) để hai đường có CÙNG ngữ nghĩa chuyển đổi
// kiểu và CÙNG điều kiện sinh lỗi — trước đây glGetnUniformdv đọc byte
// double qua bộ đệm float ⇒ trả giá trị rác.
#pragma once

#include "tglmt/Context.h"
#include "tglmt/glsl_type.h"

#include <cstdint>
#include <cstring>

namespace tglmt::gl {

// Đọc giá trị uniform `l` của program `pr` vào `out` (kiểu `want`:
// GL_FLOAT / GL_DOUBLE / GL_INT / GL_UNSIGNED_INT), có chuyển đổi kiểu.
// Trả về false nếu đã sinh lỗi theo spec (location không hợp lệ) — khi đó
// KHÔNG ghi gì vào `out`.
bool ReadUniformValueTyped(Context& c, ProgramObject& pr, GLint l, uint32_t want, void* out);

// Kiểm tra program theo spec cho GetUniform*:
//   INVALID_VALUE     : program không phải tên program/shader object
//   INVALID_OPERATION : program là shader object hoặc chưa link thành công
// Trả về nullptr kèm lỗi đã ghi.
ProgramObject* ProgramForGetUniform(Context& c, GLuint p);

}  // namespace tglmt::gl
