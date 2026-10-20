// test_shadow_sampler.cpp — sampler2DShadow phải giữ compare trong MSL:
//   texture(sampler2DShadow, vec3) → depth2d<float> + sample_compare(smp, uv, z)
// Iris đổi shadow2D(shadowtex1, p).r thành vec4(texture(shadowtex1, p)).r —
// nếu converter map sampler2DShadow → texture2d<float> + .sample() thì compare
// mất, pack đọc raw depth (~0.99) thay vì 0/1 → MakeUp/CR mất bóng.
#include "tglmt/GLSLConverter.h"
#include <cstdio>
#include <string>

using namespace tglmt;

static int gFails = 0;
#define CHECK(c, m) do { if (!(c)) { printf("FAIL: %s\n", m); ++gFails; } } while (0)
static bool Has(const std::string& s, const char* sub) {
    return s.find(sub) != std::string::npos;
}
static int Count(const std::string& s, const char* sub) {
    int n = 0; size_t p = 0;
    while ((p = s.find(sub, p)) != std::string::npos) { ++n; p += 1; }
    return n;
}

int main() {
    const char* fs =
        "#version 330 compatibility\n"
        "uniform sampler2DShadow shadowtex1;\n"
        "uniform sampler2D shadowtex0;\n"
        "layout(location = 0) out vec4 c;\n"
        "void main() {\n"
        "    vec3 p = vec3(0.5, 0.25, 0.75);\n"
        "    vec4 a = vec4(texture(shadowtex1, p));\n"
        "    float b = textureLod(shadowtex1, p, 0.0);\n"
        "    vec4 d = texture(shadowtex0, p.xy);\n"
        "    c = a + vec4(b) + d;\n"
        "}\n";
    GLSLConvertResult r = ConvertGLSLtoMSL(fs, 0x8B30);
    if (!r.ok) printf("[conv log] %s\n", r.log.c_str());
    CHECK(r.ok, "convert ok");
    if (!r.ok) return 1;
    CHECK(Has(r.msl, "depth2d<float> shadowtex1_tex"), "shadowtex1 = depth2d");
    CHECK(Has(r.msl, "shadowtex1_tex.sample_compare(shadowtex1_smp"),
          "texture(shadow2D) = sample_compare");
    CHECK(Has(r.msl, ").z"), "compare ref lấy từ .z của vec3");
    // textureLod trên shadow cũng phải compare + level.
    CHECK(Has(r.msl, "sample_compare(shadowtex1_smp") &&
          Has(r.msl, "level("), "textureLod(shadow2D) = sample_compare + level");
    // sampler2D thường KHÔNG được đổi sang depth2d/sample_compare.
    CHECK(Has(r.msl, "texture2d<float> shadowtex0_tex"), "shadowtex0 = texture2d");
    CHECK(Has(r.msl, "shadowtex0_tex.sample(shadowtex0_smp"), "sampler2D = sample");
    CHECK(Count(r.msl, "shadowtex0_tex.sample_compare") == 0,
          "sampler2D không bị sample_compare");
    if (gFails) { printf("test_shadow_sampler FAIL (%d)\n", gFails); return 1; }
    printf("test_shadow_sampler PASS\n");
    return 0;
}
