// tglmt/limits.h — giới hạn implementation (GL 4.6 §7.4 "Implementation
// Dependent Values").
//
// Trước đây các giá trị này nằm rải rác trong switch của glGetIntegerv và
// không có nơi nào khác tham chiếu được → validation uniform (§7.6.1) buộc
// phải hardcode lại. Ở đây là MỘT nguồn: gl_get.cpp đọc từ đây, validation
// đọc từ đây.
#pragma once

namespace tglmt {

// GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS — tổng số texture unit cho mọi stage.
inline constexpr int kMaxCombinedTextureImageUnits = 48;

// GL_MAX_IMAGE_UNITS — số image unit cho mỗi stage.
inline constexpr int kMaxImageUnits = 8;

}  // namespace tglmt
