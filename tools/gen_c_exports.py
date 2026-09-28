#!/usr/bin/env python3
"""Sinh C exports cho LWJGL/launcher dlsym: extern "C" wrappers plain-name
(glDrawElements, ...) forward sang tglmt::gl:: (C++ namespace, mangled).

LWJGL load opengl.libname bằng đường tuyệt đối và dlsym từng tên "glXxx"
không-mangle — không có lớp này game UnsatisfiedLinkError dù symbol C++
vẫn đủ 698/698. Wrappers là forward 1:1, không đổi hành vi.

Usage: python3 tools/gen_c_exports.py
Docs: docs/khronos/gl.xml (qua gl46.h đã sinh), COVERAGE.md
"""
import re, pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
GL46_H = ROOT / "include/tglmt/gl46.h"
OUT = ROOT / "src/gl/gl_c_exports.cpp"

decls = []
for line in GL46_H.read_text().splitlines():
    line = line.strip()
    if not line or line.startswith("//") or line.startswith("#") or line.startswith("namespace"):
        continue
    if line.startswith("void ") or line.startswith("GL") or line.startswith("const "):
        # dạng: "RET name(params); // ..."
        line = line.split("//")[0].strip()
        if not line.endswith(";"):
            continue
        line = line[:-1].strip()
        m = re.match(r"(.+?)\s+(gl[A-Za-z0-9_]+)\s*\((.*)\)\s*$", line)
        if not m:
            print("[c_exports] WARN skip:", line[:80])
            continue
        ret, name, params = m.group(1).strip(), m.group(2), m.group(3).strip()
        decls.append((ret, name, params))

# 3 bonus ngoài core (định nghĩa tay trong src/gl, không có trong gl46.h)
BONUS = ["glGetPointerv", "glLineStipple", "glDepthRangeArrayfvNV"]
have = {d[1] for d in decls}
for f in (ROOT / "src/gl").glob("*.cpp"):
    if f.name == "gl_c_exports.cpp":
        continue
    txt = f.read_text()
    for b in BONUS:
        if b in have:
            continue
        # tìm định nghĩa "RET b(params) {"
        m = re.search(r"([A-Za-z][\w\s\*]*?)\b" + b + r"\s*\(([^)]*)\)\s*\{", txt)
        if m:
            ret = " ".join(m.group(1).split())
            # bỏ tiền tố namespace nếu có trong cùng dòng (vd "void glX" đã sạch)
            ret = ret.split("::")[-1].strip()
            params = m.group(2).strip()
            decls.append((ret, b, params))
            have.add(b)

print(f"[c_exports] decls: {len(decls)} (core 698 + bonus {len(decls)-698})")

def param_names(params):
    """Tách tên biến từng param để forward (giữ nguyên srcX0, *const*strings...)."""
    if not params or params == "void":
        return []
    names = []
    for p in params.split(","):
        p = p.strip()
        if not p:
            continue
        p = p.split("=")[0].strip()  # bỏ default
        ids = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", p)
        # tên biến là identifier cuối (sau kiểu + *, vd "const GLchar *const*strings" → strings)
        names.append(ids[-1] if ids else f"a{len(names)}")
    return names

with open(OUT, "w") as f:
    f.write("// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.\n")
    f.write("// extern \"C\" forward 1:1 sang tglmt::gl:: để LWJGL dlsym(\"glXxx\") thấy.\n")
    f.write('#include "tglmt/gl46.h"\n')
    f.write("using namespace tglmt;\n")
    f.write("namespace tglmt { namespace gl {\n")
    f.write("void glGetPointerv(GLenum p, void** v);\n")
    f.write("void glLineStipple(GLint a, GLushort b);\n")
    f.write("void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c);\n")
    f.write("} }\n")
    f.write("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
    for ret, name, params in decls:
        plist = params if params else "void"
        args = ", ".join(param_names(params))
        f.write(f"{ret} {name}({plist}) {{\n")
        if ret.strip() == "void":
            f.write(f"    gl::{name}({args});\n")
        else:
            f.write(f"    return gl::{name}({args});\n")
        f.write("}\n")
    f.write("\n#ifdef __cplusplus\n} // extern \"C\"\n#endif\n")
print(f"[c_exports] wrote {OUT}")
