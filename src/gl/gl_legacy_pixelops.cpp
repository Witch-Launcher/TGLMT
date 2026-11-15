// gl_legacy_pixelops.cpp — PIXEL TRANSFER / PIXEL MAPS / PIXEL ZOOM /
//                         DRAW·COPY PIXELS / BITMAP (Compatibility Profile).
//
// Spec (đã đối chiếu TRỰC TIẾP với PDF, không suy đoán):
//   • GL 2.1 §3.6.3 "Pixel Transfer Modes"      → glspec21.pdf PDF p.116–118
//       (idx 129–131): Table 3.2 (PixelTransfer params), Table 3.3 (PixelMap
//       params), "The error INVALID_VALUE is generated if a size larger than
//       the implemented maximum, or less than one, is given to PixelMap.",
//       "A table that takes an index as an address must have size = 2^n or the
//       error INVALID_VALUE results."
//   • GL 2.1 §3.6.4 "Rasterization of Pixel Rectangles" → PDF p.127–138
//       (idx 141–152): PixelZoom, "If the current raster position is invalid,
//       then DrawPixels is ignored", vùng ảnh zoom (zx,zy).
//   • GL 2.1 §3.7 "Bitmaps"                     → PDF p.148–150 (idx 161–164)
//   • GL 2.1 §4.3.3 "Copying Pixels"            → PDF p.226–229
//   • GL 2.1 Table 2.9 "Component conversions"   → PDF p.59 (idx 72)
//   • GL 4.2C §3.7.3 "Pixel Transfer Modes"     → PDF p.247 (idx 266)
//   • GL 4.2C §3.7.4 "Transfer of Pixel Rectangles" → PDF p.258 (idx 277)
//   • GL 4.2C §3.8 "Bitmaps"                    → PDF p.284 (idx 303)
//   • GL 4.2C §4.3.3 "Copying Pixels"           → PDF p.422 (idx 441)
//
// TRẠNG THÁI:
//   • pixel transfer (scale/bias) + pixel maps + pixel zoom: state thật.
//   • draw/copy pixels + bitmap: CÓ đường ghi thật vào color attachment của
//     FBO (shadow CPU + upload GPU) và default framebuffer (texture tạm +
//     blitToTarget — cùng đường với src/gl/gl_framebuffer.cpp BlitWithDefault).
//     Xem "HOOK CẦN THÊM" ở cuối file cho các phần chưa làm được.
//
// SAI LỆCH CẦN BÁO (xem báo cáo phiên):
//   1. glGetPixelMap{f,u,us}v trong ABI công khai (sinh từ Khronos registry
//      docs/khronos/gl.xml) KHÔNG có tham số `size` (chỉ có `map` + `values`)
//      ⇒ hàm này trả TOÀN BỘ bản đồ (256 phần tử). Bản có tham số size là
//      glGetnPixelMap* (ARB_imaging) — ngoài phạm vi lát cắt này.
//   2. LegacyState::pixelMaps khởi tạo 256 phần tử giá trị = chỉ số (0..255)
//      trong khi spec (GL 2.1 Table 3.3, p.118) nói kích thước ban đầu = 1 và
//      giá trị ban đầu = 0.0. Không sửa được LegacyState.h (ngoài write-scope)
//      ⇒ map được xem là bảng 256 phần tử; các phần tử ngoài `size` khi set
//      được đặt 0.0 (spec: "undefined").
//   3. Pixel zoom không có field trong LegacyState ⇒ lưu trong shadow state
//      (pname riêng 0xF002), cùng cơ chế state phụ của gl_legacy_selection.cpp.
#include "tglmt/gl46.h"
#include "tglmt/gl_legacy.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyBeginEnd.h"
#include "tglmt/Context.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

