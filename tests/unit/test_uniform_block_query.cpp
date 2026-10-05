// test_uniform_block_query.cpp — Unit: glGetProgramiv(ACTIVE_UNIFORM_BLOCKS) +
// glGetActiveUniformBlockName/iv (Fix #1 cho GlProgram.setupUniforms path#2).
//
// Căn cứ source thật 26.1.2 (GlProgram.java.setupUniforms):
//   int totalDefinedBlocks = glGetProgrami(programId, 35382);        // ACTIVE_UNIFORM_BLOCKS
//   for (i = 0; i < totalDefinedBlocks; i++) {
//      String name = glGetActiveUniformBlockName(programId, i);
//      if (!uniformsByName.containsKey(name)
//          && !samplers.contains(name) && BUILT_IN_UNIFORMS.contains(name))  // {Projection,Lighting,Fog,Globals}
//          glUniformBlockBinding(programId, i, nextUboBinding++);
//   }
// Stub cũ trả count=0 + tên rỗng → loop không chạy → block builtin ngoài
// pipeline desc (Globals của terrain/blur) KHÔNG BAO GIỜ được bind → đọc nhầm
// buffer point0 (SamplerInfo 16B) → CameraBlockPos rác (world sai vị trí),
// MenuBlurRadius rác (blur radius 0 → "blur như không có").
//
// Index space: uniformBlocks[].index dense 0..N-1 (gộp vs trước, fs sau) —
// cùng không gian với glGetUniformBlockIndex (GL ràng buộc linker gán dense).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_uniform_block_query FAIL: %s\n", msg);
        ++gFails;
    }
}

// VS: Globals (builtin, gộp vs+fs); FS: Fog (builtin) + SamplerInfo (pipeline).
static const char* kVS =
    "#version 460 core\n"
    "layout(std140) uniform Globals { vec4 cam; };\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) out vec4 vColor;\n"
    "void main() { vColor = cam; gl_Position = vec4(pos, 0.0, 1.0); }\n";
static const char* kFS =
    "#version 460 core\n"
    "layout(std140) uniform Fog { vec4 fogc; };\n"
    "layout(std140) uniform SamplerInfo { vec4 info; };\n"
    "layout(location = 0) in vec4 vColor;\n"
    "layout(location = 0) out vec4 fragColor;\n"
    "void main() { fragColor = vColor * 0.0 + fogc * 0.0 + info * 0.0; }\n";

