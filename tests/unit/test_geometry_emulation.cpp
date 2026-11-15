// test_geometry_emulation.cpp — Emulation GEOMETRY SHADER (GS) của TGLMT.
//
// Phạm vi test:
//  1. TRANSLATOR: GLSL → MSL compute kernel + phân tích tĩnh emission.
//  2. PLAN: số primitive input / vertex mở rộng / primitive output.
//  3. LINK: program VS+GS+FS link OK; thiếu VS hoặc FS ⇒ link FAIL có log;
//     GS ngoài subset (sampler / invocations / gl_ClipDistance / max_vertices)
//     ⇒ link FAIL có log rõ (không "link OK rồi vẽ câm" — nếp P6f).
//  4. COUNTER: GL_GEOMETRY_SHADER_INVOCATIONS / GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED
//     (OpenGL 4.2C §2.16 / ARB_pipeline_statistics_query).
//  5. MODE MISMATCH: mode không tương thích input primitive của GS ⇒
//     GL_INVALID_OPERATION (OpenGL 4.2C §2.16.1).
//  6. QUERY CHƯƠNG TRÌNH: GL_ATTACHED_SHADERS / GL_SHADER_TYPE / GL_LINK_STATUS.
//  7. PIXEL (backend Metal thật): GS mở rộng 1 tam giác nhỏ thành tam giác lớn
//     ⇒ pixel trong vùng mở rộng có màu của GS, ngoài vùng đó là màu clear.
//
// Spec: OpenGL 4.2 Compatibility §2.16 (Geometry Shaders); GLSL 3.20 §4.3.8.1,
// §4.3.8.2, §8.10.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "tglmt/GeometryEmulation.h"
#include "test_util.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

const char* kVs =
    "#version 460 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "out vec3 vColor;\n"
    "void main() { vColor = aPos; gl_Position = vec4(aPos, 1.0); }\n";

const char* kFs =
    "#version 460 core\n"
    "in vec3 gColor;\n"
    "out vec4 c;\n"
    "void main() { c = vec4(gColor, 1.0); }\n";

// GS: 3 input triangle → 3 vertex (triangle_strip) → 1 primitive output.
const char* kGsSimple =
    "#version 460 core\n"
    "layout(triangles) in;\n"
    "layout(triangle_strip, max_vertices = 3) out;\n"
    "in vec3 vColor;\n"
    "out vec3 gColor;\n"
    "void main() {\n"
    "  gl_Position = gl_in[0].gl_Position;\n"
    "  gColor = vColor[0];\n"
    "  EmitVertex();\n"
    "  gl_Position = gl_in[1].gl_Position;\n"
    "  gColor = vColor[1];\n"
    "  EmitVertex();\n"
    "  gl_Position = gl_in[2].gl_Position;\n"
    "  gColor = vColor[2];\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"
    "}\n";

// GS: input points → output line_strip 2 đỉnh (1 đường), max_vertices = 2.
const char* kGsPointsToLine =
    "#version 460 core\n"
    "layout(points) in;\n"
    "layout(line_strip, max_vertices = 2) out;\n"
    "void main() {\n"
    "  gl_Position = gl_in[0].gl_Position + vec4(-0.2, 0.0, 0.0, 0.0);\n"
    "  EmitVertex();\n"
    "  gl_Position = gl_in[0].gl_Position + vec4(0.2, 0.0, 0.0, 0.0);\n"
    "  EmitVertex();\n"
    "  EndPrimitive();\n"
    "}\n";

const char* kUniformStruct = "struct TGLMTUniforms { float _pad; };\n";

std::vector<GsVar> MakeVsOutputs() {
    std::vector<GsVar> v;
    GsVar c;
    c.name = "vColor";
    c.glslType = "vec3";
    c.mslType = "float3";
    v.push_back(c);
    return v;
}

void ClearErrors() {
    while (glGetError() != 0) {}
}

GLuint MakeShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

std::string ProgramLog(GLuint p) {
    GLint len = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    if (len <= 0) return std::string();
    std::string s((size_t)len, '\0');
    GLsizei written = 0;
    glGetProgramInfoLog(p, len, &written, &s[0]);
    s.resize((size_t)(written > 0 ? written : 0));
    return s;
}

