// VersionGate — runtime check "entry point nào hợp lệ ở (version, profile) nào".
//
// Bảng dữ liệu: src/gl/gl_version_gate_gen.cpp (sinh từ docs/khronos/gl.xml).
// Semantics: xem include/tglmt/VersionGate.h.
//
// Quy tắc lỗi (GL 4.6 Core spec §2.3, Table 23.1): gọi một entry point không
// có trong context hiện tại ⇒ GL_INVALID_OPERATION (0x0502).
#include "tglmt/VersionGate.h"
#include "tglmt/Context.h"
#include <cstring>

namespace tglmt {

static const char* const kVersionSlotNames[] = {
    "1.0", "1.1", "1.2", "1.3", "1.4", "1.5",
    "2.0", "2.1", "3.0", "3.1", "3.2", "3.3",
    "4.0", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6",
};
static_assert(sizeof(kVersionSlotNames) / sizeof(kVersionSlotNames[0]) ==
                  kGlVersionSlotCount,
              "kVersionSlotNames phải khớp kGlVersionSlotCount");

const char* const* GlVersionSlots() { return kVersionSlotNames; }

int GlVersionSlotIndex(int major, int minor) {
    if (major < 1 || major > 4) return -1;
    if (minor < 0 || minor > 9) return -1;
    // GL chỉ có các minor sau: 1.0–1.5, 2.0, 2.1, 3.0–3.3, 4.0–4.6.
    switch (major) {
        case 1: if (minor > 5) return -1; break;
        case 2: if (minor > 1) return -1; break;
        case 3: if (minor > 3) return -1; break;
        case 4: if (minor > 6) return -1; break;
        default: return -1;
    }
    for (int i = 0; i < kGlVersionSlotCount; ++i) {
        const char* s = kVersionSlotNames[i];
        int mj = 0, mn = 0;
        const char* dot = std::strchr(s, '.');
        if (!dot) continue;
        for (const char* p = s; p < dot; ++p) mj = mj * 10 + (*p - '0');
        for (const char* p = dot + 1; *p; ++p) mn = mn * 10 + (*p - '0');
        if (mj == major && mn == minor) return i;
    }
    return -1;
}

// Bảng tên đã sort ⇒ binary search O(log n), không cần hash map.
int FindVersionGateIndex(const char* name) {
    if (!name) return -1;
    int lo = 0, hi = kVersionGateCount - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = std::strcmp(name, kVersionGateTable[mid].name);
        if (c == 0) return mid;
        if (c < 0) hi = mid - 1;
        else lo = mid + 1;
    }
    return -1;
}

bool GateCheck(int entryIndex) {
    Context* c = Context::CurrentPtr();
    if (!c) return true;            // chưa có context → để hàm thật báo lỗi
    if (entryIndex < 0 || entryIndex >= kVersionGateCount) return true;
    if ((c->glGateMask[entryIndex >> 6] >> (entryIndex & 63)) & 1ULL) return true;
    c->errors.Record(0x0502);        // GL_INVALID_OPERATION
    return false;
}

}  // namespace tglmt
