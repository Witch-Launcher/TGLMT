// Regression: transpose của uniform ma trận phải được tôn trọng ở CẢ HAI
// đường:
//   - glUniformMatrix*fv/dv          (GL 4.6 §7.6.1, uniform của program hiện hành)
//   - glProgramUniformMatrix*fv/dv    (GL 4.6 §7.6.2, DSA — truyền program id)
//
// Trước khi sửa: cả 18 hàm glProgramUniformMatrix* bỏ qua tham số `transpose`
// (`(void)t`), còn glUniformMatrix* đã tôn trọng nó. Hậu quả:
//   - ma trận vuông (mat2/mat3/mat4) bị ĐẢO CHIỀU: dữ liệu row-major được
//     đọc như column-major → m[0][1] lấy nhầm m[1][0].
//   - ma trận không vuông (mat2x3, mat3x2, ...) bị đọc sai layout hoàn toàn.
//
// Test đọc lại giá trị đã nạp bằng glGetUniformfv/dv và so với ma trận
// column-major mà GL yêu cầu.

#include "tglmt/gl46.h"
#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

struct Program {
    GLuint id = 0;
    ~Program() {
        if (id) glDeleteProgram(id);
    }
};

// Tạo program có đúng một uniform ma trận `uM` (kiểu mat4 để dễ kiểm tra
// layout), link và trả về id.
GLuint MakeProgramWithMat4() {
    const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* vsrc = "#version 460 core\n"
                       "layout(location = 0) in vec4 aPos;\n"
                       "uniform mat4 uM;\n"
                       "void main() { gl_Position = uM * aPos; }\n";
    const char* fsrc = "#version 460 core\n"
                       "out vec4 oColor;\n"
                       "void main() { oColor = vec4(1.0); }\n";
    glShaderSource(vs, 1, &vsrc, nullptr);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fsrc, nullptr);
    glCompileShader(fs);
    const GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ok ? p : 0;
}

bool Near(float a, float b) { return fabsf(a - b) < 1e-6f; }

// R7: glUniformMatrix4fv với transpose=GL_TRUE phải đảo chiều đúng.
bool TestUniformMatrixTranspose() {
    const GLuint p = MakeProgramWithMat4();
    if (!p) {
        printf("  (skip: không link được program)\n");
        return true;
    }
    const GLint loc = glGetUniformLocation(p, "uM");
    TGLMT_CHECK(loc >= 0, "uM phải tồn tại");

    // Dữ liệu row-major: hàng 0 = {1,2,3,4}, hàng 1 = {5,6,7,8}, ...
    const GLfloat rowMajor[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    // Ma trận column-major tương ứng: cột 0 = {1,5,9,13}, cột 1 = {2,6,10,14}, ...
    const GLfloat expect[16] = {1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};

    glUseProgram(p);
    glUniformMatrix4fv(loc, 1, GL_TRUE, rowMajor);
    GLfloat got[16] = {0};
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 16; ++i) {
        TGLMT_CHECK(Near(got[i], expect[i]),
                    ("glUniformMatrix4fv transpose phải đảo chiều: got[" + std::to_string(i) +
                     "]=" + std::to_string(got[i]) + " expected=" + std::to_string(expect[i]))
                        .c_str());
    }

    // transpose=GL_FALSE thì giữ nguyên thứ tự.
    glUniformMatrix4fv(loc, 1, GL_FALSE, rowMajor);
    memset(got, 0, sizeof(got));
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 16; ++i) {
        TGLMT_CHECK(Near(got[i], rowMajor[i]),
                    "glUniformMatrix4fv transpose=GL_FALSE phải giữ nguyên layout");
    }
    glUseProgram(0);
    return true;
}

// R8: glProgramUniformMatrix4fv (DSA) với transpose=GL_TRUE phải đảo chiều
// giống hệt glUniformMatrix4fv.
bool TestProgramUniformMatrixTranspose() {
    const GLuint p = MakeProgramWithMat4();
    if (!p) {
        printf("  (skip: không link được program)\n");
        return true;
    }
    const GLint loc = glGetUniformLocation(p, "uM");
    TGLMT_CHECK(loc >= 0, "uM phải tồn tại");

    const GLfloat rowMajor[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    const GLfloat expect[16] = {1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16};

    glProgramUniformMatrix4fv(p, loc, 1, GL_TRUE, rowMajor);
    GLfloat got[16] = {0};
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 16; ++i) {
        TGLMT_CHECK(Near(got[i], expect[i]),
                    ("glProgramUniformMatrix4fv transpose phải đảo chiều: got[" +
                     std::to_string(i) + "]=" + std::to_string(got[i]) +
                     " expected=" + std::to_string(expect[i]))
                        .c_str());
    }

    glProgramUniformMatrix4fv(p, loc, 1, GL_FALSE, rowMajor);
    memset(got, 0, sizeof(got));
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 16; ++i) {
        TGLMT_CHECK(Near(got[i], rowMajor[i]),
                    "glProgramUniformMatrix4fv transpose=GL_FALSE phải giữ nguyên layout");
    }
    return true;
}

