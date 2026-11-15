#!/usr/bin/env python3
"""Sinh C exports cho LWJGL/launcher dlsym: extern "C" wrappers plain-name
(glDrawElements, ...) forward sang tglmt::gl:: (C++ namespace, mangled).

LWJGL load opengl.libname bằng đường tuyệt đối và dlsym từng tên "glXxx"
không-mangle — không có lớp này game UnsatisfiedLinkError dù symbol C++
vẫn đủ 698/698. Wrappers là forward 1:1, không đổi hành vi.

Mỗi wrapper có version-gate: entry point không tồn tại trong (version, profile)
của context hiện tại ⇒ GL_INVALID_OPERATION, không thực thi (GL 4.6 Core spec
§2.3). Bảng gate sinh từ docs/khronos/gl.xml bởi tools/gen_version_gate.py.

Usage: python3 tools/gen_c_exports.py
Docs: docs/khronos/gl.xml (qua gl46.h đã sinh), COVERAGE.md
"""
import re, pathlib, sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gen_version_gate as gvg

ROOT = pathlib.Path(__file__).resolve().parents[1]
GL46_H = ROOT / "include/tglmt/gl46.h"
GL_LEGACY_H = ROOT / "include/tglmt/gl_legacy.h"
OUT = ROOT / "src/gl/gl_c_exports.cpp"
# Public C ABI header (đây mới là API "chính thức" của thư viện).
OUT_C_API = ROOT / "include/tglmt/gl_c_api.h"

decls = []
for hdr in (GL46_H, GL_LEGACY_H):
    for line in hdr.read_text().splitlines():
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

# Extension entry points đã export extern "C" ở gl_ext_aliases_gen.cpp (và các
# file gl_ext_*.cpp). Chúng KHÔNG thuộc bảng version-gate của core GL (gate
# bằng extension string), nhưng PHẢI có trong bảng tra getProcAddress.
ext_names = set()
ext_sigs = {}
for f in sorted((ROOT / "src/gl").glob("gl_ext*.cpp")):
    txt = f.read_text()
    m = re.search(r'extern "C"\s*\{(.*?)\n\}\s*//\s*extern "C"', txt, re.S)
    body = m.group(1) if m else ""
    for mm in re.finditer(r"^\s*([A-Za-z_][\w:<>\* ]*?)\b(gl[A-Za-z0-9_]+)\s*\(([^;{}]*)\)\s*\{",
                          body, re.M):
        nm = mm.group(2)
        ext_names.add(nm)
        ext_sigs[nm] = (mm.group(1).strip(), mm.group(3).strip())
ext_names -= have
print(f"[c_exports] extension entry points (getProcAddress only): {len(ext_names)}")

print(f"[c_exports] decls: {len(decls)} (core 698 + bonus {len(decls)-698})")

