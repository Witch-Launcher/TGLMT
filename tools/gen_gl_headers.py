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

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gl46_catalog

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
    # enums của extension: name -> ext name (chỉ emit cho ext đang advertise,
    # xem advertised_exts() — nếu không, generator sẽ xoá mất ext enum mỗi lần gen).
    ext_enums = {}
    for exts in r.findall("extensions"):
        for ext in exts.findall("extension"):
            en = ext.get("name")
            for req in ext.findall("require"):
                for e in req.findall("enum"):
                    nm = e.get("name")
                    if nm and nm not in ext_enums:
                        ext_enums[nm] = (value_of.get(nm, ""), en)
    # commands core 4.6 — dùng catalog chuẩn (khronos reg.py semantics).
    # Xem tools/gl46_catalog.py: 657 hàm core thật + 42 hàm chỉ có ở
    # compatibility profile (vẫn khai báo vì extension alias cần tới).
    proto_of = gl46_catalog.parse_command_prototypes(GLXML)
    core46 = gl46_catalog.parse_core_commands(GLXML)
    compat46 = gl46_catalog.parse_compat_only_commands(GLXML)
    return types, core_enums, proto_of, core46, ext_enums, compat46

def advertised_exts():
    """Đọc kExts[] trong gl_error_string_debug.cpp — nguồn sự thật ext đang advertise."""
    src = (ROOT / "src/gl/gl_error_string_debug.cpp").read_text()
    m = re.search(r"kExts\[\]\s*=\s*\{(.*?)\};", src, re.S)
    return set(re.findall(r'"(GL_[A-Za-z0-9_]+)"', m.group(1))) if m else set()

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
    types, enums, proto_of, core46, ext_enums, compat46 = parse()
    print(f"[gen] gl.xml core46 functions: {len(core46)}")
    print(f"[gen] gl.xml compatibility-only functions: {len(compat46)}")
    print(f"[gen] core46 enums: {len(enums)}")
    adv = advertised_exts()
    ext_extra = sorted((nm, ext_enums[nm][0], ext_enums[nm][1]) for nm in ext_enums
                       if ext_enums[nm][1] in adv and nm not in enums)
    print(f"[gen] advertised exts: {len(adv)}; ext enums thêm: {len(ext_extra)}")
    # phát hiện lệch mapping
    mp = (ROOT / "docs/mapping/GL46_to_Metal.md").read_text()
    missing = [k for k in core46 if k not in mp]
    print(f"[gen] mapping thiếu {len(missing)} hàm: {missing[:5]}{'...' if len(missing)>5 else ''}")
    if check:
        assert len(core46) == gl46_catalog.CORE_FUNCTION_COUNT, \
            f"core46={len(core46)}, kỳ vọng {gl46_catalog.CORE_FUNCTION_COUNT}"
        assert len(compat46) == gl46_catalog.COMPAT_ONLY_FUNCTION_COUNT, \
            f"compat46={len(compat46)}, kỳ vọng {gl46_catalog.COMPAT_ONLY_FUNCTION_COUNT}"
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
        # enums của ext đang advertise (bền qua mỗi lần regenerate; nếu thiếu,
        # ext_coverage --kexts sẽ fail vì ext advertise mà enum chưa define).
        for nm, val, en in ext_extra:
            if val and val != "/*alias*/":
                f.write(f"constexpr GLenum {nm} = {val}; // {en}\n")
        f.write("} // namespace tglmt\n")
    # --- gl46.h ---
    with open(OUT_H, "w") as f:
        f.write("// AUTO-GENERATED từ docs/khronos/gl.xml — KHÔNG SỬA TAY.\n")
        f.write(f"// {len(core46)} hàm OpenGL 4.6 CORE Profile trong namespace tglmt::gl.\n")
        f.write(f"// + {len(compat46)} hàm chỉ có ở COMPATIBILITY profile (không phải core).\n")
        f.write("// Nguồn đếm: tools/gl46_catalog.py (khronos reg.py PASS 1 semantics).\n")
        f.write("#pragma once\n#include \"tglmt/gl46_types.h\"\n\nnamespace tglmt { namespace gl {\n")
        for name in sorted(core46.keys()):
            ret, params = proto_of[name]
            # chuẩn hoá: thay 'void *' giữ nguyên; convert tên hàm giữ nguyên chữ gl*
            # param dạng 'GLenum mode' -> giữ nguyên vì types đã using
            plist = ", ".join(params) if params else "void"
            f.write(f"{ret} {name}({plist}); // introduced {core46[name]}\n")
        f.write("// --- OpenGL 4.6 COMPATIBILITY-profile-only (KHÔNG thuộc Core Profile) ---\n")
        f.write("// Cần cho GL_ARB_vertex_type_2_10_10_10_rev + GL_ARB_robustness aliases.\n")
        for name in sorted(compat46.keys()):
            ret, params = proto_of[name]
            plist = ", ".join(params) if params else "void"
            f.write(f"{ret} {name}({plist}); // COMPAT-ONLY, introduced {compat46[name]}\n")
        f.write("} // namespace gl\n")
        f.write(f"int GetCoreFunctionCount(); // = {len(core46)}\n")
        f.write(f"int GetCompatOnlyFunctionCount(); // = {len(compat46)}\n")
        f.write("const char* GetCoreFunctionName(int i);\n")
        f.write("int GetCoreFunctionTableSize();\n")
        # glGetStringi liệt kê KN extensions; glGet(GL_NUM_EXTENSIONS) phải khớp
        # tuyệt đối (định nghĩa trong gl_error_string_debug.cpp).
        f.write("GLuint tglmt_num_extensions();\n")
        f.write("} // namespace tglmt\n")
    # --- dispatch gen ---
    with open(OUT_DISPATCH, "w") as f:
        f.write("// AUTO-GENERATED từ docs/khronos/gl.xml.\n#include \"tglmt/gl46.h\"\nnamespace tglmt {\n")
        f.write(f"int GetCoreFunctionCount(){{return {len(core46)};}}\n")
        f.write(f"int GetCompatOnlyFunctionCount(){{return {len(compat46)};}}\n")
        f.write("struct FnEntry{const char* name; const char* introduced;};\n")
        f.write("static const FnEntry kCoreFunctions[]={\n")
        for name in sorted(core46.keys()):
            f.write(f'{{"{name}","{core46[name]}"}},\n')
        f.write("};\nint GetCoreFunctionTableSize(){return (int)(sizeof(kCoreFunctions)/sizeof(kCoreFunctions[0]));}\n")
        f.write("// Tên hàm core thứ i (để test đối chiếu symbol export).\n")
        f.write("const char* GetCoreFunctionName(int i){"
                "return (i>=0 && i<GetCoreFunctionTableSize()) ? kCoreFunctions[i].name : nullptr;}\n")
        f.write("static const FnEntry kCompatOnlyFunctions[]={\n")
        for name in sorted(compat46.keys()):
            f.write(f'{{"{name}","{compat46[name]}"}},\n')
        f.write("};\nint GetCompatOnlyFunctionTableSize(){return (int)(sizeof(kCompatOnlyFunctions)/sizeof(kCompatOnlyFunctions[0]));}\n")
        f.write("} // namespace tglmt\n")
    print(f"[gen] wrote {OUT_TYPES}, {OUT_H}, {OUT_DISPATCH}")

if __name__ == "__main__":
    main()
