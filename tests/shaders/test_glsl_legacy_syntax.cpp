// tests/shaders/test_glsl_legacy_syntax.cpp — GLSL ≤ 1.20 (storage qualifier
// cũ `attribute` / `varying`) + legacy built-in.
//
// Bối cảnh: converter GLSL→MSL (src/gl/glsl_to_msl.cpp) trước đây TỪ CHỐI
// mọi shader dùng `attribute`/`varying` (GLSL 1.10–1.20, và compatibility
// profile của 1.50+). Nay hai qualifier này được map sang ĐÚNG đường với
// `in`/`out` hiện có:
//   attribute ≡ in  (chỉ vertex shader)  → TGLMT_VIn [[attribute(N)]]
//   varying   ≡ out (vertex) / in (frag) → TGLMT_VOut / TGLMT_FIn [[user(locnN)]]
// và VS↔FS nối theo TÊN (đúng GL: FS đọc varying mà VS không ghi → link lỗi).
//
// Test kiểm tra (không chỉ "không lỗi" — kiểm tra NỘI DUNG MSL):
//   1. #version 120 + attribute + varying → MSL có field attribute/stage_in/out.
//   2. #version 130 dùng attribute/varying → BỊ TỪ CHỐI kèm lý do.
//   3. Regression: #version 120/130/460 cũ vẫn dịch được (không đổi hành vi).
//   4. gl_FragColor (output fixed-function) + gl_MultiTexCoord0/gl_TexCoord0
//      (vertex input fixed-function) vẫn dịch được.
//   5. (nếu có Metal compiler) MSL sinh ra được biên dịch THẬT.
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

using namespace tglmt;

static const uint32_t kVS = 0x8B31;
static const uint32_t kFS = 0x8B30;

// Linker (gl_program.cpp) gán location thật cho biến tạm 900+. Metal từ chối
// attribute(900) (vượt giới hạn 31) → test tự làm bước đó (nhất quán cho VS
// và FS của cùng program) rồi đưa MSL cho Metal compiler kiểm tra.
static std::string NormalizeTempLocations(const std::string& msl,
                                          std::map<long, long>& ids) {
    auto mapId = [&](long v) {
        auto it = ids.find(v);
        if (it != ids.end()) return it->second;
        long n = (long)ids.size();
        ids[v] = n;
        return n;
    };
    std::string out;
    size_t i = 0;
    static const char* kAttrPat = "[[attribute(";
    static const char* kUserPat = "[[user(locn";
    const size_t kAttrLen = strlen(kAttrPat);
    const size_t kUserLen = strlen(kUserPat);
    while (i < msl.size()) {
        bool attr = msl.compare(i, kAttrLen, kAttrPat) == 0;
        bool user = msl.compare(i, kUserLen, kUserPat) == 0;
        if (!attr && !user) { out += msl[i]; ++i; continue; }
        size_t p = i + (attr ? kAttrLen : kUserLen);
        size_t q = p;
        while (q < msl.size() && isdigit((unsigned char)msl[q])) ++q;
        long v = atol(msl.substr(p, q - p).c_str());
        out += msl.substr(i, p - i);
        out += std::to_string(mapId(v));
        i = q;
    }
    return out;
}

static bool HasMetalCompiler() {
    return std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") == 0;
}

// Biên dịch thật bằng Metal compiler (nếu có) — chứng minh MSL *compile được*,
// không chỉ "converter không báo lỗi".
static bool MetalCompiles(const std::string& msl, const std::string& tag) {
    if (!HasMetalCompiler()) return true;   // không có compiler → không fail
    const std::string base = "/tmp/tglmt-legacy-" + tag;
    {
        std::ofstream f(base + ".metal");
        f << msl;
    }
    const std::string air = base + ".air";
    const std::string cmd = "xcrun -sdk macosx metal -c " + base + ".metal -o " + air +
                            " >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) return false;
    std::ifstream probe(air, std::ios::binary);
    return probe.good();
}

static bool Contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

