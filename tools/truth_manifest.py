#!/usr/bin/env python3
"""Generate a per-symbol truth manifest for TGLMT.

This is the measurement layer for the honesty rule:
    advertise => symbol present => behavior implemented => test exists

It joins:
  * docs/khronos/gl.xml          (what OpenGL 4.6 core / extensions require)
  * src/gl/gl_error_string_debug.cpp (what TGLMT advertises)
  * exported gl* symbols         (what the built library actually exports)
  * src/gl/*.cpp                 (where a function is implemented, and how)
  * tests/{unit,integration}/*.cpp (which tests mention the function)

The output is deliberately conservative. "real" only means the source body
is not empty/constant/forwarding; it is not proof of correctness. Correctness
evidence is added by the differential/CTS/device phases.

Usage:
  python3 tools/truth_manifest.py                 # summary
  python3 tools/truth_manifest.py --write         # tests/truth/api_manifest.json + docs/TRUTH_MANIFEST.md
  python3 tools/truth_manifest.py --strict        # exit 1 on missing symbol / advertised no-op
  python3 tools/truth_manifest.py --require-tests # strict + every advertised ext has a test ref
"""
import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import gl46_catalog

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
GL_ERR = ROOT / "src/gl/gl_error_string_debug.cpp"
SRC_GL = ROOT / "src/gl"
TEST_DIRS = (
    ROOT / "tests/unit",
    ROOT / "tests/integration",
    ROOT / "tests/regression",
    ROOT / "tests/api",
    ROOT / "tests/conformance",
    ROOT / "tests/shaders",
)
OUT_JSON = ROOT / "tests/truth/api_manifest.json"
OUT_MD = ROOT / "docs/TRUTH_MANIFEST.md"

# Generated wrappers are not the implementation of record. Aliases are parsed
# separately so an extension entry point can still be marked as an alias.
GENERATED = {
    "gl_c_exports.cpp",
    "gl_ext_aliases_gen.cpp",
    "gl_stubs_gen.cpp",
    "gl_dispatch_gen.cpp",
}


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def parse_core_versions():
    """Return {function: sorted versions} for the GL 4.6 core profile.

    Delegates to tools/gl46_catalog.py so the measurement layer and the code
    generators share ONE derivation.  The previous local copy had two defects:
    it ignored ``profile="compatibility"`` on ``<require>`` (42 compat-only
    entry points were counted as core) and it applied ``<remove>`` blocks
    after all ``<require>`` blocks instead of in version order (``glGetPointerv``
    -- removed for core in 3.2, re-required in 4.3 -- was dropped).
    """
    core = gl46_catalog.parse_core_commands(GLXML)
    return {fn: [ver] for fn, ver in core.items()}


def parse_compat_only_versions():
    """Return {function: version} for compatibility-profile-only entry points.

    These are implemented and exported by TGLMT but are NOT OpenGL 4.6 Core.
    """
    return dict(gl46_catalog.parse_compat_only_commands(GLXML))


def parse_extensions():
    """Return every GL extension with its required functions and glcore scope."""
    root = ET.parse(str(GLXML)).getroot()
    out = {}
    for ext in root.find("extensions").findall("extension"):
        name = ext.get("name")
        if not name:
            continue
        supported = set((ext.get("supported") or "").split("|"))
        funcs = set()
        for req in ext.findall("require"):
            for cmd in req.findall("command"):
                if cmd.get("name"):
                    funcs.add(cmd.get("name"))
        out[name] = {"funcs": sorted(funcs), "glcore": "glcore" in supported}
    return out


def read_kexts():
    text = GL_ERR.read_text()
    match = re.search(r"kExts\s*\[\]\s*=\s*\{(.*?)\};", text, re.S)
    return re.findall(r'"(GL_[A-Za-z0-9_]+)"', match.group(1)) if match else []


def read_symbols(path):
    if not path or not path.exists():
        return set(), f"missing library: {path}"
    flag = "-gU" if path.suffix == ".dylib" else "-g"
    proc = subprocess.run(["nm", flag, str(path)], capture_output=True, text=True)
    if proc.returncode != 0:
        return set(), proc.stderr.strip()
    syms = set()
    for line in proc.stdout.splitlines():
        m = re.search(r"\b_?(gl[A-Za-z0-9_]+)$", line)
        if m:
            syms.add(m.group(1))
    return syms, None


