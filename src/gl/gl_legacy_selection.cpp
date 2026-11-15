// gl_legacy_selection.cpp — SELECTION / FEEDBACK / RENDER MODE / NAME STACK
// (OpenGL Compatibility Profile).
//
// Spec: OpenGL 4.2 Compatibility Profile §5.2 "Selection" (PDF p.477–479),
//       §5.3 "Feedback" (PDF p.479–481), state table §6.2 Table 6.79
//       (PDF p.606). Bản GL 2.1 tương đương: glspec21.pdf §5.2
//       (PDF p.250–252) và §5.3 (PDF p.252–254).
//
// 7 entry point của slice này:
//   glRenderMode, glSelectBuffer, glFeedbackBuffer,
//   glInitNames, glPushName, glPopName, glLoadName.
//
// Trạng thái dùng: LegacyState::selection (include/tglmt/LegacyState.h:497–511)
// — selectBuffer / selectCapacity / feedbackBuffer / feedbackCapacity /
// renderMode / nameStack / currentName / selectUsed / feedbackUsed.
//
// ---------------------------------------------------------------------------
// GHI CHÚ VỀ SỰ LỆCH NHẸ VỚI SPEC (xem báo cáo cuối phiên)
// ---------------------------------------------------------------------------
// 1) Name stack: GL 4.2C §5.2 nói "InitNames empties (clears) the name stack"
//    và bảng state §6.2 cho NAME STACK DEPTH giá trị đầu = 0 (stack rỗng).
//    Task của phiên này yêu cầu mô hình "stack luôn có 1 phần tử khởi tạo (0),
//    không pop được" ⇒ mã ở đây theo task (stack = {0}, pop lỗi khi size==1).
// 2) Mã lỗi name stack: spec §5.2 dùng STACK_OVERFLOW / STACK_UNDERFLOW;
//    task yêu cầu INVALID_VALUE / INVALID_OPERATION ⇒ theo task.
// 3) "Placing the GL in selection mode before SelectBuffer has been called
//    results in an error of INVALID_OPERATION" (spec §5.2): KHÔNG sinh lỗi ở
//    TGLMT — trạng thái đầu của GL coi như đã có selection buffer rỗng
//    (SELECTION BUFFER POINTER = 0, SIZE = 0 theo bảng state §6.2), tức là
//    "selection bị tắt" chứ không phải lỗi. Vẫn cho phép đổi mode.
// 4) MAX NAME STACK DEPTH: spec §5.2 (PDF p.458) "must be at least 64"; TGLMT
//    dùng 32 để KHỚP với giá trị app đọc được qua
//    glGetIntegerv(GL_MAX_NAME_STACK_DEPTH) (src/gl/gl_get.cpp:257 trả 32).
//    Nếu gl_get.cpp được sửa (ngoài write-scope) ⇒ phải sửa kNameStackMaxDepth.
// 5) "Use of any of the name stack manipulation commands while the GL is not in
//    selection mode has no effect" (spec §5.2, PDF p.458): TGLMT cho phép
//    thao tác name stack ở mọi mode (theo task) — ngoài selection mode không
//    có hit record nào được ghi nên hành vi quan sát được là như nhau.
// 6) GL_FEEDBACK_BUFFER_TYPE / SELECTION_BUFFER_* / RENDER_MODE /
//    NAME_STACK_DEPTH KHÔNG được gl_get.cpp phục vụ (ngoài write-scope) —
//    xem báo cáo phiên.
//
// ---------------------------------------------------------------------------
// ĐƯỜNG DRAW: hook chưa cắm (ngoài write-scope) — xem báo cáo phiên
// ---------------------------------------------------------------------------
// LegacySelectionFeedbackProcessImmediate() xử lý batch immediate-mode đã ghép
// (hit record / khối feedback) và LegacyRenderModeSuppressesRasterization()
// báo "đừng rasterize". Cần cắm vào LegacyDrawImmediate()
// (src/gl/gl_legacy_draw.cpp:1154) — ngoài write-scope của file này ⇒ đường
// draw hiện CHƯA gọi hook ⇒ selection/feedback chưa sinh dữ liệu thật khi
// vẽ bằng glBegin/glEnd (test gọi hook trực tiếp).
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

