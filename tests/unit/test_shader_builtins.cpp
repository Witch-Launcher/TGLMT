// test_shader_builtins.cpp — validate GLSL→MSL cho các builtin vừa thêm
// (P6a texture offset/size/gather + P6b bitcast/pack/bitfield/find).
// Trên backend Metal thật: Convert OK + MSL compile thật bằng Metal.
// Trên backend Null (device isNull): chỉ check Convert OK (bỏ qua MSL compile).
#include "tglmt/GLSLConverter.h"
#include "tglmt/MetalInterface.h"
#include <cstdio>
#include <string>
#include <vector>

using namespace tglmt;

namespace {
std::shared_ptr<metal::IDevice> MakeDev() {
    auto d = metal::CreateDevice("apple", nullptr);
    if (!d || d->isNull()) return nullptr;
    return d;
}
struct Case {
    const char* name;
    uint32_t stage;
    const char* glsl;
};
} // namespace

int main() {
    auto dev = MakeDev();
    // FS = fragment preamble có sampler2D t + out vec4 c (macro để nối literal)
#define FS "#version 460 core\nuniform sampler2D t; out vec4 c;\nvoid main(){ "
#define VS "#version 460 core\n"

    std::vector<Case> cases = {
        // --- P6a: texture builtin có offset/size/gather ---
        {"textureSize", 0x8B30, FS "ivec2 s = textureSize(t, 0); c = vec4(float(s.x), float(s.y), 0, 1);}"},
        {"textureSize_arr",
         0x8B30,
         "#version 460 core\nuniform sampler2DArray t; out vec4 c;\nvoid main(){ ivec3 s = textureSize(t, 0); c = vec4(float(s.x), float(s.y), float(s.z), 1);}"},
        {"textureGather", 0x8B30, FS "c = textureGather(t, vec2(0.5));}"},
        {"textureGatherComp", 0x8B30, FS "c = textureGather(t, vec2(0.5), 1);}"},
        {"textureOffset", 0x8B30, FS "c = textureOffset(t, vec2(0.5), ivec2(1));}"},
        {"textureOffsetBias", 0x8B30, FS "c = textureOffset(t, vec2(0.5), ivec2(1), 1.0);}"},
        {"texelFetchOffset", 0x8B30, FS "c = texelFetchOffset(t, ivec2(0), 0, ivec2(1));}"},
        {"textureLodOffset", 0x8B30, FS "c = textureLodOffset(t, vec2(0.5), 1.0, ivec2(1));}"},
        {"textureGradOffset", 0x8B30,
         FS "c = textureGradOffset(t, vec2(0.5), vec2(1), vec2(1), ivec2(1));}"},
        {"textureGatherOffset", 0x8B30, FS "c = textureGatherOffset(t, vec2(0.5), ivec2(1));}"},
        {"textureProjLod", 0x8B30, FS "c = textureProjLod(t, vec4(0.5, 0.5, 1, 1), 1.0);}"},
        {"textureProjLod_vec3", 0x8B30, FS "c = textureProjLod(t, vec3(0.5, 0.5, 1), 1.0);}"},
        {"textureProj_vec3", 0x8B30, FS "c = textureProj(t, vec3(0.5, 0.5, 1));}"},
        {"textureProj_vec4", 0x8B30, FS "c = textureProj(t, vec4(0.5, 0.5, 1, 1));}"},
        // --- P6b: bitcast → as_type ---
        {"floatBitsToInt", 0x8B30, FS "c = vec4(floatBitsToInt(1.0));}"},
        {"floatBitsToUint", 0x8B30, FS "c = vec4(floatBitsToUint(1.0));}"},
        {"intBitsToFloat", 0x8B30, FS "c = vec4(intBitsToFloat(1));}"},
        {"uintBitsToFloat", 0x8B30, FS "c = vec4(uintBitsToFloat(1u));}"},
        // --- P6b: pack/unpack ---
        {"packUnorm2x16", 0x8B30, FS "c = vec4(packUnorm2x16(vec2(0.5)));}"},
        {"packSnorm2x16", 0x8B30, FS "c = vec4(packSnorm2x16(vec2(0.5)));}"},
        {"unpackUnorm2x16", 0x8B30, FS "c = vec4(unpackUnorm2x16(1u), 0, 1);}"},
        {"unpackSnorm2x16", 0x8B30, FS "c = vec4(unpackSnorm2x16(1u), 0, 1);}"},
        {"packUnorm4x8", 0x8B30, FS "c = vec4(packUnorm4x8(vec4(0.5)));}"},
        {"unpackUnorm4x8", 0x8B30, FS "c = unpackUnorm4x8(1u);}"},
        {"packHalf2x16", 0x8B30, FS "c = vec4(packHalf2x16(vec2(0.5)));}"},
        {"unpackHalf2x16", 0x8B30, FS "c = vec4(unpackHalf2x16(1u), 0, 1);}"},
        // --- P6b: bitfield ---
        {"bitfieldExtract", 0x8B30, FS "c = vec4(bitfieldExtract(5, 1, 3));}"},
        {"bitfieldInsert", 0x8B30, FS "c = vec4(bitfieldInsert(1, 2, 2, 1));}"},
        {"bitfieldReverse", 0x8B30, FS "c = vec4(bitfieldReverse(5));}"},
        // --- P6b: findLSB/MSB ---
        {"findLSB_int", 0x8B30, FS "c = vec4(findLSB(4));}"},
        {"findLSB_uint", 0x8B30, FS "c = vec4(findLSB(uint(4)));}"},
        {"findMSB_int", 0x8B30, FS "c = vec4(findMSB(4));}"},
        {"findMSB_neg", 0x8B30, FS "c = vec4(findMSB(-4));}"},
        // --- regression: các builtin cũ vẫn ổn ---
        {"texture_basic", 0x8B30, FS "c = texture(t, vec2(0.5));}"},
        {"texelFetch", 0x8B30, FS "c = texelFetch(t, ivec2(0), 0);}"},
        {"textureLod", 0x8B30, FS "c = textureLod(t, vec2(0.5), 1.0);}"},
        {"textureGrad", 0x8B30, FS "c = textureGrad(t, vec2(0), vec2(1), vec2(1));}"},
        {"textureBias", 0x8B30, FS "c = texture(t, vec2(0.5), 1.0);}"},
        {"basic_vs", 0x8B31, VS "layout(location = 0) in vec3 p;\nvoid main(){ gl_Position = vec4(p, 1);}"},
    };

    int fail = 0;
    for (auto& cs : cases) {
        auto r = ConvertGLSLtoMSL(cs.glsl, cs.stage);
        if (!r.ok) {
            printf("CONV-FAIL %-22s %s\n", cs.name, r.log.c_str());
            ++fail;
            continue;
        }
        if (dev) {
            std::string err;
            auto lib = dev->compileLibrary(r.msl, err);
            if (!lib) {
                printf("MSL-FAIL  %-22s %s\n", cs.name, err.c_str());
                printf("  --- MSL head ---\n%.400s\n", r.msl.c_str());
                ++fail;
                continue;
            }
        }
    }
    if (fail) {
        printf("test_shader_builtins FAIL (%d/%zu)\n", fail, cases.size());
        return 1;
    }
    printf("test_shader_builtins PASS (%zu cases, metal=%s)\n", cases.size(),
           dev ? "yes" : "null-skip");
    return 0;
}
