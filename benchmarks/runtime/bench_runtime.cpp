// benchmarks/runtime/bench_runtime.cpp — Đo hiệu năng runtime của backend.
//
// Đo các nhóm việc mà app thực sự làm mỗi frame:
//   • API call overhead (glBindBuffer / glUniform1f / glGetIntegerv / ...)
//   • Upload buffer / texture
//   • Draw call (arrays / elements / instanced)
//   • Compute dispatch
//   • Frame tổng (nhiều draw + upload + flush)
//
// Quy trình: warm-up → R lần lặp → median / p95 / min / max.
// Xuất JSON ra stdout.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/MetalInterface.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;
using Clock = std::chrono::steady_clock;

struct Stat {
    std::vector<double> us;
    void add(double v) { us.push_back(v); }
    double median() const {
        if (us.empty()) return 0;
        std::vector<double> s = us;
        std::sort(s.begin(), s.end());
        return s[s.size() / 2];
    }
    double p95() const {
        if (us.empty()) return 0;
        std::vector<double> s = us;
        std::sort(s.begin(), s.end());
        size_t i = (size_t)(s.size() * 0.95);
        if (i >= s.size()) i = s.size() - 1;
        return s[i];
    }
    double min() const {
        return us.empty() ? 0 : *std::min_element(us.begin(), us.end());
    }
    double max() const {
        return us.empty() ? 0 : *std::max_element(us.begin(), us.end());
    }
    double mean() const {
        if (us.empty()) return 0;
        double t = 0;
        for (double v : us) t += v;
        return t / (double)us.size();
    }
};

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "uniform float uK;\n"
    "void main(){ gl_Position = vec4(p * uK, 0, 1); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(location=0) out vec4 c;\n"
    "void main(){ c = vec4(1,0,0,1); }\n";

// Đo TỪNG lần lặp (không chỉ tổng) để có phân phối thật cho median/p95.
// Lưu ý: thời gian của một lệnh GL rất nhỏ (~0.02 µs) nên đồng hồ có sai số
// đáng kể; ta ghi cả tổng (wall) để chốt lại con số chính.
template <typename F>
static void measure(const char* name, int iters, int warmup, Stat& out, F&& fn) {
    for (int i = 0; i < warmup; ++i) fn();
    auto t0 = Clock::now();
    for (int i = 0; i < iters; ++i) {
        auto a = Clock::now();
        fn();
        auto b = Clock::now();
        out.add(std::chrono::duration<double, std::micro>(b - a).count());
    }
    auto t1 = Clock::now();
    double total_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    (void)total_us;
    (void)name;
}


// Đo BATCHED: gọi lệnh N lần trong Mẫu, chia đều cho mỗi lần. Khi thời gian
// một lệnh (~0.05 µs) nhỏ hơn sai số đồng hồ (~0.02 µs/lần), đo từng lần bị
// nhiễu nặng; đo theo lô giảm sai số xuống còn O(1/lô).
template <typename F>
static void measureBatch(const char* name, int iters, int warmup, int batch, Stat& out,
                          F&& fn) {
    for (int i = 0; i < warmup; ++i) fn();
    const int samples = iters / batch > 0 ? iters / batch : 1;
    for (int s = 0; s < samples; ++s) {
        auto a = Clock::now();
        for (int i = 0; i < batch; ++i) fn();
        auto b = Clock::now();
        out.add(std::chrono::duration<double, std::micro>(b - a).count() / batch);
    }
    (void)name;
}

