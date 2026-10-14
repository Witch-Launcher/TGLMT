#!/usr/bin/env python3
"""ext_coverage.py — Máy kiểm trung thực extension (P2).

Đối chiếu gl.xml (điều spec YÊU CẦU) với code (điều THỰC CÓ) để trả lời:
  - Mỗi extension mobile-relevant còn thiếu hàm nào → chưa được advertise.
  - kExts hiện tại có extension nào "nói dối" (advertise nhưng thiếu hàm) không.
  - Extension nào đã đủ hàm → sẵn sàng advertise (P3+ sẽ thêm + test).

Quy tắc vàng (gl_error_string_debug.cpp): advertise ⇔ implement ⇔ test.
Công cụ này chỉ kiểm implement⇔advertise; test do CI/device đảm nhiệm.

Usage:
  python3 tools/ext_coverage.py            # báo cáo đầy đủ
  python3 tools/ext_coverage.py --ready    # chỉ ext đủ hàm (sẵn advertise)
  python3 tools/ext_coverage.py --missing  # chỉ ext còn thiếu hàm
  python3 tools/ext_coverage.py --kexts    # kiểm consistency kExts hiện tại

Không hardcode danh sách extension: mọi thứ sinh từ gl.xml + quét src/gl.
"""
import re, sys, pathlib, xml.etree.ElementTree as ET
from collections import defaultdict

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
SRC = ROOT / "src/gl"
GL_ERR = ROOT / "src/gl/gl_error_string_debug.cpp"

# Vendor mobile-relevant (giữ). Desktop/NVIDIA/AMD/... → excluded.
MOBILE_VENDOR = {"ARB", "KHR", "EXT", "OES", "APPLE", "ARM", "IMG"}


def vendor(name):
    m = re.match(r"GL_(NVX?|AMD|ATI|APPLE|ARM|IMG|OES|EXT|KHR|ARB|SGI|SGIS|SGIX|"
                 r"IBM|INGR|MESA|MGX|WIN|3DFX|GREMEDY|OML|PGI)_", name)
    return m.group(1) if m else "OTHER"


def parse_glxml():
    root = ET.parse(str(GLXML)).getroot()
    # core functions (api=gl, version <= 4.6)
    core = set()
    for f in root.findall("feature"):
        if f.get("api") != "gl":
            continue
        n = f.get("name")
        if n.startswith("GL_VERSION_") and "ES" not in n:
            ver = tuple(map(int, n.replace("GL_VERSION_", "").split("_")))
            if ver <= (4, 6):
                for req in f.findall("require"):
                    for c in req.findall("command"):
                        core.add(c.get("name"))
    removed = set()
    for f in root.findall("feature"):
        for rm in f.findall("remove"):
            for c in rm.findall("command"):
                removed.add(c.get("name"))
    core -= removed

    # extensions
    exts = {}
    for e in root.find("extensions").findall("extension"):
        name = e.get("name")
        funcs = set()
        enums = set()
        for req in e.findall("require"):
            for c in req.findall("command"):
                funcs.add(c.get("name"))
            for en in req.findall("enum"):
                enums.add(en.get("name"))
        exts[name] = {"funcs": funcs, "enums": enums}
    return core, exts


def implemented_funcs():
    """Quét src/gl/*.cpp tìm mọi glXxx được định nghĩa tay (trừ stubs_gen)."""
    impl = set()
    for f in SRC.glob("*.cpp"):
        if f.name == "gl_stubs_gen.cpp":
            continue
        txt = f.read_text()
        # định nghĩa hàm: RET glXxx(...)
        for m in re.finditer(r"\b(gl[A-Za-z0-9_]+)\s*\(", txt):
            impl.add(m.group(1))
        # macro PK2/3/4(glXxx,...), PU1(glXxx,...)
        for m in re.finditer(r"\b[A-Z][A-Z0-9_]+\s*\(\s*(gl[A-Za-z0-9_]+)", txt):
            base = m.group(1)
            impl.add(base)
            if base.endswith("ui") and "PK" in txt[max(0, m.start() - 200):m.start() + 200]:
                impl.add(base + "v")
    return impl


