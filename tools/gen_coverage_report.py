#!/usr/bin/env python3
"""Sinh báo cáo coverage API (version × profile) — số đo, không viết tay.

Nguồn chân lý:
  * `data/version_inventory.json` — sinh bởi `tools/gen_version_inventory.py`
    từ `docs/khronos/gl.xml` (registry Khronos).
  * `src/gl/gl_stubs_gen.cpp` — danh sách lệnh **còn stub** (sinh bởi
    `tools/gen_stubs.py`).

Cách đo: một lệnh "đã implement" khi KHÔNG còn nằm trong danh sách stub.
Một lệnh "có ở version V" khi `introduced_version <= V` và (với compatibility)
chưa bị gỡ khỏi compatibility ở version <= V.

Dùng:
    python3 tools/gen_coverage_report.py [--out reports/API_COVERAGE_REPORT.md]
                                          [--ctest-log /tmp/ctest_v.log]
"""
import argparse
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
INV = ROOT / "data" / "version_inventory.json"
STUBS = ROOT / "src" / "gl" / "gl_stubs_gen.cpp"

VERSIONS = [(1, 0), (1, 1), (1, 2), (1, 3), (1, 4), (1, 5), (2, 0), (2, 1),
            (3, 0), (3, 1), (3, 2), (3, 3), (4, 0), (4, 1), (4, 2), (4, 3),
            (4, 4), (4, 5), (4, 6)]


def vtuple(s):
    return tuple(int(x) for x in str(s).split(".")[:2])


def load():
    inv = json.loads(INV.read_text())
    cmds = inv["commands"]
    stub_src = STUBS.read_text()
    stubs = set(re.findall(r"\b(gl[A-Za-z0-9_]+)\s*\(", stub_src))
    return cmds, stubs


def core_at(cmds, v):
    out = []
    for name, m in cmds.items():
        iv = m.get("introduced_version")
        if not m.get("core_available") or not iv:
            continue
        if vtuple(iv) <= v:
            out.append(name)
    return out


def compat_at(cmds, v):
    out = []
    for name, m in cmds.items():
        iv = m.get("introduced_version")
        if not iv:
            continue
        if vtuple(iv) > v:
            continue
        removed = m.get("removed_in_compatibility_at") or []
        if any(vtuple(r) <= v for r in removed):
            continue
        out.append(name)
    return out


