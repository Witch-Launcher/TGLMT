// test_regression_catalog.cpp — Bảo vệ danh mục OpenGL 4.6 Core Profile.
//
// Sự cố gốc: dự án từng công bố "698 hàm OpenGL 4.6 Core" trong khi con số
// đúng theo Khronos registry là **657**. Sai lệch do hai lỗi trong cách suy
// ra danh mục từ `docs/khronos/gl.xml`:
//   B1 bỏ qua `profile="compatibility"` trên `<require>` ⇒ 42 hàm CHỈ có ở
//      compatibility profile bị tính là core;
//   B2 áp `<remove>` SAU tất cả `<require>` ⇒ `glGetPointerv` (bị gỡ ở core
//      3.2 rồi require lại ở 4.3) bị loại khỏi core.
//
// Test này khóa lại con số đúng VÀ kiểm tra mọi hàm core đều được export.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <set>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // Con số chuẩn theo Khronos registry (tools/gl46_catalog.py).
    TGLMT_CHECK(GetCoreFunctionCount() == 657,
                "core function count phải là 657 (không phải 698)");
    TGLMT_CHECK(GetCompatOnlyFunctionCount() == 42,
                "compat-only function count phải là 42");
    TGLMT_CHECK(GetCoreFunctionTableSize() == 657,
                "bảng tên hàm core phải có 657 dòng");

    // glGetPointerv là core từ GL 4.3 (gl.xml: GL_VERSION_4_3 require lại
    // sau khi GL_VERSION_3_2 remove profile="core"). Trước đây bị loại khỏi
    // danh mục dù đã được implement + export.
    {
        bool found = false;
        for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
            const char* nm = GetCoreFunctionName(i);
            if (nm && std::strcmp(nm, "glGetPointerv") == 0) { found = true; break; }
        }
        TGLMT_CHECK(found, "glGetPointerv phải có trong danh mục core");
    }

    // Mọi hàm core phải có symbol export thật trong thư viện dùng chung.
    //
    // Test này link STATIC lib nên symbol gl* nằm trong executable nhưng KHÔNG
    // nằm trong dynamic symbol table (không có -rdynamic) ⇒ dlsym(RTLD_DEFAULT)
    // không thấy. Thay vào đó mở chính libtglmt.dylib đi kèm (nếu có) và
    // dlsym từng tên; nếu không có dylib thì bỏ qua kiểm tra này (toàn bộ
    // kiểm tra symbol đầy đủ do tools/truth_manifest.py --strict đảm nhiệm).
    {
        char exePath[4096] = {0};
        uint32_t n = (uint32_t)sizeof(exePath);
        if (_NSGetExecutablePath(exePath, &n) == 0) {
            std::string dir(exePath);
            size_t slash = dir.find_last_of('/');
            dir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);
            std::string libPath = dir + "/libtglmt.dylib";
            void* h = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (h) {
                int missing = 0;
                const char* firstMissing = nullptr;
                for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
                    const char* nm = GetCoreFunctionName(i);
                    if (!nm) continue;
                    if (!dlsym(h, nm)) {
                        ++missing;
                        if (!firstMissing) firstMissing = nm;
                    }
                }
                TGLMT_CHECK(missing == 0, "mọi hàm core phải có symbol export");
                if (missing)
                    fprintf(stderr, "  (thiếu %d, ví dụ %s)\n", missing,
                            firstMissing ? firstMissing : "?");
                dlclose(h);
            } else {
                printf("(libtglmt.dylib không có cạnh test — bỏ qua kiểm tra symbol)\n");
            }
        }
    }

    // Tên hàm trong bảng phải unique.
    {
        std::set<std::string> seen;
        int dup = 0;
        for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
            const char* nm = GetCoreFunctionName(i);
            if (!nm) continue;
            if (!seen.insert(nm).second) ++dup;
        }
        TGLMT_CHECK(dup == 0, "tên hàm core phải unique");
    }

    // glGetString(GL_VERSION) phải nói đúng core profile.
    const char* ver = (const char*)glGetString(GL_VERSION);
    TGLMT_CHECK(ver != nullptr, "glGetString(GL_VERSION) phải trả chuỗi");
    if (ver) {
        TGLMT_CHECK(std::string(ver).find("4.6") != std::string::npos,
                    "GL_VERSION phải chứa 4.6");
        TGLMT_CHECK(std::string(ver).compare(0, 3, "4.6") == 0,
                    "GL_VERSION phải bắt đầu bằng 4.6");
    }

    // CONTEXT_PROFILE_MASK phải có CORE_PROFILE_BIT (không có
    // COMPATIBILITY_PROFILE_BIT) vì đây là backend OpenGL 4.6 CORE.
    GLint prof = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &prof);
    TGLMT_CHECK((prof & GL_CONTEXT_CORE_PROFILE_BIT) != 0,
                "CONTEXT_PROFILE_MASK phải có CORE bit");
    TGLMT_CHECK((prof & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0,
                "CONTEXT_PROFILE_MASK không được có COMPATIBILITY bit");

    return TGLMT_RESULT;
}