def default_lib():
    env = pathlib.Path(os.environ["TGLMT_LIB"]) if os.environ.get("TGLMT_LIB") else None
    if env:
        return env
    for rel in ("build/libtglmt.dylib", "build-apple/libtglmt.dylib",
                "build-iphoneos/libtglmt.a"):
        p = ROOT / rel
        if p.exists():
            return p
    return ROOT / "build/libtglmt.dylib"


def scan_defs(path, aliases=False):
    """Yield (function, kind, body_excerpt) for definitions in one source file."""
    text = strip_comments(path.read_text(errors="replace"))
    if aliases:
        for m in re.finditer(
                r"void\s+(gl[A-Za-z0-9_]+)\s*\([^)]*\)\s*\{\s*"
                r"(?:tglmt::)?gl::(gl[A-Za-z0-9_]+)\s*\(", text):
            yield m.group(1), "alias", m.group(2)
        return
    # A definition starts at a line beginning with a return type / qualifiers and
    # has a top-level `{`. Calls inside `if (...)` do not start a line with a type.
    pat = re.compile(
        r"(?m)^[ \t]*(?:[A-Za-z_][\w:<>\*&,\s]*?)\b(gl[A-Za-z0-9_]+)\s*"
        r"\([^;{}]*\)\s*\{")
    for m in pat.finditer(text):
        start = m.end() - 1
        depth = 0
        quote = None
        esc = False
        i = start
        end = None
        while i < len(text):
            c = text[i]
            if quote:
                if esc:
                    esc = False
                elif c == "\\":
                    esc = True
                elif c == quote:
                    quote = None
            else:
                if c in "\"'":
                    quote = c
                elif c == "{":
                    depth += 1
                elif c == "}":
                    depth -= 1
                    if depth == 0:
                        end = i + 1
                        break
            i += 1
        if end is None:
            continue
        body = text[start + 1:end - 1]
        yield m.group(1), classify_body(body), body.strip()[:160]

    # Macro-generated definitions.  The line-oriented regex above cannot see
    # functions emitted by a macro invocation, e.g.
    #     #define PK_N(fn, idx, n)  void fn(...) {...} void fn##v(...) {...}
    #     PK_N(glVertexP2ui, 0, 2);
    # or
    #     #define GLMT_UNIFORM_MATRIX_FV(fn, cols, rows) void fn(...) {...}
    #     GLMT_UNIFORM_MATRIX_FV(glUniformMatrix2fv, 2, 2)
    # Without this branch those functions were mislabelled "symbol-only"
    # even though `nm` proves they are defined text symbols.
    #
    # Dò tên macro TỔNG QUÁT: mọi "#define NAME(...)" có '{' trong thân đều
    # là macro sinh hàm; mọi lời gọi "NAME(glXxx, ...)" sinh ra hàm glXxx.
    macro_defs = {}
    for m in re.finditer(r"(?m)^[ \t]*#\s*define\s+([A-Z][A-Z0-9_]*)\s*\(([^)]*)\)(.*)$",
                         text):
        # Thân macro có thể trải nhiều dòng (nối bằng backslash) → phải gom
        # TOÀN BỘ thân, nếu không sẽ không thấy dấu '{' nằm ở dòng sau.
        body = m.group(3)
        end = m.end()
        while body.rstrip().endswith("\\") and end < len(text):
            nl = text.find("\n", end)
            if nl < 0:
                break
            body += "\n" + text[end:nl]
            end = nl + 1
        macro_defs[m.group(1)] = body
    if not macro_defs:
        return
    for name, body in list(macro_defs.items()):
        if "{" not in body:
            continue
        for inv in re.finditer(r"\b" + re.escape(name) + r"\s*\(\s*(gl[A-Za-z0-9_]+)",
                               text):
            base = inv.group(1)
            yield base, "real", "macro-generated"
            # Token-paste "fn##v" → sinh thêm biến thể "v".
            if "##" in body and base.endswith("ui"):
                yield base + "v", "real", "macro-generated"



