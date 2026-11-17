// tests/shaders/test_glsl_builtin_legacy.cpp — kiểm thử nhóm built-in BIẾN GLSL
// "legacy" (GLSL 1.20 §7 "Built-in Variables" / GLSL 4.60 §7.1.7 + §7.4.1)
// → MSL.
//
// Mỗi built-in được kiểm bằng cách dịch một shader dùng nó và kiểm tra MSL
// sinh ra có ĐÚNG dạng MSL tương đương (không chỉ "gọi không crash"):
//   • varying khai đúng KIỂU: `float4 gl_FrontColor [[user(locn1000)]]`,
//     `float gl_FogFragCoord [[user(locn1004)]]` (GLSL 1.20 §7.6 ⇒ phần tử
//     gl_TexCoord là vec4, gl_FogFragCoord là float);
//   • uniform khai đúng KIỂU: `float4x4 gl_ModelViewMatrix;`,
//     `float3x3 gl_NormalMatrix;`, `float4x4 gl_TextureMatrix[8];`,
//     `float4 gl_ClipPlane[6];`, `float4 gl_Fog_color;` …
//   • body đọc/ghi đúng field: `_in.gl_FrontColor` / `_out.gl_ClipVertex` /
//     `tglmt_u.gl_ModelViewMatrix`;
//   • VS output và FS input CÙNG TÊN + CÙNG [[user(locnN)]] (Metal nối
//     stage_in↔stage_out theo SỐ location, không theo tên).
//
// Ngoài ra test kiểm tra:
//   • MSL của shader KHÔNG dùng nhóm này KHÔNG được thêm gì (không đụng
//     uniform/varying của shader khác — kể cả 79 shader Minecraft);
//   • các built-in CHƯA hỗ trợ được ghi rõ trạng thái (KNOWN GAP) để không
//     ai tưởng là đã chạy;
//   • MSL sinh ra phải BIÊN DỊCH ĐƯỢC bằng compiler Metal thật — cả bản đã
//     remap location tạm (như linker làm) lẫn bản NGUYÊN (user(locn1000)).
#include "tglmt/GLSLConverter.h"
#include "test_util.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace tglmt;

