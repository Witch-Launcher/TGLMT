#!/usr/bin/env python3
"""pack_conformance.py — quét toàn bộ shader pack qua tiền xử lý Iris + converter TGLMT.

Giả lập đúng thứ tự Iris: include expansion + jcpp conditional → patch gl_FragData/
texture2D → ConvertGLSLtoMSL. In ra PASS/FAIL từng program để fix lần lượt.

Dùng:
  clang++ -std=c++17 -O1 tools/glsl2msl_cli.cpp src/gl/glsl_to_msl.cpp -Iinclude -o /tmp/glsl2msl
  python3 tools/pack_conformance.py PACK_SHADERS_DIR --cli /tmp/glsl2msl [--world world0]
"""
import argparse
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pack_glsl_preprocess import (disabled_programs, iris_standard_defines,  # noqa: E402
                                     preprocess)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack")
    ap.add_argument("--cli", required=True, help="glsl2msl_cli binary")
    ap.add_argument("--world", default="world0")
    ap.add_argument("--define", action="append", default=[])
    ap.add_argument("--only", default=None, help="chỉ chạy program khớp substring")
    ap.add_argument("--keep", default=None, help="thư mục lưu source đã preprocess")
    args = ap.parse_args()

    pack = os.path.abspath(args.pack)
    wdir = os.path.join(pack, args.world)
    if not os.path.isdir(wdir):
        print(f"không thấy {wdir}", file=sys.stderr)
        return 2

    defines = iris_standard_defines()
    for d in args.define:
        k, v = d.split("=", 1) if "=" in d else (d, "1")
        defines[k] = v

    disabled = disabled_programs(pack, defines)

    stages = {".vsh": "vsh", ".fsh": "fsh", ".gsh": "gsh",
              ".tcs": "tcs", ".tes": "tes", ".csh": "csh"}
    files = sorted(f for f in os.listdir(wdir) if os.path.splitext(f)[1] in stages)
    ok = fail = 0
    results = []
    tmpd = args.keep or tempfile.mkdtemp(prefix="tglmt-pack-")
    os.makedirs(tmpd, exist_ok=True)
    for fn in files:
        name, ext = os.path.splitext(fn)
        if args.only and args.only not in name:
            continue
        if f"{args.world}/{name}" in disabled or name in disabled:
            results.append(("SKIP", fn, "disabled in shaders.properties"))
            continue
        src, _ = preprocess(pack, f"{args.world}/{name}", stages[ext], defines, iris_patch=True)
        if src is None:
            continue
        out = os.path.join(tmpd, f"{name}.{stages[ext]}.glsl")
        with open(out, "w", encoding="utf-8") as f:
            f.write(src)
        r = subprocess.run([args.cli, out, stages[ext]], capture_output=True, text=True)
        if r.returncode == 0:
            ok += 1
            results.append(("PASS", fn, ""))
        else:
            fail += 1
            msg = (r.stdout + r.stderr).strip().splitlines()
            msg = msg[0] if msg else "?"
            results.append(("FAIL", fn, msg))
            print(f"FAIL {fn}: {msg}")

    print(f"\n{ok} PASS / {fail} FAIL ({len(files)} file, {args.world}, src={tmpd})")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
