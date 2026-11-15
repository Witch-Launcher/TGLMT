// gl_legacy_displaylist.cpp — DISPLAY LISTS (OpenGL Compatibility Profile).
//
// Spec: OpenGL 2.1 §5.4 "Display Lists" — glspec21.pdf PDF p.256–259
// (= spec p.242–245). Văn bản giữ nguyên trong Compatibility Profile 4.2/4.6.
//
// Trích nguyên văn (spec p.242):
//
//   "A display list is simply a group of GL commands and arguments that has
//    been stored for subsequent execution. ... Doing so causes the commands
//    within the list to be executed just as if they were given normally. The
//    only exception pertains to commands that rely upon client state. When
//    such a command is accumulated into the display list (that is, when issued,
//    not when executed), the client state in effect at that time applies to the
//    command. Only server state is affected when the command is executed. As
//    always, pointers which are passed as arguments to commands are
//    dereferenced when the command is issued."
//
// ⇒ HAI quy tắc cốt lõi (xem docs/DISPLAY_LISTS.md §3.6):
//   R2 (client state): current color / normal / texcoord / secondary color
//       được CHỤP lúc GHI (compile). Vì LegacyVertex đã resolve sẵn current
//       state thành dữ liệu đỉnh (src/gl/gl_legacy_vertex.cpp AppendVertex),
//       ta ghi đỉnh ĐÃ resolve ⇒ R2 thoả mãn.
//   R3 (server state): ma trận, viewport, blend, lighting, texture env…
//       được đọc lúc THỰC THI ⇒ chỉ ghi LỆNH, không ghi giá trị.
//
// THIẾT KẾ (docs/DISPLAY_LISTS.md §3.2, phương án PA-2 — dispatcher nằm
// trong tglmt::gl::, KHÔNG nằm ở lớp C ABI sinh):
//   • Tầng ghi: file này + hook "additive" trong src/gl/gl_legacy_vertex.cpp
//     (file sở hữu current vertex attributes + glBegin/glEnd).
//   • Các lệnh state còn lại (glEnable, glMatrixMode, glViewport, glTexEnv,
//     glLight, glMaterial…) nằm ở file KHÁC (ngoài write-scope của task
//     này) ⇒ file này CUNG CẤP API LegacyRecord… tương ứng; main agent gắn
//     hook (danh sách hook site nêu trong báo cáo cuối).
//   • Lệnh CHƯA có hook sẽ thực thi bình thường lúc đang biên dịch ⇒ đây là
//     giới hạn đã biết (S10a của docs/COMPATIBILITY_IMPLEMENTATION_PLAN.md
//     §12.3: "record the immediate-mode block, the state-changing commands and
//     the draw commands. Everything else executes immediately").
//
// GHI CHÚ VỀ CHỮ KÝ glGenLists:
//   Khronos registry (docs/khronos/gl.xml, <proto class="display list">
//   GLuint glGenLists</proto>) ⇒ `GLuint glGenLists(GLsizei range)`, trả về
//   chỉ số ĐẦU của một block chỉ số chưa dùng liên tiếp. Lớp C ABI sinh
//   (include/tglmt/gl_c_api.h:807) dùng đúng chữ ký này ⇒ không đổi được
//   (tools/gen_c_exports.py là file sinh). Spec GL 2.1 §5.4 (spec p.243):
//   "returns an integer n such that the indices n, . . . , n+s−1 are
//    previously unused ... GenLists also has the effect of creating an empty
//    display list for each of the indices n, . . . , n+s−1, so that these
//    indices all become used."
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstring>
#include <new>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

