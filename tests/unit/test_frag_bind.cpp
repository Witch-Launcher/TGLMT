// test_frag_bind.cpp — glBindFragDataLocation / Indexed reflection.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_frag_bind FAIL: %s\n", msg); ++gFails; }
}
static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFSNoLayout =
    "#version 460 core\n"
    "out vec4 color;\n"
    "void main(){ color = vec4(1); }\n";
static const char* kFSLayout =
    "#version 460 core\n"
    "layout(location=2) out vec4 color;\n"
    "void main(){ color = vec4(1); }\n";

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

    GLuint p = Make(kFSNoLayout);
    glBindFragDataLocation(p, 1, "color");
    glLinkProgram(p);
    GLint loc = glGetFragDataLocation(p, "color");
    Check(loc == 1, "bind frag data location before link");
    Check(glGetFragDataIndex(p, "color") == 0, "frag data index default");
    Check(ctx.programs[p].fragmentMSL.find("[[color(1)]]") != std::string::npos,
          "MSL emits [[color(1)]]");
    glDeleteProgram(p);

    p = Make(kFSNoLayout);
    glBindFragDataLocationIndexed(p, 3, 1, "color");
    glLinkProgram(p);
    Check(glGetFragDataLocation(p, "color") == 3, "indexed bind color");
    Check(glGetFragDataIndex(p, "color") == 1, "indexed bind index");
    glDeleteProgram(p);

    // Explicit layout wins over glBindFragDataLocation (GL spec).
    p = Make(kFSLayout);
    glBindFragDataLocation(p, 1, "color");
    glLinkProgram(p);
    Check(glGetFragDataLocation(p, "color") == 2, "layout location takes precedence");
    Check(ctx.programs[p].fragmentMSL.find("[[color(2)]]") != std::string::npos,
          "MSL emits [[color(2)]]");
    Check(glGetFragDataLocation(p, "missing") == -1, "unknown output = -1");
    glDeleteProgram(p);

    // Error paths.
    while (glGetError() != GL_NO_ERROR) {}
    glBindFragDataLocation(99999, 0, "color");
    Check(glGetError() == GL_INVALID_OPERATION, "bad program");
    p = Make(kFSNoLayout);
    glBindFragDataLocation(p, 8, "color");
    Check(glGetError() == GL_INVALID_VALUE, "bad color number");
    glBindFragDataLocation(p, 0, nullptr);
    Check(glGetError() == GL_INVALID_VALUE, "null name");
    glDeleteProgram(p);

    if (gFails == 0) { printf("test_frag_bind: OK\n"); return 0; }
    return 1;
}
