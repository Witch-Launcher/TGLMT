// tests/shaders/test_glsl_builtin_final.cpp — kiểm thử nhóm built-in GLSL
// CUỐI CÙNG (giai đoạn "final", đóng nốt 5 gap đo bởi tools/glsl_probe.cpp):
//
//   1. gl_BaseVertex (VS, GLSL 4.60 §7.1.1, PDF p.143-145):
//      Metal chỉ chấp nhận [[base_vertex]] làm THAM SỐ HÀM vertex (probe Metal
//      compiler: compile OK), KHÔNG hợp lệ trong [[stage_in]] (probe: reject)
//      ⇒ converter phát tham số `uint tglmt_baseVertex [[base_vertex]]`.
//   2. gl_DrawID (VS, GLSL 4.60 §7.1.1, PDF p.143-145; OpenGL 4.6 §11.1.3.9,
//      glspec46.core.pdf p.426): Metal KHÔNG có [[draw_index]] (probe: "unknown
//      attribute 'draw_index' ignored") ⇒ uniform `int tglmt_DrawID` do backend
//      điền mỗi draw (giá trị = chỉ số draw trong MultiDraw*; draw đơn = 0).
//   3. gl_NumSamples (FS, GLSL 4.60 §7.4 "Built-In Uniform State", PDF p.159):
//      Metal không có attribute sample-count cho FS; texture2d<float>::
//      sample_count không tồn tại trên toolchain này (probe: "no member named
//      'sample_count'") ⇒ uniform `int tglmt_NumSamples` do backend điền =
//      sampleCount của render target.
//   4. gl_ClipDistance[0] OK; [≥1]: Metal chỉ có MỘT [[clip_distance]] ⇒
//      converter phải báo lỗi RÕ RÀNG ("GL chỉ hỗ trợ 1 clip distance trên
//      Metal") — KHÔNG được âm thầm bỏ (silent wrong-translation).
//   5. TCS: khối `out gl_PerVertex { ... } gl_out[];` là INTRINSIC (GLSL
//      4.60 §7.1.2, PDF p.145) ⇒ nguồn dùng đúng form intrinsic phải dịch
//      được (pass "final_tcs" mức nguồn tự chèn khai báo nếu thiếu); khối
//      khai báo TƯỜNG MINH vẫn dịch như cũ.
//
// Mỗi built-in được kiểm bằng cách dịch shader dùng nó và kiểm tra MSL sinh
// ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash"):
//   • form MSL đúng (tham số/field/uniform member, attribute đúng chỗ);
//   • tên GLSL không còn trong MSL;
//   • uniform mới (tglmt_DrawID/tglmt_NumSamples) có trong conv.uniforms
//     (link reflection — backend điền giá trị mỗi draw);
//   • MSL biên dịch được bằng compiler Metal thật (nếu có);
//   • shader KHÔNG dùng nhóm này: MSL không đổi (không thêm uniform/tham số).
#include "tglmt/GLSLConverter.h"
#include "tglmt/gl46_types.h"
#include "tglmt/GLSLBuiltins.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;

