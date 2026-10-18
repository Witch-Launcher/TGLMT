#!/usr/bin/env python3
"""pack_glsl_preprocess.py — tiền xử lý 1 shader Iris/OptiFine-style (Complementary...).

Iris (1.11.x) chạy jcpp (C preprocessor) trên source pack trước khi patch AST:
- `#include "/lib/x.glsl"` (OptiFine style, path từ shaders root) và `#include "rel"`.
- `#moj_import <...>` (Iris style) → include tương ứng.
- `#if/#ifdef/#ifndef/#elif/#else/#endif/#define/#undef` với `defined()`.
Xuất ra source đã resolve để đưa vào converter TGLMT (đúng thứ tự game gặp).

Dùng:
  python3 tools/pack_glsl_preprocess.py PACK_DIR program/deferred1.glsl fsh \
      --define OVERWORLD --define FRAGMENT_SHADER [-o out.glsl]
  (mặc định in ra stdout; --list-programs để liệt kê program)
"""
import argparse
import os
import re
import sys

IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


def iris_standard_defines(repo_root=None):
    """Bộ define chuẩn Iris cho MÁY NÀY (darwin, TGLMT backend).

    Phải khớp runtime thật: nếu sai (vd thiếu MC_OS_MAC) pack sẽ giả lập sai
    nhánh — Complementary tắt COLORED_LIGHTING khi MC_OS_MAC.
    """
    d = {
        "MC_VERSION": "260102",           # mc 26.1.2
        "MC_MIPMAP_LEVEL": "4",
        "IRIS_VERSION": "11104",          # iris 1.11.4
        "MC_GL_VERSION": "460",           # glGetString(GL_VERSION) "4.6 (TGLMT Metal port)"
        "MC_GLSL_VERSION": "460",
        "MC_OS_MAC": "",
        "MC_GL_VENDOR_OTHER": "",         # vendor "TGLMT" → OTHER
        "MC_GL_RENDERER_OTHER": "",       # renderer "Apple GPU (Metal)" → OTHER
        "IS_IRIS": "",
        "IRIS_REQUIRES_SEPARATE_ENTITY_DRAWS": "",
        "MAX_COLOR_BUFFERS": "8",
        "IRIS_HAS_TRANSLUCENCY_SORTING": "",
        "IRIS_TAG_SUPPORT": "2",
        "MC_NORMAL_MAP": "",
        "MC_SPECULAR_MAP": "",
        "MC_RENDER_QUALITY": "1.0",
        "MC_SHADOW_QUALITY": "1.0",
        "MC_HAND_DEPTH": "0.125",
        # shaders.properties: iris.features.optional (usable → defined)
        "IRIS_FEATURE_CUSTOM_IMAGES": "",
        "IRIS_FEATURE_SSBO": "",
        "IRIS_FEATURE_BLOCK_EMISSION_ATTRIBUTE": "",
        "IRIS_FEATURE_FADE_VARIABLE": "",
        "IRIS_FEATURE_ENTITY_TRANSLUCENT": "",
        # supportsColorCorrection=true → COLOR_SPACE_* (enum ordinal)
        "COLOR_SPACE_sRGB": "0",
        "COLOR_SPACE_sRGB_LINEAR": "1",
        "COLOR_SPACE_ACES": "2",
    }
    for i, name in enumerate(["UNKNOWN", "LEAVES", "STONE", "WOOD", "METAL",
                              "DIRT", "LAVA", "DEEPSLATE", "SNOW", "SAND",
                              "TERRACOTTA", "NETHER_STONE", "WATER", "GRASS",
                              "AIR", "ILLUMINATED"]):
        d["DH_BLOCK_" + name] = str(i)
    # GL extension defines = danh sách TGLMT quảng cáo (Iris expose dạng tên trần).
    if repo_root is None:
        repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    glsrc = os.path.join(repo_root, "src", "gl", "gl_error_string_debug.cpp")
    try:
        txt = open(glsrc, encoding="utf-8").read()
        m = re.search(r"kExts\[\]\s*=\s*\{(.*?)\};", txt, re.S)
        if m:
            start = m.start(1)
            for em in re.finditer(r'"(GL_[A-Za-z0-9_]+)"', m.group(1)):
                d[em.group(1)] = ""
    except OSError:
        pass
    return d


