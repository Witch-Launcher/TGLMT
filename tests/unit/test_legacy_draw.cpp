// test_legacy_draw.cpp — ĐƯỜNG DRAW FIXED-FUNCTION (Compatibility Profile).
//
// Kiểm tra những gì CÓ THỂ kiểm tra được trên backend Null (không GPU):
//   1. Compatibility profile ⇒ fixed-function được phép; Core ⇒ không.
//   2. glBegin/glEnd ⇒ primitive đã ghép (khác với trước: giờ CÓ draw).
//   3. Chương trình fixed-function được SINH (MSL có đủ các stage) và nằm
//      NGOÀI Context::programs (không phải GL program object).
//   4. Uniform được đóng gói đúng (model-view, light, material, fog).
//   5. Vertex buffer + index buffer dựng đúng số lượng/giá trị (kể cả
//      texture matrix + texgen).
//   6. Draw được phát vào DrawTrace của backend Null.
//   7. Khi CÓ program object bound ⇒ đường fixed-function KHÔNG được dùng
//      (GL 2.1 §2.15: "if a program object is in use, the programmable
//      pipeline runs").
//
// KHÔNG kiểm tra được ở đây: kết quả RENDER thật trên Metal (lighting/fog/
// texenv đúng số hay không) — cần GPU, xem docs/LEGACY_DRAW_PATH.md.
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyDraw.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace tglmt;

namespace {

// --- enum cần thiết (đối chiếu docs/khronos/gl.xml) ---
constexpr GLenum kLighting = 0x0B50;
constexpr GLenum kColorMaterial = 0x0B57;
constexpr GLenum kFog = 0x0B60;
constexpr GLenum kLight0 = 0x4000;
constexpr GLenum kDiffuse = 0x1201;
constexpr GLenum kAmbient = 0x1200;
constexpr GLenum kPosition = 0x1203;
constexpr GLenum kMaterialDiffuse = 0x1201;
constexpr GLenum kTriangles = 0x0004;
constexpr GLenum kQuads = 0x0007;
constexpr GLenum kTexture2D = 0x0DE1;
constexpr GLenum kTextureEnvMode = 0x2200;
constexpr GLenum kTextureEnvColor = 0x2201;
constexpr GLenum kModulate = 0x2100;
constexpr GLenum kReplace = 0x1E01;
constexpr GLenum kCurrentProgram = 0x8B8D;
constexpr GLenum kAlphaTest = 0x0BC0;

bool Near(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) <= eps; }

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}

