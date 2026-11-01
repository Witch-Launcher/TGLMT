// test_shader_compute.cpp — validate P6f: compute shader → kernel MSL + SSBO.
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
#define CS(x) "#version 460 core\n" x
} // namespace

int main() {
    auto dev = MakeDev();
    const uint32_t CMP = 0x91B9, FS = 0x8B30;
    struct Case { const char* name; uint32_t stage; const char* glsl; bool expectOk; };
    std::vector<Case> cases = {
        // --- compute core: localSize parse + kernel + gl_*ID builtins ---
        {"empty", CMP, CS("layout(local_size_x=1) in;\nvoid main(){ }"), true},
        {"gid_store", CMP,
         CS("layout(local_size_x=8, local_size_y=8) in;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=1.0; }"), true},
        {"all_ids", CMP,
         CS("layout(local_size_x=16) in;\n"
            "layout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x+gl_LocalInvocationID.x+gl_WorkGroupID.x]"
            "=float(gl_LocalInvocationIndex)+float(gl_NumWorkGroups.x)+gl_WorkGroupSize.x; }"), true},
        {"uniform", CMP, CS("layout(local_size_x=4) in;\nuniform float f;\nvoid main(){ }"), true},
        {"z_size", CMP, CS("layout(local_size_x=2, local_size_y=4, local_size_z=4) in;\nvoid main(){ }"), true},
        {"sampler", CMP,
         CS("layout(local_size_x=8) in;\nuniform sampler2D t;\n"
            "void main(){ float x=texelFetch(t,ivec2(gl_GlobalInvocationID.xy),0).r; }"), true},
        // --- SSBO: unsized array → device T*; sized/multi → device struct ---
        {"ssbo_unsized", CMP,
         CS("layout(local_size_x=8) in;\nlayout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=1.0; }"), true},
        {"ssbo_vec4_unsized", CMP,
         CS("layout(local_size_x=8) in;\nlayout(std430,binding=0) buffer B{vec4 v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=vec4(1); }"), true},
        {"ssbo_sized", CMP,
         CS("layout(local_size_x=8) in;\nlayout(std430,binding=0) buffer B{float v[64];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=1.0; }"), true},
        {"ssbo_struct", CMP,
         CS("layout(local_size_x=8) in;\nlayout(std430,binding=0) buffer B{float a; vec4 b;};\n"
            "void main(){ }"), true},
        {"ssbo_with_uniform", CMP,
         CS("layout(local_size_x=8) in;\nuniform float f;\nlayout(std430,binding=0) buffer B{float v[];};\n"
            "void main(){ v[0]=f; }"), true},
        // --- negative: SSBO unsized có instance → fail trung thực ---
        {"ssbo_instance_unsized_neg", CMP,
         CS("layout(std430) buffer B{float v[];} buf;\nvoid main(){ }"), false},
        // P4: graphics SSBO giờ ĐƯỢC hỗ trợ (slot 17+k chung UBO) → convert OK.
        {"ssbo_graphics_ok", FS,
         CS("out vec4 c;\nlayout(std430) buffer B{float v[];};\nvoid main(){ c=vec4(1);}"), true},
        // --- P6g image: layout(format) + imageLoad/Store/Size → MSL read_write ---
        {"img_store", CMP,
         CS("layout(local_size_x=8) in;\nlayout(rgba8, binding=0) uniform image2D img;\n"
            "void main(){ imageStore(img, ivec2(0), vec4(1)); }"), true},
        {"img_load", CMP,
         CS("layout(local_size_x=8) in;\nlayout(rgba8, binding=0) uniform readonly image2D img;\n"
            "layout(std430,binding=1) buffer B{float v[];};\n"
            "void main(){ v[0]=imageLoad(img, ivec2(0)).r; }"), true},
        {"img_frag", FS,
         CS("layout(rgba8, binding=0) uniform image2D img;\nout vec4 c;\n"
            "void main(){ c=imageLoad(img, ivec2(gl_FragCoord.xy)); }"), true},
        {"img_size", FS,
         CS("layout(rgba8, binding=0) uniform image2D img;\nout vec4 c;\n"
            "void main(){ ivec2 s=imageSize(img); c=vec4(float(s.x), float(s.y), 0, 1); }"), true},
        {"img_iimage", CMP,
         CS("layout(local_size_x=8) in;\nlayout(r32i, binding=0) uniform writeonly iimage2D img;\n"
            "void main(){ imageStore(img, ivec2(0), ivec4(1)); }"), true},
        {"img_array", CMP,
         CS("layout(local_size_x=8) in;\nlayout(rgba8, binding=0) uniform image2DArray img;\n"
            "void main(){ imageStore(img, ivec3(0,0,0), vec4(1)); }"), true},
        {"img_3d", CMP,
         CS("layout(local_size_x=8) in;\nlayout(rgba8, binding=0) uniform image3D img;\n"
            "void main(){ imageStore(img, ivec3(0), vec4(1)); }"), true},
        {"img_atomic_neg", CMP,
         CS("layout(local_size_x=8) in;\nlayout(r32ui, binding=0) uniform uimage2D img;\n"
            "void main(){ imageAtomicAdd(img, ivec2(0), 1u); }"), false},
        {"img_cube_neg", FS,
         CS("layout(rgba8, binding=0) uniform imageCube img;\nout vec4 c;\nvoid main(){ c=vec4(1); }"), false},
        // --- P9 atomic counters: atomic_uint + Increment/Decrement/Get → MSL ---
        {"atomic_inc", CMP,
         CS("layout(local_size_x=8) in;\n"
            "layout(binding=0, offset=0) uniform atomic_uint ac;\n"
            "layout(std430,binding=0) buffer B{uint v[];};\n"
            "void main(){ v[gl_GlobalInvocationID.x]=atomicCounterIncrement(ac); }"), true},
        {"atomic_dec_get", CMP,
         CS("layout(local_size_x=8) in;\n"
            "layout(binding=0, offset=0) uniform atomic_uint ac;\n"
            "layout(std430,binding=0) buffer B{uint v[];};\n"
            "void main(){ atomicCounterDecrement(ac); v[0]=atomicCounterGet(ac); }"), true},
        {"atomic_nolayout_neg", CMP,
         CS("layout(local_size_x=8) in;\nuniform atomic_uint ac;\nvoid main(){ }"), false},
        // --- Voxy regressions (crash 2026-10-31: scatter.comp COMPUTE) ---
        // function-like macro + local_size_x macro (traversal/prefixsum).
        {"macro_local_size", CMP,
         CS("#define WORK_SIZE 64\n#define STEP(i) if ((I&(1u<<i))!=0) s[I] += s[I-1];\n"
            "layout(local_size_x=WORK_SIZE) in;\n"
            "shared uint s[64];\n"
            "uint scan(uint v){ uint I=gl_LocalInvocationID.x; s[I]=v; barrier();"
            " STEP(0) barrier(); STEP(1) return s[I]-v; }\n"
            "layout(std430,binding=0) buffer B{ uint v[]; };\n"
            "void main(){ uint gid=gl_LocalInvocationID.x; v[gid]=scan(v[gid]); }"), true},
        // SSBO fixed member + trailing runtime array (Voxy traversal queues):
        // fixed part giữ trong struct, array truy cập qua helper reinterpret_cast.
        {"ssbo_flex_fixed", CMP,
         CS("layout(local_size_x=1) in;\n"
            "layout(std430,binding=0) buffer Q { uint head; uint data[]; };\n"
            "layout(std430,binding=1) buffer O { uint outv[]; };\n"
            "void push(uint v){ uint i=atomicAdd(head,1); data[i]=v; }\n"
            "void main(){ push(gl_GlobalInvocationID.x); outv[0]=data[0]; }"), true},
        // user struct làm member trong block + flat SSBO user struct.
        {"ssbo_user_struct", CMP,
         CS("layout(local_size_x=1) in;\n"
            "struct Node { uvec4 pos; uint flags; };\n"
            "struct Cmd { uint count; uint base; };\n"
            "layout(std430,binding=0) readonly buffer S { Node nodes[]; };\n"
            "layout(std430,binding=1) buffer C { uint n; Cmd cmd; };\n"
            "void main(){ n=nodes[0].flags; cmd.base=nodes[1].pos.x; }"), true},
        // layout(binding) trên sampler + early_fragment_tests (Voxy hiz/quads).
        {"sampler_layout_binding", CMP,
         CS("layout(local_size_x=1) in;\n"
            "layout(binding=0) uniform sampler2D tex;\n"
            "layout(std430,binding=0) buffer B{ vec4 v[]; };\n"
            "void main(){ v[0]=texelFetch(tex, ivec2(0), 0); }"), true},
        {"early_fragment_tests", FS,
         CS("layout(early_fragment_tests) in;\nout vec4 c;\n"
            "void main(){ c=vec4(1); }"), true},
        // macro function-like nhiều dòng (SIMPLE_QUEUE) chỉ cần parse/định nghĩa.
        {"multiline_macro_define", CMP,
         CS("#define Q(type, name, idx) layout(binding = idx, std430) restrict buffer name##Struct { \\\n"
            "    type name##Index; \\\n"
            "    type##[] name; \\\n"
            "};\n"
            "layout(local_size_x=1) in;\n"
            "void main(){ }"), true},
        // Voxy patterns: threadgroup atomic, subgroup, bitCount, uintN init từ
        // intN, mat3x4(mat4), textureQueryLevels trong helper, mix bool lồng.
        {"voxy_atomic_subgroup_mix", CMP,
         CS("layout(local_size_x=8) in;\n"
            "shared uint s[64];\n"
            "uniform sampler2D tex;\n"
            "uniform mat4 m;\n"
            "layout(std430,binding=0) buffer B{ uint v[]; };\n"
            "uint levels(sampler2D t){ return textureQueryLevels(t); }\n"
            "void main(){\n"
            "  uint a=atomicAdd(s[gl_LocalInvocationID.x],1u);\n"
            "  uint b=subgroupMax(a)+bitCount(a);\n"
            "  uint3 u=abs(ivec3(int(a),2,3));\n"
            "  mat3x4 m34=mat3x4(m);\n"
            "  bool3 bb=lessThan(vec3(u),vec3(4.0));\n"
            "  bool3 bb2=lessThan(vec3(u),vec3(5.0));\n"
            "  vec3 mx=mix(mix(vec3(0.0),vec3(1.0),bb),vec3(2.0),bb2);\n"
            "  v[gl_GlobalInvocationID.x]=b+u.x+uint(m34[0][0])+uint(mx.x)+levels(tex);\n"
            "}"), true},
        // `vec2[](...)[idx]` — MSL không index prvalue mảng C++.
        {"array_ctor_index", FS,
         CS("out vec4 c;\nvoid main(){ vec2 v=vec2[](vec2(0.0),vec2(1.0))[int(gl_FragCoord.x)&1];"
            " c=vec4(v,0.0,1.0); }"), true},
    };

    int fail = 0;
    for (auto& cs : cases) {
        auto r = ConvertGLSLtoMSL(cs.glsl, cs.stage);
        if (!r.ok) {
            if (cs.expectOk) { printf("CONV-FAIL %-24s %s\n", cs.name, r.log.c_str()); ++fail; }
            continue;
        }
        if (!cs.expectOk) {
            printf("CONV-UNEXPECTED-OK %-24s (đáng lẽ fail)\n", cs.name);
            ++fail;
            continue;
        }
        // compute: localSize phải khớp layout (>=1)
        if (cs.stage == CMP && r.localSize[0] < 1) {
            printf("LS-FAIL %-24s localSize=0\n", cs.name); ++fail; continue;
        }
        if (dev) {
            std::string err;
            auto lib = dev->compileLibrary(r.msl, err);
            if (!lib) {
                printf("MSL-FAIL  %-24s %s\n", cs.name, err.c_str());
                printf("  --- MSL ---\n%.500s\n", r.msl.c_str());
                ++fail;
            }
        }
    }

    if (fail == 0) {
        printf("test_shader_compute: %zu cases OK\n", cases.size());
        return 0;
    }
    printf("test_shader_compute: %d FAIL\n", fail);
    return 1;
}
