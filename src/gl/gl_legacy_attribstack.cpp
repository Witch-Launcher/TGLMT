// gl_legacy_attribstack.cpp — ATTRIBUTE STACKS (Compatibility Profile).
//
// Spec (đã đối chiếu văn bản PDF):
//   • OpenGL 4.2 Compatibility Profile §6.1.22 "Saving and Restoring State"
//     (.cache/spec/glspec42.compatibility.pdf, PDF p.529–531) + Table 6.5
//       "Attribute groups" (PDF p.531);
//   • OpenGL 2.1 §6.1.15 (glspec21.pdf, PDF p.278–280) + Table 6.2;
//   • §2.6.3 "GL Commands within Begin / End" (4.2C PDF p.51): PushAttrib/
//     PopAttrib KHÔNG nằm trong danh sách lệnh được phép ⇒ INVALID_OPERATION.
//
// 4 entry point: glPushAttrib, glPopAttrib, glPushClientAttrib, glPopClientAttrib.
//
// VÍ DỤ SPEC (4.2C §6.1.22, PDF p.529-530):
//   "take a bitwise OR of symbolic constants indicating which groups of state
//    variables to push onto an attribute stack. PushAttrib uses a server
//    attribute stack while PushClientAttrib uses a client attribute stack."
//   "The error STACK_OVERFLOW is generated if PushAttrib or PushClientAttrib is
//    executed while the corresponding stack depth is MAX_ATTRIB_STACK_DEPTH or
//    MAX_CLIENT_ATTRIB_STACK_DEPTH respectively. Bits set in mask that do not
//    correspond to an attribute group are ignored."
//   "The error STACK_UNDERFLOW is generated if PopAttrib or PopClientAttrib is
//    executed while the respective stack is empty."
//   "The depth of each attribute stack is implementation-dependent but must be
//    at least 16." (PDF p.530)
//   "Matrix stacks are never pushed or popped with PushAttrib,
//    PushClientAttrib, PopAttrib, or PopClientAttrib." (PDF p.530) ⇒ chỉ lưu
//    ĐỈNH của mỗi matrix stack, không lưu cả stack.
//
// NHÓM THUỘC TÍNH (Table 6.5, PDF p.531):
//   server: ACCUM_BUFFER_BIT, COLOR_BUFFER_BIT, CURRENT_BIT, DEPTH_BUFFER_BIT,
//           ENABLE_BIT, EVAL_BIT, FOG_BIT, HINT_BIT, LIGHTING_BIT, LINE_BIT,
//           LIST_BIT, MULTISAMPLE_BIT, PIXEL_MODE_BIT, POINT_BIT,
//           POLYGON_BIT, POLYGON_STIPPLE_BIT, SCISSOR_BIT,
//           STENCIL_BUFFER_BIT, TEXTURE_BIT, TRANSFORM_BIT, VIEWPORT_BIT,
//           ALL_ATTRIB_BITS
//   client: CLIENT_VERTEX_ARRAY_BIT, CLIENT_PIXEL_STORE_BIT,
//           CLIENT_ALL_ATTRIB_BITS
//
// NHÓM ĐÃ HỖ TRỢ (xem SnapshotServer/RestoreServer):
//   CURRENT, ENABLE, COLOR_BUFFER, DEPTH_BUFFER, ACCUM_BUFFER, FOG, LIGHTING,
//   POLYGON, POLYGON_STIPPLE, PIXEL_MODE, TEXTURE, TRANSFORM, VIEWPORT, LINE,
//   POINT, SCISSOR, STENCIL_BUFFER, LIST, EVAL, MULTISAMPLE.
//   * HINT_BIT: KHÔNG hỗ trợ — TGLMT chưa có state cho glHint (src/gl/
//     gl_raster.cpp:373 là no-op) ⇒ bit bị bỏ qua (spec cho phép: "Bits set
//     in mask that do not correspond to an attribute group are ignored").
//   * GL_MATERIAL_BIT / GL_LIGHT_MODEL_BIT: KHÔNG tồn tại trong Khronos
//     registry (đã bị gộp vào GL_LIGHTING_BIT từ GL 2.0) ⇒ không thể dùng
//     hằng sinh tự động; GL_LIGHTING_BIT đã bao gồm cả material + light model.
//
// GIỚI HẠN CẤU TRÚC (báo cáo):
//   • LegacyAttribStackEntry / LegacyClientAttribStackEntry (include/tglmt/
//     LegacyState.h:523-558) KHÔNG có trường mask ⇒ mask mỗi entry phải lưu
//     ở side-table cạnh bên (xem SideTable) — vì PopAttrib phải biết nhóm nào
//     đã push.
//   • Cũng không có trường cho: pixel-store state, array element, viewport,
//     scissor box, stencil, cull/front-face/polygon-mode, line width, list
//     base, zoom/map flags, evaluator maps, sample coverage ⇒ các state này
//     nằm trong side-table.
//
// SAI LỆCH CÓ CHỦ ĐÍCH (báo cáo):
//   • mask == 0 ⇒ GL_INVALID_OPERATION. Spec 4.2C §6.1.22 (PDF p.529–530)
//     KHÔNG quy định lỗi này (chỉ nói "Bits set in mask that do not correspond
//     to an attribute group are ignored"); yêu cầu của slice này là báo lỗi
//     ⇒ TGLMT làm vậy để bắt lỗi lập trình.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <cstring>