// Đọc 1 uniform (float4) của chương trình fixed-function theo TÊN.
bool FindUniform(const Context& c, const char* name, float out[4]) {
    const ProgramObject* pr = c.legacy.program;
    if (!pr) return false;
    for (const auto& e : pr->uniformLayout) {
        if (e.name != name) continue;
        auto it = pr->uniforms.find(e.loc);
        if (it == pr->uniforms.end()) return false;
        if (it->second.bytes.size() < 16) return false;
        memcpy(out, it->second.bytes.data(), 16);
        return true;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. Profile: Compatibility thì được, Core thì không.
// ---------------------------------------------------------------------------
static void TestProfileGate() {
    TGLMTContext* cc = CompatCtx();
    TGLMT_CHECK(cc != nullptr, "tạo được context Compatibility");
    if (cc) {
        tglmtMakeCurrent(cc);
        Context& c = Context::Current();
        TGLMT_CHECK(tglmt::gl::LegacyFixedFunctionAllowed(c),
                    "Compatibility profile ⇒ fixed-function được phép");
        tglmtDestroyContext(cc);
    }
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    TGLMTContext* core = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(core != nullptr, "tạo được context Core");
    if (core) {
        tglmtMakeCurrent(core);
        Context& c = Context::Current();
        TGLMT_CHECK(!tglmt::gl::LegacyFixedFunctionAllowed(c),
                    "Core profile ⇒ fixed-function KHÔNG được phép");
        tglmtDestroyContext(core);
    }
}

// ---------------------------------------------------------------------------
// 2+3+4+5+6. Batch tam giác: chương trình, uniform, geometry, trace.
// ---------------------------------------------------------------------------
static void TestImmediateTriangle() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Dựng hình: 1 tam giác trước mặt phẳng z=0.
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    const size_t traceBefore = c.device->drawTrace().size();
    glEnd();
    TGLMT_CHECK(c.legacy.immediatePrimitives.size() == 1, "glEnd ghép 1 primitive");
    TGLMT_CHECK(c.legacy.immediateVertices.size() == 3, "3 đỉnh được gom");

    // glEnd PHẢI render ngay (spec GL 2.1 §2.6: glBegin/glEnd tạo primitive
    // và render nó khi glEnd trả về) ⇒ chương trình legacy đã được tạo và
    // draw đã vào trace TRƯỚC khi glEnd trả về.
    TGLMT_CHECK(c.legacy.program != nullptr, "glEnd đã tạo chương trình legacy");
    TGLMT_CHECK(c.legacy.programValid, "chương trình legacy đánh dấu hợp lệ");
    TGLMT_CHECK(c.device->drawTrace().size() == traceBefore + 1,
                "glEnd phát draw ngay (1 draw mới trong DrawTrace)");

    // --- Chương trình KHÔNG nằm trong Context::programs ---
    TGLMT_CHECK(c.programs.empty(), "chương trình legacy KHÔNG nằm trong Context::programs");
    GLint curProg = -1;
    glGetIntegerv(kCurrentProgram, &curProg);
    TGLMT_CHECK(curProg == 0, "GL_CURRENT_PROGRAM vẫn = 0 (legacy không phải GL program)");

    // --- MSL sinh ra có đủ các stage ---
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr != nullptr && !pr->vertexMSL.empty(), "MSL vertex sinh ra (không rỗng)");
    TGLMT_CHECK(pr != nullptr && !pr->fragmentMSL.empty(), "MSL fragment sinh ra (không rỗng)");
    if (pr) {
        TGLMT_CHECK(pr->vertexMSL.find("TGLMT_vs") != std::string::npos,
                    "MSL vertex có entry point TGLMT_vs");
        TGLMT_CHECK(pr->fragmentMSL.find("TGLMT_fs") != std::string::npos,
                    "MSL fragment có entry point TGLMT_fs");
        TGLMT_CHECK(pr->vertexMSL.find("TGLMTLegacyLight") == std::string::npos,
                    "lighting TẮT ⇒ không có khối lighting trong MSL");
        TGLMT_CHECK(pr->fragmentMSL.find("discard_fragment") == std::string::npos,
                    "alpha test TẮT ⇒ không có discard trong MSL");
        TGLMT_CHECK(pr->vertexMSL.find("[[attribute(5)]]") != std::string::npos,
                    "vertex shader đọc 6 attribute (layout legacy)");
    }

    // --- Uniform: model-view (identity) ---
    float mv[4] = {-1, -1, -1, -1};
    TGLMT_CHECK(FindUniform(c, "mvc0", mv), "có uniform mvc0");
    TGLMT_CHECK(Near(mv[0], 1.0f) && Near(mv[1], 0.0f) && Near(mv[2], 0.0f) && Near(mv[3], 0.0f),
                "mvc0 = cột 0 của model-view (identity)");
    float pj[4] = {-1, -1, -1, -1};
    TGLMT_CHECK(FindUniform(c, "pjc0", pj), "có uniform pjc0");

    // --- Geometry: 3 đỉnh, 3 index ---
    TGLMT_CHECK(c.legacy.vertexCount == 3, "3 đỉnh được đẩy lên buffer");
    TGLMT_CHECK(c.legacy.indexCount == 3, "3 index (1 tam giác)");
    TGLMT_CHECK(c.legacy.indexData.size() == 3, "index data có 3 phần tử");
    TGLMT_CHECK(c.legacy.vertexBuffer != nullptr, "vertex buffer đã được tạo");
    if (c.legacy.indexData.size() == 3) {
        TGLMT_CHECK(c.legacy.indexData[0] == 0 && c.legacy.indexData[1] == 1 &&
                        c.legacy.indexData[2] == 2,
                    "index = (0,1,2) — đúng thứ tự primitive đã ghép");
    }

    // --- Trace của backend Null ---
    const auto& trace = c.device->drawTrace();
    if (trace.size() == traceBefore + 1) {
        const metal::DrawTrace& t = trace.back();
        TGLMT_CHECK(t.prim == metal::PrimitiveType::Triangle, "trace: primitive = Triangle");
        TGLMT_CHECK(t.count == 3, "trace: count = 3 index");
        TGLMT_CHECK(t.instanceCount == 1, "trace: instanceCount = 1");
        TGLMT_CHECK(t.indexed, "trace: draw có index (indexed = true)");
    }

    // Vẽ lần 2 cùng batch ⇒ KHÔNG vẽ 2 lần.
    const size_t traceAfter = c.device->drawTrace().size();
    tglmt::gl::LegacyDrawImmediate(c);
    TGLMT_CHECK(c.device->drawTrace().size() == traceAfter,
                "vẽ lại cùng batch ⇒ không phát draw thứ hai");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4b. Uniform phản ánh state: model-view, light, material, fog.
// ---------------------------------------------------------------------------
static void TestUniformsFollowState() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glMatrixMode(0x1700 /*GL_MODELVIEW*/);
    glTranslatef(2.0f, 3.0f, 4.0f);
    glEnable(kLighting);
    glEnable(kLight0);
    GLfloat dif[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    glLightfv(kLight0, kDiffuse, dif);
    GLfloat mat[4] = {0.25f, 0.5f, 0.75f, 1.0f};
    glMaterialfv(0x0404 /*GL_FRONT*/, kMaterialDiffuse, mat);
    glEnable(kFog);
    glFogf(0x0B62 /*GL_FOG_DENSITY*/, 0.125f);

    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được sau khi bật lighting+fog");

    // Model-view: cột 3 = (2,3,4,1).
    float mv[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "mvc3", mv), "có uniform mvc3");
    TGLMT_CHECK(Near(mv[0], 2.0f) && Near(mv[1], 3.0f) && Near(mv[2], 4.0f) && Near(mv[3], 1.0f),
                "mvc3 = phép translate của model-view");
    // Light 0 diffuse.
    float ld[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "light0_dif", ld), "có uniform light0_dif");
    TGLMT_CHECK(Near(ld[0], 1.0f) && Near(ld[1], 0.0f), "light0_dif = diffuse đặt");
    // Material front diffuse.
    float md[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "matF_dif", md), "có uniform matF_dif");
    TGLMT_CHECK(Near(md[0], 0.25f) && Near(md[1], 0.5f) && Near(md[2], 0.75f),
                "matF_dif = diffuse của material");
    // Fog density.
    float fg[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "fogParams", fg), "có uniform fogParams");
    TGLMT_CHECK(Near(fg[1], 0.125f), "fogParams.y = FOG_DENSITY");
    // Lighting bật ⇒ MSL có khối lighting; fog bật ⇒ FS có phép fog.
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->vertexMSL.find("TGLMTLegacyLight") != std::string::npos,
                "lighting bật ⇒ MSL có hàm lighting");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("u.fogParams.x") != std::string::npos,
                "fog bật ⇒ MSL có phép tính fog");
    TGLMT_CHECK(pr && pr->vertexMSL.find("float4 lights[56]") != std::string::npos,
                "uniform lights có 8 nguồn × 7 float4");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 5b. Hình dạng chương trình đổi ⇒ sinh lại chương trình (linkGen tăng).