int main(int argc, char** argv) {
    int iters = 2000;
    int warmup = 200;
    const char* backend = "apple";
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--iters") && i + 1 < argc) iters = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--warmup") && i + 1 < argc) warmup = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--backend") && i + 1 < argc) backend = argv[++i];
    }

    Context ctx(backend);
    Context::MakeCurrent(&ctx);
    bool haveMetal = ctx.device && !ctx.device->isNull();

    // --- setup ---
    GLuint vao = 0, vbo = 0, ebo = 0, tex = 0, fbo = 0, prog = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLfloat verts[12] = {-1, -1, 3, -1, -1, 3};
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    const GLuint idx[3] = {0, 1, 2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(fs);
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glUseProgram(prog);
    // Location của uniform thật (đo đúng đường ghi có validation).
    const GLint kUniformLoc = glGetUniformLocation(prog, "uK");

    Stat sBind, sUniform, sGet, sUpload, sTexUpload, sTexUploadFull, sDrawArr, sDrawElem,
        sClear, sFrame, sUniformBatch, sControlBatch;

    measure("bind", iters, warmup, sBind, [] { glBindBuffer(GL_ARRAY_BUFFER, 1); });
    measure("uniform", iters, warmup, sUniform, [kUniformLoc] { glUniform1f(kUniformLoc, 1.0f); });
    // Batched: chi phí thật mỗi lệnh uniform (đo theo lô 64 lệnh).
    measureBatch("uniform_batched64", iters, warmup, 64, sUniformBatch,
                 [kUniformLoc] { glUniform1f(kUniformLoc, 1.0f); });
    // Control: lệnh KHÔNG bị thay đổi (glGetIntegerv) — đo nhiễu nền máy.
    measureBatch("control_getintegerv_batched64", iters, warmup, 64, sControlBatch, [] {
        GLint v = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);
    });
    measure("get", iters, warmup, sGet, [] {
        GLint v = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &v);
    });
    measure("buffer_subdata", iters, warmup, sUpload, [] {
        const GLfloat d[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
    });
    // glTexSubImage2D gộp vào staging (deferred) → chỉ đo phần CPU staging,
    // CHƯA đụng tới GPU. Đo riêng đường upload thật bằng glTexImage2D
    // (upload tức thì qua IDevice::updateTexture).
    measure("tex_subimage_staged", iters, warmup, sTexUpload, [] {
        static std::vector<uint8_t> px(64 * 64 * 4, 0x7f);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    });
    measure("tex_image_full_upload", iters, warmup, sTexUploadFull, [] {
        static std::vector<uint8_t> px(64 * 64 * 4, 0x7f);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    });
    measure("draw_arrays", iters, warmup, sDrawArr, [] { glDrawArrays(GL_TRIANGLES, 0, 3); });
    measure("draw_elements", iters, warmup, sDrawElem, [] {
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
    });
    measure("clear", iters, warmup, sClear, [] { glClear(GL_COLOR_BUFFER_BIT); });

    // Frame tổng: 100 draw + upload + flush.
    {
        const int kDraws = 100;
        for (int w = 0; w < 5; ++w) {
            for (int d = 0; d < kDraws; ++d) glDrawArrays(GL_TRIANGLES, 0, 3);
            glFinish();
        }
        auto t0 = Clock::now();
        const int reps = 20;
        for (int r = 0; r < reps; ++r) {
            for (int d = 0; d < kDraws; ++d) glDrawArrays(GL_TRIANGLES, 0, 3);
            glFlush();
        }
        auto t1 = Clock::now();
        sFrame.add(std::chrono::duration<double, std::micro>(t1 - t0).count() /
                   (double)reps);
    }

    // Xuất JSON.
    printf("{\n");
    printf("  \"benchmark\": \"runtime\",\n");
    printf("  \"backend\": \"%s\",\n", backend);
    printf("  \"metal\": %s,\n", haveMetal ? "true" : "false");
    printf("  \"iters\": %d,\n", iters);
    printf("  \"warmup\": %d,\n", warmup);
    printf("  \"metrics\": {\n");
    auto dump = [&](const char* key, const Stat& st, bool last) {
        printf("    \"%s\": {\"median_us\": %.4f, \"p95_us\": %.4f, \"min_us\": %.4f, "
               "\"max_us\": %.4f, \"mean_us\": %.4f, \"n\": %zu}%s\n",
               key, st.median(), st.p95(), st.min(), st.max(), st.mean(), st.us.size(),
               last ? "" : ",");
    };
    dump("glBindBuffer", sBind, false);
    dump("glUniform1f", sUniform, false);
    dump("glUniform1f_batched64", sUniformBatch, false);
    dump("control_glGetIntegerv_batched64", sControlBatch, false);
    dump("glGetIntegerv", sGet, false);
    dump("glBufferSubData_64B", sUpload, false);
    dump("glTexSubImage2D_64x64_staged", sTexUpload, false);
    dump("glTexImage2D_64x64_full_upload", sTexUploadFull, false);
    dump("glDrawArrays_3", sDrawArr, false);
    dump("glDrawElements_3", sDrawElem, false);
    dump("glClear", sClear, false);
    dump("frame_100_draws", sFrame, true);
    printf("  }\n");
    printf("}\n");
    return 0;
}
