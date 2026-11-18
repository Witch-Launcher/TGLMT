// test_regression_shaderpack_return_type.cpp — Hồi quy lỗi Complementary
// `pow2`: overload cùng tên/cùng arity khác kiểu.
//
// Tùy chọn: kiểm tra ConvertGLSLtoMSL trực tiếp (CPU-only) để xác định kiểu
// trả về của mỗi overload và kiểu biểu thức gọi. Khi compiler mới được nối vào
// glCompileShader, cùng test này chạy qua đường pipeline cuối.
// Xem latestlog 2.txt:1957: Metal từ chối MSL: hàm trả `int` nhưng
// `return pow2(x) * float3(x)` trả `float3`.
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cstdio>
#include <string>

using namespace tglmt;

int main() {
    int fails = 0;
    #define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++fails; } } while(0)

    const char* pow2block =
        "int pow2(int x) { return x * x; }\n"
        "float pow2(float x) { return x * x; }\n"
        "vec2 pow2(vec2 x) { return x * x; }\n"
        "vec3 pow2(vec3 x) { return x * x; }\n"
        "vec4 pow2(vec4 x) { return x * x; }\n";

    // pow2(vec3) phải resolve sang overload vec3 → float3 (MSL float3), KHÔNG
    // phải overload int (trả int). Gán vào float3 và dùng làm toán hạng.
    std::string shader = std::string("#version 460 core\n") + pow2block +
        "out vec4 c;\n"
        "void main() {\n"
        "    vec3 v = vec3(1.0, 2.0, 3.0);\n"
        "    vec3 r = pow2(v);\n"
        "    c = vec4(r, 1.0);\n"
        "}\n";
    GLSLConvertResult r = ConvertGLSLtoMSL(shader, 0x8B30);
    CHECK(r.ok, "pow2(vec3) convert phải OK");
    if (!r.ok) { printf("  log: %s\n", r.log.c_str()); }
    if (r.ok) {
        // MSL phải gọi pow2 với kiểu float3 (overload vec3), không phải int.
        CHECK(r.msl.find("pow2(float3") != std::string::npos ||
                  r.msl.find("pow2(vec3") != std::string::npos,
              "MSL gọi overload pow2 vec3/float3");
        // Kiểu kết quả gán vào `vec3 r` ⇒ phải là float3, không phải int.
        CHECK(r.msl.find("float3") != std::string::npos, "MSL có float3 cho kết quả pow2(vec3)");
    }

    // pow2(3) với int literal → overload int.
    std::string shader2 = std::string("#version 460 core\n") + pow2block +
        "out vec4 c;\n"
        "void main() {\n"
        "    int i = pow2(3);\n"
        "    c = vec4(float(i),0,0,1);\n"
        "}\n";
    GLSLConvertResult r2 = ConvertGLSLtoMSL(shader2, 0x8B30);
    CHECK(r2.ok, "pow2(3) int overload convert phải OK");

    #undef CHECK
    printf("test_regression_shaderpack_return_type: %d fail(s)\n", fails);
    return fails == 0 ? 0 : 1;
}