// ---------------------------------------------------------------------------
static void TestShapeRegeneration() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    tglmt::gl::LegacyDrawImmediate(c);
    const uint64_t gen1 = c.legacy.program->linkGen;
    const uint64_t shape1 = c.legacy.shapeKey;

    // Đổi GIÁ TRỊ (màu đèn) ⇒ KHÔNG sinh lại (chỉ nạp lại uniform).
    glEnable(kLight0);
    GLfloat dif[4] = {0.5f, 0.5f, 0.5f, 1.0f};
    glLightfv(kLight0, kDiffuse, dif);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    tglmt::gl::LegacyDrawImmediate(c);
    TGLMT_CHECK(c.legacy.program->linkGen == gen1,
                "đổi giá trị đèn ⇒ KHÔNG sinh lại chương trình (linkGen giữ nguyên)");
    TGLMT_CHECK(c.legacy.shapeKey == shape1, "shape key giữ nguyên khi chỉ đổi giá trị");
    float ld[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "light0_dif", ld) && Near(ld[0], 0.5f),
                "giá trị đèn mới đã vào uniform");

    // Đổi HÌNH DẠNG (bật lighting) ⇒ sinh lại chương trình.
    glEnable(kLighting);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    tglmt::gl::LegacyDrawImmediate(c);
    TGLMT_CHECK(c.legacy.program->linkGen > gen1,
                "bật lighting ⇒ sinh lại chương trình (linkGen tăng)");
    TGLMT_CHECK(c.legacy.shapeKey != shape1, "shape key đổi theo hình dạng");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 6b. Quad: 4 đỉnh → 2 tam giác → 6 index.
// ---------------------------------------------------------------------------
static void TestQuadExpansion() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(kQuads);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.immediatePrimitives.size() == 2,
                "GL_QUADS ghép thành 2 tam giác");
    tglmt::gl::LegacyDrawImmediate(c);
    TGLMT_CHECK(c.legacy.vertexCount == 4, "4 đỉnh");
    TGLMT_CHECK(c.legacy.indexCount == 6, "6 index (2 tam giác)");
    if (c.legacy.indexData.size() == 6) {
        const uint32_t* ix = c.legacy.indexData.data();
        TGLMT_CHECK(ix[0] == 0 && ix[1] == 1 && ix[2] == 2 && ix[3] == 0 && ix[4] == 2 &&
                        ix[5] == 3,
                    "quad (0,1,2,3) → (0,1,2)+(0,2,3)");
    }
    const auto& trace = c.device->drawTrace();
    TGLMT_CHECK(!trace.empty() && trace.back().count == 6, "trace: count = 6 index");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 7. Có program object bound ⇒ KHÔNG dùng đường fixed-function.