namespace {

// ---------------------------------------------------------------------------
// Opcode nội bộ của một lệnh đã ghi trong display list.
// (LegacyState::LegacyListOp::op — include/tglmt/LegacyState.h:479)
// ---------------------------------------------------------------------------
enum ListOp : GLenum {
    kOpColor = 1,           // f0..f3 = current color (client state, chụp lúc ghi)
    kOpNormal,              // f0..f2
    kOpTexCoord,            // u0 = texture unit, f0..f3
    kOpSecondaryColor,      // f0..f2
    kOpGeometry,            // u0 = nVerts, u1 = nPrims, u2 = beginMode, data = payload
    kOpMatrixMode,          // u0 = mode
    kOpLoadMatrix,          // m[16] column-major
    kOpMultMatrix,          // m[16] column-major
    kOpPushMatrix,
    kOpPopMatrix,
    kOpLoadIdentity,
    kOpEnable,              // u0 = cap
    kOpDisable,             // u0 = cap
    kOpEnableClientState,   // u0 = array
    kOpDisableClientState,  // u0 = array
    kOpShadeModel,          // u0 = mode
    kOpDepthMask,           // u0
    kOpBlendFunc,           // u0 = src, u1 = dst
    kOpCullFace,            // u0
    kOpFrontFace,           // u0
    kOpPolygonOffset,       // f0 = factor, f1 = units
    kOpViewport,            // f0..f3 = x, y, w, h
    kOpScissor,             // f0..f3 = x, y, w, h
    kOpLineWidth,           // f0
    kOpPointSize,           // f0
    kOpTexEnv,              // u0 = target, u1 = pname, u2 = số giá trị, f0..f3
    kOpTexGen,              // u0 = coord, u1 = pname, u2 = số giá trị, f0..f3
    kOpLight,               // u0 = light, u1 = pname, u2 = số giá trị, f0..f3
    kOpMaterial,            // u0 = face, u1 = pname, u2 = số giá trị, f0..f3
    kOpCallList,            // u0 = chỉ số list
    kOpCallLists,           // u0 = type, u1 = n, data = n offset (float)
};

// Số float để đóng gói 1 đỉnh / 1 primitive vào payload (xem PackVertex).
constexpr uint32_t kFloatsPerVertex = 24;
constexpr uint32_t kFloatsPerPrim = 6;

// Giới hạn độ sâu lồng nhau khi thực thi display list.
// Spec §5.4 (spec p.243): "an implementation dependent limit is placed on the
// nesting level of display lists during display list execution. This limit
// must be at least 64."
constexpr uint32_t kMaxNesting = 64;

// ---------------------------------------------------------------------------
// Phiên biên dịch (state của lệnh glNewList..glEndList)
// ---------------------------------------------------------------------------
// `ops` là buffer PENDING (transactional): spec p.242 yêu cầu "The error OUT
// OF MEMORY is generated if EndList is called and the specified display list
// cannot be stored ... GL implementations of revision 1.1 or greater insure
// that no change is made to the previous contents of the display list" ⇒ chỉ
// commit vào Context::legacy.lists khi glEndList thành công.
//
// `color/normal/secondary/texCoord` là bản sao "compile" của CLIENT state.
// Spec: "If mode is COMPILE, then commands are not executed as they are placed
// in the display list" (spec p.242) ⇒ current color KHÔNG được đổi khi đang
// biên dịch; nhưng đỉnh gom trong list phải dùng GIÁ TRỊ của lệnh color nằm
// trong list ⇒ cần một bản sao riêng (shadow).
struct CompileSession {
    bool active = false;
    GLuint list = 0;
    GLenum mode = 0;
    bool failed = false;  // hết bộ nhớ khi ghi ⇒ glEndList sẽ báo OUT_OF_MEMORY
    std::vector<LegacyState::LegacyListOp> ops;
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float normal[3] = {0.0f, 0.0f, 1.0f};
    float secondary[3] = {0.0f, 0.0f, 0.0f};
    float texCoord[8][4] = {};
};

thread_local CompileSession g_compile;
// Độ sâu replay hiện tại (chống đệ quy vô hạn + giới hạn 64).
thread_local uint32_t g_replayDepth = 0;

// Đang ghi vào display list? (nested replay KHÔNG ghi — spec p.243: "the
// CallList or CallLists, rather than those lists' constituent commands, is
// placed in the list under construction").
bool Recording() { return g_compile.active && g_replayDepth == 0; }

// Ghi 1 op vào buffer pending.
// Trả true nếu lệnh KHÔNG được thực thi (mode = GL_COMPILE) ⇒ caller phải
// return ngay. Trả false nếu lệnh phải thực thi (không ghi, hoặc ghi + thực
// thi trong GL_COMPILE_AND_EXECUTE).
bool RecordOp(LegacyState::LegacyListOp&& op) {
    if (!Recording()) return false;
    try {
        g_compile.ops.push_back(std::move(op));
    } catch (const std::bad_alloc&) {
        // Không đủ bộ nhớ khi ghi ⇒ list sẽ hỏng; glEndList sẽ báo
        // GL_OUT_OF_MEMORY và giữ nguyên list cũ (spec p.242).
        g_compile.failed = true;
        return true;
    }
    return g_compile.mode == GL_COMPILE;
}

// ---------------------------------------------------------------------------
// Đóng gói / giải nén khối hình học (glBegin..glEnd)
// ---------------------------------------------------------------------------
// Layout 1 đỉnh (24 float): pos(4) color(4) normal(3) texCoord(4)
//                              texCoord1(4) secondary(3) index(1) edgeFlag(1)
// Layout 1 primitive (6 float): mode(1) idx(4) count(1)
void PackVertex(const LegacyVertex& v, float* out) {
    for (int k = 0; k < 4; ++k) out[0 + k] = v.pos[k];
    for (int k = 0; k < 4; ++k) out[4 + k] = v.color[k];
    for (int k = 0; k < 3; ++k) out[8 + k] = v.normal[k];
    for (int k = 0; k < 4; ++k) out[11 + k] = v.texCoord[k];
    for (int k = 0; k < 4; ++k) out[15 + k] = v.texCoord1[k];
    for (int k = 0; k < 3; ++k) out[19 + k] = v.secondaryColor[k];
    out[22] = (float)v.index;
    out[23] = v.edgeFlag ? 1.0f : 0.0f;
}

void UnpackVertex(const float* in, LegacyVertex& v) {
    for (int k = 0; k < 4; ++k) v.pos[k] = in[0 + k];
    for (int k = 0; k < 4; ++k) v.color[k] = in[4 + k];
    for (int k = 0; k < 3; ++k) v.normal[k] = in[8 + k];
    for (int k = 0; k < 4; ++k) v.texCoord[k] = in[11 + k];
    for (int k = 0; k < 4; ++k) v.texCoord1[k] = in[15 + k];
    for (int k = 0; k < 3; ++k) v.secondaryColor[k] = in[19 + k];
    v.index = (GLuint)in[22];
    v.edgeFlag = in[23] != 0.0f ? 1 : 0;
}

void PackPrim(const LegacyPrimitive& p, float* out) {
    out[0] = (float)p.mode;
    for (int k = 0; k < 4; ++k) out[1 + k] = (float)p.idx[k];
    out[5] = (float)p.count;
}

void UnpackPrim(const float* in, LegacyPrimitive& p) {
    p.mode = (GLenum)in[0];
    for (int k = 0; k < 4; ++k) p.idx[k] = (uint32_t)in[1 + k];
    p.count = (uint32_t)in[5];
}

// ---------------------------------------------------------------------------
// Bảng offset của glCallLists (spec p.243 + docs/DISPLAY_LISTS.md §1.3)
// ---------------------------------------------------------------------------
// type ∈ {BYTE, UNSIGNED_BYTE, SHORT, UNSIGNED_SHORT, INT, UNSIGNED_INT,
//         FLOAT, 2_BYTES, 3_BYTES, 4_BYTES} (nhóm ListNameType của
// docs/khronos/gl.xml). Ngoài ra ⇒ GL_INVALID_ENUM.
bool ValidCallListType(GLenum type) {
    switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_2_BYTES:
        case GL_3_BYTES:
        case GL_4_BYTES:
            return true;
        default:
            return false;
    }
}

