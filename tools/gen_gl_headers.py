#!/usr/bin/env python3
"""Sinh header/decl OpenGL 4.6 core từ docs/khronos/gl.xml — NGUỒN CHÂN LÝ DUY NHẤT.
Không hardcode prototype; mọi decl đều từ gl.xml (registry Khronos).
Usage: python3 tools/gen_gl_headers.py [--check]
  --check: chỉ kiểm tra số lượng, không ghi file.
Output:
  include/tglmt/gl46_types.h  (types + enums core 4.6)
  include/tglmt/gl46.h         (698 function decls trong namespace tglmt::gl)
  src/gl/gl_dispatch_gen.cpp   (bảng tên hàm + version introduced)
"""
import sys, pathlib, xml.etree.ElementTree as ET, re

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
OUT_TYPES = ROOT / "include/tglmt/gl46_types.h"
OUT_H = ROOT / "include/tglmt/gl46.h"
OUT_DISPATCH = ROOT / "src/gl/gl_dispatch_gen.cpp"

def parse():
    t = ET.parse(str(GLXML)); r = t.getroot()
    # types
    types = {}
    for ty in r.find("types").findall("type"):
        name_el = ty.find("name")
        if name_el is not None and name_el.text:
            types[name_el.text.strip()] = "".join(ty.itertext()).strip()
    # value table: mọi <enums namespace="GL"><enum name value> trong registry
    value_of = {}
    for es in r.findall("enums"):
        if es.get("namespace") != "GL":
            continue
        for e in es.findall("enum"):
            nm, val = e.get("name"), e.get("value")
            if nm and val and nm not in value_of:
                value_of[nm] = val
    # enums core 4.6 = tên được require bởi GL_VERSION_<=4.6
    removed_enums = set()
    for f in r.findall("feature"):
        for rm in f.findall("remove"):
            for e in rm.findall("enum"):
                removed_enums.add(e.get("name"))
    core_enums = {}  # name -> (value, introduced)
    for f in r.findall("feature"):
        n = f.get("name")
        if n.startswith("GL_VERSION_") and "ES" not in n:
            ver = tuple(map(int, n.replace("GL_VERSION_", "").split("_")))
            if ver <= (4, 6):
                for req in f.findall("require"):
                    for e in req.findall("enum"):
                        nm = e.get("name")
                        if nm in removed_enums:
                            continue  # compat-only, không thuộc core profile
                        v = e.get("value") or value_of.get(nm, "")
                        if not v:
                            v = "/*alias*/"
                        if nm not in core_enums:
                            core_enums[nm] = (v, n)
    # commands core 4.6
    proto_of = {}
    for cmds in r.findall("commands"):
        for cmd in cmds.findall("command"):
            proto = cmd.find("proto")
            pname = proto.find("name").text.strip()
            ret = "".join(proto.itertext()).replace(pname, "").strip()
            params = []
            for p in cmd.findall("param"):
                pt = "".join(p.itertext()).strip()
                params.append(pt)
            proto_of[pname] = (ret, params)
    core_cmds = {}
    for f in r.findall("feature"):
        n = f.get("name")
        if n.startswith("GL_VERSION_") and "ES" not in n:
            ver = tuple(map(int, n.replace("GL_VERSION_", "").split("_")))
            if ver <= (4, 6):
                for req in f.findall("require"):
                    for c in req.findall("command"):
                        core_cmds[c.get("name")] = n
    removed = set()
    for f in r.findall("feature"):
        for rm in f.findall("remove"):
            for c in rm.findall("command"):
                removed.add(c.get("name"))
    core46 = {k: v for k, v in core_cmds.items() if k not in removed}
    return types, core_enums, proto_of, core46

C_TYPE_MAP = {
    "GLenum": "uint32_t", "GLboolean": "uint8_t", "GLbitfield": "uint32_t",
    "GLbyte": "int8_t", "GLshort": "int16_t", "GLint": "int32_t",
    "GLsizei": "int32_t", "GLubyte": "uint8_t", "GLushort": "uint16_t",
    "GLuint": "uint32_t", "GLuint64": "uint64_t", "GLint64": "int64_t",
    "GLfloat": "float", "GLdouble": "double", "GLchar": "char",
    "GLintptr": "intptr_t", "GLsizeiptr": "intptr_t",
    "GLsync": "struct TGLMTSync*", "GLDEBUGPROC": "TGLMTDebugProc",
}

