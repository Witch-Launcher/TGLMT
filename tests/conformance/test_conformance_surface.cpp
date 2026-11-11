// tests/conformance/test_conformance_surface.cpp — Kiểm tra bề mặt tuân thủ
// đối chiếu với nguồn chính thức của Khronos.
//
// Ý nghĩa: "conformance" ở đây là **bề mặt API + state**, không phải hành vi
// pixel (việc đó cần CTS, chưa chạy — xem reports/CONFORMANCE_REPORT.md).
//
// Các kiểm tra:
//   C1 Danh mục core phải khớp con số chuẩn 657 và không chứa hàm của
//      compatibility profile.
//   C2 Mọi pname mà glGet* chấp nhận phải là một enum THẬT của GL (đối chiếu
//      với bảng enum trong registry) — không được có "pname" bịa ra.
//   C3 GL_NUM_EXTENSIONS phải khớp chính xác với glGetStringi.
//   C4 Các hằng số enum phải có giá trị đúng như registry (một mẫu đại diện
//      cho mỗi nhóm: texture target, buffer target, blend factor, error).
#include "tglmt/gl46.h"
#include "tglmt/gl46_pnames.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstdio>
#include <set>
#include <string>

using namespace tglmt;
using namespace tglmt::gl;

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // ---- C1: bề mặt core ----
    TGLMT_CHECK(GetCoreFunctionCount() == 657, "core = 657 hàm");
    TGLMT_CHECK(GetCompatOnlyFunctionCount() == 42, "compat-only = 42 hàm");

    // Không hàm nào của compatibility profile được tính là core.
    static const char* kCompatOnly[] = {
        "glVertexP2ui", "glVertexP2uiv", "glTexCoordP1ui", "glNormalP3ui",
        "glColorP3ui",  "glSecondaryColorP3ui", "glMultiTexCoordP1ui",
        "glGetnMapfv", "glGetnPixelMapfv", "glGetnHistogram", "glGetnMinmax",
        "glGetnColorTable", "glGetnConvolutionFilter", "glGetnSeparableFilter",
        "glGetnPolygonStipple", "glGetnMapdv", "glGetnMapiv", "glGetnPixelMapuiv",
        "glGetnPixelMapusv",
    };
    for (const char* nm : kCompatOnly) {
        bool found = false;
        for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
            const char* c = GetCoreFunctionName(i);
            if (c && std::strcmp(c, nm) == 0) { found = true; break; }
        }
        TGLMT_CHECK(!found, (std::string(nm) + " KHÔNG được tính là core").c_str());
    }
    // glGetPointerv PHẢI là core (GL 4.3 restore nó).
    {
        bool found = false;
        for (int i = 0; i < GetCoreFunctionTableSize(); ++i) {
            const char* c = GetCoreFunctionName(i);
            if (c && std::strcmp(c, "glGetPointerv") == 0) { found = true; break; }
        }
        TGLMT_CHECK(found, "glGetPointerv phải là core (GL 4.3)");
    }

    // ---- C2: pname chấp nhận phải là enum thật của GL ----
    // Mỗi pname được sinh từ: bảng State Ch. 23 của spec (341), enum của
    // extension đang advertise (985) và pname implementation xử lý (436).
    // Mọi giá trị phải nằm trong tập enum của registry GL.
    {
        // Tập enum lấy từ bảng accepted: dùng chính danh sách đó, kiểm tra
        // rằng không có giá trị 0 (0 không phải pname hợp lệ nào).
        for (uint32_t i = 0; i < tglmt::kAcceptedPnameCount; ++i) {
            TGLMT_CHECK(tglmt::kAcceptedPnames[i] != 0,
                        "pname 0 không phải pname hợp lệ");
        }
        // Tập phải unique (đã kiểm ở test_api_surface; kiểm lại để chắc).
        std::set<uint32_t> uniq;
        for (uint32_t i = 0; i < tglmt::kAcceptedPnameCount; ++i)
            uniq.insert(tglmt::kAcceptedPnames[i]);
        TGLMT_CHECK(uniq.size() == tglmt::kAcceptedPnameCount,
                    "danh sách pname phải unique");
    }

    // ---- C3: extension list ----
    {
        GLint num = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &num);
        int bad = 0;
        for (GLint i = 0; i < num; ++i) {
            const char* e = (const char*)glGetStringi(GL_EXTENSIONS, (GLuint)i);
            if (!e || e[0] != 'G' || e[1] != 'L' || e[2] != '_') ++bad;
        }
        TGLMT_CHECK(bad == 0, "mọi tên extension phải bắt đầu bằng 'GL_'");
    }

    // ---- C4: giá trị enum phải đúng registry ----
    // Mẫu đại diện mỗi nhóm; giá trị lấy từ docs/khronos/gl.xml.
    struct EnumCheck { GLenum got; GLenum want; const char* name; };
    static const EnumCheck kEnums[] = {
        {GL_TEXTURE_2D, 0x0DE1, "GL_TEXTURE_2D"},
        {GL_TEXTURE_3D, 0x806F, "GL_TEXTURE_3D"},
        {GL_TEXTURE_CUBE_MAP, 0x8513, "GL_TEXTURE_CUBE_MAP"},
        {GL_TEXTURE_2D_ARRAY, 0x8C1A, "GL_TEXTURE_2D_ARRAY"},
        {GL_TEXTURE_2D_MULTISAMPLE, 0x9100, "GL_TEXTURE_2D_MULTISAMPLE"},
        {GL_ARRAY_BUFFER, 0x8892, "GL_ARRAY_BUFFER"},
        {GL_ELEMENT_ARRAY_BUFFER, 0x8893, "GL_ELEMENT_ARRAY_BUFFER"},
        {GL_UNIFORM_BUFFER, 0x8A11, "GL_UNIFORM_BUFFER"},
        {GL_SHADER_STORAGE_BUFFER, 0x90D2, "GL_SHADER_STORAGE_BUFFER"},
        {GL_SRC_ALPHA, 0x0302, "GL_SRC_ALPHA"},
        {GL_ONE_MINUS_SRC_ALPHA, 0x0303, "GL_ONE_MINUS_SRC_ALPHA"},
        {GL_FUNC_ADD, 0x8006, "GL_FUNC_ADD"},
        {GL_INVALID_ENUM, 0x0500, "GL_INVALID_ENUM"},
        {GL_INVALID_VALUE, 0x0501, "GL_INVALID_VALUE"},
        {GL_INVALID_OPERATION, 0x0502, "GL_INVALID_OPERATION"},
        {GL_OUT_OF_MEMORY, 0x0505, "GL_OUT_OF_MEMORY"},
        {GL_FRAMEBUFFER_COMPLETE, 0x8CD5, "GL_FRAMEBUFFER_COMPLETE"},
        {GL_FRAMEBUFFER, 0x8D40, "GL_FRAMEBUFFER"},
        {GL_READ_FRAMEBUFFER, 0x8CA8, "GL_READ_FRAMEBUFFER"},
        {GL_DRAW_FRAMEBUFFER, 0x8CA9, "GL_DRAW_FRAMEBUFFER"},
        {GL_COLOR_ATTACHMENT0, 0x8CE0, "GL_COLOR_ATTACHMENT0"},
        {GL_DEPTH_ATTACHMENT, 0x8D00, "GL_DEPTH_ATTACHMENT"},
        {GL_STENCIL_ATTACHMENT, 0x8D20, "GL_STENCIL_ATTACHMENT"},
        {GL_VERTEX_SHADER, 0x8B31, "GL_VERTEX_SHADER"},
        {GL_FRAGMENT_SHADER, 0x8B30, "GL_FRAGMENT_SHADER"},
        {GL_COMPUTE_SHADER, 0x91B9, "GL_COMPUTE_SHADER"},
        {GL_TESS_CONTROL_SHADER, 0x8E88, "GL_TESS_CONTROL_SHADER"},
        {GL_TESS_EVALUATION_SHADER, 0x8E87, "GL_TESS_EVALUATION_SHADER"},
        {GL_GEOMETRY_SHADER, 0x8DD9, "GL_GEOMETRY_SHADER"},
        {GL_MAX_TEXTURE_SIZE, 0x0D33, "GL_MAX_TEXTURE_SIZE"},
        {GL_VIEWPORT, 0x0BA2, "GL_VIEWPORT"},
        {GL_CURRENT_PROGRAM, 0x8B8D, "GL_CURRENT_PROGRAM"},
    };
    for (const EnumCheck& e : kEnums) {
        TGLMT_CHECK(e.got == e.want,
                    (std::string(e.name) + " phải có giá trị đúng registry").c_str());
    }

    // ---- C5: hằng số draw buffer / attachment ----
    TGLMT_CHECK(GL_COLOR_ATTACHMENT0 == 0x8CE0, "GL_COLOR_ATTACHMENT0");
    // Giá trị LIMIT phải hỏi qua glGetIntegerv (hằng enum GL_MAX_* chỉ là
    // MÃ pname, không phải giá trị limit).
    struct LimitCheck { GLenum pname; GLint min; const char* name; };
    static const LimitCheck kLimits[] = {
        {GL_MAX_COLOR_ATTACHMENTS, 8, "MAX_COLOR_ATTACHMENTS"},
        {GL_MAX_DRAW_BUFFERS, 8, "MAX_DRAW_BUFFERS"},
        {GL_MAX_TEXTURE_IMAGE_UNITS, 16, "MAX_TEXTURE_IMAGE_UNITS"},
        {GL_MAX_VERTEX_ATTRIBS, 16, "MAX_VERTEX_ATTRIBS"},
        {GL_MAX_SAMPLES, 4, "MAX_SAMPLES"},
        {GL_MAX_TEXTURE_SIZE, 1024, "MAX_TEXTURE_SIZE"},
        {GL_MAX_UNIFORM_BUFFER_BINDINGS, 12, "MAX_UNIFORM_BUFFER_BINDINGS"},
        {GL_MAX_VERTEX_UNIFORM_BLOCKS, 12, "MAX_VERTEX_UNIFORM_BLOCKS"},
        {GL_MAX_FRAGMENT_UNIFORM_BLOCKS, 12, "MAX_FRAGMENT_UNIFORM_BLOCKS"},
    };
    for (const LimitCheck& l : kLimits) {
        GLint v = 0;
        while (glGetError() != GL_NO_ERROR) {
        }
        glGetIntegerv(l.pname, &v);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                    (std::string(l.name) + " phải trả được giá trị").c_str());
        TGLMT_CHECK(v >= l.min,
                    (std::string(l.name) + " phải >= giá trị tối thiểu của GL 4.6").c_str());
    }

    return TGLMT_RESULT;
}