# ---- Bảng gate: name -> index trong kVersionGateTable ----------------------
gate_rows, _core_sets, _compat_sets = gvg.build_table()
gate_index = {nm: i for i, (nm, _m) in enumerate(gate_rows)}
missing_gate = [n for (_r, n, _p) in decls if n not in gate_index]
if missing_gate:
    print(f"[c_exports] WARN {len(missing_gate)} lệnh không có trong gate table: "
          f"{missing_gate[:5]}")

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
    f.write("// Mỗi wrapper đo thời gian gọi (tglmt::GlProfileScope) để log gltop#\n")
    f.write("// chỉ ra hàm gl* nào nuốt frame time.\n")
    f.write("// Version-gate: entry point không có trong (version, profile) của context\n")
    f.write("// hiện tại ⇒ GL_INVALID_OPERATION, không thực thi (GL 4.6 spec §2.3).\n")
    f.write('#include "tglmt/gl46.h"\n')
    f.write('#include "tglmt/gl_legacy.h"\n')
    f.write('#include "tglmt/GlProfile.h"\n')
    f.write('#include "tglmt/VersionGate.h"\n')
    f.write("using namespace tglmt;\n")
    f.write("namespace tglmt { namespace gl {\n")
    f.write("void glGetPointerv(GLenum p, void** v);\n")
    f.write("void glLineStipple(GLint a, GLushort b);\n")
    f.write("void glDepthRangeArrayfvNV(GLuint a, GLsizei b, const GLfloat* c);\n")
    f.write("} }\n")
    # Forward declaration extension entry points (địa chỉ hàm cho getProcAddress).
    if ext_sigs:
        f.write("\nextern \"C\" {\n")
        for nm in sorted(ext_sigs):
            ret, params = ext_sigs[nm]
            f.write(f"{ret} {nm}({params if params else 'void'});\n")
        f.write("}\n")
    f.write("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n")
    for i, (ret, name, params) in enumerate(decls):
        plist = params if params else "void"
        args = ", ".join(param_names(params))
        gi = gate_index.get(name, -1)
        f.write(f"{ret} {name}({plist}) {{\n")
        f.write(f"    GlProfileScope _gp({i});\n")
        if gi >= 0:
            f.write(f"    if (!tglmt::GateCheck({gi})) ")
            d = default_return(ret)
            if ret.strip() == "void":
                f.write("return;\n")
            else:
                f.write(f"return {d if d is not None else 'nullptr'};\n")
        if ret.strip() == "void":
            f.write(f"    gl::{name}({args});\n")
        else:
            f.write(f"    return gl::{name}({args});\n")
        f.write("}\n")
    f.write("\n#ifdef __cplusplus\n} // extern \"C\"\n#endif\n")
    # Bảng slot: định nghĩa ở đây vì chỉ file này biết danh sách + thứ tự tên.
    f.write("\nnamespace tglmt {\n")
    f.write("GlCallStat gGlProfile[kGlProfileMax];\n")
    f.write("const char* const gGlProfileName[kGlProfileMax] = {\n")
    for i, (_, name, _) in enumerate(decls):
        f.write(f'    "{name}", // {i}\n')
    f.write("};\n")
    f.write(f"int gGlProfileN = {len(decls)};\n")
    # Bảng tra địa chỉ hàm cho getProcAddress (C ABI, tên phẳng).
    # PHẢI sort theo tên để getProcAddress binary search được.
    proc_rows = sorted(
        [(name, f"(void*)&{name}") for (_r, name, _p) in decls] +
        [(nm, f"(void*)&{nm}") for nm in ext_names],
        key=lambda r: r[0])
    f.write("const TglmtProcEntry kTglmtProcTable[] = {\n")
    for name, fn in proc_rows:
        f.write(f'    {{"{name}", {fn}}},\n')
    f.write("};\n")
    f.write(f"const int kTglmtProcCount = {len(proc_rows)};\n")
    f.write("} // namespace tglmt\n")
if len(decls) > 2048:
    raise SystemExit(f"[c_exports] FATAL: {len(decls)} > kGlProfileMax (2048)")

# --- public C ABI header -----------------------------------------------------
with open(OUT_C_API, "w") as f:
    f.write("// AUTO-GENERATED bởi tools/gen_c_exports.py — KHÔNG SỬA TAY.\n")
    f.write("// Public C ABI của TGLMT. Đây là API mà app/dylopen dùng (dlsym \"glXxx\").\n")
    f.write("// Mọi entry point ở đây đều đi qua version-gate: entry point không tồn tại\n")
    f.write("// trong (version, profile) của context hiện tại ⇒ GL_INVALID_OPERATION.\n")
    f.write("//\n")
    f.write("// Bao gồm cả OpenGL 1.0–4.6 Core + Compatibility (fixed-function legacy).\n")
    f.write("#pragma once\n#include <stdint.h>\n#include <stddef.h>\n\n")
    f.write("/* ---- Kiểu C của OpenGL ---- */\n")
    f.write("typedef uint32_t GLenum;   typedef uint8_t  GLboolean;\n")
    f.write("typedef uint32_t GLbitfield; typedef int8_t  GLbyte;\n")
    f.write("typedef int16_t  GLshort;   typedef int32_t  GLint;\n")
    f.write("typedef int32_t  GLsizei;   typedef uint8_t  GLubyte;\n")
    f.write("typedef uint16_t GLushort;  typedef uint32_t GLuint;\n")
    f.write("typedef uint64_t GLuint64;  typedef int64_t  GLint64;\n")
    f.write("typedef float    GLfloat;   typedef double   GLdouble;\n")
    f.write("typedef char     GLchar;    typedef intptr_t  GLintptr;\n")
    f.write("typedef intptr_t GLsizeiptr;\n")
    f.write("typedef struct TGLMTSync* GLsync;\n")
    f.write("typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id,\n")
    f.write("                            GLenum severity, GLsizei length,\n")
    f.write("                            const GLchar* message, const void* userParam);\n")
    f.write("\n#ifdef __cplusplus\nextern \"C\" {\n#endif\n")
    for (ret, name, params) in decls:
        plist = params if params else "void"
        f.write(f"{ret} {name}({plist});\n")
    f.write("\n#ifdef __cplusplus\n} /* extern \"C\" */\n#endif\n")
print(f"[c_exports] wrote {OUT_C_API} ({len(decls)} C ABI decls)")
print(f"[c_exports] wrote {OUT} ({len(decls)} profiled slots)")