namespace {

// ---------------------------------------------------------------------------
// Hằng số — LẤY TỪ HEADER SINH TỰ ĐỘNG (không hardcode hex: DECISIONS D-006)
// ---------------------------------------------------------------------------
constexpr GLenum kRedScale = GL_RED_SCALE;
constexpr GLenum kGreenScale = GL_GREEN_SCALE;
constexpr GLenum kBlueScale = GL_BLUE_SCALE;
constexpr GLenum kAlphaScale = GL_ALPHA_SCALE;
constexpr GLenum kRedBias = GL_RED_BIAS;
constexpr GLenum kGreenBias = GL_GREEN_BIAS;
constexpr GLenum kBlueBias = GL_BLUE_BIAS;
constexpr GLenum kAlphaBias = GL_ALPHA_BIAS;
constexpr GLenum kDepthScale = GL_DEPTH_SCALE;
constexpr GLenum kDepthBias = GL_DEPTH_BIAS;
// GL_INDEX_SCALE KHÔNG tồn tại trong Khronos registry (docs/khronos/gl.xml) —
// pname kiểu integer trong Table 3.2 (GL 2.1 p.117) là GL_INDEX_SHIFT và
// GL_INDEX_OFFSET. Field `indexScale` của LegacyState::pixelTransfer được
// dùng cho GL_INDEX_SHIFT (xem báo cáo phiên).
constexpr GLenum kIndexShift = GL_INDEX_SHIFT;
constexpr GLenum kIndexOffset = GL_INDEX_OFFSET;
constexpr GLenum kMapColor = GL_MAP_COLOR;

constexpr GLenum kPixelMapIToI = GL_PIXEL_MAP_I_TO_I;
constexpr GLenum kPixelMapSToS = GL_PIXEL_MAP_S_TO_S;
constexpr GLenum kPixelMapIToR = GL_PIXEL_MAP_I_TO_R;
constexpr GLenum kPixelMapIToG = GL_PIXEL_MAP_I_TO_G;
constexpr GLenum kPixelMapIToB = GL_PIXEL_MAP_I_TO_B;
constexpr GLenum kPixelMapIToA = GL_PIXEL_MAP_I_TO_A;
constexpr GLenum kPixelMapRToR = GL_PIXEL_MAP_R_TO_R;
constexpr GLenum kPixelMapGToG = GL_PIXEL_MAP_G_TO_G;
constexpr GLenum kPixelMapBToB = GL_PIXEL_MAP_B_TO_B;

// GL_MAX_PIXEL_MAP_TABLE: "The maximum allowable size of each table is
// specified by the implementation dependent value MAX_PIXEL_MAP_TABLE, but must
// be at least 32" (GL 2.1 §3.6.3, PDF p.117). TGLMT dùng 256.
constexpr GLsizei kMaxPixelMapTable = 256;

constexpr GLenum kColorIndex = GL_COLOR_INDEX;
constexpr GLenum kRgb = GL_RGB;
constexpr GLenum kRgba = GL_RGBA;
constexpr GLenum kBgra = GL_BGRA;
constexpr GLenum kDepthComponent = GL_DEPTH_COMPONENT;
constexpr GLenum kStencilIndex = GL_STENCIL_INDEX;
constexpr GLenum kUnsignedByte = GL_UNSIGNED_BYTE;
constexpr GLenum kByte = GL_BYTE;
constexpr GLenum kUnsignedShort = GL_UNSIGNED_SHORT;
constexpr GLenum kShort = GL_SHORT;
constexpr GLenum kUnsignedInt = GL_UNSIGNED_INT;
constexpr GLenum kInt = GL_INT;
constexpr GLenum kFloat = GL_FLOAT;
constexpr GLenum kBitmapType = GL_BITMAP;

// ---------------------------------------------------------------------------
// PIXEL ZOOM — LegacyState không có field ⇒ shadow state (pname riêng 0xF002)
// ---------------------------------------------------------------------------
constexpr GLenum kPixelZoomPname = 0xF002;

struct PixelZoomState {
    float x = 1.0f;  // mặc định 1.0 (spec: magnification factor mặc định = 1)
    float y = 1.0f;
};

PixelZoomState LoadPixelZoom(const Context& c) {
    PixelZoomState z;
    c.state.GetShadow(kPixelZoomPname, &z, sizeof(z));
    return z;
}

void StorePixelZoom(Context& c, const PixelZoomState& z) {
    c.state.SetShadow(kPixelZoomPname, &z, sizeof(z));
}

// ---------------------------------------------------------------------------
// PIXEL MAPS — 9 bảng (GL 2.1 Table 3.3, PDF p.118)
// ---------------------------------------------------------------------------
enum class MapId { IToI, SToS, IToR, IToG, IToB, IToA, RToR, GToG, BToB };

bool MapFromEnum(GLenum map, MapId& out) {
    switch (map) {
        case kPixelMapIToI: out = MapId::IToI; return true;
        case kPixelMapSToS: out = MapId::SToS; return true;
        case kPixelMapIToR: out = MapId::IToR; return true;
        case kPixelMapIToG: out = MapId::IToG; return true;
        case kPixelMapIToB: out = MapId::IToB; return true;
        case kPixelMapIToA: out = MapId::IToA; return true;
        case kPixelMapRToR: out = MapId::RToR; return true;
        case kPixelMapGToG: out = MapId::GToG; return true;
        case kPixelMapBToB: out = MapId::BToB; return true;
        default: return false;
    }
}

std::vector<GLfloat>* MapVector(LegacyState::LegacyPixelMaps& maps, MapId id) {
    switch (id) {
        case MapId::IToI: return &maps.iToI;
        case MapId::SToS: return &maps.sToS;
        case MapId::IToR: return &maps.iToR;
        case MapId::IToG: return &maps.iToG;
        case MapId::IToB: return &maps.iToB;
        case MapId::IToA: return &maps.iToA;
        case MapId::RToR: return &maps.rToR;
        case MapId::GToG: return &maps.rToG;
        case MapId::BToB: return &maps.rToB;
    }
    return nullptr;
}

// Bản đồ nào cho GIÁ TRỊ là color component (⇒ chuyển đổi theo Table 2.9)
// và bản đồ nào cho color/stencil index (⇒ chuyển thẳng sang float).
// GL 2.1 §3.6.3 p.117: "An entry giving a color component value is converted
// according to table 2.9. An entry giving a color index value is converted
// from an unsigned short integer or unsigned integer to floating-point."
bool MapValueIsColorComponent(MapId id) {
    return id != MapId::IToI && id != MapId::SToS;
}

// Table 2.9 (GL 2.1 p.59): uint → c/(2^32−1), ushort → c/(2^16−1).
float UIntToFloat(GLuint c) { return (float)((double)c / 4294967295.0); }
float UShortToFloat(GLushort c) { return (float)((double)c / 65535.0); }
GLuint FloatToUInt(float f) {
    double v = (double)f * 4294967295.0;
    if (!(v > 0.0)) return 0;
    if (v > 4294967295.0) return 0xFFFFFFFFu;
    return (GLuint)(v + 0.5);
}
GLushort FloatToUShort(float f) {
    double v = (double)f * 65535.0;
    if (!(v > 0.0)) return 0;
    if (v > 65535.0) return 0xFFFFu;
    return (GLushort)(v + 0.5);
}

// ---------------------------------------------------------------------------
// PIXEL TRANSFER — scale/bias lưu trong LegacyState::pixelTransfer
// ---------------------------------------------------------------------------
// pname hợp lệ (GL 2.1 Table 3.2, PDF p.117).
bool PixelTransferPnameValid(GLenum pname) {
    switch (pname) {
        case kRedScale: case kGreenScale: case kBlueScale: case kAlphaScale:
        case kRedBias: case kGreenBias: case kBlueBias: case kAlphaBias:
        case kDepthScale: case kDepthBias:
        case kIndexShift: case kIndexOffset:
        case kMapColor:
            return true;
        default:
            return false;
    }
}

// pname kiểu SỐ NGUYÊN (Table 3.2: INDEX SCALE / INDEX OFFSET là "integer").
// Bản float phải truyền giá trị nguyên (xem báo cáo: quy tắc này là cách
// diễn đạt "the same rules apply as for PixelStore" cho pname kiểu integer).
bool PixelTransferPnameIsIntegral(GLenum pname) {
    return pname == kIndexShift || pname == kIndexOffset;
}

void PixelTransferSetFloat(Context& c, GLenum pname, float v) {
    LegacyState::LegacyPixelTransfer& pt = c.legacy.pixelTransfer;
    switch (pname) {
        case kRedScale: pt.redScale = v; return;
        case kGreenScale: pt.greenScale = v; return;
        case kBlueScale: pt.blueScale = v; return;
        case kAlphaScale: pt.alphaScale = v; return;
        case kRedBias: pt.redBias = v; return;
        case kGreenBias: pt.greenBias = v; return;
        case kBlueBias: pt.blueBias = v; return;
        case kAlphaBias: pt.alphaBias = v; return;
        case kDepthScale: pt.depthScale = v; return;
        case kDepthBias: pt.depthBias = v; return;
        case kIndexShift: pt.indexScale = (GLuint)v; return;
        case kIndexOffset: pt.indexOffset = (GLuint)v; return;
        case kMapColor: pt.mapColor = (v != 0.0f); return;
        default: return;
    }
}

void PixelTransferSetInt(Context& c, GLenum pname, GLint v) {
    LegacyState::LegacyPixelTransfer& pt = c.legacy.pixelTransfer;
    switch (pname) {
        case kIndexShift: pt.indexScale = (GLuint)v; return;
        case kIndexOffset: pt.indexOffset = (GLuint)v; return;
        case kMapColor: pt.mapColor = (v != 0); return;
        default:
            // Các pname kiểu float: giá trị nguyên chuyển thẳng sang float
            // (spec Table 2.9: float → c).
            PixelTransferSetFloat(c, pname, (float)v);
            return;
    }
}

// ---------------------------------------------------------------------------
// FORMAT / TYPE cho DrawPixels / CopyPixels
// ---------------------------------------------------------------------------
bool FormatValid(GLenum format) {
    switch (format) {
        case kColorIndex: case kRgb: case kRgba: case kBgra:
        case kDepthComponent: case kStencilIndex:
            return true;
        default:
            return false;
    }
}

bool TypeValid(GLenum type) {
    switch (type) {
        case kUnsignedByte: case kByte: case kUnsignedShort: case kShort:
        case kUnsignedInt: case kInt: case kFloat: case kBitmapType:
            return true;
        default:
            return false;
    }
}

// Cặp (format, type) được phép (GL 2.1 §3.6.4 / §4.3.3).
bool FormatTypeValid(GLenum format, GLenum type) {
    if (type == kBitmapType) return format == kColorIndex;
    switch (format) {
        case kRgb: case kRgba: case kBgra:
            return type == kUnsignedByte || type == kByte || type == kUnsignedShort ||
                   type == kShort || type == kUnsignedInt || type == kInt || type == kFloat;
        case kColorIndex:
            return type == kUnsignedByte || type == kUnsignedShort ||
                   type == kUnsignedInt || type == kFloat;
        case kDepthComponent:
            return type == kUnsignedShort || type == kUnsignedInt || type == kFloat;
        case kStencilIndex:
            return type == kUnsignedByte;
        default:
            return false;
    }
}

// Số thành phần của 1 pixel theo format.
int FormatComponents(GLenum format) {
    switch (format) {
        case kRgb: return 3;
        case kRgba: case kBgra: return 4;
        case kColorIndex: case kDepthComponent: case kStencilIndex: return 1;
        default: return 0;
    }
}

// Số byte / pixel theo (format, type).
size_t FormatTypeBpp(GLenum format, GLenum type) {
    const int comps = FormatComponents(format);
    switch (type) {
        case kUnsignedByte: case kByte: return (size_t)comps;
        case kUnsignedShort: case kShort: return (size_t)comps * 2;
        case kUnsignedInt: case kInt: case kFloat: return (size_t)comps * 4;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// ĐỌC pixel từ client memory (theo pixel-store: ROW LENGTH/SKIP/ALIGNMENT)
// ---------------------------------------------------------------------------
// Số byte của 1 hàng sau khi đã áp dụng UNPACK_ROW_LENGTH + UNPACK_ALIGNMENT.
size_t UnpackRowBytes(const Context& c, GLsizei width, size_t bpp) {
    const PixelStoreState& ps = c.state.PixelStore();
    const size_t elems = (ps.unpackRowLength > 0) ? (size_t)ps.unpackRowLength : (size_t)width;
    const size_t align =
        (ps.unpackAlignment == 1 || ps.unpackAlignment == 2 || ps.unpackAlignment == 4 ||
         ps.unpackAlignment == 8)
            ? (size_t)ps.unpackAlignment
            : 4;
    return ((elems * bpp + align - 1) / align) * align;
}

// Đọc 1 thành phần (đã chuẩn hoá về [0,1] cho màu) từ client memory.
float ReadComponent(const uint8_t* p, GLenum type, int comp) {
    switch (type) {
        case kUnsignedByte: return (float)p[comp] / 255.0f;
        case kByte: {
            const float v = (float)(const int8_t)p[comp];
            return (float)((2.0 * (double)v + 1.0) / 255.0);
        }
        case kUnsignedShort: {
            uint16_t v;
            memcpy(&v, p + comp * 2, 2);
            return (float)((double)v / 65535.0);
        }
        case kShort: {
            int16_t v;
            memcpy(&v, p + comp * 2, 2);
            return (float)((2.0 * (double)v + 1.0) / 65535.0);
        }
        case kUnsignedInt: {
            uint32_t v;
            memcpy(&v, p + comp * 4, 4);
            return (float)((double)v / 4294967295.0);
        }
        case kInt: {
            int32_t v;
            memcpy(&v, p + comp * 4, 4);
            return (float)((2.0 * (double)v + 1.0) / 4294967295.0);
        }
        case kFloat: {
            float v;
            memcpy(&v, p + comp * 4, 4);
            return v;
        }
        default: return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// PIXEL TRANSFER: áp scale/bias + pixel map lên 1 pixel RGBA
// ---------------------------------------------------------------------------
// Bản đồ mặc định của TGLMT là bảng "identity" (entry i = i, xem
// LegacyState::LegacyPixelMaps) — KHÁC với spec (Table 3.3: size 1, value
// 0.0). Để không phá màu khi app chưa set map, bảng identity được coi là
// "không áp dụng" (xem báo cáo phiên).
bool MapIsIdentity(const std::vector<GLfloat>& m) {
    if (m.size() < 256) return true;
    for (size_t i = 0; i < 256; ++i)
        if (m[i] != (float)i) return false;
    return true;
}

// Áp 1 bảng đồ lên 1 thành phần màu đã chuẩn hoá [0,1].
// entry = bảng[round(c * 255)] (địa chỉ là giá trị thành phần 8-bit).
float ApplyColorMap(const std::vector<GLfloat>& m, bool identity, float c) {
    if (identity) return c;
    if (m.empty()) return c;
    int idx = (int)(c * 255.0f + 0.5f);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    return m[(size_t)idx];
}

// Cờ "bảng đồ là identity" — tính MỘT LẦN cho cả lô pixel (MapIsIdentity
// quét 256 phần tử ⇒ không gọi trong vòng lặp pixel).
struct MapIdentity {
    bool r = true, g = true, b = true;
    bool ir = true, ig = true, ib = true, ia = true;
};

MapIdentity ComputeMapIdentity(const LegacyState& L) {
    MapIdentity id;
    const LegacyState::LegacyPixelMaps& maps = L.pixelMaps;
    id.r = MapIsIdentity(maps.rToR);
    id.g = MapIsIdentity(maps.rToG);
    id.b = MapIsIdentity(maps.rToB);
    id.ir = MapIsIdentity(maps.iToR);
    id.ig = MapIsIdentity(maps.iToG);
    id.ib = MapIsIdentity(maps.iToB);
    id.ia = MapIsIdentity(maps.iToA);
    return id;
}

// Áp pixel transfer lên RGBA (spec GL 2.1 §3.6.5 p.138: scale/bias theo
// thành phần, sau đó clamp về [0,1]).
void ApplyPixelTransfer(Context& c, const MapIdentity& id, float rgba[4]) {
    const LegacyState::LegacyPixelTransfer& pt = c.legacy.pixelTransfer;
    const LegacyState::LegacyPixelMaps& maps = c.legacy.pixelMaps;
    rgba[0] = ApplyColorMap(maps.rToR, id.r, rgba[0]) * pt.redScale + pt.redBias;
    rgba[1] = ApplyColorMap(maps.rToG, id.g, rgba[1]) * pt.greenScale + pt.greenBias;
    rgba[2] = ApplyColorMap(maps.rToB, id.b, rgba[2]) * pt.blueScale + pt.blueBias;
    rgba[3] = rgba[3] * pt.alphaScale + pt.alphaBias;
    for (int i = 0; i < 4; ++i)
        if (rgba[i] < 0.0f) rgba[i] = 0.0f;
        else if (rgba[i] > 1.0f) rgba[i] = 1.0f;
}

// ---------------------------------------------------------------------------
// GHI pixel vào framebuffer đích
// ---------------------------------------------------------------------------
// Ghi hình chữ nhật RGBA8 (toạ độ WINDOW, hàng 0 = hàng đáy theo quy ước
// GL-order của TGLMT) vào framebuffer đích.
//   • FBO có color attachment → ghi vào shadow CPU của texture attachment rồi
//     upload GPU (cùng cơ chế glTexSubImage2D trong src/gl/gl_texture.cpp).
//   • Default framebuffer → texture tạm + blitToTarget (cùng đường với
//     BlitWithDefault trong src/gl/gl_framebuffer.cpp).
// Trả true nếu đã ghi thật.
bool WriteRgbaRect(Context& c, int x, int y, int w, int h, const std::vector<uint8_t>& rgba) {
    if (w <= 0 || h <= 0 || rgba.empty()) return false;
    const GLuint fbo = c.state.BoundDrawFBO();
    if (fbo != 0) {
        auto fit = c.fbos.find(fbo);
        if (fit == c.fbos.end()) return false;
        const GLenum db = c.CurrentDrawBuffer(0);
        int slot = -1;
        if (db >= 0x8CE0 && db <= 0x8CE7) slot = (int)(db - 0x8CE0);
        else if (db == 0x0000) return false;  // GL_NONE ⇒ không ghi
        else slot = 0;                        // default-framebuffer style trong FBO
        auto cit = fit->second.colorTex.find((GLuint)slot);
        if (cit == fit->second.colorTex.end() || cit->second == 0) return false;
        auto tit = c.textures.find(cit->second);
        if (tit == c.textures.end()) return false;
        TextureObject& tx = tit->second;
        if (tx.w == 0 || tx.h == 0) return false;
        // Chỉ hỗ trợ attachment ở level 0 (mip > 0 cần đường riêng).
        auto lit = fit->second.colorLevel.find((GLuint)slot);
        if (lit != fit->second.colorLevel.end() && lit->second != 0) return false;
        const size_t need = (size_t)tx.w * tx.h * 4;
        if (tx.pixels.size() < need) tx.pixels.assign(need, 0);
        bool touched = false;
        for (int row = 0; row < h; ++row) {
            const int dy = y + row;
            if (dy < 0 || (size_t)dy >= tx.h) continue;
            for (int col = 0; col < w; ++col) {
                const int dx = x + col;
                if (dx < 0 || (size_t)dx >= tx.w) continue;
                memcpy(tx.pixels.data() + ((size_t)dy * tx.w + dx) * 4,
                       rgba.data() + ((size_t)row * w + col) * 4, 4);
                touched = true;
            }
        }
        if (!touched) return false;
        if (tx.gpu) {
            c.device->updateTexture(tx.gpu.get(), 0, 0, tx.w, tx.h, tx.pixels.data(),
                                    (size_t)tx.w * 4);
        }
        return true;
    }
    // Default framebuffer: texture tạm + blit (path có sẵn trong
    // src/gl/gl_framebuffer.cpp BlitWithDefault).
    if (!c.device || c.device->isNull()) return false;
    auto def = c.device->defaultRenderTarget();
    if (!def) return false;
    auto tmp = c.device->newTextureWithBytes((uint32_t)w, (uint32_t)h, def->pixelFormat(),
                                              rgba.data(), (size_t)w * 4);
    if (!tmp) return false;
    return c.device->blitToTarget(tmp.get(), def.get(), 0, 0, (uint32_t)w, (uint32_t)h,
                                  (uint32_t)x, (uint32_t)y);
}

// Cập nhật current raster position sau khi vẽ pixel (spec GL 2.1 §3.6.4/§3.7:
// "the current raster position is updated: (xrp, yrp) ← (xrp + xbi, yrp + ybi)").
void AdvanceRasterPos(Context& c, float dx, float dy) {
    LegacyState::LegacyRasterPos& rp = c.legacy.raster;
    if (!rp.valid) return;
    rp.x += dx;
    rp.y += dy;
}

}  // namespace

// ===========================================================================
// Public API — truy vấn state phụ (dùng bởi test + agent-main)
// ===========================================================================
bool LegacyPixelZoomState(const Context& c, float* xfactor, float* yfactor) {
    const PixelZoomState z = LoadPixelZoom(c);
    if (xfactor) *xfactor = z.x;
    if (yfactor) *yfactor = z.y;
    return true;
}

// ===========================================================================
// PIXEL TRANSFER — glPixelTransferf / glPixelTransferi
// ===========================================================================
void glPixelTransferf(GLenum pname, GLfloat param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPixelTransfer")) return;
    if (!PixelTransferPnameValid(pname)) { c.errors.Record(GL_INVALID_ENUM); return; }
    // pname kiểu integer: bản float phải truyền giá trị nguyên.
    if (PixelTransferPnameIsIntegral(pname) && param != std::floor(param)) {
        c.errors.Record(GL_INVALID_VALUE);
        return;
    }
    PixelTransferSetFloat(c, pname, param);
}

void glPixelTransferi(GLenum pname, GLint param) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glPixelTransfer")) return;
    if (!PixelTransferPnameValid(pname)) { c.errors.Record(GL_INVALID_ENUM); return; }
    PixelTransferSetInt(c, pname, param);
}

// ===========================================================================
// PIXEL MAPS — glPixelMap{f,u,us}v / glGetPixelMap{f,u,us}v
// ===========================================================================
namespace {

// Set: size < 1 hoặc > MAX_PIXEL_MAP_TABLE ⇒ INVALID_VALUE (spec p.117).
// Bảng "được địa chỉ bằng index" (I_TO_I, S_TO_S) phải có size = 2^n.
bool PixelMapSizeValid(GLenum map, GLsizei size) {
    if (size < 1 || size > kMaxPixelMapTable) return false;
    if (map == kPixelMapIToI || map == kPixelMapSToS) {
        // size = 2^n ?
        size_t v = (size_t)size;
        return (v & (v - 1)) == 0;
    }
    return true;
}

// Ghi `size` phần tử vào bảng; các phần tử còn lại = 0.0 (spec: undefined).
void PixelMapStore(LegacyState::LegacyPixelMaps& maps, MapId id, GLsizei size,
                   const float* values) {
    std::vector<GLfloat>* m = MapVector(maps, id);
    if (!m) return;
    if (m->size() < (size_t)kMaxPixelMapTable) m->resize((size_t)kMaxPixelMapTable, 0.0f);
    for (GLsizei i = 0; i < size; ++i) (*m)[(size_t)i] = values[i];
    for (size_t i = (size_t)size; i < m->size(); ++i) (*m)[i] = 0.0f;
}

}  // namespace

void glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!PixelMapSizeValid(map, mapsize)) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    PixelMapStore(c.legacy.pixelMaps, id, mapsize, values);
}

void glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!PixelMapSizeValid(map, mapsize)) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    // uint → float: color component theo Table 2.9 (c/(2^32−1)); color index
    // thì chuyển thẳng (spec §3.6.3 p.117).
    const bool comp = MapValueIsColorComponent(id);
    std::vector<GLfloat> tmp((size_t)mapsize);
    for (GLsizei i = 0; i < mapsize; ++i)
        tmp[(size_t)i] = comp ? UIntToFloat(values[i]) : (float)values[i];
    PixelMapStore(c.legacy.pixelMaps, id, mapsize, tmp.data());
}

void glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!PixelMapSizeValid(map, mapsize)) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    const bool comp = MapValueIsColorComponent(id);
    std::vector<GLfloat> tmp((size_t)mapsize);
    for (GLsizei i = 0; i < mapsize; ++i)
        tmp[(size_t)i] = comp ? UShortToFloat(values[i]) : (float)values[i];
    PixelMapStore(c.legacy.pixelMaps, id, mapsize, tmp.data());
}

void glGetPixelMapfv(GLenum map, GLfloat* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    const std::vector<GLfloat>* m = MapVector(c.legacy.pixelMaps, id);
    if (!m) return;
    // ABI (Khronos registry) không có tham số size ⇒ trả TOÀN BỘ bảng.
    for (size_t i = 0; i < m->size(); ++i) values[i] = (*m)[i];
}

void glGetPixelMapuiv(GLenum map, GLuint* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    const std::vector<GLfloat>* m = MapVector(c.legacy.pixelMaps, id);
    if (!m) return;
    // color component: float → uint theo Table 2.9 ngược (×(2^32−1));
    // color index: cắt phần thập phân.
    const bool comp = MapValueIsColorComponent(id);
    for (size_t i = 0; i < m->size(); ++i)
        values[i] = comp ? FloatToUInt((*m)[i]) : (GLuint)(*m)[i];
}

