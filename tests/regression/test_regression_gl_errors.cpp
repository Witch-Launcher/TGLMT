// test_regression_gl_errors.cpp — Hồi quy cho các lỗi sinh GL error và
// vòng đời object đã sửa.
//
// Lỗi đã sửa:
//  E1  glGetIntegerv/Floatv/Booleanv/Doublev/Integer64v + các biến thể
//      indexed: KHÔNG sinh GL_INVALID_ENUM với pname lạ (trước đây trả 0
//      im lặng) ⇒ mất cơ chế app dùng để phát hiện driver không tuân thủ.
//  E2  glBindTexture: không kiểm tra target ⇒ target lạ rơi vào slot 2D.
//  E3  glShaderSource với string NULL / count < 0 ⇒ deref NULL (CRASH).
//  E4  glUniform*fv với count < 0 ⇒ std::length_error ⇒ std::terminate (ABORT).
//  E5  glDrawElements với type lạ / mode lạ: trên đường Metal KHÔNG sinh lỗi,
//      draw bị bỏ âm thầm; mode lạ lại vẽ bằng Triangle thay vì bỏ draw.
//  E6  glDeleteBuffers: binding vẫn trỏ tới buffer đã xoá.
//  E7  glDeleteProgram: current program vẫn trỏ tới program đã xoá.
//  E8  glDeleteVertexArrays: VAO đang bind không revert về 0.
//  E9  glDeleteSamplers: sampler đang bind không revert về 0.
//  E10 glBufferData/glBufferStorage trên buffer đang mapped ⇒ con trỏ app giữ
//      bị dangling (use-after-free).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}

