#pragma once
// LegacyState — state của OpenGL fixed-function (Compatibility Profile).
//
// TGLMT hỗ trợ Desktop OpenGL 1.0–4.6 gồm cả Compatibility Profile. Phần
// fixed-function (matrix stack, current vertex attributes, lighting, fog,
// texture env, display lists...) KHÔNG tồn tại trong Core Profile nên được
// tách riêng: Context::legacy là nơi duy nhất chứa nó, để đường modern
// programmable pipeline không bị ảnh hưởng.
//
// Tham chiếu spec: OpenGL 1.x/2.1 fixed-function chapter (đã kiểm tra cùng
// GL 4.6 Core spec App. E.2 — các phần này được giữ lại trong
// Compatibility Profile).
#include "tglmt/MetalInterface.h"
#include "tglmt/StateTracker.h"
#include "tglmt/gl46_types.h"
#include "tglmt/gl_legacy_types.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace tglmt {

// Chương trình fixed-function (Compatibility Profile) — định nghĩa đầy đủ
// trong Context.h. Ở đây chỉ khai báo trước để LegacyState giữ con trỏ tới
// bản sao do Context sở hữu (xem Context::legacyProgram).
struct ProgramObject;

// Ma trận 4x4 column-major đúng layout OpenGL: m[0..3] = cột 0,
// m[4..7] = cột 1, m[8..11] = cột 2, m[12..15] = cột 3.
struct GLMatrix {
    float m[16];

    GLMatrix() { LoadIdentity(); }

    void LoadIdentity() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    // Nạp 16 số column-major (thứ tự glLoadMatrix).
    void Load(const float* src) {
        for (int i = 0; i < 16; ++i) m[i] = src[i];
    }
    // Nạp 16 số ROW-major (glLoadTransposeMatrix) ⇒ phải transpose.
    void LoadTranspose(const float* src) {
        for (int r = 0; r < 4; ++r)
            for (int col = 0; col < 4; ++col) m[col * 4 + r] = src[r * 4 + col];
    }
    // this = this * other (column-major). other là bên phải ⇒ other biến đổi
    // trước (đúng thứ tự nhân của OpenGL).
    void Mult(const float* other) {
        float t[16];
        for (int c = 0; c < 4; ++c) {
            for (int r = 0; r < 4; ++r) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) s += m[k * 4 + r] * other[c * 4 + k];
                t[c * 4 + r] = s;
            }
        }
        for (int i = 0; i < 16; ++i) m[i] = t[i];
    }
    void MultTranspose(const float* src) {
        float tr[16];
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) tr[c * 4 + r] = src[r * 4 + c];
        Mult(tr);
    }
    void Translate(float x, float y, float z) {
        float t[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
        Mult(t);
    }
    void Scale(float x, float y, float z) {
        float t[16] = {x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1};
        Mult(t);
    }
    // Xoay quanh trục (x,y,z) theo góc độ (điểm trục không chuẩn hoá).
    void Rotate(float angleDeg, float x, float y, float z);
    void Ortho(double l, double r, double b, double t, double n, double f);
    void Frustum(double l, double r, double b, double t, double n, double f);
    // this * v (w=1 cho điểm, w=0 cho hướng).
    void Transform4(float x, float y, float z, float w, float out[4]) const {
        for (int r = 0; r < 4; ++r) {
            out[r] = m[0 * 4 + r] * x + m[1 * 4 + r] * y + m[2 * 4 + r] * z + m[3 * 4 + r] * w;
        }
    }
};

// Ngăn xếp ma trận. Độ sâu tối thiểu theo spec OpenGL 2.1+ / GL 3.x
// Compatibility là 32 cho modelview/projection/texture. Ta dùng 32.
constexpr size_t kMatrixStackDepth = 32;
// Spec GL 4.2C §6.2 (PDF p.589): MAX NAME STACK DEPTH "must be at least 64".
constexpr size_t kNameStackDepth = 64;

