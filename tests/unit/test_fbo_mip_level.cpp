// test_fbo_mip_level.cpp — Hồi quy bug "đen/mất texture sau tắt-đi-mở-lại"
// (resource reload đổi video setting) trên Minecraft 26.1.2 + iOS.
//
// Căn cứ client.jar 26.1.2 (GlDevice.createTexture alloc MỌI mip level,
// TextureAtlas bake animation per-mip qua GlTextureView(texture, level, 1),
// GlTextureView.getFbo bind FBO với baseMipLevel):
//  1. FBO attachment PHẢI nhớ mip level (glFramebufferTexture2D level param).
//     Bỏ qua level làm mọi mip bake alias level 0 → smear atlas.
//  2. glDeleteTextures PHẢI detach FBO (spec §9): id tái sử dụng cho atlas mới
//     sau reload mà FBO cũ còn trỏ → bake cũ đè atlas mới.
//  3. TexImage/SubImage level>0 PHẢI lưu shadow + GetTexImage(level>0) đọc lại
//     được (MipmapGenerator upload per-mip). Drop câm → readback/copy mip đen.
//  4. TexImage level 0 realloc PHẢI xóa mipData/wasRT cũ (storage mới).
//
// Chạy được trên backend null (không cần GPU).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"

#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) {
        printf("test_fbo_mip_level FAIL: %s\n", msg);
        ++gFails;
    }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    // --- 1. FBO nhớ level + size theo mip ---
    GLuint tex = 0, fbo = 0;
    glGenTextures(1, &tex);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, tex);
    std::vector<unsigned char> white(64 * 64 * 4, 255);
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, white.data());
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(0x8CA9, fbo);
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, tex, 2);
    Check(glGetError() == 0, "attach level 2 no error");
    {
        auto it = ctx.fbos.find(fbo);
        Check(it != ctx.fbos.end(), "fbo exists");
        if (it != ctx.fbos.end()) {
            Check(it->second.colorTex[0] == tex, "color attach tex");
            auto lit = it->second.colorLevel.find(0);
            Check(lit != it->second.colorLevel.end() && lit->second == 2, "color attach level==2");
            Check(it->second.w == 16 && it->second.h == 16, "fbo size 64>>2==16");
        }
    }
    // level âm → INVALID_VALUE
    glFramebufferTexture2D(0x8CA9, 0x8CE0, 0x0DE1, tex, -1);
    Check(glGetError() == 0x0501, "negative level INVALID_VALUE");
    {
        auto it = ctx.fbos.find(fbo);
        if (it != ctx.fbos.end()) {
            auto lit = it->second.colorLevel.find(0);
            Check(lit != it->second.colorLevel.end() && lit->second == 2, "level unchanged after bad attach");
        }
    }
    // Named variant
    GLuint fbo2 = 0;
    glCreateFramebuffers(1, &fbo2);
    glNamedFramebufferTexture(fbo2, 0x8CE0, tex, 1);
    {
        auto it = ctx.fbos.find(fbo2);
        Check(it != ctx.fbos.end(), "named fbo exists");
        if (it != ctx.fbos.end()) {
            auto lit = it->second.colorLevel.find(0);
            Check(lit != it->second.colorLevel.end() && lit->second == 1, "named attach level==1");
            Check(it->second.w == 32 && it->second.h == 32, "named fbo size 64>>1==32");
        }
    }

    // --- 2. Delete texture detach FBO ---
    glDeleteTextures(1, &tex);
    {
        auto it = ctx.fbos.find(fbo);
        Check(it != ctx.fbos.end(), "fbo survives tex delete");
        if (it != ctx.fbos.end()) {
            Check(it->second.colorTex.find(0) == it->second.colorTex.end(), "color detached on delete");
            Check(it->second.colorLevel.find(0) == it->second.colorLevel.end(), "level detached on delete");
        }
        auto it2 = ctx.fbos.find(fbo2);
        if (it2 != ctx.fbos.end())
            Check(it2->second.colorTex.find(0) == it2->second.colorTex.end(), "named fbo detached on delete");
    }
    // ID tái sử dụng cho atlas mới: FBO cũ KHÔNG được trỏ sang texture mới.
    GLuint texNew = 0;
    glGenTextures(1, &texNew);
    glActiveTexture(0x84C0);
    glBindTexture(0x0DE1, texNew);
    std::vector<unsigned char> red(64 * 64 * 4, 0);
    for (size_t i = 0; i < 64 * 64; ++i) { red[i * 4] = 255; red[i * 4 + 3] = 255; }
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, red.data());
    {
        auto it = ctx.fbos.find(fbo);
        if (it != ctx.fbos.end())
            Check(it->second.colorTex.find(0) == it->second.colorTex.end(), "old fbo not aliased to reused id");
    }

    // --- 3. Mip shadow roundtrip (pattern GlDevice.createTexture + MipmapGenerator) ---
    GLuint atlas = 0;
    glGenTextures(1, &atlas);
    glBindTexture(0x0DE1, atlas);
    // alloc mọi mip NULL như GlDevice.createTexture (4 levels cho 64px)
    glTexImage2D(0x0DE1, 0, 0x8058, 64, 64, 0, 0x1908, 0x1401, nullptr);
    glTexImage2D(0x0DE1, 1, 0x8058, 32, 32, 0, 0x1908, 0x1401, nullptr);
    glTexImage2D(0x0DE1, 2, 0x8058, 16, 16, 0, 0x1908, 0x1401, nullptr);
    glTexImage2D(0x0DE1, 3, 0x8058, 8, 8, 0, 0x1908, 0x1401, nullptr);
    Check(glGetError() == 0, "alloc mips no error");
    {
        auto it = ctx.textures.find(atlas);
        Check(it != ctx.textures.end(), "atlas exists");
        if (it != ctx.textures.end()) {
            Check(it->second.levels == 4, "levels==4");
            Check(it->second.w == 64 && it->second.h == 64, "base intact (not overwritten by small mip)");
            Check(it->second.mipData.count(1) && it->second.mipData.count(2) && it->second.mipData.count(3),
                  "mip shadows allocated");
        }
    }
    // upload base + mip 1 (pattern writeToTexture per-mip)
    std::vector<unsigned char> base64(64 * 64 * 4, 0x11);
    glTexSubImage2D(0x0DE1, 0, 0, 0, 64, 64, 0x1908, 0x1401, base64.data());
    std::vector<unsigned char> mip1(32 * 32 * 4, 0x22);
    glTexSubImage2D(0x0DE1, 1, 0, 0, 32, 32, 0x1908, 0x1401, mip1.data());
    Check(glGetError() == 0, "sub mips no error");
    // out-of-bounds mip sub → INVALID_VALUE, không corrupt
    glTexSubImage2D(0x0DE1, 1, 16, 16, 32, 32, 0x1908, 0x1401, mip1.data());
    Check(glGetError() == 0x0501, "mip sub OOB INVALID_VALUE");
    // level chưa alloc → INVALID_VALUE
    glTexSubImage2D(0x0DE1, 5, 0, 0, 2, 2, 0x1908, 0x1401, mip1.data());
    Check(glGetError() == 0x0501, "mip sub no-alloc INVALID_VALUE");
    // readback
    {
        std::vector<unsigned char> out(32 * 32 * 4, 0);
        glGetTexImage(0x0DE1, 1, 0x1908, 0x1401, out.data());
        Check(glGetError() == 0, "get mip1 no error");
        Check(out[0] == 0x22 && out[4 * 100] == 0x22, "mip1 content roundtrip");
        std::vector<unsigned char> out0(64 * 64 * 4, 0);
        glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, out0.data());
        Check(out0[0] == 0x11, "base content intact after mip uploads");
    }
    // GetLevelParameter theo level
    {
        GLint w = -1, h = -1;
        glGetTexLevelParameteriv(0x0DE1, 2, 0x1000, &w);
        glGetTexLevelParameteriv(0x0DE1, 2, 0x1001, &h);
        Check(w == 16 && h == 16, "level parameter mip dims");
        glGetTexLevelParameteriv(0x0DE1, 0, 0x1000, &w);
        Check(w == 64, "level parameter base dims");
    }

    // --- 4. Realloc level 0 xóa mip/wasRT cũ ---
    {
        auto it = ctx.textures.find(atlas);
        if (it != ctx.textures.end()) it->second.wasRT = true; // giả lập đời bake trước
    }
    glTexImage2D(0x0DE1, 0, 0x8058, 128, 128, 0, 0x1908, 0x1401, nullptr);
    {
        auto it = ctx.textures.find(atlas);
        Check(it != ctx.textures.end(), "atlas still exists");
        if (it != ctx.textures.end()) {
            Check(it->second.mipData.empty(), "realloc clears stale mip shadows");
            Check(!it->second.wasRT, "realloc clears stale wasRT");
            Check(it->second.w == 128 && it->second.h == 128, "realloc new size");
        }
    }

    if (gFails) return 1;
    printf("test_fbo_mip_level PASS\n");
    return 0;
}
