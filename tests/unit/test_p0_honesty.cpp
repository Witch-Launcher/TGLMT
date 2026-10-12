// test_p0_honesty.cpp — Unit: P0 trung thực (sửa 3 extension nói dối + UB fixes).
//
// Căn cứ spec (docs/khronos/glspec46.core.pdf + ARB_polygon_offset_clamp.txt +
// ARB_pipeline_statistics_query.txt):
//  - GL_CONTEXT_PROFILE_MASK (gl.xml 0x9126) phải trả GL_CONTEXT_CORE_PROFILE_BIT
//    (0x1) vì TGLMT là GL 4.6 CORE. Trả 0 = nói dối.
//  - glGetProgramBinary (§7.2): không có binary → ghi 0 vào output, KHÔNG để rác.
//  - glPolygonOffset (§17.4.3.1) + ARB_polygon_offset_clamp: shadow phải trả lại
//    đúng factor/units/clamp đã set (glGet 0x8038/0x2A00/0x8E1B).
//  - glTexStorage2D (§8.18): immutable storage phải cấp texture object thật
//    (w/h/levels/internalFormat), không phải no-op.
//  - ARB_pipeline_statistics_query: glBeginQuery(VERTICES_SUBMITTED) → draw →
//    glEndQuery → glGetQueryObjectui64v trả ĐÚNG số vertex đã submit.
//  - Danh sách extension: ARB_pipeline_statistics_query KHÔNG được quảng cáo
//    (fragment-shader-invocations không đo được), ARB_polygon_offset_clamp CÓ.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_p0_honesty FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec3 pos;\n"
    "layout(location=0) out vec4 vColor;\n"
    "void main() { gl_Position = vec4(pos, 1.0); vColor = vec4(pos, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location=0) in vec4 vColor;\n"
    "layout(location=0) out vec4 fragColor;\n"
    "void main() { fragColor = vColor; }\n";

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- 1. GL_CONTEXT_PROFILE_MASK → GL_CORE_PROFILE_BIT (gl.xml 0x9126 = 0x1).
    {
        GLint mask = -1;
        glGetIntegerv(0x9126, &mask);
        Check(mask == 0x00000001, "GL_CONTEXT_PROFILE_MASK == GL_CONTEXT_CORE_PROFILE_BIT");
        GLint flags = -1;
        glGetIntegerv(0x821E, &flags);
        Check(flags == 0, "GL_CONTEXT_FLAGS == 0 (không debug bit)");
    }

    // --- 2. glGetProgramBinary zero-init (fix UB: không để rác của caller).
    {
        GLuint prog = glCreateProgram();
        // điền "rác" vào output trước để chắc chắn hàm ghi đè
        GLsizei len = 0x7FFF;
        GLenum fmt = 0xDEAD;
        unsigned char bin[64];
        memset(bin, 0xAB, sizeof(bin));
        glGetProgramBinary(prog, (GLsizei)sizeof(bin), &len, &fmt, bin);
        Check(len == 0, "glGetProgramBinary length == 0 (không có binary)");
        Check(fmt == 0, "glGetProgramBinary binaryFormat == 0");
        bool allZero = true;
        for (size_t i = 0; i < sizeof(bin); ++i)
            if (bin[i] != 0) { allZero = false; break; }
        Check(allZero, "glGetProgramBinary zero-filled buffer (không UB rác)");
        glDeleteProgram(prog);
    }

    // --- 3. glPolygonOffset + glPolygonOffsetClamp → shadow glGet đúng.
    {
        glPolygonOffset(1.5f, 3.0f);
        GLfloat f = -1, u = -1, cl = -1;
        glGetFloatv(0x8038, &f); // POLYGON_OFFSET_FACTOR
        glGetFloatv(0x2A00, &u); // POLYGON_OFFSET_UNITS
        Check(f == 1.5f, "glGet(GL_POLYGON_OFFSET_FACTOR) trả factor đã set");
        Check(u == 3.0f, "glGet(GL_POLYGON_OFFSET_UNITS) trả units đã set");

        glPolygonOffsetClamp(2.0f, 4.0f, 0.25f);
        glGetFloatv(0x8038, &f);
        glGetFloatv(0x2A00, &u);
        glGetFloatv(0x8E1B, &cl); // POLYGON_OFFSET_CLAMP (ARB_polygon_offset_clamp)
        Check(f == 2.0f && u == 4.0f, "glPolygonOffsetClamp cập nhật factor/units");
        Check(cl == 0.25f, "glGet(GL_POLYGON_OFFSET_CLAMP) trả clamp đã set");

        // glPolygonOffset (không clamp) → clamp theo convention = 0 (không clamp)
        glPolygonOffset(1.0f, 1.0f);
        glGetFloatv(0x8E1B, &cl);
        Check(cl == 0.0f, "glPolygonOffset (không clamp) → clamp shadow == 0");
    }

    // --- 4. glTexStorage2D → texture object được cấp thật (không no-op).
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1 /*TEXTURE_2D*/, tex);
        glTexStorage2D(0x0DE1, 3, 0x8058 /*RGBA8*/, 64, 64);
        auto it = ctx.textures.find(tex);
        Check(it != ctx.textures.end(), "glTexStorage2D tạo texture object");
        if (it != ctx.textures.end()) {
            Check(it->second.w == 64 && it->second.h == 64, "glTexStorage2D set w/h");
            Check(it->second.levels == 3, "glTexStorage2D set levels");
            Check(it->second.internalFormat == 0x8058, "glTexStorage2D set internalFormat");
        }
        glDeleteTextures(1, &tex);
    }

    // --- 5. glGetInteger64v không cắt cụt (đọc shadow 8-trước, rồi 4-trước).
    //        GL_TIMESTAMP không phải pname hợp lệ của glGetInteger64v (nó là query
    //        target). Test regression: đọc một limit 32-bit phải ra đúng giá trị
    //        (trước đây glGetInteger64v ép GLint→GLint64, mọi giá trị đều "đúng"
    //        với số < 2^31 nhưng sai cấu trúc; test xác nhận đường đọc 8-trước
    //        không làm hỏng giá trị 32-bit).
    {
        GLint64 v = -1;
        glGetInteger64v(0x0D33 /*GL_MAX_TEXTURE_SIZE*/, &v);
        Check(v == 8192, "glGetInteger64v(MAX_TEXTURE_SIZE) trả 8192 (không cắt/đảo)");
        glGetInteger64v(0x821B /*GL_MAJOR_VERSION*/, &v);
        Check(v == 4, "glGetInteger64v(MAJOR_VERSION) trả 4");
    }

    // --- 6. ARB_pipeline_statistics_query: BeginQuery(VERTICES_SUBMITTED) →
    //        draw → EndQuery → glGetQueryObjectui64v trả đúng số vertex.
    {
        GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
        glShaderSource(vs, 1, &kVS, nullptr);
        glShaderSource(fs, 1, &kFS, nullptr);
        glCompileShader(vs);
        glCompileShader(fs);
        GLint ok = 0;
        glGetShaderiv(vs, 0x8B81, &ok);
        Check(ok != 0, "vs compile (stats)");
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);
        glGetProgramiv(prog, 0x8B82, &ok);
        Check(ok != 0, "link (stats)");

        // VBO 3 đỉnh
        GLfloat verts[9] = {0, 0, 0, 1, 0, 0, 0, 1, 0};
        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, vbo);
        glBufferData(0x8892, sizeof(verts), verts, 0x88E4 /*STATIC_DRAW*/);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, 0x1406 /*FLOAT*/, 0, 12, nullptr);

        GLuint q = 0;
        glGenQueries(1, &q);
        glBeginQuery(0x82EE /*GL_VERTICES_SUBMITTED_ARB*/, q);
        glUseProgram(prog);
        glDrawArrays(0x0004 /*TRIANGLES*/, 0, 3);
        glEndQuery(0x82EE);
        GLuint64 vertsSubmitted = 0;
        glGetQueryObjectui64v(q, 0x8866 /*GL_QUERY_RESULT*/, &vertsSubmitted);
        Check(vertsSubmitted == 3, "VERTICES_SUBMITTED == 3 (1 draw × 3 đỉnh)");

        // Second draw: cộng dồn
        glBeginQuery(0x82EE, q);
        glDrawArrays(0x0004, 0, 6);
        glEndQuery(0x82EE);
        glGetQueryObjectui64v(q, 0x8866, &vertsSubmitted);
        Check(vertsSubmitted == 6, "VERTICES_SUBMITTED delta == 6 (chỉ draw thứ 2)");

        // PRIMITIVES_SUBMITTED
        GLuint q2 = 0;
        glGenQueries(1, &q2);
        glBeginQuery(0x82EF /*GL_PRIMITIVES_SUBMITTED_ARB*/, q2);
        glDrawArrays(0x0004, 0, 9); // 3 triangles
        glEndQuery(0x82EF);
        GLuint64 prims = 0;
        glGetQueryObjectui64v(q2, 0x8866, &prims);
        Check(prims == 3, "PRIMITIVES_SUBMITTED == 3 (3 triangles)");

        glDeleteQueries(1, &q);
        glDeleteQueries(1, &q2);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // --- 7. Danh sách extension trung thực.
    {
        GLint n = 0;
        glGetIntegerv(0x821D /*GL_NUM_EXTENSIONS*/, &n);
        Check(n == (GLint)tglmt_num_extensions(), "NUM_EXTENSIONS khớp tglmt_num_extensions()");
        bool hasPipe = false, hasClamp = false, hasNoErr = false, hasSpirv = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (!e) continue;
            if (std::string(e) == "GL_ARB_pipeline_statistics_query") hasPipe = true;
            if (std::string(e) == "GL_ARB_polygon_offset_clamp") hasClamp = true;
            if (std::string(e) == "GL_KHR_no_error") hasNoErr = true;
            if (std::string(e) == "GL_ARB_gl_spirv") hasSpirv = true;
        }
        Check(!hasPipe, "ARB_pipeline_statistics_query KHÔNG quảng cáo (fragment invocations không đo được)");
        Check(hasClamp, "ARB_polygon_offset_clamp CÓ quảng cáo (depthBias clamp thật)");
        Check(hasNoErr, "KHR_no_error CÓ quảng cáo (env TGLMT_NO_ERROR wire SetNoError)");
        Check(hasSpirv, "ARB_gl_spirv CÓ quảng cáo");
    }

    // --- 8. glInvalidateFramebuffer → contentUndefined (TBDR DontCare).
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1, tex);
        glTexStorage2D(0x0DE1, 1, 0x8058, 32, 32);
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(0x8D40 /*FRAMEBUFFER*/, fbo);
        glFramebufferTexture2D(0x8D40, 0x8CE0 /*COLOR_ATTACHMENT0*/, 0x0DE1, tex, 0);
        GLenum att = 0x8CE0;
        glInvalidateFramebuffer(0x8CA9 /*DRAW_FRAMEBUFFER*/, 1, &att);
        auto it = ctx.textures.find(tex);
        Check(it != ctx.textures.end(), "invalidate: texture tồn tại");
        if (it != ctx.textures.end())
            Check(it->second.contentUndefined, "glInvalidateFramebuffer set contentUndefined");
        glBindFramebuffer(0x8D40, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &tex);
    }

    if (gFails) return 1;
    printf("test_p0_honesty PASS\n");
    return 0;
}