using namespace tglmt;

namespace tglmt::gl {

// Đặt limit độ sâu 2 attribute stack vào shadow (xem định nghĩa ở cuối file).
// Được gọi từ push/pop (lazy) và từ hook Context::SetVersionProfile.
void LegacyInitAttribStackLimits();

namespace {

// Độ sâu tối thiểu của mỗi attribute stack (spec 4.2C §6.1.22: "must be at
// least 16"). TGLMT dùng đúng 16.
constexpr size_t kAttribStackDepth = 16;
constexpr size_t kClientAttribStackDepth = 16;

// Danh sách enable cap mà TGLMT theo dõi (GL 4.6 core + GL 2.1/4.2C compat).
// Dùng hằng SINH TỪ registry (docs/khronos/gl.xml) — không hardcode hex.
// LÝ DO có danh sách: StateTracker::caps_ là private và không có API liệt
// kê ⇒ không thể copy nguyên map của enable bits; ta dò từng cap đã biết.
// (Xem báo cáo: cần StateTracker::CapsSnapshot()/CapsRestore() để đúng hoàn
// toàn cho cap lạ.)
const GLenum kEnableCaps[] = {
    // --- GL 4.6 core ---
    GL_BLEND, GL_COLOR_LOGIC_OP, GL_CULL_FACE, GL_DEPTH_CLAMP, GL_DEPTH_TEST,
    GL_DITHER, GL_FRAMEBUFFER_SRGB, GL_LINE_SMOOTH, GL_MULTISAMPLE,
    GL_POLYGON_OFFSET_FILL, GL_POLYGON_OFFSET_LINE, GL_POLYGON_OFFSET_POINT,
    GL_POLYGON_SMOOTH, GL_PRIMITIVE_RESTART, GL_PRIMITIVE_RESTART_FIXED_INDEX,
    GL_RASTERIZER_DISCARD, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_ALPHA_TO_ONE,
    GL_SAMPLE_COVERAGE, GL_SAMPLE_MASK, GL_SCISSOR_TEST, GL_STENCIL_TEST,
    GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_DEBUG_OUTPUT, GL_DEBUG_OUTPUT_SYNCHRONOUS,
    GL_PROGRAM_POINT_SIZE, GL_SAMPLE_SHADING,
    // --- Compatibility Profile (GL 2.1 / 4.2C) ---
    GL_ALPHA_TEST, GL_AUTO_NORMAL, GL_COLOR_MATERIAL, GL_FOG,
    GL_INDEX_LOGIC_OP, GL_LIGHTING, GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3,
    GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7, GL_LINE_STIPPLE,
    GL_MAP1_COLOR_4, GL_MAP1_INDEX, GL_MAP1_NORMAL, GL_MAP1_TEXTURE_COORD_1,
    GL_MAP1_TEXTURE_COORD_2, GL_MAP1_TEXTURE_COORD_3, GL_MAP1_TEXTURE_COORD_4,
    GL_MAP1_VERTEX_3, GL_MAP1_VERTEX_4, GL_MAP2_COLOR_4, GL_MAP2_INDEX,
    GL_MAP2_NORMAL, GL_MAP2_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_2,
    GL_MAP2_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_4, GL_MAP2_VERTEX_3,
    GL_MAP2_VERTEX_4, GL_NORMALIZE, GL_POINT_SMOOTH, GL_POLYGON_STIPPLE,
    GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_GEN_S,
    GL_TEXTURE_GEN_T, GL_TEXTURE_GEN_R, GL_TEXTURE_GEN_Q,
    // Clip plane: cap enable của 6 mặt phẳng user (spec 4.2C §2.12.3).
    // GL_CLIP_PLANE0 (0x3000) TRÙNG GIÁ TRỊ với GL_CLIP_DISTANCE0 của Core
    // ⇒ ở Core Profile phải từ chối, nhưng ở Compatibility Profile là cap
    // hợp lệ (bảng compat-only do tools/gen_compat_tables.py sinh).
    GL_CLIP_PLANE0, GL_CLIP_PLANE1, GL_CLIP_PLANE2,
    GL_CLIP_PLANE3, GL_CLIP_PLANE4, GL_CLIP_PLANE5,
};

// State mà struct LegacyAttribStackEntry / LegacyClientAttribStackEntry KHÔNG
// có trường tương ứng.
struct SideEntry {
    GLbitfield mask = 0;
    // GL_CLIENT_PIXEL_STORE_BIT
    PixelStoreState pixelStore;
    // GL_CLIENT_VERTEX_ARRAY_BIT (glArrayElement)
    GLuint arrayElement = 0;
    // GL_POLYGON_BIT
    GLenum cullMode = 0;
    GLenum frontFace = 0;
    GLenum polygonMode = 0;
    // GL_LINE_BIT
    float lineWidth = 1.0f;
    // GL_SCISSOR_BIT
    StateTracker::ScissorBox scissor;
    // GL_VIEWPORT_BIT
    ViewportState viewport;
    // GL_STENCIL_BUFFER_BIT
    StencilFace stencilFront, stencilBack;
    // GL_LIST_BIT
    GLuint listBase = 0;
    // GL_EVAL_BIT — evaluator maps (glMap1/glMap2). LƯU Ý: lưới evaluator
    // (glMapGrid1/2) nằm ở side-table riêng của src/gl/gl_legacy_evaluator.cpp
    // ⇒ KHÔNG được push/pop (báo cáo).
    LegacyState::LegacyMap1 map1[8];
    LegacyState::LegacyMap2 map2[8];
    // GL_MULTISAMPLE_BIT — sample coverage (giá trị + invert) lưu trong shadow
    // của StateTracker (src/gl/gl_raster.cpp:350-357).
    bool hasSampleCoverage = false;
    float sampleCoverageValue = 0.0f;
    GLint sampleCoverageInvert = 0;
    // GL_COLOR_BUFFER_BIT — color write mask + color clear value.
    uint8_t colorMask[kMaxDrawBuffers] = {};
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    // GL_ACCUM_BUFFER_BIT — clear value RGBA của accumulation buffer (shadow
    // 16 byte do glClearAccum ghi; LegacyAccum chỉ giữ thành phần R).
    bool hasAccumClear = false;
    float accumClear[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

// Side-table đi kèm Context::legacy.attribStack / clientAttribStack.
// thread_local: Context của TGLMT cũng thread_local ⇒ mỗi thread một bảng.
struct SideTable {
    const void* owner = nullptr;
    std::vector<SideEntry> server;
    std::vector<SideEntry> client;
};

thread_local SideTable g_side;

SideTable& Side(Context& c) {
    if (g_side.owner != static_cast<const void*>(&c)) {
        g_side = SideTable();
        g_side.owner = &c;
    }
    return g_side;
}

// Bảo đảm side-table khớp với stack của context (context cũ bị huỷ và
// context mới tạo tại CÙNG địa chỉ ⇒ phải xoá bảng cũ).
void SyncSideWithStack(Context& c) {
    SideTable& s = Side(c);
    if (s.server.size() != c.legacy.attribStack.size()) s.server.clear();
    if (s.client.size() != c.legacy.clientAttribStack.size()) s.client.clear();
}

// Đồng bộ 2 chỗ của state fixed-function khi restore enable bits
// (mirror đúng glEnable/glDisable — src/gl/gl_raster.cpp:44-63).
void SyncLegacyEnable(Context& c, GLenum cap, bool on) {
    if (cap == GL_LIGHTING) {
        c.legacy.lightingEnabled = on;
    } else if (cap == GL_COLOR_MATERIAL) {
        c.legacy.colorMaterialEnabled = on;
    } else if (cap >= GL_LIGHT0 && cap <= GL_LIGHT7) {
        const int i = (int)(cap - GL_LIGHT0);
        if (i >= 0 && i < kMaxLights) c.legacy.lights[i].enabled = on;
    }
}

// ---- Snapshot --------------------------------------------------------------
void SnapshotServer(Context& c, LegacyState::LegacyAttribStackEntry& e,
                    SideEntry& s) {
    LegacyState& L = c.legacy;
    const GLbitfield mask = s.mask;

    // GL_CURRENT_BIT — current color/normal/texcoord/index/edge flag.
    if (mask & GL_CURRENT_BIT) {
        std::memcpy(e.currentColor, L.currentColor, sizeof(e.currentColor));
        std::memcpy(e.currentNormal, L.currentNormal, sizeof(e.currentNormal));
        std::memcpy(e.currentTexCoord, L.currentTexCoord, sizeof(e.currentTexCoord));
        e.currentIndex = L.currentIndex;
        e.edgeFlag = L.edgeFlag;
    }

    // GL_ENABLE_BIT — các enable bit.
    if (mask & GL_ENABLE_BIT) {
        for (GLenum cap : kEnableCaps) e.caps[cap] = c.state.IsEnabled(cap);
        // Blend theo draw buffer (glEnablei(GL_BLEND, i)); index 0 dùng chung
        // với trạng thái toàn cục nên không chụp riêng.
        for (GLuint i = 1; i < kMaxDrawBuffers; ++i)
            e.capsI[i] = c.state.IsEnabled(GL_BLEND, i);
    }

    // GL_COLOR_BUFFER_BIT — blend + color/index clear value + write mask +
    // color write mask + color clear value.
    if (mask & GL_COLOR_BUFFER_BIT) {
        const auto& B = c.state.Blend();
        for (int i = 0; i < kMaxDrawBuffers && i < (int)B.size(); ++i)
            e.blendSnapshot[i] = B[i];
        e.colorIndex.clearIndex = L.colorIndex.clearIndex;
        e.colorIndex.indexMask = L.colorIndex.indexMask;
        for (int i = 0; i < kMaxDrawBuffers; ++i)
            s.colorMask[i] = c.state.ColorMask((GLuint)i);
        std::memcpy(s.clearColor, c.clearColor, sizeof(s.clearColor));
    }

    // GL_DEPTH_BUFFER_BIT — depth test/func/write mask.
    if (mask & GL_DEPTH_BUFFER_BIT) e.depth = c.state.Depth();

    // GL_ACCUM_BUFFER_BIT — clear value của accumulation buffer.
    if (mask & GL_ACCUM_BUFFER_BIT) {
        e.accum = L.accum;
        // Clear value đầy đủ (RGBA) nằm trong shadow (xem glClearAccum).
        s.hasAccumClear = c.state.GetShadow(GL_ACCUM_CLEAR_VALUE, s.accumClear,
                                            sizeof(s.accumClear));
    }

    // GL_FOG_BIT.
    if (mask & GL_FOG_BIT) e.fog = L.fog;

    // GL_LIGHTING_BIT — lighting + material + lights + shade model.
    if (mask & GL_LIGHTING_BIT) {
        e.lightingEnabled = L.lightingEnabled;
        e.colorMaterialEnabled = L.colorMaterialEnabled;
        e.lightModel = L.lightModel;
        e.colorMaterial = L.colorMaterial;
        e.materialFront = L.materialFront;
        e.materialBack = L.materialBack;
        for (int i = 0; i < kMaxLights; ++i) e.lights[i] = L.lights[i];
        e.shadeModel = L.shadeModel;
    }

    // GL_POLYGON_STIPPLE_BIT / GL_LINE_BIT (line stipple).
    if (mask & GL_POLYGON_STIPPLE_BIT)
        std::memcpy(e.stipple.polygon, L.stipple.polygon,
                    sizeof(e.stipple.polygon));
    if (mask & GL_LINE_BIT) {
        e.stipple.linePattern = L.stipple.linePattern;
        e.stipple.lineRepeat = L.stipple.lineRepeat;
        s.lineWidth = c.state.LineWidth();
    }

    // GL_POINT_BIT.
    if (mask & GL_POINT_BIT) e.pointParam = L.pointParam;

    // GL_PIXEL_MODE_BIT — pixel transfer (RED/GREEN/BLUE/ALPHA/DEPTH_SCALE,
    // ..._BIAS, INDEX_SCALE/OFFSET, MAP_COLOR, POST_*). LƯU Ý: TGLMT không theo
    // dõi GL_ZOOM_X/GL_ZOOM_Y/GL_MAP_STENCIL ⇒ xem báo cáo.
    if (mask & GL_PIXEL_MODE_BIT) e.pixelTransfer = L.pixelTransfer;

    // GL_TEXTURE_BIT — texture environment + texture generation.
    if (mask & GL_TEXTURE_BIT) {
        for (int i = 0; i < 8; ++i) {
            e.texEnv[i] = L.texEnv[i];
            e.texGen[i] = L.texGen[i];
        }
    }

    // GL_TRANSFORM_BIT — matrix mode + ĐỈNH của 4 matrix stack + clip planes.
    // (Spec: "Matrix stacks are never pushed or popped" ⇒ không lưu cả stack.)
    if (mask & GL_TRANSFORM_BIT) {
        e.matrixMode = L.matrixMode;
        e.modelview = L.modelview.top();
        e.projection = L.projection.top();
        e.texture = L.texture.top();
        e.color = L.color.top();
        for (int i = 0; i < 6; ++i) e.clipPlanes[i] = L.clipPlanes[i];
    }

    // ---- Nhóm KHÔNG có trường trong struct ⇒ lưu ở side-table ----
    if (mask & GL_POLYGON_BIT) {
        s.cullMode = c.state.CullMode();
        s.frontFace = c.state.FrontFace();
        s.polygonMode = c.state.PolygonMode();
    }
    if (mask & GL_SCISSOR_BIT) s.scissor = c.state.GetScissor(0);
    if (mask & GL_VIEWPORT_BIT) s.viewport = c.state.GetViewport(0);
    if (mask & GL_STENCIL_BUFFER_BIT) {
        s.stencilFront = c.state.StencilFront();
        s.stencilBack = c.state.StencilBack();
    }
    if (mask & GL_LIST_BIT) s.listBase = L.listBase;

    // GL_EVAL_BIT — evaluator maps (glMap1/glMap2). Lưới evaluator
    // (glMapGrid1/2) nằm ở side-table riêng (gl_legacy_evaluator.cpp) ⇒
    // không push/pop được (báo cáo).
    if (mask & GL_EVAL_BIT) {
        for (int i = 0; i < 8; ++i) {
            s.map1[i] = L.map1[i];
            s.map2[i] = L.map2[i];
        }
    }

    // GL_MULTISAMPLE_BIT — sample coverage (value + invert), lưu trong shadow
    // của StateTracker (src/gl/gl_raster.cpp:350-357).
    if (mask & GL_MULTISAMPLE_BIT) {
        s.hasSampleCoverage = c.state.GetShadow(GL_SAMPLE_COVERAGE_VALUE,
                                                &s.sampleCoverageValue, 4);
        s.sampleCoverageInvert = 0;
        c.state.GetShadow(GL_SAMPLE_COVERAGE_INVERT, &s.sampleCoverageInvert, 4);
    }
}

void RestoreServer(Context& c, const LegacyState::LegacyAttribStackEntry& e,
                   const SideEntry& s) {
    LegacyState& L = c.legacy;
    const GLbitfield mask = s.mask;

    if (mask & GL_CURRENT_BIT) {
        std::memcpy(L.currentColor, e.currentColor, sizeof(e.currentColor));
        std::memcpy(L.currentNormal, e.currentNormal, sizeof(e.currentNormal));
        std::memcpy(L.currentTexCoord, e.currentTexCoord, sizeof(e.currentTexCoord));
        L.currentIndex = e.currentIndex;
        L.colorIndex.currentIndex = e.currentIndex;
        L.edgeFlag = e.edgeFlag;
    }

    if (mask & GL_ENABLE_BIT) {
        for (const auto& kv : e.caps) {
            c.state.SetEnabled(kv.first, kv.second);
            SyncLegacyEnable(c, kv.first, kv.second);
        }
        for (const auto& kv : e.capsI)
            if (kv.first != 0) c.state.SetEnabled(GL_BLEND, kv.second, kv.first);
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
        auto& B = c.state.Blend();
        for (int i = 0; i < kMaxDrawBuffers && i < (int)B.size(); ++i)
            B[i] = e.blendSnapshot[i];
        L.colorIndex.clearIndex = e.colorIndex.clearIndex;
        L.colorIndex.indexMask = e.colorIndex.indexMask;
        for (int i = 0; i < kMaxDrawBuffers; ++i)
            c.state.SetColorMask((GLuint)i, s.colorMask[i]);
        std::memcpy(c.clearColor, s.clearColor, sizeof(c.clearColor));
        // Shadow của glGet (GL_INDEX_CLEAR_VALUE / GL_INDEX_WRITEMASK) phải
        // khôi phục luôn, nếu không query sẽ trả giá trị MỚI (state nói dối).
        c.state.SetShadow(GL_INDEX_CLEAR_VALUE, &L.colorIndex.clearIndex, 4);
        c.state.SetShadow(GL_INDEX_WRITEMASK, &L.colorIndex.indexMask, 4);
    }

    if (mask & GL_DEPTH_BUFFER_BIT) c.state.Depth() = e.depth;

    if (mask & GL_ACCUM_BUFFER_BIT) {
        L.accum = e.accum;
        if (s.hasAccumClear)
            c.state.SetShadow(GL_ACCUM_CLEAR_VALUE, s.accumClear,
                              sizeof(s.accumClear));
    }

    if (mask & GL_FOG_BIT) L.fog = e.fog;

    if (mask & GL_LIGHTING_BIT) {
        L.lightingEnabled = e.lightingEnabled;
        L.colorMaterialEnabled = e.colorMaterialEnabled;
        L.lightModel = e.lightModel;
        L.colorMaterial = e.colorMaterial;
        L.materialFront = e.materialFront;
        L.materialBack = e.materialBack;
        for (int i = 0; i < kMaxLights; ++i) L.lights[i] = e.lights[i];
        L.shadeModel = e.shadeModel;
    }

    if (mask & GL_POLYGON_STIPPLE_BIT)
        std::memcpy(L.stipple.polygon, e.stipple.polygon,
                    sizeof(e.stipple.polygon));
    if (mask & GL_LINE_BIT) {
        L.stipple.linePattern = e.stipple.linePattern;
        L.stipple.lineRepeat = e.stipple.lineRepeat;
        c.state.SetLineWidth(s.lineWidth);
    }

    if (mask & GL_POINT_BIT) L.pointParam = e.pointParam;

    if (mask & GL_PIXEL_MODE_BIT) L.pixelTransfer = e.pixelTransfer;

    if (mask & GL_TEXTURE_BIT) {
        for (int i = 0; i < 8; ++i) {
            L.texEnv[i] = e.texEnv[i];
            L.texGen[i] = e.texGen[i];
        }
    }

    if (mask & GL_TRANSFORM_BIT) {
        L.matrixMode = e.matrixMode;
        L.modelview.top() = e.modelview;
        L.projection.top() = e.projection;
        L.texture.top() = e.texture;
        L.color.top() = e.color;
        for (int i = 0; i < 6; ++i) L.clipPlanes[i] = e.clipPlanes[i];
    }

    // ---- Nhóm KHÔNG có trường trong struct ⇒ restore từ side-table ----
    if (mask & GL_POLYGON_BIT) {
        c.state.SetCullFace(s.cullMode);
        c.state.SetFrontFace(s.frontFace);
        c.state.SetPolygonMode(GL_FRONT_AND_BACK, s.polygonMode);
    }
    if (mask & GL_SCISSOR_BIT)
        c.state.SetScissor(0, s.scissor.x, s.scissor.y, s.scissor.w, s.scissor.h);
    if (mask & GL_VIEWPORT_BIT) {
        c.state.SetViewport(0, s.viewport.x, s.viewport.y, s.viewport.w, s.viewport.h);
        c.state.SetDepthRangef(s.viewport.n, s.viewport.f, 0);
    }
    if (mask & GL_STENCIL_BUFFER_BIT) {
        c.state.StencilFront() = s.stencilFront;
        c.state.StencilBack() = s.stencilBack;
    }
    if (mask & GL_LIST_BIT) L.listBase = s.listBase;

    if (mask & GL_EVAL_BIT) {
        for (int i = 0; i < 8; ++i) {
            L.map1[i] = s.map1[i];
            L.map2[i] = s.map2[i];
        }
    }

    if (mask & GL_MULTISAMPLE_BIT) {
        if (s.hasSampleCoverage) {
            c.state.SetShadow(GL_SAMPLE_COVERAGE_VALUE, &s.sampleCoverageValue, 4);
            c.state.SetShadow(GL_SAMPLE_COVERAGE_INVERT, &s.sampleCoverageInvert, 4);
        }
    }
}

// ---- Client stack ----------------------------------------------------------
void SnapshotClient(Context& c, LegacyState::LegacyClientAttribStackEntry& e,
                    SideEntry& s) {
    LegacyState& L = c.legacy;
    const GLbitfield mask = s.mask;
    if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
        for (int i = 0; i < 8; ++i) e.arrayPointers[i] = L.arrayPointers[i];
        e.clientActiveTexture = L.clientActiveTexture;
        s.arrayElement = L.arrayElement;
    }
    if (mask & GL_CLIENT_PIXEL_STORE_BIT) s.pixelStore = c.state.PixelStore();
}

void RestoreClient(Context& c, const LegacyState::LegacyClientAttribStackEntry& e,
                   const SideEntry& s) {
    LegacyState& L = c.legacy;
    const GLbitfield mask = s.mask;
    if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
        bool usesClientMemory = false;
        for (int i = 0; i < 8; ++i) {
            L.arrayPointers[i] = e.arrayPointers[i];
            if (L.arrayPointers[i].enabled && L.arrayPointers[i].pointer &&
                !L.arrayPointers[i].buffer)
                usesClientMemory = true;
        }
        L.clientActiveTexture = e.clientActiveTexture;
        L.arrayElement = s.arrayElement;
        L.arrayUsesClientMemory = usesClientMemory;
    }
    if (mask & GL_CLIENT_PIXEL_STORE_BIT) c.state.PixelStore() = s.pixelStore;
}

}  // namespace

// ---------------------------------------------------------------------------
// Server attribute stack — glPushAttrib / glPopAttrib
// ---------------------------------------------------------------------------
void glPushAttrib(GLbitfield mask) {
    Context& c = Context::Current();
    LegacyInitAttribStackLimits();
    // PushAttrib/PopAttrib KHÔNG được phép trong glBegin/glEnd (spec 4.2C
    // §2.6.3, PDF p.51; GL 1.1 man page glPushAttrib: "GL_INVALID_OPERATION is
    // generated if glPushAttrib or glPopAttrib is executed between the
    // execution of glBegin and the corresponding execution of glEnd").
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // mask = 0 ⇒ không có nhóm nào để push ⇒ INVALID_OPERATION.
    if (mask == 0) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // Stack đầy ⇒ STACK_OVERFLOW (spec §6.1.22, PDF p.530).
    if (c.legacy.attribStack.size() >= kAttribStackDepth) {
        c.errors.Record(GL_STACK_OVERFLOW);
        return;
    }
    SyncSideWithStack(c);
    SideTable& side = Side(c);

    LegacyState::LegacyAttribStackEntry entry;
    SideEntry se;
    se.mask = mask;
    SnapshotServer(c, entry, se);
    c.legacy.attribStack.push_back(entry);
    side.server.push_back(se);
    const GLint depth = (GLint)c.legacy.attribStack.size();
    c.state.SetShadow(GL_ATTRIB_STACK_DEPTH, &depth, sizeof(depth));
}

void glPopAttrib(void) {
    Context& c = Context::Current();
    LegacyInitAttribStackLimits();
    if (c.legacy.inBeginEnd) { c.errors.Record(GL_INVALID_OPERATION); return; }
    // Stack rỗng ⇒ STACK_UNDERFLOW (spec §6.1.22, PDF p.530).
    if (c.legacy.attribStack.empty()) {
        c.errors.Record(GL_STACK_UNDERFLOW);
        return;
    }
    SyncSideWithStack(c);
    SideTable& side = Side(c);

    const LegacyState::LegacyAttribStackEntry entry = c.legacy.attribStack.back();
    SideEntry se;
    if (!side.server.empty()) se = side.server.back();
    c.legacy.attribStack.pop_back();
    if (!side.server.empty()) side.server.pop_back();

    RestoreServer(c, entry, se);
    const GLint depth = (GLint)c.legacy.attribStack.size();
    c.state.SetShadow(GL_ATTRIB_STACK_DEPTH, &depth, sizeof(depth));
}

// ---------------------------------------------------------------------------
// Client attribute stack — glPushClientAttrib / glPopClientAttrib
// ---------------------------------------------------------------------------
void glPushClientAttrib(GLbitfield mask) {
    Context& c = Context::Current();
    LegacyInitAttribStackLimits();
    // glPushClientAttrib/glPopClientAttrib nằm trong danh sách lệnh bị cấm
    // trong Begin/End của repo (src/gl/gl_legacy_beginend.cpp) ⇒ INVALID_OPERATION.
    if (LegacyCheckBeginEndAllowed(c, "glPushClientAttrib")) return;
    if (mask == 0) { c.errors.Record(GL_INVALID_OPERATION); return; }
    if (c.legacy.clientAttribStack.size() >= kClientAttribStackDepth) {
        c.errors.Record(GL_STACK_OVERFLOW);
        return;
    }
    SyncSideWithStack(c);
    SideTable& side = Side(c);

    LegacyState::LegacyClientAttribStackEntry entry;
    SideEntry se;
    se.mask = mask;
    SnapshotClient(c, entry, se);
    c.legacy.clientAttribStack.push_back(entry);
    side.client.push_back(se);
    const GLint depth = (GLint)c.legacy.clientAttribStack.size();
    c.state.SetShadow(GL_CLIENT_ATTRIB_STACK_DEPTH, &depth, sizeof(depth));
}

void glPopClientAttrib(void) {
    Context& c = Context::Current();
    LegacyInitAttribStackLimits();
    if (LegacyCheckBeginEndAllowed(c, "glPopClientAttrib")) return;
    if (c.legacy.clientAttribStack.empty()) {
        c.errors.Record(GL_STACK_UNDERFLOW);
        return;
    }
    SyncSideWithStack(c);
    SideTable& side = Side(c);

    const LegacyState::LegacyClientAttribStackEntry entry =
        c.legacy.clientAttribStack.back();
    SideEntry se;
    if (!side.client.empty()) se = side.client.back();
    c.legacy.clientAttribStack.pop_back();
    if (!side.client.empty()) side.client.pop_back();

    RestoreClient(c, entry, se);
    const GLint depth = (GLint)c.legacy.clientAttribStack.size();
    c.state.SetShadow(GL_CLIENT_ATTRIB_STACK_DEPTH, &depth, sizeof(depth));
}

// ---------------------------------------------------------------------------
// Accessor cho test + hook gl_get.cpp (báo cáo).
// ---------------------------------------------------------------------------

// Đặt limit độ sâu của 2 attribute stack vào shadow ⇒ glGetIntegerv
// (GL_MAX_ATTRIB_STACK_DEPTH / GL_MAX_CLIENT_ATTRIB_STACK_DEPTH) trả đúng.
// Spec 4.2C §6.1.22 (PDF p.530): "The depth of each attribute stack is
// implementation-dependent but must be at least 16."
//
// HOOK (báo cáo): src/core/Context.cpp — Context::SetVersionProfile(), gọi
// hàm này khi profile = Compatibility/Legacy (để query trả đúng NGAY cả khi
// app chưa push lần nào). Hàm cũng được gọi lazily trong push/pop nên thiếu
// hook vẫn đúng sau lần push đầu tiên.
void LegacyInitAttribStackLimits() {
    Context& c = Context::Current();
    const GLint depth = (GLint)kAttribStackDepth;
    const GLint cdepth = (GLint)kClientAttribStackDepth;
    c.state.SetShadow(GL_MAX_ATTRIB_STACK_DEPTH, &depth, sizeof(depth));
    c.state.SetShadow(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &cdepth, sizeof(cdepth));
}

GLsizei LegacyAttribStackDepth() {
    return (GLsizei)Context::Current().legacy.attribStack.size();
}
GLsizei LegacyClientAttribStackDepth() {
    return (GLsizei)Context::Current().legacy.clientAttribStack.size();
}
GLbitfield LegacyAttribStackTopMask() {
    Context& c = Context::Current();
    SideTable& s = Side(c);
    if (s.server.empty() || s.server.size() != c.legacy.attribStack.size())
        return 0;
    return s.server.back().mask;
}
GLbitfield LegacyClientAttribStackTopMask() {
    Context& c = Context::Current();
    SideTable& s = Side(c);
    if (s.client.empty() || s.client.size() != c.legacy.clientAttribStack.size())
        return 0;
    return s.client.back().mask;
}
int LegacyMaxAttribStackDepth() { return (int)kAttribStackDepth; }
int LegacyMaxClientAttribStackDepth() { return (int)kClientAttribStackDepth; }

}  // namespace tglmt::gl
