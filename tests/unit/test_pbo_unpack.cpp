// test_pbo_unpack.cpp — PIXEL_UNPACK/PACK_BUFFER là offset (spec §8/§18).
// Bug cũ: TexImage/SubImage coi offset như con trỏ (offset 0 → alloc rỗng,
// nonzero → đọc rác) + báo 0501 oan; ReadPixels ghi nhầm địa chỉ offset.
// Chạy được cả Null (logic CPU) và Apple (kèm GPU).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

static int sFail = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL %s\n", msg); sFail = 1; } } while (0)

int main() {
    for (int backend = 0; backend < 2; ++backend) {
        const char* name = backend ? "apple" : "null";
        tglmt::Context ctx(name);
        tglmt::Context::MakeCurrent(&ctx);
        if (backend && ctx.device->isNull()) {
            printf("skip apple (no GPU)\n");
            continue;
        }
        using namespace tglmt::gl;
        printf("== backend %s ==\n", name);

        // PBO staging buffer: 64B padding + 32x32x4 red.
        const int W = 32, H = 32;
        std::vector<uint8_t> red((size_t)W * H * 4);
        for (int i = 0; i < W * H; ++i) {
            red[i * 4] = 255; red[i * 4 + 1] = 0; red[i * 4 + 2] = 0; red[i * 4 + 3] = 255;
        }
        tglmt::GLuint pbo = 0;
        glGenBuffers(1, &pbo);
        glBindBuffer(0x88EC, pbo); // PIXEL_UNPACK_BUFFER
        std::vector<uint8_t> pboData(64 + red.size(), 0x7E);
        memcpy(pboData.data() + 64, red.data(), red.size());
        glBufferData(0x88EC, (tglmt::GLsizeiptr)pboData.size(), pboData.data(), 0x88E8);
        CHECK(glGetError() == 0, "pbo data");

        // 1) TexImage offset 0 (PBO[0..] = padding 0x7E, nhưng total chỉ cần red? —
        //    offset 0 đọc từ đầu PBO: để đúng, PBO2 riêng chứa red từ byte 0).
        tglmt::GLuint pbo2 = 0;
        glGenBuffers(1, &pbo2);
        glBindBuffer(0x88EC, pbo2);
        glBufferData(0x88EC, (tglmt::GLsizeiptr)red.size(), red.data(), 0x88E8);
        tglmt::GLuint tex = 0;
        glGenTextures(1, &tex);
        glActiveTexture(0x84C0);
        glBindTexture(0x0DE1, tex);
        glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, (void*)0);
        CHECK(glGetError() == 0, "teximage pbo offset0 no error");
        {
            auto it = ctx.textures.find(tex);
            CHECK(it != ctx.textures.end(), "tex exists");
            CHECK(it->second.pixels.size() == red.size(), "shadow size");
            CHECK(it->second.pixels[0] == 255 && it->second.pixels[1] == 0,
                  "shadow red via pbo offset0");
        }

        // 2) TexImage nonzero offset (skip 64B padding).
        glBindBuffer(0x88EC, pbo);
        tglmt::GLuint tex2 = 0;
        glGenTextures(1, &tex2);
        glBindTexture(0x0DE1, tex2);
        glTexImage2D(0x0DE1, 0, 0x8058, W, H, 0, 0x1908, 0x1401, (void*)64);
        CHECK(glGetError() == 0, "teximage pbo offset64 no error");
        {
            auto it = ctx.textures.find(tex2);
            CHECK(it->second.pixels[0] == 255 && it->second.pixels[3] == 255,
                  "shadow red via pbo offset64");
        }

        // 3) TexSubImage qua PBO (offset 0) lên texture đã có.
        std::vector<uint8_t> blue(8 * 8 * 4);
        for (int i = 0; i < 64; ++i) {
            blue[i * 4] = 0; blue[i * 4 + 1] = 0; blue[i * 4 + 2] = 255; blue[i * 4 + 3] = 255;
        }
        glBindBuffer(0x88EC, pbo2);
        glBufferData(0x88EC, (tglmt::GLsizeiptr)blue.size(), blue.data(), 0x88E8);
        glTexSubImage2D(0x0DE1, 0, 0, 0, 8, 8, 0x1908, 0x1401, (void*)0);
        CHECK(glGetError() == 0, "texsubimage pbo offset0 no error");
        {
            auto it = ctx.textures.find(tex2);
            CHECK(it->second.pixels[2] == 255, "shadow blue corner via staged subimage");
        }
        ctx.FlushTextureStaging(tex2); // phải flush được, không crash

        // 4) Unpack OOB → 0501 (không đọc lố PBO).
        glTexSubImage2D(0x0DE1, 0, 0, 0, 8, 8, 0x1908, 0x1401, (void*)99999);
        CHECK(glGetError() == 0x0501, "texsubimage pbo oob gives 0501");

        // 5) PACK ReadPixels vào PBO (default FB clear color trên Null).
        glBindBuffer(0x88EC, 0); // unbind UNPACK
        tglmt::GLuint pack = 0;
        glGenBuffers(1, &pack);
        glBindBuffer(0x88EB, pack); // PIXEL_PACK_BUFFER
        glBufferData(0x88EB, 64, nullptr, 0x88E8);
        glClearColor(0, 1, 0, 1);
        glReadPixels(0, 0, 2, 2, 0x1908, 0x1401, (void*)0);
        CHECK(glGetError() == 0, "readpixels pack offset0 no error");
        {
            auto it = ctx.buffers.find(pack);
            CHECK(it->second.data.size() >= 16, "pbo grown");
            CHECK(it->second.data[1] == 255, "pbo green pixel");
        }
        glBindBuffer(0x88EB, 0);
        glBindBuffer(0x88EC, 0);
    }
    if (!sFail) printf("test_pbo_unpack PASS\n");
    return sFail;
}