// ---------------------------------------------------------------------------
static void TestProgramBoundDisablesLegacy() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Tạo + link một program GLSL tối giản (vertex + fragment).
    GLuint vs = glCreateShader(0x8B31 /*GL_VERTEX_SHADER*/);
    const char* vsSrc =
        "#version 450\n"
        "layout(location=0) in vec4 p;\n"
        "void main(){ gl_Position = p; }\n";
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(0x8B30 /*GL_FRAGMENT_SHADER*/);
    const char* fsSrc =
        "#version 450\n"
        "layout(location=0) out vec4 c;\n"
        "void main(){ c = vec4(1.0); }\n";
    glShaderSource(fs, 1, &fsSrc, nullptr);
    glCompileShader(fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint linked = 0;
    glGetProgramiv(prog, 0x8B82 /*GL_LINK_STATUS*/, &linked);
    TGLMT_CHECK(linked == 1, "program GLSL tối giản link được");
    glUseProgram(prog);

    // Batch immediate-mode vẫn gom được, nhưng đường fixed-function KHÔNG
    // được chọn khi đang có program trong use (GL 2.1 §2.15).
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.state.BoundProgram() == prog, "GL_CURRENT_PROGRAM = program vừa link");
    TGLMT_CHECK(c.programs.size() == 1, "Context::programs chỉ có program GLSL");
    TGLMT_CHECK(c.legacy.program == nullptr,
                "có program bound ⇒ chương trình fixed-function KHÔNG được sinh");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 8. Auto-flush: batch immediate-mode được vẽ khi có draw tiếp theo.
