// test_preprocessor.cpp — Unit test cho preprocessor GLSL (src/compiler/Preprocessor.cpp).
// Mục tiêu: xác minh ý nghĩa chuẩn của #if/#elif/#ifdef/#ifndef/#else/#endif,
// macro (object + function-like), defined, nesting, #version/#extension/#line,
// source location diagnostic. Chạy backend Null (không cần Metal).
// TDD: viết test trước, sau implement cho pass.
#include "tglmt/compiler/Preprocessor.h"
#include "test_util.h"

#include <cstdio>
#include <string>

using namespace tglmt::compiler;

static int gFails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); ++gFails; } } while(0)

static PreprocessResult RunPP(const std::string& src, const std::string& file) {
    Preprocessor pp(src, file);
    return pp.run();
}

int main() {
    // 1) #if 1 / #if 0
    {
        auto r = RunPP("#if 1\nint a = 1;\n#endif\n#if 0\nint b = 2;\n#endif\n", "t.glsl");
        CHECK(r.success, "#if 1/#if 0 thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "#if 1 giữ nội dung");
        CHECK(r.source.find("int b = 2;") == std::string::npos, "#if 0 bỏ nội dung");
    }

    // 2) #if defined(X)
    {
        auto r = RunPP("#define FOO 1\n#if defined(FOO)\nint a = 1;\n#endif\n", "t.glsl");
        CHECK(r.success, "defined(FOO) thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "defined(FOO) giữ nội dung");

        auto r2 = RunPP("#if defined(BAR)\nint b = 2;\n#endif\n", "t.glsl");
        CHECK(r2.success, "defined(BAR) không định nghĩa thành công");
        CHECK(r2.source.find("int b = 2;") == std::string::npos, "defined(BAR) bỏ nội dung");
    }

    // 3) #ifdef / #ifndef
    {
        auto r = RunPP("#define X\n#ifdef X\nint a = 1;\n#endif\n#ifndef X\nint b = 2;\n#endif\n", "t.glsl");
        CHECK(r.success, "#ifdef/#ifndef thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "#ifdef X giữ");
        CHECK(r.source.find("int b = 2;") == std::string::npos, "#ifndef X bỏ");
    }

    // 4) #else
    {
        auto r = RunPP("#if 0\nint a = 1;\n#else\nint b = 2;\n#endif\n", "t.glsl");
        CHECK(r.success, "#else thành công");
        CHECK(r.source.find("int a = 1;") == std::string::npos, "#if 0 bỏ nhánh đầu");
        CHECK(r.source.find("int b = 2;") != std::string::npos, "#else giữ nhánh sau");
    }

    // 5) #elif
    {
        auto r = RunPP("#if 0\nint a = 1;\n#elif 1\nint b = 2;\n#elif 0\nint c = 3;\n#else\nint d = 4;\n#endif\n", "t.glsl");
        CHECK(r.success, "#elif thành công");
        CHECK(r.source.find("int a = 1;") == std::string::npos, "#elif bỏ nhánh 0");
        CHECK(r.source.find("int b = 2;") != std::string::npos, "#elif 1 giữ");
        CHECK(r.source.find("int c = 3;") == std::string::npos, "#elif 0 bỏ");
        CHECK(r.source.find("int d = 4;") == std::string::npos, "#else bỏ sau #elif đã match");
    }

    // 6) nesting
    {
        auto r = RunPP("#if 1\n#if 1\nint a = 1;\n#endif\n#endif\n", "t.glsl");
        CHECK(r.success, "nesting #if thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "nesting giữ nội dung");
    }

    // 7) #if biểu thức nguyên số: >, <, ==, !=, &&, ||, !
    {
        auto r = RunPP("#if 3 > 2\nint a = 1;\n#endif\n#if !(1 == 2)\nint b = 2;\n#endif\n#if 1 && 1\nint c = 3;\n#endif\n#if 1 || 0\nint d = 4;\n#endif\n", "t.glsl");
        CHECK(r.success, "#if biểu thức thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "3 > 2 đúng");
        CHECK(r.source.find("int b = 2;") != std::string::npos, "!(1 == 2) đúng");
        CHECK(r.source.find("int c = 3;") != std::string::npos, "1 && 1 đúng");
        CHECK(r.source.find("int d = 4;") != std::string::npos, "1 || 0 đúng");
    }

    // 8) #if số hex
    {
        auto r = RunPP("#if 0xFF == 255\nint a = 1;\n#endif\n", "t.glsl");
        CHECK(r.success, "#if hex thành công");
        CHECK(r.source.find("int a = 1;") != std::string::npos, "0xFF == 255 đúng");
    }

    // 9) #version
    {
        auto r = RunPP("#version 460 core\nint a = 1;\n", "t.glsl");
        CHECK(r.success, "#version thành công");
        CHECK(r.version.major == 4 && r.version.minor == 60, "#version 460 parse đúng");
        CHECK(r.profile == GlslProfile::Core, "#version profile core");
    }

    // 10) #extension
    {
        auto r = RunPP("#extension GL_ARB_texture_storage : enable\nint a = 1;\n", "t.glsl");
        CHECK(r.success, "#extension thành công");
        CHECK(r.extensions.enabled.count("GL_ARB_texture_storage") > 0, "#extension enable");
        CHECK(r.extensions.enabled["GL_ARB_texture_storage"] == true, "giá trị enable true");
    }

    // 11) #line
    {
        auto r = RunPP("#line 10 \"foo.glsl\"\nint a = 1;\n", "t.glsl");
        CHECK(r.success, "#line thành công");
    }

    // 12) object-like macro (tạm thời): #define FOO 1 sau này expand
    {
        auto r = RunPP("#define FOO 1\nint a = FOO;\n", "t.glsl");
        CHECK(r.success, "#define object-like không crash");
    }

    // 13) #endif thiếu → lỗi
    {
        auto r = RunPP("#if 1\nint a = 1;\n", "t.glsl");
        CHECK(!r.success, "#endif thiếu phải fail");
        bool hasPP001 = false;
        for (auto& d : r.diagnostics) if (d.code == "PP-001") hasPP001 = true;
        CHECK(hasPP001, "báo lỗi PP-001 (#if thiếu #endif)");
    }

    // 14) directive không hỗ trợ → lỗi
    {
        auto r = RunPP("#include <foo.glsl>\n", "t.glsl");
        CHECK(!r.success, "#include không hỗ trợ phải fail");
    }

    printf("test_preprocessor: %d fail(s)\n", gFails);
    return gFails == 0 ? 0 : 1;
}
