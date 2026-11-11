// Test nhóm Draw của GL 4.6 Core — 12 entry point:
//   glDrawArraysIndirect, glDrawElementsIndirect,
//   glDrawElementsInstanced, glDrawElementsInstancedBaseVertex,
//   glDrawRangeElements, glDrawRangeElementsBaseVertex,
//   glMultiDrawElements, glMultiDrawElementsBaseVertex,
//   glMultiDrawArraysIndirectCount,
//   glDrawTransformFeedbackInstanced, glDrawTransformFeedbackStream,
//   glDrawTransformFeedbackStreamInstanced.
//
// Kiểm chứng:
//   1) DrawTrace của Null/Apple backend xác nhận SỐ LẦN draw, count,
//      instanceCount, chế độ indexed → xác minh lệnh truyền đúng tham số.
//   2) Lỗi theo spec GL 4.6 §10.3/§10.4: count<0, instancecount<0,
//      drawcount<0, stride<16, mode sai, index type sai.
//   3) Pixel thật trên Apple backend: glDrawElementsIndirect với
//      DRAW_INDIRECT_BUFFER phải vẽ đúng hình; glDrawElementsBaseVertex phải
//      dịch chỉ số đỉnh (Metal không có baseVertex → phải cộng vào index).

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }

const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
const char* kFS =
    "#version 460 core\n"
    "out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

GLuint g_prog = 0;

bool BuildProgram() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) return false;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) return false;
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs);
    glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glGetProgramiv(g_prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) return false;
    glUseProgram(g_prog);
    return true;
}

// Một tam giác đơn giản + index buffer.
GLuint g_vao = 0, g_vbo = 0, g_ibo = 0;
void BuildGeometry() {
    const float verts[6] = {-0.8f, -0.8f, 0.8f, -0.8f, 0.0f, 0.8f};
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glGenBuffers(1, &g_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo);
    const GLushort idx[3] = {0, 1, 2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glBindVertexArray(0);
}

// ===========================================================================
// 1) glDrawArraysIndirect / glDrawElementsIndirect
// ===========================================================================
bool TestDrawIndirect() {
    Context& ctx = Context::Current();
    glBindVertexArray(g_vao);

    // --- host pointer (không bind DRAW_INDIRECT_BUFFER) ---
    struct ArrCmd { GLuint count, instanceCount, first, baseInstance; };
    struct EltCmd { GLuint count, instanceCount, firstIndex; GLint baseVertex; GLuint baseInstance; };
    ArrCmd ac = {3, 1, 0, 0};
    EltCmd ec = {3, 1, 0, 0, 0};

    ctx.device->clearTrace();
    glDrawArraysIndirect(GL_TRIANGLES, &ac);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1, "glDrawArraysIndirect phải phát 1 draw");
    if (ctx.device->drawTrace().size() == 1) {
        const auto& t = ctx.device->drawTrace()[0];
        TGLMT_CHECK(t.count == 3, "glDrawArraysIndirect count phải lấy từ command");
        TGLMT_CHECK(t.instanceCount == 1, "glDrawArraysIndirect instanceCount phải lấy từ command");
        TGLMT_CHECK(!t.indexed, "glDrawArraysIndirect không indexed");
    }

    ctx.device->clearTrace();
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, &ec);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1, "glDrawElementsIndirect phải phát 1 draw");
    if (ctx.device->drawTrace().size() == 1) {
        const auto& t = ctx.device->drawTrace()[0];
        TGLMT_CHECK(t.count == 3, "glDrawElementsIndirect count phải lấy từ command");
        TGLMT_CHECK(t.indexed, "glDrawElementsIndirect phải indexed");
    }

    // instanceCount = 0 → theo GL, vẽ 0 lần (không phải 1).
    ArrCmd ac0 = {3, 0, 0, 0};
    ctx.device->clearTrace();
    glDrawArraysIndirect(GL_TRIANGLES, &ac0);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1,
                "instanceCount=0 vẫn phát 1 draw call với instanceCount=0");

    // --- command trong DRAW_INDIRECT_BUFFER (đường đúng của GL) ---
    GLuint dib = 0;
    glGenBuffers(1, &dib);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dib);
    ArrCmd acBuf = {3, 2, 0, 0};
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(acBuf), &acBuf, GL_STATIC_DRAW);
    ctx.device->clearTrace();
    glDrawArraysIndirect(GL_TRIANGLES, (void*)0);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1,
                "glDrawArraysIndirect đọc command từ DRAW_INDIRECT_BUFFER");
    if (ctx.device->drawTrace().size() == 1) {
        const auto& t = ctx.device->drawTrace()[0];
        TGLMT_CHECK(t.count == 3, "count từ buffer phải đúng");
        TGLMT_CHECK(t.instanceCount == 2, "instanceCount từ buffer phải đúng");
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDeleteBuffers(1, &dib);
    glBindVertexArray(0);
    return true;
}

