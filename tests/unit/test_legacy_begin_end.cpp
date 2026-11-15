// test_legacy_begin_end.cpp — lệnh BỊ CẤM trong glBegin/glEnd.
//
// Spec OpenGL 2.1 §2.6.3 "GL Commands within Begin/End" (glspec21.pdf
// PDF p.19–20) — văn bản giữ nguyên trong OpenGL 4.2/4.6 Compatibility:
//
//   "The only GL commands that are allowed within any Begin/End pairs are the
//    commands for specifying vertex coordinates, vertex colors, normal
//    coordinates, texture coordinates, generic vertex attributes, and fog
//    coordinates (Vertex, Color, SecondaryColor, Index, Normal, TexCoord and
//    MultiTexCoord, VertexAttrib, FogCoord), the ArrayElement command, the
//    EvalCoord and EvalPoint commands, commands for specifying lighting
//    material parameters (Material commands), display list invocation commands
//    (CallList and CallLists), and the EdgeFlag command.
//    Executing any other GL command between the execution of Begin and the
//    corresponding execution of End results in the error INVALID_OPERATION.
//    Executing Begin after Begin has already been executed but before an End is
//    executed generates the INVALID_OPERATION error, as does executing End
//    without a previous corresponding Begin."
//
// Nhãn môi trường: REFERENCE_ONLY (backend Null).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_legacy_types.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "test_util.h"
#include <cstdio>

using namespace tglmt;

namespace {
constexpr GLenum kTriangles = 0x0004;

void DrainErrors() { while (glGetError() != GL_NO_ERROR) {} }

TGLMTContext* CompatCtx() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    return tglmtCreateContextAttribs(&a);
}
}  // namespace

// ---------------------------------------------------------------------------
// 1. Lệnh bị cấm trong Begin/End ⇒ GL_INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestForbiddenCommands() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // --- glBegin lồng nhau ---
    glBegin(kTriangles);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBegin đầu tiên hợp lệ");
    glBegin(kTriangles);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glBegin lồng nhau ⇒ INVALID_OPERATION [spec §2.6.3]");
    glEnd();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnd sau glBegin hợp lệ");

    // --- glEnd không có glBegin ---
    DrainErrors();
    glEnd();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glEnd không có glBegin ⇒ INVALID_OPERATION [spec §2.6.3]");

    // --- glVertex ngoài Begin/End ---
    DrainErrors();
    glVertex3f(0, 0, 0);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "glVertex ngoài Begin/End ⇒ INVALID_OPERATION");

    // --- Các lệnh bị cấm BÊN TRONG Begin/End ---
    struct Case { const char* name; void (*fn)(); };
    // Dùng con trỏ hàm để tránh phải viết lambda cho từng lệnh.
    auto inBegin = [&](void (*fn)(), const char* name) {
        DrainErrors();
        glBegin(kTriangles);
        glVertex3f(0, 0, 0);
        fn();
        GLenum e = glGetError();
        glEnd();
        TGLMT_CHECK(e == GL_INVALID_OPERATION, name);
    };

    inBegin([] { glEnable(GL_LIGHTING); }, "glEnable trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glDisable(GL_LIGHTING); }, "glDisable trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glMatrixMode(0x1700); }, "glMatrixMode trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glLoadIdentity(); }, "glLoadIdentity trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glPushMatrix(); }, "glPushMatrix trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glPopMatrix(); }, "glPopMatrix trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glTranslatef(1, 0, 0); }, "glTranslatef trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glScalef(2, 2, 2); }, "glScalef trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glRotatef(90, 0, 0, 1); }, "glRotatef trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glViewport(0, 0, 16, 16); }, "glViewport trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glLightf(GL_LIGHT0, GL_DIFFUSE, 1.0f); },
            "glLightf trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glColorMaterial(GL_FRONT, GL_AMBIENT); },
            "glColorMaterial trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glShadeModel(GL_FLAT); }, "glShadeModel trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glFogf(GL_FOG_DENSITY, 1.0f); }, "glFogf trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); },
            "glTexEnvi trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR); },
            "glTexGeni trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glAlphaFunc(GL_ALWAYS, 0.5f); },
            "glAlphaFunc trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); },
            "glPolygonMode trong Begin/End ⇒ INVALID_OPERATION");
    inBegin([] { glDrawArrays(GL_TRIANGLES, 0, 3); },
            "glDrawArrays trong Begin/End ⇒ INVALID_OPERATION");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 2. Lệnh ĐƯỢC PHÉP trong Begin/End ⇒ không lỗi.