// ---------------------------------------------------------------------------
static void TestAutoFlushOnNextDraw() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const size_t before = c.device->drawTrace().size();
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    // glEnd render NGAY (spec GL 2.1 §2.6) ⇒ draw fixed-function đã có
    // trong trace TRƯỚC khi glEnd trả về, không cần đợi draw kế tiếp.
    const auto& trace = c.device->drawTrace();
    TGLMT_CHECK(trace.size() == before + 1, "glEnd phát 1 draw fixed-function");
    bool foundLegacy = false;
    for (size_t i = before; i < trace.size(); ++i)
        if (trace[i].indexed && trace[i].count == 3 &&
            trace[i].prim == metal::PrimitiveType::Triangle)
            foundLegacy = true;
    TGLMT_CHECK(foundLegacy, "trace có draw fixed-function (indexed, Triangle, 3 index)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 9. Lỗi: glEnd không có glBegin ⇒ INVALID_OPERATION (không vẽ gì).
// ---------------------------------------------------------------------------
static void TestErrorSemantics() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const size_t before = c.device->drawTrace().size();
    glEnd();
    TGLMT_CHECK(glGetError() == 0x0502 /*GL_INVALID_OPERATION*/,
                "glEnd ngoài glBegin ⇒ GL_INVALID_OPERATION");
    TGLMT_CHECK(c.device->drawTrace().size() == before, "glEnd lỗi ⇒ không vẽ gì");

    // Batch rỗng (glBegin rồi glEnd không có đỉnh) ⇒ không vẽ.
    glBegin(kTriangles);
    glEnd();
    DrainErrors();
    TGLMT_CHECK(c.device->drawTrace().size() == before, "batch rỗng ⇒ không phát draw");
    TGLMT_CHECK(!tglmt::gl::LegacyDrawImmediate(c), "LegacyDrawImmediate trả false khi batch rỗng");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 10. Texture environment + texture matrix + texgen: MSL và vertex data.
// ---------------------------------------------------------------------------
static void TestTextureEnvAndTexGen() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Texture 2×2 RGBA trên unit 0.
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    glTexParameteri(kTexture2D, 0x2801 /*GL_TEXTURE_MIN_FILTER*/, 0x2600 /*GL_NEAREST*/);
    glTexParameteri(kTexture2D, 0x2800 /*GL_TEXTURE_MAG_FILTER*/, 0x2600);
    const GLuint px[4] = {0xFF0000FFu, 0x00FF00FFu, 0x0000FFFFu, 0xFFFFFFFFu};
    glTexImage2D(kTexture2D, 0, 0x1908 /*GL_RGBA*/, 2, 2, 0, 0x1908, 0x1401 /*GL_UNSIGNED_BYTE*/,
                 px);
    glEnable(kTexture2D);
    glTexEnvi(0x2300 /*GL_TEXTURE_ENV*/, kTextureEnvMode, kModulate);

    // Texture matrix: s' = s + 0.25 ⇒ dịch toạ độ texture.
    glMatrixMode(0x1702 /*GL_TEXTURE*/);
    glTranslatef(0.25f, 0.0f, 0.0f);
    glMatrixMode(0x1700 /*GL_MODELVIEW*/);

    glBegin(kTriangles);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với texture env");

    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->fsSamplers.size() == 1, "1 sampler (texture unit 0) trong program");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("tex0.sample(smp0") != std::string::npos,
                "FS sample texture unit 0");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("TGLMTSource") != std::string::npos,
                "FS có phép ánh xạ texel → (Cs,As)");
    // Mode MODULATE ⇒ Cv = Cp * Cs.
    TGLMT_CHECK(pr && pr->fragmentMSL.find("float4 res = float4(Cp.rgb * own.rgb,") !=
                    std::string::npos,
                "mode MODULATE ⇒ Cv = Cp·Cs");

    // Vertex data: đọc buffer đỉnh để kiểm tra toạ độ texture đã qua
    // texture matrix (s = 1.0 + 0.25 = 1.25).
    TGLMT_CHECK(c.legacy.vertexBuffer != nullptr, "vertex buffer tồn tại");
    if (c.legacy.vertexBuffer) {
        const float* v = (const float*)c.legacy.vertexBuffer->contents();
        const size_t off = c.legacy.vertexOffset / sizeof(float);
        TGLMT_CHECK(Near(v[off + 12], 1.25f) && Near(v[off + 13], 0.0f),
                    "texCoord sau texture matrix = (1.25, 0)");
        TGLMT_CHECK(Near(v[off + 0], 0.0f) && Near(v[off + 1], 0.0f) &&
                        Near(v[off + 2], 0.0f) && Near(v[off + 3], 1.0f),
                    "position đỉnh 0 = (0,0,0,1)");
        TGLMT_CHECK(Near(v[off + 6], 1.0f), "normal đỉnh 0 = (0,0,1) (mặc định GL)");
    }

    // Mode REPLACE: đặt TRỰC TIẾP vào state (glTexEnvi hiện từ chối GL_REPLACE
    // vì bảng enum của src/gl/gl_legacy_texenv.cpp sai — xem
    // docs/LEGACY_DRAW_PATH.md §"Follow-up bắt buộc"). Đường fixed-function
    // phải hiểu CẢ hai giá trị.
    c.legacy.texEnv[0].mode = kReplace;
    c.legacy.drawnPrims = 0;
    c.legacy.drawnVerts = 0;
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ lại với mode REPLACE");
    TGLMT_CHECK(c.legacy.program &&
                    c.legacy.program->fragmentMSL.find("float4 res = float4(own.rgb,") !=
                        std::string::npos,
                "mode REPLACE ⇒ Cv = Cs");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 11. TexGen: OBJECT_LINEAR sinh toạ độ texture từ toạ độ đỉnh.
