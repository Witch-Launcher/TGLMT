// test_logic_op_state.cpp — glLogicOp / GL_COLOR_LOGIC_OP state round-trip.
// GL 4.6: GL_LOGIC_OP (0x0BF1) query được; GL_COLOR_LOGIC_OP (0x0BF2) là enable;
// khi COLOR_LOGIC_OP on thì blend bị bypass (không test GPU ở đây — state only).
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstdio>

using namespace tglmt;
using namespace tglmt::gl;

static int gFails = 0;
static void Check(bool ok, const char* msg) {
    if (!ok) { printf("test_logic_op_state FAIL: %s\n", msg); ++gFails; }
}

int main() {
    Context ctx("null");
    Context::MakeCurrent(&ctx);
    while (glGetError() != GL_NO_ERROR) {}

    // Default: COLOR_LOGIC_OP off, LOGIC_OP = COPY (0x1503).
    GLint en = -1;
    glGetIntegerv(0x0BF2 /*COLOR_LOGIC_OP*/, &en);
    Check(en == 0, "COLOR_LOGIC_OP default off");
    GLint op = -1;
    glGetIntegerv(0x0BF1 /*LOGIC_OP*/, &op);
    Check(op == 0x1503, "LOGIC_OP default COPY");

    // Round-trip enable.
    glEnable(GL_COLOR_LOGIC_OP);
    Check(glGetError() == GL_NO_ERROR, "glEnable(COLOR_LOGIC_OP)");
    Check(glIsEnabled(GL_COLOR_LOGIC_OP) == GL_TRUE, "glIsEnabled(COLOR_LOGIC_OP)");
    glGetIntegerv(0x0BF2, &en);
    Check(en == 1, "glGetIntegerv(COLOR_LOGIC_OP) after enable");
    GLboolean ben = 0;
    glGetBooleanv(0x0BF2, &ben);
    Check(ben == GL_TRUE, "glGetBooleanv(COLOR_LOGIC_OP) after enable");

    // Round-trip logic op value.
    glLogicOp(GL_XOR);
    Check(glGetError() == GL_NO_ERROR, "glLogicOp(XOR)");
    glGetIntegerv(0x0BF1, &op);
    Check(op == 0x1506 /*XOR*/, "glGetIntegerv(LOGIC_OP) XOR");

    glLogicOp(GL_CLEAR);
    glGetIntegerv(0x0BF1, &op);
    Check(op == 0x1500 /*CLEAR*/, "glGetIntegerv(LOGIC_OP) CLEAR");

    glDisable(GL_COLOR_LOGIC_OP);
    Check(glIsEnabled(GL_COLOR_LOGIC_OP) == GL_FALSE, "glDisable(COLOR_LOGIC_OP)");
    glGetIntegerv(0x0BF2, &en);
    Check(en == 0, "glGetIntegerv(COLOR_LOGIC_OP) after disable");

    // Invalid opcode → GL_INVALID_VALUE (spec §17.3.9), state unchanged.
    while (glGetError() != GL_NO_ERROR) {}
    glLogicOp(0xDEAD);
    Check(glGetError() == GL_INVALID_VALUE, "glLogicOp invalid enum");
    glGetIntegerv(0x0BF1, &op);
    Check(op == 0x1500 /*CLEAR — last valid*/, "LOGIC_OP unchanged after invalid");

    if (gFails == 0) { printf("test_logic_op_state: OK\n"); return 0; }
    return 1;
}
