#!/usr/bin/env python3
"""coverage_dashboard.py — P7: Dashboard độ phủ extension (Markdown report).

Đọc kExts (advertised) + gl.xml (requirements) + src/gl (implemented) → báo cáo:
  - Tổng quan: advertised / missing / tỷ lệ.
  - Theo nhóm: debug, buffer, texture, framebuffer, shader, sync, query, DSA, v.v.
  - Sẵn sàng implement (missing ít hàm, giá trị cao).
  - Honestly-absent (Metal-impossible hoặc legacy — xem docs/HONESTLY_ABSENT.md).

Usage:
  python3 tools/coverage_dashboard.py > docs/COVERAGE_DASHBOARD.md
  python3 tools/coverage_dashboard.py --ready  # chỉ ext sẵn sàng implement
"""
import sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import ext_coverage as ec

def category(name):
    """Nhóm extension theo area."""
    n = name.lower()
    if "debug" in n or "marker" in n or "label" in n: return "debug"
    if "buffer" in n or "storage" in n: return "buffer/storage"
    if "texture" in n: return "texture"
    if "framebuffer" in n or "renderbuffer" in n: return "framebuffer"
    if "shader" in n or "program" in n or "uniform" in n: return "shader/program"
    if "sync" in n or "fence" in n or "event" in n: return "sync"
    if "query" in n or "timer" in n or "statistics" in n or "occlusion" in n: return "query"
    if "direct_state" in n or "dsa" in n: return "DSA"
    if "compute" in n or "dispatch" in n: return "compute"
    if "image" in n: return "image"
    if "atomic" in n: return "atomic"
    if "vertex" in n or "attrib" in n or "array" in n: return "vertex"
    if "sample" in n or "multisample" in n or "msaa" in n: return "multisample"
    if "blend" in n or "logic" in n: return "blend"
    if "depth" in n or "stencil" in n or "clip" in n: return "depth/stencil"
    if "robust" in n or "no_error" in n or "reset" in n: return "robustness"
    if "copy" in n or "blit" in n or "clear" in n or "invalidate" in n: return "copy/clear"
    if "map" in n or "read" in n or "pixel" in n: return "pixel-transfer"
    if "egl" in n or "cl_" in n or "x11" in n: return "external (EGL/CL/X11)"
    if "angle" in n or "apple" in n or "arm" in n or "qcom" in n or "sun" in n: return "vendor-specific"
    return "other"

def main():
    core, exts = ec.parse_glxml()
    impl = ec.implemented_funcs()
    kexts = ec.current_kexts()
    mobile = {n: d for n, d in exts.items()
              if ec.vendor(n) in ec.MOBILE_VENDOR or ec.vendor(n) == "OTHER"}

    only_ready = "--ready" in sys.argv

    # Phân loại advertised.
    adv_by_cat = {}
    for e in kexts:
        c = category(e)
        adv_by_cat.setdefault(c, []).append(e)

    # Phân loại missing (mobile-relevant, có hàm thật).
    miss_by_cat = {}
    ready = []  # (name, n_missing, n_alias) — ít hàm, dễ làm
    for name, d in sorted(mobile.items()):
        if name in kexts: continue
        need = d["funcs"] - core
        if not need: continue  # enum-only (không cần hàm)
        kind, real, alias = ec.classify(name, need, impl, core)
        if kind != "partial": continue
        c = category(name)
        miss_by_cat.setdefault(c, []).append((name, len(real), len(alias), real[:3]))
        if len(real) <= 3:  # ít hàm → sẵn sàng
            ready.append((name, len(real), len(alias)))

    if not only_ready:
        print("# Coverage Dashboard (TGLMT Extension)")
        print()
        print(f"**Advertised: {len(kexts)}** | **Missing (có hàm): {sum(len(v) for v in miss_by_cat.values())}**")
        print()
        print("## Theo nhóm (advertised)")
        print()
        print("| Nhóm | Số lượng | Extensions |")
        print("|---|---|---|")
        for c in sorted(adv_by_cat):
            es = adv_by_cat[c]
            print(f"| {c} | {len(es)} | {', '.join(sorted(es)[:5])}{'...' if len(es)>5 else ''} |")
        print()
        print("## Theo nhóm (missing, có hàm thật)")
        print()
        print("| Nhóm | Số lượng | Ví dụ |")
        print("|---|---|---|")
        for c in sorted(miss_by_cat):
            ms = miss_by_cat[c]
            ex = ", ".join(f"{n}({r})" for n, r, a, s in ms[:3])
            print(f"| {c} | {len(ms)} | {ex}{'...' if len(ms)>3 else ''} |")
        print()
    print("## Sẵn sàng implement (≤3 hàm thật)")
    print()
    if ready:
        for n, r, a in sorted(ready):
            print(f"- {n} ({r} hàm, {a} alias)")
    else:
        print("(không có — mọi ext missing đều cần >3 hàm)");
    print()
    if not only_ready:
        print("---")
        print("Xem `docs/HONESTLY_ABSENT.md` để biết ext nào KHÔNG làm và vì sao.")
        print("Quy tắc vàng: advertise ⇔ implement ⇔ test.")

if __name__ == "__main__":
    main()
