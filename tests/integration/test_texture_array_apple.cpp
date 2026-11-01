// test_texture_array_apple.cpp — GL_TEXTURE_2D_ARRAY storage/upload/sampling:
// glTexImage3D + glTexSubImage3D + glTexStorage3D trên MTLTextureType2DArray,
// sampler2DArray → texture2d_array<float>, textureSize().z = số layer.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_texture_array_apple FAIL: %s\n", msg); ++gFails; }
}
static bool Near(int v, int want, int tol = 8) { return v >= want - tol && v <= want + tol; }

static GLuint MakeProgram() {
    const char* vs =
        "#version 460 core\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    const char* fs =
        "#version 460 core\n"
        "out vec4 o;\n"
        "uniform sampler2DArray uArr;\n"
        "uniform float uLayer;\n"
        "void main() {\n"
        "  vec4 a = texture(uArr, vec3(0.5, 0.5, uLayer));\n"
        "  ivec3 s = textureSize(uArr, 0);\n"
        "  o = vec4(a.r, float(s.z) / 8.0, a.g, 1.0);\n"
        "}\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v); glCompileShader(f);
    GLint ok = 0;
    glGetShaderiv(f, 0x8B81, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetShaderInfoLog(f, sizeof(log) - 1, nullptr, log);
        printf("test_texture_array_apple: FS compile fail:\n%s\n", log);
        return 0;
    }
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) {
        char log[2048] = {0};
        glGetProgramInfoLog(p, sizeof(log) - 1, nullptr, log);
        printf("test_texture_array_apple: link fail:\n%s\n", log);
        return 0;
    }
    return p;
}

