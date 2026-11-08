// test_core41_contract.cpp — core 4.1/4.3 contract còn thiếu:
//   - ARB_ES2_compatibility queries + glShaderBinary fail trung thực khi
//     NUM_SHADER_BINARY_FORMATS = 0.
//   - glClearBuffer(Sub)Data validate format và clear pattern theo internalformat.
//   - glCopyTexSubImage2D level>0 (mip chain) upload đúng vùng.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_core41_contract FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context& c = Context::Current();

    // ===== ES2 compatibility queries =====
    {
        GLint compiler = 0, nfmt = -1;
        glGetIntegerv(0x8DFA /*SHADER_COMPILER*/, &compiler);
        glGetIntegerv(0x8DF9 /*NUM_SHADER_BINARY_FORMATS*/, &nfmt);
        Check(compiler == 1, "SHADER_COMPILER phải TRUE");
        Check(nfmt == 0, "NUM_SHADER_BINARY_FORMATS phải 0");
        GLint vmax = 0, vvary = 0, fmax = 0;
        glGetIntegerv(0x8DFB, &vmax); glGetIntegerv(0x8DFC, &vvary);
        glGetIntegerv(0x8DFD, &fmax);
        Check(vmax >= 256 && vvary >= 32 && fmax >= 256,
              "MAX_*_UNIFORM/VARYING_VECTORS phải >= GL 4.6 minimum");
        GLint rtype = 0, rformat = 0;
        glGetIntegerv(0x8B9A /*IMPLEMENTATION_COLOR_READ_TYPE*/, &rtype);
        glGetIntegerv(0x8B9B /*IMPLEMENTATION_COLOR_READ_FORMAT*/, &rformat);
        Check(rtype == 0x1401 && rformat == 0x1908, "ReadPixels type/format query");

        GLuint sh = glCreateShader(0x8B31 /*VERTEX_SHADER*/);
        const char* src = "#version 460 core\nvoid main(){gl_Position=vec4(0);}\n";
        glShaderSource(sh, 1, &src, nullptr); glCompileShader(sh);
        while (glGetError() != 0) {}
        glShaderBinary(1, &sh, 0x1908 /*RGBA8: format lạ*/, src, (GLsizei)strlen(src));
        Check(glGetError() == 0x0500, "ShaderBinary format không hỗ trợ → INVALID_ENUM");
        glShaderBinary(-1, &sh, 0x1234, src, 0);
        Check(glGetError() == 0x0501, "ShaderBinary count âm → INVALID_VALUE");
        glDeleteShader(sh);
    }

    // ===== ClearBufferData / SubData =====
    {
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, buf);
        GLuint zero[4] = {0, 0, 0, 0};
        glBufferData(0x8892, sizeof(zero), zero, 0x88E4 /*STATIC_DRAW*/);
        GLuint v = 0x11223344u;
        glClearBufferData(0x8892, 0x8236 /*R32UI*/, 0x1903 /*RED_INTEGER*/,
                          0x1405 /*UNSIGNED_INT*/, &v);
        Check(glGetError() == 0, "ClearBufferData R32UI no error");
        GLuint out[4] = {};
        glGetBufferSubData(0x8892, 0, sizeof(out), out);
        Check(out[0] == v && out[1] == v && out[2] == v && out[3] == v,
              "ClearBufferData lặp đúng pattern uint32");

        GLuint w = 0xAABBCCDDu;
        glClearBufferSubData(0x8892, 0x8236, 8, 4, 0x1903, 0x1405, &w);
        glGetBufferSubData(0x8892, 0, sizeof(out), out);
        Check(out[2] == w && out[3] == v, "ClearBufferSubData đúng offset/size");

        while (glGetError() != 0) {}
        glClearBufferData(0x8892, 0x9999 /*format lạ*/, 0x1903, 0x1405, &v);
        Check(glGetError() == 0x0500, "ClearBufferData format lạ → INVALID_ENUM");

        glBindBuffer(0x8892, 0);
        while (glGetError() != 0) {}
        glClearBufferData(0x8892, 0x8236, 0x1903, 0x1405, &v);
        Check(glGetError() == 0x0502, "ClearBufferData không có buffer → INVALID_OPERATION");
        glDeleteBuffers(1, &buf);
    }

    // ===== CopyTexSubImage2D level > 0 =====
    if (c.device && !c.device->isNull()) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1 /*TEXTURE_2D*/, tex);
        glTexStorage2D(0x0DE1, 5, 0x8058 /*RGBA8*/, 32, 32);
        glClearColor(1, 0, 0, 1);
        glClear(0x4000);
        c.FlushPendingEncoder(); c.CommitAndWait();
        glCopyTexSubImage2D(0x0DE1, 1, 0, 0, 0, 0, 16, 16);
        Check(glGetError() == 0, "CopyTexSubImage2D level=1 no error");
        unsigned char px[16 * 16 * 4] = {};
        glGetTexImage(0x0DE1, 1, 0x1908, 0x1401, px);
        Check(glGetError() == 0, "GetTexImage level=1 no error");
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50,
              "CopyTexSubImage2D level=1 chép pixel đỏ");
        glDeleteTextures(1, &tex);
    }

    if (gFails) { printf("test_core41_contract: %d FAIL\n", gFails); return 1; }
    printf("test_core41_contract PASS\n");
    return 0;
}