# No-ops that are explicitly allowed by the GL spec (performance hints or
# operations whose post-state is undefined). Everything else is a static gap.
#
# NOTE: the previous version of this function granted "allowed:discard" to ANY
# function whose name started with "glInvalidate", purely by name prefix.  That
# is not a spec check: the *discard* half of the behaviour is legal (the
# post-invalidate contents are undefined), but GL 4.6 still mandates error
# generation for bad arguments.  The five Invalidate entry points below did
# not generate those errors, so they are recorded as gaps rather than being
# silently excused.
NOOP_ALLOWED = {
    # GL 4.6 §17.2: hints may be silently ignored.  (glHint still has to
    # raise INVALID_ENUM for an unknown target -- tracked separately.)
    "glHint",
    # Pure hint, no errors listed in the spec.
    "glReleaseShaderCompiler",
}


def noop_triage(fn):
    if fn in NOOP_ALLOWED:
        return "allowed:hint"
    return "gap"


def classify_body(body):
    clean = re.sub(r"\(void\)\s*[A-Za-z_][A-Za-z0-9_]*\s*;", "", body)
    clean = re.sub(r"\breturn\s*;", "", clean).strip()
    if not clean:
        return "noop"
    if re.fullmatch(r"return\s+(?:-?\d+|GL_[A-Z0-9_]+|true|false|nullptr)\s*;", clean):
        return "constant"
    if re.fullmatch(r"(?:tglmt::)?gl::(gl[A-Za-z0-9_]+)\s*\([^;]*\)\s*;", clean):
        return "alias"
    return "real"


# --- Enum correctness -------------------------------------------------------
# Bug class #1 (đã gặp thật trong TGLMT): hàm ghi state dưới MÃ ENUM KHÔNG tồn tại
# trong registry (vd 0x8C51 thay vì GL_MIN_SAMPLE_SHADING_VALUE=0x8C37) → query
# đọc lại luôn trả 0, app tưởng chưa set. Đây là "real" về hình thức nên
# classify_body không bắt được. Ta đối chiếu MỌI hằng hex dùng làm pname trong
# SetShadow/GetShadow với danh sách enum của gl.xml.
def _enum_values():
    """{enum_name: value} cho mọi enum trong gl.xml."""
    root = ET.parse(str(GLXML)).getroot()
    out = {}
    for e in root.iter("enum"):
        n, v = e.get("name"), e.get("value")
        if n and v:
            try:
                out.setdefault(n, int(v, 0))
            except ValueError:
                pass
    return out


def _valid_pnames():
    """Tập giá trị hợp lệ cho pname: mọi enum có group GetPName/EnableCap/
    GetPName, cộng mọi giá trị enum bất kỳ (state pname riêng của app)."""
    root = ET.parse(str(GLXML)).getroot()
    vals = set()
    for e in root.iter("enum"):
        v = e.get("value")
        if not v:
            continue
        try:
            vals.add(int(v, 0))
        except ValueError:
            pass
    return vals


# Pname "private" của TGLMT: dùng để tra cứu nội bộ, không phải enum GL.
# Danh sách này phải được cập nhật khi thêm pname mới.
_PRIVATE_PNAMES = {
    0x825A,  # PROGRAM_PIPELINE_BINDING (đã là enum GL thật — giữ để rà)
}


def find_bad_enum_pnames():
    """Trả về [(file, line, hexvalue, hint)] cho mọi SetShadow/GetShadow dùng
    hằng hex KHÔNG phải enum nào trong gl.xml.

    Đây không phải lỗi tuyệt đối (TGLMT có pname nội bộ), nhưng mọi pname
    PHẢI được glGet trả lại đúng → hằng sai = query sai. Đánh dấu để review.
    """
    valid = _valid_pnames()
    out = []
    for path in sorted(SRC_GL.glob("*.cpp")):
        if path.name in GENERATED:
            continue
        for i, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
            if "SetShadow" not in line and "GetShadow" not in line:
                continue
            for m in re.finditer(r"0[xX]([0-9A-Fa-f]+)", line):
                try:
                    v = int(m.group(1), 16)
                except ValueError:
                    continue
                if v in valid:
                    continue
                out.append((str(path.relative_to(ROOT)), i, hex(v), line.strip()[:110]))
    return out


