// test_explicit_uniform_loc.cpp — GL_ARB_explicit_uniform_location (core 4.3+).
// Spec (ARB_explicit_uniform_location.txt, §2.14.7):
//  • uniform khai báo layout(location=N) giữ đúng location N;
//  • location tự sinh KHÔNG được chiếm location của uniform explicit;
//  • location phải trong [0, MAX_UNIFORM_LOCATIONS-1].
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_explicit_uniform_loc FAIL: %s\n", msg); ++gFails; }
}

static GLuint MakeProgram() {
    // uExplicit location=7, uAuto không có layout → phải tránh location 7.
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec4 aPos;\n"
        "layout(location = 7) uniform vec4 uExplicit;\n"
        "uniform vec4 uAuto;\n"
        "out vec4 vCol;\n"
        "void main() {\n"
        "  vCol = uExplicit + uAuto;\n"
        "  gl_Position = aPos;\n"
        "}\n";
    const char* fs =
        "#version 460 core\n"
        "layout(location = 7) uniform vec4 uExplicit;\n"
        "in vec4 vCol;\n"
        "out vec4 o;\n"
        "void main() { o = vCol; }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(v, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(v, sizeof(log) - 1, nullptr, log);
        printf("test_explicit_uniform_loc: VS compile fail:\n%s\n", log);
        return 0;
    }
    glGetShaderiv(f, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(f, sizeof(log) - 1, nullptr, log);
        printf("test_explicit_uniform_loc: FS compile fail:\n%s\n", log);
        return 0;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetProgramInfoLog(p, sizeof(log) - 1, nullptr, log);
        printf("test_explicit_uniform_loc: link fail:\n%s\n", log);
        return 0;
    }
    return p;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    GLuint p = MakeProgram();
    if (!p) { printf("test_explicit_uniform_loc: setup failed\n"); return 1; }

    // Location explicit phải đúng 7 (không phải 0/1 gán tuần tự).
    GLint locExp = glGetUniformLocation(p, "uExplicit");
    Check(locExp == 7, "explicit location honoured (got uExplicit)");

    // Location tự sinh phải KHÁC 7 và hợp lệ.
    GLint locAuto = glGetUniformLocation(p, "uAuto");
    Check(locAuto >= 0, "auto uniform has location");
    Check(locAuto != 7, "auto location does not collide with explicit");

    // Tên không tồn tại → -1.
    Check(glGetUniformLocation(p, "uNope") == -1, "unknown uniform -> -1");

    // Location phải nằm trong [0, MAX_UNIFORM_LOCATIONS).
    GLint maxLoc = 0;
    glGetIntegerv(0x826E /*MAX_UNIFORM_LOCATIONS*/, &maxLoc);
    Check(maxLoc == 1024, "MAX_UNIFORM_LOCATIONS = 1024");
    Check(locExp < maxLoc && locAuto < maxLoc, "locations within range");

    // Ghi uniform theo location explicit → phải không lỗi, và location này
    // đúng là location mà app dùng (glUniform4fv(7, ...)).
    glUseProgram(p);
    while (glGetError() != GL_NO_ERROR) {}
    glUniform4fv(locExp, 1, (const GLfloat[]){1, 2, 3, 4});
    Check(glGetError() == GL_NO_ERROR, "glUniform4fv at explicit location");

    // Round-trip: query lại vẫn trả 7 (location ổn định sau link).
    Check(glGetUniformLocation(p, "uExplicit") == 7, "explicit location stable");

    // GetProgramResourceLocation phải khớp GetUniformLocation.
    GLint rloc = glGetProgramResourceLocation(p, 0x92E1 /*GL_UNIFORM*/, "uExplicit");
    Check(rloc == 7, "GetProgramResourceLocation matches");

    // --- Uniform KHÔNG có layout(location=) nhưng khai ở CẢ VS và FS ---
    // Đây là MỘT uniform → phải cùng location ở cả hai stage. Nếu VS và FS
    // lệch location, glUniform*(loc) chỉ tới được 1 stage → stage kia đọc 0
    // (rất dễ xảy ra: mvp/proj dùng chung giữa VS và FS).
    {
        const char* vs2 =
            "#version 460 core\n"
            "layout(location = 0) in vec4 aPos;\n"
            "uniform vec4 uShared;\n"
            "out vec4 vCol;\n"
            "void main() { vCol = uShared; gl_Position = aPos; }\n";
        const char* fs2 =
            "#version 460 core\n"
            "uniform vec4 uShared;\n"
            "in vec4 vCol;\n"
            "out vec4 o;\n"
            "void main() { o = vCol + uShared; }\n";
        GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
        glShaderSource(v, 1, &vs2, nullptr);
        glShaderSource(f, 1, &fs2, nullptr);
        glCompileShader(v);
        glCompileShader(f);
        GLint ok2 = 0;
        glGetShaderiv(v, 0x8B81, &ok2);
        Check(ok2 == 1, "VS shared-uniform compile");
        glGetShaderiv(f, 0x8B81, &ok2);
        Check(ok2 == 1, "FS shared-uniform compile");
        GLuint p2 = glCreateProgram();
        glAttachShader(p2, v);
        glAttachShader(p2, f);
        glLinkProgram(p2);
        glGetProgramiv(p2, 0x8B82, &ok2);
        if (!ok2) {
            char log[2048] = {0};
            glGetProgramInfoLog(p2, sizeof(log) - 1, nullptr, log);
            printf("test_explicit_uniform_loc: link fail:\n%s\n", log);
        }
        Check(ok2 == 1, "link shared-uniform program");
        // Kiểm tra state NỘI BỘ trước khi gọi glGetUniformLocation: hàm đó có
        // side-effect ghi đè e.loc của entry ĐẦU TIÊN → che mất lỗi lệch
        // location giữa VS và FS.
        {
            auto it = ctx.programs.find(p2);
            int nVS = 0, nFS = 0;
            GLint locVS = -1, locFS = -1;
            if (it != ctx.programs.end()) {
                for (const auto& e : it->second.uniformLayout) {
                    if (e.name != "uShared") continue;
                    if (e.isVS) { ++nVS; locVS = e.loc; }
                    else { ++nFS; locFS = e.loc; }
                }
            }
            Check(nVS == 1 && nFS == 1, "uniform có mặt ở cả VS và FS");
            Check(locVS >= 0 && locVS == locFS, "VS và FS dùng CHUNG location");
        }
        GLint sl = glGetUniformLocation(p2, "uShared");
        Check(sl >= 0, "shared uniform có location");
        // Location phải ổn định khi query lại (VS/FS cùng location).
        Check(glGetUniformLocation(p2, "uShared") == sl, "shared uniform location ổn định");
        // Ghi uniform không được sinh lỗi.
        glUseProgram(p2);
        while (glGetError() != GL_NO_ERROR) {}
        glUniform4fv(sl, 1, (const GLfloat[]){1, 2, 3, 4});
        Check(glGetError() == GL_NO_ERROR, "glUniform4fv tại shared uniform location");
    }

    if (gFails == 0) { printf("test_explicit_uniform_loc: OK\n"); return 0; }
    return 1;
}
