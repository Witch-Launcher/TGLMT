// test_legacy_typevariants.cpp — Tier A: các entry point immediate-mode với
// tham số kiểu số nguyên (short / int / byte / ubyte / ushort / uint).
//
// Kiểm chứng theo OpenGL 2.1 spec Table 2.9 (glspec21.pdf PDF p.59 = trang
// in 59) — "Component conversions. Color, normal, and depth components, (c),
// are converted to an internal floating-point representation, (f)":
//
//   ubyte  c/(2^8-1)     byte  (2c+1)/(2^8-1)
//   ushort c/(2^16-1)    short (2c+1)/(2^16-1)
//   uint   c/(2^32-1)    int   (2c+1)/(2^32-1)
//
// Và spec §2.7 (PDF p.34–35) cho Vertex / TexCoord / MultiTexCoord / Normal /
// Color / SecondaryColor.
//
// Mỗi test kiểm tra GIÁ TRỊ THỰC TẾ nằm trong state, không chỉ "gọi không
// crash".
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "tglmt/gl_legacy_types.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;

namespace {

void DrainErrors() { while (glGetError() != 0) {} }

const LegacyState& Legacy() { return Context::CurrentPtr()->legacy; }

// So sánh float với giá trị double kỳ vọng (tính theo Table 2.9).
// Dung sai tương đối 1e-5 + sàn tuyệt đối 1e-12 để bắt được cả sai lệch
// lớn (vd (float)c thay vì (2c+1)/(2^n-1)).
bool Near(float actual, double expected) {
    const double diff = (double)actual - expected;
    const double tol = 1e-5 * std::fabs(expected) + 1e-12;
    return diff <= tol && diff >= -tol;
}

// ---- Giá trị kỳ vọng theo Table 2.9 ----------------------------------------
constexpr double kU8 = 255.0;            // 2^8  - 1
constexpr double kU16 = 65535.0;         // 2^16 - 1
constexpr double kU32 = 4294967295.0;    // 2^32 - 1

double SB(GLbyte c) { return (2.0 * (double)c + 1.0) / kU8; }
double SS(GLshort c) { return (2.0 * (double)c + 1.0) / kU16; }
double SI(GLint c) { return (2.0 * (double)c + 1.0) / kU32; }
double UB(GLubyte c) { return (double)c / kU8; }
double US(GLushort c) { return (double)c / kU16; }
double UI(GLuint c) { return (double)c / kU32; }
// Vertex / TexCoord: chuyển TRỰC TIẾP sang float — Table 2.9 KHÔNG áp
// (spec GL 2.1 §2.14 p.73: Table 2.9 chỉ cho "Color, normal, and depth
// components").
double DI(GLint c) { return (double)c; }
double DS(GLshort c) { return (double)c; }

}  // namespace

