// test_tess_vs_capture_apple.cpp — P-Tess T4b: VS prepass thật.
// VS có attribute + gl_Position, TCS đọc gl_in + varying (passthrough cùng tên),
// indexed + baseVertex. Kỳ vọng pixel đỏ: nếu capture/gl_in/buffer input sai thì
// tam giác không phủ tâm (nền xanh).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in float aBias;\n"
    "layout(location = 2) in vec2 aInst;\n"
    "uniform float uBias;\n"
    "out vec2 vPos;\n"
    "out float vBias;\n"
    "void main() {\n"
    "  vPos = aPos + aInst;\n"
    "  vBias = aBias + uBias;\n"
    "  gl_Position = vec4(aPos + aInst, 0.0, 1.0);\n"
    "}\n";
static const char* kTCS =
    "#version 460 core\n"
    "layout(vertices = 3) out;\n"
    "in vec2 vPos[];\n"
    "out vec2 vPos[];\n"
    "in float vBias[];\n"
    "out float vBias[];\n"
    "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
    "uniform vec4 uPatchColor;\n"
    "uniform sampler2D uTex;\n"
    "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
    "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
    "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
    "layout(binding = 3, offset = 0) uniform atomic_uint uCounter;\n"
    "patch out vec4 pColor;\n"
    "void main() {\n"
    "  vPos[gl_InvocationID] = vPos[gl_InvocationID];\n"
    "  vBias[gl_InvocationID] = vBias[gl_InvocationID];\n"
    "  gl_out[gl_InvocationID].gl_Position = vec4(vPos[gl_InvocationID], 0.25, 1.0);\n"
    "  uint cnt = atomicCounterIncrement(uCounter);\n"
    "  if (gl_InvocationID == 0) {\n"
    "    pColor = (vBias[0] > 0.5)\n"
    "           ? uPatchColor * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)) : vec4(0.0, 1.0, 0.0, 1.0);\n"
    "    if (cnt > 1000000u) pColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "  }\n"
    "  gl_TessLevelOuter[0] = 1.0; gl_TessLevelOuter[1] = 1.0; gl_TessLevelOuter[2] = 1.0;\n"
    "  gl_TessLevelInner[0] = 1.0;\n"
    "}\n";
static const char* kTES =
    "#version 460 core\n"
    "layout(triangles) in;\n"
    "in gl_PerVertex { vec4 gl_Position; } gl_in[];\n"
    "in vec2 vPos[];\n"
    "patch in vec4 pColor;\n"
    "uniform vec4 uTint;\n"
    "uniform sampler2D uTex;\n"
    "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
    "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
    "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
    "layout(binding = 3, offset = 0) uniform atomic_uint uCounter;\n"
    "out vec4 vColor;\n"
    "void main() {\n"
    "  vec2 p = vPos[0] * gl_TessCoord.x + vPos[1] * gl_TessCoord.y + vPos[2] * gl_TessCoord.z;\n"
    "  float cpW = (gl_in[0].gl_Position.z > 0.2 ? 1.0 : 0.0) *\n"
    "              (gl_in.length() == 3 ? 1.0 : 0.0);\n"
    "  vColor = pColor * uTint * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)) * cpW;\n"
    "  gl_Position = vec4(p, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "in vec4 vColor;\n"
    "uniform float uMul;\n"
    "uniform sampler2D uTex;\n"
    "layout(std140, binding = 0) uniform U { vec4 uBlockColor; };\n"
    "layout(std430, binding = 1) buffer S { vec4 uSsboColor; };\n"
    "layout(rgba8, binding = 2) uniform readonly image2D uImg;\n"
    "layout(binding = 3, offset = 0) uniform atomic_uint uCounter;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 1) out vec4 fragSecond;\n"
    "void main() { fragColor = vColor * uMul * texture(uTex, vec2(0.5)) * uBlockColor * uSsboColor * imageLoad(uImg, ivec2(0)); fragSecond = vec4(0.0, 1.0, 0.0, 1.0); }\n";

static tglmt::GLuint Compile(tglmt::GLenum type, const char* src, const char* nm) {
    using namespace tglmt::gl;
    tglmt::GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    tglmt::GLint ok = 0;
    glGetShaderiv(s, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("FAIL %s compile: %s\n", nm, log);
        return 0;
    }
    return s;
}