struct MatrixStack {
    std::vector<GLMatrix> stack;  // stack[0] = phần tử đang dùng
    GLMatrix& top() { return stack.back(); }
    const GLMatrix& top() const { return stack.back(); }
    void Reset() {
        stack.clear();
        stack.reserve(kMatrixStackDepth + 1);
        stack.emplace_back();  // identity
    }
    size_t depth() const { return stack.size(); }
    // Đẩy bản sao của phần tử đang dùng. Trả false nếu đầy (⇒ GL_STACK_OVERFLOW).
    bool Push() {
        if (stack.size() >= kMatrixStackDepth) return false;
        stack.push_back(stack.back());
        return true;
    }
    // Pop. Trả false nếu chỉ còn 1 phần tử (⇒ GL_STACK_UNDERFLOW).
    bool Pop() {
        if (stack.size() <= 1) return false;
        stack.pop_back();
        return true;
    }
};

// Một đỉnh trong immediate mode (GL 1.x §2.6 / §5).
struct LegacyVertex {
    float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float normal[3] = {0.0f, 0.0f, 1.0f};
    float texCoord[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float texCoord1[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float secondaryColor[3] = {0.0f, 0.0f, 0.0f};
    GLuint index = 0;        // color index (glIndex)
    GLboolean edgeFlag = 1;  // cờ cạnh (glEdgeFlag)
};

// Một primitive đã ghép từ immediate mode. Lưu chỉ số đỉnh (không phải
// (first,count)) vì nhiều primitive không liên tiếp (fan, quad strip...).
struct LegacyPrimitive {
    GLenum mode = 0;          // GL_POINTS / GL_LINES / GL_TRIANGLES (đã triangulate)
    uint32_t idx[4] = {0, 0, 0, 0};  // chỉ số đỉnh trong immediateVertices
    uint32_t count = 0;       // số đỉnh dùng (1, 2 hoặc 3)
};

// ---------------------------------------------------------------------------
// LIGHTING & MATERIALS (OpenGL 2.1 spec §2.14, giữ lại trong Compat Profile)
// ---------------------------------------------------------------------------
// Số nguồn sáng tối thiểu theo spec §2.14.4: "at least 8 sets of light
// parameters". Ta dùng 8 (khớp GL_MAX_LIGHTS = 8).
constexpr int kMaxLights = 8;

// Một nguồn sáng. Giá trị mặc định theo spec Table 2.10 (GL 2.1 §2.14.1):
//   acli = (0,0,0,1); dcli(i=0) = (1,1,1,1), dcli(i>0) = (0,0,0,1);
//   scli(i=0) = (1,1,1,1), scli(i>0) = (0,0,0,1);
//   Ppli = (0,0,1,0); sdli = (0,0,-1); srli = 0; crli = 180;
//   k0 = 1; k1 = 0; k2 = 0.
// LƯU Ý: "the default values for dcli and scli differ for i = 0 and i > 0"
// (spec GL 2.1 §2.14.1, PDF p.60) — nên khởi tạo riêng cho light 0.
struct LegacyLight {
    float ambient[4] = {0.0f, 0.0f, 0.0f, 1.0f};   // acli
    float diffuse[4] = {0.0f, 0.0f, 0.0f, 1.0f};   // dcli
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // scli
    // Ppli: đã transform bằng modelview HIỆN TẠI tại lúc glLight được gọi
    // (spec §2.14.2: "The current model-view matrix is applied to the position
    // parameter ... when that position is specified"). w = 0 ⇒ nguồn sáng
    // ở vô cùng (directional).
    float position[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    // sdli: đã transform bằng 3x3 trên-trái của modelview tại lúc gọi
    // (spec §2.14.2). KHÔNG chuẩn hoá.
    float spotDir[3] = {0.0f, 0.0f, -1.0f};
    float spotExponent = 0.0f;   // srli, range [0,128]
    float spotCutoff = 180.0f;   // crli, range [0,90] ∪ {180}
    float attConst = 1.0f;       // k0
    float attLinear = 0.0f;      // k1
    float attQuad = 0.0f;        // k2
    bool enabled = false;        // glEnable(GL_LIGHTi)

    void ResetDefaults(int index) {
        ambient[0] = ambient[1] = ambient[2] = 0.0f; ambient[3] = 1.0f;
        // Light 0 có diffuse/specular trắng; light i>0 có đen (spec Table 2.10).
        const float d = (index == 0) ? 1.0f : 0.0f;
        diffuse[0] = diffuse[1] = diffuse[2] = d; diffuse[3] = 1.0f;
        specular[0] = specular[1] = specular[2] = d; specular[3] = 1.0f;
        position[0] = position[1] = 0.0f; position[2] = 1.0f; position[3] = 0.0f;
        spotDir[0] = spotDir[1] = 0.0f; spotDir[2] = -1.0f;
        spotExponent = 0.0f;
        spotCutoff = 180.0f;
        attConst = 1.0f; attLinear = 0.0f; attQuad = 0.0f;
        enabled = false;
    }
};

// Material của một mặt. Mặc định theo spec Table 2.10 (GL 2.1 §2.14.1):
//   acm = (0.2,0.2,0.2,1); dcm = (0.8,0.8,0.8,1);
//   scm = (0,0,0,1);     ecm = (0,0,0,1); srm = 0.
struct LegacyMaterial {
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};   // acm
    float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};   // dcm
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // scm
    float emission[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // ecm
    float shininess = 0.0f;                        // srm, range [0,128]
    // Color index material (am, dm, sm) — chỉ dùng ở color-index mode.
    float colorIndexes[3] = {0.0f, 1.0f, 1.0f};
};

// Lighting model parameters (spec Table 2.10).
struct LegacyLightModel {
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};  // acs (scene ambient)
    bool localViewer = false;      // vbs — MẶC ĐỊNH FALSE (spec Table 2.10)
    bool twoSide = false;          // tbs
    GLenum colorControl = 0x81F9;  // ces: GL_SINGLE_COLOR / GL_SEPARATE_SPECULAR_COLOR
};

// glColorMaterial: face + mode.
struct LegacyColorMaterial {
    GLenum face = 0x0408;  // GL_FRONT_AND_BACK (spec §2.14.4: mặc định FRONT_AND_BACK)
    GLenum mode = 0x1602;   // GL_AMBIENT_AND_DIFFUSE (spec §2.14.4)
};

// ---------------------------------------------------------------------------
// FOG (OpenGL 2.1 spec §3.10)
// ---------------------------------------------------------------------------
struct LegacyFog {
    GLenum mode = 0x0800;         // GL_EXP — mặc định theo spec
    float density = 1.0f;         // GL_FOG_DENSITY
    float start = 0.0f;           // GL_FOG_START
    float end = 1.0f;             // GL_FOG_END
    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // GL_FOG_COLOR
    GLenum coordSrc = 0x8452;     // GL_FOG_COORDINATE_SOURCE: GL_FRAGMENT_DEPTH
    GLenum hint = 0x1101;         // GL_FOG_HINT: GL_NICEST
    // Toạ độ fog hiện tại (glFogCoord*) — chỉ dùng khi coordSrc == GL_FOG_COORD.
    float currentCoord = 0.0f;
};

// ---------------------------------------------------------------------------
// TEXTURE ENVIRONMENT (OpenGL 2.1 spec §8.x, GL_COMBINE từ 1.3)
// ---------------------------------------------------------------------------
// Tham số GL_COMBINE. Mỗi unit có 1 bộ.
// Mặc định theo spec GL 1.3 §3.8.12 (PDF p.147): "the texture and combiner
// functions are each MODULATE, the combiner RGB and ALPHA sources are each
// TEXTURE, PREVIOUS, and CONSTANT for sources 0, 1, and 2 respectively, the
// combiner RGB operands for sources 0 and 1 are each SRC_COLOR, the combiner
// RGB operand for source 2, as well as for the combiner ALPHA operands, are
// each SRC_ALPHA, and the environment color is (0, 0, 0, 0)."
struct LegacyTexEnv {
    GLenum mode = 0x2100;   // GL_MODULATE — mặc định
    float color[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // GL_TEXTURE_ENV_COLOR
    // --- GL_COMBINE (GL 1.3+) ---
    GLenum combineRGB = 0x2100;    // GL_COMBINE (pname) — giá trị mặc định MODULATE
    GLenum combineAlpha = 0x2100;  // GL_COMBINE_ALPHA — mặc định MODULATE
    // Nguồn (SOURCE): GL_TEXTURE / GL_PREVIOUS / GL_CONSTANT cho slot 0/1/2.
    GLenum src0RGB = 0x1702;       // GL_TEXTURE
    GLenum src1RGB = 0x8578;       // GL_PREVIOUS
    GLenum src2RGB = 0x8576;       // GL_CONSTANT
    GLenum src0Alpha = 0x1702;     // GL_TEXTURE
    GLenum src1Alpha = 0x8578;     // GL_PREVIOUS
    GLenum src2Alpha = 0x8576;     // GL_CONSTANT
    // Operand: SRC_COLOR cho RGB operand 0/1, SRC_ALPHA cho RGB operand 2 và
    // cho mọi operand ALPHA.
    GLenum operand0RGB = 0x0300;   // GL_SRC_COLOR
    GLenum operand1RGB = 0x0300;   // GL_SRC_COLOR
    GLenum operand2RGB = 0x0302;   // GL_SRC_ALPHA
    GLenum operand0Alpha = 0x0302; // GL_SRC_ALPHA
    GLenum operand1Alpha = 0x0302; // GL_SRC_ALPHA
    GLenum operand2Alpha = 0x0302; // GL_SRC_ALPHA
    GLenum rgbScale = 1;           // GL_RGB_SCALE (1/2/4)
    GLenum alphaScale = 1;         // GL_ALPHA_SCALE
};

// ---------------------------------------------------------------------------
// TEXTURE GENERATION (OpenGL 2.1 spec §8.x)
// ---------------------------------------------------------------------------
// Mỗi texture unit có 4 tọa độ sinh: S, T, R, Q.
// Mặc định theo spec GL 1.3 §2.11 (PDF p.39): mode = EYE_LINEAR cho mọi
// coord; p của S = (1,0,0,0), của T = (0,1,0,0), của R/Q = (0,0,0,0).
// (GL_EYE_LINEAR = 0x2400; GL_OBJECT_LINEAR = 0x2401 — xem docs/khronos/gl.xml)
struct LegacyTexGenCoord {
    GLenum mode = 0x2400;   // GL_EYE_LINEAR
    float plane[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // mặt phẳng (OBJECT_PLANE/EYE_PLANE)
};

struct LegacyTexGen {
    LegacyTexGenCoord coord[4];  // 0=S, 1=T, 2=R, 3=Q
};

// Toàn bộ state fixed-function của một context.
struct LegacyState {
    // --- Ma trận ---
    GLenum matrixMode = 0x1700;  // GL_MODELVIEW
    MatrixStack modelview, projection, texture, color;