// Đọc phần tử thứ i của mảng offset, theo kiểu `type`.
// GL_FLOAT: "floating point values are truncated" (spec p.243) ⇒ cắt về 0.
GLint ReadCallListOffset(GLenum type, const void* lists, GLsizei i) {
    switch (type) {
        case GL_BYTE:
            return (GLint)((const GLbyte*)lists)[i];
        case GL_UNSIGNED_BYTE:
            return (GLint)((const GLubyte*)lists)[i];
        case GL_SHORT:
            return (GLint)((const GLshort*)lists)[i];
        case GL_UNSIGNED_SHORT:
            return (GLint)((const GLushort*)lists)[i];
        case GL_INT:
            return ((const GLint*)lists)[i];
        case GL_UNSIGNED_INT:
            return (GLint)((const GLuint*)lists)[i];
        case GL_FLOAT:
            return (GLint)((const GLfloat*)lists)[i];  // truncation về 0
        case GL_2_BYTES:
        case GL_3_BYTES:
        case GL_4_BYTES: {
            // Thuật toán nguyên văn spec p.243 (big-endian):
            //   offset ← 0; for i = 1 to b: offset ← (offset << 8) + byte
            const int b = (type == GL_2_BYTES) ? 2 : (type == GL_3_BYTES ? 3 : 4);
            const GLubyte* p = (const GLubyte*)lists + (size_t)i * (size_t)b;
            uint32_t off = 0;
            for (int k = 0; k < b; ++k) off = (off << 8) | (uint32_t)p[k];
            return (GLint)off;
        }
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// Replay
// ---------------------------------------------------------------------------
// Thực thi lại 1 khối hình học đã ghi: đưa đỉnh/primitive đã resolve về
// immediate state rồi gọi đúng đường draw fixed-function ⇒ Metal draw path
// chạy y hệt immediate mode (src/gl/gl_legacy_draw.cpp LegacyDrawImmediate).
void ReplayGeometry(const LegacyState::LegacyListOp& op) {
    Context& c = Context::Current();
    const uint32_t nv = op.u0;
    const uint32_t np = op.u1;
    const size_t need = (size_t)nv * kFloatsPerVertex + (size_t)np * kFloatsPerPrim;
    if (op.data.size() < need) return;  // payload hỏng ⇒ bỏ qua (không vẽ)
    c.legacy.ResetImmediate();
    c.legacy.beginMode = op.u2;
    const float* d = op.data.data();
    for (uint32_t i = 0; i < nv; ++i) {
        LegacyVertex v;
        UnpackVertex(d + (size_t)i * kFloatsPerVertex, v);
        c.legacy.immediateVertices.push_back(v);
    }
    const float* p = d + (size_t)nv * kFloatsPerVertex;
    for (uint32_t i = 0; i < np; ++i) {
        LegacyPrimitive pr;
        UnpackPrim(p + (size_t)i * kFloatsPerPrim, pr);
        c.legacy.immediatePrimitives.push_back(pr);
    }
    LegacyDrawImmediate(c);
}

void ReplayOp(const LegacyState::LegacyListOp& op);

void ReplayList(GLuint list, uint32_t depth);

// Thực thi 1 lệnh đã ghi. Mọi lệnh gọi lại chính hàm GL tương ứng ⇒
// "executed just as if they were given normally" (spec p.242). Cờ replay
// (g_replayDepth > 0) đảm bảo lệnh không bị ghi lần nữa.
void ReplayOp(const LegacyState::LegacyListOp& op) {
    switch (op.op) {
        case kOpColor:
            glColor4f(op.f0, op.f1, op.f2, op.f3);
            break;
        case kOpNormal:
            glNormal3f(op.f0, op.f1, op.f2);
            break;
        case kOpTexCoord:
            glMultiTexCoord4f((GLenum)(GL_TEXTURE0 + op.u0), op.f0, op.f1, op.f2, op.f3);
            break;
        case kOpSecondaryColor:
            glSecondaryColor3f(op.f0, op.f1, op.f2);
            break;
        case kOpGeometry:
            ReplayGeometry(op);
            break;
        case kOpMatrixMode:
            glMatrixMode((GLenum)op.u0);
            break;
        case kOpLoadMatrix:
            glLoadMatrixf(op.m);
            break;
        case kOpMultMatrix:
            glMultMatrixf(op.m);
            break;
        case kOpPushMatrix:
            glPushMatrix();
            break;
        case kOpPopMatrix:
            glPopMatrix();
            break;
        case kOpLoadIdentity:
            glLoadIdentity();
            break;
        case kOpEnable:
            glEnable((GLenum)op.u0);
            break;
        case kOpDisable:
            glDisable((GLenum)op.u0);
            break;
        case kOpEnableClientState:
            glEnableClientState((GLenum)op.u0);
            break;
        case kOpDisableClientState:
            glDisableClientState((GLenum)op.u0);
            break;
        case kOpShadeModel:
            glShadeModel((GLenum)op.u0);
            break;
        case kOpDepthMask:
            glDepthMask((GLboolean)op.u0);
            break;
        case kOpBlendFunc:
            glBlendFunc((GLenum)op.u0, (GLenum)op.u1);
            break;
        case kOpCullFace:
            glCullFace((GLenum)op.u0);
            break;
        case kOpFrontFace:
            glFrontFace((GLenum)op.u0);
            break;
        case kOpPolygonOffset:
            glPolygonOffset(op.f0, op.f1);
            break;
        case kOpViewport:
            glViewport((GLint)op.f0, (GLint)op.f1, (GLsizei)op.f2, (GLsizei)op.f3);
            break;
        case kOpScissor:
            glScissor((GLint)op.f0, (GLint)op.f1, (GLsizei)op.f2, (GLsizei)op.f3);
            break;
        case kOpLineWidth:
            glLineWidth(op.f0);
            break;
        case kOpPointSize:
            glPointSize(op.f0);
            break;
        case kOpTexEnv: {
            if (op.u2 >= 4) {
                float v[4] = {op.f0, op.f1, op.f2, op.f3};
                glTexEnvfv((GLenum)op.u0, (GLenum)op.u1, v);
            } else {
                glTexEnvf((GLenum)op.u0, (GLenum)op.u1, op.f0);
            }
            break;
        }
        case kOpTexGen: {
            if (op.u2 >= 4) {
                float v[4] = {op.f0, op.f1, op.f2, op.f3};
                glTexGenfv((GLenum)op.u0, (GLenum)op.u1, v);
            } else {
                glTexGenf((GLenum)op.u0, (GLenum)op.u1, op.f0);
            }
            break;
        }
        case kOpLight: {
            if (op.u2 >= 4) {
                float v[4] = {op.f0, op.f1, op.f2, op.f3};
                glLightfv((GLenum)op.u0, (GLenum)op.u1, v);
            } else {
                glLightf((GLenum)op.u0, (GLenum)op.u1, op.f0);
            }
            break;
        }
        case kOpMaterial: {
            if (op.u2 >= 4) {
                float v[4] = {op.f0, op.f1, op.f2, op.f3};
                glMaterialfv((GLenum)op.u0, (GLenum)op.u1, v);
            } else {
                glMaterialf((GLenum)op.u0, (GLenum)op.u1, op.f0);
            }
            break;
        }
        case kOpCallList:
            ReplayList(op.u0, g_replayDepth);
            break;
        case kOpCallLists: {
            const GLsizei n = (GLsizei)op.u1;
            const GLuint base = Context::Current().legacy.listBase;
            for (GLsizei i = 0; i < n; ++i) {
                const GLint off = (GLint)op.data[(size_t)i];
                const long long idx = (long long)off + (long long)base;
                if (idx <= 0) continue;  // chỉ số không hợp lệ ⇒ không có tác dụng
                ReplayList((GLuint)idx, g_replayDepth);
            }
            break;
        }
        default:
            break;  // opcode lạ ⇒ bỏ qua
    }
}

// Thực thi list `list` (spec p.242: "This causes the commands saved in the
// display list to be executed, in order, just as if they were issued without
// using a display list").
void ReplayList(GLuint list, uint32_t depth) {
    Context& c = Context::Current();
    if (depth >= kMaxNesting) {
        c.errors.Record(0x0503);  // GL_STACK_OVERFLOW
        return;
    }
    auto it = c.legacy.lists.find(list);
    if (it == c.legacy.lists.end()) return;  // chỉ số không tồn tại ⇒ không tác dụng
    if (it->second.executing) return;        // chống đệ quy vô hạn
    // Copy ops: trong lúc replay, list khác có thể được thêm/xoá (glGenLists,
    // glDeleteLists…) ⇒ iterator của map có thể bị vô hiệu hoá.
    std::vector<LegacyState::LegacyListOp> ops = it->second.ops;
    it->second.executing = true;
    ++g_replayDepth;
    for (const LegacyState::LegacyListOp& op : ops) ReplayOp(op);
    --g_replayDepth;
    // Map có thể đã thay đổi trong lúc replay ⇒ tìm lại iterator trước khi
    // xoá cờ "đang thực thi".
    auto it2 = c.legacy.lists.find(list);
    if (it2 != c.legacy.lists.end()) it2->second.executing = false;
}

}  // namespace

// ===========================================================================
// API cho các file KHÁC (main agent gắn hook) — xem báo cáo cuối.
// Mọi hàm trả TRUE khi lệnh đã được ghi và KHÔNG được thực thi
// (mode = GL_COMPILE) ⇒ caller phải return ngay. FALSE ⇒ lệnh thực thi bình
// thường (không ghi, hoặc ghi + thực thi với GL_COMPILE_AND_EXECUTE).
// ===========================================================================
bool LegacyRecordEnable(GLenum cap) {
    LegacyState::LegacyListOp op;
    op.op = kOpEnable;
    op.u0 = cap;
    return RecordOp(std::move(op));
}

bool LegacyRecordDisable(GLenum cap) {
    LegacyState::LegacyListOp op;
    op.op = kOpDisable;
    op.u0 = cap;
    return RecordOp(std::move(op));
}

bool LegacyRecordEnableClientState(GLenum array) {
    LegacyState::LegacyListOp op;
    op.op = kOpEnableClientState;
    op.u0 = array;
    return RecordOp(std::move(op));
}

bool LegacyRecordDisableClientState(GLenum array) {
    LegacyState::LegacyListOp op;
    op.op = kOpDisableClientState;
    op.u0 = array;
    return RecordOp(std::move(op));
}

bool LegacyRecordMatrixMode(GLenum mode) {
    LegacyState::LegacyListOp op;
    op.op = kOpMatrixMode;
    op.u0 = mode;
    return RecordOp(std::move(op));
}

// `m` luôn được đưa về dạng COLUMN-MAJOR (glLoadMatrix) trước khi lưu, kể
// cả khi lệnh gốc là *TransposeMatrix (row-major).
bool LegacyRecordLoadMatrix(const GLfloat* m, bool transpose) {
    LegacyState::LegacyListOp op;
    op.op = kOpLoadMatrix;
    if (!m) return false;
    if (transpose) {
        for (int r = 0; r < 4; ++r)
            for (int col = 0; col < 4; ++col) op.m[col * 4 + r] = m[r * 4 + col];
    } else {
        for (int i = 0; i < 16; ++i) op.m[i] = m[i];
    }
    return RecordOp(std::move(op));
}

bool LegacyRecordMultMatrix(const GLfloat* m, bool transpose) {
    LegacyState::LegacyListOp op;
    op.op = kOpMultMatrix;
    if (!m) return false;
    if (transpose) {
        for (int r = 0; r < 4; ++r)
            for (int col = 0; col < 4; ++col) op.m[col * 4 + r] = m[r * 4 + col];
    } else {
        for (int i = 0; i < 16; ++i) op.m[i] = m[i];
    }
    return RecordOp(std::move(op));
}

bool LegacyRecordPushMatrix() {
    LegacyState::LegacyListOp op;
    op.op = kOpPushMatrix;
    return RecordOp(std::move(op));
}

bool LegacyRecordPopMatrix() {
    LegacyState::LegacyListOp op;
    op.op = kOpPopMatrix;
    return RecordOp(std::move(op));
}

bool LegacyRecordLoadIdentity() {
    LegacyState::LegacyListOp op;
    op.op = kOpLoadIdentity;
    return RecordOp(std::move(op));
}

bool LegacyRecordShadeModel(GLenum mode) {
    LegacyState::LegacyListOp op;
    op.op = kOpShadeModel;
    op.u0 = mode;
    return RecordOp(std::move(op));
}

bool LegacyRecordDepthMask(GLboolean flag) {
    LegacyState::LegacyListOp op;
    op.op = kOpDepthMask;
    op.u0 = flag ? 1u : 0u;
    return RecordOp(std::move(op));
}

bool LegacyRecordBlendFunc(GLenum src, GLenum dst) {
    LegacyState::LegacyListOp op;
    op.op = kOpBlendFunc;
    op.u0 = src;
    op.u1 = dst;
    return RecordOp(std::move(op));
}

bool LegacyRecordCullFace(GLenum mode) {
    LegacyState::LegacyListOp op;
    op.op = kOpCullFace;
    op.u0 = mode;
    return RecordOp(std::move(op));
}

bool LegacyRecordFrontFace(GLenum mode) {
    LegacyState::LegacyListOp op;
    op.op = kOpFrontFace;
    op.u0 = mode;
    return RecordOp(std::move(op));
}

bool LegacyRecordPolygonOffset(GLfloat factor, GLfloat units) {
    LegacyState::LegacyListOp op;
    op.op = kOpPolygonOffset;
    op.f0 = factor;
    op.f1 = units;
    return RecordOp(std::move(op));
}

bool LegacyRecordViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    LegacyState::LegacyListOp op;
    op.op = kOpViewport;
    op.f0 = (float)x;
    op.f1 = (float)y;
    op.f2 = (float)w;
    op.f3 = (float)h;
    return RecordOp(std::move(op));
}