// ---------------------------------------------------------------------------
static void TestAllowedCommands() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    glBegin(kTriangles);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glBegin hợp lệ");
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glMultiTexCoord2f(GL_TEXTURE1, 0.0f, 0.0f);
    glSecondaryColor3f(0.0f, 0.0f, 0.0f);
    glIndexi(1);
    glEdgeFlag(GL_TRUE);
    glFogCoordf(0.5f);
    // glMaterial ĐƯỢC PHÉP trong Begin/End (spec §2.6.3: "commands for
    // specifying lighting material parameters (Material commands)").
    glMaterialf(GL_FRONT, GL_SHININESS, 32.0f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "lệnh được phép trong Begin/End không sinh lỗi");
    TGLMT_CHECK(glGetError() == GL_NO_ERROR,
                "glMaterialf ĐƯỢC PHÉP trong Begin/End [spec §2.6.3]");
    glEnd();
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glEnd hợp lệ");

    // State của lệnh được phép VẪN được áp dụng.
    TGLMT_CHECK(c.legacy.materialFront.shininess == 32.0f,
                "glMaterial trong Begin/End vẫn được áp dụng");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 3. Lệnh bị cấm phải KHÔNG thực thi (state không đổi).
// ---------------------------------------------------------------------------
static void TestForbiddenDoesNotExecute() {
    TGLMTContext* ctx = CompatCtx();
    TGLMT_CHECK(ctx != nullptr, "tạo được context Compatibility");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    Context& c = Context::Current();
    DrainErrors();

    // Màu hiện tại trước khi vào Begin/End.
    glColor3f(0.25f, 0.5f, 0.75f);
    TGLMT_CHECK(glGetError() == GL_NO_ERROR, "glColor ngoài Begin/End hợp lệ");

    glBegin(kTriangles);
    glVertex3f(0, 0, 0);
    // glColorMaterial bị cấm trong Begin/End ⇒ KHÔNG được đổi mode.
    glColorMaterial(GL_BACK, GL_SPECULAR);
    GLenum e = glGetError();
    glEnd();
    TGLMT_CHECK(e == GL_INVALID_OPERATION, "glColorMaterial trong Begin/End ⇒ INVALID_OPERATION");
    TGLMT_CHECK(c.legacy.colorMaterial.face == GL_FRONT_AND_BACK,
                "lệnh bị cấm KHÔNG được thực thi (colorMaterial.face không đổi)");

    // Tương tự: glEnable(GL_LIGHTING) bị cấm ⇒ lightingEnabled phải giữ.
    DrainErrors();
    glBegin(kTriangles);
    glVertex3f(0, 0, 0);
    glEnable(GL_LIGHTING);
    e = glGetError();
    glEnd();
    TGLMT_CHECK(e == GL_INVALID_OPERATION, "glEnable trong Begin/End ⇒ INVALID_OPERATION");
    TGLMT_CHECK(!c.legacy.lightingEnabled,
                "glEnable bị cấm KHÔNG được thực thi (lighting vẫn tắt)");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

// ---------------------------------------------------------------------------
// 4. Core Profile: glBegin/glEnd không tồn tại ⇒ INVALID_OPERATION.
// ---------------------------------------------------------------------------
static void TestCoreProfile() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_CORE;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context Core");
    if (!ctx) return;
    tglmtMakeCurrent(ctx);
    DrainErrors();

    glBegin(kTriangles);
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glBegin phải INVALID_OPERATION (entry point không tồn tại)");
    glEnd();
    TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                "Core Profile: glEnd phải INVALID_OPERATION");

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);
}

int main() {
    TestForbiddenCommands();
    TestAllowedCommands();
    TestForbiddenDoesNotExecute();
    TestCoreProfile();

    if (tglmt_test::Failures() == 0) {
        printf("test_legacy_begin_end: OK\n");
        return 0;
    }
    return 1;
}