    // --- Current vertex attributes (GL 1.x §2.6 / §4.1) ---
    float currentColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float currentNormal[3] = {0.0f, 0.0f, 1.0f};
    float currentSecondaryColor[3] = {0.0f, 0.0f, 0.0f};
    // Tọa độ texture hiện tại cho từng texture unit (GL 1.3 multi-texture).
    float currentTexCoord[8][4] = {};
    // Chỉ số texture unit hiện tại (GL_TEXTURE_i, core từ 1.3).
    GLuint activeTextureUnit = 0;

    // --- Immediate mode ---
    bool inBeginEnd = false;      // đang trong glBegin/glEnd
    GLenum beginMode = 0;         // primitive mode của glBegin hiện tại
    GLboolean edgeFlag = 1;       // cờ cạnh (glEdgeFlag)
    GLuint currentIndex = 0;      // color index hiện tại (glIndex)

    // Đỉnh gom được trong glBegin..glEnd hiện tại.
    std::vector<LegacyVertex> immediateVertices;
    // Primitive đã ghép (sẽ đưa vào Metal draw path).
    std::vector<LegacyPrimitive> immediatePrimitives;

    // --- Lighting & materials (GL 2.1 §2.14) ---
    bool lightingEnabled = false;       // glEnable(GL_LIGHTING)
    bool colorMaterialEnabled = false;  // glEnable(GL_COLOR_MATERIAL)
    LegacyLightModel lightModel;
    LegacyColorMaterial colorMaterial;
    LegacyMaterial materialFront, materialBack;
    LegacyLight lights[kMaxLights];

