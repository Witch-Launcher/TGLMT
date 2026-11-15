// gl_legacy_draw.cpp — ĐƯỜNG DRAW FIXED-FUNCTION (OpenGL Compatibility Profile).
//
// Spec: OpenGL 2.1 §2.14 (lighting per-vertex), §2.11 (ma trận, texgen),
// §3.8.13 (texture environment), §3.10 (fog), §2.15 + GL 4.6 Core §1.2.4
// (fixed-function dùng khi KHÔNG có program object trong use).
// Tài liệu nghiên cứu đã có trong repo: docs/LEGACY_TO_METAL_MAPPING.md §2-§8,
// docs/LEGACY_LIGHTING_SPEC.md, docs/LEGACY_TEXENV_FOG_SPEC.md.
//
// BAO GIỜ ĐÃ CÓ, THIẾU GÌ:
//   • State fixed-function: Context::legacy (ma trận, current vertex attrs,
//     lighting, fog, texenv/texgen) — src/gl/gl_legacy_*.cpp.
//   • Ghép primitive immediate-mode: AssemblePrimitives (gl_legacy_vertex.cpp:100).
//   • THIẾU: nối primitive đã ghép vào Metal draw path ⇒ glEnd chỉ gom data.
//
// KIẾN TRÚC (docs/LEGACY_TO_METAL_MAPPING.md §2):
//   • Chương trình fixed-function = 1 cặp metal::ILibrary (VS+FS) SINH RA.
//     Nằm NGOÀI Context::programs ⇒ không phải GL program object
//     (glGetProgramiv/glUseProgram không thấy) — Context::legacyProgram.
//   • PipelineKey hiện có (vsLib/fsLib/programGen + attribs[]) đã đủ ⇒
//     KHÔNG sửa Context::PipelineKey.
//   • Giá trị state đi qua đường uniform staging SẴN CÓ (pr.uniformLayout +
//     pr.uniforms ⇒ gl_apple_draw.cpp:2817-2941) — không nhân bản cơ chế.
//   • "Hình dạng" chương trình (lighting, fog, alpha test, số texture unit,
//     mode texture env, ...) BAKED vào MSL ⇒ đổi hình dạng ⇒ sinh lại
//     chương trình (linkGen tăng ⇒ pipeline mới trong PipelineKey).
//   • Vertex data: 1 vertex buffer (layout CỐ ĐỊNH 6×float4 = 96 byte) +
//     index buffer ⇒ PipelineKey::stride ổn định.
//
// SHADER: sinh MSL trực tiếp (KHÔNG đi qua ConvertGLSLtoMSL) — lý do:
//   converter viết cho GLSL người dùng (uniform block, varying, sampler
//   GLSL, layout(location)) trong khi chương trình fixed-function cần
//   (a) hằng số biên dịch (mode texture env, hàm combiner, mask color
//   material) và (b) layout uniform struct float4 thuần để đóng gói CPU khớp
//   1-1 với byte MSL. Sinh MSL trực tiếp ngắn hơn và không rủi ro bị
//   converter bóp méo. Đây cũng là cách docs/LEGACY_TO_METAL_MAPPING.md §4-§6
//   minh hoạ (mẫu MSL trong tài liệu).
#include "tglmt/gl46.h"
#include "tglmt/LegacyState.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/Context.h"
#include "tglmt/GLAppleDraw.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <vector>

using namespace tglmt;

