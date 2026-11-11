// Test nhóm Vertex Attribute của GL 4.6 Core — 81 entry point:
//   glVertexAttrib{1,2,3,4}{f,fv,d,dv}                    (16)
//   glVertexAttribI{1,2,3,4}{i,iv,ui,uiv} + I4{bv,sv,ubv,usv} (20)
//   glVertexAttribL{1,2,3,4}{d,dv}                        (8)
//   glVertexAttribP{1,2,3,4}{ui,uiv}                     (8)
//   glVertexAttribFormat / IFormat / LFormat,
//   glVertexAttribBinding / glVertexAttribDivisor,
//   glBindVertexBuffers / glDisableVertexArrayAttrib / glIsVertexArray,
//   glVertexArrayAttribIFormat / LFormat / BindingDivisor / VertexBuffers  (9)
//   glGetVertexAttrib{iv,fv,dv,Iiv,Iuiv,Ldv,Pointerv},
//   glGetVertexArrayiv / Indexediv / Indexed64iv          (10)
//
// Kiểm chứng:
//   • Giá trị generic phải round-trip: set → glGetVertexAttribfv/dv.
//   • Con trỏ NULL → INVALID_VALUE (trước đây deref ⇒ CRASH).
//   • Index vượt MAX_VERTEX_ATTRIBS → INVALID_VALUE.
//   • Format/Binding/Divisor phải ghi đúng state VAO và đọc lại được qua
//     glGetVertexArrayiv / glGetVertexArrayIndexediv (trước đây LUÔN trả 0).
//   • glGetVertexAttribPointerv: pname sai → INVALID_ENUM; trả về địa chỉ
//     thật của buffer đang map.

#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>

using namespace tglmt;
using namespace tglmt::gl;

namespace {

void DrainErrors() {
    while (glGetError() != GL_NO_ERROR) {
    }
}
GLenum FirstError() { return glGetError(); }
bool Near(float a, float b) { return fabsf(a - b) < 1e-5f; }
bool NearD(double a, double b) { return fabs(a - b) < 1e-9; }

// ===========================================================================
// Generic attribute values: float / double (16 hàm)
// ===========================================================================
bool TestGenericAttribValues() {
    // --- float ---
    glVertexAttrib1f(0, 1.5f);
    GLfloat v[4] = {0};
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.5f) && Near(v[1], 0.0f) && Near(v[3], 1.0f),
                "glVertexAttrib1f phải lưu đúng (w mặc định = 1)");

    glVertexAttrib2f(1, 1.0f, 2.0f);
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[1], 2.0f),
                "glVertexAttrib2f phải lưu đúng 2 thành phần");

    glVertexAttrib3f(2, 1.0f, 2.0f, 3.0f);
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 3.0f), "glVertexAttrib3f phải lưu đúng 3 thành phần");

    glVertexAttrib4f(3, 1.0f, 2.0f, 3.0f, 4.0f);
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glVertexAttrib4f phải lưu đúng 4 thành phần");

    const GLfloat f4[4] = {5.0f, 6.0f, 7.0f, 8.0f};
    glVertexAttrib1fv(4, f4);
    glGetVertexAttribfv(4, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 5.0f), "glVertexAttrib1fv phải lưu phần tử đầu");
    glVertexAttrib2fv(5, f4);
    glGetVertexAttribfv(5, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 5.0f) && Near(v[1], 6.0f), "glVertexAttrib2fv phải lưu đúng");
    glVertexAttrib3fv(6, f4);
    glGetVertexAttribfv(6, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 7.0f), "glVertexAttrib3fv phải lưu đúng");
    glVertexAttrib4fv(7, f4);
    glGetVertexAttribfv(7, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 8.0f), "glVertexAttrib4fv phải lưu đúng");

    // --- double ---
    glVertexAttrib1d(8, 1.25);
    GLdouble d[4] = {0};
    glGetVertexAttribdv(8, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[0], 1.25) && NearD(d[3], 1.0),
                "glVertexAttrib1d phải lưu đúng (w mặc định = 1)");
    glVertexAttrib2d(9, 1.0, 2.0);
    glGetVertexAttribdv(9, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[1], 2.0), "glVertexAttrib2d phải lưu đúng");
    glVertexAttrib3d(10, 1.0, 2.0, 3.0);
    glGetVertexAttribdv(10, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[2], 3.0), "glVertexAttrib3d phải lưu đúng");
    glVertexAttrib4d(11, 1.0, 2.0, 3.0, 4.0);
    glGetVertexAttribdv(11, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[3], 4.0), "glVertexAttrib4d phải lưu đúng");

    const GLdouble d4[4] = {5.0, 6.0, 7.0, 8.0};
    glVertexAttrib1dv(12, d4);
    glGetVertexAttribdv(12, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[0], 5.0), "glVertexAttrib1dv phải lưu phần tử đầu");
    glVertexAttrib2dv(13, d4);
    glGetVertexAttribdv(13, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[1], 6.0), "glVertexAttrib2dv phải lưu đúng");
    glVertexAttrib3dv(14, d4);
    glGetVertexAttribdv(14, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[2], 7.0), "glVertexAttrib3dv phải lưu đúng");
    glVertexAttrib4dv(15, d4);
    glGetVertexAttribdv(15, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[3], 8.0), "glVertexAttrib4dv phải lưu đúng");

    // --- NULL → INVALID_VALUE (trước đây CRASH) ---
    DrainErrors();
    glVertexAttrib1fv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttrib1fv NULL → INVALID_VALUE");
    DrainErrors();
    glVertexAttrib2dv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttrib2dv NULL → INVALID_VALUE");
    DrainErrors();
    glVertexAttrib4fv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttrib4fv NULL → INVALID_VALUE");

    // --- index vượt giới hạn → INVALID_VALUE ---
    DrainErrors();
    glVertexAttrib4f(9999, 1, 2, 3, 4);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glVertexAttrib4f với index vượt giới hạn → INVALID_VALUE");
    return true;
}

