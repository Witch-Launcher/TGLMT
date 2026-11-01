// test_shader_precision.cpp — Unit: glGetShaderPrecisionFormat values.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_shader_precision FAIL: %s\n", msg); ++gFails; }
}

static void CheckFloat(GLenum stage, GLenum prec, const char* what) {
    GLint range[2] = {-1, -1}, precision = -1;
    glGetShaderPrecisionFormat(stage, prec, range, &precision);
    Check(range[0] == 127 && range[1] == 127 && precision == 23, what);
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    CheckFloat(0x8B31, 0x8DF2, "vertex high float");
    CheckFloat(0x8B30, 0x8DF1, "fragment medium float");
    CheckFloat(0x91B9, 0x8DF0, "compute low float");

    GLint range[2] = {-1, -1}, precision = -1;
    glGetShaderPrecisionFormat(0x8B31, 0x8DF5 /*HIGH_INT*/, range, &precision);
    Check(range[0] == 31 && range[1] == 31 && precision == 0, "vertex high int");

    glGetShaderPrecisionFormat(0x1234, 0x8DF2, range, &precision);
    Check(glGetError() == GL_INVALID_ENUM, "bad shader type INVALID_ENUM");
    glGetShaderPrecisionFormat(0x8B31, 0x1234, range, &precision);
    Check(glGetError() == GL_INVALID_ENUM, "bad precision type INVALID_ENUM");

    return gFails ? 1 : 0;
}
