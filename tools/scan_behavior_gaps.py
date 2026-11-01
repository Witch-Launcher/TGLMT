#!/usr/bin/env python3
"""Scan the implementation for explicit honesty markers and unsupported paths.

Unlike ext_behavior_audit.py (which proves empty/no-op entry points for
advertised extensions), this scanner finds comments and branches that say a
feature is skipped, shadow-only, unsupported or still TODO. It is a triage
list, not proof: every hit is reviewed before being called a gap.

Usage:
  python3 tools/scan_behavior_gaps.py
  python3 tools/scan_behavior_gaps.py --json build/behavior_gaps.json
"""
import argparse
import json
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_DIRS = [ROOT / "src/gl", ROOT / "src/metal", ROOT / "src/core"]
HEADER_DIRS = [ROOT / "include/tglmt"]

MARKERS = [
    ("unimplemented", re.compile(
        r"chưa hỗ trợ|chưa có|chưa làm|chưa wire|chưa nối|chưa triển khai|"
        r"unsupported|not implemented|not supported|not wired|missing backend",
        re.I)),
    ("skipped", re.compile(
        r"bỏ qua|skip(?:ped)?|return;\s*//\s*(?:skip|no-op)|no-op|stub", re.I)),
    ("shadow-only", re.compile(
        r"shadow[- ]only|chỉ (?:lưu )?shadow|chỉ (?:lưu )?state|trace[- ]only|"
        r"chỉ (?:ghi )?trace|không (?:áp|dùng) (?:vào )?(?:draw|gpu)|"
        r"ignored by (?:the )?draw|draw path ignores", re.I)),
    ("rejected", re.compile(
        r"từ chối|reject(?:ed|s)?|record\(0x0502\)|0x0502", re.I)),
    ("todo", re.compile(r"\bTODO\b|\bFIXME\b|\bXXX\b", re.I)),
]
FUNC_RE = re.compile(r"\b(gl[A-Za-z0-9_]{2,}|[A-Za-z_][A-Za-z0-9_:<>]*)\s*\([^;{}]*\)\s*\{?")
SKIP_FILES = {"gl_stubs_gen.cpp", "gl_ext_aliases_gen.cpp", "gl_dispatch_gen.cpp"}


def iter_lines():
    for base in SOURCE_DIRS + HEADER_DIRS:
        if not base.exists():
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in (".cpp", ".mm", ".h"):
                continue
            if path.name in SKIP_FILES:
                continue
            rel = path.relative_to(ROOT)
            try:
                lines = path.read_text().splitlines()
            except UnicodeDecodeError:
                continue
            for no, line in enumerate(lines, 1):
                yield rel, no, line


def enclosing_function(lines, no):
    for i in range(no - 1, max(-1, no - 80), -1):
        hit = FUNC_RE.search(lines[i])
        if hit:
            name = hit.group(1)
            if name.startswith("gl") or "::" in name:
                return name
    return ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", default="")
    ap.add_argument("--limit", type=int, default=120)
    args = ap.parse_args()
    hits = []
    by_file = {}
    by_kind = {}
    for rel, no, line in iter_lines():
        for kind, rx in MARKERS:
            if not rx.search(line):
                continue
            # A line can match several marker classes; keep the first specific one.
            if kind == "rejected" and "0x0502" in line and not re.search(
                    r"từ chối|reject", line, re.I):
                # Error records alone are not gaps; only take explicit comments.
                continue
            src = line.strip()
            if not src or src.startswith("//") and len(src) < 12:
                continue
            fn = enclosing_function(rel.read_text().splitlines(), no)
            item = {"file": str(rel), "line": no, "kind": kind,
                    "function": fn, "text": src[:240]}
            hits.append(item)
            by_file[str(rel)] = by_file.get(str(rel), 0) + 1
            by_kind[kind] = by_kind.get(kind, 0) + 1
            break
    print(f"behavior gap candidates: {len(hits)}")
    for kind, n in sorted(by_kind.items(), key=lambda x: (-x[1], x[0])):
        print(f"  {kind:14} {n}")
    print("top files:")
    for name, n in sorted(by_file.items(), key=lambda x: (-x[1], x[0]))[:12]:
        print(f"  {n:4}  {name}")
    for item in hits[:args.limit]:
        fn = f" {item['function']}" if item["function"] else ""
        print(f"{item['file']}:{item['line']}{fn}  [{item['kind']}] {item['text']}")
    if len(hits) > args.limit:
        print(f"... {len(hits) - args.limit} more (use --limit)")
    if args.json:
        out = pathlib.Path(args.json)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps({"total": len(hits), "by_kind": by_kind,
                                   "by_file": by_file, "hits": hits}, indent=2) + "\n")
        print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
