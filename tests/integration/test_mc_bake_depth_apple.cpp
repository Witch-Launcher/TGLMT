// test_mc_bake_depth_apple.cpp — Tái hiện GuiItemAtlas bake (MC 26.1.2):
// FBO color+depth KHÔNG clear, draw LEQUAL + colorMask đầy đủ.
//  T1: draw vào FBO+depth chưa clear phải VISIBLE (depth rác mà fail = đen).
//  T2: glColorMask(0,0,0,0) phải chặn ghi màu (hiện là no-op → ghi bậy).
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
        printf("test_mc_bake_depth_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec2 vP;\n"
    "void main() { vP = pos; gl_Position = vec4(pos, 0.5, 1.0); }\n";
static const char* kFSRed =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_bake_depth_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_bake_depth_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFSRed, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    {
        GLint ok = 0;
        glGetProgramiv(prog, 0x8B82, &ok);
        Check(ok != 0, "link");
    }
    if (gFails) return 1;

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float q[8] = {-1, -1, 1, -1, 1, 1, -1, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(q), q, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);

    // FBO color+depth như bake target (KHÔNG clear — đúng MC lazy bake)
    GLuint colTex = 0, depTex = 0, fbo = 0;
    glGenTextures(1, &colTex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, colTex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenTextures(1, &depTex);
    glBindTexture(0x0DE1, depTex);
    glTexImage2D(0x0DE1, 0, 0x81A7, 64, 64, 0, 0x1902, 0x1406, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo); // FRAMEBUFFER = READ+DRAW (ReadPixels đọc READ)
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, colTex, 0);
    glFramebufferTexture2D(0x8CA9, 0x8D00, 0x0DE1, depTex, 0);
    Check(glGetError() == 0, "fbo setup");

    glUseProgram(prog);
    glViewport(0, 0, 64, 64);
    glEnable(0x0B71); // DEPTH_TEST
    glDepthFunc(0x0203); // LEQUAL (MC DEFAULT)
    glDepthMask(1);

    // T1: draw KHÔNG clear trước (z=0.5 giữa) → phải ĐỎ
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    Check(glGetError() == 0, "t1 draw");
    {
        auto it = ctx.textures.find(colTex);
        Check(it != ctx.textures.end(), "col exists");
        if (it != ctx.textures.end() && it->second.gpu) {
            ctx.FlushPendingEncoder();
            ctx.CommitAndWait();
            auto wt = ctx.device->wrapAsTarget(it->second.gpu.get(), nullptr);
            std::vector<unsigned char> fb(64 * 64 * 4, 0);
            bool ok = wt && wt->readback(fb.data(), 64 * 4);
            Check(ok, "t1 readback");
            if (ok)
                printf("[bake] T1 center=(%u,%u,%u) (want red 255,0,0)\n", fb[(32 * 64 + 32) * 4],
                       fb[(32 * 64 + 32) * 4 + 1], fb[(32 * 64 + 32) * 4 + 2]);
            if (ok) Check(fb[(32 * 64 + 32) * 4] > 200, "T1 visible without clear");
        }
    }

    // T2: colorMask(0) → draw XANH (program xanh) → phải GIỮ nguyên đỏ từ T1
    GLuint fsG = glCreateShader(0x8B30);
    const char* kFSGreen =
        "#version 460 core\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "void main() { fragColor = vec4(0.0, 1.0, 0.0, 1.0); }\n";
    glShaderSource(fsG, 1, &kFSGreen, nullptr);
    glCompileShader(fsG);
    GLuint progG = glCreateProgram();
    glAttachShader(progG, vs);
    glAttachShader(progG, fsG);
    glBindAttribLocation(progG, 0, "pos");
    glLinkProgram(progG);
    {
        GLint ok = 0;
        glGetProgramiv(progG, 0x8B82, &ok);
        Check(ok != 0, "link green");
    }
    if (gFails) return 1;
    glUseProgram(progG);
    glColorMask(0, 0, 0, 0);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
    {
        auto it = ctx.textures.find(colTex);
        if (it != ctx.textures.end() && it->second.gpu) {
            ctx.FlushPendingEncoder();
            ctx.CommitAndWait();
            auto wt = ctx.device->wrapAsTarget(it->second.gpu.get(), nullptr);
            std::vector<unsigned char> fb(64 * 64 * 4, 0);
            bool ok = wt && wt->readback(fb.data(), 64 * 4);
            Check(ok, "t2 readback");
            if (ok)
                printf("[bake] T2 center=(%u,%u,%u) (want still red 255,0,0 — masked)\n", fb[(32 * 64 + 32) * 4],
                       fb[(32 * 64 + 32) * 4 + 1], fb[(32 * 64 + 32) * 4 + 2]);
            if (ok) Check(fb[(32 * 64 + 32) * 4] > 200 && fb[(32 * 64 + 32) * 4 + 1] < 50, "T2 colorMask blocks write");
        }
    }
    glColorMask(1, 1, 1, 1);
    if (gFails) return 1;

    // T3: depth tồn tại QUA encoder split (flush giữa frame).
    // Vẽ ĐỎ gần (z=0.2) fullscreen, flush (ReadPixels), vẽ XANH xa (z=0.8)
    // phủ lên: LEQUAL → ĐỎ thắng. Bản cũ: encoder mới force-clear depth →
    // XANH đè (X-RAY: mây/nước/entity xuyên tường sau mỗi flush).
    const char* kVS3 =
        "#version 460 core\n"
        "layout(location = 0) in vec3 pos;\n"
        "void main() { gl_Position = vec4(pos, 1.0); }\n";
    const char* kFSGreen2 =
        "#version 460 core\n"
        "layout(location = 0) out vec4 fragColor;\n"
        "void main() { fragColor = vec4(0.0, 1.0, 0.0, 1.0); }\n";
    GLuint vs3 = glCreateShader(0x8B31);
    glShaderSource(vs3, 1, &kVS3, nullptr);
    glCompileShader(vs3);
    GLuint progR = glCreateProgram(), progG2 = glCreateProgram();
    glAttachShader(progR, vs3);
    glAttachShader(progR, fs);
    glBindAttribLocation(progR, 0, "pos");
    glLinkProgram(progR);
    GLuint fsG2 = glCreateShader(0x8B30);
    glShaderSource(fsG2, 1, &kFSGreen2, nullptr);
    glCompileShader(fsG2);
    glAttachShader(progG2, vs3);
    glAttachShader(progG2, fsG2);
    glBindAttribLocation(progG2, 0, "pos");
    glLinkProgram(progG2);
    {
        GLint okR = 0, okG = 0;
        glGetProgramiv(progR, 0x8B82, &okR);
        glGetProgramiv(progG2, 0x8B82, &okG);
        Check(okR != 0 && okG != 0, "link T3");
    }
    if (gFails) return 1;
    GLuint vao3 = 0, vboR = 0, vboG = 0;
    glGenVertexArrays(1, &vao3);
    glBindVertexArray(vao3);
    float qR[12] = {-1, -1, 0.2f, 1, -1, 0.2f, 1, 1, 0.2f, -1, 1, 0.2f};
    float qG[12] = {-1, -1, 0.8f, 1, -1, 0.8f, 1, 1, 0.8f, -1, 1, 0.8f};
    glGenBuffers(1, &vboR);
    glGenBuffers(1, &vboG);
    glBindBuffer(0x8893, ebo);
    glEnableVertexAttribArray(0);
    glUseProgram(progR);
    glViewport(0, 0, 64, 64);
    glClearDepth(1.0);
    glClearColor(0, 0, 1, 1);
    glClear(0x00000100 | 0x00004000);
    glBindBuffer(0x8892, vboR);
    glBufferData(0x8892, sizeof(qR), qR, 0x88E4);
    glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    glDrawElements(0x0004, 6, 0x1405, (void*)0); // đỏ gần
    {
        unsigned char tmp[64 * 64 * 4]; // flush: encoder split tại đây
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, tmp);
    }
    glUseProgram(progG2);
    glBindBuffer(0x8892, vboG);
    glBufferData(0x8892, sizeof(qG), qG, 0x88E4);
    glVertexAttribPointer(0, 3, 0x1406, 0, 12, (void*)0);
    glDrawElements(0x0004, 6, 0x1405, (void*)0); // xanh xa phủ lên
    {
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned r = px[(32 * 64 + 32) * 4], g = px[(32 * 64 + 32) * 4 + 1];
        printf("[bake] T3 center=(%u,%u,...) (want red near wins)\n", r, g);
        Check(r > 200 && g < 50, "T3 depth persists across split");
    }
    if (gFails) return 1;
    printf("test_mc_bake_depth_apple PASS\n");
    return 0;
}
