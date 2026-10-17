// test_ssbo_graphics_apple.cpp — P4 SSBO trong graphics stage (Iris ShaderStorageBufferHolder):
// converter nhận `buffer` block ở vs/fs (slot 17+k chung UBO), backend bind buffer
// device thật; glShaderStorageBlockBinding override layout(binding=N) áp cả graphics
// + compute. Căn cứ Iris 26.1: supportsSSBO() = OpenGL44 true (TGLMT báo 4.6) →
// pack có `buffers` sẽ thật sự bind SSBO base; nếu converter fail là shaderpack chết.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_ssbo_graphics_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static bool Near(int v, int want, int tol = 10) { return v >= want - tol && v <= want + tol; }

static GLuint MakeProg(const char* vs, const char* fs) {
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
        printf("test_ssbo_graphics_apple: FS compile fail:\n%s\n", log);
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
        printf("test_ssbo_graphics_apple: link fail:\n%s\n", log);
        return 0;
    }
    return p;
}

static GLuint MakeSSBO(const float* v, size_t bytes) {
    GLuint b = 0;
    glGenBuffers(1, &b);
    glBindBuffer(0x90D2, b);
    glBufferData(0x90D2, bytes, v, 0x88E4);
    return b;
}

int main() {
    Context apple("apple");
    Context::MakeCurrent(&apple);
    if (apple.device->isNull()) {
        printf("test_ssbo_graphics_apple SKIP: no metal device\n");
        return 0;
    }

    // FBO 64x64
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
    glViewport(0, 0, 64, 64);
    glDisable(0x0B71);
    glDisable(0x0C11);
    glDisable(0x0BE2);
    glColorMask(1, 1, 1, 1);
    Check(glGetError() == 0, "setup no error");

    unsigned char px[4] = {0, 0, 0, 0};

    // --- Case 1: FS SSBO (struct + instance) → pixel = data.v ---
    {
        const char* vs =
            "#version 460 core\n"
            "layout(location = 0) in vec2 a_pos;\n"
            "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
        const char* fs =
            "#version 460 core\n"
            "out vec4 o;\n"
            "layout(std140, binding = 0) buffer Data { vec4 v; } data;\n"
            "void main() { o = data.v; }\n";
        GLuint pr = MakeProg(vs, fs);
        if (!pr) return 1;
        float red[4] = {1, 0, 0, 1};
        GLuint sb = MakeSSBO(red, sizeof(red));
        glBindBufferBase(0x90D2, 0, sb);
        glUseProgram(pr);
        glClearColor(0, 0, 0, 0);
        glClear(0x4000);
        glDrawArrays(0x0004, 0, 6);
        Check(glGetError() == 0, "draw SSBO FS no error");
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, px);
        Check(Near(px[0], 255) && Near(px[1], 0) && px[3] == 255,
              "FS SSBO struct: pixel đỏ (255,0,0,255)");

        // --- Case 2: glShaderStorageBlockBinding → override sang point 3 (xanh) ---
        float green[4] = {0, 1, 0, 1};
        GLuint sbG = MakeSSBO(green, sizeof(green));
        glBindBufferBase(0x90D2, 3, sbG);
        glShaderStorageBlockBinding(pr, 0, 3);
        Check(glGetError() == 0, "glShaderStorageBlockBinding hợp lệ không lỗi");
        glClear(0x4000);
        glDrawArrays(0x0004, 0, 6);
        Check(glGetError() == 0, "draw sau override no error");
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, px);
        Check(Near(px[0], 0) && Near(px[1], 255),
              "override point 3: pixel xanh (0,255,0,255) — đỏ@point0 bị bỏ qua");

        // Query BUFFER_BINDING phải phản ánh override (đúng spec).
        {
            GLenum prop = 0x9302;
            GLint rb = -1;
            glGetProgramResourceiv(pr, 0x92E6, 0, 1, &prop, 1, nullptr, &rb);
            Check(rb == 3, "query BUFFER_BINDING phản ánh override (3)");
        }

        // index ngoài số block → INVALID_VALUE (0x0501)
        glShaderStorageBlockBinding(pr, 7, 0);
        Check(glGetError() == 0x0501, "storageBlockIndex vượt số block → INVALID_VALUE");
    }

    // --- Case 3: VS SSBO → varying → FS (nhánh bind vertex-stage) ---
    {
        const char* vs =
            "#version 460 core\n"
            "layout(location = 0) in vec2 a_pos;\n"
            "layout(std140, binding = 0) buffer C { vec4 c; } cb;\n"
            "out vec4 vCol;\n"
            "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); vCol = cb.c; }\n";
        const char* fs =
            "#version 460 core\n"
            "in vec4 vCol;\n"
            "out vec4 o;\n"
            "void main() { o = vCol; }\n";
        GLuint pr = MakeProg(vs, fs);
        if (!pr) return 1;
        float red[4] = {1, 0, 0, 1};
        GLuint sb = MakeSSBO(red, sizeof(red));
        glBindBufferBase(0x90D2, 0, sb);
        glUseProgram(pr);
        glClear(0x4000);
        glDrawArrays(0x0004, 0, 6);
        Check(glGetError() == 0, "draw VS SSBO no error");
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, px);
        Check(Near(px[0], 255) && Near(px[1], 0), "VS SSBO qua varying: pixel đỏ");
    }

    // --- Case 4: UBO + SSBO chung program → slot xen kẽ 17/18 đúng ---
    {
        const char* vs =
            "#version 460 core\n"
            "layout(location = 0) in vec2 a_pos;\n"
            "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
        const char* fs =
            "#version 460 core\n"
            "out vec4 o;\n"
            "layout(std140, binding = 0) uniform U { vec4 k; } u;\n"
            "layout(std140, binding = 1) buffer D { vec4 v; } d;\n"
            "void main() { o = u.k + d.v; }\n";
        GLuint pr = MakeProg(vs, fs);
        if (!pr) return 1;
        float ubuf[4] = {0.2f, 0, 0, 1};   // UBO (0x8A11) k = đỏ nhạt
        float sbuf[4] = {0, 0.4f, 0, 0};   // SSBO d.v = xanh lá nhạt
        GLuint bU = 0;
        glGenBuffers(1, &bU);
        glBindBuffer(0x8A11, bU);
        glBufferData(0x8A11, sizeof(ubuf), ubuf, 0x88E4);
        glBindBufferBase(0x8A11, 0, bU);
        GLuint bS = MakeSSBO(sbuf, sizeof(sbuf));
        glBindBufferBase(0x90D2, 1, bS);
        glUseProgram(pr);
        glClear(0x4000);
        glDrawArrays(0x0004, 0, 6);
        Check(glGetError() == 0, "draw UBO+SSBO no error");
        glReadPixels(32, 32, 1, 1, 0x1908, 0x1401, px);
        // 0.2*255≈51, 0.4*255≈102
        Check(Near(px[0], 51) && Near(px[1], 102) && Near(px[2], 0) && px[3] == 255,
              "UBO(17)+SSBO(18) xen kẽ: pixel (51,102,0,255)");

        // Query: limits + indexed binding + program interface SHADER_STORAGE_BLOCK.
        GLint lim = 0;
        glGetIntegerv(0x90DB, &lim);
        Check(lim == 16, "MAX_COMPUTE_SHADER_STORAGE_BLOCKS=16");
        glGetIntegerv(0x90DA, &lim);
        Check(lim == 8, "MAX_FRAGMENT_SHADER_STORAGE_BLOCKS=8");
        glGetIntegeri_v(0x90D3, 1, &lim);
        Check(lim == (GLint)bS, "SHADER_STORAGE_BUFFER_BINDING[1]==bS");
        glGetIntegeri_v(0x90D5, 1, &lim);
        Check(lim == (GLint)sizeof(sbuf), "SHADER_STORAGE_BUFFER_SIZE[1]");
        glGetIntegeri_v(0x8A28, 0, &lim);
        Check(lim == (GLint)bU, "UNIFORM_BUFFER_BINDING[0]==bU");
        glGetProgramInterfaceiv(pr, 0x92E6, 0x92F5, &lim);
        Check(lim == 1, "SHADER_STORAGE_BLOCK ACTIVE_RESOURCES=1");
        GLuint ri = glGetProgramResourceIndex(pr, 0x92E6, "D");
        Check(ri == 0, "ResourceIndex(\"D\")==0");
        {
            GLenum props[4] = {0x92F9, 0x9302, 0x9303, 0x930A};
            GLint vals[4] = {-1, -1, -1, -1};
            GLsizei got = 0;
            glGetProgramResourceiv(pr, 0x92E6, 0, 4, props, 4, &got, vals);
            Check(vals[0] == 2, "NAME_LENGTH(\"D\")==2");
            Check(vals[1] == 1, "BUFFER_BINDING==1");
            Check(vals[2] == 16, "BUFFER_DATA_SIZE==16");
            Check(vals[3] == 1, "REFERENCED_BY_FRAGMENT_SHADER==1");
            char nm[8] = {0};
            glGetProgramResourceName(pr, 0x92E6, 0, 8, &got, nm);
            Check(std::string(nm) == "D", "ResourceName==\"D\"");
        }
        Check(glGetError() == 0, "query SSBO + program interface không lỗi");
    }

    // --- Case 5: compute SSBO override (nhánh dispatch dùng cùng EffectiveSsboBinding) ---
    {
        const char* cs =
            "#version 460 core\n"
            "layout(local_size_x=8) in;\n"
            "layout(std430, binding=0) buffer B { float v[]; };\n"
            "void main() { v[gl_GlobalInvocationID.x] = 77.0; }\n";
        GLuint sh = glCreateShader(0x91B9);
        glShaderSource(sh, 1, &cs, nullptr);
        glCompileShader(sh);
        GLuint pr = glCreateProgram();
        glAttachShader(pr, sh);
        glLinkProgram(pr);
        GLint ok = 0;
        glGetProgramiv(pr, 0x8B82, &ok);
        if (!ok) { printf("test_ssbo_graphics_apple FAIL: compute link\n"); return 1; }
        std::vector<float> z0(8, 0.0f), z4(8, 0.0f);
        GLuint b0 = MakeSSBO(z0.data(), z0.size() * 4); // point 0: không được ghi
        GLuint b4 = MakeSSBO(z4.data(), z4.size() * 4); // point 4: đích override
        glBindBufferBase(0x90D2, 0, b0);
        glBindBufferBase(0x90D2, 4, b4);
        glShaderStorageBlockBinding(pr, 0, 4);
        Check(glGetError() == 0, "compute override không lỗi");
        glUseProgram(pr);
        glDispatchCompute(1, 1, 1);
        glMemoryBarrier(0x0020 | 0x0040);
        glBindBuffer(0x90D2, b4);
        const float* r = (const float*)glMapBufferRange(0x90D2, 0, z4.size() * 4, 0x0008 | 0x0010);
        int bad4 = 0;
        if (!r) { printf("test_ssbo_graphics_apple FAIL: compute map\n"); return 1; }
        for (float f : std::vector<float>(r, r + 8)) if (f != 77.0f) ++bad4;
        glUnmapBuffer(0x90D2);
        glBindBuffer(0x90D2, b0);
        const float* r0 = (const float*)glMapBufferRange(0x90D2, 0, z0.size() * 4, 0x0008 | 0x0010);
        int bad0 = 0;
        if (r0) {
            for (int i = 0; i < 8; ++i) if (r0[i] != 0.0f) ++bad0;
            glUnmapBuffer(0x90D2);
        } else {
            printf("test_ssbo_graphics_apple FAIL: point0 map\n");
            return 1;
        }
        Check(bad4 == 0, "compute override: buffer@point4 được ghi 77.0");
        Check(bad0 == 0, "compute override: buffer@point0 KHÔNG bị ghi");
    }

    if (gFails) {
        printf("test_ssbo_graphics_apple RESULT: FAIL (%d)\n", gFails);
        return 1;
    }
    printf("test_ssbo_graphics_apple RESULT: PASS\n");
    return 0;
}