def ctest_stats(log: pathlib.Path):
    """(tổng, pass, fail, skip) từ log `ctest -V`."""
    if not log.exists():
        return None
    txt = log.read_text(errors="replace")
    total = passed = failed = skipped = 0
    # Format: "  1/207 Test #133: name ...................   Passed    0.26 sec"
    for m in re.finditer(r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(\S+)\s+\.+"
                         r"\s*(\*\*\*Failed|\*\*\*Skipped|\*\*\*Timeout|"
                         r"\*\*\*Exception|Passed)", txt, re.M):
        total += 1
        st = m.group(2)
        if st == "Passed":
            passed += 1
        elif st.startswith("***Failed") or st.startswith("***Timeout") or st.startswith("***Exception"):
            failed += 1
        elif st.startswith("***Skipped"):
            skipped += 1
    return (total, passed, failed, skipped)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(ROOT / "reports" / "API_COVERAGE_REPORT.md"))
    ap.add_argument("--ctest-log", default=None)
    args = ap.parse_args()

    cmds, stubs = load()
    rows = []
    tot_c = tot_cc = st_c = st_cc = 0
    for v in VERSIONS:
        c = core_at(cmds, v)
        cc = compat_at(cmds, v)
        sc = [n for n in c if n in stubs]
        scc = [n for n in cc if n in stubs]
        tot_c += len(c); tot_cc += len(cc); st_c += len(sc); st_cc += len(scc)
        rows.append((v, len(c), len(sc), len(cc), len(scc)))

    stats = ctest_stats(pathlib.Path(args.ctest_log)) if args.ctest_log else None

    L = []
    L.append("# API_COVERAGE_REPORT.md — Coverage OpenGL 1.0–4.6 (Core + Compatibility)")
    L.append("")
    L.append("> **Mọi số liệu đo bằng script**, không viết tay:")
    L.append("> `python3 tools/gen_coverage_report.py`")
    L.append("> (nguồn: `data/version_inventory.json` — sinh từ `docs/khronos/gl.xml` —")
    L.append("> và `src/gl/gl_stubs_gen.cpp` — danh sách lệnh còn stub).")
    L.append("")
    L.append("**Nhãn môi trường:** `REFERENCE_ONLY` (backend Null) — **KHÔNG** phải")
    L.append("`A11_DEVICE`. Xem `reports/FULL_AUDIT_REPORT.md` §0.")
    L.append("")
    L.append("## 0. Cách đo")
    L.append("")
    L.append("| Khái niệm | Định nghĩa |")
    L.append("|---|---|")
    L.append("| Lệnh **có ở version V** (core) | `core_available` && `introduced_version <= V` |")
    L.append("| Lệnh **có ở version V** (compat) | `introduced_version <= V` && chưa bị gỡ khỏi compatibility ở version <= V |")
    L.append("| Lệnh **đã implement** | không còn xuất hiện trong `src/gl/gl_stubs_gen.cpp` |")
    L.append("")
    L.append("## 1. Coverage theo version × profile")
    L.append("")
    L.append("| Version | Core | % | Compatibility | % |")
    L.append("|---|---|---|---|---|")
    for v, nc, sc, ncc, scc in rows:
        pc = 100.0 * (nc - sc) / max(1, nc)
        pcc = 100.0 * (ncc - scc) / max(1, ncc)
        L.append(f"| {v[0]}.{v[1]} | {nc - sc}/{nc} | {pc:.1f}% | {ncc - scc}/{ncc} | {pcc:.1f}% |")
    L.append("")
    L.append(f"**Tổng:** core **{tot_c - st_c}/{tot_c}** "
             f"({100.0 * (tot_c - st_c) / max(1, tot_c):.1f}%) · compatibility "
             f"**{tot_cc - st_cc}/{tot_cc}** "
             f"({100.0 * (tot_cc - st_cc) / max(1, tot_cc):.1f}%)")
    L.append("")
    L.append(f"**Lệnh còn stub: {len(stubs)}**"
             + (" (danh sách rỗng ⇒ không còn stub nào)" if not stubs else ""))
    if stubs:
        L.append("")
        L.append("| Lệnh còn stub |")
        L.append("|---|")
        for n in sorted(stubs):
            L.append(f"| `{n}` |")
    L.append("")
    L.append("## 2. Test")
    L.append("")
    if stats:
        total, passed, failed, skipped = stats
        L.append(f"`ctest -j8` → **{passed}/{total} PASS**"
                 + (f", {failed} FAIL" if failed else "")
                 + (f", {skipped} SKIP" if skipped else "")
                 + " (đọc từ log `ctest -V`).")
    else:
        L.append("Chạy `ctest -V > /tmp/ctest_v.log` rồi "
                 "`python3 tools/gen_coverage_report.py --ctest-log /tmp/ctest_v.log` "
                 "để điền số test.")
    L.append("")
    L.append("## 3. Phạm vi")
    L.append("")
    L.append("- Báo cáo này đo **bề mặt API** (lệnh nào có và đã implement).")
    L.append("- Trạng thái **hành vi** (đúng spec hay chưa) nằm ở")
    L.append("  `reports/LEGACY_COVERAGE_REPORT.md` và `reports/FULL_AUDIT_REPORT.md`.")
    L.append("- Phần chưa xác minh trên **Apple A11** vẫn là `NOT_VERIFIED` — xem")
    L.append("  `PROGRESS.md`.")
    L.append("")

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(L))
    print(f"[coverage] wrote {out} (core {tot_c-st_c}/{tot_c}, compat {tot_cc-st_cc}/{tot_cc}, "
          f"stubs {len(stubs)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
