// gl_legacy_clientarrays.cpp — CLIENT-SIDE VERTEX ARRAYS (Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §2.8 "Vertex Arrays"
//       (.cache/spec/glspec42.compatibility.pdf, PDF p.58–75) và
//       OpenGL 2.1 §2.8 (glspec21.pdf, PDF p.38–46) cho bảng kiểu dữ liệu.
//
// 13 entry point trong file này:
//   glVertexPointer, glNormalPointer, glColorPointer, glSecondaryColorPointer,
//   glIndexPointer, glEdgeFlagPointer, glFogCoordPointer, glTexCoordPointer,
//   glEnableClientState, glDisableClientState, glClientActiveTexture,
//   glArrayElement, glInterleavedArrays.
//
// NGUỒN CHÂN LÝ (đã đối chiếu văn bản PDF, không suy đoán):
//   • PDF p.58–59 (§2.8): 8 lệnh *Pointer; stride < 0 ⇒ INVALID_VALUE;
//     size sai ⇒ INVALID_VALUE; type sai ⇒ INVALID_ENUM (quy tắc chung của
//     spec về enum); *Pointer khi VAO khác 0 đang bind + ARRAY_BUFFER = 0 +
//     pointer ≠ NULL ⇒ INVALID_OPERATION (PDF p.59).
//   • PDF p.60, Table 2.5: kích thước + kiểu + cột "Integer Handling":
//       VertexPointer/TexCoordPointer/IndexPointer = "cast" (KHÔNG chuẩn hoá);
//       NormalPointer/ColorPointer/SecondaryColorPointer = "normalize".
//   • PDF p.31 (§2.1.2): phương trình chuẩn hoá (2.1) unsigned f = c/(2^b−1);
//       (2.2) signed f = max{c/(2^b−1), −1}.
//   • glspec21.pdf PDF p.73 (§2.14, Table 2.9 "Component conversions"):
//       color/normal/depth: ubyte c/(2^8−1), byte (2c+1)/(2^8−1),
//       ushort c/(2^16−1), short (2c+1)/(2^16−1), uint c/(2^32−1),
//       int (2c+1)/(2^32−1).
//     ⇒ đường draw dùng ĐÚNG bảng này cho mảng "normalize" (khớp với
//     pseudocode ArrayElement: "Color[size][type]v(color array element i)").
//   • PDF p.61–62: 8 giá trị hợp lệ của EnableClientState/DisableClientState;
//     ClientActiveTexture chọn texture coordinate set (mỗi set có client
//     state vector riêng).
//   • PDF p.63–64: ArrayElement = "chuyển phần tử thứ i của MỌI mảng đang bật
//     sang GL"; i < 0 ⇒ đề xuất sinh INVALID_VALUE.
//   • PDF p.73–75 (§2.8.3 + Table 2.8): InterleavedArrays (14 format).
//   • glspec21.pdf PDF p.42–43: DrawArrays/DrawElements tương đương
//     Begin(mode); ArrayElement(...); End() ⇒ draw path PHẢI đọc đỉnh từ
//     client array (xem src/gl/gl_draw.cpp).
//
// LƯU Ý KIẾN TRÚC:
//   • State client array nằm trong Context::legacy.arrayPointers[8] (8 mảng:
//     VERTEX, NORMAL, COLOR, SECONDARY_COLOR, INDEX, EDGE_FLAG, FOG_COORD,
//     TEXTURE_COORD) + clientActiveTexture + arrayElement (đã có sẵn trong
//     include/tglmt/LegacyState.h — file này KHÔNG sửa).
//   • Riêng TEXTURE_COORD: spec 4.2C p.62 yêu cầu MỖI texture coordinate set
//     có client state vector riêng, nhưng LegacyState chỉ có 1 slot ⇒ TGLMT
//     giữ MỘT texture coordinate array (slot 7) và dùng clientActiveTexture
//     để chọn texture unit mà array đó phục vụ (xem báo cáo: đây là sai
//     lệch đã biết so với spec, do LegacyState không có state per-unit).
//   • Đường draw: hàm LegacyDrawClientArrays() (public, xem cuối file) được
//     gọi từ src/gl/gl_draw.cpp (EmitDraw) — đó là điểm hook duy nhất cần
//     sửa ở file khác.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/LegacyDisplayList.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/Context.h"
#include <cstring>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

// Struct con của LegacyState (include/tglmt/LegacyState.h:416) — alias cục bộ
// cho gọn (file này KHÔNG sửa LegacyState.h).
using LegacyArrayPointer = LegacyState::LegacyArrayPointer;

// Khai báo ở gl_legacy_vertex.cpp (hàm ghép primitive dùng chung cho cả
// immediate mode và client array).
void AssemblePrimitives(LegacyState& st);