bool LegacyRecordScissor(GLint x, GLint y, GLsizei w, GLsizei h) {
    LegacyState::LegacyListOp op;
    op.op = kOpScissor;
    op.f0 = (float)x;
    op.f1 = (float)y;
    op.f2 = (float)w;
    op.f3 = (float)h;
    return RecordOp(std::move(op));
}

bool LegacyRecordLineWidth(GLfloat w) {
    LegacyState::LegacyListOp op;
    op.op = kOpLineWidth;
    op.f0 = w;
    return RecordOp(std::move(op));
}

bool LegacyRecordPointSize(GLfloat s) {
    LegacyState::LegacyListOp op;
    op.op = kOpPointSize;
    op.f0 = s;
    return RecordOp(std::move(op));
}

// `values` có `count` phần tử (1 hoặc 4) — dùng cho cả bản scalar (f0) và
// bản vector (f0..f3).
bool LegacyRecordTexEnv(GLenum target, GLenum pname, const GLfloat* values, int count) {
    if (!values) return false;
    LegacyState::LegacyListOp op;
    op.op = kOpTexEnv;
    op.u0 = target;
    op.u1 = pname;
    op.u2 = (GLuint)count;
    op.f0 = values[0];
    if (count >= 4) {
        op.f1 = values[1];
        op.f2 = values[2];
        op.f3 = values[3];
    }
    return RecordOp(std::move(op));
}

