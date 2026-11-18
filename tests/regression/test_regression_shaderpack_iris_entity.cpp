// test_regression_shaderpack_iris_entity.cpp — Hồi quy lỗi BSL `iris_Entity`:
// attribute được inject bởi Iris EntityPatcher (`in ivec3 iris_Entity`) và
// dùng trong helper, nhưng draw format không có element Entity ⇒ không được
// bind ⇒ field bị strip khỏi TGLMT_VIn ⇒ Metal: "no member named 'iris_Entity'".
//
// Kiểm tra: attribute active qua helper, location/reflection khớp, kiểu unsigned
// (forceUnsignedAttr vì format UShort4), Metal stage input có field iris_Entity.
// Xem latestlog 2.txt:2294/2812.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cstdio>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 aPos;\n"
    "in ivec3 iris_Entity;\n"
    "out vec4 vColor;\n"
    "ivec3 helperEntity(ivec3 e) { return e; }\n"
    "void main() {\n"
    "    gl_Position = vec4(aPos, 0.0, 1.0);\n"
    "    ivec3 e = helperEntity(iris_Entity);\n"
    "    vColor = vec4(float(e), 1.0);\n"
    "}\n";

static const char* kFS =
    "#version 460 core\n"
    "in vec4 vColor;\n"
    "out vec4 c;\n"
    "void main() { c = vColor; }\n";

int main() {
    int fails = 0;
    #define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++fails; } } while(0)

    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLint vsOk = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &vsOk);
    CHECK(vsOk == GL_TRUE, "VS compile OK");

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    GLint fsOk = 0;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &fsOk);
    CHECK(fsOk == GL_TRUE, "FS compile OK");

    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint linked = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &linked);
    CHECK(linked == GL_TRUE, "program link OK");

    Context& c = Context::Current();
    auto pit = c.programs.find(p);
    if (pit == c.programs.end()) {
        printf("FAIL: program không tìm thấy trong registry\n");
        ++fails;
        return 1;
    }
    ProgramObject& pr = pit->second;

    // iris_Entity phải nằm trong activeAttribs (dùng trong helper ⇒ active).
    bool foundAttrib = false;
    bool forceUnsigned = false;
    int attribLoc = -1;
    for (const auto& aa : pr.activeAttribs) {
        if (aa.name == "iris_Entity") { foundAttrib = true; }
    }
    for (const auto& af : pr.attribFill) {
        // attribFill keyed by location; cần tra ngược tên. Dùng attribLoc.
    }
    // tra attribLoc
    auto lit = pr.attribLoc.find("iris_Entity");
    if (lit != pr.attribLoc.end()) {
        attribLoc = lit->second;
        for (const auto& af : pr.attribFill) {
            if (af.loc == attribLoc) {
                forceUnsigned = (af.cls == 2); // uint class
            }
        }
    }
    CHECK(foundAttrib, "iris_Entity là active attribute (dùng trong helper)");
    CHECK(attribLoc >= 0, "iris_Entity có location sau link");
    CHECK(forceUnsigned, "iris_Entity force unsigned (format UShort4 → uint)");

    // MSL của vertex phải có field iris_Entity trong TGLMT_VIn.
    std::string vmsl = pr.vertexMSL;
    CHECK(vmsl.find("iris_Entity") != std::string::npos, "MSL TGLMT_VIn có field iris_Entity");

    glDeleteProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);

    #undef CHECK
    printf("test_regression_shaderpack_iris_entity: %d fail(s)\n", fails);
    return fails == 0 ? 0 : 1;
}
