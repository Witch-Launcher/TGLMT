// gl_shader.cpp — Shader objects: GLSL → MSL bằng GLSLConverter thật (subset có tài liệu).
// Spec: GLSLangSpec.4.60.pdf + MSL spec. Ngoài subset → COMPILE_STATUS false + infoLog
// rõ ràng (trung thực, không sinh code đoán mò).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLSLConverter.h"
using namespace tglmt;

namespace tglmt::gl {
GLuint glCreateShader(GLenum type) {
    Context& c = Context::Current();
    GLuint id; c.registry.Create(ObjectKind::Shader, 1, &id);
    ShaderObject s; s.id = id; s.type = type;
    c.shaders[id] = s;
    return id;
}
void glDeleteShader(GLuint s) {
    Context& c = Context::Current();
    GLuint a[1]={s}; c.registry.Delete(ObjectKind::Shader, 1, a);
    c.shaders.erase(s);
}
GLboolean glIsShader(GLuint s) {
    return Context::Current().registry.Is(ObjectKind::Shader, s) ? 1 : 0;
}
void glShaderSource(GLuint s, GLsizei n, const GLchar* const* str, const GLint* len) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    std::string src;
    for (GLsizei i = 0; i < n; ++i) {
        if (len && len[i] >= 0) src.append(str[i], (size_t)len[i]);
        else src.append(str[i]);
    }
    it->second.source = src;
}
void glCompileShader(GLuint s) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    if (it->second.source.empty()) { it->second.compiled = false; it->second.infoLog = "error: empty source"; return; }
    uint32_t stage = it->second.type;
    if (stage != 0x8B31 && stage != 0x8B30) {
        // TCS/TES/GS/COMPUTE: converter M5-vanilla chỉ VS/FS.
        // Trung thực: COMPILE_STATUS false + log rõ (trước đây true giả → link sai).
        // Tess/GS/compute thật là mục tiêu Sodium/Iris (xem ROADMAP tess-GS-compute).
        it->second.conv = GLSLConvertResult{};
        it->second.msl.clear();
        it->second.compiled = false;
        const char* nm = (stage == 0x8E88 || stage == 0x8E87) ? "tessellation"
            : (stage == 0x8DD9) ? "geometry" : (stage == 0x91B9) ? "compute" : "unknown-stage";
        it->second.infoLog = std::string("error: ") + nm +
            " shader ngoài subset vanilla (TCS/TES/GS/COMPUTE cần đường tess/mesh/compute riêng)";
        return;
    }
    GLSLConvertResult r = ConvertGLSLtoMSL(it->second.source, stage);
    it->second.conv = r;
    it->second.msl = r.ok ? r.msl : "";
    it->second.compiled = r.ok;
    it->second.infoLog = r.ok ? "" : ("error: " + r.log);
}
void glGetShaderSource(GLuint s, GLsizei n, GLsizei* l, GLchar* src) {
    Context& c = Context::Current();
    auto it = c.shaders.find(s);
    if (it == c.shaders.end()) { c.errors.Record(0x0502); return; }
    size_t k = std::min((size_t)n - 1, it->second.source.size());
    memcpy(src, it->second.source.c_str(), k); src[k] = 0;
    if (l) *l = (GLsizei)k;
}
void glSpecializeShader(GLuint s, const GLchar* pEntry, GLuint n, const GLuint* idx, const GLuint* val) {
    // GL 4.6 + ARB_gl_spirv: entry-point specialization cho SPIR-V. TGLMT: lưu + recompile MSL.
    (void)pEntry; (void)n; (void)idx; (void)val;
    glCompileShader(s);
}
// glShaderStorageBlockBinding định nghĩa duy nhất ở gl_uniform.cpp
} // namespace tglmt::gl