int main() {
    Context apple("apple");
    Context::MakeCurrent(&apple);
    if (apple.device->isNull()) {
        printf("test_texture_array_apple SKIP: no metal device\n");
        return 0;
    }

    // Array 4x4 x 3 layer: R = layer*80, G = 64 (layer count marker không dùng).
    GLuint arr = 0;
    glGenTextures(1, &arr);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A /*TEXTURE_2D_ARRAY*/, arr);
    {
        std::vector<uint8_t> px((size_t)4 * 4 * 3 * 4, 0);
        for (int z = 0; z < 3; ++z)
            for (int i = 0; i < 16; ++i) {
                uint8_t* p = px.data() + ((size_t)z * 16 + i) * 4;
                p[0] = (uint8_t)(z * 80); p[3] = 255;
            }
        glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 3, 0, 0x1908, 0x1401, px.data());
    }
    glTexParameteri(0x8C1A, 0x2801, 0x2600);
    glTexParameteri(0x8C1A, 0x2800, 0x2600);
    Check(glGetError() == 0, "glTexImage3D 2D_ARRAY không lỗi");

    // FBO 32x32, color ở unit 0 (tách slot khỏi array unit 1).
    glActiveTexture(0x84C0);
    GLuint color = 0;
    glGenTextures(1, &color);
    glBindTexture(0x0DE1, color);
    std::vector<uint8_t> zero((size_t)32 * 32 * 4, 0);
    glTexImage2D(0x0DE1, 0, 0x8058, 32, 32, 0, 0x1908, 0x1401, zero.data());
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8D40, fbo);
    glFramebufferTexture2D(0x8D40, 0x8CE0, 0x0DE1, color, 0);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glGetError() == 0, "FBO setup không lỗi");

    GLuint prog = MakeProgram();
    if (!prog) return 1;
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    float v[12] = {-1, -1, 1, -1, 1, 1, -1, -1, 1, 1, -1, 1};
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(v), v, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uArr"), 1);
    glViewport(0, 0, 32, 32);
    glDisable(0x0B71); glDisable(0x0C11); glDisable(0x0BE2);
    glColorMask(1, 1, 1, 1);

    unsigned char p[4] = {0, 0, 0, 0};
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 2.0f);
    glDrawArrays(0x0004, 0, 6);
    Check(glGetError() == 0, "draw sampler2DArray no error");
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 160), "texture(sampler2DArray) layer 2 → R=160");
    Check(Near(p[1], 96), "textureSize(sampler2DArray).z == 3 → G=96");

    // SubImage layer 1 → R=200; layer 0 giữ 0, layer 2 giữ 160.
    glActiveTexture(0x84C1);
    {
        std::vector<uint8_t> px((size_t)4 * 4 * 4, 0);
        for (int i = 0; i < 16; ++i) { px[i * 4] = 200; px[i * 4 + 3] = 255; }
        glTexSubImage3D(0x8C1A, 0, 0, 0, 1, 4, 4, 1, 0x1908, 0x1401, px.data());
    }
    Check(glGetError() == 0, "glTexSubImage3D 2D_ARRAY không lỗi");
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 200), "SubImage layer 1 → R=200");
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 2.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 160), "SubImage không đụng layer 2 (R=160)");

    // Immutable storage path: glTexStorage3D(2D_ARRAY) + SubImage từng layer.
    GLuint arr2 = 0;
    glGenTextures(1, &arr2);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arr2);
    glTexStorage3D(0x8C1A, 1, 0x8058, 4, 4, 2);
    Check(glGetError() == 0, "glTexStorage3D 2D_ARRAY không lỗi");
    for (int z = 0; z < 2; ++z) {
        std::vector<uint8_t> px((size_t)4 * 4 * 4, 0);
        for (int i = 0; i < 16; ++i) { px[i * 4] = (uint8_t)(40 + z * 40); px[i * 4 + 3] = 255; }
        glTexSubImage3D(0x8C1A, 0, 0, 0, z, 4, 4, 1, 0x1908, 0x1401, px.data());
        Check(glGetError() == 0, "SubImage immutable array không lỗi");
    }
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 80), "immutable array layer 1 → R=80");
    Check(Near(p[1], 64), "textureSize immutable = 2 → G=64");

    // glCopyImageSubData giữa hai 2D array: copy 2 layer qua blitCopySlices.
    GLuint arrCopy = 0;
    glGenTextures(1, &arrCopy);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arrCopy);
    glTexImage3D(0x8C1A, 0, 0x8058, 4, 4, 2, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "array copy storage không lỗi");
    glCopyImageSubData(arr, 0x8C1A, 0, 0, 0, 0, arrCopy, 0x8C1A, 0, 0, 0, 0,
                       4, 4, 2);
    Check(glGetError() == 0, "glCopyImageSubData 2D_ARRAY d=2 không lỗi");
    glBindTexture(0x8C1A, arrCopy);
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 200), "copy array layer 1 → R=200");

    // FBO render vào MỘT layer 2D array (glFramebufferTextureLayer) → sample lại.
    GLuint arrRT = 0;
    glGenTextures(1, &arrRT);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arrRT);
    glTexImage3D(0x8C1A, 0, 0x8058, 8, 8, 2, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "array RT storage không lỗi");
    GLuint fboArr = 0;
    glGenFramebuffers(1, &fboArr);
    glBindFramebuffer(0x8D40, fboArr);
    glFramebufferTextureLayer(0x8D40, 0x8CE0, arrRT, 0, 1);
    glDrawBuffers(1, (const GLenum[]){0x8CE0});
    Check(glGetError() == 0, "glFramebufferTextureLayer array không lỗi");
    glViewport(0, 0, 8, 8);
    glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
    glClear(0x4000);
    Check(glGetError() == 0, "clear layer array không lỗi");

    glBindFramebuffer(0x8D40, fbo);
    glViewport(0, 0, 32, 32);
    glActiveTexture(0x84C1);
    glBindTexture(0x8C1A, arrRT);
    glUniform1i(glGetUniformLocation(prog, "uArr"), 1);
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 1.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 255), "render layer 1 array → R=255");
    glUniform1f(glGetUniformLocation(prog, "uLayer"), 0.0f);
    glDrawArrays(0x0004, 0, 6);
    glReadPixels(16, 16, 1, 1, 0x1908, 0x1401, p);
    Check(Near(p[0], 0), "layer 0 array không bị clear");

    if (gFails) {
        printf("test_texture_array_apple RESULT: FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_texture_array_apple RESULT: PASS\n");
    return 0;
}
