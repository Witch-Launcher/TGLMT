// test_mc_blit_apple.cpp — Hồi quy đen màn hình 26.1.2 trên máy thật.
// Game render menu vào FBO offscreen rồi composite ra màn hình bằng
// glBlitFramebuffer(read=N, draw=0) MỖI frame (log máy: blit# read=2 draw=0,
// full-size NEAREST). Đường này từng `return` sớm khi 1 phía là default FB
// (gl_framebuffer.cpp) nên màn hình đen tuyệt đối từ frame đầu dù mọi draw
// đều encode khỏe (att==enc, noPipe=0). Bridge đã có blitTo/FromTarget nhưng
// chưa ai gọi — test này khóa cả 3 đường composite chưa từng được bao phủ:
//  A. FBO→default (blitToTarget): vẽ đỏ vào FBO, clear màn hình xanh, blit,
//     đọc màn hình phải đỏ (xanh-trước/đỏ-sau).
//  B. default→FBO (blitFromTarget): vẽ xanh lá ra màn hình, blit vào FBO mới,
//     đọc FBO phải xanh lá.
//  C. screenquad không attribute (rotscale.vsh thật dùng gl_VertexID, đúng
//     dạng composite post-chain attrs=0 trên máy, cull BACK bật): vẽ tam giác
//     fullscreen từ vertex_id, đọc phải magenta.
// Thiếu GPU → SKIP. Sai pixel → FAIL thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

// VS/FS có varying khai-nhưng-không-dùng (đúng dạng probe từng nghi ngờ):
// FS ra màu const, không đọc varying nào từ vertex.
static const char* kVSProbe =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out float dummy;\n"
    "layout(location = 0) uniform mat4 uMVP;\n"
    "void main() { dummy = 1.0; gl_Position = uMVP * vec4(pos, 0.0, 1.0); }\n";
static const char* kFSRed =
    "#version 460 core\n"
    "layout(location = 0) in float dummy;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";
static const char* kFSGreen =
    "#version 460 core\n"
    "layout(location = 0) in float dummy;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(0.0, 1.0, 0.0, 1.0); }\n";
static const char* kFSBlue =
    "#version 460 core\n"
    "layout(location = 0) in float dummy;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(0.0, 0.0, 1.0, 1.0); }\n";
// Screenquad attributeless: biểu thức vertex_id Y HỆT rotscale.vsh của game
// (uv = ((id<<1)&2, id&2), pos = uv*2-1). FS khai uv nhưng ra màu const.
static const char* kVSQuad =
    "#version 460 core\n"
    "layout(location = 0) out vec2 uv;\n"
    "void main() {\n"
    "  uv = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
    "  gl_Position = vec4(uv * vec2(2.0, 2.0) + vec2(-1.0, -1.0), 0.0, 1.0);\n"
    "}\n";
static const char* kFSMagenta =
    "#version 460 core\n"
    "layout(location = 0) in vec2 uv;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 1.0, 1.0); }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_blit_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static GLuint MakeProg(const char* vs, const char* fs, bool bindPos) {
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(v, 0x8B81, &ok);
    Check(ok != 0, "vs compile");
    glGetShaderiv(f, 0x8B81, &ok);
    Check(ok != 0, "fs compile");
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    if (bindPos) glBindAttribLocation(p, 0, "pos");
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    Check(ok != 0, "link");
    return p;
}

