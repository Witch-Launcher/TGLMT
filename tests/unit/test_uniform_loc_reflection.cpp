// test_uniform_loc_reflection.cpp — glGetUniformLocation trung thực theo reflection.
// Bối cảnh in-game (Iris): Iris dùng location != -1 để đếm texture unit
// (ProgramSamplers$Builder.addDynamicSampler). TGLMT cũ trả loc cho MỌI tên
// → composite + DH hết unit khi tới dhDepthTex → "No more available texture
// units" → pipeline fail. Từ đây: program ĐÃ link + reflection đầy đủ → tên
// không có trong program (chưa khai báo, hoặc bị converter prune vì không
// dùng) phải trả -1; TRƯỚC link giữ nới lỏng có chủ đích (app cache loc sớm).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("FAIL: %s\n", msg); ++gFails; }
}

int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    using namespace tglmt;
    using namespace tglmt::gl;
    GLint ok = 0;

    // ---- Program A: đã link — honesty theo reflection ----
    const char* vs = "#version 460 core\nvoid main(){ gl_Position = vec4(0.0); }";
    const char* fs =
        "#version 460 core\n"
        "out vec4 c;\n"
        "uniform sampler2D texUsed;\n"
        "uniform sampler2D texGhost;\n" // khai báo nhưng KHÔNG dùng → prune
        "uniform vec4 uTint;\n"
        "void main(){ c = texture(texUsed, vec2(0.5)) * uTint; }\n";
    GLuint v = glCreateShader(0x8B31), f = glCreateShader(0x8B30);
    glShaderSource(v, 1, &vs, nullptr);
    glShaderSource(f, 1, &fs, nullptr);
    glCompileShader(v); glCompileShader(f);
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f);
    glLinkProgram(p);
    glGetProgramiv(p, 0x8B82, &ok); // LINK_STATUS
    Check(ok == 1, "program A link");
    GLint locUsed = glGetUniformLocation(p, "texUsed");
    GLint locTint = glGetUniformLocation(p, "uTint");
    Check(locUsed >= 0, "sampler dùng → có location");
    Check(locTint >= 0, "uniform dùng → có location");
    Check(glGetUniformLocation(p, "texGhost") == -1, "sampler khai báo không dùng (prune) → -1");
    Check(glGetUniformLocation(p, "neverDeclared") == -1, "tên chưa khai báo → -1");
    Check(glGetUniformLocation(p, "texUsed[0]") == locUsed, "query 'texUsed[0]' = loc base (strip mảng)");
    // Loc phải vào uniformLayout (AppleDraw upload theo e.loc >= 0; thiếu →
    // uniform không bao giờ tới shader: FS đọc 0).
    {
        GLint layLoc = -2;
        for (auto& e : ctx.programs[p].uniformLayout)
            if (e.name == "uTint") layLoc = e.loc;
        Check(layLoc == locTint, "uTint loc vào uniformLayout (upload được)");
    }
    Check(glGetUniformLocation(p, "texUsed") == locUsed, "query lặp ổn định (cache)");
    Check(glGetUniformLocation(p, "") == -1, "tên rỗng (đã link) → -1");

    // ---- Program B: trước link nới lỏng, sau link trung thực ----
    const char* vs2 = "#version 460 core\nvoid main(){ gl_Position = vec4(0.0); }";
    const char* fs2 = "#version 460 core\nout vec4 c;\nuniform vec4 uB;\nvoid main(){ c = uB; }\n";
    GLuint v2 = glCreateShader(0x8B31), f2 = glCreateShader(0x8B30);
    glShaderSource(v2, 1, &vs2, nullptr);
    glShaderSource(f2, 1, &fs2, nullptr);
    glCompileShader(v2); glCompileShader(f2);
    GLuint p2 = glCreateProgram();
    glAttachShader(p2, v2); glAttachShader(p2, f2);
    GLint preLoc = glGetUniformLocation(p2, "uB");
    GLint preGhostLoc = glGetUniformLocation(p2, "uPreGhost");
    Check(preLoc >= 0 && preGhostLoc >= 0, "trước link: giữ nới lỏng (cache loc sớm)");
    glLinkProgram(p2);
    glGetProgramiv(p2, 0x8B82, &ok);
    Check(ok == 1, "program B link");
    Check(glGetUniformLocation(p2, "uB") == preLoc, "loc cache trước link vẫn đúng sau link");
    {
        // Pre-link loc 0 được cache; query lại sau link phải điền e.loc, nếu
        // không uniform bị bỏ qua im lặng (bug upload âm thầm).
        GLint layLoc = -2;
        for (auto& e : ctx.programs[p2].uniformLayout)
            if (e.name == "uB") layLoc = e.loc;
        Check(layLoc == preLoc, "loc cache sớm vẫn vào uniformLayout sau link");
    }
    Check(glGetUniformLocation(p2, "uPreGhost") == -1, "sau link: tên lạ → -1 (không dùng cache nới lỏng)");

    // ---- Compute: honesty áp dụng cả đường compute ----
    const char* cs =
        "#version 460 core\n"
        "layout(local_size_x=8) in;\n"
        "layout(std430,binding=0) buffer B{float v[];};\n"
        "uniform sampler2D tUsed;\n"
        "uniform sampler2D tGhost;\n"
        "void main(){ uint g=gl_GlobalInvocationID.x; v[g]=texture(tUsed,vec2(0.5)).r; }\n";
    GLuint csh = glCreateShader(0x91B9);
    glShaderSource(csh, 1, &cs, nullptr);
    glCompileShader(csh);
    GLuint pc = glCreateProgram();
    glAttachShader(pc, csh);
    glLinkProgram(pc);
    glGetProgramiv(pc, 0x8B82, &ok);
    Check(ok == 1, "compute link");
    Check(glGetUniformLocation(pc, "tUsed") >= 0, "compute sampler dùng → loc");
    Check(glGetUniformLocation(pc, "tGhost") == -1, "compute sampler prune → -1");
    Check(glGetUniformLocation(pc, "nonexistent") == -1, "compute tên lạ → -1");

    if (gFails) { printf("test_uniform_loc_reflection FAIL: %d\n", gFails); return 1; }
    printf("test_uniform_loc_reflection PASS\n");
    return 0;
}