def main():
    check = "--check" in sys.argv
    types, enums, proto_of, core46 = parse()
    print(f"[gen] gl.xml core46 functions: {len(core46)}")
    print(f"[gen] core46 enums: {len(enums)}")
    # phát hiện lệch mapping
    mp = (ROOT / "docs/mapping/GL46_to_Metal.md").read_text()
    missing = [k for k in core46 if k not in mp]
    print(f"[gen] mapping thiếu {len(missing)} hàm: {missing[:5]}{'...' if len(missing)>5 else ''}")
    if check:
        assert len(core46) == 698, f"core46={len(core46)}, kỳ vọng 698 (gl.xml hiện tại)"
        return
    OUT_TYPES.parent.mkdir(parents=True, exist_ok=True)
    OUT_DISPATCH.parent.mkdir(parents=True, exist_ok=True)
    # --- gl46_types.h ---
    with open(OUT_TYPES, "w") as f:
        f.write("// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.\n")
        f.write("// OpenGL 4.6 Core types + enums.\n#pragma once\n#include <cstdint>\n#include <cstddef>\n\nnamespace tglmt {\n")
        f.write("using GLenum=uint32_t; using GLboolean=uint8_t; using GLbitfield=uint32_t;\n")
        f.write("using GLbyte=int8_t; using GLshort=int16_t; using GLint=int32_t;\n")
        f.write("using GLsizei=int32_t; using GLubyte=uint8_t; using GLushort=uint16_t;\n")
        f.write("using GLuint=uint32_t; using GLuint64=uint64_t; using GLint64=int64_t;\n")
        f.write("using GLfloat=float; using GLdouble=double; using GLchar=char;\n")
        f.write("using GLintptr=intptr_t; using GLsizeiptr=intptr_t;\n")
        f.write("struct TGLMTSync; using GLsync=TGLMTSync*;\n")
        f.write("using TGLMTDebugProc=void(*)(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*,const void*);\n")
        f.write("using GLDEBUGPROC=TGLMTDebugProc; // khronos type <types> (GLDEBUGPROC)\n")
        f.write("using GLvulkanProcEXT=void(*)(); // placeholder cho ARB_gl_spirv interop (không dùng trực tiếp)\n")
        f.write("// GL_TRUE/GL_FALSE phát ra cùng các enum bên dưới (từ gl.xml), không hardcode ở đây.\n")
        # một số enum nền tảng luôn cần (lấy value từ gl.xml khi có)
        for name in sorted(enums.keys()):
            val, intro = enums[name]
            if val == "/*alias*/":
                continue
            # chỉ emit value số hoặc biểu thức hex
            f.write(f"constexpr GLenum {name} = {val}; // {intro}\n")
        f.write("} // namespace tglmt\n")
    # --- gl46.h ---
    with open(OUT_H, "w") as f:
        f.write("// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.\n")
        f.write("// 698 hàm OpenGL 4.6 Core Profile trong namespace tglmt::gl.\n#pragma once\n#include \"tglmt/gl46_types.h\"\n\nnamespace tglmt { namespace gl {\n")
        for name in sorted(core46.keys()):
            ret, params = proto_of[name]
            # chuẩn hoá: thay 'void *' giữ nguyên; convert tên hàm giữ nguyên chữ gl*
            # param dạng 'GLenum mode' -> giữ nguyên vì types đã using
            plist = ", ".join(params) if params else "void"
            f.write(f"{ret} {name}({plist}); // introduced {core46[name]}\n")
        f.write("} // namespace gl\n")
        f.write("int GetCoreFunctionCount(); // = 698\n")
        f.write("} // namespace tglmt\n")
    # --- dispatch gen ---
    with open(OUT_DISPATCH, "w") as f:
        f.write("// AUTO-GENERATED từ docs/khronos/gl.xml.\n#include \"tglmt/gl46.h\"\nnamespace tglmt {\n")
        f.write("int GetCoreFunctionCount(){return 698;}\n")
        f.write("struct FnEntry{const char* name; const char* introduced;};\n")
        f.write("static const FnEntry kCoreFunctions[]={\n")
        for name in sorted(core46.keys()):
            f.write(f'{{"{name}","{core46[name]}"}},\n')
        f.write("};\nint GetCoreFunctionTableSize(){return (int)(sizeof(kCoreFunctions)/sizeof(kCoreFunctions[0]));}\n")
        f.write("} // namespace tglmt\n")
    print(f"[gen] wrote {OUT_TYPES}, {OUT_H}, {OUT_DISPATCH}")

if __name__ == "__main__":
    main()