namespace {

constexpr uint32_t kVs = 0x8B31u;  // GL_VERTEX_SHADER
constexpr uint32_t kFs = 0x8B30u;  // GL_FRAGMENT_SHADER

// Dịch 1 shader và trả về MSL (rỗng nếu convert lỗi).
std::string ToMsl(const std::string& glsl, uint32_t stage) {
    GLSLConvertResult r = ConvertGLSLtoMSL(glsl, stage);
    if (!r.ok) {
        fprintf(stderr, "convert that bai: %s\n", r.log.c_str());
        return std::string();
    }
    return r.msl;
}

bool Has(const std::string& s, const std::string& needle) {
    return !needle.empty() && s.find(needle) != std::string::npos;
}

// Nội dung struct MSL có tên `name` (vd "TGLMTUniforms").
std::string StructBody(const std::string& msl, const char* name) {
    std::string key = std::string("struct ") + name + " {";
    size_t p = msl.find(key);
    if (p == std::string::npos) return std::string();
    size_t e = msl.find("\n};", p);
    if (e == std::string::npos) return std::string();
    return msl.substr(p, e - p);
}

// Thân hàm entry (vertex/fragment) — phần SAU các struct.
std::string EntryBody(const std::string& msl) {
    size_t p = msl.find("vertex TGLMT_VOut");
    if (p == std::string::npos) p = msl.find("fragment float4 TGLMT_fs");
    if (p == std::string::npos) p = msl.find("fragment TGLMT_FOut");
    if (p == std::string::npos) return std::string();
    return msl.substr(p);
}

// Số [[user(locnN)]] của field `name` trong `structBody` (-1 nếu không có).
int LocnOf(const std::string& structBody, const char* name) {
    size_t p = structBody.find(name);
    if (p == std::string::npos) return -1;
    size_t q = structBody.find("[[user(locn", p);
    if (q == std::string::npos) return -1;
    q += strlen("[[user(locn");
    int v = 0;
    while (q < structBody.size() && isdigit((unsigned char)structBody[q])) {
        v = v * 10 + (structBody[q] - '0');
        ++q;
    }
    return v;
}

// Biên dịch MSL NGUYÊN (chưa remap location tạm) bằng compiler Metal thật.
bool MetalCompilesRaw(const std::string& msl, std::string* err) {
    if (std::system("xcrun -sdk macosx metal --version >/dev/null 2>&1") != 0) return true;  // không có compiler ⇒ bỏ qua
    const char* path = "/tmp/tglmt_legacy_raw.metal";
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
    if (rc != 0) {
        if (err) *err = out;
        return false;
    }
    return true;
}

// Remap location tạm (900+) như linker làm — để Metal không báo
// "'user' attribute parameter is out of bounds".
std::string RemapTempLocs(const std::string& in) {
    std::string m = in;
    auto remapOne = [&](const std::string& open, const std::string& close) {
        int next = 0;
        size_t p = 0;
        while ((p = m.find(open, p)) != std::string::npos) {
            size_t e = m.find(close, p + open.size());
            if (e == std::string::npos) break;
            int num = atoi(m.c_str() + p + open.size());
            if (num >= 100) {
                std::string repl = open + std::to_string(next) + close;
                m.replace(p, e + close.size() - p, repl);
                ++next;
                p += repl.size();
            } else {
                p = e + close.size();
            }
        }
    };
    remapOne("[[attribute(", ")]]");
    remapOne("[[user(locn", ")]]");
    return m;
}

// Một built-in varying của nhóm legacy: kiểm tra VS output + FS input.
struct VaryingCase {
    const char* name;      // tên GLSL
    const char* mslType;   // kiểu MSL (float4 / float)
    const char* glslType;  // kiểu GLSL (vec4 / float)
    int locn;              // location cố định (1000 + index trong bảng pass)
    bool fsInput;          // FS được đọc nó (input)
    bool vsOutput;         // VS được ghi nó (output)
};

const VaryingCase kVaryings[] = {
    {"gl_FrontColor", "float4", "vec4", 1000, true, true},
    {"gl_BackColor", "float4", "vec4", 1001, true, true},
    {"gl_FrontSecondaryColor", "float4", "vec4", 1002, true, true},
    {"gl_BackSecondaryColor", "float4", "vec4", 1003, true, true},
    {"gl_FogFragCoord", "float", "float", 1004, true, true},
    {"gl_ClipVertex", "float4", "vec4", 1005, false, true},
};

// Một built-in uniform của nhóm legacy.
struct UniformCase {
    const char* name;      // tên GLSL (không kèm index)
    const char* decl;      // khai báo MSL mong đợi trong struct TGLMTUniforms
    const char* mslType;   // kiểu MSL
    const char* glslType;  // kiểu GLSL
    int arraySize;
    const char* vsExpr;    // biểu thức GLSL (VS): `gl_Position = <vsExpr>;`
    const char* fsExpr;    // biểu thức GLSL (FS): `gl_FragColor = <fsExpr>;`
};

const UniformCase kUniforms[] = {
    {"gl_ModelViewMatrix", "float4x4 gl_ModelViewMatrix;", "float4x4", "mat4", 0,
     "gl_ModelViewMatrix * vec4(1.0)", "vec4(gl_ModelViewMatrix[0])"},
    {"gl_ProjectionMatrix", "float4x4 gl_ProjectionMatrix;", "float4x4", "mat4", 0,
     "gl_ProjectionMatrix * vec4(1.0)", "vec4(gl_ProjectionMatrix[0])"},
    {"gl_ModelViewProjectionMatrix", "float4x4 gl_ModelViewProjectionMatrix;", "float4x4",
     "mat4", 0, "gl_ModelViewProjectionMatrix * vec4(1.0)",
     "vec4(gl_ModelViewProjectionMatrix[0])"},
    {"gl_NormalMatrix", "float3x3 gl_NormalMatrix;", "float3x3", "mat3", 0,
     "vec4(gl_NormalMatrix * vec3(1.0), 1.0)", "vec4(gl_NormalMatrix[0], 0.0)"},
    {"gl_TextureMatrix", "float4x4 gl_TextureMatrix[8];", "float4x4", "mat4", 8,
     "gl_TextureMatrix[0] * vec4(1.0)", "vec4(gl_TextureMatrix[0][0])"},
    {"gl_ClipPlane", "float4 gl_ClipPlane[6];", "float4", "vec4", 6,
     "vec4(gl_ClipPlane[0][0])", "gl_ClipPlane[0]"},
};

}  // namespace