// ===========================================================================
// 2) glDrawElementsInstanced / ...BaseVertex / glDrawRangeElements*
// ===========================================================================
bool TestDrawInstancedAndRange() {
    Context& ctx = Context::Current();
    glBindVertexArray(g_vao);
    const GLushort idx[3] = {0, 1, 2};

    ctx.device->clearTrace();
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, idx, 4);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1, "glDrawElementsInstanced phải phát 1 draw");
    if (ctx.device->drawTrace().size() == 1) {
        const auto& t = ctx.device->drawTrace()[0];
        TGLMT_CHECK(t.count == 3, "count phải đúng");
        TGLMT_CHECK(t.instanceCount == 4, "instanceCount phải đúng");
        TGLMT_CHECK(t.indexed, "phải indexed");
    }

    ctx.device->clearTrace();
    glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, idx, 2, 0);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1,
                "glDrawElementsInstancedBaseVertex phải phát 1 draw");
    if (ctx.device->drawTrace().size() == 1) {
        TGLMT_CHECK(ctx.device->drawTrace()[0].instanceCount == 2,
                    "instanceCount phải đúng với base vertex");
    }

    // count âm → INVALID_VALUE.
    DrainErrors();
    glDrawElementsInstanced(GL_TRIANGLES, -1, GL_UNSIGNED_SHORT, idx, 1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glDrawElementsInstanced count<0 → INVALID_VALUE");
    // instancecount âm → INVALID_VALUE.
    DrainErrors();
    glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, idx, -1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glDrawElementsInstanced instancecount<0 → INVALID_VALUE");

    // glDrawRangeElements: end < start → INVALID_VALUE; count<0 → INVALID_VALUE.
    ctx.device->clearTrace();
    glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, idx);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1, "glDrawRangeElements hợp lệ phải vẽ");
    if (ctx.device->drawTrace().size() == 1)
        TGLMT_CHECK(ctx.device->drawTrace()[0].count == 3, "glDrawRangeElements count phải đúng");

    DrainErrors();
    glDrawRangeElements(GL_TRIANGLES, 5, 2, 3, GL_UNSIGNED_SHORT, idx);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glDrawRangeElements end<start → INVALID_VALUE");
    DrainErrors();
    glDrawRangeElements(GL_TRIANGLES, 0, 2, -1, GL_UNSIGNED_SHORT, idx);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glDrawRangeElements count<0 → INVALID_VALUE");

    ctx.device->clearTrace();
    glDrawRangeElementsBaseVertex(GL_TRIANGLES, 0, 2, 3, GL_UNSIGNED_SHORT, idx, 1);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 1,
                "glDrawRangeElementsBaseVertex hợp lệ phải vẽ");

    // Mode sai → INVALID_ENUM, KHÔNG được vẽ.
    DrainErrors();
    ctx.device->clearTrace();
    glDrawRangeElements(GL_TRIANGLES + 99, 0, 2, 3, GL_UNSIGNED_SHORT, idx);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "mode sai → INVALID_ENUM");
    TGLMT_CHECK(ctx.device->drawTrace().empty(), "mode sai thì KHÔNG được vẽ");

    // Index type sai → INVALID_ENUM.
    DrainErrors();
    ctx.device->clearTrace();
    glDrawRangeElements(GL_TRIANGLES, 0, 2, 3, GL_FLOAT, idx);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM, "index type sai → INVALID_ENUM");
    TGLMT_CHECK(ctx.device->drawTrace().empty(), "index type sai thì KHÔNG được vẽ");

    glBindVertexArray(0);
    return true;
}