def current_kexts():
    """Đọc kExts[] hiện tại trong gl_error_string_debug.cpp."""
    txt = GL_ERR.read_text()
    m = re.search(r"kExts\[\]\s*=\s*\{(.*?)\};", txt, re.S)
    if not m:
        return []
    return re.findall(r'"(GL_[A-Za-z0-9_]+)"', m.group(1))


def vendor_suffix_strip(fn):
    """glDrawArraysInstancedARB → glDrawArraysInstanced (bỏ hậu tố vendor).
    Trả về tên core-alias nếu có, None nếu không phải alias-đơn."""
    m = re.match(r"^(gl.+?)(ARB|EXT|APPLE|OES|KHR|NV|NVX|AMD|ATI|IMG|ARM|SGIS|SGIX)$", fn)
    if m:
        return m.group(1)
    return None


def classify(name, need, impl, core):
    """Phân loại 1 ext:
      'enum'     — không hàm mới
      'alias'    — mọi hàm missing là alias của core-đã-implement (rẻ: forward)
      'partial'  — có hàm thật chưa implement
    Trả (kind, missing_real, missing_alias)."""
    real, alias = [], []
    for f in sorted(need):
        # Hàm đã implement trực tiếp (kể cả không suffix như glGetPointerv) → đủ.
        if f in impl:
            continue
        base = vendor_suffix_strip(f)
        if base and (base in impl or base in core):
            alias.append(f)
        else:
            real.append(f)
    if not need:
        return "enum", [], []
    if not real:
        return "alias", [], alias
    return "partial", real, alias


def main():
    core, exts = parse_glxml()
    impl = implemented_funcs()
    kexts = current_kexts()

    mobile = {n: d for n, d in exts.items()
              if vendor(n) in MOBILE_VENDOR or vendor(n) == "OTHER"}
    desktop = {n for n in exts if n not in mobile}

    enumonly, aliasready, partial = [], [], []
    for name, d in sorted(mobile.items()):
        need = d["funcs"] - core
        kind, real, alias = classify(name, need, impl, core)
        if kind == "enum":
            enumonly.append(name)
        elif kind == "alias":
            aliasready.append((name, len(alias)))
        else:
            partial.append((name, len(real), real, len(alias)))

    mode = sys.argv[1] if len(sys.argv) > 1 else ""

    if mode == "--kexts":
        print("=== kExts consistency check (Quy tắc vàng: advertise ⇒ đủ hàm) ===")
        lies = 0
        for e in kexts:
            if e not in exts:
                print(f"  [LIE] {e}: không có trong gl.xml!")
                lies += 1
                continue
            d = exts[e]
            need = d["funcs"] - core
            kind, real, alias = classify(e, need, impl, core)
            if real:
                print(f"  [LIE] {e}: advertise nhưng thiếu {len(real)} hàm thật: {real[:5]}")
                lies += 1
            else:
                print(f"  [ok]  {e}: đủ (alias={len(alias)})")
        print(f"\n{kexts.__len__()} advertised, {lies} nói dối.")
        return 1 if lies else 0

    print(f"=== TGLMT extension coverage (P2 honesty machine) ===")
    print(f"gl.xml: {len(exts)} ext | core: {len(core)} | implemented tay: {len(impl)}")
    print(f"Mobile-relevant: {len(mobile)} | Desktop/NVIDIA excluded: {len(desktop)}")
    print()

    if mode in ("", "--ready"):
        print(f"--- ALIAS-READY (mọi hàm = alias core-đã-implement, {len(aliasready)} ext, RẺ) ---")
        for n, c in aliasready:
            print(f"  {n}  ({c} alias)")
        print()

    if mode in ("", "--missing"):
        print(f"--- CÒN THIẾU HÀM THẬT ({len(partial)} ext) ---")
        for n, nreal, real, nalias in partial:
            a = f" (+{nalias} alias)" if nalias else ""
            print(f"  {n}: {nreal} hàm thật{a}: {real[:4]}{'...' if nreal > 4 else ''}")
        print()

    print(f"--- ENUM-ONLY ({len(enumonly)} ext) ---")
    print(f"  advertise khi verify enum/behavior (P3)")
    print()
    print(f"SUMMARY: {len(enumonly)} enum-only | {len(aliasready)} alias-ready (rẻ) | {len(partial)} cần hàm thật")
    print(f"kExts hiện tại: {len(kexts)} → {kexts}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
