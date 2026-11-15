// gl_legacy_evaluator.cpp — EVALUATOR (Compatibility Profile).
//
// Spec (đã đọc trực tiếp từ PDF, không suy đoán):
//   • OpenGL 2.1 §5.1 "Evaluators" — glspec21.pdf PDF p.244–250 (1-based;
//     trang in 230–236). Công thức (5.1) nằm ở PDF p.244.
//   • OpenGL 4.2 (Compatibility Profile) §5.1 "Evaluators" —
//     glspec42.compatibility.pdf PDF p.471–477 (1-based; trang in 451–457).
//     Nội dung §5.1 GIỐNG HỆT GL 2.1 (đã đối chiếu từng câu).
//   • OpenGL 2.1 §6.1.3 (GetMap) — glspec21.pdf PDF p.264 (trang in 251):
//     "For GetMap, map must be one of the map types described in section 5.1,
//      and value must be one of ORDER, COEFF, or DOMAIN."
//   • OpenGL 2.1 Table 6.28 "Evaluators" (state table) — glspec21.pdf
//     PDF p.307 (trang in 293): số giá trị mà GetMap trả về cho từng query.
//
// API (nguồn chân lý duy nhất: docs/khronos/gl.xml ⇒ include/tglmt/gl_legacy.h
// + include/tglmt/gl_c_api.h + src/gl/gl_stubs_gen.cpp):
//   void Map1{fd}(enum target, T u1, T u2, int stride, int order, T points);
//   void Map2{fd}(enum target, T u1, T u2, int ustride, int uorder,
//                 T v1, T v2, int vstride, int vorder, T points);
//   void MapGrid1{fd}(int n, T u1, T u2);
//   void MapGrid2{fd}(int nu, T u1, T u2, int nv, T v1, T v2);
//   void EvalCoord{12}{fd}(T arg);  void EvalCoord{12}{fd}v(const T arg);
//   void EvalMesh1(enum mode, int p1, int p2);
//   void EvalMesh2(enum mode, int p1, int p2, int q1, int q2);
//   void EvalPoint1(int p);  void EvalPoint2(int p, int q);
//   void GetMap{ifd}v(enum map, enum value, T data);
//
// TARGET CỦA Map1/Map2 (spec §5.1, Table 5.1 — glspec21.pdf PDF p.245):
//   Map1: MAP1_VERTEX_3, MAP1_VERTEX_4, MAP1_INDEX, MAP1_COLOR_4, MAP1_NORMAL,
//         MAP1_TEXTURE_COORD_1..4.
//   Map2: "target is a range type selected from the same group as is used for
//          Map1, except that the string MAP1 is replaced with MAP2"
//         (glspec21.pdf PDF p.246) ⇒ MAP2_* cùng danh sách.
//   ⇒ KHÔNG phải GL_VERTEX_3/GL_COLOR_3/... (đó là enum của glVertex3f và
//     không bao giờ được dùng làm target của Map1).
//
// CÔNG THỨC ĐÁNH GIÁ (spec §5.1, glspec21.pdf PDF p.244, công thức (5.1)):
//   p(u) = Σ_{i=0..n} B_i^n(u)·R_i ,   B_i^n(u) = C(n,i)·u^i·(1−u)^(n−i)
//   (Bernstein — KHÔNG phải B-spline; đa thức TOÀN CỤC trên TẤT CẢ control
//   point, không phải từng đoạn). order = n + 1.
//   Map2: p(u,v) = Σ_i Σ_j B_i^n(u)·B_j^m(v)·R_ij.
//   Pre-image: p'(u') = p( (u'−u1)/(u2−u1) )   [1-D]
//              p'(u',v') = p( (u'−u1)/(u2−u1), (v'−v1)/(v2−v1) )  [2-D]
//   Ta đánh giá bằng de Casteljau (tương đương tuyệt đối tổng Bernstein).
//   R_ij nằm tại points[ustride·i + vstride·j] (spec §5.1, PDF p.246).
//
// SLOT (target → slot trong LegacyState::map1[8] / map2[8]):
//   Enum GL_MAP1_* (0x0D90..0x0D98) và GL_MAP2_* (0x0DB0..0x0DB8) là 9 giá trị
//   LIÊN TIẾP, cùng thứ tự ⇒ slot = target − GL_MAP1_COLOR_4 (1-D) hoặc
//   target − GL_MAP2_COLOR_4 (2-D):
//       slot 0 = COLOR_4   1 = INDEX   2 = NORMAL
//       slot 3 = TEX_COORD_1  4 = TEX_COORD_2  5 = TEX_COORD_3  6 = TEX_COORD_4
//       slot 7 = VERTEX_3    8 = VERTEX_4
//   LegacyState chỉ có 8 slot (0..7) ⇒ slot 8 (VERTEX_4) nằm trong bảng phụ
//   thread-local của file này (GAP G1). Field `target` trong LegacyMap1/2 luôn
//   ghi đúng target để state tự mô tả.
//
// BAO GIỜ ĐÃ CÓ, THIẾU GÌ:
//   • State: LegacyState::map1[8] / map2[8] (include/tglmt/LegacyState.h) —
//     có target/order/stride/points/components. KHÔNG sửa file đó (ngoài
//     write-scope) ⇒ xem "GAP" ở cuối file cho các state còn thiếu.
//   • Cờ enable của từng map: dùng state tracker (glEnable(GL_MAP1_VERTEX_3)
//     … — các cap này là EnableCap trong gl.xml) ⇒ c.state.IsEnabled().
//
// GAP (báo cáo cho agent-main):
//   (G1) LegacyState::map1/map2 chỉ có 8 slot nhưng spec có 9 loại map
//        (Table 5.1) ⇒ map thứ 9 (MAP*_VERTEX_4) nằm trong bảng phụ
//        thread-local của file này.
//   (G2) LegacyState KHÔNG có field domain (u1,u2 / u1,u2,v1,v2) của map và
//        KHÔNG có state của lưới (glMapGrid1/2) ⇒ chúng nằm trong bảng phụ
//        thread-local của file này.
//   (G3) LegacyState KHÔNG có "current vertex position" ⇒ đỉnh sinh bởi
//        evaluator mà không có map vertex bật sẽ dùng (0,0,0,1).
//   (G4) glGetIntegerv(GL_MAX_EVAL_ORDER) / glGetFloatv(GL_MAP1_GRID_SEGMENTS)
//        / GL_MAP1_GRID_DOMAIN / GL_MAP2_GRID_* nằm ở src/gl/gl_get.cpp
//        (ngoài write-scope) ⇒ chưa trả giá trị.
//   (G5) Bảng lệnh cấm trong glBegin/glEnd (src/gl/gl_legacy_beginend.cpp)
//        chưa có "glMapGrid" ⇒ file này tự kiểm tra cho MapGrid1/2.
//   (G6) Evaluator sinh màu đánh giá; nếu GL_COLOR_MATERIAL bật thì spec yêu
//        cầu màu đánh giá được dùng như màu hiện tại trong phương trình
//        lighting — đường draw fixed-function (src/gl/gl_legacy_draw.cpp)
//        bake color-material vào MSL nên chưa xử lý được (ngoài write-scope).
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstring>
#include <unordered_map>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