void glGetPixelMapusv(GLenum map, GLushort* values) {
    Context& c = Context::Current();
    MapId id;
    if (!MapFromEnum(map, id)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!values) { c.errors.Record(GL_INVALID_VALUE); return; }
    const std::vector<GLfloat>* m = MapVector(c.legacy.pixelMaps, id);
    if (!m) return;
    const bool comp = MapValueIsColorComponent(id);
    for (size_t i = 0; i < m->size(); ++i)
        values[i] = comp ? FloatToUShort((*m)[i]) : (GLushort)(*m)[i];
}

// ===========================================================================
// PIXEL ZOOM — glPixelZoom
// ===========================================================================
void glPixelZoom(GLfloat xfactor, GLfloat yfactor) {
    Context& c = Context::Current();
    PixelZoomState z;
    z.x = xfactor;
    z.y = yfactor;
    StorePixelZoom(c, z);
}

// ===========================================================================
// DRAW PIXELS — glDrawPixels
// ===========================================================================
void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type,
                  const void* pixels) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glDrawPixels")) return;
    if (width < 0 || height < 0) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (!FormatValid(format) || !TypeValid(type)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!FormatTypeValid(format, type)) { c.errors.Record(GL_INVALID_ENUM); return; }
    if (!pixels) { c.errors.Record(GL_INVALID_VALUE); return; }
    if (width == 0 || height == 0) return;  // không vẽ gì

    // "If the current raster position is invalid, then DrawPixels is ignored"
    // (spec GL 2.1 §3.6.4, PDF p.138).
    const LegacyState::LegacyRasterPos& rp = c.legacy.raster;
    if (!rp.valid) return;

    const size_t bpp = FormatTypeBpp(format, type);
    if (!bpp) { c.errors.Record(GL_INVALID_ENUM); return; }
    const size_t rowBytes = UnpackRowBytes(c, width, bpp);
    const PixelStoreState& ps = c.state.PixelStore();
    const uint8_t* base = (const uint8_t*)pixels;
    base += (size_t)ps.unpackSkipRows * rowBytes;
    base += (size_t)ps.unpackSkipPixels * bpp;

    // Chuyển nguồn → RGBA8.
    std::vector<uint8_t> rgba((size_t)width * height * 4, 0);
    const int comps = FormatComponents(format);
    const MapIdentity mapId = ComputeMapIdentity(c.legacy);
    for (GLsizei row = 0; row < height; ++row) {
        const uint8_t* src = base + (size_t)row * rowBytes;
        for (GLsizei col = 0; col < width; ++col) {
            const uint8_t* px = src + (size_t)col * bpp;
            float c4[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            if (format == kRgb || format == kRgba || format == kBgra) {
                float v[4] = {0, 0, 0, 1};
                for (int k = 0; k < comps; ++k) v[k] = ReadComponent(px, type, k);
                if (format == kBgra) { c4[0] = v[2]; c4[1] = v[1]; c4[2] = v[0]; c4[3] = v[3]; }
                else { c4[0] = v[0]; c4[1] = v[1]; c4[2] = v[2]; c4[3] = v[3]; }
            } else if (format == kColorIndex) {
                // Color index: TGLMT không có color-index framebuffer ⇒ dùng
                // bảng pixel map I_TO_R/G/B/A để tạo RGBA (nếu bảng identity
                // thì index → thang 0..1 trực tiếp).
                const LegacyState::LegacyPixelMaps& maps = c.legacy.pixelMaps;
                const float idx = ReadComponent(px, type, 0) * 255.0f;
                c4[0] = ApplyColorMap(maps.iToR, mapId.ir, idx / 255.0f);
                c4[1] = ApplyColorMap(maps.iToG, mapId.ig, idx / 255.0f);
                c4[2] = ApplyColorMap(maps.iToB, mapId.ib, idx / 255.0f);
                c4[3] = ApplyColorMap(maps.iToA, mapId.ia, idx / 255.0f);
            } else if (format == kDepthComponent) {
                c4[0] = c4[1] = c4[2] = 0.0f;
                c4[3] = 1.0f;
            } else {  // GL_STENCIL_INDEX
                c4[0] = c4[1] = c4[2] = 0.0f;
                c4[3] = (float)px[0] / 255.0f;
            }
            ApplyPixelTransfer(c, mapId, c4);
            uint8_t* dst = rgba.data() + ((size_t)row * width + col) * 4;
            dst[0] = (uint8_t)(c4[0] * 255.0f + 0.5f);
            dst[1] = (uint8_t)(c4[1] * 255.0f + 0.5f);
            dst[2] = (uint8_t)(c4[2] * 255.0f + 0.5f);
            dst[3] = (uint8_t)(c4[3] * 255.0f + 0.5f);
        }
    }

    // Vị trí đích: current raster position, nhân với pixel zoom.
    const PixelZoomState zoom = LoadPixelZoom(c);
    const int dx = (int)std::floor((double)rp.x * (double)zoom.x);
    const int dy = (int)std::floor((double)rp.y * (double)zoom.y);
    const int dw = (int)std::floor((double)width * (double)zoom.x);
    const int dh = (int)std::floor((double)height * (double)zoom.y);
    if (dw > 0 && dh > 0) WriteRgbaRect(c, dx, dy, dw, dh, rgba);

    // KHÔNG cập nhật current raster position sau DrawPixels: GL 2.1 §3.6.4
    // (PDF p.138) và GL 4.2C §3.7.5 (PDF p.271) KHÔNG quy định điều đó
    // (chỉ Bitmap có quy tắc riêng — §3.7 p.149). Không tự ý thêm hành vi.
}

