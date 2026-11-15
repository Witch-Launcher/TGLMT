// gl_legacy_vertex.cpp — OpenGL immediate mode + current vertex attributes.
//
// Spec: OpenGL 1.0–2.1 §2.6 (immediate mode), §2.7 (current vertex
// attributes), §5 (primitive assembly). Giữ lại trong Compatibility Profile.
//
// Phạm vi của file này:
//  - current vertex attributes (glColor*, glNormal*, glTexCoord*,
//    glMultiTexCoord*, glSecondaryColor*, glIndex*, glEdgeFlag*);
//  - glBegin / glEnd + gom đỉnh + ghép primitive theo đúng thứ tự của spec;
//  - bảng lệnh bị CẤM trong glBegin/glEnd.
//
// CHƯA LÀM (xem PROGRESS.md): nối primitive đã ghép vào Metal draw path ⇒
// hiện glEnd chỉ ghép primitive, chưa rasterize. Trạng thái: PARTIALLY_IMPLEMENTED.
//
// Lưu ý về thứ tự ghép GL_QUAD_STRIP (spec GL 1.4 §2.6): quad thứ i dùng 4 đỉnh
// (2i, 2i+1, 2i+3, 2i+2) — nếu dùng (2i, 2i+1, 2i+2, 2i+3) thì WINDING bị đảo.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

// Khai báo trước: glEnd dùng AssemblePrimitives.
void AssemblePrimitives(LegacyState& st);

