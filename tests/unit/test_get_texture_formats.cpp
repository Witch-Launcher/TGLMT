// test_get_texture_formats.cpp — glGetTexImage/glGetnTexImage chuyển đổi nhiều
// format/type + PACK_ROW_LENGTH/SKIP, không chỉ RGBA/UBYTE.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>
#include <cstring>
#include <vector>

using namespace tglmt;
using namespace tglmt::gl;

static int fails = 0;
static void Check(bool ok, const char* what) {
    if (!ok) { printf("FAIL: %s\n", what); ++fails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(0x0DE1, t);
    const unsigned char px[2 * 2 * 4] = {
        10, 20, 30, 40,   50, 60, 70, 80,
        90, 100, 110, 120, 130, 140, 150, 160,
    };
    glTexImage2D(0x0DE1, 0, 0x8058, 2, 2, 0, 0x1908, 0x1401, px);
    glPixelStorei(0x0D05 /*PACK_ALIGNMENT*/, 1);
    Check(glGetError() == 0, "setup");

    unsigned char rgba[16] = {0};
    glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, rgba);
    Check(glGetError() == 0, "RGBA/UBYTE no error");
    Check(memcmp(rgba, px, sizeof(px)) == 0, "RGBA/UBYTE giữ nguyên");

    unsigned char rgb[12] = {0};
    glGetTexImage(0x0DE1, 0, 0x1907, 0x1401, rgb);
    Check(glGetError() == 0, "RGB/UBYTE no error");
    Check(rgb[0] == 10 && rgb[1] == 20 && rgb[2] == 30 &&
          rgb[9] == 130 && rgb[10] == 140 && rgb[11] == 150, "RGB/UBYTE bỏ alpha");

    unsigned char red[4] = {0};
    glGetTexImage(0x0DE1, 0, 0x1903, 0x1401, red);
    Check(glGetError() == 0, "RED/UBYTE no error");
    Check(red[0] == 10 && red[1] == 50 && red[2] == 90 && red[3] == 130,
          "RED/UBYTE lấy đúng kênh R");

    unsigned short rg[8] = {0};
    glGetTexImage(0x0DE1, 0, 0x8227, 0x1403, rg);
    Check(glGetError() == 0, "RG/USHORT no error");
    Check(rg[0] == 10 * 257 && rg[1] == 20 * 257 && rg[6] == 130 * 257,
          "RG/USHORT scale 257");

    float rgbf[12] = {0};
    glGetTexImage(0x0DE1, 0, 0x1907, 0x1406, rgbf);
    Check(glGetError() == 0, "RGB/FLOAT no error");
    Check(rgbf[0] > 0.038f && rgbf[0] < 0.041f &&
          rgbf[11] > 0.586f && rgbf[11] < 0.590f, "RGB/FLOAT normalize /255");

    unsigned char bgra[16] = {0};
    glGetTexImage(0x0DE1, 0, 0x80E1, 0x1401, bgra);
    Check(glGetError() == 0, "BGRA/UBYTE no error");
    Check(bgra[0] == 30 && bgra[1] == 20 && bgra[2] == 10 && bgra[3] == 40,
          "BGRA/UBYTE đảo R/B");

    unsigned char rev[16] = {0};
    glGetTexImage(0x0DE1, 0, 0x1908, 0x8367, rev);
    Check(glGetError() == 0, "RGBA/8888_REV no error");
    Check(rev[0] == 40 && rev[1] == 30 && rev[2] == 20 && rev[3] == 10,
          "RGBA/8888_REV byte order");

    unsigned int p210[4] = {0};
    glGetTexImage(0x0DE1, 0, 0x1908, 0x8368, p210);
    Check(glGetError() == 0, "RGBA/2_10_10_10_REV no error");
    Check((p210[0] & 1023u) == 40u && ((p210[0] >> 10) & 1023u) == 80u &&
          ((p210[0] >> 20) & 1023u) == 120u, "2_10_10_10_REV bit layout");

    // PACK_ROW_LENGTH=4 → hàng đích dài 16 byte, chỉ 8 byte đầu mỗi hàng có data.
    std::vector<unsigned char> packed(2 * 16, 0xEE);
    glPixelStorei(0x0D05 /*PACK_ALIGNMENT*/, 1);
    glPixelStorei(0x0D02 /*PACK_ROW_LENGTH*/, 4);
    glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, packed.data());
    Check(glGetError() == 0, "PACK_ROW_LENGTH no error");
    Check(packed[0] == 10 && packed[4] == 50 && packed[16] == 90 && packed[17] == 100,
          "PACK_ROW_LENGTH stride");
    Check(packed[15] == 0xEE, "PACK_ROW_LENGTH không ghi đè padding");

    // PACK_SKIP_ROWS=1, PACK_SKIP_PIXELS=1, ROW_LENGTH=2.
    unsigned char skip[32] = {0};
    glPixelStorei(0x0D02, 2);
    glPixelStorei(0x0D03 /*PACK_SKIP_ROWS*/, 1);
    glPixelStorei(0x0D04 /*PACK_SKIP_PIXELS*/, 1);
    glGetTexImage(0x0DE1, 0, 0x1908, 0x1401, skip);
    Check(glGetError() == 0, "PACK_SKIP no error");
    Check(skip[12] == 10 && skip[20] == 90, "PACK_SKIP rows/pixels");

    glPixelStorei(0x0D02, 0);
    glPixelStorei(0x0D03, 0);
    glPixelStorei(0x0D04, 0);
    glPixelStorei(0x0D05, 4);

    // glGetnTexImage phải từ chối bufSize thiếu và chấp nhận đủ.
    unsigned char nbuf[16] = {0};
    glGetnTexImage(0x0DE1, 0, 0x1908, 0x1401, 8, nbuf);
    Check(glGetError() == 0x0502, "glGetnTexImage bufSize thiếu → INVALID_OPERATION");
    glGetnTexImage(0x0DE1, 0, 0x1908, 0x1401, 16, nbuf);
    Check(glGetError() == 0, "glGetnTexImage đủ bufSize no error");
    Check(nbuf[0] == 10 && nbuf[12] == 130, "glGetnTexImage nội dung");

    // Format không hỗ trợ vẫn phải từ chối trung thực.
    glGetTexImage(0x0DE1, 0, 0x1902 /*DEPTH_COMPONENT*/, 0x1401, rgba);
    Check(glGetError() == 0x0502, "depth format → INVALID_OPERATION");

    if (fails) return 1;
    printf("test_get_texture_formats PASS\n");
    return 0;
}