// ---------------------------------------------------------------------------
static void TestTexGenObjectLinear() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // GL_TEXTURE_GEN_S với OBJECT_LINEAR: g = p1*x (p = (2,0,0,0)).
    glMatrixMode(0x1702 /*GL_TEXTURE*/);
    glLoadIdentity();
    glMatrixMode(0x1700 /*GL_MODELVIEW*/);
    glEnable(0x0C60 /*GL_TEXTURE_GEN_S*/);
    glTexGenfv(0x2000 /*GL_S*/, 0x2501 /*GL_OBJECT_PLANE*/,
               (const GLfloat[]){2.0f, 0.0f, 0.0f, 0.0f});

    glBegin(kTriangles);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(2.0f, 0.0f, 0.0f);
    glVertex3f(3.0f, 0.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với texgen");
    if (c.legacy.vertexBuffer) {
        const float* v = (const float*)c.legacy.vertexBuffer->contents();
        const size_t off = c.legacy.vertexOffset / sizeof(float);
        TGLMT_CHECK(Near(v[off + 12], 2.0f), "texgen OBJECT_LINEAR: s = 2·x = 2");
        TGLMT_CHECK(Near(v[off + 12 + 24], 4.0f), "texgen đỉnh 2: s = 2·x = 4");
    }
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 12. Alpha test bật ⇒ FS có discard_fragment.
// ---------------------------------------------------------------------------
static void TestAlphaTestShape() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glEnable(kAlphaTest);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với alpha test");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->fragmentMSL.find("discard_fragment") != std::string::npos,
                "alpha test bật ⇒ FS có discard_fragment");
    float at[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "alphaTest", at), "có uniform alphaTest");
    TGLMT_CHECK(Near(at[0], 7.0f), "alpha test func mặc định = GL_ALWAYS (7)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 13. Draw MODERN không có program KHÔNG được đi qua chương trình legacy.
//     Vertex data của glDrawArrays nằm ở vertex array của app, không nằm
//     trong buffer layout legacy ⇒ nếu bị đi nhầm sang đường legacy thì hình
//     sai. Đường fixed-function kiểu vertex-array chưa triển khai ⇒ draw đó
//     phải bị bỏ (noProgram++), đúng như trước khi có đường legacy.
// ---------------------------------------------------------------------------
static void TestModernDrawNotRoutedToLegacy() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();

    // Quy tắc chọn chương trình (dùng chung cho đường draw):
    //  • program != 0            ⇒ KHÔNG dùng fixed-function.
    //  • program == 0 nhưng draw KHÔNG phải draw fixed-function (drawActive=0)
    //                             ⇒ KHÔNG dùng fixed-function.
    ProgramObject* sel = nullptr;
    TGLMT_CHECK(!tglmt::gl::LegacySelectProgram(c, 0, sel),
                "draw modern (không phải batch legacy) ⇒ KHÔNG chọn chương trình legacy");
    TGLMT_CHECK(sel == nullptr, "out = nullptr khi không chọn legacy");
    c.legacy.drawActive = true;   // giả lập draw fixed-function đang chạy
    TGLMT_CHECK(tglmt::gl::LegacySelectProgram(c, 0, sel),
                "draw fixed-function + program == 0 ⇒ chọn chương trình legacy");
    TGLMT_CHECK(sel != nullptr && sel == c.legacy.program,
                "chương trình được chọn = Context::legacy.program");
    TGLMT_CHECK(!tglmt::gl::LegacySelectProgram(c, 7, sel),
                "có program object (id 7) ⇒ KHÔNG dùng fixed-function (GL 2.1 §2.15)");
    c.legacy.drawActive = false;

    // Batch immediate-mode VẪN phải được flush khi có draw kế tiếp (đúng thứ
    // tự GL: glEnd sinh pixel trước draw kế tiếp).
    glDrawArrays(kTriangles, 0, 3);
    const auto& trace = c.device->drawTrace();
    bool foundLegacy = false;
    for (const auto& t : trace)
        if (t.indexed && t.count == 3 && t.prim == metal::PrimitiveType::Triangle) foundLegacy = true;
    TGLMT_CHECK(foundLegacy, "batch immediate-mode vẫn được flush trước draw modern");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 14. Two-sided lighting + texture: màu primary đã chọn mặt (front/back)
//     phải là màu đi vào texture environment (spec GL 2.1 §3.8.16).
// ---------------------------------------------------------------------------
static void TestTwoSidedWithTexture() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    glTexImage2D(kTexture2D, 0, 0x1908 /*GL_RGBA*/, 2, 2, 0, 0x1908, 0x1401, nullptr);
    glEnable(kTexture2D);
    glEnable(kLighting);
    glLightModeli(0x0B52 /*GL_LIGHT_MODEL_TWO_SIDE*/, GL_TRUE);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với two-sided + texture");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->vertexMSL.find("TGLMTLegacyLight(-n,") != std::string::npos,
                "two-sided: mặt sau dùng normal đảo dấu (-n)");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("float4 Cf = primary;") != std::string::npos,
                "texture env nhận màu primary ĐÃ chọn mặt (Cf = primary)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 15. GL_DECAL: chỉ định nghĩa cho base format RGB/RGBA; base format khác
//     ⇒ undefined ⇒ quy tắc D2 của docs/LEGACY_TEXENV_FOG_SPEC.md: no-op.
// ---------------------------------------------------------------------------
static void TestDecalOnlyForRgbBaseFormat() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Texture ALPHA (base format không phải RGB/RGBA).
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    glTexImage2D(kTexture2D, 0, 0x1906 /*GL_ALPHA*/, 2, 2, 0, 0x1906, 0x1401, nullptr);
    glEnable(kTexture2D);
    c.legacy.texEnv[0].mode = 0x2101 /*GL_DECAL*/;
    c.legacy.drawnPrims = 0;
    c.legacy.drawnVerts = 0;
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với DECAL");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->fragmentMSL.find("float4 res = Cp;") != std::string::npos,
                "DECAL trên base format ALPHA ⇒ no-op (doc D2)");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("Cp.rgb * (1.0 - own.a)") == std::string::npos,
                "DECAL trên base format ALPHA ⇒ KHÔNG dùng công thức lerp");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 16. Fog GL_LINEAR với start == end ⇒ f = 0 (quy tắc D6).
