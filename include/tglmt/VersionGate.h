#pragma once
// VersionGate — bảng "entry point nào tồn tại ở (version, profile) nào".
//
// Nguồn chân lý: docs/khronos/gl.xml (Khronos OpenGL-Registry), được sinh ra
// bởi tools/gen_version_gate.py. Semantics đúng theo reg.py PASS 1:
//   <require> (không profile)          → mọi profile
//   <require profile="compatibility">  → chỉ compatibility
//   <remove>  (không profile)          → bỏ khỏi mọi profile
//   <remove profile="core">            → bỏ khỏi core
//   last-write-wins theo version tăng dần.
//
// 19 version slot: 1.0..1.5, 2.0, 2.1, 3.0, 3.1, 3.2, 3.3, 4.0..4.6.
// Mask 64-bit: bit 0..18 = CORE, bit 19..37 = COMPATIBILITY.
// GL 1.0–3.1 chưa có phân tách profile trong registry ⇒ hai nhóm bit bằng nhau
// ở các slot đó (đúng hành vi, không phải lỗi generator).
//
// Extension entry points KHÔNG nằm trong bảng này: chúng được gate bằng
// extension string mà TGLMT advertise (xem gl_error_string_debug.cpp).
#include <cstdint>

namespace tglmt {

// Cap enable CHỈ tồn tại ở Compatibility Profile (sinh tự động từ gl.xml,
// xem tools/gen_compat_tables.py). Ở Core Profile, glEnable/glDisable với cap
// này phải sinh GL_INVALID_ENUM (GL 4.6 Core spec §17.1 + App. E.2.2).
bool IsCompatOnlyEnableCap(unsigned int cap);
// Tên enum của cap (nullptr nếu không phải compat-only) — dùng cho log.
const char* CompatOnlyEnableCapName(unsigned int cap);
int CompatOnlyEnableCapCount();

// Pname (glGet) CHỈ tồn tại ở Compatibility Profile. Bảng sinh tự động từ
// nhóm "GetPName" của gl.xml. Ở Core Profile, glGetIntegerv/glGetFloatv với
// pname này phải sinh GL_INVALID_ENUM (GL 4.6 Core spec §22.1 + App. E.2.2).
bool IsCompatOnlyGetPName(unsigned int pname);
const char* CompatOnlyGetPNameName(unsigned int pname);
int CompatOnlyGetPNameCount();


// ---- Profile của một GL context -------------------------------------------
enum class GlProfile : uint8_t {
    // GL 1.0–3.1: registry không có khái niệm profile ⇒ một bộ API duy nhất.
    Legacy = 0,
    // GL >= 3.2 Core Profile.
    Core = 1,
    // GL >= 3.2 Compatibility Profile.
    Compatibility = 2,
};

// Số version slot (xem VersionSlots()).
constexpr int kGlVersionSlotCount = 19;
// Offset của nhóm bit COMPATIBILITY trong mask.
constexpr int kGlCompatBitBase = kGlVersionSlotCount;

struct VersionGateEntry {
    const char* name;
    uint64_t avail;  // bit 0..18 core, bit 19..37 compatibility
};

extern const VersionGateEntry kVersionGateTable[];
extern const int kVersionGateCount;
// Sức chứa tối đa của bảng gate (dùng để cấp phát bitmap theo context).
constexpr int kVersionGateTableCapacity = 2048;

// "1.0".."4.6" theo đúng thứ tự slot.
const char* const* GlVersionSlots();

// Index của version slot cho (major, minor). -1 nếu ngoài 1.0..4.6.
int GlVersionSlotIndex(int major, int minor);

// Index trong kVersionGateTable của tên lệnh; -1 nếu không biết.
int FindVersionGateIndex(const char* name);

// Lệnh có được phép gọi trong context (versionIndex, profile) không?
inline bool VersionGateAllows(int entryIndex, int versionSlot, GlProfile profile) {
    if (entryIndex < 0 || versionSlot < 0 || versionSlot >= kGlVersionSlotCount)
        return true;  // không biết → cho qua (fail-open, an toàn cho ext)
    const uint64_t m = kVersionGateTable[entryIndex].avail;
    const int bit = (profile == GlProfile::Compatibility) ? versionSlot + kGlCompatBitBase
                                                          : versionSlot;
    return (m >> bit) & 1ULL;
}

// Gate cho extern "C" wrapper: nếu entry point không hợp lệ trong context hiện
// tại ⇒ ghi GL_INVALID_OPERATION vào error queue và trả false (wrapper bỏ qua
// lời gọi). Cài ở src/core/VersionGate.cpp.
bool GateCheck(int entryIndex);

// Bảng tra địa chỉ hàm C ABI cho getProcAddress (sinh ở gl_c_exports.cpp).
struct TglmtProcEntry {
    const char* name;
    void* fn;
};
extern const TglmtProcEntry kTglmtProcTable[];
extern const int kTglmtProcCount;

}  // namespace tglmt
