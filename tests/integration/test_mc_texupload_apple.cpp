// test_mc_texupload_apple.cpp — Hồi quy texture hỏng trên menu 26.1.2.
// Game 26.x alloc texture qua TexImage2D NULL từng mip level (GlDevice) rồi
// upload qua TexSubImage2D (GlCommandEncoder.writeToTexture, kèm ROW_LENGTH).
// Các bug từng làm logo/text/panorama mất (đã đối chiếu bytecode client.jar):
//  A. TexImage bỏ qua `level` → w/h bị ghi đè bằng mip nhỏ nhất → SubImage
//     level 0 fail bounds → texture rỗng.
//  B. Upload RED/UBYTE (font RED8, glyph LUMINANCE) sai bpp=4 → GPU không sync.
//  C. PixelStore sai (0x0CF2 ROW_LENGTH nhầm thành alignment) + bỏ qua
//     ROW_LENGTH → sub-upload sai hàng.
//  D. Cubemap panorama (samplerCube) chỉ có placeholder 2D + skip bind.
//  E. Sampler thiếu texture để trống argument → GPU fault A11 (status=5).
// Thiếu GPU → SKIP. Sai pixel → FAIL thật.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

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
static const char* kFSRed =
    "#version 460 core\n"
    "layout(location = 0) in vec2 vUV;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform sampler2D Sampler0;\n"
    "void main() { fragColor = vec4(texture(Sampler0, vUV).rrr, 1.0); }\n";
static const char* kVSCube =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "void main() { gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFSCube =
    "#version 460 core\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "layout(location = 0) uniform samplerCube Sampler0;\n"
    "void main() { fragColor = texture(Sampler0, vec3(1.0, 0.0, 0.0)); }\n";

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_mc_texupload_apple FAIL: %s\n", msg);
        ++gFails;
    }
}

static GLuint MakeProg(const char* vs, const char* fs) {
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
    glBindAttribLocation(p, 0, "pos");
    glBindAttribLocation(p, 1, "uv");
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok);
    Check(ok != 0, "link");
    return p;
}

// Quad fullscreen pos2+uv2 (uv 0..1 cả texture).
static void DrawQuad() {
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float q[4 * 4] = {-1, -1, 0, 0, /**/ 1, -1, 1, 0, /**/ 1, 1, 1, 1, /**/ -1, 1, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(q), q, 0x88E4);
    uint32_t idx[6] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &ebo);
    glBindBuffer(0x8893, ebo);
    glBufferData(0x8893, sizeof(idx), idx, 0x88E4);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, 0x1406, 0, 16, (void*)0);
    glVertexAttribPointer(1, 2, 0x1406, 0, 16, (void*)8);
    glDrawElements(0x0004, 6, 0x1405, (void*)0);
}

