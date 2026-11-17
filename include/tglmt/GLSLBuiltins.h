// GLSLBuiltins.h — kiến trúc cho các pass dịch built-in GLSL.
//
// VÌ SAO:
//   Converter GLSL→MSL nằm gọn trong MỘT file (src/gl/glsl_to_msl.cpp, ~12k
//   dòng). Nếu nhiều người (agent) sửa cùng file ⇒ xung đột. Vì vậy mỗi
//   nhóm built-in nằm ở file RIÊNG (src/gl/glsl_builtins_<group>.cpp) và
//   **tự đăng ký** vào registry (static initializer) ⇒ không ai phải sửa
//   danh sách chung.
//
// CÁCH DÙNG (một file built-in mới):
//   #include "tglmt/GLSLBuiltins.h"
//   #include "tglmt/GLSLTextUtil.h"
//   using namespace tglmt;
//   namespace tglmt::gl {
//   static void ApplyGLSLBuiltinXXX(GLSLBuiltinCtx& ctx) { ... }
//   static const bool kRegXXX =
//       RegisterGLSLBuiltinPass("xxx", ApplyGLSLBuiltinXXX);
//   }  // namespace tglmt::gl
//
// Ràng buộc hành vi: pass phải **giữ nguyên** MSL sinh ra cho các shader
// không liên quan (chỉ sửa đúng phần liên quan tới nhóm của mình).
#ifndef TGLMT_GLSL_BUILTINS_H
#define TGLMT_GLSL_BUILTINS_H

#include "tglmt/gl46.h"
#include "tglmt/GLSLConverter.h"

#include <string>
#include <utility>
#include <vector>

namespace tglmt::gl {

// Ngữ cảnh mà pass built-in được phép đụng tới.
struct GLSLBuiltinCtx {
    // Thân hàm main (MSL sau khi các pass trước đã chạy). Pass được phép
    // sửa tại chỗ (thường là viết lại một lời gọi built-in thành lời gọi
    // MSL tương đương).
    std::string* body = nullptr;
    // Phần helper function đã tách riêng (nếu pass cần thêm helper MSL thì
    // sinh vào đây).
    std::string* helpers = nullptr;
    // Khối MSL thêm (struct/helper/biến) — nối vào prelude của MSL cuối.
    std::string* mslExtra = nullptr;
    // Bảng đổi định danh (tên GLSL → tên MSL) để pass đăng ký được alias
    // (ví dụ `modf` → `tglmt_modf`). Ưu tiên dùng cơ chế này thay vì sửa
    // thẳng `body`, để giữ nhất quán với phần còn lại của converter.
    std::vector<std::pair<std::string, std::string>>* mp = nullptr;
    // Danh sách khai báo input/output/uniform của shader hiện tại — pass có
    // thể THÊM biến (ví dụ varying `gl_Layer`) để converter emit vào struct
    // MSL. Biến thêm phải có `location` tạm (900+) — linker sẽ gán lại số
    // thật sau (xem src/gl/gl_program.cpp RewriteLocns).
    std::vector<GLSLVar>* inputs = nullptr;
    std::vector<GLSLVar>* outputs = nullptr;
    std::vector<GLSLVar>* uniforms = nullptr;
    // Cấp location tạm tiếp theo cho biến mới (dùng chung cho cả input và
    // output — linker khớp theo TÊN nên không cần trùng số).
    int* nextTempLocation = nullptr;

    // Stage hiện tại.
    bool isVertex = false;
    bool isFragment = false;
    bool isCompute = false;
    bool isTessControl = false;
    bool isTessEval = false;
    bool isGeometry = false;
};

using GLSLBuiltinPassFn = void (*)(GLSLBuiltinCtx&);

// ---- Pass ở MỨC NGUỒN (chạy TRƯỚC khi converter parse) ------------------
// Một số tính năng GLSL bị parser của converter TỪ CHỐI (vd `uniform S u[2]`
// với S là struct, `invariant gl_Position;`, SSBO unsized-array) ⇒ không thể
// sửa sau khi parse. Pass mức nguồn chuẩn hoá nguồn GLSL TRƯỚC khi parse.
//
// Hàm phải LUÔN trả về nguồn HỢP LỆ: nếu không chắc thì trả nguyên đầu vào
// (⇒ converter báo lỗi rõ ràng như trước, không sinh MSL sai).
using GLSLSourcePassFn = std::string (*)(const std::string&);

bool RegisterGLSLSourcePass(const char* id, GLSLSourcePassFn fn);
std::string RunGLSLSourcePasses(const std::string& src);
std::vector<std::string> GLSLSourcePassIds();

// Đăng ký một pass. Gọi từ static initializer trong file built-in tương ứng.
// Trả true để dùng làm biến static.
bool RegisterGLSLBuiltinPass(const char* id, GLSLBuiltinPassFn fn);

// Chạy TẤT CẢ pass đã đăng ký, theo thứ tự đăng ký.
void RunGLSLBuiltinPasses(GLSLBuiltinCtx& ctx);

// Danh sách pass đã đăng ký (để test/log).
std::vector<std::string> GLSLBuiltinPassIds();

}  // namespace tglmt::gl

#endif  // TGLMT_GLSL_BUILTINS_H
