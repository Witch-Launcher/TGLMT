#!/usr/bin/env python3
"""Sinh danh sách pname hợp lệ cho glGet* từ nguồn chính thức.

Vì sao cần
----------
GL 4.6 §6.1 / §22.1: "An INVALID_ENUM error is generated if pname is not one of
the accepted values."  Trước đây `src/gl/gl_get.cpp` có **0** lệnh ghi
GL_INVALID_ENUM: mọi pname lạ đều âm thầm trả 0 — đúng là cơ chế mà app dùng
để phát hiện driver không tuân thủ lại không hoạt động.

Danh sách pname hợp lệ được gộp từ BA nguồn chính thức:

  1. **Bảng State của OpenGL 4.6 Core spec** (`docs/khronos/glspec46.core.pdf`,
     Chương 23 "State Tables"). Tên state trong PDF không có tiền tố `GL_` và
     có thể xuống dòng; ta nối các dòng ALL-CAPS liên tiếp rồi đối chiếu với
     tên enum trong `docs/khronos/gl.xml` (registry Khronos). Kết quả được
     cache vào `docs/khronos/state_pnames_46.txt` để build không cần PDF.

  2. **Enum của các extension mà TGLMT advertise** (`kExts[]` trong
     `src/gl/gl_error_string_debug.cpp`) — lấy từ `<require><enum>` trong
     gl.xml. App hỏi pname của extension đang được advertise thì không được
     sinh lỗi.

  3. **Pname mà implementation xử lý tay** (`case 0xXXXX:` trong
     `src/gl/gl_get*.cpp`) — lưới an toàn: nếu code đã có nhánh xử lý riêng thì
     chắc chắn là pname hợp lệ (vd pname của extension chưa có trong gl.xml).

Dùng:
    python3 tools/gen_state_pnames.py            # ghi header + cache
    python3 tools/gen_state_pnames.py --check    # chỉ kiểm tra, không ghi
"""
import argparse
import os
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
SPEC_PDF = ROOT / "docs/khronos/glspec46.core.pdf"
CACHE = ROOT / "docs/khronos/state_pnames_46.txt"
OUT_H = ROOT / "include/tglmt/gl46_pnames.h"
GL_ERR = ROOT / "src/gl/gl_error_string_debug.cpp"
GET_SOURCES = ("src/gl/gl_get.cpp", "src/gl/gl_get_extra.cpp")

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gl46_catalog


def registry_enum_names():
    root = ET.parse(str(GLXML)).getroot()
    out = {}
    for es in root.findall("enums"):
        if es.get("namespace") != "GL":
            continue
        for e in es.findall("enum"):
            nm, val = e.get("name"), e.get("value")
            if nm and val and nm not in out:
                out[nm] = val
    return out


def extract_state_pnames_from_pdf(enum_names):
    """Trích tên state từ Chương 23 của spec PDF (fallback khi chưa có cache)."""
    try:
        from pypdf import PdfReader
    except ImportError:
        return None
    reader = PdfReader(str(SPEC_PDF))
    lines = []
    for i in range(603, 678):
        try:
            lines.extend((reader.pages[i].extract_text() or "").splitlines())
        except Exception:
            pass
    cands = []
    for ln in lines:
        s = ln.strip()
        if not s:
            continue
        m = re.match(
            r"^([A-Z][A-Z0-9_]*(?:[ \-][A-Z0-9_]+)*?)\s+"
            r"(Z\s*\+|[ZBEYFINP]|S|\d+\s*[×x]\s*[A-Z]|\d+\s*[×x]\s*\d*)\s", s)
        if m:
            cands.append(m.group(1))
            continue
        if re.fullmatch(r"[A-Z][A-Z0-9_]*(?:[ \-][A-Z0-9_]+)*", s) and len(s) >= 3 and cands:
            cands[-1] = cands[-1] + " " + s
    out = set()
    for c in cands:
        for key in ("GL_" + c.replace(" ", "_").replace("-", "_"),
                    "GL_" + c.replace(" ", "_")):
            if key in enum_names:
                out.add(key)
    return out


def advertised_extension_enums(enum_names):
    """Enum của các extension TGLMT advertise (app hỏi pname của chúng hợp lệ)."""
    text = GL_ERR.read_text()
    m = re.search(r"kExts\s*\[\]\s*=\s*\{(.*?)\};", text, re.S)
    adv = set(re.findall(r'"(GL_[A-Za-z0-9_]+)"', m.group(1))) if m else set()
    root = ET.parse(str(GLXML)).getroot()
    out = set()
    for ext in root.find("extensions").findall("extension"):
        name = ext.get("name")
        if name not in adv:
            continue
        for req in ext.findall("require"):
            for e in req.findall("enum"):
                nm = e.get("name")
                if nm in enum_names:
                    out.add(nm)
    return out