// R9: Hai đường phải cho kết quả GIỐNG NHAU khi cùng tham số.
bool TestBothPathsAgree() {
    const GLuint pa = MakeProgramWithMat4();
    const GLuint pb = MakeProgramWithMat4();
    if (!pa || !pb) {
        printf("  (skip: không link được program)\n");
        return true;
    }
    const GLint la = glGetUniformLocation(pa, "uM");
    const GLint lb = glGetUniformLocation(pb, "uM");

    const GLfloat src[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    glUseProgram(pa);
    glUniformMatrix4fv(la, 1, GL_TRUE, src);
    glProgramUniformMatrix4fv(pb, lb, 1, GL_TRUE, src);

    GLfloat a[16] = {0}, b[16] = {0};
    glGetUniformfv(pa, la, a);
    glGetUniformfv(pb, lb, b);
    for (int i = 0; i < 16; ++i) {
        TGLMT_CHECK(Near(a[i], b[i]),
                    "glUniformMatrix4fv và glProgramUniformMatrix4fv phải cho cùng kết quả");
    }
    glUseProgram(0);
    return true;
}

// R10: Ma trận KHÔNG VUÔNG (mat2x3) — layout row-major 3 hàng × 2 cột.
// transpose=GL_TRUE, dữ liệu row-major: 3 hàng, mỗi hàng 2 phần tử.
// Column-major mong đợi: 2 cột, mỗi cột 3 phần tử.
bool TestNonSquareTranspose() {
    const GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char* vsrc = "#version 460 core\n"
                       "layout(location = 0) in vec4 aPos;\n"
                       "uniform mat2x3 uM;\n"
                       "void main() { gl_Position = vec4(uM * aPos.xyz, 1.0); }\n";
    const char* fsrc = "#version 460 core\nout vec4 oColor;\nvoid main() { oColor = vec4(1.0); }\n";
    glShaderSource(vs, 1, &vsrc, nullptr);
    glCompileShader(vs);
    glShaderSource(fs, 1, &fsrc, nullptr);
    glCompileShader(fs);
    const GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        printf("  (skip: không link được program mat2x3)\n");
        return true;
    }
    const GLint loc = glGetUniformLocation(p, "uM");
    TGLMT_CHECK(loc >= 0, "uM (mat2x3) phải tồn tại");

    // Row-major: 3 hàng × 2 cột.
    const GLfloat rowMajor[6] = {1, 2, 3, 4, 5, 6};
    // Column-major: 2 cột × 3 hàng → cột0={1,2,3}, cột1={4,5,6}.
    const GLfloat expect[6] = {1, 2, 3, 4, 5, 6};
    // Với mat2x3: cols=2, rows=3. transpose=FALSE → GL đọc column-major
    // (2 cột × 3 hàng) = {1,2,3,4,5,6}. transpose=TRUE → GL đọc row-major
    // (3 hàng × 2 cột) = {1,2,3,4,5,6} rồi hoán đổi thành column-major
    // (2 cột × 3 hàng) = {1,3,5,2,4,6}.
    const GLfloat expectTransposed[6] = {1, 3, 5, 2, 4, 6};

    glProgramUniformMatrix2x3fv(p, loc, 1, GL_TRUE, rowMajor);
    GLfloat got[6] = {0};
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 6; ++i) {
        TGLMT_CHECK(Near(got[i], expectTransposed[i]),
                    ("glProgramUniformMatrix2x3fv transpose: got[" + std::to_string(i) +
                     "]=" + std::to_string(got[i]) +
                     " expected=" + std::to_string(expectTransposed[i]))
                        .c_str());
    }

    glProgramUniformMatrix2x3fv(p, loc, 1, GL_FALSE, rowMajor);
    memset(got, 0, sizeof(got));
    glGetUniformfv(p, loc, got);
    for (int i = 0; i < 6; ++i) {
        TGLMT_CHECK(Near(got[i], expect[i]), "glProgramUniformMatrix2x3fv transpose=FALSE phải giữ layout");
    }
    return true;
}

// R11: count < 0 phải sinh INVALID_VALUE, không abort.
bool TestNegativeCount() {
    const GLuint p = MakeProgramWithMat4();
    if (!p) return true;
    const GLint loc = glGetUniformLocation(p, "uM");
    while (glGetError() != GL_NO_ERROR) {
    }
    const GLfloat dummy[16] = {0};
    glProgramUniformMatrix4fv(p, loc, -1, GL_FALSE, dummy);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glProgramUniformMatrix4fv count<0 phải sinh INVALID_VALUE");
    while (glGetError() != GL_NO_ERROR) {
    }
    glUniformMatrix4fv(loc, -1, GL_FALSE, dummy);
    TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                "glUniformMatrix4fv count<0 phải sinh INVALID_VALUE");
    return true;
}

}  // namespace

int main() {
    printf("=== regression: uniform matrix transpose ===\n");
    // TGLMT_CHECK tăng bộ đếm lỗi toàn cục; mỗi test case được đo bằng
    // chênh lệch của bộ đếm trước/sau để một case fail không che mất case
    // sau (và không được "pass" khi chỉ cần in ra thông báo lỗi).
    struct Case {
        const char* name;
        bool (*fn)();
    };
    const Case kCases[] = {
        {"glUniformMatrix4fv transpose", TestUniformMatrixTranspose},
        {"glProgramUniformMatrix4fv transpose (DSA)", TestProgramUniformMatrixTranspose},
        {"hai đường phải khớp nhau", TestBothPathsAgree},
        {"ma trận không vuông (mat2x3)", TestNonSquareTranspose},
        {"count < 0 phải sinh INVALID_VALUE", TestNegativeCount},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
