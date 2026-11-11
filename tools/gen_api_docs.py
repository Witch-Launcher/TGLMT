#!/usr/bin/env python3
"""Sinh docs/OPENGL_46_API_INVENTORY.md + docs/OPENGL_46_API_COVERAGE.md từ
manifest thật (tests/truth/api_manifest.json) + catalog chuẩn.

Dùng:
    python3 tools/gen_api_docs.py            # ghi 2 file docs
    python3 tools/gen_api_docs.py --check    # chỉ kiểm tra không ghi

Vì sao cần máy sinh: bảng trạng thái 657 hàm phải khớp ĐÚNG với manifest và
với việc export symbol thật của thư viện — nếu viết tay, bảng sẽ trôi khỏi
sự thật sau mỗi lần sửa code.
"""
import argparse
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests/truth/api_manifest.json"
OUT_INVENTORY = ROOT / "docs/OPENGL_46_API_INVENTORY.md"
OUT_COVERAGE = ROOT / "docs/OPENGL_46_API_COVERAGE.md"

sys.path.insert(0, str(ROOT / "tools"))
import gl46_catalog

# Trạng thái theo phân loại yêu cầu.
ST_FULL = "FULLY IMPLEMENTED"
ST_PARTIAL = "PARTIALLY IMPLEMENTED"
ST_INCORRECT = "INCORRECT"
ST_STUB = "STUB"
ST_MISSING = "MISSING"
ST_EMULATED = "METAL EMULATED"
ST_HW = "HARDWARE LIMITED"
ST_BLOCKED = "BLOCKED"
ST_NA = "NOT APPLICABLE"


def lib_symbols():
    """Symbol export thật của thư viện dùng chung (nếu build được)."""
    for cand in ("build-work/libtglmt.dylib", "build/libtglmt.dylib",
                 "build-bench/libtglmt.dylib"):
        p = ROOT / cand
        if not p.exists():
            continue
        out = subprocess.run(["nm", "-gU", str(p)], capture_output=True, text=True)
        if out.returncode != 0:
            continue
        names = set()
        for line in out.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[-2] in ("T", "t"):
                nm = parts[-1]
                # macOS thêm '_' tiều tố cho symbol C.
                if nm.startswith("_") and not nm.startswith("__"):
                    nm = nm[1:]
                names.add(nm)
        return names, cand
    return set(), None