def implementation_pnames(enum_names):
    """Pname có nhánh `case 0x...:` trong gl_get*.cpp → chắc chắn hợp lệ."""
    by_value = {}
    for nm, val in enum_names.items():
        try:
            v = int(val, 0)
        except (TypeError, ValueError):
            continue
        by_value.setdefault(v, nm)
    out = set()
    for rel in GET_SOURCES:
        path = ROOT / rel
        if not path.exists():
            continue
        text = path.read_text(errors="replace")
        for m in re.finditer(r"case\s+(0x[0-9A-Fa-f]+|\d+)\s*:", text):
            try:
                v = int(m.group(1), 0)
            except ValueError:
                continue
            nm = by_value.get(v)
            if nm:
                out.add(nm)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    enum_names = registry_enum_names()

    if CACHE.exists():
        core_state = {ln.strip() for ln in CACHE.read_text().splitlines() if ln.strip()}
        origin = "cache"
    else:
        core_state = extract_state_pnames_from_pdf(enum_names)
        if core_state is None:
            print("[pnames] ERROR: không có pypdf và không có cache "
                  f"{CACHE.relative_to(ROOT)}", file=sys.stderr)
            return 1
        origin = "pdf"

    ext_enum = advertised_extension_enums(enum_names)
    impl = implementation_pnames(enum_names)
    def _raw_value(nm):
        try:
            return int(enum_names[nm], 0)
        except (KeyError, TypeError, ValueError):
            return None

    accepted = core_state | ext_enum | impl
    # Enum có giá trị 0 (GL_NO_ERROR, GL_ZERO, ...) là MÃ, không phải pname
    # hợp lệ của glGet* → loại bằng GIÁ TRỊ (tên alias khác nhau).
    accepted = {nm for nm in accepted if _raw_value(nm) != 0}

    def _val(nm):
        try:
            v = int(enum_names[nm], 0)
        except (KeyError, TypeError, ValueError):
            return None
        # GLenum là 32-bit: enum 64-bit (vd GL_ALL_BARRIER_BITS) không phải pname.
        if v < 0 or v > 0xFFFFFFFF:
            return None
        return v

    # Mảng phải sort theo GIÁ TRỊ để binary search đúng.
    # Trùng giá trị (nhiều tên alias cùng enum) → giữ tên đầu tiên.
    by_val = {}
    for nm in sorted(accepted):
        v = _val(nm)
        if v is None:
            continue
        by_val.setdefault(v, nm)
    accepted = sorted(by_val, key=lambda v: v)
    by_val = {v: nm for v, nm in by_val.items()}

    print(f"[pnames] core state (from {origin}): {len(core_state)}")
    print(f"[pnames] advertised-extension enums: {len(ext_enum)}")
    print(f"[pnames] implementation-handled: {len(impl)}")
    print(f"[pnames] accepted total: {len(accepted)}")

    if args.check:
        return 0

    if not CACHE.exists():
        CACHE.parent.mkdir(parents=True, exist_ok=True)
        CACHE.write_text("\n".join(sorted(core_state)) + "\n")
        print(f"[pnames] wrote {CACHE.relative_to(ROOT)}")

    OUT_H.parent.mkdir(parents=True, exist_ok=True)
    with open(OUT_H, "w") as f:
        f.write("// AUTO-GENERATED bởi tools/gen_state_pnames.py — KHÔNG SỬA TAY.\n")
        f.write("// Danh sách pname mà glGet* chấp nhận (dùng để sinh GL_INVALID_ENUM\n")
        f.write("// theo GL 4.6 §22.1). Gộp từ:\n")
        f.write("//   1. Bảng State Chương 3 của glspec46.core.pdf (đối chiếu gl.xml)\n")
        f.write("//   2. Enum của extension TGLMT advertise\n")
        f.write("//   3. Pname có nhánh xử lý riêng trong src/gl/gl_get*.cpp\n")
        f.write("#pragma once\n#include <cstdint>\n#include <cstddef>\n\nnamespace tglmt {\n\n")
        f.write(f"inline constexpr uint32_t kAcceptedPnameCount = {len(accepted)};\n")
        f.write("inline constexpr uint32_t kAcceptedPnames[] = {\n")
        for v in accepted:
            f.write(f"    0x{v:X}u, // {by_val[v]}\n")
        f.write("};\n\n")
        f.write("// Tìm pname trong danh sách (binary search trên mảng đã sort).\n")
        f.write("inline bool IsAcceptedPname(uint32_t p) {\n")
        f.write("    uint32_t lo = 0, hi = kAcceptedPnameCount;\n")
        f.write("    while (lo < hi) {\n")
        f.write("        uint32_t mid = lo + (hi - lo) / 2;\n")
        f.write("        uint32_t v = kAcceptedPnames[mid];\n")
        f.write("        if (v == p) return true;\n")
        f.write("        if (v < p) lo = mid + 1; else hi = mid;\n")
        f.write("    }\n")
        f.write("    return false;\n")
        f.write("}\n\n")
        f.write("} // namespace tglmt\n")
    print(f"[pnames] wrote {OUT_H.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