bool LegacyRecordTexGen(GLenum coord, GLenum pname, const GLfloat* values, int count) {
    if (!values) return false;
    LegacyState::LegacyListOp op;
    op.op = kOpTexGen;
    op.u0 = coord;
    op.u1 = pname;
    op.u2 = (GLuint)count;
    op.f0 = values[0];
    if (count >= 4) {
        op.f1 = values[1];
        op.f2 = values[2];
        op.f3 = values[3];
    }
    return RecordOp(std::move(op));
}

bool LegacyRecordLight(GLenum light, GLenum pname, const GLfloat* values, int count) {
    if (!values) return false;
    LegacyState::LegacyListOp op;
    op.op = kOpLight;
    op.u0 = light;
    op.u1 = pname;
    op.u2 = (GLuint)count;
    op.f0 = values[0];
    if (count >= 4) {
        op.f1 = values[1];
        op.f2 = values[2];
        op.f3 = values[3];
    }
    return RecordOp(std::move(op));
}

bool LegacyRecordMaterial(GLenum face, GLenum pname, const GLfloat* values, int count) {
    if (!values) return false;
    LegacyState::LegacyListOp op;
    op.op = kOpMaterial;
    op.u0 = face;
    op.u1 = pname;
    op.u2 = (GLuint)count;
    op.f0 = values[0];
    if (count >= 4) {
        op.f1 = values[1];
        op.f2 = values[2];
        op.f3 = values[3];
    }
    return RecordOp(std::move(op));
}