// ---------------------------------------------------------------------------
static void TestFogLinearStartEqualsEnd() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glEnable(kFog);
    glFogf(0x0B65 /*GL_FOG_MODE*/, 0x2601 /*GL_LINEAR*/);
    glFogf(0x0B63 /*GL_FOG_START*/, 2.0f);
    glFogf(0x0B64 /*GL_FOG_END*/, 2.0f);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với fog LINEAR");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->fragmentMSL.find("fg = (e != s) ? ((e - fogc) / (e - s)) : 0.0;") !=
                    std::string::npos,
                "fog LINEAR với s == e ⇒ f = 0 (quy tắc D6)");
    float fg[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "fogParams", fg), "có uniform fogParams");
    TGLMT_CHECK(Near(fg[0], 0.0f) && Near(fg[2], 2.0f) && Near(fg[3], 2.0f),
                "fogParams: mode LINEAR(0), start = end = 2");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 17. Alpha của màu primary = alpha của vật liệu diffuse (sau glColorMaterial).
// ---------------------------------------------------------------------------
static void TestPrimaryAlphaFromDiffuse() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glEnable(kLighting);
    glEnable(kColorMaterial);
    glColorMaterial(0x0404 /*GL_FRONT*/, 0x1201 /*GL_DIFFUSE*/);
    GLfloat mat[4] = {0.2f, 0.3f, 0.4f, 0.5f};
    glMaterialfv(0x0404 /*GL_FRONT*/, 0x1201 /*GL_DIFFUSE*/, mat);
    glBegin(kTriangles);
    glColor4f(1.0f, 0.0f, 0.0f, 0.25f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với color material");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->vertexMSL.find("float4(clamp(cpri, 0.0, 1.0), matDf.a)") !=
                    std::string::npos,
                "alpha của primary lấy từ diffuse effective (sau glColorMaterial)");
    // mask diffuse = 1 ⇒ alpha của current color thay cho alpha của material.
    TGLMT_CHECK(pr && pr->vertexMSL.find("matDeff = matD;") != std::string::npos,
                "helper lighting trả về vật liệu diffuse đã thay bởi current color");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 18. GL_FLAT: varying màu dùng [[flat]] (đỉnh provoking).
// ---------------------------------------------------------------------------
static void TestFlatShading() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với GL_SMOOTH");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->vertexMSL.find("[[flat, user(locn0)]]") == std::string::npos,
                "GL_SMOOTH (mặc định) ⇒ varying màu KHÔNG flat");

    // GL_FLAT = 0x1D00 (docs/khronos/gl.xml). Đặt trực tiếp vì
    // src/gl/gl_legacy_light.cpp dùng bảng enum sai cho glShadeModel
    // (xem docs/LEGACY_DRAW_PATH.md §"Follow-up bắt buộc").
    c.legacy.shadeModel = 0x1D00;
    c.legacy.drawnPrims = 0;
    c.legacy.drawnVerts = 0;
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với GL_FLAT");
    const ProgramObject* pr2 = c.legacy.program;
    TGLMT_CHECK(pr2 && pr2->vertexMSL.find("[[flat, user(locn0)]]") != std::string::npos,
                "GL_FLAT ⇒ varying màu dùng [[flat]]");
    TGLMT_CHECK(pr2 && pr2->fragmentMSL.find("[[flat, user(locn0)]]") != std::string::npos,
                "GL_FLAT ⇒ FS khai báo varying màu flat");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 19. Kích thước uniform buffer CPU phải khớp struct MSL (byte-for-byte).
