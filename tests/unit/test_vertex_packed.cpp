// test_vertex_packed.cpp — Unit: ARB_vertex_type_2_10_10_10_rev generic attribs.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cmath>
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool cond, const char* msg) {
    if (!cond) { printf("test_vertex_packed FAIL: %s\n", msg); ++gFails; }
}

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-6f; }

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);

    const uint32_t packed = 0x3FFu | (0x3FFu << 20) | (3u << 30);
    glVertexAttribP4ui(0, 0x8368 /*UNSIGNED_INT_2_10_10_10_REV*/, GL_TRUE, packed);
    GLfloat v4[4] = {};
    glGetVertexAttribfv(0, 0x8626 /*CURRENT_VERTEX_ATTRIB*/, v4);
    Check(Near(v4[0], 1.0f), "P4 x normalized");
    Check(Near(v4[1], 0.0f), "P4 y normalized");
    Check(Near(v4[2], 1.0f), "P4 z normalized");
    Check(Near(v4[3], 1.0f), "P4 w normalized");

    glVertexAttribP2ui(1, 0x8368, GL_TRUE, 0x3FFu | (0x3FFu << 10));
    GLfloat v2[4] = {};
    glGetVertexAttribfv(1, 0x8626, v2);
    Check(Near(v2[0], 1.0f) && Near(v2[1], 1.0f), "P2 xy normalized");
    Check(Near(v2[2], 0.0f) && Near(v2[3], 1.0f), "P2 defaults z=0,w=1");

    glVertexAttribP1uiv(2, 0x8D9F /*INT_2_10_10_10_REV*/, GL_FALSE, &packed);
    GLint vi[4] = {};
    glGetVertexAttribiv(2, 0x8626, vi);
    Check(vi[0] == -1, "P1 signed 10-bit -1");
    Check(vi[1] == 0 && vi[2] == 0 && vi[3] == 1, "P1 integer defaults");

    glVertexAttribP3ui(3, 0x1234, GL_TRUE, 0);
    Check(glGetError() == GL_INVALID_ENUM, "P3 rejects a non-packed type");

    return gFails ? 1 : 0;
}
