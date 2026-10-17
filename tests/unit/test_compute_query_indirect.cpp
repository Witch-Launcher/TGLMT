// test_compute_query_indirect.cpp — P1: GL_COMPUTE_WORK_GROUP_SIZE (0x8267) +
// validation glDispatchComputeIndirect (GL 4.3 §7.12.1).
// Chạy trên MỌI backend (Null cũng có converter + program store) — không cần
// dispatch thật, chỉ kiểm tra query + error paths đúng spec.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

namespace {
GLuint CompileLinkCompute(Context& c, const char* src) {
    GLuint sh = glCreateShader(0x91B9); // COMPUTE_SHADER
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, 0x8B81, &ok);
    if (!ok) {
        char l[512]; GLsizei n = 0;
        glGetShaderInfoLog(sh, sizeof(l), &n, l);
        printf("  compile fail: %.200s\n", l);
        return 0;
    }
    GLuint pr = glCreateProgram();
    glAttachShader(pr, sh);
    glLinkProgram(pr);
    glGetProgramiv(pr, 0x8B82, &ok);
    if (!ok) { printf("  link fail\n"); return 0; }
    return pr;
}
} // namespace

int main() {
    Context apple("apple"); // Null fallback nếu không có MTLDevice
    Context::MakeCurrent(&apple);
    int fails = 0;
#define CHECK(cond, msg)                                                        \
    do {                                                                        \
        if (!(cond)) { printf("FAIL: %s\n", msg); ++fails; }                    \
    } while (0)

    // --- 1. COMPUTE_WORK_GROUP_SIZE trả layout(local_size_*) của shader ---
    {
        const char* src = "#version 460 core\n"
            "layout(local_size_x=8, local_size_y=4, local_size_z=2) in;\n"
            "layout(std430,binding=0) buffer B{uint v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=1u; }\n";
        GLuint pr = CompileLinkCompute(apple, src);
        CHECK(pr != 0, "compute program #1 link");
        GLint sz[3] = {-1, -1, -1};
        glGetProgramiv(pr, 0x8267, sz);
        if (sz[0] != 8 || sz[1] != 4 || sz[2] != 2) {
            printf("FAIL: work group size = {%d,%d,%d}, muốn {8,4,2}\n",
                   sz[0], sz[1], sz[2]);
            ++fails;
        } else {
            printf("OK: COMPUTE_WORK_GROUP_SIZE = {8,4,2}\n");
        }
    }
    // --- 2. Shader không có layout → GLSL mặc định 1,1,1 ---
    {
        const char* src = "#version 460 core\nvoid main(){ }\n";
        GLuint pr = CompileLinkCompute(apple, src);
        CHECK(pr != 0, "compute program #2 link");
        GLint sz[3] = {-1, -1, -1};
        glGetProgramiv(pr, 0x8267, sz);
        CHECK(sz[0] == 1 && sz[1] == 1 && sz[2] == 1,
              "default work group size phải là {1,1,1}");
        if (sz[0] != 1 || sz[1] != 1 || sz[2] != 1)
            printf("  nhận {%d,%d,%d}\n", sz[0], sz[1], sz[2]);
    }
    // --- 3. Program KHÔNG compute → {0,0,0} (query vô nghĩa ngoài CS) ---
    {
        const char* vsrc = "#version 460 core\nvoid main(){ gl_Position=vec4(0); }\n";
        const char* fsrc = "#version 460 core\nout vec4 c;\nvoid main(){ c=vec4(1); }\n";
        GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
        glShaderSource(vs, 1, &vsrc, nullptr); glCompileShader(vs);
        glShaderSource(fs, 1, &fsrc, nullptr); glCompileShader(fs);
        GLuint pr = glCreateProgram();
        glAttachShader(pr, vs); glAttachShader(pr, fs);
        glLinkProgram(pr);
        GLint ok = 0; glGetProgramiv(pr, 0x8B82, &ok);
        CHECK(ok == 1, "graphics program link");
        GLint sz[3] = {-1, -1, -1};
        glGetProgramiv(pr, 0x8267, sz);
        CHECK(sz[0] == 0 && sz[1] == 0 && sz[2] == 0,
              "program không compute → work group size {0,0,0}");
    }

    // --- 4. glDispatchComputeIndirect error paths (spec §7.12.1) ---
    {
        const char* src = "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430,binding=0) buffer B{uint v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=1u; }\n";
        GLuint pr = CompileLinkCompute(apple, src);
        glUseProgram(pr);
        while (glGetError() != 0) {} // drain

        // 4a. không buffer nào bound vào DISPATCH_INDIRECT_BUFFER → INVALID_OPERATION
        glDispatchComputeIndirect(0);
        CHECK(glGetError() == 0x0502, "indirect: không buffer → INVALID_OPERATION");

        // 4b. offset âm / lệch 4 → INVALID_VALUE
        GLuint ib; glGenBuffers(1, &ib);
        glBindBuffer(0x90EE, ib);
        glBufferData(0x90EE, 12, nullptr, 0x88E4);
        glDispatchComputeIndirect(-4);
        CHECK(glGetError() == 0x0501, "indirect: offset âm → INVALID_VALUE");
        glDispatchComputeIndirect(2);
        CHECK(glGetError() == 0x0501, "indirect: offset lệch 4 → INVALID_VALUE");

        // 4c. buffer quá nhỏ cho 12 byte từ offset → INVALID_OPERATION
        glDispatchComputeIndirect(4); // 4+12 > 12
        CHECK(glGetError() == 0x0502, "indirect: vượt end buffer → INVALID_OPERATION");

        // 4d. hợp lệ → không lỗi (Null: chỉ log, Apple: chưa execute nhưng đã encode)
        glDispatchComputeIndirect(0);
        GLenum e = glGetError();
        CHECK(e == 0, "indirect hợp lệ không được sinh lỗi");
        if (e) printf("  glGetError=0x%x\n", e);

        // 4e. glDispatchCompute(0,1,1) = no work, không lỗi, không fault Metal
        glDispatchCompute(0, 1, 1);
        CHECK(glGetError() == 0, "dispatch 0 group không lỗi");
        glFinish(); // xả hết trước khi context huỷ
    }

    printf("test_compute_query_indirect: %s (%d fails)\n",
           fails ? "FAIL" : "OK", fails);
    return fails ? 1 : 0;
#undef CHECK
}