// ---------------------------------------------------------------------------
// Display list (GL 2.1 §5.4) — hook "additive".
// Định nghĩa ở src/gl/gl_legacy_displaylist.cpp.
//
// File này SỞ HỮU current vertex attributes + glBegin/glEnd ⇒ đây là nơi
// duy nhất cần gắn hook cho:
//   • current color / normal / texcoord / secondary color (CLIENT state:
//     chụp lúc GHI — spec p.242) + áp dụng bản sao "compile" khi đang biên
//     dịch (mode GL_COMPILE ⇒ lệnh KHÔNG được thực thi);
//   • khối hình học glBegin..glEnd (ghi thay vì vẽ khi mode = GL_COMPILE).
// ---------------------------------------------------------------------------
void LegacySetCurrentColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void LegacySetCurrentNormal(GLfloat x, GLfloat y, GLfloat z);
void LegacySetCurrentTexCoord(GLuint unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void LegacySetCurrentSecondaryColor(GLfloat r, GLfloat g, GLfloat b);
void LegacyCurrentColor(GLfloat out[4]);
void LegacyCurrentNormal(GLfloat out[3]);
void LegacyCurrentTexCoord(GLuint unit, GLfloat out[4]);
void LegacyCurrentSecondaryColor(GLfloat out[3]);
bool LegacyIsRecording();
bool LegacyCompileAndExecute();
void LegacyRecordGeometryBlock(GLenum beginMode);

// GL_TEXTURE0 = 0x84C0. TGLMT hỗ trợ 8 unit kiểu legacy (MAX_TEXTURE_UNITS
// của GL 1.3 là 2, driver đời sau đưa lên 8; ta để 8 và ghi rõ).
static constexpr GLenum kLegacyTexture0 = 0x84C0;

// Đổi texture unit hiện tại (glActiveTexture).
static void LegacyActiveTexture(GLenum target) {
    Context& c = Context::Current();
    if (target < kLegacyTexture0 || target > kLegacyTexture0 + 7) {
        c.errors.Record(0x0501);  // INVALID_VALUE
        return;
    }
    c.legacy.activeTextureUnit = (GLuint)(target - kLegacyTexture0);
    // Đồng bộ với texture unit của đường modern.
    glActiveTexture(target);
}

// glMultiTexCoord* ghi vào unit của TARGET và KHÔNG đổi unit đang hoạt động
// (spec: chỉ glActiveTexture mới đổi active texture unit).
static bool LegacyMultiTexCoordUnit(GLenum target, GLuint& unit) {
    Context& c = Context::Current();
    if (target < kLegacyTexture0 || target > kLegacyTexture0 + 7) {
        c.errors.Record(0x0501);
        return false;
    }
    unit = (GLuint)(target - kLegacyTexture0);
    (void)c;
    return true;
}

// ---- Primitive mode hợp lệ cho glBegin -----------------------------------
static bool ValidBeginMode(GLenum m) {
    switch (m) {
        case 0x0000:  // GL_POINTS
        case 0x0001:  // GL_LINES
        case 0x0002:  // GL_LINE_LOOP
        case 0x0003:  // GL_LINE_STRIP
        case 0x0004:  // GL_TRIANGLES
        case 0x0005:  // GL_TRIANGLE_STRIP
        case 0x0006:  // GL_TRIANGLE_FAN
        case 0x0007:  // GL_QUADS
        case 0x0008:  // GL_QUAD_STRIP
        case 0x0009:  // GL_POLYGON
            return true;
        default: return false;
    }
}

// ---- glBegin / glEnd -------------------------------------------------------
void glBegin(GLenum mode) {
    Context& c = Context::Current();
    // Nested glBegin ⇒ INVALID_OPERATION (spec GL 1.4 §2.6).
    if (c.legacy.inBeginEnd) { c.errors.Record(0x0502); return; }
    if (!ValidBeginMode(mode)) { c.errors.Record(0x0501); return; }  // INVALID_VALUE
    c.legacy.inBeginEnd = true;
    c.legacy.beginMode = mode;
    c.legacy.ResetImmediate();
}

void glEnd() {
    Context& c = Context::Current();
    if (!c.legacy.inBeginEnd) { c.errors.Record(0x0502); return; }  // glEnd lạ
    c.legacy.inBeginEnd = false;
    // Ghép primitive.
    AssemblePrimitives(c.legacy);
    // --- Display list (GL 2.1 §5.4, spec p.242) ---
    // Đang biên dịch display list ⇒ khối hình học phải được GHI vào list.
    //   • mode = GL_COMPILE           ⇒ lệnh KHÔNG được thực thi ⇒ KHÔNG vẽ.
    //   • mode = GL_COMPILE_AND_EXECUTE ⇒ vừa ghi vừa thực thi ⇒ vẽ luôn.
    if (gl::LegacyIsRecording()) {
        gl::LegacyRecordGeometryBlock(c.legacy.beginMode);
        if (!gl::LegacyCompileAndExecute()) {
            // Dọn batch (không có draw nào được phát ra).
            c.legacy.ResetImmediate();
            return;
        }
    }
    // PHÁT RA (fixed-function draw path). Spec GL 2.1 §2.6: glBegin/glEnd
    // tạo ra một primitive và RENDER nó ngay khi glEnd trả về. Không có
    // bước này thì app chỉ dùng immediate mode sẽ không vẽ gì cả.
    //
    // Trước khi có đường draw, primitive chỉ nằm trong
    // Context::legacy.immediatePrimitives và chỉ được đẩy lên GPU khi có
    // draw kế tiếp / glClear — sai semantics.
    gl::LegacyDrawImmediate(c);
}

// ---- Ghép primitive theo spec ---------------------------------------------
// Kết quả: immediateVertices chứa đỉnh, immediatePrimitives chứa primitive đã
// triangulate — Metal chỉ có GL_POINTS / GL_LINES / GL_TRIANGLES.
void AssemblePrimitives(LegacyState& st) {
    const size_t n = st.immediateVertices.size();
    auto push = [&](GLenum mode, uint32_t a, uint32_t b, uint32_t c, uint32_t count) {
        LegacyPrimitive p;
        p.mode = mode;
        p.idx[0] = a; p.idx[1] = b; p.idx[2] = c; p.idx[3] = c;
        p.count = count;
        st.immediatePrimitives.push_back(p);
    };
    switch (st.beginMode) {
        case 0x0000:  // GL_POINTS
            for (size_t i = 0; i < n; ++i) push(0x0000, (uint32_t)i, 0, 0, 1);
            break;
        case 0x0001:  // GL_LINES
            for (size_t i = 0; i + 1 < n; i += 2)
                push(0x0001, (uint32_t)i, (uint32_t)(i + 1), 0, 2);
            break;
        case 0x0002:  // GL_LINE_LOOP — n-1 cạnh + 1 cạnh khép
            if (n >= 2) {
                for (size_t i = 0; i + 1 < n; ++i)
                    push(0x0001, (uint32_t)i, (uint32_t)(i + 1), 0, 2);
                push(0x0001, (uint32_t)(n - 1), 0, 0, 2);
            }
            break;
        case 0x0003:  // GL_LINE_STRIP
            for (size_t i = 0; i + 1 < n; ++i)
                push(0x0001, (uint32_t)i, (uint32_t)(i + 1), 0, 2);
            break;
        case 0x0004:  // GL_TRIANGLES
            for (size_t i = 0; i + 2 < n; i += 3)
                push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 2), 3);
            break;
        case 0x0005:  // GL_TRIANGLE_STRIP — winding đảo ở tam giác chẵn
            for (size_t i = 0; i + 2 < n; ++i) {
                if (i & 1)
                    push(0x0004, (uint32_t)(i + 1), (uint32_t)i, (uint32_t)(i + 2), 3);
                else
                    push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 2), 3);
            }
            break;
        case 0x0006:  // GL_TRIANGLE_FAN
            for (size_t i = 1; i + 1 < n; ++i)
                push(0x0004, 0, (uint32_t)i, (uint32_t)(i + 1), 3);
            break;
        case 0x0007:  // GL_QUADS → (v0,v1,v2) + (v0,v2,v3)
            for (size_t i = 0; i + 3 < n; i += 4) {
                push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 2), 3);
                push(0x0004, (uint32_t)i, (uint32_t)(i + 2), (uint32_t)(i + 3), 3);
            }
            break;
        case 0x0008:  // GL_QUAD_STRIP → quad i = (2i, 2i+1, 2i+3, 2i+2)
            for (size_t i = 0; i + 3 < n; i += 2) {
                push(0x0004, (uint32_t)i, (uint32_t)(i + 1), (uint32_t)(i + 3), 3);
                push(0x0004, (uint32_t)i, (uint32_t)(i + 3), (uint32_t)(i + 2), 3);
            }
            break;
        case 0x0009:  // GL_POLYGON → fan từ đỉnh 0
            for (size_t i = 1; i + 1 < n; ++i)
                push(0x0004, 0, (uint32_t)i, (uint32_t)(i + 1), 3);
            break;
        default: break;
    }
}