// LegacyMap1/LegacyMap2 là struct LỒNG trong LegacyState (include/tglmt/
// LegacyState.h) ⇒ alias cục bộ cho file này.
using LegacyMap1 = LegacyState::LegacyMap1;
using LegacyMap2 = LegacyState::LegacyMap2;

namespace {

// ---------------------------------------------------------------------------
// Hằng spec
// ---------------------------------------------------------------------------
// 9 loại map (spec §5.1: "The state required for evaluators potentially
// consists of 9 one-dimensional map specifications and 9 two-dimensional map
// specifications", glspec21.pdf PDF p.250).
constexpr int kMapTypes = 9;
// MAX_EVAL_ORDER: implementation dependent, "but must be at least 8"
// (spec §5.1, glspec21.pdf PDF p.250).
constexpr int kMaxEvalOrder = 256;
// Số thành phần lớn nhất của một control point (Table 5.1: tối đa 4).
constexpr int kMaxComponents = 4;

// Slot trong LegacyState::map1[8] / map2[8] (xem bảng slot ở đầu file).
enum MapSlot {
    kSlotColor4 = 0,
    kSlotIndex  = 1,
    kSlotNormal = 2,
    kSlotTex1   = 3,
    kSlotTex2   = 4,
    kSlotTex3   = 5,
    kSlotTex4   = 6,
    kSlotVertex3 = 7,
    kSlotVertex4 = 8,  // KHÔNG có slot trong LegacyState (chỉ 8) ⇒ bảng phụ
};

struct MapTypeInfo {
    GLenum target1;  // GL_MAP1_*
    GLenum target2;  // GL_MAP2_*
    int components;  // k (Table 5.1)
    int slot;        // slot trong LegacyState::map1[] / map2[]
};

// Table 5.1 (spec §5.1, glspec21.pdf PDF p.245): target ↔ k ↔ ý nghĩa.
// Giá trị enum lấy từ docs/khronos/gl.xml (GL_MAP1_COLOR_4 = 0x0D90 …
// GL_MAP1_VERTEX_4 = 0x0D98; GL_MAP2_COLOR_4 = 0x0DB0 … GL_MAP2_VERTEX_4 =
// 0x0DB8) — cùng thứ tự ⇒ slot = target − base.
const MapTypeInfo kMapTypeInfo[kMapTypes] = {
    {GL_MAP1_COLOR_4,         GL_MAP2_COLOR_4,         4, kSlotColor4},
    {GL_MAP1_INDEX,           GL_MAP2_INDEX,           1, kSlotIndex},
    {GL_MAP1_NORMAL,          GL_MAP2_NORMAL,          3, kSlotNormal},
    {GL_MAP1_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_1, 1, kSlotTex1},
    {GL_MAP1_TEXTURE_COORD_2, GL_MAP2_TEXTURE_COORD_2, 2, kSlotTex2},
    {GL_MAP1_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_3, 3, kSlotTex3},
    {GL_MAP1_TEXTURE_COORD_4, GL_MAP2_TEXTURE_COORD_4, 4, kSlotTex4},
    {GL_MAP1_VERTEX_3,        GL_MAP2_VERTEX_3,        3, kSlotVertex3},
    {GL_MAP1_VERTEX_4,        GL_MAP2_VERTEX_4,        4, kSlotVertex4},
};

// ---------------------------------------------------------------------------
// State BỔ SUNG (GAP G1 + G2) — thread-local, theo context.
// ---------------------------------------------------------------------------
struct EvalSide {
    // Domain (pre-image) của từng map: 1-D (u1,u2), 2-D (u1,u2,v1,v2).
    double dom1[kMapTypes][2] = {};
    double dom2[kMapTypes][4] = {};
    // Đã khởi tạo map theo spec (trạng thái đầu: order = 1) chưa?
    bool init1[kMapTypes] = {};
    bool init2[kMapTypes] = {};
    // Map thứ 9 (VERTEX_4) — không có slot trong LegacyState.
    LegacyMap1 extra1;
    LegacyMap2 extra2;
    // Lưới (glMapGrid1/2). Trạng thái đầu theo spec §5.1 (PDF p.250): "the
    // bounds of the domain interval for 1-D is 0 and 1.0 …; the number of grid
    // divisions is 1 for 1-D and 1 in both directions for 2-D".
    int grid1n = 1;
    double grid1u1 = 0.0, grid1u2 = 1.0;
    int grid2nu = 1, grid2nv = 1;
    double grid2u1 = 0.0, grid2u2 = 1.0, grid2v1 = 0.0, grid2v2 = 1.0;
};

thread_local std::unordered_map<const Context*, EvalSide> g_evalSides;

EvalSide& SideOf(Context& c) { return g_evalSides[&c]; }

LegacyMap1& Map1Of(Context& c, int slot) {
    if (slot == kSlotVertex4) return SideOf(c).extra1;
    return c.legacy.map1[slot];
}
LegacyMap2& Map2Of(Context& c, int slot) {
    if (slot == kSlotVertex4) return SideOf(c).extra2;
    return c.legacy.map2[slot];
}

// slot → chỉ số loại map (ngược lại của MapTypeOf).
int TypeOfSlot(int slot) {
    for (int t = 0; t < kMapTypes; ++t)
        if (kMapTypeInfo[t].slot == slot) return t;
    return 0;
}

// Control point mặc định (spec §5.1, glspec21.pdf PDF p.250): "Initially, all
// maps have order 1 (making them constant maps). All vertex coordinate maps
// produce the coordinates (0, 0, 0, 1) (or the appropriate subset); all normal
// coordinate maps produce (0, 0, 1); RGBA maps produce (1, 1, 1, 1); color index
// maps produce 1.0; and texture coordinate maps produce (0, 0, 0, 1)."
void DefaultControlPoint(int slot, float* out) {
    switch (slot) {
        case kSlotVertex3: out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f; break;
        case kSlotVertex4: out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f; out[3] = 1.0f; break;
        case kSlotIndex:   out[0] = 1.0f; break;
        case kSlotColor4:  out[0] = 1.0f; out[1] = 1.0f; out[2] = 1.0f; out[3] = 1.0f; break;
        case kSlotNormal:  out[0] = 0.0f; out[1] = 0.0f; out[2] = 1.0f; break;
        default:           out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f; out[3] = 1.0f; break;
    }
}

// Khởi tạo 9 map 1-D + 9 map 2-D theo trạng thái đầu của spec.
void EnsureMapsInitialized(Context& c) {
    EvalSide& s = SideOf(c);
    for (int t = 0; t < kMapTypes; ++t) {
        const MapTypeInfo& info = kMapTypeInfo[t];
        float def[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
        DefaultControlPoint(info.slot, def);
        if (!s.init1[t]) {
            LegacyMap1& m = Map1Of(c, info.slot);
            m.target = info.target1;
            m.order = 1;
            m.stride = info.components;
            m.components = info.components;
            m.points.assign(def, def + info.components);
            s.dom1[t][0] = 0.0;
            s.dom1[t][1] = 1.0;
            s.init1[t] = true;
        }
        if (!s.init2[t]) {
            LegacyMap2& m = Map2Of(c, info.slot);
            m.target = info.target2;
            m.uOrder = 1;
            m.vOrder = 1;
            m.uStride = info.components;
            m.vStride = info.components;
            m.components = info.components;
            m.points.assign(def, def + info.components);
            s.dom2[t][0] = 0.0; s.dom2[t][1] = 1.0;
            s.dom2[t][2] = 0.0; s.dom2[t][3] = 1.0;
            s.init2[t] = true;
        }
    }
}

// target (GL_MAP1_*/GL_MAP2_*) → chỉ số loại map. Trả -1 nếu không hợp lệ.
// is2d = true ⇔ target thuộc nhóm MAP2_*.
int MapTypeOf(GLenum target, bool* is2d) {
    for (int t = 0; t < kMapTypes; ++t) {
        if (kMapTypeInfo[t].target1 == target) { if (is2d) *is2d = false; return t; }
        if (kMapTypeInfo[t].target2 == target) { if (is2d) *is2d = true; return t; }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Đánh giá đa thức Bernstein (spec §5.1, PDF p.244, công thức (5.1))
// ---------------------------------------------------------------------------
// p(u) = Σ_{i=0..n} B_i^n(u)·R_i,  B_i^n(u) = C(n,i)·u^i·(1−u)^(n−i).
// de Casteljau cho kết quả tương đương tuyệt đối (và ổn định số học).
// `ctrl` chứa (n+1) điểm × k thành phần; `out` nhận k thành phần.
void SplineEval1D(const float* ctrl, int n, int k, double t, float* out) {
    float buf[kMaxComponents * kMaxEvalOrder];
    std::memcpy(buf, ctrl, sizeof(float) * (size_t)(n + 1) * (size_t)k);
    for (int level = 1; level <= n; ++level) {
        for (int i = 0; i + level <= n; ++i) {
            float* dst = buf + (size_t)i * k;
            const float* b = buf + (size_t)(i + 1) * k;
            for (int cc = 0; cc < k; ++cc)
                dst[cc] = (float)((1.0 - t) * (double)dst[cc] + t * (double)b[cc]);
        }
    }
    std::memcpy(out, buf, sizeof(float) * (size_t)k);
}

// Đánh giá spline 2-D. Control points đặt theo layout canonical:
//   R[(j·(n+1) + i)·k + c]  (j = chỉ số v, i = chỉ số u).
void SplineEval2D(const float* R, int n, int m, int k, double u, double v, float* out) {
    float row[kMaxComponents * kMaxEvalOrder];
    float col[kMaxComponents * kMaxEvalOrder];
    for (int j = 0; j <= m; ++j) {
        const float* src = R + (size_t)j * (size_t)(n + 1) * (size_t)k;
        std::memcpy(row, src, sizeof(float) * (size_t)(n + 1) * (size_t)k);
        SplineEval1D(row, n, k, u, col + (size_t)j * k);
    }
    SplineEval1D(col, m, k, v, out);
}

// ---------------------------------------------------------------------------
// Đánh giá map + đạo hàm
// ---------------------------------------------------------------------------
// Chuyển toạ độ domain u' → tham số Bernstein (spec §5.1, PDF p.245).
inline double MapParam(double up, double d1, double d2) {
    if (d2 == d1) return 0.0;
    return (up - d1) / (d2 - d1);
}

// Đánh giá map 1-D tại toạ độ domain up.
void EvalMap1(Context& c, int type, double up, float* out) {
    const MapTypeInfo& info = kMapTypeInfo[type];
    const LegacyMap1& m = Map1Of(c, info.slot);
    const EvalSide& s = SideOf(c);
    const double u = MapParam(up, s.dom1[type][0], s.dom1[type][1]);
    SplineEval1D(m.points.data(), m.order - 1, info.components, u, out);
}

// Đánh giá map 2-D tại (up, vp); du/dv = đạo hàm theo u/v (nullptr ⇒ bỏ qua).
void EvalMap2(Context& c, int type, double up, double vp, float* out,
              float* du, float* dv) {
    const MapTypeInfo& info = kMapTypeInfo[type];
    const LegacyMap2& m = Map2Of(c, info.slot);
    const EvalSide& s = SideOf(c);
    const double u = MapParam(up, s.dom2[type][0], s.dom2[type][1]);
    const double v = MapParam(vp, s.dom2[type][2], s.dom2[type][3]);
    const int n = m.uOrder - 1;   // bậc theo u
    const int mm = m.vOrder - 1;  // bậc theo v
    const int k = info.components;
    SplineEval2D(m.points.data(), n, mm, k, u, v, out);

    // Đạo hàm: ∂p/∂u = n·Σ B_i^(n−1)(u)·(R_{i+1}−R_i) (identity của
    // Bernstein) ⇒ control point đạo hàm là n·(R_{i+1}−R_i).
    if (du) {
        if (n >= 1) {
            // (mm+1) hàng (mỗi hàng n control point đạo hàm theo u).
            std::vector<float> d((size_t)(mm + 1) * (size_t)n * (size_t)k);
            for (int j = 0; j <= mm; ++j) {
                const float* a = m.points.data() + (size_t)j * (size_t)(n + 1) * k;
                const float* b = a + k;
                float* dst = d.data() + ((size_t)j * (size_t)n) * k;
                for (int cc = 0; cc < k; ++cc)
                    dst[cc] = (float)((double)n * ((double)b[cc] - (double)a[cc]));
            }
            SplineEval2D(d.data(), n - 1, mm, k, u, v, du);
        } else {
            for (int cc = 0; cc < kMaxComponents; ++cc) du[cc] = 0.0f;
        }
    }
    if (dv) {
        if (mm >= 1) {
            // mm hàng (mỗi hàng (n+1) control point đạo hàm theo v).
            std::vector<float> d((size_t)mm * (size_t)(n + 1) * (size_t)k);
            for (int j = 0; j < mm; ++j) {
                const float* a = m.points.data() + (size_t)j * (size_t)(n + 1) * k;
                const float* b = a + (size_t)(n + 1) * k;
                float* dst = d.data() + ((size_t)j * (size_t)(n + 1)) * k;
                for (int cc = 0; cc < k; ++cc)
                    dst[cc] = (float)((double)mm * ((double)b[cc] - (double)a[cc]));
            }
            SplineEval2D(d.data(), n, mm - 1, k, u, v, dv);
        } else {
            for (int cc = 0; cc < kMaxComponents; ++cc) dv[cc] = 0.0f;
        }
    }
}

// Normal giải tích của mặt (spec §5.1, glspec21.pdf PDF p.247):
//   "For MAP_VERTEX_3, let q = p. For MAP_VERTEX_4, let q = (x/w, y/w, z/w)…
//    Then let m = ∂q/∂u × ∂q/∂v … n = m if a vertex shader is active, or else
//    by n = m/‖m‖" (đường fixed-function không có vertex shader ⇒ chuẩn hoá).
bool ComputeAutoNormal(Context& c, int vertexSlot, double up, double vp, float* nrm) {
    const int k = kMapTypeInfo[TypeOfSlot(vertexSlot)].components;
    float p[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float du[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float dv[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    EvalMap2(c, TypeOfSlot(vertexSlot), up, vp, p, du, dv);
    float qu[3], qv[3];
    if (vertexSlot == kSlotVertex4) {
        const float w = p[3];
        if (w == 0.0f) return false;  // q = (x/w,…) undefined
        const float iw = 1.0f / w;
        for (int i = 0; i < 3; ++i) {
            qu[i] = (du[i] * w - p[i] * du[3]) * iw * iw;
            qv[i] = (dv[i] * w - p[i] * dv[3]) * iw * iw;
        }
    } else {
        for (int i = 0; i < 3; ++i) { qu[i] = du[i]; qv[i] = dv[i]; }
    }
    const float m[3] = {
        qu[1] * qv[2] - qu[2] * qv[1],
        qu[2] * qv[0] - qu[0] * qv[2],
        qu[0] * qv[1] - qu[1] * qv[0]};
    const float len = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    if (len > 0.0f) {
        nrm[0] = m[0] / len; nrm[1] = m[1] / len; nrm[2] = m[2] / len;
    } else {
        nrm[0] = nrm[1] = nrm[2] = 0.0f;  // normal độ dài 0 (spec: chấp nhận)
    }
    return true;
}

// ---------------------------------------------------------------------------
// Sinh đỉnh từ kết quả đánh giá
// ---------------------------------------------------------------------------
// Map vertex nào đang bật (1-D/2-D). Ưu tiên map có nhiều thành phần hơn
// (spec §5.1, PDF p.247: "If more than one evaluation is enabled for a
// particular dimension … then only the result of the evaluation of the map
// with the highest number of coordinates is used").
int EnabledVertexMap1(Context& c) {
    if (c.state.IsEnabled(GL_MAP1_VERTEX_4)) return kSlotVertex4;
    if (c.state.IsEnabled(GL_MAP1_VERTEX_3)) return kSlotVertex3;
    return -1;
}
int EnabledVertexMap2(Context& c) {
    if (c.state.IsEnabled(GL_MAP2_VERTEX_4)) return kSlotVertex4;
    if (c.state.IsEnabled(GL_MAP2_VERTEX_3)) return kSlotVertex3;
    return -1;
}

// Map texture nào đang bật (nhiều thành phần nhất thắng). Trả chỉ số LOẠI
// map (index vào kMapTypeInfo), khác với EnabledVertexMap* (trả SLOT).
int EnabledTextureMap1(Context& c) {
    int best = -1;
    for (int t = 0; t < kMapTypes; ++t) {
        // CHỈ xét nhóm texture (TEX_COORD_1..4) — vertex/color/normal/index là
        // nhóm khác (xem EnabledVertexMap*).
        if (kMapTypeInfo[t].slot < kSlotTex1 || kMapTypeInfo[t].slot > kSlotTex4) continue;
        if (!c.state.IsEnabled(kMapTypeInfo[t].target1)) continue;
        if (best < 0 || kMapTypeInfo[t].components > kMapTypeInfo[best].components)
            best = t;
    }
    return best;
}
int EnabledTextureMap2(Context& c) {
    int best = -1;
    for (int t = 0; t < kMapTypes; ++t) {
        if (kMapTypeInfo[t].slot < kSlotTex1 || kMapTypeInfo[t].slot > kSlotTex4) continue;
        if (!c.state.IsEnabled(kMapTypeInfo[t].target2)) continue;
        if (best < 0 || kMapTypeInfo[t].components > kMapTypeInfo[best].components)
            best = t;
    }
    return best;
}

// Ghi toạ độ texture theo đúng quy ước của glTexCoord1f/2f/3f/4f
// (spec GL 2.1 §2.8.1: TexCoord1 set t=r=0, q=1 …).
void FillTexCoord(const float* v, int k, float* tc) {
    tc[0] = v[0];
    tc[1] = (k >= 2) ? v[1] : 0.0f;
    tc[2] = (k >= 3) ? v[2] : 0.0f;
    tc[3] = (k >= 4) ? v[3] : 1.0f;
}

// Chuyển giá trị của map chỉ số (k=1) thành color index: glIndex dùng
// floor (spec GL 1.1 §2.8.5: c = floor(a + 256·b + 65536·c)).
GLuint IndexFromMapValue(float v) {
    if (!(v > 0.0f)) return 0u;
    const double d = std::floor((double)v);
    if (d >= 4294967295.0) return 0xFFFFFFFFu;
    return (GLuint)d;
}

// EvalCoord1: đánh giá các map 1-D đang bật rồi sinh MỘT đỉnh.
// Spec §5.1 (glspec21.pdf PDF p.247): "When one of the EvalCoord commands is
// issued, all currently enabled maps of the indicated dimension are evaluated.
// Then, for each enabled map, it is as if a corresponding GL command were
// issued with the resulting coordinates … Use of evaluators has no effect on
// the current color, normal, or texture coordinates."
void EvalCoord1Internal(Context& c, double up) {
    EnsureMapsInitialized(c);
    const int vslot = EnabledVertexMap1(c);
    if (vslot < 0) {
        // Spec §5.1 (PDF p.250): "If any evaluation command is issued when no
        // vertex map is enabled for the map dimension being evaluated, nothing
        // happens." ⇒ KHÔNG sinh lỗi.
        return;
    }
    if (!c.legacy.inBeginEnd) {
        // Lệnh tương đương là Vertex ⇒ ngoài glBegin/glEnd là lỗi (giống
        // glVertex3f trong src/gl/gl_legacy_vertex.cpp:176).
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    LegacyState& L = c.legacy;
    LegacyVertex vtx;
    // --- vị trí ---
    float pos[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
    EvalMap1(c, TypeOfSlot(vslot), up, pos);
    if (vslot == kSlotVertex3) pos[3] = 1.0f;
    std::memcpy(vtx.pos, pos, sizeof(vtx.pos));
    // --- màu ---
    std::memcpy(vtx.color, L.currentColor, sizeof(vtx.color));
    if (c.state.IsEnabled(GL_MAP1_COLOR_4)) {
        float col[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
        EvalMap1(c, TypeOfSlot(kSlotColor4), up, col);
        std::memcpy(vtx.color, col, sizeof(vtx.color));
    }
    // --- normal ---
    std::memcpy(vtx.normal, L.currentNormal, sizeof(vtx.normal));
    if (c.state.IsEnabled(GL_MAP1_NORMAL)) {
        float nrm[kMaxComponents] = {0.0f, 0.0f, 1.0f, 0.0f};
        EvalMap1(c, TypeOfSlot(kSlotNormal), up, nrm);
        vtx.normal[0] = nrm[0]; vtx.normal[1] = nrm[1]; vtx.normal[2] = nrm[2];
    }
    // --- toạ độ texture (evaluator chỉ sinh cho texture unit 0) ---
    std::memcpy(vtx.texCoord, L.currentTexCoord[0], sizeof(vtx.texCoord));
    {
        const int ttype = EnabledTextureMap1(c);
        if (ttype >= 0) {
            float tc[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
            EvalMap1(c, ttype, up, tc);
            FillTexCoord(tc, kMapTypeInfo[ttype].components, vtx.texCoord);
        }
    }
    std::memcpy(vtx.texCoord1, L.currentTexCoord[1], sizeof(vtx.texCoord1));
    // --- color index ---
    vtx.index = L.currentIndex;
    if (c.state.IsEnabled(GL_MAP1_INDEX)) {
        float idx[kMaxComponents] = {0.0f, 0.0f, 0.0f, 0.0f};
        EvalMap1(c, TypeOfSlot(kSlotIndex), up, idx);
        vtx.index = IndexFromMapValue(idx[0]);
    }
    std::memcpy(vtx.secondaryColor, L.currentSecondaryColor, sizeof(vtx.secondaryColor));
    vtx.edgeFlag = L.edgeFlag;
    L.immediateVertices.push_back(vtx);
}

// EvalCoord2: đánh giá các map 2-D đang bật rồi sinh MỘT đỉnh.
void EvalCoord2Internal(Context& c, double up, double vp) {
    EnsureMapsInitialized(c);
    const int vslot = EnabledVertexMap2(c);
    if (vslot < 0) return;  // không có map vertex ⇒ "nothing happens"
    if (!c.legacy.inBeginEnd) {
        c.errors.Record(GL_INVALID_OPERATION);
        return;
    }
    LegacyState& L = c.legacy;
    LegacyVertex vtx;
    float pos[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
    EvalMap2(c, TypeOfSlot(vslot), up, vp, pos, nullptr, nullptr);
    if (vslot == kSlotVertex3) pos[3] = 1.0f;
    std::memcpy(vtx.pos, pos, sizeof(vtx.pos));
    // --- màu ---
    std::memcpy(vtx.color, L.currentColor, sizeof(vtx.color));
    if (c.state.IsEnabled(GL_MAP2_COLOR_4)) {
        float col[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
        EvalMap2(c, TypeOfSlot(kSlotColor4), up, vp, col, nullptr, nullptr);
        std::memcpy(vtx.color, col, sizeof(vtx.color));
    }
    // --- normal: auto normal > map normal 2-D > current normal ---
    // (spec §5.1, PDF p.247: "If automatic normal generation is enabled, then
    //  this computed normal is used … If automatic normal generation is
    //  disabled, then a corresponding normal map, if enabled, is used …")
    std::memcpy(vtx.normal, L.currentNormal, sizeof(vtx.normal));
    if (c.state.IsEnabled(GL_AUTO_NORMAL)) {
        float nrm[3] = {0.0f, 0.0f, 0.0f};
        if (ComputeAutoNormal(c, vslot, up, vp, nrm))
            std::memcpy(vtx.normal, nrm, sizeof(vtx.normal));
    } else if (c.state.IsEnabled(GL_MAP2_NORMAL)) {
        float nrm[kMaxComponents] = {0.0f, 0.0f, 1.0f, 0.0f};
        EvalMap2(c, TypeOfSlot(kSlotNormal), up, vp, nrm, nullptr, nullptr);
        vtx.normal[0] = nrm[0]; vtx.normal[1] = nrm[1]; vtx.normal[2] = nrm[2];
    }
    // --- toạ độ texture ---
    std::memcpy(vtx.texCoord, L.currentTexCoord[0], sizeof(vtx.texCoord));
    {
        const int ttype = EnabledTextureMap2(c);
        if (ttype >= 0) {
            float tc[kMaxComponents] = {0.0f, 0.0f, 0.0f, 1.0f};
            EvalMap2(c, ttype, up, vp, tc, nullptr, nullptr);
            FillTexCoord(tc, kMapTypeInfo[ttype].components, vtx.texCoord);
        }
    }
    std::memcpy(vtx.texCoord1, L.currentTexCoord[1], sizeof(vtx.texCoord1));
    // --- color index ---
    vtx.index = L.currentIndex;
    if (c.state.IsEnabled(GL_MAP2_INDEX)) {
        float idx[kMaxComponents] = {0.0f, 0.0f, 0.0f, 0.0f};
        EvalMap2(c, TypeOfSlot(kSlotIndex), up, vp, idx, nullptr, nullptr);
        vtx.index = IndexFromMapValue(idx[0]);
    }
    std::memcpy(vtx.secondaryColor, L.currentSecondaryColor, sizeof(vtx.secondaryColor));
    vtx.edgeFlag = L.edgeFlag;
    L.immediateVertices.push_back(vtx);
}

// Toạ độ lưới: spec §5.1 (glspec21.pdf PDF p.248) yêu cầu "if either i = 0
// or i = n, then the value computed from i·∆u′ + u′1 is precisely u′1 or u′2".
inline double GridCoord(GLint i, int n, double u1, double u2) {
    if (n <= 0) return u1;
    if (i == 0) return u1;
    if (i == n) return u2;
    return u1 + (u2 - u1) * ((double)i / (double)n);
}

// "If the value of ACTIVE_TEXTURE is not TEXTURE0, calling Map{12} generates
// the error INVALID_OPERATION" (spec §5.1, glspec21.pdf PDF p.246).
inline bool ActiveTextureIsZero(Context& c) { return c.state.ActiveTexture() == 0; }

// ---------------------------------------------------------------------------
// Map1/Map2 — phần chung cho biến thể float/double
// ---------------------------------------------------------------------------
// Spec §5.1 (glspec21.pdf PDF p.245–246):
//   • target không hợp lệ (không thuộc nhóm MAP1_*/MAP2_*) ⇒ GL_INVALID_ENUM.
//   • order < 1 hoặc order > MAX_EVAL_ORDER ⇒ GL_INVALID_VALUE.
//   • stride < k ⇒ GL_INVALID_VALUE (stride = 0 KHÔNG phải giá trị mặc định:
//     spec nói rõ "The error INVALID_VALUE results if stride is less than k";
//     vì k ≥ 1 nên stride = 0 cũng là lỗi).
//   • u1 = u2 ⇒ GL_INVALID_VALUE.
//   • ACTIVE_TEXTURE ≠ TEXTURE0 ⇒ GL_INVALID_OPERATION.
template <typename T>
void Map1Common(Context& c, GLenum target, double u1, double u2, GLint stride,
                GLint order, const T* points) {
    bool is2d = false;
    const int type = MapTypeOf(target, &is2d);
    if (type < 0 || is2d) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (order < 1 || order > kMaxEvalOrder) { c.errors.Record(GL_INVALID_VALUE); return; }
    const int k = kMapTypeInfo[type].components;
    if (stride < k) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (u1 == u2) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!points) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!ActiveTextureIsZero(c)) { c.errors.Record(GL_INVALID_OPERATION); return; }

    EnsureMapsInitialized(c);
    LegacyMap1& m = Map1Of(c, kMapTypeInfo[type].slot);
    m.target = target;
    m.order = order;
    m.stride = stride;
    m.components = k;
    m.points.resize((size_t)order * (size_t)k);
    for (GLint i = 0; i < order; ++i)
        for (int cc = 0; cc < k; ++cc)
            m.points[(size_t)i * (size_t)k + cc] =
                (float)points[(size_t)i * (size_t)stride + (size_t)cc];
    EvalSide& s = SideOf(c);
    s.dom1[type][0] = u1;
    s.dom1[type][1] = u2;
}

// Map2: thêm uorder/vorder/ustride/vstride + domain (u1,u2,v1,v2).
// R_ij nằm tại points[ustride·i + vstride·j] (spec §5.1, glspec21.pdf PDF
// p.246) ⇒ layout canonical lưu trong state: R[(j·uorder + i)·k] (u chạy
// nhanh hơn v, ĐÚNG thứ tự đọc từ mảng points của client).
template <typename T>
void Map2Common(Context& c, GLenum target, double u1, double u2, GLint ustride,
                GLint uorder, double v1, double v2, GLint vstride, GLint vorder,
                const T* points) {
    bool is2d = false;
    const int type = MapTypeOf(target, &is2d);
    if (type < 0 || !is2d) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (uorder < 1 || uorder > kMaxEvalOrder || vorder < 1 || vorder > kMaxEvalOrder) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    const int k = kMapTypeInfo[type].components;
    if (ustride < k || vstride < k) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (u1 == u2 || v1 == v2) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!points) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!ActiveTextureIsZero(c)) { c.errors.Record(GL_INVALID_OPERATION); return; }

    EnsureMapsInitialized(c);
    LegacyMap2& m = Map2Of(c, kMapTypeInfo[type].slot);
    m.target = target;
    m.uOrder = uorder;
    m.vOrder = vorder;
    m.uStride = ustride;
    m.vStride = vstride;
    m.components = k;
    // Layout canonical: R[(j·uorder + i)·k] (j = chỉ số v, i = chỉ số u).
    m.points.resize((size_t)uorder * (size_t)vorder * (size_t)k);
    for (GLint j = 0; j < vorder; ++j)
        for (GLint i = 0; i < uorder; ++i)
            for (int cc = 0; cc < k; ++cc)
                m.points[((size_t)j * (size_t)uorder + (size_t)i) * (size_t)k + cc] =
                    (float)points[(size_t)ustride * (size_t)i +
                                  (size_t)vstride * (size_t)j + (size_t)cc];
    EvalSide& s = SideOf(c);
    s.dom2[type][0] = u1; s.dom2[type][1] = u2;
    s.dom2[type][2] = v1; s.dom2[type][3] = v2;
}

// GetMap: value ∈ {ORDER, COEFF, DOMAIN} (spec §6.1.3, glspec21.pdf PDF p.264
// + Table 6.28, PDF p.307):
//   • ORDER  — 1-D: 1 giá trị (order); 2-D: 2 giá trị (uorder, vorder).
//   • COEFF  — control point: 1-D order×k; 2-D uorder×vorder×k, thứ tự u
//     chạy nhanh hơn v (KHỚP thứ tự R_ij đọc từ points: ustride·i+vstride·j).
//   • DOMAIN — 1-D: (u1,u2); 2-D: (u1,u2,v1,v2).
void GetMapCommon(Context& c, GLenum target, GLenum query, void* data, bool asInt) {
    bool is2d = false;
    const int type = MapTypeOf(target, &is2d);
    if (type < 0) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (query != GL_ORDER && query != GL_COEFF && query != GL_DOMAIN) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (!data) { c.errors.Record(GL_INVALID_VALUE); return; }
    EnsureMapsInitialized(c);
    const int k = kMapTypeInfo[type].components;
    if (query == GL_ORDER) {
        GLint iv[2] = {1, 1};
        if (is2d) {
            const LegacyMap2& m = Map2Of(c, kMapTypeInfo[type].slot);
            iv[0] = m.uOrder;
            iv[1] = m.vOrder;
        } else {
            const LegacyMap1& m = Map1Of(c, kMapTypeInfo[type].slot);
            iv[0] = m.order;
        }
        if (asInt) {
            GLint* out = (GLint*)data;
            out[0] = iv[0];
            if (is2d) out[1] = iv[1];
        } else {
            float* out = (float*)data;
            out[0] = (float)iv[0];
            if (is2d) out[1] = (float)iv[1];
        }
        return;
    }
    if (query == GL_DOMAIN) {
        double dom[4] = {0.0, 0.0, 0.0, 0.0};
        const EvalSide& s = SideOf(c);
        if (is2d) {
            dom[0] = s.dom2[type][0]; dom[1] = s.dom2[type][1];
            dom[2] = s.dom2[type][2]; dom[3] = s.dom2[type][3];
        } else {
            dom[0] = s.dom1[type][0]; dom[1] = s.dom1[type][1];
        }
        const int n = is2d ? 4 : 2;
        if (asInt) {
            GLint* out = (GLint*)data;
            for (int i = 0; i < n; ++i) out[i] = (GLint)dom[i];
        } else {
            float* out = (float*)data;
            for (int i = 0; i < n; ++i) out[i] = (float)dom[i];
        }
        return;
    }
    // GL_COEFF: control points. 1-D: order×k; 2-D: uorder×vorder×k, thứ tự
    // (v-major, u-minor) — KHỚP với layout canonical đã lưu.
    const std::vector<GLfloat>* pts = nullptr;
    if (is2d) pts = &Map2Of(c, kMapTypeInfo[type].slot).points;
    else pts = &Map1Of(c, kMapTypeInfo[type].slot).points;
    const size_t cnt = pts->size();
    if (asInt) {
        GLint* out = (GLint*)data;
        for (size_t i = 0; i < cnt; ++i) out[i] = (GLint)(*pts)[i];
    } else {
        float* out = (float*)data;
        for (size_t i = 0; i < cnt; ++i) out[i] = (*pts)[i];
    }
}

}  // namespace

// ===========================================================================
// MAP1 — 2 entry point
// ===========================================================================
void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order,
             const GLdouble* points) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMap1d")) return;
    Map1Common(c, target, u1, u2, stride, order, points);
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order,
             const GLfloat* points) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMap1f")) return;
    Map1Common(c, target, (double)u1, (double)u2, stride, order, points);
}

// ===========================================================================
// MAP2 — 2 entry point
// ===========================================================================
void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
             GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
             const GLdouble* points) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMap2d")) return;
    Map2Common(c, target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
             GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMap2f")) return;
    Map2Common(c, target, (double)u1, (double)u2, ustride, uorder, (double)v1, (double)v2,
               vstride, vorder, points);
}

// ===========================================================================
// MAP GRID — 4 entry point
// ===========================================================================
// Spec GL 2.1 §5.1 (glspec21.pdf PDF p.248) / GL 4.2C §5.1 (PDF p.475):
//   "The error INVALID_VALUE results if n ≤ 0" (MapGrid1) và
//   "If either nu ≤ 0 or nv ≤ 0, then the error INVALID_VALUE occurs" (MapGrid2).
void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMapGrid1")) return;  // GAP G5
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    if (un <= 0) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalSide& s = SideOf(c);
    s.grid1n = un;
    s.grid1u1 = u1;
    s.grid1u2 = u2;
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {
    glMapGrid1d(un, (GLdouble)u1, (GLdouble)u2);
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glMapGrid2")) return;  // GAP G5
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    if (un <= 0 || vn <= 0) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalSide& s = SideOf(c);
    s.grid2nu = un;
    s.grid2u1 = u1;
    s.grid2u2 = u2;
    s.grid2nv = vn;
    s.grid2v1 = v1;
    s.grid2v2 = v2;
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
    glMapGrid2d(un, (GLdouble)u1, (GLdouble)u2, vn, (GLdouble)v1, (GLdouble)v2);
}

// ===========================================================================
// EVAL COORD — 8 entry point
// ===========================================================================
// glEvalCoord1/2 đánh giá các map đang bật và sinh đỉnh (xem EvalCoord*Internal).
void glEvalCoord1d(GLdouble u) {
    Context& c = Context::Current();
    EvalCoord1Internal(c, u);
}
void glEvalCoord1dv(const GLdouble* u) {
    Context& c = Context::Current();
    if (!u) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalCoord1Internal(c, u[0]);
}
void glEvalCoord1f(GLfloat u) {
    Context& c = Context::Current();
    EvalCoord1Internal(c, (double)u);
}
void glEvalCoord1fv(const GLfloat* u) {
    Context& c = Context::Current();
    if (!u) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalCoord1Internal(c, (double)u[0]);
}
void glEvalCoord2d(GLdouble u, GLdouble v) {
    Context& c = Context::Current();
    EvalCoord2Internal(c, u, v);
}
void glEvalCoord2dv(const GLdouble* u) {
    Context& c = Context::Current();
    if (!u) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalCoord2Internal(c, u[0], u[1]);
}
void glEvalCoord2f(GLfloat u, GLfloat v) {
    Context& c = Context::Current();
    EvalCoord2Internal(c, (double)u, (double)v);
}
void glEvalCoord2fv(const GLfloat* u) {
    Context& c = Context::Current();
    if (!u) { c.errors.Record(GL_INVALID_VALUE); return; }
    EvalCoord2Internal(c, (double)u[0], (double)u[1]);
}

// ===========================================================================
// EVAL MESH — 2 entry point
// ===========================================================================
// Spec GL 2.1 §5.1 (glspec21.pdf PDF p.248–249) / GL 4.2C §5.1 (PDF p.475–476):
//   EvalMesh1: mode ∈ {POINT, LINE}; tương đương
//       Begin(type); for i = p1 to p2: EvalCoord1(i·∆u′ + u′1); End();
//   EvalMesh2: mode ∈ {FILL, LINE, POINT}.
void glEvalMesh1(GLenum mode, GLint i1, GLint i2) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glEvalMesh1")) return;
    if (mode != GL_POINT && mode != GL_LINE) { c.errors.Record(GL_INVALID_ENUM); return; }
    EnsureMapsInitialized(c);
    const EvalSide& s = SideOf(c);
    const int n = s.grid1n;
    glBegin(mode == GL_POINT ? GL_POINTS : GL_LINE_STRIP);
    for (GLint i = i1; i <= i2; ++i) {
        const double u = GridCoord(i, n, s.grid1u1, s.grid1u2);
        EvalCoord1Internal(c, u);
    }
    glEnd();
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glEvalMesh2")) return;
    if (mode != GL_POINT && mode != GL_LINE && mode != GL_FILL) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    EnsureMapsInitialized(c);
    const EvalSide& s = SideOf(c);
    const int nu = s.grid2nu, nv = s.grid2nv;
    if (mode == GL_FILL) {
        for (GLint i = j1; i <= j2 - 1; ++i) {
            glBegin(GL_QUAD_STRIP);
            for (GLint j = i1; j <= i2; ++j) {
                const double u = GridCoord(j, nu, s.grid2u1, s.grid2u2);
                EvalCoord2Internal(c, u, GridCoord(i, nv, s.grid2v1, s.grid2v2));
                EvalCoord2Internal(c, u, GridCoord(i + 1, nv, s.grid2v1, s.grid2v2));
            }
            glEnd();
        }
    } else if (mode == GL_LINE) {
        for (GLint i = j1; i <= j2; ++i) {
            glBegin(GL_LINE_STRIP);
            for (GLint j = i1; j <= i2; ++j) {
                const double u = GridCoord(j, nu, s.grid2u1, s.grid2u2);
                EvalCoord2Internal(c, u, GridCoord(i, nv, s.grid2v1, s.grid2v2));
            }
            glEnd();
        }
        for (GLint i = i1; i <= i2; ++i) {
            glBegin(GL_LINE_STRIP);
            for (GLint j = j1; j <= j2; ++j) {
                const double u = GridCoord(i, nu, s.grid2u1, s.grid2u2);
                EvalCoord2Internal(c, u, GridCoord(j, nv, s.grid2v1, s.grid2v2));
            }
            glEnd();
        }
    } else {  // GL_POINT
        glBegin(GL_POINTS);
        for (GLint i = j1; i <= j2; ++i) {
            for (GLint j = i1; j <= i2; ++j) {
                const double u = GridCoord(j, nu, s.grid2u1, s.grid2u2);
                EvalCoord2Internal(c, u, GridCoord(i, nv, s.grid2v1, s.grid2v2));
            }
        }
        glEnd();
    }
}

// ===========================================================================
// EVAL POINT — 2 entry point
// ===========================================================================
// Spec GL 2.1 §5.1 (glspec21.pdf PDF p.249) / GL 4.2C §5.1 (PDF p.476):
//   EvalPoint1(p) ≡ EvalCoord1(p·∆u′ + u′1)
//   EvalPoint2(p,q) ≡ EvalCoord2(p·∆u′ + u′1, q·∆v′ + v′1)
void glEvalPoint1(GLint i) {
    Context& c = Context::Current();
    EnsureMapsInitialized(c);
    const EvalSide& s = SideOf(c);
    EvalCoord1Internal(c, GridCoord(i, s.grid1n, s.grid1u1, s.grid1u2));
}

void glEvalPoint2(GLint i, GLint j) {
    Context& c = Context::Current();
    EnsureMapsInitialized(c);
    const EvalSide& s = SideOf(c);
    EvalCoord2Internal(c, GridCoord(i, s.grid2nu, s.grid2u1, s.grid2u2),
                        GridCoord(j, s.grid2nv, s.grid2v1, s.grid2v2));
}

// ===========================================================================
// GET MAP — 3 entry point
// ===========================================================================
void glGetMapdv(GLenum target, GLenum query, GLdouble* v) {
    Context& c = Context::Current();
    if (!v) { c.errors.Record(GL_INVALID_VALUE); return; }
    bool is2d = false;
    const int type = MapTypeOf(target, &is2d);
    if (type < 0) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (query != GL_ORDER && query != GL_COEFF && query != GL_DOMAIN) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    EnsureMapsInitialized(c);
    const int k = kMapTypeInfo[type].components;
    size_t n = 0;
    if (query == GL_ORDER) {
        n = is2d ? 2 : 1;
    } else if (query == GL_DOMAIN) {
        n = is2d ? 4 : 2;
    } else {
        if (is2d) {
            const LegacyMap2& m = Map2Of(c, kMapTypeInfo[type].slot);
            n = (size_t)m.uOrder * (size_t)m.vOrder * (size_t)k;
        } else {
            const LegacyMap1& m = Map1Of(c, kMapTypeInfo[type].slot);
            n = (size_t)m.order * (size_t)k;
        }
    }
    std::vector<float> tmp(n);
    GetMapCommon(c, target, query, tmp.data(), false);
    for (size_t i = 0; i < n; ++i) v[i] = (GLdouble)tmp[i];
}

void glGetMapfv(GLenum target, GLenum query, GLfloat* v) {
    Context& c = Context::Current();
    if (!v) { c.errors.Record(GL_INVALID_VALUE); return; }
    GetMapCommon(c, target, query, v, false);
}

void glGetMapiv(GLenum target, GLenum query, GLint* v) {
    Context& c = Context::Current();
    if (!v) { c.errors.Record(GL_INVALID_VALUE); return; }
    GetMapCommon(c, target, query, v, true);
}

}  // namespace tglmt::gl