class Macro:
    __slots__ = ("params", "body")

    def __init__(self, params, body):
        self.params = params  # None = object-like, list = function-like
        self.body = body


class Preprocessor:
    def __init__(self, root, defines):
        self.root = os.path.abspath(root)
        self.macros = {}
        for k, v in defines.items():
            self.macros[k] = Macro(None, v if v is not None else "1")
        self.out = []
        self.depth = 0

    # ---------- include ----------
    def read_include(self, ref, curdir):
        ref = ref.strip().strip('"<>')
        if ref.startswith("/"):
            p = os.path.join(self.root, ref.lstrip("/"))
        else:
            p = os.path.join(curdir, ref)
            if not os.path.exists(p):
                p = os.path.join(self.root, ref)
        return p

    # ---------- macro expansion ----------
    def expand(self, text, hide=frozenset()):
        out = []
        i = 0
        while i < len(text):
            m = IDENT.match(text, i)
            if not m:
                out.append(text[i])
                i += 1
                continue
            name = m.group(0)
            j = m.end()
            if name in self.macros and name not in hide:
                mac = self.macros[name]
                if mac.params is None:
                    out.append(self.expand(mac.body, hide | {name}))
                    i = j
                    continue
                # function-like: cần '(' sau tên
                k = j
                while k < len(text) and text[k] in " \t":
                    k += 1
                if k < len(text) and text[k] == "(":
                    args, end = self._read_args(text, k)
                    if args is not None:
                        body = mac.body
                        for p, a in zip(mac.params, args):
                            body = re.sub(r"\b%s\b" % re.escape(p), a, body)
                        out.append(self.expand(body, hide | {name}))
                        i = end
                        continue
            out.append(name)
            i = j
        return "".join(out)

    @staticmethod
    def _read_args(text, start):
        depth = 0
        i = start
        args = []
        cur = []
        while i < len(text):
            c = text[i]
            if c == "(":
                depth += 1
                if depth == 1:
                    i += 1
                    continue
            elif c == ")":
                depth -= 1
                if depth == 0:
                    args.append("".join(cur))
                    if len(args) == 1 and args[0].strip() == "":
                        args = []
                    return args, i + 1
            elif c == "," and depth == 1:
                args.append("".join(cur))
                cur = []
                i += 1
                continue
            cur.append(c)
            i += 1
        return None, start

    # ---------- #if expression ----------
    def strip_comments(self, s):
        s = re.sub(r"/\*.*?\*/", " ", s, flags=re.S)
        s = re.sub(r"//.*$", "", s)
        return s

    def eval_cond(self, expr):
        e = self.expand(expr)
        e = re.sub(r"defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)",
                   lambda m: "1" if m.group(1) in self.macros else "0", e)
        e = re.sub(r"defined\s+([A-Za-z_][A-Za-z0-9_]*)",
                   lambda m: "1" if m.group(1) in self.macros else "0", e)
        # jcpp: identifier còn lại (không macro) = 0
        e = IDENT.sub(lambda m: m.group(0) if m.group(0) in ("and", "or", "not") else "0", e)
        e = e.replace("&&", " and ").replace("||", " or ")
        e = re.sub(r"!(?!=)", " not ", e)
        # literal hậu tố 0u/0U
        e = re.sub(r"\b(\d+)[uUlL]+\b", r"\1", e)
        e = self.strip_comments(e)
        e = e.replace("/", "//")
        try:
            return bool(eval(e, {"__builtins__": {}}, {}))
        except Exception:
            return False

    # ---------- process ----------
    def process_text(self, text, curdir):
        lines = text.splitlines()
        stack = []  # (active_outer, any_taken, active_now, parent_active)
        active = True
        for line in lines:
            stripped = line.lstrip()
            if not stripped.startswith("#"):
                if active:
                    self.out.append(line)
                continue
            d = stripped[1:].strip()
            if not d:
                continue
            m = re.match(r"([A-Za-z_]+)\s*(.*)$", d, re.S)
            if not m:
                continue
            cmd, rest = m.group(1), m.group(2)

            if cmd in ("if", "ifdef", "ifndef"):
                parent = active
                if not parent:
                    cond = False
                elif cmd == "ifdef":
                    cond = rest.split()[0] in self.macros if rest.split() else False
                elif cmd == "ifndef":
                    cond = rest.split()[0] not in self.macros if rest.split() else False
                else:
                    cond = self.eval_cond(rest)
                stack.append([parent, cond, cond])
                active = parent and cond
                continue
            if cmd == "elif":
                if not stack:
                    continue
                parent, any_taken, _ = stack[-1]
                cond = False if any_taken or not parent else self.eval_cond(rest)
                stack[-1][1] = any_taken or cond
                stack[-1][2] = cond
                active = parent and cond
                continue
            if cmd == "else":
                if not stack:
                    continue
                parent, any_taken, _ = stack[-1]
                cond = parent and not any_taken
                stack[-1][1] = True
                stack[-1][2] = cond
                active = cond
                continue
            if cmd == "endif":
                if stack:
                    parent, _, _ = stack.pop()
                    active = parent
                continue
            if not active:
                continue
            if cmd == "define":
                mm = re.match(r"([A-Za-z_][A-Za-z0-9_]*)(\([^)]*\))?\s*(.*)$", rest, re.S)
                if not mm:
                    continue
                name = mm.group(1)
                plist = mm.group(2)
                body = self.strip_comments(mm.group(3)).strip()
                params = None
                if plist is not None:
                    params = [p.strip() for p in plist[1:-1].split(",") if p.strip()]
                self.macros[name] = Macro(params, body)
                continue
            if cmd == "undef":
                self.macros.pop(rest.split()[0], None) if rest.split() else None
                continue
            if cmd == "include":
                ref = rest.strip()
                path = self.read_include(ref, curdir)
                if os.path.exists(path) and self.depth < 64:
                    self.depth += 1
                    with open(path, encoding="utf-8", errors="replace") as f:
                        self.process_text(f.read(), os.path.dirname(path))
                    self.depth -= 1
                else:
                    self.out.append("#include " + rest)
                continue
            if cmd == "moj_import":
                # #moj_import <minecraft:xxx.glsl> / <file>
                mm = re.match(r"[<\"]([^>\"]*)[>\"]", rest.strip())
                if mm:
                    ref = mm.group(1)
                    if ":" in ref:
                        ref = ref.split(":", 1)[1]
                    path = self.read_include(ref, curdir)
                    if os.path.exists(path) and self.depth < 64:
                        self.depth += 1
                        with open(path, encoding="utf-8", errors="replace") as f:
                            self.process_text(f.read(), os.path.dirname(path))
                        self.depth -= 1
                continue
            if cmd in ("line", "pragma", "extension", "version"):
                if cmd != "version":
                    continue
                self.out.append(line)
                continue
            # directive lạ → bỏ
        return "\n".join(self.out) + "\n"