    // --- Shade model (GL 2.1 §2.14.7) ---
    // GL_SMOOTH = 0x1D01 (giá trị từ registry, KHÔNG phải 0x8DDA — 0x8DDA là
    // hằng kiểu OpenGL ES, xem docs/khronos/gl.xml).
    GLenum shadeModel = 0x1D01;  // GL_SMOOTH — mặc định

    // --- Fog (GL 2.1 §3.10) ---
    LegacyFog fog;

    // --- Texture environment & generation, theo từng texture unit ---
    LegacyTexEnv texEnv[8];
    LegacyTexGen texGen[8];

    // --- Đường draw fixed-function (Compatibility Profile) ---
    // Chương trình SINH (VS+FS) của fixed-function. NGOÀI Context::programs:
    // nó KHÔNG phải GL program object ⇒ không xuất hiện trong glGetProgramiv,
    // glUseProgram không chọn được (docs/LEGACY_TO_METAL_MAPPING.md §2.5).
    // Con trỏ trỏ vào Context::legacyProgram (Context sở hữu bản sao).
    ProgramObject* program = nullptr;
    // "Hình dạng" chương trình hiện tại (lighting/fog/texenv/...) — đổi hình
    // dạng ⇒ sinh lại chương trình (linkGen tăng ⇒ pipeline mới).
    uint64_t shapeKey = 0;
    uint64_t linkGen = 0;
    bool programValid = false;
    // Buffer đỉnh + index của batch immediate-mode đang chờ vẽ.
    std::shared_ptr<metal::IBuffer> vertexBuffer;
    size_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    std::vector<uint32_t> indexData;
    uint32_t indexCount = 0;
    GLenum primitiveMode = 0;
    // true khi đang trong draw fixed-function (chống đệ quy khi draw legacy
    // gọi lại AppleDrawGL).
    bool drawActive = false;
    // Số primitive/vertex của batch ĐÃ vẽ (tránh vẽ 2 lần cùng batch).
    uint32_t drawnPrims = 0;
    uint32_t drawnVerts = 0;

