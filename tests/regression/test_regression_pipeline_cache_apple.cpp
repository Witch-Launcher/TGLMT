// test_regression_pipeline_cache_apple.cpp — Hồi quy cho lỗi cache pipeline
// dùng CON TRỎ làm định danh chương trình.
//
// Lỗi gốc
// --------
// Cả cache pipeline ở Context (Context::PipelineKey) và cache PSO ở bridge
// Metal (AppleMetalBridge.mm `pcache_`) dùng ĐỊA CHỈ con trỏ của ILibrary
// làm khoá. `glLinkProgram` giải phóng ILibrary cũ rồi tạo cái mới; allocator
// rất hay TRẢ LẠI ĐÚNG ĐỊA CHỈ CŨ ⇒ khoá cache trùng ⇒ tái dùng pipeline
// đã compile từ shader CŨ. Hậu quả: relink program (rất phổ biến khi app
// nạp lại shader/resource) thì shader MỚI không bao giờ có hiệu lực, KHÔNG
// có lỗi nào được báo — ảnh render vẫn là ảnh cũ.
//
// Cách sửa: ILibrary::identity() trả về serial duy nhất theo vòng đời
// (AppleLibrary::serial_ từ bộ đếm atomic), dùng serial đó làm khoá cache.
//
// Test: link program ĐỎ → render → đổi fragment shader sang XAN DƯƠNG →
// link lại → render → pixel phải đổi màu.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <set>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";
static const char* kFSRed =
    "#version 460 core\n"
    "layout(location=0) out vec4 color;\n"
    "void main(){ color = vec4(1,0,0,1); }\n";
static const char* kFSBlue =
    "#version 460 core\n"
    "layout(location=0) out vec4 color;\n"
    "void main(){ color = vec4(0,0,1,1); }\n";

static GLuint MakeProgram(const char* fsSrc) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &kVS, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[1024] = {0};
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        fprintf(stderr, "link fail: %s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

int main() {
    Context ctx("apple");
    Context::MakeCurrent(&ctx);
    if (ctx.device->isNull()) {
        printf("test_regression_pipeline_cache_apple SKIP: no MTL device\n");
        return 0;
    }
    while (glGetError() != GL_NO_ERROR) {}

    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    TGLMT_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "FBO phải complete");

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLfloat verts[12] = {-1, -1, 3, -1, -1, 3}; // tam giác phủ toàn viewport
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    auto renderAndRead = [&](GLuint prog, unsigned char* px) -> bool {
        glUseProgram(prog);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, 32, 32);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFinish();
        glReadPixels(0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, px);
        return glGetError() == GL_NO_ERROR;
    };

    unsigned char px[32 * 32 * 4] = {0};

    // 1) Program ĐỎ.
    GLuint p1 = MakeProgram(kFSRed);
    TGLMT_CHECK(p1 != 0, "program đỏ phải tạo được");
    TGLMT_CHECK(renderAndRead(p1, px), "render program đỏ không lỗi");
    TGLMT_CHECK(px[0] > 200 && px[1] < 60 && px[2] < 60,
                "program đỏ phải cho pixel đỏ");

    // 2) RELINK CÙNG program với shader XAN DƯƠNG — đây là ca gây lỗi.
    //    glLinkProgram giải phóng ILibrary cũ và tạo ILibrary mới.
    GLuint fs2 = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs2, 1, &kFSBlue, nullptr);
    glCompileShader(fs2);
    // Gỡ shader fragment CŨ (giữ nguyên shader vertex).
    for (GLuint sh : ctx.programs[p1].shaders) {
        auto sit = ctx.shaders.find(sh);
        if (sit != ctx.shaders.end() && sit->second.type == GL_FRAGMENT_SHADER)
            glDetachShader(p1, sh);
    }
    glAttachShader(p1, fs2);
    glLinkProgram(p1);
    GLint ok = 0;
    glGetProgramiv(p1, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        char log[1024] = {0};
        glGetProgramInfoLog(p1, sizeof(log), nullptr, log);
        fprintf(stderr, "relink log: %s\n", log);
    }
    TGLMT_CHECK(ok == GL_TRUE, "relink phải thành công");

    renderAndRead(p1, px);
    // Đây là kiểm tra cốt lõi: pipeline cũ (compile từ shader ĐỎ) KHÔNG
    // được được tái dùng.
    TGLMT_CHECK(px[2] > 200 && px[0] < 60 && px[1] < 60,
                "sau relink phải render XAN DƯƠNG (cache pipeline không được trả PSO cũ)");

    // 2b) BẤT BIẾN của identity(): nhiều library phải có identity KHÁC NHAU.
    //     Đây là hợp đồng mà cache pipeline dựa vào — nếu identity trùng
    //     (ví dụ vì dùng con trỏ làm identity và allocator trả lại cùng địa
    //     chỉ) thì cache có thể trả nhầm PSO của shader cũ.
    {
        std::string err;
        std::vector<uint64_t> ids;
        std::vector<std::shared_ptr<metal::ILibrary>> keep;
        for (int i = 0; i < 32; ++i) {
            auto lib = ctx.device->compileLibrary("kernel void k() {}", err);
            TGLMT_CHECK(lib != nullptr, "compileLibrary phải trả library");
            if (!lib) break;
            keep.push_back(lib);
            ids.push_back(lib->identity());
        }
        std::set<uint64_t> uniq(ids.begin(), ids.end());
        TGLMT_CHECK(uniq.size() == ids.size(),
                    "identity của các library khác nhau phải KHÁC nhau");
        // Sau khi giải phóng, identity mới không được tái sử dụng giá trị cũ.
        uint64_t first = ids.empty() ? 0 : ids.front();
        keep.clear();
        auto again = ctx.device->compileLibrary("kernel void k() {}", err);
        TGLMT_CHECK(again && again->identity() != first,
                    "identity sau khi giải phóng không được tái sử dụng");
    }

    // 2c) Khoá cache pipeline phải chứa SỐ LẦN LINK của program. Đây là
    //     bảo đảm mạnh hơn: kể cả khi identity có lỗi, khoá vẫn khác.
    {
        auto pit = ctx.programs.find(p1);
        TGLMT_CHECK(pit != ctx.programs.end(), "program p1 phải tồn tại");
        if (pit != ctx.programs.end()) {
            TGLMT_CHECK(pit->second.linkGen > 0,
                        "linkGen phải tăng mỗi lần link");
            bool found = false;
            for (size_t k = 0; k < Context::kPipeCacheN; ++k) {
                const auto& e = ctx.pipeCache[k];
                if (e.valid && e.key.programGen == pit->second.linkGen) { found = true; break; }
            }
            TGLMT_CHECK(found,
                        "khoá cache pipeline phải chứa linkGen hiện tại của program");
        }
    }

    // 3) Program MỚI hoàn toàn (không phải relink) cũng phải cho màu riêng.
    GLuint p2 = MakeProgram(kFSRed);
    renderAndRead(p2, px);
    TGLMT_CHECK(px[0] > 200 && px[2] < 60, "program mới phải cho pixel đỏ");

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    glDeleteProgram(p1);
    glDeleteProgram(p2);
    return TGLMT_RESULT;
}
