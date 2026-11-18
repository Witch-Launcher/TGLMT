// test_regression_shaderpack_helper_overload.cpp — Hồi quy lỗi E-LITE
// `dayBlend`: converter báo `helper dayBlend sai arity (co 3)`.
//
// Kiểm tra: helper có 3 tham số, gọi với 3 tham số ⇒ convert OK. Helper có
// overload cùng tên khác số tham số ⇒ resolve đúng theo số tham số thực tế.
// Không được bỏ kiểm tra arity; chỉ từ chối khi GNS thực sự sai arity.
// Xem latestlog 2.txt:2508.
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cstdio>
#include <string>

using namespace tglmt;

int main() {
    int fails = 0;
    #define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++fails; } } while(0)

    // dayBlend có 3 tham số, gọi với 3 tham số ⇒ phải convert OK.
    std::string okShader = std::string("#version 460 core\n")
        + "vec3 dayBlend(vec3 a, vec3 b, float t) {\n"
        + "    return mix(a, b, t);\n"
        + "}\n"
        + "out vec4 c;\n"
        + "void main() {\n"
        + "    vec3 r = dayBlend(vec3(0), vec3(1), 0.5);\n"
        + "    c = vec4(r, 1.0);\n"
        + "}\n";
    GLSLConvertResult r = ConvertGLSLtoMSL(okShader, 0x8B30);
    CHECK(r.ok, "dayBlend(3 args) convert phải OK");
    if (!r.ok) { printf("  log: %s\n", r.log.c_str()); }

    // dayBlend overload 2 tham số, gọi với 2 ⇒ OK (không nhầm với 3-param version).
    std::string okShader2 = std::string("#version 460 core\n")
        + "vec3 dayBlend(vec3 a, vec3 b) { return mix(a, b, 0.5); }\n"
        + "vec3 dayBlend(vec3 a, vec3 b, float t) { return mix(a, b, t); }\n"
        + "out vec4 c;\n"
        + "void main() {\n"
        + "    vec3 r = dayBlend(vec3(0), vec3(1));\n"
        + "    c = vec4(r, 1.0);\n"
        + "}\n";
    GLSLConvertResult r2 = ConvertGLSLtoMSL(okShader2, 0x8B30);
    CHECK(r2.ok, "dayBlend(2 args) overload convert phải OK");

    // Gọi dayBlend với 1 tham số (không có overload 1-param) ⇒ phải báo lỗi arity
    // rõ ràng (không silent success).
    std::string badShader = std::string("#version 460 core\n")
        + "vec3 dayBlend(vec3 a, vec3 b, float t) { return mix(a, b, t); }\n"
        + "out vec4 c;\n"
        + "void main() {\n"
        + "    vec3 r = dayBlend(vec3(0));\n"
        + "    c = vec4(r, 1.0);\n"
        + "}\n";
    GLSLConvertResult r3 = ConvertGLSLtoMSL(badShader, 0x8B30);
    CHECK(!r3.ok, "dayBlend(1 arg) không có overload ⇒ phải fail");
    if (!r3.ok) {
        CHECK(r3.log.find("arity") != std::string::npos ||
                  r3.log.find("không tìm thấy") != std::string::npos ||
                  r3.log.find("overload") != std::string::npos,
              "log nêu rõ nguyên nhân (arity/overload)");
    }

    #undef CHECK
    printf("test_regression_shaderpack_helper_overload: %d fail(s)\n", fails);
    return fails == 0 ? 0 : 1;
}
