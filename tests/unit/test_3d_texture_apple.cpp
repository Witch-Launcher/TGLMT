// test_3d_texture_apple.cpp — P3 texture 1D/3D (Iris custom textures):
// glTexImage3D/glTexSubImage3D thật (MTLTextureType3D), sampler3D (texture3d),
// textureSize(3D) get_depth, texture() rewrite 3D; sampler1D ≈ texture2d w×1
// (coord y=0.5). Căn cứ Iris 26.1: CustomTextureManager tạo TEXTURE_1D/3D +
// ProgramUniforms khai sampler1D/sampler3D; TextureType.apply() gọi texImage1D/3D.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_3d_texture_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

// FS: r = 3D sample (hoặc texelFetch 3D khi uMode=1), g = textureSize.z, b = 1D sample.
static GLuint MakeProgram() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 o;\n"
        "uniform sampler3D uTex3D;\n"
        "uniform sampler1D uTex1D;\n"
        "uniform float uZ;\n"
        "uniform float uMode;\n"
        "void main() {\n"
        "  vec4 a = texture(uTex3D, vec3(0.5, 0.5, uZ));\n"
        "  vec4 tf = texelFetch(uTex3D, ivec3(1, 1, 2), 0);\n"
        "  vec4 b = texture(uTex1D, 0.625);\n"
        "  ivec3 s = textureSize(uTex3D, 0);\n"
        "  o = vec4(uMode < 0.5 ? a.r : tf.r, float(s.z) / 16.0, b.r, 1.0);\n"
        "}\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v);
    glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(f, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(f, sizeof(log) - 1, nullptr, log);
        printf("test_3d_texture_apple: FS compile fail:\n%s\n", log);
        return 0;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetProgramInfoLog(p, sizeof(log) - 1, nullptr, log);
        printf("test_3d_texture_apple: link fail:\n%s\n", log);
        return 0;
    }
    return p;
}

static bool Near(int v, int want, int tol = 8) { return v >= want - tol && v <= want + tol; }

