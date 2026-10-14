// gl_draw_buffers2_p8.cpp — P8: EXT_draw_buffers2 (indexed per-target control).
// Per-buffer color mask + enable/disable + queries (MRT). Forward sang core
// indexed functions (glColorMaski/glEnablei/...) đã có; queries xử lý COLOR_WRITEMASK.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
using namespace tglmt;

namespace tglmt::gl {
// Forward declarations (core indexed, đã implement trong gl_raster.cpp).
void glColorMaski(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
void glEnablei(GLenum target, GLuint index);
void glDisablei(GLenum target, GLuint index);
GLboolean glIsEnabledi(GLenum target, GLuint index);

void glColorMaskIndexedEXT(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    glColorMaski(buf, r, g, b, a);
}
void glEnableIndexedEXT(GLenum target, GLuint index) { glEnablei(target, index); }
void glDisableIndexedEXT(GLenum target, GLuint index) { glDisablei(target, index); }
GLboolean glIsEnabledIndexedEXT(GLenum target, GLuint index) { return glIsEnabledi(target, index); }
void glGetBooleanIndexedvEXT(GLenum target, GLuint index, GLboolean* data) {
    Context& c = Context::Current();
    if (!data) return;
    // COLOR_WRITEMASK (0x0C23): trả mask per-buffer dưới dạng 4 bools.
    if (target == 0x0C23) {
        uint8_t m = c.state.ColorMask(index);
        data[0] = (m & 1) ? 1 : 0; data[1] = (m & 2) ? 1 : 0;
        data[2] = (m & 4) ? 1 : 0; data[3] = (m & 8) ? 1 : 0;
        return;
    }
    c.errors.Record(0x0500); // INVALID_ENUM (target khác chưa hỗ trợ indexed query)
}
void glGetIntegerIndexedvEXT(GLenum target, GLuint index, GLint* data) {
    Context& c = Context::Current();
    if (!data) return;
    if (target == 0x0C23) { // COLOR_WRITEMASK
        uint8_t m = c.state.ColorMask(index);
        data[0] = (m & 1) ? 1 : 0; data[1] = (m & 2) ? 1 : 0;
        data[2] = (m & 4) ? 1 : 0; data[3] = (m & 8) ? 1 : 0;
        return;
    }
    c.errors.Record(0x0500);
}
} // namespace tglmt::gl