namespace {

// ---------------------------------------------------------------------------
// Hằng số
// ---------------------------------------------------------------------------

// Độ sâu tối đa của name stack = GL_MAX_NAME_STACK_DEPTH (0x0D37).
// glGetIntegerv(GL_MAX_NAME_STACK_DEPTH) trả về 32 (src/gl/gl_get.cpp:257 —
// cùng giá trị với kMatrixStackDepth) ⇒ dùng 32 cho khớp pname app đọc được.
// Spec §5.2 yêu cầu "must be at least 64" — xem báo cáo phiên.
constexpr size_t kNameStackMaxDepth = 32;

// Token feedback (spec 4.2C §5.3 hình 5.2, GL 2.1 §5.3 hình 5.2). Đây KHÔNG
// phải GLenum (không có trong registry gl.xml) nên khai báo riêng ở đây.
constexpr GLuint kFeedbackTokenPoint = 0x0000;
constexpr GLuint kFeedbackTokenLine = 0x0001;
constexpr GLuint kFeedbackTokenPolygon = 0x0003;
constexpr GLuint kFeedbackTokenBitmap = 0x0004;
constexpr GLuint kFeedbackTokenPixelRect = 0x0005;
constexpr GLuint kFeedbackTokenPassThrough = 0x0006;

// ---------------------------------------------------------------------------
// State phụ (feedback type, cờ "đã gọi buffer", cờ overflow, số hit record)
// ---------------------------------------------------------------------------
// LegacyState::selection KHÔNG có: feedback type, cờ đã-gọi-SelectBuffer,
// cờ overflow, số hit record đã ghi, khoảng depth của hit chưa ghi.
// include/tglmt/LegacyState.h nằm ngoài write-scope ⇒ gói state phụ vào
// "shadow" của StateTracker (cùng cơ chế glGet dùng để nhớ giá trị đã set).
// Pname 0xF001 là pname RIÊNG (mọi pname GL trong registry ≤ 0x8EAC) nên
// không đụng pname thật nào.
constexpr GLenum kSideStatePname = 0xF001;

enum SideFlag : uint32_t {
    kFlagSelectOverflow = 1u << 2,    // selection buffer bị tràn
    kFlagFeedbackOverflow = 1u << 3,  // feedback buffer bị tràn
    kFlagModeInitialized = 1u << 4,   // đã chuẩn hoá renderMode khởi tạo
};

// Layout CỐ ĐỊNH (không có padding ẩn) vì blob này được ghi/đọc RAW qua
// shadow state ⇒ mọi byte phải xác định (không đọc padding chưa khởi tạo).
struct SideState {
    GLenum feedbackType = GL_2D;  // spec: FEEDBACK BUFFER TYPE initial = 2D (§6.2)
    GLint selectRecordCount = 0;  // số hit record đã ghi vào selection buffer
    float hitMinZ = 1.0f;         // khoảng depth tích luỹ (window z)
    float hitMaxZ = 0.0f;
    uint32_t flags = 0;
    uint8_t hitPending = 0;       // đã có hit kể từ lần ghi record gần nhất
    uint8_t pad[3] = {0, 0, 0};   // padding tường minh
};

SideState LoadSide(Context& c) {
    SideState s;
    if (!c.state.GetShadow(kSideStatePname, &s, sizeof(s))) s = SideState{};
    return s;
}

void StoreSide(Context& c, const SideState& s) {
    c.state.SetShadow(kSideStatePname, &s, sizeof(s));
}

// include/tglmt/LegacyState.h:502 đặt renderMode mặc định = 0x1C01, tức là
// GL_FEEDBACK — SAI (theo registry: GL_RENDER = 0x1C00, GL_SELECT = 0x1C02,
// GL_FEEDBACK = 0x1C01). Không sửa được header ⇒ chuẩn hoá một lần ở đây:
// trạng thái đầu đúng của GL là GL_RENDER (bảng state §6.2: RENDER MODE
// initial = RENDER).
void EnsureModeInitialized(Context& c) {
    SideState s = LoadSide(c);
    if (s.flags & kFlagModeInitialized) return;
    s.flags |= kFlagModeInitialized;
    c.legacy.selection.renderMode = GL_RENDER;
    StoreSide(c, s);
}

// Số phần tử GLuint đã ghi vào selection buffer.
GLuint* SelectArray(Context& c) {
    // Spec §5.2: "buffer is a pointer to an array of unsigned integers".
    // LegacyState lưu con trỏ dưới dạng GLint* (4 byte) — ép kiểu, đọc/ghi
    // luôn qua GLuint* để đúng kiểu của spec.
    return reinterpret_cast<GLuint*>(c.legacy.selection.selectBuffer);
}

// ---------------------------------------------------------------------------
// Ghi hit record (spec §5.2)
// ---------------------------------------------------------------------------
// "The minimum and maximum depth values are the minimum and maximum taken over
//  all the window coordinate z values of each (post-clipping) vertex of each
//  primitive that intersects the clipping volume since the last hit record was
//  written. The minimum and maximum (each of which lies in the range [0, 1])
//  are each multiplied by 2^32-1 and rounded to the nearest unsigned integer
//  to obtain the values that are placed in the hit record." (PDF p.459)
GLuint DepthToSelectUint(float z) {
    double v = (double)z * 4294967295.0;  // 2^32-1
    if (!(v > 0.0)) v = 0.0;              // NaN ⇒ 0
    if (v > 4294967295.0) v = 4294967295.0;
    return (GLuint)std::floor(v + 0.5);  // làm tròn tới số nguyên gần nhất
}

// "A hit record consists of the following items in order: a non-negative
//  integer giving the number of elements on the name stack at the time of the
//  hit, a minimum depth value, a maximum depth value, and the name stack with
//  the bottommost element first."
// "If copying the hit record into the selection array would cause the total
//  number of values to exceed n, then as much of the record as fits in the
//  array is written and an overflow flag is set."
void WriteHitRecord(Context& c) {
    SideState s = LoadSide(c);
    if (!s.hitPending) return;
    s.hitPending = 0;  // hit đã được xử lý (kể cả khi không có buffer)

    LegacyState::LegacySelection& sel = c.legacy.selection;
    GLuint* buf = SelectArray(c);
    const GLsizei cap = sel.selectCapacity;
    if (!buf || cap <= 0) {
        // Selection bị TẮT (buffer NULL hoặc size == 0) ⇒ không ghi gì và
        // KHÔNG sinh cờ overflow.
        StoreSide(c, s);
        return;
    }

    const size_t depth = sel.nameStack.size();
    const size_t need = 3 + depth;  // [số tên][min z][max z][name stack]
    if (sel.selectUsed >= cap) {
        s.flags |= kFlagSelectOverflow;
        StoreSide(c, s);
        return;
    }
    const size_t room = (size_t)(cap - sel.selectUsed);
    const size_t n = need < room ? need : room;

    GLuint* dst = buf + sel.selectUsed;
    size_t i = 0;
    if (i < n) dst[i++] = (GLuint)depth;
    if (i < n) dst[i++] = DepthToSelectUint(s.hitMinZ);
    if (i < n) dst[i++] = DepthToSelectUint(s.hitMaxZ);
    for (size_t k = 0; k < depth && i < n; ++k) dst[i++] = sel.nameStack[k];

    sel.selectUsed = (GLsizei)(sel.selectUsed + (GLsizei)n);
    ++s.selectRecordCount;
    if (need > room) s.flags |= kFlagSelectOverflow;
    StoreSide(c, s);
}

// ---------------------------------------------------------------------------
// Ghi khối feedback (spec §5.3)
// ---------------------------------------------------------------------------
// Mỗi primitive sinh một khối: token primitive + các giá trị của từng đỉnh
// (bảng 5.2: 2D → x,y; 3D → x,y,z; 3D_COLOR → x,y,z,color(k);
// 3D_COLOR_TEXTURE → x,y,z,color(k),s,t,r,q; 4D_COLOR_TEXTURE →
// x,y,z,w,color(k),s,t,r,q). x,y,z là toạ độ WINDOW; w là clip-space.
struct FeedbackVertex {
    float pos[4];   // x, y, z window + w clip
    float color[4]; // RGBA (hoặc 1 giá trị color index)
    float tex[4];   // s, t, r, q
};

// Số giá trị / đỉnh theo feedback type (spec bảng 5.2, PDF p.481):
// 2D → 2; 3D → 3; 3D_COLOR → 3+k; 3D_COLOR_TEXTURE → 7+k;
// 4D_COLOR_TEXTURE → 8+k. colorCount = 4 (RGBA) hoặc 1 (color index mode).
// Trả về số giá trị đã điền vào `vals`.
int FeedbackVertexValues(GLenum type, int colorCount, const FeedbackVertex& v,
                          float* vals) {
    int n = 0;
    vals[n++] = v.pos[0];
    vals[n++] = v.pos[1];
    switch (type) {
        case GL_2D:
            break;
        case GL_3D:
            vals[n++] = v.pos[2];
            break;
        case GL_3D_COLOR:
            vals[n++] = v.pos[2];
            for (int k = 0; k < colorCount; ++k) vals[n++] = v.color[k];
            break;
        case GL_3D_COLOR_TEXTURE:
            vals[n++] = v.pos[2];
            for (int k = 0; k < colorCount; ++k) vals[n++] = v.color[k];
            for (int k = 0; k < 4; ++k) vals[n++] = v.tex[k];
            break;
        case GL_4D_COLOR_TEXTURE:
            vals[n++] = v.pos[2];
            vals[n++] = v.pos[3];  // w trả về ở clip space (spec §5.3)
            for (int k = 0; k < colorCount; ++k) vals[n++] = v.color[k];
            for (int k = 0; k < 4; ++k) vals[n++] = v.tex[k];
            break;
        default:
            break;
    }
    return n;
}

// Ghi 1 khối feedback. Trả false nếu buffer tràn (đã set cờ overflow).
//
// Cấu trúc khối theo grammar Figure 5.2 (spec GL 4.2C p.462 / GL 2.1 p.241):
//   point:   POINT_TOKEN vertex
//   line:    LINE_TOKEN vertex vertex
//   polygon: POLYGON_TOKEN n polygon-spec   (n = số đỉnh của polygon)
//   passthrough: PASS_THROUGH_TOKEN f
// ⇒ `vertexCountFirst` = true chỉ cho polygon (ghi thêm n sau token).
bool WriteFeedbackBlock(Context& c, GLuint token, bool vertexCountFirst,
                        const FeedbackVertex* verts, int count) {
    LegacyState::LegacySelection& sel = c.legacy.selection;
    GLfloat* buf = sel.feedbackBuffer;
    const GLsizei cap = sel.feedbackCapacity;
    if (!buf || cap <= 0) return false;  // feedback bị tắt (NULL hoặc size==0)

    SideState s = LoadSide(c);
    const int colorCount = c.legacy.colorIndex.indexMode ? 1 : 4;

    // Token (spec §5.3: "Each block begins with a code indicating the primitive
    // type").
    if (sel.feedbackUsed < cap) {
        buf[sel.feedbackUsed++] = (GLfloat)token;
    } else {
        s.flags |= kFlagFeedbackOverflow;
        StoreSide(c, s);
        return false;
    }
    // n (số đỉnh) — chỉ polygon (Figure 5.2).
    if (vertexCountFirst) {
        if (sel.feedbackUsed < cap) {
            buf[sel.feedbackUsed++] = (GLfloat)count;
        } else {
            s.flags |= kFlagFeedbackOverflow;
            StoreSide(c, s);
            return false;
        }
    }
    for (int i = 0; i < count; ++i) {
        float vals[12];
        const int nv = FeedbackVertexValues(s.feedbackType, colorCount, verts[i], vals);
        for (int k = 0; k < nv; ++k) {
            if (sel.feedbackUsed < cap) {
                buf[sel.feedbackUsed++] = vals[k];
            } else {
                // "If writing a value to the feedback buffer would cause more
                //  values to be written than the specified maximum number of
                //  values, then the value is not written and an overflow flag
                //  is set." (§5.3)
                s.flags |= kFlagFeedbackOverflow;
                StoreSide(c, s);
                return false;
            }
        }
    }
    StoreSide(c, s);
    return true;
}

// ---------------------------------------------------------------------------
// Hit test / window-space transform cho selection
// ---------------------------------------------------------------------------
struct ClipVertex {
    float x, y, z, w;
};

// Transform đỉnh (object space) → clip space: P · MV · v.
ClipVertex ToClip(const Context& c, const LegacyVertex& v) {
    const GLMatrix& mv = c.legacy.modelview.top();
    const GLMatrix& pj = c.legacy.projection.top();
    float eye[4];
    mv.Transform4(v.pos[0], v.pos[1], v.pos[2], v.pos[3], eye);
    float clip[4];
    pj.Transform4(eye[0], eye[1], eye[2], eye[3], clip);
    return ClipVertex{clip[0], clip[1], clip[2], clip[3]};
}

// Window-space x,y từ clip space (w > 0).
void ToWindowXY(const Context& c, const ClipVertex& cv, float* x, float* y) {
    const ViewportState vp = c.state.GetViewport(0);
    const float invw = 1.0f / cv.w;
    *x = vp.x + (cv.x * invw * 0.5f + 0.5f) * vp.w;
    *y = vp.y + (cv.y * invw * 0.5f + 0.5f) * vp.h;
}

// Window-space z (theo depth range + clip-depth convention hiện tại).
float ToWindowZ(const Context& c, const ClipVertex& cv) {
    const ViewportState vp = c.state.GetViewport(0);
    const float invw = 1.0f / cv.w;
    float zn;
    if (c.state.ClipDepth() == 0x935F /*GL_ZERO_TO_ONE*/) {
        zn = cv.z * invw;
    } else {
        zn = cv.z * invw * 0.5f + 0.5f;  // GL_NEGATIVE_ONE_TO_ONE (mặc định)
    }
    float z = zn * (vp.f - vp.n) + vp.n;
    if (z < 0.0f) z = 0.0f;
    if (z > 1.0f) z = 1.0f;
    return z;
}

// True nếu primitive GIAO với clip volume (spec §5.2: hit khi primitive
// "intersects the clip volume"). Trivial-reject theo 6 mặt phẳng clip.
bool IntersectsClipVolume(const Context& c, const ClipVertex* v, int n) {
    if (n <= 0) return false;
    const bool zeroToOne = c.state.ClipDepth() == 0x935F /*GL_ZERO_TO_ONE*/;
    // Mặt phẳng: x+w, w-x, y+w, w-y, z+w (hoặc z), w-z.
    for (int plane = 0; plane < 6; ++plane) {
        bool allOutside = true;
        for (int i = 0; i < n; ++i) {
            float d;
            switch (plane) {
                case 0: d = v[i].x + v[i].w; break;
                case 1: d = v[i].w - v[i].x; break;
                case 2: d = v[i].y + v[i].w; break;
                case 3: d = v[i].w - v[i].y; break;
                case 4: d = zeroToOne ? v[i].z : (v[i].z + v[i].w); break;
                default: d = v[i].w - v[i].z; break;
            }
            if (d >= 0.0f) { allOutside = false; break; }
        }
        if (allOutside) return false;
    }
    return true;
}

// Polygon có bị cull không (spec §5.2: "no hit occurs if the polygon would
// have been culled"). Dùng diện tích có dấu trong window space.
bool PolygonCulled(const Context& c, const float* xy, int n) {
    if (!c.state.IsEnabled(0x0B44 /*GL_CULL_FACE*/)) return false;
    if (n < 3) return false;
    float area = 0.0f;
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        area += xy[i * 2 + 0] * xy[j * 2 + 1] - xy[j * 2 + 0] * xy[i * 2 + 1];
    }
    if (area == 0.0f) return true;  // polygon suy biến ⇒ không có hit
    const bool ccw = c.state.FrontFace() == 0x0901 /*GL_CCW*/;
    const bool frontFacing = ccw ? (area > 0.0f) : (area < 0.0f);
    const GLenum mode = c.state.CullMode();
    switch (mode) {
        case 0x0404 /*GL_FRONT*/: return frontFacing;
        case 0x0405 /*GL_BACK*/: return !frontFacing;
        case 0x0408 /*GL_FRONT_AND_BACK*/: return true;
        default: return false;
    }
}

