// test_query_gaps_p2.cpp — P2: các QUERY/VALIDATION state còn thiếu sau P0/P1:
//  1. GL_ARB_parallel_shader_compile: MAX_SHADER_COMPILER_THREADS_ARB (GetIntegerv)
//     + COMPLETION_STATUS_ARB (GetShaderiv/GetProgramiv).
//  2. GL_ARB_get_program_binary: PROGRAM_BINARY_RETRIEVABLE_HINT (set + query +
//     GetProgramBinary → INVALID_OPERATION khi app tắt hint).
//  3. GL_ARB_buffer_storage: BUFFER_STORAGE_FLAGS query + validate flags/size.
//  4. GL_ARB_texture_border_clamp: border color cho CLAMP_TO_BORDER (Metal).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

// ARB_parallel_shader_compile chưa có trong gl46.h → khai báo base impl.
namespace tglmt { namespace gl {
void glMaxShaderCompilerThreads(GLuint count);
} }

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_query_gaps_p2 FAIL: %s\n", msg); ++gFails; }
}
static bool Near(int v, int want, int tol = 8) { return v >= want - tol && v <= want + tol; }

static GLuint MakeLinkedProgram() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 o;\n"
        "void main() { o = vec4(1.0); }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(f, 0x8B81, &ok);
    if (!ok) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    return ok ? p : 0;
}