# --- Shadow-only detection --------------------------------------------------
# Bug class #2: hàm chỉ ghi state (SetShadow) mà KHÔNG hành động gì khác —
# state được set nhưng draw path / pipeline không đọc → app đặt xong không
# thấy tác dụng. classify_body gọi đây là "real" nên không bắt được.
def find_shadow_only():
    """Trả về [(fn, file)] cho hàm mà body CHỈ ghi state mà state đó KHÔNG
    được đọc lại ở bất kỳ đâu trong src/.

    Bug class #2 (đã gặp thật): hàm chỉ ghi state (SetShadow) mà draw path /
    pipeline không đọc → app đặt xong không thấy tác dụng. classify_body gọi
    đây là "real" nên không bắt được.
    """
    # Gom mọi pname được ĐỌC lại (GetShadow) trong toàn src.
    read_pnames = set()
    for path in sorted(SRC_GL.glob("*.cpp")):
        if path.name in GENERATED:
            continue
        for line in path.read_text(errors="replace").splitlines():
            if "GetShadow" not in line:
                continue
            for m in re.finditer(r"0[xX]([0-9A-Fa-f]+)", line):
                read_pnames.add(int(m.group(1), 16))
    # Và mọi pname được đọc qua accessor có tên (vd GetScissor(idx)).
    out = []
    for path in sorted(SRC_GL.glob("*.cpp")):
        if path.name in GENERATED:
            continue
        text = strip_comments(path.read_text(errors="replace"))
        for fn, kind, body in scan_defs(path):
            if kind != "real":
                continue
            calls = re.findall(r"([A-Za-z_][\w:.]*)\s*\(", body)
            setters = ("SetShadow", "SetEnabled", "SetEnabledI", "SetColorMask",
                       "SetPolygonOffset", "SetCullFace", "SetFrontFace")
            sig = [c for c in calls if c.split(".")[-1] in setters]
            if not sig:
                continue
            other = [c for c in calls if c.split(".")[-1] not in setters]
            # Hàm có side effect thật ngoài state (Record lỗi, flush encoder,
            # gọi hàm khác) → không phải shadow-only.
            if any(c.split(".")[-1] in ("Record", "std", "if", "for", "return")
                   or c.startswith("c.state") for c in other):
                continue
            if any(c.split(".")[-1] not in
                   ("SetShadow", "SetEnabled", "SetEnabledI", "SetColorMask",
                    "SetPolygonOffset", "SetCullFace", "SetFrontFace", "GetShadow")
                   for c in other):
                continue
            # Hằng pname mà hàm này ghi ra.
            written = set()
            for m in re.finditer(r"0[xX]([0-9A-Fa-f]+)", body):
                written.add(int(m.group(1), 16))
            if not written:
                continue
            # Nếu pname nào trong số đó được GetShadow đọc lại → state có
            # đường vào → không phải shadow-only.
            if written & read_pnames:
                continue
            out.append((fn, str(path.relative_to(ROOT)), sorted(hex(v) for v in written)))
    return out


def build_impl_map():
    impl = {}
    for path in sorted(SRC_GL.glob("*.cpp")):
        if path.name in GENERATED:
            continue
        for fn, kind, body in scan_defs(path):
            row = {"file": str(path.relative_to(ROOT)), "kind": kind, "body": body}
            # Prefer a real implementation over a generated/constant duplicate.
            order = {"real": 5, "alias": 4, "constant": 2, "noop": 1}
            if fn not in impl or order.get(kind, 0) > order.get(impl[fn]["kind"], 0):
                impl[fn] = row
    alias_path = SRC_GL / "gl_ext_aliases_gen.cpp"
    if alias_path.exists():
        for fn, kind, target in scan_defs(alias_path, aliases=True):
            impl.setdefault(fn, {"file": str(alias_path.relative_to(ROOT)),
                                 "kind": "alias", "target": target, "body": ""})
    return impl


def test_tokens():
    tokens = {}
    for d in TEST_DIRS:
        if not d.exists():
            continue
        for path in sorted(d.glob("*.cpp")):
            text = strip_comments(path.read_text(errors="replace"))
            names = set(re.findall(r"\bgl[A-Za-z0-9_]+\b", text))
            tokens[str(path.relative_to(ROOT))] = names
    return tokens