    // =======================================================================
    // STATE CÒN THIẾU — định nghĩa trước để các nhóm implement dùng chung
    // một layout. Xem docs/LEGACY_PIPELINE_AUDIT.md cho mô tả từng nhóm.
    // =======================================================================

    // --- Clip plane (GL 1.0 §2.12.3) --------------------------------------
    // 6 mặt phẳng user; phương trình đã transform sang EYE SPACE tại lúc
    // gọi glClipPlane (spec: "these are the coefficients of the plane equation
    // of plane in eye coordinates (these coordinates are those that were
    // computed when the plane was specified)").
    struct LegacyClipPlane {
        float eq[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // eye-space
        bool enabled = false;
    };
    LegacyClipPlane clipPlanes[6];

    // --- Raster position (GL 1.0 §2.13) ------------------------------------
    // Vị trí hiện tại trong window + các giá trị raster liên quan.
    struct LegacyRasterPos {
        float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;  // window coords
        bool valid = false;      // GL_CURRENT_RASTER_POSITION_VALID
        float color[4] = {0, 0, 0, 0};       // current raster color
        float depth = 0.0f;                   // current raster depth
        GLuint index = 0;                     // current raster index
        float texCoord[4] = {0, 0, 0, 1};    // current raster texture coords
        float fogCoord = 0.0f;               // current raster distance
    };
    LegacyRasterPos raster;

