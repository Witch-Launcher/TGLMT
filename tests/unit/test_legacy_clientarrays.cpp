// test_legacy_clientarrays.cpp — 13 entry point CLIENT-SIDE VERTEX ARRAYS.
//
// Spec: OpenGL 4.2 Compatibility Profile §2.8 "Vertex Arrays"
//       (.cache/spec/glspec42.compatibility.pdf, PDF p.58–75)
//       + OpenGL 2.1 §2.8 (glspec21.pdf, PDF p.38–46) cho bảng kiểu dữ liệu.
//
// Những gì test kiểm tra (KHÔNG chỉ "gọi không crash"):
//  1. glEnableClientState/glDisableClientState: state bật/tắt + glIsEnabled +
//     enum sai ⇒ GL_INVALID_ENUM.
//  2. 8 lệnh *Pointer: size/type/stride sai ⇒ GL_INVALID_VALUE/INVALID_ENUM;
//     pointer NULL hợp lệ.
//  3. glClientActiveTexture: giá trị hợp lệ + ngoài dải ⇒ GL_INVALID_ENUM.
//  4. Trong glBegin/glEnd: lệnh client-state bị cấm ⇒ GL_INVALID_OPERATION.
//  5. ĐƯỜNG DRAW THẬT: glVertexPointer + glEnableClientState(GL_VERTEX_ARRAY)
//     + glDrawArrays(GL_TRIANGLES,0,3) ⇒ draw path phải đọc 3 đỉnh từ client
//     array (kiểm tra vertex buffer + trace + bộ đếm nội bộ).
//  6. Quy tắc chuyển đổi kiểu dữ liệu (Table 2.5 + Table 2.9):
//     VertexPointer = "cast" (KHÔNG chuẩn hoá), ColorPointer = "normalize"
//     ([0,1]), NormalPointer = "normalize" ([-1,1]).
//  7. glInterleavedArrays: 14 format của Table 2.8 (offset + stride tính đúng).
//  8. glArrayElement: trong glBegin/glEnd sinh đỉnh; i < 0 ⇒ INVALID_VALUE.
//  9. Display list: glEnableClientState được GHI vào list (GL_COMPILE ⇒ không
//     thực thi; replay ⇒ thực thi).
// 10. Client memory vs buffer object: ARRAY_BUFFER đang bind ⇒ pointer là
//     byte offset trong buffer, ngược lại là con trỏ client.
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null — không có GPU ⇒ verify được
// state/vertex buffer/trace, KHÔNG verify được pixel).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace tglmt;

// API nội bộ của src/gl/gl_legacy_clientarrays.cpp (bộ đếm để verify draw
// path thực sự đọc client array).
namespace tglmt { namespace gl {
uint32_t LegacyClientArrayVerticesFetched();
uint32_t LegacyClientArrayIndicesFetched();
bool LegacyClientArrayUsedClientMemory();
bool LegacyClientArrayDrew();
bool LegacyClientArrayActive();
} }

