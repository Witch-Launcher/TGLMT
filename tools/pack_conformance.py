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
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pack_glsl_preprocess import (disabled_programs, iris_standard_defines,  # noqa: E402
                                     preprocess)


def find_metal():
    """xcrun metal khả dụng? (extension phải `.metal` — `.msl` bị coi là linker
    input, exit 0 không tạo .air — bug harness cũ.)"""
    if sys.platform != "darwin" or not shutil.which("xcrun"):
        return None
    try:
        r = subprocess.run(["xcrun", "-sdk", "macosx", "--find", "metal"],
                           capture_output=True, text=True)
    except OSError:
        return None
    return "xcrun" if r.returncode == 0 and r.stdout.strip() else None


def remap_temp_locs(text):
    """Mô phỏng LinkProgram.RewriteLocns: location tạm (>=900) → 0..n.
    Runtime luôn làm bước này trước khi Metal compile; harness phải khớp."""
    for pat, fmt in ((r"\[\[attribute\((\d+)\)\]\]", "[[attribute(%d)]]"),
                     (r"\[\[user\(locn(\d+)\)\]\]", "[[user(locn%d)]]")):
        seen = []

        def repl(m):
            n = int(m.group(1))
            if n < 900:
                return m.group(0)
            if n not in seen:
                seen.append(n)
            return fmt % seen.index(n)

        text = re.sub(pat, repl, text)
    return text


def compile_case(tag, src, tmpd, cli, metal):
    """Convert 1 source (tag = `<name>.<stage>`) + metal -c nếu có. → (status, msg)."""
    out = os.path.join(tmpd, tag + ".glsl")
    with open(out, "w", encoding="utf-8") as f:
        f.write(src)
    msl = os.path.join(tmpd, tag + ".metal")
    r = subprocess.run([cli, out, tag.rsplit(".", 1)[-1], "-o", msl],
                       capture_output=True, text=True)
    if r.returncode != 0:
        msg = (r.stdout + r.stderr).strip().splitlines()
        return "FAIL", (msg[0] if msg else "?")
    if metal:
        air = os.path.join(tmpd, tag + ".air")
        with open(msl, encoding="utf-8") as f:
            metal_src = remap_temp_locs(f.read())
        with open(msl, "w", encoding="utf-8") as f:
            f.write(metal_src)
        rm = subprocess.run(["xcrun", "-sdk", "macosx", "metal", "-c", msl, "-o", air],
                            capture_output=True, text=True)
        errs = [l for l in rm.stderr.splitlines() if "error:" in l]
        if rm.returncode != 0 or errs:
            return "FAIL-METAL", (errs[0] if errs else f"metal exit {rm.returncode}")
    return "PASS", ""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack")
    ap.add_argument("--cli", required=True, help="glsl2msl_cli binary")
    ap.add_argument("--world", default="world0")
    ap.add_argument("--define", action="append", default=[])
    ap.add_argument("--only", action="append", default=None, help="chỉ chạy program khớp substring")
    ap.add_argument("--with-clrwl", action="store_true", help="compile cả clrwl_* (colorwheel)")
    ap.add_argument("--with-dh", action="store_true", help="compile cả dh_* (Distant Horizons)")
    ap.add_argument("--no-ie", action="store_true",
                    help="bỏ biến thể IE_COMPAT (mặc định có: gbuffers_block + shadow)")
    ap.add_argument("--keep", default=None, help="thư mục lưu source đã preprocess")
    ap.add_argument("--no-metal", action="store_true", help="bỏ bước metal compile")
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
    metal = None if args.no_metal else find_metal()
    for fn in files:
        name, ext = os.path.splitext(fn)
        if args.only and not any(o in name for o in args.only):
            continue
        if f"{args.world}/{name}" in disabled or name in disabled:
            results.append(("SKIP", fn, "disabled in shaders.properties"))
            continue
        # CLRWL (Colorwheel): pack này thiếu định nghĩa clrwl_computeFragment
        # (bản Colorwheel riêng mới có); COLORED_LIGHTING=0 → runtime không compile.
        if name.startswith("clrwl_") and not args.with_clrwl:
            results.append(("SKIP", fn, "clrwl: colorwheel off"))
            continue
        # Distant Horizons (dh_*): chỉ compile khi cài mod DH (user không dùng).
        if name.startswith("dh_") and not args.with_dh:
            results.append(("SKIP", fn, "dh: Distant Horizons không cài"))
            continue
        # IE_COMPAT / IE_COMPAT_SHADOW: Iris load mọi ShaderKey.values() nên pack
        # nào cũng compile thêm bản `uniform ivec2 iris_LightUV;` cho gbuffers_block
        # (+ shadow entities) — VanillaTransformer.java:92-96. Từng lọt bug
        # `float4(tglmt_u.iris_LightUV, 0.0f, 1.0f)` ra tới game vì harness không
        # mô phỏng nhánh isIE.
        variants = [False]
        if not args.no_ie and ext == ".vsh" and name in ("gbuffers_block", "shadow"):
            variants.append(True)
        for ie in variants:
            src, _ = preprocess(pack, f"{args.world}/{name}", stages[ext], defines,
                                iris_patch=True, ie=ie)
            if src is None:
                continue
            tag = f"{name}{'_ie' if ie else ''}.{stages[ext]}"
            st, msg = compile_case(tag, src, tmpd, args.cli, metal)
            results.append((st, tag, msg))
            if st == "PASS":
                ok += 1
            else:
                fail += 1
                print(f"{st} {tag}: {msg}")

    print(f"\n{ok} PASS / {fail} FAIL ({ok + fail} case, {len(files)} file, "
          f"{args.world}, src={tmpd})")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