    // --- Stipple (GL 1.0 §3.5.2 / §3.4.2) --------------------------------
    struct LegacyStipple {
        uint8_t polygon[32] = {};   // 32 hàng × 8 bit
        GLushort linePattern = 0xFFFF;
        GLint lineRepeat = 1;
    };
    LegacyStipple stipple;

    // --- Point rasterization (GL 1.4 §3.4.1) ------------------------------
    struct LegacyPointParam {
        float sizeMin = 0.0f;      // GL_POINT_SIZE_MIN
        float sizeMax = 1.0f;      // GL_POINT_SIZE_MAX
        float distAttenuation[3] = {1.0f, 0.0f, 0.0f};
        float fadeThreshold = 1.0f;
        GLenum spriteCoordOrigin = 0x8CA3;  // GL_UPPER_LEFT
    };
    LegacyPointParam pointParam;

    // --- Color index mode (GL 1.1) -----------------------------------------
    struct LegacyColorIndex {
        GLuint indexMask = 0xFFFFFFFFu;  // GL_INDEX_WRITEMASK
        GLuint clearIndex = 0;           // GL_INDEX_CLEAR_VALUE
        GLuint currentIndex = 0;         // current color index
        bool indexMode = false;          // GL_INDEX_MODE
    };
    LegacyColorIndex colorIndex;

    // --- Accumulation buffer (GL 1.0 §4.1) ---------------------------------
    struct LegacyAccum {
        float clearValue = 0.0f;  // GL_ACCUM_CLEAR_VALUE
        GLenum op = 0x0000;       // GL_ACCUM
        float value = 0.0f;
    };
    LegacyAccum accum;

    // --- Client-side vertex arrays (GL 1.1 §2.8) --------------------------
    // 8 mảng client: COLOR, NORMAL, TEXTURE_COORD, INDEX, EDGE_FLAG,
    // SECONDARY_COLOR, FOG_COORD, (MULTI)TEXTURE_COORD.
    struct LegacyArrayPointer {
        bool enabled = false;
        GLint size = 4;
        GLenum type = 0x1406;  // GL_FLOAT
        GLsizei stride = 0;
        const void* pointer = nullptr;
        GLuint buffer = 0;  // buffer object đang bind (nếu có)
    };
    LegacyArrayPointer arrayPointers[8];
    GLuint clientActiveTexture = 0;  // GL_CLIENT_ACTIVE_TEXTURE
    GLuint arrayElement = 0;         // glArrayElement
    // True nếu pointer nào đang trỏ vào client memory (không phải buffer obj).
    bool arrayUsesClientMemory = false;

    // --- Pixel transfer & pixel maps (GL 1.0 §3.6 / §3.4) ----------------
    struct LegacyPixelTransfer {
        float redScale = 1.0f, redBias = 0.0f;
        float greenScale = 1.0f, greenBias = 0.0f;
        float blueScale = 1.0f, blueBias = 0.0f;
        float alphaScale = 1.0f, alphaBias = 0.0f;
        float depthScale = 1.0f, depthBias = 0.0f;
        GLuint indexScale = 1, indexOffset = 0;
        bool mapColor = false;
        bool postColorMatrix = false;
        bool postConvolution = false;
    };
    LegacyPixelTransfer pixelTransfer;
    struct LegacyPixelMaps {
        // GL_PIXEL_MAP_I_TO_I / S_TO_S (256 entry)
        std::vector<GLfloat> iToI, sToS;
        // I_TO_R/G/B/A, R_TO_R/G/B — 256 entry
        std::vector<GLfloat> iToR, iToG, iToB, iToA;
        std::vector<GLfloat> rToR, rToG, rToB;
        LegacyPixelMaps() {
            for (int i = 0; i < 256; ++i) {
                iToI.push_back((float)i); sToS.push_back((float)i);
                iToR.push_back((float)i); iToG.push_back((float)i);
                iToB.push_back((float)i); iToA.push_back((float)i);
                rToR.push_back((float)i); rToG.push_back((float)i); rToB.push_back((float)i);
            }
        }
    };
    LegacyPixelMaps pixelMaps;