int main() {
    Context apple("apple");
    Context::MakeCurrent(&apple);
    if (apple.device->isNull()) {
        printf("test_3d_texture_apple SKIP: no metal device\n");
        return 0;
    }

    // --- 3D texture 4x4x4: R của slice z = z*64 (0/64/128/192) ---
    GLuint t3d = 0;
    glGenTextures(1, &t3d);
    glActiveTexture(0x84C0);
    glBindTexture(0x806F /*GL_TEXTURE_3D*/, t3d);
    {
        std::vector<uint8_t> px((size_t)4 * 4 * 4 * 4, 0);
        for (int z = 0; z < 4; ++z)
            for (int i = 0; i < 16; ++i) {
                uint8_t* p = px.data() + ((size_t)z * 16 + i) * 4;
                p[0] = (uint8_t)(z * 64);
                p[3] = 255;
            }
        glTexImage3D(0x806F, 0, 0x8058 /*RGBA8*/, 4, 4, 4, 0, 0x1908, 0x1401, px.data());
    }
    glTexParameteri(0x806F, 0x2801, 0x2600); // MIN_FILTER NEAREST
    glTexParameteri(0x806F, 0x2800, 0x2600); // MAG_FILTER NEAREST
    Check(glGetError() == 0, "glTexImage3D không lỗi");

    // --- GetTexImage 3D: shadow được giữ lại để trả đủ volume (Metal không
    // cho view 3D→2D); kiểm tra slice 2 có R=128.
    {
        std::vector<uint8_t> got((size_t)4 * 4 * 4 * 4, 0);
        glGetTextureImage(t3d, 0, 0x1908, 0x1401, (GLsizei)got.size(), got.data());
        Check(glGetError() == 0, "glGetTextureImage 3D no error");
        Check(got[(((size_t)2 * 16) + 5) * 4] == 128,
              "glGetTextureImage 3D trả đúng slice z=2 (R=128)");
    }

    // --- 2D array: shadow bị bỏ (P1); GetTexImage đọc GPU từng layer.
    {
        GLuint arr = 0;
        glGenTextures(1, &arr);
        glActiveTexture(0x84C0);
        glBindTexture(0x8C1A /*TEXTURE_2D_ARRAY*/, arr);
        std::vector<uint8_t> px((size_t)2 * 2 * 3 * 4, 0);
        for (int z = 0; z < 3; ++z)
            for (int i = 0; i < 4; ++i) {
                uint8_t* q = px.data() + (((size_t)z * 4 + i) * 4);
                q[0] = (uint8_t)(z * 80 + 10);
                q[3] = 255;
            }
        glTexImage3D(0x8C1A, 0, 0x8058, 2, 2, 3, 0, 0x1908, 0x1401, px.data());
        Check(glGetError() == 0, "setup 2D array GPU");
        std::vector<uint8_t> got((size_t)2 * 2 * 3 * 4, 0);
        glGetTextureImage(arr, 0, 0x1908, 0x1401, (GLsizei)got.size(), got.data());
        Check(glGetError() == 0, "glGetTextureImage 2D array GPU no error");
        Check(got[(((size_t)2 * 4) + 3) * 4] == 170,
              "glGetTextureImage 2D array đọc đúng layer 2 (R=170)");
        glDeleteTextures(1, &arr);
        glBindTexture(0x806F, t3d);
    }

    // --- 1D texture 4 texel: R = x*64 ---
    GLuint t1d = 0;
    glGenTextures(1, &t1d);
    glActiveTexture(0x84C1);
    glBindTexture(0x0DE0 /*GL_TEXTURE_1D*/, t1d);
    {
        uint8_t px[4 * 4] = {0, 0, 0, 255, 64, 0, 0, 255, 128, 0, 0, 255, 192, 0, 0, 255};
        glTexImage1D(0x0DE0, 0, 0x8058, 4, 0, 0x1908, 0x1401, px);
    }
    glTexParameteri(0x0DE0, 0x2801, 0x2600);
    glTexParameteri(0x0DE0, 0x2800, 0x2600);
    Check(glGetError() == 0, "glTexImage1D không lỗi");

    // --- FBO 64x64 (tạo ở unit 0: slot 0 = 2D, không đụng slot 4 của t3d/unit1 của t1d) ---
    glActiveTexture(0x84C0);
    GLuint color = 0;
    glGenTextures(1, &color);
    glBindTexture(0x0DE1, color);
    {
        std::vector<uint8_t> px((size_t)64 * 64 * 4, 0);
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, px.data());
    }
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glGetError() == 0, "setup FBO không lỗi");

    GLuint prog = MakeProgram();
    if (!prog) return 1;
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    {
        float v[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(v), v, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    }
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uTex3D"), 0);
    glUniform1i(glGetUniformLocation(prog, "uTex1D"), 1);
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.625f); // giữa slice 2
    glUniform1f(glGetUniformLocation(prog, "uMode"), 0.0f);
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71);
    glDisable(0x0C11);
    glDisable(0x0BE2);
    glColorMask(1, 1, 1, 1);

    unsigned char p[4] = {0, 0, 0, 0};
    // (1) texture(sampler3D) slice 2 → R=128; textureSize(3D).z=4 → G=64; 1D texel2 → B=128.
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "draw 3D/1D no error");
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 128), "texture(sampler3D) đọc đúng slice z (R=128)");
    Check(Near(p[1], 64), "textureSize(sampler3D).z == 4 (G=64)");
    Check(Near(p[2], 128), "texture(sampler1D) đọc đúng texel 2 (B=128)");
    Check(p[3] == 255, "alpha 255");

    // (2) glTexSubImage3D slice 2 → R=200 (chỉ slice đó), texelFetch 3D đọc lại.
    glActiveTexture(0x84C0); // về unit 0 (tex 3D) — BoundTex theo active unit
    {
        std::vector<uint8_t> px((size_t)4 * 4 * 4, 0);
        for (int i = 0; i < 16; ++i) {
            uint8_t* q = px.data() + i * 4;
            q[0] = 200;
            q[3] = 255;
        }
        glTexSubImage3D(0x806F, 0, 0, 0, 2, 4, 4, 1, 0x1908, 0x1401, px.data());
    }
    Check(glGetError() == 0, "glTexSubImage3D không lỗi");
    glUniform1f(glGetUniformLocation(prog, "uMode"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "draw sau SubImage3D no error");
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 200), "texelFetch(sampler3D) đọc slice 2 sau SubImage (R=200)");
    Check(Near(p[1], 64), "textureSize(3D).z vẫn 4");
    Check(Near(p[2], 128), "1D không đổi (B=128)");

    // (3) SubImage3D ngoài biên → INVALID_VALUE (spec §8.5).
    glTexSubImage3D(0x806F, 0, 0, 0, 3, 4, 4, 2, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0x0501, "SubImage3D vượt depth → INVALID_VALUE");

    // (4) glCopyImageSubData 3D nhiều slice + offset z (blitCopySlices).
    GLuint t3dCopy = 0;
    glGenTextures(1, &t3dCopy);
    glBindTexture(0x806F, t3dCopy);
    glTexImage3D(0x806F, 0, 0x8058, 4, 4, 4, 0, 0x1908, 0x1401, nullptr);
    glTexParameteri(0x806F, 0x2801, 0x2600);
    glTexParameteri(0x806F, 0x2800, 0x2600);
    glCopyImageSubData(t3d, 0x806F, 0, 0, 0, 1, t3dCopy, 0x806F, 0, 0, 0, 1, 4, 4, 2);
    Check(glGetError() == 0, "glCopyImageSubData 3D d=2 no error");
    glBindTexture(0x806F, t3dCopy);
    glUniform1f(glGetUniformLocation(prog, "uMode"), 0.0f);
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.375f); // dst slice 1 = src slice 1
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 64), "copy 3D d=2 → dst slice 1 = src slice 1 (R=64)");
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.625f); // dst slice 2 = src slice 2
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 200), "copy 3D d=2 → dst slice 2 = src slice 2 (R=200)");
    glDeleteTextures(1, &t3dCopy);

    // (5) Clear 3D trên GPU: clear slice 3, sample đúng slice; clear toàn volume.
    glBindTexture(0x806F, t3d);
    {
        unsigned char red[4] = {255, 0, 0, 255};
        glClearTexSubImage(t3d, 0, 0, 0, 3, 4, 4, 1, 0x1908, 0x1401, red);
    }
    Check(glGetError() == 0, "clearTexSubImage slice 3 GPU no error");
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.875f); // slice 3
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 255), "clearTexSubImage slice 3 → sample R=255");
    glClearTexImage(t3d, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "glClearTexImage 3D GPU no error");
    glUniform1f(glGetUniformLocation(prog, "uZ"), 0.625f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, p);
    Check(p[0] == 0, "glClearTexImage 3D clear slice 2 → sample R=0");

    if (gFails) {
        printf("test_3d_texture_apple RESULT: FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_3d_texture_apple RESULT: PASS\n");
    return 0;
}
