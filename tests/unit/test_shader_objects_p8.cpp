// test_shader_objects_p8.cpp — P8: ARB_shader_objects (forwards ARB→core).
// Verify ARB API hoạt động giống core (create/attach/delete/info-log/parameter).
// Chạy cả 2 backend.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace tglmt { namespace gl {
// ARB_shader_objects base impls (suffix versions do generator sinh alias).
GLuint glCreateShaderObjectARB(GLenum type);
GLuint glCreateProgramObjectARB();
void glAttachObjectARB(GLuint c, GLuint o);
void glDeleteObjectARB(GLuint o);
void glGetInfoLogARB(GLuint o, GLsizei n, GLsizei* l, GLchar* log);
void glGetObjectParameterivARB(GLuint o, GLenum p, GLint* v);
void glUseProgramObjectARB(GLuint p);
} }

using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_shader_objects_p8 FAIL: %s\n", msg); ++gFails; }
}

int main() {
    // Create shader/program qua ARB API.
    GLuint vs = glCreateShaderObjectARB(0x8B31 /*VERTEX*/);
    Check(vs != 0, "CreateShaderObjectARB trả 0");
    GLuint pr = glCreateProgramObjectARB();
    Check(pr != 0, "CreateProgramObjectARB trả 0");
    // Attach + delete (type-dispatch).
    glAttachObjectARB(pr, vs);
    Check(glGetError() == 0, "AttachObjectARB lỗi");
    // InfoLog (shader chưa compile → log rỗng hoặc lỗi, không crash).
    char log[256] = {0}; GLsizei n = 0;
    glGetInfoLogARB(vs, sizeof(log), &n, log);
    Check(glGetError() == 0, "GetInfoLogARB shader lỗi");
    glGetInfoLogARB(pr, sizeof(log), &n, log);
    Check(glGetError() == 0, "GetInfoLogARB program lỗi");
    // Parameter (SHADER_TYPE cho shader, LINK_STATUS cho program).
    GLint t = 0;
    glGetObjectParameterivARB(vs, 0x8B4F /*SHADER_TYPE*/, &t);
    Check(t == 0x8B31, "GetObjectParameterivARB SHADER_TYPE sai");
    glGetObjectParameterivARB(pr, 0x8B82 /*LINK_STATUS*/, &t);
    Check(glGetError() == 0, "GetObjectParameterivARB program lỗi");
    // Use + delete.
    glUseProgramObjectARB(pr);
    Check(glGetError() == 0, "UseProgramObjectARB lỗi");
    glDeleteObjectARB(vs);
    glDeleteObjectARB(pr);
    Check(glGetError() == 0, "DeleteObjectARB lỗi");
    // Delete object không tồn tại → INVALID_OPERATION.
    glDeleteObjectARB(99999);
    Check(glGetError() == 0x0502, "DeleteObjectARB lạ phải INVALID_OPERATION");

    if (gFails == 0) { printf("test_shader_objects_p8: all OK\n"); return 0; }
    printf("test_shader_objects_p8: %d FAIL\n", gFails);
    return 1;
}