// ===========================================================================
// API cho src/gl/gl_legacy_vertex.cpp (file sở hữu current vertex attributes
// + glBegin/glEnd).
// ===========================================================================

// Đang ghi vào display list? (dùng cho glBegin/glEnd: khối hình học phải được
// GHI thay vì vẽ khi mode = GL_COMPILE).
bool LegacyIsRecording() { return Recording(); }

// Chế độ GL_COMPILE_AND_EXECUTE? (lệnh vừa ghi vừa thực thi).
bool LegacyCompileAndExecute() {
    return g_compile.active && g_compile.mode == GL_COMPILE_AND_EXECUTE;
}

// Ghi khối hình học (glBegin..glEnd) vào list đang biên dịch.
// Đỉnh đã resolve current state (client state chụp lúc ghi — R2).
void LegacyRecordGeometryBlock(GLenum beginMode) {
    if (!Recording()) return;
    Context& c = Context::Current();
    LegacyState::LegacyListOp op;
    op.op = kOpGeometry;
    op.u0 = (GLuint)c.legacy.immediateVertices.size();
    op.u1 = (GLuint)c.legacy.immediatePrimitives.size();
    op.u2 = beginMode;
    try {
        op.data.reserve((size_t)op.u0 * kFloatsPerVertex + (size_t)op.u1 * kFloatsPerPrim);
        for (const LegacyVertex& v : c.legacy.immediateVertices) {
            const size_t base = op.data.size();
            op.data.resize(base + kFloatsPerVertex);
            PackVertex(v, op.data.data() + base);
        }
        for (const LegacyPrimitive& p : c.legacy.immediatePrimitives) {
            const size_t base = op.data.size();
            op.data.resize(base + kFloatsPerPrim);
            PackPrim(p, op.data.data() + base);
        }
    } catch (const std::bad_alloc&) {
        g_compile.failed = true;
        return;
    }
    RecordOp(std::move(op));
}

// ---- current vertex attributes (client state) --------------------------------
// Khi đang biên dịch: giá trị mới được ghi vào list + lưu vào bản sao
// "compile" (KHÔNG đổi state sống ⇒ "commands are not executed as they are
// placed in the display list", spec p.242). Với GL_COMPILE_AND_EXECUTE thì
// lệnh được thực thi ⇒ state sống đổi luôn.
void LegacySetCurrentColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    Context& c = Context::Current();
    if (Recording()) {
        LegacyState::LegacyListOp op;
        op.op = kOpColor;
        op.f0 = r;
        op.f1 = g;
        op.f2 = b;
        op.f3 = a;
        const bool skip = RecordOp(std::move(op));
        if (skip) {
            g_compile.color[0] = r;
            g_compile.color[1] = g;
            g_compile.color[2] = b;
            g_compile.color[3] = a;
            return;
        }
    }
    c.legacy.currentColor[0] = r;
    c.legacy.currentColor[1] = g;
    c.legacy.currentColor[2] = b;
    c.legacy.currentColor[3] = a;
}

void LegacySetCurrentNormal(GLfloat x, GLfloat y, GLfloat z) {
    Context& c = Context::Current();
    if (Recording()) {
        LegacyState::LegacyListOp op;
        op.op = kOpNormal;
        op.f0 = x;
        op.f1 = y;
        op.f2 = z;
        const bool skip = RecordOp(std::move(op));
        if (skip) {
            g_compile.normal[0] = x;
            g_compile.normal[1] = y;
            g_compile.normal[2] = z;
            return;
        }
    }
    c.legacy.currentNormal[0] = x;
    c.legacy.currentNormal[1] = y;
    c.legacy.currentNormal[2] = z;
}

void LegacySetCurrentTexCoord(GLuint unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    Context& c = Context::Current();
    const GLuint u = unit & 7u;
    if (Recording()) {
        LegacyState::LegacyListOp op;
        op.op = kOpTexCoord;
        op.u0 = u;
        op.f0 = s;
        op.f1 = t;
        op.f2 = r;
        op.f3 = q;
        const bool skip = RecordOp(std::move(op));
        if (skip) {
            g_compile.texCoord[u][0] = s;
            g_compile.texCoord[u][1] = t;
            g_compile.texCoord[u][2] = r;
            g_compile.texCoord[u][3] = q;
            return;
        }
    }
    c.legacy.currentTexCoord[u][0] = s;
    c.legacy.currentTexCoord[u][1] = t;
    c.legacy.currentTexCoord[u][2] = r;
    c.legacy.currentTexCoord[u][3] = q;
}

void LegacySetCurrentSecondaryColor(GLfloat r, GLfloat g, GLfloat b) {
    Context& c = Context::Current();
    if (Recording()) {
        LegacyState::LegacyListOp op;
        op.op = kOpSecondaryColor;
        op.f0 = r;
        op.f1 = g;
        op.f2 = b;
        const bool skip = RecordOp(std::move(op));
        if (skip) {
            g_compile.secondary[0] = r;
            g_compile.secondary[1] = g;
            g_compile.secondary[2] = b;
            return;
        }
    }
    c.legacy.currentSecondaryColor[0] = r;
    c.legacy.currentSecondaryColor[1] = g;
    c.legacy.currentSecondaryColor[2] = b;
}

