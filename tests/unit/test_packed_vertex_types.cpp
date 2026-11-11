// test_packed_vertex_types.cpp — Unit: ARB_vertex_type_2_10_10_10_rev +
// ARB_vertex_type_10f_11f_11f_rev cho các lệnh P* (Vertex/TexCoord/MultiTexCoord/
// Normal/Color/SecondaryColor).
// Spec: docs/khronos/extensions/ARB/ARB_vertex_type_2_10_10_10_rev.txt (số component
// mỗi lệnh tiêu thụ + INVALID_ENUM khi sai type), glspec46.core.pdf §10.3.8 (bố cục
// bit), §2.3.4.3/§2.3.4.4 (float 11/10 bit không dấu), §2.3.5.1 (eq 2.1/2.2).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_packed_vertex_types FAIL: %s\n", msg); ++gFails; }
}
static bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static const GLuint U_2_10_10_10 = 0x8368; // GL_UNSIGNED_INT_2_10_10_10_REV
static const GLuint I_2_10_10_10 = 0x8D9F; // GL_INT_2_10_10_10_REV
static const GLuint U_10F_11F_11F = 0x8C3B; // GL_UNSIGNED_INT_10F_11F_11F_REV
static const GLuint CURRENT_ATTRIB = 0x8626; // GL_CURRENT_VERTEX_ATTRIB

// Đọc generic attribute hiện tại (compat layout: 0=vertex, 2=normal, 3=color,
// 4=secondary, 8+unit=texcoord).
static void ReadAttrib(GLuint idx, GLfloat v[4]) {
    v[0] = v[1] = v[2] = v[3] = -99.0f;
    glGetVertexAttribfv(idx, CURRENT_ATTRIB, v);
}

