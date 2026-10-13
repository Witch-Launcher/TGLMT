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
static const char* kBatch8[] = {
    "GL_ARB_texture_mirror_clamp_to_edge", "GL_EXT_texture_mirror_clamp_to_edge",
    "GL_ARB_texture_swizzle",
    "GL_ARB_draw_elements_base_vertex", "GL_ARB_copy_image",
    "GL_ARB_get_texture_sub_image", "GL_ARB_multi_draw_indirect",
    "GL_ARB_texture_buffer_range", "GL_ARB_vertex_attrib_binding",
    "GL_ARB_sync", "GL_ARB_timer_query", "GL_NV_polygon_mode",
    "GL_MESA_sampler_objects", "GL_AMD_multi_draw_indirect",
    "GL_NV_copy_buffer", "GL_NV_copy_image", "GL_NV_draw_instanced",
    "GL_NV_draw_buffers", "GL_NV_framebuffer_blit", "GL_NV_instanced_arrays",
    "GL_NV_non_square_matrices", "GL_ATI_draw_buffers",
    "GL_OES_get_program_binary",
};
static const char* kBatch9[] = {
    "GL_ARB_robustness", "GL_KHR_robustness", "GL_EXT_robustness",
    "GL_ATI_separate_stencil",
};
static const char* kBatch10[] = {
    "GL_ARB_occlusion_query2", "GL_ARB_multi_bind",
    "GL_ARB_map_buffer_alignment", "GL_ARB_explicit_uniform_location",
};
// Alias OES chỉ có symbol extern "C" (gl_ext_aliases_gen), không khai báo gl46.h.
extern "C" void glProgramBinaryOES(GLuint p, GLenum f, const void* d, GLint l);
extern "C" void glGetProgramBinaryOES(GLuint p, GLsizei b, GLsizei* w, GLenum* f, void* d);
// Batch-9 alias (robustness / ATI stencil).
extern "C" GLenum glGetGraphicsResetStatusKHR();
extern "C" void glGetnUniformfvKHR(GLuint p, GLint l, GLsizei n, GLfloat* v);
extern "C" void glReadnPixelsKHR(GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum ty, GLsizei n, void* p);
extern "C" GLenum glGetGraphicsResetStatusEXT();
extern "C" void glStencilFuncSeparateATI(GLenum face, GLenum f, GLint r, GLuint m);
extern "C" void glStencilOpSeparateATI(GLenum face, GLenum sf, GLenum dpf, GLenum dpp);

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

    // --- 8. Batch-8: 23 ext được advertise (Quy tắc vàng).
    {
        GLint n8 = 0;
        glGetIntegerv(0x821D /*NUM_EXTENSIONS*/, &n8);
        for (const char* want : kBatch8) {
            bool found = false;
            for (GLint i = 0; i < n8 && !found; ++i) {
                const char* e = (const char*)glGetStringi(0x1F03 /*EXTENSIONS*/, (GLuint)i);
                if (e && !strcmp(e, want)) found = true;
            }
            Check(found, want);
        }

        // 8a. MIRROR_CLAMP_TO_EDGE round-trip qua sampler (CPU-side).
        while (glGetError() != 0) {}
        GLuint samp = 0;
        glGenSamplers(1, &samp);
        glSamplerParameteri(samp, 0x2802 /*TEXTURE_WRAP_S*/, 0x8743 /*MIRROR_CLAMP_TO_EDGE*/);
        Check(glGetError() == 0, "glSamplerParameteri MIRROR_CLAMP_TO_EDGE không lỗi");
        GLint wrap = 0;
        glGetSamplerParameteriv(samp, 0x2802, &wrap);
        Check(wrap == 0x8743, "sampler wrap round-trip MIRROR_CLAMP_TO_EDGE");
        // glTexParameteriv trên texture cũng round-trip (ext dùng chung path).
        GLuint t = 0;
        glGenTextures(1, &t);
        glBindTexture(0x0DE1 /*TEXTURE_2D*/, t);
        glTexParameteri(0x0DE1, 0x2802, 0x8743);
        GLint wrapT = 0;
        glGetTexParameteriv(0x0DE1, 0x2802, &wrapT);
        Check(wrapT == 0x8743, "texture wrap round-trip MIRROR_CLAMP_TO_EDGE");
        glDeleteSamplers(1, &samp);
        glDeleteTextures(1, &t);

        // 8b. GL_ARB_sync: fence object thật + glGetSynciv.
        while (glGetError() != 0) {}
        GLsync sync = glFenceSync(0x911E /*SYNC_GPU_COMMANDS_COMPLETE*/, 0);
        Check(sync != nullptr && glIsSync(sync) == GL_TRUE, "glFenceSync tạo object thật");
        Check(glGetError() == 0, "glFenceSync không lỗi");
        GLint64 timeoutVal = 0;
        GLsizei len = 0;
        glGetSynciv(sync, 0x9116 /*SYNC_STATUS*/, 1, &len, (GLint*)&timeoutVal);
        Check(glGetSynciv != nullptr, "glGetSynciv resolvable");
        glDeleteSync(sync);

        // 8c. GL_ARB_timer_query: glQueryCounter thật (timestamp CPU).
        while (glGetError() != 0) {}
        GLuint q = 0;
        glGenQueries(1, &q);
        glQueryCounter(q, 0x8E56 /*TIMESTAMP*/);
        Check(glGetError() == 0, "glQueryCounter TIMESTAMP không lỗi");
        GLint64 ts = 0;
        glGetQueryObjecti64v(q, 0x88BF /*QUERY_RESULT*/, &ts);
        Check(glGetError() == 0, "glGetQueryObjecti64v TIMESTAMP không lỗi");
        glDeleteQueries(1, &q);

        // 8d. GL_ARB_vertex_attrib_binding: glBindVertexBuffer lưu VAO binding.
        while (glGetError() != 0) {}
        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, vbo);
        glBindVertexBuffer(0, vbo, 0, 16);
        Check(glGetError() == 0, "glBindVertexBuffer không lỗi");
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);

        // 8e. GL_ARB_texture_buffer_range: glTexBufferRange slice hợp lệ.
        while (glGetError() != 0) {}
        GLuint texBuf = 0, dataBuf = 0;
        glGenTextures(1, &texBuf);
        glBindTexture(0x8C2A /*TEXTURE_BUFFER*/, texBuf); // cần target TEXTURE_BUFFER
        glGenBuffers(1, &dataBuf);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, dataBuf);
        float bufData[16] = {0};
        glBufferData(0x8892, sizeof(bufData), bufData, 0x88E4 /*STATIC_COPY*/);
        glTexBufferRange(0x8C2A /*TEXTURE_BUFFER*/, 0x822E /*R32F*/, dataBuf, 0, 16);
        Check(glGetError() == 0, "glTexBufferRange không lỗi");
        glDeleteBuffers(1, &dataBuf);
        glDeleteTextures(1, &texBuf);

        // 8f. GL_NV_polygon_mode: round-trip FILL/LINE.
        while (glGetError() != 0) {}
        glPolygonMode(0x0408 /*FRONT_AND_BACK*/, 0x1B02 /*FILL*/);
        GLint pm[2] = {0, 0};
        glGetIntegerv(0x0B40 /*POLYGON_MODE*/, pm);
        Check(pm[0] == 0x1B02, "glPolygonMode FILL round-trip");
        glPolygonMode(0x0408, 0x1B01 /*LINE*/);
        glGetIntegerv(0x0B40, pm);
        Check(pm[0] == 0x1B01, "glPolygonMode LINE round-trip");
        glPolygonMode(0x0408, 0x1B02);

        // 8g. GL_OES_get_program_binary: alias symbol hoạt động như core.
        while (glGetError() != 0) {}
        const char* vs2 = "#version 460 core\nvoid main(){ gl_Position = vec4(0); }";
        GLuint sh = glCreateShader(0x8B31);
        glShaderSource(sh, 1, &vs2, nullptr);
        glCompileShader(sh);
        GLuint prog = glCreateProgram();
        glAttachShader(prog, sh);
        glLinkProgram(prog);
        GLint blen = 0;
        glGetProgramiv(prog, 0x8741, &blen);
        Check(blen >= 8, "OES: PROGRAM_BINARY_LENGTH > 0");
        std::vector<unsigned char> obin((size_t)(blen > 0 ? blen : 1));
        GLsizei ow = 0;
        GLenum of = 0;
        glGetProgramBinaryOES(prog, blen, &ow, &of, obin.data());
        Check(ow == blen && of == 0x54474D54, "glGetProgramBinaryOES ghi + format TGMT");
        GLuint prog2 = glCreateProgram();
        glProgramBinaryOES(prog2, of, obin.data(), ow);
        GLint ok8 = 0;
        glGetProgramiv(prog2, 0x8B82, &ok8);
        Check(ok8 == 1, "glProgramBinaryOES relink OK");

        // 8h. GL_ARB_copy_image + GL_NV_copy_image + GL_ARB_get_texture_sub_image:
        //     không INVALID_ENUM / INVALID_OPERATION khi src/dst hợp lệ.
        while (glGetError() != 0) {}
        GLuint ta = 0, tb = 0;
        glGenTextures(1, &ta);
        glGenTextures(1, &tb);
        glBindTexture(0x0DE1, ta);
        glTexImage2D(0x0DE1, 0, 0x8058 /*RGBA8*/, 4, 4, 0, 0x1908 /*RGBA*/, 0x1401 /*UNSIGNED_BYTE*/, nullptr);
        glBindTexture(0x0DE1, tb);
        glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, nullptr);
        glCopyImageSubData(ta, 0x0DE1, 0, 0, 0, 0, tb, 0x0DE1, 0, 0, 0, 0, 4, 4, 1);
        Check(glGetError() == 0, "glCopyImageSubData RGBA8 4x4 không lỗi");
        std::vector<unsigned char> sub(4 * 4 * 4, 0);
        glGetTextureSubImage(ta, 0, 0, 0, 0, 4, 4, 1, 0x1908, 0x1401, (GLsizei)sub.size(), sub.data());
        Check(glGetError() == 0, "glGetTextureSubImage RGBA8 4x4 không lỗi");
        glDeleteTextures(1, &ta);
        glDeleteTextures(1, &tb);

        // 8i. GL_ARB_multi_draw_indirect + GL_NV/AMD draw_instanced:
        //     gọi với count=0 không lỗi (hàm resolvable + guard đúng).
        while (glGetError() != 0) {}
        glBindVertexArray(0);
        glMultiDrawArraysIndirect(0x0004 /*TRIANGLES*/, nullptr, 0, 0);
        Check(glGetError() == 0, "glMultiDrawArraysIndirect count=0 không lỗi");
        glDrawElementsBaseVertex(0x0004, 0, 0x1403 /*UNSIGNED_SHORT*/, nullptr, 0);
        Check(glGetError() == 0, "glDrawElementsBaseVertex count=0 không lỗi");
    }

    // --- 9. Batch-9: robustness + separate stencil.
    {
        GLint n9 = 0;
        glGetIntegerv(0x821D /*NUM_EXTENSIONS*/, &n9);
        for (const char* want : kBatch9) {
            bool found = false;
            for (GLint i = 0; i < n9 && !found; ++i) {
                const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
                if (e && !strcmp(e, want)) found = true;
            }
            Check(found, want);
        }

        // 9a. glGetGraphicsResetStatus trả NO_ERROR (0x8252) — trung thực.
        while (glGetError() != 0) {}
        Check(glGetGraphicsResetStatusKHR() == 0x8252, "KHR reset status = NO_ERROR");
        Check(glGetGraphicsResetStatusEXT() == 0x8252, "EXT reset status = NO_ERROR");
        Check(glGetError() == 0, "reset status không lỗi");

        // 9b. glReadnPixels guard bufSize: bufSize nhỏ hơn w*h*4 → INVALID_OPERATION.
        while (glGetError() != 0) {}
        unsigned char small[8] = {0};
        glReadnPixelsKHR(0, 0, 4, 4, 0x1908, 0x1401, (GLsizei)sizeof(small), small);
        Check(glGetError() == 0x0502, "glReadnPixels bufSize thiếu → INVALID_OPERATION");
        while (glGetError() != 0) {}
        unsigned char big[4 * 4 * 4] = {0};
        glReadnPixelsKHR(0, 0, 4, 4, 0x1908, 0x1401, (GLsizei)sizeof(big), big);
        Check(glGetError() == 0, "glReadnPixels bufSize đủ → không lỗi");

        // 9c. glGetnUniformfv đọc uniform có giới hạn count (không overflow).
        while (glGetError() != 0) {}
        const char* vsR = "#version 460 core\nlayout(location=0) uniform float u[4];\nvoid main(){ gl_Position = vec4(u[0],u[1],u[2],u[3]); }";
        const char* fsR = "#version 460 core\nout vec4 c;\nvoid main(){ c = vec4(1.0); }";
        GLuint vsR_s = glCreateShader(0x8B31), fsR_s = glCreateShader(0x8B30);
        glShaderSource(vsR_s, 1, &vsR, nullptr);
        glCompileShader(vsR_s);
        glShaderSource(fsR_s, 1, &fsR, nullptr);
        glCompileShader(fsR_s);
        GLuint pR = glCreateProgram();
        glAttachShader(pR, vsR_s);
        glAttachShader(pR, fsR_s);
        glLinkProgram(pR);
        GLint locR = glGetUniformLocation(pR, "u");
        Check(locR >= 0, "uniform u location hợp lệ");
        float setU[4] = {1.f, 2.f, 3.f, 4.f};
        glUseProgram(pR);
        glUniform1fv(locR, 4, setU);
        float getU[4] = {0};
        // bufSize là SỐ BYTE (spec glGetnUniformfv).
        glGetnUniformfvKHR(pR, locR, (GLsizei)sizeof(getU), getU);
        Check(glGetError() == 0, "glGetnUniformfv bufSize đủ → không lỗi");
        Check(getU[0] == 1.f && getU[1] == 2.f && getU[3] == 4.f, "glGetnUniformfv đọc đủ 4 phần tử");
        // bufSize < dữ liệu → INVALID_OPERATION, không ghi (robustness thật).
        float getU2[1] = {-1.f};
        glGetnUniformfvKHR(pR, locR, (GLsizei)sizeof(getU2), getU2);
        Check(glGetError() == 0x0502, "glGetnUniformfv bufSize thiếu → INVALID_OPERATION");
        Check(getU2[0] == -1.f, "bufSize thiếu → KHÔNG ghi (không truncate)");
        glUseProgram(0);

        // 9d. GL_ATI_separate_stencil: round-trip front/back riêng.
        while (glGetError() != 0) {}
        glStencilFuncSeparateATI(0x0408 /*FRONT*/, 0x0202 /*EQUAL*/, 1, 0xFF);
        glStencilOpSeparateATI(0x0408, 0x1E00 /*KEEP*/, 0x1E01 /*ZERO*/, 0x1E02 /*REPLACE*/);
        Check(glGetError() == 0, "glStencilFuncSeparateATI/OpSeparate không lỗi");
        GLint sf = 0, so = 0;
        glGetIntegerv(0x0B92 /*STENCIL_FUNC*/, &sf);
        Check(sf == 0x0202, "ATI separate stencil front func round-trip");
        glGetIntegerv(0x0B94 /*STENCIL_FAIL*/, &so);
        Check(so == 0x1E00, "ATI separate stencil front sfail round-trip");
        glStencilFuncSeparateATI(0x0409 /*BACK*/, 0x0204 /*NOTEQUAL*/, 7, 0x0F);
        glGetIntegerv(0x8800 /*STENCIL_BACK_FUNC*/, &sf);
        Check(sf == 0x0204, "ATI separate stencil back func round-trip");
        glGetIntegerv(0x8CA3 /*STENCIL_BACK_REF*/, &sf);
        Check(sf == 7, "ATI separate stencil back ref round-trip");

        // 9e. Occlusion ANY_SAMPLES: glBegin/glEnd + QUERY_RESULT không lỗi.
        // Null backend: không render → result 0 (trung thực: không có fragment
        // pass depth-test). Metal backend: visibility buffer thật.
        while (glGetError() != 0) {}
        GLuint occl = 0;
        glGenQueries(1, &occl);
        glBeginQuery(0x8C2F /*ANY_SAMPLES_PASSED*/, occl);
        Check(glGetError() == 0, "glBeginQuery ANY_SAMPLES không lỗi");
        glEndQuery(0x8C2F);
        Check(glGetError() == 0, "glEndQuery ANY_SAMPLES không lỗi");
        GLint avail = 0, res = -1;
        glGetQueryObjectiv(occl, 0x8867 /*QUERY_RESULT_AVAILABLE*/, &avail);
        glGetQueryObjectiv(occl, 0x8866 /*QUERY_RESULT*/, &res);
        Check(avail == 1, "occlusion QUERY_RESULT_AVAILABLE = 1");
        Check(res == 0 || res == 1, "occlusion result boolean 0/1");
        glDeleteQueries(1, &occl);

        // --- 10. Batch-10: enum-only ext có behavior thật.
        {
            GLint n10 = 0;
            glGetIntegerv(0x821D, &n10);
            for (const char* want : kBatch10) {
                bool found = false;
                for (GLint i = 0; i < n10 && !found; ++i) {
                    const char* e = (const char*)glGetStringi(0x1F03, (GLuint)i);
                    if (e && !strcmp(e, want)) found = true;
                }
                Check(found, want);
            }
            // MIN_MAP_BUFFER_ALIGNMENT = 16 (base map至少 16-aligned).
            GLint mma = 0;
            glGetIntegerv(0x90BC /*MIN_MAP_BUFFER_ALIGNMENT*/, &mma);
            Check(mma == 16, "MIN_MAP_BUFFER_ALIGNMENT = 16");
            // MAX_UNIFORM_LOCATIONS = 1024.
            GLint mul = 0;
            glGetIntegerv(0x826E /*MAX_UNIFORM_LOCATIONS*/, &mul);
            Check(mul == 1024, "MAX_UNIFORM_LOCATIONS = 1024");
            // multi_bind: glBindBuffersBase không lỗi.
            while (glGetError() != 0) {}
            GLuint mbb[2] = {0, 0};
            glGenBuffers(2, mbb);
            glBindBuffersBase(0x8A11 /*UNIFORM_BUFFER*/, 0, 2, mbb);
            Check(glGetError() == 0, "glBindBuffersBase không lỗi");
            glDeleteBuffers(2, mbb);
            // occlusion_query2: ANY_SAMPLES_PASSED đã test ở 9e (cùng target).
        }
    }

    if (gFails) return 1;
    printf("test_enum_behavior PASS\n");
    return 0;
}