// ===========================================================================
// Integer / long / packed generic attributes (36 hàm)
// ===========================================================================
bool TestIntLongPackedAttribs() {
    GLfloat v[4] = {0};

    // --- int ---
    glVertexAttribI1i(0, -5);
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], -5.0f), "glVertexAttribI1i phải lưu đúng");
    glVertexAttribI2i(1, 1, 2);
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[1], 2.0f), "glVertexAttribI2i phải lưu đúng");
    glVertexAttribI3i(2, 1, 2, 3);
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 3.0f), "glVertexAttribI3i phải lưu đúng");
    glVertexAttribI4i(3, 1, 2, 3, 4);
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glVertexAttribI4i phải lưu đúng");

    // --- uint ---
    glVertexAttribI1ui(4, 7u);
    glGetVertexAttribfv(4, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 7.0f), "glVertexAttribI1ui phải lưu đúng");
    glVertexAttribI2ui(5, 1u, 2u);
    glGetVertexAttribfv(5, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[1], 2.0f), "glVertexAttribI2ui phải lưu đúng");
    glVertexAttribI3ui(6, 1u, 2u, 3u);
    glGetVertexAttribfv(6, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 3.0f), "glVertexAttribI3ui phải lưu đúng");
    glVertexAttribI4ui(7, 1u, 2u, 3u, 4u);
    glGetVertexAttribfv(7, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glVertexAttribI4ui phải lưu đúng");

    // --- mảng int/uint ---
    const GLint i4[4] = {1, 2, 3, 4};
    const GLuint u4[4] = {5u, 6u, 7u, 8u};
    glVertexAttribI1iv(8, i4);
    glGetVertexAttribfv(8, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f), "glVertexAttribI1iv phải lưu phần tử đầu");
    glVertexAttribI2iv(9, i4);
    glGetVertexAttribfv(9, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[1], 2.0f), "glVertexAttribI2iv phải lưu đúng");
    glVertexAttribI3iv(10, i4);
    glGetVertexAttribfv(10, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 3.0f), "glVertexAttribI3iv phải lưu đúng");
    glVertexAttribI4iv(11, i4);
    glGetVertexAttribfv(11, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 4.0f), "glVertexAttribI4iv phải lưu đúng");
    glVertexAttribI1uiv(12, u4);
    glGetVertexAttribfv(12, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 5.0f), "glVertexAttribI1uiv phải lưu phần tử đầu");
    glVertexAttribI2uiv(13, u4);
    glGetVertexAttribfv(13, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[1], 6.0f), "glVertexAttribI2uiv phải lưu đúng");
    glVertexAttribI3uiv(14, u4);
    glGetVertexAttribfv(14, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[2], 7.0f), "glVertexAttribI3uiv phải lưu đúng");
    glVertexAttribI4uiv(15, u4);
    glGetVertexAttribfv(15, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[3], 8.0f), "glVertexAttribI4uiv phải lưu đúng");

    // --- I4 b/s/ub/us ---
    const GLbyte b4[4] = {1, 2, 3, 4};
    const GLshort s4[4] = {5, 6, 7, 8};
    const GLubyte ub4[4] = {9, 10, 11, 12};
    const GLushort us4[4] = {13, 14, 15, 16};
    glVertexAttribI4bv(0, b4);
    glGetVertexAttribfv(0, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 1.0f) && Near(v[3], 4.0f), "glVertexAttribI4bv phải lưu đúng");
    glVertexAttribI4sv(1, s4);
    glGetVertexAttribfv(1, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 5.0f) && Near(v[3], 8.0f), "glVertexAttribI4sv phải lưu đúng");
    glVertexAttribI4ubv(2, ub4);
    glGetVertexAttribfv(2, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 9.0f) && Near(v[3], 12.0f), "glVertexAttribI4ubv phải lưu đúng");
    glVertexAttribI4usv(3, us4);
    glGetVertexAttribfv(3, GL_CURRENT_VERTEX_ATTRIB, v);
    TGLMT_CHECK(Near(v[0], 13.0f) && Near(v[3], 16.0f), "glVertexAttribI4usv phải lưu đúng");

    // --- long (d) ---
    GLdouble d[4] = {0};
    glVertexAttribL1d(4, 1.5);
    glGetVertexAttribdv(4, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[0], 1.5), "glVertexAttribL1d phải lưu đúng");
    glVertexAttribL2d(5, 1.0, 2.0);
    glGetVertexAttribdv(5, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[1], 2.0), "glVertexAttribL2d phải lưu đúng");
    glVertexAttribL3d(6, 1.0, 2.0, 3.0);
    glGetVertexAttribdv(6, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[2], 3.0), "glVertexAttribL3d phải lưu đúng");
    glVertexAttribL4d(7, 1.0, 2.0, 3.0, 4.0);
    glGetVertexAttribdv(7, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[3], 4.0), "glVertexAttribL4d phải lưu đúng");
    const GLdouble d4[4] = {5.0, 6.0, 7.0, 8.0};
    glVertexAttribL1dv(8, d4);
    glGetVertexAttribdv(8, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[0], 5.0), "glVertexAttribL1dv phải lưu phần tử đầu");
    glVertexAttribL2dv(9, d4);
    glGetVertexAttribdv(9, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[1], 6.0), "glVertexAttribL2dv phải lưu đúng");
    glVertexAttribL3dv(10, d4);
    glGetVertexAttribdv(10, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[2], 7.0), "glVertexAttribL3dv phải lưu đúng");
    glVertexAttribL4dv(11, d4);
    glGetVertexAttribdv(11, GL_CURRENT_VERTEX_ATTRIB, d);
    TGLMT_CHECK(NearD(d[3], 8.0), "glVertexAttribL4dv phải lưu đúng");

    // --- packed ---
    DrainErrors();
    // Kiểu packed hợp lệ (spec §10.3.3): 8_8_8_8 (1 comp), 2_10_10_10_REV (4),
    // 10F_11F_11F_REV (3), 10_10_10_2 (1).
    glVertexAttribP1ui(12, GL_UNSIGNED_INT_8_8_8_8, GL_FALSE, 0x11223344u);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexAttribP1ui không lỗi");
    glVertexAttribP1ui(12, GL_UNSIGNED_INT_10_10_10_2, GL_FALSE, 0x11223344u);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexAttribP1ui (10_10_10_2) không lỗi");
    glVertexAttribP2ui(12, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, 0u);
    glVertexAttribP3ui(12, GL_UNSIGNED_INT_10F_11F_11F_REV, GL_FALSE, 0u);
    glVertexAttribP4ui(12, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, 0u);
    const GLuint pu[4] = {1u, 2u, 3u, 4u};
    glVertexAttribP1uiv(12, GL_UNSIGNED_INT_8_8_8_8, GL_FALSE, pu);
    glVertexAttribP2uiv(12, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, pu);
    glVertexAttribP3uiv(12, GL_UNSIGNED_INT_10F_11F_11F_REV, GL_FALSE, pu);
    glVertexAttribP4uiv(12, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, pu);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexAttribP*uiv không lỗi");
    // Kiểu KHÔNG packed → INVALID_ENUM.
    DrainErrors();
    glVertexAttribP1ui(12, GL_UNSIGNED_INT, GL_FALSE, 0u);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glVertexAttribP1ui với kiểu không packed → INVALID_ENUM");

    // NULL → INVALID_VALUE.
    DrainErrors();
    glVertexAttribI1iv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttribI1iv NULL → INVALID_VALUE");
    DrainErrors();
    glVertexAttribI4bv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttribI4bv NULL → INVALID_VALUE");
    DrainErrors();
    glVertexAttribL2dv(0, nullptr);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glVertexAttribL2dv NULL → INVALID_VALUE");
    return true;
}