static void SetIdentity(GLuint prog) {
    float ident[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    glUniformMatrix4fv(glGetUniformLocation(prog, "uMVP"), 1, 0, ident);
}

// Tam giác fullscreen (che mọi pixel) qua attribute pos2.
static void DrawFullTri() {
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float tri[6] = {-1, -1, 3, -1, -1, 3};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    glDrawArrays(0x0004, 0, 3);
}

static void ReadCenter(unsigned char out[4]) {
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    memcpy(out, mid, 4);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_blit_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_blit_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint progRed = MakeProg(kVSProbe, kFSRed, true);
    GLuint progGreen = MakeProg(kVSProbe, kFSGreen, true);
    GLuint progBlue = MakeProg(kVSProbe, kFSBlue, true);
    GLuint progQuad = MakeProg(kVSQuad, kFSMagenta, false);
    if (gFails) return 1;

    // FBO 64x64 cho case A/B.
    GLuint fbo, fboTex;
    glGenTextures(1, &fboTex);
    glBindTexture(0x0DE1, fboTex);
    std::vector<unsigned char> zeros(64 * 64 * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, zeros.data());
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo); // DRAW
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fboTex, 0);

    // ---- A. Vẽ đỏ vào FBO ----
    glBindFramebuffer(0x8CA8, fbo); // READ (giữ, dùng cho blit sau)
    glBindFramebuffer(0x8CA9, fbo); // DRAW
    glUseProgram(progRed);
    SetIdentity(progRed);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    DrawFullTri();
    Check(glGetError() == 0, "A draw gl error");
    {
        unsigned char px[4];
        ReadCenter(px); // READ vẫn là fbo
        printf("[A draw] fbo center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "A FBO do truoc blit");
    }
    // Sơn màn hình xanh (xanh-trước/đỏ-sau chứng minh blit, không phải cặn).
    glBindFramebuffer(0x8CA9, 0); // DRAW = default
    glUseProgram(progBlue);
    SetIdentity(progBlue);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    DrawFullTri();
    Check(glGetError() == 0, "A paint-default gl error");
    // Blit FBO→default (đúng shape máy thật: read=N draw=0, full NEAREST).
    glBindFramebuffer(0x8CA8, fbo);
    glBindFramebuffer(0x8CA9, 0);
    glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64, 0x00004000, 0x2600);
    Check(glGetError() == 0, "A blit gl error");
    glBindFramebuffer(0x8CA8, 0); // READ = default
    {
        unsigned char px[4];
        ReadCenter(px);
        printf("[A blit] default center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "A default do sau blit FBO->default");
    }
    // Nguồn không bị blit làm bẩn.
    glBindFramebuffer(0x8CA8, fbo);
    {
        unsigned char px[4];
        ReadCenter(px);
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "A FBO van do sau blit");
    }
    if (gFails) return 1;

    // ---- B. default→FBO ----
    GLuint fbo2, fboTex2;
    glGenTextures(1, &fboTex2);
    glBindTexture(0x0DE1, fboTex2);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, zeros.data());
    glTexParameteri(0x0DE1, 0x2801, 0x2600);
    glTexParameteri(0x0DE1, 0x2800, 0x2600);
    glGenFramebuffers(1, &fbo2);
    glBindFramebuffer(0x8CA9, fbo2);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, fboTex2, 0);
    // Sơn màn hình xanh lá.
    glBindFramebuffer(0x8CA9, 0);
    glUseProgram(progGreen);
    SetIdentity(progGreen);
    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 0, 1);
    glClear(0x00004000);
    DrawFullTri();
    Check(glGetError() == 0, "B paint-default gl error");
    glBindFramebuffer(0x8CA8, 0);   // READ = default
    glBindFramebuffer(0x8CA9, fbo2); // DRAW = fbo2
    glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64, 0x00004000, 0x2600);
    Check(glGetError() == 0, "B blit gl error");
    glBindFramebuffer(0x8CA8, fbo2);
    {
        unsigned char px[4];
        ReadCenter(px);
        printf("[B blit] fbo2 center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[1] > 200 && px[0] < 50 && px[2] < 50, "B FBO xanh la sau blit default->FBO");
    }
    if (gFails) return 1;

    // ---- C. screenquad attributeless + cull (đúng máy: cull BACK bật) ----
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao); // không enable attribute nào
        glUseProgram(progQuad);
        // CULL_FACE bật như game (mọi draw menu đều cull BACK/CCW).
        glEnable(0x0B44); // CULL_FACE
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "C draw gl error");
        if (ctx.appleStats.drawsEncoded == 0) {
            printf("test_mc_blit_apple FAIL: C draw not encoded\n");
            return 1;
        }
        glBindFramebuffer(0x8CA8, 0);
        unsigned char px[4];
        ReadCenter(px);
        printf("[C quad] center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[2] > 200 && px[1] < 50, "C attributeless quad magenta (cull bat)");
        // Biến thể count=6 như post-chain máy thật (2 tam giác từ vertex_id).
        glBindFramebuffer(0x8CA9, 0);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 6);
        Check(glGetError() == 0, "C6 draw gl error");
        glBindFramebuffer(0x8CA8, 0);
        ReadCenter(px);
        printf("[C6 quad] center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[2] > 200 && px[1] < 50, "C6 attributeless 6 verts magenta");
        glDisable(0x0B44);
    }
    if (gFails) return 1;
    printf("test_mc_blit_apple PASS (FBO<->default + attributeless quad)\n");
    return 0;
}