def disabled_programs(pack, defines):
    """Program bị tắt qua shaders.properties `program.<world>/<name>.enabled=false`
    (Iris bỏ hẳn — không compile). Trả set "world/name"."""
    props = os.path.join(os.path.abspath(pack), "shaders.properties")
    try:
        text = open(props, encoding="utf-8").read()
    except OSError:
        return set()
    pp = Preprocessor(pack, defines)
    out = pp.process_text(text, os.path.dirname(props))
    dis = set()
    for m in re.finditer(r"program\.([^\s=]+)\.enabled\s*=\s*false", out):
        dis.add(m.group(1))
    return dis


def list_programs(pack):
    progs = set()
    for base in [pack] + [os.path.join(pack, d) for d in os.listdir(pack)
                          if os.path.isdir(os.path.join(pack, d)) and d.startswith("world")]:
        pdir = os.path.join(base, "program")
        if not os.path.isdir(pdir):
            continue
        for fn in os.listdir(pdir):
            if fn.endswith((".vsh", ".fsh", ".glsl", ".gsh", ".tcs", ".tes", ".csh")):
                progs.add(fn.rsplit(".", 1)[0])
    return sorted(progs)


def patch_iris(text, stage):
    """Mô phỏng các patch AST mà Iris áp trước khi compile (mức đủ cho converter).

    - gl_FragColor → gl_FragData[0]; gl_FragData[N] → out vec4 iris_FragDataN
      (CommonTransformer).
    - Hàm legacy texture2D/textureCube → texture (CompatibilityTransformer).
    """
    frag = stage == "fsh"
    if frag:
        text = re.sub(r"\bgl_FragColor\b", "gl_FragData[0]", text)
        idxs = sorted({int(m) for m in re.findall(r"\bgl_FragData\s*\[\s*(\d+)\s*\]", text)})
        text = re.sub(r"\bgl_FragData\s*\[\s*\d+\s*\]",
                      lambda m: "iris_FragData" + re.search(r"\d+", m.group(0)).group(0), text)
        decls = "".join(f"layout (location = {i}) out vec4 iris_FragData{i};\n" for i in idxs)
        if decls:
            # chèn sau #version (Iris: BEFORE_DECLARATIONS)
            m = re.search(r"^[ \t]*#[ \t]*version[^\n]*\n", text, re.M)
            if m:
                text = text[:m.end()] + decls + text[m.end():]
            else:
                text = decls + text
    # CompatibilityTransformer: GLSL 120 `attribute`/`varying` → in/out.
    if stage == "vsh":
        text = re.sub(r"\battribute\b", "in", text)
        text = re.sub(r"\bvarying\b", "out", text)
    else:
        text = re.sub(r"\bvarying\b", "in", text)
    text = re.sub(r"\btexture2D\b", "texture", text)
    text = re.sub(r"\btextureCube\b", "texture", text)
    text = re.sub(r"\btexture2DLod\b", "textureLod", text)
    return text


