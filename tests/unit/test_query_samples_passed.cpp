// test_query_samples_passed.cpp — Query: GL_SAMPLES_PASSED trả SỐ SAMPLE thật
// (Metal visibility buffer, mode Counting) + GL_TIME_ELAPSED đo thời gian GPU.
//
// Spec: glspec46.core.pdf §4.2.1 (query targets) + §4.3 (timer queries):
//  - SAMPLES_PASSED: "số sample pass depth test" của CẢ query (không phải
//    boolean). ANY_SAMPLES_PASSED(_CONSERVATIVE): boolean TRUE/FALSE.
//  - TIME_ELAPSED: "thời gian cần để xử lý hết chuỗi lệnh" (ns).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int Fails = 0;
static void Check(bool ok, const char* m) {
    if (!ok) { printf("test_query_samples_passed FAIL: %s\n", m); ++Fails; }
}
static void Clear() { while (glGetError() != 0) {} }

// Backend Null: không render, không có GPU → vòng đời query + ngữ nghĩa lỗi.
static void TestNull() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint q = 0;
    glGenQueries(1, &q);

    glBeginQuery(0x8914 /*SAMPLES_PASSED*/, q);
    Check(glGetError() == 0, "begin SAMPLES_PASSED no error");
    GLint cur = -1;
    glGetQueryiv(0x8914, 0x8865 /*CURRENT_QUERY*/, &cur);
    Check(cur == (GLint)q, "current query id");
    glEndQuery(0x8914);
    Check(glGetError() == 0, "end SAMPLES_PASSED no error");

    GLuint avail = 0;
    glGetQueryObjectuiv(q, 0x8867 /*RESULT_AVAILABLE*/, &avail);
    Check(avail == 1, "SAMPLES_PASSED result available");
    GLuint samples = 0xFFFFFFFFu;
    glGetQueryObjectuiv(q, 0x8866 /*RESULT*/, &samples);
    Check(samples == 0, "null backend renders nothing -> 0 samples");
    GLuint64 s64 = 0xFFFFFFFFFFFFFFFFull;
    glGetQueryObjectui64v(q, 0x8866 /*RESULT*/, &s64);
    Check(s64 == 0, "64-bit samples result matches 32-bit");
    GLuint noWait = 0xFFFFFFFFu;
    glGetQueryObjectuiv(q, 0x9194 /*RESULT_NO_WAIT*/, &noWait);
    Check(noWait == 0, "RESULT_NO_WAIT matches");

    // ANY_SAMPLES_PASSED: spec yêu cầu boolean (0/1), không phải số sample.
    GLuint anyQ = 0;
    glGenQueries(1, &anyQ);
    glBeginQuery(0x8C2F /*ANY_SAMPLES_PASSED*/, anyQ);
    glEndQuery(0x8C2F);
    GLuint anyV = 0xFFFFFFFFu;
    glGetQueryObjectuiv(anyQ, 0x8866, &anyV);
    Check(anyV == 0, "null backend -> any samples 0");

    // TIME_ELAPSED: phải có kết quả (0 khi không có GPU để đo).
    GLuint t = 0;
    glGenQueries(1, &t);
    glBeginQuery(0x88BF /*TIME_ELAPSED*/, t);
    Check(glGetError() == 0, "begin TIME_ELAPSED no error");
    glEndQuery(0x88BF);
    Check(glGetError() == 0, "end TIME_ELAPSED no error");
    GLuint tAvail = 0;
    glGetQueryObjectuiv(t, 0x8867, &tAvail);
    Check(tAvail == 1, "TIME_ELAPSED result available");
    GLuint64 elapsed = 0xFFFFFFFFFFFFFFFFull;
    glGetQueryObjectui64v(t, 0x8866, &elapsed);
    Check(elapsed == 0, "null backend -> elapsed 0");
    // GL_GPU_DISJOINT_EXT: không có GPU để đo → số đo không đáng tin.
    GLuint disjoint = 0;
    glGetQueryObjectuiv(t, 0x8FBB /*GL_GPU_DISJOINT_EXT*/, &disjoint);
    Check(disjoint == 1, "null backend -> gpu disjoint");

    // Lỗi: target sai, lồng nhau, end không có begin.
    Clear();
    glBeginQuery(0x1234, q);
    Check(glGetError() == GL_INVALID_ENUM, "bad query target");
    glBeginQuery(0x8914, q);
    Check(glGetError() == 0, "begin ok");
    glBeginQuery(0x8914, q);
    Check(glGetError() == GL_INVALID_OPERATION, "nested begin rejected");
    glEndQuery(0x8914);
    glEndQuery(0x8914);
    Check(glGetError() == GL_INVALID_OPERATION, "end without begin rejected");
    glGetQueryObjectuiv(q, 0x1234, &samples);
    Check(glGetError() == GL_INVALID_ENUM, "bad query pname");

    // Query buffer object: ghi kết quả vào buffer.
    GLuint buf = 0;
    glCreateBuffers(1, &buf);
    glNamedBufferData(buf, 16, nullptr, 0x88E8 /*DYNAMIC_DRAW*/);
    glGetQueryBufferObjectui64v(q, buf, 0x8866, 0);
    GLuint64 out = 0xFFFFFFFFFFFFFFFFull;
    glGetNamedBufferSubData(buf, 0, sizeof(out), &out);
    Check(out == 0, "query result written to buffer");
    glDeleteBuffers(1, &buf);

    glDeleteQueries(1, &q);
    glDeleteQueries(1, &anyQ);
    glDeleteQueries(1, &t);
}

