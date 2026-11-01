// probe_gaps.cpp — Runtime truth probe for the static gaps in
// tests/truth/api_manifest.json. This is diagnostic, not a CTest gate:
// known gaps print GAP and exit 0; a future fix flips them to PASS.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gPass = 0, gGap = 0;
static void Probe(const char* name, bool supported, const char* detail) {
    printf("%-42s %s  %s\n", name, supported ? "PASS" : "GAP ", detail);
    if (supported) ++gPass; else ++gGap;
}

static void ClearErrors() { while (glGetError() != 0) {} }

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "out vec4 color;\n"
    "void main() { color = vec4(1.0); }\n";

static GLuint MakeProgram(const char* fs = kFS) {
    GLuint vs = glCreateShader(0x8B31);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint f = glCreateShader(0x8B30);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, f);
    glLinkProgram(p);
    return p;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    printf("=== TGLMT runtime truth probe (null backend) ===\n");

    // 1. Program pipeline / SSO state.
    {
        ClearErrors();
        GLuint pipe = 0, prog = MakeProgram();
        glGenProgramPipelines(1, &pipe);
        glBindProgramPipeline(pipe);
        glUseProgramStages(pipe, 0x00000001u | 0x00000002u, prog); // VS|FS
        glActiveShaderProgram(pipe, prog);
        GLint active = 0;
        glGetProgramPipelineiv(pipe, 0x8259 /*ACTIVE_PROGRAM*/, &active);
        char d[96];
        snprintf(d, sizeof(d), "ACTIVE_PROGRAM=%d expected=%u", active, prog);
        Probe("program pipeline state", active == (GLint)prog, d);
        glDeleteProgramPipelines(1, &pipe);
        glDeleteProgram(prog);
    }

    // 2. glBindFragDataLocation must affect the next link.
    {
        ClearErrors();
        GLuint prog = MakeProgram();
        glBindFragDataLocation(prog, 1, "color");
        glLinkProgram(prog);
        GLint loc = glGetFragDataLocation(prog, "color");
        char d[96];
        snprintf(d, sizeof(d), "location=%d expected=1", loc);
        Probe("glBindFragDataLocation", loc == 1, d);
        glDeleteProgram(prog);
    }

    // 3. Compressed formats: either a supported format round-trips, or the
    // implementation honestly rejects the call (INVALID_ENUM).
    {
        ClearErrors();
        GLint n = 0;
        glGetIntegerv(0x86A2 /*NUM_COMPRESSED_TEXTURE_FORMATS*/, &n);
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE1, tex);
        unsigned char data[64] = {};
        glCompressedTexImage2D(0x0DE1, 0, 0x84EE /*COMPRESSED_RGBA*/, 4, 4, 0, 64, data);
        GLenum err = glGetError();
        GLint w = 0;
        glGetTexLevelParameteriv(0x0DE1, 0, 0x1000 /*WIDTH*/, &w);
        char d[128];
        snprintf(d, sizeof(d), "formats=%d err=0x%x width=%d", n, err, w);
        bool ok = (n == 0) ? (err == 0x0500 /*INVALID_ENUM*/) : (err == 0 && w == 4);
        Probe("compressed texture contract", ok, d);
        glDeleteTextures(1, &tex);
    }

    // 4. MSAA storage through the GL API.
    {
        ClearErrors();
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x9100 /*TEXTURE_2D_MULTISAMPLE*/, tex);
        glTexImage2DMultisample(0x9100, 4, 0x8058 /*RGBA8*/, 8, 8, 1);
        GLenum err = glGetError();
        GLint samples = 0;
        glGetTexLevelParameteriv(0x9100, 0, 0x9106 /*TEXTURE_SAMPLES*/, &samples);
        char d[128];
        snprintf(d, sizeof(d), "err=0x%x samples=%d expected=4", err, samples);
        Probe("glTexImage2DMultisample", err == 0 && samples == 4, d);
        glDeleteTextures(1, &tex);
    }

    // 5. Phase 1 regression: DSA 1D upload + texture barrier contract.
    {
        ClearErrors();
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(0x0DE0, tex);
        const unsigned char base[4][4] = {{1,0,0,255},{2,0,0,255},{3,0,0,255},{4,0,0,255}};
        const unsigned char patch[1][4] = {{99,0,0,255}};
        glTexImage1D(0x0DE0, 0, 0x8058, 4, 0, 0x1908, 0x1401, base);
        glTextureSubImage1D(tex, 0, 2, 1, 0x1908, 0x1401, patch);
        unsigned char out[4][4] = {};
        glGetTexImage(0x0DE0, 0, 0x1908, 0x1401, out);
        bool ok = glGetError() == 0 && memcmp(out[2], patch, 4) == 0;
        Probe("glTextureSubImage1D", ok, "DSA 1D upload at offset 2");
        glDeleteTextures(1, &tex);
    }
    {
        ClearErrors();
        glTextureBarrier();
        Probe("glTextureBarrier contract", glGetError() == 0, "no GL error on null backend");
    }

    printf("--- %d pass, %d gap(s) ---\n", gPass, gGap);
    return 0;
}