//     VS: 4 (mvc) + 4 (pjc) + 3 (nrm) + 56 (lights 8×7) + 5 (matF) + 5 (matB)
//         + 1 (sceneAmb) + 1 (flags) + 1 (flags2) = 80 float4 = 1280 byte.
//     FS: 4 (fog/alpha) + 2 (env) + 8 (comb A..D) = 14 float4 = 224 byte.
// ---------------------------------------------------------------------------
static void TestUniformBufferSizes() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glEnable(kLighting);
    glEnable(kFog);
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    glTexImage2D(kTexture2D, 0, 0x1908 /*GL_RGBA*/, 2, 2, 0, 0x1908, 0x1401, nullptr);
    glEnable(kTexture2D);
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với đủ feature");
    const ProgramObject* pr = c.legacy.program;
    TGLMT_CHECK(pr && pr->vsUBSize == 80u * 16u, "VS uniform buffer = 1280 byte (80 float4)");
    TGLMT_CHECK(pr && pr->fsUBSize == 14u * 16u, "FS uniform buffer = 224 byte (14 float4)");
    // MSL phải khai báo struct cùng kích thước.
    TGLMT_CHECK(pr && pr->vertexMSL.find("float4 lights[56];") != std::string::npos,
                "struct VS có lights[56] (8 nguồn × 7 float4)");
    TGLMT_CHECK(pr && pr->fragmentMSL.find("float4 combD[2];") != std::string::npos,
                "struct FS có combD[2] (2 texture unit)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 20. Thứ tự GL: glClear sau glEnd phải XOÁ hình vừa vẽ ⇒ batch immediate-mode
//     phải được flush TRƯỚC clear.
// ---------------------------------------------------------------------------
static void TestFlushBeforeClear() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    const size_t before = c.device->drawTrace().size();
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    // glEnd đã render ⇒ batch KHÔNG còn pending ⇒ glClear không được phát
    // draw nữa (trước đây batch chỉ được flush khi có draw/clear kế tiếp).
    TGLMT_CHECK(c.device->drawTrace().size() == before + 1,
                "glEnd phát draw ngay (batch không còn pending)");
    const size_t afterDraw = c.device->drawTrace().size();
    glClear(0x4000 /*GL_COLOR_BUFFER_BIT*/);
    TGLMT_CHECK(c.device->drawTrace().size() == afterDraw,
                "glClear KHÔNG phát draw thêm (batch đã render ở glEnd)");
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 21. Mặc định của GL_COMBINE theo spec GL 1.3 §3.8.12 (p.147):
//     combiner RGB/ALPHA = MODULATE; SRC0_* = TEXTURE, SRC1_* = PREVIOUS,
//     SRC2_* = CONSTANT; OPERAND0/1_RGB = SRC_COLOR, OPERAND2_RGB = SRC_ALPHA.
//     State khởi tạo lưu *pname* (0x8580/0x8581/0x8582 …) chứ không phải giá
//     trị nguồn ⇒ đường fixed-function phải map về mặc định của spec.
// ---------------------------------------------------------------------------
static void TestCombineDefaults() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(kTexture2D, tex);
    glTexImage2D(kTexture2D, 0, 0x1908 /*GL_RGBA*/, 2, 2, 0, 0x1908, 0x1401, nullptr);
    glEnable(kTexture2D);
    c.legacy.texEnv[0].mode = 0x8570 /*GL_COMBINE*/;
    glBegin(kTriangles);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();
    TGLMT_CHECK(c.legacy.programValid, "vẽ được với GL_COMBINE");
    // combA0 = (src0RGB, src1RGB, src2RGB, combineRGB)
    //   src0 = TEXTURE(0), src1 = PREVIOUS(3), src2 = CONSTANT(1),
    //   combine = MODULATE(1)
    float a[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "combA0", a), "có uniform combA0");
    TGLMT_CHECK(Near(a[0], 0.0f) && Near(a[1], 3.0f) && Near(a[2], 1.0f) && Near(a[3], 1.0f),
                "combA0 mặc định = (TEXTURE, PREVIOUS, CONSTANT, MODULATE)");
    // combC0 = (operand0RGB, operand1RGB, operand2RGB, rgbScale)
    //   operand0/1 = SRC_COLOR(0), operand2 = SRC_ALPHA(2), scale = 1
    float cc[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "combC0", cc), "có uniform combC0");
    TGLMT_CHECK(Near(cc[0], 0.0f) && Near(cc[1], 0.0f) && Near(cc[2], 2.0f) && Near(cc[3], 1.0f),
                "combC0 mặc định = (SRC_COLOR, SRC_COLOR, SRC_ALPHA, scale 1)");
    // combB0 (alpha) = (src0A, src1A, src2A, combineAlpha)
    float b[4] = {0, 0, 0, 0};
    TGLMT_CHECK(FindUniform(c, "combB0", b), "có uniform combB0");
    TGLMT_CHECK(Near(b[0], 0.0f) && Near(b[1], 3.0f) && Near(b[2], 1.0f) && Near(b[3], 1.0f),
                "combB0 mặc định = (TEXTURE, PREVIOUS, CONSTANT, MODULATE)");
    tglmtDestroyContext(ctx);
}

int main() {
    TestProfileGate();
    TestImmediateTriangle();
    TestUniformsFollowState();
    TestShapeRegeneration();
    TestQuadExpansion();
    TestProgramBoundDisablesLegacy();
    TestAutoFlushOnNextDraw();
    TestErrorSemantics();
    TestTextureEnvAndTexGen();
    TestTexGenObjectLinear();
    TestAlphaTestShape();
    TestModernDrawNotRoutedToLegacy();
    TestTwoSidedWithTexture();
    TestDecalOnlyForRgbBaseFormat();
    TestFogLinearStartEqualsEnd();
    TestPrimaryAlphaFromDiffuse();
    TestFlatShading();
    TestUniformBufferSizes();
    TestFlushBeforeClear();
    TestCombineDefaults();
    printf("test_legacy_draw: done\n");
    return TGLMT_RESULT;
}