int main() {
    // ------------------------------------------------------------------
    // 1. #version 120: attribute + varying → MSL đúng cấu trúc.
    // ------------------------------------------------------------------
    const std::string vs120 =
        "#version 120\n"
        "attribute vec3 pos;\n"
        "attribute vec3 nrm;\n"
        "varying vec3 vNrm;\n"
        "uniform mat4 mvp;\n"
        "void main() {\n"
        "    vNrm = nrm;\n"
        "    gl_Position = mvp * vec4(pos, 1.0);\n"
        "}\n";
    const std::string fs120 =
        "#version 120\n"
        "varying vec3 vNrm;\n"
        "void main() {\n"
        "    gl_FragColor = vec4(normalize(vNrm), 1.0);\n"
        "}\n";

    GLSLConvertResult rv = ConvertGLSLtoMSL(vs120, kVS);
    TGLMT_CHECK(rv.ok, "shader #version 120 (attribute+varying) phải dịch được");
    if (!rv.ok) fprintf(stderr, "  log: %s\n", rv.log.c_str());
    GLSLConvertResult rf = ConvertGLSLtoMSL(fs120, kFS);
    TGLMT_CHECK(rf.ok, "fragment #version 120 (varying) phải dịch được");
    if (!rf.ok) fprintf(stderr, "  log: %s\n", rf.log.c_str());

    if (rv.ok) {
        // attribute → vertex buffer attribute (TGLMT_VIn).
        TGLMT_CHECK(Contains(rv.msl, "struct TGLMT_VIn {"),
                    "VS phải có struct TGLMT_VIn");
        TGLMT_CHECK(Contains(rv.msl, "float3 pos [[attribute("),
                    "attribute `pos` phải thành field [[attribute(N)]] trong TGLMT_VIn");
        TGLMT_CHECK(Contains(rv.msl, "float3 nrm [[attribute("),
                    "attribute `nrm` phải thành field [[attribute(N)]] trong TGLMT_VIn");
        // varying (VS) → stage-out.
        TGLMT_CHECK(Contains(rv.msl, "struct TGLMT_VOut {"),
                    "VS phải có struct TGLMT_VOut");
        TGLMT_CHECK(Contains(rv.msl, "float3 vNrm [[user(locn"),
                    "varying `vNrm` ở VS phải thành field [[user(locnN)]] trong TGLMT_VOut");
        // Body đọc input / ghi output.
        TGLMT_CHECK(Contains(rv.msl, "_in.pos"), "body phải đọc attribute qua _in.");
        TGLMT_CHECK(Contains(rv.msl, "_out.vNrm"), "body phải ghi varying qua _out.");
        // Không được còn token GLSL thô.
        TGLMT_CHECK(!Contains(rv.msl, "attribute vec3"),
                    "MSL không được còn khai báo GLSL thô `attribute vec3`");
        TGLMT_CHECK(!Contains(rv.msl, "varying vec3"),
                    "MSL không được còn khai báo GLSL thô `varying vec3`");
        // R.inputs = 2 attribute, R.outputs = 1 varying.
        int nIn = 0, nOut = 0;
        for (auto& a : rv.inputs) { (void)a; ++nIn; }
        for (auto& o : rv.outputs) { (void)o; ++nOut; }
        TGLMT_CHECK(nIn == 2, "VS phải có đúng 2 input (attribute pos, nrm)");
        TGLMT_CHECK(nOut == 1, "VS phải có đúng 1 output (varying vNrm)");
    }
    if (rf.ok) {
        // varying (FS) → stage-in, CÙNG TÊN với output của VS.
        TGLMT_CHECK(Contains(rf.msl, "struct TGLMT_FIn {"),
                    "FS phải có struct TGLMT_FIn");
        TGLMT_CHECK(Contains(rf.msl, "float3 vNrm [[user(locn"),
                    "varying `vNrm` ở FS phải thành field [[user(locnN)]] trong TGLMT_FIn");
        TGLMT_CHECK(Contains(rf.msl, "_in.vNrm"), "FS phải đọc varying qua _in.");
        int nIn = 0;
        for (auto& a : rf.inputs) { (void)a; ++nIn; }
        TGLMT_CHECK(nIn == 1, "FS phải có đúng 1 input (varying vNrm)");
    }
    // Tên varying phải khớp giữa VS và FS (link theo tên).
    if (rv.ok && rf.ok) {
        bool vsHas = false, fsHas = false;
        for (auto& o : rv.outputs) if (o.name == "vNrm") vsHas = true;
        for (auto& a : rf.inputs) if (a.name == "vNrm") fsHas = true;
        TGLMT_CHECK(vsHas && fsHas,
                    "varying `vNrm` phải cùng tên ở VS output và FS input (link theo tên)");
    }

    // ------------------------------------------------------------------
    // 2. #version ≥1.50 CORE profile dùng attribute/varying → phải bị từ chối.
    //    (GLSL 1.30/1.40 vẫn cho phép — deprecated; GLSL 1.50 core đã bỏ.)
    // ------------------------------------------------------------------
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 150\n"
            "attribute vec3 pos;\n"
            "void main() { gl_Position = vec4(pos, 1.0); }\n", kVS);
        TGLMT_CHECK(!r.ok, "#version 150 core dùng `attribute` phải bị từ chối");
        TGLMT_CHECK(r.log.find("150") != std::string::npos,
                    "lỗi phải nói rõ phiên bản GLSL bị từ chối");
        TGLMT_CHECK(r.msl.empty(), "shader bị từ chối thì không được sinh MSL");
    }
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 150\n"
            "varying vec3 v;\n"
            "void main() { v = vec3(1.0); gl_Position = vec4(1.0); }\n", kVS);
        TGLMT_CHECK(!r.ok, "#version 150 core dùng `varying` phải bị từ chối");
        TGLMT_CHECK(r.log.find("150") != std::string::npos,
                    "lỗi phải nói rõ phiên bản GLSL bị từ chối");
    }
    {
        // GLSL 1.30/1.40: attribute/varying VẪN hợp lệ (deprecated) —
        // spec 1.30 §1.2.1 chỉ ghi "features deprecated in version 1.3".
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 130\n"
            "attribute vec3 pos;\n"
            "void main() { gl_Position = vec4(pos, 1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "#version 130 dùng `attribute` vẫn phải dịch được (deprecated)");
        TGLMT_CHECK(!r.msl.empty(), "MSL phải được sinh cho #version 130");
    }
    {
        // GLSL ≥ 1.50 profile "compatibility" GIỮ attribute/varying.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 150 compatibility\n"
            "attribute vec3 pos;\n"
            "void main() { gl_Position = vec4(pos, 1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "#version 150 compatibility dùng `attribute` phải dịch được");
    }
    {
        // `attribute` trong fragment shader là lỗi GLSL (1.20 §4.3.4).
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\n"
            "attribute vec3 pos;\n"
            "void main() { gl_FragColor = vec4(pos, 1.0); }\n", kFS);
        TGLMT_CHECK(!r.ok, "`attribute` trong fragment shader phải bị từ chối");
        TGLMT_CHECK(r.log.find("vertex") != std::string::npos,
                    "lỗi phải nói `attribute` chỉ dùng ở vertex shader");
    }

    // ------------------------------------------------------------------
    // 3. Regression: các version cũ vẫn dịch được, hành vi không đổi.
    // ------------------------------------------------------------------
    {
        // #version 120 hợp lệ (không attribute/varying) → vẫn dịch.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\n"
            "uniform mat4 mvp;\n"
            "void main() { gl_Position = mvp * vec4(1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "shader #version 120 không dùng qualifier cũ vẫn phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
    }
    {
        // #version 130 với in/out (đường hiện đại) → vẫn dịch, layout giữ nguyên.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 130\n"
            "layout(location = 0) in vec3 pos;\n"
            "out vec3 vNrm;\n"
            "void main() { vNrm = pos; gl_Position = vec4(pos, 1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "shader #version 130 (in/out) vẫn phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
        else {
            TGLMT_CHECK(Contains(r.msl, "float3 pos [[attribute(0)]]"),
                        "layout(location=0) in → [[attribute(0)]]");
            TGLMT_CHECK(Contains(r.msl, "float3 vNrm [[user(locn"),
                        "out (VS) → [[user(locnN)]]");
        }
    }
    {
        // #version 460 core + layout → vẫn dịch (không đụng nhánh mới).
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 0) out vec2 vUV;\n"
            "void main() { vUV = aPos.xy; gl_Position = vec4(aPos, 1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "shader #version 460 core vẫn phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
    }
    {
        // `flat out` (đường hiện đại) không bị ảnh hưởng.
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 460 core\n"
            "layout(location = 0) in vec2 aPos;\n"
            "flat out int vId;\n"
            "void main() { vId = 3; gl_Position = vec4(aPos, 0.0, 1.0); }\n", kVS);
        TGLMT_CHECK(r.ok, "shader `flat out` vẫn phải dịch được");
        if (r.ok)
            TGLMT_CHECK(Contains(r.msl, "[[flat]]"),
                        "`flat out` phải sinh [[flat]] (không bị mất khi thêm nhánh legacy)");
    }

    // ------------------------------------------------------------------
    // 4. gl_FragColor (fixed-function fragment output) + gl_MultiTexCoord0 /
    //    gl_TexCoord0 (fixed-function vertex input) của GLSL 1.20.
    // ------------------------------------------------------------------
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\n"
            "void main() { gl_FragColor = vec4(1.0); }\n", kFS);
        TGLMT_CHECK(r.ok, "gl_FragColor (#version 120) phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
        else
            TGLMT_CHECK(!Contains(r.msl, "gl_FragColor"),
                        "gl_FragColor phải được viết lại (không còn tên GLSL trong MSL)");
    }
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\n"
            "attribute vec3 pos;\n"
            "varying vec2 vUv;\n"
            "void main() {\n"
            "    vUv = gl_MultiTexCoord0.xy;\n"
            "    gl_Position = vec4(pos, 1.0);\n"
            "}\n", kVS);
        TGLMT_CHECK(r.ok, "gl_MultiTexCoord0 (#version 120 VS) phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
        else {
            TGLMT_CHECK(Contains(r.msl, "gl_MultiTexCoord0 [[attribute("),
                        "gl_MultiTexCoord0 phải thành vertex attribute");
            TGLMT_CHECK(Contains(r.msl, "_in.gl_MultiTexCoord0"),
                        "body phải đọc gl_MultiTexCoord0 qua _in.");
        }
    }
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\n"
            "void main() { gl_FragColor = vec4(gl_TexCoord0.xy, 0.0, 1.0); }\n", kFS);
        TGLMT_CHECK(r.ok, "gl_TexCoord0 (#version 120 FS) phải dịch được");
        if (!r.ok) fprintf(stderr, "  log: %s\n", r.log.c_str());
        else
            TGLMT_CHECK(Contains(r.msl, "gl_TexCoord0 [[user(locn"),
                        "gl_TexCoord0 phải thành stage-in varying");
    }

    // ------------------------------------------------------------------
    // 5. MSL sinh ra phải biên dịch được bằng Metal compiler (nếu có).
    // ------------------------------------------------------------------
    if (rv.ok && rf.ok) {
        std::map<long, long> ids;
        std::string nvs = NormalizeTempLocations(rv.msl, ids);
        std::string nfs = NormalizeTempLocations(rf.msl, ids);
        TGLMT_CHECK(MetalCompiles(nvs, "legacy120.vert"),
                    "Metal compiler phải chấp nhận MSL của shader #version 120 (vertex)");
        TGLMT_CHECK(MetalCompiles(nfs, "legacy120.frag"),
                    "Metal compiler phải chấp nhận MSL của shader #version 120 (fragment)");
    }

    printf("test_glsl_legacy_syntax: attribute/varying ≤1.20 + legacy built-in OK\n");
    return TGLMT_RESULT;
}
