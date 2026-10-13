// test_enum_behavior.cpp — Unit: P3 batch-4..7 enum-ext behaviors.
//
// Quy tắc vàng: ext advertise ⇒ test pass. Verify behavior THẬT:
//  1. internalformat_query: glGetInternalformativ trả INTERNALFORMAT_SUPPORTED
//     = 1 cho format map thật (RGBA8), = 0 cho format lạ; COLOR_RENDERABLE=1.
//  2. depth_clamp: glEnable(GL_DEPTH_CLAMP) → glIsEnabled = GL_TRUE.
//  3. batch-5 vertex-format: glVertexAttribPointer chấp nhận HALF_FLOAT /
//     INT_2_10_10_10_REV / UINT_2_10_10_10_REV / GL_BGRA không INVALID_ENUM
//     và store type (ToMTLVertexFormat map Metal thật).
//  4. batch-6 EXT_texture_swizzle: mặc định identity, round-trip qua
//     glTexParameteriv(0x8E46) + component lẻ (glTexParameteri) hiện trong mảng.
//  5. batch-7 GL_ARB_get_program_binary: serialize GLSL gốc → glProgramBinary
//     parse + relink → LINK_STATUS=1, LENGTH ổn định; format lạ → INVALID_ENUM.
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
    if (!cond) { printf("test_enum_behavior FAIL: %s\n", msg); ++gFails; }
}

