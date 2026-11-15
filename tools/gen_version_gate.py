#!/usr/bin/env python3
"""Sinh bảng version-gate cho TGLMT từ docs/khronos/gl.xml (OpenGL-Registry).

Bảng này là NGUỒN CHÂN LÝ cho "entry point nào tồn tại trong version/profile nào",
theo đúng semantics của Khronos reg.py PASS 1 (xem tools/gl46_catalog.py):
  * <require> không profile          → mọi profile
  * <require profile="compatibility">→ chỉ compatibility
  * <remove> không profile           → bỏ khỏi mọi profile
  * <remove profile="core">         → bỏ khỏi core
  * <remove profile="compatibility"> → bỏ khỏi compatibility
  * last-write-wins theo thứ tự version tăng dần (một <require> sau sẽ khôi phục
    lệnh mà <remove> trước đó đã bỏ — vd glGetPointerv bị bỏ core ở 3.2, được
    yêu cầu lại core ở 4.3).

Đầu ra:
  src/gl/gl_version_gate_gen.cpp   — bảng C++ (tên + availability mask)
  include/tglmt/VersionGate.h      — KHÔNG ghi (header nằm tay, ổn định)

Availability mask: 1 bit cho mỗi (version, profile). 19 version slot
(1.0..1.5, 2.0, 2.1, 3.0, 3.1, 3.2, 3.3, 4.0..4.6) × 2 profile = 38 bit → vừa
đủ trong uint64_t.
  bit  0..18  : có trong CORE      tại version slot tương ứng
  bit 19..37  : có trong COMPATIBILITY tại version slot tương ứng

GL 1.0–3.1 không có phân tách profile trong registry ⇒ core bit == compat bit
ở các slot đó (đây là hành vi đúng, không phải lỗi của generator).

Usage:
  python3 tools/gen_version_gate.py            # ghi file
  python3 tools/gen_version_gate.py --check    # chỉ kiểm tra, không ghi
"""
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gen_version_inventory as gvi  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "src/gl/gl_version_gate_gen.cpp"

#: Thứ tự version slot — phải khớp với kGlVersionSlots trong
#: include/tglmt/VersionGate.h.
VERSION_SLOTS = [
    "1.0", "1.1", "1.2", "1.3", "1.4", "1.5",
    "2.0", "2.1", "3.0", "3.1", "3.2", "3.3",
    "4.0", "4.1", "4.2", "4.3", "4.4", "4.5", "4.6",
]
assert len(VERSION_SLOTS) == 19, len(VERSION_SLOTS)
CORE_BASE = 0
COMPAT_BASE = len(VERSION_SLOTS)


def build_table():
    """Trả về list[(name, mask)] cho MỌI command từng tồn tại ở GL 1.0..4.6."""
    inv = gvi.build_inventory()
    versions = inv["versions"]
    core_sets = {}
    compat_sets = {}
    for v in VERSION_SLOTS:
        node = versions.get(v)
        if node is None:
            raise SystemExit(f"[gate] thiếu version {v} trong inventory")
        cmds = node["commands"]
        core_sets[v] = set(cmds.get("core", []))
        compat_sets[v] = set(cmds.get("compatibility", []))

    names = sorted(core_sets[VERSION_SLOTS[-1]] | compat_sets[VERSION_SLOTS[-1]])
    # Thêm mọi command từng xuất hiện ở version cũ hơn (đã bị remove sau này).
    for v in VERSION_SLOTS:
        names = sorted(set(names) | core_sets[v] | compat_sets[v])

    rows = []
    for nm in names:
        mask = 0
        for i, v in enumerate(VERSION_SLOTS):
            if nm in core_sets[v]:
                mask |= 1 << (CORE_BASE + i)
            if nm in compat_sets[v]:
                mask |= 1 << (COMPAT_BASE + i)
        if mask == 0:
            continue
        rows.append((nm, mask))
    return rows, core_sets, compat_sets


def main():
    check = "--check" in sys.argv
    rows, core_sets, compat_sets = build_table()

    # Self-check: số lệnh của GL 4.6 phải khớp registry.
    core46 = len(core_sets["4.6"])
    compat46 = len(compat_sets["4.6"])
    if core46 != 657:
        raise SystemExit(f"[gate] FATAL: GL4.6 core = {core46}, kỳ vọng 657")
    if compat46 != 1048:
        raise SystemExit(f"[gate] FATAL: GL4.6 compat = {compat46}, kỳ vọng 1048")
    print(f"[gate] commands: {len(rows)} | GL4.6 core={core46} compat={compat46}")

    if check:
        return

    OUT.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT, "w") as f:
        f.write("// AUTO-GENERATED bởi tools/gen_version_gate.py — KHÔNG SỬA TAY.\n")
        f.write("// Availability mask: bit 0..18 = CORE (version slot 1.0..4.6),\n")
        f.write("//                   bit 19..37 = COMPATIBILITY (cùng thứ tự version slot).\n")
        f.write("// Nguồn: docs/khronos/gl.xml (Khronos OpenGL-Registry), semantics reg.py PASS 1.\n")
        f.write('#include "tglmt/VersionGate.h"\n')
        f.write("namespace tglmt {\n")
        f.write("const VersionGateEntry kVersionGateTable[] = {\n")
        for nm, mask in rows:
            f.write(f'    {{"{nm}", 0x{mask:016X}ULL}},\n')
        f.write("};\n")
        f.write(f"const int kVersionGateCount = {len(rows)};\n")
        f.write("} // namespace tglmt\n")
    print(f"[gate] wrote {OUT} ({len(rows)} entries)")


if __name__ == "__main__":
    main()
