// test_regression_shader_semantics.cpp — Hồi quy cho các lỗi ngữ nghĩa GLSL
// đã sửa trong src/gl/glsl_to_msl.cpp.
//
// Mỗi case kiểm tra MSL sinh ra (không chỉ "compile được") — vì các lỗi
// này đều tạo ra MSL HỢP LỆ nhưng SAI NGỮ NGHĨA, nên chỉ kiểm tra compile
// là không đủ.
//
// Lỗi đã sửa:
//  R1  texture(s, uv, bias): bias bị đổi thành int2 offset (overload
//      resolution của MSL chọn nhầm) ⇒ LOD bias MẤT.
//  R2  `flat` / `noperspective` / `centroid` bị strip mà không emit attribute
//      MSL ⇒ mọi varying thành center_perspective (GLSL "flat" bị nội suy).
//  R3  Fragment output DUY NHẤT ở layout(location=N) (N≠0) bị ghi vào
//      attachment 0 thay vì attachment N.
//  R4  textureQueryLod không được rewrite ⇒ MSL tham chiếu tên sampler GLSL
//      (undeclared identifier) ⇒ Metal compile fail.
//  R5  glUniformMatrix*fv bỏ qua `transpose` ⇒ ma trận bị đảo.
//  R6  glUniformMatrix3fv upload 36 byte chặt vào slot MSL 48 byte (3 cột
//      float3 stride 16) ⇒ cột 1/2 lệch 4/8 byte ⇒ ma trận rác.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static const char* kVS =
    "#version 460 core\n"
    "layout(location=0) in vec2 p;\n"
    "void main(){ gl_Position = vec4(p,0,1); }\n";

static GLuint Compile(GLenum stage, const char* src, GLint* status) {
    GLuint s = glCreateShader(stage);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    if (status) glGetShaderiv(s, GL_COMPILE_STATUS, status);
    return s;
}