// ===========================================================================
// COPY PIXELS — glCopyPixels
// ===========================================================================
void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glCopyPixels")) return;
    if (width < 0 || height < 0) { c.errors.Record(GL_INVALID_VALUE); return; }
    // Tham số cuối là FORMAT (GL_COLOR_INDEX / GL_RGB / GL_RGBA) — tên tham số
    // trong registry là "type" nhưng nhóm enum là PixelCopyType.
    if (format != kColorIndex && format != kRgb && format != kRgba) {
        c.errors.Record(GL_INVALID_ENUM);
        return;
    }
    if (width == 0 || height == 0) return;

    const LegacyState::LegacyRasterPos& rp = c.legacy.raster;
    if (!rp.valid) return;  // raster position invalid ⇒ copy bị bỏ qua

    // Đọc nguồn từ READ framebuffer (spec §4.3.3: nguồn = read framebuffer).
    std::vector<uint8_t> rgba((size_t)width * height * 4, 0);
    glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    // Chuyển RGBA → format đích rồi áp pixel transfer.
    const MapIdentity mapId = ComputeMapIdentity(c.legacy);
    std::vector<uint8_t> dst((size_t)width * height * 4, 0);
    const int comps = (format == kRgb) ? 3 : 4;
    for (GLsizei i = 0; i < width * height; ++i) {
        float c4[4] = {rgba[(size_t)i * 4 + 0] / 255.0f, rgba[(size_t)i * 4 + 1] / 255.0f,
                       rgba[(size_t)i * 4 + 2] / 255.0f, rgba[(size_t)i * 4 + 3] / 255.0f};
        ApplyPixelTransfer(c, mapId, c4);
        uint8_t* d = dst.data() + (size_t)i * 4;
        d[0] = (uint8_t)(c4[0] * 255.0f + 0.5f);
        d[1] = (uint8_t)(c4[1] * 255.0f + 0.5f);
        d[2] = (uint8_t)(c4[2] * 255.0f + 0.5f);
        d[3] = (uint8_t)(c4[3] * 255.0f + 0.5f);
        if (comps == 3) d[3] = 255;
    }

    const PixelZoomState zoom = LoadPixelZoom(c);
    const int dx = (int)std::floor((double)rp.x * (double)zoom.x);
    const int dy = (int)std::floor((double)rp.y * (double)zoom.y);
    const int dw = (int)std::floor((double)width * (double)zoom.x);
    const int dh = (int)std::floor((double)height * (double)zoom.y);
    if (dw > 0 && dh > 0) WriteRgbaRect(c, dx, dy, dw, dh, dst);

    // KHÔNG cập nhật current raster position (spec im lặng — xem glDrawPixels).
}