namespace {

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

TGLMTContext* CoreCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

// Layout vertex của đường fixed-function (LegacyDraw.h): 6 × float4.
//   pos = float 0..3, normal = 4..6, color = 8..11, tex0 = 12..15,
//   tex1 = 16..19, secColor = 20..22.
constexpr int kFloatPerVertex = 24;

const float* VertexFloats(const Context& c) {
    if (!c.legacy.vertexBuffer) return nullptr;
    const float* v = (const float*)c.legacy.vertexBuffer->contents();
    if (!v) return nullptr;
    return v + (c.legacy.vertexOffset / sizeof(float));
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. glEnableClientState / glDisableClientState
// ---------------------------------------------------------------------------
static void TestEnableDisableClientState() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnableClientState(GL_VERTEX_ARRAY) không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_VERTEX_ARRAY) == GL_TRUE,
                "glIsEnabled(GL_VERTEX_ARRAY) == TRUE sau khi enable");
    TGLMT_CHECK(c.legacy.arrayPointers[0].enabled,
                "state client array: VERTEX_ARRAY đang bật");

    glDisableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDisableClientState(GL_VERTEX_ARRAY) không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_VERTEX_ARRAY) == GL_FALSE,
                "glIsEnabled(GL_VERTEX_ARRAY) == FALSE sau khi disable");
    TGLMT_CHECK(!c.legacy.arrayPointers[0].enabled,
                "state client array: VERTEX_ARRAY đã tắt");

    // Cả 8 giá trị hợp lệ (spec 4.2C p.61) đều được chấp nhận.
    const GLenum kAll[] = {GL_VERTEX_ARRAY,   GL_NORMAL_ARRAY,
                           GL_COLOR_ARRAY,    GL_SECONDARY_COLOR_ARRAY,
                           GL_INDEX_ARRAY,    GL_EDGE_FLAG_ARRAY,
                           GL_FOG_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY};
    for (GLenum a : kAll) {
        glEnableClientState(a);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "mảng client hợp lệ được chấp nhận");
        glDisableClientState(a);
        TGLMT_CHECK(glGetError() == GL_NO_ERROR, "mảng client hợp lệ: disable OK");
    }

    // Enum sai ⇒ GL_INVALID_ENUM (không đổi state).
    glEnableClientState(0x1234);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glEnableClientState(0x1234) ⇒ GL_INVALID_ENUM");
    glDisableClientState(0x1234);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glDisableClientState(0x1234) ⇒ GL_INVALID_ENUM");
    TGLMT_CHECK(!c.legacy.arrayPointers[0].enabled,
                "enum sai ⇒ state client array KHÔNG đổi");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. 8 lệnh *Pointer: kiểm tra lỗi + state được ghi đúng.
