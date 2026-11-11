#!/usr/bin/env python3
"""glref.py — tra cứu nhanh spec OpenGL 4.6 (offline).

Dùng:
  python3 tools/glref.py enum GL_MIN_SAMPLE_SHADING_VALUE
      → tra giá trị enum trong docs/khronos/gl.xml (registry chính thức Khronos)

  python3 tools/glref.py find "glLogicOp"
      → tìm đoạn text trong glspec46.core.pdf (đã cache trong .cache/)

  python3 tools/glref.py getpname
      → liệt kê toàn bộ enum group="GetPName" (các pname glGet phải phục vụ)

  python3 tools/glref.py ext GL_ARB_sample_shading
      → liệt kê token + entrypoint của extension từ gl.xml

Không cần mạng: gl.xml và glspec46.core.pdf nằm trong docs/khronos/.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
XML = os.path.join(ROOT, "docs", "khronos", "gl.xml")
PDF = os.path.join(ROOT, "docs", "khronos", "glspec46.core.pdf")
CACHE_DIR = os.path.join(ROOT, ".cache")
CACHE = os.path.join(CACHE_DIR, "glspec46_pages.json")


def _xml():
    if not os.path.exists(XML):
        sys.exit(f"missing {XML}")
    with open(XML, "r", errors="replace") as f:
        return f.read()


def cmd_enum(args):
    t = _xml()
    for name in args:
        vals = []
        for m in re.finditer(r'<enum\s+value="([^"]+)"\s+name="' + re.escape(name) + r'"[^>]*/>', t):
            vals.append(m.group(1))
        if not vals:
            # thử attribute theo thứ tự khác
            for m in re.finditer(r'<enum\s+name="' + re.escape(name) + r'"[^>]*value="([^"]+)"', t):
                vals.append(m.group(1))
        if vals:
            print(f"{name} = {', '.join(sorted(set(vals)))}")
        else:
            print(f"{name} = NOT FOUND")


def cmd_getpname(args):
    t = _xml()
    out = []
    for m in re.finditer(r'<enum\s+value="([^"]+)"\s+name="([^"]+)"[^>]*group="([^"]*)"', t):
        if "GetPName" in m.group(3):
            out.append((m.group(1), m.group(2)))
    out.sort(key=lambda x: int(x[0], 16))
    for v, n in out:
        print(f"{v}  {n}")


def cmd_ext(args):
    t = _xml()
    for ext in args:
        m = re.search(r'<extension\s+name="' + re.escape(ext) + r'"[^>]*>(.*?)</extension>', t, re.S)
        if not m:
            print(f"{ext} = NOT FOUND")
            continue
        body = m.group(1)
        req = re.search(r"<require[^>]*>(.*?)</require>", body, re.S)
        tokens = re.findall(r'<enum\s+value="([^"]+)"\s+name="([^"]+)"', req.group(1) if req else "")
        cmds = re.findall(r'<command\s+name="([^"]+)"', req.group(1) if req else "")
        print(f"=== {ext} ===")
        if tokens:
            print("  tokens:")
            for v, n in sorted(tokens, key=lambda x: int(x[0], 16)):
                print(f"    {v}  {n}")
        if cmds:
            print("  commands:")
            for c in cmds:
                print(f"    {c}")


def _pages():
    if os.path.exists(CACHE):
        import json
        with open(CACHE) as f:
            return json.load(f)
    try:
        from pypdf import PdfReader
    except ImportError:
        sys.exit("pypdf not installed; cannot cache spec text")
    os.makedirs(CACHE_DIR, exist_ok=True)
    reader = PdfReader(PDF)
    pages = {}
    for i, pg in enumerate(reader.pages):
        try:
            pages[str(i + 1)] = pg.extract_text() or ""
        except Exception:
            pages[str(i + 1)] = ""
    import json
    with open(CACHE, "w") as f:
        json.dump(pages, f)
    return pages


def cmd_find(args):
    needle = args[0] if args else ""
    pages = _pages()
    ctx = int(args[1]) if len(args) > 1 else 600
    hits = 0
    for p in sorted(pages, key=int):
        t = pages[p]
        if needle in t:
            i = t.find(needle)
            print(f"--- page {p} ---")
            print(t[max(0, i - 200): i + ctx].strip())
            print()
            hits += 1
            if hits >= 8:
                break
    if not hits:
        print(f"'{needle}' not found in spec text")


def cmd_pages(args):
    pages = _pages()
    for p in args:
        print(f"===== page {p} =====")
        print(pages.get(str(p), ""))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    cmd = sys.argv[1]
    rest = sys.argv[2:]
    fn = {"enum": cmd_enum, "find": cmd_find, "getpname": cmd_getpname,
          "ext": cmd_ext, "pages": cmd_pages}.get(cmd)
    if not fn:
        print(__doc__)
        return
    fn(rest)


if __name__ == "__main__":
    main()