// packed word cho UNSIGNED_INT_2_10_10_10_REV: w(2) | z(10) | y(10) | x(10)
static GLuint PackU(unsigned x, unsigned y, unsigned z, unsigned w) {
    return (GLuint)((x & 0x3FFu) | ((y & 0x3FFu) << 10) |
                    ((z & 0x3FFu) << 20) | ((w & 0x3u) << 30));
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // --- glVertexP2ui: chỉ 2 component (x,y); z=0, w=1 (như glVertex2f) ---
    glVertexP2ui(U_2_10_10_10, PackU(0x3FF, 0x3FF, 0x1FF, 3));
    GLfloat v[4];
    ReadAttrib(0, v);
    Check(Near(v[0], 1.0f), "VertexP2ui x = 1.0");
    Check(Near(v[1], 1.0f), "VertexP2ui y = 1.0");
    Check(Near(v[2], 0.0f), "VertexP2ui z = 0 (component không dùng)");
    Check(Near(v[3], 1.0f), "VertexP2ui w = 1 (component không dùng)");

    // --- glVertexP3ui: 3 component (x,y,z), w=1 ---
    glVertexP3ui(U_2_10_10_10, PackU(0x3FF, 0x000, 0x3FF, 3));
    ReadAttrib(0, v);
    Check(Near(v[0], 1.0f), "VertexP3ui x = 1.0");
    Check(Near(v[1], 0.0f), "VertexP3ui y = 0");
    Check(Near(v[2], 1.0f), "VertexP3ui z = 1.0");
    Check(Near(v[3], 1.0f), "VertexP3ui w = 1");

    // --- glVertexP4ui: 4 component, w lấy từ 2 bit cao ---
    glVertexP4ui(U_2_10_10_10, PackU(0x3FF, 0x3FF, 0x3FF, 1));
    ReadAttrib(0, v);
    Check(Near(v[0], 1.0f) && Near(v[1], 1.0f) && Near(v[2], 1.0f), "VertexP4ui xyz");
    Check(Near(v[3], 1.0f / 3.0f), "VertexP4ui w = 1/3 (2-bit unsigned, eq 2.1)");

    // --- glColorP3ui: (r,g,b) + alpha = 1.0 ---
    glColorP3ui(U_2_10_10_10, PackU(0x3FF, 0x000, 0x3FF, 3));
    ReadAttrib(3, v);
    Check(Near(v[0], 1.0f) && Near(v[1], 0.0f) && Near(v[2], 1.0f), "ColorP3ui rgb");
    Check(Near(v[3], 1.0f), "ColorP3ui alpha = 1.0");

    // --- glColorP4ui: 4 component, alpha từ 2 bit ---
    glColorP4ui(U_2_10_10_10, PackU(0x3FF, 0x3FF, 0x3FF, 2));
    ReadAttrib(3, v);
    Check(Near(v[3], 2.0f / 3.0f), "ColorP4ui alpha = 2/3");

    // --- glSecondaryColorP3ui: attrib 4, alpha = 1.0 ---
    glSecondaryColorP3ui(U_2_10_10_10, PackU(0x000, 0x3FF, 0x000, 0));
    ReadAttrib(4, v);
    Check(Near(v[0], 0.0f) && Near(v[1], 1.0f) && Near(v[2], 0.0f), "SecondaryColorP3ui rgb");
    Check(Near(v[3], 1.0f), "SecondaryColorP3ui alpha = 1.0");

    // --- glNormalP3ui: attrib 2, 3 component ---
    glNormalP3ui(U_2_10_10_10, PackU(0x3FF, 0x000, 0x3FF, 0));
    ReadAttrib(2, v);
    Check(Near(v[0], 1.0f) && Near(v[1], 0.0f) && Near(v[2], 1.0f), "NormalP3ui xyz");
    Check(Near(v[3], 1.0f), "NormalP3ui w = 1");

    // --- glTexCoordP1ui: 1 component (s) → attrib 8 ---
    glTexCoordP1ui(U_2_10_10_10, PackU(0x3FF, 0x3FF, 0x3FF, 0));
    ReadAttrib(8, v);
    Check(Near(v[0], 1.0f), "TexCoordP1ui s = 1.0");
    Check(Near(v[1], 0.0f) && Near(v[2], 0.0f) && Near(v[3], 1.0f), "TexCoordP1ui t/z/w mặc định");

    // --- glMultiTexCoordP2ui: texture unit → attrib 8+unit (không phải luôn 8) ---
    glTexCoordP2ui(U_2_10_10_10, PackU(0x000, 0x000, 0, 0));   // attrib 8 = (0,0,0,1)
    glMultiTexCoordP2ui(0x84C0 + 1 /*GL_TEXTURE1*/, U_2_10_10_10, PackU(0x3FF, 0x3FF, 0, 0));
    ReadAttrib(9, v);
    Check(Near(v[0], 1.0f) && Near(v[1], 1.0f), "MultiTexCoordP2ui(unit1) ghi attrib 9");
    Check(Near(v[2], 0.0f) && Near(v[3], 1.0f), "MultiTexCoordP2ui(unit1) z/w mặc định");
    ReadAttrib(8, v);
    Check(Near(v[0], 0.0f) && Near(v[1], 0.0f), "MultiTexCoordP2ui không ghi đè attrib 8");

    // unit khác cũng phải vào attrib riêng (unit 2 → attrib 10)
    glMultiTexCoordP1ui(0x84C0 + 2 /*GL_TEXTURE2*/, U_2_10_10_10, PackU(0x3FF, 0, 0, 0));
    ReadAttrib(10, v);
    Check(Near(v[0], 1.0f), "MultiTexCoordP1ui(unit2) ghi attrib 10");

    // --- INT_2_10_10_10_REV: có dấu, clamp xuống -1.0 (eq 2.2) ---
    // x = 0x200 (10-bit two's complement) = -512 → -512/511 < -1 → clamp -1.0
    // y = 0x1FF = 511 (max dương của 10-bit có dấu) → 511/511 = 1.0
    glColorP3ui(I_2_10_10_10, (GLuint)(0x200u | (0x1FFu << 10)));
    ReadAttrib(3, v);
    Check(Near(v[0], -1.0f), "INT_2_10_10_10_REV clamp xuống -1.0");
    Check(Near(v[1], 1.0f), "INT_2_10_10_10_REV y = 511/511 = 1.0");

    // --- UNSIGNED_INT_10F_11F_11F_REV: 3 float 11/11/10 bit (§2.3.4.3/§2.3.4.4) ---
    // 1.0 trong 11-bit float: E=15, M=0 → 15<<6 = 0x3C0; 10-bit: E=15, M=0 → 15<<5 = 0x1E0
    const GLuint f11 = 0x3C0u, f10 = 0x1E0u;
    glVertexP3ui(U_10F_11F_11F, f11 | (f11 << 11) | (f10 << 22));
    ReadAttrib(0, v);
    Check(Near(v[0], 1.0f) && Near(v[1], 1.0f) && Near(v[2], 1.0f),
          "10F_11F_11F_REV decode = 1.0 (không phải uint/2047)");
    Check(Near(v[3], 1.0f), "10F_11F_11F_REV w mặc định 1");

    // --- type sai → GL_INVALID_ENUM, state không đổi ---
    glVertexP3ui(0x1406 /*GL_FLOAT*/, 0);
    Check(glGetError() == GL_INVALID_ENUM, "VertexP3ui type sai → INVALID_ENUM");
    ReadAttrib(0, v);
    Check(Near(v[0], 1.0f) && Near(v[2], 1.0f), "VertexP3ui type sai: attrib giữ nguyên");

    glMultiTexCoordP2ui(0x84C0 + 1, 0x1406, 0);
    Check(glGetError() == GL_INVALID_ENUM, "MultiTexCoordP2ui type sai → INVALID_ENUM");

    // texture unit ngoài GL_TEXTURE0.. → INVALID_ENUM
    glMultiTexCoordP2ui(0x84C0 + 40, U_2_10_10_10, 0);
    Check(glGetError() == GL_INVALID_ENUM, "MultiTexCoordP2ui texture unit sai → INVALID_ENUM");

    // P4 với format chỉ có 3 component (10F_11F_11F_REV) → INVALID_ENUM
    glVertexP4ui(U_10F_11F_11F, 0);
    Check(glGetError() == GL_INVALID_ENUM, "VertexP4ui với 10F_11F_11F_REV → INVALID_ENUM");

    // Bản "v" cũng phải validate type
    const GLuint bad = 0;
    glVertexP2uiv(0x8C3B /*UNSIGNED_INT_10F_11F_11F_REV*/, &bad);  // hợp lệ (3 comp ≥ 2)
    Check(glGetError() == GL_NO_ERROR, "VertexP2uiv nhận 10F_11F_11F_REV");
    glVertexP2uiv(0x1406, &bad);
    Check(glGetError() == GL_INVALID_ENUM, "VertexP2uiv type sai → INVALID_ENUM");

    return gFails ? 1 : 0;
}