namespace {

std::string ToMsl(const std::string& glsl, uint32_t stage) {
    GLSLConvertResult r = ConvertGLSLtoMSL(glsl, stage);
    if (!r.ok) {
        fprintf(stderr, "convert that bai: %s\n", r.log.c_str());
        return std::string();
    }
    return r.msl;
}

bool Has(const std::string& s, const char* needle) {
    return s.find(needle) != std::string::npos;
}

// Biên dịch MSL bằng compiler Metal thật (nếu có). raw = true: không remap
// location tạm 900+ (MSL của nhóm này không sinh locn900).
bool MetalCompiles(const std::string& msl, std::string* err) {
    if (std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") != 0)
        return true;  // không có compiler ⇒ bỏ qua (không tạo pass giả)
    const char* path = "/tmp/tglmt_final_check.metal";
    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fputs(msl.c_str(), f);
    fclose(f);
    std::string cmd = std::string("xcrun -sdk macosx metal -c ") + path + " -o /dev/null 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return false;
    char buf[4096];
    size_t n;
    std::string out;
    while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
    int rc = pclose(p);
    if (rc != 0 && err) *err = out;
    return rc == 0;
}

// Uniform do pass thêm có trong conv.uniforms (link reflection) không?
bool HasUniform(const GLSLConvertResult& r, const char* name) {
    for (const auto& u : r.uniforms)
        if (u.name == name) return true;
    return false;
}

bool HasUniformType(const GLSLConvertResult& r, const char* name, const char* mslType) {
    for (const auto& u : r.uniforms)
        if (u.name == name) return u.mslType == mslType;
    return false;
}

// Struct MSL có tên `name` (vd "TGLMTUniforms").
std::string StructBody(const std::string& msl, const char* name) {
    std::string key = std::string("struct ") + name + " {";
    size_t p = msl.find(key);
    if (p == std::string::npos) return std::string();
    size_t e = msl.find("\n};", p);
    if (e == std::string::npos) return std::string();
    return msl.substr(p, e - p);
}

// Pass/source-pass có đăng ký không?
bool ContainsId(const std::vector<std::string>& ids, const char* want) {
    for (const auto& s : ids)
        if (s == want) return true;
    return false;
}

}  // namespace

int main() {
    const uint32_t kVsStage = GL_VERTEX_SHADER;    // 0x8B31
    const uint32_t kFsStage = GL_FRAGMENT_SHADER;  // 0x8B30
    const uint32_t kTcsStage = GL_TESS_CONTROL_SHADER;  // 0x8E88
    const uint32_t kCsStage = GL_COMPUTE_SHADER;   // 0x91B9

    // =================================================================
    // 1. gl_BaseVertex — GLSL 4.60 §7.1.1 (PDF p.143-145):
    //    "The variable gl_BaseVertex is a vertex shader input variable that
    //    holds the integer value passed to the baseVertex parameter of the
    //    command that resulted in the current shader invocation."
    //    → MSL: tham số `uint tglmt_baseVertex [[base_vertex]]` của hàm vertex
    //    (KHÔNG hợp lệ trong [[stage_in]] — probe Metal compiler reject).
    // =================================================================
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_BaseVertex)); }\n",
            kVsStage);
        TGLMT_CHECK(r.ok, "gl_BaseVertex: convert phai thanh cong");
        std::string msl = r.msl;
        TGLMT_CHECK(Has(msl, "uint tglmt_baseVertex [[base_vertex]]"),
                    "gl_BaseVertex: phai la THAM SO HAM vertex uint tglmt_baseVertex [[base_vertex]]");
        TGLMT_CHECK(Has(msl, "float(tglmt_baseVertex)"),
                    "gl_BaseVertex: body phai doc tu tham so");
        TGLMT_CHECK(!Has(msl, "gl_BaseVertex"),
                    "gl_BaseVertex: ten GLSL khong con trong MSL");
        TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_VIn"), "base_vertex"),
                    "gl_BaseVertex: [[base_vertex]] KHONG duoc nam trong struct [[stage_in]]");
        std::string err;
        if (!MetalCompiles(msl, &err)) {
            fprintf(stderr, "--- metal compile fail (gl_BaseVertex) ---\n%s\n", err.c_str());
            TGLMT_CHECK(false, "gl_BaseVertex: MSL phai compile duoc boi Metal compiler");
        }
    }
    {
        // VS KHÔNG dùng gl_BaseVertex ⇒ KHÔNG được thêm tham số (non-regression).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_Position = vec4(1.0); }\n", kVsStage);
        TGLMT_CHECK(!msl.empty(), "non-regression gl_BaseVertex: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "base_vertex"),
                    "non-regression: MSL cua shader khong dung gl_BaseVertex khong duoc them [[base_vertex]]");
        TGLMT_CHECK(!Has(msl, "tglmt_baseVertex"),
                    "non-regression: khong duoc them local tglmt_baseVertex khi khong dung");
    }

    // =================================================================
    // 2. gl_DrawID — GLSL 4.60 §7.1.1 (PDF p.143-145) + OpenGL 4.6 §11.1.3.9
    //    (glspec46.core.pdf p.426): "gl_DrawID holds the integer draw number...
    //    In MultiDraw* variants, this is the zero-based index of the draw ...
    //    In non- MultiDraw* commands, the value of gl_DrawID is always zero."
    //    Metal KHÔNG có [[draw_index]] (probe: "unknown attribute" + "invalid
    //    type for input declaration") ⇒ uniform `int tglmt_DrawID` do backend
    //    điền mỗi draw (xem glsl_builtins_final.cpp mục REP BACKEND).
    // =================================================================
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_DrawID)); }\n",
            kVsStage);
        TGLMT_CHECK(r.ok, "gl_DrawID: convert phai thanh cong (khong fail sớm nữa)");
        std::string msl = r.msl;
        TGLMT_CHECK(Has(msl, "int tglmt_DrawID;"),
                    "gl_DrawID: struct TGLMTUniforms phai co member int tglmt_DrawID");
        TGLMT_CHECK(Has(msl, "tglmt_u.tglmt_DrawID"),
                    "gl_DrawID: body phai doc tu uniform tglmt_u.tglmt_DrawID");
        TGLMT_CHECK(!Has(msl, "gl_DrawID"),
                    "gl_DrawID: ten GLSL khong con trong MSL");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "tglmt_DrawID"),
                    "gl_DrawID: member phai nam trong struct TGLMTUniforms (buffer(16))");
        // Reflection: uniform phải trong conv.uniforms (link gán location).
        TGLMT_CHECK(HasUniform(r, "tglmt_DrawID"),
                    "gl_DrawID: conv.uniforms phai chieu uniform tglmt_DrawID (backend dien moi draw)");
        TGLMT_CHECK(HasUniformType(r, "tglmt_DrawID", "int"),
                    "gl_DrawID: conv.uniforms kieu int");
        std::string err;
        if (!MetalCompiles(msl, &err)) {
            fprintf(stderr, "--- metal compile fail (gl_DrawID) ---\n%s\n", err.c_str());
            TGLMT_CHECK(false, "gl_DrawID: MSL phai compile duoc boi Metal compiler");
        }
    }
    {
        // FS không được dùng gl_DrawID (GLSL §7.1.1 = VS input) ⇒ không uniform
        // nào được thêm; không rewrite (MSL giữ nguyên → lỗi rõ như behavior
        // của mọi builtin sai stage).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_DrawID)); }\n",
            kFsStage);
        TGLMT_CHECK(!msl.empty(), "gl_DrawID(FS): convert phai thanh cong (MSL giu nguyen)");
        TGLMT_CHECK(Has(msl, "gl_DrawID"),
                    "gl_DrawID(FS): khong duoc rewrite trong stage khong hỗ trợ");
        TGLMT_CHECK(!Has(msl, "tglmt_DrawID"),
                    "gl_DrawID(FS): khong duoc them uniform tglmt_DrawID");
    }
    {
        // VS KHÔNG dùng gl_DrawID ⇒ không thêm uniform (non-regression).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_Position = vec4(1.0); }\n", kVsStage);
        TGLMT_CHECK(!msl.empty(), "non-regression gl_DrawID: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_DrawID"),
                    "non-regression: khong uniform tglmt_DrawID khi khong dung");
    }

    // =================================================================
    // 3. gl_NumSamples — GLSL 4.60 §7.4 (PDF p.159):
    //    "uniform int gl_NumSamples; ... only guaranteed to be available in
    //    the fragment stage." Giá trị = số sample của framebuffer hiện tại
    //    (mô tả gl_SampleID, PDF p.150: "gl_NumSamples is the total number of
    //    samples in the framebuffer, or 1 if rendering to a non-multisample
    //    framebuffer").
    //    Metal không có [[sample_count]] cho FS (probe: "unknown attribute
    //    'sample_count' ignored"); texture2d<float>::sample_count KHÔNG tồn
    //    tại trên toolchain này (probe compile error) ⇒ uniform
    //    `int tglmt_NumSamples` do backend điền = sampleCount render target.
    // =================================================================
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460\nvoid main(){ gl_FragColor = vec4(float(gl_NumSamples)); }\n",
            kFsStage);
        TGLMT_CHECK(r.ok, "gl_NumSamples: convert phai thanh cong");
        std::string msl = r.msl;
        TGLMT_CHECK(Has(msl, "int tglmt_NumSamples;"),
                    "gl_NumSamples: struct TGLMTUniforms phai co member int tglmt_NumSamples");
        TGLMT_CHECK(Has(msl, "tglmt_u.tglmt_NumSamples"),
                    "gl_NumSamples: body phai doc tu uniform tglmt_u.tglmt_NumSamples");
        TGLMT_CHECK(!Has(msl, "gl_NumSamples"),
                    "gl_NumSamples: ten GLSL khong con trong MSL");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "tglmt_NumSamples"),
                    "gl_NumSamples: member phai nam trong struct TGLMTUniforms (buffer(16))");
        TGLMT_CHECK(HasUniform(r, "tglmt_NumSamples"),
                    "gl_NumSamples: conv.uniforms phai chieu uniform tglmt_NumSamples (backend dien moi draw)");
        TGLMT_CHECK(HasUniformType(r, "tglmt_NumSamples", "int"),
                    "gl_NumSamples: conv.uniforms kieu int");
        std::string err;
        if (!MetalCompiles(msl, &err)) {
            fprintf(stderr, "--- metal compile fail (gl_NumSamples) ---\n%s\n", err.c_str());
            TGLMT_CHECK(false, "gl_NumSamples: MSL phai compile duoc boi Metal compiler");
        }
    }
    {
        // VS không được đảm bảo có gl_NumSamples (GLSL §7.4) ⇒ không uniform
        // nào được thêm; MSL giữ nguyên gl_NumSamples (behavior hiện có).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_Position = vec4(float(gl_NumSamples)); }\n",
            kVsStage);
        TGLMT_CHECK(!msl.empty(), "gl_NumSamples(VS): convert phai thanh cong (MSL giu nguyen)");
        TGLMT_CHECK(Has(msl, "gl_NumSamples"),
                    "gl_NumSamples(VS): khong duoc rewrite trong stage khong hỗ trợ");
        TGLMT_CHECK(!Has(msl, "tglmt_NumSamples"),
                    "gl_NumSamples(VS): khong duoc them uniform tglmt_NumSamples");
    }
    {
        // FS KHÔNG dùng gl_NumSamples ⇒ không thêm uniform (non-regression).
        std::string msl = ToMsl(
            "#version 460\nvoid main(){ gl_FragColor = vec4(1.0); }\n", kFsStage);
        TGLMT_CHECK(!msl.empty(), "non-regression gl_NumSamples: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "tglmt_NumSamples"),
                    "non-regression: khong uniform tglmt_NumSamples khi khong dung");
    }

    // =================================================================
    // 4. gl_ClipDistance — GLSL 1.50 §7.1.6: Metal có ĐÚNG MỘT
    //    [[clip_distance]] (compiler từ chối mảng). Index 0 chạy đúng;
    //    index ≥ 1 phải fail RÕ RÀNG — KHÔNG được âm thầm bỏ.
    // =================================================================
    {
        std::string msl = ToMsl(
            "#version 150\nvoid main(){ gl_ClipDistance[0] = 1.0; gl_Position = vec4(1.0); }\n",
            kVsStage);
        TGLMT_CHECK(!msl.empty(), "gl_ClipDistance[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(msl, "float tglmt_clipDist [[clip_distance]]"),
                    "gl_ClipDistance[0]: struct VOut phai co field [[clip_distance]]");
        TGLMT_CHECK(!Has(msl, "gl_ClipDistance"),
                    "gl_ClipDistance[0]: ten GLSL khong con trong MSL");
        std::string err;
        if (!MetalCompiles(msl, &err)) {
            fprintf(stderr, "--- metal compile fail (gl_ClipDistance[0]) ---\n%s\n", err.c_str());
            TGLMT_CHECK(false, "gl_ClipDistance[0]: MSL phai compile duoc boi Metal compiler");
        }
    }
    for (int idx : {1, 2, 3, 7}) {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 150\nvoid main(){ gl_ClipDistance[" + std::to_string(idx) +
                "] = 1.0; gl_Position = vec4(1.0); }\n",
            kVsStage);
        TGLMT_CHECK(!r.ok, "gl_ClipDistance[idx>=1]: convert PHAI fail (Metal chi 1 clip_distance)");
        TGLMT_CHECK(r.log.find("GL chỉ hỗ trợ 1 clip distance trên Metal") != std::string::npos,
                    "gl_ClipDistance[idx>=1]: loi phai NOI RO gioi han Metal (honest error)");
    }

    // =================================================================
    // 5. TCS out gl_PerVertex intrinsic — GLSL 4.60 §7.1.2 (PDF p.145):
    //      out gl_PerVertex { vec4 gl_Position; ... } gl_out[];
    //    là INTRINSIC (không cần khai báo tường minh; cùng ngôn ngữ với
    //    GLSL 4.0 §7.3). Converter chỉ nhận khối khai báo ⇒ pass mức nguồn
    //    "final_tcs" chèn khai báo nếu thiếu (chỉ với dấu hiệu TCS:
    //    layout(vertices = N) out;).
    // =================================================================
    {
        // Form INTRINSIC (probe case): KHÔNG có khối out gl_PerVertex trong nguồn.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 400\nlayout(vertices=3) out;\n"
            "void main(){ gl_TessLevelOuter[0] = 1.0; "
            "gl_out[gl_InvocationID].gl_Position = vec4(1.0); }\n",
            kTcsStage);
        TGLMT_CHECK(r.ok, "TCS intrinsic gl_out: convert phai thanh cong");
        std::string msl = r.msl;
        TGLMT_CHECK(Has(msl, "tcs_out_gl_Position[tglmt_patchID*3 + (tglmt_tcs_tid.x)]"),
                    "TCS intrinsic gl_out: gl_out[i].gl_Position phai rewrite thanh ghi buffer");
        TGLMT_CHECK(Has(msl, "edgeTessellationFactor"),
                    "TCS intrinsic gl_out: gl_TessLevelOuter phai rewrite thanh factor buffer");
        TGLMT_CHECK(!Has(msl, "gl_out"),
                    "TCS intrinsic gl_out: ten GLSL gl_out khong con trong MSL");
        std::string err;
        if (!MetalCompiles(msl, &err)) {
            fprintf(stderr, "--- metal compile fail (TCS intrinsic gl_out) ---\n%s\n", err.c_str());
            TGLMT_CHECK(false, "TCS intrinsic gl_out: MSL phai compile duoc boi Metal compiler");
        }
    }
    {
        // Form TƯỜNG MINH (các test repo dùng sẵn): vẫn dịch như cũ.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 400\nlayout(vertices=3) out;\n"
            "out gl_PerVertex { vec4 gl_Position; } gl_out[];\n"
            "void main(){ gl_TessLevelOuter[0] = 1.0; "
            "gl_out[gl_InvocationID].gl_Position = vec4(1.0); }\n",
            kTcsStage);
        TGLMT_CHECK(r.ok, "TCS explicit gl_out: convert phai thanh cong");
        TGLMT_CHECK(Has(r.msl, "tcs_out_gl_Position[tglmt_patchID*3 + (tglmt_tcs_tid.x)]"),
                    "TCS explicit gl_out: phai rewrite nhu cu");
    }
    {
        // Form tường minh CẢ gl_PointSize: member được giữ như subset repo.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 400\nlayout(vertices=3) out;\n"
            "out gl_PerVertex { vec4 gl_Position; float gl_PointSize; } gl_out[];\n"
            "void main(){ gl_out[gl_InvocationID].gl_PointSize = 2.0; }\n",
            kTcsStage);
        TGLMT_CHECK(r.ok, "TCS gl_PointSize: convert phai thanh cong");
        TGLMT_CHECK(Has(r.msl, "tcs_out_gl_PointSize["),
                    "TCS gl_PointSize: phai rewrite thanh ghi buffer tcs_out_gl_PointSize");
    }
    {
        // Compute shader KHÔNG có dấu hiệu TCS ⇒ pass nguồn không chèn gì.
        std::string msl = ToMsl(
            "#version 430\nlayout(local_size_x=1) in;\nvoid main(){}\n", kCsStage);
        TGLMT_CHECK(!msl.empty(), "CS non-regression: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "gl_out"),
                    "CS non-regression: khong duoc chen gl_out vao compute shader");
    }

    // =================================================================
    // Trạng thái đăng ký pass (guard cho việc tự đăng ký bị mất).
    // =================================================================
    TGLMT_CHECK(ContainsId(gl::GLSLBuiltinPassIds(), "final"),
                "pass 'final' phai tu dang ky");
    TGLMT_CHECK(ContainsId(gl::GLSLSourcePassIds(), "final_tcs"),
                "source pass 'final_tcs' phai tu dang ky");

    printf("test_glsl_builtin_final: 5 nhom built-in cuoi cung da kiem tra\n");
    return TGLMT_RESULT;
}
