#!/usr/bin/env python3
"""gen_ext_aliases.py — Sinh alias extension functions (P2).

Một số extension chỉ thêm hậu tố tên hàm (glBindBufferARB ≡ glBindBuffer).
Khi extension đó "alias-ready" (mọi hàm = alias của core-đã-implement) và
SIGNATURE khớp đúng trong gl.xml, sinh hàm forward 1:1.

TRUNG THỰC (không đoán): chỉ forward khi
  (a) signature gl.xml của alias TRÙNG core-base (sau chuẩn hoá ABI), hoặc
  (b) ext-function được định nghĩa THẲNG trong src/gl (tên y hệt gl.xml) —
      sinh extern "C" wrapper trỏ vào định nghĩa đó (P5: nếu thiếu wrapper,
      ext advertise nhưng LWJGL dlsym ra null → crash), hoặc
  (c) base-unsuffixed được TGLMT tự đặt tên (không có trong gl.xml, vd
      glPushGroupMarkerEXT → glPushGroupMarker) — so chữ ký THẬT parse từ
      source với gl.xml; lệch → REVIEW (không đoán).

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


def split_params(s):
    """Tách danh sách param theo dấu phẩy mức 0 (an toàn với '*' nhưng không
    cần xử lý '(' lồng trong candidate set này)."""
    out, cur = [], ""
    for ch in s:
        if ch == ",":
            out.append(cur); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [p.strip() for p in out] if s.strip() else []


def strip_param_name(p):
    """'const GLchar *marker' → 'const GLchar *'; 'GLuint' → 'GLuint' (không tên)."""
    t = re.sub(r"[A-Za-z_][A-Za-z0-9_]*\s*$", "", p).strip()
    return t if t else p


def src_defs():
    """Parse định nghĩa hàm THẬT trong src/gl: name -> (ret, param_types).
    Tách real (implement thật) vs stub (gl_stubs_gen.cpp) để không alias vào stub.
    Strip comment trước (comment chứa 'glXxx(' giả làm signature giả)."""
    real, stub = {}, {}
    pat = re.compile(r"(?:(?:static|inline)\s+)?([A-Za-z_][A-Za-z0-9_ \t\*]{0,40}?)\s+"
                     r"(gl[A-Za-z0-9_]+)\s*\(([^;{}]*)\)\s*\{")
    for f in SRC.glob("*.cpp"):
        if f.name in ("gl_ext_aliases_gen.cpp", "gl_c_exports.cpp"):
            continue
        d = stub if f.name == "gl_stubs_gen.cpp" else real
        txt = f.read_text()
        txt = re.sub(r"/\*.*?\*/", " ", txt, flags=re.S)
        txt = re.sub(r"//[^\n]*", " ", txt)
        for m in pat.finditer(txt):
            ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
            if name in real or name in stub:
                continue
            d[name] = (ret, tuple(strip_param_name(p) for p in split_params(params)))
    return real, stub


def c_exported_names():
    """Tên đã có extern "C" trong gl_c_exports.cpp (core 4.6 + BONUS) → không
    sinh lại (tránh duplicate symbol). Parse file generated, không hardcode."""
    out = set()
    f = SRC / "gl_c_exports.cpp"
    if f.exists():
        for m in re.finditer(r'^extern "C"|^(?:\w[\w ]*?)\b(gl[A-Za-z0-9_]+)\s*\(', f.read_text(), re.M):
            if m.group(1):
                out.add(m.group(1))
    return out


def alias_base(fn):
    m = re.match(r"^(gl.+?)(ARB|EXT|APPLE|OES|KHR|NV|AMD|ATI|MESA|SGIS)$", fn)
    return m.group(1) if m else None


def nsig(s):
    """Chuẩn hoá chữ ký về ABI: gộp typedef đồng nghĩa, bỏ const/space, đếm '*'.
    TGLMT typedef GLint==GLsizei==int32_t, GLintptr==GLsizeiptr, GLhandleARB==GLuint,
    GLcharARB==GLchar, GLDEBUGPROC* == con trỏ hàm cùng ABI."""
    ret, pr = s
    m = {"GLsizei": "GLint", "GLsizeiptr": "GLintptr",
         "GLhandleARB": "GLuint", "GLcharARB": "GLchar"}

    def norm(t):
        # Thay token dài trước (GLcharARB trước GLchar) để khỏi cắt nhầm.
        t = (t.replace("GLsizeiptrARB", "GLsizeiptr").replace("GLintptrARB", "GLintptr")
             .replace("GLDEBUGPROCKHR", "GLDEBUGPROC").replace("GLDEBUGPROCARB", "GLDEBUGPROC")
             .replace("GLcharARB", "GLchar").replace("GLhandleARB", "GLuint"))
        t = m.get(t, t)
        t = t.replace("GLchar", "char")
        # Bỏ 'const' và space, đếm sao (*) để so sánh ABI pointer
        # (`const char**` vs `const char *const*` cùng ABI).
        stars = t.count("*")
        base = t.replace("const", "").replace("*", "").replace(" ", "").strip()
        if stars: base += "*" * stars
        return base
    return (norm(ret), tuple(norm(p) for p in pr))


def emit_type(t):
    """Type map khi emit extern "C" (TGLMT headers không có ARB/typedef lạ)."""
    return (t.replace("GLsizeiptrARB", "GLsizeiptr").replace("GLintptrARB", "GLintptr")
             .replace("GLDEBUGPROCKHR", "GLDEBUGPROC").replace("GLDEBUGPROCARB", "GLDEBUGPROC")
             .replace("GLhandleARB", "GLuint").replace("GLcharARB", "GLchar"))


def main():
    check = "--check" in sys.argv
    sig, core, exts = load_registry()
    impl = implemented_funcs()
    real, stub = src_defs()
    c_exported = c_exported_names()

    mobile = {n: fs for n, fs in exts.items()
              if vendor(n) in MOBILE_VENDOR or vendor(n) == "OTHER"}

    forward, native, review = [], [], []
    for name, funcs in sorted(mobile.items()):
        need = funcs - core
        if not need:
            continue
        # Alias-ready: hàm là alias của base-đã-implement (bỏ qua hàm real đã
        # implement trực tiếp — chúng không cần alias). Sinh alias cho subset
        # alias-ready ngay cả khi ext còn hàm real khác (P8: ARB_shader_objects
        # có 11 real + 28 alias — 28 alias vẫn sinh được).
        bases = {}
        for f in sorted(need):
            if f in real:
                # (b) Định nghĩa thẳng trong src với tên y hệt gl.xml → cần
                # extern "C" wrapper (nếu không: mangled-only, dlsym = null).
                if f in c_exported:
                    continue  # đã export qua gl_c_exports BONUS
                if f not in sig:
                    review.append((name, f, f, "native-no-sig"))
                elif nsig(real[f]) == nsig(sig[f]):
                    native.append((f, name))
                else:
                    review.append((name, f, f, f"native-sig-mismatch {real[f]} vs {sig[f]}"))
                continue
            if f in impl:
                continue  # đã xuất hiện (call/stub) — không sinh alias
            b = alias_base(f)
            if not b or (b not in impl and b not in core):
                continue  # real chưa implement (không sinh alias)
            bases[f] = b
        for f, b in bases.items():
            if f in c_exported:
                continue
            if f not in sig:
                review.append((name, f, b, "no-sig"))
                continue
            if b in sig:
                if nsig(sig[f]) == nsig(sig[b]):
                    forward.append((f, b, name))
                else:
                    review.append((name, f, b, f"sig-mismatch {sig[f]} vs {sig[b]}"))
            elif b in real:
                # (c) Base không có trong gl.xml (TGLMT tự đặt tên, vd
                # glPushGroupMarker) — so chữ ký THẬT parse từ source.
                if nsig(real[b]) == nsig(sig[f]):
                    sig[b] = sig[f]  # decl trong gen file dùng signature này
                    forward.append((f, b, name))
                else:
                    review.append((name, f, b, f"nosig-mismatch {real[b]} vs {sig[f]}"))
            else:
                review.append((name, f, b, "no-sig"))

    print(f"=== gen_ext_aliases: {len(forward)} forward | {len(native)} native-export "
          f"| {len(review)} REVIEW ===")
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
        fp.write("// Alias extension functions: extern \"C\" forward 1:1 sang hàm đã implement.\n")
        fp.write("// Chỉ sinh khi chữ ký khớp (gl.xml ↔ gl.xml, hoặc parse từ src/gl).\n")
        fp.write('// LWJGL dlsym("glXxxARB") thấy symbol này khi extension được advertise.\n')
        fp.write('#include "tglmt/gl46.h"\n')
        fp.write("using namespace tglmt;\n")
        fp.write("namespace tglmt { namespace gl {\n")
        # forward decl cho mọi hàm đích (an toàn dù đã có trong gl46.h)
        for f, b, _ in forward:
            ret, params = sig[b]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params)) if params else "void"
            fp.write(f"{ret} {b}({args});\n")
        for f, _ in native:
            ret, params = real[f]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params)) if params else "void"
            fp.write(f"{ret} {f}({args});\n")
        fp.write("} }\n\n")
        fp.write('extern "C" {\n')
        for f, b, _ in forward:
            if f in seen:
                continue
            seen.add(f)
            ret, params = sig[f]
            ret_e = emit_type(ret)
            params_e = [emit_type(p) for p in params]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params_e)) if params_e else "void"
            call = ", ".join(f"a{i}" for i in range(len(params_e)))
            kw = "return " if ret_e != "void" else ""
            fp.write(f"{ret_e} {f}({args}) {{ {kw}tglmt::gl::{b}({call}); }}\n")
        for f, _ in native:
            if f in seen:
                continue
            seen.add(f)
            ret, params = sig[f]
            ret_e = emit_type(ret)
            params_e = [emit_type(p) for p in params]
            args = ", ".join(f"{t} a{i}" for i, t in enumerate(params_e)) if params_e else "void"
            call = ", ".join(f"a{i}" for i in range(len(params_e)))
            kw = "return " if ret_e != "void" else ""
            fp.write(f"{ret_e} {f}({args}) {{ {kw}tglmt::gl::{f}({call}); }}\n")
        fp.write("} // extern \"C\"\n")
    print(f"\nWrote {OUT}: {len(seen)} functions (extern \"C\", "
          f"{len(forward)} alias + {len(native)} native).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
