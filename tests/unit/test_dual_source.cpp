// test_dual_source.cpp — Dual-source blend (GL 3.3 core): layout(index),
// glBindFragDataLocationIndexed, MAX_DUAL_SOURCE_DRAW_BUFFERS, MSL color(0/1).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_dual_source FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
// Dual-source qua layout(location=0, index=0/1).
static const char* kFSDualLayout =
    "#version 460 core\n"
    "layout(location=0, index=0) out vec4 src0;\n"
    "layout(location=0, index=1) out vec4 src1;\n"
    "void main(){ src0 = vec4(1,0,0,1); src1 = vec4(0,0,1,1); }\n";
// Dual-source qua glBindFragDataLocationIndexed (không layout index).
static const char* kFSNoLayout =
    "#version 460 core\n"
    "out vec4 a;\n"
    "out vec4 b;\n"
    "void main(){ a = vec4(1); b = vec4(1); }\n";

static GLuint Make(const char* fssrc) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr); glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fssrc, nullptr); glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    return p;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // Limit thật.
    GLint maxDual = 0;
    glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, &maxDual);
    Check(maxDual == 1, "GL_MAX_DUAL_SOURCE_DRAW_BUFFERS == 1");
    Check(glGetError() == GL_NO_ERROR, "no error after limit query");

    // layout(location, index) → MSL [[color(0)]] + [[color(1)]].
    GLuint p = Make(kFSDualLayout);
    glLinkProgram(p);
    {
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        Check(ok == GL_TRUE, "dual layout links");
    }
    Check(ctx.programs[p].fragmentMSL.find("[[color(0)]]") != std::string::npos,
          "MSL emits [[color(0)]]");
    Check(ctx.programs[p].fragmentMSL.find("[[color(1)]]") != std::string::npos,
          "MSL emits [[color(1)]]");
    Check(glGetFragDataLocation(p, "src0") == 0, "src0 location 0");
    Check(glGetFragDataIndex(p, "src0") == 0, "src0 index 0");
    Check(glGetFragDataLocation(p, "src1") == 0, "src1 location 0 (dual)");
    Check(glGetFragDataIndex(p, "src1") == 1, "src1 index 1");
    glDeleteProgram(p);

    // glBindFragDataLocationIndexed trước link → color(0)+color(1).
    p = Make(kFSNoLayout);
    glBindFragDataLocationIndexed(p, 0, 0, "a");
    glBindFragDataLocationIndexed(p, 0, 1, "b");
    glLinkProgram(p);
    {
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        Check(ok == GL_TRUE, "dual bind links");
    }
    Check(ctx.programs[p].fragmentMSL.find("[[color(0)]]") != std::string::npos,
          "bind: MSL [[color(0)]]");
    Check(ctx.programs[p].fragmentMSL.find("[[color(1)]]") != std::string::npos,
          "bind: MSL [[color(1)]]");
    Check(glGetFragDataLocation(p, "a") == 0, "bind a location 0");
    Check(glGetFragDataIndex(p, "a") == 0, "bind a index 0");
    Check(glGetFragDataLocation(p, "b") == 0, "bind b location 0");
    Check(glGetFragDataIndex(p, "b") == 1, "bind b index 1");
    glDeleteProgram(p);

    // Index ≠ 0 nhưng location ≠ 0 → compile fail (GL 3.3 dual-source chỉ attachment 0).
    {
        const char* bad =
            "#version 460 core\n"
            "layout(location=1, index=1) out vec4 x;\n"
            "void main(){ x = vec4(1); }\n";
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &bad, nullptr); glCompileShader(fs);
        GLint ok = 1;
        glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
        Check(ok == 0, "location=1,index=1 fails compile");
        glDeleteShader(fs);
    }

    // SRC1 blend enums có giá trị chuẩn (đối chiếu gl.xml / gl46_types.h).
    Check(GL_SRC1_COLOR == 0x88F9, "GL_SRC1_COLOR value");
    Check(GL_ONE_MINUS_SRC1_COLOR == 0x88FA, "GL_ONE_MINUS_SRC1_COLOR value");
    Check(GL_SRC1_ALPHA == 0x8589, "GL_SRC1_ALPHA value");
    Check(GL_ONE_MINUS_SRC1_ALPHA == 0x88FB, "GL_ONE_MINUS_SRC1_ALPHA value");
    Check(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS == 0x88FC, "GL_MAX_DUAL_SOURCE_DRAW_BUFFERS enum");

    if (gFails == 0) { printf("test_dual_source: OK\n"); return 0; }
    return 1;
}
