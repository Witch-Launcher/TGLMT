// tglmt/glsl_type.h — ánh xạ kiểu GLSL ↔ GL enum (nguồn duy nhất).
//
// Trước đây bảng ánh xạ nằm riêng trong src/gl/gl_program.cpp (static) và
// src/gl/gl_get_extra.cpp, nên validation uniform (§7.6.1) không dùng được.
// Bảng này phải khớp CHÍNH XÁC với bảng trong GL 4.6 §7.2 (Type Values).
#pragma once

#include <cstdint>
#include <string>

namespace tglmt {

inline uint32_t GlslTypeToGLEnum(const std::string& t) {
    if (t == "float") return 0x1406;
    // Double (GL 4.6 §7.2): GL_DOUBLE = 0x140A, GL_DOUBLE_VEC2/3/4 = 0x8FFC..0x8FFE.
    // Metal không có kiểu double → shader emu sang float (như ANGLE trên Metal),
    // nhưng PHẢI phản ánh đúng kiểu GL cho validation uniform (§7.6.1).
    if (t == "double") return 0x140A;
    if (t == "dvec2") return 0x8FFC;
    if (t == "dvec3") return 0x8FFD;
    if (t == "dvec4") return 0x8FFE;
    if (t == "vec2") return 0x8B50;
    if (t == "vec3") return 0x8B51;
    if (t == "vec4") return 0x8B52;
    if (t == "int") return 0x1404;
    if (t == "ivec2") return 0x8B53;
    if (t == "ivec3") return 0x8B54;
    if (t == "ivec4") return 0x8B55;
    if (t == "uint") return 0x1405;
    if (t == "uvec2") return 0x8DC6;
    if (t == "uvec3") return 0x8DC7;
    if (t == "uvec4") return 0x8DC8;
    if (t == "bool") return 0x8B56;
    if (t == "bvec2") return 0x8B57;
    if (t == "bvec3") return 0x8B58;
    if (t == "bvec4") return 0x8B59;
    if (t == "bvec2") return 0x8B57;
    if (t == "bvec3") return 0x8B58;
    if (t == "bvec4") return 0x8B59;
    if (t == "mat2") return 0x8B5A;
    if (t == "mat3") return 0x8B5B;
    if (t == "mat4") return 0x8B5C;
    // Sampler (GL_ACTIVE_UNIFORMS reflection — OpenGL spec xóa phần "Shadow"
    // khỏi tên type nhưng pack khai đúng GLSL).
    if (t == "sampler1D") return 0x8B5D;
    if (t == "sampler2D") return 0x8B5E;
    if (t == "sampler3D") return 0x8B5F;
    if (t == "samplerCube") return 0x8B60;
    if (t == "sampler1DShadow") return 0x8B61;
    if (t == "sampler2DShadow") return 0x8B62;
    if (t == "sampler2DRect") return 0x8B63;
    if (t == "sampler2DRectShadow") return 0x8B64;
    if (t == "sampler1DArray") return 0x8DC0;
    if (t == "sampler2DArray") return 0x8DC1;
    if (t == "samplerBuffer") return 0x8DC2;
    if (t == "sampler1DArrayShadow") return 0x8DC3;
    if (t == "sampler2DArrayShadow") return 0x8DC4;
    if (t == "samplerCubeShadow") return 0x8DC5;
    if (t == "sampler2DMS") return 0x9108;
    if (t == "sampler2DMSArray") return 0x9109;
    if (t == "isampler1D") return 0x8DC9;
    if (t == "isampler2D") return 0x8DCA;
    if (t == "isampler3D") return 0x8DCB;
    if (t == "isamplerCube") return 0x8DCC;
    if (t == "isampler2DRect") return 0x8DCD;
    if (t == "isampler1DArray") return 0x8DCE;
    if (t == "isampler2DArray") return 0x8DCF;
    if (t == "isamplerBuffer") return 0x8DD0;
    if (t == "usampler1D") return 0x8DD1;
    if (t == "usampler2D") return 0x8DD2;
    if (t == "usampler3D") return 0x8DD3;
    if (t == "usamplerCube") return 0x8DD4;
    if (t == "usampler2DRect") return 0x8DD5;
    if (t == "usampler1DArray") return 0x8DD6;
    if (t == "usampler2DArray") return 0x8DD7;
    if (t == "usamplerBuffer") return 0x8DD8;
    return 0; // unknown
}

// Số thành phần của một kiểu uniform (1..4). Ma trận → 0 (dùng NxM riêng).
inline int UniformComponentCount(uint32_t glType) {
    switch (glType) {
        case 0x1406: case 0x1404: case 0x1405: case 0x8B56: return 1;  // FLOAT/INT/UINT/BOOL
        case 0x8B50: case 0x8B53: case 0x8DC6: case 0x8B57: return 2;  // *_VEC2
        case 0x8B51: case 0x8B54: case 0x8DC7: case 0x8B58: return 3;  // *_VEC3
        case 0x8B52: case 0x8B55: case 0x8DC8: case 0x8B59: return 4;  // *_VEC4
        default: return 0;
    }
}

// Kiểu bool (GL_BOOL, GL_BOOL_VECn): GL 4.6 §7.6.1 — uniform bool khớp với
// MỌI lệnh Uniform*i{v} / Uniform*ui{v} / Uniform*f{v}.
inline bool UniformTypeIsBool(uint32_t glType) {
    return glType == 0x8B56 || glType == 0x8B57 || glType == 0x8B58 || glType == 0x8B59;
}

// Kiểu ma trận float (GL_FLOAT_MATn / GL_FLOAT_MATnxm) → (cols, rows).
inline bool UniformTypeIsMatrix(uint32_t glType, int& cols, int& rows) {
    switch (glType) {
        case 0x8B5A: cols = 2; rows = 2; return true;  // FLOAT_MAT2
        case 0x8B5B: cols = 3; rows = 3; return true;  // FLOAT_MAT3
        case 0x8B5C: cols = 4; rows = 4; return true;  // FLOAT_MAT4
        case 0x8B65: cols = 2; rows = 3; return true;  // FLOAT_MAT2x3
        case 0x8B66: cols = 2; rows = 4; return true;  // FLOAT_MAT2x4
        case 0x8B67: cols = 3; rows = 2; return true;  // FLOAT_MAT3x2
        case 0x8B68: cols = 3; rows = 4; return true;  // FLOAT_MAT3x4
        case 0x8B69: cols = 4; rows = 2; return true;  // FLOAT_MAT4x2
        case 0x8B6A: cols = 4; rows = 3; return true;  // FLOAT_MAT4x3
        default: return false;
    }
}

// Kiểu double (GL_DOUBLE, GL_DOUBLE_VECn) — GL 4.6 core không có double matrix.
inline bool UniformTypeIsDouble(uint32_t glType) {
    switch (glType) {
        case 0x140A: case 0x8FFC: case 0x8FFD: case 0x8FFE: return true;
        default: return false;
    }
}

// Kiểu sampler (GL 4.6 §7.2): không thể nạp bằng lệnh uniform thường —
// "a sampler or image uniform is loaded with any of the Uniform* commands other
// than Uniform1i{v}" → INVALID_OPERATION.
inline bool UniformTypeIsSampler(uint32_t glType) {
    switch (glType) {
        // float sampler
        case 0x8B5D: case 0x8B5E: case 0x8B5F: case 0x8B60:
        case 0x8B61: case 0x8B62: case 0x8B63: case 0x8B64:
        case 0x8DC0: case 0x8DC1: case 0x8DC2: case 0x8DC3: case 0x8DC4: case 0x8DC5:
        case 0x9108: case 0x910B:
        // int / uint sampler
        case 0x8DC9: case 0x8DCA: case 0x8DCB: case 0x8DCC: case 0x8DCD:
        case 0x8DCE: case 0x8DCF: case 0x8DD0:
        case 0x8DD1: case 0x8DD2: case 0x8DD3: case 0x8DD4: case 0x8DD5:
        case 0x8DD6: case 0x8DD7: case 0x8DD8:
        case 0x9109: case 0x910A: case 0x910C: case 0x910D:
            return true;
        default: return false;
    }
}

// Kiểu image (GL 4.6 §7.2): chỉ nạp được bằng Uniform1i{v}.
inline bool UniformTypeIsImage(uint32_t glType) {
    switch (glType) {
        case 0x904C: case 0x904D: case 0x904E: case 0x904F: case 0x9050:
        case 0x9051: case 0x9052: case 0x9053: case 0x9054:
        case 0x9055: case 0x9056:
        case 0x9057: case 0x9058: case 0x9059: case 0x905A: case 0x905B:
        case 0x905C: case 0x905D: case 0x905E: case 0x905F: case 0x9060:
        case 0x9061:
            return true;
        default: return false;
    }
}


// Kiểu gốc của một kiểu uniform: GL_FLOAT / GL_INT / GL_UNSIGNED_INT /
// GL_DOUBLE / GL_BOOL. Ma trận float → GL_FLOAT.
inline uint32_t UniformBaseType(uint32_t glType) {
    int c = 0, r = 0;
    if (UniformTypeIsMatrix(glType, c, r)) return 0x1406;  // GL_FLOAT
    switch (glType) {
        case 0x1406: case 0x8B50: case 0x8B51: case 0x8B52: return 0x1406;
        case 0x1404: case 0x8B53: case 0x8B54: case 0x8B55: return 0x1404;
        case 0x1405: case 0x8DC6: case 0x8DC7: case 0x8DC8: return 0x1405;
        case 0x140A: case 0x8FFC: case 0x8FFD: case 0x8FFE: return 0x140A;
        case 0x8B56: case 0x8B57: case 0x8B58: case 0x8B59: return 0x8B56;
        default: return 0;
    }
}

} // namespace tglmt
