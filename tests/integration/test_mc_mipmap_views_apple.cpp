// test_mc_mipmap_views_apple.cpp — Mip-view FBO + sampler mutation (MC 26.1.2).
//
// Căn cứ client.jar 26.1.2:
//  - TextureAtlas bake animation per-mip: GlTextureView(texture, level, 1) →
//    FBO attach level>0. Texture Metal của TGLMT chỉ có 1 level nên draw vào
//    mip view PHẢI skip (alias level 0 = smear atlas → mất texture sau reload).
//  - GlSampler: min NEAREST→0x2702 / LINEAR→0x2703, mag NEAREST→0x2600.
//    SamplerCache bất biến theo id; nếu game đổi param trên cùng id (video
//    settings), Metal sampler cache PHẢI theo params (key sid-only = stale).
// Thiếu GPU → SKIP. Sai pixel/stat → FAIL thật.
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
        printf("test_mc_mipmap_views_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec2 uv;\n"
    "layout(location = 0) out vec2 vUV;\n"
    "void main() { vUV = uv; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform sampler2D Sampler0;\n"
    "void main() { fragColor = texture(Sampler0, vUV); }\n";
static const char* kFSBlue =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vec4(0.0, 0.0, 1.0, 1.0); }\n";

static GLuint MakeProg(const char* fsSrc) {
    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glBindAttribLocation(prog, 1, "uv");
    glLinkProgram(prog);
    return prog;
}