// ===========================================================================
// BITMAP — glBitmap
// ===========================================================================
void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove,
              GLfloat ymove, const GLubyte* bitmap) {
    Context& c = Context::Current();
    if (LegacyCheckBeginEndAllowed(c, "glBitmap")) return;
    if (width < 0 || height < 0) { c.errors.Record(GL_INVALID_VALUE); return; }
    (void)xorig;
    (void)yorig;
    if (width == 0 || height == 0) {
        // Vẽ rỗng vẫn cập nhật raster position (spec §3.7: bitmap rỗng không
        // tạo fragment nhưng raster position vẫn tiến).
        AdvanceRasterPos(c, xmove, ymove);
        return;
    }
    if (!bitmap) { c.errors.Record(GL_INVALID_VALUE); return; }

    const LegacyState::LegacyRasterPos& rp = c.legacy.raster;
    if (!rp.valid) {
        // Raster position invalid ⇒ bitmap bị bỏ qua, NHƯNG raster position
        // vẫn được cập nhật (spec §3.7 p.149: "the current raster position is
        // updated: (xrp, yrp) ← (xrp + xbi, yrp + ybi)").
        AdvanceRasterPos(c, xmove, ymove);
        return;
    }

    // Màu của bitmap = current raster color (spec §3.7: "The associated data
    // for each fragment are those associated with the current raster position").
    const float rasterColor[4] = {rp.color[0], rp.color[1], rp.color[2], rp.color[3]};
    // Mỗi hàng bitmap được đệm thành byte: k = ceil(width/8) byte.
    const size_t bytesPerRow = ((size_t)width + 7) / 8;
    std::vector<uint8_t> rgba((size_t)width * height * 4, 0);
    const PixelStoreState& ps = c.state.PixelStore();
    const uint8_t* base = bitmap + (size_t)ps.unpackSkipRows * bytesPerRow;
    // Thứ tự bit: MSB-first (spec GL 2.1 §3.6.4 p.136: "The 8 single-bit
    // elements are ordered from most significant to least significant if the
    // value of UNPACK_LSB_FIRST is FALSE"). PixelStoreState của TGLMT không
    // có UNPACK_LSB_FIRST ⇒ lấy mặc định FALSE.
    for (GLsizei row = 0; row < height; ++row) {
        const uint8_t* src = base + (size_t)row * bytesPerRow;
        for (GLsizei col = 0; col < width; ++col) {
            const size_t byteIdx = (size_t)col / 8;
            const int bit = 7 - (int)((size_t)col % 8);  // MSB first (mặc định)
            const bool on = (src[byteIdx] >> bit) & 1;
            uint8_t* d = rgba.data() + ((size_t)row * width + col) * 4;
            if (on) {
                d[0] = (uint8_t)(rasterColor[0] * 255.0f + 0.5f);
                d[1] = (uint8_t)(rasterColor[1] * 255.0f + 0.5f);
                d[2] = (uint8_t)(rasterColor[2] * 255.0f + 0.5f);
                d[3] = (uint8_t)(rasterColor[3] * 255.0f + 0.5f);
            }
        }
    }

    const PixelZoomState zoom = LoadPixelZoom(c);
    const int dx = (int)std::floor((double)rp.x * (double)zoom.x);
    const int dy = (int)std::floor((double)rp.y * (double)zoom.y);
    const int dw = (int)std::floor((double)width * (double)zoom.x);
    const int dh = (int)std::floor((double)height * (double)zoom.y);
    if (dw > 0 && dh > 0) WriteRgbaRect(c, dx, dy, dw, dh, rgba);

    AdvanceRasterPos(c, xmove, ymove);
}

// ===========================================================================
// HOOK CẦN THÊM (báo cáo cho agent-main)
// ===========================================================================
// 1) glDrawPixels/glCopyPixels/glBitmap hiện ghi được vào:
//      • color attachment của FBO (shadow CPU của texture + upload GPU), và
//      • default framebuffer (texture tạm + blitToTarget).
//    CHƯA ghi được:
//      • depth/stencil attachment (spec §4.3.1) — cần đường ghi depth;
//      • color index framebuffer (TGLMT chỉ có RGBA) — color index được
//        chuyển sang RGBA qua pixel map I_TO_*;
//      • multi-sample attachment (spec §3.7.7) — cần resolve;
//      • histogram/minmax (spec §3.6.5) — chưa có bảng.
//    Điểm chèn nếu agent-main muốn nối vào đường draw chung:
//      src/gl/gl_framebuffer.cpp, hàm glBlitFramebuffer (khoảng dòng 819) —
//      đó là nơi đã có sẵn đường FBO→FBO và default→FBO; có thể thêm một
//      nhánh "glDrawPixels" gọi WriteRgbaRect tương tự.

}  // namespace tglmt::gl
