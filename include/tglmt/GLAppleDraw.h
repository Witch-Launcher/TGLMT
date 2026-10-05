#pragma once
// M5b: thực thi draw GL trên Apple backend (gl_apple_draw.cpp).
// Trả true nếu đã encode GPU thật (trace vẫn ghi riêng ở EmitDraw).
#include "tglmt/gl46_types.h"
#include <string>
namespace tglmt {
bool AppleDrawGL(GLenum mode, GLsizei count, GLenum indexType, const void* indexData,
                 bool indexed, size_t indexByteOff, GLuint eboId, GLsizei inst,
                 GLint baseVertex, GLuint baseInstance, GLint first = 0);
// glClear immediate: clear NGAY trên FBO đang bind (đúng GL semantics).
// Trả false → caller fallback cơ chế deferred applePendingClear (Null backend,
// chưa có target, makeClearEncoder fail).
bool AppleClearNow(GLbitfield mask);
// Ghi MSL đã convert (vs+fs) của program ra dir (chẩn đoán trên máy).
// Trả false khi program không tồn tại/chưa link.
bool DumpProgramMSL(GLuint program, const std::string& dir, std::string& err);
} // namespace tglmt