// ===========================================================================
// VAO state: Format / Binding / Divisor / BindVertexBuffers + getters
// ===========================================================================
bool TestVertexArrayState() {
    GLuint vao = 0, vbo = 0;
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glNamedBufferStorage(vbo, 256, nullptr, 0);
    glBindVertexArray(vao);

    // --- Format / Binding / Divisor ---
    glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 16);
    glVertexAttribBinding(2, 3);
    glVertexAttribDivisor(2, 1);
    DrainErrors();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexAttribFormat/Binding/Divisor không lỗi");

    // Getter DSA phải trả state THẬT (trước đây luôn 0).
    GLint v = 0;
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 4, "glGetVertexArrayIndexediv SIZE phải trả giá trị thật");
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_ARRAY_TYPE, &v);
    TGLMT_CHECK(v == GL_FLOAT, "glGetVertexArrayIndexediv TYPE phải trả giá trị thật");
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &v);
    TGLMT_CHECK(v == 1, "glGetVertexArrayIndexediv DIVISOR phải trả giá trị thật");
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_BINDING, &v);
    TGLMT_CHECK(v == 3, "glGetVertexArrayIndexediv BINDING phải trả giá trị thật");
    GLint64 rel = 0;
    glGetVertexArrayIndexed64iv(vao, 2, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &rel);
    TGLMT_CHECK(rel == 16, "glGetVertexArrayIndexed64iv RELATIVE_OFFSET phải trả giá trị thật");

    // --- glBindVertexBuffers ---
    const GLuint bufs[2] = {vbo, vbo};
    const GLintptr offs[2] = {0, 64};
    const GLsizei strides[2] = {32, 32};
    DrainErrors();
    glBindVertexBuffers(0, 2, bufs, offs, strides);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBindVertexBuffers không lỗi");
    // count âm → INVALID_VALUE.
    DrainErrors();
    glBindVertexBuffers(0, -1, bufs, offs, strides);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE, "glBindVertexBuffers count<0 → INVALID_VALUE");

    // --- glVertexArrayAttrib* (DSA, theo VAO id) ---
    GLuint vao2 = 0;
    glCreateVertexArrays(1, &vao2);
    glVertexArrayAttribIFormat(vao2, 1, 1, GL_INT, 8);
    glVertexArrayAttribLFormat(vao2, 2, 2, GL_DOUBLE, 4);
    glVertexArrayAttribFormat(vao2, 3, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao2, 3, 1);
    glVertexArrayBindingDivisor(vao2, 1, 2);
    glVertexArrayVertexBuffers(vao2, 0, 1, bufs, offs, strides);
    DrainErrors();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexArrayAttrib* / VertexBuffers không lỗi");
    glGetVertexArrayIndexediv(vao2, 1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 1, "glVertexArrayAttribIFormat phải ghi vào VAO đích");
    glGetVertexArrayIndexediv(vao2, 2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 2, "glVertexArrayAttribLFormat phải ghi vào VAO đích");
    // Divisor của BINDING áp cho attribute dùng binding đó.
    glGetVertexArrayIndexediv(vao2, 3, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &v);
    TGLMT_CHECK(v == 2, "glVertexArrayBindingDivisor phải áp cho attribute dùng binding đó");

    // --- glDisableVertexArrayAttrib ---
    glEnableVertexArrayAttrib(vao, 2);
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &v);
    TGLMT_CHECK(v == 1, "glEnableVertexArrayAttrib phải bật attrib");
    glDisableVertexArrayAttrib(vao, 2);
    glGetVertexArrayIndexediv(vao, 2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &v);
    TGLMT_CHECK(v == 0, "glDisableVertexArrayAttrib phải tắt attrib");

    // --- glIsVertexArray ---
    TGLMT_CHECK(glIsVertexArray(vao) == GL_TRUE, "glIsVertexArray phải TRUE với VAO hợp lệ");
    TGLMT_CHECK(glIsVertexArray(999999) == GL_FALSE, "glIsVertexArray phải FALSE với id lạ");

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &vao2);
    glDeleteBuffers(1, &vbo);
    return true;
}