    // --- Evaluator (GL 1.0 §5.1) ------------------------------------------
    // glMap1: 1 chiều. glMap2: 2 chiều (lưới u×v).
    struct LegacyMap1 {
        GLenum target = 0;         // GL_VERTEX_3/4, GL_COLOR_*, GL_TEXTURE_COORD_*
        GLint order = 1;           // số điểm điều khiển
        GLsizei stride = 0;        // 0 ⇒ dùng giá trị mặc định theo target
        std::vector<GLfloat> points;  // order × số thành phần
        int components = 0;        // số thành phần mỗi điểm
    };
    struct LegacyMap2 {
        GLenum target = 0;
        GLint uOrder = 1, vOrder = 1;
        GLsizei uStride = 0, vStride = 0;
        std::vector<GLfloat> points;  // uOrder × vOrder × components
        int components = 0;
    };
    LegacyMap1 map1[8];   // theo coord: 0=S 1=T 2=R 3=Q 4=VERTEX 5=COLOR 6=INDEX 7=NORMAL
    LegacyMap2 map2[8];

    // --- Display list (GL 1.0 §5.4) ---------------------------------------
    // Lệnh được ghi vào display list. Dùng opcode + payload cố định để
    // replay đúng thứ tự (spec: "the commands are executed in the same order
    // in which they were issued").
    struct LegacyListOp {
        GLenum op = 0;          // opcode (xem kLegacyListOp* trong gl_legacy_displaylist.cpp)
        GLuint u0 = 0, u1 = 0, u2 = 0;
        float f0 = 0.0f, f1 = 0.0f, f2 = 0.0f, f3 = 0.0f;
        float m[16] = {};       // payload ma trận (16 float)
        std::vector<float> data;  // payload biến thiên (đỉnh, màu, ...)
    };
    struct LegacyDisplayList {
        GLenum mode = 0;             // GL_COMPILE / GL_COMPILE_AND_EXECUTE
        bool valid = false;          // đã glEndList
        bool executing = false;      // đang replay (chống đệ quy)
        std::vector<LegacyListOp> ops;
    };
    std::unordered_map<GLuint, LegacyDisplayList> lists;
    GLuint listBase = 0;             // GL_LIST_BASE
    GLuint listRecording = 0;        // list đang ghi (0 = không ghi)

    // --- Selection & feedback (GL 1.0 §5.2 / §5.3) ------------------------
    struct LegacySelection {
        GLint* selectBuffer = nullptr;   // GL_SELECT_BUFFER
        GLsizei selectCapacity = 0;      // số phần tử buffer select
        GLfloat* feedbackBuffer = nullptr;  // GL_FEEDBACK_BUFFER
        GLsizei feedbackCapacity = 0;
        GLenum feedbackType = GL_2D;      // GL_FEEDBACK_BUFFER_TYPE (spec 4.2C §5.3)
        // GL_RENDER = 0x1C00 (GL_FEEDBACK = 0x1C01, GL_SELECT = 0x1C02) —
        // theo docs/khronos/gl.xml. Mặc định phải là GL_RENDER.
        GLenum renderMode = GL_RENDER;
        std::vector<GLuint> nameStack;    // name stack (spec: kNameStackDepth sâu tối đa)
        GLuint currentName = 0;
        // Số phần tử đã ghi vào buffer (selection/feedback) — để báo
        // INVALID_OPERATION khi tràn.
        GLsizei selectUsed = 0;
        GLsizei feedbackUsed = 0;
        LegacySelection() { nameStack.push_back(0); }
    };
    LegacySelection selection;

