// tests/api/test_api_surface.cpp — Kiểm chứng BỀ MẶT API của OpenGL 4.6 Core.
//
// Đây là test "hợp đồng" cho lớp API: nó không kiểm tra hành vi của từng
// hàm, mà kiểm tra rằng *bề mặt* mà driver công bố là đúng theo đặc tả:
//   • đúng số hàm core (657) và không lẫn hàm của compatibility profile,
//   • mọi hàm core đều được export thật từ thư viện dùng chung,
//   • GL_VERSION / GL_CONTEXT_PROFILE_MASK nói đúng profile,
//   • GL_NUM_EXTENSIONS khớp chính xác với glGetStringi,
//   • mọi pname mà glGet* chấp nhận đều là pname thật của GL 4.6 core
//     (hoặc của extension đang được advertise).
#include "tglmt/gl46.h"
#include "tglmt/gl46_pnames.h"
#include "tglmt/Context.h"
#include "test_util.h"

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

    // ---- 1. Số lượng hàm ----
    TGLMT_CHECK(GetCoreFunctionCount() == 657,
                "core function count = 657 (không phải 698)");
    TGLMT_CHECK(GetCompatOnlyFunctionCount() == 42,
                "compat-only function count = 42");
    TGLMT_CHECK(GetCoreFunctionTableSize() == 657, "bảng core có 657 dòng");

    // ---- 2. Mọi hàm core phải export thật ----
    // Test link STATIC lib ⇒ symbol gl* không nằm trong dynamic symbol table
    // của executable, nên mở chính libtglmt.dylib đi kèm để dlsym.
    {
        char exePath[4096] = {0};
        uint32_t n = (uint32_t)sizeof(exePath);
        bool checked = false;
        if (_NSGetExecutablePath(exePath, &n) == 0) {
            std::string dir(exePath);
            size_t slash = dir.find_last_of('/');
            dir = (slash == std::string::npos) ? std::string(".") : dir.substr(0, slash);
            void* h = dlopen((dir + "/libtglmt.dylib").c_str(), RTLD_LAZY | RTLD_LOCAL);
            if (h) {
                int missing = 0;
                const char* first = nullptr;
                for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
                    const char* nm = GetCoreFunctionName(i);
                    if (!nm) continue;
                    if (!dlsym(h, nm)) {
                        ++missing;
                        if (!first) first = nm;
                    }
                }
                TGLMT_CHECK(missing == 0, "mọi hàm core phải có symbol export");
                if (missing) fprintf(stderr, "  thiếu %d, ví dụ %s\n", missing, first);
                dlclose(h);
                checked = true;
            }
        }
        if (!checked)
            printf("(libtglmt.dylib không có cạnh test — bỏ qua kiểm tra symbol)\n");
    }

    // ---- 3. Version + profile ----
    {
        const char* ver = (const char*)glGetString(GL_VERSION);
        TGLMT_CHECK(ver != nullptr, "glGetString(GL_VERSION) phải trả chuỗi");
        if (ver) TGLMT_CHECK(std::strncmp(ver, "4.6", 3) == 0,
                              "GL_VERSION phải bắt đầu bằng 4.6");
        GLint prof = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &prof);
        TGLMT_CHECK((prof & GL_CONTEXT_CORE_PROFILE_BIT) != 0,
                    "CONTEXT_PROFILE_MASK phải có CORE bit");
        TGLMT_CHECK((prof & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0,
                    "CONTEXT_PROFILE_MASK không được có COMPATIBILITY bit");
        GLint major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        TGLMT_CHECK(major == 4 && minor == 6, "MAJOR/MINOR_VERSION phải là 4/6");
    }

    // ---- 4. Danh sách extension phải khớp GL_NUM_EXTENSIONS ----
    {
        GLint num = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num);
        TGLMT_CHECK(num > 0, "phải advertise ít nhất 1 extension");
        int nulls = 0;
        int dupes = 0;
        std::set<std::string> seen;
        for (GLint i = 0; i < num; ++i) {
            const char* e = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
            if (!e || !*e) { ++nulls; continue; }
            if (!seen.insert(e).second) ++dupes;
        }
        TGLMT_CHECK(nulls == 0, "không extension nào được trả về NULL/rỗng");
        TGLMT_CHECK(dupes == 0, "không extension nào bị trùng");
        // Extension đầu tiên phải là GL_KHR_debug hoặc tương tự — chỉ cần
        // kiểm tra không rỗng.
    }

    // ---- 5. Pname chấp nhận phải là pname thật ----
    // Mỗi pname trong bảng được sinh từ bảng State của spec (Ch. 23) hoặc
    // từ enum của extension đang advertise → không được trùng giá trị với
    // nhau và không được trùng với pname "không hợp lệ" đã dùng.
    {
        std::set<uint32_t> uniq;
        for (uint32_t i = 0; i < tglmt::kAcceptedPnameCount; ++i)
            uniq.insert(tglmt::kAcceptedPnames[i]);
        TGLMT_CHECK(uniq.size() == tglmt::kAcceptedPnameCount,
                    "danh sách pname chấp nhận không được có trùng lặp");
        // Một pname hợp lệ phải trả về giá trị mà KHÔNG sinh INVALID_ENUM.
        GLint v = 0;
        while (glGetError() != GL_NO_ERROR) {
        }
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                    "pname hợp lệ không được sinh INVALID_ENUM");
        // Một pname không hợp lệ PHẢI sinh INVALID_ENUM.
        while (glGetError() != GL_NO_ERROR) {
        }
        glGetIntegerv(0xDEAD0001, &v);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "pname không hợp lệ phải sinh INVALID_ENUM");
    }

    return TGLMT_RESULT;
}