// Đọc 1 pixel từ FBO hiện tại (RGBA8).
void ReadPixel(int x, int y, unsigned char out[4]) {
    out[0] = out[1] = out[2] = out[3] = 0;
    glReadPixels(x, y, 1, 1, 0x1908 /*GL_RGBA*/, 0x1401 /*GL_UNSIGNED_BYTE*/, out);
}

} // namespace

int main() {
    tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);

    // ===================== 1. TRANSLATOR =====================================
    {
        GsTranslation t = TranslateGeometryShader(kGsSimple, kUniformStruct, MakeVsOutputs());
        TGLMT_CHECK(t.ok, "GS đơn giản phải dịch được");
        if (!t.ok) printf("  log: %s\n", t.log.c_str());
        TGLMT_CHECK(t.inPrim == GsPrimType::Triangles, "input primitive = triangles");
        TGLMT_CHECK(t.outPrim == GsPrimType::TriangleStrip, "output primitive = triangle_strip");
        TGLMT_CHECK(t.maxVertices == 3, "max_vertices = 3");
        TGLMT_CHECK(t.invocations == 1, "invocations = 1");
        TGLMT_CHECK(t.staticEmission, "emission tĩnh (Emit/End ngoài nhánh điều kiện)");
        TGLMT_CHECK(t.emitCalls == 3, "3 lệnh EmitVertex");
        TGLMT_CHECK(t.endCalls == 1, "1 lệnh EndPrimitive");
        TGLMT_CHECK(t.vertsPerInput == 3, "3 vertex / input primitive");
        TGLMT_CHECK(t.primsPerInput == 1, "1 primitive output / input primitive");
        TGLMT_CHECK(t.staticIndexPerPrim == 3, "3 index / input primitive (triangle_strip)");
        // MSL: kernel compute + mapping built-in.
        TGLMT_CHECK(t.msl.find("kernel void TGLMT_gs") != std::string::npos,
                    "MSL phải có kernel compute TGLMT_gs");
        TGLMT_CHECK(t.msl.find("tglmt_cap_gl_Position") != std::string::npos,
                    "gl_in[].gl_Position → buffer capture");
        TGLMT_CHECK(t.msl.find("tglmt_cur_pos") != std::string::npos,
                    "gl_Position → vertex output hiện tại");
        TGLMT_CHECK(t.msl.find("tglmt_cap_vColor") != std::string::npos,
                    "input GS → buffer capture của VS output");
        TGLMT_CHECK(t.msl.find("tglmt_cur_gColor") != std::string::npos,
                    "output GS → vertex output hiện tại");
        TGLMT_CHECK(t.msl.find("EmitVertex") == std::string::npos,
                    "EmitVertex phải được nội tuyến (không gọi hàm chưa định nghĩa)");
        // Binding: gl_Position, gl_PointSize, rồi input của GS.
        TGLMT_CHECK(t.bindings.size() == 3, "3 binding buffer cho kernel");
        if (t.bindings.size() == 3) {
            TGLMT_CHECK(t.bindings[0].name == "gl_Position", "binding 0 = gl_Position");
            TGLMT_CHECK(t.bindings[1].name == "gl_PointSize", "binding 1 = gl_PointSize");
            TGLMT_CHECK(t.bindings[2].name == "vColor", "binding 2 = vColor");
        }
        // Output primitive type không hợp lệ (GL chỉ cho points/line_strip/
        // triangle_strip — OpenGL 4.2C §2.16.2) ⇒ fail rõ.
        const char* kGsBadOut =
            "#version 460 core\n"
            "layout(triangles) in;\n"
            "layout(triangles, max_vertices = 3) out;\n"
            "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n";
        GsTranslation tb = TranslateGeometryShader(kGsBadOut, kUniformStruct, MakeVsOutputs());
        TGLMT_CHECK(!tb.ok, "output primitive 'triangles' phải bị từ chối");
        TGLMT_CHECK(!tb.log.empty(), "log phải nói lý do");
        // Emission trong nhánh điều kiện ⇒ không tĩnh ⇒ dùng đường atomic.
        const char* kGsDynamic =
            "#version 460 core\n"
            "layout(triangles) in;\n"
            "layout(triangle_strip, max_vertices = 6) out;\n"
            "void main() {\n"
            "  for (int i = 0; i < 3; ++i) {\n"
            "    gl_Position = gl_in[i].gl_Position;\n"
            "    EmitVertex();\n"
            "  }\n"
            "  EndPrimitive();\n"
            "}\n";
        GsTranslation td = TranslateGeometryShader(kGsDynamic, kUniformStruct, MakeVsOutputs());
        TGLMT_CHECK(td.ok, "GS có vòng lặp vẫn phải dịch được");
        TGLMT_CHECK(!td.staticEmission, "emission trong vòng lặp ⇒ không tĩnh");
        TGLMT_CHECK(td.msl.find("metal::atomic_fetch_add_explicit") != std::string::npos,
                    "emission động ⇒ counter phải dùng atomic của Metal");
    }

    // ===================== 2. PLAN ==========================================
    {
        GsTranslation t = TranslateGeometryShader(kGsSimple, kUniformStruct, MakeVsOutputs());
        TGLMT_CHECK(t.ok, "dịch lại để tính plan");
        // 9 đỉnh = 3 tam giác input.
        GsPlan p = PlanGeometryExpansion(t, GL_TRIANGLES, 9, 1);
        TGLMT_CHECK(p.modeCompatible, "TRIANGLES hợp lệ với GS input triangles");
        TGLMT_CHECK(p.inputPrimitives == 3, "3 primitive input");
        TGLMT_CHECK(p.invocations == 3, "3 lần gọi GS");
        TGLMT_CHECK(p.outputVertices == 9, "9 vertex mở rộng (3 × 3)");
        TGLMT_CHECK(p.outputPrimitives == 3, "3 primitive output (3 × 1)");
        TGLMT_CHECK(p.exact, "plan tĩnh ⇒ số liệu chính xác");
        // Instancing: 2 instance ⇒ nhân 2.
        GsPlan pi = PlanGeometryExpansion(t, GL_TRIANGLES, 9, 2);
        TGLMT_CHECK(pi.invocations == 6, "2 instance ⇒ 6 lần gọi GS");
        TGLMT_CHECK(pi.outputVertices == 18, "2 instance ⇒ 18 vertex mở rộng");
        // Mode không tương thích (OpenGL 4.2C §2.16.1).
        GsPlan bad = PlanGeometryExpansion(t, GL_LINES, 6, 1);
        TGLMT_CHECK(!bad.modeCompatible, "LINES không hợp lệ với GS input triangles");
        // TRIANGLE_STRIP cũng hợp lệ (GL chuyển strip thành tam giác trước GS).
        GsPlan strip = PlanGeometryExpansion(t, GL_TRIANGLE_STRIP, 5, 1);
        TGLMT_CHECK(strip.modeCompatible, "TRIANGLE_STRIP hợp lệ với GS input triangles");
        TGLMT_CHECK(strip.inputPrimitives == 3, "strip 5 đỉnh ⇒ 3 tam giác");
        // GS input points + draw POINTS.
        GsTranslation tp = TranslateGeometryShader(kGsPointsToLine, kUniformStruct,
                                                    MakeVsOutputs());
        TGLMT_CHECK(tp.ok, "GS points→line_strip phải dịch được");
        GsPlan pp = PlanGeometryExpansion(tp, GL_POINTS, 4, 1);
        TGLMT_CHECK(pp.modeCompatible, "POINTS hợp lệ với GS input points");
        TGLMT_CHECK(pp.inputPrimitives == 4, "4 point = 4 primitive input");
        TGLMT_CHECK(pp.outputPrimitives == 4, "4 point → 4 line output");
    }

    // ===================== 3. LINK ==========================================
    GLuint progOk = 0;
    {
        GLuint v = MakeShader(GL_VERTEX_SHADER, kVs);
        GLuint g = MakeShader(GL_GEOMETRY_SHADER, kGsSimple);
        GLuint f = MakeShader(GL_FRAGMENT_SHADER, kFs);
        GLuint p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, g);
        glAttachShader(p, f);
        glLinkProgram(p);
        GLint ok = 0;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        TGLMT_CHECK(ok == 1, "program VS+GS+FS phải link OK");
        if (ok != 1) printf("  log: %s\n", ProgramLog(p).c_str());
        TGLMT_CHECK(ProgramLog(p).empty(), "info log phải rỗng khi link OK");
        progOk = p;
        // Query chương trình: 3 shader đính kèm, GS có type GL_GEOMETRY_SHADER.
        GLint attached = 0;
        glGetProgramiv(p, GL_ATTACHED_SHADERS, &attached);
        TGLMT_CHECK(attached == 3, "ATTACHED_SHADERS = 3 (VS + GS + FS)");
        GLuint sh[3] = {0, 0, 0};
        GLsizei n = 0;
        glGetAttachedShaders(p, 3, &n, sh);
        TGLMT_CHECK(n == 3, "glGetAttachedShaders trả 3 shader");
        bool sawGs = false;
        for (GLsizei i = 0; i < n; ++i) {
            GLint ty = 0;
            glGetShaderiv(sh[i], GL_SHADER_TYPE, &ty);
            if (ty == (GLint)GL_GEOMETRY_SHADER) sawGs = true;
        }
        TGLMT_CHECK(sawGs, "trong các shader đính kèm phải có GL_GEOMETRY_SHADER");
        // Sau link thành công, COMPILE_STATUS của GS = TRUE (glCompileShader
        // không biết stage geometry — xem docs/GEOMETRY_SHADER_EMULATION.md).
        GLint cstat = 0;
        glGetShaderiv(g, GL_COMPILE_STATUS, &cstat);
        TGLMT_CHECK(cstat == 1, "COMPILE_STATUS của GS = TRUE sau link OK");

        // Thiếu VS ⇒ link FAIL (OpenGL 4.2C §2.16: program có GS phải có VS).
        {
            GLuint p2 = glCreateProgram();
            glAttachShader(p2, g);
            glAttachShader(p2, f);
            glLinkProgram(p2);
            GLint ok2 = 1;
            glGetProgramiv(p2, GL_LINK_STATUS, &ok2);
            TGLMT_CHECK(ok2 == 0, "GS mà thiếu VS ⇒ link phải FAIL");
            TGLMT_CHECK(!ProgramLog(p2).empty(), "GS thiếu VS ⇒ phải có info log");
        }
        // Thiếu FS ⇒ link FAIL (emulation cần FS để vẽ phần mở rộng).
        {
            GLuint p3 = glCreateProgram();
            glAttachShader(p3, v);
            glAttachShader(p3, g);
            glLinkProgram(p3);
            GLint ok3 = 1;
            glGetProgramiv(p3, GL_LINK_STATUS, &ok3);
            TGLMT_CHECK(ok3 == 0, "GS mà thiếu FS ⇒ link phải FAIL");
            TGLMT_CHECK(!ProgramLog(p3).empty(), "GS thiếu FS ⇒ phải có info log");
        }
        // GS ngoài subset ⇒ link FAIL rõ (không link OK rồi vẽ câm).
        struct BadCase { const char* name; const char* src; };
        const BadCase bad[] = {
            {"sampler trong GS",
             "#version 460 core\n"
             "layout(triangles) in;\n"
             "layout(triangle_strip, max_vertices = 3) out;\n"
             "uniform sampler2D t;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
            {"invocations > 1",
             "#version 460 core\n"
             "layout(triangles, invocations = 2) in;\n"
             "layout(triangle_strip, max_vertices = 3) out;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
            {"gl_ClipDistance",
             "#version 460 core\n"
             "layout(triangles) in;\n"
             "layout(triangle_strip, max_vertices = 3) out;\n"
             "void main() { gl_ClipDistance[0] = 1.0; gl_Position = gl_in[0].gl_Position;"
             " EmitVertex(); }\n"},
            {"thiếu max_vertices",
             "#version 460 core\n"
             "layout(triangles) in;\n"
             "layout(triangle_strip) out;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
            {"input không có VS output",
             "#version 460 core\n"
             "layout(triangles) in;\n"
             "layout(triangle_strip, max_vertices = 3) out;\n"
             "in vec3 vMissing;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
            {"max_vertices vượt ngưỡng",
             "#version 460 core\n"
             "layout(triangles) in;\n"
             "layout(triangle_strip, max_vertices = 64) out;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
            {"adjacency chưa hỗ trợ",
             "#version 460 core\n"
             "layout(triangles_adjacency) in;\n"
             "layout(triangle_strip, max_vertices = 3) out;\n"
             "void main() { gl_Position = gl_in[0].gl_Position; EmitVertex(); }\n"},
        };
        for (const auto& bc : bad) {
            GLuint gb = MakeShader(GL_GEOMETRY_SHADER, bc.src);
            GLuint pb = glCreateProgram();
            glAttachShader(pb, v);
            glAttachShader(pb, gb);
            glAttachShader(pb, f);
            glLinkProgram(pb);
            GLint okb = 1;
            glGetProgramiv(pb, GL_LINK_STATUS, &okb);
            TGLMT_CHECK(okb == 0, "GS ngoài subset phải link FAIL");
            TGLMT_CHECK(!ProgramLog(pb).empty(), "GS ngoài subset phải có info log");
        }
    }

    // ===================== 4. COUNTER (query) ================================
    {
        glUseProgram(progOk);
        GLuint q = 0;
        glGenQueries(1, &q);
        ClearErrors();
        glBeginQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED, q);
        glDrawArrays(GL_TRIANGLES, 0, 9);
        glEndQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED);
        GLuint64 emitted = 0;
        glGetQueryObjectui64v(q, 0x8866 /*GL_QUERY_RESULT*/, &emitted);
        TGLMT_CHECK(emitted == 3, "GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED = 3");

        ClearErrors();
        glBeginQuery(GL_GEOMETRY_SHADER_INVOCATIONS, q);
        glDrawArrays(GL_TRIANGLES, 0, 9);
        glEndQuery(GL_GEOMETRY_SHADER_INVOCATIONS);
        GLuint64 invocations = 0;
        glGetQueryObjectui64v(q, 0x8866 /*GL_QUERY_RESULT*/, &invocations);
        TGLMT_CHECK(invocations == 3, "GL_GEOMETRY_SHADER_INVOCATIONS = 3");

        // Instance: 2 instance ⇒ 6 lần gọi GS, 6 primitive emit.
        ClearErrors();
        glBeginQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED, q);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 9, 2);
        glEndQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED);
        GLuint64 emitted2 = 0;
        glGetQueryObjectui64v(q, 0x8866, &emitted2);
        TGLMT_CHECK(emitted2 == 6, "2 instance ⇒ 6 primitive emit");
    }

    // ===================== 5. MODE MISMATCH =================================
    {
        glUseProgram(progOk);
        ClearErrors();
        glDrawArrays(GL_LINES, 0, 6);
        GLenum e = glGetError();
        TGLMT_CHECK(e == GL_INVALID_OPERATION,
                    "mode không tương thích input primitive của GS ⇒ INVALID_OPERATION");
        ClearErrors();
    }

    // ===================== 6. KẾ HOẠCH MỞ RỘNG (side-table) ================
    {
        GsProgramInfo info;
        TGLMT_CHECK(GetGeometryProgramInfo(progOk, ctx.programs[progOk].linkGen, &info),
                    "side-table program GS phải có sau link");
        if (info.gs.ok) {
            TGLMT_CHECK(info.gs.inPrim == GsPrimType::Triangles, "side-table: input triangles");
            TGLMT_CHECK(!info.gs.outputs.empty(), "side-table: có output của GS");
        }
        TGLMT_CHECK(!info.fsFields.empty(), "side-table: phải có field FS để sinh passthrough VS");
    }

    // ===================== 7. MSL THẬT (nếu có backend Metal) ===============
    // Nếu máy có Metal thật: biên dịch MSL sinh ra bằng compiler Metal thật
    // (đây là bước duy nhất kiểm được MSL thật — backend Null không có GPU).
    {
        auto dev = metal::CreateDevice("apple", nullptr);
        if (dev && !dev->isNull()) {
            GsTranslation t = TranslateGeometryShader(kGsSimple, kUniformStruct, MakeVsOutputs());
            TGLMT_CHECK(t.ok, "dịch GS để biên dịch thật");
            std::string err;
            auto lib = dev->compileLibrary(t.msl, err);
            TGLMT_CHECK(lib != nullptr, "GS kernel MSL phải biên dịch được bằng Metal thật");
            if (!lib) printf("  kernel err: %s\n--- MSL ---\n%.2000s\n", err.c_str(), t.msl.c_str());
            std::vector<GsFsField> fields;
            GsFsField f;
            f.name = "gColor";
            f.mslType = "float3";
            f.location = 0;
            fields.push_back(f);
            std::string pass = BuildGeometryPassthroughMsl(t, fields);
            auto lib2 = dev->compileLibrary(pass, err);
            TGLMT_CHECK(lib2 != nullptr, "passthrough vertex MSL phải biên dịch được");
            if (!lib2) printf("  passthrough err: %s\n--- MSL ---\n%.1500s\n", err.c_str(),
                              pass.c_str());
        }
    }

    // ===================== 8. PIXEL THẬT (backend Metal) ====================
    // Chứng minh GS THẬT SỰ mở rộng hình: 1 tam giác nhỏ ở input → GS sinh ra
    // tam giác lớn (case A) / hình vuông (case B, 2 primitive output) → pixel
    // trong vùng mở rộng có màu của GS, ngoài vùng đó là màu clear.
    {
        Context apple("apple");
        Context::MakeCurrent(&apple);
        if (apple.device->isNull()) {
            printf("test_geometry_emulation: pixel test SKIP (no metal device)\n");
        } else {
            const int W = 64, H = 64;
            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            std::vector<uint8_t> px((size_t)W * H * 4, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         px.data());
            GLuint fbo = 0;
            glGenFramebuffers(1, &fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   tex, 0);
            glViewport(0, 0, W, H);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_BLEND);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            const char* vs =
                "#version 460 core\n"
                "layout(location = 0) in vec2 aPos;\n"
                "out vec3 vColor;\n"
                "void main() { vColor = vec3(1.0, 0.0, 0.0);"
                " gl_Position = vec4(aPos, 0.0, 1.0); }\n";
            const char* fs =
                "#version 460 core\n"
                "in vec3 gColor;\n"
                "out vec4 c;\n"
                "void main() { c = vec4(gColor, 1.0); }\n";
            // GS case A: 1 tam giác input → 1 tam giác output LỚN.
            const char* gsTri =
                "#version 460 core\n"
                "layout(triangles) in;\n"
                "layout(triangle_strip, max_vertices = 3) out;\n"
                "in vec3 vColor;\n"
                "out vec3 gColor;\n"
                "void main() {\n"
                "  gl_Position = vec4(-0.8, -0.8, 0.0, 1.0); gColor = vec3(0.0,1.0,0.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.8, -0.8, 0.0, 1.0); gColor = vec3(0.0,1.0,0.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.0,  0.8, 0.0, 1.0); gColor = vec3(0.0,1.0,0.0);"
                " EmitVertex();\n"
                "  EndPrimitive();\n"
                "}\n";
            // GS case B: 1 tam giác input → 2 tam giác output (hình vuông).
            const char* gsQuad =
                "#version 460 core\n"
                "layout(triangles) in;\n"
                "layout(triangle_strip, max_vertices = 6) out;\n"
                "in vec3 vColor;\n"
                "out vec3 gColor;\n"
                "void main() {\n"
                "  gl_Position = vec4(-0.6, -0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.6, -0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4(-0.6,  0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  EndPrimitive();\n"
                "  gl_Position = vec4(-0.6,  0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.6, -0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.6,  0.6, 0.0, 1.0); gColor = vec3(0.0,0.0,1.0);"
                " EmitVertex();\n"
                "  EndPrimitive();\n"
                "}\n";
            // GS case C: có biến cục bộ (kiểu GLSL phải đổi sang kiểu MSL) và
            // uniform dùng CHUNG với vertex shader.
            const char* vsU =
                "#version 460 core\n"
                "layout(location = 0) in vec2 aPos;\n"
                "uniform float uScale;\n"
                "out vec3 vColor;\n"
                "void main() { vColor = vec3(1.0, 0.5, 0.0);"
                " gl_Position = vec4(aPos * uScale, 0.0, 1.0); }\n";
            const char* gsLocal =
                "#version 460 core\n"
                "layout(triangles) in;\n"
                "layout(triangle_strip, max_vertices = 3) out;\n"
                "in vec3 vColor;\n"
                "out vec3 gColor;\n"
                "void main() {\n"
                "  vec3 tmp = vColor[0];\n"
                "  tmp = tmp * 0.0 + vec3(1.0, 1.0, 0.0);\n"
                "  gl_Position = vec4(-0.6, -0.6, 0.0, 1.0); gColor = tmp; EmitVertex();\n"
                "  gl_Position = vec4( 0.6, -0.6, 0.0, 1.0); gColor = tmp; EmitVertex();\n"
                "  gl_Position = vec4( 0.0,  0.6, 0.0, 1.0); gColor = tmp; EmitVertex();\n"
                "  EndPrimitive();\n"
                "}\n";
            // GS case D: output `flat` + FS input `flat` (qualifier nội suy).
            const char* vsFlat =
                "#version 460 core\n"
                "layout(location = 0) in vec2 aPos;\n"
                "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";
            const char* fsFlat =
                "#version 460 core\n"
                "flat in int gId;\n"
                "out vec4 c;\n"
                "void main() { c = vec4(float(gId) / 255.0, 0.0, 0.0, 1.0); }\n";
            const char* gsFlat =
                "#version 460 core\n"
                "layout(triangles) in;\n"
                "layout(triangle_strip, max_vertices = 3) out;\n"
                "flat out int gId;\n"
                "void main() {\n"
                "  gId = 64;\n"
                "  gl_Position = vec4(-0.6, -0.6, 0.0, 1.0); EmitVertex();\n"
                "  gl_Position = vec4( 0.6, -0.6, 0.0, 1.0); EmitVertex();\n"
                "  gl_Position = vec4( 0.0,  0.6, 0.0, 1.0); EmitVertex();\n"
                "  EndPrimitive();\n"
                "}\n";
            // GS case E: input points → output line_strip (dùng cho draw indexed).
            const char* gsPoints =
                "#version 460 core\n"
                "layout(points) in;\n"
                "layout(line_strip, max_vertices = 2) out;\n"
                "void main() {\n"
                "  gl_Position = vec4(-0.5, -0.5, 0.0, 1.0); EmitVertex();\n"
                "  gl_Position = vec4( 0.5, -0.5, 0.0, 1.0); EmitVertex();\n"
                "  EndPrimitive();\n"
                "}\n";
            // GS case F: KHÔNG gọi EndPrimitive() — primitive cuối phải được
            // đóng tự động (GLSL 3.20 §8.10: "When a geometry shader
            // terminates, the current output primitive is automatically
            // completed").
            const char* gsNoEnd =
                "#version 460 core\n"
                "layout(triangles) in;\n"
                "layout(triangle_strip, max_vertices = 3) out;\n"
                "in vec3 vColor;\n"
                "out vec3 gColor;\n"
                "void main() {\n"
                "  gl_Position = vec4(-0.7, -0.7, 0.0, 1.0); gColor = vec3(1.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.7, -0.7, 0.0, 1.0); gColor = vec3(1.0,0.0,1.0);"
                " EmitVertex();\n"
                "  gl_Position = vec4( 0.0,  0.7, 0.0, 1.0); gColor = vec3(1.0,0.0,1.0);"
                " EmitVertex();\n"
                "}\n";
            const char* fsCyan =
                "#version 460 core\n"
                "out vec4 c;\n"
                "void main() { c = vec4(0.0, 1.0, 1.0, 1.0); }\n";
            // Vertex buffer: tam giác nhỏ (input) — GS không dùng vị trí này.
            GLuint vao = 0, vbo = 0;
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glGenBuffers(1, &vbo);
            {
                float v[6] = {0.0f, 0.0f, 0.05f, 0.0f, 0.0f, 0.05f};
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, (void*)0);
            }
            // Query counter + đọc pixel.
            auto runCase = [&](const char* vsSrc, const char* gsSrc, const char* fsSrc,
                               GLenum mode, bool indexed, int px, int py, bool wantInside,
                               unsigned char r, unsigned char g, unsigned char b,
                               const char* what) {
                GLuint v = MakeShader(GL_VERTEX_SHADER, vsSrc);
                GLuint gsh = MakeShader(GL_GEOMETRY_SHADER, gsSrc);
                GLuint f = MakeShader(GL_FRAGMENT_SHADER, fsSrc);
                GLuint p = glCreateProgram();
                glAttachShader(p, v);
                glAttachShader(p, gsh);
                glAttachShader(p, f);
                glLinkProgram(p);
                GLint ok = 0;
                glGetProgramiv(p, GL_LINK_STATUS, &ok);
                if (!ok) {
                    TGLMT_CHECK(false, what);
                    printf("  link log: %s\n", ProgramLog(p).c_str());
                    return;
                }
                glUseProgram(p);
                glClear(GL_COLOR_BUFFER_BIT);
                GLuint q = 0;
                glGenQueries(1, &q);
                glBeginQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED, q);
                if (indexed) glDrawElements(mode, 3, GL_UNSIGNED_SHORT, (void*)0);
                else glDrawArrays(mode, 0, 3);
                glEndQuery(GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED);
                GLuint64 emitted = 0;
                glGetQueryObjectui64v(q, 0x8866 /*GL_QUERY_RESULT*/, &emitted);
                unsigned char out[4] = {0, 0, 0, 0};
                ReadPixel(px, py, out);
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "%s: pixel(%d,%d) = %d,%d,%d (mong đợi %s %d,%d,%d), "
                         "GS_PRIMITIVES_EMITTED=%llu",
                         what, px, py, out[0], out[1], out[2],
                         wantInside ? "trong vùng mở rộng" : "ngoài vùng mở rộng",
                         r, g, b, (unsigned long long)emitted);
                TGLMT_CHECK(glGetError() == 0, "draw GS không sinh lỗi GL");
                if (wantInside) {
                    TGLMT_CHECK(out[0] == r && out[1] == g && out[2] == b, msg);
                } else {
                    TGLMT_CHECK(out[0] == 0 && out[1] == 0 && out[2] == 0, msg);
                }
            };
            // Case A: tâm tam giác mở rộng = xanh; góc trên-trái = nền.
            runCase(vs, gsTri, fs, GL_TRIANGLES, false, 32, 32, true, 0, 255, 0,
                    "GS case A (tam giác lớn)");
            runCase(vs, gsTri, fs, GL_TRIANGLES, false, 2, 60, false, 0, 0, 0,
                    "GS case A (ngoài tam giác)");
            // Case B: góc trong hình vuông = xanh dương (ngoài tam giác case A).
            runCase(vs, gsQuad, fs, GL_TRIANGLES, false, 48, 48, true, 0, 0, 255,
                    "GS case B (hình vuông)");
            runCase(vs, gsQuad, fs, GL_TRIANGLES, false, 2, 60, false, 0, 0, 0,
                    "GS case B (ngoài hình vuông)");
            // Case C: biến cục bộ + uniform dùng chung VS.
            runCase(vsU, gsLocal, fs, GL_TRIANGLES, false, 32, 28, true, 255, 255, 0,
                    "GS case C (biến cục bộ + uniform chung)");
            // Case D: flat out → flat in.
            runCase(vsFlat, gsFlat, fsFlat, GL_TRIANGLES, false, 32, 28, true, 64, 0, 0,
                    "GS case D (flat)");
            // Case E: points → line_strip, draw INDEXED (EBO).
            {
                GLuint ebo = 0;
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                unsigned short idx[3] = {0, 1, 2};
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
                runCase(vsFlat, gsPoints, fsCyan, GL_POINTS, true, 32, 15, true, 0, 255, 255,
                        "GS case E (points→line_strip, indexed)");
            }
            // Case F: không gọi EndPrimitive() → primitive cuối đóng tự động.
            runCase(vs, gsNoEnd, fs, GL_TRIANGLES, false, 32, 30, true, 255, 0, 255,
                    "GS case F (không gọi EndPrimitive)");
        }
    }

    printf("test_geometry_emulation: %s\n",
           tglmt_test::Failures() ? "FAIL" : "PASS");
    return TGLMT_RESULT;
}