// ---------------------------------------------------------------------------
static void TestPointerValidation() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const float dummy[8] = {0};
    // pointer NULL hợp lệ (spec không sinh lỗi cho con trỏ NULL).
    glVertexPointer(2, GL_FLOAT, 0, nullptr);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glVertexPointer với pointer NULL không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[0].size == 2 &&
                    c.legacy.arrayPointers[0].type == GL_FLOAT &&
                    c.legacy.arrayPointers[0].stride == 0 &&
                    c.legacy.arrayPointers[0].pointer == nullptr,
                "state vertex pointer được ghi đúng");

    // size sai ⇒ INVALID_VALUE.
    glVertexPointer(5, GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glVertexPointer(5,...) ⇒ GL_INVALID_VALUE");
    glColorPointer(2, GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glColorPointer(2,...) ⇒ GL_INVALID_VALUE (size phải 3 hoặc 4)");
    glSecondaryColorPointer(2, GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glSecondaryColorPointer(2,...) ⇒ GL_INVALID_VALUE");

    // type sai ⇒ INVALID_ENUM.
    glVertexPointer(3, 0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glVertexPointer với type lạ ⇒ GL_INVALID_ENUM");
    glNormalPointer(0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glNormalPointer với type lạ ⇒ GL_INVALID_ENUM");
    glIndexPointer(0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glIndexPointer với type lạ ⇒ GL_INVALID_ENUM");
    glFogCoordPointer(0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glFogCoordPointer với type lạ ⇒ GL_INVALID_ENUM");
    glTexCoordPointer(4, 0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glTexCoordPointer với type lạ ⇒ GL_INVALID_ENUM");

    // stride < 0 ⇒ INVALID_VALUE.
    glVertexPointer(3, GL_FLOAT, -1, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glVertexPointer stride < 0 ⇒ GL_INVALID_VALUE");
    glEdgeFlagPointer(-4, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glEdgeFlagPointer stride < 0 ⇒ GL_INVALID_VALUE");

    // Size hợp lệ theo Table 2.5: TexCoord 1..4, Normal 3 (không có size),
    // Index 1, FogCoord 1, EdgeFlag 1.
    glTexCoordPointer(1, GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glTexCoordPointer(1,...) hợp lệ");
    glNormalPointer(GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glNormalPointer hợp lệ");
    TGLMT_CHECK(c.legacy.arrayPointers[1].size == 3,
                "normal array luôn có 3 thành phần");
    glIndexPointer(GL_UNSIGNED_SHORT, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glIndexPointer hợp lệ");
    glFogCoordPointer(GL_FLOAT, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glFogCoordPointer hợp lệ");
    glEdgeFlagPointer(0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEdgeFlagPointer hợp lệ");
    TGLMT_CHECK(c.legacy.arrayPointers[5].size == 1,
                "edge flag array luôn có 1 thành phần");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. glClientActiveTexture
// ---------------------------------------------------------------------------
static void TestClientActiveTexture() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glClientActiveTexture(GL_TEXTURE0);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClientActiveTexture(GL_TEXTURE0) không lỗi");
    TGLMT_CHECK(c.legacy.clientActiveTexture == 0, "client active texture = 0");

    glClientActiveTexture(GL_TEXTURE1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glClientActiveTexture(GL_TEXTURE1) không lỗi");
    TGLMT_CHECK(c.legacy.clientActiveTexture == 1, "client active texture = 1");

    glClientActiveTexture(9999);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glClientActiveTexture(9999) ⇒ GL_INVALID_ENUM");
    TGLMT_CHECK(c.legacy.clientActiveTexture == 1,
                "giá trị sai ⇒ client active texture KHÔNG đổi");

    // Ngoài dải MAX_TEXTURE_COORDS (8 unit) ⇒ INVALID_ENUM.
    glClientActiveTexture(GL_TEXTURE0 + 8);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glClientActiveTexture(GL_TEXTURE0+8) ⇒ GL_INVALID_ENUM");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Lệnh client-state bị CẤM trong glBegin/glEnd (spec 4.2C p.51).
// ---------------------------------------------------------------------------
static void TestForbiddenInsideBeginEnd() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(GL_TRIANGLES);
    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glEnableClientState trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    TGLMT_CHECK(!c.legacy.arrayPointers[0].enabled,
                "lệnh bị cấm KHÔNG được thực thi (state không đổi)");
    glDisableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glDisableClientState trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glVertexPointer trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glClientActiveTexture(GL_TEXTURE1);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glClientActiveTexture trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glInterleavedArrays(GL_V3F, 0, nullptr);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glInterleavedArrays trong glBegin/glEnd ⇒ GL_INVALID_OPERATION");
    glEnd();
    DrainErrors();
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5. ĐƯỜNG DRAW THẬT: glDrawArrays phải đọc đỉnh từ client array.
//    (spec 4.2C p.66: DrawArrays tương đương Begin(mode); ArrayElement(i); End())
// ---------------------------------------------------------------------------
static void TestDrawArraysConsumesClientArray() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const std::vector<float> verts = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    glVertexPointer(3, GL_FLOAT, 0, verts.data());
    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "thiết lập client array không lỗi");
    TGLMT_CHECK(c.legacy.arrayUsesClientMemory,
                "arrayUsesClientMemory = true (không có VBO nào bind)");

    const size_t traceBefore = c.device->drawTrace().size();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDrawArrays không lỗi");

    // Draw path ĐÃ đọc 3 đỉnh từ client array.
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayActive(),
                "đường draw dùng client array");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayVerticesFetched() == 3,
                "đường draw đọc 3 đỉnh từ client array");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayUsedClientMemory(),
                "nguồn dữ liệu = client memory");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayDrew(),
                "draw client array đã phát draw");
    TGLMT_CHECK(c.legacy.vertexCount == 3, "vertex buffer chứa 3 đỉnh");
    TGLMT_CHECK(c.legacy.indexCount == 3, "index buffer chứa 3 index (1 tam giác)");

    // Nội dung vertex buffer = đúng dữ liệu client array.
    const float* v = VertexFloats(c);
    TGLMT_CHECK(v != nullptr, "đọc được vertex buffer");
    if (v) {
        TGLMT_CHECK(Near(v[0], 0.0f) && Near(v[1], 0.0f) && Near(v[2], 0.0f),
                    "đỉnh 0 = (0,0,0) — đọc từ client array");
        TGLMT_CHECK(Near(v[kFloatPerVertex + 0], 1.0f) &&
                        Near(v[kFloatPerVertex + 1], 0.0f),
                    "đỉnh 1 = (1,0,·) — đọc từ client array");
        TGLMT_CHECK(Near(v[2 * kFloatPerVertex + 0], 0.0f) &&
                        Near(v[2 * kFloatPerVertex + 1], 1.0f),
                    "đỉnh 2 = (0,1,·) — đọc từ client array");
    }

    // Trace của backend Null: có 1 draw mới (indexed, Triangle, 3 index).
    const auto& trace = c.device->drawTrace();
    TGLMT_CHECK(trace.size() == traceBefore + 1,
                "glDrawArrays phát đúng 1 draw vào trace");
    if (trace.size() == traceBefore + 1) {
        const metal::DrawTrace& t = trace.back();
        TGLMT_CHECK(t.prim == metal::PrimitiveType::Triangle, "trace: primitive = Triangle");
        TGLMT_CHECK(t.count == 3, "trace: 3 index");
        TGLMT_CHECK(t.indexed, "trace: draw có index");
    }
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5b. glDrawElements: index lấy từ client memory, đỉnh lấy từ client array.
// ---------------------------------------------------------------------------
static void TestDrawElementsConsumesClientArray() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const std::vector<float> verts = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    const GLushort idx[6] = {0, 1, 2, 0, 2, 3};
    glVertexPointer(3, GL_FLOAT, 0, verts.data());
    glEnableClientState(GL_VERTEX_ARRAY);
    DrainErrors();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, idx);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDrawElements không lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayIndicesFetched() == 6,
                "đọc 6 index");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayVerticesFetched() == 6,
                "đọc 6 đỉnh (1 đỉnh cho mỗi index)");
    TGLMT_CHECK(c.legacy.vertexCount == 6, "vertex buffer chứa 6 đỉnh");
    const float* v = VertexFloats(c);
    TGLMT_CHECK(v != nullptr, "đọc được vertex buffer");
    if (v) {
        // Index 2 → đỉnh (1,1,0); index 5 → đỉnh (0,1,0).
        TGLMT_CHECK(Near(v[2 * kFloatPerVertex + 0], 1.0f) &&
                        Near(v[2 * kFloatPerVertex + 1], 1.0f),
                    "đỉnh theo index 2 = (1,1,·)");
        TGLMT_CHECK(Near(v[5 * kFloatPerVertex + 0], 0.0f) &&
                        Near(v[5 * kFloatPerVertex + 1], 1.0f),
                    "đỉnh theo index 5 = (0,1,·)");
    }
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6. Quy tắc chuyển đổi kiểu dữ liệu.
//    • VertexPointer = "cast"  (Table 2.5, PDF p.60) ⇒ 100 (SHORT) ⇒ 100.0
//    • ColorPointer  = "normalize" ⇒ UNSIGNED_BYTE 255 ⇒ 1.0, 128 ⇒ 128/255
//    • NormalPointer = "normalize" ⇒ BYTE 127 ⇒ 1.0, -128 ⇒ -1.0
//      (Table 2.9 GL 2.1 PDF p.73: byte (2c+1)/(2^8−1), ubyte c/(2^8−1))
// ---------------------------------------------------------------------------
static void TestTypeConversion() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Vertex: GL_SHORT ⇒ CAST (không chuẩn hoá).
    const GLshort vshort[3] = {100, 200, 300};
    glVertexPointer(3, GL_SHORT, 0, vshort);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_POINTS, 0, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "draw với vertex array kiểu SHORT không lỗi");
    const float* v = VertexFloats(c);
    TGLMT_CHECK(v != nullptr, "đọc được vertex buffer");
    if (v) {
        TGLMT_CHECK(Near(v[0], 100.0f) && Near(v[1], 200.0f) && Near(v[2], 300.0f),
                    "VertexPointer SHORT = CAST (100,200,300 — KHÔNG chuẩn hoá)");
    }

    // Color: GL_UNSIGNED_BYTE ⇒ NORMALIZE [0,1].
    const GLubyte colors[4] = {255, 128, 0, 255};
    const GLshort cverts[3] = {0, 0, 0};
    glDisableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_SHORT, 0, cverts);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glDrawArrays(GL_POINTS, 0, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "draw với color array không lỗi");
    v = VertexFloats(c);
    if (v) {
        TGLMT_CHECK(Near(v[8], 1.0f), "ColorPointer UNSIGNED_BYTE 255 ⇒ 1.0");
        TGLMT_CHECK(Near(v[9], 128.0f / 255.0f), "ColorPointer UNSIGNED_BYTE 128 ⇒ 128/255");
        TGLMT_CHECK(Near(v[10], 0.0f), "ColorPointer UNSIGNED_BYTE 0 ⇒ 0.0");
        TGLMT_CHECK(Near(v[11], 1.0f), "ColorPointer UNSIGNED_BYTE alpha 255 ⇒ 1.0");
    }

    // Normal: GL_BYTE ⇒ NORMALIZE [-1,1] (Table 2.9: (2c+1)/(2^8−1)).
    const GLbyte norm[3] = {127, -128, 0};
    glDisableClientState(GL_COLOR_ARRAY);
    glNormalPointer(GL_BYTE, 0, norm);
    glEnableClientState(GL_NORMAL_ARRAY);
    glDrawArrays(GL_POINTS, 0, 1);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "draw với normal array không lỗi");
    v = VertexFloats(c);
    if (v) {
        TGLMT_CHECK(Near(v[4], 1.0f), "NormalPointer BYTE 127 ⇒ 1.0");
        TGLMT_CHECK(Near(v[5], -1.0f), "NormalPointer BYTE -128 ⇒ -1.0");
        // Table 2.9 (GL 2.1 PDF p.73): byte → (2c+1)/(2^8−1) ⇒ byte 0 = 1/255.
        TGLMT_CHECK(Near(v[6], 1.0f / 255.0f),
                    "NormalPointer BYTE 0 ⇒ 1/255 (Table 2.9: (2c+1)/(2^8−1))");
    }
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 7. glInterleavedArrays (Table 2.8, PDF p.74).
// ---------------------------------------------------------------------------
static void TestInterleavedArrays() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const float dummy[64] = {0};
    const uint8_t* base = (const uint8_t*)dummy;

    // GL_V3F: chỉ vertex, stride mặc định = 3 float = 12 byte.
    glInterleavedArrays(GL_V3F, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInterleavedArrays(GL_V3F) không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[0].enabled, "vertex array được bật");
    TGLMT_CHECK(c.legacy.arrayPointers[0].size == 3 &&
                    c.legacy.arrayPointers[0].type == GL_FLOAT &&
                    c.legacy.arrayPointers[0].stride == 12 &&
                    c.legacy.arrayPointers[0].pointer == dummy,
                "V3F: vertex = (3, FLOAT, stride 12, ptr = base)");
    TGLMT_CHECK(!c.legacy.arrayPointers[1].enabled, "V3F: normal array tắt");
    TGLMT_CHECK(!c.legacy.arrayPointers[2].enabled, "V3F: color array tắt");
    TGLMT_CHECK(!c.legacy.arrayPointers[7].enabled, "V3F: texture coord array tắt");

    // GL_C4UB_V3F: color (4 × ubyte) + vertex (3 float); c = 4 byte.
    glInterleavedArrays(GL_C4UB_V3F, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInterleavedArrays(GL_C4UB_V3F) không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[2].enabled &&
                    c.legacy.arrayPointers[2].size == 4 &&
                    c.legacy.arrayPointers[2].type == GL_UNSIGNED_BYTE &&
                    c.legacy.arrayPointers[2].stride == 16 &&
                    c.legacy.arrayPointers[2].pointer == dummy,
                "C4UB_V3F: color = (4, UBYTE, stride 16, ptr = base)");
    TGLMT_CHECK(c.legacy.arrayPointers[0].pointer == base + 4,
                "C4UB_V3F: vertex ở offset c = 4 byte");
    TGLMT_CHECK(c.legacy.arrayPointers[0].stride == 16,
                "C4UB_V3F: stride = c + 3f = 16 byte");

    // GL_T2F_C4UB_V3F: tex(2f) + color(4×ubyte) + vertex(3f).
    glInterleavedArrays(GL_T2F_C4UB_V3F, 0, dummy);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glInterleavedArrays(GL_T2F_C4UB_V3F) không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[7].enabled &&
                    c.legacy.arrayPointers[7].size == 2 &&
                    c.legacy.arrayPointers[7].type == GL_FLOAT &&
                    c.legacy.arrayPointers[7].stride == 24 &&
                    c.legacy.arrayPointers[7].pointer == dummy,
                "T2F_C4UB_V3F: texcoord = (2, FLOAT, stride 24, ptr = base)");
    TGLMT_CHECK(c.legacy.arrayPointers[2].pointer == base + 8,
                "T2F_C4UB_V3F: color ở offset 2f = 8 byte");
    TGLMT_CHECK(c.legacy.arrayPointers[0].pointer == base + 12,
                "T2F_C4UB_V3F: vertex ở offset 2f + c = 12 byte");
    TGLMT_CHECK(c.legacy.arrayPointers[0].stride == 24,
                "T2F_C4UB_V3F: stride = c + 5f = 24 byte");
    TGLMT_CHECK(!c.legacy.arrayPointers[1].enabled, "T2F_C4UB_V3F: normal array tắt");

    // Format sai ⇒ INVALID_ENUM; stride < 0 ⇒ INVALID_VALUE.
    glInterleavedArrays(0x1234, 0, dummy);
    TGLMT_CHECK(glGetError() == 0x0500 /*GL_INVALID_ENUM*/,
                "glInterleavedArrays với format lạ ⇒ GL_INVALID_ENUM");
    glInterleavedArrays(GL_V3F, -1, dummy);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glInterleavedArrays stride < 0 ⇒ GL_INVALID_VALUE");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 8. glArrayElement: trong glBegin/glEnd sinh đỉnh; i < 0 ⇒ INVALID_VALUE.
// ---------------------------------------------------------------------------
static void TestArrayElement() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const std::vector<float> verts = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
    };
    glVertexPointer(3, GL_FLOAT, 0, verts.data());
    glEnableClientState(GL_VERTEX_ARRAY);
    DrainErrors();

    glBegin(GL_TRIANGLES);
    glArrayElement(0);
    glArrayElement(1);
    glArrayElement(2);
    glEnd();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glArrayElement trong Begin/End không lỗi");
    TGLMT_CHECK(c.legacy.immediateVertices.size() == 3,
                "3 lần glArrayElement ⇒ 3 đỉnh trong batch");
    TGLMT_CHECK(c.legacy.immediatePrimitives.size() == 1,
                "3 đỉnh ⇒ 1 tam giác");
    if (c.legacy.immediateVertices.size() == 3) {
        const LegacyVertex& v0 = c.legacy.immediateVertices[0];
        const LegacyVertex& v2 = c.legacy.immediateVertices[2];
        TGLMT_CHECK(Near(v0.pos[0], 0.0f) && Near(v0.pos[1], 0.0f),
                    "đỉnh 0 lấy từ phần tử 0 của client array");
        TGLMT_CHECK(Near(v2.pos[0], 0.0f) && Near(v2.pos[1], 1.0f),
                    "đỉnh 2 lấy từ phần tử 2 của client array");
    }
    TGLMT_CHECK(c.legacy.arrayElement == 2,
                "arrayElement = chỉ số phần tử cuối cùng");

    glArrayElement(-1);
    TGLMT_CHECK(glGetError() == 0x0501 /*GL_INVALID_VALUE*/,
                "glArrayElement(-1) ⇒ GL_INVALID_VALUE");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 9. Display list: glEnableClientState phải được ghi vào list.
// ---------------------------------------------------------------------------
static void TestDisplayListRecording() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const GLuint list = 1;
    glNewList(list, GL_COMPILE);
    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnableClientState trong display list không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_VERTEX_ARRAY) == GL_FALSE,
                "mode GL_COMPILE: lệnh KHÔNG được thực thi ngay");
    glEndList();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEndList không lỗi");

    glCallList(list);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glCallList không lỗi");
    TGLMT_CHECK(glIsEnabled(GL_VERTEX_ARRAY) == GL_TRUE,
                "replay display list: lệnh enable ĐƯỢC thực thi");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 10. Client memory vs buffer object (spec 4.2C p.59).
//     ARRAY_BUFFER đang bind ⇒ pointer là byte offset trong buffer đó.
// ---------------------------------------------------------------------------
static void TestBufferObjectBackedArray() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const float verts[9] = {
        2.0f, 0.0f, 0.0f,
        3.0f, 0.0f, 0.0f,
        2.0f, 1.0f, 0.0f,
    };
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, 0x88E4 /*GL_STATIC_DRAW*/);
    // pointer = byte offset 0 trong buffer (spec §2.9.6).
    glVertexPointer(3, GL_FLOAT, 0, (const void*)0);
    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "thiết lập array trỏ vào buffer không lỗi");
    TGLMT_CHECK(c.legacy.arrayPointers[0].buffer == buf,
                "state nhớ buffer object đang bind");
    TGLMT_CHECK(!c.legacy.arrayUsesClientMemory,
                "array trỏ vào VBO ⇒ KHÔNG phải client memory");

    glDrawArrays(GL_TRIANGLES, 0, 3);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glDrawArrays với VBO không lỗi");
    TGLMT_CHECK(tglmt::gl::LegacyClientArrayVerticesFetched() == 3,
                "đọc 3 đỉnh từ buffer object");
    TGLMT_CHECK(!tglmt::gl::LegacyClientArrayUsedClientMemory(),
                "nguồn dữ liệu = buffer object (không phải client memory)");
    const float* v = VertexFloats(c);
    TGLMT_CHECK(v != nullptr, "đọc được vertex buffer");
    if (v) {
        TGLMT_CHECK(Near(v[0], 2.0f) && Near(v[1], 0.0f),
                    "đỉnh 0 đọc từ VBO = (2,0,·)");
        TGLMT_CHECK(Near(v[2 * kFloatPerVertex + 1], 1.0f),
                    "đỉnh 2 đọc từ VBO = (2,1,·)");
    }
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 11. Core Profile: các entry point này không tồn tại ⇒ INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestCoreProfileGate() {
    TGLMTContext* ctx = CoreCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glEnableClientState(GL_VERTEX_ARRAY);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "Core profile: glEnableClientState ⇒ GL_INVALID_OPERATION");
    glVertexPointer(3, GL_FLOAT, 0, nullptr);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "Core profile: glVertexPointer ⇒ GL_INVALID_OPERATION");
    glClientActiveTexture(GL_TEXTURE0);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "Core profile: glClientActiveTexture ⇒ GL_INVALID_OPERATION");
    glInterleavedArrays(GL_V3F, 0, nullptr);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "Core profile: glInterleavedArrays ⇒ GL_INVALID_OPERATION");
    glArrayElement(0);
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "Core profile: glArrayElement ⇒ GL_INVALID_OPERATION");
    tglmtDestroyContext(ctx);
}

int main() {
    TestEnableDisableClientState();
    TestPointerValidation();
    TestClientActiveTexture();
    TestForbiddenInsideBeginEnd();
    TestDrawArraysConsumesClientArray();
    TestDrawElementsConsumesClientArray();
    TestTypeConversion();
    TestInterleavedArrays();
    TestArrayElement();
    TestDisplayListRecording();
    TestBufferObjectBackedArray();
    TestCoreProfileGate();
    printf("test_legacy_clientarrays: done\n");
    return TGLMT_RESULT;
}