// ---- Vertex (glVertex*) — chỉ hợp lệ TRONG glBegin/glEnd -------------------
static void AppendVertex(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    Context& c = Context::Current();
    if (!c.legacy.inBeginEnd) { c.errors.Record(0x0502); return; }  // INVALID_OPERATION
    LegacyVertex vtx;
    vtx.pos[0] = x; vtx.pos[1] = y; vtx.pos[2] = z; vtx.pos[3] = w;
    // Client state (current color/normal/texcoord/secondary color) được CHỤP
    // lúc ghi (spec GL 2.1 §5.4, p.242: "the client state in effect at that
    // time applies to the command"). Khi đang biên dịch display list, đỉnh
    // phải dùng bản sao "compile" (lệnh trong list chưa được thực thi).
    gl::LegacyCurrentColor(vtx.color);
    gl::LegacyCurrentNormal(vtx.normal);
    gl::LegacyCurrentSecondaryColor(vtx.secondaryColor);
    // Texture unit 0 và unit 1 (multi-texture) lấy từ current state.
    gl::LegacyCurrentTexCoord(0, vtx.texCoord);
    gl::LegacyCurrentTexCoord(1, vtx.texCoord1);
    vtx.index = c.legacy.currentIndex;
    vtx.edgeFlag = c.legacy.edgeFlag;
    c.legacy.immediateVertices.push_back(vtx);
}