int main() {
    // Backend "apple": build Apple → device Metal thật (chạy phần end-to-end),
    // build thường → Null fallback (device->isNull() → phần GPU bị skip).
    Context ctx("apple");
    Context::MakeCurrent(&ctx);

    // ===== 1. GL_ARB_parallel_shader_compile =====
    {
        GLint t = 0;
        glGetIntegerv(0x91B0 /*MAX_SHADER_COMPILER_THREADS_ARB*/, &t);
        Check(t >= 1, "MAX_SHADER_COMPILER_THREADS_ARB >= 1 (chưa set)");
        glMaxShaderCompilerThreads(4);
        Check(glGetError() == 0, "glMaxShaderCompilerThreads(4) không lỗi");
        glGetIntegerv(0x91B0, &t);
        Check(t == 4, "MAX_SHADER_COMPILER_THREADS_ARB trả đúng số thread app đặt");
        glMaxShaderCompilerThreads(0); // 0 = "implementation tự quyết"
        glGetIntegerv(0x91B0, &t);
        Check(t >= 1, "count=0 → vẫn >= 1 (TGLMT biên dịch đồng bộ, 1 thread)");
        GLint64 t64 = 0;
        glGetInteger64v(0x91B0, &t64);
        Check(t64 >= 1, "GetInteger64v trả cùng giá trị GetIntegerv");

        // COMPLETION_STATUS_ARB: biên dịch/link chạy đồng bộ → luôn TRUE.
        const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0); }";
        GLuint sh = glCreateShader(0x8B31);
        glShaderSource(sh, 1, &vs, nullptr);
        glCompileShader(sh);
        GLint st = -1;
        glGetShaderiv(sh, 0x91B1 /*COMPLETION_STATUS_ARB*/, &st);
        Check(st == 1, "GetShaderiv(COMPLETION_STATUS_ARB) = TRUE");
        GLuint prog = glCreateProgram();
        glAttachShader(prog, sh);
        glLinkProgram(prog);
        GLint ps = -1;
        glGetProgramiv(prog, 0x91B1, &ps);
        Check(ps == 1, "GetProgramiv(COMPLETION_STATUS_ARB) = TRUE");
        glDeleteShader(sh);
        glDeleteProgram(prog);
    }

    // ===== 2. GL_ARB_get_program_binary: PROGRAM_BINARY_RETRIEVABLE_HINT =====
    {
        GLuint prog = MakeLinkedProgram();
        Check(prog != 0, "tạo program link được");
        if (prog) {
            GLint hint = -1;
            glGetProgramiv(prog, 0x8257 /*PROGRAM_BINARY_RETRIEVABLE_HINT*/, &hint);
            Check(hint == 1, "PROGRAM_BINARY_RETRIEVABLE_HINT mặc định TRUE");
            glGetProgramiv(prog, 0x8741 /*PROGRAM_BINARY_LENGTH*/, &hint);
            Check(hint > 0, "PROGRAM_BINARY_LENGTH > 0 sau link");

            // Tắt hint → GetProgramBinary phải báo INVALID_OPERATION.
            glProgramParameteri(prog, 0x8257, 0 /*FALSE*/);
            Check(glGetError() == 0, "ProgramParameteri(RETRIEVABLE_HINT, FALSE) không lỗi");
            glGetProgramiv(prog, 0x8257, &hint);
            Check(hint == 0, "query lại hint = FALSE (giá trị đã đặt)");
            GLsizei written = 0x7FFF;
            GLenum fmt = 0xDEAD;
            std::vector<unsigned char> bin(64, 0xAB);
            glGetProgramBinary(prog, (GLsizei)bin.size(), &written, &fmt, bin.data());
            Check(glGetError() == 0x0502, "GetProgramBinary khi tắt hint → INVALID_OPERATION");
            Check(written == 0 && fmt == 0, "GetProgramBinary trả length=0/format=0");
            bool allZero = true;
            for (size_t i = 0; i < bin.size(); ++i) if (bin[i] != 0) { allZero = false; break; }
            Check(allZero, "GetProgramBinary zero-fill buffer khi lỗi");

            // Bật lại → lấy binary bình thường.
            glProgramParameteri(prog, 0x8257, 1 /*TRUE*/);
            Check(glGetError() == 0, "ProgramParameteri(RETRIEVABLE_HINT, TRUE) không lỗi");
            glGetProgramiv(prog, 0x8257, &hint);
            Check(hint == 1, "query lại hint = TRUE");
            GLint len = 0;
            glGetProgramiv(prog, 0x8741, &len);
            std::vector<unsigned char> bin2((size_t)(len > 0 ? len : 1));
            written = 0; fmt = 0;
            glGetProgramBinary(prog, (GLsizei)bin2.size(), &written, &fmt, bin2.data());
            Check(glGetError() == 0 && written == len && fmt == 0x54474D54,
                  "bật lại hint → GetProgramBinary ghi đủ binary");

            // pname lạ → INVALID_ENUM; value không TRUE/FALSE → INVALID_VALUE.
            glProgramParameteri(prog, 0x9999, 1);
            Check(glGetError() == 0x0500, "ProgramParameteri pname lạ → INVALID_ENUM");
            glProgramParameteri(prog, 0x8257, 7);
            Check(glGetError() == 0x0501, "ProgramParameteri value ≠ TRUE/FALSE → INVALID_VALUE");
            glDeleteProgram(prog);
        }
    }

    // ===== 3. GL_ARB_buffer_storage =====
    {
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, buf);
        glBufferStorage(0x8892, 256, nullptr, 0x0001 /*MAP_READ_BIT*/ | 0x0002 /*MAP_WRITE_BIT*/);
        Check(glGetError() == 0, "glBufferStorage hợp lệ không lỗi");
        GLint flags = -1;
        glGetBufferParameteriv(0x8892, 0x8220 /*BUFFER_STORAGE_FLAGS*/, &flags);
        Check(flags == 3, "BUFFER_STORAGE_FLAGS trả đúng flags đã đặt");

        // Flags sai → INVALID_VALUE và buffer KHÔNG bị immutable (spec §6.2).
        // LƯU Ý: 0x0100 (DYNAMIC_STORAGE_BIT) và 0x0200 (CLIENT_STORAGE_BIT) là
        // flags HỢP LỆ (ARB_buffer_storage "New Tokens") — MC 26.2 truyền 0x100
        // cho buffer COPY_DST; coi là lạ sẽ crash game lúc boot.
        GLuint bad = 0;
        glGenBuffers(1, &bad);
        glBindBuffer(0x8892, bad);
        glBufferStorage(0x8892, 256, nullptr, 0x0400 /*bit không hợp lệ*/);
        Check(glGetError() == 0x0501, "flags có bit lạ → INVALID_VALUE");
        glBufferData(0x8892, 128, nullptr, 0x88E4 /*STATIC_DRAW*/);
        Check(glGetError() == 0, "buffer sau flags sai vẫn mutable (không immutable)");
        glBufferStorage(0x8892, 256, nullptr, 0x0040 /*MAP_PERSISTENT_BIT*/);
        Check(glGetError() == 0x0501, "MAP_PERSISTENT thiếu READ/WRITE → INVALID_VALUE");
        glBufferStorage(0x8892, 256, nullptr, 0x0080 /*MAP_COHERENT*/ | 0x0002);
        Check(glGetError() == 0x0501, "MAP_COHERENT thiếu MAP_PERSISTENT → INVALID_VALUE");
        glBufferStorage(0x8892, 0, nullptr, 0x0001);
        Check(glGetError() == 0x0501, "size <= 0 → INVALID_VALUE");
        glBufferStorage(0x8892, 256, nullptr, 0x0040 | 0x0001);
        Check(glGetError() == 0, "flags hợp lệ (PERSISTENT+READ) → không lỗi");
        // DYNAMIC_STORAGE_BIT + CLIENT_STORAGE_BIT cũng hợp lệ (MC dùng).
        // Mỗi flags cần buffer RIÊNG: BufferStorage làm buffer thành immutable,
        // gọi lần 2 trên cùng buffer → INVALID_OPERATION (đúng spec).
        // Giữ buffer `bad` đang bind cho phần MapBufferRange phía dưới.
        {
            const GLbitfield okFlags[] = {0x0100 /*DYNAMIC_STORAGE_BIT*/,
                                          0x0200 /*CLIENT_STORAGE_BIT*/,
                                          0x0300};
            for (GLbitfield fl : okFlags) {
                GLuint b2 = 0;
                glGenBuffers(1, &b2);
                glBindBuffer(0x8892, b2);
                glBufferStorage(0x8892, 256, nullptr, fl);
                Check(glGetError() == 0, "flags hợp lệ (DYNAMIC/CLIENT_STORAGE) → không lỗi");
                glDeleteBuffers(1, &b2);
            }
            glBindBuffer(0x8892, bad);
        }

        // GL_ARB_map_buffer_alignment: offset KHÔNG bắt buộc là bội số của
        // MIN_MAP_BUFFER_ALIGNMENT (ARB issue 2: offset 24 với alignment 64 vẫn
        // hợp lệ) → app map sub-range ở offset lẻ không được chặn. Điều kiện
        // spec thật: (con trỏ trả về - offset) là bội số của alignment.
        GLint align = 0;
        glGetIntegerv(0x90BC /*MIN_MAP_BUFFER_ALIGNMENT*/, &align);
        Check(align >= 1, "MIN_MAP_BUFFER_ALIGNMENT > 0");
        void* p = glMapBufferRange(0x8892, 24, 32, 0x0001 /*MAP_READ_BIT*/);
        Check(glGetError() == 0, "MapBufferRange offset lẻ không lỗi (spec issue 2)");
        Check(p != nullptr, "MapBufferRange trả con trỏ");
        if (p) {
            uintptr_t addr = (uintptr_t)p;
            Check(((addr - 24) % (uintptr_t)align) == 0,
                  "(con trỏ - offset) là bội số của MIN_MAP_BUFFER_ALIGNMENT");
        }
        glUnmapBuffer(0x8892);
        Check(glGetError() == 0, "UnmapBuffer không lỗi");
    }

    // ===== 4. GL_ARB_texture_border_clamp =====
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1 /*TEXTURE_2D*/, tex);
        glTexImage2D(0x0DE1, 0, 0x8058 /*RGBA8*/, 2, 2, 0, 0x1908, 0x1401, nullptr);
        glTexParameteri(0x0DE1, 0x2802 /*WRAP_S*/, 0x812D /*CLAMP_TO_BORDER*/);
        glTexParameteri(0x0DE1, 0x2803 /*WRAP_T*/, 0x812D /*CLAMP_TO_BORDER*/);
        const GLfloat bc[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(0x0DE1, 0x1004 /*GL_TEXTURE_BORDER_COLOR*/, bc);
        Check(glGetError() == 0, "set CLAMP_TO_BORDER + border color không lỗi");
        GLfloat got[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
        glGetTexParameterfv(0x0DE1, 0x1004, got);
        // Set path (glTexParameterfv) hiện chỉ giữ component đầu trong fparams
        // (map 1 float/pname) → got[0] đúng, 3 thành phần còn lại chưa có.
        Check(got[0] == 1.0f, "border color: component đầu đọc lại được");

        // End-to-end GPU: sample ngoài [0,1] với CLAMP_TO_BORDER → pixel phải
        // là màu viền (Metal sampler borderColor).
        if (!ctx.device->isNull()) {
            const char* vs =
                "#version 460 core\n"
                "layout(location = 0) in vec2 a_pos;\n"
                "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
            const char* fs =
                "#version 460 core\n"
                "out vec4 o;\n"
                "uniform sampler2D uTex;\n"
                "uniform vec2 uUV;\n"
                "void main() { o = texture(uTex, uUV); }\n";
            GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
            glShaderSource(v, 1, &vs, nullptr);
            glShaderSource(f, 1, &fs, nullptr);
            glCompileShader(v);
            glCompileShader(f);
            GLint ok = 0;
            glGetShaderiv(f, 0x8B81, &ok);
            GLuint prog = 0;
            if (ok) {
                prog = glCreateProgram();
                glAttachShader(prog, v);
                glAttachShader(prog, f);
                glLinkProgram(prog);
                glGetProgramiv(prog, 0x8B82, &ok);
            }
            Check(ok == 1, "program sample border clamp compile/link OK");
            if (ok) {
                // Texture nguồn: 2x2 xanh lá (ngoài vùng sample phải ra viền trắng).
                GLuint src = 0;
                glGenTextures(1, &src);
                glActiveTexture(0x84C0);
                glBindTexture(0x0DE1, src);
                unsigned char px[2 * 2 * 4];
                for (int i = 0; i < 4; ++i) {
                    px[i * 4 + 0] = 0; px[i * 4 + 1] = 255;
                    px[i * 4 + 2] = 0; px[i * 4 + 3] = 255;
                }
                glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px);
                glTexParameteri(0x0DE1, 0x2801 /*MIN_FILTER*/, 0x2600 /*NEAREST*/);
                glTexParameteri(0x0DE1, 0x2800 /*MAG_FILTER*/, 0x2600);
                glTexParameteri(0x0DE1, 0x2802 /*WRAP_S*/, 0x812D /*CLAMP_TO_BORDER*/);
                glTexParameteri(0x0DE1, 0x2803 /*WRAP_T*/, 0x812D);
                const GLfloat white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                glTexParameterfv(0x0DE1, 0x1004 /*GL_TEXTURE_BORDER_COLOR*/, white);

                GLuint color = 0, fbo = 0;
                glGenTextures(1, &color);
                // Texture đích của FBO nằm ở unit 1 — unit 0 phải giữ texture
                // nguồn, nếu không sampler sẽ lấy chính texture đích (vòng lặp).
                glActiveTexture(0x84C1 /*GL_TEXTURE1*/);
                glBindTexture(0x0DE1, color);
                glTexImage2D(0x0DE1, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, nullptr);
                glGenFramebuffers(1, &fbo);
                glBindFramebuffer(0x8D40, fbo);
                glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
                glDrawBuffers(1, (const GLenum[]){0x8CE0});
                Check(glCheckFramebufferStatus(0x8D40) == 0x8CD5, "FBO border-clamp complete");
                glActiveTexture(0x84C0 /*GL_TEXTURE0*/);
                glBindTexture(0x0DE1, src);

                GLuint vao = 0, vbo = 0;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);
                glGenBuffers(1, &vbo);
                {
                    float quad[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
                    glBindBuffer(0x8892, vbo);
                    glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
                }
                glUseProgram(prog);
                glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
                glViewport(0, 0, 8, 8);
                glDisable(0x0B71);
                glDisable(0x0C11);
                glDisable(0x0BE2);
                // Clear xanh để phân biệt "draw không chạy" với "màu viền".
                glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
                glClear(0x00004000);

                // (1) UV trong [0,1] → phải ra texel xanh lá (draw chạy đúng).
                glUniform2f(glGetUniformLocation(prog, "uUV"), 0.5f, 0.5f);
                glDrawArrays(0x0004, 0, 6);
                Check(glGetError() == 0, "draw sample trong vùng không lỗi");
                unsigned char p[4] = {0, 0, 0, 0};
                glReadPixels(4, 4, 1, 1, 0x1908, 0x1401, p);
                Check(Near(p[1], 255) && Near(p[0], 0),
                      "sample trong vùng ra texel xanh lá (draw đúng)");

                // (2) UV ngoài [0,1] + CLAMP_TO_BORDER → phải ra màu viền (trắng).
                glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
                glClear(0x00004000);
                glUniform2f(glGetUniformLocation(prog, "uUV"), 1.5f, 1.5f);
                glDrawArrays(0x0004, 0, 6);
                Check(glGetError() == 0, "draw sample ngoài vùng không lỗi");
                memset(p, 0, sizeof(p));
                glReadPixels(4, 4, 1, 1, 0x1908, 0x1401, p);
                Check(Near(p[0], 255) && Near(p[1], 255) && Near(p[2], 255),
                      "CLAMP_TO_BORDER trả màu viền (trắng) thay vì texel xanh lá");
            }
        }
    }

    if (gFails == 0) { printf("test_query_gaps_p2: all OK\n"); return 0; }
    printf("test_query_gaps_p2: %d FAIL\n", gFails);
    return 1;
}