// Chạy một hàm và trả về error đầu tiên sinh ra.
template <typename F>
static GLenum FirstError(F fn) {
    DrainErrors();
    fn();
    return glGetError();
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    DrainErrors();

    // ------------------------------------------------------------------
    // E1: pname lạ phải sinh INVALID_ENUM; pname hợp lệ thì không.
    // ------------------------------------------------------------------
    {
        GLint v = 0;
        GLenum e = FirstError([&] { glGetIntegerv(0xDEAD /*không hợp lệ*/, &v); });
        TGLMT_CHECK(e == GL_INVALID_ENUM, "E1a: glGetIntegerv pname lạ → INVALID_ENUM");

        e = FirstError([&] { glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v); });
        TGLMT_CHECK(e == GL_NO_ERROR, "E1b: glGetIntegerv pname hợp lệ → không lỗi");
        TGLMT_CHECK(v > 0, "E1b: MAX_TEXTURE_SIZE phải > 0");

        GLfloat f = 0;
        e = FirstError([&] { glGetFloatv(0xDEAD, &f); });
        TGLMT_CHECK(e == GL_INVALID_ENUM, "E1c: glGetFloatv pname lạ → INVALID_ENUM");

        GLboolean b = 0;
        e = FirstError([&] { glGetBooleanv(0xDEAD, &b); });
        TGLMT_CHECK(e == GL_INVALID_ENUM, "E1d: glGetBooleanv pname lạ → INVALID_ENUM");

        GLdouble d = 0;
        e = FirstError([&] { glGetDoublev(0xDEAD, &d); });
        TGLMT_CHECK(e == GL_INVALID_ENUM, "E1e: glGetDoublev pname lạ → INVALID_ENUM");

        GLint64 i64 = 0;
        e = FirstError([&] { glGetInteger64v(0xDEAD, &i64); });
        TGLMT_CHECK(e == GL_INVALID_ENUM, "E1f: glGetInteger64v pname lạ → INVALID_ENUM");

        GLint iv = 0;
        e = FirstError([&] { glGetIntegeri_v(GL_VIEWPORT, 0, &iv); });
        TGLMT_CHECK(e == GL_NO_ERROR, "E1g: glGetIntegeri_v(VIEWPORT,0) hợp lệ");
    }

    // ------------------------------------------------------------------
    // E2: glBindTexture target lạ → INVALID_ENUM, và không đổi binding.
    // ------------------------------------------------------------------
    {
        GLuint t = 0;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        DrainErrors();
        glBindTexture(0xDEAD, t);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "E2a: glBindTexture target lạ → INVALID_ENUM");
        GLint bound = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
        TGLMT_CHECK((GLuint)bound == t, "E2b: bind target lạ không được đổi binding");
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &t);
    }

    // ------------------------------------------------------------------
    // E3: glShaderSource NULL / count âm → INVALID_VALUE, không crash.
    // ------------------------------------------------------------------
    {
        GLuint s = glCreateShader(GL_VERTEX_SHADER);
        DrainErrors();
        const char* arr[1] = {nullptr};
        glShaderSource(s, 1, arr, nullptr); // phải KHÔNG crash
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E3a: glShaderSource string NULL → INVALID_VALUE");

        DrainErrors();
        glShaderSource(s, -1, arr, nullptr);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E3b: glShaderSource count < 0 → INVALID_VALUE");
        glDeleteShader(s);
    }

    // ------------------------------------------------------------------
    // E4: glUniform*fv count âm → INVALID_VALUE, không abort.
    // ------------------------------------------------------------------
    {
        DrainErrors();
        GLfloat f[4] = {0, 0, 0, 0};
        glUniform1fv(0, -1, f);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E4a: glUniform1fv count < 0 → INVALID_VALUE");
        DrainErrors();
        GLint iv[4] = {0, 0, 0, 0};
        glUniform1iv(0, -1, iv);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E4b: glUniform1iv count < 0 → INVALID_VALUE");
        DrainErrors();
        glUniformMatrix3fv(0, -1, GL_FALSE, (const GLfloat*)nullptr);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E4c: glUniformMatrix3fv count < 0 → INVALID_VALUE");
    }

    // ------------------------------------------------------------------
    // E5: draw với mode/type lạ → INVALID_ENUM và KHÔNG draw.
    // ------------------------------------------------------------------
    {
        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        GLfloat v[8] = {0, 0, 1, 0, 1, 1, 0, 1};
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        ctx.device->clearTrace();
        DrainErrors();
        glDrawArrays(0xDEAD, 0, 3);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "E5a: glDrawArrays mode lạ → INVALID_ENUM");
        TGLMT_CHECK(ctx.device->drawTrace().empty(),
                    "E5b: draw với mode lạ phải bị BỎ, không vẽ bằng Triangle");

        ctx.device->clearTrace();
        DrainErrors();
        glDrawElements(GL_TRIANGLES, 3, 0xDEAD, nullptr);
        TGLMT_CHECK(glGetError() == GL_INVALID_ENUM,
                    "E5c: glDrawElements type lạ → INVALID_ENUM");
        TGLMT_CHECK(ctx.device->drawTrace().empty(),
                    "E5d: draw với type lạ phải bị BỎ");

        // mode hợp lệ vẫn phải vẽ (không hỏng nhầm).
        ctx.device->clearTrace();
        DrainErrors();
        glDrawArrays(GL_TRIANGLES, 0, 3);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "E5e: draw hợp lệ không sinh lỗi");
        TGLMT_CHECK(ctx.device->drawTrace().size() == 1, "E5f: draw hợp lệ phải được encode");

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
    }

    // ------------------------------------------------------------------
    // E6: glDeleteBuffers phải unbind buffer khỏi mọi binding point.
    // ------------------------------------------------------------------
    {
        GLuint b = 0;
        glGenBuffers(1, &b);
        glBindBuffer(GL_ARRAY_BUFFER, b);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b);
        glBindBuffer(GL_UNIFORM_BUFFER, b);
        DrainErrors();
        glDeleteBuffers(1, &b);
        GLint bound = -1;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
        TGLMT_CHECK(bound == 0, "E6a: xoá buffer → ARRAY_BUFFER_BINDING = 0");
        bound = -1;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bound);
        TGLMT_CHECK(bound == 0, "E6b: xoá buffer → ELEMENT_ARRAY_BUFFER_BINDING = 0");
        bound = -1;
        glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &bound);
        TGLMT_CHECK(bound == 0, "E6c: xoá buffer → UNIFORM_BUFFER_BINDING = 0");
    }

    // ------------------------------------------------------------------
    // E7: glDeleteProgram phải revert current program về 0.
    // ------------------------------------------------------------------
    {
        GLuint p = glCreateProgram();
        glUseProgram(p);
        GLint cur = -1;
        glGetIntegerv(GL_CURRENT_PROGRAM, &cur);
        TGLMT_CHECK((GLuint)cur == p, "E7a: current program đã set");
        glDeleteProgram(p);
        glGetIntegerv(GL_CURRENT_PROGRAM, &cur);
        TGLMT_CHECK(cur == 0, "E7b: xoá program đang dùng → CURRENT_PROGRAM = 0");
    }

    // ------------------------------------------------------------------
    // E8: glDeleteVertexArrays phải revert VAO đang bind về 0.
    // ------------------------------------------------------------------
    {
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glDeleteVertexArrays(1, &vao);
        GLint bound = -1;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
        TGLMT_CHECK(bound == 0, "E8: xoá VAO đang bind → VERTEX_ARRAY_BINDING = 0");
    }

    // ------------------------------------------------------------------
    // E9: glDeleteSamplers phải revert sampler đang bind về 0.
    // ------------------------------------------------------------------
    {
        GLuint s = 0;
        glGenSamplers(1, &s);
        glBindSampler(0, s);
        glDeleteSamplers(1, &s);
        GLint bound = -1;
        glGetIntegerv(GL_SAMPLER_BINDING, &bound);
        TGLMT_CHECK(bound == 0, "E9: xoá sampler đang bind → SAMPLER_BINDING = 0");
    }

    // ------------------------------------------------------------------
    // E10: không được thay storage của buffer đang mapped.
    // ------------------------------------------------------------------
    {
        GLuint b = 0;
        glGenBuffers(1, &b);
        glBindBuffer(GL_ARRAY_BUFFER, b);
        glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW);
        void* p = glMapBufferRange(GL_ARRAY_BUFFER, 0, 64, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);
        TGLMT_CHECK(p != nullptr, "E10a: map buffer phải trả con trỏ hợp lệ");
        if (p) {
            DrainErrors();
            glBufferData(GL_ARRAY_BUFFER, 128, nullptr, GL_STATIC_DRAW);
            TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                        "E10b: glBufferData trên buffer đang mapped → INVALID_OPERATION");
            DrainErrors();
            glBufferStorage(GL_ARRAY_BUFFER, 128, nullptr, GL_MAP_READ_BIT);
            TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                        "E10c: glBufferStorage trên buffer đang mapped → INVALID_OPERATION");
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDeleteBuffers(1, &b);
    }

    // ------------------------------------------------------------------
    // E11: các hàm Invalidate* phải sinh error bắt buộc theo GL 4.6
    //      §8.20 / §9.4. Trước đây chúng là no-op hoàn toàn ⇒ không có
    //      lỗi nào dù tham số sai (và bị "triage" nhầm là hợp lệ).
    // ------------------------------------------------------------------
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        DrainErrors();
        glInvalidateTexImage(0, 0);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "E11a: InvalidateTexImage(0) → INVALID_OPERATION");
        DrainErrors();
        glInvalidateTexImage(0xDEADBEEF, 0);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "E11b: InvalidateTexImage(name không tồn tại) → INVALID_OPERATION");
        DrainErrors();
        glInvalidateTexImage(tex, -1);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E11c: InvalidateTexImage(level<0) → INVALID_VALUE");
        DrainErrors();
        glInvalidateTexSubImage(tex, 0, -1, 0, 0, 4, 4, 1);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E11d: InvalidateTexSubImage(xoffset<0) → INVALID_VALUE");

        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

        DrainErrors();
        glInvalidateNamedFramebufferData(0xDEADBEEF, 1, nullptr);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "E11e: InvalidateNamedFramebufferData(FBO không tồn tại) → INVALID_OPERATION");
        DrainErrors();
        glInvalidateNamedFramebufferSubData(fbo, 1, nullptr, -1, 0, 4, 4);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E11f: InvalidateNamedFramebufferSubData(x<0) → INVALID_VALUE");
        DrainErrors();
        glInvalidateSubFramebuffer(0xDEAD, 1, nullptr, 0, 0, 4, 4);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "E11g: InvalidateSubFramebuffer(target lạ) → INVALID_OPERATION");
        DrainErrors();
        glInvalidateSubFramebuffer(GL_FRAMEBUFFER, 1, nullptr, 0, 0, -4, 4);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "E11h: InvalidateSubFramebuffer(w<0) → INVALID_VALUE");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &tex);
    }

    return TGLMT_RESULT;
}
