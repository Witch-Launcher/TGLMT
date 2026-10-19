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
    # StandardMacros.getRenderStages() — ordinal của WorldRenderingPhase (Iris 26.1).
    for i, name in enumerate([
        "NONE", "SKY", "SUNSET", "CUSTOM_SKY", "SUN", "MOON", "STARS", "VOID",
        "TERRAIN_SOLID", "TERRAIN_CUTOUT_MIPPED", "TERRAIN_CUTOUT", "ENTITIES",
        "BLOCK_ENTITIES", "DESTROY", "OUTLINE", "DEBUG", "HAND_SOLID",
        "TERRAIN_TRANSLUCENT", "TRIPWIRE", "PARTICLES", "CLOUDS", "RAIN_SNOW",
        "WORLD_BORDER", "HAND_TRANSLUCENT",
    ]):
        d["MC_RENDER_STAGE_" + name] = str(i)
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
        self.in_block_comment = False

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
        # `defined` phải xử lý TRƯỚC khi expand: macro body rỗng ("#define OVERWORLD")
        # nếu expand trước sẽ biến "defined OVERWORLD" thành "defined " → sai.
        e = re.sub(r"defined\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)",
                   lambda m: "1" if m.group(1) in self.macros else "0", expr)
        e = re.sub(r"defined\s+([A-Za-z_][A-Za-z0-9_]*)",
                   lambda m: "1" if m.group(1) in self.macros else "0", e)
        e = self.expand(e)
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
    def expand_code_line(self, line):
        """Expand macro trong dòng CODE (jcpp làm vậy), giữ comment nguyên
        (macro không nở trong comment). Block comment trạng thái qua dòng."""
        out = []
        i, n = 0, len(line)
        while i < n:
            if self.in_block_comment:
                j = line.find("*/", i)
                if j < 0:
                    out.append(line[i:])
                    break
                out.append(line[i:j + 2])
                self.in_block_comment = False
                i = j + 2
                continue
            j = line.find("/*", i)
            k = line.find("//", i)
            if j < 0 and k < 0:
                out.append(self.expand(line[i:]))
                break
            if k >= 0 and (j < 0 or k < j):
                out.append(self.expand(line[i:k]))
                out.append(line[k:])
                break
            out.append(self.expand(line[i:j]))
            self.in_block_comment = True
            i = j
        return "".join(out)

    @staticmethod
    def join_continuations(lines):
        out = []
        buf = None
        for ln in lines:
            if buf is not None:
                ln = buf + ln
                buf = None
            if ln.rstrip().endswith("\\"):
                buf = ln.rstrip()[:-1]
                continue
            out.append(ln)
        if buf is not None:
            out.append(buf)
        return out

    def process_text(self, text, curdir):
        lines = self.join_continuations(text.splitlines())
        stack = []  # (active_outer, any_taken, active_now, parent_active)
        active = True
        for line in lines:
            stripped = line.lstrip()
            if not stripped.startswith("#"):
                if active:
                    self.out.append(self.expand_code_line(line))
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


def _rewrite_call_args(text, name, wrap):
    """`name(args)` → `wrap(args)` với ngoặc cân bằng (giữ arg nguyên)."""
    out = []
    i = 0
    n = len(name)
    while True:
        p = text.find(name, i)
        if p < 0:
            out.append(text[i:])
            break
        lb = p > 0 and (text[p - 1].isalnum() or text[p - 1] == "_")
        e = p + n
        rb = e < len(text) and (text[e].isalnum() or text[e] == "_")
        if lb or rb:
            out.append(text[i:e])
            i = e
            continue
        q = e
        while q < len(text) and text[q] in " \t\n\r":
            q += 1
        if q >= len(text) or text[q] != "(":
            out.append(text[i:e])
            i = e
            continue
        d = 0
        k = q
        while k < len(text):
            if text[k] == "(":
                d += 1
            elif text[k] == ")":
                d -= 1
                if d == 0:
                    break
            k += 1
        if k >= len(text):
            out.append(text[i:])
            break
        args = text[q + 1:k]
        out.append(text[i:p])
        out.append(wrap(args))
        i = k + 1
    return "".join(out)


IRIS_FOG_COMPOSITE = """
uniform float iris_FogDensity;
uniform float iris_FogStart;
uniform float iris_FogEnd;
uniform vec4 iris_FogColor;
struct iris_FogParameters { vec4 color; float density; float start; float end; float scale; };
iris_FogParameters irisInt_Fog = iris_FogParameters(iris_FogColor, iris_FogDensity, iris_FogStart, iris_FogEnd, 1.0 / (iris_FogEnd - iris_FogStart));
"""