def preprocess(pack, program, stage, defines, iris_patch=True):
    """Iris resolution: <program>.<stage> (vsh/fsh) rồi fallback <program>.glsl.

    Trả (source, path) đã resolve; hoặc (None, None) nếu pack không có.
    World folders: pack/world0 override (caller tự chọn root).
    """
    ext = {"vsh": "vsh", "fsh": "fsh", "gsh": "gsh", "tcs": "tcs", "tes": "tes", "csh": "csh"}[stage]
    cands = [os.path.join(pack, program + "." + ext),
             os.path.join(pack, program + ".glsl")]
    path = next((p for p in cands if os.path.exists(p)), None)
    if path is None:
        return None, None
    d = dict(defines)
    d.setdefault({"vsh": "VERTEX_SHADER", "fsh": "FRAGMENT_SHADER",
                  "gsh": "GEOMETRY_SHADER", "csh": "COMPUTE_SHADER",
                  "tcs": "TESS_CONTROL_SHADER", "tes": "TESS_EVALUATION_SHADER"}[stage], "1")
    p = Preprocessor(pack, d)
    with open(path, encoding="utf-8", errors="replace") as f:
        src = p.process_text(f.read(), os.path.dirname(path))
    if iris_patch:
        src = patch_iris(src, stage)
    return src, path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack")
    ap.add_argument("program", nargs="?")
    ap.add_argument("stage", nargs="?", default="fsh")
    ap.add_argument("--define", action="append", default=[])
    ap.add_argument("--list-programs", action="store_true")
    ap.add_argument("-o", "--output")
    args = ap.parse_args()

    if args.list_programs:
        for p in list_programs(args.pack):
            print(p)
        return 0

    defines = {}
    for d in args.define:
        if "=" in d:
            k, v = d.split("=", 1)
            defines[k] = v
        else:
            defines[d] = "1"

    src, path = preprocess(args.pack, args.program, args.stage, defines)
    if src is None:
        print(f"không tìm thấy program {args.program} ({args.stage})", file=sys.stderr)
        return 1
    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(src)
    else:
        sys.stdout.write(src)
    return 0


if __name__ == "__main__":
    sys.exit(main())
