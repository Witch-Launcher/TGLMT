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
        // --- P6c: subgroup/ballot → MSL simd ---
        {"subgroupAdd", 0x8B30, FS "c = vec4(subgroupAdd(1.0));}"},
        {"subgroupMax", 0x8B30, FS "c = vec4(subgroupMax(1.0));}"},
        {"subgroupMin", 0x8B30, FS "c = vec4(subgroupMin(1.0));}"},
        {"subgroupAnd", 0x8B30, FS "c = vec4(subgroupAnd(1u));}"},
        {"subgroupOr", 0x8B30, FS "c = vec4(subgroupOr(1u));}"},
        {"subgroupXor", 0x8B30, FS "c = vec4(subgroupXor(1u));}"},
        {"subgroupProduct", 0x8B30, FS "c = vec4(subgroupProduct(1.0));}"},
        {"subgroupBroadcast", 0x8B30, FS "c = vec4(subgroupBroadcast(1.0, 0));}"},
        {"subgroupShuffle", 0x8B30, FS "c = vec4(subgroupShuffle(1.0, 0));}"},
        {"subgroupShuffleXor", 0x8B30, FS "c = vec4(subgroupShuffleXor(1.0, 1u));}"},
        {"subgroupShuffleUp", 0x8B30, FS "c = vec4(subgroupShuffleUp(1.0, 1u));}"},
        {"subgroupShuffleDown", 0x8B30, FS "c = vec4(subgroupShuffleDown(1.0, 1u));}"},
        {"subgroupBroadcastFirst", 0x8B30, FS "c = vec4(subgroupBroadcastFirst(1.0));}"},
        {"subgroupInclusiveAdd", 0x8B30, FS "c = vec4(subgroupInclusiveAdd(1.0));}"},
        {"subgroupExclusiveAdd", 0x8B30, FS "c = vec4(subgroupExclusiveAdd(1.0));}"},
        {"subgroupInclusiveMul", 0x8B30, FS "c = vec4(subgroupInclusiveMul(1.0));}"},
        {"subgroupElect", 0x8B30, FS "c = vec4(subgroupElect() ? 1.0 : 0.0);}"},
        {"subgroupBallot", 0x8B30, FS "uvec4 b = subgroupBallot(true); c = vec4(b);}"},
        {"ballotARB", 0x8B30, FS "uvec4 b = ballotARB(true); c = vec4(b);}"},
        {"subgroupAny", 0x8B30, FS "c = vec4(subgroupAny(true) ? 1.0 : 0.0);}"},
        {"subgroupAll", 0x8B30, FS "c = vec4(subgroupAll(true) ? 1.0 : 0.0);}"},
        {"anyInvocation", 0x8B30, FS "c = vec4(anyInvocation(true) ? 1.0 : 0.0);}"},
        {"allInvocation", 0x8B30, FS "c = vec4(allInvocation(true) ? 1.0 : 0.0);}"},
        {"ballotBitCount", 0x8B30, FS "c = vec4(ballotBitCount(subgroupBallot(true)));}"},
        // --- P6d: gl_* builtins (param stage / struct out / honest-fail) ---
        {"gl_FragDepth", 0x8B30, FS "gl_FragDepth = 0.5; c = vec4(1);}"},
        {"gl_FragDepth_cond", 0x8B30, FS "if (gl_FragCoord.x > 0.0) gl_FragDepth = 0.5; c = vec4(1);}"},
        {"mrt_depth", 0x8B30,
         "#version 460 core\nlayout(location=0) out vec4 a; layout(location=1) out vec4 b;\n"
         "void main(){ gl_FragDepth = 0.5; a = vec4(1); b = vec4(0);}"},
        {"gl_StencilRef", 0x8B30, FS "c = vec4(gl_StencilRef);}"},
        {"gl_SampleID", 0x8B30, FS "c = vec4(gl_SampleID);}"},
        {"gl_SamplePosition", 0x8B30, FS "c = vec4(gl_SamplePosition, 0, 1);}"},
        {"gl_SampleMask", 0x8B30, FS "gl_SampleMask[0] = 1; c = vec4(1);}"},
        {"depth_and_mask", 0x8B30, FS "gl_FragDepth = 0.5; gl_SampleMask[0] = 0xF; c = vec4(1);}"},
        {"gl_BaseInstance", 0x8B31, VS "void main(){ gl_Position = vec4(gl_BaseInstance);}"},
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

    // --- Negative:_builtin ngoài subset phải fail TRUNG THỰG (ok=false) ---
    struct Neg { const char* name; uint32_t stage; const char* glsl; };
    std::vector<Neg> negs = {
        {"gl_HelperInvocation", 0x8B30,
         "#version 460 core\nout vec4 c;\nvoid main(){ c = vec4(gl_HelperInvocation ? 1.0 : 0.0);}"},
        {"gl_DrawID", 0x8B31, "#version 460 core\nvoid main(){ gl_Position = vec4(gl_DrawID);}"},
        {"double_type", 0x8B30,
         "#version 460 core\nout vec4 c;\nvoid main(){ double d = 1.0; c = vec4(d);}"},
        {"gl_SampleMask_N1", 0x8B30,
         "#version 460 core\nout vec4 c;\nvoid main(){ gl_SampleMask[1] = 1; c = vec4(1);}"},
        // Lưu ý: compute/image/atomic/subroutine CHƯA nằm đây vì P6f/P6g sắp
        // hỗ trợ — khi implement xong sẽ chuyển sang positive test.
    };
    for (auto& cs : negs) {
        auto r = ConvertGLSLtoMSL(cs.glsl, cs.stage);
        if (r.ok) {
            // Nói dối nếu convert OK nhưng MSL lại không compile / hoặc OK sai.
            // Riêng compute/image/atomic/subroutine: convert fail là ĐÚNG (chưa hỗ trợ).
            printf("NEG-LEAK %-22s convert trả ok=true (phải fail)\n", cs.name);
            ++fail;
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
