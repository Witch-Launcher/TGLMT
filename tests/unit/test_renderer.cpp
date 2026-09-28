// test_renderer.cpp — Renderer façade trên cả 2 backend.
// Null: Init/Begin/clear/readback/stats qua shadow (luôn chạy, cho CI).
// Apple (có GPU mới chạy phần render): tam giác đỏ thuần GL qua Renderer,
// readback kiểm tra. Thiếu GPU → SKIP phần Apple, không ép pass.
#include "tglmt/Renderer.h"
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "void main() { vCol = vec4(1.0, 0.0, 0.0, 1.0); gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

int main() {
    using namespace tglmt;
    // ---- backend lạ / tham số sai ----
    {
        Renderer r;
        assert(!r.Init("vulkan", 64, 64)); // backend lạ → false trung thực
        assert(!r.Init("null", 0, 64));    // kích thước 0 → false
        assert(!r.BeginFrame());           // chưa Init → false
        assert(!r.EndFrame(nullptr));
    }
    // ---- Null backend: clear + readback qua shadow ----
    {
        Renderer r;
        assert(r.Init("null", 64, 64));
        assert(r.width() == 64 && r.height() == 64);
        assert(r.backendName() == "null");
        assert(r.BeginFrame());
        gl::glClearColor(1, 0, 0, 1);
        gl::glClear(0x00004000);
        assert(r.EndFrame(nullptr)); // headless → true
        unsigned char px[16] = {0};
        assert(r.ReadPixels(0, 0, 2, 2, px));
        assert(px[0] == 255 && px[1] == 0 && px[2] == 0 && px[3] == 255);
        assert(!r.ReadPixels(0, 0, 0, 2, px)); // w=0 → false
        r.Resize(32, 32);
        assert(r.width() == 32 && r.BeginFrame());
        RendererStats s = r.stats();
        assert(s.drawsAttempted == s.drawsEncoded); // null: trace-only, không lệch
    }
    // ---- Apple backend: render tam giác đỏ thật qua façade ----
    {
        Renderer r;
        assert(r.Init("apple", 64, 64));
        if (!r.hasRealGPU()) {
            printf("test_renderer SKIP phần Apple (không có GPU / build null)\n");
        } else {
            assert(r.BeginFrame());
            GLuint vao, vbo;
            gl::glGenVertexArrays(1, &vao);
            gl::glBindVertexArray(vao);
            float verts[6] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.0f, 0.5f};
            gl::glGenBuffers(1, &vbo);
            gl::glBindBuffer(0x8892, vbo);
            gl::glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
            gl::glEnableVertexAttribArray(0);
            gl::glVertexAttribPointer(0, 2, 0x1406, 0, 0, (void*)0);
            GLuint vs = gl::glCreateShader(0x8B31), fs = gl::glCreateShader(0x8B30);
            gl::glShaderSource(vs, 1, &kVS, nullptr);
            gl::glShaderSource(fs, 1, &kFS, nullptr);
            gl::glCompileShader(vs);
            gl::glCompileShader(fs);
            GLuint p = gl::glCreateProgram();
            gl::glAttachShader(p, vs);
            gl::glAttachShader(p, fs);
            gl::glLinkProgram(p);
            GLint ok = 0;
            gl::glGetProgramiv(p, 0x8B82, &ok);
            assert(ok == 1);
            gl::glUseProgram(p);
            gl::glViewport(0, 0, 64, 64);
            gl::glClearColor(0, 0, 1, 1);
            gl::glClear(0x00004000);
            gl::glDrawArrays(0x0004, 0, 3);
            assert(gl::glGetError() == 0);
            assert(r.EndFrame(nullptr));
            unsigned char px[64 * 64 * 4];
            memset(px, 0, sizeof(px));
            assert(r.ReadPixels(0, 0, 64, 64, px));
            unsigned char* mid = px + (32 * 64 + 32) * 4;
            unsigned char* corner = px + (2 * 64 + 2) * 4;
            printf("apple mid=(%u,%u,%u) corner=(%u,%u,%u)\n", mid[0], mid[1], mid[2],
                   corner[0], corner[1], corner[2]);
            assert(mid[0] > 200 && mid[1] < 50 && mid[2] < 50);
            assert(corner[2] > 200 && corner[0] < 50);
            RendererStats s = r.stats();
            assert(s.drawsEncoded == s.drawsAttempted && s.drawsEncoded >= 1);
        }
    }
    printf("test_renderer PASS\n");
    return 0;
}