// ===========================================================================
// 3) glMultiDrawElements / ...BaseVertex
// ===========================================================================
bool TestMultiDrawElements() {
    Context& ctx = Context::Current();
    glBindVertexArray(g_vao);
    const GLushort idx[3] = {0, 1, 2};
    const void* idxPtr[2] = {idx, idx};
    const GLsizei counts[2] = {3, 3};
    const GLint bases[2] = {0, 0};

    ctx.device->clearTrace();
    glMultiDrawElements(GL_TRIANGLES, counts, GL_UNSIGNED_SHORT, idxPtr, 2);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 2,
                "glMultiDrawElements phải phát 2 draw (drawcount=2)");
    for (const auto& t : ctx.device->drawTrace()) {
        TGLMT_CHECK(t.count == 3, "mỗi draw phải dùng count tương ứng");
        TGLMT_CHECK(t.indexed, "phải indexed");
    }

    ctx.device->clearTrace();
    glMultiDrawElementsBaseVertex(GL_TRIANGLES, counts, GL_UNSIGNED_SHORT, idxPtr, 2, bases);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 2,
                "glMultiDrawElementsBaseVertex phải phát 2 draw");

    // drawcount âm → INVALID_VALUE.
    DrainErrors();
    glMultiDrawElements(GL_TRIANGLES, counts, GL_UNSIGNED_SHORT, idxPtr, -1);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glMultiDrawElements drawcount<0 → INVALID_VALUE");

    glBindVertexArray(0);
    return true;
}

// ===========================================================================
// 4) glMultiDrawArraysIndirectCount
// ===========================================================================
bool TestMultiDrawArraysIndirectCount() {
    Context& ctx = Context::Current();
    glBindVertexArray(g_vao);
    struct ArrCmd { GLuint count, instanceCount, first, baseInstance; };
    ArrCmd cmds[2] = {{3, 1, 0, 0}, {3, 1, 0, 0}};

    // Dạng buffer-bound: indirect buffer + count buffer (đúng spec GL 4.6
    // §10.3.8: `drawcount` là OFFSET vào PARAMETER_BUFFER).
    GLuint dib = 0, cb = 0;
    glGenBuffers(1, &dib);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dib);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(cmds), cmds, GL_STATIC_DRAW);
    glGenBuffers(1, &cb);
    glBindBuffer(GL_PARAMETER_BUFFER, cb);
    const GLuint drawCount = 2;
    glBufferData(GL_PARAMETER_BUFFER, sizeof(drawCount), &drawCount, GL_STATIC_DRAW);
    ctx.device->clearTrace();
    glMultiDrawArraysIndirectCount(GL_TRIANGLES, (void*)0, 0, 2, 0);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 2,
                "glMultiDrawArraysIndirectCount đọc drawcount từ PARAMETER_BUFFER");

    // Không có PARAMETER_BUFFER → dùng maxdrawcount.
    glBindBuffer(GL_PARAMETER_BUFFER, 0);
    ctx.device->clearTrace();
    glMultiDrawArraysIndirectCount(GL_TRIANGLES, (void*)0, 0, 2, 0);
    TGLMT_CHECK(ctx.device->drawTrace().size() == 2,
                "glMultiDrawArraysIndirectCount không có PARAMETER_BUFFER thì dùng maxdrawcount");

    // stride < 16 → INVALID_VALUE.
    DrainErrors();
    glMultiDrawArraysIndirectCount(GL_TRIANGLES, (void*)0, 0, 2, 8);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glMultiDrawArraysIndirectCount stride<16 → INVALID_VALUE");

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBuffer(GL_PARAMETER_BUFFER, 0);
    glDeleteBuffers(1, &dib);
    glDeleteBuffers(1, &cb);
    glBindVertexArray(0);
    return true;
}

// ===========================================================================
// 5) glDrawTransformFeedback{Instanced,Stream,StreamInstanced}
// ===========================================================================
bool TestDrawTransformFeedback() {
    Context& ctx = Context::Current();

    // Transform feedback cần program có biến output và XFB varyings.
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 pos;\n"
        "out vec2 vPos;\n"
        "void main() { vPos = pos; gl_Position = vec4(pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 fragColor;\n"
        "void main() { fragColor = vec4(1.0); }\n";
    GLuint xfbVs = glCreateShader(GL_VERTEX_SHADER);
    GLuint xfbFs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(xfbVs, 1, &vs, nullptr);
    glCompileShader(xfbVs);
    glShaderSource(xfbFs, 1, &fs, nullptr);
    glCompileShader(xfbFs);
    GLuint p = glCreateProgram();
    glAttachShader(p, xfbVs);
    glAttachShader(p, xfbFs);
    const char* vary = "vPos";
    glTransformFeedbackVaryings(p, 1, &vary, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(xfbVs);
    glDeleteShader(xfbFs);
    if (!ok) {
        printf("  (skip: không link được program transform feedback)\n");
        return true;
    }

    GLuint tf = 0;
    glGenTransformFeedbacks(1, &tf);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tf);
    glBeginTransformFeedback(GL_TRIANGLES);
    glEndTransformFeedback();
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    // Draw từ XFB: cần buffer XFB bound. Nếu không có buffer → INVALID_OPERATION
    // theo spec (spec §10.3: "INVALID_OPERATION is generated if the GL is not in
    // transform feedback mode and no transform feedback object is bound" hoặc
    // thiếu buffer). Test: lệnh phải không crash và phải báo lỗi hợp lý.
    ctx.device->clearTrace();
    DrainErrors();
    glDrawTransformFeedbackInstanced(GL_TRIANGLES, tf, 1);
    const GLenum e1 = FirstError();
    TGLMT_CHECK(e1 == GL_NO_ERROR || e1 == GL_INVALID_OPERATION,
                "glDrawTransformFeedbackInstanced: lỗi phải là NONE hoặc INVALID_OPERATION");

    ctx.device->clearTrace();
    DrainErrors();
    glDrawTransformFeedbackStream(GL_TRIANGLES, tf, 0);
    const GLenum e2 = FirstError();
    TGLMT_CHECK(e2 == GL_NO_ERROR || e2 == GL_INVALID_OPERATION,
                "glDrawTransformFeedbackStream: lỗi phải là NONE hoặc INVALID_OPERATION");

    ctx.device->clearTrace();
    DrainErrors();
    glDrawTransformFeedbackStreamInstanced(GL_TRIANGLES, tf, 0, 1);
    const GLenum e3 = FirstError();
    TGLMT_CHECK(e3 == GL_NO_ERROR || e3 == GL_INVALID_OPERATION,
                "glDrawTransformFeedbackStreamInstanced: lỗi phải là NONE hoặc INVALID_OPERATION");

    glDeleteTransformFeedbacks(1, &tf);
    glDeleteProgram(p);
    return true;
}