IRIS_FOG_VANILLA = """
layout(std140) uniform iris_Fog {
    vec4 FogColor;
    float FogEnvironmentalStart;
    float FogEnvironmentalEnd;
    float FogRenderDistanceStart;
    float FogRenderDistanceEnd;
    float FogSkyEnd;
    float FogCloudsEnd;
} iris_fogP;
struct iris_FogParameters { vec4 color; float density; float start; float end; float scale; };
iris_FogParameters irisInt_Fog = iris_FogParameters(iris_fogP.FogColor, 0.0, iris_fogP.FogEnvironmentalStart, iris_fogP.FogEnvironmentalEnd, 1.0 / (iris_fogP.FogEnvironmentalEnd - iris_fogP.FogEnvironmentalStart));
"""

IRIS_UBO_DYNAMIC_TRANSFORMS = """
layout(std140) uniform iris_DynamicTransforms {
    mat4 ModelViewMat;
    vec4 ColorModulator;
    vec3 ModelOffset;
    mat4 TextureMat;
} iris_transforms;
"""

IRIS_UBO_PROJECTION = """
layout(std140) uniform iris_Projection {
    mat4 iris_ProjMat;
};
"""

IRIS_UBO_GLOBALS = """
layout(std140) uniform iris_Globals {
    ivec3 CameraBlockPos;
    vec3 CameraOffset;
    vec2 ScreenSize;
    float GlintAlpha;
    float GameTime;
    int MenuBlurRadius;
} iris_globalInfo;
"""