static std::string ActiveBlockName(GLuint prog, GLuint idx) {
    char buf[128] = {0};
    GLsizei len = 0;
    glGetActiveUniformBlockName(prog, idx, sizeof(buf), &len, buf);
    return std::string(buf, (size_t)(len > 0 ? len : 0));
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    GLuint vs = glCreateShader(0x8B31), fs = glCreateShader(0x8B30);
    glShaderSource(vs, 1, &kVS, nullptr);
    glShaderSource(fs, 1, &kFS, nullptr);
    glCompileShader(vs);
    glCompileShader(fs);
    GLint ok = 0;
    glGetShaderiv(vs, 0x8B81, &ok);
    Check(ok != 0, "vs compile");
    glGetShaderiv(fs, 0x8B81, &ok);
    Check(ok != 0, "fs compile");
    if (gFails) return 1;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "pos");
    glLinkProgram(prog);
    glGetProgramiv(prog, 0x8B82, &ok);
    Check(ok != 0, "link");
    if (gFails) return 1;

    // --- 1. ACTIVE_UNIFORM_BLOCKS (0x8A36=35382): điều kiện vào loop path#2 ---
    GLint n = -1;
    glGetProgramiv(prog, 0x8A36, &n);
    Check(n == 3, "ACTIVE_UNIFORM_BLOCKS == 3 (Globals + Fog + SamplerInfo)");
    if (n != 3) return 1;

    // --- 2. glGetActiveUniformBlockName(i) cho i ∈ [0,count): không rỗng, đúng tên ---
    std::vector<std::string> names;
    for (GLint i = 0; i < n; ++i) {
        std::string nm = ActiveBlockName(prog, (GLuint)i);
        if (nm.empty()) printf("[q] block[%d] = EMPTY\n", i);
        names.push_back(nm);
    }
    // Thứ tự: vs blocks trước (Globals), fs blocks sau (Fog, SamplerInfo).
    Check(names[0] == "Globals", "block[0] == Globals (vs trước)");
    Check(names[1] == "Fog", "block[1] == Fog (fs)");
    Check(names[2] == "SamplerInfo", "block[2] == SamplerInfo (fs)");

    // --- 3. Index space thống nhất: glGetUniformBlockIndex(name) == i ---
    for (GLint i = 0; i < n; ++i) {
        GLuint idx = glGetUniformBlockIndex(prog, names[(size_t)i].c_str());
        Check(idx == (GLuint)i, "GetUniformBlockIndex(name) == dense index i");
    }
    Check(glGetUniformBlockIndex(prog, "KhongTonTai") == 0xFFFFFFFFu,
          "GL_INVALID_INDEX cho block không tồn tại");

    // --- 4. Index không tồn tại → tên rỗng (không ghi nhớ vùng lân) ---
    {
        char buf[16] = {'x', 'x', 'x', 'x'};
        GLsizei len = -1;
        glGetActiveUniformBlockName(prog, 99, sizeof(buf), &len, buf);
        Check(len == 0 && buf[0] == 0, "index 99 → name rỗng");
    }

    // --- 5. Mô phỏng path#2 setupUniforms: tìm builtin chưa bind rồi bind ---
    // BUILT_IN_UNIFORMS = {Projection, Lighting, Fog, Globals}; SamplerInfo là
    // pipeline uniform (path#1 đã bind). Ở đây ta chỉ check Globals+Fog.
    const char* builtins[] = {"Projection", "Lighting", "Fog", "Globals"};
    int nextUbo = 0;
    for (GLint i = 0; i < n; ++i) {
        const std::string& nm = names[(size_t)i];
        bool isBuiltin = false;
        for (const char* b : builtins)
            if (nm == b) isBuiltin = true;
        if (!isBuiltin) continue;
        glUniformBlockBinding(prog, (GLuint)i, (GLuint)nextUbo);
        GLint point = -1;
        glGetActiveUniformBlockiv(prog, (GLuint)i, 0x8A3F /*UNIFORM_BLOCK_BINDING*/, &point);
        Check(point == nextUbo, "bind builtin → GetActiveUniformBlockiv(BINDING) đúng");
        ++nextUbo;
    }
    Check(nextUbo == 2, "path#2 bind đúng 2 builtin (Globals, Fog)");

    // --- 6. UNIFORM_BLOCK_DATA_SIZE > 0 (minSize thật từ converter) ---
    {
        GLint sz = 0;
        glGetActiveUniformBlockiv(prog, 0, 0x8A40 /*DATA_SIZE*/, &sz);
        Check(sz == (GLint)sizeof(float) * 4, "Globals DATA_SIZE == 16 (vec4)");
    }

    // --- 7. LWJGL 2-arg chain: glGetActiveUniformBlockName(p,i) gọi
    // glGetActiveUniformBlocki(p,i,0x8A41 NAME_LENGTH) rồi malloc(bufSize).
    // Stub cũ trả NAME_LENGTH=0 → bufSize=0 → tên rỗng → path#2 skip builtin.
    // NAME_LENGTH theo GL spec gồm NUL. ---
    for (GLint i = 0; i < n; ++i) {
        GLint nameLen = 0;
        glGetActiveUniformBlockiv(prog, (GLuint)i, 0x8A41 /*NAME_LENGTH*/, &nameLen);
        Check(nameLen == (GLint)names[(size_t)i].size() + 1,
              "NAME_LENGTH == strlen+1 (gồm NUL)");
        // mô phỏng GL31C.glGetActiveUniformBlockName(p,i,bufSize=NAME_LENGTH)
        std::vector<char> buf((size_t)(nameLen > 0 ? nameLen : 1), 'x');
        GLsizei len = -1;
        glGetActiveUniformBlockName(prog, (GLuint)i, (GLsizei)buf.size(), &len, buf.data());
        Check(len == (GLint)names[(size_t)i].size() &&
              std::string(buf.data(), (size_t)len) == names[(size_t)i],
              "LWJGL chain trả đúng tên (không rỗng)");
    }

    // --- 8. REFERENCED_BY_* flag theo stage thật: Globals chỉ có ở vs,
    // Fog + SamplerInfo chỉ có ở fs; geometry luôn 0. ---
    {
        GLint v = -1;
        glGetActiveUniformBlockiv(prog, 0, 0x8A44 /*REF_VERTEX*/, &v);
        Check(v == 1, "Globals referenced by VERTEX");
        glGetActiveUniformBlockiv(prog, 0, 0x8A46 /*REF_FRAGMENT*/, &v);
        Check(v == 0, "Globals không referenced by FRAGMENT (vs-only)");
        glGetActiveUniformBlockiv(prog, 1, 0x8A44, &v);
        Check(v == 0, "Fog không referenced by VERTEX");
        glGetActiveUniformBlockiv(prog, 1, 0x8A46, &v);
        Check(v == 1, "Fog referenced by FRAGMENT (0x8A46, không phải 0x8A45)");
        glGetActiveUniformBlockiv(prog, 2, 0x8A46, &v);
        Check(v == 1, "SamplerInfo referenced by FRAGMENT");
        glGetActiveUniformBlockiv(prog, 0, 0x8A45 /*REF_GEOMETRY*/, &v);
        Check(v == 0, "geometry reference = 0 (TGLMT không có geometry stage)");
    }

    if (gFails) return 1;
    printf("test_uniform_block_query PASS\n");
    return 0;
}
