#!/usr/bin/env python3
"""Generate the OpenGL 4.6 core/glcore extension truth matrix.

Reads docs/khronos/gl.xml, kExts[] and exported symbols, joins the curated
feasibility metadata in docs/ext_feasibility.json, and writes:
  docs/EXTENSION_MATRIX.md
  docs/ext_matrix.json
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
FEASIBILITY = ROOT / "docs/ext_feasibility.json"
# Prefer the library that the current test build produced. The old default
# (build-apple) often does not exist, which silently turned every advertised
# extension into a false "advertised-lie". TGLMT_LIB overrides the search.
def _default_lib():
    env = os.environ.get("TGLMT_LIB")
    if env:
        return pathlib.Path(env)
    for rel in ("build/libtglmt.dylib", "build-apple/libtglmt.dylib",
                "build-iphoneos/libtglmt.a"):
        candidate = ROOT / rel
        if candidate.exists():
            return candidate
    return ROOT / "build/libtglmt.dylib"

DEFAULT_LIB = _default_lib()


def parse_registry():
    root = ET.parse(str(GLXML)).getroot()
    core = set()
    for feature in root.findall("feature"):
        if feature.get("api") != "gl":
            continue
        name = feature.get("name") or ""
        if not name.startswith("GL_VERSION_") or "ES" in name:
            continue
        try:
            version = tuple(int(x) for x in name[len("GL_VERSION_"):].split("_"))
        except ValueError:
            continue
        if version > (4, 6):
            continue
        for req in feature.findall("require"):
            for cmd in req.findall("command"):
                if cmd.get("name"):
                    core.add(cmd.get("name"))
    for feature in root.findall("feature"):
        for req in feature.findall("remove"):
            for cmd in req.findall("command"):
                if cmd.get("name"):
                    core.discard(cmd.get("name"))

    exts = {}
    for ext in root.find("extensions").findall("extension"):
        name = ext.get("name")
        if not name:
            continue
        row = exts.setdefault(name, {
            "supported": set(),
            "funcs": set(),
            "enums": set(),
            "alias": set(),
        })
        row["supported"].update((ext.get("supported") or "").split("|"))
        for req in ext.findall("require"):
            for cmd in req.findall("command"):
                cmd_name = cmd.get("name")
                if not cmd_name:
                    continue
                row["funcs"].add(cmd_name)
                if cmd.get("alias"):
                    row["alias"].add(cmd_name)
            for enum in req.findall("enum"):
                if enum.get("name"):
                    row["enums"].add(enum.get("name"))
    return core, exts


def read_kexts():
    text = GL_ERR.read_text()
    match = re.search(r"kExts\s*\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not match:
        return []
    return re.findall(r'"(GL_[A-Za-z0-9_]+)"', match.group(1))


def read_symbols(path):
    if not path.exists():
        return set(), f"missing library: {path}"
    flag = "-gU" if path.suffix == ".dylib" else "-g"
    proc = subprocess.run(["nm", flag, str(path)], capture_output=True, text=True)
    if proc.returncode != 0:
        return set(), proc.stderr.strip()
    symbols = set()
    for line in proc.stdout.splitlines():
        match = re.search(r"\b_?(gl[A-Za-z0-9_]+)$", line)
        if match:
            symbols.add(match.group(1))
    return symbols, None


def load_feasibility():
    if not FEASIBILITY.exists():
        return {"extensions": {}}
    return json.loads(FEASIBILITY.read_text())


def status_for(advertised, funcs, missing):
    if advertised:
        return "advertised-lie" if missing else "advertised"
    if funcs and missing:
        return "partial"
    if funcs:
        return "symbol-complete"
    return "behavior-only"


def build_rows(core, exts, kexts, symbols, feasibility):
    rows = []
    for name, info in exts.items():
        in_glcore = "glcore" in info["supported"]
        if not in_glcore and name not in kexts:
            continue
        funcs = sorted(info["funcs"])
        missing = [f for f in funcs if f not in symbols]
        meta = feasibility.get("extensions", {}).get(name, {})
        rows.append({
            "name": name,
            "scope": "glcore" if in_glcore else "gl-only",
            "advertised": name in kexts,
            "funcs": len(funcs),
            "missing_count": len(missing),
            "missing": missing,
            "status": status_for(name in kexts, funcs, missing),
            "class": meta.get("class", "verify"),
            "gate": meta.get("gate", ""),
            "reason": meta.get("reason", ""),
        })
    rows.sort(key=lambda r: (-r["missing_count"], r["name"]))
    return rows


def summarize(core, rows, kexts, symbols):
    glcore = sum(1 for r in rows if r["scope"] == "glcore")
    adv = sum(1 for r in rows if r["advertised"])
    missing_ext = sum(1 for r in rows if r["missing_count"])
    missing_funcs = sum(r["missing_count"] for r in rows)
    print("=== TGLMT extension truth matrix ===")
    print(f"core functions: {len(core)}")
    print(f"exported gl* symbols: {len(symbols)}")
    print(f"glcore extensions: {glcore}")
    print(f"rows (glcore + advertised non-glcore): {len(rows)}")
    print(f"advertised (kExts): {len(kexts)} total, {adv} in rows")
    print(f"glcore with missing functions: {missing_ext} ({missing_funcs} functions)")
    print(f"glcore without missing functions: {glcore - missing_ext}")
    for status in ("advertised", "advertised-lie", "partial", "symbol-complete", "behavior-only"):
        count = sum(1 for r in rows if r["status"] == status)
        if count:
            print(f"  {status:16} {count}")
    print("top missing:")
    for row in rows[:12]:
        if not row["missing_count"]:
            break
        print(f"  {row['name']:48} {row['missing_count']:4}  {', '.join(row['missing'][:4])}")


def write_markdown(rows, core, kexts, symbols, path):
    lines = []
    lines.append("# TGLMT OpenGL 4.6 Core / `glcore` extension matrix")
    lines.append("")
    lines.append("Generated by `python3 tools/ext_matrix.py --write`. Do not edit by hand.")
    lines.append("")
    lines.append(f"- Core functions: **{len(core)}**")
    lines.append(f"- Exported `gl*` symbols: **{len(symbols)}**")
    lines.append(f"- Advertised extensions (`kExts`): **{len(kexts)}**")
    lines.append(f"- `glcore` extensions: **{sum(1 for r in rows if r['scope'] == 'glcore')}**")
    lines.append(f"- Advertised non-`glcore` extensions included: **{sum(1 for r in rows if r['scope'] == 'gl-only')}**")
    lines.append(f"- Rows with missing entry points: **{sum(1 for r in rows if r['missing_count'])}**")
    lines.append("")
    lines.append("`status`: `advertised`, `advertised-lie` (advertised but missing symbols), "
                 "`partial`, `symbol-complete` (not advertised), `behavior-only`. "
                 "`class`: curated feasibility from `docs/ext_feasibility.json`.")
    lines.append("")
    lines.append("| Extension | Scope | Advertised | Funcs | Missing | Status | Class | Gate / reason |")
    lines.append("|---|---|---:|--:|--:|---|---|---|")
    for row in rows:
        note = row["gate"] or row["reason"]
        note = note.replace("|", "\\|")
        lines.append(
            f"| `{row['name']}` | {row['scope']} | {'yes' if row['advertised'] else 'no'} | "
            f"{row['funcs']} | {row['missing_count']} | {row['status']} | "
            f"{row['class']} | {note} |"
        )
    lines.append("")
    lines.append("## Missing entry points by extension")
    lines.append("")
    for row in rows:
        if not row["missing_count"]:
            continue
        lines.append(f"### `{row['name']}` ({row['missing_count']})")
        lines.append("")
        lines.append(", ".join(f"`{name}`" for name in row["missing"]))
        lines.append("")
    path.write_text("\n".join(lines) + "\n")


def write_json(rows, core, kexts, symbols, path):
    payload = {
        "core_functions": len(core),
        "exported_symbols": len(symbols),
        "advertised_extensions": len(kexts),
        "glcore_extensions": len(rows),
        "extensions": rows,
    }
    path.write_text(json.dumps(payload, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--lib", type=pathlib.Path, default=DEFAULT_LIB)
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--strict", action="store_true",
                        help="exit 1 on missing symbols or advertised-lie rows")
    args = parser.parse_args()

    core, exts = parse_registry()
    kexts = read_kexts()
    symbols, error = read_symbols(args.lib)
    if error:
        print(f"[ext_matrix] {error}", file=sys.stderr)
    rows = build_rows(core, exts, kexts, symbols, load_feasibility())
    summarize(core, rows, kexts, symbols)

    if args.write:
        write_markdown(rows, core, kexts, symbols, ROOT / "docs/EXTENSION_MATRIX.md")
        write_json(rows, core, kexts, symbols, ROOT / "docs/ext_matrix.json")
        print("wrote docs/EXTENSION_MATRIX.md, docs/ext_matrix.json")

    if args.strict:
        lies = [r["name"] for r in rows if r["status"] == "advertised-lie"]
        if error:
            print(f"[ext_matrix] strict: {error}", file=sys.stderr)
            return 1
        if lies:
            print(f"[ext_matrix] strict: {len(lies)} advertised-lie extension(s): "
                  + ", ".join(lies[:10]), file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