namespace tglmt::gl {

// Selection / Feedback (src/gl/gl_legacy_selection.cpp)
bool LegacySelectionFeedbackProcessImmediate(Context& c);

namespace {

// ---------------------------------------------------------------------------
// Hằng GL (định nghĩa tại chỗ để file độc lập; đã đối chiếu docs/khronos/gl.xml)
// ---------------------------------------------------------------------------
constexpr GLenum kFog = 0x0B60;
constexpr GLenum kAlphaTest = 0x0BC0;
constexpr GLenum kAlphaFunc = 0x0BC1;
constexpr GLenum kAlphaRef = 0x0BC2;
constexpr GLenum kTexture2D = 0x0DE1;
constexpr GLenum kNormalize = 0x0BA1;
constexpr GLenum kRescaleNormal = 0x803A;
constexpr GLenum kTexGenS = 0x0C60;
constexpr GLenum kFront = 0x0404;
constexpr GLenum kBack = 0x0405;
constexpr GLenum kFrontAndBack = 0x0408;
constexpr GLenum kEmission = 0x1600;
constexpr GLenum kAmbient = 0x1200;
constexpr GLenum kDiffuse = 0x1201;
constexpr GLenum kSpecular = 0x1202;
constexpr GLenum kAmbientAndDiffuse = 0x1602;
constexpr GLenum kSeparateSpecular = 0x81FA;
constexpr GLenum kFogCoord = 0x8451;
constexpr GLenum kFragmentDepth = 0x8452;
constexpr GLenum kFogExp = 0x0800;
constexpr GLenum kFogExp2 = 0x0801;
constexpr GLenum kFogLinear = 0x2601;
constexpr GLenum kObjectLinear = 0x2400;
constexpr GLenum kEyeLinear = 0x2401;
constexpr GLenum kSphereMap = 0x2402;
constexpr GLenum kReflectionMap = 0x2403;
constexpr GLenum kNormalMap = 0x2404;
constexpr GLenum kUnsignedInt = 0x1405;

// Mode của texture environment (GL 1.3 §3.8.12 + ARB_texture_env_combine).
// LƯU Ý: src/gl/gl_legacy_texenv.cpp dùng BẢNG enum KHÁC cho các hằng này
// (GL_REPLACE = 0x8001 thay vì 0x1E01, GL_BLEND = 0x0BE2 thay vì 0x2102,
// GL_SUBTRACT = 0x800A thay vì 0x84E7, GL_DOT3_RGB = 0x8570 thay vì 0x86AE,
// GL_DOT3_RGBA = 0x8E8F thay vì 0x86AF). Ta chấp nhận CẢ HAI bảng để đường
// fixed-function vẫn đúng với state do file đó ghi (xem docs/LEGACY_DRAW_PATH.md
// §"Follow-up bắt buộc").
enum EnvMode { kEnvReplace = 0, kEnvModulate, kEnvDecal, kEnvBlend, kEnvAdd, kEnvCombine };
enum CombFunc { kCombReplace = 0, kCombModulate, kCombAdd, kCombAddSigned,
                kCombInterpolate, kCombSubtract, kCombDot3RGB, kCombDot3RGBA };
// Chọn nguồn của combiner: 0 = texture của unit, 1 = constant, 2 = primary,
// 3 = previous, 4 = texture của unit kia (crossbar GL_TEXTURE0/1).
enum SrcSel { kSelOwn = 0, kSelConstant, kSelPrimary, kSelPrevious, kSelOther };
// Operand: 0 = SRC_COLOR, 1 = ONE_MINUS_SRC_COLOR, 2 = SRC_ALPHA,
// 3 = ONE_MINUS_SRC_ALPHA.
enum Operand { kOpColor = 0, kOpOneMinusColor, kOpAlpha, kOpOneMinusAlpha };
// Quy tắc ánh xạ texel → (Cs, As) theo base format (GL 1.3 Table 3.20).
enum SrcRule { kSrcRGBA = 0, kSrcRGB, kSrcLuminance, kSrcLuminanceAlpha,
               kSrcAlpha, kSrcIntensity, kSrcRed, kSrcRG };

// MapEnvMode: mode của texture environment.
// Nguồn chân lý: docs/khronos/gl.xml (nhóm "TextureEnvMode"):
//   GL_REPLACE 0x1E01, GL_MODULATE 0x2100, GL_DECAL 0x2101, GL_BLEND 0x0BE2,
//   GL_ADD 0x0104, GL_COMBINE 0x8570.
// Nhận CẢ giá trị trong bảng enum SAI của src/gl/gl_legacy_texenv.cpp
// (GL_REPLACE 0x8001, GL_BLEND 0x2102, GL_ADD 0x2104) để state do file đó ghi
// vẫn được hiểu đúng — xem docs/LEGACY_DRAW_PATH.md §"Follow-up bắt buộc".
int MapEnvMode(GLenum v) {
    switch (v) {
        case 0x1E01: case 0x8001: return kEnvReplace;   // GL_REPLACE
        case 0x2100: return kEnvModulate;                // GL_MODULATE
        case 0x2101: return kEnvDecal;                   // GL_DECAL
        case 0x0BE2: case 0x2102: return kEnvBlend;      // GL_BLEND
        case 0x0104: case 0x2104: return kEnvAdd;        // GL_ADD
        case 0x8570: return kEnvCombine;                // GL_COMBINE
        default: return kEnvModulate;
    }
}
// MapCombineFunc: hàm combiner (GL_COMBINE_RGB / GL_COMBINE_ALPHA).
// Mặc định của spec GL 1.3 §3.8.12 (p.147) là GL_MODULATE.
// Lưu ý: 0x8570 là GL_COMBINE (giá trị của *mode*), KHÔNG phải hàm combiner
// ⇒ không map sang DOT3_RGB (GL_DOT3_RGB = 0x86AE). Bảng enum cũ dùng 0x8570
// cho DOT3_RGB và giá trị mặc định của state cũng là 0x8570 ⇒ nếu map theo
// bảng sai thì mặc định thành DOT3_RGB thay vì MODULATE. Nay state lưu giá
// trị THẬT (spec GL 1.3 p.147) nên default = MODULATE là đúng.
int MapCombineFunc(GLenum v) {
    switch (v) {
        case 0x1E01: case 0x8001: return kCombReplace;
        case 0x2100: return kCombModulate;
        case 0x0104: case 0x2104: return kCombAdd;
        case 0x8574: return kCombAddSigned;
        case 0x8575: return kCombInterpolate;
        case 0x84E7: case 0x800A: return kCombSubtract;
        case 0x86AE: return kCombDot3RGB;
        case 0x86AF: case 0x8E8F: return kCombDot3RGBA;
        default: return kCombModulate;
    }
}
// MapSource: nguồn của combiner (slot 0/1/2 = SRC0/SRC1/SRC2).
// State lưu GIÁ TRỊ NGUỒN thật (spec GL 1.3 §3.8.12 p.147): mặc định
// SRC0_* = GL_TEXTURE, SRC1_* = GL_PREVIOUS, SRC2_* = GL_CONSTANT.
// (Các sentinel pname 0x8580/0x8588 vẫn được chấp nhận để tương thích với
//  state cũ ⇒ map về mặc định theo slot.)
int MapSource(GLenum v, int unit, int slot) {
    switch (v) {
        case 0x1702: return kSelOwn;        // GL_TEXTURE
        case 0x8576: return kSelConstant;   // GL_CONSTANT
        case 0x8577: return kSelPrimary;    // GL_PRIMARY_COLOR
        case 0x8578: return kSelPrevious;   // GL_PREVIOUS
        case 0x84C0: return (unit == 0) ? kSelOwn : kSelOther;  // GL_TEXTURE0
        case 0x84C1: return (unit == 1) ? kSelOwn : kSelOther;  // GL_TEXTURE1
        case 0x8580: case 0x8588:  // sentinel pname SRC0_* (state cũ)
            return slot == 0 ? kSelOwn : (slot == 1 ? kSelPrevious : kSelConstant);
        default: return kSelOwn;
    }
}
// MapOperand: operand của combiner.
// State lưu GIÁ TRỊ OPERAND thật: mặc định OPERAND0/1_RGB = GL_SRC_COLOR,
// OPERAND2_RGB = GL_SRC_ALPHA, OPERAND*_ALPHA = GL_SRC_ALPHA.
int MapOperand(GLenum v) {
    switch (v) {
        case 0x0300: return kOpColor;
        case 0x0301: return kOpOneMinusColor;
        case 0x0302: return kOpAlpha;
        case 0x0303: return kOpOneMinusAlpha;
        case 0x8592: return kOpAlpha;                       // sentinel OPERAND2_RGB
        case 0x8598: case 0x8599: case 0x859A: return kOpAlpha;  // sentinel OPERAND*_ALPHA
        default: return kOpColor;
    }
}
// Hàm alpha test (theo thứ tự enum GL): 0=NEVER 1=LESS 2=EQUAL 3=LEQUAL
// 4=GREATER 5=NOTEQUAL 6=GEQUAL 7=ALWAYS.
int MapAlphaFunc(GLenum v) {
    switch (v) {
        case 0x0200: return 0;
        case 0x0201: return 1;
        case 0x0202: return 2;
        case 0x0203: return 3;
        case 0x0204: return 4;
        case 0x0205: return 5;
        case 0x0206: return 6;
        default: return 7;
    }
}
uint32_t SrcRuleOf(const Context& c, int unit) {
    GLuint texId = c.state.BoundTexture((GLuint)unit);
    if (!texId) return kSrcRGBA;
    auto it = c.textures.find(texId);
    if (it == c.textures.end()) return kSrcRGBA;
    switch (it->second.internalFormat) {
        case 0x1903: case 0x8229: return kSrcRed;             // GL_RED / GL_R8
        case 0x8227: case 0x822B: return kSrcRG;              // GL_RG / GL_RG8
        case 0x1907: case 0x8051: return kSrcRGB;             // GL_RGB / GL_RGB8
        case 0x1908: case 0x8058: case 0x8C43: return kSrcRGBA;
        case 0x1909: return kSrcLuminance;
        case 0x190A: return kSrcLuminanceAlpha;
        case 0x1906: return kSrcAlpha;
        case 0x8049: return kSrcIntensity;
        default: return kSrcRGBA;
    }
}

// ---------------------------------------------------------------------------
// Toán: nghịch đảo 3x3 + ma trận normal
// ---------------------------------------------------------------------------
// m là 3x3 ROW-MAJOR (m[r*3+c]). Trả false nếu kì (⇒ dùng identity).
bool Inv3(const float m[9], float out[9]) {
    float det = m[0] * (m[4] * m[8] - m[5] * m[7]) -
                m[1] * (m[3] * m[8] - m[5] * m[6]) +
                m[2] * (m[3] * m[7] - m[4] * m[6]);
    if (std::fabs(det) < 1e-20f) return false;
    float id = 1.0f / det;
    out[0] = (m[4] * m[8] - m[5] * m[7]) * id;
    out[1] = -(m[1] * m[8] - m[2] * m[7]) * id;
    out[2] = (m[1] * m[5] - m[2] * m[4]) * id;
    out[3] = -(m[3] * m[8] - m[5] * m[6]) * id;
    out[4] = (m[0] * m[8] - m[2] * m[6]) * id;
    out[5] = -(m[0] * m[5] - m[2] * m[3]) * id;
    out[6] = (m[3] * m[7] - m[4] * m[6]) * id;
    out[7] = -(m[0] * m[7] - m[1] * m[6]) * id;
    out[8] = (m[0] * m[4] - m[1] * m[3]) * id;
    return true;
}

// Ma trận normal = transpose(inverse(3x3 trên-trái của modelview)).
// Xuất theo HÀNG: rows[r*3+c] = N[r][c] (N = inv(M)^T).
// Shader dùng nEye[r] = dot(rows[r], n) ⇒ nEye = N·n. ✓
void NormalMatrixRows(const GLMatrix& mv, float rows[9]) {
    float m3[9];
    for (int r = 0; r < 3; ++r)
        for (int cc = 0; cc < 3; ++cc) m3[r * 3 + cc] = mv.m[cc * 4 + r];
    float inv[9];
    if (!Inv3(m3, inv)) {
        for (int i = 0; i < 9; ++i) rows[i] = (i % 4 == 0) ? 1.0f : 0.0f;
        rows[0] = 1.0f; rows[4] = 1.0f; rows[8] = 1.0f;
        return;
    }
    for (int r = 0; r < 3; ++r)
        for (int cc = 0; cc < 3; ++cc) rows[r * 3 + cc] = inv[cc * 3 + r];
}

// ---------------------------------------------------------------------------
// Sinh MSL
// ---------------------------------------------------------------------------
// Định dạng số thực cho MSL: luôn có dấu thập phân + hậu tố 'f' để MSL
// không hiểu là double (MSL không có double).
std::string F(float v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.9g", (double)v);
    std::string s = buf;
    if (s.find("inf") != std::string::npos || s.find("nan") != std::string::npos)
        return "0.0f";
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos)
        s += ".0";
    s += "f";
    return s;
}

// Khối helper dùng chung cho cả VS và FS (mỗi stage là 1 file MSL riêng).
std::string CommonHelpers() {
    return
        "// --- helpers fixed-function (sinh tự động) ---\n"
        "static float4 TGLMTSource(float4 t, int rule) {\n"
        "  if (rule == 1) { return float4(t.rgb, 1.0); }              // RGB\n"
        "  if (rule == 2) { return float4(t.r, t.r, t.r, 1.0); }      // LUMINANCE\n"
        "  if (rule == 3) { return float4(t.r, t.r, t.r, t.a); }      // LUMINANCE_ALPHA\n"
        "  if (rule == 4) { return float4(0.0, 0.0, 0.0, t.a); }      // ALPHA\n"
        "  if (rule == 5) { return float4(t.r, t.r, t.r, t.r); }      // INTENSITY\n"
        "  if (rule == 6) { return float4(t.r, 0.0, 0.0, 1.0); }      // RED\n"
        "  if (rule == 7) { return float4(t.r, t.g, 0.0, 1.0); }      // RG\n"
        "  return t;                                                  // RGBA\n"
        "}\n"
        "static float TGLMTAlphaRule(int rule, int mode, float As, float Ap, float Ac) {\n"
        "  // mode: 0=REPLACE 1=MODULATE 2=DECAL 3=BLEND 4=ADD\n"
        "  if (mode == 0) {                                            // REPLACE\n"
        "    if (rule == 0 || rule == 3 || rule == 4 || rule == 5) { return As; }\n"
        "    return Ap;\n"
        "  }\n"
        "  if (mode == 1) {                                            // MODULATE\n"
        "    if (rule == 0 || rule == 3 || rule == 4 || rule == 5) { return Ap * As; }\n"
        "    return Ap;\n"
        "  }\n"
        "  if (mode == 2) { return Ap; }                                // DECAL\n"
        "  if (mode == 3) {                                            // BLEND\n"
        "    if (rule == 5) { return Ap * (1.0 - As) + Ac * As; }\n"
        "    if (rule == 0 || rule == 3 || rule == 4) { return Ap * As; }\n"
        "    return Ap;\n"
        "  }\n"
        "  if (rule == 5) { return Ap + As; }                          // ADD / INTENSITY\n"
        "  if (rule == 0 || rule == 3 || rule == 4) { return Ap * As; }\n"
        "  return Ap;\n"
        "}\n"
        "static float4 TGLMTSrc(int sel, float4 own, float4 other, float4 env,\n"
        "                       float4 Cf, float4 Cp) {\n"
        "  if (sel == 1) { return env; }\n"
        "  if (sel == 2) { return Cf; }\n"
        "  if (sel == 3) { return Cp; }\n"
        "  if (sel == 4) { return other; }\n"
        "  return own;\n"
        "}\n"
        "static float4 TGLMTArg(int operand, float4 s) {\n"
        "  if (operand == 1) { return float4(1.0 - s.rgb, 1.0 - s.a); }\n"
        "  if (operand == 2) { return float4(s.a, s.a, s.a, s.a); }\n"
        "  if (operand == 3) { return float4(1.0 - s.a); }\n"
        "  return s;\n"
        "}\n"
        "static float4 TGLMTArgA(int operand, float4 s) {\n"
        "  if (operand == 3) { return float4(1.0 - s.a); }\n"
        "  return float4(s.a);\n"
        "}\n"
        "static float TGLMTCombine(int func, float a0, float a1, float a2) {\n"
        "  if (func == 1) { return a0 * a1; }                            // MODULATE\n"
        "  if (func == 2) { return a0 + a1; }                            // ADD\n"
        "  if (func == 3) { return a0 + a1 - 0.5; }                      // ADD_SIGNED\n"
        "  if (func == 4) { return a0 * a2 + a1 * (1.0 - a2); }          // INTERPOLATE\n"
        "  if (func == 5) { return a0 - a1; }                            // SUBTRACT\n"
        "  return a0;                                                    // REPLACE\n"
        "}\n";
}

// Khối lighting per-vertex (spec GL 2.1 §2.14.1 — công thức verbatim trong
// docs/LEGACY_LIGHTING_SPEC.md §5.2, tham chiếu implementation §6).
std::string LightingHelper() {
    return
        "static void TGLMTLegacyLight(float3 n, float4 eye, float4 vcol,\n"
        "                             float4 matA, float4 matD, float4 matS,\n"
        "                             float4 matE, float shin, float4 mask,\n"
        "                             constant TGLMTLegacyU& u,\n"
        "                             thread float3& cpri, thread float3& csec,\n"
        "                             thread float4& matDeff) {\n"
        "  // glColorMaterial: vật liệu được thay bằng current color (mask baked).\n"
        "  matA = matA * (1.0 - mask.x) + vcol * mask.x;\n"
        "  matD = matD * (1.0 - mask.y) + vcol * mask.y;\n"
        "  matS = matS * (1.0 - mask.z) + vcol * mask.z;\n"
        "  matE = matE * (1.0 - mask.w) + vcol * mask.w;\n"
        "  matDeff = matD;   // alpha của màu primary = alpha diffuse (spec 2.14.1)\n"
        "  // c_pri = e_cm + a_cm * a_cs   (spec 2.14.1)\n"
        "  cpri = matE.rgb + matA.rgb * u.sceneAmb.rgb;\n"
        "  csec = float3(0.0);\n"
        "  for (uint i = 0u; i < uint(u.flags.w); ++i) {\n"
        "    float4 lAmb  = u.lights[i * 7u + 0u];\n"
        "    float4 lDif  = u.lights[i * 7u + 1u];\n"
        "    float4 lSpc  = u.lights[i * 7u + 2u];\n"
        "    float4 lPos  = u.lights[i * 7u + 3u];\n"
        "    float4 lSpot = u.lights[i * 7u + 4u];   // xyz = sd_li, w = s_rli\n"
        "    float4 lPar  = u.lights[i * 7u + 5u];   // x = c_rli, yzw = k0,k1,k2\n"
        "    float4 lEn   = u.lights[i * 7u + 6u];   // x = bật/tắt\n"
        "    if (lEn.x < 0.5) { continue; }\n"
        "    // L = →V P_pl (đơn vị từ đỉnh tới nguồn sáng)\n"
        "    float3 Ldir = float3(0.0, 0.0, 1.0);\n"
        "    float dist = 0.0;\n"
        "    if (lPos.w != 0.0) {\n"
        "      float3 d = lPos.xyz / lPos.w - eye.xyz / max(abs(eye.w), 1e-20f);\n"
        "      dist = length(d);\n"
        "      Ldir = (dist > 0.0) ? (d / dist) : float3(0.0);\n"
        "    } else {\n"
        "      float pl = length(lPos.xyz);\n"
        "      Ldir = (pl > 0.0) ? (lPos.xyz / pl) : float3(0.0, 0.0, 1.0);\n"
        "    }\n"
        "    // att_i = 1/(k0 + k1·d + k2·d²) — nguồn sáng hướng (w==0) KHÔNG suy giảm\n"
        "    float att = 1.0;\n"
        "    if (lPos.w != 0.0) {\n"
        "      float den = lPar.y + lPar.z * dist + lPar.w * dist * dist;\n"
        "      att = (den != 0.0) ? (1.0 / den) : 0.0;\n"
        "    }\n"
        "    // spot_i — KIỂM TRA CUTOFF TRƯỚC, số mũ SAU (spec 2.14.1 eq 2.5)\n"
        "    float spot = 1.0;\n"
        "    if (lPar.x != 180.0) {\n"
        "      float sl = length(lSpot.xyz);\n"
        "      float3 sdn = (sl > 0.0) ? (lSpot.xyz / sl) : float3(0.0, 0.0, -1.0);\n"
        "      float cs = max(dot(-Ldir, sdn), 0.0);\n"
        "      float cosCut = cos(lPar.x * 0.0174532925f);\n"
        "      spot = (cs >= cosCut) ? pow(cs, lSpot.w) : 0.0;\n"
        "    }\n"
        "    float ndotl = max(dot(n, Ldir), 0.0);\n"
        "    float fi = (ndotl != 0.0) ? 1.0 : 0.0;   // f_i chặn specular\n"
        "    float s;\n"
        "    if (shin == 0.0) {\n"
        "      s = fi;                                // pow(x,0)=1 ⇒ lobe phẳng\n"
        "    } else {\n"
        "      float3 h;\n"
        "      if (u.flags.x > 0.5) {                 // local viewer\n"
        "        float3 vv = -eye.xyz / max(abs(eye.w), 1e-20f);\n"
        "        float vl = length(vv);\n"
        "        h = normalize(Ldir + ((vl > 0.0) ? (vv / vl) : float3(0.0)));\n"
        "      } else {                               // viewer ở vô cùng\n"
        "        h = normalize(Ldir + float3(0.0, 0.0, 1.0));\n"
        "      }\n"
        "      float ndoth = max(dot(n, h), 0.0);\n"
        "      s = fi * pow(ndoth, shin);\n"
        "    }\n"
        "    float wgt = att * spot;\n"
        "    float3 spec = s * matS.rgb * lSpc.rgb;\n"
        "    cpri += wgt * (matA.rgb * lAmb.rgb + ndotl * matD.rgb * lDif.rgb);\n"
        "    if (u.flags.z > 0.5) { csec += wgt * spec; }   // SEPARATE_SPECULAR_COLOR\n"
        "    else { cpri += wgt * spec; }\n"
        "  }\n"
        "}\n";
}

std::string BuildVertexMsl(const LegacyProgramShape& sh) {
    std::ostringstream o;
    o << "#include <metal_stdlib>\nusing namespace metal;\n\n";
    // GL_FLAT: màu lấy từ đỉnh provoking (Metal flat = đỉnh CUỐI của
    // primitive — khớp provoking vertex mặc định của GL 3.2+ compat).
    const char* flatQ = sh.flatShading ? "[[flat, user(" : "[[user(";
    o << "struct TGLMTLegacyU {\n"
         "  float4 mvc[4];      // model-view (cột)\n"
         "  float4 pjc[4];      // projection (cột)\n"
         "  float4 nrm[3];      // ma trận normal (HÀNG)\n"
         "  float4 lights[56];  // 8 nguồn × 7 float4\n"
         "  float4 matF[5];     // ambient/diffuse/specular/emission/shininess\n"
         "  float4 matB[5];\n"
         "  float4 sceneAmb;\n"
         "  float4 flags;       // localViewer, twoSided, separateSpecular, numLights\n"
         "  float4 flags2;      // normalize, rescaleNormal\n"
         "};\n\n";
    o << "struct TGLMTLegacyOut {\n"
         "  float4 position [[position]];\n";
    o << "  float4 color " << flatQ << "locn0)]];\n"
         "  float4 backColor " << flatQ << "locn1)]];\n"
         "  float4 secColor " << flatQ << "locn2)]];\n"
         "  float4 backSecColor " << flatQ << "locn3)]];\n"
         "  float4 tex0 [[user(locn4)]];\n"
         "  float4 tex1 [[user(locn5)]];\n"
         "  float3 eyePos [[user(locn6)]];\n"
         "};\n\n";
    if (sh.lighting) o << LightingHelper() << "\n";
    o << "vertex TGLMTLegacyOut TGLMT_vs(\n"
         "    float4 inPos      [[attribute(0)]],\n"
         "    float4 inNormal   [[attribute(1)]],\n"
         "    float4 inColor    [[attribute(2)]],\n"
         "    float4 inTex0     [[attribute(3)]],\n"
         "    float4 inTex1     [[attribute(4)]],\n"
         "    float4 inSecColor [[attribute(5)]],\n"
         "    constant TGLMTLegacyU& u [[buffer(16)]])\n{\n"
         "  TGLMTLegacyOut o;\n"
         "  float4 eye = float4(dot(u.mvc[0], inPos), dot(u.mvc[1], inPos),\n"
         "                       dot(u.mvc[2], inPos), dot(u.mvc[3], inPos));\n"
         "  float4 clip = float4(dot(u.pjc[0], eye), dot(u.pjc[1], eye),\n"
         "                        dot(u.pjc[2], eye), dot(u.pjc[3], eye));\n"
         "  o.position = clip;\n"
         "  // Metal NDC z ∈ [0,w]; GL clip z ∈ [-w,w] ⇒ cùng phép chuyển với\n"
         "  // đường modern (glsl_to_msl.cpp kZConv).\n"
         "  o.position.z = clip.z * 0.5 + clip.w * 0.5;\n"
         "  o.eyePos = eye.xyz;\n"
         "  o.tex0 = inTex0;\n"
         "  o.tex1 = inTex1;\n";
    if (sh.lighting) {
        o << "  // --- normal: transform bằng inverse-transpose (spec 2.11.3) ---\n"
             "  float3 n = float3(dot(u.nrm[0].xyz, inNormal.xyz),\n"
             "                     dot(u.nrm[1].xyz, inNormal.xyz),\n"
             "                     dot(u.nrm[2].xyz, inNormal.xyz));\n";
        if (sh.normalize) {
            o << "  n = normalize(n);\n";
        } else if (sh.rescaleNormal) {
            o << "  n = n * (length(inNormal.xyz) / max(length(n), 1e-20f));\n";
        }
        o << "  float3 cpri = float3(0.0);\n  float3 csec = float3(0.0);\n"
             "  float4 matDf = u.matF[1];\n"
             "  TGLMTLegacyLight(n, eye, inColor, u.matF[0], u.matF[1], u.matF[2],\n"
             "                 u.matF[3], u.matF[4].x,\n"
             "                 float4(" << F((float)(sh.colorMaterialMask & 1u)) << ", "
             << F((float)((sh.colorMaterialMask >> 1) & 1u)) << ", "
             << F((float)((sh.colorMaterialMask >> 2) & 1u)) << ", "
             << F((float)((sh.colorMaterialMask >> 3) & 1u)) << "), u, cpri, csec, matDf);\n"
             "  // alpha của primary = alpha của diffuse (spec 2.14.1 F-13); khi\n"
             "  // glColorMaterial thay diffuse bằng current color thì alpha đi theo.\n"
             "  o.color = float4(clamp(cpri, 0.0, 1.0), matDf.a);\n"
             "  o.secColor = float4(clamp(csec, 0.0, 1.0), 1.0);\n";
        if (sh.twoSided) {
            o << "  float3 bpri = float3(0.0);\n  float3 bsec = float3(0.0);\n"
                 "  float4 matDb = u.matB[1];\n"
                 "  TGLMTLegacyLight(-n, eye, inColor, u.matB[0], u.matB[1], u.matB[2],\n"
                 "                 u.matB[3], u.matB[4].x,\n"
                 "                 float4(" << F((float)(sh.colorMaterialMaskBack & 1u)) << ", "
                 << F((float)((sh.colorMaterialMaskBack >> 1) & 1u)) << ", "
                 << F((float)((sh.colorMaterialMaskBack >> 2) & 1u)) << ", "
                 << F((float)((sh.colorMaterialMaskBack >> 3) & 1u)) << "), u, bpri, bsec, matDb);\n"
                 "  o.backColor = float4(clamp(bpri, 0.0, 1.0), matDb.a);\n"
                 "  o.backSecColor = float4(clamp(bsec, 0.0, 1.0), 1.0);\n";
        } else {
            o << "  o.backColor = o.color;\n  o.backSecColor = o.secColor;\n";
        }
    } else {
        // Lighting tắt: current color / current secondary color gán thẳng cho
        // vertex (spec 2.14.1).
        o << "  o.color = inColor;\n  o.secColor = inSecColor;\n"
             "  o.backColor = inColor;\n  o.backSecColor = inSecColor;\n";
    }
    o << "  return o;\n}\n";
    return o.str();
}

std::string BuildFragmentMsl(const LegacyProgramShape& sh) {
    std::ostringstream o;
    o << "#include <metal_stdlib>\nusing namespace metal;\n\n";
    const char* flatQ = sh.flatShading ? "[[flat, user(" : "[[user(";
    o << "struct TGLMTLegacyFU {\n"
         "  float4 fogParams;   // mode(0=LINEAR,1=EXP,2=EXP2), density, start, end\n"
         "  float4 fogColor;\n"
         "  float4 fogCoord;    // x = giá trị fog coord, y = nguồn (1 = GL_FOG_COORD)\n"
         "  float4 alphaTest;   // x = hàm (0..7), y = ref\n"
         "  float4 envColor[2];\n"
         "  float4 combA[2];    // src0RGB, src1RGB, src2RGB, combineRGB\n"
         "  float4 combB[2];    // src0A, src1A, src2A, combineAlpha\n"
         "  float4 combC[2];    // op0RGB, op1RGB, op2RGB, rgbScale\n"
         "  float4 combD[2];    // op0A, op1A, op2A, alphaScale\n"
         "};\n\n";
    o << "struct TGLMTLegacyIn {\n";
    o << "  float4 color " << flatQ << "locn0)]];\n"
         "  float4 backColor " << flatQ << "locn1)]];\n"
         "  float4 secColor " << flatQ << "locn2)]];\n"
         "  float4 backSecColor " << flatQ << "locn3)]];\n"
         "  float4 tex0 [[user(locn4)]];\n"
         "  float4 tex1 [[user(locn5)]];\n"
         "  float3 eyePos [[user(locn6)]];\n"
         "};\n\n";
    o << CommonHelpers() << "\n";
    o << "fragment float4 TGLMT_fs(TGLMTLegacyIn in [[stage_in]],\n";
    for (int u = 0; u < sh.numTexUnits; ++u) {
        o << "                         texture2d<float> tex" << u << " [[texture(" << u
          << ")]], sampler smp" << u << " [[sampler(" << u << ")]],\n";
    }
    o << "                         constant TGLMTLegacyFU& u [[buffer(16)]],\n"
         "                         bool frontFacing [[front_facing]]) {\n";
    // Chọn mặt (two-sided) TRƯỚC texture env: texture env chỉ tác động màu
    // primary (spec GL 2.1 §3.8.16). Cf = màu primary ĐÃ CHỌN MẶT — nếu Cf
    // lấy in.color thì màu "back" bị bỏ qua khi có texture.
    o << "  float4 primary = frontFacing ? in.color : in.backColor;\n"
         "  float4 secondary = frontFacing ? in.secColor : in.backSecColor;\n"
         "  float4 Cf = primary;    // primary color trước texture env (đã chọn mặt)\n"
         "  float4 prev = Cf;       // kết quả của unit trước (nguồn PREVIOUS)\n";
    for (int u = 0; u < sh.numTexUnits; ++u) {
        o << "  float4 raw" << u << " = tex" << u << ".sample(smp" << u << ", in.tex" << u
          << ".xy);\n";
        o << "  float4 cs" << u << " = TGLMTSource(raw" << u << ", " << sh.srcRule[u] << ");\n";
        o << "  float4 env" << u << " = u.envColor[" << u << "];\n";
    }
    for (int u = 0; u < sh.numTexUnits; ++u) {
        const char* other = (u == 0) ? ((sh.numTexUnits > 1) ? "cs1" : "cs0") : "cs0";
        o << "  {\n";
        o << "    float4 Cp = " << (u == 0 ? "Cf" : "prev") << ";\n";
        o << "    float4 own = cs" << u << ";\n";
        o << "    float4 oth = " << other << ";\n";
        switch (sh.texEnvMode[u]) {
            case kEnvReplace:
                o << "    float4 res = float4(own.rgb, TGLMTAlphaRule(" << sh.alphaRule[u]
                  << ", 0, own.a, Cp.a, env" << u << ".a));\n";
                break;
            case kEnvModulate:
                o << "    float4 res = float4(Cp.rgb * own.rgb, TGLMTAlphaRule(" << sh.alphaRule[u]
                  << ", 1, own.a, Cp.a, env" << u << ".a));\n";
                break;
            case kEnvDecal:
                // GL 1.3 Table 3.21 (RGBA/RGB): Cv = Cp*(1-As) + Cs*As, Av = Ap.
                // Trên base format KHÁC (ALPHA/LUMINANCE/...) DECAL là
                // "undefined" ⇒ theo quy tắc D2 của
                // docs/LEGACY_TEXENV_FOG_SPEC.md: giữ nguyên màu đến (no-op).
                if (sh.srcRule[u] == kSrcRGBA || sh.srcRule[u] == kSrcRGB) {
                    o << "    float4 res = float4(Cp.rgb * (1.0 - own.a) + own.rgb * own.a, Cp.a);\n";
                } else {
                    o << "    float4 res = Cp;   // DECAL undefined ⇒ no-op (doc D2)\n";
                }
                break;
            case kEnvBlend:
                // GL 1.3 Table 3.22: Cv = Cp*(1-Cs) + Cc*Cs.
                o << "    float4 res = float4(Cp.rgb * (1.0 - own.rgb) + env" << u
                  << ".rgb * own.rgb, TGLMTAlphaRule(" << sh.alphaRule[u]
                  << ", 3, own.a, Cp.a, env" << u << ".a));\n";
                break;
            case kEnvAdd:
                // GL 1.3 Table 3.22: Cv = min(1, Cp + Cs).
                o << "    float4 res = float4(min(1.0, Cp.rgb + own.rgb), TGLMTAlphaRule("
                  << sh.alphaRule[u] << ", 4, own.a, Cp.a, env" << u << ".a));\n";
                break;
            case kEnvCombine:
            default: {
                o << "    float4 s0 = TGLMTSrc(int(u.combA[" << u << "].x), own, oth, env" << u
                  << ", Cf, Cp);\n";
                o << "    float4 s1 = TGLMTSrc(int(u.combA[" << u << "].y), own, oth, env" << u
                  << ", Cf, Cp);\n";
                o << "    float4 s2 = TGLMTSrc(int(u.combA[" << u << "].z), own, oth, env" << u
                  << ", Cf, Cp);\n";
                o << "    float4 a0 = TGLMTArg(int(u.combC[" << u << "].x), s0);\n";
                o << "    float4 a1 = TGLMTArg(int(u.combC[" << u << "].y), s1);\n";
                o << "    float4 a2 = TGLMTArg(int(u.combC[" << u << "].z), s2);\n";
                o << "    float4 b0 = TGLMTArgA(int(u.combD[" << u << "].x), TGLMTSrc(int(u.combB["
                  << u << "].x), own, oth, env" << u << ", Cf, Cp));\n";
                o << "    float4 b1 = TGLMTArgA(int(u.combD[" << u << "].y), TGLMTSrc(int(u.combB["
                  << u << "].y), own, oth, env" << u << ", Cf, Cp));\n";
                o << "    float4 b2 = TGLMTArgA(int(u.combD[" << u << "].z), TGLMTSrc(int(u.combB["
                  << u << "].z), own, oth, env" << u << ", Cf, Cp));\n";
                const int cr = (int)sh.combineRGB[u];
                if (cr == kCombDot3RGB || cr == kCombDot3RGBA) {
                    o << "    float d = 4.0 * ((a0.r - 0.5) * (a1.r - 0.5) +\n"
                         "                        (a0.g - 0.5) * (a1.g - 0.5) +\n"
                         "                        (a0.b - 0.5) * (a1.b - 0.5));\n";
                    o << "    float3 rgbc = float3(d);\n";
                } else {
                    o << "    float3 rgbc = float3(TGLMTCombine(int(u.combA[" << u
                      << "].w), a0.r, a1.r, a2.r),\n"
                         "                        TGLMTCombine(int(u.combA[" << u
                      << "].w), a0.g, a1.g, a2.g),\n"
                         "                        TGLMTCombine(int(u.combA[" << u
                      << "].w), a0.b, a1.b, a2.b));\n";
                }
                o << "    float alphac = TGLMTCombine(int(u.combB[" << u << "].w), b0.a, b1.a, b2.a);\n";
                if (cr == kCombDot3RGBA) {
                    o << "    float4 res = float4(rgbc * u.combC[" << u
                      << "].w, d * u.combD[" << u << "].w);\n";
                } else {
                    o << "    float4 res = float4(rgbc * u.combC[" << u << "].w,\n"
                         "                         alphac * u.combD[" << u << "].w);\n";
                }
                o << "    res = clamp(res, 0.0, 1.0);   // spec 3.8.13: clamp [0,1]\n";
                break;
            }
        }
        o << "    primary = res;\n";
        o << "    prev = res;\n";
        o << "  }\n";
    }
    // Colour sum (spec GL 4.2C §16.3): primary + secondary.
    o << "  float4 outc = float4(primary.rgb + secondary.rgb, primary.a);\n";
    if (sh.fog) {
        o << "  // --- fog (per-fragment, SAU texture env) ---\n"
             "  float fogc = (u.fogCoord.y > 0.5) ? u.fogCoord.x : length(in.eyePos);\n"
             "  float fg;\n"
             "  int fm = int(u.fogParams.x);\n"
         "  if (fm == 0) {                       // GL_LINEAR\n"
         "    float s = u.fogParams.z, e = u.fogParams.w;\n"
         "    // s == e ⇒ kết quả undefined; quy tắc D6 của\n"
         "    // docs/LEGACY_TEXENV_FOG_SPEC.md: f = 0 (kín sương).\n"
         "    fg = (e != s) ? ((e - fogc) / (e - s)) : 0.0;\n"

             "  } else if (fm == 1) {                // GL_EXP\n"
             "    fg = exp(-u.fogParams.y * fogc);\n"
             "  } else {                             // GL_EXP2\n"
             "    float dd = u.fogParams.y * fogc;\n"
             "    fg = exp(-(dd * dd));\n"
             "  }\n"
             "  fg = clamp(fg, 0.0, 1.0);            // spec 3.10: clamp [0,1]\n"
             "  outc.rgb = fg * outc.rgb + (1.0 - fg) * u.fogColor.rgb;\n";
    }
    if (sh.alphaTest) {
        o << "  // --- alpha test (per-fragment op) ---\n"
             "  {\n"
             "    float a = outc.a;\n"
             "    float r = u.alphaTest.y;\n"
             "    int af = int(u.alphaTest.x);\n"
             "    bool pass = true;\n"
             "    if (af == 0) { pass = false; }\n"
             "    else if (af == 1) { pass = (a < r); }\n"
             "    else if (af == 2) { pass = (a == r); }\n"
             "    else if (af == 3) { pass = (a <= r); }\n"
             "    else if (af == 4) { pass = (a > r); }\n"
             "    else if (af == 5) { pass = (a != r); }\n"
             "    else if (af == 6) { pass = (a >= r); }\n"
             "    if (!pass) { discard_fragment(); }\n"
             "  }\n";
    }
    o << "  return outc;\n}\n";
    return o.str();
}

// ---------------------------------------------------------------------------
// Đóng gói uniform
// ---------------------------------------------------------------------------
struct USlot {
    std::string name;
    float v[4] = {0, 0, 0, 0};
};

void PushU(std::vector<USlot>& v, const char* name, float a, float b, float cc, float d) {
    USlot s;
    s.name = name;
    s.v[0] = a; s.v[1] = b; s.v[2] = cc; s.v[3] = d;
    v.push_back(s);
}
void PushU(std::vector<USlot>& v, const char* name, const float* src, int n) {
    USlot s;
    s.name = name;
    for (int i = 0; i < 4; ++i) s.v[i] = (i < n) ? src[i] : 0.0f;
    v.push_back(s);
}

// Ghi layout + bytes vào ProgramObject (đi qua đường staging sẵn có).
void InstallUniforms(ProgramObject& pr, const std::vector<USlot>& vs,
                      const std::vector<USlot>& fs) {
    pr.uniformLayout.clear();
    pr.uniforms.clear();
    const size_t vsBytes = vs.size() * 16;
    const size_t base = (vsBytes + 15) & ~(size_t)15;
    pr.vsUBSize = base;
    pr.fsUBSize = fs.size() * 16;
    GLint loc = 0;
    size_t off = 0;
    for (const auto& s : vs) {
        ProgramObject::UniformEntry e;
        e.name = s.name;
        e.offset = off;
        e.size = 16;
        e.loc = loc;
        e.isVS = true;
        e.glType = 0x8B50;  // GL_FLOAT_VEC4
        e.glslType = "vec4";
        e.arraySize = 0;
        pr.uniformLayout.push_back(e);
        ProgramObject::UniformSlot& us = pr.uniforms[loc];
        us.bytes.assign((const uint8_t*)s.v, (const uint8_t*)s.v + 16);
        us.spec.type = 0x8B50;
        us.spec.arraySize = 1;
        us.spec.storeType = 0x1406;
        off += 16;
        ++loc;
    }
    off = base;
    for (const auto& s : fs) {
        ProgramObject::UniformEntry e;
        e.name = s.name;
        e.offset = off;
        e.size = 16;
        e.loc = loc;
        e.isVS = false;
        e.glType = 0x8B50;
        e.glslType = "vec4";
        e.arraySize = 0;
        pr.uniformLayout.push_back(e);
        ProgramObject::UniformSlot& us = pr.uniforms[loc];
        us.bytes.assign((const uint8_t*)s.v, (const uint8_t*)s.v + 16);
        us.spec.type = 0x8B50;
        us.spec.arraySize = 1;
        us.spec.storeType = 0x1406;
        off += 16;
        ++loc;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// API công khai
// ---------------------------------------------------------------------------
bool LegacyFixedFunctionAllowed(const Context& c) {
    // Core Profile không có fixed-function (GL 4.6 Core §17.1) ⇒ giữ hành vi
    // cũ (draw không program ⇒ bị bỏ, không sinh lỗi).
    return c.glProfile != GlProfile::Core;
}

bool LegacySelectProgram(Context& c, GLuint program, ProgramObject*& out) {
    out = nullptr;
    // GL 2.1 §2.15 / GL 4.6 Core §1.2.4: có program object trong use ⇒
    // programmable pipeline chạy, fixed-function KHÔNG dùng.
    if (program != 0) return false;
    // Core Profile không có fixed-function.
    if (!LegacyFixedFunctionAllowed(c)) return false;
    // Chỉ draw fixed-function THẬT (batch immediate-mode do
    // LegacyDrawImmediate phát). Draw modern không có program vẫn bị bỏ
    // (noProgram++) như trước khi có đường legacy — vertex data của nó không
    // nằm trong layout vertex legacy.
    if (!c.legacy.drawActive) return false;
    if (!LegacyEnsureProgram(c)) return false;
    out = c.legacy.program;
    return out != nullptr;
}

LegacyProgramShape LegacyComputeShape(const Context& c) {
    const LegacyState& L = c.legacy;
    LegacyProgramShape sh;
    sh.lighting = L.lightingEnabled;
    sh.fog = c.state.IsEnabled(kFog);
    sh.alphaTest = c.state.IsEnabled(kAlphaTest);
    sh.separateSpecular = (L.lightModel.colorControl == kSeparateSpecular);
    sh.twoSided = L.lightModel.twoSide;
    sh.normalize = c.state.IsEnabled(kNormalize);
    sh.rescaleNormal = c.state.IsEnabled(kRescaleNormal);
    // Shade model: GL_FLAT ⇒ màu lấy từ đỉnh provoking (varying [[flat]]).
    // Nhận CẢ hai giá trị: 0x1D00 (GL_FLAT đúng theo docs/khronos/gl.xml) và
    // 0x8DDB (giá trị src/gl/gl_legacy_light.cpp đang dùng cho kFlat — bảng
    // enum sai, xem docs/LEGACY_DRAW_PATH.md §"Follow-up bắt buộc").
    sh.flatShading = (L.shadeModel == 0x1D00 /*GL_FLAT*/ || L.shadeModel == 0x8DDB);
    // glColorMaterial: mask cho mặt trước / mặt sau.
    if (L.colorMaterialEnabled) {
        uint32_t m = 0;
        switch (L.colorMaterial.mode) {
            case kEmission: m = 8u; break;
            case kAmbient: m = 1u; break;
            case kDiffuse: m = 2u; break;
            case kSpecular: m = 4u; break;
            case kAmbientAndDiffuse: m = 3u; break;
            default: m = 0u; break;
        }
        if (L.colorMaterial.face == kFront || L.colorMaterial.face == kFrontAndBack)
            sh.colorMaterialMask = m;
        if (L.colorMaterial.face == kBack || L.colorMaterial.face == kFrontAndBack)
            sh.colorMaterialMaskBack = m;
    }
    // Số texture unit "đang dùng": unit cao nhất có texture bound + 1.
    int n = 0;
    if (c.state.IsEnabled(kTexture2D)) {
        for (int u = 0; u < kLegacyTexUnits; ++u)
            if (c.state.BoundTexture((GLuint)u)) n = u + 1;
    }
    sh.numTexUnits = n;
    for (int u = 0; u < n; ++u) {
        sh.texEnvMode[u] = (uint32_t)MapEnvMode(L.texEnv[u].mode);
        sh.combineRGB[u] = (uint32_t)MapCombineFunc(L.texEnv[u].combineRGB);
        sh.combineAlpha[u] = (uint32_t)MapCombineFunc(L.texEnv[u].combineAlpha);
        sh.srcRule[u] = SrcRuleOf(c, u);
        sh.alphaRule[u] = sh.srcRule[u];
    }
    // Chữ ký shape (FNV-1a) — quyết định có sinh lại chương trình hay không.
    uint64_t k = 1469598103934665603ull;
    auto mix = [&k](uint64_t v) { k ^= v; k *= 1099511628211ull; };
    mix(sh.lighting ? 1 : 0);
    mix(sh.fog ? 1 : 0);
    mix(sh.alphaTest ? 1 : 0);
    mix(sh.separateSpecular ? 1 : 0);
    mix(sh.twoSided ? 1 : 0);
    mix(sh.normalize ? 1 : 0);
    mix(sh.rescaleNormal ? 1 : 0);
    mix(sh.flatShading ? 1 : 0);
    mix((uint64_t)sh.numTexUnits);
    for (int u = 0; u < kLegacyTexUnits; ++u) {
        mix(sh.texEnvMode[u]);
        mix(sh.combineRGB[u]);
        mix(sh.combineAlpha[u]);
        mix(sh.srcRule[u]);
    }
    mix(sh.colorMaterialMask);
    mix(sh.colorMaterialMaskBack);
    sh.key = k;
    return sh;
}

bool LegacyEnsureProgram(Context& c) {
    ProgramObject* pr = c.legacy.program;
    if (!pr) {
        pr = &c.legacyProgram;
        c.legacy.program = pr;
    }
    LegacyProgramShape sh = LegacyComputeShape(c);
    if (c.legacy.programValid && c.legacy.shapeKey == sh.key) {
        // Hình dạng không đổi ⇒ chỉ nạp lại GIÁ TRỊ state (uniform), không
        // sinh lại chương trình (đây là điểm mấu chốt của thiết kế: đổi màu
        // đèn KHÔNG làm hỏng pipeline cache).
        LegacyUploadUniforms(c);
        return true;
    }

    // Sinh MSL mới.
    std::string vsMsl = BuildVertexMsl(sh);
    std::string fsMsl = BuildFragmentMsl(sh);
    pr->vertexMSL = vsMsl;
    pr->fragmentMSL = fsMsl;
    pr->uniformLayout.clear();
    pr->uniforms.clear();
    pr->vsUBSize = 0;
    pr->fsUBSize = 0;
    pr->fsSamplers.clear();
    pr->vsSamplers.clear();
    pr->samplerUnits.clear();
    pr->samplerKind.clear();
    pr->samplerSlots.clear();
    pr->appleVS.reset();
    pr->appleFS.reset();
    pr->appleFSMasked.reset();

    // Sampler: mỗi texture unit = 1 texture + 1 sampler ở slot k (đúng thứ tự
    // fsSamplers mà AppleDrawGL bind).
    for (int u = 0; u < sh.numTexUnits; ++u) {
        std::string nm = "tex" + std::to_string(u);
        pr->fsSamplers.push_back(nm);
        pr->samplerUnits[nm] = (GLuint)u;
        pr->samplerKind[nm] = '2';
    }

    // Compile (Apple). Backend Null không compile MSL ⇒ vẫn giữ MSL để test
    // kiểm tra, nhưng không có library ⇒ không encode GPU (đúng: Null không có
    // GPU).
    std::string err;
    pr->appleVS = c.device->compileLibrary(vsMsl, err);
    if (!pr->appleVS) err.clear();
    pr->appleFS = c.device->compileLibrary(fsMsl, err);
    pr->linked = true;
    // linkGen tăng ⇒ PipelineKey::programGen mới ⇒ KHÔNG trả nhầm pipeline của
    // chương trình cũ (docs §2.4/§2.5).
    ++pr->linkGen;
    c.legacy.linkGen = pr->linkGen;
    c.legacy.shapeKey = sh.key;
    c.legacy.programValid = true;
    LegacyUploadUniforms(c);
    return true;
}

void LegacyUploadUniforms(Context& c) {
    ProgramObject* pr = c.legacy.program;
    if (!pr) return;
    const LegacyState& L = c.legacy;
    const GLMatrix& mv = L.modelview.top();
    const GLMatrix& pj = L.projection.top();

    // ---- VS ----
    std::vector<USlot> vs;
    vs.reserve(96);
    for (int i = 0; i < 4; ++i) {
        char nm[16];
        snprintf(nm, sizeof(nm), "mvc%d", i);
        PushU(vs, nm, mv.m[i * 4 + 0], mv.m[i * 4 + 1], mv.m[i * 4 + 2], mv.m[i * 4 + 3]);
    }
    for (int i = 0; i < 4; ++i) {
        char nm[16];
        snprintf(nm, sizeof(nm), "pjc%d", i);
        PushU(vs, nm, pj.m[i * 4 + 0], pj.m[i * 4 + 1], pj.m[i * 4 + 2], pj.m[i * 4 + 3]);
    }
    float nrows[9];
    NormalMatrixRows(mv, nrows);
    for (int r = 0; r < 3; ++r) {
        char nm[16];
        snprintf(nm, sizeof(nm), "nrm%d", r);
        PushU(vs, nm, nrows[r * 3 + 0], nrows[r * 3 + 1], nrows[r * 3 + 2], 0.0f);
    }
    // Lights: các nguồn sáng BẬT được đóng gói lên đầu mảng (docs §5.2).
    int nLit = 0;
    for (int i = 0; i < kMaxLights; ++i) {
        const LegacyLight& Lg = L.lights[i];
        if (!Lg.enabled) continue;
        char nm[24];
        snprintf(nm, sizeof(nm), "light%d_amb", nLit);
        PushU(vs, nm, Lg.ambient, 4);
        snprintf(nm, sizeof(nm), "light%d_dif", nLit);
        PushU(vs, nm, Lg.diffuse, 4);
        snprintf(nm, sizeof(nm), "light%d_spc", nLit);
        PushU(vs, nm, Lg.specular, 4);
        snprintf(nm, sizeof(nm), "light%d_pos", nLit);
        PushU(vs, nm, Lg.position, 4);
        snprintf(nm, sizeof(nm), "light%d_spt", nLit);
        PushU(vs, nm, Lg.spotDir[0], Lg.spotDir[1], Lg.spotDir[2], Lg.spotExponent);
        snprintf(nm, sizeof(nm), "light%d_par", nLit);
        PushU(vs, nm, Lg.spotCutoff, Lg.attConst, Lg.attLinear, Lg.attQuad);
        snprintf(nm, sizeof(nm), "light%d_en", nLit);
        PushU(vs, nm, 1.0f, 0.0f, 0.0f, 0.0f);
        ++nLit;
    }
    // Lights KHÔNG bật: vẫn chiếm 7 slot (giữ layout cố định).
    for (int i = nLit; i < kMaxLights; ++i) {
        for (int k = 0; k < 7; ++k) {
            char nm[24];
            snprintf(nm, sizeof(nm), "light%d_%d", i, k);
            PushU(vs, nm, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }
    PushU(vs, "matF_amb", L.materialFront.ambient, 4);
    PushU(vs, "matF_dif", L.materialFront.diffuse, 4);
    PushU(vs, "matF_spc", L.materialFront.specular, 4);
    PushU(vs, "matF_emi", L.materialFront.emission, 4);
    PushU(vs, "matF_shin", L.materialFront.shininess, 0.0f, 0.0f, 0.0f);
    PushU(vs, "matB_amb", L.materialBack.ambient, 4);
    PushU(vs, "matB_dif", L.materialBack.diffuse, 4);
    PushU(vs, "matB_spc", L.materialBack.specular, 4);
    PushU(vs, "matB_emi", L.materialBack.emission, 4);
    PushU(vs, "matB_shin", L.materialBack.shininess, 0.0f, 0.0f, 0.0f);
    PushU(vs, "sceneAmb", L.lightModel.ambient, 4);
    PushU(vs, "flags", L.lightModel.localViewer ? 1.0f : 0.0f,
          L.lightModel.twoSide ? 1.0f : 0.0f,
          (L.lightModel.colorControl == kSeparateSpecular) ? 1.0f : 0.0f,
          (float)nLit);
    PushU(vs, "flags2", c.state.IsEnabled(kNormalize) ? 1.0f : 0.0f,
          c.state.IsEnabled(kRescaleNormal) ? 1.0f : 0.0f, 0.0f, 0.0f);

    // ---- FS ----
    std::vector<USlot> fs;
    fs.reserve(16);
    float fogMode = 2.0f;  // EXP2 (fallback khi mode lạ)
    if (L.fog.mode == kFogLinear) fogMode = 0.0f;
    else if (L.fog.mode == kFogExp) fogMode = 1.0f;
    else if (L.fog.mode == kFogExp2) fogMode = 2.0f;
    PushU(fs, "fogParams", fogMode, L.fog.density, L.fog.start, L.fog.end);
    PushU(fs, "fogColor", L.fog.color, 4);
    PushU(fs, "fogCoord", L.fog.currentCoord,
          (L.fog.coordSrc == kFogCoord) ? 1.0f : 0.0f, 0.0f, 0.0f);
    GLuint afunc = 0x0207;  // GL_ALWAYS
    GLfloat aref = 0.0f;
    c.state.GetShadow(kAlphaFunc, &afunc, sizeof(afunc));
    c.state.GetShadow(kAlphaRef, &aref, sizeof(aref));
    PushU(fs, "alphaTest", (float)MapAlphaFunc(afunc), aref, 0.0f, 0.0f);
    for (int u = 0; u < kLegacyTexUnits; ++u) {
        char nm[16];
        snprintf(nm, sizeof(nm), "env%d", u);
        PushU(fs, nm, L.texEnv[u].color, 4);
    }
    for (int u = 0; u < kLegacyTexUnits; ++u) {
        char nm[16];
        const LegacyTexEnv& e = L.texEnv[u];
        snprintf(nm, sizeof(nm), "combA%d", u);
        PushU(fs, nm, (float)MapSource(e.src0RGB, u, 0), (float)MapSource(e.src1RGB, u, 1),
              (float)MapSource(e.src2RGB, u, 2), (float)MapCombineFunc(e.combineRGB));
        snprintf(nm, sizeof(nm), "combB%d", u);
        PushU(fs, nm, (float)MapSource(e.src0Alpha, u, 0),
              (float)MapSource(e.src1Alpha, u, 1), (float)MapSource(e.src2Alpha, u, 2),
              (float)MapCombineFunc(e.combineAlpha));
        snprintf(nm, sizeof(nm), "combC%d", u);
        PushU(fs, nm, (float)MapOperand(e.operand0RGB), (float)MapOperand(e.operand1RGB),
              (float)MapOperand(e.operand2RGB),
              (e.rgbScale == 2u) ? 2.0f : ((e.rgbScale == 4u) ? 4.0f : 1.0f));
        snprintf(nm, sizeof(nm), "combD%d", u);
        PushU(fs, nm, (float)MapOperand(e.operand0Alpha), (float)MapOperand(e.operand1Alpha),
              (float)MapOperand(e.operand2Alpha),
              (e.alphaScale == 2u) ? 2.0f : ((e.alphaScale == 4u) ? 4.0f : 1.0f));
    }

    InstallUniforms(*pr, vs, fs);
}

bool LegacyBuildGeometry(Context& c) {
    const LegacyState& L = c.legacy;
    const std::vector<LegacyVertex>& verts = L.immediateVertices;
    const std::vector<LegacyPrimitive>& prims = L.immediatePrimitives;
    if (verts.empty() || prims.empty()) return false;

    const GLMatrix& mv = L.modelview.top();
    const GLMatrix& tm = L.texture.top();
    float nrows[9];
    NormalMatrixRows(mv, nrows);
    const int numUnits = LegacyComputeShape(c).numTexUnits;
    (void)numUnits;  // số unit có texture: chỉ dùng ở fragment stage

    // --- Vertex data (layout cố định 6 × float4) ---
    std::vector<uint8_t> vdata(verts.size() * LegacyVertexLayout::kStride, 0);
    for (size_t i = 0; i < verts.size(); ++i) {
        const LegacyVertex& v = verts[i];
        float* out = (float*)(vdata.data() + i * LegacyVertexLayout::kStride);
        for (int k = 0; k < 4; ++k) out[0 + k] = v.pos[k];
        out[4] = v.normal[0]; out[5] = v.normal[1]; out[6] = v.normal[2]; out[7] = 0.0f;
        for (int k = 0; k < 4; ++k) out[8 + k] = v.color[k];
        // Toạ độ texture: texgen (nếu bật) rồi * texture matrix.
        for (int u = 0; u < 2; ++u) {
            const float* cur = (u == 0) ? v.texCoord
                                        : (u == 1 ? v.texCoord1 : L.currentTexCoord[u & 7]);
            float tc[4] = {cur[0], cur[1], cur[2], cur[3]};
            {
                // TexGen áp dụng cho MỌI unit (kể cả khi chưa bind texture:
                // texcoord là thuộc tính đỉnh, không phụ thuộc texture).
                const LegacyTexGen& tg = L.texGen[u];
                // Eye-space position + normal (cho sphere/reflection/normal map).
                float eye[4];
                mv.Transform4(v.pos[0], v.pos[1], v.pos[2], v.pos[3], eye);
                float ne[3] = {0, 0, 0};
                for (int r = 0; r < 3; ++r)
                    ne[r] = nrows[r * 3 + 0] * v.normal[0] + nrows[r * 3 + 1] * v.normal[1] +
                            nrows[r * 3 + 2] * v.normal[2];
                float nl = std::sqrt(ne[0] * ne[0] + ne[1] * ne[1] + ne[2] * ne[2]);
                float nf[3] = {ne[0], ne[1], ne[2]};
                if (nl > 0.0f) {
                    nf[0] /= nl; nf[1] /= nl; nf[2] /= nl;
                }
                float el = std::sqrt(eye[0] * eye[0] + eye[1] * eye[1] + eye[2] * eye[2]);
                float uu[3] = {0, 0, 0};
                if (el > 0.0f) {
                    uu[0] = eye[0] / el; uu[1] = eye[1] / el; uu[2] = eye[2] / el;
                }
                float refl[3] = {0, 0, 0};
                float dn = nf[0] * uu[0] + nf[1] * uu[1] + nf[2] * uu[2];
                for (int r = 0; r < 3; ++r) refl[r] = uu[r] - 2.0f * dn * nf[r];
                for (int ci = 0; ci < 4; ++ci) {
                    if (!c.state.IsEnabled(kTexGenS + ci)) continue;
                    const LegacyTexGenCoord& g = tg.coord[ci];
                    float gv = 0.0f;
                    switch (g.mode) {
                        case kObjectLinear:
                            gv = g.plane[0] * v.pos[0] + g.plane[1] * v.pos[1] +
                                 g.plane[2] * v.pos[2] + g.plane[3] * v.pos[3];
                            break;
                        case kEyeLinear:
                            gv = g.plane[0] * eye[0] + g.plane[1] * eye[1] +
                                 g.plane[2] * eye[2] + g.plane[3] * eye[3];
                            break;
                        case kSphereMap: {
                            float m = 2.0f / std::sqrt(refl[0] * refl[0] + refl[1] * refl[1] +
                                                       (refl[2] + 1.0f) * (refl[2] + 1.0f));
                            gv = (ci == 0) ? refl[0] / m + 0.5f
                                           : (ci == 1 ? refl[1] / m + 0.5f : 0.0f);
                            break;
                        }
                        case kReflectionMap:
                            gv = (ci == 0) ? refl[0] : (ci == 1 ? refl[1] : refl[2]);
                            break;
                        case kNormalMap:
                            gv = (ci == 0) ? nf[0] : (ci == 1 ? nf[1] : nf[2]);
                            break;
                        default:
                            continue;  // GL_NONE ⇒ giữ current texture coordinate
                    }
                    tc[ci] = gv;
                }
            }
            // Texture matrix (spec GL 2.1 §2.11.2): [s t r q] × M_texture.
            float s0 = tc[0], t0 = tc[1], r0 = tc[2], q0 = tc[3];
            float s1 = s0 * tm.m[0] + t0 * tm.m[4] + r0 * tm.m[8] + q0 * tm.m[12];
            float t1 = s0 * tm.m[1] + t0 * tm.m[5] + r0 * tm.m[9] + q0 * tm.m[13];
            float r1 = s0 * tm.m[2] + t0 * tm.m[6] + r0 * tm.m[10] + q0 * tm.m[14];
            float q1 = s0 * tm.m[3] + t0 * tm.m[7] + r0 * tm.m[11] + q0 * tm.m[15];
            out[12 + u * 4 + 0] = s1;
            out[12 + u * 4 + 1] = t1;
            out[12 + u * 4 + 2] = r1;
            out[12 + u * 4 + 3] = q1;
        }
        out[20] = v.secondaryColor[0];
        out[21] = v.secondaryColor[1];
        out[22] = v.secondaryColor[2];
        out[23] = 1.0f;
    }

    // --- Index data ---
    std::vector<uint32_t> idx;
    idx.reserve(prims.size() * 3);
    for (const auto& p : prims)
        for (uint32_t k = 0; k < p.count; ++k) idx.push_back(p.idx[k]);

    // --- Upload vertex buffer (ring allocator: 0 MTLBuffer alloc/frame) ---
    auto [rb, voff] = c.RingAlloc(vdata.size(), 256);
    if (rb) {
        memcpy((uint8_t*)rb->contents() + voff, vdata.data(), vdata.size());
        rb->didModifyRange(voff, vdata.size());
        c.legacy.vertexBuffer.reset();
        for (size_t i = 0; i < Context::kRingFrames; ++i)
            if (c.ringBuf[i] && c.ringBuf[i].get() == rb) {
                c.legacy.vertexBuffer = c.ringBuf[i];
                break;
            }
        if (!c.legacy.vertexBuffer) c.legacy.vertexBuffer = c.ringBuf[0];
        c.legacy.vertexOffset = voff;
    } else {
        ++c.appleStats.tempAllocs;
        c.legacy.vertexBuffer = c.device->newBufferWithBytes(vdata.data(), vdata.size(),
                                                             metal::StorageMode::Shared);
        c.legacy.vertexOffset = 0;
        if (!c.legacy.vertexBuffer) return false;
        c.pendingKeep.push_back(c.legacy.vertexBuffer);
    }
    c.legacy.vertexCount = (uint32_t)verts.size();
    c.legacy.indexData = idx;
    c.legacy.indexCount = (uint32_t)idx.size();
    c.legacy.primitiveMode = prims[0].mode;
    return true;
}

bool LegacyDrawImmediate(Context& c) {
    if (c.legacy.inBeginEnd) return false;             // đang trong glBegin..glEnd
    // GL 2.1 §2.15 / GL 4.6 Core §1.2.4: "if a program object is in use, the
    // programmable pipeline runs" ⇒ khi có program bound, đường fixed-function
    // KHÔNG được dùng (đỉnh immediate-mode lẽ ra phải được đưa vào program
    // của app — chưa hỗ trợ, xem docs/LEGACY_DRAW_PATH.md).
    if (c.state.BoundProgram() != 0) return false;
    if (c.legacy.immediatePrimitives.empty()) return false;
    // Batch đã vẽ rồi (glBegin lần sau sẽ xoá batch) ⇒ không vẽ 2 lần.
    if (c.legacy.drawnPrims != 0 &&
        c.legacy.drawnPrims == c.legacy.immediatePrimitives.size() &&
        c.legacy.drawnVerts == c.legacy.immediateVertices.size())
        return false;
    // Selection / Feedback (spec GL 4.2C §5.2 p.457, §5.3 p.460): ở chế độ
    // GL_SELECT / GL_FEEDBACK, GL KHÔNG rasterize — nó ghi hit-record hoặc
    // khối feedback vào buffer người dùng.
    if (LegacySelectionFeedbackProcessImmediate(c)) {
        c.legacy.drawnPrims = (uint32_t)c.legacy.immediatePrimitives.size();
        c.legacy.drawnVerts = (uint32_t)c.legacy.immediateVertices.size();
        return true;
    }
    if (!LegacyEnsureProgram(c)) return false;
    if (!LegacyBuildGeometry(c)) return false;

    // Số primitive theo NGỮ NGHĨA GL (1 quad = 1 primitive, không phải 2 tam
    // giác) — cho ARB_pipeline_statistics_query.
    uint64_t glPrims = 0;
    size_t nv = c.legacy.immediateVertices.size();
    switch (c.legacy.beginMode) {
        case 0x0000: glPrims = nv; break;                       // POINTS
        case 0x0001: glPrims = nv / 2; break;                   // LINES
        case 0x0002: glPrims = (nv >= 2) ? nv : 0; break;       // LINE_LOOP
        case 0x0003: glPrims = (nv >= 2) ? nv - 1 : 0; break;   // LINE_STRIP
        case 0x0004: glPrims = nv / 3; break;                   // TRIANGLES
        case 0x0005: case 0x0006: glPrims = (nv >= 3) ? nv - 2 : 0; break;
        case 0x0007: glPrims = nv / 4; break;                   // QUADS
        case 0x0008: glPrims = (nv >= 4) ? (nv / 2 - 1) : 0; break;  // QUAD_STRIP
        case 0x0009: glPrims = (nv >= 3) ? 1 : 0; break;        // POLYGON
        default: glPrims = c.legacy.immediatePrimitives.size(); break;
    }
    c.pipeStats.vertsSubmitted += c.legacy.vertexCount;
    c.pipeStats.primsSubmitted += glPrims;
    c.pipeStats.vsInvocations += c.legacy.vertexCount;

    metal::PrimitiveType prim = metal::PrimitiveType::Triangle;
    if (c.legacy.primitiveMode == 0x0000) prim = metal::PrimitiveType::Point;
    else if (c.legacy.primitiveMode == 0x0001) prim = metal::PrimitiveType::Line;

    // Trace (backend Null — không có GPU): EmitDraw cũng ghi trace khi Null.
    if (c.device->isNull()) {
        auto enc = c.device->makeEncoder();
        if (enc) {
            enc->drawIndexed(prim, c.legacy.indexCount, metal::IndexType::UInt32, nullptr,
                             0, 1);
            enc->endEncoding();
        }
    }

    // GPU thật: đi qua ĐƯỜNG DRAW CHUNG (AppleDrawGL) — cùng encoder reuse,
    // pipeline cache, uniform staging, viewport/depth/blend encode.
    c.legacy.drawActive = true;
    AppleDrawGL(c.legacy.primitiveMode, (GLsizei)c.legacy.indexCount, kUnsignedInt,
                c.legacy.indexData.empty() ? nullptr : c.legacy.indexData.data(), true, 0, 0,
                1, 0, 0, 0);
    c.legacy.drawActive = false;
    c.legacy.drawnPrims = (uint32_t)c.legacy.immediatePrimitives.size();
    c.legacy.drawnVerts = (uint32_t)c.legacy.immediateVertices.size();
    return true;
}

bool LegacyFlushPendingImmediate(Context& c) {
    if (!LegacyFixedFunctionAllowed(c)) return false;
    if (c.state.BoundProgram() != 0) return false;
    if (c.legacy.immediatePrimitives.empty()) return false;
    return LegacyDrawImmediate(c);
}

}  // namespace tglmt::gl
