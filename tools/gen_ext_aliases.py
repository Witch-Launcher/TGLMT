#!/usr/bin/env python3
"""gen_ext_aliases.py — Sinh alias extension functions (P2).

Một số extension chỉ thêm hậu tố tên hàm (glBindBufferARB ≡ glBindBuffer).
Khi extension đó "alias-ready" (mọi hàm = alias của core-đã-implement) và
SIGNATURE khớp đúng trong gl.xml, sinh hàm forward 1:1.

TRUNG THỰC (không đoán): chỉ forward khi signature gl.xml của alias TRÙNG với
core-base. Lệch → bỏ qua + báo cáo "REVIEW" (cần implement tay riêng).

Output: src/gl/gl_ext_aliases_gen.cpp (AUTO-GENERATED, không sửa tay) + báo cáo.

Usage: python3 tools/gen_ext_aliases.py [--check]
  --check: chỉ in báo cáo, không ghi file.
"""
import re, sys, pathlib, xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
SRC = ROOT / "src/gl"
OUT = ROOT / "src/gl/gl_ext_aliases_gen.cpp"
MOBILE_VENDOR = {"ARB", "KHR", "EXT", "OES", "APPLE", "ARM", "IMG",
                 "NV", "AMD", "ATI", "MESA", "SGIS", "3DFX", "IBM", "WIN"}


def vendor(name):
    m = re.match(r"GL_(NVX?|AMD|ATI|APPLE|ARM|IMG|OES|EXT|KHR|ARB|SGI|SGIS|SGIX|"
                 r"IBM|INGR|MESA|MGX|WIN|3DFX|GREMEDY|OML|PGI)_", name)
    return m.group(1) if m else "OTHER"


def load_registry():
    root = ET.parse(str(GLXML)).getroot()
    # command signature: name -> (ret, [param_types])
    sig = {}
    for cmds in root.findall("commands"):
        for cmd in cmds.findall("command"):
            proto = cmd.find("proto")
            name = proto.find("name").text.strip()
            ret = "".join(proto.itertext()).replace(name, "").strip()
            params = []
            for p in cmd.findall("param"):
                ptxt = "".join(p.itertext()).strip()
                # bỏ tên biến, giữ kiểu (loại bỏ khoảng trắng thừa)
                ptype = re.sub(r"\s+", " ", re.sub(r"[A-Za-z_][A-Za-z0-9_]*\s*$", "", ptxt).strip())
                params.append(ptype)
            sig[name] = (ret, tuple(params))
    # core funcs
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
        funcs = set()
        for req in e.findall("require"):
            for c in req.findall("command"):
                funcs.add(c.get("name"))
        exts[e.get("name")] = funcs
    return sig, core, exts


def implemented_funcs():
    impl = set()
    for f in SRC.glob("*.cpp"):
        if f.name in ("gl_stubs_gen.cpp", "gl_ext_aliases_gen.cpp"):
            continue
        txt = f.read_text()
        for m in re.finditer(r"\b(gl[A-Za-z0-9_]+)\s*\(", txt):
            impl.add(m.group(1))
        for m in re.finditer(r"\b[A-Z][A-Z0-9_]+\s*\(\s*(gl[A-Za-z0-9_]+)", txt):
            base = m.group(1)
            impl.add(base)
            if base.endswith("ui") and "PK" in txt[max(0, m.start() - 200):m.start() + 200]:
                impl.add(base + "v")
    return impl


def alias_base(fn):
    m = re.match(r"^(gl.+?)(ARB|EXT|APPLE|OES|KHR|NV|AMD|ATI|MESA|SGIS)$", fn)
    return m.group(1) if m else None


def main():
    check = "--check" in sys.argv
    sig, core, exts = load_registry()
    impl = implemented_funcs()

    mobile = {n: fs for n, fs in exts.items()
              if vendor(n) in MOBILE_VENDOR or vendor(n) == "OTHER"}

    forward, review = [], []
    for name, funcs in sorted(mobile.items()):
        need = funcs - core
        if not need:
            continue
        # alias-ready: mọi need là alias của core-impl
        bases = {}
        is_alias_ready = True
        for f in sorted(need):
            b = alias_base(f)
            if not b or (b not in impl and b not in core):
                is_alias_ready = False
                break
            bases[f] = b
        if not is_alias_ready:
            continue
        # verify signature từng alias. So sánh sau chuẩn hoá: TGLMT typedef
        # GLint==GLsizei==int32_t, GLintptr==GLsizeiptr — lệch tên nhưng C ABI
        # giống hệt → forward an toàn (vd glProgramBinaryOES GLint vs GLsizei).
        def nsig(s):
            ret, pr = s
            m = {"GLsizei": "GLint", "GLsizeiptr": "GLintptr"}
            return (ret, tuple(m.get(p, p) for p in pr))
        for f, b in bases.items():
            if f not in sig or b not in sig:
                review.append((name, f, b, "no-sig"))
                continue
            if nsig(sig[f]) == nsig(sig[b]):
                forward.append((f, b, name))
            else:
                review.append((name, f, b, f"sig-mismatch {sig[f]} vs {sig[b]}"))

    print(f"=== gen_ext_aliases: {len(forward)} forward | {len(review)} REVIEW ===")
    if review:
        print("--- REVIEW (không forward, cần tay) ---")
        for n, f, b, why in review[:30]:
            print(f"  {f} → {b}  [{n}]  {why}")
        if len(review) > 30:
            print(f"  ... +{len(review)-30} khác")

    if check:
        return 0

    # sinh file — extern "C" (như c_exports) để LWJGL dlsym("glXxxARB") thấy.
    # Đây là lý do P3 cần file này: 5 ext đang advertise nhưng hàm ARB/EXT của
    # chúng KHÔNG có symbol (glSpecializeShaderARB=0) → code check ext rồi gọi
    # sẽ null. Sinh extern "C" forward làm alias resolvable.
    seen = set()
    with open(OUT, "w") as fp:
        fp.write("// AUTO-GENERATED bởi tools/gen_ext_aliases.py — KHÔNG SỬA TAY.\n")
        fp.write("// Alias extension functions: extern \"C\" forward 1:1 sang core-đã-implement.\n")
        fp.write("// Chỉ sinh khi signature gl.xml TRÙNG core-base (trung thực, không đoán).\n")
        fp.write('// LWJGL dlsym("glXxxARB") thấy symbol này khi extension được advertise.\n')
        fp.write('#include "tglmt/gl46.h"\n')
        fp.write("using namespace tglmt;\n")
        fp.write("namespace tglmt { namespace gl {\n")
        # forward decl cho mọi hàm core-base (an toàn dù đã có trong gl46.h)
        for f, b, _ in forward:
            ret, params = sig[b]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params)) if params else "void"
            fp.write(f"{ret} {b}({args});\n")
        fp.write("} }\n\n")
        fp.write('extern "C" {\n')
        for f, b, _ in forward:
            if f in seen:
                continue
            seen.add(f)
            ret, params = sig[f]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params)) if params else "void"
            call = ", ".join(f"a{i}" for i in range(len(params)))
            kw = "return " if ret != "void" else ""
            fp.write(f"{ret} {f}({args}) {{ {kw}tglmt::gl::{b}({call}); }}\n")
        fp.write("} // extern \"C\"\n")
    print(f"\nWrote {OUT}: {len(seen)} alias functions (extern \"C\").")
    return 0


if __name__ == "__main__":
    sys.exit(main())
