// test_legacy_immediate.cpp — T1: OpenGL immediate mode + primitive assembly.
//
// Kiểm chứng theo spec OpenGL 1.x/2.1 §2.6 (immediate mode) + §5 (primitive
// assembly):
//  - glBegin/glEnd phải cân bằng; glBegin lồng nhau và glEnd lạ ⇒
//    GL_INVALID_OPERATION.
//  - glVertex* ngoài glBegin ⇒ GL_INVALID_OPERATION.
//  - glBegin với mode sai ⇒ GL_INVALID_VALUE.
//  - Current vertex attributes (color/normal/texcoord) được chụp lại theo từng
//    đỉnh tại thời điểm gọi glVertex.
//  - Ghép primitive đúng thứ tự, đặc biệt GL_QUAD_STRIP (quad i = (2i,2i+1,
//    2i+3,2i+2)) và GL_TRIANGLE_STRIP (winding đảo ở tam giác chẵn).
#include "tglmt/gl_c_api.h"
#include "tglmt/gl_context_api.h"
#include "tglmt/Context.h"
#include "tglmt/LegacyState.h"
#include "tglmt/gl_legacy_types.h"
#include "test_util.h"
#include <cstdio>
#include <cstring>

using namespace tglmt;

namespace {


void DrainErrors() { while (glGetError() != 0) {} }

const LegacyState& Legacy() { return Context::CurrentPtr()->legacy; }

bool PrimEq(const LegacyPrimitive& p, GLenum mode, uint32_t a, uint32_t b, uint32_t c,
            uint32_t count) {
    return p.mode == mode && p.count == count && p.idx[0] == a && p.idx[1] == b &&
           p.idx[2] == c;
}

}  // namespace