static GLuint gVao = 0, gVbo = 0, gEbo = 0;
static void Quad(float x0, float y0, float x1, float y1) {
    float q[16] = {x0, y0, 0, 0, x1, y0, 1, 0, x1, y1, 1, 1, x0, y1, 0, 1};
    glBindBuffer(0x8892, gVbo);
    glBufferData(0x8892, sizeof(q), q, 0x88E4);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_mipmap_views_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_mipmap_views_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    glGenVertexArrays(1, &gVao);
    glBindVertexArray(gVao);
    glGenBuffers(1, &gVbo);
    glBindBuffer(0x8892, gVbo);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &gEbo);
    glBindBuffer(0x8893, gEbo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, 0x1406, 0, 16, (void*)0);
    glVertexAttribPointer(1, 2, 0x1406, 0, 16, (void*)8);

    // ---------- A. draw vào mip view level>0 của texture 1-level → skip ----------
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    std::vector<unsigned char> red(64 * 64 * 4);
    for (size_t i = 0; i < 64 * 64; ++i) {
        red[i * 4] = 255; red[i * 4 + 1] = 0; red[i * 4 + 2] = 0; red[i * 4 + 3] = 255;
    }
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, red.data());
    GLuint mipFbo = 0;
    glGenFramebuffers(1, &mipFbo);
    glBindFramebuffer(0x8CA9, mipFbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, tex, 1);
    Check(glGetError() == 0, "mip attach no error");

    GLuint blueProg = MakeProg(kFSBlue);
    {
        GLint ok = 0;
        glGetProgramiv(blueProg, 0x8B82, &ok);
        Check(ok != 0, "blue link");
    }
    glUseProgram(blueProg);
    uint64_t skipBefore = ctx.appleStats.mipLevelSkipped;
    uint64_t encBefore = ctx.appleStats.drawsEncoded;
    Quad(-1, -1, 1, 1);
    Check(glGetError() == 0, "mip draw no gl error");
    Check(ctx.appleStats.mipLevelSkipped == skipBefore + 1, "mip view draw skipped");
    Check(ctx.appleStats.drawsEncoded == encBefore, "skipped draw not encoded");

    glBindFramebuffer(0x8CA9, 0);
    {
        auto it = ctx.textures.find(tex);
        Check(it != ctx.textures.end(), "tex exists");
        if (it != ctx.textures.end() && it->second.gpu) {
            ctx.FlushPendingEncoder();
            ctx.CommitAndWait();
            auto wt = ctx.device->wrapAsTarget(it->second.gpu.get(), nullptr);
            std::vector<unsigned char> fb(64 * 64 * 4, 0);
            bool ok = wt && wt->readback(fb.data(), 64 * 4);
            Check(ok, "gpu readback ok");
            if (ok)
                Check(fb[0] == 255 && fb[1] == 0 && fb[2] == 0, "base intact (still red)");
        }
    }
    if (gFails) return 1;

    // ---------- B. sampler mutation (MC GlSampler enum thật) ----------
    // Texture 2x1 [đen, trắng], phóng đại fullscreen:
    // MAG NEAREST (0x2600): biên cứng (spread 255); MAG LINEAR (0x2601): mịn.
    // Đổi MAG trên CÙNG sampler id phải đổi kết quả (key stale → FAIL).
    GLuint bw = 0;
    glGenTextures(1, &bw);
    glBindTexture(0x0DE1, bw);
    unsigned char pixels[8] = {0, 0, 0, 255, 255, 255, 255, 255};
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 1, 0, 0x1908, 0x1401, pixels);
    GLuint samp = 0;
    glGenSamplers(1, &samp);
    glSamplerParameteri(samp, 0x2801, 0x2703); // MIN LINEAR_MIPMAP (MC LINEAR)
    glSamplerParameteri(samp, 0x2800, 0x2600); // MAG NEAREST (MC NEAREST)
    glBindSampler(0, samp);

    GLuint texProg = MakeProg(kFS);
    {
        GLint ok = 0;
        glGetProgramiv(texProg, 0x8B82, &ok);
        Check(ok != 0, "tex link");
    }
    glUseProgram(texProg);
    glViewport(0, 0, 64, 64);
    auto boundarySpread = [&]() {
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        Quad(-1, -1, 1, 1);
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned left = px[(32 * 64 + 31) * 4];
        unsigned right = px[(32 * 64 + 32) * 4];
        printf("[samp] boundary L=%u R=%u err=%d\n", left, right, glGetError());
        return left > right ? (int)(left - right) : (int)(right - left);
    };
    int spreadNearest = boundarySpread();
    glSamplerParameteri(samp, 0x2800, 0x2601); // MAG LINEAR, cùng id
    int spreadLinear = boundarySpread();
    printf("[samp] spread NEAREST=%d LINEAR=%d\n", spreadNearest, spreadLinear);
    Check(spreadNearest > 150, "mag nearest hard edge");
    Check(spreadLinear < 100, "mag linear smooth — stale sampler if FAIL");
    if (gFails) return 1;

    // ---------- C. MIN 0x2702/0x2703 (MC dùng cho atlas) minify phải VISIBLE ----------
    // Sọc 1px 64px → quad 8px: cả 2 filter đều phải ra pixel khác clear (xanh).
    GLuint stripes = 0;
    glGenTextures(1, &stripes);
    glBindTexture(0x0DE1, stripes);
    std::vector<unsigned char> sp(64 * 64 * 4);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            unsigned char v = (x & 1) ? 255 : 0;
            sp[(y * 64 + x) * 4 + 0] = v; sp[(y * 64 + x) * 4 + 1] = v;
            sp[(y * 64 + x) * 4 + 2] = v; sp[(y * 64 + x) * 4 + 3] = 255;
        }
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, sp.data());
    auto minVisible = [&](GLint minF) {
        glSamplerParameteri(samp, 0x2801, minF);
        glSamplerParameteri(samp, 0x2800, 0x2601);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        Quad(-0.125f, -0.125f, 0.125f, 0.125f);
        unsigned char px[64 * 64 * 4];
        memset(px, 0, sizeof(px));
        glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
        unsigned c = px[(32 * 64 + 32) * 4 + 2]; // kênh B: quad render → B~vải, nền → 255
        printf("[samp] min=0x%x centerB=%u\n", minF, c);
        return c;
    };
    unsigned c2702 = minVisible(0x2702);
    unsigned c2703 = minVisible(0x2703);
    Check(c2702 != 255, "min 0x2702 renders (not clear-blue)");
    Check(c2703 != 255, "min 0x2703 renders (not clear-blue)");
    if (gFails) return 1;

    printf("test_mc_mipmap_views_apple PASS\n");
    return 0;
}