// Client state mà ĐỈNH sẽ dùng (bản "compile" khi đang biên dịch).
void LegacyCurrentColor(GLfloat out[4]) {
    Context& c = Context::Current();
    if (Recording()) {
        for (int k = 0; k < 4; ++k) out[k] = g_compile.color[k];
        return;
    }
    for (int k = 0; k < 4; ++k) out[k] = c.legacy.currentColor[k];
}

void LegacyCurrentNormal(GLfloat out[3]) {
    Context& c = Context::Current();
    if (Recording()) {
        for (int k = 0; k < 3; ++k) out[k] = g_compile.normal[k];
        return;
    }
    for (int k = 0; k < 3; ++k) out[k] = c.legacy.currentNormal[k];
}

void LegacyCurrentTexCoord(GLuint unit, GLfloat out[4]) {
    Context& c = Context::Current();
    const GLuint u = unit & 7u;
    if (Recording()) {
        for (int k = 0; k < 4; ++k) out[k] = g_compile.texCoord[u][k];
        return;
    }
    for (int k = 0; k < 4; ++k) out[k] = c.legacy.currentTexCoord[u][k];
}

void LegacyCurrentSecondaryColor(GLfloat out[3]) {
    Context& c = Context::Current();
    if (Recording()) {
        for (int k = 0; k < 3; ++k) out[k] = g_compile.secondary[k];
        return;
    }
    for (int k = 0; k < 3; ++k) out[k] = c.legacy.currentSecondaryColor[k];
}

// ===========================================================================
// 8 entry point của display list
// ===========================================================================