static GLuint MakeProg() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 o_color;\n"
        "void main() { o_color = vec4(1.0, 0.0, 0.0, 1.0); }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, 0x8B82 /*LINK_STATUS*/, &ok);
    if (!ok) printf("test_query_samples_passed: shader link fail\n");
    return ok ? p : 0;
}

// Backend Metal: đo thật — draw 1 quad fullscreen 64x64 có depth → số sample
// phải bằng số pixel (4096) nếu GPU hỗ trợ Counting mode (Apple3+).
static void TestApple() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_query_samples_passed SKIP apple: no metal device\n");
        return;
    }
    const GLsizei kSize = 64;
    GLuint prog = MakeProg();
    if (!prog) { ++Fails; return; }

    GLuint tex = 0, depthRb = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(0x0DE1, tex);
    std::vector<uint8_t> px((size_t)kSize * kSize * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058 /*RGBA8*/, kSize, kSize, 0, 0x1908, 0x1401, px.data());
    glGenRenderbuffers(1, &depthRb);
    glBindRenderbuffer(0x8D41 /*RENDERBUFFER*/, depthRb);
    glRenderbufferStorage(0x8D41, 0x81A6 /*DEPTH_COMPONENT24*/, kSize, kSize);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40 /*FRAMEBUFFER*/, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0 /*COLOR_ATTACHMENT0*/, 0x0DE1, tex, 0);
    glFramebufferRenderbuffer(0x8D40, 0x8D00 /*DEPTH_ATTACHMENT*/, 0x8D41, depthRb);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5 /*COMPLETE*/) {
        printf("test_query_samples_passed SKIP apple: FBO incomplete\n");
        return;
    }

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    {
        float v[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, vbo);
        glBufferData(0x8892, sizeof(v), v, 0x88E4 /*STATIC_DRAW*/);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406 /*FLOAT*/, 0, 0, (void*)0);
    }
    glUseProgram(prog);
    glViewport(0, 0, kSize, kSize);
    glEnable(0x0B71 /*DEPTH_TEST*/);
    glDepthFunc(0x0201 /*LESS*/);
    glClearDepth(1.0);
    glClear(0x00000100 /*DEPTH_BUFFER_BIT*/);

    const bool counting = ctx.device->visibilityCountingSupported();

    // (1) SAMPLES_PASSED quanh draw → số sample thật.
    GLuint sq = 0;
    glGenQueries(1, &sq);
    glBeginQuery(0x8914 /*SAMPLES_PASSED*/, sq);
    glDrawArrays(0x0004 /*TRIANGLES*/, 0, 6);
    glEndQuery(0x8914);
    GLuint64 samples = 0xFFFFFFFFFFFFFFFFull;
    glGetQueryObjectui64v(sq, 0x8866 /*RESULT*/, &samples);
    if (counting) {
        // 64x64 pixel, 1 sample/pixel, depth test pass hết → 4096 sample.
        Check(samples == (GLuint64)kSize * kSize, "SAMPLES_PASSED = so sample that");
    } else {
        // GPU không có Counting mode → không có số để trả → 0 (không giả số).
        Check(samples == 0, "khong co Counting -> SAMPLES_PASSED = 0");
    }

    // (2) ANY_SAMPLES_PASSED: boolean (0/1) dù có bao nhiêu sample.
    GLuint aq = 0;
    glGenQueries(1, &aq);
    glBeginQuery(0x8C2F /*ANY_SAMPLES_PASSED*/, aq);
    glDrawArrays(0x0004, 0, 6);
    glEndQuery(0x8C2F);
    GLuint anyV = 0xFFFFFFFFu;
    glGetQueryObjectuiv(aq, 0x8866, &anyV);
    Check(anyV == 1, "ANY_SAMPLES_PASSED = 1 (visible)");

    // (3) TIME_ELAPSED: nhiều draw để có thời gian GPU đo được (> 0).
    GLuint tq = 0;
    glGenQueries(1, &tq);
    glBeginQuery(0x88BF /*TIME_ELAPSED*/, tq);
    for (int i = 0; i < 512; ++i) glDrawArrays(0x0004, 0, 6);
    glEndQuery(0x88BF);
    GLuint64 elapsed = 0;
    glGetQueryObjectui64v(tq, 0x8866 /*RESULT*/, &elapsed);
    Check(elapsed > 0, "TIME_ELAPSED > 0 (do thoi gian GPU that)");
    GLuint disjoint = 1;
    glGetQueryObjectuiv(tq, 0x8FBB /*GL_GPU_DISJOINT_EXT*/, &disjoint);
    Check(disjoint == 0, "do duoc GPU timestamp -> khong disjoint");

    glDeleteQueries(1, &sq);
    glDeleteQueries(1, &aq);
    glDeleteQueries(1, &tq);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &depthRb);
    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
}

int main() {
    TestNull();
    TestApple();
    return Fails ? 1 : 0;
}
