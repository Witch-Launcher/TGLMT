// test_bake_repro_apple.cpp — Replicate GuiAtlas item bake 1:1 trên GPU:
// ortho-1024, identity MV, ENTITY verts (pos z DƯƠNG = sau camera GL),
// cull BACK, depth LEQUAL vs cleared, scissor cell 80x80, textured.
// Ma trận 2x2: {cull on/off} x {z sau/trước camera} → xem ô nào render.
// Thiếu GPU → SKIP.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_bake_repro_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec3 pos;\n"
    "layout(location = 1) in vec2 uv;\n"
    "layout(location = 0) out vec2 vUV;\n"
    "uniform mat4 Proj;\n"
    "void main() { vUV = uv; gl_Position = Proj * vec4(pos, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform sampler2D Sampler0;\n"
    "void main() { fragColor = texture(Sampler0, vUV); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_bake_repro_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_bake_repro_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glBindAttribLocation(prog, 1, "uv");
    glLinkProgram(prog);
    {
        GLint ok = 0;
        glGetProgramiv(prog, 0x8B82, &ok);
        Check(ok != 0, "link");
    }
    if (gFails) return 1;
    GLint projLoc = glGetUniformLocation(prog, "Proj");
    Check(projLoc >= 0, "proj loc");

    // texture 4x4: nửa trái ĐỎ đục, nửa phải TRONG SUỐT
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    unsigned char px[4 * 4 * 4];
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x) {
            unsigned char* p = px + (y * 4 + x) * 4;
            if (x < 2) { p[0] = 255; p[1] = 0; p[2] = 0; p[3] = 255; }
            else { p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0; }
        }
    glTexImage2D(0x0DE1, 0, 0x8058, 4, 4, 0, 0x1908, 0x1401, px);

    // FBO color+depth 64 (bake target), KHÔNG clear trước (như MC)
    GLuint colTex = 0, depTex = 0, fbo = 0;
    glGenTextures(1, &colTex);
    glBindTexture(0x0DE1, colTex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenTextures(1, &depTex);
    glBindTexture(0x0DE1, depTex);
    glTexImage2D(0x0DE1, 0, 0x81A7, 64, 64, 0, 0x1902, 0x1406, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, colTex, 0);
    glFramebufferTexture2D(0x8D40, 0x8D00, 0x0DE1, depTex, 0);

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    // quad CCW mặt +z (front khi nhìn từ +z): (0,0),(64,0),(64,64),(-0,64) + uv full
    auto quadZ = [&](float z) {
        float q[5 * 4] = {0, 0, z, 0, 0, /**/ 64, 0, z, 1, 0, /**/ 64, 64, z, 1, 1, /**/ 0, 64, z, 0, 1};
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(q), q, 0x88E4);
        glVertexAttribPointer(0, 3, 0x1406, 0, 20, (void*)0);
        glVertexAttribPointer(1, 2, 0x1406, 0, 20, (void*)12);
    };

    // ortho như bake (scale 2/W), column-major
    float ortho[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ortho[0] = 2.0f / 64; ortho[5] = -2.0f / 64; ortho[10] = -0.001f;
    ortho[12] = -1; ortho[13] = 1; ortho[15] = 1;
    glUseProgram(prog);
    glUniformMatrix4fv(projLoc, 1, 0, ortho);
    glViewport(0, 0, 64, 64);
    glEnable(0x0B71);
    glDepthFunc(0x0203);
    glDepthMask(1);
    glEnable(0x0C11);
    glScissor(8, 8, 48, 48);

    auto readR = [&]() {
        unsigned char out[64 * 64 * 4];
        memset(out, 0, sizeof(out));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, out);
        return out[(32 * 64 + 32) * 4];
    };
    // case A: z=+2.5 (SAU camera, như bake f0 z=43) + cull BACK
    glEnable(0x0B44);
    glCullFace(0x0405);
    quadZ(2.5f);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned a = readR();
    printf("[bake-repro] A behind-cam + cullBACK R=%u\n", a);
    // case B: z=+2.5, cull OFF
    glDisable(0x0B44);
    quadZ(2.5f);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned b = readR();
    printf("[bake-repro] B behind-cam + cullOFF R=%u\n", b);
    // case C: z=-2.5 (TRƯỚC camera) + cull BACK
    glEnable(0x0B44);
    quadZ(-2.5f);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned c = readR();
    printf("[bake-repro] C front-cam + cullBACK R=%u\n", c);
    // case D: z=-2.5, cull OFF
    glDisable(0x0B44);
    quadZ(-2.5f);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    unsigned d = readR();
    printf("[bake-repro] D front-cam + cullOFF R=%u\n", d);
    // Kỳ vọng GL đúng: A=0 (sau cam, cull mặt trước), B=255, C=255, D=255
    Check(a < 50, "A behind+cull culled");
    Check(b > 100, "B behind+nocull visible");
    Check(c > 100, "C front+cull visible");
    Check(d > 100, "D front+nocull visible");
    if (gFails) return 1;
    printf("test_bake_repro_apple PASS\n");
    return 0;
}