static bool CenterRed(tglmt::Context& ctx, const char* tag) {
    using namespace tglmt::gl;
    if (glGetError() != 0) { printf("FAIL %s: GL error\n", tag); return false; }
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    printf("%s mid=(%u,%u,%u,%u)\n", tag, mid[0], mid[1], mid[2], mid[3]);
    if (!(mid[0] > 200 && mid[1] < 50 && mid[2] < 50)) {
        printf("FAIL %s: center not red\n", tag);
        return false;
    }
    (void)ctx;
    return true;
}

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_tess_vs_capture_apple SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;
    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vs = Compile(0x8B31, kVS, "vs");
    tglmt::GLuint tc = Compile(0x8E88, kTCS, "tcs");
    tglmt::GLuint te = Compile(0x8E87, kTES, "tes");
    tglmt::GLuint fs = Compile(0x8B30, kFS, "fs");
    if (!vs || !tc || !te || !fs) return 1;
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, tc); glAttachShader(p, te); glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        printf("FAIL link: %s\n", log);
        return 1;
    }
    tglmt::GLint al0 = glGetAttribLocation(p, "aPos");
    tglmt::GLint al1 = glGetAttribLocation(p, "aBias");
    if (al0 != 0 || al1 != 1) {
        printf("FAIL attrib reflection: aPos=%d aBias=%d\n", (int)al0, (int)al1);
        return 1;
    }
    tglmt::GLint uBias = glGetUniformLocation(p, "uBias");
    tglmt::GLint uPatch = glGetUniformLocation(p, "uPatchColor");
    tglmt::GLint uTint = glGetUniformLocation(p, "uTint");
    tglmt::GLint uMul = glGetUniformLocation(p, "uMul");
    tglmt::GLint uTex = glGetUniformLocation(p, "uTex");
    if (uBias < 0 || uPatch < 0 || uTint < 0 || uMul < 0 || uTex < 0) {
        printf("FAIL uniform reflection: %d %d %d %d tex=%d\n", (int)uBias, (int)uPatch,
               (int)uTint, (int)uMul, (int)uTex);
        return 1;
    }
    glUseProgram(p);
    glUniform1f(uBias, 1.0f);
    glUniform4f(uPatch, 1.0f, 0.0f, 0.0f, 1.0f);
    glUniform4f(uTint, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1f(uMul, 1.0f);
    glUniform1i(uTex, 0);
    const unsigned char texel[4] = {255, 0, 0, 255};
    tglmt::GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    glTexParameteri(0x0DE1, 0x2801, 0x2601);
    glTexParameteri(0x0DE1, 0x2800, 0x2601);
    glTexImage2D(0x0DE1, 0, 0x8058, 1, 1, 0, 0x1908, 0x1401, texel);
    const float blockColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    tglmt::GLuint ubo = 0;
    glGenBuffers(1, &ubo);
    glBindBuffer(0x8A11, ubo);
    glBufferData(0x8A11, sizeof(blockColor), blockColor, 0x88E4);
    glBindBufferBase(0x8A11, 0, ubo);
    tglmt::GLuint ssbo = 0;
    glGenBuffers(1, &ssbo);
    glBindBuffer(0x90D2, ssbo);
    glBufferData(0x90D2, sizeof(blockColor), blockColor, 0x88E4);
    glBindBufferBase(0x90D2, 1, ssbo);
    glBindImageTexture(2, tex, 0, 0, 0, 0x88B8 /*READ_ONLY*/, 0x8058 /*RGBA8*/);
    tglmt::GLuint acbuf = 0;
    const unsigned int aczero = 0;
    glGenBuffers(1, &acbuf);
    glBindBuffer(0x92C0 /*ATOMIC_COUNTER_BUFFER*/, acbuf);
    glBufferData(0x92C0, sizeof(aczero), &aczero, 0x88E4);
    glBindBufferBase(0x92C0, 3, acbuf);

    // 6 đỉnh: 0..2 suy biến, 3..5 tam giác phủ toàn viewport.
    const float verts[] = {
        0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f,
        -1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f,
    };
    const unsigned short idx[] = {0, 1, 2};
    tglmt::GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
    glEnableVertexAttribArray(0);
    // location 1 cố tình TẮT array: GL dùng current generic value (1.0), không zero.
    glVertexAttrib1f(1, 0.0f);
    glVertexAttrib2f(2, 0.0f, 0.0f);
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);

    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E /*PATCHES*/, 3, 3);
    if (!CenterRed(ctx, "arrays-first3")) return 1;

    // 2 patch liên tiếp: patch 0 suy biến, patch 1 phủ màn hình. Kiểm tra capture
    // ghi đúng index toàn cục và TCS dispatch nhiều patch (pid*3).
    glClear(0x00004000);
    glDrawArrays(0x000E /*PATCHES*/, 0, 6);
    if (!CenterRed(ctx, "two-patches")) return 1;

    // Binding offset: base=byte 24 (vertex 3), cùng stride 8; nếu capture bỏ
    // base offset sẽ đọc 3 đỉnh suy biến ở đầu buffer → không đỏ.
    glBindVertexBuffer(0, vbo, 24, 8);
    glClear(0x00004000);
    glDrawArrays(0x000E /*PATCHES*/, 0, 3);
    if (!CenterRed(ctx, "binding-offset24")) return 1;
    glBindVertexBuffer(0, vbo, 0, 8);

    glClear(0x00004000);
    glDrawElementsBaseVertex(0x000E /*PATCHES*/, 3, 0x1403, nullptr, 3);
    if (!CenterRed(ctx, "indexed-basevertex3")) return 1;

    // Index pointer offset (không baseVertex): 3 index prefix rồi 3..5.
    tglmt::GLuint ebo2 = 0;
    glGenBuffers(1, &ebo2);
    glBindBuffer(0x8893, ebo2);
    const unsigned short idxPref[] = {0, 0, 0, 3, 4, 5};
    glBufferData(0x8893, sizeof(idxPref), idxPref, 0x88E4);
    glClear(0x00004000);
    glDrawElements(0x000E /*PATCHES*/, 3, 0x1403, (void*)(3 * sizeof(unsigned short)));
    if (!CenterRed(ctx, "index-offset3")) return 1;
    glBindBuffer(0x8893, ebo);

    glFinish();
    unsigned int counter = 0;
    glBindBuffer(0x92C0, acbuf);
    glGetBufferSubData(0x92C0, 0, sizeof(counter), &counter);
    if (counter == 0) {
        printf("FAIL tess atomic counter không tăng\n");
        return 1;
    }
    // FBO target: draw tess vào texture thay vì default framebuffer.
    tglmt::GLuint ftex = 0, fbo = 0;
    glGenTextures(1, &ftex);
    glBindTexture(0x0DE1, ftex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, ftex, 0);
    glBindTexture(0x0DE1, tex); // trả unit0 về texture đỏ cho uTex (tránh feedback)
    tglmt::GLenum fbs = glCheckFramebufferStatus(0x8D40);
    if (fbs != 0x8CD5) {
        printf("FAIL tess FBO incomplete: 0x%x\n", (unsigned)fbs);
        return 1;
    }
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E /*PATCHES*/, 3, 3);
    glFinish();
    unsigned char fpx[64 * 64 * 4];
    memset(fpx, 0, sizeof(fpx));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, fpx);
    unsigned char* fmid = fpx + (32 * 64 + 32) * 4;
    printf("fbo mid=(%u,%u,%u,%u)\n", fmid[0], fmid[1], fmid[2], fmid[3]);
    if (!(fmid[0] > 200 && fmid[1] < 50 && fmid[2] < 50)) {
        printf("FAIL tess FBO center not red\n");
        return 1;
    }
    glBindFramebuffer(0x8D40, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &ftex);
    // MSAA tess target: pipeline phải có rasterSampleCount = 4, resolve blit đọc đỏ.
    tglmt::GLuint msaaTex = 0, msaaFbo = 0, resTex = 0, resFbo = 0;
    glGenTextures(1, &msaaTex);
    glBindTexture(0x9100 /*TEXTURE_2D_MULTISAMPLE*/, msaaTex);
    glTexImage2DMultisample(0x9100, 4, 0x8058, 64, 64, 1);
    glGenFramebuffers(1, &msaaFbo);
    glBindFramebuffer(0x8D40, msaaFbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x9100, msaaTex, 0);
    glBindTexture(0x0DE1, tex);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5) {
        printf("FAIL tess MSAA FBO incomplete\n");
        return 1;
    }
    glGenTextures(1, &resTex);
    glBindTexture(0x0DE1, resTex);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &resFbo);
    glBindFramebuffer(0x8D40, resFbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, resTex, 0);
    glBindTexture(0x0DE1, tex);
    glBindFramebuffer(0x8D40, msaaFbo);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E, 3, 3);
    glFinish();
    glBindFramebuffer(0x8CA8 /*READ_FRAMEBUFFER*/, msaaFbo);
    glBindFramebuffer(0x8CA9 /*DRAW_FRAMEBUFFER*/, resFbo);
    glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64, 0x00004000, 0x2600 /*NEAREST*/);
    glBindFramebuffer(0x8D40, resFbo);
    unsigned char mpx[64 * 64 * 4];
    memset(mpx, 0, sizeof(mpx));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, mpx);
    unsigned char* mmid = mpx + (32 * 64 + 32) * 4;
    printf("msaa mid=(%u,%u,%u,%u)\n", mmid[0], mmid[1], mmid[2], mmid[3]);
    if (!(mmid[0] > 200 && mmid[1] < 50 && mmid[2] < 50)) {
        printf("FAIL tess MSAA resolve center not red\n");
        return 1;
    }
    glBindFramebuffer(0x8D40, 0);
    glDeleteFramebuffers(1, &msaaFbo);
    glDeleteFramebuffers(1, &resFbo);
    glDeleteTextures(1, &msaaTex);
    glDeleteTextures(1, &resTex);
    // MRT tess: 2 color attachment; FS location0 đỏ, location1 xanh lá.
    tglmt::GLuint mt0 = 0, mt1 = 0, mfbo2 = 0;
    glGenTextures(1, &mt0);
    glBindTexture(0x0DE1, mt0);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenTextures(1, &mt1);
    glBindTexture(0x0DE1, mt1);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glGenFramebuffers(1, &mfbo2);
    glBindFramebuffer(0x8D40, mfbo2);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, mt0, 0);
    glFramebufferTexture2D(0x8D40, 0x8CE1, 0x0DE1, mt1, 0);
    glBindTexture(0x0DE1, tex);
    tglmt::GLenum bufs2[2] = {0x8CE0, 0x8CE1};
    glDrawBuffers(2, bufs2);
    if (glCheckFramebufferStatus(0x8D40) != 0x8CD5) {
        printf("FAIL tess MRT FBO incomplete\n");
        return 1;
    }
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    glDrawArrays(0x000E, 3, 3);
    glFinish();
    unsigned char m0[4] = {0}, m1[4] = {0};
    glReadBuffer(0x8CE0);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, m0);
    glReadBuffer(0x8CE1);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, m1);
    printf("mrt0=(%u,%u,%u) mrt1=(%u,%u,%u)\n", m0[0], m0[1], m0[2], m1[0], m1[1], m1[2]);
    if (!(m0[0] > 200 && m0[1] < 50 && m0[2] < 50 && m1[0] < 50 && m1[1] > 200 && m1[2] < 50)) {
        printf("FAIL tess MRT color attachment mismatch\n");
        return 1;
    }
    glBindFramebuffer(0x8D40, 0);
    glDeleteFramebuffers(1, &mfbo2);
    glDeleteTextures(1, &mt0);
    glDeleteTextures(1, &mt1);
    // Instancing: 2 instance, per-instance attrib divisor=1 đẩy tam giác sang trái/phải.
    tglmt::GLuint ivbo = 0;
    const float instData[] = {-2.0f, 0.0f, 1.0f, 0.0f};
    glGenBuffers(1, &ivbo);
    glBindBuffer(0x8892, ivbo);
    glBufferData(0x8892, sizeof(instData), instData, 0x88E4);
    glBindVertexArray(vao);
    glBindBuffer(0x8892, ivbo);
    glVertexAttribBinding(2, 1);
    glVertexAttribPointer(2, 2, 0x1406, 0, 8, (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    glVertexBindingDivisor(1, 1);
    glBindFramebuffer(0x8D40, 0);
    glViewport(0, 0, 64, 64);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(0x00004000);
    unsigned int cBefore = 0;
    glBindBuffer(0x92C0, acbuf);
    glGetBufferSubData(0x92C0, 0, sizeof(cBefore), &cBefore);
    glDrawArraysInstanced(0x000E, 3, 3, 2);
    glFinish();
    unsigned int cAfter = 0;
    glGetBufferSubData(0x92C0, 0, sizeof(cAfter), &cAfter);
    unsigned char ipx[64 * 64 * 4];
    memset(ipx, 0, sizeof(ipx));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, ipx);
    unsigned char* il = ipx + (32 * 64 + 4) * 4;
    unsigned char* ir = ipx + (32 * 64 + 60) * 4;
    printf("inst left=(%u,%u,%u) right=(%u,%u,%u) center=(%u,%u,%u) dCounter=%u err=%x\n",
           il[0], il[1], il[2], ir[0], ir[1], ir[2],
           ipx[(32*64+32)*4], ipx[(32*64+32)*4+1], ipx[(32*64+32)*4+2],
           cAfter - cBefore, glGetError());
    if (!(il[0] > 200 && il[1] < 50 && il[2] < 50 &&
          ir[0] > 200 && ir[1] < 50 && ir[2] < 50 &&
          cAfter - cBefore == 6)) {
        printf("FAIL tess instancing (per-instance attr hoặc dispatch)\n");
        return 1;
    }
    glVertexAttribDivisor(2, 0);
    glDisableVertexAttribArray(2);
    glDeleteBuffers(1, &ivbo);
    glDeleteBuffers(1, &acbuf);
    glDeleteBuffers(1, &ssbo);
    glDeleteBuffers(1, &ubo);
    glDeleteTextures(1, &tex);
    printf("test_tess_vs_capture_apple PASS (VS capture + gl_in + indexed baseVertex + per-stage uniforms/samplers/UBO/SSBO/image/atomic + TCS gl_out->TES gl_in + FBO/MSAA/MRT + instancing)\n");
    return 0;
}