void glVertex2f(GLfloat x, GLfloat y) { AppendVertex(x, y, 0.0f, 1.0f); }
void glVertex3f(GLfloat x, GLfloat y, GLfloat z) { AppendVertex(x, y, z, 1.0f); }
void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) { AppendVertex(x, y, z, w); }
void glVertex2d(GLdouble x, GLdouble y) { glVertex2f((GLfloat)x, (GLfloat)y); }
void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
    glVertex3f((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    glVertex4f((GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}
void glVertex2fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex2f(v[0], v[1]);
}
void glVertex3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex3f(v[0], v[1], v[2]);
}
void glVertex4fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex4f(v[0], v[1], v[2], v[3]);
}
void glVertex2dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex2d(v[0], v[1]);
}
void glVertex3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex3d(v[0], v[1], v[2]);
}
void glVertex4dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glVertex4d(v[0], v[1], v[2], v[3]);
}

// ---- Current vertex attributes --------------------------------------------
// Ghi current color: nếu đang biên dịch display list ⇒ ghi vào list (và lưu
// bản sao "compile"); nếu GL_COMPILE_AND_EXECUTE ⇒ thực thi luôn.
static void SetCurrentColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    gl::LegacySetCurrentColor(r, g, b, a);
}
void glColor3f(GLfloat r, GLfloat g, GLfloat b) { SetCurrentColor(r, g, b, 1.0f); }
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { SetCurrentColor(r, g, b, a); }
void glColor3d(GLdouble r, GLdouble g, GLdouble b) {
    SetCurrentColor((GLfloat)r, (GLfloat)g, (GLfloat)b, 1.0f);
}
void glColor4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a) {
    SetCurrentColor((GLfloat)r, (GLfloat)g, (GLfloat)b, (GLfloat)a);
}
void glColor3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentColor(v[0], v[1], v[2], 1.0f);
}
void glColor4fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentColor(v[0], v[1], v[2], v[3]);
}
void glColor3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentColor((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], 1.0f);
}
void glColor4dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentColor((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
}
// ---- Chuyển đổi thành phần màu theo Table 2.9 (spec GL 2.1 §2.14, PDF p.59)
//   ubyte  c/(2^8-1)        byte  (2c+1)/(2^8-1)
//   ushort c/(2^16-1)       short (2c+1)/(2^16-1)
//   uint   c/(2^32-1)       int   (2c+1)/(2^32-1)
// Mọi phép tính làm trong double để (a) 2c+1 không tràn int32 (c = INT_MIN)
// và (b) phép chia số 32-bit không mất chính xác (float chỉ giữ 24 bit).
static float ColorSignedByte(GLbyte v) {
    return (float)((2.0 * (double)v + 1.0) / 255.0);
}
static float ColorSignedShort(GLshort v) {
    return (float)((2.0 * (double)v + 1.0) / 65535.0);
}
static float ColorSignedInt(GLint v) {
    return (float)((2.0 * (double)v + 1.0) / 4294967295.0);
}
static float ColorUnsignedByte(GLubyte v) {
    return (float)((double)v / 255.0);
}
static float ColorUnsignedShort(GLushort v) {
    return (float)((double)v / 65535.0);
}
static float ColorUnsignedInt(GLuint v) {
    return (float)((double)v / 4294967295.0);
}

void glColor3ub(GLubyte r, GLubyte g, GLubyte b) {
    SetCurrentColor(ColorUnsignedByte(r), ColorUnsignedByte(g),
                    ColorUnsignedByte(b), 1.0f);
}
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    SetCurrentColor(ColorUnsignedByte(r), ColorUnsignedByte(g),
                    ColorUnsignedByte(b), ColorUnsignedByte(a));
}
void glColor3ubv(const GLubyte* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3ub(v[0], v[1], v[2]);
}
void glColor4ubv(const GLubyte* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4ub(v[0], v[1], v[2], v[3]);
}
void glColor3b(GLbyte r, GLbyte g, GLbyte b) {
    SetCurrentColor(ColorSignedByte(r), ColorSignedByte(g), ColorSignedByte(b),
                    1.0f);
}
void glColor4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a) {
    SetCurrentColor(ColorSignedByte(r), ColorSignedByte(g), ColorSignedByte(b),
                    ColorSignedByte(a));
}
void glColor3bv(const GLbyte* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3b(v[0], v[1], v[2]);
}
void glColor4bv(const GLbyte* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4b(v[0], v[1], v[2], v[3]);
}
void glColor3s(GLshort r, GLshort g, GLshort b) {
    SetCurrentColor(ColorSignedShort(r), ColorSignedShort(g),
                    ColorSignedShort(b), 1.0f);
}
void glColor4s(GLshort r, GLshort g, GLshort b, GLshort a) {
    SetCurrentColor(ColorSignedShort(r), ColorSignedShort(g),
                    ColorSignedShort(b), ColorSignedShort(a));
}
void glColor3sv(const GLshort* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3s(v[0], v[1], v[2]);
}
void glColor4sv(const GLshort* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4s(v[0], v[1], v[2], v[3]);
}
void glColor3i(GLint r, GLint g, GLint b) {
    SetCurrentColor(ColorSignedInt(r), ColorSignedInt(g), ColorSignedInt(b),
                    1.0f);
}
void glColor4i(GLint r, GLint g, GLint b, GLint a) {
    SetCurrentColor(ColorSignedInt(r), ColorSignedInt(g), ColorSignedInt(b),
                    ColorSignedInt(a));
}
void glColor3iv(const GLint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3i(v[0], v[1], v[2]);
}
void glColor4iv(const GLint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4i(v[0], v[1], v[2], v[3]);
}
void glColor3ui(GLuint r, GLuint g, GLuint b) {
    SetCurrentColor(ColorUnsignedInt(r), ColorUnsignedInt(g),
                    ColorUnsignedInt(b), 1.0f);
}
void glColor4ui(GLuint r, GLuint g, GLuint b, GLuint a) {
    SetCurrentColor(ColorUnsignedInt(r), ColorUnsignedInt(g),
                    ColorUnsignedInt(b), ColorUnsignedInt(a));
}
void glColor3uiv(const GLuint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3ui(v[0], v[1], v[2]);
}
void glColor4uiv(const GLuint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4ui(v[0], v[1], v[2], v[3]);
}
void glColor3us(GLushort r, GLushort g, GLushort b) {
    SetCurrentColor(ColorUnsignedShort(r), ColorUnsignedShort(g),
                    ColorUnsignedShort(b), 1.0f);
}
void glColor4us(GLushort r, GLushort g, GLushort b, GLushort a) {
    SetCurrentColor(ColorUnsignedShort(r), ColorUnsignedShort(g),
                    ColorUnsignedShort(b), ColorUnsignedShort(a));
}
void glColor3usv(const GLushort* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor3us(v[0], v[1], v[2]);
}
void glColor4usv(const GLushort* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glColor4us(v[0], v[1], v[2], v[3]);
}