int main() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context GL 4.6 Compatibility");
    if (!ctx) return 1;
    tglmtMakeCurrent(ctx);

    // =====================================================================
    // T1 — glVertex{234}i / iv: int → trực tiếp (KHÔNG Table 2.9)
    // =====================================================================
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        glVertex3i(1, 0, -1);
        glVertex3i(0, 0, 0);
        glVertex3i(0, 0, 0);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "T1: glVertex3i hợp lệ ⇒ không lỗi");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.immediateVertices.size() == 3, "T1: gom đủ 3 đỉnh");
        if (st.immediateVertices.size() == 3) {
            const LegacyVertex& v = st.immediateVertices[0];
            TGLMT_CHECK(Near(v.pos[0], DI(1)), "T1: x = 1 (chuyển trực tiếp)");
            TGLMT_CHECK(Near(v.pos[1], DI(0)), "T1: y = 0 (chuyển trực tiếp)");
            TGLMT_CHECK(Near(v.pos[2], DI(-1)), "T1: z = -1 (chuyển trực tiếp)");
            TGLMT_CHECK(v.pos[3] == 1.0f, "T1: w mặc định = 1");
            // Spec GL 2.1 §2.14 (p.73): Table 2.9 chỉ cho color/normal/depth
            // ⇒ toạ độ đỉnh chuyển TRỰC TIẾP sang float.
            TGLMT_CHECK(v.pos[0] == 1.0f, "T1: x = raw cast (float)1");
        }
    }
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        const GLint v[4] = {2, -3, 4, 5};
        glVertex4iv(v);
        glVertex3i(0, 0, 0);
        glVertex3i(0, 0, 0);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "T1b: glVertex4iv hợp lệ");
        const LegacyState& st = Legacy();
        if (!st.immediateVertices.empty()) {
            const LegacyVertex& v0 = st.immediateVertices[0];
            TGLMT_CHECK(Near(v0.pos[0], DI(2)) && Near(v0.pos[1], DI(-3)) &&
                            Near(v0.pos[2], DI(4)) && Near(v0.pos[3], DI(5)),
                        "T1b: glVertex4iv dùng đúng Table 2.9 cho cả 4 thành phần");
        }
    }
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        glVertex2i(7, 8);
        glVertex3i(0, 0, 0);
        glVertex3i(0, 0, 0);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "T1c: glVertex2i hợp lệ");
        const LegacyState& st = Legacy();
        if (!st.immediateVertices.empty()) {
            const LegacyVertex& v0 = st.immediateVertices[0];
            TGLMT_CHECK(Near(v0.pos[0], DI(7)) && Near(v0.pos[1], DI(8)) &&
                            v0.pos[2] == 0.0f && v0.pos[3] == 1.0f,
                        "T1c: glVertex2i ⇒ z=0, w=1");
        }
    }

    // =====================================================================
    // T2 — glVertex{234}s / sv: short → trực tiếp (KHÔNG Table 2.9)
    // =====================================================================
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        glVertex2s(-32768, 32767);
        glVertex3i(0, 0, 0);
        glVertex3i(0, 0, 0);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "T2: glVertex2s hợp lệ");
        const LegacyState& st = Legacy();
        if (!st.immediateVertices.empty()) {
            const LegacyVertex& v0 = st.immediateVertices[0];
            TGLMT_CHECK(Near(v0.pos[0], DS(-32768)) && Near(v0.pos[1], DS(32767)),
                        "T2: short chuyển trực tiếp ⇒ (-32768, 32767)");
            TGLMT_CHECK(v0.pos[0] == -32768.0f && v0.pos[1] == 32767.0f,
                        "T2: -32768 → -32768.0 và 32767 → 32767.0");
        }
    }
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        const GLshort v[3] = {100, -100, 0};
        glVertex3sv(v);
        glVertex3i(0, 0, 0);
        glVertex3i(0, 0, 0);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "T2b: glVertex3sv hợp lệ");
        const LegacyState& st = Legacy();
        if (!st.immediateVertices.empty()) {
            const LegacyVertex& v0 = st.immediateVertices[0];
            TGLMT_CHECK(Near(v0.pos[0], DS(100)) && Near(v0.pos[1], DS(-100)) &&
                            Near(v0.pos[2], DS(0)),
                        "T2b: glVertex3sv dùng đúng Table 2.9");
        }
    }

    // =====================================================================
    // T3 — Color: ubyte (đã có) + int + short
    // =====================================================================
    {
        DrainErrors();
        glColor3ub(255, 128, 0);
        TGLMT_CHECK(glGetError() == 0, "T3: glColor3ub hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], UB(255)) &&
                        Near(st.currentColor[1], UB(128)) &&
                        Near(st.currentColor[2], UB(0)) &&
                        st.currentColor[3] == 1.0f,
                    "T3: glColor3ub ⇒ (1, 128/255, 0), alpha=1");
    }
    {
        DrainErrors();
        glColor3i(1, 0, -1);
        TGLMT_CHECK(glGetError() == 0, "T3b: glColor3i hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SI(1)) &&
                        Near(st.currentColor[1], SI(0)) &&
                        Near(st.currentColor[2], SI(-1)) &&
                        st.currentColor[3] == 1.0f,
                    "T3b: glColor3i ⇒ int → (2c+1)/(2^32-1)");
    }
    {
        DrainErrors();
        const GLint v[4] = {3, 4, 5, 6};
        glColor4iv(v);
        TGLMT_CHECK(glGetError() == 0, "T3c: glColor4iv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SI(3)) &&
                        Near(st.currentColor[1], SI(4)) &&
                        Near(st.currentColor[2], SI(5)) &&
                        Near(st.currentColor[3], SI(6)),
                    "T3c: glColor4iv ⇒ cả alpha cũng theo Table 2.9");
    }
    {
        DrainErrors();
        glColor3s(-32768, 32767, 0);
        TGLMT_CHECK(glGetError() == 0, "T3d: glColor3s hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SS(-32768)) &&
                        Near(st.currentColor[1], SS(32767)) &&
                        Near(st.currentColor[2], SS(0)),
                    "T3d: glColor3s ⇒ short → (2c+1)/(2^16-1)");
    }
    {
        DrainErrors();
        const GLshort v[4] = {1, 2, 3, 4};
        glColor4sv(v);
        TGLMT_CHECK(glGetError() == 0, "T3e: glColor4sv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SS(1)) &&
                        Near(st.currentColor[3], SS(4)),
                    "T3e: glColor4sv ⇒ alpha theo Table 2.9");
    }

    // =====================================================================
    // T4 — Color byte/ubyte/ushort/uint (đã có sẵn, đã sửa phép chuyển)
    //      byte → (2c+1)/(2^8-1)  ⇒ 127 → 1.0, 0 → 1/255, -128 → -1.0
    // =====================================================================
    {
        DrainErrors();
        glColor3b(127, 0, -128);
        TGLMT_CHECK(glGetError() == 0, "T4: glColor3b hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SB(127)) &&
                        Near(st.currentColor[1], SB(0)) &&
                        Near(st.currentColor[2], SB(-128)),
                    "T4: glColor3b(127,0,-128) ⇒ (1, 1/255, -1)");
        TGLMT_CHECK(Near(st.currentColor[0], 255.0 / 255.0) &&
                        Near(st.currentColor[1], 1.0 / 255.0) &&
                        Near(st.currentColor[2], -255.0 / 255.0),
                    "T4: giá trị đúng (255/255, 1/255, -255/255)");
        TGLMT_CHECK(st.currentColor[1] != 0.0f,
                    "T4: byte 0 → 1/255 (KHÔNG phải 0 như công thức c/127 cũ)");
    }
    {
        DrainErrors();
        const GLbyte v[4] = {-128, 127, 0, 5};
        glColor4bv(v);
        TGLMT_CHECK(glGetError() == 0, "T4b: glColor4bv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], SB(-128)) &&
                        Near(st.currentColor[3], SB(5)),
                    "T4b: glColor4bv alpha theo Table 2.9");
    }
    {
        DrainErrors();
        glColor3ui(4294967295u, 0u, 0u);
        TGLMT_CHECK(glGetError() == 0, "T4c: glColor3ui hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentColor[0], UI(4294967295u)) &&
                        st.currentColor[0] == 1.0f,
                    "T4c: uint → c/(2^32-1) ⇒ 4294967295 → 1.0");
    }
    {
        DrainErrors();
        glColor3us(65535, 0, 0);
        TGLMT_CHECK(glGetError() == 0, "T4d: glColor3us hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.currentColor[0] == 1.0f && st.currentColor[1] == 0.0f,
                    "T4d: ushort → c/(2^16-1) ⇒ 65535 → 1.0");
    }

    // =====================================================================
    // T5 — Normal: byte/int/short
    //      spec §2.7: "Byte, short, or integer values passed to Normal are
    //      converted to floating-point values as indicated for the
    //      corresponding (signed) type in table 2.9."
    // =====================================================================
    {
        DrainErrors();
        glNormal3b(-128, 0, 127);
        TGLMT_CHECK(glGetError() == 0, "T5: glNormal3b hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentNormal[0], SB(-128)) &&
                        Near(st.currentNormal[1], SB(0)) &&
                        Near(st.currentNormal[2], SB(127)),
                    "T5: glNormal3b ⇒ (-1, 1/255, 1)");
    }
    {
        DrainErrors();
        const GLint v[3] = {1, 0, -1};
        glNormal3iv(v);
        TGLMT_CHECK(glGetError() == 0, "T5b: glNormal3iv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentNormal[0], SI(1)) &&
                        Near(st.currentNormal[2], SI(-1)),
                    "T5b: glNormal3iv ⇒ int → (2c+1)/(2^32-1)");
    }
    {
        DrainErrors();
        const GLshort v[3] = {-32768, 32767, 0};
        glNormal3sv(v);
        TGLMT_CHECK(glGetError() == 0, "T5c: glNormal3sv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.currentNormal[0] == -1.0f && st.currentNormal[1] == 1.0f,
                    "T5c: glNormal3sv ⇒ (-1, 1)");
    }
    {
        DrainErrors();
        glNormal3i(2, 3, 4);
        TGLMT_CHECK(glGetError() == 0, "T5d: glNormal3i hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentNormal[0], SI(2)) &&
                        Near(st.currentNormal[1], SI(3)) &&
                        Near(st.currentNormal[2], SI(4)),
                    "T5d: glNormal3i dùng Table 2.9");
    }

    // =====================================================================
    // T6 — TexCoord{1..4}{i,s}{,v}
    // =====================================================================
    {
        DrainErrors();
        glActiveTexture(GL_TEXTURE0);
        glTexCoord2s(-32768, 32767);
        TGLMT_CHECK(glGetError() == 0, "T6: glTexCoord2s hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[0][0], DS(-32768)) &&
                        Near(st.currentTexCoord[0][1], DS(32767)) &&
                        st.currentTexCoord[0][2] == 0.0f &&
                        st.currentTexCoord[0][3] == 1.0f,
                    "T6: glTexCoord2s ⇒ s=-32768, t=32767 (chuyển trực tiếp), r=0, q=1");
        TGLMT_CHECK(st.currentTexCoord[0][0] == -32768.0f &&
                        st.currentTexCoord[0][1] == 32767.0f,
                    "T6: s = -32768.0, t = 32767.0 (chuyển trực tiếp)");
    }
    {
        DrainErrors();
        glTexCoord1i(1);
        TGLMT_CHECK(glGetError() == 0, "T6b: glTexCoord1i hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[0][0], DI(1)) &&
                        st.currentTexCoord[0][1] == 0.0f &&
                        st.currentTexCoord[0][2] == 0.0f &&
                        st.currentTexCoord[0][3] == 1.0f,
                    "T6b: glTexCoord1i ⇒ t=0, r=0, q=1");
    }
    {
        DrainErrors();
        const GLint v[4] = {1, 2, 3, 4};
        glTexCoord4iv(v);
        TGLMT_CHECK(glGetError() == 0, "T6c: glTexCoord4iv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[0][0], DI(1)) &&
                        Near(st.currentTexCoord[0][1], DI(2)) &&
                        Near(st.currentTexCoord[0][2], DI(3)) &&
                        Near(st.currentTexCoord[0][3], DI(4)),
                    "T6c: glTexCoord4iv dùng Table 2.9 cho cả 4 thành phần");
    }
    {
        DrainErrors();
        const GLshort v[3] = {5, 6, 7};
        glTexCoord3sv(v);
        TGLMT_CHECK(glGetError() == 0, "T6d: glTexCoord3sv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[0][0], DS(5)) &&
                        Near(st.currentTexCoord[0][2], DS(7)) &&
                        st.currentTexCoord[0][3] == 1.0f,
                    "T6d: glTexCoord3sv ⇒ q=1");
    }

    // =====================================================================
    // T7 — MultiTexCoord{1..4}{i,s}{,v}: ghi vào unit của TARGET — chuyển TRỰC TIẾP
    // =====================================================================
    {
        DrainErrors();
        const float unit0_before = Legacy().currentTexCoord[0][0];
        glMultiTexCoord2s(GL_TEXTURE0 + 2, -32768, 32767);
        TGLMT_CHECK(glGetError() == 0, "T7: glMultiTexCoord2s hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[2][0], DS(-32768)) &&
                        Near(st.currentTexCoord[2][1], DS(32767)) &&
                        st.currentTexCoord[2][2] == 0.0f &&
                        st.currentTexCoord[2][3] == 1.0f,
                    "T7: glMultiTexCoord2s ghi vào unit 2 (target), không phải unit đang active");
        TGLMT_CHECK(st.currentTexCoord[0][0] == unit0_before,
                    "T7: unit 0 KHÔNG bị đụng tới");
    }
    {
        DrainErrors();
        const GLint v[2] = {10, 20};
        glMultiTexCoord2iv(GL_TEXTURE0 + 1, v);
        TGLMT_CHECK(glGetError() == 0, "T7b: glMultiTexCoord2iv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[1][0], DI(10)) &&
                        Near(st.currentTexCoord[1][1], DI(20)),
                    "T7b: glMultiTexCoord2iv ghi vào unit 1");
    }
    {
        DrainErrors();
        glMultiTexCoord1i(GL_TEXTURE0 + 3, 1);
        TGLMT_CHECK(glGetError() == 0, "T7c: glMultiTexCoord1i hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[3][0], DI(1)) &&
                        st.currentTexCoord[3][1] == 0.0f &&
                        st.currentTexCoord[3][3] == 1.0f,
                    "T7c: glMultiTexCoord1i ⇒ t=0, r=0, q=1");
    }
    {
        DrainErrors();
        const GLshort v[4] = {1, 2, 3, 4};
        glMultiTexCoord4sv(GL_TEXTURE0 + 4, v);
        TGLMT_CHECK(glGetError() == 0, "T7d: glMultiTexCoord4sv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentTexCoord[4][0], DS(1)) &&
                        Near(st.currentTexCoord[4][3], DS(4)),
                    "T7d: glMultiTexCoord4sv dùng Table 2.9");
    }

    // =====================================================================
    // T8 — SecondaryColor{3}{b,i,s,ub,ui,us}{,v}
    // =====================================================================
    {
        DrainErrors();
        glSecondaryColor3ub(255, 128, 0);
        TGLMT_CHECK(glGetError() == 0, "T8: glSecondaryColor3ub hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentSecondaryColor[0], UB(255)) &&
                        Near(st.currentSecondaryColor[1], UB(128)) &&
                        Near(st.currentSecondaryColor[2], UB(0)),
                    "T8: glSecondaryColor3ub ⇒ (1, 128/255, 0)");
    }
    {
        DrainErrors();
        glSecondaryColor3b(127, 0, -128);
        TGLMT_CHECK(glGetError() == 0, "T8b: glSecondaryColor3b hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentSecondaryColor[0], SB(127)) &&
                        Near(st.currentSecondaryColor[1], SB(0)) &&
                        Near(st.currentSecondaryColor[2], SB(-128)),
                    "T8b: glSecondaryColor3b ⇒ (1, 1/255, -1)");
    }
    {
        DrainErrors();
        const GLint v[3] = {1, 0, -1};
        glSecondaryColor3iv(v);
        TGLMT_CHECK(glGetError() == 0, "T8c: glSecondaryColor3iv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(Near(st.currentSecondaryColor[0], SI(1)) &&
                        Near(st.currentSecondaryColor[2], SI(-1)),
                    "T8c: glSecondaryColor3iv ⇒ int → (2c+1)/(2^32-1)");
    }
    {
        DrainErrors();
        const GLshort v[3] = {-32768, 32767, 0};
        glSecondaryColor3sv(v);
        TGLMT_CHECK(glGetError() == 0, "T8d: glSecondaryColor3sv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.currentSecondaryColor[0] == -1.0f &&
                        st.currentSecondaryColor[1] == 1.0f,
                    "T8d: glSecondaryColor3sv ⇒ (-1, 1)");
    }
    {
        DrainErrors();
        glSecondaryColor3ui(4294967295u, 0u, 0u);
        TGLMT_CHECK(glGetError() == 0, "T8e: glSecondaryColor3ui hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.currentSecondaryColor[0] == 1.0f,
                    "T8e: uint → c/(2^32-1) ⇒ 1.0");
    }
    {
        DrainErrors();
        const GLushort v[3] = {65535, 0, 0};
        glSecondaryColor3usv(v);
        TGLMT_CHECK(glGetError() == 0, "T8f: glSecondaryColor3usv hợp lệ");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.currentSecondaryColor[0] == 1.0f,
                    "T8f: ushort → c/(2^16-1) ⇒ 1.0");
    }

    // =====================================================================
    // T9 — Lỗi: ngoài Begin/End ⇒ GL_INVALID_OPERATION (spec §2.6.3)
    // =====================================================================
    {
        DrainErrors();
        const size_t before = Legacy().immediateVertices.size();
        glVertex3i(1, 2, 3);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "T9: glVertex3i ngoài Begin/End ⇒ GL_INVALID_OPERATION");
        TGLMT_CHECK(Legacy().immediateVertices.size() == before,
                    "T9: glVertex3i ngoài Begin/End KHÔNG sinh đỉnh");
        DrainErrors();
        glColor3i(1, 2, 3);
        TGLMT_CHECK(glGetError() == 0,
                    "T9: glColor3i ngoài Begin/End hợp lệ (current color vẫn set)");
    }
    {
        // Target texture không hợp lệ ⇒ lỗi giống hệt biến thể float.
        DrainErrors();
        glMultiTexCoord2i(0x84C0 + 99, 1, 2);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE,
                    "T9b: glMultiTexCoord2i target sai ⇒ GL_INVALID_VALUE");
    }
    {
        // Con trỏ NULL: cùng mã lỗi với biến thể float (xem ghi chú trong
        // src/gl/gl_legacy_typevariants.cpp).
        DrainErrors();
        glVertex3iv(nullptr);
        const GLenum e1 = glGetError();
        const GLenum e2 = glGetError();
        TGLMT_CHECK(e1 != 0, "T9c: glVertex3iv(NULL) ⇒ sinh lỗi");
        TGLMT_CHECK(e2 == 0, "T9c: lỗi chỉ sinh 1 lần");
    }

    tglmtMakeCurrent(nullptr);
    return TGLMT_RESULT;
}
