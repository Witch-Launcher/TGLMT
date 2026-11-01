#!/usr/bin/env python3
"""Fetch the official Khronos spec for every OpenGL `glcore` extension.

Writes docs/khronos/extensions/<VENDOR>/<EXT>.txt plus manifest.json with the
source URL, SHA-256 and byte size. Existing files are replaced only when the
downloaded content is non-empty.
"""
import hashlib
import json
import pathlib
import sys
import time
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]
GLXML = ROOT / "docs/khronos/gl.xml"
OUT = ROOT / "docs/khronos/extensions"
BASE = "https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/extensions"
UA = "TGLMT-spec-fetch/1.0 (+https://github.com/anomalyco/opencode)"


def glcore_extensions():
    root = ET.parse(str(GLXML)).getroot()
    names = []
    for ext in root.find("extensions").findall("extension"):
        if "glcore" in (ext.get("supported") or "").split("|"):
            names.append(ext.get("name"))
    return sorted(set(names))


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def candidates(name):
    vendor = name.split("_")[1]
    stem = name[len("GL_"):]
    return [
        f"{BASE}/{vendor}/{stem}.txt",
        f"{BASE}/{vendor}/{stem}.gl.txt",
    ]


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    only_missing = "--only-missing" in sys.argv
    manifest = {}
    previous_missing = {}
    if only_missing and (OUT / "manifest.json").exists():
        previous = json.loads((OUT / "manifest.json").read_text())
        manifest = dict(previous.get("extensions", {}))
        previous_missing = {
            item["name"]: item for item in previous.get("missing", [])
            if isinstance(item, dict) and item.get("name")
        }
    missing = {name: item for name, item in previous_missing.items()}
    todo = glcore_extensions()
    if only_missing:
        todo = [n for n in todo if n not in manifest]
    for index, name in enumerate(todo, 1):
        vendor = name.split("_")[1]
        data = None
        used_url = candidates(name)[0]
        for url in candidates(name):
            for attempt in range(3):
                try:
                    data = fetch(url)
                    used_url = url
                    break
                except urllib.error.HTTPError as error:
                    if error.code == 404:
                        break
                    time.sleep(0.5 * (attempt + 1))
                except Exception:
                    time.sleep(0.5 * (attempt + 1))
            if data:
                break
        if not data:
            # Preserve curated annotations (covered_by/note) across reruns.
            missing.setdefault(name, {"name": name, "url": candidates(name)[0]})
            print(f"[{index:3}/{len(todo)}] MISS {name}")
            continue
        filename = pathlib.Path(used_url).name
        path = OUT / vendor / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        manifest[name] = {
            "path": str(path.relative_to(ROOT)),
            "url": used_url,
            "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        }
        print(f"[{index:3}/{len(todo)}] ok   {name} ({len(data)} B)")
        time.sleep(0.05)

    missing = [missing[name] for name in sorted(missing)]
    (OUT / "manifest.json").write_text(json.dumps({
        "source": BASE,
        "extensions": manifest,
        "missing": missing,
    }, indent=2) + "\n")
    print(f"fetched {len(manifest)}, missing {len(missing)}")
    return 0 if not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