// ---- Normal ----------------------------------------------------------------
static void SetCurrentNormal(GLfloat x, GLfloat y, GLfloat z) {
    gl::LegacySetCurrentNormal(x, y, z);
}
void glNormal3f(GLfloat x, GLfloat y, GLfloat z) { SetCurrentNormal(x, y, z); }
void glNormal3d(GLdouble x, GLdouble y, GLdouble z) {
    SetCurrentNormal((GLfloat)x, (GLfloat)y, (GLfloat)z);
}
void glNormal3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentNormal(v[0], v[1], v[2]);
}
void glNormal3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentNormal((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

// ---- Texture coordinates ---------------------------------------------------
// Ghi tọa độ texture hiện tại của MỘT texture unit (có hook display list:
// client state được chụp lúc ghi).
static void SetCurrentTexCoordOf(GLuint unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    gl::LegacySetCurrentTexCoord(unit, s, t, r, q);
}

// glTexCoord* ghi vào texture unit ĐANG HOẠT ĐỘNG (spec GL 1.3 §2.8.1:
// "the current texture coordinate is per texture unit"; glTexCoord* tương
// đương glMultiTexCoord* với target = unit đang hoạt động).
static void SetCurrentTexCoord(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    Context& c = Context::Current();
    SetCurrentTexCoordOf(c.legacy.activeTextureUnit, s, t, r, q);
}
void glTexCoord1f(GLfloat s) { SetCurrentTexCoord(s, 0.0f, 0.0f, 1.0f); }
void glTexCoord2f(GLfloat s, GLfloat t) { SetCurrentTexCoord(s, t, 0.0f, 1.0f); }
void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) { SetCurrentTexCoord(s, t, r, 1.0f); }
void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    SetCurrentTexCoord(s, t, r, q);
}
void glTexCoord1d(GLdouble s) { glTexCoord1f((GLfloat)s); }
void glTexCoord2d(GLdouble s, GLdouble t) { glTexCoord2f((GLfloat)s, (GLfloat)t); }
void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) {
    glTexCoord3f((GLfloat)s, (GLfloat)t, (GLfloat)r);
}
void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
    glTexCoord4f((GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q);
}
void glTexCoord1fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord1f(v[0]);
}
void glTexCoord2fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord2f(v[0], v[1]);
}
void glTexCoord3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord3f(v[0], v[1], v[2]);
}
void glTexCoord4fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord4f(v[0], v[1], v[2], v[3]);
}
void glTexCoord1dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord1f((GLfloat)v[0]);
}
void glTexCoord2dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord2f((GLfloat)v[0], (GLfloat)v[1]);
}
void glTexCoord3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord3f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}
void glTexCoord4dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glTexCoord4f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
}