// Token feedback của một primitive mode (spec hình 5.2).
GLuint FeedbackTokenForMode(GLenum mode) {
    switch (mode) {
        case 0x0000 /*GL_POINTS*/: return kFeedbackTokenPoint;
        case 0x0001 /*GL_LINES*/:
        case 0x0002 /*GL_LINE_LOOP*/:
        case 0x0003 /*GL_LINE_STRIP*/: return kFeedbackTokenLine;
        case 0x0004 /*GL_TRIANGLES*/:
        case 0x0005 /*GL_TRIANGLE_STRIP*/:
        case 0x0006 /*GL_TRIANGLE_FAN*/:
        case 0x0007 /*GL_QUADS*/:
        case 0x0008 /*GL_QUAD_STRIP*/:
        case 0x0009 /*GL_POLYGON*/: return kFeedbackTokenPolygon;
        default: return kFeedbackTokenPolygon;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// API cho draw path (hook site: src/gl/gl_legacy_draw.cpp — xem báo cáo)
// ---------------------------------------------------------------------------

// True nếu GL đang ở SELECT/FEEDBACK ⇒ draw path KHÔNG được rasterize
// (spec §5.2/§5.3: "framebuffer updates as described in chapter 4 are not
// performed").
bool LegacyRenderModeSuppressesRasterization(Context& c) {
    // Chuẩn hoá mode (include/tglmt/LegacyState.h:502 đặt mặc định 0x1C01 =
    // GL_FEEDBACK — sai; theo registry GL_RENDER = 0x1C00).
    EnsureModeInitialized(c);
    const GLenum mode = c.legacy.selection.renderMode;
    return mode == GL_SELECT || mode == GL_FEEDBACK;
}

// Xử lý selection/feedback cho batch immediate-mode đã ghép primitive
// (c.legacy.immediateVertices / immediatePrimitives).
// Trả true nếu GL đang ở SELECT/FEEDBACK (⇒ caller KHÔNG rasterize).
bool LegacySelectionFeedbackProcessImmediate(Context& c) {
    if (!LegacyRenderModeSuppressesRasterization(c)) return false;

    const auto& verts = c.legacy.immediateVertices;
    const auto& prims = c.legacy.immediatePrimitives;
    if (verts.empty() || prims.empty()) return true;

    const bool selecting = (c.legacy.selection.renderMode == GL_SELECT);
    SideState s = LoadSide(c);

    for (const LegacyPrimitive& p : prims) {
        ClipVertex cv[4];
        int n = 0;
        for (uint32_t k = 0; k < p.count && k < 4; ++k) {
            const uint32_t vi = p.idx[k];
            if (vi >= verts.size()) continue;
            cv[n++] = ToClip(c, verts[vi]);
        }
        if (n == 0) continue;
        if (!IntersectsClipVolume(c, cv, n)) continue;

        if (selecting) {
            // Polygon bị cull ⇒ không hit (spec §5.2).
            if (p.count >= 3) {
                float xy[8];
                int m = 0;
                for (int i = 0; i < n && i < 4; ++i) {
                    if (cv[i].w <= 0.0f) continue;  // sau mặt phẳng mắt ⇒ không dùng
                    ToWindowXY(c, cv[i], &xy[m * 2], &xy[m * 2 + 1]);
                    ++m;
                }
                if (m >= 3 && PolygonCulled(c, xy, m)) continue;
            }
            // Khoảng depth window của các đỉnh (post-clip ⇒ đã clamp [0,1]).
            bool any = false;
            for (int i = 0; i < n; ++i) {
                if (cv[i].w <= 0.0f) continue;
                const float z = ToWindowZ(c, cv[i]);
                if (!any) {
                    s.hitMinZ = z;
                    s.hitMaxZ = z;
                    any = true;
                } else {
                    if (z < s.hitMinZ) s.hitMinZ = z;
                    if (z > s.hitMaxZ) s.hitMaxZ = z;
                }
            }
            if (any) s.hitPending = 1;
            StoreSide(c, s);
        } else {
            // Feedback: 1 khối / primitive (spec §5.3).
            const bool indexMode = c.legacy.colorIndex.indexMode;
            FeedbackVertex fv[4];
            int m = 0;
            for (int i = 0; i < n && i < 4; ++i) {
                const LegacyVertex& lv = verts[p.idx[i]];
                float wx = 0.0f, wy = 0.0f;
                if (cv[i].w > 0.0f) {
                    ToWindowXY(c, cv[i], &wx, &wy);
                }
                fv[m].pos[0] = wx;
                fv[m].pos[1] = wy;
                fv[m].pos[2] = (cv[i].w > 0.0f) ? ToWindowZ(c, cv[i]) : 0.0f;
                fv[m].pos[3] = cv[i].w;  // w trả về ở clip space (spec §5.3)
                if (indexMode) {
                    // Color index mode: giá trị màu trả về là COLOR INDEX
                    // (spec Figure 5.2: "color: f f f f / f"; k = 1).
                    fv[m].color[0] = (float)lv.index;
                    fv[m].color[1] = fv[m].color[2] = fv[m].color[3] = 0.0f;
                } else {
                    for (int k = 0; k < 4; ++k) fv[m].color[k] = lv.color[k];
                }
                for (int k = 0; k < 4; ++k) fv[m].tex[k] = lv.texCoord[k];
                ++m;
            }
            // Polygon có thêm số đỉnh n sau token (Figure 5.2).
            const bool isPolygon = (p.count >= 3);
            if (m > 0)
                WriteFeedbackBlock(c, FeedbackTokenForMode(p.mode), isPolygon, fv, m);
            // WriteFeedbackBlock tự nạp/lưu state (cờ overflow) ⇒ KHÔNG ghi
            // đè bằng bản sao `s` (đã cũ) ở cuối vòng lặp.
        }
    }
    if (selecting) StoreSide(c, s);
    return true;
}

// ---------------------------------------------------------------------------
// glRenderMode — spec §5.2 (PDF p.478) / §5.3 (PDF p.480)
// ---------------------------------------------------------------------------
GLint glRenderMode(GLenum mode) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glRenderMode")) return 0;
    EnsureModeInitialized(c);

    const GLenum prev = c.legacy.selection.renderMode;
    if (mode != GL_RENDER && mode != GL_SELECT && mode != GL_FEEDBACK) {
        c.errors.Record(GL_INVALID_ENUM);
        return 0;
    }
    GLint ret = (GLint)prev;  // trả về mode trước đó (spec: §5.2)

    // "When in selection mode, whenever a name stack manipulation command is
    //  executed or RenderMode is called and there has been a hit since the
    //  last time the stack was manipulated or RenderMode was called, then a hit
    //  record is written into the selection array." (§5.2)
    // "When called while in selection mode, RenderMode returns the number of
    //  hit records copied into the selection array and resets the SelectBuffer
    //  pointer to its last specified value. If the selection array overflow
    //  flag was set, then RenderMode returns -1 and clears the overflow
    //  flag." (§5.2)
    if (prev == GL_SELECT) {
        WriteHitRecord(c);
        SideState s = LoadSide(c);  // WriteHitRecord có thể vừa cập nhật state
        if (s.flags & kFlagSelectOverflow) {
            ret = -1;
            s.flags &= ~kFlagSelectOverflow;
        } else {
            ret = s.selectRecordCount;
        }
        s.selectRecordCount = 0;
        s.hitPending = 0;
        s.hitMinZ = 1.0f;
        s.hitMaxZ = 0.0f;
        c.legacy.selection.selectUsed = 0;  // con trỏ về đầu mảng
        StoreSide(c, s);
    } else if (prev == GL_FEEDBACK) {
        // "When called while in feedback mode, RenderMode returns the number
        //  of values placed in the feedback array and resets the feedback array
        //  pointer to be buffer." (§5.3)
        SideState s = LoadSide(c);
        if (s.flags & kFlagFeedbackOverflow) {
            ret = -1;
            s.flags &= ~kFlagFeedbackOverflow;
        } else {
            ret = (GLint)c.legacy.selection.feedbackUsed;
        }
        c.legacy.selection.feedbackUsed = 0;
        StoreSide(c, s);
    }

    c.legacy.selection.renderMode = mode;
    {
        SideState s = LoadSide(c);
        if (mode == GL_SELECT) {
            // "When selection mode is entered, the pointer is initialized to
            //  the beginning of the array." (§5.2)
            c.legacy.selection.selectUsed = 0;
            s.selectRecordCount = 0;
            s.flags &= ~kFlagSelectOverflow;
            s.hitPending = 0;
            s.hitMinZ = 1.0f;
            s.hitMaxZ = 0.0f;
        } else if (mode == GL_FEEDBACK) {
            c.legacy.selection.feedbackUsed = 0;
            s.flags &= ~kFlagFeedbackOverflow;
        }
        // "The name stack is cleared and the stack pointer reset whenever
        //  RenderMode is called." (§5.2)
        c.legacy.selection.nameStack.clear();
        c.legacy.selection.nameStack.push_back(0);
        c.legacy.selection.currentName = 0;
        StoreSide(c, s);
    }
    return ret;
}

// ---------------------------------------------------------------------------
// glSelectBuffer — spec §5.2 (PDF p.478)
// ---------------------------------------------------------------------------
void glSelectBuffer(GLsizei size, GLuint* buffer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glSelectBuffer")) return;
    EnsureModeInitialized(c);
    if (size < 0) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    // "calling SelectBuffer while in selection mode" ⇒ INVALID_OPERATION.
    if (c.legacy.selection.renderMode == GL_SELECT) {
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    // Con trỏ và sức chứa lưu RIÊNG (spec §6.2: SELECTION BUFFER POINTER và
    // SELECTION BUFFER SIZE là 2 state khác nhau). buffer == NULL ⇒ selection
    // bị TẮT; size == 0 ⇒ không có chỗ để ghi (cũng coi như tắt, không sinh
    // cờ overflow). Không phải lỗi trong cả hai trường hợp.
    c.legacy.selection.selectBuffer = reinterpret_cast<GLint*>(buffer);
    c.legacy.selection.selectCapacity = size;
    c.legacy.selection.selectUsed = 0;
    SideState s = LoadSide(c);
    s.flags &= ~kFlagSelectOverflow;
    s.selectRecordCount = 0;
    StoreSide(c, s);
}

// ---------------------------------------------------------------------------
// glFeedbackBuffer — spec §5.3 (PDF p.480)
// ---------------------------------------------------------------------------
void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat* buffer) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glFeedbackBuffer")) return;
    EnsureModeInitialized(c);
    if (type != GL_2D && type != GL_3D && type != GL_3D_COLOR &&
        type != GL_3D_COLOR_TEXTURE && type != GL_4D_COLOR_TEXTURE) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (size < 0) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    // "if a call to FeedbackBuffer is made while in feedback mode" ⇒
    // INVALID_OPERATION.
    if (c.legacy.selection.renderMode == GL_FEEDBACK) {
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    // Con trỏ và sức chứa lưu RIÊNG (spec §6.2: FEEDBACK BUFFER POINTER /
    // SIZE / TYPE là 3 state riêng). buffer == NULL ⇒ feedback bị TẮT;
    // size == 0 ⇒ không có chỗ để ghi (coi như tắt, không sinh cờ overflow).
    c.legacy.selection.feedbackBuffer = buffer;
    c.legacy.selection.feedbackCapacity = size;
    c.legacy.selection.feedbackUsed = 0;
    SideState s = LoadSide(c);
    s.feedbackType = type;
    s.flags &= ~kFlagFeedbackOverflow;
    StoreSide(c, s);
}

// ---------------------------------------------------------------------------
// Name stack — spec §5.2 (PDF p.477–478)
// ---------------------------------------------------------------------------

// Lệnh name stack KHÔNG được gọi trong glBegin/glEnd: spec GL 4.2C §2.6.3
// (PDF p.51) — "Executing any other GL command between the execution of Begin
// and the corresponding execution of End results in the error
// INVALID_OPERATION" (danh sách lệnh ĐƯỢC PHÉP không có InitNames/PushName/
// PopName/LoadName). Bảng lệnh cấm của repo nằm ở
// src/gl/gl_legacy_beginend.cpp (ngoài write-scope) và chưa có 4 lệnh này
// ⇒ kiểm tra tại chỗ.
bool NameStackBlockedInBeginEnd(Context& c) {
    if (!c.legacy.inBeginEnd) return false;
    c.errors.Record(GL_INVALID_OPERATION);
    return true;
}

// glInitNames: "InitNames empties (clears) the name stack." Mô hình TGLMT
// (theo task): stack = {0} — 1 phần tử khởi tạo, không pop được.
void glInitNames(void) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glInitNames")) return;
    if (NameStackBlockedInBeginEnd(c)) return;
    EnsureModeInitialized(c);
    // Hit record phải chứa name stack "at the time of the hit" (§5.2) ⇒ ghi
    // record TRƯỚC khi xoá stack.
    WriteHitRecord(c);
    c.legacy.selection.nameStack.clear();
    c.legacy.selection.nameStack.push_back(0);
    c.legacy.selection.currentName = 0;
}