    // --- Attribute stack (GL 1.0 §2.10) -----------------------------------
    // glPushAttrib/glPopAttrib: lưu nhóm state. Giữ snapshot của nhóm
    // state liên quan (client attrib stack lưu riêng).
    struct LegacyAttribStackEntry {
        std::unordered_map<GLenum, bool> caps;
        std::unordered_map<GLuint, bool> capsI;
        BlendState blendSnapshot[kMaxDrawBuffers];
        DepthState depth;
        bool lightingEnabled = false;
        bool colorMaterialEnabled = false;
        LegacyLightModel lightModel;
        LegacyColorMaterial colorMaterial;
        LegacyMaterial materialFront, materialBack;
        LegacyLight lights[kMaxLights];
        GLenum shadeModel = 0x1D01;
        LegacyFog fog;
        LegacyTexEnv texEnv[8];
        LegacyTexGen texGen[8];
        LegacyClipPlane clipPlanes[6];
        LegacyStipple stipple;
        LegacyPointParam pointParam;
        LegacyColorIndex colorIndex;
        LegacyAccum accum;
        LegacyPixelTransfer pixelTransfer;
        GLenum matrixMode = 0x1700;
        GLMatrix modelview, projection, texture, color;
        float currentColor[4] = {1, 1, 1, 1};
        float currentNormal[3] = {0, 0, 1};
        float currentTexCoord[8][4] = {};
        GLuint currentIndex = 0;
        GLboolean edgeFlag = 1;
    };
    std::vector<LegacyAttribStackEntry> attribStack;
    // Client attribute stack (glPushClientAttrib/glPopClientAttrib).
    struct LegacyClientAttribStackEntry {
        LegacyArrayPointer arrayPointers[8];
        GLuint clientActiveTexture = 0;
    };
    std::vector<LegacyClientAttribStackEntry> clientAttribStack;

    LegacyState() {
        modelview.Reset();
        projection.Reset();
        texture.Reset();
        color.Reset();
        for (int i = 0; i < 8; ++i) currentTexCoord[i][3] = 1.0f;
        for (int i = 0; i < kMaxLights; ++i) lights[i].ResetDefaults(i);
        // TexGen mặc định (spec GL 1.3 §2.11 p.39): EYE_LINEAR cho mọi coord;
        // p của S = (1,0,0,0), của T = (0,1,0,0), của R/Q = (0,0,0,0).
        for (int u = 0; u < 8; ++u) {
            for (int i = 0; i < 4; ++i) {
                texGen[u].coord[i].mode = 0x2400;  // GL_EYE_LINEAR
                texGen[u].coord[i].plane[0] = texGen[u].coord[i].plane[1] =
                    texGen[u].coord[i].plane[2] = texGen[u].coord[i].plane[3] = 0.0f;
            }
            texGen[u].coord[0].plane[0] = 1.0f;  // S: p1 = 1
            texGen[u].coord[1].plane[1] = 1.0f;  // T: p2 = 1
        }
        // currentTexCoord mặc định = (0,0,0,1) (spec: q = 1).
        for (int u = 0; u < 8; ++u) currentTexCoord[u][3] = 1.0f;
        // Stipple mặc định: line pattern 0xFFFF, repeat 1 (spec GL 1.0 §3.4.2).
        stipple.linePattern = 0xFFFF;
        stipple.lineRepeat = 1;
        // Point size max mặc định = 1.0 (spec GL 1.4 §3.4.1).
        pointParam.sizeMax = 1.0f;
    }

    void ResetImmediate() {
        immediateVertices.clear();
        immediatePrimitives.clear();
        // Batch mới ⇒ cờ "đã vẽ" phải xoá (đường fixed-function dùng cờ này
        // để không phát lại cùng một batch).
        drawnPrims = 0;
        drawnVerts = 0;
    }
};

}  // namespace tglmt
