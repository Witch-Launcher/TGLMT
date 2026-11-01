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

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
GL_ERR = ROOT / "src/gl/gl_error_string_debug.cpp"
SRC_GL = ROOT / "src/gl"
TEST_DIRS = (ROOT / "tests/unit", ROOT / "tests/integration")
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
    """Return {function: sorted versions} for the GL 4.6 core profile."""
    root = ET.parse(str(GLXML)).getroot()
    current = {}
    for feature in root.findall("feature"):
        if feature.get("api") != "gl":
            continue
        name = feature.get("name") or ""
        if not name.startswith("GL_VERSION_") or "ES" in name:
            continue
        try:
            ver = name[len("GL_VERSION_"):].replace("_", ".")
        except ValueError:
            continue
        for req in feature.findall("require"):
            for cmd in req.findall("command"):
                fn = cmd.get("name")
                if fn:
                    current.setdefault(fn, set()).add(ver)
    # Match the project's definition of "GL 4.6 core": anything removed by a
    # later core feature is not part of the 4.6 core surface.
    for feature in root.findall("feature"):
        if feature.get("api") != "gl":
            continue
        for rm in feature.findall("remove"):
            for cmd in rm.findall("command"):
                fn = cmd.get("name")
                if fn:
                    current.pop(fn, None)
    return {fn: sorted(vers, key=lambda v: tuple(int(x) for x in v.split(".")))
            for fn, vers in current.items()}


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


# No-ops that are explicitly allowed by the GL spec (performance hints or
# operations whose post-state is undefined). Everything else is a static gap.
def noop_triage(fn):
    if fn in {"glHint", "glReleaseShaderCompiler"}:
        return "allowed:hint"
    if fn.startswith("glInvalidate"):
        return "allowed:discard"
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
    print(f"core no-op/constant bodies: {len(core_noop)} ({len(core_noop_gaps)} static gaps, {len(core_noop) - len(core_noop_gaps)} allowed)")
    print(f"extension rows: {len(ext_rows)} | advertised: {len(adv)}")
    print(f"advertised missing symbols: {len(adv_missing)}")
    print(f"advertised no-op/constant: {len(adv_noop)}")
    print(f"advertised with no direct test refs: {len(adv_no_test)}")

    if args.write:
        payload = {
            "schema": 1,
            "generated_by": "tools/truth_manifest.py",
            "git": git_state(),
            "library": str(lib.relative_to(ROOT)) if lib.is_relative_to(ROOT) else str(lib),
            "summary": {
                "core_functions": len(core_rows),
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
            f"- Core functions: **{len(core_rows)}**",
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
