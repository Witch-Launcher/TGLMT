// tglmt/uniform_pack.h — đóng gói dữ liệu uniform GLSL → layout byte MSL.
//
// Vấn đề nền (GL 4.6 §7.6.1 vs Metal):
//   * GL `glUniformMatrix3fv` nhận 9 float column-major CHẶT = 36 byte.
//   * MSL `float3x3` trong `constant` struct có 3 cột `float3`, mỗi cột
//     stride 16 byte (float3 trong MSL có size 16, align 16) → 48 byte.
//   → Copy thẳng 36 byte vào offset của struct ⇒ cột 1 và 2 lệch 4/8 byte
//     ⇒ ma trận đọc ra là rác.
//
// Thêm nữa, `transpose=GL_TRUE` nghĩa là app gửi dữ liệu ROW-major; GL yêu
// cầu implementation phải transpose về column-major trước khi lưu.
//
// File này gom hai việc đó vào một chỗ để cả draw path (uniform struct) và
// tess path dùng chung — tránh trường hợp một đường sửa còn đường kia quên.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace tglmt {

// Số cột/hàng của kiểu ma trận GLSL ("mat3", "mat2x4", ...).
// Trả về false nếu không phải ma trận.
inline bool GlslMatrixDims(const std::string& t, int& cols, int& rows) {
    if (t.size() < 4 || t.compare(0, 3, "mat") != 0) return false;
    size_t x = t.find('x');
    if (x == std::string::npos) {
        // mat2 / mat3 / mat4 → vuông.
        if (t.size() != 4) return false;
        int n = t[3] - '0';
        if (n < 2 || n > 4) return false;
        cols = rows = n;
        return true;
    }
    if (t.size() != 6) return false;
    int c = t[3] - '0', r = t[5] - '0';
    if (c < 2 || c > 4 || r < 2 || r > 4) return false;
    cols = c;
    rows = r;
    return true;
}

// Số byte MSL của một cột ma trận: MSL floatCxR = C cột × R hàng, mỗi cột
// là 1 vector R phần tử (float2 → 8B, float3/float4 → 16B).
// Dùng chung cho cả GLSL type ("mat3x2") và MSL type ("float3x2") để
// converter (src/gl/glsl_to_msl.cpp) và upload path (src/gl/gl_apple_draw.cpp)
// không lệch nhau.
inline size_t MslMatrixColumnStrideForRows(int rows) {
    return rows == 2 ? 8 : 16; // rows 3 hoặc 4 → 16
}

// Kích thước byte MSL của cả ma trận (đã tính padding theo cột).
inline size_t MslMatrixSizeFor(int cols, int rows) {
    return MslMatrixColumnStrideForRows(rows) * (size_t)cols;
}

// Số byte MSL của một cột ma trận.
inline size_t MslMatrixColumnStride(const std::string& t) {
    int cols = 0, rows = 0;
    if (!GlslMatrixDims(t, cols, rows)) return 0;
    return MslMatrixColumnStrideForRows(rows);
}

// Kích thước byte MSL của cả ma trận (đã tính padding theo cột).
inline size_t MslMatrixSize(const std::string& t) {
    int cols = 0, rows = 0;
    if (!GlslMatrixDims(t, cols, rows)) return 0;
    return MslMatrixSizeFor(cols, rows);
}

// Chuyển dữ liệu ma trận GL (column-major chặt) → layout MSL (cột stride 16).
// `dst` phải đủ MslMatrixSize(glslType) byte. `src` là count ma trận liên tiếp
// theo column-major chặt.
// Trả về false nếu kiểu không phải ma trận (caller copy thẳng).
inline bool PackGlslMatrixToMsl(const std::string& glslType, const void* src, size_t srcBytes,
                                 void* dst, size_t dstBytes) {
    int cols = 0, rows = 0;
    if (!GlslMatrixDims(glslType, cols, rows)) return false;
    const size_t comps = (size_t)cols * (size_t)rows;      // số float mỗi ma trận
    const size_t tight = comps * sizeof(float);            // byte GL chặt
    const size_t colStride = MslMatrixColumnStride(glslType);
    const size_t mslSize = colStride * (size_t)cols;
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dst;
    if (srcBytes < tight || dstBytes < mslSize) return true; // không đủ dữ liệu → giữ nguyên
    // Chỉ 1 ma trận: caller gọi cho từng phần tử.
    const float* m = (const float*)s;
    for (int c = 0; c < cols; ++c) {
        memcpy(d + (size_t)c * colStride, m + (size_t)c * (size_t)rows, (size_t)rows * sizeof(float));
    }
    return true;
}

// Transpose ma trận column-major chặt (GL) — dùng khi app truyền
// transpose=GL_TRUE (dữ liệu row-major). Ghi kết quả column-major chặt.
// `dst` phải đủ tight bytes.
inline void TransposeGlslMatrix(const void* src, void* dst, int cols, int rows) {
    const float* s = (const float*)src;
    float* d = (float*)dst;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            d[(size_t)c * (size_t)rows + (size_t)r] = s[(size_t)r * (size_t)cols + (size_t)c];
}

} // namespace tglmt
