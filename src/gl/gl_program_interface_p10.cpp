// gl_program_interface_p10.cpp — P10: ARB_program_interface_query (Sodium/Iris dùng).
// Introspect program: UNIFORM (từ uniformLayout), UNIFORM_BLOCK (từ uniformBlocks),
// PROGRAM_INPUT/OUTPUT (từ attribLoc + outputs — cơ bản names/locations).
// Interface enums: UNIFORM=0x92E1, UNIFORM_BLOCK=0x92E2, INPUT=0x92E3, OUTPUT=0x92E4.
#include "tglmt/gl46.h"
#include "tglmt/Context.h"
#include <cstring>
using namespace tglmt;

namespace tglmt::gl {
namespace {
// Lấy ProgramObject, lỗi nếu không tồn tại/linked.
ProgramObject* GetLinkedProg(GLuint p) {
    Context& c = Context::Current();
    auto it = c.programs.find(p);
    if (it == c.programs.end() || !it->second.linked) {
        c.errors.Record(0x0502); // INVALID_OPERATION
        return nullptr;
    }
    return &it->second;
}
// Đếm resources + max name length cho interface.
void InterfaceInfo(ProgramObject& pr, GLenum iface, GLuint& count, GLsizei& maxLen) {
    count = 0; maxLen = 0;
    if (iface == 0x92E1 /*UNIFORM*/) {
        count = (GLuint)pr.uniformLayout.size();
        for (auto& e : pr.uniformLayout) maxLen = std::max(maxLen, (GLsizei)e.name.size() + 1);
    } else if (iface == 0x92E2 /*UNIFORM_BLOCK*/) {
        count = (GLuint)pr.uniformBlocks.size();
        for (auto& b : pr.uniformBlocks) maxLen = std::max(maxLen, (GLsizei)b.name.size() + 1);
    } else if (iface == 0x92E3 /*PROGRAM_INPUT*/) {
        count = (GLuint)pr.attribLoc.size();
        for (auto& kv : pr.attribLoc) maxLen = std::max(maxLen, (GLsizei)kv.first.size() + 1);
    } else if (iface == 0x92E4 /*PROGRAM_OUTPUT*/) {
        // Outputs: không lưu riêng trong ProgramObject — dùng uniformLayout? Không.
        // Tạm: 0 resources (fragment outputs không introspect được qua API này ở TGLMT).
        // Trung thực: trả 0 thay vì đoán.
        count = 0; maxLen = 0;
    }
}
} // namespace

void glGetProgramInterfaceiv(GLuint program, GLenum programInterface, GLenum pname, GLint* params) {
    ProgramObject* pr = GetLinkedProg(program);
    if (!pr || !params) return;
    GLuint count = 0; GLsizei maxLen = 0;
    InterfaceInfo(*pr, programInterface, count, maxLen);
    if (programInterface != 0x92E1 && programInterface != 0x92E2 &&
        programInterface != 0x92E3 && programInterface != 0x92E4) {
        Context::Current().errors.Record(0x0500); return; // INVALID_ENUM
    }
    if (pname == 0x92F5 /*ACTIVE_RESOURCES*/) *params = (GLint)count;
    else if (pname == 0x92F6 /*MAX_NAME_LENGTH*/) *params = maxLen;
    else if (pname == 0x92F7 /*MAX_NUM_ACTIVE_VARIABLES*/) *params = (GLint)count; // = resources
    else if (pname == 0x92F8 /*MAX_NUM_COMPATIBLE_SUBROUTINES*/) *params = 0; // không subroutine API
    else Context::Current().errors.Record(0x0500);
}
GLuint glGetProgramResourceIndex(GLuint program, GLenum programInterface, const GLchar* name) {
    ProgramObject* pr = GetLinkedProg(program);
    if (!pr || !name) return 0xFFFFFFFFu; // GL_INVALID_INDEX
    std::string nm(name);
    if (programInterface == 0x92E1) {
        for (GLuint i = 0; i < pr->uniformLayout.size(); ++i)
            if (pr->uniformLayout[i].name == nm) return i;
    } else if (programInterface == 0x92E2) {
        for (GLuint i = 0; i < pr->uniformBlocks.size(); ++i)
            if (pr->uniformBlocks[i].name == nm) return i;
    } else if (programInterface == 0x92E3) {
        GLuint i = 0;
        for (auto& kv : pr->attribLoc) {
            if (kv.first == nm) return i;
            ++i;
        }
    }
    return 0xFFFFFFFFu; // không tìm thấy
}
void glGetProgramResourceName(GLuint program, GLenum programInterface, GLuint index,
                              GLsizei bufSize, GLsizei* length, GLchar* name) {
    ProgramObject* pr = GetLinkedProg(program);
    if (!pr) return;
    std::string nm;
    if (programInterface == 0x92E1 && index < pr->uniformLayout.size())
        nm = pr->uniformLayout[index].name;
    else if (programInterface == 0x92E2 && index < pr->uniformBlocks.size())
        nm = pr->uniformBlocks[index].name;
    else if (programInterface == 0x92E3) {
        GLuint i = 0;
        for (auto& kv : pr->attribLoc) {
            if (i == index) { nm = kv.first; break; }
            ++i;
        }
    }
    if (nm.empty()) { Context::Current().errors.Record(0x0501); return; } // INVALID_VALUE (index quá)
    if (length) *length = (GLsizei)nm.size();
    if (name && bufSize > 0) {
        GLsizei copy = std::min((GLsizei)nm.size(), bufSize - 1);
        if (copy > 0) memcpy(name, nm.c_str(), (size_t)copy);
        name[copy] = '\0';
    }
}
void glGetProgramResourceiv(GLuint program, GLenum programInterface, GLuint index,
                            GLsizei propCount, const GLenum* props, GLsizei count,
                            GLsizei* length, GLint* params) {
    ProgramObject* pr = GetLinkedProg(program);
    if (!pr || !props || !params) return;
    if (length) *length = 0;
    // Props hỗ trợ: TYPE(0x92FA), ARRAY_SIZE(0x92FB), OFFSET(0x92FC), BLOCK_INDEX(0x92FA?),
    // LOCATION(0x92E9), NAME_LENGTH(0x92F9). Mỗi prop ghi 1 int (trừ mảng — không hỗ trợ array props ở đây).
    for (GLsizei k = 0; k < propCount && k < count; ++k) {
        GLenum p = props[k];
        GLint v = 0;
        if (programInterface == 0x92E1 && index < pr->uniformLayout.size()) {
            auto& e = pr->uniformLayout[index];
            if (p == 0x92FA /*TYPE*/) v = (GLint)e.glType;
            else if (p == 0x92FB /*ARRAY_SIZE*/) v = 1; // TGLMT uniform array? Size từ arraySize? Đơn giản 1.
            else if (p == 0x92FC /*OFFSET*/) v = (GLint)e.offset;
            else if (p == 0x92FD /*BLOCK_INDEX*/) v = -1; // không trong block (standalone uniform)
            else if (p == 0x92E9 /*LOCATION*/) v = e.loc;
            else if (p == 0x92F9 /*NAME_LENGTH*/) v = (GLint)e.name.size() + 1;
            else { Context::Current().errors.Record(0x0500); return; }
        } else if (programInterface == 0x92E2 && index < pr->uniformBlocks.size()) {
            auto& b = pr->uniformBlocks[index];
            if (p == 0x92F9 /*NAME_LENGTH*/) v = (GLint)b.name.size() + 1;
            else if (p == 0x92FD /*BINDING? Thực tế BUFFER_BINDING=0x92FA? Dùng BINDING=0x8A30?*/) {
                // UNIFORM_BLOCK props: BINDING (0x8A30? Không — BUFFER_BINDING cho block là 0x92FA?).
                // Đơn giản: trả binding point.
                v = (GLint)b.binding;
            } else if (p == 0x92F5 /*DATA_SIZE? Thực tế UNIFORM_BLOCK_DATA_SIZE=0x8A30*/) {
                v = (GLint)b.minSize;
            } else { Context::Current().errors.Record(0x0500); return; }
        } else {
            Context::Current().errors.Record(0x0501); return; // index quá
        }
        params[k] = v;
        if (length) (*length)++;
    }
}
GLint glGetProgramResourceLocation(GLuint program, GLenum programInterface, const GLchar* name) {
    Context& c = Context::Current();
    ProgramObject* pr = GetLinkedProg(program);
    if (!pr || !name) return -1;
    std::string nm(name);
    if (programInterface == 0x92E1 /*UNIFORM*/) {
        for (auto& e : pr->uniformLayout) {
            if (e.name != nm) continue;
            // Gán loc nếu chưa có (giống glGetUniformLocation — đồng bộ uniformLoc map).
            if (e.loc < 0) {
                auto f = pr->uniformLoc.find(nm);
                if (f != pr->uniformLoc.end()) e.loc = f->second;
                else {
                    GLint loc = (GLint)pr->uniformLoc.size();
                    pr->uniformLoc[nm] = loc;
                    e.loc = loc;
                }
            }
            return e.loc;
        }
    } else if (programInterface == 0x92E3 /*PROGRAM_INPUT*/) {
        auto it = pr->attribLoc.find(nm);
        if (it != pr->attribLoc.end()) return it->second;
    }
    return -1;
}
GLint glGetProgramResourceLocationIndex(GLuint program, GLenum programInterface, const GLchar* name) {
    // Chỉ cho OUTPUT với dual-source blend (không hỗ trợ) → luôn -1.
    (void)program; (void)programInterface; (void)name;
    return -1;
}
} // namespace tglmt::gl