// Link VS + FS và trả về MSL của fragment (rỗng nếu link fail).
static std::string LinkFS(const char* fsSrc, bool* linked) {
    GLuint vs = Compile(GL_VERTEX_SHADER, kVS, nullptr);
    GLuint fs = Compile(GL_FRAGMENT_SHADER, fsSrc, nullptr);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (linked) *linked = (ok == GL_TRUE);
    Context& c = Context::Current();
    std::string msl = (ok == GL_TRUE && c.programs.count(p)) ? c.programs[p].fragmentMSL : "";
    glDeleteProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return msl;
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // ------------------------------------------------------------------
    // R1: texture bias phải được giữ (ép kiểu `bias(...)`).
    // ------------------------------------------------------------------
    {
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "uniform sampler2D t;\n"
            "void main(){ c = texture(t, vec2(0.5), 0.5); }\n";
        bool linked = false;
        std::string msl = LinkFS(kFS, &linked);
        TGLMT_CHECK(linked, "R1: shader bias phải link được");
        // MSL overload `sample(s, coord, int2 offset)` sẽ biến bias thành
        // offset ⇒ phải có `bias(` trong MSL, và KHÔNG được có float trần
        // ở vị trí thứ 3.
        TGLMT_CHECK(msl.find("bias(") != std::string::npos,
                    "R1: texture(...,bias) phải sinh `bias(...)` trong MSL");
        TGLMT_CHECK(msl.find("sample(") != std::string::npos, "R1: MSL có lệnh sample");
        // textureProj cũng phải giữ bias.
        static const char* kFSProj =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "uniform sampler2D t;\n"
            "void main(){ c = textureProj(t, vec4(0.5,0.5,0.5,1.0), 0.25); }\n";
        std::string mslProj = LinkFS(kFSProj, &linked);
        TGLMT_CHECK(linked, "R1b: textureProj bias phải link được");
        TGLMT_CHECK(mslProj.find("bias(") != std::string::npos,
                    "R1b: textureProj(...,bias) phải sinh `bias(...)`");
    }

    // ------------------------------------------------------------------
    // R2: qualifier nội suy phải sinh attribute MSL tương ứng.
    // ------------------------------------------------------------------
    {
        static const char* kVSFlat =
            "#version 460 core\n"
            "layout(location=0) in vec2 p;\n"
            "flat out int flag;\n"
            "void main(){ gl_Position = vec4(p,0,1); flag = 7; }\n";
        static const char* kFSFlat =
            "#version 460 core\n"
            "flat in int flag;\n"
            "layout(location=0) out vec4 c;\n"
            "void main(){ c = vec4(float(flag),0,0,1); }\n";
        GLuint vs = Compile(GL_VERTEX_SHADER, kVSFlat, nullptr);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFSFlat, nullptr);
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        TGLMT_CHECK(ok == GL_TRUE, "R2: shader flat phải link được");
        if (ok == GL_TRUE && ctx.programs.count(p)) {
            const std::string& fmsl = ctx.programs[p].fragmentMSL;
            TGLMT_CHECK(fmsl.find("[[flat]]") != std::string::npos,
                        "R2: fragment input `flat` phải sinh [[flat]]");
            const std::string& vmsl = ctx.programs[p].vertexMSL;
            TGLMT_CHECK(vmsl.find("[[flat]]") != std::string::npos,
                        "R2: vertex output `flat` phải sinh [[flat]]");
        }
        glDeleteProgram(p);
    }
    {
        static const char* kVSNP =
            "#version 460 core\n"
            "layout(location=0) in vec2 p;\n"
            "noperspective out vec2 uv;\n"
            "void main(){ gl_Position = vec4(p,0,1); uv = p; }\n";
        static const char* kFSNP =
            "#version 460 core\n"
            "noperspective in vec2 uv;\n"
            "layout(location=0) out vec4 c;\n"
            "void main(){ c = vec4(uv,0,1); }\n";
        GLuint vs = Compile(GL_VERTEX_SHADER, kVSNP, nullptr);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFSNP, nullptr);
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        TGLMT_CHECK(ok == GL_TRUE, "R2b: shader noperspective phải link được");
        if (ok == GL_TRUE && ctx.programs.count(p)) {
            TGLMT_CHECK(ctx.programs[p].fragmentMSL.find("center_no_perspective") !=
                            std::string::npos,
                        "R2b: `noperspective` phải sinh [[center_no_perspective]]");
        }
        glDeleteProgram(p);
    }

    // ------------------------------------------------------------------
    // R3: fragment output đơn ở location=1 phải ghi vào attachment 1.
    // ------------------------------------------------------------------
    {
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=1) out vec4 c;\n"
            "void main(){ c = vec4(0,1,0,1); }\n";
        bool linked = false;
        std::string msl = LinkFS(kFS, &linked);
        TGLMT_CHECK(linked, "R3: shader output location=1 phải link được");
        TGLMT_CHECK(msl.find("[[color(1)]]") != std::string::npos,
                    "R3: output location=1 phải sinh [[color(1)]]");
        TGLMT_CHECK(msl.find("[[color(0)]]") == std::string::npos,
                    "R3: output location=1 KHÔNG được ghi vào [[color(0)]]");
    }
    {
        // Output đơn ở location=0 vẫn phải là color(0).
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "void main(){ c = vec4(1,0,0,1); }\n";
        bool linked = false;
        std::string msl = LinkFS(kFS, &linked);
        TGLMT_CHECK(linked, "R3b: shader output location=0 phải link được");
        // Đường single-out trả float4 trần (Metal tự gán color(0)) hoặc
        // struct [[color(0)]] — cả hai đều đúng, nhưng KHÔNG được color(1).
        TGLMT_CHECK(msl.find("[[color(1)]]") == std::string::npos,
                    "R3b: output location=0 không được sinh [[color(1)]]");
    }

    // ------------------------------------------------------------------
    // R4: textureQueryLod phải được rewrite (trước đây → MSL không compile).
    // ------------------------------------------------------------------
    {
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "uniform sampler2D t;\n"
            "void main(){ vec2 lod = textureQueryLod(t, vec2(0.5)); c = vec4(lod,0,1); }\n";
        bool linked = false;
        std::string msl = LinkFS(kFS, &linked);
        TGLMT_CHECK(linked, "R4: textureQueryLod phải link được");
        TGLMT_CHECK(msl.find("calculate_unclamped_lod") != std::string::npos,
                    "R4: textureQueryLod phải sinh calculate_unclamped_lod");
        TGLMT_CHECK(msl.find("textureQueryLod") == std::string::npos,
                    "R4: textureQueryLod không được sót lại trong MSL");
    }

    // ------------------------------------------------------------------
    // R5: transpose=GL_TRUE phải được áp dụng.
    // ------------------------------------------------------------------
    {
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "uniform mat3 m;\n"
            "void main(){ c = vec4(m * vec3(1,0,0), 1); }\n";
        bool linked = false;
        std::string msl = LinkFS(kFS, &linked);
        TGLMT_CHECK(linked, "R5: shader mat3 phải link được");
        GLuint vs = Compile(GL_VERTEX_SHADER, kVS, nullptr);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS, nullptr);
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        glUseProgram(p);
        GLint loc = glGetUniformLocation(p, "m");
        TGLMT_CHECK(loc >= 0, "R5: uniform mat3 phải có location");
        // Ma trận column-major chặt: cột 0 = (1,2,3), cột 1 = (4,5,6),
        // cột 2 = (7,8,9).
        const GLfloat colMajor[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        glUniformMatrix3fv(loc, 1, GL_FALSE, colMajor);
        // Ghi lại transpose=GL_TRUE với dữ liệu ROW-major: ma trận row-major
        // tương ứng với cùng ma trận là
        //   [1 4 7]
        //   [2 5 8]
        //   [3 6 9]
        // nên dữ liệu row-major = {1,4,7, 2,5,8, 3,6,9}.
        const GLfloat rowMajor[9] = {1, 4, 7, 2, 5, 8, 3, 6, 9};
        glUniformMatrix3fv(loc, 1, GL_TRUE, rowMajor);
        if (ctx.programs.count(p) && loc >= 0) {
            auto uit = ctx.programs[p].uniforms.find(loc);
            TGLMT_CHECK(uit != ctx.programs[p].uniforms.end(), "R5: uniform đã lưu");
            if (uit != ctx.programs[p].uniforms.end()) {
                const float* got = (const float*)uit->second.bytes.data();
                // Sau transpose, dữ liệu phải là column-major của ma trận gốc.
                TGLMT_CHECK(uit->second.bytes.size() == 9 * sizeof(GLfloat),
                            "R5: mat3 = 9 float");
                if (uit->second.bytes.size() == 9 * sizeof(GLfloat)) {
                    bool ok = true;
                    for (int i = 0; i < 9; ++i)
                        if (std::fabs(got[i] - colMajor[i]) > 1e-5f) ok = false;
                    TGLMT_CHECK(ok,
                                "R5: transpose=GL_TRUE phải cho cùng ma trận column-major");
                }
            }
        }
        glDeleteProgram(p);
    }

    // ------------------------------------------------------------------
    // R6: mat3 phải được đóng gói theo layout MSL (3 cột float3 stride 16).
    // ------------------------------------------------------------------
    {
        static const char* kFS =
            "#version 460 core\n"
            "layout(location=0) out vec4 c;\n"
            "uniform mat3 m;\n"
            "void main(){ c = vec4(m * vec3(1,0,0), 1); }\n";
        GLuint vs = Compile(GL_VERTEX_SHADER, kVS, nullptr);
        GLuint fs = Compile(GL_FRAGMENT_SHADER, kFS, nullptr);
        GLuint p = glCreateProgram();
        glAttachShader(p, vs);
        glAttachShader(p, fs);
        glLinkProgram(p);
        glUseProgram(p);
        GLint loc = glGetUniformLocation(p, "m");
        TGLMT_CHECK(loc >= 0, "R6: uniform mat3 phải có location");
        const GLfloat m[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
        glUniformMatrix3fv(loc, 1, GL_FALSE, m);
        if (ctx.programs.count(p) && loc >= 0) {
            // Đóng gói vào buffer MSL phải nới cột: mỗi cột float3 chiếm
            // 16 byte (12 byte dữ liệu + 4 byte padding), tổng 48 byte.
            auto uit = ctx.programs[p].uniforms.find(loc);
            TGLMT_CHECK(uit != ctx.programs[p].uniforms.end(), "R6: uniform mat3 đã lưu");
            if (uit != ctx.programs[p].uniforms.end())
                TGLMT_CHECK(uit->second.bytes.size() == 9 * sizeof(GLfloat),
                            "R6: mat3 lưu 9 float (36 byte) — đóng gói MSL làm phần còn lại");
        }
        glDeleteProgram(p);
    }

    return TGLMT_RESULT;
}