// glPushName: đẩy name lên đỉnh stack.
void glPushName(GLuint name) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPushName")) return;
    if (NameStackBlockedInBeginEnd(c)) return;
    EnsureModeInitialized(c);
    if (name > (GLuint)kNameStackMaxDepth) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    auto& stack = c.legacy.selection.nameStack;
    if (stack.size() >= kNameStackMaxDepth) {
        // Stack đầy (spec §5.2: STACK_OVERFLOW; task yêu cầu INVALID_VALUE).
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    WriteHitRecord(c);
    stack.push_back(name);
    c.legacy.selection.currentName = name;
}

// glPopName: bỏ phần tử đỉnh.
void glPopName(void) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPopName")) return;
    if (NameStackBlockedInBeginEnd(c)) return;
    EnsureModeInitialized(c);
    auto& stack = c.legacy.selection.nameStack;
    if (stack.size() <= 1) {
        // Chỉ còn phần tử khởi tạo (spec §5.2: STACK_UNDERFLOW; task yêu cầu
        // INVALID_OPERATION).
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    WriteHitRecord(c);
    stack.pop_back();
    c.legacy.selection.currentName = stack.back();
}

// glLoadName: thay phần tử đỉnh.
void glLoadName(GLuint name) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glLoadName")) return;
    if (NameStackBlockedInBeginEnd(c)) return;
    EnsureModeInitialized(c);
    if (name > (GLuint)kNameStackMaxDepth) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    auto& stack = c.legacy.selection.nameStack;
    if (stack.empty()) {
        // "Loading a name onto an empty stack generates the error
        //  INVALID_OPERATION." (§5.2)
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    WriteHitRecord(c);
    stack.back() = name;
    c.legacy.selection.currentName = name;
}

}  // namespace tglmt::gl