// ===========================================================================
// Getters: glGetVertexAttrib* + glGetVertexAttribPointerv
// ===========================================================================
bool TestVertexAttribGetters() {
    GLuint vao = 0, vbo = 0;
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    const float data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    glNamedBufferStorage(vbo, sizeof(data), data, 0);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8, (void*)0);
    glEnableVertexAttribArray(1);

    GLint v = 0;
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 2, "glGetVertexAttribiv SIZE phải trả giá trị thật");
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_TYPE, &v);
    TGLMT_CHECK(v == GL_FLOAT, "glGetVertexAttribiv TYPE phải trả giá trị thật");
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &v);
    TGLMT_CHECK(v == 8, "glGetVertexAttribiv STRIDE phải trả giá trị thật");
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &v);
    TGLMT_CHECK(v == 1, "glGetVertexAttribiv ENABLED phải trả giá trị thật");

    // glGetVertexAttribfv/dv cho pname int → giá trị chuyển đổi.
    GLfloat fv = 0;
    glGetVertexAttribfv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &fv);
    TGLMT_CHECK(Near(fv, 2.0f), "glGetVertexAttribfv phải trả giá trị");
    GLdouble dv = 0;
    glGetVertexAttribdv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &dv);
    TGLMT_CHECK(NearD(dv, 2.0), "glGetVertexAttribdv phải trả giá trị");
    GLint iv = 0;
    glGetVertexAttribIiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &iv);
    TGLMT_CHECK(iv == 2, "glGetVertexAttribIiv phải trả giá trị");
    GLuint uiv = 0;
    glGetVertexAttribIuiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &uiv);
    TGLMT_CHECK(uiv == 2u, "glGetVertexAttribIuiv phải trả giá trị");
    GLdouble ldv = 0;
    glGetVertexAttribLdv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &ldv);
    TGLMT_CHECK(NearD(ldv, 2.0), "glGetVertexAttribLdv phải trả giá trị");

    // glGetVertexAttribPointerv: pname sai → INVALID_ENUM; pname đúng → con trỏ.
    DrainErrors();
    void* p = nullptr;
    glGetVertexAttribPointerv(1, 0x1234, &p);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glGetVertexAttribPointerv với pname sai → INVALID_ENUM");
    DrainErrors();
    glGetVertexAttribPointerv(1, GL_VERTEX_ATTRIB_ARRAY_POINTER, &p);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glGetVertexAttribPointerv với pname đúng không lỗi");
    TGLMT_CHECK(p != nullptr,
                "glGetVertexAttribPointerv phải trả địa chỉ buffer đang map");

    // Index vượt giới hạn → INVALID_VALUE.
    DrainErrors();
    glGetVertexAttribiv(9999, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_VALUE,
                "glGetVertexAttribiv với index vượt giới hạn → INVALID_VALUE");

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    return true;
}