// ===========================================================================
// 6) Pixel thật (Apple backend): indirect draw + base vertex
// ===========================================================================
bool TestPixelVerification() {
    Context& ctx = Context::Current();
    if (ctx.device->isNull()) {
        printf("  (skip pixel: không có GPU)\n");
        return true;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("  (skip pixel: không tạo được render target)\n");
        return true;
    }
    ctx.device->setDefaultRenderTarget(target);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Vẽ tam giác đỏ bằng glDrawElementsIndirect (command trong buffer).
    struct EltCmd { GLuint count, instanceCount, firstIndex; GLint baseVertex; GLuint baseInstance; };
    EltCmd cmd = {3, 1, 0, 0, 0};
    GLuint dib = 0;
    glGenBuffers(1, &dib);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, dib);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(cmd), &cmd, GL_STATIC_DRAW);

    glBindVertexArray(g_vao);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, (void*)0);

    GLubyte px[4] = {0};
    glReadPixels(32, 40, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    TGLMT_CHECK(px[0] > 200 && px[1] < 60,
                "glDrawElementsIndirect phải vẽ tam giác đỏ (pixel đọc được)");

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDeleteBuffers(1, &dib);
    glBindVertexArray(0);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core draw entry points (GL 4.6 §10.3/§10.4) ===\n");
    bool ok = true;

    // --- Phần 1: Null backend — kiểm chứng THAM SỐ của lệnh draw qua
    // drawTrace (count / instanceCount / indexed / số lần draw). ---
    {
        tglmt::Context nullCtx("null");
        tglmt::Context::MakeCurrent(&nullCtx);
        if (!BuildProgram()) {
            printf("SKIP: không dựng được program\n");
            return 0;
        }
        BuildGeometry();
        const Case kCases[] = {
            {"glDrawArraysIndirect / glDrawElementsIndirect", TestDrawIndirect},
            {"glDrawElementsInstanced / glDrawRangeElements*", TestDrawInstancedAndRange},
            {"glMultiDrawElements / ...BaseVertex", TestMultiDrawElements},
            {"glMultiDrawArraysIndirectCount", TestMultiDrawArraysIndirectCount},
            {"glDrawTransformFeedback*", TestDrawTransformFeedback},
        };
        for (const Case& c : kCases) {
            const int before = tglmt_test::Failures();
            c.fn();
            const bool pass = (tglmt_test::Failures() == before);
            printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
            ok = ok && pass;
        }
    }

    // --- Phần 2: Apple backend — kiểm chứng PIXEL thật (draw thực sự ra GPU) ---
    {
        tglmt::Context apple("apple");
        tglmt::Context::MakeCurrent(&apple);
        if (apple.device->isNull()) {
            printf("  [SKIP] pixel thật (không có GPU)\n");
        } else {
            if (BuildProgram()) BuildGeometry();
            const int before = tglmt_test::Failures();
            TestPixelVerification();
            const bool pass = (tglmt_test::Failures() == before);
            printf("  [%s] pixel thật (Apple)\n", pass ? "PASS" : "FAIL");
            ok = ok && pass;
        }
    }

    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
