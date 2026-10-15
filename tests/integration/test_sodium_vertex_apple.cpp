// test_sodium_vertex_apple.cpp — Bug 2 Sodium mất chunk.
// Giả thuyết #1: uvec2 a_Position + IPointer bị đọc sai.
// Giả thuyết #2: baseVertex/indirect validation loại draws (gap dAtt/dEnc + 0x0502).
// Null backend: kiểm tra state (isInt, stride validation, error codes), bỏ qua encode.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool c, const char* m) {
    if (!c) { printf("test_sodium_vertex_apple FAIL: %s\n", m); ++gFails; }
}

// Shader giải mã uint16 packed → NDC (giống pattern Sodium _deinterleave đơn giản).
static const char* kVS =
    "#version 330 core\n"
    "layout(location=0) in uvec2 a_P;\n"
    "void main(){\n"
    "  vec2 p = vec2(float(a_P.x) / 65535.0, float(a_P.y) / 65535.0) * 2.0 - 1.0;\n"
    "  gl_Position = vec4(p, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 330 core\n"
    "out vec4 o_C;\n"
    "void main(){ o_C = vec4(1.0, 1.0, 1.0, 1.0); }\n";

static GLuint MakeProg() {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    Check(ok == 1, "vs compile uvec2");
    glGetShaderiv(fs, 0x8B81, &ok);
    Check(ok == 1, "fs compile");
    GLuint pr = glCreateProgram();
    glAttachShader(pr, vs);
    glAttachShader(pr, fs);
    glLinkProgram(pr);
    glGetProgramiv(pr, 0x8B82, &ok);
    Check(ok == 1, "link uvec2");
    return pr;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);

    // --- 1. State: IPointer đánh dấu isInt, Pointer reset (cả Null lẫn Metal) ---
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        uint32_t dummy[2] = {0, 0};
        glBufferData(0x8892, sizeof(dummy), dummy, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, 0x1405, 0, (void*)0); // 2 x UNSIGNED_INT
        auto it = ctx.vaos.find(vao);
        Check(it != ctx.vaos.end(), "vao missing");
        if (it != ctx.vaos.end()) {
            Check(it->second.attribs[0].isInt == true, "IPointer isInt=true");
            Check(it->second.attribs[0].type == 0x1405, "IPointer type UINT");
        }
        // Pointer sau đó phải reset isInt (tránh dính flag khi tái dùng VAO).
        glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
        Check(it->second.attribs[0].isInt == false, "Pointer reset isInt=false");
        // IPointer sau Pointer(normalized=1) phải reset normalized/isLong.
        glVertexAttribPointer(0, 4, 0x1401, 1, 0, (void*)0);
        glVertexAttribIPointer(0, 2, 0x1405, 0, (void*)0);
        Check(it->second.attribs[0].normalized == 0, "IPointer reset normalized=0");
        Check(it->second.attribs[0].isLong == false, "IPointer reset isLong=false");
        glDeleteVertexArrays(1, &vao);
        // Dọn lỗi có thể có do VAO 0.
        while (glGetError() != 0) {}
    }

    // --- 2. Indirect stride validation (cả Null): stride !=0 mà <20 phải 0x0501 ---
    {
        // Gọi với drawcount âm → 0x0501 (không cần GPU).
        glMultiDrawElementsIndirect(0x0004, 0x1405, nullptr, -1, 20);
        GLenum e = glGetError();
        Check(e == 0x0501, "multidraw dc<0 -> INVALID_VALUE");
    }

    if (ctx.device->isNull()) {
        if (gFails == 0) printf("test_sodium_vertex_apple PASS (null: state OK)\n");
        else printf("test_sodium_vertex_apple: %d FAIL\n", gFails);
        return gFails ? 1 : 0;
    }

    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("test_sodium_vertex_apple FAIL: target nil\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    GLuint prog = MakeProg();
    if (gFails) { printf("test_sodium_vertex_apple: %d FAIL\n", gFails); return 1; }

    // --- 3. IPointer uvec2 draw thật: tam giác phủ center → pixels trắng ---
    {
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        // 3 đỉnh NDC (-1,-1),(1,-1),(0,1) mã hoá uint16.
        uint32_t tri[3 * 2] = {0, 0, 65535, 0, 32767, 65535};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, 0x1405, 8, (void*)0);
        glUseProgram(prog);
        glViewport(0, 0, 64, 64);
        glDisable(0x0BE2);
        glDisable(0x0B71);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "IPointer draw gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "IPointer draw not encoded (noPipe?)");
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        printf("ipointer mid=(%u,%u,%u)\n", mid[0], mid[1], mid[2]);
        Check(mid[0] > 200 && mid[1] > 200 && mid[2] > 200, "IPointer uvec2 vi tri sai");
    }

    // --- 4. baseVertex hợp lệ: EBO {0,1,2} + bv=1 → vẫn encode ---
    {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        uint32_t quad[4 * 2] = {0, 0, 65535, 0, 65535, 65535, 0, 0};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(quad), quad, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, 0x1405, 8, (void*)0);
        uint32_t idx[3] = {0, 1, 2};
        glGenBuffers(1, &ebo);
        glBindBuffer(0x8893, ebo);
        glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
        glUseProgram(prog);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        while (glGetError() != 0) {}
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glDrawElementsBaseVertex(0x0004, 3, 0x1405, (void*)0, 1);
        Check(glGetError() == 0, "baseVertex=1 gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "baseVertex=1 not encoded");
    }

    // --- 5. baseVertex tràn U16: EBO {0xFFFF} + bv=1 → 0x0502 + không encode ---
    // (Tái hiện gap dAtt/dEnc trong latestlog 2.txt frame 6000 + glerr 0x502.)
    {
        GLuint vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        uint32_t tri[3 * 2] = {0, 0, 65535, 0, 32767, 65535};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, 0x1405, 8, (void*)0);
        uint16_t idx[3] = {0xFFFF, 0xFFFF, 0xFFFF};
        glGenBuffers(1, &ebo);
        glBindBuffer(0x8893, ebo);
        glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
        glUseProgram(prog);
        while (glGetError() != 0) {}
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        uint64_t miscBefore = ctx.appleStats.miscFail;
        glDrawElementsBaseVertex(0x0004, 3, 0x1403, (void*)0, 1);
        GLenum e = glGetError();
        printf("overflow bv err=0x%x encSame=%d miscDelta=%llu\n", e,
               ctx.appleStats.drawsEncoded == encBefore,
               (unsigned long long)(ctx.appleStats.miscFail - miscBefore));
        Check(e == 0x0502, "baseVertex tran phai 0x0502");
        Check(ctx.appleStats.drawsEncoded == encBefore, "tran phai khong encode");
        Check(ctx.appleStats.miscFail > miscBefore, "tran phai dem miscFail (truy gap dAtt/dEnc)");
    }

    // --- 6. MultiDrawElementsIndirect thật: 1 draw qua DIB ---
    {
        GLuint vao, vbo, ebo, dib;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        uint32_t tri[3 * 2] = {0, 0, 65535, 0, 32767, 65535};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, 0x1405, 8, (void*)0);
        uint32_t idx[3] = {0, 1, 2};
        glGenBuffers(1, &ebo);
        glBindBuffer(0x8893, ebo);
        glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
        // indirect cmd: count=3, instanceCount=1, firstIndex=0, baseVertex=0, baseInstance=0
        struct Cmd { uint32_t count, inst, first, bv, bi; };
        Cmd cmd{3, 1, 0, 0, 0};
        glGenBuffers(1, &dib);
        glBindBuffer(0x8F3F, dib); // DRAW_INDIRECT_BUFFER
        glBufferData(0x8F3F, sizeof(cmd), &cmd, 0x88E4);
        glUseProgram(prog);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        while (glGetError() != 0) {}
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        glMultiDrawElementsIndirect(0x0004, 0x1405, (void*)0, 1, 0);
        GLenum e = glGetError();
        printf("indirect err=0x%x encoded=%d\n", e, ctx.appleStats.drawsEncoded > encBefore);
        Check(e == 0, "indirect gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "indirect not encoded");
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned char* mid = px + (32 * 64 + 32) * 4;
        printf("indirect mid=(%u,%u,%u)\n", mid[0], mid[1], mid[2]);
        Check(mid[0] > 200, "indirect pixels sai");
    }

    glDeleteProgram(prog);
    if (gFails == 0) { printf("test_sodium_vertex_apple PASS\n"); return 0; }
    printf("test_sodium_vertex_apple: %d FAIL\n", gFails);
    return 1;
}