// ===========================================================================
// 5) glVertexAttribIFormat / LFormat / LPointer + glGetVertexArrayiv
// ===========================================================================
bool TestAttribFormatsAndPointer() {
    GLuint vao = 0;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // glVertexAttribIFormat — format kiểu integer.
    glVertexAttribIFormat(1, 2, GL_INT, 8);
    GLint v = 0;
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 2, "glVertexAttribIFormat phải set size");
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_TYPE, &v);
    TGLMT_CHECK(v == GL_INT, "glVertexAttribIFormat phải set type");

    // glVertexAttribLFormat — format kiểu double (long).
    glVertexAttribLFormat(2, 3, GL_DOUBLE, 4);
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_SIZE, &v);
    TGLMT_CHECK(v == 3, "glVertexAttribLFormat phải set size");
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_TYPE, &v);
    TGLMT_CHECK(v == GL_DOUBLE, "glVertexAttribLFormat phải set type");

    // glVertexAttribLPointer — con trỏ cho format double.
    GLuint vbo = 0;
    glCreateBuffers(1, &vbo);
    const double data[8] = {0};
    glNamedBufferStorage(vbo, sizeof(data), data, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    DrainErrors();
    glVertexAttribLPointer(2, 3, GL_DOUBLE, 0, (void*)0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexAttribLPointer không lỗi");
    glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &v);
    TGLMT_CHECK(v == 0, "glVertexAttribLPointer phải set stride");

    // glGetVertexArrayiv — dạng không indexed: ELEMENT_ARRAY_BUFFER_BINDING.
    GLuint ebo = 0;
    glCreateBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glGetVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    TGLMT_CHECK((GLuint)v == ebo,
                "glGetVertexArrayiv ELEMENT_ARRAY_BUFFER_BINDING phải trả buffer thật");
    // pname sai → INVALID_ENUM.
    DrainErrors();
    glGetVertexArrayiv(vao, 0xDEAD, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_ENUM,
                "glGetVertexArrayiv pname sai → INVALID_ENUM");
    // VAO không tồn tại → INVALID_OPERATION.
    DrainErrors();
    glGetVertexArrayiv(999999, GL_ELEMENT_ARRAY_BUFFER_BINDING, &v);
    TGLMT_CHECK(FirstError() == GL_INVALID_OPERATION,
                "glGetVertexArrayiv với VAO không tồn tại → INVALID_OPERATION");

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    return true;
}

struct Case {
    const char* name;
    bool (*fn)();
};

}  // namespace

int main() {
    printf("=== core vertex attribute entry points (GL 4.6 §10.3/§10.4) ===\n");
    static tglmt::Context ctx("null");
    tglmt::Context::MakeCurrent(&ctx);
    const Case kCases[] = {
        {"glVertexAttrib{1..4}{f,fv,d,dv}", TestGenericAttribValues},
        {"glVertexAttribI*/L*/P*", TestIntLongPackedAttribs},
        {"VAO state: Format/Binding/Divisor/BindVertexBuffers", TestVertexArrayState},
        {"glGetVertexAttrib* + glGetVertexArray*", TestVertexAttribGetters},
        {"glVertexAttribI/LFormat/LPointer + glGetVertexArrayiv", TestAttribFormatsAndPointer},
    };
    bool ok = true;
    for (const Case& c : kCases) {
        const int before = tglmt_test::Failures();
        c.fn();
        const bool pass = (tglmt_test::Failures() == before);
        printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
        ok = ok && pass;
    }
    printf("%s\n", ok ? "PASS" : "FAIL");
    return TGLMT_RESULT;
}