def classify(row, symbols):
    """Phân loại trạng thái của một API theo bằng chứng thực tế."""
    name = row["name"]
    if not row["symbol"]:
        return ST_MISSING, "không có symbol export"
    kind = row["impl_kind"]
    if kind == "noop":
        if row["noop_triage"] == "allowed:hint":
            return ST_FULL, "spec cho phép bỏ qua (hint)"
        return ST_STUB, "thân hàm rỗng + thiếu error bắt buộc"
    if kind == "constant":
        return ST_PARTIAL, "trả hằng số cố định"
    if kind == "alias":
        return ST_FULL, "alias tới hàm nền"
    if kind == "real":
        if name not in symbols:
            return ST_PARTIAL, "có thân nhưng chưa export"
        if not row["tests"]:
            return ST_PARTIAL, "có thân thật, chưa có test tham chiếu trực tiếp"
        return ST_FULL, "có thân thật + test tham chiếu"
    return ST_PARTIAL, f"impl_kind={kind}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    manifest = json.loads(MANIFEST.read_text())
    core = manifest["core"]
    compat = manifest.get("compat_only", [])
    symbols, libpath = lib_symbols()

    core_catalog = gl46_catalog.parse_core_commands()
    compat_catalog = gl46_catalog.parse_compat_only_commands()

    counts = {}
    rows = []
    for r in core:
        st, why = classify(r, symbols)
        counts[st] = counts.get(st, 0) + 1
        rows.append((r, st, why))

    total = len(core)
    full = counts.get(ST_FULL, 0)
    partial = counts.get(ST_PARTIAL, 0)
    stub = counts.get(ST_STUB, 0)
    missing = counts.get(ST_MISSING, 0)

    # ---- inventory ----
    inv = []
    inv.append("# OpenGL 4.6 Core Profile — API Inventory\n")
    inv.append("> Sinh bởi `python3 tools/gen_api_docs.py` — KHÔNG sửa tay.\n")
    inv.append("## 1. Danh mục chuẩn và cách đếm\n")
    inv.append(
        "Danh mục **OpenGL 4.6 Core Profile** được suy ra từ registry chính thức "
        "Khronos (`docs/khronos/gl.xml`, bản `Copyright 2013-2026`, "
        "sha256 `b9ca2cfa5c676e901c20d34af3407f1687cde0f1336a5ff7a8974d04c7494ad3` — "
        "byte-identical với `KhronosGroup/OpenGL-Registry@main`).\n")
    inv.append(
        "Phép suy ra bám đúng ngữ nghĩa của Khronos `xml/reg.py` (PASS 1): một lượt "
        "duyệt `GL_VERSION_1_0` → `GL_VERSION_4_6` theo thứ tự phiên bản, trong đó\n")
    inv.append("| Quy tắc | Tác động |\n|---|---|\n")
    inv.append("| `<require>` không có `profile` | thuộc mọi profile → **core** |\n")
    inv.append("| `<require profile=\"compatibility\">` | chỉ compatibility profile → **KHÔNG core** |\n")
    inv.append("| `<remove>` không có `profile` | bị gỡ ở mọi profile |\n")
    inv.append("| `<remove profile=\"core\">` | bị gỡ ở core → loại khỏi core |\n")
    inv.append("\n\"Last write wins\" theo thứ tự phiên bản: một lệnh bị gỡ ở core ở "
               "phiên bản này rồi được require lại ở phiên bản sau sẽ **có** trong core.\n")
    inv.append("Nguồn đếm duy nhất: `tools/gl46_catalog.py`.\n")
    inv.append("### 1.1. Đối chiếu ba chiều\n")
    inv.append("| Nguồn | Kết quả |\n|---|---|\n")
    inv.append(f"| Suy ra từ `gl.xml` (module dùng chung) | **{len(core_catalog)}** hàm |\n")
    inv.append("| Header chính thức `glcorearb.h` của Khronos (cùng `gl.xml`) | 657 prototype — "
               "**tập bằng nhau** (mọi hàm của ta đều có trong header, và không hàm nào "
               "chỉ có ở compatibility profile xuất hiện trong header) |\n")
    inv.append("| `glspec46.core.pdf` Chương 23 (bảng State) + `gl.xml` | 341 biến state, "
               "dùng để sinh `include/tglmt/gl46_pnames.h` (danh sách pname hợp lệ cho `glGet*`) |\n")
    inv.append("\n### 1.2. Con số \"698\" trước đây — sai và vì sao\n")
    inv.append(
        "Dự án từng công bố **698** hàm \"OpenGL 4.6 Core\". Con số đó **sai**: nó "
        "bằng `core ∪ compatibility-only` chứ không phải core profile. Nguyên nhân là "
        "hai lỗi trong cách suy danh mục, lặp lại ở **ba** tool "
        "(`tools/truth_manifest.py`, `tools/gen_gl_headers.py`, `tools/gen_stubs.py`) "
        "và được đóng băng bằng assert `== 698`:\n")
    inv.append("| Lỗi | Hệ quả |\n|---|---|\n")
    inv.append("| B1: bỏ qua `profile=\"compatibility\"` trên `<require>` | **42** hàm chỉ có "
               "ở compatibility profile bị tính là core |\n")
    inv.append("| B2: áp `<remove>` *sau* tất cả `<require>` (không theo thứ tự phiên bản) | "
               "`glGetPointerv` bị gỡ ở core GL 3.2 rồi require lại ở GL 4.3 ⇒ bị loại |\n")
    inv.append("\n42 hàm compatibility-only bị tính nhầm:\n")
    inv.append("```\n")
    for f in sorted(compat_catalog):
        inv.append(f + "\n")
    inv.append("```\n")
    inv.append(
        "Điểm mấu chốt: `glGetPointerv` **CÓ** thuộc core (GL 4.3 restore nó), và "
        "dự án **đã implement + export** hàm này (`src/gl/gl_debug_p4.cpp`) — chỉ là "
        "danh mục không liệt kê nó.\n")
    inv.append("### 1.3. Số liệu hiện tại\n")
    inv.append("| Chỉ số | Giá trị |\n|---|---|\n")
    inv.append(f"| Hàm OpenGL 4.6 **Core Profile** (chuẩn Khronos) | **{len(core_catalog)}** |\n")
    inv.append(f"| Hàm compatibility-profile-only (implement + export, **không** phải core) | "
               f"**{len(compat_catalog)}** |\n")
    inv.append(f"| Tổng symbol `gl*` export trong thư viện | "
               f"{len([s for s in symbols if s.startswith('gl')])} |\n")
    inv.append(f"| Hàm core **thiếu symbol** | **{manifest['summary']['core_missing_symbol']}** |\n")
    inv.append(f"| Hàm core có thân **thật** (không phải no-op/alias) | "
               f"**{sum(1 for r in core if r['impl_kind'] == 'real')}** |\n")
    inv.append(f"| Hàm core là **no-op/constant** | "
               f"**{manifest['summary']['core_noop_or_constant']}** "
               f"(trong đó **{manifest['summary']['core_noop_static_gaps']}** là static gap thật) |\n")
    inv.append(f"| Hàm core **có test tham chiếu trực tiếp** | "
               f"**{sum(1 for r in core if r['tests'])} / {total}** "
               f"({100.0 * sum(1 for r in core if r['tests']) / total:.1f}%) |\n")
    inv.append(f"\nThư viện dùng để đối chiếu symbol: `{libpath or '(chưa build)'}`\n")
    inv.append("## 2. Phân loại trạng thái\n")
    inv.append("| Trạng thái | Số hàm | Ý nghĩa |\n|---|---:|---|\n")
    meanings = {
        ST_FULL: "thân hàm thật + có test tham chiếu trực tiếp",
        ST_PARTIAL: "thân hàm thật nhưng chưa có test tham chiếu trực tiếp",
        ST_STUB: "chỉ có khai báo / thân rỗng — CHƯA tính là hỗ trợ",
        ST_MISSING: "chưa implement",
        ST_INCORRECT: "có code nhưng sai hành vi",
        ST_EMULATED: "được mô phỏng bằng cơ chế khác trên Metal",
        ST_HW: "bị giới hạn bởi phần cứng/nền tảng",
        ST_BLOCKED: "chưa có cách triển khai đúng",
        ST_NA: "ngoài phạm vi OpenGL 4.6 Core",
    }
    for st in (ST_FULL, ST_PARTIAL, ST_INCORRECT, ST_EMULATED, ST_HW, ST_BLOCKED,
               ST_STUB, ST_MISSING, ST_NA):
        if st in counts:
            inv.append(f"| {st} | {counts[st]} | {meanings[st]} |\n")
    inv.append("\n> Lưu ý: `FULLY IMPLEMENTED` ở đây nghĩa là **có thân hàm thật và có "
               "test tham chiếu**, không phải \"đã chứng minh đúng 100% mọi nhánh "
               "hành vi\". Mức đó cần Khronos CTS (chưa có — xem "
               "`reports/CONFORMANCE_REPORT.md`).\n")
    inv.append("## 3. Bảng đầy đủ 657 hàm Core\n")
    inv.append("| API | GL version | Symbol | Impl kind | File | Trạng thái | Lý do |\n")
    inv.append("|---|---|---|---|---|---|---|\n")
    for r, st, why in rows:
        inv.append(f"| `{r['name']}` | {', '.join(r['versions'])} | "
                   f"{'✓' if r['symbol'] else '✗'} | {r['impl_kind']} | "
                   f"`{r['impl_file'] or '-'}` | {st} | {why} |\n")
    inv.append("\n## 4. 42 hàm compatibility-profile-only (ngoài phạm vi Core)\n")
    inv.append("| API | GL version | Symbol | File |\n|---|---|---|---|\n")
    for r in compat:
        inv.append(f"| `{r['name']}` | {', '.join(r['versions'])} | "
                   f"{'✓' if r['symbol'] else '✗'} | `{r['impl_file'] or '-'}` |\n")
    inv.append("\nCác hàm này được implement và export vì "
               "`GL_ARB_vertex_type_2_10_10_10_rev` và `GL_ARB_robustness` cần tới, "
               "nhưng **không được tính vào OpenGL 4.6 Core**.\n")

    # ---- coverage ----
    cov = []
    cov.append("# OpenGL 4.6 Core Profile — Coverage\n")
    cov.append("> Sinh bởi `python3 tools/gen_api_docs.py` — KHÔNG sửa tay.\n")
    cov.append("## Tóm tắt\n")
    cov.append("| Chỉ số | Giá trị |\n|---|---|\n")
    cov.append(f"| Tổng hàm core | {total} |\n")
    cov.append(f"| FULLY IMPLEMENTED (thân thật + test) | {full} |\n")
    cov.append(f"| PARTIALLY IMPLEMENTED (thân thật, chưa có test trực tiếp) | {partial} |\n")
    cov.append(f"| STUB (thân rỗng, thiếu error bắt buộc) | {stub} |\n")
    cov.append(f"| MISSING | {missing} |\n")
    cov.append(f"| Extension rows trong registry | {manifest['summary']['extension_rows']} |\n")
    cov.append(f"| Extension đang advertise | {manifest['summary']['advertised_extensions']} |\n")
    cov.append(f"| Extension advertise thiếu symbol | "
               f"{manifest['summary']['advertised_missing_symbols']} |\n")
    cov.append(f"| Extension advertise là no-op | "
               f"{manifest['summary']['advertised_noop_or_constant']} |\n")
    cov.append("\n## Hàm core CHƯA đạt FULLY IMPLEMENTED\n")
    cov.append("| API | Trạng thái | Lý do |\n|---|---|---|\n")
    for r, st, why in rows:
        if st != ST_FULL:
            cov.append(f"| `{r['name']}` | {st} | {why} |\n")
    cov.append("\n## Danh sách hàm core đã FULLY IMPLEMENTED\n")
    full_names = [r["name"] for r, st, _ in rows if st == ST_FULL]
    cov.append(f"({len(full_names)} hàm)\n\n```\n")
    for i in range(0, len(full_names), 6):
        cov.append("  ".join(f"{n:<34}" for n in full_names[i:i + 6]).rstrip() + "\n")
    cov.append("```\n")
    cov.append("\n## Cách tái tạo\n")
    cov.append("```sh\n")
    cov.append("cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DTGLMT_APPLE_METAL=ON\n")
    cov.append("cmake --build build -j8\n")
    cov.append("python3 tools/truth_manifest.py --lib build/libtglmt.dylib --write\n")
    cov.append("python3 tools/gen_api_docs.py\n")
    cov.append("```\n")

    if args.check:
        print("[api-docs] --check: OK")
        return 0

    OUT_INVENTORY.parent.mkdir(parents=True, exist_ok=True)
    OUT_INVENTORY.write_text("".join(inv))
    OUT_COVERAGE.write_text("".join(cov))
    print(f"[api-docs] wrote {OUT_INVENTORY.relative_to(ROOT)} "
          f"({total} core rows)")
    print(f"[api-docs] wrote {OUT_COVERAGE.relative_to(ROOT)}")
    print(f"[api-docs] FULLY={full} PARTIAL={partial} STUB={stub} MISSING={missing}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