static const char* kBatch4[] = {
    "GL_ARB_internalformat_query", "GL_ARB_internalformat_query2",
    "GL_ARB_depth_clamp",
};
static const char* kBatch5[] = {
    "GL_ARB_half_float_vertex", "GL_ARB_vertex_type_2_10_10_10_rev",
    "GL_ARB_vertex_array_bgra", "GL_EXT_vertex_array_bgra",
};
static const char* kBatch6[] = {"GL_EXT_texture_swizzle"};
static const char* kBatch7[] = {"GL_ARB_get_program_binary"};

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- 1. Cả 3 ext batch-4 được advertise.
    GLint n = 0;
    glGetIntegerv(0x821D /*GL_NUM_EXTENSIONS*/, &n);
    Check(n == (GLint)tglmt_num_extensions(), "NUM_EXTENSIONS khớp");
    for (const char* want : kBatch4) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break; }
        }
        Check(found, want);
    }

    // --- 2. internalformat_query: INTERNALFORMAT_SUPPORTED.
    GLint sup = -1;
    glGetInternalformativ(0x0DE1 /*TEXTURE_2D*/, 0x8058 /*RGBA8*/,
                          0x826F /*INTERNALFORMAT_SUPPORTED*/, 1, &sup);
    Check(sup == 1, "RGBA8 supported = 1");
    GLint weird = -1;
    glGetInternalformativ(0x0DE1, 0x1234 /*format lạ*/,
                          0x826F, 1, &weird);
    Check(weird == 0, "format lạ supported = 0 (trung thực)");
    GLint colorR = -1;
    glGetInternalformativ(0x0DE1, 0x8058 /*RGBA8*/,
                          0x8286 /*COLOR_RENDERABLE*/, 1, &colorR);
    Check(colorR == 1, "RGBA8 color renderable = 1");

    // --- 3. depth_clamp: glEnable → glIsEnabled.
    glDisable(0x864F /*GL_DEPTH_CLAMP*/);
    Check(glIsEnabled(0x864F) == GL_FALSE, "depth_clamp mặc định off");
    glEnable(0x864F);
    Check(glIsEnabled(0x864F) == GL_TRUE, "glEnable(GL_DEPTH_CLAMP) → IsEnabled=TRUE");
    glDisable(0x864F);

    // --- P3 batch-5: vertex-format behaviors (advertise ⇒ verify).
    for (const char* want : kBatch5) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break;        }
        }
        Check(found, want);
    }
    // Behavior: glVertexAttribPointer chấp nhận 3 type mới KHÔNG INVALID_ENUM
    // và store đúng type (ToMTLVertexFormat map thật trên Metal thật).
    while (glGetError() != 0) {} // drain
    GLuint vao = 0; glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glBindBuffer(0x8892 /*ARRAY_BUFFER*/, 0);
    struct C { GLenum type; GLenum want; };
    const C kCases[] = {
        {0x140B /*HALF_FLOAT*/, 0x140B},
        {0x8368 /*INT_2_10_10_10_REV*/, 0x8368},
        {0x8367 /*UNSIGNED_INT_2_10_10_10_REV*/, 0x8367},
        {0x80E1 /*GL_BGRA*/, 0x80E1},
    };
    for (const C& t : kCases) {
        glVertexAttribPointer(0, 4, t.type, GL_TRUE, 0, nullptr);
        Check(glGetError() == 0, "attrib type không INVALID_ENUM");
        GLenum got = 0;
        glGetVertexAttribiv(0, 0x8626 /*ARRAY_TYPE*/, (GLint*)&got);
        Check(got == t.want, "attrib type store đúng");
    }
    glBindVertexArray(0); glDeleteVertexArrays(1, &vao);

    // --- P3 batch-6: GL_EXT_texture_swizzle round-trip.
    for (const char* want : kBatch6) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break; }
        }
        Check(found, want);
    }
    while (glGetError() != 0) {}
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    // Spec: mặc định = identity (RED, GREEN, BLUE, ALPHA).
    GLint got[4] = {-1, -1, -1, -1};
    glGetTexParameteriv(GL_TEXTURE_2D, 0x8E46 /*SWIZZLE_RGBA*/, got);
    Check(got[0] == 0x1903 && got[1] == 0x1904 && got[2] == 0x1905 &&
              got[3] == 0x1906,
          "swizzle mặc định identity");
    // Đặt (ZERO, RED, GREEN, ALPHA) → round-trip qua glTexParm array.
    const GLint set[4] = {0x0000, 0x1903, 0x1904, 0x1906};
    glTexParameteriv(GL_TEXTURE_2D, 0x8E46, set);
    Check(glGetError() == 0, "glTexParameteriv(0x8E46) không lỗi");
    glGetTexParameteriv(GL_TEXTURE_2D, 0x8E46, got);
    Check(got[0] == 0x0000 && got[1] == 0x1903 && got[2] == 0x1904 &&
              got[3] == 0x1906,
          "swizzle round-trip (ZERO,RED,GREEN,ALPHA)");
    // Đặt component lẻ cũng phải hiện trong mảng 0x8E46.
    glTexParameteri(GL_TEXTURE_2D, 0x8E45 /*SWIZZLE_A*/, 0x0001 /*ONE*/);
    glGetTexParameteriv(GL_TEXTURE_2D, 0x8E46, got);
    Check(got[3] == 0x0001, "glTexParameteri component lẻ hiện trong 0x8E46");

    // swizzledView: identity → nullptr (dùng base); khác identity →
    // Apple backend tạo MTLTextureSwizzle view THẬT, Null backend trung thực
    // trả nullptr (không giả vờ có view).
    GLubyte px[4] = {255, 0, 0, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    {
        Context& c = Context::Current();
        auto it = c.textures.find(tex);
        Check(it != c.textures.end() && it->second.gpu != nullptr, "gpu texture tạo");
        if (it != c.textures.end() && it->second.gpu) {
            auto idView = it->second.gpu->swizzledView(2, 3, 4, 5); // identity
            Check(!idView, "swizzledView(identity) → nullptr (base)");
            auto swz = it->second.gpu->swizzledView(0, 0, 0, 5); // ZERO,ZERO,ZERO,ALPHA
            if (c.device && c.device->isNull()) {
                Check(!swz, "Null backend: swizzledView → nullptr (trung thực)");
            } else {
                Check(swz != nullptr, "Metal: swizzledView tạo view");
                if (swz) Check(swz->width() == it->second.gpu->width(),
                               "view giữ kích thước gốc");
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glDeleteTextures(1, &tex);

    // --- P3 batch-7: GL_ARB_get_program_binary round-trip.
    for (const char* want : kBatch7) {
        bool found = false;
        for (GLint i = 0; i < n; ++i) {
            const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
            if (e && std::string(e) == want) { found = true; break; }
        }
        Check(found, want);
    }
    while (glGetError() != 0) {}
    GLint nf = -1, fmtQ = 0;
    glGetIntegerv(0x87FE /*NUM_PROGRAM_BINARY_FORMATS*/, &nf);
    Check(nf == 1, "NUM_PROGRAM_BINARY_FORMATS = 1");
    glGetIntegerv(0x87FD /*PROGRAM_BINARY_FORMATS*/, &fmtQ);
    Check(fmtQ == (GLint)0x54474D54, "PROGRAM_BINARY_FORMATS[0] = TGMT");
    {
        const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0.25); }";
        const char* fs = "#version 460 core\nout vec4 c;\nvoid main(){ c = vec4(0.5); }";
        GLuint vsS = glCreateShader(0x8B31), fsS = glCreateShader(0x8B30);
        glShaderSource(vsS, 1, &vs, nullptr);
        glCompileShader(vsS);
        glShaderSource(fsS, 1, &fs, nullptr);
        glCompileShader(fsS);
        GLuint p1 = glCreateProgram();
        glAttachShader(p1, vsS);
        glAttachShader(p1, fsS);
        glLinkProgram(p1);
        GLint ok = 0;
        glGetProgramiv(p1, 0x8B82 /*LINK_STATUS*/, &ok);
        Check(ok == 1, "link program gốc");
        GLint len = 0;
        glGetProgramiv(p1, 0x8741 /*PROGRAM_BINARY_LENGTH*/, &len);
        Check(len >= 8, "PROGRAM_BINARY_LENGTH > 0");
        std::vector<unsigned char> bin((size_t)(len > 0 ? len : 1));
        GLsizei written = 0;
        GLenum f = 0;
        glGetProgramBinary(p1, len, &written, &f, bin.data());
        Check(written == len && f == 0x54474D54, "glGetProgramBinary ghi đủ + format TGMT");
        GLuint p2 = glCreateProgram();
        glProgramBinary(p2, f, bin.data(), written);
        GLint ok2 = 0;
        glGetProgramiv(p2, 0x8B82, &ok2);
        Check(ok2 == 1, "glProgramBinary → relink OK");
        GLint len2 = 0;
        glGetProgramiv(p2, 0x8741, &len2);
        Check(len2 == len, "round-trip LENGTH ổn định");
        // Format lạ → INVALID_ENUM trung thực (không đoán nội dung driver khác).
        while (glGetError() != 0) {}
        GLuint p3 = glCreateProgram();
        glProgramBinary(p3, 0x1234, bin.data(), written);
        Check(glGetError() == 0x0500, "format lạ → INVALID_ENUM");
    }

    if (gFails) return 1;
    printf("test_enum_behavior PASS\n");
    return 0;
}