static void ReadAt(int x, int y, int W, unsigned char out[4]) {
    std::vector<unsigned char> px((size_t)W * W * 4, 0);
    glReadPixels(0, 0, W, W, 0x1908, 0x1401, px.data());
    memcpy(out, px.data() + ((size_t)y * W + x) * 4, 4);
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_mc_texupload_apple SKIP: no MTL device\n");
        return 0;
    }
    auto target = ctx.device->makeRenderTarget(64, 64, metal::PixelFormat::RGBA8Unorm);
    if (!target) {
        printf("test_mc_texupload_apple FAIL: target nil\n");
        return 1;
    }
    ctx.device->setDefaultRenderTarget(target);

    GLuint progTex = MakeProg(kVS, kFS);
    GLuint progR = MakeProg(kVS, kFSRed);
    GLuint progCube = MakeProg(kVSCube, kFSCube);
    if (gFails) return 1;

    // ---- A. mip alloc NULL rồi SubImage level 0 (đúng GlDevice.createTexture)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, tex);
        glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
        glTexImage2D(0x0DE1, 1, 0x8058, 32, 32, 0, 0x1908, 0x1401, nullptr);
        glTexImage2D(0x0DE1, 2, 0x8058, 16, 16, 0, 0x1908, 0x1401, nullptr);
        std::vector<unsigned char> red(64 * 64 * 4);
        for (size_t i = 0; i < 64 * 64; ++i) {
            red[i * 4] = 255;
            red[i * 4 + 1] = 0;
            red[i * 4 + 2] = 0;
            red[i * 4 + 3] = 255;
        }
        glTexSubImage2D(0x0DE1, 0, 0, 0, 64, 64, 0x1908, 0x1401, red.data());
        Check(glGetError() == 0, "A subimage gl error (bounds sau mip alloc)");
        glTexParameteri(0x0DE1, 0x2801, 0x2600);
        glTexParameteri(0x0DE1, 0x2800, 0x2600);
        glUseProgram(progTex);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        DrawQuad();
        unsigned char px[4];
        ReadAt(32, 32, 64, px);
        printf("[A mip] center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "A do sau mip-alloc + subimage");
    }
    if (gFails) return 1;

    // ---- B. RED8 font: glyph trắng trên nền đen, sample .r
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, tex);
        glTexImage2D(0x0DE1, 0, 0x8229, 32, 32, 0, 0x1903, 0x1401, nullptr);
        std::vector<unsigned char> glyph(32 * 32, 0);
        for (int y = 8; y < 24; ++y)
            for (int x = 8; x < 24; ++x) glyph[(size_t)y * 32 + x] = 255;
        glTexSubImage2D(0x0DE1, 0, 0, 0, 32, 32, 0x1903, 0x1401, glyph.data());
        Check(glGetError() == 0, "B subimage gl error");
        glTexParameteri(0x0DE1, 0x2801, 0x2600);
        glTexParameteri(0x0DE1, 0x2800, 0x2600);
        glUseProgram(progR);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 0, 1);
        glClear(0x00004000);
        DrawQuad();
        unsigned char mid[4], corner[4];
        ReadAt(32, 32, 64, mid);
        ReadAt(2, 2, 64, corner);
        printf("[B red8] mid=(%u,%u,%u) corner=(%u,%u,%u)\n", mid[0], mid[1], mid[2],
               corner[0], corner[1], corner[2]);
        Check(mid[0] > 200, "B glyph trang giua");
        Check(corner[0] < 50, "B nen den goc");
    }
    if (gFails) return 1;

    // ---- C. ROW_LENGTH pitch (glyph sub-upload từ image lớn)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, tex);
        glTexImage2D(0x0DE1, 0, 0x8058, 16, 16, 0, 0x1908, 0x1401, nullptr);
        // Source 8 hàng x ROW 16: cột 0-7 đỏ, 8-15 xanh lá.
        std::vector<unsigned char> src(8 * 16 * 4);
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 16; ++x) {
                unsigned char* p = src.data() + ((size_t)y * 16 + x) * 4;
                p[0] = (x < 8) ? 255 : 0;
                p[1] = (x < 8) ? 0 : 255;
                p[2] = 0;
                p[3] = 255;
            }
        glPixelStorei(0x0CF2, 16); // UNPACK_ROW_LENGTH
        glPixelStorei(0x0CF5, 4);  // UNPACK_ALIGNMENT
        glTexSubImage2D(0x0DE1, 0, 0, 0, 8, 8, 0x1908, 0x1401, src.data());
        glPixelStorei(0x0CF2, 0); // trả về 0 (tight)
        Check(glGetError() == 0, "C subimage gl error");
        glTexParameteri(0x0DE1, 0x2801, 0x2600);
        glTexParameteri(0x0DE1, 0x2800, 0x2600);
        glUseProgram(progTex);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        DrawQuad();
        // Vùng upload (0,0)-(8,8)/16 → góc GL dưới-trái màn hình.
        unsigned char px[4];
        ReadAt(8, 8, 64, px); // texel ~(2,2) trong vùng đỏ
        printf("[C rowlen] px=(%u,%u,%u)\n", px[0], px[1], px[2]);
        Check(px[0] > 200 && px[1] < 50, "C do dung pitch ROW_LENGTH");
    }
    if (gFails) return 1;

    // ---- D. cubemap panorama: face +X đỏ, sample +X
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x8513, tex); // TEXTURE_CUBE_MAP
        unsigned char face[4][4] = {{255, 0, 0, 255},   // +X đỏ
                                    {0, 255, 0, 255},   // -X xanh lá
                                    {0, 0, 255, 255},   // +Y xanh dương
                                    {255, 255, 255, 255}}; // -Y trắng
        for (int f = 0; f < 6; ++f) {
            unsigned char* px = face[f < 4 ? f : 0];
            std::vector<unsigned char> img(8 * 8 * 4);
            for (size_t i = 0; i < 8 * 8; ++i) memcpy(img.data() + i * 4, px, 4);
            glTexImage2D(0x8515 + f, 0, 0x8058, 8, 8, 0, 0x1908, 0x1401, img.data());
        }
        Check(glGetError() == 0, "D cube upload gl error");
        glTexParameteri(0x8513, 0x2801, 0x2600);
        glTexParameteri(0x8513, 0x2800, 0x2600);
        // Vẽ tam giác thường với program cube (không attribute UV).
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        float tri[6] = {-1, -1, 3, -1, -1, 3};
        glGenBuffers(1, &vbo);
        glBindBuffer(0x8892, vbo);
        glBufferData(0x8892, sizeof(tri), tri, 0x88E4);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, 0x1406, 0, 8, (void*)0);
        glUseProgram(progCube);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        glDrawArrays(0x0004, 0, 3);
        Check(glGetError() == 0, "D draw gl error");
        unsigned char px[4];
        ReadAt(32, 32, 64, px);
        printf("[D cube] center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] > 200 && px[1] < 50 && px[2] < 50, "D panorama +X do");
    }
    if (gFails) return 1;

    // ---- E. sampler thiếu texture → đen (fallback), vẫn encode, không fault
    {
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, 0); // unbind
        glUseProgram(progTex);
        glBindFramebuffer(0x8CA9, 0);
        glViewport(0, 0, 64, 64);
        glClearColor(0, 0, 1, 1);
        glClear(0x00004000);
        uint64_t encBefore = ctx.appleStats.drawsEncoded;
        DrawQuad();
        Check(glGetError() == 0, "E draw gl error");
        Check(ctx.appleStats.drawsEncoded > encBefore, "E draw encoded (fallback)");
        unsigned char px[4];
        ReadAt(32, 32, 64, px);
        printf("[E fallback] center=(%u,%u,%u,%u)\n", px[0], px[1], px[2], px[3]);
        Check(px[0] < 50 && px[1] < 50 && px[2] < 50, "E thieu texture den");
    }
    if (gFails) return 1;
    printf("test_mc_texupload_apple PASS (mip/RED8/rowlen/cube/fallback)\n");
    return 0;
}