// Hook current-vertex-attribute của display list (định nghĩa ở
// src/gl/gl_legacy_displaylist.cpp) — dùng để glArrayElement cập nhật
// current state ĐÚNG kiểu display list (chụp lúc ghi).
void LegacySetCurrentColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void LegacySetCurrentNormal(GLfloat x, GLfloat y, GLfloat z);
void LegacySetCurrentTexCoord(GLuint unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void LegacySetCurrentSecondaryColor(GLfloat r, GLfloat g, GLfloat b);

namespace {

// ---------------------------------------------------------------------------
// Hằng / bảng
// ---------------------------------------------------------------------------
// Chỉ số mảng client trong LegacyState::arrayPointers[8].
enum ArraySlot {
    kSlotVertex = 0,
    kSlotNormal = 1,
    kSlotColor = 2,
    kSlotSecondaryColor = 3,
    kSlotIndex = 4,
    kSlotEdgeFlag = 5,
    kSlotFogCoord = 6,
    kSlotTexCoord = 7,
};

// Số texture coordinate set (MAX_TEXTURE_COORDS) hỗ trợ cho client array.
// Khớp với LegacyState::currentTexCoord[8] / texEnv[8] / texGen[8].
constexpr int kMaxTexCoordSets = 8;

// Bảng thống kê draw gần nhất (để test + chẩn đoán). Thread-local: Context
// của TGLMT cũng là thread-local ⇒ mỗi thread một bảng. KHÔNG chứa state
// client array (state nằm trong Context::legacy) để không bị stale khi một
// context mới được tạo tại cùng địa chỉ cũ.
struct ClientArrayStats {
    const void* owner = nullptr;  // Context* sở hữu thống kê này
    uint32_t lastVerticesFetched = 0;
    uint32_t lastIndicesFetched = 0;
    bool lastUsedClientMemory = false;
    bool lastDrew = false;
    bool lastActive = false;
};

thread_local ClientArrayStats g_stats;

ClientArrayStats& Stats(Context& c) {
    if (g_stats.owner != static_cast<const void*>(&c)) {
        g_stats = ClientArrayStats();
        g_stats.owner = static_cast<const void*>(&c);
    }
    return g_stats;
}

// 8 giá trị hợp lệ của EnableClientState/DisableClientState (spec 4.2C p.61).
bool IsClientArrayEnum(GLenum array) {
    switch (array) {
        case GL_VERTEX_ARRAY:
        case GL_NORMAL_ARRAY:
        case GL_COLOR_ARRAY:
        case GL_SECONDARY_COLOR_ARRAY:
        case GL_INDEX_ARRAY:
        case GL_EDGE_FLAG_ARRAY:
        case GL_FOG_COORD_ARRAY:
        case GL_TEXTURE_COORD_ARRAY:
            return true;
        default:
            return false;
    }
}

// Kiểu dữ liệu được phép cho các lệnh *Pointer.
// Spec 4.2C Table 2.5 (PDF p.60) liệt kê kiểu riêng cho từng lệnh (ví dụ
// VertexPointer không có BYTE). TGLMT chấp nhận tập hợp chung của các kiểu
// cố định để không từ chối app cũ; quy tắc CHUYỂN ĐỔI vẫn theo cột
// "Integer Handling" của Table 2.5 (cast / normalize).
bool IsArrayType(GLenum type) {
    switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
            return true;
        default:
            return false;
    }
}

// Kích thước (byte) của 1 phần tử theo kiểu.
size_t TypeSize(GLenum type) {
    switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE: return 1;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT: return 2;
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT: return 4;
        case GL_DOUBLE: return 8;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// Quy tắc chuyển đổi (Table 2.5 "Integer Handling" + Table 2.9 GL 2.1)
// ---------------------------------------------------------------------------
// CAST: giá trị nguyên được đổi thẳng sang float (KHÔNG chuẩn hoá).
float CastByte(GLbyte v) { return (float)v; }
float CastUByte(GLubyte v) { return (float)v; }
float CastShort(GLshort v) { return (float)v; }
float CastUShort(GLushort v) { return (float)v; }
float CastInt(GLint v) { return (float)v; }
float CastUInt(GLuint v) { return (float)v; }

// NORMALIZE (Table 2.9 của GL 2.1, áp cho color/normal):
//   ubyte  c/(2^8−1)      byte  (2c+1)/(2^8−1)
//   ushort c/(2^16−1)     short (2c+1)/(2^16−1)
//   uint   c/(2^32−1)     int   (2c+1)/(2^32−1)
float NormUByte(GLubyte v) { return (float)((double)v / 255.0); }
float NormByte(GLbyte v) { return (float)((2.0 * (double)v + 1.0) / 255.0); }
float NormUShort(GLushort v) { return (float)((double)v / 65535.0); }
float NormShort(GLshort v) { return (float)((2.0 * (double)v + 1.0) / 65535.0); }
float NormUInt(GLuint v) { return (float)((double)v / 4294967295.0); }
float NormInt(GLint v) { return (float)((2.0 * (double)v + 1.0) / 4294967295.0); }

// ---------------------------------------------------------------------------
// Truy cập state
// ---------------------------------------------------------------------------
// Con trỏ tới array client tương ứng.
LegacyArrayPointer* ArrayOf(Context& c, GLenum array) {
    switch (array) {
        case GL_VERTEX_ARRAY: return &c.legacy.arrayPointers[kSlotVertex];
        case GL_NORMAL_ARRAY: return &c.legacy.arrayPointers[kSlotNormal];
        case GL_COLOR_ARRAY: return &c.legacy.arrayPointers[kSlotColor];
        case GL_SECONDARY_COLOR_ARRAY:
            return &c.legacy.arrayPointers[kSlotSecondaryColor];
        case GL_INDEX_ARRAY: return &c.legacy.arrayPointers[kSlotIndex];
        case GL_EDGE_FLAG_ARRAY: return &c.legacy.arrayPointers[kSlotEdgeFlag];
        case GL_FOG_COORD_ARRAY: return &c.legacy.arrayPointers[kSlotFogCoord];
        case GL_TEXTURE_COORD_ARRAY: return &c.legacy.arrayPointers[kSlotTexCoord];
        default: return nullptr;
    }
}

// Cờ "dùng client memory": true nếu có array nào đang bật và trỏ vào client
// memory (không phải buffer object).
void UpdateClientMemoryFlag(Context& c) {
    bool clientMem = false;
    for (int i = 0; i < 8; ++i) {
        const LegacyArrayPointer& a = c.legacy.arrayPointers[i];
        if (a.enabled && a.buffer == 0 && a.pointer) clientMem = true;
    }
    c.legacy.arrayUsesClientMemory = clientMem;
}

// Ghi 1 array client (size/type/stride/pointer) — dùng chung cho cả 8 lệnh
// *Pointer. `size` < 0 ⇒ lệnh không có tham số size (normal/index/fog/edge).
void StoreArrayPointer(Context& c, GLenum array, GLint size, GLenum type,
                       GLsizei stride, const void* pointer) {
    LegacyArrayPointer* ap = ArrayOf(c, array);
    if (!ap) return;
    ap->size = size;
    ap->type = type;
    ap->stride = stride;
    ap->pointer = pointer;
    // Buffer object đang bind lúc gọi (spec 4.2C §2.9): nếu ARRAY_BUFFER ≠ 0
    // thì pointer là BYTE OFFSET trong buffer đó; ngược lại là con trỏ client.
    ap->buffer = c.state.BoundBuffer(GL_ARRAY_BUFFER);
    UpdateClientMemoryFlag(c);
}

// Bật/tắt một array client (KHÔNG ghi display list — dùng nội bộ).
void SetArrayEnabled(Context& c, GLenum array, bool on) {
    LegacyArrayPointer* ap = ArrayOf(c, array);
    if (!ap) return;
    ap->enabled = on;
    // Cờ enable chung để glIsEnabled(array) trả đúng (Mesa cũng vậy).
    c.state.SetEnabled(array, on);
    UpdateClientMemoryFlag(c);
}

// Kiểm tra chung cho 8 lệnh *Pointer:
//   • size sai              ⇒ GL_INVALID_VALUE (Table 2.5)
//   • type sai              ⇒ GL_INVALID_ENUM
//   • stride < 0            ⇒ GL_INVALID_VALUE (spec p.61)
//   • VAO ≠ 0 đang bind + ARRAY_BUFFER = 0 + pointer ≠ NULL
//                          ⇒ GL_INVALID_OPERATION (spec p.59)
// Trả false nếu có lỗi (đã ghi lỗi vào error queue).
bool ValidatePointerArgs(Context& c, GLint size, bool hasSize, int minSize,
                         int maxSize, GLenum type, GLsizei stride,
                         const void* pointer) {
    if (hasSize) {
        if (size < minSize || size > maxSize) {
            c.errors.Record(0x0501);  // GL_INVALID_VALUE
            return false;
        }
    }
    if (!IsArrayType(type)) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return false;
    }
    if (stride < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return false;
    }
    // Spec 4.2C p.59: *Pointer với VAO khác 0 đang bind, ARRAY_BUFFER = 0 và
    // pointer ≠ NULL ⇒ INVALID_OPERATION (không cho tạo VAO chứa client
    // pointer).
    if (pointer != nullptr && c.state.BoundVAO() != 0 &&
        c.state.BoundBuffer(GL_ARRAY_BUFFER) == 0) {
        c.errors.Record(0x0502);  // GL_INVALID_OPERATION
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Đọc dữ liệu client array
// ---------------------------------------------------------------------------
// Đọc `comps` thành phần của phần tử thứ `index`.
//   normalize = false ⇒ "cast"   (Table 2.5: Vertex/TexCoord/Index)
//   normalize = true  ⇒ "normalize" (Table 2.5: Normal/Color/SecondaryColor)
// Nguồn dữ liệu: client memory (buffer == 0) hoặc buffer object (buffer ≠ 0,
// pointer là byte offset trong buffer đó).
bool ReadArrayElement(Context& c, const LegacyArrayPointer& ap, int comps,
                       bool normalize, uint32_t index, float* out) {
    const size_t esz = TypeSize(ap.type);
    if (esz == 0) return false;
    const uint8_t* p = nullptr;
    if (ap.buffer != 0) {
        Context::BufferView bv = c.SyncBuffer(ap.buffer);
        if (!bv.p) return false;
        const size_t stride =
            (ap.stride != 0) ? (size_t)ap.stride : esz * (size_t)comps;
        const size_t off = (size_t)ap.pointer + (size_t)index * stride;
        if (off + esz * (size_t)comps > bv.size) return false;  // ngoài vùng
        p = bv.p + off;
    } else {
        if (!ap.pointer) return false;
        const size_t stride =
            (ap.stride != 0) ? (size_t)ap.stride : esz * (size_t)comps;
        p = (const uint8_t*)ap.pointer + (size_t)index * stride;
    }
    for (int k = 0; k < comps; ++k) {
        const uint8_t* q = p + (size_t)k * esz;
        float v = 0.0f;
        switch (ap.type) {
            case GL_BYTE: {
                GLbyte b;
                memcpy(&b, q, 1);
                v = normalize ? NormByte(b) : CastByte(b);
                break;
            }
            case GL_UNSIGNED_BYTE: {
                GLubyte b;
                memcpy(&b, q, 1);
                v = normalize ? NormUByte(b) : CastUByte(b);
                break;
            }
            case GL_SHORT: {
                GLshort s;
                memcpy(&s, q, 2);
                v = normalize ? NormShort(s) : CastShort(s);
                break;
            }
            case GL_UNSIGNED_SHORT: {
                GLushort s;
                memcpy(&s, q, 2);
                v = normalize ? NormUShort(s) : CastUShort(s);
                break;
            }
            case GL_INT: {
                GLint i;
                memcpy(&i, q, 4);
                v = normalize ? NormInt(i) : CastInt(i);
                break;
            }
            case GL_UNSIGNED_INT: {
                GLuint i;
                memcpy(&i, q, 4);
                v = normalize ? NormUInt(i) : CastUInt(i);
                break;
            }
            case GL_FLOAT: {
                float f;
                memcpy(&f, q, 4);
                v = f;
                break;
            }
            case GL_DOUBLE: {
                double d;
                memcpy(&d, q, 8);
                v = (float)d;
                break;
            }
            default:
                return false;
        }
        out[k] = v;
    }
    return true;
}

// Đọc phần tử thứ `index` của texture coordinate array (một array duy nhất,
// phục vụ texture unit clientActiveTexture — xem ghi chú kiến trúc).
bool ReadTexCoordElement(Context& c, uint32_t index, float out[4]) {
    const LegacyArrayPointer& ap = c.legacy.arrayPointers[kSlotTexCoord];
    if (!ap.enabled) return false;
    if (ap.buffer == 0 && !ap.pointer) return false;
    const int comps = (ap.size >= 1 && ap.size <= 4) ? ap.size : 4;
    float tmp[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (!ReadArrayElement(c, ap, comps, false /*texcoord = "cast"*/, index, tmp))
        return false;
    for (int k = 0; k < 4; ++k) out[k] = tmp[k];
    if (comps < 4) out[3] = 1.0f;
    return true;
}

}  // namespace

// ===========================================================================
// 1. Tám lệnh *Pointer
// ===========================================================================
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glVertexPointer")) return;
    // Table 2.5: VertexPointer size ∈ {2,3,4}.
    if (!ValidatePointerArgs(c, size, true, 2, 4, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_VERTEX_ARRAY, size, type, stride, pointer);
}

void glNormalPointer(GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glNormalPointer")) return;
    // Normal luôn có 3 thành phần (spec p.59) ⇒ không có tham số size.
    if (!ValidatePointerArgs(c, 3, false, 3, 3, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_NORMAL_ARRAY, 3, type, stride, pointer);
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glColorPointer")) return;
    // Table 2.5: ColorPointer size ∈ {3,4}.
    if (!ValidatePointerArgs(c, size, true, 3, 4, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_COLOR_ARRAY, size, type, stride, pointer);
}

void glSecondaryColorPointer(GLint size, GLenum type, GLsizei stride,
                             const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glSecondaryColorPointer")) return;
    // Table 2.5: SecondaryColorPointer size ∈ {3,4} (4 tương ứng "BGRA" trong
    // spec; TGLMT đọc theo thứ tự RGBA cho mọi kiểu — xem báo cáo).
    if (!ValidatePointerArgs(c, size, true, 3, 4, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_SECONDARY_COLOR_ARRAY, size, type, stride, pointer);
}

void glIndexPointer(GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glIndexPointer")) return;
    // Color index luôn có 1 thành phần.
    if (!ValidatePointerArgs(c, 1, false, 1, 1, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_INDEX_ARRAY, 1, type, stride, pointer);
}

void glEdgeFlagPointer(GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glEdgeFlagPointer")) return;
    // Edge flag luôn là boolean 1 thành phần (không có tham số type).
    if (!ValidatePointerArgs(c, 1, false, 1, 1, GL_UNSIGNED_BYTE, stride, pointer))
        return;
    StoreArrayPointer(c, GL_EDGE_FLAG_ARRAY, 1, GL_UNSIGNED_BYTE, stride, pointer);
}

void glFogCoordPointer(GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFogCoordPointer")) return;
    // Fog coord luôn có 1 thành phần.
    if (!ValidatePointerArgs(c, 1, false, 1, 1, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_FOG_COORD_ARRAY, 1, type, stride, pointer);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glTexCoordPointer")) return;
    // Table 2.5: TexCoordPointer size ∈ {1,2,3,4}.
    if (!ValidatePointerArgs(c, size, true, 1, 4, type, stride, pointer)) return;
    StoreArrayPointer(c, GL_TEXTURE_COORD_ARRAY, size, type, stride, pointer);
}

// ===========================================================================
// 2. glEnableClientState / glDisableClientState
// ===========================================================================
void glEnableClientState(GLenum array) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glEnableClientState")) return;
    if (!IsClientArrayEnum(array)) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    // Display list: lệnh phải được GHI vào list (spec §5.5). Trả true ⇒
    // đã ghi và KHÔNG được thực thi (mode GL_COMPILE).
    if (LegacyRecordEnableClientState(array)) return;
    SetArrayEnabled(c, array, true);
}

void glDisableClientState(GLenum array) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glDisableClientState")) return;
    if (!IsClientArrayEnum(array)) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    if (LegacyRecordDisableClientState(array)) return;
    SetArrayEnabled(c, array, false);
}

// ===========================================================================
// 3. glClientActiveTexture
// ===========================================================================
void glClientActiveTexture(GLenum texture) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glClientActiveTexture")) return;
    // Spec 4.2C p.62: "Specifying an invalid texture generates the error
    // INVALID_ENUM. Valid values of texture are the same as for the MultiTexCoord
    // commands" ⇒ GL_TEXTURE0 .. GL_TEXTURE0+MAX_TEXTURE_COORDS−1.
    if (texture < GL_TEXTURE0 || texture >= GL_TEXTURE0 + kMaxTexCoordSets) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    c.legacy.clientActiveTexture = (GLuint)(texture - GL_TEXTURE0);
}

// ===========================================================================
// 4. glArrayElement
// ===========================================================================
void glArrayElement(GLint i) {
    Context& c = Context::Current();
    // Spec 4.2C p.64: "Specifying i < 0 results in undefined behavior.
    // Generating the error INVALID_VALUE is recommended in this case."
    if (i < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    c.legacy.arrayElement = (GLuint)i;
    const uint32_t idx = (uint32_t)i;

    // Chuyển phần tử thứ i của MỌI mảng đang bật sang current state
    // (spec p.63–64: tương đương chuỗi Normal3v/Color4v/...).
    const LegacyArrayPointer* normal = &c.legacy.arrayPointers[kSlotNormal];
    if (normal->enabled) {
        float n[3] = {0.0f, 0.0f, 0.0f};
        if (ReadArrayElement(c, *normal, 3, true /*normalize → [-1,1]*/, idx, n))
            LegacySetCurrentNormal(n[0], n[1], n[2]);
    }
    const LegacyArrayPointer* color = &c.legacy.arrayPointers[kSlotColor];
    if (color->enabled) {
        const int comps = color->size >= 3 ? color->size : 3;
        float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        if (ReadArrayElement(c, *color, comps, true /*normalize*/, idx, col)) {
            if (comps == 3) col[3] = 1.0f;
            LegacySetCurrentColor(col[0], col[1], col[2], col[3]);
        }
    }
    const LegacyArrayPointer* sec = &c.legacy.arrayPointers[kSlotSecondaryColor];
    if (sec->enabled) {
        float s[3] = {0.0f, 0.0f, 0.0f};
        if (ReadArrayElement(c, *sec, 3, true, idx, s))
            LegacySetCurrentSecondaryColor(s[0], s[1], s[2]);
    }
    const LegacyArrayPointer* fog = &c.legacy.arrayPointers[kSlotFogCoord];
    if (fog->enabled) {
        float f = 0.0f;
        if (ReadArrayElement(c, *fog, 1, false, idx, &f))
            c.legacy.fog.currentCoord = f;
    }
    // Texture coordinate: array phục vụ texture unit clientActiveTexture.
    {
        const LegacyArrayPointer* tc = &c.legacy.arrayPointers[kSlotTexCoord];
        if (tc->enabled) {
            float t[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            if (ReadTexCoordElement(c, idx, t)) {
                GLuint u = c.legacy.clientActiveTexture;
                if (u >= kMaxTexCoordSets) u = 0;
                LegacySetCurrentTexCoord(u, t[0], t[1], t[2], t[3]);
            }
        }
    }
    const LegacyArrayPointer* index = &c.legacy.arrayPointers[kSlotIndex];
    if (index->enabled) {
        float v = 0.0f;
        if (ReadArrayElement(c, *index, 1, false, idx, &v))
            c.legacy.currentIndex = (GLuint)v;
    }
    const LegacyArrayPointer* edge = &c.legacy.arrayPointers[kSlotEdgeFlag];
    if (edge->enabled) {
        float v = 0.0f;
        if (ReadArrayElement(c, *edge, 1, false, idx, &v))
            c.legacy.edgeFlag = (v != 0.0f) ? 1 : 0;
    }
    // Nếu đang trong glBegin/glEnd ⇒ đây là một ĐỈNH (spec: ArrayElement kèm
    // vertex array đang bật tương đương lệnh Vertex).
    const LegacyArrayPointer* vert = &c.legacy.arrayPointers[kSlotVertex];
    if (c.legacy.inBeginEnd && vert->enabled &&
        (vert->pointer != nullptr || vert->buffer != 0)) {
        float p[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        const int comps = vert->size >= 2 ? vert->size : 3;
        if (ReadArrayElement(c, *vert, comps, false /*cast*/, idx, p)) {
            if (comps == 2) {
                glVertex3f(p[0], p[1], 0.0f);
            } else if (comps == 4) {
                glVertex4f(p[0], p[1], p[2], p[3]);
            } else {
                glVertex3f(p[0], p[1], p[2]);
            }
        }
    }
}

// ===========================================================================
// 5. glInterleavedArrays
// ===========================================================================
namespace {

// Cấu hình 1 format của Table 2.8 (spec 4.2C p.74).
//   et/ec/en : có texture coord / color / normal trong format
//   st/sc/sv : số thành phần của texture coord / color / vertex
//   tc       : kiểu dữ liệu của color
//   pc/pn/pv : offset (byte) của color / normal / vertex trong 1 phần tử
//   s        : stride mặc định (byte) khi stride == 0
struct InterleavedCfg {
    bool et;
    bool ec;
    bool en;
    int st;
    int sc;
    int sv;
    GLenum tc;
    int pc;
    int pn;
    int pv;
    int s;
};

// f = sizeof(GLfloat) = 4; c = 4*sizeof(GLubyte) làm tròn lên bội số của f
// ⇒ c = 4 (spec: "c is 4 times sizeof(UNSIGNED_BYTE), rounded up to the
// nearest multiple of f").
constexpr int kF = 4;  // sizeof(GLfloat)
constexpr int kC = 4;  // 4 * sizeof(GLubyte), đã là bội số của sizeof(GLfloat)

// Trả cấu hình theo format; false nếu format không hợp lệ.
bool InterleavedConfig(GLenum format, InterleavedCfg& cfg) {
    switch (format) {
        case GL_V2F:
            cfg = {false, false, false, 2, 0, 2, GL_UNSIGNED_BYTE, 0, 0, 0, 2 * kF};
            return true;
        case GL_V3F:
            cfg = {false, false, false, 3, 0, 3, GL_UNSIGNED_BYTE, 0, 0, 0, 3 * kF};
            return true;
        case GL_C4UB_V2F:
            cfg = {false, true, false, 2, 4, 2, GL_UNSIGNED_BYTE, 0, 0, kC,
                   kC + 2 * kF};
            return true;
        case GL_C4UB_V3F:
            cfg = {false, true, false, 3, 4, 3, GL_UNSIGNED_BYTE, 0, 0, kC,
                   kC + 3 * kF};
            return true;
        case GL_C3F_V3F:
            cfg = {false, true, false, 3, 3, 3, GL_FLOAT, 0, 0, 3 * kF, 6 * kF};
            return true;
        case GL_N3F_V3F:
            cfg = {false, false, true, 3, 0, 3, GL_UNSIGNED_BYTE, 0, 0, 3 * kF,
                   6 * kF};
            return true;
        case GL_C4F_N3F_V3F:
            cfg = {false, true, true, 3, 4, 3, GL_FLOAT, 0, 4 * kF, 7 * kF, 10 * kF};
            return true;
        case GL_T2F_V3F:
            cfg = {true, false, false, 2, 0, 3, GL_UNSIGNED_BYTE, 0, 0, 2 * kF,
                   5 * kF};
            return true;
        case GL_T4F_V4F:
            cfg = {true, false, false, 4, 0, 4, GL_UNSIGNED_BYTE, 0, 0, 4 * kF,
                   8 * kF};
            return true;
        case GL_T2F_C4UB_V3F:
            cfg = {true, true, false, 2, 4, 3, GL_UNSIGNED_BYTE, 2 * kF, 0,
                   kC + 2 * kF, kC + 5 * kF};
            return true;
        case GL_T2F_C3F_V3F:
            cfg = {true, true, false, 2, 3, 3, GL_FLOAT, 2 * kF, 0, 5 * kF, 8 * kF};
            return true;
        case GL_T2F_N3F_V3F:
            cfg = {true, false, true, 2, 0, 3, GL_UNSIGNED_BYTE, 0, 2 * kF, 5 * kF,
                   8 * kF};
            return true;
        case GL_T2F_C4F_N3F_V3F:
            cfg = {true, true, true, 2, 4, 3, GL_FLOAT, 2 * kF, 6 * kF, 9 * kF,
                   12 * kF};
            return true;
        case GL_T4F_C4F_N3F_V4F:
            cfg = {true, true, true, 4, 4, 4, GL_FLOAT, 4 * kF, 8 * kF, 11 * kF,
                   15 * kF};
            return true;
        default:
            return false;
    }
}

}  // namespace

void glInterleavedArrays(GLenum format, GLsizei stride, const void* pointer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glInterleavedArrays")) return;
    InterleavedCfg cfg;
    if (!InterleavedConfig(format, cfg)) {
        c.errors.Record(0x0500);  // GL_INVALID_ENUM
        return;
    }
    if (stride < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return;
    }
    // Spec 4.2C p.75: str = stride; nếu str == 0 ⇒ str = s (stride mặc định).
    const GLsizei str = (stride == 0) ? (GLsizei)cfg.s : stride;
    const uint8_t* base = pointer ? (const uint8_t*)pointer : nullptr;
    // Con trỏ offset trong 1 phần tử (nullptr ⇒ giữ nullptr).
    auto at = [base](int off) -> const void* {
        return base ? (const void*)(base + off) : nullptr;
    };

    // Đúng thứ tự lệnh trong pseudocode của spec:
    //   Disable EDGE_FLAG/INDEX/SECONDARY_COLOR/FOG_COORD, rồi bật/đặt
    //   TEXTURE_COORD, COLOR, NORMAL, VERTEX.
    SetArrayEnabled(c, GL_EDGE_FLAG_ARRAY, false);
    SetArrayEnabled(c, GL_INDEX_ARRAY, false);
    SetArrayEnabled(c, GL_SECONDARY_COLOR_ARRAY, false);
    SetArrayEnabled(c, GL_FOG_COORD_ARRAY, false);

    if (cfg.et) {
        SetArrayEnabled(c, GL_TEXTURE_COORD_ARRAY, true);
        StoreArrayPointer(c, GL_TEXTURE_COORD_ARRAY, cfg.st, GL_FLOAT, str, at(0));
    } else {
        SetArrayEnabled(c, GL_TEXTURE_COORD_ARRAY, false);
    }
    if (cfg.ec) {
        SetArrayEnabled(c, GL_COLOR_ARRAY, true);
        StoreArrayPointer(c, GL_COLOR_ARRAY, cfg.sc, cfg.tc, str, at(cfg.pc));
    } else {
        SetArrayEnabled(c, GL_COLOR_ARRAY, false);
    }
    if (cfg.en) {
        SetArrayEnabled(c, GL_NORMAL_ARRAY, true);
        StoreArrayPointer(c, GL_NORMAL_ARRAY, 3, GL_FLOAT, str, at(cfg.pn));
    } else {
        SetArrayEnabled(c, GL_NORMAL_ARRAY, false);
    }
    SetArrayEnabled(c, GL_VERTEX_ARRAY, true);
    StoreArrayPointer(c, GL_VERTEX_ARRAY, cfg.sv, GL_FLOAT, str, at(cfg.pv));
}

// ===========================================================================
// 6. API cho đường draw (src/gl/gl_draw.cpp gọi) + thống kê cho test
// ===========================================================================
namespace {

// Đọc index thứ k (0-based) từ client pointer hoặc buffer object.
bool ReadIndexAt(Context& c, GLuint ebo, const void* cli, GLenum type,
                 size_t byteOff, GLsizei k, uint32_t& out) {
    size_t elem = 0;
    switch (type) {
        case GL_UNSIGNED_BYTE: elem = 1; break;
        case GL_UNSIGNED_SHORT: elem = 2; break;
        case GL_UNSIGNED_INT: elem = 4; break;
        default: return false;
    }
    if (cli) {
        const uint8_t* p = (const uint8_t*)cli + (size_t)k * elem;
        if (elem == 1) {
            out = *p;
        } else if (elem == 2) {
            uint16_t v;
            memcpy(&v, p, 2);
            out = v;
        } else {
            uint32_t v;
            memcpy(&v, p, 4);
            out = v;
        }
        return true;
    }
    if (!ebo) return false;
    Context::BufferView bv = c.SyncBuffer(ebo);
    if (!bv.p) return false;
    const size_t at = byteOff + (size_t)k * elem;
    if (at + elem > bv.size) return false;
    const uint8_t* p = bv.p + at;
    if (elem == 1) out = *p;
    else if (elem == 2) out = (uint32_t)(p[0] | (p[1] << 8));
    else out = (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
    return true;
}

// Gom 1 đỉnh từ các mảng client đang bật.
bool FetchVertex(Context& c, uint32_t index, LegacyVertex& v) {
    const LegacyArrayPointer& vert = c.legacy.arrayPointers[kSlotVertex];
    if (!vert.enabled) return false;
    if (vert.buffer == 0 && !vert.pointer) return false;
    const int comps = (vert.size >= 2 && vert.size <= 4) ? vert.size : 3;
    float p[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    // VertexPointer = "cast" (Table 2.5) ⇒ KHÔNG chuẩn hoá.
    if (!ReadArrayElement(c, vert, comps, false, index, p)) return false;
    v.pos[0] = p[0];
    v.pos[1] = p[1];
    v.pos[2] = (comps >= 3) ? p[2] : 0.0f;
    v.pos[3] = (comps >= 4) ? p[3] : 1.0f;

    const LegacyArrayPointer& normal = c.legacy.arrayPointers[kSlotNormal];
    if (normal.enabled && (normal.pointer || normal.buffer != 0)) {
        float n[3] = {0.0f, 0.0f, 1.0f};
        // NormalPointer = "normalize" (Table 2.5) ⇒ [-1,1].
        if (ReadArrayElement(c, normal, 3, true, index, n)) {
            v.normal[0] = n[0];
            v.normal[1] = n[1];
            v.normal[2] = n[2];
        }
    }
    const LegacyArrayPointer& color = c.legacy.arrayPointers[kSlotColor];
    if (color.enabled && (color.pointer || color.buffer != 0)) {
        const int cc = color.size >= 3 ? color.size : 3;
        float col[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        // ColorPointer = "normalize" (Table 2.5) ⇒ [0,1] cho kiểu unsigned.
        if (ReadArrayElement(c, color, cc, true, index, col)) {
            v.color[0] = col[0];
            v.color[1] = col[1];
            v.color[2] = col[2];
            v.color[3] = (cc >= 4) ? col[3] : 1.0f;
        }
    }
    const LegacyArrayPointer& sec = c.legacy.arrayPointers[kSlotSecondaryColor];
    if (sec.enabled && (sec.pointer || sec.buffer != 0)) {
        float s[3] = {0.0f, 0.0f, 0.0f};
        if (ReadArrayElement(c, sec, 3, true, index, s)) {
            v.secondaryColor[0] = s[0];
            v.secondaryColor[1] = s[1];
            v.secondaryColor[2] = s[2];
        }
    }
    const LegacyArrayPointer& idxArr = c.legacy.arrayPointers[kSlotIndex];
    if (idxArr.enabled && (idxArr.pointer || idxArr.buffer != 0)) {
        float v0 = 0.0f;
        if (ReadArrayElement(c, idxArr, 1, false, index, &v0))
            v.index = (GLuint)v0;
    }
    const LegacyArrayPointer& edge = c.legacy.arrayPointers[kSlotEdgeFlag];
    if (edge.enabled && (edge.pointer || edge.buffer != 0)) {
        float v0 = 0.0f;
        if (ReadArrayElement(c, edge, 1, false, index, &v0))
            v.edgeFlag = (v0 != 0.0f) ? 1 : 0;
    }
    // Texture coordinate: array phục vụ texture unit clientActiveTexture; layout
    // vertex của đường fixed-function có 2 unit (texCoord / texCoord1 — xem
    // LegacyDraw.h) ⇒ unit 1 đi vào texCoord1, unit khác đi vào texCoord.
    {
        float t[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        if (ReadTexCoordElement(c, index, t)) {
            GLuint u = c.legacy.clientActiveTexture;
            float* dst = (u == 1) ? v.texCoord1 : v.texCoord;
            dst[0] = t[0];
            dst[1] = t[1];
            dst[2] = t[2];
            dst[3] = t[3];
        }
    }
    return true;
}

}  // namespace

// Vẽ bằng client-side vertex arrays (spec 4.2C §2.8).
// Điều kiện áp dụng (xem LegacyClientArrayDrawApplies): fixed-function được
// phép, không có program object trong use, vertex array đang bật và có dữ
// liệu. Trả true nếu draw ĐÃ được phát.
bool LegacyClientArrayDrawApplies(const Context& c) {
    // (a) Core Profile không có fixed-function ⇒ client array không dùng được.
    if (!LegacyFixedFunctionAllowed(c)) return false;
    // (b) Có program object trong use ⇒ programmable pipeline chạy, client
    //     array bị bỏ qua (GL 2.1 §2.15 / GL 4.6 Core §1.2.4).
    if (c.state.BoundProgram() != 0) return false;
    // (c) Phải có vertex array đang bật và có dữ liệu.
    const LegacyArrayPointer& vert = c.legacy.arrayPointers[kSlotVertex];
    if (!vert.enabled) return false;
    if (vert.buffer == 0 && !vert.pointer) return false;
    return true;
}

// Vẽ bằng client-side vertex arrays (spec 4.2C §2.8).
// Trả true nếu draw ĐÃ được xử lý (đã phát draw) — caller (gl_draw.cpp)
// phải return ngay và KHÔNG đi tiếp vào đường draw modern.
bool LegacyDrawClientArrays(Context& c, GLenum mode, GLint first, GLsizei count,
                             GLenum indexType, const void* indexData,
                             GLint baseVertex) {
    ClientArrayStats& e = Stats(c);
    e.lastVerticesFetched = 0;
    e.lastIndicesFetched = 0;
    e.lastUsedClientMemory = false;
    e.lastDrew = false;
    e.lastActive = false;

    if (!LegacyClientArrayDrawApplies(c)) return false;
    if (count <= 0) return false;
    // first < 0 ⇒ undefined theo spec; TGLMT sinh INVALID_VALUE (giống
    // đường draw modern trong gl_draw.cpp).
    if (indexType == 0 && first < 0) {
        c.errors.Record(0x0501);  // GL_INVALID_VALUE
        return false;
    }

    // Index: đọc từ EBO (nếu có) hoặc client pointer.
    std::vector<uint32_t> indices;
    const void* cli = nullptr;
    GLuint ebo = 0;
    size_t eboOff = 0;
    if (indexType != 0) {
        auto vaoIt = c.vaos.find(c.state.BoundVAO());
        if (vaoIt != c.vaos.end()) ebo = vaoIt->second.elementBuffer;
        if (!ebo) ebo = c.state.BoundBuffer(GL_ELEMENT_ARRAY_BUFFER);
        if (ebo && c.buffers.find(ebo) != c.buffers.end()) {
            eboOff = (size_t)indexData;  // indexData là byte offset trong EBO
        } else {
            ebo = 0;
            cli = indexData;
            if (!cli) return false;  // không có nguồn index ⇒ bỏ draw
        }
        indices.resize((size_t)count);
        for (GLsizei k = 0; k < count; ++k) {
            uint32_t v = 0;
            if (!ReadIndexAt(c, ebo, cli, indexType, eboOff, k, v)) return false;
            indices[(size_t)k] = v + (uint32_t)baseVertex;
        }
        e.lastIndicesFetched = (uint32_t)count;
    } else {
        indices.resize((size_t)count);
        for (GLsizei k = 0; k < count; ++k)
            indices[(size_t)k] = (uint32_t)(first + k);
    }
    e.lastActive = true;

    // Gom đỉnh.
    std::vector<LegacyVertex> verts;
    verts.reserve(indices.size());
    for (size_t k = 0; k < indices.size(); ++k) {
        LegacyVertex v;
        if (!FetchVertex(c, indices[k], v)) return false;  // dữ liệu không hợp lệ
        verts.push_back(v);
    }
    e.lastVerticesFetched = (uint32_t)verts.size();

    // Client memory hay VBO? (spec: pointer là offset trong ARRAY_BUFFER nếu
    // buffer đang bind, ngược lại là con trỏ client).
    bool clientMem = false;
    for (int i = 0; i < 8; ++i) {
        const LegacyArrayPointer& a = c.legacy.arrayPointers[i];
        if (a.enabled && a.buffer == 0) clientMem = true;
    }
    e.lastUsedClientMemory = clientMem;

    // Đưa vào batch fixed-function rồi vẽ qua đường draw legacy
    // (src/gl/gl_legacy_draw.cpp) để có lighting/fog/texenv như immediate mode.
    LegacyFlushPendingImmediate(c);
    c.legacy.immediateVertices = verts;
    c.legacy.immediatePrimitives.clear();
    c.legacy.beginMode = mode;
    AssemblePrimitives(c.legacy);
    c.legacy.drawnPrims = 0;
    c.legacy.drawnVerts = 0;
    const bool ok = LegacyDrawImmediate(c);
    e.lastDrew = ok;
    return ok;
}

// ---------------------------------------------------------------------------
// Thống kê draw gần nhất (test + chẩn đoán). Không có header riêng (task này
// không được thêm header) ⇒ các hàm scalar để test gọi trực tiếp.
// ---------------------------------------------------------------------------
uint32_t LegacyClientArrayVerticesFetched() { return g_stats.lastVerticesFetched; }
uint32_t LegacyClientArrayIndicesFetched() { return g_stats.lastIndicesFetched; }
bool LegacyClientArrayUsedClientMemory() { return g_stats.lastUsedClientMemory; }
bool LegacyClientArrayDrew() { return g_stats.lastDrew; }
bool LegacyClientArrayActive() { return g_stats.lastActive; }

}  // namespace tglmt::gl
