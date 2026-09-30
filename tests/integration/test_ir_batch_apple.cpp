// test_ir_batch_apple.cpp — IR lowering proof: N OpenGL draws cùng state
// phải lower xuống 1 MTLRenderCommandEncoder + 1 setPipeline + N draws.
// Bằng chứng cho cập nhật lớn "1 GL → 0 Metal khi không đổi".
// - Pixel đúng (đỏ trước/xanh sau: sai batching là mất hình/đen).
// - encodersCreated == 1 cho 3 draws cùng pass (trước fix = 3).
// - Redundant GL binds không tăng StateSeq (shadowing).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* kVS =
    "#version 460 core\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec4 col;\n"
    "layout(location = 0) out vec4 vCol;\n"
    "void main() {\n"
    "  vCol = col;\n"
    "  gl_Position = vec4(pos, 0.0, 1.0);\n"
    "}\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location = 0) in vec4 vCol;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vCol; }\n";

int main() {
    tglmt::Context ctx("apple");
    tglmt::Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) { printf("test_ir_batch SKIP: no MTL device\n"); return 0; }
    using namespace tglmt::gl;

    auto target = ctx.device->makeRenderTarget(64, 64, tglmt::metal::PixelFormat::RGBA8Unorm);
    if (!target) { printf("FAIL target\n"); return 1; }
    ctx.device->setDefaultRenderTarget(target);

    tglmt::GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    float verts[3 * 6] = {-0.5f, -0.5f, 1, 0, 0, 1, 0.5f, -0.5f, 1, 0, 0, 1,
                          0.0f, 0.5f, 1, 0, 0, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(0x8892, vbo);
    glBufferData(0x8892, sizeof(verts), verts, 0x88E4);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, 0x1406, 0, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, 0x1406, 0, 24, (void*)8);

    tglmt::GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    tglmt::GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    tglmt::GLint ok = 0;
    glGetProgramiv(p, 0x8B82, &ok);
    if (!ok) { printf("FAIL link\n"); return 1; }
    glUseProgram(p);

    // --- Shadowing proof: redundant binds không đổi StateSeq ---
    uint64_t s0 = ctx.state.StateSeq();
    glBindVertexArray(vao); // trùng
    glUseProgram(p);        // trùng
    glBindBuffer(0x8892, vbo); // trùng target+id
    glEnable(0x0B44); glEnable(0x0B44); // CULL_FACE 2 lần
    glDisable(0x0B44); glDisable(0x0B44);
    uint64_t s1 = ctx.state.StateSeq();
    // s1-s0 chỉ đếm lần ĐỔI thật (enable+disable = 2), không đếm trùng.
    // Trùng: BindVAO+UseProgram+BindBuffer (3) + Enable lần 2 (1) + Disable lần 2 (1) = 5 lần bỏ qua.
    // Đổi thật: Enable lần 1 + Disable lần 1 = 2.
    if (s1 - s0 != 2) {
        printf("FAIL shadowing: seq delta=%llu want 2 (s0=%llu s1=%llu)\n",
               (unsigned long long)(s1 - s0), (unsigned long long)s0, (unsigned long long)s1);
        return 1;
    }
    printf("shadowing OK: 5 redundant GL calls -> 0 seq (delta=2 for 2 real changes)\n");
    glDisable(0x0B44); // về trạng thái vẽ (đã disable, trùng → no-op)

    glViewport(0, 0, 64, 64);
    glClearColor(0, 0, 1, 1);
    glClear(0x00004000);

    uint64_t enc0 = ctx.appleStats.encodersCreated;
    uint64_t reuse0 = ctx.appleStats.encoderReused;

    // 3 draws CÙNG state liên tiếp → phải batch vào 1 encoder.
    glDrawArrays(0x0004, 0, 3);
    glDrawArrays(0x0004, 0, 3);
    glDrawArrays(0x0004, 0, 3);
    if (glGetError() != 0) { printf("FAIL gl error after batch draws\n"); return 1; }

    uint64_t encNew = ctx.appleStats.encodersCreated - enc0;
    uint64_t reuseNew = ctx.appleStats.encoderReused - reuse0;
    printf("batch: encodersCreated=%llu (want 1), encoderReused=%llu (want 2), "
           "pipelineReused=%llu stateSkipped=%llu uniformReused=%llu\n",
           (unsigned long long)encNew, (unsigned long long)reuseNew,
           (unsigned long long)ctx.appleStats.pipelineReused,
           (unsigned long long)ctx.appleStats.stateSkipped,
           (unsigned long long)ctx.appleStats.uniformReused);
    if (encNew != 1) {
        printf("FAIL batching: 3 draws cùng pass phải dùng 1 encoder, got %llu\n",
               (unsigned long long)encNew);
        return 1;
    }
    if (reuseNew != 2) {
        printf("FAIL batching: want 2 reuses, got %llu\n", (unsigned long long)reuseNew);
        return 1;
    }
    // Pending encoder còn mở (chưa flush) — ReadPixels phải flush và vẫn đúng pixel.
    unsigned char px[64 * 64 * 4];
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    unsigned char* mid = px + (32 * 64 + 32) * 4;
    unsigned char* corner = px + (2 * 64 + 2) * 4;
    printf("mid=(%u,%u,%u) corner=(%u,%u,%u)\n", mid[0], mid[1], mid[2], corner[0], corner[1],
           corner[2]);
    if (!(mid[0] > 200 && mid[1] < 4 && mid[2] < 4)) { printf("FAIL mid (want red)\n"); return 1; }
    if (!(corner[2] > 200 && corner[0] < 4)) { printf("FAIL corner (want blue clear)\n"); return 1; }

    // Sau flush (ReadPixels đã flush), draw tiếp phải tạo encoder mới (pass mới LOAD).
    uint64_t enc1 = ctx.appleStats.encodersCreated;
    glDrawArrays(0x0004, 0, 3);
    // flush显式 để readback thấy (glReadPixels tự flush, nhưng đếm encoder ngay)
    ctx.FlushPendingEncoder();
    uint64_t encNew2 = ctx.appleStats.encodersCreated - enc1;
    if (encNew2 != 1) {
        printf("FAIL post-flush: want 1 new encoder, got %llu\n", (unsigned long long)encNew2);
        return 1;
    }
    memset(px, 0, sizeof(px));
    glReadPixels(0, 0, 64, 64, 0x1908, 0x1401, px);
    mid = px + (32 * 64 + 32) * 4;
    if (!(mid[0] > 200)) { printf("FAIL mid2\n"); return 1; }

    printf("test_ir_batch PASS (3 draws -> 1 encoder, pixels đúng, shadowing đúng)\n");
    return 0;
}
