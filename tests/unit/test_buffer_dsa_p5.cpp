// test_buffer_dsa_p5.cpp — P5: ARB_buffer_storage + ARB_direct_state_access.
// Verify behavior thật (storage immutable, DSA texture/buffer/framebuffer).
// Chạy cả 2 backend (Null: verify state/shadow; Apple: verify GPU upload).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_buffer_dsa_p5 FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context& c = Context::Current();

    // ===== ARB_buffer_storage: immutable buffer + flags =====
    {
        GLuint buf = 0;
        glGenBuffers(1, &buf);
        float data[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        glBindBuffer(0x8892 /*ARRAY_BUFFER*/, buf);
        glBufferStorage(0x8892, sizeof(data), data, 0x0001 /*MAP_READ_BIT*/);
        Check(glGetError() == 0, "BufferStorage lỗi");
        // Size phải đúng.
        GLint size = 0;
        glGetBufferParameteriv(0x8892, 0x8764 /*BUFFER_SIZE*/, &size);
        Check(size == (GLint)sizeof(data), "BufferStorage size sai");
        // Immutable: BufferData sau Storage phải lỗi (INVALID_OPERATION).
        glBufferData(0x8892, sizeof(data), nullptr, 0x88E4);
        Check(glGetError() == 0x0502, "BufferData sau Storage phải INVALID_OPERATION");
        // Readback verify data.
        glBindBuffer(0x8892, buf);
        const float* r = (const float*)glMapBufferRange(0x8892, 0, sizeof(data), 0x0001);
        Check(r && r[0] == 1.0f && r[3] == 4.0f, "BufferStorage readback sai");
        if (r) glUnmapBuffer(0x8892);
        glDeleteBuffers(1, &buf);
    }

    // ===== DSA texture: create + storage + subimage + param (không bind) =====
    {
        GLuint tex = 0;
        glCreateTextures(0x0DE1 /*TEXTURE_2D*/, 1, &tex);
        Check(tex != 0 && glGetError() == 0, "CreateTextures lỗi");
        glTextureStorage2D(tex, 1, 0x8058 /*RGBA8*/, 4, 4);
        Check(glGetError() == 0, "TextureStorage2D lỗi");
        unsigned char px[4 * 4 * 4];
        for (int i = 0; i < 4 * 4; ++i) { px[i*4]=255; px[i*4+1]=0; px[i*4+2]=0; px[i*4+3]=255; }
        glTextureSubImage2D(tex, 0, 0, 0, 4, 4, 0x1908 /*RGBA*/, 0x1401 /*UBYTE*/, px);
        Check(glGetError() == 0, "TextureSubImage2D lỗi");
        glTextureParameteri(tex, 0x2801 /*MIN_FILTER*/, 0x2601 /*LINEAR*/);
        Check(glGetError() == 0, "TextureParameteri lỗi");
        // Verify param.
        GLint f = 0;
        glGetTextureParameteriv(tex, 0x2801, &f);
        Check(f == 0x2601, "TextureParameteri round-trip sai");
        // Texture không bind (DSA) — BoundTexture(0) phải khác tex.
        GLuint bound = c.state.BoundTexture(0, 0x0DE1);
        Check(bound != tex, "DSA không được bind texture");
        glDeleteTextures(1, &tex);
    }

    // ===== DSA buffer: create + named data/subdata (không bind) =====
    {
        GLuint buf = 0;
        glCreateBuffers(1, &buf);
        Check(buf != 0, "CreateBuffers lỗi");
        float data[8] = {0};
        glNamedBufferData(buf, sizeof(data), data, 0x88E4);
        Check(glGetError() == 0, "NamedBufferData lỗi");
        float sub[2] = {9.0f, 8.0f};
        glNamedBufferSubData(buf, 0, sizeof(sub), sub);
        Check(glGetError() == 0, "NamedBufferSubData lỗi");
        // Buffer không bind — BoundBuffer phải khác buf (hoặc 0).
        GLuint bound = c.state.BoundBuffer(0x8892);
        Check(bound != buf, "DSA không được bind buffer");
        glDeleteBuffers(1, &buf);
    }

    // ===== DSA framebuffer: create + texture attach + check status =====
    {
        GLuint fb = 0, tex = 0;
        glCreateFramebuffers(1, &fb);
        glCreateTextures(0x0DE1, 1, &tex);
        glTextureStorage2D(tex, 1, 0x8058, 4, 4);
        glNamedFramebufferTexture(fb, 0x8CE0 /*COLOR_ATTACHMENT0*/, tex, 0);
        Check(glGetError() == 0, "NamedFramebufferTexture lỗi");
        GLenum st = glCheckNamedFramebufferStatus(fb, 0x8D40 /*FRAMEBUFFER*/);
        Check(st == 0x8CD5 /*COMPLETE*/, "NamedFramebufferStatus không COMPLETE");
        glDeleteFramebuffers(1, &fb);
        glDeleteTextures(1, &tex);
    }

    if (gFails == 0) { printf("test_buffer_dsa_p5: all OK\n"); return 0; }
    printf("test_buffer_dsa_p5: %d FAIL\n", gFails);
    return 1;
}