def git_state():
    try:
        rev = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"],
                                      cwd=ROOT, text=True).strip()
        dirty = bool(subprocess.check_output(["git", "status", "--porcelain"],
                                             cwd=ROOT, text=True).strip())
        return rev + ("+dirty" if dirty else "")
    except Exception:
        return "unknown"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lib", type=pathlib.Path, default=None)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--strict", action="store_true")
    ap.add_argument("--require-tests", action="store_true")
    args = ap.parse_args()

    lib = args.lib or default_lib()
    symbols, sym_err = read_symbols(lib)
    core_versions = parse_core_versions()
    all_ext = parse_extensions()
    kexts = read_kexts()
    impl = build_impl_map()
    tests = test_tokens()

    def refs(fn):
        return sorted(p for p, names in tests.items() if fn in names)

    core_rows = []
    for fn in sorted(core_versions):
        meta = impl.get(fn, {})
        core_rows.append({
            "name": fn,
            "versions": core_versions[fn],
            "symbol": fn in symbols,
            "impl_file": meta.get("file"),
            "impl_kind": meta.get("kind", "missing" if fn not in symbols else "symbol-only"),
            "noop_triage": (noop_triage(fn)
                            if meta.get("kind") in ("noop", "constant") else None),
            "tests": refs(fn),
        })

    # Compatibility-profile-only entry points.  Implemented and exported, but
    # NOT part of the OpenGL 4.6 Core Profile -- tracked separately so the
    # core coverage number is never inflated by them.
    compat_rows = []
    for fn in sorted(parse_compat_only_versions()):
        meta = impl.get(fn, {})
        compat_rows.append({
            "name": fn,
            "versions": [parse_compat_only_versions()[fn]],
            "symbol": fn in symbols,
            "impl_file": meta.get("file"),
            "impl_kind": meta.get("kind", "missing" if fn not in symbols else "symbol-only"),
            "profile": "compatibility",
            "tests": refs(fn),
        })

    ext_rows = []
    for name in sorted(all_ext):
        info = all_ext[name]
        if not info["glcore"] and name not in kexts:
            continue
        funcs = info["funcs"]
        missing = [f for f in funcs if f not in symbols]
        noops = [f for f in funcs if impl.get(f, {}).get("kind") in ("noop", "constant")]
        test_refs = sorted({
            p for p, names in tests.items()
            if name in names or any(f in names for f in funcs)
        })
        ext_rows.append({
            "name": name,
            "scope": "glcore" if info["glcore"] else "gl-only",
            "advertised": name in kexts,
            "funcs": len(funcs),
            "missing_symbols": missing,
            "noop_funcs": noops,
            "tests": test_refs,
            "has_behavior_test": bool(test_refs),
        })

    adv = [r for r in ext_rows if r["advertised"]]
    adv_missing = [r for r in adv if r["missing_symbols"]]
    adv_noop = [r for r in adv if r["noop_funcs"]]
    adv_no_test = [r for r in adv if not r["has_behavior_test"]]
    core_no_test = [r for r in core_rows if not r["tests"]]
    core_noop = [r for r in core_rows if r["impl_kind"] in ("noop", "constant")]
    core_noop_gaps = [r for r in core_noop if r["noop_triage"] == "gap"]
    core_missing = [r for r in core_rows if not r["symbol"]]

    print("=== TGLMT truth manifest ===")
    print(f"library: {lib} ({'found' if lib.exists() else 'MISSING'})")
    if sym_err:
        print(f"symbols: {sym_err}")
    print(f"core functions: {len(core_rows)} | with symbol: {len(core_rows) - len(core_missing)} | missing: {len(core_missing)}")
    print(f"core without direct test refs: {len(core_no_test)}")
    print(f"compat-profile-only functions (NOT core): {len(compat_rows)} "
          f"| missing symbol: {len([r for r in compat_rows if not r['symbol']])}")
    print(f"core no-op/constant bodies: {len(core_noop)} ({len(core_noop_gaps)} static gaps, {len(core_noop) - len(core_noop_gaps)} allowed)")
    print(f"extension rows: {len(ext_rows)} | advertised: {len(adv)}")
    print(f"advertised missing symbols: {len(adv_missing)}")
    print(f"advertised no-op/constant: {len(adv_noop)}")
    print(f"advertised with no direct test refs: {len(adv_no_test)}")

    # Bug class #1: pname ghi state bằng hằng không phải enum GL nào → query sai.
    bad_enum = find_bad_enum_pnames()
    print(f"state pnames not in gl.xml registry: {len(bad_enum)}")
    # Bug class #2: hàm chỉ ghi state, không có hành động quan sát được.
    shadow_only = find_shadow_only()
    print(f"state-only (shadow-only) functions: {len(shadow_only)}")

    if args.write:
        payload = {
            "schema": 1,
            "generated_by": "tools/truth_manifest.py",
            "git": git_state(),
            "library": str(lib.relative_to(ROOT)) if lib.is_relative_to(ROOT) else str(lib),
            "summary": {
                "core_functions": len(core_rows),
                "compat_only_functions": len(compat_rows),
                "core_catalog_source": "Khronos gl.xml, reg.py PASS-1 profile semantics (tools/gl46_catalog.py)",
                "core_missing_symbol": len(core_missing),
                "core_without_test_refs": len(core_no_test),
                "core_noop_or_constant": len(core_noop),
                "core_noop_static_gaps": len(core_noop_gaps),
                "extension_rows": len(ext_rows),
                "advertised_extensions": len(adv),
                "advertised_missing_symbols": len(adv_missing),
                "advertised_noop_or_constant": len(adv_noop),
                "advertised_without_test_refs": len(adv_no_test),
            },
            "core": core_rows,
            "compat_only": compat_rows,
            "extensions": ext_rows,
        }
        OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
        OUT_JSON.write_text(json.dumps(payload, indent=2) + "\n")

        lines = [
            "# TGLMT truth manifest",
            "",
            "Generated by `python3 tools/truth_manifest.py --write`. Do not edit by hand.",
            "",
            f"- Git: `{git_state()}`",
            f"- Library: `{payload['library']}`",
            f"- Core functions: **{len(core_rows)}** (canonical OpenGL 4.6 Core Profile, Khronos registry)",
            f"- Compatibility-profile-only functions (NOT core): **{len(compat_rows)}**",
            f"- Core missing symbol: **{len(core_missing)}**",
            f"- Core without direct test references: **{len(core_no_test)}**",
            f"- Core no-op/constant bodies: **{len(core_noop)}** (**{len(core_noop_gaps)} static gaps**, {len(core_noop) - len(core_noop_gaps)} spec-allowed)",
            f"- Extension rows (glcore + advertised): **{len(ext_rows)}**",
            f"- Advertised extensions: **{len(adv)}**",
            f"- Advertised missing symbols: **{len(adv_missing)}**",
            f"- Advertised no-op/constant: **{len(adv_noop)}**",
            f"- Advertised without direct test references: **{len(adv_no_test)}**",
            "",
            "`impl_kind` is source-shape evidence only: `real` means the body is",
            "not empty/constant/forwarding. It is not proof of spec correctness.",
            "",
            "## Advertised extensions with no test reference",
            "",
        ]
        lines += [f"- `{r['name']}`" for r in adv_no_test] or ["- none"]
        lines += ["", "## Advertised extensions with no-op/constant entry points", ""]
        if adv_noop:
            for r in adv_noop:
                lines.append(f"- `{r['name']}`: " + ", ".join(f"`{f}`" for f in r["noop_funcs"][:12]))
        else:
            lines.append("- none")
        lines += ["", "## Core functions with no direct test reference (first 120)", ""]
        lines += [f"- `{r['name']}` ({', '.join(r['versions'])})" for r in core_no_test[:120]]
        if len(core_no_test) > 120:
            lines.append(f"- ... {len(core_no_test) - 120} more (see `tests/truth/api_manifest.json`)")
        lines += ["", "## Core no-op/constant bodies (gap unless marked allowed)", ""]
        lines += [f"- `{r['name']}`: {r['impl_kind']} [{r['noop_triage']}] ({r['impl_file'] or '?'})"
                  for r in core_noop] or ["- none"]
        lines += ["", "## Static behavior gaps (no-op/constant, not spec-allowed)", ""]
        lines += [f"- `{r['name']}` ({', '.join(r['versions'])}) — {r['impl_kind']} in `{r['impl_file'] or '?'}`"
                  for r in core_noop_gaps] or ["- none"]
        lines.append("")
        OUT_MD.write_text("\n".join(lines) + "\n")
        print(f"wrote {OUT_JSON.relative_to(ROOT)}, {OUT_MD.relative_to(ROOT)}")

    if args.strict and (sym_err or adv_missing or adv_noop):
        print("[strict] failing: missing symbols or advertised no-op/constant entry points", file=sys.stderr)
        return 1
    if args.require_tests and (adv_missing or adv_noop or adv_no_test):
        print("[strict] failing: advertised extension lacks symbol/behavior/test evidence", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