int main() {
    TGLMTContextAttribs a{};
    a.majorVersion = 4;
    a.minorVersion = 6;
    a.profile = TGLMT_PROFILE_COMPATIBILITY;
    a.backend = "null";
    TGLMTContext* ctx = tglmtCreateContextAttribs(&a);
    TGLMT_CHECK(ctx != nullptr, "tạo được context GL 4.6 Compatibility");
    if (!ctx) return 1;
    tglmtMakeCurrent(ctx);

    // ---- 1. Cân bằng glBegin/glEnd -----------------------------------------
    {
        DrainErrors();
        glEnd();
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION, "glEnd lạ ⇒ INVALID_OPERATION");
        DrainErrors();
        glBegin(GL_TRIANGLES);
        glBegin(GL_TRIANGLES);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glBegin lồng nhau ⇒ INVALID_OPERATION");
        glEnd();
        DrainErrors();
        glBegin(0x9999);
        TGLMT_CHECK(glGetError() == GL_INVALID_VALUE, "glBegin mode sai ⇒ INVALID_VALUE");
        DrainErrors();
        glVertex3f(0, 0, 0);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glVertex ngoài glBegin ⇒ INVALID_OPERATION");
    }

    // ---- 2. Current vertex attributes được chụp theo từng đỉnh -------------
    {
        DrainErrors();
        glBegin(GL_TRIANGLES);
        glColor4f(1, 0, 0, 1);
        glNormal3f(0, 0, 1);
        glTexCoord2f(0.25f, 0.75f);
        glVertex3f(1, 2, 3);
        glColor4f(0, 1, 0, 0.5f);
        glTexCoord2f(0.5f, 0.5f);
        glVertex3f(4, 5, 6);
        glVertex3f(7, 8, 9);
        glEnd();
        TGLMT_CHECK(glGetError() == 0, "immediate mode hợp lệ ⇒ không lỗi");
        const LegacyState& st = Legacy();
        TGLMT_CHECK(st.immediateVertices.size() == 3, "gom đủ 3 đỉnh");
        if (st.immediateVertices.size() == 3) {
            const LegacyVertex& v0 = st.immediateVertices[0];
            TGLMT_CHECK(v0.pos[0] == 1 && v0.pos[1] == 2 && v0.pos[2] == 3 && v0.pos[3] == 1,
                        "v0 giữ đúng toạ độ");
            TGLMT_CHECK(v0.color[0] == 1.0f && v0.color[1] == 0.0f && v0.color[2] == 0.0f &&
                            v0.color[3] == 1.0f,
                        "v0 giữ màu tại thời điểm gọi glVertex");
            TGLMT_CHECK(v0.normal[2] == 1.0f, "v0 giữ normal");
            TGLMT_CHECK(v0.texCoord[0] == 0.25f && v0.texCoord[1] == 0.75f,
                        "v0 giữ texcoord");
            const LegacyVertex& v1 = st.immediateVertices[1];
            TGLMT_CHECK(v1.color[1] == 1.0f && v1.color[3] == 0.5f,
                        "v1 giữ màu MỚI (màu không phải trạng thái dùng chung)");
            TGLMT_CHECK(v1.texCoord[0] == 0.5f, "v1 giữ texcoord mới");
            TGLMT_CHECK(v1.normal[2] == 1.0f, "v1 giữ normal (không bị reset)");
        }
    }

    // ---- 3. Ghép primitive ----------------------------------------------------
    {
        struct Case { GLenum mode; size_t n; };
        // GL_TRIANGLES
        {
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < 3; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 1, "TRIANGLES ⇒ 1 primitive");
            if (st.immediatePrimitives.size() == 1)
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 2, 3),
                            "TRIANGLES ⇒ (0,1,2)");
        }
        // GL_TRIANGLE_STRIP: tam giác chẵn giữ thứ tự, tam giác lẻ đảo winding
        {
            glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i < 4; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "TRIANGLE_STRIP ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 2, 3),
                            "TRIANGLE_STRIP tam giác 0 = (0,1,2)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_TRIANGLES, 2, 1, 3, 3),
                            "TRIANGLE_STRIP tam giác 1 đảo winding = (2,1,3)");
            }
        }
        // GL_TRIANGLE_FAN
        {
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i < 4; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "TRIANGLE_FAN ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 2, 3),
                            "FAN tam giác 0 = (0,1,2)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_TRIANGLES, 0, 2, 3, 3),
                            "FAN tam giác 1 = (0,2,3)");
            }
        }
        // GL_QUADS
        {
            glBegin(GL_QUADS);
            for (int i = 0; i < 4; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "QUADS ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 2, 3),
                            "QUADS tam giác 0 = (0,1,2)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_TRIANGLES, 0, 2, 3, 3),
                            "QUADS tam giác 1 = (0,2,3)");
            }
        }
        // GL_QUAD_STRIP: quad i = (2i, 2i+1, 2i+3, 2i+2)  ← thứ tự QUAN TRỌNG
        {
            glBegin(GL_QUAD_STRIP);
            for (int i = 0; i < 6; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 4, "QUAD_STRIP ⇒ 4 primitive");
            if (st.immediatePrimitives.size() == 4) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 3, 3),
                            "QUAD_STRIP quad0 tam giác 0 = (0,1,3)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_TRIANGLES, 0, 3, 2, 3),
                            "QUAD_STRIP quad0 tam giác 1 = (0,3,2)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[2], GL_TRIANGLES, 2, 3, 5, 3),
                            "QUAD_STRIP quad1 tam giác 0 = (2,3,5)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[3], GL_TRIANGLES, 2, 5, 4, 3),
                            "QUAD_STRIP quad1 tam giác 1 = (2,5,4)");
            }
        }
        // GL_POLYGON
        {
            glBegin(GL_POLYGON);
            for (int i = 0; i < 4; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "POLYGON ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_TRIANGLES, 0, 1, 2, 3),
                            "POLYGON tam giác 0 = (0,1,2)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_TRIANGLES, 0, 2, 3, 3),
                            "POLYGON tam giác 1 = (0,2,3)");
            }
        }
        // GL_LINES / GL_LINE_STRIP / GL_LINE_LOOP
        {
            glBegin(GL_LINES);
            for (int i = 0; i < 4; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "LINES ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_LINES, 0, 1, 0, 2),
                            "LINES cặp 0 = (0,1)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_LINES, 2, 3, 0, 2),
                            "LINES cặp 1 = (2,3)");
            }
        }
        {
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i < 3; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 2, "LINE_STRIP ⇒ 2 primitive");
            if (st.immediatePrimitives.size() == 2) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[0], GL_LINES, 0, 1, 0, 2),
                            "LINE_STRIP cạnh 0 = (0,1)");
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[1], GL_LINES, 1, 2, 0, 2),
                            "LINE_STRIP cạnh 1 = (1,2)");
            }
        }
        {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 3; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 3,
                        "LINE_LOOP ⇒ 3 primitive (2 cạnh + 1 cạnh khép)");
            if (st.immediatePrimitives.size() == 3) {
                TGLMT_CHECK(PrimEq(st.immediatePrimitives[2], GL_LINES, 2, 0, 0, 2),
                            "LINE_LOOP cạnh khép = (2,0)");
            }
        }
        {
            glBegin(GL_POINTS);
            for (int i = 0; i < 3; ++i) glVertex3f((float)i, 0, 0);
            glEnd();
            const LegacyState& st = Legacy();
            TGLMT_CHECK(st.immediatePrimitives.size() == 3, "POINTS ⇒ 3 primitive");
            if (!st.immediatePrimitives.empty())
                TGLMT_CHECK(st.immediatePrimitives[0].mode == GL_POINTS &&
                                st.immediatePrimitives[0].count == 1,
                            "POINTS giữ mode GL_POINTS, count = 1");
        }
    }

    // ---- 4. Multi-texture: current texcoord theo unit ------------------------
    {
        DrainErrors();
        // Mỗi texture unit giữ current texcoord RIÊNG (spec GL 1.3 §2.8.1).
        // glMultiTexCoord* KHÔNG đổi unit đang hoạt động; chỉ glActiveTexture mới đổi.
        glActiveTexture(0x84C0 /*GL_TEXTURE0*/);
        glTexCoord2f(0.1f, 0.2f);
        glMultiTexCoord2f(0x84C1 /*GL_TEXTURE1*/, 0.3f, 0.4f);
        GLfloat tc[4] = {0, 0, 0, 0};
        glGetFloatv(0x0B04 /*GL_CURRENT_TEXTURE_COORDS*/, tc);
        TGLMT_CHECK(tc[0] == 0.1f && tc[1] == 0.2f,
                    "GL_CURRENT_TEXTURE_COORDS = unit đang hoạt động (unit 0)");
        glActiveTexture(0x84C1 /*GL_TEXTURE1*/);
        glGetFloatv(0x0B04, tc);
        TGLMT_CHECK(tc[0] == 0.3f && tc[1] == 0.4f,
                    "unit 1 giữ texcoord riêng của nó");
        glActiveTexture(0x84C0);
    }

    tglmtMakeCurrent(nullptr);
    tglmtDestroyContext(ctx);

    // ---- 5. Core Profile 4.6: immediate mode không tồn tại -------------------
    {
        TGLMTContextAttribs ca{};
        ca.majorVersion = 4;
        ca.minorVersion = 6;
        ca.profile = TGLMT_PROFILE_CORE;
        ca.backend = "null";
        TGLMTContext* core = tglmtCreateContextAttribs(&ca);
        tglmtMakeCurrent(core);
        DrainErrors();
        glBegin(GL_TRIANGLES);
        TGLMT_CHECK(glGetError() == GL_INVALID_OPERATION,
                    "glBegin ở GL 4.6 Core ⇒ INVALID_OPERATION");
        tglmtMakeCurrent(nullptr);
        tglmtDestroyContext(core);
    }

    printf("[test_legacy_immediate] OK (%d lỗi)\n", tglmt_test::Failures());
    return TGLMT_RESULT;
}
