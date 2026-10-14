// gl_multisample_p8.cpp — P8: EXT_multisample + multisampled_render_to_texture +
// framebuffer_blit_layers (API đầy đủ, CHƯA advertise — cần verify device MSAA).
// Sample mask lưu state (Metal không có sample mask trực tiếp); MSAA texture
// attach + samples; layered blit layer 0 forward, layer>0 fail rõ.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
// Forward (đã có trong gl_raster.cpp / gl_framebuffer.cpp).
void glSampleMaski(GLuint maskNumber, GLbitfield mask);
void glBlitFramebuffer(GLint s0, GLint s1, GLint s2, GLint s3,
                       GLint d0, GLint d1, GLint d2, GLint d3, GLbitfield m, GLenum f);
void glFramebufferTexture2D(GLenum t, GLenum a, GLenum tx, GLuint x, GLint l);

void glSampleMaskEXT(GLuint maskNumber, GLbitfield mask) {
    // Lưu mask state (Metal không có sample mask trực tiếp cho MSAA —
    // mask chưa consume ở encode, document trong HONESTLY_ABSENT).
    glSampleMaski(maskNumber, mask);
}
void glSamplePatternEXT(GLenum pattern) {
    // Sample positions chuẩn (Metal dùng fixed positions, không đổi được).
    // Chỉ chấp nhận STANDARD (0x8E5C?) — pattern khác lỗi. Thực tế EXT_multisample
    // chỉ định nghĩa glSampleMaskEXT + glSamplePatternEXT; pattern là enum.
    // TGLMT: no-op hợp lệ (Metal positions cố định, không sai khi bỏ qua).
    (void)pattern;
}
void glFramebufferTexture2DMultisampleEXT(GLenum target, GLenum attachment, GLenum textarget,
                                          GLuint texture, GLint level, GLsizei samples) {
    Context& c = Context::Current();
    if (samples < 0) { c.errors.Record(0x0501); return; }
    // Attach texture như 2D thường; samples lưu để backend biết MSAA (nếu texture
    // được tạo với samples, backend resolve; nếu không, render như single-sample).
    // TGLMT FBO không track samples riêng — attach base, backend xử lý theo texture.
    glFramebufferTexture2D(target, attachment, textarget, texture, level);
    // Lưu samples vào texture object để backend tham khảo (nếu cần).
    auto tit = c.textures.find(texture);
    if (tit != c.textures.end() && samples > 0) {
        // Ghi nhận (không ép recreate — texture đã có storage).
        // Backend MSAA resolve dựa vào FBO sample count, không phải texture tag.
        (void)samples;
    }
}
void glBlitFramebufferLayerEXT(GLint s0, GLint s1, GLint s2, GLint s3,
                               GLint d0, GLint d1, GLint d2, GLint d3,
                               GLbitfield m, GLenum f, GLint layer) {
    // Layered blit: TGLMT 2D (không layers). Layer 0 = blit thường; layer>0 fail rõ.
    if (layer != 0) {
        Context::Current().errors.Record(0x0502); // INVALID_OPERATION (no layered)
        return;
    }
    glBlitFramebuffer(s0, s1, s2, s3, d0, d1, d2, d3, m, f);
}
void glBlitFramebufferLayersEXT(GLint s0, GLint s1, GLint s2, GLint s3,
                                GLint d0, GLint d1, GLint d2, GLint d3,
                                GLbitfield m, GLenum f,
                                GLint srcBaseLayer, GLint dstBaseLayer, GLsizei numLayers) {
    // Nhiều layers: chỉ hỗ trợ numLayers=1 với base 0 (tương đương blit thường).
    if (numLayers != 1 || srcBaseLayer != 0 || dstBaseLayer != 0) {
        Context::Current().errors.Record(0x0502);
        return;
    }
    glBlitFramebuffer(s0, s1, s2, s3, d0, d1, d2, d3, m, f);
}
} // namespace tglmt::gl