// ---- Multi-texture coordinates (GL 1.3) ------------------------------------
// glMultiTexCoord* ghi vào unit của TARGET, KHÔNG đổi unit đang hoạt động.
#define TGLMT_MT1(NAME, TYPE)                                                     \
    void NAME(GLenum target, TYPE s) {                                            \
        GLuint u;                                                                 \
        if (!LegacyMultiTexCoordUnit(target, u)) return;                          \
        SetCurrentTexCoordOf(u, (GLfloat)s, 0.0f, 0.0f, 1.0f);                   \
    }
#define TGLMT_MT2(NAME, TYPE)                                                     \
    void NAME(GLenum target, TYPE s, TYPE t) {                                    \
        GLuint u;                                                                 \
        if (!LegacyMultiTexCoordUnit(target, u)) return;                          \
        SetCurrentTexCoordOf(u, (GLfloat)s, (GLfloat)t, 0.0f, 1.0f);            \
    }
#define TGLMT_MT3(NAME, TYPE)                                                     \
    void NAME(GLenum target, TYPE s, TYPE t, TYPE r) {                            \
        GLuint u;                                                                 \
        if (!LegacyMultiTexCoordUnit(target, u)) return;                          \
        SetCurrentTexCoordOf(u, (GLfloat)s, (GLfloat)t, (GLfloat)r, 1.0f);       \
    }
#define TGLMT_MT4(NAME, TYPE)                                                     \
    void NAME(GLenum target, TYPE s, TYPE t, TYPE r, TYPE q) {                   \
        GLuint u;                                                                 \
        if (!LegacyMultiTexCoordUnit(target, u)) return;                          \
        SetCurrentTexCoordOf(u, (GLfloat)s, (GLfloat)t, (GLfloat)r, (GLfloat)q); \
    }
TGLMT_MT1(glMultiTexCoord1f, GLfloat)
TGLMT_MT2(glMultiTexCoord2f, GLfloat)
TGLMT_MT3(glMultiTexCoord3f, GLfloat)
TGLMT_MT4(glMultiTexCoord4f, GLfloat)
TGLMT_MT1(glMultiTexCoord1d, GLdouble)
TGLMT_MT2(glMultiTexCoord2d, GLdouble)
TGLMT_MT3(glMultiTexCoord3d, GLdouble)
TGLMT_MT4(glMultiTexCoord4d, GLdouble)
#undef TGLMT_MT1
#undef TGLMT_MT2
#undef TGLMT_MT3
#undef TGLMT_MT4

#undef TGLMT_MULTITEXV

