#!/usr/bin/env python3
"""Sinh stub cho các hàm core 4.6 chưa được implement tay.
Đảm bảo link đủ 698/698 mà vẫn TRUNG THỰC: stub log UNIMPLEMENTED qua debug callback
và trả giá trị an toàn (0/null), không giả vờ pass.
Usage: python3 tools/gen_stubs.py
"""
import re, pathlib, xml.etree.ElementTree as ET, sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gl46_catalog

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
OUT = ROOT / "src/gl/gl_stubs_gen.cpp"

impl = set()
for f in (ROOT / "src/gl").glob("*.cpp"):
    if f.name == "gl_stubs_gen.cpp":
        continue
    txt = f.read_text()
    for m in re.finditer(r"\b(gl[A-Za-z0-9_]+)\s*\(", txt):
        impl.add(m.group(1))
    # macro định nghĩa hàm: PK2/3/4(glXxx,...), PU1(glXxx,...) — mọi MACRO(glXxx
    for m in re.finditer(r"\b[A-Z][A-Z0-9_]+\s*\(\s*(gl[A-Za-z0-9_]+)", txt):
        base = m.group(1)
        impl.add(base)
        # macro PK tự sinh thêm bản 'v' (vd glVertexP2ui -> glVertexP2uiv)
        if base.endswith("ui") and "PK" in txt[max(0, m.start()-200):m.start()+200]:
            impl.add(base + "v")

proto_of = gl46_catalog.parse_command_prototypes(GLXML)

# core46 — catalog chuẩn (657 hàm core thật) + 42 hàm compatibility-only.
# Cả hai nhóm đều phải có định nghĩa để link (gl46.h khai báo chúng).
core = gl46_catalog.parse_core_commands(GLXML)
compat = gl46_catalog.parse_compat_only_commands(GLXML)
core = dict(core)
for _k, _v in compat.items():
    core.setdefault(_k, _v)

missing = sorted([k for k in core if k not in impl])
print(f"[stubs] implemented tay: {len(impl)}, thiếu: {len(missing)}")
print("[stubs] missing sample:", missing[:10])

def default_return(ret):
    ret = ret.strip()
    if ret == "void":
        return None
    if "*" in ret:
        return "nullptr"
    if ret in ("GLboolean", "GLbyte", "GLubyte", "GLshort", "GLushort",
               "GLint", "GLuint", "GLsizei", "GLenum", "GLbitfield",
               "GLint64", "GLuint64", "GLfloat", "GLdouble", "GLchar"):
        return "0"
    if ret == "GLsync":
        return "nullptr"
    if ret in ("const GLubyte *", "const GLubyte*"):
        return "nullptr"
    return "0"

with open(OUT, "w") as f:
    f.write("// AUTO-GENERATED bởi tools/gen_stubs.py — stub TRUNG THỰC cho hàm chưa port tay.\n")
    f.write("// Mỗi stub ghi debug UNIMPLEMENTED + trả giá trị an toàn. Không giả pass.\n")
    f.write('#include "tglmt/gl46.h"\n#include "tglmt/Context.h"\nnamespace tglmt::gl {\n')
    for name in missing:
        ret, params = proto_of[name]
        plist = ", ".join(params) if params else "void"
        f.write(f"{ret} {name}({plist}) {{\n")
        f.write(f'    Context::Current().LogDebug(0,0,0,0,"UNIMPLEMENTED: {name} ({core[name]})");\n')
        d = default_return(ret)
        if d is not None:
            f.write(f"    return {d};\n")
        f.write("}\n")
    f.write("} // namespace tglmt::gl\n")
print(f"[stubs] wrote {OUT}")