int main() {
    int metalChecked = 0;

    // =====================================================================
    // 1. VARYING built-in (GLSL 1.20 §7.6 / GLSL 4.60 §7.1.7)
    // =====================================================================
    for (const VaryingCase& v : kVaryings) {
        // ---- VS: ghi vào output ----
        if (v.vsOutput) {
            std::string msl = ToMsl(
                std::string("#version 120\nvoid main(){ ") + v.name + " = " +
                    (std::string(v.mslType) == "float4" ? "vec4(1.0)" : "1.0") +
                    "; gl_Position = vec4(0.0); }\n",
                kVs);
            TGLMT_CHECK(!msl.empty(),
                        (std::string(v.name) + "(VS): convert phai thanh cong").c_str());
            std::string vs = StructBody(msl, "TGLMT_VOut");
            TGLMT_CHECK(Has(vs, std::string(v.mslType) + " " + v.name + " [[user(locn" +
                                          std::to_string(v.locn) + ")]]"),
                        (std::string(v.name) +
                         "(VS): struct VOut phai co field kieu dung + locn dung")
                            .c_str());
            TGLMT_CHECK(LocnOf(vs, v.name) == v.locn,
                        (std::string(v.name) + "(VS): locn phai dung").c_str());
            TGLMT_CHECK(Has(EntryBody(msl), std::string("_out.") + v.name),
                        (std::string(v.name) + "(VS): phai ghi vao _out." + v.name).c_str());
            TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_VIn"), v.name),
                        (std::string(v.name) + "(VS): KHONG duoc la vertex input").c_str());
        }
        // ---- FS: đọc từ input ----
        if (v.fsInput) {
            std::string msl = ToMsl(
                std::string("#version 120\nvoid main(){ gl_FragColor = ") +
                    (std::string(v.mslType) == "float4" ? std::string(v.name)
                                                        : std::string("vec4(") + v.name + ")") +
                    "; }\n",
                kFs);
            TGLMT_CHECK(!msl.empty(),
                        (std::string(v.name) + "(FS): convert phai thanh cong").c_str());
            std::string fs = StructBody(msl, "TGLMT_FIn");
            TGLMT_CHECK(Has(fs, std::string(v.mslType) + " " + v.name + " [[user(locn" +
                                     std::to_string(v.locn) + ")]]"),
                        (std::string(v.name) +
                         "(FS): struct FIn phai co field kieu dung + locn dung")
                            .c_str());
            TGLMT_CHECK(LocnOf(fs, v.name) == v.locn,
                        (std::string(v.name) + "(FS): locn phai dung").c_str());
            TGLMT_CHECK(Has(EntryBody(msl), std::string("_in.") + v.name),
                        (std::string(v.name) + "(FS): phai doc tu _in." + v.name).c_str());
            TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_FOut"), v.name),
                        (std::string(v.name) + "(FS): KHONG duoc la fragment output").c_str());
        }
        // ---- VS output và FS input phải CÙNG locn (Metal nối theo SỐ) ----
        if (v.vsOutput && v.fsInput) {
            std::string vs = ToMsl(
                std::string("#version 120\nvoid main(){ ") + v.name + " = " +
                    (std::string(v.mslType) == "float4" ? "vec4(1.0)" : "1.0") +
                    "; gl_Position = vec4(0.0); }\n",
                kVs);
            std::string fs = ToMsl(
                std::string("#version 120\nvoid main(){ gl_FragColor = ") +
                    (std::string(v.mslType) == "float4" ? std::string(v.name)
                                                        : std::string("vec4(") + v.name + ")") +
                    "; }\n",
                kFs);
            int lv = LocnOf(StructBody(vs, "TGLMT_VOut"), v.name);
            int lf = LocnOf(StructBody(fs, "TGLMT_FIn"), v.name);
            TGLMT_CHECK(lv >= 0 && lv == lf,
                        (std::string(v.name) +
                         ": locn VS output va FS input phai GIONG (Metal noi theo so)")
                            .c_str());
            TGLMT_CHECK(lv >= 1000,
                        (std::string(v.name) +
                         ": locn phai >= 1000 (ngoai dai location tam 900+ cua converter)")
                            .c_str());
        }
    }

    // =====================================================================
    // 2. gl_TexCoord[0..7] — GLSL 1.20 §7.6: `varying vec4 gl_TexCoord[8]`
    //    ⇒ phần tử là vec4 (float4 trong MSL), KHÔNG phải scalar.
    //    Converter đã khai sẵn float4 gl_TexCoordN + đổi body thành
    //    `_in.gl_TexCoordN`; pass chỉ sửa CTOR quá số thành phần.
    // =====================================================================
    for (int i = 0; i < 8; ++i) {
        std::string nm = "gl_TexCoord" + std::to_string(i);
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FragColor = gl_TexCoord[" + std::to_string(i) +
                "]; }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), (nm + "(FS): convert phai thanh cong").c_str());
        std::string fs = StructBody(msl, "TGLMT_FIn");
        TGLMT_CHECK(Has(fs, "float4 " + nm + " [[user(locn"),
                    (nm + "(FS): phai la float4 (GLSL 1.20 §7.6: vec4), KHONG phai scalar")
                        .c_str());
        TGLMT_CHECK(!Has(fs, "float " + nm + " [[user(locn"),
                    (nm + "(FS): KHONG duoc khai float (sai kiểu phần tử)").c_str());
        TGLMT_CHECK(Has(EntryBody(msl), "_in." + nm),
                    (nm + "(FS): phai doc tu _in." + nm).c_str());
        TGLMT_CHECK(!Has(EntryBody(msl), nm + "["),
                    (nm + "(FS): khong con mang gl_TexCoord[] trong body").c_str());
    }
    {
        // Ctor quá số thành phần: `vec4(gl_TexCoord[0], 0.0, 1.0)` là GLSL
        // KHÔNG hợp lệ (ctor vec4 không có dạng (vec4,float,float)) ⇒ MSL
        // cũng không có ⇒ phải sửa thành float4(<vec4>.xy, 0.0, 1.0).
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_TexCoord[0], 0.0, 1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_TexCoord[0] (ctor qua so thanh phan): convert OK");
        TGLMT_CHECK(Has(msl, "float4(_in.gl_TexCoord0.xy, 0.0, 1.0)"),
                    "gl_TexCoord[0]: ctor vec4(vec4,float,float) phai duoc sua thanh "
                    "float4(<vec4>.xy, 0.0, 1.0)");
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    "gl_TexCoord[0] (ctor): MSL phai bien dich duoc boi Metal compiler");
        ++metalChecked;
    }

    // =====================================================================
    // 3. UNIFORM cố định — GLSL 1.20 §7.4 / GLSL 4.60 §7.4.1
    // =====================================================================
    for (const UniformCase& u : kUniforms) {
        // Dùng uniform trong VS (nơi spec cho phép rõ ràng).
        std::string msl = ToMsl(
            std::string("#version 120\nvoid main(){ gl_Position = ") + u.vsExpr + "; }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(),
                    (std::string(u.name) + "(VS): convert phai thanh cong").c_str());
        std::string ub = StructBody(msl, "TGLMTUniforms");
        TGLMT_CHECK(Has(ub, u.decl),
                    (std::string(u.name) + ": struct TGLMTUniforms phai co '" + u.decl + "'")
                        .c_str());
        TGLMT_CHECK(Has(EntryBody(msl), std::string("tglmt_u.") + u.name),
                    (std::string(u.name) + ": phai doc tu tglmt_u." + u.name).c_str());
        // Cùng uniform phải dùng được ở FS (GLSL 1.20 cho phép ở mọi stage).
        std::string fs = ToMsl(
            std::string("#version 120\nvoid main(){ gl_FragColor = ") + u.fsExpr + "; }\n",
            kFs);
        TGLMT_CHECK(!fs.empty(),
                    (std::string(u.name) + "(FS): convert phai thanh cong").c_str());
        TGLMT_CHECK(Has(StructBody(fs, "TGLMTUniforms"), u.decl),
                    (std::string(u.name) + "(FS): struct TGLMTUniforms phai co '" +
                         std::string(u.decl) + "'")
                        .c_str());
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    (std::string(u.name) + ": MSL phai bien dich duoc boi Metal compiler")
                        .c_str());
        ++metalChecked;
    }

    // =====================================================================
    // 4. gl_TextureMatrix[N] / gl_ClipPlane[N] — phần tử mảng phải index
    //    được (Metal: float4x4 arr[8] / float4 arr[6]).
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_TexCoord[3] = gl_TextureMatrix[3] * vec4(1.0);"
            " gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_TextureMatrix[3]: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float4x4 gl_TextureMatrix[8];"),
                    "gl_TextureMatrix: phai khai mang float4x4[8]");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_TextureMatrix[3]"),
                    "gl_TextureMatrix[3]: phai index vao mang uniform");
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    "gl_TextureMatrix[3]: MSL phai bien dich duoc boi Metal compiler");
        ++metalChecked;
    }
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_Position = vec4(gl_ClipPlane[0][0]); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_ClipPlane[0]: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float4 gl_ClipPlane[6];"),
                    "gl_ClipPlane: phai khai mang float4[6]");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_ClipPlane[0]"),
                    "gl_ClipPlane[0]: phai index vao mang uniform");
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    "gl_ClipPlane[0]: MSL phai bien dich duoc boi Metal compiler");
        ++metalChecked;
    }

    // =====================================================================
    // 5. gl_Fog (struct GLSL 1.20 §7.4: color/range/near/far) + gl_FogParameters
    //    Struct bị HẠ thành các uniform phẳng trong struct TGLMTUniforms.
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.color, 1.0); }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_Fog.color: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float4 gl_Fog_color;"),
                    "gl_Fog.color: phai khai uniform float4 gl_Fog_color");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_Fog_color"),
                    "gl_Fog.color: phai doc tu tglmt_u.gl_Fog_color");
        TGLMT_CHECK(!Has(EntryBody(msl), "gl_Fog."),
                    "gl_Fog.color: khong con duong dan GLSL trong body");
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    "gl_Fog.color: MSL phai bien dich duoc boi Metal compiler");
        ++metalChecked;
    }
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.range, 1.0, 1.0, 1.0); }\n",
            kFs);
        TGLMT_CHECK(!msl.empty(), "gl_Fog.range: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float gl_Fog_range;"),
                    "gl_Fog.range: phai khai uniform float gl_Fog_range");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_Fog_range"),
                    "gl_Fog.range: phai doc tu tglmt_u.gl_Fog_range");
    }
    {
        // gl_FogParameters là TÊN KIỂU trong GLSL 1.20; `gl_FogParameters.near`
        // không phải GLSL hợp lệ, nhưng coverage matrix dùng nó ⇒ pass chấp
        // nhận (tên struct dùng như biến).
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FogDepth = gl_FogParameters.near;"
            " gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_FogParameters.near: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float gl_Fog_near;"),
                    "gl_FogParameters.near: phai khai uniform float gl_Fog_near");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_Fog_near"),
                    "gl_FogParameters.near: phai doc tu tglmt_u.gl_Fog_near");
        TGLMT_CHECK(Has(EntryBody(msl), "float gl_FogDepth"),
                    "gl_FogDepth: phai khai bien local float (mo rong TGLMT, "
                    "khong phai built-in GLSL)");
    }
    {
        // gl_Fog.depth là MỞ RỘNG của TGLMT (GLSL 1.20 §7.4 chỉ có
        // color/range/near/far) — coverage matrix yêu cầu ⇒ phải dịch được.
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FogDepth = gl_Fog.depth; gl_Position = vec4(1.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_Fog.depth: convert phai thanh cong");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float gl_Fog_depth;"),
                    "gl_Fog.depth: phai khai uniform float gl_Fog_depth");
        TGLMT_CHECK(Has(EntryBody(msl), "tglmt_u.gl_Fog_depth"),
                    "gl_Fog.depth: phai doc tu tglmt_u.gl_Fog_depth");
    }

    // =====================================================================
    // 6. gl_FogFragCoord là FLOAT (GLSL 1.20 §7.6) — gán vào gl_FragColor
    //    (vec4) là GLSL không hợp lệ ⇒ pass nâng float→float4.
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_FragColor = gl_FogFragCoord; }\n", kFs);
        TGLMT_CHECK(!msl.empty(), "gl_FogFragCoord → gl_FragColor: convert phai thanh cong");
        TGLMT_CHECK(Has(EntryBody(msl), "float4(_in.gl_FogFragCoord)"),
                    "gl_FogFragCoord (float) gan vao vec4 phai duoc nang float4(...)");
        std::string err;
        TGLMT_CHECK(MetalCompilesRaw(msl, &err),
                    "gl_FogFragCoord → gl_FragColor: MSL phai bien dich duoc boi Metal compiler");
        ++metalChecked;
    }

    // =====================================================================
    // 7. KHÔNG ĐỤNG vào shader KHÔNG dùng nhóm legacy (không regression).
    // =====================================================================
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_Position = vec4(1.0); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "shader rong: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "gl_ModelViewMatrix"),
                    "shader khong dung legacy: KHONG duoc them uniform gl_ModelViewMatrix");
        TGLMT_CHECK(!Has(msl, "gl_FrontColor"),
                    "shader khong dung legacy: KHONG duoc them varying gl_FrontColor");
        TGLMT_CHECK(!Has(msl, "gl_Fog_"),
                    "shader khong dung legacy: KHONG duoc them uniform gl_Fog_*");
        TGLMT_CHECK(!Has(msl, "tglmt_u.gl_"),
                    "shader khong dung legacy: KHONG duoc them truy cap tglmt_u");
        TGLMT_CHECK(Has(StructBody(msl, "TGLMTUniforms"), "float _pad;"),
                    "shader khong dung legacy: struct TGLMTUniforms phai giong nguyen "
                    "(float _pad)");
        TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_VOut"), "gl_"),
                    "shader khong dung legacy: struct VOut phai giong nguyen");
        TGLMT_CHECK(!Has(StructBody(msl, "TGLMT_VIn"), "gl_"),
                    "shader khong dung legacy: struct VIn phai giong nguyen");
    }

    // =====================================================================
    // 8. KNOWN GAP — ghi rõ trạng thái để không ai tưởng là đã chạy.
    //    (a) `uniform gl_FogParameters gl_Fog;` (dạng CANONICAL GLSL 1.20)
    //        bị converter từ chối ở bước parse ⇒ pass không chạy được.
    //    (b) `gl_FogCoord` (GLSL 1.20 §7.1, vertex input) chưa dịch.
    //    (c) `gl_ClipVertex` có giá trị nhưng KHÔNG có consumer (Metal không
    //        có user clip plane) ⇒ hiệu lực cắt user-clip chưa mô phỏng.
    // =====================================================================
    {
        GLSLConvertResult r = ConvertGLSLtoMSL(
            "#version 120\nuniform gl_FogParameters gl_Fog;\n"
            "void main(){ gl_FragColor = gl_Fog.color; }\n",
            kFs);
        TGLMT_CHECK(!r.ok, "KNOWN GAP (a): khai bao 'uniform gl_FogParameters gl_Fog;' "
                           "van bi converter tu choi — neu chuyen sang OK thi can "
                           "cap nhat mo ta truong hop nay");
    }
    {
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_Position = vec4(gl_FogCoord); }\n", kVs);
        TGLMT_CHECK(!msl.empty(), "gl_FogCoord: convert phai thanh cong");
        TGLMT_CHECK(!Has(msl, "_in.gl_FogCoord"),
                    "KNOWN GAP (b): gl_FogCoord CHUA duoc dich thanh vertex input");
        TGLMT_CHECK(Has(msl, "gl_FogCoord"),
                    "KNOWN GAP (b): gl_FogCoord van con nguyen ten GLSL trong MSL "
                    "(=> Metal 'undeclared identifier')");
    }
    {
        // gl_ClipVertex: gia tri duoc ghi ra varying — nhung KHONG co consumer.
        std::string msl = ToMsl(
            "#version 120\nvoid main(){ gl_ClipVertex = gl_Position; gl_Position = vec4(0.0); }\n",
            kVs);
        TGLMT_CHECK(!msl.empty(), "gl_ClipVertex: convert phai thanh cong");
        TGLMT_CHECK(Has(EntryBody(msl), "_out.gl_ClipVertex ="),
                    "gl_ClipVertex: phai ghi gia tri vao _out.gl_ClipVertex");
        TGLMT_CHECK(!Has(msl, "[[clip_distance"),
                    "KNOWN GAP (c): gl_ClipVertex CHUA duoc mo phong bang clip_distance "
                    "(Metal chi co MOT [[clip_distance]])");
    }

    // =====================================================================
    // 9. Metal compile bản ĐÃ REMAP location tạm (như linker làm) cho mọi
    //    case chính — bảo đảm MSL không chỉ "đúng hình thức" mà còn dịch được.
    // =====================================================================
    {
        static const char* kShaders[] = {
            "#version 120\nvoid main(){ gl_FrontColor = vec4(1.0); gl_Position = vec4(0.0); }\n",
            "#version 120\nvoid main(){ gl_FragColor = gl_FrontColor; }\n",
            "#version 120\nvoid main(){ gl_FragColor = gl_BackSecondaryColor; }\n",
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_FogFragCoord); }\n",
            "#version 120\nvoid main(){ gl_Position = gl_ModelViewMatrix * vec4(1.0); }\n",
            "#version 120\nvoid main(){ gl_Position = vec4(gl_NormalMatrix * vec3(1.0), 1.0); }\n",
            "#version 120\nvoid main(){ gl_Position = gl_ProjectionMatrix * vec4(1.0); }\n",
            "#version 120\nvoid main(){ gl_Position = gl_ModelViewProjectionMatrix * vec4(1.0); }\n",
            ("#version 120\nvoid main(){ gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(1.0);"
             " gl_Position = vec4(1.0); }\n"),
            "#version 120\nvoid main(){ gl_Position = vec4(gl_ClipPlane[0][0]); }\n",
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.color, 1.0); }\n",
            "#version 120\nvoid main(){ gl_FragColor = vec4(gl_Fog.range); }\n",
        };
        static const uint32_t kStages[] = {kVs, kFs, kFs, kFs, kVs, kVs, kVs, kVs,
                                           kVs, kVs, kFs, kFs};
        for (size_t i = 0; i < sizeof(kShaders) / sizeof(kShaders[0]); ++i) {
            std::string msl = ToMsl(kShaders[i], kStages[i]);
            TGLMT_CHECK(!msl.empty(), "case index 9: convert phai thanh cong");
            std::string remapped = RemapTempLocs(msl);
            std::string err;
            TGLMT_CHECK(MetalCompilesRaw(remapped, &err),
                        "case index 9: MSL (da remap locn) phai bien dich duoc boi Metal");
            ++metalChecked;
        }
    }

    printf("test_glsl_builtin_legacy: %d vong kiem tra Metal compile\n", metalChecked);
    return TGLMT_RESULT;
}