def patch_iris(text, stage, kind="composite", ie=False, newlines=False):
    """Mô phỏng các patch AST mà Iris áp trước khi compile (mức đủ cho converter).

    - gl_FragColor → gl_FragData[0]; gl_FragData[N] → out vec4 iris_FragDataN
      (CommonTransformer).
    - Hàm legacy texture2D/textureCube/... → texture (CompatibilityTransformer).
    - shadow2D(x, y) → vec4(texture(x, y)) (CommonTransformer.renameAndWrapShadow).
    - gl_Fog → irisInt_Fog + inject struct/global (CommonTransformer).
    - ie=True: nhánh `parameters.inputs.isIE()` (IE_COMPAT/IE_COMPAT_SHADOW —
      Iris load mọi ShaderKey.values()): `gl_MultiTexCoord1` →
      `vec4(iris_LightUV, 0.0, 1.0)` + `uniform ivec2 iris_LightUV;`
      (VanillaTransformer.java:92-96) thay vì attribute `iris_UV2`.
    - newlines=True (SHADOW_LINES): VanillaTransformer.java:196-224 — rename
      main → irisMain + inject `iris_vertex_offset`, `iris_widen_lines` và
      wrapper main (gl_Vertex → `vec4(iris_Position + iris_vertex_offset, 1.0)`).
      Helper Iris sinh ra dùng `gl_Position` TRỰC TIẾP (case thật trong game).
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
    text = re.sub(r"\btexture3D\b", "texture", text)
    text = re.sub(r"\btextureCube\b", "texture", text)
    text = re.sub(r"\btexture2DLod\b", "textureLod", text)
    text = re.sub(r"\btexture3DLod\b", "textureLod", text)
    text = re.sub(r"\btexture2DProj\b", "textureProj", text)
    text = re.sub(r"\btexture3DProj\b", "textureProj", text)
    text = re.sub(r"\btexture2DGrad(?:ARB)?\b", "textureGrad", text)
    text = re.sub(r"\btexture3DGrad(?:ARB)?\b", "textureGrad", text)
    text = re.sub(r"\btexelFetch2D\b", "texelFetch", text)
    text = re.sub(r"\btexelFetch3D\b", "texelFetch", text)
    text = re.sub(r"\btextureSize2D\b", "textureSize", text)
    # shadow2D(s, c) → vec4(texture(s, c)); shadow2DLod → vec4(textureLod(...)).
    if "shadow2D" in text:
        text = _rewrite_call_args(text, "shadow2DLod", lambda a: f"vec4(textureLod({a}))")
        text = _rewrite_call_args(text, "shadow2D", lambda a: f"vec4(texture({a}))")
    # CompositeTransformer (deferred/composite/final): screenquad không attribute
    # GL cũ — thay hằng số. Thứ tự theo CompositeTransformer.java.
    if kind == "composite":
        prelude = ""
        if stage == "vsh" and re.search(r"\bgl_MultiTexCoord0\b", text):
            prelude += "in vec2 UV0;\n"
        has_ftransform = stage == "vsh" and re.search(r"\bftransform\s*\(", text)
        if has_ftransform or (stage == "vsh" and re.search(r"\bgl_Vertex\b", text)):
            prelude += "in vec3 Position;\n"
        if has_ftransform:
            prelude += "vec4 ftransform() { return gl_ModelViewProjectionMatrix * gl_Vertex; }\n"
        # Chèn prelude TRƯỚC khi replace để các regex phủ luôn thân ftransform
        # (Iris inject def rồi mới thay gl_Vertex/MVP — cùng kết quả).
        if prelude:
            m = re.search(r"^[ \t]*#[ \t]*version[^\n]*\n", text, re.M)
            if m:
                text = text[:m.end()] + prelude + text[m.end():]
            else:
                text = prelude + text
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*[0-7]\s*\]", "mat4(1.0)", text)
        if stage == "vsh":
            text = re.sub(r"\bgl_MultiTexCoord0\b", "vec4(UV0, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord[1-7]\b", "vec4(0.0, 0.0, 0.0, 1.0)", text)
        text = re.sub(r"\bgl_Color\b", "vec4(1.0, 1.0, 1.0, 1.0)", text)
        text = re.sub(r"\bgl_NormalMatrix\b", "mat3(1.0)", text)
        if stage == "vsh":
            text = re.sub(r"\bgl_Normal\b", "vec3(0.0, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_Vertex\b", "vec4(Position, 1.0)", text)
            text = re.sub(r"\bgl_ModelViewProjectionMatrix\b",
                          "(gl_ProjectionMatrix * gl_ModelViewMatrix)", text)
        text = re.sub(r"\bgl_ModelViewMatrix\b", "mat4(1.0)", text)
        text = re.sub(r"\bgl_ProjectionMatrix\b",
                      "mat4(vec4(2.0, 0.0, 0.0, 0.0), vec4(0.0, 2.0, 0.0, 0.0), vec4(0.0), vec4(-1.0, -1.0, 0.0, 1.0))",
                      text)
    # VanillaTransformer (nhánh non-core — runtime thật của gbuffers/shadow với pack
    # `#version 130`). LƯU Ý: khi Sodium có mặt, terrain/water/shadow runtime dùng
    # SodiumTransformer (attribute sodium `_vert_*`) — harness vẫn mô phỏng bản VANILLA:
    # thân pack giống nhau, khác plumbing attribute; đủ để bắt lỗi converter/MSL.
    if kind == "vanilla":
        prelude = "const float mc_chunkFade = -1.0;\n"  # VanillaTransformer.java:30
        has_ftransform = stage == "vsh" and re.search(r"\bftransform\s*\(", text)
        # ftransform/MVP được expand thành gl_ModelViewMatrix/gl_ProjectionMatrix
        # ở dưới → tính trước như đã dùng (nếu không UBO/helper thiếu).
        uses_mvp = bool(re.search(r"\bgl_ModelViewProjectionMatrix\b", text)) or has_ftransform
        uses_mvm = bool(re.search(r"\bgl_ModelViewMatrix\b", text)) or uses_mvp
        uses_proj = bool(re.search(r"\bgl_ProjectionMatrix\b", text)) or uses_mvp
        if uses_mvm or re.search(r"\bgl_TextureMatrix\s*\[\s*0\s*\]|\bgl_Color\b", text):
            prelude += IRIS_UBO_DYNAMIC_TRANSFORMS
        if uses_proj:
            prelude += IRIS_UBO_PROJECTION
        # Helper phải đứng TRƯỚC ftransform/defs dùng nó (MSL cần khai báo trước).
        if uses_mvm:
            prelude += ("mat4 _iris_internal_translate(vec3 offset) {"
                        "return mat4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,"
                        "0.0, 0.0, 1.0, 0.0, offset.x, offset.y, offset.z, 1.0); }\n")
        if stage == "vsh":
            if re.search(r"\bgl_Color\b", text):
                prelude += "in vec4 iris_Color;\n"
            if re.search(r"\bgl_MultiTexCoord0\b", text):
                prelude += "in vec2 iris_UV0;\n"
            if re.search(r"\bgl_MultiTexCoord[12]\b", text):
                prelude += ("uniform ivec2 iris_LightUV;\n" if ie
                            else "in ivec2 iris_UV2;\n")
            if re.search(r"\bgl_Normal\b", text):
                prelude += "in vec3 iris_Normal;\n"
            if re.search(r"\bgl_Vertex\b", text) or has_ftransform:
                prelude += "in vec3 iris_Position;\n"
            if newlines:
                # VanillaTransformer.java:76-78 (mọi VS vanilla) + nhánh isNewLines
                # (:200 — BEFORE_DECLARATIONS < BEFORE_FUNCTIONS của ftransform,
                # giữ đúng thứ tự Iris). Wrapper dùng iris_Normal → chắc chắn có
                # khai báo (format lines có Normal; pack không dùng gl_Normal thì
                # ở trên chưa thêm).
                prelude += "in float iris_LineWidth;\n"
                if not re.search(r"\bgl_Normal\b", text):
                    prelude += "in vec3 iris_Normal;\n"
                prelude += "vec3 iris_vertex_offset = vec3(0.0);\n"
                prelude += IRIS_UBO_GLOBALS
            if has_ftransform:
                prelude += "vec4 ftransform() { return gl_ModelViewProjectionMatrix * gl_Vertex; }\n"
        if re.search(r"\bgl_NormalMatrix\b", text):
            prelude += "uniform mat3 iris_NormalMat;\n"
        if re.search(r"\bgl_TextureMatrix\s*\[\s*1\s*\]", text):
            prelude += "uniform mat4 iris_LightmapTextureMatrix;\n"
        if re.search(r"\bgl_ModelViewMatrixInverse\b", text):
            prelude += "uniform mat4 iris_ModelViewMatInverse;\n"
        if re.search(r"\bgl_ProjectionMatrixInverse\b", text):
            prelude += "uniform mat4 iris_ProjMatInverse;\n"
        if prelude:
            m = re.search(r"^[ \t]*#[ \t]*version[^\n]*\n", text, re.M)
            if m:
                text = text[:m.end()] + prelude + text[m.end():]
            else:
                text = prelude + text
        text = re.sub(r"\bgl_ModelViewMatrixInverse\b", "iris_ModelViewMatInverse", text)
        text = re.sub(r"\bgl_ProjectionMatrixInverse\b", "iris_ProjMatInverse", text)
        text = re.sub(r"\bgl_ModelViewProjectionMatrix\b",
                      "(gl_ProjectionMatrix * gl_ModelViewMatrix)", text)
        text = re.sub(r"\bgl_ModelViewMatrix\b",
                      "(iris_transforms.ModelViewMat * _iris_internal_translate(iris_transforms.ModelOffset))",
                      text)
        text = re.sub(r"\bgl_ProjectionMatrix\b", "iris_ProjMat", text)
        text = re.sub(r"\bgl_NormalMatrix\b", "iris_NormalMat", text)
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*0\s*\]", "iris_transforms.TextureMat", text)
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*1\s*\]", "iris_LightmapTextureMatrix", text)
        if stage == "vsh":
            # isNewLines: `gl_Vertex → vec4(iris_Position + iris_vertex_offset, 1.0)`
            # (VanillaTransformer.java:199).
            text = re.sub(r"\bgl_Vertex\b",
                          "vec4(iris_Position + iris_vertex_offset, 1.0)" if newlines
                          else "vec4(iris_Position, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord0\b", "vec4(iris_UV0, 0.0, 1.0)", text)
            # ĐÚNG dạng Iris (VanillaTransformer.java:101): `vec4(iris_UV2, 0.0, 1.0)`
            # với `in ivec2 iris_UV2;` (implicit int→float của GLSL). Từng bị
            # `vec2(iris_UV2)` ở đây = workaround che lỗi converter → game thật
            # fail "no matching constructor" (TERRAIN_SOLID). Converter phải tự
            # bọc float2 — giữ nguyên dạng Iris để sweep bắt được hồi quy.
            # Nhánh IE (:92-96): `vec4(iris_LightUV, 0.0, 1.0)` + uniform ivec2.
            text = re.sub(r"\bgl_MultiTexCoord[12]\b",
                          "vec4(iris_LightUV, 0.0, 1.0)" if ie
                          else "vec4(iris_UV2, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord[3-7]\b", "vec4(0.0, 0.0, 0.0, 1.0)", text)
            if re.search(r"\bgl_Normal\b", text):
                text = re.sub(r"\bgl_Normal\b", "iris_Normal", text)
            if re.search(r"\bgl_Color\b", text):
                text = re.sub(r"\bgl_Color\b",
                              "(iris_Color * iris_transforms.ColorModulator)", text)
        else:
            text = re.sub(r"\bgl_Color\b", "iris_transforms.ColorModulator", text)
        if newlines and stage == "vsh":
            # VanillaTransformer.java:196-224 (isNewLines) — rename main → irisMain
            # + sinh iris_widen_lines/wrapper main (gl_Position dùng trong HELPER
            # là case thật trong game: SHADOW_LINES "use of undeclared identifier
            # 'gl_Position'"; harness cũ không mô phỏng nên lọt khỏi sweep).
            text = re.sub(r"\bvoid\s+main\s*\(", "void irisMain(", text, count=1)
            text += (
                "\nvoid iris_widen_lines(vec4 linePosStart, vec4 linePosEnd) {"
                "vec3 ndc1 = linePosStart.xyz / linePosStart.w;"
                "vec3 ndc2 = linePosEnd.xyz / linePosEnd.w;"
                "vec2 lineScreenDirection = normalize((ndc2.xy - ndc1.xy) * iris_globalInfo.ScreenSize);"
                "vec2 lineOffset = vec2(-lineScreenDirection.y, lineScreenDirection.x) * iris_LineWidth"
                " / iris_globalInfo.ScreenSize;"
                "if (lineOffset.x < 0.0) { lineOffset *= -1.0; }"
                "if (gl_VertexID % 2 == 0) {"
                "gl_Position = vec4((ndc1 + vec3(lineOffset, 0.0)) * linePosStart.w, linePosStart.w);"
                "} else {"
                "gl_Position = vec4((ndc1 - vec3(lineOffset, 0.0)) * linePosStart.w, linePosStart.w);"
                "}}\n"
                "void main() {"
                "iris_vertex_offset = iris_Normal;"
                "irisMain();"
                "vec4 linePosEnd = gl_Position;"
                "gl_Position = vec4(0.0);"
                "iris_vertex_offset = vec3(0.0);"
                "irisMain();"
                "vec4 linePosStart = gl_Position;"
                "iris_widen_lines(linePosStart, linePosEnd);}\n")
    # VanillaCoreTransformer (isLines + VANILLA → Iris bump #version 330 core).
    # TransformPatcher: LINES → core branch. Rename theo VanillaCoreTransformer.java.
    if kind == "vanilla_core":
        prelude = "const float mc_chunkFade = -1.0;\n"                 # :29
        # addIfNotExists(...) — Iris inject cả 6 attr bất kể dùng hay không (:128-134)
        if stage == "vsh":
            prelude += ("in vec3 iris_Position;\nin vec3 iris_Normal;\nin vec2 iris_UV0;\n"
                        "in vec2 iris_UV1;\nin vec2 iris_UV2;\nin vec4 iris_Color;\n")
        for re_use, block in ((r"\bmodelViewMatrix\b|\bgl_ModelViewMatrix\b", IRIS_UBO_DYNAMIC_TRANSFORMS),
                              (r"\bprojectionMatrix\b|\bgl_ProjectionMatrix\b", IRIS_UBO_PROJECTION)):
            if re.search(re_use, text):
                prelude += block
        if re.search(r"\bnormalMatrix\b|\bgl_NormalMatrix\b", text):
            prelude += "uniform mat3 iris_NormalMat;\n"
        if re.search(r"\balphaTestRef\b", text):
            prelude += "uniform float iris_currentAlphaTest;\n"      # :75 (CommonTransformer)
        has_ftransform = stage == "vsh" and re.search(r"\bftransform\s*\(", text)
        if has_ftransform:
            prelude += ("vec4 iris_ftransform() { return iris_ProjMat * iris_transforms."
                        "ModelViewMat * vec4(iris_Position, 1.0); }\n")
        if prelude:
            m = re.search(r"^[ \t]*#[ \t]*version[^\n]*\n", text, re.M)
            if m:
                text = text[:m.end()] + prelude + text[m.end():]
            else:
                text = prelude + text
        if has_ftransform:
            text = re.sub(r"\bftransform\b", "iris_ftransform", text)
        text = re.sub(r"\bvaPosition\b", "iris_Position", text)          # :102
        text = re.sub(r"\bvaColor\b", "(iris_Color * iris_transforms.ColorModulator)", text)
        text = re.sub(r"\bvaNormal\b", "iris_Normal", text)              # :107
        text = re.sub(r"\bvaUV0\b", "iris_UV0", text)                    # :109
        text = re.sub(r"\bvaUV1\b", "iris_UV1", text)                    # :125
        text = re.sub(r"\bvaUV2\b", "iris_UV2", text)                    # :114/:123
        text = re.sub(r"\balphaTestRef\b", "iris_currentAlphaTest", text)  # :75
        text = re.sub(r"\bmodelViewMatrix\b", "iris_transforms.ModelViewMat", text)  # :76
        text = re.sub(r"\bgl_ModelViewMatrix\b", "iris_transforms.ModelViewMat", text)
        text = re.sub(r"\bprojectionMatrix\b", "iris_ProjMat", text)     # :80
        text = re.sub(r"\bgl_ProjectionMatrix\b", "iris_ProjMat", text)
        text = re.sub(r"\btextureMatrix\b", "iris_transforms.TextureMat", text)  # :83
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*0\s*\]",
                      "iris_transforms.TextureMat", text)                # :85
        _lm_mat = ("mat4(vec4(0.00390625, 0.0, 0.0, 0.0), vec4(0.0, 0.00390625, 0.0, 0.0), "
                   "vec4(0.0, 0.0, 0.00390625, 0.0), vec4(0.03125, 0.03125, 0.03125, 1.0))")
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*1\s*\]", _lm_mat, text)  # :86-89
        text = re.sub(r"\bgl_TextureMatrix\s*\[\s*2\s*\]", _lm_mat, text)
        text = re.sub(r"\bgl_ModelViewMatrixInverse\b", "iris_ModelViewMatInverse", text)
        text = re.sub(r"\bgl_ProjectionMatrixInverse\b", "iris_ProjMatInverse", text)
        text = re.sub(r"\bnormalMatrix\b", "iris_NormalMat", text)       # :92
        text = re.sub(r"\bgl_NormalMatrix\b", "iris_NormalMat", text)
        text = re.sub(r"\bchunkOffset\b", "iris_transforms.ModelOffset", text)  # :94
        if stage == "vsh":
            text = re.sub(r"\bgl_Vertex\b", "vec4(iris_Position, 1.0)", text)
            text = re.sub(r"\bgl_Normal\b", "iris_Normal", text)
            text = re.sub(r"\bgl_MultiTexCoord0\b", "vec4(iris_UV0, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord1\b", "vec4(iris_UV2, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord2\b", "vec4(iris_UV2, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_MultiTexCoord[3-7]\b", "vec4(0.0, 0.0, 0.0, 1.0)", text)
            text = re.sub(r"\bgl_Color\b",
                          "(iris_Color * iris_transforms.ColorModulator)", text)
        else:
            text = re.sub(r"\bgl_Color\b", "iris_transforms.ColorModulator", text)
    # CommonTransformer fog: `gl_Fog` → `irisInt_Fog` + inject (chỉ khi dùng).
    if re.search(r"\bgl_Fog\b", text):
        text = re.sub(r"\bgl_Fog\b", "irisInt_Fog", text)
        inj = IRIS_FOG_VANILLA if kind == "vanilla" else IRIS_FOG_COMPOSITE
        m = re.search(r"^[ \t]*#[ \t]*version[^\n]*\n", text, re.M)
        if m:
            text = text[:m.end()] + inj + text[m.end():]
        else:
            text = inj + text
    return text


def patch_kind(program):
    """Program nào Iris compile qua VanillaParameters vs composite (Parameters)."""
    program = program.rsplit("/", 1)[-1]  # bỏ prefix world dir (`world0/gbuffers_water`)
    # isLines + VANILLA → TransformPatcher bump core (VanillaCoreTransformer).
    if program.startswith("gbuffers_line"):
        return "vanilla_core"
    if program.startswith(("gbuffers", "shadow", "clrwl", "dh_")):
        return "vanilla"
    return "composite"


def preprocess(pack, program, stage, defines, iris_patch=True, ie=False, newlines=False):
    """Iris resolution: <program>.<stage> (vsh/fsh) rồi fallback <program>.glsl.

    Trả (source, path) đã resolve; hoặc (None, None) nếu pack không có.
    World folders: pack/world0 override (caller tự chọn root).
    ie=True: biến thể IE_COMPAT (iris_LightUV) — xem patch_iris.
    newlines=True: biến thể SHADOW_LINES (irisMain/iris_widen_lines) — xem patch_iris.
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
        src = patch_iris(src, stage, patch_kind(program), ie=ie, newlines=newlines)
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