// void NewList( uint n, enum mode );  (spec p.242)
//   "n is a positive integer to which the display list that follows is
//    assigned, and mode is a symbolic constant that controls the behavior of
//    the GL during display list creation. If mode is COMPILE, then commands
//    are not executed as they are placed in the display list. If mode is
//    COMPILE AND EXECUTE then commands are executed as they are encountered,
//    then placed in the display list. If n = 0, then the error INVALID VALUE
//    is generated."
//   + "The error INVALID OPERATION is generated if EndList is called without
//    a previous matching NewList, or if NewList is called a second time
//    before calling EndList."
void glNewList(GLuint list, GLenum mode) {
    Context& c = Context::Current();
    if (list == 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    if (g_compile.active) {
        c.errors.Record(0x0502);  // GL_INVALID_OPERATION (NewList lần 2)
        return;
    }
    g_compile = CompileSession();
    g_compile.active = true;
    g_compile.list = list;
    g_compile.mode = mode;
    // Shadow client state = client state lúc ghi (R2).
    memcpy(g_compile.color, c.legacy.currentColor, sizeof(g_compile.color));
    memcpy(g_compile.normal, c.legacy.currentNormal, sizeof(g_compile.normal));
    memcpy(g_compile.secondary, c.legacy.currentSecondaryColor, sizeof(g_compile.secondary));
    memcpy(g_compile.texCoord, c.legacy.currentTexCoord, sizeof(g_compile.texCoord));
    c.legacy.listRecording = list;
}

// void EndList( void );  (spec p.242)
//   "It is only when EndList occurs that the specified display list is
//    actually associated with the index indicated with NewList. The error
//    INVALID OPERATION is generated if EndList is called without a previous
//    matching NewList ... The error OUT OF MEMORY is generated if EndList is
//    called and the specified display list cannot be stored ... no change is
//    made to the previous contents of the display list, if any."
void glEndList(void) {
    Context& c = Context::Current();
    if (!g_compile.active) {
        c.errors.Record(0x0502);  // GL_INVALID_OPERATION
        return;
    }
    const GLuint list = g_compile.list;
    const GLenum mode = g_compile.mode;
    if (g_compile.failed) {
        // Không đủ bộ nhớ ⇒ list cũ giữ nguyên (spec p.242).
        g_compile = CompileSession();
        c.legacy.listRecording = 0;
        c.errors.Record(0x0505);  // GL_OUT_OF_MEMORY
        return;
    }
    // Commit (transactional): chỉ khi commit thành công mới thay list cũ.
    LegacyState::LegacyDisplayList dl;
    dl.mode = mode;
    dl.valid = true;
    dl.executing = false;
    dl.ops = std::move(g_compile.ops);
    bool ok = true;
    try {
        c.legacy.lists[list] = std::move(dl);
    } catch (const std::bad_alloc&) {
        ok = false;
    }
    g_compile = CompileSession();
    c.legacy.listRecording = 0;
    if (!ok) c.errors.Record(0x0505);  // GL_OUT_OF_MEMORY
}

// uint GenLists( sizei s );  (spec p.243)
//   "returns an integer n such that the indices n, . . . , n+s−1 are
//    previously unused ... GenLists also has the effect of creating an empty
//    display list for each of the indices n, . . . , n+s−1, so that these
//    indices all become used. GenLists returns 0 if there is no group of s
//    contiguous previously unused display list indices, or if s = 0."
GLuint glGenLists(GLsizei range) {
    Context& c = Context::Current();
    if (range < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return 0;
    }
    if (range == 0) return 0;  // spec: trả 0, không làm gì khác
    // Chỉ số đã dùng = khóa có trong Context::legacy.lists (list rỗng tạo bởi
    // glGenLists cũng nằm trong map ⇒ đã dùng).
    GLuint maxUsed = 0;
    for (const auto& kv : c.legacy.lists)
        if (kv.first > maxUsed) maxUsed = kv.first;
    // Tìm block `range` chỉ số LIÊN TIẾP chưa dùng (bắt đầu từ 1).
    // Giới hạn quét: tránh quét vô hạn khi `range` rất lớn (spec: "GenLists
    // returns 0 if there is no group of s contiguous previously unused display
    // list indices" ⇒ vượt cửa sổ quét ⇒ coi như không có block).
    constexpr GLuint kMaxGenScan = 1u << 20;
    GLuint start = 0;
    GLuint run = 0;
    const GLuint limit = maxUsed + (GLuint)range + 1u;
    for (GLuint i = 1; i <= limit && run < (GLuint)range && i <= kMaxGenScan; ++i) {
        bool used = (c.legacy.lists.find(i) != c.legacy.lists.end());
        if (g_compile.active && i == g_compile.list) used = true;
        if (used) {
            run = 0;
            continue;
        }
        ++run;
        if (run == (GLuint)range) {
            start = i - (GLuint)range + 1u;
            break;
        }
    }
    if (start == 0) return 0;  // không có block liên tiếp chưa dùng
    // Tạo một list RỖNG cho mỗi chỉ số ⇒ các chỉ số này "đã dùng".
    try {
        for (GLuint i = 0; i < (GLuint)range; ++i) {
            LegacyState::LegacyDisplayList dl;
            dl.mode = 0;
            dl.valid = true;
            dl.executing = false;
            c.legacy.lists[start + i] = std::move(dl);
        }
    } catch (const std::bad_alloc&) {
        c.errors.Record(0x0505);  // GL_OUT_OF_MEMORY
        return 0;
    }
    return start;
}

// void DeleteLists( uint list, sizei range );  (spec p.244)
//   "where list is the index of the first display list to be deleted and range
//    is the number of display lists to be deleted. All information about the
//    display lists is lost, and the indices become unused. Indices to which no
//    display list corresponds are ignored. If range = 0, nothing happens."
void glDeleteLists(GLuint list, GLsizei range) {
    Context& c = Context::Current();
    if (range < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    if (range == 0) return;  // spec: nothing happens
    for (GLsizei i = 0; i < range; ++i) {
        const GLuint idx = list + (GLuint)i;
        c.legacy.lists.erase(idx);  // chỉ số không tồn tại bị bỏ qua
    }
}

// void CallList( uint n );  (spec p.242)
//   "This causes the commands saved in the display list to be executed, in
//    order, just as if they were issued without using a display list. If
//    n = 0, then the error INVALID VALUE is generated."
void glCallList(GLuint list) {
    Context& c = Context::Current();
    if (list == 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    if (Recording()) {
        // Ghi LỆNH call (spec p.243: "the CallList or CallLists, rather than
        // those lists' constituent commands, is placed in the list under
        // construction").
        LegacyState::LegacyListOp op;
        op.op = kOpCallList;
        op.u0 = list;
        const bool skip = RecordOp(std::move(op));
        if (!skip) {
            // GL_COMPILE_AND_EXECUTE ⇒ list con được thực thi ngay.
            ReplayList(list, g_replayDepth);
        }
        return;
    }
    ReplayList(list, g_replayDepth);
}

// void CallLists( sizei n, enum type, void *lists );  (spec p.243)
//   "n is an integer indicating the number of display lists to be called, and
//    lists is a pointer that points to an array of offsets. ... Each of the n
//    constructed offsets is taken in order and added to a display list base to
//    obtain a display list number. For each number, the indicated display list
//    is executed. ... If n = 0, CallLists does nothing."
void glCallLists(GLsizei n, GLenum type, const void* lists) {
    Context& c = Context::Current();
    if (n < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    if (!ValidCallListType(type)) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    if (n == 0) return;  // spec: does nothing
    if (!lists) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE (con trỏ NULL)
        return;
    }
    // Đọc n offset theo kiểu `type` (con trỏ được dereference lúc GỌI lệnh).
    std::vector<GLint> offs;
    offs.reserve((size_t)n);
    for (GLsizei i = 0; i < n; ++i) offs.push_back(ReadCallListOffset(type, lists, i));
    const GLuint base = c.legacy.listBase;
    if (Recording()) {
        LegacyState::LegacyListOp op;
        op.op = kOpCallLists;
        op.u0 = type;
        op.u1 = (GLuint)n;
        try {
            op.data.reserve((size_t)n);
            for (GLsizei i = 0; i < n; ++i) op.data.push_back((float)offs[(size_t)i]);
        } catch (const std::bad_alloc&) {
            g_compile.failed = true;
            return;
        }
        const bool skip = RecordOp(std::move(op));
        if (!skip) {
            // GL_COMPILE_AND_EXECUTE ⇒ các list con được thực thi ngay.
            for (GLsizei i = 0; i < n; ++i) {
                const long long idx = (long long)offs[(size_t)i] + (long long)base;
                if (idx <= 0) continue;
                ReplayList((GLuint)idx, g_replayDepth);
            }
        }
        return;
    }
    for (GLsizei i = 0; i < n; ++i) {
        const long long idx = (long long)offs[(size_t)i] + (long long)base;
        if (idx <= 0) continue;  // chỉ số không tồn tại ⇒ không có tác dụng
        ReplayList((GLuint)idx, g_replayDepth);
    }
}

// boolean IsList( uint list );  (spec p.244)
//   "returns TRUE if list is the index of some display list."
GLboolean glIsList(GLuint list) {
    Context& c = Context::Current();
    if (list == 0) return GL_FALSE;  // 0 không phải chỉ số list hợp lệ
    return c.legacy.lists.find(list) != c.legacy.lists.end() ? GL_TRUE : GL_FALSE;
}

// void ListBase( uint base );  (spec p.243)
//   "The base is set by calling void ListBase( uint base ); to specify the
//    offset."  (giá trị ban đầu = 0)
void glListBase(GLuint base) {
    Context& c = Context::Current();
    c.legacy.listBase = base;
}

}  // namespace tglmt::gl