void glMultiTexCoord1fv(GLenum target, const GLfloat* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, v[0], 0.0f, 0.0f, 1.0f);
}
void glMultiTexCoord2fv(GLenum target, const GLfloat* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, v[0], v[1], 0.0f, 1.0f);
}
void glMultiTexCoord3fv(GLenum target, const GLfloat* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, v[0], v[1], v[2], 1.0f);
}
void glMultiTexCoord4fv(GLenum target, const GLfloat* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, v[0], v[1], v[2], v[3]);
}
void glMultiTexCoord1dv(GLenum target, const GLdouble* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, (GLfloat)v[0], 0.0f, 0.0f, 1.0f);
}
void glMultiTexCoord2dv(GLenum target, const GLdouble* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, (GLfloat)v[0], (GLfloat)v[1], 0.0f, 1.0f);
}
void glMultiTexCoord3dv(GLenum target, const GLdouble* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], 1.0f);
}
void glMultiTexCoord4dv(GLenum target, const GLdouble* v) {
    GLuint u;
    if (!LegacyMultiTexCoordUnit(target, u)) return;
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentTexCoordOf(u, (GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2], (GLfloat)v[3]);
}

// ---- Secondary color (GL 1.6) ---------------------------------------------
void glSecondaryColor3f(GLfloat r, GLfloat g, GLfloat b) {
    gl::LegacySetCurrentSecondaryColor(r, g, b);
}
void glSecondaryColor3d(GLdouble r, GLdouble g, GLdouble b) {
    glSecondaryColor3f((GLfloat)r, (GLfloat)g, (GLfloat)b);
}
void glSecondaryColor3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glSecondaryColor3f(v[0], v[1], v[2]);
}
void glSecondaryColor3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glSecondaryColor3f((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]);
}

// ---- Color index (GL 1.1) --------------------------------------------------
static void SetCurrentIndex(GLuint i) {
    Context& c = Context::Current();
    c.legacy.currentIndex = i;
}
void glIndex1i(GLint i) { SetCurrentIndex((GLuint)i); }
void glIndex1d(GLdouble i) { SetCurrentIndex((GLuint)i); }
void glIndex1iv(const GLint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentIndex((GLuint)v[0]);
}
void glIndex1dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentIndex((GLuint)v[0]);
}
void glIndex1ui(GLuint i) { SetCurrentIndex(i); }
void glIndex1uiv(const GLuint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    SetCurrentIndex(v[0]);
}
void glIndex2f(GLfloat a, GLfloat b) {
    SetCurrentIndex((GLuint)(a + b * 256.0f));
}
void glIndex2d(GLdouble a, GLdouble b) {
    SetCurrentIndex((GLuint)(a + b * 256.0));
}
void glIndex2fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex2f(v[0], v[1]);
}
void glIndex2dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex2d(v[0], v[1]);
}
void glIndex2ui(GLuint a, GLuint b) { SetCurrentIndex(a + b * 256u); }
void glIndex2uiv(const GLuint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex2ui(v[0], v[1]);
}
void glIndex3f(GLfloat a, GLfloat b, GLfloat c) {
    SetCurrentIndex((GLuint)(a + b * 256.0f + c * 65536.0f));
}
void glIndex3d(GLdouble a, GLdouble b, GLdouble c) {
    SetCurrentIndex((GLuint)(a + b * 256.0 + c * 65536.0));
}
void glIndex3fv(const GLfloat* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex3f(v[0], v[1], v[2]);
}
void glIndex3dv(const GLdouble* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex3d(v[0], v[1], v[2]);
}
void glIndex3ui(GLuint a, GLuint b, GLuint c) { SetCurrentIndex(a + b * 256u + c * 65536u); }
void glIndex3uiv(const GLuint* v) {
    if (!v) { Context::Current().errors.Record(0x0500); return; }
    glIndex3ui(v[0], v[1], v[2]);
}

// ---- Edge flag -------------------------------------------------------------
void glEdgeFlag(GLboolean flag) {
    Context& c = Context::Current();
    c.legacy.edgeFlag = flag;
}
void glEdgeFlagv(const GLboolean* flags) {
    if (!flags) { Context::Current().errors.Record(0x0500); return; }
    glEdgeFlag(flags[0]);
}

}  // namespace tglmt::gl
