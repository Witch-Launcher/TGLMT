// test_logic_op_state.cpp — glLogicOp / GL_COLOR_LOGIC_OP state round-trip.
//
// GL 4.6 Core Profile: pname hợp lệ để đọc logic op là GL_LOGIC_OP_MODE
// (0x0BF0) — xem glspec46.core.pdf bảng state p.625 ("LOGIC OP MODE  E
// GetIntegerv  COPY"). GL_LOGIC_OP (0x0BF1) là alias CŨ của GL 1.x và ĐÃ
// BỊ GỠ khỏi Core Profile từ 3.2 (registry docs/khronos/gl.xml: <remove
// profile="core"> trong feature GL_VERSION_3_2 liệt kê GL_LOGIC_OP).
// ⇒ ở Core Profile, glGetIntegerv(GL_LOGIC_OP) phải sinh GL_INVALID_ENUM;
//   ở Compatibility Profile, GL_LOGIC_OP vẫn hợp lệ (alias của MODE).
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
    glGetIntegerv(0x0BF0 /*GL_LOGIC_OP_MODE — pname Core 4.6*/, &op);
    Check(glGetError() == GL_NO_ERROR, "glGetIntegerv(LOGIC_OP_MODE) no error");
    Check(op == 0x1503, "LOGIC_OP_MODE default COPY");

    // GL_LOGIC_OP (0x0BF1) KHÔNG tồn tại ở Core Profile ⇒ INVALID_ENUM.
    while (glGetError() != GL_NO_ERROR) {}
    GLint legacyOp = -1;
    glGetIntegerv(0x0BF1 /*GL_LOGIC_OP — compat-only*/, &legacyOp);
    Check(glGetError() == GL_INVALID_ENUM,
          "glGetIntegerv(GL_LOGIC_OP) phải INVALID_ENUM ở Core Profile");

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
    glGetIntegerv(0x0BF0, &op);
    Check(op == 0x1506 /*XOR*/, "glGetIntegerv(LOGIC_OP_MODE) XOR");

    glLogicOp(GL_CLEAR);
    glGetIntegerv(0x0BF0, &op);
    Check(op == 0x1500 /*CLEAR*/, "glGetIntegerv(LOGIC_OP_MODE) CLEAR");

    glDisable(GL_COLOR_LOGIC_OP);
    Check(glIsEnabled(GL_COLOR_LOGIC_OP) == GL_FALSE, "glDisable(COLOR_LOGIC_OP)");
    glGetIntegerv(0x0BF2, &en);
    Check(en == 0, "glGetIntegerv(COLOR_LOGIC_OP) after disable");

    // Invalid opcode → GL_INVALID_VALUE (spec §17.3.9), state unchanged.
    while (glGetError() != GL_NO_ERROR) {}
    glLogicOp(0xDEAD);
    Check(glGetError() == GL_INVALID_VALUE, "glLogicOp invalid enum");
    glGetIntegerv(0x0BF0, &op);
    Check(op == 0x1500 /*CLEAR — last valid*/, "LOGIC_OP_MODE unchanged after invalid");

    if (gFails == 0) { printf("test_logic_op_state: OK\n"); return 0; }
    return 1;
}
